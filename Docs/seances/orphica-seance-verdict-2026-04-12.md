# Seance Verdict: Orphica
**Date:** 2026-04-12
**Convened by:** Claude (Ringleader session)
**Status:** FORMAL NUMERIC SEANCE — supersedes "~8.7 est." on record
**Engine file:** `Source/Engines/Orphica/OrphicaEngine.h`
**Parameter prefix:** `orph_` (frozen)
**Presets:** 264 total (sampled)

---

## Context

Orphica carried a "~8.7 est." designation in the Under Consideration list with notes "Buffer extended; velocity→resonance wired; needs formal numeric seance on record." This seance provides the formal numeric score and documents findings that prevent immediate V1 promotion.

The engine was examined in full (34 parameters, DSP pipeline, coupling output, preset library sample). Two material blockers found: D002 partial fail (no second LFO, no mod matrix) and severe preset schema split (macro dict keys stale fleet-wide).

---

## Ghost Panel Summary

| Ghost | Score | Key Comment |
|-------|-------|-------------|
| **Moog** | 8.5 | "The velocity-to-body-resonance lift — `velBodyLift = vel * 0.45f` shifting the resonant frequency peak — is physically correct harp behavior. The plucked string brightens with harder attacks. But only one drift oscillator and no mod matrix means the performer cannot explore the modulation space live." |
| **Buchla** | 7.5 | "Two FX paths split by a crossover frequency — LOW gets tape saturation and plate reverb, HIGH gets shimmer and spectral smear — this is spatial topology thinking. But there is no modulation architecture at all. A single drift LFO to body damping is one voice. I need at least two independent modulation voices plus a routing matrix before I call this alive." |
| **Smith** | 8.0 | "34 parameters declared, 34 dispatched — D004 clean. But the voice architecture is minimal. Single LFO equivalent, no mod matrix, no per-voice expression beyond the smoothed aftertouch and mod wheel scalars. The OrphicaMicrosound granular buffer at 131072 samples (2.7s) with 4 grain voices and four modes (Stutter/Scatter/Freeze/Reverse) is interesting, but it's not wired into modulation." |
| **Kakehashi** | 8.5 | "PLUCK/FRACTURE/SURFACE/DIVINE — four macros with immediately legible identities. PLUCK brightens and lifts gain. FRACTURE adds microsound scatter. SURFACE shifts crossover bias between the two FX paths. DIVINE deepens shimmer, plate, and spectral smear simultaneously. These are musical controls. The preset library (264 entries) gives a player a world to explore." |
| **Ciani** | 9.0 | "The dual FX topology — LOW path for body warmth (tape saturation, dark delay, deep plate), HIGH path for air and shimmer (shimmer reverb, micro delay, spectral smear, crystal chorus), with SURFACE macro as the crossover dial — this is quadraphonic thinking collapsed to stereo. The spatial character is sophisticated. I want to push SURFACE from 0 to 1 and hear the sound migrate from wood to mist." |
| **Schulze** | 8.5 | "FamilyOrganicDrift modulates body damping at geological rates — the drift can slow to 0.01 Hz, meaning a single cycle takes 100 seconds. That is temporal thinking. But one drift voice is insufficient for long-form evolution. I need at minimum a second independent LFO with its own rate and a mod matrix to route it. Without routing, drift is decoration, not evolution." |
| **Vangelis** | 8.5 | "Aftertouch smoothed 5ms up / 50ms down — a fast attack and slow decay, exactly right for pressure-sensitive expression. `+0.2f` to brightness and `× (1 + at * 0.3f)` to effect intensity. Mod wheel adds `+0.15f` brightness. The expression response is musical. But the performer cannot modulate the microsound modes or grain parameters in real time — that is the missed opportunity." |
| **Tomita** | 8.5 | "Waveguide body with OrphicaMicrosound grain layer, dual FX spectral processing — the timbral architecture spans clean harp to crystal dissolution. Four OrphicaMicrosound modes (Stutter, Scatter, Freeze, Reverse) × four macros × SURFACE crossover gives substantial timbre range for 34 parameters. The orchestral utility is real: plucked strings, prepared strings, spectral strings, textural drones." |

**Consensus Score: 8.4 / 10 — Under Consideration (below V1 threshold)**

---

## Doctrine Compliance

