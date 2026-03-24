#pragma once
// xolokun-engine-sdk — EngineModule.h
// C export macros for community engine modules.
//
// Community engines are compiled as shared libraries (.dylib / .so / .dll)
// and must export two C functions:
//
//   xolokun_create_engine()  — factory that returns a new SynthEngine instance
//   xolokun_engine_info()    — fills metadata struct without creating an engine
//
// Usage in your engine .cpp:
//
//   #include <xolokun/EngineModule.h>
//   XOLOKUN_EXPORT_ENGINE(MyEngine, "MyEngine", "My Engine Display Name",
//                          "me_", 0x1E, 0x8B, 0x7E, "1.0.0", "Your Name")

#include "SynthEngine.h"
#include <cstring>

namespace xolokun {

/// Metadata returned by xolokun_engine_info() without instantiating the engine.
struct EngineMetadata {
    char id[64]        {};   ///< Engine ID (e.g. "MyEngine")
    char displayName[128] {};///< Human-readable name
    char paramPrefix[16] {};  ///< Parameter prefix (e.g. "me_")
    char version[16]   {};   ///< Semantic version string
    char author[64]    {};   ///< Author/studio name
    uint8_t accentR = 128;   ///< Accent colour R
    uint8_t accentG = 128;   ///< Accent colour G
    uint8_t accentB = 128;   ///< Accent colour B
    int maxVoices = 8;       ///< Maximum polyphony
    int sdkVersion = 1;      ///< SDK ABI version (bump on breaking changes)
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
