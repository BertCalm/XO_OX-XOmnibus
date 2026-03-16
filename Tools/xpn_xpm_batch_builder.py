#!/usr/bin/env python3
"""
XPN XPM Batch Builder — XO_OX Designs
Builds multiple XPM program files from a single batch specification JSON,
enabling rapid creation of an entire pack's program structure in one pass.

Supported program types:
  DrumProgram       Pad-based (up to 16 pads, each pad = one Instrument slot)
  KeygroupProgram   Chromatic single-sample across full MIDI range
  MultiKeygroup     Multi-sample chromatic with explicit note zones

XPM Rules (never break):
  KeyTrack = True    (samples transpose across zones)
  RootNote = 0       (MPC auto-detect convention)
  VelStart = 0       on empty/unused layers (prevents ghost triggering)

Batch spec JSON shape:
{
  "pack_name": "TIDE TABLES",
  "output_dir": "build/tide_tables/Programs",
  "programs": [
    {
      "name": "UNDERTOW",
      "type": "DrumProgram",
      "template": "basic_drum_kit",
      "pads": [
        {"pad": 1, "sample": "Samples/kick_deep_01.wav", "name": "Kick Deep", "choke": 0},
        {"pad": 2, "sample": "Samples/snare_snap_01.wav", "name": "Snare Snap", "choke": 0}
      ]
    },
    {
      "name": "SURFACE",
      "type": "KeygroupProgram",
      "template": "chromatic_instrument",
      "sample": "Samples/pad_shimmer.wav",
      "root_note": 60
    },
    {
      "name": "DEPTHS",
      "type": "MultiKeygroup",
      "template": "chromatic_instrument",
      "zones": [
        {"low": 0,  "high": 47, "sample": "Samples/bass_low.wav",  "root_note": 36},
        {"low": 48, "high": 71, "sample": "Samples/bass_mid.wav",  "root_note": 60},
        {"low": 72, "high": 127,"sample": "Samples/bass_high.wav", "root_note": 84}
      ]
    }
  ]
}

Usage:
    python xpn_xpm_batch_builder.py --spec batch.json
    python xpn_xpm_batch_builder.py --spec batch.json --dry-run
    python xpn_xpm_batch_builder.py --spec batch.json --output-dir override/path/
"""

import argparse
import json
import sys
from pathlib import Path
from typing import Dict, List, Optional, Tuple
from xml.sax.saxutils import escape as xml_escape

# ---------------------------------------------------------------------------
# Constants
# ---------------------------------------------------------------------------

MPC_VERSION_BLOCK = (
    "  <Version>\n"
    "    <File_Version>1.7</File_Version>\n"
    "    <Application>MPC-V</Application>\n"
    "    <Application_Version>2.10.0.0</Application_Version>\n"
    "    <Platform>OSX</Platform>\n"
    "  </Version>\n"
)

# Drum program: 128 instrument slots; pads 1-16 map to instruments 0-15
# Remaining 112 slots are empty placeholders.
DRUM_INSTRUMENTS_TOTAL = 128

# Keygroup program: up to 128 instrument zones (one per semitone, or fewer)
KEYGROUP_INSTRUMENTS_TOTAL = 128

# ---------------------------------------------------------------------------
# Validation
# ---------------------------------------------------------------------------

def validate_spec(spec: dict) -> List[str]:
    """Return a list of error strings; empty list = valid."""
    errors = []

    if "programs" not in spec:
        errors.append("Missing required key: 'programs'")
        return errors

    names_seen = set()  # type: ignore
    valid_types = {"DrumProgram", "KeygroupProgram", "MultiKeygroup"}

    for idx, prog in enumerate(spec["programs"]):
        loc = f"programs[{idx}]"

        name = prog.get("name", "").strip()
        if not name:
            errors.append(f"{loc}: 'name' is required and must be non-empty")
        elif name in names_seen:
            errors.append(f"{loc}: duplicate program name '{name}'")
        else:
            names_seen.add(name)

        prog_type = prog.get("type", "")
        if prog_type not in valid_types:
            errors.append(
                f"{loc} '{name}': unknown type '{prog_type}'. "
                f"Valid types: {sorted(valid_types)}"
            )

        if prog_type == "DrumProgram":
            errors.extend(_validate_drum_program(prog, loc, name))
        elif prog_type == "KeygroupProgram":
            errors.extend(_validate_keygroup_program(prog, loc, name))
        elif prog_type == "MultiKeygroup":
            errors.extend(_validate_multikeygroup(prog, loc, name))

    return errors


