#!/usr/bin/env python3
"""Generate .xometa coupling preset stubs for OBSIDIAN × ORACLE and their engine pairs.

OBSIDIAN (Crystal White #E8E0D8) — crystalline resonator, cold/bright/mineral
ORACLE   (Prophecy Indigo #4B0082) — GENDY stochastic + Maqam, mysterious/prophetic

Preset breakdown:
  OBSIDIAN × ORACLE marquee pairs  :  6 presets
  OBSIDIAN-led (22 engine partners): 22 presets (1 per partner)
  ORACLE-led   (22 engine partners): 22 presets (1 per partner)
  Total                            : 50 presets

Usage:
    python3 Tools/xpn_obsidian_oracle_coupling_pack.py
    python3 Tools/xpn_obsidian_oracle_coupling_pack.py --dry-run
    python3 Tools/xpn_obsidian_oracle_coupling_pack.py --output-dir /tmp/test
    python3 Tools/xpn_obsidian_oracle_coupling_pack.py --seed 42
"""

import argparse
import json
import random
from pathlib import Path

# ---------------------------------------------------------------------------
# Sonic DNA baselines
# OBSIDIAN: cold, bright, crystalline (brightness 0.85+, warmth 0.2)
# ORACLE:   mysterious, prophetic (brightness 0.4, warmth 0.5, movement 0.7)
# ---------------------------------------------------------------------------
DNA_BASELINES = {
    "OBSIDIAN":   dict(brightness=0.88, warmth=0.18, movement=0.28, density=0.50, space=0.62, aggression=0.12),
    "ORACLE":     dict(brightness=0.40, warmth=0.50, movement=0.72, density=0.65, space=0.68, aggression=0.38),
    "OUROBOROS":  dict(brightness=0.55, warmth=0.45, movement=0.80, density=0.70, space=0.60, aggression=0.50),
    "ORIGAMI":    dict(brightness=0.70, warmth=0.42, movement=0.65, density=0.55, space=0.55, aggression=0.55),
    "OPTIC":      dict(brightness=0.85, warmth=0.28, movement=0.90, density=0.40, space=0.50, aggression=0.45),
    "OBLIQUE":    dict(brightness=0.60, warmth=0.50, movement=0.58, density=0.60, space=0.65, aggression=0.40),
    "OSPREY":     dict(brightness=0.65, warmth=0.48, movement=0.75, density=0.55, space=0.60, aggression=0.55),
    "OSTERIA":    dict(brightness=0.55, warmth=0.72, movement=0.50, density=0.65, space=0.55, aggression=0.30),
    "OWLFISH":    dict(brightness=0.42, warmth=0.58, movement=0.42, density=0.62, space=0.80, aggression=0.20),
    "OHM":        dict(brightness=0.45, warmth=0.75, movement=0.55, density=0.60, space=0.70, aggression=0.30),
    "ORPHICA":    dict(brightness=0.80, warmth=0.50, movement=0.70, density=0.45, space=0.75, aggression=0.25),
    "OBBLIGATO":  dict(brightness=0.55, warmth=0.65, movement=0.60, density=0.65, space=0.60, aggression=0.45),
    "OTTONI":     dict(brightness=0.60, warmth=0.60, movement=0.50, density=0.70, space=0.55, aggression=0.60),
    "OLE":        dict(brightness=0.65, warmth=0.70, movement=0.75, density=0.60, space=0.60, aggression=0.55),
    "OMBRE":      dict(brightness=0.50, warmth=0.62, movement=0.60, density=0.58, space=0.68, aggression=0.32),
    "ORCA":       dict(brightness=0.45, warmth=0.40, movement=0.65, density=0.75, space=0.50, aggression=0.70),
    "OCTOPUS":    dict(brightness=0.55, warmth=0.55, movement=0.78, density=0.72, space=0.45, aggression=0.60),
    "ONSET":      dict(brightness=0.50, warmth=0.55, movement=0.70, density=0.80, space=0.35, aggression=0.70),
    "OPAL":       dict(brightness=0.75, warmth=0.50, movement=0.75, density=0.45, space=0.80, aggression=0.20),
    "ORBITAL":    dict(brightness=0.60, warmth=0.65, movement=0.55, density=0.60, space=0.60, aggression=0.50),
    "OVERDUB":    dict(brightness=0.45, warmth=0.72, movement=0.55, density=0.55, space=0.75, aggression=0.25),
    "ODYSSEY":    dict(brightness=0.55, warmth=0.50, movement=0.68, density=0.50, space=0.70, aggression=0.30),
    "OBLONG":     dict(brightness=0.58, warmth=0.62, movement=0.52, density=0.68, space=0.52, aggression=0.48),
    "OVERWORLD":  dict(brightness=0.70, warmth=0.40, movement=0.65, density=0.60, space=0.45, aggression=0.55),
}

