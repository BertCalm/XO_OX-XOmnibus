# MPC 3.5 Features — Oxport Integration Opportunities
**Authors:** Hex + Rex (XO_OX hacker/bridge androids)
**Date:** 2026-03-16
**Status:** R&D — integration roadmap for Oxport pipeline

MPC Software 3.5 Desktop Beta (May 2025) added seven capabilities that directly affect how XO_OX expansion packs can be designed, built, and delivered. This document specifies the XPM/XPN schema changes, the Oxport tool that handles each feature, and the producer-facing workflow for each one.

The goal is to update the Oxport pipeline so that packs shipped post-3.5 exploit every structural advantage the new firmware offers — not just basic sample delivery.

---

## Status Markers

| Marker | Meaning |
|--------|---------|
| **[CONFIRMED]** | Verified from Akai official communications, release notes, or directly observed in MPC behavior |
| **[PLAUSIBLE]** | Logical inference from known schema patterns; likely correct but untested by us |
| **[SPECULATIVE]** | Educated guess; treat as hypothesis until hardware test confirms |
| **[PROPOSED]** | XO_OX spec for something we intend to build; no prior art in existing files |

---

## Feature 1: VST3 Plugin Programs

### What Changed

MPC 3.5 added VST3 plugin support for MPC Desktop — the first time a non-AIR, non-VST2 plugin can run as a first-class program type inside an MPC project. **[CONFIRMED]**

For XO_OX this means XOmnibus — which already ships as a VST3 — can theoretically appear inside an MPC project not as a bounced sample set but as a live synthesis engine driving an XPM program directly.

### XPM Schema for a VST3 Plugin Program

**[PLAUSIBLE]** The VST3 program type uses `Program Type="Plugin"` at the top-level element, matching the existing `Plugin` type designation Akai uses for AIR instruments in earlier firmware. The difference is that the `<PluginFormat>` field now accepts `VST3` as a value alongside `AIR`.

```xml
<?xml version="1.0" encoding="UTF-8"?>
<MPCVObject>
  <Version Value="2.1" />

  <!-- [PLAUSIBLE] Plugin type — same Type as AIR programs; PluginFormat distinguishes -->
  <Program Type="Plugin">

    <Name Value="XOmnibus_Opal_Crystal_Drift" />
    <Author Value="XO_OX Designs" />
    <Tempo Value="120.0" />
    <BendRange Value="4" />

    <!-- [PLAUSIBLE] Plugin identity block -->
    <Plugin>
      <PluginName Value="XOmnibus" />
      <PluginVendor Value="XO_OX Designs" />
      <!-- [CONFIRMED] VST3 is a valid PluginFormat value in 3.5 -->
      <PluginFormat Value="VST3" />
      <!-- VST3 UID — 16-byte hex, matches JUCE UID registered in Projucer/CMakeLists -->
      <PluginUID Value="58 4F 4D 42 55 53 00 00 00 00 00 00 00 00 00 00" />
      <!-- Preset name — XOmnibus loads this via PresetManager on program restore -->
      <PresetName Value="Opal_Crystal_Drift" />
      <!-- Base64-encoded VST3 state chunk — fallback when preset name lookup fails -->
      <PluginState Encoding="base64" Value="" />
    </Plugin>

    <!-- Q-Link assignments map MPC Q-Links to XOmnibus VST3 parameter indices -->
    <!-- JUCE assigns parameter indices sequentially at APVTS registration order -->
    <QLinkAssignments>
      <QLinkAssignment Index="0">
        <Label Value="CHARACTER" />
        <ParameterIndex Value="0" />
        <RangeMin Value="0.0" />
        <RangeMax Value="1.0" />
      </QLinkAssignment>
      <QLinkAssignment Index="1">
        <Label Value="MOVEMENT" />
        <ParameterIndex Value="1" />
        <RangeMin Value="0.0" />
        <RangeMax Value="1.0" />
      </QLinkAssignment>
      <QLinkAssignment Index="2">
        <Label Value="COUPLING" />
        <ParameterIndex Value="2" />
        <RangeMin Value="0.0" />
        <RangeMax Value="1.0" />
      </QLinkAssignment>
      <QLinkAssignment Index="3">
        <Label Value="SPACE" />
        <ParameterIndex Value="3" />
        <RangeMin Value="0.0" />
        <RangeMax Value="1.0" />
      </QLinkAssignment>
    </QLinkAssignments>

    <!-- Pad note map — 16 pads trigger MIDI notes into XOmnibus -->
    <PadNoteMap>
      <Pad Index="0" Note="36" />   <!-- C2  Pad 1  -->
      <Pad Index="1" Note="37" />   <!-- C#2 Pad 2  -->
      <Pad Index="2" Note="38" />   <!-- D2  Pad 3  -->
      <Pad Index="3" Note="39" />   <!-- D#2 Pad 4  -->
      <Pad Index="4" Note="40" />   <!-- E2  Pad 5  -->
      <Pad Index="5" Note="41" />   <!-- F2  Pad 6  -->
      <Pad Index="6" Note="42" />   <!-- F#2 Pad 7  -->
      <Pad Index="7" Note="43" />   <!-- G2  Pad 8  -->
      <Pad Index="8" Note="44" />   <!-- G#2 Pad 9  -->
      <Pad Index="9" Note="45" />   <!-- A2  Pad 10 -->
      <Pad Index="10" Note="46" />  <!-- A#2 Pad 11 -->
      <Pad Index="11" Note="47" />  <!-- B2  Pad 12 -->
      <Pad Index="12" Note="48" />  <!-- C3  Pad 13 -->
      <Pad Index="13" Note="49" />  <!-- C#3 Pad 14 -->
      <Pad Index="14" Note="50" />  <!-- D3  Pad 15 -->
      <Pad Index="15" Note="51" />  <!-- D#3 Pad 16 -->
    </PadNoteMap>

  </Program>
</MPCVObject>
```

