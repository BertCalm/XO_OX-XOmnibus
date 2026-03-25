#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "../XOlokunProcessor.h"
#include "../Core/EngineRegistry.h"
#include "EngineVocabulary.h"
// GalleryColors.h provides GalleryColors, GalleryFonts, A11y — must come before
// CouplingVisualizer.h and ExportDialog.h which depend on those namespaces.
#include "GalleryColors.h"
#include "CouplingVisualizer/CouplingVisualizer.h"
#include "ExportDialog/ExportDialog.h"
// PlaySurface.h has a minimal GalleryColors::get() stub guarded by this macro.
// Define before inclusion so the stub is skipped (full def is already above).
#define XOLOKUN_GALLERY_COLORS_DEFINED 1
#include "PlaySurface/PlaySurface.h"
// PresetBrowser.h opens its own namespace xolokun { } block — must be included
// at file scope (before our namespace xolokun { below) to avoid nesting.
#include "PresetBrowser/PresetBrowser.h"

// ── Extracted Gallery components ──────────────────────────────────────
#include "Gallery/MidiLearnMouseListener.h"
#include "Gallery/GalleryLookAndFeel.h"
#include "Gallery/GalleryKnob.h"
#include "Gallery/ParameterGrid.h"
#include "Gallery/MacroHeroStrip.h"
#include "Gallery/EngineDetailPanel.h"
#include "Gallery/FieldMapPanel.h"
#include "Gallery/OverviewPanel.h"
#include "Gallery/CompactEngineTile.h"
#include "Gallery/MacroSection.h"
#include "Gallery/AdvancedFXPanel.h"
#include "Gallery/MasterFXSection.h"
#include "Gallery/PresetBrowserPanel.h"
#include "Gallery/PresetBrowserStrip.h"
#include "Gallery/ChordMachinePanel.h"
#include "Gallery/PerformanceViewPanel.h"
#include "Gallery/CouplingArcOverlay.h"
#include "Gallery/ColumnLayoutManager.h"
#include "Gallery/SidebarPanel.h"
#include "Gallery/StatusBar.h"

