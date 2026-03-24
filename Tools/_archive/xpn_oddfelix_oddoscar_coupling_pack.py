#!/usr/bin/env python3
"""Generate OddfeliX (ODDFELIX) and OddOscar (ODDOSCAR) coupling presets for XOlokun.

The founding sibling pair — feliX the neon tetra (clinical/bright, snap_ prefix) meets
Oscar the axolotl (warm/organic, morph_ prefix).  Together they define the core feliX-Oscar
axis: digital vs. animal, cold vs. warm, precision vs. surrender.

Output: 62 presets total
  - ODDFELIX × ODDOSCAR marquee pair           =  8  (founding axis — largest marquee)
  - ODDFELIX × 9 external partners × 3 each   = 27
  - ODDOSCAR × 9 external partners × 3 each   = 27

Usage:
    python3 Tools/xpn_oddfelix_oddoscar_coupling_pack.py
    python3 Tools/xpn_oddfelix_oddoscar_coupling_pack.py --dry-run
    python3 Tools/xpn_oddfelix_oddoscar_coupling_pack.py --count 5 --seed 99
    python3 Tools/xpn_oddfelix_oddoscar_coupling_pack.py --output-dir /tmp/test
"""

import argparse
import json
import os
import random
from pathlib import Path

# ---------------------------------------------------------------------------
# DNA baselines
# ---------------------------------------------------------------------------
ODDFELIX_DNA = dict(
    brightness=0.85, warmth=0.2,  movement=0.6,
    density=0.5,     space=0.6,   aggression=0.4,
)
ODDOSCAR_DNA = dict(
    brightness=0.3,  warmth=0.8,  movement=0.5,
    density=0.6,     space=0.65,  aggression=0.2,
)

PARTNER_DNA: dict[str, dict] = {
    # OddfeliX external partners
    "ORACLE":    dict(brightness=0.5,  warmth=0.4,  movement=0.6,  density=0.7,  space=0.7,  aggression=0.4),
    "OVERWORLD": dict(brightness=0.75, warmth=0.4,  movement=0.5,  density=0.65, space=0.55, aggression=0.6),
    "ORIGAMI":   dict(brightness=0.7,  warmth=0.45, movement=0.65, density=0.55, space=0.55, aggression=0.55),
    "OBLIQUE":   dict(brightness=0.75, warmth=0.4,  movement=0.7,  density=0.5,  space=0.6,  aggression=0.55),
    "OPTIC":     dict(brightness=0.85, warmth=0.3,  movement=0.9,  density=0.4,  space=0.5,  aggression=0.45),
    "ONSET":     dict(brightness=0.55, warmth=0.5,  movement=0.8,  density=0.75, space=0.5,  aggression=0.7),
    "OBSIDIAN":  dict(brightness=0.9,  warmth=0.2,  movement=0.3,  density=0.5,  space=0.6,  aggression=0.1),
    "OUTWIT":    dict(brightness=0.6,  warmth=0.45, movement=0.8,  density=0.65, space=0.55, aggression=0.5),
    "OPAL":      dict(brightness=0.7,  warmth=0.5,  movement=0.75, density=0.45, space=0.8,  aggression=0.2),
    # OddOscar external partners
    "OVERDUB":   dict(brightness=0.45, warmth=0.7,  movement=0.55, density=0.55, space=0.75, aggression=0.25),
    "ORGANON":   dict(brightness=0.6,  warmth=0.6,  movement=0.65, density=0.75, space=0.65, aggression=0.4),
    "OCEANIC":   dict(brightness=0.65, warmth=0.55, movement=0.5,  density=0.55, space=0.75, aggression=0.3),
    "OBBLIGATO": dict(brightness=0.55, warmth=0.65, movement=0.6,  density=0.65, space=0.6,  aggression=0.45),
    "OHM":       dict(brightness=0.45, warmth=0.75, movement=0.55, density=0.6,  space=0.7,  aggression=0.3),
    "ORBITAL":   dict(brightness=0.6,  warmth=0.65, movement=0.55, density=0.6,  space=0.6,  aggression=0.5),
    "OWLFISH":   dict(brightness=0.4,  warmth=0.6,  movement=0.4,  density=0.65, space=0.8,  aggression=0.2),
}

