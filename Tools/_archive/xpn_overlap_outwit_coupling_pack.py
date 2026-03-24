#!/usr/bin/env python3
"""Generate OVERLAP and OUTWIT coupling presets for XOlokun.

Seeds Entangled coupling coverage for two newly installed engines:
  - OVERLAP  (Lion's Mane jellyfish FDN reverb,   olap_ prefix)
  - OUTWIT   (Giant Pacific Octopus 8-arm CA,      owit_ prefix)

Output: 53 presets total
  - OVERLAP  × 8 partner engines × 3 presets = 24
  - OUTWIT   × 8 partner engines × 3 presets = 24
  - OVERLAP  + OUTWIT marquee pair           =  5
"""

import argparse
import json
import os
import random
from pathlib import Path

# ---------------------------------------------------------------------------
# DNA baselines
# ---------------------------------------------------------------------------
OVERLAP_DNA = dict(
    brightness=0.65, warmth=0.55, movement=0.6,
    density=0.7,     space=0.85,  aggression=0.25,
)
OUTWIT_DNA = dict(
    brightness=0.6,  warmth=0.45, movement=0.8,
    density=0.65,    space=0.55,  aggression=0.5,
)

# Partner engine DNA approximations (kept deliberately sparse — only the
# dimensions that meaningfully differ from neutral 0.5 are nudged).
PARTNER_DNA: dict[str, dict] = {
    "OPAL":       dict(brightness=0.7,  warmth=0.5,  movement=0.65, density=0.55, space=0.8,  aggression=0.2),
    "ORACLE":     dict(brightness=0.55, warmth=0.6,  movement=0.5,  density=0.6,  space=0.7,  aggression=0.3),
    "OBSIDIAN":   dict(brightness=0.3,  warmth=0.35, movement=0.55, density=0.75, space=0.5,  aggression=0.7),
    "ODYSSEY":    dict(brightness=0.6,  warmth=0.65, movement=0.7,  density=0.5,  space=0.75, aggression=0.25),
    "OVERWORLD":  dict(brightness=0.7,  warmth=0.3,  movement=0.6,  density=0.45, space=0.35, aggression=0.45),
    "OCEANIC":    dict(brightness=0.5,  warmth=0.7,  movement=0.4,  density=0.6,  space=0.9,  aggression=0.15),
    "ORBITAL":    dict(brightness=0.65, warmth=0.4,  movement=0.5,  density=0.5,  space=0.65, aggression=0.35),
    "ORGANON":    dict(brightness=0.6,  warmth=0.55, movement=0.55, density=0.65, space=0.6,  aggression=0.3),
    "OUROBOROS":  dict(brightness=0.45, warmth=0.5,  movement=0.75, density=0.8,  space=0.5,  aggression=0.55),
    "ORIGAMI":    dict(brightness=0.6,  warmth=0.45, movement=0.7,  density=0.5,  space=0.6,  aggression=0.4),
    "OPTIC":      dict(brightness=0.8,  warmth=0.35, movement=0.65, density=0.45, space=0.5,  aggression=0.45),
    "OBLIQUE":    dict(brightness=0.5,  warmth=0.5,  movement=0.6,  density=0.55, space=0.55, aggression=0.5),
    "OCELOT":     dict(brightness=0.65, warmth=0.5,  movement=0.85, density=0.55, space=0.45, aggression=0.6),
    "OBBLIGATO":  dict(brightness=0.55, warmth=0.6,  movement=0.5,  density=0.6,  space=0.6,  aggression=0.3),
}

COUPLING_TYPES = ["SYNC_LFO", "FILTER_MOD", "AMP_MOD", "PITCH_MOD"]

# ---------------------------------------------------------------------------
# Name vocabularies
# ---------------------------------------------------------------------------
OVERLAP_WORDS  = ["Tangle", "Web", "Frond", "Diffuse", "Bloom",
                  "Spread", "Cascade", "Dissolve", "Propagate", "Trailing"]
OUTWIT_WORDS   = ["Rule", "Cell", "Arm", "Pattern", "Emerge",
                  "Evolve", "Branch", "Fracture", "Automata", "Signal"]
MARQUEE_NAMES  = [
    "The Grand Entanglement",
    "Complexity Bloom",
    "FDN Automata",
    "Cellular Reverb",
    "Web Pattern",
]

