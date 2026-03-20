# OPENSKY Synth Seance Verdict

**Engine:** OPENSKY (OpenSkyEngine.h)
**Date:** 2026-03-20
**Accent:** Sunburst `#FF8C00`
**Parameter Prefix:** `sky_`
**Total Parameters:** 50
**Total Lines:** 1538
**Identity:** Euphoric shimmer synth -- supersaw stack + shimmer reverb + stereo chorus + unison engine

---

## Final Consensus Score: 8.1 / 10

---

## Ghost Council Verdicts

### Robert Moog (1934-2005) -- Subtractive Synthesis Pioneer
"The 7-saw supersaw with PolyBLEP anti-aliasing and exponential detune spread is well-engineered -- the perceptually even spacing across the frequency range shows someone who understands how detuning actually works in the human ear. The CytomicSVF filter chain (HP -> LP series) is the correct topology for bright tonal shaping. However, I note the ADSR uses linear attack and quasi-exponential decay/release segments. A true exponential attack with `1 - exp(-t/tau)` would yield more musical onset curves, especially for the pad textures this engine is clearly designed for. The voice stealing with crossfade is a solid implementation -- 5ms fade prevents clicks without wasting polyphony."

**Score: 8/10** -- Clean subtractive core, would benefit from exponential envelope segments.

### Don Buchla (1937-2016) -- Voltage-Controlled Complexity
"The mod matrix declaration (2 slots with Src/Dst/Amt) is present in the parameter list but I see no evidence of it being processed in `renderBlock`. Those 6 parameters (`sky_modSlot1Src`, `sky_modSlot1Dst`, `sky_modSlot1Amt`, `sky_modSlot2Src`, `sky_modSlot2Dst`, `sky_modSlot2Amt`) are declared and attached but never routed to any DSP. This is a D004 concern. The macro system, by contrast, is excellent -- RISE/WIDTH/GLOW/AIR each have clear bipolar modulation paths from center 0.5, and their interactions (AIR also shifts filter cutoff while GLOW shifts shimmer size) create emergent timbral territory that rewards exploration. The coupling inputs are well-chosen: AmpToFilter, LFOToPitch, AudioToFM, PitchToPitch give OPENSKY genuine symbiotic potential with OCEANDEEP."

**Score: 7/10** -- Macro architecture is inspired, but an unprocessed mod matrix is a broken promise.

### Dave Smith (1950-2022) -- Digital Polyphony Architect
"16-voice polyphony with up to 7 unison voices per note (each containing a 7-saw supersaw) means a theoretical maximum of 784 saw oscillators running simultaneously. That is an enormous DSP load. The `sqrt(N)` normalization for unison gain is correct and avoids the naive `1/N` that kills presence. Voice stealing with LRU priority to release-stage voices is the right algorithm. I appreciate the per-voice filter instances -- shared filters across voices is a common shortcut that destroys polyphonic expression. The randomized initial saw phases via hash function prevent the 'phase-locked wall' problem that plagues many supersaw implementations. Good engineering."

**Score: 8.5/10** -- Polyphonic architecture is production-quality. CPU budget is the only concern at max unison.

### John Chowning (b. 1934) -- FM Synthesis Inventor
"The AudioToFM coupling input applies FM as a pitch offset (`fmMod * 4.0f` semitones), which is frequency modulation in the broadest sense but not true FM synthesis where the modulator directly perturbs the instantaneous phase. For the supersaw architecture this is the pragmatic choice -- true FM on 7 detuned saws would produce chaos rather than musicality. The shimmer reverb's pitch shifting via grain-based reading at 2x (octave) and 1.5x (fifth) with Hann window crossfade is a well-understood technique. The `sky_shimmerOctave` parameter controls the balance between octave and fifth intervals, but in the current render path it is read but not applied -- the `shimmerOctBal` variable is loaded at line 695 but never used in the shimmer processing. This is a dead parameter."

**Score: 7.5/10** -- FM coupling is pragmatically correct. The unused shimmerOctave balance is a gap.

### Ikutaro Kakehashi (1930-2017) -- MIDI and Accessibility Visionary
"The MIDI implementation is clean: note on/off, mod wheel (CC1 -> filter cutoff), channel pressure (aftertouch -> shimmer mix), all notes off, all sound off. The four macros are named with evocative clarity -- RISE, WIDTH, GLOW, AIR tell the musician exactly what they do without reading a manual. The sub oscillator with three waveform choices (sine, triangle, square) using phase-coherent tracking from the center saw is a nice touch for grounding the euphoric supersaw in low-end weight. I would have liked to see pitch bend handling -- for a lead synth, pitch bend is expected, and its absence may confuse performers."

