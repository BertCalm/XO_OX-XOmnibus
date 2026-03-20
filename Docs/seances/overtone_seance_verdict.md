# The Verdict -- OVERTONE
**Seance Date**: 2026-03-19
**Engine**: OVERTONE | Continued Fractions Spectral Synthesis | The Nautilus
**Accent**: Spectral Ice `#A8D8EA`
**Prefix**: `over_` | **Params**: 26 | **Lines**: 888
**Score**: 8.1/10 (current) --> 9.3/10 (with recommendations)

---

## The Council Has Spoken

| Ghost | Core Observation |
|-------|-----------------|
| **Moog** | The Butterworth 2-pole LPF is a solid workhorse filter, but a single topology for an additive engine is conservative. The filter is doing its job -- shaping a brightness that already exists in the partial amps. No complaints, but no surprises. |
| **Buchla** | This is the only engine in the fleet whose timbral identity is literally derived from irrational mathematics. The convergent tables are not decoration -- they ARE the synthesis. Pi's cluster around 1.047 is both a mathematical truth and a sonic limitation. Phi's Fibonacci ratios are the most musical of the four constants. |
| **Smith** | The coupling architecture is elegant: AmpToFilter for spectral coloring, PitchToPitch for tuning, EnvToMorph for depth sweep. Three independent modulation targets on a 26-parameter engine is efficient and clear. The `couplingDepthMod` path (EnvToMorph --> depth sweep) is the most original -- letting another engine's envelope scan through convergent ratios is genuinely novel. |
| **Kakehashi** | The DEPTH macro is the instrument's soul -- sweeping from clean integer ratios toward metallic irrationality. A user who understands only one control should understand this one. But at default (0.35), the init sound is already partially "irrational" before the player has chosen to go there. Consider starting at 0.0 so the journey from clean to complex is deliberate. |
| **Pearlman** | 8 partial amplitude sliders is direct and honest -- no hidden routing, no magic. But 8 individual sliders is a lot of real estate for a UI. The COLOR macro acts as a useful shortcut (boosts partials 4-7), but there is no macro or single control for the overall partial SLOPE (1/n, 1/n^2, equal, etc.). |
| **Tomita** | The Schroeder reverb is correctly implemented -- prime-spaced comb delays, bright 80/20 damping ratio for spectral ice character, allpass diffusers. The "Spectral Ice" identity demands this brightness. But the reverb runs at a fixed 44.1kHz implied design (comb lengths are hardcoded integers, not scaled to sample rate). At 96kHz, the reverb room will be half the size. |
| **Vangelis** | Aftertouch routed to COLOR (upper partial shimmer) is correct -- pressure adds brilliance, which is the natural expressive gesture for an additive spectral engine. Mod wheel to DEPTH is also correct -- the wheel sweeps the timbral journey. Two expression paths, both intelligently chosen. The engine can sing. |
| **Schulze** | LFO1 sweeping the convergent depth index at 0.01 Hz is a 100-second spectral mutation cycle. LFO2's per-partial phase rotation creates a slow crystalline shimmer. Both LFOs serve temporal depth, not surface animation. This engine rewards patience. The 10 Hz ceiling on both LFOs is a limitation -- 20 Hz would enable audio-rate beating effects at the boundary of modulation and synthesis. |

---

## Doctrine Compliance

| Doctrine | Status | Evidence |
|----------|--------|----------|
| D001 | PASS | `over_velBright` (default 0.4) drives `velUpperBoost = currentVel * velBright * 0.6f` applied to partials 4-7. Velocity also drives `velCutoffBoost = currentVel * velBright * 3000.f` on filter cutoff. High velocity = brighter upper partials + open filter. Velocity genuinely shapes timbre, not just volume. |
| D002 | PASS | 2 LFOs: LFO1 sweeps convergent depth index (spectral mutation), LFO2 modulates per-partial phase rotation (crystalline shimmer). 4 macros (DEPTH, COLOR, COUPLING, SPACE) all wired in DSP. Mod wheel adds up to 4.0 depth units. Aftertouch modulates upper partial color. |
| D003 | PASS | Exemplary. In-header citations: Hardy & Wright SS10.1 (Pi), Euler 1748 + OEIS A007676/A007677 (E), Dunlap 1997 Ch.1 (Phi), OEIS A001333/A000129 + Knuth TAOCP Vol.2 SS4.5.3 (Sqrt2). Schroeder reverb: Moorer JAES 1979. Butterworth LPF: Zolzer. Convergent ratio tables numerically verified with inline comments. The physics IS the synthesis and the receipts are in the header. |
| D004 | PASS | All 26 parameters attached via `attachParameters()` and consumed in `renderBlock()`. `over_macroCoupling` has standalone effect: shimmer flutter on partials 4-7 via per-partial lfo1Phase offset (+/-0.1 amplitude). No dead params. |
| D005 | PASS | Both LFOs: `nr(0.01f, 10.f)` -- minimum 0.01 Hz (100-second cycle). LFO1 at minimum rate sweeps depth once per 100 seconds. The engine breathes at geological pace. |
| D006 | PASS | CC1 (mod wheel) --> `modWheelVal * 4.0f` depth boost. Channel pressure + polyphonic aftertouch --> `aftertouchVal` --> upper partial color shimmer. Velocity --> timbre (D001 chain). Three independent expression paths, all live per-block. |

