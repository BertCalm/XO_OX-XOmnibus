#!/usr/bin/env python3
"""
XPN Pad Note Mapper — XO_OX Designs
Maps MPC pad positions (A01-A16, B01-B16, ...) to MIDI note numbers and vice versa.
Generates pad layout reports for .xpn programs. Validates drum kit conventions.

MPC 4x4 pad grid (standard layout):
  Row 4 (top):    A01-A04  → notes 48-51
  Row 3:          A05-A08  → notes 44-47
  Row 2:          A09-A12  → notes 40-43
  Row 1 (bottom): A13-A16  → notes 36-39

XO_OX Drum Convention (GM-adjacent):
  A13 → 36 (Kick)     A15 → 38 (Snare)
  A09 → 42 (Cl.Hat)   A11 → 46 (Open Hat)

Usage:
    python xpn_pad_note_mapper.py --note 36
    python xpn_pad_note_mapper.py --pad A13
    python xpn_pad_note_mapper.py --pack MyKit.xpn [--program "Kit 1"] [--validate]
    python xpn_pad_note_mapper.py --pack MyKit.xpn --format json
"""

import argparse
import json
import sys
import zipfile
import xml.etree.ElementTree as ET
from typing import Optional

# ---------------------------------------------------------------------------
# Constants
# ---------------------------------------------------------------------------

NOTE_NAMES = ["C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"]

# Standard MPC pad → note mapping (bank A, 4×4 grid)
# Row 1 bottom = pads 13-16, Row 4 top = pads 1-4
# Pad 1 = note 48, pad 13 = note 36
def _build_pad_to_note():
    mapping = {}
    for bank_idx, bank in enumerate("ABCD"):
        bank_base = bank_idx * 16  # not used for note offset — each bank shifts by 16
        for pad_num in range(1, 17):
            # Row is 0-based from bottom; pad 13 is row 0, pad 1 is row 3
            row = (16 - pad_num) // 4   # 0 (bottom) … 3 (top)
            col = (pad_num - 1) % 4     # 0…3 left to right
            note = 36 + row * 4 + col + bank_idx * 16
            pad_id = f"{bank}{pad_num:02d}"
            mapping[pad_id] = note
    return mapping

PAD_TO_NOTE = _build_pad_to_note()
NOTE_TO_PAD = {v: k for k, v in PAD_TO_NOTE.items()}

# General MIDI drum map (note → name)
GM_DRUMS = {
    35: "Acoustic Bass Drum",
    36: "Bass Drum 1",
    37: "Side Stick",
    38: "Acoustic Snare",
    39: "Hand Clap",
    40: "Electric Snare",
    41: "Low Floor Tom",
    42: "Closed Hi-Hat",
    43: "High Floor Tom",
    44: "Pedal Hi-Hat",
    45: "Low Tom",
    46: "Open Hi-Hat",
    47: "Low-Mid Tom",
    48: "Hi-Mid Tom",
    49: "Crash Cymbal 1",
    50: "High Tom",
    51: "Ride Cymbal 1",
    52: "Chinese Cymbal",
    53: "Ride Bell",
    54: "Tambourine",
    55: "Splash Cymbal",
    56: "Cowbell",
    57: "Crash Cymbal 2",
    58: "Vibraslap",
    59: "Ride Cymbal 2",
    60: "Hi Bongo",
    61: "Low Bongo",
    62: "Mute Hi Conga",
    63: "Open Hi Conga",
    64: "Low Conga",
    65: "High Timbale",
    66: "Low Timbale",
    67: "High Agogo",
    68: "Low Agogo",
    69: "Cabasa",
    70: "Maracas",
}

