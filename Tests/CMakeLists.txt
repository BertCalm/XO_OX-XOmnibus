cmake_minimum_required(VERSION 3.15)
project(FamilyWaveguideTest LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(FamilyWaveguideTest FamilyWaveguideTest.cpp)

# Source/DSP/FamilyWaveguide.h is relative to XO_OX-XOlokun root (one level up)
target_include_directories(FamilyWaveguideTest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)

# Strict warnings + -Werror to match XOlokun quality gate (closes #408).
# Same exceptions as root CMakeLists.txt: unused-parameter and
# deprecated-declarations are suppressed to avoid noise from JUCE callbacks.
target_compile_options(FamilyWaveguideTest PRIVATE
    $<$<CXX_COMPILER_ID:Clang,AppleClang,GNU>:
        -Wall -Wextra -Wpedantic
        -Werror
        -Wno-error=unused-parameter
        -Wno-error=deprecated-declarations>
)
