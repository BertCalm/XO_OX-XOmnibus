# Macro Audit — XOceanus 8-Engine Survey (2026-03-14)

**Doctrine:** D002 — Modulation Is the Lifeblood
**Standard:** M1-M4 (or equivalent) must produce audible, musically meaningful change in every preset.
A macro that modulates a parameter by less than 5% (scaling < 0.05) is effectively dead.

---

## Audit Table

| Engine | Macro | Name | Destination | Scaling | Status |
|--------|-------|------|-------------|---------|--------|
| **OVERWORLD** | M1 | ERA | ERA X offset (externalEraMod) | +1.0 | ✅ FIXED (was missing) |
| **OVERWORLD** | M2 | CRUSH | BitCrusher mix | +0.85 | ✅ FIXED (was missing) |
| **OVERWORLD** | M3 | GLITCH | GlitchEngine amount +0.9, mix +0.8 | +0.9/+0.8 | ✅ FIXED (was missing) |
| **OVERWORLD** | M4 | SPACE | FIREcho mix | +0.7 | ✅ FIXED (was missing) |
| **MORPH** | M1 | BLOOM | morph position offset | +1.5 | ✅ FIXED (was missing) |
| **MORPH** | M2 | DRIFT | detune spread (cents) | +30 cents | ✅ FIXED (was missing) |
| **MORPH** | M3 | DEPTH | filter cutoff | +6000 Hz | ✅ FIXED (was missing) |
| **MORPH** | M4 | SPACE | attack time multiplier | ×4.0 | ✅ FIXED (was missing) |
| **OBLIQUE** | M1 | FOLD | wavefold amount | +0.70 | ✅ FIXED (was missing) |
| **OBLIQUE** | M2 | BOUNCE | bounce rate +220ms, gravity +0.2 | +220ms/+0.2 | ✅ FIXED (was missing) |
| **OBLIQUE** | M3 | COLOR | prism color spread +0.5, prism mix +0.35 | +0.5/+0.35 | ✅ FIXED (was missing) |
| **OBLIQUE** | M4 | SPACE | phaser mix +0.5, prism feedback +0.3 | +0.5/+0.3 | ✅ FIXED (was missing) |
| **OCELOT** | PROWL | PROWL | ecosystemDepth +0.5, density +0.4 | +0.5/+0.4 | ✅ STRONG |
| **OCELOT** | FOLIAGE | FOLIAGE | reverbSize +0.4, reverbMix +0.3 | +0.4/+0.3 | ✅ STRONG |
| **OCELOT** | ECOSYSTEM | ECOSYSTEM | xfFloorCanopy +0.5, xfCanopyFloor +0.3, xfUnderEmerg +0.4 | +0.5 | ✅ STRONG |
| **OCELOT** | CANOPY | CANOPY | canopyLevel +0.4, canopyShimmer +0.5, canopySpectralFilter +0.3 | +0.5 | ✅ STRONG |
| **OSPREY** | M1 | CHARACTER | seaState +0.5 (+ coupling) | +0.5 | ✅ ADEQUATE |
| **OSPREY** | M2 | MOVEMENT | swellPeriod −4.0 (range 0.5-30s), LFO rate 0.05→1.0 Hz | −4.0s | ✅ STRONG |
| **OSPREY** | M3 | COUPLING | creatureDepth +0.3 | +0.3 | ⚠️ WEAK (solo effect subtle) |
| **OSPREY** | M4 | SPACE | harborVerb +0.4 | +0.4 | ✅ ADEQUATE |
| **OSTERIA** | M1 | CHARACTER | blend +0.8, convergence = macro value | +0.8 | ✅ STRONG |
| **OSTERIA** | M2 | MOVEMENT | elastic −0.7, stretch +0.4 | −0.7/+0.4 | ✅ STRONG |
| **OSTERIA** | M3 | COUPLING | sympathy +0.5, memory +0.5 | +0.5 each | ✅ STRONG |
| **OSTERIA** | M4 | SPACE | tavernMix +0.6, hall +0.5, oceanBleed +0.5 | +0.6 | ✅ STRONG |
| **OBSIDIAN** | M1 | CHARACTER | density +0.5, depth +0.3 | +0.5/+0.3 | ✅ ADEQUATE |
| **OBSIDIAN** | M2 | MOVEMENT | crossModulation +0.5 | +0.5 | ✅ ADEQUATE |
| **OBSIDIAN** | M3 | COUPLING | cascadeBlend +0.3 | +0.3 | ⚠️ WEAK (cascade is subtle) |
| **OBSIDIAN** | M4 | SPACE | stiffness +0.4, stereoWidth +0.3 | +0.4/+0.3 | ✅ ADEQUATE |
| **ORBITAL** | M1 | SPECTRUM | brightness = macro (direct map 0→1) | 1.0 full range | ✅ STRONG |
| **ORBITAL** | M2 | EVOLVE | morph lifted, envelope ×1–5x slower | ×5.0 | ✅ STRONG |
| **ORBITAL** | M3 | COUPLING | formantShift up to 12 semitones | +12 semitones | ✅ STRONG |
| **ORBITAL** | M4 | SPACE | stereoSpread (direct map 0→1) | 1.0 full range | ✅ STRONG |

