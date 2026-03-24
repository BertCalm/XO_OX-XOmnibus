#!/usr/bin/env python3
"""Generate OVERDUB and OPAL coupling presets for XOlokun.

Seeds Entangled coupling coverage for two rich "texture" engines:
  - OVERDUB  (tape/dub/spring reverb,   dub_  prefix, Olive #6B7B3A)
  - OPAL     (granular synthesis,        opal_ prefix, Lavender #A78BFA)

OVERDUB→OPAL is a key coupling in the XOlokun design spec.

Output: 66 presets total
  - OVERDUB + OPAL marquee pair                  =  6
  - OVERDUB × 10 external partners × 3 presets  = 30
  - OPAL    × 10 external partners × 3 presets  = 30
"""

import argparse
import json
import os
import random
from pathlib import Path

# ---------------------------------------------------------------------------
# DNA baselines
# ---------------------------------------------------------------------------

OVERDUB_DNA = dict(brightness=0.45, warmth=0.7,  movement=0.55,
                   density=0.55,    space=0.75,   aggression=0.25)

OPAL_DNA    = dict(brightness=0.7,  warmth=0.5,  movement=0.75,
                   density=0.45,    space=0.8,   aggression=0.2)

PARTNER_DNA: dict[str, dict] = {
    "OMBRE":   dict(brightness=0.45, warmth=0.6,  movement=0.55, density=0.6,  space=0.65, aggression=0.3),
    "ORCA":    dict(brightness=0.3,  warmth=0.35, movement=0.7,  density=0.7,  space=0.5,  aggression=0.75),
    "OCTOPUS": dict(brightness=0.7,  warmth=0.4,  movement=0.85, density=0.6,  space=0.5,  aggression=0.55),
    "OVERLAP": dict(brightness=0.65, warmth=0.55, movement=0.6,  density=0.7,  space=0.85, aggression=0.25),
    "OUTWIT":  dict(brightness=0.6,  warmth=0.45, movement=0.8,  density=0.65, space=0.55, aggression=0.5),
    "OHM":     dict(brightness=0.45, warmth=0.75, movement=0.55, density=0.6,  space=0.7,  aggression=0.3),
    "ORPHICA": dict(brightness=0.8,  warmth=0.5,  movement=0.7,  density=0.45, space=0.75, aggression=0.25),
    "ORGANON": dict(brightness=0.6,  warmth=0.6,  movement=0.65, density=0.75, space=0.65, aggression=0.4),
    "OBESE":   dict(brightness=0.5,  warmth=0.7,  movement=0.45, density=0.8,  space=0.4,  aggression=0.75),
    "OSTERIA": dict(brightness=0.4,  warmth=0.75, movement=0.45, density=0.7,  space=0.6,  aggression=0.45),
    "ORACLE":  dict(brightness=0.5,  warmth=0.4,  movement=0.6,  density=0.7,  space=0.7,  aggression=0.4),
    "ONSET":   dict(brightness=0.55, warmth=0.5,  movement=0.8,  density=0.75, space=0.5,  aggression=0.7),
    "OBLONG":  dict(brightness=0.6,  warmth=0.65, movement=0.5,  density=0.65, space=0.55, aggression=0.5),
    "ORIGAMI": dict(brightness=0.7,  warmth=0.45, movement=0.65, density=0.55, space=0.55, aggression=0.55),
}

COUPLING_TYPES = ["SYNC_LFO", "FILTER_MOD", "AMP_MOD", "PITCH_MOD"]

# ---------------------------------------------------------------------------
# Name vocabularies
# ---------------------------------------------------------------------------

OVERDUB_WORDS = ["Tape", "Dub", "Spring", "Olive", "Delay",
                 "Saturate", "Wow", "Flutter", "Analog", "Warmth"]
OPAL_WORDS    = ["Grain", "Scatter", "Cloud", "Granular", "Lavender",
                 "Fragment", "Drift", "Texture", "Shimmer", "Disperse"]
MARQUEE_NAMES = [
    "Tape Granules",
    "Dub Opal",
    "Spring Grain",
    "Warm Scatter",
    "Olive Lavender",
    "Tape Cloud",
]

