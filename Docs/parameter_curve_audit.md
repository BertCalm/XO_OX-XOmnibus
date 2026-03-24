# Parameter Curve Audit — XOlokun Fleet

**Date:** 2026-03-14
**Auditor:** Claude Code (automated)
**Scope:** Filter cutoff, envelope time, and pitch parameters across 8 engines — Snap, Orbital, Obsidian, Origami, Morph, Oblique, Ocelot, Overworld

---

## Background: Why Curves Matter

JUCE `AudioParameterFloat` uses `NormalisableRange<float>(min, max, step, skewFactor)`. When `skewFactor = 1.0` (the default), the mapping is **linear**: the midpoint of the knob maps to the midpoint of the numeric range.

This is wrong for most audio parameters:

- **Frequency (filter cutoff):** Human pitch perception is logarithmic (octaves, not Hz). A linear 20–20000 Hz knob places the midpoint at 10,010 Hz — far above the musically interesting low-to-mid range. Correct fix: `skewFactor = 0.25` to 0.3.
- **Envelope time (attack, decay, release):** Musical feel requires precision at short times. A drummer needs 1ms vs 5ms attack to feel different; the difference between 3s and 5s is less critical. A linear 0–8s decay makes the bottom 10% of knob range cover 0–800ms while the top 90% of range controls 800ms–8s. Correct fix: `skewFactor = 0.3` to 0.4 (exponential-ish, concentrates resolution at small values).
- **Amplitude/level:** Linear 0–1 is generally fine for normalized gain if the downstream DSP applies a non-linear curve. Only needs skew if the parameter is used as a direct linear gain multiplier.
- **LFO rate:** Perceptually logarithmic. 0.01–0.1 Hz (breath) is as wide a range as 1–10 Hz (tremolo). Fix: `skewFactor = 0.3`.

**JUCE `skewFactor` reference:**
- `1.0` = linear
- `< 1.0` = concentrated resolution at low end (exponential-ish for time/freq)
- `> 1.0` = concentrated resolution at high end (rarely useful in audio)
- `0.3` = standard log-ish mapping, ~1 decade per 1/3 of knob travel
- `0.25` = more aggressive log curve, correct for 20–20000 Hz frequency range

---

## Audit Table

