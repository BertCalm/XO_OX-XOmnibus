# XOceanus Engine SDK

Build your own XOceanus engine — no JUCE required.

The SDK lets anyone write a custom synthesis engine that loads into XOceanus
at runtime, participates in the coupling matrix, and appears in the UI alongside
the 76 factory engines.

---

## Quick Start

```bash
# 1. Study the Hello World example
open SDK/examples/HelloEngine/HelloEngine.h

# 2. Validate it passes all checks
python3 SDK/tools/validate_engine.py SDK/examples/HelloEngine/HelloEngine.h

# 3. Copy the template, rename, and build
cp -r SDK/examples/HelloEngine SDK/examples/MyEngine
# ... edit MyEngine.h ...
clang++ -std=c++17 -I SDK/include -fvisibility=hidden -shared -fPIC \
        -o MyEngine.dylib SDK/examples/MyEngine/MyEngine.h
```

---

## What Is an Engine?

An **engine** is a self-contained synthesizer module. XOceanus hosts up to 4 engines
simultaneously and routes audio + modulation between them through the
**MegaCouplingMatrix** (15 coupling types).

Every engine:
- Implements the `xoceanus::SynthEngine` interface
- Declares its own namespaced parameters (`prefix_paramName`)
- Responds to MIDI (note on/off, velocity, aftertouch, mod wheel, pitch bend)
- Sends audio for other engines to use as modulation
- Receives modulation from other engines

---

## SDK Directory Layout

```
SDK/
  include/
    xoceanus/
      SynthEngine.h       — the interface (inherit this)
      CouplingTypes.h     — 15 coupling type enums
      EngineModule.h      — XOCEANUS_EXPORT_ENGINE macro
  examples/
    HelloEngine/
      HelloEngine.h       — complete "Hello World" engine (start here)
      README.md           — build instructions + doctrine checklist
  templates/
    MinimalEngine/
      MinimalEngine.h     — bare-minimum template (1 param, 1 voice)
  tools/
    validate_engine.py    — doctrine + interface compliance checker
  README.md               — this file
```

---

## The SynthEngine Interface

```cpp
#include <xoceanus/SynthEngine.h>

class MyEngine : public xoceanus::SynthEngine
{
public:
    // Lifecycle
    void prepare (double sampleRate, int maxBlockSize) override;
    void reset() override;
    void releaseResources() override;          // optional but expected

    // Audio — MUST be real-time safe (no alloc, no I/O, no locks)
    void renderBlock (xoceanus::StereoBuffer& buffer,
                      const xoceanus::MidiEventList& midi) override;

    // Coupling
    float getSampleForCoupling (int channel, int sampleIndex) const override;
    void  applyCouplingInput   (xoceanus::CouplingType type, float amount,
                                const float* sourceBuffer, int numSamples) override;

    // Parameters
    std::vector<xoceanus::ParameterDef> getParameterDefs() const override;
    void  setParameter (const std::string& id, float value) override;
    float getParameter (const std::string& id) const override;

    // Identity
    std::string       getEngineId()       const override;
    xoceanus::Colour  getAccentColour()   const override;
    int               getMaxVoices()      const override;
    int               getActiveVoiceCount() const override;
};
```

Full documentation: `SDK/include/xoceanus/SynthEngine.h`

---

## The 6 Doctrines

Every XOceanus engine must satisfy all 6 Doctrines. The validation script checks these
automatically. Engines that fail any Doctrine will not be admitted to the registry.

| ID | Doctrine | Minimum Requirement |
|----|----------|---------------------|
| D001 | **Velocity Must Shape Timbre** | Velocity drives filter brightness or harmonic content — not just amplitude. |
| D002 | **Modulation is the Lifeblood** | At least 2 LFOs (or 1 LFO + another modulator), mod wheel (CC1), aftertouch, and 4 working macros. |
| D003 | **The Physics IS the Synthesis** | Any physically modeled engine must cite source papers. Approximations require documented justification. |
| D004 | **Dead Parameters Are Broken Promises** | Every declared parameter must affect audio output. Zero tolerance for wired-but-silent parameters. |
| D005 | **An Engine That Cannot Breathe Is a Photograph** | At least one LFO with rate floor ≤ 0.01 Hz (one cycle per 100 seconds). |
| D006 | **Expression Input Is Not Optional** | Velocity → timbre + at least one CC (aftertouch and/or mod wheel). |

---

## Coupling System

XOceanus routes audio and modulation between engines through the **MegaCouplingMatrix**.
Your engine participates in two roles:

### Sending (making yourself available as a modulation source)

```cpp
// Cache your output in renderBlock():
for (int s = 0; s < ns; ++s) {
    float sample = computeSample();
    buffer.left[s] += sample;
    couplingBuf[s] = sample;         // cache for coupling reads
}

// Return from cache — O(1), called per-sample during tight coupling:
float getSampleForCoupling (int /*channel*/, int sampleIndex) const override {
    return couplingBuf[sampleIndex]; // bounds-check before accessing
}
```

### Receiving (being modulated by another engine)

