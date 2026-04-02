#!/usr/bin/env python3
"""
xpn_engine_intro_preset_builder.py

Generates placeholder .xometa intro presets for V1 concept engines that have
zero source code yet. Presets describe what the engine WILL do, enabling the
preset system to be ready the moment DSP builds are complete.

Targets: OSTINATO, OPENSKY, OCEANDEEP, OUIE

Usage:
    python xpn_engine_intro_preset_builder.py --engine OSTINATO
    python xpn_engine_intro_preset_builder.py --engine OPENSKY --count 10
    python xpn_engine_intro_preset_builder.py --engine OCEANDEEP --output-dir Presets/XOceanus/
    python xpn_engine_intro_preset_builder.py --engine OUIE --dry-run
"""

import argparse
import json
import random
from pathlib import Path


# ---------------------------------------------------------------------------
# Engine identity catalogue
# ---------------------------------------------------------------------------

ENGINES = {
    "OSTINATO": {
        "engine_id": "Ostinato",
        "prefix": "ostinato_",
        "accent": "#E8701A",
        "description": "Communal drum circle engine — polyrhythmic interlocking voices, firelight energy",
        "macros": ["CIRCLE", "PULSE", "HEAT", "SCATTER"],
        "tags_base": ["rhythmic", "drum-adjacent", "polyrhythmic", "percussive", "communal"],
        "dna_profile": {
            "brightness": (0.45, 0.75),
            "warmth": (0.55, 0.85),
            "movement": (0.70, 0.95),
            "density": (0.60, 0.90),
            "space": (0.25, 0.55),
            "aggression": (0.40, 0.75),
        },
        "mood_weights": {
            "Foundation": 3,
            "Flux": 3,
            "Prism": 2,
            "Atmosphere": 1,
            "Aether": 0,
            "Family": 1,
        },
        "params": [
            "circleSize", "voiceCount", "rhythmDensity", "pulseRate",
            "heatAmount", "timbreBlend", "swingAmount", "accentDepth",
            "attackSharp", "decayTail", "reverbMix", "delayMix",
            "macro1", "macro2", "macro3", "macro4",
        ],
        "preset_names": [
            "Firelight Circle", "Ember Groove", "Village Pulse", "Open Flame",
            "Drumline Echo", "Ash & Beat", "Ritual Ground", "Cinder Drive",
            "Hot Coals", "Circle Break", "Warm Downbeat", "Stoke the Fire",
            "Late Night Circle", "Communal Heat", "Pulse of Many",
        ],
        "descriptions": [
            "Interlocking drum voices orbit a shared pulse. Pure communal heat.",
            "Polyrhythmic layers that breathe together like a circle of players.",
            "Firelight percussion — warm attack transients over a dense groove bed.",
            "High movement density with that irreplaceable human-circle timing feel.",
            "Multiple rhythmic strata locked in conversation around a central beat.",
        ],
    },
    "OPENSKY": {
        "engine_id": "OpenSky",
        "prefix": "sky_",
        "accent": "#FF8C00",
        "description": "Euphoric shimmer engine — pure feliX polarity, expansive brightness and space",
        "macros": ["LIFT", "SHIMMER", "HORIZON", "BLOOM"],
        "tags_base": ["bright", "expansive", "shimmer", "euphoric", "feliX", "atmospheric"],
        "dna_profile": {
            "brightness": (0.75, 0.98),
            "warmth": (0.40, 0.65),
            "movement": (0.30, 0.60),
            "density": (0.20, 0.50),
            "space": (0.75, 0.98),
            "aggression": (0.05, 0.25),
        },
        "mood_weights": {
            "Foundation": 1,
            "Flux": 2,
            "Prism": 2,
            "Atmosphere": 3,
            "Aether": 1,
            "Family": 1,
        },
        "params": [
            "liftAmount", "shimmerRate", "horizonBlend", "bloomDepth",
            "filterCutoff", "filterReso", "attack", "decay",
            "sustain", "release", "chorusRate", "chorusMix",
            "macro1", "macro2", "macro3", "macro4",
        ],
        "preset_names": [
            "Open Sky", "Sunburst Pad", "Horizon Line", "First Light",
            "Clear Ascent", "Bright Bloom", "Euphoric Wash", "Lifted",
            "Solar Wing", "Cerulean Drift", "Upper Air", "Golden Hour",
            "Morning Shimmer", "Altitude", "Radiant Chord",
        ],
        "descriptions": [
            "Pure feliX energy — bright, expansive, and relentlessly uplifting.",
            "Euphoric shimmer pad with vast horizontal spread and minimal aggression.",
            "High brightness, maximum space. The sound of open sky at altitude.",
            "Sunburst harmonics cascade across a wide stereo field.",
            "Lifted, bright textures with long tail and gentle movement.",
        ],
    },
    "OCEANDEEP": {
        "engine_id": "OceanDeep",
        "prefix": "deep_",
        "accent": "#2D0A4E",
        "description": "Abyssal bass engine — pure Oscar polarity, dark warmth and dense low-end pressure",
        "macros": ["DEPTH", "PRESSURE", "CURRENT", "ABYSSAL"],
        "tags_base": ["bass", "dark", "abyssal", "warm", "dense", "oscar", "sub"],
        "dna_profile": {
            "brightness": (0.05, 0.30),
            "warmth": (0.75, 0.98),
            "movement": (0.15, 0.45),
            "density": (0.65, 0.92),
            "space": (0.10, 0.35),
            "aggression": (0.20, 0.50),
        },
        "mood_weights": {
            "Foundation": 3,
            "Flux": 1,
            "Prism": 1,
            "Atmosphere": 2,
            "Aether": 2,
            "Family": 1,
        },
        "params": [
            "depthLevel", "pressureAmount", "currentFlow", "abyssalDark",
            "filterCutoff", "filterReso", "subOscLevel", "driveAmount",
            "attack", "decay", "sustain", "release",
            "macro1", "macro2", "macro3", "macro4",
        ],
        "preset_names": [
            "Abyssal Drift", "Trench Floor", "Deep Pressure", "Dark Current",
            "Sub Thermal", "Benthic Low", "Black Water", "Midnight Zone",
            "Pressure Ridge", "Cold Seep", "Hadal Pulse", "Deep Oscar",
            "Thermal Vent", "Abyssal Plain", "Fathom Bass",
        ],
        "descriptions": [
            "Pure Oscar energy at maximum depth — warm, dark, and inexorably heavy.",
            "Abyssal bass textures with sub-oceanic pressure and minimal brightness.",
            "Low-end density that fills the room from the floor up.",
            "Dark thermal currents at 11,000 metres — slow movement, immense warmth.",
            "Dense sub-bass with that unmistakable deep-ocean pressure character.",
        ],
    },
    "OUIE": {
        "engine_id": "Ouie",
        "prefix": "ouie_",
        "accent": "#708090",
        "description": "Duophonic hammerhead engine — STRIFE↔LOVE axis, dynamic tension and release",
        "macros": ["STRIFE", "LOVE", "TENSION", "RESOLVE"],
        "tags_base": ["duophonic", "dynamic", "tension", "hammerhead", "expressive", "bipolar"],
        "dna_profile": {
            "brightness": (0.35, 0.65),
            "warmth": (0.35, 0.65),
            "movement": (0.55, 0.85),
            "density": (0.40, 0.65),
            "space": (0.35, 0.65),
            "aggression": (0.30, 0.65),
        },
        "mood_weights": {
            "Foundation": 2,
            "Flux": 3,
            "Prism": 2,
            "Atmosphere": 1,
            "Aether": 1,
            "Family": 1,
        },
        "params": [
            "strifeAmount", "loveAmount", "tensionAxis", "resolveDepth",
            "voiceA_cutoff", "voiceB_cutoff", "voiceA_level", "voiceB_level",
            "interactionAmount", "crossMod", "attack", "release",
            "macro1", "macro2", "macro3", "macro4",
        ],
        "preset_names": [
            "Strife & Love", "Hammerhead", "Duophonic Drift", "Two Voices",
            "Tension Arc", "Resolve Point", "Push & Pull", "Steel Grace",
            "Opposing Tides", "STRIFE Lead", "LOVE Pad", "Axis Shift",
            "Hammerhead Steel", "Bipolar Dance", "Twin Currents",
        ],
        "descriptions": [
            "Two voices in permanent negotiation — STRIFE and LOVE as synthesis poles.",
            "Duophonic hammerhead character with dynamic STRIFE↔LOVE axis.",
            "High movement energy from two independent voices in expressive dialogue.",
            "The tension between opposing timbres creates a third, emergent character.",
            "STRIFE voice cuts; LOVE voice sustains. Together: something completely new.",
        ],
    },
}