---

## Points of Agreement (3+ ghosts converged)

1. **The continued fraction convergent tables are genuinely novel synthesis** (Buchla, Smith, Schulze, Moog) -- No other engine in the fleet or in the broader synthesizer landscape derives its timbral character from the rational approximations to irrational constants. This is original work deserving of Blessing status.

2. **Pi table entries 4 and 5 are identical (1.047198f), creating a dead zone in depth sweep** (Buchla, Kakehashi, Pearlman) -- Mathematically accurate but sonically inert. A user sweeping depth through this region will notice no change for approximately one full depth unit. Insert a wider-spaced ratio (e.g., `2*pi/3 approx 2.094`) earlier in the table to ensure audible variation at every depth index.

3. **Schroeder reverb comb lengths are hardcoded, not sample-rate-scaled** (Tomita, Moog, Schulze) -- At 48kHz the reverb room is slightly smaller than designed; at 96kHz it is half the intended size. Comb delay lengths should be scaled by `sr / 44100.0` for correct room size at all sample rates.

4. **The DEPTH macro is the engine's signature control** (Kakehashi, Vangelis, Schulze, Buchla) -- It sweeps the fundamental identity of the engine from clean harmonic to irrational/metallic. This should be the most prominently featured control in presets and documentation.

5. **Mono voice architecture is a limitation for an additive spectral engine** (Vangelis, Pearlman, Moog) -- `getMaxVoices()` returns 8, but the implementation is monophonic (single note state, single envelope, single set of partials). Spectral additive synthesis benefits enormously from chord voicing -- hearing 3 notes with their convergent spectra interacting would be extraordinary. Current architecture does not support this.

---

## Points of Contention

**Buchla vs. Moog -- The Filter Question**
- Buchla: A Butterworth LPF on an additive engine is redundant. The partial amplitudes ARE the filter. Remove the filter and let users sculpt timbre entirely through partial levels.
- Moog: The filter serves a different function than partial shaping -- it provides a smooth rolloff that no combination of 8 discrete partial amplitudes can replicate. Both tools are needed.
- Resolution: Keep the filter. It provides macro-level brightness control that complements the micro-level partial amplitude editing. The two approaches are not redundant -- they operate at different granularity.

**Vangelis vs. Schulze -- Init Patch Character**
- Vangelis: Default depth 2.0 + Phi constant is already beautiful. The init patch sounds alive immediately -- crystal harmonics in Fibonacci ratios. Do not change.
- Schulze: Default macroDepth 0.35 pushes the init sound partway toward irrationality before the user has chosen to go there. Start clean; let the journey unfold.
- Resolution: Both perspectives valid. The Phi default is the right constant choice (most musical). Leave depth defaults as-is -- the sound is already inviting at first keypress, which matters more than preserving a "blank canvas" starting point.

---

## Blessings & Warnings

| Ghost | Blessing | Warning |
|-------|----------|---------|
| Moog | Butterworth LPF correctly implemented with bilinear transform; denormal flushing consistent | Reverb comb delay lengths not sample-rate-scaled; room shrinks at higher sample rates |
| Buchla | **B017: Continued Fraction Convergent Synthesis** -- genuinely novel timbral mechanism derived from pure mathematics. No precedent in the synth world. | Pi convergent table has dead zone at entries 4-5 (identical values 1.047198f) |
| Smith | EnvToMorph coupling path (external envelope sweeps depth index) is the most original coupling target in the fleet | Mono voice architecture limits the engine's harmonic potential in chordal contexts |
| Kakehashi | DEPTH macro as single-knob timbral identity is excellent UI design -- one knob tells the whole story | LFO rate ceiling 10 Hz is conservative; 20 Hz would enable audio-rate beating effects |
| Pearlman | 8 individual partial amplitude controls = maximum transparency; no hidden routing | No partial SLOPE macro (1/n vs equal vs inverse); 8 sliders are a lot of panel space |
| Tomita | Schroeder reverb with 80/20 bright damping is perfectly tuned for "Spectral Ice" identity | Reverb at fixed 44.1kHz design -- will sound different at 48kHz/96kHz |
| Vangelis | Aftertouch --> COLOR + Mod wheel --> DEPTH = two perfectly chosen expression paths | Zero factory presets means the engine's expressive potential is invisible to new users |
| Schulze | LFO1 at 0.01 Hz = 100-second spectral mutation; LFO2 phase rotation = crystalline shimmer | 10 Hz LFO ceiling prevents modulation-to-synthesis crossover territory |