def _validate_drum_program(prog: dict, loc: str, name: str) -> List[str]:
    errors = []  # type: List[str]
    pads = prog.get("pads")
    if not pads:
        errors.append(f"{loc} '{name}': DrumProgram requires 'pads' list")
        return errors

    pad_nums_seen = set()  # type: ignore
    for pidx, pad in enumerate(pads):
        pad_num = pad.get("pad")
        if pad_num is None:
            errors.append(f"{loc} '{name}' pad[{pidx}]: missing 'pad' number")
            continue
        if not isinstance(pad_num, int) or not (1 <= pad_num <= 16):
            errors.append(
                f"{loc} '{name}' pad[{pidx}]: 'pad' must be integer 1-16, got {pad_num!r}"
            )
        elif pad_num in pad_nums_seen:
            errors.append(
                f"{loc} '{name}' pad[{pidx}]: duplicate pad number {pad_num}"
            )
        else:
            pad_nums_seen.add(pad_num)

        sample = pad.get("sample", "")
        if sample and Path(sample).is_absolute():
            errors.append(
                f"{loc} '{name}' pad[{pidx}]: sample path must be relative, got '{sample}'"
            )

    return errors


def _validate_keygroup_program(prog: dict, loc: str, name: str) -> List[str]:
    errors = []
    sample = prog.get("sample", "")
    if not sample:
        errors.append(f"{loc} '{name}': KeygroupProgram requires 'sample'")
    elif Path(sample).is_absolute():
        errors.append(
            f"{loc} '{name}': sample path must be relative, got '{sample}'"
        )
    root = prog.get("root_note")
    if root is not None and not (0 <= root <= 127):
        errors.append(f"{loc} '{name}': root_note must be 0-127, got {root!r}")
    return errors


def _validate_multikeygroup(prog: dict, loc: str, name: str) -> List[str]:
    errors = []
    zones = prog.get("zones")
    if not zones:
        errors.append(f"{loc} '{name}': MultiKeygroup requires 'zones' list")
        return errors
    for zidx, zone in enumerate(zones):
        sample = zone.get("sample", "")
        if not sample:
            errors.append(f"{loc} '{name}' zone[{zidx}]: 'sample' is required")
        elif Path(sample).is_absolute():
            errors.append(
                f"{loc} '{name}' zone[{zidx}]: sample path must be relative, got '{sample}'"
            )
        low = zone.get("low")
        high = zone.get("high")
        if low is None or high is None:
            errors.append(f"{loc} '{name}' zone[{zidx}]: 'low' and 'high' are required")
        else:
            if not (0 <= low <= 127) or not (0 <= high <= 127):
                errors.append(
                    f"{loc} '{name}' zone[{zidx}]: low/high must be 0-127"
                )
            if low > high:
                errors.append(
                    f"{loc} '{name}' zone[{zidx}]: low ({low}) > high ({high})"
                )
        root = zone.get("root_note")
        if root is not None and not (0 <= root <= 127):
            errors.append(
                f"{loc} '{name}' zone[{zidx}]: root_note must be 0-127, got {root!r}"
            )
    return errors

# ---------------------------------------------------------------------------
# XPM Generators
# ---------------------------------------------------------------------------

def _fmt(v: float) -> str:
    return f"{v:.6f}"


def _empty_layer_xml(layer_num: int = 1) -> str:
    """Empty placeholder layer — VelStart=0 per XPM rules."""
    return (
        f'            <Layer number="{layer_num}">\n'
        f'              <Active>False</Active>\n'
        f'              <SampleName></SampleName>\n'
        f'              <SampleFile></SampleFile>\n'
        f'              <File></File>\n'
        f'              <VelStart>0</VelStart>\n'
        f'              <VelEnd>0</VelEnd>\n'
        f'            </Layer>\n'
    )


