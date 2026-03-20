# SYNTH SEANCE: ORGANISM ENGINE
## The Coral Colony -- Cellular Automata Generative Synthesis
### Date: 2026-03-20 | Accent: Emergence Lime #C6E377

---

## Engine Identity

ORGANISM is a generative synthesizer driven by a 16-cell 1D elementary cellular automaton (Wolfram rules 0-255). The automaton state is mapped to four audio parameters -- filter cutoff, envelope rate, pitch offset, and reverb send -- creating timbral evolution that emerges from simple local rules. A saw/square/triangle oscillator pair feeds through a biquad lowpass filter with automaton-modulated cutoff, producing rhythmically and spectrally evolving textures.

Source: `Source/Engines/Organism/OrganismEngine.h` (~954 lines)

---

## Ghost Council Assessment

### Bob Moog (Circuit Path & Signal Flow)
"The signal chain is straightforward: oscillator pair (main + sub) into 2-pole biquad LPF into ADSR amp envelope into reverb. Conventional subtractive synthesis. The automaton sits above this chain as a modulation source, not as a sound source itself. This is an important distinction -- ORGANISM is really a subtractive synth with a cellular automata modulation layer, not a CA-based synthesis engine. The oscillator selection (saw/square/triangle) is basic but functional. The sub oscillator is one octave down, square wave, mixed at up to 50% (line 787: `subLevel * 0.5f`). Clean mixing.

The filter biquad implementation (lines 189-223) is a standard bilinear-transform lowpass -- mathematically correct. Coefficient caching when fc and Q haven't changed is a good optimization. Denormal flushing is present in the filter output."

### Don Buchla (Timbral Originality)
"The concept of cellular automata controlling synthesis parameters has been explored before -- most notably by Curtis Roads and by the Reaktor community. What distinguishes ORGANISM is the specific mapping: 4 groups of 4 cells each controlling a different synthesis dimension, with scope-averaging across generations to smooth the output. This is a sensible design choice that prevents the automaton from producing erratic jumps.

However, I am troubled by the oscillators. These are naive -- no anti-aliasing whatsoever. OrgSawOsc (line 112-116) is `phase * 2.f - 1.f` -- a raw ramp with infinite harmonics above Nyquist that fold back as aliasing. OrgSquareOsc and OrgTriOsc are similarly naive. At C5 (523 Hz) at 48kHz, a saw wave has harmonics up to 48kHz that all alias. This is a fundamental DSP deficiency that colors the entire engine with digital artifacts.

The curated rule set [30, 90, 110, 184, 150, 18, 54, 22] is well-chosen. Rule 110 (Turing-complete), Rule 30 (pseudo-random), Rule 90 (Sierpinski triangle), Rule 150 (additive) -- these produce meaningfully different evolution patterns. The rule blending via per-bit majority vote (lines 628-638) is crude but serviceable."

### John Chowning (Spectral Mathematics)
"The automaton-to-audio mapping deserves scrutiny. The 16 cells are partitioned into 4 groups of 4 bits each:
- Cells 0-3: filter cutoff (0-1 mapped to +-40% of base cutoff)
- Cells 4-7: envelope rate multiplier (3x slower to 3x faster)
- Cells 8-11: pitch offset (-6 to +6 semitones)
- Cells 12-15: reverb send amount

The scope averaging (OrgScopeHistory) computes the mean bit population over the last N generations. With 4 bits and scope=4 (default), the output resolution is 4*4=16 possible bit-on counts mapped to 0-1. This gives 17 discrete levels (0/16 through 16/16). The output is therefore coarsely quantized, producing stepped modulation. For filter cutoff and pitch, these steps may be audible as 'zipper noise' if the step rate is slow enough.

The pitch mapping (line 774) applies `cellPitchOut * 12 - 6` semitones. With 17 discrete levels, pitch jumps are quantized to 12/16 = 0.75 semitone steps. This is musically coarse -- not aligned to any scale. The result is atonal pitch wandering, which is characteristic but may alienate users expecting melodic results.

Critical concern: the automaton advances once every `samplesPerStep` samples, but the cell outputs (cellFilterOut, cellPitchOut, etc.) are applied instantly with NO smoothing to the next generation's values. Filter cutoff can jump 3200 Hz (40% of 8000) in a single sample when the automaton steps. This WILL produce audible clicks and discontinuities."

