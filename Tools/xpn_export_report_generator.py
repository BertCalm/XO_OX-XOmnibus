#!/usr/bin/env python3
"""
XPN Export Report Generator — XO_OX Designs

Reads an oxport.py output directory and generates a detailed EXPORT_REPORT.md
summarising everything produced in a pipeline run.

Directory layout expected (per oxport.py output):
    {output_dir}/
    ├── {engine}/
    │   ├── Programs/*.xpm          drum or keygroup programs
    │   ├── Samples/                sample WAV files (flat or in sub-dirs)
    │   ├── expansion.json          Akai-style expansion manifest
    │   └── bundle_manifest.json    XO_OX bundle manifest
    └── *.xpn                       final packaged ZIP archive(s)

CLI:
    python xpn_export_report_generator.py --output-dir ./dist/Onset
    python xpn_export_report_generator.py --output-dir ./dist/Onset --format text
    python xpn_export_report_generator.py --output-dir ./dist/Onset --write
    python xpn_export_report_generator.py --output-dir ./dist/Onset --format markdown --write

Options:
    --output-dir PATH   Root of the oxport output directory  [required]
    --format            markdown (default) | text
    --write             Save EXPORT_REPORT.md into the output directory
"""

import argparse
import json
import os
import struct
import sys
import zipfile
import zlib
from datetime import datetime
from pathlib import Path
from typing import Any, Optional


# ---------------------------------------------------------------------------
# Low-level helpers
# ---------------------------------------------------------------------------

def _fmt_bytes(n: int) -> str:
    """Human-readable file size."""
    for unit in ("B", "KB", "MB", "GB"):
        if n < 1024:
            return f"{n:.1f} {unit}"
        n /= 1024
    return f"{n:.1f} TB"


def _dir_mtime(path: Path) -> str:
    """Return ISO-formatted mtime of a directory (or '—' if unavailable)."""
    try:
        ts = path.stat().st_mtime
        return datetime.fromtimestamp(ts).strftime("%Y-%m-%d %H:%M:%S")
    except OSError:
        return "—"


def _read_json(path: Path) -> dict:
    """Load JSON file; return empty dict on any error."""
    try:
        with open(path, encoding="utf-8") as f:
            return json.load(f)
    except Exception:
        return {}


def _png_dimensions(path: Path) -> tuple[Optional[int], Optional[int]]:
    """
    Parse PNG IHDR chunk to extract (width, height) without Pillow.
    Returns (None, None) if the file is not a valid PNG.
    """
    PNG_SIG = b"\x89PNG\r\n\x1a\n"
    try:
        with open(path, "rb") as f:
            sig = f.read(8)
            if sig != PNG_SIG:
                return None, None
            # IHDR is always the first chunk: 4-byte length + 4-byte type + 13-byte data
            f.read(4)           # chunk length
            chunk_type = f.read(4)
            if chunk_type != b"IHDR":
                return None, None
            data = f.read(13)
            width, height = struct.unpack(">II", data[:8])
            return width, height
    except Exception:
        return None, None


def _wav_info(path: Path) -> dict:
    """
    Extract sample_rate and bit_depth from a WAV file header (no wave module).
    Returns dict with keys sample_rate, bit_depth (int or None on failure).
    """
    try:
        with open(path, "rb") as f:
            riff = f.read(12)
            if riff[:4] != b"RIFF" or riff[8:12] != b"WAVE":
                return {}
            while True:
                hdr = f.read(8)
                if len(hdr) < 8:
                    break
                chunk_id = hdr[:4]
                chunk_size = struct.unpack("<I", hdr[4:])[0]
                if chunk_id == b"fmt ":
                    fmt = f.read(chunk_size)
                    sample_rate = struct.unpack("<I", fmt[4:8])[0]
                    bit_depth = struct.unpack("<H", fmt[14:16])[0]
                    return {"sample_rate": sample_rate, "bit_depth": bit_depth}
                else:
                    f.seek(chunk_size, 1)
    except Exception as exc:
        print(f"[WARN] Reading WAV format info from {path}: {exc}", file=sys.stderr)
    return {}


