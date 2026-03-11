# XO_OX Mega-Tool — Engine Module Catalog

**Version:** 1.0
**Author:** XO_OX Designs
**Date:** 2026-03-08
**Status:** Specification Complete
**Depends on:** `xo_mega_tool_feasibility.md`, `xo_mega_tool_dev_strategy.md`

---

## 1. Introduction

This document catalogs every synth engine module available for the XO_OX Mega-Tool platform. Each module originated as a standalone XO_OX instrument and contributes a unique sonic identity to the merged platform. The catalog informs:

- Which engines become first-class mega-tool modules
- How engines map to the `SynthEngine` interface
- What DSP can be shared across modules
- Which module pairings create emergent sounds through coupling
- Build priority and CPU budget allocation

**Reference documents:**
- Architecture: `xo_mega_tool_feasibility.md` (Hybrid A+D approach)
- Strategy: `xo_mega_tool_dev_strategy.md` (Interface-First Hybrid, 14-week roadmap)
- Playing surface: `xo_signature_playsurface_spec.md` (3-mode unified surface)

---

## 2. Module Summary

| Module | Source | Sonic Role | Osc Types | Filter | Voices | Params | CPU Est. | Mega-Tool Role |
|--------|--------|-----------|-----------|--------|--------|--------|----------|----------------|
| **FAT** | XObese | Width / thickness | 13-osc stacked (morph: sine→saw→sq→noise) | 4× ZDF Ladder (4-pole LP) | 5 (65 osc) | 45 | <12% | Makes anything massive |
| **BITE** | XOppossum | Bass / character | OscA (warm, 4 waves) + OscB (harsh, 5 waves) | Cytomic SVF (4 voiced modes) | up to 16 | 122 | <10% | Grit, weight, attitude |
| **SNAP** | OddfeliX/OddOscar EngX | Percussive / rhythmic | PolyBLEP, FM, Karplus-Strong | Cytomic SVF | 8 | ~26 | <8% | Attacks, plucks, rhythm |
| **MORPH** | OddfeliX/OddOscar EngO | Pads / lush | Wavetable morph (2048 frames) | Moog Ladder (4-pole LP) | 16 | ~26 | <12% | Atmospheres, evolving pads |
| **DUB** | XOverdub | FX architecture | Basic osc + filter + env | Per-voice SVF | 8 | 38 | <8% | Send/return FX routing |
| **DRIFT** | XOdyssey | Psychedelic pads | OscA/B × 4 modes (classic, WT, supersaw, FM) | Dual: SVF LP + 3-band formant | 24 | ~130 | <15% | Journey engine, Climax |
| **ONSET** | XOnset (spec) | Drums / percussion | Circuit (808/909) + Algorithm (FM, Modal, K-S, PD) | Per-voice Cytomic SVF | 8 dedicated | ~110 | <15.5% | Drum synthesis + morphing |

**Note on XOzone:** The XO_OX-XOzone-Instrument project is the rebranded Instability Synth (originally at `~/Desktop/synth-plugin`). Its DSP concepts (Cytomic SVF, PolyBLEP, ParamSnapshot) were absorbed into XObese and XOppossum during their development. It does not represent a separate module — its legacy lives on through the FAT and BITE engines.

---

## 3. Detailed Module Profiles

---

### 3.1 FAT (XObese) — Width & Weight

**Sonic Identity:** Instant stereo width through 13-oscillator stacking with independent pan/detune per group. The Mojo Engine provides a unique analog↔digital blend axis — warm drifting chaos at one end, pristine wavetable precision at the other. Even single notes feel enormous.

**Signal Flow:**
```
Per Voice (×5 max):
┌─────────────────────────────────────────────────────┐
│  Osc Sub (Triangle, -12st)  ──────────────────────┐ │
│                                                    │ │
│  Triplet Group 1 (3 osc, center pan)  → LadderF1 ─┤ │
│  Triplet Group 2 (3 osc, L -0.3)     → LadderF2 ─┤ │
│  Triplet Group 3 (3 osc, R +0.3)     → LadderF3 ─┤ │
│  Triplet Group 4 (3 osc, wide stereo) → LadderF4 ─┤ │
│                                                    │ │
│  Mojo Engine (per-osc drift + soft-clip blend) ────┘ │
│                        ↓                             │
│                   Amp Envelope                       │
└────────────────────────┬────────────────────────────┘
                         ↓
Global FX Chain (serial):
Saturation → Bitcrusher → Chorus → Ping-Pong Delay → Dattorro Reverb → 3-Band EQ → Limiter
                         ↓
                   Master Output
```

