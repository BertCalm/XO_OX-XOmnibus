#!/usr/bin/env python3
"""
xpn_coupling_docs_generator.py

Reads a .xometa preset file, extracts coupling routes, and generates
a <CouplingDNA> XML comment block that can be embedded into .xpm files.

Usage:
  python xpn_coupling_docs_generator.py --xometa preset.xometa
  python xpn_coupling_docs_generator.py --xometa preset.xometa --inject program.xpm
  python xpn_coupling_docs_generator.py --xometa preset.xometa --batch-dir ./xpms/

Also importable:
  from xpn_coupling_docs_generator import generate_coupling_comment
"""

import argparse
import json
import os
import sys
from datetime import date
from pathlib import Path
from typing import Optional


def generate_coupling_comment(
    xometa_path: str,
    bpm: float = 120.0,
    root_note: str = "C3",
    xolokun_version: str = "1.0",
    render_date: Optional[str] = None,
) -> str:
    """
    Parse a .xometa file and return a CouplingDNA XML comment string.

    Args:
        xometa_path: Path to the .xometa JSON file.
        bpm: BPM to embed in RenderConditions.
        root_note: Root note to embed in RenderConditions.
        xolokun_version: XOlokun version string.
        render_date: ISO date string (YYYY-MM-DD). Defaults to today.

    Returns:
        A multi-line string containing the XML comment block.
    """
    if render_date is None:
        render_date = date.today().isoformat()

    with open(xometa_path, "r", encoding="utf-8") as f:
        meta = json.load(f)

    routes = meta.get("couplingRoutes", [])

    lines = ["<!-- CouplingDNA"]
    lines.append("  <Routes>")

    for route in routes:
        source = route.get("source_engine", "")
        target = route.get("target_engine", "")
        ctype = route.get("coupling_type", "")
        amount = route.get("coupling_amount", 0.0)
        polarity = route.get("coupling_polarity", 1)

        polarity_str = f"+{polarity}" if polarity >= 0 else str(polarity)
        amount_str = f"{float(amount):.2f}"

        lines.append(
            f'    <Route source="{source}" target="{target}" '
            f'type="{ctype}" amount="{amount_str}" polarity="{polarity_str}" />'
        )

    lines.append("  </Routes>")

    bpm_str = f"{float(bpm):.0f}" if float(bpm) == int(bpm) else str(bpm)
    lines.append(
        f'  <RenderConditions bpm="{bpm_str}" root_note="{root_note}" '
        f'xolokun_version="{xolokun_version}" render_date="{render_date}" />'
    )

    lines.append("-->")

    return "\n".join(lines)


def inject_into_xpm(xpm_path: str, comment_block: str) -> None:
    """
    Inject a CouplingDNA comment block into a .xpm file.

    Inserts after </MPCVObject> if present, otherwise appends to end of file.
    Skips injection if a CouplingDNA block already exists.

    Args:
        xpm_path: Path to the .xpm file to modify.
        comment_block: The XML comment string to inject.
    """
    with open(xpm_path, "r", encoding="utf-8") as f:
        content = f.read()

    if "<!-- CouplingDNA" in content:
        print(f"  [skip] CouplingDNA already present in {xpm_path}")
        return

    insert_tag = "</MPCVObject>"
    if insert_tag in content:
        idx = content.index(insert_tag) + len(insert_tag)
        new_content = content[:idx] + "\n" + comment_block + "\n" + content[idx:]
    else:
        # Append at end of file
        new_content = content.rstrip() + "\n" + comment_block + "\n"

    with open(xpm_path, "w", encoding="utf-8") as f:
        f.write(new_content)

    print(f"  [ok] Injected CouplingDNA into {xpm_path}")


def batch_inject(xpms_dir: str, comment_block: str) -> int:
    """
    Inject comment_block into all .xpm files found in xpms_dir (non-recursive).

    Returns the count of files processed.
    """
    xpm_files = sorted(Path(xpms_dir).glob("*.xpm"))
    if not xpm_files:
        print(f"No .xpm files found in {xpms_dir}")
        return 0

    for xpm_path in xpm_files:
        inject_into_xpm(str(xpm_path), comment_block)

    return len(xpm_files)


def build_arg_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description=(
            "Generate a CouplingDNA XML comment block from a .xometa preset file "
            "and optionally inject it into .xpm files."
        )
    )
    parser.add_argument(
        "--xometa",
        required=True,
        metavar="FILE",
        help="Path to the .xometa preset JSON file.",
    )
    parser.add_argument(
        "--inject",
        metavar="XPM_FILE",
        help="Inject the CouplingDNA block into a single .xpm file.",
    )
    parser.add_argument(
        "--batch-dir",
        metavar="DIR",
        help="Inject the CouplingDNA block into all .xpm files in this directory.",
    )
    parser.add_argument(
        "--bpm",
        type=float,
        default=120.0,
        help="BPM to embed in RenderConditions (default: 120).",
    )
    parser.add_argument(
        "--root-note",
        default="C3",
        help="Root note to embed in RenderConditions (default: C3).",
    )
    parser.add_argument(
        "--xolokun-version",
        default="1.0",
        help="XOlokun version string (default: 1.0).",
    )
    parser.add_argument(
        "--render-date",
        default=None,
        help="Render date in YYYY-MM-DD format (default: today).",
    )
    return parser


def main() -> None:
    parser = build_arg_parser()
    args = parser.parse_args()

    if not os.path.isfile(args.xometa):
        print(f"Error: .xometa file not found: {args.xometa}", file=sys.stderr)
        sys.exit(1)

    try:
        comment_block = generate_coupling_comment(
            xometa_path=args.xometa,
            bpm=args.bpm,
            root_note=args.root_note,
            xolokun_version=args.xolokun_version,
            render_date=args.render_date,
        )
    except json.JSONDecodeError as exc:
        print(f"Error: Failed to parse .xometa JSON: {exc}", file=sys.stderr)
        sys.exit(1)
    except KeyError as exc:
        print(f"Error: Unexpected .xometa structure — missing key {exc}", file=sys.stderr)
        sys.exit(1)

    if args.inject:
        if not os.path.isfile(args.inject):
            print(f"Error: XPM file not found: {args.inject}", file=sys.stderr)
            sys.exit(1)
        inject_into_xpm(args.inject, comment_block)

    elif args.batch_dir:
        if not os.path.isdir(args.batch_dir):
            print(f"Error: Directory not found: {args.batch_dir}", file=sys.stderr)
            sys.exit(1)
        count = batch_inject(args.batch_dir, comment_block)
        print(f"Done — processed {count} .xpm file(s).")

    else:
        # Default: print comment block to stdout
        print(comment_block)


if __name__ == "__main__":
    main()
