#!/usr/bin/env python3
"""
fix_mood_and_entangled.py

Fix 1: Mood/Folder Mismatch
  For every .xometa in Presets/XOmnibus/ (not _quarantine/), if the JSON
  `mood` field doesn't match the parent folder name, update the `mood` field
  to match the folder name.  Skips non-standard mood folders.

Fix 2: Re-Mood Solo Entangled Presets
  Find all presets in Entangled/ that are single-engine AND have no coupling
  pairs.  Re-assign their mood based on DNA profile and move the file.

Usage:
  python fix_mood_and_entangled.py            # dry-run (default)
  python fix_mood_and_entangled.py --apply    # write changes
"""

import argparse
import json
import os
import shutil
import sys
from collections import defaultdict
from pathlib import Path

# ---------------------------------------------------------------------------
# Constants
# ---------------------------------------------------------------------------

PRESETS_ROOT = Path(__file__).parent.parent / "Presets" / "XOmnibus"

STANDARD_MOODS = {
    "Foundation",
    "Atmosphere",
    "Entangled",
    "Prism",
    "Flux",
    "Aether",
    "Family",
    "Submerged",
}

# Folders that exist but are outside scope for Fix 1 (separate issue)
SKIP_FOLDERS = {
    "Crystalline",
    "Deep",
    "Ethereal",
    "Kinetic",
    "Luminous",
    "Organic",
}


# ---------------------------------------------------------------------------
# DNA → Mood scorer
# ---------------------------------------------------------------------------

def best_mood_for_dna(dna: dict) -> str:
    """Pick the best mood based on DNA values."""
    brightness = dna.get("brightness", 0.5)
    warmth = dna.get("warmth", 0.5)
    movement = dna.get("movement", 0.5)
    density = dna.get("density", 0.5)
    space = dna.get("space", 0.5)
    aggression = dna.get("aggression", 0.5)

    scores = {
        "Foundation": density * 0.4 + warmth * 0.3 + (1 - space) * 0.3,
        "Atmosphere": space * 0.4 + (1 - aggression) * 0.3 + warmth * 0.3,
        "Prism": brightness * 0.4 + (1 - density) * 0.3 + movement * 0.3,
        "Flux": movement * 0.4 + aggression * 0.3 + brightness * 0.3,
        "Aether": (
            space * 0.3
            + (1 - aggression) * 0.3
            + (1 - density) * 0.2
            + brightness * 0.2
        ),
        "Submerged": (
            (1 - brightness) * 0.4 + space * 0.3 + (1 - aggression) * 0.3
        ),
    }
    return max(scores, key=scores.get)


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def load_xometa(path: Path) -> dict:
    with open(path, encoding="utf-8") as fh:
        return json.load(fh)


def save_xometa(path: Path, data: dict, apply: bool) -> None:
    if apply:
        with open(path, "w", encoding="utf-8") as fh:
            fh.write(json.dumps(data, indent=2, ensure_ascii=False))
            fh.write("\n")


def move_file(src: Path, dst_dir: Path, apply: bool) -> Path:
    """Move src into dst_dir.  Returns the new path."""
    dst = dst_dir / src.name
    if apply:
        dst_dir.mkdir(parents=True, exist_ok=True)
        shutil.move(str(src), str(dst))
    return dst


# ---------------------------------------------------------------------------
# Fix 1
# ---------------------------------------------------------------------------

def fix_mood_mismatch(apply: bool) -> list[dict]:
    """
    Walk all standard mood folders (not _quarantine, not skip folders).
    If mood JSON field != folder name, update it.

    Returns list of change records:
        {"file": str, "old_mood": str, "new_mood": str}
    """
    changes = []

    for folder in sorted(PRESETS_ROOT.iterdir()):
        if not folder.is_dir():
            continue
        folder_name = folder.name
        if folder_name.startswith("_"):
            continue  # skip _quarantine etc.
        if folder_name in SKIP_FOLDERS:
            continue
        if folder_name not in STANDARD_MOODS:
            # Unknown folder — skip silently
            continue

        for xometa in sorted(folder.glob("*.xometa")):
            data = load_xometa(xometa)
            old_mood = data.get("mood", "")
            if old_mood != folder_name:
                changes.append(
                    {
                        "file": str(xometa),
                        "old_mood": old_mood,
                        "new_mood": folder_name,
                    }
                )
                if apply:
                    data["mood"] = folder_name
                    save_xometa(xometa, data, apply=True)

    return changes


# ---------------------------------------------------------------------------
# Fix 2
# ---------------------------------------------------------------------------