```cpp
// Accumulate in applyCouplingInput() — called before renderBlock():
void applyCouplingInput (CouplingType type, float amount,
                         const float* src, int numSamples) override {
    if (type == CouplingType::AmpToFilter) {
        float rms = computeRMS(src, numSamples);
        couplingFilterMod += rms * amount;  // consume in renderBlock()
    }
}
```

### Supported coupling types

| Type | What it does |
|------|-------------|
| `AmpToFilter` | Source amplitude opens/closes receiver's filter |
| `AmpToPitch` | Source amplitude modulates receiver's pitch |
| `LFOToPitch` | Source LFO modulates receiver's pitch |
| `AudioToFM` | Source audio becomes FM carrier for receiver |
| `AudioToRing` | Source audio ring-modulates receiver |
| `FilterToFilter` | Source filter output drives receiver's filter input |
| `AmpToChoke` | Source amplitude ducks/chokes receiver |
| `EnvToDecay` | Source envelope modulates receiver's decay time |
| `PitchToPitch` | Source pitch transposes receiver (harmony) |
| ... | See `SDK/include/xoceanus/CouplingTypes.h` for all 15 |

---

## Parameter Naming Convention

Parameter IDs are frozen once released. Choose carefully.

```
Format: {prefix}_{paramName}
         ^^^^^^   ^^^^^^^^^
         2-8 lowercase chars   camelCase or lowercase
         unique to your engine

Examples:  hlo_cutoff   onyx_depth   ort_spread
```

Rules:
- Prefix must be unique across all registered engines (check the engine table in CLAUDE.md)
- Never rename a parameter ID after shipping — presets depend on them
- All parameters must appear in both `getParameterDefs()` and `setParameter()`
- Every parameter must affect audio output (D004)

---

## Audio-Thread Safety Rules

These are not suggestions. Violating them causes dropouts and crashes.

| Rule | Required |
|------|----------|
| No heap allocation in `renderBlock()` | Yes — allocate in `prepare()` |
| No blocking I/O in `renderBlock()` | Yes — no file reads, printf, sleep |
| No mutex locking in `renderBlock()` | Yes — use atomics or lock-free patterns |
| No exceptions thrown in `renderBlock()` | Yes — use return codes or flags |
| `getSampleForCoupling()` must be O(1) | Yes — return a cached value |
| Denormal protection in feedback paths | Yes — flush near-zero values |

---

## Exporting Your Engine

The `XOCEANUS_EXPORT_ENGINE` macro generates the two C functions XOceanus needs to
load your engine as a shared library:

```cpp
// At file scope (outside any class or namespace):
XOCEANUS_EXPORT_ENGINE (
    MyEngine,         // C++ class name
    "Onyx",           // Engine ID (unique O-word)
    "Onyx Engine",    // Display name
    "onyx_",          // Parameter prefix
    0x1A, 0x1A, 0x2B, // Accent colour RGB
    "1.0.0",          // Version string
    "Your Studio"     // Author
)
```

---

## Validation

Run the validator before submitting to the engine registry:

```bash
python3 SDK/tools/validate_engine.py path/to/YourEngine.h
```

The validator checks:
- All `SynthEngine` virtual methods implemented
- `XOCEANUS_EXPORT_ENGINE` macro present
- Parameter naming convention (`prefix_name`)
- D001–D006 doctrine patterns
- No banned audio-thread operations (allocation, I/O, locks)
- Denormal protection
- Silence gate / idle bypass
- Coupling contract (O(1) send, accumulator receive)

Exit code 0 = all FAIL checks pass. WARN items are advisory.

---

## Examples

| Example | What it demonstrates |
|---------|---------------------|
| `examples/HelloEngine/` | Complete engine: 2 voices, 4 params, coupling, all 6 doctrines |
| `templates/MinimalEngine/` | Bare minimum: 1 voice, 1 param, sine oscillator |

Start with HelloEngine if you want to understand the full integration.
Start with MinimalEngine if you want the smallest possible scaffolding.

---

## Submitting to the Registry

To have your engine included in a future XOceanus release:

1. Run `validate_engine.py` — zero FAIL results required
2. Write at least 10 factory presets in `.xometa` format
3. Choose an O-word engine ID not already registered (see CLAUDE.md engine table)
4. Open a pull request to `BertCalm/XO_OX-XOceanus` with:
   - Engine `.h` file in `SDK/community/{EngineName}/`
   - At least 10 presets in `Presets/XOceanus/{mood}/`
   - A brief description (what it sounds like, what makes it interesting)

---

## SDK Version

SDK ABI version: **1**

The `sdkVersion` field in `EngineMetadata` will be bumped on any breaking interface change.
Engines built against a given SDK version will continue loading without modification until the next breaking change.

---

## See Also

- `SDK/examples/HelloEngine/README.md` — detailed build + doctrine walkthrough
- `Source/Core/SynthEngine.h` — JUCE-native interface (for built-in engines)
- `Docs/xoceanus_new_engine_process.md` — full process guide (ideation → integration)
- `Docs/xoceanus_master_specification.md` — authoritative product specification
