#!/usr/bin/env python3
"""Exhaustive .xometa preset quality audit for XOmnibus."""

import json
import os
import sys
from collections import Counter, defaultdict
from pathlib import Path

PRESET_ROOT = Path("/home/user/XO_OX-XOmnibus/Presets/XOmnibus")  # FIXME: hardcoded path — should use os.path.join or argparse
PROJECT_ROOT = Path("/home/user/XO_OX-XOmnibus")  # FIXME: hardcoded path — should use os.path.join or argparse
MAX_NAME_LEN = 30
DNA_DIMS = {"brightness", "warmth", "movement", "density", "space", "aggression"}
REQUIRED_FIELDS = ["mood", "tags", "author", "dna", "macroLabels", "engines", "parameters"]

def load_preset(path):
    with open(path, "r", encoding="utf-8") as f:
        return json.load(f)

def get_preset_name(data, path):
    """Get preset name from JSON or filename."""
    if "name" in data and data["name"]:
        return data["name"]
    return Path(path).stem

def main():
    # Collect all .xometa files under Presets/XOmnibus
    all_files = sorted(PRESET_ROOT.rglob("*.xometa"))
    print(f"Total .xometa files under Presets/XOmnibus: {len(all_files)}")
    print("=" * 80)

    # Load all presets
    presets = []
    load_errors = []
    for f in all_files:
        try:
            data = load_preset(f)
            presets.append((f, data))
        except Exception as e:
            load_errors.append((f, str(e)))

    if load_errors:
        print(f"\nLOAD ERRORS: {len(load_errors)} files failed to parse")
        for path, err in load_errors[:20]:
            print(f"  {path.relative_to(PROJECT_ROOT)}: {err}")

    print(f"Successfully loaded: {len(presets)}")

    # =========================================================================
    # 1. NAMES OVER 30 CHARS
    # =========================================================================
    print("\n" + "=" * 80)
    print("1. PRESET NAMES OVER 30 CHARACTERS")
    print("=" * 80)

    long_names = []
    for path, data in presets:
        name = get_preset_name(data, path)
        if len(name) > MAX_NAME_LEN:
            long_names.append((name, len(name), path))

    long_names.sort(key=lambda x: -x[1])
    print(f"Total presets with name > 30 chars: {len(long_names)}")
    print(f"\nTop 20 longest names:")
    for name, length, path in long_names[:20]:
        mood_folder = path.parent.name
        print(f"  [{length} chars] \"{name}\" ({mood_folder})")

    # =========================================================================
    # 2. NULL/MISSING FIELDS
    # =========================================================================
    print("\n" + "=" * 80)
    print("2. NULL OR MISSING FIELDS")
    print("=" * 80)

    missing_counts = {field: 0 for field in REQUIRED_FIELDS}
    missing_counts["name"] = 0

    for path, data in presets:
        # Check name
        if "name" not in data or data["name"] is None or data["name"] == "":
            missing_counts["name"] += 1

        for field in REQUIRED_FIELDS:
            if field not in data or data[field] is None:
                missing_counts[field] += 1
            elif isinstance(data[field], (list, dict, str)) and len(data[field]) == 0:
                missing_counts[field] += 1

    print(f"{'Field':<15} {'Missing/Null/Empty Count':>25}")
    print("-" * 42)
    for field in ["name"] + REQUIRED_FIELDS:
        count = missing_counts[field]
        status = "OK" if count == 0 else f"** {count} **"
        print(f"  {field:<15} {status:>25}")

    # =========================================================================
    # 3. ENTANGLED COUPLING GAP
    # =========================================================================
    print("\n" + "=" * 80)
    print("3. ENTANGLED COUPLING GAP ANALYSIS")
    print("=" * 80)

    entangled_dir = PRESET_ROOT / "Entangled"
    entangled_files = sorted(entangled_dir.rglob("*.xometa")) if entangled_dir.exists() else []
    print(f"Total Entangled presets: {len(entangled_files)}")

    entangled_valid_coupling = 0
    entangled_missing_coupling = 0
    entangled_single_engine = 0
    entangled_multi_engine_no_coupling = []
    entangled_single_engine_list = []

    for path in entangled_files:
        try:
            data = load_preset(path)
        except:
            continue

        engines = data.get("engines", [])
        coupling = data.get("coupling", data.get("couplingPairs", []))
        has_multi_engine = len(engines) >= 2
        has_coupling = coupling is not None and len(coupling) > 0

        if not has_multi_engine:
            entangled_single_engine += 1
            entangled_single_engine_list.append((get_preset_name(data, path), engines))
        elif has_coupling:
            entangled_valid_coupling += 1
        else:
            entangled_missing_coupling += 1
            entangled_multi_engine_no_coupling.append(get_preset_name(data, path))

    print(f"  Valid coupling (2+ engines + coupling data): {entangled_valid_coupling}")
    print(f"  Missing coupling (2+ engines, NO coupling):  {entangled_missing_coupling}")
    print(f"  Single-engine in Entangled:                   {entangled_single_engine}")

    if entangled_multi_engine_no_coupling:
        print(f"\n  Multi-engine presets MISSING coupling (first 20):")
        for name in entangled_multi_engine_no_coupling[:20]:
            print(f"    - {name}")
        if len(entangled_multi_engine_no_coupling) > 20:
            print(f"    ... and {len(entangled_multi_engine_no_coupling) - 20} more")

    if entangled_single_engine_list:
        print(f"\n  Single-engine Entangled presets (first 20):")
        for name, engines in entangled_single_engine_list[:20]:
            print(f"    - {name} (engines: {engines})")
        if len(entangled_single_engine_list) > 20:
            print(f"    ... and {len(entangled_single_engine_list) - 20} more")

    # =========================================================================
    # 4. FAMILY MOOD CONSISTENCY
    # =========================================================================
    print("\n" + "=" * 80)
    print("4. FAMILY MOOD CONSISTENCY")
    print("=" * 80)

    family_dir = PRESET_ROOT / "Family"
    family_files = sorted(family_dir.rglob("*.xometa")) if family_dir.exists() else []
    print(f"Total Family presets: {len(family_files)}")

    family_mood_mismatch = 0
    family_mood_mismatch_list = []
    family_high_aggression = 0
    family_high_aggression_list = []

    for path in family_files:
        try:
            data = load_preset(path)
        except:
            continue

        name = get_preset_name(data, path)
        mood = data.get("mood", "")
        if mood != "Family":
            family_mood_mismatch += 1
            family_mood_mismatch_list.append((name, mood))

        dna = data.get("dna", {})
        aggression = dna.get("aggression", 0)
        if isinstance(aggression, (int, float)) and aggression > 0.5:
            family_high_aggression += 1
            family_high_aggression_list.append((name, aggression))

    print(f"  Mood field mismatches (not 'Family'): {family_mood_mismatch}")
    if family_mood_mismatch_list:
        for name, mood in family_mood_mismatch_list[:20]:
            print(f"    - \"{name}\" has mood=\"{mood}\"")

    print(f"  Presets with aggression DNA > 0.5:     {family_high_aggression}")
    if family_high_aggression_list:
        for name, agg in sorted(family_high_aggression_list, key=lambda x: -x[1])[:20]:
            print(f"    - \"{name}\" aggression={agg}")

    # =========================================================================
    # 5. DUPLICATE NAMES
    # =========================================================================
    print("\n" + "=" * 80)
    print("5. DUPLICATE PRESET NAMES")
    print("=" * 80)

    name_counter = Counter()
    name_paths = defaultdict(list)
    for path, data in presets:
        name = get_preset_name(data, path)
        name_lower = name.strip().lower()
        name_counter[name_lower] += 1
        name_paths[name_lower].append((name, path))

    dupes = {k: v for k, v in name_counter.items() if v > 1}
    print(f"Total unique names (case-insensitive): {len(name_counter)}")
    print(f"Duplicate name groups: {len(dupes)}")
    total_dupe_files = sum(v for v in dupes.values())
    print(f"Total files involved in duplicates: {total_dupe_files}")

    if dupes:
        sorted_dupes = sorted(dupes.items(), key=lambda x: -x[1])
        print(f"\nDuplicate names (showing up to 30):")
        for name_key, count in sorted_dupes[:30]:
            entries = name_paths[name_key]
            print(f"  \"{entries[0][0]}\" x{count}:")
            for orig_name, path in entries[:5]:
                print(f"    - {path.relative_to(PROJECT_ROOT)}")
            if len(entries) > 5:
                print(f"    ... and {len(entries) - 5} more")

    # =========================================================================
    # 6. DNA VALIDATION
    # =========================================================================
    print("\n" + "=" * 80)
    print("6. DNA VALIDATION (6 dimensions, values in [0.0, 1.0])")
    print("=" * 80)

    dna_missing_dims = 0
    dna_out_of_range = 0
    dna_missing_entirely = 0
    dna_issues = []

    for path, data in presets:
        name = get_preset_name(data, path)
        dna = data.get("dna", None)

        if dna is None or not isinstance(dna, dict) or len(dna) == 0:
            dna_missing_entirely += 1
            continue

        present_dims = set(dna.keys())
        missing_dims = DNA_DIMS - present_dims

        if missing_dims:
            dna_missing_dims += 1
            dna_issues.append((name, f"missing dims: {missing_dims}"))

        for dim in DNA_DIMS:
            val = dna.get(dim)
            if val is not None:
                if not isinstance(val, (int, float)):
                    dna_out_of_range += 1
                    dna_issues.append((name, f"{dim}={val} (not numeric)"))
                elif val < 0.0 or val > 1.0:
                    dna_out_of_range += 1
                    dna_issues.append((name, f"{dim}={val} (out of range)"))

    print(f"  DNA missing entirely:       {dna_missing_entirely}")
    print(f"  DNA missing dimensions:     {dna_missing_dims}")
    print(f"  DNA values out of [0,1]:    {dna_out_of_range}")
    total_dna_violations = dna_missing_entirely + dna_missing_dims + dna_out_of_range
    print(f"  TOTAL DNA violations:       {total_dna_violations}")

    if dna_issues:
        print(f"\n  DNA issues (first 20):")
        for name, issue in dna_issues[:20]:
            print(f"    - \"{name}\": {issue}")

    # =========================================================================
    # 7. MACRO VALIDATION (exactly 4 non-empty labels)
    # =========================================================================
    print("\n" + "=" * 80)
    print("7. MACRO LABEL VALIDATION (exactly 4 non-empty strings)")
    print("=" * 80)

    macro_violations = 0
    macro_missing = 0
    macro_wrong_count = 0
    macro_empty_labels = 0
    macro_issues = []

    for path, data in presets:
        name = get_preset_name(data, path)
        macros = data.get("macroLabels", None)

        if macros is None or not isinstance(macros, list):
            macro_missing += 1
            macro_violations += 1
            continue

        if len(macros) != 4:
            macro_wrong_count += 1
            macro_violations += 1
            macro_issues.append((name, f"count={len(macros)}, labels={macros}"))
            continue

        has_empty = any(not isinstance(m, str) or m.strip() == "" for m in macros)
        if has_empty:
            macro_empty_labels += 1
            macro_violations += 1
            macro_issues.append((name, f"empty label in {macros}"))

    print(f"  macroLabels missing/null:   {macro_missing}")
    print(f"  Wrong count (not 4):        {macro_wrong_count}")
    print(f"  Has empty label strings:    {macro_empty_labels}")
    print(f"  TOTAL macro violations:     {macro_violations}")

    if macro_issues:
        print(f"\n  Macro issues (first 20):")
        for name, issue in macro_issues[:20]:
            print(f"    - \"{name}\": {issue}")

    # =========================================================================
    # 8. ORPHAN PRESETS (outside Presets/XOmnibus/)
    # =========================================================================
    print("\n" + "=" * 80)
    print("8. ORPHAN PRESETS (outside Presets/XOmnibus/)")
    print("=" * 80)

    orphans = []
    for root, dirs, files in os.walk(PROJECT_ROOT):
        root_path = Path(root)
        # Skip the main preset directory
        if str(root_path).startswith(str(PRESET_ROOT)):
            continue
        for f in files:
            if f.endswith(".xometa"):
                orphans.append(root_path / f)

    print(f"  Total orphan .xometa files: {len(orphans)}")

    # Group by parent directory
    orphan_dirs = Counter()
    for o in orphans:
        rel = o.parent.relative_to(PROJECT_ROOT)
        orphan_dirs[str(rel)] += 1

    if orphan_dirs:
        print(f"\n  Orphan locations:")
        for dir_path, count in sorted(orphan_dirs.items(), key=lambda x: -x[1]):
            print(f"    {dir_path}/  ({count} files)")

    # =========================================================================
    # SUMMARY
    # =========================================================================
    print("\n" + "=" * 80)
    print("AUDIT SUMMARY")
    print("=" * 80)
    print(f"  Total presets scanned:          {len(presets)}")
    print(f"  Load errors:                    {len(load_errors)}")
    print(f"  Names > 30 chars:               {len(long_names)}")
    print(f"  Null/missing field violations:  {sum(missing_counts.values())}")
    print(f"  Entangled missing coupling:     {entangled_missing_coupling}")
    print(f"  Entangled single-engine:        {entangled_single_engine}")
    print(f"  Family mood mismatches:         {family_mood_mismatch}")
    print(f"  Family high aggression:         {family_high_aggression}")
    print(f"  Duplicate name groups:          {len(dupes)}")
    print(f"  DNA violations:                 {total_dna_violations}")
    print(f"  Macro violations:               {macro_violations}")
    print(f"  Orphan presets:                 {len(orphans)}")

if __name__ == "__main__":
    main()
