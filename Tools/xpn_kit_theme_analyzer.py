#!/usr/bin/env python3
"""
xpn_kit_theme_analyzer.py — Sonic Theme Analyzer for drum kit .xpn packs
XO_OX Designs

Analyzes each drum kit program in an MPC .xpn and determines its sonic theme
(vintage / modern / lo-fi / electronic / acoustic / hybrid) based on WAV
filenames and XPM parameter values.

Usage:
    python xpn_kit_theme_analyzer.py <pack.xpn> [--format text|json] [--program NAME]

Exit codes:
    0  Analysis complete
    1  File not found or not a valid .xpn
    2  No drum programs found
"""

import argparse
import json
import sys
import zipfile
from pathlib import Path
from typing import Optional
from xml.etree import ElementTree as ET

# ---------------------------------------------------------------------------
# Theme keyword tables
# ---------------------------------------------------------------------------

THEME_KEYWORDS: dict[str, list[str]] = {
    "vintage":    ["808", "909", "cr78", "linn", "sp12", "sp1200", "mpc60",
                   "vintage", "old", "worn", "tape"],
    "modern":     ["clean", "punchy", "tight", "crisp", "studio"],
    "lo-fi":      ["dirty", "lofi", "lo-fi", "vinyl", "crackle", "dusty",
                   "gritty", "bit"],
    "electronic": ["synth", "electronic", "digital", "glitch", "noise", "sub"],
    "acoustic":   ["acoustic", "live", "room", "overhead", "natural", "wood"],
}

THEMES = list(THEME_KEYWORDS.keys())

# GM voice detection — maps pad note number → voice label
# MPC convention: kick on C1 (36), snare on D1 (38), etc.
GM_VOICE_NOTE_MAP: dict[str, list[int]] = {
    "Kick":  [35, 36],
    "Snare": [38, 40],
    "Hat":   [42, 44, 46],
    "Clap":  [39],
    "Tom":   [41, 43, 45, 47, 48, 50],
    "Perc":  [49, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64,
              65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79],
    "FX":    [80, 81],
}

# Also check voice label by name substring for when note mapping misses
GM_VOICE_NAME_MAP: dict[str, list[str]] = {
    "Kick":  ["kick", "bd", "bass drum", "bassdrum"],
    "Snare": ["snare", "sd", "snr"],
    "Hat":   ["hat", "hh", "hihat", "hi-hat", "cymbal", "ride"],
    "Clap":  ["clap", "clp", "handclap"],
    "Tom":   ["tom"],
    "Perc":  ["perc", "conga", "bongo", "shaker", "tamb", "cowbell", "rim",
              "clave", "guiro", "wood", "block", "triangle"],
    "FX":    ["fx", "effect", "reverse", "noise", "sweep", "impact"],
}

# ---------------------------------------------------------------------------
# XPM parsing helpers
# ---------------------------------------------------------------------------

def _text(el: ET.Element, tag: str, default: str = "") -> str:
    child = el.find(tag)
    return child.text.strip() if child is not None and child.text else default


def _float(el: ET.Element, tag: str, default: float = 0.0) -> float:
    try:
        return float(_text(el, tag, str(default)))
    except ValueError:
        return default


def parse_xpm(xpm_text: str) -> dict:
    """Parse an XPM XML string into a structured dict."""
    root = ET.fromstring(xpm_text)

    program_type = _text(root, "Type", "").lower()
    program_name = _text(root, "Name", "Unnamed")

    pads: list[dict] = []
    for pad_el in root.iter("Pad"):
        pad_num = pad_el.get("number", "0")
        note = int(_text(pad_el, "Note", "0"))
        layer_els = list(pad_el.iter("Layer"))
        samples: list[str] = []
        volumes: list[float] = []
        for layer in layer_els:
            sfile = _text(layer, "SampleFile", "")
            if sfile:
                samples.append(sfile)
            vol = _float(layer, "Volume", 100.0)
            volumes.append(vol)

        # Grab a few key params if present at pad level
        attack   = _float(pad_el, "Attack", 0.0)
        cutoff   = _float(pad_el, "Cutoff", 127.0)
        resonance = _float(pad_el, "Resonance", 0.0)
        drive    = _float(pad_el, "Drive", 0.0)

        if samples:
            pads.append({
                "number": pad_num,
                "note": note,
                "samples": samples,
                "volumes": volumes,
                "attack": attack,
                "cutoff": cutoff,
                "resonance": resonance,
                "drive": drive,
            })

    return {
        "name": program_name,
        "type": program_type,
        "pads": pads,
    }


