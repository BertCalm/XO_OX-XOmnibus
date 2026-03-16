# Skill: /sro-optimizer

**Invoke with:** `/sro-optimizer [engine-name | audit | integrate]`
**Status:** LIVE
**Purpose:** Sustainability & Resource Optimization — audit engines for CPU waste, apply zero-idle bypass, replace expensive math with LUTs, reduce coupling to control rate, and enforce the "Earn Your Place" protocol.

---

## Core Ethos: "The Lean Synthesis Manifesto"

Every CPU cycle must earn its place. The SRO Framework treats the user's CPU and memory as a finite, precious resource.

| Principle | Rule |
|-----------|------|
| No Low-Value Bloat | If a feature adds 10% CPU but only 1% perceived sonic improvement, reject or refactor |
| Zero Performance Waste | Inactive slots or silent tails consume zero cycles |
| Logical Upcycling | Reuse `Source/DSP/` kernel components before writing engine-local code |
| Control vs. Audio Rate | Modulation runs at 1/32–1/64 rate; only final output runs at audio rate |
| Earn Your Place | Every engine slot must justify its CPU cost via creative value (ROI) |

---

## When to Use This Skill

Use this skill when:
- **Auditing** an engine for CPU waste or performance bloat
- **Integrating** SRO components into a new or existing engine
- **Profiling** a 4-slot configuration for total CPU budget
- **Replacing** expensive per-sample math with LUTs or FastMath
- **Optimizing** coupling routes to run at control rate
- A seance or health check flags CPU concerns
- Preparing for release and want to minimize user CPU load

---

## SRO Components

All SRO DSP lives in `Source/DSP/SRO/`. These are the building blocks:

| Component | File | Purpose |
|-----------|------|---------|
| SilenceGate | `SRO/SilenceGate.h` | Zero-idle bypass — skips renderBlock() when output is silent |
| ControlRateReducer | `SRO/ControlRateReducer.h` | Decimates modulation to 1/32 or 1/64 rate with linear interpolation |
| LookupTable | `SRO/LookupTable.h` | Pre-computed function tables replacing sin/tanh/exp/pow2 |
| SROAuditor | `SRO/SROAuditor.h` | CPU ROI diagnostic — per-slot cost-benefit analysis |

Supporting kernel components (already exist):
| Component | File | Purpose |
|-----------|------|---------|
| FastMath | `DSP/FastMath.h` | Inline fast approximations (fastSin, fastTanh, fastExp, etc.) |
| EngineProfiler | `DSP/EngineProfiler.h` | Per-engine CPU measurement with RAII ScopedMeasurement |
| CytomicSVF | `DSP/CytomicSVF.h` | Shared filter (all engines should use this, not local filters) |
| Saturator | `DSP/Effects/Saturator.h` | Shared saturation (all engines should use this, not local tanh) |

---

## Phase 1: Audit an Engine (`/sro-optimizer [engine-name]`)

### Step 1: Profile Baseline

Read the engine header and identify:

```
Search for in Source/Engines/{Name}/{Name}Engine.h:
  - std::sin, std::cos, std::tan, std::exp, std::pow, std::tanh
  - Any per-sample transcendental math NOT using FastMath.h
  - Local filter implementations (not CytomicSVF)
  - Local saturation (not Saturator.h or FastMath::softClip)
  - Local envelope implementations
  - Any heap allocation in renderBlock (new, malloc, vector::push_back, resize)
```

### Step 2: Check for Zero-Idle Compliance

```
Search for: SilenceGate, isBypassed, silence, bypass, idle
Question: Does the engine skip processing when no notes are sounding?
```

| Finding | Status |
|---------|--------|
| SilenceGate integrated, renderBlock skips when silent | ✅ PASS |
| Manual silence check exists but imprecise | ⚠️ PARTIAL |
| No silence detection — always processes full block | ❌ FAIL |

### Step 3: Check Coupling Rate

