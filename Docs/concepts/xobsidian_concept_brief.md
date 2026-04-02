# XObsidian — Concept Brief (Retrospective)

*Retrospective Phase 0 | March 2026 | XO_OX Designs*
*Enriching the fourth generation with the aquatic mythology*

---

## Identity

**XO Name:** XObsidian
**Gallery Code:** OBSIDIAN
**Accent Color:** Crystal White `#E8E0D8`
**Parameter Prefix:** `obsidian_`
**Engine Dir:** `Source/Engines/Obsidian/`

**Thesis:** Phase distortion synthesis — Casio's CZ-series method resurrected with 40 years of missed evolution. Volcanic glass formed under impossible pressure. Beautiful surfaces hiding razor-sharp edges.

---

## Aquatic Identity

Volcanic glass. Formed at the boundary where molten rock meets deep ocean water — instant crystallization under extreme pressure. The surface is smooth, reflective, beautiful. The edge is the sharpest thing in nature — sharper than any surgical steel, capable of cutting at the molecular level. XObsidian exists at this boundary: the phase distortion function is the crystallization process itself, molten waveform energy forced through a transfer function that freezes it into shape. At low depth, the result is pristine — glassy cosine tones with subtle harmonic enrichment. Push the depth and the glass fractures, revealing serrated harmonics that cut through any mix.

The 2D distortion space (density x tilt) is the cross-section of the glass: density controls how many crystalline structures form (harmonic density), tilt controls their orientation (spectral weighting). Move through this 32x32 grid and you traverse the entire landscape of phase distortion — from the rounded warmth of gently warped cosines to the bristling complexity of fully distorted spectra. The distortion depth ADSR envelope means this traversal happens dynamically within each note — a sound that begins pristine and crystallizes into complexity, or begins fractured and smooths as the envelope decays.

The crystal white accent is the glass itself — pale, cold, deceptively elegant. Hold a piece of obsidian and you see your reflection. Run your finger along the edge and you bleed. That duality — beauty and danger in the same material — is the entire sonic identity. The Euler-Bernoulli stiffness engine adds inharmonicity to the partial series, stretching the harmonics the way physical stiffness stretches the overtones of a struck bar or bell. The 4-formant resonance network sculpts vowel-like peaks into the spectrum. The stereo phase divergence splits the crystallization across channels, creating a width that comes from the sound itself rather than processing.

---

## Polarity

**Position:** The Deep — volcanic seabed, where magma meets water under crushing pressure
**feliX-Oscar balance:** 75% Oscar / 25% feliX — Oscar-leaning deep. The glass is formed in Oscar's territory, but its cutting edge carries feliX's precision. When pushed hard, Obsidian can cut like feliX from the deep.

---

## DSP Architecture (As Built)

```
COSINE OSCILLATOR
├── Phase accumulator at MIDI pitch frequency
├── 1024-entry cosine LUT with linear interpolation
│
▼
PHASE DISTORTION STAGE 1
├── 2D distortion LUT: 32x32 density/tilt grid, 512 phase samples
├── 2 MB shared LUT (bilinear interpolation across density/tilt/phase)
├── Distortion depth controlled by PD ADSR envelope
├── LFO1 → density modulation, LFO2 → tilt modulation
│
▼
PD STAGE 2 CASCADE
├── Second phase distortion pass with cross-modulation from Stage 1
├── Stage 1 output modulates Stage 2 phase position
├── Cascade amount parameter controls Stage 2 blend
│
▼
EULER-BERNOULLI STIFFNESS ENGINE
├── 16-partial inharmonic series
├── Ratio: f_n = n * sqrt(1 + B * n^2) (Euler-Bernoulli beam equation)
├── B coefficient: exponentially mapped [0, 0.15]
├── Shifts harmonics progressively sharp (piano-like inharmonicity)
│
▼
STEREO PHASE DIVERGENCE
├── L/R channels use offset density/tilt coordinates
├── Divergence amount controls stereo decorrelation
├── Width from the distortion itself, not processing
│
▼
4-FORMANT RESONANCE NETWORK
├── 4 parallel CytomicSVF bandpass filters
├── Formant parameter sweeps center frequencies
├── Formant intensity controls blend with dry signal
├── Vowel-like spectral shaping
│
▼
MAIN FILTER
├── CytomicSVF lowpass, key-tracked
├── Cutoff + resonance + coupling modulation
│
▼
OUTPUT
├── Amp ADSR envelope, velocity scaling
├── Mono/Legato/Poly8/Poly16 voice modes with LRU stealing + 5ms crossfade
├── 2 LFOs (Sine/Tri/Saw/Square/S&H) — LFO1→density, LFO2→tilt
├── 4 macros: CHARACTER (density+depth), MOVEMENT (crossMod), COUPLING (cascade), SPACE (stiffness+stereo)
├── Full coupling: AudioToFM→PD depth, AmpToFilter→cutoff, EnvToMorph→density/tilt
```

