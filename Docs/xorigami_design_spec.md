# XO-Origami — Phase 1 Architecture Specification

**Engine:** XO-Origami (Spectral Folding Synthesis)
**Short Name:** ORIGAMI
**Engine ID:** `"Origami"`
**Parameter Prefix:** `origami_`
**Accent Color:** Vermillion Fold `#E63946`
**Max Voices:** 8 (each voice maintains independent FFT state)
**CPU Budget:** <12% single-engine, <18% dual-engine
**Date:** 2026-03-11

---

## 1. Product Identity

**Thesis:** "XO-Origami treats sound as a sheet of spectral paper — folding, mirroring, rotating, and stretching the frequency spectrum through geometric transformations that produce timbres impossible in the time domain."

**Sound family:** Texture / Pad / Lead / Experimental

**Unique capability:** Real-time geometric transformation of spectral content via STFT analysis-resynthesis. Every existing XOmnibus engine works in the time domain — oscillators generating waveforms sample by sample. ORIGAMI operates in the frequency domain, applying spatial geometry to the spectrum itself. It answers the question: "What does a sound look like when you fold its spectrum in half?"

**Personality in 3 words:** Kaleidoscopic, geometric, alive.

**Gallery gap filled:** No existing engine manipulates sound in the spectral domain. ODDOSCAR morphs wavetables (time domain). OPAL scatters grains (time domain). ORBITAL stacks partials (frequency domain, but additive — constructive only). ORIGAMI is the first *transformative* spectral engine — it takes existing spectral content and geometrically reshapes it.

---

## 2. Signal Flow Architecture

```
┌───────────────────────────────────────────────────────────────────────────┐
│                         XO-Origami Voice                                   │
│                                                                           │
│  ┌─────────────────────────────────────────────────────────────────────┐  │
│  │  SOURCE STAGE                                                       │  │
│  │                                                                     │  │
│  │  Internal Oscillator Bank ──┐                                       │  │
│  │  (Saw + Square + Noise)     ├──► Source Mixer ──► Analysis Window   │  │
│  │  Coupling Input ────────────┘                                       │  │
│  │                                                                     │  │
│  │  Window: 2048 samples, Hann, 4× overlap (hop = 512)                │  │
│  └──────────────────────────────────────┬──────────────────────────────┘  │
│                                          │                                │
│  ┌──────────────────────────────────────▼──────────────────────────────┐  │
│  │  STFT ANALYSIS                                                      │  │
│  │                                                                     │  │
│  │  2048-point FFT ──► Magnitude + Phase extraction                    │  │
│  │  1024 frequency bins (DC to Nyquist)                                │  │
│  │  Phase vocoder tracking for pitch-coherent resynthesis              │  │
│  └──────────────────────────────────────┬──────────────────────────────┘  │
│                                          │                                │
│  ┌──────────────────────────────────────▼──────────────────────────────┐  │
│  │  SPECTRAL FOLD ENGINE (up to 4 cascaded operations)                 │  │
│  │                                                                     │  │
│  │  Operation 1 ──► Operation 2 ──► Operation 3 ──► Operation 4       │  │
│  │                                                                     │  │
│  │  Each operation is one of:                                          │  │
│  │    FOLD:    Reflect spectrum at fold point                          │  │
│  │    MIRROR:  Bilateral symmetry around fold point                    │  │
│  │    ROTATE:  Circular shift of spectral content                     │  │
│  │    STRETCH: Nonlinear frequency-axis warping                       │  │
│  │    FREEZE:  Hold current spectral frame (bypass further input)     │  │
│  │                                                                     │  │
│  │  Fold Point: 0.0 (DC) to 1.0 (Nyquist) — modulatable              │  │
│  │  Fold Depth: 0.0 (no fold) to 1.0 (complete fold)                  │  │
│  │  Cascade Count: 1–4 (each fold multiplies spectral complexity)     │  │
│  └──────────────────────────────────────┬──────────────────────────────┘  │
│                                          │                                │
│  ┌──────────────────────────────────────▼──────────────────────────────┐  │
│  │  STFT RESYNTHESIS                                                   │  │
│  │                                                                     │  │
│  │  Modified Magnitude + Phase ──► IFFT ──► Overlap-Add               │  │
│  │  Phase vocoder correction for artifact-free reconstruction          │  │
│  │  Output window: 2048 samples, Hann, normalized for 4× OLA          │  │
│  └──────────────────────────────────────┬──────────────────────────────┘  │
│                                          │                                │
│                                          ▼                                │
│                                Output Cache [L, R]                        │
│                                                                           │
└───────────────────────────────────────────────────────────────────────────┘
```

