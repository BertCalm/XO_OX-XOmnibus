cmake_minimum_required(VERSION 3.22)
project(XOceanusTests LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---------------------------------------------------------------------------
# Catch2 v3 — fetched automatically if not present
# ---------------------------------------------------------------------------
include(FetchContent)
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.5.4
)
FetchContent_MakeAvailable(Catch2)

# ---------------------------------------------------------------------------
# Compiler warning flags — match XOceanus quality gate (closes #408)
# Same exceptions as root CMakeLists.txt: unused-parameter and
# deprecated-declarations are suppressed to avoid noise from JUCE callbacks.
# ---------------------------------------------------------------------------
set(XOCEANUS_TEST_COMPILE_OPTIONS
    $<$<CXX_COMPILER_ID:Clang,AppleClang,GNU>:
        -Wall -Wextra -Wpedantic
        -Werror
        -Wno-error=unused-parameter
        -Wno-error=deprecated-declarations>
)

# ---------------------------------------------------------------------------
# Main test runner — Catch2 with all suites linked in
# ---------------------------------------------------------------------------
add_executable(XOceanusTests
    run_tests.cpp
    DSPTests/DSPStabilityTests.cpp
    DSPTests/DSPComponentTests.cpp
    CouplingTests/CouplingMatrixTests.cpp
    PresetTests/PresetRoundTripTests.cpp
    PresetTests/BackwardCompatibilityTests.cpp
    ExportTests/XPNExportTests.cpp
    DoctrineTests/DoctrineTests.cpp
    PlaySurfaceTests/HarmonicFieldTests.cpp
    PlaySurfaceTests/GestureTrailTests.cpp
    PipelineTests/FullPipelineTests.cpp
    ParameterSweepTests/ParameterSweepTests.cpp
    BenchmarkTests/CPUBenchmarkTests.cpp
    RegressionTests/AudioRegressionTests.cpp
)

target_include_directories(XOceanusTests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(XOceanusTests PRIVATE Catch2::Catch2WithMain)

target_compile_options(XOceanusTests PRIVATE ${XOCEANUS_TEST_COMPILE_OPTIONS})

# ---------------------------------------------------------------------------
# FamilyWaveguideTest — standalone unit test (no Catch2 dependency)
# ---------------------------------------------------------------------------
add_executable(FamilyWaveguideTest FamilyWaveguideTest.cpp)

target_include_directories(FamilyWaveguideTest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)

target_compile_options(FamilyWaveguideTest PRIVATE ${XOCEANUS_TEST_COMPILE_OPTIONS})

# ---------------------------------------------------------------------------
# CTest integration
# ---------------------------------------------------------------------------
include(CTest)
include(Catch)
catch_discover_tests(XOceanusTests)
add_test(NAME FamilyWaveguideTest COMMAND FamilyWaveguideTest)
