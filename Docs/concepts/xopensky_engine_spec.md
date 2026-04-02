# XOpenSky Engine Specification

**Date:** March 16, 2026
**Phase:** 1 — Architecture R&D
**Status:** V1 Scope — DSP build pending
**Gallery code:** OPENSKY
**Accent color:** Sunburst `#FF8C00`
**Parameter prefix:** `sky_`
**Aquatic identity:** Above the water column — the open sky where feliX ascends fully into light

---

## 1. Concept

XOpenSky is the euphoric synthesis engine. Shimmering supersaw anthems, crystalline bell pads, soaring leads that feel like breaking through clouds. Other engines go deep, go weird, go aggressive. OpenSky goes up.

Every other XOceanus engine lives in the water. OpenSky is what happens when you leave it. The neon tetra — feliX — has broken the surface completely. For one shimmering moment, neon scales catch the full spectrum of sunlight. The entire water column lies below. Nothing but air and light above. This is what the surface looks like from the other side.

XOpenSky is the engine that completes the top of the vertical axis. It pairs with XOceanDeep (pure Oscar, the floor) as the engine that completes the bottom. Together they span the full water column mythology — from the Trench Violet of the abyssal floor to the Sunburst of the open sky.

### Design Lineage

**Arp String Ensemble (1974)** — the first synthesizer to use high-density detuned chorus across multiple oscillators to simulate the shimmer of a string section. Not accurate. More beautiful than accurate. Chorusing as a primary synthesis technique rather than an effect.

**Roland Juno string machine brightness** — specifically the Juno-106's chorus section (CCO architecture, a single chorus BBD running on all voices) and the high-frequency brightness that comes from a chorus applied before the resonant filter. The top-end sparkle that no digital recreation quite captures — because the sparkle was an artifact, not a feature.

**PPG Wave upper harmonics** — Wolfgang Palm's digital-analog hybrid architecture pushed harmonic content into the upper registers in a way pure analog synthesis rarely reached. The PPG Wave 2.3 at maximum harmonic settings sounds like something trying to break out of the speaker cone. OPENSKY's harmonic exciter stage is in direct conversation with this.

**Eventide Harmonizer shimmer** — Brian Eno and Daniel Lanois's discovery that pitch-shifted reverb feedback — a slight octave-up shift fed back into a reverb — created infinite harmonic ascension. "No Such Thing as Silence" shimmer. The sky opening above you.

### What This Engine Is Not

OPENSKY is not a general-purpose polyphonic synth. It is not a workstation. It is not trying to be neutral. It is a character instrument with a single clear personality: euphoric ascension. Every design decision should reinforce this identity.

A preset that sounds muddy, dark, or aggressive is a failed preset for this engine. The goal is always upward.

---

## 2. Architecture

### 2.1 Signal Path

```
SUPERSAW OSCILLATOR STACK
├── 7-voice unison per note (sky_unisonVoices: 1–7)
├── Per-voice detune: sky_unisonDetune (0–100 cents spread)
├── Waveform: sky_oscWave (Saw / Pulse / Sine)
├── Stereo scatter: per-voice pan randomized within sky_stereoSpread
│
▼
HARMONIC SHIMMER STAGE
├── Octave-up pitch shifter (semitone: +12, mix: sky_shimmerAmount)
├── Shimmer feedback reverb (tail: sky_shimmerDecay)
├── Shimmer tone: sky_shimmerTone (dark shimmer ↔ crystalline)
│
▼
BRIGHT FILTER (Cytomic SVF, key-tracked)
├── High-shelf boost: sky_airAmount (8–12 kHz presence exciter)
├── Resonant HPF: sky_filterCutoff, sky_filterReso
├── Filter envelope: fast attack for leads, slow sweep for pads
│
▼
AMP ENVELOPE
├── sky_attack: 0.001–4s (fast lead to slow pad)
├── sky_decay / sky_sustain / sky_release
│
▼
FX CHAIN
├── Chorus (sky_chorusMix — width, lush, "sky-width")
├── Cathedral Reverb (sky_reverbMix, sky_reverbSize)
├── Stereo Widener (sky_stereoWidth)
│
▼
Output (stereo, 16-voice poly)
```