### Constraint: Desktop Only

**[CONFIRMED]** VST3 support in 3.5 is MPC Desktop only — standalone hardware cannot host third-party VST3 plugins as of this writing. Hardware VST3 support is on Akai's roadmap but has no confirmed shipping date. XPN packs containing VST3 plugin programs will fail to load on standalone MPC hardware.

**Implication for Oxport:** VST3 plugin program XPMs must be shipped in a separate `Plugin_Programs/` subfolder inside the XPN archive — not mixed with standard keygroup/drum programs. `xpn_packager.py` needs a `--plugin-programs` flag to handle this routing.

### Oxport Tool

A new `xpn_plugin_program_builder.py` tool would generate the VST3 XPM given an engine name, preset name, and Q-Link macro assignment. It would read the XOmnibus parameter index table (from APVTS registration order in CMakeLists.txt) to populate `<ParameterIndex>` values correctly.

**Estimated build time:** 2 days (1 day schema, 1 day APVTS parameter index extraction).

---

## Feature 2: Keygroup Synth Engine

### What Changed

MPC 3.5 introduced a new program type that embeds a synthesis engine inside a keygroup program. Unlike a standard plugin program (which hosts an arbitrary VST3), the Keygroup Synth Engine is Akai's own wavetable+sample hybrid engine — think of it as the MPC's native "mini-synth" that can run without an external plugin. **[CONFIRMED]**

### Pack Design Implications

The Keygroup Synth Engine opens a new pack category: **embedded wavetable programs** that ship XO_OX wavetable content directly into the MPC's native synth engine rather than as pre-rendered sample sets.

**[PLAUSIBLE]** The Keygroup Synth Engine accepts:
- A set of wavetable files (single-cycle waveforms, presumably WAV format)
- An oscillator configuration (wavetable position, blend mode)
- A filter + envelope configuration embedded in the XPM
- Standard keygroup zone mapping for pitch range and velocity

This means XO_OX can export single-cycle waveforms from XOmnibus engines (particularly OVERWORLD, OBLONG, ODYSSEY) and ship them as native MPC Keygroup Synth programs — no external plugin required, runs standalone.

### Proposed XPM Structure — Keygroup Synth Engine

