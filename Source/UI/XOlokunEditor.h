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
#include "Gallery/WaveformDisplay.h"
#include "Gallery/EnginePickerPopup.h"
#include "Gallery/CouplingPopover.h"
#include "Gallery/DepthZoneDial.h"
#include "Gallery/ABCompare.h"
#include "Gallery/HeaderIndicators.h"
#include "Gallery/MiniCouplingGraph.h"
#include "Gallery/CockpitHost.h"

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
                       public CockpitHost,  // B041: Dark Cockpit opacity interface
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
          presetBrowser(proc),
          ghostTile(proc, 4)   // Ghost Slot — 5th tile, slot index 4
    {
        // Light mode is the default (brand rule); SettingsPanel restores user's saved preference.
        laf = std::make_unique<GalleryLookAndFeel>();
        setLookAndFeel(laf.get());

        // Read persisted theme preference before any component styling.
        // SettingsPanel will also read this later, but we need it now so
        // setColour() calls use the correct theme from the start.
        {
            juce::PropertiesFile::Options opts;
            opts.applicationName     = "XOlokun";
            opts.filenameSuffix      = "settings";
            opts.osxLibrarySubFolder = "Application Support";
            juce::PropertiesFile earlySettings(opts);
            GalleryColors::darkMode() = earlySettings.getBoolValue("darkMode", true);
        }

        // Primary tiles (slots 0-3)
        for (int i = 0; i < kNumPrimarySlots; ++i)
        {
            tiles[i] = std::make_unique<CompactEngineTile>(proc, i);
            tiles[i]->onSelect = [this](int slot) { selectSlot(slot); };
            addAndMakeVisible(*tiles[i]);
        }

        // Ghost tile (slot 4) — hidden until collection detection fires
        ghostTile.onSelect = [this](int slot) { selectSlot(slot); };
        addAndMakeVisible(ghostTile);
        ghostTile.setVisible(false);

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
        addAndMakeVisible(couplingHitTester);

        detail.setVisible(false);
        detail.setAlpha(0.0f);
        chordPanel.setVisible(false);
        chordPanel.setAlpha(0.0f);
        performancePanel.setVisible(false);
        performancePanel.setAlpha(0.0f);

        // "ENGINES" button in header — engine selection shortcut
        addAndMakeVisible(enginesBtn);
        enginesBtn.setButtonText(juce::String("ENGINES ") + juce::String(juce::CharPointer_UTF8("\xe2\x96\xbe")));
        enginesBtn.setTooltip("Select engine for focused slot");
        // Style: T2 text (secondary color), matches prototype .engines-btn — use theme-aware accessors
        enginesBtn.setColour(juce::TextButton::textColourOffId, juce::Colour(GalleryColors::t2()));
        enginesBtn.setColour(juce::TextButton::textColourOnId,  juce::Colour(GalleryColors::t1()));
        A11y::setup (enginesBtn, "Engines Button", "Open engine selection for the focused slot");
        enginesBtn.onClick = [this]
        {
            // Open the engine picker for the focused slot
            int slot = (selectedSlot >= 0 && selectedSlot < kNumPrimarySlots) ? selectedSlot : 0;
            depthDial.setSlot(slot);
            depthDial.openEnginePickerPopup();
        };

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

        // "CI" Cinematic Mode toggle button — collapses Columns A+C, Column B fills full width
        addAndMakeVisible(cinematicToggleBtn);
        cinematicToggleBtn.setButtonText("CI");
        cinematicToggleBtn.setTooltip("Cinematic Mode — collapse side columns so Column B fills full width (key: M)");
        A11y::setup (cinematicToggleBtn, "Cinematic Mode Toggle",
                     "Toggle cinematic mode: collapse Columns A and C so Column B fills the full width");
        cinematicToggleBtn.setClickingTogglesState(true);
        cinematicToggleBtn.onClick = [this]
        {
            layout.cinematicMode = cinematicToggleBtn.getToggleState();
            resized();
        };

        // "PS" toggle button — PlaySurface popup window (4-zone performance interface)
        addAndMakeVisible(surfaceToggleBtn);
        surfaceToggleBtn.setButtonText("PS");
        surfaceToggleBtn.setTooltip("PlaySurface — floating 4-zone performance interface (pads, orbit, strip, triggers)");
        A11y::setup (surfaceToggleBtn, "PlaySurface Toggle",
                     "Show or hide the PlaySurface popup window");
        surfaceToggleBtn.setClickingTogglesState(true);
        surfaceToggleBtn.onClick = [this]
        {
            if (surfaceToggleBtn.getToggleState())
                showPlaySurface();
            else
                hidePlaySurface();
        };
        // PlaySurface window is created lazily in showPlaySurface() on first press.
        // No addAndMakeVisible() or MIDI wiring here — happens on first open.

        // Dark mode toggle — light is default (brand rule)
        addAndMakeVisible(themeToggleBtn);
        themeToggleBtn.setButtonText("DK");
        themeToggleBtn.setTooltip("Toggle dark/light theme");
        A11y::setup (themeToggleBtn, "Dark Mode Toggle", "Switch between light and dark theme");
        themeToggleBtn.setClickingTogglesState(true);
        themeToggleBtn.onClick = [this]
        {
            GalleryColors::darkMode() = themeToggleBtn.getToggleState();
            laf->applyTheme();
            repaint();
            for (int i = 0; i < kNumPrimarySlots; ++i)
                tiles[i]->repaint();
            ghostTile.repaint();
            overview.repaint();
            detail.repaint();
            chordPanel.repaint();
            performancePanel.repaint();
            macros.repaint();
            masterFXStrip.repaint();
            presetBrowser.repaint();
            sidebar.repaint();
            // Also repaint the PlaySurface content (not the window frame) if open.
            // B4: calling repaint() on the DocumentWindow only repaints the title-bar frame;
            // the content component must be repainted directly.
            if (playSurfaceWindow != nullptr && playSurfaceWindow->isVisible())
                playSurfaceWindow->getPlaySurface().repaint();
        };
        // Sync toggle visual state to the preference we read early in the constructor.
        themeToggleBtn.setToggleState(GalleryColors::darkMode(), juce::dontSendNotification);

        // Make icon buttons wider (28px) so 2-char labels don't truncate to "..."
        // (Font size is controlled by GalleryLookAndFeel; buttons just need enough width)

        // P0-3: Slim preset nav — prev/next arrow buttons
        addAndMakeVisible(presetPrevBtn);
        presetPrevBtn.setButtonText("<");
        presetPrevBtn.setTooltip("Previous preset");
        A11y::setup(presetPrevBtn, "Previous Preset", "Go to previous preset");
        presetPrevBtn.onClick = [this]
        {
            repaint(); // placeholder: step to previous preset
        };

        addAndMakeVisible(presetNextBtn);
        presetNextBtn.setButtonText(">");
        presetNextBtn.setTooltip("Next preset");
        A11y::setup(presetNextBtn, "Next Preset", "Go to next preset");
        presetNextBtn.onClick = [this]
        {
            repaint(); // placeholder: step to next preset
        };

        // P0-4: Settings gear button — far-right header
        addAndMakeVisible(settingsBtn);
        settingsBtn.setButtonText(juce::String(juce::CharPointer_UTF8("\xe2\x9a\x99")));
        settingsBtn.setTooltip("Settings");
        A11y::setup(settingsBtn, "Settings", "Open settings panel");
        settingsBtn.setColour(juce::TextButton::textColourOffId, juce::Colour(GalleryColors::t3()));
        settingsBtn.setColour(juce::TextButton::textColourOnId,  juce::Colour(GalleryColors::t1()));
        settingsBtn.onClick = [this]
        {
            // placeholder: switch sidebar to settings tab
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
        addAndMakeVisible(sidebar);

        // ── Status Bar ────────────────────────────────────────────────────────
        addAndMakeVisible(statusBar);
        addKeyListener(statusBar.getKeyListener());

        // W01: Wire trigger-pad callbacks.
        // onPanic: sends All Notes Off on all 16 MIDI channels via the MidiCollector.
        statusBar.onFire    = [this] { processor.fireChordMachine(); };
        statusBar.onXoSend  = [this] { processor.triggerCouplingBurst(); };
        statusBar.onEchoCut = [this] { processor.killDelayTails(); };
        statusBar.onPanic   = [this]
        {
            for (int ch = 1; ch <= 16; ++ch)
                processor.getMidiCollector()
                    .addMessageToQueue(juce::MidiMessage::allNotesOff(ch));
        };

        // ── Tier 1 Gallery components ─────────────────────────────────────────
        addAndMakeVisible(depthDial);
        addAndMakeVisible(abCompare);
        addAndMakeVisible(cpuMeter);
        addAndMakeVisible(midiIndicator);
        addAndMakeVisible(miniCouplingGraph);

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
        // timer to 30Hz so the pulse animation runs smoothly.  Reverts to 1Hz
        // after the first poll cycle that finds nothing pending.
        // CQ16: setLearnCompleteCallback fires from a non-message thread — marshal to message thread.
        proc.getMIDILearnManager().setLearnCompleteCallback(
            [this](const juce::String&, int) {
                juce::MessageManager::callAsync([this] { startTimerHz(30); });
            });

        // ── DepthZoneDial wiring ──────────────────────────────────────────────
        // Default to slot 0 — updated in selectSlot() when the user picks a tile.
        depthDial.setSlot(0);
        depthDial.onEngineSelected = [this](const juce::String& engineId)
        {
            // selectedSlot tracks the currently focused slot (-1 = overview).
            // When no tile is selected, the dial operates on slot 0.
            int slot = (selectedSlot >= 0 && selectedSlot < kNumPrimarySlots)
                           ? selectedSlot : 0;
            processor.loadEngine(slot, engineId.toStdString());
            if (slot < kNumPrimarySlots && tiles[slot])
                tiles[slot]->refresh();
            if (detail.isVisible())
                detail.loadSlot(slot);
            overview.refresh();
        };

        // ── ABCompare wiring ─────────────────────────────────────────────────
        // W04: Fire onPresetLoaded() and also refresh tiles + detail panel.
        presetBrowser.onPresetLoaded = [this]()
        {
            abCompare.onPresetLoaded();
            // Refresh all primary tiles so engine names/accents update immediately.
            for (int i = 0; i < kNumPrimarySlots; ++i)
                if (tiles[i]) tiles[i]->refresh();
            // Refresh detail panel if it is currently visible.
            if (detail.isVisible())
                detail.loadSlot(selectedSlot);
        };

        // Event-driven tile refresh: only repaint the affected tile and overview on engine change.
        // CQ02: callback fires from the audio thread — marshal all UI calls to the message thread.
        proc.onEngineChanged = [this](int slot)
        {
            juce::MessageManager::callAsync([this, slot]
            {
                if (slot >= 0 && slot < kNumPrimarySlots)
                    tiles[slot]->refresh();
                else if (slot == 4)
                    ghostTile.refresh();
                overview.refresh();
                if (performancePanel.isVisible())
                    performancePanel.refresh();
                // Re-evaluate ghost tile visibility whenever any slot changes.
                checkCollectionUnlock();
            });
        };

        setSize(1100, 700);

        // ── Column C Sidebar: wire PresetManager AFTER setSize() so sidebar
        // has valid bounds when setPresetManager() calls resized() internally.
        sidebar.setPresetManager(proc.getPresetManager());

        // Wire C2–C6 panels (CouplingInspector, FXInspector, PlayControl, Export, Settings).
        sidebar.setProcessor(proc);

        // Wire MIDILearnManager into the Settings panel so the MIDI mappings
        // table stays live and the Clear All button is functional.
        if (auto* sp = sidebar.getSettingsPanel())
        {
            sp->setMidiLearnManager(&proc.getMIDILearnManager());
            // W02: Wire Performance Lock callback — syncs StatusBar lock button visual state.
            sp->onPerformanceLockChanged = [this](bool locked)
            {
                statusBar.setLocked(locked);
            };
        }

        // Auto-select slot 0 on startup — skip the overview landing page.
        // Shows parameter detail immediately instead of wasted space.
        if (processor.getEngine(0) != nullptr)
            selectSlot(0);

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
        // Destroy the PlaySurface popup window before the processor goes away.
        // This ensures the MidiMessageCollector pointer inside the window is
        // not accessed after the processor is deallocated.
        playSurfaceWindow.reset();
        setLookAndFeel(nullptr);
    }

    bool keyPressed(const juce::KeyPress& key) override
    {
        // Keys 1-4 jump directly to primary engine slots
        for (int i = 0; i < kNumPrimarySlots; ++i)
        {
            if (key == juce::KeyPress('1' + i))
            {
                if (processor.getEngine(i) != nullptr)
                    selectSlot(i);
                return true;
            }
        }
        // Key 5 jumps to ghost slot when visible
        if (key == juce::KeyPress('5') && ghostTile.isVisible())
        {
            if (processor.getEngine(4) != nullptr)
                selectSlot(4);
            return true;
        }
        // 'M' toggles Cinematic Mode
        if (key == juce::KeyPress('m') || key == juce::KeyPress('M'))
        {
            bool nowCinematic = cinematicToggleBtn.getToggleState();
            cinematicToggleBtn.setToggleState(!nowCinematic, juce::dontSendNotification);
            layout.cinematicMode = !nowCinematic;
            resized();
            return true;
        }
        // 'P' toggles the PlaySurface popup window
        if (key == juce::KeyPress('p') || key == juce::KeyPress('P'))
        {
            bool nowVisible = (playSurfaceWindow != nullptr && playSurfaceWindow->isVisible());
            surfaceToggleBtn.setToggleState(!nowVisible, juce::dontSendNotification);
            if (!nowVisible)
                showPlaySurface();
            else
                hidePlaySurface();
            return true;
        }
        // 'B' toggles Dark Cockpit bypass (fully lit regardless of audio activity)
        if (key == juce::KeyPress('b') || key == juce::KeyPress('B'))
        {
            cockpitBypass_ = !cockpitBypass_;
            repaint();
            return true;
        }
        // Escape returns to overview (and also hides PlaySurface popup if open)
        if (key == juce::KeyPress::escapeKey)
        {
            // If the PlaySurface window is in front, close it first;
            // otherwise fall through to the overview.
            if (playSurfaceWindow != nullptr && playSurfaceWindow->isVisible())
            {
                hidePlaySurface();
                surfaceToggleBtn.setToggleState(false, juce::dontSendNotification);
                return true;
            }
            showOverview();
            return true;
        }
        return false;
    }

    // Re-apply theme-sensitive colours to components that use explicit setColour()
    // calls at construction time. This fires whenever applyTheme() propagates a
    // LookAndFeel change through the component tree (e.g., user toggles dark mode).
    void lookAndFeelChanged() override
    {
        enginesBtn.setColour(juce::TextButton::textColourOffId, juce::Colour(GalleryColors::t2()));
        enginesBtn.setColour(juce::TextButton::textColourOnId,  juce::Colour(GalleryColors::t1()));
        // P0-4: Settings gear — T3 text, re-apply on theme change
        settingsBtn.setColour(juce::TextButton::textColourOffId, juce::Colour(GalleryColors::t3()));
        settingsBtn.setColour(juce::TextButton::textColourOnId,  juce::Colour(GalleryColors::t1()));
        // Re-apply any other explicit setColour() calls that use theme-aware values here.
    }

    void paint(juce::Graphics& g) override
    {
        using namespace GalleryColors;

        // ── Body background — deepest layer (body bg #080809) ────────────────
        // Note: Dark::bg is #0E0E10 (shell); body is one level darker.
        g.fillAll(juce::Colour(0xFF080809)); // body bg — below shell layer

        // ── Plugin shell — rounded rect with depth shadow ────────────────────
        // Leave 4px on each edge for shadow bleed. Shadow layers painted BEFORE shell.
        auto shellBounds = getLocalBounds().toFloat().reduced(4.0f);

        // Shadow layer 1 — widest, lowest alpha
        g.setColour(juce::Colour(0x66000000)); // ~40% black
        g.fillRoundedRectangle(shellBounds.expanded(3.0f), 14.0f);
        // Shadow layer 2 — tighter
        g.setColour(juce::Colour(0x33000000)); // ~20% black
        g.fillRoundedRectangle(shellBounds.expanded(1.5f), 13.0f);

        // Shell fill — Dark::bg (#0E0E10)
        g.setColour(juce::Colour(Dark::bg));
        g.fillRoundedRectangle(shellBounds, 12.0f);

        // Shell border — 1px subtle highlight
        g.setColour(juce::Colour(0x0EFFFFFF));
        g.drawRoundedRectangle(shellBounds, 12.0f, 1.0f);

        const int headerH = ColumnLayoutManager::kHeaderH;

        // ── Header area — surface layer, 52px nominal height ─────────────────
        auto header = getLocalBounds().removeFromTop(headerH).toFloat();

        // Surface fill — GalleryColors::surface() = #1A1A1C in dark mode
        g.setColour(get(surface()));
        g.fillRect(header);

        // Bottom border — border() = rgba(255,255,255,0.07) in dark mode
        g.setColour(border());
        g.fillRect(header.getX(), header.getBottom() - 1.0f, header.getWidth(), 1.0f);

        // ── XO_OX Dual-Circle Logo Mark (30×30px) ───────────────────────────
        // Two stroke rings with filled center dots; overlap ~8px horizontally.
        // Left circle: Reef Jade (#1E8B7E), Right circle: XO Gold (#E9C46A)
        {
            const float circR    = 7.0f;            // ring radius
            const float dotR     = 2.5f;            // center dot radius
            const float strokeW  = 1.5f;            // ring stroke width
            const float cx1      = 14.0f + circR;   // center x of left circle
            const float cx2      = cx1 + 8.0f;      // center x of right circle (offset 8px)
            const float cy       = (float)(headerH / 2);  // vertically centred in header

            // Left ring — Reef Jade stroke
            g.setColour(juce::Colour(0xFF1E8B7E).withAlpha(0.85f));
            g.drawEllipse(cx1 - circR, cy - circR, circR * 2.0f, circR * 2.0f, strokeW);
            // Left center dot
            g.fillEllipse(cx1 - dotR, cy - dotR, dotR * 2.0f, dotR * 2.0f);

            // Right ring — XO Gold stroke
            g.setColour(juce::Colour(0xFFE9C46A).withAlpha(0.85f));
            g.drawEllipse(cx2 - circR, cy - circR, circR * 2.0f, circR * 2.0f, strokeW);
            // Right center dot
            g.fillEllipse(cx2 - dotR, cy - dotR, dotR * 2.0f, dotR * 2.0f);
        }

        // Engine name — T1 text, vertically stacked in 52px header (shifted right by 34px)
        g.setColour(get(t1()));
        g.setFont(GalleryFonts::display(12.0f));
        g.drawText("XOlokun",
                   juce::Rectangle<int>(48, 6, 100, 20),
                   juce::Justification::centredLeft);

        // Subtitle — "XO_OX Designs" at 8px, T3 color (shifted right by 34px)
        g.setColour(get(t3()));
        g.setFont(GalleryFonts::body(8.0f));
        g.drawText("XO_OX Designs",
                   juce::Rectangle<int>(48, 26, 100, 14),
                   juce::Justification::centredLeft);

        // NOTE: Coupling stats / engine-count text has been moved out of paint().
        // It will be surfaced via StatusBar::setStatusText() in a future timerCallback() update
        // (Step 12). For now the header is reserved for the macro knobs component.

        // ── MIDI Learn status badge — appears in header when listening ──────
        {
            const auto& mlm = processor.getMIDILearnManager();
            if (mlm.isLearning())
            {
                // Pulse the badge alpha at ~2Hz using system time
                double t = juce::Time::getMillisecondCounterHiRes() * 0.002;
                float pulse = 0.65f + 0.35f * (float)std::sin(t * juce::MathConstants<double>::twoPi);

                juce::String badge = "MIDI LEARN: move controller to map";
                auto badgeRect = juce::Rectangle<int>(200, 4, getWidth() - 400, headerH - 8);

                g.setColour(juce::Colour(0xFFE9C46A).withAlpha(0.18f * pulse));
                g.fillRoundedRectangle(badgeRect.toFloat(), 4.0f);
                g.setColour(juce::Colour(0xFFE9C46A).withAlpha(pulse));
                g.setFont(GalleryFonts::heading(9.0f));
                g.drawText(badge, badgeRect, juce::Justification::centred);
            }
        }

        // Column A / Column B separator line — border() = rgba(255,255,255,0.07)
        const int sepX = layout.getColumnAWidth();
        if (sepX > 0)
        {
            g.setColour(border());
            g.drawVerticalLine(sepX, (float)headerH, (float)getHeight());
        }

        // ── P0-3: Preset name text — centered between prev/next arrow buttons ──
        // Draw centered preset name between presetPrevBtn and presetNextBtn.
        if (presetPrevBtn.isVisible() && presetNextBtn.isVisible())
        {
            int nameX = presetPrevBtn.getRight();
            int nameW = presetNextBtn.getX() - nameX;
            int nameY = presetPrevBtn.getY();
            int nameH = presetPrevBtn.getHeight();

            juce::String presetName = processor.getPresetManager().getCurrentPreset().name;
            if (presetName.isEmpty())
                presetName = "No Preset";

            g.setColour(get(t1()));
            g.setFont(GalleryFonts::body(12.0f));
            // Truncate with ellipsis if too long
            juce::String truncated = presetName;
            {
                auto font = GalleryFonts::body(12.0f);
                if (font.getStringWidth(presetName) > nameW - 8)
                {
                    while (truncated.length() > 1 &&
                           font.getStringWidth(truncated + "...") > nameW - 8)
                        truncated = truncated.dropLastCharacters(1);
                    truncated += "...";
                }
            }
            g.drawText(truncated, nameX, nameY, nameW, nameH, juce::Justification::centred, false);
        }

        // ── P0-12: Signal flow breadcrumb strip in Column B ───────────────────
        {
            auto colBPanelFull = layout.getColumnBPanel();
            // Strip sits at the very top of Column B panel (below header edge)
            auto stripBounds = colBPanelFull.removeFromTop(kSignalFlowStripH).toFloat();

            // Subtle gradient background: rgba(255,255,255,0.015) → transparent
            {
                juce::ColourGradient grad(
                    juce::Colour(0xFFFFFFFF).withAlpha(0.015f), stripBounds.getX(), stripBounds.getY(),
                    juce::Colour(0x00FFFFFF),                    stripBounds.getX(), stripBounds.getBottom(),
                    false);
                g.setGradientFill(grad);
                g.fillRect(stripBounds);
            }

            // Section labels: SRC1 → SRC2 → FILTER → SHAPER → FX → OUT
            static const juce::String kSections[] = { "SRC1", "SRC2", "FILTER", "SHAPER", "FX", "OUT" };
            static const int kNumSections = 6;
            const int activeSection = 0; // P0-12: default active = SRC1 (index 0)

            const float hPad = 12.0f;
            const float usableW = stripBounds.getWidth() - hPad * 2.0f;
            const float cy = stripBounds.getCentreY();

            // Count total text + arrow widths to distribute evenly
            g.setFont(GalleryFonts::value(8.5f));
            float totalTextW = 0.0f;
            for (int i = 0; i < kNumSections; ++i)
                totalTextW += g.getCurrentFont().getStringWidthFloat(kSections[i]);
            const float arrowW = g.getCurrentFont().getStringWidthFloat(" \xe2\x86\x92 "); // " → "
            float totalW = totalTextW + arrowW * (kNumSections - 1);
            float startX = stripBounds.getX() + hPad + (usableW - totalW) * 0.5f;

            for (int i = 0; i < kNumSections; ++i)
            {
                bool active = (i == activeSection);
                g.setColour(active ? get(t1()) : get(t3()));
                g.setFont(GalleryFonts::value(8.5f));

                float secW = g.getCurrentFont().getStringWidthFloat(kSections[i]);
                g.drawText(kSections[i],
                           juce::Rectangle<float>(startX, cy - 8.0f, secW + 2.0f, 16.0f),
                           juce::Justification::centredLeft, false);
                startX += secW;

                // Draw arrow separator (not after last)
                if (i < kNumSections - 1)
                {
                    g.setColour(get(t3()));
                    juce::String arrow(juce::CharPointer_UTF8(" \xe2\x86\x92 "));
                    float aW = g.getCurrentFont().getStringWidthFloat(arrow);
                    g.drawText(arrow,
                               juce::Rectangle<float>(startX, cy - 8.0f, aW + 2.0f, 16.0f),
                               juce::Justification::centredLeft, false);
                    startX += aW;
                }
            }
        }

        // Macro knobs row removed — EngineDetailPanel has real interactive MacroHeroStrip.
    }

    void resized() override
    {
        // ── Sync layout state ────────────────────────────────────────────────
        // PlaySurface is now a floating popup window — not an embedded component,
        // so playSurfaceVisible is always false for layout purposes.
        layout.playSurfaceVisible = false;
        // cinematicMode is toggled by cinematicToggleBtn (CI button in header).
        // columnCCollapsed is reserved for a future Column C collapse button.
        layout.compute(getWidth(), getHeight());

        // ── Header (52px): Logo | ENGINES | --- macros --- | CPU | PLAY | EXPORT | gear
        auto header = layout.getHeader();

        // Hide removed elements off-screen
        depthDial.setBounds(0, -100, 0, 0);
        abCompare.setBounds(0, -100, 0, 0);
        presetBrowser.setBounds(0, -200, 0, 0);
        presetPrevBtn.setBounds(0, -100, 0, 0);
        presetNextBtn.setBounds(0, -100, 0, 0);
        cinematicToggleBtn.setBounds(0, -100, 0, 0);
        cmToggleBtn.setBounds(0, -100, 0, 0);
        perfToggleBtn.setBounds(0, -100, 0, 0);
        themeToggleBtn.setBounds(0, -100, 0, 0);
        midiIndicator.setBounds(0, -100, 0, 0);

        // ── Left: Logo (painted) + ENGINES button ──────────────────────────
        header.removeFromLeft(150); // logo rings + "XOlokun" / "XO_OX Designs" text
        enginesBtn.setBounds(header.removeFromLeft(74).withSizeKeepingCentre(70, 22));

        // ── Right (from edge inward): gear | EXPORT | PLAY | CPU ───────────
        {
            auto gearSlice = header.removeFromRight(36); // 8px margin included
            settingsBtn.setBounds(gearSlice.withSizeKeepingCentre(28, 28));
        }
        exportBtn.setBounds(header.removeFromRight(56).reduced(4, 10));
        // PLAY button (was PS) — launches PlaySurface popup
        surfaceToggleBtn.setButtonText("PLAY");
        surfaceToggleBtn.setBounds(header.removeFromRight(52).withSizeKeepingCentre(48, 26));
        cpuMeter.setBounds(header.removeFromRight(68).withSizeKeepingCentre(64, 20));

        // ── Center: macros fill remaining space ────────────────────────────
        macros.setBounds(header.reduced(8, 4));

        // ── Column A — Engine Rack (full height, MacroSection now in header) ──
        auto colA = layout.getColumnA();

        // Reserve bottom 80px for MiniCouplingGraph before dividing tile space.
        static constexpr int kMiniGraphH = 80;
        miniCouplingGraph.setBounds(colA.removeFromBottom(kMiniGraphH));

        // Divide Column A among the 4 primary tiles.
        // The ghost tile sits below tile[3] at the same height, but only takes
        // space when visible — when hidden it receives zero-height bounds so it
        // doesn't steal hit-test area from anything beneath it.
        // CQ11: last tile absorbs the remainder to prevent a pixel gap from integer division.
        int tileH = colA.getHeight() / kNumPrimarySlots;
        for (int i = 0; i < kNumPrimarySlots; ++i)
        {
            int h = (i == kNumPrimarySlots - 1)
                    ? colA.getBottom() - (colA.getY() + i * tileH)
                    : tileH;
            tiles[i]->setBounds(colA.getX(), colA.getY() + i * tileH, colA.getWidth(), h);
        }

        // Ghost tile — positioned below tile[3] when visible, zero-height when hidden.
        if (ghostTile.isVisible())
        {
            ghostTile.setBounds(colA.getX(), colA.getY() + kNumPrimarySlots * tileH,
                                colA.getWidth(), tileH);
        }
        else
        {
            // Park off-screen (zero height) — no layout impact when hidden.
            ghostTile.setBounds(colA.getX(), colA.getY() + kNumPrimarySlots * tileH,
                                colA.getWidth(), 0);
        }

        // ── Column B — Panel stack + MasterFX strip + FieldMap ───────────────
        // getColumnBPanel() already excludes the FieldMap strip at the bottom.
        auto colBPanel = layout.getColumnBPanel();

        // MasterFX strip at bottom of Column B panel area
        auto masterFXBounds = colBPanel.removeFromBottom(kMasterFXH).reduced(6, 3);
        masterFXStrip.setBounds(masterFXBounds);

        // Signal flow strip (28px) at top of Column B — painted in paint().
        // Macro knobs row removed — redundant with EngineDetailPanel's MacroHeroStrip.
        colBPanel.removeFromTop(kSignalFlowStripH);

        // Remaining Column B panel area for the view stack (stacked, one visible at a time)
        overview.setBounds(colBPanel);
        detail.setBounds(colBPanel);
        chordPanel.setBounds(colBPanel);
        performancePanel.setBounds(colBPanel);

        // FieldMap hidden — 80px reclaimed for parameter sections
        fieldMap.setBounds(0, -200, 0, 0);

        // ── Column C — Tabbed Sidebar (SidebarPanel) ─────────────────────────
        sidebar.setBounds(layout.getColumnC());

        // PlaySurface is now a floating popup window — no embedded bounds to set.

        // ── Status Bar ───────────────────────────────────────────────────────
        statusBar.setBounds(layout.getStatusBar());

        // ── Coupling arc overlay — full editor bounds ─────────────────────────
        couplingArcs.setBounds(getLocalBounds());
        for (int i = 0; i < kNumPrimarySlots; ++i)
        {
            if (tiles[i])
                couplingArcs.setTileCenter(i, tiles[i]->getBounds().getCentre().toFloat());
        }
        // Ghost tile (slot 4): only register its center when visible so arcs draw correctly.
        if (ghostTile.isVisible())
            couplingArcs.setTileCenter(4, ghostTile.getBounds().getCentre().toFloat());

        // ── Coupling arc hit-tester — covers the OverviewPanel bounds ─────────
        couplingHitTester.setBounds(overview.getBounds());
    }

private:
    void selectSlot(int slot)
    {
        // Deselect all tiles (primary + ghost)
        for (int i = 0; i < kNumPrimarySlots; ++i)
            tiles[i]->setSelected(i == slot);
        ghostTile.setSelected(slot == 4);

        if (slot == selectedSlot && detail.isVisible())
            return; // already showing this one

        selectedSlot = slot;
        depthDial.setSlot(juce::jlimit(0, 3, slot)); // DepthDial tracks the selected slot
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
                if (slot != self.selectedSlot) return;  // CQ17: user clicked elsewhere during fade
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
                if (slot != self.selectedSlot) return;  // CQ17: user clicked elsewhere during fade
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
        for (int i = 0; i < kNumPrimarySlots; ++i)
            tiles[i]->setSelected(false);
        ghostTile.setSelected(false);
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
        for (int i = 0; i < kNumPrimarySlots; ++i)
            tiles[i]->setSelected(false);
        ghostTile.setSelected(false);

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
        for (int i = 0; i < kNumPrimarySlots; ++i)
            tiles[i]->setSelected(false);
        ghostTile.setSelected(false);

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

    // Dark Cockpit B041: read current performance opacity for child panels.
    // Panels call this in their paint() to dim non-essential controls during silence.
    float getCockpitOpacity() const override { return cockpitOpacity_; }

    // ── PlaySurface show/hide (popup window) ──────────────────────────────────
    // On first call, the window is created lazily and MIDI is wired.
    // Subsequent calls simply show/hide the existing window so mode selections,
    // bank, and octave state survive hide/show cycles.

    void showPlaySurface()
    {
        // Lazy creation: build window and wire MIDI on first show.
        if (playSurfaceWindow == nullptr)
        {
            playSurfaceWindow = std::make_unique<PlaySurfaceWindow>();
            // F10: Propagate LookAndFeel to all child components in the popup tree.
            if (laf)
                playSurfaceWindow->setLookAndFeel (laf.get());
            // Wire MIDI: PlaySurface note events flow through the processor's
            // MidiMessageCollector, drained into processBlock each audio callback.
            playSurfaceWindow->getPlaySurface()
                .setMidiCollector (&processor.getMidiCollector(), 1);
            // Sync the PS toggle button when the window is closed by the user.
            playSurfaceWindow->onClosed = [this]
            {
                surfaceToggleBtn.setToggleState(false, juce::dontSendNotification);
            };
        }

        playSurfaceWindow->setVisible (true);
        playSurfaceWindow->toFront (true);

        // P2-4: Set accent immediately so there is no 1-frame XO Gold flash on first open.
        // The timerCallback will keep it updated, but it may not have fired yet at this point.
        {
            juce::Colour accent(0xFFE9C46A);
            if (selectedSlot >= 0 && selectedSlot < XOlokunProcessor::MaxSlots)
            {
                if (auto* eng = processor.getEngine(selectedSlot))
                    accent = eng->getAccentColour();
            }
            else
            {
                for (int i = 0; i < XOlokunProcessor::MaxSlots; ++i)
                    if (auto* eng = processor.getEngine(i)) { accent = eng->getAccentColour(); break; }
            }
            playSurfaceWindow->getPlaySurface().setAccentColour(accent);
        }
    }

    void hidePlaySurface()
    {
        if (playSurfaceWindow != nullptr)
            playSurfaceWindow->setVisible (false);
    }

    void timerCallback() override
    {
        for (int i = 0; i < kNumPrimarySlots; ++i)
            tiles[i]->refresh();
        if (ghostTile.isVisible())
            ghostTile.refresh();
        checkCollectionUnlock();
        if (!detail.isVisible())
            overview.refresh();
        if (performancePanel.isVisible())
            performancePanel.refresh();

        // ── Refresh Export tab panel with current preset/kit info ────────────
        sidebar.refreshExportPanel();

        // ── Update coupling hit tester with current arc data ─────────────────
        {
            auto routes = processor.getCouplingMatrix().getRoutes();
            std::array<juce::Point<float>, EngineRegistry::MaxSlots> nodeCenters;
            // Compute node centers matching OverviewPanel's diamond layout
            float w = (float)overview.getWidth();
            float h = (float)overview.getHeight();
            float portraitH = h * 0.38f;
            float chainY = portraitH + (h - portraitH) * 0.28f;
            float routeY = chainY + 12.0f + 12.0f;
            float padding = 16.0f;
            auto nodeArea = juce::Rectangle<float>(padding, routeY, w - 2.0f * padding, 60.0f);
            nodeCenters[0] = nodeArea.getTopLeft().translated(8.0f, 8.0f);
            nodeCenters[1] = nodeArea.getTopRight().translated(-8.0f, 8.0f);
            nodeCenters[2] = nodeArea.getBottomLeft().translated(8.0f, -8.0f);
            nodeCenters[3] = nodeArea.getBottomRight().translated(-8.0f, -8.0f);
            // Ghost Slot position — below the 4 primary nodes if visible
            if (ghostTile.isVisible())
                nodeCenters[4] = ghostTile.getBounds().getCentre().toFloat() - overview.getPosition().toFloat();
            else
                nodeCenters[4] = nodeCenters[3].translated(0.0f, 20.0f); // fallback
            couplingHitTester.updateArcs(routes, nodeCenters);
        }

        // ── MIDI Learn: finalise pending learn captures ───────────────────────
        // checkPendingLearn() is safe to call from the message thread at any rate.
        // It reads a single atomic written by processMidi() on the audio thread
        // and, if a CC was captured, creates the mapping and fires the callback.
        {
            auto& mlm = processor.getMIDILearnManager();
            mlm.checkPendingLearn();

            // While a learn is active, keep the timer running fast (30Hz) so the
            // amber-pulse animation (2Hz sin oscillation rendered at 30fps) is
            // visually smooth.  When idle, revert to 1Hz to avoid wasting cycles.
            if (mlm.isLearning())
            {
                startTimerHz(30);
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
            // MaxSlots now includes the Ghost Slot (4) — safe to query all 5.
            if (ev.slot >= 0 && ev.slot < XOlokunProcessor::MaxSlots)
            {
                if (auto* eng = processor.getEngine(ev.slot))
                    colour = eng->getAccentColour();
            }
            fieldMap.addNote(ev.midiNote, ev.velocity, colour);
            midiIndicator.flash(colour); // flash on every incoming note event
        });

        // ── Status Bar updates ────────────────────────────────────────────────
        // Sum voice counts across all active slots and update slot indicator dots.
        // Ghost slot (4) dot is only shown when the ghost tile is visible.
        {
            int totalVoices = 0;
            for (int i = 0; i < kNumPrimarySlots; ++i)
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
            // Ghost slot dot
            if (ghostTile.isVisible())
            {
                if (auto* eng = processor.getEngine(4))
                {
                    totalVoices += eng->getActiveVoiceCount();
                    statusBar.setSlotActive(4, true, eng->getAccentColour());
                }
                else
                {
                    statusBar.setSlotActive(4, false, juce::Colours::transparentBlack);
                }
            }
            else
            {
                statusBar.setSlotActive(4, false, juce::Colours::transparentBlack);
            }
            statusBar.setVoiceCount(totalVoices);

            // W03: BPM — read from host transport if available; 0.0 means "not connected".
            {
                double bpm = 0.0;
                if (auto* ph = processor.getPlayHead())
                {
                    auto pos = ph->getPosition();
                    if (pos.hasValue() && pos->getBpm().hasValue())
                        bpm = *pos->getBpm();
                }
                statusBar.setBpm(bpm);
            }

            // W03: CPU — read from processor's measured processBlock load.
            statusBar.setCpuPercent(processor.getProcessingLoad() * 100.0f);
        }

        // ── Header indicators ─────────────────────────────────────────────────
        // CPU meter — read from processor's measured processBlock load.
        cpuMeter.setCpuPercent(processor.getProcessingLoad() * 100.0f);

        // MIDI indicator learn state — keeps amber pulse in sync.
        midiIndicator.setLearning(processor.getMIDILearnManager().isLearning());

        // ── MiniCouplingGraph ─────────────────────────────────────────────────
        miniCouplingGraph.refresh();
        for (int i = 0; i < kNumPrimarySlots; ++i)
        {
            if (tiles[i])
                miniCouplingGraph.setNodeCenter(i, (float)tiles[i]->getBounds().getCentreY());
        }
        if (ghostTile.isVisible())
            miniCouplingGraph.setNodeCenter(4, (float)ghostTile.getBounds().getCentreY());

        // ── PlaySurface accent colour — tracks focused slot (B3 fix) ──────────
        // B3: prefer selectedSlot's engine accent; fall back to first loaded engine
        // so the PlaySurface colour matches whatever engine the performer is focused on.
        if (playSurfaceWindow != nullptr && playSurfaceWindow->isVisible())
        {
            static const juce::Colour kXOGoldAccent(0xFFE9C46A);
            juce::Colour accent = kXOGoldAccent;
            if (selectedSlot >= 0 && selectedSlot < XOlokunProcessor::MaxSlots)
            {
                if (auto* eng = processor.getEngine(selectedSlot))
                    accent = eng->getAccentColour();
            }
            else
            {
                for (int i = 0; i < XOlokunProcessor::MaxSlots; ++i)
                {
                    if (auto* eng = processor.getEngine(i))
                    {
                        accent = eng->getAccentColour();
                        break;
                    }
                }
            }
            playSurfaceWindow->getPlaySurface().setAccentColour(accent);
        }

        // Dark Cockpit B041: compute UI opacity from note activity
        // When cockpitBypass_ is true, hold at 1.0 (fully lit regardless of activity)
        {
            float activity = processor.getNoteActivity();
            // 5 levels: 0.15 (ghost) → 0.35 (tertiary) → 0.55 (secondary) → 0.80 (primary) → 1.0 (active)
            // Map activity 0-1 to opacity with a floor of 0.15 (never fully invisible)
            cockpitOpacity_ = cockpitBypass_ ? 1.0f : (0.15f + activity * 0.85f);
            statusBar.setCockpitBypass(cockpitBypass_);
        }
    }

    // kHeaderH and kFieldMapH are now defined in ColumnLayoutManager.
    // Use ColumnLayoutManager::kHeaderH (52) and ColumnLayoutManager::kFieldMapH (65).
    static constexpr int kMasterFXH        = 68;  // MasterFX compact strip at bottom of Column B
    static constexpr int kSignalFlowStripH = 28;  // P0-12: signal flow breadcrumb strip
    static constexpr int kMacroKnobsRowH   = 64;  // P0-13: macro knobs row placeholder
    static constexpr int kFadeMs           = 150; // Panel cross-fade duration (ms)
    // kNumPrimarySlots: the 4 slots always visible (indices 0-3).
    // The Ghost Slot (index 4) is conditional — managed by checkCollectionUnlock().
    static constexpr int kNumPrimarySlots = 4;

    // ── Ghost Slot: collection detection + materialise/dematerialise animation ──
    //
    // Reads engine IDs for slots 0-3 and calls EngineRegistry::detectCollection().
    // When the collection condition is newly met, fades in the ghost tile.
    // When the condition is lost, fades out and hides it.
    //
    // Safe to call at any frequency; internally guards on visibility state changes
    // to avoid redundant animation calls.
    //
    void checkCollectionUnlock()
    {
        std::array<juce::String, 4> ids;
        for (int i = 0; i < kNumPrimarySlots; ++i)
        {
            auto* eng = processor.getEngine(i);
            ids[static_cast<size_t>(i)] = eng ? eng->getEngineId() : juce::String();
        }

        const auto collection  = EngineRegistry::detectCollection(ids);
        const bool shouldShow  = collection.isNotEmpty();
        auto& anim             = juce::Desktop::getInstance().getAnimator();

        if (shouldShow && !ghostTile.isVisible())
        {
            // Materialise: set alpha 0, make visible, animate to alpha 1.
            ghostTile.setAlpha(0.0f);
            ghostTile.setVisible(true);
            resized(); // give the ghost tile its bounds before animating
            anim.animateComponent(&ghostTile, ghostTile.getBounds(),
                                  1.0f, 500, false, 1.0, 0.0);
        }
        else if (!shouldShow && ghostTile.isVisible())
        {
            // Dematerialise: fade out, then hide and recompute layout.
            anim.animateComponent(&ghostTile, ghostTile.getBounds(),
                                  0.0f, 100, false, 1.0, 0.0);
            juce::Component::SafePointer<XOlokunEditor> safeThis(this);
            juce::Timer::callAfterDelay(120, [safeThis]()
            {
                if (safeThis == nullptr) return;
                if (safeThis->ghostTile.getAlpha() < 0.05f)
                    safeThis->ghostTile.setVisible(false);
                // resized() is NOT called here — hiding a zero-height off-screen
                // tile has no visual impact on the 4 primary tiles.
            });
        }
    }

    XOlokunProcessor& processor;
    std::unique_ptr<GalleryLookAndFeel> laf;

    std::array<std::unique_ptr<CompactEngineTile>, kNumPrimarySlots> tiles;
    FieldMapPanel          fieldMap;
    OverviewPanel          overview;
    EngineDetailPanel      detail;
    ChordMachinePanel      chordPanel;
    PerformanceViewPanel   performancePanel;
    MacroSection           macros;
    MasterFXSection        masterFXStrip;
    PresetBrowserStrip     presetBrowser;
    // Ghost Slot tile — declared after presetBrowser to match constructor MIL order.
    // Hidden until EngineRegistry::detectCollection() returns a non-empty collection.
    CompactEngineTile      ghostTile;
    juce::TextButton       enginesBtn;
    juce::TextButton       cinematicToggleBtn;
    juce::TextButton       cmToggleBtn;
    juce::TextButton       perfToggleBtn;
    juce::TextButton       surfaceToggleBtn;
    juce::TextButton       themeToggleBtn;
    juce::TextButton       exportBtn;
    // P0-3: Slim inline preset nav
    juce::TextButton       presetPrevBtn;
    juce::TextButton       presetNextBtn;
    // P0-4: Settings gear button
    juce::TextButton       settingsBtn;
    // PlaySurface lives in a floating DocumentWindow popup (created lazily on first show).
    std::unique_ptr<PlaySurfaceWindow> playSurfaceWindow;
    CouplingArcOverlay     couplingArcs { processor };
    CouplingArcHitTester   couplingHitTester { processor };
    SidebarPanel           sidebar;
    StatusBar              statusBar;

    // ── Tier 1 Gallery components ─────────────────────────────────────────────
    DepthZoneDial          depthDial    { processor };
    ABCompare              abCompare    { processor };
    CPUMeter               cpuMeter;
    MIDIActivityIndicator  midiIndicator;
    MiniCouplingGraph      miniCouplingGraph { processor };

    int selectedSlot = -1;

    // Dark Cockpit B041: current UI opacity derived from note activity.
    // 0.15 (ghost, silent) → 1.0 (fully lit, maximum activity).
    float cockpitOpacity_ = 0.2f;
    // When true, cockpit is bypassed — UI held at full opacity regardless of activity.
    // Toggle with 'B' key. Shown as COCKPIT: OFF in status bar paint.
    bool cockpitBypass_ = false;

    ColumnLayoutManager layout;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(XOlokunEditor)
};

} // namespace xolokun
