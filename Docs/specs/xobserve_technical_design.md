# XObserve — Technical Design Document

**XO_OX Designs | March 2026**
**Status: Concept Phase — Full Design Spec**
**Engine ID:** OBSERVE
**Parameter Prefix:** `obs_`
**Accent Color:** Spectral Amber `#E8A020`
**Creature:** Mantis shrimp — 16 photoreceptor types, perceives UV, infrared, and polarized light. The most spectrally sophisticated eye in nature.

---

## 1. Concept Identity

XObserve is a 6-band parametric EQ where every band carries a feliX↔Oscar character axis — sliding from clinical precision (feliX) to analog warmth and harmonic saturation (Oscar). It is not a transparency tool. It is a tonal character sculptor.

The five aquatic frequency zones (Trench → Abyss → Reef → Surface → Sky) frame every decision as a choice about where in the water column you are shaping the sound. Tide LFO modulation per band makes the EQ breathe. Dynamic EQ bands allow amplitude-responsive shaping. Mid/side processing unlocks stereo depth.

**Character duality expressed:** feliX = surgical, phase-linear, no color. Oscar = transformer iron, tape saturation, harmonic excitement. The feliX↔Oscar slider per band is not just a tone knob — it is a harmonic and phase-attitude control.

**Historical homage:** GML 8200 (George Massenburg, 1981) — parametric EQ as musical instrument. Neve 1073 (Rupert Neve, 1970) — transformer iron as tonal character. API 550A (Lou Davis/Ed Kelso, 1967) — proportional-Q as musical choice. Sontec MES-432 (George Massenburg, 1978) — the paper that proved parametric EQ could have taste.

---

## 2. Aquatic Frequency Zones

The five zones map EQ bands to the XO_OX water column mythology:

| Zone | Frequency Range | Character | Suggested Band Role |
|------|----------------|-----------|---------------------|
| **Trench** | 20 Hz – 80 Hz | Sub-seismic, pre-conscious | Sub shelf / HP filter |
| **Abyss** | 80 Hz – 250 Hz | Pressure, weight, body | Low shelf / low-mid peak |
| **Reef** | 250 Hz – 2 kHz | Coral complexity, voice harmonics | Midrange peak / notch |
| **Surface** | 2 kHz – 8 kHz | Light refracting, transient bite | Presence peak / air |
| **Sky** | 8 kHz – 22 kHz | Atmosphere, feliX territory | High shelf / LP filter |

The UI renders zone label overlays on the spectrum analyzer background, color-coded with translucent tints at each band's position.

---

## 3. Parameter List

### 3.1 Per-Band Parameters (6 bands × 9 parameters = 54)

Each band uses the naming convention `obs_b{N}_{param}` where N = 1–6.

| Parameter | ID Suffix | Type | Range | Default | Notes |
|-----------|-----------|------|-------|---------|-------|
| Frequency | `_freq` | Float | 20–22000 Hz | Band-specific† | Log taper |
| Gain | `_gain` | Float | -18 to +18 dB | 0 dB | ±0.1 dB precision |
| Q / Bandwidth | `_q` | Float | 0.1–20.0 | 0.707 | Log taper; peak/notch only |
| Type | `_type` | Choice | Peak, Notch, LowShelf, HighShelf, LP6, LP12, LP24, HP6, HP12, HP24, BandPass, Tilt | Peak | Enum |
| Character | `_character` | Float | 0.0–1.0 | 0.0 | 0 = feliX (clinical), 1.0 = Oscar (warm/saturated) |
| Bypass | `_bypass` | Bool | off/on | off | Per-band mute |
| Dynamic Enable | `_dyn_enable` | Bool | off/on | off | Enable dynamic EQ mode |
| Tide Enable | `_tide_enable` | Bool | off/on | off | Enable Tide LFO on this band |
| Mid/Side Scope | `_ms_scope` | Choice | Stereo, Mid, Side | Stereo | Only active when global M/S on |

† Default frequencies: Band 1 = 80 Hz, Band 2 = 250 Hz, Band 3 = 800 Hz, Band 4 = 2500 Hz, Band 5 = 6000 Hz, Band 6 = 16000 Hz.

**Per-band subtotal: 54 parameters**

### 3.2 Per-Band Dynamic EQ Parameters (6 bands × 5 = 30)

Active only when `obs_b{N}_dyn_enable` is on. Stored regardless; DSP checks enable flag.

