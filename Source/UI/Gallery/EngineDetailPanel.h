// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "../../XOceanusProcessor.h"
#include "../GalleryColors.h"
#include "../CouplingColors.h"
#include "MacroHeroStrip.h"
#include "ParameterGrid.h"
#include "MidiLearnMouseListener.h"
#include "DrumDetailPanel.h"
#include "ObrixDetailPanel.h"
#include "WaveformDisplay.h"
#include "DrumPadGrid.h"
#include "SpecializedDisplays.h"
#include "SpecializedWidgets.h"
#include "ModMatrixDrawer.h"

namespace xoceanus
{

//==============================================================================
// ADSRDisplay — paint-only component that draws an ADSR envelope curve.
// Lives inside EngineDetailPanel, displayed next to the oscilloscope.
class ADSRDisplay : public juce::Component
{
public:
    void setValues(float attack, float decay, float sustain, float release)
    {
        a = attack;
        d = decay;
        s = sustain;
        r = release;
        repaint();
    }

    void setAccentColour(juce::Colour c)
    {
        accent = c;
        repaint();
    }

    void paint(juce::Graphics& g) override
    {
        auto b = getLocalBounds().toFloat().reduced(4, 2);

        // Background
        g.setColour(juce::Colour(0x08FFFFFF));
        g.fillRoundedRectangle(b, 3.0f);

        // ADSR curve
        // Divide width into 4 segments: Attack, Decay, Sustain, Release
        float totalW = b.getWidth();
        float h = b.getHeight();
        float x0 = b.getX();
        float y0 = b.getBottom(); // bottom = 0 level
        float yTop = b.getY();    // top = peak level
        float ySus = y0 - s * h;  // sustain level

        // Proportional widths (attack + decay take ~40%, sustain ~30%, release ~30%)
        float aW = a * totalW * 0.3f + totalW * 0.05f; // min 5% width
        float dW = d * totalW * 0.15f + totalW * 0.05f;
        float rW = r * totalW * 0.25f + totalW * 0.05f;
        float sW = totalW - aW - dW - rW;
        if (sW < 10.0f)
            sW = 10.0f;

        juce::Path curve;
        curve.startNewSubPath(x0, y0);            // start at bottom-left
        curve.lineTo(x0 + aW, yTop);              // attack peak
        curve.lineTo(x0 + aW + dW, ySus);         // decay to sustain
        curve.lineTo(x0 + aW + dW + sW, ySus);    // sustain hold
        curve.lineTo(x0 + aW + dW + sW + rW, y0); // release to zero

        // Draw the curve
        g.setColour(accent.withAlpha(0.7f));
        g.strokePath(curve, juce::PathStrokeType(2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // Draw dots at the 4 control points
        auto drawDot = [&](float cx, float cy)
        {
            g.setColour(accent.withAlpha(0.9f));
            g.fillEllipse(cx - 3, cy - 3, 6, 6);
        };
        drawDot(x0 + aW, yTop);              // attack peak
        drawDot(x0 + aW + dW, ySus);         // decay end
        drawDot(x0 + aW + dW + sW, ySus);    // sustain end
        drawDot(x0 + aW + dW + sW + rW, y0); // release end

        // A D S R labels at bottom
        // WCAG AAA fix: raised from 0.4f → 0.62f (≥7:1 at 10pt on dark ADSR display bg).
        g.setFont(GalleryFonts::body(10.0f)); // (#885: 9pt→10pt legibility floor)
        g.setColour(juce::Colour(200, 204, 216).withAlpha(0.62f));
        float labelY = b.getBottom() - 10;
        g.drawText("A", (int)(x0 + aW * 0.5f - 4), (int)labelY, 8, 10, juce::Justification::centred);
        g.drawText("D", (int)(x0 + aW + dW * 0.5f - 4), (int)labelY, 8, 10, juce::Justification::centred);
        g.drawText("S", (int)(x0 + aW + dW + sW * 0.5f - 4), (int)labelY, 8, 10, juce::Justification::centred);
        g.drawText("R", (int)(x0 + aW + dW + sW + rW * 0.5f - 4), (int)labelY, 8, 10, juce::Justification::centred);
    }

private:
    // #881: These defaults are only visible for <1 frame before the 30Hz timer
    // (timerCallback in EngineDetailPanel) pushes real APVTS values via setValues().
    // They are intentionally neutral so the initial paint is not misleading.
    float a = 0.1f, d = 0.3f, s = 0.7f, r = 0.4f;
    juce::Colour accent{0xFF1E8B7E};
};

//==============================================================================
// SubmarineSliderLnF — thick track + large thumb for ADSR sliders.
// Matches the submarine prototype's slider style.
class SubmarineSliderLnF : public juce::LookAndFeel_V4
{
public:
    void drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPos, float /*minPos*/, float /*maxPos*/,
                          juce::Slider::SliderStyle style, juce::Slider& slider) override
    {
        if (style != juce::Slider::LinearHorizontal)
        {
            juce::LookAndFeel_V4::drawLinearSlider(g, x, y, width, height, sliderPos, 0, 0, style, slider);
            return;
        }

        const float trackH = 6.0f;
        const float thumbR = 8.0f;
        const float cy = (float)y + (float)height * 0.5f;
        const float trackY = cy - trackH * 0.5f;
        const float trackX = (float)x + thumbR;
        const float trackW = (float)width - thumbR * 2.0f;

        // Background track (gray bar)
        g.setColour(slider.findColour(juce::Slider::backgroundColourId));
        g.fillRoundedRectangle(trackX, trackY, trackW, trackH, trackH * 0.5f);

        // Filled portion (teal bar)
        float fillW = sliderPos - trackX;
        if (fillW > 0.0f)
        {
            g.setColour(slider.findColour(juce::Slider::trackColourId));
            g.fillRoundedRectangle(trackX, trackY, juce::jmin(fillW, trackW), trackH, trackH * 0.5f);
        }

        // Thumb circle
        g.setColour(slider.findColour(juce::Slider::thumbColourId));
        g.fillEllipse(sliderPos - thumbR, cy - thumbR, thumbR * 2.0f, thumbR * 2.0f);
        // Subtle border on thumb
        g.setColour(juce::Colour(200, 204, 216).withAlpha(0.15f));
        g.drawEllipse(sliderPos - thumbR, cy - thumbR, thumbR * 2.0f, thumbR * 2.0f, 1.0f);
    }
};

//==============================================================================
// EngineDetailPanel — right-side parameter view for one engine slot.
// Contains a MacroHeroStrip (4 pillar sliders for engine macros) plus a
// scrollable ParameterGrid showing all remaining params.
class EngineDetailPanel : public juce::Component,
                          private juce::Timer
{
public:
    explicit EngineDetailPanel(XOceanusProcessor& proc) : processor(proc), macroHero(proc), waveformDisplay(proc), modMatrix_(proc.getAPVTS())
    {
        macroHero.setCompactMode(true); // Zone 2: no header, sliders fill height
        addAndMakeVisible(macroHero);
        addAndMakeVisible(viewport);
        // Disable scroll-on-drag — it steals vertical mouse drags from
        // RotaryVerticalDrag knobs inside the ParameterGrid.
        viewport.setScrollOnDragMode(juce::Viewport::ScrollOnDragMode::never);
        addAndMakeVisible(waveformDisplay);
        addAndMakeVisible(adsrDisplay);

        // ModMatrixDrawer — starts hidden; shown when the MOD tab is clicked.
        addChildComponent(modMatrix_);
        modMatrix_.onExpandedChanged = [this](bool)
        {
            resized();
            repaint();
        };

        // ADSR horizontal sliders — custom thick-track LookAndFeel
        static const char* adsrNames[] = {"A", "D", "S", "R"};
        for (int i = 0; i < 4; ++i)
        {
            adsrSliders[i].setLookAndFeel(&adsrSliderLnF);
            adsrSliders[i].setSliderStyle(juce::Slider::LinearHorizontal);
            adsrSliders[i].setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
            adsrSliders[i].setColour(juce::Slider::thumbColourId, juce::Colour(127, 219, 202));
            adsrSliders[i].setColour(juce::Slider::trackColourId, juce::Colour(60, 180, 170));
            adsrSliders[i].setColour(juce::Slider::backgroundColourId, juce::Colour(60, 70, 85));
            adsrSliders[i].setRange(0.0, 1.0, 0.001);
            adsrSliders[i].setValue(0.5);
            addAndMakeVisible(adsrSliders[i]);

            adsrSliderLabels[i].setText(adsrNames[i], juce::dontSendNotification);
            adsrSliderLabels[i].setFont(GalleryFonts::value(10.0f));
            adsrSliderLabels[i].setColour(juce::Label::textColourId, juce::Colour(127, 219, 202).withAlpha(0.6f));
            adsrSliderLabels[i].setJustificationType(juce::Justification::centred);
            addAndMakeVisible(adsrSliderLabels[i]);
        }

        A11y::setup(*this, "Engine Detail Panel",
                    "Shows parameters for the selected engine. Press Escape to return to overview.");
        // S5: Poll ADSR parameters at 30Hz (matches WaveformDisplay update rate).
        const int hz = A11y::prefersReducedMotion() ? 10 : 30;
        startTimerHz(hz);
    }

    ~EngineDetailPanel() override
    {
        stopTimer();
        for (auto& s : adsrSliders)
            s.setLookAndFeel(nullptr);
    }

    // Optional: wire MIDI learn manager before the first loadSlot() call.
    void setMidiLearnManager(MIDILearnManager* mgr)
    {
        learnManager = mgr;
        macroHero.setMidiLearnManager(mgr);
    }

    // Scroll the parameter grid viewport to a given section (MUST A1-05).
    // Called by XOceanusEditor::scrollDetailPanelToSection().
    void scrollToSection(ParameterGrid::Section s)
    {
        if (auto* grid = dynamic_cast<ParameterGrid*>(viewport.getViewedComponent()))
            grid->scrollToSection(s);
    }

    // Callback: fired when the back button is clicked. Parent should hide this panel.
    std::function<void()> onBackClicked;

    void mouseDown(const juce::MouseEvent& e) override
    {
        // Click on the collapsed MOD tab → expand the drawer.
        if (!modMatrix_.isExpanded() && modTabBounds_.contains(e.getPosition()))
        {
            modMatrix_.setExpanded(true);
            return;
        }

        // Back button (top-left, 32x32 at pad=12, y=20)
        auto backRect = juce::Rectangle<int>(12, 20, 32, 32);
        if (backRect.contains(e.getPosition()))
        {
            if (onBackClicked)
                onBackClicked();
        }
    }

    // Called when the user selects an engine slot to inspect.
    // Returns false if the slot is empty.
    bool loadSlot(int slot)
    {
        auto* eng = processor.getEngine(slot);
        if (!eng)
            return false;

        activeSlot_ = slot; // #903: persist slot for coupling route polling

        // W11: Track whether the engine actually changed so we only reset scroll
        // position on a genuine engine switch (not on parameter-only refreshes).
        const bool engineChanged = (eng->getEngineId() != engineId);

        engineId = eng->getEngineId();
        accentColour = eng->getAccentColour();

        // P30: cache toUpperCase() result to avoid String alloc in paint().
        cachedEngineName = engineId.toUpperCase();
        // P29: cache header gradient — rebuilt here and in resized(), not in paint().
        // #895: guard against zero-size bounds — ColourGradient with width=0 or
        // height=0 produces a degenerate gradient and causes rendering artefacts.
        // resized() will rebuild once the component is properly sized.
        if (getWidth() > 0 && getHeight() > 0)
            cachedHeaderGrad =
                juce::ColourGradient(accentColour.withAlpha(0.14f), 0.0f, 0.0f,
                                     juce::Colours::transparentBlack, (float)getWidth(), 0.0f, false);

        auto prefix = GalleryColors::prefixForEngine(engineId);

        // Configure waveform display for this slot
        waveformDisplay.setSlot(slot);
        waveformDisplay.setAccentColour(accentColour);
        waveformDisplay.setEngineId(engineId);

        // Configure ADSR display accent colour
        adsrDisplay.setAccentColour(accentColour);

        // S5: Discover ADSR parameters by scanning ALL engine params.
        // Engines use varied names — scan for keywords in the inner param name.
        {
            auto& apvts = processor.getAPVTS();
            juce::String pfx = prefix.endsWith("_") ? prefix : (prefix + "_");

            juce::String foundPids[4]; // A, D, S, R
            adsrAttack = adsrDecay = adsrSustain = adsrRelease = nullptr;

            for (auto* p : processor.getParameters())
            {
                auto* rp = dynamic_cast<juce::RangedAudioParameter*>(p);
                if (!rp) continue;
                juce::String pid = rp->getParameterID();
                if (!pid.startsWithIgnoreCase(pfx)) continue;

                juce::String inner = pid.substring(pfx.length()).toLowerCase();

                // Match attack / atk
                if (!adsrAttack && (inner.contains("attack") || inner.endsWith("atk")))
                {
                    adsrAttack = apvts.getRawParameterValue(pid);
                    foundPids[0] = pid;
                }
                // Match decay / dec
                else if (!adsrDecay && (inner.contains("decay") || inner.endsWith("dec")))
                {
                    adsrDecay = apvts.getRawParameterValue(pid);
                    foundPids[1] = pid;
                }
                // Match sustain / sus
                else if (!adsrSustain && (inner.contains("sustain") || inner.endsWith("sus")))
                {
                    adsrSustain = apvts.getRawParameterValue(pid);
                    foundPids[2] = pid;
                }
                // Match release / rel
                else if (!adsrRelease && (inner.contains("release") || inner.endsWith("rel")))
                {
                    adsrRelease = apvts.getRawParameterValue(pid);
                    foundPids[3] = pid;
                }
            }

            // Push immediate ADSR display update
            if (adsrAttack != nullptr)
            {
                adsrDisplay.setValues(
                    adsrAttack->load(),
                    adsrDecay   != nullptr ? adsrDecay->load()   : 0.3f,
                    adsrSustain != nullptr ? adsrSustain->load() : 0.7f,
                    adsrRelease != nullptr ? adsrRelease->load() : 0.4f);
            }

            // Wire ADSR horizontal sliders — show only if attack param found
            bool hasAdsr = (adsrAttack != nullptr);
            for (int i = 0; i < 4; ++i)
            {
                adsrAttachments[i].reset();
                if (foundPids[i].isNotEmpty())
                    adsrAttachments[i] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                        apvts, foundPids[i], adsrSliders[i]);
                adsrSliders[i].setVisible(hasAdsr);
                adsrSliderLabels[i].setVisible(hasAdsr);
            }
        }

        // ── Reset ALL specialized components before recreating ────────────────
        fiveMacroDisplay.reset();
        drumGrid.reset();
        // drumDetailPanel and obrixPanel ownership is transferred to the viewport
        // via release() when created, so these resets are no-ops after that point.
        // They guard against the case where a previous load was interrupted before
        // the viewport assignment (e.g., if the engine failed to load mid-way).
        drumDetailPanel.reset();
        trianglePad.reset();
        conductorArc.reset();
        modeSelector.reset();
        axisBar.reset();

        // ── OVERBITE gets FiveMacroDisplay instead of standard MacroHeroStrip ─
        if (engineId.equalsIgnoreCase("Overbite"))
        {
            fiveMacroDisplay = std::make_unique<FiveMacroDisplay>(processor.getAPVTS(), learnManager);
            addAndMakeVisible(*fiveMacroDisplay);
            macroHero.setVisible(false);
        }
        else
        {
            // loadEngine shows the strip if macros found, hides if not
            macroHero.loadEngine(engineId, prefix, accentColour);
        }

        // ── Percussion engines ────────────────────────────────────────────────
        // ONSET: uses DrumDetailPanel — a full drum-machine hierarchical layout
        //        inside the viewport (VOICES / GLOBAL / MACROS / XVC / FX sections).
        //        No separate DrumPadGrid needed above the viewport for ONSET.
        // OFFERING: keeps the classic DrumPadGrid above the generic ParameterGrid.
        if (engineId.equalsIgnoreCase("Offering"))
        {
            int numVoices = 8;
            drumGrid = std::make_unique<DrumPadGrid>(processor, prefix, accentColour, numVoices);
            addAndMakeVisible(*drumGrid);
        }

        // ── TriangleXYPad — OVERWORLD (ERA triangle) and OXYTOCIN (love triangle)
        if (engineId.equalsIgnoreCase("Overworld"))
        {
            // Verify both params exist before creating (graceful degradation).
            auto* px = processor.getAPVTS().getParameter("ow_era");
            auto* py = processor.getAPVTS().getParameter("ow_eraY");
            if (px && py)
            {
                trianglePad =
                    std::make_unique<TriangleXYPad>(processor.getAPVTS(),
                                                    "ow_era",  // X param — ERA horizontal blend
                                                    "ow_eraY", // Y param — ERA vertical blend
                                                    std::array<juce::String, 3>{{"ANALOG", "DIGITAL", "HYBRID"}},
                                                    std::array<juce::Colour, 3>{{
                                                        juce::Colour(0xFF39FF14), // Neon Green (OVERWORLD accent)
                                                        juce::Colour(0xFFBF40FF), // Prism Violet
                                                        juce::Colour(GalleryColors::xoGold) // XO Gold
                                                    }});
                addAndMakeVisible(*trianglePad);
            }
        }
        else if (engineId.equalsIgnoreCase("Oxytocin"))
        {
            // OXYTOCIN's love triangle uses oxy_intimacy (X) and oxy_passion (Y).
            // These are the two independent love-triangle axes present in OxytocinAdapter.
            auto* px = processor.getAPVTS().getParameter("oxy_intimacy");
            auto* py = processor.getAPVTS().getParameter("oxy_passion");
            if (px && py)
            {
                trianglePad =
                    std::make_unique<TriangleXYPad>(processor.getAPVTS(),
                                                    "oxy_intimacy", // X param — Intimacy axis
                                                    "oxy_passion",  // Y param — Passion axis
                                                    std::array<juce::String, 3>{{"RE-201", "MS-20", "MOOG"}},
                                                    std::array<juce::Colour, 3>{{
                                                        juce::Colour(0xFF9B5DE5), // Synapse Violet (OXYTOCIN accent)
                                                        juce::Colour(0xFFFF6B6B), // Warm Red
                                                        juce::Colour(GalleryColors::xoGold) // XO Gold
                                                    }});
                addAndMakeVisible(*trianglePad);
            }
        }

        // ── ConductorArcDisplay — OPERA autonomous dramatic arc visualizer ────
        if (engineId.equalsIgnoreCase("Opera"))
        {
            conductorArc = std::make_unique<ConductorArcDisplay>(processor.getAPVTS());
            addAndMakeVisible(*conductorArc);
        }

        // ── NamedModeSelector — engines with meaningful choice parameter names ─
        // Note: ow_era is a float param (not a choice), so OVERWORLD's ERA
        // blending is handled entirely by the TriangleXYPad above.
        // NamedModeSelector requires an AudioParameterChoice to bind via
        // ComboBoxAttachment — only wire it to genuine choice params.
        if (engineId.equalsIgnoreCase("Ocelot"))
        {
            if (processor.getAPVTS().getParameter("ocelot_biome"))
            {
                modeSelector = std::make_unique<NamedModeSelector>(
                    processor.getAPVTS(), "ocelot_biome", juce::StringArray{"RAINFOREST", "SAVANNA", "TUNDRA", "REEF"},
                    static_cast<const std::vector<juce::Colour>*>(nullptr),
                    juce::Colour(0xFFC5832B)); // Ocelot Tawny
                addAndMakeVisible(*modeSelector);
            }
        }
        else if (engineId.equalsIgnoreCase("Oracle"))
        {
            auto* p = processor.getAPVTS().getParameter("oracle_maqam");
            if (p)
            {
                if (auto* choice = dynamic_cast<juce::AudioParameterChoice*>(p))
                {
                    modeSelector =
                        std::make_unique<NamedModeSelector>(processor.getAPVTS(), "oracle_maqam", choice->choices,
                                                            static_cast<const std::vector<juce::Colour>*>(nullptr),
                                                            juce::Colour(0xFF4B0082)); // Prophecy Indigo
                    addAndMakeVisible(*modeSelector);
                }
            }
        }
        else if (engineId.equalsIgnoreCase("Orbweave"))
        {
            if (processor.getAPVTS().getParameter("weave_knotType"))
            {
                modeSelector =
                    std::make_unique<NamedModeSelector>(processor.getAPVTS(), "weave_knotType",
                                                        juce::StringArray{"TREFOIL", "FIGURE-8", "TORUS", "SOLOMON"},
                                                        static_cast<const std::vector<juce::Colour>*>(nullptr),
                                                        juce::Colour(0xFF8E4585)); // Kelp Knot Purple (ORBWEAVE accent)
                addAndMakeVisible(*modeSelector);
            }
        }

        // ── BipolarAxisBar — engines with bipolar interaction axes ────────────
        if (engineId.equalsIgnoreCase("Ouie"))
        {
            if (processor.getAPVTS().getParameter("ouie_macroHammer"))
            {
                axisBar = std::make_unique<BipolarAxisBar>(processor.getAPVTS(), "ouie_macroHammer", "STRIFE", "LOVE",
                                                           juce::Colour(0xFFFF2D2D),  // Ouroboros Red (strife)
                                                           juce::Colour(0xFFFF69B4)); // Pink (love)
                addAndMakeVisible(*axisBar);
            }
        }
        else if (engineId.equalsIgnoreCase("Obese"))
        {
            // OBESE mojo axis: fat_mojo is the real parameter ID (not fat_satMojo).
            if (processor.getAPVTS().getParameter("fat_mojo"))
            {
                axisBar = std::make_unique<BipolarAxisBar>(processor.getAPVTS(), "fat_mojo", "ANALOG", "DIGITAL",
                                                           juce::Colour(0xFFE9A84A),  // Warm amber
                                                           juce::Colour(0xFFBF40FF)); // Prism Violet
                addAndMakeVisible(*axisBar);
            }
        }
        else if (engineId.equalsIgnoreCase("Oware"))
        {
            if (processor.getAPVTS().getParameter("owr_material"))
            {
                axisBar = std::make_unique<BipolarAxisBar>(processor.getAPVTS(), "owr_material", "SOFT", "HARD",
                                                           juce::Colour(0xFF8B6914),  // Warm wood
                                                           juce::Colour(0xFFC0C0C0)); // Metallic silver
                addAndMakeVisible(*axisBar);
            }
        }

        // ── Custom panels replace the generic ParameterGrid ──────────────────
        // OBRIX → ObrixDetailPanel  |  ONSET → DrumDetailPanel
        // All other engines → generic ParameterGrid.
        //
        // Pattern: clear viewport first, then reset the unique_ptr, then create
        // a new panel and transfer ownership to the viewport via release().
        // This avoids dangling raw pointers when the viewport owns the component.
        if (engineId.equalsIgnoreCase("Onset"))
        {
            viewport.setViewedComponent(nullptr, false);
            drumDetailPanel.reset();
            obrixPanel.reset(); // ensure OBRIX panel is cleared if switching from it
            drumDetailPanel = std::make_unique<DrumDetailPanel>(processor, learnManager);
            viewport.setViewedComponent(drumDetailPanel.release(), true);
        }
        else if (engineId.equalsIgnoreCase("Obrix"))
        {
            // Clear the viewport BEFORE resetting obrixPanel to avoid a dangling
            // pointer: if a previous OBRIX load transferred ownership to the
            // viewport via release(), the viewport holds the raw ptr. Clearing
            // first lets the viewport delete that component safely before we
            // create a new one.
            viewport.setViewedComponent(nullptr, false);
            obrixPanel.reset(); // safe: viewport no longer references it
            drumDetailPanel.reset();
            obrixPanel = std::make_unique<ObrixDetailPanel>(processor, learnManager);
            viewport.setViewedComponent(obrixPanel.release(), true);
        }
        else
        {
            viewport.setViewedComponent(nullptr, false);
            drumDetailPanel.reset();
            obrixPanel.reset();
            auto* newGrid = new ParameterGrid(processor, engineId, prefix, accentColour, learnManager);
            newGrid->setFlatMode(true); // submarine: continuous grid, no section headers
            newGrid->setParentViewport(&viewport);
            viewport.setViewedComponent(newGrid, true);
        }

        // W11: Only reset scroll to top when the engine actually changed.
        if (engineChanged)
            viewport.setViewPosition(0, 0);

        // Reload mod matrix for the new engine (clear first to drop old attachments).
        modMatrix_.clear();
        modMatrix_.loadEngine(prefix);

        resized();
        repaint();
        return true;
    }

