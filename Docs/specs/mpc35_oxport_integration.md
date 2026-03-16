# MPC 3.5 Features — Oxport Integration Opportunities

**Date**: 2026-03-16
**Status**: R&D — integration roadmap for Oxport pipeline

---

## Overview

MPC 3.5 introduced several structural changes to program format, pad behavior, and insert effect routing. This document maps each new feature to Oxport's XPM generation pipeline and identifies concrete integration work required.

---

## 1. VST3 Plugin Programs

MPC 3.5 added a new program type: VST3 Plugin Program. This allows a VST3 plugin to be embedded as a program in a project, with its state saved alongside keygroup and drum programs.

### What This Means for XOmnibus

XOmnibus as a VST3 can theoretically become a Program type in an MPC 3.5 project. A user loads XOmnibus as a VST3 plugin program, selects a preset (e.g., OPAL "Cephalopod Drift"), and that program slot behaves identically to any other MPC program: it receives MIDI from pads, responds to Q-Links, exports via Stems.

### Speculative XPM Schema for VST3 Program

Based on MPC's XML conventions and the VST3 program type additions documented in the 3.5 release notes:

```xml
<?xml version="1.0" encoding="UTF-8"?>
<MPCVObject>
  <Version value="2.10"/>
  <Program type="VST3Plugin" name="XOmnibus - OPAL Cephalopod Drift">
    <PluginName value="XOmnibus"/>
    <PluginVendor value="XO_OX Designs"/>
    <PluginUID value="XOOX-XOMN-0001-0000"/>
    <PluginState>
      <!-- Base64-encoded VST3 chunk containing engine selection + preset parameters -->
      <StateData encoding="base64">
        [BASE64_ENCODED_PRESET_CHUNK]
      </StateData>
    </PluginState>
    <PadNoteMap>
      <Entry pad="0" note="36"/>
      <Entry pad="1" note="37"/>
      <!-- ... 64 pads ... -->
    </PadNoteMap>
    <Volume value="100"/>
    <Pan value="0"/>
  </Program>
</MPCVObject>
```

### Oxport Integration Work Required

1. **VST3 state serialization**: Oxport needs a pathway to read an XOmnibus `.xometa` preset, serialize it as a VST3 state chunk (base64), and embed it in the VST3 program XPM.
2. **PluginUID registry**: XO_OX needs a registered VST3 UID that MPC 3.5 can look up. Current plugin codes (PLUGIN_CODE varies per engine) need consolidation to a single XOmnibus UID.
3. **Fallback behavior**: If MPC version < 3.5 or XOmnibus VST3 not installed, Oxport should fall back to a rendered keygroup program with equivalent samples. Document this fallback in the XPN expansion README.

**Priority**: Medium. VST3 program type enables the richest playback experience but requires XOmnibus to be installed on the target machine. Rendered keygroup programs remain the universal path.

---

## 2. Keygroup Synth Engine

MPC 3.5 introduced a native Keygroup Synth Engine program type: a wavetable + sample hybrid synthesis mode built into the MPC firmware itself. No plugin required.

### Architecture

The Keygroup Synth Engine combines:
- Up to 4 oscillators per voice (wavetable or sample)
- Per-oscillator pitch, level, and wavetable position
- Shared filter (low-pass, high-pass, band-pass options) with envelope
- Amplitude envelope per layer
- LFO routing to pitch, filter, and amplitude

### Embedded Wavetable Opportunity for XO_OX

Yes, XO_OX can ship embedded wavetables inside XPN expansions. The wavetable format is a single-cycle WAV (2048 samples at 44100Hz) or a sequential wavetable file (multiple single-cycle frames concatenated, with frame count specified in XPM).

**Recommended workflow:**
1. Render representative single-cycle waveforms from XOmnibus engines (OVERWORLD era snapshots, OPAL grain freezes, OUROBOROS chaos attractors as pitch).
2. Package these as `Wavetables/` folder inside the XPN bundle.
3. Reference them in a Keygroup Synth Engine program XPM.