| Parameter | ID Suffix | Range | Default |
|-----------|-----------|-------|---------|
| Threshold | `_dyn_thresh` | -60 to 0 dBFS | -12 dBFS |
| Ratio | `_dyn_ratio` | 1:1 to 20:1 | 4:1 |
| Attack | `_dyn_attack` | 0.1–500 ms | 10 ms |
| Release | `_dyn_release` | 10–5000 ms | 100 ms |
| Mode | `_dyn_mode` | Compress (cut above thresh), Expand (boost above thresh) | Compress |

**Dynamic EQ subtotal: 30 parameters**

### 3.3 Tide (Per-Band LFO) Parameters (6 bands × 4 = 24)

Active only when `obs_b{N}_tide_enable` is on.

| Parameter | ID Suffix | Range | Default | Notes |
|-----------|-----------|-------|---------|-------|
| Rate | `_tide_rate` | 0.01–20 Hz | 0.5 Hz | Log taper; sync-able to host BPM |
| Depth | `_tide_depth` | 0–100% | 30% | % of gain parameter range |
| Shape | `_tide_shape` | Sine, Tri, Saw, RevSaw, Square, S&H, Drift | Sine | Drift = low-pass-filtered S&H |
| Target | `_tide_target` | Freq, Gain, Q, Character | Gain | What the LFO modulates |

**Tide subtotal: 24 parameters**

### 3.4 Global Parameters (18)

| Parameter | ID | Range | Default | Notes |
|-----------|-----|-------|---------|-------|
| Input Gain | `obs_input_gain` | -24 to +24 dB | 0 dB | Pre-EQ gain |
| Output Gain | `obs_output_gain` | -24 to +24 dB | 0 dB | Post-EQ gain |
| Mix | `obs_mix` | 0–100% | 100% | Dry/wet parallel |
| Mid/Side Enable | `obs_ms_enable` | off/on | off | Enables M/S matrix |
| Linear Phase | `obs_linear_phase` | off/on | off | FIR mode; adds latency |
| Analyzer Enable | `obs_analyzer_on` | off/on | on | FFT display on/off |
| Analyzer Decay | `obs_analyzer_decay` | 0–100% | 70% | Peak hold fallback speed |
| Oversampling | `obs_oversample` | 1x, 2x, 4x | 2x | For Oscar saturation accuracy |
| Character Global | `obs_character_global` | 0.0–1.0 | 0.0 | Master feliX↔Oscar offset added to all bands |
| Tide Master Rate | `obs_tide_rate_global` | 0.01–20 Hz | 0.5 Hz | Global multiplier for all Tide LFOs |
| Tide Sync | `obs_tide_sync` | off/on | off | Sync all Tide LFOs to host BPM |
| Tide Sync Division | `obs_tide_division` | 1/32, 1/16, 1/8, 1/4, 1/2, 1/1, 2/1, 4/1 | 1/4 | When Tide Sync = on |
| Coupling Input Gain | `obs_coupling_gain` | 0–200% | 100% | Scales incoming coupling modulation |
| Coupling Target | `obs_coupling_target` | Choice (see §6) | AmpToFilter→Band1Gain | Routes coupling input |
| Phase Scope | `obs_phase_scope` | Lissajous, Correlation, Off | Lissajous | Mini meter in UI |
| True Peak Limit | `obs_tpeak_limit` | off/on | off | Brickwall true peak ceiling at 0 dBFS |
| Stereo Link | `obs_stereo_link` | 0–100% | 100% | Links L/R band edits; 0% = full dual mono |
| Bypass All | `obs_bypass_all` | off/on | off | Hardwire bypass entire plugin |

**Global subtotal: 18 parameters**

### 3.5 Total Parameter Count

| Section | Count |
|---------|-------|
| Per-band (6 × 9) | 54 |
| Dynamic EQ (6 × 5) | 30 |
| Tide (6 × 4) | 24 |
| Global | 18 |
| **Total** | **126** |

This is a lean, focused parameter set. No hidden parameters. Every parameter is reachable in the UI and audibly consequential (Doctrine D004).

---

## 4. DSP Architecture

### 4.1 Filter Algorithm: Cytomic SVF (Andrew Simper, 2013)

The State Variable Filter (SVF) from Andrew Simper's "Solving the Continuous SVF Equations Using Trapezoidal Integration and Equivalent Circuits" is the correct choice for XObserve:

- **Stability at high Q:** Unlike the biquad transposed-direct-form-II, the SVF remains stable at Q > 10. This is critical for surgical notches and tight resonant peaks.
- **Simultaneous LP/HP/BP/Notch outputs from a single coefficient computation:** Efficient for the filter type enum — no coefficient rebuild cost when switching between Peak and Notch.
- **Modulation without zipper noise:** The SVF's g/k coefficients can be smoothed per-sample without causing clicks. Essential for Tide LFO modulation of frequency and Q.
- **No catastrophic cancellation at extreme frequencies:** Unlike bilinear-transform biquads, the SVF's trapezoidal integration handles near-Nyquist frequencies correctly.

