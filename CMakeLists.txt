cmake_minimum_required(VERSION 3.22)
project(XOmnibus VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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
    Source/Core/PresetManager.h
    Source/Core/MacroSystem.h
    Source/Core/SharedTransport.h

    # DSP (shared, pure C++)
    Source/DSP/FastMath.h
    Source/DSP/CytomicSVF.h
    Source/DSP/PolyBLEP.h
    Source/DSP/WavetableOscillator.h

    # Effects (shared, pure C++)
    Source/DSP/Effects/DubDelay.h
    Source/DSP/Effects/LushReverb.h
    Source/DSP/Effects/Saturator.h
    Source/DSP/Effects/Compressor.h

    # Engine adapters
    Source/Engines/Snap/SnapEngine.h
    Source/Engines/Morph/MorphEngine.h
    Source/Engines/Dub/DubEngine.h
    Source/Engines/Drift/DriftEngine.h
    Source/Engines/Bob/BobEngine.h
    Source/Engines/Fat/FatEngine.h
    Source/Engines/Onset/OnsetEngine.h

    # Main processor
    Source/XOmnibusProcessor.h
    Source/XOmnibusProcessor.cpp
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

if(XO_MOBILE)
    target_link_libraries(XOmnibus PRIVATE
        juce::juce_audio_plugin_client
    )
endif()
