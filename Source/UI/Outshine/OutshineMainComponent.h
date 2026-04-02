// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
// OutshineMainComponent — UI for the Outshine DSP engine.
// Part of the Originate workflow (Export Pyramid).
// See Docs/export-architecture.md for the full architecture.
#include <juce_gui_basics/juce_gui_basics.h>
#include "../GalleryColors.h"
#include "../../XOceanusProcessor.h"
#include "../../Export/XOutshine.h"
#include "OutshineShellState.h"
#include "OutshineInputPanel.h"
#include "OutshineGrainStrip.h"
#include "OutshineAutoMode.h"
#include "OutshineExportBar.h"
#include "OutshinePreviewPlayer.h"

namespace xoceanus {

enum class OutshineState { Shell, Input, Preview, Exporting };

class OutshineMainComponent : public juce::Component
{
public:
    explicit OutshineMainComponent(XOceanusProcessor& /*processorRef*/)
    {
        setWantsKeyboardFocus(true);
        A11y::setup(*this, "Outshine Main", "Sample instrument forge — drag, analyze, export");

        // Create all child components
        shellState = std::make_unique<OutshineShellState>();
        inputPanel = std::make_unique<OutshineInputPanel>();
        grainStrip = std::make_unique<OutshineGrainStrip>();
        autoMode   = std::make_unique<OutshineAutoMode>();
        exportBar  = std::make_unique<OutshineExportBar>();
        previewPlayer = std::make_unique<OutshinePreviewPlayer>();

        addAndMakeVisible(*shellState);
        addAndMakeVisible(*inputPanel);
        addAndMakeVisible(*grainStrip);
        addAndMakeVisible(*autoMode);
        addAndMakeVisible(*exportBar);
        addAndMakeVisible(*previewPlayer);

        // Wire callbacks
        shellState->onFilesDropped = [this](const juce::StringArray& files) {
            onGrainsChanged(files);
        };

        inputPanel->onFilesSelected = [this](const juce::StringArray& files) {
            grainStrip->addGrains(files);
        };

        grainStrip->onGrainsChanged = [this](const juce::StringArray& paths) {
            onGrainsChanged(paths);
        };

        autoMode->getZoneMap().onZoneClicked = [](int /*grainIndex*/) {
            // In Phase 1A: no scrolling action — just a click event placeholder
        };

        exportBar->onExportClicked = [this](const juce::String& pearlName,
                                            ExportFormat format,
                                            const juce::File& outputPath) {
            onExportClicked(pearlName, format, outputPath);
        };

        exportBar->onCancelClicked = [this]() {
            cancelFlag = true;
        };

        // Initial state
        transitionToState(OutshineState::Shell);
        setSize(900, 660);
    }

    ~OutshineMainComponent() override
    {
        cancelPipeline();
    }

    void cancelPipeline()
    {
        cancelFlag = true;
        outshine.cancel();
        // Wait for background thread to finish (500ms timeout)
        analysisComplete_.wait(500);
    }

    void onGrainsChanged(const juce::StringArray& grainPaths)
    {
        currentGrains = grainPaths;

        if (grainPaths.isEmpty())
        {
            transitionToState(OutshineState::Shell);
            return;
        }

        // Add grains to strip if not already there
        grainStrip->addGrains(grainPaths);
        transitionToState(OutshineState::Input);
        exportBar->setGrainCount(grainPaths.size());

        // Run analysis on background thread
        runAnalysis(grainPaths);
    }

    void onExportClicked(const juce::String& pearlName,
                         ExportFormat format,
                         const juce::File& outputPath)
    {
        runPipeline(pearlName, format, outputPath);
    }

    OutshineExportBar& getExportBar() { return *exportBar; }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(GalleryColors::get(GalleryColors::shellWhite()));
    }

    void resized() override
    {
        auto area = getLocalBounds();

        // Export bar always pinned at bottom
        auto exportBarArea = area.removeFromBottom(kExportBarH);
        auto grainArea     = area.removeFromBottom(kGrainStripH);

        exportBar->setBounds(exportBarArea);
        grainStrip->setBounds(grainArea);

        // Preview player: small button in auto mode area
        // (positioned by autoMode internally in Phase 1B; for now, just below grain strip)

        // Remaining area split by state
        if (uiState == OutshineState::Shell)
        {
            shellState->setBounds(area);
            shellState->setVisible(true);
            inputPanel->setVisible(false);
            autoMode->setVisible(false);
        }
        else if (uiState == OutshineState::Input)
        {
            inputPanel->setBounds(area);
            shellState->setVisible(false);
            inputPanel->setVisible(true);
            autoMode->setVisible(false);
        }
        else // Preview or Exporting
        {
            auto left  = area.removeFromLeft(area.getWidth() / 2);
            inputPanel->setBounds(left);
            autoMode->setBounds(area);
            shellState->setVisible(false);
            inputPanel->setVisible(true);
            autoMode->setVisible(true);

            // Preview player floats in the autoMode area
            previewPlayer->setBounds(area.getX() + area.getWidth() - 128,
                                     area.getY() + 4, 120, 32);
        }
    }