### 2.1 Source Stage

ORIGAMI needs spectral content to fold. Two sources:

**Internal Oscillator Bank:** A simple 3-oscillator mixer (sawtooth, square, filtered noise) provides harmonically rich source material for standalone use. The saw provides a full harmonic series. The square provides odd harmonics. The noise provides broadband content for textural folds. Oscillator mix is parameterized — the user can select the spectral "paper" before folding it.

**Coupling Input:** When receiving audio from the MegaCouplingMatrix, the coupling input replaces or blends with the internal oscillators. This is ORIGAMI's primary mode of creative interaction — fold another engine's spectral output. ODYSSEY's Climax bloom, spectrally folded. OBESE's 13-oscillator stack, spectrally mirrored. OBSIDIAN's crystalline harmonics, spectrally rotated.

### 2.2 STFT Analysis

Short-Time Fourier Transform with the following fixed parameters:
- **Window size:** 2048 samples (~46ms at 44.1kHz)
- **Hop size:** 512 samples (4× overlap for artifact-free reconstruction)
- **Window function:** Hann (von Hann) — optimal for overlap-add reconstruction at 4× overlap
- **FFT size:** 2048 (1024 usable frequency bins from DC to Nyquist)
- **Phase vocoder:** Phase differences between successive frames tracked for coherent resynthesis

**Latency:** One full window = ~46ms. Acceptable for pad/texture engine. For lead use, a future "low-latency mode" with 1024-sample window (~23ms) is planned.

### 2.3 Spectral Fold Engine — The Core

The heart of ORIGAMI. Four spectral operations, applied in cascade to the magnitude spectrum:

#### FOLD Operation

Reflects the spectrum at a configurable fold point. Everything above the fold point is mirrored back below it, adding to the existing content:

```
For bin k from fold_bin to N:
    target_bin = 2 * fold_bin - k
    if target_bin >= 0:
        magnitude[target_bin] += magnitude[k] * fold_depth
        magnitude[k] *= (1.0 - fold_depth)
```

Musical effect: Folding at the midpoint creates a spectrum symmetric around a center frequency — like folding a sheet of paper in half. Folding near the top produces a bright, metallic doubling. Folding near the bottom creates dark, bass-heavy reflections. Modulating the fold point sweeps the reflection point through the spectrum, creating timbral animation impossible with any filter.

#### MIRROR Operation

Creates bilateral symmetry around the fold point — unlike FOLD which adds reflected content, MIRROR *replaces* the spectrum with its symmetric version:

```
For bin k = 0 to fold_bin:
    mirror_bin = 2 * fold_bin - k
    if mirror_bin < N:
        magnitude[k] = magnitude[mirror_bin] * mirror_depth
                       + magnitude[k] * (1.0 - mirror_depth)
```

Musical effect: Perfect spectral symmetry produces metallic, bell-like timbres (like the inharmonic spectra of physical bells, which have approximate bilateral spectral symmetry). Sweeping the mirror point creates a "spectral palindrome" effect — the sound becomes its own reflection.

#### ROTATE Operation

Circular shift of the entire magnitude spectrum by a configurable amount:

```
For bin k = 0 to N:
    target_bin = (k + rotate_amount) % N
    rotated_magnitude[target_bin] = magnitude[k]
```

Musical effect: Spectral rotation shifts all frequency content up or down without changing pitch (because phase is preserved). A small rotation creates subtle spectral detuning. A large rotation fundamentally transforms the timbre — bass content appears at treble frequencies and vice versa. This is frequency-domain ring modulation but with finer control.

#### STRETCH Operation

Nonlinear frequency-axis warping using a configurable stretch function:

```
For bin k = 0 to N:
    normalized = k / N  // [0, 1]
    warped = stretchFunction(normalized, stretch_amount)
    target_bin = warped * N
    // Interpolated assignment to avoid spectral gaps
```

Musical effect: Stretch compresses or expands specific frequency regions. A positive stretch compresses the bass and expands the treble (brightening). A negative stretch does the reverse (darkening). At extreme values, the entire spectrum is compressed into a narrow band, producing whistle-like or sub-bass-like tones from any source material.

#### CASCADE

Operations are applied in series: the output of Operation 1 feeds Operation 2, etc. Each fold multiplies the spectral complexity — a single fold creates bilateral structure, two folds create quadrilateral structure, four folds create a kaleidoscopic spectral pattern with 16-fold symmetry.

**Fold count vs. spectral complexity:**
| Folds | Symmetry | Musical Character |
|-------|----------|------------------|
| 1 | Bilateral | Metallic, bell-like |
| 2 | Quadrilateral | Crystalline, gamelan-like |
| 3 | Octagonal | Dense, shimmering |
| 4 | 16-fold (kaleidoscopic) | Abstract, alien, deeply textural |

### 2.4 Spectral Freeze

When activated, FREEZE captures the current spectral frame and holds it indefinitely. Subsequent fold operations are applied to the frozen spectrum rather than to live input. This creates a sustained spectral drone that can be geometrically transformed in real time — the player captures a moment of sound and sculpts it with fold gestures.

FREEZE is implemented as a simple flag that stops the analysis stage from overwriting the magnitude/phase buffers. The fold engine continues to operate on the held data.

---

## 3. Parameter Taxonomy

### 3.1 Core Parameters (8)

| ID | Parameter | Range | Curve | Rate | Description |
|----|-----------|-------|-------|------|-------------|
| `origami_foldPoint` | Fold Point | 0.0–1.0 | Linear | Audio | Position on the frequency axis where folds occur. 0 = DC, 0.5 = midpoint, 1.0 = Nyquist. Primary timbre control. CHARACTER macro target. |
| `origami_foldDepth` | Fold Depth | 0.0–1.0 | Linear | Audio | Intensity of fold operations. 0 = no fold (pass-through). 1 = complete fold. |
| `origami_foldCount` | Fold Count | 1–4 | Stepped | Control | Number of cascaded fold operations. Each fold multiplies spectral complexity. |
| `origami_operation` | Operation Type | 0–3 | Stepped | Control | FOLD (0), MIRROR (1), ROTATE (2), STRETCH (3). Selects the geometric transformation applied. |
| `origami_rotate` | Rotation Amount | -1.0–1.0 | Linear | Audio | Circular shift of spectrum. -1 = full shift down, +1 = full shift up. Used when operation = ROTATE. MOVEMENT macro target. |
| `origami_stretch` | Stretch Amount | -1.0–1.0 | Exponential | Control | Nonlinear frequency warping. Negative = compress treble. Positive = compress bass. |
| `origami_freeze` | Spectral Freeze | 0–1 | Toggle | Control | 0 = live analysis. 1 = freeze current spectral frame. |
| `origami_source` | Source Mix | 0.0–1.0 | Linear | Control | Internal oscillator (0) to coupling input (1). Selects the spectral "paper" to fold. |

### 3.2 Macro Mapping

