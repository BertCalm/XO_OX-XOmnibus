# OPERA Hardware Controller Mapping

**Engine:** OPERA — XOpera (Engine #45)
**Accent:** Aria Gold `#D4AF37`
**Parameter Prefix:** `opera_`
**Total Params:** 45
**Seance Score:** 8.85/10 (P0 fixed 2026-03-22, Blessings B035/B036/B037)
**Macros:** DRAMA (Kuramoto synchronicity) / VOICE (formant character) / CHORUS (unison width) / STAGE (reverb depth)
**Unique Architecture:** Additive-vocal synthesis via Kuramoto oscillator field + autonomous OperaConductor dramatic arc

---

## Parameter Reference

### Core Synthesis (18 params)
| ID | Name | Range | Default | Notes |
|----|------|-------|---------|-------|
| `opera_drama` | Drama | 0.0–1.0 | 0.35 | Kuramoto coupling K — synchronicity depth |
| `opera_voice` | Voice | 0.0–1.0 | 0.5 | Vowel formant morph position |
| `opera_vowelA` | Vowel A | 0–5 (/a/e/i/o/u/schwa) | 0 (/a/) | Formant target when Voice=0 |
| `opera_vowelB` | Vowel B | 0–5 | 3 (/o/) | Formant target when Voice=1 |
| `opera_breath` | Breath | 0.0–1.0 | 0.2 | Breath noise injection into partials |
| `opera_effort` | Effort | 0.0–1.0 | 0.5 | Spectral brightness — vocal effort |
| `opera_partials` | Partials | 4–48 | 32 | Additive oscillator count per voice |
| `opera_detune` | Detune | 0.0–1.0 | 0.1 | Partial frequency scatter |
| `opera_tilt` | Spectral Tilt | -1.0–1.0 | 0.0 | -1=dark (bass emphasis), +1=bright; bipolar |
| `opera_fundamental` | Fundamental | -36–+36 st | 0 | Pitch transposition in semitones |
| `opera_resSens` | Resonance Sens | 0.0–1.0 | 0.5 | Vocal tract resonance sensitivity |
| `opera_portamento` | Portamento | 0.0–1.0 | 0.0 | Glide time |
| `opera_unison` | Unison | 1–4 voices | 1 | Voice stacking |
| `opera_width` | Stereo Width | 0.0–1.0 | 0.5 | B036 — Kuramoto-driven spatial panning |
| `opera_vibRate` | Vibrato Rate | 0.01–20 Hz | 5.5 Hz | |
| `opera_vibDepth` | Vibrato Depth | 0.0–1.0 | 0.25 | |
| `opera_stage` | Stage | 0.0–1.0 | 0.3 | ReactiveStage depth — Tomita reverb |
| `opera_responseSpeed` | Response Speed | 0.0–1.0 | 0.5 | Arc tracking speed |

### Filter (7 params)
| ID | Name | Range | Default | Notes |
|----|------|-------|---------|-------|
| `opera_filterCutoff` | Filter Cutoff | 20–20kHz (log, skew 0.3) | 8000 Hz | |
| `opera_filterRes` | Filter Resonance | 0.0–1.0 | 0.0 | |
| `opera_filterEnvAmt` | Filter Env Amt | -1.0–1.0 | 0.3 | Bipolar — negative sweeps down |
| `opera_filterA` | Filter Attack | 0.0–1.0 (scaled) | 0.01 | |
| `opera_filterD` | Filter Decay | 0.0–1.0 (scaled) | 0.3 | |
| `opera_filterS` | Filter Sustain | 0.0–1.0 | 0.5 | |
| `opera_filterR` | Filter Release | 0.0–1.0 (scaled) | 0.4 | |

### Amp Envelope (4 params)
| ID | Name | Range | Default | Notes |
|----|------|-------|---------|-------|
| `opera_ampA` | Amp Attack | 0.0–1.0 (→real seconds) | 0.05 | |
| `opera_ampD` | Amp Decay | 0.0–1.0 | 0.2 | |
| `opera_ampS` | Amp Sustain | 0.0–1.0 | 0.8 | |
| `opera_ampR` | Amp Release | 0.0–2.0 (cubic scale) | 0.6 | 2.0 ≈ 80s for Flux/Schulze-scale arcs |

### LFOs (6 params — 2 LFOs × 3 each)
| ID | Name | Range | Default | Notes |
|----|------|-------|---------|-------|
| `opera_lfo1Rate` | LFO1 Rate | 0.01–30 Hz | 0.1 Hz | Slow by default — breathing |
| `opera_lfo1Depth` | LFO1 Depth | 0.0–1.0 | 0.3 | |
| `opera_lfo1Dest` | LFO1 Dest | 0–7 (Drama/Voice/Breath/Effort/Tilt/Cut/VibDepth/ResSens) | 1 (Voice) | |
| `opera_lfo2Rate` | LFO2 Rate | 0.01–30 Hz | 3.0 Hz | Faster by default |
| `opera_lfo2Depth` | LFO2 Depth | 0.0–1.0 | 0.0 | Off by default |
| `opera_lfo2Dest` | LFO2 Dest | 0–7 | 0 (Drama) | |

### Conductor (4 params — B035 Blessing)
| ID | Name | Range | Default | Notes |
|----|------|-------|---------|-------|
| `opera_arcMode` | Arc Mode | 0=Manual, 1=Conductor, 2=Both | 1 | Default is autonomous Conductor ON |
| `opera_arcShape` | Arc Shape | 0–3 (Sigmoid/Bell/Plateau/Inverse) | 1 (Bell) | Dramatic arc curve |
| `opera_arcTime` | Arc Time | 0.5–3600 s | 8.0 s | Arc duration (3600s = Schulze-scale) |
| `opera_arcPeak` | Arc Peak | 0.0–1.0 | 0.8 | Max Drama K reached at arc apex |

### Expression Routing (6 params)
| ID | Name | Range | Default | Notes |
|----|------|-------|---------|-------|
| `opera_modWheelDest` | MW Dest | 0–7 | 0 (Drama) | Mod wheel target |
| `opera_modWheelAmt` | MW Amount | 0.0–1.0 | 0.5 | Mod wheel depth |
| `opera_atDest` | AT Dest | 0–7 | 3 (Effort) | Aftertouch target — pressure → effort |
| `opera_atAmt` | AT Amount | 0.0–1.0 | 0.5 | Aftertouch depth |
| `opera_velToFilter` | Vel→Filter | 0.0–1.0 | 0.4 | Velocity scales filter cutoff |
| `opera_velToEffort` | Vel→Effort | 0.0–1.0 | 0.3 | Velocity scales effort |

---

## Push 2/3 Mapping

### Philosophy
OPERA is the most theatrical engine in the fleet. On Push, this means the top knob row handles the live performance layer — the things a vocalist would control breath-to-breath. The second row manages the Conductor arc and formant architecture. Chromatic pads let you play it like a polyphonic vocal instrument.

OPERA's unique feature for Push: the **Conductor arc** means you can set the arc time with a knob, press play, and watch the synthesizer perform its own dramatic arc — hands-free. Your job is to intervene at key moments.

### Knob Layout

```
TOP ROW (8 knobs — Tier 1: Vocal Expression)
┌──────────┬──────────┬──────────┬──────────┬──────────┬──────────┬──────────┬──────────┐
│  DRAMA   │  VOICE   │  BREATH  │  EFFORT  │  Filter  │  Filter  │ Vib Rate │ Vib Depth│
│ K sync   │ formant  │  noise   │ spectral │  Cutoff  │  Reso    │ 0.01-20Hz│  0→1     │
│  0→1     │  0→1     │  0→1     │  0→1     │ 20-20k   │  0→1     │          │          │
└──────────┴──────────┴──────────┴──────────┴──────────┴──────────┴──────────┴──────────┘

ROW 2 (8 knobs — Tier 2: Architecture + Conductor)
┌──────────┬──────────┬──────────┬──────────┬──────────┬──────────┬──────────┬──────────┐
│  Spec    │ Res Sens │  Stage   │  Width   │  Arc     │  Arc     │  Arc     │  Arc     │
│  Tilt    │  0→1     │  reverb  │ stereo   │  Mode    │  Shape   │  Time    │  Peak    │
│ bipolar  │          │  0→1     │  0→1     │ 0/1/2    │  0-3     │  0.5-3600│  0→1     │
└──────────┴──────────┴──────────┴──────────┴──────────┴──────────┴──────────┴──────────┘
```

### Page System

**Page A — Vocal Core (default)**
- Knobs: Drama | Voice | Breath | Effort | Spec Tilt | ResSens | Partials | Detune
- Buttons: [Vowel A prev] [Vowel A next] [Vowel B prev] [Vowel B next] [Unison 1] [Unison 2] [Unison 3] [Unison 4]

**Page B — Filter**
- Knobs: Cutoff | Resonance | Env Amt | Filter A | Filter D | Filter S | Filter R | Vel→Filter
- Buttons: [Env Amt bipolar display] — no mode buttons needed here

**Page C — Amp Envelope + LFO**
- Knobs: Amp A | Amp D | Amp S | Amp R | LFO1 Rate | LFO1 Depth | LFO2 Rate | LFO2 Depth
- Buttons: [LFO1 Dest 0] [LFO1 Dest 1] ... [LFO2 Dest cycle]

**Page D — Conductor + Expression**
- Knobs: Arc Mode | Arc Shape | Arc Time | Arc Peak | MW Amt | AT Amt | Vel→Filter | Vel→Effort
- Buttons: [Arc Mode cycle] [Arc Shape cycle] [MW Dest cycle] [AT Dest cycle] [Portamento] [Vibrato on/off] [Fundamental ↑] [Fundamental ↓]

### Pad Matrix (Chromatic — OPERA in 4ths layout)

OPERA is a melodic polyphonic engine — chromatic pads, played like a keytar:

```
Standard Push 4th-interval chromatic layout
E4  F4  F#4  G4  G#4  A4  A#4  B4
C4  C#4  D4  D#4  E4  F4  F#4  G4
Ab3  A3  Bb3  B3  C4  C#4  D4  Eb4
E3  F3  F#3  G3  G#3  A3  A#3  B3
```

Pad behavior:
- Velocity → filter brightness + effort (dual D001 compliance)
- Aftertouch → Effort (default) — press harder = more spectral brightness
- Pad color: Aria Gold `#D4AF37` when active, dim gold when idle

**Conductor-aware LED behavior:** When Arc Mode = Conductor, the pad ring brightness slowly crescendos and decrescendos to mirror the dramatic arc — subtle visual feedback of the autonomous performance.

**Special buttons (bottom strip):**
- [Arc Play/Pause]: Trigger Conductor arc reset (same as conductor CC20 > 63)
- [Portamento on/off]: Toggle glide
- [Unison cycle]: 1→2→3→4 voices

---

## Maschine Mapping

### Philosophy
OPERA on Maschine works best with pads in Keyboard mode (chromatic). Smart Strips control the 4 real-time performance axes. Because OPERA has an autonomous Conductor, you can leave Arc Mode = 1 and the engine performs itself — your hands are free to modulate Drama and Voice in real time.

### Smart Strips (always-visible)
| Strip | Parameter | Ergonomic Note |
|-------|-----------|----------------|
| Strip 1 | `opera_drama` | DRAMA — Kuramoto synchronicity, most dramatic macro |
| Strip 2 | `opera_voice` | VOICE — vowel morph A→B |
| Strip 3 | `opera_filterCutoff` | Filter cutoff — live breath control |
| Strip 4 | `opera_breath` | Breath noise — vocal air injection |
| Strip 5 | `opera_effort` | Effort — spectral brightness |
| Strip 6 | `opera_stage` | Stage / reverb depth |
| Strip 7 | `opera_vibDepth` | Vibrato depth |
| Strip 8 | `opera_width` | Stereo width (Kuramoto-coherence panning) |

### Knob Pages

**Page 1 — Vocal Performance**
| K1 | K2 | K3 | K4 | K5 | K6 | K7 | K8 |
|----|----|----|----|----|----|----|-----|
| Drama | Voice | Breath | Effort | Spec Tilt | Res Sens | Vib Rate | Vib Depth |

**Page 2 — Envelope + Filter**
| K1 | K2 | K3 | K4 | K5 | K6 | K7 | K8 |
|----|----|----|----|----|----|----|-----|
| Amp A | Amp D | Amp S | Amp R | Flt Cut | Flt Res | Flt Env | Flt Dcay |

**Page 3 — Conductor (B035)**
| K1 | K2 | K3 | K4 | K5 | K6 | K7 | K8 |
|----|----|----|----|----|----|----|-----|
| Arc Mode | Arc Shape | Arc Time | Arc Peak | Response Spd | Portamento | Partials | Detune |

**Page 4 — Expression + Architecture**
| K1 | K2 | K3 | K4 | K5 | K6 | K7 | K8 |
|----|----|----|----|----|----|----|-----|
| MW Amt | AT Amt | Vel→Flt | Vel→Effort | LFO1 Rate | LFO1 Depth | LFO2 Rate | LFO2 Depth |

### Pad Layout (Keyboard Mode — C3 root)

```
G3  Ab3  A3  Bb3
Eb3  E3  F3  F#3
C3  C#3  D3  D#3  ← Root
Ab2  A2  Bb2  B2
```

Maschine touch strips can be assigned to Drama and Voice for smooth formant morphing while playing. Use Maschine's Chord mode to trigger the engine's 8-voice polyphony with minimal finger movement.

---

## Generic MIDI CC Map

### Design Notes
- OPERA's most important real-time parameters are Drama, Voice, Breath, Effort, and Filter Cutoff
- Bipolar parameters (Spectral Tilt, Filter Env Amt) center at CC value 64 = 0.0
- Arc Time is logarithmic — CC 1 ≈ 0.5s, CC 64 ≈ 8s, CC 127 ≈ 3600s. Requires curve mapping.
- Conductor trigger: CC20 > 63 = reset arc (hardcoded in engine)
- Mod Wheel (CC01) is hardwired in the engine to modWheelDest/Amt

| CC# | Parameter ID | Name | Range Notes |
|-----|-------------|------|-------------|
| **Tier 1 — Vocal Expression (highest priority live)** | | | |
| CC01 | `opera_drama` | DRAMA (via MW routing) | MW drives Drama by default; 0=chaos, 127=fully locked |
| CC02 | `opera_voice` | VOICE (formant morph) | 0=Vowel A, 127=Vowel B |
| CC03 | `opera_breath` | Breath Noise | 0=pure tone, 127=full breath |
| CC04 | `opera_effort` | Effort (spectral bright) | 0=dark/relaxed, 127=bright/strained |
| CC05 | `opera_filterCutoff` | Filter Cutoff | [log] 20Hz–20kHz |
| CC06 | `opera_filterRes` | Filter Resonance | 0–127 |
| CC07 | `opera_tilt` | Spectral Tilt | 64=flat, 0=bass, 127=treble; bipolar |
| CC08 | `opera_stage` | Stage (ReactiveStage depth) | 0=dry, 127=full Tomita reverb |
| **Tier 2 — Shape** | | | |
| CC09 | `opera_vibRate` | Vibrato Rate | [log] 0.01–20Hz |
| CC10 | `opera_vibDepth` | Vibrato Depth | 0=none, 127=max |
| CC11 | `opera_width` | Stereo Width | 0=mono, 127=wide |
| CC12 | `opera_resSens` | Resonance Sensitivity | 0–127 |
| CC13 | `opera_detune` | Partial Detune | 0=pure, 127=scattered |
| CC14 | `opera_portamento` | Portamento | 0=instant, 127=max glide |
| **Tier 3 — Amp Envelope** | | | |
| CC15 | `opera_ampA` | Amp Attack | 0=instant, 127=slow |
| CC16 | `opera_ampD` | Amp Decay | 0–127 |
| CC17 | `opera_ampS` | Amp Sustain | 0=none, 127=full |
| CC18 | `opera_ampR` | Amp Release | [log, cubic] 0=instant, 127≈80s |
| **Tier 4 — Filter Envelope** | | | |
| CC19 | `opera_filterEnvAmt` | Filter Env Amount | 64=zero, bipolar sweep |
| CC20 | Conductor trigger | Arc Reset (hardcoded) | >63 = trigger arc restart |
| CC21 | `opera_arcMode` | Arc Mode | 0–42=Manual, 43–85=Conductor, 86–127=Both |
| CC22 | `opera_arcShape` | Arc Shape | 0–31=Sigmoid, 32–63=Bell, 64–95=Plateau, 96–127=Inverse |
| CC23 | `opera_arcTime` | Arc Time | [log] 0=0.5s, 64=8s, 127=3600s |
| CC24 | `opera_arcPeak` | Arc Peak | 0=quiet, 127=full dramatic crescendo |
| **Tier 5 — LFO + Expression** | | | |
| CC25 | `opera_lfo1Rate` | LFO1 Rate | [log] 0.01–30Hz |
| CC26 | `opera_lfo1Depth` | LFO1 Depth | 0–127 |
| CC27 | `opera_lfo2Rate` | LFO2 Rate | [log] 0.01–30Hz |
| CC28 | `opera_lfo2Depth` | LFO2 Depth | 0–127 |
| CC29 | `opera_modWheelAmt` | MW Amount | 0–127 |
| CC30 | `opera_atAmt` | Aftertouch Amount | 0–127 |
| CC31 | `opera_responseSpeed` | Response Speed | 0=slow, 127=fast |
| **Coupling Bus** | | | |
| CC80 | xoceanus coupling A→B | Coupling Out | see xoceanus map |
| CC81 | xoceanus coupling B→A | Coupling In | see xoceanus map |
| CC82 | xoceanus coupling type | Coupling Type | 0–14 |
| CC83 | xoceanus coupling bypass | Bypass | 0=active, 127=bypass |

### Recommended Mod Wheel Routing
- **Default (engine hardcoded):** CC01 → Drama (Kuramoto K) with `opera_modWheelAmt` depth
- This means the mod wheel directly controls how synchronised/chaotic the partial field is
- At MW=0: partials scatter (chaos) | At MW=127: partials lock (unison-like coherence)

### Recommended Aftertouch Routing
- **Default:** AT → Effort (spectral brightness) with `opera_atAmt` depth
- Pressure → timbral brightness mimics real vocal physiology
- For a more dramatic effect: route AT → Drama and press hard to push toward Kuramoto lock

---

## OPERA-Specific Performance Notes

### Working with the Conductor (B035)
The OperaConductor is OPERA's most distinctive feature — no other synth in the fleet has it.

**Arc Mode = 1 (Conductor only):**
- Set `opera_arcTime` to your song's phrase length (e.g., 16 bars ≈ 32s at 120 BPM)
- Set `opera_arcPeak` to 0.7–0.9
- Press play. The engine performs a complete dramatic arc on its own.
- You intervene by adding notes, adjusting Voice formant, or grabbing filter.

**Arc Mode = 2 (Both — Conductor + Manual):**
- The engine takes max(Conductor K, manual Drama K)
- Play the arc AND add your own Drama gestures on top
- This is the recommended performance mode for live sets

**Arc Time tips:**
- 4–8s: One-shot drama (good for FX, not sustained passages)
- 30–60s: Verse-length arc
- 300s+: Slow burn — Schulze/ambient mode
- 3600s: One hour. Set and forget.

### B036 — Stereo as Dramatic Indicator
Watch the stereo width as you play. When `opera_drama` is low, the Kuramoto field is chaotic — the stereo image is wide and scattered. As drama increases and partials lock together, the stereo image narrows toward center. This is not a bug — it's OPERA telling you where it is emotionally. Map `opera_width` if you want to manually override this behavior.

### B037 — Emotional Memory (within 500ms)
If you play two notes with less than 500ms gap, the Kuramoto field remembers where the partial phases were on the previous note. The new note wakes up coherent — fewer clicks, more legato vocal feel. Exploit this by playing legato phrases.
