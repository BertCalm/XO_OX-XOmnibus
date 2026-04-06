"""
xpn_manifest_generator.py — XO_OX XPN pack manifest generator

Generates:
  expansion.json       — MPC-format expansion manifest
  bundle_manifest.json — XO_OX extended metadata sidecar

Usage:
  python xpn_manifest_generator.py --pack-name "OBESE Rising" --engine OBESE \
      --mood Foundation --version 1.0.0 \
      [--engines OBESE,OVERDUB] [--bpm 120] [--key C] \
      [--tags saturated,warm,bass] [--output ./]

Also importable:
  from xpn_manifest_generator import generate_manifests
  expansion_json, bundle_json = generate_manifests(pack_config)
"""

import argparse
import json
import os
from datetime import datetime, timezone

# ---------------------------------------------------------------------------
# Constants
# ---------------------------------------------------------------------------

OXPORT_VERSION = "2.0"
MANUFACTURER = "XO_OX Designs"

# Canonical engine short-name → display name mapping (all 34 registered + 4 V1 concept)
ENGINE_DISPLAY_NAMES = {
    "ODDFELIX":  "OddfeliX",
    "ODDOSCAR":  "OddOscar",
    "OVERDUB":   "XOverdub",
    "ODYSSEY":   "XOdyssey",
    "OBLONG":    "XOblong",
    "OBESE":     "XObese",
    "ONSET":     "XOnset",
    "OVERWORLD": "XOverworld",
    "OPAL":      "XOpal",
    "ORGANON":   "XOrganon",
    "OUROBOROS": "XOuroboros",
    "OBSIDIAN":  "XObsidian",
    "ORIGAMI":   "XOrigami",
    "ORACLE":    "XOracle",
    "OBSCURA":   "XObscura",
    "OCEANIC":   "XOceanic",
    "OCELOT":    "XOcelot",
    "OVERBITE":  "XOverbite",
    "ORBITAL":   "XOrbital",
    "OPTIC":     "XOptic",
    "OBLIQUE":   "XOblique",
    "OSPREY":    "XOsprey",
    "OSTERIA":   "XOsteria",
    "OWLFISH":   "XOwlfish",
    "OHM":       "XOhm",
    "ORPHICA":   "XOrphica",
    "OBBLIGATO": "XObbligato",
    "OTTONI":    "XOttoni",
    "OLE":       "XOlé",
    "OVERLAP":   "XOverlap",
    "OUTWIT":    "XOutwit",
    "OMBRE":     "XOmbre",
    "ORCA":      "XOrca",
    "OCTOPUS":   "XOctopus",
    # V1 concept engines (no source yet, valid names)
    "OSTINATO":  "XOstinato",
    "OPENSKY":   "XOpenSky",
    "OCEANDEEP": "XOceanDeep",
    "OUIE":      "XOuïe",
}

VALID_MOODS = {
    "Foundation", "Atmosphere", "Entangled", "Prism", "Flux", "Aether", "Family",
    "Submerged", "Coupling", "Crystalline", "Deep", "Ethereal", "Kinetic", "Luminous", "Organic",
    "Shadow"
}

# Placeholder Sonic DNA — caller should override with real values
DEFAULT_SONIC_DNA = {
    "brightness": 0.5,
    "warmth":     0.5,
    "movement":   0.5,
    "density":    0.5,
    "space":      0.5,
    "aggression": 0.5,
}


# ---------------------------------------------------------------------------
# Core generation functions
# ---------------------------------------------------------------------------

def _resolve_engine_display(engine: str) -> str:
    """Return the display name for an engine short-name, with fallback."""
    key = engine.upper()
    return ENGINE_DISPLAY_NAMES.get(key, f"X{engine.capitalize()}")


def _build_description(pack_name: str, mood: str, primary_engine: str) -> str:
    display = _resolve_engine_display(primary_engine)
    return f"{pack_name} — A {mood} expansion for {display}. {MANUFACTURER}."


def build_expansion_json(pack_config: dict) -> dict:
    """
    Build the MPC-format expansion.json dict.

    Required keys in pack_config:
      pack_name, version, mood, engine (primary)
    Optional:
      description (overrides auto-generated)
    """
    pack_name = pack_config["pack_name"]
    version   = pack_config["version"]
    mood      = pack_config["mood"]
    engine    = pack_config["engine"]

    description = pack_config.get(
        "description",
        _build_description(pack_name, mood, engine),
    )

    return {
        "name":         pack_name,
        "manufacturer": MANUFACTURER,
        "version":      version,
        "type":         "Expansion",
        "description":  description,
    }