**Oscillator Architecture:**
- 13 oscillators per voice: 1 sub (triangle, configurable octave -24/-12/0) + 4 triplet groups (root + root±12st)
- Waveform morphing 0→1: sine → saw → square → noise (continuous, crossfaded)
- Wavetable: 2048-sample frames, 32-bit float, mip-mapped anti-aliasing (10 levels)
- XORshift32 PRNG noise source for morph positions >0.66
- Per-group detune: -5ct, 0, +5ct, ±10ct across groups
- Per-group pan: -0.3, 0, +0.3, ±hard

**Mojo Engine (Unique):**
- `p_mojo` 0.0–1.0: analog warmth ↔ digital precision
- Analog (0.0): per-oscillator Perlin noise LFO at 0.1Hz, ±3 cents drift + tanh soft-clip per osc
- Digital (1.0): zero drift, zero soft-clip, pure wavetable
- Drift amount: `base_drift × (1 - mojo)`, soft-clip: `apply_gain × (1 - mojo)`

**Filter Architecture:**
- 4 independent ZDF Ladder filters (4-pole lowpass), one per triplet group
- Self-oscillation above 0.95 resonance
- Internal drive: pre-filter tanh saturation for volume compensation at high Q
- Sub oscillator bypasses all filters (preserves phase coherence)
- Modulation: bipolar filter envelope (-1 to +1), keyboard tracking (0–1, default 0.5), aftertouch (+2 octaves, 10ms slew)

**FX Chain:**
1. Asymmetric polynomial saturation (volume-compensated)
2. Bitcrusher (2–16 bit, 500–44100 Hz sample rate; bypassed at max)
3. Stereo Chorus (dual-voice, 90° L/R phase offset, 0.1–5Hz)
4. Ping-Pong Delay (mono in → stereo out, sync: 1/4, 1/8, 1/8T, 1/16, 1/32)
5. Dattorro Plate Reverb
6. 3-Band EQ (200Hz low shelf, 1kHz peak, 4kHz high shelf, ±12dB each)
7. Brickwall Limiter (always-on, -0.3dB ceiling, 0.1ms lookahead)

**Modulation:** Filter envelope (ADSR), aftertouch → cutoff, pitch bend ±2st. Arpeggiator (UP/DOWN/UP-DOWN/RANDOM/AS-PLAYED, 1–3 octaves, tempo sync).

**Unique Features:**
- 13 oscillators per voice — no other module has this density
- Mojo analog↔digital blend — unique continuous axis
- 4 parallel independent filters — each group has its own timbral character
- Even single notes fill the stereo field

**Parameters:** 45 | **CPU:** <12% | **Voices:** 5

**Reusable DSP:** ZDF Ladder filter, wavetable with mip-mapping, PolyBLEP (saw/square), Dattorro reverb, arpeggiator

**SynthEngine Mapping:**
- `prepare()`: allocate 5 voices × 13 oscillators, init all filters + FX
- `renderBlock()`: per-sample voice render → sum → FX chain → output
- `getSampleForCoupling()`: post-filter, pre-FX voice sum (raw "fat" signal)
- `applyCouplingInput()`: filter cutoff offset, morph position, mojo blend

---

### 3.2 BITE (XOppossum) — Bass & Character

**Sonic Identity:** Two-pole character engine where Belly (warm, weighted) and Bite (aggressive, sharp) define the instrument's personality. Four per-voice character stages (Fur, Chew, Gnash, Trash) color the sound at the DSP level — not post-FX. The Weight Engine guarantees bass integrity even under extreme drive.

**Signal Flow:**
```
Per Voice (mono/legato/duo/poly4/unison/poly8/poly16):
┌─────────────────────────────────────────────────────┐
│  OscA (Belly: warm)  ──┐                            │
│  OscB (Bite: harsh)  ──┤── Osc Interaction ──┐     │
│  Weight/Sub  ───────────┤                     │     │
│  Noise Source  ─────────┘                     │     │
│                                               ↓     │
│                                         Fur Stage   │
│                                    (pre-filter sat) │
│                                               ↓     │
│                                     FilterBlock     │
│                                   (Cytomic SVF ×4)  │
│                                       + Chew        │
│                                       + Drive       │
│                                               ↓     │
│                                        Gnash Stage  │
│                                   (asymmetric bite) │
│                                               ↓     │
│                                        Trash Stage  │
│                                    (4-mode dirt)    │
│                                               ↓     │
│                                       Weight Re-    │
│                                       inforcement   │
│                                               ↓     │
│                                      Amp Envelope   │
└───────────────────────────────────────────┬─────────┘
                                            ↓
Global FX:
Motion (Chorus/Doubler/Flange) → Echo (4 delay modes) → Space (3 reverb modes) → Finish (Glue/Clip/Width)
                                            ↓
                                      Master Output
```

