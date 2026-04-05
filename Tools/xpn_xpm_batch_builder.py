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
# Velocity / taxonomy imports (lazy — only needed for layered builder)
# ---------------------------------------------------------------------------
def _get_velocity_standard():
    """Lazy import of xpn_velocity_standard to avoid hard dependency."""
    try:
        from xpn_velocity_standard import vel_start, vel_end, NUM_LAYERS
        return vel_start, vel_end, NUM_LAYERS
    except ImportError:
        import importlib.util, os
        _here = Path(__file__).parent
        spec = importlib.util.spec_from_file_location(
            "xpn_velocity_standard", _here / "xpn_velocity_standard.py"
        )
        mod = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(mod)
        return mod.vel_start, mod.vel_end, mod.NUM_LAYERS


def _get_voice_taxonomy():
    """Lazy import of xpn_voice_taxonomy to avoid hard dependency."""
    try:
        from xpn_voice_taxonomy import compute_slot_budget, display_label, rr_count
        return compute_slot_budget, display_label, rr_count
    except ImportError:
        import importlib.util
        _here = Path(__file__).parent
        spec = importlib.util.spec_from_file_location(
            "xpn_voice_taxonomy", _here / "xpn_voice_taxonomy.py"
        )
        mod = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(mod)
        return mod.compute_slot_budget, mod.display_label, mod.rr_count

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

# Tier slot ceilings (QDD locked 2026-04-04)
# SURFACE: single-layer, 16 active pads max
# DEEP: 4 velocity layers, no round-robin
# TRENCH: 4 velocity layers + per-voice round-robin
TIER_SLOT_LIMITS = {
    "SURFACE": 16,
    "DEEP": 64,
    "TRENCH": 96,
}

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
# Slot Budget Validator
# ---------------------------------------------------------------------------

def validate_slot_budget(pads: list, tier: str = "DEEP") -> Tuple[bool, int, int, list]:
    """Validate that total XPM slots for all pads fit within the tier ceiling.

    Slot count per pad:
      SURFACE : 1 per pad (single layer)
      DEEP    : NUM_LAYERS per pad (velocity layers, no RR)
      TRENCH  : NUM_LAYERS * max(1, rr_count(voice)) per pad

    Args:
        pads: List of pad dicts, each with at least a "voice" key (engine-internal name).
        tier: One of "SURFACE", "DEEP", "TRENCH".

    Returns:
        Tuple of (is_valid, total_slots, limit, details_per_pad).
        details_per_pad is a list of dicts:
            {"voice": str, "slots": int, "rr": int, "vel_layers": int}
        is_valid is False when total_slots > limit.

    Hard gate: prints a clear error when invalid. Callers should abort the build
    on False.
    """
    tier = tier.upper()
    if tier not in TIER_SLOT_LIMITS:
        valid_tiers = sorted(TIER_SLOT_LIMITS)
        print(
            f"ERROR: Unknown tier {tier!r}. Valid tiers: {valid_tiers}",
            file=sys.stderr,
        )
        return False, 0, 0, []

    limit = TIER_SLOT_LIMITS[tier]
    _, _, num_layers = _get_velocity_standard()
    _, _, rr_count_fn = _get_voice_taxonomy()

    details: List[dict] = []
    total = 0

    for pad in pads:
        voice = pad.get("voice", "")
        if tier == "SURFACE":
            vel_layers = 1
            rr = 1
        elif tier == "DEEP":
            vel_layers = num_layers
            rr = 1
        else:  # TRENCH
            vel_layers = num_layers
            rr = max(1, rr_count_fn(voice))

        slots = vel_layers * rr
        total += slots
        details.append(
            {
                "voice": voice,
                "slots": slots,
                "rr": rr,
                "vel_layers": vel_layers,
            }
        )

    is_valid = total <= limit

    if not is_valid:
        print(
            f"SLOT BUDGET ERROR: tier={tier!r} allows {limit} slots, "
            f"but {total} slots requested across {len(pads)} pads.",
            file=sys.stderr,
        )
        print("  Per-pad breakdown:", file=sys.stderr)
        for d in details:
            print(
                f"    voice={d['voice']!r:12s}  vel_layers={d['vel_layers']}  "
                f"rr={d['rr']}  slots={d['slots']}",
                file=sys.stderr,
            )
        print(
            f"  => Reduce pad count, choose a higher tier, or "
            f"reduce RR counts to fit within {limit} slots.",
            file=sys.stderr,
        )

    return is_valid, total, limit, details


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
    note_label = f"  <!-- {xml_escape(note_name).replace('--', '- -')} -->" if note_name else ""

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


