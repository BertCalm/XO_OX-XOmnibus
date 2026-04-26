# XOceanus — Monorepo Structure & Migration Plan

**Version:** 1.0
**Author:** XO_OX Designs
**Date:** 2026-03-08
**Purpose:** Define the GitHub monorepo structure for XOceanus and the migration plan from 6 existing XO_OX repos

---

## 1. Overview

### Why a Monorepo

XOceanus is the collected works of XO_OX Designs — 7 synth engines unified into a single gallery-model instrument. A monorepo is the correct structure for three reasons:

1. **Shared DSP.** CytomicSVF, PolyBLEP, FastMath, ADSR, Wavetable, DCBlocker, and Reverb are used across multiple engines. A monorepo means one copy, one place to fix bugs, one place to optimize. No submodule version drift, no diamond dependency problems.

2. **Single build system.** One CMakeLists.txt builds all engines, the shared DSP library, the PlaySurface, the UI shell, and the plugin targets for macOS and iOS. CI runs once, validates everything. No per-repo build matrix.

3. **Clean forward.** The 6 existing repos were each built as standalone instruments. They served that purpose. XOceanus is a new product with a new architecture (gallery shell, engine registry, coupling matrix, shared preset system). Migrating into a fresh repo avoids carrying forward 6 different directory conventions, build configs, and naming patterns.

**Repo location:** `~/Documents/GitHub/XOceanus/`
**Existing repos:** Archived as read-only historical references after migration.

---

## 2. Directory Structure