**Oscillator Architecture:**
- **OscA (Belly):** 4 waveforms — Triangle (shape: pure→peaked), Rounded Saw (softened discontinuity), Hollow Pulse (width + fundamental reduction), Pedal Hybrid (tri+square blend, organ-like). Drift param: filtered white-noise ~2Hz pitch wander.
- **OscB (Bite):** 5 waveforms — Narrow Pulse, Saw Buzz (upper-harmonic emphasis), Hollow Reed (nasal, odd-harmonic), CMOS Rasp (clipped/starved), Folded Buzz (wavefolder depth). Instability param: aggressive pitch/phase randomness ~8Hz, up to 2% deviation.
- **Osc Interaction:** 4 modes (Soft Sync, Low FM, Phase Push, Grit Multiply)
- **Weight Engine:** Detects fundamental loss under saturation, reinforces. 5 shapes (Sine, Rounded Square, Sub Fifth ×2/3, Triangle, Oct Up). Octave: -2/-1/0.
- **Noise:** 5 types (air, dust, hiss, static, scrape), 4 routing modes (pre-filter, post-filter, attack-only, always-on)

**Character Stages (per-voice, NOT post-FX):**

| Stage | Position | DSP | Character |
|-------|----------|-----|-----------|
| Fur | Pre-filter | Soft tanh + 8kHz LP on saturated signal | Plush, velvety, low-mid glue |
| Chew | Post-filter | Upper-mid emphasis contour | Sharpened midrange bite |
| Gnash | Post-filter | Asymmetric: positive=hard clip (atan), negative=soft (tanh) | Odd-harmonic aggressive snarl |
| Trash | Post-filter | 4 modes: Rust (tube + DC bias), Splatter (full-wave rect), Fold (wavefolder), Crushed (bitcrusher) | Dirt / corruption |

**Filter:** Cytomic SVF (topology-preserving transform). 4 voiced modes:

| Mode | Type | Character |
|------|------|-----------|
| Burrow | LP (2-pole) | Warm lowpass with low-band compensation |
| Snarl | BP | Nasal resonance emphasis (extra BP mixed in) |
| Wire | HP | Thin, nervous highpass |
| Hollow | Notch | Eerie band-reject scooped character |

Pre-filter drive (tanh), resonance 0–1 (Q=2 to self-oscillation), keytracking (ref C4), NaN guarding on integrator states.

**Macro System:**

| Macro | Name | Curve | Controls | Musical Intent |
|-------|------|-------|----------|---------------|
| M1 | BELLY | ease_in_out | ↑OscA level, sub, weight, fur; ↓cutoff, resonance, OscB instability | Warmth / body |
| M2 | BITE | ease_out | ↑OscB level+shape, resonance, chew, gnash, noise transient, instability | Edge / aggression |
| M3 | SCURRY | ease_in | ↑LFO2 depth, osc interaction, FX Motion depth | Animation / movement |
| M4 | TRASH | ease_in_out | ↑trash amount, noise, gnash, output clip | Dirt / destruction |
| M5 | PLAY DEAD | slow_rise | ↑amp release; ↓cutoff, Motion depth; pitch sag, harmonic damping | Fade / decay |

**Modulation:** 3 envelopes (Amp ADSR, Filter ADSR bipolar with velocity scaling, Mod ADSR). 3 LFOs (7 shapes, 15 tempo divisions, per-voice retrigger + unique seeds). 8-slot mod matrix (16 sources × 19 destinations, bipolar amounts).

**Parameters:** 122 | **CPU:** <10% | **Voices:** mono to poly16

**Reusable DSP:** Cytomic SVF, PolyBLEP, ParamSnapshot, FastMath (fastSin, fastTanh, fastExp2, fastAtan), ADSR envelope, DC blocker

**SynthEngine Mapping:**
- `getSampleForCoupling()`: post-Gnash, pre-Trash voice sum (character-rich signal before destruction)
- `applyCouplingInput()`: filter cutoff, gnash amount, fur amount, osc blend, weight amount

---

### 3.3 SNAP (OddfeliX) — Percussive & Rhythmic

**Sonic Identity:** Punchy, snappy, rhythmic. Built for percussive attacks and plucked textures. The terracotta engine — warm but sharp, like struck clay.