def _active_drum_layer_xml(
    sample_path: str,
    layer_num: int,
    vel_start: int,
    vel_end: int,
    rr_index: int = 0,
    cycle_group: int = 0,
) -> str:
    """Single active sample layer for a drum program (OneShot=True, KeyTrack=False).

    Args:
        sample_path: Relative path to the WAV file.
        layer_num: XPM layer number (1-indexed).
        vel_start: VelStart value.
        vel_end: VelEnd value.
        rr_index: Round-robin variant index (0 = base, no RR tags added).
        cycle_group: CycleGroup value for round-robin grouping (velocity zone index).
    """
    sample_file = Path(sample_path).name
    sample_name = Path(sample_path).stem

    rr_xml = ""
    if rr_index > 0:
        rr_xml = (
            f'              <CycleType>RoundRobin</CycleType>\n'
            f'              <CycleGroup>{cycle_group}</CycleGroup>\n'
        )

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
        f'              <KeyTrack>False</KeyTrack>\n'
        f'              <OneShot>True</OneShot>\n'
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
        f'{rr_xml}'
        f'            </Layer>\n'
    )


def _instrument_block_drum_layered(
    instrument_num: int,
    voice_name: str,
    samples: list,
    tier: str,
    choke_group: int = 0,
) -> str:
    """Build one <Instrument> block for a multi-layer drum pad.

    Args:
        instrument_num: XPM instrument slot (0-indexed).
        voice_name: Engine-internal voice name (e.g., "kick", "snare").
        samples: List of sample dicts from the pad spec, each with:
            - "file": str — relative WAV path
            - "vel_layer": int — velocity layer index (0–3)
            - "rr_index": int — round-robin variant (0 = base, 1+ = RR)
        tier: "SURFACE", "DEEP", or "TRENCH" (controls RR emission).
        choke_group: MuteGroup value (0 = no choke).

    Returns:
        XML string for the complete <Instrument> block.
    """
    vel_start_fn, vel_end_fn, num_layers = _get_velocity_standard()
    _, display_label_fn, _ = _get_voice_taxonomy()

    instr_name = display_label_fn(voice_name)

    if not samples:
        # Ghost-trigger prevention: empty pad
        return (
            f'      <Instrument number="{instrument_num}">\n'
            f'        <Active>False</Active>\n'
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
            f'        <LowNote>{instrument_num}</LowNote>\n'
            f'        <HighNote>{instrument_num}</HighNote>\n'
            f'        <RootNote>0</RootNote>\n'
            f'        <KeyTrack>False</KeyTrack>\n'
            f'        <OneShot>True</OneShot>\n'
            f'        <Layers>\n'
            f'          <Layer number="1">\n'
            f'            <Active>False</Active>\n'
            f'            <SampleName></SampleName>\n'
            f'            <SampleFile></SampleFile>\n'
            f'            <File></File>\n'
            f'            <VelStart>0</VelStart>\n'
            f'            <VelEnd>0</VelEnd>\n'
            f'          </Layer>\n'
            f'        </Layers>\n'
            f'      </Instrument>\n'
        )

    # Group samples by velocity layer, then by rr_index within each layer
    # Structure: {vel_layer_idx: [(rr_index, file), ...]}
    layers_by_vel: Dict[int, List[Tuple[int, str]]] = {}
    for s in samples:
        vl = int(s.get("vel_layer", 0))
        ri = int(s.get("rr_index", 0))
        layers_by_vel.setdefault(vl, []).append((ri, s["file"]))

    # Sort RR variants within each velocity layer
    for vl in layers_by_vel:
        layers_by_vel[vl].sort(key=lambda x: x[0])

    # Build layer XML — iterate velocity layers in order, emit RR variants per layer
    layers_xml = ""
    layer_num = 1
    use_rr = (tier.upper() == "TRENCH")

    for vl_idx in range(num_layers):
        vs = vel_start_fn(vl_idx)
        ve = vel_end_fn(vl_idx)
        variants = layers_by_vel.get(vl_idx, [])

        if not variants:
            # Missing sample for this velocity zone — emit inactive placeholder
            layers_xml += (
                f'            <Layer number="{layer_num}">\n'
                f'              <Active>False</Active>\n'
                f'              <SampleName></SampleName>\n'
                f'              <SampleFile></SampleFile>\n'
                f'              <File></File>\n'
                f'              <VelStart>0</VelStart>\n'
                f'              <VelEnd>0</VelEnd>\n'
                f'            </Layer>\n'
            )
            layer_num += 1
        else:
            for rr_idx, (ri, file_path) in enumerate(variants):
                # cycle_group = velocity layer index (so MPC groups RR within zone)
                emit_rr = use_rr and ri > 0
                layers_xml += _active_drum_layer_xml(
                    sample_path=file_path,
                    layer_num=layer_num,
                    vel_start=vs,
                    vel_end=ve,
                    rr_index=ri if emit_rr else 0,
                    cycle_group=vl_idx if emit_rr else 0,
                )
                layer_num += 1

    choke_xml = (
        f"        <MuteGroup>{choke_group}</MuteGroup>\n"
        if choke_group
        else ""
    )
    note_comment = f"  <!-- {xml_escape(instr_name).replace('--', '- -')} -->"

    return (
        f'      <Instrument number="{instrument_num}">{note_comment}\n'
        f'        <Active>True</Active>\n'
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
        f'        <LowNote>{instrument_num}</LowNote>\n'
        f'        <HighNote>{instrument_num}</HighNote>\n'
        f'        <RootNote>0</RootNote>\n'
        f'        <KeyTrack>False</KeyTrack>\n'
        f'        <OneShot>True</OneShot>\n'
        f'{choke_xml}'
        f'        <Layers>\n'
        f'{layers_xml}'
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
# Build: DrumProgram — multi-layer (DEEP / TRENCH)
# ---------------------------------------------------------------------------

def build_drum_program_layered(
    program_name: str,
    pads: list,
    tier: str = "DEEP",
    output_dir: Optional[str] = None,
) -> str:
    """Build a multi-velocity-layer drum program XPM.

    This is the DEEP/TRENCH builder. SURFACE callers should use
    build_drum_program() instead.

    Args:
        program_name: Program display name.
        pads: List of pad dicts, each with:
            - "voice": str — engine-internal voice name (e.g., "kick", "snare")
            - "samples": list of dicts, each with:
                - "file": str — relative WAV path
                - "vel_layer": int — velocity layer index 0–3
                - "rr_index": int — round-robin variant (0 = base, 1+ = RR)
            - "choke": int (optional) — MuteGroup number
            - "pad": int (optional) — explicit pad slot (1-indexed); defaults to
              position in the pads list + 1
        tier: "SURFACE", "DEEP", or "TRENCH".
        output_dir: Not used by this function (reserved for future file writing).
            Returns the XPM XML string; callers write the file.

    Returns:
        str — XPM XML content (not yet written to disk).

    Raises:
        ValueError: If slot budget validation fails (hard gate).
    """
    tier = tier.upper()

    # Hard gate: validate slot budget before building
    is_valid, total_slots, limit, _details = validate_slot_budget(pads, tier)
    if not is_valid:
        raise ValueError(
            f"Slot budget exceeded for tier {tier!r}: "
            f"{total_slots} slots > {limit} limit. "
            f"Aborting build for program {program_name!r}."
        )

    # Build a slot→XML map (0-indexed) for all 128 instrument slots.
    # If pad dict has "pad" key (1-indexed), use that; otherwise use list position.
    all_instrument_blocks: Dict[int, str] = {}

    for pos, pad_cfg in enumerate(pads):
        slot = (pad_cfg.get("pad", pos + 1) - 1)  # convert 1-indexed → 0-indexed
        slot = max(0, min(slot, DRUM_INSTRUMENTS_TOTAL - 1))
        voice = pad_cfg.get("voice", f"pad{pos+1}")
        samples = pad_cfg.get("samples", [])
        choke = pad_cfg.get("choke", 0)
        all_instrument_blocks[slot] = _instrument_block_drum_layered(
            instrument_num=slot,
            voice_name=voice,
            samples=samples,
            tier=tier,
            choke_group=choke,
        )

    # Fill remaining 128 slots with empty (inactive) instruments
    for slot in range(DRUM_INSTRUMENTS_TOTAL):
        if slot not in all_instrument_blocks:
            all_instrument_blocks[slot] = _instrument_block_drum_layered(
                instrument_num=slot,
                voice_name="",
                samples=[],
                tier=tier,
                choke_group=0,
            )

    instruments_xml = "".join(
        all_instrument_blocks[s] for s in range(DRUM_INSTRUMENTS_TOTAL)
    )

    # PadNoteMap: pads 1-16 → instrument indices 0-15 (standard MPC mapping)
    pad_note_entries = "\n".join(
        f'        <Pad number="{p}" note="{p - 1}"/>'
        for p in range(1, 17)
    )

    return (
        _xpm_header(program_name, "Drum")
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


def _dispatch_program_from_spec(prog: dict) -> str:
    """Internal: dispatch to the correct builder by type field in a batch spec dict."""
    prog_type = prog.get("type", "")
    builder = BUILDERS.get(prog_type)
    if builder is None:
        raise ValueError(f"Unknown program type: {prog_type!r}")
    return builder(prog)


def build_program(
    program_name: str,
    pads: list,
    tier: str = "DEEP",
    **kwargs,
) -> str:
    """Route to the correct drum program builder based on tier.

    SURFACE tier uses build_drum_program() (single layer, legacy format).
    DEEP and TRENCH tiers use build_drum_program_layered() (multi-layer + RR).

    Args:
        program_name: Program display name.
        pads: List of pad dicts.
            For SURFACE: legacy format with "pad", "sample", "name", "choke" keys.
            For DEEP/TRENCH: new format with "voice", "samples", "choke", "pad" keys.
        tier: "SURFACE", "DEEP", or "TRENCH".
        **kwargs: Reserved for future options (e.g., output_dir).

    Returns:
        str — XPM XML content.
    """
    tier = tier.upper()
    if tier == "SURFACE":
        # Adapt new-style pads to the legacy format if needed, then delegate
        prog_spec = {"name": program_name, "type": "DrumProgram", "pads": pads}
        return build_drum_program(prog_spec)
    else:
        return build_drum_program_layered(program_name, pads, tier=tier, **kwargs)


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
            xpm_xml = _dispatch_program_from_spec(prog)
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