```
Search for: applyCouplingInput, coupling, ControlRateReducer
Question: Is coupling processed at audio rate or control rate?
```

| Finding | Status |
|---------|--------|
| Coupling uses ControlRateReducer at 1/32+ | ✅ PASS |
| Coupling processes every sample but signals are slow (LFO/env) | ⚠️ WASTEFUL |
| No coupling implementation (stub) | N/A |

### Step 4: Math Efficiency Scan

Count per-sample transcendental calls and classify:

| Function | FastMath Alternative | LUT Alternative | Action |
|----------|---------------------|-----------------|--------|
| `std::sin(x)` | `fastSin(x)` ~0.02% err | `SROTables::sin().lookup(x)` ~0.001% err | Replace |
| `std::cos(x)` | `fastCos(x)` | Via sin LUT with phase offset | Replace |
| `std::tan(x)` | `fastTan(x)` ~0.03% err | Generate custom LUT | Replace (filter prewarp only) |
| `std::tanh(x)` | `fastTanh(x)` ~0.1% err | `SROTables::tanh().lookup(x)` | Replace |
| `std::exp(x)` | `fastExp(x)` ~4% err | `SROTables::exp().lookup(x)` ~0.05% err | Replace |
| `std::pow(2,x)` | `fastPow2(x)` ~0.1% err | `SROTables::pow2().lookup(x)` ~0.02% err | Replace |
| `std::pow(10,x)` | Via `fastExp(x * 2.302585)` | Via exp LUT | Replace |

**Decision tree:**
- If called < 1x per sample (e.g., once per block for coefficient calc): **keep std::**
- If called 1x per sample: **use FastMath**
- If called N× per sample (per-voice, per-oscillator): **use LUT**

### Step 5: Kernel Reuse Check

```
Scan for local implementations of:
  - SVF / state-variable filter → should use CytomicSVF
  - One-pole filter / smoothing → should use FastMath::smoothCoeffFromTime
  - Saturation / waveshaping → should use Saturator.h or fastTanh/softClip
  - PolyBLEP oscillator → should use DSP/PolyBLEP.h
  - Noise generator → centralize if identical to existing xorshift
```

| Finding | Status |
|---------|--------|
| All DSP uses kernel components | ✅ LEAN |
| 1–2 local utilities that could be centralized | ⚠️ UPCYCLE |
| Engine has its own filter/envelope/saturation implementations | ❌ BLOAT |

**Exception:** Engine-specific DSP that is genuinely unique (e.g., Organon's metabolic model, Oracle's GENDY) is NOT bloat. Only flag components that duplicate existing kernel functionality.

### Step 6: Generate SRO Report

```
=== SRO AUDIT: [ENGINE NAME] ===
Date: [date]

ZERO-IDLE
---------
SilenceGate:     [✅ Integrated | ⚠️ Partial | ❌ Missing]
Idle CPU:        [0.0% | measured value]

MATH EFFICIENCY
---------------
std:: transcendentals per sample: [count]
FastMath replacements available:  [count]
LUT replacements recommended:    [count]

KERNEL REUSE
------------
Local filters:     [0 | count — list them]
Local saturation:  [0 | count]
Local envelopes:   [0 | count]
Upcycle candidates: [list any local code that duplicates kernel]

COUPLING RATE
-------------
applyCouplingInput: [STUB | AUDIO-RATE | CONTROL-RATE]
Recommended:        [current is optimal | reduce to 1/N]

CPU SUMMARY
-----------
Estimated per-voice cost:  [low | medium | high]
Optimization potential:    [minimal | moderate | significant]

RECOMMENDATIONS (priority order):
1. [highest impact fix]
2. [next fix]
3. [next fix]
```

---

## Phase 2: Integrate SRO into an Engine (`/sro-optimizer integrate [engine-name]`)

### Step 1: Add SilenceGate

Add to the engine's private members:

```cpp
#include "../../DSP/SRO/SilenceGate.h"

// In engine class private section:
SilenceGate silenceGate;
```