**Implementation:**
```cpp
// Cytomic SVF per-sample, per-band
struct SVFCoeffs {
    float g, k;         // derived from freq, Q, sampleRate
    float a1, a2, a3;   // tick-derived
};

struct SVFState {
    float ic1eq, ic2eq; // integrator states — two floats per band
};

// Two states required per band for stereo; four for M/S mode
```

Coefficient computation occurs at block rate (not per-sample) unless Tide LFO is active on frequency or Q, in which case coefficient smoothing occurs per-sample via one-pole lowpass on g and k.

**Shelf and high-pass/low-pass types** use the same SVF core with gain distributed differently across LP/BP/HP outputs. Specific shelf formulas follow Simper's 2013 cookbook.

### 4.2 Oscar Mode: Per-Band Character Saturation

The `obs_b{N}_character` slider (0 = feliX, 1 = Oscar) controls three simultaneous processes:

**Process 1: Harmonic saturation (tanh soft clipper)**

A mild tanh saturation stage applied to the band's contribution signal after filtering, before summing back to the main path. The drive amount scales with character:

```cpp
float drive = 1.0f + character * 4.0f;  // 1x at feliX, 5x at full Oscar
float bandSignal = std::tanh(bandContrib * drive) / drive;
```

The division by drive preserves approximate unity gain. At character = 0, drive = 1 and `tanh(x)/1 ≈ x` for small signals (transparent). At character = 1.0, the 2nd and 3rd harmonic contribution is audible.

**Process 2: Transformer iron emulation (loosely-coupled inductor model)**

A first-order IIR highpass at ~3 Hz (matched-Z: `exp(-2*PI*3/sr)`) applied to the saturation stage output. This introduces the low-frequency phase rotation and bass-range saturation character of transformer-coupled circuits. Active above character = 0.3.

```cpp
// Matched-Z highpass (not Euler — see CLAUDE.md critical pattern)
float b1 = std::exp(-2.0f * juce::MathConstants<float>::pi * 3.0f / sampleRate);
float transformerHP = x - b1 * x_prev + b1 * y_prev;
```

**Process 3: Phase character (minimum vs. linear blend)**

At feliX = 0: minimum phase (IIR SVF, zero added latency, zero pre-ringing). At Oscar approaching 1.0: a subtle pre-ringing is achieved not via full FIR linear phase (expensive), but via a small all-pass chain that adds frequency-dependent phase rotation, approximating the character of vintage transformers. This is the "feel" difference between the Neve and the SSL.

The all-pass chain (2 first-order all-pass sections per band) is only instantiated when `obs_b{N}_character > 0.05` — zero overhead in feliX mode.

**Oversampling for Oscar accuracy:** When `obs_oversample` = 2x or 4x, the saturation stage is processed at 2× or 4× sample rate using polyphase resampling (3-tap FIR up, 3-tap FIR down). This prevents high-order harmonic aliasing from the tanh nonlinearity. Required for accurate results above character = 0.7 at high frequencies.

### 4.3 Linear Phase Mode (FIR via FFT Overlap-Add)

When `obs_linear_phase = on`, the IIR SVF pipeline is replaced by FFT-domain filtering:

- **FFT size:** 4096 samples (configurable: 2048 for lower latency, 8192 for higher frequency resolution)
- **Overlap:** 75% (3/4 overlap, Hann window — produces flat summing with 50% overlap for perfect reconstruction at 75%)
- **Latency:** FFT_size / 2 = 2048 samples at default settings (~42 ms at 48 kHz). Reported to host via `getLatencySamples()`.
- **Algorithm:** Overlap-add. The filter's frequency response is computed from the 6 SVF bands and inverse-FFT'd to produce an FIR impulse response, then convolved in the frequency domain.
- **Drawbacks communicated to user:** Pre-ringing visible in the waveform display for cuts; latency compensation required in DAW; not suitable for live monitoring.
- **Oscar character in linear phase mode:** The saturation stage (tanh + transformer HP) remains in the time domain, applied to the overlap-add output. Linear phase addresses filter phase only; Oscar harmonic color is additive and time-domain.

### 4.4 Mid/Side Matrix

```cpp
// Encode L/R to M/S
float mid  = (L + R) * 0.7071f;  // * 1/sqrt(2) for power consistency
float side = (L - R) * 0.7071f;

// Apply EQ independently to mid and side channels
// ...process mid through its band-scoped filters...
// ...process side through its band-scoped filters...

// Decode M/S back to L/R
L = (mid + side) * 0.7071f;
R = (mid - side) * 0.7071f;
```

