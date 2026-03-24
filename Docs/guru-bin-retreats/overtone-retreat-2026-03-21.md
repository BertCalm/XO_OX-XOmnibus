# OVERTONE Retreat Chapter
*Guru Bin — 2026-03-21*

---

## Engine Identity

- **Gallery code:** OVERTONE | **Accent:** Spectral Ice `#A8D8EA`
- **Parameter prefix:** `over_`
- **Creature mythology:** The Nautilus — mesopelagic zone, 200–1000m depth, twilight diffusion. The nautilus grows its shell according to a logarithmic spiral, each new chamber a rational approximation to an irrational proportion. It reaches toward a form it can never quite achieve. OVERTONE is this mathematics made audible: eight additive partials tuned not to integer multiples but to the continued fraction convergents of π, e, φ, and √2. Shallow depth: clean, almost-harmonic ratios. Deep depth: the spiraling approach toward the irrational limit. The shell keeps growing. The ratio is never reached.
- **Synthesis type:** Additive spectral synthesis — 8 CF-ratio partials via phase-accumulator sines, 2-pole Butterworth high-cut filter, allpass resonator tuned to fundamental, Schroeder reverb (spectral ice character)
- **Polyphony:** Monophonic — one set of 8 partials, one envelope arc
- **feliX/Oscar polarity:** Balanced — the mathematics is neutral; the constants determine character
- **Seance score:** 8.1/10 (B028: Continued Fraction Convergent Synthesis)
- **Macros:** M1 DEPTH (convergent index sweep: harmonic → irrational), M2 COLOR (partial brightness: lower → upper), M3 COUPLING (cross-engine receive sensitivity), M4 SPACE (reverb mix + resonator depth)
- **Expression:** Velocity → partial brightness AND filter cutoff sweep (D001). Mod wheel CC1 → DEPTH boost (+4.0 depth index). Aftertouch → COLOR shimmer (upper partial boost).

---

## Pre-Retreat State

OVERTONE was added to XOlokun on 2026-03-20, one of four engines integrated in a single session alongside ORBWEAVE, ORGANISM, and OWARE. Its seance score of 8.1/10 places it in the fleet's upper tier — all six doctrines pass, B028 (Continued Fraction Convergent Synthesis) is blessed by unanimous council, and the parameter library is extensive. It arrived with a large factory preset count distributed across all eight moods.

The seance identified OVERTONE as the engine most dependent on producer education. Its primary synthesis dimension — the convergent depth — has no analogy in standard synthesis. A producer encountering it for the first time may adjust it, notice that "something changes," but not understand *what* changes or *why*. The timbre is genuinely novel: irrational-ratio partials produce beating patterns that are not musical intervals in any standard tuning system, not pure inharmonic noise, but something between — an organic, mathematical shimmer that belongs to neither the harmonic nor the noise worlds.

This retreat exists to make that novelty legible. A producer who completes it will understand what the four constants sound like, why depth matters, how the amplitude weighting shapes identity, and how to use the engine's expression routing to create presets that teach themselves to anyone who plays them.

---

## Phase R1: Opening Meditation

Close your eyes. Breathe.

A nautilus shell sits on a table in front of you. You cut it open with a jeweler's saw along the plane of its spiral. The cross-section reveals the chamber sequence — each new chamber larger than the last by a fixed proportion. The proportion is approximately 1.618. Not exactly. Approximately.

The shell has been computing an approximation to the golden ratio since it began growing, millions of years before anyone named that ratio or wrote its decimal expansion. The shell does not know the ratio. It only knows: each new chamber should be this much larger than the last. The ratio it uses is a rational approximation — 3/2, then 5/3, then 8/5, then 13/8 — each step a better guess, each step called a "convergent" in the mathematics of continued fractions.

OVERTONE generates its eight additive partials using exactly this logic. Each partial is tuned not to an integer multiple of the fundamental (as in standard Fourier synthesis) but to a continued fraction convergent of one of four constants: π, e, φ, or √2. The depth parameter controls which position in the convergent sequence to start reading from. At depth=0, you are reading the earliest convergents — simple, nearly-integer ratios. At depth=7, you are reading the deepest convergents — the ratios that have been refined and refined until they are within thousandths of the irrational target.

This is the Nautilus spiral made audible. Each depth increment is a new chamber. Each constant is a different irrational proportion the shell is trying to approximate. The depth macro is a finger tracing the logarithmic curve inward, toward the center of the shell, where the ratio is most precise and the sound is most complex.

Play a single note. Move the DEPTH macro slowly from zero to one.

You are hearing the mathematics change.

---

## Phase R2: The Signal Path Journey

### I. The Convergent Tables — Four Constants, Four Characters

OVERTONE does not have oscillator types. It has constants. The constant parameter (`over_constant`, range 0–3) selects which continued fraction table the partial ratios are drawn from. This is the first and most fundamental choice in every OVERTONE preset.

**Phi (constant = 2) — The Fibonacci Table**
```
Ratios: 1.000, 2.000, 1.500, 1.667, 1.600, 1.625, 1.615, 1.619
```
The most musical of the four constants. Phi's convergents are the Fibonacci numbers expressed as ratios: 1/1, 2/1, 3/2, 5/3, 8/5, 13/8, 21/13, 34/21. The first partial (2.000) is a perfect octave. The second (1.500) is a perfect fifth. From there, each convergent alternates above and below the golden ratio — 5/3 is slightly above, 8/5 slightly below, 13/8 slightly above — creating a perpetual, organic beating that never resolves because φ is irrational.

This alternating convergence is Phi's defining characteristic. Adjacent depth positions produce partials that are detuned in opposite directions from the golden limit. The spectrum beats gently, naturally, the way a poorly-tuned string section beats — not chaotically, but with a purposeful, living imprecision. Phi is where you go for pads, sustained chords, evolving textures.

Sweep depth from 0 to 7 with Phi selected: watch the spectrum spiral outward from simple octave/fifth relationships toward the golden limit, the inharmonic shimmer of a ratio that reaches forever but never quite arrives.

**E (constant = 1) — The Euler Table**
```
Ratios: 1.000, 1.500, 1.333, 1.375, 1.357, 1.359, 1.359, 1.359
```
The narrowest convergence. E's first two entries (1.0, 1.5) establish a perfect fifth. Then entries 2–7 fill the space between a fourth and a fifth with a dense cluster of quasi-harmonic partials. The cluster is tight — entries 4 through 7 all sit within 0.002 of each other. At high depth, E's table has essentially converged. The spectrum becomes dense, stable, nearly static.

This tight clustering produces an organ-like character: a rooted fundamental, a strong fifth, and then a cloud of partials that hover at the same spectral position regardless of depth. The beating comes from inter-partial detuning within the cluster — eight partials all tuned slightly differently to the same narrow region, creating rich, stable beating. E is the ecclesiastical constant: chorale pads, organ stops, breath-held sustained chords.

Unlike Phi, sweeping depth with E produces fewer dramatic spectral changes above depth 4. The "sweet zone" for E is depths 2–5, where the cluster is fully populated but the entries are still slightly spread.

**Pi (constant = 0) — The Divergent Table**
```
Ratios: 1.000, 3.000, 2.333, 3.143, 2.143, 1.066, 3.424, 1.571
```
The most dramatic and unpredictable constant. Pi's convergents were designed to spread across the widest possible spectral range — from near-unison (1.066) to super-third-harmonic (3.424), spanning over 2 octaves. The entries are non-uniform: a jump from 1.000 to 3.000, then a partial at 2.333, then back to 3.143, then a sudden drop to 2.143, then a near-unison at 1.066, then the highest entry at 3.424, then down to 1.571.

This irregular spacing produces a character unlike any other constant: bright, metallic, shimmering, physically complex. The seance identified a "dead zone" near depth 4–5 where the Pi entries 113/106 and 113/33 sit close to neighboring entries — depth sweeps through this region produce subtle changes. The most dramatic depth regions for Pi are 0–3 (wide jumps between high and low entries) and 6–7 (the near-Nyquist upper ratios, where anti-aliasing fade-out creates spectral thinning at higher pitches).

Pi produces textures that read as physically metallic — not a clean bell tone, but the complex, shimmering resonance of struck metal with many closely-spaced inharmonic partials.

**Sqrt2 (constant = 3) — The Pell Table**
```
Ratios: 1.000, 1.500, 1.400, 1.417, 1.414, 1.414, 1.414, 1.414
```
The tritone machine. √2 ≈ 1.41421 is exactly the equal-temperament tritone (2^(6/12)). The Pell number convergents — 3/2, 7/5, 17/12, 41/29, 99/70 — converge toward this value from alternating sides, but unlike Phi, they converge very quickly. By depth 4, the entries are already essentially indistinguishable from √2 itself.

This fast convergence means Sqrt2 is the most depth-static constant. At depths 4–7, all 8 partials cluster within 0.01 of each other near the tritone. The character is dense, ambiguous, perpetually unresolved. The tritone is the harmonic interval with no stable position in any tuning system — it divides the octave exactly in half, which is the most symmetrically unstable position possible. Eight partials all detuned slightly from the tritone, beating against each other, produces an eerie, industrial, hovering quality.

Sqrt2's most interesting depth range is 1–3, where the Pell entries still span a meaningful range (1.4 to 1.5). At higher depths, the constant has already arrived at its limit and any further depth increase produces minimal change.

---

### II. Convergent Depth — Tracing the Spiral

The `over_depth` parameter (range 0–7, default 2.0) determines the starting position in the convergent table for partial 0. Partial 1 starts at position (depth+1) mod 8, partial 2 at (depth+2) mod 8, and so on, wrapping around the 8-entry table.