# ---------------------------------------------------------------------------
# Descriptions
# ---------------------------------------------------------------------------

OVERDUB_DESCS: dict[str, list[str]] = {
    "OMBRE": [
        "Dub saturation bleeds into OMBRE dual-narrative blend — memory layered on tape hiss",
        "OMBRE perception axis modulates OVERDUB wow depth — forgetting warps the tape",
        "OVERDUB spring reverb tail extends OMBRE shadow mauve space — warm echo in the fade",
    ],
    "ORCA": [
        "ORCA hunt velocity slams OVERDUB saturator into red — apex predator drives the dub",
        "OVERDUB tape delay lag tracks ORCA echolocation timing — the echo hunts with the whale",
        "ORCA breach transient feeds OVERDUB spring splash — impact into metallic reverb bloom",
    ],
    "OCTOPUS": [
        "OCTOPUS arm modulation randomises OVERDUB delay time — chromatophore chaos on tape",
        "OVERDUB wow depth follows OCTOPUS ink cloud density — murk warps the signal",
        "OCTOPUS adaptive timbre triggers OVERDUB flutter — alien motion, analog tremor",
    ],
    "OVERLAP": [
        "OVERLAP FDN tail bleeds into OVERDUB send return — two reverb worlds in conversation",
        "OVERDUB spring decay rate modulates OVERLAP diffusion — splash and spread entangled",
        "OVERLAP mod depth tracks OVERDUB saturation — harmonic bloom from analog warmth",
    ],
    "OUTWIT": [
        "OUTWIT Wolfram cell generation clocks OVERDUB delay time — pattern becomes echo grid",
        "OVERDUB flutter rate follows OUTWIT evolve speed — tape instability meets cellular time",
        "OUTWIT arm count maps to OVERDUB multi-tap delay voices — 8 arms, 8 echoes",
    ],
    "OHM": [
        "OHM commune depth swells OVERDUB spring reverb size — hippie resonance in the tank",
        "OVERDUB tape warmth amplifies OHM meddling macro — saturation nourishes the drone",
        "OHM sage warmth and OVERDUB olive warmth merge — two earth tones, one deep field",
    ],
    "ORPHICA": [
        "ORPHICA harp scatter feeds OVERDUB delay subdivision — plucked fragments multiply in time",
        "OVERDUB spring bloom follows ORPHICA brightness sweep — metal and light intertwined",
        "ORPHICA siphonophore motion modulates OVERDUB wow — colonial pulse, tape tremor",
    ],
    "ORGANON": [
        "ORGANON pipe resonance feeds OVERDUB spring tank — the cathedral reverb doubled",
        "OVERDUB tape saturation compresses ORGANON metabolic output — warm tube on the organ",
        "ORGANON registration depth modulates OVERDUB delay feedback — dense stops, dense echoes",
    ],
    "OBESE": [
        "OBESE mojo saturation and OVERDUB tape drive stack — analog heat on analog heat",
        "OVERDUB delay smears OBESE sub-bass attack — transient dissolved in echo",
        "OBESE density weight grounds OVERDUB spring tail — fat low end anchors the reverb",
    ],
    "OSTERIA": [
        "OSTERIA porto wine warmth soaks OVERDUB delay signal — table wine through tape",
        "OVERDUB spring reverb adds spatial depth to OSTERIA qBass shore — warmth across the bay",
        "OSTERIA cultural rhythm patterns gate OVERDUB delay repeats — the meal has a tempo",
    ],
}