def build_bundle_manifest(pack_config: dict) -> dict:
    """
    Build the XO_OX bundle_manifest.json dict.

    Required keys in pack_config:
      pack_name, version, mood, engine (primary)
    Optional:
      engines      — list of engine short-names (defaults to [engine])
      sonic_dna    — dict with 6 float values (defaults to 0.5 placeholders)
      felix_oscar_bias — float 0–1 (default 0.0)
      bpm          — int or None
      key          — string or None
      tags         — list of strings (default [])
      program_count — int (default 0)
      collection   — string or None
    """
    pack_name = pack_config["pack_name"]
    version   = pack_config["version"]
    mood      = pack_config["mood"]
    engine    = pack_config["engine"]

    engines = pack_config.get("engines") or [engine]
    # Normalise to uppercase
    engines = [e.upper() for e in engines]

    sonic_dna = pack_config.get("sonic_dna") or dict(DEFAULT_SONIC_DNA)

    return {
        "schema_version":   2,
        "pack_name":        pack_name,
        "version":          version,
        "build_date":       datetime.now(timezone.utc).strftime("%Y-%m-%d"),
        "oxport_version":   OXPORT_VERSION,
        "engines":          engines,
        "mood":             mood,
        "collection":       pack_config.get("collection"),
        "sonic_dna":        sonic_dna,
        "felix_oscar_bias": pack_config.get("felix_oscar_bias", 0.0),
        "bpm_suggestion":   pack_config.get("bpm"),
        "key_suggestion":   pack_config.get("key"),
        "tags":             pack_config.get("tags") or [],
        "program_count":    pack_config.get("program_count", 0),
        "author":           MANUFACTURER,
    }


def generate_manifests(pack_config: dict) -> tuple:
    """
    Public API — returns (expansion_json_str, bundle_manifest_json_str).

    pack_config keys: see build_expansion_json / build_bundle_manifest above.
    """
    expansion = build_expansion_json(pack_config)
    bundle    = build_bundle_manifest(pack_config)
    return (
        json.dumps(expansion, indent=2, ensure_ascii=False),
        json.dumps(bundle,    indent=2, ensure_ascii=False),
    )


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def parse_args():
    parser = argparse.ArgumentParser(
        description="Generate XO_OX XPN pack manifest files.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  python xpn_manifest_generator.py \\
      --pack-name "OBESE Rising" --engine OBESE --mood Foundation --version 1.0.0

  python xpn_manifest_generator.py \\
      --pack-name "Dub & Fat" --engine OVERDUB --mood Flux --version 1.1.0 \\
      --engines OVERDUB,OBESE --bpm 90 --key Am \\
      --tags dub-pump,warm,saturated,bass --output ./output/
""",
    )

    parser.add_argument("--pack-name",  required=True, help="Pack display name (≤40 chars recommended)")
    parser.add_argument("--engine",     required=True, help="Primary engine short-name (e.g. OBESE)")
    parser.add_argument("--mood",       required=True, choices=sorted(VALID_MOODS), help="Pack mood category")
    parser.add_argument("--version",    required=True, help="Semantic version (e.g. 1.0.0)")
    parser.add_argument("--engines",    default=None,  help="Comma-separated engine list (e.g. OBESE,OVERDUB)")
    parser.add_argument("--bpm",        type=int, default=None, help="BPM suggestion (omit if not rhythmically locked)")
    parser.add_argument("--key",        default=None,  help="Key suggestion (e.g. Am, C)")
    parser.add_argument("--tags",       default=None,  help="Comma-separated tags from canonical vocabulary")
    parser.add_argument("--output",     default="./",  help="Output directory (default: current directory)")
    parser.add_argument("--program-count", type=int, default=0, dest="program_count",
                        help="Number of XPM programs in the pack")
    parser.add_argument("--collection", default=None,
                        help="Collection name (e.g. 'Kitchen Essentials') or omit for standalone")
    parser.add_argument("--felix-oscar-bias", type=float, default=0.0, dest="felix_oscar_bias",
                        help="0.0=pure Oscar (dark/bass) … 1.0=pure feliX (bright/melodic)")
    parser.add_argument("--description", default=None,
                        help="Override auto-generated description")
    parser.add_argument("--dry-run", action="store_true",
                        help="Print output to stdout instead of writing files")

    return parser.parse_args()


def main():
    args = parse_args()

    engines_list = [e.strip() for e in args.engines.split(",")] if args.engines else None
    tags_list    = [t.strip() for t in args.tags.split(",")]    if args.tags    else []

    pack_config = {
        "pack_name":        args.pack_name,
        "engine":           args.engine.upper(),
        "mood":             args.mood,
        "version":          args.version,
        "engines":          engines_list,
        "bpm":              args.bpm,
        "key":              args.key,
        "tags":             tags_list,
        "program_count":    args.program_count,
        "collection":       args.collection,
        "felix_oscar_bias": args.felix_oscar_bias,
    }
    if args.description is not None:
        pack_config["description"] = args.description

    expansion_str, bundle_str = generate_manifests(pack_config)

    if args.dry_run:
        print("=== expansion.json ===")
        print(expansion_str)
        print("\n=== bundle_manifest.json ===")
        print(bundle_str)
        return

    output_dir = os.path.expanduser(args.output)
    os.makedirs(output_dir, exist_ok=True)

    expansion_path = os.path.join(output_dir, "expansion.json")
    bundle_path    = os.path.join(output_dir, "bundle_manifest.json")

    with open(expansion_path, "w", encoding="utf-8") as f:
        f.write(expansion_str + "\n")

    with open(bundle_path, "w", encoding="utf-8") as f:
        f.write(bundle_str + "\n")

    print(f"Written: {expansion_path}")
    print(f"Written: {bundle_path}")


if __name__ == "__main__":
    main()