COUPLING_TYPES = ["SYNC_LFO", "FILTER_MOD", "AMP_MOD", "PITCH_MOD", "ENV_MOD"]

# ---------------------------------------------------------------------------
# Name vocabularies
# ---------------------------------------------------------------------------
MARQUEE_NAMES = [
    "The Axis",
    "Felix Meets Oscar",
    "Neon Axolotl",
    "Clinical Organic",
    "Digital Animal",
    "Cold Warm",
    "The Siblings",
    "Founding Pair",
]

ODDFELIX_WORDS = [
    "Neon", "Clinical", "Digital", "Bright Wire",
    "Tetra Flash", "Crystal Signal", "Phosphor Cut",
]
ODDOSCAR_WORDS = [
    "Gill", "Axolotl", "Warm Body", "Organic Pulse",
    "Pink Depth", "Soft Signal", "Flesh Tone",
]

# ---------------------------------------------------------------------------
# Pair descriptions
# ---------------------------------------------------------------------------
MARQUEE_DESCS = [
    "The founding axis: feliX clinical precision collides with Oscar warm surrender — the original tension from which all XO_OX coupling flows",
    "Two siblings meet across the species divide — neon tetra signal sharpens the axolotl\'s soft biology into focus",
    "snap_ digital grid hosts morph_ organic drift — the cold tetra net catches the warm axolotl current",
    "OddfeliX filter precision tracks OddOscar envelope swell — clinical structure emerges from organic breath",
    "OddOscar warmth modulates OddfeliX brightness floor — the axolotl flushes the tetra\'s glow",
    "COUPLING depth inverts across the axis: feliX drives hard, Oscar yields — control and surrender in one patch",
    "snap_ pitch stability meets morph_ pitch drift — one holds the frequency, one lets it breathe",
    "The first pair. The ground of every coupling in this fleet. Neither complete without the other.",
]

ODDFELIX_DESCS: dict[str, list[str]] = {
    "ORACLE": [
        "Oracle stochastic resonance feeds OddfeliX brightness control — prophecy sharpens the tetra flash",
        "OddfeliX digital grid quantises ORACLE breakpoint timing — precision imprisons the oracle",
        "ORACLE filter sweep modulates snap_ filterCutoff — divination reshapes the neon spectrum",
    ],
    "OVERWORLD": [
        "Overworld ERA state selects OddfeliX oscillator mode — chip era as clinical parameter",
        "OddfeliX phosphor brightness maps to OVERWORLD NES duty cycle — clinical signal, retro output",
        "OVERWORLD glitch rate triggers snap_ envelope reset — pixel collapse resets the tetra",
    ],
    "ORIGAMI": [
        "Origami fold depth modulates OddfeliX filter resonance — geometry sharpens the neon edge",
        "OddfeliX digital step rate clocks ORIGAMI crease events — tetra flash folds new shapes",
        "ORIGAMI carrier pitch seeds snap_ oscillator fine-tune — one fold, one frequency",
    ],
    "OBLIQUE": [
        "OBLIQUE prism angle modulates OddfeliX brightness — light splits the neon signal",
        "OddfeliX filter tracking drives OBLIQUE FM index — clinical precision into oblique chaos",
        "OBLIQUE rate generator clocks snap_ LFO reset — skewed time, precise pulse",
    ],
    "OPTIC": [
        "Two phosphor engines in resonance — OPTIC pulse rate syncs to snap_ LFO clock",
        "OddfeliX oscillator brightness maps to OPTIC harmonic tilt — one light source, two spectra",
        "OPTIC waveshaper output feeds snap_ filter input — visual modulation as clinical signal",
    ],
    "ONSET": [
        "ONSET percussion transient triggers OddfeliX envelope gate — drum hit as clinical event",
        "OddfeliX brightness floor sets ONSET perc noise level — the tetra flash illuminates the drum",
        "ONSET machine macro modulates snap_ aggression — the drum tightens the neon grid",
    ],
    "OBSIDIAN": [
        "Two bright clinical engines: OBSIDIAN crystal stillness meets OddfeliX neon precision",
        "OddfeliX oscillator movement modulates OBSIDIAN depth parameter — motion disturbs the crystal",
        "OBSIDIAN wavefold extends snap_ filter modulation range — crystal edge sharpens neon cut",
    ],
    "OUTWIT": [
        "OUTWIT Wolfram rules seed OddfeliX step sequencer pattern — cellular law as clinical data",
        "OddfeliX grid clock drives OUTWIT generation rate — the tetra sets the pace of evolution",
        "OUTWIT arm count maps to snap_ oscillator voice count — 8 arms, 8 neon pulses",
    ],
    "OPAL": [
        "OPAL granular clouds feed OddfeliX filter input — spectral shimmer through a clinical lens",
        "OddfeliX brightness precision drives OPAL grain playback rate — the tetra flashes the grains",
        "OPAL space parameter modulates snap_ reverb send — granular depth opens the neon room",
    ],
}

