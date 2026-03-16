# XObserve Engine — Complete Technical Specification

**XO_OX Designs | 2026-03-16**
**Status: Concept Phase — Canonical Spec. Ready for Phase 1 scaffold.**
**Engine ID:** OBSERVE
**Parameter Prefix:** `obs_`
**Accent Color:** Spectral Amber `#E8A020`
**Aquatic Creature:** Mantis Shrimp (Stomatopod)
**Source Documents:** `xobserve_technical_design.md` + `xobserve_competitive_analysis.md`

---

## 1. Identity

### 1.1 What XObserve Is

XObserve is the spectral intelligence layer of XOmnibus. It is a 6-band parametric EQ where every band carries an independent feliX↔Oscar character axis, per-band Tide LFO modulation, optional dynamic EQ response, and per-band mid/side scope. It is not a transparency tool. It is a tonal sculptor that listens, adapts, and teaches.

The core idea that defines XObserve and has no parallel in any competing tool: **per-band analog character as a first-class parameter.** Every other EQ in every DAW's toolkit applies analog emulation as a single global toggle across all bands simultaneously. XObserve gives you six independent character decisions, one per band — and makes those decisions breathe via LFO and respond to other engines via coupling.

**feliX mode:** Minimum phase, surgical precision, zero harmonic color. Clinical. The EQ as a microscope.

**Oscar mode:** Natural phase rotation, transformer iron, tanh saturation, harmonic richness. Musical. The EQ as a lens through analog glass.

Both are on the same dial, per band.

### 1.2 Aquatic Creature: The Mantis Shrimp

The mantis shrimp has 16 photoreceptor types compared to the human's 3. It does not see more color — it categorizes and acts on spectral data faster than any other creature on Earth. It perceives UV, infrared, and polarized light simultaneously, then acts on that spectral intelligence with a club strike at 23 m/s that generates a cavitation bubble.

XObserve is the mantis shrimp of the XOmnibus fleet. It perceives the frequency spectrum with 16-photoreceptor precision and shapes it with physical force. The feliX bands are the UV perception — clinical, beyond-human, invisible. The Oscar bands are the smashing claw — warm, destructive, the analog warmth of physical force applied to signal.

The EQ does not observe passively. It reacts.

**feliX expression:** UV perception — precise, colorless, beyond the human register. Band handles glow cool blue `#00A6D6`.

**Oscar expression:** The smashing claw — transformer iron, 2nd harmonic bloom, all-pass phase rotation. Band handles glow warm amber `#E8A020`.

### 1.3 Aquatic Frequency Zones

XObserve maps the five XO_OX water column depth zones onto the frequency spectrum. These are not decorative labels — they create a cognitive model for EQ decisions that maps to acoustic and emotional reality.

| Zone | Frequency Range | Acoustic Character | Suggested Band Role |
|------|-----------------|--------------------|---------------------|
| **TRENCH** | 20 Hz – 80 Hz | Sub-seismic, pre-conscious. Felt before heard. | Sub shelf / HP filter |
| **ABYSS** | 80 Hz – 250 Hz | Pressure, weight, physical body. Chest resonance. | Low shelf / low-mid peak |
| **REEF** | 250 Hz – 2 kHz | Coral complexity, voice harmonics, midrange competition. | Midrange peak / notch |
| **SURFACE** | 2 kHz – 8 kHz | Light refracting, transient bite, definition, presence. | Presence peak / air |
| **SKY** | 8 kHz – 22 kHz | Atmosphere, feliX territory. Air that exists above the signal. | High shelf / LP filter |

Default band frequencies follow the zones: Band 1 = 80 Hz, Band 2 = 250 Hz, Band 3 = 800 Hz, Band 4 = 2500 Hz, Band 5 = 6000 Hz, Band 6 = 16000 Hz.

The UI renders ghosted zone labels along the frequency axis with translucent tinted glow at zone boundaries: deep navy (TRENCH), indigo (ABYSS), teal (REEF), golden (SURFACE), pale cyan (SKY).

### 1.4 Historical Homage

XObserve's character system draws from a specific lineage of analog EQ design philosophy:

**Neve 1073 (Rupert Neve, 1970):**
The defining insight that transformer iron is not noise to be eliminated but a tonal instrument to be played. The 1073's input transformer introduces a specific low-frequency phase rotation and 2nd/3rd harmonic enrichment in the 200–500 Hz range that makes kick drums feel physical and bass instruments feel weighted. XObserve's Oscar mode at Abyss-zone bands is the 1073 homage. The matched-Z transformer HP filter (`exp(-2*PI*3/sr)`) in the per-band character saturation chain directly emulates this character.

