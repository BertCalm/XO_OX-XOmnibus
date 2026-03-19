#!/usr/bin/env python3
"""
xpn_entangled_preset_fixer.py — Add second-engine coupling to single-engine Entangled presets

The coupling auditor found 235 Entangled mood presets with only 1 engine — likely migrated
from standalone instruments without being upgraded. This tool auto-fixes them by adding
the most appropriate second engine partner based on coupling affinity rules.

Usage:
    python xpn_entangled_preset_fixer.py <presets_dir> [--engine FILTER] [--dry-run] [--apply] [--format text|json]

Options:
    --dry-run       Show what would change, don't write files (default)
    --apply         Write changes to files (creates .bak backup first)
    --engine NAME   Only fix presets for this engine (e.g. Overdub)
    --format        Output format: text (default) or json
"""

import json
import glob
import argparse
import shutil
import sys
from pathlib import Path

# ---------------------------------------------------------------------------
# Coupling affinity rules
# Maps engine ID (canonical) → preferred partner engine ID
# ---------------------------------------------------------------------------

# Canonical engine ID aliases for lookup (handles legacy names)
ENGINE_ALIASES = {
    "snap": "OddfeliX",
    "oddfelix": "OddfeliX",
    "morph": "OddOscar",
    "oddoscar": "OddOscar",
    "dub": "Overdub",
    "overdub": "Overdub",
    "drift": "Odyssey",
    "odyssey": "Odyssey",
    "bob": "Oblong",
    "oblong": "Oblong",
    "fat": "Obese",
    "obese": "Obese",
    "bite": "Overbite",
    "overbite": "Overbite",
    "opal": "Opal",
    "onset": "Onset",
    "orbital": "Orbital",
    "oracle": "Oracle",
    "ouroboros": "Ouroboros",
    "overworld": "Overworld",
    "organon": "Organon",
    "obsidian": "Obsidian",
    "origami": "Origami",
    "obscura": "Obscura",
    "oceanic": "Oceanic",
    "ocelot": "Ocelot",
    "optic": "Optic",
    "oblique": "Oblique",
    "osprey": "Osprey",
    "osteria": "Osteria",
    "owlfish": "Owlfish",
    "ohm": "Ohm",
    "orphica": "Orphica",
    "obbligato": "Obbligato",
    "ottoni": "Ottoni",
    "ole": "Ole",
    "overlap": "Overlap",
    "outwit": "Outwit",
    "ombre": "Ombre",
    "orca": "Orca",
    "octopus": "Octopus",
}

# Explicit affinity pairs (engine → preferred partner)
AFFINITY_MAP = {
    "OddfeliX":  "OddOscar",     # feliX polarity → Oscar polarity
    "OddOscar":  "OddfeliX",     # Oscar → feliX
    "Overdub":   "Opal",         # tape + granular: canonical Entangled
    "Opal":      "Overdub",      # granular → tape dub
    "Onset":     "Orbital",      # drum machine + group envelope
    "Orbital":   "Onset",        # group envelope ← drum machine
    "Oblong":    "Obese",        # bass character pair
    "Obese":     "Oblong",       # saturation ← bass character
    "Oracle":    "Ouroboros",    # stochastic + chaos
    "Ouroboros": "Oracle",       # chaos → stochastic
    "Odyssey":   "OddfeliX",     # FM analog → bright neon
    "Overworld": "Odyssey",      # chip → FM retro
    "Overbite":  "Obese",        # bite + fat: aggressive pair
    "Organon":   "Ouroboros",    # metabolic + chaos
    "Obsidian":  "OddOscar",     # crystal → warm axolotl
    "Origami":   "OddfeliX",     # fold/bright → neon
    "Obscura":   "OddOscar",     # daguerreotype silver → warm Oscar
    "Oceanic":   "Organon",      # bioluminescent pair
    "Ocelot":    "Overworld",    # biome tawny → chip world
    "Optic":     "OddfeliX",     # visual phosphor → neon feliX
    "Oblique":   "OddOscar",     # prism violet → warm Oscar
    "Osprey":    "Osteria",      # shore pair
    "Osteria":   "Osprey",       # port wine ← azure shore
    "Owlfish":   "Organon",      # abyssal + metabolic
    "Ohm":       "OddOscar",     # sage commune → warm Oscar
    "Orphica":   "Opal",         # microsound harp → granular
    "Obbligato": "Overdub",      # wind breath → tape dub
    "Ottoni":    "Organon",      # brass grow → metabolic
    "Ole":       "OddfeliX",     # latin drama → bright neon
    "Overlap":   "Opal",         # FDN reverb → granular cloud
    "Outwit":    "Ouroboros",    # cellular automata → chaos
    "Ombre":     "OddOscar",     # shadow mauve → warm Oscar
    "Orca":      "Onset",        # apex predator → drum machine
    "Octopus":   "Outwit",       # 8-arm CA → CA chaos
}

