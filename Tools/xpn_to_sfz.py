#!/usr/bin/env python3
"""
xpn_to_sfz.py — XPN Keygroup → SFZ Converter
XO_OX Designs

Converts an Akai MPC XPN pack file (ZIP containing XPM + WAV samples) into an
SFZ instrument file compatible with Kontakt (SFZ import), Decent Sampler,
sforzando, HISE, and any other SFZ 1.0/2.0-capable sampler.

Usage:
    python xpn_to_sfz.py --xpn pack.xpn --program "Preset Name" --output ./sfz/
    python xpn_to_sfz.py --xpn pack.xpn --output ./sfz/          # uses first program found
    python xpn_to_sfz.py --xpn pack.xpn --list-programs           # list available programs

Output layout:
    ./sfz/
        PresetName.sfz
        samples/
            sample_001.wav
            sample_002.wav
            ...

SFZ mapping:
    XPM LowNote   → lokey
    XPM HighNote  → hikey
    XPM RootNote  → pitch_keycenter  (0 = MPC auto-detect → fallback MIDI 60)
    XPM VelStart  → lovel
    XPM VelEnd    → hivel
    XPM Volume    → volume (linear → dB)
    XPM TuneCoarse + TuneFine → tune (semitones + cents combined into cents)
    XPM VolumeAttack  → ampeg_attack
    XPM VolumeDecay   → ampeg_decay
    XPM VolumeSustain → ampeg_sustain (linear → dB for SFZ)
    XPM VolumeRelease → ampeg_release
    XPM CycleGroup + CycleType=2 → seq_length / seq_position (round-robin)
"""

import argparse
import math
import os
import sys
import zipfile
import xml.etree.ElementTree as ET
from collections import defaultdict
from pathlib import Path
from typing import Dict, List, Optional, Tuple


# ---------------------------------------------------------------------------
# Constants
# ---------------------------------------------------------------------------

CYCLE_TYPE_ROUNDROBIN = 2   # XPM CycleType value for round-robin playback
FALLBACK_ROOT_MIDI    = 60  # C4 — used when RootNote=0 (MPC auto-detect)
DEFAULT_VOLUME_DB     = 0.0
DEFAULT_TUNE_CENTS    = 0
DEFAULT_ATTACK        = 0.001   # seconds — SFZ minimum
DEFAULT_DECAY         = 0.0
DEFAULT_SUSTAIN_DB    = 0.0     # dB (full sustain)
DEFAULT_RELEASE       = 0.1


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def linear_to_db(linear: float) -> float:
    """Convert linear amplitude (0.0–1.0+) to dB. Clamps floor at -144 dB."""
    if linear <= 0.0:
        return -144.0
    return 20.0 * math.log10(linear)


def xpm_volume_to_db(xpm_vol: float) -> float:
    """
    XPM Volume is a linear amplitude in range ~0.0–1.0 (sometimes up to ~1.41).
    Convert to dB for SFZ volume= opcode.
    """
    return round(linear_to_db(xpm_vol), 2)


def xpm_sustain_to_db(xpm_sustain: float) -> float:
    """
    XPM VolumeSustain is linear 0.0–1.0.
    SFZ ampeg_sustain is in dB (0 = full, -144 = silence).
    """
    return round(linear_to_db(xpm_sustain) if xpm_sustain > 0 else -144.0, 2)


def xpm_tune_to_cents(coarse: int, fine: int) -> int:
    """
    XPM TuneCoarse is in semitones (-36..+36), TuneFine in cents (-100..+100).
    SFZ tune= is in cents.
    """
    return coarse * 100 + fine


def clamp_attack(seconds: float) -> float:
    """SFZ ampeg_attack minimum is 0.001 s to avoid clicks."""
    return max(seconds, 0.001)


def safe_float(el: Optional[ET.Element], default: float) -> float:
    if el is None or not el.text:
        return default
    try:
        return float(el.text.strip())
    except ValueError:
        return default


def safe_int(el: Optional[ET.Element], default: int) -> int:
    if el is None or not el.text:
        return default
    try:
        return int(el.text.strip())
    except ValueError:
        return default


def safe_str(el: Optional[ET.Element], default: str = "") -> str:
    if el is None or not el.text:
        return default
    return el.text.strip()


