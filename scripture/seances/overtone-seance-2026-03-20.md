# SYNTH SEANCE: OVERTONE ENGINE
## The Nautilus -- Continued Fraction Spectral Synthesis
### Date: 2026-03-20 | Accent: Spectral Ice #A8D8EA

---

## Engine Identity

OVERTONE is an additive synthesizer whose partial frequency ratios are derived from the convergent sequences of mathematical constants (Pi, E, Phi, Sqrt2). Eight phase-accumulator sine oscillators are tuned not to integer harmonics but to continued fraction convergents, producing spectra that range from near-harmonic to subtly metallic/irrational. The DEPTH macro sweeps through convergent table entries, morphing from clean ratios toward the irrational limit.

Source: `Source/Engines/Overtone/OvertoneEngine.h` (~889 lines)

---

## Ghost Council Assessment

### Bob Moog (Circuit Path & Signal Flow)
"The signal flow is admirably clean: 8 additive sines through a Butterworth LPF, then an allpass resonator, then Schroeder reverb. No unnecessary routing complexity. The phase-accumulator oscillators are the right choice for pure additive work -- no aliasing from the oscillators themselves, since they are pure sines. However, I note that with 8 partials at max amplitude you could reach approximately 2.87 peak, and your normalization divides by 6. This is conservative but correct for preventing clipping. The softClip after envelope application is a good safety net. Sound engineering."

### Don Buchla (Timbral Originality)
"The continued fraction concept is genuinely original -- I have not encountered another synthesizer that tunes partials to convergent sequences of irrational numbers. The Phi table (Fibonacci ratios) is particularly compelling: 1.0, 2.0, 1.5, 1.667, 1.6, 1.625, 1.615, 1.619 -- these produce a spectrum that is neither harmonic nor random, but structurally *golden*. The Pi ratios concern me: entries 0-5 are all clustered between 1.0 and 1.0476. This means at low depth, 6 of 8 partials are nearly at the fundamental. The spectral content collapses to a near-unison. Entry 6 jumps to 2.094 and entry 7 to 3.14159 -- a discontinuous leap that will produce audible timbral jumps when sweeping depth."

### John Chowning (Spectral Mathematics)
"The mathematical foundation is sound but the Pi table normalization is problematic. By dividing all Pi convergents by 3.0 (the first convergent), you get ratios hovering around 1.047 for entries 1-5. This is nearly equal temperament's minor second (ratio 1.0595). Six partials beating within 5% of the fundamental produces a thick unison rather than spectral complexity. E, Phi, and Sqrt2 tables have much better spectral spread and are more musically interesting.

For the interpolation between table entries (lines 737-740, 746-750): this is correct -- linearly interpolating between convergent ratios gives smooth depth sweeps. The wrap-around indexing `(depthLo + i) & 7` means each partial reads a different table entry, which is the right approach for spectral diversity.

One concern: there is NO anti-aliasing on the partial frequencies. Line 754 clamps to `sr * 0.47f` (Nyquist margin), but this is a hard cutoff. A partial at 22kHz at 48kHz sample rate will be at 0.458 Nyquist -- audible aliasing from the fastSin approximation error at these frequencies. The fastSin has ~0.02% error which translates to spurious harmonics at high frequencies."

### Rupert Neve (Gain Staging & Dynamics)
"The gain staging is reasonable. 1/6 normalization for 8 partials with 1/n falloff means peak output before envelope is approximately 0.48. After softClip this will never exceed 1.0. The Butterworth filter is correctly implemented with bilinear transform. The minimum cutoff of 80 Hz (line 335) is appropriate -- prevents the filter from attenuating the fundamental on bass notes. The velocity-to-brightness mapping (D001) adds up to 0.6 * velBright to upper partial amplitudes, which is musically sensible. I would prefer to see filter envelope depth rather than just static velocity scaling, but the LFO modulation partially compensates."

### Suzanne Ciani (Patch Design & Modulation)
"The LFO architecture is adequate but not inspired. LFO1 sweeps the depth index (convergent position) and LFO2 applies phase rotation to partials. The phase rotation effect (line 776) is very subtle -- `lfo2Out * (i+1) * 0.0001f` per sample. At 48kHz and full lfo2Depth, partial 7 gets 8 * 0.0001 = 0.0008 phase advance per sample = 38.4 Hz equivalent rotation. This will produce a gentle chorusing effect. Good.

The COLOR macro boosting upper partials is a simple but effective timbral control. The SPACE macro's connection to both resonator mix and reverb depth is well-designed -- a single gesture takes you from intimate to vast.

