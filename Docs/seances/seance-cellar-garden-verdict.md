# The Verdict — CELLAR & GARDEN Quads

**Seance Date**: 2026-03-21
**Collections**: CELLAR (4 bass engines) + GARDEN (4 string engines) = 8 engines total
**Collection Theme**: The Kitchen / The Growing World
**Fleet Average**: ~8.7 (post-2026-03-21 fixes)

---

## CELLAR QUAD OVERVIEW

Four engines spanning the complete bass spectrum: sub (OGRE), analog (OLATE), acoustic (OAKEN), FM digital (OMEGA). All share MIDI-domain gravitational coupling. Architecture document in `Docs/cellar-quad-architecture.md`.

---

# ENGINE 1: OGRE (XOgre — "The Root Cellar")

**Identity**: Sub bass. Sine + triangle oscillators, sub-harmonic generator, tanh saturation, body resonance filter, tectonic LFO.
**Parameter prefix**: `ogre_`
**Params**: ~27
**Presets**: 10 (Foundation: 4, Atmosphere: 2, Flux: 1, Organic: 1, Entangled: 1, Deep: 1)
**Accent**: Deep Earth Brown `#4A2C0A`

---

## The Council Has Spoken — OGRE

| Ghost | Core Observation |
|-------|-----------------|
| **Moog** | XOgre is exactly what sub bass synthesis should be: physically honest about where it lives. The sine-to-triangle crossfade (`ogre_oscMix`) is the correct pair — sine for pure fundamental weight, triangle for the first harmonic addition that makes it more than a tone generator. The sub-harmonic generator using a zero-crossing toggle is classical frequency division and it works. The tectonic LFO is this engine's true identity: pitch drift at 0.005-0.5 Hz is the geological timescale, and calling it "continental drift" is not metaphor — it is the perceptual psychology of a bass note that does not quite hold still. This is the correct emotional register for a sub bass engine. My concern is the DSP: the body filter is computed twice per sample. Line 388 runs `bodyFilter.processSample(saturated)` and assigns to `filtered`. Lines 391-395 then recompute the filter coefficients (envelope-modulated cutoff) and run `bodyFilter.processSample(saturated)` again, overwriting `filtered`. The first pass with soil character is discarded. The `ogre_soil` parameter has zero effect on audio output. This is a D004 violation. |
| **Buchla** | I want to defend the tectonic LFO rate floor at 0.005 Hz — that is a 3.3-minute cycle. Most synthesizers would call this a malfunction. XOgre calls it geology. The naming earns this choice. However, the density parameter is weak in its implementation. The spec promises "infrasound sub-frequency emphasis" at extreme low frequencies, but the implementation is a secondary LP filter on the sub signal with cutoff at 40-60 Hz and the output added back at only `densNow * 0.5f`. At 48kHz this is barely perceivable — a small bump in the 40-60 Hz region that producers working on laptop speakers will never hear. The density parameter should be bolder or it should be renamed. |
| **Kakehashi** | The preset library is strong for 10 presets. "Seismic Root" (`ogre_rootDepth=0.8`, clay soil, slow tectonic) and "Potato Stomp" (percussive, fast decay, `ogre_sustain=0`) demonstrate the range well — from sustained earthquake foundation to rhythmic sub-bump. The gravitational coupling parameter is exposed in presets (gravity=0.4-0.6 range) which is correct practice. The Organic preset "Peasant Stew" explores the body resonance territory. Coverage is appropriate. My concern: no preset uses `ogre_tectonicDepth` above 3.0 cents. The maximum is 20 cents. There should be at least one preset with deep tectonic movement (15-20 cents) to show this parameter's full character. |
| **Smith** | The sub-harmonic generator is a square wave derived from zero crossings, low-pass filtered at 80 Hz. This is the correct approach — the filtered square approximates a sine at the octave-below frequency. The one-pole coefficient is correctly computed using the matched-Z transform (`exp(-2πfc/sr)`). However: the sub filter (`subFilter`) is a separate CytomicSVF instance that filters the combined signal again at 40-60 Hz, not the sub-harmonic alone. This causes the main oscillator signal to also be low-pass filtered by the sub emphasis stage, darkening the full mix rather than just boosting sub content. This is a minor tonal artifact but it is not what the spec describes. The sub filter should apply only to the sub-harmonic signal before it is mixed with the main oscillator via `rootDepth`. |
| **Schulze** | XOgre interests me precisely because it has no ambition beyond its mandate. It is a sub bass engine and it does not try to be a pad or a lead. This restraint is strength. The tectonic LFO with its geological metaphor creates the correct emotional frame: the listener should feel this engine as immovable earth, not as a synthesizer making sounds. The soil parameter (`ogre_soil`) is a good idea poorly executed — the distinction between clay, sandy, and rocky should be unmistakable, but the body filter bug means it is currently inaudible. The "rocky" mode (BandPass character from pSoil > 0.5) is the most interesting because it introduces mid-frequency notch character into what is otherwise a pure low-frequency instrument. This deserves to work. |
| **Vangelis** | I approach this engine from the listener's body, not the engineer's graph. Sub bass is felt before it is heard. XOgre understands this: the density parameter is trying to reach infrasound, the tectonic LFO is trying to make the note breathe at a timescale below conscious perception. What I want from this engine is the feeling that the ground is not quite still. The presets need at least one that captures that: a held note that develops slowly, where the tectonic drift is audible only after 30-60 seconds of listening — not as pitch instability but as the sense that the room has changed. "Continental Drift" (Atmosphere) attempts this but the tectonic depth is only 3 cents. Go to 12-15 cents and the effect becomes physical. |
| **Tomita** | The absence of any stereo processing concerns me slightly for a bass engine. The pan spread for bass voices (`(idx - 3.5) * 0.03`) is essentially zero — ±3% stereo placement. This is appropriate for sub frequencies (sub bass should be mono below 80 Hz) but the engine makes no distinction: `ogre_brightness` can reach 20kHz, meaning high-resonance presets with boosted brightness would benefit from stereo width that currently does not exist. The `ogre_bodyResonance` filter at high settings adds a resonant character that sounds different per voice, but the voices are panned nearly identically. This is not a flaw for the primary use case but limits textural variation in polyphonic contexts. |

---

## Doctrine Compliance — OGRE

| Doctrine | Status | Notes |
|----------|--------|-------|
| D001 | PASS | Velocity scales both filter brightness (`brightNow + velocity * 3000`) and saturation gain magnitude via voice.velocity output multiplier |
| D002 | PASS | 2 LFOs + tectonic LFO + mod wheel (→drive) + aftertouch (→body resonance) + 4 macros |
| D003 | N/A | No physical modeling claimed |
| D004 | **FAIL** | `ogre_soil` has zero effect on audio due to double-filter bug. First `bodyFilter.processSample(saturated)` result is discarded; second run overwrites it with envelope cutoff. Soil character computation is wasted. |
| D005 | PASS | Tectonic LFO floor at 0.005 Hz (well below 0.01 Hz requirement) |
| D006 | PASS | Mod wheel → drive, aftertouch → body resonance |

---

## Scores — OGRE

| Dimension | Score | Notes |
|-----------|-------|-------|
| Concept Clarity | 9.0 | "Earthquake bass" is immediately legible |
| DSP Soundness | 6.5 | Body filter double-pass bug (D004 violation), sub filter architectural issue |
| Uniqueness | 8.0 | Tectonic LFO + soil typology is genuinely distinct |
| Expressiveness | 7.5 | MW→drive, AT→body res, tectonic are all there; soil being dead hurts |
| Preset Quality | 7.5 | Good range but no extreme tectonic depth explored |
| Doctrine Compliance | 7.0 | D004 fail on ogre_soil |
| Fleet Fit | 9.0 | No other sub bass engine in fleet |
| Emotional Weight | 8.5 | When it works, it physically lands |

**OVERALL SCORE: 7.9 / 10**
*Would be 8.5+ with soil bug fixed*

---

## Top 3 Concerns — OGRE

1. **P0 — D004 Soil Bug**: `ogre_soil` is dead. Lines 388 + 394-395 run the body filter twice; the first pass with soil character is discarded when the second pass (envelope cutoff) overwrites `filtered` running `bodyFilter.processSample(saturated)` again. Fix: apply soil character to cutoff computation *before* the envelope cutoff calculation, or apply both in a single filter pass using the combined cutoff.
2. **P1 — Sub Filter Architecture**: The density emphasis filter applies to the full combined signal, not the sub-harmonic alone, darkening the full mix. Should filter only the sub before the `rootDepth` mix.
3. **P2 — Tectonic Depth Presets**: Maximum tectonic depth (20 cents) is never used in any preset. Add one preset demonstrating the engine's geological extreme.

## Top 3 Enhancements — OGRE

1. **Blessing Candidate**: The Tectonic LFO concept — geological timescale pitch drift as a musical parameter — is novel and sonically verifiable. Needs no new code; needs its bug fixed and then deserves recognition. Consider B039 "Tectonic Timescale: pitch drift at geological rate as a bass synthesis parameter" after the soil fix validates the surrounding context.
2. **Soil Stereo Expansion**: At "rocky" mode (bandpass body filter), introduce slight stereo width from the body resonance stage — the rocky/bandpass character could benefit from spatial treatment that clay/sandy (LP) would not have.
3. **Infrasound Density Mode**: At `ogre_density` > 0.8, introduce a very low frequency (12-20 Hz) additional boost to the sub, creating true infrasound presence that is felt rather than heard. Currently the density sub-emphasis tops out at 60 Hz.

## Blessing Candidates — OGRE

- **B039 Candidate**: Tectonic LFO (geological timescale pitch drift). *Conditional: requires soil bug fix first to establish the engine's credibility.*