# ---------------------------------------------------------------------------
# XPN / ZIP inspection
# ---------------------------------------------------------------------------

def list_xpn_programs(xpn_path: Path) -> List[str]:
    """Return the names of all Keygroup programs inside an XPN."""
    programs: List[str] = []
    with zipfile.ZipFile(xpn_path, "r") as zf:
        for name in zf.namelist():
            if name.endswith(".xpm"):
                try:
                    with zf.open(name) as f:
                        tree = ET.parse(f)
                        root = tree.getroot()
                    prog_el = root.find(".//Program")
                    if prog_el is None:
                        prog_el = root  # some XPMs wrap at root
                    prog_type = prog_el.get("type", "")
                    if prog_type.lower() == "keygroup":
                        name_el = prog_el.find("ProgramName")
                        pname = safe_str(name_el, Path(name).stem)
                        programs.append(pname)
                except ET.ParseError as exc:
                    print(f"[WARN] Parsing XPM file {name} for keygroup program list: {exc}", file=sys.stderr)
    return programs


def find_xpm_in_zip(zf: zipfile.ZipFile, program_name: Optional[str]) -> Optional[Tuple[str, ET.Element]]:
    """
    Find and parse the XPM file for the given program name.
    If program_name is None, returns the first Keygroup XPM found.
    Returns (xpm_zip_path, program_element) or None.
    """
    for entry in zf.namelist():
        if not entry.endswith(".xpm"):
            continue
        try:
            with zf.open(entry) as f:
                tree = ET.parse(f)
                xml_root = tree.getroot()
        except ET.ParseError:
            continue

        prog_el = xml_root.find(".//Program")
        if prog_el is None:
            # Some XPMs have Program as the root element
            if xml_root.tag == "MPCVObject":
                prog_el = xml_root.find("Program")
            if prog_el is None:
                prog_el = xml_root if xml_root.tag == "Program" else None
        if prog_el is None:
            continue

        prog_type = prog_el.get("type", "")
        if prog_type.lower() != "keygroup":
            continue

        name_el = prog_el.find("ProgramName")
        pname = safe_str(name_el, Path(entry).stem)

        if program_name is None or pname == program_name:
            return (entry, prog_el)

    return None


# ---------------------------------------------------------------------------
# XPM → structured data
# ---------------------------------------------------------------------------

class LayerData:
    """One layer (velocity split or round-robin take) within a keyzone."""
    __slots__ = (
        "sample_file", "sample_name",
        "vel_start", "vel_end",
        "volume_db", "tune_cents",
        "root_note",
        "attack", "decay", "sustain_db", "release",
        "cycle_type", "cycle_group",
        "active",
    )

    def __init__(self) -> None:
        self.sample_file: str = ""
        self.sample_name: str = ""
        self.vel_start: int = 1
        self.vel_end: int = 127
        self.volume_db: float = 0.0
        self.tune_cents: int = 0
        self.root_note: int = 0        # 0 = MPC auto-detect
        self.attack: float = 0.001
        self.decay: float = 0.0
        self.sustain_db: float = 0.0
        self.release: float = 0.1
        self.cycle_type: int = 0
        self.cycle_group: int = 0
        self.active: bool = True


class ZoneData:
    """One instrument zone (keygroup range) in an XPM program."""
    __slots__ = (
        "low_note", "high_note", "root_note",
        "volume_db", "tune_cents",
        "attack", "decay", "sustain_db", "release",
        "layers",
    )

    def __init__(self) -> None:
        self.low_note: int = 0
        self.high_note: int = 127
        self.root_note: int = 0
        self.volume_db: float = 0.0
        self.tune_cents: int = 0
        self.attack: float = 0.001
        self.decay: float = 0.0
        self.sustain_db: float = 0.0
        self.release: float = 0.1
        self.layers: List[LayerData] = []


