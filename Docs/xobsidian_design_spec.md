# XO-Obsidian — Phase 1 Architecture Specification

**Engine:** XO-Obsidian (Crystalline Phase Distortion Synthesis)
**Short Name:** OBSIDIAN
**Engine ID:** `"Obsidian"`
**Parameter Prefix:** `obsidian_`
**Accent Color:** Volcanic Glass `#2D2D3F` with Iridescent Highlight `#8B5CF6`
**Max Voices:** 16
**CPU Budget:** <8% single-engine, <12% dual-engine
**Date:** 2026-03-11

---

## 1. Product Identity

**Thesis:** "XO-Obsidian is a crystalline phase distortion synth that resurrects the lost 80s synthesis method with 40 years of missed evolution — the peak melodic engine for leads, pads, and arps."

**Sound family:** Lead / Pad / Arp / Keys

**Unique capability:** Phase distortion produces sounds with *coherent harmonic evolution* — all partials derive from a single phase-warping operation, so they move together as a unified musical voice rather than as independent frequencies. Extended with multi-stage cascading, cross-modulation, and Euler-Bernoulli stiffness, XO-Obsidian produces timbres with the crystalline clarity of FM synthesis and the organic warmth of analog — simultaneously.

**Personality in 3 words:** Crystalline, melodic, expressive.

**Gallery gap filled:** No existing engine is optimized to be the *peak melodic voice*. The gallery has texture (OBLONG), width (OBESE), chaos (OUROBOROS), granular (OPAL), drums (ONSET), and metabolic evolution (ORGANON). OBSIDIAN is the engine you reach for when the melody must *sing* — when the lead must cut through, the pad must shimmer, the arp must sparkle.

---

## 2. Signal Flow Architecture

```
┌──────────────────────────────────────────────────────────────────────────┐
│                        XO-Obsidian Voice                                  │
│                                                                          │
│  ┌─────────────────────────────────────────────────────────────────────┐ │
│  │  PD STAGE 1                                                         │ │
│  │                                                                     │ │
│  │  Cosine Oscillator ──► Phase Distortion ──► Stage 1 Output          │ │
│  │      (V/Oct)             │                                          │ │
│  │                          │                                          │ │
│  │                    Distortion Function                               │ │
│  │                    (2D Morphable Space)                              │ │
│  │                    X = Harmonic Density                              │ │
│  │                    Y = Spectral Tilt                                 │ │
│  │                          │                                          │ │
│  │                    Depth Envelope (DAHDSR)                           │ │
│  │                          │                                          │ │
│  │                    Stiffness Engine                                  │ │
│  │                    (Euler-Bernoulli Inharmonicity)                   │ │
│  └──────────────────────────┬──────────────────────────────────────────┘ │
│                              │                                           │
│  ┌──────────────────────────▼──────────────────────────────────────────┐ │
│  │  PD STAGE 2 (Cascade)                                               │ │
│  │                                                                     │ │
│  │  Stage 1 Phase ──► Phase Distortion 2 ──► Cascade Output            │ │
│  │                        │                                            │ │
│  │                  Cross-Modulation Bus                                │ │
│  │                  (Stage 1 output modulates                          │ │
│  │                   Stage 2 depth/rate/function)                      │ │
│  │                        │                                            │ │
│  │                  Cascade Blend (0 = Stage 1 only, 1 = full)         │ │
│  └──────────────────────────┬──────────────────────────────────────────┘ │
│                              │                                           │
│  ┌──────────────────────────▼──────────────────────────────────────────┐ │
│  │  RESONANCE NETWORK                                                  │ │
│  │                                                                     │ │
│  │  4-Formant Bandpass Array ──► Formant Mix ──► Post-Resonance        │ │
│  │  (F1: 300-800 Hz, F2: 800-2500 Hz,                                 │ │
│  │   F3: 2500-4000 Hz, F4: 4000-6000 Hz)                              │ │
│  │  Formant Intensity (0 = bypass, 1 = full vocal)                     │ │
│  └──────────────────────────┬──────────────────────────────────────────┘ │
│                              │                                           │
│  ┌──────────────────────────▼──────────────────────────────────────────┐ │
│  │  STEREO PHASE DIVERGENCE                                            │ │
│  │                                                                     │ │
│  │  L channel: Distortion Function at (X, Y)                           │ │
│  │  R channel: Distortion Function at (X + Δ, Y + Δ)                  │ │
│  │  Δ = Stereo param × 0.1 (subtle divergence)                        │ │
│  │  Natural stereo width from phase interference                       │ │
│  └──────────────────────────┬──────────────────────────────────────────┘ │
│                              │                                           │
│                              ▼                                           │
│                    Output Cache [L, R]                                    │
│                                                                          │
└──────────────────────────────────────────────────────────────────────────┘
```