# ---------------------------------------------------------------------------
# Theme scoring
# ---------------------------------------------------------------------------

def score_from_filenames(pads: list[dict]) -> dict[str, int]:
    """Count keyword hits per theme across all sample filenames."""
    counts: dict[str, int] = {t: 0 for t in THEMES}
    for pad in pads:
        for sample_path in pad["samples"]:
            name_lower = Path(sample_path).stem.lower()
            for theme, keywords in THEME_KEYWORDS.items():
                for kw in keywords:
                    if kw in name_lower:
                        counts[theme] += 1
    return counts


def score_from_params(pads: list[dict]) -> dict[str, float]:
    """Derive fractional bonus scores from XPM parameter values."""
    bonuses: dict[str, float] = {t: 0.0 for t in THEMES}
    if not pads:
        return bonuses

    avg_cutoff  = sum(p["cutoff"]  for p in pads) / len(pads)
    avg_volume  = sum((sum(p["volumes"]) / max(len(p["volumes"]), 1)) for p in pads) / len(pads)
    avg_attack  = sum(p["attack"]  for p in pads) / len(pads)
    avg_drive   = sum(p["drive"]   for p in pads) / len(pads)

    # High drive + low cutoff → vintage / lo-fi
    if avg_drive > 50:
        bonuses["vintage"] += 1.5
        bonuses["lo-fi"]   += 1.5
    if avg_cutoff < 60:
        bonuses["vintage"] += 1.0
        bonuses["lo-fi"]   += 1.0

    # High cutoff + any drive → modern / electronic
    if avg_cutoff > 100:
        bonuses["modern"]     += 1.5
        bonuses["electronic"] += 0.5

    # High average volume → punchy/modern
    if avg_volume > 120:
        bonuses["modern"] += 1.0

    # Slow attack → acoustic / loose
    if avg_attack > 10:
        bonuses["acoustic"] += 1.5

    return bonuses


def compute_theme_profile(pads: list[dict]) -> dict[str, float]:
    """Return a 0–100 confidence score per theme."""
    kw_counts = score_from_filenames(pads)
    param_bonuses = score_from_params(pads)

    raw: dict[str, float] = {}
    for t in THEMES:
        raw[t] = kw_counts[t] + param_bonuses[t]

    total = sum(raw.values())
    if total == 0:
        # No signal — mark hybrid equally
        return {t: 20.0 for t in THEMES}

    # Normalise to 0–100
    max_raw = max(raw.values())
    return {t: round(raw[t] / max_raw * 100, 1) for t in THEMES}


# ---------------------------------------------------------------------------
# Voice coverage detection
# ---------------------------------------------------------------------------

def detect_voice_coverage(pads: list[dict]) -> dict[str, bool]:
    coverage = {v: False for v in GM_VOICE_NOTE_MAP}
    for pad in pads:
        note = pad["note"]
        # Check by note number
        for voice, notes in GM_VOICE_NOTE_MAP.items():
            if note in notes:
                coverage[voice] = True

        # Check by sample name
        for sample_path in pad["samples"]:
            name_lower = Path(sample_path).stem.lower()
            for voice, keywords in GM_VOICE_NAME_MAP.items():
                if any(kw in name_lower for kw in keywords):
                    coverage[voice] = True
    return coverage


# ---------------------------------------------------------------------------
# Text rendering
# ---------------------------------------------------------------------------

BAR_WIDTH = 10


def render_bar(pct: float) -> str:
    filled = round(pct / 100 * BAR_WIDTH)
    return "█" * filled + "░" * (BAR_WIDTH - filled)


