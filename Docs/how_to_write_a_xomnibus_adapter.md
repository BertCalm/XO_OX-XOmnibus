# How to Write a XOceanus Engine Adapter

## Overview

A XOceanus engine adapter is a thin C++ class that wraps a standalone XO_OX synthesizer behind the `SynthEngine` interface. It exists because standalone instruments have their own parameter namespaces, lifecycle management, and audio routing — the adapter bridges all of that into XOceanus's unified system. XOceanus holds up to four active engines in slots and connects them through the `MegaCouplingMatrix`. Every engine in those slots must speak the same interface; the adapter is the translator.

In practice, an adapter does three things: it re-exposes the standalone engine's parameters under a namespaced prefix (e.g., `poss_filterCutoff` instead of `filter_cutoff`), it routes `renderBlock()` calls to the underlying DSP, and it wires up the two coupling hooks (`getSampleForCoupling` and `applyCouplingInput`) so other engines can modulate it.

The canonical example of a complete adapter is `BiteEngine` (wrapping XOppossum), located at:
`Source/Engines/Bite/BiteEngine.h`

---

## The SynthEngine Interface

Every adapter inherits from `xoceanus::SynthEngine` (defined in `Source/Core/SynthEngine.h`). The full interface:

```cpp
class SynthEngine {
public:
    virtual ~SynthEngine() = default;

    // Lifecycle
    virtual void prepare(double sampleRate, int maxBlockSize) = 0;
    virtual void releaseResources() = 0;
    virtual void reset() = 0;

    // Audio (must be real-time safe: no allocation, no blocking, no exceptions)
    virtual void renderBlock(juce::AudioBuffer<float>& buffer,
                             juce::MidiBuffer& midi,
                             int numSamples) = 0;

    // Coupling — called by MegaCouplingMatrix
    virtual float getSampleForCoupling(int channel, int sampleIndex) const = 0;
    virtual void applyCouplingInput(CouplingType type,
                                    float amount,
                                    const float* sourceBuffer,
                                    int numSamples) = 0;

    // Parameters
    virtual juce::AudioProcessorValueTreeState::ParameterLayout
        createParameterLayout() = 0;
    virtual void attachParameters(juce::AudioProcessorValueTreeState& apvts) = 0;

    // Identity
    virtual juce::String getEngineId() const = 0;
    virtual juce::Colour getAccentColour() const = 0;
    virtual int getMaxVoices() const = 0;

    // Optional (defaults to 0)
    virtual int getActiveVoiceCount() const { return 0; }
};
```

**Design contract enforced by XOceanus:**
- `renderBlock()` must never allocate memory or perform blocking I/O.
- `getSampleForCoupling()` must be O(1) — return from a pre-written cache, not a computation.
- `applyCouplingInput()` accumulates modulation into member variables; `renderBlock()` consumes them.
- `createParameterLayout()` returns engine-namespaced parameter IDs, which are merged into the shared APVTS.
- `attachParameters()` is called once after the APVTS is fully constructed; cache all pointers here.

---

## Step-by-Step

### Step 1: Create the Adapter Header

Create `Source/Engines/{CanonicalName}/{CanonicalName}Engine.h`. The canonical name is the XOceanus gallery name (e.g., `Bite` for XOppossum, `Overworld` for XOverworld).

Minimal skeleton:

```cpp
#pragma once
#include "../../Core/SynthEngine.h"
// Include any shared DSP headers your DSP uses:
#include "../../DSP/CytomicSVF.h"
#include "../../DSP/FastMath.h"

// If wrapping a separate standalone engine, include its DSP headers here.
// OverworldEngine does this pattern:
//   #include "engine/VoicePool.h"
//   #include "dsp/SVFilter.h"

namespace xoceanus {  // or a sub-namespace if needed

class MyNewEngine : public SynthEngine {
public:
    static constexpr int kMaxVoices = 8;

    //-- SynthEngine interface -------------------------------------------------

    void prepare(double sampleRate, int maxBlockSize) override;
    void releaseResources() override;
    void reset() override;
    void renderBlock(juce::AudioBuffer<float>& buffer,
                     juce::MidiBuffer& midi,
                     int numSamples) override;
    float getSampleForCoupling(int channel, int sampleIndex) const override;
    void applyCouplingInput(CouplingType type, float amount,
                            const float* sourceBuffer, int numSamples) override;
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout() override;
    void attachParameters(juce::AudioProcessorValueTreeState& apvts) override;
    juce::String getEngineId() const override { return "MyEngine"; }
    juce::Colour getAccentColour() const override { return juce::Colour(0xFF00A6D6); }
    int getMaxVoices() const override { return kMaxVoices; }
    int getActiveVoiceCount() const override;

    // Called by XOceanusProcessor::createParameterLayout()
    static void addParameters(
        std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params);

private:
    // DSP objects
    double sr = 44100.0;

    // Coupling state — accumulate here, consume in renderBlock()
    float externalFilterMod = 0.0f;
    const float* externalFMBuffer = nullptr;
    int externalFMSamples = 0;
    float externalFMAmount = 0.0f;

    // Output cache for getSampleForCoupling() — pre-allocated in prepare()
    std::vector<float> outputCacheL;
    std::vector<float> outputCacheR;

    // Cached APVTS pointers — all nullptr until attachParameters() is called
    std::atomic<float>* pFilterCutoff = nullptr;
    std::atomic<float>* pAmpAttack    = nullptr;
    // ... one per parameter

    // Safe load helpers
    static int safeLoad(std::atomic<float>* p, int fallback) noexcept {
        return (p != nullptr) ? static_cast<int>(p->load()) : fallback;
    }
    static float safeLoadF(std::atomic<float>* p, float fallback) noexcept {
        return (p != nullptr) ? p->load() : fallback;
    }
};

} // namespace xoceanus
```

Also create a one-line `{CanonicalName}Engine.cpp`:

```cpp
#include "MyNewEngine.h"
// All DSP is inline in the .h. This stub provides the translation unit for CMake.
// Registration is centralized in XOceanusProcessor.cpp.
```

**Important:** All DSP in XOceanus lives inline in `.h` headers. The `.cpp` is always a one-line stub. This is a project-wide architectural rule documented in `CLAUDE.md`.

---

### Step 2: Implement createParameterLayout()

This method must return every parameter the engine exposes, with namespaced IDs. The APVTS is built once for the whole XOceanus processor with all engine parameters merged together — parameter ID collisions across engines will cause a JUCE assertion at startup.

The required pattern is a static `addParametersImpl()` helper called from both `createParameterLayout()` and `addParameters()`:

```cpp
static void addParametersImpl(
    std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params)
{
    // Every ParameterID must carry the engine's prefix
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"myeng_filterCutoff", 1}, "My Engine Filter Cutoff",
        juce::NormalisableRange<float>(20.0f, 20000.0f, 0.1f, 0.3f), 2000.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"myeng_ampAttack", 1}, "My Engine Amp Attack",
        juce::NormalisableRange<float>(0.001f, 10.0f, 0.001f, 0.4f), 0.01f));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{"myeng_filterMode", 1}, "My Engine Filter Mode",
        juce::StringArray{"Low Pass", "Band Pass", "High Pass"}, 0));
}

static void addParameters(
    std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params)
{
    addParametersImpl(params);
}

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout() override
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
    addParametersImpl(params);
    return {params.begin(), params.end()};
}
```

**Key rules:**
- Parameter IDs are permanent. They appear in saved presets. Never rename them after any preset ships with that ID.
- The `ParameterID` second argument (version number) is always `1` for new engines.
- Use `juce::NormalisableRange` skew factors (fourth argument) for perceptually uniform controls. Frequency parameters typically use `0.3`; time parameters use `0.4`.

The parameter prefix for each engine is documented in `CLAUDE.md` under "Engine ID vs Parameter Prefix". For new engines, choose a short, unique prefix and register it there. Examples: `poss_` (Overbite/XOppossum), `ow_` (Overworld/XOverworld), `snap_` (OddfeliX).

---

### Step 3: Implement attachParameters()

After the shared APVTS is constructed, XOceanus calls `attachParameters()` once. Cache every raw parameter pointer here. Use these cached pointers in `renderBlock()` — never call `apvts.getRawParameterValue()` on the audio thread.

```cpp
void attachParameters(juce::AudioProcessorValueTreeState& apvts) override
{
    pFilterCutoff = apvts.getRawParameterValue("myeng_filterCutoff");
    pAmpAttack    = apvts.getRawParameterValue("myeng_ampAttack");
    pFilterMode   = apvts.getRawParameterValue("myeng_filterMode");
    // ... one line per parameter
}
```