# ---------------------------------------------------------------------------
# Parameter stubs  (lightweight placeholders — values are musically plausible
# but are meant as starting points for sound designers, not final patches)
# ---------------------------------------------------------------------------
def _overlap_params(rng: random.Random) -> dict:
    return {
        "olap_size":         round(rng.uniform(0.55, 0.95), 3),
        "olap_decay":        round(rng.uniform(0.5,  0.9),  3),
        "olap_damping":      round(rng.uniform(0.3,  0.65), 3),
        "olap_diffusion":    round(rng.uniform(0.5,  0.85), 3),
        "olap_modDepth":     round(rng.uniform(0.1,  0.45), 3),
        "olap_modRate":      round(rng.uniform(0.05, 0.35), 3),
        "olap_preDelay":     round(rng.uniform(0.0,  0.25), 3),
        "olap_earlyMix":     round(rng.uniform(0.2,  0.5),  3),
        "olap_lateMix":      round(rng.uniform(0.4,  0.8),  3),
        "olap_filterCutoff": round(rng.uniform(0.4,  0.8),  3),
        "olap_filterReso":   round(rng.uniform(0.1,  0.4),  3),
        "olap_outputLevel":  round(rng.uniform(0.65, 0.85), 3),
        "olap_couplingLevel":round(rng.uniform(0.55, 0.8),  3),
        "olap_couplingBus":  0,
    }


def _outwit_params(rng: random.Random) -> dict:
    return {
        "owit_rule":         rng.choice([30, 90, 110, 184, 22, 54]),
        "owit_arms":         rng.randint(4, 8),
        "owit_cellSize":     round(rng.uniform(0.1,  0.5),  3),
        "owit_density":      round(rng.uniform(0.4,  0.75), 3),
        "owit_evolveRate":   round(rng.uniform(0.3,  0.7),  3),
        "owit_mutateProb":   round(rng.uniform(0.05, 0.3),  3),
        "owit_pitchSpread":  round(rng.uniform(0.2,  0.6),  3),
        "owit_filterCutoff": round(rng.uniform(0.35, 0.75), 3),
        "owit_filterReso":   round(rng.uniform(0.1,  0.45), 3),
        "owit_ampAttack":    round(rng.uniform(0.01, 0.15), 3),
        "owit_ampRelease":   round(rng.uniform(0.2,  0.8),  3),
        "owit_outputLevel":  round(rng.uniform(0.65, 0.85), 3),
        "owit_couplingLevel":round(rng.uniform(0.5,  0.8),  3),
        "owit_couplingBus":  0,
    }


# ---------------------------------------------------------------------------
# Blended DNA helper
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
    tags=None,  # list[str] | None
) -> dict:
    blended = _blend(dna_a, dna_b, weight_a=0.5)
    coupling_amount = round(rng.uniform(0.5, 0.85), 3)
    intensity = "Deep" if coupling_amount >= 0.7 else "Moderate"

    params: dict = {}
    if engine_a == "OVERLAP" or engine_b == "OVERLAP":
        params["OVERLAP"] = _overlap_params(rng)
    if engine_a == "OUTWIT" or engine_b == "OUTWIT":
        params["OUTWIT"] = _outwit_params(rng)

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
# Preset definitions
# ---------------------------------------------------------------------------

# Descriptions: (engine_pair_key) -> list of 3 description strings
OVERLAP_DESCS: dict[str, list[str]] = {
    "OPAL":      ["Granular clouds refract through Lion's Mane FDN — spectral shimmer meets deep space",
                  "OPAL grain density drives OVERLAP diffusion rate — the more fragments, the wider the web",
                  "OVERLAP decay envelopes OPAL grain pitch — sustained tones bloom outward"],
    "ORACLE":    ["Oracle resonance feeds OVERLAP early reflections — prophecy echoed in cavern walls",
                  "OVERLAP mod rate tracks ORACLE harmonic series — overtones become geometry",
                  "Oracle filter position modulates OVERLAP damping — spectral gate on infinite space"],
    "OBSIDIAN":  ["Obsidian aggression cuts through OVERLAP bloom — darkness fractures the reverb tail",
                  "OVERLAP diffusion softens OBSIDIAN transients — glass edges dissolve in water",
                  "OBSIDIAN wavefold depth extends OVERLAP modulation — sharp becomes diffuse"],
    "ODYSSEY":   ["Odyssey tidal swell expands OVERLAP room size — tide and tail rise together",
                  "OVERLAP trailing decay follows ODYSSEY phrase shape — the sea remembers",
                  "ODYSSEY modulation clock syncs OVERLAP mod rate — rhythmic bloom in open water"],
    "OVERWORLD": ["Overworld chip glitch triggers OVERLAP scatter bursts — pixels dissolve into reverb",
                  "OVERLAP bloom softens OVERWORLD ERA transitions — chip era changes wash through space",
                  "OVERWORLD pitch steps modulate OVERLAP pre-delay — staccato geometry meets diffuse tail"],
    "OCEANIC":   ["Two deep-water engines in resonance — OCEANIC pressure feeds OVERLAP space size",
                  "OVERLAP frond-spread follows OCEANIC current rate — slow drift, infinite room",
                  "OCEANIC depth zone index modulates OVERLAP damping — deeper zones, warmer tails"],
    "ORBITAL":   ["Orbital additive partials spread across OVERLAP FDN matrix — harmonic diffusion",
                  "OVERLAP cascade tail feeds ORBITAL timbre bus — reverb becomes a new oscillator",
                  "ORBITAL partial balance modulates OVERLAP early/late mix — spectral bloom control"],
    "ORGANON":   ["Organon pipe resonance propagates through OVERLAP web — the cathedral breathes",
                  "OVERLAP pre-delay follows ORGANON attack envelope — space opens after the pipe speaks",
                  "ORGANON registration depth modulates OVERLAP diffusion — dense stops, dense reflections"],
}

