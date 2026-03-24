#!/usr/bin/env python3
"""
xpn_ombre_orca_octopus_presets.py

Generate single-engine preset stubs for OMBRE, ORCA, and OCTOPUS engines,
seeding solo preset coverage across all 7 moods (Entangled excluded — handled
by coupling tools).

Usage:
    python Tools/xpn_ombre_orca_octopus_presets.py
    python Tools/xpn_ombre_orca_octopus_presets.py --engine ORCA --count 6
    python Tools/xpn_ombre_orca_octopus_presets.py --dry-run
    python Tools/xpn_ombre_orca_octopus_presets.py --seed 42 --output-dir /tmp/presets
"""

import argparse
import json
import os
import random
import sys
from pathlib import Path

# ---------------------------------------------------------------------------
# Engine DNA definitions
# ---------------------------------------------------------------------------

ENGINES = {
    "OMBRE": {
        "prefix": "ombre_",
        "accent": "#7B6B8A",
        "engine_key": "XOmbre",
        "description": "Dual-narrative memory/forgetting engine — shadow and light in gradient motion",
        "vocabulary": ["Shadow", "Memory", "Gradient", "Dusk", "Recall", "Fade", "Liminal", "Twilight"],
        "macro_labels": ["MEMORY", "SHADOW", "GRADIENT", "DISSOLVE"],
        "tags_base": ["ombre", "shadow", "gradient", "dual"],
        "moods": {
            "Foundation":  {"warmth": 0.6,  "density": 0.65, "brightness": 0.35, "movement": 0.3,  "aggression": 0.2,  "space": 0.45},
            "Atmosphere":  {"warmth": 0.7,  "space": 0.7,   "brightness": 0.4,  "movement": 0.35, "density": 0.45,    "aggression": 0.15},
            "Prism":       {"brightness": 0.6, "movement": 0.5, "warmth": 0.5,  "density": 0.5,   "space": 0.45,      "aggression": 0.3},
            "Flux":        {"movement": 0.65, "aggression": 0.4, "brightness": 0.45, "density": 0.55, "warmth": 0.4,  "space": 0.4},
            "Aether":      {"space": 0.7,   "brightness": 0.4, "warmth": 0.55,  "movement": 0.3,  "density": 0.35,    "aggression": 0.1},
            "Family":      {"warmth": 0.65, "movement": 0.5, "brightness": 0.5,  "density": 0.5,   "space": 0.5,      "aggression": 0.2},
        },
        "param_defaults": {
            "ombre_memoryDecay":    0.55,
            "ombre_shadowDepth":    0.6,
            "ombre_gradientPos":    0.5,
            "ombre_blendMode":      0,
            "ombre_filterCutoff":   0.6,
            "ombre_filterReso":     0.2,
            "ombre_filterEnvAmt":   0.35,
            "ombre_ampAttack":      0.25,
            "ombre_ampDecay":       0.4,
            "ombre_ampSustain":     0.7,
            "ombre_ampRelease":     0.5,
            "ombre_lfoRate":        0.3,
            "ombre_lfoDepth":       0.25,
            "ombre_reverbSize":     0.5,
            "ombre_reverbMix":      0.3,
            "ombre_delayTime":      0.4,
            "ombre_delayFeedback":  0.35,
            "ombre_delayMix":       0.2,
            "ombre_memory":         0.5,
            "ombre_shadow":         0.5,
            "ombre_gradient":       0.5,
            "ombre_dissolve":       0.3,
            "ombre_outputLevel":    0.85,
            "ombre_outputPan":      0.0,
            "ombre_couplingLevel":  0.5,
            "ombre_couplingBus":    0,
        },
    },

    "ORCA": {
        "prefix": "orca_",
        "accent": "#1B2838",
        "engine_key": "XOrca",
        "description": "Apex predator wavetable + echolocation synthesis — deep ocean hunting intelligence",
        "vocabulary": ["Hunt", "Breach", "Echo", "Surge", "Sonar", "Apex", "Deep", "Current"],
        "macro_labels": ["HUNT", "BREACH", "SONAR", "CURRENT"],
        "tags_base": ["orca", "wavetable", "echolocation", "predator"],
        "moods": {
            "Foundation":  {"density": 0.75, "aggression": 0.6,  "brightness": 0.3,  "movement": 0.35, "warmth": 0.3,  "space": 0.5},
            "Atmosphere":  {"movement": 0.6, "space": 0.6,       "brightness": 0.4,  "density": 0.5,   "warmth": 0.35, "aggression": 0.25},
            "Prism":       {"brightness": 0.55, "movement": 0.7, "density": 0.55,    "warmth": 0.35,   "space": 0.45,  "aggression": 0.45},
            "Flux":        {"aggression": 0.8, "movement": 0.75, "brightness": 0.5,  "density": 0.65,  "warmth": 0.25, "space": 0.35},
            "Aether":      {"space": 0.65,  "density": 0.6,      "brightness": 0.35, "movement": 0.4,  "warmth": 0.3,  "aggression": 0.2},
            "Family":      {"warmth": 0.45, "movement": 0.55,    "brightness": 0.45, "density": 0.55,  "space": 0.5,   "aggression": 0.35},
        },
        "param_defaults": {
            "orca_wavetablePos":    0.5,
            "orca_wavetableMorph":  0.3,
            "orca_echoDepth":       0.6,
            "orca_echoRate":        0.4,
            "orca_filterCutoff":    0.55,
            "orca_filterReso":      0.25,
            "orca_filterTrack":     0.5,
            "orca_ampAttack":       0.15,
            "orca_ampDecay":        0.35,
            "orca_ampSustain":      0.65,
            "orca_ampRelease":      0.45,
            "orca_lfoRate":         0.4,
            "orca_lfoDepth":        0.3,
            "orca_driveAmount":     0.35,
            "orca_reverbSize":      0.55,
            "orca_reverbMix":       0.25,
            "orca_delayTime":       0.38,
            "orca_delayFeedback":   0.4,
            "orca_delayMix":        0.25,
            "orca_hunt":            0.5,
            "orca_breach":          0.4,
            "orca_sonar":           0.5,
            "orca_current":         0.45,
            "orca_outputLevel":     0.85,
            "orca_outputPan":       0.0,
            "orca_couplingLevel":   0.5,
            "orca_couplingBus":     0,
        },
    },

    "OCTOPUS": {
        "prefix": "octo_",
        "accent": "#E040FB",
        "engine_key": "XOctopus",
        "description": "Decentralized alien intelligence — 8-arm chromatophore synthesis with distributed processing",
        "vocabulary": ["Ink", "Chromo", "Shift", "Arm", "Signal", "Adapt", "Flash", "Pattern"],
        "macro_labels": ["INK", "CHROMO", "ADAPT", "SIGNAL"],
        "tags_base": ["octopus", "chromatophore", "alien", "adaptive"],
        "moods": {
            "Foundation":  {"density": 0.65, "warmth": 0.45,    "brightness": 0.4,  "movement": 0.4,  "space": 0.45,  "aggression": 0.3},
            "Atmosphere":  {"movement": 0.7, "space": 0.6,      "brightness": 0.45, "density": 0.5,   "warmth": 0.4,  "aggression": 0.2},
            "Prism":       {"brightness": 0.75, "movement": 0.8,"density": 0.55,    "warmth": 0.4,    "space": 0.5,   "aggression": 0.4},
            "Flux":        {"movement": 0.85, "aggression": 0.55,"brightness": 0.55,"density": 0.6,   "warmth": 0.35, "space": 0.4},
            "Aether":      {"space": 0.6,   "brightness": 0.6,  "movement": 0.45,   "density": 0.4,   "warmth": 0.4,  "aggression": 0.15},
            "Family":      {"warmth": 0.5,  "movement": 0.65,   "brightness": 0.5,  "density": 0.5,   "space": 0.5,   "aggression": 0.25},
        },
        "param_defaults": {
            "octo_armCount":        8,
            "octo_armSpread":       0.5,
            "octo_chromoRate":      0.4,
            "octo_chromoDepth":     0.6,
            "octo_inkDensity":      0.5,
            "octo_shiftSpeed":      0.45,
            "octo_filterCutoff":    0.6,
            "octo_filterReso":      0.2,
            "octo_filterEnvAmt":    0.4,
            "octo_ampAttack":       0.1,
            "octo_ampDecay":        0.3,
            "octo_ampSustain":      0.7,
            "octo_ampRelease":      0.4,
            "octo_lfoRate":         0.5,
            "octo_lfoDepth":        0.35,
            "octo_reverbSize":      0.5,
            "octo_reverbMix":       0.25,
            "octo_delayTime":       0.35,
            "octo_delayFeedback":   0.3,
            "octo_delayMix":        0.2,
            "octo_ink":             0.5,
            "octo_chromo":          0.5,
            "octo_adapt":           0.5,
            "octo_signal":          0.45,
            "octo_outputLevel":     0.85,
            "octo_outputPan":       0.0,
            "octo_couplingLevel":   0.5,
            "octo_couplingBus":     0,
        },
    },
}