---

# ENGINE 2: OLATE (XOlate — "The Aged Wine Analog")

**Identity**: Analog subtractive bass. Saw + variable-width pulse, ladder filter, fermentation integrator, session aging, vintage era + terroir.
**Parameter prefix**: `olate_`
**Params**: 30
**Presets**: 10 (Foundation: 3, Flux: 2, Atmosphere: 2, Entangled: 1, Deep: 1, Organic: 0)
**Accent**: Burgundy `#6B1A2A`

---

## The Council Has Spoken — OLATE

| Ghost | Core Observation |
|-------|-----------------|
| **Moog** | The FermentationIntegrator is the most interesting analog modeling idea in this engine, and I have complicated feelings about it. Real analog bass tones do not become harmonically richer during sustain — they become harmonically simpler as the string or resonator settles. The fermentation metaphor inverts this: "fermented bass starts simple and grows rich." This is the correct creative inversion. The implementation uses a leaky integrator driving progressive tanh saturation, which does genuinely add harmonics over sustain. Musically, this creates a characteristic that is not analog — no Minimoog does this — but it is musically interesting. The `olate_ageRate` control makes it fast or slow. My concern is the normalization: `fastTanh(input * driveScale) / fastTanh(driveAmount)`. This assumes `fastTanh(driveAmount)` is the maximum output, but when `input` is larger than 1.0 (which it can be from the oscillator summing), this normalization fails. The saturation output can exceed 1.0 if the input signal is hot. |
| **Buchla** | Terroir is the most culturally specific parameter in the CELLAR quad and I want it to succeed. The concept — West Coast cool, East Coast grit, UK mid-forward, Japanese transparent — is geographically legible and musically meaningful. But the implementation has a serious dead parameter problem. The code handles only two cases: West Coast (pTerroir 0.01-0.4, slight amplitude reduction) and UK (pTerroir 0.4-0.7, mid boost via lfo2Val). East Coast (pTerroir 0.7-1.0) and Japanese transparent (pTerroir = 1.0) have no DSP code. A user moving `olate_terroir` from 0.7 to 1.0 hears nothing change. This is a partial D004 violation. East Coast should have something — more harmonic distortion, more high-frequency presence. Japanese transparent should be the cleanest setting, perhaps with less filter resonance. |
| **Kakehashi** | The session aging model is clever: over 20 minutes of play, the cutoff drops by 800 Hz as the "amp warms up." But there is a player experience problem: this effect is imperceptible for the first 10 minutes, and by the time it is perceptible (after 15+ minutes), the player has likely switched presets or sessions. The decay is too slow to be a compositional tool and too subtle to be experienced. I would prefer either making the aging dramatically faster (3-5 minutes to full aging) or exposing an `olate_sessionInstant` parameter that lets the player dial in the aged state directly. "Fresh from the cellar" vs "aged 20 years" should be an immediate performance option, not a waiting game. |
| **Smith** | The `olate_warmth` low shelf filter is correctly specified but the gain value deserves scrutiny. `voice.warmthFilter.setCoefficients(200.0f, 0.5f, srf, warmNow * 6.0f)` — 6 dB maximum boost at 200 Hz. At high warmth settings combined with high drive and fermentation, the bass frequencies accumulate: oscillator fundamentals (40-120 Hz) + fermentation saturation + 6 dB shelf boost. The engine can easily clip its output without any headroom protection. The output stage needs either a limiter or the gain staging needs to account for the fermentation-plus-warmth accumulation. The `filtered * ampLevel * voice.velocity` output has no ceiling beyond the filter's output range. |
| **Schulze** | The vintage parameter's implementation is correct in concept but limited in granularity. Four discrete character regions (transistor, Moog, 303, modern) with step-change behavior at 0.25/0.5/0.75 boundaries means the transition between vintage eras is audible as a jump, not a smooth evolution. A Moog bass at vintage=0.49 sounds different from vintage=0.51 — not by a small amount but by a step change in `vintageResoBoost` and `vintageDriveBoost`. Players who automate this parameter will hear the step. The vintage parameter should use a blend or crossfade across the threshold regions. |
| **Vangelis** | "Aged Wine Analog" is an identity that lives in time, and XOlate does engage with time in multiple ways: fermentation grows during sustain, session aging drifts over 20 minutes, the terroir suggests a geographic origin that pre-dates the session. This is layered temporal identity and I respect it. The emotional register is "warmth accumulating" — not explosive or aggressive but patient and deepening. The presets capture this: "Burgundy Reserve" is a held note that opens slowly, "Aged Tannin" is a darker version with more roll-off. The musical use case is evident: XOlate is the bass that sounds better in the second half of the session than the first. |
| **Tomita** | From an orchestral perspective, XOlate is the low string section found in a mid-century recording — warm, slightly compressed, with the character of a specific studio. The terroir concept maps well to this: different recording studios had different tonal signatures. West Coast studios (Capitol, Western Recorders) tended toward a clean low end; East Coast (Bell Sound, RCA) had more harmonic complexity; UK (Abbey Road, Olympic) had the distinctive mid presence. The Japanese concept maps to the Nippon Columbia/Victor studios — extremely clean, almost no coloration. The implementation for East Coast and Japanese needs to reflect these real differences. |

---

## Doctrine Compliance — OLATE

| Doctrine | Status | Notes |
|----------|--------|-------|
| D001 | PASS | Velocity scales filter cutoff (`velCutMod = velocity * 3000`) |
| D002 | PASS | 2 LFOs + mod wheel (→cutoff) + aftertouch (→resonance) + 4 macros + session age |
| D003 | N/A | Analog emulation, not physical modeling |
| D004 | **PARTIAL FAIL** | `olate_terroir` is dead for values 0.7-1.0 (East Coast + Japanese regions). Two of four terroir regions have no DSP implementation. |
| D005 | PASS | LFO floor at 0.005 Hz |
| D006 | PASS | Mod wheel → cutoff, aftertouch → resonance |

---

## Scores — OLATE

| Dimension | Score | Notes |
|-----------|-------|-------|
| Concept Clarity | 8.5 | Aged wine as analog bass is immediately evocative |
| DSP Soundness | 7.0 | Normalization risk in fermentation, output level accumulation, terroir gap |
| Uniqueness | 8.5 | FermentationIntegrator + session aging is genuinely novel |
| Expressiveness | 8.0 | MW/AT/macros all meaningful; terroir dead for 30% of its range |
| Preset Quality | 8.0 | Good range, covers analog sub territory well |
| Doctrine Compliance | 7.5 | D004 partial fail on terroir East Coast/Japanese |
| Fleet Fit | 8.5 | Most classic-analog of the four bass engines |
| Emotional Weight | 8.5 | Patient, accumulating warmth is a distinct emotional register |

**OVERALL SCORE: 8.1 / 10**

---

## Top 3 Concerns — OLATE

1. **P0 — Terroir D004 Violation**: East Coast (pTerroir 0.7-1.0) and Japanese transparent (1.0) are unimplemented. Two of four geographic regions have no DSP code. Fix: East Coast = add high-harmonic presence boost (saturate pre-filter at low drive, or HP shelf boost around 2-4kHz); Japanese = reduce filter resonance scaling, remove drive boost, present the cleanest possible signal.
2. **P1 — Output Level Accumulation**: Fermentation + warmth shelf + drive can stack without headroom protection. Add a soft limiter or gain normalization before the voice output to prevent inter-sample peaks above 1.0.
3. **P2 — Vintage Step Function**: The vintage character changes are step-changes at 0.25/0.5/0.75 boundaries. Smooth these with linear blending across a ±0.05 band around each threshold.

## Top 3 Enhancements — OLATE

1. **Session Aging Curve Adjustment**: Compress the aging timeline from 20 minutes to 5-7 minutes, making it a perceptible compositional tool within a typical recording session.
2. **Terroir Completion**: Implement East Coast grit (pre-filter saturation boost, slight presence shelf) and Japanese transparent (minimal resonance, no drive, ultra-clean signal path).
3. **Fermentation Preset Showcase**: Add one preset specifically designed to demonstrate fermentation over a long held note — `olate_ageRate=1.0`, slow attack, very long sustain. This feature needs a signature preset to be discovered.

---

# ENGINE 3: OAKEN (XOaken — "The Cured Wood")

**Identity**: Acoustic/upright bass. Karplus-Strong string synthesis, 3-mode playing styles (pluck/bow/slap), wood body resonator, curing model.
**Parameter prefix**: `oaken_`
**Params**: 29
**Presets**: 10 (Foundation: 3, Flux: 1, Organic: 1, Entangled: 1, Atmosphere: 2, Deep: 1, Luminous: 1)
**Accent**: Dark Walnut `#3D2412`

---

## The Council Has Spoken — OAKEN

