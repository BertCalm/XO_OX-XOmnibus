#!/usr/bin/env python3
"""fix_446_macro_prefix_migration.py — Restore macro parameter values stripped by fix_preset_macros.py.

Issue #446: 3,200+ presets lost their macro parameter values when fix_preset_macros.py
removed `macro_character`, `macro_movement`, `macro_coupling`, and `macro_space` keys
without migrating them to the correct engine-prefixed format.

This script:
  1. Reads each .xometa.bak file (pre-strip backups) for macro_ values.
  2. Translates macro_character → {prefix}_macroCharacter (and the other 3 macros)
     using the frozen prefix table from CLAUDE.md.
  3. Merges those values into the live .xometa file WITHOUT overwriting any existing
     engine-prefixed params already present.
  4. Only writes files that actually needed changes (idempotent).

Usage:
    python3 Tools/fix_446_macro_prefix_migration.py [--dry-run]

Options:
    --dry-run   Print what would change without writing any files.
"""

import json
import os
import sys
from pathlib import Path

# ---------------------------------------------------------------------------
# Frozen prefix table (from CLAUDE.md, never changes)
# ---------------------------------------------------------------------------
ENGINE_PREFIXES = {
    "OddfeliX":   "snap_",
    "OddOscar":   "morph_",
    "Overdub":    "dub_",
    "Odyssey":    "drift_",
    "Oblong":     "bob_",
    "Obese":      "fat_",
    "Overbite":   "poss_",
    "Onset":      "perc_",
    "Overworld":  "ow_",
    "Opal":       "opal_",
    "Orbital":    "orb_",
    "Organon":    "organon_",
    "Ouroboros":  "ouro_",
    "Obsidian":   "obsidian_",
    "Origami":    "origami_",
    "Oracle":     "oracle_",
    "Obscura":    "obscura_",
    "Oceanic":    "ocean_",
    "Ocelot":     "ocelot_",
    "Optic":      "optic_",
    "Oblique":    "oblq_",
    "Osprey":     "osprey_",
    "Osteria":    "osteria_",
    "Owlfish":    "owl_",
    "Ohm":        "ohm_",
    "Orphica":    "orph_",
    "Obbligato":  "obbl_",
    "Ottoni":     "otto_",
    "Ole":        "ole_",
    "Ombre":      "ombre_",
    "Orca":       "orca_",
    "Octopus":    "octo_",
    "Ostinato":   "osti_",
    "OpenSky":    "sky_",
    "OceanDeep":  "deep_",
    "Ouie":       "ouie_",
    "Overlap":    "olap_",
    "Outwit":     "owit_",
    "Obrix":      "obrix_",
    "Orbweave":   "weave_",
    "Overtone":   "over_",
    "Organism":   "org_",
    "Oxbow":      "oxb_",
    "Oware":      "owr_",
    "Opera":      "opera_",
    "Offering":   "ofr_",
    "Osmosis":    "osmo_",
    "Oxytocin":   "oxy_",
    "Oto":        "oto_",
    "Octave":     "oct_",
    "Oleg":       "oleg_",
    "Otis":       "otis_",
    "Oven":       "oven_",
    "Ochre":      "ochre_",
    "Obelisk":    "obel_",
    "Opaline":    "opal2_",
    "Ogre":       "ogre_",
    "Olate":      "olate_",
    "Oaken":      "oaken_",
    "Omega":      "omega_",
    "Orchard":    "orch_",
    "Overgrow":   "grow_",
    "Osier":      "osier_",
    "Oxalis":     "oxal_",
    "Overwash":   "wash_",
    "Overworn":   "worn_",
    "Overflow":   "flow_",
    "Overcast":   "cast_",
    "Oasis":      "oas_",
    "Oddfellow":  "oddf_",
    "Onkolo":     "onko_",
    "Opcode":     "opco_",
    "Outlook":    "look_",
    "Obiont":     "obnt_",
    "Okeanos":    "okan_",
    "Outflow":    "out_",
}

# Legacy alias resolution (from PresetManager.h resolveEngineAlias)
ENGINE_ALIASES = {
    "Snap":  "OddfeliX",
    "Morph": "OddOscar",
    "Dub":   "Overdub",
    "Drift": "Odyssey",
    "Bob":   "Oblong",
    "Fat":   "Obese",
    "Bite":  "Overbite",
}

# The 4 generic macro keys from old presets → camelCase suffix for new key
MACRO_KEY_MAP = {
    "macro_character": "macroCharacter",
    "macro_movement":  "macroMovement",
    "macro_coupling":  "macroCoupling",
    "macro_space":     "macroSpace",
}