**API 550A (Lou Davis / Ed Kelso, 1967):**
The invention of proportional-Q behavior in parametric EQ — where boosting by ±3 dB widens the bell and ±12 dB narrows it. This emerges from passive circuit physics: large boosts automatically self-focus. It sounds musical because the EQ defends against harshness intrinsically. XObserve's optional per-band proportional-Q toggle (see §3.2) is the API 550A homage.

**Pultec EQP-1A (Gene Shenk / Ollie Summerland, early 1950s):**
The counterintuitive discovery that simultaneously boosting and cutting at the same low frequency (e.g., boost +6 dB at 60 Hz, cut -3 dB at 60 Hz) produces a resonant shelving curve that sounds more musical than either operation alone. This is passive component interaction producing a "bump then roll" that no digital approximation initially captured. XObserve's Oscar shelf behavior at Trench and Abyss zones borrows the Pultec's approach: the tanh saturation modifies the shelf transfer function so that the corners of the shelf gain a subtle resonance artifact absent in feliX mode.

**George Massenburg GML 8200 (1981):**
The event that proved parametric EQ could be a musical instrument rather than a correction tool. Massenburg's 1978 paper "Parametric Equalization" (AES) established the theoretical framework. The GML 8200 was the hardware realization: wide-to-narrow Q range, precision gain steps, transparent operation when gain is zero. XObserve's feliX mode is the GML 8200 homage — when all bands are at feliX = 0, the plugin adds nothing and removes nothing.

**SSL G-Bus EQ (Neve/SSL, early 1980s):**
The console bus EQ that defined modern pop and hip-hop mixing. High shelf at 10 kHz that adds "air" without sounding boosted. The distinctive SSL topend character — slightly forward, slightly aggressive — defines feliX mode at the Surface and Sky bands. XObserve's Sky-zone bands in feliX mode are the SSL homage.

---

## 2. feliX / Oscar Duality

The feliX↔Oscar axis is not merely a tone knob. It simultaneously controls three processes that together constitute "analog character":

### Process 1 — Harmonic Saturation (tanh)

Applied to the band's contribution signal after filtering, before summing back to the main path:

```cpp
float drive = 1.0f + character * 4.0f;  // 1x at feliX, 5x at full Oscar
float bandSignal = std::tanh(bandContrib * drive) / drive;
```

The division by `drive` preserves approximate unity gain. At `character = 0`, `tanh(x)/1 ≈ x` for small signals — transparent. At `character = 1.0`, the 2nd and 3rd harmonic contribution is audible. Oscar bands are no longer linear frequency shapers; they are harmonic generators.

### Process 2 — Transformer Iron Emulation (matched-Z IIR)

A first-order IIR highpass at ~3 Hz applied to the saturation stage output. This introduces the low-frequency phase rotation and bass-range saturation character of transformer-coupled circuits. Active above `character = 0.3`.

```cpp
// Matched-Z only — never Euler approximation (see CLAUDE.md)
float b1 = std::exp(-2.0f * juce::MathConstants<float>::pi * 3.0f / sampleRate);
float transformerHP = x - b1 * x_prev + b1 * y_prev;
```

This is the specific low-end character difference between the Neve 1073 and the SSL G-Channel. The 1073 uses a transformer with more low-frequency phase rotation; the SSL is a cleaner topology. feliX = SSL. Oscar = Neve.

### Process 3 — Phase Character (all-pass chain)

At feliX = 0: minimum phase (IIR SVF, zero latency, zero pre-ringing). At Oscar approaching 1.0: two first-order all-pass sections per band are instantiated, adding frequency-dependent phase rotation that approximates the "feel" of vintage transformer coupling. This is the subjective sense that Oscar-mode EQ has "depth" while feliX-mode EQ is "flat."

The all-pass chain is only instantiated when `obs_b{N}_character > 0.05` — zero overhead in feliX mode.

### The Competitive Position

No other EQ — bundled or commercial — assigns analog character as a per-band parameter. AIR Pro EQ offers global analog on/off. Pro-Q 3 is intentionally colorless. DMG EQuilibrium offers per-plugin circuit model (not per-band). TDR Slick EQ offers genre modes (not per-band). XObserve's per-band character slider gives the producer an independent tonal decision for each frequency region:

*Concrete example:* Sidechain pumping on a sub bass. Set Band 1 (Trench, HP filter) to feliX = 0 — the high-pass is surgical and colorless. Set Band 6 (Sky, high shelf) to Oscar = 0.9 — the air boost blooms with 2nd harmonic warmth. You get clean sub filtering and warm air simultaneously. No other EQ can do this in five seconds.

---

## 3. Parameter List

### 3.1 Per-Band Parameters (6 bands × 9 = 54)

ID convention: `obs_b{N}_{param}` where N = 1–6.