Wire into lifecycle:

```cpp
void prepare(double sampleRate, int maxBlockSize) override
{
    // ... existing prepare code ...
    silenceGate.prepare(sampleRate, maxBlockSize);
}

void reset() override
{
    // ... existing reset code ...
    silenceGate.reset();
}
```

Wire into renderBlock:

```cpp
void renderBlock(juce::AudioBuffer<float>& buffer,
                 juce::MidiBuffer& midi, int numSamples) override
{
    // Wake on note-on
    for (const auto metadata : midi)
    {
        auto msg = metadata.getMessage();
        if (msg.isNoteOn())
            silenceGate.wake();
    }

    // Zero-idle bypass
    if (silenceGate.isBypassed() && midi.isEmpty())
    {
        buffer.clear();
        return;
    }

    // ... existing render code ...

    // Analyze output for silence detection
    silenceGate.analyzeBlock(buffer.getReadPointer(0),
                             buffer.getNumChannels() > 1
                                 ? buffer.getReadPointer(1) : nullptr,
                             numSamples);
}
```

### Step 2: Add ControlRateReducer to Coupling

Replace audio-rate coupling with control-rate:

```cpp
#include "../../DSP/SRO/ControlRateReducer.h"

// In engine class private section:
ControlRateReducer<32> couplingReducer;  // 32:1 decimation

void applyCouplingInput(CouplingType type, float amount,
                        const float* sourceBuffer, int numSamples) override
{
    // For slow modulation types, reduce to control rate
    if (type == CouplingType::LFOToPitch
     || type == CouplingType::EnvToMorph
     || type == CouplingType::AmpToFilter
     || type == CouplingType::EnvToDecay
     || type == CouplingType::RhythmToBlend)
    {
        // Decimate source and store for use in renderBlock
        couplingReducer.processBlock(sourceBuffer, couplingBuffer.data(), numSamples);
        // ... apply modulation from couplingBuffer ...
    }
    else
    {
        // Audio-rate types (AudioToFM, AudioToRing) stay at full rate
        // ... existing per-sample coupling code ...
    }
}
```

### Step 3: Replace Expensive Math

Find and replace per the decision tree in Phase 1, Step 4.

**Before:**
```cpp
float freq = 440.0f * std::pow(2.0f, (note - 69.0f) / 12.0f);
```

**After (FastMath):**
```cpp
float freq = midiToFreq(note);  // uses fastPow2 internally
```

**Before (per-voice, called 8×/sample):**
```cpp
float shaped = std::tanh(input * drive);
```

**After (LUT — better for hot inner loops):**
```cpp
float shaped = SROTables::tanh().lookup(input * drive);
```

### Step 4: Centralize Duplicate DSP

If the engine has a local filter:

**Before:**
```cpp
// Local one-pole filter in engine header
float lpState = 0.0f;
float processLP(float in, float coeff) {
    lpState += coeff * (in - lpState);
    return lpState;
}
```

**After:**
```cpp
// Use CytomicSVF from kernel
CytomicSVF filter;
// In prepare(): filter.setCoefficients(cutoff, 0.0f, sampleRate);
// In render(): float out = filter.processSample(input);
```

---

## Phase 3: Full Fleet Audit (`/sro-optimizer audit`)

Run across all 34 engines to produce a fleet-wide efficiency report:

### Procedure

1. For each engine in `Source/Engines/*/`:
   - Count `std::sin`, `std::cos`, `std::tan`, `std::exp`, `std::pow`, `std::tanh` calls in renderBlock path
   - Check for SilenceGate presence
   - Check for local filter/saturation implementations
   - Classify coupling as STUB / AUDIO-RATE / CONTROL-RATE

2. Produce fleet summary:

