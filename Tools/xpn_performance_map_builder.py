#!/usr/bin/env python3
"""
XPN Performance Map Builder — XO_OX Designs
Builds custom MPC XPM programs where pads are arranged for live performance
ergonomics rather than chromatic order.

Each pad slot is assigned an explicit sample with a velocity sensitivity mode
and optional choke group. The output is a ready-to-use Keygroup XPM file.

Pad ↔ MIDI note mapping (MPC 4×4 grid, row-major from bottom-left):
    Pad  1 (D1) → MIDI 36 (C2)   Pad  2 (D2) → MIDI 37 (C#2)
    Pad  3 (D3) → MIDI 38 (D2)   Pad  4 (D4) → MIDI 39 (D#2)
    Pad  5 (C1) → MIDI 40 (E2)   Pad  6 (C2) → MIDI 41 (F2)
    Pad  7 (C3) → MIDI 42 (F#2)  Pad  8 (C4) → MIDI 43 (G2)
    Pad  9 (B1) → MIDI 44 (G#2)  Pad 10 (B2) → MIDI 45 (A2)
    Pad 11 (B3) → MIDI 46 (A#2)  Pad 12 (B4) → MIDI 47 (B2)
    Pad 13 (A1) → MIDI 48 (C3)   Pad 14 (A2) → MIDI 49 (C#3)
    Pad 15 (A3) → MIDI 50 (D3)   Pad 16 (A4) → MIDI 51 (D#3)

Velocity sensitivity modes:
    none   — single layer 0–127
    low    — single layer 0–127 (alias for none; use when dynamics don't matter)
    medium — 2 layers: 0–90 (soft), 91–127 (hard)
    high   — 4 layers (Vibe's curve): 0–40 / 41–90 / 91–110 / 111–127

Config JSON schema:
    {
      "program_name": "My Performance Map",
      "pads": [
        {
          "pad": 1,
          "sample_path": "Samples/MyKit/kick.wav",
          "name": "Kick",
          "velocity_sensitivity": "high",
          "choke_group": 0
        },
        ...
      ]
    }

Usage:
    python xpn_performance_map_builder.py --config perf_map.json --output program.xpm
    python xpn_performance_map_builder.py --config perf_map.json --output program.xpm --no-validate-paths
    python xpn_performance_map_builder.py --example   # print example config JSON

XPM rules (XO_OX CLAUDE.md — never break):
    KeyTrack  = True   (samples transpose across zones)
    RootNote  = 0      (MPC auto-detect convention)
    VelStart  = 0      on empty/unused layers (prevents ghost triggering)
"""

import argparse
import json
import sys
from pathlib import Path
from xml.sax.saxutils import escape as xml_escape

# ---------------------------------------------------------------------------
# Constants
# ---------------------------------------------------------------------------

TOTAL_PADS = 16
PAD_MIDI_BASE = 36   # Pad 1 → MIDI 36 (C2)

# Velocity layer definitions per sensitivity mode.
# Each entry: (vel_start, vel_end, volume)
# Note: VelStart on the first active layer is 0 (per XO_OX convention for
# performance maps — we want every hit, including very soft ghost notes).
VEL_LAYERS: dict[str, list[tuple[int, int, float]]] = {
    "none": [
        (0, 127, 0.707946),
    ],
    "low": [
        (0, 127, 0.707946),
    ],
    "medium": [
        (0,  90, 0.550000),   # soft
        (91, 127, 0.950000),  # hard
    ],
    "high": [
        (0,   20, 0.350000),  # ghost   (Ghost Council zone — barely touching)
        (21,  55, 0.620000),  # light   (finger drumming sweet spot)
        (56,  90, 0.820000),  # medium  (deliberate hits)
        (91, 127, 0.970000),  # hard    (power hits, peak force)
    ],
}

VALID_SENSITIVITY = set(VEL_LAYERS.keys())

