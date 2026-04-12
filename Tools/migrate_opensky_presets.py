#!/usr/bin/env python3
"""
OpenSky preset parameter migration: legacy vocab → current engine vocab.
Detected by presence of `sky_filterFc` in parameters["OpenSky"].

Parameters are stored nested under an engine-name key:
  preset["parameters"]["OpenSky"]["sky_filterFc"] = ...

Top-level macros and macroLabels also need updating for old-schema presets.

Run from repo root:
    python3 Tools/migrate_opensky_presets.py [--dry-run]
"""

import json
import os
import sys
import argparse
from pathlib import Path

PARAM_RENAMES = {
    "sky_filterFc":       "sky_filterCutoff",
    "sky_filterRes":      "sky_filterReso",
    "sky_ampAttack":      "sky_attack",
    "sky_ampDecay":       "sky_decay",
    "sky_ampSustain":     "sky_sustain",
    "sky_ampRelease":     "sky_release",
    "sky_lfoRate":        "sky_lfo1Rate",
    "sky_lfoDepth":       "sky_lfo1Depth",
    "sky_lfoShape":       "sky_lfo1Shape",
    "sky_shimmerOct":     "sky_shimmerOctave",
    "sky_outputGain":     "sky_level",
    "sky_sawDetune":      "sky_sawSpread",
    "sky_shimmerBright":  "sky_shimmerDamping",
    "sky_macroCharacter": "sky_macroRise",
    "sky_macroMovement":  "sky_macroWidth",
    "sky_macroCoupling":  "sky_macroGlow",
    "sky_macroSpace":     "sky_macroAir",
}

# Parameters removed from the engine — drop silently
PARAM_REMOVED = {"sky_reverbMix", "sky_couplingAmt"}

OLD_MACRO_LABELS = ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"]
NEW_MACRO_LABELS = ["RISE",      "WIDTH",    "GLOW",     "AIR"  ]
OLD_TO_NEW_LABEL = dict(zip(OLD_MACRO_LABELS, NEW_MACRO_LABELS))

# The engine key may appear under different capitalizations in coupling presets
ENGINE_KEYS = {"OpenSky", "opensky", "OPENSKY"}


def find_opensky_key(params: dict):
    for k in params:
        if k in ENGINE_KEYS or k.lower() == "opensky":
            return k
    return None


def is_stale(preset: dict) -> bool:
    params = preset.get("parameters", {})
    engine_key = find_opensky_key(params)
    if not engine_key:
        return False
    engine_params = params[engine_key]
    if not isinstance(engine_params, dict):
        return False
    return "sky_filterFc" in engine_params or "sky_macroCharacter" in engine_params


def migrate_preset(preset: dict) -> dict:
    params = preset.get("parameters", {})
    engine_key = find_opensky_key(params)

    if engine_key:
        old_engine_params = params[engine_key]
        new_engine_params = {}
        for k, v in old_engine_params.items():
            if k in PARAM_REMOVED:
                continue
            new_key = PARAM_RENAMES.get(k, k)
            new_engine_params[new_key] = v
        params[engine_key] = new_engine_params
        preset["parameters"] = params

    # Top-level macroLabels
    labels = preset.get("macroLabels", [])
    if labels == OLD_MACRO_LABELS:
        preset["macroLabels"] = NEW_MACRO_LABELS

    # Top-level macros dict (keyed by label string)
    macros = preset.get("macros", {})
    if any(k in macros for k in OLD_MACRO_LABELS):
        new_macros = {}
        for k, v in macros.items():
            new_macros[OLD_TO_NEW_LABEL.get(k, k)] = v
        preset["macros"] = new_macros

    return preset


def process_file(path: Path, dry_run: bool) -> bool:
    try:
        text = path.read_text(encoding="utf-8")
        data = json.loads(text)
    except Exception as e:
        print(f"  SKIP (parse error): {path} — {e}")
        return False

    if not is_stale(data):
        return False

    migrated = migrate_preset(data)
    if not dry_run:
        path.write_text(json.dumps(migrated, indent=2, ensure_ascii=False) + "\n",
                        encoding="utf-8")
    return True


def main():
    parser = argparse.ArgumentParser(description="Migrate stale OpenSky presets")
    parser.add_argument("--dry-run", action="store_true",
                        help="Report what would change without writing files")
    args = parser.parse_args()

    root = Path(__file__).parent.parent / "Presets" / "XOceanus"
    if not root.exists():
        print(f"ERROR: preset root not found: {root}")
        sys.exit(1)

    files = list(root.rglob("*.xometa"))
    migrated_count = 0
    skipped = 0

    for f in sorted(files):
        changed = process_file(f, args.dry_run)
        if changed:
            migrated_count += 1
            tag = "[DRY RUN] " if args.dry_run else ""
            print(f"  {tag}migrated: {f.relative_to(root.parent.parent)}")
        else:
            skipped += 1

    print(f"\n{'DRY RUN — ' if args.dry_run else ''}Done: {migrated_count} migrated, {skipped} already current")


if __name__ == "__main__":
    main()