| Ghost | Core Observation |
|-------|-----------------|
| **Moog** | The Karplus-Strong implementation in OakenKarplusString has a subtle but significant problem in setStringType. The feedback LP coefficient uses `1.0f / (1.0f + 1.0f / (fc * 0.00005f))` — this is an Euler-approximation first-order LP, not a matched-Z transform. For string synthesis, where the LP coefficient controls the harmonic decay rate, this approximation introduces frequency-dependent error. At gut string tension (fc = 1000 Hz), the coefficient is `1/(1 + 20) ≈ 0.048` — which means the LP filter barely passes anything, creating extremely fast decay that does not match gut string behavior. This should use `1 - exp(-2πfc/sr)` for the coefficient. The actual string behavior is dominated by the feedback loop LP in `readFromDelay()` which uses `stringLPCoeff`, not `feedbackGain`. These two paths are tangled and the string type (gut/steel/synthetic) is not having the effect the spec describes. |
| **Buchla** | The three-exciter model is physically informed and musically distinct. Pluck (decaying noise burst), bow (sustained LP-filtered noise with pressure control), slap (click + noise burst) — these are the right trio for upright bass. The bow exciter's sustained noise injection through a LP filter with velocity-derived cutoff is correct modeling: faster bow strokes on a real bass are brighter, and `fc = 400 + velocity * 1200` captures this range (400-1600 Hz LP cutoff). What is missing is the bow pressure modulation during sustain. In real bowing, pressure changes the string's response — too light and it squeaks, too heavy and it chokes. The mod wheel maps to bow pressure (D006 compliant) but this only affects the amplitude of noise injection. The tonal character of bow pressure should also shift — high pressure should harden the attack and add harmonics, low pressure should produce the eerie harmonic (flageolet) mode. |
| **Kakehashi** | The curing model is this engine's unique innovation and it is sonically verifiable. The LP cutoff dropping from 12kHz to ~1kHz over the note's sustain creates audible darkening — a note that starts bright and ends as a fundamental-heavy, dull tone. This is "wood drying" as a synthesis metaphor and it is physically plausible (drying wood does change resonant properties over long timescales). The `oaken_curingRate` parameter controls how fast this happens. For bass production, this creates a useful character: slap bass notes start bright and crisply cut through the mix, then settle into warm thickness. The curing model is doing real DSP work. My concern is that none of the presets push `oaken_curingRate` above 0.5 — the extreme curing (rapid darkening from bright to pure fundamental) is unexplored. |
| **Smith** | The body resonator's three-mode BPF bank (180-220 Hz, 520-600 Hz, 1000-1200 Hz) correctly captures the first three resonant modes of a typical upright bass body. These frequencies are physically accurate: an upright bass body resonance typically falls around 185-200 Hz (Helmholtz air resonance), 520-540 Hz (first plate mode), and 950-1100 Hz (second plate mode). The wood age modulation of these frequencies (old wood shifts them lower) is directionally correct. The Q values (0.4-0.7) produce audible coloration without excessive ringing. The body resonator is the most physically grounded DSP in the CELLAR quad and it validates D003 compliance. The curing model's citation (Fletcher & Rossing 1998) is appropriate, and Karplus & Strong (1983) / Smith (1992) are correct references. D003 is genuinely earned. |
| **Schulze** | From the perspective of long-form composition, XOaken is the most temporally interesting bass engine. It develops over time at three different scales: the curing model (seconds, within the note), the session aging via curing age accumulation (minutes, across notes), and theoretically the Mycorrhizal network (absent here — OAKEN does not implement GardenAccumulators). This gives it a "lived-in" quality that the other three CELLAR engines do not have. The slap exciter is particularly appealing: the sharp click + noise burst creates a transient that carries rhythmic information beyond just pitch, and the decay into the body resonator produces a "woody" character that acoustic bass players recognize immediately. |
| **Vangelis** | I play XOaken in my mind and I hear a jazz trio at 2 AM — the bassist's fingers, the hollow body, the room. The engine captures the gestural quality of acoustic bass: you can feel the difference between pluck and bow in how the note begins. The bow mode with sustained noise injection creates the "infinite sustain" of a well-bowed string, which XOgre's sub cannot offer and XOlate's analog filter cannot replicate. The emotional register here is human — something played by hands, not generated by circuits. This is a rare quality in synthesis. The wood body resonator adds the room: it is not a reverb but a character, the specific frequencies that a hollow wooden instrument body emphasizes. |
| **Tomita** | The room parameter (`oaken_room`) is currently implemented as only panning spread (`pRoom * 0.15` stereo offset per note). This is not a room — it is a width control. "Studio → jazz club → concert hall" is what the room parameter promises, but all that changes with pRoom is how spread the voices are across the stereo field (by ±7.5% at maximum). A real room distinction needs different reverb time, different early reflection density, and different frequency coloration. Even a simple allpass diffuser section with time proportional to pRoom would substantially improve this. As implemented, the room parameter is misleadingly named. |

---

## Doctrine Compliance — OAKEN

| Doctrine | Status | Notes |
|----------|--------|-------|
| D001 | PASS | Velocity scales exciter energy and brightness cutoff (`velocity * 2000`) |
| D002 | PASS | 2 LFOs + mod wheel (→bow pressure) + aftertouch (→damping) + 4 macros |
| D003 | PASS | Karplus & Strong (1983), Smith (1992), Fletcher & Rossing (1998) all cited and implemented |
| D004 | **PARTIAL FAIL** | `oaken_room` claims "studio → jazz club → concert hall" but only implements stereo spread. Room timbre, reverb character, and early reflection density are absent. |
| D005 | PASS | LFO floor at 0.005 Hz |
| D006 | PASS | Mod wheel → bow pressure, aftertouch → damping |

---

## Scores — OAKEN

| Dimension | Score | Notes |
|-----------|-------|-------|
| Concept Clarity | 9.0 | Upright acoustic bass is immediately legible and sonically specific |
| DSP Soundness | 7.5 | KS string type LP coefficient uses wrong formula; room param misleading |
| Uniqueness | 8.5 | Curing model is fleet-unique; body resonator is physically grounded |
| Expressiveness | 8.5 | Exciter switch + bow pressure + mod wheel creates real performance range |
| Preset Quality | 8.0 | Good coverage across contexts; slap and bow both represented |
| Doctrine Compliance | 7.5 | D004 issue on oaken_room (misleading implementation) |
| Fleet Fit | 9.5 | Only physically modeled bass engine in fleet; no competition |
| Emotional Weight | 9.0 | Most "human" of the four bass engines |

**OVERALL SCORE: 8.4 / 10**

---

## Top 3 Concerns — OAKEN

1. **P0 — String Type LP Coefficient**: `setStringType` uses Euler approximation (`1/(1 + 1/(fc*0.00005))`) instead of matched-Z transform (`1 - exp(-2πfc/sr)`). At gut string tension (fc=1000 Hz), this gives a near-zero coefficient that causes extremely rapid high-frequency decay unrelated to gut string behavior. Replace with correct matched-Z formula.
2. **P1 — Room Parameter Implementation**: `oaken_room` affects only stereo spread, not timbral room character. The label promises three distinct acoustic environments; the implementation delivers minor panning offset. Add at minimum a short allpass diffuser whose delay time scales with pRoom, to introduce room reflection character.
3. **P2 — Curing Rate Extreme Underexplored**: No preset uses `oaken_curingRate` above 0.5. The maximum rate (1.0) produces dramatic darkening within seconds — add one preset showcasing rapid curing for expressiveness.

## Top 3 Enhancements — OAKEN

1. **Bow Pressure Timbral Extension**: High bow pressure should produce harmonic distortion and a hardened attack character; very low pressure should allow flageolet mode (pitch one octave above, softer). Mod wheel could sweep between these extremes.
2. **Room Proper Implementation**: Add a short allpass diffuser network (2-4 stages, delay times proportional to pRoom) after the body resonator. This would genuinely distinguish "studio" from "jazz club" from "concert hall."
3. **String Preparation Mode**: A subtle extended technique — high curing rate + slap exciter + rocky body filter — could produce a prepared upright bass character (muted, percussive, woody) that has no equivalent anywhere in the fleet.

---

# ENGINE 4: OMEGA (XOmega — "The Distillation")

**Identity**: FM/digital bass. 2-operator FM with ratio presets, distillation model (mod index decays toward pure carrier), purity noise.
**Parameter prefix**: `omega_`
**Params**: 30
**Presets**: 10 (Foundation: 4, Flux: 3, Entangled: 1, Crystalline: 1, Atmosphere: 1, Deep: 1)
**Accent**: Copper Still `#B04010`

---

## The Council Has Spoken — OMEGA

