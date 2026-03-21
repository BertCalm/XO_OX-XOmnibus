cmake_minimum_required(VERSION 3.22)
project(XOmnibus VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Security hardening flags
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    add_compile_options(
        -Wall -Wextra -Wpedantic
        -fstack-protector-strong
        -D_FORTIFY_SOURCE=2
    )
    if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_compile_options(-ffunction-sections -fdata-sections)
    endif()
endif()

# Platform detection
if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
    set(XO_MOBILE ON)
    set(XO_FORMATS AUv3 Standalone)
    add_compile_definitions(XO_MOBILE=1)
else()
    set(XO_MOBILE OFF)
    set(XO_FORMATS AU Standalone)  # VST3 added in v2
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

juce_add_plugin(XOmnibus
    COMPANY_NAME "XO_OX Designs"
    PLUGIN_MANUFACTURER_CODE XoOx
    PLUGIN_CODE Xomn
    FORMATS ${XO_FORMATS}
    PRODUCT_NAME "XOmnibus"
    BUNDLE_ID "com.xo-ox.xomnibus"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD TRUE
)

# Source files — only list files that exist.
# As new modules are built, add them here.
target_sources(XOmnibus PRIVATE
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
    Source/Engines/Outwit/XOutwitAdapter.h
    Source/Engines/Outwit/XOutwitAdapter.cpp

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
    Source/Engines/OceanDeep/OceanDeepEngine.h
    Source/Engines/OceanDeep/OceanDeepEngine.cpp
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
    Source/UI/XOmnibusEditor.h
    Source/UI/PlaySurface/PlaySurface.h
    Source/UI/CouplingStrip/CouplingStripEditor.h
    Source/UI/OpticVisualizer/OpticVisualizer.h
    Source/UI/PresetBrowser/PresetBrowser.h

    # Oscar (OddOscar animated companion) — requires Rive, added conditionally below

    # UI (Mobile — compiled on all platforms, gated by XO_MOBILE)
    Source/UI/Mobile/MobileTouchHandler.h
    Source/UI/Mobile/SensorManager.h
    Source/UI/Mobile/HapticEngine.h
    Source/UI/Mobile/MobileLayoutManager.h
    Source/UI/Mobile/CPUMonitor.h
    Source/UI/Mobile/MobilePlaySurface.h
    Source/UI/Mobile/ParameterDrawer.h

    # Main processor
    Source/XOmnibusProcessor.h
    Source/XOmnibusProcessor.cpp
)

# Oscar animated companion — requires Rive runtime
if(XO_HAS_RIVE)
    target_sources(XOmnibus PRIVATE
        Source/UI/OddOscar/OscarAnimState.h
        Source/UI/OddOscar/OscarJuceRenderer.h
        Source/UI/OddOscar/OscarRiveComponent.h
    )
endif()

# XOverworld DSP headers — embedded in Source/Engines/Overworld/
target_include_directories(XOmnibus PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/Engines/Overworld"
)

# XOverlap and XOutwit DSP headers — embedded in Source/Engines/{Overlap,Outwit}/DSP/
# FastMath.h stubs forward to the shared Source/DSP/FastMath.h via #include "DSP/FastMath.h".
# That path requires Source/ in the search path.
target_include_directories(XOmnibus PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/Source"
)

target_compile_definitions(XOmnibus PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
)

target_link_libraries(XOmnibus PRIVATE
    juce::juce_audio_utils
    juce::juce_audio_processors
    juce::juce_dsp
    juce::juce_gui_extra
)
if(XO_HAS_RIVE)
    target_link_libraries(XOmnibus PRIVATE rive)
    target_compile_definitions(XOmnibus PRIVATE XO_HAS_RIVE=1)
endif()
if(XO_HAS_OSCAR)
    target_link_libraries(XOmnibus PRIVATE OscarRivData)
    target_compile_definitions(XOmnibus PRIVATE XO_HAS_OSCAR=1)
endif()

if(XO_MOBILE)
    target_link_libraries(XOmnibus PRIVATE
        juce::juce_audio_plugin_client
    )
endif()

# ==============================================================================
# Tests (standalone console application, no audio device or GUI required)
# ==============================================================================
add_executable(XOmnibusTests
    Tests/run_tests.cpp
    Tests/DSPTests/DSPStabilityTests.cpp
    Tests/CouplingTests/CouplingMatrixTests.cpp
    Tests/PresetTests/PresetRoundTripTests.cpp
    Tests/ExportTests/XPNExportTests.cpp
    Tests/DoctrineTests/DoctrineTests.cpp
    Tests/FamilyWaveguideTest.cpp
)

target_include_directories(XOmnibusTests PRIVATE
    Source
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/Engines/Overworld"
)

target_link_libraries(XOmnibusTests PRIVATE
    juce::juce_core
    juce::juce_audio_basics
    juce::juce_audio_formats
    juce::juce_audio_processors
    juce::juce_gui_basics
)

target_compile_definitions(XOmnibusTests PRIVATE
    JUCE_STANDALONE_APPLICATION=1
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
)

enable_testing()
add_test(NAME XOmnibusTests COMMAND XOmnibusTests)