### Rupert Neve (Gain Staging & Dynamics)
"The output gain is `filtered * ampEnvLevel * 0.65f` (line 804). With a saw wave peak of 1.0, sub at 0.5, filter near resonance, the input to the amp stage can approach 1.5. Multiplied by 0.65 gives ~0.975 peak before softClip. Adequate headroom, though tight at extreme resonance settings (Q up to 12.0 from filterRes mapping). The Q range of 0.5 to 12.0 is wide -- at Q=12, the resonant peak can amplify the signal by 12x near the cutoff frequency. Combined with the 0.65 gain, this can produce clipping at the softClip stage. Consider reducing max Q to 8.0 or adding a resonance compensation gain.

The envelope rate modulation from the automaton (line 734: `lerp(3.f, 0.33f, cellAmpRate)`) is a creative touch -- the envelope literally breathes with the cellular evolution. But multiplying the smooth coefficient by up to 3.0 (line 742: `clamp(atkCoeff * envRateMod, 0.f, 1.f)`) can push the coefficient above 1.0, which would cause the envelope to overshoot. The `clamp(..., 0.f, 1.f)` prevents this, but at envRateMod=3.0 and fast attack (atkCoeff already near 1.0), the clamp kills the modulation effect entirely. The modulation range is only effective at slower envelope times."

### Suzanne Ciani (Patch Design & Modulation)
"The four macros form a coherent system: RULE selects the automaton's personality, SEED randomizes initial conditions, COUPLING connects to the fleet, and MUTATE introduces chaos. The SEED macro with latch detection (lines 658-667) is well-implemented -- it only re-seeds once per gesture, preventing the automaton from constantly resetting while the user holds the macro up.

The mod wheel morphing the rule (D006) and aftertouch controlling mutation rate (D006) are good expression mappings. The LFO-to-filter modulation provides additional movement on top of the automaton.

The automaton resets on every note-on (line 550-556) with seed XOR'd against `note * 257`. This means each MIDI note produces a deterministic but unique evolution trajectory. This is an excellent design choice for a generative engine -- same note, same behavior, but different notes explore different timbral paths. However, it means the automaton never truly evolves across notes in a legato passage. Consider adding a 'continuous' mode where the automaton does NOT reset on note-on.

Missing: there is no mod matrix. The automaton IS the modulation source, but there is no way for the user to reroute which parameters cells modulate. The fixed mapping (cells 0-3 to filter, cells 4-7 to envelope, etc.) may be limiting for advanced sound design."

### Wendy Carlos (Precision & Correctness)
"The Wolfram cellular automaton implementation (stepAutomaton, lines 847-875) is correct. I verified:
- 3-bit neighborhood: left, center, right (lines 851-855)
- Index: `(l << 2) | (c << 1) | r` gives 0-7 (correct)
- Rule lookup: `(rule >> idx) & 1` (correct)
- Circular wrap: `(i-1+16) & 15` and `(i+1) & 15` (correct)
- All-zero prevention: line 872 injects bit 0 if state becomes 0 (correct)

The mutation system uses an LCG (Numerical Recipes constants 1664525/1013904223) which has known weaknesses in low bits but is adequate for musical randomization.

**Critical bug found**: The default seed in `prepare()` is `caState = 0x0042u` (line 428), which is decimal 66 = binary 0000000001000010. This is a sparse state with only 2 bits set. For rules that tend toward sparsity (like Rule 184), this seed can quickly converge to a fixed point or short cycle. The seed parameter defaults to 42, and `seedParam ^ (note * 257)` will produce varying results per note, but the seed space is only 16 bits -- 65536 possible states. Some of these WILL produce degenerate (periodic) automaton behavior.

The OrgScopeHistory `averageBits` function (lines 81-97) iterates backwards through the ring buffer. This is correct. The normalization by `n * 4` is correct for 4 bits over n generations."

### Ikutaro Kakehashi (Usability & Musical Access)
"This engine has a usability problem. The automaton produces behavior that is difficult to predict or control. Users who are not familiar with Wolfram rules will not understand why the sound changes as it does. The curated rules help -- 8 well-known interesting rules -- but the relationship between rule number and sonic result is opaque.