# XO_OX pad convention expectations for drum kits
XOOX_CONVENTIONS = [
    {"pad": "A13", "note": 36, "alt_note": 37, "role": "Kick",       "severity": "WARNING"},
    {"pad": "A15", "note": 38, "alt_note": 40, "role": "Snare",      "severity": "WARNING"},
    {"pad": "A09", "note": 42, "alt_note": 44, "role": "Closed Hat", "severity": "WARNING"},
    {"pad": "A11", "note": 46, "alt_note": None, "role": "Open Hat", "severity": "WARNING"},
]

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def note_name(note: int) -> str:
    """Return note name like C2, F#3."""
    octave = (note // 12) - 2
    name = NOTE_NAMES[note % 12]
    return f"{name}{octave}"


def pad_row_col(pad_id: str) -> tuple[int, int]:
    """Return (row, col) 1-based for display (row 1 = bottom)."""
    pad_num = int(pad_id[1:])
    row = (16 - pad_num) // 4 + 1
    col = (pad_num - 1) % 4 + 1
    return row, col


def gm_name(note: int) -> str:
    return GM_DRUMS.get(note, "—")


# ---------------------------------------------------------------------------
# Mode: single note or pad lookup
# ---------------------------------------------------------------------------

def cmd_map_note(note: int, fmt: str):
    if note < 0 or note > 127:
        print(f"ERROR: Note {note} out of range 0–127", file=sys.stderr)
        sys.exit(1)

    pad = NOTE_TO_PAD.get(note)
    row, col = pad_row_col(pad) if pad else (None, None)
    gm = gm_name(note)
    nn = note_name(note)

    if fmt == "json":
        print(json.dumps({
            "note": note,
            "note_name": nn,
            "pad": pad,
            "row": row,
            "col": col,
            "gm_name": gm,
        }, indent=2))
    else:
        pad_str = f"Pad {pad} (Row {row}, Col {col})" if pad else "No standard pad"
        print(f"Note {note:3d} ({nn:4s}) → {pad_str} | GM: {gm}")


def cmd_map_pad(pad_id: str, fmt: str):
    pad_id = pad_id.upper()
    note = PAD_TO_NOTE.get(pad_id)
    if note is None:
        print(f"ERROR: Unknown pad '{pad_id}'. Valid: A01–A16, B01–B16, etc.", file=sys.stderr)
        sys.exit(1)

    row, col = pad_row_col(pad_id)
    gm = gm_name(note)
    nn = note_name(note)

    if fmt == "json":
        print(json.dumps({
            "pad": pad_id,
            "note": note,
            "note_name": nn,
            "row": row,
            "col": col,
            "gm_name": gm,
        }, indent=2))
    else:
        print(f"Pad {pad_id} (Row {row}, Col {col}) → Note {note} ({nn}) | GM: {gm}")


# ---------------------------------------------------------------------------
# XPN parsing
# ---------------------------------------------------------------------------

def load_programs_from_xpn(xpn_path: str) -> list[dict]:
    """Extract all drum program pad note assignments from .xpn (ZIP+XML)."""
    programs = []
    with zipfile.ZipFile(xpn_path, "r") as zf:
        xpm_files = [n for n in zf.namelist() if n.endswith(".xpm")]
        for xpm_name in sorted(xpm_files):
            with zf.open(xpm_name) as f:
                try:
                    tree = ET.parse(f)
                except ET.ParseError as e:
                    print(f"WARNING: Could not parse {xpm_name}: {e}", file=sys.stderr)
                    continue

            root = tree.getroot()
            prog_el = root.find("Program")
            if prog_el is None:
                continue

            prog_type = prog_el.get("type", "")
            prog_name = prog_el.get("name", xpm_name.split("/")[-1].replace(".xpm", ""))

            # Collect pad assignments from <Pad number="N"> / <Note>
            pads_info = {}
            for pad_el in prog_el.iter("Pad"):
                pad_num = pad_el.get("number")
                note_el = pad_el.find("Note")
                if pad_num is not None and note_el is not None:
                    try:
                        note = int(note_el.text)
                        pad_id = f"A{int(pad_num):02d}"
                        pads_info[pad_id] = note
                    except (ValueError, TypeError) as exc:
                        print(f"[WARN] Parsing pad number/note for pad {pad_num}: {exc}", file=sys.stderr)

            programs.append({
                "name": prog_name,
                "type": prog_type,
                "file": xpm_name,
                "pads": pads_info,
            })

    return programs


# ---------------------------------------------------------------------------
# Mode: report
# ---------------------------------------------------------------------------

def cmd_report(programs: list, target_name: Optional[str], fmt: str, drum_mode: bool):
    if target_name:
        programs = [p for p in programs if target_name.lower() in p["name"].lower()]
        if not programs:
            print(f"ERROR: No program matching '{target_name}' found.", file=sys.stderr)
            sys.exit(1)

    if fmt == "json":
        output = []
        for prog in programs:
            rows = []
            for row in range(4, 0, -1):  # top to bottom display
                row_pads = []
                for col in range(1, 5):
                    pad_num = (4 - row) * 4 + col
                    pad_id = f"A{pad_num:02d}"
                    note = prog["pads"].get(pad_id)
                    row_pads.append({
                        "pad": pad_id,
                        "note": note,
                        "note_name": note_name(note) if note is not None else None,
                        "gm_name": gm_name(note) if note is not None else None,
                    })
                rows.append(row_pads)
            output.append({"program": prog["name"], "type": prog["type"], "grid": rows})
        print(json.dumps(output, indent=2))
        return

    for prog in programs:
        print(f"\nPAD LAYOUT — {prog['name']}  [{prog['type']}]")
        print()
        for row in range(4, 0, -1):  # row 4 top, row 1 bottom
            cells = []
            for col in range(1, 5):
                pad_num = (4 - row) * 4 + col
                pad_id = f"A{pad_num:02d}"
                note = prog["pads"].get(pad_id)
                if note is None:
                    cells.append(f"[{pad_id}:--     ]")
                elif drum_mode and note in GM_DRUMS:
                    short = GM_DRUMS[note][:8]
                    cells.append(f"[{pad_id}:{note} {short:<8s}]")
                else:
                    cells.append(f"[{pad_id}:{note} {note_name(note):<5s}]")
            print("  " + "  ".join(cells))
        print()


# ---------------------------------------------------------------------------
# Mode: validate
# ---------------------------------------------------------------------------

def cmd_validate(programs: list, target_name: Optional[str], fmt: str):
    if target_name:
        programs = [p for p in programs if target_name.lower() in p["name"].lower()]

    all_results = []

    for prog in programs:
        issues = []
        pads = prog["pads"]

        for conv in XOOX_CONVENTIONS:
            pad_id = conv["pad"]
            expected = conv["note"]
            alt = conv["alt_note"]
            role = conv["role"]
            actual = pads.get(pad_id)

            if actual is None:
                issues.append({
                    "severity": "WARNING",
                    "pad": pad_id,
                    "role": role,
                    "expected_note": expected,
                    "actual_note": None,
                    "message": f"{pad_id} has no note assigned (expected {expected} for {role})",
                })
            elif actual != expected and actual != alt:
                issues.append({
                    "severity": conv["severity"],
                    "pad": pad_id,
                    "role": role,
                    "expected_note": expected,
                    "actual_note": actual,
                    "message": (
                        f"{pad_id} has note {actual} ({note_name(actual)}) — "
                        f"expected {expected} ({note_name(expected)}) for {role}"
                        + (f" or alt {alt} ({note_name(alt)})" if alt else "")
                    ),
                })

        status = "PASS" if not issues else "WARN"
        all_results.append({"program": prog["name"], "status": status, "issues": issues})

    if fmt == "json":
        print(json.dumps(all_results, indent=2))
        return

    for result in all_results:
        marker = "PASS" if result["status"] == "PASS" else "WARN"
        print(f"\n[{marker}] {result['program']}")
        if not result["issues"]:
            print("  All XO_OX pad conventions satisfied.")
        for issue in result["issues"]:
            print(f"  {issue['severity']}: {issue['message']}")


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def main():
    parser = argparse.ArgumentParser(
        description="XPN Pad Note Mapper — map pads↔notes, report layouts, validate conventions."
    )
    parser.add_argument("--note", type=int, metavar="NOTE",
                        help="MIDI note number (0–127) to look up")
    parser.add_argument("--pad", type=str, metavar="PAD",
                        help="Pad ID (e.g. A13) to look up")
    parser.add_argument("--pack", type=str, metavar="PACK.xpn",
                        help="Path to .xpn file for report/validate modes")
    parser.add_argument("--program", type=str, metavar="NAME",
                        help="Program name filter (substring match, case-insensitive)")
    parser.add_argument("--validate", action="store_true",
                        help="Validate drum pad conventions instead of showing report")
    parser.add_argument("--drum", action="store_true",
                        help="Show GM drum names in grid report (auto-enabled for Drum programs)")
    parser.add_argument("--format", choices=["text", "json"], default="text",
                        help="Output format (default: text)")

    args = parser.parse_args()

    if args.note is not None:
        cmd_map_note(args.note, args.format)
    elif args.pad is not None:
        cmd_map_pad(args.pad, args.format)
    elif args.pack is not None:
        programs = load_programs_from_xpn(args.pack)
        if not programs:
            print("ERROR: No programs found in pack.", file=sys.stderr)
            sys.exit(1)
        if args.validate:
            cmd_validate(programs, args.program, args.format)
        else:
            drum_mode = args.drum or any(p["type"] == "Drum" for p in programs)
            cmd_report(programs, args.program, args.format, drum_mode)
    else:
        parser.print_help()
        sys.exit(0)


if __name__ == "__main__":
    main()