# DNA brightness threshold: if brightness > this, prefer feliX; else Oscar
DNA_BRIGHTNESS_THRESHOLD = 0.5


def normalize_engine_id(raw: str) -> str:
    """Resolve legacy or lowercase engine names to canonical form."""
    return ENGINE_ALIASES.get(raw.lower(), raw)


def pick_partner(engine_id: str, dna: dict) -> str:
    """
    Return the best partner engine for a single-engine Entangled preset.
    Priority: explicit AFFINITY_MAP → DNA brightness fallback.
    """
    canonical = normalize_engine_id(engine_id)
    if canonical in AFFINITY_MAP:
        return AFFINITY_MAP[canonical]
    # Fallback: use DNA brightness to choose feliX or Oscar
    brightness = dna.get("brightness", 0.5) if dna else 0.5
    return "OddfeliX" if brightness >= DNA_BRIGHTNESS_THRESHOLD else "OddOscar"


def is_single_engine_entangled(preset: dict) -> bool:
    """Return True if preset is Entangled mood with exactly 1 engine."""
    return (
        preset.get("mood") == "Entangled"
        and isinstance(preset.get("engines"), list)
        and len(preset["engines"]) == 1
    )


def scan_presets(presets_dir: str, engine_filter) -> list[dict]:
    """
    Scan all .xometa files under presets_dir.
    Returns list of dicts: {path, preset, engine, partner}
    """
    pattern = str(Path(presets_dir) / "**" / "*.xometa")
    all_files = glob.glob(pattern, recursive=True)

    results = []
    for fpath in sorted(all_files):
        try:
            with open(fpath, "r", encoding="utf-8") as f:
                preset = json.load(f)
        except (json.JSONDecodeError, OSError) as e:
            print(f"  WARNING: could not parse {fpath}: {e}", file=sys.stderr)
            continue

        if not is_single_engine_entangled(preset):
            continue

        engine = preset["engines"][0]
        canonical_engine = normalize_engine_id(engine)

        # Apply optional engine filter
        if engine_filter:
            if canonical_engine.lower() != engine_filter.lower() and engine.lower() != engine_filter.lower():
                continue

        dna = preset.get("dna", {})
        partner = pick_partner(engine, dna)

        results.append({
            "path": fpath,
            "preset": preset,
            "engine": engine,
            "partner": partner,
        })

    return results


def build_fix(entry: dict) -> dict:
    """
    Return the updated preset dict with second engine added and coupling fields set.
    Does NOT mutate entry['preset'] in place.
    """
    import copy
    preset = copy.deepcopy(entry["preset"])
    partner = entry["partner"]

    # Add partner engine
    if partner not in preset["engines"]:
        preset["engines"].append(partner)

    # Set coupling fields
    preset["couplingType"] = "STANDARD"
    preset["couplingIntensity"] = 0.5

    # Ensure coupling.pairs exists (add skeleton if missing)
    if not preset.get("coupling") or not isinstance(preset.get("coupling"), dict):
        preset["coupling"] = {"pairs": []}
    if not preset["coupling"].get("pairs"):
        preset["coupling"]["pairs"] = [
            {
                "engineA": entry["engine"],
                "engineB": partner,
                "type": "Amp->Filter",
                "amount": 0.5,
            }
        ]

    return preset


