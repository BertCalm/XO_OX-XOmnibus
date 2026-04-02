# Skill: /mod-matrix-builder

**Invoke with:** `/mod-matrix-builder`
**Status:** LIVE
**Last Updated:** 2026-03-20 | **Version:** 1.0 | **Next Review:** On doctrine update (D002/D005/D006)
**Purpose:** Provide a reusable, doctrine-compliant 8-slot modulation matrix template for any XO_OX engine — covering D002 (modulation depth), D005 (breathing), and D006 (expression) requirements in one pass.

---

## When to Use This Skill

Use this skill when:
- Writing a new standalone XO_OX engine and need to implement the mod matrix
- An engine is failing D002, D005, or D006 (missing LFOs, aftertouch, mod wheel)
- Refactoring an engine's Parameters.h or adapter to add proper modulation routing
- Reviewing an engine's expression coverage against doctrines

---

## The 8-Slot Mod Matrix Standard

Every XO_OX engine should have **at minimum** these 8 modulation routes. They map to the doctrine requirements:

| Slot | Source | Destination | Doctrine | Notes |
|------|--------|-------------|----------|-------|
| 1 | LFO 1 | Filter Cutoff | D002 + D005 | Rate floor ≤ 0.01 Hz (16 min cycle) |
| 2 | LFO 2 | Pitch / Amplitude | D002 + D005 | Independent second LFO |
| 3 | Envelope | Filter Cutoff (velocity-scaled) | D001 + D002 | Higher velocity = brighter |
| 4 | Velocity | Envelope Attack | D001 | Velocity shapes timbre, not just volume |
| 5 | Mod Wheel (CC1) | LFO Depth or Filter Resonance | D006 | Non-negotiable for MIDI-capable engines |
| 6 | Aftertouch | LFO Rate or Filter Cutoff | D006 | Non-negotiable for MIDI-capable engines |
| 7 | Envelope | Pitch (vibrato) | D002 | Slow envelope for natural vibrato |
| 8 | Velocity | Character Macro Target | D001 | Velocity drives the engine's identity parameter |

**Exception:** OPTIC is the only engine explicitly exempted from D006 (visual-only engine, no pitch/filter).

---

## Parameters.h Template

Add to the engine's `Parameters.h` under the existing parameter declarations:

```cpp
// === MODULATION MATRIX PARAMETERS ===

// LFO 1 (filter/timbre)
PARAM_ID( LFO1Rate,  "{prefix}_lfo1Rate"  )  // 0.01–20 Hz (LOG), default 0.3 Hz
PARAM_ID( LFO1Depth, "{prefix}_lfo1Depth" )  // 0–1, default 0.0
PARAM_ID( LFO1Shape, "{prefix}_lfo1Shape" )  // 0=Sine 1=Tri 2=Saw 3=SqHi 4=S&H, default 0

// LFO 2 (pitch/amplitude)
PARAM_ID( LFO2Rate,  "{prefix}_lfo2Rate"  )  // 0.01–20 Hz (LOG), default 0.5 Hz
PARAM_ID( LFO2Depth, "{prefix}_lfo2Depth" )  // 0–1, default 0.0
PARAM_ID( LFO2Shape, "{prefix}_lfo2Shape" )  // same enum as LFO1

// Envelope mod
PARAM_ID( EnvFilterAmt, "{prefix}_envFilterAmt" )  // -1–1, default 0.5 (positive = brighter with attack)
PARAM_ID( VelFilterAmt, "{prefix}_velFilterAmt" )  // 0–1, default 0.6 (D001 compliance)

// Expression inputs (D006)
PARAM_ID( ModWheelAmt,   "{prefix}_modWheelAmt"   )  // 0–1, default 0.0 (user sets depth)
PARAM_ID( AftertouchAmt, "{prefix}_aftAmt"         )  // 0–1, default 0.0
PARAM_ID( AftertouchDst, "{prefix}_aftDst"         )  // 0=Filter 1=LFORate 2=Pitch 3=Character
```

Replace `{prefix}` with the engine's parameter prefix (e.g., `snap_`, `morph_`, `dub_`).

---

## DSP Implementation in EngineVoice.h

### LFO Object (add to voice or engine class)