    // S5: Poll ADSR parameter values and push them to adsrDisplay.
    // Runs at 30Hz (matches WaveformDisplay). Uses the 4 raw parameter pointers
    // cached in loadSlot() — safe to read atomically on the message thread.
    void timerCallback() override
    {
        if (adsrAttack != nullptr)
        {
            const float a = adsrAttack->load();
            const float d = adsrDecay  != nullptr ? adsrDecay->load()   : 0.3f;
            const float s = adsrSustain != nullptr ? adsrSustain->load() : 0.7f;
            const float r = adsrRelease != nullptr ? adsrRelease->load() : 0.4f;
            adsrDisplay.setValues(a, d, s, r);
        }

        // #903: Refresh modulation arcs on parameter knobs based on active
        // coupling routes whose destination is this engine's slot.
        refreshModulationArcs();
    }

    void paint(juce::Graphics& g) override
    {
        using namespace GalleryColors;

        // Submarine secondary text — matches rgba(200,204,216,0.6) from prototype
        static const juce::Colour kSubSecondary = juce::Colour(200, 204, 216).withAlpha(0.6f);

        // ── Opaque dark panel background (submarine) ─────────────────────────
        g.fillAll(juce::Colour(20, 23, 32));

        // Subtle teal border
        g.setColour(juce::Colour(60, 180, 170).withAlpha(0.14f));
        g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), 8.0f, 1.0f);

