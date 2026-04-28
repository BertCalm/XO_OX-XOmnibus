#!/usr/bin/env python3
"""
Fix duplicate preset names in Presets/XOceanus/ .xometa files.

Strategy:
  1. Scan all .xometa files recursively.
  2. Group by the "name" field.
  3. For names appearing in 2+ files:
     - If each duplicate is from a different engine, append " (EngineName)" to
       disambiguate.  When a preset has multiple engines, use the first one.
     - If two files share the same engine label (or would produce the same
       disambiguated name), append " (Alt)", " (Alt 2)", etc. to the later ones.
  4. Write the updated "name" back into the JSON. Never rename files.
"""

import json
import os
import sys
from collections import defaultdict
from pathlib import Path

PRESETS_ROOT = Path(__file__).resolve().parent / "Presets" / "XOceanus"


def engine_label(meta: dict, filepath: Path) -> str:
    """Return a short engine label for disambiguation.

    Prefer the 'engines' list in the JSON.  Fall back to the parent-directory
    name if the list is empty.
    """
    engines = meta.get("engines", [])
    if engines:
        return engines[0]
    # Fall back to the immediate parent directory name
    return filepath.parent.name


def load_all_presets(root: Path):
    """Return list of (path, meta_dict) for every .xometa file under root."""
    records = []
    for path in sorted(root.rglob("*.xometa")):
        try:
            with open(path, "r", encoding="utf-8") as fh:
                meta = json.load(fh)
            records.append((path, meta))
        except Exception as exc:
            print(f"  SKIP (parse error): {path}  [{exc}]", file=sys.stderr)
    return records


def fix_duplicates(records):
    """
    Mutate meta dicts in-place to resolve ALL duplicate names globally.

    Strategy:
      - Iterate until no duplicates remain (handles cascading collisions).
      - Each round: for every duplicate group, propose "(EngineName)" suffix.
        If the proposed name STILL collides (either within the group or with
        any other name in the corpus), append "(Alt)", "(Alt 2)", etc.

    Returns a list of (path, old_name, new_name) for every change made.
    """
    changes: list[tuple] = []
    original_names: dict[int, str] = {
        idx: meta.get("name", "") for idx, (_, meta) in enumerate(records)
    }

    for _round in range(20):  # safety limit; should converge in 1-2 rounds
        # Build current name → indices mapping
        by_name: dict[str, list[int]] = defaultdict(list)
        for idx, (path, meta) in enumerate(records):
            name = meta.get("name", "")
            if name:
                by_name[name].append(idx)

        duplicates = {n: ids for n, ids in by_name.items() if len(ids) > 1}
        if not duplicates:
            break  # all clear

        # All names currently in use (we must not collide with these either)
        all_current_names: set[str] = set(by_name.keys())

        for dup_name, indices in duplicates.items():
            # Step 1: propose "(EngineName)" for each item in the group
            proposed: dict[int, str] = {}
            for idx in indices:
                path, meta = records[idx]
                label = engine_label(meta, path)
                proposed[idx] = f"{dup_name} ({label})"

            # Step 2: resolve collisions globally — a proposed name must not
            #   a) collide with another proposed name in the SAME group, OR
            #   b) already exist in all_current_names (from a DIFFERENT group)

            # Collect names we intend to "take" so we can detect intra-group dups
            reserved: set[str] = set()  # names assigned so far in this group

            for idx in indices:
                candidate = proposed[idx]
                alt_num = 0
                while candidate in reserved or (
                    candidate in all_current_names and candidate != dup_name
                ):
                    alt_num += 1
                    suffix = "(Alt)" if alt_num == 1 else f"(Alt {alt_num})"
                    candidate = f"{proposed[idx]} {suffix}"
                proposed[idx] = candidate
                reserved.add(candidate)

            # Step 3: apply to meta dicts and record changes
            for idx in indices:
                path, meta = records[idx]
                old_name = meta["name"]
                new_name = proposed[idx]
                meta["name"] = new_name
                # Track the cumulative change from the ORIGINAL name
                changes.append((path, old_name, new_name))

    return changes


def write_changes(records, changes):
    """Write back only the files whose name was changed."""
    changed_paths = {path for path, _, _ in changes}
    written = 0
    for path, meta in records:
        if path in changed_paths:
            try:
                with open(path, "w", encoding="utf-8") as fh:
                    json.dump(meta, fh, indent=2, ensure_ascii=False)
                    fh.write("\n")
                written += 1
            except Exception as exc:
                print(f"  ERROR writing {path}: {exc}", file=sys.stderr)
    return written


def main():
    print(f"Scanning {PRESETS_ROOT} ...")
    records = load_all_presets(PRESETS_ROOT)
    print(f"Loaded {len(records)} .xometa files.\n")

    changes = fix_duplicates(records)

    if not changes:
        print("No duplicate names found. Nothing to do.")
        return

    # Summarise
    unique_original_names = len({old for _, old, _ in changes})
    print(f"Found {unique_original_names} duplicate name(s) across {len(changes)} files.\n")
    print(f"{'FILE':<70}  {'OLD NAME':<40}  NEW NAME")
    print("-" * 160)
    for path, old, new in sorted(changes, key=lambda x: str(x[0])):
        rel = path.relative_to(PRESETS_ROOT)
        print(f"{str(rel):<70}  {old:<40}  {new}")

    written = write_changes(records, changes)
    print(f"\n--- Done: {written} file(s) updated. ---")
    print(f"    Duplicate groups fixed : {unique_original_names}")
    print(f"    Total name fields changed : {len(changes)}")


if __name__ == "__main__":
    main()