# ---------------------------------------------------------------------------
# Engine display IDs and parameter prefixes
# ---------------------------------------------------------------------------
ENGINE_ID = {
    "OBSIDIAN":  "Obsidian",
    "ORACLE":    "Oracle",
    "OUROBOROS": "Ouroboros",
    "ORIGAMI":   "Origami",
    "OPTIC":     "Optic",
    "OBLIQUE":   "Oblique",
    "OSPREY":    "Osprey",
    "OSTERIA":   "Osteria",
    "OWLFISH":   "Owlfish",
    "OHM":       "Ohm",
    "ORPHICA":   "Orphica",
    "OBBLIGATO": "Obbligato",
    "OTTONI":    "Ottoni",
    "OLE":       "Ole",
    "OMBRE":     "Ombre",
    "ORCA":      "Orca",
    "OCTOPUS":   "Octopus",
    "ONSET":     "Onset",
    "OPAL":      "Opal",
    "ORBITAL":   "Orbital",
    "OVERDUB":   "Overdub",
    "ODYSSEY":   "Odyssey",
    "OBLONG":    "Oblong",
    "OVERWORLD": "Overworld",
}

PARAM_PREFIX = {
    "OBSIDIAN":  "obsidian_",
    "ORACLE":    "oracle_",
    "OUROBOROS": "ouro_",
    "ORIGAMI":   "origami_",
    "OPTIC":     "optic_",
    "OBLIQUE":   "oblique_",
    "OSPREY":    "osprey_",
    "OSTERIA":   "osteria_",
    "OWLFISH":   "owl_",
    "OHM":       "ohm_",
    "ORPHICA":   "orph_",
    "OBBLIGATO": "obbl_",
    "OTTONI":    "otto_",
    "OLE":       "ole_",
    "OMBRE":     "ombre_",
    "ORCA":      "orca_",
    "OCTOPUS":   "oct_",
    "ONSET":     "onset_",
    "OPAL":      "opal_",
    "ORBITAL":   "orb_",
    "OVERDUB":   "dub_",
    "ODYSSEY":   "drift_",
    "OBLONG":    "bob_",
    "OVERWORLD": "ow_",
}

# Coupling types drawn from the full canonical set
COUPLING_TYPES = [
    "HARMONIC_BLEND",
    "FREQUENCY_MODULATION",
    "AMPLITUDE_MODULATION",
    "FILTER_COUPLING",
    "ENVELOPE_SHARING",
    "SPECTRAL_TRANSFER",
    "RHYTHMIC_SYNC",
    "CHAOS_INJECTION",
    "PITCH_TRACKING",
    "WAVETABLE_MORPH",
    "SPATIAL_BLEND",
    "GRANULAR_EXCHANGE",
]

DATE = "2026-03-16"

