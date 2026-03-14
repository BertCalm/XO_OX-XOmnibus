# XOrigami — Concept Brief (Retrospective)

*Retrospective Phase 0 | March 2026 | XO_OX Designs*
*Enriching the fourth generation with the aquatic mythology*

---

## Identity

**XO Name:** XOrigami
**Gallery Code:** ORIGAMI
**Accent Color:** Vermillion Fold `#E63946`
**Parameter Prefix:** `origami_`
**Engine Dir:** `Source/Engines/Origami/`

**Thesis:** Spectral folding FFT synthesis — sound enters the frequency domain and is folded, mirrored, rotated, and stretched like paper. Surface tension made audible. Water folding on itself.

---

## Aquatic Identity

Surface tension. The invisible membrane where water meets air — strong enough to hold a needle, delicate enough to break with a breath. Watch water closely at the surface and you see it fold: a wave crests and the leading edge curls over itself, the surface doubling back, creating a moment where two layers of water occupy the same space before collapsing into spray. XOrigami does this to sound. Audio enters the frequency domain via STFT, and four spectral operations fold the magnitude spectrum the way water folds at its surface — precisely, delicately, with sharp creases that create unexpected harmonics.

The FOLD operation is the primary gesture: the spectrum reflects at a fold point, bins above the threshold mirroring back below it, constructively interfering with the existing content. This is the exact physics of a wave folding over — energy reflecting back on itself, amplifying where the fold coincides with existing structure. MIRROR creates bilateral symmetry around the fold point — the surface frozen in perfect stillness, a reflection so clean you cannot tell which side is real. ROTATE shifts the entire magnitude spectrum circularly, sliding harmonics up or down the frequency axis like water sliding along the surface tension membrane. STRETCH warps the frequency axis nonlinearly, compressing some regions and expanding others — the way surface tension deforms under pressure, not uniformly but with the complex elasticity of a liquid membrane.

The vermillion accent is a paper crane floating on the current — bold color on fragile structure. Origami lives in feliX's shallows, where the surface is visible, where the folding happens in light. The spectral freeze function captures a single STFT frame and holds it — a droplet suspended at the moment of maximum surface tension, frozen in mid-fold. The fold count parameter cascades the operation, each successive fold slightly offset from the last, creating dense, layered spectral structures like nested paper cranes or water folding on itself multiple times before breaking.

---

## Polarity

**Position:** Sunlit Shallows — the surface tension membrane, where water meets air
**feliX-Oscar balance:** 65% feliX / 35% Oscar — feliX-leaning shallows. The precision of the fold is feliX's contribution; the spectral depth and density it creates when cascaded carries Oscar's complexity. At low fold depth, Origami is sharp and precise. At high fold depth with multiple cascades, the spectral content becomes dense and strange — feliX's surface folding into Oscar's depth.

---

## DSP Architecture (As Built)

```
INTERNAL OSCILLATOR BANK
├── Saw oscillator (naive — anti-aliased by STFT process)
├── Square oscillator
├── Noise generator (LCG)
├── OscMix: [0, 0.5] saw→square crossfade, [0.5, 1.0] square→noise crossfade
├── Source parameter: blend internal oscillator with coupling input
│
▼
2048-POINT STFT ANALYSIS
├── 2048-sample Hann-windowed input ring buffer per voice
├── 4x overlap (hop size = 512 samples)
├── Radix-2 DIT FFT with pre-computed bit-reversal table + twiddle factors
├── Magnitude/phase extraction for 1025 bins (DC to Nyquist)
├── Phase vocoder: instantaneous frequency tracking via inter-hop phase deviation
│
▼
SPECTRAL OPERATIONS (4 modes, cascaded 1-4x)
├── FOLD: Reflect spectrum at fold point (constructive interference below fold)
├── MIRROR: Bilateral symmetry around fold point (magnitude averaging)
├── ROTATE: Circular shift of magnitude spectrum (frequency sliding)
├── STRETCH: Nonlinear frequency-axis warping (compress/expand regions)
├── 3-point triangular smoothing on folded magnitude (artifact reduction)
├── Fold count: 1-4 cascaded passes with 0.1 fold-point offset per cascade
│
▼
SPECTRAL FREEZE
├── Captures current analysis frame (magnitude + phase)
├── Holds frozen frame indefinitely when active
├── Coupling-triggerable via RhythmToBlend (rhythm → freeze toggle)
│
▼
STFT RESYNTHESIS
├── Magnitude/phase → complex spectrum reconstruction
├── Conjugate mirror for negative frequencies
├── Inverse FFT (conjugate + forward FFT + scale)
├── Hann window + overlap-add with 2/3 normalization factor
│
▼
OUTPUT
├── CytomicSVF lowpass post-filter (post-FFT smoothing)
├── Amp ADSR + Fold ADSR envelopes (fold depth modulated by fold envelope)
├── 2 LFOs: LFO1 → fold point modulation, LFO2 → rotate modulation
├── Mono/Legato/Poly4/Poly8 voice modes with LRU stealing + 5ms crossfade
├── Deterministic per-voice stereo panning (voice index → pan position)
├── Denormal protection on all accumulator and FFT outputs
├── 4 macros: FOLD (foldPoint+foldDepth), MOTION (rotate+LFO1 depth),
│             COUPLING (source blend), SPACE (foldCount+stretch)
├── Coupling: AudioToWavetable (replace source), AmpToFilter (→fold depth),
│             EnvToMorph (→fold point), RhythmToBlend (→freeze trigger)
```