def parse_program(prog_el: ET.Element) -> Tuple[dict, List[ZoneData]]:
    """
    Parse a <Program type="Keygroup"> element into global params + zone list.
    Returns (global_dict, zones).
    """
    # --- Global program-level params ---
    global_params = {
        "volume_db": xpm_volume_to_db(safe_float(prog_el.find("Volume"), 1.0)),
        "pan":       safe_float(prog_el.find("Pan"), 0.5),   # 0.0–1.0 → map to -100..+100
        "tune_cents": xpm_tune_to_cents(
            safe_int(prog_el.find("TuneCoarse"), 0),
            safe_int(prog_el.find("TuneFine"), 0),
        ),
    }

    zones: List[ZoneData] = []
    instruments_el = prog_el.find("Instruments")
    if instruments_el is None:
        return global_params, zones

    for inst_el in instruments_el.findall("Instrument"):
        zone = ZoneData()

        zone.low_note  = safe_int(inst_el.find("LowNote"), 0)
        zone.high_note = safe_int(inst_el.find("HighNote"), 127)
        zone.root_note = safe_int(inst_el.find("RootNote"), 0)

        zone.volume_db  = xpm_volume_to_db(safe_float(inst_el.find("Volume"), 1.0))
        zone.tune_cents = xpm_tune_to_cents(
            safe_int(inst_el.find("TuneCoarse"), 0),
            safe_int(inst_el.find("TuneFine"), 0),
        )

        # Instrument-level envelope (used as defaults for layers)
        zone.attack     = clamp_attack(safe_float(inst_el.find("VolumeAttack"), 0.0))
        zone.decay      = safe_float(inst_el.find("VolumeDecay"), 0.0)
        zone.sustain_db = xpm_sustain_to_db(safe_float(inst_el.find("VolumeSustain"), 1.0))
        zone.release    = max(safe_float(inst_el.find("VolumeRelease"), 0.1), 0.001)

        # Layers
        layers_el = inst_el.find("Layers")
        if layers_el is not None:
            for layer_el in layers_el.findall("Layer"):
                active_text = safe_str(layer_el.find("Active"), "True")
                if active_text.lower() == "false":
                    # Skip inactive placeholder layers
                    vel_start = safe_int(layer_el.find("VelStart"), 0)
                    vel_end   = safe_int(layer_el.find("VelEnd"), 0)
                    if vel_start == 0 and vel_end == 0:
                        continue  # Rex Rule #3 empty layer

                layer = LayerData()

                # Sample reference — prefer <File> (full relative path), fall back to <SampleFile>
                file_text = safe_str(layer_el.find("File"), "")
                sample_file_text = safe_str(layer_el.find("SampleFile"), "")
                layer.sample_file = file_text if file_text else sample_file_text
                layer.sample_name = safe_str(layer_el.find("SampleName"), "")

                layer.vel_start = safe_int(layer_el.find("VelStart"), 1)
                layer.vel_end   = safe_int(layer_el.find("VelEnd"), 127)

                layer.volume_db  = xpm_volume_to_db(safe_float(layer_el.find("Volume"), 0.707946))
                layer.tune_cents = xpm_tune_to_cents(
                    safe_int(layer_el.find("TuneCoarse"), 0),
                    safe_int(layer_el.find("TuneFine"), 0),
                )

                layer.root_note  = safe_int(layer_el.find("RootNote"), zone.root_note)

                # Layer-level envelope overrides (fall back to zone defaults)
                layer.attack     = clamp_attack(
                    safe_float(layer_el.find("VolumeAttack"), zone.attack))
                layer.decay      = safe_float(layer_el.find("VolumeDecay"), zone.decay)
                layer.sustain_db = xpm_sustain_to_db(
                    safe_float(layer_el.find("VolumeSustain"), 1.0))
                layer.release    = max(
                    safe_float(layer_el.find("VolumeRelease"), zone.release), 0.001)

                layer.cycle_type  = safe_int(layer_el.find("CycleType"), 0)
                layer.cycle_group = safe_int(layer_el.find("CycleGroup"), 0)
                layer.active      = active_text.lower() != "false"

                if layer.sample_file or layer.sample_name:
                    zone.layers.append(layer)

        if zone.layers:
            zones.append(zone)

    return global_params, zones


# ---------------------------------------------------------------------------
# Round-robin grouping
# ---------------------------------------------------------------------------