**Speculative wavetable XPM reference:**
```xml
<Layer num="1">
  <SampleName value="xo_ox_overworld_nes_single.wav"/>
  <WavetableMode value="True"/>
  <WavetableFrameCount value="64"/>
  <WavetablePosition value="0"/>
  <VelStart value="0"/>
  <VelEnd value="127"/>
  <RootNote value="60"/>
  <KeyTrack value="True"/>
</Layer>
```

**Oxport work required**: Add wavetable export mode to `fleet_render_automation.py` — render single-cycle snapshots from each engine at key wavetable positions, output to `Wavetables/` subfolder in XPN.

---

## 3. 4-Slot Insert Effects

MPC 3.5 expanded insert effect slots from 2 to 4 per program. The `<InsertFX>` XML block now supports up to 4 sequential effect processors.

### XML Schema

```xml
<InsertFX>
  <InsertFXPlugin num="1">
    <PluginName value="AIR Distortion"/>
    <Preset value="Tube Warmth"/>
    <Enabled value="True"/>
    <Wet value="75"/>
  </InsertFXPlugin>
  <InsertFXPlugin num="2">
    <PluginName value="AIR Spring Reverb"/>
    <Preset value="Room Tail"/>
    <Enabled value="True"/>
    <Wet value="30"/>
  </InsertFXPlugin>
  <InsertFXPlugin num="3">
    <PluginName value="AIR Chorus"/>
    <Preset value="Subtle Width"/>
    <Enabled value="True"/>
    <Wet value="40"/>
  </InsertFXPlugin>
  <InsertFXPlugin num="4">
    <PluginName value="AIR EQ"/>
    <Preset value="High Shelf Air"/>
    <Enabled value="False"/>
    <Wet value="100"/>
  </InsertFXPlugin>
</InsertFX>
```

### AIR Effects to XO_OX Engine Character Mapping

The following AIR effects approximate XO_OX engine processing characters and are recommended for export pairing:

| AIR Effect | Maps to XO_OX Engine | Character Match |
|---|---|---|
| AIR Distortion | **OXIDE** | Tube/transistor saturation, even/odd harmonic character |
| AIR Spring Reverb | **OVERDUB** | Spring reverb with metallic tail, send return blend |
| AIR Chorus | **OCEANIC** | Fluid phase movement, spatial spreading, temperature |
| AIR Flanger | **ORBITAL** | Modulated comb filtering, resonant sweep |
| AIR Delay | **OVERDUB** | Tape delay echo, flutter, degradation |
| AIR Reverb | **OPAL** | Granular reverb approximation, space, grain decay |
| AIR Stereo Width | **OBLIQUE** | Prismatic stereo field, mid-side manipulation |
| AIR Frequency Shifter | **ORIGAMI** | Fold distortion approximation via pitch/harmonic shift |
| AIR Compressor | **OBESE** | Dynamics shaping, parallel compression workflow |
| AIR Lo-Fi | **OVERWORLD** | Era degradation: NES/Genesis/SNES bit reduction, sample rate |

### Oxport Integration

Oxport's program generator should optionally populate `<InsertFX>` based on the source engine. When building a keygroup program from OVERDUB presets, Oxport adds AIR Spring Reverb at insert 1 as a default. When building from OXIDE presets (once OXIDE ships), Oxport adds AIR Distortion at insert 1 with matching drive character.

Flag in `expansion.json`: `"insert_fx_mode": "engine_matched"` (default off, opt-in per expansion).

---

## 4. PadColor

MPC 3.5 confirmed PadColor support at the program level — individual pads can carry accent color assignments that display on the MPC hardware's RGB pad surface.

### XML Encoding

PadColor in MPC 3.5 uses **hex string format** (6-digit RGB, no alpha):

```xml
<Pad num="0" note="36" color="#D4500A"/>
<Pad num="1" note="37" color="#A78BFA"/>
<Pad num="2" note="38" color="#00A6D6"/>
<Pad num="3" note="39" color="#E8839B"/>
```

No decimal encoding. No leading `0x`. The `#` prefix is required. Values outside the 000000–FFFFFF range are clamped. Alpha channel is not supported at the pad level in 3.5.

### Oxport Color Generation from Engine Accent Palette

Oxport should generate pad colors from the 34-engine accent palette when building multi-engine XPN expansions. The mapping rule: **each pad's color corresponds to the source engine that generated that pad's sample**.

