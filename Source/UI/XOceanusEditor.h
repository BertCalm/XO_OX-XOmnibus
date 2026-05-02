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
#include "Gallery/OverviewPanel.h"
#include "Gallery/CompactEngineTile.h"
#include "Gallery/MacroSection.h"
#include "Gallery/AdvancedFXPanel.h"
#include "Gallery/PresetBrowserPanel.h"
#include "Gallery/PerformanceViewPanel.h"
#include "Gallery/ColumnLayoutManager.h"
#include "Gallery/SidebarPanel.h"
#include "Gallery/StatusBar.h"
#include "Gallery/WaveformDisplay.h"
#include "Gallery/EnginePickerPopup.h"
#include "Gallery/DepthZoneDial.h"
#include "Gallery/ABCompare.h"
#include "Gallery/HeaderIndicators.h"
#include "Gallery/MiniCouplingGraph.h"
#include "Gallery/CockpitHost.h"
#include "Gallery/DnaHexagon.h"
#include "RegisterManager.h"
#include "ToastOverlay.h"
#include "Ocean/OceanLayoutConstants.h"
#include "Ocean/OceanView.h"
// D12: About/Lore modal + "O" brand badge button.
#include "AboutModal.h"
// Wave 5 A1: DragDropModRouter is transitively included via XOceanusProcessor.h
// (which now includes Future/UI/ModRouting/DragDropModRouter.h).
// No explicit re-include needed here — pragma once guards prevent duplication.
// Wave 5 A3: ModMatrixBreakout (strip + slide-up panel).
#include "Future/UI/ModRouting/ModMatrixBreakout.h"
// Wave 6.5 (#1306): cut(1B): Wave65SurfaceWiring removed (PlaySurface removed).
// #include "Ocean/Wave65SurfaceWiring.h"
// Wave 9c (#1303): First-hour onboarding walkthrough overlay.
#include "FirstHourWalkthrough.h"