| Parameter | Suffix | Type | Range | Default | Notes |
|-----------|--------|------|-------|---------|-------|
| Frequency | `_freq` | Float | 20–22000 Hz | Band-specific† | Log taper |
| Gain | `_gain` | Float | -18 to +18 dB | 0 dB | ±0.1 dB precision |
| Q / Bandwidth | `_q` | Float | 0.1–20.0 | 0.707 | Log taper; peak/notch only |
| Type | `_type` | Choice | Peak/Notch/LowShelf/HighShelf/LP6/LP12/LP24/HP6/HP12/HP24/BandPass/Tilt | Peak | Enum |
| Character | `_character` | Float | 0.0–1.0 | 0.0 | 0 = feliX (clinical), 1.0 = Oscar (warm) |
| Bypass | `_bypass` | Bool | off/on | off | Per-band mute |
| Dynamic Enable | `_dyn_enable` | Bool | off/on | off | Enable dynamic EQ mode |
| Tide Enable | `_tide_enable` | Bool | off/on | off | Enable Tide LFO on this band |
| Mid/Side Scope | `_ms_scope` | Choice | Stereo/Mid/Side | Stereo | Only active when global M/S on |

† Default frequencies: Band 1 = 80 Hz, Band 2 = 250 Hz, Band 3 = 800 Hz, Band 4 = 2500 Hz, Band 5 = 6000 Hz, Band 6 = 16000 Hz.

**Per-band subtotal: 54 parameters**

### 3.2 Per-Band Dynamic EQ Parameters (6 bands × 5 = 30)

Active only when `obs_b{N}_dyn_enable` is on. DSP checks the enable flag; storage is unconditional.

| Parameter | Suffix | Range | Default |
|-----------|--------|-------|---------|
| Threshold | `_dyn_thresh` | -60 to 0 dBFS | -12 dBFS |
| Ratio | `_dyn_ratio` | 1:1 to 20:1 | 4:1 |
| Attack | `_dyn_attack` | 0.1–500 ms | 10 ms |
| Release | `_dyn_release` | 10–5000 ms | 100 ms |
| Mode | `_dyn_mode` | Compress (cut above thresh) / Expand (boost above thresh) | Compress |

**Dynamic EQ subtotal: 30 parameters**

### 3.3 Tide LFO Parameters (6 bands × 4 = 24)

Active only when `obs_b{N}_tide_enable` is on.

| Parameter | Suffix | Range | Default | Notes |
|-----------|--------|-------|---------|-------|
| Rate | `_tide_rate` | 0.01–20 Hz | 0.5 Hz | Log taper; sync-able to host BPM |
| Depth | `_tide_depth` | 0–100% | 30% | % of target parameter range |
| Shape | `_tide_shape` | Sine/Tri/Saw/RevSaw/Square/S&H/Drift | Sine | Drift = low-pass-filtered S&H |
| Target | `_tide_target` | Freq/Gain/Q/Character | Gain | What the LFO modulates |

**Drift shape** is the secret weapon: low-pass-filtered S&H that produces the specific slow, wandering character of vintage console temperature drift. It never repeats. Every note sounds subtly different. Use it on Band 3 Character target at 0.2 Hz for "Living Console" behavior.

**Tide subtotal: 24 parameters**

### 3.4 Global Parameters (18)

| Parameter | ID | Range | Default | Notes |
|-----------|----|-------|---------|-------|
| Input Gain | `obs_input_gain` | -24 to +24 dB | 0 dB | Pre-EQ gain |
| Output Gain | `obs_output_gain` | -24 to +24 dB | 0 dB | Post-EQ gain |
| Mix | `obs_mix` | 0–100% | 100% | Dry/wet parallel |
| Mid/Side Enable | `obs_ms_enable` | off/on | off | Enables M/S matrix |
| Phase Mode | `obs_phase_mode` | Minimum / Natural / Linear | Minimum | 3-way enum — NOT a bool |
| Analyzer Enable | `obs_analyzer_on` | off/on | on | FFT display on/off |
| Analyzer Decay | `obs_analyzer_decay` | 0–100% | 70% | Peak hold fallback speed |
| Oversampling | `obs_oversample` | 1x / 2x / 4x | 2x | For Oscar saturation accuracy |
| Character Global | `obs_character_global` | 0.0–1.0 | 0.0 | Master feliX↔Oscar offset added to all bands |
| Tide Master Rate | `obs_tide_rate_global` | 0.01–20 Hz | 0.5 Hz | Global multiplier for all Tide LFOs |
| Tide Sync | `obs_tide_sync` | off/on | off | Sync all Tide LFOs to host BPM |
| Tide Sync Division | `obs_tide_division` | 1/32–4/1 | 1/4 | When Tide Sync = on |
| Coupling Input Gain | `obs_coupling_gain` | 0–200% | 100% | Scales incoming coupling modulation |
| Coupling Target | `obs_coupling_target` | Choice (see §5) | AmpToFilter→Band1Gain | Routes coupling input |
| Phase Scope | `obs_phase_scope` | Lissajous / Correlation / Off | Lissajous | Mini meter in UI |
| True Peak Limit | `obs_tpeak_limit` | off/on | off | Brickwall true peak ceiling |
| Stereo Link | `obs_stereo_link` | 0–100% | 100% | Links L/R band edits; 0% = full dual mono |
| Bypass All | `obs_bypass_all` | off/on | off | Hardwire bypass entire plugin |