### 2.1 PD Stage 1 — The Core

The foundation of OBSIDIAN is a cosine oscillator whose phase is warped through a nonlinear distortion function before being read. The oscillator itself always runs at the standard V/Oct rate — only the *speed at which it traverses the cosine* changes.

**Phase Distortion Principle:**

Standard cosine oscillator: `output = cos(2π × phase)` where `phase` increments linearly.

Phase-distorted cosine: `output = cos(2π × D(phase))` where `D()` is a nonlinear distortion function that maps `[0, 1]` → `[0, 1]` but with non-uniform speed.

When `D(phase)` accelerates through the positive peak of the cosine, harmonics are added (brightening). When it decelerates through zero-crossings, the waveform approaches a pure sine (darkening). The *shape* of `D()` determines the harmonic spectrum.

**2D Morphable Distortion Space:**

Unlike Casio's 8 fixed distortion shapes, OBSIDIAN parameterizes the distortion function as a 2D morphable surface:

- **X axis (Harmonic Density):** Controls the number of inflection points in `D()`. Low = simple waveforms (sine → triangle → saw). High = complex waveforms (rich, FM-like spectra with many harmonics).
- **Y axis (Spectral Tilt):** Controls the asymmetry of `D()`. Low = energy concentrated in low harmonics (warm). High = energy concentrated in upper harmonics (bright, glassy).

Every (X, Y) coordinate produces a unique, valid distortion function. The space is pre-computed as a 2D lookup table (64×64 grid, bilinear interpolation) for zero-cost real-time morphing.

**Implementation:**

```cpp
struct PDStage
{
    // 2D distortion function LUT: distortionLUT[densityIdx][tiltIdx][phaseIdx]
    // Pre-computed in prepare(), 64 × 64 × 1024 entries
    float distortionLUT[64][64][1024];

    float phase = 0.0f;      // Master phase accumulator [0, 1)
    float depth = 1.0f;       // Distortion depth (0 = pure cosine, 1 = full PD)

    float process(float phaseIncrement, float densityX, float tiltY, float depthEnv)
    {
        phase += phaseIncrement;
        if (phase >= 1.0f) phase -= 1.0f;

        // Lookup distorted phase from 2D morphable function
        float distortedPhase = lookupDistortion(densityX, tiltY, phase);

        // Blend between linear phase (pure cosine) and distorted phase
        float finalPhase = phase + depthEnv * depth * (distortedPhase - phase);

        return std::cos(2.0f * M_PI * finalPhase);
    }
};
```

### 2.2 Stiffness Engine (Euler-Bernoulli Inharmonicity)

Real physical vibrating objects (piano strings, bells, glass) have *inharmonic* overtone series — the partials are stretched above the harmonic series due to the stiffness of the vibrating medium. This is what gives pianos their characteristic warmth and bells their metallic ring.

OBSIDIAN applies this by warping the phase distortion function's periodicity. Instead of the distortion function repeating at exact integer multiples of the fundamental, each repetition is slightly stretched:

```
Partial n frequency = n × f₀ × √(1 + B × n²)
```

Where `B` is the stiffness coefficient (from the Euler-Bernoulli beam equation). At `B = 0`: perfectly harmonic. At `B = 0.001`: piano-like warmth. At `B = 0.1`: bell/glass inharmonicity.

