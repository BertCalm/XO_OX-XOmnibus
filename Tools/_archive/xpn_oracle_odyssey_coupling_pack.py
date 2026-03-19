#!/usr/bin/env python3
"""Generate ORACLE + ODYSSEY coupling pack presets for XOmnibus.

Covers:
  - ORACLE × ODYSSEY marquee pair  (6 presets)
  - ORACLE external pairs × 9 partners  (27 presets, 3 each)
  - ODYSSEY external pairs × 9 partners (27 presets, 3 each)
Total: 60 presets, all mood=Entangled.

Usage:
  python Tools/xpn_oracle_odyssey_coupling_pack.py
  python Tools/xpn_oracle_odyssey_coupling_pack.py --dry-run
  python Tools/xpn_oracle_odyssey_coupling_pack.py --seed 42 --count 2
  python Tools/xpn_oracle_odyssey_coupling_pack.py --output-dir /tmp/presets
"""

import argparse
import json
import os
import random
from pathlib import Path

# ---------------------------------------------------------------------------
# Sonic DNA baselines
# ---------------------------------------------------------------------------

DNA = {
    "ORACLE":  {"brightness": 0.5,  "warmth": 0.4,  "movement": 0.6,  "density": 0.7,  "space": 0.7,  "aggression": 0.4},
    "ODYSSEY": {"brightness": 0.55, "warmth": 0.5,  "movement": 0.7,  "density": 0.5,  "space": 0.7,  "aggression": 0.3},
    "OPTIC":   {"brightness": 0.85, "warmth": 0.3,  "movement": 0.9,  "density": 0.4,  "space": 0.5,  "aggression": 0.45},
    "OBLONG":  {"brightness": 0.6,  "warmth": 0.65, "movement": 0.5,  "density": 0.65, "space": 0.55, "aggression": 0.5},
    "ORBITAL": {"brightness": 0.6,  "warmth": 0.65, "movement": 0.55, "density": 0.6,  "space": 0.6,  "aggression": 0.5},
    "OVERLAP": {"brightness": 0.65, "warmth": 0.55, "movement": 0.6,  "density": 0.7,  "space": 0.85, "aggression": 0.25},
    "OMBRE":   {"brightness": 0.45, "warmth": 0.6,  "movement": 0.55, "density": 0.6,  "space": 0.65, "aggression": 0.3},
    "ORCA":    {"brightness": 0.3,  "warmth": 0.35, "movement": 0.7,  "density": 0.7,  "space": 0.5,  "aggression": 0.75},
    "OCTOPUS": {"brightness": 0.7,  "warmth": 0.4,  "movement": 0.85, "density": 0.6,  "space": 0.5,  "aggression": 0.55},
    "OHM":     {"brightness": 0.45, "warmth": 0.75, "movement": 0.55, "density": 0.6,  "space": 0.7,  "aggression": 0.3},
    "ORPHICA": {"brightness": 0.8,  "warmth": 0.5,  "movement": 0.7,  "density": 0.45, "space": 0.75, "aggression": 0.25},
    "OPAL":    {"brightness": 0.7,  "warmth": 0.5,  "movement": 0.75, "density": 0.45, "space": 0.8,  "aggression": 0.2},
    "ORGANON": {"brightness": 0.6,  "warmth": 0.6,  "movement": 0.65, "density": 0.75, "space": 0.65, "aggression": 0.4},
    "OCEANIC": {"brightness": 0.65, "warmth": 0.55, "movement": 0.5,  "density": 0.55, "space": 0.75, "aggression": 0.3},
    "OBLIQUE": {"brightness": 0.75, "warmth": 0.4,  "movement": 0.7,  "density": 0.5,  "space": 0.6,  "aggression": 0.55},
    "OBESE":   {"brightness": 0.5,  "warmth": 0.7,  "movement": 0.45, "density": 0.8,  "space": 0.4,  "aggression": 0.75},
}

# Engine prefix map
PREFIX = {
    "ORACLE":  "oracle_",
    "ODYSSEY": "drift_",
    "OPTIC":   "optic_",
    "OBLONG":  "bob_",
    "ORBITAL": "orb_",
    "OVERLAP": "olap_",
    "OMBRE":   "ombr_",
    "ORCA":    "orca_",
    "OCTOPUS": "oct_",
    "OHM":     "ohm_",
    "ORPHICA": "orph_",
    "OPAL":    "opal_",
    "ORGANON": "org_",
    "OCEANIC": "ocean_",
    "OBLIQUE": "obl_",
    "OBESE":   "ob_",
}