# ---------------------------------------------------------------------------
# Name vocabularies
# ---------------------------------------------------------------------------
VOCAB_OBSIDIAN = [
    "Crystal", "Mineral", "Vitreous", "Facet", "Edge", "Clear", "Shard",
    "Glacial", "Brittle", "Prismatic", "Sharp", "Pure",
]
VOCAB_ORACLE = [
    "Prophecy", "Vision", "Augury", "Maqam", "Oracle", "Foretold", "Rune",
    "Stochastic", "Sibyl", "Omen", "Divination", "Arcane",
]
MARQUEE_NAMES = [
    "Crystal Prophecy",
    "Vitreous Oracle",
    "Mineral Vision",
    "Glacial Augury",
    "Shard of Divination",
    "Prismatic Foretelling",
]

# Partner-specific vocabulary for name generation
PARTNER_VOCAB = {
    "OUROBOROS": ["Cycle", "Serpent", "Infinite", "Loop", "Coil"],
    "ORIGAMI":   ["Fold", "Crease", "Geometric", "Paper", "Scored"],
    "OPTIC":     ["Pulse", "Phosphene", "Retinal", "Visual", "Flash"],
    "OBLIQUE":   ["Slant", "Angled", "Diagonal", "Skew", "Tilt"],
    "OSPREY":    ["Soar", "Predator", "Thermal", "Hover", "Dive"],
    "OSTERIA":   ["Warm", "Table", "Gathering", "Savory", "Candlelit"],
    "OWLFISH":   ["Abyssal", "Filament", "Bioluminescent", "Deep", "Silent"],
    "OHM":       ["Commune", "Drift", "Campfire", "Still", "Jam"],
    "ORPHICA":   ["Shimmer", "Grain", "Harp", "Spectral", "Ether"],
    "OBBLIGATO": ["Reed", "Breath", "Wind", "Bond", "Swell"],
    "OTTONI":    ["Brass", "Copper", "Fanfare", "Bell", "Horn"],
    "OLE":       ["Clave", "Rhythmic", "Afro", "Drama", "Groove"],
    "OMBRE":     ["Shadow", "Gradient", "Haze", "Fade", "Dusk"],
    "ORCA":      ["Predatory", "Deep Strike", "Sonar", "Breach", "Pod"],
    "OCTOPUS":   ["Arm", "Ink", "Cellular", "Wolfram", "Distributed"],
    "ONSET":     ["Drum", "Attack", "Impact", "Crack", "Punch"],
    "OPAL":      ["Iridescent", "Granular", "Dispersed", "Fire", "Shimmer"],
    "ORBITAL":   ["Loop", "Rotate", "Ellipse", "Gravitational", "Orbit"],
    "OVERDUB":   ["Dub", "Tape", "Saturated", "Echo", "Layered"],
    "ODYSSEY":   ["Voyage", "Drift", "Wander", "Journey", "Waypoint"],
    "OBLONG":    ["Skewed", "Stretched", "Character", "Fat", "Oblate"],
    "OVERWORLD": ["Chip", "Retro", "Era", "Glitch", "CRT"],
}

