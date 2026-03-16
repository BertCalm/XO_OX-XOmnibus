#!/usr/bin/env python3
"""
xpn_program_type_classifier.py — XPN program type classifier.

Classifies each XPM program in a .xpn pack as one of:
  DRUM_KIT | MELODIC | BASS | LEAD | PAD | TEXTURE | UNKNOWN

Classification uses sample names, root note distribution, and velocity layer
structure. No external dependencies — stdlib only (zipfile + xml.etree).

Usage:
    python xpn_program_type_classifier.py <pack.xpn> [--format text|json] [--verbose]

Output (text):
    PACK: Machine Gun Reef
      Program 1: KICK PATTERNS          → DRUM_KIT     (95%)
      Program 2: BASS LAYER 01          → BASS         (82%)
      ...
    Summary: 1 DRUM_KIT | 2 BASS | 1 PAD | 1 TEXTURE
"""

import argparse
import json
import sys
import zipfile
import xml.etree.ElementTree as ET
from pathlib import Path
from typing import Any


# ---------------------------------------------------------------------------
# Constants
# ---------------------------------------------------------------------------

GM_DRUM_LOW = 35   # Acoustic Bass Drum
GM_DRUM_HIGH = 51  # Ride Cymbal 1

DRUM_KEYWORDS = {
    "kick", "snare", "hat", "clap", "tom", "perc",
    "rim", "crash", "ride", "shaker",
}

BASS_KEYWORDS     = {"bass", "sub", "808", "low", "bss"}
LEAD_KEYWORDS     = {"lead", "synth", "mono", "arp", "seq"}
PAD_KEYWORDS      = {"pad", "string", "choir", "atmo", "wash", "shimmer", "layer"}
TEXTURE_KEYWORDS  = {"texture", "noise", "drone", "ambient", "grain", "glitch"}

CATEGORY_ORDER = ["DRUM_KIT", "MELODIC", "BASS", "LEAD", "PAD", "TEXTURE", "UNKNOWN"]


# ---------------------------------------------------------------------------
# XPN parsing helpers
# ---------------------------------------------------------------------------

def _xpms_from_zip(path: Path) -> list[tuple[str, str]]:
    """
    Return list of (filename, xml_text) for every .xpm in the archive.
    """
    results = []
    with zipfile.ZipFile(path, "r") as zf:
        for name in zf.namelist():
            if name.lower().endswith(".xpm"):
                try:
                    results.append((name, zf.read(name).decode("utf-8", errors="replace")))
                except Exception:
                    pass
    return results


def _parse_xpm(xml_text: str) -> dict[str, Any]:
    """
    Parse an XPM XML string. Returns:
      name        : str  — program name
      keygroups   : list of dicts with keys: root_note, vel_start, vel_end, sample_name
    """
    result: dict[str, Any] = {"name": "", "keygroups": []}
    try:
        root = ET.fromstring(xml_text)
    except ET.ParseError:
        return result

    # Program name is usually the root element's "name" attribute or a child tag
    result["name"] = root.get("name", "") or root.findtext("Name") or ""

    for kg in root.iter("KeyGroup"):
        # Root note
        root_note_str = kg.findtext("RootNote") or kg.get("rootNote", "")
        try:
            root_note = int(root_note_str)
        except (ValueError, TypeError):
            root_note = -1

        # Velocity range
        try:
            vel_start = int(kg.findtext("VelStart") or kg.get("velStart", "0"))
        except (ValueError, TypeError):
            vel_start = 0
        try:
            vel_end = int(kg.findtext("VelEnd") or kg.get("velEnd", "127"))
        except (ValueError, TypeError):
            vel_end = 127

        # Sample name — look for SamplePath, SampleFile, or FileName child
        sample_name = (
            kg.findtext("SamplePath")
            or kg.findtext("SampleFile")
            or kg.findtext("FileName")
            or ""
        )
        # Strip to basename without extension for keyword matching
        sample_name = Path(sample_name).stem.lower()

        result["keygroups"].append({
            "root_note":   root_note,
            "vel_start":   vel_start,
            "vel_end":     vel_end,
            "sample_name": sample_name,
        })

    return result