def _active_layer_xml(sample_path: str, layer_num: int = 1,
                      vel_start: int = 0, vel_end: int = 127) -> str:
    """Single active sample layer."""
    sample_file = Path(sample_path).name
    sample_name = Path(sample_path).stem
    return (
        f'            <Layer number="{layer_num}">\n'
        f'              <Active>True</Active>\n'
        f'              <Volume>{_fmt(1.0)}</Volume>\n'
        f'              <Pan>{_fmt(0.5)}</Pan>\n'
        f'              <Pitch>{_fmt(0.0)}</Pitch>\n'
        f'              <TuneCoarse>0</TuneCoarse>\n'
        f'              <TuneFine>0</TuneFine>\n'
        f'              <SampleName>{xml_escape(sample_name)}</SampleName>\n'
        f'              <SampleFile>{xml_escape(sample_file)}</SampleFile>\n'
        f'              <File>{xml_escape(sample_path)}</File>\n'
        f'              <RootNote>0</RootNote>\n'
        f'              <KeyTrack>True</KeyTrack>\n'
        f'              <OneShot>False</OneShot>\n'
        f'              <Loop>False</Loop>\n'
        f'              <LoopStart>0</LoopStart>\n'
        f'              <LoopEnd>0</LoopEnd>\n'
        f'              <LoopXFade>0</LoopXFade>\n'
        f'              <VolumeAttack>{_fmt(0)}</VolumeAttack>\n'
        f'              <VolumeDecay>{_fmt(0)}</VolumeDecay>\n'
        f'              <VolumeSustain>{_fmt(1)}</VolumeSustain>\n'
        f'              <VolumeRelease>{_fmt(0.5)}</VolumeRelease>\n'
        f'              <VelStart>{vel_start}</VelStart>\n'
        f'              <VelEnd>{vel_end}</VelEnd>\n'
        f'            </Layer>\n'
    )


def _instrument_block_drum(instrument_num: int, pad_cfg: Optional[dict],
                            choke_group: int = 0) -> str:
    """One <Instrument> for a drum program slot."""
    if pad_cfg is None:
        # Empty slot
        active = "False"
        low = high = instrument_num
        layers = _empty_layer_xml(1)
        note_name = ""
    else:
        active = "True"
        low = high = instrument_num
        sample = pad_cfg.get("sample", "")
        layers = (
            _active_layer_xml(sample, layer_num=1, vel_start=0, vel_end=127)
            if sample
            else _empty_layer_xml(1)
        )
        note_name = pad_cfg.get("name", "")

    choke_xml = (
        f"        <MuteGroup>{choke_group}</MuteGroup>\n"
        if choke_group
        else ""
    )
    note_label = f"  <!-- {xml_escape(note_name)} -->" if note_name else ""

    return (
        f'      <Instrument number="{instrument_num}">{note_label}\n'
        f'        <Active>{active}</Active>\n'
        f'        <Volume>{_fmt(1.0)}</Volume>\n'
        f'        <Pan>{_fmt(0.5)}</Pan>\n'
        f'        <Tune>0</Tune>\n'
        f'        <Transpose>0</Transpose>\n'
        f'        <VolumeAttack>{_fmt(0)}</VolumeAttack>\n'
        f'        <VolumeHold>{_fmt(0)}</VolumeHold>\n'
        f'        <VolumeDecay>{_fmt(0.5)}</VolumeDecay>\n'
        f'        <VolumeSustain>{_fmt(1)}</VolumeSustain>\n'
        f'        <VolumeRelease>{_fmt(0.05)}</VolumeRelease>\n'
        f'        <FilterType>2</FilterType>\n'
        f'        <Cutoff>{_fmt(1.0)}</Cutoff>\n'
        f'        <Resonance>{_fmt(0.0)}</Resonance>\n'
        f'        <FilterEnvAmt>{_fmt(0.0)}</FilterEnvAmt>\n'
        f'        <LowNote>{low}</LowNote>\n'
        f'        <HighNote>{high}</HighNote>\n'
        f'        <RootNote>0</RootNote>\n'
        f'        <KeyTrack>True</KeyTrack>\n'
        f'        <OneShot>True</OneShot>\n'
        f'{choke_xml}'
        f'        <Layers>\n'
        f'{layers}'
        f'        </Layers>\n'
        f'      </Instrument>\n'
    )