def fix_solo_entangled(apply: bool) -> list[dict]:
    """
    Find single-engine presets in Entangled/ with no coupling pairs.
    Re-mood them by DNA and optionally move files.

    Returns list of change records:
        {"file": str, "new_mood": str, "dna": dict}
    """
    entangled_dir = PRESETS_ROOT / "Entangled"
    if not entangled_dir.is_dir():
        print("WARNING: Entangled/ folder not found — skipping Fix 2.", file=sys.stderr)
        return []

    changes = []

    for xometa in sorted(entangled_dir.glob("*.xometa")):
        data = load_xometa(xometa)

        engines = data.get("engines") or []
        coupling = data.get("coupling") or {}
        pairs = coupling.get("pairs") or []

        # Only re-mood single-engine presets with no coupling pairs
        if len(engines) != 1 or len(pairs) != 0:
            continue

        dna = data.get("dna") or {}
        new_mood = best_mood_for_dna(dna)

        changes.append(
            {
                "file": str(xometa),
                "new_mood": new_mood,
                "dna": dna,
            }
        )

        if apply:
            data["mood"] = new_mood
            # Save to existing location first, then move
            save_xometa(xometa, data, apply=True)
            dst_dir = PRESETS_ROOT / new_mood
            move_file(xometa, dst_dir, apply=True)

    return changes


# ---------------------------------------------------------------------------
# Reporting
# ---------------------------------------------------------------------------

def report_fix1(changes: list[dict], apply: bool) -> None:
    verb = "Updated" if apply else "Would update"
    print(f"\n=== Fix 1: Mood/Folder Mismatch ===")
    print(f"{verb} {len(changes)} mood field(s) to match folder name.")
    if changes:
        # Group by new_mood for a compact summary
        by_mood: dict[str, list] = defaultdict(list)
        for c in changes:
            by_mood[c["new_mood"]].append(c)
        for mood in sorted(by_mood):
            entries = by_mood[mood]
            print(f"  {mood}: {len(entries)} file(s)")
            for c in entries[:5]:
                fname = Path(c["file"]).name
                print(f"    {fname!r}  [{c['old_mood']!r} → {mood!r}]")
            if len(entries) > 5:
                print(f"    ... and {len(entries) - 5} more")


def report_fix2(changes: list[dict], apply: bool) -> None:
    verb = "Moved" if apply else "Would move"
    print(f"\n=== Fix 2: Re-Mood Solo Entangled Presets ===")
    print(
        f"{verb} {len(changes)} preset(s) out of Entangled/ "
        f"(single-engine, no coupling pairs)."
    )
    if changes:
        by_dest: dict[str, list] = defaultdict(list)
        for c in changes:
            by_dest[c["new_mood"]].append(c)
        print("  Breakdown by destination mood:")
        for mood in sorted(by_dest):
            entries = by_dest[mood]
            print(f"    {mood}: {len(entries)} preset(s)")
        print()
        # Show a sample from each destination
        for mood in sorted(by_dest):
            entries = by_dest[mood]
            print(f"  → {mood} ({len(entries)} presets):")
            for c in entries[:3]:
                fname = Path(c["file"]).name
                dna = c["dna"]
                dna_str = (
                    f"b={dna.get('brightness', 0.5):.2f} "
                    f"w={dna.get('warmth', 0.5):.2f} "
                    f"m={dna.get('movement', 0.5):.2f} "
                    f"d={dna.get('density', 0.5):.2f} "
                    f"s={dna.get('space', 0.5):.2f} "
                    f"a={dna.get('aggression', 0.5):.2f}"
                )
                print(f"    {fname}  [{dna_str}]")
            if len(entries) > 3:
                print(f"    ... and {len(entries) - 3} more")


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main() -> None:
    parser = argparse.ArgumentParser(
        description="Fix mood/folder mismatches and re-mood solo Entangled presets."
    )
    parser.add_argument(
        "--apply",
        action="store_true",
        default=False,
        help="Write changes to disk (default: dry-run only).",
    )
    args = parser.parse_args()

    apply = args.apply
    mode = "APPLY" if apply else "DRY-RUN"
    print(f"fix_mood_and_entangled.py — mode: {mode}")
    print(f"Presets root: {PRESETS_ROOT}")

    # --- Fix 1 ---
    fix1_changes = fix_mood_mismatch(apply=apply)
    report_fix1(fix1_changes, apply=apply)

    # --- Fix 2 ---
    fix2_changes = fix_solo_entangled(apply=apply)
    report_fix2(fix2_changes, apply=apply)

    # --- Summary ---
    print("\n=== Summary ===")
    verb = "updated" if apply else "to update"
    print(f"  Fix 1: {len(fix1_changes)} mood field(s) {verb} to match folder name")

    by_dest: dict[str, int] = defaultdict(int)
    for c in fix2_changes:
        by_dest[c["new_mood"]] += 1
    dest_str = ", ".join(
        f"{mood} ({n})" for mood, n in sorted(by_dest.items())
    )
    moved_verb = "moved" if apply else "to move"
    if dest_str:
        print(
            f"  Fix 2: {len(fix2_changes)} preset(s) {moved_verb} from Entangled → {dest_str}"
        )
    else:
        print(f"  Fix 2: 0 presets {moved_verb} from Entangled")

    if not apply:
        print(
            "\nThis was a dry-run. Pass --apply to write changes."
        )


if __name__ == "__main__":
    main()
