# XOuïe Engine Specification

**Date:** March 16, 2026
**Phase:** 1 — Architecture R&D
**Status:** Initial Scope — DSP build pending
**Gallery code:** OUIE
**Accent color:** Hammerhead Steel `#708090`
**Parameter prefix:** `ouie_`
**Aquatic identity:** The thermocline — the invisible boundary between warm surface and cold deep

---

## 1. Identity

**XOuïe** (pronounced *WEE* — French: *ouïe*, the sense of hearing; also the gill slits of a fish) is XOceanus's duophonic synthesis engine. Two synthesizer voices. Two independent synthesis algorithms. One shared relationship that the player controls.

The name carries a double meaning: *ouïe* is the raw faculty of hearing — the French word that names the act of listening — and *ouïes* are the gill openings of a fish, the breathing apparatus that mediates between an animal and its world. XOuïe does both: it listens to itself, constantly monitoring the relationship between its two voices, and breathes through the interaction between them.

### Creature: The Hammerhead Shark

The hammerhead shark (*Sphyrna lewini*) is the only predator whose most distinctive feature is radical spatial separation. The cephalofoil — that T-shaped head extending up to 90cm across — positions two eyes, two nostrils, and thousands of ampullae of Lorenzini in widely separated locations. Each eye sees a completely different hemisphere. Each nostril tracks a different chemical gradient. The electroreceptors on each side of the wide head detect separate, independent electrical fields simultaneously.

This is not redundancy. This is stereoscopic intelligence — the same environment perceived from two angles, combined into a three-dimensional picture no single-sensor predator can achieve. The hammerhead knows where things are because it triangulates from two separated points of view.

XOuïe's two voices are the two halves of the cephalofoil. They can work in perfect concert — **LOVE mode**, binocular depth perception, harmonic intervals sought and locked together — or they can be in active competition — **STRIFE mode**, two signals fighting for frequency space, repelling each other, generating dissonance as a texture. The two macros STRIFE and LOVE control which state dominates.

**Position in the water column:** The thermocline. The hammerhead patrols the invisible boundary where warm surface water meets cold deep water. XOuïe is the only XOceanus engine at dead-center polarity — 50% feliX, 50% Oscar. Not because it is neutral. Because it inhabits both simultaneously.

### Accent Color: Hammerhead Steel `#708090`

Slate gray with a cool-blue undertone. The color of cartilage, of industrial mesh, of the underside of a shark seen from below. The first and only neutral-cool accent in the XOceanus gallery — every other engine uses a vibrant or saturated color. OUIE's restraint is intentional. The hammerhead is not flamboyant. It is precise, purposeful, and dangerous exactly because it does not announce itself.

### Historical Homage

**Don Buchla — 259 Complex Waveform Generator (1976)**
The Buchla 259 was two oscillators — a principal and a modulating oscillator — with the relationship between them set at the patch level. Each oscillator had independent character. Their interaction (FM, waveshaping, amplitude modulation) was the musical event. The 259 did not provide a simple "detune" relationship. It provided a *defined dialogue* between two synthesis voices. OUIE's Interaction Stage is a direct conversation with this design philosophy: two synthesis voices, independently configured, with a parameterized relationship bus between them. The STRIFE↔LOVE axis is what Buchla would have built if he had been thinking in terms of polarity rather than modulation routing.

**Morton Subotnick — Silver Apples of the Moon (1967) / The Wild Bull (1968)**
The first electronic compositions created specifically for album release used the Buchla 100 series modular for two-voice counterpoint not because two voices was a technical limit, but because two voices in dialogue is a compositional form with ancient roots. The relationship between the voices was the composition. When Subotnick's two voices moved toward consonance, it felt like resolution. When they diverged into rhythmic counterpoint, it felt like tension. XOuïe's STRIFE↔LOVE axis is the parameterization of what Subotnick achieved compositionally: the relationship between voices as a real-time expressive control.

**Roland Juno-106 DCO Layering (1984)**
The Juno-106's chorus and DCO architecture produced a particular warmth when two voices were layered at the same pitch — not quite unison, not quite detune, but two distinct DCOs producing slightly different harmonic profiles. "Take On Me" (A-Ha), the pads in "Just Can't Get Enough" (Depeche Mode). The LOVE end of XOuïe's axis — when both voices are drawn toward the same harmonic intervals and their spectra begin to blend — is built on the same principle: two synthesis paths cooperating to produce something neither achieves alone.

### What This Engine Is Not

OUIE is not a standard polyphonic synthesizer with two voices. It is not a detuned mono synth. It is not a chorus or ensemble effect. It is a philosophical instrument: two synthesized entities in a defined relationship, where the relationship itself is the primary performance parameter. Every design decision should reinforce this identity.

A preset that treats the two voices as identical is a failed preset. The engine's power comes from difference — two algorithms, two characters, one connection.

---

## 2. Concept

XOuïe is built on a single thesis: **two brothers, two algorithms, one axis decides if they harmonize or destroy each other.**

This is not polyphony. Polyphony spreads notes across identical voices. XOuïe gives two fully independent voices — each with its own synthesis algorithm, its own character, its own timbral identity — and places them in a defined philosophical relationship.