**Total oscillator count at maximum settings:** 7 (unison) × 16 (voices) = 112 oscillators. CPU management via quality mode — high quality runs all voices, performance mode caps unison at 4 voices and voices at 8.

### 2.2 Supersaw Oscillator Stack

The fundamental building block of OPENSKY. Each MIDI note spawns `sky_unisonVoices` oscillators detuned across a spread of `sky_unisonDetune` cents, distributed logarithmically (more voices at the extremes, fewer in the center — this is why the classic supersaw is wide without sounding nasal).

Per-voice stereo scatter pans each unison voice to a position within `sky_stereoSpread`. The center unison voice is always panned to center — this preserves mono compatibility. Extreme voices are panned hard.

Waveform selection (`sky_oscWave`):
- **Saw** — classic, dense, full harmonic spectrum. Arena anthem mode.
- **Pulse** — narrower, more hollow, cuts through mixes differently. Crystal mode.
- **Sine** — minimal harmonics, pure, works best with shimmer to generate overtones from clean source.

### 2.3 Harmonic Shimmer Stage

The shimmer stage is OPENSKY's defining feature. It is an octave-up pitch shifter feeding a short reverb with feedback, creating cascading harmonic reflections.

**Pitch shift:** exact +12 semitones (or selectable +7/+12/+24 via `sky_shimmerInterval`). The shifted signal is added at `sky_shimmerAmount` — at 0 the shimmer is off, at 1 it equals the dry signal level.

**Shimmer decay:** `sky_shimmerDecay` controls the feedback tail length — short (0) for a brief harmonic flash, long (1) for infinite ascending shimmer.

**Shimmer tone:** `sky_shimmerTone` is a one-parameter EQ applied to the shimmer signal only. At 0, the shimmer is slightly dark and warm (à la Eno/Lanois). At 1, it is crystalline — a narrow peak boost at 6kHz and a shelf rise above 10kHz.

**Denormal protection required** in the shimmer feedback path — the pitch-shifted reverb feedback must be guarded before accumulation.

### 2.4 Bright Filter

A Cytomic SVF implementation (key-tracked, zero-delay feedback). Configured as a high-pass filter with resonance — this is unusual (most synth filters are low-pass) but is exactly right for OPENSKY: the engine lives in the high frequencies.

`sky_filterCutoff` ranges from 80 Hz to 20 kHz with exponential scaling. The filter envelope (`sky_filterAttack`, `sky_filterDecay`, `sky_filterEnvAmount`) is designed for two modes:
- **Lead mode:** fast attack, short decay, high envelope amount. The filter snaps open on attack, briefly reveals full brightness, then settles to the cutoff.
- **Pad mode:** slow attack, no envelope, cutoff set statically high.

`sky_airAmount` drives a separate high-shelf exciter on the filter output — an analog-modeled presence boost in the 8–12 kHz band. This is the "air" quality of bright condenser microphones and expensive analog chains — not achievable with just filter cutoff alone.

### 2.5 FX Chain

The FX chain is integral to OPENSKY's identity — unlike some engines where FX are post-synthesis flavor, OPENSKY's FX are part of the voice architecture.

**Chorus** (`sky_chorusMix`): A lush, wide chorus with three delay lines (1.2ms, 1.7ms, 2.3ms) and a slow LFO (0.2–0.8 Hz). This is the Juno-style chorus — not subtle, intentionally obvious, wideness as a feature.

**Cathedral Reverb** (`sky_reverbMix`, `sky_reverbSize`): A long-tail algorithmic reverb tuned for brightness. Damping is minimal at small sizes, increasingly bright at large sizes — the sky is not a padded room. Decay time ranges from 1.2s (chapel) to 12s (open sky, effectively infinite).

