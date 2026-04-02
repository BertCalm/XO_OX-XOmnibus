# XPN Q-Link Defaults R&D

Smart Q-Link default assignments for XO_OX XPN packs — what should the 4 Q-Links control out-of-box for each engine type.

---

## 1. Q-Link Assignment in XPN

Q-Links are defined at the **program level** inside the `<QLinks>` block, which sits between `<PadGroupMap>` and `<Instruments>` in the XPM XML. Each knob is one `<QLink number="N">` element.

```xml
<QLinks>
  <QLink number="1">
    <Name>TONE</Name>
    <Parameter>FilterCutoff</Parameter>
    <Min>0.200000</Min>
    <Max>1.000000</Max>
  </QLink>
  ...
</QLinks>
```

**Field reference:**

| Field | Notes |
|---|---|
| `number` | 1–4, maps to the four physical Q-Link knobs |
| `<Name>` | Display label shown on MPC screen — up to ~8 characters |
| `<Parameter>` | MPC-internal parameter ID (see table below) |
| `<Min>` / `<Max>` | Normalized float or integer; defines the knob travel range |

**Known parameter IDs** (confirmed in Tools):

| Parameter ID | Description | Typical range |
|---|---|---|
| `FilterCutoff` | Low-pass filter cutoff | 0.0 – 1.0 |
| `Resonance` | Filter resonance | 0.0 – 1.0 |
| `TuneCoarse` | Coarse pitch in semitones | -12 – +12 |
| `VolumeAttack` | Amplitude envelope attack | 0.0 – 1.0 |
| `VolumeDecay` | Amplitude envelope decay | 0.05 – 1.5 |
| `VolumeRelease` | Amplitude envelope release | 0.0 – 2.0 |
| `Send1` | FX send 1 (reverb bus) | 0.0 – 0.7 |
| `Volume` | Layer/instrument level | 0.0 – 1.0 |
| `Pan` | Stereo pan | 0.0 – 1.0 (0.5 = center) |
| `VelocitySensitivity` | Velocity-to-volume depth | 0.5 – 2.0 |

Q-Links are **program-level only** in standard XPM. They cannot reference individual keygroup or layer parameters directly — the parameter IDs apply globally to the program's engine. Per-voice Q-Links require a separate XPM per voice, which is not practical for kit programs. This means ONSET's per-voice Q-Link targeting must be approximated via program globals or via separate pad-group routing.

---

## 2. Design Philosophy

**Primary principle:** Q1–Q4 should map to the four XOceanus macros wherever the macro hierarchy survives the XPN render:

| Q-Link | XOceanus Macro | Underlying behavior |
|---|---|---|
| Q1 | CHARACTER | Filter cutoff / tone / drive — defines the instrument's identity |
| Q2 | MOVEMENT | Pitch, LFO rate, or envelope decay — things that change over time |
| Q3 | COUPLING | Resonance, filter envelope amount, or cross-voice interaction |
| Q4 | SPACE | Send 1 (reverb bus) — room size, environment |

This macro-to-knob correspondence is already established in `xpn_drum_export.py` (`_generate_qlink_xml`) and `xpn_keygroup_export.py`. It should be treated as doctrine across all Tools.

**When macros don't survive the render** (rendered samples lose their plugin state): fall back to the program-type defaults below. The musical intent of CHARACTER/MOVEMENT/COUPLING/SPACE is preserved even if the underlying parameter is now a generic MPC control rather than a plugin macro.

**Naming convention for `<Name>` labels:** Short, musical, uppercase. Prefer instrument-character words over technical terms — TONE not CUTOFF, PUNCH not ATTACK, SPACE not REVERB. The name is what the performer reads on stage.

---

## 3. Per-Engine-Type Q-Link Defaults

### Drum programs

Target: immediate kit shaping without menu-diving.

