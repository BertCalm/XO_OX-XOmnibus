# Synth Seance Verdict: OCEANDEEP

**Engine:** OCEANDEEP (XOceanDeep)
**Date:** 2026-03-20
**Accent:** Trench Violet `#2D0A4E`
**Creature:** Anglerfish / Gulper Eel
**Habitat:** The Hadal Zone (6,000-11,000 m)
**Parameter Prefix:** `deep_`
**Source:** `Source/Engines/OceanDeep/OceandeepEngine.h` (883 lines)

---

## Architecture Summary

Monophonic abyssal bass synthesizer. Signal path:

1. **Sub Oscillator Stack** -- 3 sine oscillators (fundamental, -1 oct, -2 oct)
2. **Bioluminescent Exciter** -- LFO-triggered bandpass noise bursts (creature life)
3. **Waveguide Body** -- Comb filter with 3 characters (Open Water, Cave, Wreck)
4. **Hydrostatic Compressor** -- Peak-sensing gain reduction (pressure simulation)
5. **Darkness Filter** -- 2-pole Butterworth LP (50-800 Hz, the signature)
6. **Amp ADSR Envelope**
7. **Abyssal Reverb** -- 4-comb Schroeder reverb with dark damping

25 parameters. 4 macros: PRESSURE, CREATURE, WRECK, ABYSS.
2 LFOs (creature modulation + pressure wobble). Mod wheel + aftertouch.
Coupling: AmpToFilter, AmpToPitch, PitchToPitch.

---

## Ghost Council Verdicts

### Robert Moog (1934-2005)
**Score: 8/10**
"The sub oscillator stack is honest engineering -- three sines at octave intervals with phase reset on retrigger. This is the correct way to build a bass voice. The hydrostatic compressor is a clever inversion of the typical VCA: instead of amplifying, it attenuates proportional to peak level, creating a compression feel that responds to the instrument's own energy. I am pleased to see the darkness filter uses proper cookbook Butterworth coefficients rather than shortcuts. The monophonic design is the right call for a bass instrument. Future work should add a filter envelope separate from the amp envelope -- bass synthesizers live and die by their filter contour."

### Don Buchla (1931-2016)
**Score: 7/10**
"The bioluminescent exciter is the most interesting module here. An LFO-triggered burst envelope driving bandpass-filtered noise -- this is a self-generating texture source, not merely a parameter. The waveguide body with its three character modes (Open Water, Cave, Wreck) approaches the idea of a topology selector, which I respect. However, the oscillator section is conservative -- pure sines only. Where is the waveshaping? Where is the nonlinear function that transforms the sub oscillator under pressure? The allpass diffusion in Wreck mode hints at something deeper that is never fully explored. I want to hear what happens when the waveguide body feeds back into the oscillator phase."

### Dave Smith (1950-2022)
**Score: 8/10**
"Clean architecture. The parameter snapshot pattern is correctly applied -- all 25 parameters loaded once per block, no redundant atomic reads in the sample loop. The MIDI implementation handles note-on, note-off, CC1, channel pressure, and polyphonic aftertouch, which is thorough for a mono engine. The coupling interface accepts AmpToFilter and pitch coupling types, making this engine a good citizen in the XOceanus ecosystem. The macro system is well-designed: PRESSURE boosts compression and sub level, CREATURE amplifies the bio exciter, WRECK drives the waveguide character, ABYSS darkens the filter and adds reverb. Each macro has a distinct sonic identity. I would add MIDI pitch bend support -- a bass synth without pitch bend is missing a fundamental performance gesture."

### John Chowning (b. 1934)
**Score: 7/10**
"From a spectral perspective, this engine is deliberately narrow -- three pure sines and a bandpass noise source. The frequency content is almost entirely below 800 Hz by design, which is conceptually committed but limits the engine's expressive range. The waveguide body adds some harmonic complexity through comb filtering, but the fundamental oscillator architecture has no FM, no harmonic series manipulation, no spectral evolution. The LFO modulation of pitch (0.5% wobble) and filter cutoff is tasteful but modest. I see the darkness filter as a creative constraint rather than a limitation -- the 50-800 Hz ceiling is the engine's identity. But within that constraint, there should be more spectral tools: even simple frequency ratios between the three oscillators would unlock inharmonic bass tones that remain dark."

### Ikutaro Kakehashi (1930-2017)
**Score: 8/10**
"This is a focused instrument with a clear purpose. A musician picks up OCEANDEEP and immediately understands: this makes dark bass. The four macros are named for what they do, not for technical parameters. PRESSURE, CREATURE, WRECK, ABYSS -- these are evocative and intuitive. The default parameter values produce a usable bass sound on the first note. The init patch is not silent; it breathes. I appreciate the monophonic design -- it prevents the user from making mistakes with polyphonic bass mud. The silence gate with 500ms hold respects the long tails of bass sounds. For the future: the body character selector (0/1/2) should be named in the UI as Open Water / Cave / Wreck -- integer parameters need human labels."