| Ghost | Core Observation |
|-------|-----------------|
| **Moog** | The distillation model is the correct conceptual inversion for FM synthesis. DX7 basses work by starting with complex FM spectra and using the amp envelope's attack to shape how quickly that complexity reveals itself. XOmega turns this inside out: the complexity is always there at note-on, and it irreversibly decays toward purity during sustain. This is an anti-DX7: instead of complexity as a transient event, complexity is the default state and simplicity is earned by patience. The implementation — exponential decay of mod index with configurable half-life — is mathematically clean. The `distill.currentIndex` multiplied by the user `modIndex` gives `activeModIndex`, which is then used for phase modulation. At half-life of 2s (distillRate=1.0), a note held for 6 seconds will reduce complexity to 12.5% of initial. This is perceptible and musical. |
| **Buchla** | The purity parameter operates at the per-sample level by injecting noise into `modSignal` and drift into `carrier.phase`. The noise amount is `noise * impurity * 0.02` — 2% phase noise at full impurity. This is intentionally subtle, but the drift is `noise * impurity * 0.001` per sample, which at 48000 Hz means accumulated drift of up to 48 Hz of frequency error per second at full impurity. This can cause FM sidebands to drift significantly off their theoretical positions, producing inharmonic beating. Whether this is a bug or a feature depends on intent — the spec says "subtle random modulation" and "slight random drift." 48 Hz/second of drift is not subtle at bass frequencies. The `omega_purity` parameter should be logarithmically scaled or the drift constant reduced by 10×. |
| **Kakehashi** | The eight algorithm ratio presets are well-chosen. The progression from unison (1:1) to octave (2:1) to fifth (3:1) to sub-octave (0.5:1) covers the major bass FM applications. Algorithm 4 (1.414:1, minor seventh territory) produces the Reese bass characteristic, and it has a preset — "Reese Distillate" at Foundation — that demonstrates this correctly. Algorithm 6 (7:1 bell ratio) opens the engine to non-bass applications. Algorithm 7 (custom ratio) allows players to explore. This is a complete FM bass toolkit in 8 steps. The Crystalline preset "Bell Bass" (Algorithm 6) shows range. I would like to see an algorithm 5 (3.5:1 inharmonic) preset that is not in the library currently. |
| **Smith** | The FM operator's `process()` function has a subtle bug in the self-feedback path. The code computes `feedbackSample = out * feedback` but never uses `feedbackSample` — it calculates it then discards it. The actual feedback is implemented in `renderBlock` as `voice.modulator.process(voice.modulator.lastOutput * fbNow * 0.5f)` where the feedback is passed as the modInput argument. So the feedback is operational but `OmegaFMOperator::feedback` field is never read. This is dead code in the operator struct, but the functional feedback path (via modInput) works correctly. The confusion is that `setFeedback()` sets the field `feedback` which is then never used — the caller in `renderBlock` computes it independently. This is harmless but confusing. |
| **Schulze** | From a compositional perspective, XOmega offers something none of the other three CELLAR engines provide: a bass sound that changes its fundamental character while you hold it. Not just its filter or amplitude — its actual harmonic content. A note that begins as a complex FM cluster (mod index 10+, distillRate=1.0) and becomes a near-pure sine over 4-6 seconds is a compositional gesture, not just a timbre. This is the kind of event that a bassist can build a section around: hold the note, let it purify, release into silence. I hear this as modern classical music — the bass note as a structural element that completes a harmonic process before the next note arrives. |
| **Vangelis** | XOmega is the most cerebral of the four bass engines and it is the furthest from the body. Sub bass (XOgre) is felt in the chest. Analog bass (XOlate) is felt in the warmth of the room. Acoustic bass (XOaken) is felt in the resonance of wood. FM bass (XOmega) is heard in the mind — a mathematical event more than a physical one. The purity parameter is this engine's emotional axis: at purity=0.0, the distillate is "impure" — contaminated, industrial, unstable. At purity=1.0, it is mathematically clean — the perfect sine wave, the theoretical ideal. Playing between these extremes is playing between chaos and order. This is correct for XOmega's identity. The emotional register of XOmega is precision and dissolution: something exact that becomes more exact as it decays. |
| **Tomita** | The mod index → amplitude of FM sidebands relationship deserves attention for audio safety. At `omega_modIndex=20`, the FM spectrum is extremely wide, potentially generating significant energy at alias frequencies. The output LP filter (`voice.outputFilter`) with its `velBright + velocity * 4000` Hz cutoff at high velocities can be 12000+ Hz, which is wide enough to allow FM aliases through. For a preset with algorithm 5 (3.5:1 inharmonic ratio) at high mod index, the sideband structure is already complex — with Nyquist aliasing on top, the result could be harsh digital artifacts. The output filter should have a frequency ceiling of approximately `srf * 0.4` (Nyquist at 0.4 headroom) regardless of velocity or brightness settings. |

---

## Doctrine Compliance — OMEGA

| Doctrine | Status | Notes |
|----------|--------|-------|
| D001 | PASS | Velocity scales FM brightness (`velBright = brightness + velocity * 4000`) |
| D002 | PASS | 2 LFOs + mod wheel (→mod index) + aftertouch (→feedback) + 4 macros |
| D003 | PASS | Chowning (1973), Yamaha DX7 (1983) cited and correctly implemented |
| D004 | **MINOR** | `OmegaFMOperator::feedback` field is set by `setFeedback()` but never read in DSP (feedback is passed via modInput in renderBlock instead). Dead code, not a dead parameter. |
| D005 | PASS | LFO floor at 0.005 Hz |
| D006 | PASS | Mod wheel → mod index, aftertouch → feedback |

---

## Scores — OMEGA

| Dimension | Score | Notes |
|-----------|-------|-------|
| Concept Clarity | 9.0 | FM bass + distillation model is conceptually sharp |
| DSP Soundness | 8.0 | Purity drift too aggressive; operator feedback dead code; otherwise clean |
| Uniqueness | 9.0 | Distillation model (complexity→purity over sustain) is fleet-unique |
| Expressiveness | 8.5 | MW→mod index, AT→feedback, algorithm selection all musically meaningful |
| Preset Quality | 8.5 | Good coverage; Reese and DX Essence are library-quality |
| Doctrine Compliance | 8.5 | Minor dead code in operator, not a player-facing issue |
| Fleet Fit | 9.0 | Only FM bass engine; digital precision fills a clear gap |
| Emotional Weight | 8.0 | Cerebral/mathematical; less physical than the other three |

**OVERALL SCORE: 8.6 / 10**
*Strongest engine in CELLAR quad*

---

## Top 3 Concerns — OMEGA

1. **P0 — Purity Drift Magnitude**: `noise * impurity * 0.001` per-sample phase drift accumulates to ~48 Hz/second at maximum impurity — not "subtle drift" as spec claims. Scale down by 10× or apply log scaling to purity.
2. **P1 — Operator Feedback Dead Code**: `OmegaFMOperator::setFeedback()` sets a field that is never read. The actual feedback path works (via modInput in renderBlock) but `feedback` is dead code. Either remove the field from the operator struct and the `setFeedback()` method, or use the field in `process()` for a second feedback path.
3. **P2 — Alias Protection**: Output filter ceiling should be clamped to `srf * 0.4` to prevent high-mod-index aliases from passing through at high velocity/brightness settings.

## Top 3 Enhancements — OMEGA

1. **Blessing Candidate B039**: The Distillation Model — FM complexity irreversibly decaying toward pure carrier over note sustain — is genuinely novel. No commercial FM synthesizer does this by default. The "complexity-to-purity within a held note" is a musical behavior worth naming and recognizing.
2. **Algorithm 5 Preset**: The 3.5:1 inharmonic ratio has no featured preset. It produces bell-plus-bass character that bridges into non-bass territory. Add an Entangled or Prism preset showing algorithm 5 at moderate mod index.
3. **Distillation Visualization**: The UI should show the current distilled mod index as a readout — watching it decay in real time as a note is held would make the concept immediate and dramatic for first-time users.

## Blessing Candidates — OMEGA

- **B039 Candidate**: Distillation Model (FM complexity→purity decay). *Recommend formal naming: "The Distillation: FM modulation index as a quantity that is consumed by time, irreversibly simplifying toward mathematical purity."*

---

# CELLAR CROSS-QUAD ASSESSMENT

## How Well Do the 4 Bass Engines Differentiate?

**Differentiation is excellent at the identity layer.** OGRE (sub/earth), OLATE (analog/wine), OAKEN (acoustic/wood), OMEGA (FM/distillate) occupy clearly non-overlapping perceptual spaces. A producer choosing between them is choosing between different ontologies of bass, not between different colors of the same sound.

**Differentiation at the synthesis layer is also strong.** Zero overlap in oscillator topology:
- OGRE: sine/triangle + sub-harmonic (additive-adjacent)
- OLATE: saw/pulse + ladder filter (subtractive classic)
- OAKEN: Karplus-Strong + body resonator (physically modeled)
- OMEGA: 2-op FM with distillation decay (frequency modulation)

**Where differentiation weakens**: All four engines share the same macro labels (CHARACTER/MOVEMENT/COUPLING/SPACE) and the same LFO parameters (rate/depth/shape × 2). The gravitational coupling parameter (`*_gravity`) is identical across all four in implementation but none of them actually receive gravity signals from each other (V2 feature per architecture doc). This shared but non-functional parameter is potentially confusing.

**Ranking by originality of core mechanism**:
1. OMEGA — Distillation model (irreversible FM decay over sustain) is unique in synthesis literature
2. OAKEN — Curing model + KS + body resonator is the most physically complete bass engine
3. OLATE — FermentationIntegrator (complexity grows) is an interesting inversion of normal synthesis
4. OGRE — Sub-harmonic generator + tectonic LFO are solid but less novel

---

# GARDEN QUAD

Four engines sharing the GARDEN ecological metaphor: pioneer (OXALIS), intermediate (OVERGROW), later intermediate (OSIER), climax (ORCHARD). All use GardenAccumulators (W/A/D + Season) and GardenMycorrhizalNetwork. Architecture in shared DSP: `Source/DSP/GardenAccumulators.h`.

---

# ENGINE 5: OXALIS (XOxalis — "The Geometric Garden")

**Identity**: Pioneer strings. Phyllotaxis harmonic spacing (golden ratio partial distribution), synthetic precision. Fast germination.
**Parameter prefix**: `oxal_`
**Params**: ~27
**Presets**: 1 (Atmosphere: "Golden Spiral")
**Accent**: Clover Purple `#6A0DAD`

---

## The Council Has Spoken — OXALIS