**Key DSP structures:** `SpectralFrame` (magnitude/phase/instFreq arrays for 1025 bins), `OrigamiVoice` (STFT pipeline with analysis/resynthesis buffers, overlap-add accumulator), hand-rolled radix-2 DIT FFT with pre-computed twiddle factors, 4 spectral operations (`applySpectralFold`, `applySpectralMirror`, `applySpectralRotate`, `applySpectralStretch`), `OrigamiADSR`, `OrigamiLFO`.

**Voice count:** 8 maximum (Poly8 mode), 4 default. Each voice carries a full 2048-point STFT pipeline.

**Memory per voice:** ~50 KB (input ring, output accumulator, FFT working buffers, spectral frames).

---

## Signature Sound

XOrigami produces a class of timbres impossible through time-domain methods. The spectral fold creates dense metallic textures when the fold point sits in the upper harmonics — energy reflecting back into the lower spectrum, constructively interfering with the fundamental and low partials to create a shimmering, resonant quality. At low fold depth, the effect is subtle — a slight thickening of the harmonic content, like light catching on the edge of a paper fold. At high fold depth with multiple cascades, the spectrum becomes something entirely new: dense, inharmonic, crystalline structures where every bin carries reflected energy from multiple fold passes. The spectral freeze captures this complexity and holds it — a sustained, evolving texture that preserves the exact spectral fingerprint of one moment. The phase vocoder ensures that frozen frames retain their pitch accuracy, creating sustained tones with the internal complexity of the folded spectrum. The ROTATE operation produces unique frequency-sliding effects — not pitch shifting, but spectral translation that moves harmonic content up or down the frequency axis while preserving the overall spectral shape.

---

## Coupling Thesis

Origami gives precisely shaped harmonics — spectral content that has been folded and sculpted in the frequency domain. His post-fold stereo output provides material with a complexity and precision that time-domain engines cannot match. Feed Origami into Overdub and the tape echo preserves the crystalline spectral folds. Feed it into Ouroboros as velocity injection and the chaotic attractor absorbs folded harmonics into its trajectory.

He receives coupling in four ways, each mapping to a different aspect of the fold:

- **AudioToWavetable**: Replace the internal oscillator entirely — fold someone else's spectrum. Feed OddOscar's wavetable morph into Origami and fold the continuously transforming spectrum. Feed Onset's drum hits and fold their transient spectra into sustained, ringing tones.
- **AmpToFilter**: External amplitude drives fold depth — louder input means deeper folds, more reflected energy, denser harmonics. Gentle input creates delicate paper folds. Aggressive input tears the paper.
- **EnvToMorph**: External envelope sweeps the fold point across the spectrum — another engine's ADSR navigating Origami's entire timbral range.
- **RhythmToBlend**: Rhythmic coupling triggers spectral freeze — Onset's drum pattern captures and holds spectral snapshots in time with the beat.

The fold point parameter is the crease. Everything about Origami's coupling behavior hinges on where the fold happens and how deep it goes.

---

## Preset Affinity

| Foundation | Atmosphere | Entangled | Prism | Flux | Aether |
|-----------|-----------|-----------|-------|------|--------|
| Medium | Low | Medium | High | Medium | Low |

---

*XO_OX Designs | XOrigami — The spectrum, folded at the surface where water meets light*
