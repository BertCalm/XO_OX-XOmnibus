# Guru Bin Retreat — OVERTONE
**Retreat Date**: 2026-03-21
**Engine**: OVERTONE | Continued Fraction Spectral Synthesis | The Nautilus
**Accent**: Spectral Ice `#A8D8EA`
**Tier**: Awakening (10 presets, gold)
**Seance Score**: 8.1/10 (Blessed: B017 Continued Fraction Convergent Synthesis)

---

## Before We Begin

The Nautilus does not grow in integers. Each new chamber is a rational approximation to an
irrational proportion — the best fraction the shell can manage at that scale, reaching toward
a ratio it can never quite achieve. OVERTONE is this process made audible: eight additive
partials tuned not to 2×, 3×, 4× the fundamental, but to the continued fraction convergents
of pi, e, phi, and sqrt(2). At DEPTH=0 the partials cluster near clean integer multiples.
As DEPTH climbs, the ratios spiral outward toward their irrational limits. The spectrum
never repeats. The shell keeps growing.

This retreat assumes you have read the seance verdict and understand the engine's architecture.
It will not explain the parameters — it will explain what the parameters mean.

---

## Part I: The Four Families

### Question 1: Which ratio tables produce the most distinct timbres?

The four constants are not interchangeable. They produce fundamentally different spectral
architectures, and understanding their characters is the first lesson of this retreat.

**Phi (constant = 2, Fibonacci convergents)**
```
1.000, 2.000, 1.500, 1.667, 1.600, 1.625, 1.615, 1.619
```
The most musical of the four. Fibonacci ratios sit in the musical "sweet zone" between
pure harmonics and full inharmonicity. Partial 1 at 2.0× is a perfect octave. Partial 2
at 1.5× is a perfect fifth. From there: 5/3 (major sixth), 8/5 (minor sixth), 13/8, 21/13,
34/21 — each convergent step narrowing the gap to the golden ratio (1.61803...) from
alternating sides. The spectrum beats gently. Chords of Fibonacci-tuned partials create
slow, natural-sounding shimmer. Phi is where you go for pads, organs, living sustained tones.

The key insight: Phi convergents alternate *above and below* the golden ratio. This means
adjacent partials at different depth settings will always be slightly detuned in opposite
directions — one sharp, one flat — creating a perpetual, organic beating that never resolves.

**Sqrt2 (constant = 3, Pell number convergents)**
```
1.000, 1.500, 1.400, 1.417, 1.414, 1.414, 1.414, 1.414
```
The tritone machine. Sqrt(2) = 1.41421... is exactly the equal-temperament tritone
(2^(6/12) = 1.41421...). The Pell number convergents 3/2, 7/5, 17/12, 41/29, 99/70...
converge toward this value from alternating sides, exactly as Fibonacci does for Phi.
But where Phi's range spans 1.0 to 2.0 (a full octave of spread), Sqrt2's entries
cluster tightly between 1.4 and 1.5 — a very narrow spectral band.

This tight clustering produces a distinctive character: depth changes cause only subtle
ratio shifts (the entries from index 2–7 differ by less than 0.02). The timbre is
dense, clustered, and ambiguous — not quite a chord, not quite a unison. The tritone
flavor (sqrt(2) ≈ augmented fourth) gives a perpetual sense of harmonic suspension.

Sqrt2 is where you go for metallic ambiguity, unresolved tension, industrial textures.
At HIGH DEPTH the spectrum is essentially identical to LOW DEPTH — the constant has
already converged. This is the engine's most static constant at high depth settings.

