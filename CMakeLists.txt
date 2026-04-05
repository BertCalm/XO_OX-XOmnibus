cmake_minimum_required(VERSION 3.22)

# Universal binary — must be set BEFORE project() (closes #167)
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Universal binary")

project(XOceanus VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Security hardening and warning-as-error flags.
# -Werror is enabled so that warnings cannot accumulate silently in CI.
# Exceptions:
#   -Wno-error=unused-parameter  — JUCE virtual callback overrides routinely
#                                   omit parameters by design; not a code bug.
#   -Wno-error=deprecated-declarations — JUCE itself uses deprecated APIs on
#                                         some macOS versions; suppress until
#                                         the JUCE version bump resolves them.
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    add_compile_options(
        -Wall -Wextra -Wpedantic
        -Werror
        -Wno-error=unused-parameter
        -Wno-error=deprecated-declarations
        -Wno-error=unused-but-set-variable  # JUCE 8 internals set-but-don't-read in lambda conditions
        -Wno-error=macro-redefined          # _FORTIFY_SOURCE conflicts with sanitizer built-ins
        -Wno-error=shorten-64-to-32         # Xcode default; safe size_t→int in bounded loops
        -fstack-protector-strong
        -D_FORTIFY_SOURCE=2
    )
    if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_compile_options(-ffunction-sections -fdata-sections)
    endif()
endif()

# Build options
option(XOCEANUS_BUILD_AI  "Build AI sound assistant subsystem" OFF)
option(XOCEANUS_BUILD_VST3 "Build VST3 format (experimental)" OFF)  # closes #638
option(XOCEANUS_BUILD_COVERAGE "Build with code coverage instrumentation (gcov/llvm-cov)" OFF)

if(XOCEANUS_BUILD_COVERAGE)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        add_compile_options(--coverage -fprofile-arcs -ftest-coverage)
        add_link_options(--coverage)
    else()
        message(WARNING "Coverage instrumentation not supported for this compiler")
    endif()
endif()

option(XOCEANUS_BUILD_CRASH_REPORTING "Build with Sentry crash reporting (opt-in at runtime)" OFF)

if(XOCEANUS_BUILD_CRASH_REPORTING)
    include(FetchContent)
    FetchContent_Declare(
        sentry
        GIT_REPOSITORY https://github.com/getsentry/sentry-native.git
        GIT_TAG        0.7.17
        GIT_SHALLOW    TRUE
    )
    set(SENTRY_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(SENTRY_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(sentry)
endif()

# Platform detection
# iOS build: cmake -B build-ios -G Xcode -DCMAKE_TOOLCHAIN_FILE=cmake/ios-toolchain.cmake
if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
    set(XO_MOBILE ON)
    set(XO_FORMATS AUv3 Standalone)
    add_compile_definitions(XO_MOBILE=1)
else()
    set(XO_MOBILE OFF)
    # VST3 is a V2 target (tracked in #98). Add "VST3" to XO_FORMATS here when
    # VST3 work begins. Pass -DXOCEANUS_BUILD_VST3=ON to enable before that lands.
    if(XOCEANUS_BUILD_VST3)
        set(XO_FORMATS AU Standalone VST3)
    else()
        set(XO_FORMATS AU Standalone)
    endif()
endif()

# JUCE
add_subdirectory(Libs/JUCE)

# Rive C++ runtime (MIT licensed — https://github.com/rive-app/rive-cpp)
# Clone into Libs/rive-cpp:
#   git clone https://github.com/rive-app/rive-cpp Libs/rive-cpp
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Libs/rive-cpp/CMakeLists.txt")
    add_subdirectory(Libs/rive-cpp)
    set(XO_HAS_RIVE ON)
else()
    message(STATUS "Rive not found — Oscar animation disabled (clone Libs/rive-cpp to enable)")
    set(XO_HAS_RIVE OFF)
endif()

# Oscar character — oscar.riv embedded as C++ binary data.
# Place the authored .riv file at Resources/Oscar/oscar.riv before building.
# JUCE generates BinaryData::oscar_riv and BinaryData::oscar_rivSize.
if(XO_HAS_RIVE AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Resources/Oscar/oscar.riv")
    juce_add_binary_data(OscarRivData SOURCES
        Resources/Oscar/oscar.riv
    )
    set(XO_HAS_OSCAR ON)
else()
    set(XO_HAS_OSCAR OFF)
endif()

# Typography — Space Grotesk (display), Inter (body), JetBrains Mono (values),
# Overbit (engine nameplate display). All embedded as C++ binary data via JUCE.
juce_add_binary_data(XOceanusFont
    NAMESPACE "FontData"
    SOURCES
    Assets/fonts/SpaceGrotesk-Bold.otf
    Assets/fonts/SpaceGrotesk-Medium.otf
    Assets/fonts/Inter-Regular.ttf
    Assets/fonts/Inter-Medium.ttf
    Assets/fonts/Inter-Bold.ttf
    Assets/fonts/JetBrainsMono-Regular.ttf
    Assets/fonts/Overbit-Regular.otf
)

juce_add_plugin(XOceanus
    COMPANY_NAME "XO_OX Designs"
    PLUGIN_MANUFACTURER_CODE XoOx
    PLUGIN_CODE Xocn
    FORMATS ${XO_FORMATS}
    PRODUCT_NAME "XOceanus"
    BUNDLE_ID "com.xo-ox.xoceanus"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD TRUE
)

# Source files — only list files that exist.
# As new modules are built, add them here.
target_sources(XOceanus PRIVATE
    # Core (interface + infrastructure)
    Source/Core/SynthEngine.h
    Source/Core/EngineRegistry.h
    Source/Core/MegaCouplingMatrix.h
    Source/Core/MasterFXChain.h
    Source/Core/PresetManager.h
    Source/Core/ChordMachine.h
    Source/Core/MacroSystem.h
    Source/Core/SharedTransport.h
    Source/Core/MPEManager.h

    # DSP (shared, pure C++)
    Source/DSP/FastMath.h
    Source/DSP/CytomicSVF.h
    Source/DSP/PolyBLEP.h
    Source/DSP/WavetableOscillator.h
    Source/DSP/EngineProfiler.h
    Source/DSP/ThreadInit.h

    # Effects (shared, pure C++)
    Source/DSP/Effects/DubDelay.h
    Source/DSP/Effects/LushReverb.h
    Source/DSP/Effects/Saturator.h
    Source/DSP/Effects/Compressor.h
    Source/DSP/Effects/AquaticFXSuite.h
    Source/DSP/Effects/MathFXChain.h
    Source/DSP/Effects/BoutiqueFXChain.h
    Source/DSP/Effects/EntropyCooler.h
    Source/DSP/Effects/VoronoiShatter.h
    Source/DSP/Effects/QuantumSmear.h
    Source/DSP/Effects/AttractorDrive.h
    Source/DSP/Effects/AnomalyEngine.h
    Source/DSP/Effects/DissolvingArchive.h
    Source/DSP/Effects/ArtifactCathedral.h
    Source/DSP/Effects/SubmersionEngine.h

    # Engine adapters (DSP in .h, registration stubs in .cpp)
    Source/Engines/Snap/SnapEngine.h
    Source/Engines/Snap/SnapEngine.cpp
    Source/Engines/Morph/MorphEngine.h
    Source/Engines/Morph/MorphEngine.cpp
    Source/Engines/Dub/DubEngine.h
    Source/Engines/Dub/DubEngine.cpp
    Source/Engines/Drift/DriftEngine.h
    Source/Engines/Drift/DriftEngine.cpp
    Source/Engines/Bob/BobEngine.h
    Source/Engines/Bob/BobEngine.cpp
    Source/Engines/Fat/FatEngine.h
    Source/Engines/Fat/FatEngine.cpp
    Source/Engines/Onset/OnsetEngine.h
    Source/Engines/Onset/OnsetEngine.cpp
    Source/Engines/Overworld/OverworldEngine.h
    Source/Engines/Overworld/OverworldEngine.cpp
    Source/Engines/Overworld/dsp/BitCrusher.h
    Source/Engines/Overworld/dsp/FIREcho.h
    Source/Engines/Overworld/dsp/GlitchEngine.h
    Source/Engines/Overworld/dsp/SVFilter.h
    Source/Engines/Overworld/Parameters.h
    Source/Engines/Overworld/engine/VoicePool.h
    Source/Engines/Opal/OpalDSP.h
    Source/Engines/Opal/OpalEngine.h
    Source/Engines/Opal/OpalEngine.cpp
    Source/Engines/Organon/OrganonEngine.h
    Source/Engines/Organon/OrganonEngine.cpp
    Source/Engines/Ouroboros/OuroborosEngine.h
    Source/Engines/Ouroboros/OuroborosEngine.cpp
    Source/Engines/Orbital/OrbitalEngine.h
    Source/Engines/Orbital/OrbitalEngine.cpp
    Source/Engines/Obsidian/ObsidianEngine.h
    Source/Engines/Obsidian/ObsidianEngine.cpp
    Source/Engines/Bite/BiteEngine.h
    Source/Engines/Bite/BiteEngine.cpp
    Source/Engines/Origami/OrigamiEngine.h
    Source/Engines/Origami/OrigamiEngine.cpp
    Source/Engines/Oracle/OracleEngine.h
    Source/Engines/Oracle/OracleEngine.cpp
    Source/Engines/Obscura/ObscuraEngine.h
    Source/Engines/Obscura/ObscuraEngine.cpp
    Source/Engines/Oceanic/OceanicEngine.h
    Source/Engines/Oceanic/OceanicEngine.cpp
    Source/Engines/Optic/OpticEngine.h
    Source/Engines/Optic/OpticEngine.cpp
    Source/Engines/Oblique/ObliqueEngine.h
    Source/Engines/Oblique/ObliqueEngine.cpp
    Source/Engines/Ocelot/OcelotEngine.h
    Source/Engines/Ocelot/OcelotEngine.cpp
    Source/Engines/Ocelot/OcelotParameters.h
    Source/Engines/Ocelot/OcelotParamSnapshot.h
    Source/Engines/Ocelot/OcelotVoicePool.h
    Source/Engines/Ocelot/OcelotVoice.h
    Source/Engines/Ocelot/OcelotFloor.h
    Source/Engines/Ocelot/OcelotUnderstory.h
    Source/Engines/Ocelot/OcelotCanopy.h
    Source/Engines/Ocelot/OcelotEmergent.h
    Source/Engines/Ocelot/BiomeMorph.h
    Source/Engines/Ocelot/EcosystemMatrix.h
    Source/Engines/Ocelot/AmpEnvelope.h
    Source/Engines/Ocelot/ModalResonator.h
    Source/Engines/Ocelot/KarplusStrong.h
    Source/Engines/Ocelot/BitCrusher.h
    Source/Engines/Ocelot/TapeWarp.h
    Source/Engines/Ocelot/WaveFolder.h

    Source/Engines/Osprey/OspreyEngine.h
    Source/Engines/Osprey/OspreyEngine.cpp
    Source/Engines/Osteria/OsteriaEngine.h
    Source/Engines/Osteria/OsteriaEngine.cpp

    Source/Engines/Owlfish/OwlfishEngine.h
    Source/Engines/Owlfish/OwlfishEngine.cpp
    Source/Engines/Owlfish/OwlfishParameters.h
    Source/Engines/Owlfish/OwlfishParamSnapshot.h
    Source/Engines/Owlfish/OwlfishVoice.h
    Source/Engines/Owlfish/OwlfishAmpEnvelope.h
    Source/Engines/Owlfish/SubharmonicOsc.h
    Source/Engines/Owlfish/OwlfishCompressor.h
    Source/Engines/Owlfish/MicroGranular.h
    Source/Engines/Owlfish/ArmorBuffer.h
    Source/Engines/Owlfish/AbyssReverb.h
    Source/Engines/Owlfish/OwlfishCytomicSVF.h
    Source/Engines/Owlfish/OwlfishFastMath.h

    # Constellation Family Engines (SP7)
    Source/Engines/Ohm/OhmEngine.h
    Source/Engines/Ohm/OhmEngine.cpp
    Source/Engines/Orphica/OrphicaEngine.h
    Source/Engines/Orphica/OrphicaEngine.cpp
    Source/Engines/Obbligato/ObbligatoEngine.h
    Source/Engines/Obbligato/ObbligatoEngine.cpp
    Source/Engines/Ottoni/OttoniEngine.h
    Source/Engines/Ottoni/OttoniEngine.cpp
    Source/Engines/Ole/OleEngine.h
    Source/Engines/Ole/OleEngine.cpp

    # Concept Phase — Standalone Adapters (Phase 4)
    Source/Engines/Overlap/XOverlapAdapter.h
    Source/Engines/Overlap/XOverlapAdapter.cpp
    Source/Engines/Overlap/DSP/Bioluminescence.h
    Source/Engines/Overlap/DSP/Entrainment.h
    Source/Engines/Overlap/DSP/FastMath.h
    Source/Engines/Overlap/DSP/FDN.h
    Source/Engines/Overlap/DSP/KnotMatrix.h
    Source/Engines/Overlap/DSP/ParamSnapshot.h
    Source/Engines/Overlap/DSP/PostFX.h
    Source/Engines/Overlap/DSP/Voice.h
    Source/Engines/Outwit/XOutwitAdapter.h
    Source/Engines/Outwit/XOutwitAdapter.cpp
    Source/Engines/Outwit/DSP/AmpEnvelope.h
    Source/Engines/Outwit/DSP/ArmChannel.h
    Source/Engines/Outwit/DSP/DenReverb.h
    Source/Engines/Outwit/DSP/FastMath.h
    Source/Engines/Outwit/DSP/InkCloud.h
    Source/Engines/Outwit/DSP/ParamSnapshot.h

    Source/Engines/Ombre/OmbreEngine.h
    Source/Engines/Ombre/OmbreEngine.cpp

    Source/Engines/Orca/OrcaEngine.h
    Source/Engines/Orca/OrcaEngine.cpp
    Source/Engines/Octopus/OctopusEngine.h
    Source/Engines/Octopus/OctopusEngine.cpp
    Source/Engines/Obrix/ObrixEngine.h
    Source/Engines/Obrix/ObrixEngine.cpp

    # V1 Concept Engines — Full DSP Builds
    Source/Engines/Ostinato/OstinatoEngine.h
    Source/Engines/Ostinato/OstinatoEngine.cpp
    Source/Engines/OpenSky/OpenSkyEngine.h
    Source/Engines/OpenSky/OpenSkyEngine.cpp
    Source/Engines/OceanDeep/OceandeepEngine.h
    Source/Engines/OceanDeep/OceandeepEngine.cpp
    Source/Engines/Ouie/OuieEngine.h
    Source/Engines/Ouie/OuieEngine.cpp

    # V2 Theorem Engines
    Source/Engines/Orbweave/OrbweaveEngine.h
    Source/Engines/Orbweave/OrbweaveEngine.cpp
    Source/Engines/Overtone/OvertoneEngine.h
    Source/Engines/Overtone/OvertoneEngine.cpp
    Source/Engines/Organism/OrganismEngine.h
    Source/Engines/Organism/OrganismEngine.cpp
    Source/Engines/Oxbow/OxbowEngine.h
    Source/Engines/Oxbow/OxbowEngine.cpp
    Source/Engines/Oware/OwareEngine.h
    Source/Engines/Oware/OwareEngine.cpp

    # Kuramoto Vocal Synthesis
    Source/Engines/Opera/OperaConstants.h
    Source/Engines/Opera/KuramotoField.h
    Source/Engines/Opera/OperaPartialBank.h
    Source/Engines/Opera/OperaBreathEngine.h
    Source/Engines/Opera/OperaConductor.h
    Source/Engines/Opera/ReactiveStage.h
    Source/Engines/Opera/OperaEngine.h
    Source/Engines/Opera/OperaAdapter.h
    Source/Engines/Opera/OperaAdapter.cpp

    # Love Triangle Circuit Synth — Engine #48
    Source/Engines/Oxytocin/OxytocinParamSnapshot.h
    Source/Engines/Oxytocin/OxytocinMemory.h
    Source/Engines/Oxytocin/OxytocinTriangle.h
    Source/Engines/Oxytocin/OxytocinLoveEnvelope.h
    Source/Engines/Oxytocin/OxytocinThermal.h
    Source/Engines/Oxytocin/OxytocinDrive.h
    Source/Engines/Oxytocin/OxytocinReactive.h
    Source/Engines/Oxytocin/OxytocinVoice.h
    Source/Engines/Oxytocin/OxytocinEngine.h
    Source/Engines/Oxytocin/OxytocinAdapter.h
    Source/Engines/Oxytocin/OxytocinAdapter.cpp

    # Singularity Collection FX
    Source/DSP/Effects/fXObscura.h
    Source/DSP/Effects/fXOratory.h
    Source/DSP/Effects/fXOnslaught.h

    # Export pipeline
    Source/Export/XPNVelocityCurves.h
    Source/Export/XPNCoverArt.h
    Source/Export/XOriginate.h
    Source/Export/XPNExporter.h
    Source/Export/XOutshine.h
    Source/Export/XPNDrumExporter.h

    # Core (MIDI Learn — shared across platforms)
    Source/Core/MIDILearnManager.h

    # UI (Gallery Model)
    Source/UI/XOceanusEditor.cpp
    Source/UI/XOceanusEditor.h
    Source/UI/PlaySurface/PlaySurface.h
    Source/UI/CouplingColors.h
    Source/UI/SharedPreviewDevice.h
    Source/UI/OpticVisualizer/OpticVisualizer.h
    Source/UI/PresetBrowser/PresetBrowser.h
    Source/UI/Gallery/ObrixDetailPanel.h
    Source/UI/Gallery/EnginePickerPopup.h
    Source/UI/Gallery/CouplingPopover.h
    Source/UI/Gallery/WaveformDisplay.h
    Source/UI/Gallery/CouplingChainView.h
    Source/UI/Gallery/DrumPadGrid.h
    Source/UI/Gallery/SpecializedWidgets.h
    Source/UI/Gallery/SpecializedDisplays.h
    Source/UI/Gallery/CouplingInspectorPanel.h
    Source/UI/Gallery/FXInspectorPanel.h
    Source/UI/Gallery/PlayControlPanel.h
    Source/UI/Gallery/SettingsPanel.h
    Source/UI/Gallery/ExportTabPanel.h
    Source/UI/Gallery/DepthZoneDial.h
    Source/UI/Gallery/ABCompare.h
    Source/UI/Gallery/HeaderIndicators.h
    Source/UI/Gallery/MiniCouplingGraph.h

    # Oscar (OddOscar animated companion) — requires Rive, added conditionally below

    # UI (Mobile — compiled on all platforms, gated by XO_MOBILE)
    Source/UI/Mobile/MobileTouchHandler.h
    Source/UI/Mobile/SensorManager.h
    Source/UI/Mobile/HapticEngine.h
    Source/UI/Mobile/MobileLayoutManager.h
    Source/UI/Mobile/CPUMonitor.h
    Source/UI/Mobile/MobilePlaySurface.h
    Source/UI/Mobile/ParameterDrawer.h

    # iOS platform bridge implementations (Objective-C++)
    # .mm files are only compiled by Clang on Apple platforms; #if JUCE_IOS guards
    # inside each file further restrict runtime behaviour to iOS only.
    Source/UI/Mobile/HapticEngine_iOS.mm
    Source/UI/Mobile/SensorManager_iOS.mm
    Source/UI/Mobile/AudioSession_iOS.h
    Source/UI/Mobile/AudioSession_iOS.mm
    Source/UI/Mobile/TouchForce_iOS.mm
    Source/UI/Mobile/DisplayLink_iOS.mm

    # SpriteKit Companion Layer — OBRIX Pocket reef + creatures + particles
    # ReefBridge.h compiles on all platforms (no-op stubs for macOS).
    # ReefBridge.mm requires SpriteKit; linked conditionally in the XO_MOBILE block.
    # CreatureBehaviorTimer.h is a pure JUCE header — compiles everywhere.
    Source/UI/Mobile/ReefBridge.h
    Source/UI/Mobile/ReefBridge.mm
    Source/UI/Mobile/CreatureBehaviorTimer.h

    # Kitchen Collection — Chef (Organs) engines
    Source/Engines/Oto/OtoEngine.h
    Source/Engines/Octave/OctaveEngine.h
    Source/Engines/Oleg/OlegEngine.h
    Source/Engines/Otis/OtisEngine.h

    # Kitchen Collection — Kitchen (Pianos) engines
    Source/Engines/Oven/OvenEngine.h
    Source/Engines/Ochre/OchreEngine.h
    Source/Engines/Obelisk/ObeliskEngine.h
    Source/Engines/Opaline/OpalineEngine.h

    # Kitchen Collection — Cellar (Bass) engines
    Source/Engines/Ogre/OgreEngine.h
    Source/Engines/Olate/OlateEngine.h
    Source/Engines/Oaken/OakenEngine.h
    Source/Engines/Omega/OmegaEngine.h

    # Kitchen Collection — Broth (Pads) engines
    Source/Engines/Overwash/OverwashEngine.h
    Source/Engines/Overworn/OverwornEngine.h
    Source/Engines/Overflow/OverflowEngine.h
    Source/Engines/Overcast/OvercastEngine.h

    # Kitchen Collection — Fusion (EP) engines
    Source/Engines/Oasis/OasisEngine.h
    Source/Engines/Oasis/OasisEngine.cpp
    Source/Engines/Oddfellow/OddfellowEngine.h
    Source/Engines/Oddfellow/OddfellowEngine.cpp
    Source/Engines/Onkolo/OnkoloEngine.h
    Source/Engines/Onkolo/OnkoloEngine.cpp
    Source/Engines/Opcode/OpcodeEngine.h
    Source/Engines/Opcode/OpcodeEngine.cpp

    # Kitchen Collection — Garden (Strings) engines
    Source/Engines/Orchard/OrchardEngine.h
    Source/Engines/Orchard/OrchardEngine.cpp
    Source/Engines/Osier/OsierEngine.h
    Source/Engines/Osier/OsierEngine.cpp
    Source/Engines/Overgrow/OvergrowEngine.h
    Source/Engines/Overgrow/OvergrowEngine.cpp
    Source/Engines/Oxalis/OxalisEngine.h
    Source/Engines/Oxalis/OxalisEngine.cpp

    # Psychology Boom Bap Drum Synthesis
    Source/Engines/Offering/OfferingEngine.h
    Source/Engines/Offering/OfferingCity.h
    Source/Engines/Offering/OfferingCollage.h
    Source/Engines/Offering/OfferingCuriosity.h
    Source/Engines/Offering/OfferingTexture.h
    Source/Engines/Offering/OfferingTransient.h
    Source/Engines/Offering/OfferingEngine.cpp

    # Late-registered engines (closes #162)
    Source/Engines/Obiont/ObiontEngine.h
    Source/Engines/Obiont/ObiontEngine.cpp
    Source/Engines/Okeanos/OkeanosEngine.h
    Source/Engines/Okeanos/OkeanosEngine.cpp
    Source/Engines/Outflow/OutflowEngine.h
    Source/Engines/Outflow/OutflowEngine.cpp
    Source/Engines/Osmosis/OsmosisEngine.h
    Source/Engines/Osmosis/OsmosisEngine.cpp
    Source/Engines/Outlook/OutlookEngine.h
    Source/Engines/Outlook/OutlookEngine.cpp

    # Shapers
    Source/Shapers/Oxide/OxideShaper.h
    Source/Shapers/Oxide/OxideShaper.cpp
    Source/Shapers/Observe/ObserveShaper.h
    Source/Shapers/Observe/ObserveShaper.cpp

    # Main processor
    Source/XOceanusProcessor.h
    Source/XOceanusProcessor.cpp
)

# AI Sound Assistant (disabled by default — opt-in via -DXOCEANUS_BUILD_AI=ON)
# Files live in Source/AI/. RecipeEngine.h was moved from Source/Core/ to Source/AI/
# on 2026-04-01 (closes #575).
if(XOCEANUS_BUILD_AI)
    target_sources(XOceanus PRIVATE
        Source/AI/SoundAssistant.h
        Source/AI/AIParameterSchema.h
        Source/AI/NaturalLanguageInterpreter.h
        Source/AI/SecureKeyStore.h
        Source/AI/SharedRecipeVault.h
        Source/AI/CommunityInsights.h
        Source/AI/RecipeEngine.h
    )
endif()

# Oscar animated companion — requires Rive runtime
if(XO_HAS_RIVE)
    target_sources(XOceanus PRIVATE
        Source/UI/OddOscar/OscarAnimState.h
        Source/UI/OddOscar/OscarJuceRenderer.h
        Source/UI/OddOscar/OscarRiveComponent.h
    )
endif()

# XOverworld DSP headers — embedded in Source/Engines/Overworld/
target_include_directories(XOceanus PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/Engines/Overworld"
)

# XOverlap and XOutwit DSP headers — embedded in Source/Engines/{Overlap,Outwit}/DSP/
# FastMath.h stubs forward to the shared Source/DSP/FastMath.h via #include "DSP/FastMath.h".
# That path requires Source/ in the search path.
target_include_directories(XOceanus PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/Source"
)

target_compile_definitions(XOceanus PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
)

# JUCE_VST3_CAN_REPLACE_VST2 is only relevant when building VST3.
# Guard it to avoid confusion when VST3 is not in XO_FORMATS.
if(XOCEANUS_BUILD_VST3)
    target_compile_definitions(XOceanus PUBLIC JUCE_VST3_CAN_REPLACE_VST2=0)
endif()

target_link_libraries(XOceanus PRIVATE
    juce::juce_audio_utils
    juce::juce_audio_processors
    juce::juce_dsp
    juce::juce_gui_extra
    XOceanusFont
)
if(XO_HAS_RIVE)
    target_link_libraries(XOceanus PRIVATE rive)
    target_compile_definitions(XOceanus PRIVATE XO_HAS_RIVE=1)
endif()
if(XO_HAS_OSCAR)
    target_link_libraries(XOceanus PRIVATE OscarRivData)
    target_compile_definitions(XOceanus PRIVATE XO_HAS_OSCAR=1)
endif()

if(XO_MOBILE)
    target_link_libraries(XOceanus PRIVATE
        juce::juce_audio_plugin_client
    )
    target_link_libraries(XOceanus PRIVATE
        "-framework CoreMotion"
        "-framework CoreHaptics"
        "-framework AVFoundation"
        "-framework SpriteKit"   # ReefBridge.mm — SpriteKit companion layer
    )
endif()

# ==============================================================================
# Catch2 v3 — fetched automatically; provides Catch2::Catch2WithMain target
# which supplies the test runner main(). No external install required.
# ==============================================================================
include(FetchContent)
FetchContent_Declare(Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.5.4
)
FetchContent_MakeAvailable(Catch2)

# ==============================================================================
# Tests (standalone console application, no audio device or GUI required)
# ==============================================================================
add_executable(XOceanusTests
    Tests/run_tests.cpp
    Tests/DSPTests/DSPStabilityTests.cpp
    Tests/DSPTests/DSPComponentTests.cpp
    Tests/CouplingTests/CouplingMatrixTests.cpp
    Tests/PresetTests/PresetRoundTripTests.cpp
    Tests/PresetTests/BackwardCompatibilityTests.cpp
    Tests/ExportTests/XPNExportTests.cpp
    Tests/DoctrineTests/DoctrineTests.cpp
    Tests/PlaySurfaceTests/HarmonicFieldTests.cpp
    Tests/PlaySurfaceTests/GestureTrailTests.cpp
    Tests/PipelineTests/FullPipelineTests.cpp
    Tests/ParameterSweepTests/ParameterSweepTests.cpp
    Tests/BenchmarkTests/CPUBenchmarkTests.cpp
    Tests/RegressionTests/AudioRegressionTests.cpp
)

# FamilyWaveguideTest is a standalone executable (has its own main())
add_executable(FamilyWaveguideTest
    Tests/FamilyWaveguideTest.cpp
)

# Fix #173 — FamilyWaveguideTest needs includes, link libs, and ctest registration
target_include_directories(FamilyWaveguideTest PRIVATE Source Source/DSP)
target_link_libraries(FamilyWaveguideTest PRIVATE juce::juce_core)
add_test(NAME FamilyWaveguideTest COMMAND FamilyWaveguideTest)

target_include_directories(XOceanusTests PRIVATE
    Source
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/Engines/Overworld"
)

target_link_libraries(XOceanusTests PRIVATE
    Catch2::Catch2WithMain
    juce::juce_core
    juce::juce_audio_basics
    juce::juce_audio_formats
    juce::juce_audio_processors
    juce::juce_gui_basics
)

target_compile_definitions(XOceanusTests PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
)

enable_testing()
add_test(NAME XOceanusTests COMMAND XOceanusTests)

# ==============================================================================
# Installer package (macOS only)
# Builds Installer/XOceanus-Installer.pkg using pkgbuild + productbuild.
# Usage:  cmake --build build --target installer
# ==============================================================================
if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    add_custom_target(installer
        COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/Installer/build_pkg.sh ${CMAKE_BUILD_TYPE}
        DEPENDS XOceanus_AU XOceanus_Standalone
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Building XOceanus installer package..."
    )
endif()

# ==============================================================================
# CMake install() rules — closes #407
#
# Usage:
#   cmake --install build                      (install to /usr/local by default)
#   cmake --install build --prefix ~/my-prefix
#
# Installed layout:
#   ${prefix}/lib/AU/XOceanus.component        (macOS AU plugin bundle)
#   ${prefix}/bin/XOceanus                     (Standalone application)
#   ${prefix}/share/XOceanus/Presets/          (factory presets)
# ==============================================================================
if(APPLE)
    # AU plugin bundle (macOS only — not applicable to iOS)
    if(NOT CMAKE_SYSTEM_NAME STREQUAL "iOS")
        install(
            DIRECTORY "$<TARGET_BUNDLE_DIR:XOceanus_AU>"
            DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/AU"
            USE_SOURCE_PERMISSIONS
        )
    endif()

    # Standalone application
    install(
        TARGETS XOceanus_Standalone
        BUNDLE DESTINATION "${CMAKE_INSTALL_PREFIX}/bin"
        RUNTIME DESTINATION "${CMAKE_INSTALL_PREFIX}/bin"
    )
else()
    # Non-macOS: install standalone executable and shared-library plugin (VST3)
    install(
        TARGETS XOceanus_Standalone
        RUNTIME DESTINATION bin
    )
    install(
        DIRECTORY "$<TARGET_FILE_DIR:XOceanus_VST3>/XOceanus.vst3"
        DESTINATION lib/vst3
        OPTIONAL
    )
endif()

# Factory presets (platform-independent)
install(
    DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/Presets/"
    DESTINATION "share/XOceanus/Presets"
)

# ==============================================================================
# CPack configuration — closes #407
#
# On macOS: produces a .dmg containing the AU bundle + Standalone app.
# On other platforms: produces a .tar.gz.
#
# Usage:
#   cd build && cpack               (auto-detects generator)
#   cd build && cpack -G DragNDrop  (explicit .dmg)
#   cd build && cpack -G TGZ        (explicit .tar.gz)
# ==============================================================================
set(CPACK_PACKAGE_NAME              "XOceanus")
set(CPACK_PACKAGE_VENDOR            "XO_OX Designs")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "XOceanus — 76-engine synthesizer platform")
set(CPACK_PACKAGE_VERSION_MAJOR     ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR     ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH     ${PROJECT_VERSION_PATCH})
set(CPACK_PACKAGE_VERSION           "${PROJECT_VERSION}")
set(CPACK_RESOURCE_FILE_LICENSE     "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "XOceanus")

if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    # DragNDrop creates a .dmg with the AU bundle and Standalone side-by-side.
    set(CPACK_GENERATOR "DragNDrop")
    set(CPACK_DMG_VOLUME_NAME   "XOceanus ${PROJECT_VERSION}")
    set(CPACK_DMG_FORMAT        "UDZO")   # zlib-compressed, read-write
    set(CPACK_DMG_DS_STORE_SETUP_SCRIPT
        "${CMAKE_CURRENT_SOURCE_DIR}/Installer/dmg_setup.scpt"
    )
    # Also make a .pkg available as an alternative generator
    set(CPACK_PRODUCTBUILD_IDENTIFIER "com.xo-ox.xoceanus")
else()
    set(CPACK_GENERATOR "TGZ")
endif()

include(CPack)