ODDOSCAR_DESCS: dict[str, list[str]] = {
    "OVERDUB": [
        "OVERDUB tape delay warmth wraps OddOscar organic signal — axolotl through the dub machine",
        "OddOscar envelope swell modulates OVERDUB send amount — organic breath controls the echo",
        "OVERDUB spring reverb feeds morph_ filter — the metallic splash warms the axolotl body",
    ],
    "OPAL": [
        "OPAL granular texture feeds OddOscar modulation bus — scattered grains become axolotl motion",
        "OddOscar warmth parameter expands OPAL grain density — organic heat multiplies the clouds",
        "OPAL space depth modulates morph_ envelope release — the grains extend the axolotl breath",
    ],
    "ORGANON": [
        "ORGANON metabolic rate clocks OddOscar LFO — biological metabolism drives organic drift",
        "OddOscar envelope shape modulates ORGANON pipe register — the axolotl breathes the organ",
        "ORGANON variational energy maps to morph_ density — the organism fills the axolotl body",
    ],
    "OCEANIC": [
        "Two organic deep-water engines: OCEANIC current feeds OddOscar drift rate",
        "OddOscar warmth modulates OCEANIC depth zone index — the axolotl descends",
        "OCEANIC phosphorescent teal maps to morph_ filter — bioluminescence as warm signal",
    ],
    "OBBLIGATO": [
        "OBBLIGATO dual winds breathe into OddOscar envelope swell — oboe + axolotl biology",
        "OddOscar gill motion modulates OBBLIGATO BOND macro — soft biology tightens the bond",
        "OBBLIGATO breath rate clocks morph_ LFO — woodwind time as organic time",
    ],
    "OHM": [
        "OHM commune warmth wraps OddOscar organic signal — hippy dad meets the axolotl",
        "OddOscar warmth maps to OHM COMMUNE macro — the axolotl joins the circle",
        "OHM drone root modulates morph_ oscillator pitch — communal tone as organic base",
    ],
    "ORBITAL": [
        "ORBITAL additive partials spread through OddOscar filter — harmonic warmth, organic body",
        "OddOscar envelope attack modulates ORBITAL voice balance — organic timing shapes harmony",
        "ORBITAL group envelope follows morph_ breath rate — the choir breathes with the axolotl",
    ],
    "OWLFISH": [
        "OWLFISH Mixtur-Trautonium subharmonics feed OddOscar warmth bus — deep tones warm the gill",
        "OddOscar organic movement modulates OWLFISH filter depth — axolotl motion stirs the abyss",
        "OWLFISH abyssal gold accent harmonises with morph_ timbre — depth meets organic warmth",
    ],
    "ORACLE": [
        "ORACLE stochastic resonance warms OddOscar organic signal — prophecy breathes with the axolotl",
        "OddOscar envelope swell modulates ORACLE breakpoint timing — organic breath shapes the prophecy",
        "ORACLE harmonic series maps to morph_ filter sweep — divination colours the warm body",
    ],
}