| Macro | Primary Target | Secondary Target | Musical Effect |
|-------|---------------|-----------------|----------------|
| CHARACTER (M1) | `origami_foldPoint` | `origami_foldDepth` | Sweeps fold point across spectrum — dramatic timbral transformation |
| MOVEMENT (M2) | `origami_rotate` | LFO1 → fold point | Spectral rotation + animated fold point creates kaleidoscopic motion |
| COUPLING (M3) | `origami_source` | Coupling input gain | Controls blend between internal oscillator and external engine |
| SPACE (M4) | `origami_foldCount` | `origami_stretch` + reverb send | From single fold (simple symmetry) to 4-fold kaleidoscope with spatial stretch |

### 3.3 Envelope & Modulation Parameters

| ID | Parameter | Type | Description |
|----|-----------|------|-------------|
| `origami_ampAttack` | Amp Attack | Time | 0ms–10s |
| `origami_ampDecay` | Amp Decay | Time | 0ms–10s |
| `origami_ampSustain` | Amp Sustain | Level | 0–1 |
| `origami_ampRelease` | Amp Release | Time | 0ms–20s |
| `origami_foldEnvAttack` | Fold Env Attack | Time | Fold depth envelope — controls fold animation |
| `origami_foldEnvDecay` | Fold Env Decay | Time | |
| `origami_foldEnvSustain` | Fold Env Sustain | Level | |
| `origami_foldEnvRelease` | Fold Env Release | Time | |
| `origami_lfo1Rate` | LFO 1 Rate | Hz | 0.01–30 Hz |
| `origami_lfo1Depth` | LFO 1 Depth | Level | Modulation depth |
| `origami_lfo1Shape` | LFO 1 Shape | Enum | Sine / Triangle / Saw / Square / S&H |
| `origami_lfo2Rate` | LFO 2 Rate | Hz | 0.01–30 Hz |
| `origami_lfo2Depth` | LFO 2 Depth | Level | |
| `origami_lfo2Shape` | LFO 2 Shape | Enum | Sine / Triangle / Saw / Square / S&H |

### 3.4 Voice Parameters

| ID | Parameter | Description |
|----|-----------|-------------|
| `origami_voiceMode` | Voice Mode | Mono / Legato / Poly4 / Poly8 |
| `origami_glide` | Glide Time | Portamento time (0–2s) |
| `origami_oscMix` | Internal Osc Mix | Saw / Square / Noise balance for internal source |

---

## 4. The Ghosts in ORIGAMI

### Ghost 1: Evgeny Murzin's ANS Synthesizer (1958) — The Spectral Canvas

**The instrument:** Named after the composer Alexander Nikolayevich Scriabin, the ANS was designed by Evgeny Murzin over 20 years in Moscow. It used 720 pure-tone generators (sine waves at chromatic and microtonal intervals) activated by light shining through hand-painted glass plates. The composer painted black marks on clear glass; wherever the glass was opaque, the corresponding sine tone was silenced. The glass plate moved horizontally on a motorized track, creating time-varying spectral patterns.

