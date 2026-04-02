#!/usr/bin/env python3
"""
xpn_pack_catalog_generator.py — XO_OX Designs

Generates a human-readable catalog document for an XPN pack.
The output is a structured Markdown file (or HTML wrapper) that doubles
as marketing copy, documentation, and a producer reference guide.

Scans a pack directory for:
  - expansion.json   — pack metadata
  - **/*.xpm         — MPC program files (Drum or Keygroup)
  - **/*.xometa      — XOceanus preset files

Catalog sections:
  1. Pack Header         name, version, description, author, tags
  2. Contents at a Glance  program/sample/preset counts + total size
  3. Engine Roster       which XO_OX engines appear
  4. Program Directory   table: Program Name | Type | Samples | Notes
  5. Preset Directory    table (if .xometa present): Name | Mood | Engine(s) | DNA Character
  6. How to Use          XO_OX boilerplate + pack-specific notes
  7. Compatibility       supported MPC hardware + firmware

CLI:
    python xpn_pack_catalog_generator.py <pack_dir> [--format md|html] [--output CATALOG.md]
"""

import argparse
import json
import sys
import xml.etree.ElementTree as ET
from pathlib import Path

# ---------------------------------------------------------------------------
# DNA word bank — 2-3 word descriptors derived from dominant 6D DNA axes
# ---------------------------------------------------------------------------

# Maps a dominant axis + rough intensity to a two-word descriptor phrase.
# Format: (axis, threshold_high) -> adjective for DNA character string
DNA_AXIS_WORDS = {
    "brightness": {
        "high": ["crystalline", "bright", "piercing", "airy", "shimmering"],
        "low":  ["shadowed", "dark", "muffled", "veiled", "subterranean"],
    },
    "warmth": {
        "high": ["warm", "lush", "saturated", "syrupy", "honeyed"],
        "low":  ["cold", "clinical", "icy", "sparse", "austere"],
    },
    "movement": {
        "high": ["kinetic", "restless", "evolving", "spinning", "turbulent"],
        "low":  ["static", "frozen", "sustained", "meditative", "glacial"],
    },
    "density": {
        "high": ["dense", "layered", "thick", "stacked", "crowded"],
        "low":  ["sparse", "naked", "open", "minimal", "stripped"],
    },
    "space": {
        "high": ["cavernous", "spatial", "reverberant", "oceanic", "expansive"],
        "low":  ["dry", "close", "intimate", "direct", "anechoic"],
    },
    "aggression": {
        "high": ["aggressive", "driven", "raw", "abrasive", "feral"],
        "low":  ["gentle", "tender", "soft", "yielding", "hushed"],
    },
}


def _dna_character(dna: dict) -> str:
    """
    Return a 2-3 word DNA character description derived from dominant axes.
    dna is a dict with keys: brightness, warmth, movement, density, space, aggression (0.0-1.0).
    """
    if not dna:
        return "—"

    axis_order = ["brightness", "warmth", "movement", "density", "space", "aggression"]
    # Score each axis by how far it is from 0.5 (how opinionated)
    scored = []
    for axis in axis_order:
        val = dna.get(axis, 0.5)
        deviation = abs(val - 0.5)
        pole = "high" if val >= 0.5 else "low"
        scored.append((deviation, axis, pole, val))

    scored.sort(key=lambda x: x[0], reverse=True)

    words = []
    for _, axis, pole, _ in scored[:2]:
        bank = DNA_AXIS_WORDS.get(axis, {}).get(pole, [])
        if bank:
            # Pick deterministically (first word)
            words.append(bank[0])

    if len(words) < 2:
        return words[0] if words else "—"
    return f"{words[0]} {words[1]}"


# ---------------------------------------------------------------------------
# Engine detection helpers
# ---------------------------------------------------------------------------