# Descriptions for OBSIDIAN-led pairs
OBSIDIAN_PAIR_DESCS = {
    "OUROBOROS": "Crystal clarity wraps the infinite coil. OBSIDIAN edge loops through OUROBOROS cycle.",
    "ORIGAMI":   "Mineral facets fold along geometric crease lines. OBSIDIAN geometry meets ORIGAMI mathematics.",
    "OPTIC":     "Prismatic shards refract phosphor pulse. OBSIDIAN crystal lens focuses OPTIC visual field.",
    "OBLIQUE":   "Ice-clear signal hits a slanted plane. OBSIDIAN purity couples with OBLIQUE angular drift.",
    "OSPREY":    "Glacial crystal thermal — cold ascent via OSPREY predator soar.",
    "OSTERIA":   "Cold mineral edge warms at the candlelit table. OBSIDIAN clarity in OSTERIA gathering.",
    "OWLFISH":   "Crystal surface couples with abyssal filament. Cold light descends into OWLFISH deep.",
    "OHM":       "Pure mineral edge feeds the commune drone. OBSIDIAN sharpness dissolves in OHM stillness.",
    "ORPHICA":   "Crystal facets scatter into ORPHICA shimmer. Cold grain harp from vitreous shards.",
    "OBBLIGATO": "OBSIDIAN edge breathes through OBBLIGATO wind bond. Crystal meets double reed necessity.",
    "OTTONI":    "Mineral hardness resonates through OTTONI brass. Cold crystal fanfare.",
    "OLE":       "Faceted crystal catches the clave drama. OBSIDIAN purity in OLE Afro-Latin groove.",
    "OMBRE":     "Clear OBSIDIAN surface fades into OMBRE gradient shadow.",
    "ORCA":      "Crystal sonar pulse couples with ORCA deep-strike predatory force.",
    "OCTOPUS":   "OBSIDIAN mineral feeds OCTOPUS distributed cellular arms. Cold logic, distributed spread.",
    "ONSET":     "Crystal crack meets ONSET percussive attack. Pure mineral impact.",
    "OPAL":      "OBSIDIAN cold brightness scatters through OPAL iridescent granular cloud.",
    "ORBITAL":   "Volcanic glass traces elliptical ORBITAL path. Crystal in gravitational rotation.",
    "OVERDUB":   "Pure crystal signal enters OVERDUB tape saturation. Mineral clarity warped.",
    "ODYSSEY":   "OBSIDIAN edge as compass heading. Crystal navigation through ODYSSEY voyage.",
    "OBLONG":    "Mineral sharpness stretched through OBLONG oblate character.",
    "OVERWORLD": "OBSIDIAN cold clarity powers OVERWORLD chip retro CRT shimmer.",
}

# Descriptions for ORACLE-led pairs
ORACLE_PAIR_DESCS = {
    "OUROBOROS": "Prophecy cycles through infinite coil. ORACLE stochastic vision feeds OUROBOROS loop.",
    "ORIGAMI":   "Oracle fold — prophecy geometry written in ORIGAMI crease mathematics.",
    "OPTIC":     "Stochastic vision phosphene. ORACLE maqam feeds OPTIC visual pulse field.",
    "OBLIQUE":   "Prophecy arrives at a slant. ORACLE arcane signal couples with OBLIQUE diagonal drift.",
    "OSPREY":    "Oracle augury soars on OSPREY thermal. Vision carried on predator wings.",
    "OSTERIA":   "Prophecy over the candlelit table. ORACLE vision shared at OSTERIA gathering.",
    "OWLFISH":   "Arcane vision descends into OWLFISH deep filament. Prophet in the abyss.",
    "OHM":       "Oracle communes in silence. Stochastic maqam dissolves into OHM still drone.",
    "ORPHICA":   "Prophetic grain harp. ORACLE stochastic micro-gestures shimmer through ORPHICA.",
    "OBBLIGATO": "Oracle breathes necessity. Prophecy through OBBLIGATO wind bond obligation.",
    "OTTONI":    "Prophecy brass fanfare. ORACLE maqam modal ascent through OTTONI triple horn.",
    "OLE":       "Arcane drama clave. ORACLE stochastic rhythm coupled with OLE Afro-Latin pulse.",
    "OMBRE":     "Vision fades into OMBRE gradient. Oracle prophecy diffusing into shadow.",
    "ORCA":      "Oracle predicts the breach. Prophetic sonar couples with ORCA deep strike.",
    "OCTOPUS":   "ORACLE stochastic feeds OCTOPUS Wolfram cellular arms. Distributed prophecy.",
    "ONSET":     "Prophetic attack. ORACLE stochastic triggers ONSET percussive impact.",
    "OPAL":      "Oracle vision scatters through OPAL granular iridescence. Prophecy dispersed.",
    "ORBITAL":   "Vision in elliptical path. ORACLE arcane loops in ORBITAL gravitational field.",
    "OVERDUB":   "Prophecy on tape. ORACLE stochastic maqam warped through OVERDUB dub saturation.",
    "ODYSSEY":   "Oracle charts the voyage. Prophetic heading through ODYSSEY long drift.",
    "OBLONG":    "Arcane character stretching. ORACLE stochastic coupled with OBLONG oblate density.",
    "OVERWORLD": "Prophecy as glitch code. ORACLE maqam through OVERWORLD CRT chip retro noise.",
}

