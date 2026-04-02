# XO_OX Mega-Tool Module Chaining Architecture

**Version:** 1.0
**Author:** XO_OX Designs
**Date:** 2026-03-08
**Status:** Technical Architecture Specification
**Depends on:**
- `xo_mega_tool_feasibility.md` (Hybrid A+D architecture, routing approach, CPU budgets)
- `xo_mega_tool_dev_strategy.md` (Interface-First Hybrid, adapter pattern, parameter namespacing)
- `xo_mega_tool_engine_catalog.md` (7 engine modules, signal flows, synergy matrix)
- `Source/Coupling/CouplingMatrix.h` (existing X-O coupling implementation)

---

## 1. Architecture Overview

### 1.1 The "Normalled Matriarch" Pattern

The Moog Matriarch has 90 patch points, but every connection is pre-wired so the instrument plays without a single cable inserted. Patching overrides the normalled signal path. This is the architectural model for the XO_OX Mega-Tool.

**Principles:**

1. **Sounds correct by default.** Every engine pair has pre-wired coupling routes with musically useful default amounts. A user who never touches the coupling matrix hears a well-designed sound.
2. **Patching overrides defaults.** When a user explicitly sets a coupling route, the normalled default for that specific route is replaced. Other normalled routes on the same pair remain active.
3. **Unpatch restores normal.** Removing a user-defined route re-engages the normalled default. There is no state where a connection is "broken" — only "normalled" or "overridden."
4. **No sound without an engine.** Empty slots produce silence and consume zero CPU. The system does not render placeholder audio.

### 1.2 Core Architecture: Monolithic Inline Processor with Engine Registry

The architecture is **Option 2 (Hybrid)** from the feasibility study — monolithic inline rendering for active engines with a registration system for engine types. This preserves per-sample cross-engine modulation (the XO_OX differentiator) while supporting runtime engine loading.

```
┌──────────────────────────────────────────────────────────────┐
│                    MegaToolProcessor                          │
│                                                              │
│  ┌────────────────┐                                          │
│  │ EngineRegistry │ ← registers factory functions by ID      │
│  └────────┬───────┘                                          │
│           │ instantiates                                     │
│           ▼                                                  │
│  ┌─────┐ ┌─────┐ ┌─────┐ ┌─────┐                           │
│  │Slot │ │Slot │ │Slot │ │Slot │  ← max 4 active            │
│  │  A  │ │  B  │ │  C  │ │  D  │                           │
│  └──┬──┘ └──┬──┘ └──┬──┘ └──┬──┘                           │
│     │       │       │       │                                │
│     └───┬───┘       └───┬───┘                                │
│         │               │                                    │
│  ┌──────▼───────────────▼──────┐                             │
│  │    MegaCouplingMatrix       │ ← per-sample cross-mod      │
│  │  (N×(N-1)/2 coupling pairs)│                              │
│  └─────────────┬───────────────┘                             │
│                │                                              │
│  ┌─────────────▼───────────────┐                             │
│  │      Routing Engine         │                             │
│  │  Independent / Shared / Chain│                             │
│  └─────────────┬───────────────┘                             │
│                │                                              │
│  ┌─────────────▼───────────────┐                             │
│  │        Master Bus           │                             │
│  │  Volume / Limiter / Width   │                             │
│  └─────────────┬───────────────┘                             │
│                │                                              │
│          [Master Out]                                        │
└──────────────────────────────────────────────────────────────┘
```

### 1.3 Per-Sample vs Block-Level Rendering

The system supports two rendering granularities, selected automatically based on CPU load:

| Mode | Coupling Granularity | CPU Cost | When Used |
|------|---------------------|----------|-----------|
| **Per-sample** | Every sample crosses the coupling matrix | Higher | 2 engines, coupling amount > 0 |
| **Block-level** | Engines render full blocks, coupling applied as block-rate modulation | Lower | 3-4 engines, or eco quality mode |

Per-sample mode is the default for 2-engine configurations because it produces the tightest cross-engine modulation — the dub pump effect that defines the XO_OX sound. Block-level mode is a fallback that preserves musicality while reducing CPU.

---

## 2. SynthEngine Interface

Every engine module in the mega-tool implements this interface. It extends the preliminary interface from the dev strategy document with voice management, color identity, and state serialization.

### 2.1 CouplingType Enum

```cpp
#pragma once
#include <JuceHeader.h>

namespace xo {

/// All coupling modulation types supported by the mega-tool.
/// Each type defines a specific signal routing between engine pairs.
enum class CouplingType
{
    // Amplitude-driven routes
    AmpToFilter,     // Source amplitude → target filter cutoff (dub pump / sidechain)
    AmpToPitch,      // Source amplitude → target pitch (pitch ducking)
    AmpToChoke,      // Source trigger → target voice kill (exclusive group behavior)

    // Modulation source routes
    LFOToPitch,      // Source LFO → target oscillator pitch (vibrato / drift)
    EnvToMorph,      // Source envelope → target wavetable/morph position (timbral evolution)
    EnvToDecay,      // Source envelope → target decay time (dynamic interaction)

    // Audio-rate routes
    AudioToFM,       // Source audio output → target oscillator FM input (cross-engine FM)
    AudioToRing,     // Source audio × target audio multiplication (ring modulation)

    // Filter routes
    FilterToFilter,  // Source filter output → target filter input (serial cascade)

    // Rhythm routes
    RhythmToBlend,   // Source rhythmic pattern → target blend position (synthesis morphing)

    // Control routes
    TriggerToReset,  // Source note trigger → target phase/envelope reset (sync)

    // FX routes
    SendToFX,        // Source output → OVERDUB engine send input (dub FX throw)

    NUM_TYPES        // Sentinel for iteration
};

/// String names for UI display and preset serialization.
inline juce::String couplingTypeToString(CouplingType type)
{
    switch (type)
    {
        case CouplingType::AmpToFilter:     return "Amp > Filter";
        case CouplingType::AmpToPitch:      return "Amp > Pitch";
        case CouplingType::AmpToChoke:      return "Amp > Choke";
        case CouplingType::LFOToPitch:      return "LFO > Pitch";
        case CouplingType::EnvToMorph:      return "Env > Morph";
        case CouplingType::EnvToDecay:      return "Env > Decay";
        case CouplingType::AudioToFM:       return "Audio > FM";
        case CouplingType::AudioToRing:     return "Audio > Ring";
        case CouplingType::FilterToFilter:  return "Filter > Filter";
        case CouplingType::RhythmToBlend:   return "Rhythm > Blend";
        case CouplingType::TriggerToReset:  return "Trigger > Reset";
        case CouplingType::SendToFX:        return "Send > FX";
        default:                            return "Unknown";
    }
}

} // namespace xo
```

### 2.2 EngineColorProfile

```cpp
namespace xo {

/// Visual identity for an engine module — used by the UI for slot coloring,
/// patch cable tinting, and coupling matrix highlighting.
struct EngineColorProfile
{
    juce::Colour primary;       // Main engine brand colour
    juce::Colour secondary;     // Accent / highlight colour
    juce::Colour background;    // Panel background tint
    juce::Colour cableColour;   // Patch cable colour when this engine is the source
};

} // namespace xo
```

### 2.3 Full SynthEngine Interface

```cpp
namespace xo {

/// The contract every engine module must implement to participate in the
/// XO_OX Mega-Tool. This interface formalizes what OddfeliX/OddOscar's EngineX
/// and EngineO already do informally.
///
/// Design principles:
///   - renderBlock takes a buffer + sample count, NOT the full processBlock
///     signature. The hub handles MIDI splitting and routing.
///   - getSampleForCoupling() is const and returns the latest sample. This
///     enables per-sample cross-engine modulation.
///   - createParameterLayout() lets each engine define its own parameters.
///     The hub merges them with namespaced prefixes.
///   - Voice management is exposed so the hub can throttle polyphony under
///     CPU pressure.
class SynthEngine
{
public:
    virtual ~SynthEngine() = default;

    //==================================================================
    // Lifecycle
    //==================================================================

    /// Called once when sample rate or block size changes. Engines must
    /// allocate all audio-thread resources here (buffers, filter states,
    /// delay lines). No allocation is permitted after this call.
    virtual void prepare(double sampleRate, int blockSize) = 0;

    /// Release all DSP resources. Called when the engine is being
    /// deactivated or swapped out of a slot.
    virtual void releaseResources() = 0;

    //==================================================================
    // Rendering
    //==================================================================

    /// Render audio into the provided buffer. The hub pre-splits MIDI
    /// per engine based on the active MIDI routing mode.
    /// @param buffer    Stereo buffer to render into (additive — clear first if needed)
    /// @param midi      MIDI events for this engine only
    virtual void renderBlock(juce::AudioBuffer<float>& buffer,
                             juce::MidiBuffer& midi) = 0;

    //==================================================================
    // Coupling I/O
    //==================================================================

    /// Return the most recent output sample for cross-engine coupling.
    /// Called once per sample in the per-sample coupling loop.
    /// Typically returns the post-filter, pre-FX mono sum of all voices.
    virtual float getSampleForCoupling() const = 0;

    /// Apply an incoming coupling modulation from another engine.
    /// Called before renderBlock (block-level mode) or inline during
    /// the per-sample loop. The value is pre-scaled by the coupling
    /// amount — the engine applies it directly to the target parameter.
    /// @param value   Modulation value (range depends on CouplingType)
    /// @param type    Which modulation route this value represents
    virtual void applyCouplingInput(float value, CouplingType type) = 0;

    //==================================================================
    // Parameter Interface
    //==================================================================

    /// Return the total number of parameters this engine owns.
    virtual int getParameterCount() const = 0;

    /// Return all parameter IDs this engine owns. IDs must be unique
    /// within the engine but need not include the engine prefix — the
    /// hub adds the prefix at registration time.
    virtual juce::StringArray getParameterIDs() const = 0;

    /// Create the APVTS parameter layout for this engine. The hub
    /// merges all engine layouts into a single APVTS with namespaced
    /// prefixes (e.g., "fat_mojo", "poss_oscAWaveform").
    virtual juce::AudioProcessorValueTreeState::ParameterLayout
        createParameterLayout() = 0;

    /// Snapshot parameters from the APVTS into internal state.
    /// Called once per processBlock before renderBlock. Engines should
    /// cache all parameter values into a ParamSnapshot struct for
    /// zero-cost per-sample access.
    virtual void snapshotParameters(
        const juce::AudioProcessorValueTreeState& apvts) = 0;

    //==================================================================
    // Identity
    //==================================================================

    /// Human-readable name (e.g., "OBESE", "OVERBITE", "ODDFELIX")
    virtual juce::String getModuleName() const = 0;

    /// Machine-readable identifier (e.g., "fat", "bite", "snap").
    /// Used for parameter namespacing, preset serialization, and
    /// engine registry lookup.
    virtual juce::String getModuleID() const = 0;

    /// Visual identity for UI rendering.
    virtual EngineColorProfile getColorProfile() const = 0;

    //==================================================================
    // State Serialization
    //==================================================================

    /// Serialize engine state for preset storage.
    virtual void getStateInformation(juce::MemoryBlock& destData) = 0;

    /// Restore engine state from serialized data.
    virtual void setStateInformation(const void* data, int sizeInBytes) = 0;

    //==================================================================
    // Voice Management
    //==================================================================

    /// Return the number of voices currently sounding.
    virtual int getActiveVoiceCount() const = 0;

    /// Set the maximum voice count. The hub calls this to throttle
    /// polyphony when multiple engines are active. The engine must
    /// respect this limit and steal voices as needed.
    /// @param maxVoices  New maximum (may be lower than the engine's default)
    virtual void setMaxVoices(int maxVoices) = 0;

    //==================================================================
    // Optional Overrides
    //==================================================================

    /// Whether this engine manages its own FX chain. If true, the
    /// engine's output bypasses the shared FX rack. Default: false.
    /// Engines like OVERDUB (XOverdub) override this to true because
    /// their FX architecture IS the instrument.
    virtual bool ownsEffects() const { return false; }

    /// Reset all voice and FX state immediately (panic).
    virtual void reset() { /* default: no-op */ }
};

} // namespace xo
```