_XPM_HEADER = """\
<?xml version="1.0" encoding="UTF-8"?>
<MPCVObject>
  <Version>
    <File_Version>1.7</File_Version>
    <Application>MPC-V</Application>
    <Application_Version>2.10.0.0</Application_Version>
    <Platform>OSX</Platform>
  </Version>"""

EXAMPLE_CONFIG = {
    "program_name": "Live Performance Map",
    "pads": [
        {"pad": 1,  "sample_path": "Samples/Kit/kick.wav",    "name": "Kick",    "velocity_sensitivity": "high",   "choke_group": 0},
        {"pad": 2,  "sample_path": "Samples/Kit/snare.wav",   "name": "Snare",   "velocity_sensitivity": "high",   "choke_group": 0},
        {"pad": 3,  "sample_path": "Samples/Kit/clap.wav",    "name": "Clap",    "velocity_sensitivity": "medium", "choke_group": 0},
        {"pad": 4,  "sample_path": "Samples/Kit/chat.wav",    "name": "CHat",    "velocity_sensitivity": "medium", "choke_group": 1},
        {"pad": 5,  "sample_path": "Samples/Kit/ohat.wav",    "name": "OHat",    "velocity_sensitivity": "medium", "choke_group": 1},
        {"pad": 6,  "sample_path": "Samples/Kit/tom1.wav",    "name": "Tom Lo",  "velocity_sensitivity": "high",   "choke_group": 0},
        {"pad": 7,  "sample_path": "Samples/Kit/tom2.wav",    "name": "Tom Hi",  "velocity_sensitivity": "high",   "choke_group": 0},
        {"pad": 8,  "sample_path": "Samples/Kit/perc.wav",    "name": "Perc",    "velocity_sensitivity": "medium", "choke_group": 0},
        {"pad": 9,  "sample_path": "Samples/Kit/fx1.wav",     "name": "FX 1",    "velocity_sensitivity": "none",   "choke_group": 2},
        {"pad": 10, "sample_path": "Samples/Kit/fx2.wav",     "name": "FX 2",    "velocity_sensitivity": "none",   "choke_group": 2},
        {"pad": 11, "sample_path": "Samples/Kit/crash.wav",   "name": "Crash",   "velocity_sensitivity": "medium", "choke_group": 0},
        {"pad": 12, "sample_path": "Samples/Kit/ride.wav",    "name": "Ride",    "velocity_sensitivity": "medium", "choke_group": 0},
        {"pad": 13, "sample_path": "Samples/Kit/bass.wav",    "name": "Bass",    "velocity_sensitivity": "high",   "choke_group": 0},
        {"pad": 14, "sample_path": "Samples/Kit/stab.wav",    "name": "Stab",    "velocity_sensitivity": "medium", "choke_group": 0},
        {"pad": 15, "sample_path": "Samples/Kit/riser.wav",   "name": "Riser",   "velocity_sensitivity": "none",   "choke_group": 3},
        {"pad": 16, "sample_path": "Samples/Kit/down.wav",    "name": "Downlift","velocity_sensitivity": "none",   "choke_group": 3},
    ]
}


# ---------------------------------------------------------------------------
# Validation
# ---------------------------------------------------------------------------

