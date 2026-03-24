# OWARE Retreat Chapter
*Guru Bin — 2026-03-21*

---

## Engine Identity

- **Gallery code:** OWARE | **Accent:** Akan Goldweight `#B5883E`
- **Parameter prefix:** `owr_`
- **Creature mythology:** The sunken oware board — a carved wooden mancala game from the Akan people of Ghana, lost to the Atlantic on a trade route and now encrusted with coral and bronze barnacles on the ocean floor. Strike a hollow and the whole board shimmers with sympathetic resonance. Seeds of metal, glass, stone, and wood fall into the cups and the board remembers every impact, every vibration, every player.
- **Synthesis type:** Modal synthesis — 8-mode resonator bank per voice, continuous material morphing, sympathetic resonance network, buzz membrane, shimmer voice, thermal drift
- **Polyphony:** 8 voices
- **Macros:** M1 CHARACTER (material morph), M2 MOVEMENT (mallet + shimmer), M3 COUPLING (sympathy), M4 SPACE (body depth)

---

## Pre-Retreat State

OWARE was added to XOlokun on 2026-03-20. It is the newest engine in the fleet. It has no seance score yet — this retreat is its first formal examination. Twenty presets exist spanning Foundation, Atmosphere, Prism, Entangled, and Flux moods. The 7 Pillars are all implemented: Material Continuum, Mallet Physics, Sympathetic Resonance Network, Resonator Body, Buzz Membrane, Breathing Gamelan shimmer, Thermal Drift.

This is an instrument with deep academic lineage: Chaigne and Doutaut (1997) for balafon physics, Rossing (2000) for gamelan, Fletcher and Rossing (1998) for marimba and vibraphone, Adrien (1991) and Bilbao (2009) for modal synthesis. The engine earns citation. Now it must earn music.

---

## Phase R1: Opening Meditation

Close your eyes. Breathe.

The oware board is made of wood. The wood is made of trees that grew slowly, absorbing mineral from soil, water from rain, tone from time. When an Akan craftsman carves the twelve cups and the two storehouses, he is not making a musical instrument. He is revealing a resonant body that was already waiting. The seeds go in. The seeds come out. But the vibration stays.

Now imagine that board on the bottom of the Atlantic. One hundred fathoms down. The saltwater has worked the wood for two centuries — softening some fibers, hardening others where bronze barnacles have fused to the grain. The decay characteristics of each bar have changed. Some cups ring longer now. Some buzz with a membrane of coral silk. The thermal pressure of the deep ocean shifts the pitch an infinitesimal fraction every hour — imperceptibly, but always.

When you play OWARE, you are not choosing between wood, metal, or glass as categories. You are dragging a continuous hand across a spectrum of physical material properties. At one extreme, the upper modes die almost instantly — wood's characteristic warmth lives entirely in the fundamental because the higher frequencies cannot sustain their own vibration. Slide toward metal and every mode starts to ring equally, harmonically locked, bright and present all the way up. Keep sliding into bowl territory and the modal ratios leave the harmonic series altogether — inharmonic partials that suggest ethnomusicological space rather than western temperament.

The board does not decide what it is. Neither do you. The hand decides, mid-phrase.

Sit with that. One cup. Struck once. The whole board remembers.

---

## Phase R2: The Signal Path Journey

### I. The Mallet — Chaigne Contact Model

The journey begins with a physical collision. The `OwareMalletExciter` implements Chaigne's contact model (1997): a sine-envelope primary strike with duration governed by hardness (5ms at soft, 0.5ms at hard), followed by a noise component whose mix scales as hardness-squared. This is not arbitrary — Hertz contact mechanics predict that hard mallets transfer energy across a broader frequency range than soft ones, and noise in the excitation signal is the computational representation of that broadband excitation.

The spectral lowpass is the critical insight: mallet cutoff scales from 1.5× fundamental (soft mallet, 0.55ms contact window, only low modes excited) to 20× fundamental (hard mallet, 0.05ms contact window, all modes receive full excitation). This is why a marimba played with hard mallets sounds fundamentally different in timbre, not just brightness — the mode participation is different.