```
XOceanus/
├── CLAUDE.md                          # Agent guide (see Section 4)
├── CMakeLists.txt                     # Top-level build (see Section 3)
├── ios.cmake                          # iOS cross-compilation toolchain
│
├── Libs/
│   └── JUCE/                          # JUCE 8.0.12 (git clone, not submodule)
│
├── Source/
│   ├── Core/
│   │   ├── SynthEngine.h              # Abstract interface all engines implement
│   │   ├── EngineRegistry.h/.cpp      # Runtime engine discovery + instantiation
│   │   ├── CouplingMatrix.h/.cpp      # Cross-engine modulation routing
│   │   ├── SharedTransport.h/.cpp     # Global BPM, sync, clock
│   │   ├── PresetManager.h/.cpp       # .xometa load/save, mood categorization
│   │   ├── SonicDNA.h/.cpp            # DNA vector computation + comparison
│   │   └── ParameterLayout.h          # APVTS layout builder, namespaced IDs
│   │
│   ├── DSP/
│   │   ├── CytomicSVF.h              # State-variable filter (Cytomic/Zavalishin)
│   │   ├── PolyBLEP.h                # Band-limited oscillator
│   │   ├── FastMath.h                 # Fast sin/cos/tanh approximations
│   │   ├── ADSR.h                     # Envelope generator
│   │   ├── Wavetable.h               # Wavetable oscillator + frame interpolation
│   │   ├── ReverbDSP.h               # Shared reverb algorithm
│   │   ├── DCBlocker.h               # DC offset removal
│   │   ├── DenormalGuard.h           # Flush-to-zero RAII wrapper
│   │   └── SmoothParam.h             # SmoothedValue utilities
│   │
│   ├── Engines/
│   │   ├── Fat/                       # XObese engine — bold, industrial
│   │   │   ├── FatEngine.h/.cpp
│   │   │   ├── FatVoice.h
│   │   │   ├── FatOscillator.h
│   │   │   └── FatParameters.h
│   │   ├── Bite/                      # XOverbite engine — bass-forward, feral
│   │   │   ├── BiteEngine.h/.cpp
│   │   │   ├── BiteVoice.h
│   │   │   └── BiteParameters.h
│   │   ├── Snap/                      # OddfeliX — percussive
│   │   │   ├── SnapEngine.h/.cpp
│   │   │   ├── SnapVoice.h
│   │   │   └── SnapParameters.h
│   │   ├── Morph/                     # OddOscar — pad/lush
│   │   │   ├── MorphEngine.h/.cpp
│   │   │   ├── MorphVoice.h
│   │   │   └── MorphParameters.h
│   │   ├── Dub/                       # XOverdub engine — dub FX + performance
│   │   │   ├── DubEngine.h/.cpp
│   │   │   ├── DubVoice.h
│   │   │   └── DubParameters.h
│   │   ├── Drift/                     # XOdyssey engine — psychedelic pads
│   │   │   ├── DriftEngine.h/.cpp
│   │   │   ├── DriftVoice.h
│   │   │   └── DriftParameters.h
│   │   └── Onset/                     # XOnset engine — percussive synthesis (new)
│   │       ├── OnsetEngine.h/.cpp
│   │       ├── OnsetVoice.h
│   │       └── OnsetParameters.h
│   │
│   ├── Surface/
│   │   ├── PlaySurface.h/.cpp         # Main XY performance surface
│   │   ├── NoteInput.h/.cpp           # Keyboard/pad note handling
│   │   ├── OrbitPath.h/.cpp           # Orbital motion path generator
│   │   ├── PerfStrip.h/.cpp           # Performance strip (bend, mod, expression)
│   │   └── PerfPads.h/.cpp            # Performance pad triggers
│   │
│   ├── UI/
│   │   ├── XOceanusLookAndFeel.h/.cpp # Gallery shell look-and-feel
│   │   ├── ThemeManager.h/.cpp        # Light/dark mode, engine accent registration
│   │   ├── HeaderBar.h/.cpp           # Logo, preset nav, macro knobs
│   │   ├── BottomBar.h/.cpp           # Mood tabs, browser/DNA/sequencer toggle
│   │   ├── EnginePanel.h/.cpp         # Generic engine panel frame (accent border)
│   │   ├── EnginePanelLayout.h/.cpp   # 1/2/3 engine responsive layout
│   │   ├── CouplingView.h/.cpp        # Coupling strip visualization
│   │   ├── PresetBrowser.h/.cpp       # Card grid, mood tabs, search
│   │   ├── PresetCard.h/.cpp          # Single preset card with DNA sparkline
│   │   ├── DNARadarChart.h/.cpp       # Hexagonal radar for DNA detail
│   │   ├── MoodMap.h/.cpp             # 2D DNA-axis scatter of all presets (mood-tinted)
│   │   └── KnobComponents.h/.cpp      # Standard, Macro, Mini knob variants
│   │
│   ├── Export/
│   │   ├── XPNExporter.h/.cpp         # .xpn pack generation
│   │   └── XPMBuilder.h/.cpp          # .xpm program file generation
│   │
│   ├── PluginProcessor.h/.cpp         # Main processor: engine routing, APVTS, state
│   └── PluginEditor.h/.cpp            # Main editor: shell layout, engine panel hosting
│
├── Presets/
│   └── XOceanus/                      # .xometa files organized by mood
│       ├── Foundation/                # Terracotta — grounded, solid, rhythmic
│       ├── Atmosphere/                # Teal — ambient, floating, textural
│       ├── Entangled/                 # Gold — cross-engine, coupled, complex
│       ├── Prism/                     # Silver — bright, crystalline, harmonic
│       ├── Flux/                      # Crimson — aggressive, evolving, chaotic
│       └── Aether/                    # Indigo — deep, vast, otherworldly
│
├── Tools/
│   ├── compute_preset_dna.py          # Sonic DNA fingerprinting
│   ├── breed_presets.py               # Preset breeding/crossover
│   ├── extract_cpp_presets.py         # C++ preset extraction utility
│   ├── migrate_xocmeta_to_xometa.py  # OddfeliX/OddOscar format migration
│   ├── fix_xobese_xpms.py            # XPM format fixer
│   └── xpn_exporter/
│       └── xpn_export.py             # MPC .xpn pack builder
│
├── Docs/
│   ├── xoceanus_repo_structure.md     # This document
│   ├── xoceanus_technical_design_system.md
│   ├── xoceanus_brand_identity_and_launch.md
│   ├── xoceanus_preset_spec_for_builder.md
│   ├── xo_mega_tool_engine_catalog.md
│   ├── xo_mega_tool_chaining_architecture.md
│   ├── xo_mega_tool_preset_system.md
│   ├── xo_pad_surface_spec.md
│   ├── xo_performance_strip_spec.md
│   ├── xo_signature_playsurface_spec.md
│   ├── xonset_percussive_engine_spec.md
│   ├── xometa_schema.json
│   └── xometa_examples.json
│
├── Assets/
│   ├── icons/
│   │   ├── engines/                   # SVG engine icons (16/24/32px)
│   │   ├── ui/                        # SVG action/state icons (1.5px stroke)
│   │   └── mood/                      # SVG mood category icons
│   ├── fonts/
│   │   ├── Inter-Regular.ttf
│   │   ├── Inter-Medium.ttf
│   │   ├── Inter-SemiBold.ttf
│   │   ├── Inter-Black.ttf
│   │   ├── SpaceGrotesk-Bold.ttf
│   │   └── JetBrainsMono-Regular.ttf
│   └── images/
│       ├── logo-light.svg
│       ├── logo-dark.svg
│       └── splash.png
│
└── Tests/
    ├── DSP/
    │   ├── TestCytomicSVF.cpp         # Filter response, stability, denormals
    │   ├── TestPolyBLEP.cpp           # Aliasing, frequency accuracy
    │   ├── TestADSR.cpp               # Envelope shape, edge cases
    │   └── TestWavetable.cpp          # Frame interpolation, boundary conditions
    └── Integration/
        ├── TestEngineRegistry.cpp     # Engine load/unload, param namespacing
        ├── TestCouplingMatrix.cpp     # Cross-engine mod routing
        ├── TestPresetManager.cpp      # .xometa load/save round-trip
        └── TestSonicDNA.cpp           # DNA computation determinism
```