def validate_config(config: dict, validate_paths: bool = True) -> list[str]:
    """
    Validate a parsed config dict. Returns a list of error strings (empty = OK).
    """
    errors: list[str] = []

    if "pads" not in config or not isinstance(config["pads"], list):
        errors.append("Config must have a 'pads' list.")
        return errors   # nothing else to check

    pads = config["pads"]
    seen_pads: set[int] = set()

    for i, entry in enumerate(pads):
        ctx = f"pads[{i}]"

        # pad number
        pad_num = entry.get("pad")
        if not isinstance(pad_num, int) or not (1 <= pad_num <= TOTAL_PADS):
            errors.append(f"{ctx}: 'pad' must be an integer 1–{TOTAL_PADS}, got {pad_num!r}")
            continue
        if pad_num in seen_pads:
            errors.append(f"{ctx}: duplicate pad number {pad_num}")
        seen_pads.add(pad_num)

        # sample_path
        sample_path = entry.get("sample_path", "")
        if not sample_path:
            errors.append(f"{ctx} (pad {pad_num}): 'sample_path' is required and cannot be empty")
        elif validate_paths and not Path(sample_path).exists():
            errors.append(
                f"{ctx} (pad {pad_num}): sample file not found: {sample_path}"
            )

        # name
        name = entry.get("name", "")
        if not isinstance(name, str):
            errors.append(f"{ctx} (pad {pad_num}): 'name' must be a string")

        # velocity_sensitivity
        vel_mode = entry.get("velocity_sensitivity", "none")
        if vel_mode not in VALID_SENSITIVITY:
            errors.append(
                f"{ctx} (pad {pad_num}): 'velocity_sensitivity' must be one of "
                f"{sorted(VALID_SENSITIVITY)}, got {vel_mode!r}"
            )

        # choke_group
        choke = entry.get("choke_group", 0)
        if not isinstance(choke, int) or not (0 <= choke <= 8):
            errors.append(
                f"{ctx} (pad {pad_num}): 'choke_group' must be an integer 0–8, got {choke!r}"
            )

    # Check all 16 pads present
    missing = sorted(set(range(1, TOTAL_PADS + 1)) - seen_pads)
    if missing:
        errors.append(
            f"Not all 16 pads are defined. Missing pad(s): {missing}. "
            "Every pad must have an entry (use a placeholder sample for unused pads)."
        )

    return errors


# ---------------------------------------------------------------------------
# XPM generation helpers
# ---------------------------------------------------------------------------

def _fmt(v: float) -> str:
    """Format a float to 6 decimal places."""
    return f"{v:.6f}"


def _pad_to_midi(pad_num: int) -> int:
    """Convert a 1-based pad number to its MPC MIDI note (pad 1 → 36)."""
    return PAD_MIDI_BASE + (pad_num - 1)


def _layer_xml(layer_num: int, vel_start: int, vel_end: int, volume: float,
               sample_name: str, sample_file: str) -> str:
    """
    Build one <Layer> XML block.

    XO_OX XPM rules applied here:
        KeyTrack  = True
        RootNote  = 0
        VelStart  = 0  on empty layers (prevents ghost triggering)
    """
    active = "True" if sample_name else "False"
    safe_name = xml_escape(sample_name)
    safe_file = xml_escape(sample_file)
    # Empty layer: VelStart=0, VelEnd=0 (Rex Rule #3)
    if not sample_name:
        vel_start = 0
        vel_end = 0
        volume = 0.707946

    return (
        f'            <Layer number="{layer_num}">\n'
        f'              <Active>{active}</Active>\n'
        f'              <Volume>{_fmt(volume)}</Volume>\n'
        f'              <Pan>0.500000</Pan>\n'
        f'              <Pitch>0.000000</Pitch>\n'
        f'              <TuneCoarse>0</TuneCoarse>\n'
        f'              <TuneFine>0</TuneFine>\n'
        f'              <SampleName>{safe_name}</SampleName>\n'
        f'              <SampleFile>{safe_file}</SampleFile>\n'
        f'              <File>{safe_file}</File>\n'
        f'              <RootNote>0</RootNote>\n'
        f'              <KeyTrack>True</KeyTrack>\n'
        f'              <OneShot>True</OneShot>\n'
        f'              <Loop>False</Loop>\n'
        f'              <LoopStart>0</LoopStart>\n'
        f'              <LoopEnd>0</LoopEnd>\n'
        f'              <LoopXFade>0</LoopXFade>\n'
        f'              <VolumeAttack>{_fmt(0)}</VolumeAttack>\n'
        f'              <VolumeDecay>{_fmt(0)}</VolumeDecay>\n'
        f'              <VolumeSustain>{_fmt(1)}</VolumeSustain>\n'
        f'              <VolumeRelease>{_fmt(0.05)}</VolumeRelease>\n'
        f'              <VelStart>{vel_start}</VelStart>\n'
        f'              <VelEnd>{vel_end}</VelEnd>\n'
        f'            </Layer>\n'
    )