This is the ParamSnapshot pattern: cache once, read atomically many times. The `safeLoad` / `safeLoadF` helpers (shown in Step 1) guard against the brief window between construction and `attachParameters()` where pointers are still null.

**Overworld variant:** If your standalone engine uses a `Parameters.h` with its own namespace (as XOverworld does), `attachParameters()` uses string constants from that namespace:

```cpp
void attachParameters(juce::AudioProcessorValueTreeState& apvts) override
{
    namespace PID = xoverworld::ParamID;
    p_era    = apvts.getRawParameterValue(PID::ERA);
    p_eraY   = apvts.getRawParameterValue(PID::ERA_Y);
    // ...
}
```

This works as long as the string values of those constants match the IDs registered in `createParameterLayout()`.

---

### Step 4: Implement prepare(), releaseResources(), and reset()

`prepare()` is where you size the output cache and initialize all DSP:

```cpp
void prepare(double sampleRate, int maxBlockSize) override
{
    sr = sampleRate;

    // Pre-allocate output cache — size to maxBlockSize
    outputCacheL.assign(static_cast<size_t>(maxBlockSize), 0.0f);
    outputCacheR.assign(static_cast<size_t>(maxBlockSize), 0.0f);

    // Initialize DSP objects
    for (int i = 0; i < kMaxVoices; ++i) {
        voices[i].prepare(sampleRate);
    }
    filter.prepare(sampleRate);
}

void releaseResources() override
{
    // Release non-essential resources. The output cache can stay.
    // If you have a voice pool with allNotesOff(), call it here.
}

void reset() override
{
    // Reset all state without reallocating. Called on preset change and
    // transport reset. Clear voice state, filter state, envelopes, etc.
    for (auto& v : voices) v.reset();
    filter.reset();
    externalFilterMod = 0.0f;
    externalFMBuffer = nullptr;
    externalFMSamples = 0;
    std::fill(outputCacheL.begin(), outputCacheL.end(), 0.0f);
    std::fill(outputCacheR.begin(), outputCacheR.end(), 0.0f);
}
```

---

### Step 5: Implement renderBlock()

This is the audio thread entry point. Follow this structure exactly — the order matters:

```cpp
void renderBlock(juce::AudioBuffer<float>& buffer,
                 juce::MidiBuffer& midi,
                 int numSamples) override
{
    if (numSamples <= 0) return;

    juce::ScopedNoDenormals noDenormals;

    // 1. ParamSnapshot: read ALL parameters once, at the top of the block.
    //    Never call p->load() inside the per-sample loop.
    const float filterCutoff = safeLoadF(pFilterCutoff, 2000.0f);
    const float ampAttack    = safeLoadF(pAmpAttack, 0.01f);
    const int   filterMode   = safeLoad(pFilterMode, 0);

    // 2. Apply coupling accumulators to compute effective parameter values.
    //    externalFilterMod was written by applyCouplingInput() before this block.
    const float effectiveCutoff = filterCutoff + externalFilterMod;

    // 3. Process MIDI events (note on/off, sustain, etc.)
    for (const auto metadata : midi) {
        const auto msg = metadata.getMessage();
        if (msg.isNoteOn())  noteOn(msg.getNoteNumber(), msg.getVelocity());
        if (msg.isNoteOff()) noteOff(msg.getNoteNumber());
    }

    // 4. Render audio per sample.
    auto* L = buffer.getWritePointer(0);
    auto* R = buffer.getWritePointer(buffer.getNumChannels() > 1 ? 1 : 0);

    for (int s = 0; s < numSamples; ++s)
    {
        float sample = renderSample(effectiveCutoff, /* ... */);

        L[s] = sample;
        R[s] = sample;

        // 5. Write to output cache for coupling reads.
        if (s < static_cast<int>(outputCacheL.size())) {
            outputCacheL[static_cast<size_t>(s)] = L[s];
            outputCacheR[static_cast<size_t>(s)] = R[s];
        }
    }

    // 6. Reset coupling accumulators after consumption.
    externalFilterMod = 0.0f;
    externalFMBuffer  = nullptr;
    externalFMSamples = 0;
}
```

**Critical rules:**
- No `new`, `delete`, `malloc`, `free`, or `std::vector::push_back` inside `renderBlock()`.
- No file I/O, mutex locks, or calls that may block.
- `ScopedNoDenormals` at the top of every `renderBlock()` — denormals cause CPU spikes in filter feedback paths.
- The output cache write (`outputCacheL[s] = L[s]`) is required for `getSampleForCoupling()` to work correctly. Several engines in the fleet initially skipped this and were patched as P0 bugs.