---

## Pre-Audit Status Summary

| Engine | Macros Before Audit | Status Before |
|--------|---------------------|---------------|
| OVERWORLD | 0 — no macro params declared | DEAD (score: 0/10) |
| MORPH | 0 — no macro params declared | DEAD (score: 0/10) |
| OBLIQUE | 0 — no macro params declared | DEAD (score: 0/10) |
| OCELOT | 4 — wired in D004 Round 3 fix | STRONG (score: 7/10) |
| OSPREY | 4 — wired (D004 Round 3 fix for LFO) | ADEQUATE (score: 6/10) |
| OSTERIA | 4 — wired with 0.5–0.8 scaling | STRONG (score: 8/10) |
| OBSIDIAN | 4 — wired with 0.3–0.5 scaling | ADEQUATE (score: 6/10) |
| ORBITAL | 4 — direct full-range mapping | STRONG (score: 9/10) |

---

## The 3 Fixed Engines

### Fix 1: OVERWORLD — 4 Macros Added

**File:** `Source/Engines/Overworld/OverworldEngine.h`

**Problem:** The OverworldEngine adapter had zero macro parameters. The XOverworld standalone binary defines all its parameters via `xoverworld::addParameters()` but that function never included XOceanus macros. From an XOceanus performance perspective, turning the 4 macro knobs on an OVERWORLD slot produced zero audio change.

**Solution:** 4 new parameters declared directly in `addParameters()` of the XOceanus adapter (after the `xoverworld::addParameters()` delegate call), attached in `attachParameters()`, and applied at the top of `renderBlock()` before FX units are configured:

| Macro | Param ID | Before | After (at max) | Scaling |
|-------|----------|--------|----------------|---------|
| M1 ERA | `ow_macroEra` | No effect | ERA X sweeps +1.0 → full chip crossfade | 0→+1.0 |
| M2 CRUSH | `ow_macroCrush` | No effect | BitCrusher mix 0→0.85 (heavy lo-fi crunch) | 0→+0.85 |
| M3 GLITCH | `ow_macroGlitch` | No effect | GlitchEngine amount 0→0.9, mix 0→0.8 | 0→+0.9 |
| M4 SPACE | `ow_macroSpace` | No effect | FIREcho mix 0→0.7 (deep echo tail) | 0→+0.7 |

**Bonus:** The original code never called `crusher.setMix()` — this fix also activates the BitCrusher mix parameter (a pre-existing dead parameter) by calling `crusher.setMix(effectiveCrushMix)`.

**Macro strength:** 0/10 → **8/10**

---

### Fix 2: MORPH — 4 Macros Added

**File:** `Source/Engines/Morph/MorphEngine.h`

**Problem:** The MORPH (OddOscar axolotl) engine had zero macro parameters. A rich, characterful engine with a ladder filter, 3-oscillator chorus, and wavetable morph — but no live performance control. All 4 macro knob positions were inert from an audio perspective.

**Solution:** 4 new parameters declared in `addParametersImpl()`, attached in `attachParameters()`, and applied in the ParamSnapshot block at the top of `renderBlock()`. The effective values replace the original parameter reads throughout the block:

| Macro | Param ID | Before | After (at max) | Scaling |
|-------|----------|--------|----------------|---------|
| M1 BLOOM | `morph_macroBloom` | No effect | morph position +1.5 (sine→square character sweep) | 0→+1.5 |
| M2 DRIFT | `morph_macroDrift` | No effect | detune spread +30 cents (wider animated chorus) | 0→+30 cts |
| M3 DEPTH | `morph_macroDepth` | No effect | filter cutoff +6000 Hz (surface from cave) | 0→+6000 Hz |
| M4 SPACE | `morph_macroSpace` | No effect | attack time ×1 to ×4 (atmospheric long bloom) | ×4.0 |

The MORPH macro story: BLOOM unfurls Oscar's gills (more harmonics), DRIFT makes the axolotl churn through the reef (wider chorus), DEPTH brings it to the surface (brighter filter), SPACE gives it a very long meditative breath (slow attack).

**Macro strength:** 0/10 → **8/10**

---

### Fix 3: OBLIQUE — 4 Macros Added

**File:** `Source/Engines/Oblique/ObliqueEngine.h`

**Problem:** The OBLIQUE (Prism Fish) engine had zero macro parameters. A complex engine with wavefolder, 6-tap spectral delay, 6-stage phaser, and bouncing-ball ricochet rhythm — all the raw material for dramatic macro sweeps — but no wiring.