```
=== SRO FLEET AUDIT ===
Date: [date]
Engines scanned: [count]

ZERO-IDLE STATUS
  Integrated:  [count] engines
  Missing:     [count] engines — [list names]

MATH HOTSPOTS (engines with >4 std:: transcendentals per sample)
  [engine]: [count] calls — [recommendations]
  [engine]: [count] calls — [recommendations]

KERNEL REUSE VIOLATIONS (engines with local duplicates)
  [engine]: local [component] — should use [kernel component]

COUPLING EFFICIENCY
  Control-rate:  [count] engines
  Audio-rate:    [count] engines (candidates for ControlRateReducer)
  Stub:          [count] engines

TOP 3 OPTIMIZATION TARGETS (highest CPU savings potential):
  1. [engine] — [reason] — est. [X]% savings
  2. [engine] — [reason] — est. [X]% savings
  3. [engine] — [reason] — est. [X]% savings
```

---

## The "Earn Your Place" Protocol for Oscillators

Every oscillator class in XOmnibus should pass this checklist:

| Check | Requirement | Why |
|-------|-------------|-----|
| Phase increment | Use `double` phase, `float` output | Prevents audible phase drift on long notes |
| Anti-aliasing | PolyBLEP or bandlimited wavetable | No naive saw/square above 2kHz fundamental |
| Frequency calc | Use `midiToFreq()` from FastMath.h | Avoids per-note `std::pow` |
| Waveform math | `fastSin()` or LUT for sine | `std::sin` is 5–10× slower |
| Denormal safety | `flushDenormal()` on feedback paths | Prevents CPU spikes on silence |
| Morph/blend | `lerp()` from FastMath.h | Consistent, branch-free blending |
| State reset | Explicit `reset()` method | Prevents clicks on voice steal |
| Voice gating | Check `isActive()` before processing | Skip idle voices entirely |

**Example: Lean Oscillator skeleton**

```cpp
class LeanOsc
{
public:
    void prepare(double sampleRate) noexcept { sr = sampleRate; }

    void setFrequency(int midiNote) noexcept
    {
        // FastMath — no std::pow
        phaseInc = static_cast<double>(midiToFreq(midiNote)) / sr;
    }

    float process(float morph) noexcept
    {
        float t = static_cast<float>(phase);

        // LUT sine — 5× faster than std::sin in hot loops
        float sine = SROTables::sin().lookup(t * 6.2831853f);

        // PolyBLEP saw — anti-aliased
        float saw = 2.0f * t - 1.0f;
        // (add polyBLEP correction here)

        float out = lerp(sine, saw, morph);

        // Advance phase (double precision prevents drift)
        phase += phaseInc;
        if (phase >= 1.0) phase -= 1.0;

        return out;
    }

    void reset() noexcept { phase = 0.0; }

private:
    double sr = 44100.0;
    double phase = 0.0;
    double phaseInc = 0.0;
};
```

---

## Quality Gate

Before marking an SRO integration as complete:

- [ ] SilenceGate bypasses renderBlock when output < -90 dB for 100ms
- [ ] SilenceGate wakes on note-on and preset change
- [ ] No `std::sin/cos/tan/exp/pow/tanh` in per-sample inner loops
- [ ] All filters use CytomicSVF (or justified engine-specific implementation)
- [ ] No heap allocation in renderBlock (no `new`, `malloc`, `push_back`, `resize`)
- [ ] Coupling modulation routes use ControlRateReducer where applicable
- [ ] EngineProfiler ScopedMeasurement wraps renderBlock
- [ ] Build passes (no new warnings)
- [ ] Audio output is bit-identical or perceptually identical to pre-optimization

---

## Related Resources

- `Source/DSP/FastMath.h` — Fast approximation library (the foundation)
- `Source/DSP/EngineProfiler.h` — CPU measurement with RAII helper
- `Source/DSP/CytomicSVF.h` — Shared TPT SVF filter
- `Source/DSP/Effects/Saturator.h` — Shared saturation/waveshaping
- `Skills/engine-health-check/SKILL.md` — Doctrine compliance (complements SRO)
- `Docs/xomnibus_master_specification.md` — Architecture rules
