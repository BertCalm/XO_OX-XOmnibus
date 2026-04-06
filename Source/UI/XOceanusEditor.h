// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "../XOceanusProcessor.h"
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
// PresetBrowser.h opens its own namespace xoceanus { } block — must be included
// at file scope (before our namespace xoceanus { below) to avoid nesting.
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
#include "Gallery/DnaHexagon.h"
#include "RegisterManager.h"
#include "ToastOverlay.h"

namespace xoceanus
{

// GalleryColors, GalleryFonts, and A11y are provided by GalleryColors.h
// (included above).  Their definitions are canonical in that file; they are
// available here via the xoceanus:: namespace that GalleryColors.h opens.

//==============================================================================
// XOceanusEditor — Gallery Model plugin window.
//
// Layout:
//   ┌──────────────────────────────────────────────────────────────────────────┐
//   │  Header: Logo | DepthDial | < | > | DNA | A/B | CI | CM | P | DK |      │
//   │          KEYS | EXPORT | gear                                            │
//   ├────────┬─────────────────────────────────────────┬────────────────────── ┤
//   │ Tile 1 │  [MacroSection — top of Col B, 56pt]    │                       │
//   │ Tile 2 │  ─────────────────────────────────────  │  Column C (Sidebar)   │
//   │ Tile 3 │  OverviewPanel / EngineDetailPanel       │                       │
//   │ Tile 4 │                                          │                       │
//   ├────────┴─────────────────────────────────────────┴───────────────────────┤
//   │  StatusBar (BPM · Voices · CPU label · MIDI dot · slot dots · LK)        │
//   └──────────────────────────────────────────────────────────────────────────┘
//
// Transition: 150ms opacity cross-fade via juce::ComponentAnimator
// when switching between overview and engine detail, or between engines.
//
class XOceanusEditor : public juce::AudioProcessorEditor,
                       public CockpitHost, // B041: Dark Cockpit opacity interface
                       private juce::Timer
{
public:
    explicit XOceanusEditor(XOceanusProcessor& proc)
        : AudioProcessorEditor(proc), processor(proc), overview(proc), detail(proc), chordPanel(proc),
          performancePanel(proc), macros(proc.getAPVTS()), masterFXStrip(proc.getAPVTS()), presetBrowser(proc),
          ghostTile(proc, 4) // Ghost Slot — 5th tile, slot index 4
    {
        // Dark mode is primary; SettingsPanel restores user's saved preference.
        laf = std::make_unique<GalleryLookAndFeel>();
        setLookAndFeel(laf.get());

        // Read persisted theme preference before any component styling.
        // SettingsPanel will also read this later, but we need it now so
        // setColour() calls use the correct theme from the start.
        {
            juce::PropertiesFile::Options opts;
            opts.applicationName = "XOceanus";
            opts.filenameSuffix = "settings";
            opts.osxLibrarySubFolder = "Application Support";
            juce::PropertiesFile earlySettings(opts);
            const bool savedDark = earlySettings.getBoolValue("darkMode", true);
            // Register this instance in the per-instance dark mode registry (fix #329).
            // unregisterInstance() is called in the destructor.
            GalleryColors::setInstanceDarkMode(this, savedDark);
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
        enginesBtn.setColour(juce::TextButton::textColourOnId, juce::Colour(GalleryColors::t1()));
        A11y::setup(enginesBtn, "Engines Button", "Open engine selection for the focused slot");
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
        A11y::setup(cmToggleBtn, "Chord Machine Toggle", "Toggle the chord machine sequencer panel");
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
        A11y::setup(perfToggleBtn, "Performance View Toggle", "Toggle the coupling performance panel");
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
        A11y::setup(cinematicToggleBtn, "Cinematic Mode Toggle",
                    "Toggle cinematic mode: collapse Columns A and C so Column B fills the full width");
        cinematicToggleBtn.setClickingTogglesState(true);
        cinematicToggleBtn.onClick = [this]
        {
            layout.cinematicMode = cinematicToggleBtn.getToggleState();
            resized();
        };

        // "PS" toggle button — PlaySurface embedded zone (4-zone performance interface)
        // spec §2.2: PlaySurface occupies a fixed 264pt bottom zone of the main plugin window.
        addAndMakeVisible(surfaceToggleBtn);
        surfaceToggleBtn.setButtonText("KEYS");
        surfaceToggleBtn.setTooltip(
            "KEYS — show or hide the embedded PlaySurface keyboard/pads zone");
        A11y::setup(surfaceToggleBtn, "Keys Toggle", "Show or hide the embedded PlaySurface zone");
        surfaceToggleBtn.setClickingTogglesState(true);
        surfaceToggleBtn.onClick = [this]
        {
            if (surfaceToggleBtn.getToggleState())
                showPlaySurface();
            else
                hidePlaySurface();
        };

        // ── Embedded PlaySurface — added as direct child (spec §2.2) ─────────────
        // Wire MIDI and processor immediately (no lazy creation needed).
        playSurface_.setMidiCollector(&processor.getMidiCollector(), 1);
        playSurface_.setProcessor(&processor);
        // Wire TideController default target: CHARACTER macro (macro1).
        if (auto* p = dynamic_cast<juce::RangedAudioParameter*>(proc.getAPVTS().getParameter("macro1")))
            playSurface_.setTideTargetParameter(p);
        // Wire LookAndFeel so embedded surface matches the plugin theme.
        if (laf)
            playSurface_.setLookAndFeel(laf.get());
        addAndMakeVisible(playSurface_);
        // V2 fix: On first launch (no persisted state) show the PlaySurface so the
        // user sees a playable keyboard immediately.  On subsequent launches, restore
        // the last state.  "playSurfaceVisible" is absent on fresh install, so we
        // default to true; after any explicit toggle it is written and honoured.
        {
            juce::PropertiesFile::Options psOpts;
            psOpts.applicationName = "XOceanus";
            psOpts.filenameSuffix = "settings";
            psOpts.osxLibrarySubFolder = "Application Support";
            juce::PropertiesFile psSettings(psOpts);
            const bool hasSavedState = psSettings.containsKey("playSurfaceVisible");
            const bool shouldShow    = hasSavedState ? psSettings.getBoolValue("playSurfaceVisible", false)
                                                     : true; // first launch: default open
            playSurface_.setVisible(shouldShow);
            surfaceToggleBtn.setToggleState(shouldShow, juce::dontSendNotification);
        }

        // Dark mode toggle — dark is primary (brand rule)
        addAndMakeVisible(themeToggleBtn);
        themeToggleBtn.setButtonText("DK");
        themeToggleBtn.setTooltip("Toggle dark/light theme");
        A11y::setup(themeToggleBtn, "Dark Mode Toggle", "Switch between light and dark theme");
        themeToggleBtn.setClickingTogglesState(true);
        themeToggleBtn.onClick = [this]
        {
            const bool newState = themeToggleBtn.getToggleState();
            GalleryColors::setInstanceDarkMode(this, newState);
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
            // Also repaint the embedded PlaySurface if visible.
            if (playSurface_.isVisible())
                playSurface_.repaint();
            // Persist preference so the theme survives plugin reload (#215).
            juce::PropertiesFile::Options opts;
            opts.applicationName = "XOceanus";
            opts.filenameSuffix = "settings";
            opts.osxLibrarySubFolder = "Application Support";
            juce::PropertiesFile settings(opts);
            settings.setValue("darkMode", newState);
        };
        // Sync toggle visual state to the preference we read early in the constructor.
        themeToggleBtn.setToggleState(GalleryColors::darkMode(), juce::dontSendNotification);

        // D4: Register lock toggle — small padlock icon in header.
        // Locks the current visual register so auto-switching is paused.
        // Unicode: U+1F512 CLOSED LOCK, U+1F513 OPEN LOCK (UTF-8 encoded).
        addAndMakeVisible(registerLockBtn);
        registerLockBtn.setButtonText(juce::String(juce::CharPointer_UTF8("\xf0\x9f\x94\x13"))); // open lock
        registerLockBtn.setTooltip("Lock visual register — prevent auto-switching between Gallery/Performance/Coupling modes");
        A11y::setup(registerLockBtn, "Register Lock", "Lock or unlock automatic visual register switching");
        registerLockBtn.setClickingTogglesState(true);
        registerLockBtn.onClick = [this]
        {
            registerMgr_.setLocked(registerLockBtn.getToggleState());
            // Update icon: closed lock when locked, open lock when unlocked.
            const bool locked = registerMgr_.isLocked();
            // U+1F512 CLOSED LOCK (\xf0\x9f\x94\x92) / U+1F513 OPEN LOCK (\xf0\x9f\x94\x13)
            registerLockBtn.setButtonText(juce::String(juce::CharPointer_UTF8(
                locked ? "\xf0\x9f\x94\x92" : "\xf0\x9f\x94\x13")));
            // Persist lock state in processor so it survives DAW session reload.
            processor.setPersistedRegisterLocked(locked);
            processor.setPersistedRegisterCurrent(static_cast<int>(registerMgr_.current()));
        };

        // Make icon buttons wider (28px) so 2-char labels don't truncate to "..."
        // (Font size is controlled by GalleryLookAndFeel; buttons just need enough width)

        // P0-3: Slim preset nav — prev/next arrow buttons
        addAndMakeVisible(presetPrevBtn);
        presetPrevBtn.setButtonText("<");
        presetPrevBtn.setTooltip("Previous preset");
        A11y::setup(presetPrevBtn, "Previous Preset", "Go to previous preset");
        presetPrevBtn.onClick = [this]
        {
            auto& pm = processor.getPresetManager();
            pm.previousPreset();
            try
            {
                const auto& preset = pm.getCurrentPreset();
                processor.getUndoManager().beginNewTransaction("Load preset: " + preset.name);
                processor.applyPreset(preset);
            }
            catch (const std::exception& e)
            {
                DBG("Preset load failed (prev): " << e.what());
            }
            repaint();
        };

        addAndMakeVisible(presetNextBtn);
        presetNextBtn.setButtonText(">");
        presetNextBtn.setTooltip("Next preset");
        A11y::setup(presetNextBtn, "Next Preset", "Go to next preset");
        presetNextBtn.onClick = [this]
        {
            auto& pm = processor.getPresetManager();
            pm.nextPreset();
            try
            {
                const auto& preset = pm.getCurrentPreset();
                processor.getUndoManager().beginNewTransaction("Load preset: " + preset.name);
                processor.applyPreset(preset);
            }
            catch (const std::exception& e)
            {
                DBG("Preset load failed (next): " << e.what());
            }
            repaint();
        };

        // P0-4: Settings gear button — far-right header
        addAndMakeVisible(settingsBtn);
        settingsBtn.setButtonText(juce::String(juce::CharPointer_UTF8("\xe2\x9a\x99")));
        settingsBtn.setTooltip("Settings");
        A11y::setup(settingsBtn, "Settings", "Open settings panel");
        settingsBtn.setColour(juce::TextButton::textColourOffId, juce::Colour(GalleryColors::t3()));
        settingsBtn.setColour(juce::TextButton::textColourOnId, juce::Colour(GalleryColors::t1()));
        settingsBtn.onClick = [this] { sidebar.selectTab(SidebarPanel::Settings); };

        // Export button — launches ExportDialog as a CallOutBox
        addAndMakeVisible(exportBtn);
        exportBtn.setButtonText("XPN");
        exportBtn.setTooltip("Export presets as MPC-compatible XPN expansion pack");
        A11y::setup(exportBtn, "Export", "Open export dialog to build XPN expansion packs");
        exportBtn.onClick = [this]
        {
            juce::CallOutBox::launchAsynchronously(std::make_unique<ExportDialog>(processor.getPresetManager(),
                                                                                  &processor.getAPVTS(),
                                                                                  &processor.getCouplingMatrix()),
                                                   exportBtn.getScreenBounds(), getTopLevelComponent());
        };

        // Issue #712 — Scan factory preset directory on a background thread so
        // the message thread is never blocked by disk I/O.
        // `setScanning(true)` shows "Loading presets…" immediately; the async
        // callback fires on the message thread once the library is ready.
        auto presetDir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                             .getChildFile("Application Support/XO_OX/XOceanus/Presets");
        presetBrowser.setMacroSection(&macros); // wire preset macroLabels → macro knob labels
        if (presetDir.isDirectory())
        {
            presetBrowser.setScanning(true);
            // SafePointer guards against the editor being destroyed before the
            // background scan completes (e.g., plugin window closed quickly).
            juce::Component::SafePointer<XOceanusEditor> safeThis(this);
            proc.getPresetManager().scanPresetDirectoryAsync(
                presetDir,
                [safeThis]()
                {
                    if (safeThis == nullptr)
                        return;
                    safeThis->presetBrowser.setScanning(false); // clears loading state + refreshes strip
                    // Fix S2: also refresh the sidebar's PresetBrowser panel so it re-reads the
                    // library if it was constructed before the async scan completed (shows 0 presets
                    // forever without this call).
                    safeThis->sidebar.refreshPresetBrowser();

                    // Issue #899 — If the directory existed but contained no valid
                    // presets (empty factory install or all files malformed), fall back
                    // to the embedded Init preset so the user isn't left with silence.
                    // Guard: skip if a named preset is already active (restored from
                    // DAW session state or loaded by another path).
                    auto& pm = safeThis->processor.getPresetManager();
                    if (pm.getLibrary()->empty() &&
                        pm.getCurrentPreset().name.isEmpty())
                    {
                        auto initPreset = pm.loadEmbeddedInitPreset();
                        if (initPreset.name.isNotEmpty())
                        {
                            // If slot 0 has no engine (silence condition), load
                            // Odyssey and apply Welcome parameters.  If an engine
                            // is already present (e.g. Oxbow from the first-breath
                            // path in prepareToPlay), just update the preset name
                            // for the browser strip without clobbering the sound.
                            if (safeThis->processor.getEngine(0) == nullptr)
                            {
                                safeThis->processor.loadEngine(0, "Odyssey");
                                safeThis->processor.applyPreset(initPreset);
                            }
                            pm.addPreset(initPreset);
                            safeThis->presetBrowser.updateDisplay();
                            safeThis->sidebar.refreshPresetBrowser();
                        }
                    }
                });
        }
        else
        {
            // Issue #899 — No preset directory at all (fresh install or user
            // deleted the directory).  Load the embedded Init preset from
            // BinaryData immediately on the message thread so the user hears
            // sound on the first key press and sees a named preset in the strip.
            // Guard: skip if a named preset was already restored from DAW state.
            auto& pm = proc.getPresetManager();
            if (pm.getCurrentPreset().name.isEmpty())
            {
                auto initPreset = pm.loadEmbeddedInitPreset();
                if (initPreset.name.isNotEmpty())
                {
                    // If slot 0 is empty (silence condition), load Odyssey and
                    // apply Welcome parameters.  If already populated (e.g.
                    // first-breath Oxbow from prepareToPlay), preserve that
                    // engine and just register the preset name for display.
                    if (proc.getEngine(0) == nullptr)
                    {
                        proc.loadEngine(0, "Odyssey");
                        proc.applyPreset(initPreset);
                    }
                    pm.addPreset(initPreset);
                }
            }
            presetBrowser.updateDisplay();
        }

        // ── Column C Sidebar ──────────────────────────────────────────────────
        addAndMakeVisible(sidebar);

        // ── Status Bar ────────────────────────────────────────────────────────
        addAndMakeVisible(statusBar);
        addKeyListener(statusBar.getKeyListener());

        // W01: Wire trigger-pad callbacks.
        // onPanic: sends All Notes Off on all 16 MIDI channels via the MidiCollector.
        statusBar.onFire = [this] { processor.fireChordMachine(); };
        statusBar.onXoSend = [this] { processor.triggerCouplingBurst(); };
        statusBar.onEchoCut = [this] { processor.killDelayTails(); };
        statusBar.onPanic = [this]
        {
            for (int ch = 1; ch <= 16; ++ch)
                processor.getMidiCollector().addMessageToQueue(juce::MidiMessage::allNotesOff(ch));
        };

        // ── Tier 1 Gallery components ─────────────────────────────────────────
        addAndMakeVisible(depthDial);
        addAndMakeVisible(abCompare);
        addAndMakeVisible(cpuMeter);
        addAndMakeVisible(midiIndicator);
        addAndMakeVisible(miniCouplingGraph);

        // Header DNA hexagon — initialise with default DNA and XO Gold accent.
        // Accent and DNA are updated in timerCallback() once the first preset loads.
        addAndMakeVisible(headerHex_);
        A11y::setup(headerHex_, "Sonic DNA",
                    "Hexagonal visualization of the active preset's 6D Sonic DNA fingerprint");

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
            [this](const juce::String&, int) { juce::MessageManager::callAsync([this] { startTimerHz(30); }); });

        // ── DepthZoneDial wiring ──────────────────────────────────────────────
        // Default to slot 0 — updated in selectSlot() when the user picks a tile.
        depthDial.setSlot(0);
        depthDial.onEngineSelected = [this](const juce::String& engineId)
        {
            // selectedSlot tracks the currently focused slot (-1 = overview).
            // When no tile is selected, the dial operates on slot 0.
            int slot = (selectedSlot >= 0 && selectedSlot < kNumPrimarySlots) ? selectedSlot : 0;
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
                if (tiles[i])
                    tiles[i]->refresh();
            // Refresh detail panel if it is currently visible.
            if (detail.isVisible())
                detail.loadSlot(selectedSlot);
        };

        // Event-driven tile refresh: only repaint the affected tile and overview on engine change.
        // CQ02: callback fires from the audio thread — marshal all UI calls to the message thread.
        proc.onEngineChanged = [this](int slot)
        {
            juce::MessageManager::callAsync(
                [this, slot]
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

        // Base plugin height is 700pt (PlaySurface collapsed).
        // V2: On first launch the PlaySurface starts open, so add kPlaySurfaceH immediately.
        const int initialHeight = playSurface_.isVisible()
                                      ? 700 + ColumnLayoutManager::kPlaySurfaceH
                                      : 700;
        setSize(1100, initialHeight);

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
            sp->onPerformanceLockChanged = [this](bool locked) { statusBar.setLocked(locked); };
            // #226: Wire CPU Meters toggle — live visibility control on StatusBar.
            sp->onCpuMetersVisibilityChanged = [this](bool visible) { statusBar.setCpuVisible(visible); };
            // Apply persisted CPU meters visibility at startup.
            statusBar.setCpuVisible(sp->isCpuMetersVisible());
        }

        // Restore editor UI state from the last session (#357, #314).
        // Override the default slot=0 with the persisted selection.
        cockpitBypass_ = processor.getPersistedCockpitBypass();
        signalFlowActiveSection = processor.getPersistedSignalFlowSection();

        // D4: Restore register lock state from persisted processor state.
        {
            const bool locked = processor.getPersistedRegisterLocked();
            const int  reg    = processor.getPersistedRegisterCurrent();
            registerMgr_.restoreFromXmlValues(locked, reg);
            registerLockBtn.setToggleState(locked, juce::dontSendNotification);
            // Sync icon to restored state.
            registerLockBtn.setButtonText(juce::String(juce::CharPointer_UTF8(
                locked ? "\xf0\x9f\x94\x92" : "\xf0\x9f\x94\x13")));
        }

        {
            const int restoredSlot = processor.getPersistedSelectedSlot();
            const int slotToShow = (restoredSlot >= 0 && restoredSlot < XOceanusProcessor::MaxSlots &&
                                    processor.getEngine(restoredSlot) != nullptr)
                                       ? restoredSlot
                                       : -1;

            // Auto-select on startup — skip the overview landing page.
            // Direct visibility (no animation) to ensure interactive from first frame.
            const int effectiveSlot = (slotToShow >= 0) ? slotToShow : (processor.getEngine(0) != nullptr ? 0 : -1);

            if (effectiveSlot >= 0)
            {
                selectedSlot = effectiveSlot;
                if (effectiveSlot < kNumPrimarySlots)
                    tiles[effectiveSlot]->setSelected(true);
                else
                    ghostTile.setSelected(true);
                if (detail.loadSlot(effectiveSlot))
                {
                    overview.setVisible(false);
                    detail.setAlpha(1.0f);
                    detail.setVisible(true);
                    couplingHitTester.setVisible(false); // hide — overlaps detail panel bounds
                    if (auto* eng = processor.getEngine(effectiveSlot))
                        masterFXStrip.setAccentColour(eng->getAccentColour());
                }
            }
        }

        setResizable(true, true);
        // PlaySurface adds 264pt when expanded; max height allows for both states.
        setResizeLimits(960, 600, 1600, 1000 + ColumnLayoutManager::kPlaySurfaceH);
        setWantsKeyboardFocus(true);
        setTitle("XOceanus Synthesizer");
        setDescription("Multi-engine synthesizer with cross-engine coupling. "
                       "Keys 1-4 select engine slots, Escape returns to overview.");

        // ── ToastOverlay — MUST be the last addAndMakeVisible call ────────────
        // JUCE paints children in insertion order; last child paints on top.
        // setInterceptsMouseClicks(false, false) is set inside ToastOverlay's
        // constructor so it never captures events that should reach panels below.
        addAndMakeVisible(toastOverlay_);
        toastOverlay_.setAlwaysOnTop(true);
        ToastOverlay::setInstance(&toastOverlay_);

        startTimerHz(1); // Reduced from 5Hz — idle polling only as a fallback
    }

    ~XOceanusEditor() override
    {
        stopTimer();
        removeKeyListener(statusBar.getKeyListener());
        processor.onEngineChanged = nullptr; // prevent callback after editor is destroyed
        // Detach the embedded PlaySurface from the processor before the processor
        // goes away, so the MidiMessageCollector pointer is not accessed after dealloc.
        playSurface_.setProcessor(nullptr);
        // Clear the ToastOverlay singleton before child components are destroyed
        // so any in-flight callAsync lambdas find instance_ == nullptr and are no-ops.
        ToastOverlay::setInstance(nullptr);
        // S4: Remove this editor from the per-instance dark mode registry BEFORE
        // child components are destroyed, so no child paint() call that fires
        // during teardown finds a dangling pointer in the registry.
        GalleryColors::unregisterInstance(this);
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
        // Shift+P toggles Performance View — checked before plain 'P' so it isn't
        // consumed by the PlaySurface handler below.
        if (key == juce::KeyPress('P', juce::ModifierKeys::shiftModifier, 0))
        {
            bool nowOn = perfToggleBtn.getToggleState();
            perfToggleBtn.setToggleState(!nowOn, juce::dontSendNotification);
            if (!nowOn)
            {
                cmToggleBtn.setToggleState(false, juce::dontSendNotification);
                showPerformanceView();
            }
            else
            {
                showOverview();
            }
            return true;
        }
        // 'P' (without shift) toggles the embedded PlaySurface zone
        if (key == juce::KeyPress('p') || key == juce::KeyPress('P'))
        {
            bool nowVisible = playSurface_.isVisible();
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
            processor.setPersistedCockpitBypass(cockpitBypass_); // persist for session restore (#357)
            repaint();
            return true;
        }
        // Escape returns to overview (and also collapses embedded PlaySurface if open)
        if (key == juce::KeyPress::escapeKey)
        {
            // If the PlaySurface zone is expanded, collapse it first;
            // otherwise fall through to the overview.
            if (playSurface_.isVisible())
            {
                hidePlaySurface();
                surfaceToggleBtn.setToggleState(false, juce::dontSendNotification);
                return true;
            }
            showOverview();
            return true;
        }
        // Cmd+Z — undo last parameter change or preset load
        if (key == juce::KeyPress('z', juce::ModifierKeys::commandModifier, 0))
        {
            processor.getUndoManager().undo();
            return true;
        }
        // Cmd+Shift+Z — redo
        if (key == juce::KeyPress('z', juce::ModifierKeys::commandModifier | juce::ModifierKeys::shiftModifier, 0))
        {
            processor.getUndoManager().redo();
            return true;
        }
        return false;
    }

    // Re-apply theme-sensitive colours to components that use explicit setColour()
    // calls at construction time. This fires whenever applyTheme() propagates a
    // LookAndFeel change through the component tree (e.g., user toggles dark mode).
    void lookAndFeelChanged() override
    {
        // D1: enginesBtn is parked off-screen (replaced by depthDial) — no need to re-apply colours.
        // P0-4: Settings gear — T3 text, re-apply on theme change
        settingsBtn.setColour(juce::TextButton::textColourOffId, juce::Colour(GalleryColors::t3()));
        settingsBtn.setColour(juce::TextButton::textColourOnId, juce::Colour(GalleryColors::t1()));
        // Re-apply any other explicit setColour() calls that use theme-aware values here.
    }

    // Signal flow strip mouse interaction (MUST A1-02)
    void mouseDown(const juce::MouseEvent& e) override
    {
        if (signalFlowStripBounds.contains(e.position.toInt()))
        {
            for (int i = 0; i < 6; ++i)
            {
                if (sfHitRects[static_cast<size_t>(i)].contains(e.position))
                {
                    signalFlowActiveSection = i;
                    processor.setPersistedSignalFlowSection(i); // persist for session restore (#357)
                    scrollDetailPanelToSection(i);
                    repaint(signalFlowStripBounds);
                    return;
                }
            }
        }
    }

    void mouseMove(const juce::MouseEvent& e) override
    {
        if (!signalFlowStripBounds.contains(e.position.toInt()))
        {
            if (signalFlowHoveredSection != -1)
            {
                signalFlowHoveredSection = -1;
                repaint(signalFlowStripBounds);
            }
            return;
        }
        int newHovered = -1;
        for (int i = 0; i < 6; ++i)
        {
            if (sfHitRects[static_cast<size_t>(i)].contains(e.position))
            {
                newHovered = i;
                break;
            }
        }
        if (newHovered != signalFlowHoveredSection)
        {
            signalFlowHoveredSection = newHovered;
            repaint(signalFlowStripBounds);
        }
    }

    // Scroll the EngineDetailPanel viewport to the section matching sfIndex (MUST A1-06)
    void scrollDetailPanelToSection(int sfIndex)
    {
        // Map signal flow index → ParameterGrid::Section
        // SRC1→OSC, SRC2→OSC, FILTER→FILTER, SHAPER→MOD, FX→FX, OUT→OTHER
        static const ParameterGrid::Section kSfToSection[] = {
            ParameterGrid::Section::OSC, ParameterGrid::Section::OSC, ParameterGrid::Section::FILTER,
            ParameterGrid::Section::MOD, ParameterGrid::Section::FX,  ParameterGrid::Section::OTHER};
        if (sfIndex < 0 || sfIndex >= 6)
            return;
        detail.scrollToSection(kSfToSection[sfIndex]);
    }

    void paint(juce::Graphics& g) override
    {
        // S4: Set the active dark mode context for this instance before any paint
        // logic (or child components) query GalleryColors::darkMode().
        GalleryColors::setActiveDarkModeContext(this);

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
            const float circR = 7.0f;              // ring radius
            const float dotR = 2.5f;               // center dot radius
            const float strokeW = 1.5f;            // ring stroke width
            const float cx1 = 14.0f + circR;       // center x of left circle
            const float cx2 = cx1 + 8.0f;          // center x of right circle (offset 8px)
            const float cy = (float)(headerH / 2); // vertically centred in header

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

        // Engine name — T1 text, proportional to header height and component width.
        // Logo mark ends at ~cx2 + circR = 14 + 7 + 8 + 7 = 36px; start text at 48px.
        // Text area extends to half the header width to avoid overlap with controls.
        {
            const int textX = 48;
            const int textMaxW = getWidth() / 2 - textX;
            const int nameH = juce::roundToInt(headerH * 0.38f);
            const int nameY = juce::roundToInt(headerH * 0.12f);
            const int subH = juce::roundToInt(headerH * 0.27f);
            const int subY = juce::roundToInt(headerH * 0.52f);

            g.setColour(get(t1()));
            g.setFont(GalleryFonts::display(12.0f));
            g.drawText("XOceanus", juce::Rectangle<int>(textX, nameY, textMaxW, nameH),
                       juce::Justification::centredLeft);

            // Subtitle — "XO_OX Designs" at 10px, T3 color
            g.setColour(get(t3()));
            g.setFont(GalleryFonts::body(10.0f));
            g.drawText("XO_OX Designs", juce::Rectangle<int>(textX, subY, textMaxW, subH),
                       juce::Justification::centredLeft);
        }

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
                    while (truncated.length() > 1 && font.getStringWidth(truncated + "...") > nameW - 8)
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
                juce::ColourGradient grad(juce::Colour(0xFFFFFFFF).withAlpha(0.015f), stripBounds.getX(),
                                          stripBounds.getY(), juce::Colour(0x00FFFFFF), stripBounds.getX(),
                                          stripBounds.getBottom(), false);
                g.setGradientFill(grad);
                g.fillRect(stripBounds);
            }

            // Section labels: SRC1 → SRC2 → FILTER → SHAPER → FX → OUT
            static const juce::String kSections[] = {"SRC1", "SRC2", "FILTER", "SHAPER", "FX", "OUT"};
            static const int kNumSections = 6;
            const int activeSection = signalFlowActiveSection;

            const float hPad = 12.0f;
            const float usableW = stripBounds.getWidth() - hPad * 2.0f;
            const float cy = stripBounds.getCentreY();

            // Determine engine accent color for the active label
            juce::Colour activeAccent = get(t1());
            {
                if (selectedSlot >= 0)
                {
                    if (auto* eng = processor.getEngine(selectedSlot))
                        activeAccent = eng->getAccentColour();
                }
            }

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
                bool hovered = (!active && i == signalFlowHoveredSection);
                g.setFont(GalleryFonts::value(8.5f));

                float secW = g.getCurrentFont().getStringWidthFloat(kSections[i]);
                juce::Rectangle<float> secRect(startX, cy - 8.0f, secW + 6.0f, 16.0f);

                // Pill background for active label (4% white)
                if (active)
                {
                    g.setColour(juce::Colour(0xFFFFFFFF).withAlpha(0.04f));
                    g.fillRoundedRectangle(secRect.reduced(-3.0f, -3.0f), 3.0f);
                }

                // Text color: active=accent, hover=T2, inactive=T3
                if (active)
                    g.setColour(activeAccent);
                else if (hovered)
                    g.setColour(get(t2()));
                else
                    g.setColour(get(t3()));

                g.drawText(kSections[i], secRect, juce::Justification::centredLeft, false);

                startX += secW;

                // Draw arrow separator (not after last)
                if (i < kNumSections - 1)
                {
                    g.setColour(get(t3()));
                    juce::String arrow(juce::CharPointer_UTF8(" \xe2\x86\x92 "));
                    float aW = g.getCurrentFont().getStringWidthFloat(arrow);
                    g.drawText(arrow, juce::Rectangle<float>(startX, cy - 8.0f, aW + 2.0f, 16.0f),
                               juce::Justification::centredLeft, false);
                    startX += aW;
                }
            }
        }