**Stereo Widener** (`sky_stereoWidth`): M/S widening post-reverb. At 0, dry mono core. At 1, maximum width. Combined with the unison stereo scatter, this makes OPENSKY presets fill the full stereo field.

---

## 3. Parameter List (~75 parameters, `sky_` prefix)

### 3.1 Oscillator Parameters

| Parameter ID | Range | Default | Description |
|-------------|-------|---------|-------------|
| `sky_oscWave` | 0–2 (enum) | 0 | Saw / Pulse / Sine |
| `sky_oscPulseWidth` | 0.05–0.95 | 0.5 | Pulse width (active when sky_oscWave = 1) |
| `sky_unisonVoices` | 1–7 | 7 | Unison voice count per MIDI note |
| `sky_unisonDetune` | 0–100 cents | 18 | Detune spread across unison stack |
| `sky_unisonStereoSpread` | 0–1 | 0.85 | Stereo scatter width of unison voices |
| `sky_unisonPhaseReset` | 0/1 | 0 | Reset oscillator phase on note-on |
| `sky_octave` | -2–+2 | 0 | Octave transposition |
| `sky_fine` | -50–+50 cents | 0 | Fine tuning |
| `sky_pitchBendRange` | 1–24 semitones | 2 | Pitch bend range |

### 3.2 Shimmer Stage Parameters

| Parameter ID | Range | Default | Description |
|-------------|-------|---------|-------------|
| `sky_shimmerAmount` | 0–1 | 0.55 | Octave-up shimmer mix level |
| `sky_shimmerDecay` | 0–1 | 0.6 | Shimmer tail length (short flash → infinite) |
| `sky_shimmerTone` | 0–1 | 0.7 | Dark shimmer ↔ crystalline shimmer |
| `sky_shimmerInterval` | 0–2 (enum) | 1 | +7 semitones / +12 / +24 |
| `sky_shimmerFeedback` | 0–0.85 | 0.4 | Shimmer feedback amount (clamped for stability) |

### 3.3 Filter Parameters

| Parameter ID | Range | Default | Description |
|-------------|-------|---------|-------------|
| `sky_filterCutoff` | 80–20000 Hz | 4000 | High-pass filter cutoff |
| `sky_filterReso` | 0–1 | 0.15 | Filter resonance |
| `sky_filterKeyTrack` | 0–1 | 0.5 | Key tracking amount (1 = full, cutoff follows pitch) |
| `sky_filterEnvAmount` | 0–1 | 0.35 | Filter envelope depth |
| `sky_filterAttack` | 0.001–2s | 0.01 | Filter envelope attack |
| `sky_filterDecay` | 0.01–4s | 0.3 | Filter envelope decay |
| `sky_airAmount` | 0–1 | 0.6 | 8–12 kHz air presence exciter |

### 3.4 Amplitude Envelope Parameters

| Parameter ID | Range | Default | Description |
|-------------|-------|---------|-------------|
| `sky_attack` | 0.001–4s | 0.008 | Amp attack |
| `sky_decay` | 0.01–4s | 0.5 | Amp decay |
| `sky_sustain` | 0–1 | 0.8 | Amp sustain |
| `sky_release` | 0.01–8s | 1.2 | Amp release |

### 3.5 Modulation Parameters

