#!/usr/bin/env python3
"""
F4 Sound Designer Audit Fixes
  Fix 1: Rename "Bob" from OddfeliX/OblongBob migration artifacts
  Fix 2: Add tier:awakening to all ONSET + OPERA presets missing a tier
  Fix 3: Update Conductor's Crescendo drama parameter from 0.0 to 0.6
"""

import json
import os
import re
import shutil
import sys
from pathlib import Path

PRESET_ROOT = Path("/Users/joshuacramblet/Documents/GitHub/XO_OX-XOmnibus/Presets/XOlokun")

# ---------------------------------------------------------------------------
# Replacement map: "Bob" word → audible descriptor
# Rules:
#   - "Bob" is the Oblong engine's legacy display name (parameter prefix: bob_)
#   - Replace with descriptors from: weight, body, depth, pressure, resonance,
#     stomp, current, drift, pulse, mass, core, dense, thick, low, warm, submerge
# ---------------------------------------------------------------------------

# Each entry: (old_name, new_name, new_filename_stem)
# new_filename_stem must be derived from new_name (spaces → spaces, underscores preserved)
# We define explicit renames for every unique internal name.

BOB_NAME_MAP = {
    # Presets where "Bob" is the leading word (engine tag, not descriptive)
    "Bob Arp Warm":           "Warm Arp Low",
    "Bob Bounce Trap":        "Mass Bounce Trap",
    "Bob Cross-Tide":         "Current Cross-Tide",
    "Bob Crystal Keys":       "Resonance Crystal Keys",
    "Bob Dawn Presence":      "Warm Dawn Presence",
    "Bob Drift Atmosphere":   "Weight Drift Atmosphere",
    "Bob Drift Fade":         "Depth Drift Fade",
    "Bob Drift Floating":     "Current Drift Floating",
    "Bob Drift FM Two":       "Body Drift FM Two",
    "Bob Drift Glide Lead":   "Low Drift Glide Lead",
    "Bob Drift Harmony":      "Resonance Drift Harmony",
    "Bob Drift Horizon":      "Depth Drift Horizon",
    "Bob Drift Morph Melody": "Warm Drift Morph Melody",
    "Bob Drift Pulse":        "Weight Drift Pulse",
    "Bob Drift Snap Spectral":"Dense Drift Snap Spectral",
    "Bob Drift Spectral Chord":"Resonance Drift Spectral Chord",
    "Bob Drift Twilight":     "Warm Drift Twilight",
    "Bob Drift Warm Layer":   "Body Drift Warm Layer",
    "Bob Drift Weave":        "Current Drift Weave",
    "Bob Dub Choke":          "Low Dub Choke",
    "Bob Dub Classic":        "Body Dub Classic",
    "Bob Dub Decay":          "Mass Dub Decay",
    "Bob Dub Deep Space":     "Depth Dub Deep Space",
    "Bob Dub Ether":          "Low Dub Ether",
    "Bob Dub Flux Rhythm":    "Dense Dub Flux Rhythm",
    "Bob Dub Synco":          "Body Dub Synco",
    "Bob Dust Only":          "Pressure Dust Only",
    "Bob Entangle":           "Low Entangle",
    "Bob Fast Arp":           "Dense Fast Arp",
    "Bob Fat Decay":          "Mass Fat Decay",
    "Bob Fat Deep Aether":    "Depth Fat Deep Aether",
    "Bob Fat Drift Cosmos":   "Weight Fat Drift Cosmos",
    "Bob Fat Drift Full Prism":"Dense Fat Drift Full Prism",
    "Bob Fat Drift Snap Four":"Mass Fat Drift Snap Four",
    "Bob Fat Dub Soulful":    "Body Fat Dub Soulful",
    "Bob Fat Fast Rhythm":    "Dense Fat Fast Rhythm",
    "Bob Fat Gate":           "Mass Fat Gate",
    "Bob Fat LFO":            "Body Fat LFO",
    "Bob Fat Morph Four":     "Weight Fat Morph Four",
    "Bob Fat Morph Room":     "Dense Fat Morph Room",
    "Bob Fat Pad Layer":      "Mass Fat Pad Layer",
    "Bob Fat Prism Layer":    "Resonance Fat Prism Layer",
    "Bob Fat Pulse":          "Mass Fat Pulse",
    "Bob Fat Warp":           "Dense Fat Warp",
    "Bob Filament":           "Low Filament",
    "Bob Filter Rip":         "Pressure Filter Rip",
    "Bob Funky Stab":         "Mass Funky Stab",
    "Bob Ghost Pad":          "Depth Ghost Pad",
    "Bob Glide":              "Body Glide",
    "Bob Glide Character":    "Weight Glide Character",
    "Bob Glide Run":          "Current Glide Run",
    "Bob Haze":               "Low Haze",
    "Bob Morph Dub Run":      "Dense Morph Dub Run",
    "Bob Morph Fat Aether":   "Body Morph Fat Aether",
    "Bob Morph Fat Prism Three":"Mass Morph Fat Prism Three",
    "Bob Morph Filter Mirror":"Resonance Morph Filter Mirror",
    "Bob Morph Flux Layer":   "Dense Morph Flux Layer",
    "Bob Morph Forest":       "Body Morph Forest",
    "Bob Morph Snap Dynamic": "Weight Morph Snap Dynamic",
    "Bob Morph Spectrum":     "Resonance Morph Spectrum",
    "Bob Morph Wavetable":    "Dense Morph Wavetable",
    "Bob Night Pad":          "Depth Night Pad",
    "Bob Seq Pluck":          "Low Seq Pluck",
    "Bob Smoke Room":         "Warm Smoke Room",
    "Bob Snap Choke Two":     "Mass Snap Choke Two",
    "Bob Snap Dub Kit":       "Low Snap Dub Kit",
    "Bob Snap Rhythm":        "Dense Snap Rhythm",
    "Bob Solo Lead":          "Body Solo Lead",
    "Bob Spectral Lead":      "Resonance Spectral Lead",
    "Bob Sync Lead":          "Dense Sync Lead",
    "Bob Under":              "Depth Under",
    "Bob Vibrato Lead":       "Body Vibrato Lead",
    "Bob Wah Sweep":          "Stomp Wah Sweep",
    "Bob Warmth":             "Low Warmth",
    "Bob Bright Keys":        "Body Bright Keys",
    "Bob Commune":            "Low Commune",
    "Bob Dark Prism":         "Dense Dark Prism",
    "Bob Light Prism":        "Resonance Light Prism",
    "Bob Prism Riot":         "Mass Prism Riot",
    "Bob Scatter":            "Dense Scatter",
    "Bob Tremolo":            "Body Tremolo",
    "Bob Alive":              "Current Alive",
    "Bob Dissolved":          "Depth Dissolved",
    "Bob Rage":               "Pressure Rage",
    "Bob Bounce":             "Mass Bounce",
    "Bob Twilight":           "Depth Twilight",
    "Bob Optical":            "Resonance Optical",

    # Multi-engine presets where "Bob" sits in middle or end position
    # Here "Bob" is a shorthand for Oblong engine — replace with "Low" or "Body"
    "Drift Bob Celestial":    "Drift Low Celestial",
    "Drift Bob Cosmos":       "Drift Low Cosmos",
    "Drift Bob Foundation":   "Drift Body Foundation",
    "Drift Bob Harmony Two":  "Drift Body Harmony Two",
    "Drift Bob Morph Flux":   "Drift Dense Morph Flux",
    "Drift Bob Orbit":        "Drift Body Orbit",
    "Drift Bob Particle Field":"Drift Low Particle Field",
    "Drift Bob Wavetable":    "Drift Body Wavetable",
    "Drift Fat Bob Coupled":  "Drift Fat Body Coupled",
    "Drift Fat Bob Trinity":  "Drift Fat Body Trinity",
    "Drift Fat Bob Warmth":   "Drift Fat Warm Body",
    "Drift Morph Bob Fat Prism":"Drift Morph Dense Fat Prism",

    "Dub Bob Drift Riddim":   "Dub Low Drift Riddim",
    "Dub Bob Fat Drift Full": "Dub Body Fat Drift Full",
    "Dub Bob Harbor Two":     "Dub Body Harbor Two",
    "Dub Bob Harmony":        "Dub Body Harmony",
    "Dub Bob Interlock":      "Dub Low Interlock",
    "Dub Bob Morph Snap Four":"Dub Body Morph Snap Four",
    "Dub Bob Morph Three Foundation":"Dub Body Morph Three Foundation",
    "Dub Bob Rain Room":      "Dub Body Rain Room",
    "Dub Bob Reverb Pool":    "Dub Low Reverb Pool",
    "Dub Bob Riddim":         "Dub Low Riddim",
    "Dub Bob Ring":           "Dub Body Ring",
    "Dub Bob Wavetable":      "Dub Body Wavetable",
    "Dub Drift Bob Forest":   "Dub Drift Low Forest",
    "Dub Fat Bob Groove":     "Dub Fat Body Groove",
    "Dub Fat Drift Bob Landscape":"Dub Fat Drift Body Landscape",

    "Fat Bob Arp Layer":      "Fat Body Arp Layer",
    "Fat Bob Atmosphere Deep":"Fat Body Atmosphere Deep",
    "Fat Bob Drift Dub World":"Fat Body Drift Dub World",
    "Fat Bob Drift Three Foundation":"Fat Body Drift Three Foundation",
    "Fat Bob Drift Weather":  "Fat Body Drift Weather",
    "Fat Bob Dub Three":      "Fat Body Dub Three",
    "Fat Bob Flux Lead":      "Fat Body Flux Lead",
    "Fat Bob FM":             "Fat Body FM",
    "Fat Bob Morph Aether":   "Fat Body Morph Aether",
    "Fat Bob Morph Groove":   "Fat Body Morph Groove",
    "Fat Bob Morph Landscape":"Fat Body Morph Landscape",
    "Fat Bob Pitch":          "Fat Body Pitch",
    "Fat Bob Snap Prism Three":"Fat Body Snap Prism Three",
    "Fat Bob Void Deep":      "Fat Body Void Deep",
    "Fat Drift Bob Prism Two":"Fat Drift Body Prism Two",
    "Fat Drift Bob Three":    "Fat Drift Body Three",
    "Fat Snap Bob Groove":    "Fat Snap Body Groove",

    "Gnash Bob":              "Gnash Low",
    "Mojo Bob":               "Mojo Low",

    "Morph Bob Drift Three":  "Morph Body Drift Three",
    "Morph Bob Fade":         "Morph Body Fade",
    "Morph Bob Gate":         "Morph Body Gate",
    "Morph Bob Ghost Layer":  "Morph Body Ghost Layer",
    "Morph Bob Glide":        "Morph Body Glide",
    "Morph Bob Spectral Two": "Morph Body Spectral Two",
    "Morph Drift Bob Aurora": "Morph Drift Body Aurora",
    "Morph Dub Bob Trio":     "Morph Dub Body Trio",
    "Morph Snap Bob Garden":  "Morph Snap Body Garden",

    "Snap Bob Crystal Two":   "Crystal Snap Low Two",
    "Snap Bob Drift Ghost":   "Snap Body Drift Ghost",
    "Snap Bob Drift Prism":   "Snap Body Drift Prism",
    "Snap Bob Drift Sunset":  "Snap Body Drift Sunset",
    "Snap Bob Dub Kit":       "Snap Low Dub Kit",
    "Snap Bob Fat Dub Groove":"Snap Body Fat Dub Groove",
    "Snap Bob Fat Environment":"Snap Body Fat Environment",
    "Snap Bob FM":            "Snap Body FM",
    "Snap Bob Layer":         "Snap Body Layer",
    "Snap Bob LFO":           "Snap Body LFO",
    "Snap Bob Morph Dub Full":"Snap Body Morph Dub Full",
    "Snap Bob Morph Trigger": "Snap Body Morph Trigger",
    "Snap Bob Spark":         "Snap Body Spark",
    "Snap Drift Bob Mist":    "Snap Drift Body Mist",
    "Snap Dub Bob Three":     "Snap Dub Body Three",
    "Snap Morph Bob Prism":   "Snap Morph Body Prism",

    "Sub Bob":                "Sub Body",
    "Thick Bob Mass":         "Thick Body Mass",

    # Oblong_* coupling presets (name field already drops "Oblong_" prefix)
    "Tidal Bob":              "Tidal Current",
    "Sage Bob":               "Sage Resonance",
    "Drama Bob":              "Drama Pressure",
    "Predator Bob":           "Predator Current",
    "Harp Bob":               "Harp Resonance",
    "Brass Bob":              "Brass Body",
    "Subharmonic Bob":        "Subharmonic Body",
    "Spectral Bob":           "Spectral Body",
    "Bright Bob":             "Bright Body",

    # These already have good names in the name field (filename has Bob, internal doesn't)
    # Amber Twitch, Nap Pad, Tide Margin, Sniff Machine, Evaporation Zone — skip
    # Vowel Depth, Pelagic Glide, Surface Strike (Oblong), Music Box, Strange Bass — skip
}

