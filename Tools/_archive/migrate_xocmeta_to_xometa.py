#!/usr/bin/env python3
"""
OddfeliX .xocmeta → XOmnibus .xometa Migration Script

Converts all 114 OddfeliX factory presets from .xocmeta format
to the unified .xometa format for XOmnibus.

Usage:
    python3 migrate_xocmeta_to_xometa.py [--dry-run]
"""

import json
import os
import sys
from pathlib import Path

# ---------------------------------------------------------------------------
# Config
# ---------------------------------------------------------------------------

FACTORY_DIR = Path(__file__).parent.parent / "Presets" / "Factory"
OUTPUT_DIR = Path(__file__).parent.parent / "Presets" / "XOmnibus"

# OddfeliX category → XOmnibus mood
MOOD_MAP = {
    "Grounded": "Foundation",
    "Floating": "Atmosphere",
    "Entangled": "Entangled",
    "Deep Space": "Aether",
}

# OddfeliX macro labels (hardcoded in the engine)
MACRO_LABELS = ["Snap+Morph", "Bloom", "Coupling", "Space"]

# ---------------------------------------------------------------------------
# Migration
# ---------------------------------------------------------------------------

def migrate_preset(xocmeta: dict) -> dict:
    """Convert a single .xocmeta preset to .xometa format."""

    category = xocmeta.get("category", "Entangled")
    mood = MOOD_MAP.get(category, "Entangled")

    # Remap sequencer data if present
    sequencer = None
    if xocmeta.get("sequencer") and xocmeta.get("sequencerPattern"):
        seq_src = xocmeta["sequencer"]
        sequencer = {
            "bpm": xocmeta["parameters"].get("seqTempo", 120),
            "swing": xocmeta["parameters"].get("seqSwing", 0),
            "tracks": [],
        }
        for track in seq_src.get("tracks", []):
            new_track = {
                "engine": "OddfeliX",
                "length": track.get("length", 16),
                "muted": track.get("muted", False),
                "steps": [],
            }
            for step in track.get("steps", []):
                new_step = {
                    "index": step.get("index", 0),
                    "active": step.get("active", False),
                    "note": step.get("note", 60),
                    "velocity": step.get("velocity", 1.0),
                    "probability": step.get("probability", 1.0),
                }
                new_track["steps"].append(new_step)
            sequencer["tracks"].append(new_track)

    # Build parameters block — nest under engine name
    params = dict(xocmeta.get("parameters", {}))
    # Remove sequencer params from the DSP params (they go in sequencer block)
    for key in ["seqPlaying", "seqTempo", "seqSwing"]:
        params.pop(key, None)

    xometa = {
        "schema_version": 1,
        "name": xocmeta["name"],
        "mood": mood,
        "engines": ["OddfeliX"],
        "author": xocmeta.get("author", "XO_OX Designs"),
        "version": xocmeta.get("version", "1.0.0"),
        "description": xocmeta.get("description", ""),
        "tags": xocmeta.get("tags", []),
        "macroLabels": MACRO_LABELS,
        "couplingIntensity": xocmeta.get("couplingIntensity", "None"),
        "tempo": xocmeta.get("tempo"),
        "created": xocmeta.get("created", "2026-03-08"),
        "legacy": {
            "sourceInstrument": "OddfeliX",
            "sourceCategory": category,
            "sourcePresetName": None,
        },
        "parameters": {
            "OddfeliX": params,
        },
        "coupling": None,
        "sequencer": sequencer,
    }

    return xometa


def validate_xometa(xometa: dict) -> list:
    """Basic validation — returns list of issues."""
    issues = []

    if not xometa.get("name"):
        issues.append("Missing name")
    if len(xometa.get("name", "")) > 30:
        issues.append(f"Name too long: {len(xometa['name'])} chars (max 30)")
    if xometa.get("mood") not in ["Foundation", "Atmosphere", "Entangled", "Prism", "Flux", "Aether", "User"]:
        issues.append(f"Invalid mood: {xometa.get('mood')}")
    if not xometa.get("engines"):
        issues.append("Missing engines")
    if len(xometa.get("tags", [])) < 3:
        issues.append(f"Too few tags: {len(xometa.get('tags', []))} (min 3)")
    if not xometa.get("description"):
        issues.append("Missing description")

    return issues


def main():
    dry_run = "--dry-run" in sys.argv

    if not FACTORY_DIR.is_dir():
        print(f"Error: Factory dir not found: {FACTORY_DIR}")
        sys.exit(1)

    # Collect all .xocmeta files
    xocmeta_files = sorted(FACTORY_DIR.rglob("*.xocmeta"))
    print(f"Found {len(xocmeta_files)} .xocmeta files\n")

    if not dry_run:
        OUTPUT_DIR.mkdir(parents=True, exist_ok=True)
        # Create mood subdirectories
        for mood in MOOD_MAP.values():
            (OUTPUT_DIR / mood).mkdir(exist_ok=True)

    stats = {
        "total": 0,
        "migrated": 0,
        "warnings": 0,
        "errors": 0,
        "by_mood": {},
    }
    all_names = set()
    all_issues = []

    for xocmeta_path in xocmeta_files:
        stats["total"] += 1

        try:
            with open(xocmeta_path) as f:
                xocmeta = json.load(f)
        except json.JSONDecodeError as e:
            print(f"  ERROR: {xocmeta_path.name}: {e}")
            stats["errors"] += 1
            continue

        # Migrate
        xometa = migrate_preset(xocmeta)

        # Validate
        issues = validate_xometa(xometa)
        if issues:
            for issue in issues:
                all_issues.append((xometa["name"], issue))
            stats["warnings"] += len(issues)

        # Check for name collision
        name_key = xometa["name"].lower()
        if name_key in all_names:
            all_issues.append((xometa["name"], "DUPLICATE NAME"))
            stats["errors"] += 1
        all_names.add(name_key)

        # Track mood distribution
        mood = xometa["mood"]
        stats["by_mood"][mood] = stats["by_mood"].get(mood, 0) + 1

        # Write
        if not dry_run:
            out_path = OUTPUT_DIR / mood / f"{xometa['name']}.xometa"
            with open(out_path, "w") as f:
                json.dump(xometa, f, indent=2)
            stats["migrated"] += 1
        else:
            print(f"  [DRY] {xocmeta_path.parent.name}/{xocmeta_path.stem} → {mood}/{xometa['name']}.xometa")
            stats["migrated"] += 1

    # Report
    print(f"\n{'=' * 60}")
    print("MIGRATION REPORT")
    print(f"{'=' * 60}")
    print(f"Total .xocmeta files:  {stats['total']}")
    print(f"Successfully migrated: {stats['migrated']}")
    print(f"Warnings:              {stats['warnings']}")
    print(f"Errors:                {stats['errors']}")
    print(f"\nMood distribution:")
    for mood, count in sorted(stats["by_mood"].items()):
        print(f"  {mood}: {count}")

    if all_issues:
        print(f"\nIssues:")
        for name, issue in all_issues:
            print(f"  {name}: {issue}")

    if not dry_run:
        print(f"\nOutput: {OUTPUT_DIR}")

    return 0 if stats["errors"] == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
