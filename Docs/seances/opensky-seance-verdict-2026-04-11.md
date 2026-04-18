# Seance Verdict: OpenSky
**Date:** 2026-04-11
**Convened by:** Claude (Ringleader session)
**Status:** FORMAL NUMERIC RE-SEANCE — supersedes partial 8.1/10 score (2026-03-xx)
**Issue:** #1080

---

## Context

OpenSky was previously listed as "Under Consideration" with a partial score of 8.1/10. Two blockers were noted:

1. **sky_subWave D004 partial** — investigated and confirmed FALSE POSITIVE. The parameter is fully dispatched at lines 841–853 of `OpenSkyEngine.h` (Sine/Triangle/Square via `switch(subWaveIdx)`). No bug.
2. **No formal numeric re-seance on record** — resolved by this document.

A third finding emerged mid-seance: **preset schema split** — approximately one-third of presets use an older parameter vocabulary (`sky_filterFc`, `sky_macroCharacter`, `sky_ampAttack`, etc.) that no longer matches engine parameter IDs. These presets load at parameter defaults. Logged separately as a post-seance issue.

---

## Ghost Panel Summary

| Ghost | Score | Key Comment |
|-------|-------|-------------|
| **Moog** | 8.5 | "That RISE macro is voltage-controlled gesture in digital form. The filter tracks velocity like a well-calibrated ladder. Aftertouch to shimmer is the most natural connection I've seen in this fleet." |
| **Buchla** | 8.0 | "The Shepard shimmer architecture is West Coast in spirit — pitch-transposed feedback with octave balance control. My concern: two mod matrix slots is not a patch system. I want four." |
| **Smith** | 8.5 | "Eight voices, PolyBLEP supersaw, voice stealing with 5ms crossfade, AudioToFM coupling type. The polyphony is solid and the MIDI integration is clean. I'd have added a third filter mode." |
| **Kakehashi** | 8.0 | "RISE, WIDTH, GLOW, AIR — these are immediately musical macro names. When the presets work, this is immediately accessible. The broken old-schema presets need to be fixed before public release." |
| **Ciani** | 9.0 | "The way the shimmer breathes across the stereo field — shimmerFeedback building over held notes, unison spread painting width — this is spatial synthesis. The Shepard architecture creates vertical space." |
| **Schulze** | 9.0 | "Cloud Atlas: 3.0s attack, 6.0s release, sub-0.01Hz LFOs, feedback shimmer accumulating over time. This is the correct timescale for cosmic music. OpenSky breathes over minutes, not milliseconds." |
| **Vangelis** | 8.5 | "RISE pushes pitch upward and opens the filter simultaneously. Aftertouch deepens the shimmer. This is how expression should work — the instrument responds to pressure with warmth, not just volume." |
| **Tomita** | 8.0 | "417 presets with atmospheric naming — Stratosphere, Noctilucent, Crepuscular Ray, Exosphere. The cinematic range is there. But the broken old-schema presets thin the usable palette." |

**Consensus Score: 8.5 / 10 — V1 PASS**

---

## Doctrine Compliance

| Doctrine | Status | Commentary |
|----------|--------|------------|
| **D001** Velocity Must Shape Timbre | **PASS** | `velCutoffMod = voice.velocity * velFilterEnv * 8000.0f` at render time (line 878). Velocity shapes filter brightness AND amplitude (`gain = envVal * voice.velocity`). `sky_velSensitivity` is user-adjustable. |
| **D002** Modulation is the Lifeblood | **PASS** | LFO1 → pitch (±2st), LFO2 → subtle pitch vibrato. Mod wheel (CC1) → filter cutoff ±6000Hz. Aftertouch → shimmerMix +0.4. 4 macros fully wired. 2 mod matrix slots. Minimum met; 4-slot expansion recommended. |
| **D003** The Physics IS the Synthesis | **N/A** | OpenSky is a supersaw + shimmer + chorus architecture. No physical modeling claimed. D003 does not apply. |
| **D004** Dead Parameters Are Broken Promises | **PASS (engine)** / **FLAG (preset library)** | All 45 engine parameters are dispatched. `sky_subWave` (previously flagged) is fully dispatched via `switch(subWaveIdx)` at lines 841–853. Preset schema split (1/3 of presets use stale param names) logged separately — engine is clean. |
| **D005** An Engine That Cannot Breathe | **PASS** | `SkyBreathingLFO` with rate floor ≤ 0.01 Hz. LFO1 at 0.01–0.08 Hz in working presets. Breath modulates `filterCutoff ± 2000Hz`. `flushDenormal()` called after pitch envelope integration. |
| **D006** Expression Input Is Not Optional | **PASS** | Velocity → filter brightness (D001). Mod wheel (CC1) → filter cutoff. Aftertouch → shimmerMix. Pitch bend → ±2 semitones. Full expression toolkit. |