# Files whose internal name is ALREADY good (no Bob in name field)
SKIP_INTERNAL_NAMES = {
    "Amber Twitch",
    "Nap Pad",
    "Tide Margin",
    "Sniff Machine",
    "Evaporation Zone",
    "Vowel Depth",
    "Pelagic Glide",
    "Surface Strike (Oblong)",
    "Music Box",
    "Strange Bass",
}


def name_to_filename_stem(name: str) -> str:
    """Convert a preset name to a filesystem-safe filename stem."""
    # Preserve spaces, replace characters that are problematic on some FSes
    # The existing convention uses both spaces and underscores; we'll match the
    # source style (spaces for multi-word, underscores when original had them)
    return name.replace("/", "-").replace(":", "")


def process_bob_rename(dry_run: bool = False) -> dict:
    """
    Scan all non-quarantine preset directories for files with 'Bob' in
    filename or internal name, apply renames.
    Returns stats dict.
    """
    stats = {
        "files_scanned": 0,
        "name_updated": 0,
        "file_renamed": 0,
        "skipped_already_clean": 0,
        "skipped_internal_clean": 0,
        "renames": [],
    }

    # Collect every xometa file (excluding _quarantine)
    all_xometa = []
    for mood_dir in PRESET_ROOT.iterdir():
        if mood_dir.name.startswith("_") or not mood_dir.is_dir():
            continue
        for f in mood_dir.glob("*.xometa"):
            all_xometa.append(f)

    for fpath in sorted(all_xometa):
        stats["files_scanned"] += 1
        fname = fpath.stem  # filename without .xometa

        # Quick check: does "Bob" appear in filename OR content?
        has_bob_in_filename = "bob" in fname.lower() or "Bob" in fname
        try:
            with open(fpath, "r", encoding="utf-8") as fh:
                data = json.load(fh)
        except Exception as e:
            print(f"  ERROR reading {fpath}: {e}", file=sys.stderr)
            continue

        internal_name = data.get("name", "")
        has_bob_in_name = "Bob" in internal_name or "bob" in internal_name

        if not has_bob_in_filename and not has_bob_in_name:
            continue  # Nothing to do

        # Check if internal name is already clean (intentional)
        if internal_name in SKIP_INTERNAL_NAMES:
            # Only the filename may have Bob — rename file if it does, but name is fine
            if has_bob_in_filename and "Bob" in fname:
                # The filename contains Bob but name field is clean already
                # Just rename the file to match the internal name
                new_stem = name_to_filename_stem(internal_name)
                new_path = fpath.with_name(new_stem + ".xometa")
                if new_path != fpath:
                    if not dry_run:
                        shutil.move(str(fpath), str(new_path))
                    stats["file_renamed"] += 1
                    stats["renames"].append({
                        "old_file": str(fpath),
                        "new_file": str(new_path),
                        "old_name": internal_name,
                        "new_name": internal_name,
                        "action": "file_rename_only",
                    })
            else:
                stats["skipped_internal_clean"] += 1
            continue

        # Look up new name
        new_name = BOB_NAME_MAP.get(internal_name)
        if new_name is None:
            # If Bob appears in name but we have no mapping, log it
            if has_bob_in_name:
                print(f"  WARN: no mapping for '{internal_name}' ({fpath.name})")
            continue

        if new_name == internal_name:
            stats["skipped_already_clean"] += 1
            continue

        # Compute new filename
        new_stem = name_to_filename_stem(new_name)
        new_path = fpath.parent / (new_stem + ".xometa")

        # Build updated JSON
        data["name"] = new_name
        updated_json = json.dumps(data, indent=2, ensure_ascii=False)

        if not dry_run:
            # Write updated content to NEW path
            with open(new_path, "w", encoding="utf-8") as fh:
                fh.write(updated_json)
                fh.write("\n")
            # Remove old file if the path changed
            if new_path != fpath:
                fpath.unlink()

        stats["name_updated"] += 1
        if new_path != fpath:
            stats["file_renamed"] += 1

        stats["renames"].append({
            "old_file": str(fpath),
            "new_file": str(new_path),
            "old_name": internal_name,
            "new_name": new_name,
            "action": "full_rename",
        })

    return stats