        // Macro knobs row removed — EngineDetailPanel has real interactive MacroHeroStrip.

        // D4: Coupling register — XO Gold tint overlay at 10-15% alpha.
        // Painted last so it sits above all other content (subtly warms the whole UI
        // when the coupling inspector tab is active, fading in/out with 400ms transition).
        if (couplingTintAlpha_ > 0.002f)
        {
            // XO Gold: #E9C46A
            g.setColour(juce::Colour(0xFFE9C46A).withAlpha(couplingTintAlpha_));
            g.fillAll();
        }
    }

    void resized() override
    {
        // ── Sync layout state ────────────────────────────────────────────────
        // spec §2.2: PlaySurface is an embedded 264pt bottom zone, not a popup.
        // When visible, ColumnLayoutManager reserves kPlaySurfaceH (264pt) for it.
        layout.playSurfaceVisible = playSurface_.isVisible();
        // cinematicMode is toggled by cinematicToggleBtn (CI button in header).
        // columnCCollapsed is reserved for a future Column C collapse button.
        layout.compute(getWidth(), getHeight());

        // ── Header (52px): Logo | DepthDial | < | > | DNA | A/B | CI | CM | P | DK | KEYS | EXPORT | gear
        // Macros moved to top of Column B (56pt strip). CPU meter + MIDI dot moved to StatusBar area.
        // KEYS button toggles the embedded 264pt PlaySurface zone (spec §2.2)
        auto header = layout.getHeader();

        // Park legacy widgets that are no longer shown in the header.
        // D1: enginesBtn replaced by depthDial — park it off-screen.
        enginesBtn.setBounds(0, -100, 0, 0);
        enginesBtn.setVisible(false);
        presetBrowser.setBounds(0, -200, 0, 0);
        presetBrowser.setVisible(false);

        // ── Left: Logo (painted) + DepthZoneDial + preset nav ──────────────
        // D1: 48×48pt circular dial replaces the ENGINES text button.
        // The dial is a direct child of XOceanusEditor (not part of overview/detail),
        // so it is architecturally isolated from all register transition animations
        // (satisfies Foreseer StaticHeaderComponent constraint).
        header.removeFromLeft(150); // logo rings + "XOceanus" / "XO_OX Designs" text
        {
            auto dialSlice = header.removeFromLeft(56); // 56px slice, dial is 48×48 centered
            depthDial.setVisible(true);
            depthDial.setBounds(dialSlice.withSizeKeepingCentre(48, 48));
        }

        // Preset nav arrows — just after ENGINES button
        presetPrevBtn.setBounds(header.removeFromLeft(28).reduced(2));
        presetNextBtn.setBounds(header.removeFromLeft(28).reduced(2));

        // Header DNA hexagon — 24×24, immediately right of the next-preset button.
        // Provides at-a-glance fingerprint of the active preset without label clutter.
        {
            auto hexSlice = header.removeFromLeft(30); // 30px slice, hex is 24×24 centered
            headerHex_.setBounds(hexSlice.withSizeKeepingCentre(24, 24));
        }

        // ── Right (from edge inward): gear | EXPORT | KEYS | utility strip ──
        // CPU meter and MIDI dot have moved to the StatusBar area (issue #906).
        {
            auto gearSlice = header.removeFromRight(36); // 8px margin included
            settingsBtn.setBounds(gearSlice.withSizeKeepingCentre(28, 28));
        }
        exportBtn.setBounds(header.removeFromRight(56).reduced(4, 10));
        // KEYS button — shows/hides the embedded PlaySurface zone (renamed from PLAY to avoid
        // confusion with the sidebar PLAY/PERFORM performance-controls tab, issue #921)
        surfaceToggleBtn.setButtonText("KEYS");
        surfaceToggleBtn.setBounds(header.removeFromRight(52).withSizeKeepingCentre(48, 26));

        // Utility strip — right side (inward from KEYS)
        themeToggleBtn.setBounds(header.removeFromRight(28).reduced(2));
        // D4: Register lock button — 24px wide, visually paired with theme toggle
        registerLockBtn.setBounds(header.removeFromRight(24).reduced(2));
        perfToggleBtn.setBounds(header.removeFromRight(28).reduced(2));
        cmToggleBtn.setBounds(header.removeFromRight(28).reduced(2));
        cinematicToggleBtn.setBounds(header.removeFromRight(28).reduced(2));

        // ── A/B Compare toggle — small button in header, left side of remaining space ──
        // Restored (issue #918): was parked off-screen, now lives in header centre-right.
        abCompare.setVisible(true);
        abCompare.setBounds(header.removeFromRight(28).reduced(2));

        // ── Column A — Engine Rack (full height) ──
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
            int h = (i == kNumPrimarySlots - 1) ? colA.getBottom() - (colA.getY() + i * tileH) : tileH;
            tiles[i]->setBounds(colA.getX(), colA.getY() + i * tileH, colA.getWidth(), h);
        }

        // Ghost tile — positioned below tile[3] when visible, zero-height when hidden.
        if (ghostTile.isVisible())
        {
            ghostTile.setBounds(colA.getX(), colA.getY() + kNumPrimarySlots * tileH, colA.getWidth(), tileH);
        }
        else
        {
            // Park off-screen (zero height) — no layout impact when hidden.
            ghostTile.setBounds(colA.getX(), colA.getY() + kNumPrimarySlots * tileH, colA.getWidth(), 0);
        }

        // ── Column B — Panel stack + MasterFX strip + FieldMap ───────────────
        // getColumnBPanel() already excludes the FieldMap strip at the bottom.
        auto colBPanel = layout.getColumnBPanel();

        // MasterFX strip at bottom of Column B panel area
        auto masterFXBounds = colBPanel.removeFromBottom(kMasterFXH).reduced(14, 6);
        masterFXStrip.setBounds(masterFXBounds);

        // ── Macro strip — top of Column B (issue #906: macros moved out of header) ──
        // 56pt strip accommodates 44px knobs (enlarged by MacroSection.h) + padding.
        {
            auto macroStrip = colBPanel.removeFromTop(56);
            macros.setBounds(macroStrip.reduced(4, 2));
        }

        // Signal flow strip (28px) at top of Column B — painted in paint().
        signalFlowStripBounds = colBPanel.removeFromTop(kSignalFlowStripH);

        // Pre-compute signal flow hit rects here (not in paint()) so mouse handlers
        // always have valid geometry — even before the first paint() call or after
        // a resize that hasn't triggered a repaint yet.
        {
            static const juce::String kSFSections[] = {"SRC1", "SRC2", "FILTER", "SHAPER", "FX", "OUT"};
            static const int kNumSFSections = 6;

            auto font = GalleryFonts::value(8.5f);
            const float hPad = 12.0f;
            const float usableW = (float)signalFlowStripBounds.getWidth() - hPad * 2.0f;
            const float cy = (float)signalFlowStripBounds.getCentreY();

            // Mirror the centering logic from paint(): total text + arrow widths
            juce::String arrow(juce::CharPointer_UTF8(" \xe2\x86\x92 "));
            float totalTextW = 0.0f;
            for (int i = 0; i < kNumSFSections; ++i)
                totalTextW += font.getStringWidthFloat(kSFSections[i]);
            const float arrowW = font.getStringWidthFloat(arrow);
            const float totalW = totalTextW + arrowW * (kNumSFSections - 1);
            float startX = (float)signalFlowStripBounds.getX() + hPad + (usableW - totalW) * 0.5f;

            for (int i = 0; i < kNumSFSections; ++i)
            {
                float secW = font.getStringWidthFloat(kSFSections[i]);
                sfHitRects[static_cast<size_t>(i)] =
                    juce::Rectangle<float>(startX, cy - 8.0f, secW + 6.0f, 16.0f).expanded(4.0f, 4.0f);
                startX += secW;
                if (i < kNumSFSections - 1)
                    startX += arrowW;
            }
        }

        // Remaining Column B panel area for the view stack (stacked, one visible at a time)
        overview.setBounds(colBPanel);
        detail.setBounds(colBPanel);
        chordPanel.setBounds(colBPanel);
        performancePanel.setBounds(colBPanel);

        // FieldMap hidden — 80px reclaimed for parameter sections
        fieldMap.setBounds(0, -200, 0, 0);
        fieldMap.setVisible(false);

        // ── Column C — Tabbed Sidebar (SidebarPanel) ─────────────────────────
        sidebar.setBounds(layout.getColumnC());

        // ── Embedded PlaySurface — spec §2.2: fixed 264pt bottom zone ────────
        // ColumnLayoutManager reserves kPlaySurfaceH (264pt) when playSurfaceVisible.
        // When collapsed, getPlaySurface() returns a rect parked below the window.
        playSurface_.setBounds(layout.getPlaySurface());

        // ── Status Bar ───────────────────────────────────────────────────────
        statusBar.setBounds(layout.getStatusBar());

        // ── CPU meter + MIDI indicator — right side of StatusBar area (issue #906) ──
        // These are direct children of XOceanusEditor (not StatusBar), positioned
        // inside the status bar row. StatusBar reserves ~104px on the far right for
        // its lock button (28px) and slot dots (66px) + padding — so we sit left of that.
        // The StatusBar's cpuLabel text remains; the CPUMeter graphical pill supplements it.
        {
            auto statusArea = layout.getStatusBar();
            // Leave 104px for StatusBar's internal lock + dots on the far right.
            statusArea.removeFromRight(104);
            // Place MIDI dot then CPU pill, right-to-left from that boundary.
            midiIndicator.setBounds(statusArea.removeFromRight(16).withSizeKeepingCentre(8, 8));
            cpuMeter.setBounds(statusArea.removeFromRight(68).withSizeKeepingCentre(64, 20));
        }

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

        // ── ToastOverlay — always covers the full editor, paints on top ───────
        // Must be sized last (mirrors addAndMakeVisible order in constructor).
        toastOverlay_.setBounds(getLocalBounds());
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
        signalFlowActiveSection = 0; // Reset to SRC1 on engine switch (SHOULD A1-06)
        signalFlowHoveredSection = -1;
        // Persist slot selection for session restore (#357)
        processor.setPersistedSelectedSlot(selectedSlot);
        processor.setPersistedSignalFlowSection(signalFlowActiveSection);
        repaint(signalFlowStripBounds);
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
        juce::Component::SafePointer<XOceanusEditor> safeThis(this);

        if (detail.isVisible())
        {
            // Cross-fade: fade out → swap → fade in
            // #926: skip animation when the OS/in-app reduced-motion preference is active
            if (A11y::prefersReducedMotion())
            {
                detail.setVisible(false);
                if (detail.loadSlot(slot))
                {
                    overview.setVisible(false);
                    couplingHitTester.setVisible(false);
                    detail.setAlpha(1.0f);
                    detail.setVisible(true);
                    if (auto* eng = processor.getEngine(slot))
                        masterFXStrip.setAccentColour(eng->getAccentColour());
                }
                else
                {
                    showOverview();
                }
            }
            else
            {
                anim.fadeOut(&detail, kFadeMs);
                juce::Timer::callAfterDelay(
                    kFadeMs,
                    [safeThis, slot]
                    {
                        if (safeThis == nullptr)
                            return;
                        auto& self = *safeThis;
                        if (slot != self.selectedSlot)
                            return; // CQ17: user clicked elsewhere during fade
                        if (self.detail.loadSlot(slot))
                        {
                            self.overview.setVisible(false);
                            self.couplingHitTester.setVisible(false); // hide — overlaps detail panel bounds
                            self.detail.setAlpha(0.0f);
                            self.detail.setVisible(true);
                            juce::Desktop::getInstance().getAnimator().fadeIn(&self.detail, kFadeMs);
                            if (auto* eng = self.processor.getEngine(slot))
                                self.masterFXStrip.setAccentColour(eng->getAccentColour());
                        }
                        else
                        {
                            self.showOverview();
                        }
                    });
            }
        }
        else
        {
            // Fade out overview, fade in detail
            // #926: skip animation when the OS/in-app reduced-motion preference is active
            if (A11y::prefersReducedMotion())
            {
                overview.setVisible(false);
                couplingHitTester.setVisible(false);
                if (detail.loadSlot(slot))
                {
                    detail.setAlpha(1.0f);
                    detail.setVisible(true);
                    if (auto* eng = processor.getEngine(slot))
                        masterFXStrip.setAccentColour(eng->getAccentColour());
                }
                else
                {
                    overview.setAlpha(1.0f);
                    overview.setVisible(true);
                    couplingHitTester.setVisible(true);
                }
            }
            else
            {
                anim.fadeOut(&overview, kFadeMs);
                juce::Timer::callAfterDelay(
                    kFadeMs,
                    [safeThis, slot]
                    {
                        if (safeThis == nullptr)
                            return;
                        auto& self = *safeThis;
                        if (slot != self.selectedSlot)
                            return; // CQ17: user clicked elsewhere during fade
                        if (self.detail.loadSlot(slot))
                        {
                            self.overview.setVisible(false);
                            self.couplingHitTester.setVisible(false); // hide — overlaps detail panel bounds
                            self.detail.setAlpha(0.0f);
                            self.detail.setVisible(true);
                            juce::Desktop::getInstance().getAnimator().fadeIn(&self.detail, kFadeMs);
                            if (auto* eng = self.processor.getEngine(slot))
                                self.masterFXStrip.setAccentColour(eng->getAccentColour());
                        }
                        else
                        {
                            self.overview.setAlpha(1.0f);
                            self.overview.setVisible(true);
                        }
                    });
            }
        }
    }

    void showOverview()
    {
        selectedSlot = -1;
        processor.setPersistedSelectedSlot(-1); // persist for session restore (#357)
        for (int i = 0; i < kNumPrimarySlots; ++i)
            tiles[i]->setSelected(false);
        ghostTile.setSelected(false);
        cmToggleBtn.setToggleState(false, juce::dontSendNotification);
        perfToggleBtn.setToggleState(false, juce::dontSendNotification);

        auto& anim = juce::Desktop::getInstance().getAnimator();
        juce::Component* outgoing = detail.isVisible()             ? static_cast<juce::Component*>(&detail)
                                    : chordPanel.isVisible()       ? static_cast<juce::Component*>(&chordPanel)
                                    : performancePanel.isVisible() ? static_cast<juce::Component*>(&performancePanel)
                                                                   : nullptr;
        if (outgoing)
        {
            // #926: skip cross-fade when reduced-motion preference is active
            if (A11y::prefersReducedMotion())
            {
                outgoing->setVisible(false);
                overview.setAlpha(1.0f);
                overview.setVisible(true);
                couplingHitTester.setVisible(true);
            }
            else
            {
                juce::Component::SafePointer<XOceanusEditor> safeThis(this);
                juce::Component::SafePointer<juce::Component> safeOutgoing(outgoing);
                anim.fadeOut(outgoing, kFadeMs);
                juce::Timer::callAfterDelay(
                    kFadeMs,
                    [safeThis, safeOutgoing]
                    {
                        if (safeThis == nullptr)
                            return;
                        if (safeOutgoing != nullptr)
                            safeOutgoing->setVisible(false);
                        safeThis->overview.setAlpha(0.0f);
                        safeThis->overview.setVisible(true);
                        safeThis->couplingHitTester.setVisible(true); // restore — overview needs arc hit-testing
                        juce::Desktop::getInstance().getAnimator().fadeIn(&safeThis->overview, kFadeMs);
                    });
            }
        }
    }

    void showChordMachine()
    {
        selectedSlot = -1;
        processor.setPersistedSelectedSlot(-1); // persist for session restore (#357)
        for (int i = 0; i < kNumPrimarySlots; ++i)
            tiles[i]->setSelected(false);
        ghostTile.setSelected(false);

        auto& anim = juce::Desktop::getInstance().getAnimator();
        juce::Component* outgoing = detail.isVisible()             ? static_cast<juce::Component*>(&detail)
                                    : performancePanel.isVisible() ? static_cast<juce::Component*>(&performancePanel)
                                    : overview.isVisible()         ? static_cast<juce::Component*>(&overview)
                                                                   : nullptr;
        if (outgoing)
        {
            // #926: skip cross-fade when reduced-motion preference is active
            if (A11y::prefersReducedMotion())
            {
                outgoing->setVisible(false);
                chordPanel.setAlpha(1.0f);
                chordPanel.setVisible(true);
                chordPanel.grabKeyboardFocus();
            }
            else
            {
                juce::Component::SafePointer<XOceanusEditor> safeThis(this);
                juce::Component::SafePointer<juce::Component> safeOutgoing(outgoing);
                anim.fadeOut(outgoing, kFadeMs);
                juce::Timer::callAfterDelay(kFadeMs,
                                            [safeThis, safeOutgoing]
                                            {
                                                if (safeThis == nullptr)
                                                    return;
                                                if (safeOutgoing != nullptr)
                                                    safeOutgoing->setVisible(false);
                                                safeThis->chordPanel.setAlpha(0.0f);
                                                safeThis->chordPanel.setVisible(true);
                                                juce::Desktop::getInstance().getAnimator().fadeIn(&safeThis->chordPanel,
                                                                                                  kFadeMs);
                                                safeThis->chordPanel.grabKeyboardFocus();
                                            });
            }
        }
        else
        {
            // #926: skip fade-in when reduced-motion preference is active
            if (A11y::prefersReducedMotion())
            {
                chordPanel.setAlpha(1.0f);
                chordPanel.setVisible(true);
                chordPanel.grabKeyboardFocus();
            }
            else
            {
                chordPanel.setAlpha(0.0f);
                chordPanel.setVisible(true);
                anim.fadeIn(&chordPanel, kFadeMs);
                chordPanel.grabKeyboardFocus();
            }
        }
    }

    void showPerformanceView()
    {
        selectedSlot = -1;
        processor.setPersistedSelectedSlot(-1); // persist for session restore (#357)
        for (int i = 0; i < kNumPrimarySlots; ++i)
            tiles[i]->setSelected(false);
        ghostTile.setSelected(false);

        performancePanel.refresh();

        auto& anim = juce::Desktop::getInstance().getAnimator();
        juce::Component* outgoing = detail.isVisible()       ? static_cast<juce::Component*>(&detail)
                                    : chordPanel.isVisible() ? static_cast<juce::Component*>(&chordPanel)
                                    : overview.isVisible()   ? static_cast<juce::Component*>(&overview)
                                                             : nullptr;
        if (outgoing)
        {
            // #926: skip cross-fade when reduced-motion preference is active
            if (A11y::prefersReducedMotion())
            {
                outgoing->setVisible(false);
                performancePanel.setAlpha(1.0f);
                performancePanel.setVisible(true);
                performancePanel.grabKeyboardFocus();
            }
            else
            {
                juce::Component::SafePointer<XOceanusEditor> safeThis(this);
                juce::Component::SafePointer<juce::Component> safeOutgoing(outgoing);
                anim.fadeOut(outgoing, kFadeMs);
                juce::Timer::callAfterDelay(kFadeMs,
                                            [safeThis, safeOutgoing]
                                            {
                                                if (safeThis == nullptr)
                                                    return;
                                                if (safeOutgoing != nullptr)
                                                    safeOutgoing->setVisible(false);
                                                safeThis->performancePanel.setAlpha(0.0f);
                                                safeThis->performancePanel.setVisible(true);
                                                juce::Desktop::getInstance().getAnimator().fadeIn(
                                                    &safeThis->performancePanel, kFadeMs);
                                                safeThis->performancePanel.grabKeyboardFocus();
                                            });
            }
        }
        else
        {
            // #926: skip fade-in when reduced-motion preference is active
            if (A11y::prefersReducedMotion())
            {
                performancePanel.setAlpha(1.0f);
                performancePanel.setVisible(true);
                performancePanel.grabKeyboardFocus();
            }
            else
            {
                performancePanel.setAlpha(0.0f);
                performancePanel.setVisible(true);
                anim.fadeIn(&performancePanel, kFadeMs);
                performancePanel.grabKeyboardFocus();
            }
        }
    }

    // Dark Cockpit B041: read current performance opacity for child panels.
    // Panels call this in their paint() to dim non-essential controls during silence.
    float getCockpitOpacity() const override { return cockpitOpacity_; }

    // ── PlaySurface show/hide (embedded zone, spec §2.2) ──────────────────────
    // Toggling the KEYS button expands/collapses the 264pt bottom zone by resizing
    // the plugin window.  A 200ms ComponentAnimator transition animates the resize.
    // MIDI wiring and mode/bank/octave state persist across hide/show cycles because
    // playSurface_ is a permanent child of XOceanusEditor (not recreated on each open).

    // Persist the playSurface visibility state so V2 first-launch default is honoured
    // on subsequent sessions and explicit user toggles are remembered.
    static void persistPlaySurfaceVisible(bool visible)
    {
        juce::PropertiesFile::Options opts;
        opts.applicationName = "XOceanus";
        opts.filenameSuffix = "settings";
        opts.osxLibrarySubFolder = "Application Support";
        juce::PropertiesFile settings(opts);
        settings.setValue("playSurfaceVisible", visible);
    }

    void showPlaySurface()
    {
        if (playSurface_.isVisible())
            return; // already expanded

        persistPlaySurfaceVisible(true);

        // Make visible first so resized() sees it and computes correct bounds.
        playSurface_.setVisible(true);
        playSurface_.grabKeyboardFocus();

        // P2-4: Set accent immediately — no 1-frame XO Gold flash on first open.
        {
            juce::Colour accent(0xFFE9C46A);
            if (selectedSlot >= 0 && selectedSlot < XOceanusProcessor::MaxSlots)
            {
                if (auto* eng = processor.getEngine(selectedSlot))
                    accent = eng->getAccentColour();
            }
            else
            {
                for (int i = 0; i < XOceanusProcessor::MaxSlots; ++i)
                    if (auto* eng = processor.getEngine(i))
                    {
                        accent = eng->getAccentColour();
                        break;
                    }
            }
            playSurface_.setAccentColour(accent);
        }

        // Expand plugin window by kPlaySurfaceH (264pt) with a 200ms animated resize.
        // Clamp to the resize limit ceiling.
        const int newH = juce::jmin(getHeight() + ColumnLayoutManager::kPlaySurfaceH,
                                    1000 + ColumnLayoutManager::kPlaySurfaceH);
        // #926: skip resize animation when reduced-motion preference is active
        if (A11y::prefersReducedMotion())
        {
            setSize(getWidth(), newH);
        }
        else
        {
            juce::Desktop::getInstance().getAnimator().animateComponent(
                this, getBounds().withHeight(newH), 1.0f, 200, false, 1.0, 0.0);
        }
    }

    void hidePlaySurface()
    {
        if (!playSurface_.isVisible())
            return; // already collapsed

        persistPlaySurfaceVisible(false);

        // Collapse plugin window first (200ms animated resize), then hide the zone.
        const int newH = juce::jmax(getHeight() - ColumnLayoutManager::kPlaySurfaceH, 600);
        juce::Component::SafePointer<XOceanusEditor> safeThis(this);
        // #926: skip resize animation when reduced-motion preference is active
        if (A11y::prefersReducedMotion())
        {
            setSize(getWidth(), newH);
            playSurface_.setVisible(false);
            resized();
        }
        else
        {
            juce::Desktop::getInstance().getAnimator().animateComponent(
                this, getBounds().withHeight(newH), 1.0f, 200, false, 1.0, 0.0);
            juce::Timer::callAfterDelay(200,
                [safeThis]()
                {
                    if (safeThis == nullptr)
                        return;
                    safeThis->playSurface_.setVisible(false);
                    safeThis->resized(); // recompute layout after hide
                });
        }
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
        processor.drainNoteEvents(
            [&](const XOceanusProcessor::NoteMapEvent& ev)
            {
                juce::Colour colour = kXOGold;
                // MaxSlots now includes the Ghost Slot (4) — safe to query all 5.
                if (ev.slot >= 0 && ev.slot < XOceanusProcessor::MaxSlots)
                {
                    if (auto* eng = processor.getEngine(ev.slot))
                        colour = eng->getAccentColour();
                }
                if (fieldMap.isVisible())
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

        // ── Header DNA hexagon — update when preset changes ───────────────────
        // Polls by name to avoid redundant setDNA()/repaint() every tick.
        {
            const auto& currentPreset = processor.getPresetManager().getCurrentPreset();
            if (currentPreset.name != lastHeaderHexPreset_)
            {
                lastHeaderHexPreset_ = currentPreset.name;
                headerHex_.setDNA(currentPreset.dna);

                // Derive accent from the first active engine in the preset,
                // falling back to XO Gold when no engine is loaded.
                juce::Colour hexAccent(0xFFE9C46A); // XO Gold default
                for (int i = 0; i < XOceanusProcessor::MaxSlots; ++i)
                {
                    if (auto* eng = processor.getEngine(i))
                    {
                        hexAccent = eng->getAccentColour();
                        break;
                    }
                }
                headerHex_.setAccentColor(hexAccent);
            }
        }

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
        if (playSurface_.isVisible())
        {
            static const juce::Colour kXOGoldAccent(0xFFE9C46A);
            juce::Colour accent = kXOGoldAccent;
            if (selectedSlot >= 0 && selectedSlot < XOceanusProcessor::MaxSlots)
            {
                if (auto* eng = processor.getEngine(selectedSlot))
                    accent = eng->getAccentColour();
            }
            else
            {
                for (int i = 0; i < XOceanusProcessor::MaxSlots; ++i)
                {
                    if (auto* eng = processor.getEngine(i))
                    {
                        accent = eng->getAccentColour();
                        break;
                    }
                }
            }
            playSurface_.setAccentColour(accent);
        }

        // ── D4: Register Manager update ───────────────────────────────────────
        // Compute elapsed time (ms) since last timer tick for smooth transitions.
        // Uses a fixed-point approximation: timerHz is 1–30, so dt is 33–1000ms.
        // We cap at 100ms to avoid a single large jump after a timer gap.
        {
            const double nowMs    = juce::Time::getMillisecondCounterHiRes();
            const float  dtMs     = juce::jlimit(0.0f, 100.0f,
                                                  static_cast<float>(nowMs - lastTimerMs_));
            lastTimerMs_ = nowMs;

            const float activity        = processor.getNoteActivity();
            const bool  hasNoteActivity = activity > 0.01f;
            const bool  couplingVisible = (sidebar.getActiveTab() == SidebarPanel::Couple);

            registerMgr_.update(hasNoteActivity, couplingVisible, dtMs);

            // ── Dark Cockpit B041: compute UI opacity from register + note activity ──
            // Each register targets a distinct cockpit opacity envelope:
            //   Gallery     : 0.9 (browsing — comfortable, well-lit)
            //   Performance : B041 activity-driven 0.15..1.0
            //   Coupling    : 0.75 (routing inspector — dim enough to read arcs)
            // cockpitBypass_ overrides all registers to 1.0 (fully lit).
            {
                float galleryTarget     = 0.9f;
                float performanceTarget = 0.15f + activity * 0.85f;
                float couplingTarget    = 0.75f;

                // Blend between registers using smoothstep transition progress.
                const float p = registerMgr_.transitionProgress();
                const auto  cur = registerMgr_.current();
                const auto  tgt = registerMgr_.target();

                // Determine src/dst opacity for cross-fade.
                auto registerOpacity = [&](RegisterManager::Register r) -> float
                {
                    switch (r)
                    {
                        case RegisterManager::Register::Gallery:     return galleryTarget;
                        case RegisterManager::Register::Performance: return performanceTarget;
                        case RegisterManager::Register::Coupling:    return couplingTarget;
                        default:                                      return galleryTarget;
                    }
                };

                const float srcOp = registerOpacity(cur);
                const float dstOp = registerOpacity(tgt);
                // When p == 1.0 the transition is settled (cur == tgt), so this is just dstOp.
                const float registerDrivenOpacity = srcOp + (dstOp - srcOp) * p;
                const float targetOpacity = cockpitBypass_ ? 1.0f : registerDrivenOpacity;

                // Lerp toward target: ~60ms smoothing at 60Hz timer, ~120ms at 30Hz.
                const float newOpacity = cockpitOpacity_ + (targetOpacity - cockpitOpacity_) * 0.15f;
                const float clamped    = juce::jlimit(0.0f, 1.0f, newOpacity);

                bool needsRepaint = false;
                if (std::abs(clamped - cockpitOpacity_) > 0.001f)
                {
                    cockpitOpacity_ = clamped;
                    needsRepaint    = true;
                }

                // ── Coupling register: gold tint overlay alpha ─────────────────
                // XO Gold tint at 10-15% alpha when in (or transitioning to) Coupling register.
                const float couplingTintAlpha = [&]() -> float
                {
                    const float tgtAlpha = (tgt == RegisterManager::Register::Coupling) ? 0.12f : 0.0f;
                    const float srcAlpha = (cur == RegisterManager::Register::Coupling) ? 0.12f : 0.0f;
                    return srcAlpha + (tgtAlpha - srcAlpha) * p;
                }();

                if (std::abs(couplingTintAlpha - couplingTintAlpha_) > 0.001f)
                {
                    couplingTintAlpha_ = couplingTintAlpha;
                    needsRepaint       = true;
                }

                if (needsRepaint)
                    repaint();

                statusBar.setCockpitBypass(cockpitBypass_);
            }
        }

        // ── ShaperRegistry deferred deletion drain (#751 fix) ─────────────────
        // Audio thread may have deferred shared_ptr destruction here to avoid
        // running ShaperEngine destructors on the RT thread.  Drain on every
        // timer tick (worst case 1 second delay — well within acceptable range).
        ShaperRegistry::instance().drainDeferredDeletions();
    }

    // kHeaderH and kFieldMapH are now defined in ColumnLayoutManager.
    // Use ColumnLayoutManager::kHeaderH (52) and ColumnLayoutManager::kFieldMapH (80).
    static constexpr int kMasterFXH = 68;        // MasterFX compact strip at bottom of Column B
    static constexpr int kSignalFlowStripH = 28; // P0-12: signal flow breadcrumb strip
    static constexpr int kFadeMs = 150;          // Panel cross-fade duration (ms)
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

        const auto collection = EngineRegistry::detectCollection(ids);

        // Guard (#188): only animate the ghost slot if every engine ID in the
        // matched collection is actually registered in the picker table.
        // detectCollection() may return a match for engine IDs that exist in
        // preset data but are not registered in the current build, causing a
        // spinning/pulsing ghost tile with no engine behind it.
        bool allRegistered = collection.isNotEmpty();
        if (allRegistered)
        {
            const auto& reg = EngineRegistry::instance();
            for (const auto& id : ids)
            {
                if (id.isNotEmpty() && !reg.isRegistered(id.toStdString()))
                {
                    allRegistered = false;
                    break;
                }
            }
        }

        const bool shouldShow = allRegistered;
        auto& anim = juce::Desktop::getInstance().getAnimator();

        if (shouldShow && !ghostTile.isVisible())
        {
            // Materialise: set alpha 0, make visible, animate to alpha 1.
            ghostTile.setAlpha(0.0f);
            ghostTile.setVisible(true);
            resized(); // give the ghost tile its bounds before animating
            anim.animateComponent(&ghostTile, ghostTile.getBounds(), 1.0f, 500, false, 1.0, 0.0);
        }
        else if (!shouldShow && ghostTile.isVisible())
        {
            // Dematerialise: fade out, then hide and recompute layout.
            anim.animateComponent(&ghostTile, ghostTile.getBounds(), 0.0f, 100, false, 1.0, 0.0);
            juce::Component::SafePointer<XOceanusEditor> safeThis(this);
            juce::Timer::callAfterDelay(120,
                                        [safeThis]()
                                        {
                                            if (safeThis == nullptr)
                                                return;
                                            if (safeThis->ghostTile.getAlpha() < 0.05f)
                                                safeThis->ghostTile.setVisible(false);
                                            // resized() is NOT called here — hiding a zero-height off-screen
                                            // tile has no visual impact on the 4 primary tiles.
                                        });
        }
    }

    XOceanusProcessor& processor;
    std::unique_ptr<GalleryLookAndFeel> laf;

    std::array<std::unique_ptr<CompactEngineTile>, kNumPrimarySlots> tiles;
    FieldMapPanel fieldMap;
    OverviewPanel overview;
    EngineDetailPanel detail;
    ChordMachinePanel chordPanel;
    PerformanceViewPanel performancePanel;
    MacroSection macros;
    MasterFXSection masterFXStrip;
    PresetBrowserStrip presetBrowser;
    // Ghost Slot tile — declared after presetBrowser to match constructor MIL order.
    // Hidden until EngineRegistry::detectCollection() returns a non-empty collection.
    CompactEngineTile ghostTile;
    juce::TextButton enginesBtn;
    juce::TextButton cinematicToggleBtn;
    juce::TextButton cmToggleBtn;
    juce::TextButton perfToggleBtn;
    juce::TextButton surfaceToggleBtn;
    juce::TextButton themeToggleBtn;
    // D4: Register lock button — padlock icon, toggles auto-register switching on/off
    juce::TextButton registerLockBtn;
    juce::TextButton exportBtn;
    // P0-3: Slim inline preset nav
    juce::TextButton presetPrevBtn;
    juce::TextButton presetNextBtn;
    // P0-4: Settings gear button
    juce::TextButton settingsBtn;
    // PlaySurface — embedded 264pt bottom zone (spec §2.2).
    // Previously a floating DocumentWindow popup; now a permanent direct child.
    // Visible/hidden by the KEYS button; never recreated so MIDI state persists.
    PlaySurface playSurface_;
    // V1 fix: TooltipWindow activates all setTooltip() calls across the entire UI.
    // JUCE requires exactly one TooltipWindow child per top-level component; without it
    // every setTooltip() call is dead code. 400ms delay matches standard plugin UX.
    juce::TooltipWindow tooltipWindow{this, 400};
    CouplingArcOverlay couplingArcs{processor};
    CouplingArcHitTester couplingHitTester{processor};
    SidebarPanel sidebar;
    StatusBar statusBar;

    // ── Tier 1 Gallery components ─────────────────────────────────────────────
    DepthZoneDial depthDial{processor};
    ABCompare abCompare{processor};
    CPUMeter cpuMeter;
    MIDIActivityIndicator midiIndicator;
    MiniCouplingGraph miniCouplingGraph{processor};

    // Header DNA hexagon — 24×24 mini hex showing current preset's Sonic DNA.
    // Updated in timerCallback() whenever the active preset changes.
    DnaHexagon headerHex_;
    // Cache the last preset name to detect changes without polling every frame.
    juce::String lastHeaderHexPreset_;

    int selectedSlot = -1;

    // Signal flow strip interaction state (MUST A1-01)
    int signalFlowActiveSection = 0;                  // 0=SRC1 … 5=OUT
    int signalFlowHoveredSection = -1;                // -1 = none hovered
    std::array<juce::Rectangle<float>, 6> sfHitRects; // populated in resized(), read in paint() and mouse handlers
    juce::Rectangle<int> signalFlowStripBounds;       // set in resized()

    // Dark Cockpit B041: current UI opacity derived from note activity.
    // 0.15 (ghost, silent) → 1.0 (fully lit, maximum activity).
    float cockpitOpacity_ = 0.2f;
    // When true, cockpit is bypassed — UI held at full opacity regardless of activity.
    // Toggle with 'B' key. Shown as COCKPIT: OFF in status bar paint.
    bool cockpitBypass_ = false;

    // D4: 3-Register Auto-Switching
    RegisterManager registerMgr_;
    // Alpha for the Coupling register XO Gold tint overlay (0.0 = off, 0.12 = full Coupling).
    float couplingTintAlpha_ = 0.0f;
    // Timestamp (ms) for elapsed-time delta computation in timerCallback.
    double lastTimerMs_ = 0.0;

    ColumnLayoutManager layout;

    // ── ToastOverlay — non-blocking notification layer ────────────────────────
    // Declared last so it is destroyed first (child components destroyed in
    // reverse declaration order in C++), ensuring the singleton pointer is
    // cleared before any other members are freed.
    // setBounds: full editor bounds (set in resized()).
    // addAndMakeVisible: called LAST in constructor so it paints above all panels.
    ToastOverlay toastOverlay_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(XOceanusEditor)
};

} // namespace xoceanus