ALL_ENGINES = [
    "ODDFELIX", "ODDOSCAR", "OVERDUB", "ODYSSEY", "OBLONG", "OBESE",
    "ONSET", "OVERWORLD", "OPAL", "ORBITAL", "ORGANON", "OUROBOROS",
    "OBSIDIAN", "ORIGAMI", "ORACLE", "OBSCURA", "OCEANIC", "OCELOT",
    "OVERBITE", "OPTIC", "OBLIQUE", "OSPREY", "OSTERIA", "OWLFISH",
    "OHM", "ORPHICA", "OBBLIGATO", "OTTONI", "OLE", "OVERLAP", "OUTWIT",
    "OMBRE", "ORCA", "OCTOPUS", "OSTINATO", "OPENSKY", "OCEANDEEP", "OUIE",
]


def _detect_engines_from_text(text: str) -> set:
    """
    Find any XO_OX engine names mentioned in a string (case-insensitive).
    Uses word-boundary-style matching: engine name must not be immediately
    preceded by a letter (avoids 'XOWLFISH' matching 'OWLFISH').
    """
    import re
    upper = text.upper()
    found = set()
    for e in ALL_ENGINES:
        # Require the engine name to be preceded by a non-alpha char (or start)
        if re.search(r"(?<![A-Z])" + re.escape(e), upper):
            found.add(e)
    return found


# ---------------------------------------------------------------------------
# XPM parsing
# ---------------------------------------------------------------------------

def _parse_xpm(path: Path) -> dict:
    """
    Parse a minimal summary from an MPC .xpm XML file.
    Returns: {name, type, sample_count, notes}
    """
    result = {"name": path.stem, "type": "Unknown", "sample_count": 0, "notes": ""}
    try:
        tree = ET.parse(path)
        root = tree.getroot()

        # Program name
        prog_name_el = root.find(".//ProgramName")
        if prog_name_el is not None and prog_name_el.text:
            result["name"] = prog_name_el.text.strip()

        # Program type attribute (Drum / Keygroup)
        prog_type = root.get("type", "") or root.find(".//Program")
        if hasattr(prog_type, "get"):
            prog_type = prog_type.get("type", "")
        if isinstance(prog_type, str):
            if "drum" in prog_type.lower():
                result["type"] = "Drum"
            elif "keygroup" in prog_type.lower() or "key" in prog_type.lower():
                result["type"] = "Keygroup"

        # Try alternative type detection from root tag or child
        if result["type"] == "Unknown":
            for el in root.iter():
                t = el.get("type", "")
                if "drum" in t.lower():
                    result["type"] = "Drum"
                    break
                elif "keygroup" in t.lower():
                    result["type"] = "Keygroup"
                    break

        # Count sample file references (SampleFile elements or similar)
        sample_files = set()
        for el in root.iter():
            for attr in ("SampleFile", "FileName", "file"):
                val = el.get(attr, "") or (el.text or "" if el.tag in ("SampleFile", "FileName") else "")
                if val and (val.endswith(".wav") or val.endswith(".WAV") or val.endswith(".aif")):
                    sample_files.add(val)
            # Also check child text nodes named SampleFile
            if el.tag in ("SampleFile", "FileName", "AudioFile"):
                txt = (el.text or "").strip()
                if txt:
                    sample_files.add(txt)

        result["sample_count"] = len(sample_files)

    except ET.ParseError:
        result["notes"] = "parse error"
    except Exception as e:
        result["notes"] = str(e)[:40]

    return result


# ---------------------------------------------------------------------------
# .xometa parsing
# ---------------------------------------------------------------------------

def _parse_xometa(path: Path) -> dict:
    """
    Parse a .xometa JSON preset file.
    Returns: {name, mood, engines, dna_character}
    """
    result = {
        "name": path.stem,
        "mood": "—",
        "engines": "—",
        "dna_character": "—",
    }
    try:
        with open(path, encoding="utf-8") as f:
            data = json.load(f)

        result["name"] = data.get("name", path.stem)

        mood = data.get("mood", "")
        if mood:
            result["mood"] = mood

        engines = data.get("engines", [])
        if isinstance(engines, list) and engines:
            result["engines"] = ", ".join(str(e).upper() for e in engines)
        elif isinstance(engines, str) and engines:
            result["engines"] = engines.upper()

        dna = data.get("sonicDNA", data.get("dna", {}))
        if isinstance(dna, dict):
            result["dna_character"] = _dna_character(dna)

    except Exception:
        pass

    return result


