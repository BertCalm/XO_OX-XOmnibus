#!/usr/bin/env python3
"""
fix_425_447_macro_format.py — Fix deprecated macro format and missing macroLabels.

Closes issues #425 (301 deprecated format) and #447 (296 non-standard macros dict).

Four categories fixed:

1. OCELOT string-M1 (103 presets):
   macros={M1:'PROWL', M2:'FOLIAGE', M3:'ECOSYSTEM', M4:'CANOPY'}
   → macros={PROWL: 0.5, FOLIAGE: 0.5, ECOSYSTEM: 0.5, CANOPY: 0.5}
   (macroLabels already correct, values default to 0.5 since none stored)

2. OFFERING M1 without macroLabels (79 presets):
   macros={M1: 0.6, M2: 0.15, ...} + no macroLabels
   → macros={DIG: 0.6, CITY: 0.15, FLIP: 0.0, DUST: 0.0}
      macroLabels=['DIG','CITY','FLIP','DUST']

3. OFFERING M1 with macroLabels (75 presets):
   macros={M1: 0.6, M2: 0.15, ...} + macroLabels=['DIG','CITY','FLIP','DUST']
   → macros={DIG: 0.6, CITY: 0.15, FLIP: 0.0, DUST: 0.0}

4. Standard macros without macroLabels (217 presets):
   macros={CHARACTER:..., MOVEMENT:..., COUPLING:..., SPACE:...} + no macroLabels
   → add macroLabels=['CHARACTER','MOVEMENT','COUPLING','SPACE']

Usage:
    python3 Tools/fix_425_447_macro_format.py --dry-run
    python3 Tools/fix_425_447_macro_format.py
"""

import json
import sys
from pathlib import Path

PRESETS_ROOT = Path(__file__).parent.parent / "Presets" / "XOlokun"
STANDARD_LABELS = ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"]
OFFERING_LABELS = ["DIG", "CITY", "FLIP", "DUST"]


def fix_preset(data: dict) -> tuple[dict, str]:
    """Return (fixed_data, description) or (data, '') if no fix needed."""
    macros = data.get("macros")
    macrolabels = data.get("macroLabels")

    if not isinstance(macros, dict):
        return data, ""

    keys = list(macros.keys())

    # --- Case 1: Ocelot string-M1 format ---
    # macros = {M1: 'PROWL', M2: 'FOLIAGE', ...} where values are label strings
    if (
        keys == ["M1", "M2", "M3", "M4"]
        and all(isinstance(macros[k], str) for k in keys)
    ):
        label_list = [macros["M1"], macros["M2"], macros["M3"], macros["M4"]]
        fixed = dict(data)
        fixed["macros"] = {label: 0.5 for label in label_list}
        # macroLabels should already be correct; confirm it matches
        if not macrolabels or macrolabels != label_list:
            fixed["macroLabels"] = label_list
        return fixed, "ocelot_str_m1"

    # --- Cases 2 & 3: Offering M1 with numeric float values ---
    if (
        keys == ["M1", "M2", "M3", "M4"]
        and all(isinstance(macros[k], (int, float)) for k in keys)
    ):
        m1_val = float(macros["M1"])
        m2_val = float(macros["M2"])
        m3_val = float(macros["M3"])
        m4_val = float(macros["M4"])

        # Determine correct labels from macroLabels or default to OFFERING_LABELS
        if isinstance(macrolabels, list) and len(macrolabels) == 4:
            label_list = macrolabels
            case = "offering_m1_with_ml"
        else:
            label_list = OFFERING_LABELS
            case = "offering_m1_no_ml"

        fixed = dict(data)
        fixed["macros"] = {
            label_list[0]: m1_val,
            label_list[1]: m2_val,
            label_list[2]: m3_val,
            label_list[3]: m4_val,
        }
        fixed["macroLabels"] = label_list
        return fixed, case

    # --- Case 4: Standard macros without macroLabels ---
    if (
        macrolabels is None
        and set(keys) == set(STANDARD_LABELS)
    ):
        fixed = dict(data)
        # Order macroLabels to match the standard order
        fixed["macroLabels"] = STANDARD_LABELS
        return fixed, "std_macros_no_ml"

    return data, ""


def process_all(dry_run: bool = False) -> dict:
    """Walk all presets and apply fixes. Returns counters dict."""
    counters = {
        "ocelot_str_m1": 0,
        "offering_m1_with_ml": 0,
        "offering_m1_no_ml": 0,
        "std_macros_no_ml": 0,
        "skipped": 0,
        "errors": 0,
    }

    files = sorted(PRESETS_ROOT.rglob("*.xometa"))
    for filepath in files:
        # Skip quarantine — those are known-bad and intentionally isolated
        if "_quarantine" in str(filepath):
            continue

        try:
            data = json.loads(filepath.read_text(encoding="utf-8"))
        except (json.JSONDecodeError, OSError) as e:
            print(f"  ERROR reading {filepath.name}: {e}")
            counters["errors"] += 1
            continue

        fixed, case = fix_preset(data)
        if not case:
            counters["skipped"] += 1
            continue

        counters[case] += 1
        if dry_run:
            print(f"  WOULD FIX [{case}]: {filepath.name}")
        else:
            try:
                filepath.write_text(
                    json.dumps(fixed, indent=2, ensure_ascii=False) + "\n",
                    encoding="utf-8",
                )
            except OSError as e:
                print(f"  ERROR writing {filepath.name}: {e}")
                counters["errors"] += 1
                counters[case] -= 1  # undo count since write failed

    return counters


def main():
    dry_run = "--dry-run" in sys.argv
    mode = "DRY RUN" if dry_run else "FIXING"
    print(f"=== fix_425_447_macro_format.py — {mode} ===")
    print(f"Scanning: {PRESETS_ROOT}")
    print()

    counters = process_all(dry_run)

    print()
    print("Results:")
    print(f"  Ocelot string-M1 fixed     : {counters['ocelot_str_m1']}")
    print(f"  Offering M1 with macroLabels: {counters['offering_m1_with_ml']}")
    print(f"  Offering M1 no macroLabels  : {counters['offering_m1_no_ml']}")
    print(f"  Standard macros no labels   : {counters['std_macros_no_ml']}")
    total_fixed = sum(
        v for k, v in counters.items() if k not in ("skipped", "errors")
    )
    print(f"  Total fixed                 : {total_fixed}")
    print(f"  Skipped (no fix needed)     : {counters['skipped']}")
    print(f"  Errors                      : {counters['errors']}")

    if dry_run and total_fixed > 0:
        print()
        print("Run without --dry-run to apply fixes.")


if __name__ == "__main__":
    main()