**Solution:** 4 new parameters declared in `addParametersImpl()`, attached in `attachParameters()`, and applied in the ParamSnapshot block. A naming conflict with the existing aftertouch `effectivePrismMix` was resolved by staging: macro produces `macroPrismMixBase`, then aftertouch adds on top to produce the final `effectivePrismMix`.

| Macro | Param ID | Before | After (at max) | Scaling |
|-------|----------|--------|----------------|---------|
| M1 FOLD | `oblq_macroFold` | No effect | wavefold +0.7 (heavy grit, metallic harmonics) | 0→+0.70 |
| M2 BOUNCE | `oblq_macroBounce` | No effect | bounce rate +220ms, gravity +0.2 (faster ricochets) | +220ms/+0.2 |
| M3 COLOR | `oblq_macroColor` | No effect | prism color spread +0.5, prism mix +0.35 | +0.5/+0.35 |
| M4 SPACE | `oblq_macroSpace` | No effect | phaser mix +0.5, prism feedback +0.3 | +0.5/+0.3 |

The OBLIQUE macro story: FOLD cranks the wavefolding grit, BOUNCE accelerates the ricochet rhythm, COLOR multiplies spectral fragments through the 6-tap delay, SPACE sweeps the psychedelic phaser swirl wide.

**Macro strength:** 0/10 → **8/10**

---

## Macro Strength Scores (Post-Fix)

Scoring basis: 0 = no macros; 10 = full-range direct mapping on primary DSP axes.

| Engine | Score | Notes |
|--------|-------|-------|
| ORBITAL | 9/10 | SPECTRUM/SPACE are direct 0→1 full-range; EVOLVE ×5 envelope is dramatic |
| OSTERIA | 8/10 | Strong scaling (0.5–0.8), multi-param sweeps, CHARACTER blend very audible |
| OVERWORLD | 8/10 | ERA crossfade + CRUSH 0.85 + GLITCH 0.9 — all 4 are distinctly audible |
| MORPH | 8/10 | DEPTH (+6000 Hz) and BLOOM (+1.5 morph) are dramatically audible |
| OBLIQUE | 8/10 | FOLD (+0.7) and SPACE (+0.5 phaser) are strongly audible |
| OCELOT | 7/10 | PROWL/ECOSYSTEM strong; FOLIAGE (reverb only) subtler |
| OSPREY | 6/10 | MOVEMENT (LFO + period) is strong; COUPLING (creature depth 0.3) is weak |
| OBSIDIAN | 6/10 | CHARACTER/MOVEMENT adequate; COUPLING (+0.3 cascade) is subtle |

---

## Remaining Weak Macros (Not Fixed — Below Threshold)

These macros are wired but could be strengthened in a future pass:

- **OSPREY M3 COUPLING** (`osprey_macroCoupling` → `creatureDepth +0.3`): The creature voices are already low in the mix. At max macro, the effect is audible but minimal. Recommend increasing to +0.5 or adding a secondary target (e.g., creature rate).
- **OBSIDIAN M3 COUPLING** (`obsidian_macroCoupling` → `cascadeBlend +0.3`): The PD cascade stage 2 is a subtle character difference. Recommend pairing with a filter cutoff offset for a more immediate sweep.

---

## Parameter IDs Added

All new parameters default to `0.0f` — existing presets are completely unaffected.

### OVERWORLD
- `ow_macroEra` — Overworld ERA [0.0, 1.0]
- `ow_macroCrush` — Overworld Crush [0.0, 1.0]
- `ow_macroGlitch` — Overworld Glitch [0.0, 1.0]
- `ow_macroSpace` — Overworld Space [0.0, 1.0]

### MORPH
- `morph_macroBloom` — Morph BLOOM [0.0, 1.0]
- `morph_macroDrift` — Morph DRIFT [0.0, 1.0]
- `morph_macroDepth` — Morph DEPTH [0.0, 1.0]
- `morph_macroSpace` — Morph SPACE [0.0, 1.0]

### OBLIQUE
- `oblq_macroFold` — Oblique FOLD [0.0, 1.0]
- `oblq_macroBounce` — Oblique BOUNCE [0.0, 1.0]
- `oblq_macroColor` — Oblique COLOR [0.0, 1.0]
- `oblq_macroSpace` — Oblique SPACE [0.0, 1.0]

---

## Files Modified

| File | Engine | Change |
|------|--------|--------|
| `Source/Engines/Overworld/OverworldEngine.h` | OVERWORLD | Add 4 macros: ERA, CRUSH, GLITCH, SPACE |
| `Source/Engines/Morph/MorphEngine.h` | MORPH | Add 4 macros: BLOOM, DRIFT, DEPTH, SPACE |
| `Source/Engines/Oblique/ObliqueEngine.h` | OBLIQUE | Add 4 macros: FOLD, BOUNCE, COLOR, SPACE |
| `Docs/macro_audit.md` | — | This document |