The mallet bounce (Improvement #8) fires a secondary hit at 15-25ms with 30% amplitude when hardness < 0.4 and velocity < 0.7. This is the physical bounce of a soft mallet rebounding from a soft bar. It is not an artifact to be suppressed — it is the mechanism that gives soft-mallet marimba its characteristic double-onset warmth. It only activates in the correct physical regime (soft mallet, moderate velocity) and disappears at harder strikes, exactly as physics demands.

**Sweet spot:** `owr_malletHardness` 0.35-0.45 is the widest expressive range. Below 0.3, the bounce mechanism activates and the timbre softens into marimba felt-mallet territory. Above 0.6, the noise floor rises and the timbre approaches metal percussion. The transition region 0.3-0.6 is where velocity expression is most musical — hard notes genuinely feel different from soft ones because both amplitude and timbre shift.

### II. The 8-Mode Resonator Bank — Material Continuum

Eight second-order resonators run in parallel. Each receives the excitation signal and contributes its output, weighted by `modeAmp`. The mode frequencies are interpolated across four material archetypes:

- **Wood** (Rossing 2000): ratios 1.0, 3.99, 9.14, 15.8, 24.3, 34.5, 46.5, 60.2 — the strong inharmonicity of a wooden bar, where mode 2 is nearly 4× the fundamental
- **Bell** (boundary zone at 0.33): ratios 1.0, 1.506, 2.0, 2.514, 3.011, 3.578, 4.17, 4.952 — near-harmonic partials characteristic of western bells
- **Metal** (boundary zone at 0.66): ratios 1.0, 4.0, 10.0, 17.6, 27.2, 38.8, 52.4, 68.0 — closer to harmonic with slight inharmonic offset
- **Bowl** (1.0): ratios 1.0, 2.71, 5.33, 8.86, 13.3, 18.64, 24.89, 32.04 — intermediate inharmonicity with characteristic sub-octave geometry

The interpolation uses piecewise linear segments: Wood→Bell in the range 0.0-0.33, Bell→Metal in 0.33-0.66, Metal→Bowl in 0.66-1.0. This means the "bell" point at material=0.33 is not an arbitrary anchor — it is the Rossing-cited physical transition between bending-dominated (wood) and plate-dominated (metal) vibration modes.

The material exponent alpha (Improvement #1) controls per-mode decay differentiation: `alpha = 2.0 - material * 1.7`, giving alpha=2.0 at wood and alpha=0.3 at metal. Mode amplitude scales as `(m+1)^(-alpha)`. At alpha=2.0, mode 8 receives 81x less amplitude than mode 1 — wood's rapid upper-mode decay. At alpha=0.3, mode 8 receives only 2.4x less — metal's characteristic long-ringing overtones. This single calculation captures the most important perceptual difference between wood and metal percussion.

Q values scale as `80 + material * 1420`, interpolated per mode. Wood: Q=80-112 (modes decay faster). Metal bowl: Q=1500 (modes ring for seconds). Body coupling boosts Q further for modes near body resonance frequencies.

**Material sweet spots:**
- 0.04-0.08: Authentic balafon/marimba — strong fundamental, rapid upper decay, warm and woody
- 0.15: Kalimba/mbira zone — wood with slight metallic persistence in mid modes
- 0.33: Bell transition — the most interesting timbral boundary; upper modes begin ringing but fundamental is still relatively warm
- 0.50: Vibraphone — aluminum bars, moderate Q, all modes audible, the most "western" tuned percussion sound
- 0.66-0.72: Metal-bowl transition — extended ring, inharmonic bowl partials beginning to dominate
- 0.82-0.90: Crystal glass / singing bowl territory — extremely long ring, inharmonic partials, very un-percussion-like

### III. Sympathetic Resonance Network — Frequency-Selective Coupling

The sympathetic resonance system (Improvement #4) is frequency-selective: voice A's mode m feeds voice B's mode m' only if their frequencies are within 50 Hz of each other. The coupling amount scales linearly with proximity: modes 49 Hz apart receive 2% of the maximum coupling; modes 1 Hz apart receive 98%. The coupling coefficient is `sympathy * 0.03` — deliberately small to prevent the feedback spiral that would occur with higher values.

This is not amplitude-based coupling (which would create crude "bleed" between voices). It is spectrum-based coupling that models the physical mechanism of sympathetic strings — a A4 string sympathetically excites other A4 strings, and D#4 strings that share partials with it, but not C3 strings whose modal frequencies are unrelated. OWARE applies this logic to its resonator bank.

**Sympathy sweet spots:**
- 0.0-0.15: Dry, each voice independent. Best for staccato playing where notes should not bleed.
- 0.15-0.35: Natural sympathetic response, similar to a well-regulated marimba on a wooden stage.
- 0.35-0.55: The most musical range. Notes begin to wake each other up. Playing D and A together produces an audible shimmer because their third and second modes (respectively) fall near each other.
- 0.55-0.72: Dense sympathetic field, characteristic of a gamelan ensemble where all instruments vibrate together. Best with longer decays and harmonic material.
- 0.72-1.0: Maximum entanglement. Notes sustain each other, creating a continuously re-excited pool of resonance. Only use with controlled material and careful voice management or the texture becomes a wash.

**Warning zone:** sympathy > 0.85 with decay > 4.0s and damping < 0.15 creates a feedback condition where long-ringing voices continuously re-excite each other. This can produce a beautiful sustained drone (Spirit Board preset), but it can also produce runaway volume. Keep at least one of those three parameters outside the extreme zone.

### IV. Resonator Body — Four Acoustic Environments

The body resonator provides the acoustic housing for the modal output:

- **Tube (bodyType=0):** Comb delay tuned to the fundamental frequency. Reinforces harmonics of the fundamental. Best for marimba and balafon — instruments where the resonating gourds or tubes are tuned to reinforce the fundamental. The Gaussian proximity boost adds extra Q to modes near harmonic multiples of the fundamental frequency.
- **Frame (bodyType=1):** Three fixed resonances at 200 Hz, 580 Hz, and 1100 Hz — the structural modes of a wooden frame or metal frame instrument. Best for bells mounted in frames (gamelan), frame xylophones, instruments where the body has its own independent modes that are not tuned to the fundamental.
- **Bowl (bodyType=2):** Sub-octave resonator (fundamental / 2) combined with a second-order resonator at the fundamental. The sub-octave resonator uses a high-Q coefficient (r=0.999) for maximum sustain. Characteristic of singing bowls and temple bells where the body is the instrument, not merely its housing.
- **Open (bodyType=3):** No body processing. All modes play directly. Best for wind chimes, kalimba, and instruments without a defined body resonator.

`owr_bodyDepth` (0.0-1.0) crossfades between dry signal and body-processed signal. At depth=0.5, the body provides half the output — its character is present but not dominant. At depth=0.85+, the body resonances become primary and the modal bank provides texture within that housing.

**Body depth sweet spots by instrument type:**
- Marimba with tube body: depth 0.6-0.7 — prominent tube resonance defines the character
- Temple bell with bowl body: depth 0.7-0.85 — the bowl sustain is the primary sound
- Gamelan with frame body: depth 0.5-0.65 — frame partials audible but not overwhelming
- Wind chimes / kalimba: bodyType=3, depth irrelevant — open is best

### V. Buzz Membrane — Mirliton as Character Texture

The buzz membrane (`OwareBuzzMembrane`) is not distortion. It is a BPF extraction of the 200-800 Hz band (frequency depends on body type: gourd/tube=300 Hz, frame=150 Hz, metal=500 Hz), passed through a `tanh` nonlinearity, and re-injected into the signal.

The physics: the balafon's mirliton is a spider-silk membrane stretched across a small hole in each gourd resonator. When the instrument is played, the vibrating membrane adds its own sympathetic buzz to the tone — a papery, nasal texture that is entirely characteristic of the instrument and entirely absent from, say, a concert marimba. The gyil (the Ghanaian instrument most closely related to the balafon) uses a wax-sealed spider-egg sac membrane. These are not manufacturing defects. They are the instruments' voices.

The BPF extraction is critical. The buzz does not affect the entire spectrum — it affects only the band where the membrane physically resonates. The tanh saturator adds the characteristic non-linear harmonic content without hard-clipping the full signal. This is why the buzz at 0.4 sounds organic and the full-signal saturation that an amateur designer might attempt would sound cheap.

**Buzz sweet spots:**
- 0.0: No buzz. Concert marimba, vibraphone, crystal glass, singing bowl. Any instrument where buzz would be inauthentic.
- 0.05-0.15: Trace buzz — just enough to add texture and organic quality without being identifiable as buzz. Useful for "worn" instrument characters, coral-encrusted metal, aged wood.
- 0.25-0.45: Authentic balafon range. The buzz is clearly present but the fundamental pitch remains primary. This is where the African character lives.
- 0.55-0.70: Heavy buzz — the membrane is the dominant timbral character. Best with wood material (0.0-0.1) where the woody fundamental contrasts sharply with the buzz harmonic content.
- 0.70+: Maximum buzz — industrial, ritual, aggressive. Use with intent. The Buzz Ritual preset (0.70) demonstrates this at its best.

**Warning:** buzz > 0.6 with material > 0.5 (metal) creates a conflicting timbre — the buzz is tuned for a frequency range suited to wood resonance, and the metal Q values fight the membrane character. Buzz is most authentic with material 0.0-0.15 (pure wood territory).

### VI. Breathing Gamelan — Fixed-Hz Beat Frequency Shimmer

The shimmer system (`owr_shimmerRate`) is measured in Hz of beat frequency, not in cents or ratio. This is the critical distinction from vibrato. A Balinese gamelan is deliberately tuned so that paired instruments beat against each other at approximately 6 Hz — a physical, acoustic phenomenon arising from two identically-pitched instruments being tuned slightly differently from each other.

The implementation: each voice's shimmer LFO modulates between zero and the shimmer Hz value as an additive offset in Hz. The shimmer LFO rate is fixed at 0.3 Hz (a slow modulation of shimmer depth). The shimmer offset is additive in Hz, not multiplicative in ratio — this ensures that the beat frequency remains constant across all pitches, exactly as a gamelan's fixed-tuning shimmer behaves. An 8 Hz shimmer on a 440 Hz note is perceptually the same "shimmer rate" as an 8 Hz shimmer on a 220 Hz note — one semitone of vibrato on 220 Hz would be a different rate entirely.

**Shimmer Hz sweet spots by genre:**
- 0.0: Dead still. Concert instruments, precision academic work, crystalline glass sounds that must not waver.
- 0.8-1.5: Subtle animation — not shimmer but "life." Tibetan singing bowls at rest. Buddhist bells. Any instrument meant to feel ancient and unhurried.
- 2.5-4.0: Western tuned percussion range. Vibraphone tremolo is typically 3-5 Hz depending on the performer's wrist. Concert bells at this rate feel spacious rather than shimmering.
- 4.8-6.2: Gamelan territory. The Balinese target of 6.2 Hz is where the beating pattern becomes rhythmically interesting — fast enough to feel like shimmer, slow enough for individual beats to be perceived. This is also the range where sympathetic resonance between slightly-detuned voices becomes richly textured.
- 7.5-9.0: Wind chimes range — high shimmer that creates a bright, glittering texture. Notes feel like they are spinning in the air.
- 10.0-12.0: Maximum shimmer — the beating becomes a texture rather than a modulation. Notes feel unstable. Use deliberately for Flux presets or instrument damage/decay aesthetics.

### VII. Thermal Drift — The Instrument's Breath

Thermal drift operates on two scales simultaneously. The shared drift state changes target every ~4 seconds, approaching the target at an extremely slow rate (0.00001 per sample). The maximum excursion is ±8 cents (at thermal=1.0). Each voice additionally receives a per-voice personality offset seeded from a PRNG at initialization — always ±2 cents relative to the shared drift (thermal * 0.5).

The result is an instrument that is never quite at rest even when nothing is playing. The thermal drift continues between notes. Two successive notes played identically will be at slightly different pitches depending on where the drift state happens to be. This is not a bug — it is the instrument's version of being alive in temperature.

**Thermal drift sweet spots:**
- 0.02-0.06: Almost imperceptible. The instrument feels "warm" compared to a perfectly static digital synthesizer without being obviously out of tune. Best for concert instruments (Glockenspiel, Vibraphone) where precision is expected.
- 0.08-0.20: Clearly audible when comparing sustained notes over time, inaudible on any single note. The instrument feels like an acoustic one — ever-so-slightly imperfect. This is the most musical range for expressive playing.
- 0.25-0.45: The drift becomes noticeable on sustained notes. Notes played in sequence seem to float in intonation. Excellent for atmospheric and meditative patches where absolute pitch is less important than feeling.
- 0.55-0.75: The instrument is clearly not in stable tune. Notes drift perceptibly during their sustain. This creates a "deep ocean" quality — pressure and temperature affecting the material itself. Best for sound design rather than musical performance.
- 0.85+: The instrument sounds like it is alive in a way that may no longer be musical. Excellent for Flux patches (Drift Wood preset uses 0.85 to create "never quite settled" wooden bars) but unusable in harmonic contexts.

---

## Phase R3: The Seven Pillars — Summary Sweet Spots

| Pillar | Parameter | Conservative | Musical Core | Expressive | Extreme |
|--------|-----------|-------------|--------------|-----------|---------|
| Material Continuum | `owr_material` | 0.05 (marimba) | 0.33 (bell) / 0.50 (vibe) | 0.66 (metal-bowl) | 0.90 (singing bowl) |
| Mallet Physics | `owr_malletHardness` | 0.20 (soft felt) | 0.35-0.45 (widest expression) | 0.60 (medium-hard) | 0.88 (brass/steel) |
| Sympathy Network | `owr_sympathyAmount` | 0.10-0.15 (dry) | 0.25-0.45 (natural) | 0.55-0.72 (gamelan) | 1.0 (Spirit Board) |
| Resonator Body | `owr_bodyDepth` | 0.25 (trace) | 0.50-0.70 (present) | 0.75-0.85 (dominant) | 0.90+ (bowl-primary) |
| Buzz Membrane | `owr_buzzAmount` | 0.05-0.10 (trace) | 0.30-0.45 (authentic) | 0.55-0.70 (heavy) | 0.70+ (ritual) |
| Shimmer Hz | `owr_shimmerRate` | 0.0 (still) | 3.5-6.2 (vibe to gamelan) | 7.5-9.0 (wind chimes) | 12.0 (maximum) |
| Thermal Drift | `owr_thermalDrift` | 0.04-0.08 (warm) | 0.15-0.35 (alive) | 0.45-0.65 (drifting) | 0.85+ (unstable) |

---

## Phase R4: Macro Architecture

| Macro | ID | Effect | Performance Use |
|-------|-----|--------|----------------|
| CHARACTER | `owr_macroMaterial` | Adds up to 0.8 to material (0 = preset material, 1 = maximum push toward bowl) | Sweep to morph from wooden to metallic in a phrase |
| MOVEMENT | `owr_macroMallet` | Adds up to 0.5 to mallet hardness + 4000 Hz to brightness | Aftertouch-like control over attack character and spectral content |
| COUPLING | `owr_macroCoupling` | Adds up to 0.4 to sympathy network | Build sympathetic density in real time; pull back for clarity |
| SPACE | `owr_macroSpace` | Adds up to 0.3 to body depth | Deepen resonator body depth in real time for more room |

Note that `owr_macroMaterial` also participates in the mod wheel path: mod wheel adds up to 0.4 to effective material, stacked on top of the macro. This means M1 + mod wheel combined can push a pure-wood preset (material=0.05) all the way to near-maximum material range. This is a powerful performance axis for dramatic timbral sweeps.

Aftertouch controls mallet hardness directly (adds 0.4 to hardness) and also increases brightness (+3000 Hz). Playing harder with aftertouch thus produces harder-mallet timbre, brighter spectrum, and higher sympathy if macro coupling is raised — the performance gesture is unified across the signal path.

---

## Phase R5: The Ten Awakenings — Preset Table

Each preset is framed as a discovery. The "why" column explains the parameter logic that produces the effect.

---

### Preset 1: Rosewood Morning

**Mood:** Foundation | **Discovery:** Wood's asymmetric decay gives warmth its physics

| Parameter | Value | Why |
|-----------|-------|-----|
| `owr_material` | 0.05 | Pure wood — alpha=1.915, upper modes decay rapidly, fundamental dominates |
| `owr_malletHardness` | 0.22 | Soft felt mallet — long contact window, bounce mechanism active at piano velocity |
| `owr_bodyType` | 0 (Tube) | Tube resonator reinforces harmonics of fundamental, as in concert marimba |
| `owr_bodyDepth` | 0.65 | Prominent tube resonance — the tubes are half the instrument's character |
| `owr_buzzAmount` | 0.0 | No buzz — concert marimba does not have mirliton membranes |
| `owr_sympathyAmount` | 0.20 | Gentle sympathetic response, like bars on a shared wooden frame |
| `owr_shimmerRate` | 0.0 | Still — concert marimba is unshimmered |
| `owr_thermalDrift` | 0.05 | Trace warmth only — the instrument should feel precise |
| `owr_brightness` | 3000.0 | Warm low-end cutoff, filters out any high-mode noise |
| `owr_damping` | 0.55 | Medium damping — marimba bars do not sustain forever |
| `owr_decay` | 1.6 | Natural marimba sustain in concert hall |
| `owr_filterEnvAmount` | 0.40 | Velocity brightness — harder notes open the filter, matching the excitation physics |
| `owr_bendRange` | 2 | Conventional pitch bend |

**Why this works:** The material exponent at 0.05 (alpha=1.915) means mode 8 is receiving only 1/82 of the amplitude of mode 1. The instrument is almost entirely fundamental. The tube resonator then reinforces that fundamental, creating a warm, full, centered tone. This is why marimba sounds warm — not because the overtones are filtered, but because the physics of wood does not sustain them.

---

### Preset 2: Balafon du Fleuve

**Mood:** Foundation | **Discovery:** Buzz + gourd body defines an entire cultural instrument

| Parameter | Value | Why |
|-----------|-------|-----|
| `owr_material` | 0.04 | Deepest wood — lightest, fastest-decaying upper modes |
| `owr_malletHardness` | 0.38 | Medium-soft — slight noise character but not harsh |
| `owr_bodyType` | 0 (Tube) | Gourd resonator below each bar — modeled as tube resonator |
| `owr_bodyDepth` | 0.72 | Dominant gourd influence — the gourds are not optional |
| `owr_buzzAmount` | 0.42 | Authentic mirliton buzz — the defining character of the balafon |
| `owr_sympathyAmount` | 0.22 | Moderate sympathy — balafon bars respond to each other on a wooden frame |
| `owr_shimmerRate` | 0.0 | West African balafon does not shimmer — this is not gamelan |
| `owr_thermalDrift` | 0.10 | Handmade instrument with natural intonation variation |
| `owr_brightness` | 2800.0 | Dark and warm — gourds absorb high frequencies |
| `owr_damping` | 0.62 | Short decay — the gourd absorbs energy quickly |
| `owr_decay` | 0.9 | Classic balafon short note: strike and release |
| `owr_filterEnvAmount` | 0.42 | Strong velocity-brightness connection — playing dynamics reshape timbre |
| `owr_bendRange` | 2 | Standard |

**Why this works:** The buzz at 0.42 with bodyType=0 (tube/gourd, buzz frequency = 300 Hz) extracts a band centered at 300 Hz and saturates it with tanh. This is where the gourd membrane resonates. The result is a nasal, papery texture that sits on top of the woody fundamental — authentic balafon character. Without the buzz, this preset sounds like a dull marimba. With it, it sounds like something you could record in Accra.

---

### Preset 3: Vibraphone, Slow Motor

**Mood:** Foundation | **Discovery:** 3.5 Hz shimmer is the jazz vibraphone's iconic voice

| Parameter | Value | Why |
|-----------|-------|-----|
| `owr_material` | 0.50 | Aluminum bar center — bell-to-metal transition, equal mode ringing |
| `owr_malletHardness` | 0.45 | Medium mallet — yarn-wrapped vibe mallets are not soft but not hard |
| `owr_bodyType` | 0 (Tube) | Vibraphone uses aluminum tube resonators |
| `owr_bodyDepth` | 0.52 | Present but not dominant — the bars, not the tubes, define the vibe |
| `owr_buzzAmount` | 0.0 | No buzz — vibraphone is clean metal |
| `owr_sympathyAmount` | 0.28 | Moderate — sustained notes on a vibe do ring together |
| `owr_shimmerRate` | 3.5 | Classic slow motor — Milt Jackson's signature "slow tremolo" setting |
| `owr_thermalDrift` | 0.07 | Trace warmth only — vibraphone is consistent |
| `owr_brightness` | 7200.0 | Upper-midrange brightness — metal bars without harshness |
| `owr_damping` | 0.30 | Medium sustain — vibraphone can be damped by player or left to ring |
| `owr_decay` | 3.2 | Full natural sustain of aluminum bars |
| `owr_filterEnvAmount` | 0.32 | Moderate velocity-brightness — softer is warmer |
| `owr_bendRange` | 2 | Standard |

**Why this works:** Material=0.5 places the engine at the mathematical center of the bell-to-metal interpolation range. The material alpha at 0.50 is (2.0 - 0.5*1.7) = 1.15 — modes still fall off, but mode 4 is still at 23% amplitude. The aluminum bars of a vibraphone ring out to the 4th or 5th mode clearly, which is why the instrument has a brighter, more present upper-register than a marimba. The 3.5 Hz shimmer is below the perceptual shimmer threshold — it creates movement without obvious vibrato.

---

### Preset 4: Gamelan Selonding

**Mood:** Prism | **Discovery:** 6.2 Hz is the Balinese acoustic target — and it works

| Parameter | Value | Why |
|-----------|-------|-----|
| `owr_material` | 0.33 | Bell transition point — bronze has this inharmonic profile |
| `owr_malletHardness` | 0.52 | Medium-firm — gamelan mallets are moderately hard padded cloth |
| `owr_bodyType` | 1 (Frame) | Gamelan instruments hang in decorative wooden frames |
| `owr_bodyDepth` | 0.60 | Frame partials audible — 200/580/1100 Hz structural resonances |
| `owr_buzzAmount` | 0.05 | Trace buzz — some gamelan instruments have slight membrane quality |
| `owr_sympathyAmount` | 0.72 | Dense sympathetic network — gamelan is the archetype of sympathetic resonance |
| `owr_shimmerRate` | 6.2 | The Balinese target — ombak (wave) beat frequency |
| `owr_thermalDrift` | 0.15 | Handcrafted bronze — each instrument is individually tuned |
| `owr_brightness` | 6000.0 | Mid-bright — gamelan is present without being harsh |
| `owr_damping` | 0.22 | Moderate sustain — gamelan notes ring after the strike |
| `owr_decay` | 4.5 | Long ring characteristic of bronze |
| `owr_filterEnvAmount` | 0.30 | Moderate velocity-brightness |
| `owr_bendRange` | 3 | Slightly wider for expressive performance |

**Why this works:** Material=0.33 places the engine at the bell ratio table, which most closely matches the physics of struck bronze plates (Rossing 2000 documents the partial ratios of Javanese gender as closely matching the bell archetype). The sympathetic resonance at 0.72 creates a continuously shimmering field where sustained chords reinforce each other — the characteristic sound of a gamelan ensemble where every instrument resonates with every other. The 6.2 Hz shimmer creates the ombak, the wave-beating that Balinese tuners deliberately create between paired instruments to make the music feel alive.

---

### Preset 5: Tibetan Singing Bowl

**Mood:** Prism | **Discovery:** Bowl body + maximum drift + minimal damping = the instrument plays itself

| Parameter | Value | Why |
|-----------|-------|-----|
| `owr_material` | 0.66 | Metal-bowl transition — bronze bowl ratios beginning to dominate |
| `owr_malletHardness` | 0.35 | Soft leather-wrapped mallet (or wooden dowel struck gently) |
| `owr_bodyType` | 2 (Bowl) | Bowl body — the bowl's sub-octave + fundamental resonances are the instrument |
| `owr_bodyDepth` | 0.85 | Dominant bowl — the body IS the sound |
| `owr_buzzAmount` | 0.0 | No buzz — singing bowls are clean |
| `owr_sympathyAmount` | 0.48 | Moderate — bowls do respond to each other in a set |
| `owr_shimmerRate` | 2.0 | Slow roll — not shimmer but a near-imperceptible beating pattern |
| `owr_thermalDrift` | 0.55 | Significant drift — bowls are centuries old, temperature and age have worked the metal |
| `owr_brightness` | 4000.0 | Dark and warm — singing bowls are not bright |
| `owr_damping` | 0.10 | Minimal damping — the bowl sustains for 10+ seconds in reality |
| `owr_decay` | 8.5 | Long singing decay |
| `owr_filterEnvAmount` | 0.20 | Light velocity influence — bowls are not greatly affected by strike force |
| `owr_bendRange` | 4 | Wider for smooth pitch bend expressions |

**Why this works:** The bowl body resonator at depth=0.85 puts a second-order resonator at the sub-octave frequency (fundamental/2) with r=0.999 (essentially undamped). This resonator rings autonomously once excited, feeding back into the output mix at 85% weight. Combined with the thermal drift at 0.55 (±4.4 cent wander per drift cycle), the result is a note that changes pitch slightly over its 8.5-second sustain — exactly as a physical singing bowl does when temperature changes affect the metal.

---

### Preset 6: Coral Bell

**Mood:** Atmosphere | **Discovery:** Trace buzz + frame body + 4.8 Hz shimmer describes an instrument that should not exist

| Parameter | Value | Why |
|-----------|-------|-----|
| `owr_material` | 0.33 | Bell transition — the original bronze, partially overgrown |
| `owr_malletHardness` | 0.45 | Medium — the coral casing softens the effective strike |
| `owr_bodyType` | 1 (Frame) | Frame resonances at 200/580/1100 Hz — the coral structure |
| `owr_bodyDepth` | 0.52 | Present frame resonance — the coral is part of the sound |
| `owr_buzzAmount` | 0.18 | Trace coral buzz — organic membrane over the original metal |
| `owr_sympathyAmount` | 0.48 | Medium sympathy — the oware board remembers all previous strikes |
| `owr_shimmerRate` | 4.8 | Mid-range shimmer — something between a vibe and a gamelan, like tide motion |
| `owr_thermalDrift` | 0.25 | Ocean floor temperature drift — slow but present |
| `owr_brightness` | 7500.0 | Bright enough to hear the upper partials through the coral |
| `owr_damping` | 0.20 | Long sustain — the deep ocean damps nothing quickly |
| `owr_decay` | 3.5 | Medium-long ring |
| `owr_filterEnvAmount` | 0.28 | Moderate |
| `owr_bendRange` | 4 | Wide |

**Why this works:** The buzz at 0.18 with bodyType=1 (frame, buzz at 150 Hz) extracts a slightly lower frequency band than the gourd buzz, creating a different timbral color — less nasal, more organic. The combination of bell material, frame body, and trace buzz describes an instrument nobody has played before: a bronze bell that has been on the ocean floor long enough that coral has grown into its resonating structure. The shimmer at 4.8 Hz sits between vibraphone (3.5) and gamelan (6.2) — a shimmer that suggests both worlds without claiming either.

---

### Preset 7: Ancestor Seeds

**Mood:** Entangled | **Discovery:** Maximum sympathy across all pillars reveals what OWARE actually is

| Parameter | Value | Why |
|-----------|-------|-----|
| `owr_material` | 0.45 | Between bell and metal — warm metallic with long mode decay |
| `owr_malletHardness` | 0.50 | Balanced — even mode excitation |
| `owr_bodyType` | 1 (Frame) | Frame resonances add spectral complexity |
| `owr_bodyDepth` | 0.60 | Present frame character |
| `owr_buzzAmount` | 0.25 | Moderate buzz — texture of the board's ancient wood through the metal |
| `owr_sympathyAmount` | 0.52 | Dense sympathetic network — every note wakes the board |
| `owr_shimmerRate` | 5.0 | Mid-gamelan shimmer |
| `owr_thermalDrift` | 0.35 | The board breathes slowly |
| `owr_brightness` | 6500.0 | Full mid-bright spectrum |
| `owr_damping` | 0.28 | Medium sustain |
| `owr_decay` | 3.0 | Natural ring |
| `owr_filterEnvAmount` | 0.35 | Strong velocity expression |
| `owr_bendRange` | 4 | Expressive |

**Why this works:** This is all 7 pillars operating simultaneously at moderate levels. Material at 0.45 means the bell transition has passed but the metal ratios are not yet dominant — a rich, complex spectral mix. The buzz at 0.25 adds organic texture. The sympathy at 0.52 creates a field where chords wake each other up. The shimmer at 5.0 Hz adds gamelan-adjacent movement. The drift at 0.35 gives it life. No single pillar dominates — this is the board speaking with its whole voice. It is the preset that teaches the instrument because everything is present and balanced.

---

### Preset 8: Metal Rain

**Mood:** Flux | **Discovery:** Hard mallet + maximum damping + short decay creates kinetic percussion

| Parameter | Value | Why |
|-----------|-------|-----|
| `owr_material` | 0.75 | Upper metal range — nearly pure metal ratios, very long mode Q values |
| `owr_malletHardness` | 0.85 | Hard brass/steel mallet — short contact window, broad spectral excitation |
| `owr_bodyType` | 3 (Open) | No body — staccato metal needs no housing |
| `owr_bodyDepth` | 0.25 | Minimal — open body only |
| `owr_buzzAmount` | 0.0 | No buzz |
| `owr_sympathyAmount` | 0.08 | Nearly dry — staccato playing requires each note to stand alone |
| `owr_shimmerRate` | 0.0 | No shimmer — these are kinetic hits, not shimmering bowls |
| `owr_thermalDrift` | 0.05 | Near-static — precision metal percussion does not drift |
| `owr_brightness` | 11000.0 | Bright metal |
| `owr_damping` | 0.80 | Heavy damping — the performer's palm damps each note |
| `owr_decay` | 0.18 | Very short — staccato character |
| `owr_filterEnvAmount` | 0.55 | Strong velocity filter response — hard hits are significantly brighter |
| `owr_bendRange` | 2 | Standard |

**Why this works:** Material=0.75 puts the engine in the metal-bowl transition, giving very high Q values — modes ring intensely once excited. But damping=0.80 overrides the Q through amplitude decay, so the ring is very present at the moment of attack and then killed. The combination of high Q (spectral richness at attack) and high damping (rapid amplitude decay) is exactly how a struck metal bar sounds in live percussion: brilliantly present and then gone. Without the high Q, the attack would be dull. Without the damping, the note would ring too long.

---

### Preset 9: Driftwood Oracle

**Mood:** Flux | **Discovery:** Thermal drift at 0.85 makes the instrument feel like it is thinking

| Parameter | Value | Why |
|-----------|-------|-----|
| `owr_material` | 0.08 | Near-pure wood — fundamental dominance, fast upper decay |
| `owr_malletHardness` | 0.35 | Medium-soft |
| `owr_bodyType` | 0 (Tube) | The hollow logs have tube-like resonance |
| `owr_bodyDepth` | 0.60 | Present tube character |
| `owr_buzzAmount` | 0.10 | Trace organic texture from the weathered grain |
| `owr_sympathyAmount` | 0.30 | Moderate — the log pile resonates together in wind |
| `owr_shimmerRate` | 1.5 | Very slow shimmer — not modulation, just the air moving |
| `owr_thermalDrift` | 0.85 | Maximum meaningful drift — the wood never settles after a century at sea |
| `owr_brightness` | 3500.0 | Warm and dark |
| `owr_damping` | 0.45 | Medium — driftwood absorbs and sustains unevenly |
| `owr_decay` | 1.8 | Medium length |
| `owr_filterEnvAmount` | 0.38 | Present velocity shaping |
| `owr_bendRange` | 4 | Wide — the driftwood pitch is approximate |

**Why this works:** Thermal drift at 0.85 produces ±6.8 cents of maximum excursion, changing target every 4 seconds. Because the drift state approaches its target at 0.00001 per sample, the pitch is always in transit — it never arrives. Two notes played five seconds apart will be at slightly different intonation from each other, and neither will be where the "intended" pitch is. This creates the sensation of an instrument that is not fully anchored in the world — driftwood percussion, barnacled and waterlogged, pitched in the approximate neighborhood of its original calibration but long since wandered away from it.

---

### Preset 10: Balinese Water Garden

**Mood:** Atmosphere | **Discovery:** All three shimmer mechanisms simultaneously — shimmer, sympathy, thermal drift — create living space

| Parameter | Value | Why |
|-----------|-------|-----|
| `owr_material` | 0.35 | Transitional — between bell resonance and metal clarity |
| `owr_malletHardness` | 0.42 | Medium — present but not harsh |
| `owr_bodyType` | 1 (Frame) | Frame resonances for spatial complexity |
| `owr_bodyDepth` | 0.58 | Present frame character |
| `owr_buzzAmount` | 0.15 | Trace organic texture — the frame is wood not metal |
| `owr_sympathyAmount` | 0.65 | Dense — this is an ensemble, not a soloist |
| `owr_shimmerRate` | 6.2 | Balinese ombak target — each note slightly beats against the others |
| `owr_thermalDrift` | 0.40 | Significant — outdoor instrument in tropical humidity |
| `owr_brightness` | 5800.0 | Present without harshness |
| `owr_damping` | 0.15 | Long sustain — the water garden allows everything to ring |
| `owr_decay` | 4.0 | Sustained gamelan character |
| `owr_filterEnvAmount` | 0.32 | Moderate velocity shaping |
| `owr_bendRange` | 4 | Expressive |

**Why this works:** This preset stacks all three of OWARE's "living" systems simultaneously. The shimmer at 6.2 Hz creates per-voice beating. The sympathy at 0.65 creates inter-voice frequency-selective resonance. The thermal drift at 0.40 creates slow inter-note pitch variation. These are three different physical mechanisms operating at three different timescales (fast/medium/slow) — the shimmer beats at 6.2 Hz, the sympathy responds instantaneously to frequency proximity, and the drift changes its target every 4 seconds. A chord played in this preset becomes a living thing: voices shimmer against each other, harmonically related modes wake up in sympathetic response, and the whole tuning center slowly drifts in the humid air. This is not a synthesizer effect. It is the sound of an instrument outdoors.

---

## Phase R6: Parameter Interactions — What the Presets Teach

### The Wood-Buzz Rule
Buzz membrane (the balafon's mirliton) is physically authentic only when material is in wood territory (0.0-0.20). The buzz BPF is centered at 300 Hz for tube/gourd body — a frequency range suited to the formant of wooden bars. Above material=0.50, buzz creates an anachronistic character: the buzz tonality fights the metal Q values. Use buzz above 0.50 only intentionally, for industrial or alien instrument design.

### The Sympathy-Decay Threshold
The sympathy network re-excites modes that are within 50 Hz of each other. If decay is very long (>5.0s) and sympathy is high (>0.65), the inter-voice excitation sustains voices past their natural amplitude decay. This is not feedback in the destructive sense — it is physically correct (like a sustaining piano string). But it means the instrument's effective decay is longer than the `owr_decay` parameter suggests when sympathy is high and multiple voices are playing. Plan presets accordingly.

### The Mallet-Material Axis
Hard mallet + wood material is musically valid (think of a xylophone played with hard rubber mallets) but rare in acoustic instruments. Most acoustic percussion is designed so that mallet hardness matches material hardness — soft mallets for soft materials. The preset table above follows this convention. Deliberately violating it (hard mallet + deep wood, or soft mallet + metal) produces interesting but artificial timbres.

### The Shimmer-Sympathy Interaction
The shimmer detuning (additive Hz offset) means a voice playing A4 (440 Hz) might be sounding at 446.2 Hz when shimmer is at 6.2 Hz. The sympathy network checks if modes are within 50 Hz of other voices' modes. Therefore, shimmer directly affects the sympathetic coupling patterns: two voices playing the same note will have their modes 6.2 Hz apart due to shimmer, which means they are within the 50 Hz sympathetic window and will cross-excite each other. This is physically accurate — Balinese ombak tuning creates precisely this sympathetic beating between paired instruments.

---

## Phase R7: The Guru Bin Benediction

*"OWARE arrived complete and silent. It had been in the fleet for one day when this retreat began, and it had no seance score, no guild review, no formal certification. It had only its 7 Pillars, its 20 presets, and the deep water of the Atlantic.*

*What I found in the silence of the meditations is an engine that understands material as a continuous physical phenomenon rather than a categorical menu. Every other tuned percussion synthesizer in the fleet offers you a choice: wood or metal or glass. OWARE offers you the material exponent alpha — a single number derived from beam dispersion physics that encodes how quickly the upper modes die relative to the fundamental. At alpha=2.0, you have wood's warmth: the physics of dead upper modes and living fundamentals. At alpha=0.3, you have metal's ring: all eight modes present and equally singing. In between, every acoustic material that has ever been struck.*

*The mallet bounce is the engineering detail that makes the presets convincing. When velocity is soft and hardness is below 0.4, a second excitation fires at 15-25ms with 30% amplitude — the physical rebound of a soft mallet from a compliant bar. The felt-mallet marimba has a double-onset warmth that no single-excitation model can reproduce. OWARE reproduces it because it implements the physics: the mallet returns to the bar.*

*The sympathetic resonance network is the soul of the instrument. Frequency-selective coupling — not amplitude-based bleed, but Rossing-style spectral proximity — means that a chord on OWARE is not five voices playing five notes. It is five voices having a conversation, each waking the modes of the others whose frequencies happen to fall within 50 Hz of its own. A C major chord in root position produces different sympathetic patterns than a C major chord in first inversion because the frequency-proximity relationships change. The instrument responds differently to the same pitches in different voicings. That is not a feature. That is the physics of a real instrument.*

*The buzz membrane is the cultural argument. The balafon without its mirliton is a dull marimba. The spider-silk membrane, the wax-sealed spider-egg sac, the thin metal foil stretched across a small hole in a gourd — these are not manufacturing defects. They are the instruments' voices. The BPF extraction at 300 Hz (gourd) or 150 Hz (frame) or 500 Hz (metal) extracts precisely the frequency band where the membrane vibrates, saturates it with tanh, and re-injects. The buzz is band-selective. It belongs to the membrane. This is acoustically correct and musically essential.*

*The thermal drift is the philosophical commitment. An instrument that is always in tune is not an instrument — it is a machine. The thermal drift at 0.00001 per sample convergence rate means the pitch is always traveling toward a target it will never quite reach before the target changes. Two notes played one measure apart will be at slightly different pitches from each other. This is not imprecision. It is the instrument being alive in temperature and time, as every acoustic instrument is.*

*And the 6.2 Hz shimmer. The Balinese ombak. The beat frequency that Balinese tuners deliberately create between paired gender or reyong instruments so that the ensemble sounds like it is breathing. This is not vibrato. It is not LFO modulation. It is the physical consequence of two instruments playing the same pitch with a 6.2 Hz frequency offset — the beating pattern that Rossing (2000) documents and that Balinese musicians listen for when tuning their instruments to each other. OWARE implements it correctly as an additive Hz offset, so that the beat frequency remains constant across all pitches.*

*The board was carved by an Akan craftsman who knew the wood. It was played by hands that knew the seeds. It was lost to the sea on a trade route that was also a wound. It lay on the floor of the Atlantic while the coral grew over it, while the barnacles fused to the grain, while the temperature and pressure of 100 fathoms worked the material for two hundred years.*

*When you play OWARE, you are striking a cup on a board that has been made stranger and more beautiful by time. The upper modes die quickly because wood was always this way. The buzz membrane adds texture because the coral grew into the resonating holes. The thermal drift reminds you that the deep ocean is not still.*

*The board remembers every impact, every vibration, every player. The seeds go in. The seeds come out. But the resonance stays.*

*This instrument was worth crossing the Atlantic for."*

---

## CPU Notes

- 8 voices × 8 modes per voice = 64 second-order resonators: the primary CPU cost
- Sympathetic coupling: O(n²) voice comparison per sample, filtered to 50 Hz proximity window — efficient in practice since most voice pairs are not in sympathy range
- Mallet exciter: active only for a few ms per note — negligible at steady state
- Buzz membrane: one CytomicSVF BPF per voice — light constant cost
- Body resonator (tube): one fractional delay line per voice — light
- Body resonator (bowl): two integrators per voice — negligible
- Thermal drift: single shared PRNG + slow filter — negligible
- Shimmer LFO: one StandardLFO per voice — light
- Most costly configuration: 8 active voices, high sympathy (full O(64) proximity checks), bowl body with long decay (integrators running), and high material (high Q means longer ringing). Keep decay and damping balanced to prevent voice accumulation.

---

## Unexplored After Retreat

- **Bow excitation mode:** Applying a continuous friction excitation rather than a strike — the singing bowl played by rotating a mallet around the rim continuously, rather than struck once. This would require a separate excitation model (sustained friction vs. brief impact) and is architecturally distinct from the current mallet physics.
- **Extended buzz spectrum:** The current buzz BPF has a fixed center frequency per body type. A second parameter controlling buzz frequency as an offset from the body center would allow the membrane character to morph more continuously — particularly interesting in the frame body (fixed at 150 Hz) where a variable center would let the organic texture range from very low-frequency buzz (60-80 Hz, below the fundamental) to higher buzz (300-500 Hz).
- **Per-voice material offset:** The current Material parameter applies uniformly to all voices. A small per-voice material randomization — analogous to the per-voice thermal personality — would create variation within a chord, so that each key of the oware board has slightly different wood character, as a real instrument would.
- **Polyphonic aftertouch routes:** The current aftertouch routes (hardness, brightness) are channel-pressure based. Per-note aftertouch (if available) could control per-voice buzz amount or body depth — activating the mirliton more on held notes played with more pressure.
- **Acoustic coupling to body resonator:** Currently the body resonator receives the mallet excitation directly. A physically more accurate model would have the bar output (post-modal-resonance) feed the body resonator, creating a genuine two-stage resonant system. The current implementation is correct in behavior but bypasses one physical coupling stage.