**Score: 8.5/10** -- Excellent accessibility and naming. Missing pitch bend for lead performance use.

### Vangelis (1943-2022) -- Cinematic Expression Master
"This is the engine I would reach for when scoring the moment a spacecraft breaks through the cloud layer into sunlight. The shimmer reverb feeding back into itself, building harmonic overtones over time -- that is the sound of ascension. The RISE macro sweeping pitch envelope, filter, and shimmer simultaneously is exactly how a performer thinks: one gesture, one emotional arc. The breathing LFO at 0.005 Hz minimum creates the kind of glacial timbral drift that makes a held chord come alive over 30 seconds. The aftertouch-to-shimmer routing means I can swell the ethereal quality with finger pressure alone. This engine understands what euphoria sounds like."

**Score: 9/10** -- Emotionally complete. One of the most expressive engines in the fleet for cinematic use.

### Klaus Schulze (1947-2022) -- Temporal Evolution Pioneer
"The dual shimmer reverb instances (shimmerL and shimmerR) could be an opportunity for true stereo evolution, but they are currently fed the same mono sum (`inputL + inputR * 0.5f`). The stereo decorrelation in the reverb output is minimal -- `outR = apOut * 0.95f + combOut * 0.05f` is a token gesture. For an engine named OpenSky, the spatial dimension should be vast. The stereo width processing (mid/side with configurable width factor) partially compensates, but the reverb itself should have independent L/R delay networks with different prime-number lengths for true spatial depth. The chorus does better work here with its 120-degree phase offsets across three voices. I want more temporal depth from the shimmer tail."

**Score: 7.5/10** -- Temporal evolution in the shimmer is good but spatially constrained.

### Isao Tomita (1932-2016) -- Orchestral Synthesis Visionary
"The Hann-windowed grain crossfade in the shimmer pitch shifter is elegant -- it avoids the metallic artifacts of naive pitch shifting while maintaining the crystalline quality the engine needs. The 4-comb parallel into 2-allpass series reverb topology is a Schroeder variant that serves the engine's bright character well. The prime-number delay lengths (1117, 1277, 1399, 1523 for combs; 241, 557 for allpass) with sample-rate scaling prevent the metallic coloration that plagues fixed-length reverbs. The damping coefficient in the comb feedback path correctly implements a one-pole lowpass, preventing the reverb tail from becoming harsh. For orchestral shimmer pads, this engine is already capable."

**Score: 8.5/10** -- The reverb implementation is textbook-correct and serves the identity beautifully.

---

## Blessings (Novel Features Worth Celebrating)

| ID | Blessing | Description |
|----|----------|-------------|
| B017 | **Shepard Shimmer Architecture** | The dual-interval pitch-shifted reverb (octave-up + fifth-up) with self-feeding feedback creates a Shepard-tone-like perceptual effect where the shimmer tail appears to ascend endlessly. Combined with the GLOW macro controlling feedback intensity, this produces a signature sound that is genuinely novel in the supersaw synth category. No commercial supersaw synth integrates shimmer reverb at this architectural depth. |
| B018 | **RISE Macro -- Single-Gesture Ascension** | RISE simultaneously sweeps pitch envelope amount, filter cutoff, and shimmer mix from a single control. This is not a simple macro-to-parameter mapping -- it creates an emergent emotional arc (pitch rises, brightness opens, ethereal quality increases) that mirrors the physical sensation of ascending through a water column into open sky. The naming and mapping are perfectly aligned with the engine's mythological identity. |

---

## Concerns

