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
#include "Gallery/MacroSection.h"
#include "Gallery/AdvancedFXPanel.h"
#include "Gallery/MasterFXSection.h"
#include "Gallery/PresetBrowserPanel.h"
#include "Gallery/PresetBrowserStrip.h"
#include "Gallery/ChordMachinePanel.h"
#include "Gallery/PerformanceViewPanel.h"
#include "Gallery/CouplingArcOverlay.h"
#include "Gallery/StatusBar.h"
#include "Gallery/WaveformDisplay.h"
#include "Gallery/EnginePickerPopup.h"
#include "Gallery/CouplingPopover.h"
#include "Gallery/DepthZoneDial.h"
#include "Gallery/ABCompare.h"
#include "Gallery/MiniCouplingGraph.h"
#include "Gallery/CockpitHost.h"
#include "Gallery/DnaHexagon.h"
#include "RegisterManager.h"
#include "ToastOverlay.h"
#include "Ocean/OceanView.h"

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
        : AudioProcessorEditor(proc), processor(proc), detail(proc), chordPanel(proc),
          performancePanel(proc), macros(proc.getAPVTS()), masterFXStrip(proc.getAPVTS()), presetBrowser(proc)
    {
        // #893: Constructor extracted into named helpers to reduce monolithic body.
        initTheme();
        initLegacyComponents(proc);
        initHeaderButtons(proc);
        initPlaySurfaceAndPresets(proc);
        initWiring(proc);
        initOceanView(proc);
        startTimerHz(1); // Reduced from 5Hz — idle polling only as a fallback
    }

    //==========================================================================
    // #893 — Constructor helpers
    // Each helper encapsulates one logical initialisation phase of XOceanusEditor.
    // Called in order from the constructor above.
    //==========================================================================

    /** Phase 1: LAF construction and early dark-mode preference restore. */
    void initTheme()
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
    }

    /** Phase 2: Remaining Gallery components — kept for chord/performance panels and coupling arcs.
        These remain in the component tree but are hidden when OceanView is active. */
    void initLegacyComponents(XOceanusProcessor& proc)
    {
        addAndMakeVisible(fieldMap);
        addAndMakeVisible(detail);
        addAndMakeVisible(chordPanel);
        addAndMakeVisible(performancePanel);
        addAndMakeVisible(macros);
        addAndMakeVisible(masterFXStrip);
        addAndMakeVisible(presetBrowser);

        // Coupling arc overlay: must be added after other components so it paints on top.
        // setInterceptsMouseClicks(false,false) means panels below still receive clicks.
        addAndMakeVisible(couplingArcs);
        addAndMakeVisible(couplingHitTester);

        detail.setVisible(false);
        detail.setAlpha(0.0f);
        chordPanel.setVisible(false);
        chordPanel.setAlpha(0.0f);
        performancePanel.setVisible(false);
        performancePanel.setAlpha(0.0f);
    }

    /** Phase 3: Legacy header buttons (ENGINES, CM, P, CI, KEYS, DK, lock, nav, gear, XPN).
        Hidden when OceanView is active, but kept in the tree for keyboard-shortcut parity. */
    void initHeaderButtons(XOceanusProcessor& proc)
    {
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
            detail.repaint();
            chordPanel.repaint();
            performancePanel.repaint();
            macros.repaint();
            masterFXStrip.repaint();
            presetBrowser.repaint();
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
                // #879: show user-visible toast so the failure is not silent
                ToastOverlay::show("Failed to load preset: " + juce::String(e.what()),
                                   Toast::Level::Warn);
                // #894: flush any in-flight crossfade/FX state so the engine is not
                // left in an inconsistent state after a partial parameter application.
                processor.killDelayTails();
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
                // #879: show user-visible toast so the failure is not silent
                ToastOverlay::show("Failed to load preset: " + juce::String(e.what()),
                                   Toast::Level::Warn);
                // #894: flush any in-flight crossfade/FX state so the engine is not
                // left in an inconsistent state after a partial parameter application.
                processor.killDelayTails();
            }
            repaint();
        };

        // P0-4: Settings gear button — far-right header
        addAndMakeVisible(settingsBtn);
        settingsBtn.setButtonText(juce::String(juce::CharPointer_UTF8("\xe2\x9a\x99")));
        // Settings button is hidden when OceanView is active; disable it so it cannot
        // accidentally fire if visibility is toggled before OceanView settings are wired.
        settingsBtn.setEnabled(false);
        settingsBtn.setTooltip("Settings (coming in Ocean View)");

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
    }

    /** Phase 4: Embedded PlaySurface wiring and async preset library scan (#712).
        Also handles first-launch and empty-library fallback paths (#899). */
    void initPlaySurfaceAndPresets(XOceanusProcessor& proc)
    {
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
    }

    /** Phase 5: StatusBar, MIDI-Learn, DepthZoneDial, ABCompare, and
        engine-change callback wiring. */
    void initWiring(XOceanusProcessor& proc)
    {
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
        addAndMakeVisible(miniCouplingGraph);

        // Header DNA hexagon — initialise with default DNA and XO Gold accent.
        // Accent and DNA are updated in timerCallback() once the first preset loads.
        addAndMakeVisible(headerHex_);
        A11y::setup(headerHex_, "Sonic DNA",
                    "Hexagonal visualization of the active preset's 6D Sonic DNA fingerprint");

        // ── #911: Signal flow breadcrumb accessibility nodes ──────────────────
        // Six transparent child components that expose WCAG-conformant aria-labels
        // for each section of the signal flow breadcrumb strip (SRC1→OUT).
        // setInterceptsMouseClicks(false) on each so the editor's own mouse handlers
        // still receive events.
        {
            static const struct { const char* label; const char* desc; } kSfLabels[] = {
                { "Source 1",  "First oscillator / sample source in the signal chain. Click to scroll to SRC1 parameters." },
                { "Source 2",  "Second oscillator / source layer. Click to scroll to SRC2 parameters." },
                { "Filter",    "Filter stage — cutoff, resonance, envelope amount. Click to scroll to FILTER parameters." },
                { "Shaper",    "Waveshaper / modulation stage. Click to scroll to SHAPER parameters." },
                { "FX",        "Effects chain — reverb, delay, chorus. Click to scroll to FX parameters." },
                { "Output",    "Master output — level, pan, send amount. Click to scroll to OUT parameters." },
            };
            for (int i = 0; i < 6; ++i)
            {
                A11y::setup(sfAccessNodes[i], kSfLabels[i].label, kSfLabels[i].desc);
                addAndMakeVisible(sfAccessNodes[i]);
            }
        }

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
            if (detail.isVisible())
                detail.loadSlot(slot);
        };

        // ── ABCompare wiring ─────────────────────────────────────────────────
        // W04: Fire onPresetLoaded() and also refresh detail panel.
        presetBrowser.onPresetLoaded = [this]()
        {
            abCompare.onPresetLoaded();
            // Refresh detail panel if it is currently visible.
            if (detail.isVisible())
                detail.loadSlot(selectedSlot);
        };

        // CQ02: callback fires from the audio thread — marshal all UI calls to the message thread.
        proc.onEngineChanged = [this](int slot)
        {
            juce::MessageManager::callAsync(
                [this, slot]
                {
                    juce::ignoreUnused(slot);
                    if (performancePanel.isVisible())
                        performancePanel.refresh();
                    checkCollectionUnlock();
                });
        };
    }

    /** Phase 6: OceanView construction, setSize(), state restore, resize limits,
        and toast overlay (must be added last so it paints on top). */
    void initOceanView(XOceanusProcessor& proc)
    {
        // Base plugin height is 700pt (PlaySurface collapsed).
        // V2: On first launch the PlaySurface starts open, so add kPlaySurfaceH immediately.
        const int initialHeight = playSurface_.isVisible()
                                      ? 700 + 264
                                      : 700;
        setSize(1100, initialHeight);


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
            }
        }

        setResizable(true, true);
        // PlaySurface adds 264pt when expanded; max height allows for both states.
        setResizeLimits(960, 600, 1600, 1264);
        setWantsKeyboardFocus(true);
        setTitle("XOceanus Synthesizer");
        setDescription("Multi-engine synthesizer with cross-engine coupling. "
                       "Keys 1-4 select engine slots, Escape returns to overview.");

        // ── Ocean View (primary layout) ──────────────────────────────────────
        // Added BEFORE toastOverlay_ so the overlay stays on top.
        addAndMakeVisible(oceanView_);
        oceanView_.initMacros(proc.getAPVTS());
        oceanView_.initDetailPanel(proc);
        oceanView_.initStatusBar();

        // Wire OceanView callbacks
        oceanView_.onEngineSelected = [this](int slot) { if (slot >= 0) selectSlot(slot); };
        oceanView_.onEngineDiveDeep = [this](int slot) { selectSlot(slot); /* detail panel loaded by OceanView */ };
        oceanView_.onPresetSelected = [this](int idx)
        {
            // Load preset by index from the library snapshot.
            auto& pm = processor.getPresetManager();
            auto lib = pm.getLibrary();
            if (lib && idx >= 0 && idx < static_cast<int>(lib->size()))
            {
                const auto& preset = (*lib)[static_cast<size_t>(idx)];
                processor.getUndoManager().beginNewTransaction("Load preset: " + preset.name);
                try
                {
                    processor.applyPreset(preset);
                    pm.setCurrentPreset(preset);
                }
                catch (const std::exception& e)
                {
                    ToastOverlay::show("Failed to load preset: " + juce::String(e.what()),
                                       Toast::Level::Warn);
                }
            }
        };

        // Wire MIDI + PlaySurface inside OceanView
        {
            auto& ps = oceanView_.getPlaySurface();
            ps.setMidiCollector(&processor.getMidiCollector(), 1);
            ps.setProcessor(&processor);
            if (laf)
                ps.setLookAndFeel(laf.get());
        }

        // #897: On first launch (no persisted state) show the OceanView PlaySurface
        // so users see a playable interface immediately.  On subsequent launches,
        // restore the persisted state ("playSurfaceVisible" key in XOceanus settings).
        // The same key is used by the legacy standalone playSurface_ path so both
        // paths share one persistent preference.
        {
            juce::PropertiesFile::Options psOpts;
            psOpts.applicationName    = "XOceanus";
            psOpts.filenameSuffix     = "settings";
            psOpts.osxLibrarySubFolder = "Application Support";
            juce::PropertiesFile psSettings(psOpts);
            const bool hasSavedState  = psSettings.containsKey("playSurfaceVisible");
            const bool shouldShow     = hasSavedState
                                            ? psSettings.getBoolValue("playSurfaceVisible", false)
                                            : true; // first launch: default open
            if (shouldShow)
                oceanView_.showPlaySurface();
        }

        // Wire OceanView PlaySurface visibility-change callback so state is
        // persisted whenever the user toggles it via the KEYS button or K key.
        oceanView_.onPlaySurfaceVisibilityChanged = [](bool visible)
        {
            persistPlaySurfaceVisible(visible);
        };

        // Wire engine slots into OceanView (initial state)
        for (int i = 0; i < 4; ++i)
        {
            if (auto* eng = processor.getEngine(i))
            {
                auto id     = eng->getEngineId();
                auto accent = eng->getAccentColour();
                oceanView_.setEngine(i, id, accent, EngineOrbit::DepthZone::Sunlit);
            }
        }

        // Hide legacy Gallery layout components (replaced by OceanView)
        fieldMap.setVisible(false);
        detail.setVisible(false);
        chordPanel.setVisible(false);
        performancePanel.setVisible(false);
        macros.setVisible(false);
        masterFXStrip.setVisible(false);
        presetBrowser.setVisible(false);
        couplingArcs.setVisible(false);
        couplingHitTester.setVisible(false);
        statusBar.setVisible(false);
        depthDial.setVisible(false);
        abCompare.setVisible(false);
        miniCouplingGraph.setVisible(false);
        headerHex_.setVisible(false);
        // Hide header buttons
        enginesBtn.setVisible(false);
        cmToggleBtn.setVisible(false);
        perfToggleBtn.setVisible(false);
        cinematicToggleBtn.setVisible(false);
        themeToggleBtn.setVisible(false);
        registerLockBtn.setVisible(false);
        surfaceToggleBtn.setVisible(false);
        exportBtn.setVisible(false);
        settingsBtn.setVisible(false);
        presetPrevBtn.setVisible(false);
        presetNextBtn.setVisible(false);
        // Hide the old standalone PlaySurface — OceanView owns its own PlaySurfaceOverlay.
        playSurface_.setVisible(false);

        // ── ToastOverlay — MUST be the last addAndMakeVisible call ────────────
        // JUCE paints children in insertion order; last child paints on top.
        // setInterceptsMouseClicks(false, false) is set inside ToastOverlay's
        // constructor so it never captures events that should reach panels below.
        addAndMakeVisible(toastOverlay_);
        toastOverlay_.setAlwaysOnTop(true);
        ToastOverlay::setInstance(&toastOverlay_);
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
        // Delegate to OceanView first — it handles P (browser), K (PlaySurface),
        // Escape (exit overlay/state), and 1-5 (zoom-in to slot).
        if (oceanView_.keyPressed(key))
            return true;

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
        // 'M' toggles Cinematic Mode
        if (key == juce::KeyPress('m') || key == juce::KeyPress('M'))
        {
            bool nowCinematic = cinematicToggleBtn.getToggleState();
            cinematicToggleBtn.setToggleState(!nowCinematic, juce::dontSendNotification);
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
        // #891 / S4: Set the active dark mode context for this instance before any
        // paint logic (or child components) query GalleryColors::darkMode(). Without
        // this call, all instances share the last-registered context, so a second
        // plugin window could render in the wrong theme.
        GalleryColors::setActiveDarkModeContext(this);

        // OceanView handles all rendering as a child component.
        // paint() only draws the MIDI Learn overlay badge when active.

        // ── MIDI Learn status badge — appears when listening ──────────────────
        {
            const auto& mlm = processor.getMIDILearnManager();
            if (mlm.isLearning())
            {
                // Pulse the badge alpha at ~2Hz using system time
                double t = juce::Time::getMillisecondCounterHiRes() * 0.002;
                float pulse = 0.65f + 0.35f * (float)std::sin(t * juce::MathConstants<double>::twoPi);

                const int headerH = 52;
                juce::String badge = "MIDI LEARN: move controller to map";
                auto badgeRect = juce::Rectangle<int>(200, 4, getWidth() - 400, headerH - 8);

                g.setColour(juce::Colour(0xFFE9C46A).withAlpha(0.18f * pulse));
                g.fillRoundedRectangle(badgeRect.toFloat(), 4.0f);
                g.setColour(juce::Colour(0xFFE9C46A).withAlpha(pulse));
                g.setFont(GalleryFonts::heading(10.0f)); // (#885: 9pt→10pt legibility floor)
                g.drawText(badge, badgeRect, juce::Justification::centred);
            }
        }

        // D4: Coupling register — XO Gold tint overlay at 10-15% alpha.
        // Painted last so it sits above OceanView content when coupling inspector is active.
        if (couplingTintAlpha_ > 0.002f)
        {
            // XO Gold: #E9C46A
            g.setColour(juce::Colour(0xFFE9C46A).withAlpha(couplingTintAlpha_));
            g.fillAll();
        }

    }

    void resized() override
    {
        oceanView_.setBounds(getLocalBounds());
        toastOverlay_.setBounds(getLocalBounds());
    }