---

### Step 6: Implement Coupling

**getSampleForCoupling()** — Return from the pre-written output cache. This is called O(1) per sample by the MegaCouplingMatrix:

```cpp
float getSampleForCoupling(int channel, int sampleIndex) const override
{
    auto si = static_cast<size_t>(sampleIndex);
    if (channel == 0 && si < outputCacheL.size()) return outputCacheL[si];
    if (channel == 1 && si < outputCacheR.size()) return outputCacheR[si];
    // Channel 2 convention: return envelope output (amplitude follower)
    // if your engine supports AmpToFilter coupling as a source
    return 0.0f;
}
```

**applyCouplingInput()** — Handle the coupling types your engine supports. Ignore the rest with a no-op `default` branch:

```cpp
void applyCouplingInput(CouplingType type, float amount,
                        const float* sourceBuffer, int numSamples) override
{
    switch (type)
    {
        case CouplingType::AmpToFilter:
            // Accumulate a filter cutoff offset in Hz
            externalFilterMod += amount * 8000.0f;
            break;

        case CouplingType::AudioToFM:
            // Store the source buffer pointer for per-sample FM modulation
            if (sourceBuffer != nullptr && numSamples > 0) {
                externalFMBuffer  = sourceBuffer;
                externalFMSamples = numSamples;
                externalFMAmount  = amount;
            }
            break;

        default:
            break;  // Silently ignore coupling types not supported
    }
}
```

**Decide which coupling types your engine supports.** The 12 available types are defined in `SynthEngine.h`:

| CouplingType | Meaning |
|---|---|
| `AmpToFilter` | Source amplitude → destination filter cutoff |
| `AmpToPitch` | Source amplitude → destination pitch |
| `LFOToPitch` | Source LFO → destination pitch |
| `EnvToMorph` | Source envelope → destination wavetable/morph position |
| `AudioToFM` | Source audio → destination FM input |
| `AudioToRing` | Source audio × destination audio |
| `FilterToFilter` | Source filter output → destination filter input |
| `AmpToChoke` | Source amplitude chokes destination |
| `RhythmToBlend` | Source rhythm pattern → destination blend parameter |
| `EnvToDecay` | Source envelope → destination decay time |
| `PitchToPitch` | Source pitch → destination pitch (harmony) |
| `AudioToWavetable` | Source audio → destination wavetable source |

For a minimal first integration, implement at least `AmpToFilter` (the most common coupling route). Document which types you accept in a comment at the top of your class declaration (see `OverworldEngine.h` for a clean example).

---

### Step 7: Register with XOceanus

Open `Source/XOceanusProcessor.cpp`. There are two places to edit:

**1. Add an `#include` at the top:**
```cpp
#include "Engines/MyEngine/MyNewEngine.h"
```

**2. Add a static registration block near the other registrations:**
```cpp
static bool registered_MyEngine =
    xoceanus::EngineRegistry::instance().registerEngine(
        "MyEngine",  // Must exactly match getEngineId()
        []() -> std::unique_ptr<xoceanus::SynthEngine> {
            return std::make_unique<xoceanus::MyNewEngine>();
        });
```

**3. Add a parameter merge call in `createParameterLayout()`:**
```cpp
MyNewEngine::addParameters(params);
```

This is placed alongside the other engine `addParameters()` calls at approximately line 184. The order here determines the order parameters appear in DAW automation lists, but does not affect audio behavior.

**4. Add the engine ID to `validEngineNames` in `PresetManager.h`** if it is not already present. Without this, presets referencing your engine will fail to load.

---

### Step 8: Verify Integration

