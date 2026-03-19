#!/usr/bin/env python3
"""
XPN Quick Preview Card Generator — XO_OX Designs

Generates a human-readable plain-text "preview card" for a .xpn pack.
Output is a Markdown summary suitable for Discord, Patreon posts, or README files.

The tool reads expansion.json + bundle_manifest.json from inside the .xpn ZIP,
then walks the Programs/ directory for .xpm files to build the programs table.

Usage:
    python xpn_quick_preview_generator.py <pack.xpn>
    python xpn_quick_preview_generator.py <pack.xpn> --output preview.md
    python xpn_quick_preview_generator.py <pack.xpn> --no-programs

Requirements:
    Python 3.10+ (standard library only — zipfile + xml.etree.ElementTree + json)
"""

import argparse
import json
import sys
import zipfile
import xml.etree.ElementTree as ET
from pathlib import Path
from typing import Optional


# =============================================================================
# CONSTANTS
# =============================================================================

DNA_DIMENSIONS = ["brightness", "warmth", "movement", "density", "space", "aggression"]
DNA_BAR_WIDTH = 10  # number of chars in the block bar

VALID_TIERS = {"FORM", "TEXTURE", "MOTION", "COLOR", "SPACE", "TIME"}
VALID_MOODS = {"Foundation", "Atmosphere", "Entangled", "Prism", "Flux", "Aether", "Family"}

# Programs with more than this many keygroups are classified as Melodic
MELODIC_KEYGROUP_THRESHOLD = 20


# =============================================================================
# BAR RENDERING
# =============================================================================

def dna_bar(value: float, width: int = DNA_BAR_WIDTH) -> str:
    """
    Render a value (0.0–1.0) as a filled block bar.

    Example: dna_bar(0.82, 10) → '████████░░'
    """
    if value is None:
        return "░" * width
    value = max(0.0, min(1.0, float(value)))
    filled = round(value * width)
    return "█" * filled + "░" * (width - filled)


# =============================================================================
# XPN READING
# =============================================================================

def read_json_from_zip(zf: zipfile.ZipFile, filename: str) -> Optional[dict]:
    """Read and parse a JSON file from inside a ZIP. Returns None if not found."""
    for name in zf.namelist():
        # Match by basename to handle nested paths
        if Path(name).name == filename:
            try:
                with zf.open(name) as f:
                    return json.load(f)
            except (json.JSONDecodeError, KeyError):
                return None
    return None


def count_keygroups(xpm_text: str) -> int:
    """
    Count Keygroup elements in an XPM XML string.
    Returns 0 if parsing fails.
    """
    try:
        root = ET.fromstring(xpm_text)
        return len(root.findall(".//Keygroup"))
    except ET.ParseError:
        return 0


def count_layers(xpm_text: str) -> int:
    """
    Count the maximum number of SampleData/Layer elements across all keygroups.
    Used as an approximation of voice layers.
    """
    try:
        root = ET.fromstring(xpm_text)
        max_layers = 0
        for kg in root.findall(".//Keygroup"):
            layers = len(kg.findall(".//SampleData"))
            if layers == 0:
                layers = len(kg.findall(".//Layer"))
            max_layers = max(max_layers, layers)
        return max(max_layers, 1)
    except ET.ParseError:
        return 1


def classify_program(n_keygroups: int) -> str:
    """Classify a program as Melodic or Drum based on keygroup count."""
    return "Melodic" if n_keygroups > MELODIC_KEYGROUP_THRESHOLD else "Drum"


def read_programs(zf: zipfile.ZipFile) -> list[dict]:
    """
    Walk the ZIP for .xpm files and return a list of program dicts:
      {name, type, pads, layers}
    Sorted by name.
    """
    programs = []
    for entry in sorted(zf.namelist()):
        if not entry.lower().endswith(".xpm"):
            continue
        try:
            with zf.open(entry) as f:
                xpm_text = f.read().decode("utf-8", errors="replace")
        except KeyError:
            continue

        name = Path(entry).stem
        n_kg = count_keygroups(xpm_text)
        prog_type = classify_program(n_kg)
        n_layers = count_layers(xpm_text)

        # Pad count: for Melodic use keygroup count (≈ zones), for Drum cap at 16
        pads = n_kg if prog_type == "Melodic" else min(n_kg, 64)

        programs.append({
            "name": name,
            "type": prog_type,
            "pads": pads,
            "layers": n_layers,
        })

    return programs