**[PROPOSED]** Based on standard keygroup extension:

```xml
<?xml version="1.0" encoding="UTF-8"?>
<MPCVObject>
  <Version Value="2.1" />

  <!-- [PLAUSIBLE] New Type value for native synth engine programs -->
  <Program Type="KeygroupSynth">

    <Name Value="XOverworld_NES_Era_NATIVE" />
    <Author Value="XO_OX Designs" />
    <BendRange Value="2" />

    <!-- [PROPOSED] Native synth engine configuration -->
    <SynthEngine>
      <!-- Wavetable files — single-cycle WAVs embedded in Samples/ folder -->
      <WavetableA Ref="Samples/XOverworld_NES_Saw.wav" />
      <WavetableB Ref="Samples/XOverworld_NES_Pulse.wav" />
      <!-- Oscillator blend — 0.0 = pure A, 1.0 = pure B -->
      <WavetableBlend Value="0.5" />
      <!-- Position within wavetable frame (0–127) -->
      <WavetablePosition Value="64" />
    </SynthEngine>

    <!-- Filter and envelope (same schema as standard keygroup) -->
    <Filter>
      <Type Value="LowPass" />
      <Cutoff Value="80" />
      <Resonance Value="25" />
      <EnvAmount Value="40" />
    </Filter>

    <AmpEnvelope>
      <Attack Value="5" />
      <Decay Value="30" />
      <Sustain Value="80" />
      <Release Value="200" />
    </AmpEnvelope>

    <!-- Standard keygroup zone layout — unchanged from sample-based keygroup -->
    <KeyGroups>
      <KeyGroup Number="0">
        <KeyLow Value="0" />
        <KeyHigh Value="127" />
        <Layer Number="1">
          <VelStart Value="1" />
          <VelEnd Value="127" />
          <RootNote Value="60" />  <!-- C3 root, MPC auto-detects if 0 -->
          <KeyTrack Value="True" />
        </Layer>
      </KeyGroup>
    </KeyGroups>

  </Program>
</MPCVObject>
```

### Which XO_OX Engines Map Well

| Engine | Exported Content | Wavetable Suitability |
|--------|-----------------|----------------------|
| OVERWORLD | Single-cycle NES/Genesis/SNES waveforms per ERA | Excellent — 3 distinct wavetable banks |
| OBLONG | Morphed analog waveshapes | Good — 4 waveform positions |
| ODYSSEY | Wavetable scan positions | Excellent — designed around wavetable scan |
| OPTIC | Visual pulse waveforms | Experimental — the visual-to-audio crossover |
| OUROBOROS | Chaotic attractor waveforms | Advanced — frame positions = attractor states |

### Oxport Tool

`xpn_keygroup_export.py` would need a `--synth-engine` mode that skips the standard sample layer structure and writes the `<SynthEngine>` block with wavetable references instead.

**New export step:** `oxport.py run --engine Overworld --mode keygroup-synth` — render single-cycle wavetables from each ERA position, then build Keygroup Synth XPM.

**Estimated build time:** 2 days (1 day single-cycle render infrastructure, 1 day XPM schema addition).

---

## Feature 3: 4-Slot Insert Effects

### What Changed

**[CONFIRMED]** MPC 3.5 expanded program-level insert effects from 2 slots to 4 slots per program. Each slot accepts any AIR effect plugin. The XPM schema gained two additional `<InsertFX>` blocks.

This means XPN packs can now ship with 4 pre-configured AIR effects embedded — producers load the pack and the sound design intent is preserved without manual effect chain setup.

### XPM Schema — 4-Slot InsertFX