def format_text_report(entries: list[dict], mode: str) -> str:
    lines = []
    title = f"ENTANGLED PRESET FIXER — {mode}"
    lines.append(title)
    lines.append(f"Found {len(entries)} single-engine Entangled preset(s)\n")

    for entry in entries:
        rel = Path(entry["path"]).name
        # Try to show mood/engine subfolder if it exists
        parts = Path(entry["path"]).parts
        try:
            mood_idx = [p.lower() for p in parts].index("entangled")
            rel = "/".join(parts[mood_idx:])
        except ValueError:
            pass

        lines.append(f"  {rel}")
        lines.append(f'    engines: {json.dumps([entry["engine"]])} → {json.dumps([entry["engine"], entry["partner"]])}')

        preset = entry["preset"]
        old_ct = preset.get("couplingType", "(none)")
        old_ci = preset.get("couplingIntensity", "(none)")
        lines.append(f'    couplingType: {old_ct!r} → "STANDARD"')
        lines.append(f'    couplingIntensity: {old_ci!r} → 0.5')

    if mode == "dry run":
        lines.append("\nTo apply: rerun with --apply")

    return "\n".join(lines)


def format_json_report(entries: list[dict], mode: str) -> str:
    out = {
        "mode": mode,
        "count": len(entries),
        "presets": [],
    }
    for entry in entries:
        preset = entry["preset"]
        out["presets"].append({
            "path": entry["path"],
            "engine": entry["engine"],
            "proposed_partner": entry["partner"],
            "engines_before": [entry["engine"]],
            "engines_after": [entry["engine"], entry["partner"]],
            "couplingType_before": preset.get("couplingType"),
            "couplingType_after": "STANDARD",
            "couplingIntensity_before": preset.get("couplingIntensity"),
            "couplingIntensity_after": 0.5,
        })
    return json.dumps(out, indent=2)


def apply_fixes(entries: list[dict]) -> tuple[int, int]:
    """Write updated .xometa files. Creates .bak backups first. Returns (ok, failed)."""
    ok = 0
    failed = 0
    for entry in entries:
        fpath = entry["path"]
        try:
            # Backup
            shutil.copy2(fpath, fpath + ".bak")
            # Build updated preset
            updated = build_fix(entry)
            with open(fpath, "w", encoding="utf-8") as f:
                json.dump(updated, f, indent=2, ensure_ascii=False)
                f.write("\n")
            ok += 1
        except OSError as e:
            print(f"  ERROR writing {fpath}: {e}", file=sys.stderr)
            failed += 1
    return ok, failed


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Auto-fix single-engine Entangled presets by adding a second engine partner."
    )
    parser.add_argument("presets_dir", help="Root presets directory to scan")
    parser.add_argument(
        "--engine",
        metavar="ENGINE",
        default=None,
        help="Only fix presets for this engine (e.g. Overdub)",
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        default=False,
        help="Show what would change, don't write (default if neither flag given)",
    )
    parser.add_argument(
        "--apply",
        action="store_true",
        default=False,
        help="Write changes to files (creates .bak backup first)",
    )
    parser.add_argument(
        "--format",
        choices=["text", "json"],
        default="text",
        help="Output format (default: text)",
    )
    args = parser.parse_args()

    # Default to dry-run if neither --dry-run nor --apply specified
    if not args.apply:
        args.dry_run = True

    presets_dir = Path(args.presets_dir)
    if not presets_dir.is_dir():
        print(f"ERROR: '{presets_dir}' is not a directory.", file=sys.stderr)
        sys.exit(1)

    entries = scan_presets(str(presets_dir), args.engine)

    mode = "dry run" if args.dry_run else "apply"

    if args.format == "json":
        print(format_json_report(entries, mode))
    else:
        print(format_text_report(entries, mode))

    if args.apply and entries:
        ok, failed = apply_fixes(entries)
        print(f"\nApplied: {ok} updated, {failed} failed.")
        if failed:
            sys.exit(1)


if __name__ == "__main__":
    main()