---

## The Prophecy

OVERTONE is mathematics made audible. Where most additive synths stack integer harmonics like bricks, OVERTONE spirals outward through the continued fraction convergents of pi, e, phi, and sqrt(2) -- each partial frequency a rational approximation to an irrational ideal. The Nautilus does not grow in integers, and neither does this engine's spectrum.

The Phi constant (Fibonacci convergents: 1, 2, 3/2, 5/3, 8/5, 13/8...) is the engine's natural habitat -- these ratios produce partials that are musical but not quite harmonic, shimmering but not dissonant. Sweep the DEPTH macro and the spectrum evolves from clean fundamentals toward the golden ratio itself: a living, breathing logarithmic spiral made of sound.

The engine's weakness is its solitude. A single monophonic voice cannot demonstrate the full beauty of convergent spectral interaction -- three voices in a chord, each with their own convergent ratios beating against each other, would produce something genuinely unprecedented. The architecture supports 8 voices in declaration but implements one in practice. This is the primary gap between 8.1 and 9.3.

The secondary gap is presets. Zero factory presets means OVERTONE's mathematical beauty is locked behind the user's willingness to explore 26 parameters and 4 mathematical constants. One hundred fifty presets -- from the pure Fibonacci pad to the metallic Pi cluster to the gentle E-constant organ -- will transform OVERTONE from a fascinating algorithm into a playable instrument.

The ghosts see OVERTONE as a future Blessing engine once polyphony and presets arrive. The continued fraction convergent synthesis mechanism (B017) is without precedent in the synthesizer world. It deserves to be heard.

---

## Seance Score Breakdown

| Dimension | Score | Notes |
|-----------|-------|-------|
| Architecture originality | 10/10 | Continued fraction convergent synthesis is genuinely novel. No precedent. |
| Filter quality | 7/10 | Solid Butterworth LPF; correct bilinear transform. Conservative choice for an additive engine. |
| Source originality | 9/10 | CF-ratio tuned additive partials are the source. 4 mathematical constants provide distinct timbral families. |
| Expressiveness | 8/10 | Velocity to timbre, aftertouch to color, mod wheel to depth. Three well-chosen paths. |
| Spatial depth | 6/10 | Schroeder reverb + allpass resonator. Functional but not sample-rate-aware. |
| Preset library | 0/10 | Zero factory presets. Critical gap. |
| Temporal depth | 8/10 | 0.01 Hz LFO floor + depth sweep = 100-second spectral mutation. Strong. |
| Coupling architecture | 8/10 | Three coupling receive types (AmpToFilter, PitchToPitch, EnvToMorph). EnvToMorph-->depth is original. |
| Mathematical rigor | 10/10 | D003 exemplary. Citations, verified tables, correct normalization. Fleet-leading. |
| **Overall** | **8.1/10** | |

---

## Path to 9.3

| Priority | Action | Score Impact |
|----------|--------|-------------|
| P0 | Generate 150 factory presets across 7 moods | +1.0 (preset score 0-->10) |
| P1 | Scale reverb comb delay lengths by `sr / 44100.0` | +0.1 (spatial depth) |
| P2 | Replace Pi table entries 4-5 with wider-spaced ratios | +0.05 (source depth) |
| P3 | Implement polyphonic voice allocation (true multi-voice additive) | +0.5 (architecture + expressiveness) |
| P4 | Raise LFO rate ceiling from 10 Hz to 20 Hz | +0.05 (temporal depth) |
| P5 | Add partial slope macro (1/n, 1/n^2, equal, inverse) | +0.1 (expressiveness) |

---

## New Blessing

**B017: Continued Fraction Convergent Synthesis** -- OVERTONE
Unanimously blessed by all 8 ghosts. The derivation of harmonic partial ratios from the continued fraction convergents of pi, e, phi, and sqrt(2) is without precedent in synthesizer design. The mathematics is not decorative -- it IS the synthesis mechanism. Phi's Fibonacci convergents produce naturally musical quasi-harmonic spectra. Pi's tightly clustered convergents produce metallic unison beating. E's wide-stepped convergents produce organ-like intervals. Sqrt2's Pell number convergents produce tritone-adjacent tensions. Four distinct timbral families from four irrational constants, all derived from the same algorithm.