def compute_rr_groups(
    zones: List[ZoneData],
) -> Dict[Tuple[int, int], Dict[int, List[Tuple[int, int]]]]:
    """
    For each (low_note, high_note) zone, collect layers that share a CycleGroup
    and have CycleType == CYCLE_TYPE_ROUNDROBIN.

    Returns:
        { (low_note, high_note): { cycle_group: [layer_indices_in_zone...] } }
    """
    rr: Dict[Tuple[int, int], Dict[int, List[int]]] = {}
    for z_idx, zone in enumerate(zones):
        key = (zone.low_note, zone.high_note)
        for l_idx, layer in enumerate(zone.layers):
            if layer.cycle_type == CYCLE_TYPE_ROUNDROBIN and layer.cycle_group > 0:
                rr.setdefault(key, defaultdict(list))[layer.cycle_group].append(l_idx)
    return rr


# ---------------------------------------------------------------------------
# SFZ generation
# ---------------------------------------------------------------------------

def sfz_note(midi: int) -> int:
    """SFZ accepts raw MIDI note numbers (0–127). Return as-is."""
    return midi


def generate_sfz(
    global_params: dict,
    zones: List[ZoneData],
    samples_rel_dir: str = "samples",
    sfz_path: Optional[Path] = None,
) -> str:
    """
    Build and return the full SFZ text for the parsed program data.

    samples_rel_dir: path to samples directory relative to the .sfz file.
    """
    lines: List[str] = []

    # Header comment
    sfz_name = sfz_path.stem if sfz_path else "XPN Export"
    lines.append(f"// {sfz_name}")
    lines.append("// Generated by xpn_to_sfz.py — XO_OX Designs")
    lines.append("")

    # <global> section
    lines.append("<global>")
    lines.append(f"volume={global_params['volume_db']}")
    # Pan: XPM 0.5 = center; map to SFZ pan= -100..+100
    pan_sfz = round((global_params["pan"] - 0.5) * 200.0, 1)
    if pan_sfz != 0.0:
        lines.append(f"pan={pan_sfz}")
    if global_params["tune_cents"] != 0:
        lines.append(f"tune={global_params['tune_cents']}")
    lines.append("")

    # Group round-robin info: {(low, high): {cg: [layer_idx, ...]}}
    rr_groups = compute_rr_groups(zones)

    # Track seq_length / seq_position per (low, high, cycle_group)
    # We'll compute positions as we emit regions
    rr_positions: Dict[Tuple[int, int, int], int] = defaultdict(int)
    rr_lengths: Dict[Tuple[int, int, int], int] = {}
    for (lo, hi), cg_map in rr_groups.items():
        for cg, indices in cg_map.items():
            rr_lengths[(lo, hi, cg)] = len(indices)

    # Group layers by velocity range → one <group> per unique vel band
    # Collect all unique vel bands across all zones
    vel_bands: List[Tuple[int, int]] = []
    seen_bands = set()
    for zone in zones:
        for layer in zone.layers:
            band = (layer.vel_start, layer.vel_end)
            if band not in seen_bands:
                seen_bands.add(band)
                vel_bands.append(band)
    vel_bands.sort()

    if vel_bands:
        for (lovel, hivel) in vel_bands:
            group_has_regions = False
            group_lines: List[str] = []

            for zone in zones:
                for layer in zone.layers:
                    if (layer.vel_start, layer.vel_end) != (lovel, hivel):
                        continue

                    # Determine sample path relative to SFZ
                    raw_path = layer.sample_file or layer.sample_name
                    if not raw_path:
                        continue

                    # Normalise path: strip leading "Samples/<slug>/" prefix if present,
                    # then place under our samples_rel_dir.
                    parts = Path(raw_path.replace("\\", "/")).parts
                    # Drop "Samples" prefix directory if present
                    if parts and parts[0].lower() == "samples":
                        parts = parts[1:]
                    # If there's a slug sub-directory, keep it (preserves sample namespacing)
                    rel_sample = "/".join(parts) if parts else Path(raw_path).name
                    sfz_sample_path = f"{samples_rel_dir}/{rel_sample}"

                    # pitch_keycenter: RootNote=0 means auto-detect → fallback MIDI 60
                    root_midi = layer.root_note if layer.root_note != 0 else (
                        zone.root_note if zone.root_note != 0 else FALLBACK_ROOT_MIDI
                    )

                    # Round-robin seq info
                    rr_key = (zone.low_note, zone.high_note, layer.cycle_group)
                    is_rr = (
                        layer.cycle_type == CYCLE_TYPE_ROUNDROBIN
                        and layer.cycle_group > 0
                        and rr_key in rr_lengths
                    )

                    region_lines: List[str] = []
                    region_lines.append("<region>")
                    region_lines.append(f"sample={sfz_sample_path}")
                    region_lines.append(f"lokey={sfz_note(zone.low_note)}")
                    region_lines.append(f"hikey={sfz_note(zone.high_note)}")
                    region_lines.append(f"pitch_keycenter={sfz_note(root_midi)}")
                    region_lines.append(f"lovel={layer.vel_start}")
                    region_lines.append(f"hivel={layer.vel_end}")

                    if layer.volume_db != 0.0:
                        region_lines.append(f"volume={layer.volume_db}")
                    if layer.tune_cents != 0:
                        region_lines.append(f"tune={layer.tune_cents}")

                    region_lines.append(f"ampeg_attack={layer.attack:.4f}")
                    region_lines.append(f"ampeg_decay={layer.decay:.4f}")
                    region_lines.append(f"ampeg_sustain={layer.sustain_db:.2f}")
                    region_lines.append(f"ampeg_release={layer.release:.4f}")

                    if is_rr:
                        seq_len = rr_lengths[rr_key]
                        rr_positions[rr_key] += 1
                        seq_pos = rr_positions[rr_key]
                        region_lines.append(f"seq_length={seq_len}")
                        region_lines.append(f"seq_position={seq_pos}")

                    region_lines.append("")
                    group_lines.extend(region_lines)
                    group_has_regions = True

            if group_has_regions:
                lines.append("<group>")
                lines.append(f"lovel={lovel}")
                lines.append(f"hivel={hivel}")
                lines.append("")
                lines.extend(group_lines)
    else:
        # No velocity grouping — emit flat regions
        for zone in zones:
            for layer in zone.layers:
                raw_path = layer.sample_file or layer.sample_name
                if not raw_path:
                    continue
                parts = Path(raw_path.replace("\\", "/")).parts
                if parts and parts[0].lower() == "samples":
                    parts = parts[1:]
                rel_sample = "/".join(parts) if parts else Path(raw_path).name
                sfz_sample_path = f"{samples_rel_dir}/{rel_sample}"

                root_midi = layer.root_note if layer.root_note != 0 else (
                    zone.root_note if zone.root_note != 0 else FALLBACK_ROOT_MIDI
                )

                lines.append("<region>")
                lines.append(f"sample={sfz_sample_path}")
                lines.append(f"lokey={sfz_note(zone.low_note)}")
                lines.append(f"hikey={sfz_note(zone.high_note)}")
                lines.append(f"pitch_keycenter={sfz_note(root_midi)}")
                lines.append(f"lovel={layer.vel_start}")
                lines.append(f"hivel={layer.vel_end}")
                if layer.volume_db != 0.0:
                    lines.append(f"volume={layer.volume_db}")
                if layer.tune_cents != 0:
                    lines.append(f"tune={layer.tune_cents}")
                lines.append(f"ampeg_attack={layer.attack:.4f}")
                lines.append(f"ampeg_decay={layer.decay:.4f}")
                lines.append(f"ampeg_sustain={layer.sustain_db:.2f}")
                lines.append(f"ampeg_release={layer.release:.4f}")
                lines.append("")

    return "\n".join(lines)