def _instrument_block_keygroup(instrument_num: int, low: int, high: int,
                                sample_path: str) -> str:
    """One <Instrument> for a keygroup zone."""
    active = "True" if sample_path else "False"
    layers = (
        _active_layer_xml(sample_path, layer_num=1, vel_start=0, vel_end=127)
        if sample_path
        else _empty_layer_xml(1)
    )
    return (
        f'      <Instrument number="{instrument_num}">\n'
        f'        <Active>{active}</Active>\n'
        f'        <Volume>{_fmt(1.0)}</Volume>\n'
        f'        <Pan>{_fmt(0.5)}</Pan>\n'
        f'        <Tune>0</Tune>\n'
        f'        <Transpose>0</Transpose>\n'
        f'        <VolumeAttack>{_fmt(0)}</VolumeAttack>\n'
        f'        <VolumeHold>{_fmt(0)}</VolumeHold>\n'
        f'        <VolumeDecay>{_fmt(0)}</VolumeDecay>\n'
        f'        <VolumeSustain>{_fmt(1)}</VolumeSustain>\n'
        f'        <VolumeRelease>{_fmt(0.5)}</VolumeRelease>\n'
        f'        <FilterType>2</FilterType>\n'
        f'        <Cutoff>{_fmt(1.0)}</Cutoff>\n'
        f'        <Resonance>{_fmt(0.0)}</Resonance>\n'
        f'        <FilterEnvAmt>{_fmt(0.0)}</FilterEnvAmt>\n'
        f'        <LowNote>{low}</LowNote>\n'
        f'        <HighNote>{high}</HighNote>\n'
        f'        <RootNote>0</RootNote>\n'
        f'        <KeyTrack>True</KeyTrack>\n'
        f'        <OneShot>False</OneShot>\n'
        f'        <Layers>\n'
        f'{layers}'
        f'        </Layers>\n'
        f'      </Instrument>\n'
    )


def _xpm_header(prog_name: str, prog_type_str: str) -> str:
    return (
        '<?xml version="1.0" encoding="UTF-8"?>\n\n'
        '<MPCVObject>\n'
        f'{MPC_VERSION_BLOCK}'
        f'  <Program type="{prog_type_str}">\n'
        f'    <Name>{xml_escape(prog_name)}</Name>\n'
    )


def _xpm_footer() -> str:
    return "  </Program>\n</MPCVObject>\n"


# ---------------------------------------------------------------------------
# Build: DrumProgram
# ---------------------------------------------------------------------------

def build_drum_program(prog: dict) -> str:
    """Generate XPM XML for a DrumProgram."""
    prog_name = prog["name"]
    pads_cfg = prog.get("pads", [])

    # Map pad number (1-16) → config
    pad_map: dict[int, dict] = {p["pad"]: p for p in pads_cfg if "pad" in p}

    instruments_xml = ""
    for slot in range(DRUM_INSTRUMENTS_TOTAL):
        pad_num = slot + 1  # pad numbers are 1-indexed
        if pad_num in pad_map:
            cfg = pad_map[pad_num]
            choke = cfg.get("choke", 0)
            instruments_xml += _instrument_block_drum(slot, cfg, choke_group=choke)
        else:
            instruments_xml += _instrument_block_drum(slot, None)

    # PadNoteMap: map physical pads 1-16 to instrument indices 0-15
    pad_note_entries = "\n".join(
        f'        <Pad number="{p}" note="{p - 1}"/>'
        for p in range(1, 17)
    )

    return (
        _xpm_header(prog_name, "Drum")
        + "    <PadNoteMap>\n"
        + pad_note_entries + "\n"
        + "    </PadNoteMap>\n"
        + "    <Instruments>\n"
        + instruments_xml
        + "    </Instruments>\n"
        + _xpm_footer()
    )


# ---------------------------------------------------------------------------
# Build: KeygroupProgram (single sample, chromatic)
# ---------------------------------------------------------------------------

def build_keygroup_program(prog: dict) -> str:
    """Generate XPM XML for a KeygroupProgram (single sample across all MIDI notes)."""
    prog_name = prog["name"]
    sample = prog.get("sample", "")

    # One instrument spanning the full MIDI range (0-127)
    instruments_xml = _instrument_block_keygroup(
        instrument_num=0,
        low=0,
        high=127,
        sample_path=sample,
    )
    # Remaining slots empty
    for slot in range(1, KEYGROUP_INSTRUMENTS_TOTAL):
        instruments_xml += _instrument_block_keygroup(slot, slot, slot, "")

    return (
        _xpm_header(prog_name, "Keygroup")
        + "    <Instruments>\n"
        + instruments_xml
        + "    </Instruments>\n"
        + _xpm_footer()
    )


# ---------------------------------------------------------------------------
# Build: MultiKeygroup (multiple zones)
# ---------------------------------------------------------------------------

