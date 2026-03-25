# OBRIX Hardware Controller Mapping

**Engine:** OBRIX — Ocean Bricks: The Living Reef
**Accent:** Reef Jade `#1E8B7E`
**Parameter Prefix:** `obrix_`
**Total Params:** 81 (Waves 1–5)
**Macros:** CHARACTER (brick topology bias) / MOVEMENT (LFO rate + audio-rate unlock) / COUPLING (reef resident ecology) / SPACE (FX depth)

---

## Parameter Reference (by function group)

### Sources (Shells)
| ID | Name | Range | Default | Notes |
|----|------|-------|---------|-------|
| `obrix_src1Type` | Source 1 Type | 0–8 (Off/Sine/Saw/Sq/Tri/Noise/WT/Pulse/LoFiSaw) | 2 (Saw) | Primary oscillator |
| `obrix_src2Type` | Source 2 Type | 0–8 | 0 (Off) | Secondary oscillator |
| `obrix_src1Tune` | Src 1 Tune | -24 to +24 st | 0 | Semitone offset |
| `obrix_src2Tune` | Src 2 Tune | -24 to +24 st | 0 | Semitone offset |
| `obrix_src1PW` | Src 1 Pulse Width | 0.05–0.95 | 0.5 | Only active when Pulse selected |
| `obrix_src2PW` | Src 2 Pulse Width | 0.05–0.95 | 0.5 | Only active when Pulse selected |
| `obrix_srcMix` | Source Mix | 0.0–1.0 | 0.5 | 0=Src1 only, 1=Src2 only |
| `obrix_fmDepth` | FM Depth | -1.0–1.0 (→±24 st) | 0.0 | Src1→Src2 FM; bipolar |
| `obrix_wtBank` | Wavetable Bank | 0–3 (Analog/Vocal/Metallic/Organic) | 0 | Active when Src=Wavetable |
| `obrix_unisonDetune` | Unison Detune | 0–50 ct | 0 | Voice spread |

### Processors (Coral)
| ID | Name | Range | Default | Notes |
|----|------|-------|---------|-------|
| `obrix_proc1Type` | Proc 1 Type | 0–5 (Off/LP/HP/BP/Wavefold/RingMod) | 1 (LP) | On Src1 path |
| `obrix_proc1Cutoff` | Proc 1 Cutoff | 20–20kHz (log) | 8000 Hz | |
| `obrix_proc1Reso` | Proc 1 Resonance | 0.0–1.0 | 0.0 | |
| `obrix_proc1Feedback` | Proc 1 Feedback | 0.0–1.0 | 0.0 | tanh saturation; 1.0=self-osc |
| `obrix_proc2Type` | Proc 2 Type | 0–5 | 0 (Off) | On Src2 path |
| `obrix_proc2Cutoff` | Proc 2 Cutoff | 20–20kHz (log) | 4000 Hz | |
| `obrix_proc2Reso` | Proc 2 Resonance | 0.0–1.0 | 0.0 | |
| `obrix_proc2Feedback` | Proc 2 Feedback | 0.0–1.0 | 0.0 | |
| `obrix_proc3Type` | Proc 3 Type | 0–5 | 0 (Off) | Post-mix path |
| `obrix_proc3Cutoff` | Proc 3 Cutoff | 20–20kHz (log) | 4000 Hz | |
| `obrix_proc3Reso` | Proc 3 Resonance | 0.0–1.0 | 0.0 | |

### Amplitude Envelope
| ID | Name | Range | Default |
|----|------|-------|---------|
| `obrix_ampAttack` | Amp Attack | 0.0–10.0 s (log) | 0.01 s |
| `obrix_ampDecay` | Amp Decay | 0.0–10.0 s (log) | 0.3 s |
| `obrix_ampSustain` | Amp Sustain | 0.0–1.0 | 0.7 |
| `obrix_ampRelease` | Amp Release | 0.0–20.0 s (log) | 0.5 s |

### Modulators (4 slots)
| ID | Name | Range | Default |
|----|------|-------|---------|
| `obrix_mod1Type` | Mod 1 Type | 0–4 (Off/ADSR/LFO/Vel/AT) | 1 (ADSR) |
| `obrix_mod1Target` | Mod 1 Target | 0–8 (None/Pitch/Cut/Reso/Vol/WTPos/PW/FXMix/Pan) | 2 (Cut) |
| `obrix_mod1Depth` | Mod 1 Depth | -1.0–1.0 | 0.5 — bipolar |
| `obrix_mod1Rate` | Mod 1 Rate | 0.01–30 Hz (log) | 1.0 Hz |
| `obrix_mod2Type` | Mod 2 Type | 0–4 | 2 (LFO) | |
| `obrix_mod2Depth` | Mod 2 Depth | -1.0–1.0 | 0.0 — bipolar |
| `obrix_mod3Type` | Mod 3 Type | 0–4 | 3 (Vel) | |
| `obrix_mod4Type` | Mod 4 Type | 0–4 | 4 (AT) | |