# 7 moods — Entangled skipped (concept engines cannot couple yet)
MOODS = ["Foundation", "Atmosphere", "Prism", "Flux", "Aether", "Family"]
# Entangled deliberately excluded per spec

PLACEHOLDER_TAG = "placeholder"
AUTHOR = "XO_OX"
VERSION = "1.0.0"
SCHEMA_VERSION = 1
CREATED_DATE = "2026-03-16"


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def rand_dna(profile: dict) -> dict:
    """Generate a random Sonic DNA within the engine's profile bounds."""
    return {k: round(random.uniform(lo, hi), 3) for k, (lo, hi) in profile.items()}


def rand_params(prefix: str, param_names: list[str]) -> dict:
    """Generate placeholder parameter values (all 0.5 base, slight jitter for variety)."""
    params = {}
    for name in param_names:
        # macro params stay at 0.5; others get mild jitter so presets differ visually
        if name.startswith("macro"):
            params[prefix + name] = 0.5
        else:
            params[prefix + name] = round(0.5 + random.uniform(-0.1, 0.1), 3)
    return params


def weighted_mood_pick(mood_weights: dict, exclude_used: list[str]) -> str:
    """Pick a mood with weighted probability, preferring unused ones."""
    available = [m for m in MOODS if m not in exclude_used]
    if not available:
        available = MOODS[:]
    weights = [mood_weights.get(m, 1) for m in available]
    total = sum(weights)
    if total == 0:
        return random.choice(available)
    r = random.uniform(0, total)
    cumulative = 0
    for mood, w in zip(available, weights):
        cumulative += w
        if r <= cumulative:
            return mood
    return available[-1]


