# Skill: /sro-optimizer

**Invoke with:** `/sro-optimizer [engine-name | audit | integrate]`
**Status:** LIVE
**Last Updated:** 2026-03-20 | **Version:** 1.0 | **Next Review:** On new engine addition or platform target change
**Purpose:** Sustainability & Resource Optimization — make constraint the accelerator that drives creative DSP solutions, ensures the platform stays accessible to every user's machine, and keeps XOceanus sustainable for the long haul.

---

## Core Ethos: Constraint as Creative Catalyst

> *"The enemy of art is the absence of limitations."* — Orson Welles

Every breakthrough instrument in history was born from constraint — and the constraint became inseparable from the music.

**The Minimoog (1970)** — Bob Moog could only fit 3 oscillators, 1 filter, and 2 envelopes into a portable case. He couldn't compete with the room-sized Moog modular on features, so he competed on *immediacy* — hardwired signal flow, no patch cables needed, performance-ready in seconds. That constraint defined the sound of progressive rock, funk bass, and lead synthesis for a decade. Three oscillators was enough because the signal path was *curated*.

**The Yamaha DX7 (1983)** — John Chowning's FM synthesis was computationally cheap enough to run on a single chip. Yamaha chose 6 operators not from luxury but from silicon budget. Those 6 operators and 32 algorithms became the most commercially successful synthesizer in history. The limitation forced musicians into harmonic structures no one would have explored with 32 operators — the electric piano, the bell, the bass slap, all emerged from the constraint of "only 6."

**The Roland TB-303 (1981)** — Designed to replace a bass guitarist. Failed completely at that job — the sequencer was too rigid, the filter too squelchy, the oscillator too thin. It was a commercial disaster. Then DJ Pierre accidentally ran it through distortion, and acid house was born. The "flaws" — the accent circuit's filter sweep, the slide's portamento glitch, the resonance's self-oscillation — were the sound. *Every flaw was a feature waiting for the right context.*

**The Akai MPC60 (1988)** — Roger Linn couldn't afford the Fairlight CMI's $25,000 sampling system. So he built the MPC60 with 12-bit samples, 750KB RAM, and finger-drummed pads instead of a keyboard. The low bit depth gave samples grit. The pads made rhythm a physical, expressive act. The RAM constraint forced users to chop, truncate, and loop — inventing the production techniques of hip-hop and electronic music. *The MPC didn't overcome its constraints. It performed them.*

**The Buchla 259 (1970s)** — Don Buchla's Complex Waveform Generator put two oscillators in one module and had them modulate each other bidirectionally — not because he wanted complexity for its own sake, but because panel space was expensive and he needed one module to do the work of three. Bidirectional FM emerged from the constraint of *physical real estate on an aluminum panel*.

**Beyond synthesis — constraint as creative method:**

- **Toyota Production System (1950s)** — Post-war Japan had no capital for inventory stockpiles. Taiichi Ohno invented just-in-time manufacturing: produce only what's needed, when it's needed. The constraint of poverty became the most influential manufacturing philosophy in history. Our zero-idle policy is the same principle — don't compute what nobody's listening to.
- **Unix philosophy (1970s)** — Ken Thompson and Dennis Ritchie had a PDP-7 with 8KB of RAM. They couldn't build a monolithic OS, so they built small tools that composed via pipes. "Do one thing well" wasn't ideology — it was necessity. Our shared kernel philosophy (CytomicSVF, FastMath, Saturator) is the Unix pipe for DSP: small, composable, shared.
- **LEGO (1958)** — Godtfred Kirk Christiansen constrained the entire product line to one interlocking system with one stud size. Infinite complexity from finite, shared components. Our DSP kernel is the stud — CytomicSVF is the 2×4 brick. Every engine clicks into the same foundation, and the constraint is what makes the combinations possible.
- **Haiku (8th century)** — 5-7-5 syllables. The constraint forces the poet to distill an entire scene into seventeen sounds. The result is compression so extreme it becomes its own form of beauty. A lean oscillator class is a haiku — every line present because removing it would break the poem.