The STRIFE↔LOVE axis is the engine's primary expressive control. At STRIFE, the two voices compete: frequency-space repulsion pushes them away from each other's pitch territory, cross-modulation generates interference, the two algorithms fight. At LOVE, the two voices cooperate: harmonic attraction draws Voice B toward interval relationships with Voice A, spectral content blends, the two algorithms fuse. Between these extremes: every shade of sibling relationship from grudging coexistence to fierce argument to tender harmony.

**The STRIFE↔LOVE axis IS the feliX-Oscar polarity axis for this engine.** STRIFE is feliX — analytical, separated, each entity in its own precise space. LOVE is Oscar — warm, blended, resonant, human. Moving the axis in real time traverses the entire XO_OX polarity spectrum.

**What XOuïe sounds like across the axis:**
- Full STRIFE: Industrial cross-FM leads. Ring-modulated aggression. Metallic dissonance. Frequency space warfare between two algorithms. Every interval feels contested.
- Center: Clean two-voice counterpoint. Two distinct timbres in melodic dialogue, aware of each other without merging. The MiniFreak playing two patches side by side.
- Full LOVE: Massive layered pads with rich internal harmonic structure. Two algorithms blended into a unified sound neither could produce alone. Lush unison walls with algorithmic depth underneath.

---

## 3. Architecture

### 3.1 Signal Path

```
VOICE A (Brother 1)                  VOICE B (Brother 2)
┌──────────────────────┐             ┌──────────────────────┐
│  Algorithm Select    │             │  Algorithm Select    │
│  ┌────────────────┐  │             │  ┌────────────────┐  │
│  │ SMOOTH:        │  │             │  │ SMOOTH:        │  │
│  │  VA            │  │             │  │  VA            │  │
│  │  Wavetable     │  │             │  │  Wavetable     │  │
│  │  FM            │  │             │  │  FM            │  │
│  │  Additive      │  │             │  │  Additive      │  │
│  │ ROUGH:         │  │             │  │ ROUGH:         │  │
│  │  Phase Dist    │  │             │  │  Phase Dist    │  │
│  │  Wavefolder    │  │             │  │  Wavefolder    │  │
│  │  Karplus-St    │  │             │  │  Karplus-St    │  │
│  │  Noise         │  │             │  │  Noise         │  │
│  └────────────────┘  │             │  └────────────────┘  │
│  Tune / Fine         │             │  Tune / Fine         │
│  Level / Pan         │             │  Level / Pan         │
│  Unison 1-4          │             │  Unison 1-4          │
└──────────┬───────────┘             └──────────┬───────────┘
           │                                    │
           │    ┌───────────────────────────┐   │
           └────►   INTERACTION STAGE       ◄───┘
                │                           │
                │  Pitch Tracking           │
                │  Frequency Repulsion (S)  │
                │  Harmonic Attraction (L)  │
                │  Cross-FM (STRIFE)        │
                │  Ring Modulation (STRIFE) │
                │  Spectral Blend (LOVE)    │
                │  Interval Lock (LOVE)     │
                │  Voice B Lag control      │
                └────────────┬──────────────┘
                             │
                  ┌──────────▼──────────────┐
                  │      SHARED FILTER      │
                  │  LP / HP / BP / Notch   │
                  │  ADSR filter envelope   │
                  │  Key tracking           │
                  └──────────┬──────────────┘
                             │
                  ┌──────────▼──────────────┐
                  │      AMP ENVELOPE       │
                  │      ADSR               │
                  │      Velocity scaling   │
                  └──────────┬──────────────┘
                             │
                  ┌──────────▼──────────────┐
                  │        FX CHAIN         │
                  │  Character Drive        │
                  │  Stereo Widener         │
                  │  Delay                  │
                  │  Reverb                 │
                  └──────────┬──────────────┘
                             │
                          OUTPUT
                     (duophonic: 2 voices max)
```

### 3.2 Algorithm Pool (8 algorithms, 2 clusters)

Each voice independently selects from 8 synthesis algorithms organized into behavioral clusters that interact predictably with the STRIFE↔LOVE axis.

**Smooth Brothers** — harmonically predictable, blend-friendly, LOVE-affinity:

| # | Algorithm | Character | DSP Source |
|---|-----------|-----------|------------|
| 0 | **Virtual Analog (VA)** | Classic saw/square/pulse/triangle/sine shapes | OddfeliX BLIT oscillator |
| 1 | **Wavetable (WT)** | Morphable wavetable with smooth interpolation | XOdyssey WT scanner |
| 2 | **FM (2-op)** | 2-operator FM, ratio/depth, bell to metallic | XOrbital 2-op subset |
| 3 | **Additive** | 16-partial additive, per-bank spectral shaping | New — ~200 lines |

**Rough Brothers** — harmonically unpredictable, conflict-prone, STRIFE-affinity:

| # | Algorithm | Character | DSP Source |
|---|-----------|-----------|------------|
| 4 | **Phase Distortion (PD)** | Casio CZ-style waveform bending, glassy to aggressive | XObsidian PD core |
| 5 | **Wavefolder (WF)** | West Coast folding, gentle folds to serrated chaos | XOrigami WF stage |
| 6 | **Karplus-Strong (KS)** | Plucked string physical modeling, organic transients | OddfeliX KS model |
| 7 | **Noise** | Filtered noise with spectral color control | Core DSP noise |

**Default pairing:** Voice A = VA (0), Voice B = Wavefolder (5). One smooth brother, one rough brother. HAMMER at center means they coexist without interaction — the engine's factory identity.

### 3.3 Interaction Stage (Signature Feature)