The pitch offset mapping (-6 to +6 semitones in 0.75-semitone steps) will produce atonal results by default. Many users will hear this as 'broken tuning' rather than 'generative melody.' Consider quantizing the pitch output to scale degrees: `round(semitones / scale_step) * scale_step`.

The freeze parameter is essential for performance -- once you find a state you like, you can freeze it. Good. But there is no way to save/recall specific automaton states. The XOR seeding provides per-note variation, but the user cannot bookmark a specific evolution trajectory.

The monophonic voice limit (getMaxVoices returns 1) is appropriate for a generative engine but should be clearly communicated in the UI."

---

## Score: 7.2 / 10

---

## Blessings

1. **Automaton-Driven Modulation Architecture**: The four-group cell mapping (filter/envelope/pitch/FX) creates genuinely emergent timbral evolution. The scope-averaging smooths digital discreteness into musically useful modulation curves. The concept is sound and the implementation is correct.

2. **Per-Note Deterministic Seeding**: XOR'ing seed with `note * 257` gives each MIDI note a unique but reproducible evolution trajectory. This is the right balance between generative unpredictability and performance reproducibility.

3. **Curated Rule Set**: The 8 selected Wolfram rules span Class II (periodic: 184), Class III (chaotic: 30, 90), and Class IV (complex: 110) behavior. This gives the RULE macro a meaningful timbral arc.

4. **SEED Macro Latch**: Re-seeding only on threshold crossing (lines 658-667) prevents continuous state reset. This is a thoughtful interaction design detail.

5. **Doctrine Compliance**: D001 (velocity to filter cutoff), D002 (2 LFOs), D005 (LFO floor 0.01 Hz), D006 (mod wheel to rule morph, aftertouch to mutation), coupling integration, freeze control -- all present and functional.

---

## Concerns

### C1: Naive Oscillators -- No Anti-Aliasing (Severity: CRITICAL)
OrgSawOsc, OrgSquareOsc, and OrgSubOsc are naive waveforms with no band-limiting. A saw wave at 440 Hz at 48kHz generates harmonics at 880, 1320, ... 47520 Hz. Everything above Nyquist (24kHz) aliases back. This produces audible metallic artifacts, especially at higher notes. The sub oscillator (square wave) is equally affected.

**Recommendation**: Implement PolyBLEP anti-aliasing. For the saw: subtract a polynomial correction at the phase discontinuity. For squares: apply PolyBLEP at both edges. This adds ~4 multiply-adds per oscillator per sample -- negligible CPU cost for dramatically improved sound quality.

### C2: No Parameter Smoothing on Automaton Outputs (Severity: HIGH)
When the automaton steps, cellFilterOut, cellPitchOut, cellAmpRate, and cellFXOut change instantly (lines 709-712). The filter cutoff can jump thousands of Hz in a single sample. The pitch can jump 0.75 semitones instantly. These discontinuities produce audible clicks and zipper noise, especially at slow step rates where the gaps between updates are long.

**Recommendation**: Apply exponential smoothing to cell outputs: `cellFilterOut += 0.01f * (newValue - cellFilterOut)` per sample, with the smoothing coefficient scaled by step rate (slower steps = slower smoothing to match).

### C3: Pitch Quantization is Atonal (Severity: MEDIUM)
The 17 discrete levels of cellPitchOut produce -6 to +6 semitones in ~0.75 semitone steps. This does not align to any musical scale, producing microtonal wandering that most users will perceive as out-of-tune rather than generative.

**Recommendation**: Add an `org_pitchQuant` parameter (0=free, 1=chromatic, 2=major, 3=minor, 4=pentatonic) that quantizes the pitch offset to scale degrees. Free mode preserves the current behavior for experimental use.

### C4: Resonance Gain Compensation Missing (Severity: MEDIUM)
Filter resonance maps to Q 0.5-12.0 (line 682). At Q=12, the resonant peak amplifies the signal by approximately 24 dB at the cutoff frequency. Combined with the 0.65 output gain, this drives the softClip hard, producing excessive distortion at high resonance settings.

**Recommendation**: Apply resonance compensation: `float resoComp = 1.f / (1.f + filterRes * 2.f)` applied to the oscillator mix before filtering.