namespace xoceanus
{

// GalleryColors, GalleryFonts, and A11y are provided by GalleryColors.h
// (included above).  Their definitions are canonical in that file; they are
// available here via the xoceanus:: namespace that GalleryColors.h opens.

//==============================================================================
// XOceanusEditor — Ocean View plugin window. Legacy Gallery components retained for keyboard parity.
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
// Wave 5 A1 note: DragAndDropContainer was initially added here for
// ModSourceHandle drag-drop, but caused a diamond inheritance to juce::Component
// (via both AudioProcessorEditor and DragAndDropContainer), making `this`->Component*
// ambiguous and breaking every addChildComponent/addAndMakeVisible call. The
// container is now on DragDropModRouter (the overlay), which any future source
// handle reaches via findParentDragContainerFor(this).
class XOceanusEditor : public juce::AudioProcessorEditor,
                       public CockpitHost, // B041: Dark Cockpit opacity interface
                       private juce::Timer,
                       private XOceanusProcessor::SlotPresetListener // #1356 per-slot pill sync
{
public:
    explicit XOceanusEditor(XOceanusProcessor& proc)
        : AudioProcessorEditor(proc), processor(proc), overview(proc), detail(proc),
          performancePanel(proc), macros(proc.getAPVTS()),
          ghostTile(proc, 4) // Ghost Slot — 5th tile, slot index 4
    {
        // #893: Constructor extracted into named helpers to reduce monolithic body.
        initTheme();
        initLegacyComponents(proc);
        initHeaderButtons(proc);
        initPlaySurfaceAndPresets(proc);
        initSidebarAndWiring(proc);
        initOceanView(proc);
        startTimerHz(10); // #1008 FIX 6: raised from 1Hz — OceanView voice counts
                          // and coupling routes need 10Hz minimum for responsive feel.
                          // MIDI-learn boost to 30Hz applied separately in MIDI callback.
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

        // Open the shared settings file once — reused for all subsequent reads/writes
        // so message-thread callbacks never perform blocking file I/O inline.
        {
            juce::PropertiesFile::Options opts;
            opts.applicationName      = "XOceanus";
            opts.filenameSuffix       = "settings";
            opts.osxLibrarySubFolder  = "Application Support";
            settingsFile_ = std::make_unique<juce::PropertiesFile>(opts);
        }

        // Read persisted theme preference before any component styling.
        // SettingsPanel will also read this later, but we need it now so
        // setColour() calls use the correct theme from the start.
        {
            const bool savedDark = settingsFile_->getBoolValue("darkMode", true);
            // Register this instance in the per-instance dark mode registry (fix #329).
            // unregisterInstance() is called in the destructor.
            GalleryColors::setInstanceDarkMode(this, savedDark);
        }
    }

    /** Phase 2: Legacy Gallery components — tiles, overview, coupling arcs.
        These remain in the component tree but are hidden when OceanView is active.
        #1007 FIX 6: Use addChildComponent (invisible by default, zero repaint cost)
        instead of addAndMakeVisible + setVisible(false).  Components that were
        previously shown then hidden still triggered repaint() invalidations on every
        parent dirty rect — especially expensive with 19,574 preset dots populating
        the DnaMapBrowser.  addChildComponent() never marks the component as visible
        so JUCE skips the repaint subtree entirely. */
    void initLegacyComponents(XOceanusProcessor& proc)
    {
        // Primary tiles (slots 0-3) — start hidden; made visible by selectSlot()
        for (int i = 0; i < kNumPrimarySlots; ++i)
        {
            tiles[i] = std::make_unique<CompactEngineTile>(proc, i);
            tiles[i]->onSelect = [this](int slot) { selectSlot(slot); };
            addChildComponent(*tiles[i]); // #1007 FIX 6: invisible by default, no repaint
        }

        // Ghost tile (slot 4) — hidden until collection detection fires
        ghostTile.onSelect = [this](int slot) { selectSlot(slot); };
        addChildComponent(ghostTile); // #1007 FIX 6

        addChildComponent(overview);         // #1007 FIX 6
        addChildComponent(detail);           // #1007 FIX 6
        addChildComponent(performancePanel); // #1007 FIX 6
        addChildComponent(macros);           // #1007 FIX 6

        // Preserve alpha=0.0f so the fade-in animation still works when toggled on.
        detail.setAlpha(0.0f);
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
        // Wire TideController default target: TONE macro (macro1). [D11]
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
            performancePanel.repaint();
            macros.repaint();
            sidebar.repaint();
            // Also repaint the embedded PlaySurface if visible.
            if (playSurface_.isVisible())
                playSurface_.repaint();
            // Persist preference so the theme survives plugin reload (#215).
            // Use the shared settingsFile_ member — avoids blocking file open on the message thread.
            if (settingsFile_ != nullptr)
                settingsFile_->setValue("darkMode", newState);
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
        settingsBtn.setTooltip("Settings");
        A11y::setup(settingsBtn, "Settings", "Open settings panel");
        settingsBtn.setColour(juce::TextButton::textColourOffId, juce::Colour(GalleryColors::t3()));
        settingsBtn.setColour(juce::TextButton::textColourOnId, juce::Colour(GalleryColors::t1()));
        settingsBtn.onClick = [this] { sidebar.selectTab(SidebarPanel::Settings); };

        // Export button — launches ExportDialog as a CallOutBox
        addAndMakeVisible(exportBtn);
        exportBtn.setButtonText("XPN");
        GalleryLookAndFeel::setExportButtonStyle(exportBtn);
        exportBtn.setTooltip("Export presets as MPC-compatible XPN expansion pack");
        A11y::setup(exportBtn, "Export", "Open export dialog to build XPN expansion packs");
        GalleryLookAndFeel::setButtonStyle(exportBtn, GalleryLookAndFeel::kBtnStyleExport);
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
        if (presetDir.isDirectory())
        {
            // #923: Also start the spinner in the sidebar's full PresetBrowser.
            sidebar.setPresetBrowserScanning(true);
            // SafePointer guards against the editor being destroyed before the
            // background scan completes (e.g., plugin window closed quickly).
            juce::Component::SafePointer<XOceanusEditor> safeThis(this);
            proc.getPresetManager().scanPresetDirectoryAsync(
                presetDir,
                [safeThis]()
                {
                    if (safeThis == nullptr)
                        return;
                    // #923: Stop the spinner and rebuild the sidebar's full PresetBrowser.
                    safeThis->sidebar.setPresetBrowserScanning(false);
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
        }
    }

    /** Phase 5: Sidebar, StatusBar, MIDI-Learn, DepthZoneDial, ABCompare, and
        engine-change callback wiring.  setSize() is deferred to Phase 6 so the
        sidebar has valid bounds when PresetManager is attached. */
    void initSidebarAndWiring(XOceanusProcessor& proc)
    {
        // ── Column C Sidebar ──────────────────────────────────────────────────
        addAndMakeVisible(sidebar);

        // #913: Wire the double-click expand callback so the collapsed 48pt icon
        // strip can be restored by the user without hunting for a resize handle.
        sidebar.onRequestExpand = [this]
        {
            layout.columnCCollapsed = false;
            resized();
        };

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
        }

        // ── MIDI Learn learn-complete notification ──────────────────────────
        // When a mapping is completed (audio thread captured a CC), bump the
        // timer to 30Hz so the pulse animation runs smoothly.  Reverts to 1Hz
        // after the first poll cycle that finds nothing pending.
        // CQ16: setLearnCompleteCallback fires from a non-message thread — marshal to message thread.
        // Use SafePointer in the inner lambda: if the editor is destroyed between callback
        // registration and async dispatch, the raw `this` would be dangling.
        proc.getMIDILearnManager().setLearnCompleteCallback(
            [this](const juce::String&, int) {
                juce::Component::SafePointer<XOceanusEditor> safeThis(this);
                juce::MessageManager::callAsync([safeThis] {
                    if (safeThis != nullptr)
                        safeThis->startTimerHz(30);
                });
            });

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
                    // Wave 5 A3: feed the new engine's param prefix to the mod matrix strip
                    // so the per-engine APVTS slot drawer loads the correct params.
                    if (modMatrixStrip_ != nullptr && slot == selectedSlot)
                    {
                        if (auto* eng = processor.getEngine(slot))
                            modMatrixStrip_->loadEngine(
                                GalleryColors::prefixForEngine(eng->getEngineId()));
                    }
                });
        };
    }

    /** Phase 6: OceanView construction, setSize(), state restore, resize limits,
        and toast overlay (must be added last so it paints on top). */
    void initOceanView(XOceanusProcessor& proc)
    {
        // 1D-P3: heights derived from layout constants — single source of truth in
        // OceanLayoutConstants.h. Adding a strip to the dashboard budget no longer
        // requires manually bumping the magic 764/864 numbers.
        constexpr int kDefaultBaseHeight =
            ocean_layout::kDashboardH + ocean_layout::kStatusBarH +
            ocean_layout::kWaterlineH + ocean_layout::kDefaultOceanViewportH;  // = 864
        constexpr int kMinBaseHeight =
            ocean_layout::kDashboardH + ocean_layout::kStatusBarH +
            ocean_layout::kWaterlineH + ocean_layout::kMinOceanViewportH;      // = 764

        // V2: On first launch the PlaySurface starts open, so add kPlaySurfaceH immediately.
        const int initialHeight = playSurface_.isVisible()
                                      ? kDefaultBaseHeight + ColumnLayoutManager::kPlaySurfaceH
                                      : kDefaultBaseHeight;

        // Resize limits must be declared BEFORE setSize() — JUCE's constrainer
        // only runs on user-initiated drags, not programmatic setSize(), so any
        // host (e.g. Logic Pro) that re-sizes the editor immediately after
        // construction could otherwise land outside the declared bounds.
        setResizable(true, true);
        // PlaySurface adds 264pt when expanded; max height allows for both states.
        setResizeLimits(960, kMinBaseHeight, 1600, 1200 + ColumnLayoutManager::kPlaySurfaceH);

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
            // Wire Restart Walkthrough — walkthrough_ lives here in the editor so
            // we dispatch via callback rather than passing a pointer into SettingsPanel.
            sp->onRestartWalkthrough = [this]
            {
                jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());
                walkthrough_.restartWalkthrough(settingsFile_.get());
            };
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
                }
            }
        }

        setWantsKeyboardFocus(true);
        setTitle("XOceanus Synthesizer");
        setDescription("Multi-engine synthesizer with cross-engine coupling. "
                       "Keys 1-4 select engine slots, Escape returns to overview.");

        // ── Ocean View (primary layout) ──────────────────────────────────────
        // Added BEFORE toastOverlay_ so the overlay stays on top.
        addAndMakeVisible(oceanView_);
        oceanView_.initMacros(proc.getAPVTS());
        oceanView_.initDetailPanel(proc);
        oceanView_.initSidebar();
        oceanView_.initWaterline(proc.getAPVTS(), proc.getMasterFXChain().getSequencer());
        // #1309 wiring: register TideWaterline persistence callbacks so per-step data
        // survives DAW session recall.  waterline_ is guaranteed non-null after initWaterline().
        if (auto* wl = oceanView_.getWaterline())
        {
            proc.onGetTideWaterlineState = [wl]() { return wl->toValueTree(); };
            proc.onSetTideWaterlineState = [wl](const juce::ValueTree& t) { wl->fromValueTree(t); };
            // Consume any state that arrived while the editor was closed.
            auto deferred = proc.getPersistedTideWaterlineState();
            if (deferred.isValid())
            {
                wl->fromValueTree(deferred);
                proc.clearPersistedTideWaterlineState();
            }
        }
        oceanView_.initChordBar(proc.getAPVTS(), proc.getChordMachine());
        // Wave 5 B3 mount: chord breakout panel (must follow initChordBar).
        oceanView_.initChordBreakout(proc.getAPVTS(), proc.getChordMachine());
        // Wave 5 C2 mount: seq strip + breakout (needs APVTS; slot0_seq_ params from C1).
        oceanView_.initSeqStrip(proc.getAPVTS());
        oceanView_.initMasterFxStrip(proc.getAPVTS());
        oceanView_.initEpicSlotsPanel(proc.getAPVTS());
        oceanView_.initTransportBar();
        // Wire transport bar callbacks.
        if (auto* tb = oceanView_.getTransportBar())
        {
            tb->onPlayToggled = [&proc, this]()
            {
                // Toggle sequencer enabled via APVTS.
                if (auto* p = proc.getAPVTS().getParameter("master_seqEnabled"))
                {
                    bool nowOn = p->getValue() < 0.5f;
                    p->beginChangeGesture();
                    p->setValueNotifyingHost(nowOn ? 1.0f : 0.0f);
                    p->endChangeGesture();
                    if (auto* bar = oceanView_.getTransportBar())
                        bar->setPlaying(nowOn);
                    // Also expand the waterline when starting.
                    if (nowOn)
                        if (auto* wl = oceanView_.getWaterline())
                            wl->setExpanded(true);
                }
            };
            tb->onBpmChanged = [&proc](double newBpm)
            {
                // Write BPM to chord machine's sequencer (cm_seq_bpm).
                if (auto* p = proc.getAPVTS().getParameter("cm_seq_bpm"))
                {
                    p->beginChangeGesture();
                    p->setValueNotifyingHost(p->convertTo0to1(static_cast<float>(newBpm)));
                    p->endChangeGesture();
                }
            };
            // #1097: Coupling tab removed (CPL-tab-removed). Coupling configuration
            // lives in CouplingConfigPopup triggered by knot double-click on OceanView.
            tb->onCoupleClicked = nullptr;
        }
        oceanView_.initStatusBar();

        // Ocean View is the primary layout — hide ALL legacy Gallery-mode panels
        // so they don't ghost behind the Ocean View.
        detail.setVisible(false);
        overview.setVisible(false);
        removeChildComponent(&detail);
        removeChildComponent(&overview);

        // wire(#orphan-sweep items 5/6/7): SettingsDrawer, TransportBar, ChordBar callbacks.
        // onSettingChanged routes keys to processor/APVTS where receivers exist;
        // waveSensitivity is handled locally inside OceanView (background reactivity).
        // #1359: All 7 settings keys are now wired to processor/APVTS receivers.
        // "tempo num/den" is handled separately by onTimeSigChanged below — do
        // not duplicate that routing here.
        // "waveSensitivity" is handled entirely inside OceanView → OceanBackground
        // and never reaches this callback.
        oceanView_.onSettingChanged = [this](const juce::String& key, float value)
        {
            if (key == "polyphony")
            {
                processor.setPolyphony(static_cast<int>(value));
            }
            else if (key == "voiceMode")
            {
                processor.setVoiceMode(static_cast<int>(value));
            }
            else if (key == "masterTune")
            {
                processor.setMasterTune(value);
            }
            else if (key == "pitchBendRange")
            {
                processor.setPitchBendRange(static_cast<int>(value));
            }
            else if (key == "mpeMode")
            {
                processor.setMpeEnabled(value >= 0.5f);
            }
            else if (key == "midiChannel")
            {
                processor.setMidiChannel(static_cast<int>(value));
            }
            else if (key == "oversampling")
            {
                processor.setOversamplingFactor(static_cast<int>(value));
            }
            else if (key == "uiScale")
            {
                // Fix #1432: uiScale combo fires index 0-3 (75% / 100% / 125% / 150%).
                // Scale the editor width proportionally while keeping the current height.
                // Base width is 1100 pt (the initial setSize() value from initOceanView).
                static constexpr int kBaseW = 1100;
                static constexpr float kScales[] = { 0.75f, 1.0f, 1.25f, 1.5f };
                const int idx = juce::jlimit(0, 3, static_cast<int>(value));
                const float scale = kScales[idx];
                const int newW = juce::roundToInt(kBaseW * scale);
                // Clamp to declared resize limits so the host's constrainer is respected.
                const int clampedW = juce::jlimit(960, 1600, newW);
                setSize(clampedW, getHeight());
            }
            // "waveSensitivity" is handled inside OceanView → OceanBackground; no-op here.

            // Fix #1419: persist every setting change so values survive plugin reload.
            if (settingsFile_)
                oceanView_.settingsDrawer().saveSettings(*settingsFile_);
        };

        // Fix #1419: restore persisted settings on startup.  Callbacks are now wired
        // (onSettingChanged assigned above), so applySettings() will synchronize the
        // processor/APVTS for each restored value.
        if (settingsFile_)
            oceanView_.settingsDrawer().applySettings(*settingsFile_);

        // onTimeSigChanged: propagate numerator/denominator to both SharedTransport
        // (read by all tempo-synced engines) and ChordMachine (sequencer step grid).
        oceanView_.onTimeSigChanged = [this](int num, int den)
        {
            processor.getHostTransport().setTimeSignature(num, den);
            processor.getChordMachine().setTimeSignature(num, den);
        };

        // onChordBarVisibilityChanged: OceanView already calls resized() internally.
        // No additional action needed from the editor at this time.
        oceanView_.onChordBarVisibilityChanged = []() {};

        // onChordBarInputModeChanged: ChordBarComponent already writes to APVTS
        // "chord_input_mode" before firing this callback (see ChordBarComponent.h:1095).
        // No additional action needed here — the callback is wired so it is not null.
        oceanView_.onChordBarInputModeChanged = [](ChordBarComponent::InputMode) {};

        // Wire OceanView callbacks
        oceanView_.onUndoRequested = [this]() { processor.getUndoManager().undo(); };
        oceanView_.onRedoRequested = [this]() { processor.getUndoManager().redo(); };
        oceanView_.onEngineSelected = [this](int slot) { if (slot >= 0) selectSlot(slot); };
        oceanView_.onEngineDiveDeep = [this](int slot) { selectSlot(slot); };
        // F2-006: Persist OceanView ViewState + slot so DAW session recall restores navigation.
        oceanView_.onViewStateChanged = [this](int stateInt, int slot)
        {
            processor.setPersistedOceanViewState(stateInt);
            processor.setPersistedOceanViewSlot(slot);
        };
        // F2-012: Pull per-slot waveform data inside OceanView's 30Hz timer so wreath
        // animation and data updates are synchronised.  Moved from editor's 10Hz timer.
        oceanView_.onPullWaveformData = [this]()
        {
            for (int i = 0; i < 4; ++i)
            {
                if (processor.getEngine(i) != nullptr)
                {
                    std::array<float, 128> wreathSamples{};
                    processor.getWaveformFifo(i).readLatest(wreathSamples.data(), 128);
                    float slotRms = 0.0f;
                    for (int s = 64; s < 128; ++s)
                        slotRms += wreathSamples[static_cast<size_t>(s)] * wreathSamples[static_cast<size_t>(s)];
                    slotRms = std::sqrt(slotRms / 64.0f);
                    oceanView_.pushSlotWaveData(i, wreathSamples.data(), 128, slotRms);
                }
            }
        };
        oceanView_.onEngineSelectedFromDrawer = [this](const juce::String& engineId)
        {
            // If a slot is explicitly selected, always replace it directly.
            // Only search for an empty slot when nothing is selected.
            int slot = (selectedSlot >= 0 && selectedSlot < kNumPrimarySlots) ? selectedSlot : -1;
            if (slot < 0)
            {
                // No slot selected — find first empty, or use slot 0
                slot = 0;
                for (int i = 0; i < kNumPrimarySlots; ++i)
                {
                    if (processor.getEngine(i) == nullptr)
                    {
                        slot = i;
                        break;
                    }
                }
            }
            processor.loadEngine(slot, engineId.toStdString());
        };
        oceanView_.onCouplingRouteRequested = [this](int srcSlot, int dstSlot)
        {
            MegaCouplingMatrix::CouplingRoute route;
            route.sourceSlot  = srcSlot;
            route.destSlot    = dstSlot;
            route.type        = CouplingType::AmpToFilter;
            route.amount      = 0.5f;
            route.isNormalled = false;
            route.active      = true;
            processor.getCouplingMatrix().addRoute(route);
        };
        oceanView_.onCouplingConfigChanged = [this](int routeIndex, int newType, float newDepth, int /*direction*/)
        {
            processor.getCouplingMatrix().updateRoute(routeIndex,
                                                      static_cast<CouplingType>(newType),
                                                      newDepth);
        };

        oceanView_.onCouplingDeleteRequested = [this](int routeIndex)
        {
            auto& matrix = processor.getCouplingMatrix();
            auto routes = matrix.getRoutes();
            if (routeIndex < 0 || routeIndex >= static_cast<int>(routes.size()))
                return;
            const auto& r = routes[static_cast<size_t>(routeIndex)];
            if (r.isNormalled)
            {
                ToastOverlay::show("Cannot remove default coupling route", Toast::Level::Info);
                return;
            }
            matrix.removeUserRoute(r.sourceSlot, r.destSlot, r.type);
        };

        // wire(1C-2): Engine context-menu callbacks — 5 actions from right-click buoy menu.
        // OceanView tracks the toggle states internally (slotMuted_[], slotSoloed_[]).
        // The editor reads those states back and propagates to the processor.

        // Mute: sync OceanView's toggle state to the processor's slotMuted[] atomic.
        oceanView_.onEngineMuteToggled = [this](int slot)
        {
            if (slot < 0 || slot >= kNumPrimarySlots) return;
            processor.setSlotMuted(slot, oceanView_.isSlotMuted(slot));
        };

        // Solo: no dedicated processor solo API exists — implement as "mute all others".
        // When slot X becomes soloed: mute all non-soloed slots.
        // When all solos are cleared: restore mute states from OceanView's slotMuted_.
        oceanView_.onEngineSoloToggled = [this](int slot)
        {
            if (slot < 0 || slot >= kNumPrimarySlots) return;
            // Check if any slot is still soloed after this toggle.
            bool anySoloed = false;
            for (int i = 0; i < kNumPrimarySlots; ++i)
                if (oceanView_.isSlotSoloed(i)) { anySoloed = true; break; }
            if (anySoloed)
            {
                // Mute any slot that is not soloed; un-mute soloed slots.
                for (int i = 0; i < kNumPrimarySlots; ++i)
                    processor.setSlotMuted(i, !oceanView_.isSlotSoloed(i));
            }
            else
            {
                // All solos cleared — restore explicit per-slot mute states.
                for (int i = 0; i < kNumPrimarySlots; ++i)
                    processor.setSlotMuted(i, oceanView_.isSlotMuted(i));
            }
        };

        // Remove: unload the engine from the slot.  OceanView clears its buoy visually;
        // the processor releases the shared_ptr and stops rendering that slot.
        oceanView_.onEngineRemoveRequested = [this](int slot)
        {
            if (slot < 0 || slot >= kNumPrimarySlots) return;
            processor.unloadEngine(slot);
        };

        // Chain mode: OceanView handles all visual chain-mode state internally.
        // No processor-side action is needed for chain-mode entry; the route is
        // created when the user completes the knot via onCouplingRouteRequested.
        oceanView_.onChainModeRequested = [](int /*slot*/)
        {
            // Visual chain-mode is fully managed by OceanView (toggleChainMode /
            // setChainInProgress).  No processor call needed at this stage.
        };

        // Picker: the engine drawer opened for slot selection.  This fires AFTER
        // the user has already committed a selection (from engineDrawer_.onEngineSelected)
        // which triggers onEngineSelectedFromDrawer above.  No additional action needed.
        oceanView_.onEnginePickerRequested = []()
        {
            // Notification-only: selection is handled by onEngineSelectedFromDrawer.
        };

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

        // fix(#1354): Wire the 6 previously-unwired HUD bar callbacks.
        //
        // onPresetPrev / onPresetNext — same pattern as the legacy header prev/next buttons.
        oceanView_.onPresetPrev = [this]()
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
                ToastOverlay::show("Failed to load preset: " + juce::String(e.what()),
                                   Toast::Level::Warn);
                processor.killDelayTails();
            }
        };

        oceanView_.onPresetNext = [this]()
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
                ToastOverlay::show("Failed to load preset: " + juce::String(e.what()),
                                   Toast::Level::Warn);
                processor.killDelayTails();
            }
        };

        // onSavePreset — prompt for a name via juce::AlertWindow, then write the
        // .xometa file via PresetManager::savePresetToFile().
        // TODO(#1354): A richer "Save As" dialog (overwrite-check, mood selector)
        // is a follow-up task.  For now, a modal input box is sufficient.
        oceanView_.onSavePreset = [this]()
        {
            auto& pm = processor.getPresetManager();
            const juce::String currentName = pm.getCurrentPreset().name;
            const juce::String suggestion  = currentName.isEmpty() ? "My Preset" : currentName;

            // takeOwnership=true: JUCE manages lifetime — do NOT delete dialog inside callback.
            auto* dialog = new juce::AlertWindow(
                "Save Preset",
                "Enter a name for this preset:",
                juce::MessageBoxIconType::NoIcon,
                this);
            dialog->addTextEditor("name", suggestion, "Preset name:");
            dialog->addButton("Save",   1);
            dialog->addButton("Cancel", 0);

            dialog->enterModalState(
                true,
                juce::ModalCallbackFunction::create(
                    [safeThis = juce::Component::SafePointer<XOceanusEditor>(this), dialog](int result)
                    {
                        if (result != 1 || safeThis == nullptr)
                            return;

                        const juce::String newName = dialog->getTextEditorContents("name").trim();
                        if (newName.isEmpty())
                            return;

                        auto& pm2  = safeThis->processor.getPresetManager();
                        auto  data = pm2.getCurrentPreset();
                        data.name  = newName;

                        const auto presetDir =
                            juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                                .getChildFile("Application Support/XO_OX/XOceanus/Presets");
                        presetDir.createDirectory();
                        const auto file = presetDir.getChildFile(data.name + ".xometa");

                        if (pm2.savePresetToFile(file, data))
                        {
                            pm2.setCurrentPreset(data);
                            ToastOverlay::show("Preset saved: " + data.name, Toast::Level::Info);
                            // Rescan so the new preset appears in the browser immediately.
                            if (auto* sb = safeThis->oceanView_.getSidebar())
                                sb->refreshPresetBrowser();
                        }
                        else
                        {
                            ToastOverlay::show(
                                "Failed to save preset — check disk space or permissions.",
                                Toast::Level::Warn);
                        }
                    }),
                true /* deleteWhenDismissed */);
        };

        // onFavToggled — toggle favourite status on the current preset and persist.
        // Routes to PresetBrowser::toggleFavorite() which handles settings-file I/O.
        oceanView_.onFavToggled = [this](bool /*newFavState*/)
        {
            auto& pm = processor.getPresetManager();
            const auto& current = pm.getCurrentPreset();
            if (current.name.isEmpty())
                return;

            // Reach the PresetBrowser via the OceanView sidebar (always present).
            if (auto* sb = oceanView_.getSidebar())
                if (auto* pb = sb->getPresetBrowser())
                    pb->toggleFavorite(current);
        };

        // onABCompareToggled — delegate to the (hidden) ABCompare Gallery component.
        // When active=true: enter A/B mode (captures A snapshot).
        // When active=false: deactivate (clears A/B slots, no state restore).
        oceanView_.onABCompareToggled = [this](bool active)
        {
            abCompare.setABActive(active);
        };

        // F3-006: Persist REACT dial level so it survives DAW session reload.
        oceanView_.onReactLevelChanged = [this](float value01)
        {
            processor.setPersistedReactLevel(value01);
        };

        // F3-011/F3-017: Persist breakout panel open states.
        oceanView_.onSeqBreakoutToggled = [this](bool isOpen)
        {
            processor.setPersistedSeqBreakoutOpen(isOpen);
        };
        oceanView_.onChordBreakoutToggled = [this](bool isOpen)
        {
            processor.setPersistedChordBreakoutOpen(isOpen);
        };

        // onExportClicked — open the ExportDialog in a CallOutBox anchored to the
        // HUD bar (same as the legacy exportBtn in the Gallery header).
        oceanView_.onExportClicked = [this]()
        {
            juce::CallOutBox::launchAsynchronously(
                std::make_unique<ExportDialog>(processor.getPresetManager(),
                                               &processor.getAPVTS(),
                                               &processor.getCouplingMatrix()),
                oceanView_.getScreenBounds(), getTopLevelComponent());
        };

        // onPresetNameClicked — clicking the preset name label opens the Preset tab
        // in the OceanView sidebar so the user can browse and select a preset.
        oceanView_.onPresetNameClicked = [this]()
        {
            if (auto* sb = oceanView_.getSidebar())
                sb->selectTab(SidebarPanel::Preset);
        };

        // Wire SubmarinePlaySurface MIDI callbacks to the processor's MidiMessageCollector.
        // Without this wiring the keyboard/pad/drum surface was completely silent — clicks
        // fired the onNoteOn/onNoteOff lambdas but they were null (never assigned).
        {
            auto& subPS = oceanView_.getSubmarinePlaySurface();
            subPS.onNoteOn = [this](int note, float velocity)
            {
                auto msg = juce::MidiMessage::noteOn(1, note, velocity);
                msg.setTimeStamp(juce::Time::getMillisecondCounterHiRes() * 0.001);
                processor.getMidiCollector().addMessageToQueue(msg);
            };
            subPS.onNoteOff = [this](int note)
            {
                auto msg = juce::MidiMessage::noteOff(1, note);
                msg.setTimeStamp(juce::Time::getMillisecondCounterHiRes() * 0.001);
                processor.getMidiCollector().addMessageToQueue(msg);
            };
            subPS.onAftertouch = [this](int note, float pressure)
            {
                auto msg = juce::MidiMessage::aftertouchChange(1, note,
                               juce::jlimit(0, 127, juce::roundToInt(pressure * 127.0f)));
                msg.setTimeStamp(juce::Time::getMillisecondCounterHiRes() * 0.001);
                processor.getMidiCollector().addMessageToQueue(msg);
            };

            // P1-7 (1F): SubmarinePlaySurface XY position is persisted via
            // SurfaceRightPanel's per-slot path (D4.c, slot-suffixed APVTS params:
            // xy_pos_x_slot0 … xy_pos_x_slot3).  The unslotted "xy_pos_x" / "xy_pos_y"
            // IDs that the old wire(1C-5) body tried to look up are NOT registered in
            // the APVTS — those lookups returned nullptr and the lambda was silently
            // dead.  Leave the callback assigned to an empty lambda so the std::function
            // null-check in SubmarinePlaySurface.h is always satisfied.
            subPS.onXYChanged = [](float, float) {};
        }

        // F03 fix (#1322): Wire SurfaceRightPanel note callbacks to MidiCollector.
        // Without this, clicks on PAD/DRUM pads fired onNoteOn/onNoteOff but the
        // lambdas were null — the pad grid was completely silent.
        // Wiring pattern mirrors SubmarinePlaySurface (lines above).
        {
            auto& srp = oceanView_.getSurfaceRight();
            srp.onNoteOn = [this](int note, float velocity)
            {
                auto msg = juce::MidiMessage::noteOn(1, note, velocity);
                msg.setTimeStamp(juce::Time::getMillisecondCounterHiRes() * 0.001);
                processor.getMidiCollector().addMessageToQueue(msg);
            };
            srp.onNoteOff = [this](int note)
            {
                auto msg = juce::MidiMessage::noteOff(1, note);
                msg.setTimeStamp(juce::Time::getMillisecondCounterHiRes() * 0.001);
                processor.getMidiCollector().addMessageToQueue(msg);
            };

            // D4 (1D-P2B): XY pad grid visibility toggle — sync from APVTS on init
            // and wire the toggle callback back to persist the state.
            if (auto* gridParam = processor.getAPVTS().getRawParameterValue("xy_pad_grid_visible"))
                srp.setGridVisible(gridParam->load() >= 0.5f);
            // P1-9 (1F): Wrap setValueNotifyingHost in gesture pair so Logic Pro
            // and other DAWs can record automation for this toggle.
            srp.onGridToggled = [this](bool gridOn) {
                if (auto* p = processor.getAPVTS().getParameter("xy_pad_grid_visible"))
                {
                    p->beginChangeGesture();
                    p->setValueNotifyingHost(gridOn ? 1.0f : 0.0f);
                    p->endChangeGesture();
                }
            };
        }

        // wire(1C-1): ExpressionStrips pitch bend and mod wheel → MidiCollector.
        // onPitchBend fires -1..+1; onModWheel fires 0..+1.  Both are forwarded
        // outward via OceanView so the editor can insert them into the MIDI stream.
        oceanView_.onExpressionPitchBend = [this](float v)
        {
            const int wheelVal = static_cast<int>(std::round(8192.0f + v * 8191.0f));
            auto msg = juce::MidiMessage::pitchWheel(1, juce::jlimit(0, 16383, wheelVal));
            msg.setTimeStamp(juce::Time::getMillisecondCounterHiRes() * 0.001);
            processor.getMidiCollector().addMessageToQueue(msg);
        };
        oceanView_.onExpressionModWheel = [this](float v)
        {
            const int cc1 = juce::jlimit(0, 127, static_cast<int>(std::round(v * 127.0f)));
            auto msg = juce::MidiMessage::controllerEvent(1, 1, cc1);
            msg.setTimeStamp(juce::Time::getMillisecondCounterHiRes() * 0.001);
            processor.getMidiCollector().addMessageToQueue(msg);
        };

        // wire(#orphan-sweep item 1): SurfaceRightPanel::onXYChanged was declared but
        // never assigned.  Wire it so XY pad gestures route to APVTS parameters.
        //
        // The XY surface maps two axes to assignable canonical params:
        //   xy_assignX_slot{n} / xy_assignY_slot{n} — choice index into canonical list.
        //   Canonical list: None(0), FilterCutoff(1), FilterRes(2), LFORate(3),
        //     LFODepth(4), EnvAttack(5), EnvRelease(6), Drive(7),
        //     Macro1(8), Macro2(9), Macro3(10), Macro4(11),
        //     FX1WetDry(12), FX2WetDry(13), FX3WetDry(14).
        //
        // resolveXYParamId translates (slotIdx, canonIdx) → APVTS param ID string.
        // The active slot is read from OceanView::getSelectedSlot().
        oceanView_.getSurfaceRight().onXYChanged = [this](float x, float y)
        {
            auto& apvts = processor.getAPVTS();
            const int slot = juce::jlimit(0, 3, oceanView_.getSelectedSlot());
            const juce::String sfx = "_slot" + juce::String(slot);

            // Resolve canonical index for each axis.
            auto getCanonIdx = [&](const juce::String& paramId) -> int
            {
                auto* p = apvts.getParameter(paramId);
                if (!p) return 0;
                return static_cast<int>(p->getValue() * 14.f + 0.5f);
            };

            // Canonical index → APVTS param ID lookup (#1357).
            // Matches the table in XYSurface.h and XOceanusProcessor.cpp (#1331).
            //
            // Per-engine params (cases 1–7) are resolved by probing the APVTS with
            // the active engine's frozen prefix (from getEngineParamPrefix()) and a
            // list of known suffix variants. The first candidate that resolves wins.
            // This handles fleet-wide naming variation (e.g. "filterCutoff" vs
            // "fltCutoff") without requiring per-engine special-casing.
            //
            // FX wet/dry (cases 12–14) use EpicChainSlotController's "slot{n}_mix"
            // APVTS params (registered by EpicChainSlotController::addParameters()).
            //
            // Helper: probe APVTS for a list of candidate param IDs; return the
            // first that resolves (i.e. apvts.getParameter() returns non-null).
            auto probeParam = [&](std::initializer_list<juce::String> candidates) -> juce::String
            {
                for (const auto& id : candidates)
                    if (apvts.getParameter(id) != nullptr)
                        return id;
                return {};
            };

            // Resolve prefix for the active slot's engine.
            const juce::String pfx = [&]() -> juce::String
            {
                if (auto* eng = processor.getEngine(slot))
                    return eng->getEngineParamPrefix();
                return {};
            }();

            auto resolveXYParamId = [&](int canonIdx) -> juce::String
            {
                // Per-engine prefixes use the active slot's engine prefix.
                // For canvas-level params (macros, FX mix), use fixed IDs.
                switch (canonIdx)
                {
                    case 0:  return {}; // None
                    // Engine-specific params — probe APVTS with known suffix variants.
                    case 1:  return probeParam({pfx + "filterCutoff", pfx + "fltCutoff",
                                                pfx + "cutoff",       pfx + "filterFreq"});
                    case 2:  return probeParam({pfx + "filterRes", pfx + "filterReso",
                                                pfx + "fltRes",    pfx + "fltReso",
                                                pfx + "resonance"});
                    case 3:  return probeParam({pfx + "lfo1Rate", pfx + "lfoRate",
                                                pfx + "lfoFreq",  pfx + "lfo1Freq"});
                    case 4:  return probeParam({pfx + "lfo1Depth", pfx + "lfoDepth",
                                                pfx + "lfoAmt",    pfx + "lfo1Amt"});
                    case 5:  return probeParam({pfx + "attack",    pfx + "envAttack",
                                                pfx + "attackTime",pfx + "ampAttack"});
                    case 6:  return probeParam({pfx + "release",    pfx + "envRelease",
                                                pfx + "releaseTime",pfx + "ampRelease"});
                    case 7:  return probeParam({pfx + "drive",    pfx + "satDrive",
                                                pfx + "distDrive",pfx + "driveAmt"});
                    // Canvas-level params
                    case 8:  return "macro1";
                    case 9:  return "macro2";
                    case 10: return "macro3";
                    case 11: return "macro4";
                    // EpicChainSlot wet/dry — "slot{n}_mix" (1-indexed).
                    case 12: return "slot1_mix";
                    case 13: return "slot2_mix";
                    case 14: return "slot3_mix";
                    default: return {};
                }
            };

            const int xIdx = getCanonIdx("xy_assignX" + sfx);
            const int yIdx = getCanonIdx("xy_assignY" + sfx);

            const juce::String xParamId = resolveXYParamId(xIdx);
            const juce::String yParamId = resolveXYParamId(yIdx);

            if (xParamId.isNotEmpty())
            {
                if (auto* px = apvts.getParameter(xParamId))
                    px->setValueNotifyingHost(px->convertTo0to1(x));
            }
            if (yParamId.isNotEmpty())
            {
                if (auto* py = apvts.getParameter(yParamId))
                    py->setValueNotifyingHost(py->convertTo0to1(y));
            }

            // Also persist XY cursor position to APVTS (preset-recall support).
            if (auto* px = apvts.getParameter("xy_pos_x" + sfx))
                px->setValueNotifyingHost(px->convertTo0to1(x));
            if (auto* py = apvts.getParameter("xy_pos_y" + sfx))
                py->setValueNotifyingHost(py->convertTo0to1(y));

            // #1357 W8B: publish XY position to processor atomics so the mod routing
            // system can read them as ModSourceId::XYX{n}/XYY{n} sources.
            processor.setXYPosition(slot, x, y);
        };

        // #897: First-launch PlaySurface auto-show removed (cut 1B-#13).
        // persistPlaySurfaceVisible() is still available for backward-compat settings key writes.

        // Wire engine slots into OceanView (initial state)
        for (int i = 0; i < 4; ++i)
        {
            if (auto* eng = processor.getEngine(i))
            {
                auto id     = eng->getEngineId();
                auto accent = eng->getAccentColour();
                const int zoneInt = DepthZoneDial::depthZoneOf(id);
                const auto zone = (zoneInt == 0) ? EngineOrbit::DepthZone::Sunlit
                                : (zoneInt == 2) ? EngineOrbit::DepthZone::Midnight
                                                 : EngineOrbit::DepthZone::Twilight;
                oceanView_.setEngine(i, id, accent, zone);
            }
        }

        // ── Fix #1005: Wire OceanView floating buttons ────────────────────────

        // presetPrev / presetNext — same logic as the legacy header buttons.
        oceanView_.presetPrevButton().onClick = [this]
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
                ToastOverlay::show("Failed to load preset: " + juce::String(e.what()),
                                   Toast::Level::Warn);
                processor.killDelayTails();
            }
        };

        oceanView_.presetNextButton().onClick = [this]
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
                ToastOverlay::show("Failed to load preset: " + juce::String(e.what()),
                                   Toast::Level::Warn);
                processor.killDelayTails();
            }
        };

        // favToggleButton — toggle current preset favourite in shared settings.
        // #1114: PresetBrowserStrip (Gallery-era strip) removed. Favourite toggling is
        // now the responsibility of the OceanView sidebar's full PresetBrowser.
        oceanView_.favToggleButton().onClick = [this]
        {
            if (auto* sb = oceanView_.getSidebar())
                sb->selectTab(SidebarPanel::Preset);
        };

        // settingsTogButton — open the Settings tab in the OceanView sidebar.
        oceanView_.settingsTogButton().onClick = [this]
        {
            if (auto* sb = oceanView_.getSidebar())
                sb->selectTab(SidebarPanel::Settings);
        };

        // DNA hexagon click — toggle the DNA map browser (same as pressing 'P').
        oceanView_.onDnaClicked = [this]
        {
            juce::KeyPress p('p');
            oceanView_.keyPressed(p);
        };

        // ── Fix #1005: Seed the DnaMapBrowser with all preset dots ────────────
        // Build PresetDot vector from the current library snapshot so the scatter
        // map is populated on first open (not empty).  Re-seeded in timerCallback
        // whenever the library changes (preset scan completes after editor opens).
        {
            auto lib = processor.getPresetManager().getLibrary();
            if (lib && !lib->empty())
            {
                std::vector<PresetDot> dots;
                dots.reserve(lib->size());
                for (int idx = 0; idx < static_cast<int>(lib->size()); ++idx)
                {
                    const auto& p = (*lib)[static_cast<size_t>(idx)];
                    PresetDot dot;
                    dot.name        = p.name;
                    dot.mood        = p.mood;
                    dot.brightness  = p.dna.brightness;
                    dot.warmth      = p.dna.warmth;
                    dot.movement    = p.dna.movement;
                    dot.density     = p.dna.density;
                    dot.space       = p.dna.space;
                    dot.aggression  = p.dna.aggression;
                    dot.presetIndex = idx;
                    dots.push_back(std::move(dot));
                }
                oceanView_.setPresetDots(std::move(dots));
            }
            // setActivePresetIndex will be kept live via timerCallback.
        }

        // Hide old Gallery layout components (replaced by OceanView)
        for (int i = 0; i < kNumPrimarySlots; ++i)
            tiles[i]->setVisible(false);
        ghostTile.setVisible(false);
        overview.setVisible(false);
        detail.setVisible(false);
        performancePanel.setVisible(false);
        macros.setVisible(false);
        sidebar.setVisible(false);
        statusBar.setVisible(false);
        depthDial.setVisible(false);
        abCompare.setVisible(false);
        cpuMeter.setVisible(false);
        midiIndicator.setVisible(false);
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
        // Remove from the component tree entirely so it does not double-render alongside
        // SubmarinePlaySurface inside OceanView (Fix 4: dual PlaySurface crash vector).
        playSurface_.setVisible(false);
        playSurface_.setMidiCollector(nullptr);  // Detach — OceanView owns MIDI now
        removeChildComponent(&playSurface_);

        // ── D12: O badge + About modal ───────────────────────────────────────
        // Added before ToastOverlay so the toast sits on top of the modal.
        // OBadgeButton is always visible; AboutModal starts hidden.
        addAndMakeVisible(obadge_);
        obadge_.setAlwaysOnTop(true);
        obadge_.onClick = [this]
        {
            // D12: single click → About tab.
            aboutModal_.openTab(AboutModal::Tab::About);
        };

        obadge_.onLongPress = [this]
        {
            // D12: long-press (≥ 500 ms) → Lore tab.
            aboutModal_.openTab(AboutModal::Tab::Lore);
        };

        addChildComponent(aboutModal_);
        aboutModal_.setAlwaysOnTop(true);

        // ── Wave 5 A1: DragDropModRouter overlay ─────────────────────────────
        // Instantiated here (after processor is fully constructed) so the
        // ModRoutingModel reference in the processor is stable.
        //
        // The overlay is added BEFORE toastOverlay_ so toasts still paint above it.
        // It is transparent + pass-through (setInterceptsMouseClicks(false,false))
        // while idle; it activates mouse interception only during an active drag.
        //
        // A1 test route: LFO1 → orry_fltCutoff, depth=+0.5, bipolar=true.
        // Added unconditionally on first launch so the user can verify the route
        // fires (filter cutoff sweeps when Orrery is loaded and LFO1 is running).
        // The route is serialised in the preset state — remove via the route list
        // panel (right-click or double-click to open depth editor).
        // Wire the flush listener to the processor before creating the router.
        modRouteFlushListener_.proc = &proc;

        modRouter_ = std::make_unique<DragDropModRouter>(proc.getAPVTS(), proc.getModRoutingModel());
        addAndMakeVisible(*modRouter_);
        modRouter_->toFront(false);
        // Route list panel visible by default in A1 as the dev affordance (#670).
        // Users confirm activation by seeing the route strip and verifying LFO1 sweeps the filter.
        modRouter_->setRouteListVisible(true);

        // Install the first end-to-end test route: LFO1 → Orrery filter cutoff, depth=+0.5.
        // Only add it if the model is empty (first launch or fresh preset state) so
        // repeated editor constructions don't accumulate duplicate routes.
        if (proc.getModRoutingModel().getRouteCount() == 0)
        {
            proc.getModRoutingModel().addRoute(
                static_cast<int>(ModSourceId::LFO1),
                "orry_fltCutoff",
                0.5f,
                /* bipolar = */ true);
            proc.flushModRoutesSnapshot();
        }

        // Register a change listener so the processor snapshot is flushed
        // whenever the route table is modified through the drag-drop UI.
        // modRouteFlushListener_ is a member (lifetime tied to the editor) so
        // the processor never receives a dangling listener pointer.
        proc.getModRoutingModel().addListener(&modRouteFlushListener_);

        // ── Wave 5 A3: ModMatrixStrip mount ──────────────────────────────────
        // Built after modRouter_ so modModel_ + router references are stable.
        // addPanelToParent() adds the slide-up panel as a direct child of the
        // editor root so the panel can float above all other UI at ~60% height.
        modMatrixStrip_ = std::make_unique<ModMatrixStrip>(
            proc.getAPVTS(), proc.getModRoutingModel(), *modRouter_);
        addAndMakeVisible(*modMatrixStrip_);
        modMatrixStrip_->addPanelToParent(*this);

        // ── Wave 9c (#1303): First-hour walkthrough overlay ──────────────────
        // Wire bound accessors so the walkthrough bubbles point at the correct
        // components.  Components that are not yet directly addressable from the
        // editor return empty rectangles; the bubble falls back to the safe
        // centred-area position and the tour still functions.
        walkthrough_.getPlaySurfaceBounds  = [this]() { return oceanView_.getSubmarinePlaySurface().getBounds(); };
        walkthrough_.getEngineSlotBounds   = [this]() {
            return tiles[0] != nullptr ? tiles[0]->getBounds() : juce::Rectangle<int>{};
        };
        walkthrough_.getMacroBounds        = [this]() { return macros.getBounds(); };
        walkthrough_.getDnaBrowserBounds   = [this]() {
            // F-003 / #1395: points at the HudBar preset-name pill (always visible;
            // opens DnaMapBrowser when clicked). getDnaMapBrowserBounds() now
            // returns the preset-name label bounds — not browser_.getBounds()
            // (which is {} unless BrowserOpen state).
            auto b = oceanView_.getDnaMapBrowserBounds();
            return b.isEmpty() ? juce::Rectangle<int>{}
                               : b.translated(oceanView_.getX(), oceanView_.getY());
        };
        walkthrough_.getCoupleOrbitBounds  = [this]() {
            // wire(#orphan-sweep item 2): orbit slot 1 bounds for the coupling step.
            // Slot 1 = second engine buoy; falls back to {} if not yet visible.
            auto b = oceanView_.getOrbitBounds(1);
            return b.isEmpty() ? juce::Rectangle<int>{}
                               : b.translated(oceanView_.getX(), oceanView_.getY());
        };
        walkthrough_.getCmToggleBounds     = [this]() { return cmToggleBtn.getBounds(); };
        walkthrough_.getFavBtnBounds       = [this]() {
            // wire(#orphan-sweep item 2): HUD fav button bounds via SubmarineHudBar.getFavBounds().
            auto b = oceanView_.getHudFavBounds();
            return b.isEmpty() ? juce::Rectangle<int>{}
                               : b.translated(oceanView_.getX(), oceanView_.getY());
        };
        // walkthrough_.getXouijaBounds removed 2026-05-01 (XOuija wholesale removal)

        // Mount as topmost child before toastOverlay_ — paints above all panels
        // but below toasts so notifications are never obscured.
        addAndMakeVisible(walkthrough_);

        // wire(1C-5): walkthrough complete — the overlay dismisses itself before
        // firing this callback (skipAll/completeTour both call dismissAll() first).
        // We just trigger a repaint so the now-invisible walkthrough layer is cleared.
        walkthrough_.onWalkthroughComplete = [this]()
        {
            repaint();
        };

                // ── ToastOverlaybe the last addAndMakeVisible call ────────────
        // JUCE paints children in insertion order; last child paints on top.
        // setInterceptsMouseClicks(false, false) is set inside ToastOverlay's
        // constructor so it never captures events that should reach panels below.
        addAndMakeVisible(toastOverlay_);
        toastOverlay_.setAlwaysOnTop(true);
        ToastOverlay::setInstance(&toastOverlay_);

        // ── #1356: Preset pill → CallOutBox wiring ───────────────────────────
        // Wire per-slot pill click: open a per-engine filtered PresetBrowserPanel
        // in a CallOutBox anchored to the buoy bounds.
        // Constraint: must NOT touch lines 824-836 (#1359 territory).
        oceanView_.onPresetPillClicked = [this](int slotIndex)
        {
            if (slotIndex < 0 || slotIndex >= XOceanusProcessor::kNumPrimarySlots)
                return;

            auto* eng = processor.getEngine(slotIndex);
            if (eng == nullptr)
                return; // empty slot — pill shows "no engine", no menu

            const juce::String engineId = eng->getEngineId();
            const juce::Rectangle<int> buoyBounds = [&]()
            {
                // Translate buoy bounds from OceanView local → screen coords for CallOutBox anchor.
                auto orbitBounds = oceanView_.getOrbitBounds(slotIndex);
                return orbitBounds.translated(oceanView_.getScreenX(), oceanView_.getScreenY());
            }();

            // Build the panel: filtered to slotIndex's engine, menu stays open after load (Q2).
            auto panel = std::make_unique<PresetBrowserPanel>(
                processor.getPresetManager(),
                [this, slotIndex](const PresetData& preset)
                {
                    // Apply preset to this slot's engine then update data model + APVTS (Q4).
                    try
                    {
                        processor.getUndoManager().beginNewTransaction("Load preset: " + preset.name);
                        processor.applyPreset(preset);
                        processor.setSlotPreset(slotIndex, preset); // updates APVTS slot{n}_presetName
                        // Pill text updated via slotPresetChanged() listener (registered below).
                    }
                    catch (const std::exception& e)
                    {
                        ToastOverlay::show("Could not load " + preset.name + " — " + e.what(),
                                           Toast::Level::Warn);
                    }
                    // Q2: menu stays open — no dismiss here.
                },
                engineId,
                slotIndex);

            // Size: 280x380 per design spec.
            panel->setSize(PresetBrowserPanel::kMinWidth + 20, 380);

            juce::CallOutBox::launchAsynchronously(
                std::move(panel),
                buoyBounds,
                getTopLevelComponent());
        };

        // Register this editor as a SlotPresetListener so pills stay in sync
        // when setStateInformation restores slot presets or undo fires (#1356 acceptance #8).
        processor.addSlotPresetListener(this);

        // Initialise pill text from current state (e.g. after DAW session restore).
        for (int i = 0; i < XOceanusProcessor::kNumPrimarySlots; ++i)
        {
            const auto& sp = processor.getSlotPreset(i);
            oceanView_.setOrbitPresetName(i, sp.name);
        }

        // F3-006: Restore REACT dial level from persisted session state.
        {
            const float restoredReact = proc.getPersistedReactLevel();
            oceanView_.setReactivity(restoredReact);
        }

        // F3-011/F3-017: Restore breakout panel open states (default false = closed).
        oceanView_.restoreBreakoutState(proc.getPersistedSeqBreakoutOpen(),
                                        proc.getPersistedChordBreakoutOpen());

        // F2-006: Restore OceanView ViewState from session.  Uses a one-tick deferred
        // call so OceanView layout is fully settled before any state transition.
        {
            const int restoredState = proc.getPersistedOceanViewState();
            const int restoredSlot  = proc.getPersistedOceanViewSlot();
            // Only restore non-trivial states — Orbital (0) is the default and needs no action.
            if (restoredState == 1 && restoredSlot >= 0) // ZoomIn
            {
                // F2-015: Schedule zoom-in so the first layout pass has completed.
                juce::Timer::callAfterDelay(50,
                    [safeThis = juce::Component::SafePointer<XOceanusEditor>(this), restoredSlot]()
                    {
                        if (safeThis != nullptr)
                            safeThis->oceanView_.requestZoomIn(restoredSlot);
                    });
            }
            // SplitTransform (2) and BrowserOpen (3) are not restored — too complex and
            // rarely persisted intentionally; users re-enter them manually.
        }

        // P1-8 (1F): Wire dashboard tab + kit sub-mode persistence callbacks.
        oceanView_.onDashboardTabChanged = [this](int tabIndex)
        {
            processor.setPersistedDashboardTab(tabIndex);
        };
        oceanView_.onDashboardKitSubModeChanged = [this](bool kitMode)
        {
            processor.setPersistedKitSubMode(kitMode);
        };

        // P1-8 (1F): Restore dashboard tab + kit sub-mode from saved session.
        // Use callAfterDelay so the first resized() pass has completed before
        // selectTab() triggers a second layout pass.  50ms matches the ZoomIn
        // restore pattern above.
        {
            const int restoredTab = proc.getPersistedDashboardTab();
            const bool restoredKit = proc.getPersistedKitSubMode();
            if (restoredTab != 0 || restoredKit)
            {
                juce::Timer::callAfterDelay(50,
                    [safeThis = juce::Component::SafePointer<XOceanusEditor>(this),
                     restoredTab, restoredKit]()
                    {
                        if (safeThis != nullptr)
                            safeThis->oceanView_.restoreDashboardTab(restoredTab, restoredKit);
                    });
            }
        }
    }

    ~XOceanusEditor() override
    {
        stopTimer();
        removeKeyListener(statusBar.getKeyListener());
        // #1356: Unsubscribe from per-slot preset change notifications before teardown.
        processor.removeSlotPresetListener(this);
        processor.onEngineChanged = nullptr; // prevent callback after editor is destroyed

        // F2-001 (CRIT): Null TideWaterline + MIDILearn callbacks before members are
        // destroyed.  DAW may call getStateInformation() after editor close — these
        // lambdas capture local pointers and would be UAF without the null-out.
        processor.onGetTideWaterlineState = nullptr;
        processor.onSetTideWaterlineState = nullptr;

        // F2-004 (CRIT): Null MIDILearnManager learn-complete callback.
        // The callback captures `this` (editor) — null it out before teardown.
        processor.getMIDILearnManager().setLearnCompleteCallback({});

        // Wave 5 A1: Remove the mod route flush listener before the editor members
        // are destroyed so the processor never calls back into a freed listener.
        processor.getModRoutingModel().removeListener(&modRouteFlushListener_);
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
            layout.cinematicMode = !layout.cinematicMode;
            cinematicToggleBtn.setToggleState(layout.cinematicMode, juce::dontSendNotification);
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
        // 'P' — PlaySurface toggle no-op (PlaySurface removed, cut 1B-#13)
        if (key == juce::KeyPress('p') || key == juce::KeyPress('P'))
            return false;
        // 'B' toggles Dark Cockpit bypass (fully lit regardless of audio activity)
        if (key == juce::KeyPress('b') || key == juce::KeyPress('B'))
        {
            cockpitBypass_ = !cockpitBypass_;
            processor.setPersistedCockpitBypass(cockpitBypass_); // persist for session restore (#357)
            repaint();
            return true;
        }
        // Escape returns to overview
        if (key == juce::KeyPress::escapeKey)
        {
            // PlaySurface toggle removed (cut 1B-#13); fall through to overview.
            showOverview();
            return true;
        }
        // F2-014: Cmd+S — save current preset (mirrors the SAVE button in the sidebar).
        if (key == juce::KeyPress('s', juce::ModifierKeys::commandModifier, 0))
        {
            if (oceanView_.onSavePreset)
                oceanView_.onSavePreset();
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

                const int headerH = ColumnLayoutManager::kHeaderH;
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
        // ── Ocean View takes full editor bounds ───────────────────────────────
        auto fullBounds = getLocalBounds();
        oceanView_.setBounds(fullBounds);

        // ── Wave 5 A1: DragDropModRouter overlay — always full editor bounds ──
        if (modRouter_ != nullptr)
            modRouter_->setBounds(fullBounds);

        // ── Wave 5 A3: ModMatrixStrip — 28px strip at the bottom of the editor ──
        if (modMatrixStrip_ != nullptr)
        {
            constexpr int kStripH = ModMatrixStrip::kStripHeight;
            modMatrixStrip_->setBounds(0, getHeight() - kStripH, getWidth(), kStripH);
            modMatrixStrip_->setEditorBounds(fullBounds);
        }

        // ── OceanView mode: skip the entire legacy Gallery layout ─────────────
        // All legacy tiles, overview, detail, chord panel, sidebar, etc. are
        // permanently hidden when OceanView is active.  Only statusBar and
        // toastOverlay_ need bounds; everything else is dead weight.
        if (oceanView_.isVisible())
        {
            statusBar.setBounds(layout.getStatusBar());
            {
                auto statusArea = layout.getStatusBar();
                statusArea.removeFromRight(104);
                midiIndicator.setBounds(statusArea.removeFromRight(16).withSizeKeepingCentre(8, 8));
                cpuMeter.setBounds(statusArea.removeFromRight(68).withSizeKeepingCentre(64, 20));
            }

            // D12: O badge — top-left corner, 8px inset from the SubmarineHudBar
            // left edge (SubmarineHudBar is positioned at x=16, so badge at x=8).
            obadge_.setBounds(8, 8, OBadgeButton::kBadgeSize, OBadgeButton::kBadgeSize);

            // D12: About modal — full editor bounds; card is centered inside it.
            aboutModal_.setBounds(getLocalBounds());

            toastOverlay_.setBounds(getLocalBounds());

            // Wave 9c: walkthrough overlay always tracks full editor bounds.
            walkthrough_.setBounds(getLocalBounds());

            return;
        }
    }

private:
    //==========================================================================
    // XOceanusProcessor::SlotPresetListener (#1356)
    //==========================================================================

    /** Called on the message thread whenever a slot's preset changes.
        Updates the EngineOrbit preset pill text so it stays in sync with any
        code path that writes via setSlotPreset() (including setStateInformation
        restores, undo, and our own pill-menu selection).
        F3-001: also notifies ABCompare so a "B" snapshot is captured whenever
        a new preset loads while A/B compare mode is active. */
    void slotPresetChanged(int slotIdx, const PresetData& preset) override
    {
        jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());
        oceanView_.setOrbitPresetName(slotIdx, preset.name);
        abCompare.onPresetLoaded(); // F3-001: capture new state into B slot
    }

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
        depthDial.setSlot(juce::jlimit(0, 4, slot)); // DepthDial tracks the selected slot
        cmToggleBtn.setToggleState(false, juce::dontSendNotification);
        perfToggleBtn.setToggleState(false, juce::dontSendNotification);

        // Hide performance panel if visible
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
                    detail.setAlpha(1.0f);
                    detail.setVisible(true);
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
        }
        else
        {
            // Fade out overview, fade in detail
            // #926: skip animation when the OS/in-app reduced-motion preference is active
            if (A11y::prefersReducedMotion())
            {
                overview.setVisible(false);
                if (detail.loadSlot(slot))
                {
                    detail.setAlpha(1.0f);
                    detail.setVisible(true);
                }
                else
                {
                    overview.setAlpha(1.0f);
                    overview.setVisible(true);
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
                        juce::Desktop::getInstance().getAnimator().fadeIn(&safeThis->overview, kFadeMs);
                    });
            }
        }
    }

    void showChordMachine()
    {
        // #1114: Gallery-era ChordMachinePanel removed. In OceanView mode the chord
        // machine is exposed via the OceanView's TideWaterline/ChordBar. The cmToggleBtn
        // is hidden under OceanView so this path is unreachable at runtime; kept as a
        // stub to avoid orphaned caller references.
        juce::ignoreUnused(this);
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
        juce::Component* outgoing = detail.isVisible()     ? static_cast<juce::Component*>(&detail)
                                    : overview.isVisible() ? static_cast<juce::Component*>(&overview)
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

    // ── PlaySurface show/hide — legacy stubs (cut 1B-#13) ────────────────────
    // persistPlaySurfaceVisible() is kept for backward-compat settings key writes.
    void persistPlaySurfaceVisible(bool visible)
    {
        if (settingsFile_ != nullptr)
            settingsFile_->setValue("playSurfaceVisible", visible);
    }

    void showPlaySurface() { persistPlaySurfaceVisible(true); }
    void hidePlaySurface() { persistPlaySurfaceVisible(false); }
    void timerCallback() override
    {
        // ── Legacy Gallery refresh — skip entirely when OceanView is active ───
        if (!oceanView_.isVisible())
        {
            for (int i = 0; i < kNumPrimarySlots; ++i)
                tiles[i]->refresh();
            if (ghostTile.isVisible())
                ghostTile.refresh();
            if (!detail.isVisible())
                overview.refresh();
            if (performancePanel.isVisible())
                performancePanel.refresh();
        }
        checkCollectionUnlock();

        // ── Wave 9c (#1303): prompt for walkthrough after first timer tick ────
        // Fires once per session — one tick of latency ensures the editor is
        // fully constructed and bounds are settled before the overlay appears.
        // Pre-Wave7 interim path: no explicit greeting flag, so we use the
        // session-guard bool.  Post-Wave 7: move this into
        // OceanStateMachine::onGreetingComplete().
        //
        // wire(#orphan-sweep item 2): race fix — do NOT prompt while firstBreath is
        // active (the greeting note is playing and the user is in the first-sound
        // experience).  If firstBreath is still active this tick, hold off; the
        // walkthroughTriggeredThisSession_ guard is intentionally NOT set so the
        // next timer tick re-evaluates the condition.
        if (!walkthroughTriggeredThisSession_ && !processor.isFirstBreathActive())
        {
            walkthroughTriggeredThisSession_ = true;
            walkthrough_.promptIfEligible(settingsFile_.get());
        }

        // ── Refresh Export tab panel with current preset/kit info ────────────
        if (!oceanView_.isVisible())
            sidebar.refreshExportPanel();

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
                startTimerHz(10);
            }
        }

        // Drain Field Map note events from the lock-free audio-thread queue.
        // Color is resolved here on the message thread (safe: getEngine / getAccentColour).
        // XO Gold is used as fallback when no engine occupies the slot.
        static const juce::Colour kXOGold = juce::Colour(0xFFE9C46A);
        bool hadNoteOn = false; // Fix #1005: detect any note-on for PlaySurface auto-show
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
                midiIndicator.flash(colour); // flash on every incoming note event
                // velocity > 0 = note-on; velocity == 0 = note-off.
                if (ev.velocity > 0.0f)
                {
                    hadNoteOn = true;

                    // D6 (#1096): NexusDisplay removed. Session DNA drift was fed here;
                    // TODO(#1096-followup): route session DNA to preset browser overlay.
                    lastNote_ = ev.midiNote;
                }
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
            double bpm = 0.0;
            {
                if (auto* ph = processor.getPlayHead())
                {
                    auto pos = ph->getPosition();
                    if (pos.hasValue() && pos->getBpm().hasValue())
                        bpm = *pos->getBpm();
                }
                statusBar.setBpm(bpm);
            }

            // W03: CPU — read from processor's measured processBlock load.
            const float cpuPct = processor.getProcessingLoad() * 100.0f;
            statusBar.setCpuPercent(cpuPct);

            // Push same data to submarine TransportBar.
            if (auto* tb = oceanView_.getTransportBar())
            {
                tb->setVoiceCount(totalVoices);
                // When host transport is unavailable (bpm == 0), read the persisted
                // cm_seq_bpm APVTS value so the TransportBar reflects the user's last
                // setting rather than always snapping back to 120.
                double displayBpm = bpm;
                if (displayBpm <= 0.0)
                {
                    if (auto* raw = processor.getAPVTS().getRawParameterValue("cm_seq_bpm"))
                        displayBpm = static_cast<double>(raw->load());
                    if (displayBpm <= 0.0)
                        displayBpm = 120.0;
                }
                tb->setBpm(displayBpm);
                tb->setCpuPercent(cpuPct);
            }

            // Push audio levels to dot-matrix visualizer.
            if (auto* dm = oceanView_.getDotMatrix())
            {
                // Use CPU load as a proxy for audio activity until we have real RMS.
                const float activity = cpuPct / 100.0f;
                dm->pushLevels(activity * 0.7f, activity * 0.5f);

                // Push engine activity levels.
                float engLevels[4] = {};
                for (int i = 0; i < 4; ++i)
                {
                    if (auto* eng = processor.getEngine(i))
                        engLevels[i] = static_cast<float>(eng->getActiveVoiceCount()) / static_cast<float>(std::max(1, eng->getMaxVoices()));
                }
                dm->pushEngineActivity(engLevels, 4);

                // Push sequencer step.
                const auto& seq = processor.getMasterFXChain().getSequencer();
                dm->pushSequencerStep(seq.getCurrentStep(), 16, seq.isEnabled());
            }
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

        // Wave6.5 mount B removed (cut 1B): layoutModeCache_ unused.
        juce::ignoreUnused(layoutModeCache_);

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
            // #1097: SidebarPanel::Couple tab removed (CPL-tab-removed). Coupling
            // configuration now lives in CouplingConfigPopup (knot double-click).
            // couplingVisible is permanently false; RegisterManager::Coupling register
            // can no longer be triggered from the sidebar.
            constexpr bool couplingVisible = false;

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
        {
            bool hasSunlit   = false;
            bool hasTwilight = false;
            bool hasMidnight = false;

            for (int i = 0; i < 4; ++i)
            {
                if (auto* eng = processor.getEngine(i))
                {
                    auto id     = eng->getEngineId();
                    auto accent = eng->getAccentColour();
                    const int zoneInt = DepthZoneDial::depthZoneOf(id);
                    const auto zone = (zoneInt == 0) ? EngineOrbit::DepthZone::Sunlit
                                    : (zoneInt == 2) ? EngineOrbit::DepthZone::Midnight
                                                     : EngineOrbit::DepthZone::Twilight;
                    oceanView_.setEngine(i, id, accent, zone);

                    // Step 8b: Track voice count changes to trigger buoy ripples on note-on.
                    int newVoices = eng->getActiveVoiceCount();
                    if (newVoices > prevVoiceCounts_[static_cast<size_t>(i)])
                        oceanView_.triggerBuoyRipple(i);
                    prevVoiceCounts_[static_cast<size_t>(i)] = newVoices;

                    oceanView_.setVoiceCount(i, newVoices);

                    // Step 8a: Waveform data is now pushed via oceanView_.onPullWaveformData
                    // (F2-012) from OceanView's 30Hz timer — removed from here (10Hz).

                    if (zone == EngineOrbit::DepthZone::Sunlit)   hasSunlit   = true;
                    if (zone == EngineOrbit::DepthZone::Twilight)  hasTwilight = true;
                    if (zone == EngineOrbit::DepthZone::Midnight)  hasMidnight = true;
                }
                else
                {
                    oceanView_.clearEngine(i);
                }
            }

            // Fix #1005: drive AmbientEdge depth glow from populated zones.
            oceanView_.setDepthZones(hasSunlit, hasTwilight, hasMidnight);
        }

        // Ocean wave surface — push master output waveform to background.
        oceanView_.pushMasterWaveData(processor.getMasterWaveformFifo());

        // Preset info — update nexus when preset name changes.
        {
            const auto& pm = processor.getPresetManager();
            const auto& currentPreset = pm.getCurrentPreset();
            oceanView_.setPresetName(currentPreset.name);

            // Fix #1005: push mood name + colour (no-op since D6 — NexusDisplay removed).
            oceanView_.setMoodName(currentPreset.mood);
            // Map mood string to colour — same table used by DnaMapBrowser.
            auto moodColourFor = [](const juce::String& mood) -> juce::Colour
            {
                if (mood == "Foundation")  return juce::Colour(0xFF9E9B97);
                if (mood == "Atmosphere")  return juce::Colour(0xFF48CAE4);
                if (mood == "Entangled")   return juce::Colour(0xFF9B5DE5);
                if (mood == "Prism")       return juce::Colour(0xFFBF40FF);
                if (mood == "Flux")        return juce::Colour(0xFFFF6B6B);
                if (mood == "Aether")      return juce::Colour(0xFFA8D8EA);
                if (mood == "Family")      return juce::Colour(0xFFE9C46A);
                if (mood == "Submerged")   return juce::Colour(0xFF0096C7);
                if (mood == "Coupling")    return juce::Colour(0xFFE9C46A);
                if (mood == "Crystalline") return juce::Colour(0xFFFFFFF0);
                if (mood == "Deep")        return juce::Colour(0xFF1A6B5A);
                if (mood == "Ethereal")    return juce::Colour(0xFF7FDBCA);
                if (mood == "Kinetic")     return juce::Colour(0xFFFF8C00);
                if (mood == "Luminous")    return juce::Colour(0xFFC6E377);
                if (mood == "Organic")     return juce::Colour(0xFF228B22);
                if (mood == "Shadow")      return juce::Colour(0xFF546E7A);
                return juce::Colour(0xFF9E9B97); // fallback
            };
            oceanView_.setMoodColour(moodColourFor(currentPreset.mood));

            // DNA update — push 6D vector to OceanView nexus.
            const auto& dna = currentPreset.dna;
            oceanView_.setDNA(dna.brightness, dna.warmth, dna.movement,
                              dna.density, dna.space, dna.aggression);

            // Fix #1005: keep activePresetIndex live so the browser dot glows.
            oceanView_.setActivePresetIndex(pm.getCurrentPresetIndex());

            // Fix #1005: re-seed preset dots when library size changes (scan just completed).
            {
                auto lib = pm.getLibrary();
                const int currentLibSize = lib ? static_cast<int>(lib->size()) : 0;
                if (currentLibSize != lastLibrarySize_)
                {
                    lastLibrarySize_ = currentLibSize;
                    if (lib && currentLibSize > 0)
                    {
                        std::vector<PresetDot> dots;
                        dots.reserve(static_cast<size_t>(currentLibSize));
                        for (int idx = 0; idx < currentLibSize; ++idx)
                        {
                            const auto& p = (*lib)[static_cast<size_t>(idx)];
                            PresetDot dot;
                            dot.name        = p.name;
                            dot.mood        = p.mood;
                            dot.brightness  = p.dna.brightness;
                            dot.warmth      = p.dna.warmth;
                            dot.movement    = p.dna.movement;
                            dot.density     = p.dna.density;
                            dot.space       = p.dna.space;
                            dot.aggression  = p.dna.aggression;
                            dot.presetIndex = idx;
                            dots.push_back(std::move(dot));
                        }
                        oceanView_.setPresetDots(std::move(dots));
                    }
                }
            }
        }

        // ── #909: Live parameter feedback — voice count + macro values ─────────
        // D6 (#1096): NexusDisplay removed. setLiveReadouts / tickSustainedDna are
        // now no-ops; calls retained so call sites compile without change.
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

            // Feature 6 (Schulze): Sustained-voice DNA accumulation.
            // Drive continuous DNA drift when voices are held (0.1s dt at 10 Hz).
            oceanView_.tickSustainedDna(totalVoices, 0.1f);
        }

        // Coupling routes — convert MegaCouplingMatrix routes to OceanView CouplingRoute structs.
        // Fix #1005: also compute per-slot lean from net coupling amounts and call setCouplingLean().
        {
            const auto& matrixRoutes = processor.getCouplingMatrix().getRoutes();
            std::vector<CouplingRoute> oceanRoutes;
            oceanRoutes.reserve(matrixRoutes.size());

            // Accumulate per-slot net lean: sum of (amount) for routes FROM that slot.
            // Positive = leans right (more modulator), negative = leans left (more target).
            std::array<float, 5> leanAccum{};

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

                // Accumulate lean: source leans toward destination (positive),
                // destination leans back toward source (negative fraction).
                if (r.sourceSlot >= 0 && r.sourceSlot < 5)
                    leanAccum[static_cast<size_t>(r.sourceSlot)] += r.amount;
                if (r.destSlot >= 0 && r.destSlot < 5)
                    leanAccum[static_cast<size_t>(r.destSlot)]   -= r.amount * 0.5f;
            }
            oceanView_.setCouplingRoutes(oceanRoutes);

            // Fix #1005: drive per-slot coupling lean into EngineOrbit.
            for (int i = 0; i < 5; ++i)
            {
                const float lean = juce::jlimit(-1.0f, 1.0f,
                                               leanAccum[static_cast<size_t>(i)]);
                oceanView_.setCouplingLean(i, lean);
            }
        }

        // Feature 7 (Schulze): Push coupling age timeline to StatusBar.
        oceanView_.updateCouplingTimeline();

        // #1005: MIDI auto-show removed (cut 1B-#13 PlaySurface removed).
        juce::ignoreUnused(hadNoteOn);

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

    // Shared persistent settings file — opened once at construction, reused for all
    // read/write operations to avoid blocking message-thread callbacks with repeated
    // file open/close overhead (Fix 3: PropertiesFile inline-construction crash vector).
    std::unique_ptr<juce::PropertiesFile> settingsFile_;

    std::array<std::unique_ptr<CompactEngineTile>, kNumPrimarySlots> tiles;
    OverviewPanel overview;
    EngineDetailPanel detail;
    PerformanceViewPanel performancePanel;
    MacroSection macros;
    // Ghost Slot tile — declared after macros to maintain constructor MIL order.
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

    // Wave6.5 mount C (#1306): per-slot layout mode cache for APVTS polling.
    std::array<int, 4> layoutModeCache_ { -1, -1, -1, -1 };

    // Last MIDI note number seen (for interval computation in session DNA drift).
    // -1 = no note played yet this session.
    int lastNote_ = -1;

    // Tracks the last known preset library size so we only re-seed preset dots
    // when the library is first populated or changes (e.g. after a background scan).
    // Promoted from function-local static to avoid per-instance aliasing bugs.
    int lastLibrarySize_ = 0;

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

    ColumnLayoutManager layout;

    // ── Ocean View — primary layout (replaces Gallery 3-column model) ─────────
    // Declared before toastOverlay_ so it is destroyed after the toast singleton
    // is cleared.  addAndMakeVisible is called BEFORE toastOverlay_ in the
    // constructor so it renders beneath the overlay.
    OceanView oceanView_;

    // Step 8b: Previous voice counts per slot — used to detect note-on events
    // (voice count increase) and trigger buoy ripple animations.
    std::array<int, 5> prevVoiceCounts_ {};

    // ── D12: About/Lore modal + O badge brand button ──────────────────────────
    // Both are overlaid on top of OceanView as direct editor children.
    // obadge_ sits in the top-left corner; aboutModal_ is a full-editor-bounds
    // centered-card overlay (click outside card → dismiss).
    OBadgeButton obadge_;
    AboutModal   aboutModal_;

    // ── Wave 5 A1: DragDropModRouter overlay ──────────────────────────────────

    // ChangeListener that flushes the processor's mod route snapshot whenever
    // the route table changes on the message thread.
    // Stored as a member so the listener pointer never outlives the editor.
    struct ModRouteFlushListener : public juce::ChangeListener
    {
        XOceanusProcessor* proc{nullptr};
        void changeListenerCallback(juce::ChangeBroadcaster*) override
        {
            if (proc != nullptr)
                proc->flushModRoutesSnapshot();
        }
    } modRouteFlushListener_;

    // Wave 5 A3: ModMatrixStrip — 28px footer strip + slide-up panel.
    // Constructed in initOceanView() after modRouter_ is built so that
    // modModel_ and modRouter_ pointers are already stable.
    std::unique_ptr<ModMatrixStrip> modMatrixStrip_;

    // Transparent full-editor overlay — activates only while a drag is in flight
    // or when the route list panel is shown.  Declared before toastOverlay_ so
    // the overlay still paints above the router.
    // unique_ptr: constructed in initOceanView() after the processor is ready
    // (ModRoutingModel lives in the processor, pointer stable for plugin lifetime).
    std::unique_ptr<DragDropModRouter> modRouter_;

    // ── Wave 9c (#1303): First-hour walkthrough overlay ──────────────────────
    // Transparent overlay that paints the gold highlight ring and floating bubbles.
    // Mounted as topmost child (above obadge_/aboutModal_, below toastOverlay_).
    // setBounds: always getLocalBounds() (set in resized()).
    FirstHourWalkthrough walkthrough_;
    // True once promptIfEligible() has been fired this session — prevents re-firing
    // on every timerCallback tick after the greeting flag clears.
    bool walkthroughTriggeredThisSession_ = false;

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