| Parameter ID | Range | Default | Description |
|-------------|-------|---------|-------------|
| `sky_lfoRate` | 0.01–8 Hz | 0.15 | Shimmer LFO rate |
| `sky_lfoDepth` | 0–1 | 0.3 | Shimmer LFO depth (modulates sky_shimmerAmount) |
| `sky_lfoShape` | 0–2 (enum) | 1 | Sine / Triangle / S&H |
| `sky_lfoTempoSync` | 0/1 | 0 | Sync LFO to DAW tempo |
| `sky_harmonicDrift` | 0–1 | 0.2 | Slow random drift in shimmer interval (micro-flutter) |
| `sky_vibratoRate` | 0.01–8 Hz | 4.5 | Pitch vibrato rate |
| `sky_vibratoDepth` | 0–50 cents | 0 | Pitch vibrato depth |
| `sky_modWheelTarget` | 0–3 (enum) | 0 | Mod wheel → shimmer / vibrato / filter / air |
| `sky_aftertouchTarget` | 0–2 (enum) | 2 | Aftertouch → shimmer / filter / air |
| `sky_velocityToFilter` | 0–1 | 0.5 | Velocity scales filter brightness |
| `sky_velocityToAmp` | 0–1 | 0.8 | Velocity scales amplitude |

### 3.6 FX Parameters

| Parameter ID | Range | Default | Description |
|-------------|-------|---------|-------------|
| `sky_chorusMix` | 0–1 | 0.6 | Chorus wet/dry |
| `sky_chorusRate` | 0.1–2 Hz | 0.4 | Chorus LFO rate |
| `sky_chorusDepth` | 0–1 | 0.5 | Chorus modulation depth |
| `sky_reverbMix` | 0–1 | 0.5 | Reverb wet/dry |
| `sky_reverbSize` | 0–1 | 0.7 | Room size (chapel → open sky) |
| `sky_reverbPreDelay` | 0–100ms | 15 | Pre-delay for definition |
| `sky_reverbDamping` | 0–1 | 0.1 | High-frequency damping (0 = bright sky, 1 = dark room) |
| `sky_stereoWidth` | 0–1 | 0.8 | M/S stereo widening |

### 3.7 Macro Parameters (M1–M4)

| Macro | ID | Label | Controls | Behavior |
|-------|-----|-------|----------|----------|
| M1 | `sky_macroRise` | RISE | `sky_shimmerAmount`, `sky_airAmount`, `sky_filterCutoff` (up), `sky_octave` (soft push) | 0 = warm, grounded, just above the water. 1 = pure crystalline ascension, everything bright, harmonic shimmer at full. The altitude dial. |
| M2 | `sky_macroWidth` | WIDTH | `sky_unisonDetune`, `sky_chorusMix`, `sky_stereoWidth`, `sky_unisonStereoSpread` | 0 = focused beam of light. 1 = fills the entire sky, wall of sound. The expansion dial. |
| M3 | `sky_macroGlow` | GLOW | `sky_shimmerDecay`, `sky_shimmerFeedback`, `sky_reverbMix`, `sky_reverbSize` | 0 = dry, present, immediate. 1 = infinite shimmer tail, the note plays long after you release. The afterglow. |
| M4 | `sky_macroBreathe` | BREATH | `sky_lfoDepth`, `sky_harmonicDrift`, `sky_reverbSize`, `sky_lfoRate` (inverse) | 0 = static, eternal, crystalline stillness. 1 = slow breathing, organic drift, the sky is alive. The Oscar influence in an otherwise feliX engine. |

---

## 4. DSP Approach

### 4.1 Supersaw Implementation

Each unison voice is a bandlimited sawtooth oscillator (BLIT or polynomial BLEP anti-aliasing — never naive modulo saw). Phase is distributed using a golden ratio scatter to avoid beating artifacts between voices that share similar detune amounts.

Detune distribution: for N voices, the detune offsets are spread as:
```
offset[i] = detuneCents * (2.0f * i / (N - 1) - 1.0f) * detuneSpreadCurve
```
where `detuneSpreadCurve` is slightly exponential (more voices near the extremes) — this is what gives the supersaw its characteristic width-without-thinness.

Per-voice pan scatter: voice 0 (center) is always at pan=0. Voices are paired symmetrically. Pan values distributed within `sky_unisonStereoSpread`.