**Build:**
```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

**AU validation:**
```bash
auval -v aumu Xomn Xoox  # Replace with your plugin codes
```

**Smoke tests to run manually:**
1. Load XOceanus. Open a preset that references your engine. Confirm it sounds correct.
2. Create a coupling route from your engine to another engine with `AmpToFilter`. Move a note and confirm the destination filter responds.
3. Create a coupling route from another engine to yours with a type you declared support for. Confirm your engine responds.
4. Load 20+ presets at random. Confirm no crashes and no parameter ID collisions in the JUCE console.

---

## Parameter Namespace Translation

### The Core Problem

Standalone XO_OX instruments often have parameter IDs that lack a prefix. XOppossum (the standalone synth behind OVERBITE) uses plain names like `filter_cutoff`, `osc_a_wave`, `amp_attack`. XOceanus cannot use these as-is because the shared APVTS holds parameters from all engines simultaneously — collisions are guaranteed if two engines both register `filter_cutoff`.

### The Solution: Prefix Everything in the Adapter

The adapter's `createParameterLayout()` registers parameters under the engine's canonical prefix (`poss_` for Overbite). The standalone instrument is not modified. All the DSP code inside the adapter uses the prefixed IDs when calling `apvts.getRawParameterValue()`:

```cpp
// In attachParameters():
pFilterCutoff = apvts.getRawParameterValue("poss_filterCutoff");
// NOT:
pFilterCutoff = apvts.getRawParameterValue("filter_cutoff");
```

### Presets Must Use the XOceanus IDs

Factory presets for the engine must use the adapter's parameter IDs (with prefix), not the standalone instrument's IDs. A preset's `parameters` block is keyed by engine canonical name:

```json
{
  "parameters": {
    "Overbite": {
      "poss_filterCutoff": 2000.0,
      "poss_ampAttack": 0.01
    }
  }
}
```

### If the Standalone Engine Has Its Own Parameters.h

Some standalone engines define parameter ID constants in a namespace (XOverworld uses `xoverworld::ParamID::ERA`, `xoverworld::ParamID::FILTER_CUTOFF`, etc.). When integrating these, you have two options:

**Option A:** Use those constants directly in `attachParameters()`. The values of those constants must match whatever strings you registered in `createParameterLayout()`.

**Option B:** Hardcode the XOceanus-prefixed strings everywhere. Simpler, no dependency on the standalone's header structure.

`OverworldEngine.h` uses Option A and delegates `createParameterLayout()` entirely:
```cpp
juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout() override {
    return xoverworld::createParameterLayout();  // standalone's own layout function
}
```
This only works if the standalone's parameter IDs are already the `ow_`-prefixed canonical XOceanus IDs — which XOverworld's were, because it was built XOceanus-ready from day one.

For older engines retrofitted from pre-XOceanus code, Option A requires a translation shim or Option B (rewrite all IDs in the adapter).

---

## Common Pitfalls

### Missing output cache fill in renderBlock()
The output cache (per-sample write in the render loop) is required for `getSampleForCoupling()` to function. Several engines in the fleet were shipped without this and were patched as P0 bugs. If coupling from your engine to another engine produces silence or constant-value modulation, this is the first thing to check.

**Where to look:** `OverworldEngine.h` commit history, comments marked "P0-05 fix".

### Duplicate parameter IDs
If two engines register the same parameter ID string, JUCE will assert at startup (debug builds) or silently use whichever registration appeared first (release builds). The symptom is one engine appearing to control another's parameters.

**Prevention:** Grep for your chosen prefix before committing: `grep -r "myeng_" Source/`.

### getEngineId() must match the registry key
The string returned by `getEngineId()` must exactly match the key passed to `EngineRegistry::instance().registerEngine()`. If they differ, preset loading will fail (presets reference the canonical engine name, which PresetManager resolves through `validEngineNames` and `resolveEngineAlias()`).

### applyCouplingInput() called before prepare()
XOceanus may call `applyCouplingInput()` before the engine is fully prepared if the engine is loaded mid-stream. Guard all pointer dereferences with null checks. The `externalFMBuffer` pattern (store pointer, check in renderBlock) handles this cleanly.

### Forgetting to reset coupling accumulators
`externalFilterMod` and similar accumulators must be reset to zero at the **end** of `renderBlock()`, after they are consumed. Failure to reset causes modulation to accumulate across blocks, producing an ever-growing offset. In `BiteEngine.h`, see the "Clear coupling buffer pointer after use" comment at line ~1362.

### Parameter version mismatch in ParameterID
The second argument of `juce::ParameterID{"myeng_param", 1}` is the parameter version. It must be `1` for all new parameters. If you accidentally use `0`, JUCE may behave differently across builds. Always use `1`.

### Omitting addParameters() in XOceanusProcessor.cpp
The `createParameterLayout()` on `XOceanusProcessor` merges parameters from all engines by calling each engine's static `addParameters()`. If you add an engine registration but forget the `addParameters()` call, none of your engine's parameters will exist in the APVTS and `attachParameters()` will silently fill every pointer with `nullptr`.

### Engine ID not in validEngineNames
`PresetManager::parseJSON()` validates engine names against `validEngineNames` (in `PresetManager.h`). If your engine's canonical ID is not in that list, presets referencing your engine will be rejected at load time with no audio and no obvious error.

---

## Example: Minimal Complete Adapter

This is a complete working skeleton. Replace `MyEngine`, `myeng_`, and the DSP placeholders with actual implementation.

**`Source/Engines/MyEngine/MyEngineAdapter.h`:**

```cpp
#pragma once
#include "../../Core/SynthEngine.h"
#include <array>
#include <atomic>
#include <vector>