```cpp
// In class header:
struct SimpleLFO {
    float phase = 0.0f;
    float rate = 0.3f;    // Hz
    float sampleRate = 44100.0f;

    void prepare(float sr) { sampleRate = sr; }

    // Call once per sample. Returns -1..+1.
    float tick() {
        phase += rate / sampleRate;
        if (phase >= 1.0f) phase -= 1.0f;
        return std::sin(phase * 6.28318530718f);  // Sine default
    }

    void setRate(float rateHz) {
        rate = juce::jlimit(0.01f, 20.0f, rateHz);  // Constrain to 0.01–20 Hz range (D005 floor: 0.01 Hz)
    }
};

SimpleLFO lfo1, lfo2;
```

### Mod Matrix Apply (call at start of renderBlock or per-voice)

```cpp
// Read mod sources
float lfo1Out = lfo1.tick();   // -1..+1
float lfo2Out = lfo2.tick();   // -1..+1
float vel01   = currentVelocity / 127.0f;  // 0..1
float modWheel = *params.modWheelAmt * modWheelCC;   // modWheelCC = CC1 value 0..1
float aft      = *params.aftAmt * aftertouchValue;   // aftertouchValue = poly/mono AT 0..1

// Apply to filter cutoff (combine all contributors)
float filterMod = 0.0f;
filterMod += *params.lfo1Depth * lfo1Out;          // Slot 1: LFO1 → filter
filterMod += *params.envFilterAmt * envLevel;      // Slot 3: envelope → filter
filterMod += *params.velFilterAmt * vel01;         // Slot 4: velocity → filter brightness (D001)
filterMod += modWheel * 0.3f;                      // Slot 5: mod wheel → filter depth
filterMod += aft * (afterTouchDst == 0 ? 0.4f : 0.0f);  // Slot 6 (if dst=Filter)

float finalCutoff = baseCutoff + filterMod * (20000.0f - baseCutoff);

// Apply to pitch
float pitchMod = 0.0f;
pitchMod += *params.lfo2Depth * lfo2Out * 0.1f;   // Slot 2: LFO2 → pitch (±semitone * 0.1)
pitchMod += aft * (afterTouchDst == 2 ? 0.05f : 0.0f);  // Slot 6 (if dst=Pitch) — subtle vibrato
```

---

## ParamSnapshot Pattern (D004 compliance)

Add mod matrix params to the snapshot struct to avoid APVTS lookups on the audio thread:

```cpp
struct {EnginePrefix}ParamSnapshot {
    // ... existing params ...

    // Mod matrix — add these
    float lfo1Rate, lfo1Depth, lfo1Shape;
    float lfo2Rate, lfo2Depth, lfo2Shape;
    float envFilterAmt, velFilterAmt;
    float modWheelAmt, aftAmt, aftDst;

    void update(juce::AudioProcessorValueTreeState& apvts) {
        // ... existing reads ...
        lfo1Rate  = *apvts.getRawParameterValue("{prefix}_lfo1Rate");
        lfo1Depth = *apvts.getRawParameterValue("{prefix}_lfo1Depth");
        lfo1Shape = *apvts.getRawParameterValue("{prefix}_lfo1Shape");
        lfo2Rate  = *apvts.getRawParameterValue("{prefix}_lfo2Rate");
        lfo2Depth = *apvts.getRawParameterValue("{prefix}_lfo2Depth");
        lfo2Shape = *apvts.getRawParameterValue("{prefix}_lfo2Shape");
        envFilterAmt = *apvts.getRawParameterValue("{prefix}_envFilterAmt");
        velFilterAmt = *apvts.getRawParameterValue("{prefix}_velFilterAmt");
        modWheelAmt  = *apvts.getRawParameterValue("{prefix}_modWheelAmt");
        aftAmt       = *apvts.getRawParameterValue("{prefix}_aftAmt");
        aftDst       = *apvts.getRawParameterValue("{prefix}_aftDst");
    }
} snapshot;
```

---

## Adapter Wiring (XOceanus)

When wrapping in an XOceanus adapter, ensure MIDI expression is forwarded:

```cpp
// In adapter's renderBlock(), before calling the standalone processBlock:
void renderBlock(juce::AudioBuffer<float>& buffer,
                 juce::MidiBuffer& midi,
                 int numSamples) override
{
    // Forward CC1 (mod wheel) to standalone engine
    for (const auto metadata : midi) {
        auto msg = metadata.getMessage();
        if (msg.isController()) {
            if (msg.getControllerNumber() == 1) {
                modWheelValue = msg.getControllerValue() / 127.0f;
                engine.setModWheel(modWheelValue);    // engine must accept this
            }
        }
        if (msg.isAftertouch() || msg.isChannelPressure()) {
            float atVal = msg.isChannelPressure()
                ? msg.getChannelPressureValue() / 127.0f
                : msg.getAfterTouchValue() / 127.0f;
            engine.setAftertouch(atVal);
        }
    }
    // ... rest of render ...
}
```

**Important:** Many adapters strip CC1 in the MIDI loop. Check the adapter's MIDI iterator before assuming mod wheel works.

---

## JUCE Parameter Layout Additions

In `createParameterLayout()` (or `addParametersImpl()`):

```cpp
// LFO 1
layout.add(std::make_unique<juce::AudioParameterFloat>(
    "{prefix}_lfo1Rate", "LFO 1 Rate",
    juce::NormalisableRange<float>(0.01f, 20.0f, 0.0f, 0.3f),  // skew=0.3 for LOG feel
    0.3f, "Hz"));

layout.add(std::make_unique<juce::AudioParameterFloat>(
    "{prefix}_lfo1Depth", "LFO 1 Depth",
    juce::NormalisableRange<float>(0.0f, 1.0f),
    0.0f));

layout.add(std::make_unique<juce::AudioParameterChoice>(
    "{prefix}_lfo1Shape", "LFO 1 Shape",
    juce::StringArray{"Sine", "Triangle", "Sawtooth", "Square", "S&H"},
    0));

// LFO 2 — same pattern with lfo2Rate, lfo2Depth, lfo2Shape, defaults 0.5f

// Expression inputs
layout.add(std::make_unique<juce::AudioParameterFloat>(
    "{prefix}_modWheelAmt", "Mod Wheel Amount",
    juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));

layout.add(std::make_unique<juce::AudioParameterFloat>(
    "{prefix}_aftAmt", "Aftertouch Amount",
    juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));

layout.add(std::make_unique<juce::AudioParameterChoice>(
    "{prefix}_aftDst", "Aftertouch Destination",
    juce::StringArray{"Filter", "LFO Rate", "Pitch", "Character"},
    0));
```

---

## Doctrine Compliance Checklist

After implementing the mod matrix, verify:

- [ ] **D001** — `velFilterAmt` routes velocity to filter brightness (higher vel = brighter). Test: play ppp vs fff; filter cutoff should change.
- [ ] **D002** — At least 2 independent LFOs wired to audible destinations. Test: set LFO1 depth to 0.5, confirm filter moves; set LFO2 depth to 0.3, confirm pitch/amp moves.
- [ ] **D005** — LFO rate floor ≤ 0.01 Hz. Test: set `lfo1Rate` to minimum, wait 16 seconds, confirm modulation has not completed one cycle.
- [ ] **D006 mod wheel** — CC1 routes to LFO depth or filter cutoff. Test: in DAW, draw CC1 automation from 0 to 127; confirm audible change.
- [ ] **D006 aftertouch** — Aftertouch routes to LFO rate, filter, or pitch. Test: send poly/mono aftertouch; confirm change.
- [ ] **D004** — All new parameters affect audio output. Test each parameter: set to min and max, confirm different output.

---

## Reference Implementations

Strong examples to copy from in this codebase:

| Engine | Why Study It |
|--------|-------------|
| `Source/Engines/Bite/BiteEngine.h` | Cleanest adapter + D006 aftertouch pattern |
| `Source/Engines/Ouroboros/OuroborosEngine.h` | Most complete mod routing including leash + coupling |
| `Source/Engines/Orbital/OrbitalEngine.h` | Group envelope system + full coupling + 9 coupling input types |
| `Source/Engines/Origami/OrigamiEngine.h` | STFT-based engine with proper mod matrix |
