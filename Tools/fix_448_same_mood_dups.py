#!/usr/bin/env python3
"""
fix_448_same_mood_dups.py — Fix 8 same-mood duplicate preset names.

Closes issue #448 (same-mood duplicates only — cross-mood dups are P2).

Fixes applied:

  Coupling/Entangled duplicates (3 pairs): same engines, different descriptions.
  The Coupling/ copy gets a suffix that reflects its actual narrative/technique.

  Spectral* duplicates (5 pairs): different engines share a generic name.
  The second preset in each pair gets an engine-specific qualifier.
"""

import json
import sys
from pathlib import Path

PRESETS_ROOT = Path(__file__).parent.parent / "Presets" / "XOceanus"

# Mapping: filepath (relative to PRESETS_ROOT) → new "name" field value.
# File on disk keeps its filename; only the internal "name" field changes.
RENAMES = {
    # Coupling/ vs Entangled/ — same engines, different descriptions.
    # Rename the Coupling/ copy to reflect its unique angle.
    "Coupling/Coupling_Journey_Atomized.xometa":
        "Journey Scattered",     # ODYSSEY env sweeps OPAL grain scatter (vs full atomization)
    "Coupling/Coupling_Orbit_Collapse.xometa":
        "Orbit Imploding",       # implosion-phase variation vs collapse
    "Coupling/Coupling_Prophecy_Fed.xometa":
        "Prophecy Channeled",    # channeled signal vs fed

    # Spectral* — different engines, generic name collision.
    # Second file in each pair gets an engine qualifier.
    "Prism/Orbital_FM_Crown.xometa":
        "FM Crown",              # Orbital (FM additive) — drop 'Spectral'
    "Aether/Obese_Spectral_Ghost.xometa":
        "Fat Ghost",             # Obese engine accent
    "Flux/Orbital_Spectral_Storm.xometa":
        "FM Storm",              # Orbital FM storm vs Odyssey spectral storm
    "Flux/Oblong_FM_Surge.xometa":
        "FM Surge",              # Oblong FM surge vs Overtone spectral surge
    "Aether/OVERWORLD_FM_Trace.xometa":
        "FM Trace",              # Overworld FM trace vs OddfeliX spectral trace
}


def main():
    dry_run = "--dry-run" in sys.argv
    mode = "DRY RUN" if dry_run else "FIXING"
    print(f"=== fix_448_same_mood_dups.py — {mode} ===")
    fixed = 0
    errors = 0

    for rel_path, new_name in RENAMES.items():
        fp = PRESETS_ROOT / rel_path
        if not fp.exists():
            print(f"  MISSING: {rel_path}")
            errors += 1
            continue

        try:
            data = json.loads(fp.read_text(encoding="utf-8"))
        except (json.JSONDecodeError, OSError) as e:
            print(f"  ERROR reading {fp.name}: {e}")
            errors += 1
            continue

        old_name = data.get("name", "")
        if old_name == new_name:
            print(f"  SKIP (already correct): {fp.name}")
            continue

        print(f"  {'WOULD RENAME' if dry_run else 'RENAMING'}: {old_name!r} → {new_name!r}  [{fp.name}]")
        if not dry_run:
            data["name"] = new_name
            try:
                fp.write_text(
                    json.dumps(data, indent=2, ensure_ascii=False) + "\n",
                    encoding="utf-8",
                )
                fixed += 1
            except OSError as e:
                print(f"    ERROR writing: {e}")
                errors += 1
        else:
            fixed += 1

    print()
    print(f"{'Would fix' if dry_run else 'Fixed'}: {fixed}")
    print(f"Errors: {errors}")
    if dry_run and fixed:
        print("\nRun without --dry-run to apply.")


if __name__ == "__main__":
    main()