def _instrument_xml(inst_idx: int, midi_note: int, pad_entry: dict) -> str:
    """
    Build one <Instrument> XML block for a single performance-map pad.

    Each instrument covers exactly one MIDI note (low=high=root=midi_note).
    Velocity layers are expanded from the velocity_sensitivity mode.
    """
    sample_path = pad_entry.get("sample_path", "")
    name        = pad_entry.get("name", f"Pad {pad_entry['pad']}")
    vel_mode    = pad_entry.get("velocity_sensitivity", "none")
    choke_group = int(pad_entry.get("choke_group", 0))

    # Resolve to sanitized mode
    if vel_mode not in VALID_SENSITIVITY:
        vel_mode = "none"

    layers_def = VEL_LAYERS[vel_mode]
    num_layers = len(layers_def)

    # Build sample name from path stem
    sample_file = Path(sample_path).name if sample_path else ""
    sample_name = Path(sample_path).stem if sample_path else ""

    # Generate layer XML — up to 4 layers maximum
    layers_xml = ""
    for i, (vs, ve, vol) in enumerate(layers_def):
        layers_xml += _layer_xml(
            layer_num=i + 1,
            vel_start=vs,
            vel_end=ve,
            volume=vol,
            sample_name=sample_name,
            sample_file=sample_file,
        )
    # Pad remaining slots (up to 4) with empty/silent layers (VelStart=0)
    for i in range(num_layers, 4):
        layers_xml += _layer_xml(
            layer_num=i + 1,
            vel_start=0,
            vel_end=0,
            volume=0.707946,
            sample_name="",
            sample_file="",
        )

    choke_attr = f' ChokeGroup="{choke_group}"' if choke_group > 0 else ""

    return (
        f'      <Instrument number="{inst_idx}"{choke_attr}>\n'
        f'        <Active>True</Active>\n'
        f'        <Volume>1.000000</Volume>\n'
        f'        <Pan>0.500000</Pan>\n'
        f'        <Tune>0</Tune>\n'
        f'        <Transpose>0</Transpose>\n'
        f'        <VolumeAttack>{_fmt(0)}</VolumeAttack>\n'
        f'        <VolumeHold>{_fmt(0)}</VolumeHold>\n'
        f'        <VolumeDecay>{_fmt(0)}</VolumeDecay>\n'
        f'        <VolumeSustain>{_fmt(1)}</VolumeSustain>\n'
        f'        <VolumeRelease>{_fmt(0.05)}</VolumeRelease>\n'
        f'        <FilterType>2</FilterType>\n'
        f'        <Cutoff>1.000000</Cutoff>\n'
        f'        <Resonance>0.000000</Resonance>\n'
        f'        <FilterEnvAmt>0.000000</FilterEnvAmt>\n'
        f'        <LowNote>{midi_note}</LowNote>\n'
        f'        <HighNote>{midi_note}</HighNote>\n'
        f'        <RootNote>0</RootNote>\n'
        f'        <KeyTrack>True</KeyTrack>\n'
        f'        <OneShot>True</OneShot>\n'
        f'        <Layers>\n'
        f'{layers_xml}'
        f'        </Layers>\n'
        f'      </Instrument>\n'
    )