# ---------------------------------------------------------------------------
# expansion.json loading
# ---------------------------------------------------------------------------

def _load_expansion_json(pack_dir: Path) -> dict:
    """Load expansion.json from pack_dir if present."""
    path = pack_dir / "expansion.json"
    if not path.exists():
        return {}
    try:
        with open(path, encoding="utf-8") as f:
            return json.load(f)
    except Exception:
        return {}


# ---------------------------------------------------------------------------
# Directory scanning
# ---------------------------------------------------------------------------

def _scan_pack(pack_dir: Path) -> dict:
    """
    Scan the pack directory and return a summary dict with all findings.
    """
    xpm_files = sorted(pack_dir.rglob("*.xpm"))
    xometa_files = sorted(pack_dir.rglob("*.xometa"))
    all_files = list(pack_dir.rglob("*"))
    wav_files = [f for f in all_files if f.suffix.lower() in (".wav", ".aif", ".aiff")]

    total_bytes = sum(f.stat().st_size for f in wav_files if f.is_file())

    programs = [_parse_xpm(p) for p in xpm_files]
    presets = [_parse_xometa(p) for p in xometa_files]

    # Collect total sample count across all programs (de-duplicated)
    all_samples_referenced = set()
    for prog in programs:
        all_samples_referenced.update(range(prog["sample_count"]))  # placeholder — per-file is fine

    # Better: count unique WAV files in pack
    total_samples = len(wav_files)

    # Engine detection: from presets + program names + expansion.json text
    engines_found: set = set()
    for pr in presets:
        if pr["engines"] != "—":
            for e in pr["engines"].split(", "):
                engines_found.add(e.strip().upper())
    for prog in programs:
        engines_found.update(_detect_engines_from_text(prog["name"]))

    return {
        "programs": programs,
        "presets": presets,
        "total_samples": total_samples,
        "total_bytes": total_bytes,
        "engines_found": sorted(engines_found),
    }


# ---------------------------------------------------------------------------
# Size formatting
# ---------------------------------------------------------------------------

def _fmt_size(n_bytes: int) -> str:
    if n_bytes == 0:
        return "—"
    if n_bytes < 1024:
        return f"{n_bytes} B"
    if n_bytes < 1024 ** 2:
        return f"{n_bytes / 1024:.1f} KB"
    if n_bytes < 1024 ** 3:
        return f"{n_bytes / 1024**2:.1f} MB"
    return f"{n_bytes / 1024**3:.2f} GB"


# ---------------------------------------------------------------------------
# Markdown generation
# ---------------------------------------------------------------------------

def _md_table(headers: list, rows: list) -> str:
    """Generate a Markdown table from headers and rows (list of lists)."""
    sep = " | ".join(["---"] * len(headers))
    header_row = " | ".join(headers)
    lines = [f"| {header_row} |", f"| {sep} |"]
    for row in rows:
        cells = " | ".join(str(c) for c in row)
        lines.append(f"| {cells} |")
    return "\n".join(lines)


