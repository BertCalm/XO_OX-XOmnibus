# Sweep Critical & High Findings — 2026-03-20

Zero Critical findings. Three High findings requiring prompt attention.

---

## HIGH-1 — Reef FDN read-position can escape buffer bounds at high sample rates

**File:** `Source/DSP/Effects/AquaticFXSuite.h` ~line 686
**Area:** Aquatic FX, Stage 4 (Reef reverb)

**Problem:**
```cpp
int rp = (delays[ch].writePos - static_cast<int> (kPrimeLengths48k[ch] * srScale * size * 2.7f + 1)
           + len * 8) % len;
rp = std::max (0, rp % len);
```
At 96 kHz (`srScale = 2.0`) with `size = 1.0`, the delay buffer is hard-capped at 5000 samples. The read offset computes as `4799 * 2.0 * 1.0 * 2.7 = 25913`. The padding `len * 8 = 40000` handles this case. However at 192 kHz (`srScale = 4.0`) the offset becomes `4799 * 4 * 2.7 = 51826`, which exceeds `len * 8 = 40000`. The modulo produces a negative value that the `std::max(0, ...)` guard does not recover correctly because the `% len` inside `std::max` is applied to the already-wrong value.

Additionally, the outer `(... + len * 8) % len` already fully wraps the address — the inner `rp % len` on the same value is a redundant no-op that may confuse the reader.

**Symptoms:** Buffer read from an uninitialized or wrong memory address inside the FDN at 192 kHz. Likely produces garbage reverb output rather than a crash (reads within the allocated vector, just wrong offset), but this is technically undefined behavior territory.

**Fix:**
```cpp
// Pre-clamp the effective delay offset to [1, len-1] before subtracting.
int effectiveLen = static_cast<int> (kPrimeLengths48k[ch] * srScale * size * 2.7f + 1);
effectiveLen = std::max (1, std::min (effectiveLen, len - 1));
int rp = ((delays[ch].writePos - effectiveLen) + len) % len;
```

---

## HIGH-2 — AttractorDrive hardcodes 44100 Hz for ODE normalization (sample rate independence bug)

**File:** `Source/DSP/Effects/AttractorDrive.h` line 84
**Area:** Mathematical FX, Stage 4 (Attractor Drive)

**Problem:**
```cpp
float basedt = (0.0001f + speed * 0.002f) * (44100.0f / sr);
```
The ratio `44100.0f / sr` normalizes the Lorenz attractor's orbit speed *down* at higher sample rates. At 96 kHz the orbit is half as fast as at 44100 Hz, and at 192 kHz it's a quarter speed. This changes the sonic character of the effect when the host runs at non-44100 rates.

If the intent is "orbit speed should be perceptually the same at all sample rates", the ratio is inverted. It should be `sr / 44100.0f` to keep the physical orbit time constant:
- At 48 kHz: `48000/44100 ≈ 1.09` (slightly faster, acceptable)
- At 96 kHz: `96000/44100 ≈ 2.18` (doubles, matches the doubled tick rate)

If the intent is specifically Lorenz numerical stability (smaller step = more stable Euler integration), then the current direction is correct and the slower-at-96kHz behavior is by design.

**Fix (pending design decision):**
- If sample-rate-independent orbit speed: change to `(0.0001f + speed * 0.002f) * (sr / 44100.0f)`
- If intentional stability tradeoff: add comment `// Intentionally slower at 96kHz — Euler stability budget`

**Immediate action:** Review the original design intent and resolve the ambiguity. The effect currently sounds different at 44100 vs 96 kHz.

---

## HIGH-3 — OBRIX `renderBlock` signature diverges from SDK interface (portability/documentation)

**File:** `Source/Engines/Obrix/ObrixEngine.h` line 326
**Area:** OBRIX DSP
**Runtime impact:** None (in-tree build uses internal JUCE SynthEngine correctly)
**Portability impact:** High

**Problem:**
OBRIX implements the internal JUCE-based `SynthEngine` interface, which takes `juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi, int numSamples`. The SDK `SynthEngine` interface (used by community developers) takes `StereoBuffer& buffer, const MidiEventList& midi` with no sample count (it lives in `StereoBuffer::numSamples`).

These are two completely different calling conventions. Any attempt to compile OBRIX as an SDK community engine will fail to compile. This is actually the desired failure (internal engines shouldn't be shipped as third-party modules), but it creates confusion when OBRIX is held up as a reference implementation.

**Fix:**
Add a comment block at the top of `ObrixEngine.h`:
```cpp
// NOTE: ObrixEngine implements the INTERNAL xoceanus::SynthEngine interface
// (juce::AudioBuffer / juce::MidiBuffer), not the public SDK interface
// (xoceanus::StereoBuffer / xoceanus::MidiEventList).
// This engine is not directly portable as an SDK community engine without
// an adapter. See SDK/templates/MinimalEngine/ for the SDK interface.
```

Also update `CLAUDE.md` to clarify the distinction between internal engines and SDK engines.

---

*See full details in `Docs/sweep-report-2026-03-20.md`.*
