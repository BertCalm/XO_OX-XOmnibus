cmake_minimum_required(VERSION 3.22)
project(libobrix VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Root of the XOceanus source tree (parent of libobrix/)
get_filename_component(XO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE)

# --- libobrix static library -------------------------------------------------

add_library(obrix STATIC
    src/ObrixParamStore.cpp
    src/ObrixSDKAdapter.cpp

    # Original engine source (one-line stub that includes the header)
    "${XO_ROOT}/Source/Engines/Obrix/ObrixEngine.cpp"
)

# Include path ordering is CRITICAL:
#   1. libobrix/include/shim/  — intercepts <juce_audio_processors/juce_audio_processors.h>
#   2. libobrix/include/       — provides obrix/juce_compat.h, obrix/ObrixSDKAdapter.h, etc.
#   3. XO_ROOT/Source/          �� original DSP headers (FastMath.h, PolyBLEP.h, etc.)
target_include_directories(obrix PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include/shim"   # MUST be first — shim intercepts JUCE includes
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${XO_ROOT}/Source"
)

target_compile_definitions(obrix PRIVATE
    OBRIX_STANDALONE=1
)

if(ANDROID)
    target_compile_options(obrix PRIVATE -O2 -ffast-math)
    target_link_libraries(obrix PRIVATE log)
endif()

# --- Tests (host build only, not Android) ------------------------------------

if(NOT ANDROID)
    enable_testing()

    add_executable(test_obrix_compile
        tests/test_compile.cpp
    )
    target_link_libraries(test_obrix_compile PRIVATE obrix)

    add_test(NAME obrix_compile COMMAND test_obrix_compile)

    add_executable(test_obrix_render
        tests/test_render.cpp
    )
    target_link_libraries(test_obrix_render PRIVATE obrix)

    add_test(NAME obrix_render COMMAND test_obrix_render)
endif()