def _empty_instrument_xml(inst_idx: int, midi_note: int) -> str:
    """
    Silent placeholder instrument for any pad that was not specified in config.
    VelStart=0 prevents ghost triggering (XO_OX rule).
    """
    layers_xml = ""
    for i in range(4):
        layers_xml += _layer_xml(
            layer_num=i + 1,
            vel_start=0,
            vel_end=0,
            volume=0.707946,
            sample_name="",
            sample_file="",
        )

    return (
        f'      <Instrument number="{inst_idx}">\n'
        f'        <Active>False</Active>\n'
        f'        <Volume>1.000000</Volume>\n'
        f'        <Pan>0.500000</Pan>\n'
        f'        <Tune>0</Tune>\n'
        f'        <Transpose>0</Transpose>\n'
        f'        <VolumeAttack>0.000000</VolumeAttack>\n'
        f'        <VolumeHold>0.000000</VolumeHold>\n'
        f'        <VolumeDecay>0.000000</VolumeDecay>\n'
        f'        <VolumeSustain>1.000000</VolumeSustain>\n'
        f'        <VolumeRelease>0.000000</VolumeRelease>\n'
        f'        <FilterType>2</FilterType>\n'
        f'        <Cutoff>1.000000</Cutoff>\n'
        f'        <Resonance>0.000000</Resonance>\n'
        f'        <FilterEnvAmt>0.000000</FilterEnvAmt>\n'
        f'        <LowNote>{midi_note}</LowNote>\n'
        f'        <HighNote>{midi_note}</HighNote>\n'
        f'        <RootNote>0</RootNote>\n'
        f'        <KeyTrack>True</KeyTrack>\n'
        f'        <OneShot>True</OneShot>\n'
        f'        <Layers>\n'
        f'{layers_xml}'
        f'        </Layers>\n'
        f'      </Instrument>\n'
    )


# ---------------------------------------------------------------------------
# XPM assembly
# ---------------------------------------------------------------------------

def build_performance_xpm(config: dict) -> str:
    """
    Generate a complete Keygroup XPM string from a validated config dict.

    The program contains exactly 16 instruments, one per MPC pad.
    Each instrument covers a single MIDI note (the pad's trigger note).
    Pad assignments from config override; unspecified pads get silent placeholders.
    """
    program_name = xml_escape(
        str(config.get("program_name", "Performance Map"))[:32]
    )

    # Index pad entries by pad number for O(1) lookup
    pad_map: dict[int, dict] = {}
    for entry in config.get("pads", []):
        pad_num = int(entry["pad"])
        pad_map[pad_num] = entry

    instruments_xml = ""
    for pad_num in range(1, TOTAL_PADS + 1):
        midi_note = _pad_to_midi(pad_num)
        inst_idx  = pad_num - 1          # 0-based instrument index

        if pad_num in pad_map:
            instruments_xml += _instrument_xml(inst_idx, midi_note, pad_map[pad_num])
        else:
            instruments_xml += _empty_instrument_xml(inst_idx, midi_note)

    # Q-Link assignments — sensible defaults for a performance map
    qlink_xml = (
        '    <QLinks>\n'
        '      <QLink number="1">\n'
        '        <Name>TONE</Name>\n'
        '        <Parameter>FilterCutoff</Parameter>\n'
        '        <Min>0.200000</Min>\n'
        '        <Max>1.000000</Max>\n'
        '      </QLink>\n'
        '      <QLink number="2">\n'
        '        <Name>ATTACK</Name>\n'
        '        <Parameter>VolumeAttack</Parameter>\n'
        '        <Min>0.000000</Min>\n'
        '        <Max>0.500000</Max>\n'
        '      </QLink>\n'
        '      <QLink number="3">\n'
        '        <Name>DECAY</Name>\n'
        '        <Parameter>VolumeDecay</Parameter>\n'
        '        <Min>0.000000</Min>\n'
        '        <Max>1.000000</Max>\n'
        '      </QLink>\n'
        '      <QLink number="4">\n'
        '        <Name>SEND</Name>\n'
        '        <Parameter>Send1</Parameter>\n'
        '        <Min>0.000000</Min>\n'
        '        <Max>0.700000</Max>\n'
        '      </QLink>\n'
        '    </QLinks>\n'
    )

    xpm = (
        f"{_XPM_HEADER}\n"
        f"  <Program type=\"Keygroup\">\n"
        f"    <Name>{program_name}</Name>\n"
        f"    <KeygroupNumKeygroups>{TOTAL_PADS}</KeygroupNumKeygroups>\n"
        f"    <KeygroupPitchBendRange>2</KeygroupPitchBendRange>\n"
        f"    <KeygroupWheelToLfo>0.000000</KeygroupWheelToLfo>\n"
        f"{qlink_xml}"
        f"    <Instruments>\n"
        f"{instruments_xml}"
        f"    </Instruments>\n"
        f"  </Program>\n"
        f"</MPCVObject>\n"
    )
    return xpm


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def _print_example():
    """Print an example config JSON to stdout."""
    print(json.dumps(EXAMPLE_CONFIG, indent=2))