### Effects (3 slots)
| ID | Name | Range | Default |
|----|------|-------|---------|
| `obrix_fx1Type` | FX 1 Type | 0–3 (Off/Delay/Chorus/Reverb) | 0 |
| `obrix_fx1Mix` | FX 1 Mix | 0.0–1.0 | 0.0 |
| `obrix_fx1Param` | FX 1 Param | 0.0–1.0 | 0.3 |
| `obrix_fx2Type` | FX 2 Type | 0–3 | 0 |
| `obrix_fx2Mix` | FX 2 Mix | 0.0–1.0 | 0.0 |
| `obrix_fx3Type` | FX 3 Type | 0–3 | 0 |
| `obrix_fx3Mix` | FX 3 Mix | 0.0–1.0 | 0.0 |
| `obrix_fxMode` | FX Mode | 0=Serial, 1=Parallel | 0 |

### Biophonic / Wave 4
| ID | Name | Range | Default | Notes |
|----|------|-------|---------|-------|
| `obrix_fieldStrength` | Field Strength | 0.0–1.0 | 0.0 | JI attractor intensity |
| `obrix_fieldPolarity` | Field Polarity | -1.0–1.0 | 1.0 | 1=attract, -1=repulse |
| `obrix_fieldRate` | Field Rate | IIR rate | 0.01 | Convergence speed |
| `obrix_fieldPrimeLimit` | Prime Limit | 0–2 (3/5/7-limit) | 1 | JI lattice |
| `obrix_envTemp` | Temperature | 0.0–1.0 | 0.0 | Drift rate scaling |
| `obrix_envPressure` | Pressure | 0.0–1.0 | 0.5 | LFO rate scaling |
| `obrix_envCurrent` | Current | -1.0–1.0 | 0.0 | Cutoff bias ±2kHz; bipolar |
| `obrix_envTurbidity` | Turbidity | 0.0–1.0 | 0.0 | Spectral noise add |
| `obrix_competitionStrength` | Competition | 0.0–1.0 | 0.0 | Src1↔Src2 amplitude ecology |
| `obrix_symbiosisStrength` | Symbiosis | 0.0–1.0 | 0.0 | Noise Src1→FM Src2 |
| `obrix_stressDecay` | Stress Decay | 0.0–1.0 | 0.0 | Velocity leaky integrator |
| `obrix_bleachRate` | Bleach Rate | 0.0–1.0 | 0.0 | High-register harmonic attenuation |

### Reef Residency / Wave 5
| ID | Name | Range | Default | Notes |
|----|------|-------|---------|-------|
| `obrix_reefResident` | Reef Resident | 0–3 (Off/Competitor/Symbiote/Parasite) | 0 | Coupling ecology mode |
| `obrix_residentStrength` | Resident Strength | 0.0–1.0 | 0.3 | Coupling influence depth |

### Wave 3 / Spatial
| ID | Name | Range | Default | Notes |
|----|------|-------|---------|-------|
| `obrix_driftRate` | Drift Rate | 0.001–0.05 Hz | 0.005 | Berlin School ensemble drift |
| `obrix_driftDepth` | Drift Depth | 0.0–1.0 | 0.0 | Pitch+filter drift amount |
| `obrix_journeyMode` | Journey Mode | 0/1 | 0 | Suppress note-off |
| `obrix_distance` | Distance | 0.0–1.0 | 0.0 | HF rolloff (air absorption) |
| `obrix_air` | Air | 0.0–1.0 | 0.5 | 0=warm/bass, 1=cold/treble |

### Global
| ID | Name | Range | Default |
|----|------|-------|---------|
| `obrix_macroCharacter` | CHARACTER | 0.0–1.0 | 0.0 |
| `obrix_macroMovement` | MOVEMENT | 0.0–1.0 | 0.0 |
| `obrix_macroCoupling` | COUPLING | 0.0–1.0 | 0.0 |
| `obrix_macroSpace` | SPACE | 0.0–1.0 | 0.0 |
| `obrix_level` | Level | 0.0–1.0 | 0.8 |
| `obrix_glideTime` | Glide | 0.0–1.0 (log) | 0.0 |
| `obrix_pitchBendRange` | PB Range | 1–24 st | 2 |
| `obrix_polyphony` | Voice Mode | Mono/Legato/Poly4/Poly8 | Poly8 |
| `obrix_gestureType` | Gesture | Ripple/Pulse/Flow/Tide | Ripple |