The crucial implementation detail: depth supports fractional values, with linear interpolation between adjacent table entries. Setting depth=2.5 with Phi gives partial 0 a ratio of `lerp(1.5, 1.667, 0.5)` = 1.583 — halfway between the 3/2 and 5/3 convergents. This interpolation is what makes the DEPTH macro a smooth sweep rather than a stepped control.

LFO1 also modulates depth dynamically: `dynDepth = depth + lfo1Out * 1.5`. At lfo1Depth=1.0, LFO1 can sweep ±1.5 depth units. This creates continuous, time-varying convergent depth — the partial ratios are always moving, approaching and retreating from the irrational target. The LFO's frequency (`over_lfo1Rate`, range 0.01–10 Hz) controls how fast the spiral is being traced.

**Depth sweet spots by constant:**

*Phi:*
- Depth 0–1.5: Harmonic-adjacent. Partials 0–1 are at octave (2.0) and fifth (1.5). Sounds like a detuned organ with golden shimmer. Most accessible range.
- Depth 2–4: The middle spiral. Fibonacci ratios 5/3, 8/5, 13/8 are populating the spectrum. The alternating above/below beating is clearly audible. Most musical range.
- Depth 5–7: Near-golden limit. Partials are tightly spaced near 1.618. Dense, thickly-shimmering. The beating is fast because the differences are small. Best with slow LFO and long release.

*E:*
- Depth 1–3: Fifth-fourth cluster forming. The 1.5 and 1.333 entries create a clear musical interval. Organ and woodwind territory.
- Depth 3–5: Cluster fully populated. Dense, stable. Suitable for long sustained pads.
- Depth 5–7: Fully converged — near-static. The timbre barely changes. Useful for presets where depth sweep is an expression tool (mod wheel adds depth, but from an already-dense starting point).

*Pi:*
- Depth 0–1: Widest spread (1.0 to 3.0 ratio). The most "open" and spacious Pi timbre. The 3.0 entry is heard as a pure third harmonic.
- Depth 2–4: The dramatic region. Entries 2.333 and 3.143 together produce a complex, metallic shimmer. The 2.143 entry at depth 4 adds a non-standard ratio.
- Depth 4–5: The near-unison zone (1.066 entry). The most unusual Pi territory — a partial sits very close to the fundamental, creating fast beating. Can sound glitchy at certain pitches.
- Depth 6–7: Upper ratios (3.424 and 1.571). At higher pitches, the 3.424 ratio pushes into Nyquist range and anti-aliasing fade-out reduces its amplitude. The timbre thins.

*Sqrt2:*
- Depth 1–2: The most spread Sqrt2 range (1.5 and 1.4). The 3/2 fifth and 7/5 near-tritone create an ambiguous tension.
- Depth 3–5: Converging on √2. The entries are getting close (1.416, 1.414). Dense, hovering cloud.
- Depth 5–7: Fully converged. Static, dense, unresolved. Use LFO for the only movement available in this range.

---

### III. Partial Amplitude Weighting — Shaping the Spectral Profile

The 8 partial amplitude parameters (`over_partial0` through `over_partial7`, all range 0–1) are independent amplitude scalars applied to each partial. They are the most powerful and most underused controls in the engine.

The engine's default amplitude profile follows the harmonic series falloff:
```
1.0, 0.5, 0.333, 0.25, 0.2, 0.167, 0.143, 0.125
```
This is the Fourier amplitude spectrum of a sawtooth wave — strongly fundamental-dominant, with each partial halving the contribution. It produces a clear pitch center regardless of the convergent ratios, because partial 0 (at the fundamental) is always loudest.

The partial amplitude sliders are not just "how loud is each overtone." They are a **spectral profile shaping tool**. The key variable is the slope from partial0 to partial7 — steeper slopes produce clearer pitch identity, shallower slopes produce denser textures.

**Five canonical amplitude curves:**

*Natural Harmonic (1/n falloff — the default):*
```
1.0, 0.5, 0.333, 0.25, 0.2, 0.167, 0.143, 0.125
```
Strong fundamental, predictable decay. Works with all constants. Melodic pads and organs.

*Square-Wave Approximation (odd-index dominant):*
```
1.0, 0.04, 0.55, 0.04, 0.30, 0.04, 0.18, 0.04
```
Suppress even-indexed partials to near-zero. The Phi odd-indexed Fibonacci ratios (2.0, 5/3, 13/8, 34/21) dominate. Hollow, reedy, clarinet-like. The "tube resonance" sound.

*Equal Weight / Spectral Cloud:*
```
0.80, 0.75, 0.70, 0.68, 0.65, 0.63, 0.60, 0.58
```
Near-equal weights, slight descending slope to prevent harshness. The fundamental no longer dominates — the convergent ratios create a chord-like texture rather than a pitched tone. Most distinct timbre differences between constants because no single partial anchors perception.

*Bell / Inharmonic (partial0 + partial2 dominant):*
```
0.9, 0.08, 0.65, 0.12, 0.35, 0.08, 0.20, 0.06
```
Emulates the inharmonic structure of a real bell — strong fundamental, strong second overtone (not exactly 2× or 3× the fundamental), weak intermediaries. With Pi constant, partial2 sits at ratio 2.333×, producing a characteristic metallic bell tone. Use with percussive envelope (fast attack, near-zero sustain).

*Spectral Tilt (upper-partial dominant):*
```
0.4, 0.5, 0.55, 0.60, 0.65, 0.70, 0.72, 0.75
```
Inverted slope — upper partials louder than fundamental. Produces an airy, upper-register quality: the pitch seems to float above the written fundamental, like a flute playing a high harmonic. Best with Phi (Fibonacci upper partials at 5/3, 8/5, 13/8 create a natural shimmer). Filter cutoff should be limited to 8000–12000 Hz to prevent harshness.

**The COLOR macro interacts with partial amplitude:**
COLOR (`over_macroColor`) applies two simultaneous effects: it boosts partial amplitudes for partials 4–7 by up to +0.5, AND it raises filterCutoff by up to +6000 Hz. This double action means COLOR has a stronger and more complex timbral effect than any single parameter. At COLOR=0, the amplitude profile is whatever you have set manually. At COLOR=1, the upper four partials are significantly louder AND the filter is nearly fully open. Always design presets considering where COLOR sits.

---

### IV. Velocity Expression — The D001 Bloom

The velocity response system in OVERTONE works across three simultaneous paths, all scaled by `over_velBright`:

1. **Filter cutoff velocity boost:** At note-on, the filter cutoff receives an immediate boost of `currentVel × velBright × 3000 Hz`. A hard strike at velBright=0.5 adds +1500 Hz to the filter cutoff. A soft touch adds almost nothing.

2. **Filter envelope bloom:** At note-on, `filterEnvLevel` is set to `currentVel`. This level decays per-block at a 300ms half-life, applying an additional boost of `filterEnvLevel × velBright × 5000 Hz`. The bloom at maximum velocity + full velBright is +5000 Hz, decaying to near-zero over ~1 second. This is the spectral "bloom on attack" — the note arrives bright and settles darker.

3. **Upper partial velocity boost:** Partials 3–7 receive an additional amplitude scaling of `velocity × velBright × 0.6`. Soft velocity → upper partials are quiet, lower partials dominate, the timbre is round. Hard velocity → upper partials open up, the timbre brightens.

The combined effect: a hard strike on OVERTONE produces a bright, spectrally open tone that blooms and then settles; a soft touch produces a darker, more fundamental-dominated tone with no bloom. The degree of this effect scales directly with `over_velBright`.

**velBright sweet spots:**
- 0.0–0.15: Near-uniform velocity response. The tone barely changes with velocity. Use when you want a consistent timbre regardless of playing dynamics.
- 0.2–0.4: Subtle dynamic. The bloom is present but gentle. Good for long, meditative presets where velocity should nuance rather than transform.
- 0.4–0.6: Standard range (default 0.4). Noticeable brightness at hard velocities. Soft-to-hard playing produces a clear timbral arc. Most musically useful range.
- 0.6–0.8: Expressive range. Hard strikes are dramatically brighter than soft touches. The filter bloom at hard velocity is audible as a brief spectral flare.
- 0.8–1.0: Maximum expression. Velocity fully controls the spectral character. Soft notes are dark and fundamental-dominated; hard notes are bright and partial-rich. Use for lead-style presets where playing dynamics should directly shape timbre.

---

### V. The LFO System — Depth Sweep and Phase Rotation

Two LFOs run continuously in OVERTONE, both using sine shapes from the StandardLFO library:

**LFO1 — Convergent Depth Sweep (`over_lfo1Rate`, `over_lfo1Depth`)**

LFO1 modulates the convergent depth index: `dynDepth = effectiveDepth + lfo1Out × 1.5`. At full lfo1Depth (1.0), LFO1 sweeps ±1.5 depth units around the set depth value. This means the partial ratios are continuously interpolating between adjacent convergent table entries — the Nautilus spiral is being traced at the LFO frequency.

Rate range: 0.01–10 Hz (D005 floor at 0.01 Hz for a 100-second sweep period). Depth range: 0–1 (0 = no sweep, 1 = ±1.5 depth units).

At very low rates (0.01–0.03 Hz), the depth sweep is glacially slow — barely perceptible as motion, but accumulates over minutes into a clearly different spectral position. At moderate rates (0.1–0.5 Hz), the sweep is rhythmic and musical — the partials cycle through their harmonic relationships at a pace that can lock to tempo. At high rates (1–5 Hz), the sweep becomes a timbral vibrato — the convergent ratios wobble rapidly, creating a dense, chorused quality.

