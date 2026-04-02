#!/usr/bin/env python3
"""Generate OBLONG + ONSET coupling expansion presets for XOceanus.

Covers the rhythm backbone marquee pair plus new partner pairings for both
OBLONG (bob_ prefix, Amber) and ONSET (perc_ prefix, Electric Blue).

Total: 65 presets
  - 5  marquee OBLONG × ONSET
  - 30 OBLONG new pairs (10 partners × 3 presets each)
  - 30 ONSET  new pairs (10 partners × 3 presets each)
"""

import argparse
import json
import os
import random
import sys
from pathlib import Path

# ---------------------------------------------------------------------------
# Sonic DNA baselines
# ---------------------------------------------------------------------------
DNA = {
    "Oblong":     {"brightness": 0.60, "warmth": 0.65, "movement": 0.50, "density": 0.65, "space": 0.55, "aggression": 0.50},
    "Onset":      {"brightness": 0.55, "warmth": 0.50, "movement": 0.80, "density": 0.75, "space": 0.50, "aggression": 0.70},
    "Obbligato":  {"brightness": 0.55, "warmth": 0.65, "movement": 0.60, "density": 0.65, "space": 0.60, "aggression": 0.45},
    "Ohm":        {"brightness": 0.45, "warmth": 0.75, "movement": 0.55, "density": 0.60, "space": 0.70, "aggression": 0.30},
    "Orphica":    {"brightness": 0.80, "warmth": 0.50, "movement": 0.70, "density": 0.45, "space": 0.75, "aggression": 0.25},
    "Ottoni":     {"brightness": 0.60, "warmth": 0.60, "movement": 0.50, "density": 0.70, "space": 0.55, "aggression": 0.60},
    "Ole":        {"brightness": 0.65, "warmth": 0.70, "movement": 0.75, "density": 0.60, "space": 0.60, "aggression": 0.55},
    "Ombre":      {"brightness": 0.45, "warmth": 0.60, "movement": 0.55, "density": 0.60, "space": 0.65, "aggression": 0.30},
    "Orca":       {"brightness": 0.30, "warmth": 0.35, "movement": 0.70, "density": 0.70, "space": 0.50, "aggression": 0.75},
    "Octopus":    {"brightness": 0.70, "warmth": 0.40, "movement": 0.85, "density": 0.60, "space": 0.50, "aggression": 0.55},
    "Overlap":    {"brightness": 0.65, "warmth": 0.55, "movement": 0.60, "density": 0.70, "space": 0.85, "aggression": 0.25},
    "Outwit":     {"brightness": 0.60, "warmth": 0.45, "movement": 0.80, "density": 0.65, "space": 0.55, "aggression": 0.50},
    "Oracle":     {"brightness": 0.50, "warmth": 0.40, "movement": 0.60, "density": 0.70, "space": 0.70, "aggression": 0.40},
}

# ---------------------------------------------------------------------------
# Vocabulary pools
# ---------------------------------------------------------------------------
OBLONG_VOCAB = ["Foundation", "Amber", "Grounding", "Anchor", "Bass",
                "Foundational", "Warm Bottom", "Core"]
ONSET_VOCAB  = ["Kick", "Snare", "Impact", "Percussion", "Machine",
                "Punch", "Attack", "Transient"]

MARQUEE_NAMES = [
    "The Rhythm Backbone",
    "Foundation Beat",
    "Amber Pulse",
    "Drum and Bass",
    "The Engine Room",
]

# Coupling types available in MegaCouplingMatrix
COUPLING_TYPES = [
    "Amp->Filter",
    "Env->Pitch",
    "Env->Morph",
    "LFO->Filter",
    "LFO->Amp",
    "Env->LFO",
    "Chaos->Pitch",
    "Audio->Wavetable",
    "Velocity->Cutoff",
    "Gate->Env",
    "Pitch->Morph",
    "Amp->Pitch",
]


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------
def blend_dna(engine_a: str, engine_b: str, weight_a: float = 0.5) -> dict:
    """Linear blend of two engine DNA baselines, clamped to [0, 1]."""
    weight_b = 1.0 - weight_a
    a = DNA[engine_a]
    b = DNA[engine_b]
    return {
        k: round(min(1.0, max(0.0, a[k] * weight_a + b[k] * weight_b)), 3)
        for k in a
    }