# ---------------------------------------------------------------------------
# Classification logic
# ---------------------------------------------------------------------------

def _keyword_hits(sample_names: list[str], keywords: set[str]) -> int:
    """Count how many distinct keywords appear across all sample names."""
    found: set[str] = set()
    for name in sample_names:
        for kw in keywords:
            if kw in name:
                found.add(kw)
    return len(found)


def classify_program(prog: dict[str, Any]) -> dict[str, Any]:
    """
    Classify a parsed XPM program dict.
    Returns: { type, confidence, indicators }
    """
    kgs = prog["keygroups"]
    sample_names = [kg["sample_name"] for kg in kgs]
    root_notes = [kg["root_note"] for kg in kgs if kg["root_note"] >= 0]

    n_kg = len(kgs)
    indicators: list[str] = []

    # -----------------------------------------------------------------------
    # 1. Drum detection
    # -----------------------------------------------------------------------
    drum_score = 0

    # GM note range check
    if root_notes:
        notes_in_gm = sum(1 for n in root_notes if GM_DRUM_LOW <= n <= GM_DRUM_HIGH)
        gm_fraction = notes_in_gm / len(root_notes)
        if n_kg <= 24 and gm_fraction >= 0.5:
            drum_score += 50
            indicators.append(f"GM drum range ({notes_in_gm}/{len(root_notes)} notes)")

    # Keyword hits
    drum_hits = _keyword_hits(sample_names, DRUM_KEYWORDS)
    if drum_hits >= 3:
        drum_score += min(50, drum_hits * 12)
        indicators.append(f"{drum_hits} drum keywords")
    elif drum_hits >= 1:
        drum_score += drum_hits * 8

    if drum_score >= 50:
        confidence = min(100, drum_score)
        return {"type": "DRUM_KIT", "confidence": confidence, "indicators": indicators}

    # -----------------------------------------------------------------------
    # 2. Melodic / tonal classification
    # -----------------------------------------------------------------------

    # Octave span of root notes
    octave_span = 0
    if len(root_notes) >= 2:
        octave_span = (max(root_notes) - min(root_notes)) / 12.0

    # Keyword scoring
    bass_hits    = _keyword_hits(sample_names, BASS_KEYWORDS)
    lead_hits    = _keyword_hits(sample_names, LEAD_KEYWORDS)
    pad_hits     = _keyword_hits(sample_names, PAD_KEYWORDS)
    texture_hits = _keyword_hits(sample_names, TEXTURE_KEYWORDS)

    scores = {
        "BASS":    bass_hits * 25,
        "LEAD":    lead_hits * 25,
        "PAD":     pad_hits  * 22,
        "TEXTURE": texture_hits * 20,
    }

    # Boost based on octave span and keygroup count
    if octave_span >= 2:
        scores["MELODIC"] = 40
        indicators.append(f"{octave_span:.1f} octave span")
    else:
        scores["MELODIC"] = 20

    # Keygroup density hints
    if n_kg > 16:
        scores["MELODIC"] = scores.get("MELODIC", 0) + 15
        indicators.append(f"{n_kg} keygroups (dense)")
    if n_kg <= 4 and octave_span < 1:
        scores["TEXTURE"] = scores.get("TEXTURE", 0) + 15

    # Velocity layers: multiple layers at same root note suggests PAD / layered instrument
    vel_layer_roots: dict[int, int] = {}
    for kg in kgs:
        rn = kg["root_note"]
        if rn >= 0:
            vel_layer_roots[rn] = vel_layer_roots.get(rn, 0) + 1
    max_layers = max(vel_layer_roots.values(), default=1)
    if max_layers >= 3:
        scores["PAD"] = scores.get("PAD", 0) + 15
        indicators.append(f"{max_layers} velocity layers")

    # Find winner
    best_type = max(scores, key=lambda k: scores[k])
    best_score = scores[best_type]

    if best_score == 0:
        return {"type": "UNKNOWN", "confidence": 20, "indicators": ["no indicators matched"]}

    # Build confidence: base from score, capped at 95 unless very strong keyword match
    confidence = min(95, best_score + (20 if octave_span >= 2 else 5))

    # Track keyword indicators
    if bass_hits:    indicators.append(f"{bass_hits} BASS keyword(s)")
    if lead_hits:    indicators.append(f"{lead_hits} LEAD keyword(s)")
    if pad_hits:     indicators.append(f"{pad_hits} PAD keyword(s)")
    if texture_hits: indicators.append(f"{texture_hits} TEXTURE keyword(s)")

    return {"type": best_type, "confidence": confidence, "indicators": indicators}