XOceanus is free, open-source, and runs on everything from a 2018 MacBook Air to a maxed M-series studio rig. That's not a constraint to fight — it's the design space we *chose*. When we say "earn your place," we're not policing CPU. We're asking the same question every great instrument designer asks: **"What is the most musical thing I can do with what I have?"**

The SRO Framework exists because:
- A user running 4 engines on a modest laptop deserves the same creative experience as someone on a $10K workstation
- When you can't afford to waste cycles, you find the *cheaper path that sounds better* — and that's where the happy accidents live
- `fastTanh` doesn't sound identical to `std::tanh`. It sounds *different*. And sometimes that difference *is* the sound — just like tape saturation, just like 12-bit sampling, just like the 303's accent circuit
- A shared kernel means one inspired optimization lifts all 34 engines simultaneously — the LEGO principle, the Unix principle, compounding returns for the entire community

### The Five Principles

| Principle | Spirit | Precedent |
|-----------|--------|-----------|
| **Constraint Breeds Character** | When you can't brute-force it, you design around it — and the detour often sounds better than the highway. A polynomial tanh approximation has its own saturation curve. A control-rate LFO with interpolation has its own smoothness. These aren't compromises — they're timbral signatures. | The TB-303's "flawed" filter accent became the defining sound of acid. The DX7's "limited" 6 operators created an entire genre of timbres. |
| **Zero-Idle = Room for More** | When silent engines consume nothing, the user gets headroom to load a 4th engine, add more coupling, push the effects chain harder. Efficiency *expands* creative possibility. | Toyota's kanban: empty shelves aren't waste — they're capacity waiting for the next order. A sleeping engine slot is an invitation. |
| **Upcycling > Reinventing** | A shared CytomicSVF filter, battle-tested across 34 engines, is more reliable and more optimized than 34 local filters written at different time. Reuse is a creative act — it frees the engine designer to focus on what makes *their* engine unique. | LEGO's single stud system. Unix pipes. Eurorack's standardized power/signal format — Doepfer's A-100 bus freed 500+ manufacturers to focus on what their module *does*, not how it connects. |
| **Control Rate = Creative Smoothing** | Running modulation at 1/32 rate with interpolation isn't just cheaper — it naturally smooths zipper artifacts. The constraint *solves a problem you'd have to solve anyway*. | Pixar's RenderMan shading rate: surfaces further from camera get fewer shader evaluations. The variable rate is imperceptible and saves 60% of render time. Same principle, different domain. |
| **Sustainability is Love** | This project is built by passion, maintained by community, and given away free. Every CPU cycle we save is a gift to every user who loads XOceanus on their machine. That's the ROI that matters. | The Volca series — Korg proved that $150 synths with real analog circuits could exist by ruthlessly constraining the feature set. Accessibility *is* the feature. |

### Historical Precedents in XOceanus

Constraint has already driven innovation here:

- **FastMath.h** — Schraudolph's `fastExp` uses IEEE 754 bit manipulation instead of Taylor series. It's ~4% off from `std::exp`. That 4% gives envelope curves a subtly different character — and runs 10× faster. Nobody asked for a "slightly warmer exponential." Constraint delivered one.
- **PolyBLEP anti-aliasing** — A 2nd-order polynomial correction instead of oversampled bandlimiting. Costs almost nothing. Sounds different from a wavetable saw — that difference is part of what makes each engine's oscillator voice distinct.
- **CytomicSVF `setCoefficients_fast()`** — Uses `fastTan` instead of `std::tan` for modulated cutoff. The approximation's slight deviation at high frequencies gives the filter a character that pristine math doesn't. This was a performance optimization that became a sonic feature.
- **50ms crossfade on engine hot-swap** — Born from the constraint "no clicks on slot change." The crossfade became a design element — morphing between engines feels organic, not switched.

---

## When to Use This Skill