def mood_tags(mood: str) -> list[str]:
    mood_map = {
        "Foundation": ["foundation", "core"],
        "Atmosphere": ["atmospheric", "ambient"],
        "Prism": ["prismatic", "colorful"],
        "Flux": ["flux", "evolving"],
        "Aether": ["aether", "ethereal"],
        "Family": ["family", "warm"],
    }
    return mood_map.get(mood, [])


def build_preset(
    engine_key: str,
    name: str,
    mood: str,
    description: str,
    seed: int,
) -> dict:
    random.seed(seed)
    cfg = ENGINES[engine_key]
    dna = rand_dna(cfg["dna_profile"])
    params = rand_params(cfg["prefix"], cfg["params"])
    tags = cfg["tags_base"] + mood_tags(mood) + [PLACEHOLDER_TAG]

    return {
        "schema_version": SCHEMA_VERSION,
        "name": name,
        "mood": mood,
        "engines": [cfg["engine_id"]],
        "author": AUTHOR,
        "version": VERSION,
        "description": description,
        "tags": sorted(set(tags)),
        "macroLabels": cfg["macros"],
        "couplingIntensity": None,
        "tempo": None,
        "created": CREATED_DATE,
        "legacy": {
            "sourceInstrument": cfg["engine_id"],
            "sourceCategory": mood,
            "sourcePresetName": None,
        },
        "parameters": {
            cfg["engine_id"]: params,
        },
        "coupling": None,
        "sequencer": None,
        "dna": dna,
    }


