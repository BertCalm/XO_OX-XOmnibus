#!/usr/bin/env python3
"""
XPN Velocity Map Visualizer — XO_OX Designs
Reads an Akai MPC XPM XML file and renders its velocity layer structure
as an ASCII chart in the terminal.

Keygroup programs: Y axis = MIDI notes 0–127, labeled every 12 semitones.
Drum programs:     Y axis = 16 pads (auto-detected from <Pad> structure).

ASCII columns represent velocity 0–127 mapped to 64 characters (2 vel units each):
  █  = layer coverage (exactly one layer claims this velocity range)
  ░  = overlap (two or more layers claim the same velocity unit)
  .  = gap (no layer covers this velocity unit)

ANSI color (disabled by NO_COLOR env var or --no-color flag):
  cyan   (\\033[36m): layers where sample name contains "fx" or "bright"
                      OR root note above MIDI 72
  orange (\\033[33m): layers with root note below MIDI 48
  default:            everything else

Summary after chart:
  - Total layer count
  - WARNING if any overlap detected
  - WARNING if any gap >= 5 velocity units

Usage:
    python xpn_velocity_map_visualizer.py path/to/program.xpm [--no-color] [--strict]

    --no-color   Disable ANSI color output
    --strict     Exit code 1 if any overlap or gap >= 5 velocity units found
"""

import argparse
import os
import sys
from dataclasses import dataclass, field
from pathlib import Path
from typing import Optional
from xml.etree import ElementTree as ET


# ---------------------------------------------------------------------------
# MIDI helpers
# ---------------------------------------------------------------------------

NOTE_NAMES = ["C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"]