### 4.2 Shimmer Stage DSP

The shimmer stage implements a pitch-shifted reverb using a phase vocoder approach for the pitch shift:
- Short-time Fourier transform per block
- Phase rotation by the frequency ratio (2× for +12 semitones)
- Inverse STFT for reconstruction
- Output fed into a feedback reverb (Schroeder-style with allpass diffusers)

Feedback gain is parameterized as `sky_shimmerFeedback` and clamped to 0.85 to prevent instability. A subtle high-shelf cut on each feedback loop iteration controls self-oscillation and contributes to the crystalline vs. warm tone.

**Denormal protection required** in all feedback paths. Use the standard guard before each feedback accumulation.

### 4.3 Cytomic SVF Filter

Zero-delay-feedback state-variable filter, key-tracked. The HPF configuration means low frequencies are attenuated and high frequencies pass — this is the correct configuration for OPENSKY's bright character.

Key tracking uses `sky_filterKeyTrack` to move the cutoff proportionally with MIDI note frequency. At full key tracking (1.0), cutoff follows pitch exactly — ensuring the "air" quality is preserved across all registers. At 0, cutoff is fixed regardless of pitch.

Filter coefficient computation uses matched-Z transform (never Euler approximation — this is mandatory per CLAUDE.md). Sample rate must be read from the audio context, never hardcoded to 44100.

### 4.4 Velocity-to-Timbre Mapping (Doctrine D001)

Velocity drives filter brightness (`sky_velocityToFilter`) and shimmer amount. Soft velocities produce warm, muted pads with minimal shimmer. Hard velocities reveal full brightness and shimmer — the note cuts through and glows.

This is implemented as a velocity-scaled multiplier on `sky_filterCutoff` (above the static setting) and a velocity-scaled boost on `sky_shimmerAmount`. The mapping curve is slightly exponential — most of the velocity response happens in the upper half of the velocity range.

### 4.5 Cathedral Reverb

A parallel comb filter + allpass diffuser reverb tuned for brightness:
- 8 parallel comb filters with prime-number delay lengths (to avoid metallic resonance)
- Damping coefficient near zero for sky-appropriate brightness
- Pre-delay (`sky_reverbPreDelay`) separates the dry signal from the reverb onset — this creates definition without compromising tail length
- Decay time derived from `sky_reverbSize`: small = 1.2s, large = 12s

**Cultural reference:** the reverb at maximum size (`sky_reverbSize = 1`) should evoke the sense of sound in open air — not a room, not a hall, but the open sky. No ceiling. Infinite height. The design target is the feeling of playing outside on a clear day, not the feeling of a cathedral interior.

---

## 5. feliX/Oscar Polarity

OPENSKY is almost entirely feliX. It sits at the extreme positive pole of the feliX/Oscar axis — brighter, higher, more crystalline, more euphoric.

The only Oscar presence is macro M4 BREATH — the slow LFO modulation that introduces organic drift into the shimmer. Without BREATH, OPENSKY is purely mathematical ascension. With BREATH, it breathes. This is the one point of contact with Oscar's warmth and humanity.

**feliX maximum (BREATH = 0):** Perfectly static shimmer. Every note is identical. The shimmer tail decays at exactly the programmed rate. The sky is crystalline and eternal.

**Oscar touch (BREATH = 1):** The shimmer drifts. The LFO modulates the shimmer amount slowly (0.01–0.3 Hz range). Each note sounds slightly different from the last. The sky is alive, not a photograph.

Most OPENSKY presets should have some BREATH — a fully static sky feels more like a test tone than a musical instrument. Even 10–20% BREATH introduces enough life to feel human without sacrificing the euphoric brightness.

---

## 6. Coupling Potential

### 6.1 Signature Coupling Routes

