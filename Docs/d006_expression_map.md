# D006 Expression Map — XOmnibus Fleet Audit
*Generated: 2026-03-14 | Auditor: Expression Auditor, Round 4 Prism Sweep*

Doctrine D006: "Expression Input Is Not Optional."
Every engine must respond to velocity (affecting timbre, not just amplitude), aftertouch,
and at minimum one continuous controller (mod wheel CC1 or expression CC11).

---

## Summary Table

| Engine | Vel→Timbre | Aftertouch | Mod Wheel (CC1) | Expression (CC11) | Vel Curve | Priority |
|--------|-----------|------------|-----------------|-------------------|-----------|----------|
| Bite (OVERBITE) | No — ampVelSens scales amplitude only | Listed in mod matrix but NOT wired to MIDI handler | No | No | Linear | HIGH |
| Bob (OBLONG) | No — amplitude only | No | No | No | Linear | HIGH |
| Drift (ODYSSEY) | YES — velocity scales filter env depth (±10kHz sweep) | No | YES — CC1 → filter cutoff +4000 Hz | No | Linear | MEDIUM |
| Dub (OVERDUB) | YES — velocity scales filter env depth (±10kHz sweep) | No | No | No | Linear | MEDIUM |
| Fat (OBESE) | No — amplitude only | No | No | No | Linear | HIGH |
| Morph (ODDOSCAR) | No — amplitude only | No | YES — CC1 → morph position sweep (0–3.0 range) | No | Linear | MEDIUM |
| Oblique | YES — velocity sets bounce click level (harmonic decay cascade) | No | No | No | Linear | MEDIUM |
| Obscura | No — amplitude only | No | No | No | Linear | HIGH |
| Obsidian | No — amplitude only | No | No | No | Linear | HIGH |
| Oceanic | YES — velocity → particle scatter impulse (FM/amp/pan distribution) | No | No | No | Linear | MEDIUM |
| Ocelot | YES — velocity → KS string excitation (spectral content of pluck) | No | No | No | Linear | MEDIUM |
| Onset (drum) | No — amplitude only | No | No | No | Linear | HIGH |
| Opal | No — ampVelSens scales amplitude only (same pattern as Bite) | No | No | No | Linear | HIGH |
| Optic | N/A — pure modulation engine, no voices | No | No | No | N/A | N/A |
| Oracle | No — amplitude only | No | No | No | Linear | HIGH |
| Orbital | No — amplitude only | No | No | No | Linear | HIGH |
| Organon | YES — velocity → free energy catalyst (bloom rate, harmonic evolution speed) | No | No | No | Linear | MEDIUM |
| Origami | No — amplitude only | No | No | No | Linear | HIGH |
| Osprey | No — amplitude only | No | No | No | Linear | HIGH |
| Osteria | No — amplitude only | No | No | No | Linear | HIGH |
| Ouroboros | MARGINAL — velocity → attractor injection boost (transient onset energy, 50ms) | No | No | No | Linear | MEDIUM |
| Overworld | No — amplitude only (vel passed to VoicePool but used only for `mixed * envLevel * vel`) | No | No | No | Linear | HIGH |
| Owlfish | No — lastVelocity stored but never applied in audio loop | No | No | No | Linear | HIGH |
| Snap (ODDFELIX) | No — amplitude only | No | No | No | Linear | HIGH |

**Priority key:** HIGH = 4–5 items missing, MEDIUM = 2–3 missing.
*Optic is excluded as a zero-audio modulation engine — D006 does not apply.*

---

## Fleet-Wide Counts

| Item | Passing | Failing |
|------|---------|---------|
| Velocity → Timbre | 6 (Drift, Dub, Oblique, Oceanic, Ocelot, Organon) + 1 marginal (Ouroboros) | 16 |
| Aftertouch | 0 | 23 |
| Mod Wheel CC1 | 2 (Drift, Morph) | 21 |
| Expression CC11 | 0 | 23 |
| Non-linear Vel Curve | 0 | 23 |

**23 out of 23 voice engines are missing aftertouch.**
**23 out of 23 engines use a flat linear velocity-to-amplitude curve.**
**21 out of 23 voice engines ignore mod wheel entirely.**

---

## PolyAftertouch.h Analysis

File: `Source/Core/PolyAftertouch.h`