---

## Push 2/3 Mapping

### Philosophy
OBRIX is a habitat, not an instrument. On Push, Tier 1 knobs are the reef's vital signs — the parameters that change character fastest. Tier 2 is ecology and spatial. The pad matrix in chromatic mode lets you build coral from any pitch.

### Knob Layout

```
TOP ROW (8 knobs — Tier 1: Core Reef Vitals)
┌──────────┬──────────┬──────────┬──────────┬──────────┬──────────┬──────────┬──────────┐
│  Proc1   │  Proc1   │  Src Mix │  FM Depth│CHARACTER │MOVEMENT  │ COUPLING │  SPACE   │
│  Cutoff  │  Reso    │ Src1/2   │ ±24st    │ Macro    │ Macro    │ Macro    │  Macro   │
│ 20Hz-20k │ 0→1      │ 0→1      │ bipolar  │ 0→1      │ 0→1      │ 0→1      │ 0→1      │
└──────────┴──────────┴──────────┴──────────┴──────────┴──────────┴──────────┴──────────┘

ROW 2 (8 knobs — Tier 2: Shape & Ecology)
┌──────────┬──────────┬──────────┬──────────┬──────────┬──────────┬──────────┬──────────┐
│ Amp Atk  │ Amp Dcy  │ Amp Sus  │ Amp Rel  │  Field   │ Turbidity│ Drift    │ Distance │
│  0→10s   │  0→10s   │  0→1     │  0→20s   │ Strength │  0→1     │  Depth   │  0→1     │
└──────────┴──────────┴──────────┴──────────┴──────────┴──────────┴──────────┴──────────┘
```

### Page System (Push Device Pages)

**Page A — Source + Filter (default)**
- Knobs: Src1 Type | Src1 Tune | Src2 Type | Src2 Tune | Src Mix | FM Depth | Proc1 Cut | Proc1 Reso
- Buttons below: [Proc1 Type] [Proc2 On/Off] [Proc3 On/Off] [WT Bank ←] [WT Bank →] [Journey] [FX Mode] [State Reset]

**Page B — Amp Envelope + Mod**
- Knobs: Amp A | Amp D | Amp S | Amp R | Mod1 Rate | Mod1 Depth | Mod2 Rate | Mod2 Depth
- Buttons: [Mod1 Type cycle] [Mod1 Target] [Mod2 Type] [Mod2 Target] [Mod3 Type] [Mod3 Target] [Mod4 Type] [Polyphony]

**Page C — Biophonic Environment (The Water)**
- Knobs: Field Strength | Field Polarity | Env Temp | Env Pressure | Env Current | Turbidity | Competition | Symbiosis
- Buttons: [Field Prime 3-lim] [Field Prime 5-lim] [Field Prime 7-lim] [ ] [Stress on/off] [Bleach on/off] [FX Mode Srl/Par] [Reef Resident cycle]

**Page D — FX + Spatial**
- Knobs: FX1 Mix | FX1 Param | FX2 Mix | FX2 Param | FX3 Mix | FX3 Param | Distance | Air
- Buttons: [FX1 Type cycle] [FX2 Type cycle] [FX3 Type cycle] [Drift Rate ↓] [Drift Rate ↑] [Drift Depth] [Unison Det] [Glide]

### Pad Matrix

OBRIX uses **Chromatic** mode on Push — the reef grows in all directions:
- Pads play chromatically in 4ths layout (default Push chromatic)
- Velocity-sensitive: harder hits = more stress accumulation (Wave 4 stateful synthesis)
- Aftertouch: drives Mod 4 (Aftertouch modulator) → filter cutoff or volume by default
- Pad color: Reef Jade `#1E8B7E` when active, dim teal when idle

**Gesture pads (top row, pads 57–64):**
```
[Ripple] [  Pulse  ] [  Flow  ] [  Tide  ] [  —  ] [  —  ] [Journey toggle] [State Reset]
```

---

## Maschine Mapping

### Philosophy
On Maschine, OBRIX maps beautifully to the 4-page knob system. Smart Strips handle the 4 macros — live performance gestures while playing. Pads stay chromatic, 16 pads per octave range.