The Interaction Stage is computed post-oscillator, pre-filter. It reads both voices' audio outputs and pitch states and applies a continuous transformation controlled by two primary macros: **STRIFE** (ouie_macroStrife) and **LOVE** (ouie_macroLove).

These macros are not opposing ends of a single slider. They are two independent parameters. Both can be zero (clean duophony). Both can be moderate (ambiguous relationship). High STRIFE with low LOVE = conflict. High LOVE with low STRIFE = cooperation. High both = complex, unstable cooperation with tension — the most interesting territory.

**STRIFE operations (active when ouie_macroStrife > 0):**

- **Frequency Repulsion:** Voice B's pitch is pushed away from Voice A's pitch when they get closer than `ouie_frequency_repulsion` semitones. The repulsion force is exponential — very strong at < 1 semitone, weaker at > 3 semitones. At maximum repulsion, two voices that try to play the same note will push apart by up to 6 semitones in opposite directions.
- **Cross-FM:** Voice A's output modulates Voice B's oscillator phase (and vice versa). Modulation index controlled by `ouie_crossModRatio`. Creates sum/difference frequency sidebands, metallic sheen, instability.
- **Ring Modulation:** Voice A × Voice B time-domain multiplication produces sum and difference tones. Neither voice's fundamental — their mathematical relationship. The STRIFE/ring-mod level scales with ouie_macroStrife.

**LOVE operations (active when ouie_macroLove > 0):**

- **Harmonic Attraction:** Voice B's pitch is pulled toward the nearest `ouie_preferred_interval` interval relative to Voice A's pitch. The attraction force follows a soft exponential toward the interval. At max LOVE, Voice B will always seek and lock the preferred interval whenever both voices are playing.
- **Spectral Blend:** The amplitude spectra of both voices are weighted toward each other, producing a merged timbre that is neither voice alone.
- **Unison Thicken:** When harmonically locked, both voices stack at the same pitch with slight detune, producing the thick unison warmth of the LOVE state.

**Voice B Lag (`ouie_voice_b_lag`):** Voice B's response to note events is delayed by 0–500ms. At 0ms, both voices respond simultaneously. At 500ms, Voice A arrives a full half-second before Voice B follows. Combined with Harmonic Attraction, this creates the effect of Voice B *answering* Voice A — call and response synthesis. Combined with Frequency Repulsion, it creates the sensation of Voice B fleeing from Voice A's territory.

**Rhythm Sync (`ouie_rhythm_sync`):** Controls how the voices' amplitude envelopes phase-lock to each other:
- **Locked:** Both voices trigger simultaneously on every note
- **Free:** Voice B triggers at its own rate determined by its own envelope
- **Polyrhythm 3:4:** Voice A's envelope repeats every 3 beats, Voice B's every 4 — creating cross-rhythmic interaction
- **Polyrhythm 5:4:** Voice A every 5, Voice B every 4

### 3.4 Voice Allocation

XOuïe supports exactly 2 simultaneous MIDI notes. Three allocation modes:

| Mode | Behavior | Best for |
|------|----------|----------|
| **Duo** | First note → Voice A, second note → Voice B. Third note steals oldest. | Two-voice melodic counterpoint |
| **Layer** | Both voices on every note (monophonic, both algorithms active) | Thick mono with internal STRIFE/LOVE |
| **Split** | Voice A = notes ≤ `ouie_split_point`, Voice B = notes above | Bass/lead algorithm split |

---

## 4. Parameter List (~90 parameters, `ouie_` prefix)

### 4.1 Per-Voice Parameters (14 × 2 = 28 total)

Voice A parameters use `ouie_voiceA_*` prefix; Voice B parameters use `ouie_voiceB_*`.

| Parameter ID | Range | Default A / B | Description |
|-------------|-------|--------------|-------------|
| `ouie_voiceA_algorithm` | 0–7 | 0 (VA) / 5 (WF) | Synthesis algorithm selection |
| `ouie_voiceA_tune` | -24–+24 semitones | 0 / 0 | Coarse tune |
| `ouie_voiceA_fine` | -100–+100 cents | 0 / 0 | Fine tune |
| `ouie_voiceA_octave` | -2–+2 | 0 / 0 | Octave offset |
| `ouie_voiceA_level` | 0.0–1.0 | 1.0 / 1.0 | Voice output level |
| `ouie_voiceA_pan` | -1.0–+1.0 | -0.3 / +0.3 | Stereo position |
| `ouie_voiceA_unison` | 1–4 | 1 / 1 | Unison voice count per note |
| `ouie_voiceA_unisonDetune` | 0–50 cents | 10 / 10 | Unison detune spread |
| `ouie_voiceA_waveform` | 0–7 | 0 / 0 | Algorithm-specific waveform/table/mode |
| `ouie_voiceA_color` | 0.0–1.0 | 0.5 / 0.5 | Algorithm-specific primary timbre control |
| `ouie_voiceA_shape` | 0.0–1.0 | 0.0 / 0.3 | Algorithm-specific secondary timbre control |
| `ouie_voiceA_sub` | 0.0–1.0 | 0.0 / 0.0 | Sub oscillator blend (−1 octave, sine) |
| `ouie_voiceA_drift` | 0.0–1.0 | 0.05 / 0.05 | Analog-style pitch micro-drift |
| `ouie_voiceA_glide` | 0.0–5.0s | 0.0 / 0.0 | Portamento time (ECHO macro target for Voice B) |