def coupling_intensity(amount: float) -> str:
    if amount >= 0.75:
        return "Deep"
    if amount >= 0.45:
        return "Moderate"
    return "Light"


def safe_filename(name: str) -> str:
    """Sanitize preset name for use as a filename."""
    return name.replace("/", "-").replace("\\", "-")


def make_preset(
    name: str,
    desc: str,
    tags: list,
    engine_a: str,
    engine_b: str,
    coupling_type: str,
    coupling_amount: float,
    dna: dict,
) -> dict:
    return {
        "schema_version": 1,
        "name": name,
        "mood": "Entangled",
        "engines": [engine_a, engine_b],
        "author": "XO_OX Designs",
        "version": "1.0.0",
        "description": desc,
        "tags": sorted(set(["coupling", "entangled"] + tags)),
        "macroLabels": ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"],
        "couplingIntensity": coupling_intensity(coupling_amount),
        "tempo": None,
        "dna": dna,
        "parameters": {},
        "coupling": {
            "pairs": [{
                "engineA": engine_a,
                "engineB": engine_b,
                "type": coupling_type,
                "amount": round(coupling_amount, 3),
            }]
        },
        "sequencer": None,
    }


def pick(pool: list):
    return random.choice(pool)


# ---------------------------------------------------------------------------
# Preset definitions
# ---------------------------------------------------------------------------

def build_marquee_presets() -> list:
    """5 OBLONG × ONSET marquee presets — the rhythm backbone."""
    specs = [
        (
            MARQUEE_NAMES[0],
            "Kick transient from ONSET anchors BOB's foundational bass — the indivisible rhythm core.",
            ["oblong", "onset", "bass", "kick", "rhythm"],
            "Onset", "Oblong", "Amp->Filter", 0.78,
        ),
        (
            MARQUEE_NAMES[1],
            "Snare envelope opens BOB's filter on every backbeat — warmth breathing with the grid.",
            ["oblong", "onset", "snare", "filter", "groove"],
            "Onset", "Oblong", "Env->Filter", 0.65,
        ),
        (
            MARQUEE_NAMES[2],
            "BOB's amber character feeds ONSET machine parameters — the bass drum warms with the sub.",
            ["oblong", "onset", "amber", "machine", "bass"],
            "Oblong", "Onset", "Amp->Pitch", 0.55,
        ),
        (
            MARQUEE_NAMES[3],
            "Dual-coupling: ONSET kick drives BOB pitch while BOB amplitude gates ONSET decay.",
            ["oblong", "onset", "dual", "coupling", "backbone"],
            "Oblong", "Onset", "Amp->Filter", 0.82,
        ),
        (
            MARQUEE_NAMES[4],
            "BOB and ONSET locked in mutual envelope exchange — the engine room that never stops.",
            ["oblong", "onset", "engine", "room", "production"],
            "Onset", "Oblong", "Gate->Env", 0.72,
        ),
    ]
    presets = []
    for name, desc, tags, ea, eb, ct, ca in specs:
        dna = blend_dna("Oblong", "Onset", weight_a=0.5)
        presets.append(make_preset(name, desc, tags, ea, eb, ct, ca, dna))
    return presets


