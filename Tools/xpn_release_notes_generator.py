#!/usr/bin/env python3
"""
XPN Release Notes Generator — XO_OX Designs
Auto-generates RELEASE_NOTES.md for an XPN expansion pack.

Analyzes pack contents (programs, samples, size, expansion.json metadata) and,
when a previous version is provided, computes a diff to highlight what changed.

Sections generated:
  - What's New     (diff vs previous version, or "Initial Release")
  - Pack Contents  (program count, sample count, total size, featured engines)
  - Compatibility  (standard XO_OX MPC boilerplate)
  - Credits        (from expansion.json if present)

Engine detection: scans program/sample filenames for known XO_OX engine tokens.

Usage:
    python xpn_release_notes_generator.py --pack <dir_or_zip> [--prev-pack <dir_or_zip>] [--output RELEASE_NOTES.md]
    python xpn_release_notes_generator.py --pack MyPack_v1.1.zip --prev-pack MyPack_v1.0.zip
"""

import argparse
import json
import os
import re
import sys
import zipfile
from datetime import date
from pathlib import Path
from typing import Optional

# ---------------------------------------------------------------------------
# Constants
# ---------------------------------------------------------------------------

KNOWN_ENGINES = [
    "ONSET", "OPAL", "OBLONG", "OBESE", "OVERDUB", "ODYSSEY",
    "OVERWORLD", "OVERBITE", "ORBITAL", "ORGANISM", "OSTINATO",
    "OPENSKY", "OCEANDEEP", "OUIE", "OVERLAP", "OUTWIT",
    "OHM", "ORPHICA", "OBBLIGATO", "OTTONI", "OLE",
    "ORGANON", "OUROBOROS", "DRIFT", "FAT", "SNAP", "MORPH",
    "OVERTONE", "KNOT",
]

# Engines that also carry short prefix tokens in filenames
ENGINE_ALIASES: dict[str, list[str]] = {
    "ONSET":     ["ONS", "ONSET"],
    "OPAL":      ["OPL", "OPAL"],
    "OBLONG":    ["OBL", "OBLONG", "BOB"],
    "OBESE":     ["OBS", "OBESE", "XOB"],
    "OVERDUB":   ["DUB", "OVERDUB"],
    "ODYSSEY":   ["ODY", "ODYSSEY"],
    "OVERWORLD": ["OWD", "OVERWORLD"],
    "OVERBITE":  ["BITE", "OVERBITE"],
    "ORBITAL":   ["ORB", "ORBITAL"],
    "OVERLAP":   ["OLAP", "OVERLAP"],
    "OUTWIT":    ["OWIT", "OUTWIT"],
    "OHM":       ["OHM"],
    "ORPHICA":   ["ORPH", "ORPHICA"],
    "OBBLIGATO": ["OBBL", "OBBLIGATO"],
    "OTTONI":    ["OTTO", "OTTONI"],
    "OLE":       ["OLE"],
    "ORGANON":   ["ORG", "ORGANON"],
    "OUROBOROS": ["OURO", "OUROBOROS"],
    "DRIFT":     ["DRIFT"],
    "FAT":       ["FAT"],
    "SNAP":      ["SNAP"],
    "MORPH":     ["MORPH"],
    "OVERTONE":  ["OVERTONE"],
    "KNOT":      ["KNOT"],
    "ORGANISM":  ["ORGANISM"],
    "OSTINATO":  ["OSTINATO"],
    "OPENSKY":   ["OPENSKY"],
    "OCEANDEEP": ["OCEANDEEP"],
    "OUIE":      ["OUIE"],
}

SAMPLE_EXTENSIONS = {".wav", ".aif", ".aiff", ".mp3", ".flac", ".ogg"}

# ---------------------------------------------------------------------------
# Pack reading — works for both directory and ZIP
# ---------------------------------------------------------------------------

class PackInfo:
    """Snapshot of a pack's contents."""

    def __init__(self):
        self.programs: list[str] = []          # bare filenames, e.g. "KICK DRUM.xpm"
        self.samples: list[str] = []           # bare filenames
        self.total_bytes: int = 0
        self.expansion: dict = {}              # parsed expansion.json (may be empty)
        self.source_label: str = ""            # human-readable path label