### 4.2 Interaction Stage Parameters

| Parameter ID | Range | Default | Description |
|-------------|-------|---------|-------------|
| `ouie_frequency_repulsion` | 0.0–12.0 semitones | 2.0 | Distance threshold below which voices push apart (STRIFE behavior) |
| `ouie_harmonic_attraction` | 0.0–1.0 | 0.4 | Strength of pull toward preferred_interval (LOVE behavior) |
| `ouie_preferred_interval` | 0–7 | 4 (P5) | LOVE target interval: unison / m2 / M3 / tritone / P5 / M6 / octave / random |
| `ouie_rhythm_sync` | 0–3 | 0 (locked) | Envelope phase: locked / free / polyrhythm 3:4 / polyrhythm 5:4 |
| `ouie_voice_b_lag` | 0–500ms | 0 | Voice B note-response delay behind Voice A |
| `ouie_cross_mod_ratio` | 0.5–8.0 | 1.0 | FM carrier:modulator ratio for STRIFE cross-FM |
| `ouie_interaction_depth` | 0.0–1.0 | 0.5 | Global scale for all Interaction Stage operations |
| `ouie_blend_mode` | 0–2 | 0 | LOVE blend method: frequency-split / spectral / unison-thicken |

### 4.3 Filter Parameters

| Parameter ID | Range | Default | Description |
|-------------|-------|---------|-------------|
| `ouie_filter_cutoff` | 20–20000 Hz | 8000 | Filter cutoff frequency |
| `ouie_filter_resonance` | 0.0–1.0 | 0.0 | Resonance |
| `ouie_filter_type` | 0–4 | 1 (LP24) | LP12 / LP24 / HP / BP / Notch |
| `ouie_filter_env_amount` | -1.0–+1.0 | 0.3 | Filter envelope depth (bipolar) |
| `ouie_filter_key_track` | 0.0–1.0 | 0.5 | Key-tracking amount |
| `ouie_filter_attack` | 0.001–10s | 0.01 | Filter envelope attack |
| `ouie_filter_decay` | 0.001–10s | 0.5 | Filter envelope decay |
| `ouie_filter_sustain` | 0.0–1.0 | 0.4 | Filter envelope sustain |
| `ouie_filter_release` | 0.001–30s | 0.5 | Filter envelope release |

### 4.4 Amplitude Envelope Parameters

| Parameter ID | Range | Default | Description |
|-------------|-------|---------|-------------|
| `ouie_amp_attack` | 0.001–10s | 0.005 | Amplitude attack |
| `ouie_amp_decay` | 0.001–10s | 0.3 | Amplitude decay |
| `ouie_amp_sustain` | 0.0–1.0 | 0.8 | Amplitude sustain |
| `ouie_amp_release` | 0.001–30s | 0.5 | Amplitude release |

### 4.5 Modulation Envelope Parameters

| Parameter ID | Range | Default | Description |
|-------------|-------|---------|-------------|
| `ouie_mod_attack` | 0.001–10s | 0.01 | Modulation envelope attack |
| `ouie_mod_decay` | 0.001–10s | 0.3 | Modulation envelope decay |
| `ouie_mod_sustain` | 0.0–1.0 | 0.5 | Modulation envelope sustain |
| `ouie_mod_release` | 0.001–30s | 0.3 | Modulation envelope release |

### 4.6 LFO Parameters (2 × 4 = 8 total)

| Parameter ID | Range | Default | Description |
|-------------|-------|---------|-------------|
| `ouie_lfo1_rate` | 0.01–50 Hz | 1.0 | LFO 1 rate |
| `ouie_lfo1_depth` | 0.0–1.0 | 0.0 | LFO 1 depth |
| `ouie_lfo1_shape` | 0–5 | 0 (Sine) | Sine / Tri / Saw / Square / S&H / Random |
| `ouie_lfo1_sync` | 0/1 | 0 | Free / Tempo sync |
| `ouie_lfo2_rate` | 0.01–50 Hz | 3.0 | LFO 2 rate |
| `ouie_lfo2_depth` | 0.0–1.0 | 0.0 | LFO 2 depth |
| `ouie_lfo2_shape` | 0–5 | 2 (Saw) | Sine / Tri / Saw / Square / S&H / Random |
| `ouie_lfo2_sync` | 0/1 | 0 | Free / Tempo sync |

### 4.7 Voice Allocation Parameters

| Parameter ID | Range | Default | Description |
|-------------|-------|---------|-------------|
| `ouie_voice_mode` | 0–2 | 0 (Duo) | Duo / Layer / Split |
| `ouie_split_point` | C0–C8 | C4 | Note split point (Split mode only) |
| `ouie_velocity_split` | 0.0–1.0 | 0.0 | Velocity threshold above which note routes to Voice B |

### 4.8 Expression Parameters

| Parameter ID | Range | Default | Description |
|-------------|-------|---------|-------------|
| `ouie_velocity_to_filter` | 0.0–1.0 | 0.6 | Velocity scales filter brightness (D001) |
| `ouie_velocity_to_amp` | 0.0–1.0 | 0.8 | Velocity scales amplitude |
| `ouie_velocity_to_strife` | -1.0–+1.0 | 0.0 | Velocity shifts STRIFE level (+ = harder = more STRIFE) |
| `ouie_aftertouch_target` | 0–3 | 0 | Aftertouch → strife_love / filter / drive / LFO depth |
| `ouie_mod_wheel_target` | 0–3 | 0 | Mod wheel → strife_love / filter / LFO rate / voice_b_lag |
| `ouie_pitch_bend_range` | 1–24 semitones | 2 | Pitch bend range |