The stiffness parameter continuously morphs the engine from flute-like purity to metallic shimmer in a single gesture.

### 2.3 PD Stage 2 — Cascade

The second PD stage receives the phase output from Stage 1 and applies its own distortion function. This is analogous to two-operator FM synthesis but in the phase domain — it produces richer spectra with fewer aliasing artifacts than FM because the distortion operates on phase (bounded [0,1]) rather than frequency (unbounded).

**Cross-modulation:** Stage 1's output signal modulates Stage 2's distortion parameters:
- Stage 1 output → Stage 2 distortion depth (amplitude cross-mod)
- Stage 1 phase velocity → Stage 2 function position (phase cross-mod)

The `cascade` parameter blends between Stage 1 only (0.0) and full cascade output (1.0).

### 2.4 Resonance Network

A 4-formant bandpass filter array adds vocal-like resonance character to the PD output. Each formant is a 2-pole bandpass filter with configurable center frequency and bandwidth:

| Formant | Center Range | Musical Role |
|---------|-------------|-------------|
| F1 | 300–800 Hz | Body, warmth, vowel openness |
| F2 | 800–2500 Hz | Vowel identity, nasal character |
| F3 | 2500–4000 Hz | Presence, brilliance |
| F4 | 4000–6000 Hz | Air, shimmer |

The `formant` parameter (0–1) blends between bypass (pure PD output) and full formant filtering (vocal-like resonance). At intermediate values, the formant network adds subtle vocal character — the engine "sings" without being explicitly voice-like.

### 2.5 Stereo Phase Divergence

Left and right channels use slightly different distortion function coordinates:
- L channel: `(densityX, tiltY)`
- R channel: `(densityX + Δ, tiltY + Δ)` where `Δ = stereo × 0.1`

This produces natural stereo width from phase interference — no chorus or detune needed. The stereo image is mathematically derived from the distortion space geometry.

---

## 3. Parameter Taxonomy

### 3.1 Core Parameters (8)

| ID | Parameter | Range | Curve | Rate | Description |
|----|-----------|-------|-------|------|-------------|
| `obsidian_distX` | Harmonic Density | 0.0–1.0 | Linear | Control | X position in 2D distortion space. Low = simple waveforms. High = complex, FM-like spectra. CHARACTER macro target. |
| `obsidian_distY` | Spectral Tilt | 0.0–1.0 | Linear | Control | Y position in 2D distortion space. Low = warm (energy in low harmonics). High = bright/glassy (energy in upper harmonics). |
| `obsidian_depth` | Distortion Depth | 0.0–1.0 | Linear | Audio | Intensity of phase distortion. 0 = pure cosine. 1 = maximum harmonic generation. Primary timbre control. |
| `obsidian_stiffness` | Stiffness | 0.0–1.0 | Exponential | Control | Euler-Bernoulli inharmonicity coefficient. 0 = perfectly harmonic. 0.3 = piano warmth. 0.7 = bell/glass. 1.0 = deeply metallic. SPACE macro target. |
| `obsidian_cascade` | Cascade Depth | 0.0–1.0 | Linear | Control | Stage 2 influence. 0 = single-stage PD. 1 = full two-stage cascade. Increases spectral complexity. |
| `obsidian_crossmod` | Cross-Modulation | 0.0–1.0 | Linear | Control | Depth of Stage 1 → Stage 2 cross-modulation. Creates breathing, evolving timbres. MOVEMENT macro target. |
| `obsidian_formant` | Formant Intensity | 0.0–1.0 | Linear | Control | Resonance network intensity. 0 = bypass. 1 = full vocal-like formant filtering. |
| `obsidian_stereo` | Stereo Phase | 0.0–1.0 | Linear | Control | Phase divergence between L/R channels. 0 = mono. 1 = maximum stereo width. |

### 3.2 Macro Mapping

