#!/usr/bin/env python3
"""
migrate_opensky_presets.py — Migrate stale OpenSky preset parameter schema
to the current engine vocabulary.

Approximately one-third of OpenSky presets use a legacy parameter vocabulary
that no longer matches the current engine's declared parameters.  This script
applies the rename table, updates macroLabels and macros keys, and removes
deprecated parameters.

Detection:  presence of ``sky_filterFc`` or ``sky_macroCharacter`` in the
            OpenSky parameter block.

Usage:
    python3 Tools/migrate_opensky_presets.py                # dry-run (default)
    python3 Tools/migrate_opensky_presets.py --apply        # apply changes in-place
"""

import argparse
import json
import sys
from pathlib import Path

# ---------------------------------------------------------------------------
# Configuration
# ---------------------------------------------------------------------------

PRESETS_ROOT = Path(__file__).resolve().parent.parent / "Presets" / "XOceanus"

# Parameter rename table  (old → new)
PARAM_RENAME = {
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
    "sky_macroCharacter": "sky_macroRise",
    "sky_macroMovement":  "sky_macroWidth",
    "sky_macroCoupling":  "sky_macroGlow",
    "sky_macroSpace":     "sky_macroAir",
    "sky_sawDetune":      "sky_sawSpread",
    "sky_shimmerBright":  "sky_shimmerDamping",
}

# Parameters removed outright (no replacement)
PARAMS_REMOVED = {
    "sky_reverbMix",
    "sky_couplingAmt",
}

# Macro label migration
OLD_MACRO_LABELS = ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"]
NEW_MACRO_LABELS = ["RISE", "WIDTH", "GLOW", "AIR"]

MACRO_LABEL_RENAME = dict(zip(OLD_MACRO_LABELS, NEW_MACRO_LABELS))

# Current engine parameter IDs (from OpenSkyEngine.h addParametersImpl)
VALID_ENGINE_PARAMS = {
    "sky_sawSpread", "sky_sawMix", "sky_subLevel", "sky_subWave",
    "sky_coarseTune", "sky_fineTune",
    "sky_pitchEnvAmount", "sky_pitchEnvDecay",
    "sky_filterCutoff", "sky_filterReso", "sky_filterHP",
    "sky_filterEnvAmount", "sky_filterType",
    "sky_shimmerMix", "sky_shimmerSize", "sky_shimmerDamping",
    "sky_shimmerFeedback", "sky_shimmerOctave",
    "sky_chorusRate", "sky_chorusDepth", "sky_chorusMix",
    "sky_unisonCount", "sky_unisonDetune", "sky_unisonSpread",
    "sky_attack", "sky_decay", "sky_sustain", "sky_release",
    "sky_level", "sky_pan",
    "sky_lfo1Rate", "sky_lfo1Depth", "sky_lfo1Shape",
    "sky_lfo2Rate", "sky_lfo2Depth", "sky_lfo2Shape",
    "sky_stereoWidth",
    "sky_macroRise", "sky_macroWidth", "sky_macroGlow", "sky_macroAir",
    "sky_modSlot1Src", "sky_modSlot1Dst", "sky_modSlot1Amt",
    "sky_modSlot2Src", "sky_modSlot2Dst", "sky_modSlot2Amt",
    "sky_velSensitivity",
}


# ---------------------------------------------------------------------------
# Detection
# ---------------------------------------------------------------------------

def is_old_schema(params: dict) -> bool:
    """Return True if the OpenSky parameter block uses the legacy vocabulary."""
    return "sky_filterFc" in params or "sky_macroCharacter" in params


# ---------------------------------------------------------------------------
# Migration
# ---------------------------------------------------------------------------

def migrate_preset(data: dict) -> tuple[bool, list[str]]:
    """Migrate a single preset dict in-place.

    Returns (changed, warnings) where *changed* is True when the preset was
    modified and *warnings* is a list of human-readable warning strings.
    """
    warnings: list[str] = []
    changed = False

    # Only process presets that include OpenSky
    if "OpenSky" not in data.get("engines", []):
        return changed, warnings

    params = data.get("parameters", {}).get("OpenSky")
    if params is None:
        return changed, warnings

    if not is_old_schema(params):
        return changed, warnings

    # --- 1. Rename parameters ---
    new_params: dict = {}
    for key, value in params.items():
        if key in PARAMS_REMOVED:
            changed = True
            continue  # drop removed params
        if key in PARAM_RENAME:
            new_key = PARAM_RENAME[key]
            if new_key in new_params:
                warnings.append(f"  CONFLICT: both '{key}' and '{new_key}' present; keeping '{new_key}'")
            else:
                new_params[new_key] = value
            changed = True
        else:
            new_params[key] = value

    data["parameters"]["OpenSky"] = new_params

    # --- 2. Update macroLabels ---
    labels = data.get("macroLabels")
    if labels == OLD_MACRO_LABELS:
        data["macroLabels"] = list(NEW_MACRO_LABELS)
        changed = True

    # --- 3. Update macros keys ---
    macros = data.get("macros")
    if macros and any(k in macros for k in OLD_MACRO_LABELS):
        new_macros: dict = {}
        for k, v in macros.items():
            new_macros[MACRO_LABEL_RENAME.get(k, k)] = v
        data["macros"] = new_macros
        changed = True

    # --- 4. Validate against current engine parameter list ---
    final_params = data["parameters"]["OpenSky"]
    for key in sorted(final_params):
        if key not in VALID_ENGINE_PARAMS:
            warnings.append(f"  UNKNOWN PARAM after migration: {key}")

    return changed, warnings


# ---------------------------------------------------------------------------
# I/O
# ---------------------------------------------------------------------------

def collect_xometa_files(root: Path) -> list[Path]:
    """Recursively find all .xometa files under *root*."""
    return sorted(root.rglob("*.xometa"))


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Migrate legacy OpenSky preset parameters to current schema.")
    parser.add_argument("--apply", action="store_true",
                        help="Write changes in-place (default is dry-run).")
    parser.add_argument("--preset-dir", type=Path, default=PRESETS_ROOT,
                        help="Root preset directory to scan.")
    args = parser.parse_args()

    files = collect_xometa_files(args.preset_dir)
    if not files:
        print(f"No .xometa files found under {args.preset_dir}")
        return 1

    migrated = 0
    skipped = 0
    errors = 0
    all_warnings: list[str] = []

    for path in files:
        try:
            text = path.read_text(encoding="utf-8")
            data = json.loads(text)
        except (json.JSONDecodeError, OSError) as exc:
            print(f"  ERROR reading {path}: {exc}")
            errors += 1
            continue

        changed, warnings = migrate_preset(data)

        if warnings:
            all_warnings.append(f"{path.relative_to(args.preset_dir)}:")
            all_warnings.extend(warnings)

        if not changed:
            skipped += 1
            continue

        migrated += 1
        rel = path.relative_to(args.preset_dir)
        print(f"  {'MIGRATE' if args.apply else 'WOULD MIGRATE'}: {rel}")

        if args.apply:
            out = json.dumps(data, indent=2, ensure_ascii=False) + "\n"
            path.write_text(out, encoding="utf-8")

    # --- Summary ---
    print()
    print(f"{'Applied' if args.apply else 'Dry-run'} summary:")
    print(f"  Scanned:  {len(files)}")
    print(f"  Migrated: {migrated}")
    print(f"  Skipped:  {skipped}")
    print(f"  Errors:   {errors}")

    if all_warnings:
        print()
        print("Warnings:")
        for w in all_warnings:
            print(f"  {w}")

    return 0


if __name__ == "__main__":
    sys.exit(main())