---

## 3. Engine Registry System

### 3.1 EngineRegistry

The registry maps module IDs to factory functions. Engines are instantiated on demand when loaded into a slot. This decouples the mega-tool shell from specific engine implementations.

```cpp
namespace xo {

/// Factory function type — creates a new engine instance.
using EngineFactory = std::function<std::unique_ptr<SynthEngine>()>;

/// Central registry of all available engine types. Engines register
/// themselves at static initialization time via the REGISTER_ENGINE macro.
class EngineRegistry
{
public:
    /// Register an engine factory by module ID.
    /// @param moduleID   Unique identifier (e.g., "Obese", "Overbite", "OddfeliX")
    /// @param factory    Function that creates a new instance of the engine
    void registerEngine(const juce::String& moduleID, EngineFactory factory)
    {
        jassert(!factories.count(moduleID)); // No duplicate registration
        factories[moduleID] = std::move(factory);
    }

    /// Create a new engine instance by module ID.
    /// @return New engine instance, or nullptr if moduleID is not registered
    std::unique_ptr<SynthEngine> createEngine(const juce::String& moduleID) const
    {
        auto it = factories.find(moduleID);
        if (it != factories.end())
            return it->second();
        return nullptr;
    }

    /// Return all registered module IDs (sorted alphabetically).
    juce::StringArray getAvailableModuleIDs() const
    {
        juce::StringArray ids;
        for (const auto& [id, _] : factories)
            ids.add(id);
        ids.sort(false);
        return ids;
    }

    /// Check if a module ID is registered.
    bool hasEngine(const juce::String& moduleID) const
    {
        return factories.count(moduleID) > 0;
    }

    /// Singleton access. The registry is process-global.
    static EngineRegistry& getInstance()
    {
        static EngineRegistry instance;
        return instance;
    }

private:
    std::map<juce::String, EngineFactory> factories;
};

/// Convenience macro for engine registration at file scope.
/// Usage: REGISTER_ENGINE("fat", FatEngine)
#define REGISTER_ENGINE(moduleID, EngineClass) \
    static const bool _reg_##EngineClass = [] { \
        xo::EngineRegistry::getInstance().registerEngine( \
            moduleID, []() { return std::make_unique<EngineClass>(); }); \
        return true; \
    }();

} // namespace xo
```

### 3.2 Engine Slot System

The mega-tool has 4 engine slots. Each slot can hold any registered engine or remain empty.

```
┌──────────────────────────────────────────────────────────┐
│  Slot A        Slot B        Slot C        Slot D        │
│  ┌──────┐     ┌──────┐     ┌──────┐     ┌──────┐       │
│  │OBESE │     │OVRBIT│     │OVRDUB│     │(empty)│       │
│  │      │     │      │     │      │     │      │       │
│  │ 12%  │     │ 10%  │     │  8%  │     │  0%  │       │
│  └──────┘     └──────┘     └──────┘     └──────┘       │
│                                                          │
│  Active: 3                      Total CPU: ~36%          │
└──────────────────────────────────────────────────────────┘
```

**Slot rules:**

| Rule | Detail |
|------|--------|
| Max 4 active | Hard limit. UI greys out the "add engine" button at 4. |
| Empty = 0 CPU | No rendering, no parameter snapshotting, no coupling processing. |
| Any engine in any slot | Slot A can hold ONSET, Slot D can hold OBESE. Order affects chain routing only. |
| No duplicate engines | Each engine type can occupy at most one slot. |
| Slot order = chain order | In Chain routing mode, audio flows A→B→C→D. |

### 3.3 Hot-Swapping Engines

Swapping an engine in an occupied slot must not produce an audio glitch. The procedure uses a 50ms equal-power crossfade:

```
Timeline (50ms crossfade window):

Old engine ████████████████▓▓▓▓░░░░────────────────
                          ↑ fade out begins
New engine ────────────░░░░▓▓▓▓████████████████████
                          ↑ fade in begins
                               ↑ crossfade complete

0ms        25ms       50ms
│          │          │
prepare()  crossfade  releaseResources()
on new     active     on old engine
engine                (message thread)
```

**Procedure:**

1. **Message thread:** Create new engine via `EngineRegistry::createEngine()`. Call `prepare()` with current sample rate and block size.
2. **Audio thread:** Set crossfade flag. For the next ~50ms (2205 samples at 44.1kHz), render both old and new engines. Apply equal-power crossfade: old engine fades out with `cos(t * pi/2)`, new engine fades in with `sin(t * pi/2)`.
3. **Audio thread:** After crossfade completes, null the old engine pointer (atomic swap).
4. **Message thread:** Call `releaseResources()` on old engine, then destroy it.

```cpp
/// Crossfade state for glitch-free engine swapping.
struct SlotCrossfade
{
    std::unique_ptr<SynthEngine> outgoing;  // Engine being replaced
    int samplesRemaining = 0;               // Countdown
    static constexpr int kCrossfadeSamples = 2205; // ~50ms at 44.1kHz

    bool isActive() const { return samplesRemaining > 0; }

    /// Returns {oldGain, newGain} for the current sample.
    std::pair<float, float> getGains() const
    {
        float t = 1.0f - (float)samplesRemaining / (float)kCrossfadeSamples;
        float fadeOut = std::cos(t * juce::MathConstants<float>::halfPi);
        float fadeIn  = std::sin(t * juce::MathConstants<float>::halfPi);
        return { fadeOut, fadeIn };
    }
};
```

---

## 4. Coupling Matrix Architecture

### 4.1 MegaCouplingMatrix

The existing `XO::CouplingMatrix` handles a single pair (X-to-O and O-to-X) with a single `coupling` float. The `MegaCouplingMatrix` generalizes this to support arbitrary engine pairs with multiple independent coupling routes per pair.