# ---------------------------------------------------------------------------
# Sample extraction
# ---------------------------------------------------------------------------

def extract_samples(
    zf: zipfile.ZipFile,
    zones: List[ZoneData],
    samples_out_dir: Path,
) -> int:
    """
    Extract WAV files referenced by the parsed zones into samples_out_dir.
    Returns count of extracted files.
    """
    samples_out_dir.mkdir(parents=True, exist_ok=True)

    # Build set of filenames referenced in layers
    referenced: set = set()
    for zone in zones:
        for layer in zone.layers:
            if layer.sample_file:
                referenced.add(layer.sample_file.replace("\\", "/"))
            if layer.sample_name:
                referenced.add(layer.sample_name)

    zip_entries = {e: e for e in zf.namelist()}
    # Also build a basename→entry map for fallback matching
    basename_map: Dict[str, str] = {}
    for entry in zf.namelist():
        basename_map[Path(entry).name.lower()] = entry

    extracted = 0
    for zone in zones:
        for layer in zone.layers:
            raw_path = layer.sample_file or layer.sample_name
            if not raw_path:
                continue

            norm = raw_path.replace("\\", "/")

            # Try direct match first
            zip_entry = zip_entries.get(norm)
            if zip_entry is None:
                # Try basename fallback
                zip_entry = basename_map.get(Path(norm).name.lower())

            if zip_entry is None:
                print(f"  [warn] sample not found in XPN: {raw_path}", file=sys.stderr)
                continue

            # Compute destination path: preserve sub-directory under samples/
            parts = Path(norm).parts
            if parts and parts[0].lower() == "samples":
                parts = parts[1:]
            rel_dest = Path(*parts) if len(parts) > 1 else Path(parts[0])
            dest = samples_out_dir / rel_dest
            dest.parent.mkdir(parents=True, exist_ok=True)

            if not dest.exists():
                with zf.open(zip_entry) as src, open(dest, "wb") as dst:
                    dst.write(src.read())
                extracted += 1

    return extracted


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def build_sfz_filename(program_name: str) -> str:
    """Sanitise program name for use as a filename."""
    safe = "".join(c if (c.isalnum() or c in " _-") else "_" for c in program_name)
    return safe.strip().replace(" ", "_") + ".sfz"


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Convert an XPN keygroup program to SFZ format.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    parser.add_argument("--xpn", required=True, metavar="FILE",
                        help="Path to the .xpn pack file")
    parser.add_argument("--program", metavar="NAME", default=None,
                        help="Program name to export (default: first Keygroup found)")
    parser.add_argument("--output", metavar="DIR", default=".",
                        help="Output directory for .sfz and samples/ (default: .)")
    parser.add_argument("--list-programs", action="store_true",
                        help="List all Keygroup programs in the XPN and exit")
    args = parser.parse_args()

    xpn_path = Path(args.xpn)
    if not xpn_path.exists():
        print(f"error: file not found: {xpn_path}", file=sys.stderr)
        sys.exit(1)

    if not zipfile.is_zipfile(xpn_path):
        print(f"error: not a valid ZIP/XPN file: {xpn_path}", file=sys.stderr)
        sys.exit(1)

    # --list-programs
    if args.list_programs:
        programs = list_xpn_programs(xpn_path)
        if not programs:
            print("No Keygroup programs found in XPN.")
        else:
            print(f"Keygroup programs in {xpn_path.name}:")
            for p in programs:
                print(f"  {p}")
        sys.exit(0)

    output_dir = Path(args.output)
    output_dir.mkdir(parents=True, exist_ok=True)

    with zipfile.ZipFile(xpn_path, "r") as zf:
        result = find_xpm_in_zip(zf, args.program)
        if result is None:
            if args.program:
                print(f"error: program '{args.program}' not found in {xpn_path.name}",
                      file=sys.stderr)
                print("Use --list-programs to see available programs.", file=sys.stderr)
            else:
                print(f"error: no Keygroup program found in {xpn_path.name}", file=sys.stderr)
            sys.exit(1)

        xpm_zip_path, prog_el = result
        name_el = prog_el.find("ProgramName")
        program_name = safe_str(name_el, Path(xpm_zip_path).stem)
        print(f"Converting program: {program_name}")

        # Parse XPM
        global_params, zones = parse_program(prog_el)
        print(f"  Zones parsed: {len(zones)}")
        total_layers = sum(len(z.layers) for z in zones)
        print(f"  Layers parsed: {total_layers}")

        # Determine output paths
        sfz_filename = build_sfz_filename(program_name)
        sfz_out_path = output_dir / sfz_filename
        samples_out_dir = output_dir / "samples"

        # Extract samples
        print(f"  Extracting samples → {samples_out_dir}/")
        n_extracted = extract_samples(zf, zones, samples_out_dir)
        print(f"  Extracted: {n_extracted} WAV files")

    # Generate SFZ
    sfz_text = generate_sfz(global_params, zones, "samples", sfz_path=sfz_out_path)

    with open(sfz_out_path, "w", encoding="utf-8") as f:
        f.write(sfz_text)

    print(f"  Written: {sfz_out_path}")
    print("Done.")


if __name__ == "__main__":
    main()