| Ghost | Core Observation |
|-------|-----------------|
| **Moog** | The PhyllotaxisOscBank is the most mathematically precise oscillator design in the GARDEN quad and it lives up to the promise. Seven partials at golden-ratio-derived frequencies blend from standard harmonic series (phi=0) through phyllotaxis ratios (phi=1). The crossfade `ratio = standardRatio + (phyllotaxisRatio - standardRatio) * phi` is clean. At phi=0, this is a conventional supersaw; at phi=1, the partials appear at irrational ratios that make the spectrum permanently non-periodic in the traditional sense. This is "too mathematical to be natural, too beautiful to be mechanical" — the spec's own words are exactly right. The critical implementation decision I want to flag: `setFundamental()` is called per-sample (`voice.oscBank.setFundamental(freq, srf, phiNow + l2 * 0.2f)`). This recomputes all seven partial phase increments and amplitudes every sample. Given that LFO modulation of phi is applied here, this is intentional — but `setFundamental` uses `std::pow(kPhi, i)` for phyllotaxis ratios, which is a floating-point power function. Seven calls to `std::pow` per sample, per voice, at block rate will cost CPU. This needs a cache or block-rate update. |
| **Buchla** | The Growth Mode in XOxalis is the most mathematically legible of the four: partials emerge at golden angle intervals over time. `int activePartials = 1 + (growthPhase * 6)` steps through 1 to 7 partials as growth completes. This is the correct implementation of "phyllotaxis germination" — the geometric plant unfolds one layer at a time, each new layer appearing at a golden-angle position relative to the previous. The symmetry parameter (`oxal_symmetry`) adds asymmetric waveshaping at low values: `fastTanh(oscOut * 2) * 0.3 * asymmetry`. This creates organic imperfection in a mathematically perfect instrument. That is good design — the imperfection makes it livable. |
| **Kakehashi** | One preset. This is critically thin for a fleet engine. "Golden Spiral" is a good preset — high phi (0.8), wide spread (0.8), moderate sustain — but it shows only one face of this engine. XOxalis needs at minimum: a standard-harmonic baseline (phi=0, like a thick supersaw), a phi=0.5 hybrid (the interesting middle ground), a growth-mode preset (partials unfolding over 10 seconds), and a high-symmetry preset (the pure mathematical version). One preset is insufficient for artist discovery. |
| **Smith** | The dormancy pitch variance in XOxalis uses `accumulators.getDormancyPitchVariance() * 0.5f` — half the variance of the organic engines. This is the correct design decision: "geometric precision degrades slightly during dormancy" is in the source comment, and halving the variance acknowledges that a mathematical plant is less susceptible to temperature than an organic one. The dormancy system is working correctly here. The GardenAccumulators are properly configured for the pioneer role (`wRiseRate=0.004`, `aThreshold=0.3`, `dDecayRate=0.015`) — fastest warmth rise, lowest aggression threshold, fastest dormancy recovery. The hierarchy is internally consistent. |
| **Schulze** | XOxalis interests me as a question about the limits of harmonic perception. Standard harmonic series are integer multiples of the fundamental — the auditory system hears them as a single pitch. Phyllotaxis ratios (1.0, 1.618, 2.618, 4.236...) are inharmonic in the mathematical sense but contain the golden ratio, which is known to be optimally "far" from any simple fraction. Whether the auditory system fuses these partials into a perceived pitch or hears them as separate tones depends on the fundamental frequency and the listener's harmonic fusion threshold. At low phi values, fusion is likely. At phi=1.0, the partials may separate into distinct pitches — not a chord, but a quasi-periodic texture. This is precisely the intended effect: "too orderly to be natural." |
| **Vangelis** | XOxalis is the fastest engine in the GARDEN quad — "pioneer species" that responds immediately to note-on without waiting for W or D accumulation. Its emotional register is clarity and mathematical inevitability. When I imagine it in music, I hear it in minimalist or neoclassical contexts where the harmonic purity of strings is wanted but the standard supersaw pad is too thick. The phi parameter allows a player to dial between "conventional precision" and "golden ratio precision" — this is a gradual journey from familiar to alien. The Growth Mode, where partials emerge sequentially at golden angle intervals, is the engine's most dramatic feature and deserves its own preset. |
| **Tomita** | Orchestrally, XOxalis occupies the space between a string section and a harmonic series synthesizer. At phi=0 it sounds like a string ensemble. At phi=1 it sounds like a spectral composition from the European avant-garde. This is an unusually wide identity range for an engine with one continuous parameter. The spread parameter (`oxal_spread`) controls how the four voices are panned, and at maximum spread (1.0) with phi=0.5 the engine has a lush orchestral width. The vibratoDepth default at 0.15 is conservative — strings at this depth barely have vibrato. Consider raising the default to 0.25 for more immediate expressiveness. |

---

## Doctrine Compliance — OXALIS

| Doctrine | Status | Notes |
|----------|--------|-------|
| D001 | PASS | Velocity scales output brightness (`velBright = 0.4 + velocity * 0.6`) |
| D002 | PASS | 2 LFOs + vibratoLFO + aftertouch (→cutoff) + mod wheel absent from render — **CONCERN** |
| D003 | N/A | Mathematical/algorithmic, not physical modeling |
| D004 | PASS | All params wired (phi→partial ratios, symmetry→waveshaping, spread→panning) |
| D005 | PASS | LFO floor at 0.005 Hz |
| D006 | **PARTIAL** | Aftertouch → cutoff (effective). Mod wheel: no explicit MW routing in renderBlock — effectiveVibratoDepth does not include modWheelAmount, unlike Orchard/Overgrow/Osier. |

---

## Scores — OXALIS

| Dimension | Score | Notes |
|-----------|-------|-------|
| Concept Clarity | 9.0 | Phyllotaxis strings is immediately legible and scientifically grounded |
| DSP Soundness | 7.5 | setFundamental per-sample with std::pow (CPU cost); MW not routed to vibrato |
| Uniqueness | 9.5 | Phyllotaxis harmonic spacing is fleet-unique and synthesis-unique |
| Expressiveness | 7.0 | Phi, spread, growth mode all strong; MW routing absent |
| Preset Quality | 4.0 | One preset is critically insufficient |
| Doctrine Compliance | 8.0 | D006 partial (MW not wired to vibrato as expected) |
| Fleet Fit | 9.0 | Mathematical pioneer role distinguishes from all other string engines |
| Emotional Weight | 8.0 | Beautiful and precise; slightly cold by design |

**OVERALL SCORE: 7.8 / 10**
*Conceptually the most original GARDEN engine; held back by thin presets and CPU concern*

---

## Top 3 Concerns — OXALIS

1. **P0 — setFundamental CPU**: `std::pow(kPhi, i)` called per-sample for each of 7 partials, each of 4 voices = 28 std::pow calls per sample. Cache the phyllotaxis ratios at block rate or only update when freq or phi changes significantly (threshold ~1 cent change).
2. **P1 — Mod Wheel Not Routed**: `effectiveVibratoDepth` in XOxalis does not include `modWheelAmount`, unlike all three peer GARDEN engines (Orchard, Overgrow, Osier all add `modWheelAmount * 0.4-0.5` to vibratoDepth). D006 partial violation.
3. **P2 — Preset Critical Shortage**: One preset for a flagship string concept. Minimum viable library: phi=0 (standard supersaw), phi=0.5 (hybrid), phi=1.0 + growthMode (phyllotaxis germination), high symmetry + high spread (mathematical ensemble).

## Top 3 Enhancements — OXALIS

1. **Preset Library Expansion**: Minimum 5 presets across different phi values and growth modes. This is the most urgent non-bug issue.
2. **Phi Automation Cache**: When LFO2 modulates phi, it can change slowly. Cache partial ratios and only recompute when phi changes by more than 0.001. Saves the majority of per-sample `std::pow` calls.
3. **GoldenAngle Panning**: Pan the 7 partials at golden-angle offsets across the stereo field rather than summing them. Would create a spatial shimmer unique to this engine.

## Blessing Candidates — OXALIS

- **B040 Candidate**: Phyllotaxis Harmonic Synthesis — harmonic partials distributed at golden-angle intervals, creating spectra that are inharmonic in the mathematical sense but optimally spaced by the golden ratio. *First synthesizer in fleet (and possibly commercially) to use phyllotaxis as a harmonic spacing model.*

---

# ENGINE 6: OVERGROW (XOvergrow — "The Weed Through Cracks")

**Identity**: Solo string. Karplus-Strong + runner generation (sympathetic sub-harmonics from stressed notes), wildness control, silence response.
**Parameter prefix**: `grow_`
**Presets**: 1 (Atmosphere: "Vine Crawl")
**Accent**: Vine Green `#3A5F0B`

---

## The Council Has Spoken — OVERGROW