```cpp
namespace xo {

/// A single modulation route between two engines.
struct CouplingRoute
{
    CouplingType type;          // What kind of modulation
    float amount     = 0.0f;    // 0.0–1.0 modulation depth
    bool  enabled    = true;    // Can be toggled without losing amount
    bool  isNormalled = true;   // True if this is a default connection
    bool  bidirectional = false;// True if modulation flows both ways
};

/// All coupling routes between a specific pair of engine slots.
struct CouplingPair
{
    int slotA = -1;                           // Source slot index (0-3)
    int slotB = -1;                           // Target slot index (0-3)
    std::vector<CouplingRoute> routes;        // All routes for this pair
    float masterAmount = 1.0f;                // Scales all routes (0.0–1.0)

    /// Return the effective amount for a specific route.
    /// Effective = route.amount × masterAmount × (enabled ? 1 : 0)
    float getEffectiveAmount(int routeIndex) const
    {
        if (routeIndex < 0 || routeIndex >= (int)routes.size())
            return 0.0f;
        const auto& r = routes[(size_t)routeIndex];
        return r.enabled ? (r.amount * masterAmount) : 0.0f;
    }
};

/// The full coupling matrix for N active engines.
/// For N engines, there are N×(N-1)/2 coupling pairs.
/// Each pair can have multiple independent routes.
class MegaCouplingMatrix
{
public:
    static constexpr int kMaxSlots = 4;
    static constexpr int kMaxPairs = kMaxSlots * (kMaxSlots - 1) / 2; // 6

    //==================================================================
    // Configuration
    //==================================================================

    /// Set up coupling pairs for the currently active engines.
    /// Called when engines are loaded/swapped.
    void configure(const std::array<SynthEngine*, kMaxSlots>& engines)
    {
        activePairs.clear();
        activeEngines = engines;

        for (int a = 0; a < kMaxSlots; ++a)
        {
            if (!engines[a]) continue;
            for (int b = a + 1; b < kMaxSlots; ++b)
            {
                if (!engines[b]) continue;

                CouplingPair pair;
                pair.slotA = a;
                pair.slotB = b;
                pair.routes = getNormalledRoutes(
                    engines[a]->getModuleID(),
                    engines[b]->getModuleID()
                );
                pair.masterAmount = 1.0f;
                activePairs.push_back(std::move(pair));
            }
        }
    }

    /// Override a specific route on a pair. Marks the route as
    /// non-normalled so the user's setting is preserved.
    void setRoute(int pairIndex, int routeIndex, float amount)
    {
        if (pairIndex < (int)activePairs.size()
            && routeIndex < (int)activePairs[pairIndex].routes.size())
        {
            auto& route = activePairs[pairIndex].routes[routeIndex];
            route.amount = juce::jlimit(0.0f, 1.0f, amount);
            route.isNormalled = false; // User override
        }
    }

    /// Reset a route to its normalled default.
    void resetRouteToDefault(int pairIndex, int routeIndex)
    {
        if (pairIndex < (int)activePairs.size())
        {
            auto& pair = activePairs[pairIndex];
            if (routeIndex < (int)pair.routes.size())
            {
                auto defaults = getNormalledRoutes(
                    activeEngines[pair.slotA]->getModuleID(),
                    activeEngines[pair.slotB]->getModuleID()
                );
                if (routeIndex < (int)defaults.size())
                    pair.routes[routeIndex] = defaults[routeIndex];
            }
        }
    }

    //==================================================================
    // Per-Sample Processing
    //==================================================================

    /// Process all coupling routes for one sample. Called inside
    /// the per-sample render loop.
    void processSample()
    {
        for (const auto& pair : activePairs)
        {
            auto* engineA = activeEngines[pair.slotA];
            auto* engineB = activeEngines[pair.slotB];
            if (!engineA || !engineB) continue;

            float sampleA = engineA->getSampleForCoupling();
            float sampleB = engineB->getSampleForCoupling();

            for (size_t r = 0; r < pair.routes.size(); ++r)
            {
                const auto& route = pair.routes[r];
                float amount = pair.getEffectiveAmount((int)r);
                if (amount < 1e-6f) continue;

                // Compute modulation value based on coupling type
                float modValue = computeModulation(
                    route.type, sampleA, sampleB, amount
                );

                // Apply: A→B
                engineB->applyCouplingInput(modValue, route.type);

                // Apply: B→A (bidirectional only)
                if (route.bidirectional)
                {
                    float reverseValue = computeModulation(
                        route.type, sampleB, sampleA, amount
                    );
                    // Feedback prevention: reduce gain on reverse path
                    reverseValue *= kBidirectionalGainReduction;
                    engineA->applyCouplingInput(reverseValue, route.type);
                }
            }
        }
    }

    //==================================================================
    // Block-Level Processing (CPU fallback)
    //==================================================================

    /// Process coupling at block rate. Called once per processBlock
    /// instead of per-sample. Uses envelope followers for smoother
    /// modulation at reduced CPU cost.
    void processBlock(int numSamples)
    {
        for (const auto& pair : activePairs)
        {
            auto* engineA = activeEngines[pair.slotA];
            auto* engineB = activeEngines[pair.slotB];
            if (!engineA || !engineB) continue;

            float sampleA = engineA->getSampleForCoupling();
            float sampleB = engineB->getSampleForCoupling();

            for (size_t r = 0; r < pair.routes.size(); ++r)
            {
                float amount = pair.getEffectiveAmount((int)r);
                if (amount < 1e-6f) continue;

                const auto& route = pair.routes[r];
                float modValue = computeModulation(
                    route.type, sampleA, sampleB, amount
                );

                // Smooth modulation value over the block
                engineB->applyCouplingInput(modValue, route.type);

                if (route.bidirectional)
                {
                    float reverseValue = computeModulation(
                        route.type, sampleB, sampleA, amount
                    );
                    reverseValue *= kBidirectionalGainReduction;
                    engineA->applyCouplingInput(reverseValue, route.type);
                }
            }
        }
    }

    //==================================================================
    // Access
    //==================================================================

    const std::vector<CouplingPair>& getPairs() const { return activePairs; }
    int getActivePairCount() const { return (int)activePairs.size(); }

private:
    std::vector<CouplingPair> activePairs;
    std::array<SynthEngine*, kMaxSlots> activeEngines = {};

    /// Gain reduction applied to the reverse path of bidirectional
    /// coupling to prevent feedback runaway.
    static constexpr float kBidirectionalGainReduction = 0.7f;

    //------------------------------------------------------------------
    // Modulation Computation
    //------------------------------------------------------------------

    /// Compute the modulation value for a given coupling type.
    /// @param type     The coupling route type
    /// @param srcSample Source engine's coupling output
    /// @param tgtSample Target engine's coupling output (used for ring mod)
    /// @param amount   Pre-computed effective amount
    static float computeModulation(CouplingType type,
                                   float srcSample,
                                   float tgtSample,
                                   float amount)
    {
        switch (type)
        {
            case CouplingType::AmpToFilter:
            {
                // Source amplitude ducks/opens target filter.
                // Invert for classic dub pump: when source hits,
                // target filter ducks then opens.
                float ducked = 1.0f - std::abs(srcSample);
                return (ducked - 0.5f) * 2.0f * amount;
            }

            case CouplingType::AmpToPitch:
            {
                // Source amplitude → target pitch offset (semitones).
                // Max ±1 semitone at full amount.
                return std::abs(srcSample) * amount * 1.0f;
            }

            case CouplingType::AmpToChoke:
            {
                // Binary: if source amplitude > threshold, choke target.
                // Returns 1.0 (choke) or 0.0 (no choke).
                return (std::abs(srcSample) > 0.5f * (1.0f - amount))
                       ? 1.0f : 0.0f;
            }

            case CouplingType::LFOToPitch:
            {
                // Source sample (assumed to be LFO range -1..1)
                // → target pitch. Max ±0.5 semitones at full amount.
                return srcSample * amount * 0.5f;
            }

            case CouplingType::EnvToMorph:
            {
                // Source envelope level → target morph position.
                // Unipolar 0..1, scaled by amount.
                return std::abs(srcSample) * amount;
            }

            case CouplingType::EnvToDecay:
            {
                // Source envelope → target decay multiplier.
                // Higher source = shorter target decay.
                return (1.0f - std::abs(srcSample)) * amount;
            }

            case CouplingType::AudioToFM:
            {
                // Raw audio → FM input. Bipolar, scaled by amount.
                // Amount acts as FM index multiplier.
                return srcSample * amount;
            }

            case CouplingType::AudioToRing:
            {
                // Ring modulation: source × target.
                return srcSample * tgtSample * amount;
            }

            case CouplingType::FilterToFilter:
            {
                // Source filter output → target filter input.
                // Direct audio routing, scaled by amount.
                return srcSample * amount;
            }

            case CouplingType::RhythmToBlend:
            {
                // Source rhythmic signal → target blend position.
                // Unipolar, envelope-followed.
                return std::abs(srcSample) * amount;
            }

            case CouplingType::TriggerToReset:
            {
                // Binary trigger: source crosses zero upward → reset.
                // Returns 1.0 on trigger, 0.0 otherwise.
                return (srcSample > 0.1f * (1.0f - amount)) ? 1.0f : 0.0f;
            }

            case CouplingType::SendToFX:
            {
                // Direct audio send to OVERDUB engine's FX input.
                // Scaled by amount (acts as send level).
                return srcSample * amount;
            }

            default:
                return 0.0f;
        }
    }

    //------------------------------------------------------------------
    // Normalled Route Tables
    //------------------------------------------------------------------

    /// Return the default normalled routes for a given engine pairing.
    /// These are active by default and can be overridden by the user.
    static std::vector<CouplingRoute> getNormalledRoutes(
        const juce::String& idA, const juce::String& idB)
    {
        // See Section 8 for the full normalled routing table.
        // This method encodes those defaults programmatically.

        std::vector<CouplingRoute> routes;

        // ODDFELIX + ODDOSCAR (OddfeliX/OddOscar legacy coupling)
        if ((idA == "snap" && idB == "morph")
         || (idA == "morph" && idB == "snap"))
        {
            routes.push_back({ CouplingType::AmpToFilter, 0.30f,
                               true, true, false });
            routes.push_back({ CouplingType::LFOToPitch,  0.15f,
                               true, true, false });
        }
        // ONSET + any melodic
        else if (idA == "onset" || idB == "onset")
        {
            routes.push_back({ CouplingType::AmpToFilter, 0.15f,
                               true, true, false });
        }
        // OBESE + OVERBITE
        else if ((idA == "fat" && idB == "bite")
              || (idA == "bite" && idB == "fat"))
        {
            routes.push_back({ CouplingType::FilterToFilter, 0.0f,
                               false, true, false }); // chain mode default off
        }
        // Any + OVERDUB
        else if (idA == "dub" || idB == "dub")
        {
            routes.push_back({ CouplingType::SendToFX, 0.0f,
                               false, true, false }); // user-activated
        }
        // ODYSSEY + any
        else if (idA == "drift" || idB == "drift")
        {
            routes.push_back({ CouplingType::LFOToPitch, 0.10f,
                               true, true, false });
            routes.push_back({ CouplingType::EnvToMorph, 0.15f,
                               true, true, false });
        }

        return routes;
    }
};

} // namespace xo
```

### 4.2 Coupling Amount Scaling

The coupling `amount` (0.0-1.0) scales the modulation depth linearly within type-specific ranges:

| CouplingType | Amount = 0.0 | Amount = 0.5 | Amount = 1.0 | Units |
|-------------|-------------|-------------|-------------|-------|
| AmpToFilter | No mod | Moderate pump | Full sidechain duck | Bipolar filter offset |
| AmpToPitch | No mod | ±0.5 semitone | ±1 semitone | Semitones |
| LFOToPitch | No mod | ±0.25 semitone | ±0.5 semitone | Semitones |
| EnvToMorph | No mod | 50% morph range | Full morph range | 0-1 position |
| AudioToFM | No mod | Moderate FM | Deep FM distortion | FM index |
| AudioToRing | No mod | Partial ring mod | Full ring multiplication | Gain |
| FilterToFilter | No audio pass | 50% bleed | Full serial cascade | Linear gain |
| SendToFX | No send | -6dB send | Unity send | Linear gain |

### 4.3 Bidirectional vs Unidirectional Rules