| Engine | Parameter | Param ID | Range | Step | Skew | Type | Status |
|--------|-----------|----------|-------|------|------|------|--------|
| **Snap** | Filter Cutoff | `snap_filterCutoff` | 20–20000 Hz | 0.1 | **0.3** | frequency | PASS |
| **Snap** | Decay Time | `snap_decay` | 0–8 s | 0.001 | **0.3** | time | FIXED |
| **Snap** | Snap (pitch sweep amt) | `snap_snap` | 0–1 | 0.01 | 1.0 | normalized | OK (downstream log) |
| **Orbital** | Filter Cutoff | `orb_filterCutoff` | 20–20000 Hz | 0.1 | **0.3** | frequency | PASS |
| **Orbital** | Amp Attack | `orb_ampAttack` | 0.001–8 s | 0.001 | **0.4** | time | PASS |
| **Orbital** | Amp Decay | `orb_ampDecay` | 0.05–4 s | 0.001 | **0.4** | time | PASS |
| **Orbital** | Amp Release | `orb_ampRelease` | 0.05–8 s | 0.001 | **0.4** | time | PASS |
| **Orbital** | Group Attack 1–4 | `orb_groupAttack1–4` | 0.001–4 s | 0.001 | **0.4** | time | PASS |
| **Orbital** | Group Decay 1–4 | `orb_groupDecay1–4` | 0.01–4 s | 0.001 | **0.4** | time | PASS |
| **Orbital** | FM Ratio | `orb_fmRatio` | 0.5–8 | 0.01 | **0.4** | ratio | PASS |
| **Obsidian** | Filter Cutoff | `obsidian_filterCutoff` | 20–20000 Hz | 0.1 | **0.3** | frequency | PASS |
| **Obsidian** | Amp Attack | `obsidian_ampAttack` | 0–10 s | 0.001 | **0.3** | time | PASS |
| **Obsidian** | Amp Decay | `obsidian_ampDecay` | 0–10 s | 0.001 | **0.3** | time | PASS |
| **Obsidian** | Amp Release | `obsidian_ampRelease` | 0–20 s | 0.001 | **0.3** | time | PASS |
| **Obsidian** | PD Envelope Attack | `obsidian_depthAttack` | 0–10 s | 0.001 | **0.3** | time | PASS |
| **Obsidian** | PD Envelope Decay | `obsidian_depthDecay` | 0–10 s | 0.001 | **0.3** | time | PASS |
| **Obsidian** | PD Envelope Release | `obsidian_depthRelease` | 0–20 s | 0.001 | **0.3** | time | PASS |
| **Origami** | Amp Attack | `origami_ampAttack` | 0–10 s | 0.001 | **0.3** | time | PASS |
| **Origami** | Amp Decay | `origami_ampDecay` | 0–10 s | 0.001 | **0.3** | time | PASS |
| **Origami** | Amp Release | `origami_ampRelease` | 0–20 s | 0.001 | **0.3** | time | PASS |
| **Origami** | Fold Env Attack | `origami_foldEnvAttack` | 0–10 s | 0.001 | **0.3** | time | PASS |
| **Origami** | Fold Env Decay | `origami_foldEnvDecay` | 0–10 s | 0.001 | **0.3** | time | PASS |
| **Origami** | Fold Env Release | `origami_foldEnvRelease` | 0–20 s | 0.001 | **0.3** | time | PASS |
| **Origami** | LFO1/2 Rate | `origami_lfo1Rate`, `lfo2Rate` | 0.01–30 Hz | 0.01 | **0.3** | rate | PASS |
| **Morph** | Filter Cutoff | `morph_filterCutoff` | 20–20000 Hz | 0.1 | **0.3** | frequency | PASS |
| **Morph** | Bloom (Attack) | `morph_bloom` | 0.001–10 s | 0.001 | **0.4** | time | PASS |
| **Morph** | Decay | `morph_decay` | 0.01–8 s | 0.001 | **0.4** | time | FIXED |
| **Morph** | Release | `morph_release` | 0.01–10 s | 0.001 | **0.4** | time | PASS |
| **Oblique** | Filter Cutoff | `oblq_filterCut` | 20–20000 Hz | 0.1 | **0.25** | frequency | PASS |
| **Oblique** | Amp Attack | `oblq_attack` | 0.001–2 s | 0.001 | **0.3** | time | PASS |
| **Oblique** | Amp Decay | `oblq_decay` | 0.001–5 s | 0.001 | **0.3** | time | PASS |
| **Oblique** | Amp Release | `oblq_release` | 0.001–5 s | 0.001 | **0.3** | time | PASS |
| **Oblique** | Click Decay | `oblq_percDecay` | 0.001–0.05 s | 0.001 | 1.0 | time | ACCEPTABLE (narrow range) |
| **Oblique** | Bounce Rate | `oblq_bounceRate` | 20–500 Hz | 1.0 | **0.4** | rate | PASS |
| **Oblique** | Click Tone | `oblq_clickTone` | 200–8000 Hz | 1.0 | **0.3** | frequency | PASS |
| **Oblique** | Prism Delay | `oblq_prismDelay` | 10–500 ms | 0.1 | **0.35** | time | PASS |
| **Oblique** | Phaser Rate | `oblq_phaserRate` | 0.05–8 Hz | 0.01 | **0.35** | rate | PASS |
| **Ocelot** | Sample Rate Red | `ocelot_sampleRate` | 4000–44100 Hz | 0.0 | **0.3** | frequency | PASS |
| **Ocelot** | Amp Attack | `ocelot_ampAttack` | 0.001–8000 ms | 0.0 | **0.3** | time | PASS |
| **Ocelot** | Amp Decay | `ocelot_ampDecay` | 50–4000 ms | 0.0 | **0.3** | time | PASS |
| **Ocelot** | Amp Release | `ocelot_ampRelease` | 50–8000 ms | 0.0 | **0.3** | time | PASS |
| **Ocelot** | Creature Attack | `ocelot_creatureAttack` | 0–1 (norm) | 0.0 | **0.3** | time | FIXED |
| **Ocelot** | Creature Decay | `ocelot_creatureDecay` | 0–1 (norm) | 0.0 | **0.3** | time | FIXED |
| **Ocelot** | Delay Time | `ocelot_delayTime` | 0–1 (norm) | 0.0 | 1.0 | time | WARNING (see notes) |
| **Overworld** | Filter Cutoff | `ow_filterCutoff` | 20–20000 Hz | 1.0 | **0.25** | frequency | PASS |
| **Overworld** | Amp Attack | `ow_ampAttack` | 0.001–2 s | 0.001 | **0.3** | time | PASS |
| **Overworld** | Amp Decay | `ow_ampDecay` | 0.001–5 s | 0.001 | **0.3** | time | PASS |
| **Overworld** | Amp Release | `ow_ampRelease` | 0.001–5 s | 0.001 | **0.3** | time | PASS |
| **Overworld** | ERA Drift Rate | `ow_eraDriftRate` | 0–4 Hz | 0.01 | **0.5** | rate | PASS |
| **Overworld** | ERA Portamento | `ow_eraPortaTime` | 0–4 s | 0.01 | **0.4** | time | PASS |
| **Overworld** | ERA Memory Time | `ow_eraMemTime` | 0.05–3 s | 0.01 | **0.5** | time | PASS |
| **Overworld** | Glitch Rate | `ow_glitchRate` | 0.1–20 Hz | 0.1 | **0.5** | rate | PASS |