### What it provides
- CS-80-style per-voice pressure modulation
- Up to 3 configurable target slots: FilterCutoff, Amplitude, VibratoDepth, MorphPosition, PitchBend
- Four response curves: Linear, Exponential (x²), S-Curve (Hermite), Logarithmic (√x)
- Independent per-voice smoothing with separate attack (~5ms) and release (~20ms) coefficients
- `setChannelPressure(float)` for mono aftertouch — all voices receive the same pressure
- `setVoicePressure(int voiceIndex, float)` for polyphonic aftertouch
- `updateBlock(int numSamples)` — call once per block, produces `VoiceModulation` structs
- Output struct `VoiceModulation` provides: `filterOffset` (Hz), `amplitudeScale`, `vibratoDepth`, `morphOffset`, `pitchBend` (semitones)
- Max 16 voices (kMaxVoices = 16)
- No heap allocation, fully deterministic

### Is it drop-in usable?
**Yes, with minimal integration work.** The module is self-contained and requires three integration steps:

1. **MIDI handling** — catch `msg.isChannelPressure()` in the engine's MIDI loop and call `aftertouch.setChannelPressure(msg.getChannelPressureValue() / 127.0f)`.
2. **Prepare** — call `aftertouch.prepare(sampleRate)` in the engine's `prepare()` method.
3. **Block processing** — call `aftertouch.updateBlock(numSamples)` once per block, then read `aftertouch.getVoiceMod(voiceIndex)` inside the per-voice loop to apply `filterOffset`, `amplitudeScale`, etc.

The module handles smoothing internally. The engine does not need to manage smoothed pressure state.

### What would adding aftertouch to a standard engine require?
```
// In engine class declaration:
xomnibus::PolyAftertouch aftertouch;

// In prepare():
aftertouch.prepare(sampleRate);
aftertouch.setTarget1({ Target::FilterCutoff, 0.7f, 0.0f, 1.0f, ResponseCurve::SCurve });

// In MIDI handling loop:
if (msg.isChannelPressure())
    aftertouch.setChannelPressure(msg.getChannelPressureValue() / 127.0f);

// In processBlock(), before the per-voice loop:
aftertouch.updateBlock(numSamples);

// Inside per-voice loop:
auto& atMod = aftertouch.getVoiceMod(voiceIndex);
float modCutoff = baseCutoff + atMod.filterOffset;
```
**Estimated effort per engine: 10–15 lines. No structural changes required.**

---

## Quick Win Engines
*(Easiest to add aftertouch — minimal architecture disruption)*

### 1. Orbital
Has a simple voice struct with `velocity` already stored, a clear per-voice render loop, and additive partials that would respond beautifully to filter cutoff pressure modulation. The `spectralCouplingOffset[]` array shows the team is comfortable with per-partial offset patterns.

### 2. Oracle
Voice struct has `velocity` field and a clean per-voice `voiceGain` line. Stochastic synthesis responds naturally to pressure — a `filterOffset` from aftertouch would open/close the harmonic window in real time.

### 3. Obsidian
Phase-distortion synthesis that already uses filter LFO offsets. `filterOffset` from aftertouch targets the PD depth or amplitude stage naturally. The engine's existing `crossfadeGain` pattern means per-voice mods are already factored in.

### 4. Origami
Has per-voice `velocity` and `crossfadeGain` fields, STFT-based fold synthesis with a `foldPoint` parameter that would respond expressively to pressure. No coupling complexity.

### 5. Snap (ODDFELIX)
Smallest engine in the fleet, simplest voice struct, already has `envelopeLevel * voice.velocity` pattern. A `filterOffset` addition at `modCutoff` is one line. Good "template" engine to prove the pattern works before fleet-wide rollout.

---

## Recommended Wiring — Top 5 HIGH Priority Engines

### 1. Orbital (Additive synthesis — partials engine)
**Target:** Filter cutoff (spectral brightness gate) or partial tilt offset
**Range:** 0–6000 Hz cutoff offset, S-curve response
**Why:** Pressing harder opens the harmonic window — classic CS-80 expressiveness on additive synthesis.
```
// After voiceGain computation in per-voice loop:
auto& atMod = aftertouch.getVoiceMod(voiceIdx);
float brightnessMod = atMod.filterOffset * 0.001f; // scale to 0-6 semitones-equivalent
// Apply to partial gain rolloff parameter
```