```xml
<!-- Insert FX chain — 4 slots in MPC 3.5+ (was 2 in 3.x) -->
<InsertFX>

  <!-- Slot 1: Tone shaping — per-engine timbral character -->
  <InsertFXSlot Number="1">
    <FXName Value="AIR Lo-Fi" />
    <Enabled Value="True" />
    <Parameters>
      <Parameter Name="BitDepth" Value="0.85" />
      <Parameter Name="SampleRateReduction" Value="0.0" />
      <Parameter Name="Noise" Value="0.1" />
    </Parameters>
  </InsertFXSlot>

  <!-- Slot 2: Dynamics — compression character -->
  <InsertFXSlot Number="2">
    <FXName Value="AIR Compressor" />
    <Enabled Value="True" />
    <Parameters>
      <Parameter Name="Threshold" Value="-12.0" />
      <Parameter Name="Ratio" Value="4.0" />
      <Parameter Name="Attack" Value="5.0" />
      <Parameter Name="Release" Value="50.0" />
      <Parameter Name="MakeupGain" Value="3.0" />
    </Parameters>
  </InsertFXSlot>

  <!-- Slot 3: Spatial — engine-appropriate reverb or chorus -->
  <InsertFXSlot Number="3">
    <FXName Value="AIR Spring Reverb" />
    <Enabled Value="False" />  <!-- Off by default; producer activates -->
    <Parameters>
      <Parameter Name="DecayTime" Value="1.2" />
      <Parameter Name="PreDelay" Value="10" />
      <Parameter Name="HiDamp" Value="0.65" />
    </Parameters>
  </InsertFXSlot>

  <!-- Slot 4: Character — modulation or saturation -->
  <InsertFXSlot Number="4">
    <FXName Value="AIR Chorus" />
    <Enabled Value="False" />  <!-- Off by default -->
    <Parameters>
      <Parameter Name="Rate" Value="0.3" />
      <Parameter Name="Depth" Value="0.25" />
      <Parameter Name="Mix" Value="0.4" />
    </Parameters>
  </InsertFXSlot>

</InsertFX>
```

### Engine-to-AIR Effect Mapping

Each XO_OX engine has a character that maps to a preferred AIR effect chain. This is the canonical recommendation for Slot 1 (tonal shaping) and Slot 3 (spatial) per engine family:

| Engine | Character | Slot 1 (Tonal) | Slot 3 (Spatial) |
|--------|-----------|----------------|-----------------|
| OBLONG | Amber warm analog | AIR Vintage Filter | AIR Spring Reverb |
| ODYSSEY | Violet detuned pads | AIR Chorus | AIR Reverb (medium hall) |
| OVERWORLD | Neon Green chip | AIR Lo-Fi | AIR Delay (tape) |
| OPAL | Lavender granular | AIR Reverb (large hall) | AIR Flanger |
| ONSET | Electric Blue drum | AIR Compressor | AIR Transient Shaper |
| OVERDUB | Olive dub | AIR Tape Delay | AIR Spring Reverb |
| OUROBOROS | Red chaotic | AIR Distortion | AIR Reverb (large) |
| ORACLE | Indigo gendy | AIR Phaser | AIR Reverb (plate) |
| ORGANON | Bioluminescent Cyan | AIR Filter Gate | AIR Reverb (room) |
| OCEANIC | Phosphorescent Teal | AIR Chorus | AIR Reverb (large hall) |
| OSPREY / OSTERIA | Blue / Wine | AIR Lo-Fi | AIR Reverb (outdoor) |

### Oxport Tool

`xpn_drum_export.py` and `xpn_keygroup_export.py` both need an `<InsertFX>` generation step. The AIR effect preset per engine should be defined in a new config file: `Tools/engine_insert_fx_profiles.json`.

```bash
# Export with engine-default InsertFX chain
python3 oxport.py run --engine Opal --preset "Crystal Drift" \
    --insert-fx engine_default --output-dir dist/
```

**Estimated build time:** 1 day to add InsertFX block generation to both drum and keygroup export tools; 1 day to document the effect profiles JSON.

---

## Feature 4: PadColor XML

### What Changed

**[CONFIRMED]** MPC 3.5 added `<PadColor>` support in the XPM schema. Each pad can now carry a color definition that MPC displays on the hardware pad (backlit pads on MPC Live, Force) and in the software grid.

XO_OX can ship packs where each pad glows in the engine's accent color — or uses color to encode semantic information (velocity layer, synthesis type, compositional section).

### XML Encoding

