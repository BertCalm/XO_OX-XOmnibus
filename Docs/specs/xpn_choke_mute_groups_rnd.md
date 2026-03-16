# XPN Choke & Mute Groups R&D
## Making Drum Kits Feel Real in MPC XPN Programs

**Date**: 2026-03-16
**Scope**: MPC XPM XML choke/mute group mechanics, ONSET kit design, oxport.py integration

---

## 1. Choke vs Mute in MPC — What Is the Difference?

These terms are often used interchangeably in drum machine lore, but in MPC they describe two distinct physical behaviors implemented through the same XML field.

**Choke** — one pad's trigger immediately silences another pad that is still playing. The canonical example: a closed hi-hat hits while an open hi-hat is ringing. The open hat is cut off mid-decay, mimicking the physical reality of a cymbal being grabbed. The choke is *directional in practice but symmetric in assignment* — both pads are in the same group, and whichever triggers later wins.

**Mute** — pads in the same group cannot play simultaneously at all. Any new hit stops the previous hit in the group. This applies to snare ghost variants, alternate snare tunings, or any voice where layering feels like clutter rather than texture.

In MPC XPM XML, both behaviors are controlled by a single field: `<MuteGroup>`. There is no separate `<ChokingGroup>` tag. The MPC firmware treats `MuteGroup` as "whichever pad in this group fires last, wins — all others are silenced." This covers both choke (cut-off decay) and mute (prevent stacking). The distinction lives in the *sound design intent*, not the XML.

Additionally, `<PadGroupMap>` in the program-level XML maps pad numbers to group numbers, enabling the pad matrix UI to show group assignments visually.

---

## 2. Standard Choke Group Conventions

Industry-standard assignments used across Roland, Akai, and Native Instruments drum machines:

| Group | Members | Behavior |
|-------|---------|---------|
| 1 | Closed Hat + Open Hat | CHat chokes OHat (and OHat chokes OHat itself if polyphony > 1) |
| 2 | Snare + Ghost Snare | Any snare variant mutes the previous |
| 3 | Ride + Ride Bell | Bell chokes ride body ring |
| 4 | Crash variants (optional) | Prevents crash wash stacking on dense kits |

**Elements that should NOT be in a choke group:**
- **Kick** — kicks must overlap on busy patterns (double bass). Choke causes audible cut on syncopated rolls.
- **Tom** — fills require 3–4 toms ringing simultaneously. Choke destroys the fill.
- **Clap** — natural clap decay is short; stacking two claps a millisecond apart adds crack, not mud. No choke.
- **Perc / FX** — usually polyphonic by design. Choke only if the specific sample is a one-shot sustained tone.

---

## 3. XPM Implementation

### Field Names and Value Range

| Field | Location | Value | Notes |
|-------|---------|-------|-------|
| `<MuteGroup>` | Inside `<Instrument>` block | `0` = no group, `1–32` = group number | MPC software supports up to 32 groups |
| `<Pad number="N" group="G"/>` | Inside `<PadGroupMap>` at program level | Same 1–32 range | Only pads with group > 0 need an entry |

### Example: Open/Closed Hi-Hat Choke Pair

```xml
<!-- Program-level PadGroupMap (pad numbers are 1-indexed) -->
<PadGroupMap>
    <Pad number="4" group="1"/>  <!-- Closed Hat — pad A4 -->
    <Pad number="5" group="1"/>  <!-- Open Hat  — pad B1 -->
</PadGroupMap>

<!-- Instrument-level MuteGroup inside each <Instrument> block -->

<!-- Closed Hat instrument -->
<Instrument number="42">   <!-- MIDI note 42 = F#2 -->
    <Name>CHat</Name>
    <MuteGroup>1</MuteGroup>
    <!-- ... other fields ... -->
</Instrument>

<!-- Open Hat instrument -->
<Instrument number="46">   <!-- MIDI note 46 = A#2 -->
    <Name>OHat</Name>
    <MuteGroup>1</MuteGroup>
    <!-- ... other fields ... -->
</Instrument>
```

Both instruments carry `<MuteGroup>1</MuteGroup>`. When either pad fires, MPC silences all instruments currently playing in group 1. Since the open hat has a longer decay, triggering the closed hat mid-ring produces the classic choke.

---

## 4. ONSET Kit Choke Group Assignments

ONSET's 8 voices mapped to the current `PAD_MAP` in `xpn_drum_export.py`:

| Voice | MIDI Note | Pad | MuteGroup | Rationale |
|-------|----------|-----|-----------|-----------|
| Kick  | 36 (C2)  | A1  | 0         | Must ring freely; double-kick patterns require overlap |
| Snare | 38 (D2)  | A2  | 2         | Mute group 2 for snare variants |
| Clap  | 39 (D#2) | A3  | 0         | Short natural decay; stacking adds crack |
| CHat  | 42 (F#2) | A4  | **1**     | Choke group 1 — chokes OHat on trigger |
| OHat  | 46 (A#2) | B1  | **1**     | Choke group 1 — cut by CHat |
| Tom   | 41 (F2)  | B2  | 0         | Poly; fills need simultaneous ring |
| Perc  | 43 (G2)  | B3  | 0         | Varies by preset; default open |
| FX    | 49 (C#3) | B4  | 0         | Sustained FX must overlap |

This matches the current `xpn_drum_export.py` `PAD_MAP` which already assigns `mg=1` to CHat and OHat. Snare mute group 2 is a proposed addition — not yet implemented in the current PAD_MAP (both snare and clap are `mg=0`).

**Ghost snare handling**: If a kit exports a dedicated ghost snare pad (e.g., lower velocity snare as a separate pad), assign it `MuteGroup=2` alongside the main snare. This prevents ghost snare + main snare ringing simultaneously when triggered in rapid succession.

---

## 5. Multi-Layer Choke — Velocity Layers and the Choke Group

When a voice uses 4 velocity layers (v1–v4 WAVs mapped to different velocity ranges within one `<Instrument>` block), **the choke group is assigned to the instrument, not the individual layers**. All 4 layers share the same `<MuteGroup>` value because they represent a single logical pad.

MPC's choke logic operates at the *instrument/pad* level: when a pad in the group fires, MPC silences all instruments in that group — regardless of which velocity layer is currently playing. A soft open hat hit (v1 layer) will be choked equally by the closed hat as a hard hit (v4 layer).

Consequence for design: ensure all velocity layers of the open hat have consistent release tails. If v1 is very short and v4 is long, the choke will be audible only on hard hits, which is acoustically correct.

---

## 6. Oxport Integration — `--choke-preset` Flag Design

`oxport.py` should expose choke/mute group assignment as a named preset rather than raw group numbers. Proposed flag:

```
--choke-preset  onset | standard | none | custom
```

### Preset Definitions

**`onset`** (default for ONSET-sourced kits):
```python
CHOKE_PRESET_ONSET = {
    "kick":  0,
    "snare": 2,   # mute group — snare variants
    "clap":  0,
    "chat":  1,   # choke group — hats
    "ohat":  1,   # choke group — hats
    "tom":   0,
    "perc":  0,
    "fx":    0,
}
```

**`standard`** (generic GM drum kit):
```python
CHOKE_PRESET_STANDARD = {
    "kick":  0,
    "snare": 2,
    "clap":  3,   # separate mute group for clap variants
    "chat":  1,
    "ohat":  1,
    "tom":   0,
    "perc":  4,   # perc variants muted together
    "fx":    0,
}
```

**`none`** — all `MuteGroup=0`. Useful for melodic / non-drum kits exported through the drum template, or for testing.

**`custom`** — reads group assignments from a JSON sidecar file (`--choke-json path/to/groups.json`), where keys are voice names and values are integers 0–32.

### Implementation Notes for oxport.py

- Apply choke preset values to both `<MuteGroup>` inside each `<Instrument>` block AND generate the corresponding `<PadGroupMap>` entries at the program level.
- Default behavior (no flag): use `onset` preset if the source preset contains `perc_xvc_` parameters (indicating ONSET origin); otherwise use `standard`.
- Log which voices received non-zero group assignments to stdout when `--verbose` is active.

---

## Summary

MPC XPM uses `<MuteGroup>` (integer, 0=off, 1–32=group) at the instrument level and `<PadGroupMap>/<Pad group="N"/>` at the program level. No separate choke tag exists — all cut behavior is "last trigger wins" within the group. For ONSET's 8 voices, only the hi-hat pair (CHat/OHat) requires active choke assignment (group 1); snare variants benefit from mute group 2 if ghost layers are present. All other ONSET voices are correctly left at group 0. The velocity-layer architecture does not change choke logic — group assignment lives at the instrument, not the layer. Oxport's `--choke-preset onset` flag automates correct assignment for all ONSET-sourced kit exports.