# ---------------------------------------------------------------------------
# DNA helpers
# ---------------------------------------------------------------------------
def blend_dna(key_a, key_b, weight_a=0.5, rng=None):
    a = DNA_BASELINES[key_a]
    b = DNA_BASELINES[key_b]
    w = weight_a

    def mix(x, y):
        v = x * w + y * (1.0 - w)
        if rng:
            v += rng.uniform(-0.04, 0.04)
        return round(max(0.0, min(1.0, v)), 3)

    return {k: mix(a[k], b[k]) for k in a}


def stub_params(engine_key, rng):
    prefix = PARAM_PREFIX[engine_key]
    base = DNA_BASELINES[engine_key]
    vary = lambda v: round(max(0.0, min(1.0, v + rng.uniform(-0.08, 0.08))), 3)
    return {
        f"{prefix}brightness": vary(base["brightness"]),
        f"{prefix}warmth":     vary(base["warmth"]),
        f"{prefix}movement":   vary(base["movement"]),
        f"{prefix}density":    vary(base["density"]),
        f"{prefix}space":      vary(base["space"]),
        f"{prefix}aggression": vary(base["aggression"]),
        f"{prefix}level":      round(rng.uniform(0.70, 0.90), 3),
    }


# ---------------------------------------------------------------------------
# Preset factory
# ---------------------------------------------------------------------------
def make_preset(name, desc, tags, engine_keys, coupling_type, coupling_amount, dna_blend, rng):
    engine_ids = [ENGINE_ID[k] for k in engine_keys]
    params = {}
    for k in engine_keys:
        params[ENGINE_ID[k]] = stub_params(k, rng)

    intensity = rng.choice(["Moderate", "Deep", "Subtle"])

    return {
        "schema_version": 1,
        "name": name,
        "mood": "Entangled",
        "engines": engine_ids,
        "author": "XO_OX",
        "version": "1.0.0",
        "description": desc,
        "tags": tags,
        "macroLabels": ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"],
        "couplingIntensity": intensity,
        "tempo": None,
        "created": DATE,
        "legacy": {
            "sourceInstrument": None,
            "sourceCategory": None,
            "sourcePresetName": None,
        },
        "parameters": params,
        "coupling": {
            "pairs": [
                {
                    "engineA": engine_ids[0],
                    "engineB": engine_ids[1],
                    "type": coupling_type,
                    "amount": round(coupling_amount, 3),
                }
            ]
        },
        "sequencer": None,
        "dna": dna_blend,
    }


# ---------------------------------------------------------------------------
# Name helpers
# ---------------------------------------------------------------------------
def marquee_name(idx):
    return MARQUEE_NAMES[idx % len(MARQUEE_NAMES)]


def pair_name(primary_vocab, partner_key, rng):
    partner_vocab = PARTNER_VOCAB.get(partner_key, ["Coupled", "Linked", "Merged"])
    w1 = rng.choice(primary_vocab)
    w2 = rng.choice(partner_vocab)
    return f"{w1} {w2}"[:40]


# ---------------------------------------------------------------------------
# Preset suite builder
# ---------------------------------------------------------------------------

PARTNER_LIST = [
    "OUROBOROS", "ORIGAMI", "OPTIC", "OBLIQUE", "OSPREY", "OSTERIA",
    "OWLFISH", "OHM", "ORPHICA", "OBBLIGATO", "OTTONI", "OLE",
    "OMBRE", "ORCA", "OCTOPUS", "ONSET", "OPAL", "ORBITAL",
    "OVERDUB", "ODYSSEY", "OBLONG", "OVERWORLD",
]