**[PLAUSIBLE]** Based on observed MPC file structure and the Force's RGBA pad color system, `<PadColor>` encodes as a decimal or hex RGB tuple inside the `<Instrument>` or `<Pad>` block:

```xml
<!-- [PLAUSIBLE] Per-pad color encoding — decimal RGB components -->
<Pad Index="0">
  <Note Value="36" />
  <!-- PadColor as decimal R/G/B components (0–255) -->
  <PadColor R="167" G="139" B="250" />   <!-- XOpal Lavender #A78BFA -->
</Pad>

<!-- Alternative encoding observed in some MPC file variants — packed hex string -->
<!-- <PadColor Value="#A78BFA" /> -->

<!-- Alternative — decimal packed as integer (R*65536 + G*256 + B) -->
<!-- <PadColor Value="10984442" />  for #A78BFA = 167*65536 + 139*256 + 250 -->
```

**Recommendation:** Generate all three formats and test on hardware. Until hardware confirmation, prioritize the decimal R/G/B attribute form as it is the most human-readable and consistent with how Akai encodes other multi-component values (e.g., `<Volume L="..." R="..."/>`).

### Engine Accent Color Palette — MPC Pad Colors

The following table translates every registered XO_OX engine accent color to its decimal RGB form for `<PadColor>` injection:

| Engine | Accent Hex | R | G | B |
|--------|-----------|---|---|---|
| ODDFELIX (OddfeliX) | `#00A6D6` | 0 | 166 | 214 |
| ODDOSCAR (OddOscar) | `#E8839B` | 232 | 131 | 155 |
| OVERDUB | `#6B7B3A` | 107 | 123 | 58 |
| ODYSSEY | `#7B2D8B` | 123 | 45 | 139 |
| OBLONG | `#E9A84A` | 233 | 168 | 74 |
| OBESE | `#FF1493` | 255 | 20 | 147 |
| ONSET | `#0066FF` | 0 | 102 | 255 |
| OVERWORLD | `#39FF14` | 57 | 255 | 20 |
| OPAL | `#A78BFA` | 167 | 139 | 250 |
| ORBITAL | `#FF6B6B` | 255 | 107 | 107 |
| ORGANON | `#00CED1` | 0 | 206 | 209 |
| OUROBOROS | `#FF2D2D` | 255 | 45 | 45 |
| OBSIDIAN | `#E8E0D8` | 232 | 224 | 216 |
| ORIGAMI | `#E63946` | 230 | 57 | 70 |
| ORACLE | `#4B0082` | 75 | 0 | 130 |
| OBSCURA | `#8A9BA8` | 138 | 155 | 168 |
| OCEANIC | `#00B4A0` | 0 | 180 | 160 |
| OCELOT | `#C5832B` | 197 | 131 | 43 |
| OVERBITE | `#F0EDE8` | 240 | 237 | 232 |
| OPTIC | `#00FF41` | 0 | 255 | 65 |
| OBLIQUE | `#BF40FF` | 191 | 64 | 255 |
| OSPREY | `#1B4F8A` | 27 | 79 | 138 |
| OSTERIA | `#722F37` | 114 | 47 | 55 |
| OWLFISH | `#B8860B` | 184 | 134 | 11 |
| OHM | `#87AE73` | 135 | 174 | 115 |
| ORPHICA | `#7FDBCA` | 127 | 219 | 202 |
| OBBLIGATO | `#FF8A7A` | 255 | 138 | 122 |
| OTTONI | `#5B8A72` | 91 | 138 | 114 |
| OLE | `#C9377A` | 201 | 55 | 122 |
| OMBRE | `#7B6B8A` | 123 | 107 | 138 |
| ORCA | `#1B2838` | 27 | 40 | 56 |
| OCTOPUS | `#E040FB` | 224 | 64 | 251 |
| OVERLAP | `#00FFB4` | 0 | 255 | 180 |
| OUTWIT | `#CC6600` | 204 | 102 | 0 |

### Oxport Tool — Color Injection

`xpn_drum_export.py` and `xpn_keygroup_export.py` should accept an `--engine-color` flag that auto-populates `<PadColor>` for all active pads using the engine's registered accent color.

