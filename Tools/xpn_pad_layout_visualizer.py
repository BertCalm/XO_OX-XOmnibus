#!/usr/bin/env python3
"""
XPN Pad Layout Visualizer — XO_OX Designs
Renders ASCII pad layout grids from MPC XPM drum program files.

Shows which pads are assigned, their sample names, and bank organization.
Supports single .xpm files or entire pack directories.

Usage:
    python3 xpn_pad_layout_visualizer.py <file_or_dir>
    python3 xpn_pad_layout_visualizer.py <file_or_dir> --compact
    python3 xpn_pad_layout_visualizer.py <file_or_dir> --bank A
    python3 xpn_pad_layout_visualizer.py <file_or_dir> --bank B --compact

Bank mapping (MPC convention, 16 pads per bank):
    Bank A = instruments  1-16  (pads 1-16)
    Bank B = instruments 17-32  (pads 1-16 on bank B)
    Bank C = instruments 33-48
    Bank D = instruments 49-64
    ... up to Bank H (instruments 113-128)

Grid display (MPC pad order — rows top-to-bottom are rows 4,3,2,1):
    Row 4: pads 13 14 15 16
    Row 3: pads  9 10 11 12
    Row 2: pads  5  6  7  8
    Row 1: pads  1  2  3  4
"""

import argparse
import os
import sys
import xml.etree.ElementTree as ET
from pathlib import Path
from typing import Dict, List, Optional, Tuple

# ---------------------------------------------------------------------------
# Constants
# ---------------------------------------------------------------------------

PADS_PER_BANK = 16
BANK_LETTERS = "ABCDEFGH"
CELL_WIDTH = 14        # characters per pad cell (excluding borders)
COMPACT_NAME_WIDTH = 30

# MPC pad grid order: rows displayed top-to-bottom are rows 4,3,2,1
# Each row has 4 pads left-to-right.
GRID_ROWS = [
    [13, 14, 15, 16],  # top row
    [ 9, 10, 11, 12],
    [ 5,  6,  7,  8],
    [ 1,  2,  3,  4],  # bottom row
]

# ---------------------------------------------------------------------------
# XPM Parsing
# ---------------------------------------------------------------------------

def parse_xpm(path: Path) -> dict:
    """
    Parse an XPM file and return a dict with:
        program_name: str
        program_type: str  ("Drum", "Keygroup", "Plugin", etc.)
        pads: dict[int, dict]  instrument_number (1-128) -> pad info
            pad info keys: sample_name, sample_file, note, layer_count
        pad_note_map: dict[int, int]  pad_number -> note
    """
    result = {
        "program_name": path.stem,
        "program_type": "Unknown",
        "pads": {},
        "pad_note_map": {},
        "source_file": str(path),
    }

    try:
        tree = ET.parse(str(path))
    except ET.ParseError as exc:
        result["parse_error"] = str(exc)
        return result

    root = tree.getroot()

    program_elem = root.find("Program")
    if program_elem is None:
        result["parse_error"] = "No <Program> element found"
        return result

    result["program_type"] = program_elem.get("type", "Unknown")

    name_elem = program_elem.find("ProgramName")
    if name_elem is not None and name_elem.text:
        result["program_name"] = name_elem.text.strip()

    # PadNoteMap: <Pad number="N" note="M"/>
    pad_note_map_elem = program_elem.find("PadNoteMap")
    if pad_note_map_elem is not None:
        for pad_elem in pad_note_map_elem.findall("Pad"):
            try:
                pad_num = int(pad_elem.get("number", 0))
                note = int(pad_elem.get("note", -1))
                if pad_num > 0:
                    result["pad_note_map"][pad_num] = note
            except (ValueError, TypeError):
                pass

    # Instruments: <Instrument number="N">
    instruments_elem = program_elem.find("Instruments")
    if instruments_elem is not None:
        for inst_elem in instruments_elem.findall("Instrument"):
            try:
                inst_num = int(inst_elem.get("number", 0))
            except (ValueError, TypeError):
                continue
            if inst_num < 1:
                continue

            # Collect sample names from all layers
            layers_elem = inst_elem.find("Layers")
            sample_names = []
            sample_files = []
            if layers_elem is not None:
                for layer_elem in layers_elem.findall("Layer"):
                    sn_elem = layer_elem.find("SampleName")
                    sf_elem = layer_elem.find("SampleFile")
                    sn = (sn_elem.text or "").strip() if sn_elem is not None else ""
                    sf = (sf_elem.text or "").strip() if sf_elem is not None else ""
                    if sn:
                        sample_names.append(sn)
                    if sf:
                        sample_files.append(sf)

            # Determine primary sample name (first non-empty layer)
            primary_name = sample_names[0] if sample_names else ""
            primary_file = sample_files[0] if sample_files else ""

            # Use filename stem if sample name is empty but file path exists
            if not primary_name and primary_file:
                primary_name = Path(primary_file).stem

            result["pads"][inst_num] = {
                "sample_name": primary_name,
                "sample_file": primary_file,
                "layer_count": len(sample_names),
                "note": result["pad_note_map"].get(inst_num, -1),
            }

    return result