def _oblong_pair_specs() -> list:
    """3 presets per OBLONG partner (10 partners × 3 = 30)."""
    pairs = []

    # OBLONG × OBBLIGATO
    pairs += [
        ("Anchor Breath",
         "BOB's foundational bass locks under OBBLIGATO's dual-wind breath — grounded and lyrical.",
         ["oblong", "obbligato", "bass", "wind", "foundation"],
         "Oblong", "Obbligato", "Amp->Filter", 0.60),
        ("Warm Wind Column",
         "OBBLIGATO breath envelope flows through BOB's filter — warmth ascending the scale.",
         ["oblong", "obbligato", "wind", "warmth", "column"],
         "Obbligato", "Oblong", "Env->Pitch", 0.55),
        ("Core Bond",
         "BOB anchor oscillator and OBBLIGATO BOND macro in harmonic lock — the core holds.",
         ["oblong", "obbligato", "bond", "core", "harmonic"],
         "Oblong", "Obbligato", "LFO->Filter", 0.62),
    ]

    # OBLONG × OHM
    pairs += [
        ("Amber Commune",
         "BOB's warm amber character feeds OHM's COMMUNE axis — hippy-dad bass joins the circle.",
         ["oblong", "ohm", "amber", "commune", "warm"],
         "Oblong", "Ohm", "Amp->Filter", 0.58),
        ("Grounded Meddling",
         "OHM MEDDLING macro stirs BOB's motion depth — meditation disrupted from below.",
         ["oblong", "ohm", "meddling", "ground", "motion"],
         "Ohm", "Oblong", "LFO->Amp", 0.50),
        ("Bass Sage",
         "BOB sub-bass envelope opens OHM's sage harmonic space — earthy resonance coupled.",
         ["oblong", "ohm", "bass", "sage", "harmonic"],
         "Oblong", "Ohm", "Env->Morph", 0.54),
    ]

    # OBLONG × ORPHICA
    pairs += [
        ("Pluck Anchor",
         "ORPHICA microsound harp struck against BOB's foundational anchor — fragile above solid.",
         ["oblong", "orphica", "pluck", "anchor", "contrast"],
         "Oblong", "Orphica", "Amp->Filter", 0.52),
        ("Amber Siren",
         "BOB's amber warmth pulls ORPHICA's siren seafoam brightness downward — seduction grounded.",
         ["oblong", "orphica", "amber", "siren", "grounded"],
         "Orphica", "Oblong", "Env->Pitch", 0.48),
        ("Core Harp",
         "ORPHICA harp transient modulates BOB character oscillator — strings plucking the foundation.",
         ["oblong", "orphica", "harp", "core", "transient"],
         "Orphica", "Oblong", "Env->Morph", 0.57),
    ]

    # OBLONG × OTTONI
    pairs += [
        ("Brass Foundation",
         "OTTONI triple-brass GROW macro expands from BOB's Amber anchor — brass built on bass.",
         ["oblong", "ottoni", "brass", "foundation", "grow"],
         "Oblong", "Ottoni", "Amp->Filter", 0.66),
        ("Warm Patina",
         "BOB filter warmth bleeds into OTTONI patina resonance — organic brass decay.",
         ["oblong", "ottoni", "warm", "patina", "decay"],
         "Oblong", "Ottoni", "Env->Pitch", 0.61),
        ("Anchor Fanfare",
         "OTTONI envelope bursts unlock BOB's motion depth — the horn section grounds the groove.",
         ["oblong", "ottoni", "fanfare", "anchor", "groove"],
         "Ottoni", "Oblong", "Env->Morph", 0.58),
    ]

    # OBLONG × OLE
    pairs += [
        ("Amber Drama",
         "OLE DRAMA macro pulls BOB's warm bottom into Afro-Latin rhythmic fire — bass meets heat.",
         ["oblong", "ole", "amber", "drama", "latin"],
         "Ole", "Oblong", "LFO->Filter", 0.68),
        ("Foundation Groove",
         "BOB foundational pulse locks OLE trio into a groove — the bass doesn't lie.",
         ["oblong", "ole", "foundation", "groove", "trio"],
         "Oblong", "Ole", "Amp->Filter", 0.63),
        ("Warm Olé",
         "BOB's warmth envelopes OLE's hibiscus brightness — the dance floor breathes deep.",
         ["oblong", "ole", "warmth", "hibiscus", "dance"],
         "Oblong", "Ole", "Env->Morph", 0.59),
    ]

    # OBLONG × OMBRE
    pairs += [
        ("Shadow Anchor",
         "BOB's amber weight holds OMBRE's shadow mauve memory from dissolving — presence and loss.",
         ["oblong", "ombre", "shadow", "anchor", "memory"],
         "Oblong", "Ombre", "Amp->Filter", 0.48),
        ("Warm Fade",
         "OMBRE forgetting axis modulates BOB's filter character — the warmth gradually dims.",
         ["oblong", "ombre", "warm", "fade", "forgetting"],
         "Ombre", "Oblong", "LFO->Amp", 0.45),
        ("Grounded Dusk",
         "BOB core oscillator steadies OMBRE's dual-narrative blend — grounded at dusk.",
         ["oblong", "ombre", "ground", "dusk", "narrative"],
         "Oblong", "Ombre", "Env->Pitch", 0.52),
    ]

    # OBLONG × ORCA
    pairs += [
        ("Apex Foundation",
         "ORCA hunt macro drives BOB's filter open — the apex predator surfaces on the bass.",
         ["oblong", "orca", "apex", "hunt", "deep"],
         "Orca", "Oblong", "Env->Filter", 0.74),
        ("Deep Anchor",
         "BOB's foundational sub meets ORCA's deep ocean echolocation — abyssal resonance.",
         ["oblong", "orca", "deep", "anchor", "abyssal"],
         "Oblong", "Orca", "Amp->Pitch", 0.70),
        ("Amber Breach",
         "ORCA breach event unlocks BOB's motion depth — the warmth surfaces violently.",
         ["oblong", "orca", "amber", "breach", "violent"],
         "Orca", "Oblong", "Chaos->Pitch", 0.78),
    ]

    # OBLONG × OCTOPUS
    pairs += [
        ("Eight Arms Bass",
         "OCTOPUS arm-depth modulates BOB's filter in decentralized waves — alien bass intelligence.",
         ["oblong", "octopus", "arms", "bass", "decentralized"],
         "Octopus", "Oblong", "LFO->Filter", 0.65),
        ("Chromatophore Amber",
         "BOB's amber pulse triggers OCTOPUS chromatophore flashes — color born from warmth.",
         ["oblong", "octopus", "chromatophore", "amber", "flash"],
         "Oblong", "Octopus", "Amp->Morph", 0.60),
        ("Ink Core",
         "OCTOPUS ink-cloud envelope swallows BOB's core oscillator — obscured foundation.",
         ["oblong", "octopus", "ink", "core", "obscure"],
         "Octopus", "Oblong", "Env->Pitch", 0.57),
    ]

    # OBLONG × OVERLAP
    pairs += [
        ("Spatial Foundation",
         "OVERLAP's FDN spatial field wraps BOB's anchor bass — the foundation echoes everywhere.",
         ["oblong", "overlap", "spatial", "foundation", "fdn"],
         "Overlap", "Oblong", "LFO->Amp", 0.55),
        ("Warm Lattice",
         "BOB's warmth feeds into OVERLAP's knot-topology lattice — amber woven into space.",
         ["oblong", "overlap", "warm", "lattice", "knot"],
         "Oblong", "Overlap", "Amp->Filter", 0.58),
        ("Grounded Diffusion",
         "OVERLAP diffusion network extends from BOB's grounded anchor — space built on earth.",
         ["oblong", "overlap", "grounded", "diffusion", "space"],
         "Oblong", "Overlap", "Env->Morph", 0.52),
    ]

    # OBLONG × OUTWIT
    pairs += [
        ("Wolfram Bass",
         "OUTWIT Wolfram CA modulation sweeps BOB's filter — cellular automata meets amber bass.",
         ["oblong", "outwit", "wolfram", "bass", "automata"],
         "Outwit", "Oblong", "Chaos->Pitch", 0.67),
        ("Core Pattern",
         "BOB foundational pulse seeds OUTWIT's rule-based evolution — the pattern starts here.",
         ["oblong", "outwit", "core", "pattern", "evolution"],
         "Oblong", "Outwit", "Gate->Env", 0.63),
        ("Amber Octopus Logic",
         "OUTWIT eight-arm logic modulates BOB's warm character — unpredictable warmth.",
         ["oblong", "outwit", "amber", "logic", "unpredictable"],
         "Outwit", "Oblong", "LFO->Filter", 0.61),
    ]

    return pairs


