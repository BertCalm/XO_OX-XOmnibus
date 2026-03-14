# Oracle Synthesis Guide

**Engine:** ORACLE | **Accent:** Prophecy Indigo `#4B0082`
**Parameter prefix:** `oracle_` | **Max voices:** 8
**XOmnibus seance score:** 8.6/10 — Buchla: 10/10 (only perfect score across all 24 seances)

---

## Introduction

Oracle is the only synthesizer in the XOmnibus fleet that uses Xenakis's 1991 GENDY (GENération DYnamique) synthesis algorithm. Where every other engine in XOmnibus — even the experimental ones like OUROBOROS (chaotic ODE) and ORGANON (metabolic) — produces output that is mathematically determined by its inputs, Oracle introduces genuine randomness as a creative force. Given identical parameter values, Oracle will never produce the same sound twice.

The engine combines two systems that have no business being in the same instrument and are made extraordinary by their proximity:

**GENDY stochastic synthesis:** Waveforms are not calculated, oscillated, or sampled. They are composed by constrained random walks — N breakpoints drifting across a time-amplitude space each cycle, with the composer controlling the character of the randomness rather than each individual shape. The result ranges from slowly mutating drones to convulsing noise to pitched tones with organic flutter.

**Maqam microtonal tuning:** The pitch system is not Western 12-tone equal temperament but the ancient modal framework of Arabic, Turkish, and Persian classical music — with quarter-tone intervals (50 cents) that have no equivalent in Western tuning and context-dependent intervals that differ between ascending and descending melody.

Neither system alone would earn Buchla's only perfect score. Together, they produce sounds that are simultaneously ancient — grounded in tuning systems centuries older than the piano — and genuinely alien, sounds that have never been heard before and will never be heard again in the same form.

---

## Part 1: GENDY Stochastic Synthesis

### 1.1 Historical Context