Each band's `obs_b{N}_ms_scope` determines which channel(s) that band processes. When scope = Stereo, the band's coefficients are applied to both mid and side independently. When scope = Mid, only the mid channel is processed by that band. When scope = Side, only the side channel.

This enables precise control: e.g., a 200 Hz cut applied to Mid only reduces muddiness in the center image without touching the low-end stereo information from room mics.

**M/S enable guard:** When `obs_ms_enable = off`, the M/S matrix encode/decode is bypassed entirely and `obs_b{N}_ms_scope` is ignored.

### 4.5 Dynamic EQ: Envelope Follower Per Band

Dynamic EQ converts a static band into a compressor/expander that only acts on that frequency region:

```cpp
struct DynamicEQState {
    float envLevel;     // current envelope estimate (linear amplitude)
    float gainMod;      // current gain modification (dB, smoothed)
};
```

**Signal path per dynamic band:**
1. The band's SVF extracts a sidechain signal (bandpass output centered on the band's frequency, Q = 1.0, fixed — not affected by the band's Q setting).
2. The sidechain signal is half-wave rectified and fed to an analog-modeled envelope follower:
   ```cpp
   // Attack/release envelope follower (one-pole, per-sample)
   float att = std::exp(-1.0f / (attackMs * 0.001f * sampleRate));
   float rel = std::exp(-1.0f / (releaseMs * 0.001f * sampleRate));
   envLevel = (rectified > envLevel) ? att * envLevel + (1 - att) * rectified
                                     : rel * envLevel + (1 - rel) * rectified;
   ```
3. The envelope level (converted to dBFS) is compared against threshold. When above threshold:
   - Compress mode: gain is reduced by `(level - thresh) * (1 - 1/ratio)` dB
   - Expand mode: gain is boosted by `(level - thresh) * (1 - 1/ratio)` dB
4. The resulting gain offset modifies the band's `obs_b{N}_gain` parameter (added on top of the static setting, not replacing it).
5. Gain offset is smoothed with a one-pole lowpass at 20 Hz to prevent high-frequency amplitude modulation artifacts.

**Self-sidechain only initially.** External sidechain routing (e.g., ONSET kick triggering Band 1 compression) is handled via the coupling system (see §6).

### 4.6 Spectral Analyzer

- **FFT size:** 2048 samples
- **Window:** Hann (sidelobe attenuation -31 dB; adequate for musical visualization)
- **Overlap:** 50% (75% for smoother animation at cost of CPU)
- **Refresh rate:** 30 fps (1 FFT frame per ~33 ms, interleaved with audio processing on message thread)
- **Display:** Log-frequency X axis (20 Hz – 22 kHz), -90 to +6 dBFS Y axis
- **Averaging:** Temporal smoothing per bin: fast mode (3-frame average), slow mode (10-frame average)
- **Peak hold:** Separate peak hold buffer with configurable decay (controlled by `obs_analyzer_decay`)
- **Pre/Post view:** A toggle shows the analyzer before or after all EQ bands. The delta (difference curve) between pre and post is rendered as the EQ transfer function overlay.
- **Thread safety:** FFT processing runs on the audio thread into a double-buffered display frame. The message thread reads the inactive frame for rendering. No locks — just atomic frame index.

---

## 5. Preset Categories and Factory Preset Concepts

Target: **150 factory presets** (XO_OX fleet standard). Below are 20 concepts across four moods.

### Foundation (clean utility shapes — 5 concepts)

| Name | Description | Active Bands | Character |
|------|-------------|-------------|-----------|
| **Clean Slate** | Flat line reference. HP at 30 Hz only. All feliX. | B1: HP24 @ 30 Hz | feliX = 0.0 all |
| **Broadcast Ready** | Classic radio shelf combo: +2 dB high shelf, -2 dB low-mid | B2: -2 dB @ 300 Hz, B5: +2 dB @ 10 kHz | feliX = 0.1 |
| **Deep Foundation** | Sub shelf boost for electronic music: +4 dB @ 60 Hz shelf | B1: +4 dB LoShelf @ 60 Hz, B2: -1 dB @ 200 Hz | feliX = 0.15 |
| **High Pass Only** | 80 Hz HP12, utility cleaning filter | B1: HP12 @ 80 Hz | feliX = 0.0 |
| **Telephone** | Classic telephone bandpass effect: HP @ 300 Hz, LP @ 3.4 kHz | B1: HP12 @ 300 Hz, B6: LP12 @ 3400 Hz | feliX = 0.8 (adds grit) |

### Atmosphere (gentle character, Tide active — 5 concepts)