---

## Sonic Identity

**Unique voice:** OpenSky is the fleet's dedicated shimmer-atmosphere engine. The 7-saw PolyBLEP supersaw through an SVF filter into the Shepard shimmer reverb produces a vertical, expanding texture that no other fleet engine replicates. A held chord grows upward in perceived pitch as the feedback shimmer accumulates — the room gets taller.

**Dry patch quality:** The supersaw into SVF filter sounds compelling without shimmer. The sub oscillator (sine/triangle/square) grounds the texture. At moderate unison counts (3–5) with spread, the dry sound alone has body and character.

**Character range:** From a single unison voice with minimal shimmer (crystalline, precise, almost digital) to 7 unison voices with full shimmer feedback and 6-second release (diffuse, atmospheric, impossible to localize spatially). Two presets from this engine can sound as different as a guitar harmonic and a cloud.

**Init patch quality:** Default parameter values (filterCutoff: 6000Hz, shimmerMix: 0.3, shimmerFeedback: 0.4, unisonCount: 1, attack: 0.01s, release: 0.3s) produce an immediately inviting sound — bright, clear, shimmer-kissed. Not a blank canvas.

---

## Preset Review

**417 total presets** across Atmosphere (74), Family (28), Ethereal (2), Foundation (approx. 250 more across all moods, with GB_ prefix presets in most). Well above the 100-preset V1 threshold.

| Preset | Schema | Assessment |
|--------|--------|------------|
| **Stratosphere** | NEW ✅ | Excellent use of new schema. High shimmerFeedback (0.6), high shimmerOctave (0.8), long attack (2.0s), release (5.0s). All 4 macros functional. DNA accurate (brightness 0.8, space 0.85). |
| **Cloud Atlas** (Ethereal) | NEW ✅ | Definitive OpenSky showcase. 5 unison voices, sawSpread 0.95, shimmerMix 0.88, shimmerFeedback 0.72. Mod matrix active: LFO1→shimmer, LFO2→filter. macroRise/Width/Glow/Air all meaningful. |
| **Aftertouch Glow** | NEW ✅ | Well-named. Demonstrates aftertouch→shimmer expressivity. Correct macro labels (RISE/WIDTH/GLOW/AIR). |
| **Noctilucent** | OLD ⚠️ | Uses sky_filterFc, sky_ampAttack, sky_macroCharacter/Movement/Coupling/Space. Loads at defaults. Great concept, broken parameters. |
| **Clear Horizon** | OLD ⚠️ | Old schema. sky_filterFc, sky_reverbMix, sky_shimmerOct (integer). Will load at defaults. |
| **First Light** | OLD ⚠️ | Old schema. sky_filterFc, sky_shimmerBright — neither exist in current engine. Foundation of the preset library; needs migration urgently. |

**Schema split assessment:** New-schema presets are excellent quality with meaningful macro assignments and accurate DNA. Old-schema presets are conceptually rich but functionally broken at load time. Migration is a maintenance task, not an architectural change — parameter name remapping via a Python script.

**Macro effectiveness (new-schema presets):**
- **RISE:** Pitch envelope sweep + filter opening — audible and expressive ✅
- **WIDTH:** Chorus depth + unison spread — clear stereo widening ✅
- **GLOW:** shimmerSize + shimmerFeedback — audible shimmer intensity ✅
- **AIR:** shimmerSize + filter opening — sky-brightness control ✅

---

## Coupling Assessment

**`getSampleForCoupling()`** outputs: stereo audio (channels 0/1) + peak envelope (channel 2). The envelope output is particularly useful as a dynamic modulation source for partner engines.

**`applyCouplingInput()`** handles:
| Type | Effect | Notes |
|------|--------|-------|
| `AmpToFilter` | `externalFilterMod += amount * 6000Hz` | Strong filter modulation from partner amplitude |
| `LFOToPitch` | `externalPitchMod += amount * 0.5st` | Subtle pitch drift from partner LFO |
| `AmpToPitch` | Same as LFOToPitch | Amplitude-to-pitch for FM-adjacent texture |
| `PitchToPitch` | Same as LFOToPitch | Voice tracking between engines |
| `AudioToFM` | `externalFMMod += amount * 0.3` | ±4 semitones FM at full amount |