| Route | Type | Musical Effect |
|-------|------|---------------|
| OPENSKY ↔ OCEANDEEP | `AmpToFilter` (bidirectional) | The Full Column. Sky shimmer over abyssal bass. The entire XO_OX mythology in one patch. feliX's sky above Oscar's floor. |
| OPENSKY → OPAL | `AudioToWavetable` | Shimmer source granulated into light particles. Spectral → granular. The sky dissolves into cloud. |
| OPENSKY → OVERDUB | `getSample` | Euphoric leads through dub echo delay. Vangelis shimmer through Caribbean reverb. Heavenly dub. |
| OPENSKY → OVERBITE | `AmpToFilter` | Sky energy drives bass bite — brightness calls the feral. feliX reaches down. |
| OVERWORLD (ERA bright) → OPENSKY | `EnvToMorph` | ERA crossfade drives shimmer morph — chiptune brightness ascending into sky. |

### 6.2 OPENSKY as Target

| Coupling Type | What OPENSKY Does |
|--------------|------------------|
| `AmpToFilter` | External amplitude drives `sky_airAmount` — drums make the sky flash brighter |
| `EnvToMorph` | External envelope drives `sky_macroRise` — external dynamics control the ascension |
| `LFOToPitch` | External LFO drives `sky_unisonDetune` — cross-engine width modulation |
| `AmpToVoice` | External amplitude scales shimmer amount |

### 6.3 OPENSKY as Source

OPENSKY's shimmer output can be sent as an amplitude-modulation signal to other engines. The shimmer level is a continuous signal that rises and falls with the envelope and LFO — this becomes a modulation source when routed out via the coupling matrix.

### 6.4 Coupling Types OPENSKY Should NOT Receive

- `AmpToChoke` — you do not choke the sky
- `AudioToFM` — shimmer + FM creates unmusical interference artifacts
- `AmpToNoise` — noise in the shimmer defeats the purpose

---

## 7. Historical Homage

### Arp String Ensemble (1974)

The ARP String Ensemble used divide-down oscillators (one per note, all 61 keys simultaneously available) run through a BBD (bucket-brigade device) chorus circuit. The chorus was not subtle — it was the sound. The machine was not accurate string synthesis. It was something better: the shimmer that made "Good Times" by Chic, "Don't Stop Me Now" by Queen, and "Baba O'Riley" by The Who sound the way they did.

OPENSKY's chorus section (`sky_chorusMix`) honors this directly. The default chorus rate (0.4 Hz) and depth are tuned to approximate the ARP String Ensemble's BBD chorus behavior. At maximum chorus depth, OPENSKY should sound like the ARP String Ensemble dreaming about being a synthesizer.

### Roland Juno String Brightness (1982)

The Juno-106's DCO architecture produced a slightly different brightness than the earlier Junos — cleaner, more predictable, but with a high-frequency shimmer from the chorus applied pre-filter that became an iconic sound. "Take On Me" (A-Ha), "Just Can't Get Enough" (Depeche Mode), countless house tracks of the late 80s.

The `sky_airAmount` high-shelf exciter is the OPENSKY equivalent — present-boosting, not clinical, adding the sense of air and light above the fundamental.

### PPG Wave Upper Harmonics (1982)

Wolfgang Palm's wavetable synthesis in the PPG Wave 2.3 produced harmonic content in the upper registers (6–12 kHz) that analog synthesis rarely reached. When a PPG Wave is playing a pad with maximum harmonic content, there are timbral events happening above 8 kHz that fill the listening space. This is the `sky_airAmount` shelf and the `sky_shimmerTone` crystalline setting — overtone content in the upper registers as an intentional timbral texture, not a byproduct.

### Eventide Harmonizer — Shimmer Technique (circa 1983)

Brian Eno and Daniel Lanois's use of the Eventide H949/H3000 to create pitch-shifted reverb shimmer effects on albums like "The Unforgettable Fire" (U2, 1984) and "Peter Gabriel 4 (Security)" (1982). The technique: route audio through a pitch shifter set to +1 octave, feed the output into a reverb, feed a portion of the reverb output back into the pitch shifter. The feedback loop creates ascending harmonic ghost images that accumulate into infinite brightness.

