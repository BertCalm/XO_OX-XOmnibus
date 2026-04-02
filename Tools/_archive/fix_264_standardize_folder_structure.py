#!/usr/bin/env python3
"""
fix_264_standardize_folder_structure.py — Standardize preset folder structure.

Closes issue #264.

Convention: each mood folder contains per-engine (or per-engine-pair for
coupling presets) subdirectories.  Flat presets — those sitting directly in
the mood folder rather than in a subfolder — are migrated into subfolders
named after the preset's primary engine.

Primary engine resolution:
  - Solo preset (1 engine)  → subfolder named after that engine
  - Coupling preset (2+ engines, coupling array non-empty) → "engine_A-engine_B"
    subfolder using the first coupling pair, or the first two engine names
    alphabetically if no coupling block is present.
  - Fallback → "Unclassified"

Only mood folders listed in MIGRATE_MOODS are touched.  The _quarantine tree
and already-nested presets are left alone.

Usage:
    python3 Tools/fix_264_standardize_folder_structure.py --dry-run
    python3 Tools/fix_264_standardize_folder_structure.py [--moods Foundation,Flux,Aether,Organic]
    python3 Tools/fix_264_standardize_folder_structure.py --all-moods
"""

import json
import shutil
import sys
from pathlib import Path

PRESETS_ROOT = Path(__file__).parent.parent / "Presets" / "XOceanus"

# Default: migrate the four moods that the issue identified as flat
DEFAULT_MOODS = ["Foundation", "Flux", "Aether", "Organic"]

# All non-quarantine moods (use --all-moods to target every mood)
ALL_MOODS = [
    "Aether", "Atmosphere", "Coupling", "Crystalline", "Deep", "Entangled",
    "Ethereal", "Family", "Flux", "Foundation", "Kinetic", "Luminous",
    "Organic", "Prism", "Submerged",
]


def resolve_subfolder(data: dict) -> str:
    """Return the target subfolder name for a preset."""
    engines = data.get("engines")
    coupling = data.get("coupling", {})
    pairs = coupling.get("pairs", []) if isinstance(coupling, dict) else []

    if not isinstance(engines, list) or len(engines) == 0:
        return "Unclassified"

    if len(engines) == 1:
        return engines[0]

    # Coupling preset: prefer the first declared coupling pair
    if pairs and isinstance(pairs[0], dict):
        eng_a = pairs[0].get("engineA", "")
        eng_b = pairs[0].get("engineB", "")
        if eng_a and eng_b:
            return f"{eng_a}-{eng_b}"

    # Fallback: first two engines alphabetically
    sorted_engines = sorted(engines[:2])
    return f"{sorted_engines[0]}-{sorted_engines[1]}"


def migrate_mood(mood_dir: Path, dry_run: bool) -> dict:
    """Migrate flat presets in one mood folder into per-engine subdirs."""
    counters: dict = {"moved": 0, "skipped_already_nested": 0, "errors": 0}

    flat_files = [f for f in mood_dir.iterdir() if f.suffix == ".xometa"]
    for filepath in sorted(flat_files):
        try:
            data = json.loads(filepath.read_text(encoding="utf-8"))
        except (json.JSONDecodeError, OSError) as exc:
            print(f"  ERROR reading {filepath.name}: {exc}")
            counters["errors"] += 1
            continue

        subfolder_name = resolve_subfolder(data)
        dest_dir = mood_dir / subfolder_name
        dest_path = dest_dir / filepath.name

        if dry_run:
            print(f"  WOULD MOVE → {mood_dir.name}/{subfolder_name}/{filepath.name}")
        else:
            try:
                dest_dir.mkdir(exist_ok=True)
                shutil.move(str(filepath), str(dest_path))
            except OSError as exc:
                print(f"  ERROR moving {filepath.name}: {exc}")
                counters["errors"] += 1
                continue

        counters["moved"] += 1

    return counters


def parse_moods_arg(argv: list[str]) -> list[str]:
    """Parse --moods X,Y or --all-moods from argv."""
    if "--all-moods" in argv:
        return ALL_MOODS
    for i, arg in enumerate(argv):
        if arg == "--moods" and i + 1 < len(argv):
            return [m.strip() for m in argv[i + 1].split(",") if m.strip()]
        if arg.startswith("--moods="):
            return [m.strip() for m in arg[len("--moods="):].split(",") if m.strip()]
    return DEFAULT_MOODS


def main() -> None:
    dry_run = "--dry-run" in sys.argv
    moods = parse_moods_arg(sys.argv)
    mode = "DRY RUN" if dry_run else "MIGRATING"

    print(f"=== fix_264_standardize_folder_structure.py — {mode} ===")
    print(f"Target moods: {', '.join(moods)}")
    print()

    total = {"moved": 0, "errors": 0}
    for mood in moods:
        mood_dir = PRESETS_ROOT / mood
        if not mood_dir.is_dir():
            print(f"  SKIP {mood}: directory not found")
            continue

        flat_count = sum(1 for f in mood_dir.iterdir() if f.suffix == ".xometa")
        if flat_count == 0:
            print(f"  {mood}: already fully nested (0 flat presets), skipping")
            continue

        print(f"  {mood}: {flat_count} flat presets to migrate")
        counters = migrate_mood(mood_dir, dry_run)
        total["moved"] += counters["moved"]
        total["errors"] += counters["errors"]

    print()
    print("Results:")
    print(f"  Presets {'would be ' if dry_run else ''}moved: {total['moved']}")
    print(f"  Errors                            : {total['errors']}")

    if dry_run and total["moved"] > 0:
        print()
        print("Run without --dry-run to apply migration.")
        print("Tip: use --moods Foundation,Flux to migrate specific moods only.")
        print("     use --all-moods to migrate every mood folder.")


if __name__ == "__main__":
    main()