def _read_zip(zip_path: Path) -> PackInfo:
    info = PackInfo()
    info.source_label = zip_path.name
    with zipfile.ZipFile(zip_path, "r") as zf:
        names = zf.namelist()
        for entry in zf.infolist():
            if entry.is_dir():
                continue
            lower = entry.filename.lower()
            bare = entry.filename.split("/")[-1]
            if not bare or bare.startswith("."):
                continue
            info.total_bytes += entry.file_size
            if lower.endswith(".xpm"):
                info.programs.append(bare)
            elif any(lower.endswith(ext) for ext in SAMPLE_EXTENSIONS):
                info.samples.append(bare)
            elif bare.lower() == "expansion.json":
                try:
                    raw = zf.read(entry.filename)
                    info.expansion = json.loads(raw.decode("utf-8", errors="replace"))
                except (json.JSONDecodeError, KeyError) as exc:
                    print(f"[WARN] Reading expansion.json from ZIP archive: {exc}", file=sys.stderr)
    return info


def _read_dir(dir_path: Path) -> PackInfo:
    info = PackInfo()
    info.source_label = dir_path.name
    for root, dirs, files in os.walk(dir_path):
        # skip hidden dirs
        dirs[:] = [d for d in dirs if not d.startswith(".")]
        for fname in files:
            if fname.startswith("."):
                continue
            fpath = Path(root) / fname
            lower = fname.lower()
            try:
                size = fpath.stat().st_size
            except OSError:
                size = 0
            info.total_bytes += size
            if lower.endswith(".xpm"):
                info.programs.append(fname)
            elif any(lower.endswith(ext) for ext in SAMPLE_EXTENSIONS):
                info.samples.append(fname)
            elif lower == "expansion.json":
                try:
                    info.expansion = json.loads(fpath.read_text(encoding="utf-8", errors="replace"))
                except (json.JSONDecodeError, OSError) as exc:
                    print(f"[WARN] Reading expansion.json from directory: {exc}", file=sys.stderr)
    return info


def load_pack(path_str: str) -> PackInfo:
    p = Path(path_str)
    if not p.exists():
        sys.exit(f"ERROR: pack path not found: {path_str}")
    if p.is_file() and p.suffix.lower() in (".zip", ".xpn"):
        return _read_zip(p)
    elif p.is_dir():
        return _read_dir(p)
    else:
        sys.exit(f"ERROR: unsupported pack format (expected .zip/.xpn or directory): {path_str}")


# ---------------------------------------------------------------------------
# Diff computation
# ---------------------------------------------------------------------------

class PackDiff:
    def __init__(self, prev: PackInfo, curr: PackInfo):
        prev_set = set(prev.programs)
        curr_set = set(curr.programs)
        self.added: list[str] = sorted(curr_set - prev_set)
        self.removed: list[str] = sorted(prev_set - curr_set)
        self.unchanged: list[str] = sorted(curr_set & prev_set)
        self.sample_delta: int = len(curr.samples) - len(prev.samples)
        self.size_delta: int = curr.total_bytes - prev.total_bytes


# ---------------------------------------------------------------------------
# Engine detection
# ---------------------------------------------------------------------------

def detect_engines(info: PackInfo) -> list[str]:
    """Return sorted list of engine names found in program/sample names."""
    all_names = info.programs + info.samples

    # Also check expansion.json tags / description
    exp_text = " ".join([
        info.expansion.get("name", ""),
        info.expansion.get("description", ""),
        " ".join(info.expansion.get("tags", [])),
    ]).upper()

    found: set[str] = set()

    for engine, tokens in ENGINE_ALIASES.items():
        for token in tokens:
            pattern = re.compile(r'\b' + re.escape(token) + r'\b', re.IGNORECASE)
            if pattern.search(exp_text):
                found.add(engine)
                break
            if any(pattern.search(name) for name in all_names):
                found.add(engine)
                break

    return sorted(found)


# ---------------------------------------------------------------------------
# Human-readable helpers
# ---------------------------------------------------------------------------

def _fmt_bytes(n: int) -> str:
    if n < 1024:
        return f"{n} B"
    if n < 1024 ** 2:
        return f"{n / 1024:.1f} KB"
    if n < 1024 ** 3:
        return f"{n / (1024 ** 2):.1f} MB"
    return f"{n / (1024 ** 3):.2f} GB"


def _fmt_delta(n: int) -> str:
    sign = "+" if n >= 0 else ""
    return f"{sign}{_fmt_bytes(n)}" if abs(n) >= 1024 else f"{sign}{n} B"