Missing: there is no filter envelope. Attack transients always have the same spectral content as sustain. For a spectral engine, this is a significant omission."

### Wendy Carlos (Precision & Correctness)
"The convergent ratio tables are mathematically correct. I verified:
- Pi: 22/7 / 3 = 1.04762 (correct), 355/113 / 3 = 1.04734... wait. 355/113 = 3.14159292... / 3 = 1.04719764. The table says 1.047493. This is wrong. 355/339 = 1.04720 but 355/113 / 3 = 1.04720. The comment says '355/339' which is incorrect -- it should be '355/113 / 3 = 355/339'. Actually 3 * 113 = 339, so 355/339 IS 355/113/3. The value 1.047493 is slightly off -- the correct value is 1.047198. Table entry [3] is therefore inaccurate by 0.0003.

The Phi and Sqrt2 tables are correct (verified against OEIS).

The E table: 8/3 / 2 = 8/6 = 1.3333 (correct). 11/4 / 2 = 11/8 = 1.375 (correct). All verified.

The allpass resonator (OverAllpassReso) uses integer delay length with no fractional interpolation. For a fundamental at 261.6 Hz (middle C) at 48kHz, the delay is 183 samples = 262.3 Hz. The 0.7 Hz detuning is inaudible. At 440 Hz: 109 samples = 440.4 Hz -- also acceptable. At 4186 Hz (C8): 11 samples = 4363.6 Hz -- a 4.2% error. The resonator becomes increasingly detuned at high notes. This needs fractional delay interpolation."

### Ikutaro Kakehashi (Usability & Musical Access)
"The macros are well-designed for exploration. DEPTH is the soul of the engine -- sweeping from clean to irrational. COLOR brightens upper partials. SPACE adds reverb. COUPLING connects to the fleet. These are intuitive.

The constant selector (Pi/E/Phi/Sqrt2) is a unique creative parameter, but the Pi setting will disappoint users due to the collapsed spectral spread issue noted above. Default to Phi (index 2) was the right choice -- Fibonacci ratios are the most musically diverse of the four tables.

8 individual partial amplitude sliders give detailed control. This is good for sound design but may overwhelm casual users. The default 1/n falloff is a safe starting point."

---

## Score: 7.6 / 10

---

## Blessings

1. **Continued Fraction Concept (potential B-class)**: The idea of tuning partials to convergent sequences of irrational numbers is genuinely novel. The Phi (Fibonacci) and Sqrt2 tables produce spectra that are mathematically rigorous and musically distinctive -- neither harmonic nor random, but *convergent*. No other synthesizer does this.

2. **Smooth Depth Interpolation**: Linear interpolation between adjacent table entries (lines 737-750) prevents audible stepping when sweeping the depth macro. The wrap-around indexing gives each partial a unique ratio, maximizing spectral diversity at any depth setting.

3. **Doctrine Compliance**: D001 (velocity to brightness via upper partial boost), D002 (2 LFOs), D005 (LFO floor 0.01 Hz), D006 (mod wheel to depth, aftertouch to color), coupling integration (AmpToFilter, PitchToPitch, EnvToMorph) -- all present and functional.

4. **Clean Signal Architecture**: Pure additive sines have zero oscillator aliasing. Phase-accumulator design is CPU-efficient. Denormal flushing in all feedback paths. SilenceGate integration.

5. **LFO2 Phase Rotation**: The per-partial phase offset creates a subtle spectral chorus that is unique to additive synthesis -- each partial detuned by a different amount, producing living shimmer rather than static spectra.

---

## Concerns

### C1: Pi Table Spectral Collapse (Severity: HIGH)
Entries 0-5 of kPiRatios are all between 1.0 and 1.0476. Six of eight partials within 5% of the fundamental produces a thick unison, not meaningful spectral content. The jump to 2.094 at index 6 and 3.14159 at index 7 creates a discontinuous timbral break when sweeping depth. This makes the Pi constant significantly less useful than Phi, E, or Sqrt2.

**Recommendation**: Replace the Pi table with better-spread convergent-derived ratios. Consider using `convergent[n]/convergent[0]` without renormalization, or use the actual continued fraction partial quotients [3, 7, 15, 1, 292...] as harmonic multipliers (e.g., 1.0, 3.0/1, 7.0/3, 15.0/7...) to get wider spectral spread.

### C2: No Anti-Aliasing on Partials (Severity: MEDIUM)
Partials are clamped to `sr * 0.47f` (line 754) but there is no amplitude fadeout near Nyquist. A partial sitting at 22.5 kHz at 48kHz SR contributes full amplitude despite being in the aliasing danger zone. The fastSin approximation error (~0.02%) generates spurious harmonics that fold back.