---

## 3. CMakeLists.txt Design

### Draft Top-Level CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.24)
project(XOceanus VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── Platform Detection ──────────────────────────────────────────────
if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
    set(XO_MOBILE ON)
    set(XO_PLATFORM "iOS")
    add_compile_definitions(XO_MOBILE=1)
else()
    set(XO_MOBILE OFF)
    set(XO_PLATFORM "macOS")
endif()

message(STATUS "XOceanus — Building for ${XO_PLATFORM}")

# ── JUCE ────────────────────────────────────────────────────────────
add_subdirectory(Libs/JUCE)

# ── Shared DSP Library ──────────────────────────────────────────────
add_library(XO_DSP STATIC
    Source/DSP/CytomicSVF.h
    Source/DSP/PolyBLEP.h
    Source/DSP/FastMath.h
    Source/DSP/ADSR.h
    Source/DSP/Wavetable.h
    Source/DSP/ReverbDSP.h
    Source/DSP/DCBlocker.h
    Source/DSP/DenormalGuard.h
    Source/DSP/SmoothParam.h
)
# Header-only DSP — provide an empty .cpp if needed for CMake
target_include_directories(XO_DSP PUBLIC Source/DSP)
set_target_properties(XO_DSP PROPERTIES LINKER_LANGUAGE CXX)

# ── Core Library ────────────────────────────────────────────────────
add_library(XO_Core STATIC
    Source/Core/EngineRegistry.cpp
    Source/Core/CouplingMatrix.cpp
    Source/Core/SharedTransport.cpp
    Source/Core/PresetManager.cpp
    Source/Core/SonicDNA.cpp
)
target_include_directories(XO_Core PUBLIC Source/Core)
target_link_libraries(XO_Core PUBLIC XO_DSP juce::juce_core juce::juce_data_structures)

# ── Engine Libraries (one per engine) ───────────────────────────────
set(XO_ENGINES Fat Bite Snap Morph Dub Drift Onset)

foreach(ENGINE ${XO_ENGINES})
    string(TOLOWER ${ENGINE} ENGINE_LOWER)
    add_library(XO_${ENGINE} STATIC
        Source/Engines/${ENGINE}/${ENGINE}Engine.cpp
    )
    target_include_directories(XO_${ENGINE} PUBLIC Source/Engines/${ENGINE})
    target_link_libraries(XO_${ENGINE} PUBLIC XO_Core XO_DSP)
endforeach()

# ── Surface Library ─────────────────────────────────────────────────
add_library(XO_Surface STATIC
    Source/Surface/PlaySurface.cpp
    Source/Surface/NoteInput.cpp
    Source/Surface/OrbitPath.cpp
    Source/Surface/PerfStrip.cpp
    Source/Surface/PerfPads.cpp
)
target_include_directories(XO_Surface PUBLIC Source/Surface)
target_link_libraries(XO_Surface PUBLIC XO_Core juce::juce_gui_basics)

# ── UI Library ──────────────────────────────────────────────────────
add_library(XO_UI STATIC
    Source/UI/XOceanusLookAndFeel.cpp
    Source/UI/ThemeManager.cpp
    Source/UI/HeaderBar.cpp
    Source/UI/BottomBar.cpp
    Source/UI/EnginePanel.cpp
    Source/UI/EnginePanelLayout.cpp
    Source/UI/CouplingView.cpp
    Source/UI/PresetBrowser.cpp
    Source/UI/PresetCard.cpp
    Source/UI/DNARadarChart.cpp
    Source/UI/MoodMap.cpp
    Source/UI/KnobComponents.cpp
)
target_include_directories(XO_UI PUBLIC Source/UI)
target_link_libraries(XO_UI PUBLIC XO_Core XO_Surface juce::juce_gui_basics juce::juce_gui_extra)

# ── Export Library ──────────────────────────────────────────────────
add_library(XO_Export STATIC
    Source/Export/XPNExporter.cpp
    Source/Export/XPMBuilder.cpp
)
target_include_directories(XO_Export PUBLIC Source/Export)
target_link_libraries(XO_Export PUBLIC XO_Core juce::juce_core)

# ── Plugin Target ───────────────────────────────────────────────────
if(XO_MOBILE)
    # iOS: AUv3 + Standalone
    juce_add_plugin(XOceanus
        PRODUCT_NAME "XOceanus"
        COMPANY_NAME "XO_OX Designs"
        BUNDLE_ID "com.xo-ox.xoceanus"
        PLUGIN_MANUFACTURER_CODE XoOx
        PLUGIN_CODE Xocn
        FORMATS AUv3 Standalone
        IS_SYNTH TRUE
        NEEDS_MIDI_INPUT TRUE
        IS_MIDI_EFFECT FALSE
    )
else()
    # macOS: AU + VST3 + Standalone
    juce_add_plugin(XOceanus
        PRODUCT_NAME "XOceanus"
        COMPANY_NAME "XO_OX Designs"
        BUNDLE_ID "com.xo-ox.xoceanus"
        PLUGIN_MANUFACTURER_CODE XoOx
        PLUGIN_CODE Xocn
        FORMATS AU VST3 Standalone
        IS_SYNTH TRUE
        NEEDS_MIDI_INPUT TRUE
        IS_MIDI_EFFECT FALSE
    )
endif()

target_sources(XOceanus PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
)

target_link_libraries(XOceanus PRIVATE
    XO_Core
    XO_DSP
    XO_Fat
    XO_Bite
    XO_Snap
    XO_Morph
    XO_Dub
    XO_Drift
    XO_Onset
    XO_Surface
    XO_UI
    XO_Export
    juce::juce_audio_basics
    juce::juce_audio_devices
    juce::juce_audio_formats
    juce::juce_audio_plugin_client
    juce::juce_audio_processors
    juce::juce_audio_utils
    juce::juce_core
    juce::juce_data_structures
    juce::juce_graphics
    juce::juce_gui_basics
    juce::juce_gui_extra
)

target_compile_definitions(XOceanus PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
)

# ── Binary Assets (fonts, icons) ────────────────────────────────────
juce_add_binary_data(XOceanusAssets SOURCES
    Assets/fonts/Inter-Regular.ttf
    Assets/fonts/Inter-Medium.ttf
    Assets/fonts/Inter-SemiBold.ttf
    Assets/fonts/SpaceGrotesk-Bold.ttf
    Assets/fonts/JetBrainsMono-Regular.ttf
    Assets/images/logo-light.svg
    Assets/images/logo-dark.svg
)
target_link_libraries(XOceanus PRIVATE XOceanusAssets)

# ── Tests (optional, gated behind BUILD_TESTING) ────────────────────
option(XO_BUILD_TESTS "Build XOceanus test suite" OFF)
if(XO_BUILD_TESTS)
    enable_testing()
    add_subdirectory(Tests)
endif()
```

### Build Commands

**macOS (AU + VST3 + Standalone):**
```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release && cmake --build build
```

**iOS (AUv3 + Standalone):**
```bash
cmake -B build-ios -G Xcode -DCMAKE_TOOLCHAIN_FILE=ios.cmake
```

**With tests:**
```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DXO_BUILD_TESTS=ON && cmake --build build && ctest --test-dir build
```

---

## 4. CLAUDE.md Template

The following is the complete `CLAUDE.md` to be placed at the repo root (`XOceanus/CLAUDE.md`):

````markdown
# XOceanus — Claude Code Project Guide

## Product Identity

XOceanus ("for all") is the collected works of **XO_OX Designs** — 7 synth engines
unified in a gallery-model instrument. Each engine is a distinct sonic exhibition; the
platform provides a clean neutral shell, shared DSP, coupling between engines, and a
unified preset system.

**Framework:** JUCE 8.0.12, C++17, CMake + Ninja
**Platforms:** macOS (AU + VST3 + Standalone), iOS/iPadOS (AUv3 + Standalone)

## The 7 Engines

| Engine | Module Dir | Origin | Identity |
|--------|-----------|--------|----------|
| Fat | Source/Engines/Obese/ | XObese | Bold, industrial, high-contrast |
| Bite | Source/Engines/Overbite/ | XOverbite | Bass-forward, plush weight + feral bite |
| Snap | Source/Engines/OddfeliX/ | OddfeliX | Percussive (PolyBLEP, FM, Karplus-Strong) |
| Morph | Source/Engines/OddOscar/ | OddOscar | Pad/lush (wavetable morph, ladder filter) |
| Dub | Source/Engines/Overdub/ | XOverdub | Dub-focused synth + performance FX |
| Drift | Source/Engines/Odyssey/ | XOdyssey | Psychedelic pads, Climax system, journey |
| Onset | Source/Engines/Onset/ | New build | Percussive synthesis, transient design |

## Architecture Rules

- **Never** allocate memory on the audio thread
- **Never** perform blocking I/O on the audio thread
- **Never** rename stable parameter IDs after release
- All DSP lives inline in `.h` headers; `.cpp` files are one-line stubs
- DSP modules must be testable in isolation
- Export systems must run on non-audio worker threads
- Denormal protection required in all feedback/filter paths
- Engine accent colors and XO Gold never change between light/dark modes

## Key File Paths

| Path | Purpose |
|------|---------|
| Source/PluginProcessor.h/.cpp | Main processor: engine routing, APVTS, state |
| Source/PluginEditor.h/.cpp | Main editor: gallery shell, engine panel hosting |
| Source/Core/SynthEngine.h | Abstract engine interface |
| Source/Core/EngineRegistry.h | Runtime engine discovery + instantiation |
| Source/Core/CouplingMatrix.h | Cross-engine modulation routing |
| Source/Core/PresetManager.h | .xometa load/save, mood categorization |
| Source/Core/SonicDNA.h | DNA vector computation + comparison |
| Source/DSP/ | Shared DSP: CytomicSVF, PolyBLEP, FastMath, ADSR, etc. |
| Source/Engines/{Name}/ | One directory per engine module |
| Source/Surface/ | PlaySurface, NoteInput, OrbitPath, PerfStrip, PerfPads |
| Source/UI/ | Gallery shell: LookAndFeel, panels, browser, coupling view |
| Source/Export/ | XPN/XPM pipeline for MPC export |
| Presets/XOceanus/ | .xometa files by mood (6 categories) |
| Tools/ | Python utilities (DNA, breeding, migration, export) |
| Assets/ | Icons (SVG), fonts (TTF), images |

## Parameter Namespacing

Every parameter ID is prefixed with its engine name to prevent collisions:

```
fat_osc1_shape      # Fat engine oscillator 1 shape
poss_filterCutoff   # Overbite engine filter cutoff
snap_attack         # Snap engine attack
morph_wavetable_pos # Morph engine wavetable position
dub_delay_time      # Dub engine delay time
drift_journey       # Drift engine journey macro
onset_transient     # Onset engine transient amount
```

Global parameters (coupling, macros, master) use the `global_` prefix:

```
global_macro1       # Macro knob 1
global_coupling_ab  # Coupling amount between active pair
global_master_gain  # Master output gain
```

## Build Commands

```bash
# macOS (AU + VST3 + Standalone)
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release && cmake --build build

# iOS (AUv3 + Standalone)
cmake -B build-ios -G Xcode -DCMAKE_TOOLCHAIN_FILE=ios.cmake

# With tests
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DXO_BUILD_TESTS=ON && cmake --build build && ctest --test-dir build

# auval validation (macOS, after build)
auval -a | grep XOceanus
```

## Preset System

- **Format:** `.xometa` JSON (version-controlled, human-readable)
- **Location:** `Presets/XOceanus/{mood}/`
- **6 moods:** Foundation, Atmosphere, Entangled, Prism, Flux, Aether
- **Sonic DNA:** 6-dimensional vector (Brightness, Warmth, Movement, Density, Space, Aggression)
- **Target:** 1000 presets (519 already migrated)
- **Multi-engine presets** store parameter maps for 1-3 engines + coupling settings
- **Tools:** `compute_preset_dna.py` for fingerprinting, `breed_presets.py` for crossover

## Design System Reference

See `Docs/xoceanus_technical_design_system.md` for the full visual specification:
- Gallery model: warm white shell (#F8F6F3), engine-specific accent panels
- XO Gold (#E9C46A) for brand constants (macro knobs, active states, coupling)
- Light mode default, dark mode toggle
- Typography: Inter (body), Space Grotesk (display), JetBrains Mono (values)
- Component library: Standard/Macro/Mini knobs, toggle/action buttons, preset cards

## Development Workflow

1. Read the relevant engine spec before modifying any engine
2. Plan architecture changes before coding
3. Run DSP stability checks after engine modifications
4. All new parameters must follow the namespacing convention
5. Preserve existing parameter IDs and preset compatibility
6. Test coupling interactions when modifying engine outputs
7. Verify both light and dark mode after UI changes
````

---

## 5. Migration Plan

### Phase 1: Scaffold

**Goal:** Empty repo that compiles.

| Step | Action | Verify |
|------|--------|--------|
| 1.1 | Create `~/Documents/GitHub/XOceanus/` repo on GitHub | `git clone` succeeds |
| 1.2 | Create full directory structure (all dirs from Section 2) | `ls -R` matches tree |
| 1.3 | Clone JUCE 8.0.12 into `Libs/JUCE/` | `Libs/JUCE/CMakeLists.txt` exists |
| 1.4 | Write `CMakeLists.txt` (Section 3 draft) | — |
| 1.5 | Write `CLAUDE.md` (Section 4 draft) | — |
| 1.6 | Copy shared DSP headers into `Source/DSP/` | Files present |
| 1.7 | Create stub `SynthEngine.h` interface in `Source/Core/` | — |
| 1.8 | Create stub `PluginProcessor.h/.cpp` and `PluginEditor.h/.cpp` | — |
| 1.9 | Create placeholder `.cpp` stubs for all library targets | — |
| 1.10 | `cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release && cmake --build build` | Compiles with 0 errors |

**DSP modules to copy from existing repos into `Source/DSP/`:**

| Module | Source Location |
|--------|----------------|
| CytomicSVF.h | OddfeliX/OddOscar `Source/DSP/Filters.h` (extract SVF) |
| PolyBLEP.h | OddfeliX/OddOscar `Source/DSP/Oscillators.h` (extract PolyBLEP) |
| FastMath.h | XOverdub `src/` or XOdyssey `Source/DSP/` |
| ADSR.h | OddfeliX/OddOscar `Source/DSP/` or XOdyssey `Source/DSP/` |
| Wavetable.h | XOdyssey `Source/DSP/` |
| ReverbDSP.h | XOverdub `src/` (spring reverb) or XOdyssey `Source/DSP/` |
| DCBlocker.h | XOverdub `src/` |

### Phase 2: Engine Migration

Migrate one engine at a time. For each engine, follow this sequence:

1. **Copy** source files into `Source/Engines/{Name}/`
2. **Rename** main processor class to `{Name}Engine` (e.g., `PluginProcessor` becomes `FatEngine`)
3. **Wrap** in `SynthEngine` interface using the adapter pattern — the engine's `processBlock` becomes the implementation of `SynthEngine::render()`
4. **Prefix** all parameter IDs with the engine name (e.g., `osc1_shape` becomes `fat_osc1_shape`)
5. **Register** in `EngineRegistry` with engine metadata (name, accent color, icon path)
6. **Build** — verify the project compiles
7. **Validate** — run auval for standalone engine test

**Priority order and rationale:**

| Priority | Engine(s) | Source Repo | Rationale |
|----------|-----------|-------------|-----------|
| 1 | Snap + Morph | OddfeliX/OddOscar | Already coupled, coupling logic ports directly |
| 2 | Dub | XOverdub | Simplest standalone engine, good early validation |
| 3 | Fat | XObese | Build complete, straightforward migration |
| 4 | Bite | XOverbite | Phase 7, may need minor completion |
| 5 | Drift | XOdyssey | Most complex engine (Climax system, journey macro) |
| 6 | Onset | Spec only | New build from XOnset spec — no migration, fresh code |

**Source mapping:**

| Engine | Source Repo | Source Dir | Key Classes to Rename |
|--------|-------------|-----------|----------------------|
| Fat | XObese | `Source/` | PluginProcessor -> FatEngine |
| Bite | XOverbite | `Source/` | PluginProcessor -> BiteEngine |
| Snap | OddfeliX/OddOscar | `Source/Engines/EngineX.h` | EngineX -> SnapEngine |
| Morph | OddfeliX/OddOscar | `Source/Engines/EngineO.h` | EngineO -> MorphEngine |
| Dub | XOverdub | `src/` | PluginProcessor -> DubEngine |
| Drift | XOdyssey | `Source/` | PluginProcessor -> DriftEngine |
| Onset | N/A | N/A | New build from spec |

### Phase 3: Shared Systems

After at least 2 engines are migrated and building:

| Step | System | Description |
|------|--------|-------------|
| 3.1 | PlaySurface | Port XOblong's PlaySurface into `Source/Surface/`, generalize for multi-engine use |
| 3.2 | PresetManager | Implement `.xometa` JSON loading/saving with mood categorization in `Source/Core/` |
| 3.3 | PresetBrowser | Build card grid UI with mood tabs, search, DNA sparklines in `Source/UI/` |
| 3.4 | XOceanusLookAndFeel | Implement gallery shell (warm white frame, per-engine accent panels, light/dark toggle) per design system |
| 3.5 | CouplingMatrix | Port from OddfeliX/OddOscar, generalize to arbitrary engine pairs |
| 3.6 | SharedTransport | Global BPM, sync, clock shared across all engines |
| 3.7 | SonicDNA | Port `compute_preset_dna.py` logic into C++ for real-time DNA display |
| 3.8 | ThemeManager | Light/dark mode toggle, engine accent color registration |

### Phase 4: Integration

After all engines and shared systems are in place:

| Step | Task | Description |
|------|------|-------------|
| 4.1 | MegaToolProcessor | Wire `PluginProcessor` to route MIDI + audio through `EngineRegistry`, support 1-3 active engines |
| 4.2 | Coupling wiring | Connect `CouplingMatrix` outputs to engine inputs for all valid engine pairs |
| 4.3 | Multi-engine presets | `PresetManager` loads/saves parameter maps for multiple engines + coupling state in a single `.xometa` |
| 4.4 | Responsive layout | `EnginePanelLayout` handles 1/2/3 engine display with correct panel sizing |
| 4.5 | XPN export pipeline | Wire `Source/Export/` to `PresetManager` for MPC expansion generation |
| 4.6 | Macro routing | 4 macro knobs in `HeaderBar` with per-preset routing to engine parameters |
| 4.7 | Full-chain auval | Validate AU with all engines loaded, coupling active, preset switching |

### Phase 5: Preset Library

| Step | Task | Details |
|------|------|---------|
| 5.1 | Import migrated presets | Copy 519 `.xometa` files into `Presets/XOceanus/` by mood |
| 5.2 | Parameter ID remapping | Update all parameter IDs in imported presets to use new engine-prefixed names |
| 5.3 | Validation pass | Load every imported preset, verify no missing parameters, no silent failures |
| 5.4 | DNA computation | Run `compute_preset_dna.py` on all 519 presets, embed DNA vectors |
| 5.5 | Create new presets | Build 481 new presets to reach the 1000 target, emphasizing multi-engine Entangled presets |
| 5.6 | Quality pass | Listen to every preset, verify sonic intent matches mood category |
| 5.7 | Mood balance audit | Ensure roughly even distribution across 6 moods (150-200 per mood) |

---

## 6. Archive Plan for Existing Repos

After migration is complete and XOceanus is building with all engines validated:

| Step | Action |
|------|--------|
| 6.1 | Tag each repo with `v1.0-standalone-archive` |
| 6.2 | Add archive notice to each repo's README: *"This project has been merged into [XOceanus](https://github.com/xo-ox-designs/XOceanus). This repo is archived as a historical reference."* |
| 6.3 | Set each repo to archived/read-only on GitHub (Settings > Archive) |
| 6.4 | Do NOT delete any repos — they serve as historical reference for design decisions, commit history, and issue discussions |

**Repos to archive:**

| Repo | Location | Archive Tag |
|------|----------|-------------|
| XObese | ~/Documents/GitHub/XObese/ | `v1.0-standalone-archive` |
| XOverbite | ~/Documents/GitHub/XOverbite/ | `v1.0-standalone-archive` |
| OddfeliX/OddOscar | ~/Documents/GitHub/OddfeliX/OddOscar/ | `v1.0-standalone-archive` |
| XOverdub | ~/Documents/GitHub/XOverdub/ | `v1.0-standalone-archive` |
| XOdyssey | ~/Documents/GitHub/XOdyssey/ | `v1.0-standalone-archive` |
| XOblong | ~/Documents/GitHub/XOblong/ | `v1.0-standalone-archive` |

Note: XOnset has no existing repo (spec only, lives in OddfeliX/OddOscar docs) — no archive needed.

---

## 7. CI/CD (Future)

### GitHub Actions Pipeline

```yaml
# .github/workflows/build.yml
name: XOceanus Build
on:
  push:
    branches: [main, develop]
  pull_request:
    branches: [main]

jobs:
  build-macos:
    runs-on: macos-14
    steps:
      - uses: actions/checkout@v4
        with:
          submodules: false
      - name: Clone JUCE
        run: git clone --depth 1 --branch 8.0.12 https://github.com/juce-framework/JUCE.git Libs/JUCE
      - name: Build
        run: cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release && cmake --build build
      - name: auval
        run: auval -v aumu Xocn XoOx

  build-ios:
    runs-on: macos-14
    steps:
      - uses: actions/checkout@v4
      - name: Clone JUCE
        run: git clone --depth 1 --branch 8.0.12 https://github.com/juce-framework/JUCE.git Libs/JUCE
      - name: Build iOS
        run: cmake -B build-ios -G Xcode -DCMAKE_TOOLCHAIN_FILE=ios.cmake && cmake --build build-ios --config Release

  test:
    runs-on: macos-14
    steps:
      - uses: actions/checkout@v4
      - name: Clone JUCE
        run: git clone --depth 1 --branch 8.0.12 https://github.com/juce-framework/JUCE.git Libs/JUCE
      - name: Build + Test
        run: |
          cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DXO_BUILD_TESTS=ON
          cmake --build build
          ctest --test-dir build --output-on-failure

  validate-presets:
    runs-on: macos-14
    steps:
      - uses: actions/checkout@v4
      - name: Schema validation
        run: python3 Tools/validate_presets.py Presets/XOceanus/
      - name: DNA completeness check
        run: python3 Tools/compute_preset_dna.py --check Presets/XOceanus/
```

### CI Checklist

| Check | Gate | Description |
|-------|------|-------------|
| macOS build | PR merge | AU + VST3 + Standalone compile with 0 errors, 0 warnings |
| iOS build | PR merge | AUv3 + Standalone compile with 0 errors |
| auval | PR merge | AU validation passes |
| DSP unit tests | PR merge | All Tests/DSP/ tests pass |
| Integration tests | PR merge | All Tests/Integration/ tests pass |
| Preset schema | PR merge | All .xometa files validate against schema |
| DNA completeness | PR merge | Every preset has a computed DNA vector |
| No parameter ID changes | PR merge (release branches) | Diff check that no existing param IDs were renamed |

---

*This document is the source of truth for XOceanus repo structure and migration. Update it as decisions evolve.*