# ---------------------------------------------------------------------------
# Parameter stubs
# ---------------------------------------------------------------------------
def _snap_params(rng: random.Random) -> dict:
    """OddfeliX (snap_) parameter stubs — feliX clinical/bright character."""
    return {
        "snap_filterCutoff":   round(rng.uniform(0.55, 0.95), 3),
        "snap_filterReso":     round(rng.uniform(0.1,  0.5),  3),
        "snap_oscWave":        rng.choice([0, 1, 2, 3]),
        "snap_oscDetune":      round(rng.uniform(-0.1, 0.1),  3),
        "snap_ampAttack":      round(rng.uniform(0.001, 0.05), 3),
        "snap_ampDecay":       round(rng.uniform(0.05, 0.3),  3),
        "snap_ampSustain":     round(rng.uniform(0.5,  0.85), 3),
        "snap_ampRelease":     round(rng.uniform(0.05, 0.4),  3),
        "snap_envAttack":      round(rng.uniform(0.001, 0.05), 3),
        "snap_envAmount":      round(rng.uniform(0.3,  0.75), 3),
        "snap_lfoRate":        round(rng.uniform(0.1,  8.0),  3),
        "snap_lfoDepth":       round(rng.uniform(0.1,  0.5),  3),
        "snap_brightness":     round(rng.uniform(0.7,  1.0),  3),
        "snap_outputLevel":    round(rng.uniform(0.65, 0.85), 3),
        "snap_couplingLevel":  round(rng.uniform(0.5,  0.85), 3),
        "snap_couplingBus":    0,
    }


def _morph_params(rng: random.Random) -> dict:
    """OddOscar (morph_) parameter stubs — Oscar warm/organic character."""
    return {
        "morph_morph":         round(rng.uniform(0.3,  0.75), 3),
        "morph_filterCutoff":  round(rng.uniform(0.3,  0.7),  3),
        "morph_filterReso":    round(rng.uniform(0.05, 0.4),  3),
        "morph_warmth":        round(rng.uniform(0.6,  0.95), 3),
        "morph_ampAttack":     round(rng.uniform(0.02, 0.2),  3),
        "morph_ampDecay":      round(rng.uniform(0.1,  0.5),  3),
        "morph_ampSustain":    round(rng.uniform(0.5,  0.9),  3),
        "morph_ampRelease":    round(rng.uniform(0.15, 0.8),  3),
        "morph_lfoRate":       round(rng.uniform(0.01, 3.0),  3),
        "morph_lfoDepth":      round(rng.uniform(0.15, 0.6),  3),
        "morph_depth":         round(rng.uniform(0.4,  0.85), 3),
        "morph_organicDrift":  round(rng.uniform(0.2,  0.7),  3),
        "morph_outputLevel":   round(rng.uniform(0.65, 0.85), 3),
        "morph_couplingLevel": round(rng.uniform(0.5,  0.85), 3),
        "morph_couplingBus":   0,
    }


# ---------------------------------------------------------------------------
# DNA blend helper
# ---------------------------------------------------------------------------
def _blend(dna_a: dict, dna_b: dict, weight_a: float = 0.5) -> dict:
    weight_b = 1.0 - weight_a
    keys = set(dna_a) | set(dna_b)
    return {
        k: round(dna_a.get(k, 0.5) * weight_a + dna_b.get(k, 0.5) * weight_b, 3)
        for k in keys
    }