| Name | Description | Tide | Character |
|------|-------------|------|-----------|
| **Breathing Reef** | Gentle 1 dB undulation at 1 kHz, Tide on gain, Sine shape | B3: Tide on Gain, Sine 0.3 Hz | Oscar = 0.4 |
| **Tidal Warmth** | Low shelf rises and falls like the tide: +0-3 dB @ 120 Hz | B2: Tide on Gain, 0.1 Hz Sine | Oscar = 0.6 |
| **Shimmer Surface** | High shelf shimmers with Drift shape LFO | B6: Tide on Gain, Drift 0.25 Hz | feliX = 0.3 |
| **Deep Swell** | Sub content swells in/out over 8-second cycle | B1: Tide on Gain, 0.125 Hz Tri | Oscar = 0.5 |
| **Morphing Presence** | Presence peak sweeps 2–4 kHz via Tide on Freq | B4: Tide on Freq, 0.4 Hz Sine | feliX = 0.2 |

### Prism (dramatic character, dynamic bands active — 5 concepts)

| Name | Description | Dynamic | Character |
|------|-------------|---------|-----------|
| **Bloom Gate** | Low-mid expands when kick hits: boost appears on transients | B2: Expand dyn, fast attack | Oscar = 0.7 |
| **Surgical Strike** | Dynamic notch at 800 Hz cuts resonance only when signal exceeds -12 dBFS | B3: Notch, Compress dyn | feliX = 0.05 |
| **Vintage Air** | M/S mode, high shelf boost on Mid only, Oscar saturation | B6: Mid scope, +3 dB, Oscar = 0.9 | Oscar = 0.9 |
| **Iron Heart** | Full Oscar on all bands: every band adds harmonic color, transformer warmth | All bands Oscar = 1.0 | Oscar = 1.0 |
| **Side Wide** | Side channel only high shelf boost: widens perceived stereo image | B6: Side scope, +4 dB @ 12 kHz | feliX = 0.3 |

### Flux (extreme Tide, moving EQ — 5 concepts)

| Name | Description | Tide | Character |
|------|-------------|------|-----------|
| **Filter Surf** | Frequency sweep from 200 Hz to 2 kHz over 2-second cycle, all bands riding | B3: Tide on Freq, 0.5 Hz deep | Oscar = 0.5 |
| **Wobble Bass** | Sub gain oscillates ±6 dB at 4 Hz (tremolo in the Trench zone) | B1: Tide on Gain, 4 Hz Sine | Oscar = 0.8 |
| **Storm Cell** | All six bands in Tide mode simultaneously at different rates | All bands: Tide on Gain, 0.1–2 Hz | Oscar = 0.6 |
| **Q Tremor** | Q modulated on Band 3: between wide and razor-narrow at 1 Hz | B3: Tide on Q, 1 Hz Tri | feliX = 0.4 |
| **Character Ride** | Tide targets Character on Band 4: sweeps between feliX and Oscar on presence band | B4: Tide on Character, 0.2 Hz Sine | sweeping |

---

## 6. Coupling Design

XObserve is an FX processor engine, not a synthesis engine. Its coupling model is therefore receiver-heavy on the input side and provides spectral analysis data on the output side.

### 6.1 Coupling Inputs Accepted (`applyCouplingInput`)

| CouplingType | XObserve Behavior | Example |
|---|---|---|
| `AmpToFilter` | Source amplitude modulates target band's gain (acts as sidechain compressor). Maps to `obs_coupling_target` selected band. Negative amount = ducking. | ONSET kick → AmpToFilter → Band 1 gain ducking. Classic parallel compression sidechain. |
| `RhythmToBlend` | Source rhythm gate modulates the Mix parameter. Rhythm on = wet, rhythm off = dry. Effective for gated EQ timbral effects. | ONSET groove pattern → gated EQ blend |
| `EnvToMorph` | Source envelope modulates global `obs_character_global`. Attack of source pulls toward Oscar; sustain returns toward feliX. | DUB reverb tail → character sweep |
| `LFOToPitch` | Source LFO frequency modulates the selected target band's frequency parameter. Treats band frequency as a pitch-analogue target. | ODYSSEY LFO → Band 3 freq sweep |
| `AudioToFM` | Source audio drives the Tide depth of the selected band. Audio amplitude = Tide depth scaling. High-energy source = deeper modulation. | ONSET transient energy → Tide depth |
| `FilterToFilter` | Source filter output feeds into XObserve's sidechain detector for the dynamic EQ. Allows a coupled engine's filtered output to be the key signal. | OBESE filtered bass → dynamic EQ key |
| `AmpToChoke` | Source amplitude above threshold bypasses all XObserve processing (output = dry). Choke mode for performance muting of EQ character. | SNAP note-off → bypass character |
| `EnvToDecay` | Source envelope → Tide rate on selected band. Long envelope = slow Tide. Short envelope = fast Tide. | OPAL grain density → Tide rate |