def _onset_pair_specs() -> list:
    """3 presets per ONSET partner (10 partners × 3 = 30)."""
    pairs = []

    # ONSET × OBBLIGATO
    pairs += [
        ("Kick Breath",
         "ONSET kick transient gates OBBLIGATO's wind breath on every downbeat — percussive inhale.",
         ["onset", "obbligato", "kick", "breath", "gate"],
         "Onset", "Obbligato", "Gate->Env", 0.72),
        ("Snare Wind",
         "OBBLIGATO BOND macro unlocks on every ONSET snare — the wind answers the crack.",
         ["onset", "obbligato", "snare", "wind", "bond"],
         "Obbligato", "Onset", "Env->Pitch", 0.60),
        ("Machine Breath",
         "ONSET machine-mode transients sculpt OBBLIGATO dual-wind shape — drum machine breathing.",
         ["onset", "obbligato", "machine", "breath", "sculpt"],
         "Onset", "Obbligato", "Amp->Filter", 0.65),
    ]

    # ONSET × OHM
    pairs += [
        ("Drum Circle",
         "ONSET percussion feeds OHM's commune circle — the drum is the meditation anchor.",
         ["onset", "ohm", "drum", "circle", "commune"],
         "Onset", "Ohm", "Amp->Filter", 0.60),
        ("Meddling Machine",
         "OHM MEDDLING macro disrupts ONSET's machine parameters — the jam session goes sideways.",
         ["onset", "ohm", "meddling", "machine", "disrupt"],
         "Ohm", "Onset", "LFO->Amp", 0.55),
        ("Impact Sage",
         "ONSET impact velocity sculpts OHM's sage harmonic depth — harder hit, deeper resonance.",
         ["onset", "ohm", "impact", "sage", "velocity"],
         "Onset", "Ohm", "Velocity->Cutoff", 0.68),
    ]

    # ONSET × ORPHICA
    pairs += [
        ("Harp Hits",
         "ONSET transients pluck ORPHICA's microsound harp strings — the machine plays the harp.",
         ["onset", "orphica", "harp", "pluck", "transient"],
         "Onset", "Orphica", "Gate->Env", 0.65),
        ("Siren Attack",
         "ORPHICA siren envelope modulates ONSET attack character — the harp shapes the drum.",
         ["onset", "orphica", "siren", "attack", "shape"],
         "Orphica", "Onset", "Env->Morph", 0.52),
        ("Percussion Seafoam",
         "ONSET snare brightness sweeps ORPHICA's seafoam filter — transient to shimmer.",
         ["onset", "orphica", "percussion", "seafoam", "shimmer"],
         "Onset", "Orphica", "Amp->Filter", 0.58),
    ]

    # ONSET × OLE
    pairs += [
        ("Latin Machine",
         "OLE DRAMA macro ignites ONSET machine mode — the rhythm section catches fire.",
         ["onset", "ole", "latin", "drama", "fire"],
         "Ole", "Onset", "LFO->Filter", 0.74),
        ("Kick Drama",
         "ONSET kick amplitude fuels OLE's trio energy — the bassline drama starts with the beat.",
         ["onset", "ole", "kick", "drama", "energy"],
         "Onset", "Ole", "Amp->Pitch", 0.68),
        ("Percussion Hibiscus",
         "ONSET percussion triggers OLE hibiscus brightness — every hit blooms.",
         ["onset", "ole", "percussion", "hibiscus", "bloom"],
         "Onset", "Ole", "Gate->Env", 0.70),
    ]

    # ONSET × OMBRE
    pairs += [
        ("Fading Machine",
         "OMBRE forgetting axis gradually dissolves ONSET machine parameters — the beat remembers less.",
         ["onset", "ombre", "fade", "machine", "forgetting"],
         "Ombre", "Onset", "LFO->Amp", 0.45),
        ("Impact Memory",
         "ONSET impact velocity writes to OMBRE memory axis — the hit defines the shade.",
         ["onset", "ombre", "impact", "memory", "velocity"],
         "Onset", "Ombre", "Velocity->Cutoff", 0.55),
        ("Shadow Snare",
         "OMBRE shadow narrative modulates ONSET snare body — the crack carries a shadow.",
         ["onset", "ombre", "shadow", "snare", "narrative"],
         "Ombre", "Onset", "Env->Pitch", 0.48),
    ]

    # ONSET × ORCA
    pairs += [
        ("Hunt Transient",
         "ORCA hunt macro releases on every ONSET kick — predator synchronized with the machine.",
         ["onset", "orca", "hunt", "transient", "predator"],
         "Orca", "Onset", "Gate->Env", 0.78),
        ("Breach Snare",
         "ONSET snare triggers ORCA breach event — the crack surfaces like a kill.",
         ["onset", "orca", "breach", "snare", "violent"],
         "Onset", "Orca", "Amp->Pitch", 0.82),
        ("Echolocation Kick",
         "ONSET kick envelope maps to ORCA echolocation depth — the beat pings the abyss.",
         ["onset", "orca", "echolocation", "kick", "depth"],
         "Onset", "Orca", "Env->Morph", 0.72),
    ]

    # ONSET × OCTOPUS
    pairs += [
        ("Eight Arm Percussion",
         "OCTOPUS arm-depth modulation distributes ONSET percussion across 8 channels — decentralized drums.",
         ["onset", "octopus", "arms", "percussion", "decentralized"],
         "Octopus", "Onset", "LFO->Filter", 0.62),
        ("Chromatophore Hit",
         "ONSET impact velocity triggers OCTOPUS chromatophore color flashes — hit = light.",
         ["onset", "octopus", "chromatophore", "hit", "color"],
         "Onset", "Octopus", "Velocity->Cutoff", 0.67),
        ("Ink Machine",
         "OCTOPUS ink-cloud obscures ONSET machine parameters — the drums dissolve into the fog.",
         ["onset", "octopus", "ink", "machine", "obscure"],
         "Octopus", "Onset", "Env->Morph", 0.55),
    ]

    # ONSET × OVERLAP
    pairs += [
        ("Spatial Kick",
         "ONSET kick amplitude feeds OVERLAP's FDN diffusion — the bass drum fills every room.",
         ["onset", "overlap", "spatial", "kick", "fdn"],
         "Onset", "Overlap", "Amp->Filter", 0.65),
        ("Snare Lattice",
         "OVERLAP's knot-topology network resonates on every ONSET snare — the crack echoes topologically.",
         ["onset", "overlap", "snare", "lattice", "echo"],
         "Onset", "Overlap", "Gate->Env", 0.60),
        ("Percussion Diffusion",
         "ONSET percussion transients seed OVERLAP diffusion tails — rhythm blooming into space.",
         ["onset", "overlap", "percussion", "diffusion", "space"],
         "Onset", "Overlap", "Env->Morph", 0.57),
    ]

    # ONSET × OUTWIT
    pairs += [
        ("Wolfram Kick",
         "OUTWIT Wolfram CA rule-set reshapes ONSET kick waveform — the machine rewrites itself.",
         ["onset", "outwit", "wolfram", "kick", "automata"],
         "Outwit", "Onset", "Chaos->Pitch", 0.70),
        ("Cellular Machine",
         "ONSET machine mode seeds OUTWIT eight-arm evolution — the drum pattern becomes cellular.",
         ["onset", "outwit", "cellular", "machine", "evolution"],
         "Onset", "Outwit", "Gate->Env", 0.66),
        ("Pattern Attack",
         "OUTWIT pattern logic modulates ONSET attack transient — structured chaos hits harder.",
         ["onset", "outwit", "pattern", "attack", "chaos"],
         "Outwit", "Onset", "LFO->Filter", 0.63),
    ]

    # ONSET × ORACLE
    pairs += [
        ("Prophecy Kick",
         "ORACLE breakpoints trigger ONSET kick modulations — the drum knows what comes next.",
         ["onset", "oracle", "prophecy", "kick", "breakpoint"],
         "Oracle", "Onset", "Env->Morph", 0.64),
        ("Stochastic Snare",
         "ONSET snare randomness feeds ORACLE GENDY stochastic parameters — the snare speaks in tongues.",
         ["onset", "oracle", "stochastic", "snare", "gendy"],
         "Onset", "Oracle", "Amp->Filter", 0.59),
        ("Machine Prophecy",
         "ORACLE indigo modulation shapes ONSET machine character — the oracle programs the beat.",
         ["onset", "oracle", "machine", "prophecy", "indigo"],
         "Oracle", "Onset", "LFO->Amp", 0.62),
    ]

    return pairs