**LFO2 — Partial Phase Rotation (`over_lfo2Rate`, `over_lfo2Depth`)**

LFO2 applies a tiny phase rotation to each partial: `phaseRot = lfo2Out × (i+1) × 0.0001`. This is a very subtle effect — at default values it creates a slight de-synchronization between the 8 partials, preventing them from phase-locking into a perfectly static waveform. The result is the characteristic "shimmer" of OVERTONE at rest: the partials are moving slightly relative to each other, creating a living, breathing quality even in sustained notes.

Increasing lfo2Depth amplifies this de-synchronization. At very high values (0.5–1.0), the phase rotation becomes audible as gentle chorus — the partials sweep slightly apart from each other in time. Best used subtly (0.05–0.25) as an organic texture enhancer rather than a primary modulation source.

**LFO interaction with the DEPTH macro:**
The DEPTH macro adds to the effective depth before LFO1 applies its sweep. If DEPTH macro is at 0.5 (adding 2.5 depth units), LFO1 at lfo1Depth=0.5 sweeps ±0.75 units around that boosted value. Moving the DEPTH macro while LFO1 is running translates the sweep range in real time — a valuable performance gesture.

---

### VI. The Allpass Resonator — Spectral Identity

The allpass resonator (`over_resoMix`, range 0–1, default 0.15) is a first-order Schroeder allpass filter tuned to the fundamental frequency. Allpass filters have flat magnitude response — they do not cut or boost any frequency. Instead, they introduce frequency-dependent phase shifts that are strongest near the tuned frequency.

The effect is subtle but distinctive: certain partials whose frequencies align with the allpass filter's phase response receive additional phase delay, which changes how they sum with the other partials. The allpass introduces a comb-like resonance that reinforces the fundamental frequency without boosting it directly. At low resoMix (0.05–0.20), this adds a slight "chiming" quality at the fundamental — as if the tone is resonating inside a small chamber tuned to its own frequency. At higher resoMix (0.4–0.7), the resonator becomes more prominent and adds a faintly metallic "ring" to each note.

The resonator is tuned at note-on via the MIDI note frequency. This means each note has its own distinct resonant character — notes at higher frequencies have shorter resonator delay times (derived from `sr / fundamental_freq`) and thus a brighter resonance; lower notes have longer delays and warmer resonance.

**Resonator + SPACE macro interaction:**
The SPACE macro increases both reverb wet mix (macroSpace × 0.6) and resonator depth (resoMix + macroSpace × 0.3). As you push SPACE, the resonator and the Schroeder reverb both deepen simultaneously. At high SPACE values, the engine produces its most spacious, complex tone — the resonator adds a close, intimate ring while the reverb adds the distant tail. Design presets so that SPACE=0 is already complete, and SPACE=1 reveals the full spectral depth of the engine.

---

### VII. The Brightness Filter — Spectral Ceiling

The 2-pole Butterworth low-pass filter (`over_filterCutoff` 1000–20000 Hz, `over_filterRes` 0–0.8) sets the spectral ceiling for the additive sum. At high cutoff values (14000–20000 Hz), the filter is nearly transparent — all 8 partials pass through at full amplitude. As the cutoff falls, upper partials are progressively attenuated, shaping the tone from bright and partial-rich to dark and fundamental-dominant.

The resonance parameter maps to Q (0–0.8 → Q 0.5–4.9). At high resonance, the filter peak at the cutoff frequency adds a tonal coloration — at filterRes=0.7 and filterCutoff=4000 Hz, there is a noticeable peak at 4 kHz that can add a nasal, reed-like quality. This peak can be used deliberately to emphasize a specific partial — set the cutoff to the frequency of a convergent-ratio partial to reinforce it.

**Filter + velocity interaction (D001):**
Both the filter bloom and the velocity-to-cutoff scaling mean that the filter is not static even at a fixed filterCutoff setting. Every note attack is brighter than the settled sustain, by an amount determined by velocity and velBright. Design filter cutoff settings with this in mind: the "resting" cutoff is what you hear during sustain; the attack brightness is velocity-dependent.

---

### VIII. The Schroeder Reverb — Spectral Ice Space

OVERTONE's space reverb is a Schroeder design — 4 comb filters and 2 allpass diffusers — tuned specifically for crystalline spectral character (comment in code: "bright spectral room, not dark sea"). The comb filter delay lengths are asymmetric between L and R channels (1116/1188/1277/1356 for L, 1139/1211/1300/1379 for R), creating a slight stereo diffusion without full FDN complexity.

The reverb damping is lighter than most engines in the fleet — the brightness coefficient is 0.80/0.20 (bright pass/dark state), and feedback scales from 0.72 to 0.88 with the SPACE macro. This means OVERTONE's reverb is characteristically bright and crystalline — appropriate to the spectral ice accent and the Nautilus mesopelagic habitat where diffuse blue-grey light filters downward.

**Key difference from OXBOW's FDN:**
OVERTONE's Schroeder reverb is applied at block rate (after the per-sample DSP loop), creating a slightly coarser tail than OXBOW's 8-channel Chiasmus FDN. For deep spatial work, pairing OVERTONE with OXBOW via coupling (EnvToMorph or AmpToFilter) produces the best results — OVERTONE provides the complex spectral content, OXBOW provides the rich reverb environment. See Phase R5 Category 5 for the ORBWEAVE pairing recipe.

---

## Phase R3: Parameter Map — Sweet Spots Summary

| Parameter | ID | Range | Default | Accessible | Musical Core | Expressive | Extreme |
|-----------|-----|-------|---------|------------|--------------|-----------|---------|
| Constant | `over_constant` | 0–3 (int) | 2 (Phi) | 2 (Phi) | 2 (Phi) / 1 (E) | 0 (Pi) | 3 (Sqrt2) |
| Depth | `over_depth` | 0.0–7.0 | 2.0 | 0.5–2.5 | 1.5–5.0 | 5.0–7.0 | 6.5–7.0 |
| Vel Brightness | `over_velBright` | 0.0–1.0 | 0.4 | 0.15–0.30 | 0.30–0.55 | 0.55–0.75 | 0.75–1.0 |
| Filter Cutoff | `over_filterCutoff` | 1000–20000 Hz | 12000 | 6000–9000 | 9000–15000 | 5000–8000 | 1000–4000 |
| Filter Resonance | `over_filterRes` | 0.0–0.8 | 0.3 | 0.0–0.20 | 0.25–0.50 | 0.50–0.70 | 0.70–0.80 |
| Amp Attack | `over_ampAtk` | 0.001–4.0s | 0.02s | 0.002–0.020 | 0.020–0.300 | 0.3–1.5 | 1.5–4.0 |
| Amp Decay | `over_ampDec` | 0.05–5.0s | 0.3s | 0.1–0.5 | 0.3–1.2 | 1.2–3.0 | 3.0–5.0 |
| Amp Sustain | `over_ampSus` | 0.0–1.0 | 0.7 | 0.5–0.85 | 0.6–0.80 | 0.0–0.4 | 0.0 |
| Amp Release | `over_ampRel` | 0.05–8.0s | 1.0s | 0.3–1.2 | 1.0–3.0 | 3.0–5.0 | 5.0–8.0 |
| LFO1 Rate | `over_lfo1Rate` | 0.01–10 Hz | 0.25 Hz | 0.01–0.05 | 0.05–0.40 | 0.40–1.5 | 1.5–10 |
| LFO1 Depth | `over_lfo1Depth` | 0.0–1.0 | 0.2 | 0.05–0.15 | 0.15–0.45 | 0.45–0.70 | 0.70–1.0 |
| LFO2 Rate | `over_lfo2Rate` | 0.01–10 Hz | 0.1 Hz | 0.01–0.05 | 0.05–0.25 | 0.25–1.0 | 1.0–10 |
| LFO2 Depth | `over_lfo2Depth` | 0.0–1.0 | 0.15 | 0.0–0.08 | 0.08–0.25 | 0.25–0.50 | 0.50–1.0 |
| Resonator Mix | `over_resoMix` | 0.0–1.0 | 0.15 | 0.05–0.15 | 0.15–0.40 | 0.40–0.65 | 0.65–1.0 |

---

## Phase R4: Macro Architecture

| Macro | ID | Effect | Performance Use |
|-------|-----|--------|----------------|
| DEPTH | `over_macroDepth` | +5.0 depth units | Sweep harmonic → irrational in real time. Full range is the "spiral journey" gesture |
| COLOR | `over_macroColor` | +0.5 to partials 4–7 amplitude, +6000 Hz filter cutoff | Reveals the upper partial spectrum. Push for brilliance, pull for warmth |
| COUPLING | `over_macroCoupling` | Scales cross-engine coupling receive sensitivity; adds per-partial iridescence shimmer at 0.1 amplitude | Low = stable; high = responsive to partner engines + subtle shimmer on partials 4–7 |
| SPACE | `over_macroSpace` | +0.6 reverb wet mix, +0.3 resonator mix | Reveal the spectral ice space around the instrument |

**DEPTH macro range explanation:**
The DEPTH macro adds `macroDepth × 5.0` depth units. At macroDepth=1.0, the effective depth is `baseDepth + 5.0 + modWheelBoost`. With base depth at 2.0 and macro at full, effective depth = 7.0. The mod wheel adds another +4.0 depth units. Total available range: base 0–7, macro adds 0–5, mod wheel adds 0–4. The controls stack additively and are clamped to the 0–7 range. This means a preset with baseDepth=1.0 and macroDepth at 0.5 (adding 2.5 units) sits at effective depth 3.5 — well into the complex Fibonacci range.