| Q-Link | Name | Parameter | Range | Rationale |
|---|---|---|---|---|
| Q1 | TONE | `FilterCutoff` | 0.2 – 1.0 | Opens/closes the whole kit |
| Q2 | PITCH | `TuneCoarse` | -12 – +12 | Global tune shift, useful for key matching |
| Q3 | BITE | `Resonance` | 0.0 – 0.6 | Adds grit without going into self-oscillation |
| Q4 | SPACE | `Send1` | 0.0 – 0.7 | Reverb send — keep Max at 0.7 to avoid washed-out drums |

This is the current default in `xpn_drum_export.py`. It is confirmed correct. The `Max` cap on `Send1` at 0.7 is intentional — full reverb on drums destroys transient clarity.

### Melodic keygroups

Target: expressive lead/bass playing from first touch.

| Q-Link | Name | Parameter | Range | Rationale |
|---|---|---|---|---|
| Q1 | TONE | `FilterCutoff` | 0.2 – 1.0 | Core tone shaping |
| Q2 | ATTACK | `VolumeAttack` | 0.0 – 1.0 | Slow attack = pad feel, fast = pluck — huge character change |
| Q3 | RELEASE | `VolumeRelease` | 0.0 – 2.0 | Controls note length and room behavior |
| Q4 | SPACE | `Send1` | 0.0 – 0.7 | Reverb depth |

Current default in `xpn_keygroup_export.py`. Note: the brief proposed Q3=LFO Rate, but `LfoRate` is not a confirmed MPC XPM parameter ID. `VolumeRelease` is confirmed, more universally useful, and maps cleanly to the COUPLING (sustain/decay interaction) macro intent. Recommend keeping current implementation.

### Atmospheric / pad programs

Target: slow-evolving texture design.

| Q-Link | Name | Parameter | Range | Rationale |
|---|---|---|---|---|
| Q1 | TONE | `FilterCutoff` | 0.1 – 0.9 | Tighter upper bound prevents shrillness in pads |
| Q2 | SWELL | `VolumeAttack` | 0.1 – 2.0 | Long attack range — pads live in this zone |
| Q3 | DEPTH | `Resonance` | 0.0 – 0.5 | Gentle resonance adds warmth without harshness |
| Q4 | SPACE | `Send1` | 0.0 – 0.7 | Reverb wash |

