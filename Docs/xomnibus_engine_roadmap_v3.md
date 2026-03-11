# XOmnibus — Engine Expansion Roadmap: Volume 3
*Next 5 Gallery Additions: OBSIDIAN, ORIGAMI, ORACLE, OBSCURA, OCEANIC*
*Document version: 1.0 | March 2026*

---

## Gallery State at Roadmap Start

### Integrated (Volume 0 — Original Gallery)

| Code | Source | Role | Status |
|------|--------|------|--------|
| SNAP | XOddCouple X | Percussive / rhythmic | Integrated |
| MORPH | XOddCouple O | Wavetable pads | Integrated |
| DUB | XOverdub | FX send / return bus | Integrated |
| DRIFT | XOdyssey | Psychedelic pads, Climax | Integrated |
| BOB | XOblongBob | Warm fuzzy textures | Integrated |
| FAT | XObese | Width / thickness, 13-osc | Integrated |
| ONSET | XOnset | Dedicated drums | Integrated |
| OVERWORLD | XOverworld | Chip synthesis, 6 engines | Integrated |

### Volume 1 — In Development

| Code | Source | Role | Status |
|------|--------|------|--------|
| BITE | XOpossum | Bass-forward character | Phase 0 complete |
| OPAL | XOpal | Granular time-scatter | Phase 0 complete |

### Volume 2 — Architecture Designed

| Code | Source | Role | Status |
|------|--------|------|--------|
| ORGANON | XOrganon | Metabolic / dissipative | Phase 1 architecture |
| OUROBOROS | XOuroboros | Chaotic attractor ODE | Phase 1 architecture |
| ORBITAL | XOrbital | Additive / psychoacoustic | Phase 1 architecture |

### Volume 2 — Expansion Wave (Moving Forward)

| Code | V2 Name | Core Concept | Status |
|------|---------|-------------|--------|
| — | XOscillum | Psychoacoustic phantom pitch | Design complete |
| — | XObliqua | Kinematic phase-time | Design complete |
| — | XOccult | Cellular automata | Design complete |
| — | XOblivion | Electromagnetic hysteresis | Design complete |
| — | XOntara | Topological sympathetic resonance | Design complete |

### Volume 2 — Parked (No Timeline)

| Name | Core Concept | Reason Parked |
|------|-------------|---------------|
| XOMEMBRA | 2D wave mesh polyrhythm | CPU cost, niche rhythmic role |
| XOGRAMA | 720-voice optical holography | Massive polyphony management unsolved |
| XOSMOSIS | 1D computational fluid dynamics | Hostile timbre, limited melodic range |

### Volume 2 — Absorbed / Killed

| Name | Fate |
|------|------|
| XOBOLIC | Killed — duplicate of ORGANON's metabolic territory |
| XOBSESSION | Absorbed — Mandelbrot/Julia/DIVE concepts folded into OUROBOROS v2 |

---

## Volume 3 — The Synthesis Science Gallery

### Thesis

Volume 3 fills the final conceptual gaps in XOmnibus with five engines drawn from underexplored or entirely novel synthesis science. Where Volumes 1–2 built on established paradigms (subtractive, granular, FM, wavetable, physical modeling, chaos), Volume 3 reaches into academic research, forgotten commercial dead-ends, and mathematical domains never before applied to real-time audio synthesis.