OPAL_DESCS: dict[str, list[str]] = {
    "OMBRE": [
        "OMBRE memory-fade axis modulates OPAL grain decay — forgetting at the grain level",
        "OPAL scatter density mirrors OMBRE blend ratio — the more fragments, the deeper the fade",
        "OMBRE dual-narrative feeds two OPAL grain streams — two stories, two cloud layers",
    ],
    "ORCA": [
        "ORCA echolocation frequency seeds OPAL grain pitch spread — sonar becomes granular texture",
        "OPAL cloud density tracks ORCA hunt intensity — more grains when the predator accelerates",
        "ORCA breach impulse scatters OPAL grain positions — surface break shatters the cloud",
    ],
    "OCTOPUS": [
        "OCTOPUS chromatophore modulator drives OPAL grain brightness sweep — colour as grain size",
        "OPAL grain density mirrors OCTOPUS arm activity count — 8 arms, grain cloud depth",
        "OCTOPUS ink cloud triggers OPAL granular density surge — obscure and saturate",
    ],
    "OVERLAP": [
        "OPAL grain scatter feeds OVERLAP FDN node inputs — fragments propagate through the web",
        "OVERLAP diffusion rate shapes OPAL grain position randomness — the wider the web, the looser the cloud",
        "OPAL cloud position modulates OVERLAP pre-delay — grain density sculpts reverb onset",
    ],
    "ORACLE": [
        "ORACLE harmonic series seeds OPAL grain pitch quantisation — prophecy shapes the cloud",
        "OPAL grain rhythm follows ORACLE GENDY stochastic timing — stochastic grains, stochastic time",
        "ORACLE resonance filter tilt modulates OPAL brightness — spectral prophecy colours the scatter",
    ],
    "OHM": [
        "OHM commune swell modulates OPAL grain size — collective breathing expands the cloud",
        "OPAL lavender scatter enriches OHM sage drone — two soft textures in sympathy",
        "OHM MEDDLING macro depth controls OPAL grain overlap — more meddling, denser fragments",
    ],
    "ORPHICA": [
        "ORPHICA microsound harp and OPAL grain share the same spectral register — twin cloud engines",
        "OPAL grain density increases with ORPHICA pluck brightness — light excites the cloud",
        "ORPHICA siphonophore colony modulates OPAL grain colony size — two swarm intelligences",
    ],
    "ONSET": [
        "ONSET drum transients trigger OPAL grain scatter bursts — every hit seeds a cloud",
        "OPAL cloud density decays in step with ONSET drum release — grain tail after the hit",
        "ONSET perc voice pitch feeds OPAL grain pitch centre — percussion becomes texture",
    ],
    "OBLONG": [
        "OBLONG amber warmth enriches OPAL grain spectral content — warm scatter from warm source",
        "OPAL grain movement rate follows OBLONG filter envelope — texture breathes with the melody",
        "OBLONG bob filter cutoff modulates OPAL cloud brightness — filter sweep shapes the cloud",
    ],
    "ORIGAMI": [
        "ORIGAMI fold geometry maps to OPAL grain position grid — crease determines scatter pattern",
        "OPAL cloud density increases with ORIGAMI fold count — more folds, more fragments",
        "ORIGAMI carrier frequency seeds OPAL grain pitch centre — one fold, one cloud pitch",
    ],
}

MARQUEE_DESCS = [
    "Tape saturation smears granular clouds — OVERDUB wow modulates OPAL grain scatter in real time",
    "Spring reverb tail becomes a grain source — OPAL fragments the OVERDUB metallic splash",
    "OVERDUB delay grid seeds OPAL grain timing — echo position becomes grain position",
    "OPAL density feedback drives OVERDUB saturation harder — the denser the cloud, the warmer the tape",
    "Dub-delay groove and granular drift locked to same LFO — SYNC_LFO coupling at its warmest",
    "Olive warmth and lavender shimmer merge into a unified texture engine — the definitive pairing",
]

# ---------------------------------------------------------------------------
# Parameter stubs
# ---------------------------------------------------------------------------

def _overdub_params(rng: random.Random) -> dict:
    return {
        "dub_oscWave":       rng.randint(0, 3),
        "dub_filterCutoff":  round(rng.uniform(0.35, 0.75), 3),
        "dub_filterReso":    round(rng.uniform(0.1,  0.4),  3),
        "dub_sendAmount":    round(rng.uniform(0.45, 0.85), 3),
        "dub_tapeSpeed":     round(rng.uniform(0.3,  0.7),  3),
        "dub_wowDepth":      round(rng.uniform(0.05, 0.35), 3),
        "dub_flutterRate":   round(rng.uniform(0.05, 0.3),  3),
        "dub_satDrive":      round(rng.uniform(0.3,  0.7),  3),
        "dub_delayTime":     round(rng.uniform(0.1,  0.6),  3),
        "dub_delayFeedback": round(rng.uniform(0.2,  0.65), 3),
        "dub_springMix":     round(rng.uniform(0.3,  0.75), 3),
        "dub_springDecay":   round(rng.uniform(0.4,  0.85), 3),
        "dub_outputLevel":   round(rng.uniform(0.65, 0.85), 3),
        "dub_couplingLevel": round(rng.uniform(0.5,  0.8),  3),
        "dub_couplingBus":   0,
    }


