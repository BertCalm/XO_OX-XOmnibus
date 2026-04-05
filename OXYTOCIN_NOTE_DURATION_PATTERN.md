# Oxytocin B040: Note Duration as Synthesis Parameter

## Overview
B040 (Blessing 40) implements **note duration as a first-class timbral parameter** — long notes unlock synthesis states (warmth, bond depth, intimacy accumulation) unavailable to short notes.

The pattern uses three mechanisms:
1. **Per-voice stage-based state machine** (LoveEnvelope) with distinct attack/decay/sustain/release shapes
2. **Block-rate coefficient caching** (exp() and linear ramps computed once per block, not per sample)
3. **Leaky integrator accumulators** (three-stage NTC thermal model with signal-dependent acceleration)

---

## 1. Per-Voice State Tracking

### Voice Struct (OxytocinVoice.h, lines 181–517)
```cpp
class OxytocinVoice
{
private:
    bool active = false;
    float vel = 0.8f;
    float pitchBendRatio = 1.0f;
    double sr = 0.0;
    
    // Block-average accumulators for I/P/C (lines 304–305)
    float accI = 0.0f, accP = 0.0f, accC = 0.0f;  // Per-voice accumulators
    
    // Thermal state (three-stage accumulator chain)
    OxytocinThermal thermal;
    
    // Three distinct love envelopes per voice
    LoveEnvelope passionEnv;
    LoveEnvelope intimacyEnv;
    LoveEnvelope commitmentEnv;
};
```

**Key insight:** Each voice maintains its own accumulator state (`accI`, `accP`, `accC`) that builds over the entire note duration. The engine's `processBlock()` samples these accumulators to feed the global memory system.

---

## 2. Note Duration Tracking via State Machine (LoveEnvelope)

### Stage Machine (OxytocinLoveEnvelope.h, lines 33–40)
```cpp
enum class Stage
{
    Idle,      // Note off or initial state
    Attack,    // From noteOn() until attack time expires
    Decay,     // Optional exponential decay to sustain level
    Sustain,   // Long-held state (where duration matters most)
    Release    // From noteOff() until silence
};
```

### Three Envelope Shapes with Different Time Constants

#### **Passion Envelope** (lines 154–211)
- **Attack:** Exponential rise, time = `passionRate`, scaled by velocity (`attackScale`)
- **Sustain:** Stays at 15% (`sustainLevel = 0.15f`)
- **Release:** Exponential decay
- **Purpose:** Rapid transient energy; decays quickly to allow other voices to shine

```cpp
float tickPassion(float pRate)
{
    const float attackTime = std::max(0.0001f, pRate) / std::max(0.1f, attackScale);
    const float sustainLevel = 0.15f;
    // ... state machine handles Attack/Decay/Sustain/Release
}
```

#### **Intimacy Envelope** (lines 215–268)
- **Attack:** **Sigmoid curve** (1/(1 + exp(-6*(t - 0.5)))) — smooth, warm onset
- **Sustain:** High (90%, `sustainLevel = 0.9f`)
- **Release:** Slow exponential decay (2× attack time)
- **Purpose:** Core warmth parameter; long sustain means warmth accumulates over note duration

```cpp
case Stage::Attack:
{
    phase += cachedIntimacyPhaseInc;  // Phase ramps 0→1
    float sigmoid = 1.0f / (1.0f + std::exp(-6.0f * (phase - 0.5f)));
    value = sigmoid * sustainLevel;  // Sustain = 0.9
    // ...
}
```

#### **Commitment Envelope** (lines 272–316)
- **Attack:** **Linear ramp** (accumulated increment `cachedCommitPhaseInc` per sample)
- **Sustain:** Very high (95%, `sustainLevel = 0.95f`)
- **Release:** Extremely slow (10× attack time)
- **Purpose:** Long-term bond accumulation; takes longest to build, persists longest after noteOff

```cpp
case Stage::Attack:
{
    value += cachedCommitPhaseInc;  // Linear ramp increment
    if (value >= sustainLevel)      // Sustain = 0.95
    {
        value = sustainLevel;
        stage = Stage::Sustain;
    }
}
```

---

## 3. Duration-Dependent Accumulation: The NTC Thermal Model

### Three-Stage Leaky Integrator (OxytocinThermal.h)

The **core duration-as-parameter** mechanism. The thermal system is a cascaded three-stage IIR filter chain that slowly converges toward the intimacy envelope value.