| Macro | Targets | Musical Effect |
|-------|---------|---------------|
| CHARACTER (M1) | `obsidian_distX` (primary), `obsidian_depth` (secondary) | Sweeps from pure sine through triangle through saw through complex FM-like spectra. The timbre journey. |
| MOVEMENT (M2) | `obsidian_crossmod` (primary), LFO1 depth (secondary) | From static crystal to breathing, evolving cascade. Adds life and internal motion. |
| COUPLING (M3) | Coupling input gain, `obsidian_cascade` (secondary) | Controls how much external engines influence OBSIDIAN's timbre. More coupling = more cascade complexity. |
| SPACE (M4) | `obsidian_stiffness` (primary), `obsidian_stereo` (secondary), reverb send | From tight, harmonic, mono core to wide, inharmonic, reverberant crystal space. |

### 3.3 Envelope & Modulation Parameters

| ID | Parameter | Type | Description |
|----|-----------|------|-------------|
| `obsidian_ampAttack` | Amp Attack | Time | 0ms–10s |
| `obsidian_ampDecay` | Amp Decay | Time | 0ms–10s |
| `obsidian_ampSustain` | Amp Sustain | Level | 0–1 |
| `obsidian_ampRelease` | Amp Release | Time | 0ms–20s |
| `obsidian_pdAttack` | PD Env Attack | Time | Distortion depth envelope attack |
| `obsidian_pdDecay` | PD Env Decay | Time | Distortion depth envelope decay |
| `obsidian_pdSustain` | PD Env Sustain | Level | Distortion depth envelope sustain |
| `obsidian_pdRelease` | PD Env Release | Time | Distortion depth envelope release |
| `obsidian_lfo1Rate` | LFO 1 Rate | Hz | 0.01–30 Hz |
| `obsidian_lfo1Depth` | LFO 1 Depth | Level | LFO 1 modulation depth |
| `obsidian_lfo1Shape` | LFO 1 Shape | Enum | Sine / Triangle / Saw / Square / S&H |
| `obsidian_lfo2Rate` | LFO 2 Rate | Hz | 0.01–30 Hz |
| `obsidian_lfo2Depth` | LFO 2 Depth | Level | LFO 2 modulation depth |
| `obsidian_lfo2Shape` | LFO 2 Shape | Enum | Sine / Triangle / Saw / Square / S&H |

### 3.4 Voice Parameters

| ID | Parameter | Description |
|----|-----------|-------------|
| `obsidian_voiceMode` | Voice Mode | Mono / Legato / Poly8 / Poly16 |
| `obsidian_glide` | Glide Time | Portamento time (0–2s) |
| `obsidian_unisonVoices` | Unison Count | 1–4 voices per note |
| `obsidian_unisonSpread` | Unison Detune | Spread in cents (0–50) |

---

## 4. The Ghosts in OBSIDIAN

### Ghost 1: The Casio CZ-101 (1984) — The Phase Distortion Pioneer

**The instrument:** Masahiro Kawakami's team at Casio created phase distortion synthesis as an alternative to Yamaha's FM patent. Instead of modulating one oscillator's frequency with another, PD warps the phase of a single cosine wave — mathematically simpler, sonically warmer, dramatically more intuitive to program. The CZ-101 offered 8-voice polyphony, MIDI, and battery operation at $495 — a fraction of the DX7's $1,995. It was the first truly portable professional synthesizer.