# ---------------------------------------------------------------------------
# Bank Organization
# ---------------------------------------------------------------------------

def split_into_banks(pads: dict) -> dict:
    """
    Split instrument pad dict into banks (A-H, 16 pads each).
    Returns: dict[bank_letter] -> dict[bank_pad_number(1-16)] -> pad_info
    """
    banks = {}
    for inst_num, pad_info in pads.items():
        bank_idx = (inst_num - 1) // PADS_PER_BANK
        if bank_idx >= len(BANK_LETTERS):
            continue
        bank_letter = BANK_LETTERS[bank_idx]
        bank_pad_num = ((inst_num - 1) % PADS_PER_BANK) + 1
        if bank_letter not in banks:
            banks[bank_letter] = {}
        banks[bank_letter][bank_pad_num] = pad_info
    return banks

def bank_fill_stats(bank_pads: Dict) -> Tuple:
    """Return (assigned_count, total=16, fill_pct)."""
    assigned = sum(
        1 for p in bank_pads.values()
        if p.get("sample_name") or p.get("sample_file")
    )
    return assigned, PADS_PER_BANK, int(assigned / PADS_PER_BANK * 100)

# ---------------------------------------------------------------------------
# ASCII Grid Rendering
# ---------------------------------------------------------------------------

def truncate(text: str, width: int) -> str:
    """Truncate text to fit within width, appending '…' if needed."""
    if len(text) <= width:
        return text
    return text[:width - 1] + "\u2026"


def render_cell(pad_num: int, pad_info: Optional[Dict], width: int = CELL_WIDTH) -> List:
    """
    Return a list of strings representing one pad cell (3 lines of content).
    Lines do NOT include the left border '|' — caller adds borders.
    """
    inner_w = width  # usable width inside borders

    if pad_info is None or (not pad_info.get("sample_name") and not pad_info.get("sample_file")):
        # Empty pad
        pad_label = f"P{pad_num:02d}".center(inner_w)
        empty_marker = "[ ]".center(inner_w)
        filler = " " * inner_w
        return [pad_label, empty_marker, filler]

    sample_name = pad_info.get("sample_name", "")
    layers = pad_info.get("layer_count", 1)

    pad_label = f"P{pad_num:02d}".center(inner_w)
    filled_marker = "[■]".center(inner_w)

    # Sample name line — truncated to fit
    name_display = truncate(sample_name, inner_w)
    name_line = name_display.center(inner_w)

    return [pad_label, filled_marker, name_line]


def render_bank_grid(bank_letter: str, bank_pads: Dict) -> List:
    """
    Render a 4×4 ASCII grid for one bank.
    Returns a list of strings (lines) to print.
    """
    lines = []
    assigned, total, fill_pct = bank_fill_stats(bank_pads)

    lines.append(f"  Bank {bank_letter}  [{assigned}/{total} pads filled — {fill_pct}%]")
    lines.append("")

    # Column separator
    sep_cell = "-" * (CELL_WIDTH + 2)  # +2 for the spaces around content
    h_border = "+" + ("+".join([sep_cell] * 4)) + "+"

    lines.append(h_border)

    for row in GRID_ROWS:
        # Each pad cell is 3 content lines
        cell_lines = []
        for pad_num in row:
            pad_info = bank_pads.get(pad_num)
            cell_lines.append(render_cell(pad_num, pad_info))

        # Interleave: for each of 3 content rows, output all 4 cells on one line
        for row_idx in range(3):
            line = "|"
            for cell_idx in range(4):
                content = cell_lines[cell_idx][row_idx]
                line += f" {content} |"
            lines.append(line)
        lines.append(h_border)

    return lines


# ---------------------------------------------------------------------------
# Compact Table Rendering
# ---------------------------------------------------------------------------

def render_compact_table(bank_letter: str, bank_pads: Dict) -> List:
    """Render a compact table: pad number | note | layers | sample name."""
    lines = []
    assigned, total, fill_pct = bank_fill_stats(bank_pads)

    lines.append(f"  Bank {bank_letter}  [{assigned}/{total} pads filled — {fill_pct}%]")
    header = f"  {'Pad':>4}  {'Note':>4}  {'Lyr':>3}  {'Sample Name':<{COMPACT_NAME_WIDTH}}"
    lines.append(header)
    lines.append("  " + "-" * (len(header) - 2))

    for pad_num in range(1, PADS_PER_BANK + 1):
        pad_info = bank_pads.get(pad_num)
        if pad_info and (pad_info.get("sample_name") or pad_info.get("sample_file")):
            note_val = pad_info.get("note", -1)
            note_str = str(note_val) if note_val >= 0 else "—"
            layers = pad_info.get("layer_count", 1)
            name = truncate(pad_info.get("sample_name", ""), COMPACT_NAME_WIDTH)
            status = "■"
        else:
            note_str = "—"
            layers = 0
            name = "(empty)"
            status = " "

        lines.append(
            f"  [{status}] P{pad_num:02d}  {note_str:>4}  {layers:>3}  {name:<{COMPACT_NAME_WIDTH}}"
        )

    return lines