**Note on `obs_phase_mode`:** This is a 3-way enum — Minimum Phase (IIR, zero latency), Natural Phase (256-tap FIR, ~3 ms latency), Linear Phase (full FIR, ~42 ms latency). It is explicitly NOT a boolean. The Natural Phase mode is XObserve's competitive response to Pro-Q 3's Natural Phase: zero latency is for live monitoring, Natural Phase is for the "clean but not clinical" space, Linear Phase is for mastering. Three distinct tools, one parameter.

**Global subtotal: 18 parameters**

### 3.5 Total Parameter Count

| Section | Count |
|---------|-------|
| Per-band (6 × 9) | 54 |
| Dynamic EQ (6 × 5) | 30 |
| Tide LFO (6 × 4) | 24 |
| Global | 18 |
| **Total** | **126** |

Every parameter is reachable in the UI and audibly consequential (Doctrine D004). Tide and dynamic parameters are guarded by enable flags but produce audible changes when enabled — they are not dead.

---

## 4. DSP Architecture

### 4.1 Filter Core: Cytomic SVF (Andrew Simper, 2013)

The State Variable Filter from Andrew Simper's "Solving the Continuous SVF Equations Using Trapezoidal Integration and Equivalent Circuits" is the correct algorithm for XObserve:

- **Stability at high Q:** Unlike the biquad transposed-direct-form-II, the SVF remains stable at Q > 10. Critical for surgical notches.
- **Simultaneous LP/HP/BP/Notch outputs from a single coefficient computation:** Efficient for the filter type enum — no coefficient rebuild cost when switching between Peak and Notch.
- **Modulation without zipper noise:** The SVF's g/k coefficients smooth per-sample without clicks. Essential for Tide LFO modulation of frequency and Q.
- **No catastrophic cancellation at extreme frequencies:** The SVF's trapezoidal integration handles near-Nyquist frequencies correctly.

```cpp
struct SVFCoeffs {
    float g, k;         // derived from freq, Q, sampleRate
    float a1, a2, a3;   // tick-derived
};

struct SVFState {
    float ic1eq, ic2eq; // integrator states — two floats per band
};

// Two states per band for stereo; four for M/S mode
```

Coefficient computation occurs at block rate unless Tide LFO is active on frequency or Q, in which case coefficient smoothing occurs per-sample via one-pole lowpass on g and k.

### 4.2 Phase Mode Implementations

**Minimum Phase (default):** IIR SVF pipeline. Zero added latency. Zero pre-ringing. feliX character. Used for live performance, real-time mixing, and any context where latency is unacceptable.

**Natural Phase (short FIR, ~3 ms latency):** A 256-tap FIR impulse at 48 kHz providing approximate phase linearity without the full 2048-sample latency of Linear Phase mode. The tradeoff: not truly linear phase but perceptibly more neutral than minimum phase on transient material. Competitive with FabFilter Pro-Q 3's Natural Phase mode. Latency reported to host via `getLatencySamples()`.

**Linear Phase (full FIR, ~42 ms latency):** FFT-domain filtering using overlap-add.
- FFT size: 4096 samples (configurable: 2048 for lower latency, 8192 for higher resolution)
- Overlap: 75% (Hann window, produces flat summing with perfect reconstruction)
- Latency: FFT_size / 2 = 2048 samples (~42 ms at 48 kHz), reported to host
- The filter's frequency response is computed from the 6 SVF bands and inverse-FFT'd to a FIR impulse, then convolved in the frequency domain
- Oscar character saturation remains in the time domain post-convolution; linear phase addresses filter phase only

### 4.3 Dynamic EQ: Per-Band Envelope Follower

When a band's `_dyn_enable` is on, a sidechain signal is extracted via a fixed bandpass (same center frequency as the band, Q = 1.0), half-wave rectified, and fed to an analog-modeled attack/release envelope follower:

```cpp
// Matched-Z time constants — never hardcoded sample counts
float att = std::exp(-1.0f / (attackMs * 0.001f * sampleRate));
float rel = std::exp(-1.0f / (releaseMs * 0.001f * sampleRate));
envLevel = (rectified > envLevel)
    ? att * envLevel + (1 - att) * rectified
    : rel * envLevel + (1 - rel) * rectified;
```