**The five pillars:**
1. **OBSIDIAN** — Crystalline Phase Distortion (the melodic anchor — peak leads, pads, arps)
2. **ORIGAMI** — Spectral Folding (geometric spectral transformation in FFT domain)
3. **ORACLE** — Stochastic GENDY Synthesis (Xenakis's algorithmic waveform construction + maqam microtonality)
4. **OBSCURA** — Scanned Synthesis (Verlet-integrated mass-spring network read by a moving scanner)
5. **OCEANIC** — Swarm Particle Synthesis (autonomous boid-flocking oscillators with emergent collective behavior)

**Design principles for all V3 engines:**
- Each engine introduces a synthesis paradigm not represented anywhere in the existing gallery or V1/V2 pipeline
- Cultural lens: every engine is grounded in a specific world music tradition that shares structural DNA with its synthesis method
- Instrument heritage: every engine honors forgotten, obscure, or underappreciated instruments and inventors
- CPU budget enforced: no engine exceeds 15% single-engine at 44.1kHz / 512 block on M1
- Parameter prefix locked from day one: `obsidian_`, `origami_`, `oracle_`, `obscura_`, `ocean_`
- Presets in `.xometa` JSON from day one
- DSP in inline `.h` headers — portable, testable in isolation
- M1–M4 macros produce audible change in every preset

### Gaps Being Filled

| Gap | Engine | Why It Matters |
|-----|--------|---------------|
| No peak melodic engine optimized for leads/pads/arps | OBSIDIAN | The gallery has experimental, textural, rhythmic, and chaotic voices — but no engine specifically designed to be *the* melodic voice. OBSIDIAN fills this with crystalline phase distortion tuned for 80s/synthpop/vaporwave melodic excellence. |
| No spectral-domain transformation engine | ORIGAMI | All existing engines work in the time domain. ORIGAMI operates entirely in the frequency domain via real-time FFT, applying geometric transformations (fold, mirror, rotate, stretch) to spectral content. A fundamentally different way to think about timbre. |
| No stochastic/algorithmic waveform construction | ORACLE | Every engine uses deterministic oscillators or physical models. ORACLE uses Xenakis's GENDY algorithm — stochastic breakpoint interpolation — to *construct* waveforms from probability distributions. The waveform itself is composed by chance within constraints. |
| No physical-simulation-to-wavetable engine | OBSCURA | Physical modeling engines (XOntara, ORGANON) simulate resonance. OBSCURA simulates a physical *object* — a chain of masses and springs — and reads its shape as a waveform. The physical simulation IS the oscillator. |
| No emergent/swarm synthesis | OCEANIC | All engines are "top-down" — the system schedules every oscillator. OCEANIC is "bottom-up" — 128 autonomous oscillator-particles follow flocking rules, and the collective behavior produces the sound. Emergence, not arrangement. |

---

## Development Order & Rationale

### Engine 1: OBSIDIAN — XObsidian (Crystalline Phase Distortion)

**Why first:** OBSIDIAN is the melodic anchor for the entire V3 wave. Its crystalline leads and pads provide the musical context against which the other four experimental engines will be tested and preset-designed. Building the melodic reference point first means every subsequent engine can be evaluated for how it couples with, contrasts against, and complements a strong melodic voice. OBSIDIAN is also the lightest engine in V3 (<8% CPU), making it the lowest-risk starting point.

### Engine 2: ORIGAMI — XOrigami (Spectral Folding)

**Why second:** ORIGAMI introduces real-time FFT infrastructure (STFT analysis-resynthesis with overlap-add) that may be reused or referenced by other engines. Building it early establishes the spectral-domain toolkit. Its kaleidoscopic timbres complement OBSIDIAN's crystalline clarity — together they cover melodic + textural territory for early V3 preset design. Moderate CPU cost (~12%).

### Engine 3: ORACLE — XOracle (Stochastic GENDY + Maqam)

**Why third:** ORACLE is the most mathematically novel engine — stochastic waveform construction has almost no commercial precedent. Building it third allows the team to tackle the hardest algorithmic challenge (GENDY breakpoint interpolation with mirror barriers) after establishing confidence with PD and FFT. ORACLE's microtonal maqam system also requires careful tuning infrastructure. Moderate CPU (~10%).

### Engine 4: OBSCURA — XObscura (Scanned Synthesis)

**Why fourth:** OBSCURA requires Verlet physics integration and real-time mass-spring simulation — the heaviest per-sample math in V3 (though still <12% CPU). Building it fourth means the physics infrastructure benefits from lessons learned in ORACLE's stochastic math and ORIGAMI's buffer management. OBSCURA's cinematic, evolving timbres provide the atmospheric layer that bridges the melodic engines (OBSIDIAN, ORIGAMI) and the experimental engines (ORACLE, OCEANIC).

### Engine 5: OCEANIC — XOceanic (Swarm Particle Synthesis)

**Why last:** OCEANIC is the most emergent and least predictable engine — 128 autonomous particles with boid dynamics producing sound through collective behavior. It benefits from being built last because: (a) all four other V3 engines exist as coupling partners for testing swarm interaction, (b) the particle system's musical utility is most apparent when evaluated against a complete V3 palette, (c) preset design for an emergent engine requires the most iteration and benefits from a mature creative context. Peak CPU (~15%).

---

## Engine 1: OBSIDIAN — XObsidian

### Identity

- **Gallery code:** OBSIDIAN
- **Source instrument:** XObsidian
- **Accent color:** Volcanic Glass `#2D2D3F` with iridescent highlight `#8B5CF6`
- **Thesis:** Crystalline phase distortion synthesis — the lost 80s method perfected. Peak melodic engine for leads, pads, and arps.
- **Parameter prefix:** `obsidian_`
- **Max voices:** 16
- **CPU budget:** <8%

### Why OBSIDIAN First

The gallery has texture (BOB), width (FAT), chaos (OUROBOROS), granular (OPAL), drums (ONSET), and metabolic evolution (ORGANON). It has no engine specifically optimized to be the *peak melodic voice* — the sound that carries the melody, the lead that cuts glass, the pad that shimmers. OBSIDIAN fills this gap with a synthesis method that produces inherently coherent harmonic evolution: all partials derive from a single phase-warping operation, so they move together as a unified musical voice.

**Key coupling routes unlocked:**
> **OBSIDIAN → OCEANIC** — Crystalline phase distortion drives swarm attractor position. The crystal herds the flock.
> **OBSCURA → OBSIDIAN** — Scanned synthesis mass positions modulate stiffness parameter. Physical simulation shapes the crystal's inharmonicity.
> **ORIGAMI → OBSIDIAN** — Spectrally folded output becomes the phase distortion function. Folded spectra AS waveshaping.

### Cultural Lens

**City Pop** (Japan, 1978–1988) and **Italo Disco** (Italy, 1977–1992) — the two great cultural phase distortions of the 1980s. American pop culture transmitted through Japanese and Italian aesthetic sensibilities, arriving as something crystalline and slightly uncanny. Both genres extensively used the synthesizers of the era (DX7, CZ-101, Jupiter-8) and both prioritized melody above all else. They are the direct ancestors of vaporwave.

### Instrument Heritage

- **Casio CZ-101** (1984, Tokyo) — introduced phase distortion synthesis at a fraction of the DX7's price. Adopted by Vince Clarke, OMD, and countless Italo Disco producers.
- **Cristal Baschet** (Bernard & François Baschet, 1952, Paris) — glass rods amplified through flame-shaped aluminum radiators. Fewer than 30 exist worldwide.
- **Glass Armonica** (Benjamin Franklin, 1761) — banned in several European cities for causing "excessive melancholy."
- **Ensoniq Fizmo** (1998, Malvern, PA) — commercial failure, cult classic. "Liquid crystal" timbres that circulate in vaporwave production communities today.

### Build Phases

#### Phase 0: COMPLETE (this document)
Design spec at `Docs/xobsidian_design_spec.md`.

#### Phase 1 — Parameter Architecture
- All parameter IDs locked with `obsidian_` prefix
- JSON schema, PresetManager scaffold
- **Gate:** compiles, save→load round-trips

#### Phase 2 — Core PD Engine
- Cosine oscillator with phase distortion stage 1
- 2D morphable distortion function space (X = harmonic density, Y = spectral tilt)
- Distortion depth envelope (DAHDSR)
- Stiffness engine (Euler-Bernoulli inharmonicity)
- Voice pool: mono/legato/poly8/poly16
- **Gate:** audible crystalline tone, distortion sweep produces FM-like harmonics

#### Phase 3 — Cascade + Cross-Modulation
- Stage 2 phase distortion (cascaded from stage 1)
- Cross-modulation bus (stage 1 output modulates stage 2 depth/rate/function)
- Stereo phase divergence (L/R use slightly different distortion functions)
- **Gate:** cascade produces richer spectra than single-stage, stereo width audible

#### Phase 4 — Resonance Network + Modulation
- 4-formant post-filter (vocal tract modeling)
- Expression mapping (velocity → depth, aftertouch → stiffness, mod wheel → cross-mod)
- LFO1, LFO2, mod envelope
- 4 macros: CHARACTER, MOVEMENT, COUPLING, SPACE
- **Gate:** macros produce audible layered tonal movement, formant adds "singing" quality

#### Phase 5 — FX Chain
- Chorus / ensemble (stereo thickening complementing the PD stereo phase)
- Delay (crystal echo — bright, pristine repeats)
- Reverb (glass chamber — shimmering, extended decay)
- Finish (soft clip, width, lo-fi degradation for vaporwave aesthetic)
- **Gate:** crystalline character preserved through FX chain

#### Phase 6 — Presets
- 10 hero presets first (Crystal Lead, Glass Pad, Obsidian Arp, Volcanic Bell, etc.)
- Fill to 150 presets in `.xometa` format
- Category: Foundation (40), Atmosphere (25), Entangled (20), Prism (25), Flux (25), Aether (15)
- **Gate:** all macros respond, deterministic load, DNA computed

#### Phase 7 — UI + Polish
- Volcanic glass dark base with iridescent purple highlights
- Distortion function visualizer (shows real-time phase warping)
- 5 pages: Main / PD Core / Mod / FX / Browser
- Full QA pass
- **Gate:** Definition of Done criteria met

#### Phase 3.X — XOmnibus Integration Prep
- Write adapter against `SynthEngine` interface
- Verify coupling: `AudioToFM`, `AmpToFilter`, `EnvToMorph` supported
- Write integration spec
- Test coupling routes with ORIGAMI, OBSCURA, OCEANIC locally

#### Phase 4.X — Gallery Install
- Copy DSP headers to `XO_OX-XOmnibus/Source/Engines/OBSIDIAN/`
- `REGISTER_ENGINE(XObsidianAdapter)`
- Copy presets to XOmnibus Presets directory
- Run `compute_preset_dna.py`
- Design 10 cross-engine Entangled presets
- Update gallery docs

### Coupling Matrix (OBSIDIAN)

| As Target | Source Engine | Type | Musical Effect |
|-----------|-------------|------|----------------|
| OBSIDIAN receives | OBSCURA | AmpToFilter | Scanned mass positions modulate stiffness — physical sim shapes crystal inharmonicity |
| OBSIDIAN receives | ORIGAMI | AudioToFM | Spectrally folded output becomes PD distortion function — folded spectra AS waveshaping |
| OBSIDIAN receives | ORACLE | AudioToFM | Stochastic GENDY curves replace smooth distortion functions — crystal with cracks |
| OBSIDIAN receives | OCEANIC | AmpToFilter | Swarm density modulates formant intensity — flock makes the crystal sing |
| OBSIDIAN receives | FAT | AmpToFilter | 13-osc amplitude drives distortion depth — massive harmonics through PD cascade |

| As Source | Target Engine | Type | Musical Effect |
|-----------|-------------|------|----------------|
| OBSIDIAN sends | OCEANIC | AudioToFM | Crystal tone drives swarm attractor — the lead herds the flock |
| OBSIDIAN sends | DUB | getSample | Crystalline output through dub echo/spring chain |
| OBSIDIAN sends | OPAL | AudioToWavetable | PD harmonics granulated — crystal shattered into time particles |

---

## Engine 2: ORIGAMI — XOrigami

### Identity

- **Gallery code:** ORIGAMI
- **Source instrument:** XOrigami
- **Accent color:** Vermillion Fold `#E63946`
- **Thesis:** Spectral folding synthesis — geometric transformation of frequency-domain content through real-time FFT
- **Parameter prefix:** `origami_`
- **Max voices:** 8 (each voice maintains independent FFT state)
- **CPU budget:** <12%

### Why ORIGAMI Second

Every existing engine works in the time domain — oscillators generating waveforms sample-by-sample. ORIGAMI operates entirely in the frequency domain. Sound enters as spectral data (via internal oscillator or coupling input), and geometric transformations — fold, mirror, rotate, stretch — reshape the spectrum before inverse FFT reconstructs audio. This is a fundamentally different way to think about timbre: not "what waveform am I generating?" but "what shape does my spectrum have?"

**Key coupling routes unlocked:**
> **ORIGAMI → OBSIDIAN** — Spectrally folded output drives PD distortion function. The fold becomes the waveshape.
> **DRIFT → ORIGAMI** — Climax bloom enters FFT and gets spectrally folded. Psychedelic pads kaleidoscoped.
> **ORIGAMI → OPAL** — Folded spectral output granulated. Kaleidoscopic grains.

### Cultural Lens

**Gamelan** (Java/Bali, Indonesia) — the original spectral folding. Gamelan metallophones produce inharmonic spectra where partials don't follow the harmonic series — instead they cluster around specific formant regions determined by the metallurgy and physical geometry of each key. When multiple gamelan instruments play simultaneously, their inharmonic spectra fold into each other, creating composite timbres impossible with harmonic instruments. The kotèkan interlocking technique — where two players each perform half a melody, their parts folding together into a composite whole — is spectral folding as musical practice.

### Instrument Heritage

- **ANS Synthesizer** (Evgeny Murzin, 1958, Moscow) — 720-sine-wave optical photosonic instrument. Drawing on glass plates created spectral shapes directly.
- **Fairlight CMI** (Peter Vogel & Kim Ryrie, 1979, Sydney) — first commercial sampler with spectral editing ("Page D" waveform drawing).
- **Buchla Music Easel** (Don Buchla, 1973) — the 296 Spectral Processor applied frequency-domain thinking to modular synthesis decades before FFT was computationally feasible in real-time.

### Build Phases

#### Phase 0: COMPLETE (this document)
Design spec at `Docs/xorigami_design_spec.md`.

#### Phase 1 — Parameter Architecture + FFT Infrastructure
- STFT framework: 2048-sample window, 4x overlap, Hann window
- Overlap-add reconstruction with phase vocoder
- All parameter IDs locked with `origami_` prefix
- **Gate:** FFT analysis-resynthesis passes audio transparently (unity gain, no artifacts)

#### Phase 2 — Spectral Fold Engine
- Internal oscillator bank (source material for folding when no coupling input)
- Fold operation: frequency axis reflected at a configurable fold point
- Mirror operation: bilateral spectral symmetry around fold point
- Stretch/compress: nonlinear frequency-axis warping
- Rotate: circular shift of spectral content
- **Gate:** single fold operation produces audible, musically interesting timbral change

#### Phase 3 — Multi-Fold + Modulation
- Cascade: up to 4 fold operations in series (kaleidoscope effect)
- Fold point modulation by LFO, envelope, and expression
- Spectral freeze (hold current spectral frame, apply folds to frozen content)
- **Gate:** cascaded folds produce increasingly complex, non-repeating spectral patterns

#### Phase 4 — Macros + Expression
- 4 macros: CHARACTER, MOVEMENT, COUPLING, SPACE
- Mod matrix (6 slots)
- PlaySurface mapping
- **Gate:** macros produce dramatic spectral transformation

#### Phase 5 — FX + Presets
- FX chain (spectral delay, spectral reverb, conventional chorus/delay)
- 150 presets in `.xometa` format
- **Gate:** all macros respond, deterministic load, DNA computed

#### Phase 6 — UI + Polish
- Vermillion fold UI with real-time spectrogram visualizer showing fold geometry
- Full QA pass
- **Gate:** Definition of Done criteria met

### Coupling Matrix (ORIGAMI)

| As Target | Source Engine | Type | Musical Effect |
|-----------|-------------|------|----------------|
| ORIGAMI receives | DRIFT | AudioToWavetable | Climax bloom spectrally folded — psychedelic kaleidoscope |
| ORIGAMI receives | FAT | AudioToWavetable | 13-osc stacked timbre spectrally transformed |
| ORIGAMI receives | MORPH | AudioToWavetable | Wavetable output folded — wavetable of wavetables |
| ORIGAMI receives | OBSIDIAN | AudioToFM | Crystal harmonics fold-modulate the spectral content |

| As Source | Target Engine | Type | Musical Effect |
|-----------|-------------|------|----------------|
| ORIGAMI sends | OBSIDIAN | AudioToFM | Folded spectral output drives PD distortion function |
| ORIGAMI sends | OPAL | AudioToWavetable | Kaleidoscopic audio granulated into spectral particles |
| ORIGAMI sends | DUB | getSample | Folded output through dub effects chain |

---

## Engine 3: ORACLE — XOracle

### Identity

- **Gallery code:** ORACLE
- **Source instrument:** XOracle
- **Accent color:** Prophecy Indigo `#4B0082`
- **Thesis:** Stochastic waveform construction (GENDY) with maqam microtonal intelligence — chance within constraint
- **Parameter prefix:** `oracle_`
- **Max voices:** 8
- **CPU budget:** <10%

### Why ORACLE Third

Every existing engine uses deterministic oscillators — given the same parameters, they produce the same waveform every time. ORACLE introduces genuine stochasticity into waveform construction. Using Iannis Xenakis's GENDY algorithm (1991), the engine constructs waveforms from probability distributions: breakpoints defined by random walks in time and amplitude, constrained by mirror barriers. Each waveform cycle is unique. Combined with a maqam-aware microtonal system, ORACLE produces sounds that feel simultaneously ancient and alien.

**Key coupling routes unlocked:**
> **ORACLE → OBSIDIAN** — Stochastic breakpoint curves replace smooth PD functions. Crystal with cracks.
> **ORACLE → ORIGAMI** — GENDY waveforms enter FFT and get spectrally folded. Stochastic kaleidoscope.
> **ORACLE × ORGANON** — Stochastic waveforms feed the metabolic engine. Random nutrients.

### Cultural Lens

**Maqam** (Arab world, Turkey, Iran, Central Asia) — the modal system underlying Arabic, Turkish, and Persian classical music. Maqam is not a scale — it's a set of behavioral rules governing how melody moves through microtonal pitch space. Specific intervals are wider or narrower depending on whether the melody ascends or descends, which degree it's approaching, and the emotional trajectory of the improvisation (taqsim). This context-dependent pitch behavior maps directly to GENDY's constrained stochastic walks: the breakpoints (notes) are probabilistic, but the barriers (maqam rules) ensure musical coherence.

### Instrument Heritage

- **UPIC** (Iannis Xenakis, 1977, CEMAMu, Paris) — electromagnetic drawing tablet where graphical gestures became sound. Xenakis's bridge between visual composition and audio synthesis.
- **Bazantar** (Mark Deutsch, 2000, San Diego) — a 5-string acoustic bass with 29 sympathetic drone strings and a cello-like body. Produces microtonal resonance clouds that exemplify the maqam concept of "pitch as field."
- **Ondes Martenot Diffuseurs** (Maurice Martenot, 1928, Paris) — three specialized speakers (palme, métallique, résonance) that gave the Ondes Martenot its extraordinary timbral range. The palme diffuser used a stretched speaker cone strung with metal springs, creating sympathetic resonance halos.
- **Halim El-Dabh's wire recorder experiments** (1944, Cairo) — arguably the first musique concrète. El-Dabh manipulated recordings of a zaar ceremony at the Middle East Radio studios a full four years before Pierre Schaeffer's celebrated études. Almost entirely overlooked in Western electronic music history.

### Build Phases

#### Phase 0: COMPLETE (this document)
Design spec at `Docs/xoracle_design_spec.md`.

#### Phase 1 — Parameter Architecture + GENDY Core
- Breakpoint interpolation engine (N breakpoints, cubic Hermite interpolation)
- Random walk system: Cauchy and logistic distributions for time/amplitude steps
- Mirror barriers (elastic boundaries constraining breakpoint movement)
- All parameter IDs locked with `oracle_` prefix
- **Gate:** GENDY oscillator produces pitched, controllable stochastic tones

#### Phase 2 — Maqam Microtonal System
- Maqam tuning tables (Rast, Bayati, Saba, Hijaz, Sikah, Nahawand, etc.)
- Context-dependent pitch adjustment (ascend vs descend intervals)
- Jins (tetrachord) detection and transition logic
- **Gate:** maqam tuning audible, ascending/descending intervals differ correctly

#### Phase 3 — Modulation + Macros
- LFO1, LFO2, mod envelope
- 4 macros: CHARACTER, MOVEMENT, COUPLING, SPACE
- Mod matrix (6 slots)
- Distribution morphing (smooth interpolation between Cauchy and logistic)
- **Gate:** macros produce audible change from subtle drift to full stochastic chaos

#### Phase 4 — FX + Presets
- FX chain (stochastic delay, reverb, saturation)
- 150 presets in `.xometa` format
- **Gate:** all macros respond, deterministic load, DNA computed

#### Phase 5 — UI + Polish
- Deep indigo UI with breakpoint visualizer showing real-time GENDY waveform construction
- Maqam mode indicator
- Full QA pass
- **Gate:** Definition of Done criteria met

### Coupling Matrix (ORACLE)

| As Target | Source Engine | Type | Musical Effect |
|-----------|-------------|------|----------------|
| ORACLE receives | ORGANON | AmpToFilter | Metabolic breathing modulates GENDY mirror barriers — organism constrains chaos |
| ORACLE receives | OUROBOROS | AudioToFM | Chaotic attractor perturbs breakpoint walks — chaos feeding stochasticity |
| ORACLE receives | SNAP | AmpToFilter | Percussive envelopes punch through stochastic texture |

| As Source | Target Engine | Type | Musical Effect |
|-----------|-------------|------|----------------|
| ORACLE sends | OBSIDIAN | AudioToFM | GENDY curves replace smooth PD functions — crystal with cracks |
| ORACLE sends | ORIGAMI | AudioToWavetable | Stochastic waveforms enter FFT for spectral folding — random kaleidoscope |
| ORACLE sends | ORGANON | AudioToWavetable | Stochastic audio feeds metabolic engine — random nutrients |
| ORACLE sends | DUB | getSample | GENDY output through dub effects chain |

---

## Engine 4: OBSCURA — XObscura

### Identity

- **Gallery code:** OBSCURA
- **Source instrument:** XObscura
- **Accent color:** Daguerreotype Silver `#8A9BA8`
- **Thesis:** Scanned synthesis — a vibrating physical model read by a moving scanner produces continuously evolving wavetables from Newtonian physics
- **Parameter prefix:** `obscura_`
- **Max voices:** 8
- **CPU budget:** <12%

### Why OBSCURA Fourth

Physical modeling engines simulate resonant bodies (XOntara, ORGANON). OBSCURA simulates a physical *object* — a 1D chain of masses connected by springs with adjustable stiffness, damping, and nonlinearity — then *reads its shape as a waveform*. The physical simulation IS the oscillator. A scanner sweeps across the vibrating chain at audio rate, and the displacement values it reads become the output waveform. Because the chain is constantly vibrating and evolving under Newtonian physics, the waveform is never static — it breathes, ripples, and transforms continuously. This is Bill Verplank and Max Mathews's scanned synthesis (2000), barely explored beyond academic papers and a handful of CSound opcodes.

**Key coupling routes unlocked:**
> **OBSCURA → OBSIDIAN** — Mass-spring displacement drives PD stiffness parameter. Physics shapes crystal.
> **OBSCURA → OCEANIC** — Scanner position modulates swarm attractor. The physical simulation herds the flock.
> **OBSIDIAN → OBSCURA** — Crystal harmonics excite the mass-spring chain. PD output as physical force.

### Cultural Lens

**Sardinian Tenores** (Sardinia, Italy) — the cantu a tenore vocal tradition where four male voices (bassu, contra, mesu boghe, boghe) create a vibrating vocal "chain." The bassu and contra produce sustained drones that physically vibrate in the singers' shared acoustic space, while the mesu boghe and boghe sing melodies that "scan" across this vibrating field. The resulting overtone interactions create timbres impossible for any single voice. UNESCO Intangible Cultural Heritage since 2005. The four voices form a literal 1D chain of coupled resonators — the bassu anchors one end, the boghe floats at the other, and the middle voices are the masses on springs between them.

### Instrument Heritage

- **Telharmonium** (Thaddeus Cahill, 1897, Washington D.C.) — the 200-ton, 60-foot electromechanical instrument that generated audio by spinning massive tonewheels past electromagnetic pickups. The first instrument where a physical mechanism was scanned to produce audio — the tonewheel IS the vibrating object, the pickup IS the scanner.
- **Cristal Baschet** (referenced also in OBSIDIAN) — glass rods read by wet fingertips as a scanning interface.
- **Bill Verplank & Max Mathews's Scanned Synthesis** (2000, Stanford/Interval Research) — the theoretical framework that OBSCURA directly implements. Mathews (the father of computer music, creator of MUSIC I–V) spent his final years exploring scanned synthesis as the next frontier of digital sound. The technique was presented at ICMC 2000 but never reached commercial synthesis.

### Build Phases

#### Phase 0: COMPLETE (this document)
Design spec at `Docs/xobscura_design_spec.md`.

#### Phase 1 — Parameter Architecture + Physics Core
- Verlet integrator for N-mass chain (128 masses default)
- Spring stiffness, damping, nonlinearity per mass
- Boundary conditions (fixed, free, periodic)
- All parameter IDs locked with `obscura_` prefix
- **Gate:** mass-spring chain vibrates stably, responds to impulse excitation

#### Phase 2 — Scanner + Audio Output
- Audio-rate scanner (circular sweep across mass chain)
- Scanner speed = pitch (MIDI note controls scan rate)
- Scanner width (single point vs. averaged window)
- Excitation system (impulse, continuous force, coupling input)
- **Gate:** pitched, evolving tones produced from scanning the vibrating chain

#### Phase 3 — Chain Shaping + Modulation
- Initial displacement presets (sine, saw, random, drawn)
- Nonlinear spring modes (cubic stiffness for metallic tones)
- Mass injection (add/remove masses for timbral density control)
- LFO, envelope, mod matrix
- 4 macros: CHARACTER, MOVEMENT, COUPLING, SPACE
- **Gate:** macros produce dramatic timbral evolution

#### Phase 4 — FX + Presets
- FX chain (physical reverb, delay, saturation)
- 150 presets in `.xometa` format
- **Gate:** all macros respond, deterministic load, DNA computed

#### Phase 5 — UI + Polish
- Silver/daguerreotype UI with mass-spring chain visualizer
- Real-time display of scanner position and chain displacement
- Full QA pass
- **Gate:** Definition of Done criteria met

### Coupling Matrix (OBSCURA)

| As Target | Source Engine | Type | Musical Effect |
|-----------|-------------|------|----------------|
| OBSCURA receives | OBSIDIAN | AudioToFM | Crystal harmonics excite mass-spring chain — PD as physical force |
| OBSCURA receives | ONSET | AmpToFilter | Drum hits inject impulse energy into the chain — percussion excites physics |
| OBSCURA receives | SNAP | AudioToFM | Karplus-Strong pluck excites mass-spring — string exciting string |
| OBSCURA receives | OUROBOROS | AudioToFM | Chaotic attractor perturbs mass positions — chaos rattles the chain |

| As Source | Target Engine | Type | Musical Effect |
|-----------|-------------|------|----------------|
| OBSCURA sends | OBSIDIAN | AmpToFilter | Mass displacement drives PD stiffness — physics shapes crystal |
| OBSCURA sends | OCEANIC | AudioToFM | Scanner output modulates swarm attractor — physical sim herds the flock |
| OBSCURA sends | DUB | getSample | Scanned output through dub effects chain |
| OBSCURA sends | OPAL | AudioToWavetable | Scanned waveform granulated — physics shattered into time particles |

---

## Engine 5: OCEANIC — XOceanic

### Identity

- **Gallery code:** OCEANIC
- **Source instrument:** XOceanic
- **Accent color:** Phosphorescent Teal `#00B4A0`
- **Thesis:** Swarm particle synthesis — 128 autonomous oscillator-particles with boid flocking dynamics producing emergent collective timbre
- **Parameter prefix:** `ocean_`
- **Max voices:** 4 (each voice is an independent swarm of 128 particles)
- **CPU budget:** <15%

### Why OCEANIC Last

OCEANIC is the most emergent engine in V3 — its sound arises from the collective behavior of 128 autonomous particles following simple flocking rules. The output is inherently unpredictable (in a bounded, musical way). Building it last ensures: (a) all four other V3 engines exist as coupling partners for swarm interaction testing, (b) the preset designers have a complete V3 palette to evaluate swarm contributions against, (c) the most iteration-dependent engine gets the most accumulated development experience.

**Key coupling routes unlocked:**
> **OBSIDIAN → OCEANIC** — Crystal tone drives swarm attractor position. The lead herds the flock.
> **OCEANIC → ORIGAMI** — Swarm output enters FFT for spectral folding. Emergent kaleidoscope.
> **OCEANIC × ORGANON** — Swarm density feeds metabolic rate. More particles = faster metabolism.

### Cultural Lens

**Salegy** (Madagascar), **Kompa** (Haiti), and **Chutney Soca** (Trinidad & Tobago) — three traditions where interlocking autonomous musical agents create emergent collective groove.

*Salegy:* Each musician (accordion, guitar, bass, percussion) plays a simple independent figure with no conductor. The resulting 6/8 polyrhythm is emergent — separation (each instrument in its own frequency/rhythmic space), alignment (everyone drifts in the same tempo direction), cohesion (all patterns orbit the tonal center). Boid dynamics in acoustic form.

*Kompa:* Layers of tanbou, guitar skank, bass ostinato, and horn stabs create the *gouyad* groove — a rolling, hip-driven feel that emerges from autonomous layer interaction. Kompa musicians describe the groove as something that "arrives" during performance, not something they play.

*Chutney Soca:* Indian musical traditions (dholak, tassa, film-song melody) and Afro-Caribbean traditions (soca rhythm, calypso melody, steelpan) coexist as autonomous agents. Neither tradition dissolves; both maintain identity while producing emergent hybrid forms. Boid dynamics with two sub-flocks.

### Instrument Heritage

- **Swarmatron** (Dewanatron, designed by Leon & Brian Dewan, 2004) — eight analog oscillators with swarming behavior. Used by Trent Reznor/Alessandro Cortini. Proved that flocking oscillators produce sounds no other method can.
- **Verbos Harmonic Oscillator** (Mark Verbos, 2014) — eurorack module allowing real-time rebalancing of harmonic partials. Demonstrated that controlling many simultaneous oscillators produces extraordinary timbres.
- **Cristián Vogel's Buchla "flock" patches** (2000s) — cross-patched 200e oscillators through feedback networks. Unrepeatable emergent performances.

### Build Phases

#### Phase 0: COMPLETE (this document)
Design spec at `Docs/xoceanic_design_spec.md`.

#### Phase 1 — Parameter Architecture + Boid Core
- 128-particle pool: each with frequency, amplitude, pan, velocity vector
- Boid force evaluation: separation, alignment, cohesion in 3D (freq/amp/pan)
- MIDI attractor (tether to note frequency)
- All parameter IDs locked with `ocean_` prefix
- **Gate:** particles self-organize visibly, boid rules produce stable flocking

#### Phase 2 — Audio Output + Particle Oscillators
- 128 PolyBLEP oscillators (one per particle)
- Waveform selection per particle (sine/saw/pulse/noise)
- Particle amplitude weighted by distance from swarm centroid
- Note-on perturbation (velocity impulse scatters swarm)
- **Gate:** pitched, evolving swarm tone responds to MIDI, note-on scatter audible

#### Phase 3 — Sub-Flocks + Advanced Dynamics
- Sub-flock system (1–4 independent sub-swarms with separate attractors)
- Murmuration trigger (cascading reorganization wave through swarm)
- Damping control (velocity decay rate)
- LFO, envelope, mod matrix
- 4 macros: CHARACTER, MOVEMENT, COUPLING, SPACE
- **Gate:** sub-flocks orbit independently, murmuration produces dramatic timbral shift

#### Phase 4 — FX + Presets
- FX chain (swarm chorus, delay, reverb, spatial enhancement)
- 150 presets in `.xometa` format
- **Gate:** all macros respond, deterministic load, DNA computed

#### Phase 5 — UI + Polish
- Phosphorescent teal UI with real-time particle visualization (2D frequency/pan plot)
- Particle trails showing recent movement
- Full QA pass
- **Gate:** Definition of Done criteria met

### Coupling Matrix (OCEANIC)

| As Target | Source Engine | Type | Musical Effect |
|-----------|-------------|------|----------------|
| OCEANIC receives | OBSIDIAN | AudioToFM | Crystal harmonics drive swarm attractor — the lead herds the flock |
| OCEANIC receives | OBSCURA | AudioToFM | Scanned output modulates particle velocities — physics perturbs swarm |
| OCEANIC receives | ONSET | RhythmToBlend | Drum hits trigger murmuration — percussion startles the flock |
| OCEANIC receives | ORGANON | AmpToFilter | Metabolic breathing modulates cohesion — organism breathes the swarm |

| As Source | Target Engine | Type | Musical Effect |
|-----------|-------------|------|----------------|
| OCEANIC sends | ORIGAMI | AudioToWavetable | Swarm output enters FFT for spectral folding — emergent kaleidoscope |
| OCEANIC sends | OBSIDIAN | AmpToFilter | Swarm density modulates formant intensity — flock makes crystal sing |
| OCEANIC sends | DUB | getSample | Swarm output through dub effects chain |
| OCEANIC sends | OPAL | AudioToWavetable | Swarm audio granulated — flock shattered into time particles |

---

## CPU Budget With All Five Added

| Configuration | Engines | Est. CPU |
|---------------|---------|----------|
| V3 lightest pair | OBSIDIAN + ORIGAMI | ~20% |
| V3 melodic pair | OBSIDIAN + ORACLE | ~18% |
| V3 physical pair | OBSCURA + OCEANIC | ~27% |
| V3 heaviest pair | OCEANIC + ORIGAMI | ~27% |
| V3 triple (melodic) | OBSIDIAN + ORIGAMI + ORACLE | ~30% |
| V3 triple (experimental) | ORACLE + OBSCURA + OCEANIC | ~37% |
| All five V3 (theoretical) | All V3 engines | ~57% with voice reduction |
| V3 + V1 showcase | OBSIDIAN + OCEANIC + BITE + OPAL | ~45% |

OCEANIC is the heaviest V3 engine (128 PolyBLEP oscillators per voice × 4 voices). Eco mode recommendation: cap particles at 64 in 4-engine configs.

---

## Cross-Engine Preset Priority (V3 Gallery Opening)

When all five V3 engines are installed, these are the priority cross-engine presets:

1. **OBSIDIAN × OCEANIC** — *Crystal Flock* — PD crystal herds swarm particles into formation
2. **OBSCURA × OBSIDIAN** — *Glass Physics* — scanned mass-spring drives PD stiffness in real-time
3. **ORIGAMI × ORACLE** — *Stochastic Kaleidoscope* — GENDY waveforms spectrally folded
4. **ORACLE × ORGANON** — *Random Nutrients* — stochastic audio feeds metabolic engine
5. **OCEANIC × ORIGAMI** — *Emergent Geometry* — swarm output spectrally folded into evolving patterns
6. **OBSIDIAN × ORIGAMI** — *Folded Crystal* — PD harmonics geometrically transformed
7. **OBSCURA × OCEANIC** — *Physics Herds Flock* — scanner output drives swarm attractor
8. **ALL V3** — *The Synthesis Lab* — 4-slot config cycling all five engines in coupled pairs
9. **OBSIDIAN × DRIFT** — *Crystal Bloom* — PD lead through Climax psychedelic bloom
10. **OCEANIC × OPAL** — *Swarm Scatter* — swarm output granulated into time-scattered particles

---

## V3 Inter-Engine Coupling Web

```
                    OBSIDIAN
                   /    |    \
                  /     |     \
            ORIGAMI --- | --- ORACLE
                  \     |     /
                   \    |    /
                    OBSCURA
                       |
                    OCEANIC

Every V3 engine couples bidirectionally with at least 2 other V3 engines.
Every V3 engine couples with at least 1 engine from V0/V1/V2.
The coupling web ensures no V3 engine is isolated.
```

### V3 Coupling Types Used

| Coupling Type | Used By |
|---------------|---------|
| AudioToFM | ORIGAMI→OBSIDIAN, ORACLE→OBSIDIAN, OBSIDIAN→OCEANIC, OBSIDIAN→OBSCURA, OBSCURA→OCEANIC |
| AmpToFilter | OBSCURA→OBSIDIAN, OCEANIC→OBSIDIAN, ORGANON→ORACLE, OCEANIC (formant mod) |
| AudioToWavetable | ORACLE→ORIGAMI, OCEANIC→ORIGAMI, OBSCURA→OPAL, OCEANIC→OPAL |
| RhythmToBlend | ONSET→OCEANIC (murmuration trigger) |
| getSample | All V3 engines → DUB |
| EnvToMorph | OBSIDIAN (expression mapping), ORIGAMI (fold modulation) |

---

## Full Gallery State After Volume 3

| # | Code | Volume | Synthesis Paradigm |
|---|------|--------|-------------------|
| 1 | SNAP | V0 | Percussive subtractive |
| 2 | MORPH | V0 | Wavetable |
| 3 | DUB | V0 | FX send/return |
| 4 | DRIFT | V0 | Psychedelic subtractive |
| 5 | BOB | V0 | Warm texture subtractive |
| 6 | FAT | V0 | Massive unison stacking |
| 7 | ONSET | V0 | Drum synthesis |
| 8 | OVERWORLD | V0 | Chip / retro |
| 9 | BITE | V1 | Bass character |
| 10 | OPAL | V1 | Granular time-scatter |
| 11 | ORGANON | V2 | Metabolic / dissipative |
| 12 | OUROBOROS | V2 | Chaotic attractor ODE |
| 13 | ORBITAL | V2 | Additive / psychoacoustic |
| 14 | OBSIDIAN | **V3** | **Crystalline phase distortion** |
| 15 | ORIGAMI | **V3** | **Spectral folding (FFT)** |
| 16 | ORACLE | **V3** | **Stochastic GENDY + maqam** |
| 17 | OBSCURA | **V3** | **Scanned synthesis** |
| 18 | OCEANIC | **V3** | **Swarm particle synthesis** |

**18 engines. 18 distinct synthesis paradigms. No two engines share a mathematical foundation.**

---

*Roadmap owner: XO_OX Designs | Process: `/new-xo-engine` skill | Design specs: `Docs/xo{engine}_design_spec.md`*