OUTWIT_DESCS: dict[str, list[str]] = {
    "ORACLE":    ["Oracle harmonic ratios seed OUTWIT Wolfram rules — prophecy becomes cellular law",
                  "OUTWIT arm pattern modulates ORACLE filter sweep — 8-arm intelligence shapes resonance",
                  "ORACLE resonance frequency maps to OUTWIT cell pitch spread — tone becomes pattern"],
    "OUROBOROS": ["Serpent cycle feeds OUTWIT generation clock — endless recursion meets endless arms",
                  "OUTWIT evolve rate tracks OUROBOROS phase position — pattern complexity rises with the coil",
                  "OUROBOROS loop length determines OUTWIT rule lifespan — structure within structure"],
    "ORIGAMI":   ["Origami fold depth increases OUTWIT branch count — geometry births geometry",
                  "OUTWIT cell fracture triggers ORIGAMI crease events — cellular collapse folds new shapes",
                  "ORIGAMI carrier frequency seeds OUTWIT initial row — one fold, infinite generations"],
    "OPTIC":     ["OPTIC waveshaper brightness maps to OUTWIT cell density — light becomes population",
                  "OUTWIT emergence events modulate OPTIC harmonic tilt — pattern shapes spectrum",
                  "OPTIC filter tracking drives OUTWIT pitch spread — spectral focus → cellular focus"],
    "OBLIQUE":   ["OBLIQUE angle offset modulates OUTWIT arm phase stagger — diagonal automata",
                  "OUTWIT signal output feeds OBLIQUE FM index — cellular chaos into oblique modulation",
                  "OBLIQUE rate generator clocks OUTWIT generation steps — skewed time, emergent rhythm"],
    "OVERWORLD": ["Overworld ERA state selects OUTWIT Wolfram rule set — chip era as cellular law",
                  "OUTWIT branch complexity drives OVERWORLD glitch rate — arms fracture the pixels",
                  "OVERWORLD noise register seeds OUTWIT initial pattern — retro chaos into cellular order"],
    "OCELOT":    ["Ocelot predator speed clocks OUTWIT evolve rate — hunter time becomes cellular time",
                  "OUTWIT pattern density modulates OCELOT aggression — complex cells, fierce output",
                  "OCELOT leap transient triggers OUTWIT rule mutation — the hunt reshapes the automata"],
    "OBBLIGATO": ["Obbligato dual winds breathe into OUTWIT cell birth/death rates — flute and oboe as life",
                  "OUTWIT arm count maps to OBBLIGATO voice balance — 8 arms, 2 voices, infinite shades",
                  "OBBLIGATO BOND macro depth modulates OUTWIT mutation probability — bond tightens order"],
}

MARQUEE_DESCS = [
    "The ultimate aquatic coupling: Lion's Mane FDN reverb meets Giant Pacific Octopus cellular automata — space and pattern entangled",
    "OUTWIT arm patterns feed OVERLAP FDN node routing — cellular intelligence chooses the reflection path",
    "OVERLAP decay length modulates OUTWIT generation rate — the longer the tail, the faster the evolution",
    "OUTWIT emergence events scatter OVERLAP diffusion coefficients — unpredictable bloom in infinite space",
    "OVERLAP frond-spread follows OUTWIT Wolfram frontier — the jellyfish traces the octopus's thought",
]


# ---------------------------------------------------------------------------
# Build all presets
# ---------------------------------------------------------------------------