def compute_pack_size_mb(zf: zipfile.ZipFile) -> float:
    """Sum uncompressed sizes of all members and return total in MB."""
    total_bytes = sum(info.file_size for info in zf.infolist())
    return total_bytes / (1024 * 1024)


def count_samples(zf: zipfile.ZipFile) -> int:
    """Count WAV/AIFF files inside the ZIP."""
    count = 0
    for name in zf.namelist():
        lower = name.lower()
        if lower.endswith(".wav") or lower.endswith(".aif") or lower.endswith(".aiff"):
            count += 1
    return count


# =============================================================================
# METADATA EXTRACTION
# =============================================================================

def extract_metadata(zf: zipfile.ZipFile) -> dict:
    """
    Extract pack metadata from expansion.json and bundle_manifest.json.
    Returns a dict with canonical keys used by the card renderer.
    """
    meta = {
        "pack_name": "Unknown Pack",
        "engine": "Unknown",
        "tier": "FORM",
        "mood": "Foundation",
        "version": "1.0.0",
        "description": None,
        "tags": [],
        "dna": {d: None for d in DNA_DIMENSIONS},
    }

    # --- expansion.json (primary) ---
    expansion = read_json_from_zip(zf, "expansion.json")
    if expansion:
        if "name" in expansion:
            meta["pack_name"] = expansion["name"]
        if "engine" in expansion:
            meta["engine"] = expansion["engine"]
        if "tier" in expansion:
            raw_tier = str(expansion["tier"]).upper()
            if raw_tier in VALID_TIERS:
                meta["tier"] = raw_tier
        if "mood" in expansion:
            raw_mood = expansion["mood"]
            if raw_mood in VALID_MOODS:
                meta["mood"] = raw_mood
        if "version" in expansion:
            meta["version"] = str(expansion["version"])
        if "description" in expansion:
            meta["description"] = expansion["description"]
        if "tags" in expansion and isinstance(expansion["tags"], list):
            meta["tags"] = [str(t) for t in expansion["tags"]]

        # Sonic DNA may live directly inside expansion.json
        dna_src = expansion.get("sonic_dna") or expansion.get("sonicDna") or expansion
        for dim in DNA_DIMENSIONS:
            if dim in dna_src:
                try:
                    meta["dna"][dim] = float(dna_src[dim])
                except (ValueError, TypeError):
                    pass

    # --- bundle_manifest.json (supplements/overrides) ---
    manifest = read_json_from_zip(zf, "bundle_manifest.json")
    if manifest:
        # Override pack name and engine if cleaner in manifest
        if "pack_name" in manifest and meta["pack_name"] == "Unknown Pack":
            meta["pack_name"] = manifest["pack_name"]
        if "engine" in manifest and meta["engine"] == "Unknown":
            meta["engine"] = manifest["engine"]
        if "version" in manifest and meta["version"] == "1.0.0":
            meta["version"] = str(manifest["version"])
        if "tier" in manifest:
            raw_tier = str(manifest["tier"]).upper()
            if raw_tier in VALID_TIERS:
                meta["tier"] = raw_tier
        if "mood" in manifest:
            raw_mood = manifest["mood"]
            if raw_mood in VALID_MOODS:
                meta["mood"] = raw_mood

        # Sonic DNA in manifest
        dna_src = manifest.get("sonic_dna") or manifest.get("sonicDna") or {}
        for dim in DNA_DIMENSIONS:
            if dim in dna_src and meta["dna"][dim] is None:
                try:
                    meta["dna"][dim] = float(dna_src[dim])
                except (ValueError, TypeError):
                    pass

        # Tags
        if "tags" in manifest and isinstance(manifest["tags"], list) and not meta["tags"]:
            meta["tags"] = [str(t) for t in manifest["tags"]]

    return meta


# =============================================================================
# CARD RENDERING
# =============================================================================