**E (constant = 1, Euler's number convergents)**
```
1.000, 1.500, 1.333, 1.375, 1.357, 1.359, 1.359, 1.359
```
The narrowest convergence of all four constants. E's convergents cluster between 1.333
and 1.500 — a minor third above and a fifth above the fundamental, with the higher
convergents all within 0.016 of each other. This is structurally similar to Sqrt2 but
even more tightly packed.

The surprising character: E's first two entries (1.0, 1.5) set up a perfect fifth.
Then entries 2–7 fill the space between the fourth and the fifth with a dense cluster
of quasi-harmonic partials. The result is organ-like: a rooted fundamental, a prominent
fifth, and then a cloud of partials that are neither clearly harmonic nor clearly
inharmonic. At depth sweep, the partials don't move much — they hover. The organ
analogy is apt: E produces stable, held, ecclesiastical timbres.

E is where you go for organ stops, chorale pads, breath-held sustained chords.

**Pi (constant = 0, Pi convergents)**
```
1.000, 3.000, 2.333, 3.143, 2.143, 1.066, 3.424, 1.571
```
The most dramatic and unruly table. Pi's convergents were designed to spread across the
widest possible spectral range — from near-unison (1.066) to super-third-harmonic (3.424).
This means Pi has the greatest spread and the greatest internal contrast.

The engineering note: Pi convergent entries 4 and 5 (the 113/106 = 1.066 entry, which sits
a mere semitone above the fundamental) are narrow-spaced from entry 3 (2.143). The seance
flagged a "dead zone" in the Pi table where adjacent depth indices produce nearly identical
timbres. This is not a bug but a mathematical fact: Pi's convergents 333/106 and 355/113
are famous for being extremely close to each other (both ≈ π) while their predecessors and
successors are widely spaced.

Pi is where you go for shimmering upper register content, aggressive spectral clusters,
and textures that feel both mathematical and physically metallic. High DEPTH with Pi sounds
like struck metal — not a clean bell, but something complex and overtone-rich.

---

## Part II: Density vs. Clarity

### Question 2: How does the number of convergent partials affect density vs. clarity?

The engine runs 8 partials simultaneously. All 8 are always sounding. What changes with DEPTH
is *which* ratios those 8 partials use — the index into the convergent table advances as DEPTH
increases. The density vs. clarity question is therefore a question about partial *amplitude
weighting*, not about how many partials are active.

The engine's default amplitude curve is harmonic series falloff: 1/1, 1/2, 1/3, 1/4, 1/5, 1/6,
1/7, 1/8. This is the "natural" weighting — it produces the same envelope as a sawtooth wave's
harmonic series. Partial 0 (the fundamental) dominates; each successive partial halves in
contribution.

Three amplitude regimes are worth understanding:

**Pyramid (default 1/n falloff)**
Fundamental-heavy. Clear pitch center. The lower partials anchor the spectrum; upper partials
add shimmer without dominating. Good for melodic contexts — the pitch reads clearly even as
the convergent ratios detune the upper spectrum.

Adjust: Keep partial0 at 1.0. Set partial1–7 as `1/(n+1)`. The standard falloff.

**Equal weight (all partials 0.7–0.8)**
No hierarchy. Every partial contributes equally. The convergent ratios now matter more
because there is no dominant fundamental. The result is "spectral cloud" — each constant
produces a distinctive chord-like texture with no clear root. This is the most abstract
setting. With Phi at equal weights, you hear a cluster of golden-ratio-spaced tones, no
tonal center. With Pi, you hear a dense, shimmering cluster that sounds almost unpitched.

Equal weight produces the *most* distinct timbre differences between the four constants
because the fundamental no longer "anchors" the perception.

**Inverse or upper-biased (partial7 > partial0)**
Unusual but revealing. Heavy upper partials with light fundamental produces a whistle-like,
flute-harmonic quality — the pitch seems to float above the fundamental. The COLOR macro
(which boosts partials 4-7) moves the balance in this direction. Setting COLOR high while
keeping partial0–3 low achieves an upper-partial-dominated spectrum that shimmers and
drifts. Best with Phi (the Fibonacci spacing keeps the upper partials from clashing).

The practical lesson: **clarity comes from partial0 dominance; density comes from flattening
the slope.** The 8 partial amplitude sliders are a slope-shaping tool. The key variable is
not the absolute values but the ratio between partial0 and partial7.

---

## Part III: Amplitude Weighting Curves

### Question 3: What amplitude weighting curves create the most musical harmonic series?

Five canonical curves that produce distinct and musically useful timbres:

**Curve A: Natural Harmonic (1/n)**
`1.0, 0.5, 0.33, 0.25, 0.2, 0.17, 0.14, 0.125`
The "correct" Fourier reconstruction of a sawtooth wave. Strong fundamental, even and
predictable decay. Most forgiving — works with all four constants. The go-to for melodic
pads and organs. The seance verdict's Phi default produces this shape.

**Curve B: Square-Wave Approximation (odd harmonics boosted)**
`1.0, 0.0, 0.33, 0.0, 0.2, 0.0, 0.14, 0.0`
Set partials 1, 3, 5, 7 to zero (or near zero). Only odd indices contribute. This
approximates a square wave with convergent-ratio detuning. The result is hollow, reedy,
clarinet-like — the "hollow tube" quality. Works best with Phi (the odd Fibonacci
convergents sit near harmonically meaningful intervals).

Practical version: rather than zero, set even partials to 0.05 so they don't completely
vanish but contribute subthreshold texture:
`1.0, 0.05, 0.33, 0.05, 0.2, 0.05, 0.14, 0.05`

**Curve C: Spectral Tilt (bright)**
`0.4, 0.5, 0.55, 0.6, 0.65, 0.7, 0.72, 0.75`
Inverted: upper partials louder than lower. The fundamental is present but recessed.
The spectrum has an airy, upper-register quality — almost like a bowed cymbal. Combine
with a low filterCutoff (6000–9000 Hz) to prevent the upper partials from becoming
harsh. Best with Phi. With Pi, produces something like struck glass harmonics.

**Curve D: Bell/Inharmonic (partial0 + partial2 dominant)**
`0.9, 0.1, 0.7, 0.15, 0.35, 0.1, 0.2, 0.1`
Boosts partials 0 and 2 while suppressing 1 and 3. This mimics the inharmonic
structure of a bell — a strong fundamental, a strong second overtone that is not
exactly 2× or 3× the fundamental, and weaker intermediary partials. Combine with
Pi constant (which places partial 2 at 2.333× — a noninteger) and a percussive
envelope (fast attack, no sustain) to produce metallic bell-like transients.

**Curve E: Dense Cloud (near-equal with slight random offset)**
`0.85, 0.78, 0.72, 0.68, 0.65, 0.63, 0.6, 0.58`
Near-equal but with a small descending slope to prevent harshness. This is the
"cloud" curve — 8 convergent-ratio partials of nearly equal weight, producing a
dense spectral texture rather than a pitched tone. Use with low filterCutoff
(8000–12000 Hz), high COLOR macro, and high SPACE for abstract textural work.
With Sqrt2, this becomes an unresolved tritone mass. With E, a dense organ cluster.

---

## Part IV: Filter and Resonator Interaction

### Question 4: How do the allpass resonator and Butterworth filter interact?

These two are in series in the signal chain:

```
additive sum -> Butterworth LPF -> allpass resonator -> soft-clip -> amp envelope -> reverb
```

But their roles are different enough that they don't compete — they complement at
different spectral positions.

**The Butterworth LPF** (`over_filterCutoff`, `over_filterRes`) shapes the macro spectrum.
It is a 2-pole lowpass with a rolloff starting at filterCutoff. At 12kHz (default), it lets
almost everything through. At 3000–6000 Hz, it creates a warm, band-limited spectral dome.
The Q parameter (mapped from `over_filterRes`: 0–0.8 -> Q 0.5–4.9) adds resonance at the
cutoff frequency — at high filterRes, the filter emphasizes the cutoff frequency itself,
adding a peak that can color the spectrum. At Q > 3.0 the resonance becomes audible as a
tonal component.

Key behavior: the COLOR macro raises filterCutoff by up to +6000 Hz. This means COLOR is
partly a partial-level boost (upper partials 4-7) AND a filter-opening. At COLOR=0 the
filter provides some spectral shaping; at COLOR=1 it is almost fully open. This double
action means COLOR has a stronger timbral effect than any single parameter.

**The allpass resonator** (`over_resoMix`) is a Schroeder first-order allpass tuned to the
fundamental frequency. It does not change the magnitude spectrum — allpass filters by
definition have flat magnitude response. Instead, it introduces phase shifts that are
strongest near the fundamental frequency. This creates a subtle comb-like resonance:
certain partials whose frequencies align with the allpass notches/peaks experience phase
reinforcement or cancellation. The effect is audible as a slight "singing" quality — the
fundamental acquires a tail, and certain partials seem to "lock in" more strongly.

The interaction: **the filter determines which frequencies reach the resonator.** At low
filterCutoff, only lower partials pass through the filter and into the resonator — which
reinforces the low-frequency resonance, creating a warm, organ-like bloom. At high
filterCutoff, all partials pass through, and the resonator adds a subtle shimmer across
the full spectrum.

**Sweet spots for the combination:**
- filterCutoff 8000–10000 Hz + resoMix 0.3–0.45: The filter removes harsh upper content
  while the resonator adds a musical resonant bloom to the lower partials. The classic
  "warm spectral pad" sound. Best with Phi.
- filterCutoff 16000–20000 Hz + resoMix 0.1–0.2: Near-flat filter, minimal resonator.
  The additive spectrum is largely untouched. Maximum transparency, revealing the
  convergent ratios as-is. Best for hearing the mathematical character of each constant.
- filterCutoff 4000–6000 Hz + resoMix 0.5–0.7: Heavy filter + heavy resonator. The
  fundamentals are boosted, most overtones removed. The allpass makes the remaining
  spectrum resonate strongly. Produces something organ-like, almost physical. Best with E.
- filterCutoff 12000 Hz + filterRes 0.6 + resoMix 0.35: The resonance peak at 12kHz
  emphasizes upper partials while the resonator creates a fundamental bloom. This
  combination produces the "spectral ice" quality the engine is named for — bright,
  shimmering, crystalline.

---

## Part V: Macro Sweet Spots

### Question 5: Sweet spots for the four macros?

**DEPTH (over_macroDepth): The Identity Knob**
Range: 0.0–1.0 adds 0–5 extra depth index units on top of `over_depth` (0–7).
Effective depth = baseDepth + macroDepth * 5.

The engine has a monophonic voice, so DEPTH is not a danger zone — there is no risk of
partials suddenly jumping to aliased frequencies. But the character change is dramatic.
Below 0.3: partial ratios are near the low end of the convergent table (more harmonic).
0.3–0.6: the "middle convergents" — this is where the golden ratio, the E cluster, and
the Pi mid-range ratios live. Most presets will sound best here.
0.7–1.0: high depth, pushing toward the most "irrational" entries. With Phi, this is
approaching the golden ratio itself. With Pi, this means the cluster at 1.066× and 3.424×.

**Guideline**: At zero, the engine sounds almost like a conventional additive synth.
At 0.5, it has its characteristic "mathematical" quality. At 1.0, it is maximally irrational.
Most presets should live in the 0.3–0.7 range. Extremes (0.0 and 1.0) are musically valid
but specific — not defaults.

The mod wheel adds another 0–4 units of depth. Combined with DEPTH macro at 0.5, the wheel
sweeps from the mid-table to the maximum index. This is the engine's primary expressive
gesture: a physical sweep from harmonic to irrational, mirroring the nautilus spiral.

**COLOR (over_macroColor): Spectral Brightness**
Range: 0.0–1.0 adds 0–+6000 Hz to filterCutoff and 0–+0.5 amplitude to partials 4-7.

COLOR is a double-action control. Half of its effect is additive (more partial amplitude
in the upper spectrum). The other half is subtractive (a wider filter window lets through
what was previously cut). The two effects reinforce: COLOR brightens via both paths.

Sweet spots:
- 0.0–0.2: Dark and low. The engine's lower partials dominate. Good for bass and
  sub-register applications.
- 0.3–0.5: Balanced. Neither upper nor lower partials dominate. The natural mid-point.
- 0.6–0.8: Bright and shimmering. The CHARACTER sweet spot for "Spectral Ice" textures.
  This range, combined with aftertouch, gives the engine its most expressive feel.
- 0.9–1.0: Very bright. Upper partials are prominent. Require the filter to already be
  set to a moderate cutoff or the result will be harsh. At COLOR=1.0 and filterCutoff=20kHz,
  upper partials can become fatiguing on sustained notes.

**Aftertouch rides on top of COLOR.** At COLOR=0.5, aftertouch adds COLOR-proportional shimmer.
At COLOR=0, aftertouch has no effect. This means COLOR should be set to at least 0.2 for
aftertouch to be expressive.

**COUPLING (over_macroCoupling): Iridescence / Shimmer**
Range: 0.0–1.0 controls two things:
1. Receive sensitivity for AmpToFilter, PitchToPitch, EnvToMorph coupling inputs (0.5 + 0.5 * macro).
2. Standalone shimmer on partials 4-7: each upper partial gets a slightly different phase
   offset on LFO1, producing a slow amplitude flutter (±0.1 per partial).

When playing solo (no coupling partner), COUPLING is a "living iridescence" knob. At 0.0,
partials behave identically — no per-partial variation. At 0.5–0.7, upper partials breathe
and flutter independently. This is the "Nautilus iridescence" effect.

Sweet spots:
- 0.0: Clean, static upper partials. Best for melodic clarity.
- 0.2–0.4: Subtle iridescence. The upper spectrum seems to "breathe" without obvious
  beating. The most musical range for sustained pads.
- 0.5–0.7: Noticeable per-partial beating. Appropriate for textural and FX presets.
- 0.8–1.0: Heavy flutter. Upper partials are conspicuously animated. Best for
  evolving pads and drone textures where variation is the point.

**SPACE (over_macroSpace): Room Presence**
Range: 0.0–1.0 scales Schroeder reverb wet mix (0–0.6) and feedback depth (0.72–0.88).
Also adds 0–0.3 to the allpass resoMix calculation.

Sweet spots:
- 0.0: Dry. The additive spectrum in the room — no tails. Reveals the engine's
  mathematical clarity most directly.
- 0.2–0.35: Light room. Enough reverb to give the engine presence without obscuring
  its spectral identity. The default range. Most melodic presets live here.
- 0.4–0.6: Full room. The spectral tails extend. Notes bloom into each other. Better
  for pads than melodies at this setting. The reverb has bright 80/20 damping so it
  does not muddy the spectrum.
- 0.7–1.0: Saturated hall. Heavy SPACE with Phi or Pi constants produces an almost
  organ-like sustain — notes die slowly, spectral content lingers. The high feedback
  (0.88 at SPACE=1.0) creates a long, bright tail. Combine with low filterCutoff to
  prevent the reverb tail from becoming harsh.

---

## Part VI: Awakening Preset Table

Ten presets covering the conceptual space of the engine. Each focuses on one distinct
character or technique. Awakening tier: pedagogical, immediately musical, clearly
differentiated.

### Awakening Preset Architecture — Design Logic

The 10 presets are distributed:
- 2 presets per constant (Pi, E, Phi, Sqrt2)
- 2 wild-card presets (boundary explorations)
- Moods distributed: Foundation (2), Atmosphere (2), Aether (2), Flux (2), Prism (2)

Each preset illustrates one named principle from the retreat questions above.

---

### Preset 1: Fibonacci Shell
**Concept**: Phi constant at depth 0 — the "most harmonic" Fibonacci ratios. Entry point.
**Teaching**: What OVERTONE sounds like before the irrational takes hold.
**Mood**: Foundation | **DNA**: brightness 0.55, warmth 0.62, movement 0.25, density 0.50, space 0.30, aggression 0.08

```json
{
  "schema_version": 1,
  "name": "Fibonacci Shell",
  "mood": "Foundation",
  "engines": ["Overtone"],
  "author": "XO_OX Designs",
  "version": "1.0.0",
  "description": "Phi constant, shallow depth. Partials tuned to Fibonacci convergents: octave, fifth, sixth. The most harmonic the engine can sound. DEPTH macro sweeps toward the golden ratio; COLOR brightens the upper Fibonacci spectrum.",
  "tier": "awakening",
  "tags": ["fibonacci", "harmonic", "foundation", "awakening", "phi"],
  "macroLabels": ["DEPTH", "COLOR", "COUPLING", "SPACE"],
  "dna": {
    "brightness": 0.55,
    "warmth": 0.62,
    "movement": 0.25,
    "density": 0.50,
    "space": 0.30,
    "aggression": 0.08
  },
  "parameters": {
    "Overtone": {
      "over_constant": 2,
      "over_depth": 0.5,
      "over_partial0": 1.0,
      "over_partial1": 0.5,
      "over_partial2": 0.333,
      "over_partial3": 0.25,
      "over_partial4": 0.2,
      "over_partial5": 0.167,
      "over_partial6": 0.143,
      "over_partial7": 0.125,
      "over_velBright": 0.45,
      "over_filterCutoff": 11000.0,
      "over_filterRes": 0.25,
      "over_ampAtk": 0.04,
      "over_ampDec": 0.4,
      "over_ampSus": 0.72,
      "over_ampRel": 1.5,
      "over_lfo1Rate": 0.08,
      "over_lfo1Depth": 0.15,
      "over_lfo2Rate": 0.05,
      "over_lfo2Depth": 0.1,
      "over_resoMix": 0.15,
      "over_macroDepth": 0.2,
      "over_macroColor": 0.45,
      "over_macroCoupling": 0.0,
      "over_macroSpace": 0.28
    }
  }
}
```

---

### Preset 2: Golden Drift
**Concept**: Phi at high depth — converging on 1.61803... Each partial approaches phi from alternating sides. The beating never resolves.
**Teaching**: What happens when DEPTH is pushed toward the golden limit. The alternating above/below convergence creates perpetual, organic shimmer.
**Mood**: Atmosphere | **DNA**: brightness 0.60, warmth 0.50, movement 0.55, density 0.58, space 0.45, aggression 0.06

```json
{
  "schema_version": 1,
  "name": "Golden Drift",
  "mood": "Atmosphere",
  "engines": ["Overtone"],
  "author": "XO_OX Designs",
  "version": "1.0.0",
  "description": "Phi constant at depth 6-7. Convergents tightening toward 1.61803 from both sides. Upper partials beat at irrational intervals — the golden ratio itself, which can never be expressed as a simple fraction. Slow LFO1 sweeps depth, orbiting the limit. Meditation on the unreachable.",
  "tier": "awakening",
  "tags": ["phi", "golden-ratio", "atmospheric", "awakening", "irrational", "drift"],
  "macroLabels": ["DEPTH", "COLOR", "COUPLING", "SPACE"],
  "dna": {
    "brightness": 0.60,
    "warmth": 0.50,
    "movement": 0.55,
    "density": 0.58,
    "space": 0.45,
    "aggression": 0.06
  },
  "parameters": {
    "Overtone": {
      "over_constant": 2,
      "over_depth": 5.5,
      "over_partial0": 0.85,
      "over_partial1": 0.52,
      "over_partial2": 0.38,
      "over_partial3": 0.30,
      "over_partial4": 0.25,
      "over_partial5": 0.22,
      "over_partial6": 0.20,
      "over_partial7": 0.18,
      "over_velBright": 0.50,
      "over_filterCutoff": 13500.0,
      "over_filterRes": 0.35,
      "over_ampAtk": 0.12,
      "over_ampDec": 0.5,
      "over_ampSus": 0.68,
      "over_ampRel": 2.2,
      "over_lfo1Rate": 0.04,
      "over_lfo1Depth": 0.35,
      "over_lfo2Rate": 0.02,
      "over_lfo2Depth": 0.20,
      "over_resoMix": 0.28,
      "over_macroDepth": 0.35,
      "over_macroColor": 0.52,
      "over_macroCoupling": 0.22,
      "over_macroSpace": 0.40
    }
  }
}
```

---

### Preset 3: Euler Organ
**Concept**: E constant — the organ-like character of tightly clustered 4th-5th convergents.
**Teaching**: E's convergents (1.333–1.5 range) create a dense organ-stop quality. Stable, breath-held, ecclesiastical.
**Mood**: Foundation | **DNA**: brightness 0.45, warmth 0.70, movement 0.15, density 0.65, space 0.55, aggression 0.05

```json
{
  "schema_version": 1,
  "name": "Euler Organ",
  "mood": "Foundation",
  "engines": ["Overtone"],
  "author": "XO_OX Designs",
  "version": "1.0.0",
  "description": "E constant. Convergents 2/1, 3/1, 8/3... normalized to cluster between a fourth and a fifth above fundamental. Dense, stable organ-stop quality. Slow attack, long sustain, heavy resonator. The mathematics of natural growth as a pipe organ.",
  "tier": "awakening",
  "tags": ["euler", "organ", "foundation", "awakening", "warm", "dense"],
  "macroLabels": ["DEPTH", "COLOR", "COUPLING", "SPACE"],
  "dna": {
    "brightness": 0.45,
    "warmth": 0.70,
    "movement": 0.15,
    "density": 0.65,
    "space": 0.55,
    "aggression": 0.05
  },
  "parameters": {
    "Overtone": {
      "over_constant": 1,
      "over_depth": 2.5,
      "over_partial0": 1.0,
      "over_partial1": 0.7,
      "over_partial2": 0.6,
      "over_partial3": 0.55,
      "over_partial4": 0.5,
      "over_partial5": 0.45,
      "over_partial6": 0.42,
      "over_partial7": 0.40,
      "over_velBright": 0.35,
      "over_filterCutoff": 7500.0,
      "over_filterRes": 0.55,
      "over_ampAtk": 0.18,
      "over_ampDec": 0.6,
      "over_ampSus": 0.80,
      "over_ampRel": 3.0,
      "over_lfo1Rate": 0.03,
      "over_lfo1Depth": 0.08,
      "over_lfo2Rate": 0.02,
      "over_lfo2Depth": 0.06,
      "over_resoMix": 0.55,
      "over_macroDepth": 0.25,
      "over_macroColor": 0.30,
      "over_macroCoupling": 0.0,
      "over_macroSpace": 0.52
    }
  }
}
```

---

### Preset 4: Euler Breath
**Concept**: E constant with natural harmonic falloff but slow attack — demonstrates the "breath-held" quality.
**Teaching**: Long attack + E constant = the sensation of air filling a resonant cavity.
**Mood**: Atmosphere | **DNA**: brightness 0.40, warmth 0.68, movement 0.30, density 0.55, space 0.62, aggression 0.04

```json
{
  "schema_version": 1,
  "name": "Euler Breath",
  "mood": "Atmosphere",
  "engines": ["Overtone"],
  "author": "XO_OX Designs",
  "version": "1.0.0",
  "description": "E constant. Very slow attack (1.5s) lets the Euler-spaced partials bloom in gradually. The 1.5x and 1.33x convergents create a fourth/fifth cluster that swells like a choir breath. LFO1 at 0.05 Hz adds imperceptible, tide-like depth motion. Slow pressure aftertouch adds shimmer.",
  "tier": "awakening",
  "tags": ["euler", "breath", "atmospheric", "awakening", "slow", "choir"],
  "macroLabels": ["DEPTH", "COLOR", "COUPLING", "SPACE"],
  "dna": {
    "brightness": 0.40,
    "warmth": 0.68,
    "movement": 0.30,
    "density": 0.55,
    "space": 0.62,
    "aggression": 0.04
  },
  "parameters": {
    "Overtone": {
      "over_constant": 1,
      "over_depth": 3.0,
      "over_partial0": 0.90,
      "over_partial1": 0.55,
      "over_partial2": 0.42,
      "over_partial3": 0.35,
      "over_partial4": 0.28,
      "over_partial5": 0.22,
      "over_partial6": 0.18,
      "over_partial7": 0.14,
      "over_velBright": 0.40,
      "over_filterCutoff": 9000.0,
      "over_filterRes": 0.30,
      "over_ampAtk": 1.5,
      "over_ampDec": 0.8,
      "over_ampSus": 0.75,
      "over_ampRel": 3.5,
      "over_lfo1Rate": 0.05,
      "over_lfo1Depth": 0.12,
      "over_lfo2Rate": 0.03,
      "over_lfo2Depth": 0.10,
      "over_resoMix": 0.40,
      "over_macroDepth": 0.30,
      "over_macroColor": 0.35,
      "over_macroCoupling": 0.10,
      "over_macroSpace": 0.58
    }
  }
}
```

---

### Preset 5: Pi Metal
**Concept**: Pi constant with bell-curve amplitude weighting (partial0 + partial2 dominant). Metallic transient.
**Teaching**: Pi's wide ratio spread + bell weighting = struck-metal attack, inharmonic bell character.
**Mood**: Prism | **DNA**: brightness 0.72, warmth 0.35, movement 0.28, density 0.45, space 0.38, aggression 0.35

```json
{
  "schema_version": 1,
  "name": "Pi Metal",
  "mood": "Prism",
  "engines": ["Overtone"],
  "author": "XO_OX Designs",
  "version": "1.0.0",
  "description": "Pi constant. Bell-curve partial weighting: fundamental and partial 2 (Pi ratio 2.333x) are prominent; adjacent partials suppressed. Produces a metallic, bell-like attack. Short decay, no sustain. Pi's wide ratio spread creates inharmonic overtones that read as physical metal resonance.",
  "tier": "awakening",
  "tags": ["pi", "metallic", "bell", "prism", "awakening", "percussive", "inharmonic"],
  "macroLabels": ["DEPTH", "COLOR", "COUPLING", "SPACE"],
  "dna": {
    "brightness": 0.72,
    "warmth": 0.35,
    "movement": 0.28,
    "density": 0.45,
    "space": 0.38,
    "aggression": 0.35
  },
  "parameters": {
    "Overtone": {
      "over_constant": 0,
      "over_depth": 2.0,
      "over_partial0": 0.90,
      "over_partial1": 0.08,
      "over_partial2": 0.65,
      "over_partial3": 0.12,
      "over_partial4": 0.35,
      "over_partial5": 0.08,
      "over_partial6": 0.20,
      "over_partial7": 0.06,
      "over_velBright": 0.55,
      "over_filterCutoff": 15000.0,
      "over_filterRes": 0.45,
      "over_ampAtk": 0.003,
      "over_ampDec": 0.45,
      "over_ampSus": 0.0,
      "over_ampRel": 1.8,
      "over_lfo1Rate": 0.30,
      "over_lfo1Depth": 0.15,
      "over_lfo2Rate": 0.12,
      "over_lfo2Depth": 0.10,
      "over_resoMix": 0.30,
      "over_macroDepth": 0.30,
      "over_macroColor": 0.60,
      "over_macroCoupling": 0.15,
      "over_macroSpace": 0.35
    }
  }
}
```

---

### Preset 6: Pi Shimmer (Awakening Version)
**Concept**: Pi constant at mid depth, slow LFO1, high resonator — non-repeating shimmer.
**Teaching**: Pi's convergent table spreads far and wide; slow LFO1 causes a non-periodic depth scan. The shimmer pattern never repeats.
**Mood**: Aether | **DNA**: brightness 0.65, warmth 0.42, movement 0.52, density 0.48, space 0.70, aggression 0.08

```json
{
  "schema_version": 1,
  "name": "Pi Shimmer",
  "mood": "Aether",
  "engines": ["Overtone"],
  "author": "XO_OX Designs",
  "version": "1.0.0",
  "description": "Pi constant. Slow LFO1 at 0.12 Hz sweeps convergent depth — because depth and LFO are irrational relative to each other, the shimmer pattern never repeats. High resonator mix adds a chiming tail at the fundamental. The digits of Pi as a spectral event horizon.",
  "tier": "awakening",
  "tags": ["pi", "shimmer", "aether", "awakening", "non-repeating", "irrational", "meditative"],
  "macroLabels": ["DEPTH", "COLOR", "COUPLING", "SPACE"],
  "dna": {
    "brightness": 0.65,
    "warmth": 0.42,
    "movement": 0.52,
    "density": 0.48,
    "space": 0.70,
    "aggression": 0.08
  },
  "parameters": {
    "Overtone": {
      "over_constant": 0,
      "over_depth": 3.5,
      "over_partial0": 0.88,
      "over_partial1": 0.60,
      "over_partial2": 0.42,
      "over_partial3": 0.30,
      "over_partial4": 0.22,
      "over_partial5": 0.16,
      "over_partial6": 0.12,
      "over_partial7": 0.08,
      "over_velBright": 0.45,
      "over_filterCutoff": 16000.0,
      "over_filterRes": 0.35,
      "over_ampAtk": 0.18,
      "over_ampDec": 0.55,
      "over_ampSus": 0.68,
      "over_ampRel": 2.8,
      "over_lfo1Rate": 0.12,
      "over_lfo1Depth": 0.55,
      "over_lfo2Rate": 0.05,
      "over_lfo2Depth": 0.22,
      "over_resoMix": 0.48,
      "over_macroDepth": 0.50,
      "over_macroColor": 0.45,
      "over_macroCoupling": 0.0,
      "over_macroSpace": 0.58
    }
  }
}
```

---

### Preset 7: Tritone Suspension
**Concept**: Sqrt2 constant — the tritone machine. Near-equal partial amplitudes expose the ambiguous tritone quality.
**Teaching**: Sqrt2 entries cluster between 1.4 and 1.5 (the tritone). Equal-weighted partials produce an unresolved harmonic suspension.
**Mood**: Flux | **DNA**: brightness 0.52, warmth 0.40, movement 0.38, density 0.70, space 0.40, aggression 0.22

```json
{
  "schema_version": 1,
  "name": "Tritone Suspension",
  "mood": "Flux",
  "engines": ["Overtone"],
  "author": "XO_OX Designs",
  "version": "1.0.0",
  "description": "Sqrt2 constant. Pell number convergents cluster at the tritone (1.41421... = 2^(1/2)). Near-equal partial amplitudes produce a dense, unresolved harmonic suspension — neither consonant nor chaotic. The rational approximations to sqrt(2) as perpetual harmonic tension.",
  "tier": "awakening",
  "tags": ["sqrt2", "tritone", "tension", "flux", "awakening", "pell", "suspension"],
  "macroLabels": ["DEPTH", "COLOR", "COUPLING", "SPACE"],
  "dna": {
    "brightness": 0.52,
    "warmth": 0.40,
    "movement": 0.38,
    "density": 0.70,
    "space": 0.40,
    "aggression": 0.22
  },
  "parameters": {
    "Overtone": {
      "over_constant": 3,
      "over_depth": 1.5,
      "over_partial0": 0.80,
      "over_partial1": 0.72,
      "over_partial2": 0.68,
      "over_partial3": 0.65,
      "over_partial4": 0.62,
      "over_partial5": 0.60,
      "over_partial6": 0.58,
      "over_partial7": 0.55,
      "over_velBright": 0.35,
      "over_filterCutoff": 10000.0,
      "over_filterRes": 0.40,
      "over_ampAtk": 0.08,
      "over_ampDec": 0.35,
      "over_ampSus": 0.65,
      "over_ampRel": 2.0,
      "over_lfo1Rate": 0.18,
      "over_lfo1Depth": 0.25,
      "over_lfo2Rate": 0.08,
      "over_lfo2Depth": 0.18,
      "over_resoMix": 0.20,
      "over_macroDepth": 0.28,
      "over_macroColor": 0.48,
      "over_macroCoupling": 0.35,
      "over_macroSpace": 0.38
    }
  }
}
```

---

### Preset 8: Pell Number Cloud
**Concept**: Sqrt2 at maximum depth — all convergents collapsed near 1.41421. A dense unison cloud.
**Teaching**: At high depth, Sqrt2's convergents have all converged. The 8 partials are all nearly the same ratio — a tight spectral cluster. Maximum density, minimum spread.
**Mood**: Flux | **DNA**: brightness 0.48, warmth 0.45, movement 0.58, density 0.82, space 0.50, aggression 0.18

```json
{
  "schema_version": 1,
  "name": "Pell Number Cloud",
  "mood": "Flux",
  "engines": ["Overtone"],
  "author": "XO_OX Designs",
  "version": "1.0.0",
  "description": "Sqrt2 constant at depth 6-7. Pell convergents have converged: all 8 partials are packed within 0.01 of each other near the tritone. Maximum spectral density for this engine. A slow, throbbing unison cloud. LFO1 at 0.25 Hz creates gentle amplitude beating between nearly-identical partials.",
  "tier": "awakening",
  "tags": ["sqrt2", "cloud", "dense", "flux", "awakening", "pell", "tritone"],
  "macroLabels": ["DEPTH", "COLOR", "COUPLING", "SPACE"],
  "dna": {
    "brightness": 0.48,
    "warmth": 0.45,
    "movement": 0.58,
    "density": 0.82,
    "space": 0.50,
    "aggression": 0.18
  },
  "parameters": {
    "Overtone": {
      "over_constant": 3,
      "over_depth": 6.0,
      "over_partial0": 0.75,
      "over_partial1": 0.72,
      "over_partial2": 0.70,
      "over_partial3": 0.68,
      "over_partial4": 0.66,
      "over_partial5": 0.64,
      "over_partial6": 0.62,
      "over_partial7": 0.60,
      "over_velBright": 0.30,
      "over_filterCutoff": 9500.0,
      "over_filterRes": 0.45,
      "over_ampAtk": 0.10,
      "over_ampDec": 0.40,
      "over_ampSus": 0.70,
      "over_ampRel": 2.5,
      "over_lfo1Rate": 0.25,
      "over_lfo1Depth": 0.30,
      "over_lfo2Rate": 0.10,
      "over_lfo2Depth": 0.25,
      "over_resoMix": 0.32,
      "over_macroDepth": 0.22,
      "over_macroColor": 0.42,
      "over_macroCoupling": 0.45,
      "over_macroSpace": 0.45
    }
  }
}
```

---

### Preset 9: Hollow Reed
**Concept**: Phi constant with square-wave-like partial weighting (partials 1, 3, 5, 7 suppressed). Clarinet/reed quality.
**Teaching**: Suppressing even-indexed partials (in the Fibonacci table context) creates the hollow tube of a woodwind. Demonstrates that amplitude curve design shapes instrument family.
**Mood**: Prism | **DNA**: brightness 0.58, warmth 0.55, movement 0.20, density 0.38, space 0.25, aggression 0.12

```json
{
  "schema_version": 1,
  "name": "Hollow Reed",
  "mood": "Prism",
  "engines": ["Overtone"],
  "author": "XO_OX Designs",
  "version": "1.0.0",
  "description": "Phi constant. Alternate partials suppressed (near-zero amplitudes on indices 1, 3, 5, 7). The remaining odd-indexed Fibonacci partials (2/1, 5/3, 13/8, 34/21) create a hollow reed quality — the mathematics of a clarinet's odd-harmonic dominance applied to irrational Fibonacci ratios. Expressive with mod wheel for depth sweep.",
  "tier": "awakening",
  "tags": ["phi", "reed", "hollow", "prism", "awakening", "fibonacci", "woodwind"],
  "macroLabels": ["DEPTH", "COLOR", "COUPLING", "SPACE"],
  "dna": {
    "brightness": 0.58,
    "warmth": 0.55,
    "movement": 0.20,
    "density": 0.38,
    "space": 0.25,
    "aggression": 0.12
  },
  "parameters": {
    "Overtone": {
      "over_constant": 2,
      "over_depth": 1.5,
      "over_partial0": 1.0,
      "over_partial1": 0.04,
      "over_partial2": 0.55,
      "over_partial3": 0.04,
      "over_partial4": 0.30,
      "over_partial5": 0.04,
      "over_partial6": 0.18,
      "over_partial7": 0.04,
      "over_velBright": 0.42,
      "over_filterCutoff": 10000.0,
      "over_filterRes": 0.30,
      "over_ampAtk": 0.025,
      "over_ampDec": 0.30,
      "over_ampSus": 0.72,
      "over_ampRel": 1.2,
      "over_lfo1Rate": 0.10,
      "over_lfo1Depth": 0.12,
      "over_lfo2Rate": 0.06,
      "over_lfo2Depth": 0.08,
      "over_resoMix": 0.22,
      "over_macroDepth": 0.25,
      "over_macroColor": 0.38,
      "over_macroCoupling": 0.0,
      "over_macroSpace": 0.22
    }
  }
}
```

---

### Preset 10: Nautilus Ascent
**Concept**: Phi constant, DEPTH macro at full sweep range, iridescence on, SPACE at 0.55 — the "full journey" preset.
**Teaching**: Everything working together. DEPTH macro sweeps the spiral; COUPLING provides per-partial iridescence; SPACE gives the hall. The intended performance preset for a new user.
**Mood**: Aether | **DNA**: brightness 0.65, warmth 0.52, movement 0.60, density 0.58, space 0.58, aggression 0.06

```json
{
  "schema_version": 1,
  "name": "Nautilus Ascent",
  "mood": "Aether",
  "engines": ["Overtone"],
  "author": "XO_OX Designs",
  "version": "1.0.0",
  "description": "Phi constant. DEPTH macro set mid-range so the mod wheel sweeps the full harmonic-to-irrational journey. COUPLING at 0.40 for living per-partial iridescence. SPACE at moderate reverb. Aftertouch adds upper-partial shimmer (apply pressure for brilliance). The signature OVERTONE performance preset: play it slowly, use the wheel deliberately.",
  "tier": "awakening",
  "tags": ["phi", "nautilus", "journey", "aether", "awakening", "performance", "expressive"],
  "macroLabels": ["DEPTH", "COLOR", "COUPLING", "SPACE"],
  "dna": {
    "brightness": 0.65,
    "warmth": 0.52,
    "movement": 0.60,
    "density": 0.58,
    "space": 0.58,
    "aggression": 0.06
  },
  "parameters": {
    "Overtone": {
      "over_constant": 2,
      "over_depth": 2.5,
      "over_partial0": 0.90,
      "over_partial1": 0.52,
      "over_partial2": 0.38,
      "over_partial3": 0.28,
      "over_partial4": 0.22,
      "over_partial5": 0.18,
      "over_partial6": 0.15,
      "over_partial7": 0.12,
      "over_velBright": 0.50,
      "over_filterCutoff": 13000.0,
      "over_filterRes": 0.38,
      "over_ampAtk": 0.06,
      "over_ampDec": 0.45,
      "over_ampSus": 0.70,
      "over_ampRel": 2.5,
      "over_lfo1Rate": 0.06,
      "over_lfo1Depth": 0.28,
      "over_lfo2Rate": 0.04,
      "over_lfo2Depth": 0.18,
      "over_resoMix": 0.30,
      "over_macroDepth": 0.45,
      "over_macroColor": 0.55,
      "over_macroCoupling": 0.40,
      "over_macroSpace": 0.52
    }
  }
}
```

---

## Awakening Preset Summary Table

| # | Name | Constant | Core Concept | Mood | DEPTH | COLOR | COUPLING | SPACE |
|---|------|----------|--------------|------|-------|-------|----------|-------|
| 1 | Fibonacci Shell | Phi | Harmonic Fibonacci — the clean starting point | Foundation | 0.20 | 0.45 | 0.00 | 0.28 |
| 2 | Golden Drift | Phi | Converging on 1.618 — alternating-side beating | Atmosphere | 0.35 | 0.52 | 0.22 | 0.40 |
| 3 | Euler Organ | E | Dense organ-stop character of E's 4th/5th cluster | Foundation | 0.25 | 0.30 | 0.00 | 0.52 |
| 4 | Euler Breath | E | Long attack + E cluster = breath-held choir swell | Atmosphere | 0.30 | 0.35 | 0.10 | 0.58 |
| 5 | Pi Metal | Pi | Bell weighting + wide Pi spread = struck metal | Prism | 0.30 | 0.60 | 0.15 | 0.35 |
| 6 | Pi Shimmer | Pi | Slow depth scan — never-repeating shimmer pattern | Aether | 0.50 | 0.45 | 0.00 | 0.58 |
| 7 | Tritone Suspension | Sqrt2 | Equal weights expose unresolved tritone ambiguity | Flux | 0.28 | 0.48 | 0.35 | 0.38 |
| 8 | Pell Number Cloud | Sqrt2 | Converged Pell numbers — maximum density cloud | Flux | 0.22 | 0.42 | 0.45 | 0.45 |
| 9 | Hollow Reed | Phi | Alternate-partial suppression = woodwind hollow tone | Prism | 0.25 | 0.38 | 0.00 | 0.22 |
| 10 | Nautilus Ascent | Phi | Full-engine performance — wheel sweeps the spiral | Aether | 0.45 | 0.55 | 0.40 | 0.52 |

---

## Closing Meditation

The four constants are four different attitudes toward irrationality.

**Phi** is patient. Its Fibonacci convergents take the long road — over-shooting, then
under-shooting, over-shooting again, each time by less. Every partial is in gentle,
unresolvable argument with the golden ratio. This is why Phi sounds alive: it never
settles. It is the Nautilus itself.

**E** is dense. Its convergents cluster around the space between a fourth and a fifth,
that musically rich zone where natural harmonics live. E makes organs. It makes choir.
It makes the sound of mathematics that was discovered in the counting of animals and the
study of population growth — e is alive in a different way from phi, a biological way.

**Sqrt2** converges fast and early. By depth 4, it has already arrived. This early
convergence produces the most static constant at high depth — and the most interesting
at depth 1 or 2, where the 3/2 (fifth) and 7/5 (near-tritone) entries sit in maximum
contrast. Sqrt2 is industrial. Ambiguous. The tritone that mathematics derived, not ears.

**Pi** is unruly. Its convergents span the widest range and have the least uniform
spacing. 3/1 is a natural third harmonic; 22/7 is the famous approximation; 113/106
is a near-unison; 113/33 reaches past 3× the fundamental. Pi's table was built for
spread, not convergence. This is why it sounds metallic and shimmering: the partials
are genuinely far apart in frequency, and the LFO that sweeps depth is sweeping across
a much wider spectral landscape than the other constants.

The DEPTH macro is a finger tracing the spiral. The mod wheel extends that finger.
Aftertouch makes the shell glow. SPACE is the ocean around the shell.

The Nautilus does not grow in integers.

---

*Guru Bin Retreat — OVERTONE | Tier: Awakening | 10 presets | 2026-03-21*
*Next tier: Transcendental — 15 presets + XPN export + PDF booklet*