# Assign coupling types deterministically across partner list for variety
COUPLING_ASSIGNMENT_OBS = {
    "OUROBOROS": "HARMONIC_BLEND",
    "ORIGAMI":   "SPECTRAL_TRANSFER",
    "OPTIC":     "FREQUENCY_MODULATION",
    "OBLIQUE":   "FILTER_COUPLING",
    "OSPREY":    "PITCH_TRACKING",
    "OSTERIA":   "AMPLITUDE_MODULATION",
    "OWLFISH":   "SPATIAL_BLEND",
    "OHM":       "ENVELOPE_SHARING",
    "ORPHICA":   "GRANULAR_EXCHANGE",
    "OBBLIGATO": "HARMONIC_BLEND",
    "OTTONI":    "SPECTRAL_TRANSFER",
    "OLE":       "RHYTHMIC_SYNC",
    "OMBRE":     "SPATIAL_BLEND",
    "ORCA":      "AMPLITUDE_MODULATION",
    "OCTOPUS":   "CHAOS_INJECTION",
    "ONSET":     "RHYTHMIC_SYNC",
    "OPAL":      "GRANULAR_EXCHANGE",
    "ORBITAL":   "WAVETABLE_MORPH",
    "OVERDUB":   "FILTER_COUPLING",
    "ODYSSEY":   "PITCH_TRACKING",
    "OBLONG":    "ENVELOPE_SHARING",
    "OVERWORLD": "FREQUENCY_MODULATION",
}

COUPLING_ASSIGNMENT_ORC = {
    "OUROBOROS": "CHAOS_INJECTION",
    "ORIGAMI":   "WAVETABLE_MORPH",
    "OPTIC":     "SPECTRAL_TRANSFER",
    "OBLIQUE":   "HARMONIC_BLEND",
    "OSPREY":    "AMPLITUDE_MODULATION",
    "OSTERIA":   "ENVELOPE_SHARING",
    "OWLFISH":   "SPATIAL_BLEND",
    "OHM":       "GRANULAR_EXCHANGE",
    "ORPHICA":   "FREQUENCY_MODULATION",
    "OBBLIGATO": "FILTER_COUPLING",
    "OTTONI":    "PITCH_TRACKING",
    "OLE":       "RHYTHMIC_SYNC",
    "OMBRE":     "SPECTRAL_TRANSFER",
    "ORCA":      "CHAOS_INJECTION",
    "OCTOPUS":   "WAVETABLE_MORPH",
    "ONSET":     "RHYTHMIC_SYNC",
    "OPAL":      "HARMONIC_BLEND",
    "ORBITAL":   "SPATIAL_BLEND",
    "OVERDUB":   "ENVELOPE_SHARING",
    "ODYSSEY":   "PITCH_TRACKING",
    "OBLONG":    "AMPLITUDE_MODULATION",
    "OVERWORLD": "FREQUENCY_MODULATION",
}

MARQUEE_COUPLING_TYPES = [
    "HARMONIC_BLEND",
    "SPECTRAL_TRANSFER",
    "FREQUENCY_MODULATION",
    "WAVETABLE_MORPH",
    "CHAOS_INJECTION",
    "SPATIAL_BLEND",
]

MARQUEE_DESCS = [
    "Crystal clarity prophecy — OBSIDIAN vitreous edge channels ORACLE stochastic maqam divination.",
    "Mineral shards refracting arcane vision. Cold brightness feeds prophetic GENDY stochastic field.",
    "Glacial surface carries the augury. OBSIDIAN cold space opens as ORACLE prophecy unfolds.",
    "Pure crystal edge as scrying surface. OBSIDIAN facets reveal what ORACLE stochastic predicts.",
    "Crystalline resonance of the prophet — cold brightness coupled to indigo mystery.",
    "Vitreous oracle window — OBSIDIAN prismatic clarity transmits ORACLE maqam modal prophecy.",
]