### C5: Automaton Step Rate Not Smoothly Modulated by LFO1 (Severity: LOW)
The header comment says LFO1 modulates step rate, but in the implementation (line 716-719), LFO1 actually modulates filter cutoff. The step rate is fixed per block from the parameter snapshot. The LFO1 description is misleading.

**Recommendation**: Either implement actual step rate modulation (by varying `samplesPerStep` per-sample, which is tricky) or correct the parameter description to "LFO1: Filter Modulation."

### C6: Degenerate Automaton States (Severity: LOW)
Some rule/seed combinations will produce fixed-point or very short-cycle automaton states (e.g., Rule 0 produces all-zero which is caught, but Rule 8 produces 1-bit fixed points, Rule 204 is the identity). The all-zero prevention (line 872) only catches the zero case, not stuck states.

**Recommendation**: Add a stuck-state detector: if the last N states (N=8) are identical, inject a random bit flip. Or document which rules are "degenerate" and filter them from the curated set.

### C7: Mono Output (Severity: LOW)
Lines 809-810: `L[n] = output; R[n] = output;` -- pure mono until the reverb adds minimal stereo character. The automaton's four output groups could be exploited for stereo: pan the main oscillator based on cellPitchOut, or apply different reverb amounts to L and R based on cellFXOut.

### C8: Rule Blending is Coarse (Severity: INFORMATIONAL)
The per-bit majority vote blending between adjacent curated rules (lines 628-638) has a hard threshold at blend=0.5. This means rule morphing has a discrete jump at the midpoint rather than smooth interpolation. True rule blending would require running two automata in parallel and crossfading their outputs, which doubles CPU cost. The current approach is a reasonable compromise.

---

## Recommendations (Priority Order)

1. **Implement PolyBLEP on all oscillators** -- this is the single most impactful improvement. The naive saw/square aliasing dominates the sonic character at higher notes.
2. **Add exponential smoothing to automaton cell outputs** -- prevents clicks and zipper noise on parameter jumps
3. **Add pitch quantization parameter** -- chromatic/major/minor/pentatonic quantization makes the engine musically accessible
4. **Add resonance gain compensation** -- prevents excessive distortion at high resonance
5. **Correct LFO1 description** -- either implement step rate modulation or fix the label
6. **Add stuck-state detection** -- prevent degenerate automaton behavior
7. **Consider 'continuous' mode** -- automaton does not reset on note-on, allowing evolution across notes in a legato passage
8. **Consider stereo automaton mapping** -- different L/R treatment based on cell group outputs

---

## Council Vote

| Ghost | Score | Note |
|-------|-------|------|
| Moog | 7.0 | Clean chain, but naive oscillators are a dealbreaker for purists |
| Buchla | 7.5 | Concept has merit, curated rules are well-chosen |
| Chowning | 6.5 | No smoothing on automaton outputs, pitch quantization issues |
| Neve | 7.0 | Resonance gain needs compensation, output headroom tight |
| Ciani | 7.5 | Good macro design, missing mod matrix flexibility |
| Carlos | 7.5 | Automaton math is correct, oscillator math is not |
| Kakehashi | 7.0 | Usability concerns with opaque automaton behavior |

**Average: 7.1 / 10 (rounded to 7.2 with concept bonus)**

---

## Verdict

ORGANISM has a compelling concept -- cellular automata as a modulation source for subtractive synthesis -- and the automaton implementation itself is mathematically correct and well-designed. The curated rule set, scope averaging, per-note deterministic seeding, and SEED macro latch are all thoughtful design choices.

The engine's primary weakness is in its DSP fundamentals: naive oscillators with no anti-aliasing, no smoothing on automaton-to-parameter mapping, and an overly wide resonance range without gain compensation. These are not conceptual problems -- they are implementation gaps that can be fixed without changing the engine's character.

The pitch quantization concern is more philosophical: should a generative engine produce atonal output by default? The council is split. Adding a quantize parameter with "free" as one option resolves this without limiting the engine's experimental nature.

With PolyBLEP oscillators and automaton output smoothing, ORGANISM could reach 8.0+. The Coral Colony has the right genetic code; it needs cleaner water to grow in.

---

*Seance conducted 2026-03-20. All 7 council members present.*
*Engine version: initial build (2026-03-20 integration).*