# ---------------------------------------------------------------------------
# Vocabulary pools
# ---------------------------------------------------------------------------

ORACLE_VOCAB = [
    "Prophecy", "Breakpoint", "Stochastic", "Maqam", "Vision",
    "Divination", "GENDY", "Indigo", "Oracle", "Omen", "Augury",
    "Foretelling", "Revelation", "Sibyl", "Augur",
]

ODYSSEY_VOCAB = [
    "Drift", "Voyage", "Journey", "Float", "Wander",
    "Drift State", "Migrate", "Passage", "Odyssey", "Traverse",
    "Current", "Expedition", "Sojourn", "Roam",
]

PARTNER_VOCAB = {
    "OPTIC":   ["Optic", "Lens", "Refraction", "Beam", "Spectrum"],
    "OBLONG":  ["Bob", "Oblong", "Curious", "Bob's", "Wandering"],
    "ORBITAL": ["Orbital", "Orbit", "Harmonic", "Resonant", "Circling"],
    "OVERLAP": ["Overlap", "Layered", "Braided", "Interleaved", "Nested"],
    "OMBRE":   ["Ombre", "Gradient", "Fade", "Shadow", "Haze"],
    "ORCA":    ["Orca", "Apex", "Pod", "Hunter", "Deep Strike"],
    "OCTOPUS": ["Octopus", "Tentacle", "Eight-Armed", "Adaptive", "Fluid"],
    "OHM":     ["Ohm", "Commune", "Drone", "Resonant", "Zen"],
    "ORPHICA": ["Orphica", "Harp", "Lyre", "Microsound", "Plucked"],
    "OPAL":    ["Opal", "Granular", "Iridescent", "Shimmer", "Cloud"],
    "ORGANON": ["Organon", "Logic", "System", "Modular", "Ordered"],
    "OCEANIC": ["Oceanic", "Deep", "Tidal", "Abyssal", "Pelagic"],
    "OBLIQUE": ["Oblique", "Angle", "Slant", "Skewed", "Off-Axis"],
    "OBESE":   ["Obese", "Massive", "Heavy", "Dense", "Fat"],
}

COUPLING_TYPES = [
    "Env->Filter",
    "Env->Morph",
    "Amp->Filter",
    "LFO->Pitch",
    "Rhythm->Blend",
    "Spectral->Amp",
    "Stochastic->Morph",
    "Pitch->Detune",
    "Tidal->LFO",
    "Audio->Wavetable",
]

MACRO_SETS = {
    "ORACLE":  ["PROPHECY", "STOCHASTIC", "MAQAM", "INDIGO"],
    "ODYSSEY": ["DRIFT", "TIDAL", "VOYAGE", "SPACE"],
    "OPTIC":   ["BEAM", "SPECTRUM", "VELOCITY", "REFRACTION"],
    "OBLONG":  ["CHARACTER", "CURIOSITY", "WARMTH", "BOB"],
    "ORBITAL": ["ORBIT", "RESONANCE", "HARMONIC", "GRAVITY"],
    "OVERLAP": ["DEPTH", "LAYER", "BRAID", "SPACE"],
    "OMBRE":   ["FADE", "GRADIENT", "SHADOW", "DEPTH"],
    "ORCA":    ["POWER", "HUNT", "POD", "STRIKE"],
    "OCTOPUS": ["ARMS", "ADAPT", "FLUID", "MORPH"],
    "OHM":     ["COMMUNE", "DRONE", "WARMTH", "RESONANCE"],
    "ORPHICA": ["PLUCK", "MICRO", "LYRE", "SHIMMER"],
    "OPAL":    ["GRAIN", "CLOUD", "SHIMMER", "SPACE"],
    "ORGANON": ["LOGIC", "SYSTEM", "ORDER", "MODULAR"],
    "OCEANIC": ["TIDE", "DEPTH", "CURRENT", "ABYSS"],
    "OBLIQUE": ["ANGLE", "SLANT", "TENSION", "SKEW"],
    "OBESE":   ["MASS", "FAT", "DRIVE", "DENSITY"],
}