Difference from melodic: `FilterCutoff` Max pulled down to 0.9 (pads shouldn't be bright), `VolumeAttack` Min raised to 0.1 (instantaneous attack on a pad sounds wrong). Q2 label is SWELL not ATTACK.

### Character / saturation programs

Target: drive and tone sculpting for processed, gritty sounds.

| Q-Link | Name | Parameter | Range | Rationale |
|---|---|---|---|---|
| Q1 | DRIVE | `Volume` | 0.7 – 1.5 | Drives the sample level into saturation |
| Q2 | TONE | `FilterCutoff` | 0.1 – 1.0 | Full range for EQ-style tone sculpting |
| Q3 | BITE | `Resonance` | 0.0 – 0.7 | Higher Max than drums — character sounds can take more resonance |
| Q4 | SPACE | `Send1` | 0.0 – 0.5 | Smaller Max — character sounds often want a drier mix |

Note: MPC XPM does not expose a native `Saturation` or `Drive` parameter ID at program level. Using `Volume` with a range above 1.0 (up to 1.5) achieves soft clip behavior via sample headroom. This is the closest available approximation until VST3 plugin program Q-Link targeting is confirmed working (see `mpc_vst3_plugin_program_rnd.md`).

---

## 4. ONSET-Specific Q-Links

ONSET has 8 synthesis voices (Kick, Snare, CHat, OHat, Clap, Tom, Perc, FX). Because Q-Links are program-level in XPM, per-voice targeting is not directly available — the assignments control the full kit. Smart defaults should therefore target the most musically impactful cross-kit dimensions:

| Q-Link | Name | Parameter | Range | Musical effect |
|---|---|---|---|---|
| Q1 | PUNCH | `FilterCutoff` | 0.3 – 1.0 | Opens all voices simultaneously — mix brightness |
| Q2 | SNAP | `VolumeDecay` | 0.05 – 0.8 | Shorter decay = snappier, longer = roomy; most audible on Snare/Kick |
| Q3 | GRIT | `Resonance` | 0.0 – 0.5 | Adds character across all transients |
| Q4 | SPACE | `Send1` | 0.0 – 0.7 | Global reverb send |

The `xpn_drum_export.py` per-voice `qlink_1`/`qlink_2` fields (e.g. kick gets `FilterCutoff`/`TuneCoarse`, snare gets `FilterCutoff`/`TuneCoarse`) are used for voice-level parameter initialization in the individual `<Instrument>` nodes — they are not the program-level Q-Links. Keep this distinction clear: per-voice qlink fields in the Python voice-defaults dict set instrument parameters, not the `<QLinks>` block.

---

## 5. Oxport Integration

**Recommendation: auto-assign with override flag.**

`oxport.py` should auto-detect engine type from the export context (drum program vs. keygroup program) and apply the correct preset automatically. Manual override should be available for edge cases.

Proposed flag:

```
--qlink-preset drum|melodic|atmospheric|character
```

**Implementation sketch:**

```python
QLINK_PRESETS = {
    "drum": [
        ("TONE",    "FilterCutoff",  "0.200000", "1.000000"),
        ("PITCH",   "TuneCoarse",    "-12",       "12"),
        ("BITE",    "Resonance",     "0.000000",  "0.600000"),
        ("SPACE",   "Send1",         "0.000000",  "0.700000"),
    ],
    "melodic": [
        ("TONE",    "FilterCutoff",  "0.200000", "1.000000"),
        ("ATTACK",  "VolumeAttack",  "0.000000", "1.000000"),
        ("RELEASE", "VolumeRelease", "0.000000", "2.000000"),
        ("SPACE",   "Send1",         "0.000000", "0.700000"),
    ],
    "atmospheric": [
        ("TONE",    "FilterCutoff",  "0.100000", "0.900000"),
        ("SWELL",   "VolumeAttack",  "0.100000", "2.000000"),
        ("DEPTH",   "Resonance",     "0.000000", "0.500000"),
        ("SPACE",   "Send1",         "0.000000", "0.700000"),
    ],
    "character": [
        ("DRIVE",   "Volume",        "0.700000", "1.500000"),
        ("TONE",    "FilterCutoff",  "0.100000", "1.000000"),
        ("BITE",    "Resonance",     "0.000000", "0.700000"),
        ("SPACE",   "Send1",         "0.000000", "0.500000"),
    ],
}
```

Auto-detection logic: if the export pipeline calls `xpn_drum_export.py`, default to `drum`; if it calls `xpn_keygroup_export.py`, default to `melodic`. The `--qlink-preset atmospheric` and `--qlink-preset character` flags are for the designer to set explicitly on keygroup exports that warrant them.

Q4=SPACE/Send1 is constant across all four presets — it should always be the final knob. This is a UX convention: performers reaching for reverb always find it on Q4.

---

## Open Questions

- **`LfoRate` parameter ID**: Not yet confirmed as a valid MPC XPM program-level parameter. Needs validation against a live MPC XPM file using an LFO-heavy preset.
- **Plugin program Q-Links**: If `mpc_vst3_plugin_program_rnd.md` confirms that VST3 plugin programs expose engine-specific parameter IDs, ONSET Q-Links could target actual `onset_kick_punch` / `onset_snare_snap` parameters directly. This would make per-voice targeting possible and is the ideal long-term solution.
- **Q-Link range calibration**: Current ranges are set conservatively. A listening pass with each preset type should validate that full knob travel produces musically useful results without sudden jumps or inaudible zones.