### 4.9 FX Parameters

| Parameter ID | Range | Default | Description |
|-------------|-------|---------|-------------|
| `ouie_drive_amount` | 0.0–1.0 | 0.0 | Saturation drive |
| `ouie_drive_type` | 0–2 | 0 (Soft) | Soft / Hard / Tube |
| `ouie_stereo_width` | 0.0–1.0 | 0.3 | M/S stereo widening |
| `ouie_delay_time` | 0.01–2.0s | 0.375 | Delay time |
| `ouie_delay_feedback` | 0.0–0.95 | 0.3 | Delay feedback (hard-clamped at 0.95) |
| `ouie_delay_mix` | 0.0–1.0 | 0.15 | Delay wet/dry |
| `ouie_reverb_size` | 0.0–1.0 | 0.3 | Reverb room size |
| `ouie_reverb_mix` | 0.0–1.0 | 0.15 | Reverb wet/dry |

### 4.10 Macro Parameters (M1–M4)

| Macro | ID | Label | Controls | Behavior |
|-------|-----|-------|----------|----------|
| M1 | `ouie_macroStrife` | STRIFE | `ouie_frequency_repulsion` (up), `ouie_cross_mod_ratio` (up), ring-mod level (up), `ouie_interaction_depth` (up at extremes) | 0 = no competition. 1 = maximum conflict — voices push apart, cross-FM at full, ring modulation active, frequency space warfare. |
| M2 | `ouie_macroLove` | LOVE | `ouie_harmonic_attraction` (up), `ouie_blend_mode` (toward unison-thicken), filter warmth (up), reverb mix (slight up) | 0 = no cooperation. 1 = maximum harmony — Voice B seeks preferred interval, spectra blend, unison thickening, everything warm. |
| M3 | `ouie_macroResonance` | RESONANCE | `ouie_preferred_interval` morph, `ouie_filter_resonance` (up), both voices' `color` toward resonant zone, cross-mod to shared harmonic | The shared resonant frequency between the two voices. High RESONANCE means both voices are converging on the same overtone series — even in STRIFE, there is one shared harmonic. |
| M4 | `ouie_macroEcho` | ECHO | `ouie_voice_b_lag` (0→500ms), Voice B glide (up), Voice B LFO depth (slight up) | Voice B lag behind Voice A. At 0: simultaneous. At 1: Voice B responds 500ms after Voice A — pure call and response. The ECHO macro makes the two voices a question and an answer. |

---

## 5. DSP Approach

### 5.1 Frequency-Space Repulsion (Core STRIFE DSP)

The frequency repulsion algorithm is OUIE's most novel DSP contribution. It operates in real time on the pitch state of both voices.

**Repulsion force function:**
```
distance = abs(voiceA_pitch_semitones - voiceB_pitch_semitones)
if distance < ouie_frequency_repulsion:
    repulsion_force = exp(-distance / ouie_frequency_repulsion) × ouie_macroStrife
    voiceA_pitch_offset -= repulsion_force × 0.5 × pitchSign  // push A up or down
    voiceB_pitch_offset += repulsion_force × 0.5 × pitchSign  // push B in opposite direction
```

The `pitchSign` is positive when Voice A is below Voice B (both voices pushed farther apart), negative when Voice A is above Voice B. This ensures the voices always push away from each other, never toward each other, under repulsion.

The force function uses exponential decay — very strong at close intervals (< 1 semitone), nearly zero at > `ouie_frequency_repulsion` semitones. This means STRIFE does not affect notes at large intervals; it only activates when the voices crowd each other's frequency space.

**Implementation note:** The pitch offsets are applied to the oscillator frequency, not to a pitch parameter. They are transient modulation values, not persistent parameter changes. They reset when notes release. They must not accumulate across buffer blocks.

### 5.2 Harmonic Attraction (Core LOVE DSP)

The harmonic attraction algorithm pulls Voice B's pitch toward the nearest `ouie_preferred_interval` interval relative to Voice A's pitch.

**Attraction target computation:**
```
interval_semitones = INTERVAL_RATIOS[ouie_preferred_interval]  // e.g., P5 = 7.0
target_pitch = voiceA_pitch + interval_semitones
direction = target_pitch - voiceB_current_pitch
attraction_delta = direction × ouie_harmonic_attraction × smoothingCoef
voiceB_pitch_offset += attraction_delta
```

The `smoothingCoef` is derived from the current sample rate — the attraction moves Voice B toward the target at a rate independent of block size. At maximum `ouie_harmonic_attraction`, Voice B reaches the target within ~200ms of a new Voice A note.

**Interval quantization**: The attraction algorithm selects the *nearest* instance of the interval — so if preferred_interval is P5 and Voice A is at C4, Voice B will seek G4 or G3, whichever is closer to Voice B's current position.

### 5.3 Cross-FM and Ring Modulation

**Cross-FM:** Voice A's output buffer values are applied as phase modulation to Voice B's oscillator (and vice versa). Phase modulation depth: `depth = ouie_macroStrife × ouie_cross_mod_ratio × 0.5`. Both voices modulate each other simultaneously. This is bilateral FM — the signature Buchla 259 complex waveform approach.