def _xpm_stats(path: Path) -> dict:
    """
    Parse an XPM file (XML) to extract program type, pad count, and max
    velocity layer count — using only stdlib xml.etree.ElementTree.
    """
    import xml.etree.ElementTree as ET
    result = {"program_type": "unknown", "pad_count": 0, "velocity_layers": 0}
    try:
        tree = ET.parse(path)
        root = tree.getroot()
        # <Program type="Drum"> or <Program type="Keygroup">
        prog = root if root.tag == "Program" else root.find(".//Program")
        if prog is None:
            return result
        ptype = prog.get("type", "unknown")
        result["program_type"] = ptype.lower()

        if ptype.lower() == "drum":
            instruments = prog.findall(".//Instrument")
            result["pad_count"] = len(instruments)
            # Count velocity layers inside the first active instrument
            max_layers = 0
            for instr in instruments:
                layers = instr.findall(".//Layer")
                max_layers = max(max_layers, len(layers))
            result["velocity_layers"] = max_layers
        else:
            # Keygroup: count KeyGroup elements and layers
            keygroups = prog.findall(".//KeyGroup")
            result["pad_count"] = len(keygroups)
            max_layers = 0
            for kg in keygroups:
                layers = kg.findall(".//Layer")
                max_layers = max(max_layers, len(layers))
            result["velocity_layers"] = max_layers
    except Exception as exc:
        print(f"[WARN] Parsing XPM program structure from {path}: {exc}", file=sys.stderr)
    return result


# ---------------------------------------------------------------------------
# Data collection
# ---------------------------------------------------------------------------