MOODS = ["Foundation", "Atmosphere", "Prism", "Flux", "Aether", "Family"]

MOOD_TAGS = {
    "Foundation": ["grounded", "stable", "bass", "root"],
    "Atmosphere": ["ambient", "pad", "space", "air"],
    "Prism":      ["bright", "spectral", "shimmer", "color"],
    "Flux":       ["motion", "dynamic", "evolving", "drive"],
    "Aether":     ["ethereal", "sparse", "subtle", "float"],
    "Family":     ["harmonic", "warm", "blend", "ensemble"],
}


# ---------------------------------------------------------------------------
# Preset generation helpers
# ---------------------------------------------------------------------------

def _jitter(val: float, amount: float = 0.08, rng: random.Random = None) -> float:
    """Apply small random variation to a DNA value while clamping to [0, 1]."""
    if rng is None:
        rng = random
    delta = rng.uniform(-amount, amount)
    return round(max(0.0, min(1.0, val + delta)), 3)


def _build_preset_name(vocab: list, mood: str, index: int, rng: random.Random) -> str:
    """Build a unique-ish preset name from vocabulary and mood context."""
    word = vocab[rng.randint(0, len(vocab) - 1)]
    mood_word = mood  # e.g. "Foundation", "Flux"
    suffixes = ["I", "II", "III", "IV", "V", "VI", "VII", "VIII"]
    suffix = suffixes[index % len(suffixes)]
    # Alternate between "Word Mood" and "Mood Word suffix" styles
    if index % 3 == 0:
        return f"{word} {mood_word}"
    elif index % 3 == 1:
        return f"{mood_word} {word}"
    else:
        return f"{word} {suffix}"