Clamping required: cross-FM depth must never exceed `depth = 2.0` to prevent runaway frequency deviation. Add hard clamp in `processBlock()`.

**Ring modulation:** `ring_output = voiceA_sample × voiceB_sample`. The ring-mod signal is mixed into the output at `ring_mix = ouie_macroStrife × 0.6`. It is not a replacement for the voices — it is an additive third timbre produced by their relationship.

Denormal protection required in any feedback path created by the ring modulator.

### 5.4 Voice B Lag Implementation

`ouie_voice_b_lag` introduces a delay between when a MIDI note triggers Voice A vs. when it triggers Voice B.

Implementation: on note-on, Voice A triggers immediately. A `lagTimer` is started. When `lagTimer >= ouie_voice_b_lag`, Voice B triggers. The lag timer uses sample-accurate timing — measured in samples at the current sample rate, not in milliseconds as a floating-point delay.

During the lag period, Voice A is playing and Voice B is silent. The Interaction Stage during this period runs only Voice A's pitch through the repulsion/attraction algorithms and produces no interaction output until Voice B triggers.

### 5.5 Filter and Envelope DSP

Cytomic SVF (zero-delay feedback), key-tracked. `ouie_filter_env_amount` is bipolar — uses `!= 0` check per CLAUDE.md critical pattern (not `> 0`). Negative envelope amounts sweep the filter downward.

`cancelAndHoldAtTime` when re-triggering a note mid-envelope — prevents filter clicks on fast repeated notes.

Filter coefficient computation uses matched-Z transform (`exp(-2*PI*fc/sr)`) per CLAUDE.md mandate. Sample rate always read from audio context. Never hardcoded to 44100 or 48000.

### 5.6 Velocity-to-Timbre Mapping (Doctrine D001)

Three independent velocity mappings:
1. `ouie_velocity_to_filter` — velocity scales filter cutoff above the static setting (brightness)
2. `ouie_velocity_to_amp` — velocity scales amplitude
3. `ouie_velocity_to_strife` — velocity shifts the effective STRIFE level

The third mapping is the musically distinctive one. Soft playing can push toward LOVE (quiet = cooperative), while hard playing increases STRIFE (loud = competitive). Or the inverse. This means velocity physically changes the relationship between the voices on every note. The same chord played piano vs. forte has a different internal harmonic relationship.

Velocity-to-strife uses `!= 0` check before applying the offset — both positive and negative offsets are valid per CLAUDE.md.

### 5.7 Algorithm Implementations

Each algorithm follows the same interface: takes `frequency`, `color`, `shape`, `waveform`, and `numSamples`; returns a mono float buffer. Algorithm switching crossfades over ~10ms (480 samples at 48kHz) to prevent clicks.

All 5 existing-algorithm wrappers delegate to their source engine's DSP header via thin include. The 3 new algorithms (WT, Additive, Noise) are self-contained in `OuieWT.h`, `OuieAdditive.h`, `OuieNoise.h`.

Use `?? 0` not `|| 0` when indexing Float32Array buffers in algorithm output — `0.0` is a valid sample amplitude, not an error.

---

## 6. feliX/Oscar Polarity

XOuïe is the only XOceanus engine at dead-center polarity — 50% feliX, 50% Oscar.

This is not neutrality. It is simultaneous inhabitation of both poles.

**STRIFE = feliX:** Analytical separation. Cross-FM creates mathematically precise frequency relationships. Frequency repulsion is exponential force mathematics applied to pitch space. Ring modulation produces sum/difference tones with exact arithmetic relationships. STRIFE is feliX's rationality and precision applied to competition.

**LOVE = Oscar:** Warmth, blending, harmonic richness. Harmonic attraction produces chord-tone warmth. Spectral blending creates indistinct, fused sonority. Unison thickening adds density. LOVE is Oscar's warmth and humanity applied to cooperation.

**The STRIFE and LOVE macros ARE the polarity controls.** A preset with STRIFE=1, LOVE=0 is the most feliX preset in the gallery. A preset with STRIFE=0, LOVE=1 is among the most Oscar. A preset with STRIFE=0.5, LOVE=0.5 is the thermocline — both at half strength, pushing and pulling simultaneously.

The RESONANCE macro adds a third dimension: a shared harmonic frequency that both voices converge toward even at high STRIFE. It is the point of minimum tension — the one thing they agree on.

---

## 7. Coupling Potential

### 7.1 Signature Coupling Routes

| Route | Type | Musical Effect |
|-------|------|---------------|
| OUIE × OCEANDEEP | `AmpToFilter` | Apex Dive — duophonic STRIFE leads over abyssal 808. The hammerhead descending to Oscar's floor. |
| OUIE × OPENSKY | `AudioToWavetable` | Thermal Rising — two-voice texture erupting through the surface into euphoric shimmer. |
| OUIE × OPAL | `AudioToWavetable` | Two-voice texture granulated into particles. The predator dissolved into plankton. |
| OUIE × OVERDUB | `getSample` | Two brothers' argument echoing endlessly through dub delay. |
| OUROBOROS → OUIE | `EnvToMorph` | Chaos amplitude drives STRIFE macro — strange attractor makes the brothers fight. |
| ORACLE → OUIE | `EnvToMorph` | Stochastic intervals selected by ORACLE drive `ouie_preferred_interval` — the oracle chooses the interval. |
| OHM → OUIE | `AmpToFilter` | COMMUNE axis amplitude drives LOVE macro — the commune makes the shark harmonize. |
| OUIE × OUIE (self) | `AmpToFilter` | Cephalofoil — 4 interacting algorithms across two instances. Two hammerheads circling. |