| Ghost | Core Observation |
|-------|-----------------|
| **Moog** | The RunnerGenerator is XOvergrow's signature mechanism and it is genuinely original. Under conditions of high velocity and high aggression (via GardenAccumulators.A), a sympathetic sub-harmonic string spontaneously generates after a 2-8 second delay. The sub-harmonic is at either octave-below or fifth-below, randomly chosen. This is the "weed sprouting from a stressed note" — the music metaphor fully realized in DSP. The implementation is clean: probability check at block rate, KarplusStrongString excitation at sub-harmonic frequency, amplitude decay until it fades. The delayed onset (2-8 seconds at wildness=0.3-1.0) ensures the runner is a surprise, not a predictable effect. My concern is that `amplitude *= 0.99998f` per sample at 48kHz gives a half-life of approximately 25 minutes. The runner never meaningfully fades — it just becomes very quiet over a long time. The decay should be faster: `0.9998f` gives a half-life of ~24 seconds, which is more appropriate for a "runner" that appears briefly and fades. |
| **Buchla** | The silence response mechanism is architecturally present (`voice.silenceTimer`, `voice.lastOutputLevel`) but not used in the render path. The variables exist in OvergrowVoice but no code in renderBlock reads `silenceTimer` or uses it to trigger any behavior. The spec says "the moments between notes are where XOvergrow develops" — but the silenceTimer never does anything. This is a D004 violation for a behavioral feature: the silence response is described as a core engine characteristic and is dead code. |
| **Kakehashi** | One preset again. "Vine Crawl" demonstrates wildness (0.6) and bow noise (0.4) but does not show the runner generation, which requires extended playing with high velocity. A runner generation preset needs: high wildness (0.8), high aggression accumulation (which requires playing to build up A), and should be documented in the preset description that runners appear after sustained forceful playing. The preset library must teach the engine's unique behavior, and one atmospheric preset cannot do that. |
| **Smith** | The bow noise injection (`bowNoiseSignal = noise * bowNow * 0.05f`) is added directly to `stringOut` without filtering. On a real bowed string, bow noise is bandlimited by the string's resonant response — the bow excites the string, which then filters the noise through its resonances. Here, wideband noise is added to the Karplus-Strong output, which means high-frequency bow noise bypasses the string's damping LP filter. The noise should be added to the KarplusStrongString's delay line (as additional excitation) rather than to the output, so it passes through the string's damping filter. |
| **Schulze** | XOvergrow is the most compositionally interesting concept in the GARDEN quad: a string instrument that responds to what happens in the silences between notes. If the silence response were implemented, a player who holds a note then pauses would hear the silence develop — runners fading in, subtle pitch drift, the "weed growing through the crack." This is temporal composition at the instrument level, and it is currently absent from the code. The runner generator alone is a strong innovation. But the silence response would elevate XOvergrow from "interesting" to "singular." |
| **Vangelis** | I hear XOvergrow as the cello playing alone in an abandoned building — the instrument sounding in a space that has its own life, that responds to the playing in unexpected ways. The wildness parameter is this engine's emotional axis: at low wildness, it is a somewhat conventional solo string; at high wildness, it grows unpredictably, throwing runners and pitch jitter like a weed finding cracks in every surface. The pitch jitter (`wildNow * 0.05` per-sample noise, ±5 cents at max) is modest and correct for a "growing" rather than "broken" instrument. This engine has strong poetic identity. Its DSP needs the silence response implemented to deliver on it. |
| **Tomita** | The runner generator opens XOvergrow to polyphonic string behavior from a monophonic-focused engine. When a runner fires, you have two Karplus-Strong strings sounding simultaneously — the original note and a sympathetic sub-harmonic. If the runner's amplitude were louder (say 0.3-0.4 of main string rather than 0.15), this would be audibly present rather than just a subliminal enrichment. The runner's pitch (octave below or fifth below) is always harmonically related to the main note, so it will never create dissonance. Louder runners = more interesting string ensemble behavior from a solo engine. |

---

## Doctrine Compliance — OVERGROW

| Doctrine | Status | Notes |
|----------|--------|-------|
| D001 | PASS | Velocity scales KS excitation brightness and attack time |
| D002 | PASS | 2 LFOs + vibratoLFO + aftertouch (→cutoff) + mod wheel (→vibratoDepth) |
| D003 | N/A | Karplus-Strong is referenced in OakenEngine; Overgrow doesn't cite sources |
| D004 | **FAIL** | `voice.silenceTimer` and `voice.lastOutputLevel` exist but are never used in renderBlock. Silence response is dead. |
| D005 | PASS | LFO floor at 0.005 Hz |
| D006 | PASS | Mod wheel → vibrato depth, aftertouch → cutoff |

---

## Scores — OVERGROW

| Dimension | Score | Notes |
|-----------|-------|-------|
| Concept Clarity | 9.0 | "Weed through cracks" is vivid and the runner concept delivers it |
| DSP Soundness | 6.5 | Runner fade too slow; bow noise bypasses string LP; silence response dead |
| Uniqueness | 9.0 | RunnerGenerator is fleet-unique; no other engine has sympathetic sub-harmonics |
| Expressiveness | 8.0 | Wildness, bow noise, vibrato all strong; MW → vibrato correct |
| Preset Quality | 4.0 | One preset, doesn't demonstrate runners |
| Doctrine Compliance | 7.0 | D004 fail on silenceTimer |
| Fleet Fit | 8.5 | Distinct from other string engines; intermediate ecological role clear |
| Emotional Weight | 8.0 | When runners fire, it is genuinely surprising and beautiful |

**OVERALL SCORE: 7.5 / 10**
*Most intriguing concept; most incomplete implementation*

---

## Top 3 Concerns — OVERGROW

1. **P0 — Silence Response Dead Code**: `voice.silenceTimer` and `voice.lastOutputLevel` are tracked but never used to trigger any behavior. This is a D004 violation for the engine's stated core feature. Implement: during silence, increment silenceTimer; when silenceTimer crosses a wildness-scaled threshold, trigger a quiet runner at a harmonically unexpected pitch (7th harmonic, etc.).
2. **P0 — Runner Fade Too Slow**: `amplitude *= 0.99998f` per sample = ~25-minute half-life. Change to `0.9998f` (24-second half-life) so runners are episodic events rather than perpetual subliminal hum.
3. **P1 — Bow Noise Bypass**: Bow noise added to `stringOut` bypasses the string's LP damping filter. Move bow noise injection to inside `KarplusStrongString::excite()` or add it to the delay line directly, so it is filtered by the string's resonance.

## Top 3 Enhancements — OVERGROW

1. **Silence Response Implementation**: This is what makes XOvergrow unique — implement it. During silence, let the engine slowly introduce pitch drift, harmonic shimmer, or a quiet runner. "The silence develops" is a compositional promise that must be kept.
2. **Runner Amplitude Increase**: Raise runner base amplitude from `0.15 * wildness` to `0.25 * wildness`. Runners should be audibly present, not subliminal.
3. **Preset Library for Runners**: Add a performance guide preset that explains (in description text) how to elicit runners: play forcefully for 30+ seconds at high wildness, then hold a note and wait.

---

# ENGINE 7: OSIER (XOsier — "The Herb Garden")

**Identity**: Chamber quartet strings. 4 voices with named roles (Soprano/Alto/Tenor/Bass), per-role tonal character, companion planting (inter-voice pitch affinity over session time).
**Parameter prefix**: `osier_`
**Params**: 27
**Presets**: 1 (Atmosphere: "Thyme Garden")
**Accent**: Willow Green `#6B8E23`

---

## The Council Has Spoken — OSIER

| Ghost | Core Observation |
|-------|-----------------|
| **Moog** | The companion planting mechanism is architecturally sound and well-implemented. Six affinity accumulators for four voices (C(4,2)=6 pairs), rising when both voices are active and decaying during silence. Each voice's total affinity (`getVoiceAffinity(i)`) is the normalized sum of the three pair affinities it participates in. This is then used to pull each voice's pitch toward its companions via `cents * aff * pCompanion * 0.01 * pIntimacy`. The mathematics are correct and the scale is appropriate: at maximum affinity (1.0) and maximum companion/intimacy (1.0 each), the pitch pull is `cents * 0.01` — 1% of the inter-voice pitch distance. This is subtle enough to not cause audible retuning but strong enough to create the "playing together" feel over extended sessions. |
| **Buchla** | The per-role tonal shaping is the engine's most elegant feature. Each of the four quartet roles gets a distinct configuration: Soprano (fast vibrato, narrow, bright, +800 Hz filter bias), Alto (medium vibrato, warm, +200 Hz), Tenor (wider vibrato, neutral, -200 Hz), Bass (slow vibrato, dark, -600 Hz). This creates genuine timbral individuality without requiring separate DSP chains — just different parameter values applied to the same synthesis path. The implementation in `kRoleConfigs[4]` is clean and easy to extend. My concern: voice roles are assigned by voice index (`voices[idx].role = OsierQuartetRole(idx)`), which means voice stealing uses LRU and will reassign notes to whichever voice is free — so a high note played when the "Bass" voice (index 3) is free will be played by the bass voice with dark, slow-vibrato character. The role-to-voice binding should be stable and pitch-aware, not LRU-based. |
| **Kakehashi** | One preset. Same problem as OXALIS and OVERGROW — thin preset library for a rich engine concept. The companion planting mechanism requires playing time to demonstrate: a preset that starts with warm companion values (after an extended session) will sound different from cold-start values. The "Thyme Garden" preset is an atmospheric pad that needs an accompanying note about playing behavior. Add at minimum: a cold-start quartet (short companion, no intimacy), a warm quartet (high companion, high intimacy), and a growth mode chamber piece. |
| **Smith** | The companion pitch pull `pull += cents * aff * pCompanion * 0.01 * pIntimacy` accumulates across all active companion voices without normalization. With four voices active, each voice accumulates pulls from three companions simultaneously. If all three companions are significantly off-pitch from voice i, the total pull could be `3 * cents * 1.0 * 1.0 * 0.01` = 3% of the pitch distance — which at a tritone interval (600 cents) gives 18 cents of pull. This is below the detectable threshold for a chord (listeners tolerate ~15-20 cents) but at extreme intervals the companion pull could cause unexpected retuning artifacts. Consider capping `voice.companionPitchCents` at ±10 cents maximum. |
| **Schulze** | The Bass voice's "nutrient provider" comment in the render code (`// Bass voice (nutrient provider): its warmth feeds the upper voices`) has no actual DSP implementation — it is a comment saying that the companion planting handles this effect through the accumulator, but the companion planting only affects pitch, not amplitude or warmth. The "Bass as nutrient provider" is a poetic statement that currently does nothing beyond the companion pitch pull that all voices participate in equally. This is not a D004 violation (the Bass voice still affects audio through its pitch and output), but the nutrient metaphor overreaches what the code actually does. |
| **Vangelis** | XOsier is the most intimate engine in the GARDEN quad and in many ways in the fleet. A quartet of four named individuals playing together, developing affinity over time, pulling slightly toward each other in pitch as the session deepens — this is chamber music sociology in a synthesizer. The emotional register is warmth and trust: these four voices learn to play together in your sessions. The `osier_intimacy` parameter is particularly well-named: at high intimacy, the voices converge slightly in pitch, creating the "ensemble lock" that characterizes a long-standing chamber group. |
| **Tomita** | Orchestrally, the quartet seating positions (Soprano left, Alto center-left, Tenor center-right, Bass right) are correctly aligned with traditional string quartet placement. But because voice roles are assigned by LRU allocation rather than pitch, a high note played last (on the Bass voice) will be panned to the right with dark, slow-vibrato character. This creates seating-inconsistencies: the brightest, fastest notes might be assigned to the Bass position and panned right, while a low sustained note sits in the Soprano position at left. A more sophisticated voice allocation that considers pitch-to-role compatibility would better serve the quartet metaphor. |