**Implementation:** `applyCouplingInput()` accumulates modulation into per-coupling-type accumulators. These are consumed at the start of `renderBlock()` and added to (not replacing) the relevant parameter values. Modulation is latched for the block duration.

### 6.2 Coupling Outputs Exposed (`getSampleForCoupling`)

XObserve exposes spectral analysis data as coupling outputs:

| Channel | Content | Type |
|---------|---------|------|
| `ch0` | Pre-EQ audio (left/mid) | Audio |
| `ch1` | Post-EQ audio (left/mid) | Audio |
| `ch2` | Spectral centroid (brightness, 0–1) | Scalar |
| `ch3` | Band 1–3 total energy (low-end mass, 0–1) | Scalar |
| `ch4` | Band 4–6 total energy (high-end air, 0–1) | Scalar |
| `ch5` | Dynamic EQ gain reduction (dBFS, signed) | Scalar |

These outputs enable reverse coupling: XObserve's spectral analysis feeding back into synthesis engines. An engine coupled to `ch2` (centroid) could track brightness and self-modulate as the EQ shapes it.

### 6.3 ONSET → XOBSERVE Coupling (Designed Use Case)

This is the flagship coupling scenario:

- **Type:** `AmpToFilter`
- **ONSET output:** ch2 (perc envelope, amplitude per-kick trigger)
- **XObserve response:** Band 1 gain ducking by amount = coupling_amount * obs_coupling_gain
- **Result:** Every kick hit from ONSET ducks the low-end of whatever audio XObserve is processing. Classic parallel compression / sidechain pumping, but with:
  - The Oscar character on Band 1 meaning the duck itself has harmonic color (the transient is warm, not clinical)
  - Tide modulation still active during the release phase of the duck
  - Dynamic EQ threshold determining when kicks are "big enough" to trigger full duck vs. light touch

**Practical preset name:** "Kick Duck Warmth" — preset that ships in the Entangled mood demonstrating this routing.

### 6.4 Coupling Types XObserve Does NOT Accept

- `AudioToRing` — XObserve has no ring modulator
- `PitchToPitch` — XObserve has no pitch tracking output
- `AudioToWavetable` — XObserve has no wavetable
- `AudioToBuffer` — Not applicable (XObserve reads coupled audio via normal audio routing, not buffer injection)

---

## 7. XPN Integration

### 7.1 Oxport Handling of XObserve

XObserve is an FX engine applied to a synthesis engine's output. When Oxport renders a kit:

**Recommended path: Bake EQ into rendered audio.**

The Oxport pipeline should apply XObserve's EQ curve to the offline render:

```
Engine render → raw buffer → XObserve offline process → baked buffer → .wav sample
```

This produces XPN samples that carry the tonal character of the EQ preset even on MPC hardware that cannot run XObserve. The XPN kit therefore sounds identical to the in-plugin result.

**Implementation:** XObserve must expose a `processOffline(juce::AudioBuffer<float>&, double sampleRate)` method that applies the current EQ state without Tide LFO modulation (static snapshot). Oxport calls this after the engine render.

**Tide modulation in Oxport:** For presets with Tide LFO active, Oxport renders the full Tide cycle as a time-varying EQ applied to the sample. The result is a single .wav file with the modulation baked in. This preserves the artistic intent of Tide presets at the cost of the sample not being re-playable at different note lengths. Oxport log should note "Tide baked" in the manifest.

### 7.2 EQ Settings as XPN Metadata