**Recommendation**: Apply a soft gain rolloff as partial frequency approaches Nyquist: `amp *= clamp(1.0f - (freq / (sr * 0.5f) - 0.85f) * 6.667f, 0.0f, 1.0f)`. This fades partials to silence between 0.85*Nyquist and 1.0*Nyquist.

### C3: No Filter Envelope (Severity: MEDIUM)
The brightness filter is statically positioned per block (cutoff + velocity + macro offsets). There is no per-note filter envelope. For a spectral engine, the attack transient should be spectrally different from the sustain -- brighter on attack, settling to the base cutoff. This is standard subtractive synthesis practice and its absence makes patches sound less alive.

**Recommendation**: Add `over_fltEnvAmt` (0-1) and `over_fltEnvDec` (0.01-2.0s) parameters. Apply a simple AD envelope to filter cutoff: `finalCutoff += fltEnvLevel * fltEnvAmt * 8000.f`.

### C4: Allpass Resonator Has No Fractional Delay (Severity: LOW)
The OverAllpassReso uses integer sample delay, which detunes at high frequencies (4.2% error at C8). For a spectral engine where pitch precision matters, this is audible.

**Recommendation**: Add linear interpolation between adjacent delay buffer samples. This costs one multiply-add per sample.

### C5: Pi Table Entry [3] Numerical Error (Severity: LOW)
kPiRatios[3] = 1.047493f but the correct value of 355/113/3 = 1.047198. The error is 0.03% -- below audibility for most purposes but violates D003 (physics rigor).

### C6: Monophonic Output (Severity: LOW)
Lines 800-801: `L[n] = output; R[n] = output;` -- pure mono until the reverb stage adds stereo spread. The LFO2 phase rotation could be exploited for true stereo by applying different rotation amounts to L and R channels, or by panning alternating partials.

### C7: Per-Sample Partial Computation (Severity: INFORMATIONAL)
All 8 partials are computed per-sample (line 744-781 inner loop). At 48kHz this is 384,000 sine evaluations per second per voice. fastSin is cheap (~4 multiplies) so this totals roughly 1.5M multiply-adds per voice. With 8-voice polyphony, budget is ~12M multiply-adds/sec -- well within modern CPU budget. No optimization needed unless targeting mobile.

---

## Recommendations (Priority Order)

1. **Fix Pi table** -- either redesign with better spectral spread or document it as "experimental/tight unison" and default to Phi
2. **Add Nyquist fade** for partial amplitudes approaching sr/2
3. **Add filter envelope** (AD or ADSR) with amount parameter
4. **Add fractional delay** to allpass resonator
5. **Correct Pi table entry [3]** to 1.047198f
6. **Consider stereo partial panning** -- odds to L, evens to R, with width parameter
7. **Consider per-partial detuning parameter** -- `over_spread` that multiplies each ratio by `1 + spread * i * 0.001` for micro-detuning

---

## Council Vote

| Ghost | Score | Note |
|-------|-------|------|
| Moog | 8.0 | Clean architecture, good gain staging |
| Buchla | 7.5 | Original concept, Pi table needs work |
| Chowning | 7.0 | Math is mostly sound, anti-aliasing gap |
| Neve | 8.0 | Good dynamics, conservative but correct |
| Ciani | 7.0 | Needs filter envelope for expressiveness |
| Carlos | 7.5 | Tables mostly correct, resonator detuning |
| Kakehashi | 8.0 | Intuitive macros, good defaults |

**Average: 7.6 / 10**

---

## Verdict

OVERTONE is a conceptually brilliant engine with a genuinely novel synthesis approach. The continued fraction convergent tables -- particularly Phi (Fibonacci) and Sqrt2 -- produce spectra that are mathematically rigorous and musically unique. The architecture is clean, the doctrine compliance is complete, and the macro design is intuitive.

The primary weaknesses are: (1) the Pi table's collapsed spectral spread, which makes one of four available constants significantly less useful; (2) the lack of anti-aliasing fadeout near Nyquist; and (3) the absence of a filter envelope, which limits the expressiveness of attack transients.

With the Pi table redesign and filter envelope addition, this engine could reach 8.5+. The core concept -- irrationally tuned additive partials -- is a keeper. The Nautilus has a beautiful shell; it needs a few more chambers.

---

*Seance conducted 2026-03-20. All 7 council members present.*
*Engine version: initial build (2026-03-20 integration).*