def render_preview_card(
    meta: dict,
    programs: list[dict],
    n_samples: int,
    size_mb: float,
    include_programs: bool = True,
) -> str:
    """
    Render the full markdown preview card string.
    """
    lines = []

    # --- Header ---
    lines.append(f"# {meta['pack_name']} — {meta['engine']} Pack")
    lines.append("")

    # --- Stats line ---
    lines.append(
        f"**Tier**: {meta['tier']}  |  "
        f"**Mood**: {meta['mood']}  |  "
        f"**Version**: {meta['version']}"
    )
    lines.append(
        f"**Programs**: {len(programs)}  |  "
        f"**Samples**: {n_samples}  |  "
        f"**Size**: {size_mb:.1f} MB"
    )
    lines.append("")

    # --- Sonic DNA ---
    lines.append("## Sonic DNA")
    for dim in DNA_DIMENSIONS:
        val = meta["dna"][dim]
        bar = dna_bar(val)
        label = dim.capitalize().ljust(11)
        val_str = f"{val:.2f}" if val is not None else " n/a"
        lines.append(f"{label} {bar}  {val_str}")
    lines.append("")

    # --- Programs table ---
    if include_programs and programs:
        lines.append("## Programs")
        lines.append("| Name             | Type    | Pads | Layers |")
        lines.append("|-----------------|---------|------|--------|")
        for p in programs:
            name_col = p["name"][:16].ljust(16)
            type_col = p["type"].ljust(7)
            lines.append(f"| {name_col} | {type_col} | {str(p['pads']).ljust(4)} | {p['layers']}      |")
        lines.append("")

    # --- Tags ---
    if meta["tags"]:
        tag_str = "  ".join(f"`{t}`" for t in meta["tags"])
        lines.append("## Tags")
        lines.append(tag_str)
        lines.append("")

    # --- Pack Story ---
    lines.append("## Pack Story")
    lines.append(meta["description"] if meta["description"] else "No description.")
    lines.append("")

    return "\n".join(lines)


# =============================================================================
# MAIN
# =============================================================================

def generate_preview_card(
    pack_path: Path,
    include_programs: bool = True,
) -> str:
    """
    Open a .xpn ZIP, extract all metadata and program info, and return
    the rendered markdown preview card string.
    """
    if not pack_path.exists():
        raise FileNotFoundError(f"Pack not found: {pack_path}")
    if not zipfile.is_zipfile(pack_path):
        raise ValueError(f"Not a valid ZIP/XPN file: {pack_path}")

    with zipfile.ZipFile(pack_path, "r") as zf:
        meta = extract_metadata(zf)
        programs = read_programs(zf)
        n_samples = count_samples(zf)
        size_mb = compute_pack_size_mb(zf)

    return render_preview_card(
        meta=meta,
        programs=programs,
        n_samples=n_samples,
        size_mb=size_mb,
        include_programs=include_programs,
    )


def main() -> int:
    parser = argparse.ArgumentParser(
        description="XPN Quick Preview Card Generator — generate a markdown "
                    "summary card for a .xpn pack (Discord/Patreon/README).",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    parser.add_argument(
        "pack",
        metavar="PACK.xpn",
        help="Path to the .xpn expansion file",
    )
    parser.add_argument(
        "--output", "-o",
        metavar="FILE",
        default=None,
        help="Write output to FILE instead of stdout",
    )
    parser.add_argument(
        "--no-programs",
        action="store_true",
        help="Omit the programs table (useful for very large packs)",
    )

    args = parser.parse_args()

    pack_path = Path(args.pack)

    try:
        card = generate_preview_card(
            pack_path=pack_path,
            include_programs=not args.no_programs,
        )
    except FileNotFoundError as e:
        print(f"[ERROR] {e}", file=sys.stderr)
        return 1
    except ValueError as e:
        print(f"[ERROR] {e}", file=sys.stderr)
        return 1

    if args.output:
        out_path = Path(args.output)
        out_path.parent.mkdir(parents=True, exist_ok=True)
        out_path.write_text(card, encoding="utf-8")
        print(f"Preview card written to: {out_path}", file=sys.stderr)
    else:
        print(card)

    return 0


if __name__ == "__main__":
    sys.exit(main())