### Smart Strips (always-visible)
| Strip | Parameter | Notes |
|-------|-----------|-------|
| Strip 1 | `obrix_macroCharacter` | Brick topology bias — most dramatic macro |
| Strip 2 | `obrix_macroMovement` | LFO rate multiplier + audio-rate unlock |
| Strip 3 | `obrix_macroCoupling` | Reef resident ecology depth |
| Strip 4 | `obrix_macroSpace` | Global FX wet amount |
| Strip 5 | `obrix_proc1Cutoff` | Primary filter cutoff — live sweep |
| Strip 6 | `obrix_proc1Reso` | Filter resonance |
| Strip 7 | `obrix_srcMix` | Src1/Src2 blend |
| Strip 8 | `obrix_fmDepth` | FM depth (bipolar — center = 0) |

### Knob Pages

**Page 1 — Performance (most-grabbed in a session)**
| K1 | K2 | K3 | K4 | K5 | K6 | K7 | K8 |
|----|----|----|----|----|----|----|-----|
| Proc1 Cut | Proc1 Reso | FM Depth | Src Mix | Proc1 Fb | Proc2 Cut | Proc2 Reso | Level |

**Page 2 — Envelope + Mod**
| K1 | K2 | K3 | K4 | K5 | K6 | K7 | K8 |
|----|----|----|----|----|----|----|-----|
| Amp Atk | Amp Dcy | Amp Sus | Amp Rel | Mod1 Rate | Mod1 Depth | Mod2 Rate | Mod2 Depth |

**Page 3 — Biophonic Environment (The Water)**
| K1 | K2 | K3 | K4 | K5 | K6 | K7 | K8 |
|----|----|----|----|----|----|----|-----|
| Field Str | Field Pol | Env Temp | Env Pressure | Env Current | Turbidity | Competition | Symbiosis |

**Page 4 — Spatial + Effects**
| K1 | K2 | K3 | K4 | K5 | K6 | K7 | K8 |
|----|----|----|----|----|----|----|-----|
| FX1 Mix | FX1 Param | FX2 Mix | FX2 Param | Distance | Air | Drift Depth | Drift Rate |

### Pad Layout (Chromatic — C2 root)

```
[G#] [A ] [A#] [B ]
[E ] [F ] [F#] [G ]
[C ] [C#] [D ] [D#]  ← Root row
[G#] [A ] [A#] [B ]  ← (lower octave)
```

Pads 13–16 (top-left quadrant with Shift): Gesture type selection
- Pad 13: Ripple | Pad 14: Pulse | Pad 15: Flow | Pad 16: Tide

---

## Generic MIDI CC Map

### Design Notes
- All CCs use 0–127 range
- Bipolar parameters (FM Depth, Field Polarity, Env Current) center at CC value 64 = 0.0
- Log-scaled parameters (Attack, Decay, Release, Cutoff) are marked `[log]` — host must apply curve or use NRPN
- CC 80–83 reserved for coupling bus (XOlokun-level control, see xolokun-performance-mapping.md)