```cpp
class OxytocinThermal
{
private:
    float stage1 = 0.0f;   // First leaky integrator
    float stage2 = 0.0f;   // Second leaky integrator (output of stage1)
    float stage3 = 0.0f;   // Final stage (actual warmth parameter)
    float targetIntimacy = 0.0f;
    
    // IIR coefficients (computed at block rate, not per sample)
    float coeff1, coeff2, coeff3;
};
```

### Block-Rate Coefficient Update (lines 65–109)
Called once per block (e.g., 128 samples = 2.67 ms at 48 kHz):

```cpp
float updateWarmth(float intimacy, float warmthRate, float /*blockTime*/) noexcept
{
    if (warmthRate != lastWarmthRate)
    {
        float tau1 = std::max(0.001f, warmthRate * 0.3f);
        float tau2 = std::max(0.001f, warmthRate * 0.6f);
        float tau3 = std::max(0.001f, warmthRate * 1.0f);
        
        // exp() computed ONCE per block, not per sample
        coeff1 = std::exp(-1.0f / (static_cast<float>(sr) * tau1));
        coeff2 = std::exp(-1.0f / (static_cast<float>(sr) * tau2));
        coeff3 = std::exp(-1.0f / (static_cast<float>(sr) * tau3));
        lastWarmthRate = warmthRate;
    }
    
    // Signal-load-adjusted coefficients (NTC thermal acceleration)
    float loadMult = 1.0f + signalEnvLevel * 2.0f;  // [1.0 .. 3.0]
    if (loadMult > 1.001f)
    {
        float invLoad = 1.0f / loadMult;
        cachedC1f = std::pow(coeff1, invLoad);  // Loud signals heat faster
        cachedC2f = std::pow(coeff2, invLoad);
        cachedC3f = std::pow(coeff3, invLoad);
    }
    
    targetIntimacy = intimacy;  // From LoveEnvelope
    return stage3;  // Current warmth output
}
```

### Per-Sample Accumulation (lines 112–167)

```cpp
float processSample(float input, float circuitAge) noexcept
{
    // Signal-dependent thermal acceleration (envelope follower)
    float absIn = std::abs(input);
    if (absIn > signalEnvLevel)
        signalEnvLevel = absIn + (signalEnvLevel - absIn) * signalEnvAttackCoeff;
    else
        signalEnvLevel = signalEnvLevel * signalEnvReleaseCoeff;
    
    // Three-stage cascaded IIR filter (exponential convergence)
    stage1 = targetIntimacy + (stage1 - targetIntimacy) * cachedC1f;
    stage2 = stage1 + (stage2 - stage1) * cachedC2f;
    stage3 = stage2 + (stage3 - stage2) * cachedC3f;
    
    const float warmth = stage3;  // Final accumulated warmth [0..1]
    
    // Warmth modulates saturation depth
    const float saturated = fastTanh(input * (1.0f + warmth * 2.5f)) * 0.7f;
    const float clean = input;
    float output = clean * (1.0f - warmth * 0.4f) + saturated * (warmth * 0.4f);
    
    return output;
}
```

**Why duration matters:** The three `stage` variables accumulate incrementally toward `targetIntimacy` (which comes from the Intimacy envelope). A long note means:
- The Intimacy envelope has longer to reach its high sustain level (0.9)
- The three-stage thermal model has more blocks to converge toward that value
- Short notes never reach full thermal convergence → less warmth, less saturation

---

## 4. Engine-Level Integration

### Voice Processing Loop (OxytocinEngine.h, lines 216–258)

```cpp
for (int vi = 0; vi < maxV; ++vi)
{
    auto& v = voices[vi];
    if (!v.isActive())
        continue;
    
    // Apply per-voice LFO modulation
    ParamSnapshot voiceSnap = snap;
    voiceSnap.cutoff *= std::pow(2.0f, lfo1Val * snap.lfoDepth * 2.0f / 12.0f);
    
    // Process this voice's block
    // The voice's internal accumulators (intimacyEnv, thermal, drive) build over time
    v.processBlock(monoBuffer.getData(), numSamples, voiceSnap, memory, vi, maxV);
    
    // Accumulate love values for memory
    sumI += v.lastEffI;  // Block-averaged I
    sumP += v.lastEffP;  // Block-averaged P
    sumC += v.lastEffC;  // Block-averaged C
    ++activeCount;
}

// Update global memory (fed by per-voice accumulators)
float avgI = (activeCount > 0) ? (sumI / activeCount) : 0.0f;
float avgP = (activeCount > 0) ? (sumP / activeCount) : 0.0f;
float avgC = (activeCount > 0) ? (sumC / activeCount) : 0.0f;
memory.update(avgI, avgP, avgC, anyActive, snap.memoryDepth, snap.memoryDecay, blockTime);
```