```bash
# Export drum kit with ONSET blue pad colors
python3 xpn_drum_export.py --preset "808 Reborn" --engine Onset \
    --pad-color engine --output-dir dist/

# Export with manual hex override
python3 xpn_drum_export.py --preset "808 Reborn" \
    --pad-color-hex "#FF1493" --output-dir dist/
```

A utility function `engine_accent_to_pad_color(engine_name)` should be added to a shared `xpn_color_utils.py` module, reading from the canonical engine table in CLAUDE.md.

**Estimated build time:** 1 day (color utility module + `--pad-color` flag in both export tools).

---

## Feature 5: BendRange Per-Program

### What Changed

**[PLAUSIBLE]** MPC 3.5 added per-program `BendRange` specification as a top-level `<Program>` attribute. Previously, pitch bend range was a global MIDI setting; per-program BendRange allows pack designers to set appropriate bend behavior for the program's instrument character.

### XPM Schema

```xml
<Program Type="Keygroup">
  <Name Value="XOdyssey_Deep_Drift" />
  <!-- BendRange in semitones — 1 to 24 -->
  <BendRange Value="7" />
  ...
</Program>
```

### XO_OX Standard BendRange Values

XO_OX uses polarity (feliX / Oscar / Hybrid) to define BendRange defaults:

| Polarity | BendRange | Semitones | Rationale |
|----------|-----------|-----------|-----------|
| feliX (bright, precise) | 2 | ±2 semitones | Subtle expressive pitch nudge; too much bend breaks the clarity |
| Oscar (dark, expressive) | 7 | ±7 semitones | Wide dub/bass bend; approaching a perfect fifth for dramatic sweeps |
| Hybrid (balanced) | 4 | ±4 semitones | Middle ground; wide enough for expression, contained enough for melody |
| Drum programs | 2 | ±2 semitones | Pitch bend on drums is occasional effect, not primary expression |

**Engine BendRange assignments:**

| Engine | Polarity | BendRange |
|--------|---------|-----------|
| ODDFELIX | feliX | 2 |
| ODDOSCAR | Oscar | 7 |
| OVERDUB | Oscar (send-heavy) | 7 |
| ODYSSEY | Hybrid (drift) | 4 |
| OBLONG | Hybrid | 4 |
| OBESE | Oscar (saturation-forward) | 7 |
| OVERWORLD | feliX (chip era) | 2 |
| OPAL | Hybrid (granular) | 4 |
| ONSET | Drum | 2 |
| ORACLE | Oscar (gendy, radical) | 7 |
| ORGANON | Hybrid (metabolic) | 4 |
| OUROBOROS | Oscar (chaotic) | 7 |
| OCEANIC | Oscar (separation) | 7 |
| OPTIC | feliX (visual, no pitch intent) | 2 |
| ORPHICA | feliX (harp, precise) | 2 |
| OSPREY / OSTERIA | Hybrid (ShoreSystem) | 4 |
| OVERLAP | Hybrid (FDN knot) | 4 |
| OUTWIT | Oscar (8-arm CA, wild) | 7 |
| OHM | Hybrid (commune) | 4 |
| OBBLIGATO | feliX (wind, melodic) | 2 |
| OTTONI | feliX (brass, melodic) | 2 |
| OLE | Oscar (drama axis) | 7 |

### Oxport Tool

Both `xpn_drum_export.py` and `xpn_keygroup_export.py` should embed `<BendRange>` using the engine's registered polarity. Add a `BEND_RANGE_BY_ENGINE` dict to a shared `xpn_engine_defaults.py` config module, populated from the table above.

**Estimated build time:** Half a day (add field to both export tools; no new infrastructure).

---

## Feature 6: Stem Track Integration

### Background

MPC Stems arrived for standalone hardware in July 2024 (zplane Stems Pro algorithm). By MPC 3.5, stems integration is stable and available on all Stems-capable hardware: MPC Live III, Force (3.5 update), and hardware with sufficient RAM. **[CONFIRMED]**