---

## Doctrine Compliance — OSIER

| Doctrine | Status | Notes |
|----------|--------|-------|
| D001 | PASS | Velocity scales output (`0.5 + velocity * 0.5`) |
| D002 | PASS | 2 LFOs + vibratoLFO + mod wheel (→vibratoDepth) + aftertouch (→cutoff) |
| D003 | N/A | No physical modeling claimed |
| D004 | PASS | All params have DSP destinations (companion→pitch pull, intimacy→pull scaling, role configs→filter bias) |
| D005 | PASS | LFO floor at 0.005 Hz |
| D006 | PASS | Mod wheel → vibrato depth, aftertouch → cutoff |

---

## Scores — OSIER

| Dimension | Score | Notes |
|-----------|-------|-------|
| Concept Clarity | 9.5 | Chamber quartet with named roles is fully realized conceptually |
| DSP Soundness | 8.0 | Companion pull accumulation risk; role/pitch mismatch; Bass nutrient comment misleads |
| Uniqueness | 9.0 | Companion planting pitch affinity is fleet-unique |
| Expressiveness | 8.5 | Role configs, companion, intimacy, vibrato all contribute expressively |
| Preset Quality | 4.0 | One preset; companion planting needs session time to demonstrate |
| Doctrine Compliance | 9.0 | All params wired; all D001-D006 met |
| Fleet Fit | 9.0 | Chamber quartet niche unoccupied elsewhere |
| Emotional Weight | 9.0 | Warmth, trust, ensemble intimacy — rare in synthesis |

**OVERALL SCORE: 8.3 / 10**

---

## Top 3 Concerns — OSIER

1. **P1 — Role-Pitch Mismatch**: Voice roles are assigned by LRU allocation, not pitch range. High notes played on the Bass voice (dark, slow vibrato, right-panned) create unmusical dissonance between pitch character and voice identity. Either sort voices by pitch at the end of each block, or create a pitch-to-role assignment heuristic in `noteOn`.
2. **P1 — Companion Pull Cap**: Total companion pitch pull can reach ±18+ cents at extreme intervals with maximum affinity/companion/intimacy. Cap at ±10 cents maximum to prevent audible retuning artifacts.
3. **P2 — Bass Nutrient Comment**: The "Bass voice: its warmth feeds the upper voices" comment in the render path has no DSP implementation. Either implement it (Bass voice's W accumulation could boost upper voices' filter cutoff or vibrato depth) or remove the comment to avoid user expectation mismatch.

## Top 3 Enhancements — OSIER

1. **Preset Expansion**: Minimum 4 presets exploring companion planting at different stages: cold (new ensemble), warm (established), intimate (long-standing), and growth mode chamber piece.
2. **Pitch-Aware Voice Assignment**: In `noteOn`, sort the active voices by pitch and reassign roles accordingly — lowest active voice becomes Bass, highest becomes Soprano. This preserves the quartet metaphor across all note combinations.
3. **Bass Nutrient Implementation**: Make the Bass voice's warmth accumulation (W > 0.3) provide a +100-200 Hz cutoff boost to the Soprano and Alto voices. Small, warm, and semantically correct — the bass player's tone does warm the upper voices in a real ensemble.

---

# ENGINE 8: ORCHARD (XOrchard — "The Cultivated Grove")

**Identity**: Orchestral strings. 4 detuned sawtooth oscillators per voice (ensemble), formant filter (orchestral body resonance), Concertmaster mechanism (highest voice leads). Seasonal tonal character.
**Parameter prefix**: `orch_`
**Presets**: 1 (Atmosphere: "Winter Orchard")
**Accent**: Harvest Gold `#DAA520`

---

## The Council Has Spoken — ORCHARD

| Ghost | Core Observation |
|-------|-----------------|
| **Moog** | XOrchard is the most resource-intensive engine in the GARDEN quad and justifiably so: 4 voices × 4 oscillators = 16 simultaneous sawtooth generators, plus formant filter, plus GardenAccumulators. The 4-voice limit (vs. 8 in CELLAR engines) is an honest CPU tradeoff. The detuning implementation (`freq * (detuneOffsets[o] + detNow * (o < 2 ? -1.0f : 1.0f)) / 1200.0f`) attempts cents-to-frequency conversion but uses an approximation: `detuneHz = freq * cents / 1200` is a first-order Taylor approximation that is accurate within ±1 cent for detuning up to about 50 cents. At `ogre_detune=20` (20 cents), the error is approximately 0.17% per octave — barely perceptible but worth noting. The correct conversion is `freq * (2^(cents/1200) - 1)`. |
| **Buchla** | The Concertmaster mechanism mentioned in the architecture doc is not implemented in the render code. The architecture states "the highest voice leads, others follow" — but in the render path, all four voices are processed identically with no Concertmaster-specific routing. The highest voice does not have any special parameter treatment, does not lead LFO timing, and does not influence other voices' behavior. The Concertmaster is an architectural promise that does not exist in the code. This is either a V2 feature (like the gravitational coupling V2 plan in CELLAR) or a D004 issue for a behavioral feature. |
| **Kakehashi** | The seasonal system (Spring/Summer/Fall/Winter) is the most elaborate temporal mechanism in the GARDEN quad. The `orch_season` parameter allows manual override (`-1 = auto`), which is thoughtful design: players can set a seasonal character without waiting for W/A/D to accumulate it organically. The "Winter Orchard" preset uses `orch_season=3` (Winter), demonstrating deliberate use of this feature. Winter's tonal modifier (`getSeasonBrightness() = -0.5`) subtracts 1000 Hz from effective cutoff — audible darkness. This is the correct emotional register for winter. The auto-season progression (Spring → Summer as W rises, Fall as A rises, Winter as D rises) creates a tonal journey over the session without player intervention. This is musically interesting and unique. |
| **Smith** | The ensemble detuning in XOrchard is identical in concept to standard supersaw synthesis but with the growth mode adding a second temporal dimension: oscillators fade in sequentially (`float oscOnset = i * 0.2f; oscGain = clamp((growthPhase - oscOnset) * 3, 0, 1)`). The four oscillators bloom at 20% growth intervals — osc0 at 20%, osc1 at 40%, osc2 at 60%, osc3 at 80%. This gives the "germination" behavior: the chord starts thin (one oscillator) and fills out (four oscillators). The quadratic `growthGain = growthPhase^2` ensures the early growth is slow, which is correct for a climax species that "requires established conditions to bloom fully." The physics of ecological succession is correctly modeled in the timing. |
| **Schulze** | XOrchard is the engine I want to use for slow, extended compositions where the tonal character shifts with the season of the session. A 45-minute improvisation that begins in Spring (slightly bright, fresh), passes through Summer (full, lush, neutral), moves into Fall (rich, slightly rolled-off), and ends in Winter (dark, sparse, dormant) — this is compositional time at the scale of a seasonal arc. No other engine in the fleet offers this. The formant filter adds orchestral body to what would otherwise be a supersaw pad, which is critical for the "cultivated grove" identity — this is a managed, resonant space, not a synthesizer. |
| **Vangelis** | XOrchard is the most cinematic engine in the GARDEN quad. "Slow to arrive, most resource-demanding, most stable once established" — this is the orchestral string section arriving late in the recording session, after all the other instruments have found their places. The 0.15-second attack default, the lush sustain, the formant body resonance — these create the specific emotional weight of a full string orchestra. What is missing from the preset library (one preset!) is any exploration of the seasonal arc: a Summer Orchard (W>0.5, A<0.4, D<0.3) would be the engine's most expressive state, and it has no preset. |
| **Tomita** | The formant filter (`formantFilter`, CytomicSVF BandPass) centered at 300-2800 Hz with 30-70% wet blend is well-designed. The formant center sweeping from 300 Hz to 2800 Hz via `pFormant` parameter gives orchestral body character at low values (viola-like warmth around 300-600 Hz) and brighter violin character at high values (bright presence around 2-4kHz). The blend `blended = oscMix * (1 - formNow * 0.5) + formantSig * formNow * 0.5` ensures the dry signal always remains present. The coupling input `EnvToMorph` modulating formant is a sophisticated choice — coupling from a dynamic engine like OUROBOROS or OBRIX into XOrchard's formant creates orchestral timbre that responds to cross-engine energy. |

---

## Doctrine Compliance — ORCHARD

| Doctrine | Status | Notes |
|----------|--------|-------|
| D001 | PASS | Velocity scales output level (`0.5 + velocity * 0.5`) |
| D002 | PASS | 2 LFOs + vibratoLFO + mod wheel (→vibratoDepth) + aftertouch (→filterCutoff) |
| D003 | N/A | No physical modeling claimed |
| D004 | **PARTIAL** | Concertmaster mechanism mentioned in architecture doc is absent from renderBlock. If "highest voice leads, others follow" is intended as a behavioral feature, it is dead. If it is a metaphor for the seasonal/accumulator system, the architecture doc needs clarification. |
| D005 | PASS | LFO floor at 0.005 Hz |
| D006 | PASS | Mod wheel → vibrato depth, aftertouch → filter cutoff |

---

## Scores — ORCHARD