def build_presets(rng):
    presets = []

    # --- 6 MARQUEE: OBSIDIAN × ORACLE ---
    for i in range(6):
        name = marquee_name(i)
        ctype = MARQUEE_COUPLING_TYPES[i]
        amt = rng.uniform(0.60, 0.88)
        dna = blend_dna("OBSIDIAN", "ORACLE", weight_a=0.50, rng=rng)
        tags = ["obsidian", "oracle", "crystal", "prophecy", "marquee", "entangled"]
        p = make_preset(name, MARQUEE_DESCS[i], tags,
                        ["OBSIDIAN", "ORACLE"], ctype, amt, dna, rng)
        presets.append(p)

    # --- 22 OBSIDIAN-led presets ---
    for partner in PARTNER_LIST:
        name = pair_name(VOCAB_OBSIDIAN, partner, rng)
        ctype = COUPLING_ASSIGNMENT_OBS[partner]
        amt = rng.uniform(0.42, 0.82)
        dna = blend_dna("OBSIDIAN", partner, weight_a=0.58, rng=rng)
        tags = ["obsidian", partner.lower(), "crystal", "entangled"]
        desc = OBSIDIAN_PAIR_DESCS.get(partner, f"OBSIDIAN crystalline resonator couples with {partner}.")
        p = make_preset(name, desc, tags, ["OBSIDIAN", partner], ctype, amt, dna, rng)
        presets.append(p)

    # --- 22 ORACLE-led presets ---
    for partner in PARTNER_LIST:
        name = pair_name(VOCAB_ORACLE, partner, rng)
        ctype = COUPLING_ASSIGNMENT_ORC[partner]
        amt = rng.uniform(0.42, 0.82)
        dna = blend_dna("ORACLE", partner, weight_a=0.58, rng=rng)
        tags = ["oracle", partner.lower(), "prophecy", "entangled"]
        desc = ORACLE_PAIR_DESCS.get(partner, f"ORACLE prophecy engine couples with {partner}.")
        p = make_preset(name, desc, tags, ["ORACLE", partner], ctype, amt, dna, rng)
        presets.append(p)

    return presets


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------
def main():
    parser = argparse.ArgumentParser(
        description="Generate OBSIDIAN × ORACLE coupling presets for XOmnibus."
    )
    repo_root = Path(__file__).resolve().parent.parent
    default_out = str(repo_root / "Presets" / "XOmnibus" / "Entangled")

    parser.add_argument(
        "--output-dir", default=default_out,
        help="Directory to write .xometa files (default: Presets/XOmnibus/Entangled/)",
    )
    parser.add_argument(
        "--dry-run", action="store_true",
        help="Print preset names without writing files",
    )
    parser.add_argument(
        "--seed", type=int, default=None,
        help="Random seed for reproducible output",
    )
    args = parser.parse_args()

    rng = random.Random(args.seed)
    presets = build_presets(rng)

    marquee      = [p for p in presets if "marquee" in p["tags"]]
    obs_led      = [p for p in presets if "obsidian" in p["tags"] and "marquee" not in p["tags"]]
    oracle_led   = [p for p in presets if "oracle" in p["tags"] and "marquee" not in p["tags"]]

    print(f"OBSIDIAN × ORACLE marquee : {len(marquee):3d} presets")
    print(f"OBSIDIAN-led pairs        : {len(obs_led):3d} presets")
    print(f"ORACLE-led   pairs        : {len(oracle_led):3d} presets")
    print(f"Total                     : {len(presets):3d} presets")

    if args.dry_run:
        print("\n--- DRY RUN ---")
        for p in presets:
            print(f"  [{p['engines'][0]:12s} + {p['engines'][1]:12s}]  {p['name']}")
        return

    out_dir = Path(args.output_dir)
    out_dir.mkdir(parents=True, exist_ok=True)

    written = 0
    skipped = 0
    for preset in presets:
        filename = preset["name"].replace(" ", "_").replace("/", "-") + ".xometa"
        filepath = out_dir / filename
        if filepath.exists():
            skipped += 1
            continue
        with open(filepath, "w", encoding="utf-8") as f:
            json.dump(preset, f, indent=2)
        written += 1

    print(f"\nWritten : {written}")
    print(f"Skipped : {skipped} (already exist)")
    print(f"Output  : {out_dir}")


if __name__ == "__main__":
    main()