DESCRIPTIONS_ORACLE_EXTERNAL = {
    "OPTIC":   [
        "ORACLE stochastic breakpoints modulate OPTIC beam refraction. Prophecy illuminated.",
        "GENDY noise seeds OPTIC spectral velocity. Divination through light.",
        "Maqam microtonal shifts carve OPTIC lens angles. Sacred geometry of color.",
    ],
    "OBLONG":  [
        "ORACLE Indigo probability gates meet Bob's curious warmth. Oracular introspection.",
        "Stochastic breakpoints modulate Bob's character morphology. Augury of personality.",
        "Maqam scales guide Oblong's harmonic wandering. Ancient modes, fresh curiosity.",
    ],
    "ORBITAL": [
        "ORACLE prophecy feeds ORBITAL resonance cycles. Orbits foretold.",
        "GENDY stochastic texture modulates ORBITAL harmonic partials. Divination in harmonics.",
        "Indigo envelope shapes ORBITAL gravity well depth. Prophetic pull.",
    ],
    "OVERLAP": [
        "ORACLE stochastic events layer into OVERLAP's FDN braids. Prophecy in knots.",
        "GENDY breakpoints scatter OVERLAP's nested reflections. Tangled futures.",
        "Maqam Indigo tones weave through OVERLAP's spatial web. Sacred entanglement.",
    ],
    "OMBRE":   [
        "ORACLE vision fades through OMBRE gradient shadow. Prophecy dissolving.",
        "Stochastic oracle gates shift OMBRE's colour depth. Fading augury.",
        "Maqam oracle modulates OMBRE haze density. Ancient tones, modern shadow.",
    ],
    "ORCA":    [
        "ORACLE Indigo prophecy unleashed through ORCA apex aggression. Vision becomes hunt.",
        "GENDY noise drives ORCA pod strike timing. Stochastic predation.",
        "Maqam oracle breakpoints detune ORCA's deep resonance. Prophetic power.",
    ],
    "OCTOPUS": [
        "ORACLE stochastic branches feed OCTOPUS eight-arm CA. Prophecy through adaptation.",
        "GENDY texture modulates OCTOPUS tentacle spread. Divination in fluid motion.",
        "Maqam oracle pitches route through OCTOPUS morphology. Sacred arms.",
    ],
    "OHM":     [
        "ORACLE Indigo prophecy resonates through OHM commune drone. Divination in stillness.",
        "Stochastic breakpoints flutter OHM warmth field. GENDY meets commune.",
        "Maqam oracle tones sustain OHM's resonant hum. Ancient prayer, new frequency.",
    ],
    "ORPHICA": [
        "ORACLE GENDY noise plucked through ORPHICA microsound harp. Prophecy crystallised.",
        "Stochastic vision breakpoints arpeggiate ORPHICA lyre. Divination in microsound.",
        "Maqam Indigo envelope shapes ORPHICA shimmer decay. Sacred pluck.",
    ],
}

DESCRIPTIONS_ORACLE_ORACLE_ODYSSEY = [
    "ORACLE GENDY stochastic probability meets ODYSSEY tidal drift. Prophecy set adrift.",
    "Maqam Indigo breakpoints scatter into ODYSSEY voyage current. Vision in motion.",
    "ORACLE stochastic envelope modulates ODYSSEY drift rate. The oracle migrates.",
    "Prophetic breakpoints seed ODYSSEY's passage vectors. Divination as journey.",
    "GENDY noise triggers ODYSSEY wander state. Augury adrift on current.",
    "Indigo vision oscillates with ODYSSEY tidal float. Prophet and voyage unite.",
]