| Dimension | Score | Notes |
|-----------|-------|-------|
| Concept Clarity | 9.0 | Orchestral climax species is immediately legible and emotionally clear |
| DSP Soundness | 8.0 | Detuning approximation minor; Concertmaster absent; season system solid |
| Uniqueness | 8.5 | Seasonal tonal arc + formant body resonance combination is distinctive |
| Expressiveness | 8.5 | Formant, season, ensemble width, vibrato all meaningful |
| Preset Quality | 4.0 | One preset; no Summer/Spring/Fall presets showing seasonal arc |
| Doctrine Compliance | 8.0 | D004 partial for Concertmaster (absent from code) |
| Fleet Fit | 9.5 | Orchestral string climax; unoccupied niche in fleet |
| Emotional Weight | 9.0 | Cinematic, seasonal, patient — the most orchestral engine in the fleet |

**OVERALL SCORE: 8.2 / 10**

---

## Top 3 Concerns — ORCHARD

1. **P1 — Concertmaster Not Implemented**: Architecture doc describes "highest voice leads, others follow" but the render path treats all voices identically. Either implement (e.g., highest active voice has lower vibrato damping, its LFO phase leads others) or explicitly mark as V2 in the architecture doc.
2. **P1 — Detuning Approximation**: `freq * cents / 1200` should be `freq * (pow2(cents/1200) - 1)` for correct semitone-to-Hz conversion. At ±20 cent detune, the error is small but accumulates across 4 oscillators. Use `fastPow2(cents/1200) - 1.0f` for correctness.
3. **P2 — Seasonal Preset Gap**: The season system is this engine's most novel feature and there are no presets demonstrating it except a manually-set Winter. Add Spring/Summer/Fall presets with appropriate W/A/D parameter contexts.

## Top 3 Enhancements — ORCHARD

1. **Concertmaster Implementation**: The highest active voice at block rate should have slightly reduced LFO rate jitter, receive the SeasonBrightness bonus first, and have its LFO phase read by the other voices as a reference offset. Makes the "highest voice leads" metaphor real.
2. **Seasonal Preset Series**: Four presets, one per season — Spring (W=0, A=0, D=0.1, manual override season=0), Summer (high W, auto), Fall (moderate A override), Winter (high D, manual). Show the entire seasonal arc in the preset library.
3. **Dormancy Attack Noise**: The dormancy attack noise (`dormNoise`) is added when `dormancyPitchCents > 0.5 && growthTimer < 0.5` — but the `dormancyPitchCents` is set at note-on from the D accumulator, which means cold-start playing triggers this. A cold orchestra in a cold hall has the specific sound of rosined strings needing a moment to settle. This is a beautiful feature that should be more audible — increase dormancy noise amplitude from `0.1` to `0.2`.

---

# GARDEN CROSS-QUAD ASSESSMENT

## Growth Mode Differentiation Across the 4 String Engines

| Engine | Growth Mode Mechanism | Duration Range | Onset Character |
|--------|----------------------|----------------|-----------------|
| OXALIS | Partials activate sequentially in golden-angle order (1→7) | 3-60s | Instantaneous to gradual, mathematical |
| OVERGROW | Quadratic gain ramp on full KS output | 3-60s | Organic, single voice |
| OSIER | Per-voice onset staggered by role index (`roleDelay = vi * 0.15`) | 3-60s | Instrumental — bass enters last, soprano first |
| ORCHARD | Per-oscillator onset at 20% growth intervals; quadratic gain | 3-60s (20s default) | Ensemble buildup, 4 oscillators per voice |

**Differentiation is strong at the synthesis layer.** OXALIS grows by adding frequency-domain components (partials). OVERGROW grows by scaling a complete synthesis output. OSIER grows by staggering voice entries (ensemble building). ORCHARD grows by staggering oscillator entries within each voice (ensemble thickening).

The implementations are genuinely distinct. However, all four use the same `growthPhase^2` quadratic ramp as the envelope shape, which creates a shared perceptual similarity. Consider: OXALIS could use linear growth (mathematical), OVERGROW could use sigmoid (organic), OSIER could use per-role log curves (natural staggering), ORCHARD could use the quadratic (classic orchestral swell). Differentiated envelope shapes would make the four growth modes sonically and perceptually distinguishable even before the synthesis differences are heard.

---

## Strongest to Weakest — All 8 Engines

| Rank | Engine | Score | Key Strength | Critical Gap |
|------|--------|-------|-------------|-------------|
| 1 | OMEGA | 8.6 | Distillation model is synthesis-unique | Purity drift magnitude; operator feedback dead code |
| 2 | OAKEN | 8.4 | Most physically complete bass; curing model | String type LP coefficient wrong formula; room param misleading |
| 3 | OSIER | 8.3 | Companion planting is fleet-unique; quartet identity is fully realized | Role/pitch mismatch; one preset |
| 4 | ORCHARD | 8.2 | Orchestral climax role; seasonal arc | Concertmaster absent; one preset |
| 5 | OLATE | 8.1 | FermentationIntegrator is novel | Terroir East Coast/Japanese dead; terroir step-changes |
| 6 | OGRE | 7.9 | Tectonic LFO concept; correct sub physics | D004 soil bug; sub filter architecture |
| 7 | OXALIS | 7.8 | Phyllotaxis is synthesis-original; fleet-unique | setFundamental per-sample CPU; MW missing; 1 preset |
| 8 | OVERGROW | 7.5 | RunnerGenerator concept; strong wildness axis | Silence response dead; runner fade too slow; 1 preset |

---

## Fleet Comparison

CELLAR quad fleet average: **(8.25 / 10)** — slightly below fleet average of 8.7, primarily due to OGRE's soil bug and OLATE's terroir gap.

GARDEN quad fleet average: **(7.95 / 10)** — notably below fleet average, primarily due to thin preset coverage across all four engines (3 engines have only 1 preset each).

**Combined 8-engine average: ~8.1 / 10** — Below fleet average. The conceptual quality is consistently high; the implementation gaps and preset thinness are the primary reasons for underscoring.

---

## Priority Fix List — All 8 Engines

### P0 (Blocking — D004 violations and significant DSP errors)

| Engine | Fix | File |
|--------|-----|------|
| OGRE | Body filter double-pass bug (ogre_soil dead) — combine soil + envelope into single filter call | `Source/Engines/Ogre/OgreEngine.h` lines 376-395 |
| OVERGROW | Silence response dead code — implement silenceTimer behavior | `Source/Engines/Overgrow/OvergrowEngine.h` |
| OVERGROW | Runner fade too slow (`*= 0.99998f` → `*= 0.9998f`) | `Source/Engines/Overgrow/OvergrowEngine.h` RunnerGenerator |
| OLATE | Terroir East Coast + Japanese regions unimplemented (D004) | `Source/Engines/Olate/OlateEngine.h` lines 403-412 |
| OXALIS | setFundamental called per-sample with std::pow (CPU) | `Source/Engines/Oxalis/OxalisEngine.h` line 402 |

### P1 (Important — quality and correctness)

| Engine | Fix | File |
|--------|-----|------|
| OGRE | Sub filter applies to full combined signal, not sub-harmonic only | `Source/Engines/Ogre/OgreEngine.h` lines 357-361 |
| OLATE | Vintage step-change at 0.25/0.5/0.75 — add blend regions | `Source/Engines/Olate/OlateEngine.h` lines 268-282 |
| OLATE | Output level accumulation — add soft limiter | `Source/Engines/Olate/OlateEngine.h` |
| OAKEN | String type LP coefficient — replace Euler with matched-Z | `Source/Engines/Oaken/OakenEngine.h` OakenKarplusString::setStringType |
| OAKEN | Room parameter — add allpass diffuser for genuine room character | `Source/Engines/Oaken/OakenEngine.h` |
| OMEGA | Purity drift magnitude — scale down 10× or log-scale purity | `Source/Engines/Omega/OmegaEngine.h` lines 419-425 |
| OSIER | Role/pitch mismatch — pitch-aware voice allocation | `Source/Engines/Osier/OsierEngine.h` noteOn |
| OSIER | Companion pull cap — clamp at ±10 cents max | `Source/Engines/Osier/OsierEngine.h` lines 407-426 |
| ORCHARD | Concertmaster — implement or mark V2 explicitly | `Source/Engines/Orchard/OrchardEngine.h` |
| OXALIS | Mod wheel not routed to vibratoDepth | `Source/Engines/Oxalis/OxalisEngine.h` line 364 |
| OVERGROW | Bow noise injection — add to delay line not to output | `Source/Engines/Overgrow/OvergrowEngine.h` lines 491-494 |

### P2 (Polish)

| Engine | Fix |
|--------|-----|
| All 4 GARDEN | Preset library expansion (OXALIS, OVERGROW, OSIER each need minimum 4 more presets; ORCHARD needs seasonal arc presets) |
| OMEGA | Operator feedback field — clean up dead code |
| OMEGA | Alias protection ceiling on output filter |
| OAKEN | Extreme curing rate preset needed |
| OGRE | Extreme tectonic depth preset needed |
| OLATE | Session aging — compress to 5-7 minutes from 20 minutes |

---

## Blessing Candidates

| ID | Engine | Candidate | Status |
|----|--------|-----------|--------|
| B039a | OGRE | Tectonic LFO (geological timescale pitch drift) | Conditional — requires soil bug fix |
| B039b | OMEGA | Distillation Model (FM complexity→purity irreversible decay over sustain) | Strong candidate — ratification recommended |
| B040 | OXALIS | Phyllotaxis Harmonic Synthesis (golden ratio partial spacing as synthesis parameter) | Strong candidate — CPU fix needed first |

---

*CELLAR & GARDEN seance conducted 2026-03-21. Ghost council: Moog (DSP rigor), Buchla (conceptual boldness), Kakehashi (player experience / commercial viability), Smith (academic/physics grounding), Schulze (long-form composition), Vangelis (emotional immediacy), Tomita (orchestral perspective).*
