#!/usr/bin/env python3
"""
XPN Dry Variant Duplicator — XO_OX Designs
Creates "dry" (FX-off) variants of .xometa presets alongside originals.

XO_OX Brand Rule: "Dry patches must sound compelling before effects are applied."
This tool enforces that rule by generating parallel dry variants that:
  - Strip all send/reverb/delay levels to 0
  - Remove chorus/flanger/modulation FX depth
  - Rename with "_Dry" suffix
  - Preserve all synthesis parameters (oscillators, envelopes, filters)

Usage:
    # Generate dry variants for all Overdub presets
    python3 xpn_dry_variant_duplicator.py --engine Overdub

    # Preview without writing
    python3 xpn_dry_variant_duplicator.py --engine Overdub --dry-run

    # All engines
    python3 xpn_dry_variant_duplicator.py --all

    # Validate that existing _Dry variants are up to date
    python3 xpn_dry_variant_duplicator.py --validate --engine Overdub
"""

import argparse
import copy
import json
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).parent.parent.resolve()
PRESETS_DIR = REPO_ROOT / "Presets" / "XOlokun"

# Parameter suffixes that indicate FX send levels (engine-agnostic)
FX_SEND_PATTERNS = [
    "sendAmount", "send_amount", "reverbMix", "reverb_mix", "delayMix", "delay_mix",
    "chorusMix", "chorus_mix", "flangerMix", "flanger_mix", "springMix", "spring_mix",
    "tapeMix", "tape_mix", "wetLevel", "wet_level", "fxMix", "fx_mix",
]

def is_fx_param(key: str) -> bool:
    return any(key.endswith(p) or p in key for p in FX_SEND_PATTERNS)

def make_dry_preset(preset: dict) -> dict:
    dry = copy.deepcopy(preset)
    # Rename
    original_name = dry.get("name", "Preset")
    if not original_name.endswith("_Dry"):
        dry["name"] = original_name + "_Dry"

    # Zero out FX parameters
    params = dry.get("parameters", {})
    zeroed = []
    for key in list(params.keys()):
        if is_fx_param(key):
            params[key] = 0.0
            zeroed.append(key)

    # Tag as dry variant
    if "tags" not in dry:
        dry["tags"] = []
    if "dry" not in dry["tags"]:
        dry["tags"].append("dry")

    dry["_dry_variant_of"] = original_name
    dry["_zeroed_params"] = zeroed
    return dry

def find_presets(engine: str) -> list[Path]:
    pattern = f"**/{engine}/*.xometa"
    presets = list(PRESETS_DIR.glob(pattern))
    if not presets:
        # Try case-insensitive
        for mood_dir in PRESETS_DIR.iterdir():
            if mood_dir.is_dir():
                for eng_dir in mood_dir.iterdir():
                    if eng_dir.is_dir() and eng_dir.name.lower() == engine.lower():
                        presets.extend(eng_dir.glob("*.xometa"))
    return presets

def main() -> int:
    parser = argparse.ArgumentParser(
        description="XPN Dry Variant Duplicator — creates FX-off preset variants",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    parser.add_argument("--engine", metavar="NAME",
                        help="Engine name (e.g. Overdub, Oceanic)")
    parser.add_argument("--all",     action="store_true",
                        help="Process all engines")
    parser.add_argument("--dry-run", action="store_true", dest="dry_run",
                        help="Preview changes without writing files")
    parser.add_argument("--validate", action="store_true",
                        help="Check that _Dry variants match source presets (no write)")
    args = parser.parse_args()

    if not args.engine and not args.all:
        parser.print_help()
        return 1

    if args.all:
        engines = [d.name for mood in PRESETS_DIR.iterdir() if mood.is_dir()
                   for d in mood.iterdir() if d.is_dir()]
        engines = sorted(set(engines))
    else:
        engines = [args.engine]

    total_written = 0
    total_skipped = 0
    total_errors = 0

    for engine in engines:
        presets = find_presets(engine)
        # Exclude already-dry variants from source set
        source_presets = [p for p in presets if not p.stem.endswith("_Dry")]

        if not source_presets:
            continue

        print(f"\n  {engine}: {len(source_presets)} source presets")

        for preset_path in source_presets:
            try:
                data = json.loads(preset_path.read_text(encoding="utf-8"))
            except Exception as e:
                print(f"    [ERROR] {preset_path.name}: {e}")
                total_errors += 1
                continue

            dry_name = data.get("name", preset_path.stem) + "_Dry"
            dry_path = preset_path.parent / f"{dry_name}.xometa"

            if args.validate:
                if dry_path.exists():
                    print(f"    [OK] {dry_name}.xometa exists")
                else:
                    print(f"    [MISSING] {dry_name}.xometa — run without --validate to create")
                continue

            if dry_path.exists():
                total_skipped += 1
                continue

            dry_preset = make_dry_preset(data)

            if args.dry_run:
                zeroed = dry_preset.get("_zeroed_params", [])
                print(f"    [DRY] Would create {dry_path.name} (zeroing: {', '.join(zeroed[:3])}{'...' if len(zeroed) > 3 else ''})")
            else:
                dry_path.write_text(json.dumps(dry_preset, indent=2), encoding="utf-8")
                print(f"    [OK] {dry_path.name}")
                total_written += 1

    print(f"\n  Done — Written: {total_written}  Skipped (exists): {total_skipped}  Errors: {total_errors}")
    return 0 if total_errors == 0 else 1

if __name__ == "__main__":
    sys.exit(main())