def _mutate_params(defaults: dict, dna: dict, prefix: str, rng: random.Random) -> dict:
    """Return a copy of param defaults with DNA-informed mutations."""
    params = dict(defaults)

    # Map DNA axes to parameter families and nudge them
    brightness = dna.get("brightness", 0.5)
    warmth     = dna.get("warmth",     0.5)
    aggression = dna.get("aggression", 0.3)
    movement   = dna.get("movement",   0.4)
    density    = dna.get("density",    0.5)
    space      = dna.get("space",      0.5)

    cutoff_key = f"{prefix}filterCutoff"
    if cutoff_key in params:
        params[cutoff_key] = round(
            max(0.1, min(1.0, brightness * 0.6 + aggression * 0.4 + rng.uniform(-0.05, 0.05))), 3
        )

    reso_key = f"{prefix}filterReso"
    if reso_key in params:
        params[reso_key] = round(
            max(0.0, min(0.9, aggression * 0.5 + rng.uniform(-0.05, 0.05))), 3
        )

    reverb_mix_key = f"{prefix}reverbMix"
    if reverb_mix_key in params:
        params[reverb_mix_key] = round(
            max(0.0, min(0.85, space * 0.55 + rng.uniform(-0.05, 0.05))), 3
        )

    reverb_size_key = f"{prefix}reverbSize"
    if reverb_size_key in params:
        params[reverb_size_key] = round(
            max(0.1, min(1.0, space * 0.7 + rng.uniform(-0.05, 0.05))), 3
        )

    lfo_depth_key = f"{prefix}lfoDepth"
    if lfo_depth_key in params:
        params[lfo_depth_key] = round(
            max(0.0, min(1.0, movement * 0.55 + rng.uniform(-0.06, 0.06))), 3
        )

    lfo_rate_key = f"{prefix}lfoRate"
    if lfo_rate_key in params:
        params[lfo_rate_key] = round(
            max(0.05, min(1.0, movement * 0.5 + rng.uniform(-0.06, 0.06))), 3
        )

    drive_key = f"{prefix}driveAmount"
    if drive_key in params:
        params[drive_key] = round(
            max(0.0, min(1.0, aggression * 0.7 + rng.uniform(-0.06, 0.06))), 3
        )

    # Envelope adjustments based on movement + space
    attack_key = f"{prefix}ampAttack"
    if attack_key in params:
        params[attack_key] = round(
            max(0.01, min(1.0, (1.0 - movement) * 0.4 + rng.uniform(-0.04, 0.04))), 3
        )

    release_key = f"{prefix}ampRelease"
    if release_key in params:
        params[release_key] = round(
            max(0.1, min(1.0, space * 0.6 + rng.uniform(-0.04, 0.04))), 3
        )

    sustain_key = f"{prefix}ampSustain"
    if sustain_key in params:
        params[sustain_key] = round(
            max(0.3, min(1.0, (warmth * 0.4 + density * 0.3 + 0.3) + rng.uniform(-0.04, 0.04))), 3
        )

    # Engine-specific DNA-linked params
    for dna_key, dna_val in [
        ("memory", warmth),
        ("shadow", 1.0 - brightness),
        ("gradient", movement),
        ("dissolve", space * 0.8),
        ("hunt", aggression),
        ("breach", movement * 0.8),
        ("sonar", density * 0.7),
        ("current", movement * 0.6),
        ("ink", density),
        ("chromo", brightness * 0.9),
        ("adapt", movement * 0.7),
        ("signal", density * 0.6),
    ]:
        param_key = f"{prefix}{dna_key}"
        if param_key in params:
            params[param_key] = round(
                max(0.0, min(1.0, dna_val + rng.uniform(-0.06, 0.06))), 3
            )

    return params