### Per-Voice Block Accumulation (OxytocinVoice.h, lines 304–349)

```cpp
float accI = 0.0f, accP = 0.0f, accC = 0.0f;

for (int i = 0; i < numSamples; ++i)
{
    // ... envelope ticks ...
    float pEnv = passionEnv.tick(...);
    float iEnv = intimacyEnv.tick(...);
    float cEnv = commitmentEnv.tick(...);
    
    float effI = snap.intimacy * iEnv;
    float effP = snap.passion * pEnv;
    float effC = snap.commitment * cEnv;
    
    memory.applyBoost(effI, effP, effC, ...);  // Memory boost applied
    
    // Accumulate this sample's contribution
    accI += boostedI;
    accP += boostedP;
    accC += boostedC;
}

// At end of block, store block-average for engine to use
if (numSamples > 0)
{
    float invN = 1.0f / static_cast<float>(numSamples);
    lastEffI = accI * invN;  // Block average
    lastEffP = accP * invN;
    lastEffC = accC * invN;
}
```

---

## 5. Velocity Interaction

### Attack Speed Modulation (OxytocinVoice.h, lines 294–297)
```cpp
float velAttackScale = 0.5f + 0.5f * vel;  // [0.5 at ppp .. 1.0 at fff]
passionEnv.setAttackScale(velAttackScale);
```

### Passion Peak Scaling (OxytocinVoice.h, lines 342–344)
```cpp
float passionVelScale = vel;  // D001: velocity → passion peak
boostedP *= passionVelScale;
```

**Effect on duration:** Higher velocity = faster attack + louder passion peak, but does NOT shorten the sustain stage. A long note at low velocity still accumulates intimacy and commitment over time.

---

## 6. Key Patterns for OXIDIZE Implementation

### Pattern 1: Multi-Stage Envelope
```
Passion (fast attack, low sustain)     → Transient character, velocity-dependent
Intimacy (sigmoid attack, high sustain) → Core accumulation parameter (duration-sensitive)
Commitment (linear attack, sustain)    → Long-term bond (very slow convergence)
```

### Pattern 2: Block-Rate Coefficient Cache
```
prepare()                    → initialize sr, invSr
updateCoefficients()         → cache exp() results once per block
tick() per sample            → use cached coefficients in state machine
```

### Pattern 3: Leaky Integrator with Signal-Load Adaptation
```
Update phase:               targetIntimacy = envelope.getValue()
Per-sample accumulation:    stage_n = target + (stage_n - target) * coeff_n
Load adjustment:            envelope_follower shortens tau when signal is loud
```

### Pattern 4: Block-Average Accumulation
```
Per-sample loop:   accI += boostedI
After loop:        lastEffI = accI / numSamples
Engine reads:      v.lastEffI for memory boost
```

---

## Summary Table

| Envelope | Attack Shape | Attack Time | Sustain | Release | Purpose |
|----------|--------------|-------------|---------|---------|---------|
| **Passion** | Exponential | `passionRate / velScale` | 15% | 4× attack | Transient; velocity-dependent; quick decay |
| **Intimacy** | Sigmoid | `warmthRate` | 90% | 2× attack | **Core warmth accumulator; duration-sensitive** |
| **Commitment** | Linear | `commitRate` | 95% | 10× attack | **Long-term bond; slowest convergence** |

| Component | Computation | Update Rate | Purpose |
|-----------|-------------|-------------|---------|
| **Envelope Coefficient** | `exp(-1/(sr*tau))` | Once per block (if rate changed) | IIR filter stability; never per-sample |
| **Thermal Stage** | `stage_n = target + (stage_n - target) * coeff` | Per sample | Smooth convergence toward envelope value |
| **Load Adjustment** | `std::pow(coeff, 1/loadMult)` | Once per block | Signal-dependent thermal acceleration |
| **Block Average** | `accI / numSamples` | Once per block (after loop) | Feed engine memory system |