**Natural pairing partners:**
- **Opal (granular):** Opal's grain cloud amplitude modulating OpenSky's filter = shimmer gating
- **Osprey/Osteria (ShoreSystem):** Tidal rhythm amplitude driving OpenSky pitch = breathing supersaw
- **Ouroboros (chaos):** Lorenz attractor pitch output to OpenSky AudioToFM = deterministic chaos FM shimmer
- **Organon (metabolism):** Metabolic rhythm driving OpenSky filter = organic shimmer pulsing

**Entangled preset quality:** Family mood presets (Sky_x_Shore_Shimmer, Sky_x_Reef_Shimmer, etc.) demonstrate coupling routes. Schema status varies — newer Family presets appear to use new schema.

---

## Blessings

**B023 — Shepard Shimmer Architecture** (previously awarded) ✅ Confirmed. Pitch-transposed FDN shimmer reverb with `shimmerOctBal` parameter for balancing octave-up vs octave-down contributions creates an indefinitely-rising texture under held notes. Genuinely novel in this fleet.

**B024 — RISE Macro** (previously awarded) ✅ Confirmed. Bipolar pitch-envelope macro simultaneously controls pitch envelope amount (±12st), filter cutoff (±6000Hz), and shimmerMix (±0.3). A single gestural control that defines the engine's personality.

**No new Blessings this seance.** The existing two accurately describe the engine's achievements.

---

## Debate Relevance

| Debate | Relevance |
|--------|-----------|
| **DB001** Mutual exclusivity vs. effect chaining | Shimmer + chorus run in series (shimmer first, chorus after). Not mutually exclusive — but shimmer at high feedback can mask chorus character. A MACRO balance parameter (shimmer/chorus proportion) would resolve this naturally. |
| **DB003** Init patch: immediate beauty vs. blank canvas | OpenSky has a good init patch — shimmer-kissed, clear, inviting. Sides with "immediate beauty." |
| **DB004** Expression vs. Evolution | **OpenSky resolves DB004.** The RISE macro enables immediate gestural expression; the 0.01Hz breath LFO and long-release shimmer enable temporal evolution. This is one of the few engines that serves both master performers and ambient composers equally. The DB004 tension dissolves here. |

---

## Recommendations

**Priority 1 — V1 Prerequisite:**
- **Preset schema migration** (logged as separate issue): Remap all old-schema presets (`sky_filterFc` → `sky_filterCutoff`, `sky_macroCharacter` → `sky_macroRise`, `sky_ampAttack` → `sky_attack`, etc.) via Python migration script. Test that all 417 presets load non-default values after migration.

**Priority 2 — Path to 8.8+:**
- Expand mod matrix from 2 to 4 slots (Buchla's main objection; D002 minimum is met, but richer routing would elevate the score)
- Consider a third filter mode (e.g., notch or formant) — Smith requested this

**Priority 3 — Nice to have:**
- Add a dedicated macro layer for sub oscillator level (CHARACTER or RISE could incorporate sub blend)
- Consider shimmer/chorus balance parameter to address DB001 more cleanly

---

## Final Verdict

**Score: 8.5 / 10 — FORMAL V1 PASS**

OpenSky is the fleet's atmospheric sky synthesizer. The Shepard shimmer architecture (B023) and RISE macro design (B024) are genuine innovations. Full doctrine compliance, 417 presets, 4 coupling types, and a distinct sonic identity that no other engine replicates make it a clear V1 candidate.

**V1 Prerequisite:** Preset schema migration must complete before public release. The migration is a maintenance task (parameter name remapping), not an architectural change. Engine DSP is clean and complete.

**Ghost consensus:** "Every time you hold a note, the room grows taller." — Buchla (confirmed; this is still the definitive description.)

---

## Score History

| Date | Score | Context |
|------|-------|---------|
| 2026-03-xx | 8.1/10 | Partial seance; sky_subWave flagged as D004 partial (now confirmed false positive) |
| **2026-04-11** | **8.5/10** | Formal re-seance. D004 false positive cleared. Preset schema split identified and logged separately. V1 PASS. |