namespace xoceanus {

//==============================================================================
// MyEngineAdapter — wraps XOmyengine standalone DSP behind the SynthEngine
// interface.
//
// Coupling inputs supported:
//   AmpToFilter  — external amp signal modulates filter cutoff (±8000 Hz)
//   EnvToMorph   — external envelope modulates morph position (±0.5)
//
class MyEngineAdapter : public SynthEngine {
public:
    static constexpr int kMaxVoices = 8;

    //==========================================================================
    // Lifecycle
    //==========================================================================

    void prepare(double sampleRate, int maxBlockSize) override
    {
        sr  = sampleRate;
        srf = static_cast<float>(sampleRate);
        outputCacheL.assign(static_cast<size_t>(maxBlockSize), 0.0f);
        outputCacheR.assign(static_cast<size_t>(maxBlockSize), 0.0f);

        // TODO: prepare your DSP objects here
        // voicePool.prepare(sampleRate);
        // filter.prepare(sampleRate);
    }

    void releaseResources() override
    {
        // TODO: voice pool allNotesOff(), etc.
    }

    void reset() override
    {
        externalFilterMod = 0.0f;
        externalMorphMod  = 0.0f;
        std::fill(outputCacheL.begin(), outputCacheL.end(), 0.0f);
        std::fill(outputCacheR.begin(), outputCacheR.end(), 0.0f);
        // TODO: reset DSP state
    }

    //==========================================================================
    // Audio
    //==========================================================================

    void renderBlock(juce::AudioBuffer<float>& buffer,
                     juce::MidiBuffer& midi,
                     int numSamples) override
    {
        if (numSamples <= 0) return;
        juce::ScopedNoDenormals noDenormals;

        // ParamSnapshot — read everything once, at the top
        const float cutoff     = safeLoadF(pFilterCutoff, 2000.0f);
        const float resonance  = safeLoadF(pFilterReso,   0.3f);
        const float ampAttack  = safeLoadF(pAmpAttack,    0.01f);
        const float ampDecay   = safeLoadF(pAmpDecay,     0.3f);
        const float ampSustain = safeLoadF(pAmpSustain,   0.8f);
        const float ampRelease = safeLoadF(pAmpRelease,   0.3f);

        // Apply coupling accumulators
        const float effectiveCutoff = juce::jlimit(20.0f, 20000.0f,
                                                    cutoff + externalFilterMod);

        // Process MIDI
        for (const auto meta : midi)
        {
            const auto msg = meta.getMessage();
            if (msg.isNoteOn())       noteOn(msg.getNoteNumber(), msg.getVelocity());
            else if (msg.isNoteOff()) noteOff(msg.getNoteNumber());
        }

        // Render
        auto* L = buffer.getWritePointer(0);
        auto* R = buffer.getWritePointer(buffer.getNumChannels() > 1 ? 1 : 0);

        for (int s = 0; s < numSamples; ++s)
        {
            // TODO: replace with actual DSP
            float sample = 0.0f; // voicePool.render(effectiveCutoff, resonance);

            L[s] = sample;
            R[s] = sample;

            // Write output cache — required for getSampleForCoupling()
            if (s < static_cast<int>(outputCacheL.size()))
            {
                outputCacheL[static_cast<size_t>(s)] = L[s];
                outputCacheR[static_cast<size_t>(s)] = R[s];
            }
        }

        // Reset coupling accumulators
        externalFilterMod = 0.0f;
        externalMorphMod  = 0.0f;
    }

    //==========================================================================
    // Coupling
    //==========================================================================

    float getSampleForCoupling(int channel, int sampleIndex) const override
    {
        auto si = static_cast<size_t>(sampleIndex);
        if (channel == 0 && si < outputCacheL.size()) return outputCacheL[si];
        if (channel == 1 && si < outputCacheR.size()) return outputCacheR[si];
        return 0.0f;
    }

