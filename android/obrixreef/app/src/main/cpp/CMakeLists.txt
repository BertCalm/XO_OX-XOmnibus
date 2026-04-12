cmake_minimum_required(VERSION 3.22)
project(obrixreef_native LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- Oboe (low-latency audio) -----------------------------------------------
include(FetchContent)
FetchContent_Declare(oboe
    GIT_REPOSITORY https://github.com/google/oboe.git
    GIT_TAG        1.9.0
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(oboe)

# --- libobrix (portable Obrix DSP) ------------------------------------------
# OBRIX_ROOT is passed from build.gradle.kts
if(NOT DEFINED OBRIX_ROOT)
    set(OBRIX_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../../../..")
endif()

add_subdirectory("${OBRIX_ROOT}/libobrix" "${CMAKE_BINARY_DIR}/libobrix")

# --- JNI shared library ------------------------------------------------------
add_library(obrixreef_jni SHARED
    obrix_jni.cpp
    obrix_audio_engine.cpp
)

target_include_directories(obrixreef_jni PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_link_libraries(obrixreef_jni PRIVATE
    obrix
    oboe
    log
    android
)

# Optimize for size in release
target_compile_options(obrixreef_jni PRIVATE
    $<$<CONFIG:Release>:-O2 -ffast-math -fvisibility=hidden>
)