namespace xolokun {

// GalleryColors, GalleryFonts, and A11y are provided by GalleryColors.h
// (included above).  Their definitions are canonical in that file; they are
// available here via the xolokun:: namespace that GalleryColors.h opens.

//==============================================================================
// XOlokunEditor — Gallery Model plugin window.
//
// Layout:
//   ┌─────────────────────────────────────┐
//   │  Header (title + tagline)           │
//   ├────────┬────────────────────────────┤
//   │ Tile 1 │                            │
//   │ Tile 2 │  Right panel               │
//   │ Tile 3 │  (OverviewPanel or         │
//   │ Tile 4 │   EngineDetailPanel)       │
//   ├────────┴────────────────────────────┤
//   │  MacroSection                       │
//   └─────────────────────────────────────┘
//
// Transition: 150ms opacity cross-fade via juce::ComponentAnimator
// when switching between overview and engine detail, or between engines.
//
class XOlokunEditor : public juce::AudioProcessorEditor,
                       private juce::Timer
{
public:
    explicit XOlokunEditor(XOlokunProcessor& proc)
        : AudioProcessorEditor(proc),
          processor(proc),
          overview(proc),
          detail(proc),
          chordPanel(proc),
          performancePanel(proc),
          macros(proc.getAPVTS()),
          masterFXStrip(proc.getAPVTS()),
          presetBrowser(proc)
    {
        laf = std::make_unique<GalleryLookAndFeel>();
        setLookAndFeel(laf.get());

        for (int i = 0; i < XOlokunProcessor::MaxSlots; ++i)
        {
            tiles[i] = std::make_unique<CompactEngineTile>(proc, i);
            tiles[i]->onSelect = [this](int slot) { selectSlot(slot); };
            addAndMakeVisible(*tiles[i]);
        }

        addAndMakeVisible(fieldMap);
        addAndMakeVisible(overview);
        addAndMakeVisible(detail);
        addAndMakeVisible(chordPanel);
        addAndMakeVisible(performancePanel);
        addAndMakeVisible(macros);
        addAndMakeVisible(masterFXStrip);
        addAndMakeVisible(presetBrowser);

        // Coupling arc overlay: must be added AFTER tiles so it paints on top.
        // setInterceptsMouseClicks(false,false) means tiles still receive clicks.
        addAndMakeVisible(couplingArcs);

        detail.setVisible(false);
        detail.setAlpha(0.0f);
        chordPanel.setVisible(false);
        chordPanel.setAlpha(0.0f);
        performancePanel.setVisible(false);
        performancePanel.setAlpha(0.0f);

        // "CM" toggle button in header area
        addAndMakeVisible(cmToggleBtn);
        cmToggleBtn.setButtonText("CM");
        cmToggleBtn.setTooltip("Chord Machine — generative chord sequencer");
        A11y::setup (cmToggleBtn, "Chord Machine Toggle", "Toggle the chord machine sequencer panel");
        cmToggleBtn.setClickingTogglesState(true);
        cmToggleBtn.onClick = [this]
        {
            if (cmToggleBtn.getToggleState())
            {
                perfToggleBtn.setToggleState(false, juce::dontSendNotification);
                showChordMachine();
            }
            else
                showOverview();
        };

        // "P" toggle button in header area — Performance View
        addAndMakeVisible(perfToggleBtn);
        perfToggleBtn.setButtonText("P");
        perfToggleBtn.setTooltip("Performance View — real-time coupling control");
        A11y::setup (perfToggleBtn, "Performance View Toggle", "Toggle the coupling performance panel");
        perfToggleBtn.setClickingTogglesState(true);
        perfToggleBtn.onClick = [this]
        {
            if (perfToggleBtn.getToggleState())
            {
                cmToggleBtn.setToggleState(false, juce::dontSendNotification);
                showPerformanceView();
            }
            else
                showOverview();
        };

        // "PS" toggle button — PlaySurface (4-zone performance interface)
        addAndMakeVisible(surfaceToggleBtn);
        surfaceToggleBtn.setButtonText("PS");
        surfaceToggleBtn.setTooltip("PlaySurface — 4-zone performance interface (pads, orbit, strip, triggers)");
        A11y::setup (surfaceToggleBtn, "PlaySurface Toggle",
                     "Toggle the PlaySurface performance panel at the bottom of the editor");
        surfaceToggleBtn.setClickingTogglesState(true);
        surfaceToggleBtn.onClick = [this]
        {
            if (surfaceToggleBtn.getToggleState())
                showPlaySurface();
            else
                hidePlaySurface();
        };

        // PlaySurface — hidden by default; revealed via surfaceToggleBtn
        addAndMakeVisible(playSurface);
        playSurface.setVisible(false);
        playSurface.setAlpha(0.0f);

        // Wire MIDI: PlaySurface note events flow through the processor's
        // MidiMessageCollector, which is drained into processBlock each audio callback.
        playSurface.setMidiCollector(&proc.getMidiCollector(), 1);

        // Dark mode toggle — light is default (brand rule)
        addAndMakeVisible(themeToggleBtn);
        themeToggleBtn.setButtonText("D");
        themeToggleBtn.setTooltip("Toggle dark mode");
        A11y::setup (themeToggleBtn, "Dark Mode Toggle", "Switch between light and dark theme");
        themeToggleBtn.setClickingTogglesState(true);
        themeToggleBtn.onClick = [this]
        {
            GalleryColors::darkMode() = themeToggleBtn.getToggleState();
            laf->applyTheme();
            repaint();
            for (int i = 0; i < XOlokunProcessor::MaxSlots; ++i)
                tiles[i]->repaint();
            overview.repaint();
            detail.repaint();
            chordPanel.repaint();
            performancePanel.repaint();
            macros.repaint();
            masterFXStrip.repaint();
            presetBrowser.repaint();
            sidebar.repaint();
        };

        // Export button — launches ExportDialog as a CallOutBox
        addAndMakeVisible(exportBtn);
        exportBtn.setButtonText("XPN");
        exportBtn.setTooltip("Export presets as MPC-compatible XPN expansion pack");
        A11y::setup (exportBtn, "Export", "Open export dialog to build XPN expansion packs");
        exportBtn.onClick = [this]
        {
            juce::CallOutBox::launchAsynchronously(
                std::make_unique<ExportDialog>(
                    processor.getPresetManager(),
                    &processor.getAPVTS(),
                    &processor.getCouplingMatrix()),
                exportBtn.getScreenBounds(),
                getTopLevelComponent());
        };

        // Scan factory preset directory
        auto presetDir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                             .getChildFile("Application Support/XO_OX/XOlokun/Presets");
        if (presetDir.isDirectory())
            proc.getPresetManager().scanPresetDirectory(presetDir);
        presetBrowser.setMacroSection(&macros); // wire preset macroLabels → macro knob labels
        presetBrowser.updateDisplay();

        // ── Column C Sidebar ──────────────────────────────────────────────────
        // Wire the PresetManager so C1 (Preset tab) has a live browser.
        sidebar.setPresetManager(proc.getPresetManager());
        addAndMakeVisible(sidebar);

        // ── Status Bar ────────────────────────────────────────────────────────
        // Trigger callbacks are stubs for now; wired to processor in a later step.
        addAndMakeVisible(statusBar);
        addKeyListener(statusBar.getKeyListener());

        // ── MIDI Learn wiring ─────────────────────────────────────────────────
        // Connect the processor's MIDILearnManager to every parameter knob in the UI.
        // This enables right-click → MIDI Learn on all rotary controls.
        {
            auto& mlm = proc.getMIDILearnManager();
            detail.setMidiLearnManager(&mlm);
            macros.setupMidiLearn(mlm);
            masterFXStrip.setupMidiLearn(mlm);
        }

        // ── MIDI Learn learn-complete notification ──────────────────────────
        // When a mapping is completed (audio thread captured a CC), bump the
        // timer to 10Hz so visual feedback updates promptly.  Reverts to 1Hz
        // after the first poll cycle that finds nothing pending.
        proc.getMIDILearnManager().setLearnCompleteCallback(
            [this](const juce::String&, int) { startTimerHz(10); });

        // Event-driven tile refresh: only repaint the affected tile and overview on engine change.
        proc.onEngineChanged = [this](int slot)
        {
            if (slot >= 0 && slot < XOlokunProcessor::MaxSlots)
                tiles[slot]->refresh();
            overview.refresh();
            if (performancePanel.isVisible())
                performancePanel.refresh();
        };

        setSize(1100, 700);
        setResizable(true, true);
        setResizeLimits(960, 600, 1600, 1000);
        setWantsKeyboardFocus(true);
        setTitle ("XOlokun Synthesizer");
        setDescription ("Multi-engine synthesizer with cross-engine coupling. "
                        "Keys 1-4 select engine slots, Escape returns to overview.");
        startTimerHz(1); // Reduced from 5Hz — idle polling only as a fallback
    }

    ~XOlokunEditor() override
    {
        stopTimer();
        removeKeyListener(statusBar.getKeyListener());
        processor.onEngineChanged = nullptr; // prevent callback after editor is destroyed
        setLookAndFeel(nullptr);
    }

    bool keyPressed(const juce::KeyPress& key) override
    {
        // Keys 1-4 jump directly to engine slots
        for (int i = 0; i < XOlokunProcessor::MaxSlots; ++i)
        {
            if (key == juce::KeyPress('1' + i))
            {
                if (processor.getEngine(i) != nullptr)
                    selectSlot(i);
                return true;
            }
        }
        // Escape returns to overview
        if (key == juce::KeyPress::escapeKey)
        {
            showOverview();
            return true;
        }
        return false;
    }

    void paint(juce::Graphics& g) override
    {
        using namespace GalleryColors;
        g.fillAll(get(shellWhite()));

        const int headerH = ColumnLayoutManager::kHeaderH;

        // Header area
        auto header = getLocalBounds().removeFromTop(headerH).toFloat();
        g.setColour(get(shellWhite()));
        g.fillRect(header);

        // XO Gold stripe
        g.setColour(get(xoGold));
        g.fillRect(header.removeFromBottom(3.0f));

        g.setColour(get(textDark()));
        g.setFont(GalleryFonts::display(19.0f));
        g.drawText("XOlokun",
                   juce::Rectangle<int>(16, 0, 160, headerH - 3),
                   juce::Justification::centredLeft);

        g.setColour(get(xoGoldText())); // Darkened gold — meets WCAG AA on shellWhite
        g.setFont(GalleryFonts::heading(8.5f));
        g.drawText("XO_OX Designs",
                   juce::Rectangle<int>(16, headerH - 18, 110, 12),
                   juce::Justification::centredLeft);

        // Active coupling route count in header
        {
            auto routes = processor.getCouplingMatrix().getRoutes();
            int activeRoutes = 0;
            for (const auto& r : routes) if (r.active && r.amount >= 0.001f) ++activeRoutes;

            juce::String routeLabel = (activeRoutes > 0)
                ? juce::String(activeRoutes) + " coupling route" + (activeRoutes > 1 ? "s" : "") + " active"
                : ([this]() -> juce::String {
                      int numEngines = (int)EngineRegistry::instance().getRegisteredIds().size();
                      int numCoupling = 14;
                      int numPresets = (int)processor.getPresetManager().getLibrary().size();
                      juce::String presetStr = (numPresets > 0)
                          ? juce::String(numPresets) + "+"
                          : "18000+";
                      return juce::String(numEngines) + " Engines \xc2\xb7 "
                           + juce::String(numCoupling) + " Coupling Types \xc2\xb7 "
                           + presetStr + " Presets";
                  }());

            g.setColour(activeRoutes > 0 ? get(xoGoldText()) : get(textMid()).withAlpha(0.5f));
            g.setFont(GalleryFonts::body(9.0f));
            g.drawText(routeLabel,
                       juce::Rectangle<int>(getWidth() - 310, 0, 298, headerH - 6),
                       juce::Justification::centredRight);
        }

        // ── MIDI Learn status badge — appears in header when listening ──────
        {
            const auto& mlm = processor.getMIDILearnManager();
            if (mlm.isLearning())
            {
                // Pulse the badge alpha at ~2Hz using system time
                double t = juce::Time::getMillisecondCounterHiRes() * 0.002;
                float pulse = 0.65f + 0.35f * (float)std::sin(t * juce::MathConstants<double>::twoPi);

                juce::String badge = "MIDI LEARN: move a controller to map \xe2\x80\xa2 right-click to cancel";
                auto badgeRect = juce::Rectangle<int>(165, 4, getWidth() - 360, headerH - 10);

                g.setColour(juce::Colour(0xFFE9C46A).withAlpha(0.18f * pulse));
                g.fillRoundedRectangle(badgeRect.toFloat(), 4.0f);
                g.setColour(juce::Colour(0xFFE9C46A).withAlpha(pulse));
                g.setFont(GalleryFonts::heading(9.0f));
                g.drawText(badge, badgeRect, juce::Justification::centred);
            }
        }

        // Column A / Column B separator line
        const int sepX = layout.getColumnAWidth();
        if (sepX > 0)
        {
            g.setColour(get(borderGray()));
            g.drawVerticalLine(sepX, (float)headerH, (float)getHeight());
        }
    }

    void resized() override
    {
        // ── Sync layout state ────────────────────────────────────────────────
        layout.playSurfaceVisible = playSurface.isVisible();
        // columnCCollapsed and cinematicMode are toggled by their respective buttons
        // (not yet wired — Column C is reserved space only in this step)
        layout.compute(getWidth(), getHeight());

        // ── Header: toggle buttons and preset browser strip (right side) ─────
        auto header = layout.getHeader();
        presetBrowser.setBounds(header.removeFromRight(220).reduced(4, 10));
        exportBtn.setBounds(header.removeFromRight(46).reduced(4, 12));
        themeToggleBtn.setBounds(header.removeFromRight(32).reduced(2, 12));
        surfaceToggleBtn.setBounds(header.removeFromRight(36).reduced(2, 12));
        perfToggleBtn.setBounds(header.removeFromRight(32).reduced(2, 12));
        cmToggleBtn.setBounds(header.removeFromRight(42).reduced(4, 12));

        // ── Column A — Engine Rack + MacroSection at bottom ──────────────────
        // Grab a mutable copy so we can slice the bottom for macros.
        auto colA = layout.getColumnA();

        // MacroSection stays at bottom of Column A (moves to header in Step 11)
        auto macroArea = colA.removeFromBottom(kMacroH).reduced(6, 4);
        macros.setBounds(macroArea);

        // Remaining Column A for engine tiles
        int tileH = colA.getHeight() / XOlokunProcessor::MaxSlots;
        for (int i = 0; i < XOlokunProcessor::MaxSlots; ++i)
        {
            tiles[i]->setBounds(colA.getX(), colA.getY() + i * tileH,
                                colA.getWidth(), tileH);
        }

        // ── Column B — Panel stack + MasterFX strip + FieldMap ───────────────
        // getColumnBPanel() already excludes the FieldMap strip at the bottom.
        auto colBPanel = layout.getColumnBPanel();

        // MasterFX strip at bottom of Column B panel area
        auto masterFXBounds = colBPanel.removeFromBottom(kMasterFXH).reduced(6, 3);
        masterFXStrip.setBounds(masterFXBounds);

        // Remaining Column B panel area for the view stack (stacked, one visible at a time)
        overview.setBounds(colBPanel);
        detail.setBounds(colBPanel);
        chordPanel.setBounds(colBPanel);
        performancePanel.setBounds(colBPanel);

        // FieldMap — bottom strip of Column B (reserved by ColumnLayoutManager)
        fieldMap.setBounds(layout.getFieldMap());

        // ── Column C — Tabbed Sidebar (SidebarPanel) ─────────────────────────
        sidebar.setBounds(layout.getColumnC());

        // ── PlaySurface ───────────────────────────────────────────────────────
        // ColumnLayoutManager parks the surface below the window when invisible,
        // so we always apply the computed bounds unconditionally.
        playSurface.setBounds(layout.getPlaySurface());

        // ── Status Bar ───────────────────────────────────────────────────────
        statusBar.setBounds(layout.getStatusBar());

        // ── Coupling arc overlay — full editor bounds ─────────────────────────
        couplingArcs.setBounds(getLocalBounds());
        for (int i = 0; i < XOlokunProcessor::MaxSlots; ++i)
        {
            if (tiles[i])
                couplingArcs.setTileCenter(i, tiles[i]->getBounds().getCentre().toFloat());
        }
    }

private:
    void selectSlot(int slot)
    {
        // Deselect all tiles
        for (int i = 0; i < XOlokunProcessor::MaxSlots; ++i)
            tiles[i]->setSelected(i == slot);

        if (slot == selectedSlot && detail.isVisible())
            return; // already showing this one

        selectedSlot = slot;
        cmToggleBtn.setToggleState(false, juce::dontSendNotification);
        perfToggleBtn.setToggleState(false, juce::dontSendNotification);

        // Hide chord/performance panels if visible
        if (chordPanel.isVisible())
            chordPanel.setVisible(false);
        if (performancePanel.isVisible())
            performancePanel.setVisible(false);

        auto& anim = juce::Desktop::getInstance().getAnimator();

        // SafePointer prevents crash if editor is destroyed during animation
        juce::Component::SafePointer<XOlokunEditor> safeThis(this);

        if (detail.isVisible())
        {
            // Cross-fade: fade out → swap → fade in
            anim.fadeOut(&detail, kFadeMs);
            juce::Timer::callAfterDelay(kFadeMs, [safeThis, slot]
            {
                if (safeThis == nullptr) return;
                auto& self = *safeThis;
                if (self.detail.loadSlot(slot))
                {
                    self.overview.setVisible(false);
                    self.detail.setAlpha(0.0f);
                    self.detail.setVisible(true);
                    juce::Desktop::getInstance().getAnimator().fadeIn(&self.detail, kFadeMs);
                }
                else
                {
                    self.showOverview();
                }
            });
        }
        else
        {
            // Fade out overview, fade in detail
            anim.fadeOut(&overview, kFadeMs);
            juce::Timer::callAfterDelay(kFadeMs, [safeThis, slot]
            {
                if (safeThis == nullptr) return;
                auto& self = *safeThis;
                if (self.detail.loadSlot(slot))
                {
                    self.overview.setVisible(false);
                    self.detail.setAlpha(0.0f);
                    self.detail.setVisible(true);
                    juce::Desktop::getInstance().getAnimator().fadeIn(&self.detail, kFadeMs);
                }
                else
                {
                    self.overview.setAlpha(1.0f);
                    self.overview.setVisible(true);
                }
            });
        }
    }

    void showOverview()
    {
        selectedSlot = -1;
        for (int i = 0; i < XOlokunProcessor::MaxSlots; ++i)
            tiles[i]->setSelected(false);
        cmToggleBtn.setToggleState(false, juce::dontSendNotification);
        perfToggleBtn.setToggleState(false, juce::dontSendNotification);

        auto& anim = juce::Desktop::getInstance().getAnimator();
        juce::Component* outgoing = detail.isVisible() ? static_cast<juce::Component*>(&detail)
                                  : chordPanel.isVisible() ? static_cast<juce::Component*>(&chordPanel)
                                  : performancePanel.isVisible() ? static_cast<juce::Component*>(&performancePanel)
                                  : nullptr;
        if (outgoing)
        {
            juce::Component::SafePointer<XOlokunEditor> safeThis(this);
            juce::Component::SafePointer<juce::Component> safeOutgoing(outgoing);
            anim.fadeOut(outgoing, kFadeMs);
            juce::Timer::callAfterDelay(kFadeMs, [safeThis, safeOutgoing]
            {
                if (safeThis == nullptr) return;
                if (safeOutgoing != nullptr) safeOutgoing->setVisible(false);
                safeThis->overview.setAlpha(0.0f);
                safeThis->overview.setVisible(true);
                juce::Desktop::getInstance().getAnimator().fadeIn(&safeThis->overview, kFadeMs);
            });
        }
    }

    void showChordMachine()
    {
        selectedSlot = -1;
        for (int i = 0; i < XOlokunProcessor::MaxSlots; ++i)
            tiles[i]->setSelected(false);

        auto& anim = juce::Desktop::getInstance().getAnimator();
        juce::Component* outgoing = detail.isVisible() ? static_cast<juce::Component*>(&detail)
                                  : performancePanel.isVisible() ? static_cast<juce::Component*>(&performancePanel)
                                  : overview.isVisible() ? static_cast<juce::Component*>(&overview)
                                  : nullptr;
        if (outgoing)
        {
            juce::Component::SafePointer<XOlokunEditor> safeThis(this);
            juce::Component::SafePointer<juce::Component> safeOutgoing(outgoing);
            anim.fadeOut(outgoing, kFadeMs);
            juce::Timer::callAfterDelay(kFadeMs, [safeThis, safeOutgoing]
            {
                if (safeThis == nullptr) return;
                if (safeOutgoing != nullptr) safeOutgoing->setVisible(false);
                safeThis->chordPanel.setAlpha(0.0f);
                safeThis->chordPanel.setVisible(true);
                juce::Desktop::getInstance().getAnimator().fadeIn(&safeThis->chordPanel, kFadeMs);
            });
        }
        else
        {
            chordPanel.setAlpha(0.0f);
            chordPanel.setVisible(true);
            anim.fadeIn(&chordPanel, kFadeMs);
        }
    }

    void showPerformanceView()
    {
        selectedSlot = -1;
        for (int i = 0; i < XOlokunProcessor::MaxSlots; ++i)
            tiles[i]->setSelected(false);

        performancePanel.refresh();

        auto& anim = juce::Desktop::getInstance().getAnimator();
        juce::Component* outgoing = detail.isVisible() ? static_cast<juce::Component*>(&detail)
                                  : chordPanel.isVisible() ? static_cast<juce::Component*>(&chordPanel)
                                  : overview.isVisible() ? static_cast<juce::Component*>(&overview)
                                  : nullptr;
        if (outgoing)
        {
            juce::Component::SafePointer<XOlokunEditor> safeThis(this);
            juce::Component::SafePointer<juce::Component> safeOutgoing(outgoing);
            anim.fadeOut(outgoing, kFadeMs);
            juce::Timer::callAfterDelay(kFadeMs, [safeThis, safeOutgoing]
            {
                if (safeThis == nullptr) return;
                if (safeOutgoing != nullptr) safeOutgoing->setVisible(false);
                safeThis->performancePanel.setAlpha(0.0f);
                safeThis->performancePanel.setVisible(true);
                juce::Desktop::getInstance().getAnimator().fadeIn(&safeThis->performancePanel, kFadeMs);
            });
        }
        else
        {
            performancePanel.setAlpha(0.0f);
            performancePanel.setVisible(true);
            anim.fadeIn(&performancePanel, kFadeMs);
        }
    }

    // ── PlaySurface show/hide ──────────────────────────────────────────────────
    // The PlaySurface overlays the bottom ~40% of the editor.  Showing it
    // triggers a resized() so the layout pushes up the macros/masterFX strips;
    // hiding it restores the original layout.  Both use the standard 150ms fade.

    void showPlaySurface()
    {
        auto& anim = juce::Desktop::getInstance().getAnimator();
        playSurface.setAlpha(0.0f);
        playSurface.setVisible(true);
        resized(); // recalculate layout so PlaySurface gets its bounds before fading in
        anim.fadeIn(&playSurface, kFadeMs);
    }

    void hidePlaySurface()
    {
        auto& anim = juce::Desktop::getInstance().getAnimator();
        juce::Component::SafePointer<XOlokunEditor> safeThis(this);
        anim.fadeOut(&playSurface, kFadeMs);
        juce::Timer::callAfterDelay(kFadeMs, [safeThis]
        {
            if (safeThis == nullptr) return;
            safeThis->playSurface.setVisible(false);
            safeThis->resized(); // restore layout once panel is hidden
        });
    }

    void timerCallback() override
    {
        for (int i = 0; i < XOlokunProcessor::MaxSlots; ++i)
            tiles[i]->refresh();
        if (!detail.isVisible())
            overview.refresh();
        if (performancePanel.isVisible())
            performancePanel.refresh();

        // ── MIDI Learn: finalise pending learn captures ───────────────────────
        // checkPendingLearn() is safe to call from the message thread at any rate.
        // It reads a single atomic written by processMidi() on the audio thread
        // and, if a CC was captured, creates the mapping and fires the callback.
        {
            auto& mlm = processor.getMIDILearnManager();
            mlm.checkPendingLearn();

            // While a learn is active, keep the timer running fast (10Hz) so the
            // amber-pulse animation refreshes.  When idle, revert to 1Hz to avoid
            // wasting cycles.
            if (mlm.isLearning())
            {
                startTimerHz(10);
                repaint(); // pulse the header badge + listening knob ring
            }
            else
            {
                startTimerHz(1);
            }
        }

        // Drain Field Map note events from the lock-free audio-thread queue.
        // Color is resolved here on the message thread (safe: getEngine / getAccentColour).
        // XO Gold is used as fallback when no engine occupies the slot.
        static const juce::Colour kXOGold = juce::Colour(0xFFE9C46A);
        processor.drainNoteEvents([&](const XOlokunProcessor::NoteMapEvent& ev)
        {
            juce::Colour colour = kXOGold;
            if (ev.slot >= 0 && ev.slot < XOlokunProcessor::MaxSlots)
            {
                if (auto* eng = processor.getEngine(ev.slot))
                    colour = eng->getAccentColour();
            }
            fieldMap.addNote(ev.midiNote, ev.velocity, colour);
        });

        // ── Status Bar updates ────────────────────────────────────────────────
        // Sum voice counts across all active slots and update slot indicator dots.
        {
            int totalVoices = 0;
            for (int i = 0; i < XOlokunProcessor::MaxSlots; ++i)
            {
                if (auto* eng = processor.getEngine(i))
                {
                    totalVoices += eng->getActiveVoiceCount();
                    statusBar.setSlotActive(i, true, eng->getAccentColour());
                }
                else
                {
                    statusBar.setSlotActive(i, false, juce::Colours::transparentBlack);
                }
            }
            statusBar.setVoiceCount(totalVoices);
            // BPM and CPU: placeholder values until processor atomics are added.
            statusBar.setBpm(120.0);
            statusBar.setCpuPercent(0.0f);
        }
    }

    // kHeaderH and kFieldMapH are now defined in ColumnLayoutManager.
    // Use ColumnLayoutManager::kHeaderH (64) and ColumnLayoutManager::kFieldMapH (65).
    static constexpr int kMacroH    = 105; // MacroSection strip height — moves to header in Step 11
    static constexpr int kMasterFXH = 68;  // MasterFX compact strip at bottom of Column B
    static constexpr int kFadeMs    = 150; // Panel cross-fade duration (ms)

    XOlokunProcessor& processor;
    std::unique_ptr<GalleryLookAndFeel> laf;

    std::array<std::unique_ptr<CompactEngineTile>, XOlokunProcessor::MaxSlots> tiles;
    FieldMapPanel          fieldMap;
    OverviewPanel          overview;
    EngineDetailPanel      detail;
    ChordMachinePanel      chordPanel;
    PerformanceViewPanel   performancePanel;
    MacroSection           macros;
    MasterFXSection        masterFXStrip;
    PresetBrowserStrip     presetBrowser;
    juce::TextButton       cmToggleBtn;
    juce::TextButton       perfToggleBtn;
    juce::TextButton       surfaceToggleBtn;
    juce::TextButton       themeToggleBtn;
    juce::TextButton       exportBtn;
    PlaySurface            playSurface;
    CouplingArcOverlay     couplingArcs { processor };
    SidebarPanel           sidebar;
    StatusBar              statusBar;

    int selectedSlot = -1;

    ColumnLayoutManager layout;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(XOlokunEditor)
};

} // namespace xolokun