def generate_md(pack_dir: Path, expansion: dict, scan: dict) -> str:
    pack_name = expansion.get("name", pack_dir.name)
    version = expansion.get("version", "1.0.0")
    description = expansion.get("description", "")
    author = expansion.get("author", expansion.get("creator", "XO_OX Designs"))
    tags = expansion.get("tags", expansion.get("keywords", []))
    if isinstance(tags, str):
        tags = [t.strip() for t in tags.split(",") if t.strip()]
    usage_notes = expansion.get("usage_notes", expansion.get("usageNotes", ""))

    programs = scan["programs"]
    presets = scan["presets"]
    total_samples = scan["total_samples"]
    total_bytes = scan["total_bytes"]
    engines_found = scan["engines_found"]

    lines = []

    # ── 1. Pack Header ──────────────────────────────────────────────────────
    lines += [
        f"# {pack_name}",
        "",
        f"> **XO_OX** | v{version} | Author: {author}  ",
        f"> [xo-ox.org](https://xo-ox.org)",
        "",
        "---",
        "",
    ]

    if description:
        lines += [description, "", "---", ""]

    if tags:
        tag_str = "  ".join(f"`{t}`" for t in tags)
        lines += [f"**Tags:** {tag_str}", "", "---", ""]

    # ── 2. Contents at a Glance ─────────────────────────────────────────────
    lines += [
        "## Contents at a Glance",
        "",
        "| Detail         | Value |",
        "| -------------- | ----- |",
        f"| Programs       | {len(programs)} |",
        f"| Samples        | {total_samples} |",
        f"| Total Size     | {_fmt_size(total_bytes)} |",
        f"| Presets        | {len(presets)} |",
        f"| Engines        | {len(engines_found)} |",
        "",
        "---",
        "",
    ]

    # ── 3. Engine Roster ────────────────────────────────────────────────────
    lines += ["## Engine Roster", ""]
    if engines_found:
        for engine in engines_found:
            lines.append(f"- **{engine}**")
    else:
        lines.append("_No engines detected — check preset .xometa files or program names._")
    lines += ["", "---", ""]

    # ── 4. Program Directory ────────────────────────────────────────────────
    lines += ["## Program Directory", ""]
    if programs:
        rows = [
            [p["name"], p["type"], p["sample_count"] if p["sample_count"] else "—", p["notes"] or "—"]
            for p in programs
        ]
        lines.append(_md_table(["Program Name", "Type", "Samples", "Notes"], rows))
    else:
        lines.append("_No .xpm program files found in this directory._")
    lines += ["", "---", ""]

    # ── 5. Preset Directory ─────────────────────────────────────────────────
    if presets:
        lines += ["## Preset Directory", ""]
        rows = [
            [pr["name"], pr["mood"], pr["engines"], pr["dna_character"]]
            for pr in presets
        ]
        lines.append(_md_table(["Preset Name", "Mood", "Engine(s)", "DNA Character"], rows))
        lines += ["", "---", ""]

    # ── 6. How to Use ───────────────────────────────────────────────────────
    lines += [
        "## How to Use",
        "",
        "### Loading on MPC",
        "",
        "1. Connect your MPC (or open MPC Software / MPC Beats).",
        "2. Press **Menu** and navigate to **Expansions**.",
        "3. Select **Browse** and locate **" + pack_name + "**.",
        "4. Open the pack folder and tap the `.xpm` program you want to load.",
        "5. The program loads into the current track. Hit a pad to play.",
        "",
        "> If the pack does not appear, confirm the expansion folder is set in  ",
        "> **Preferences → Expansions Directory** and that the `.xpn` file has  ",
        "> been installed (drag to the MPC or use MPC Software's expansion installer).",
        "",
        "### Performance Macros",
        "",
        "XOceanus programs expose four Q-Link macros:",
        "",
        "| Macro     | Q-Link | Description                                    |",
        "| --------- | ------ | ---------------------------------------------- |",
        "| CHARACTER | Q1     | Core tonal character — Low: raw  /  High: shaped |",
        "| MOVEMENT  | Q2     | Rhythmic animation — Low: static  /  High: active |",
        "| COUPLING  | Q3     | feliX ↔ Oscar polarity sweep                   |",
        "| SPACE     | Q4     | Reverb / delay depth — Low: dry  /  High: deep  |",
        "",
        "### feliX-Oscar Axis",
        "",
        "Every XO_OX engine lives on a polarity axis:",
        "",
        "- **feliX** — bright, forward, harmonic, ordered, light",
        "- **Oscar** — dark, recessed, noisy, entropic, heavy",
        "",
        "The COUPLING macro sweeps between these poles in real time.",
        "",
    ]

    if usage_notes:
        lines += [
            "### Pack Notes",
            "",
            usage_notes,
            "",
        ]

    lines += ["---", ""]

    # ── 7. Compatibility ────────────────────────────────────────────────────
    lines += [
        "## Compatibility",
        "",
        "| Hardware         | Supported |",
        "| ---------------- | --------- |",
        "| MPC Live III     | Yes       |",
        "| MPC X            | Yes       |",
        "| MPC One          | Yes       |",
        "| MPC Key 61       | Yes       |",
        "| MPC Studio       | Yes       |",
        "| MPC Software 2.x | Yes       |",
        "",
        "**Firmware:** MPC 3.x or later recommended.",
        "",
        "---",
        "",
        f"*XO_OX | {pack_name} v{version} | [xo-ox.org](https://xo-ox.org)*",
        "",
    ]

    return "\n".join(lines)