def build_preset(engine_name: str, mood: str, index: int, rng: random.Random) -> dict:
    """Build a single preset dict."""
    eng = ENGINES[engine_name]
    dna_base = eng["moods"][mood]

    # Jitter each DNA axis slightly for variety
    dna = {k: _jitter(v, amount=0.06, rng=rng) for k, v in dna_base.items()}

    name = _build_preset_name(eng["vocabulary"], mood, index, rng)

    tags = list(eng["tags_base"]) + MOOD_TAGS.get(mood, [])
    # Add a random vocabulary word as a tag
    extra_tag = rng.choice(eng["vocabulary"]).lower()
    if extra_tag not in tags:
        tags.append(extra_tag)

    prefix = eng["prefix"]
    parameters = _mutate_params(eng["param_defaults"], dna, prefix, rng)

    engine_key = eng["engine_key"]

    preset = {
        "schema_version": 1,
        "name": name,
        "mood": mood,
        "sonic_dna": {
            "brightness": dna["brightness"],
            "warmth":     dna["warmth"],
            "aggression": dna["aggression"],
            "movement":   dna["movement"],
            "density":    dna["density"],
            "space":      dna["space"],
        },
        "engines": [engine_key],
        "author": "XO_OX",
        "version": "1.0.0",
        "description": f"{eng['description']} — {mood} expression",
        "tags": tags,
        "macroLabels": eng["macro_labels"],
        "couplingIntensity": "None",
        "dna": {
            "brightness": dna["brightness"],
            "warmth":     dna["warmth"],
            "movement":   dna["movement"],
            "density":    dna["density"],
            "space":      dna["space"],
            "aggression": dna["aggression"],
        },
        "parameters": {
            engine_key: parameters,
        },
        "coupling": {
            "pairs": []
        },
    }
    return preset


def safe_filename(name: str) -> str:
    """Convert a preset name to a safe filename component."""
    return name.replace("/", "-").replace("\\", "-").replace(" ", "_")


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def parse_args():
    parser = argparse.ArgumentParser(
        description="Generate OMBRE / ORCA / OCTOPUS solo preset stubs for XOlokun."
    )
    parser.add_argument(
        "--output-dir",
        default="Presets/XOlokun/",
        help="Root preset directory. Mood subdirs will be created inside. "
             "(default: Presets/XOlokun/)",
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Print what would be written without creating any files.",
    )
    parser.add_argument(
        "--engine",
        choices=["OMBRE", "ORCA", "OCTOPUS", "all"],
        default="all",
        help="Which engine(s) to generate presets for. (default: all)",
    )
    parser.add_argument(
        "--count",
        type=int,
        default=4,
        help="Number of presets per engine per mood. (default: 4)",
    )
    parser.add_argument(
        "--seed",
        type=int,
        default=None,
        help="Random seed for reproducibility.",
    )
    return parser.parse_args()


def main():
    args = parse_args()

    rng = random.Random(args.seed)

    # Resolve output directory relative to cwd or as absolute path
    output_root = Path(args.output_dir)

    engines_to_run = list(ENGINES.keys()) if args.engine == "all" else [args.engine]

    total_written = 0
    total_skipped = 0

    for engine_name in engines_to_run:
        for mood in MOODS:
            mood_dir = output_root / mood
            if not args.dry_run:
                mood_dir.mkdir(parents=True, exist_ok=True)

            for i in range(args.count):
                preset = build_preset(engine_name, mood, i, rng)
                preset_name = preset["name"]
                filename = f"{engine_name.capitalize()}_{safe_filename(preset_name)}.xometa"
                filepath = mood_dir / filename

                if args.dry_run:
                    print(f"[DRY-RUN] Would write: {filepath}")
                    print(f"          Name: {preset_name}  Mood: {mood}  Engine: {engine_name}")
                    total_written += 1
                else:
                    if filepath.exists():
                        print(f"[SKIP] Already exists: {filepath}")
                        total_skipped += 1
                        continue
                    with open(filepath, "w", encoding="utf-8") as f:
                        json.dump(preset, f, indent=2)
                        f.write("\n")
                    print(f"[WRITE] {filepath}")
                    total_written += 1

    action = "Would generate" if args.dry_run else "Generated"
    print(
        f"\n{action} {total_written} preset(s) across "
        f"{len(engines_to_run)} engine(s) × {len(MOODS)} moods × {args.count} per mood."
    )
    if total_skipped:
        print(f"Skipped {total_skipped} already-existing file(s).")

    return 0


if __name__ == "__main__":
    sys.exit(main())
