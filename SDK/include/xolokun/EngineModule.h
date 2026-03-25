#pragma once
// xolokun-engine-sdk — EngineModule.h
//
// Community engines ship as shared libraries (.dylib / .so / .dll).
// XOlokun loads them at runtime and calls two C functions it expects to find:
//
//   xolokun_create_engine()  — allocates and returns a new SynthEngine instance
//   xolokun_engine_info()    — returns metadata without constructing the engine
//                              (used for display names, accent colours, etc.)
//
// The XOLOKUN_EXPORT_ENGINE macro generates both functions from a single line.
// Drop this at file scope (outside any class or namespace) in your .h file:
//
//   #include <xolokun/EngineModule.h>
//   XOLOKUN_EXPORT_ENGINE (MyEngine, "Onyx", "Onyx Engine",
//                           "onyx_", 0x1E, 0x8B, 0x7E, "1.0.0", "Your Studio")

#include "SynthEngine.h"
#include <cstring>

namespace xolokun {

/// Metadata returned by xolokun_engine_info() without constructing the engine.
/// XOlokun reads this to populate the engine browser, UI headers, and registry.
struct EngineMetadata
{
    char id[64]          {};  ///< Engine ID string, e.g. "Onyx"
    char displayName[128] {}; ///< Human-readable UI name, e.g. "Onyx Engine"
    char paramPrefix[16] {};  ///< Parameter namespace prefix, e.g. "onyx_"
    char version[16]     {};  ///< Semantic version string, e.g. "1.0.0"
    char author[64]      {};  ///< Author or studio name

    uint8_t accentR = 128;    ///< Accent colour — red channel (0–255)
    uint8_t accentG = 128;    ///< Accent colour — green channel (0–255)
    uint8_t accentB = 128;    ///< Accent colour — blue channel (0–255)

    int maxVoices  = 8;       ///< Maximum simultaneous voices
    int sdkVersion = 1;       ///< SDK ABI version — bumped on breaking interface changes
};

} // namespace xolokun

//==============================================================================
// Export macros
//==============================================================================

#ifdef _WIN32
  #define XOLOKUN_EXPORT extern "C" __declspec(dllexport)
#else
  #define XOLOKUN_EXPORT extern "C" __attribute__((visibility("default")))
#endif

/// Convenience macro: define both C export functions for your engine.
///
/// @param EngineClass   Your C++ class (must inherit xolokun::SynthEngine)
/// @param id            Engine ID string (must be unique, use an O-word)
/// @param displayName   Display name for UI
/// @param prefix        Parameter prefix (e.g. "me_")
/// @param r, g, b       Accent colour RGB (0-255)
/// @param ver           Version string (e.g. "1.0.0")
/// @param author        Author name
#define XOLOKUN_EXPORT_ENGINE(EngineClass, id, displayName, prefix, r, g, b, ver, author) \
    XOLOKUN_EXPORT std::unique_ptr<xolokun::SynthEngine> xolokun_create_engine()         \
    {                                                                                       \
        return std::make_unique<EngineClass>();                                              \
    }                                                                                       \
    XOLOKUN_EXPORT void xolokun_engine_info (xolokun::EngineMetadata* out)                \
    {                                                                                       \
        if (!out) return;                                                                   \
        std::memset (out, 0, sizeof (*out));                                                \
        std::strncpy (out->id, id, sizeof (out->id) - 1);                                  \
        std::strncpy (out->displayName, displayName, sizeof (out->displayName) - 1);        \
        std::strncpy (out->paramPrefix, prefix, sizeof (out->paramPrefix) - 1);             \
        std::strncpy (out->version, ver, sizeof (out->version) - 1);                        \
        std::strncpy (out->author, author, sizeof (out->author) - 1);                       \
        out->accentR = r; out->accentG = g; out->accentB = b;                              \
        out->sdkVersion = 1;                                                                \
    }
