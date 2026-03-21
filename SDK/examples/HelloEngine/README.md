# Hello Engine — XOmnibus SDK Example

A complete, minimal XOmnibus engine demonstrating every integration contract.
Start here before building your own engine.

---

## What This Engine Does

| Component | Implementation |
|-----------|---------------|
| Oscillator | Sine wave, per-voice |
| Filter | One-pole lowpass with resonance feedback |
| Envelope | Pitch envelope (attack transient) + exponential amplitude decay |
| Polyphony | 2 voices, round-robin allocation |
| Parameters | `hlo_pitch`, `hlo_cutoff`, `hlo_resonance`, `hlo_decay` |
| Macros | M1=Pitch, M2=Cutoff, M3=Resonance, M4=Decay |
| Coupling send | All audio (channel 0/1) via `getSampleForCoupling()` |
| Coupling receive | `AmpToFilter` — another engine's amplitude opens the filter |
| Doctrines | D001–D006 all satisfied (see Doctrine Checklist below) |

---

## Directory Layout

```
SDK/examples/HelloEngine/
  HelloEngine.h      — The engine (single-header, all DSP inline)
  README.md          — This file
```

---

## Building

### Prerequisites

- C++17 compiler (Clang 12+, GCC 10+, MSVC 2019+)
- XOmnibus SDK headers in your include path (`SDK/include/`)
- No JUCE required for SDK engines

### macOS / Linux — shared library

```bash
# From the repo root
clang++ -std=c++17 \
        -I SDK/include \
        -fvisibility=hidden \
        -shared -fPIC \
        -o HelloEngine.dylib \
        SDK/examples/HelloEngine/HelloEngine.h
# (HelloEngine.h is the compilation unit — there is no .cpp)
```

For a .cpp stub (preferred for incremental builds), create `HelloEngine.cpp`:

```cpp
#include "HelloEngine.h"
```

Then compile:

```bash
clang++ -std=c++17 -I SDK/include \
        -fvisibility=hidden -shared -fPIC \
        -o HelloEngine.dylib \
        SDK/examples/HelloEngine/HelloEngine.cpp
```

### Windows — DLL

```bat
cl /std:c++17 /I SDK\include /LD SDK\examples\HelloEngine\HelloEngine.cpp
```

### Inside XOmnibus (as a built-in engine)

Add to `Source/Core/EngineRegistry.h`:

```cpp
#include "../Engines/HelloEngine/HelloEngine.h"
// In registerAllEngines():
registry.add (std::make_unique<HelloEngine>());
```

---

## Validate Before Shipping

Run the validation script to check doctrine compliance:

```bash
python3 SDK/tools/validate_engine.py SDK/examples/HelloEngine/HelloEngine.h
```

All checks must pass before submitting to the XO_OX engine registry.

---

## Parameter Reference

| ID | Range | Default | Unit | Macro |
|----|-------|---------|------|-------|
| `hlo_pitch` | -24 .. +24 | 0 | semitones | M1 CHARACTER |
| `hlo_cutoff` | 20 .. 20000 | 2000 | Hz | M2 MOVEMENT |
| `hlo_resonance` | 0 .. 0.95 | 0.2 | — | M3 COUPLING |
| `hlo_decay` | 0.01 .. 4.0 | 0.5 | seconds | M4 SPACE |

---

## Doctrine Checklist

Every XOmnibus engine must satisfy all 6 Doctrines before shipping.
HelloEngine satisfies them as follows:

| ID | Doctrine | HelloEngine Implementation |
|----|----------|---------------------------|
| D001 | Velocity Must Shape Timbre | Velocity scales cutoff boost (+4 kHz at full velocity) and amplitude. Pitch envelope also velocity-gated. |
| D002 | Modulation is the Lifeblood | Triangle LFO breathes filter cutoff ±500 Hz. Mod wheel controls LFO rate (0.01–4 Hz). Aftertouch adds resonance shimmer. 4 macros wired. |
| D003 | The Physics IS the Synthesis | N/A for this engine (no physical model). Engines using physical models must cite source papers. |
| D004 | Dead Parameters Are Broken Promises | All 4 `hlo_*` parameters affect audio output. Verified by `validate_engine.py`. |
| D005 | An Engine That Cannot Breathe Is a Photograph | LFO rate floor is 0.01 Hz (set by mod wheel minimum). Engine never stops modulating during a held note. |
| D006 | Expression Input Is Not Optional | Velocity → timbre (D001 overlap). Mod wheel (CC1) → LFO rate. Aftertouch → resonance. |

