cmake_minimum_required(VERSION 3.15)
project(FamilyWaveguideTest LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(FamilyWaveguideTest FamilyWaveguideTest.cpp)

# Source/DSP/FamilyWaveguide.h is relative to XO_OX-XOmnibus root (one level up)
target_include_directories(FamilyWaveguideTest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)

# Strict warnings to match XOmnibus quality gate
target_compile_options(FamilyWaveguideTest PRIVATE
    $<$<CXX_COMPILER_ID:Clang,AppleClang,GNU>:-Wall -Wextra -Wpedantic>
)