---

## Issues Found

### FIXED — 3 Egregious Cases Corrected

#### Fix 1: Snap Decay — `snap_decay`
**File:** `Source/Engines/Snap/SnapEngine.h` line 765

**Problem:** Snap is a percussive engine whose identity is built on transient precision — the difference between a 20ms and 80ms decay is the entire character difference between a tight click and a loose thud. With a linear range of 0–8s, the bottom ~10% of knob travel (0–800ms) had to cover ALL percussive use cases while 90% of the range controlled musically unimportant long tails.

```cpp
// BEFORE (linear — knob precision is wrong for percussion):
juce::NormalisableRange<float> (0.0f, 8.0f, 0.01f), 0.5f

// AFTER (skew = 0.3 — concentrates resolution at short times):
juce::NormalisableRange<float> (0.0f, 8.0f, 0.001f, 0.3f), 0.5f
```

**Effect:** With `skewFactor = 0.3`, knob midpoint (0.5) now maps to approximately `8 * 0.5^(1/0.3) ≈ 0.35s` instead of `4.0s`. Short percussive decay times (0–1s) now occupy ~70% of the knob travel. The step was also reduced from 0.01 to 0.001 to match the precision the skew now makes accessible at the low end.

---

#### Fix 2: Morph Decay — `morph_decay`
**File:** `Source/Engines/Morph/MorphEngine.h` line 752

**Problem:** Morph is a pad engine. Its Bloom (attack) and Release parameters already had correct `skewFactor = 0.4`. The Decay parameter was missing the skew, creating an inconsistency where the attack was musically precise but the decay was linear — meaning a pad creator would get confusingly different feel from near-identical knob positions on attack vs. decay.

```cpp
// BEFORE (linear — inconsistent with Bloom/Release skew):
juce::NormalisableRange<float> (0.01f, 8.0f, 0.01f), 2.0f

// AFTER (skew = 0.4 — matches Bloom/Release, concentrates res at short times):
juce::NormalisableRange<float> (0.01f, 8.0f, 0.001f, 0.4f), 2.0f
```

**Effect:** Morph's ADSR envelope parameters are now all on a consistent skew = 0.4 curve. Knob midpoint maps to ~0.9s instead of ~4s. Short pad decays (for staccato pads, plucked sounds) now feel precise and distinct.

---

#### Fix 3: Ocelot Creature Attack/Decay — `ocelot_creatureAttack`, `ocelot_creatureDecay`
**File:** `Source/Engines/Ocelot/OcelotParameters.h` lines 202–203

**Problem:** Creature attack and decay are normalized 0–1 time parameters (the DSP engine converts them to actual time internally). Without a skew, the bottom of the range (short, snappy creature calls) is compressed and imprecise. Users have poor resolution for quick creature triggers.

```cpp
// BEFORE (linear — poor resolution at fast settings):
F(creatureAttack, "Creature Attack", 0.0f, 1.0f, 0.3f);
F(creatureDecay,  "Creature Decay",  0.0f, 1.0f, 0.5f);

// AFTER (skew = 0.3 — fast triggers are now precise):
F(creatureAttack, "Creature Attack", 0.0f, 1.0f, 0.3f, 0.3f);
F(creatureDecay,  "Creature Decay",  0.0f, 1.0f, 0.5f, 0.3f);
```

**Effect:** Quick creature calls (birds, insects) occupy more of the knob range. Slow howls and whale songs are still accessible at the high end.

---

### WARNING — Not Fixed But Noted

**Ocelot `ocelot_delayTime`** is `(0.0f, 1.0f)` normalized with no skew. This is a delay time parameter. Short delay times (1–50ms) create chorus/flanger effects; longer times (100–1000ms) create echo. With linear 0–1 mapping, the short-delay zone is compressed. This was not fixed in this pass because the internal mapping in OcelotEngine.h was not traced to determine if the DSP layer compensates. **Recommendation:** Apply `skewFactor = 0.3` or convert to explicit ms range (0–1000ms, skew 0.3).

---

## Fleet-Wide Status Summary