### 7.2 Coupling Types OUIE Accepts

- `AmpToFilter` — external amplitude modulates filter cutoff
- `EnvToMorph` — external envelope drives STRIFE or LOVE macro (most powerful OUIE input)
- `LFOToPitch` — external LFO modulates per-voice detune
- `AudioToFM` — external audio FM-modulates Voice B via cross-mod path

### 7.3 Coupling Types OUIE Sends

- `getSample` — post-FX stereo output to target engine
- `AmpToFilter` — OUIE amplitude envelope as external modulation source
- `EnvToMorph` — STRIFE/LOVE axis position exported as continuous modulation signal

---

## 8. UI Concept

**The cephalofoil layout.**

Voice A controls on the left panel. Voice B controls on the right panel. The Interaction Stage occupies the center column, spanning the full height, connecting both panels. The STRIFE and LOVE macros are two separate controls on the center column — not a single bipolar slider — because both can be elevated simultaneously.

The central interaction visualizer shows the live relationship between Voice A and Voice B:
- At high STRIFE: waveforms shown on the same timeline, colliding, interference artifacts highlighted in cool blue
- At high LOVE: waveforms converging and blending, shown in amber warmth
- At center/independent: waveforms shown side by side, parallel, coexisting without overlap

Voice A has a subtle cool-white indicator light. Voice B has a warm amber indicator. These are the only warm and cool tones in the otherwise steel-gray UI. They represent the feliX/Oscar identity of the two voices.

**Color palette:** Hammerhead Steel `#708090` primary accent. Deep charcoal background `#2A2E35`. Voice A cool blue indicator `#B0C4DE`. Voice B amber indicator `#DAA520`. The ECHO macro (Voice B lag) shows a time delay visualization between the two voice indicators — a visible gap that grows as `ouie_voice_b_lag` increases.

---

## 9. Preset Strategy

**150 factory presets** across 7 categories (fleet standard):

| Category | Count | Character |
|----------|-------|-----------|
| Brotherly Love | 22 | LOVE macro elevated — harmonious duophony, interval lock, blended texture |
| Brotherly Strife | 22 | STRIFE macro elevated — cross-FM, frequency repulsion, dissonant aggression |
| Thermocline | 20 | Both macros balanced — clean duophony with subtle interaction tension |
| Predator | 18 | Aggressive leads and bass — high drive, filter sweeps, STRIFE-forward |
| Deep Scan | 15 | RESONANCE-focused ambient — sensitive, velocity-shaped, electroreceptive |
| Coupling Showcases | 15 | OUIE + OCEANDEEP, OPENSKY, OPAL, OVERDUB, OHM |
| Hero Presets | 12 | Best-of-category showpieces |
| Rhythm Experiments | 8 | Polyrhythm 3:4 and 5:4 presets — OUIE as rhythmic engine |
| ECHO Series | 8 | `ouie_voice_b_lag` at various settings — call-and-response synthesis |
| Algorithm Pairs | 10 | Systematic exploration of cross-cluster algorithm combinations |

**Hero Preset Concepts:**

1. **Cephalofoil** — VA + WF, STRIFE=0, LOVE=0, HAMMER at 0.5, voices panned ±0.4. Clean duophony. The engine's identity.
2. **Apex** — FM + PD, STRIFE=1.0, LOVE=0. Cross-FM at max ratio. The attack.
3. **Schooling** — WT + WT, STRIFE=0, LOVE=1.0. Spectral blend + unison thicken. Brothers as one massive wall.
4. **Thermocline Drift** — VA + Add, STRIFE=0.3, LOVE=0.3. LFO sweeping STRIFE. The boundary.
5. **Question / Answer** — VA + KS, ECHO=1.0 (500ms lag), LOVE=0.6 (interval lock). Voice B answering Voice A.
6. **Gill Breath** — Noise + WF, STRIFE=0.2, LFO on filter at 0.01 Hz. The engine breathing.
7. **Brothers** — VA + PD, STRIFE=0.4, LOVE=0.2. Neither full conflict nor full harmony. Tension.
8. **Apex Dive** — Coupling preset: OUIE STRIFE + OCEANDEEP. The hammerhead descending to the floor.
9. **Thermal Rising** — Coupling preset: OUIE LOVE + OPENSKY. Brothers rising through the surface.
10. **Polyrhythm** — VA + FM, rhythm_sync = 3:4, RESONANCE elevated. Cross-rhythmic counterpoint.
11. **The Repulsion** — VA + VA, STRIFE=1.0, frequency_repulsion=6 semitones. Identical algorithms pushed apart.
12. **The Agreement** — Add + WT, LOVE=1.0, preferred_interval = P5. Perfect harmonic cooperation.

---

## 10. Doctrine Compliance Plan