Use this skill when:
- **Designing** a new engine and want to find the most creative efficient path from the start
- **Auditing** an existing engine to free up CPU headroom for the user
- **Integrating** SRO components (SilenceGate, ControlRateReducer, LUTs) into any engine
- **Profiling** a 4-slot configuration and looking for room to breathe
- **Exploring** whether a constraint-driven alternative might sound *better* than the brute-force approach
- **Upcycling** local DSP into the shared kernel so all engines benefit
- Preparing for release and ensuring the widest possible hardware compatibility

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

**Decision tree** (not rigid — always A/B the result):
- If called < 1× per sample (e.g., once per block for coefficient calc): **keep `std::`** — precision matters here, and the cost is amortized
- If called 1× per sample: **try FastMath** — listen to the result; if the approximation adds character you like, that's a happy accident, keep it
- If called N× per sample (per-voice, per-oscillator): **use LUT** — this is where the real savings live; the table interpolation also acts as subtle smoothing, which often helps
- If replacing changes the sound in a way you *prefer*: **document it and keep the new version** — constraint just made your engine better

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

**The crucial distinction:** Engine-specific DSP that is genuinely unique is NOT a reuse violation — it's the whole point. Organon's metabolic model, Oracle's GENDY, Ouroboros's strange attractor, Owlfish's Mixtur-Trautonium oscillator — these are the *reason* each engine exists. They earned their place by being irreplaceable.

Only flag components that duplicate existing kernel functionality (filters, envelopes, saturation, noise generators). The goal of upcycling is to free engine designers from re-solving solved problems so they can focus entirely on what makes their engine *theirs*.

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

Wire into renderBlock (**ordering is critical** — Vangelis: *"A performer who strikes a key and hears silence for 5ms has lost trust in the instrument forever"*):

```cpp
void renderBlock(juce::AudioBuffer<float>& buffer,
                 juce::MidiBuffer& midi, int numSamples) override
{
    // STEP 1: ALWAYS parse MIDI first — even if bypassed.
    // This is non-negotiable. The gate must wake before the bypass check.
    for (const auto metadata : midi)
    {
        auto msg = metadata.getMessage();
        if (msg.isNoteOn())
            silenceGate.wake();
    }

    // STEP 2: THEN check bypass.
    if (silenceGate.isBypassed() && midi.isEmpty())
    {
        buffer.clear();
        return;
    }

    // STEP 3: existing render code ...

    // STEP 4: analyze output for silence detection (AFTER rendering).
    silenceGate.analyzeBlock(buffer.getReadPointer(0),
                             buffer.getNumChannels() > 1
                                 ? buffer.getReadPointer(1) : nullptr,
                             numSamples);
}
```

**Set hold time per engine category** — the default 100ms suits percussive engines, but reverb-tail engines need more:

| Category | Hold Time | Engines | Why |
|----------|-----------|---------|-----|
| Percussive | 100ms (default) | ONSET, OVERBITE, ODDFELIX, OBSIDIAN | Fast decay — silence means silence |
| Standard | 200ms | OBLONG, ODYSSEY, OBLIQUE, ODDOSCAR | Moderate release tails |
| Reverb-tail | 500ms | OVERDUB (spring reverb), OPAL (granular smear), OCEANIC (triple-BBD) | Long reverb tails must ring out naturally |
| Infinite-sustain | 1000ms | ORGANON (metabolic), OUROBOROS (attractor), ORBITAL (group envelopes) | These engines evolve over minutes — premature bypass interrupts the evolution |

```cpp
// In prepare():
silenceGate.setHoldTime(500.0f);  // reverb-tail engine — 500ms hold
```

**Disabling SilenceGate per engine** — Some engines intentionally use feedback loops, near-silence textures, or sub-threshold signals as part of their design. Industrial/noise workflows exploit the boundary between silence and sound. For these engines, SilenceGate should be disabled or configured with a much lower threshold:

```cpp
// Option 1: Disable entirely for feedback-heavy engines
// Simply don't integrate SilenceGate — the engine always processes.

// Option 2: Lower the threshold for engines that live near silence
silenceGate.setThreshold(-120.0f);  // effectively disabled but still measuring

// Option 3: Conditional — disable when feedback coupling is active
if (hasFeedbackCoupling)
    silenceGate.wake();  // keep it awake while feedback is routed
```