Above threshold: Compress mode reduces gain by `(level - thresh) * (1 - 1/ratio)` dB. Expand mode boosts by the same formula. The gain offset is added on top of the static band gain (not replacing it) and smoothed with a one-pole lowpass at 20 Hz to prevent amplitude modulation artifacts.

External sidechain routing is handled via the coupling system — see §5.1.

### 4.4 Mid/Side Matrix

```cpp
// Encode L/R → M/S
float mid  = (L + R) * 0.7071f;
float side = (L - R) * 0.7071f;

// Each band processed according to its obs_b{N}_ms_scope
// Scope = Stereo: apply to both mid and side
// Scope = Mid: apply to mid only
// Scope = Side: apply to side only

// Decode M/S → L/R
L = (mid + side) * 0.7071f;
R = (mid - side) * 0.7071f;
```

When `obs_ms_enable = off`, the M/S matrix encode/decode is bypassed entirely and `obs_b{N}_ms_scope` is ignored. The per-band M/S scope gives mastering-grade stereo control uncommon even in commercial EQ plugins — a 200 Hz cut applied to Mid only reduces muddiness in the center image without touching the low-end stereo information from room mics.

### 4.5 Spectral Analyzer

- FFT size: 2048 samples
- Window: Hann
- Overlap: 50% (75% for smoother animation)
- Refresh: 30 fps (interleaved with audio processing on message thread)
- Display: log-frequency X axis (20 Hz – 22 kHz), -90 to +6 dBFS Y axis
- Thread safety: FFT on audio thread into double-buffered display frame; message thread reads inactive frame. No locks — atomic frame index.
- Pre/Post view toggle: shows analyzer before or after EQ. The delta (difference curve) renders as the EQ transfer function overlay.
- Band handles overlay the curve with color, size, and animation encoding (see UI §7).

---

## 5. Coupling Design

XObserve is an FX processor engine. Its coupling model is receiver-heavy on input and provides spectral analysis data on output — making it unique in the fleet as a sensor node that participates in the XOmnibus network.

### 5.1 Coupling Inputs Accepted

| CouplingType | XObserve Behavior | Designed Use Case |
|---|---|---|
| `AmpToFilter` | Source amplitude modulates selected band's gain. Negative amount = ducking. | ONSET kick → Band 1 gain ducking (sidechain pumping with Oscar warmth on release) |
| `RhythmToBlend` | Source rhythm gate modulates the Mix parameter. Rhythm on = wet, off = dry. | ONSET groove pattern → gated EQ timbral effect |
| `EnvToMorph` | Source envelope modulates `obs_character_global`. Attack → Oscar; sustain → feliX. | OVERDUB reverb tail → character sweep |
| `LFOToPitch` | Source LFO frequency modulates target band's frequency parameter. | ODYSSEY LFO → Band 3 frequency sweep |
| `AudioToFM` | Source audio amplitude drives target band's Tide depth. High energy = deeper modulation. | ONSET transient energy → Tide depth |
| `FilterToFilter` | Source filter output feeds into XObserve's dynamic EQ sidechain detector. | OBESE filtered bass → dynamic EQ key signal |
| `AmpToChoke` | Source amplitude above threshold bypasses all XObserve processing (output = dry). | SNAP note-off → bypass character |
| `EnvToDecay` | Source envelope → Tide rate on selected band. Long envelope = slow Tide. | OPAL grain density → Tide rate |

### 5.2 Coupling Outputs Exposed

This is what makes XObserve novel in the coupling network — it is simultaneously an FX processor and a spectral analysis engine that can drive synthesis engines:

| Channel | Content | Type |
|---------|---------|------|
| `ch0` | Pre-EQ audio (left/mid) | Audio |
| `ch1` | Post-EQ audio (left/mid) | Audio |
| `ch2` | Spectral centroid — brightness (0–1) | Scalar |
| `ch3` | Band 1–3 total energy — low-end mass (0–1) | Scalar |
| `ch4` | Band 4–6 total energy — high-end air (0–1) | Scalar |
| `ch5` | Dynamic EQ gain reduction (dBFS, signed) | Scalar |

An engine coupled to `ch2` (spectral centroid) could track brightness and self-modulate as the EQ shapes it — closing a feedback loop that no standalone EQ product even considers. The architectural uniqueness: XObserve is simultaneously an FX processor and a spectral sensor in the XOmnibus signal graph.

### 5.3 Flagship Coupling: ONSET → XOBSERVE

- **Type:** `AmpToFilter`
- **ONSET output:** ch2 (perc envelope, amplitude per-kick trigger)
- **XObserve response:** Band 1 gain ducking by `coupling_amount * obs_coupling_gain`
- **Result:** Every kick hit from ONSET ducks the low-end of the audio running through XObserve. Unlike standard sidechain compression:
  - Band 1's Oscar character means the duck itself has harmonic color (the transient is warm, not clinical)
  - Tide modulation continues during the release phase of the duck
  - Dynamic EQ threshold determines when kicks are "big enough" to trigger full duck vs. light touch