    void applyCouplingInput(CouplingType type, float amount,
                            const float* /*sourceBuffer*/,
                            int /*numSamples*/) override
    {
        switch (type)
        {
            case CouplingType::AmpToFilter:
                externalFilterMod += amount * 8000.0f;
                break;
            case CouplingType::EnvToMorph:
                externalMorphMod += amount * 0.5f;
                break;
            default:
                break;
        }
    }

    //==========================================================================
    // Parameters
    //==========================================================================

    static void addParameters(
        std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params)
    {
        addParametersImpl(params);
    }

    juce::AudioProcessorValueTreeState::ParameterLayout
        createParameterLayout() override
    {
        std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
        addParametersImpl(params);
        return {params.begin(), params.end()};
    }

    void attachParameters(juce::AudioProcessorValueTreeState& apvts) override
    {
        pFilterCutoff = apvts.getRawParameterValue("myeng_filterCutoff");
        pFilterReso   = apvts.getRawParameterValue("myeng_filterReso");
        pAmpAttack    = apvts.getRawParameterValue("myeng_ampAttack");
        pAmpDecay     = apvts.getRawParameterValue("myeng_ampDecay");
        pAmpSustain   = apvts.getRawParameterValue("myeng_ampSustain");
        pAmpRelease   = apvts.getRawParameterValue("myeng_ampRelease");
    }

    //==========================================================================
    // Identity
    //==========================================================================

    juce::String getEngineId()    const override { return "MyEngine"; }
    juce::Colour getAccentColour() const override { return juce::Colour(0xFF00A6D6); }
    int          getMaxVoices()   const override { return kMaxVoices; }

    int getActiveVoiceCount() const override
    {
        // TODO: count active voices
        return 0;
    }

private:
    //==========================================================================
    // Parameter definitions — called from both addParameters() and
    // createParameterLayout(). Keep this as the single source of truth.
    //==========================================================================
    static void addParametersImpl(
        std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params)
    {
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"myeng_filterCutoff", 1}, "My Engine Filter Cutoff",
            juce::NormalisableRange<float>(20.0f, 20000.0f, 0.1f, 0.3f), 2000.0f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"myeng_filterReso", 1}, "My Engine Filter Resonance",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.3f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"myeng_ampAttack", 1}, "My Engine Amp Attack",
            juce::NormalisableRange<float>(0.001f, 10.0f, 0.001f, 0.4f), 0.01f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"myeng_ampDecay", 1}, "My Engine Amp Decay",
            juce::NormalisableRange<float>(0.001f, 10.0f, 0.001f, 0.4f), 0.3f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"myeng_ampSustain", 1}, "My Engine Amp Sustain",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.8f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"myeng_ampRelease", 1}, "My Engine Amp Release",
            juce::NormalisableRange<float>(0.001f, 10.0f, 0.001f, 0.4f), 0.3f));

        // Add all remaining parameters following the same pattern.
        // Each ParameterID must use the "myeng_" prefix.
    }

    //==========================================================================
    // Helper: voice management (minimal example)
    //==========================================================================

    void noteOn(int noteNumber, int velocity)
    {
        // TODO: implement voice allocation
        (void)noteNumber; (void)velocity;
    }

    void noteOff(int noteNumber)
    {
        // TODO: implement voice release
        (void)noteNumber;
    }

    //==========================================================================
    // Safe parameter load (guards against nullptr before attachParameters())
    //==========================================================================

    static int safeLoad(std::atomic<float>* p, int fallback) noexcept {
        return (p != nullptr) ? static_cast<int>(p->load()) : fallback;
    }
    static float safeLoadF(std::atomic<float>* p, float fallback) noexcept {
        return (p != nullptr) ? p->load() : fallback;
    }

    //==========================================================================
    // State
    //==========================================================================

    double sr  = 44100.0;
    float  srf = 44100.0f;

    // Coupling accumulators — written by applyCouplingInput(), consumed in
    // renderBlock(), reset to 0 at the end of renderBlock()
    float externalFilterMod = 0.0f;
    float externalMorphMod  = 0.0f;

    // Per-sample output cache for coupling reads
    std::vector<float> outputCacheL;
    std::vector<float> outputCacheR;

    // Cached APVTS parameter pointers — null until attachParameters() is called
    std::atomic<float>* pFilterCutoff = nullptr;
    std::atomic<float>* pFilterReso   = nullptr;
    std::atomic<float>* pAmpAttack    = nullptr;
    std::atomic<float>* pAmpDecay     = nullptr;
    std::atomic<float>* pAmpSustain   = nullptr;
    std::atomic<float>* pAmpRelease   = nullptr;
};

} // namespace xoceanus
```

**`Source/Engines/MyEngine/MyEngineAdapter.cpp`:**

```cpp
#include "MyEngineAdapter.h"
// All DSP is inline in the .h. This stub is required by CMake.
// Registration is in XOceanusProcessor.cpp.
```

**In `Source/XOceanusProcessor.cpp`** (three locations):

```cpp
// 1. At top with other includes:
#include "Engines/MyEngine/MyEngineAdapter.h"