DESCRIPTIONS_ODYSSEY_EXTERNAL = {
    "OPAL":    [
        "ODYSSEY tidal drift modulates OPAL granular cloud dispersion. Journey in grain.",
        "Drift state variance seeds OPAL shimmer density. Wandering iridescence.",
        "ODYSSEY passage envelope shapes OPAL cloud release. Float into shimmer.",
    ],
    "OBLONG":  [
        "ODYSSEY voyage current guides Bob's curious character morph. Wandering warmth.",
        "Drift state modulates Oblong amplitude envelope. Journey of personality.",
        "Tidal float shapes Bob's harmonic drift position. Odyssey of curiosity.",
    ],
    "ORGANON": [
        "ODYSSEY drift chaos feeds ORGANON logical order. Journey meets system.",
        "Voyage current modulates ORGANON modular routing. Ordered migration.",
        "Tidal drift rhythms sync ORGANON's systematic pulse. Wandering logic.",
    ],
    "OCEANIC": [
        "ODYSSEY drift and OCEANIC tidal depth — double current entanglement.",
        "Voyage state modulates OCEANIC abyssal pressure. Deep migration.",
        "ODYSSEY wander feeds OCEANIC current velocity. Odyssey in the deep.",
    ],
    "OBLIQUE": [
        "ODYSSEY drift angle feeds OBLIQUE skew tension. Wandering off-axis.",
        "Tidal float shifts OBLIQUE's slant detune. Journey at an angle.",
        "Voyage state modulates OBLIQUE angle morphology. Oblique passage.",
    ],
    "OHM":     [
        "ODYSSEY drift float sustains OHM commune resonance. Wandering drone.",
        "Voyage current modulates OHM warmth field swell. Drifting commune.",
        "Tidal migration shapes OHM's resonant hum tempo. Journey in stillness.",
    ],
    "ORPHICA": [
        "ODYSSEY voyage current plucks ORPHICA microsound harp arpeggios. Drifting lyre.",
        "Drift state variance modulates ORPHICA shimmer release. Wandering pluck.",
        "Tidal float shapes ORPHICA's microsound grain density. Passage in microsound.",
    ],
    "OVERLAP": [
        "ODYSSEY passage threads through OVERLAP's FDN knot-topology. Voyage in braid.",
        "Drift current feeds OVERLAP nested layer depth. Wandering through web.",
        "Tidal float modulates OVERLAP's spatial reflection time. Drifting overlap.",
    ],
    "OBESE":   [
        "ODYSSEY tidal drift meets OBESE massive density. Journey into weight.",
        "Voyage envelope modulates OBESE drive intensity. Wandering heaviness.",
        "Drift state shifts OBESE amplitude mass. Passage through thickness.",
    ],
}

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def blend_dna(a_name: str, b_name: str, rng: random.Random) -> dict:
    """Blend two engine DNA baselines with slight random variation."""
    a = DNA[a_name]
    b = DNA[b_name]
    blend = {}
    for key in a:
        base = (a[key] + b[key]) / 2.0
        jitter = rng.uniform(-0.04, 0.04)
        blend[key] = round(max(0.0, min(1.0, base + jitter)), 3)
    return blend


def make_stub_params(engine_name: str, rng: random.Random) -> dict:
    """Generate minimal stub parameters for an engine."""
    prefix = PREFIX.get(engine_name, engine_name.lower()[:4] + "_")
    keys = [
        f"{prefix}outputLevel",
        f"{prefix}outputPan",
        f"{prefix}couplingLevel",
        f"{prefix}couplingBus",
    ]
    return {
        keys[0]: round(rng.uniform(0.75, 0.9), 3),
        keys[1]: round(rng.uniform(-0.05, 0.05), 3),
        keys[2]: round(rng.uniform(0.5, 0.85), 3),
        keys[3]: 0,
    }


def build_preset(
    name: str,
    engine_a: str,
    engine_b: str,
    description: str,
    coupling_type: str,
    coupling_amount: float,
    dna: dict,
    rng: random.Random,
) -> dict:
    macro_a = MACRO_SETS.get(engine_a, ["A", "B", "C", "D"])
    macro_b = MACRO_SETS.get(engine_b, ["A", "B", "C", "D"])
    macro_labels = [macro_a[0], macro_a[2], macro_b[0], macro_b[2]]

    intensity = "Deep" if abs(coupling_amount) >= 0.7 else "Moderate"

    tags = ["coupling", "entangled", engine_a.lower(), engine_b.lower()]

    params = {}
    params[engine_a] = make_stub_params(engine_a, rng)
    params[engine_b] = make_stub_params(engine_b, rng)

    return {
        "schema_version": 1,
        "name": name,
        "mood": "Entangled",
        "engines": [engine_a, engine_b],
        "author": "XO_OX Designs",
        "version": "1.0.0",
        "description": description,
        "tags": tags,
        "macroLabels": macro_labels,
        "couplingIntensity": intensity,
        "tempo": None,
        "dna": dna,
        "parameters": params,
        "coupling": {
            "pairs": [
                {
                    "engineA": engine_a,
                    "engineB": engine_b,
                    "type": coupling_type,
                    "amount": round(coupling_amount, 3),
                }
            ]
        },
        "sequencer": None,
    }


# ---------------------------------------------------------------------------
# Preset definitions
# ---------------------------------------------------------------------------