def process_tier_tags(dry_run: bool = False) -> dict:
    """
    Add "tier": "awakening" to all ONSET and OPERA presets (including _quarantine)
    that do not already have a tier field.
    Returns stats dict.
    """
    stats = {
        "onset_files": 0,
        "opera_files": 0,
        "tier_added": 0,
        "already_had_tier": 0,
    }

    target_engines = {"Onset", "Opera"}

    for mood_dir in PRESET_ROOT.iterdir():
        if not mood_dir.is_dir():
            continue
        # Use rglob to capture engine-specific subdirectories (e.g. Atmosphere/Onset/)
        for fpath in mood_dir.rglob("*.xometa"):
            try:
                with open(fpath, "r", encoding="utf-8") as fh:
                    data = json.load(fh)
            except Exception as e:
                print(f"  ERROR reading {fpath}: {e}", file=sys.stderr)
                continue

            engines = data.get("engines", [])
            # Check if this preset is exclusively or primarily Onset/Opera
            involved = set(engines) & target_engines
            if not involved:
                continue

            if "Onset" in involved:
                stats["onset_files"] += 1
            if "Opera" in involved:
                stats["opera_files"] += 1

            if "tier" in data:
                stats["already_had_tier"] += 1
                continue

            # Insert tier at the top of the JSON (after schema_version if present)
            # We'll add it to the dict and re-serialize with consistent ordering
            data["tier"] = "awakening"
            updated_json = json.dumps(data, indent=2, ensure_ascii=False)

            if not dry_run:
                with open(fpath, "w", encoding="utf-8") as fh:
                    fh.write(updated_json)
                    fh.write("\n")

            stats["tier_added"] += 1

    return stats