    bool keyPressed(const juce::KeyPress& key) override
    {
        if (key == juce::KeyPress::escapeKey)
        {
            // Find parent DocumentWindow and close it
            if (auto* parent = findParentComponentOfClass<juce::DocumentWindow>())
                parent->closeButtonPressed();
            return true;
        }
        return false;
    }

private:
    void transitionToState(OutshineState newState)
    {
        uiState = newState;

        bool showGrainStrip = (newState != OutshineState::Shell);
        grainStrip->setVisible(showGrainStrip);

        bool showExportBar = (newState == OutshineState::Preview || newState == OutshineState::Exporting);
        exportBar->setVisible(showExportBar);

        previewPlayer->setVisible(newState == OutshineState::Preview);

        if (newState == OutshineState::Exporting)
            exportBar->setExporting(true);
        else
            exportBar->setExporting(false);

        resized();
    }

    void runAnalysis(const juce::StringArray& grainPaths)
    {
        cancelFlag = false;

        auto safeThis = juce::Component::SafePointer<OutshineMainComponent>(this);
        auto paths = grainPaths;
        // #355: Capture engine pointer before thread launch (on message thread).
        // The destructor sets cancelFlag=true and waits 500ms — if the component is
        // alive when we check cancelFlag here the pointer is still valid; if it's
        // about to be destroyed the destructor will wait for us to finish.
        auto* enginePtr = &outshine;
        auto& cancelRef = cancelFlag;

        // Background thread for analysis
        analysisComplete_.reset();
        juce::Thread::launch([safeThis, paths, enginePtr, &cancelRef]() {
            OutshineSettings settings;
            OutshineProgress progress;
            progress.cancelled = false;

            auto callback = [safeThis](OutshineProgress& p) {
                if (p.cancelled) return;
                float prog  = p.overallProgress;
                juce::String stg = p.stage;
                juce::MessageManager::callAsync([safeThis, prog, stg]() {
                    if (auto* w = safeThis.getComponent())
                        w->getExportBar().setProgress(prog, stg);
                });
            };

            bool success = false;
            if (!cancelRef.load())
                success = enginePtr->analyzeGrains(paths, settings, callback);

            juce::MessageManager::callAsync([safeThis, success]() {
                if (auto* w = safeThis.getComponent())
                {
                    if (success)
                    {
                        const auto& samples = w->outshine.getAnalyzedSamples();
                        w->autoMode->populate(samples);
                        w->previewPlayer->setSamples(samples);

                        // Count unverified pitches
                        int unverified = 0;
                        for (const auto& s : samples)
                            if (s.pitchConfidence < 0.15f)
                                ++unverified;
                        w->exportBar->setUnverifiedCount(unverified);
                        w->exportBar->setReadyToExport(true);

                        w->transitionToState(OutshineState::Preview);
                    }
                    else
                    {
                        // Analysis failed or cancelled — stay in Input state
                        w->exportBar->setProgress(0.0f, "Analysis failed");
                    }
                    w->analysisComplete_.signal();
                }
                else
                {
                    // Component was deleted — nothing to update, but still signal
                    // (can't signal through safeThis; signal is stored on the object,
                    // so if the object is gone there's nothing to signal — cancelPipeline
                    // will time out gracefully at 500ms)
                }
            });
        });
    }

    void runPipeline(const juce::String& pearlName,
                     ExportFormat /*format*/,
                     const juce::File& outputPath)
    {
        cancelFlag = false;
        transitionToState(OutshineState::Exporting);

        auto safeThis = juce::Component::SafePointer<OutshineMainComponent>(this);
        // #355: Capture engine pointer before thread launch (on message thread).
        auto* enginePtr = &outshine;
        auto& cancelRef = cancelFlag;

        analysisComplete_.reset();
        juce::Thread::launch([safeThis, pearlName, outputPath, enginePtr, &cancelRef]() {
            OutshineSettings settings;

            auto callback = [safeThis](OutshineProgress& p) {
                if (p.cancelled) return;
                float prog  = p.overallProgress;
                juce::String stg = p.stage;
                juce::MessageManager::callAsync([safeThis, prog, stg]() {
                    if (auto* w = safeThis.getComponent())
                        w->getExportBar().setProgress(prog, stg);
                });
            };

            bool success = false;
            if (!cancelRef.load())
                success = enginePtr->exportPearl(outputPath, settings, callback);

            juce::MessageManager::callAsync([safeThis, success, pearlName]() {
                if (auto* w = safeThis.getComponent())
                {
                    if (success)
                    {
                        w->exportBar->setProgress(1.0f, "Pearl complete: " + pearlName);
                    }
                    else
                    {
                        w->exportBar->setProgress(0.0f, "Export failed");
                    }
                    w->transitionToState(OutshineState::Preview);
                    w->analysisComplete_.signal();
                }
            });
        });
    }

    XOutshine outshine;
    OutshineState uiState { OutshineState::Shell };

    std::unique_ptr<OutshineShellState>    shellState;
    std::unique_ptr<OutshineInputPanel>    inputPanel;
    std::unique_ptr<OutshineGrainStrip>    grainStrip;
    std::unique_ptr<OutshineAutoMode>      autoMode;
    std::unique_ptr<OutshineExportBar>     exportBar;
    std::unique_ptr<OutshinePreviewPlayer> previewPlayer;

    juce::StringArray currentGrains;
    std::atomic<bool> cancelFlag { false };
    juce::WaitableEvent analysisComplete_ { true }; // manualReset=true; reset() before launch, signal() at end

    static constexpr int kGrainStripH  = 40;
    static constexpr int kExportBarH   = 60;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OutshineMainComponent)
};

} // namespace xoceanus