def collect(output_dir: Path) -> dict[str, Any]:
    """
    Walk the output directory and collect all data needed for the report.
    Returns a structured dict with keys: header, programs, samples,
    cover_art, manifests, pack_summary.
    """
    data: dict[str, Any] = {}

    # ---- Locate engine sub-directory -----------------------------------
    # oxport.py writes into {output_dir}/{EngineName}/
    # If output_dir itself contains Programs/, treat it as the engine dir.
    engine_dir = output_dir
    candidates = [p for p in output_dir.iterdir() if p.is_dir()]
    for c in candidates:
        if (c / "Programs").is_dir() or (c / "expansion.json").exists():
            engine_dir = c
            break

    engine_name = engine_dir.name

    # ---- Manifests -----------------------------------------------------
    expansion_json = _read_json(engine_dir / "expansion.json")
    bundle_json    = _read_json(engine_dir / "bundle_manifest.json")

    # ---- Header --------------------------------------------------------
    pack_name  = (bundle_json.get("name")
                  or expansion_json.get("Name")
                  or f"{engine_name} Pack")
    version    = (bundle_json.get("version")
                  or expansion_json.get("Version")
                  or "—")
    run_ts     = bundle_json.get("built_at") or _dir_mtime(engine_dir)

    data["header"] = {
        "engine":    engine_name,
        "pack_name": pack_name,
        "version":   version,
        "timestamp": run_ts,
    }

    # ---- Programs ------------------------------------------------------
    programs_dir = engine_dir / "Programs"
    programs = []
    if programs_dir.is_dir():
        for xpm in sorted(programs_dir.glob("*.xpm")):
            stats = _xpm_stats(xpm)
            programs.append({
                "name":             xpm.stem,
                "program_type":     stats["program_type"],
                "pad_count":        stats["pad_count"],
                "velocity_layers":  stats["velocity_layers"],
                "size":             xpm.stat().st_size,
            })
    data["programs"] = programs

    # ---- Samples -------------------------------------------------------
    samples_dir = engine_dir / "Samples"
    sample_files = list(samples_dir.rglob("*.wav")) if samples_dir.is_dir() else []
    sr_counts: dict[int, int] = {}
    bd_counts: dict[int, int] = {}
    total_sample_bytes = 0
    for wav in sample_files:
        total_sample_bytes += wav.stat().st_size
        info = _wav_info(wav)
        if "sample_rate" in info:
            sr_counts[info["sample_rate"]] = sr_counts.get(info["sample_rate"], 0) + 1
        if "bit_depth" in info:
            bd_counts[info["bit_depth"]] = bd_counts.get(info["bit_depth"], 0) + 1

    data["samples"] = {
        "count":       len(sample_files),
        "total_bytes": total_sample_bytes,
        "sr_counts":   sr_counts,
        "bd_counts":   bd_counts,
    }

    # ---- Cover art -----------------------------------------------------
    art_info = {}
    art_candidates = list(engine_dir.glob("*.png")) + list(output_dir.glob("*.png"))
    if art_candidates:
        art = art_candidates[0]
        w, h = _png_dimensions(art)
        art_info = {
            "filename": art.name,
            "width":    w,
            "height":   h,
            "size":     art.stat().st_size,
        }
    data["cover_art"] = art_info

    # ---- Manifests (key fields) ----------------------------------------
    dna = bundle_json.get("sonic_dna") or bundle_json.get("dna") or {}
    data["manifests"] = {
        "expansion_name":    expansion_json.get("Name", "—"),
        "expansion_version": expansion_json.get("Version", "—"),
        "expansion_author":  expansion_json.get("Author", "—"),
        "bundle_version":    bundle_json.get("version", "—"),
        "bundle_mood":       bundle_json.get("mood", "—"),
        "bundle_pack_id":    bundle_json.get("pack_id", "—"),
        "sonic_dna":         dna,
    }

    # ---- Pack summary (XPN archive) ------------------------------------
    xpn_files = list(output_dir.glob("*.xpn"))
    pack_info: dict[str, Any] = {}
    if xpn_files:
        xpn = xpn_files[0]
        xpn_size = xpn.stat().st_size
        # Compute uncompressed size via zipfile
        try:
            with zipfile.ZipFile(xpn, "r") as zf:
                uncompressed = sum(i.file_size for i in zf.infolist())
        except Exception:
            uncompressed = 0
        ratio = (1 - xpn_size / uncompressed) * 100 if uncompressed else 0.0
        pack_info = {
            "filename":         xpn.name,
            "size":             xpn_size,
            "uncompressed":     uncompressed,
            "compression_pct":  ratio,
            "program_count":    len(programs),
        }
    data["pack_summary"] = pack_info

    return data


# ---------------------------------------------------------------------------
# Report rendering
# ---------------------------------------------------------------------------

def _md_table(headers: list[str], rows: list[list[str]]) -> str:
    widths = [max(len(h), max((len(r[i]) for r in rows), default=0))
              for i, h in enumerate(headers)]
    def row_str(cells: list[str]) -> str:
        return "| " + " | ".join(c.ljust(widths[i]) for i, c in enumerate(cells)) + " |"
    sep = "| " + " | ".join("-" * w for w in widths) + " |"
    lines = [row_str(headers), sep] + [row_str(r) for r in rows]
    return "\n".join(lines)