The ANS was the first instrument to treat the frequency spectrum as a *visual canvas* — a surface to be drawn on, sculpted, and shaped. Composers Edward Artemyev (who scored Tarkovsky's *Solaris* and *Stalker*), Alfred Schnittke, Sofia Gubaidulina, and Edison Denisov all worked with the ANS at the Moscow Experimental Electronic Music Studio. The instrument was never commercially produced; only one was ever built. It survives at the Glinka Museum of Musical Culture in Moscow.

**How it lives in ORIGAMI:** The ANS treated the spectrum as a 2D surface. ORIGAMI extends this: the spectrum is not just a surface to draw on — it's a sheet to *fold*. Where Murzin used painted glass to create spectral shapes, ORIGAMI uses geometric operations to transform them. The ANS was spectral *composition*; ORIGAMI is spectral *origami*.

### Ghost 2: The Fairlight CMI (1979) — The Spectral Editor

**The instrument:** Peter Vogel and Kim Ryrie, working in Sydney, created the Computer Musical Instrument — the first commercial digital sampler with a screen-based spectral editor. "Page D" allowed users to draw waveforms directly on the CRT using a light pen, seeing the time-domain waveform update in real time. More significantly, Page R allowed additive harmonic editing — adjusting individual harmonic amplitudes visually. The Fairlight CMI cost $25,000 (1979 dollars) and was adopted by Peter Gabriel, Kate Bush, Jean-Michel Jarre, and the Art of Noise.

**How it lives in ORIGAMI:** The Fairlight proved that musicians want to *see and manipulate* spectral content directly. Its light-pen interface was the ancestor of every spectral editor since. ORIGAMI's real-time spectrogram UI — showing fold geometry overlaid on the live spectrum — is a direct descendant of Page D. But where the Fairlight let you draw individual harmonics, ORIGAMI lets you fold the entire spectrum in a single gesture.

### Ghost 3: Don Buchla's 296 Spectral Processor (1979) — The Hardware Folder

**The instrument:** Part of the Buchla Music Easel ecosystem, the 296 was a 16-band spectral processor — essentially a graphic equalizer where each band had its own envelope follower and CV input. By patching the envelope followers to cross-modulate other bands, the 296 could create spectral transformations that no simple filter could achieve: frequency-dependent amplitude modulation, spectral gating, and — with creative patching — crude spectral folding.

Buchla designed the 296 as a way to think about sound in the frequency domain within a modular synthesis context, decades before real-time FFT was computationally feasible. It was frequency-domain thinking implemented in analog hardware.

**How it lives in ORIGAMI:** Buchla's 296 was the analog ancestor of spectral folding — 16 bands of frequency-dependent processing with cross-modulation. ORIGAMI replaces the 16 bands with 1024 FFT bins, the analog envelope followers with digital magnitude extraction, and the patch cables with geometric operations. The musical intent is identical: reshape the spectrum as a *structure*, not as a series of filter cuts. Buchla thought in frequency-domain terms in 1979. ORIGAMI makes that thinking literal.

---

## 5. The Cultural Lens: Gamelan

### Javanese and Balinese Gamelan

Gamelan is the ensemble music of Java and Bali, Indonesia — orchestras of bronze metallophones, xylophones, gongs, drums, and flutes that have been central to Javanese and Balinese culture for over a millennium. Gamelan instruments produce *inharmonic* spectra — the partials of a struck bronze key don't follow the Western harmonic series but cluster around formant regions determined by the key's metallurgy, shape, and mode of vibration.

When multiple gamelan instruments play simultaneously, their inharmonic spectra interact in ways that are fundamentally different from Western harmonic instruments. Partials from different instruments fold into each other, creating composite timbres that belong to no single instrument — they exist only in the spectral interference between instruments. This is *spectral folding as cultural practice*.

**Kotèkan** (Balinese interlocking figuration): Two players perform complementary parts — each plays every other note of a fast melodic line, their parts interlocking like the teeth of a zipper. The composite melody exists in neither individual part. This is bilateral spectral symmetry: fold one player's part onto the other, and the full melody appears at the fold line.

**Ombak** (beating): Paired gamelan instruments are deliberately tuned slightly apart, creating a slow beating (ombak, "wave") when played together. The beat rate is considered a core aesthetic parameter — different gamelan sets have different ombak frequencies. This is spectral interference: two nearly-identical spectra offset by a tiny frequency difference, their fold creating a slow amplitude modulation.

**Pelog and Slendro tuning systems:** Gamelan uses non-equal temperaments (pelog: 7 unequal intervals; slendro: 5 approximately equal intervals) that create spectral relationships impossible in 12-tone equal temperament. These tuning systems pre-fold the harmonic series — the intervals themselves create spectral symmetries that don't exist in Western tuning.

ORIGAMI's spectral folding operations produce timbres with the same metallic, inharmonic, multi-layered quality that defines gamelan — not by imitating gamelan instruments, but by applying the same underlying mathematical principle (spectral reflection and interference) to any source material.

---

## 6. XOmnibus Integration

### 6.1 MegaCouplingMatrix Compatibility

**Emits:**
- `SPECTRAL_SHAPE` — The current folded magnitude spectrum as a normalized 64-band representation. Other engines can use this as a spectral filter template.
- `FOLD_POSITION` — Current fold point position (0–1) as a continuous modulation signal.
- `FOLD_SYMMETRY` — Degree of bilateral spectral symmetry (0 = asymmetric, 1 = perfect mirror). Computed from the folded spectrum.

**Accepts:**
- `AudioToWavetable` — External audio becomes the spectral "paper" to fold. Primary coupling mode.
- `AmpToFilter` — External amplitude drives fold depth. Loud = deeply folded, quiet = unfolded.
- `EnvToMorph` — External envelope sweeps fold point. Creates synchronized spectral animation.
- `RhythmToBlend` — External rhythm triggers spectral freeze/unfreeze.

### 6.2 PlaySurface Interaction Model

**Pad Mode:**
- X-axis: `origami_foldPoint` — Sweep fold position across spectrum
- Y-axis: `origami_foldDepth` — Control fold intensity
- Pressure (Z): `origami_foldCount` — Harder = more cascaded folds (1→4)

**Fretless Mode:**
- X-axis: Continuous pitch (internal oscillator frequency)
- Y-axis: `origami_rotate` — Slide up = spectral rotation upward
- Pressure (Z): `origami_stretch` — Press harder = more spectral compression

**Drum Mode:**
- X-axis: Pad assignment by operation type (4 zones × 2 fold points = 8 pads)
- Y-axis: `origami_foldDepth` — Vertical position controls fold intensity
- Pressure (Z): Velocity = fold depth per hit

---

## 7. Preset Archetypes

### 7.1 Paper Crane
`foldPoint=0.5, foldDepth=0.7, foldCount=2, operation=FOLD, rotate=0.0, stretch=0.0, freeze=0, source=0 (saw)`

Clean bilateral fold of a sawtooth spectrum. The fold at midpoint creates perfect spectral symmetry — a metallic, bell-like timbre from a simple saw wave. Two cascade folds produce quadrilateral symmetry reminiscent of gamelan metallophone spectra.

### 7.2 Kaleidoscope
`foldPoint=LFO, foldDepth=0.9, foldCount=4, operation=FOLD, rotate=0.1, stretch=0.0, freeze=0, source=0.3 (saw+square)`

Maximum fold cascade with LFO-animated fold point. The spectrum continuously refolds into kaleidoscopic patterns — 16-fold symmetry in constant motion. Dense, shimmering, deeply textural.

### 7.3 Frozen Bloom
`foldPoint=0.4, foldDepth=0.6, foldCount=2, operation=MIRROR, rotate=0.0, stretch=0.0, freeze=1 (triggered), source=0 (coupling input)`

Spectral freeze captures a moment of coupled audio (e.g., ODYSSEY's Climax bloom), then mirror-folds the frozen spectrum. The result is a sustained, symmetric spectral drone — a captured moment sculpted into geometric perfection.

### 7.4 Ring Shift
`foldPoint=0.0, foldDepth=0.0, foldCount=1, operation=ROTATE, rotate=0.35, stretch=0.0, freeze=0, source=0 (saw)`

Pure spectral rotation — all frequency content shifted upward by 35% of Nyquist. Creates metallic, ring-modulated tones without the intermodulation noise of true ring modulation. The rotation amount is tunable for precise inharmonic control.

### 7.5 Spectral Origami
`foldPoint=0.3, foldDepth=0.8, foldCount=3, operation=FOLD, rotate=0.15, stretch=0.2, freeze=0, source=0.5 (saw+noise)`

Combined fold + rotate + stretch. Three cascaded folds with rotation and stretch create a spectral pattern of extraordinary complexity — dense, metallic, alive with internal motion. The broadband noise source ensures content across the full spectrum for the folds to work with.

---

## 8. CPU Analysis

### 8.1 Per-Block Cost

| Component | Operations per Block (512 samples) | Notes |
|-----------|------------------------------------|-------|
| Source oscillators | ~3000 multiply-adds | 3 oscillators × 512 samples |
| FFT (2048-point) | ~22,000 multiply-adds | Split-radix FFT |
| Magnitude/Phase extraction | ~2048 sqrt + atan2 | Can be approximated |
| Fold operations (4× cascade) | ~16,384 multiply-adds | 4 × 1024 bins × 4 ops |
| Phase vocoder correction | ~4096 multiply-adds | Phase adjustment per bin |
| IFFT (2048-point) | ~22,000 multiply-adds | Split-radix IFFT |
| Overlap-add | ~2048 multiply-adds | Window + accumulate |
| **Total per block** | **~70,000 ops** | |

### 8.2 Voice Budget

At 44.1kHz, 512-sample block, hop size 512:
- FFT/IFFT occurs once per block (not per sample)
- Per-voice cost: ~70,000 ops per block × 86 blocks/sec ≈ 6M ops/sec
- 8 voices: ~48M ops/sec
- M1 single-core throughput: ~3.2 GFLOPS
- **CPU usage: ~1.5%** per engine instance

This is conservative. With Accelerate framework's vDSP FFT and SIMD magnitude extraction, ORIGAMI should comfortably fit within **12%** even at maximum fold cascade with all modulation active.

### 8.3 Memory

- FFT buffers (per voice): 2048 × 2 (real + imaginary) × 4 bytes × 2 (analysis + synthesis) = 32 KB
- Magnitude + Phase (per voice): 1024 × 4 bytes × 2 = 8 KB
- Overlap-add buffer (per voice): 2048 × 4 bytes = 8 KB
- 8 voices: ~384 KB total
- **Total: ~400 KB** — negligible

### 8.4 Latency

- Analysis window: 2048 samples / 44100 = **~46ms**
- This is inherent to any FFT-based processing
- Acceptable for pads and textures
- Future: optional 1024-sample window mode (~23ms) for lead use with reduced spectral resolution

---

## 9. Implementation Notes

### 9.1 Phase Vocoder Coherence

Spectral folding must preserve phase coherence to avoid the "phasiness" artifacts typical of poorly implemented phase vocoders. Strategy:

1. **Track instantaneous frequency per bin** using phase difference between successive frames
2. **After folding magnitude**, reconstruct phase for folded bins using the instantaneous frequency of the *source* bin (the bin that was folded into the target position)
3. **Crossfade overlapping contributions** when multiple source bins fold into the same target bin

### 9.2 Spectral Leakage Mitigation

Folding can create spectral energy at bin boundaries, causing leakage. Mitigation:
- Apply 3-point triangular smoothing to the folded magnitude spectrum before resynthesis
- This slightly blurs spectral detail but eliminates audible artifacts from hard fold edges

### 9.3 Thread Safety

- FFT buffers pre-allocated per voice in `prepare()`. No audio-thread allocation.
- STFT processing runs in-place on pre-allocated buffers.
- Fold operations are pure functions on magnitude arrays — no state, no allocation.
- ParamSnapshot pattern: all parameters cached once per block before FFT processing.

### 9.4 Denormal Protection

All magnitude values floored at `1e-15f` after fold operations to prevent denormal accumulation in the phase vocoder feedback path.

---

*Architecture spec owner: XO_OX Designs | Engine: ORIGAMI | Next action: Phase 1 — STFT Infrastructure*