Stems separates any audio file into up to 4 stems (drums / bass / melody / vocals — exact labels device-dependent).

### Recommended Workflow: XO_OX Keygroup + Stems

The recommended configuration for a producer using an XO_OX pack alongside reference material:

```
Track 1:  XO_OX Keygroup program (e.g., XOpal Crystal Drift)
Track 2:  Stems — Drums stem from reference track
Track 3:  Stems — Bass stem from reference track
Track 4:  Stems — Melody stem from reference track
Track 5:  Stems — Vocals stem from reference track
```

**Q-Link shared control:** Assign Q-Link 1 to Track 1's CHARACTER macro (XOmnibus parameter index 0) and simultaneously to Track 2–5 volume via Q-Link page assignments. This lets a single Q-Link dial blend the synthesis character against the stems mix.

**[PROPOSED] XPN liner notes field:** XO_OX packs designed for stems integration should include a `stems_workflow` note in the `expansion.json`:

```json
{
  "name": "XOpal: Aquatic Dreams",
  "version": "1.0",
  "author": "XO_OX Designs",
  "stems_workflow": "Load this keygroup on Track 1. Use MPC Stems on Tracks 2-5 with your source material. Q-Links 1-4 control XOpal macros; Q-Links 5-8 are free for stems mix automation.",
  "stems_ready": true
}
```

### Design Principle for Stems-Compatible Packs

Packs designed to work alongside MPC Stems should favor:

1. **Frequency complement** — if the stems track will provide drums and bass, the XO_OX keygroup should sit in the midrange/treble (pad character, atmosphere, melody). If the stems track is primarily vocals, the keygroup should cover low and low-mid.
2. **Harmonic neutrality** — avoid packs that dominate all frequency ranges; leave room for stems to contribute
3. **Dynamics headroom** — set levels assuming the stems track will also be present; reduce the keygroup's internal compression slightly
4. **Loop length alignment** — if the pack includes any loop-based content, ensure loop lengths are exact multiples of common BPMs (16 bars at 90/120/140 BPM)

### Oxport Tool

No structural XPM change is required. The stems workflow is a documentation and liner notes feature.

`xpn_liner_notes.py` should accept a `--stems-workflow` flag that appends the recommended workflow as a page in the generated liner notes PDF.

`xpn_packager.py` should write a `stems_ready` boolean to `expansion.json` when the flag is present.

**Estimated build time:** 1 day (liner notes addition + expansion.json field).

---

## Feature 7: New XPN Packaging Requirements — MPC 3.5 vs 3.x

### expansion.json Schema Changes

**[PLAUSIBLE]** MPC 3.5 extended the `expansion.json` manifest schema to support additional metadata. The core structure remains backward-compatible with 3.x; new fields are additive.

Current `expansion.json` (as generated by `xpn_packager.py`):

```json
{
  "name": "XOnset: 808 Collection",
  "version": "1.0",
  "author": "XO_OX Designs",
  "description": "808 drum kits from XOnset"
}
```

Proposed 3.5-aware `expansion.json`:

```json
{
  "name": "XOnset: 808 Collection",
  "version": "1.0",
  "author": "XO_OX Designs",
  "description": "808 drum kits from XOnset",

  "mpc_version_min": "3.5",
  "program_types": ["Drum"],
  "engine_family": "ONSET",
  "accent_color": "#0066FF",

  "features": {
    "pad_colors": true,
    "insert_fx": true,
    "bend_range": true,
    "stems_ready": false,
    "plugin_programs": false,
    "microtonal": false
  },

  "pack_id": "xo-ox-onset-808-collection-v1",
  "website": "https://xo-ox.org/packs",
  "patreon": "https://patreon.com/xoox"
}
```

### Folder Structure — 3.5 Extensions

The core XPN ZIP structure is unchanged from 3.x. Two new optional subfolders are relevant for 3.5 packs:

```
MyPack.xpn (ZIP archive)
├── Expansions/
│   └── manifest                    (plain text: Name=, Version=, Author=)
│   └── expansion.json              (3.5-aware metadata — NEW, optional)
├── Programs/
│   ├── MyDrumKit.xpm
│   ├── MyDrumKit.mp3               (preview — same base name)
│   └── Plugin_Programs/            (VST3 plugin programs subfolder — NEW, 3.5+)
│       └── XOmnibus_OpalDrift.xpm
├── Samples/
│   ├── MyDrumKit/
│   │   ├── Kick_v1.wav
│   │   └── Snare_v2.wav
│   └── Wavetables/                 (single-cycle WAVs for KeygroupSynth — NEW, 3.5+)
│       └── XOverworld_NES_Saw.wav
└── artwork.png
```

**[SPECULATIVE]** Whether `Plugin_Programs/` is an Akai-enforced folder name or a convention is unconfirmed. Use it as an organizational convention until hardware testing clarifies.

### Naming Conventions — 3.5

No breaking changes to file naming in 3.5 vs 3.x. The following existing conventions remain in force:

- XPM files: `{PresetSlug}.xpm` — no spaces, no special characters except underscore
- WAV files: `{preset_slug}_{voice}_{layer}.wav` — as per existing Oxport convention
- Preview MP3: same base name as XPM — `{PresetSlug}.mp3`
- Pack archive: `{PackName}.xpn` — spaces replaced by underscores

**One new naming convention for 3.5 packs:** When a pack includes both sample-based programs and plugin programs, use a suffix to distinguish:

```
Programs/
    XOpal_Crystal_Drift.xpm              # Standard keygroup
    Plugin_Programs/
        XOpal_Crystal_Drift_LIVE.xpm     # VST3 plugin program variant
```

The `_LIVE` suffix signals to producers that this program requires XOmnibus VST3 installed to function.

### Oxport Tool Updates

`xpn_packager.py` needs four additions for full 3.5 support:

1. `--expansion-json` flag — writes the extended `expansion.json` with all 3.5 fields
2. `--plugin-programs` flag — routes VST3 XPM files to `Programs/Plugin_Programs/` subfolder
3. `--wavetables` flag — routes single-cycle WAVs to `Samples/Wavetables/` subfolder
4. `--mpc-version-min 3.5` flag — stamps the manifest with the minimum required MPC version

**Estimated build time:** 1 day for all four additions to packager.

---

## Summary: Oxport Update Roadmap

| Feature | Schema Change | Oxport Tool | Est. Effort |
|---------|-------------|------------|------------|
| 1. VST3 Plugin Programs | New `Plugin` program type + `<Plugin>` block | New `xpn_plugin_program_builder.py` | 2 days |
| 2. Keygroup Synth Engine | `Type="KeygroupSynth"` + `<SynthEngine>` block | `xpn_keygroup_export.py --synth-engine` | 2 days |
| 3. 4-Slot Insert Effects | 2 additional `<InsertFXSlot>` blocks | `--insert-fx` flag on both exporters | 2 days |
| 4. PadColor | `<PadColor R G B />` per pad | `--pad-color engine` flag + color utils module | 1 day |
| 5. BendRange | `<BendRange Value="N" />` in Program header | `BEND_RANGE_BY_ENGINE` dict in engine defaults | 0.5 days |
| 6. Stem Track Integration | `stems_ready` field in expansion.json | `--stems-workflow` flag on liner notes + packager | 1 day |
| 7. Packaging Requirements | Extended expansion.json schema + new subfolders | 4 additions to `xpn_packager.py` | 1 day |

**Total estimated effort:** 9.5 days for full 3.5 feature parity in Oxport.

**Priority order:**
1. PadColor — highest visual impact; 1 day; works on existing packs immediately
2. BendRange — trivial effort; corrects an oversight in existing packs
3. 4-Slot Insert Effects — meaningful producer value; applies to all future packs
4. Packaging (expansion.json extensions) — housekeeping; ensures forward compatibility
5. Stem Track Integration — documentation feature; no structural XPM work
6. Keygroup Synth Engine — medium effort; opens new pack category (wavetable-native programs)
7. VST3 Plugin Programs — highest effort; requires hardware test before shipping; desktop-only limitation