| CC# | Parameter ID | Name | Range Notes |
|-----|-------------|------|-------------|
| **Tier 1 — Most Performance-Critical** | | | |
| CC01 | `obrix_macroCharacter` | CHARACTER Macro | 0=off, 127=full |
| CC02 | `obrix_macroMovement` | MOVEMENT Macro | 0=still, 127=audio-rate |
| CC03 | `obrix_macroCoupling` | COUPLING Macro | 0=isolated, 127=deep ecology |
| CC04 | `obrix_macroSpace` | SPACE Macro | 0=dry, 127=full FX |
| CC05 | `obrix_proc1Cutoff` | Proc 1 Cutoff | [log] 20Hz–20kHz |
| CC06 | `obrix_proc1Reso` | Proc 1 Resonance | 0–127 |
| CC07 | `obrix_srcMix` | Source Mix | 64=center, 0=Src1, 127=Src2 |
| CC08 | `obrix_fmDepth` | FM Depth | 64=zero, bipolar ±24st |
| **Tier 2 — Sound Shaping** | | | |
| CC09 | `obrix_proc2Cutoff` | Proc 2 Cutoff | [log] |
| CC10 | `obrix_proc2Reso` | Proc 2 Resonance | 0–127 |
| CC11 | `obrix_proc3Cutoff` | Proc 3 Cutoff | [log] |
| CC12 | `obrix_ampAttack` | Amp Attack | [log] 0–10s |
| CC13 | `obrix_ampDecay` | Amp Decay | [log] 0–10s |
| CC14 | `obrix_ampSustain` | Amp Sustain | 0–127 |
| CC15 | `obrix_ampRelease` | Amp Release | [log] 0–20s |
| CC16 | `obrix_proc1Feedback` | Proc 1 Feedback (self-osc) | 0=clean, 127=self-osc |
| CC17 | `obrix_proc2Feedback` | Proc 2 Feedback | 0=clean, 127=self-osc |
| **Tier 3 — Mod Routing** | | | |
| CC18 | `obrix_mod1Rate` | Mod 1 Rate | [log] 0.01–30Hz |
| CC19 | `obrix_mod1Depth` | Mod 1 Depth | 64=zero, bipolar |
| CC20 | `obrix_mod2Rate` | Mod 2 Rate | [log] 0.01–30Hz |
| CC21 | `obrix_mod2Depth` | Mod 2 Depth | 64=zero, bipolar |
| CC22 | `obrix_mod3Depth` | Mod 3 Depth | 64=zero, bipolar |
| CC23 | `obrix_mod4Depth` | Mod 4 Depth | 64=zero, bipolar |
| **Tier 4 — Biophonic Environment** | | | |
| CC24 | `obrix_fieldStrength` | Field Strength | 0=off, 127=full JI pull |
| CC25 | `obrix_fieldPolarity` | Field Polarity | 64=zero, 0=repulse, 127=attract |
| CC26 | `obrix_envTemp` | Temperature | 0=cold, 127=hot (drift rate) |
| CC27 | `obrix_envPressure` | Pressure | 64=neutral LFO rate |
| CC28 | `obrix_envCurrent` | Current (cutoff bias) | 64=center, bipolar ±2kHz |
| CC29 | `obrix_envTurbidity` | Turbidity (noise) | 0=clear, 127=turbid |
| CC30 | `obrix_competitionStrength` | Competition | 0=coexist, 127=war |
| CC31 | `obrix_symbiosisStrength` | Symbiosis | 0=isolated, 127=mutualism |
| **Tier 5 — Spatial + FX** | | | |
| CC70 | `obrix_fx1Mix` | FX 1 Mix | 0=dry, 127=wet |
| CC71 | `obrix_fx1Param` | FX 1 Param | context-dependent |
| CC72 | `obrix_fx2Mix` | FX 2 Mix | |
| CC73 | `obrix_fx2Param` | FX 2 Param | |
| CC74 | `obrix_fx3Mix` | FX 3 Mix | |
| CC75 | `obrix_distance` | Distance (HF rolloff) | 0=close, 127=far |
| CC76 | `obrix_air` | Air (warm↔cold tilt) | 64=neutral, bipolar |
| CC77 | `obrix_driftDepth` | Drift Depth | 0=off, 127=full ensemble drift |
| CC78 | `obrix_residentStrength` | Reef Resident Strength | 0=off, 127=dominant ecology |
| CC79 | `obrix_glideTime` | Glide Time | 0=instant, 127=max |
| **Coupling Bus (XOlokun-level)** | | | |
| CC80 | xolokun coupling A→B amount | Coupling Out | see xolokun map |
| CC81 | xolokun coupling B→A amount | Coupling In | see xolokun map |
| CC82 | xolokun coupling type | Coupling Type | 0–14 |
| CC83 | xolokun coupling bypass | Bypass toggle | 0=active, 127=bypass |

### Recommended Mod Wheel Routing
- **Default:** `obrix_mod2Depth` (LFO depth) — breathing filter motion
- **Performance:** Temporarily override to `obrix_envCurrent` for manual cutoff bias sweep

### Recommended Aftertouch Routing
- **Default:** Mod 4 target = Volume — pressure controls dynamics
- **Biophonic:** Route to `obrix_fieldStrength` — finger pressure pulls harmony toward JI

---

## Quick Reference: Ergonomic Focus Points

For a typical OBRIX session, these 8 parameters do 80% of the expressive work:
1. `obrix_proc1Cutoff` — the reef's breathing
2. `obrix_srcMix` — between the two bricks
3. `obrix_fmDepth` — metallic to warm in one gesture (bipolar center = sweet spot)
4. `obrix_macroCharacter` — changes the whole ecosystem's texture
5. `obrix_macroMovement` — from slow drift to audio-rate chaos
6. `obrix_envCurrent` — directional push/pull on the filter
7. `obrix_fieldStrength` — engage the JI attractor mid-phrase
8. `obrix_macroCoupling` — how deep the reef resident burrows