def _opal_params(rng: random.Random) -> dict:
    return {
        "opal_grainSize":    round(rng.uniform(0.1,  0.55), 3),
        "opal_grainDensity": round(rng.uniform(0.35, 0.75), 3),
        "opal_scatter":      round(rng.uniform(0.2,  0.65), 3),
        "opal_pitchSpread":  round(rng.uniform(0.0,  0.4),  3),
        "opal_cloudMix":     round(rng.uniform(0.4,  0.8),  3),
        "opal_position":     round(rng.uniform(0.0,  1.0),  3),
        "opal_positionRand": round(rng.uniform(0.1,  0.5),  3),
        "opal_grainAttack":  round(rng.uniform(0.01, 0.2),  3),
        "opal_grainRelease": round(rng.uniform(0.05, 0.35), 3),
        "opal_filterCutoff": round(rng.uniform(0.4,  0.85), 3),
        "opal_filterReso":   round(rng.uniform(0.1,  0.4),  3),
        "opal_driftRate":    round(rng.uniform(0.05, 0.45), 3),
        "opal_outputLevel":  round(rng.uniform(0.65, 0.85), 3),
        "opal_couplingLevel":round(rng.uniform(0.5,  0.8),  3),
        "opal_couplingBus":  0,
    }


# ---------------------------------------------------------------------------
# DNA blend helper
# ---------------------------------------------------------------------------

def _blend(dna_a: dict, dna_b: dict, weight_a: float = 0.5) -> dict:
    weight_b = 1.0 - weight_a
    keys = set(dna_a) | set(dna_b)
    return {
        k: round(dna_a.get(k, 0.5) * weight_a + dna_b.get(k, 0.5) * weight_b, 3)
        for k in sorted(keys)
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
    count: int = 1,
) -> dict:
    blended = _blend(dna_a, dna_b, weight_a=0.5)
    coupling_amount = round(rng.uniform(0.5, 0.85), 3)
    intensity = "Deep" if coupling_amount >= 0.7 else "Moderate"

    params: dict = {}
    if "OVERDUB" in (engine_a, engine_b):
        params["OVERDUB"] = _overdub_params(rng)
    if "OPAL" in (engine_a, engine_b):
        params["OPAL"] = _opal_params(rng)

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

def _pair_name(word_pool: list[str], idx: int) -> str:
    """Return a two-word name from pool using sequential pairs."""
    a = word_pool[(idx * 2) % len(word_pool)]
    b = word_pool[(idx * 2 + 1) % len(word_pool)]
    return f"{a} {b}"


# ---------------------------------------------------------------------------
# Build all presets
# ---------------------------------------------------------------------------