        // ── Submarine header: breadcrumb + back button + creature icon + name ──
        {
            const int pad = 12;

            // Row 1: Breadcrumb "Ocean › {Engine}"
            // WCAG AAA fix: raised from 0.35f → 0.65f (≥7:1 at 10pt on #141720 bg).
            g.setFont(GalleryFonts::value(10.0f));
            g.setColour(juce::Colour(200, 204, 216).withAlpha(0.65f));
            g.drawText("Ocean  >  " + cachedEngineName,
                       pad, 4, getWidth() - pad * 2, 14,
                       juce::Justification::centredLeft, false);

            // Row 2: Back button area (32x32) + creature icon (36x36) + engine name + "ENGINE"
            const int row2Y = 20;

            // Back arrow circle — painted, click handled by parent overlay
            {
                auto backRect = juce::Rectangle<int>(pad, row2Y, 32, 32).toFloat();
                g.setColour(juce::Colour(200, 204, 216).withAlpha(0.06f));
                g.fillRoundedRectangle(backRect, 6.0f);
                g.setColour(juce::Colour(200, 204, 216).withAlpha(0.12f));
                g.drawRoundedRectangle(backRect, 6.0f, 1.0f);
                // Arrow glyph
                g.setFont(GalleryFonts::display(16.0f));
                g.setColour(juce::Colour(200, 204, 216).withAlpha(0.7f));
                g.drawText(juce::String(juce::CharPointer_UTF8("\xe2\x86\x90")), // ←
                           backRect.toNearestInt(), juce::Justification::centred);
            }

            // Creature icon placeholder (rounded square, accent-tinted)
            {
                auto iconRect = juce::Rectangle<int>(pad + 40, row2Y - 2, 36, 36).toFloat();
                g.setColour(accentColour.withAlpha(0.12f));
                g.fillRoundedRectangle(iconRect, 8.0f);
                g.setColour(accentColour.withAlpha(0.25f));
                g.drawRoundedRectangle(iconRect, 8.0f, 1.0f);
                // Creature emoji placeholder — engine initial
                g.setFont(GalleryFonts::display(16.0f));
                g.setColour(accentColour.withAlpha(0.6f));
                g.drawText(cachedEngineName.substring(0, 1),
                           iconRect.toNearestInt(), juce::Justification::centred);
            }

            // Engine name — bold, primary text
            const int textX = pad + 84;
            g.setFont(GalleryFonts::engineName(16.0f));
            g.setColour(juce::Colour(200, 204, 216).withAlpha(0.9f));
            auto displayName = GalleryUtils::ellipsizeText(
                g.getCurrentFont(), cachedEngineName, (float)(getWidth() - textX - pad));
            g.drawText(displayName, textX, row2Y - 2, getWidth() - textX - pad, 20,
                       juce::Justification::centredLeft, false);

            // "ENGINE" subtitle — WCAG AAA fix: was 0.4f (fails 7:1); use ensureMinContrast to
            // guarantee ≥7:1 against the dark panel bg while preserving the accent hue.
            g.setFont(GalleryFonts::value(10.0f));
            g.setColour(GalleryColors::ensureMinContrast(accentColour, 7.0f));
            g.drawText("ENGINE", textX, row2Y + 18, 100, 14,
                       juce::Justification::centredLeft, false);

            // Thin border under header
            g.setColour(juce::Colour(200, 204, 216).withAlpha(0.06f));
            g.drawHorizontalLine(kHeaderH - 1, (float)pad, (float)(getWidth() - pad));
        }