| Route Type | Direction | Rationale |
|-----------|-----------|-----------|
| AmpToFilter | Unidirectional | Bidirectional pump creates unstable feedback |
| AmpToPitch | Unidirectional | Pitch feedback is musically destructive |
| AmpToChoke | Unidirectional | Choke is inherently one-way |
| LFOToPitch | **Bidirectional allowed** | Both engines drifting against each other is musical |
| EnvToMorph | Unidirectional | Bidirectional morph is confusing, not musical |
| AudioToFM | Unidirectional | FM feedback requires careful gain staging — see below |
| AudioToRing | **Bidirectional inherent** | Ring mod is commutative: A×B = B×A |
| FilterToFilter | Unidirectional | Serial cascading has defined direction |
| RhythmToBlend | Unidirectional | Rhythm source drives blend target |
| TriggerToReset | Unidirectional | Reset is inherently one-way |
| SendToFX | Unidirectional | Send routing has defined direction |

### 4.4 Feedback Prevention

Bidirectional coupling and audio-rate routes (AudioToFM, AudioToRing) risk runaway feedback. Three safeguards are applied:

1. **Gain reduction on reverse path:** Bidirectional routes apply `kBidirectionalGainReduction` (0.7) to the reverse direction. This ensures the feedback loop decays rather than grows.

2. **Soft limiter on coupling output:** After all coupling routes are applied, each engine's coupling output is soft-limited to [-1, 1] via `tanh()`. This prevents DC offset accumulation.

3. **DC blocker on AudioToFM:** FM coupling passes through a DC blocker (`y[n] = x[n] - x[n-1] + 0.995 * y[n-1]`) to prevent pitch drift from DC components in the modulator.

---

## 5. Routing Architecture

### 5.1 Full Signal Flow

```
┌─────────────────────────────────────────────────────────────────────┐
│                       MegaToolProcessor                              │
│                                                                      │
│  MIDI Input ──────────────────────────┐                              │
│                                       │                              │
│                              ┌────────▼─────────┐                   │
│                              │  MIDI Router      │                   │
│                              │  (all/split/ch)   │                   │
│                              └──┬──┬──┬──┬──────┘                   │
│                                 │  │  │  │                           │
│  ┌──────────┐  Coupling  ┌──────────┐  Coupling  ┌──────────┐      │
│  │ Slot A   │◄── AB ────►│ Slot B   │◄── BC ────►│ Slot C   │      │
│  │ (engine) │            │ (engine) │            │ (engine) │      │
│  └────┬─────┘            └────┬─────┘            └────┬─────┘      │
│       │        Coupling       │        Coupling       │             │
│       │◄────── AC ──────────►│                        │             │
│       │                      │        Coupling       │             │
│       │◄────── AD ──────────►│◄────── BD ──────────►│             │
│       │                      │                       │             │
│       │                      │                 ┌────┴─────┐       │
│       │                      │                 │ Slot D   │       │
│       │                      │                 │ (engine) │       │
│       │                      │                 └────┬─────┘       │
│       │                      │                      │              │
│       ▼                      ▼                      ▼              │
│  ┌──────────────────────────────────────────────────────────────┐  │
│  │                    Routing Engine                             │  │
│  │                                                              │  │
│  │  ┌─ INDEPENDENT ─┐  ┌── SHARED FX ──┐  ┌──── CHAIN ─────┐ │  │
│  │  │ A → FX_A → ─┐ │  │ A ──┐         │  │ A → B.filter   │ │  │
│  │  │ B → FX_B → ─┤ │  │ B ──┤→ SUM →  │  │   → C.fx      │ │  │
│  │  │ C → FX_C → ─┤ │  │ C ──┤  Shared │  │   → D.filter  │ │  │
│  │  │ D → FX_D → ─┤ │  │ D ──┘  FX     │  │   → out       │ │  │
│  │  │      SUM     │ │  │        Rack   │  │               │ │  │
│  │  └──────┬───────┘ │  └────┬──────────┘  └───────┬───────┘ │  │
│  │         ▼              ▼                    ▼              │  │
│  └─────────┬──────────────┬────────────────────┬─────────────┘  │
│            │              │                    │                  │
│            └──────────────┼────────────────────┘                 │
│                           ▼                                      │
│                  ┌────────────────┐                               │
│                  │  Master Bus    │                               │
│                  │  Volume        │                               │
│                  │  Stereo Width  │                               │
│                  │  Soft Limiter  │                               │
│                  └───────┬────────┘                               │
│                          │                                        │
│                    [Master Out]                                   │
└─────────────────────────────────────────────────────────────────┘
```

### 5.2 Three Routing Modes

#### Independent Mode

Each engine renders into its own stereo buffer. Engines with `ownsEffects() == true` apply their own FX chain. Engines with `ownsEffects() == false` can optionally pass through a per-slot FX instance (if CPU budget allows). All buffers sum to the master bus.

```
Slot A ─→ [A's own FX if ownsEffects] ─→ ─┐
Slot B ─→ [Shared FX copy B]          ─→ ─┤─→ Master Sum ─→ Master Bus
Slot C ─→ [Shared FX copy C]          ─→ ─┤
Slot D ─→ [D's own FX if ownsEffects] ─→ ─┘
```

- **CPU cost:** Highest. Multiple FX instances running in parallel.
- **Use case:** Each engine needs a different reverb/delay character.
- **Constraint:** With 4 engines, per-engine FX may exceed CPU budget. The system warns the user and suggests Shared FX mode.

#### Shared FX Mode

All engine outputs sum to a single stereo bus. That bus passes through one shared FX rack (delay, reverb, chorus, LoFi). Engines with `ownsEffects() == true` still apply their own FX but their output is summed before the shared rack — the shared rack processes their output too, unless the engine is flagged as FX-isolated.

```
Slot A ─→ ─┐
Slot B ─→ ─┤─→ Engine Sum ─→ [Shared FX Rack] ─→ Master Bus
Slot C ─→ ─┤     Delay / Reverb / Chorus / LoFi
Slot D ─→ ─┘
```

- **CPU cost:** Lowest. One FX instance for all engines.
- **Use case:** Default for 3-4 engine configurations. Cohesive FX character.
- **Constraint:** All engines share the same reverb and delay settings. Per-engine FX sends control wet/dry balance.

#### Chain Mode

Audio flows through engine slots in series. Slot A's output becomes Slot B's input. Each engine can process the incoming audio through its filter, character stages, or FX chain.

```
Slot A ─→ Slot B's filter/character ─→ Slot C's FX chain ─→ Slot D ─→ Master Bus
```

- **CPU cost:** Moderate (engines process sequentially, not in parallel).
- **Use case:** OBESE → OVERBITE (13-osc width through character stages). Any source → OVERDUB (through tape delay + spring reverb).
- **Constraint:** Per-sample rendering only (block-level coupling is incompatible with chaining). Chain order = slot order (A→B→C→D). Empty slots are skipped.

**Chain mode requires an additional interface method:**

```cpp
/// Process external audio input through this engine's filter/FX chain.
/// Used in Chain routing mode. Engines that don't support chaining
/// pass audio through unchanged.
/// @param buffer  Audio to process (in-place modification)
virtual void processExternalAudio(juce::AudioBuffer<float>& buffer)
{
    // Default: passthrough. Override in engines that support chaining.
    juce::ignoreUnused(buffer);
}
```

---

## 6. The MegaToolProcessor Class

### 6.1 Class Definition

```cpp
#pragma once
#include <JuceHeader.h>
#include "Shared/SynthEngine.h"
#include "Shared/Coupling/MegaCouplingMatrix.h"
#include "Shared/EngineRegistry.h"

namespace xo {

/// Routing mode for how engine outputs are mixed and processed.
enum class RoutingMode
{
    Independent,  // Each engine → own FX → mix to master
    SharedFX,     // All engines → sum → shared FX rack → master
    Chain          // A → B's processing → C's processing → master
};

/// Shared FX rack containing one instance of each common effect.
struct SharedFXRack
{
    // These are the shared instances — engines with ownsEffects()==false
    // route through these.
    std::unique_ptr<DubDelay>         delay;
    std::unique_ptr<LushReverb>       reverb;
    std::unique_ptr<ChorusModule>     chorus;
    std::unique_ptr<LoFiModule>       lofi;
    std::unique_ptr<MasterCompressor> compressor;

    void prepare(double sampleRate, int blockSize);
    void process(juce::AudioBuffer<float>& buffer, int numSamples);
    void reset();
};

/// The top-level audio processor for the XO_OX Mega-Tool.
/// Owns engine slots, coupling matrix, FX rack, and routing logic.
class MegaToolProcessor : public juce::AudioProcessor
{
public:
    static constexpr int kMaxSlots = 4;

    MegaToolProcessor();
    ~MegaToolProcessor() override;

    //==================================================================
    // AudioProcessor Overrides
    //==================================================================

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>& buffer,
                      juce::MidiBuffer& midi) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "XO_OX Mega-Tool"; }
    bool acceptsMidi() const override  { return true; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 15.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    //==================================================================
    // Engine Management (message thread only)
    //==================================================================

    /// Load an engine into a slot. Creates a new instance from the
    /// registry and prepares it for the current sample rate.
    void loadEngine(int slot, const juce::String& moduleID);

    /// Swap an engine in an occupied slot with crossfade.
    void swapEngine(int slot, const juce::String& moduleID);

    /// Deactivate a slot. The engine is destroyed and CPU freed.
    void deactivateSlot(int slot);

    /// Get the engine in a slot (nullptr if empty).
    SynthEngine* getEngine(int slot) const;

    /// Get the number of currently active engine slots.
    int getActiveEngineCount() const;

    //==================================================================
    // Routing
    //==================================================================

    void setRoutingMode(RoutingMode mode);
    RoutingMode getRoutingMode() const { return routingMode; }

    //==================================================================
    // Coupling Access
    //==================================================================

    MegaCouplingMatrix& getCouplingMatrix() { return couplingMatrix; }
    const MegaCouplingMatrix& getCouplingMatrix() const
    { return couplingMatrix; }

private:
    //------------------------------------------------------------------
    // Engine Slots
    //------------------------------------------------------------------

    std::array<std::unique_ptr<SynthEngine>, kMaxSlots> engines;
    std::array<SlotCrossfade, kMaxSlots> crossfades;

    //------------------------------------------------------------------
    // Coupling & Routing
    //------------------------------------------------------------------

    MegaCouplingMatrix couplingMatrix;
    RoutingMode routingMode = RoutingMode::SharedFX;

    //------------------------------------------------------------------
    // FX
    //------------------------------------------------------------------

    SharedFXRack fxRack;

    //------------------------------------------------------------------
    // MIDI
    //------------------------------------------------------------------

    enum class MidiRoutingMode { All, SplitKeyboard, ChannelBased };
    MidiRoutingMode midiRouting = MidiRoutingMode::All;
    std::array<juce::MidiBuffer, kMaxSlots> perEngineMidi;

    void splitMidi(const juce::MidiBuffer& source);

    //------------------------------------------------------------------
    // Audio Buffers (pre-allocated, never resized on audio thread)
    //------------------------------------------------------------------

    std::array<juce::AudioBuffer<float>, kMaxSlots> engineBuffers;
    juce::AudioBuffer<float> masterBuffer;

    //------------------------------------------------------------------
    // Voice Management
    //------------------------------------------------------------------

    void updateVoiceBudgets();

    //------------------------------------------------------------------
    // Parameters
    //------------------------------------------------------------------

    juce::AudioProcessorValueTreeState apvts;

    static juce::AudioProcessorValueTreeState::ParameterLayout
        buildParameterLayout(
            const std::array<std::unique_ptr<SynthEngine>, kMaxSlots>& engines);

    //------------------------------------------------------------------
    // State
    //------------------------------------------------------------------

    double currentSampleRate = 44100.0;
    int currentBlockSize = 512;
    bool usPerSampleCoupling = true; // false when 3+ engines active in eco mode
};

} // namespace xo
```