| Doctrine | Status | Commentary |
|----------|--------|------------|
| **D001** Velocity Must Shape Timbre | **PASS** | `velBright = 0.80f + v.vel * 0.20f` (20% brightness range). Body resonance frequency = `v.freq * (0.6 + velBodyLift + pBS * 0.8)` where `velBodyLift = v.vel * 0.45f`. Harder velocity raises body resonant peak — physically correct. |
| **D002** Modulation is the Lifeblood | **PARTIAL FAIL** | `orph_driftRate` / `orph_driftDepth` = single LFO-equivalent modulating body damping. **No second LFO. No mod matrix.** D002 minimum requires 2 LFOs, mod wheel/aftertouch wired, 4 working macros, 4+ mod matrix slots. Macros and expression pass; LFO count and routing fail. |
| **D003** The Physics IS the Synthesis | **N/A** | Waveguide body is an approximation, not a cited physical model. Not claimed. |
| **D004** Dead Parameters Are Broken Promises | **PASS** | All 34 declared parameters dispatched in DSP. No dead parameters found. |
| **D005** An Engine That Cannot Breathe Is a Photograph | **CONDITIONAL PASS** | `orph_driftRate` floor = 0.01 Hz (100-second cycle). One LFO meets the letter of D005. The spirit requires richer autonomous evolution — which D002 also flags. |
| **D006** Expression Input Is Not Optional | **PASS** | Aftertouch (5ms up/50ms down smoothing) → `+0.2f` brightness, `×(1 + at * 0.3f)` effect intensity. Mod wheel → `+0.15f` brightness. Both wired and audible. |

---

## Sonic Identity

**Unique voice:** The fleet's harp and prepared-string engine. Waveguide body resonance + OrphicaMicrosound granular layer creates the intersection of acoustic plucked string and spectral dissolution. No other engine navigates from clean harp to crystal scatter within the same voice architecture.

**Architecture highlights:**
- **Dual FX path with SURFACE crossover**: LOW path (tape sat + dark delay + deep plate) and HIGH path (shimmer reverb + micro delay + spectral smear + crystal chorus) split by a crossover frequency the performer controls with the SURFACE macro. The sound's spatial character is actively morphable.
- **OrphicaMicrosound**: 131072-sample grain buffer (~2.7s) with 4 grain voices, Hann windowing, 4 modes. FRACTURE macro controls `microMix` and `scatterAmt` simultaneously — one gesture moves the sound from clean string to granular cloud.
- **DIVINE macro**: simultaneously deepens shimmer reverb mix, plate reverb mix, and spectral smear amount — one knob migrates from dry body to full spectral dissolution.

**Dry patch quality:** Good. Waveguide body without OrphicaMicrosound gives a clean, warm harp pluck immediately responsive to velocity. The timbre is usable without effects.

**Character range:** `SURFACE: 0.0, FRACTURE: 0.0` (clean harp, wood resonance) vs. `SURFACE: 1.0, FRACTURE: 0.9, DIVINE: 0.8` (crystal scatter, shimmer dissolution) — dramatically different timbres from the same engine.

---

## Preset Review

| Preset | Schema | Assessment |
|--------|--------|------------|
| **Clean Harp** (Foundation) | SPLIT ❌ | macroLabels: PLUCK/FRACTURE/SURFACE/DIVINE ✅ correct. macros dict: CHARACTER/MOVEMENT/COUPLING/SPACE ❌ old keys. Stale params: `orph_filterEnvAmt`, `orph_lfo1Rate`, `orph_velocitySensitivity` — not in addParameters(). |
| **The Final Overtone** (Aether) | SPLIT ❌ | macroLabels: PLUCK/BLOOM/COUPLING/SPACE ❌ old. macros dict: CHARACTER/MOVEMENT/COUPLING/SPACE ❌ old. Stale: `orph_ampAttack`, `orph_filterCutoff`, `orph_reverbMix`, `orph_shimmerAmt`, `orph_macroBloom`, `orph_macroCoupling`, `orph_macroSpace`. |

**Fleet-wide schema status:** The two sampled presets both have stale macro dict keys. The schema split in Orphica is more severe than OpenSky: both the macro names (CHARACTER→PLUCK etc.) and the parameter names (orph_lfo1Rate→orph_driftRate etc.) changed during development. Every Orphica preset likely requires migration.