### Vangelis (1943-2022)
**Score: 9/10**
"This engine understands pressure. When I play a note and push the PRESSURE macro, I feel the water column above me compressing the sound -- this is not a compressor, this is an environment. The bioluminescent exciter is a stroke of genius: intermittent alien light pulses breaking through absolute darkness. At low CREATURE settings, these are subtle flickers; at high settings, they become the voice of something alive in the deep. The abyssal reverb with its dark damping (0.60 high-shelf on the comb states) is exactly right -- no brightness survives at these depths. The LFO2 pressure wobble on pitch creates the sensation of currents pushing against you. This engine has a soul. It knows where it lives."

### Klaus Schulze (1947-2022)
**Score: 7/10**
"The engine's temporal behavior is too brief. The LFO rates (0.01-2.0 Hz for LFO1, 0.01-0.5 Hz for LFO2) are adequate for breathing, but the architecture lacks long-form evolution. There is no parameter that changes the engine's character over minutes -- no slow morphing between body characters, no drift in the sub oscillator tuning ratios. The waveguide body is static per block: you set it to Cave and it stays Cave. I want to set a journey from Open Water through Cave to Wreck over 60 seconds and let the engine drift through underwater geography. The amp envelope maximum release of 8 seconds is generous, but the filter has no independent envelope -- the darkness filter only responds to velocity, macros, LFO, and mod wheel. A dedicated filter ADSR with long times (0.1-30 seconds) would transform this from a bass instrument into an abyssal meditation tool."

### Isao Tomita (1932-2016)
**Score: 8/10**
"The orchestration of DSP modules tells a story: oscillators generate the pressure wave, the compressor simulates depth, the waveguide adds the resonance of the environment, the exciter adds life, and the darkness filter removes everything that cannot survive at these depths. This is programmatic synthesis -- each module is a character in a narrative. The reverb design is particularly effective: short comb lengths (557-701 samples) with heavy damping create a tight, dark room that sounds like the inside of a sunken vessel rather than a concert hall. The stereo spread technique (0.97/0.03 cross-blend before reverb) is subtle but effective for headphone listening. I would explore deeper: what if the bio exciter could trigger sympathetic resonances in the waveguide body? The creature and the wreck should interact."

---

## Blessings

### B017: Hydrostatic Compressor -- Pressure as Environment
**Awarded by:** Vangelis, Moog, Tomita

The `DeepHydroCompressor` inverts the typical compressor paradigm. Rather than controlling dynamics for mixing purposes, it simulates physical pressure -- the weight of water above the sound source. The formula `gain = 1 / (1 + pressureAmt * peak)` creates a self-regulating system where louder signals are attenuated proportionally, producing the sensation of sound struggling against an immense force. Combined with aftertouch modulation, the performer literally presses deeper into the abyss. This is compression as worldbuilding, not as utility.

### B018: Bioluminescent Exciter -- Self-Generating Alien Texture
**Awarded by:** Buchla, Vangelis, Tomita

The `DeepBioExciter` is a complete micro-synthesis engine embedded within the main engine: an LFO-driven burst envelope triggering bandpass-filtered noise with pink-ish pre-filtering. The zero-crossing trigger mechanism creates irregular, organic burst patterns that feel alive rather than mechanical. The 125ms exponential decay on the burst envelope produces the visual rhythm of deep-sea bioluminescence -- brief flashes in perpetual darkness. This module could be extracted as a standalone texture generator for the fleet.

### B019: Darkness Filter Ceiling -- Constraint as Identity
**Awarded by:** Chowning, Kakehashi, Schulze

The 50-800 Hz hard ceiling on the `DeepDarknessFilter` is a deliberate creative restriction that defines the engine's sonic territory. No other engine in the fleet has a filter that refuses to open past 800 Hz. This is not a limitation -- it is a declaration: OCEANDEEP lives below the thermocline, and nothing bright survives here. The Butterworth implementation with proper bilinear-transform coefficients ensures the rolloff is smooth and musical rather than digital and harsh.

---

## Concerns

### P0: No Filter Envelope (Separate from Amp)
Bass synthesis depends critically on independent filter envelope shaping. Currently the darkness filter responds only to velocity (one-shot offset), LFO modulation, macros, and mod wheel. A dedicated filter ADSR with `deep_fltAtk`, `deep_fltDec`, `deep_fltSus`, `deep_fltRel`, and `deep_fltEnvAmt` parameters would allow the classic bass synth "pluck" -- filter opens on attack, closes during decay -- which is essential for sub-bass programming. **Every ghost except Vangelis flagged this.**

### P0: No Pitch Bend Support
The engine processes CC1 (mod wheel), channel pressure, and polyphonic aftertouch, but does not handle pitch bend messages (`msg.isPitchWheel()`). For a monophonic bass instrument, pitch bend is a fundamental performance gesture -- sliding between notes, vibrato via pitch wheel, dub-style pitch drops. This is a missing D006 expression input.