# ---------------------------------------------------------------------------
# HTML wrapper
# ---------------------------------------------------------------------------

_HTML_TEMPLATE = """\
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>{title}</title>
<style>
  body {{ font-family: 'Helvetica Neue', Arial, sans-serif; max-width: 860px; margin: 40px auto;
         padding: 0 20px; color: #1a1a1a; background: #F8F6F3; line-height: 1.6; }}
  h1 {{ font-size: 2em; border-bottom: 3px solid #E9C46A; padding-bottom: .3em; }}
  h2 {{ font-size: 1.4em; color: #333; border-bottom: 1px solid #ddd; padding-bottom: .2em; margin-top: 2em; }}
  h3 {{ font-size: 1.1em; color: #555; }}
  table {{ border-collapse: collapse; width: 100%; margin: 1em 0; }}
  th {{ background: #E9C46A; color: #1a1a1a; padding: 8px 12px; text-align: left; }}
  td {{ padding: 7px 12px; border-bottom: 1px solid #e0e0e0; }}
  tr:nth-child(even) td {{ background: #f0ede8; }}
  code {{ background: #e8e0d8; padding: 2px 6px; border-radius: 3px; font-size: .9em; }}
  blockquote {{ border-left: 4px solid #E9C46A; margin: 0; padding: .5em 1em; color: #555; background: #fdf8ef; }}
  hr {{ border: none; border-top: 1px solid #ddd; margin: 2em 0; }}
  a {{ color: #0066cc; }}
  ul {{ padding-left: 1.5em; }}
</style>
</head>
<body>
<div id="content">
{body}
</div>
</body>
</html>
"""