**Full accent color mapping table (all 34 registered engines):**

| Engine | Accent Color | Hex |
|---|---|---|
| ODDFELIX | Neon Tetra Blue | `#00A6D6` |
| ODDOSCAR | Axolotl Gill Pink | `#E8839B` |
| OVERDUB | Olive | `#6B7B3A` |
| ODYSSEY | Violet | `#7B2D8B` |
| OBLONG | Amber | `#E9A84A` |
| OBESE | Hot Pink | `#FF1493` |
| ONSET | Electric Blue | `#0066FF` |
| OVERWORLD | Neon Green | `#39FF14` |
| OPAL | Lavender | `#A78BFA` |
| ORGANON | Bioluminescent Cyan | `#00CED1` |
| OUROBOROS | Strange Attractor Red | `#FF2D2D` |
| OBSIDIAN | Crystal White | `#E8E0D8` |
| ORIGAMI | Vermillion Fold | `#E63946` |
| ORACLE | Prophecy Indigo | `#4B0082` |
| OBSCURA | Daguerreotype Silver | `#8A9BA8` |
| OCEANIC | Phosphorescent Teal | `#00B4A0` |
| OCELOT | Ocelot Tawny | `#C5832B` |
| OVERBITE | Fang White | `#F0EDE8` |
| ORBITAL | Warm Red | `#FF6B6B` |
| OPTIC | Phosphor Green | `#00FF41` |
| OBLIQUE | Prism Violet | `#BF40FF` |
| OSPREY | Azulejo Blue | `#1B4F8A` |
| OSTERIA | Porto Wine | `#722F37` |
| OWLFISH | Abyssal Gold | `#B8860B` |
| OHM | Sage | `#87AE73` |
| ORPHICA | Siren Seafoam | `#7FDBCA` |
| OBBLIGATO | Rascal Coral | `#FF8A7A` |
| OTTONI | Patina | `#5B8A72` |
| OLE | Hibiscus | `#C9377A` |
| OMBRE | Shadow Mauve | `#7B6B8A` |
| ORCA | Deep Ocean | `#1B2838` |
| OCTOPUS | Chromatophore Magenta | `#E040FB` |
| OVERLAP | Bioluminescent Green | `#00FFB4` |
| OUTWIT | Copper | `#CC6600` |

**V1 concept engines (not yet registered):**

| Engine | Accent Color | Hex |
|---|---|---|
| OXIDE | Forge Orange | `#D4500A` |
| OSTINATO | Firelight Orange | `#E8701A` |
| OPENSKY | Sunburst | `#FF8C00` |
| OCEANDEEP | Trench Violet | `#2D0A4E` |
| OUIE | Hammerhead Steel | `#708090` |

**Pad color generation logic in Oxport:**

```python
def pad_color_for_engine(engine_name: str) -> str:
    ENGINE_COLORS = {
        "OXIDE": "#D4500A",
        "OPAL": "#A78BFA",
        "OVERDUB": "#6B7B3A",
        # ... full table
    }
    return ENGINE_COLORS.get(engine_name.upper(), "#888888")
```

For single-engine expansions, all 16 pads on bank A carry the engine accent color. For multi-engine expansions, pads are colored per source engine.

---

## 5. BendRange

MPC 3.5 exposes `<BendRange>` as a per-program XML field, allowing programs to specify pitch bend range in semitones. This is set by the expansion author, not by the end user, and persists across saves.

### XO_OX Standard

The XO_OX pitch bend doctrine aligns with feliX/Oscar polarity:

| Polarity | BendRange | Rationale |
|---|---|---|
| **feliX** | 2 semitones | Subtle, expressive. Keyboard technique-friendly. String vibrato range. |
| **Oscar** | 7 semitones | Aggressive. Dub sirens, hip-hop slides, bass drops. A fifth. |
| **Hybrid** | 4 semitones | Balanced. Standard synthesizer convention. Works for most contexts. |

### XML Field

```xml
<BendRange value="2"/>
```

### Oxport Implementation

Oxport reads engine polarity from the source preset's DNA (feliX/Oscar axis) and assigns BendRange automatically:

```python
def bend_range_for_preset(preset: dict) -> int:
    polarity = preset.get("polarity", "hybrid")
    return {"felix": 2, "oscar": 7, "hybrid": 4}.get(polarity, 4)
```

For ONSET drum programs: BendRange=2 (drums rarely bend; small range prevents accidental pitch shift).
For OVERDUB programs: BendRange=7 (dub tradition; wide bend for siren and slide effects).
For OPAL programs: BendRange=4 (granular sources benefit from expressive bend without excess).

---

## 6. Stems Integration

MPC 3.5 Stems uses zplane's élastique stem separation to split audio into up to 5 channels: Vocals, Bass, Drums, Melody, Other. This creates a workflow opportunity for XO_OX keygroup programs.

### Recommended Workflow: XO_OX Keygroup + Stems

**Track layout for a 6-track XO_OX stems session:**

| Track | Type | Content | Q-Links |
|---|---|---|---|
| Track 1 | XO_OX Keygroup | Primary engine performance (e.g., OPAL pads) | Q1–Q4: ENGINE macros |
| Track 2 | Stems – Vocals | Separated vocal from reference track | Q5: Vocals level |
| Track 3 | Stems – Bass | Separated bass | Q6: Bass level |
| Track 4 | Stems – Drums | Separated drums | Q7: Drums level |
| Track 5 | Stems – Melody | Separated melody/lead | Q8: Melody level |
| Track 6 | Stems – Other | Harmonic content, pads, misc | Q9: Other level |

**Q-Link split control**: Q-Links 1–4 assigned to XO_OX engine macros (CHARACTER, MOVEMENT, COUPLING, SPACE). Q-Links 5–8 assigned to Stems track levels. This gives the performer direct control over both the synthesis character and the reference track balance from a single 8-knob surface.

**Practical application**: Load a full-mix reference track on Track 6 as Stems input. Use the separated stems to understand the space the XO_OX pack needs to fill. Program the XO_OX keygroup to complement — not compete with — the stems in the frequency and harmonic space.

---

## 7. XPN Packaging Changes: 3.5 vs 3.x

### expansion.json Changes

MPC 3.5 introduced versioned expansion.json format. Key differences:

**MPC 3.x expansion.json:**
```json
{
  "name": "XO_OX OPAL Series",
  "version": "1.0",
  "author": "XO_OX Designs",
  "programs": ["Programs/opal_drift.xpm"],
  "samples": ["Samples/opal_drift/"],
  "previewAudio": "preview.wav"
}
```

**MPC 3.5 expansion.json additions:**

```json
{
  "name": "XO_OX OPAL Series",
  "version": "2.0",
  "format_version": "3.5",
  "author": "XO_OX Designs",
  "programs": ["Programs/opal_drift.xpm"],
  "samples": ["Samples/opal_drift/"],
  "wavetables": ["Wavetables/"],
  "previewAudio": "preview.wav",
  "padColors": true,
  "bendRanges": true,
  "stemsSuggested": false,
  "insertFX": {
    "mode": "engine_matched",
    "fallback": "none"
  },
  "mpcMinVersion": "3.5.0"
}
```

**New fields in 3.5:**
- `format_version`: Explicit MPC version target. Older MPC versions will display a compatibility warning.
- `wavetables`: Path to embedded wavetable folder (new in 3.5).
- `padColors`: Boolean flag signaling that pad colors are specified in XPM files.
- `bendRanges`: Boolean flag signaling per-program BendRange values are set.
- `stemsSuggested`: Whether the expansion ships a stems workflow guide.
- `insertFX`: Insert effect mode and fallback behavior specification.
- `mpcMinVersion`: Minimum MPC firmware version required. Oxport defaults to `"3.0.0"` for broad compatibility unless wavetables or VST3 programs are included.

### Oxport Versioning

Oxport should generate expansion.json with `format_version` set to the highest MPC version features used in the pack:
- Any pack using padColors, bendRanges only → `"3.5"`
- Any pack using wavetables → `"3.5"`
- Any pack using VST3 programs → `"3.5"` + `mpcMinVersion: "3.5.0"`
- Packs with no 3.5 features → `"3.0"` for maximum compatibility

Default for all new XO_OX packs: `"3.5"` with `mpcMinVersion: "3.5.0"`, given the target user base is current hardware.
