# MPC OS 3.5 Format Innovations — R&D

**Topic**: Speculative XPN/XPM format extensions that would dramatically improve expansion pack quality
**Date**: 2026-03-16
**Status**: R&D / Speculative design

---

## 1. Current Format Limitations (RESEARCH-BASED)

The XPM format as of MPC OS 3.2–3.4 is a static XML snapshot. It describes what samples are loaded and how they respond to velocity and note ranges, but it cannot express intent.

Key gaps:

- **No macro definitions.** Q-Link assignments exist in the XPM but have no names, descriptions, or parameter-to-control mappings beyond raw CC routing. The hardware shows generic "Q1–Q4" labels unless the user renames them manually — and those names don't travel with the pack.
- **No coupling or relationship metadata.** There is no way to tell the MPC that Pad 1 and Pad 5 are thematically linked, or that pads should be muted in groups, without using the existing (limited) mute group integers.
- **No preset DNA or design intent.** When XO_OX ships a preset called "Anchor Weight," the XPM contains no field that says "this is a bass program, built for 75–95 BPM, tuned reference D1, character: dense." That context is invisible to the MPC browser.
- **No dynamic/streaming sample references.** All samples must be present locally at load time. There is no lazy-load or on-demand stem fetch mechanism.
- **No morph or variation states.** A program is one static configuration. There is no built-in way to define "here is the dry version, here is the wet version, interpolate between them."
- **No tempo-synced parameters.** LFO rates, delay times, and modulation depths cannot be expressed as beat-relative values in the XPM — they are always absolute milliseconds or Hz values burned at export time.
- **No conditional sample selection logic.** The format is declarative only: velocity triggers this layer, note range triggers that one. There is no "if Q-Link-3 is above 0.5, prefer sample_B over sample_A" without separate pads.

---

## 2. Speculative: `<MacroDefinition>` Block (SPECULATIVE)

If Akai exposed Q-Link metadata in XPM, it could look like this:

```xml
<MacroDefinitions>
  <Macro index="1" name="MACHINE" displayColor="#FF4400">
    <Description>Drives analog circuit saturation from clean to crushed</Description>
    <TargetParam engine="Filter" param="Drive" rangeMin="0.0" rangeMax="1.0"/>
    <TargetParam engine="Amp" param="Saturation" rangeMin="0.0" rangeMax="0.6"/>
  </Macro>
  <Macro index="2" name="SPACE" displayColor="#0044FF">
    <Description>Reverb pre-delay + tail length unified control</Description>
    <TargetParam engine="Reverb" param="PreDelay" rangeMin="0ms" rangeMax="80ms"/>
    <TargetParam engine="Reverb" param="DecayTime" rangeMin="0.4s" rangeMax="6.0s"/>
  </Macro>
</MacroDefinitions>
```

For XO_OX packs, this would be transformative. Every engine ships with four named macros (MACHINE/PUNCH/SPACE/MUTATE for ONSET, DRIVE/DRIFT/SPACE/MORPH for OVERDUB). Right now those names exist only in documentation and in the user's memory. With `<MacroDefinition>`, the MPC hardware could display "MACHINE" on the Q-Link screen and a producer new to the pack would immediately understand what they are turning.

---

## 3. Speculative: `<PresetMorph>` Element (SPECULATIVE)

A morph defines two complete program states (A and B) and instructs the MPC to interpolate all numerical parameters between them as a Q-Link is turned.

```xml
<PresetMorph controlSource="Q-Link-4" morphMode="linear">
  <StateA label="Dry Organic">
    <ParamValue engine="Reverb" param="WetDry" value="0.05"/>
    <ParamValue engine="Filter" param="Cutoff" value="18000"/>
  </StateA>
  <StateB label="Drenched Space">
    <ParamValue engine="Reverb" param="WetDry" value="0.90"/>
    <ParamValue engine="Filter" param="Cutoff" value="800"/>
  </StateB>
</PresetMorph>
```

This is fundamentally different from velocity layering. Velocity is a per-note trigger — it selects a sample at the moment of attack. A morph operates at the program level across time and can be moved between notes, during a held chord, or automated in a sequence. Velocity cannot create a slow filter-sweep across a sustained pad; a morph control can.

---

## 4. Speculative: `<StemReference>` for Dynamic Loading (SPECULATIVE)