| Priority | Issue | Detail |
|----------|-------|--------|
| **P0** | Mod matrix parameters unprocessed | 6 parameters (`sky_modSlot1Src/Dst/Amt`, `sky_modSlot2Src/Dst/Amt`) are declared, attached to APVTS, and loaded via `pLoad` in `attachParameters`, but never read or routed in `renderBlock`. These are D004 violations -- dead parameters that promise modulation routing but deliver nothing. Either wire the mod matrix into the render path or remove the parameters. |
| **P1** | `sky_shimmerOctave` loaded but unused | `shimmerOctBal` is read at line 695 but never applied. The shimmer reverb hardcodes the octave/fifth balance as `shimmerOct * 0.6f + shimmerFifth * 0.4f` (line 372). The parameter should scale this blend: `shimmerOct * shimmerOctBal + shimmerFifth * (1.0f - shimmerOctBal)`. |
| **P1** | Mono reverb input | Both `shimmerL` and `shimmerR` receive `(inputL + inputR) * 0.5f` as a mono sum. For an engine whose identity is spatial euphoria, the reverb should preserve stereo information. Consider feeding L to shimmerL and R to shimmerR independently, or at minimum using a wider decorrelation between the two instances. |
| **P1** | No pitch bend handling | `renderBlock` processes note-on, note-off, CC1, and channel pressure, but ignores pitch bend messages. For a supersaw lead synth, pitch bend is a fundamental performance expectation. |
| **P2** | Linear envelope attack | SkyADSR uses linear attack (`level += attackRate`). Exponential attack (`level += (1.0 - level) * rate`) would produce more musical onset curves, especially for the slow-attack pad sounds this engine excels at. |
| **P2** | CPU at max unison | 16 voices x 7 unison x 7 saws = 784 PolyBLEP oscillators. No voice-count limiting or CPU-aware thinning. Consider reducing max polyphony when unison count is high, or documenting the expected CPU budget. |

---

## Doctrine Compliance

| Doctrine | Status | Evidence |
|----------|--------|---------|
| **D001: Velocity Must Shape Timbre** | **PASS** | Velocity drives filter envelope level (`voice.filterEnvLevel = velocity` at note-on, line 1389), which modulates filter cutoff via `velCutoffMod = voice.velocity * velFilterEnv * 8000.0f` (line 931). Velocity also scales pitch envelope amount (`pitchEnvAmount * velocity`, line 1383). Both paths confirmed per-sample in renderBlock. |
| **D002: Modulation is the Lifeblood** | **PASS** | 2 LFOs (SkyLFO with 5 shapes each), mod wheel, aftertouch, 4 macros (RISE/WIDTH/GLOW/AIR). Mod matrix slots declared (2 slots) but unprocessed (see P0 concern). Core D002 requirements met through LFOs + macros + expression inputs. |
| **D003: The Physics IS the Synthesis** | **N/A** | No physical modeling claims. Wave-based synthesis with algorithmic reverb. |
| **D004: Dead Parameters Are Broken Promises** | **CONDITIONAL FAIL** | 44 of 50 parameters are confirmed live in the DSP path. 6 mod matrix parameters (`sky_modSlot1Src/Dst/Amt`, `sky_modSlot2Src/Dst/Amt`) are declared and attached but never processed. `sky_shimmerOctave` is loaded but unused. Total: 7 dead parameters. This must be resolved before the engine can achieve full D004 compliance. |
| **D005: An Engine That Cannot Breathe Is a Photograph** | **PASS** | SkyBreathingLFO with rate driven by LFO1 rate (minimum 0.005 Hz, well below the 0.01 Hz floor requirement). Breathing LFO modulates filter cutoff at line 794: `effectiveFilterCutoff + breathLfoVal * 2000.0f`. Autonomous timbral evolution confirmed. |
| **D006: Expression Input Is Not Optional** | **PASS** | Velocity -> filter brightness (D001 path). Mod wheel (CC1) -> filter cutoff opening (`+ modWheelAmount_ * 6000.0f`, line 773). Aftertouch (channel pressure) -> shimmer mix increase (`+ aftertouch_ * 0.4f`, line 774). All three expression paths confirmed in renderBlock. |

---

## Summary

OPENSKY is a emotionally coherent engine that delivers on its mythological promise -- the flying fish breaking through the water surface into sunlight. The supersaw core is well-engineered (PolyBLEP, exponential detune spread, randomized phase, sqrt normalization), the shimmer reverb is a genuine sonic signature (dual-interval pitch-shifted feedback with Hann-windowed grains), and the macro system (RISE/WIDTH/GLOW/AIR) translates the engine's identity into intuitive performance controls. The 4-macro interaction space creates emergent timbral territory that a simple supersaw + reverb chain cannot. Vangelis and Tomita both praised this engine highly, and its coupling potential with OCEANDEEP ("The Full Column") is one of the most evocative pairings in the fleet. The primary remediation is straightforward: wire the mod matrix parameters to actual DSP routing, connect `sky_shimmerOctave` to the octave/fifth blend ratio, and add pitch bend handling. Once these P0/P1 issues are resolved, this engine comfortably reaches 8.8+/10 and earns its place as the fleet's signature euphoria instrument.