# ---------------------------------------------------------------------------
# Top-level Rendering
# ---------------------------------------------------------------------------

def render_xpm(parsed: Dict, compact: bool, bank_filter: Optional[str]) -> List:
    """
    Render all banks for a single parsed XPM.
    Returns list of output lines.
    """
    lines = []

    if "parse_error" in parsed:
        lines.append(f"  ERROR: {parsed['parse_error']}")
        return lines

    pads = parsed.get("pads", {})
    if not pads:
        lines.append("  No <Instruments> data found (may be Keygroup or Plugin type).")
        return lines

    banks = split_into_banks(pads)
    if not banks:
        lines.append("  No pad assignments detected.")
        return lines

    # Overall stats
    total_assigned = sum(
        1 for p in pads.values()
        if p.get("sample_name") or p.get("sample_file")
    )
    total_slots = len(banks) * PADS_PER_BANK
    overall_pct = int(total_assigned / total_slots * 100) if total_slots else 0
    lines.append(
        f"  Total: {total_assigned}/{total_slots} pads assigned ({overall_pct}%) "
        f"across {len(banks)} bank(s)"
    )
    lines.append("")

    for bank_letter in sorted(banks.keys()):
        if bank_filter and bank_letter != bank_filter.upper():
            continue
        bank_pads = banks[bank_letter]
        if compact:
            lines.extend(render_compact_table(bank_letter, bank_pads))
        else:
            lines.extend(render_bank_grid(bank_letter, bank_pads))
        lines.append("")

    return lines


def process_file(path: Path, compact: bool, bank_filter: Optional[str]) -> None:
    """Parse and display one XPM file."""
    parsed = parse_xpm(path)
    prog_name = parsed.get("program_name", path.stem)
    prog_type = parsed.get("program_type", "Unknown")

    print()
    print(f"{'=' * 60}")
    print(f"  {prog_name}  [{prog_type}]")
    print(f"  {path.name}")
    print(f"{'=' * 60}")

    output_lines = render_xpm(parsed, compact=compact, bank_filter=bank_filter)
    for line in output_lines:
        print(line)


def process_directory(directory: Path, compact: bool, bank_filter: Optional[str]) -> None:
    """Find and process all .xpm files in a directory (non-recursive)."""
    xpm_files = sorted(directory.glob("*.xpm"))
    if not xpm_files:
        # Try recursive
        xpm_files = sorted(directory.rglob("*.xpm"))

    if not xpm_files:
        print(f"No .xpm files found in: {directory}")
        sys.exit(1)

    print(f"\nFound {len(xpm_files)} XPM file(s) in: {directory}")

    for xpm_path in xpm_files:
        process_file(xpm_path, compact=compact, bank_filter=bank_filter)


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def main() -> None:
    parser = argparse.ArgumentParser(
        description="Visualize MPC pad layouts from XPM files or pack directories.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    parser.add_argument(
        "target",
        type=Path,
        help="Path to a .xpm file or a pack directory containing .xpm files.",
    )
    parser.add_argument(
        "--compact",
        action="store_true",
        help="Show a compact table (pad numbers + sample names) instead of ASCII art grids.",
    )
    parser.add_argument(
        "--bank",
        metavar="LETTER",
        type=str,
        default=None,
        help="Only show the specified bank (A-H). Example: --bank A",
    )

    args = parser.parse_args()
    target: Path = args.target.expanduser().resolve()

    if not target.exists():
        print(f"Error: path does not exist: {target}", file=sys.stderr)
        sys.exit(1)

    bank_filter = args.bank.upper() if args.bank else None  # type: Optional[str]
    if bank_filter and bank_filter not in BANK_LETTERS:
        print(
            f"Error: --bank must be one of {', '.join(BANK_LETTERS)}",
            file=sys.stderr,
        )
        sys.exit(1)

    if target.is_file():
        if target.suffix.lower() != ".xpm":
            print(f"Warning: file does not have .xpm extension: {target.name}")
        process_file(target, compact=args.compact, bank_filter=bank_filter)
    elif target.is_dir():
        process_directory(target, compact=args.compact, bank_filter=bank_filter)
    else:
        print(f"Error: target is neither a file nor a directory: {target}", file=sys.stderr)
        sys.exit(1)


if __name__ == "__main__":
    main()