        // ── "OSCILLOSCOPE" label above waveform display ───────────────────────
        if (!oscLabelBounds.isEmpty())
        {
            g.setFont(GalleryFonts::value(10.0f)); // (#885: 9pt→10pt legibility floor)
            g.setColour(kSubSecondary);
            g.drawText("OSCILLOSCOPE", oscLabelBounds.withTrimmedLeft(8), juce::Justification::centredLeft, false);
        }

        // ── "ENVELOPE" label above ADSR display ───────────────────────────────
        if (!envLabelBounds.isEmpty())
        {
            g.setFont(GalleryFonts::value(10.0f)); // (#885: 9pt→10pt legibility floor)
            g.setColour(kSubSecondary);
            g.drawText("ENVELOPE", envLabelBounds.withTrimmedLeft(8), juce::Justification::centredLeft, false);
        }

        // ── Collapsed MOD tab on right edge ──────────────────────────────────
        if (!modTabBounds_.isEmpty())
        {
            // Tab background
            g.setColour(juce::Colour(60, 180, 170).withAlpha(0.08f));
            g.fillRoundedRectangle(modTabBounds_.toFloat(), 4.0f);
            g.setColour(juce::Colour(60, 180, 170).withAlpha(0.15f));
            g.drawRoundedRectangle(modTabBounds_.toFloat().reduced(0.5f), 4.0f, 1.0f);

            // "MOD" text drawn vertically
            // WCAG AAA fix: raised from 0.4f → 0.65f (≥7:1 on dark panel bg).
            g.setFont(GalleryFonts::value(10.0f));
            g.setColour(juce::Colour(200, 204, 216).withAlpha(0.65f));
            // Draw each letter stacked vertically
            int textX = modTabBounds_.getX();
            int textW = modTabBounds_.getWidth();
            int centerY = modTabBounds_.getCentreY() - 18;
            g.drawText("M", textX, centerY,      textW, 14, juce::Justification::centred);
            g.drawText("O", textX, centerY + 12,  textW, 14, juce::Justification::centred);
            g.drawText("D", textX, centerY + 24,  textW, 14, juce::Justification::centred);
        }
    }

    void resized() override
    {
        auto area = getLocalBounds();

        // ── Header: 40px with engine name ─────────────────────
        auto headerArea = area.removeFromTop(kHeaderH);
        (void)headerArea; // header is painted in paint(), no child components needed

        // P29: rebuild header gradient when width changes.
        // #895: guard against zero-size bounds — skip if not yet laid out.
        if (getWidth() > 0 && getHeight() > 0)
            cachedHeaderGrad =
                juce::ColourGradient(accentColour.withAlpha(0.14f), 0.0f, 0.0f,
                                     juce::Colours::transparentBlack, (float)getWidth(), 0.0f, false);

        // ── Body: horizontal 4-zone layout (submarine prototype) ──
        // Prototype is COMPACT — fixed heights, not dynamic fill.
        auto body = area.reduced(8, 4);

        // ZONE 1 (LEFT): Viz column — 40% normally, 20% when mod matrix is open
        {
            const int vizPct = modMatrix_.isExpanded() ? 20 : 40;
            const int vizW = juce::jmax(120, body.getWidth() * vizPct / 100);
            auto vizCol = body.removeFromLeft(vizW);

            const bool hasAdsr = adsrSliders[0].isVisible();
            const bool hasSpecialViz = drumGrid || trianglePad || conductorArc
                                    || modeSelector || axisBar;

            // Always show oscilloscope
            oscLabelBounds = vizCol.removeFromTop(12);

            if (hasAdsr)
            {
                // Full layout: oscilloscope + envelope + ADSR sliders
                waveformDisplay.setBounds(vizCol.removeFromTop(54).reduced(2, 1));
                vizCol.removeFromTop(3);

                envLabelBounds = vizCol.removeFromTop(12);
                adsrDisplay.setBounds(vizCol.removeFromTop(70).reduced(2, 1));
                adsrDisplay.setVisible(true);
                vizCol.removeFromTop(3);

                auto adsrRow = vizCol.removeFromTop(36);
                const int sliderW = adsrRow.getWidth() / 4;
                for (int i = 0; i < 4; ++i)
                {
                    auto col = adsrRow.removeFromLeft(sliderW).reduced(3, 0);
                    adsrSliderLabels[i].setBounds(col.removeFromTop(12));
                    col.removeFromTop(2);
                    adsrSliders[i].setBounds(col);
                }
            }
            else if (hasSpecialViz)
            {
                // Engine-specific viz: oscilloscope + specialized display
                waveformDisplay.setBounds(vizCol.removeFromTop(54).reduced(2, 1));
                vizCol.removeFromTop(3);
                envLabelBounds = {};
                adsrDisplay.setVisible(false);

                if (drumGrid)
                {
                    int drumH = drumGrid->getRequiredHeight(vizCol.getWidth());
                    drumGrid->setBounds(vizCol.removeFromTop(juce::jmin(drumH, vizCol.getHeight())));
                }
                if (trianglePad)
                    trianglePad->setBounds(vizCol.removeFromTop(juce::jmin(120, vizCol.getHeight()))
                        .withSizeKeepingCentre(juce::jmin(140, vizCol.getWidth()), juce::jmin(116, vizCol.getHeight())));
                if (conductorArc)
                    conductorArc->setBounds(vizCol.removeFromTop(50).withSizeKeepingCentre(
                        juce::jmin(180, vizCol.getWidth()), 46));
                if (modeSelector)
                    modeSelector->setBounds(vizCol.removeFromTop(30).reduced(2, 1));
                if (axisBar)
                    axisBar->setBounds(vizCol.removeFromTop(28).reduced(2, 1));
            }
            else
            {
                // Fallback: oscilloscope fills the viz column
                envLabelBounds = {};
                adsrDisplay.setVisible(false);
                waveformDisplay.setBounds(vizCol.reduced(2, 1));
            }
        }

        body.removeFromLeft(8);

        // ZONE 2: M1-M4 Macro pillar sliders — compact, aligned to viz content
        {
            bool showMacros = macroHero.isVisible() || (fiveMacroDisplay != nullptr);
            if (showMacros)
            {
                auto macroCol = body.removeFromLeft(kMacroColW);
                if (fiveMacroDisplay)
                    fiveMacroDisplay->setBounds(macroCol);
                else
                    macroHero.setBounds(macroCol);
                body.removeFromLeft(8);
            }
            else
            {
                macroHero.setBounds(0, 0, 0, 0);
                if (fiveMacroDisplay)
                    fiveMacroDisplay->setBounds(0, 0, 0, 0);
            }
        }

        // ZONE 4: MOD tab / ModMatrixDrawer
        if (modMatrix_.isExpanded())
        {
            auto drawerArea = body.removeFromRight(ModMatrixDrawer::kExpandedWidth);
            modMatrix_.setBounds(drawerArea);
            modMatrix_.setVisible(true);
            modTabBounds_ = {}; // tab is replaced by the drawer header
        }
        else
        {
            modMatrix_.setVisible(false);
            modMatrix_.setBounds(0, 0, 0, 0);
            modTabBounds_ = body.removeFromRight(24);
        }

        // ZONE 3 (CENTER): Parameter grid viewport — fills remaining space
        viewport.setBounds(body);

        // Resize viewport content: support ParameterGrid, ObrixDetailPanel, DrumDetailPanel
        {
            int gridW = juce::jmax(200, viewport.getWidth() - viewport.getScrollBarThickness() - 4);
            auto* viewed = viewport.getViewedComponent();
            if (auto* grid = dynamic_cast<ParameterGrid*>(viewed))
            {
                grid->setParentViewport(&viewport);
                grid->setSize(gridW, grid->getRequiredHeight(gridW));
            }
            else if (auto* obrix = dynamic_cast<ObrixDetailPanel*>(viewed))
            {
                obrix->setSize(gridW, obrix->getRequiredHeight(gridW));
            }
            else if (auto* drum = dynamic_cast<DrumDetailPanel*>(viewed))
            {
                drum->setSize(gridW, drum->getRequiredHeight(gridW));
            }
        }
    }