// 2. With other static registrations (after the existing blocks):
static bool registered_MyEngine =
    xoceanus::EngineRegistry::instance().registerEngine(
        "MyEngine",
        []() -> std::unique_ptr<xoceanus::SynthEngine> {
            return std::make_unique<xoceanus::MyEngineAdapter>();
        });

// 3. In createParameterLayout(), with other addParameters() calls:
MyEngineAdapter::addParameters(params);
```

**In `Source/Core/PresetManager.h`**, add `"MyEngine"` to `validEngineNames`:

```cpp
inline const juce::StringArray validEngineNames {
    "OddfeliX", "OddOscar", "Overdub", "Odyssey", "Oblong",
    // ... existing engines ...
    "MyEngine",  // <-- add here
};
```

**Write a factory preset** at `Presets/XOceanus/{Mood}/01_MyPreset.xometa`:

```json
{
  "schema_version": 1,
  "name": "My First Preset",
  "mood": "Foundation",
  "engines": ["MyEngine"],
  "author": "XO_OX",
  "version": "1.0.0",
  "description": "A starting point for MyEngine",
  "tags": ["template"],
  "macroLabels": ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"],
  "couplingIntensity": "None",
  "dna": {
    "brightness": 0.5, "warmth": 0.5, "movement": 0.5,
    "density": 0.5, "space": 0.5, "aggression": 0.5
  },
  "parameters": {
    "MyEngine": {
      "myeng_filterCutoff": 2000.0,
      "myeng_filterReso": 0.3,
      "myeng_ampAttack": 0.01,
      "myeng_ampDecay": 0.3,
      "myeng_ampSustain": 0.8,
      "myeng_ampRelease": 0.3
    }
  },
  "coupling": { "pairs": [] }
}
```

---

## What Was Found vs. What Was Inferred

**Directly verified in source code:**
- `SynthEngine` interface: `Source/Core/SynthEngine.h` (complete, all 10 methods)
- `EngineRegistry` and the static registration pattern: `Source/Core/EngineRegistry.h`
- Registration is done via raw `registerEngine()` calls in `XOceanusProcessor.cpp`, NOT via the `REGISTER_ENGINE` macro (the macro exists in `EngineRegistry.h` but is not used in the actual codebase)
- The `addParameters()` / `addParametersImpl()` / `createParameterLayout()` three-method pattern: verified in `BiteEngine.h` and `SnapEngine.h`
- The coupling accumulator pattern (write in `applyCouplingInput`, consume in `renderBlock`, reset after): verified in `BiteEngine.h` and `OverworldEngine.h`
- The output cache pattern (per-sample write during render, O(1) read in `getSampleForCoupling`): verified in both engines, with P0 bug history confirming this was initially missed
- The `.cpp` one-line stub convention: verified in `BiteEngine.cpp`
- `validEngineNames` location and `resolveEngineAlias()`: `Source/Core/PresetManager.h`
- Preset `.xometa` format and parameter keying: `Presets/XOceanus/Foundation/01_Bathyal_Floor.xometa`
- The three locations requiring edits in `XOceanusProcessor.cpp` (include, registration, addParameters call): verified at lines 3-26, 31-126, 184-207

**Inferred or extrapolated:**
- The six-step verification checklist under "Step 8" is recommended practice based on patterns seen in coupling audit and P0 fix documentation — no explicit test suite was found
- The `validEngineNames` edit requirement is inferred from `PresetManager::parseJSON()` validation code; there is no explicit warning in the engine onboarding docs about this step
- Preset parameter keying uses the canonical engine name (e.g., `"MyEngine"`) matching `getEngineId()`, not the XPN class name — inferred from the `Bathyal_Floor.xometa` example using `"XOwlfish"` and the alias resolution logic in `parseJSON()`