Engines where SilenceGate should be disabled or relaxed: OUROBOROS (strange attractor feedback), ORGANON (metabolic processes that idle at sub-threshold), any engine in an AudioToFM or AudioToRing self-feedback coupling route.

**Future: Standby mode** (Tomita's recommendation) — Between "fully active" and "bypassed," a standby state would maintain filter state, phase positions, and envelope levels but skip the inner voice loop. Cost: ~5% of full active. Benefit: seamless re-entry without filter ramp-up transients or phase discontinuity. Critical for orchestral scoring where an engine may be silent for 8 bars then enter with a pianissimo swell. *A conductor holds silence. The instrument must hold readiness.*

### Step 2: Add ControlRateReducer to Coupling

The ControlRateReducer at 1/32 rate with linear interpolation is functionally a **sample-and-hold module with built-in slew limiting** — a topology Buchla called "source of uncertainty" when he implemented it in the Model 266. The smoothing between control points is not a compromise; it is a modulation character. Analog synthesizers achieved this naturally through CV line capacitance. In digital, we choose it deliberately.

**Expression protection — signals that must NEVER be reduced:**

Velocity, note-on, aftertouch, and mod wheel are performer gestures. They must remain sample-accurate. A jazz pianist striking a chord expects instantaneous timbral response from velocity. A cellist leaning into aftertouch expects continuous, immediate pressure mapping. The ControlRateReducer is for *slow modulation sources* (LFOs, envelopes, rhythmic patterns) — never for expression signals.

| Signal Type | Rate | Why |
|-------------|------|-----|
| Velocity (note-on) | **Audio-rate (instant)** | Performer expects immediate timbral response — D001 non-negotiable |
| Aftertouch / Channel Pressure | **Audio-rate** | Continuous expression must not be quantized — D006 non-negotiable |
| Mod Wheel (CC1) | **Audio-rate** | Real-time performance control — D006 non-negotiable |
| LFO → Pitch | Control-rate (1/32) | Slow modulation — interpolation adds natural slew |
| Envelope → Morph | Control-rate (1/32) | Slow modulation — interpolation is imperceptible |
| Amp → Filter (coupling) | Control-rate (1/32) | Cross-engine modulation — smooth is better |
| Audio → FM (coupling) | **Audio-rate** | Timbral FM requires sample-accurate carriers |
| Audio → Ring (coupling) | **Audio-rate** | Ring modulation IS the audio-rate interaction |

Replace audio-rate coupling with control-rate (for slow types only):

```cpp
#include "../../DSP/SRO/ControlRateReducer.h"

// In engine class private section:
ControlRateReducer<32> couplingReducer;  // 32:1 decimation

void applyCouplingInput(CouplingType type, float amount,
                        const float* sourceBuffer, int numSamples) override
{
    // EXPRESSION SIGNALS: velocity, aftertouch, mod wheel — ALWAYS audio-rate.
    // These are performer gestures. Never reduce them.

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
        // Audio-rate types (AudioToFM, AudioToRing) stay at full rate.
        // Also: velocity, aftertouch, mod wheel — never reduced.
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

The question isn't "how cheap can we make this?" — it's "what's the most musical solution at the lowest cost?" Every constraint below exists because someone found that the efficient path *also* sounded better, or freed up budget for something that mattered more.

| Check | Requirement | The Creative Reason |
|-------|-------------|---------------------|
| Phase increment | Use `double` phase, `float` output | `double` phase prevents the slow pitch drift that makes long pads go sour — precision *is* musicality here |
| Anti-aliasing | PolyBLEP or bandlimited wavetable | PolyBLEP costs almost nothing and each implementation has its own subtle character. Aliasing isn't "raw" — it's just broken |
| Frequency calc | `midiToFreq()` from FastMath.h | One shared function, optimized once. Frees you from thinking about pitch math — go design the *timbre* |
| Waveform math | `fastSin()` or LUT for sine | The LUT sine has a micro-staircase at extreme zoom. In a mix, that's warmth. In a solo, it's transparent. Either way it's 5× faster |
| Denormal safety | `flushDenormal()` on feedback | Denormals cause CPU spikes on *silence* — the opposite of what silence should cost. This is the zero-idle philosophy at the sample level |
| Morph/blend | `lerp()` from FastMath.h | One function. Branch-free. Every engine morphs the same way, so users build intuition that transfers across engines |
| State reset | Explicit `reset()` method | Clean restarts prevent clicks on voice steal. The constraint of resettable state also makes engines testable in isolation — constraint breeds quality |
| Voice gating | Check `isActive()` before processing | An 8-voice engine where only 3 voices sound should cost 3/8 of full. The saved 5/8 is headroom for coupling, effects, or another engine slot |

**Example: Lean Oscillator skeleton**

This is the minimum viable oscillator that passes "Earn Your Place." It's also a
perfectly good starting point — the constraint of this skeleton has produced
oscillators with genuine character across the fleet.

```cpp
class LeanOsc
{
public:
    void prepare(double sampleRate) noexcept { sr = sampleRate; }

    void setFrequency(int midiNote) noexcept
    {
        // FastMath — one function, no std::pow, shared across all engines.
        // The ~0.1% pitch deviation from fastPow2 is inaudible.
        phaseInc = static_cast<double>(midiToFreq(midiNote)) / sr;
    }

    float process(float morph) noexcept
    {
        float t = static_cast<float>(phase);

        // LUT sine — the 4096-point table has its own micro-character.
        // In A/B tests against std::sin, users preferred "whichever was
        // already loaded." That means it's good enough. And 5× faster.
        float sine = SROTables::sin().lookup(t * 6.2831853f);

        // PolyBLEP saw — 2nd-order polynomial anti-aliasing.
        // Costs ~2 multiplies. Sounds different from a wavetable saw.
        // That difference is part of this oscillator's identity.
        float saw = 2.0f * t - 1.0f;
        // (add polyBLEP correction here)

        float out = lerp(sine, saw, morph);

        // Double-precision phase — no drift on 10-minute drones
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

### When Constraint Becomes Discovery

Watch for these moments during engine development — they're the happy accidents. Document them. They're the most valuable output of the optimization process.

- **"The LUT version sounds warmer"** — The interpolation between table entries acts as a subtle lowpass. Sometimes this is exactly what a pad oscillator needs. Don't fix it. PPG's wavetable synthesis had a similar accident — the crossfade between 64-sample frames was a *design flaw* that became the signature Wolfgang Palm sound. Linear interpolation between fixed points has its own sonic fingerprint.

- **"The control-rate modulation feels smoother"** — Running an LFO at 1/32 rate with lerp naturally filters out high-frequency jitter. The cheap path and the musical path were the same path. This mirrors how analog synths naturally slew-limited their control voltages — the capacitance in the CV lines was a "defect" that made modulation feel organic.

- **"The fast filter has a different resonance peak"** — `fastTan` prewarping deviates slightly from `std::tan` near Nyquist. Some engines sound better with the approximation. If it does, keep it and document it. The Moog ladder filter's famous resonance character comes partly from component tolerances — imprecision *is* the sound.

- **"Sharing the kernel forced me to simplify"** — When you can't write a custom filter, you design the oscillator to need less filtering. Simpler signal paths often sound more direct and present. Brian Eno's Oblique Strategies card: *"What would you do if this were simple?"* Sometimes the kernel constraint answers that question for you.

- **"Voice gating created an accidental rhythmic effect"** — Voices that cleanly stop create tighter transients than voices that fade to inaudible noise. Sometimes the "efficient" behavior is the more musical behavior. The TR-808's instant-off trigger circuits were a cost-saving measure that defined the rhythmic precision of an entire genre.

- **"I couldn't afford oversampling, so I redesigned the waveshaper"** — A gentler saturation curve that doesn't alias at 1× is often more useful than an aggressive one that needs 4× oversampling. Curtis Roads documented this in *The Computer Music Tutorial*: "The best anti-aliasing strategy is to not alias in the first place." Constraint in the output stage drove innovation in the input stage.

---

## Quality Gate

Before marking an SRO integration as complete, verify these. They're not bureaucracy — each one exists because skipping it burned someone in a real session.

- [ ] **SilenceGate bypasses renderBlock** when output < -90 dB for 100ms — *this is the gift of headroom to every user*
- [ ] **SilenceGate wakes on note-on** and preset change — *never steal sound from the player*
- [ ] **No `std::sin/cos/tan/exp/pow/tanh` in per-sample inner loops** — *use FastMath or LUTs; if the approximation sounds different, A/B it and decide musically*
- [ ] **All filters use CytomicSVF** (or a justified engine-specific implementation with documented reasoning) — *"justified" means it does something CytomicSVF literally cannot*
- [ ] **No heap allocation in renderBlock** (no `new`, `malloc`, `push_back`, `resize`) — *this isn't a style preference; it causes audio glitches on real hardware*
- [ ] **Coupling modulation routes use ControlRateReducer** where applicable — *slow signals don't need audio-rate precision; the interpolation is a feature*
- [ ] **EngineProfiler ScopedMeasurement wraps renderBlock** — *you can't improve what you don't measure*
- [ ] **Build passes** (no new warnings)
- [ ] **Factory presets are audibly identical** after optimization — *shipped presets approved by producers and sound designers must not change. This is non-negotiable for session recall, scoring consistency, and user trust. A/B test with null-difference analysis where possible.*
- [ ] **New engine development: perceptual equivalence is acceptable** — *during development (before presets ship), if optimization changes the sound, A/B test it. If the new version is better, keep it and document the change. But once a preset ships, its sound is frozen.*
- [ ] **The constraint was explored, not just applied** — *did we find anything surprising? A happy accident? A cheaper path that sounded better? Log it in the commit message or engine comments. These discoveries compound.*

---

## The Sustainability Commitment

XOceanus is free. It will stay free. It runs on hardware people already own. That's a promise to the community, and SRO is how we keep it.

Every cycle we save is:
- A user on a 2018 laptop who can load 4 engines instead of 3
- A live performer whose set doesn't glitch during the bridge
- A bedroom producer who doesn't need to bounce stems to free up CPU
- A student who can run XOceanus on a school computer
- Another year this project stays viable without monetization pressure

The Teenage Engineering OP-1 runs an entire production studio on a battery-powered ARM chip. Dirtywave's M8 runs a tracker on a Teensy microcontroller. Monome's norns runs SuperCollider on a Raspberry Pi. These aren't compromised instruments — they're *beloved* instruments, precisely because their designers embraced constraint as identity rather than fighting it as limitation.

XOceanus has 34 engines, a coupling matrix, a sequencer, an effects chain, and 2,550 presets. It runs on hardware from 2018. The only way that's possible — the only way it *stays* possible as we add engines 35, 36, 37 — is if every component earns its place, every shared module lifts the whole fleet, and every optimization is explored for its creative potential before it's filed as "just" an efficiency gain.

Constraint isn't the enemy. Constraint is the instrument.

---

## Phase 2 Roadmap: Architecture Extensions

These decisions were made during the SRO framework review (2026-03-17) and are logged here for implementation when the current Phase 1 (audit + integrate) work stabilizes.

### 2A. Dynamic Oversampling Manager (Graceful Degradation)

**Status:** Approved for early Phase 2
**Concept:** A new SRO component that dynamically reduces oversampling (4×→2×→1×) based on CPU budget pressure.

The SROAuditor already tracks budget via `budgetAlarm`. When the alarm fires, the OversamplingManager could step down the oversampling rate for the most CPU-expensive slot first. When budget pressure eases, step back up. This is Graceful Degradation — the system maintains sonic quality as long as it can afford to, and sacrifices intelligently when it can't.

**Interface sketch:**
```cpp
class OversamplingManager
{
public:
    void prepare(double sampleRate, int maxBlockSize) noexcept;
    int getCurrentFactor(int slot) const noexcept;  // returns 1, 2, or 4
    void update(const SROAuditor::Report& report) noexcept;  // call once per block
};
```

**Creative angle:** Variable oversampling is analogous to how tape machines behave — at higher speeds (15 ips, 30 ips) you get more bandwidth; at lower speeds you get warmth and compression. The dynamic toggle isn't a quality loss — it's a timbral shift. Document the sonic difference at each rate so users (and preset designers) can hear the trade-off.

### 2B. Prime Mover Engines — Send-Only Slot Class

**Status:** Approved architecture
**Concept:** A new engine class that drives a 4-engine effects chain. Prime Movers generate audio (from MIDI, from internal sources, from audio input) and send it into the coupling matrix, but **never receive coupling input**. They are the source — the prime mover of the signal chain.

**Key design decisions:**
- `MaxSlots` remains 4 for generator engines. The Prime Mover occupies a **separate, dedicated slot** — not a 5th generic slot.
- Coupling is **send-only**: Prime Mover → Slot 0-3. No reverse path. This keeps the coupling matrix at 4×3 routes (not 5×4), avoiding a 67% complexity increase.
- Prime Movers implement `SynthEngine` with an additional trait/flag indicating send-only status.
- SROAuditor monitors the Prime Mover slot separately (not counted in the 4-slot budget).
- SilenceGate applies normally to Prime Movers — if the source is silent, the whole chain can sleep.

**Use case:** User picks an fXO_ effect chain (4 slots of effects). The Prime Mover generates the source audio that feeds through them. This enables XOceanus to function as a full effects processor, not just a generator.

**Interface addition to SynthEngine:**
```cpp
// In SynthEngine.h — optional override:
virtual bool isPrimeMover() const noexcept { return false; }
```

### 2C. fXO_ Effects Ecosystem — Shared-Core Architecture

**Status:** Approved approach — shared cores first, regional presets second
**Naming convention:** `fXO_` prefix + O-word (e.g., `fXO_Obsession`, `fXO_Orbit`, `fXO_Overtone`)

The `fXO_` prefix maintains XO brand DNA while clearly differentiating effects from generator engines (`XO_` prefix). The `f` signals "effect" to both users and code.

#### Shared Effect Cores (Build These First)

Instead of 48 individual fX modules, build **6-8 shared effect cores**. Each core is a configurable DSP engine. The "48 effects" become **preset configurations** of these cores with regional naming and character.

| Core | DSP Foundation | What It Covers |
|------|---------------|----------------|
| `ReverbCore` | Feedback delay network + diffusion | Room, hall, plate, spring, shimmer |
| `DelayCore` | Multi-tap delay + modulation | Tape echo, ping-pong, granular delay, rhythmic |
| `SaturationCore` | Waveshaping + multiband | Tube, tape, transistor, bitcrush, fold |
| `ModulationCore` | LFO → delay line | Chorus, flanger, phaser, vibrato, rotary |
| `FilterCore` | CytomicSVF chains + resonators | Formant, comb, vocal, tilt EQ, wah |
| `SpatialCore` | Stereo manipulation + panning | Width, rotation, mid-side, Haas, binaural |
| `PitchCore` | Pitch detection + shifting | Harmonizer, octaver, detune, glitch |
| `DynamicsCore` | Envelope follower + gain | Compressor, gate, expander, ducker, sidechain |

This is the LEGO principle applied to effects: 8 bricks, infinite combinations. Each core exposes a parameter set that regional presets configure differently. A "Midwest" reverb and a "Nordic" reverb are both `ReverbCore` with different diffusion, decay, and tonal character presets.

#### Regional Consolidation

Instead of 13 regions × 4 fX = 48 separate modules, each region becomes a **single fXO_ engine** that configures the shared cores into a curated effects chain.

| Region | fXO_ Engine | Character | Cores Used |
|--------|------------|-----------|------------|
| Midwest | `fXO_Outpost` | Wide open, tape warmth, AM radio grit | SaturationCore + DelayCore + ReverbCore |
| Nordic | `fXO_Overcast` | Glacial reverb, pristine delay, subtle modulation | ReverbCore + DelayCore + ModulationCore |
| Tokyo | `fXO_Origami` | Glitch, bitcrush, precision delay | SaturationCore + DelayCore + PitchCore |
| Saharan | `fXO_Oasis` | Desert reverb, sand-filtered resonance | ReverbCore + FilterCore + SpatialCore |
| Coastal | `fXO_Offshore` | Wave-shaped modulation, sea-spray diffusion | ModulationCore + ReverbCore + SpatialCore |
| ... | ... | ... | ... |

Each regional fXO_ engine is a thin adapter that instantiates 2-3 shared cores and routes them in a specific topology. The adapter is ~200 lines. The shared cores do the heavy lifting.

**SRO alignment:** This is the Unix philosophy — small composable tools. 8 cores × regional routing = unlimited character with minimal code. Every optimization to `ReverbCore` improves every regional engine that uses it. One fix, 13 regions better.

#### fXO_ Integration with SynthEngine

Effects engines extend `SynthEngine` with an optional audio input bus (Path B — approved):

```cpp
// In SynthEngine.h — optional override for effects:
virtual bool isEffect() const noexcept { return false; }

// Effects override renderBlock to read from an input bus:
// void renderBlock(AudioBuffer& buffer, MidiBuffer& midi, int numSamples) override
// {
//     // buffer arrives pre-filled with input audio (from Prime Mover or previous slot)
//     // Apply effect processing in-place
//     // SilenceGate, ControlRateReducer, etc. work identically
// }
```

### 2D. Prime Mover Engine Candidates

A reasonable initial set of Prime Mover engines (source generators for effects chains):

| Engine | Concept | Why It's a Prime Mover |
|--------|---------|----------------------|
| `XO_Origin` | Clean multi-osc generator (saw/square/sine/noise) | Neutral source — lets the fXO_ chain define character |
| `XO_Overture` | Sample/loop player | Brings any audio into the fXO_ chain |
| `XO_Oscillograph` | Wavetable + FM source | Rich harmonic starting material |
| `XO_Ouverture` | Mic/line input pass-through | Makes XOceanus a live effects processor |

These are **not** creative engines like OPAL or ORGANON — they're transparent sources designed to feed effects chains. Their job is to get out of the way and let the fXO_ engines shine.

---

## Architectural Decisions Log

| Date | Decision | Rationale |
|------|----------|-----------|
| 2026-03-17 | `MaxSlots` stays at 4 for generators; Prime Mover is a separate slot | Avoids 67% coupling complexity increase (4×3 → 5×4 routes) |
| 2026-03-17 | Effects extend `SynthEngine` with optional audio input (Path B) | Organic — one interface, one system. Effects are engines that process input instead of generating from MIDI |
| 2026-03-17 | No `XO_Kernel` abstraction class | SRO components composed at call site ARE the kernel. 6 lines in each adapter. Adding a class adds indirection without benefit |
| 2026-03-17 | `fXO_` naming prefix for effects | Maintains XO brand DNA, clear differentiator from `XO_` generators |
| 2026-03-17 | 6-8 shared effect cores, not 48 individual modules | LEGO/Unix principle — composable cores, regional presets. One optimization lifts all regions |
| 2026-03-17 | Regional engines = single fXO_ adapter per region | Consolidates quad-per-region to one engine configuring 2-3 shared cores |
| 2026-03-17 | Dynamic oversampling → early Phase 2 | SROAuditor budget alarm is the trigger; needs OversamplingManager component |
| 2026-03-17 | CodeShark / Synesthesia → separate workstreams | Not DSP architecture — engagement/visualization features, orthogonal to SRO |

---

## Related Resources

- `Source/DSP/FastMath.h` — Fast approximation library (the foundation — and a gallery of creative constraint)
- `Source/DSP/EngineProfiler.h` — CPU measurement with RAII helper
- `Source/DSP/CytomicSVF.h` — Shared TPT SVF filter (one great filter, 34 engines)
- `Source/DSP/Effects/Saturator.h` — Shared saturation/waveshaping
- `Skills/engine-health-check/SKILL.md` — Doctrine compliance (complements SRO)
- `Docs/xoceanus_master_specification.md` — Architecture rules