def main() -> int:
    parser = argparse.ArgumentParser(
        description=(
            "XPN Performance Map Builder — generate MPC XPM programs "
            "with ergonomic pad layouts for live performance."
        ),
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    parser.add_argument(
        "--config", "-c",
        metavar="CONFIG_JSON",
        help="Path to performance map config JSON file.",
    )
    parser.add_argument(
        "--output", "-o",
        metavar="OUTPUT_XPM",
        help="Output path for the generated .xpm file.",
    )
    parser.add_argument(
        "--no-validate-paths",
        action="store_true",
        default=False,
        help="Skip checking that sample files exist on disk.",
    )
    parser.add_argument(
        "--example",
        action="store_true",
        default=False,
        help="Print an example config JSON and exit.",
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        default=False,
        help="Validate config and print XPM to stdout without writing a file.",
    )

    args = parser.parse_args()

    # --example shortcut
    if args.example:
        _print_example()
        return 0

    # Require --config for all other operations
    if not args.config:
        parser.error("--config is required (or use --example to print a sample config)")

    # Load config
    config_path = Path(args.config)
    if not config_path.exists():
        print(f"ERROR: Config file not found: {config_path}", file=sys.stderr)
        return 1

    try:
        with open(config_path, encoding="utf-8") as fh:
            config = json.load(fh)
    except json.JSONDecodeError as exc:
        print(f"ERROR: Invalid JSON in {config_path}: {exc}", file=sys.stderr)
        return 1

    # Validate
    validate_paths = not args.no_validate_paths
    errors = validate_config(config, validate_paths=validate_paths)
    if errors:
        print("Validation errors:", file=sys.stderr)
        for err in errors:
            print(f"  • {err}", file=sys.stderr)
        return 1

    # Generate XPM
    xpm_content = build_performance_xpm(config)

    # Output
    if args.dry_run or not args.output:
        print(xpm_content)
        return 0

    output_path = Path(args.output)
    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_text(xpm_content, encoding="utf-8")

    # Summary
    pad_count = len(config.get("pads", []))
    prog_name = config.get("program_name", "Performance Map")
    choke_pads = [
        e for e in config.get("pads", []) if int(e.get("choke_group", 0)) > 0
    ]

    print(f"✓ Written: {output_path}")
    print(f"  Program : {prog_name}")
    print(f"  Pads    : {pad_count}/{TOTAL_PADS} populated")
    if choke_pads:
        choke_groups: dict[int, list[int]] = {}
        for e in choke_pads:
            g = int(e["choke_group"])
            choke_groups.setdefault(g, []).append(e["pad"])
        for g_num, pads in sorted(choke_groups.items()):
            print(f"  Choke {g_num} : pads {pads}")

    # Velocity mode breakdown
    from collections import Counter
    vel_counts = Counter(
        e.get("velocity_sensitivity", "none") for e in config.get("pads", [])
    )
    for mode, count in sorted(vel_counts.items()):
        layers = len(VEL_LAYERS.get(mode, []))
        print(f"  {mode:8s}: {count} pad(s) × {layers} layer(s)")

    return 0


if __name__ == "__main__":
    sys.exit(main())