**Signal Flow:**
```
Per Voice (×8):
MIDI Note → Oscillator (PolyBLEP / FM / Karplus-Strong)
         → Snap Attack Shaper
         → Cytomic SVF Filter
         → Amp Envelope (fast attack bias)
         → Voice Output
              ↓
         Coupling Bus → X→O filter ducking
```

**Oscillator Architecture:**
- PolyBLEP: band-limited saw, square, pulse with anti-aliased transitions
- FM: 2-operator with modulatable ratio and index
- Karplus-Strong: delay line + averaging filter for plucked/struck strings

**Filter:** Cytomic SVF (lowpass, bandpass, highpass modes)

**Unique Features:**
- Snap parameter: controls pitch envelope depth + transient sharpness
- Designed specifically for percussive/rhythmic playing — fast envelopes, punchy attacks
- Coupling output: amplitude envelope drives Engine O's filter cutoff (dub pump effect)

**Parameters:** ~26 (part of OddfeliX/OddOscar's 52 total) | **CPU:** <8% | **Voices:** 8

**Color Identity:** Terracotta #C8553D

**SynthEngine Mapping:**
- `getSampleForCoupling()`: post-filter voice sum (the "hit" signal)
- `applyCouplingInput()`: pitch offset (from Engine O drift), filter cutoff mod

---

### 3.4 MORPH (OddOscar) — Pads & Lush

**Sonic Identity:** Blooming, evolving, ethereal. Wavetable morphing through rich timbral landscapes. The teal engine — cool, deep, oceanic.

**Signal Flow:**
```
Per Voice (×16):
MIDI Note → Wavetable Oscillator (2048-frame morph)
         → Bloom Shaper (attack envelope modifier)
         → Moog Ladder Filter (4-pole LP)
         → Amp Envelope (slow attack bias)
         → Voice Output
              ↓
         Coupling Bus → O→X pitch drift
```

**Oscillator Architecture:**
- Wavetable: 2048-sample frames, 32-bit float
- Morph parameter: continuous sweep through wavetable positions
- Smooth interpolation between frames

**Filter:** Moog Ladder (4-pole lowpass), classic warm rolloff with resonance compensation

**Unique Features:**
- Bloom parameter: shapes the attack envelope for swelling pad behavior
- Wavetable morph provides evolving timbral character over sustained notes
- Coupling output: LFO modulates Engine X's pitch ±0.5 semitones (organic drift)

**Parameters:** ~26 (part of OddfeliX/OddOscar's 52 total) | **CPU:** <12% | **Voices:** 16

**Color Identity:** Teal #2A9D8F

**SynthEngine Mapping:**
- `getSampleForCoupling()`: post-filter voice sum (the "pad" signal)
- `applyCouplingInput()`: morph position, filter cutoff, bloom amount

---

### 3.5 DUB (XOverdub) — FX Architecture

**Sonic Identity:** The dub engineer's toolkit as a synth engine. Not primarily about the voice — it's about what happens to sound when you throw it through drive, tape delay, and spring reverb via a gated send bus. The performance pads (FIRE, XOSEND, ECHO CUT, PANIC) turn mixing into playing.

**Signal Flow:**
```
Per Voice (×8):
MIDI Note → Oscillator → Filter → Amp Envelope → Voice Sum
                                                      │
DRY PATH ←──────────────────────────────────────────── ┤
                                                      │
SEND PATH (gated by XOSEND pad, 5ms smooth): ─────── ┤
    → Drive (tanh saturation, amount 1.0–10.0)        │
    → Tape Delay (max 2s, self-osc >100% feedback)    │
    → Spring Reverb (6-allpass diffuser, metallic)    │
    → Return Bus                                       │
                                                      │
Dry/Return Mixer ←────────────────────────────────────┘
    → Master Volume
    → Soft Limiter (tanh, output safety)
    → Master Output

Performance Pads:
  [FIRE: retrigger]  [XOSEND: gate send]  [ECHO CUT: kill fb]  [PANIC: clear all]
```

**Unique Architecture — Send/Return Routing:**
- Send VCA: exponential smoothing with ~5ms time constant for musical "throw" gesture
- Tape Delay: circular buffer, self-oscillating feedback >100% with tanh saturation in loop (classic dub runaway)
- Spring Reverb: 6-allpass diffuser chain for metallic spring character
- ECHO CUT: zeroes feedback without clearing buffer — existing echoes decay naturally
- FX tail: 15 seconds persistence after patch change

**Unique Features:**
- Performance pads as core interaction (not just a synth with FX — the FX ARE the instrument)
- Send/return architecture allows any external engine to route through DUB's FX chain
- Self-oscillating tape delay creates drone/texture beds from any input
- The "throw" gesture (XOSEND) is a signature dub production technique made playable

**Parameters:** 38 | **CPU:** <8% | **Voices:** 8

**Colors:** Green #00FF88 (synth/FIRE), Amber #FFAA00 (send/echo), Cyan #00CCCC (reverb), Red #FF3333 (panic)

**SynthEngine Mapping:**
- `getSampleForCoupling()`: post-limiter master output (full wet+dry mix)
- `applyCouplingInput()`: send VCA level (external engines can "throw" themselves into DUB's FX), delay time, feedback amount
- **Special role:** DUB can function as a shared FX module — other engines route audio through its send/return chain without using DUB's own voice engine

---

### 3.6 DRIFT (XOdyssey) — Psychedelic Pads

**Sonic Identity:** The journey engine. Starts familiar, evolves toward alien. The Climax system is the signature feature — when the JOURNEY macro crosses a per-preset threshold, the instrument blooms across filter, shimmer, reverb, and width in a 1-3 second S-curve. Psychedelic pad architecture with dual filters (standard LP + formant vowel filter).

**Signal Flow:**
```
Per Voice (×24):
┌───────────────────────────────────────────────────────┐
│  OscA (4 modes: Classic/WT/Supersaw/FM) ─┐           │
│  OscB (4 modes: Classic/WT/Supersaw/FM) ─┤── Mix     │
│  Sub Oscillator ─────────────────────────┘   │       │
│                                              ↓       │
│                                    Haze Saturation   │
│                                      (pre-filter)    │
│                                              ↓       │
│                              ┌── FilterA (SVF LP 12/24)
│                              │                       │
│                              ├── FilterB (3-band     │
│                              │   parallel formant,   │
│                              │   5 vowel presets)    │
│                              ↓                       │
│                                   Prism Shimmer      │
│                                    (post-filter)     │
│                                              ↓       │
│                                      Fracture        │
│                                     (pre-amp)        │
│                                              ↓       │
│                                    Amp Envelope       │
└──────────────────────────────────────┬───────────────┘
                                       ↓
7 Signature Traits (modulated by JOURNEY macro + Climax system)
                                       ↓
FX: Chorus → Phaser → Delay → Reverb → Bass Integrity HPF
                                       ↓
                                 Master Output
```

**Oscillator Architecture:**
- OscA and OscB are identical, each with 4 selectable modes:
  - Classic: PolyBLEP saw/square/pulse
  - Wavetable: 2048-sample frames (placeholder sine in v0.7, needs file loading)
  - Supersaw: multi-oscillator detuned saw stack
  - FM: frequency modulation with ratio/index control

**Filter Architecture:**
- FilterA: Cytomic SVF lowpass, 12dB or 24dB slope
- FilterB: 3-band parallel SVF bandpass (formant filter), 5 vowel presets
- Filters can run in series or parallel

**7 Signature Traits:**

| Trait | Type | Musical Effect |
|-------|------|---------------|
| Voyager Drift | Filtered white-noise pitch mod | Organic pitch wandering |
| Prism Shimmer | Post-filter spectral effect | Crystalline sparkle |
| Haze Saturation | Pre-filter warm saturation | Analog warmth |
| Tidal Pulse | Rhythmic macro-driven pulse | Breathing movement |
| Bloom Attack | Shaped attack transient | Swelling note onset |
| Fracture | Pre-amp glitch/break | Digital disruption |
| Climax | Per-preset JOURNEY threshold → S-curve bloom | THE signature: emotional peak + resolve |

**Climax System (Unique):**
- Per-preset threshold on JOURNEY macro (0–1)
- When JOURNEY crosses threshold: 1–3 second S-curve bloom
- Simultaneously affects: filter cutoff opens, shimmer increases, reverb swells, stereo width expands
- Builds to emotional peak, then resolves — designed for live performance moments

**Macros:** JOURNEY (Familiar→Alien + Climax trigger), BREATHE, BLOOM, FRACTURE

**Modulation:** 8-slot mod matrix, 3 envelopes, 3 LFOs, unique `drift` mod source

**Parameters:** ~130 | **CPU:** <15% | **Voices:** 24 poly + legato

**SynthEngine Mapping:**
- `getSampleForCoupling()`: post-traits, pre-FX voice sum
- `applyCouplingInput()`: JOURNEY position, filter cutoff, shimmer amount, drift intensity

---

### 3.7 ONSET (XOnset) — Drums & Percussion

**Sonic Identity:** The drum machine that morphs between an 808 and a physics simulation on every hit. Each of 8 voices holds two synthesis paradigms — Circuit (analog warmth) and Algorithm (digital complexity) — with a continuous Blend axis between them. Cross-voice coupling makes the kit interact like neurons in a rhythm brain.

**Signal Flow:**
```
Per Voice (×8 dedicated drum voices):
┌─────────────────────────────────────────────────────────┐
│  MIDI Trigger → Transient Designer                      │
│                 (snap: pitch spike + noise burst)        │
│                        │                                │
│  ┌─────────────────────┼─────────────────────┐          │
│  │                     │                     │          │
│  │  Layer X (Circuit)  │  Layer O (Algorithm) │          │
│  │  ┌───────────────┐  │  ┌────────────────┐ │          │
│  │  │ Bridged-T     │  │  │ FM (2-op+fb)   │ │          │
│  │  │ Noise-Burst   │  │  │ Modal (8-mode) │ │          │
│  │  │ 6-Osc Metal   │  │  │ Karplus-Strong │ │          │
│  │  │ Self-Osc Filt │  │  │ Phase Distort  │ │          │
│  │  └───────┬───────┘  │  └───────┬────────┘ │          │
│  │          └──────────┼──────────┘          │          │
│  │              BLEND (equal-power)           │          │
│  └──────────────────┬────────────────────────┘          │
│                     ↓                                    │
│              Per-Voice SVF Filter                        │
│                     ↓                                    │
│              Amp Envelope (AD/AHD/ADSR)                  │
│                     ↓                                    │
│              Voice Output → Cross-Voice Coupling Matrix  │
└─────────────────────┬───────────────────────────────────┘
                      ↓
Voice Mixer (level, pan per voice)
     → Character Stage (Grit/Warmth)
     → Shared FX Rack [Delay] [Reverb] [LoFi]
     → Master Output
```

**Layer X — Circuit Modeling:**

| Topology | Voices | Heritage | Key Behavior |
|----------|--------|----------|-------------|
| Bridged-T Kick | V1, V6 | TR-808 | Ringing impulse, pitch sweep (snap × 4 oct), diode-starvation drift |
| Noise-Burst Snare | V2, V5 | 808/909 hybrid | Dual bridged-T oscs + HPF noise, multi-burst clap envelope |
| 6-Osc Metallic | V3, V4 | 808 hi-hat | 6 square waves at non-harmonic freqs → dual bandpass → HPF |
| Self-Osc Filter | V1 alt | Filter-as-oscillator | SVF at Q≈0.99 + impulse trigger, clean sub |

**Layer O — Algorithmic Synthesis:**

| Algorithm | Key Feature | `character` Controls |
|-----------|-------------|---------------------|
| FM (2-op + feedback) | Carrier + modulator with independent envelopes | Modulation index |
| Modal Resonator (8-mode bank) | 3 material models: membrane (Bessel), bar (stiffness), plate (dense) | Inharmonicity factor |
| Karplus-Strong | Delay line + averaging filter, plucked/struck | Blend factor (snare↔string) |
| Phase Distortion | Non-linear phase accumulator, 3 resonant waveshapes | DCW amount |

**Blend Engine:** Equal-power crossfade (cos/sin curves). Shared parameters (pitch, decay, snap, tone) affect both layers coherently. `character` knob adapts meaning based on blend position.

**Cross-Voice Coupling Matrix:**

| Type | Signal Path | Musical Effect |
|------|------------|---------------|
| Amp→Filter | Voice A amplitude → Voice B filter cutoff | Rhythmic filtering |
| Amp→Pitch | Voice A amplitude → Voice B pitch | Pitch ducking |
| Amp→Choke | Voice A trigger → Voice B kill | Exclusive grouping |
| Rhythm→Blend | Voice A pattern → Voice B blend | Synthesis morphing |
| Env→Decay | Voice A envelope → Voice B decay time | Dynamic interaction |
| Audio→Ring | Voice A × Voice B multiplication | Metallic fusion |

**Default Normalled Connections:** HH-Closed chokes HH-Open, Kick→Snare filter ducking 15%, Snare→HH-C decay 10%

**Step Sequencer:** Per-voice lanes, per-step parameter locks (Elektron-style: blend, pitch, decay, snap per step), probability triggers, condition triggers, ratchets (1–4 micro-repeats)

**Macros:** MACHINE (all blend positions), PUNCH (all snap+body), SPACE (reverb+delay send), MUTATE (±20% randomize all voices)

**Parameters:** ~110 | **CPU:** <15.5% | **Voices:** 8 dedicated

**SynthEngine Mapping:**
- `getSampleForCoupling()`: master output (summed kit) OR `getVoiceOutput(int)` for per-voice coupling
- `applyCouplingInput()`: individual voice decay, blend position, filter cutoff (external engines modulate the kit)

---

## 4. Cross-Module Synergy Matrix

What happens when modules are chained through the coupling matrix:

| Pairing | Coupling Route | Musical Outcome | Category |
|---------|---------------|----------------|----------|
| **SNAP → MORPH** | X amplitude → O filter cutoff | Classic dub pump — pads breathe with percussive hits | Pumped Pads |
| **MORPH → SNAP** | O LFO → X pitch | Organic pitch drift on percussive engine — alive, breathing plucks | Living Plucks |
| **FAT → BITE** | FAT output → BITE filter input | 13-oscillator width through Gnash/Trash character stages — massive aggressive bass | Fat Bite Bass |
| **BITE → FAT** | BITE Belly macro → FAT Mojo | Character-driven analog drift — BITE's warmth softens FAT's precision | Warm Width |
| **ONSET → MORPH** | Kick amplitude → pad filter cutoff | Drum hits pump the pad engine — classic dub techno | Pumped Pads |
| **ONSET → DUB** | Drum output → DUB send input | Drums through tape delay + spring reverb — dub percussion | Dub Drums |
| **DRIFT → MORPH** | JOURNEY/Climax → morph position | Climax system drives wavetable morphing — psychedelic evolution | Alien Orchestra |
| **FAT → DUB** | FAT output → DUB send chain | 13 oscillators through self-oscillating tape delay — massive dub drones | Dub Drones |
| **BITE → ONSET** | Bass amplitude → kick decay | When bass hits, kick shortens — tight locked groove | Lock Groove |
| **ONSET → DRIFT** | Hat pattern triggers → DRIFT shimmer | Hat rhythm modulates psychedelic shimmer — textural rhythm | Psychedelic Rhythm |
| **SNAP → DUB** | Percussive hits → DUB send/return | Short plucks through runaway delay — dub techno stabs | Dub Techno |
| **DRIFT → FAT** | Drift mod source → FAT morph position | Voyager Drift applied to 65 oscillators — massive evolving pad | Drifting Width |
| **FAT + BITE + DUB** | FAT→BITE character, sum→DUB FX | Triple-chained: width + character + dub FX architecture | Mega Bass |
| **ONSET → BITE** | Snare amplitude → BITE gnash amount | Snare hits increase bass aggression — interactive rhythm+bass | Living Texture |
| **MORPH → DRIFT** | Morph LFO → DRIFT JOURNEY macro | Slow wavetable LFO drives the Climax system — auto-journey | Auto Journey |

---

## 5. Shared DSP Library

Components appearing in 3+ engines that should become shared code:

| Component | Used In | Implementation | Notes |
|-----------|---------|---------------|-------|
| **Cytomic SVF** | BITE, SNAP, ONSET, DRIFT | Topology-preserving transform (Andy Simper) | Reference: XOppossum FilterBlock.h |
| **PolyBLEP** | FAT, SNAP, DRIFT, ONSET | Band-limited oscillator transitions | Reference: OddfeliX/OddOscar Oscillators.h |
| **ParamSnapshot** | All engines | Cache all param pointers once per block | Zero-cost per-sample access |
| **FastMath** | BITE, FAT, ONSET | fastSin, fastTanh, fastExp2, fastAtan, fastIsFinite | Chebyshev/rational approximations |
| **ADSR Envelope** | All engines | Attack-Decay-Sustain-Release with optional AHD/AD modes | Shared base class, per-engine extensions |
| **tanh Saturation** | FAT, BITE, DUB, ONSET | Waveshaping for drive/warmth | Single shared function |
| **DC Blocker** | BITE, ONSET, DUB | `y[n] = x[n] - x[n-1] + R × y[n-1]` | Required after asymmetric saturation |
| **Wavetable Reader** | FAT, MORPH, DRIFT | 2048-sample frames, linear interpolation, mip-mapping | Shared loader + playback |
| **Dattorro Reverb** | FAT, (potential shared FX) | Plate reverb algorithm | Could become shared FX rack reverb |

**Recommendation:** Extract these into a `Shared/DSP/` directory. Each engine links against the shared library. Reduces code duplication and ensures bug fixes propagate.

---

## 6. New Preset Categories from Chaining

Sounds that are **only possible** through module coupling — impossible with any single engine:

| Category | Engine Combo | Why It's New | Example Preset |
|----------|-------------|-------------|----------------|
| **Pumped Pads** | ONSET + any melodic | Drum synthesis drives pad dynamics in real-time | "Breathing Cathedral" |
| **Fat Bite Bass** | FAT + BITE | 13-osc width through per-voice character stages | "Earthquake Sub" |
| **Dub Drones** | Any + DUB | Self-oscillating tape delay creates drone beds from any source | "Tape Meditation" |
| **Psychedelic Rhythm** | DRIFT + ONSET | Climax system modulated by drum patterns | "Journey Drums" |
| **Living Texture** | 3+ engines light coupling | Emergent organic movement from multiple interactions | "Symbiosis" |
| **Morphing Drums** | ONSET + MORPH | Wavetable position driven by drum amplitude | "Evolving Kit" |
| **Dub Techno** | SNAP + DUB | Percussive plucks through runaway delay | "Chord Stab Echo" |
| **Lock Groove** | BITE + ONSET | Bass and drums dynamically interact (amplitude↔decay) | "Locked In" |
| **Auto Journey** | MORPH + DRIFT | LFO-driven Climax system — plays itself | "Autopilot" |
| **Alien Orchestra** | DRIFT + FAT + MORPH | Voyager Drift across 65 oscillators + wavetable morph | "First Contact" |
| **Character Width** | FAT + BITE | Mojo warmth applied through Belly↔Bite character axis | "Warm Expanse" |
| **Siren Pad** | DUB + DRIFT | Dub siren voice through psychedelic trait processing | "Siren Shimmer" |

**Estimated new chained presets:** 85 (across all categories)

---

## 7. CPU Budget Summary

### Two-Engine Configurations (Default)

| Pairing | Engine A | Engine B | Coupling | Shared FX | Total |
|---------|----------|----------|----------|-----------|-------|
| SNAP + MORPH | 8% | 12% | 2% | 6% | **28%** |
| FAT + BITE | 12% | 10% | 2% | 6% | **30%** |
| ONSET + DUB | 15.5% | 8% | 2% | 0% (DUB IS FX) | **25.5%** |
| FAT + DUB | 12% | 8% | 2% | 0% | **22%** |
| DRIFT + MORPH | 15% | 12% | 2% | 6% | **35%** |

### Three-Engine Configurations (Expanded)

| Config | Engines | Couplings (×3 pairs) | Shared FX | Total | Voice Reduction |
|--------|---------|---------------------|-----------|-------|-----------------|
| FAT + BITE + DUB | 12+10+8 | 6% | 0% | **36%** | FAT: 5→3 voices |
| SNAP + MORPH + ONSET | 8+12+15.5 | 6% | 6% | **47.5%** | MORPH: 16→8 voices |
| DRIFT + FAT + DUB | 15+12+8 | 6% | 0% | **41%** | DRIFT: 24→12 voices |

### Four-Engine Configurations (Maximum)

| Config | Raw Total | With Voice Reduction | Notes |
|--------|-----------|---------------------|-------|
| 4 engines average | ~52% raw | ~45% with reductions | Approaching limit |
| Inactive engines | +0% per inactive | — | Full deactivation = zero CPU |

### Mitigation Strategies
1. Only 2 engines active by default; 3-4 is "expanded mode" with warning
2. Shared FX rack (one reverb/delay instance, not per-engine) when possible
3. Inactive engines contribute 0% CPU (no silent rendering)
4. Per-engine voice count reduction when multiple engines active
5. Quality mode toggle: eco (filter coeffs batched every 4 samples) vs standard

---

## 8. Recommended Build Priority

Ordered by integration value ÷ implementation effort:

| Priority | Module | Rationale | Effort | Value |
|----------|--------|-----------|--------|-------|
| **1** | SNAP + MORPH | Already coupled in OddfeliX/OddOscar — wrap existing code behind SynthEngine interface | Low | High (proves architecture) |
| **2** | DUB | Simplest engine (38 params), unique as shared FX module | Low | High (every engine benefits) |
| **3** | FAT | Complete engine, unique width character, straightforward wrapping | Medium | High (flagship sound) |
| **4** | BITE | Most parameters (122) but clean architecture, strong bass identity | Medium | High (fills bass gap) |
| **5** | DRIFT | Complex (130 params, 7 traits, Climax), but powerful sonic payoff | High | High (unique journey concept) |
| **6** | ONSET | Not yet built — full implementation required from spec | High | High (completes the platform with drums) |

**MVP:** SNAP + MORPH + DUB (priorities 1-2). This gives: dual melodic engines with coupling + dub FX architecture + PlaySurface. Proves the mega-tool concept with minimal new code.

---

*CONFIDENTIAL — XO_OX Internal Design Document*