Preset name: **"Kick Duck Warmth"** — ships in the Entangled mood.

### 5.4 Why This Is Novel

No bundled MPC EQ receives modulation from other instruments. XObserve's `AmpToFilter` coupling from ONSET means a kick trigger can duck Band 1 gain with transformer-colored release — all without touching automation. An ONSET coupling can simultaneously modulate Tide depth on Band 4 via `AudioToFM`, sweep the character of Band 6 via `EnvToMorph`, and gate the entire Mix via `RhythmToBlend`. The EQ becomes a performance instrument that responds to the groove of the beat.

This is synthesis-level expressiveness applied to what is normally a static mix tool.

---

## 6. Macros (4 Required — Doctrine D002)

| Macro | ID | Range | What It Does |
|-------|----|-------|--------------|
| **M1: SHAPE** | `obs_macro_shape` | 0–1 | Morphs from flat (0) to musically opinionated curve (1): Trench boost, Abyss cut, Reef scoop, Surface air boost. One-gesture "taste" macro. |
| **M2: TIDE** | `obs_macro_tide` | 0–1 | Master depth control for all active Tide LFOs simultaneously. 0 = no modulation, 1 = full depth. |
| **M3: CHARACTER** | `obs_macro_character` | 0–1 | Aliases `obs_character_global`. The feliX↔Oscar macro. Dragging from 0 to 1 applies global analog warmth. |
| **M4: DYNAMICS** | `obs_macro_dynamics` | 0–1 | Master threshold offset for all dynamic bands. 0 = preset threshold, 1 = thresholds raised 12 dB (less dynamic action). "How reactive" macro. |

---

## 7. UI Design

### 7.1 Overall Layout (800 × 500 px default, resizable)

Three horizontal zones:

1. **Top bar (48 px):** Plugin name / logo left, preset browser center, input/output gain knobs right, bypass toggle right edge.
2. **Main area (350 px):** Spectrum analyzer with band handles overlaid, aquatic zone labels on background.
3. **Bottom bar (102 px):** Per-band parameter strip (left 60%), global controls (right 40%), coupling strip (collapsible).

Between main area and bottom bar: **Global Character Strip** — a thin horizontal bar spanning full width. The feliX↔Oscar global offset. Gradient: cool blue `#00A6D6` (left) to warm amber `#E8A020` (right). Label: `CHARACTER — feliX ←————— Oscar`. This is M3.

### 7.2 Main Area — Spectrum Analyzer

- Background: deep navy-to-black gradient
- Pre-EQ spectrum: translucent grey fill
- Post-EQ spectrum: warm amber `#E8A020` at 30% opacity (boost) / cooler blue (cut)
- EQ transfer function: bright amber line `#E8A020` connecting band gain points
- Aquatic zone labels: ghosted text at zone boundaries with colored glow

**Band handles:**
- Six glowing dots on the EQ transfer curve
- Size proportional to Q (wider Q = larger dot)
- Color: feliX (cool blue `#00A6D6`) → Oscar (warm amber `#E8A020`) based on character
- Tide active: dot pulses with a slow glow animation at the Tide rate
- Dynamic active: dot has a subtle ring that compresses/expands with gain reduction
- Interactions: vertical drag = gain, horizontal drag = frequency, Shift+drag = Q, double-click = band detail popover

### 7.3 Band Detail Popover

```
[ Band 3 — REEF ]
Freq:    [800 Hz     ]   Type: [ Peak         ▼]
Gain:    [+2.3 dB    ]   Q:    [1.40          ]
Scope:   [ Stereo    ▼]  Bypass: [ ]

Character: [feliX ——●—————————————————— Oscar]
              clinical                   warm

Tide:  [✓ Enable]  Rate: [0.5 Hz]  Depth: [30%]
       Shape: [Drift ▼]   Target: [Character ▼]

Dynamic: [ Enable]  Thresh: [-12 dB]  Ratio: [4:1]
         Attack: [10 ms]   Release: [100 ms]  Mode: [Compress ▼]
```

### 7.4 Coupling Strip

Revealed when XObserve detects it is running inside an XOmnibus slot:
- Active coupling route display (e.g., "ONSET → Band 1 Gain [AmpToFilter]")
- Coupling gain knob
- Coupling target selector

---

## 8. XPN Integration

### 8.1 Oxport Bake Path

XObserve is an FX engine applied to synthesis engine output. When Oxport renders a kit, the correct path is to bake the EQ curve into the offline render:

```
Engine render → raw buffer → XObserve.processOffline() → baked buffer → .wav sample
```

This produces XPN samples that carry the tonal character of the EQ preset even on MPC hardware that cannot run XObserve in real time. The XPN kit sounds identical to the in-plugin result.