**Macro effectiveness:** PLUCK, FRACTURE, SURFACE, DIVINE produce distinct audible changes when macros dict is correctly keyed. Macro design is musically clear — Kakehashi would recognize the accessibility. The stale keys are a tooling problem, not a design problem.

---

## Coupling Assessment

**`getSampleForCoupling()`:** Returns only `lastL` / `lastR` (single scalar per channel). No block-length buffer. No envelope channel (ch2). This is the weakest coupling output in the fleet — only instantaneous sample, no buffer-based FM available, no envelope summary for AmpToFilter routing.

**`applyCouplingInput()`:**
| Type | Effect |
|------|--------|
| `AmpToFilter` | Partner amplitude scales body brightness |
| `LFOToPitch` | External LFO modulates pitch |
| `AudioToFM` | Partner audio FM to pitch (uses scalar `externalFM`, not buffer) |

**Natural partners concept:** Onset (drum hits modulate Orphica brightness), Orbital (group envelope modulation), Opal (granular grain scatter sync). The pairing concept is strong but the scalar coupling output limits what partners can receive from Orphica.

**Coupling gap:** getSampleForCoupling upgrade to block-length buffer cache would unlock buffer-based FM for engines receiving from Orphica (matching Overbite's full AudioToFM capability). This is a meaningful improvement, not cosmetic.

---

## Blessings

No new Blessings proposed. The dual FX topology with SURFACE crossover (B-candidate) is sophisticated but not clearly beyond existing Blessing-tier architecture (OpenSky's Shepard shimmer already received B023). If the D002 gap is addressed and the coupling output upgraded, a Blessing for the OrphicaMicrosound + DIVINE macro dissolve pathway would be appropriate for consideration.

---

## Debate Relevance

| Debate | Position |
|--------|----------|
| **DB003** Init patch | Waveguide body at defaults = warm harp pluck. Sides with "immediate beauty" — you get a musical sound immediately. |
| **DB004** Expression vs. Evolution | Sides with **Expression** in the short term (aftertouch + mod wheel wired). Evolution is aspirational — DIVINE macro opens toward long-form dissolution but the single drift LFO limits autonomous evolution depth. Addressing D002 would move this engine meaningfully toward the Evolution pole. |

---

## Recommendations

**Priority 1 — Required for V1 (D002 fix):**
- Add `orph_lfo2Rate` (0.01–10 Hz), `orph_lfo2Depth`, `orph_lfo2Shape` (Sine/Tri/Saw/Square/S&H minimum) to `addParameters()`
- Route LFO2 target to shimmer rate or microsound scatter rate — destinations the performer would hear
- Add 2-slot mod matrix (orph_mod1Src, orph_mod1Dst, orph_mod1Amt, orph_mod2Src, orph_mod2Dst, orph_mod2Amt)
- Minimum viable destinations: body damping, shimmer mix, microsound scatter, macro FRACTURE, crossover frequency

**Priority 2 — Required for V1 (preset schema migration):**
- Rename all preset macro dict keys: CHARACTER→PLUCK, MOVEMENT→FRACTURE, COUPLING→SURFACE, SPACE→DIVINE
- Remove all stale parameter names: `orph_filterEnvAmt`, `orph_lfo1Rate`, `orph_lfo1Depth`, `orph_velocitySensitivity`, `orph_ampAttack`, `orph_filterCutoff`, `orph_reverbMix`, `orph_shimmerAmt`, `orph_macroBloom`, `orph_macroCoupling`, `orph_macroSpace`
- Map legacy values to current parameter names where semantically equivalent (e.g., `orph_lfo1Rate` → `orph_driftRate`)

**Priority 3 — Post-D002 fix, pre-V1:**
- `getSampleForCoupling()` upgrade: cache block-length buffer for AudioToFM, add envelope ch2 for AmpToFilter. This brings Orphica coupling output to fleet standard.

**Priority 4 — Post-V1:**
- Consider Blessing candidate for OrphicaMicrosound + dual FX dissolve pathway once D002 is resolved

---

## Score History

| Date | Score | Context |
|------|-------|---------|
| Pre-2026-04-12 | "~8.7 est." | Informal estimate; buffer extended, velocity→resonance wired, formal seance not run |
| **2026-04-12** | **8.4/10** | Formal seance. D002 partial fail (no LFO2, no mod matrix). Preset schema split (macro keys + stale params). getSampleForCoupling weakest in fleet. Under Consideration — requires D002 fix + schema migration before V1. |