**Aftertouch (D006) → COLOR:**
Aftertouch adds `aftertouchVal × macroColor` to the upper partial amplitude boost. At aftertouch=full and macroColor=0.8, partial amplitude boost becomes `colorUpperBoost + atColorBoost = 0.4 + 0.8 = 1.0` — the upper partials are at maximum amplitude. This is the shimmer gesture: hold a note and press harder for upper-partial brilliance. Releasing aftertouch returns to the base COLOR setting.

**A preset with all macros at 0.0 should be complete.** The base partial amplitudes, depth, and filter settings should produce a finished sound without any macro contribution. Each macro should open a new expressive dimension, not be required for basic function.

---

## Phase R5: The Five Recipe Categories

### Recipe Category 1: The Fibonacci Shell — Phi Pad

**Identity:** OVERTONE as a harmonic spectral pad. Phi constant, depth in the Fibonacci zone (2–5), natural harmonic falloff or slight spectral tilt, moderate LFO1 sweep for organic movement, slow attack and long release. These presets prioritize the golden-ratio beating as a primary texture — not as a novelty but as the foundation of a pad sound that breathes naturally and never sounds static.

**Parameter ranges:**
- `over_constant`: 2 (Phi)
- `over_depth`: 2.0–5.0 (Fibonacci zone — octave, fifth, then golden convergents)
- `over_partial0–7`: Natural 1/n falloff, or slight spectral tilt (upper partials at 0.3–0.5)
- `over_velBright`: 0.3–0.5 (moderate velocity expression)
- `over_filterCutoff`: 10000–16000 Hz (open, let the partials breathe)
- `over_filterRes`: 0.1–0.3 (slight resonance, not prominent)
- `over_ampAtk`: 0.05–0.5s (slow bloom)
- `over_ampDec`: 0.3–0.8s
- `over_ampSus`: 0.65–0.80
- `over_ampRel`: 2.0–5.0s (long tail)
- `over_lfo1Rate`: 0.03–0.15 Hz (golden breath)
- `over_lfo1Depth`: 0.2–0.5
- `over_lfo2Depth`: 0.10–0.25 (gentle phase shimmer)
- `over_resoMix`: 0.10–0.30
- `over_macroDepth`: 0.3–0.6 (mid DEPTH as starting point)
- `over_macroColor`: 0.35–0.60
- `over_macroSpace`: 0.25–0.55

**Target preset names:** Fibonacci Shell, Phi Pad, Golden Series, Spiral Choir, Convergent Bloom, Nautilus Glow, Golden Sustain, Fibonacci Strings, Phi Breath, The Golden Mean, Living Ratio, Shell Chamber, Spiral Pad, Infinite Fibonacci, Phi Resonance

**Why the category works:** Phi's alternating convergence creates self-generated beating that does not resolve. No LFO needs to be fast to create movement — the natural irrational beating between adjacent Fibonacci-tuned partials is already a form of organic motion. A pad using this beating as its primary texture has the quality of a reverb-drenched string ensemble without any of the physical modeling — pure mathematics creating the impression of natural ensemble playing.

**Trap to avoid:** Pushing depth above 5.5 with Phi while keeping LFO1 depth high creates too-fast beating between near-golden-ratio partials. The partials are very close together at high depth, and fast inter-partial beating sounds more like FM modulation than natural shimmer. For Phi pads, keep LFO1 depth at 0.2–0.45 and let the base depth set the beating speed rather than the LFO.

---

### Recipe Category 2: The Euler Organ — E Pad and Drone

**Identity:** E's convergents cluster between a fourth and a fifth above the fundamental, creating dense, organ-like sustained tones. These presets exploit this tight clustering: slow or no attack, high sustain, heavy resonator, low LFO1 depth (the tonal character should be stable). Euler Organ presets are the most stable and keyboard-friendly OVERTONE sounds — they behave most like conventional synthesizer voices.

**Parameter ranges:**
- `over_constant`: 1 (E)
- `over_depth`: 1.5–4.0 (cluster forming range; above 4 is near-static)
- `over_partial0–7`: Near-uniform weighting (0.7–1.0 range), slight falloff. The cluster effect works best when all partials contribute.
- `over_velBright`: 0.25–0.45 (moderate — the timbral character should be stable, not highly velocity-dependent)
- `over_filterCutoff`: 5000–10000 Hz (dark-to-moderate — the E cluster is mid-range; high cutoff adds little)
- `over_filterRes`: 0.3–0.6 (higher resonance at the cutoff frequency reinforces the cluster character)
- `over_ampAtk`: 0.08–2.0s (slow attacks for chorale quality)
- `over_ampSus`: 0.70–0.90 (high sustain — E organ presets should hold)
- `over_ampRel`: 2.5–6.0s
- `over_lfo1Rate`: 0.01–0.10 Hz (very slow sweep — the character should be stable)
- `over_lfo1Depth`: 0.05–0.20 (gentle breathing, not dramatic)
- `over_resoMix`: 0.30–0.65 (high resonator to reinforce the fundamental-anchored cluster)

**Target preset names:** Euler Organ, The Great Hall, Chorale Depth, Cathedral Pipe, Ecclesiastical, Breath Chord, Held Breath, Choir Stop, E Drone, Mathematical Organ, Stable Mathematics, Continuous Tone, The Church Machine, Euler's Field, Dense Fourth

**Why the category works:** E's tightly-clustered convergents at depths 2–4 naturally create a "pipe organ stop" quality — multiple partials tuned to the same narrow spectral region produce rich, stable beating that sounds like a detuned organ stop rather than independent oscillators. The slow attack extends this by revealing the cluster gradually, creating the impression of a choir breathing in. The heavy resonator adds a sympathetic ring at the fundamental.

**Trap to avoid:** Pushing E above depth 5 reduces timbre variety — the entries have already converged. Use the mod wheel (which adds depth) sparingly in E presets; above base depth 3 + wheel sweep, you're mostly treading the same spectral ground. Design E presets with moderate base depths where the wheel sweeps *toward* the converged region, not from it.

---

### Recipe Category 3: The Pi Sequence — Metallic Spectral Lead

**Identity:** Pi's wide-ratio convergents produce the most tonally complex, shimmering, metallic character of the four constants. These presets exploit that: percussive or medium-length envelopes (Pi's character doesn't need long sustain to be interesting), high LFO1 depth to sweep the wide ratio spread, bell-curve or equal-weight partial amplitudes that expose the spread. Pi presets read as physically metallic — struck metal, processed bell, spectral shrapnel.