**Required method:** XObserve must expose:
```cpp
void processOffline(juce::AudioBuffer<float>& buffer, double sampleRate);
```

Oxport calls this after the engine render, applying the current EQ state as a static snapshot (no Tide LFO modulation).

**Tide modulation in Oxport:** For presets with Tide active, Oxport renders one full Tide cycle as a time-varying EQ applied to the sample. The resulting .wav has modulation baked in. Bake duration = `1 / tide_rate` seconds. For multi-Tide presets, use the LCM of all active rates up to a maximum of 8 seconds. The manifest notes "Tide baked."

**Dynamic EQ baking:** If dynamic bands are active, Oxport detects and warns: "Dynamic EQ detected — rendered with static threshold snapshot. Enable bake anyway?" Dynamic behavior depends on playback dynamics and cannot be faithfully baked.

### 8.2 XPN Metadata: EQ Fingerprint

The XPN `.xpnmeta` bundle includes the EQ configuration as machine-readable metadata:

```json
{
  "xobserve_snapshot": {
    "preset_name": "Kick Duck Warmth",
    "bands": [
      { "band": 1, "freq": 80, "gain": -3.5, "q": 0.7, "type": "LowShelf", "character": 0.6 },
      ...
    ],
    "character_global": 0.3,
    "tide_active_bands": [2, 4],
    "linear_phase": false,
    "phase_mode": "Minimum"
  }
}
```

This serves as both reference documentation and the foundation for future smart tooling (a `xpn_observe_matcher.py` that suggests EQ presets based on spectral content of new samples).

### 8.3 Cover Art: Spectral Fingerprint

XObserve exposes its frequency response curve as a visual input for cover art generation via an `ObserveFingerprint`:

```json
{
  "type": "xobserve_fingerprint",
  "transfer_curve": [0.0, 0.3, 1.2, -0.5, ...],  // 512 dB values, log-spaced
  "centroid_shift": -0.15,    // net change in spectral centroid (negative = darker)
  "character_mean": 0.45,     // average Oscar-ness across active bands
  "tide_active": true,
  "dynamic_active": false
}
```

`xpn_cover_art.py` renders the `transfer_curve` as a structural element in the cover art. A V-shaped cut becomes a V in the cover geometry. A high-shelf boost becomes an ascending arc. The artwork is literally derived from the EQ shape — the most direct possible expression of "sound as image."

### 8.4 Why Nobody Does This

Dynamic EQ with spectral modulation output represents a novel coupling pathway:

1. Every other EQ accepts audio, shapes it, outputs audio. That's the complete signal path.
2. XObserve accepts audio, shapes it, outputs audio — *and* outputs spectral analysis scalars (centroid, band energy) that can drive other engines as modulation signals.
3. This closes a feedback loop: the EQ's analysis of the signal becomes a modulation source for the synthesis engines generating that signal.
4. Combined with coupling inputs that let other engines *drive* the EQ in real time, XObserve is simultaneously a receiver, a processor, and a sensor.

No standalone EQ product, bundled or commercial, occupies this architectural position.

---

## 9. Factory Presets (150 target)

20 concept presets across the four fleet moods. Full 150-preset factory library follows the same distribution (see sound design guidelines).

### Foundation (5 — clean utility)

| Name | Description | Character |
|------|-------------|-----------|
| **Clean Slate** | Flat reference. HP at 30 Hz only. | feliX = 0.0 all |
| **Broadcast Ready** | Radio shelf combo: +2 dB high shelf, -2 dB low-mid | feliX = 0.1 |
| **Deep Foundation** | Sub shelf boost for electronic music: +4 dB @ 60 Hz | feliX = 0.15 |
| **High Pass Only** | 80 Hz HP12, utility cleaning filter | feliX = 0.0 |
| **Telephone** | Bandpass effect: HP @ 300 Hz, LP @ 3.4 kHz | feliX = 0.8 |

### Atmosphere (5 — Tide active, gentle)

| Name | Description | Tide | Character |
|------|-------------|------|-----------|
| **Breathing Reef** | Gentle 1 dB undulation at 1 kHz, Tide on Gain, Sine | Band 3: 0.3 Hz | Oscar = 0.4 |
| **Tidal Warmth** | Low shelf rises and falls like the tide: +0-3 dB @ 120 Hz | Band 2: 0.1 Hz Sine | Oscar = 0.6 |
| **Living Console** | Band 4 character Tide: sweeps feliX→Oscar at 0.15 Hz | Band 4: Drift on Character | Oscar = sweeping |
| **Shimmer Surface** | High shelf shimmers with Drift shape | Band 6: Drift 0.25 Hz on Gain | feliX = 0.3 |
| **Morphing Presence** | Presence peak sweeps 2–4 kHz via Tide on Freq | Band 4: Sine 0.4 Hz on Freq | feliX = 0.2 |