**Key DSP structures:** 2D phase distortion LUT (32x32x512, bilinear interpolated), `ObsidianADSR` (amp + PD depth envelopes), `ObsidianLFO` (5-shape modulation), `ObsidianVoice` (dual-phase accumulators + formant network + stiffness partials), `CytomicSVF` (formant BPs + main LP filter).

**Voice count:** 16 maximum (Poly16 mode), 8 default.

**LUT memory:** 2 MB (shared across all voices — zero per-voice allocation for the distortion tables).

---

## Signature Sound

XObsidian produces sounds with a crystalline clarity that no subtractive or FM synth can match. At low distortion depth, the output is pure, glassy, almost bell-like — cosine oscillators with the subtlest harmonic enrichment from the phase distortion function. As depth increases, the harmonics emerge with a precision that sounds crystalline rather than gritty — each harmonic placed exactly where the transfer function puts it, not smeared by a filter or blurred by wavetable interpolation. The two-stage cascade with cross-modulation creates a complexity ceiling far above single-pass PD, producing spectra with the density of FM synthesis but the controllability of a morphable 2D grid. The Euler-Bernoulli stiffness adds a metallic, bell-like quality — partials that stretch progressively sharp, creating timbres that ring like struck glass or volcanic crystal singing bowls. At maximum depth with high stiffness and formant intensity, the sound becomes something dangerous: serrated, inharmonic, cutting — the razor edge of the glass.

---

## Coupling Thesis

Obsidian gives sharp, defined harmonics — clean when controlled, cutting when pushed. His post-filter stereo output via `getSampleForCoupling` provides spectrally rich material that retains its crystalline character even when processed by other engines. Feed Obsidian into Overdub's echo and the tape delay preserves the glass-like transients. Feed it into Organon and the metabolic engine has rich, well-defined entropy to consume.

He receives amplitude and envelope coupling that pushes him between beauty and danger. `AudioToFM` modulates PD depth — external audio drives the crystallization process. `AmpToFilter` opens the filter proportionally to external energy. `EnvToMorph` sweeps the density/tilt position, allowing another engine's envelope to navigate Obsidian's entire timbral space.

Pair him with warm engines (Oblong, OddOscar) to soften his edges — their amplitude coupling gently modulates his filter, keeping the glass smooth. Pair him with aggressive engines (Overbite, Onset) to weaponize the edge — their transients drive PD depth spikes that create cutting harmonic bursts on every hit. The duality is the instrument: same engine, same patch, beauty or danger depending on who it couples with.

---

## Preset Affinity

| Foundation | Atmosphere | Entangled | Prism | Flux | Aether |
|-----------|-----------|-----------|-------|------|--------|
| Medium | Medium | Medium | High | Medium | Low |

---

*XO_OX Designs | XObsidian — Beautiful surfaces hiding razor-sharp edges*