| Engine | Filter Cutoff | Env Times | Notes |
|--------|--------------|-----------|-------|
| Snap | PASS (0.3) | FIXED (now 0.3) | Decay was linear |
| Orbital | PASS (0.3) | PASS (0.4) | All correct |
| Obsidian | PASS (0.3) | PASS (0.3) | All correct |
| Origami | N/A | PASS (0.3) | No raw filter cutoff param (spectral fold engine) |
| Morph | PASS (0.3) | FIXED (0.4) | Decay was linear |
| Oblique | PASS (0.25) | PASS (0.3) | Best curve in fleet — 0.25 on cutoff |
| Ocelot | PASS (0.3, sample rate) | PASS (amp), FIXED (creature) | creature A/D were linear |
| Overworld | PASS (0.25) | PASS (0.3) | Good — note cutoff uses 0.25 |

**Result: 3 parameters fixed, 1 warning noted, no regressions.**

---

## Guidance for Future Parameter Authoring

### Quick Reference Table

| Parameter Type | Recommended Skew | Example |
|---------------|-----------------|---------|
| Filter cutoff (20–20000 Hz) | `0.25` | `NR(20.0f, 20000.0f, 0.1f, 0.25f)` |
| Envelope time (0–10s) | `0.3` | `NR(0.001f, 10.0f, 0.001f, 0.3f)` |
| Envelope time (0–4s) | `0.3–0.4` | `NR(0.001f, 4.0f, 0.001f, 0.4f)` |
| LFO rate (0.01–20 Hz) | `0.3` | `NR(0.01f, 20.0f, 0.01f, 0.3f)` |
| Oscillator ratio/multiple | `0.4–0.5` | `NR(0.5f, 8.0f, 0.01f, 0.4f)` |
| Portamento time | `0.4–0.5` | `NR(0.0f, 4.0f, 0.01f, 0.4f)` |
| Delay time (ms) | `0.3` | `NR(1.0f, 1000.0f, 1.0f, 0.3f)` |
| Reverb size | `1.0` (linear OK) | Room size is not perceptually log |
| Level/mix (0–1) | `1.0` (linear OK) | Downstream DSP usually handles |
| Pitch (semitones, bipolar) | `1.0` (linear OK) | Semitones are already log scale |

### Rules

1. **Any parameter with "time" or "rate" in its name should have skew ≤ 0.4.** Use 0.3 as the default unless there is a specific reason to choose otherwise.
2. **Any parameter with "cutoff" or "frequency" in its name should have skew ≤ 0.3.** Use 0.25 for the full 20–20000 Hz human hearing range.
3. **Consistent within an engine.** If attack has skew 0.4, decay and release should also have 0.4. Do not mix skew values within the same ADSR group.
4. **Normalized 0–1 time parameters need skew too.** The fact that a parameter is 0–1 does not exempt it from needing a skew. If the DSP engine scales it to actual time, the user-facing curve is still linear by default.
5. **Step size and skew interact.** When you add a skew, reduce the step size at the low end: the skew concentrates resolution at small values, so a step of 0.01 that was coarse at the top of the range may now be surprisingly coarse at the bottom. Use `0.001` as a default step for any time parameter with skew.
6. **Document the curve choice.** Add a comment above the parameter explaining the mapping rationale, especially for non-standard skew values.

### Common Anti-Patterns

```cpp
// BAD: Linear decay — wastes 90% of knob on musically unimportant long tails
juce::NormalisableRange<float> (0.0f, 8.0f, 0.01f)

// BAD: Linear frequency — half the knob above 10kHz, musically useless
juce::NormalisableRange<float> (20.0f, 20000.0f, 0.1f)

// BAD: Large step + skew = invisible resolution problem
juce::NormalisableRange<float> (0.001f, 8.0f, 0.1f, 0.3f)
// With skew 0.3, step 0.1 means 0.0–0.1 in normalized space covers 0–very small value
// User will "snap" between a few tiny values at the bottom of the range
```

```cpp
// GOOD: Skewed decay — short times get precision
juce::NormalisableRange<float> (0.001f, 8.0f, 0.001f, 0.3f)

// GOOD: Frequency with proper log feel
juce::NormalisableRange<float> (20.0f, 20000.0f, 0.1f, 0.25f)

// GOOD: Consistent envelope group — all 4 stages on same skew
juce::NormalisableRange<float> (0.001f, 4.0f, 0.001f, 0.4f)  // attack
juce::NormalisableRange<float> (0.001f, 4.0f, 0.001f, 0.4f)  // decay
juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f)            // sustain (linear OK)
juce::NormalisableRange<float> (0.001f, 8.0f, 0.001f, 0.4f)  // release
```