---

## Coupling Integration

HelloEngine participates in the XOmnibus coupling matrix in two roles:

### As a sender (any engine can listen to HelloEngine)

`getSampleForCoupling(channel, sampleIndex)` returns the cached audio output.
Use coupling type `AudioToFM`, `AudioToRing`, or `AmpToFilter` with HelloEngine as source.

### As a receiver (HelloEngine listens to another engine)

Only `AmpToFilter` is handled. Route any engine → HelloEngine with type `AmpToFilter`:

```
MegaCouplingMatrix: EngineA (send=Audio) → HelloEngine (receive=AmpToFilter, amount=0.8)
```

Result: EngineA's amplitude sweeps HelloEngine's filter cutoff open (up to +8 kHz).
This creates "sidechain filter" behavior — e.g., a drum engine opening a pad engine's filter.

---

## Adapting This Engine

To create a new engine from HelloEngine:

1. Copy `SDK/examples/HelloEngine/` to `SDK/examples/YourEngine/`
2. Rename `HelloEngine` → `YourEngine` everywhere (class name, file names, export macro)
3. Change parameter prefix `hlo_` → `your_` (freeze the prefix before release — never rename)
4. Replace DSP: oscillator, filter, envelope in `HelloVoice::tick()`
5. Run `python3 SDK/tools/validate_engine.py YourEngine.h` — all checks must pass
6. Write at least 10 presets in `.xometa` format (`Presets/XOmnibus/{mood}/`)

### Naming rules

- Engine ID: an O-word (e.g. "Onyx", "Ortolan", "Ozone")
- Parameter prefix: short + unique (e.g. `onyx_`, `ort_`, `oz_`)
- Accent colour: pick one not used by existing engines (see CLAUDE.md engine table)

---

## Frequently Asked Questions

**Q: Why is all DSP in a `.h` file?**
A: XOmnibus architecture rule: "All DSP lives inline in `.h` headers; `.cpp` files are one-line stubs."
This makes engines portable and testable in isolation without a build system.

**Q: Why does the filter use `exp(-2*PI*fc/sr)` instead of `w/(w+1)`?**
A: Matched-Z transform gives correct frequency at any sample rate (44.1 kHz, 48 kHz, 96 kHz).
The Euler approximation (`w/(w+1)`) only works near DC. See CLAUDE.md DSP rules.

**Q: Why is there a `flushDenormal()` call in the filter?**
A: Denormal floating-point numbers (values ~1e-40) can cause 100x CPU spikes on x86.
They occur naturally in decaying feedback paths. Flushing keeps the filter CPU-stable.

**Q: Can I allocate memory inside `renderBlock()`?**
A: No. This violates the XOmnibus Architecture Rule: "Never allocate memory on the audio thread."
Allocate all buffers in `prepare()` and resize/clear them there.

**Q: What is the SilenceGate?**
A: An SRO (Spectral Resonance Object) that bypasses all DSP when the engine has been
silent for longer than the hold time (200 ms for HelloEngine). This eliminates idle CPU cost.
See `Source/Core/SynthEngine.h` for the JUCE-native version with `SilenceGate`.

---

## See Also

- `SDK/include/xomnibus/SynthEngine.h` — full interface definition
- `SDK/include/xomnibus/CouplingTypes.h` — all 14 coupling types
- `SDK/include/xomnibus/EngineModule.h` — `XOMNIBUS_EXPORT_ENGINE` macro
- `SDK/tools/validate_engine.py` — doctrine validation script
- `SDK/templates/MinimalEngine/MinimalEngine.h` — bare-minimum template (1 param, 1 voice)
- `Source/Core/SynthEngine.h` — JUCE-native interface (for built-in engines)
- `Docs/xomnibus_new_engine_process.md` — full new-engine process guide