### 2. Obsidian (Phase-distortion synthesis)
**Target:** PD depth (stage 2 cross-modulation depth)
**Range:** 0.0–0.35 additional depth (additive, clamped to 1.0)
**Why:** Pressing adds harmonic edge — the glass wall cracks. PD depth is the core timbral parameter; small pressure changes create dramatic spectral shifts.
```
// In per-voice block before PD computation:
auto& atMod = aftertouch.getVoiceMod(voiceIdx);
float pressurePDBoost = atMod.filterOffset / 8000.0f * 0.35f;
float effectivePDDepth = std::min(basePDDepth + pressurePDBoost, 1.0f);
```

### 3. Oracle (GENDY stochastic synthesis)
**Target:** Breakpoint amplitude step modulation (LFO2 depth proxy)
**Range:** Vibrato depth 0–1.0
**Why:** Pressure controls how much the stochastic voice "breathes" — gentle pressure adds harmonic flutter, hard pressure pins it to stability. Matches the oracle/prophecy identity.
```
auto& atMod = aftertouch.getVoiceMod(voiceIdx);
float stochMod = atMod.vibratoDepth * 0.5f;
// Add to existing stochAmpStep scaling
```

### 4. Bob (OBLONG — warm character synth)
**Target:** Filter cutoff + texture level
**Range:** 0–3000 Hz filter offset; texture level += aftertouch * 0.15 (fade in tape texture)
**Why:** Bob's warmth comes from the texture layer. Pressure expression could bring the tape texture layer in and out while also brightening the filter — mimicking a warm push into distortion.
```
auto& atMod = aftertouch.getVoiceMod(voiceIdx);
modCutoff += atMod.filterOffset * 0.3f;
effTexLevel = std::min(effTexLevel + atMod.amplitudeScale * 0.15f, 1.0f);
```

### 5. Origami (STFT fold synthesis)
**Target:** Fold point offset
**Range:** foldPoint += 0.0–0.25 (opens additional folds under pressure)
**Why:** The fold point is Origami's core timbral expression — pressing into a note creates new spectral folds in real time. Musically very compelling and architecturally straightforward.
```
auto& atMod = aftertouch.getVoiceMod(voiceIdx);
float pressureFold = atMod.morphOffset * 0.25f;
float effectiveFoldPoint = std::min(baseFoldPoint + pressureFold, 1.0f);
```

---

## Additional Notes

### Velocity Curves — Fleet-Wide Gap
Every engine in the fleet uses raw linear velocity multiplication:
```
out = signal * voice.velocity;  // or velGain = 1.0 - sens + sens * velocity
```
No engine uses a velocity curve. Musical recommendation for velocity curves:
- **Soft engines** (Osprey, Osteria, Oracle): `vel = std::sqrt(velocity)` — logarithmic, gentle response
- **Hard engines** (Bite, Oblique, Dub): `vel = velocity * velocity` — exponential, demands forceful playing
- **Universal improvement**: `vel = velocity * velocity * (3.0f - 2.0f * velocity)` — S-curve, musical at both extremes

### Bite "Aftertouch" False Positive
Bite's mod matrix lists "Aftertouch" as a source (line 1422) but there is no MIDI handler for channel pressure in `BiteEngine.h`. The source index is never populated with actual pressure data. This is a documented parameter that is silently non-functional — a D004 violation compounded inside the D006 gap.

### Mod Wheel Dead Zones
Morph's CC1 handler maps 0–127 to 0–3.0 morph offset. At rest (CC1=0) no morph shift occurs.
Drift's CC1 handler maps 0–127 to 0–4000 Hz filter offset. Musical but no configurable sensitivity.
Neither engine allows the user to disable or scale the CC1 response.

### Onset (Drum Engine)
Onset is a percussion synthesis engine. D006's aftertouch requirement does not apply to drums in the same way. However, velocity→timbre is still relevant — hard hits could change the DCW brightness or FM modulation index. Currently velocity only scales output amplitude.

---

*Audit conducted against all 24 engine files in `Source/Engines/`. PolyAftertouch.h analyzed at `Source/Core/PolyAftertouch.h`.*