OPENSKY's shimmer stage (`sky_shimmerAmount`, `sky_shimmerDecay`, `sky_shimmerFeedback`) is a direct digital implementation of this technique. The `sky_shimmerInterval` parameter adds the ability to shift by a fifth (+7) rather than an octave — Lanois's preferred setting for a more musical, less obviously "shimmer" effect.

---

## 8. UI Concept

**The opposite of every dark synth UI.**

Where most synthesizers use dark interfaces (black panels, dark gray backgrounds, CRT-green or amber readouts), OPENSKY uses warm white and gold. The panel feels like looking at the sky through water — light refracting, golden warmth, nothing heavy or threatening.

**Color palette:** Warm white base `#FFFEF7`, sunburst orange accent `#FF8C00`, golden highlights `#E9C46A` (XO Gold). Text in deep amber `#7A4500` rather than black. The one XOceanus engine that leans into warmth and brightness as a design statement rather than just a brand accent.

**Panel concept:** The oscillator section is at the left (the horizon). The shimmer stage is at the center-top (the sky). The FX chain is at the right (the space). The four macro sliders are at the bottom in warm orange — RISE, WIDTH, GLOW, BREATH.

The shimmer stage visualization should show the octave-up content as a gentle glow above the fundamental visualization — a literal visual representation of harmonics ascending.

---

## 9. Preset Strategy

**150 factory presets** across 7 categories (fleet standard):

| Category | Count | Character |
|----------|-------|-----------|
| Arena Anthems | 22 | Van Halen jump, trance leads, festival-sized supersaw walls |
| Heaven Pads | 22 | Vangelis-inspired, slow shimmer, infinite reverb tails |
| Crystal Leads | 22 | Bright, cutting, soaring mono leads with fast attack |
| Sunrise Textures | 18 | Slow-building, dawn-energy, golden hour evolving |
| Shimmer Bells | 18 | Crystalline bell tones with shimmer tails, pitched percussion feel |
| The Full Column | 13 | Designed for OPENSKY × OCEANDEEP coupling — euphoria over depth |
| Euphoria | 10 | Maximum RISE, maximum WIDTH, maximum everything — the edge of too much |
| Other Coupling | 25 | Showcases with OVERDUB, OPAL, OVERBITE, etc. |

### Naming Convention
Names should feel like looking up, moving forward, light:
- "Break The Surface"
- "Nothing But Sky"
- "Golden Hour"
- "The Van Halen Moment"
- "Sunlight On Scales"
- "feliX Ascending"
- "Above The Water Column"
- "Eventide Heaven"
- "The Lanois Shimmer"
- "Jump"
- "Baba O'Riley Moment"
- "Infinite"

---

## 10. Doctrine Compliance Plan

| Doctrine | Requirement | OPENSKY Implementation |
|----------|------------|----------------------|
| D001 | Velocity → timbre | `sky_velocityToFilter` + velocity-scaled shimmer amount |
| D002 | Modulation lifeblood | 2 LFOs (shimmer + vibrato), mod wheel, aftertouch, 4 working macros |
| D003 | Physics rigor | Cytomic SVF (documented algorithm), Schroeder reverb (documented), shimmer (Eventide-derived) |
| D004 | No dead parameters | All 75+ params audibly wired — verify in Phase 2 |
| D005 | Breathing ≤ 0.01 Hz | `sky_lfoRate` floor at 0.01 Hz; BREATH macro enables at all times |
| D006 | Expression input | Velocity→timbre (D001), aftertouch→air/shimmer, mod wheel→shimmer/vibrato |

---

*XO_OX Designs | Engine: OPENSKY | Accent: #FF8C00 | Prefix: sky_ | V1 scope | Phase 1 R&D*