MARQUEE_ORACLE_ODYSSEY = [
    # (name, engine_a, engine_b, coupling_type, coupling_amount)
    ("Prophecy Drift",    "ORACLE", "ODYSSEY", "Stochastic->Morph", 0.75),
    ("Oracle Voyage",     "ORACLE", "ODYSSEY", "Env->Filter",       0.68),
    ("Prophetic Journey", "ORACLE", "ODYSSEY", "Spectral->Amp",     0.72),
    ("Stochastic Drift",  "ORACLE", "ODYSSEY", "LFO->Pitch",        0.65),
    ("Vision Quest",      "ODYSSEY","ORACLE",  "Tidal->LFO",        0.70),
    ("Maqam Odyssey",     "ORACLE", "ODYSSEY", "Env->Morph",        0.78),
]

# ORACLE external pairs: partner → coupling_types (3 entries each)
ORACLE_PAIRS = {
    "OPTIC":   [("Spectral->Amp", 0.72), ("Env->Filter",      0.65), ("LFO->Pitch",       0.68)],
    "OBLONG":  [("Env->Morph",    0.65), ("Stochastic->Morph",0.70), ("Amp->Filter",      0.60)],
    "ORBITAL": [("Spectral->Amp", 0.68), ("LFO->Pitch",       0.72), ("Env->Filter",      0.65)],
    "OVERLAP": [("Stochastic->Morph",0.75),("Env->Morph",     0.68), ("Spectral->Amp",    0.70)],
    "OMBRE":   [("Env->Filter",   0.62), ("Tidal->LFO",       0.58), ("Amp->Filter",      0.64)],
    "ORCA":    [("Stochastic->Morph",0.78),("Amp->Filter",    0.72), ("LFO->Pitch",       0.65)],
    "OCTOPUS": [("Spectral->Amp", 0.70), ("Stochastic->Morph",0.74), ("Env->Morph",       0.68)],
    "OHM":     [("Env->Filter",   0.62), ("Tidal->LFO",       0.65), ("Stochastic->Morph",0.70)],
    "ORPHICA": [("Spectral->Amp", 0.68), ("Stochastic->Morph",0.72), ("Env->Morph",       0.66)],
}

# ORACLE external preset names (partner → [3 names])
ORACLE_NAMES = {
    "OPTIC":   ["Prophetic Lens",    "Indigo Beam",         "Stochastic Light"],
    "OBLONG":  ["Oracle Bob",        "Augury of Bob",       "Maqam Curiosity"],
    "ORBITAL": ["Prophetic Orbit",   "Indigo Resonance",    "GENDY Partials"],
    "OVERLAP": ["Prophecy in Knots", "Stochastic Braid",    "Maqam Weave"],
    "OMBRE":   ["Vision Fade",       "Prophecy Dissolving", "Maqam Shadow"],
    "ORCA":    ["Prophetic Hunt",    "Indigo Strike",       "Stochastic Pod"],
    "OCTOPUS": ["Oracle Arms",       "GENDY Tentacle",      "Maqam Adaptation"],
    "OHM":     ["Prophecy Commune",  "Indigo Drone",        "Stochastic Hum"],
    "ORPHICA": ["Prophetic Harp",    "GENDY Lyre",          "Maqam Crystal"],
}

# ODYSSEY external pairs
ODYSSEY_PAIRS = {
    "OPAL":    [("Tidal->LFO",       0.70), ("Env->Morph",       0.68), ("LFO->Pitch",       0.65)],
    "OBLONG":  [("Env->Morph",       0.65), ("Amp->Filter",      0.60), ("Tidal->LFO",       0.68)],
    "ORGANON": [("Rhythm->Blend",    0.68), ("Tidal->LFO",       0.72), ("Env->Filter",      0.65)],
    "OCEANIC": [("Tidal->LFO",       0.78), ("Env->Morph",       0.72), ("Spectral->Amp",    0.68)],
    "OBLIQUE": [("LFO->Pitch",       0.68), ("Env->Filter",      0.65), ("Tidal->LFO",       0.70)],
    "OHM":     [("Tidal->LFO",       0.70), ("Env->Filter",      0.65), ("Amp->Filter",      0.62)],
    "ORPHICA": [("Tidal->LFO",       0.68), ("LFO->Pitch",       0.70), ("Env->Morph",       0.65)],
    "OVERLAP": [("Env->Morph",       0.72), ("Tidal->LFO",       0.68), ("Spectral->Amp",    0.70)],
    "OBESE":   [("Amp->Filter",      0.72), ("Tidal->LFO",       0.65), ("Rhythm->Blend",    0.70)],
}