def build_multikeygroup_program(prog: dict) -> str:
    """Generate XPM XML for a MultiKeygroup (explicit note zones)."""
    prog_name = prog["name"]
    zones = prog.get("zones", [])

    instruments_xml = ""
    for slot, zone in enumerate(zones):
        instruments_xml += _instrument_block_keygroup(
            instrument_num=slot,
            low=zone.get("low", 0),
            high=zone.get("high", 127),
            sample_path=zone.get("sample", ""),
        )
    # Pad remaining slots as empty up to total
    for slot in range(len(zones), KEYGROUP_INSTRUMENTS_TOTAL):
        instruments_xml += _instrument_block_keygroup(slot, slot, slot, "")

    return (
        _xpm_header(prog_name, "Keygroup")
        + "    <Instruments>\n"
        + instruments_xml
        + "    </Instruments>\n"
        + _xpm_footer()
    )


# ---------------------------------------------------------------------------
# Dispatch
# ---------------------------------------------------------------------------

BUILDERS = {
    "DrumProgram":    build_drum_program,
    "KeygroupProgram": build_keygroup_program,
    "MultiKeygroup":  build_multikeygroup_program,
}


def build_program(prog: dict) -> str:
    """Dispatch to the correct builder by type."""
    prog_type = prog.get("type", "")
    builder = BUILDERS.get(prog_type)
    if builder is None:
        raise ValueError(f"Unknown program type: {prog_type!r}")
    return builder(prog)


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def parse_args(argv=None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Build multiple XPM program files from a batch spec JSON.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    parser.add_argument(
        "--spec",
        required=True,
        metavar="BATCH_JSON",
        help="Path to the batch specification JSON file.",
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Validate the spec and show what would be written without writing files.",
    )
    parser.add_argument(
        "--output-dir",
        metavar="DIR",
        help="Override the output_dir from the spec.",
    )
    return parser.parse_args(argv)


def main(argv=None) -> int:
    args = parse_args(argv)

    spec_path = Path(args.spec)
    if not spec_path.exists():
        print(f"ERROR: spec file not found: {spec_path}", file=sys.stderr)
        return 1

    try:
        spec = json.loads(spec_path.read_text(encoding="utf-8"))
    except json.JSONDecodeError as exc:
        print(f"ERROR: invalid JSON in spec file: {exc}", file=sys.stderr)
        return 1

    # Validate
    errors = validate_spec(spec)
    if errors:
        print("Validation errors:", file=sys.stderr)
        for err in errors:
            print(f"  - {err}", file=sys.stderr)
        return 1

    # Resolve output directory
    raw_out = args.output_dir or spec.get("output_dir", "build/programs")
    output_dir = Path(raw_out)
    # Resolve relative to spec file's directory if not absolute
    if not output_dir.is_absolute():
        output_dir = spec_path.parent / output_dir

    pack_name = spec.get("pack_name", "UNNAMED PACK")
    programs = spec.get("programs", [])

    print(f"Pack: {pack_name}")
    print(f"Output dir: {output_dir}")
    print(f"Programs: {len(programs)}")

    if not args.dry_run:
        output_dir.mkdir(parents=True, exist_ok=True)

    results = []   # type: List[Tuple[str, str, str]]  # (name, type, filepath)
    build_errors = []  # type: List[str]

    for prog in programs:
        name = prog["name"]
        prog_type = prog.get("type", "?")
        safe_name = name.replace(" ", "_").replace("/", "-")
        xpm_filename = f"{safe_name}.xpm"
        xpm_path = output_dir / xpm_filename

        try:
            xpm_xml = build_program(prog)
        except Exception as exc:
            build_errors.append(f"{name}: {exc}")
            continue

        if args.dry_run:
            results.append((name, prog_type, str(xpm_path) + "  [dry-run]"))
        else:
            xpm_path.write_text(xpm_xml, encoding="utf-8")
            results.append((name, prog_type, str(xpm_path)))

    # Summary
    print()
    col_w = max((len(r[0]) for r in results), default=10) + 2
    type_w = max((len(r[1]) for r in results), default=14) + 2
    print(f"{'Program':<{col_w}}  {'Type':<{type_w}}  Path")
    print("-" * 80)
    for name, prog_type, path in results:
        print(f"{name:<{col_w}}  {prog_type:<{type_w}}  {path}")

    if build_errors:
        print("\nBuild errors:", file=sys.stderr)
        for err in build_errors:
            print(f"  - {err}", file=sys.stderr)

    wrote = len(results) - (len(results) if args.dry_run else 0)
    action = "Would write" if args.dry_run else "Wrote"
    print(f"\n{action} {len(results)} XPM file(s) to {output_dir}")

    return 1 if build_errors else 0


if __name__ == "__main__":
    sys.exit(main())