In addition to the baked audio, the XPN `.xpnmeta` bundle includes:

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
    "linear_phase": false
  }
}
```

This metadata serves two purposes:
1. Reference documentation — sound designers can see the EQ settings that produced the sound
2. Future XObserve-aware MPC firmware could apply the EQ non-destructively

### 7.3 Cover Art: Spectral Fingerprint

Following the XOptic–Oxport integration spec pattern, XObserve exposes its frequency response curve as a visual input for cover art generation.

The EQ transfer function (gain vs. frequency) is computed at 512 log-spaced frequency points and exported as an `ObserveFingerprint`:

```json
{
  "type": "xobserve_fingerprint",
  "transfer_curve": [0.0, 0.3, 1.2, -0.5, ...],  // 512 dB values
  "centroid_shift": -0.15,    // net change in spectral centroid (negative = darker)
  "character_mean": 0.45,     // average Oscar-ness across active bands
  "tide_active": true,
  "dynamic_active": false
}
```

`xpn_cover_art.py` consumes the `transfer_curve` to draw an actual EQ curve as a structural element in the cover art. A V-shaped cut becomes a V in the cover geometry. A high-shelf boost becomes an ascending arc. The artwork is literally derived from the EQ shape — the most direct possible expression of "sound as image."

---

## 8. UI Wireframe (Text Description)

### 8.1 Overall Layout

**Dimensions:** Standard plugin window, 800 × 500 px default (resizable 1×–1.5×).

**Three horizontal zones:**
1. **Top bar** (height 48 px): Plugin name / logo left, preset browser center, input/output gain knobs right, bypass toggle right edge
2. **Main area** (height 350 px): Spectrum analyzer with band handles overlaid
3. **Bottom bar** (height 102 px): Per-band parameter strip, global controls, coupling strip

### 8.2 Main Area — Spectrum Analyzer

- Background: Deep navy-to-black gradient, ocean depth atmosphere
- Pre-EQ spectrum: translucent grey fill (like XO_OX water under surface)
- Post-EQ spectrum: warm amber fill (`#E8A020` at 30% opacity) where gain is boosted; cooler blue where cut
- EQ transfer function: bright amber line (`#E8A020`) rendered as continuous curve connecting all active band gain points
- **Aquatic zone labels:** Ghosted text along the frequency axis at zone boundaries:
  - "TRENCH" at 20–80 Hz (deep navy glow)
  - "ABYSS" at 80–250 Hz (indigo glow)
  - "REEF" at 250–2000 Hz (teal glow)
  - "SURFACE" at 2000–8000 Hz (golden glow)
  - "SKY" at 8000–22000 Hz (pale cyan glow)
- **Band handles:** Six glowing dots on the EQ transfer curve. Each dot:
  - Size proportional to Q (wider Q = larger dot)
  - Color ranges feliX (cool blue `#00A6D6`) to Oscar (warm amber `#E8A020`) based on character slider
  - Tide active: dot pulses with a slow glow animation at the Tide rate
  - Dynamic active: dot has a subtle ring that compresses/expands with the dynamic gain reduction
  - Draggable: vertical drag = gain, horizontal drag = frequency, Shift+drag = Q
  - Double-click = open band detail popover
- **Grid lines:** Horizontal at 0, ±3, ±6, ±12, ±18 dB (subtle). Vertical at decade boundaries.

### 8.3 Band Detail Popover

Right-click or double-click a band handle opens a floating panel:

```
[ Band 3 — REEF ]
Freq:    [800 Hz     ]   Type: [ Peak         ▼]
Gain:    [+2.3 dB    ]   Q:    [1.40          ]
Scope:   [ Stereo    ▼]  Bypass: [ ]

Character: [feliX ——●—————————————————— Oscar]
              clinical                   warm

Tide:  [✓ Enable]  Rate: [0.5 Hz]  Depth: [30%]
       Shape: [Sine ▼]   Target: [Gain ▼]

Dynamic: [ Enable]  Thresh: [-12 dB]  Ratio: [4:1]
         Attack: [10 ms]   Release: [100 ms]  Mode: [Compress ▼]
```

### 8.4 Band Parameter Strip (Bottom Left — 60% width)

Six columns, one per band, each showing:
- Band number (1–6) and zone icon (depth icon: anchor for Trench, fish for Abyss, coral for Reef, wave for Surface, bird for Sky)
- Frequency value (small numeric readout)
- Gain value with ±indicator
- Character mini-slider (thin horizontal bar, blue–amber gradient)
- Tide indicator dot (pulsing if active)
- Dyn indicator dot (breathing if active)
- Bypass toggle (circle with line)

Clicking any element in a band column opens that band's detail popover.

### 8.5 Global Controls (Bottom Right — 40% width)

Two rows:
- Row 1: Input Gain knob, Mix knob, Output Gain knob, M/S enable toggle, Linear Phase toggle
- Row 2: Tide Master Rate knob, Tide Sync toggle, Oversampling selector, Phase Scope mini-display (Lissajous), True Peak limiter toggle

### 8.6 Character Global Strip

A thin horizontal bar spanning the full width between the main area and the bottom bar. This is the Global Character slider — dragging it left/right offsets all bands' character simultaneously. The bar gradient goes from feliX cool blue (left) to Oscar warm amber (right). The current global position is shown as a vertical line on the gradient. Label: "CHARACTER — feliX ←————— Oscar"

### 8.7 Coupling Strip (visible when inside XOceanus)

A collapsed panel at the very bottom, revealed when XObserve detects it is running as an XOceanus slot. Shows:
- Active coupling route (e.g., "ONSET → Band 1 Gain [AmpToFilter]")
- Coupling gain knob
- Coupling target selector