def generate_presets(engine_key: str, count: int) -> list[dict]:
    cfg = ENGINES[engine_key]
    names = cfg["preset_names"]
    descriptions = cfg["descriptions"]

    # Ensure we have enough unique names (cycle if count > len)
    chosen_names: list[str] = []
    pool = names[:]
    random.seed(42)
    random.shuffle(pool)
    while len(chosen_names) < count:
        chosen_names.extend(pool)
    chosen_names = chosen_names[:count]

    used_moods: list[str] = []
    presets = []

    for i, name in enumerate(chosen_names):
        mood = weighted_mood_pick(cfg["mood_weights"], used_moods)
        used_moods.append(mood)
        desc = descriptions[i % len(descriptions)]
        preset = build_preset(engine_key, name, mood, desc, seed=1000 + i)
        presets.append(preset)

    return presets


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Generate placeholder .xometa intro presets for V1 concept engines.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Engines:
  OSTINATO   Communal drum circle — Firelight Orange
  OPENSKY    Euphoric shimmer (pure feliX) — Sunburst
  OCEANDEEP  Abyssal bass (pure Oscar) — Trench Violet
  OUIE       Duophonic hammerhead (STRIFE<->LOVE) — Hammerhead Steel

Examples:
  python xpn_engine_intro_preset_builder.py --engine OSTINATO
  python xpn_engine_intro_preset_builder.py --engine OPENSKY --count 15
  python xpn_engine_intro_preset_builder.py --engine OUIE --output-dir Presets/XOceanus/ --dry-run
        """,
    )
    parser.add_argument(
        "--engine",
        required=True,
        choices=list(ENGINES.keys()),
        metavar="ENGINE",
        help="Engine to generate presets for: OSTINATO, OPENSKY, OCEANDEEP, OUIE",
    )
    parser.add_argument(
        "--count",
        type=int,
        default=10,
        help="Number of presets to generate (default: 10)",
    )
    parser.add_argument(
        "--output-dir",
        type=str,
        default=None,
        help=(
            "Root preset directory. Presets land in <output-dir>/<Mood>/<name>.xometa. "
            "Defaults to current working directory."
        ),
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Print presets to stdout instead of writing files.",
    )
    return parser.parse_args()


def safe_filename(name: str) -> str:
    """Convert preset name to a safe filename."""
    return name.replace("/", "-").replace("\\", "-").replace(":", "").strip()


def main() -> None:
    args = parse_args()

    engine_key = args.engine
    count = max(1, args.count)
    cfg = ENGINES[engine_key]

    print(f"Engine : {engine_key} ({cfg['engine_id']}) — {cfg['accent']}")
    print(f"Presets: {count}")
    print(f"Mode   : {'dry-run' if args.dry_run else 'write'}")
    print()

    presets = generate_presets(engine_key, count)

    if args.dry_run:
        for p in presets:
            print(f"--- {p['name']} [{p['mood']}] ---")
            print(json.dumps(p, indent=2))
            print()
        print(f"Total: {len(presets)} preset(s) (dry run — no files written)")
        return

    # Determine output root
    if args.output_dir:
        output_root = Path(args.output_dir)
    else:
        output_root = Path.cwd()

    written = 0
    skipped = 0

    for preset in presets:
        mood_dir = output_root / preset["mood"]
        mood_dir.mkdir(parents=True, exist_ok=True)
        filename = safe_filename(preset["name"]) + ".xometa"
        filepath = mood_dir / filename

        if filepath.exists():
            print(f"  SKIP (exists) : {filepath}")
            skipped += 1
            continue

        with open(filepath, "w", encoding="utf-8") as f:
            json.dump(preset, f, indent=2)
            f.write("\n")

        print(f"  WROTE [{preset['mood']:12s}] : {filepath.name}")
        written += 1

    print()
    print(f"Done. {written} written, {skipped} skipped.")
    if written:
        print(f"Tag '{PLACEHOLDER_TAG}' identifies all pre-DSP presets for later refinement.")


if __name__ == "__main__":
    main()