| Doctrine | Requirement | OUIE Implementation |
|----------|------------|---------------------|
| D001 | Velocity → timbre | `ouie_velocity_to_filter` (brightness), `ouie_velocity_to_strife` (relationship axis) — velocity shapes both filter brightness and the inter-voice relationship |
| D002 | Modulation lifeblood | 2 LFOs (rate floor 0.01 Hz), mod wheel, aftertouch, 4 macros (STRIFE/LOVE/RESONANCE/ECHO), mod envelope, velocity → 3 targets |
| D003 | Physics rigor | Frequency repulsion: exponential force function (documented); cross-FM: Buchla 259 bilateral FM (cited); ring mod: product modulation (standard); KS: Karplus-Strong physical model (cited) |
| D004 | No dead parameters | All ~90 params wired — per-voice `color` and `shape` remap to active algorithm's specific DSP controls. Document per-algorithm mappings in `OuieEngine.h` header comments |
| D005 | Breathing ≤ 0.01 Hz | `ouie_lfo1_rate` and `ouie_lfo2_rate` floor at 0.01 Hz. ECHO macro at non-zero creates autonomous Voice B behavior. |
| D006 | Expression input | Velocity → filter (D001) + velocity → STRIFE level, aftertouch → STRIFE/LOVE macro, mod wheel → voice_b_lag / interval |

---

## 11. Phase 1 Build Readiness

### What's Needed to Scaffold XOuïe

**1. JUCE project creation:**
New AudioUnit instrument project. PLUGIN_CODE=`Xoui`, PLUGIN_MANUFACTURER_CODE=`Xoox`. All parameters under `ouie_` prefix. Source location: `Source/Engines/Ouie/OuieEngine.h`.

**2. Algorithm infrastructure:**
Abstract `OuieAlgorithm` base class: `virtual float* process(float frequency, float color, float shape, int waveform, int numSamples)`. Concrete implementations:
- `OuieVA.h` — wraps OddfeliX BLIT core oscillator (~50 lines)
- `OuieFM.h` — wraps XOrbital 2-op FM subset (~80 lines)
- `OuieKS.h` — wraps OddfeliX KS pluck model (~60 lines)
- `OuiePD.h` — wraps XObsidian phase distortion core (~60 lines)
- `OuieWF.h` — wraps XOrigami wavefolder stage (~60 lines)
- `OuieWT.h` — new wavetable scanner with cubic interpolation (~150 lines)
- `OuieAdditive.h` — new 16-partial additive synthesis (~200 lines)
- `OuieNoise.h` — new colored noise via cascaded first-order filters (~80 lines)

**3. Interaction Stage:**
`OuieInteractionStage.h` — single class, ~300 lines. Methods: `applyRepulsion(pitchA, pitchB)`, `applyAttraction(pitchA, pitchB)`, `applyCrossFM(bufA, bufB)`, `applyRingMod(bufA, bufB)`, `applySpectralBlend(bufA, bufB)`. All operations gated on STRIFE/LOVE macro levels.

**4. Voice B Lag:**
`OuieVoiceBLag.h` — sample-accurate trigger delay. `scheduleTrigger(sampleDelay)` called on note-on. Counts down in `processBlock()`. Fires Voice B trigger when counter reaches zero. ~60 lines.

**5. Voice allocation:**
`OuieDuoVoiceManager.h` — manages 2 voices, 3 allocation modes, per-voice portamento. ~150 lines. Simpler than standard polyphonic voice managers because maximum voices = 2.

**6. Parameter registration:**
~90 APVTS parameters in `createParameterLayout()`. All `ouie_` prefix. STRIFE, LOVE, RESONANCE, ECHO mapped to underlying parameter targets in `applyMacros()` called once per block.

**7. DSP safety requirements:**
- Cross-FM depth hard-clamped at 2.0 maximum in `OuieFM.h` bilateral modulator
- Delay feedback hard-clamped at 0.95 in FX chain
- Denormal protection in ring modulator path and any feedback accumulator
- `cancelAndHoldAtTime` in filter envelope on note retrigger
- Algorithm switch crossfade: 480-sample fade between old and new algorithm buffers

**8. Preset scaffolding:**
12 hero presets first, in `.xometa` format, establishing STRIFE/LOVE/ECHO vocabulary. Expand to 150 in preset phase.

**9. Coupling adapter stubs:**
Accepted: `AmpToFilter`, `EnvToMorph`, `LFOToPitch`, `AudioToFM`. Sent: `getSample`, `AmpToFilter`, `EnvToMorph`. Define in `OuieEngine.h` before XOceanus integration.

### Build Scope Estimate

~2,000–2,500 lines of DSP across 8 algorithm files + interaction stage + voice manager. The engine is lighter than any build since OPTIC because:
- Maximum 2 voices (no polyphonic voice management complexity)
- 5 of 8 algorithms delegate to existing engine DSP
- Interaction Stage is arithmetic, not FFT or convolution
- No shimmer reverb, no spectral vocoder, no convolution

### Phase Gates

**Phase 1 → Phase 2:**
- Both voices produce independent audio at STRIFE=0, LOVE=0
- STRIFE macro produces audible cross-FM/ring-mod artifacts
- LOVE macro produces audible harmonic attraction behavior
- All 8 algorithm types produce audio in both Voice A and Voice B

**Phase 2 → Phase 3 (Preset):**
- D001–D006 all satisfied (verified via `/engine-health-check`)
- auval PASS
- 150 factory presets complete, named, DNA-tagged

---

*XO_OX Designs | Engine: OUIE | Accent: #708090 | Prefix: ouie_ | initial scope | Phase 1 R&D*
*"Two brothers. Two algorithms. One axis decides if they harmonize or destroy each other."*