Iannis Xenakis (1922–2001) was an architect, mathematician, and composer who spent his career applying rigorous mathematical formalism to music. He studied under Le Corbusier and designed the Philips Pavilion for the 1958 Brussels World's Fair (where *Poème électronique* by Varèse was premiered), later working at Pierre Schaeffer's GRM in Paris and founding his own research center, CEMAMu (Centre d'Études de Mathématique et Automatique Musicales).

In 1977 he created the UPIC — a large electromagnetic drawing tablet connected to a minicomputer. Composers drew graphical curves with a stylus; the system synthesized the corresponding sounds. The UPIC demonstrated that waveforms could be *drawn* rather than calculated, composed as shapes rather than equations.

In 1991 Xenakis published GENDY in the journal *Perspectives of New Music*. GENDY was the logical conclusion of the UPIC's philosophy: where UPIC required composers to draw every curve by hand, GENDY delegates the drawing to probability distributions. The algorithm "draws" the waveform through constrained random walks, with the composer controlling the character of the randomness — not each individual shape. Xenakis implemented GENDY on a NeXT computer at CEMAMu, producing the pieces *GENDY1* (1991), *GENDY2* (1992), and *GENDY3* (1991). These pieces consist entirely of GENDY-synthesized material — no samples, no recorded audio, no conventional oscillators.

GENDY has never been commercialized. Academic implementations exist (SuperCollider's `Gendy1`/`Gendy2`/`Gendy3` UGens, and Max/MSP approximations), but no commercial synthesizer before Oracle has brought the genuine Xenakis algorithm to a musician-facing interface with performance controls. Oracle is the UPIC that draws itself, given to players rather than programmers.

### 1.2 The Algorithm: How GENDY Constructs Sound

#### Breakpoints

GENDY constructs each waveform cycle from N discrete control points called **breakpoints**. Each breakpoint has two coordinates:

- **Time offset:** Its horizontal position within the cycle (0 = cycle start, 1 = cycle end)
- **Amplitude:** Its height at that position (-1 = minimum, +1 = maximum)

At any moment, the N breakpoints form a constellation that defines the shape of one waveform cycle. Oracle extends this into audio by using cubic Hermite (Catmull-Rom) spline interpolation to generate smooth per-sample output between breakpoints.

#### The Random Walk

At the end of each complete waveform cycle, every breakpoint undergoes a stochastic random walk — its time offset and amplitude are perturbed by a random step drawn from the current probability distribution. This is the engine's fundamental act:

```
per cycle, for each breakpoint i:
  breakpoint[i].timeOffset += sampleDistribution(rng, distribution) * scaledTimeStep
  breakpoint[i].amplitude  += sampleDistribution(rng, distribution) * scaledAmpStep
  apply_mirror_barrier(breakpoint[i])

sort breakpoints by timeOffset
normalize timeOffset span to [0, 1]
```

The waveform produced by the *next* cycle is different from the current one — not randomly different, but stochastically evolved. The character of the difference is controlled by the probability distribution and step size parameters. A low step size produces nearly identical successive cycles (slow evolution). A high step size produces radically different cycles (fast transformation).

Because breakpoints are updated once per cycle and the cycle period equals 1/frequency, at higher pitches the waveform evolves faster in wall-clock time. A 440Hz tone evolves 440 times per second; a 55Hz tone evolves 55 times per second. This creates a natural relationship between pitch and timbral stability.

#### The Probability Distributions

Xenakis's key insight was that the probability distribution governing the random walk determines the *musical character* of the sound. Oracle implements two distributions with continuous morphing:

**Cauchy distribution** (`oracle_distribution = 0.0`):
```
sample = tan(π * (u - 0.5)) × 0.1
```
Heavy-tailed: most random steps are small, but the distribution has no finite variance — occasional extreme jumps occur. A GENDY voice using Cauchy distribution is mostly stable, with sudden dramatic breakpoint relocations that resolve back into the general flow. The sound of prophecy: calm surface, sudden revelation. Xenakis used Cauchy exclusively in his original GENDY3 implementation.

**Logistic distribution** (`oracle_distribution = 1.0`):
```
sample = log(u / (1 - u)) × 0.15
```
Sigmoid-like tails, finite variance. Most steps are moderate, with no extreme outliers. GENDY with logistic distribution evolves continuously and gradually — a slow geological drift rather than episodic revelation. The sound of water eroding stone over millennia.

The `oracle_distribution` parameter morphs continuously between these two: `0.0` is pure Cauchy, `1.0` is pure Logistic, and values in between blend them linearly.

#### Mirror Barriers

When a breakpoint's random walk pushes it past a boundary (amplitude beyond [-1, 1] or time beyond [0, 1]), the value is *reflected* back — like a billiard ball bouncing off a wall. The momentum of the walk is preserved, just redirected.

Oracle extends Xenakis's original hard barriers with a parameterized `oracle_barrierElasticity`:

- `elasticity = 0.0`: The breakpoint is clamped to the boundary and stops. Hard reflection, sharp constraint. The breakpoint "sticks" at the wall.
- `elasticity = 1.0`: Full elastic reflection — energy is fully preserved. The overshoot beyond the boundary is mirrored back inside. The breakpoint "bounces" off the wall with its full momentum.

Values in between create intermediate behavior — the breakpoint reflects, but with some energy loss on each bounce. This means a breakpoint that overshoots a boundary multiple times progressively loses energy, eventually settling near the boundary region. This creates a natural accumulation of breakpoints near the amplitude extremes when elasticity is low — a more "saturated" quality.

#### Time Normalization

After each round of random walks, the breakpoints are sorted by time offset and normalized so the first breakpoint sits at 0 and the span covers the full [0, 1] cycle. This prevents waveform collapse (all breakpoints drifting to the same time position) and ensures the pitch remains stable even as the waveform shape evolves. The frequency is controlled by how fast the phase advances through the normalized breakpoint constellation.

#### Stereo Decorrelation

Oracle reads the breakpoint constellation at two slightly different phases: the left channel reads at the current phase, the right channel reads at phase + 0.01 (1% of the waveform cycle offset). This tiny phase offset creates subtle stereo width without requiring a second oscillator or chorus effect. The two channels share the same breakpoint evolution but are always slightly out of phase with each other, creating the impression of spatial depth.

#### Cycle-Boundary Smoothing

When breakpoints evolve at a cycle boundary, the waveform shape can change discontinuously — the last sample of one cycle and the first sample of the next can be far apart in amplitude. Oracle prevents audible clicks by crossfading over 64 samples (~1.5ms at 44.1kHz), blending the last sample of the previous cycle with the new waveform at the start of the next. This crossfade is inaudible at normal pitches and transparent at audio rates.

### 1.3 The Stochastic Depth Control System

The amount of breakpoint evolution per cycle is the product of three things:

1. **`oracle_drift`** — The global evolution intensity. At 0, breakpoints do not walk at all; the waveform is frozen at its initial sine shape. At 1, breakpoints walk at maximum step size.

2. **Stochastic envelope** (`oracle_stochEnvAttack/Decay/Sustain/Release`) — A second ADSR envelope that controls how much the breakpoints drift over the note's lifetime. This is independent of the amplitude envelope. It allows the waveform to start stable (low stochastic envelope) and evolve into chaos (high stochastic envelope) over the course of the note, or vice versa.

3. **LFO modulation** — LFO1 modulates the time step, LFO2 modulates the amplitude step. This creates rhythmic patterns in the evolution: a slow LFO on time step causes the waveform to pulse in and out of temporal instability; a fast LFO on amplitude step causes rapid oscillation between smooth and chaotic timbre.

Effective stochastic depth per cycle:
```
stochasticDepth = stochasticEnvelope.level × effectiveDrift
actualTimeStep  = oracle_timeStep × stochasticDepth
actualAmpStep   = oracle_ampStep  × stochasticDepth
```

Both step parameters use quadratic scaling internally (`step² × constant`) to create a gentle onset of chaos at low values and aggressive drift at high values.

### 1.4 The `oracle_drift` Parameter in Detail

`oracle_drift` is the master control for stochastic evolution intensity. It is arguably Oracle's most expressive parameter.

- **`0.0`:** Breakpoints do not walk. The waveform is frozen at its initial sine shape (all voices initialize as a sine wave before evolution begins). The result sounds like a pure sine oscillator with the current maqam-adjusted pitch.

- **`0.1–0.2`:** Extremely slow evolution. The waveform breathes over minutes rather than seconds — subtle, almost subliminal mutation. Useful for drone textures where the timbral shift is felt rather than heard consciously.

- **`0.3–0.4` (default):** Moderate evolution. The waveform changes audibly over the course of several seconds. Each cycle introduces small perturbations that accumulate into meaningful timbral shifts over time. The characteristic "living tone" quality.

- **`0.5–0.6`:** Rapid evolution. The waveform changes character within individual notes — starting one way and arriving somewhere different by the note's end. Useful for melodic leads where each note has its own timbral arc.

- **`0.7–0.8`:** Aggressive evolution. Near-continuous timbral transformation. The waveform rarely holds a consistent shape for more than a few cycles. Pitched, but richly unstable.

- **`0.9–1.0`:** Maximum evolution. The waveform transforms completely over just a few cycles. At high frequencies (above ~200Hz), the timbre shifts dozens of times per second. The distinction between "pitch" and "timbre" begins to blur.

Aftertouch (channel pressure) maps to `oracle_drift` with a sensitivity of 0.15, allowing performance control over evolution intensity. Pressing harder increases stochastic chaos in real time.

### 1.5 Sound Character

GENDY produces a distinctive sonic vocabulary that has no close equivalent in conventional synthesis:

**Pitched but organic:** At moderate drift settings, Oracle produces tones with a clear fundamental frequency but with continuously evolving harmonic content. The pitch is stable (controlled by the maqam-adjusted frequency), but the timbre — which harmonics are present and at what amplitudes — shifts each waveform cycle. The result is a living tone that sounds more like a bowed instrument or wind column than an electronic oscillator.

**Spectral unpredictability:** Because the waveform shape is never predetermined, the harmonic series changes every cycle. There is no reliable "sound" to a GENDY wave at a given pitch — only a probability distribution over possible sounds. Low breakpoint counts (8–12) produce relatively simple spectra. High counts (24–32) produce dense, noise-like textures where individual harmonics are difficult to isolate.

**Metallic and percussive possibilities:** With high amplitude step size (`oracle_ampStep > 0.6`) and low breakpoint count (8–10), breakpoints can make dramatic jumps that produce sharp amplitude discontinuities within cycles. These create buzz, ring, and metallic edge — textures related to FM synthesis but with a different kind of unpredictability.

**Noise-to-pitch continuum:** By increasing breakpoint count and amplitude step together, the waveform complexity increases to the point where the pitch becomes difficult to perceive — the waveform is so complex and changes so rapidly that it approaches broadband noise. Decreasing these parameters brings clarity back. Oracle can traverse the full continuum from pitched tone to textured noise without changing synthesis technique.

**Temporal evolution as composition:** A single held ORACLE note is not a static event but a miniature composition. The breakpoints evolve according to probability — sometimes gradually, sometimes with sudden dramatic shifts (especially on Cauchy distribution). Long notes develop arcs and trajectories. This is the GENDY property Xenakis called "temporal morphology" — the sound has a direction, a history, an implicit narrative.

---

## Part 2: Maqam Microtonal Mode

### 2.1 Cultural and Historical Context

Maqam (Arabic: مقام, pl. maqamat) is the modal framework underlying Arabic, Turkish, Persian, and Central Asian classical music. The word literally means "place" or "position" — the tonic and modal area where a musical piece resides. Maqam has been theorized, practiced, and documented for over a thousand years; Al-Kindi wrote about it in the 9th century, Al-Farabi systematized it in the 10th, and it remains the living basis of musical practice across North Africa, the Middle East, Turkey, and Central Asia today.

Maqam is frequently described as "like Western modes" or "Middle Eastern scales" — both descriptions miss the point. Maqam is a behavioral system, not a pitch collection. It defines:

- Which pitches are available (the interval structure, including quarter-tones)
- Which pitches are emphasized (the tonic, the dominant, the sensitive tones)
- How those pitches should be approached and departed (melodic conventions)
- What emotional register the mode occupies (each maqam has a recognized affective character)
- How ascending and descending movement differs (many maqamat have different intervals going up vs. coming down)

Oracle implements the pitch-collection aspect — the microtonal interval structure — and leaves the behavioral, melodic, and ornamental aspects to the player. The `oracle_gravity` parameter controls how strongly the tuning system pulls toward maqam intervals.

**On cultural respect:** Maqam is a living musical tradition practiced by hundreds of millions of people. It is not an "exotic scale" or a source of "Eastern" atmosphere. Oracle's maqam implementation is offered as a tool for exploring these interval structures — both for musicians from traditions where maqam is native and for experimentalists interested in microtonal synthesis. Neither user should approach it as tourism.

### 2.2 Microtonal Intervals

Western 12-tone equal temperament divides the octave into 12 equal semitones of 100 cents each. Maqam uses the semitone as a basic unit but extends it with the **quarter-tone** (approximately 50 cents) — an interval halfway between two adjacent semitones. Quarter-tones are structural intervals in maqam, not ornaments.

Notation used in this guide:
- Standard semitone: 100 cents (e.g., C to C#)
- Quarter-tone: 50 cents (e.g., D to D half-sharp, written D†)
- Three-quarter-tone: 150 cents (e.g., three-quarter step, halfway between a semitone and a whole tone)
- Augmented second: 300 cents (three semitones — the characteristic interval of Maqam Hijaz)

Turkish maqam theory uses a 53-comma octave division (one comma ≈ 22.6 cents) for more precise specification; Arabic theory uses 24 quarter-tones per octave. Oracle implements the Arabic quarter-tone system, which is more widely accessible to Western-trained musicians.

### 2.3 The Eight Implemented Maqamat

Oracle implements eight maqamat plus 12-TET bypass. Intervals are given in cents from the scale root, with the note names shown starting from C for accessibility:

#### Rast (رست) — "The Foundation"
```
C    D    E♭†   F    G    A    B♭†   C
0   200   350  500  700  900  1050  1200
```
Intervals: 200, 150, 150, 200, 200, 150, 150

Rast is the most common maqam in Arabic music, considered the foundation or mother of all maqamat. Its characteristic feature is the E♭† (E half-flat, a quarter-tone above E♭ and a quarter-tone below E natural) and B♭† (B half-flat). These neutral thirds give Rast its distinctive character — neither major nor minor, but occupying a middle ground that Western ears sometimes describe as "neither sad nor happy." In Egypt, Rast is often the first maqam taught to students. Rast melodic movement ascends through the full octave and descends back through identical pitches, unlike many maqamat that change intervals between ascending and descending movement.

*Sound character:* Grounded, neutral, contemplative. The neutral third creates a floating quality — rooted but unbounded.

#### Bayati (بياتي) — "The Devotional"
```
D    E♭†   F    G    A    B♭   C    D
0   150   300  500  700  800  1000 1200
```
Intervals: 150, 150, 200, 200, 100, 200, 200

Bayati is the most emotionally weighted of the common maqamat — associated with contemplation, devotion, longing, and introspection. Its opening interval is a three-quarter-tone step (150 cents) followed by another three-quarter-tone step, creating a highly characteristic "neutral" second at the very beginning of the scale. Bayati is used extensively in Quran recitation (Tajweed) and is said to evoke the spirit of devotional music across both Islamic and Christian Arab traditions. Descending Bayati often uses the same pitches as ascending, though with more emphasis on the middle registers.

*Sound character:* Melancholic, yearning, intimate. The concentrated lower tetrachord creates emotional density.

#### Saba (صبا) — "The Sorrowful"
```
D    E♭†   F    G♭   A    B♭   C    D
0   150   300  400  700  800  1000 1200
```
Intervals: 150, 150, 100, 300, 100, 200, 200

Saba is among the darkest of the common maqamat. Its distinctive feature is the diminished fourth (G♭, which is 400 cents from the root — three semitones rather than the expected four of a perfect fourth). This creates a compressed, constricted quality in the lower tetrachord. The augmented third from G♭ to A (300 cents) that follows gives Saba its dramatic, anguished character. Saba is traditionally associated with expressions of grief, longing, and spiritual yearning. In Egyptian musical culture, Saba is sometimes called "the maqam of tears."

*Sound character:* Dark, dramatic, anguished. The diminished quality creates a sense of constriction and release.

#### Hijaz (حجاز) — "The Exotic"
```
D    E♭   F#   G    A    B♭   C    D
0   100   400  500  700  800  1000 1200
```
Intervals: 100, 300, 100, 200, 100, 200, 200

Hijaz is named after the western coastal region of the Arabian Peninsula (where Mecca and Medina are located). Its signature feature is the augmented second — the 300-cent interval between E♭ and F# — which gives Hijaz the most immediately recognizable sound to Western ears. This interval appears in the Western "harmonic minor" scale (between the 6th and 7th degrees), but in Hijaz it sits at the very opening of the scale (between the 2nd and 3rd degrees). Hijaz has been widely adopted in popular music globally — it appears in flamenco (as the "Phrygian dominant" scale), in klezmer music, in Turkish folk music, and in Western film scoring as a signifier of "the Orient." Hijaz is the maqam most accessible to Western-trained ears while still being genuinely microtonal in its authentic practice (the E♭ is sometimes raised slightly above the equal-tempered E♭ in performance).

*Sound character:* Bright, dramatic, expansive. The augmented second creates an immediate sense of exoticism and grandeur.

#### Sikah (سيكاه) — "The Floating"
```
E♭†   F    G    A♭   B♭†   C    D    E♭†
0    150  350  450  650   850  1050 1200
```
Intervals: 150, 200, 100, 200, 200, 200, 150

Sikah is among the most "alien" of the common maqamat to Western ears because its root is a quarter-tone — E♭† is not a note that exists in 12-TET. The entire maqam is built on a pitch that has no keyboard equivalent, which means that in Oracle, even with the root note played, the resulting pitch will be microtonally offset from 12-TET if gravity is set high. Sikah is associated with ethereal, floating, slightly otherworldly qualities. It is used in elevated, spiritual contexts and is sometimes considered a "philosopher's maqam" because of its cerebral, detached character. The equal whole-tone movement through its middle register creates an unusual symmetry.

*Sound character:* Ethereal, floating, otherworldly. The quarter-tone root creates an inherent sense of displacement.

#### Nahawand (نهاوند) — "The Familiar"
```
C    D    E♭   F    G    A♭   B♭   C
0   200  300  500  700  800  1000 1200
```
Intervals: 200, 100, 200, 200, 100, 200, 200

Nahawand is the maqam closest to Western harmonic minor — its intervals are nearly identical to the natural minor (Aeolian mode). However, Nahawand performance practice is distinct from Western minor: the emphasis on certain degrees, the ornamental patterns, and the ascending/descending variations (Nahawand ascending sometimes raises the 6th and 7th degrees, creating a "melodic minor" variant) give it a different emotional texture. Nahawand is accessible to Western ears while still being authentically maqam in its behavioral dimension. Its name comes from the Iranian city of Nahavand.

*Sound character:* Warm, familiar, introspective. The minor quality is accessible but the performance practice reveals depth.

#### Kurd (كرد) — "The Resolute"
```
D    E♭   F    G    A    B♭   C    D
0   100  300  500  700  800  1000 1200
```
Intervals: 100, 200, 200, 200, 100, 200, 200

Kurd opens with a semitone from the root — its first interval is 100 cents, like the Phrygian mode. This gives Kurd a dark, resolute quality from the outset; the scale begins with compression rather than expansion. Kurd is sometimes called the "Phrygian" of Arabic music, though its upper tetrachord differs from Phrygian and its performance practice follows maqam conventions rather than Greek modal logic. Kurd is associated with steadiness, determination, and a kind of quiet gravity.

*Sound character:* Dark, stable, determined. The semitone opening creates groundedness rather than melancholy.

#### Ajam (عجم) — "The Bright"
```
C    D    E    F    G    A    B    C
0   200  400  500  700  900  1100 1200
```
Intervals: 200, 200, 100, 200, 200, 200, 100

Ajam is the major scale — Ionian mode. In Arabic musical theory, Ajam is included as one of the fundamental maqamat despite being identical in interval structure to the Western major scale. What distinguishes Ajam in practice is not its interval structure but its performance conventions, ornamental patterns, and the specific pitches it emphasizes. Ajam is used for celebratory, bright, assertive music. The word "Ajam" in Arabic originally meant "Persia" or "foreign" — the major scale was an adopted element from Persian musical culture, and the name acknowledges this origin. In Oracle, Ajam with gravity set to 0.5 provides a gentle "pull" toward the major scale intervals, which might differ slightly from 12-TET's precisely equal divisions.

*Sound character:* Bright, celebratory, assertive. The equal whole tones of Ajam create a sense of openness and confidence.

### 2.4 The Gravity System

`oracle_gravity` controls how strongly the maqam tuning system affects the pitch. It is a continuous blend between 12-TET and maqam intonation.

**`gravity = 0.0`:** All pitches are standard 12-TET. The maqam selection has no effect on tuning. MIDI note numbers produce their equal-tempered frequencies. This setting makes Oracle sound like any other synthesizer in terms of pitch.

**`gravity = 0.5`:** Pitches are pulled halfway toward maqam-correct intervals. A note that is 50 cents above or below its maqam target in 12-TET will be shifted to 25 cents away — audibly inflected toward the maqam but not fully there. This setting is useful for blending ORACLE with 12-TET instruments in an ensemble while adding subtle microtonal color.

**`gravity = 1.0`:** Full maqam intonation. Pitches are computed entirely from the maqam table. Quarter-tone intervals are reproduced faithfully — E♭† in Rast will be exactly 350 cents above the root, not the 300 or 400 cents that the nearest semitones would provide.

The gravity calculation uses linear interpolation between 12-TET frequency and maqam frequency: `freq = 12TETfreq + gravity × (maqamFreq - 12TETfreq)`. Because the interpolation is on frequency (not cents), the actual cent deviation varies slightly across the keyboard — a fundamental property of equal temperament's exponential frequency spacing.

### 2.5 How Maqam Interacts with GENDY

The maqam tuning system operates at the pitch level — it sets the frequency of each voice's waveform cycle. GENDY operates at the waveform level — it shapes the timbre of that frequency. They are orthogonal dimensions: the maqam determines *what pitch* is produced, GENDY determines *what sound texture* is produced at that pitch.

However, at high drift settings, an interesting phenomenon emerges: the GENDY evolution causes the effective timbre to change so rapidly that the pitch becomes ambiguous. The note may be tuned to Bayati's characteristic quarter-tone, but if the waveform is evolving explosively, the harmonic content shifts so fast that the fundamental's presence is obscured. This creates a productive tension between the two systems: maqam gravity pulls the pitch into a precise microtonal position, while GENDY evolution works to dissolve that precise position into texture.

At moderate settings (gravity 0.5–0.8, drift 0.3–0.5), the two systems coexist beautifully — a pitched tone with identifiable maqam character and living, evolving timbre. At extremes (gravity 1.0, drift 0.8+), the maqam tuning is precise but the waveform makes it hard to hear; the result is a note that is *correctly* in Bayati but sounds like a stochastic texture that happens to occupy that pitch area.

Glide (`oracle_glide`) in maqam mode slides through maqam-correct pitch intervals, which is particularly effective for simulating oud or violin portamento within a maqam context.

---

## Part 3: Parameter Reference

### 3.1 Core GENDY Parameters

| Parameter ID | Name | Range | Default | Musical Function |
|---|---|---|---|---|
| `oracle_breakpoints` | Breakpoint Count | 8–32 (stepped) | 16 | Number of GENDY control points per waveform cycle. Low (8–12) = simple, clear waveforms with identifiable harmonics. High (24–32) = complex, dense, noise-adjacent. PROPHECY macro increases this toward complexity. |
| `oracle_timeStep` | Time Step Size | 0.0–1.0 | 0.30 | Maximum random walk step for breakpoint time offsets. Low = breakpoints maintain their relative positions (stable waveform shape). High = breakpoints migrate across the cycle (rhythmic instability). Scaled internally as step² × 0.15. |
| `oracle_ampStep` | Amplitude Step Size | 0.0–1.0 | 0.30 | Maximum random walk step for breakpoint amplitudes. Low = amplitudes drift gently. High = amplitudes jump dramatically (timbral convulsion). Scaled internally as step² × 0.3. This is the primary driver of timbral change. |
| `oracle_distribution` | Distribution Morph | 0.0–1.0 | 0.50 | 0 = pure Cauchy (heavy-tailed, occasional extreme jumps). 1 = pure Logistic (smooth, bounded). The single most important character control. PROPHECY macro shifts toward Cauchy. |
| `oracle_barrierElasticity` | Barrier Elasticity | 0.0–1.0 | 0.50 | Mirror barrier behavior. 0 = hard clamp (breakpoints stick at boundaries, creating dense concentration near extremes). 1 = full elastic bounce (energy preserved across reflections, breakpoints distribute more freely). |
| `oracle_drift` | Stochastic Drift | 0.0–1.0 | 0.30 | Master evolution intensity. Multiplied with stochastic envelope and step parameters to determine actual breakpoint walk magnitude per cycle. 0 = frozen waveform. 1 = maximum evolution. Aftertouch adds up to 0.15 on top. |

### 3.2 Maqam Parameters

| Parameter ID | Name | Range | Default | Musical Function |
|---|---|---|---|---|
| `oracle_maqam` | Maqam Mode | 0–8 (choice) | 0 (12-TET) | Selects the tuning system. 0=12-TET (standard), 1=Rast, 2=Bayati, 3=Saba, 4=Hijaz, 5=Sikah, 6=Nahawand, 7=Kurd, 8=Ajam. |
| `oracle_gravity` | Maqam Gravity | 0.0–1.0 | 0.00 | Blend between 12-TET (0) and full maqam intonation (1). 0 = standard equal temperament regardless of maqam selection. 1 = quarter-tone intervals reproduced faithfully. GRAVITY macro increases this. |

### 3.3 Amplitude Envelope Parameters

| Parameter ID | Name | Range | Default | Notes |
|---|---|---|---|---|
| `oracle_ampAttack` | Amp Attack | 0–10s | 0.01s | Linear attack. 0s = immediate onset. Long attacks create gradual fade-in of the stochastic texture. |
| `oracle_ampDecay` | Amp Decay | 0–10s | 0.10s | Exponential decay toward sustain. |
| `oracle_ampSustain` | Amp Sustain | 0–1 | 0.80 | Sustain level as fraction of peak. |
| `oracle_ampRelease` | Amp Release | 0–20s | 0.30s | Exponential release. Long releases allow the stochastic waveform to evolve through its release tail. |

### 3.4 Stochastic Envelope Parameters

The stochastic envelope is Oracle's unique second ADSR — it controls the *intensity of waveform evolution* over the note's lifetime, independent of the amplitude envelope.

| Parameter ID | Name | Range | Default | Notes |
|---|---|---|---|---|
| `oracle_stochEnvAttack` | Stoch Attack | 0–10s | 0.05s | How quickly the stochastic evolution reaches full intensity after note-on. Long stoch attack = note starts frozen, gradually comes alive. |
| `oracle_stochEnvDecay` | Stoch Decay | 0–10s | 0.30s | Decay toward sustain evolution level. |
| `oracle_stochEnvSustain` | Stoch Sustain | 0–1 | 0.70 | Sustained evolution intensity (multiplied with oracle_drift). 0 = evolution stops during sustain. 1 = full evolution throughout. |
| `oracle_stochEnvRelease` | Stoch Release | 0–20s | 0.50s | How quickly evolution slows after note-off. Long stoch release = waveform continues evolving through the amplitude release tail. |

**The stochastic depth formula:**
```
stochasticDepth = stochEnvelope.level × oracle_drift
actualTimeStep  = oracle_timeStep × stochasticDepth
actualAmpStep   = oracle_ampStep  × stochasticDepth
```

This means that `oracle_drift = 0` overrides the envelope — no evolution regardless of envelope settings. And `stochEnvSustain = 0` creates an evolution envelope that fades to frozen after the decay, regardless of `oracle_drift`.

### 3.5 LFO Parameters

Oracle has two LFOs. LFO1 modulates the time step (horizontal breakpoint drift); LFO2 modulates the amplitude step (vertical breakpoint drift). Modulation depth is additive, scaled by 0.3 to keep influence musical.

| Parameter ID | Name | Range | Default | Notes |
|---|---|---|---|---|
| `oracle_lfo1Rate` | LFO1 Rate | 0.01–30Hz | 1.0Hz | Rate range includes sub-audio for very slow modulation (0.01Hz = one cycle per 100 seconds). |
| `oracle_lfo1Depth` | LFO1 Depth | 0–1 | 0.00 | LFO1 modulation depth on time step. Default 0 = LFO inactive. |
| `oracle_lfo1Shape` | LFO1 Shape | 0–4 (choice) | Sine | Sine, Triangle, Saw, Square, S&H (Sample-and-Hold). S&H creates stepped random time-step modulation. |
| `oracle_lfo2Rate` | LFO2 Rate | 0.01–30Hz | 1.0Hz | Independent rate from LFO1. |
| `oracle_lfo2Depth` | LFO2 Depth | 0–1 | 0.00 | LFO2 modulation depth on amplitude step. Default 0 = LFO inactive. |
| `oracle_lfo2Shape` | LFO2 Shape | 0–4 (choice) | Sine | Same shapes as LFO1. |

**Practical LFO interactions:**
- LFO1 at slow rate (0.05–0.2Hz) on time step = breathing temporal instability — the waveform's rhythmic structure pulses in and out
- LFO2 at fast rate (4–8Hz) on amplitude step = rapid timbral oscillation — a kind of stochastic vibrato in the harmonic content
- S&H on LFO1 at 1–3Hz = stepped random time evolution — quantized bursts of horizontal instability
- Both LFOs at slightly different rates = complex beating pattern in the evolution intensity

### 3.6 Voice and Performance Parameters

| Parameter ID | Name | Range | Default | Notes |
|---|---|---|---|---|
| `oracle_voiceMode` | Voice Mode | Mono/Legato/Poly4/Poly8 | Poly4 | Mono/Legato for taqsim improvisation. Poly4 for chordal textures. Poly8 for full orchestral density. In legato mode, the stochastic evolution continues uninterrupted through note changes — the waveform "carries" from note to note. |
| `oracle_glide` | Glide Time | 0–2s | 0s | Portamento. In maqam mode, the glide slides through intermediate microtonal positions, creating the kind of portamento characteristic of oud and violin playing within maqam. |
| `oracle_level` | Master Level | 0–1 | 0.80 | Output gain. |

### 3.7 Macro Parameters

Oracle's macros are named for the engine's four experiential dimensions:

| Parameter ID | Macro Name | Primary Effect | Secondary Effect |
|---|---|---|---|
| `oracle_macroProphecy` | PROPHECY | Increases amplitude step size (+0.2) | Shifts distribution toward Cauchy (+0.3) — more dramatic random jumps |
| `oracle_macroEvolution` | EVOLUTION | Increases time step size (+0.4) | Increases amplitude step size (+0.3) — both dimensions of chaos accelerate |
| `oracle_macroGravity` | GRAVITY | Increases maqam gravity (+0.5) | — Pulls tuning strongly toward selected maqam |
| `oracle_macroDrift` | DRIFT | Increases drift amount (+0.3) | Increases time step (+0.2) — global evolution acceleration |

Macro contributions are additive to the base parameter values and clamped to [0, 1]. This means macros at maximum can push parameters to their limits regardless of where the base knobs sit — use this for performance automation.

---

## Part 4: Sound Design Recipes

### Recipe 1: Geological Drone

**Concept:** A single sustained tone that evolves imperceptibly slowly, shifting over minutes rather than seconds. Suitable as a long-form ambient drone or an underlying texture for other engines.

**Settings:**
```
oracle_breakpoints:       12
oracle_timeStep:           0.05
oracle_ampStep:            0.08
oracle_distribution:       0.9  (mostly Logistic — smooth, bounded)
oracle_barrierElasticity:  0.8  (elastic barriers — breakpoints bounce freely)
oracle_maqam:              1    (Rast)
oracle_gravity:            0.7  (clear maqam intonation but some chromatic allowance)
oracle_drift:              0.08 (extremely slow evolution)
oracle_ampAttack:          3.0s (long fade-in creates emergence from silence)
oracle_ampSustain:         1.0
oracle_ampRelease:         8.0s (long release lets the waveform evolve through its tail)
oracle_stochEnvAttack:     10.0s (stochastic intensity builds very slowly)
oracle_stochEnvSustain:    0.5
oracle_voiceMode:          Mono
oracle_lfo1Rate:           0.02Hz
oracle_lfo1Depth:          0.4
oracle_lfo1Shape:          Sine  (very slow time-step breathing)
```

**How to play:** Hold a low note (C2 or D2 for Rast, which roots on D). Let the note sustain for 30–60 seconds before evaluating the sound — the stochastic envelope and LFO need time to work. The drone will slowly shift character while remaining anchored to Rast's neutral tonal quality.

**Why it works:** Low breakpoint count keeps the waveform simple and pitched. Near-Logistic distribution ensures gradual, smooth evolution without sudden jumps. The very slow LFO creates a breathing motion in the temporal dimension that is felt more than heard. Maqam gravity creates microtonal character without pulling the note away from its intended pitch.

---

### Recipe 2: Taqsim Lead — Maqam Bayati

**Concept:** A melodic lead voice suitable for Arabic or Turkish-influenced improvisation. Reproduces the characteristic sound of a ney flute or oud in the middle register, with the waveform's organic evolution creating natural expression.

**Settings:**
```
oracle_breakpoints:       16
oracle_timeStep:           0.12
oracle_ampStep:            0.18
oracle_distribution:       0.65 (Logistic-biased — smooth, organic)
oracle_barrierElasticity:  0.6
oracle_maqam:              2    (Bayati)
oracle_gravity:            0.85 (strong maqam intonation)
oracle_drift:              0.35
oracle_ampAttack:          0.05s
oracle_ampDecay:           0.2s
oracle_ampSustain:         0.85
oracle_ampRelease:         0.4s
oracle_stochEnvAttack:     0.15s
oracle_stochEnvSustain:    0.75
oracle_voiceMode:          Legato
oracle_glide:              0.12s (short glide for maqam portamento)
oracle_lfo2Rate:           0.8Hz
oracle_lfo2Depth:          0.15
oracle_lfo2Shape:          Sine  (gentle amplitude-step vibrato)
```

**How to play:** Use in Legato mode for continuous melodic lines. Begin phrases on D (Bayati's root). The characteristic note in Bayati is the E♭† — a three-quarter-tone step from the root. Play D → E♭† slowly and let the glide carry through the microtonal interval. The waveform's evolution will create subtle timbral inflection within each sustained note that mimics the breath control of a wind player.

For more maqam authenticity: start phrases in the lower tetrachord (D to G), establish the tonic, then ascend to the upper register (A to D an octave up) and return. Bayati's emotional weight is concentrated in the lower tetrachord's characteristic neutral intervals.

**Why it works:** Moderate drift and Logistic distribution create the organic, breathing quality of acoustic instruments without introducing chaotic discontinuities. High maqam gravity ensures the quarter-tone intervals are rendered faithfully. Short glide simulates the continuous pitch slides characteristic of maqam performance on fretless instruments. LFO2 on amplitude step creates a subtle timbre-vibrato that enriches sustained notes.

---

### Recipe 3: Cauchy Prophecy — Stochastic Texture

**Concept:** An aggressive, unpredictable texture using heavy Cauchy distribution. Based on the sound world of Xenakis's actual GENDY3 compositions — noise-adjacent, metallic, and alive with sudden dramatic events.

**Settings:**
```
oracle_breakpoints:       24
oracle_timeStep:           0.45
oracle_ampStep:            0.55
oracle_distribution:       0.05 (heavily Cauchy — maximum heavy-tail behavior)
oracle_barrierElasticity:  0.25 (mostly hard barriers — breakpoints concentrate at extremes)
oracle_maqam:              3    (Hijaz — the augmented second creates additional drama)
oracle_gravity:            0.4  (some maqam influence but not overwhelming)
oracle_drift:              0.75 (aggressive evolution)
oracle_ampAttack:          0.001s (immediate onset)
oracle_ampSustain:         1.0
oracle_ampRelease:         0.8s
oracle_stochEnvAttack:     0.02s (evolution begins immediately)
oracle_stochEnvSustain:    1.0   (full evolution throughout)
oracle_voiceMode:          Poly4
oracle_lfo1Rate:           0.3Hz
oracle_lfo1Depth:          0.5
oracle_lfo1Shape:          S&H  (random steps in time evolution — quantized bursts)
oracle_lfo2Rate:           0.5Hz
oracle_lfo2Depth:          0.45
oracle_lfo2Shape:          S&H  (random steps in amplitude evolution)
```

**How to play:** Play chords. In Poly4, each voice evolves independently from its own PRNG seed, creating a four-way stochastic texture where different voices make different dramatic jumps at different moments. Play notes in the lower-middle register (C3–C4) where the Cauchy jumps are dramatic but the pitch is still perceptible. Hold notes for 5–15 seconds to experience the full range of Cauchy behavior — long periods of near-stability punctuated by sudden violent character shifts.

For maximum intensity: push `oracle_macroProphecy` to 0.8 during performance. This shifts distribution further toward Cauchy and increases amplitude step, creating a controlled escalation from "agitated" to "prophetic convulsion."

**Why it works:** Cauchy's heavy tail ensures the dramatic events that characterize Xenakis's original GENDY works. High breakpoint count creates dense harmonic content. Hard barriers concentrate breakpoints near amplitude extremes, adding saturation-like edge. S&H LFOs create quantized, stepped evolution in the meta-parameters — the rate of change itself changes stepwise, adding a second layer of stochastic structure.

---

### Recipe 4: Zaar Ceremony — Maximum Maqam Intensity

**Concept:** Named for Halim El-Dabh's 1944 wire recorder manipulations of a Cairo zaar ceremony — the reference ghost in Oracle's design documentation. A ritual texture of maximum stochastic intensity within the tight constraints of Maqam Saba.

**Settings:**
```
oracle_breakpoints:       28
oracle_timeStep:           0.38
oracle_ampStep:            0.48
oracle_distribution:       0.15 (heavy Cauchy — frequent dramatic jumps)
oracle_barrierElasticity:  0.3  (mostly hard barriers)
oracle_maqam:              3    (Saba — dark, diminished character)
oracle_gravity:            0.95 (near-maximum maqam fidelity)
oracle_drift:              0.80 (very high evolution)
oracle_ampAttack:          0.8s (gradual emergence)
oracle_ampSustain:         1.0
oracle_ampRelease:         3.0s
oracle_stochEnvAttack:     0.5s
oracle_stochEnvSustain:    1.0
oracle_voiceMode:          Mono
oracle_glide:              0.3s (slow glide creates ceremonial processional feel)
oracle_lfo1Rate:           0.07Hz
oracle_lfo1Depth:          0.6  (very slow time-step breathing — approximately one breath per 14 seconds)
oracle_lfo2Rate:           0.15Hz
oracle_lfo2Depth:          0.5  (slow amplitude-step cycle)
```

**How to play:** Single-note melodic movement, slowly. Saba's characteristic diminished fourth (400 cents rather than the expected 500 for a perfect fourth) creates a constricted, compressed feeling in the lower tetrachord. Play D → E♭† → F → G♭ (Saba's diminished fourth) very slowly, allowing the glide to travel through the microtonal intervals. The stochastic evolution within each note creates the sense of internal agitation within the ritual structure. The very slow LFOs create large-scale breathing across the texture.

**Why it works:** Saba's dark diminished quality combined with maximum GENDY evolution creates a sound that is simultaneously ancient (the maqam's centuries of ritual association) and unprecedented (no acoustic instrument produces stochastic waveforms this way). The near-maximum gravity (0.95) maintains taut maqam correctness even as the waveform convulses — the pitch is pinned to Saba's intervals while the timbre works to escape them. This is the productive tension at Oracle's core.

---

### Recipe 5: Maqam Chorus — Polyphonic Texture in Sikah

**Concept:** A chordal texture using Maqam Sikah, which has a quarter-tone root (E♭†). The impossibility of playing this maqam on a standard piano becomes an advantage — Oracle's gravity system allows the full chord to inhabit microtonal space impossible for fixed-pitch instruments.

**Settings:**
```
oracle_breakpoints:       14
oracle_timeStep:           0.2
oracle_ampStep:            0.22
oracle_distribution:       0.7  (Logistic-biased — gradual, layered)
oracle_barrierElasticity:  0.7  (elastic — breakpoints flow freely)
oracle_maqam:              5    (Sikah — quarter-tone root, ethereal character)
oracle_gravity:            0.9  (strong microtonal pull)
oracle_drift:              0.4
oracle_ampAttack:          1.5s (slow fade-in — chords emerge from silence)
oracle_ampDecay:           0.5s
oracle_ampSustain:         0.9
oracle_ampRelease:         4.0s (long release — chords dissolve slowly)
oracle_stochEnvAttack:     1.0s (evolution builds slowly within each chord)
oracle_stochEnvSustain:    0.6
oracle_voiceMode:          Poly8 (full polyphony)
oracle_lfo1Rate:           0.04Hz
oracle_lfo1Depth:          0.3   (very slow time-step modulation — the chord breathes over ~25 seconds)
oracle_lfo2Rate:           0.06Hz
oracle_lfo2Depth:          0.25
```

**How to play:** Play chords slowly and let them sustain. In Poly8 with Sikah's quarter-tone root, each note in the chord will be microtonally offset from 12-TET, creating a dense microtonal harmonic field that is acoustically unusual — no two pitches are in simple integer ratios to each other, which produces a rich, shimmering beating between partials. Try playing parallel thirds and fifths; the maqam's non-equal intervals create constantly shifting harmonic relationships.

For maximum effect: use with reverb (Oracle's dry output benefits from spatial processing). The long amplitude attack and release mean chords blend into each other, creating a continuous evolving harmonic cloud rather than distinct chord events.

**Why it works:** Sikah's quarter-tone root means that with gravity at 0.9, no note in the chord sits precisely on a 12-TET frequency. All eight voices drift through their stochastic evolution at different rates (different PRNG seeds, initialized from note number + voice counter). The slow LFOs create large-scale breathing across the full eight-voice texture. The result is a microtonal soundscape that could not be produced by any instrument using conventional tuning.

---

## Part 5: Coupling Guide

Oracle participates in XOmnibus's MegaCouplingMatrix as both a sender and receiver. Understanding the coupling interface enables cross-engine synthesis techniques not possible within any single engine.

### 5.1 What Oracle Exports

Oracle's `getSampleForCoupling()` method exposes three channels:

**Channel 0 — Left audio output:**
Oracle's processed stereo left channel, post-DC-blocker, post-limiter, post-envelope. Other engines can use this as an audio modulation source — feeding Oracle's stochastic waveform directly into another engine's frequency modulation, filter modulation, or waveform shaping.

**Channel 1 — Right audio output:**
Oracle's processed stereo right channel. Because the right channel reads the breakpoint constellation at a 1% phase offset from the left, channels 0 and 1 are slightly different signals — subtle stereo decorrelation that becomes more interesting when used as independent modulation sources.

**Channel 2 — Envelope follower:**
Oracle's peak amplitude envelope level (0.0–1.0). This is a smooth control signal representing Oracle's loudness at any moment — not sample-accurate audio but a slower-moving amplitude tracker. Other engines can use this for sidechain-style modulation, where Oracle's amplitude controls a parameter in a partner engine.

### 5.2 Coupling Inputs Oracle Accepts

**AudioToFM (`CouplingType::AudioToFM`):**
External audio perturbs Oracle's breakpoint amplitudes directly. The coupling signal is added to each breakpoint's amplitude walk at 0.05× scaling, ensuring external influence colors the stochastic evolution without overwhelming it. Connect ONSET (drum engine) via AudioToFM to create rhythmic disturbances in Oracle's waveform — the kick's transients will shake the reef's breakpoints, creating percussive modulation of the stochastic texture.

**AmpToFilter (`CouplingType::AmpToFilter`):**
External amplitude modulates Oracle's mirror barrier positions. When a coupling engine's amplitude is high, Oracle's barriers are pushed wider (breakpoints can occupy more of the amplitude space). When coupling amplitude is low, barriers narrow (breakpoints are constrained to a smaller range, creating a more tonal, pitched sound). This is an amplitude-controlled "wildness" gate — connect OBLONG or ODYSSEY's envelope output to create a coupling where another engine's musical dynamics control Oracle's timbral range.

**EnvToMorph (`CouplingType::EnvToMorph`):**
External envelope drives Oracle's Cauchy/Logistic distribution morph. Scaling is 0.4×. This allows another engine's envelope to dynamically shift Oracle's distribution character during performance. A long, slow attack from OVERDUB's reverb tail could smoothly shift Oracle from Cauchy to Logistic as the dub's reverb blooms — a kind of sympathetic calming.

### 5.3 Recommended Coupling Combinations

**ORACLE → OVERDUB (via AudioToFM):**
Route Oracle's stochastic audio output into Overdub's frequency modulation input. Overdub's tape delay smears the stochastic waveform, creating rhythmic echoes of stochastic events. The maqam-tuned pitches in Oracle's output become pitch-shifting delay grains in Overdub. Result: a dub production using a GENDY source instead of a conventional melody.

**ONSET → ORACLE (via AudioToFM):**
Route ONSET's kick and snare transients into Oracle's breakpoint perturbation. Each drum hit creates a brief disturbance in Oracle's stochastic evolution — the breakpoints shake on the beat, then return to their probabilistic drift between hits. Result: rhythmically structured stochastic texture where drum hits are "felt" through the waveform's behavior rather than heard as separate events.

**ORACLE → OPAL (via AudioToFM):**
Route Oracle's stochastic output into Opal's granular synthesis source input. Opal can granulate Oracle's evolving waveform, creating grain clouds from GENDY material. The stochastic texture at the grain level (Oracle's waveform) and the stochastic texture at the cloud level (Opal's grain distribution) compound, creating multi-scale randomness. Result: granular synthesis where every grain is itself a GENDY waveform.

**ORGANON → ORACLE (via EnvToMorph):**
Route Organon's metabolic envelope output into Oracle's distribution morph. Organon's variational free energy system produces irregular, organism-like envelope shapes — these will drive Oracle's Cauchy/Logistic blend in biologically irregular patterns. Result: Oracle's stochastic character breathes with the rhythms of a metabolic process.

---

## Appendix: The Historical Ghosts in Oracle

The design specification for Oracle names four historical precedents that the engine "remembers" — instruments and moments that are embedded in its concept:

**The UPIC (Iannis Xenakis, 1977):** The graphical tablet where composers drew waveforms by hand, which led directly to GENDY's automated waveform drawing.

**The Bazantar (Mark Deutsch, 2000):** A modified double bass with 29 sympathetic strings tuned to Indian raga intervals — acoustic proof that microtonal sympathetic resonance produces extraordinary results. Oracle's gravity system is the digital analog of the Bazantar's sympathetic string tuning.

**The Ondes Martenot Diffuseurs (Maurice Martenot, 1928):** The Ondes Martenot's three specialized speakers — including the "palme" (strings strung across the speaker cone) and "métallique" (speaker firing into a suspended gong) — added sympathetic resonance to the electronic signal. These physical resonant structures are the acoustic ancestor of Oracle's mirror barriers.

**Halim El-Dabh's Wire Recorder (1944):** Egyptian-American composer El-Dabh recorded a zaar ceremony in Cairo and manipulated the wire recording with filtering and speed changes — an act that predates Pierre Schaeffer's *Études de bruits* (credited as the "first" musique concrète) by four years. El-Dabh's obscured place in electronic music history reflects the field's Euro-American bias. Oracle's maqam system is dedicated to the principle that El-Dabh's cultural context and Xenakis's algorithmic formalism belong in the same instrument.

---

*Engine: ORACLE | XO_OX Designs | Blessing B010 — Buchla 10/10*