### 6.2 processBlock Implementation

```cpp
void MegaToolProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                      juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const int numSamples = buffer.getNumSamples();

    //------------------------------------------------------------------
    // 1. Snapshot all engine parameters from APVTS
    //------------------------------------------------------------------
    for (int s = 0; s < kMaxSlots; ++s)
    {
        if (engines[s])
            engines[s]->snapshotParameters(apvts);
    }

    //------------------------------------------------------------------
    // 2. Split MIDI to target engines
    //------------------------------------------------------------------
    splitMidi(midi);

    //------------------------------------------------------------------
    // 3. Clear per-engine buffers
    //------------------------------------------------------------------
    for (int s = 0; s < kMaxSlots; ++s)
        engineBuffers[s].clear();

    //------------------------------------------------------------------
    // 4. Render each engine + apply coupling
    //------------------------------------------------------------------
    if (routingMode == RoutingMode::Chain)
    {
        // Chain mode: sequential rendering, each engine processes
        // the previous engine's output.
        renderChainMode(numSamples);
    }
    else if (usPerSampleCoupling && couplingMatrix.getActivePairCount() > 0)
    {
        // Per-sample coupling: render all engines sample-by-sample
        // with coupling matrix applied between each sample.
        renderPerSampleMode(numSamples);
    }
    else
    {
        // Block-level: render each engine independently, apply
        // coupling at block rate.
        renderBlockMode(numSamples);
    }

    //------------------------------------------------------------------
    // 5. Handle crossfades for engine swaps
    //------------------------------------------------------------------
    for (int s = 0; s < kMaxSlots; ++s)
    {
        if (crossfades[s].isActive())
            applyCrossfade(s, numSamples);
    }

    //------------------------------------------------------------------
    // 6. Route through FX based on routing mode
    //------------------------------------------------------------------
    buffer.clear();

    switch (routingMode)
    {
        case RoutingMode::Independent:
        {
            // Each engine buffer already has its FX applied (if ownsEffects).
            // Sum all engine buffers to master.
            for (int s = 0; s < kMaxSlots; ++s)
            {
                if (engines[s])
                {
                    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
                        buffer.addFrom(ch, 0, engineBuffers[s], ch, 0,
                                       numSamples);
                }
            }
            break;
        }

        case RoutingMode::SharedFX:
        {
            // Sum all engine buffers, then apply shared FX rack.
            for (int s = 0; s < kMaxSlots; ++s)
            {
                if (engines[s])
                {
                    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
                        buffer.addFrom(ch, 0, engineBuffers[s], ch, 0,
                                       numSamples);
                }
            }
            fxRack.process(buffer, numSamples);
            break;
        }

        case RoutingMode::Chain:
        {
            // Chain mode output is already in the last active slot's buffer.
            int lastSlot = findLastActiveSlot();
            if (lastSlot >= 0)
            {
                for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
                    buffer.copyFrom(ch, 0, engineBuffers[lastSlot], ch, 0,
                                    numSamples);
            }
            break;
        }
    }

    //------------------------------------------------------------------
    // 7. Master bus processing
    //------------------------------------------------------------------
    applyMasterBus(buffer, numSamples);
}
```

### 6.3 Per-Sample Render Loop

This is the performance-critical path that enables tight cross-engine modulation:

```cpp
void MegaToolProcessor::renderPerSampleMode(int numSamples)
{
    // Collect active engine pointers for tight inner loop
    SynthEngine* active[kMaxSlots] = {};
    int activeCount = 0;
    for (int s = 0; s < kMaxSlots; ++s)
    {
        if (engines[s])
            active[activeCount++] = engines[s].get();
    }

    if (activeCount == 0) return;

    // Per-sample loop
    for (int sample = 0; sample < numSamples; ++sample)
    {
        // Create single-sample MIDI buffers for each engine
        // (extract events at this sample position)
        for (int e = 0; e < activeCount; ++e)
        {
            // Render one sample from each engine
            juce::AudioBuffer<float> singleSample(2, 1);
            singleSample.clear();

            juce::MidiBuffer singleMidi;
            extractMidiForSample(perEngineMidi[e], sample, singleMidi);

            active[e]->renderBlock(singleSample, singleMidi);

            // Write to per-engine buffer
            for (int ch = 0; ch < 2; ++ch)
                engineBuffers[e].setSample(ch, sample,
                    singleSample.getSample(ch, 0));
        }

        // Apply coupling matrix between all active pairs
        couplingMatrix.processSample();
    }
}
```

### 6.4 Block-Level Render (CPU Fallback)

```cpp
void MegaToolProcessor::renderBlockMode(int numSamples)
{
    // Render each engine independently as a full block
    for (int s = 0; s < kMaxSlots; ++s)
    {
        if (!engines[s]) continue;

        engineBuffers[s].clear();
        engines[s]->renderBlock(engineBuffers[s], perEngineMidi[s]);
    }

    // Apply coupling at block rate
    if (couplingMatrix.getActivePairCount() > 0)
        couplingMatrix.processBlock(numSamples);
}
```

### 6.5 Chain Mode Render

```cpp
void MegaToolProcessor::renderChainMode(int numSamples)
{
    // Find the first active slot
    int firstSlot = -1;
    for (int s = 0; s < kMaxSlots; ++s)
    {
        if (engines[s]) { firstSlot = s; break; }
    }
    if (firstSlot < 0) return;

    // Render the first engine normally (it generates audio)
    engineBuffers[firstSlot].clear();
    engines[firstSlot]->renderBlock(engineBuffers[firstSlot],
                                     perEngineMidi[firstSlot]);

    // Each subsequent engine processes the previous engine's output
    int prevSlot = firstSlot;
    for (int s = firstSlot + 1; s < kMaxSlots; ++s)
    {
        if (!engines[s]) continue;

        // Copy previous output into this slot's buffer
        engineBuffers[s].makeCopyOf(engineBuffers[prevSlot]);

        // The engine processes the incoming audio (filter, character, FX)
        engines[s]->processExternalAudio(engineBuffers[s]);

        // Also render the engine's own voices and mix in
        juce::AudioBuffer<float> ownVoices(2, numSamples);
        ownVoices.clear();
        engines[s]->renderBlock(ownVoices, perEngineMidi[s]);

        for (int ch = 0; ch < 2; ++ch)
            engineBuffers[s].addFrom(ch, 0, ownVoices, ch, 0, numSamples);

        prevSlot = s;
    }

    // Apply coupling at block rate (per-sample not compatible with chain)
    if (couplingMatrix.getActivePairCount() > 0)
        couplingMatrix.processBlock(numSamples);
}
```

---

## 7. Parameter Namespacing Strategy

### 7.1 Naming Convention

Every parameter in the mega-tool follows this pattern:

```
{enginePrefix}_{parameterName}
```

| Prefix | Engine | Example |
|--------|--------|---------|
| `fat` | OBESE (XObese) | `fat_mojo`, `fat_morphPosition`, `fat_filterCutoff` |
| `poss` | OVERBITE (XOverbite) | `poss_oscAWaveform`, `poss_oscMix`, `poss_filterCutoff` |
| `snap` | ODDFELIX (OddfeliX/OddOscar EngX) | `snap_attack`, `snap_filterCutoff`, `snap_decay` |
| `morph` | ODDOSCAR (OddfeliX/OddOscar EngO) | `morph_position`, `morph_bloom`, `morph_filterCutoff` |
| `dub` | OVERDUB (XOverdub) | `dub_sendAmount`, `dub_delayTime`, `dub_delayFeedback` |
| `drift` | ODYSSEY (XOdyssey) | `drift_journey`, `drift_shimmer`, `drift_formantVowel` |
| `onset` | ONSET (XOnset) | `onset_v1Blend`, `onset_v1Pitch`, `onset_v1Decay` |

### 7.2 Coupling Parameters

Coupling parameters follow a structured naming pattern:

```
coupling_{slotPair}_{routeName}
```

| Parameter ID | Type | Range | Purpose |
|-------------|------|-------|---------|
| `coupling_AB_master` | Float | 0.0-1.0 | Master amount for Slot A ↔ B pair |
| `coupling_AB_ampToFilter` | Float | 0.0-1.0 | Amp→Filter route on pair AB |
| `coupling_AB_lfoToPitch` | Float | 0.0-1.0 | LFO→Pitch route on pair AB |
| `coupling_AC_master` | Float | 0.0-1.0 | Master amount for Slot A ↔ C pair |
| `coupling_BC_master` | Float | 0.0-1.0 | Master amount for Slot B ↔ C pair |
| ... | | | (up to 6 pairs × ~12 routes = ~72 coupling params max) |

### 7.3 Shared FX Parameters

```
fx_{effectName}_{parameterName}
```