def _md_to_simple_html(md: str) -> str:
    """
    Very minimal Markdown-to-HTML converter for the sections we generate.
    Handles: headings, bold, code, blockquote, hr, tables, lists, paragraphs.
    """
    import re

    lines = md.split("\n")
    html_lines = []
    in_table = False
    in_ul = False
    table_head_done = False

    def _inline(text: str) -> str:
        # Bold
        text = re.sub(r"\*\*(.+?)\*\*", r"<strong>\1</strong>", text)
        # Inline code
        text = re.sub(r"`([^`]+)`", r"<code>\1</code>", text)
        # Links
        text = re.sub(r"\[([^\]]+)\]\(([^)]+)\)", r'<a href="\2">\1</a>', text)
        # Italic
        text = re.sub(r"\*([^*]+)\*", r"<em>\1</em>", text)
        return text

    for line in lines:
        stripped = line.strip()

        # Close open list
        if in_ul and not stripped.startswith("- ") and not stripped.startswith("* "):
            html_lines.append("</ul>")
            in_ul = False

        # Table rows
        if stripped.startswith("|") and stripped.endswith("|"):
            cells = [c.strip() for c in stripped[1:-1].split("|")]
            if re.match(r"^[\-\s|]+$", stripped):
                # Separator row
                if not table_head_done:
                    html_lines.append("</tr></thead><tbody>")
                    table_head_done = True
                continue
            if not in_table:
                html_lines.append('<table>')
                html_lines.append("<thead><tr>")
                in_table = True
                table_head_done = False
                tag = "th"
            else:
                html_lines.append("<tr>")
                tag = "td"
            for c in cells:
                html_lines.append(f"<{tag}>{_inline(c)}</{tag}>")
            html_lines.append("</tr>")
            continue
        else:
            if in_table:
                html_lines.append("</tbody></table>")
                in_table = False
                table_head_done = False

        # Headings
        m = re.match(r"^(#{1,6})\s+(.+)$", stripped)
        if m:
            level = len(m.group(1))
            html_lines.append(f"<h{level}>{_inline(m.group(2))}</h{level}>")
            continue

        # HR
        if stripped in ("---", "***", "___"):
            html_lines.append("<hr>")
            continue

        # Blockquote
        if stripped.startswith(">"):
            content = stripped.lstrip("> ").rstrip("  ")
            html_lines.append(f"<blockquote>{_inline(content)}</blockquote>")
            continue

        # List item
        if stripped.startswith("- ") or stripped.startswith("* "):
            if not in_ul:
                html_lines.append("<ul>")
                in_ul = True
            html_lines.append(f"<li>{_inline(stripped[2:])}</li>")
            continue

        # Empty line
        if not stripped:
            if in_ul:
                html_lines.append("</ul>")
                in_ul = False
            html_lines.append("")
            continue

        # Paragraph
        html_lines.append(f"<p>{_inline(stripped)}</p>")

    if in_ul:
        html_lines.append("</ul>")
    if in_table:
        html_lines.append("</tbody></table>")

    return "\n".join(html_lines)


def generate_html(md_content: str, title: str) -> str:
    body = _md_to_simple_html(md_content)
    return _HTML_TEMPLATE.format(title=title, body=body)


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def _parse_args(argv=None):
    parser = argparse.ArgumentParser(
        description="Generate a catalog document for an XPN pack directory."
    )
    parser.add_argument("pack_dir", help="Path to the pack directory to scan")
    parser.add_argument(
        "--format", choices=["md", "html"], default="md",
        help="Output format: md (default) or html"
    )
    parser.add_argument(
        "--output", default=None,
        help="Output file path (default: CATALOG.md or CATALOG.html inside pack_dir)"
    )
    return parser.parse_args(argv)


def main(argv=None):
    args = _parse_args(argv)

    pack_dir = Path(args.pack_dir).resolve()
    if not pack_dir.is_dir():
        print(f"ERROR: '{pack_dir}' is not a directory.", file=sys.stderr)
        sys.exit(1)

    expansion = _load_expansion_json(pack_dir)
    scan = _scan_pack(pack_dir)

    # Supplement engine detection from expansion.json text
    for field in ("name", "description", "tags", "engines"):
        val = expansion.get(field, "")
        if isinstance(val, list):
            val = " ".join(str(v) for v in val)
        scan["engines_found"] = sorted(
            set(scan["engines_found"]) | _detect_engines_from_text(str(val))
        )

    md_content = generate_md(pack_dir, expansion, scan)

    fmt = args.format
    if args.output:
        out_path = Path(args.output)
    else:
        ext = ".md" if fmt == "md" else ".html"
        out_path = pack_dir / f"CATALOG{ext}"

    if fmt == "html":
        pack_name = expansion.get("name", pack_dir.name)
        content = generate_html(md_content, title=pack_name)
    else:
        content = md_content

    out_path.parent.mkdir(parents=True, exist_ok=True)
    with open(out_path, "w", encoding="utf-8") as f:
        f.write(content)

    # Summary to stdout
    programs = scan["programs"]
    presets = scan["presets"]
    print(f"Catalog written: {out_path}")
    print(f"  Programs : {len(programs)}")
    print(f"  Samples  : {scan['total_samples']}")
    print(f"  Presets  : {len(presets)}")
    print(f"  Engines  : {', '.join(scan['engines_found']) or '(none detected)'}")
    print(f"  Size     : {_fmt_size(scan['total_bytes'])}")


if __name__ == "__main__":
    main()