# ---------------------------------------------------------------------------
# Preset factory
# ---------------------------------------------------------------------------
def make_preset(
    name: str,
    engine_a: str,
    engine_b: str,
    dna_a: dict,
    dna_b: dict,
    coupling_type: str,
    rng: random.Random,
    description: str = "",
    tags=None,
) -> dict:
    blended = _blend(dna_a, dna_b, weight_a=0.5)
    coupling_amount = round(rng.uniform(0.5, 0.88), 3)
    intensity = "Deep" if coupling_amount >= 0.7 else "Moderate"

    params: dict = {}
    if engine_a == "ODDFELIX" or engine_b == "ODDFELIX":
        params["ODDFELIX"] = _snap_params(rng)
    if engine_a == "ODDOSCAR" or engine_b == "ODDOSCAR":
        params["ODDOSCAR"] = _morph_params(rng)

    preset_tags = ["coupling", "entangled",
                   engine_a.lower(), engine_b.lower()] + (tags or [])

    return {
        "schema_version": 1,
        "name": name,
        "mood": "Entangled",
        "engines": [engine_a, engine_b],
        "author": "XO_OX",
        "version": "1.0.0",
        "description": description or f"{engine_a} × {engine_b} coupling — {coupling_type}",
        "tags": preset_tags,
        "macroLabels": ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"],
        "couplingIntensity": intensity,
        "dna": blended,
        "sonic_dna": blended,
        "parameters": params,
        "coupling": {
            "pairs": [
                {
                    "engineA": engine_a,
                    "engineB": engine_b,
                    "type": coupling_type,
                    "amount": coupling_amount,
                }
            ]
        },
    }


# ---------------------------------------------------------------------------
# Name generation helpers
# ---------------------------------------------------------------------------
def _felix_partner_name(felix_word: str, partner_name: str, idx: int) -> str:
    """Produce a 2-3 word preset name for an OddfeliX × partner pair."""
    suffixes = ["Trace", "Cut", "Grid", "Frame", "Edge", "Line", "Map"]
    suffix = suffixes[idx % len(suffixes)]
    return f"{felix_word} {suffix}"


def _oscar_partner_name(oscar_word: str, partner_name: str, idx: int) -> str:
    """Produce a 2-3 word preset name for an OddOscar × partner pair."""
    suffixes = ["Flow", "Bloom", "Touch", "Breath", "Glow", "Wave", "Rise"]
    suffix = suffixes[idx % len(suffixes)]
    return f"{oscar_word} {suffix}"