def build_presets(rng: random.Random) -> list[dict]:
    presets: list[dict] = []

    # OVERLAP pairs
    overlap_partners = ["OPAL", "ORACLE", "OBSIDIAN", "ODYSSEY",
                        "OVERWORLD", "OCEANIC", "ORBITAL", "ORGANON"]
    for partner in overlap_partners:
        partner_dna = PARTNER_DNA[partner]
        descs = OVERLAP_DESCS[partner]
        coupling_choices = rng.sample(COUPLING_TYPES, k=min(3, len(COUPLING_TYPES)))
        # pick 3 words for this pair
        word_pool = rng.sample(OVERLAP_WORDS, k=6)
        for i in range(3):
            name_a = word_pool[i * 2]
            name_b = word_pool[i * 2 + 1]
            preset_name = f"{name_a} {name_b}"
            coupling_type = coupling_choices[i % len(coupling_choices)]
            preset = make_preset(
                name=preset_name,
                engine_a="OVERLAP",
                engine_b=partner,
                dna_a=OVERLAP_DNA,
                dna_b=partner_dna,
                coupling_type=coupling_type,
                rng=rng,
                description=descs[i],
                tags=[partner.lower(), "overlap", "fdn", "reverb"],
            )
            presets.append(preset)

    # OUTWIT pairs
    outwit_partners = ["ORACLE", "OUROBOROS", "ORIGAMI", "OPTIC",
                       "OBLIQUE", "OVERWORLD", "OCELOT", "OBBLIGATO"]
    for partner in outwit_partners:
        partner_dna = PARTNER_DNA[partner]
        descs = OUTWIT_DESCS[partner]
        coupling_choices = rng.sample(COUPLING_TYPES, k=min(3, len(COUPLING_TYPES)))
        word_pool = rng.sample(OUTWIT_WORDS, k=6)
        for i in range(3):
            name_a = word_pool[i * 2]
            name_b = word_pool[i * 2 + 1]
            preset_name = f"{name_a} {name_b}"
            coupling_type = coupling_choices[i % len(coupling_choices)]
            preset = make_preset(
                name=preset_name,
                engine_a="OUTWIT",
                engine_b=partner,
                dna_a=OUTWIT_DNA,
                dna_b=partner_dna,
                coupling_type=coupling_type,
                rng=rng,
                description=descs[i],
                tags=[partner.lower(), "outwit", "cellular-automata", "wolfram"],
            )
            presets.append(preset)

    # Marquee: OVERLAP + OUTWIT (5 presets)
    marquee_couplings = COUPLING_TYPES + ["FILTER_MOD"]  # 5 items
    for i, mname in enumerate(MARQUEE_NAMES):
        coupling_type = marquee_couplings[i % len(COUPLING_TYPES)]
        preset = make_preset(
            name=mname,
            engine_a="OVERLAP",
            engine_b="OUTWIT",
            dna_a=OVERLAP_DNA,
            dna_b=OUTWIT_DNA,
            coupling_type=coupling_type,
            rng=rng,
            description=MARQUEE_DESCS[i],
            tags=["overlap", "outwit", "marquee", "fdn", "wolfram", "cellular-automata"],
        )
        presets.append(preset)

    return presets


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def main() -> None:
    parser = argparse.ArgumentParser(
        description="Generate OVERLAP + OUTWIT coupling presets for XOlokun."
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
    args = parser.parse_args()

    rng = random.Random(args.seed)
    presets = build_presets(rng)

    if args.dry_run:
        print(f"[dry-run] Would write {len(presets)} presets to: {args.output_dir}")
        for p in presets:
            engines = " × ".join(p["engines"])
            ctype = p["coupling"]["pairs"][0]["type"]
            print(f"  {p['name']:<40}  [{engines}]  {ctype}")
        return

    args.output_dir.mkdir(parents=True, exist_ok=True)
    written = 0
    skipped = 0
    for preset in presets:
        filename = preset["name"] + ".xometa"
        filepath = args.output_dir / filename
        if filepath.exists():
            # Avoid clobbering hand-edited presets
            skipped += 1
            continue
        with open(filepath, "w", encoding="utf-8") as fh:
            json.dump(preset, fh, indent=2)
            fh.write("\n")
        written += 1

    overlap_count  = sum(1 for p in presets if "OVERLAP" in p["engines"] and "OUTWIT" not in p["engines"])
    outwit_count   = sum(1 for p in presets if "OUTWIT" in p["engines"] and "OVERLAP" not in p["engines"])
    marquee_count  = sum(1 for p in presets if "OVERLAP" in p["engines"] and "OUTWIT" in p["engines"])

    print(f"OVERLAP coupling presets : {overlap_count}")
    print(f"OUTWIT  coupling presets : {outwit_count}")
    print(f"OVERLAP+OUTWIT marquee   : {marquee_count}")
    print(f"Total generated          : {len(presets)}")
    print(f"Written  : {written}")
    print(f"Skipped  : {skipped}  (files already exist)")
    print(f"Output   : {args.output_dir}")


if __name__ == "__main__":
    main()