def build_presets(rng: random.Random, count: int = 3) -> list[dict]:
    presets: list[dict] = []

    # --- OVERDUB × 10 external partners ---
    overdub_partners = [
        "OMBRE", "ORCA", "OCTOPUS", "OVERLAP", "OUTWIT",
        "OHM", "ORPHICA", "ORGANON", "OBESE", "OSTERIA",
    ]
    for partner in overdub_partners:
        partner_dna = PARTNER_DNA[partner]
        descs = OVERDUB_DESCS[partner]
        coupling_choices = rng.sample(COUPLING_TYPES, k=min(count, len(COUPLING_TYPES)))
        word_pool = rng.sample(OVERDUB_WORDS, k=min(count * 2, len(OVERDUB_WORDS)))
        for i in range(count):
            preset_name = _pair_name(word_pool, i)
            coupling_type = coupling_choices[i % len(coupling_choices)]
            desc_idx = i % len(descs)
            preset = make_preset(
                name=preset_name,
                engine_a="OVERDUB",
                engine_b=partner,
                dna_a=OVERDUB_DNA,
                dna_b=partner_dna,
                coupling_type=coupling_type,
                rng=rng,
                description=descs[desc_idx],
                tags=[partner.lower(), "overdub", "tape", "dub", "spring"],
                count=count,
            )
            presets.append(preset)

    # --- OPAL × 10 external partners ---
    opal_partners = [
        "OMBRE", "ORCA", "OCTOPUS", "OVERLAP", "ORACLE",
        "OHM", "ORPHICA", "ONSET", "OBLONG", "ORIGAMI",
    ]
    for partner in opal_partners:
        partner_dna = PARTNER_DNA[partner]
        descs = OPAL_DESCS[partner]
        coupling_choices = rng.sample(COUPLING_TYPES, k=min(count, len(COUPLING_TYPES)))
        word_pool = rng.sample(OPAL_WORDS, k=min(count * 2, len(OPAL_WORDS)))
        for i in range(count):
            preset_name = _pair_name(word_pool, i)
            coupling_type = coupling_choices[i % len(coupling_choices)]
            desc_idx = i % len(descs)
            preset = make_preset(
                name=preset_name,
                engine_a="OPAL",
                engine_b=partner,
                dna_a=OPAL_DNA,
                dna_b=partner_dna,
                coupling_type=coupling_type,
                rng=rng,
                description=descs[desc_idx],
                tags=[partner.lower(), "opal", "granular", "grain", "cloud"],
                count=count,
            )
            presets.append(preset)

    # --- OVERDUB + OPAL marquee (6 presets) ---
    marquee_couplings = COUPLING_TYPES + ["AMP_MOD", "SYNC_LFO"]
    for i, mname in enumerate(MARQUEE_NAMES):
        coupling_type = marquee_couplings[i % len(COUPLING_TYPES)]
        preset = make_preset(
            name=mname,
            engine_a="OVERDUB",
            engine_b="OPAL",
            dna_a=OVERDUB_DNA,
            dna_b=OPAL_DNA,
            coupling_type=coupling_type,
            rng=rng,
            description=MARQUEE_DESCS[i],
            tags=["overdub", "opal", "marquee", "tape", "granular",
                  "texture", "key-coupling"],
        )
        presets.append(preset)

    return presets


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def main() -> None:
    parser = argparse.ArgumentParser(
        description="Generate OVERDUB + OPAL coupling presets for XOlokun."
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
        metavar="N",
        help="Presets per external partner pair (default: 3)",
    )
    args = parser.parse_args()

    rng = random.Random(args.seed)
    presets = build_presets(rng, count=args.count)

    total_overdub = sum(
        1 for p in presets if "OVERDUB" in p["engines"] and "OPAL" not in p["engines"]
    )
    total_opal = sum(
        1 for p in presets if "OPAL" in p["engines"] and "OVERDUB" not in p["engines"]
    )
    total_marquee = sum(
        1 for p in presets if "OVERDUB" in p["engines"] and "OPAL" in p["engines"]
    )

    if args.dry_run:
        print(f"[dry-run] Would write {len(presets)} presets to: {args.output_dir}")
        for p in presets:
            engines = " × ".join(p["engines"])
            ctype = p["coupling"]["pairs"][0]["type"]
            print(f"  {p['name']:<40}  [{engines}]  {ctype}")
        print()
        print(f"OVERDUB external presets : {total_overdub}")
        print(f"OPAL    external presets : {total_opal}")
        print(f"OVERDUB+OPAL marquee     : {total_marquee}")
        print(f"Total                    : {len(presets)}")
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

    print(f"OVERDUB external presets : {total_overdub}")
    print(f"OPAL    external presets : {total_opal}")
    print(f"OVERDUB+OPAL marquee     : {total_marquee}")
    print(f"Total generated          : {len(presets)}")
    print(f"Written                  : {written}")
    print(f"Skipped                  : {skipped}  (files already exist)")
    print(f"Output                   : {args.output_dir}")


if __name__ == "__main__":
    main()