| Parameter ID | Type | Range | Default |
|-------------|------|-------|---------|
| `fx_delay_time` | Float | 0.01-2.0 s | 0.375 |
| `fx_delay_feedback` | Float | 0.0-1.1 | 0.4 |
| `fx_delay_mix` | Float | 0.0-1.0 | 0.0 |
| `fx_reverb_size` | Float | 0.0-1.0 | 0.5 |
| `fx_reverb_damping` | Float | 0.0-1.0 | 0.5 |
| `fx_reverb_mix` | Float | 0.0-1.0 | 0.3 |
| `fx_chorus_rate` | Float | 0.1-5.0 Hz | 1.0 |
| `fx_chorus_depth` | Float | 0.0-1.0 | 0.5 |
| `fx_chorus_mix` | Float | 0.0-1.0 | 0.0 |
| `fx_lofi_bits` | Float | 2-16 | 16 |
| `fx_lofi_rate` | Float | 500-44100 Hz | 44100 |
| `fx_lofi_mix` | Float | 0.0-1.0 | 0.0 |

### 7.4 Master Parameters

```
master_{parameterName}
```

| Parameter ID | Type | Range | Default |
|-------------|------|-------|---------|
| `master_volume` | Float | -inf to +6 dB | 0 dB |
| `master_width` | Float | 0.0-2.0 | 1.0 (normal stereo) |
| `master_limiter` | Bool | on/off | on |
| `master_routingMode` | Choice | Independent/Shared/Chain | Shared |
| `master_midiRouting` | Choice | All/Split/Channel | All |

### 7.5 Total Parameter Budget

| Category | Count | Notes |
|----------|-------|-------|
| Engine OBESE | 45 | Prefixed `fat_` |
| Engine OVERBITE | 122 | Prefixed `poss_` |
| Engine ODDFELIX | 26 | Prefixed `snap_` |
| Engine ODDOSCAR | 26 | Prefixed `morph_` |
| Engine OVERDUB | 38 | Prefixed `dub_` |
| Engine ODYSSEY | 130 | Prefixed `drift_` |
| Engine ONSET | 110 | Prefixed `onset_` |
| **Subtotal engines** | **497** | Only active engines consume APVTS memory |
| Coupling (6 pairs × 12 routes + 6 masters) | 78 | `coupling_*` |
| Shared FX | 12 | `fx_*` |
| Master | 5 | `master_*` |
| **Grand total (all engines loaded)** | **592** | Theoretical max — never all active |
| **Typical (2 engines + coupling)** | **~110-200** | Practical range |

**APVTS strategy:** Parameters are registered in the APVTS at startup for all *registered* engines, not all *active* engines. This avoids dynamic APVTS rebuilding (which JUCE does not support). Inactive engine parameters have no listeners and consume negligible CPU.

---

## 8. Normalled Routing Defaults

These are the pre-wired connections active by default for every common engine pairing. They follow the Moog Matriarch pattern: everything sounds correct without user intervention. Users can override any route amount or disable it entirely.

### 8.1 Default Connection Table

| Pair | Default Route | Amount | Direction | Musical Purpose |
|------|--------------|--------|-----------|-----------------|
| **ODDFELIX + ODDOSCAR** | AmpToFilter | 30% | ODDFELIX→ODDOSCAR | Dub pump — percussive hits duck pad filter |
| **ODDFELIX + ODDOSCAR** | LFOToPitch | 15% | ODDOSCAR→ODDFELIX | Organic drift — pad LFO wobbles pluck pitch |
| **ONSET + ODDOSCAR** | AmpToFilter | 15% | ONSET→ODDOSCAR | Kick pumps pad brightness |
| **ONSET + ODDFELIX** | AmpToFilter | 10% | ONSET→ODDFELIX | Rhythmic filtering on plucks |
| **ONSET + ODYSSEY** | LFOToPitch | 10% | ODYSSEY→ONSET | Pad LFO subtly detunes drums |
| **ONSET + OVERDUB** | SendToFX | 0% | ONSET→OVERDUB | User-activated: drums through dub FX |
| **OBESE + OVERBITE** | FilterToFilter | 0% | OBESE→OVERBITE | User-activated: width through character (chain mode) |
| **OBESE + ODDOSCAR** | EnvToMorph | 15% | OBESE→ODDOSCAR | OBESE dynamics drive pad morph position |
| **OBESE + OVERDUB** | SendToFX | 0% | OBESE→OVERDUB | User-activated: massive osc through tape delay |
| **OVERBITE + ONSET** | AmpToFilter | 15% | OVERBITE→ONSET | Bass hits pump drum filter — locked groove |
| **OVERBITE + OVERDUB** | SendToFX | 0% | OVERBITE→OVERDUB | User-activated: bass through dub FX |
| **ODDOSCAR + OVERDUB** | EnvToMorph | 15% | ODDOSCAR→OVERDUB | Pad dynamics drive delay send level |
| **ODDOSCAR + ODYSSEY** | LFOToPitch | 10% | Bidirectional | Both pads drift against each other |
| **ODYSSEY + OBESE** | EnvToMorph | 15% | ODYSSEY→OBESE | Journey drives OBESE morph position |
| **ODYSSEY + OVERDUB** | SendToFX | 0% | ODYSSEY→OVERDUB | User-activated: psychedelic pads through tape delay |
| **ODDFELIX + OVERDUB** | SendToFX | 0% | ODDFELIX→OVERDUB | User-activated: percussive hits through dub FX |
| **Any + OVERDUB** | SendToFX | 0% | Any→OVERDUB | All OVERDUB sends start at 0 — user activates the "throw" |

### 8.2 Default Route Design Principles

1. **Coupling that adds rhythm is normalled on.** AmpToFilter between percussive and melodic engines (ODDFELIX→ODDOSCAR, ONSET→melodic) creates rhythmic pumping that sounds musical immediately.

2. **OVERDUB sends are normalled off.** The OVERDUB engine's send/return FX are a conscious performance decision — the "throw" gesture. Normalling them on would create muddy output. Users activate sends deliberately.

3. **Destructive routes are normalled off.** AudioToRing, AudioToFM, and FilterToFilter (chain mode) produce aggressive, often dissonant results. They are registered as available routes but start at 0%.

4. **Drift coupling is normalled on at low amounts.** LFOToPitch between melodic engines at 10-15% adds organic movement without obvious pitch instability. This is the XO_OX sonic signature.

5. **Default amounts are conservative.** 10-30% coupling produces subtle-to-moderate musical effects. Users dial up for dramatic results.

---

## 9. Patch Cable UI (Advanced Mode)

### 9.1 Visual Structure

Each engine slot is rendered as a panel with labeled input/output jacks. Cable connections are drawn as Bezier curves between jacks. The panel is hidden in Intuitive Mode and revealed in Advanced Mode.

```
┌─ ODDFELIX ──────────────────┐     ┌─ ODDOSCAR ─────────────────┐
│ ┌──────────────────────────┐│     │┌──────────────────────────┐│
│ │       Terracotta         ││     ││         Teal             ││
│ │       Module Panel       ││     ││       Module Panel       ││
│ └──────────────────────────┘│     │└──────────────────────────┘│
│                             │     │                            │
│  OUTPUTS           INPUTS  │     │  OUTPUTS           INPUTS  │
│  ○ Amp Out    ○ Pitch In   │     │  ○ Amp Out    ○ Pitch In   │
│  ○ Filter Out ○ Filter In  │     │  ○ Filter Out ○ Filter In  │
│  ○ LFO Out    ○ FM In      │     │  ○ LFO Out    ○ FM In      │
│  ○ Env Out    ○ Morph In   │     │  ○ Env Out    ○ Morph In   │
│  ○ Trigger    ○ Ring In    │     │  ○ Trigger    ○ Ring In    │
│  ○ Audio Out  ○ Send In    │     │  ○ Audio Out  ○ Send In    │
│                             │     │                            │
└─────────────────────────────┘     └────────────────────────────┘
              │                                  │
              │          ┌──[0.30]──┐            │
              ○ Amp Out ═══════════════ Filter In ○
              │          └──────────┘            │
              │                                  │
              │                 ┌──[0.15]──┐     │
              ○ Pitch In ═══════════════════ LFO Out ○
              │                 └──────────┘     │
              │                                  │

Legend:
  ═══  Active connection (solid line, engine color)
  ---  Normalled connection (dotted line, grey)
  [0.30]  Attenuator knob showing amount
```

### 9.2 Interaction Model

| Action | Result |
|--------|--------|
| **Drag from output jack to input jack** | Creates a new coupling route. The route type is determined by the jack types. |
| **Click on cable** | Reveals the attenuator knob. Drag to adjust amount (0-1). |
| **Double-click on cable** | Removes the user override. Route reverts to normalled default (or disappears if no default). |
| **Right-click on cable** | Context menu: Remove, Set Amount, Toggle Bidirectional, Reset to Default. |
| **Hover over jack** | Tooltip showing jack name and current connections. Compatible target jacks highlight. |
| **"Clear All" button** | Removes all user overrides. All normalled defaults restored. |
| **"Reset to Defaults" button** | Same as Clear All but also resets coupling master amounts to 1.0. |

### 9.3 Cable Visual Properties

| Property | Detail |
|----------|--------|
| **Shape** | Quadratic Bezier curve. Control point positioned below the midpoint for natural droop. |
| **Color** | Source engine's `cableColour` from EngineColorProfile. |
| **Opacity** | Proportional to route amount. Amount=0 is nearly transparent, amount=1 is fully opaque. |
| **Thickness** | 2px for normalled defaults, 3px for user connections. |
| **Normalled indicator** | Dotted line style. Becomes solid when overridden by user. |
| **Animation (optional)** | Subtle sine-wave oscillation on cable droop point (1Hz, 2px amplitude). Gives "hanging cable" feel. Disabled if CPU > 50%. |

### 9.4 Color Coding by Coupling Type

| Coupling Type | Cable Accent | Visual Indicator |
|--------------|-------------|-----------------|
| AmpToFilter | Yellow pulse markers | Small sine wave icon on cable |
| LFOToPitch | Cyan | Wavy line pattern |
| EnvToMorph | Green gradient | Arrow markers showing direction |
| AudioToFM | Red (warning: destructive) | Lightning bolt icon |
| AudioToRing | Orange (warning: destructive) | Ring/circle icon |
| FilterToFilter | White | Parallel line markers |
| SendToFX | Amber | Speaker icon |