### Prism (5 — dynamic bands, dramatic)

| Name | Description | Dynamic | Character |
|------|-------------|---------|-----------|
| **Bloom Gate** | Low-mid expands when kick hits | Band 2: Expand, fast attack | Oscar = 0.7 |
| **Surgical Strike** | Dynamic notch at 800 Hz, -12 dBFS threshold | Band 3: Notch, Compress | feliX = 0.05 |
| **Vintage Air** | M/S mode, high shelf on Mid only | Band 6: Mid scope, +3 dB | Oscar = 0.9 |
| **Iron Heart** | Full Oscar all bands: transformer warmth on every band | All bands Oscar = 1.0 | Oscar = 1.0 |
| **Side Wide** | Side channel high shelf boost: widens stereo image | Band 6: Side scope, +4 dB @ 12 kHz | feliX = 0.3 |

### Flux (5 — extreme Tide, moving EQ)

| Name | Description | Tide | Character |
|------|-------------|------|-----------|
| **Storm Cell** | All six bands in Tide at prime rates (0.07–0.41 Hz) — never repeats | All bands: different rates | Oscar = 0.6 |
| **Wobble Bass** | Sub gain oscillates ±6 dB at 4 Hz (Trench tremolo) | Band 1: 4 Hz Sine on Gain | Oscar = 0.8 |
| **Deep Pulse** | The flagship demo preset: Band 2 Oscar, Tide Drift on Character, ONSET coupling | Band 2: Drift 0.2 Hz on Character | Oscar = 1.0 |
| **Q Tremor** | Q modulated on Band 3: razor-narrow to wide at 1 Hz | Band 3: Tri 1 Hz on Q | feliX = 0.4 |
| **Character Ride** | Band 4 Tide targets Character: sweeps feliX↔Oscar on presence band | Band 4: Sine 0.2 Hz on Character | sweeping |

---

## 10. Doctrine Compliance

| Doctrine | Compliance |
|----------|-----------|
| D001: Velocity shapes timbre | N/A — FX engine. Mod wheel routes to `obs_character_global` for expression input. |
| D002: Modulation is lifeblood | PASS — 4 macros, Tide LFO per band with 7 shapes, 8 coupling input types |
| D003: Physics IS the synthesis | PASS — Cytomic SVF documented with theoretical basis; matched-Z transformer emulation only |
| D004: Dead parameters forbidden | PASS — 126 params, all wired; Tide and dynamic params guarded by enable flags but audible when enabled |
| D005: Engine must breathe | PASS — Tide LFO per band, minimum rate 0.01 Hz; Global Tide Master Rate exists |
| D006: Expression input not optional | PASS — Coupling inputs; mod wheel → `obs_character_global` |

---

## 11. Engine Registration

```cpp
REGISTER_ENGINE("XObserve", []() -> std::unique_ptr<SynthEngine> {
    return std::make_unique<ObserveEngine>();
});

// getEngineId() → "OBSERVE"
// getAccentColour() → juce::Colour(0xFFE8A020)  // Spectral Amber
// getMaxVoices() → 1 (mono/stereo FX, no voice concept)
// Parameter prefix: "obs_"
```

Add case to `prefixForEngine()` in `XOmnibusEditor.h`:
```cpp
if (engineId == "Observe") return "obs_";
```

---

## 12. Build Sequencing

| Phase | Deliverable | Notes |
|-------|-------------|-------|
| Phase 0 | Concept (this document) | COMPLETE |
| Phase 1 | `ObserveEngine.h` scaffold: SynthEngine interface, 126 params, silence output | Standalone builds |
| Phase 2 | Cytomic SVF for 6 bands, feliX mode only | All filter types working, no character |
| Phase 3 | Oscar character: tanh + transformer HP + all-pass phase | Character slider audibly moves |
| Phase 4 | Tide LFO per band with all 7 shapes including Drift | Modulation shapes audible |
| Phase 5 | Dynamic EQ per band | Sidechain detection + gain reduction working |
| Phase 6 | Mid/Side processing | M/S matrix, per-band scope |
| Phase 7 | Phase mode enum (Minimum / Natural / Linear) | Latency reporting, overlap-add |
| Phase 8 | Spectral analyzer display | FFT display at 30 fps |
| Phase 9 | Coupling I/O | AmpToFilter, RhythmToBlend, spectral centroid output |
| Phase 10 | Factory presets (150) | All 4 mood categories populated |
| Phase 11 | Oxport integration (bake path) | Oxport calls processOffline(); metadata included |

---

*Authored: XO_OX Designs, 2026-03-16*
*Document type: Canonical engine specification. Supersedes standalone design documents as the definitive reference.*
*Source materials: `xobserve_technical_design.md` (DSP), `xobserve_competitive_analysis.md` (Vibe, positioning)*