# ---------------------------------------------------------------------------
# Build all presets
# ---------------------------------------------------------------------------
def build_presets(rng: random.Random, external_count: int = 3) -> list[dict]:
    presets: list[dict] = []

    # ------------------------------------------------------------------
    # 1. Marquee: ODDFELIX × ODDOSCAR — 8 founding axis presets
    # ------------------------------------------------------------------
    marquee_couplings = COUPLING_TYPES * 2  # enough for 8
    for i, mname in enumerate(MARQUEE_NAMES):
        coupling_type = marquee_couplings[i % len(COUPLING_TYPES)]
        preset = make_preset(
            name=mname,
            engine_a="ODDFELIX",
            engine_b="ODDOSCAR",
            dna_a=ODDFELIX_DNA,
            dna_b=ODDOSCAR_DNA,
            coupling_type=coupling_type,
            rng=rng,
            description=MARQUEE_DESCS[i],
            tags=["oddfelix", "oddoscar", "marquee", "founding-axis",
                  "felix-oscar", "neon-tetra", "axolotl"],
        )
        presets.append(preset)

    # ------------------------------------------------------------------
    # 2. OddfeliX external pairs — 9 partners × external_count each
    # ------------------------------------------------------------------
    felix_partners = [
        "ORACLE", "OVERWORLD", "ORIGAMI", "OBLIQUE",
        "OPTIC", "ONSET", "OBSIDIAN", "OUTWIT", "OPAL",
    ]
    felix_word_pool = ODDFELIX_WORDS * 4  # ensure enough words

    for partner in felix_partners:
        partner_dna = PARTNER_DNA[partner]
        descs = ODDFELIX_DESCS[partner]
        coupling_choices = rng.sample(COUPLING_TYPES, k=min(external_count, len(COUPLING_TYPES)))
        words = rng.sample(felix_word_pool, k=external_count)
        for i in range(external_count):
            coupling_type = coupling_choices[i % len(coupling_choices)]
            preset_name = _felix_partner_name(words[i], partner, i)
            preset = make_preset(
                name=preset_name,
                engine_a="ODDFELIX",
                engine_b=partner,
                dna_a=ODDFELIX_DNA,
                dna_b=partner_dna,
                coupling_type=coupling_type,
                rng=rng,
                description=descs[i % len(descs)],
                tags=["oddfelix", partner.lower(), "neon-tetra", "snap"],
            )
            presets.append(preset)

    # ------------------------------------------------------------------
    # 3. OddOscar external pairs — 9 partners × external_count each
    # ------------------------------------------------------------------
    oscar_partners = [
        "OVERDUB", "OPAL", "ORGANON", "OCEANIC",
        "ORACLE", "OBBLIGATO", "OHM", "ORBITAL", "OWLFISH",
    ]
    oscar_word_pool = ODDOSCAR_WORDS * 4

    for partner in oscar_partners:
        partner_dna = PARTNER_DNA[partner]
        descs = ODDOSCAR_DESCS[partner]
        coupling_choices = rng.sample(COUPLING_TYPES, k=min(external_count, len(COUPLING_TYPES)))
        words = rng.sample(oscar_word_pool, k=external_count)
        for i in range(external_count):
            coupling_type = coupling_choices[i % len(coupling_choices)]
            preset_name = _oscar_partner_name(words[i], partner, i)
            preset = make_preset(
                name=preset_name,
                engine_a="ODDOSCAR",
                engine_b=partner,
                dna_a=ODDOSCAR_DNA,
                dna_b=partner_dna,
                coupling_type=coupling_type,
                rng=rng,
                description=descs[i % len(descs)],
                tags=["oddoscar", partner.lower(), "axolotl", "morph"],
            )
            presets.append(preset)

    return presets


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------
def main() -> None:
    parser = argparse.ArgumentParser(
        description="Generate OddfeliX + OddOscar coupling presets for XOlokun.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    repo_root = Path(__file__).resolve().parent.parent
    default_out = repo_root / "Presets" / "XOlokun" / "Entangled"

    parser.add_argument(
        "--output-dir",
        type=Path,
        default=default_out,
        help=f"Output directory (default: {default_out})",
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Print preset names without writing files",
    )
    parser.add_argument(
        "--seed",
        type=int,
        default=42,
        help="Random seed for reproducibility (default: 42)",
    )
    parser.add_argument(
        "--count",
        type=int,
        default=3,
        help="Presets per external partner pair (default: 3; marquee is always 8)",
    )
    args = parser.parse_args()

    rng = random.Random(args.seed)
    presets = build_presets(rng, external_count=args.count)

    marquee_count = sum(
        1 for p in presets
        if "ODDFELIX" in p["engines"] and "ODDOSCAR" in p["engines"]
    )
    felix_count = sum(
        1 for p in presets
        if "ODDFELIX" in p["engines"] and "ODDOSCAR" not in p["engines"]
    )
    oscar_count = sum(
        1 for p in presets
        if "ODDOSCAR" in p["engines"] and "ODDFELIX" not in p["engines"]
    )

    if args.dry_run:
        print(f"[dry-run] Would write {len(presets)} presets to: {args.output_dir}")
        print(f"  Marquee (ODDFELIX × ODDOSCAR) : {marquee_count}")
        print(f"  ODDFELIX external pairs        : {felix_count}")
        print(f"  ODDOSCAR external pairs        : {oscar_count}")
        print()
        for p in presets:
            engines = " × ".join(p["engines"])
            ctype = p["coupling"]["pairs"][0]["type"]
            print(f"  {p['name']:<35}  [{engines}]  {ctype}")
        return

    args.output_dir.mkdir(parents=True, exist_ok=True)
    written = 0
    skipped = 0
    for preset in presets:
        filename = preset["name"] + ".xometa"
        filepath = args.output_dir / filename
        if filepath.exists():
            skipped += 1
            continue
        with open(filepath, "w", encoding="utf-8") as fh:
            json.dump(preset, fh, indent=2)
            fh.write("\n")
        written += 1

    print(f"ODDFELIX × ODDOSCAR marquee    : {marquee_count}")
    print(f"ODDFELIX external pairs        : {felix_count}")
    print(f"ODDOSCAR external pairs        : {oscar_count}")
    print(f"Total generated                : {len(presets)}")
    print(f"Written                        : {written}")
    print(f"Skipped (already exist)        : {skipped}")
    print(f"Output                         : {args.output_dir}")


if __name__ == "__main__":
    main()