# ---------------------------------------------------------------------------
# Pack classification
# ---------------------------------------------------------------------------

def classify_pack(pack_path: Path) -> dict[str, Any]:
    """
    Classify all programs in a .xpn pack.
    Returns dict: { pack_name, programs: [...], summary: {...} }
    """
    pack_name = pack_path.stem

    # Read pack name from bundle_manifest if present
    try:
        with zipfile.ZipFile(pack_path, "r") as zf:
            manifests = [n for n in zf.namelist() if n.endswith("bundle_manifest.json")]
            if manifests:
                data = json.loads(zf.read(manifests[0]).decode("utf-8", errors="replace"))
                pack_name = data.get("name", pack_name)
    except Exception:
        pass

    xpms = _xpms_from_zip(pack_path)
    programs = []

    for idx, (filename, xml_text) in enumerate(sorted(xpms), start=1):
        prog = _parse_xpm(xml_text)
        prog_name = prog["name"] or Path(filename).stem
        classification = classify_program(prog)
        programs.append({
            "index":      idx,
            "filename":   filename,
            "name":       prog_name,
            "type":       classification["type"],
            "confidence": classification["confidence"],
            "indicators": classification["indicators"],
            "keygroup_count": len(prog["keygroups"]),
        })

    # Build summary counts
    summary: dict[str, int] = {t: 0 for t in CATEGORY_ORDER}
    for p in programs:
        summary[p["type"]] = summary.get(p["type"], 0) + 1

    return {"pack_name": pack_name, "programs": programs, "summary": summary}


# ---------------------------------------------------------------------------
# Formatters
# ---------------------------------------------------------------------------

def format_text(result: dict[str, Any], verbose: bool = False) -> str:
    lines = [f"PACK: {result['pack_name']}"]

    for p in result["programs"]:
        name_col  = p["name"][:35].ljust(35)
        type_col  = p["type"].ljust(10)
        conf_col  = f"({p['confidence']}%)"
        line = f"  Program {p['index']:>2}: {name_col} → {type_col} {conf_col}"
        lines.append(line)
        if verbose and p["indicators"]:
            lines.append(f"              [{', '.join(p['indicators'])}]")

    summary = result["summary"]
    parts = [f"{v} {k}" for k, v in summary.items() if v > 0]
    lines.append("")
    lines.append("Summary: " + " | ".join(parts))
    return "\n".join(lines)


def format_json(result: dict[str, Any]) -> str:
    return json.dumps(result, indent=2)


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def main() -> None:
    parser = argparse.ArgumentParser(
        description="Classify XPM programs in a .xpn pack by instrument type."
    )
    parser.add_argument("pack", type=Path, help="Path to .xpn pack file")
    parser.add_argument(
        "--format", choices=["text", "json"], default="text",
        help="Output format (default: text)"
    )
    parser.add_argument(
        "--verbose", action="store_true",
        help="Show matched indicators for each program"
    )
    args = parser.parse_args()

    if not args.pack.exists():
        print(f"ERROR: File not found: {args.pack}", file=sys.stderr)
        sys.exit(1)
    if not zipfile.is_zipfile(args.pack):
        print(f"ERROR: Not a valid ZIP/XPN file: {args.pack}", file=sys.stderr)
        sys.exit(1)

    result = classify_pack(args.pack)

    if args.format == "json":
        print(format_json(result))
    else:
        print(format_text(result, verbose=args.verbose))


if __name__ == "__main__":
    main()