def format_text(results: list[dict]) -> str:
    lines: list[str] = []
    for kit in results:
        lines.append(f"\nKIT: {kit['name']}\n")
        lines.append("Theme Profile:")
        for theme, score in sorted(kit["theme_profile"].items(), key=lambda x: -x[1]):
            bar = render_bar(score)
            lines.append(f"  {theme.capitalize():<12} {bar}  {score:.0f}%")

        primary   = kit["primary_theme"].upper()
        secondary = kit["secondary_theme"].upper()
        lines.append(f"\nPrimary Theme: {primary} | Secondary: {secondary}")

        cov = kit["voice_coverage"]
        cov_str = " | ".join(
            f"{v} {'✓' if present else '✗'}" for v, present in cov.items()
        )
        lines.append(f"Voice Coverage: {cov_str}")

        lines.append(
            f"\n{kit['assigned_pads']}/{kit['total_pads']} pads assigned"
            f" | Sample count: {kit['sample_count']}"
        )
        lines.append("-" * 56)
    return "\n".join(lines)


# ---------------------------------------------------------------------------
# Main analysis
# ---------------------------------------------------------------------------

def analyze_xpn(xpn_path: Path, program_filter: Optional[str] = None) -> list[dict]:
    results: list[dict] = []

    try:
        zf = zipfile.ZipFile(xpn_path, "r")
    except (zipfile.BadZipFile, FileNotFoundError) as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        sys.exit(1)

    with zf:
        xpm_names = [n for n in zf.namelist() if n.lower().endswith(".xpm")]
        if not xpm_names:
            print("ERROR: No .xpm files found in pack.", file=sys.stderr)
            sys.exit(2)

        for xpm_name in sorted(xpm_names):
            try:
                xpm_text = zf.read(xpm_name).decode("utf-8", errors="replace")
                program  = parse_xpm(xpm_text)
            except ET.ParseError as exc:
                print(f"WARNING: Skipping {xpm_name} — parse error: {exc}", file=sys.stderr)
                continue

            # Only include drum-type programs
            if program["type"] not in ("drumprogram", "drum", ""):
                continue

            name = program["name"]
            if program_filter and program_filter.lower() not in name.lower():
                continue

            pads = program["pads"]
            assigned = [p for p in pads if p["samples"]]
            all_samples = [s for p in assigned for s in p["samples"]]

            profile  = compute_theme_profile(assigned)
            sorted_t = sorted(profile.items(), key=lambda x: -x[1])
            primary   = sorted_t[0][0] if sorted_t else "hybrid"
            secondary = sorted_t[1][0] if len(sorted_t) > 1 else "hybrid"

            # If top two are within 10 points of each other → hybrid
            if len(sorted_t) > 1 and (sorted_t[0][1] - sorted_t[1][1]) < 10:
                secondary = sorted_t[1][0]
                if sorted_t[0][1] < 30:
                    primary = "hybrid"

            results.append({
                "name":           name,
                "xpm_file":       xpm_name,
                "theme_profile":  profile,
                "primary_theme":  primary,
                "secondary_theme": secondary,
                "voice_coverage": detect_voice_coverage(assigned),
                "assigned_pads":  len(assigned),
                "total_pads":     len(pads) if pads else 16,
                "sample_count":   len(all_samples),
            })

    return results


# ---------------------------------------------------------------------------
# CLI entry point
# ---------------------------------------------------------------------------

def main() -> None:
    parser = argparse.ArgumentParser(
        description="Analyze sonic theme of drum kits inside a .xpn pack."
    )
    parser.add_argument("xpn", help="Path to .xpn pack file")
    parser.add_argument(
        "--format", choices=["text", "json"], default="text",
        help="Output format (default: text)"
    )
    parser.add_argument(
        "--program", metavar="NAME", default=None,
        help="Filter to programs whose name contains NAME (case-insensitive)"
    )
    args = parser.parse_args()

    xpn_path = Path(args.xpn)
    if not xpn_path.exists():
        print(f"ERROR: File not found: {xpn_path}", file=sys.stderr)
        sys.exit(1)

    results = analyze_xpn(xpn_path, program_filter=args.program)

    if not results:
        print("No matching drum programs found.", file=sys.stderr)
        sys.exit(2)

    if args.format == "json":
        print(json.dumps(results, indent=2))
    else:
        print(format_text(results))


if __name__ == "__main__":
    main()