---

## 10. Intuitive vs Advanced Mode

### 10.1 Mode Comparison

| Aspect | Intuitive Mode | Advanced Mode |
|--------|---------------|---------------|
| **Engine slots visible** | 1-2 (engine selector dropdown) | All 4 (full slot panel) |
| **Coupling control** | Single BLEND knob | Full coupling matrix panel |
| **Patch cables** | Hidden | Visible (toggle overlay) |
| **Per-route attenuators** | Hidden | Visible per cable |
| **Parameter access** | 4-8 macro knobs per engine | Full parameter pages per engine |
| **PlaySurface** | Simplified (pad mode only) | Full 3-mode surface |
| **Routing mode selector** | Hidden (Shared FX default) | Visible (Independent/Shared/Chain) |
| **Preset interaction** | Primary (browse, play, switch) | Secondary (browse + deep edit) |
| **Step sequencer** | Hidden | Full access |
| **CPU meter** | Hidden (auto-management) | Visible real-time display |
| **Export config** | One-click XPN export | Full bundle options |

### 10.2 Intuitive Mode — What Users See

```
┌───────────────────────────────────────────────────────┐
│  ┌─────────────────┐  ┌─────────────────────────────┐│
│  │  Engine Selector │  │        Preset Browser       ││
│  │  [ODDFELIX ▼] + [ODDOSCAR ▼]  │  "Dub Pressure" ││
│  └─────────────────┘  └─────────────────────────────┘│
│                                                       │
│  ┌──── Macros ───────────────────────────────────────┐│
│  │  [ODDFELIX]  [BLOOM]  [BLEND]  [FX MIX]         ││
│  │   ◉        ◉        ◉        ◉                  ││
│  └───────────────────────────────────────────────────┘│
│                                                       │
│  ┌──── PlaySurface (4x4 Pad) ───────────────────────┐│
│  │  ┌──┐ ┌──┐ ┌──┐ ┌──┐                            ││
│  │  │  │ │  │ │  │ │  │   X = Engine Blend          ││
│  │  ├──┤ ├──┤ ├──┤ ├──┤   Y = Expression            ││
│  │  │  │ │  │ │  │ │  │                             ││
│  │  ├──┤ ├──┤ ├──┤ ├──┤                             ││
│  │  │  │ │  │ │  │ │  │                             ││
│  │  ├──┤ ├──┤ ├──┤ ├──┤                             ││
│  │  │  │ │  │ │  │ │  │                             ││
│  │  └──┘ └──┘ └──┘ └──┘                            ││
│  └───────────────────────────────────────────────────┘│
│                                                       │
│                        [ADVANCED MODE →]              │
└───────────────────────────────────────────────────────┘
```

**The BLEND knob** is the single coupling control exposed in Intuitive Mode. It scales the `masterAmount` on all coupling pairs between the two active engines. At 0%, engines are independent. At 100%, all normalled coupling routes are at full depth.

### 10.3 Advanced Mode — What Users See

```
┌─────────────────────────────────────────────────────────────────┐
│  ┌────┐ ┌────┐ ┌────┐ ┌────┐                                  │
│  │ODFX│ │ODOC│ │OVDB│ │ +  │   Engine Slots (click to edit)   │
│  └────┘ └────┘ └────┘ └────┘                                  │
│                                                                 │
│  ┌──── Coupling Matrix ────────────────────────────────────────┐│
│  │           ODDFELIX ODDOSCAR OVERDUB                         ││
│  │  ODDFELIX   --     [AB]    [AC]                             ││
│  │  ODDOSCAR  [BA]     --     [BC]                             ││
│  │  OVERDUB   [CA]    [CB]    --                               ││
│  │                                                             ││
│  │  [AB] AmpToFilter: ◉ 0.30                                  ││
│  │  [AB] LFOToPitch:  ◉ 0.15                                  ││
│  │  [AC] SendToFX:    ◉ 0.00  [Enable]                        ││
│  └─────────────────────────────────────────────────────────────┘│
│                                                                 │
│  ┌── Patch Cable Overlay ──┐  ┌── Engine Parameters ──────────┐│
│  │  [Toggle Cables]        │  │  ODDFELIX: Attack, Decay...   ││
│  │  [Clear All]            │  │  ODDOSCAR: Position, Bloom... ││
│  │  [Reset Defaults]       │  │  OVERDUB: Send, Delay, Fb... ││
│  └─────────────────────────┘  └────────────────────────────────┘│
│                                                                 │
│  ┌── Routing ─┐  ┌── PlaySurface ──────────────────────────────┐│
│  │ ◉ Shared   │  │  [Pad] [Fretless] [Drum]                   ││
│  │ ○ Independ │  │  Full 3-mode surface with all zones         ││
│  │ ○ Chain    │  │                                             ││
│  └────────────┘  └─────────────────────────────────────────────┘│
│                                                                 │
│  CPU: [████████░░░░░░░░░░] 36%        [← INTUITIVE MODE]       │
└─────────────────────────────────────────────────────────────────┘
```

### 10.4 Mode Switching

Mode switching is instantaneous — no audio interruption, no preset change. All it does is show/hide UI panels. The underlying engine state, coupling matrix, and routing mode are identical in both modes.

The user's mode preference is stored per-preset. A preset designed in Advanced Mode can specify `"uiMode": "advanced"` so it opens in Advanced Mode. Intuitive Mode presets open in Intuitive Mode.

---

## 11. MIDI Routing

### 11.1 Routing Modes

| Mode | Behavior | Use Case |
|------|----------|----------|
| **All** | All engines receive all MIDI events | Default. Play the same notes on all engines for layered sound. |
| **Split Keyboard** | Each engine assigned a key zone (e.g., OVERBITE below C3, ODDFELIX above C3) | Bass + lead setup. Live performance split. |
| **Channel-Based** | Each engine listens on a specific MIDI channel (1-4) | External sequencer driving different engines independently. |

### 11.2 MIDI Split Implementation

```cpp
void MegaToolProcessor::splitMidi(const juce::MidiBuffer& source)
{
    // Clear per-engine buffers
    for (auto& buf : perEngineMidi)
        buf.clear();

    switch (midiRouting)
    {
        case MidiRoutingMode::All:
        {
            // Every engine gets all events
            for (int s = 0; s < kMaxSlots; ++s)
            {
                if (engines[s])
                    perEngineMidi[s] = source;
            }
            break;
        }

        case MidiRoutingMode::SplitKeyboard:
        {
            for (const auto metadata : source)
            {
                auto msg = metadata.getMessage();
                if (msg.isNoteOnOrOff())
                {
                    int note = msg.getNoteNumber();
                    int targetSlot = getSplitSlotForNote(note);
                    if (targetSlot >= 0 && targetSlot < kMaxSlots)
                        perEngineMidi[targetSlot].addEvent(
                            msg, metadata.samplePosition);
                }
                else
                {
                    // Non-note events (CC, pitch bend, etc.) go to all engines
                    for (int s = 0; s < kMaxSlots; ++s)
                    {
                        if (engines[s])
                            perEngineMidi[s].addEvent(
                                msg, metadata.samplePosition);
                    }
                }
            }
            break;
        }

        case MidiRoutingMode::ChannelBased:
        {
            for (const auto metadata : source)
            {
                auto msg = metadata.getMessage();
                int ch = msg.getChannel(); // 1-based
                int targetSlot = ch - 1;   // 0-based
                if (targetSlot >= 0 && targetSlot < kMaxSlots && engines[targetSlot])
                    perEngineMidi[targetSlot].addEvent(
                        msg, metadata.samplePosition);
            }
            break;
        }
    }
}
```

### 11.3 MIDI Learn for Coupling Parameters

All coupling parameters (route amounts, master amounts) are MIDI-learnable. Implementation follows the standard JUCE MIDI learn pattern:

1. User right-clicks a coupling knob and selects "MIDI Learn."
2. The next incoming CC message is mapped to that parameter.
3. The mapping is stored in the preset and recalled on load.

### 11.4 Performance Strip Integration

The Performance Strip (see `xo_performance_strip_spec.md`) maps physical controller inputs to coupling parameters:

| Strip Zone | Default Mapping | Adjustable |
|-----------|----------------|-----------|
| Ribbon (horizontal) | BLEND (coupling master amount between Slot A and Slot B) | Yes |
| Pressure | Engine A volume (expression) | Yes |
| Mod Wheel | Coupling route 1 amount (first active coupling) | Yes |
| Aftertouch | Coupling route 2 amount (second active coupling) | Yes |

---

## 12. Preset Integration

### 12.1 Multi-Engine Preset Format

Multi-engine presets use the `.xomega` JSON format defined in the dev strategy document. The chaining architecture adds coupling state and routing mode to the format:

```json
{
    "schema_version": 1,
    "mega_tool_version": "1.0.0",
    "preset_name": "Dub Pressure",
    "author": "XO_OX",
    "category": "Entangled",
    "tags": ["dub", "coupled", "dark"],
    "ui_mode": "intuitive",
    "engines": [
        {
            "engine_id": "OddfeliX",
            "slot": 0,
            "active": true,
            "max_voices": 8,
            "parameters": {
                "snap_attack": 0.01,
                "snap_decay": 0.3,
                "snap_filterCutoff": 800.0,
                "snap_filterRes": 0.4
            }
        },
        {
            "engine_id": "OddOscar",
            "slot": 1,
            "active": true,
            "max_voices": 16,
            "parameters": {
                "morph_position": 0.5,
                "morph_bloom": 0.7,
                "morph_filterCutoff": 2000.0
            }
        }
    ],
    "coupling": {
        "pairs": [
            {
                "slot_a": 0,
                "slot_b": 1,
                "master_amount": 1.0,
                "routes": [
                    {
                        "type": "AmpToFilter",
                        "amount": 0.35,
                        "enabled": true,
                        "normalled": false,
                        "bidirectional": false
                    },
                    {
                        "type": "LFOToPitch",
                        "amount": 0.15,
                        "enabled": true,
                        "normalled": true,
                        "bidirectional": false
                    }
                ]
            }
        ]
    },
    "routing": {
        "mode": "shared",
        "midi_routing": "all"
    },
    "shared_fx": {
        "fx_delay_time": 0.375,
        "fx_delay_feedback": 0.5,
        "fx_delay_mix": 0.25,
        "fx_reverb_size": 0.6,
        "fx_reverb_mix": 0.35
    },
    "master": {
        "master_volume": 0.0,
        "master_width": 1.0,
        "master_limiter": true
    },
    "playsurface": {
        "mode": "pad",
        "scale": "minor_pentatonic",
        "root_note": 48,
        "blend_axis": "x"
    }
}
```