def build_all_presets(count_per_pair: int) -> list:
    """Build full preset list with optional count reduction."""
    presets = []

    # Marquee (always all 5)
    presets.extend(build_marquee_presets())

    # OBLONG pairs
    oblong_by_partner: dict = {}
    for spec in _oblong_pair_specs():
        name, desc, tags, ea, eb, ct, ca = spec
        partner = eb if ea == "Oblong" else ea
        oblong_by_partner.setdefault(partner, []).append(spec)

    for partner, specs in oblong_by_partner.items():
        chosen = specs[:count_per_pair]
        for name, desc, tags, ea, eb, ct, ca in chosen:
            dna = blend_dna("Oblong", partner)
            presets.append(make_preset(name, desc, tags, ea, eb, ct, ca, dna))

    # ONSET pairs
    onset_by_partner: dict = {}
    for spec in _onset_pair_specs():
        name, desc, tags, ea, eb, ct, ca = spec
        partner = eb if ea == "Onset" else ea
        onset_by_partner.setdefault(partner, []).append(spec)

    for partner, specs in onset_by_partner.items():
        chosen = specs[:count_per_pair]
        for name, desc, tags, ea, eb, ct, ca in chosen:
            dna = blend_dna("Onset", partner)
            presets.append(make_preset(name, desc, tags, ea, eb, ct, ca, dna))

    return presets


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------
def main():
    repo_root = Path(__file__).resolve().parent.parent
    default_output = repo_root / "Presets" / "XOceanus" / "Entangled"

    parser = argparse.ArgumentParser(
        description="Generate OBLONG + ONSET coupling expansion presets."
    )
    parser.add_argument(
        "--output-dir",
        default=str(default_output),
        help="Directory to write .xometa files (default: Presets/XOceanus/Entangled/)",
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Print preset names without writing files.",
    )
    parser.add_argument(
        "--seed",
        type=int,
        default=None,
        help="Random seed for reproducible output.",
    )
    parser.add_argument(
        "--count",
        type=int,
        default=3,
        metavar="N",
        help="Number of presets per pair group (1-3, default 3). Marquee 5 always written.",
    )
    args = parser.parse_args()

    count_per_pair = max(1, min(3, args.count))

    if args.seed is not None:
        random.seed(args.seed)

    presets = build_all_presets(count_per_pair)

    output_dir = Path(args.output_dir)

    if args.dry_run:
        print(f"Dry run — {len(presets)} presets would be written to: {output_dir}")
        for p in presets:
            engines = " × ".join(p["engines"])
            print(f"  [{engines}] {p['name']}")
        return

    output_dir.mkdir(parents=True, exist_ok=True)

    written = 0
    skipped = 0
    for preset in presets:
        filename = safe_filename(preset["name"]) + ".xometa"
        filepath = output_dir / filename
        if filepath.exists():
            skipped += 1
            continue
        with open(filepath, "w", encoding="utf-8") as f:
            json.dump(preset, f, indent=2)
            f.write("\n")
        written += 1

    print(f"Done. Written: {written}  Skipped (already exist): {skipped}")
    print(f"Output dir: {output_dir}")


if __name__ == "__main__":
    main()