private:
    void selectSlot(int slot)
    {
        if (slot == selectedSlot)
            return;

        selectedSlot = slot;
        signalFlowActiveSection = 0;
        signalFlowHoveredSection = -1;
        processor.setPersistedSelectedSlot(selectedSlot);
        processor.setPersistedSignalFlowSection(signalFlowActiveSection);
        depthDial.setSlot(juce::jlimit(0, 3, slot));
        cmToggleBtn.setToggleState(false, juce::dontSendNotification);
        perfToggleBtn.setToggleState(false, juce::dontSendNotification);
        if (chordPanel.isVisible()) chordPanel.setVisible(false);
        if (performancePanel.isVisible()) performancePanel.setVisible(false);
    }
    void showOverview()
    {
        selectedSlot = -1;
        processor.setPersistedSelectedSlot(-1);
        cmToggleBtn.setToggleState(false, juce::dontSendNotification);
        perfToggleBtn.setToggleState(false, juce::dontSendNotification);
        chordPanel.setVisible(false);
        performancePanel.setVisible(false);
        detail.setVisible(false);
    }
    void showChordMachine()
    {
        selectedSlot = -1;
        processor.setPersistedSelectedSlot(-1);

        auto& anim = juce::Desktop::getInstance().getAnimator();
        juce::Component* outgoing = detail.isVisible()             ? static_cast<juce::Component*>(&detail)
                                    : performancePanel.isVisible() ? static_cast<juce::Component*>(&performancePanel)
                                                                   : nullptr;
        if (outgoing)
        {
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
        processor.setPersistedSelectedSlot(-1);

        performancePanel.refresh();

        auto& anim = juce::Desktop::getInstance().getAnimator();
        juce::Component* outgoing = detail.isVisible()       ? static_cast<juce::Component*>(&detail)
                                    : chordPanel.isVisible() ? static_cast<juce::Component*>(&chordPanel)
                                                             : nullptr;
        if (outgoing)
        {
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
        const int newH = juce::jmin(getHeight() + 264, 1264);
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
        const int newH = juce::jmax(getHeight() - 264, 600);
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
        checkCollectionUnlock();
        if (performancePanel.isVisible())
            performancePanel.refresh();

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
            statusBar.setSlotActive(4, false, juce::Colours::transparentBlack);
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
            // SidebarPanel removed — query OceanView coupling inspector state instead.
            // Currently returns false until CouplingSubstrate interactions are implemented
            // (#979 follow-up adds isCouplingInspectorVisible() to OceanView).
            const bool  couplingVisible = oceanView_.isCouplingInspectorVisible();

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

        // ── Ocean View state updates ──────────────────────────────────────────
        // Engine presence — push live slot state into OceanView.
        for (int i = 0; i < 4; ++i)
        {
            if (auto* eng = processor.getEngine(i))
            {
                auto id     = eng->getEngineId();
                auto accent = eng->getAccentColour();
                oceanView_.setEngine(i, id, accent, EngineOrbit::DepthZone::Sunlit);
                oceanView_.setVoiceCount(i, eng->getActiveVoiceCount());
            }
            else
            {
                oceanView_.clearEngine(i);
            }
        }

        // Preset info — update nexus when preset name changes.
        {
            const auto& pm = processor.getPresetManager();
            oceanView_.setPresetName(pm.getCurrentPreset().name);
            // DNA update — push 6D vector to OceanView nexus.
            const auto& dna = pm.getCurrentPreset().dna;
            oceanView_.setDNA(dna.brightness, dna.warmth, dna.movement,
                              dna.density, dna.space, dna.aggression);
        }

        // ── #909: Live parameter feedback — voice count + macro values ─────────
        // Push total voice count and current macro knob positions to NexusDisplay
        // so the Overview (Orbital) state always shows live activity readouts.
        {
            int totalVoices = 0;
            for (int i = 0; i < XOceanusProcessor::MaxSlots; ++i)
                if (auto* eng = processor.getEngine(i))
                    totalVoices += eng->getActiveVoiceCount();

            // Read macro parameter values from APVTS — normalised [0,1].
            std::array<float, 4> macroVals{};
            static const char* kMacroIds[] = {"macro1", "macro2", "macro3", "macro4"};
            for (int m = 0; m < 4; ++m)
            {
                if (auto* param = processor.getAPVTS().getParameter(kMacroIds[m]))
                    macroVals[static_cast<size_t>(m)] = param->getValue();
            }
            oceanView_.setLiveReadouts(totalVoices, macroVals);
        }

        // Coupling routes — convert MegaCouplingMatrix routes to OceanView CouplingRoute structs.
        {
            const auto& matrixRoutes = processor.getCouplingMatrix().getRoutes();
            std::vector<CouplingRoute> oceanRoutes;
            oceanRoutes.reserve(matrixRoutes.size());
            for (const auto& r : matrixRoutes)
            {
                if (!r.active)
                    continue;
                CouplingRoute cr;
                cr.sourceSlot = r.sourceSlot;
                cr.destSlot   = r.destSlot;
                cr.type       = static_cast<int>(r.type);
                cr.amount     = r.amount;
                oceanRoutes.push_back(cr);
            }
            oceanView_.setCouplingRoutes(oceanRoutes);
        }
    }

    static constexpr int kMasterFXH = 68;        // MasterFX compact strip at bottom of Column B
    static constexpr int kSignalFlowStripH = 28; // P0-12: signal flow breadcrumb strip
    static constexpr int kFadeMs = 150;          // Panel cross-fade duration (ms)
    // kNumPrimarySlots: the 4 slots always visible (indices 0-3).
    // The Ghost Slot (index 4) is conditional — managed by checkCollectionUnlock().
    static constexpr int kNumPrimarySlots = 4;

    // Checks if a collection is unlocked (for future ghost slot use).
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

        juce::ignoreUnused(allRegistered);
    }

    XOceanusProcessor& processor;
    std::unique_ptr<GalleryLookAndFeel> laf;

    FieldMapPanel fieldMap;
    EngineDetailPanel detail;
    ChordMachinePanel chordPanel;
    PerformanceViewPanel performancePanel;
    MacroSection macros;
    MasterFXSection masterFXStrip;
    PresetBrowserStrip presetBrowser;
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
    StatusBar statusBar;

    // ── Tier 1 Gallery components ─────────────────────────────────────────────
    DepthZoneDial depthDial{processor};
    ABCompare abCompare{processor};
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

    // #911: Accessibility overlay for the signal flow breadcrumb.
    // Six transparent child components placed over the sfHitRects so screen readers
    // can announce each section by name.  They are invisible to the painter but live
    // in the JUCE accessibility tree.  Each component has setInterceptsMouseClicks(false)
    // so the parent editor's mouseDown/mouseMove handlers still receive events.
    struct SFBreadcrumbSection : public juce::Component
    {
        SFBreadcrumbSection() { setInterceptsMouseClicks(false, false); }
    };
    std::array<SFBreadcrumbSection, 6> sfAccessNodes;

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


    // ── Ocean View — primary layout (replaces Gallery 3-column model) ─────────
    // Declared before toastOverlay_ so it is destroyed after the toast singleton
    // is cleared.  addAndMakeVisible is called BEFORE toastOverlay_ in the
    // constructor so it renders beneath the overlay.
    OceanView oceanView_;

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