**Who used it:** Vince Clarke (Erasure, Depeche Mode's "Just Can't Get Enough" era), OMD ("If You Leave"), Howard Jones, and dozens of anonymous Italo Disco producers who prized its ability to produce DX7-adjacent timbres on a nightclub musician's budget. The CZ-101 appears on more Italo Disco records than any other single synthesizer — its breathy pads and cutting leads defined the genre's sonic palette.

**How it lives in OBSIDIAN:** The entire PD core is Casio's insight extended. Where Casio offered 8 fixed distortion shapes, OBSIDIAN offers a continuous 2D morphable space. Where Casio had single-stage PD, OBSIDIAN cascades two stages with cross-modulation. The CZ-101 proved that phase distortion produces more intuitive timbres than FM with fewer parameters. OBSIDIAN honors this by keeping the core interaction to 8 parameters — maximum musical result from minimum complexity.

### Ghost 2: The Cristal Baschet (1952) — The Glass Voice

**The instrument:** Brothers Bernard and François Baschet, working in their Paris workshop, created an acoustic instrument using glass rods as exciters and large flame-shaped aluminum panels as radiators. The player rubs the glass rods with wet fingers; vibrations travel through metal bridges to the aluminum radiators, which project the sound acoustically with no electronics. The tone is pure, crystalline, and capable of extraordinary sustain — a glass rod, once singing, can sustain indefinitely with continuous fingertip friction.

**Who used it:** Tom Waits (*Rain Dogs*, 1985), Björk, and the soundtrack to Jean-Pierre Jeunet's *Amélie* (2001). Jacques and François Baschet built fewer than 30 Cristal Baschets in their lifetimes. Most survive in museums or private collections. Hearing one performed live is a rare event.

**How it lives in OBSIDIAN:** The Stiffness Engine. When `obsidian_stiffness` rises above 0.3, the overtone structure enters the same territory as vibrating glass — inharmonic partials stretched above the harmonic series, each partial sustaining independently. The Cristal Baschet's timbral character — simultaneously warm and crystalline, organic and mineral — is exactly the aesthetic target for OBSIDIAN's mid-to-high stiffness range.

### Ghost 3: Benjamin Franklin's Glass Armonica (1761) — The Banned Crystal

**The instrument:** Franklin, attending a concert of musical glasses in London, designed a mechanized version: 37 glass bowls of graduated sizes, nested together and mounted horizontally on a spindle turned by a foot treadle. The player touched the spinning rims with moistened fingers, producing sustained crystalline tones with seamless dynamic control. Mozart composed his Adagio and Rondo K. 617 for it. Beethoven wrote a melodrama with glass armonica. It was the most fashionable instrument in late 18th-century Europe.

Then it was banned. Multiple European cities restricted or prohibited glass armonica performances. The sustained crystalline tones were blamed for nervous disorders, depression, marital strife, and "excessive melancholy" in both performers and audiences. Marianne Kirchgessner, the instrument's greatest virtuoso (who was blind), died at 39 — attributed by contemporaries to the glass armonica's effects on her nervous system. Modern analysis suggests lead content in the glass was the actual culprit, not the sound itself. But the reputation stuck, and the instrument vanished from concert life for over a century.

**How it lives in OBSIDIAN:** The Glass Armonica is proof that crystalline tones have extraordinary emotional power — enough to be perceived as *dangerous*. OBSIDIAN's Stiffness Engine at high values (0.7–1.0) produces the same psychoacoustic signature: sustained, pure, inharmonic tones with slow beating between adjacent partials. The Glass Armonica was banned because its sound was too beautiful to bear. OBSIDIAN makes that sound programmable.

### Ghost 4: The Ensoniq Fizmo (1998) — The Cult Failure

**The instrument:** Ensoniq's final synthesizer before being absorbed by Creative Labs. The Fizmo used "Transwaves" — evolving wavetables swept by internal LFOs — through aggressive resonant filters with an interface designed for real-time performance. It produced sounds variously described as "liquid crystal," "frozen light," and "digital aurora." It was a commercial disaster: Ensoniq marketed it as a live performance tool in an era of studio-bound production, priced it at $1,299 against a Korg Triton, and included a user interface that baffled reviewers.

The Fizmo sold poorly. Ensoniq dissolved within a year. But the instrument became a cult object — sought after by producers who discovered that its Transwave engine produced crystalline, evolving timbres no other synth could replicate. Fizmo patches from 1998 circulate today in vaporwave and synthwave production communities, prized for their "too-perfect-to-be-analog" quality.

**How it lives in OBSIDIAN:** The Fizmo proved that the vaporwave aesthetic — crystalline, evolving, slightly uncanny — requires a synthesis engine specifically designed for it. The Fizmo stumbled onto this accidentally through Transwaves. OBSIDIAN achieves it intentionally through phase distortion cascading with cross-modulation: timbres that evolve continuously, that shimmer with internal motion, that occupy the uncanny valley between digital precision and organic warmth.

---

## 5. The Cultural Lens: City Pop + Italo Disco

### City Pop (Japan, 1978–1988)

City Pop is what happened when Japanese musicians, producers, and engineers absorbed American AOR, funk, and disco through the filter of Japanese aesthetic perfectionism. Tatsuro Yamashita, Mariya Takeuchi, Taeko Ohnuki, Toshiki Kadomatsu, and Miki Matsubara created music that was simultaneously more polished and more emotionally direct than its American sources. The production quality was obsessive — Japanese studios invested in the latest equipment (SSL consoles, Lexicon reverbs, DX7, Prophet-5, CZ-101) and their engineers had the budget and cultural mandate to spend weeks mixing a single track.

City Pop is *cultural phase distortion*. American pop is the input waveform. Japanese aesthetic sensibility is the distortion function. The output has the same fundamental structure (verse-chorus, 4/4, major keys) but radically different harmonic character — more sophisticated chord voicings (borrowed from jazz), more crystalline production (borrowing from Ryuichi Sakamoto's electronic experiments), more emotional directness (borrowing from enka tradition).

City Pop became the foundational sample source for vaporwave in the early 2010s. Macintosh Plus's "Floral Shoppe" (2011) — the ur-vaporwave album — is built primarily from slowed-down City Pop and smooth jazz. The crystalline production quality of City Pop, when slowed to 60–70% speed, produces exactly the uncanny, nostalgic, slightly melancholic timbre that defines vaporwave. OBSIDIAN's phase distortion at low depth values — warm, shimmering, almost-but-not-quite analog — is the synthesis equivalent.

### Italo Disco (Italy, 1977–1992)

Where City Pop filtered American pop through Japanese perfectionism, Italo Disco filtered it through Italian melodic sensibility. Giorgio Moroder (Munich Machine, Donna Summer's "I Feel Love"), Bobby Orlando (Divine, Pet Shop Boys' early productions), Kano, Gazebo, Den Harrow, and hundreds of anonymous producers created electronic dance music that prioritized *melody* above all else.

Italo Disco producers, often working with limited budgets, gravitated toward affordable synths: the Juno-106, the CZ-101, the DX7 (when they could afford it), the Korg Poly-800. The CZ-101 was a particular favorite — its phase distortion pads had a warmth and immediacy that sat perfectly in Italo Disco's arrangements, and its $495 price point meant that even small studios in Milan, Turin, and Rimini could afford one.

The Italo Disco aesthetic — lush pads, cutting leads, sequenced arps, emotional chord progressions — is the exact sonic territory OBSIDIAN is designed to dominate. Phase distortion was literally the sound of Italo Disco. OBSIDIAN resurrects that connection with 40 years of accumulated potential.

### The Convergence

City Pop and Italo Disco are the two parents of vaporwave — one Eastern, one Western, both taking American pop music and running it through a cultural distortion function that produced something crystalline and slightly uncanny. They converge in OBSIDIAN's aesthetic territory: melodic, shimmering, emotionally resonant, technologically precise, occupying the uncanny valley between human and machine.

---

## 6. XOmnibus Integration

### 6.1 Macro Mapping

| Macro | Primary Target | Secondary Target | Musical Effect |
|-------|---------------|-----------------|----------------|
| CHARACTER | `obsidian_distX` | `obsidian_depth` | Timbre sweep: pure sine → rich FM-like harmonics |
| MOVEMENT | `obsidian_crossmod` | LFO1 depth | Static crystal → breathing cascade |
| COUPLING | Coupling input gain | `obsidian_cascade` | External engine influence on timbre |
| SPACE | `obsidian_stiffness` | `obsidian_stereo` + reverb send | Harmonic mono → inharmonic wide crystal |

### 6.2 MegaCouplingMatrix Compatibility

**Emits:**
- `PD_HARMONICS` — Real-time spectral content of the PD cascade output as a continuous modulation signal. Rich in harmonic detail.
- `STIFFNESS_STATE` — Current inharmonicity coefficient as a normalized [0, 1] control signal.
- `CRYSTAL_ENVELOPE` — The PD depth envelope output, broadcasting the engine's timbral "shape" to other engines.

**Accepts:**
- `AudioToFM` — External audio modulates the distortion depth, creating FM-like interaction between PD and the source signal.
- `AmpToFilter` — External amplitude drives formant intensity or stiffness parameter.
- `EnvToMorph` — External envelope drives distortion function position (X, Y sweep).
- `RhythmToBlend` — External rhythmic triggers sync the PD depth envelope to tempo.

### 6.3 PlaySurface Interaction Model

**Pad Mode:**
- X-axis: `obsidian_distX` — Harmonic Density sweep
- Y-axis: `obsidian_distY` — Spectral Tilt sweep
- Pressure (Z): `obsidian_depth` — Distortion intensity (light touch = pure, hard press = fully distorted)

**Fretless Mode:**
- X-axis: Continuous pitch (mandatory for melodic engine)
- Y-axis: `obsidian_stiffness` — Slide up = more inharmonic/glassy
- Pressure (Z): `obsidian_crossmod` — Pressing harder activates cascade cross-modulation

**Drum Mode:**
- X-axis: Pad assignment by distortion function preset (8 zones = 8 timbres)
- Y-axis: `obsidian_cascade` — Vertical position controls cascade depth
- Pressure (Z): `obsidian_depth` — Velocity = distortion intensity

---

## 7. Preset Archetypes

### 7.1 Crystal Lead
`distX=0.6, distY=0.7, depth=0.8, stiffness=0.15, cascade=0.4, crossmod=0.2, formant=0.1, stereo=0.3`

Cutting, present lead with slight piano-like warmth from low stiffness. Moderate cascade adds harmonic complexity without losing clarity. Sits forward in any mix. The DX7 Rhodes attack with sustained PD warmth.

### 7.2 Glass Pad
`distX=0.3, distY=0.4, depth=0.5, stiffness=0.5, cascade=0.6, crossmod=0.4, formant=0.3, stereo=0.7`

Shimmering, wide pad with glass-like inharmonicity. Cross-modulation creates slow internal breathing. Formant network adds subtle vocal quality — the pad "sings." Wide stereo from phase divergence. The vaporwave pad.

### 7.3 Obsidian Arp
`distX=0.8, distY=0.6, depth=0.9, stiffness=0.05, cascade=0.7, crossmod=0.1, formant=0.0, stereo=0.4`

Dense harmonic arp with near-perfect harmonicity (low stiffness). Full cascade for maximum spectral richness. Fast PD envelope decay creates percussive pluck character. Zero formant — pure crystalline PD.

### 7.4 Volcanic Bell
`distX=0.5, distY=0.5, depth=0.7, stiffness=0.8, cascade=0.3, crossmod=0.0, formant=0.0, stereo=0.5`

Bell-like inharmonic tone with long decay. High stiffness stretches partials into metallic territory. No cascade or cross-mod — pure single-stage PD with extreme inharmonicity. The Glass Armonica in digital form.

### 7.5 City Pop Rhodes
`distX=0.45, distY=0.55, depth=0.65, stiffness=0.2, cascade=0.5, crossmod=0.15, formant=0.0, stereo=0.35`

Warm electric piano with the characteristic "tine" brightness from moderate PD depth. Light cascade adds the characteristic velocity-dependent harmonic bloom. Piano stiffness (0.2) gives authentic inharmonic warmth. The sound of 1983 Tokyo.

### 7.6 Italo Lead
`distX=0.7, distY=0.8, depth=0.85, stiffness=0.1, cascade=0.6, crossmod=0.3, formant=0.15, stereo=0.5`

Bright, emotional lead with subtle formant presence (the "singing" quality). High spectral tilt for brightness, full depth for harmonic richness. Cross-modulation adds subtle movement — the lead *breathes*. The sound of Den Harrow's "Future Brain."

---

## 8. CPU Analysis

### 8.1 Per-Sample Cost

| Component | Operations per Sample | Notes |
|-----------|----------------------|-------|
| Phase accumulator | 2 add | Standard V/Oct |
| Distortion LUT lookup (Stage 1) | 4 multiply + 4 add | Bilinear interpolation in 2D |
| Cosine evaluation | 1 LUT lookup | Pre-computed 4096-point table |
| Stiffness warp | 2 multiply + 1 sqrt | Per-partial stretch |
| Stage 2 cascade | 4 multiply + 4 add | Same as Stage 1 |
| Cross-modulation | 2 multiply | Depth + rate mod |
| Formant filters (4×) | 16 multiply + 8 add | 4 × 2-pole bandpass |
| Stereo divergence | 4 multiply + 4 add | Second LUT lookup for R channel |
| **Total per sample** | **~34 multiply + ~22 add** | |

### 8.2 Voice Budget

At 44.1kHz, 512-sample block, 16 voices:
- Per-voice cost: ~34 multiply-adds per sample × 44100 = ~1.5M ops/sec
- 16 voices: ~24M ops/sec
- M1 single-core throughput: ~3.2 GFLOPS
- **CPU usage: ~0.75%** per engine instance

This is extremely conservative. Even with control-rate modulation overhead, OBSIDIAN should not exceed **8%** of a single core under any configuration. This makes it the lightest engine in the entire XOmnibus gallery.

### 8.3 Memory

- Distortion LUT: 64 × 64 × 1024 × 4 bytes = 16 MB (shared across all voices, loaded once in `prepare()`)
- Cosine LUT: 4096 × 4 bytes = 16 KB
- Per-voice state: ~256 bytes (phase accumulators, filter states, envelope states)
- **Total: ~16.3 MB** (dominated by distortion LUT; could be reduced to 4 MB with 32×32×512 grid and minimal quality loss)

---

## 9. Implementation Notes

### 9.1 Anti-Aliasing

Phase distortion can produce aliasing when the distortion function causes the effective phase velocity to exceed Nyquist. Mitigation strategies:

1. **Distortion function bandwidth limiting:** Pre-filter the 2D distortion LUT to ensure no distortion function produces phase velocities exceeding 2× the fundamental frequency. This is enforced during LUT generation in `prepare()`.
2. **Oversampling at high depth:** When `obsidian_depth > 0.85`, enable 2× oversampling on the PD stages (not the formant filters). This adds ~50% to the PD cost but keeps aliasing inaudible.
3. **PolyBLEP on hard transitions:** If the distortion function produces near-discontinuous phase jumps, apply PolyBLEP correction at the transition points.

### 9.2 Denormal Protection

All filter states (formant bandpass filters, DC blocker) include denormal flushing:

```cpp
inline float flushDenormal(float x)
{
    return (std::abs(x) < 1e-15f) ? 0.0f : x;
}
```

Applied after every IIR filter tick. Mandatory for the 4-formant array where feedback coefficients can drive filter states toward denormal territory during quiet passages.

### 9.3 Parameter Smoothing

All audio-rate parameters (`obsidian_depth`, `obsidian_distX`, `obsidian_distY`) use one-pole smoothing with a 5ms time constant to prevent zipper noise during automation:

```cpp
smoothedValue += (targetValue - smoothedValue) * smoothingCoeff;
// smoothingCoeff = 1.0 - exp(-2π × (1.0 / 0.005) / sampleRate)
```

### 9.4 Thread Safety

- Distortion LUT computed in `prepare()` on the message thread. Pointer-swapped to audio thread via atomic.
- No heap allocation on the audio thread. All voice state pre-allocated in voice pool.
- ParamSnapshot pattern: all parameter pointers cached once per block.

---

*Architecture spec owner: XO_OX Designs | Engine: OBSIDIAN | Next action: Phase 1 — Parameter Architecture*