def resolve_engine(name: str) -> str:
    """Resolve legacy engine aliases to canonical names."""
    return ENGINE_ALIASES.get(name, name)


def get_prefix(engine_name: str) -> str | None:
    """Return the frozen parameter prefix for an engine, or None if unknown."""
    canonical = resolve_engine(engine_name)
    return ENGINE_PREFIXES.get(canonical)


def migrate_file(bak_path: Path, xometa_path: Path, dry_run: bool) -> int:
    """
    Read macro_ values from bak_path, merge corrected keys into xometa_path.

    Returns the number of parameter keys added (0 if nothing to do).
    """
    try:
        with open(bak_path, encoding="utf-8") as f:
            bak_data = json.load(f)
    except (json.JSONDecodeError, IOError) as exc:
        print(f"  SKIP (bak parse error): {bak_path.name} — {exc}", file=sys.stderr)
        return 0

    try:
        with open(xometa_path, encoding="utf-8") as f:
            xometa_data = json.load(f)
    except (json.JSONDecodeError, IOError) as exc:
        print(f"  SKIP (xometa parse error): {xometa_path.name} — {exc}", file=sys.stderr)
        return 0

    bak_params = bak_data.get("parameters", {})
    if not isinstance(bak_params, dict):
        return 0

    xometa_params = xometa_data.get("parameters")
    if not isinstance(xometa_params, dict):
        return 0

    keys_added = 0

    for engine_name, bak_engine_block in bak_params.items():
        if not isinstance(bak_engine_block, dict):
            continue

        # Collect macro_ values from the backup
        macro_values = {
            MACRO_KEY_MAP[k]: v
            for k, v in bak_engine_block.items()
            if k in MACRO_KEY_MAP
        }
        if not macro_values:
            continue

        prefix = get_prefix(engine_name)
        if prefix is None:
            print(
                f"  WARN: unknown engine '{engine_name}' in {bak_path.name} — skipping",
                file=sys.stderr,
            )
            continue

        # Ensure the engine block exists in xometa
        if engine_name not in xometa_params:
            xometa_params[engine_name] = {}
        live_block = xometa_params[engine_name]
        if not isinstance(live_block, dict):
            continue

        # Write corrected keys, but don't overwrite already-present keys
        for suffix, value in macro_values.items():
            new_key = prefix + suffix  # e.g. "bob_macroCharacter"
            if new_key not in live_block:
                live_block[new_key] = value
                keys_added += 1

    if keys_added == 0:
        return 0

    if not dry_run:
        with open(xometa_path, "w", encoding="utf-8") as f:
            json.dump(xometa_data, f, indent=2, ensure_ascii=False)
            f.write("\n")

    return keys_added


def main():
    dry_run = "--dry-run" in sys.argv

    script_dir = Path(__file__).resolve().parent
    repo_root = script_dir.parent
    presets_root = repo_root / "Presets" / "XOceanus"

    if not presets_root.is_dir():
        print(f"ERROR: Presets directory not found: {presets_root}", file=sys.stderr)
        sys.exit(1)

    if dry_run:
        print("DRY RUN — no files will be written.\n")

    bak_files = sorted(presets_root.rglob("*.xometa.bak"))
    print(f"Found {len(bak_files)} .xometa.bak files under {presets_root}\n")

    total_files_changed = 0
    total_keys_added = 0
    skipped_no_xometa = 0

    for bak_path in bak_files:
        xometa_path = bak_path.with_suffix("")  # strip trailing .bak
        if not xometa_path.exists():
            skipped_no_xometa += 1
            continue

        added = migrate_file(bak_path, xometa_path, dry_run=dry_run)
        if added > 0:
            total_files_changed += 1
            total_keys_added += added
            rel = xometa_path.relative_to(repo_root)
            tag = "[DRY] " if dry_run else ""
            print(f"  {tag}+{added:3d} key(s): {rel}")

    print()
    print("=" * 60)
    print("Summary")
    print("=" * 60)
    print(f"  .bak files scanned   : {len(bak_files)}")
    print(f"  Skipped (no .xometa) : {skipped_no_xometa}")
    print(f"  Files updated        : {total_files_changed}")
    print(f"  Keys added           : {total_keys_added}")
    print()
    if total_files_changed == 0:
        print("  No files required changes (already migrated or no macro_ values found).")
    else:
        action = "Would update" if dry_run else "Updated"
        print(f"  {action} {total_files_changed} preset(s) with {total_keys_added} corrected macro parameter(s).")
    print("=" * 60)


if __name__ == "__main__":
    main()