def midi_to_name(midi: int) -> str:
    """Return e.g. 'C4' for MIDI note 60."""
    octave = (midi // 12) - 2
    name = NOTE_NAMES[midi % 12]
    return f"{name}{octave}"


# ---------------------------------------------------------------------------
# Data model
# ---------------------------------------------------------------------------

@dataclass
class VelocityLayer:
    vel_start: int
    vel_end: int
    sample_file: str
    root_note: int = 60  # default if not specified


@dataclass
class Instrument:
    number: int
    root_note: int
    low_note: int
    high_note: int
    layers: list[VelocityLayer] = field(default_factory=list)


@dataclass
class DrumPad:
    number: int      # 1-based pad number
    note: int        # MIDI note the pad triggers
    layers: list[VelocityLayer] = field(default_factory=list)


# ---------------------------------------------------------------------------
# XPM parser
# ---------------------------------------------------------------------------

def _safe_int(text: Optional[str], default: int = 0) -> int:
    if text is None:
        return default
    try:
        return int(text.strip())
    except ValueError:
        return default


def parse_xpm(path: Path) -> tuple[list[Instrument], list[DrumPad], str]:
    """
    Parse an XPM file.
    Returns (instruments, drum_pads, program_type).
    Exactly one of instruments/drum_pads will be non-empty.
    program_type is 'keygroup' or 'drum'.
    """
    tree = ET.parse(path)
    root = tree.getroot()

    # Detect program type from the Program element's type attribute
    program_el = root.find(".//Program")
    if program_el is None:
        program_el = root  # some XPMs have Program as root
    prog_type_attr = (program_el.get("type") or "").lower()
    is_drum = "drum" in prog_type_attr

    # Build pad note map for drum programs
    pad_note_map: dict[int, int] = {}     # pad_number -> MIDI note
    for pad_el in root.findall(".//PadNoteMap/Pad"):
        num = _safe_int(pad_el.get("number"))
        note = _safe_int(pad_el.get("note"), default=36)
        if num:
            pad_note_map[num] = note

    instruments: list[Instrument] = []
    drum_pads: list[DrumPad] = []

    for inst_el in root.findall(".//Instruments/Instrument"):
        inst_num = _safe_int(inst_el.get("number"))
        root_note = _safe_int(inst_el.findtext("RootNote"), default=60)
        low_note  = _safe_int(inst_el.findtext("LowNote"),  default=0)
        high_note = _safe_int(inst_el.findtext("HighNote"), default=127)

        layers: list[VelocityLayer] = []
        for layer_el in inst_el.findall(".//Layer"):
            vel_start = _safe_int(layer_el.findtext("VelStart"))
            vel_end   = _safe_int(layer_el.findtext("VelEnd"))
            sample_file = (layer_el.findtext("SampleFile") or "").strip()

            # Skip placeholder/empty layers (VelStart=0 AND VelEnd=0 convention)
            if vel_start == 0 and vel_end == 0:
                continue
            # Skip empty sample references unless the range is real
            if not sample_file and vel_start == 0 and vel_end == 0:
                continue

            layers.append(VelocityLayer(
                vel_start=vel_start,
                vel_end=vel_end,
                sample_file=sample_file,
                root_note=root_note,
            ))

        if not layers:
            continue

        if is_drum:
            # Map instrument number to pad note
            note = pad_note_map.get(inst_num, 36 + inst_num - 1)
            drum_pads.append(DrumPad(number=inst_num, note=note, layers=layers))
        else:
            instruments.append(Instrument(
                number=inst_num,
                root_note=root_note,
                low_note=low_note,
                high_note=high_note,
                layers=layers,
            ))

    # If no explicit Program type but we have pad note map data, treat as drum
    if not is_drum and not instruments and drum_pads:
        is_drum = True
    if not is_drum and pad_note_map and not instruments:
        is_drum = True

    prog_type = "drum" if is_drum else "keygroup"
    return instruments, drum_pads, prog_type


# ---------------------------------------------------------------------------
# ANSI color helpers
# ---------------------------------------------------------------------------

CYAN    = "\033[36m"
ORANGE  = "\033[33m"
RESET   = "\033[0m"
BOLD    = "\033[1m"


def color_for_layer(layer: VelocityLayer, use_color: bool) -> str:
    if not use_color:
        return ""
    name_lower = layer.sample_file.lower()
    if "fx" in name_lower or "bright" in name_lower or layer.root_note > 72:
        return CYAN
    if layer.root_note < 48:
        return ORANGE
    return ""


# ---------------------------------------------------------------------------
# ASCII chart rendering
# ---------------------------------------------------------------------------

BAR_WIDTH = 64    # chars; each char = 2 velocity units (128 / 64 = 2)
VEL_UNITS_PER_CHAR = 2


def vel_to_col(vel: int) -> int:
    """Velocity 0–127 mapped to column index 0–63."""
    return min(vel // VEL_UNITS_PER_CHAR, BAR_WIDTH - 1)


def build_coverage_bar(layers: list[VelocityLayer]) -> tuple[str, int, list[tuple[int,int]]]:
    """
    Build a 64-char bar string showing coverage.
    Returns (bar_str, overlap_count, gap_spans).
    gap_spans is list of (gap_start_vel, gap_end_vel) pairs for gaps >= 5 units.
    """
    # Count how many layers cover each of 128 velocity units
    counts = [0] * 128
    for layer in layers:
        for v in range(max(0, layer.vel_start), min(128, layer.vel_end + 1)):
            counts[v] += 1

    # Build bar chars (each char = 2 velocity units, use max of the two)
    bar_chars: list[str] = []
    overlap_total = 0
    for col in range(BAR_WIDTH):
        v0 = col * VEL_UNITS_PER_CHAR
        v1 = v0 + 1
        max_count = max(counts[v0], counts[min(v1, 127)])
        if max_count == 0:
            bar_chars.append(".")
        elif max_count == 1:
            bar_chars.append("█")
        else:
            bar_chars.append("░")
            overlap_total += 1

    # Find gap spans (consecutive uncovered velocity units of length >= 5)
    gap_spans: list[tuple[int, int]] = []
    gap_start: Optional[int] = None
    for v in range(128):
        if counts[v] == 0:
            if gap_start is None:
                gap_start = v
        else:
            if gap_start is not None:
                gap_len = v - gap_start
                if gap_len >= 5:
                    gap_spans.append((gap_start, v - 1))
                gap_start = None
    if gap_start is not None:
        gap_len = 128 - gap_start
        if gap_len >= 5:
            gap_spans.append((gap_start, 127))

    return "".join(bar_chars), overlap_total, gap_spans


def truncate_sample(name: str, width: int = 12) -> str:
    stem = Path(name).stem if name else "(empty)"
    if len(stem) <= width:
        return stem.ljust(width)
    return stem[:width - 1] + "…"


def render_row(
    label: str,
    layers: list[VelocityLayer],
    use_color: bool,
) -> tuple[str, int, list[tuple[int, int]]]:
    """
    Render a single instrument/pad row.
    Returns (rendered_line, overlap_count, gap_spans).
    """
    bar, overlaps, gaps = build_coverage_bar(layers)

    # Colorize the bar by layer (first layer's color for simplicity)
    if use_color and layers:
        col = color_for_layer(layers[0], use_color)
        if col:
            bar = col + bar + RESET

    # Sample name column: show first layer's sample
    first_sample = layers[0].sample_file if layers else ""
    sample_label = truncate_sample(first_sample, 12)

    # Layer count indicator
    layer_count_str = f"[{len(layers)}L]"

    line = f"{label:>6}  {bar}  {sample_label}  {layer_count_str}"
    return line, overlaps, gaps


def print_header(use_color: bool) -> None:
    """Print velocity axis header."""
    # Ruler: 0    16   32   48   64   80   96  112  127
    ruler_labels = "0        16       32       48       64       80       96      112     127"
    tick_line    = "|        |        |        |        |        |        |        |        |"
    if use_color:
        print(f"{'':>6}  {BOLD}{tick_line}{RESET}")
        print(f"{'vel':>6}  {ruler_labels}")
    else:
        print(f"{'':>6}  {tick_line}")
        print(f"{'vel':>6}  {ruler_labels}")
    print()


# ---------------------------------------------------------------------------
# Summary
# ---------------------------------------------------------------------------

def print_summary(
    total_layers: int,
    total_overlaps: int,
    all_gaps: list[tuple[int, int]],
    use_color: bool,
    strict: bool,
) -> int:
    """Print summary. Returns exit code (1 if strict + issues)."""
    print()
    print("─" * 80)
    print(f"  Total layers : {total_layers}")

    has_issue = False

    if total_overlaps > 0:
        has_issue = True
        warn = f"\033[31mWARNING\033[0m" if use_color else "WARNING"
        print(f"  Overlaps     : {warn} — {total_overlaps} velocity-column(s) claimed by 2+ layers")
    else:
        ok = f"\033[32mOK\033[0m" if use_color else "OK"
        print(f"  Overlaps     : {ok} — none")

    sig_gaps = [g for g in all_gaps if (g[1] - g[0] + 1) >= 5]
    if sig_gaps:
        has_issue = True
        warn = f"\033[31mWARNING\033[0m" if use_color else "WARNING"
        gap_strs = ", ".join(f"{a}–{b}" for a, b in sig_gaps[:5])
        print(f"  Gaps (>=5vel): {warn} — vel {gap_strs}")
    else:
        ok = f"\033[32mOK\033[0m" if use_color else "OK"
        print(f"  Gaps (>=5vel): {ok} — none")

    print("─" * 80)

    if strict and has_issue:
        return 1
    return 0


# ---------------------------------------------------------------------------
# Main entry point
# ---------------------------------------------------------------------------

def main() -> int:
    parser = argparse.ArgumentParser(
        description="Visualize XPM velocity layer structure as an ASCII chart."
    )
    parser.add_argument("xpm_path", help="Path to .xpm file")
    parser.add_argument("--no-color", action="store_true", help="Disable ANSI color output")
    parser.add_argument("--strict", action="store_true",
                        help="Exit code 1 if any overlap or gap >= 5 velocity units found")
    args = parser.parse_args()

    # Color detection
    use_color = not args.no_color and not os.environ.get("NO_COLOR") and sys.stdout.isatty()

    xpm_path = Path(args.xpm_path)
    if not xpm_path.exists():
        print(f"Error: file not found: {xpm_path}", file=sys.stderr)
        return 1

    try:
        instruments, drum_pads, prog_type = parse_xpm(xpm_path)
    except ET.ParseError as e:
        print(f"Error: XML parse failed: {e}", file=sys.stderr)
        return 1

    title = f"  {xpm_path.name}  [{prog_type.upper()}]"
    print()
    if use_color:
        print(BOLD + title + RESET)
    else:
        print(title)
    print()
    print_header(use_color)

    total_layers = 0
    total_overlaps = 0
    all_gaps: list[tuple[int, int]] = []

    if prog_type == "keygroup":
        # Sort by root note
        instruments.sort(key=lambda i: i.root_note)
        prev_note = -1
        for inst in instruments:
            # Emit octave boundary label if we crossed one
            note = inst.root_note
            if note // 12 != prev_note // 12 and note != inst.low_note:
                octave_note = (note // 12) * 12
                if octave_note != note:
                    pass  # label is on the row itself
            label = midi_to_name(note)
            line, ovl, gaps = render_row(label, inst.layers, use_color)
            print(line)
            total_layers += len(inst.layers)
            total_overlaps += ovl
            all_gaps.extend(gaps)
            prev_note = note
    else:
        # Drum mode: sort by pad number, show all 16
        drum_pads.sort(key=lambda p: p.number)
        pad_by_num = {p.number: p for p in drum_pads}
        for pad_num in range(1, 17):
            if pad_num in pad_by_num:
                pad = pad_by_num[pad_num]
                note_label = midi_to_name(pad.note)
                label = f"P{pad_num:02d}/{note_label}"
                line, ovl, gaps = render_row(label, pad.layers, use_color)
                print(line)
                total_layers += len(pad.layers)
                total_overlaps += ovl
                all_gaps.extend(gaps)
            else:
                # Empty pad row
                empty_bar = "." * BAR_WIDTH
                print(f"P{pad_num:02d}/---  {empty_bar}  {'(empty)':12}  [0L]")

    return print_summary(total_layers, total_overlaps, all_gaps, use_color, args.strict)


if __name__ == "__main__":
    sys.exit(main())