Rather than embedding all 300MB of samples in the XPN bundle, a `<StemReference>` would point to a drive path or a registered expansion library that the MPC loads on demand.

```xml
<StemReference id="bass_layer_wet"
               externalPath="XO_OX/ONSET/stems/kick_wet_full.wav"
               loadStrategy="on-demand"
               fallbackSampleId="kick_dry_compressed"/>
```

RAM implications: The current MPC sample RAM ceiling (2GB on MPC One+, higher on Live II) would be dramatically extended in effective terms if only the playing pads' current layers were resident. The tradeoff is disk latency — a cold stem fetch mid-performance is audible. The `loadStrategy` attribute could support `"preload"`, `"on-demand"`, and `"background-prefetch"` modes to let pack designers balance quality against reliability.

---

## 5. Speculative: `<BehaviorScript>` Micro-Language (SPECULATIVE)

A minimal conditional language embedded in XPM, evaluated per-hit by the MPC engine:

```
IF velocity > 100 AND qlink[3] > 0.5 THEN sample = "hit_hard_wet"
IF velocity > 100 AND qlink[3] <= 0.5 THEN sample = "hit_hard_dry"
IF velocity <= 100 THEN sample = "hit_soft"
WEIGHT cycle [sample_A, sample_B, sample_C] BY [0.5, 0.3, 0.2]
```

Five-line syntax covers: velocity threshold branching, Q-Link state conditionals, explicit weighted random cycle selection. This replaces the current workaround of creating 8 pads to represent what should be one intelligent pad.

---

## 6. MPCe `<PadCornerAssignment>` — Most Likely Real Schema (RESEARCH-BASED / SPECULATIVE SCHEMA)

The MPCe introduces 3D pressure-sensitive pads where each corner can carry a distinct assignment. Based on the known hardware architecture (4 corners per pad, pressure z-axis, XY position tracking), the most probable XPM extension is:

```xml
<PadCornerAssignments padIndex="0">
  <Corner position="top-left"     sampleId="chord_maj7"   pitchOffset="0"/>
  <Corner position="top-right"    sampleId="chord_dom7"   pitchOffset="0"/>
  <Corner position="bottom-left"  sampleId="chord_min7"   pitchOffset="0"/>
  <Corner position="bottom-right" sampleId="chord_dim7"   pitchOffset="0"/>
  <CenterZone sampleId="chord_root" pressureControlsParam="Filter.Cutoff"/>
</PadCornerAssignments>
```

This section is the most likely to reflect Akai's actual implementation direction because the hardware investment in 3D pads has no software return unless the XPM format can describe where on the pad a sample lives. Akai's existing `<PadNoteMap>` precedent confirms they are willing to extend pad assignment schemas when hardware demands it.

---

## 7. Community Wishlist (RESEARCH-BASED)

Based on longstanding MPC forum and user community sentiment, the most consistently requested format-level changes are:

- **Persistent Q-Link names per program.** Producers building complex kits spend significant time renaming Q-Links and lose those names when loading a new program. A persistent `<MacroDefinition>` block (see section 2) is the single most-requested format feature.
- **Preset tagging and category metadata.** The MPC browser has basic folder navigation but no tag-based filtering. Users want to filter by mood, genre, key, BPM range — none of which the XPM can currently express.
- **Relative velocity curve definitions.** Advanced users want to ship packs with customized velocity response curves that travel with the program, not require manual per-machine setup.
- **Pad color persistence in XPN.** Pad colors assigned in a program are not reliably restored from XPN bundles across OS versions. A canonical `<PadColor>` element with guaranteed spec compliance is frequently requested.
- **Multi-output routing hints.** Users with multi-output setups want programs to suggest routing (pad 1 → output 3, pad 2 → output 4) that the MPC could optionally apply on load.

---

## XO_OX Priority Order for Advocacy

If Akai opened a format RFC process, the order of impact for XO_OX expansion packs:

1. `<MacroDefinition>` — directly exposes engine character to first-time users
2. `<PresetMorph>` — enables the live morph performance philosophy central to XO_OX design
3. Preset tagging metadata — improves discoverability of 2,369+ presets in the browser
4. `<PadCornerAssignment>` — future-proofs packs for MPCe hardware
5. `<BehaviorScript>` — reduces pad count overhead for intelligent multi-sample programs
6. `<StemReference>` — long-term RAM/quality ceiling raiser, lower immediate priority