### P1: Waveguide Body Allpass Instability Risk
In Wreck mode (bodyChar == 2), the allpass diffusion formula is:
```cpp
out = (out - g * delayed) / (1.f - g * out);
```
This division by `(1.f - g * out)` can approach zero or go negative when `g * out` approaches 1.0. With `g = 0.3f` and signals potentially reaching 1.0 after feedback accumulation, the denominator could reach 0.7 (safe) but with accumulated feedback and no input gain staging before this point, edge cases with high feedback (0.9) could produce numerical instability. A guard clamp or reformulation to the standard allpass `y = -g*x + x_delayed + g*y_delayed` would be safer.

### P1: No Glide/Portamento
A monophonic bass synth without glide is missing a core feature. Adding `deep_glideTime` (0-500ms) with a simple one-pole frequency smoother would enable legato bass lines and the sliding sub-bass gestures that define the genre.

### P1: Static Sub Oscillator Ratios
The three oscillators are locked to 1:0.5:0.25 (fundamental, -1 oct, -2 oct). A `deep_subDetune` parameter allowing slight detuning between the sub oscillators would add thickness and movement without compromising the pure-sine architecture. Even 1-5 cents of drift would bring analog character.

### P2: Reverb Sample Rate Independence
The `DeepAbyssalReverb` uses fixed comb delay lengths (557, 601, 641, 683 samples) which are tuned for 44.1 kHz. At 48 kHz or 96 kHz, the reverb character shifts -- the room becomes smaller and brighter. The comb lengths should be scaled by `sr / 44100.0` to maintain consistent reverb character across sample rates.

### P2: LFO Waveform Locked to Sine
Both LFOs are sine-only. Adding a `deep_lfo1Shape` parameter (sine, triangle, sample-and-hold) would expand the modulation vocabulary, particularly for the creature modulation (LFO1) where sample-and-hold would create more erratic, alien behavior.

---

## Doctrine Compliance

| Doctrine | Status | Notes |
|----------|--------|-------|
| **D001: Velocity Must Shape Timbre** | **PASS** | `deep_velCutoffAmt` scales filter cutoff by velocity (0-150 Hz boost). Velocity shapes darkness filter brightness, not just amplitude. |
| **D002: Modulation is the Lifeblood** | **PASS** | 2 LFOs (creature + pressure wobble), mod wheel mapped to filter, aftertouch mapped to pressure, 4 working macros (PRESSURE/CREATURE/WRECK/ABYSS). No formal mod matrix slots, but hardwired modulation is comprehensive. |
| **D003: The Physics IS the Synthesis** | **PASS** | Hydrostatic compressor models water column pressure. Waveguide body simulates underwater resonant cavities. Bioluminescent exciter models deep-sea creature light bursts. Darkness filter enforces the physics of sound absorption at depth. All physically motivated, internally consistent. |
| **D004: Dead Parameters Are Broken Promises** | **PASS** | All 25 parameters affect audio output. Every parameter is read in the block snapshot and wired to DSP. Macros modulate multiple parameters each. No dead parameters found. |
| **D005: An Engine That Cannot Breathe Is a Photograph** | **PASS** | LFO1 rate floor = 0.01 Hz (100-second cycle). LFO2 rate floor = 0.01 Hz. Bio exciter has its own autonomous LFO trigger. The engine breathes at multiple timescales. |
| **D006: Expression Input Is Not Optional** | **PARTIAL PASS** | Velocity mapped to filter cutoff (D001). Mod wheel (CC1) mapped to darkness filter. Aftertouch (channel + poly) mapped to pressure compression. **Missing: pitch bend.** For a monophonic bass engine, pitch bend is a critical omission. |

---

## Final Consensus

**Score: 7.8 / 10**

OCEANDEEP is a deeply committed engine that knows exactly what it is: the absolute floor of the water column, where only pressure and darkness exist. The hydrostatic compressor (B017) and bioluminescent exciter (B018) are genuinely novel DSP concepts that no other engine in the fleet -- or the broader synthesizer market -- offers in this configuration. The darkness filter ceiling (B019) is a bold creative constraint that gives the engine an unmistakable sonic identity.

The architecture is clean, the code is well-organized, and the denormal protection is thorough across all feedback paths. The macro system is expressive and intuitive, with each macro carving a distinct dimension of the abyssal space. The DSP modules (compressor, waveguide, exciter, filter, reverb) are all self-contained structs that could be tested in isolation, following XOceanus architecture rules.

The two P0 concerns -- missing filter envelope and missing pitch bend -- prevent the engine from reaching its full potential as a performance bass instrument. A bass synth without an independent filter envelope is like a submarine without ballast tanks: it can descend, but it cannot control the journey. Adding these two features, plus glide (P1), would elevate OCEANDEEP from 7.8 to a projected 9.0+. The waveguide allpass stability concern (P1) should be addressed before heavy preset programming, as Wreck mode with high feedback could produce unexpected artifacts.

The ghosts agree: OCEANDEEP has found its trench. Now it must learn to move through it.