private:
    static constexpr int kHeaderH = 64; // prototype: breadcrumb(14) + icon row(40) + pad(10)
    static constexpr int kMacroColW = 130; // width of Zone 2 macro pillar column

    // #903: Map a CouplingType to parameter keyword patterns so the modulation
    // arc refresh can find the right knobs in the ParameterGrid without knowing
    // engine-specific parameter names.  Returns a comma-separated keyword list;
    // an empty string means "no specific param target — skip".
    static juce::String keywordsForCouplingType(CouplingType type)
    {
        switch (type)
        {
        case CouplingType::AmpToFilter:
        case CouplingType::FilterToFilter:
            return "filter,cutoff,flt,resonance,freq";
        case CouplingType::AmpToPitch:
        case CouplingType::LFOToPitch:
        case CouplingType::PitchToPitch:
            return "pitch,tune,detune,semi,coarse,fine,glide";
        case CouplingType::EnvToMorph:
        case CouplingType::RhythmToBlend:
            return "morph,blend,mix,warp,shape";
        case CouplingType::AudioToFM:
            return "fm,fmratio,modindex,fmamt";
        case CouplingType::AudioToRing:
            return "ring,ringmod";
        case CouplingType::EnvToDecay:
            return "decay,release,sustain,env,adsr";
        case CouplingType::AudioToWavetable:
            return "wave,wt,table,wtpos,position,scan";
        case CouplingType::KnotTopology:
        case CouplingType::TriangularCoupling:
            return "macro,character,movement,coupling,space";
        default:
            return {}; // AmpToChoke, AudioToBuffer — no single param target
        }
    }

    // Poll active coupling routes for this slot and update modulation arcs
    // on all live ParameterGrid knobs that match the route's target type.
    void refreshModulationArcs()
    {
        if (activeSlot_ < 0)
            return;

        auto* viewed = viewport.getViewedComponent();
        auto* grid = dynamic_cast<ParameterGrid*>(viewed);
        if (!grid)
            return;

        // Collect the set of incoming coupling routes for activeSlot_ from
        // the coupling matrix (message-thread safe — getRoutes() reads the
        // atomic-swap double-buffer and returns a snapshot copy).
        auto routes = processor.getCouplingMatrix().getRoutes();

        // For each CouplingType, accumulate the strongest incoming amount.
        // Multiple routes of the same type (from different sources) are combined
        // by taking the max absolute value so the arc reflects the dominant source.
        juce::HashMap<int, float> typeToAmount;    // CouplingType → max |amount|
        juce::HashMap<int, juce::Colour> typeToColour;

        for (const auto& route : routes)
        {
            if (!route.active || route.destSlot != activeSlot_)
                continue;
            int typeKey = static_cast<int>(route.type);
            float existing = typeToAmount.contains(typeKey) ? typeToAmount[typeKey] : 0.0f;
            if (std::abs(route.amount) > std::abs(existing))
            {
                typeToAmount.set(typeKey, route.amount);
                typeToColour.set(typeKey, CouplingTypeColors::forType(route.type));
            }
        }

        if (typeToAmount.size() == 0)
        {
            // No active routes → clear all arcs
            grid->clearAllModulationArcs();
            return;
        }

        // Apply the strongest route of each type to matching knobs.
        // If a knob matches multiple types, the last one wins (acceptable — rare).
        // First clear so removed routes don't leave stale arcs.
        grid->clearAllModulationArcs();
        for (juce::HashMap<int, float>::Iterator it(typeToAmount); it.next();)
        {
            auto ctype = static_cast<CouplingType>(it.getKey());
            auto kws   = keywordsForCouplingType(ctype);
            if (kws.isEmpty())
                continue;
            grid->setModulationForKeywords(kws,
                                            it.getValue(),
                                            typeToColour[it.getKey()].withAlpha(0.40f));
        }
    }

    XOceanusProcessor& processor;
    MacroHeroStrip macroHero;
    WaveformDisplay waveformDisplay;
    ADSRDisplay adsrDisplay;
    // ADSR horizontal sliders — interactive controls below the envelope curve
    std::array<juce::Slider, 4> adsrSliders;
    std::array<juce::Label, 4> adsrSliderLabels;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>, 4> adsrAttachments;
    std::unique_ptr<ObrixDetailPanel> obrixPanel;       // only created when OBRIX is loaded
    std::unique_ptr<DrumDetailPanel> drumDetailPanel;  // only created when ONSET is loaded
    std::unique_ptr<DrumPadGrid> drumGrid;              // created for OFFERING (ONSET uses DrumDetailPanel)
    std::unique_ptr<FiveMacroDisplay> fiveMacroDisplay; // created for OVERBITE
    std::unique_ptr<TriangleXYPad> trianglePad;         // created for OVERWORLD, OXYTOCIN
    std::unique_ptr<ConductorArcDisplay> conductorArc;  // created for OPERA
    std::unique_ptr<NamedModeSelector> modeSelector;    // created for OVERWORLD, OCELOT, ORACLE, ORBWEAVE
    std::unique_ptr<BipolarAxisBar> axisBar;            // created for OUIE, OBESE, OWARE
    juce::Viewport viewport;
    juce::String engineId{"—"};
    juce::Colour accentColour{GalleryColors::get(GalleryColors::borderGray())};
    MIDILearnManager* learnManager = nullptr;
    // P29: cached header gradient — rebuilt in loadSlot() and resized().
    juce::ColourGradient cachedHeaderGrad;
    // P30: cached uppercase engine name — set in loadSlot(), used in paint().
    juce::String cachedEngineName{"—"};
    // Bounds for the "OSCILLOSCOPE" label painted above the waveform display.
    juce::Rectangle<int> oscLabelBounds;
    // Bounds for the "ENVELOPE" label painted above the ADSR display.
    juce::Rectangle<int> envLabelBounds;

    // #903: Active slot index — set in loadSlot(), used by refreshModulationArcs().
    int activeSlot_ = -1;

    // S5: Raw pointers to the active engine's ADSR parameters.
    // Cached in loadSlot() and read by timerCallback() at 30Hz.
    // Null when the current engine has no standard ADSR params.
    std::atomic<float>* adsrAttack  = nullptr;
    std::atomic<float>* adsrDecay   = nullptr;
    std::atomic<float>* adsrSustain = nullptr;
    std::atomic<float>* adsrRelease = nullptr;

    // Bounds for the collapsed MOD tab painted on the right edge.
    juce::Rectangle<int> modTabBounds_;
    ModMatrixDrawer modMatrix_;        // collapsible mod matrix panel (Zone 4)
    SubmarineSliderLnF adsrSliderLnF; // thick-track rendering for ADSR sliders

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EngineDetailPanel)
};

} // namespace xoceanus