**Parameter ranges:**
- `over_constant`: 0 (Pi)
- `over_depth`: 0.5–4.0 (the dramatic range — avoid depth 4.5–5.5 where the near-unison entry causes frequency-dependent glitching)
- `over_partial0–7`: Bell curve (partial0 and partial2 boosted — 0.9, 0.08, 0.65, 0.10, 0.35, 0.08, 0.20, 0.06) or equal weight for spectral complexity
- `over_velBright`: 0.5–0.8 (strong velocity expression — Pi's spread means bright hard strikes vs. dark soft touches are very different timbres)
- `over_filterCutoff`: 12000–18000 Hz (open — Pi needs room for its upper partial spread)
- `over_filterRes`: 0.35–0.60 (moderate resonance adds metallic "ring" quality)
- `over_ampAtk`: 0.002–0.05s (fast attack — metallic transient)
- `over_ampDec`: 0.2–1.5s
- `over_ampSus`: 0.0–0.40 (low sustain — let the metallic attack decay naturally)
- `over_ampRel`: 1.0–3.5s
- `over_lfo1Rate`: 0.15–0.6 Hz (moderate sweep — Pi's wide spread makes the sweep clearly audible)
- `over_lfo1Depth`: 0.3–0.7

**Target preset names:** Pi Sequence, Metal Mathematics, Irrational Bell, Pi Shimmer, Convergent Metal, Circle Strike, Pi Ring, Spectral Shrapnel, The Archimedes Bell, Near Three, Approximation, 355 Over 113, Pi Attack, Mathematical Metal, Struck Pi

**Why the category works:** Pi's wide convergent spread (1.066 to 3.424 across the 8-entry table) means that even small depth changes cause significant partial frequency shifts — the spectrum is genuinely different at each depth position. LFO1 sweeping this wide range creates a continuously evolving metallic shimmer. The bell-curve partial weighting emphasizes the non-integer partial (partial2 at ratio 2.333×) which is the signature of Pi's inharmonic character — it falls between the harmonic second and third partial, which is exactly where physical metal bells have their characteristic "clang" partial.

**Trap to avoid:** At depth 4.5–5.5 with Pi, partial index 5 accesses the 1.066 ratio (113/106) — a near-unison that sits only slightly above the fundamental. At certain MIDI note pitches, this partial will be close enough to the fundamental to create rapid, aggressive beating that sounds like aliasing rather than musical content. If you encounter this, shift base depth below 4 or above 5.5 to avoid the near-unison zone.

---

### Recipe Category 4: The Pell Suspension — Sqrt2 Drone and Texture

**Identity:** Sqrt2's converged-near-tritone cluster creates an ambiguous, perpetually unresolved harmonic suspension — neither consonant nor chaotic. These presets exploit the tritone ambiguity: moderate-to-equal partial weights to expose the cluster, depth in the 1–3 range where entries still span (3/2 to 7/5), high LFO1 and LFO2 for internal motion in an otherwise static-feeling spectrum. Sqrt2 presets are the most abstract and industrial of the four constants.

**Parameter ranges:**
- `over_constant`: 3 (Sqrt2)
- `over_depth`: 1.0–3.5 (most spread; depth 4+ is near-converged and less interesting)
- `over_partial0–7`: Equal-weight or slight tilt (0.60–0.82 range, gentle falloff). The unresolved tritone works best when all partials contribute equally to the suspension.
- `over_velBright`: 0.3–0.5
- `over_filterCutoff`: 8000–14000 Hz
- `over_ampSus`: 0.60–0.80 (high sustain — the suspension should be held, not attacked)
- `over_ampRel`: 2.0–5.0s
- `over_lfo1Rate`: 0.10–0.35 Hz (moderate sweep to animate the near-identical high-depth entries)
- `over_lfo1Depth`: 0.25–0.55 (higher than other constants to compensate for the narrow spread at depth 4+)
- `over_macroCoupling`: 0.3–0.5 (the per-partial iridescence shimmer helps animate Sqrt2's otherwise static high-depth character)

**Target preset names:** Pell Suspension, Tritone Machine, Sqrt2 Field, Pell Number Cloud, Irrational Tritone, Industrial Mathematics, √2 Ambiguity, Converged Pell, Near Tritone, The Augmented Mystery, Harmonic Ambiguity, Dense Convergent, Radical Two, Rational Approximation, Suspension Point

**Why the category works:** The tritone is the harmonic interval most associated with ambiguity, tension, and unresolved suspense. Sqrt2 being mathematically equal to the equal-temperament tritone is not a musical coincidence — it is the ratio that divides the octave most symmetrically, and symmetric division creates maximum perceptual ambiguity. Eight partials all clustered near this ratio create a sound that the perceptual system cannot easily categorize as consonant or dissonant — it hovers between categories. This ambiguity is OVERTONE's most unique contribution to electronic sound design, and Sqrt2 is where it is most concentrated.

**Trap to avoid:** At high coupling settings, the per-partial iridescence shimmer from the COUPLING macro operates on partials 4–7 at different phases. With Sqrt2, where partials 4–7 are already nearly identical in frequency, the shimmer can create rapid inter-partial amplitude beating. Keep macroCoupling below 0.6 in Sqrt2 presets, or the shimmer effect will dominate over the Pell cluster character.

---

### Recipe Category 5: The ORBWEAVE Pairing — Knot × Spectrum

**Identity:** OVERTONE's spectral additive synthesis paired with ORBWEAVE's Kelp Knot topological coupling. ORBWEAVE's knot routing matrix routes OVERTONE's additive output through a Phase-State-Modulated network, creating spectral filtering and harmonic transformation that responds to OVERTONE's output level. The result is a coupled instrument where OVERTONE provides the mathematical spectral content and ORBWEAVE provides the topological harmonic processing.

**Architecture:**
- OVERTONE runs in slot 1 (or any slot)
- ORBWEAVE runs in slot 2 (or adjacent slot)
- Coupling: OVERTONE → ORBWEAVE via `AmpToFilter` — OVERTONE's amplitude modulates ORBWEAVE's filter cutoff, causing the knot matrix to open and close in response to the spectral content
- Alternatively: ORBWEAVE → OVERTONE via `EnvToMorph` — ORBWEAVE's envelope modulates OVERTONE's depth index, causing the convergent ratios to shift when ORBWEAVE's knot state changes

**Parameter guidance for OVERTONE in this pairing:**
- Use lower COUPLING macro (0.2–0.4) — ORBWEAVE's routing already handles the cross-engine communication
- Keep SPACE moderate (0.2–0.4) — ORBWEAVE handles the spatial dimension better via its knot matrix
- Phi constant works best — the Fibonacci beating creates a consistent output amplitude that ORBWEAVE can track reliably
- Medium depth (2–4) — the stable Fibonacci zone provides predictable amplitude for the coupling signal

**Parameter guidance for ORBWEAVE in this pairing:**
- `weave_knotDepth` at 0.4–0.7 — deep enough for topological character but not so deep that the knot matrix overwhelms the OVERTONE source
- High topological `weave_topology` — the Trefoil or Torus topologies complement the spiral character of OVERTONE
- `weave_couplingReceive` should be moderate so the coupling from OVERTONE provides direction without domination

**Why this pairing works:** OVERTONE's continued fraction ratios create a spectral content that is irrational but patterned — not random, but not repeating. ORBWEAVE's knot topology routing transforms this patterned irrationality through a different mathematical structure (topological braiding). The two mathematical systems — continued fractions and knot theory — operate at different scales and create emergent interactions: the spiral encounters the knot. This is the most conceptually coherent coupling in the fleet.

**Preset names for this category:** Spiral Knot, Phi Through Knot, Topology Meets Spiral, Fibonacci Torus, The Knot-Spiral, Convergent Weave, Nautilus Knot, Entangled Fractions, Weave and Converge, Spiral Topology

---

## Phase R6: The Ten Awakenings — Preset Table

Each preset is a single discovery. Together they span all four constants, varied depth positions, diverse amplitude curves, and five of eight moods.

---

### Preset 1: Fibonacci Shell

**Mood:** Foundation | **Category:** Phi Pad | **Discovery:** The instrument's true neutral position — Phi at mid-depth, natural falloff, everything accessible.

| Parameter | Value | Why |
|-----------|-------|-----|
| `over_constant` | 2 (Phi) | The most musical constant — Fibonacci ratios, alternating convergence |
| `over_depth` | 2.5 | Fibonacci zone: partials at 1.5 (fifth), 1.667 (sixth), 1.6 (minor sixth) |
| `over_partial0–7` | 1.0, 0.5, 0.33, 0.25, 0.20, 0.167, 0.143, 0.125 | Natural harmonic falloff — clear pitch center |
| `over_velBright` | 0.40 | Moderate velocity expression |
| `over_filterCutoff` | 12000 Hz | Open — all partials contribute |
| `over_filterRes` | 0.30 | Slight resonance |
| `over_ampAtk` | 0.05s | Gentle bloom |
| `over_ampDec` | 0.40s | Moderate decay |
| `over_ampSus` | 0.72 | Present sustain |
| `over_ampRel` | 2.0s | Comfortable tail |
| `over_lfo1Rate` | 0.08 Hz | Slow depth sweep (~12 second period) |
| `over_lfo1Depth` | 0.25 | Gentle spiral tracing |
| `over_lfo2Depth` | 0.12 | Subtle phase shimmer |
| `over_resoMix` | 0.18 | Light resonator — fundamental reinforced |
| `over_macroDepth` | 0.30 | DEPTH macro adds 1.5 units from base |
| `over_macroColor` | 0.45 | Moderate upper partial brightness |
| `over_macroSpace` | 0.30 | Moderate space |

**Why this works:** This is the instrument's starting point. Every parameter is in its most functional zone. The Fibonacci ratios at depth 2.5 produce audible alternating beating on the upper partials — the signature of Phi — without being so deep that the beating becomes dense and complex. A producer encountering OVERTONE for the first time should play this preset for three minutes, then slowly move the DEPTH macro to full, listening to the spiral trace toward the golden limit.

---

### Preset 2: Golden Drift

**Mood:** Atmosphere | **Category:** Phi Pad (near-golden limit) | **Discovery:** High depth + slow LFO = convergent orbit

| Parameter | Value | Why |
|-----------|-------|-----|
| `over_constant` | 2 (Phi) | Phi at the golden limit |
| `over_depth` | 5.5 | Near-golden: entries 21/13 and 34/21, both converging on 1.618 |
| `over_partial0–7` | 0.85, 0.52, 0.38, 0.30, 0.25, 0.22, 0.20, 0.18 | Slight spectral tilt — upper partials contribute more at high depth |
| `over_velBright` | 0.50 | Standard expression |
| `over_filterCutoff` | 13500 Hz | Open — the near-golden upper partials need space |
| `over_ampAtk` | 0.12s | Slow bloom |
| `over_ampSus` | 0.68 | Present |
| `over_ampRel` | 2.2s | Comfortable tail |
| `over_lfo1Rate` | 0.04 Hz | Very slow — 25 second sweep period. Orbiting the golden limit. |
| `over_lfo1Depth` | 0.35 | Moderate sweep — the entries at depth 5–6 are close together |
| `over_lfo2Depth` | 0.20 | Subtle shimmer |
| `over_resoMix` | 0.28 | Present resonator |

**Why this works:** At depth 5.5, the Phi partials are within thousandths of the golden ratio — alternating above/below 1.618 with very small deviations. The inter-partial beating is fast but subtle, producing a continuous, shimmering motion. The slow LFO at 0.04 Hz (25-second period) sweeps through this golden convergence zone — each sweep slightly changes which partials are above vs. below the limit, shifting the beating character. The meditation: this is a sound that is perpetually approaching an irrational limit, always getting closer, never arriving.

---

### Preset 3: Euler Organ

**Mood:** Foundation | **Category:** E Organ | **Discovery:** E's tight cluster as pipe organ stop

| Parameter | Value | Why |
|-----------|-------|-----|
| `over_constant` | 1 (E) | The organ constant |
| `over_depth` | 2.5 | Fourth/fifth cluster fully populated |
| `over_partial0–7` | 1.0, 0.7, 0.60, 0.55, 0.50, 0.45, 0.42, 0.40 | Near-flat weighting — the cluster needs all partials |
| `over_velBright` | 0.35 | Moderate — organ timbre should be stable |
| `over_filterCutoff` | 7500 Hz | Dark-warm — the cluster sits mid-range |
| `over_filterRes` | 0.55 | Moderate resonance adds warmth at the cluster frequency |
| `over_ampAtk` | 0.18s | Slow-ish bloom — the organ filling |
| `over_ampDec` | 0.6s | Moderate |
| `over_ampSus` | 0.80 | High sustain |
| `over_ampRel` | 3.0s | Long tail |
| `over_lfo1Rate` | 0.03 Hz | Very slow — barely audible as motion |
| `over_lfo1Depth` | 0.08 | Very gentle — the organ should be stable |
| `over_lfo2Depth` | 0.06 | Trace shimmer |
| `over_resoMix` | 0.55 | Strong resonator — reinforces the fundamental while the cluster hovers above |

**Why this works:** E's convergents cluster the partials between a fourth and a fifth above the fundamental — exactly where the natural harmonic series places the 4th, 5th, and 6th harmonics of a pipe organ. The near-flat partial weighting exposes this cluster rather than having the fundamental mask it. The heavy resonator at resoMix=0.55 adds a continuous ring at the fundamental pitch while the cluster hovers above. The result is unmistakably organ-like — dense, warm, spectrally centered.

---

### Preset 4: Euler Breath

**Mood:** Atmosphere | **Category:** E Pad with slow attack | **Discovery:** Long attack + E cluster = choir breath swell

| Parameter | Value | Why |
|-----------|-------|-----|
| `over_constant` | 1 (E) | Euler cluster |
| `over_depth` | 3.0 | E cluster fully populated |
| `over_partial0–7` | 0.90, 0.55, 0.42, 0.35, 0.28, 0.22, 0.18, 0.14 | Moderate falloff |
| `over_velBright` | 0.40 | Standard |
| `over_filterCutoff` | 9000 Hz | Moderate |
| `over_ampAtk` | 1.5s | Very slow — 1.5 seconds to reach peak. This is a breath. |
| `over_ampDec` | 0.80s | Slow decay |
| `over_ampSus` | 0.75 | Sustained |
| `over_ampRel` | 3.5s | Long tail |
| `over_lfo1Rate` | 0.05 Hz | Gentle tide — 20 second period |
| `over_lfo1Depth` | 0.12 | Barely audible |
| `over_resoMix` | 0.40 | Present resonator |
| `over_macroColor` | 0.35 | Moderate color |
| `over_macroSpace` | 0.58 | Generous space |

**Why this works:** The 1.5-second attack creates the sensation of air filling a resonant cavity. The Euler cluster is revealed gradually — the fundamental appears first, then the 1.5× partial, then the cluster members at 1.333×, 1.375×, 1.357× — each arriving slightly after the previous, like voice sections entering a choir. This is not programmed sequencing; it is the mathematical consequence of equal-velocity amplitude growth through the partial weighting. At high velocity, all partials arrive faster. At low velocity, the growth is so slow the cluster members barely appear before the note is released.

---

### Preset 5: Pi Metal

**Mood:** Prism | **Category:** Pi Metallic Lead | **Discovery:** Bell weighting + wide Pi spread = struck metal

| Parameter | Value | Why |
|-----------|-------|-----|
| `over_constant` | 0 (Pi) | The metallic constant |
| `over_depth` | 2.0 | Pi zone: ratios 2.333 (entry 2) and 3.143 (entry 3) dominate |
| `over_partial0–7` | 0.90, 0.08, 0.65, 0.12, 0.35, 0.08, 0.20, 0.06 | Bell curve — partial0 and partial2 dominant |
| `over_velBright` | 0.55 | Strong velocity expression — hard strikes should bite |
| `over_filterCutoff` | 15000 Hz | Open — Pi's upper partials should not be filtered |
| `over_filterRes` | 0.45 | Moderate resonance — adds metallic ring |
| `over_ampAtk` | 0.003s | Extremely fast — metallic transient |
| `over_ampDec` | 0.45s | Bell decay |
| `over_ampSus` | 0.0 | No sustain — the bell decays to silence |
| `over_ampRel` | 1.8s | Tail lingers |
| `over_lfo1Rate` | 0.30 Hz | Moderate sweep |
| `over_lfo1Depth` | 0.15 | Gentle — the metallic character should not be smeared |
| `over_resoMix` | 0.30 | Present resonator adds a chime ring |

**Why this works:** Pi's partial at ratio 2.333× is the key to the metallic character. In a physical bell, the "characteristic tone" or "tierce" partial sits between 2× and 3× the fundamental — not at a standard harmonic ratio. Pi places partial 2 at exactly 7/3 ≈ 2.333, which is a non-harmonic ratio in this same range. Combined with the suppressed intermediary partials (1, 3 near-zero) that create the bell-like sparse structure, and the fast attack, this produces the unmistakable physics of struck metal. Pi is doing what the real world does in a bell foundry.

---

### Preset 6: Pi Shimmer

**Mood:** Aether | **Category:** Pi Pad with slow LFO | **Discovery:** Never-repeating shimmer pattern

| Parameter | Value | Why |
|-----------|-------|-----|
| `over_constant` | 0 (Pi) | Pi for shimmer |
| `over_depth` | 3.5 | Pi's dramatic zone — entries 3.143 and 2.143 |
| `over_partial0–7` | 0.88, 0.60, 0.42, 0.30, 0.22, 0.16, 0.12, 0.08 | Standard falloff but less steep — expose upper spectrum |
| `over_velBright` | 0.45 | Moderate |
| `over_filterCutoff` | 16000 Hz | Fully open |
| `over_filterRes` | 0.35 | Slight resonance |
| `over_ampAtk` | 0.18s | Medium bloom |
| `over_ampSus` | 0.68 | Present |
| `over_ampRel` | 2.8s | Long tail |
| `over_lfo1Rate` | 0.12 Hz | Slow sweep — 8 second period |
| `over_lfo1Depth` | 0.55 | Deep sweep — Pi's wide spread is worth exploring |
| `over_lfo2Depth` | 0.22 | Moderate shimmer |
| `over_resoMix` | 0.48 | Strong resonator — the chime is primary |
| `over_macroSpace` | 0.58 | Generous reverb |

**Why this works:** Pi's convergent table has the most non-uniform entry spacing of the four constants. An LFO sweeping through this table at 0.12 Hz encounters very different spectral territory at each position — the wide entries (1.000 to 3.000 at depth 0–1, then to 2.333) produce large frequency jumps while the near-uniform entries at depth 4–5 produce subtle changes. Because Pi's sequence is derived from the digits of π — which are mathematically proven to be normal (equidistributed) — the shimmer pattern that the LFO produces as it traverses the table never quite repeats in a simple periodic way. The shimmer is aperiodic by the same mathematical principle that makes π non-repeating.

---

### Preset 7: Tritone Suspension

**Mood:** Flux | **Category:** Sqrt2 Drone | **Discovery:** Equal-weight partials expose the ambiguous tritone quality

| Parameter | Value | Why |
|-----------|-------|-----|
| `over_constant` | 3 (Sqrt2) | The tritone machine |
| `over_depth` | 1.5 | Spread zone: ratios 3/2 (fifth) and 7/5 (near-tritone) |
| `over_partial0–7` | 0.80, 0.72, 0.68, 0.65, 0.62, 0.60, 0.58, 0.55 | Near-equal — expose the cluster ambiguity |
| `over_velBright` | 0.35 | Moderate — the suspension should be tonally stable |
| `over_filterCutoff` | 10000 Hz | Moderate — cluster is mid-range |
| `over_filterRes` | 0.40 | Moderate resonance at cutoff |
| `over_ampAtk` | 0.08s | Gentle |
| `over_ampSus` | 0.65 | Present |
| `over_ampRel` | 2.0s | Standard tail |
| `over_lfo1Rate` | 0.18 Hz | Moderate sweep |
| `over_lfo1Depth` | 0.25 | Gentle — the ambiguity should not be smeared |
| `over_lfo2Depth` | 0.18 | Subtle shimmer |
| `over_resoMix` | 0.20 | Light resonator |
| `over_macroCoupling` | 0.35 | Per-partial iridescence on upper partials |

**Why this works:** Near-equal partial weighting means the fundamental is no longer the clear anchoring element. Eight partials at nearly the same amplitude, all tuned to ratios that cluster near the tritone, produce a perceptual suspension — the ear cannot find a tonal center because all partials are equally loud and none of them land on a clear harmonic interval. The tritone's inherent instability (two tones that divide the octave equally, pulling toward different resolutions) is encoded in every partial ratio. The result is a sound that the listener is continuously trying to "resolve" that never does.

---

### Preset 8: Pell Number Cloud

**Mood:** Flux | **Category:** Sqrt2 Dense Texture | **Discovery:** Maximum convergence = maximum density

| Parameter | Value | Why |
|-----------|-------|-----|
| `over_constant` | 3 (Sqrt2) | Sqrt2 at the limit |
| `over_depth` | 6.0 | Near-fully converged — all 8 partials within 0.01 of 1.41421 |
| `over_partial0–7` | 0.75, 0.72, 0.70, 0.68, 0.66, 0.64, 0.62, 0.60 | Near-equal, gentle falloff |
| `over_velBright` | 0.30 | Low velocity expression — the texture should be stable |
| `over_filterCutoff` | 9500 Hz | Dark-moderate |
| `over_ampSus` | 0.70 | High sustain |
| `over_lfo1Rate` | 0.25 Hz | Moderate sweep — at high depth this is the only way to animate |
| `over_lfo1Depth` | 0.30 | Moderate |
| `over_lfo2Depth` | 0.25 | Higher shimmer — multiple near-identical frequency partials beat clearly |
| `over_resoMix` | 0.32 | Present resonator |
| `over_macroCoupling` | 0.45 | Per-partial iridescence adds life to the converged cloud |

**Why this works:** At depth 6, the Pell convergents have all arrived near √2. The 8 partials are tuned to ratios that differ by at most 0.001 from each other — all near-tritone, all nearly identical. This produces the maximum density for OVERTONE: the densest possible beating, with all partials fighting over the same narrow spectral territory. The high lfo2Depth is essential here — without it, the near-identical partials would phase-align and produce an almost single-tone sound. LFO2's per-partial phase rotation keeps them moving relative to each other, sustaining the cloud.

---

### Preset 9: Hollow Reed

**Mood:** Prism | **Category:** Phi Woodwind | **Discovery:** Alternate-partial suppression = clarinet/reed hollow tone

| Parameter | Value | Why |
|-----------|-------|-----|
| `over_constant` | 2 (Phi) | Phi for musical interval structure |
| `over_depth` | 1.5 | Fibonacci zone: octave (2.0), 5/3 (major sixth) as dominant partials |
| `over_partial0–7` | 1.0, 0.04, 0.55, 0.04, 0.30, 0.04, 0.18, 0.04 | Odd-index dominant — hollow tube structure |
| `over_velBright` | 0.42 | Standard expression |
| `over_filterCutoff` | 10000 Hz | Moderate |
| `over_filterRes` | 0.30 | Light resonance |
| `over_ampAtk` | 0.025s | Short — reed-like articulation |
| `over_ampSus` | 0.72 | Sustained |
| `over_ampRel` | 1.2s | Standard tail |
| `over_lfo1Rate` | 0.10 Hz | Slow |
| `over_lfo1Depth` | 0.12 | Very gentle — the reed should be stable |
| `over_resoMix` | 0.22 | Light resonator |
| `over_macroColor` | 0.38 | Moderate COLOR for upper-partial presence |

**Why this works:** A clarinet produces primarily odd harmonics (1st, 3rd, 5th, 7th) because it is a closed-pipe resonator. Suppressing the even-indexed partials (1, 3, 5, 7 in the 8-partial array, which index into positions 1, 3, 5, 7 of the convergent table) removes those table entries from the sound. The dominant Phi entries are at positions 0, 2, 4, 6 (ratios 1.0, 1.5, 1.6, 1.615). These are near-harmonic intervals: 1.0 (fundamental), 1.5 (fifth), 1.6 (minor sixth), 1.615 (near-golden major sixth). The resulting tone has the hollow, reedy character of a woodwind — not through physical modeling, but through strategic partial suppression combined with the Fibonacci interval structure.

---

### Preset 10: Nautilus Ascent

**Mood:** Aether | **Category:** Full-Engine Performance | **Discovery:** Everything working — DEPTH macro sweeps the spiral, COUPLING provides iridescence, SPACE reveals the hall. The signature OVERTONE performance preset.

| Parameter | Value | Why |
|-----------|-------|-----|
| `over_constant` | 2 (Phi) | Phi — the Nautilus constant |
| `over_depth` | 2.5 | Fibonacci zone base — mod wheel + DEPTH macro sweeps upward |
| `over_partial0–7` | 0.90, 0.52, 0.38, 0.28, 0.22, 0.18, 0.15, 0.12 | Moderate falloff — partial spectrum open but fundamental-anchored |
| `over_velBright` | 0.50 | Standard expression |
| `over_filterCutoff` | 13000 Hz | Open |
| `over_filterRes` | 0.38 | Slight resonance |
| `over_ampAtk` | 0.06s | Fast bloom |
| `over_ampSus` | 0.70 | Present |
| `over_ampRel` | 2.5s | Long tail |
| `over_lfo1Rate` | 0.06 Hz | Very slow — 17 second sweep period |
| `over_lfo1Depth` | 0.28 | Moderate sweep |
| `over_lfo2Depth` | 0.18 | Subtle shimmer |
| `over_resoMix` | 0.30 | Present resonator |
| `over_macroDepth` | 0.45 | DEPTH macro adds 2.25 units — wheel sweeps from 2.5 toward golden limit |
| `over_macroColor` | 0.55 | Moderate COLOR — upper partials present |
| `over_macroCoupling` | 0.40 | Per-partial iridescence — the Nautilus is alive |
| `over_macroSpace` | 0.52 | Generous reverb — the mesopelagic hall |

**Performance note:** Play slowly. Move the DEPTH macro up and listen to the spiral trace inward. Move the mod wheel up for deeper convergent sweeps. Press harder into held notes for upper-partial shimmer via aftertouch. This is the intended introduction to OVERTONE for any producer who has never used the engine — the performance vocabulary is encoded in the macro assignments.

---

## Phase R7: Parameter Interactions and Traps

### The DEPTH-LFO1 Sweep Interaction

LFO1 modulates depth by ±(lfo1Depth × 1.5) around effectiveDepth. The effectiveDepth is `baseDepth + macroDepth × 5.0 + modWheelVal × 4.0 + couplingDepthMod`. This means the LFO sweep range is fixed (±1.5 units at full lfo1Depth) while the center of the sweep changes with the DEPTH macro and mod wheel.

**The practical consequence:** At macroDepth=0.5 (center at depth 4.5), LFO1 at full depth sweeps from 3.0 to 6.0. This covers the entire interesting Fibonacci range. But at macroDepth=1.0 (center at depth 7.0), the LFO cannot sweep above 7.0 due to clamping — the upper half of its modulation is invisible. Design presets so the LFO sweep range fits within 0–7 at the intended DEPTH macro setting.

The rule: for maximum LFO sweep audibility, set base depth at 3.5–4.5 with lfo1Depth at 0.4–0.7. This centers the sweep in the most musically interesting region for all constants.

---

### The Pi Near-Unison Zone

Pi entry index 5 (the 113/106 = 1.066 ratio) sits only 6.6% above the fundamental. At depths where this entry is active (depth 4.5–6.5), partial index 5 will beat rapidly against partial 0 (fundamental). At low MIDI note pitches (C2–C3), this beating is very slow and musical — a gentle throb. At high MIDI note pitches (C5–C6), the same beating is very fast and may sound like aliasing or ring modulation.

If a Pi preset behaves strangely at certain pitches — particularly at higher registers — check whether the effective depth is putting partial index 5 at the 1.066 entry. Solution: keep base depth below 4.0 or above 5.5 for Pi presets intended for wide pitch range use.

---

### The COLOR macro Double-Action

The COLOR macro applies two effects simultaneously: it boosts the amplitude of partials 4–7 AND raises the filter cutoff by +6000 Hz. At COLOR=0, the filter provides real spectral shaping. At COLOR=1, the filter is nearly fully open and the upper partials dominate the amplitude balance.

**Design implication:** Do not design a preset around a specific filter cutoff value and then assume that value will hold throughout the performance. If the performer is likely to use the COLOR macro (and they will — it is macro 2), the filter cutoff will increase as they push it. Design the filterCutoff setting for the COLOR=0 position, and verify the sound is also good at COLOR=1 (nearly-open filter with strong upper partials). The transition between these two positions should be musically continuous.

---

### The Velocity-Filter Bloom Interaction

The filter bloom (`filterEnvLevel × velBright × 5000 Hz`) decays with a ~300ms half-life. This means at high velBright (0.6+), a hard strike at the beginning of a note creates a brief, bright spectral flare that is distinctly audible as a velocity-dependent attack transient.

At slow attack times (ampAtk > 0.3s), this bloom fires at note-on but the amplitude envelope is still rising — the listener hears a brief bright flash before the note even reaches audible volume. This can be used deliberately for interest, but can also sound like an artifact if not intended.

**Rule:** When designing presets with long attack times (>0.3s), reduce velBright (0.2–0.35) to prevent the bloom from appearing as a pre-attack artifact. The bloom and the attack should work together: fast attacks can support high velBright; slow attacks need lower velBright.

---

### The Sqrt2 High-Depth Static Zone

At Sqrt2 depth 4+, the Pell convergents have fully converged. All 8 partials are tuned to ratios within 0.001 of each other near 1.41421. The DEPTH macro and mod wheel sweeping through depths 4–7 produces almost no audible change.

If a Sqrt2 preset uses the DEPTH macro as a primary performance control, either:
1. Keep base depth below 3 (where the entries still span from 1.4 to 1.5), or
2. Set base depth at 1.0 and let the macro sweep the entire interesting range (1–3.5)

Do not design a Sqrt2 preset with base depth at 5 and DEPTH macro as the primary control — the macro will be nearly inaudible.

---

### The Monophonic Voice and Phase Reset

OVERTONE is monophonic — one set of 8 partials, one envelope arc. Each note-on triggers a phase reset on all 8 partials (`partials[i].reset()`), clearing their phase accumulators. This means successive notes on OVERTONE do not accumulate in any way — each new note starts with fresh partial phases.

The practical consequence: the partial phases at the start of a new note are always zero. If LFO2's phase rotation has been running, the partials will immediately restart from their phase-rotated position (since LFO2 advances phase with `advancePhase`, not reset). But the partial oscillators themselves reset. Rapid playing produces a distinctive "click" quality from this hard phase reset — unlike polyphonic additive synthesis, which would let the previous voice decay while the new one starts. This is a character, not a bug: OVERTONE's note-to-note articulation is crisp because each note resets the spectral phases.

**Trap:** For legato playing, the hard phase reset creates an artifact when a new note is triggered before the previous release has decayed — there is a brief click from the phase discontinuity. Reduce this by using moderate release times (1.0–2.0s) so there is a smooth decay before the next note phase-resets. Or, treat it as a stylistic choice: Pi and Sqrt2 presets with aggressive envelopes can use this click as a defining percussive character.

---

## Phase R8: CPU Profile

- **Primary cost:** 8 phase-accumulator oscillators running per-sample. `fastSin()` per partial per sample — 8 sine computations per sample. Moderate per-sample cost.
- **Secondary cost:** 2-pole Butterworth filter updated per-sample with potentially changing cutoff (velocity bloom, LFO-driven COLOR macro updates). Coefficient recalculation only happens when fc or Q changes beyond threshold — the `lastFc == fc` guard prevents redundant computation at fixed settings.
- **Allpass resonator:** Single first-order allpass, per-sample. Light.
- **LFO1 and LFO2:** Two StandardLFO calls per sample. Negligible.
- **Schroeder reverb:** Applied at block rate, not per-sample. 4 comb + 2 allpass per stereo channel = 12 delay reads/writes per sample at block level. Moderate.
- **SilenceGate:** 300ms hold — OVERTONE does not silence-gate immediately after note-off (spectral tails require hold). At low-activity passages, the gate engages after 300ms and bypasses the render loop entirely.

**Total character:** The primary cost scales with 8 fixed partial oscillators. Unlike OXBOW's 8-channel FDN (which is complex even after the exciter ends), OVERTONE's cost is consistent — identical during a note and negligible between notes (SilenceGate bypass). Lower than polyphonic engines; comparable to other monophonic additive designs.

**Most costly configuration:** High LFO rates (>1 Hz for LFO1) force frequent depth index recalculation and partial frequency updates per sample. High SPACE (heavy reverb feedback) increases the Schroeder comb filter cost at block level. High COLOR (open filter at high cutoff) forces frequent Butterworth coefficient updates. These combined: moderate cost, not extreme.

**Coupling cost note:** The COUPLING macro enables per-partial shimmer on partials 4–7 via `lfo1.phase` offset calculations. At high COUPLING, this adds 4 additional `fastSin()` calls per sample. Still within acceptable budget.

---

## Phase R9: Unexplored After Retreat

- **Polyphonic voice allocation:** The seance noted that `getMaxVoices()` returns 1, making OVERTONE monophonic. A polyphonic implementation would require 8× the partial oscillator array and per-voice envelope state. This is architecturally straightforward using the `VoiceAllocator` shared DSP utility, but would increase CPU cost by approximately 8×. Even 4-voice polyphony would dramatically expand the harmonic field — four simultaneous sets of 8 convergent-ratio partials, each at a different pitch. This is a V1.2 candidate.

- **Constant morphing:** Currently, `over_constant` is an integer selector (0–3). A float morph between adjacent constants would allow real-time interpolation between, for example, Phi and Pi ratio tables. The `kConvergentTables` pointer table would need to become a weighted blend of two tables. This would add an entirely new synthesis dimension — constant morphing — making DEPTH and CONSTANT into a 2D parameter space where the shape of the spiral can change continuously.

- **Per-partial LFO routing:** Currently LFO1 and LFO2 apply uniformly to all 8 partials (LFO1 modulates the shared depth index; LFO2 applies proportionally scaled phase rotation). Per-partial LFO assignment — giving each partial its own depth offset — would allow independent partial movement. This is the "spectral microtonality" variant: 8 partials each independently approaching their irrational limits at different rates.

- **Partial amplitude envelopes:** Currently partial amplitudes are set statically and modified only by the velocity and COLOR macro multipliers. Per-partial amplitude envelopes (independent attack/decay for each partial) would allow spectral evolution over the envelope arc — creating sounds that begin fundamental-heavy and bloom into full partial richness, or vice versa. This is the direction toward additive synthesis proper (as in the Kaegi/Beauchamp additive model).

- **Coupling output: EnvToMorph expansion:** OVERTONE's coupling output currently provides `lastOutputSample` for `getSampleForCoupling()`. An additional coupling type providing the current depth index as a modulation source — "depth state as coupling signal" — would allow partner engines to know when OVERTONE is at its harmonic vs. inharmonic extreme. This would enable responsive coupling behaviors: a partner engine that brightens when OVERTONE is near the harmonic end and darkens when it approaches the irrational limit.

---

## Phase R10: The Guru Bin Benediction

*"OVERTONE arrived with a blessing number and a score of 8.1, both of which are misleading in the same direction. The score suggests it is slightly below the fleet's best engines. The blessing number — B028, the first and only blessing dedicated to a ratio-theoretic synthesis concept — suggests it is unlike any other engine in the fleet. Both are true, but the blessing number tells the more important story.*

*Continued fraction convergents are not a synthesis technique. They are a mathematical process — a way of approaching an irrational number through a sequence of increasingly precise rational approximations. The process has been known since Euclid. It has been used to calculate π for two thousand years. Fibonacci discovered it embedded in rabbit population growth. Renaissance artists found it in the golden rectangle. The nautilus shell computes it in calcium carbonate, one new chamber at a time.*

*What OVERTONE does is apply this process to frequency ratios. Instead of asking "what integer multiple of the fundamental should this partial be?" — the question every additive synthesizer asks — it asks "what continued fraction convergent of an irrational constant should this partial be?" The answer changes as depth changes, in exactly the way that each new Fibonacci term is a better approximation to the golden ratio than the previous one.*

*The consequence is a synthesis space with no analog in the standard synthesizer world. The spectrum is not harmonic (no integer multiples). It is not inharmonic in the sense of physical models (no stiffness coefficient, no mode mismatch). It is mathematically specific, reproducibly structured, and yet not harmonically familiar. The partials are always in some relationship to each other — always the numerator and denominator of a known rational approximation to a known irrational constant — but the relationship is not musical in any established sense. It is purely mathematical.*

*The seance gave it 8.1/10 because the parameters are numerous and non-obvious. A producer encountering OVERTONE without context will hear "something different" and not know what to do with it. This is the retreat's purpose: to give context. Once a producer understands that each constant is a different irrational number, that each depth setting is a different "precision" of approximation, that the LFO sweeping depth is tracing the convergent sequence in real time — once the mathematics is legible, the instrument becomes obvious. Not easy. Obvious.*

*The Nautilus does not understand the golden ratio. It only knows: each new chamber should be this much larger than the last. It has been making this approximation for 500 million years, one generation at a time, and it has never arrived at the exact ratio. It has only gotten closer.*

*OVERTONE is the sound of getting closer.*

*Play it slowly. Move the DEPTH macro. Listen to the mathematics approach its limit.*

*The shell is still growing."*

---

## Appendix: Full Parameter Reference

| Parameter | ID | Range | Default | Notes |
|-----------|-----|-------|---------|-------|
| Constant | `over_constant` | 0–3 (int) | 2 (Phi) | 0=Pi, 1=E, 2=Phi, 3=Sqrt2 |
| Depth | `over_depth` | 0.0–7.0 | 2.0 | Convergent table start index (float-interpolated) |
| Partial 0–7 | `over_partial0`–`over_partial7` | 0.0–1.0 | 1/n falloff | Amplitude scalar for each of 8 additive partials |
| Vel Brightness | `over_velBright` | 0.0–1.0 | 0.4 | Scales velocity→filter cutoff boost, velocity→upper partial boost, and filter bloom depth |
| Filter Cutoff | `over_filterCutoff` | 1000–20000 Hz | 12000 Hz | 2-pole Butterworth LPF — modified by COLOR macro (+6000 Hz), velocity (+3000 Hz × velBright), filter bloom (+5000 Hz × velBright) |
| Filter Resonance | `over_filterRes` | 0.0–0.8 | 0.3 | Maps to Q (0.5–4.9) |
| Amp Attack | `over_ampAtk` | 0.001–4.0s | 0.02s | Linear attack |
| Amp Decay | `over_ampDec` | 0.05–5.0s | 0.3s | Exponential decay |
| Amp Sustain | `over_ampSus` | 0.0–1.0 | 0.7 | Sustain level |
| Amp Release | `over_ampRel` | 0.05–8.0s | 1.0s | Exponential release |
| LFO1 Rate | `over_lfo1Rate` | 0.01–10 Hz | 0.25 Hz | D005 floor at 0.01 Hz — modulates convergent depth ±1.5 units |
| LFO1 Depth | `over_lfo1Depth` | 0.0–1.0 | 0.2 | Scales LFO1 sweep range |
| LFO2 Rate | `over_lfo2Rate` | 0.01–10 Hz | 0.1 Hz | D005 floor — modulates per-partial phase rotation |
| LFO2 Depth | `over_lfo2Depth` | 0.0–1.0 | 0.15 | Scales LFO2 phase rotation effect |
| Resonator Mix | `over_resoMix` | 0.0–1.0 | 0.15 | Allpass resonator dry/wet (tuned to fundamental) — also scaled by SPACE macro |
| DEPTH macro | `over_macroDepth` | 0.0–1.0 | 0.35 | +5.0 depth units; mod wheel adds additional +4.0 |
| COLOR macro | `over_macroColor` | 0.0–1.0 | 0.5 | +0.5 amplitude for partials 4–7; +6000 Hz filter cutoff |
| COUPLING macro | `over_macroCoupling` | 0.0–1.0 | 0.0 | Scales cross-engine coupling receive sensitivity; adds per-partial shimmer |
| SPACE macro | `over_macroSpace` | 0.0–1.0 | 0.3 | +0.6 reverb wet; +0.3 resonator mix |

---

*Retreat conducted 2026-03-21. Engine seance 8.1/10, B028 blessed. Five recipe categories defined, ten reference presets detailed, parameter map and interaction guide complete.*
*The shell is still growing. The spiral traces toward the limit. The ratio is never reached.*