---

## 9. Macros (4 Required — Doctrine D002)

| Macro | ID | Range | What It Does |
|-------|----|-------|--------------|
| **M1: SHAPE** | `obs_macro_shape` | 0–1 | Morphs from flat (0) to a musically opinionated curve (1): Trench boost, Abyss cut, Reef scoop, Surface air boost. A "taste" macro. |
| **M2: TIDE** | `obs_macro_tide` | 0–1 | Master depth control for all active Tide LFOs simultaneously. 0 = no modulation, 1 = full depth |
| **M3: CHARACTER** | `obs_macro_character` | 0–1 | Aliases `obs_character_global`. The feliX↔Oscar macro. |
| **M4: DYNAMICS** | `obs_macro_dynamics` | 0–1 | Master threshold offset for all dynamic bands (0 = all dyn bands at preset threshold, 1 = thresholds raised by 12 dB = less dynamic action). A "how reactive" macro. |

---

## 10. Aquatic Identity

- **Creature:** Mantis shrimp (Stomatopod)
- **Mythology:** The mantis shrimp has 16 photoreceptor types compared to the human's 3. It doesn't see more color than humans — it categorizes and acts on spectral data faster. XObserve observes the frequency spectrum with mantis-shrimp precision and acts on it with the speed of a 23 m/s punch. The EQ is not passive. It reacts.
- **feliX expression:** The mantis shrimp's UV perception — clinical, beyond-human, precise
- **Oscar expression:** The smashing claw — the analog, mechanical, destructive warmth of physical force
- **Zone mascots:** Anchor (Trench), Anglerfish (Abyss), Coral polyp (Reef), Flying fish (Surface), Albatross (Sky)

---

## 11. Doctrine Compliance Checklist

| Doctrine | Compliance |
|----------|-----------|
| D001: Velocity shapes timbre | N/A — FX engine. Velocity could be wired to `obs_character_global` via note input if XObserve receives MIDI. Optional V2 feature. |
| D002: Modulation is lifeblood | PASS — 4 macros, Tide LFO per band with 7 shapes, coupling modulation inputs |
| D003: Physics IS the synthesis | PASS — Cytomic SVF documented with theoretical basis; transformer emulation matched-Z only |
| D004: Dead parameters forbidden | PASS — 126 params, all wired; Tide and dynamic params guarded by enable flags but audible when enabled |
| D005: Engine must breathe | PASS — Tide LFO per band, minimum rate 0.01 Hz; Global Tide Master Rate exists |
| D006: Expression input not optional | PASS — Coupling inputs handle AmpToFilter, EnvToMorph etc.; mod-wheel could route to CHARACTER global |

---

## 12. Engine Registration Data

```cpp
// In EngineRegistry.h registration:
REGISTER_ENGINE("XObserve", []() -> std::unique_ptr<SynthEngine> {
    return std::make_unique<ObserveEngine>();
});

// Engine identity:
// getEngineId() → "OBSERVE"
// getAccentColour() → juce::Colour(0xFFE8A020)  // Spectral Amber
// getMaxVoices() → 1 (mono/stereo FX, no voice concept)
// Parameter prefix: "obs_"
```

---

## 13. Build Sequencing (Recommended)

| Phase | Deliverable | Notes |
|-------|-------------|-------|
| Phase 0 | Concept (this document) | COMPLETE |
| Phase 1 | `ObserveEngine.h` scaffold: SynthEngine interface, 126 params, no DSP | Standalone builds, silence output |
| Phase 2 | Cytomic SVF for 6 bands, feliX mode only | Target: all filter types working, no character |
| Phase 3 | Oscar character: tanh + transformer HP + all-pass phase | Target: character slider audibly moves between clean and warm |
| Phase 4 | Tide LFO per band | Target: modulation shapes audible |
| Phase 5 | Dynamic EQ per band | Target: sidechain detection + gain reduction working |
| Phase 6 | Mid/Side processing | Target: M/S matrix encode/decode, per-band scope |
| Phase 7 | Linear phase (FIR) mode | Target: latency reporting, overlap-add, phase linearity verified |
| Phase 8 | Spectral analyzer (display only) | Target: FFT display updating at 30 fps |
| Phase 9 | Coupling I/O | Target: AmpToFilter, RhythmToBlend, spectral centroid output |
| Phase 10 | Factory presets (150) | Target: all 4 mood categories populated |
| Phase 11 | Oxport integration (bake path) | Target: Oxport calls processOffline(); metadata included |

---

*Authored: XO_OX Designs, March 2026*
*Document status: Canonical technical design. Ready for Phase 1 scaffold.*