def _strip_xpm(name: str) -> str:
    """Remove .xpm extension for display."""
    return re.sub(r'\.xpm$', '', name, flags=re.IGNORECASE)


# ---------------------------------------------------------------------------
# Markdown generation
# ---------------------------------------------------------------------------

def build_release_notes(curr: PackInfo, prev: Optional[PackInfo], output_path: Path) -> str:
    today = date.today().strftime("%Y-%m-%d")

    # Metadata from expansion.json
    exp = curr.expansion
    pack_name = exp.get("name") or output_path.parent.name or curr.source_label
    pack_version = exp.get("version") or "1.0"
    pack_author = exp.get("author") or exp.get("manufacturer") or None
    pack_desc = exp.get("description") or ""
    pack_tags = exp.get("tags") or []
    credits_raw = exp.get("credits") or exp.get("credit") or {}

    engines = detect_engines(curr)

    lines: list[str] = []

    # Header
    lines.append(f"# {pack_name} — Release Notes")
    lines.append("")
    lines.append(f"**Version**: {pack_version}  ")
    lines.append(f"**Released**: {today}  ")
    if pack_author:
        lines.append(f"**Author**: {pack_author}  ")
    lines.append("")
    if pack_desc:
        lines.append(f"> {pack_desc}")
        lines.append("")

    # -----------------------------------------------------------------------
    # What's New
    # -----------------------------------------------------------------------
    lines.append("---")
    lines.append("")
    lines.append("## What's New")
    lines.append("")

    if prev is None:
        lines.append("**Initial Release.**")
        lines.append("")
        lines.append(
            f"This is the first public release of **{pack_name}**, containing "
            f"{len(curr.programs)} program{'s' if len(curr.programs) != 1 else ''} "
            f"and {len(curr.samples)} sample{'s' if len(curr.samples) != 1 else ''}."
        )
        lines.append("")
    else:
        diff = PackDiff(prev, curr)
        has_changes = diff.added or diff.removed or diff.sample_delta != 0

        if not has_changes:
            lines.append("No program-level changes from previous version. Internal refinements only.")
            lines.append("")
        else:
            if diff.added:
                lines.append(f"### New Programs ({len(diff.added)})")
                lines.append("")
                for p in diff.added:
                    lines.append(f"- {_strip_xpm(p)}")
                lines.append("")

            if diff.removed:
                lines.append(f"### Removed Programs ({len(diff.removed)})")
                lines.append("")
                for p in diff.removed:
                    lines.append(f"- {_strip_xpm(p)}")
                lines.append("")

            if diff.sample_delta != 0:
                direction = "added" if diff.sample_delta > 0 else "removed"
                abs_delta = abs(diff.sample_delta)
                lines.append(
                    f"### Sample Pool Update\n\n"
                    f"{abs_delta} sample{'s' if abs_delta != 1 else ''} {direction} "
                    f"({_fmt_delta(diff.size_delta)} size change)."
                )
                lines.append("")

        # size summary
        size_note = f"Pack size: {_fmt_bytes(curr.total_bytes)}"
        if prev:
            size_note += f" ({_fmt_delta(diff.size_delta)} vs previous)"
        lines.append(f"_{size_note}_")
        lines.append("")

    # -----------------------------------------------------------------------
    # Pack Contents
    # -----------------------------------------------------------------------
    lines.append("---")
    lines.append("")
    lines.append("## Pack Contents")
    lines.append("")
    lines.append(f"| Item | Count |")
    lines.append(f"|------|-------|")
    lines.append(f"| Programs | {len(curr.programs)} |")
    lines.append(f"| Samples | {len(curr.samples)} |")
    lines.append(f"| Total Size | {_fmt_bytes(curr.total_bytes)} |")
    lines.append("")

    if engines:
        lines.append("### Featured Engines")
        lines.append("")
        for eng in engines:
            lines.append(f"- **{eng}**")
        lines.append("")
    elif pack_tags:
        lines.append("### Tags")
        lines.append("")
        lines.append(", ".join(f"`{t}`" for t in pack_tags))
        lines.append("")

    if curr.programs:
        lines.append("### Programs")
        lines.append("")
        for prog in sorted(curr.programs):
            lines.append(f"- {_strip_xpm(prog)}")
        lines.append("")

    # -----------------------------------------------------------------------
    # Compatibility
    # -----------------------------------------------------------------------
    lines.append("---")
    lines.append("")
    lines.append("## Compatibility")
    lines.append("")
    lines.append(
        "This expansion pack is designed for **Akai MPC** hardware and software running "
        "**MPC firmware / software 3.x or later**."
    )
    lines.append("")
    lines.append("**Tested on:**")
    lines.append("")
    lines.append("- MPC Live III")
    lines.append("- MPC X")
    lines.append("- MPC One / One+")
    lines.append("- MPC Key 61 / 37")
    lines.append("- MPC Studio (software mode)")
    lines.append("")
    lines.append("**Requirements:**")
    lines.append("")
    lines.append("- MPC firmware 3.x or higher")
    lines.append("- Sufficient storage space for sample content")
    if engines:
        lines.append(
            f"- XO_OX plugin{'s' if len(engines) != 1 else ''} required: "
            + ", ".join(f"**{e}**" for e in engines)
        )
    lines.append("")
    lines.append(
        "> **Note**: XO_OX expansion packs require the corresponding XO_OX plugin(s) to be installed "
        "and authorized on your MPC. Visit [XO-OX.org](https://xo-ox.org) for downloads and licensing."
    )
    lines.append("")

    # -----------------------------------------------------------------------
    # Credits
    # -----------------------------------------------------------------------
    lines.append("---")
    lines.append("")
    lines.append("## Credits")
    lines.append("")

    has_credits = False

    if isinstance(credits_raw, dict) and credits_raw:
        for role, name in credits_raw.items():
            lines.append(f"- **{role.title()}**: {name}")
            has_credits = True
        lines.append("")
    elif isinstance(credits_raw, str) and credits_raw.strip():
        lines.append(credits_raw.strip())
        lines.append("")
        has_credits = True
    elif isinstance(credits_raw, list) and credits_raw:
        for entry in credits_raw:
            lines.append(f"- {entry}")
            has_credits = True
        lines.append("")

    if pack_author and not has_credits:
        lines.append(f"- **Sound Design**: {pack_author}")
        lines.append("")

    lines.append("Produced by **XO_OX Designs**.  ")
    lines.append("Sound design, synthesis, and curation: XO_OX team.  ")
    lines.append("")
    lines.append("---")
    lines.append("")
    lines.append(
        "_For support, updates, and additional packs visit [XO-OX.org](https://xo-ox.org)._"
    )
    lines.append("")

    return "\n".join(lines)


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def main() -> None:
    parser = argparse.ArgumentParser(
        description="Generate RELEASE_NOTES.md for an XPN expansion pack.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    parser.add_argument(
        "--pack", "-p", required=True,
        help="Current pack: path to directory or .zip/.xpn file",
    )
    parser.add_argument(
        "--prev-pack", "--prev", "-P", default=None,
        help="Previous version pack for diff (optional)",
    )
    parser.add_argument(
        "--output", "-o", default="RELEASE_NOTES.md",
        help="Output file path (default: RELEASE_NOTES.md)",
    )
    parser.add_argument(
        "--stdout", action="store_true",
        help="Print to stdout instead of writing a file",
    )

    args = parser.parse_args()

    print(f"[xpn_release_notes_generator] Loading pack: {args.pack}")
    curr = load_pack(args.pack)
    print(f"  Programs : {len(curr.programs)}")
    print(f"  Samples  : {len(curr.samples)}")
    print(f"  Size     : {_fmt_bytes(curr.total_bytes)}")

    prev: Optional[PackInfo] = None
    if args.prev_pack:
        print(f"[xpn_release_notes_generator] Loading previous pack: {args.prev_pack}")
        prev = load_pack(args.prev_pack)
        diff = PackDiff(prev, curr)
        print(f"  Added    : {len(diff.added)} programs")
        print(f"  Removed  : {len(diff.removed)} programs")
        print(f"  Unchanged: {len(diff.unchanged)} programs")
        print(f"  Size Δ   : {_fmt_delta(diff.size_delta)}")

    output_path = Path(args.output)
    notes = build_release_notes(curr, prev, output_path)

    if args.stdout:
        print()
        print(notes)
    else:
        output_path.write_text(notes, encoding="utf-8")
        print(f"\n[xpn_release_notes_generator] Written: {output_path.resolve()}")


if __name__ == "__main__":
    main()