### 12.2 Preset Switching

Switching presets uses a 200ms equal-power crossfade to prevent clicks:

1. **Snapshot current output** into a fade-out buffer.
2. **Load new preset state:** engine params, coupling matrix, routing mode, PlaySurface config.
3. **Render new preset** into a fade-in buffer.
4. **Crossfade** for 200ms (8820 samples at 44.1kHz): old fades out, new fades in.
5. **Release old state** after crossfade completes.

If the new preset uses different engines, the engine swap crossfade (Section 3.3, 50ms) executes first, then the preset crossfade applies to the parameter transition.

### 12.3 Single-Engine Preset Import

Presets from standalone XO_OX instruments (`.xocmeta`, `.xopmeta`, etc.) are imported by wrapping them in a single-engine `.xomega` envelope:

```json
{
    "schema_version": 1,
    "preset_name": "Imported: Terracotta Hit",
    "engines": [
        {
            "engine_id": "OddfeliX",
            "slot": 0,
            "active": true,
            "parameters": { /* mapped from .xocmeta with prefix */ }
        }
    ],
    "coupling": { "pairs": [] },
    "routing": { "mode": "shared" }
}
```

The adapter class (see dev strategy Section 4.2) handles parameter ID mapping between the standalone format (unprefixed) and the mega-tool format (prefixed).

---

## 13. CPU Management

### 13.1 Voice Reduction Table

When multiple engines are active, the hub automatically reduces per-engine voice counts to stay within the CPU budget:

| Active Engines | Engine Default Voices | Reduced Voices | Reduction |
|---------------|----------------------|----------------|-----------|
| 1 | Full (per engine) | Full | 0% |
| 2 | Full | Full | 0% |
| 3 | OBESE: 5, OVERBITE: 16, ODDFELIX: 8, ODDOSCAR: 16, OVERDUB: 8, ODYSSEY: 24, ONSET: 8 | OBESE: 3, OVERBITE: 8, ODDFELIX: 6, ODDOSCAR: 8, OVERDUB: 6, ODYSSEY: 12, ONSET: 8 | ~40-50% |
| 4 | (as above) | OBESE: 2, OVERBITE: 4, ODDFELIX: 4, ODDOSCAR: 4, OVERDUB: 4, ODYSSEY: 8, ONSET: 8 | ~50-67% |

ONSET always retains 8 voices because drum engines need dedicated per-voice playback. Other engines reduce proportionally.

### 13.2 Quality Modes

| Mode | Filter Coefficient Updates | Coupling Granularity | Target CPU |
|------|--------------------------|---------------------|-----------|
| **Standard** | Every sample | Per-sample (2 engines) or per-block (3-4) | < 55% |
| **Eco** | Every 4 samples | Per-block always | < 40% |

Quality mode is selectable by the user in Advanced Mode. In Intuitive Mode, the system automatically drops to Eco when CPU exceeds 50%.

### 13.3 Engine Deactivation

Deactivated slots consume exactly 0% CPU. The engine pointer is null, and the render loop skips it entirely. No silent rendering, no parameter snapshotting, no coupling processing.

```cpp
// In the render loop:
for (int s = 0; s < kMaxSlots; ++s)
{
    if (!engines[s]) continue; // 0 CPU for empty slots
    // ... render only active engines
}
```

### 13.4 Real-Time CPU Meter

The Advanced Mode UI displays a real-time CPU meter showing:

```
┌── CPU Monitor ──────────────────────────────┐
│  ODDFELIX: [████░░░░░░░░]   8%              │
│  ODDOSCAR: [██████░░░░░░]  12%              │
│  Coupling: [██░░░░░░░░░░]   2%              │
│  FX:       [████░░░░░░░░]   6%              │
│  ─────────────────────────────              │
│  Total:    [████████░░░░]  28%   OK         │
│                                              │
│  Voice Count: ODDFELIX 5/8  ODDOSCAR 12/16  │
└──────────────────────────────────────────────┘
```

The meter updates at 10Hz (every ~100ms) using `juce::AudioProcessorListener` callback data.

### 13.5 Automatic Voice Stealing Across Engines

When total CPU approaches the budget ceiling (55%), the hub implements cross-engine voice stealing:

1. **Priority ordering:** Engines are ranked by musical importance. Engines playing the most recent note-on events have highest priority.
2. **Steal from lowest priority:** The engine with the oldest, quietest voice loses a voice first.
3. **Never steal below 1 voice:** Every active engine retains at least 1 sounding voice.
4. **Recovery:** When CPU drops below 45%, stolen voice slots are restored.

---

## 14. Thread Safety

### 14.1 Thread Responsibilities

| Thread | Operations | Lock Requirements |
|--------|-----------|-------------------|
| **Audio thread** | `renderBlock`, coupling matrix `processSample`/`processBlock`, parameter snapshot reads, engine `renderBlock` calls | No locks. All reads are atomic or from pre-snapshotted state. |
| **Message thread** | Engine loading/swapping, UI updates, preset loading, APVTS parameter changes, coupling configuration | SpinLock for crossfade state. No contention with audio thread during normal operation. |
| **Background thread** | XPN export rendering, preset file I/O | No shared state with audio thread. Operates on copies. |

### 14.2 Lock-Free Parameter Snapshots

Every engine uses the `ParamSnapshot` pattern established across all XO_OX instruments:

```cpp
/// Snapshotted once per processBlock on the message thread boundary.
/// Cached in a plain struct for zero-cost per-sample access.
struct FatParamSnapshot
{
    float mojo;
    float morphPosition;
    float filterCutoff;
    float filterRes;
    float attack, decay, sustain, release;
    // ... all 45 OBESE parameters
};

/// In the engine's snapshotParameters() method:
void snapshotParameters(const juce::AudioProcessorValueTreeState& apvts) override
{
    snap.mojo = apvts.getRawParameterValue("fat_mojo")->load();
    snap.morphPosition = apvts.getRawParameterValue("fat_morphPosition")->load();
    // ... etc.
}
```

The `getRawParameterValue()` returns a `std::atomic<float>*`. The `.load()` is a single atomic read — no lock, no contention.

### 14.3 Crossfade State Management

The crossfade during engine swap requires coordination between message and audio threads:

```cpp
/// Thread-safe crossfade initiation.
/// Called on the message thread. The audio thread reads the state atomically.
void MegaToolProcessor::swapEngine(int slot, const juce::String& moduleID)
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    // 1. Create and prepare the new engine (message thread — allocation OK)
    auto newEngine = EngineRegistry::getInstance().createEngine(moduleID);
    if (!newEngine) return;
    newEngine->prepare(currentSampleRate, currentBlockSize);

    // 2. Atomic swap setup
    // Move old engine into crossfade struct
    crossfades[slot].outgoing = std::move(engines[slot]);
    crossfades[slot].samplesRemaining = SlotCrossfade::kCrossfadeSamples;

    // 3. Install new engine (pointer swap is atomic on all targets)
    engines[slot] = std::move(newEngine);

    // 4. Reconfigure coupling matrix for new engine pairing
    std::array<SynthEngine*, kMaxSlots> rawPtrs = {};
    for (int s = 0; s < kMaxSlots; ++s)
        rawPtrs[s] = engines[s].get();
    couplingMatrix.configure(rawPtrs);
}
```

### 14.4 Pre-Allocated Coupling Buffers

All coupling buffers are allocated during `prepareToPlay()` and never resized on the audio thread:

```cpp
void MegaToolProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    currentBlockSize = samplesPerBlock;

    // Pre-allocate per-engine audio buffers
    for (int s = 0; s < kMaxSlots; ++s)
        engineBuffers[s].setSize(2, samplesPerBlock);

    masterBuffer.setSize(2, samplesPerBlock);

    // Prepare all active engines
    for (int s = 0; s < kMaxSlots; ++s)
    {
        if (engines[s])
            engines[s]->prepare(sampleRate, samplesPerBlock);
    }

    // Prepare shared FX rack
    fxRack.prepare(sampleRate, samplesPerBlock);

    // Configure coupling matrix
    std::array<SynthEngine*, kMaxSlots> rawPtrs = {};
    for (int s = 0; s < kMaxSlots; ++s)
        rawPtrs[s] = engines[s].get();
    couplingMatrix.configure(rawPtrs);

    // Update voice budgets based on active engine count
    updateVoiceBudgets();
}
```

### 14.5 Audio Thread Safety Checklist

The following rules are enforced across the entire mega-tool codebase:

| Rule | Enforcement |
|------|------------|
| No `new` / `delete` / `malloc` / `free` on audio thread | Code review. All allocations in `prepare()` or message thread. |
| No `std::vector::push_back()` on audio thread | Pre-sized vectors only. Use `std::array` where possible. |
| No `juce::String` construction on audio thread | Strings are pre-built in the message thread or compile-time constants. |
| No mutex locks on audio thread | All shared state is atomic or snapshotted. SpinLock only for crossfade flag (< 1us contention). |
| No file I/O on audio thread | Preset loading and XPN export run on message/background threads. |
| No APVTS listener callbacks on audio thread | Parameter changes flow through atomic `getRawParameterValue()` reads. |
| Denormal protection in all feedback paths | `juce::ScopedNoDenormals` at top of `processBlock`. Per-engine `flush-to-zero` on filter/delay states. |
| NaN guarding on coupling matrix output | `juce::jlimit(-1.0f, 1.0f, ...)` after every `computeModulation()` call. |

---

*CONFIDENTIAL -- XO_OX Internal Design Document*