def fix_conductors_crescendo(dry_run: bool = False) -> dict:
    """
    Update Conductor's Crescendo: set opera_drama to 0.6.
    The preset is a linear arc (opera_arcMode=1) that ramps from silence
    to operatic climax — drama=0.0 makes the starting state a lie.
    drama=0.6 gives the engine a vivid starting timbre before the arc takes hold.
    """
    target = PRESET_ROOT / "Flux" / "Conductor's Crescendo.xometa"
    stats = {"found": False, "updated": False, "old_drama": None, "new_drama": 0.6}

    if not target.exists():
        return stats

    stats["found"] = True
    with open(target, "r", encoding="utf-8") as fh:
        data = json.load(fh)

    old_drama = data.get("parameters", {}).get("Opera", {}).get("opera_drama")
    stats["old_drama"] = old_drama

    if old_drama == 0.6:
        return stats  # already fixed

    data["parameters"]["Opera"]["opera_drama"] = 0.6

    if not dry_run:
        updated_json = json.dumps(data, indent=2, ensure_ascii=False)
        with open(target, "w", encoding="utf-8") as fh:
            fh.write(updated_json)
            fh.write("\n")

    stats["updated"] = True
    return stats


def main():
    dry_run = "--dry-run" in sys.argv

    if dry_run:
        print("=== DRY RUN MODE — no files will be modified ===\n")

    # --- Fix 1: Bob renames ---
    print("FIX 1: Bob preset renames")
    print("-" * 60)
    bob_stats = process_bob_rename(dry_run=dry_run)
    print(f"  Files scanned: {bob_stats['files_scanned']}")
    print(f"  Names updated: {bob_stats['name_updated']}")
    print(f"  Files renamed: {bob_stats['file_renamed']}")
    print(f"  Skipped (internal name already clean): {bob_stats['skipped_internal_clean']}")
    if bob_stats["renames"]:
        print("\n  Rename log:")
        for r in bob_stats["renames"]:
            action = r["action"]
            if action == "file_rename_only":
                print(f"    [file only] {r['old_file']!r}")
                print(f"             → {r['new_file']!r}")
            else:
                print(f"    '{r['old_name']}' → '{r['new_name']}'")
                if r["old_file"] != r["new_file"]:
                    old_fname = Path(r["old_file"]).name
                    new_fname = Path(r["new_file"]).name
                    print(f"    file: {old_fname!r} → {new_fname!r}")

    print()

    # --- Fix 2: ONSET/OPERA tier tags ---
    print("FIX 2: ONSET + OPERA tier metadata")
    print("-" * 60)
    tier_stats = process_tier_tags(dry_run=dry_run)
    print(f"  Onset presets found:  {tier_stats['onset_files']}")
    print(f"  Opera presets found:  {tier_stats['opera_files']}")
    print(f"  Tier tags added:      {tier_stats['tier_added']}")
    print(f"  Already had tier:     {tier_stats['already_had_tier']}")
    print()

    # --- Fix 3: Conductor's Crescendo ---
    print("FIX 3: Conductor's Crescendo — drama parameter")
    print("-" * 60)
    cc_stats = fix_conductors_crescendo(dry_run=dry_run)
    if not cc_stats["found"]:
        print("  WARNING: Conductor's Crescendo.xometa not found!")
    elif cc_stats["updated"]:
        print(f"  opera_drama: {cc_stats['old_drama']} → {cc_stats['new_drama']}")
        print("  File updated.")
    else:
        print(f"  opera_drama already at {cc_stats['old_drama']} — no change needed.")
    print()

    print("=== SUMMARY ===")
    print(f"  Bob renames applied:     {bob_stats['name_updated']}")
    print(f"  Tier tags added:         {tier_stats['tier_added']}")
    print(f"  Conductor's Crescendo:   {'drama=0.6 applied' if cc_stats['updated'] else 'already correct or not found'}")
    if dry_run:
        print("\n  (dry run — rerun without --dry-run to apply)")


if __name__ == "__main__":
    main()
