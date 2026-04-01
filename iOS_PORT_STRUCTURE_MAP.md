================================================================================
XOMNIBUS/XOLOKUN iOS PORT DIRECTORY STRUCTURE MAP
================================================================================
Repository: ~/Documents/GitHub/XO_OX-XOmnibus
Rename Status: XOmnibus → XOlokun (2026-03-24)
Last Updated: 2026-03-25

================================================================================
1. TOP-LEVEL STRUCTURE
================================================================================

Root Directories:
├── Source/              (11 subdirectories, 356 total files: 297 .h + 59 .cpp)
├── Docs/                (245 items, extensive specification library)
├── Presets/             (Factory presets by mood)
├── Tools/               (Python utilities: DNA, breeding, migration, export)
├── Build/               (CMake build outputs)
├── SDK/                 (JUCE-free SDK headers for third-party engines)
├── Skills/              (Claude Code skill guides)
├── Tests/               (Test suite)
├── CMakeLists.txt       (CMake build configuration)
├── CLAUDE.md            (Project guide, THIS FILE IS AUTHORITATIVE)
├── README.md            (Main documentation)
└── LICENSE              (XO_OX licensing)

================================================================================
2. SOURCE DIRECTORY STRUCTURE (Detailed Map)
================================================================================

Source/
├── AI/                           (AI utilities)
│   ├── CommunityInsights.h       ✓ iOS conditional: #if JUCE_MAC || JUCE_IOS
│   └── (other AI modules)
│
├── Core/                         (Core synthesis & state management)
│   ├── SynthEngine.h             (Base engine interface)
│   ├── EngineRegistry.h          (Factory + 4-slot management)
│   ├── MegaCouplingMatrix.h      (15 coupling types)
│   ├── PresetManager.h           (.xometa loading/saving)
│   ├── MIDILearnManager.h        (Cross-platform MIDI mapping)
│   ├── SharedTransport.h         (Transport sync)
│   ├── MacroSystem.h             (4-macro coordination)
│   ├── PresetMorpher.h           (Preset interpolation)
│   ├── StructuralUndoManager.h   (Undo/redo)
│   ├── ChordMachine.h            (Chord generation)
│   ├── AudioRingBuffer.h         (Lock-free ring buffer)
│   ├── MPEManager.h              (MPE support)
│   ├── PageRSequencer.h          (Sequencer)
│   ├── PinMatrixRandomizer.h     (Randomization)
│   ├── RecipeEngine.h            (Recipe system)
│   ├── FamilyConstellationCoupling.h
│   ├── CouplingPresetManager.h
│   ├── CouplingCrossfader.h
│   ├── MasterFXSequencer.h
│   └── ShaperRegistry.h
│
├── DSP/                          (Core DSP library)
│   ├── Effects/                  (FX chains)
│   │   ├── AquaticFXSuite.h      (Reef, Fathom, Drift, Tide)
│   │   ├── MathFXChain.h         (Entropy, Voronoi, Quantum, Attractor)
│   │   ├── BoutiqueFXChain.h     (Anomaly, Archive, Cathedral, Submersion)
│   │   ├── fXOnslaught.h         (Singularity: transient chorus → PM collapse)
│   │   ├── fXObscura.h           (Singularity: spectral degradation)
│   │   └── fXOratory.h           (Singularity: poetic delay)
│   ├── ShoreSystem/              (5-coastline shared data system)
│   ├── SRO/                      (Spectral Resonance Objects — CPU optimization)
│   └── (core DSP modules)
│
├── Engines/                      (76 total engines)
│   ├── Bite, Bob, Drift, Dub, Fat, Morph (legacy, pre-O-word era)
│   ├── Oaken, Oasis, Obbligato, Obelisk, Oblique, Obrix (O-word generation)
│   ├── Obscura, Obsidian, OceanDeep, Oceanic, Ocelot, Ochre (continuing)
│   ├── Octave, Octopus, Oddfellow, Offering, Ogre, Ohm (continuing)
│   ├── Olate, Ole, Oleg, Ombre, Omega, Onkolo (continuing)
│   ├── Onset, Opal, Opaline, Opcode, OpenSky, Opera (continuing)
│   ├── Optic, Oracle, Orbital, Orbweave, Orca, Orchard (continuing)
│   ├── Organism, Organon, Origami, Orphica, Osier, Osmosis (continuing)
│   ├── Osprey, Osteria, Ostinato, Otis, Oto, Ottoni (continuing)
│   ├── Ouie, Ouroboros, Outlook, Outwit, Oven, Overgrow (continuing)
│   ├── Overlap, Overflow, Overcast, Overwash, Overworn (continuing)
│   ├── Overworld, Oxalis, Oxbow, Oxytocin, Oware (continuing)
│   ├── Owlfish, Outwit/DSP, Overlap/DSP (special DSP subdirs)
│   └── (Each engine directory contains *.h/.cpp + Presets/ subdir)
│
├── Export/                       (Format export systems)
│   ├── XDrip.h                   ✓ iOS conditional: #if JUCE_MAC || JUCE_IOS
│   └── (XPN, preset export pipelines)
│
├── Shapers/                      (Modulation shapers)
│   ├── Observe/                  (XObserve shaper)
│   └── Oxide/                    (XOxide shaper)
│
└── UI/                           (JUCE-based UI)
    ├── Mobile/                   ✓✓ iPhone/iPad-SPECIFIC UI (NEW SUBSYSTEM)
    │   ├── SensorManager.h       (Accelerometer, gyroscope; Obj-C++ bridge notes)
    │   ├── ParameterDrawer.h     (iPhone bottom drawer for parameters)
    │   ├── MobileTouchHandler.h  (Multi-touch abstraction; UITouch → TouchEvent)
    │   ├── MobilePlaySurface.h   (Touch-optimized PlaySurface; carousel + drawer)
    │   ├── MobileLayoutManager.h (Adaptive layout: 5 modes based on width/height)
    │   │   ├── iPhonePortrait    (width < 430pt)
    │   │   ├── iPhoneLandscape   (width ≥ 430pt && height < 430pt)
    │   │   ├── iPadCompact       (width ≥ 430pt && width < 600pt, Split View 33%)
    │   │   ├── iPadRegular       (width ≥ 600pt && width < 1024pt)
    │   │   └── iPadFull          (width ≥ 1024pt; also Desktop with XO_MOBILE=1)
    │   ├── HapticEngine.h        (Taptic feedback; #if XO_MOBILE guards)
    │   └── CPUMonitor.h          (Real-time CPU tracking; device class 0/1/2)
    │
    ├── PlaySurface/              (Desktop/shared PlaySurface)
    ├── Gallery/                  (Gallery Model UI framework)
    ├── GalleryColors.h           ✓ iOS conditional + mobile parameter
    ├── PresetBrowser/            (Preset UI)
    ├── CouplingStrip/            (Coupling UI)
    ├── CouplingVisualizer/       (Coupling visualization)
    ├── OpticVisualizer/          (Winamp-style visualizer)
    ├── ExportDialog/             (Export UI)
    ├── OddOscar/                 (Character mascot UI)
    └── (other UI components)

================================================================================
3. iOS/AUv3 ARCHITECTURE (CONDITIONAL COMPILATION RESULTS)
================================================================================

Files with iOS/Mobile Conditional Compilation:
1. /Source/UI/GalleryColors.h
   - #if JUCE_MAC || JUCE_IOS
   - Function: meetsMinTargetSize(const juce::Component& comp, bool mobile = false)
   - Mobile minimum size: 44pt (Apple HIG), Desktop: 24pt

2. /Source/UI/Mobile/SensorManager.h
   - Comment: "On iOS: wraps CMMotionManager via Objective-C++ bridge"
   - Purpose: Accelerometer/gyroscope input

3. /Source/UI/Mobile/ParameterDrawer.h
   - Comment: "ParameterDrawer — iPhone bottom drawer for engine parameters"

4. /Source/UI/Mobile/MobileTouchHandler.h
   - Comment: "Converts JUCE mouse events (which on iOS carry touch information) into ..."
   - Pressure handling: "JUCE exposes pressure from iOS UITouch.force"
   - Multi-touch: "currency of the mobile input pipeline"

5. /Source/UI/Mobile/CPUMonitor.h
   - Comment: "Base multiplier from engine count (per mobile spec)"
   - Device classes: 0=iPhone, 1=iPad, 2=iPad Pro (M-series)

6. /Source/UI/Mobile/MobileLayoutManager.h
   - Comment: "This means an AUv3 in a narrow host panel gets iPhone layout even on iPad"
   - Layout modes: 5 breakpoints (iPhonePortrait, iPhoneLandscape, iPadCompact, iPadRegular, iPadFull)

7. /Source/UI/Mobile/HapticEngine.h
   - #if XO_MOBILE (7 uses within the file)
   - Taptic feedback system (platform-aware)

8. /Source/AI/CommunityInsights.h
   - #if JUCE_MAC || JUCE_IOS

9. /Source/Export/XDrip.h
   - #if JUCE_MAC || JUCE_IOS

================================================================================
4. ENGINE ROSTER (76 Total)
================================================================================

Early Generations (Pre-O naming):
  Bite, Bob, Drift, Dub, Fat, Morph (6 total)

Constellation Family (B042 added 2026-03-14):
  OVERTONE, KNOT, ORGANISM, ORBWEAVE (4 total, integrated 2026-03-14/15)

Kitchen Collection (24 total across 6 quads):
  Chef Quad (Organs):      OTO, OCTAVE, OLEG, OTIS
  Kitchen Quad (Pianos):   OVEN, OCHRE, OBELISK, OPALINE
  Cellar Quad (Bass):      OGRE, OLATE, OAKEN, OMEGA
  Garden Quad (Strings):   ORCHARD, OVERGROW, OSIER, OXALIS
  Broth Quad (Pads):       OVERWASH, OVERWORN, OVERFLOW, OVERCAST
  Fusion Quad (EP):        OASIS, ODDFELLOW, ONKOLO, OPCODE

Open Source Engines (76 total - Fleet):
  ODDFELIX, ODDOSCAR (2), OVERDUB, ODYSSEY, OBLONG, OBESE, ONSET, OVERWORLD,
  OPAL, ORBITAL, ORGANON, OUROBOROS, OBSIDIAN, OVERBITE, ORIGAMI, ORACLE,
  OBSCURA, OCEANIC, OCELOT, OPTIC, OBLIQUE, OSPREY, OSTERIA, OWLFISH,
  OHM, ORPHICA, OBBLIGATO, OTTONI, OLE, OVERLAP, OUTWIT, OMBRE, ORCA,
  OCTOPUS, OSTINATO, OPENSKY, OCEANDEEP, OUIE, OBRIX, ORBWEAVE, OVERTONE,
  ORGANISM, OXBOW, OWARE, OPERA, OFFERING, OSMOSIS, OXYTOCIN, OUTLOOK
  (+ 24 Kitchen Collection + OBIONT + OSIER + OVERFLOW = 76 total)

Status Summary:
  - 72/76 seanced (OSMOSIS = design-only, not built)
  - 6 engines at 9.0+ (OXYTO 9.5, OVERBITE 9.2, OWARE 9.2, OBSCURA 9.1,
                       OUROBOROS 9.0, OXBOW 9.0)
  - Fleet average: ~8.8/10 (post 2026-03-22 DSP fixes)

================================================================================
5. DOCUMENTATION INDEX — iOS/MOBILE RELEVANT
================================================================================

Mobile & Layout:
  ✓ /Docs/xomnibus_mobile_and_midi_spec.md (Spec complete; Version 1.0, 2026-03-08)
  ✓ /Docs/xomnibus_mobile_implementation_strategy.md (Version 1.0, 2026-03-13)
  ✓ /Docs/design/xolokun-definitive-ui-spec.md
  ✓ /Docs/design/xolokun-spatial-architecture-v1.md
  ✓ /Docs/design/xolokun-wiring-manifest.md
  ✓ /Docs/design/xolokun-ui-blessing-session.md

Design System:
  ✓ /Docs/design/xomnibus_design_guidelines.md
  ✓ /Docs/design/xomnibus_ui_master_spec_v2.md
  ✓ /Docs/design/playsurface-design-spec.md

Other Relevant:
  ✓ /Docs/xomnibus_technical_design_system.md (Gallery Model, color, typography)
  ✓ /Docs/ios-design-spec.md (mentioned in MEMORY.md, may be in user's Claude config)

================================================================================
6. CONDITIONAL COMPILATION FLAGS
================================================================================

Found Flags:
  - #if JUCE_MAC || JUCE_IOS          (macOS + iOS together)
  - #if XO_MOBILE                     (Mobile-specific, set at CMake time)
  - #if !XO_MOBILE                    (Desktop-only branches)

CMake Integration:
  - XO_MOBILE=1 enables mobile UI subsystem
  - Touch handling is first-class input pipeline (not mouse emulation)
  - Layout determined by width, not device type (AUv3 multitasking just works)

================================================================================
7. FILE COUNT SUMMARY
================================================================================

Total Source Files (Source/):
  - Header files (.h):     297
  - Implementation files (.cpp): 59
  - Total:                 356 files

By Subsystem:
  - Engines/:              ~170 files (76 engines × 2–3 files each)
  - UI/:                   ~80 files (including Mobile/ subsystem)
  - Core/:                 ~40 files
  - DSP/:                  ~30 files
  - Export/:               ~8 files
  - AI/:                   ~5 files
  - Shapers/:              ~3 files

================================================================================
8. MOBILE IMPLEMENTATION READINESS
================================================================================

COMPLETED (Ready to Build):
  ✓ MobileLayoutManager.h  - 5 layout breakpoints implemented
  ✓ MobileTouchHandler.h   - Multi-touch abstraction complete
  ✓ HapticEngine.h         - Taptic feedback system complete
  ✓ CPUMonitor.h           - Device-class CPU degradation complete
  ✓ Mobile UI spec         - Detailed spec in Docs/ (v1.0)
  ✓ Mobile strategy        - Phase 1 (Touch Foundation) documented
  ✓ MIDI integration       - MIDILearnManager supports cross-platform
  ✓ DSP (76 engines)       - All identical to macOS (no platform-specific DSP)

IN PROGRESS (Phase 1 — Touch Foundation):
  ✓ MobilePlaySurface.h    - Touch-optimized PlaySurface (27KB header)
  ✓ ParameterDrawer.h      - iPhone parameter drawer UI
  ⚠ SensorManager.h        - Accel/gyro support (Obj-C++ bridge notes present)

NOT YET STARTED:
  - iOS-specific Objective-C++ bridges (AVAudioSession, CoreMIDI, Motion)
  - AUv3 plugin wrapper (AU v3 specification compliance)
  - Standalone iOS app wrapper
  - MetalKit rendering backend (currently JUCE default, may need optimization)
  - TestFlight beta distribution packaging
  - App Store submission prep

================================================================================
9. KEY ARCHITECTURAL INSIGHTS FOR iOS PORT
================================================================================

Design Philosophy (from xomnibus_mobile_implementation_strategy.md):
  - Mobile is NOT a scaled-down desktop; it's a first-class instrument
  - Touch is superior to mouse for expressive performance
  - Same DSP on all platforms — only the UI/input changes
  - Progressive disclosure, not feature hiding
  - Latency budget: < 14ms touch-to-sound (5.8ms buffer default)

Touch Input Pipeline (Revolutionary):
  UITouch → MobileTouchHandler → TouchEvent → Zone dispatch
  (NOT a mouse emulation layer — native multi-touch support)

Layout Strategy (Device-Agnostic):
  Layout mode determined by available WIDTH, not device type
  Result: AUv3 in narrow DAW panel automatically gets iPhone layout on iPad
  Multitasking (Split View, Slide Over) just works

CPU Degradation Strategy (Priority Order):
  1. Reduce visual animation frame rate (60fps → 30fps)
  2. Halve mod matrix update rate
  3. Switch oversampling from 2x → 1x
  4. Simplify FX (shorter reverb, linear interpolation)
  5. Reduce voice count (5ms fade on oldest note)
  [NEVER reduce audio sample rate or bit depth]

Haptic Strategy:
  Pre-prepared haptic patterns with < 16ms trigger latency
  Integrates with performance pads (FIRE, XOSEND, ECHO CUT, PANIC)

================================================================================
10. NEXT STEPS FOR iOS DEVELOPER
================================================================================

Phase 1 (Touch Foundation — Current):
  1. Set up iOS toolchain (XCode, iOS 16+ SDK, JUCE 8 iOS target)
  2. Wire CMake iOS cross-compilation
  3. Implement AVAudioSession bridge (sample rate, buffer size, categories)
  4. Test MobileTouchHandler with physical UITouch events
  5. Verify MobileLayoutManager breakpoints across device sizes
  6. Create TestFlight beta (unsigned, for team testing)

Phase 2 (AUv3 Plugin — Next):
  1. Implement AUv3 wrapper using JUCE's built-in AU support
  2. Wire CoreMIDI input (MIDI learn, MPE, sustained notes)
  3. Validate state persistence across suspend/resume
  4. Test in DAW (GarageBand, Ableton Link via AUv3)

Phase 3 (Standalone App — Later):
  1. Implement custom app lifecycle (foreground/background audio)
  2. Wire CoreAudio device selection (Bluetooth, USB, built-in)
  3. Add preset import/export (AirDrop, Files app)
  4. Optimize MetalKit rendering for sustained 60fps

================================================================================
