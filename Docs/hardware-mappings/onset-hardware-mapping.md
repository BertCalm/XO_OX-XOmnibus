# ONSET Hardware Controller Mapping

**Engine:** ONSET — The Surface Splash
**Accent:** Electric Blue `#0066FF`
**Parameter Prefix:** `perc_` (internal); public prefix `onset_`
**Total Params:** 111 (8 voices × 11 params + 15 global + 6 XVC + 8 FX)
**Macros:** MACHINE (Circuit↔Algorithm blend bias) / PUNCH (snap+body aggression, AT-boosted) / SPACE (FX depth) / MUTATE (stochastic drift, MW-scaled)

---

## Engine Architecture

8 dedicated voices, permanently mapped:
| Voice | Name | MIDI Note | Circuit | Default Blend | Default Decay |
|-------|------|-----------|---------|---------------|---------------|
| V1 | Kick | 36 (C2) | BridgedT | 0.2 (mostly circuit) | 0.5 s |
| V2 | Snare | 38 (D2) | NoiseBurst | 0.5 (balanced) | 0.3 s |
| V3 | HH-C | 42 (F#2) | Metallic | 0.7 (mostly algo) | 0.05 s |
| V4 | HH-O | 46 (A#2) | Metallic | 0.7 | 0.4 s |
| V5 | Clap | 39 (D#2) | NoiseBurst | 0.4 | 0.25 s |
| V6 | Tom | 45 (A2) | BridgedT | 0.3 | 0.4 s |
| V7 | Perc A | 37 (C#2) | BridgedT | 0.6 | 0.3 s |
| V8 | Perc B | 44 (G#2) | Metallic | 0.8 (full algo) | 0.35 s |

Layer Algorithm choices per voice: FM (0) / Modal (1) / Karplus-Strong (2) / PhaseDistortion (3)

---

## Parameter Reference

### Per-Voice Parameters (× 8 voices — prefix: `perc_v{1-8}_`)
| Suffix | Name | Range | Default | Notes |
|--------|------|-------|---------|-------|
| `blend` | Circuit/Algo Blend | 0.0–1.0 | voice-specific | 0=Circuit (808), 1=Algorithm |
| `algoMode` | Algorithm Mode | 0–3 (FM/Modal/K-S/PhaseDist) | voice-specific | Layer O selection |
| `pitch` | Pitch Offset | -24 to +24 st | 0.0 | Tuning |
| `decay` | Decay | 0.01–8.0 s (log) | voice-specific | Most-tweaked per-voice param |
| `tone` | Tone | 0.0–1.0 | 0.5 | Filter brightness |
| `snap` | Snap | 0.0–1.0 | 0.3 | Transient attack spike |
| `body` | Body | 0.0–1.0 | 0.5 | Resonant body emphasis |
| `character` | Character | 0.0–1.0 | 0.0 | Harmonic distortion/grit |
| `level` | Level | 0.0–1.0 | 0.7 | Per-voice volume |
| `pan` | Pan | -1.0–1.0 | 0.0 | Stereo position |
| `envShape` | Env Shape | AD/AHD/ADSR | AD | Drum envelope type |

### Global Parameters
| ID | Name | Range | Default | Notes |
|----|------|-------|---------|-------|
| `perc_level` | Master Level | 0.0–1.0 | 0.8 | Output volume |
| `perc_drive` | Drive | 0.0–1.0 | 0.0 | Master overdrive |
| `perc_masterTone` | Master Tone | 0.0–1.0 | 0.5 | Master LP filter |
| `perc_macro_machine` | MACHINE | 0.0–1.0 | 0.5 | Blend bias all voices |
| `perc_macro_punch` | PUNCH | 0.0–1.0 | 0.5 | Snap+body bias all voices (AT-boosted ×0.3) |
| `perc_macro_space` | SPACE | 0.0–1.0 | 0.0 | Global FX wet |
| `perc_macro_mutate` | MUTATE | 0.0–1.0 | 0.0 | Stochastic drift (MW scales ×1→×2) |

### Cross-Voice Coupling (XVC) — B002 Blessing
| ID | Name | Range | Default | Effect |
|----|------|-------|---------|--------|
| `perc_xvc_global_amount` | XVC Amount | 0.0–1.0 | 0.5 | Master XVC bus level |
| `perc_xvc_kick_to_snare_filter` | Kick→Snare Filter | 0.0–1.0 | 0.15 | Kick hit brightens snare tone |
| `perc_xvc_snare_to_hat_decay` | Snare→Hat Decay | 0.0–1.0 | 0.10 | Snare tightens hi-hat decay |
| `perc_xvc_kick_to_tom_pitch` | Kick→Tom Pitch | 0.0–1.0 | 0.0 | Kick ducks tom pitch |
| `perc_xvc_snare_to_perc_blend` | Snare→Perc A Blend | 0.0–1.0 | 0.0 | Snare pushes Perc A toward algo |
| `perc_xvc_hat_choke` | Hat Choke | 0.0–1.0 | 1.0 | HH-O chokes when HH-C hits |

### Character Stage
| ID | Name | Range | Default | Notes |
|----|------|-------|---------|-------|
| `perc_char_grit` | Grit | 0.0–1.0 | 0.0 | Harmonic saturation |
| `perc_char_warmth` | Warmth | 0.0–1.0 | 0.5 | Low-end emphasis |

### FX Rack
| ID | Name | Range | Default | Notes |
|----|------|-------|---------|-------|
| `perc_fx_delay_time` | Delay Time | 0.01–1.0 s (log) | 0.3 s | |
| `perc_fx_delay_feedback` | Delay Feedback | 0.0–0.95 | 0.3 | Hard limit at 0.95 |
| `perc_fx_delay_mix` | Delay Mix | 0.0–1.0 | 0.0 | |
| `perc_fx_reverb_size` | Reverb Size | 0.0–1.0 | 0.4 | |
| `perc_fx_reverb_decay` | Reverb Decay | 0.0–1.0 | 0.3 | |
| `perc_fx_reverb_mix` | Reverb Mix | 0.0–1.0 | 0.0 | |
| `perc_fx_lofi_bits` | LoFi Bits | 4–16 bit | 16 | 16=clean, 4=crushed |
| `perc_fx_lofi_mix` | LoFi Mix | 0.0–1.0 | 0.0 | |

---

## Push 2/3 Mapping

### Philosophy
ONSET is a drum machine — Push in Drum Rack mode is the natural home. 16 pads map the 8 drum voices (plus alternates and extras). The top 8 knobs handle macros + the most critical live-tweakable per-kit params. Voice-specific editing opens on pad selection.

### Default Knob Layout (global view, no pad selected)

```
TOP ROW (8 knobs — Global / Macro layer)
┌──────────┬──────────┬──────────┬──────────┬──────────┬──────────┬──────────┬──────────┐
│  MACHINE │  PUNCH   │  SPACE   │  MUTATE  │  Master  │  Drive   │  Grit    │  Warmth  │
│ 0→1      │ 0→1      │ 0→1      │ 0→1      │  Level   │  0→1     │  0→1     │  0→1     │
│ blend    │ snap+bod │ FX wet   │ stochast │  0→1     │          │  char    │  char    │
└──────────┴──────────┴──────────┴──────────┴──────────┴──────────┴──────────┴──────────┘

ROW 2 (8 knobs — XVC Rhythm Brain)
┌──────────┬──────────┬──────────┬──────────┬──────────┬──────────┬──────────┬──────────┐
│   XVC    │ K>Sn Flt │ Sn>Ht Dy │ K>Tm Pch │ Sn>PcBld │ Hat Choke│ Rev Mix  │ Dly Mix  │
│  Amount  │  0→1     │  0→1     │  0→1     │  0→1     │  0→1     │  0→1     │  0→1     │
└──────────┴──────────┴──────────┴──────────┴──────────┴──────────┴──────────┴──────────┘
```

### Per-Voice Editing (pad selected — voice knobs)

When a drum pad is selected, top 8 knobs switch to:
```
┌──────────┬──────────┬──────────┬──────────┬──────────┬──────────┬──────────┬──────────┐
│  Blend   │  Decay   │  Tone    │  Snap    │  Body    │Character │  Level   │   Pan    │
│ Cir↔Alg  │ 0.01-8s  │  0→1     │  0→1     │  0→1     │  0→1     │  0→1     │ -1→+1    │
└──────────┴──────────┴──────────┴──────────┴──────────┴──────────┴──────────┴──────────┘
```
Row 2 (second voice-level parameters):
```
┌──────────┬──────────┬──────────┬──────────┬──────────┬──────────┬──────────┬──────────┐
│  Pitch   │ Algo Mode│ Env Shape│  —       │  —       │  —       │  —       │ [Back]   │
│ -24→+24  │ FM/Mod/  │ AD/AHD/  │          │          │          │          │  global  │
│  st      │ KS/PhasD │ ADSR     │          │          │          │          │          │
└──────────┴──────────┴──────────┴──────────┴──────────┴──────────┴──────────┴──────────┘
```

### Drum Pad Matrix (Push 2/3 Drum Mode, 4×4)

```
Row 4 (top)   [Perc A ] [Perc B ] [  —    ] [  —    ]
Row 3         [  Tom  ] [  Clap ] [HH-Choke] [  —   ]
Row 2         [ HH-C  ] [  HH-O ] [ Sn+Rim] [  —    ]
Row 1 (bottom)[  Kick ] [  Kick2] [  Snare] [  Tom2 ]
```

Pad colors (Reef Jade → Electric Blue when active):
- Kick: deep blue, full brightness on hit
- Snare: mid blue, flashes brighter on velocity
- Hi-hats: lighter blue, fast decay visual
- Clap: cyan flash
- Tom/Perc: varied blue shades by voice index

**MIDI notes (GM channel 10):**
- Kick: 36 | Snare: 38 | HH-C: 42 | HH-O: 46 | Clap: 39 | Tom: 45 | Perc A: 37 | Perc B: 44

### Push FX Page
Navigate to FX page for delay/reverb/lofi controls:
```
┌──────────┬──────────┬──────────┬──────────┬──────────┬──────────┬──────────┬──────────┐
│ Dly Time │  Dly FB  │  Dly Mix │ Rev Size │ Rev Dcay │  Rev Mix │ LoFi Bit │ LoFi Mix │
└──────────┴──────────┴──────────┴──────────┴──────────┴──────────┴──────────┴──────────┘
```

---

## Maschine Mapping

### Philosophy
Maschine is native territory for ONSET. 16 pads = drum kit. Use Groups (A–H) for 8 voices, one per group. Smart Strips control macros and XVC globally. Pad pages per group access voice-specific parameters.

### Smart Strips (always-visible, kit-level)
| Strip | Parameter | Ergonomic Note |
|-------|-----------|----------------|
| Strip 1 | `perc_macro_machine` | MACHINE — whole-kit character |
| Strip 2 | `perc_macro_punch` | PUNCH — aggression; aftertouch also drives this |
| Strip 3 | `perc_macro_space` | SPACE — reverb/delay send |
| Strip 4 | `perc_macro_mutate` | MUTATE — introduce randomness live |
| Strip 5 | `perc_xvc_global_amount` | XVC Amount — rhythm brain depth |
| Strip 6 | `perc_char_grit` | Grit — overdrive texture |
| Strip 7 | `perc_level` | Master output |
| Strip 8 | `perc_masterTone` | Master tone LP |

### Knob Pages (Kit-Level)

**Page 1 — Kit Performance**
| K1 | K2 | K3 | K4 | K5 | K6 | K7 | K8 |
|----|----|----|----|----|----|----|-----|
| MACHINE | PUNCH | SPACE | MUTATE | Master Level | Drive | Grit | Warmth |

**Page 2 — XVC Rhythm Brain**
| K1 | K2 | K3 | K4 | K5 | K6 | K7 | K8 |
|----|----|----|----|----|----|----|-----|
| XVC Amount | K→Sn Flt | Sn→Ht Dcy | K→Tm Pch | Sn→Pc Bld | Hat Choke | — | — |

**Page 3 — Delay + Reverb**
| K1 | K2 | K3 | K4 | K5 | K6 | K7 | K8 |
|----|----|----|----|----|----|----|-----|
| Dly Time | Dly FB | Dly Mix | Rev Size | Rev Decay | Rev Mix | LoFi Bits | LoFi Mix |

**Page 4 — Voice Editor (active Group/pad)**
Accessed per-voice via Group selection:
| K1 | K2 | K3 | K4 | K5 | K6 | K7 | K8 |
|----|----|----|----|----|----|----|-----|
| Blend | Decay | Tone | Snap | Body | Character | Level | Pan |

### Pad Layout (Standard 4×4, Group = 1 voice per pad)

Each Group holds one drum voice. Within a Group, 16 pads = 16 pitches (chromatic triggers).

Kit overview (Groups A–H):
- Group A: Kick (blue, pad 1 = MIDI 36)
- Group B: Snare (lighter blue, pad 1 = MIDI 38)
- Group C: HH-C (cyan, pad 1 = MIDI 42)
- Group D: HH-O (teal, pad 1 = MIDI 46)
- Group E: Clap (white-blue, pad 1 = MIDI 39)
- Group F: Tom (mid blue, pad 1 = MIDI 45)
- Group G: Perc A (lavender-blue, pad 1 = MIDI 37)
- Group H: Perc B (deep blue, pad 1 = MIDI 44)

**Tip:** Use Maschine's Lock feature to freeze XVC settings between patterns while live-tweaking macros.

---

## Generic MIDI CC Map

### Design Notes
- ONSET has 111 parameters — only the most performance-critical are listed (voice-specific params would need NRPN in a full implementation)
- Per-voice editing via CC requires multiplexing: consider using Scenes or Program Change to select active voice
- Macros (CC01–04) are always global — touch them any time without mode switching

| CC# | Parameter ID | Name | Range Notes |
|-----|-------------|------|-------------|
| **Macros (always active)** | | | |
| CC01 | `perc_macro_machine` | MACHINE | 0=circuit, 127=algorithm |
| CC02 | `perc_macro_punch` | PUNCH | 0=soft, 127=aggressive (AT adds up to 38%) |
| CC03 | `perc_macro_space` | SPACE | 0=dry, 127=full FX send |
| CC04 | `perc_macro_mutate` | MUTATE | 0=static, 127=chaotic (MW scales ×2) |
| **Kit Character** | | | |
| CC05 | `perc_level` | Master Level | 0–127 |
| CC06 | `perc_drive` | Drive | 0=clean, 127=driven |
| CC07 | `perc_masterTone` | Master Tone | 0=dark, 127=bright |
| CC08 | `perc_char_grit` | Grit | 0=clean, 127=saturated |
| CC09 | `perc_char_warmth` | Warmth | 0=neutral, 127=warm |
| **XVC Rhythm Brain** | | | |
| CC10 | `perc_xvc_global_amount` | XVC Global Amount | 0=off, 127=max coupling |
| CC11 | `perc_xvc_kick_to_snare_filter` | Kick→Snare Filter | 0–127 |
| CC12 | `perc_xvc_snare_to_hat_decay` | Snare→Hat Decay | 0–127 |
| CC13 | `perc_xvc_kick_to_tom_pitch` | Kick→Tom Pitch | 0–127 |
| CC14 | `perc_xvc_snare_to_perc_blend` | Snare→Perc Blend | 0–127 |
| CC15 | `perc_xvc_hat_choke` | Hat Choke Amount | 0=free, 127=hard choke |
| **FX Rack** | | | |
| CC16 | `perc_fx_delay_time` | Delay Time | [log] 0.01–1.0s |
| CC17 | `perc_fx_delay_feedback` | Delay Feedback | 0=single, 127=near-infinite |
| CC18 | `perc_fx_delay_mix` | Delay Mix | 0=dry, 127=wet |
| CC19 | `perc_fx_reverb_size` | Reverb Size | 0=small, 127=large |
| CC20 | `perc_fx_reverb_decay` | Reverb Decay | 0=fast, 127=slow |
| CC21 | `perc_fx_reverb_mix` | Reverb Mix | 0=dry, 127=wet |
| CC22 | `perc_fx_lofi_bits` | LoFi Bit Depth | 0=4bit (crushed), 127=16bit (clean) |
| CC23 | `perc_fx_lofi_mix` | LoFi Mix | 0=dry, 127=full lo-fi |
| **Per-Voice (active voice, use with mode switch)** | | | |
| CC24 | `perc_v{n}_decay` | Active Voice Decay | [log] 10ms–8s |
| CC25 | `perc_v{n}_blend` | Active Voice Blend | 0=circuit, 127=algorithm |
| CC26 | `perc_v{n}_tone` | Active Voice Tone | 0=dark, 127=bright |
| CC27 | `perc_v{n}_snap` | Active Voice Snap | 0=soft, 127=crisp transient |
| CC28 | `perc_v{n}_body` | Active Voice Body | 0=hollow, 127=resonant |
| CC29 | `perc_v{n}_pitch` | Active Voice Pitch | 64=center, ±24st bipolar |
| CC30 | `perc_v{n}_level` | Active Voice Level | 0–127 |
| CC31 | `perc_v{n}_pan` | Active Voice Pan | 64=center, bipolar |
| **Coupling Bus** | | | |
| CC80 | xolokun coupling A→B | Coupling Out | see xolokun map |
| CC81 | xolokun coupling B→A | Coupling In | see xolokun map |
| CC82 | xolokun coupling type | Coupling Type | 0–14 |
| CC83 | xolokun coupling bypass | Bypass | 0=active, 127=bypass |

### Recommended Mod Wheel Routing
- **Default:** `perc_macro_mutate` × 2 depth (mod wheel doubles MUTATE range)
- This is hard-coded in the engine — MW automatically scales MUTATE at render time

### Recommended Aftertouch Routing
- **Default:** PUNCH macro boost (+0.3 max) — pressing harder on pads makes the whole kit hit harder
- This is hard-coded in the engine — channel aftertouch automatically boosts PUNCH

---

## Performance Techniques

### The "Rhythm Brain" Performance Approach
1. Start with XVC Amount = 0.5 (default). Play kick + snare patterns normally.
2. Slowly raise `perc_xvc_kick_to_snare_filter` (CC11) — the kick will start brightening the snare on every hit.
3. Raise `perc_xvc_snare_to_hat_decay` (CC12) — the hi-hat tightens up on snare hits.
4. With MUTATE > 0 and Mod Wheel up, the kit begins to evolve on its own.

### Machine vs. Algorithm Live Sweep
- MACHINE macro at 0.0: 808-style analog warmth on all 8 voices simultaneously
- MACHINE macro at 1.0: Full FM/Modal/Karplus-Strong algorithmic character
- Slow MACHINE sweep during a breakdown: sounds like the beat melts and reforms
- Best on Push: assign MACHINE to a touch strip for smooth, hands-free automation