ODYSSEY_NAMES = {
    "OPAL":    ["Drifting Grain",     "Voyage in Shimmer",   "Journey Cloud"],
    "OBLONG":  ["Wandering Bob",      "Drift Curiosity",     "Bob's Voyage"],
    "ORGANON": ["Ordered Passage",    "Logic Drift",         "System Journey"],
    "OCEANIC": ["Twin Currents",      "Abyssal Drift",       "Deep Migration"],
    "OBLIQUE": ["Off-Axis Voyage",    "Drift at Angle",      "Oblique Journey"],
    "OHM":     ["Drifting Commune",   "Voyage Drone",        "Wandering Hum"],
    "ORPHICA": ["Drifting Lyre",      "Voyage Pluck",        "Journey Microsound"],
    "OVERLAP": ["Drift in Knots",     "Voyage Braided",      "Wandering Web"],
    "OBESE":   ["Heavy Drift",        "Massive Voyage",      "Dense Journey"],
}


# ---------------------------------------------------------------------------
# Main generation logic
# ---------------------------------------------------------------------------

def generate_all_presets(rng: random.Random, count: int) -> list:
    """Return list of (preset_dict, filename) tuples."""
    presets = []

    # --- Marquee: ORACLE × ODYSSEY (always all 6) ---
    entries = MARQUEE_ORACLE_ODYSSEY
    for i, (name, eng_a, eng_b, ctype, camount) in enumerate(entries):
        desc = DESCRIPTIONS_ORACLE_ORACLE_ODYSSEY[i % len(DESCRIPTIONS_ORACLE_ORACLE_ODYSSEY)]
        dna = blend_dna(eng_a, eng_b, rng)
        p = build_preset(name, eng_a, eng_b, desc, ctype, camount, dna, rng)
        presets.append((p, f"{name}.xometa"))

    # --- ORACLE external pairs ---
    for partner, coupling_list in ORACLE_PAIRS.items():
        names = ORACLE_NAMES[partner]
        descs = DESCRIPTIONS_ORACLE_EXTERNAL[partner]
        entries_to_use = min(count, len(coupling_list))
        for j in range(entries_to_use):
            ctype, camount = coupling_list[j]
            name = names[j]
            desc = descs[j % len(descs)]
            dna = blend_dna("ORACLE", partner, rng)
            p = build_preset(name, "ORACLE", partner, desc, ctype, camount, dna, rng)
            presets.append((p, f"{name}.xometa"))

    # --- ODYSSEY external pairs ---
    for partner, coupling_list in ODYSSEY_PAIRS.items():
        names = ODYSSEY_NAMES[partner]
        descs = DESCRIPTIONS_ODYSSEY_EXTERNAL[partner]
        entries_to_use = min(count, len(coupling_list))
        for j in range(entries_to_use):
            ctype, camount = coupling_list[j]
            name = names[j]
            desc = descs[j % len(descs)]
            dna = blend_dna("ODYSSEY", partner, rng)
            p = build_preset(name, "ODYSSEY", partner, desc, ctype, camount, dna, rng)
            presets.append((p, f"{name}.xometa"))

    return presets


def write_presets(presets: list, output_dir: Path, dry_run: bool) -> list:
    output_dir.mkdir(parents=True, exist_ok=True)
    written = []
    for preset, filename in presets:
        filepath = output_dir / filename
        if dry_run:
            print(f"[dry-run] Would write: {filepath}")
        else:
            with open(filepath, "w") as f:
                json.dump(preset, f, indent=2)
                f.write("\n")
            print(f"Wrote: {filepath}")
        written.append(str(filepath))
    return written


def main():
    repo_root = Path(__file__).parent.parent
    default_output = repo_root / "Presets" / "XOmnibus" / "Entangled"

    parser = argparse.ArgumentParser(
        description="Generate ORACLE + ODYSSEY coupling pack presets for XOmnibus."
    )
    parser.add_argument(
        "--output-dir",
        type=Path,
        default=default_output,
        help=f"Directory to write .xometa files (default: {default_output})",
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Print filenames without writing files.",
    )
    parser.add_argument(
        "--seed",
        type=int,
        default=None,
        help="Random seed for reproducible DNA jitter.",
    )
    parser.add_argument(
        "--count",
        type=int,
        default=3,
        help="Number of presets per pair (1-3, default 3). Full marquee always uses 6.",
    )
    args = parser.parse_args()

    count = max(1, min(3, args.count))
    rng = random.Random(args.seed)

    presets = generate_all_presets(rng, count)
    written = write_presets(presets, args.output_dir, args.dry_run)

    action = "Would write" if args.dry_run else "Wrote"
    print(f"\n{action} {len(written)} presets to {args.output_dir}")


if __name__ == "__main__":
    main()