def render_markdown(data: dict[str, Any]) -> str:
    h = data["header"]
    lines = [
        f"# XPN Export Report — {h['pack_name']}",
        "",
        f"**Engine:** {h['engine']}  ",
        f"**Version:** {h['version']}  ",
        f"**Run:** {h['timestamp']}  ",
        "",
        "---",
        "",
        "## Programs",
        "",
    ]

    programs = data["programs"]
    if programs:
        rows = [
            [p["name"], p["program_type"], str(p["pad_count"]),
             str(p["velocity_layers"]), _fmt_bytes(p["size"])]
            for p in programs
        ]
        lines.append(_md_table(
            ["Name", "Type", "Pads", "Vel Layers", "Size"], rows))
    else:
        lines.append("_No .xpm files found._")

    s = data["samples"]
    lines += [
        "",
        "---",
        "",
        "## Samples",
        "",
        f"- **Count:** {s['count']}",
        f"- **Total size:** {_fmt_bytes(s['total_bytes'])}",
    ]
    if s["sr_counts"]:
        sr_str = ", ".join(f"{sr} Hz × {n}" for sr, n in sorted(s["sr_counts"].items()))
        lines.append(f"- **Sample rates:** {sr_str}")
    if s["bd_counts"]:
        bd_str = ", ".join(f"{bd}-bit × {n}" for bd, n in sorted(s["bd_counts"].items()))
        lines.append(f"- **Bit depths:** {bd_str}")

    art = data["cover_art"]
    lines += ["", "---", "", "## Cover Art", ""]
    if art:
        dim = f"{art['width']} × {art['height']} px" if art.get("width") else "dimensions unknown"
        lines += [
            f"- **File:** `{art['filename']}`",
            f"- **Dimensions:** {dim}",
            f"- **Size:** {_fmt_bytes(art['size'])}",
        ]
    else:
        lines.append("_No cover art PNG found._")

    m = data["manifests"]
    lines += ["", "---", "", "## Manifests", "", "### expansion.json", ""]
    lines += [
        f"- **Name:** {m['expansion_name']}",
        f"- **Version:** {m['expansion_version']}",
        f"- **Author:** {m['expansion_author']}",
        "",
        "### bundle_manifest.json",
        "",
        f"- **Version:** {m['bundle_version']}",
        f"- **Mood:** {m['bundle_mood']}",
        f"- **Pack ID:** {m['bundle_pack_id']}",
    ]
    if m["sonic_dna"]:
        lines += ["", "**Sonic DNA:**", ""]
        for dim, val in m["sonic_dna"].items():
            lines.append(f"- {dim}: {val}")

    ps = data["pack_summary"]
    lines += ["", "---", "", "## Pack Summary", ""]
    if ps:
        lines += [
            f"- **Archive:** `{ps['filename']}`",
            f"- **ZIP size:** {_fmt_bytes(ps['size'])}",
            f"- **Uncompressed:** {_fmt_bytes(ps['uncompressed'])}",
            f"- **Compression:** {ps['compression_pct']:.1f}%",
            f"- **Program count:** {ps['program_count']}",
        ]
    else:
        lines.append("_No .xpn archive found._")

    lines.append("")
    return "\n".join(lines)


def render_text(data: dict[str, Any]) -> str:
    """Plain-text version (section headers + aligned columns)."""
    md = render_markdown(data)
    # Strip Markdown syntax for a clean plain-text view
    out = []
    for line in md.splitlines():
        line = (line.replace("**", "")
                    .replace("_No ", "  No ")
                    .replace("._", ".")
                    .replace("`", ""))
        if line.startswith("# "):
            out.append("=" * 60)
            out.append(line[2:].upper())
            out.append("=" * 60)
        elif line.startswith("## "):
            out.append("")
            out.append(line[3:].upper())
            out.append("-" * 40)
        elif line.startswith("### "):
            out.append("")
            out.append(line[4:])
        else:
            out.append(line)
    return "\n".join(out)


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def main() -> None:
    parser = argparse.ArgumentParser(
        description="Generate an export report for an oxport.py output directory."
    )
    parser.add_argument("--output-dir", required=True,
                        help="Root directory produced by oxport.py")
    parser.add_argument("--format", choices=["markdown", "text"],
                        default="markdown",
                        help="Output format (default: markdown)")
    parser.add_argument("--write", action="store_true",
                        help="Write EXPORT_REPORT.md into the output directory")
    args = parser.parse_args()

    output_dir = Path(args.output_dir).resolve()
    if not output_dir.is_dir():
        print(f"ERROR: {output_dir} is not a directory", file=sys.stderr)
        sys.exit(1)

    data   = collect(output_dir)
    report = render_markdown(data) if args.format == "markdown" else render_text(data)

    print(report)

    if args.write:
        report_path = output_dir / "EXPORT_REPORT.md"
        report_path.write_text(render_markdown(data), encoding="utf-8")
        print(f"\n[report saved → {report_path}]", file=sys.stderr)


if __name__ == "__main__":
    main()
