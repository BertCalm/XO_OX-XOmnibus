#!/usr/bin/env python3
"""
xpn_family_expansion_pack.py
Generate 60 Family mood presets — engine portraits, extended family, reunion duos.
Writes to Presets/XOmnibus/Family/. Skips existing files.
"""

import json
import os
from pathlib import Path

OUTPUT_DIR = Path(__file__).parent.parent / "Presets" / "XOmnibus" / "Family"
OUTPUT_DIR.mkdir(parents=True, exist_ok=True)

TODAY = "2026-03-16"

# ---------------------------------------------------------------------------
# Engine DNA profiles — canonical sonic fingerprints
# ---------------------------------------------------------------------------
ENGINE_DNA = {
    # Core / original engines
    "OddfeliX": {
        "brightness": 0.72, "warmth": 0.55, "movement": 0.45, "density": 0.5,
        "space": 0.55, "aggression": 0.2,
        "character": "neon tetra — sharp digital shimmer with warm undertow",
        "macro_labels": ["Snap+Morph", "Bloom", "Coupling", "Space"],
    },
    "OddOscar": {
        "brightness": 0.38, "warmth": 0.78, "movement": 0.35, "density": 0.55,
        "space": 0.6, "aggression": 0.12,
        "character": "axolotl — soft analog glow, regenerative warmth",
        "macro_labels": ["Snap+Morph", "Bloom", "Coupling", "Space"],
    },
    "OVERDUB": {
        "brightness": 0.35, "warmth": 0.82, "movement": 0.62, "density": 0.65,
        "space": 0.75, "aggression": 0.28,
        "character": "tape dub machine — saturated trails, spacious wobble",
        "macro_labels": ["DRIVE", "WOBBLE", "SEND", "SPACE"],
    },
    "ODYSSEY": {
        "brightness": 0.55, "warmth": 0.48, "movement": 0.7, "density": 0.45,
        "space": 0.65, "aggression": 0.22,
        "character": "open-water wanderer — slow drift, exploratory motion",
        "macro_labels": ["DRIFT", "DEPTH", "COUPLING", "HORIZON"],
    },
    "OBLONG": {
        "brightness": 0.4, "warmth": 0.75, "movement": 0.3, "density": 0.7,
        "space": 0.45, "aggression": 0.35,
        "character": "XOblongBob — chunky harmonic mass, chord warmth",
        "macro_labels": ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"],
    },
    "OBESE": {
        "brightness": 0.25, "warmth": 0.72, "movement": 0.28, "density": 0.85,
        "space": 0.38, "aggression": 0.55,
        "character": "XObese — sub-heavy, saturated, low-end dominant",
        "macro_labels": ["WEIGHT", "GRIT", "COUPLING", "SPACE"],
    },
    "ONSET": {
        "brightness": 0.58, "warmth": 0.42, "movement": 0.8, "density": 0.72,
        "space": 0.4, "aggression": 0.65,
        "character": "percussion architect — punchy transients, rhythmic density",
        "macro_labels": ["MACHINE", "PUNCH", "SPACE", "MUTATE"],
    },
    "OVERWORLD": {
        "brightness": 0.65, "warmth": 0.38, "movement": 0.55, "density": 0.5,
        "space": 0.58, "aggression": 0.3,
        "character": "chip synth — NES/Genesis/SNES era palette, pixelated warmth",
        "macro_labels": ["ERA", "CHAOS", "COUPLING", "SPACE"],
    },
    "OPAL": {
        "brightness": 0.6, "warmth": 0.52, "movement": 0.75, "density": 0.55,
        "space": 0.72, "aggression": 0.15,
        "character": "granular gem — shimmering grain clouds, luminous scatter",
        "macro_labels": ["GRAIN", "SCATTER", "COUPLING", "SPACE"],
    },
    "ORBITAL": {
        "brightness": 0.62, "warmth": 0.45, "movement": 0.68, "density": 0.48,
        "space": 0.8, "aggression": 0.18,
        "character": "deep-space sequencer — elliptical motion, celestial pulse",
        "macro_labels": ["ORBIT", "RESONANCE", "COUPLING", "SPACE"],
    },
    "ORGANON": {
        "brightness": 0.5, "warmth": 0.6, "movement": 0.4, "density": 0.65,
        "space": 0.5, "aggression": 0.25,
        "character": "biological logic — living harmonic metabolism",
        "macro_labels": ["LOGIC", "METABOLISM", "COUPLING", "SPACE"],
    },
    "OUROBOROS": {
        "brightness": 0.48, "warmth": 0.5, "movement": 0.85, "density": 0.6,
        "space": 0.55, "aggression": 0.4,
        "character": "Lorenz chaos serpent — self-devouring feedback spirals",
        "macro_labels": ["CHAOS", "DECAY", "COUPLING", "SPACE"],
    },
    "OBSIDIAN": {
        "brightness": 0.22, "warmth": 0.35, "movement": 0.38, "density": 0.78,
        "space": 0.42, "aggression": 0.62,
        "character": "volcanic glass — dark, cutting, brutal edge",
        "macro_labels": ["EDGE", "MASS", "COUPLING", "SPACE"],
    },
    "OVERBITE": {
        "brightness": 0.45, "warmth": 0.65, "movement": 0.52, "density": 0.68,
        "space": 0.48, "aggression": 0.48,
        "character": "bass-forward character — warm fang, low bite",
        "macro_labels": ["BITE", "WARMTH", "COUPLING", "SPACE"],
    },
    "ORIGAMI": {
        "brightness": 0.58, "warmth": 0.55, "movement": 0.45, "density": 0.42,
        "space": 0.65, "aggression": 0.15,
        "character": "paper fold — delicate geometric precision, airy structure",
        "macro_labels": ["FOLD", "TENSION", "COUPLING", "SPACE"],
    },
    "ORACLE": {
        "brightness": 0.52, "warmth": 0.58, "movement": 0.55, "density": 0.5,
        "space": 0.7, "aggression": 0.2,
        "character": "ancient voice — resonant prophecy, deep harmonic truth",
        "macro_labels": ["VOICE", "RESONANCE", "COUPLING", "SPACE"],
    },
    "OBSCURA": {
        "brightness": 0.3, "warmth": 0.45, "movement": 0.48, "density": 0.62,
        "space": 0.68, "aggression": 0.35,
        "character": "pinhole darkness — veiled frequencies, hidden beauty",
        "macro_labels": ["VEIL", "DEPTH", "COUPLING", "SPACE"],
    },
    "OCEANIC": {
        "brightness": 0.42, "warmth": 0.62, "movement": 0.72, "density": 0.58,
        "space": 0.85, "aggression": 0.18,
        "character": "deep ocean column — pressure, swell, vast resonance",
        "macro_labels": ["DEPTH", "SWELL", "COUPLING", "SPACE"],
    },
    "OCELOT": {
        "brightness": 0.68, "warmth": 0.5, "movement": 0.78, "density": 0.4,
        "space": 0.52, "aggression": 0.45,
        "character": "spotted hunter — quick attack, feline precision",
        "macro_labels": ["HUNT", "AGILITY", "COUPLING", "SPACE"],
    },
    "OPTIC": {
        "brightness": 0.82, "warmth": 0.32, "movement": 0.6, "density": 0.38,
        "space": 0.7, "aggression": 0.28,
        "character": "light refraction — spectral clarity, prismatic scatter",
        "macro_labels": ["SPECTRUM", "REFRACT", "COUPLING", "SPACE"],
    },
    # Constellation / newer engines
    "OBLIQUE": {
        "brightness": 0.55, "warmth": 0.5, "movement": 0.62, "density": 0.45,
        "space": 0.6, "aggression": 0.32,
        "character": "off-axis harmonics — slanted perspective, unexpected angles",
        "macro_labels": ["ANGLE", "SKEW", "COUPLING", "SPACE"],
    },
    "OSPREY": {
        "brightness": 0.7, "warmth": 0.42, "movement": 0.72, "density": 0.38,
        "space": 0.75, "aggression": 0.38,
        "character": "sea hawk — clean dive, Mediterranean air, sharp clarity",
        "macro_labels": ["DIVE", "CLARITY", "COUPLING", "SPACE"],
    },
    "OSTERIA": {
        "brightness": 0.48, "warmth": 0.78, "movement": 0.38, "density": 0.6,
        "space": 0.55, "aggression": 0.2,
        "character": "Italian table — warm gather, rich texture, communal warmth",
        "macro_labels": ["WARMTH", "TEXTURE", "COUPLING", "SPACE"],
    },
    "OWLFISH": {
        "brightness": 0.45, "warmth": 0.5, "movement": 0.42, "density": 0.55,
        "space": 0.65, "aggression": 0.22,
        "character": "deep-sea serpent — slow luminescence, patient mystery",
        "macro_labels": ["DEPTH", "GLOW", "COUPLING", "SPACE"],
    },
    "OHM": {
        "brightness": 0.42, "warmth": 0.72, "movement": 0.35, "density": 0.55,
        "space": 0.7, "aggression": 0.12,
        "character": "hippy dad resonance — drone warmth, communal hum",
        "macro_labels": ["MEDDLING", "COMMUNE", "COUPLING", "SPACE"],
    },
    "ORPHICA": {
        "brightness": 0.68, "warmth": 0.45, "movement": 0.6, "density": 0.38,
        "space": 0.78, "aggression": 0.15,
        "character": "microsound harp — siphonophore scatter, delicate shimmer",
        "macro_labels": ["SCATTER", "SHIMMER", "COUPLING", "SPACE"],
    },
    "OBBLIGATO": {
        "brightness": 0.55, "warmth": 0.5, "movement": 0.48, "density": 0.52,
        "space": 0.6, "aggression": 0.28,
        "character": "dual wind obligation — interlocking breath, BOND coupling",
        "macro_labels": ["BOND", "BREATH", "COUPLING", "SPACE"],
    },
    "OTTONI": {
        "brightness": 0.62, "warmth": 0.55, "movement": 0.55, "density": 0.65,
        "space": 0.5, "aggression": 0.48,
        "character": "triple brass — growing ensemble power, GROW macro",
        "macro_labels": ["GROW", "BRASS", "COUPLING", "SPACE"],
    },
    "OLE": {
        "brightness": 0.72, "warmth": 0.65, "movement": 0.78, "density": 0.58,
        "space": 0.52, "aggression": 0.42,
        "character": "Afro-Latin trio — rhythmic drama, communal fire",
        "macro_labels": ["DRAMA", "RHYTHM", "COUPLING", "SPACE"],
    },
    "OMBRE": {
        "brightness": 0.5, "warmth": 0.6, "movement": 0.55, "density": 0.48,
        "space": 0.72, "aggression": 0.18,
        "character": "gradient shading — smooth tonal transition, watercolor fade",
        "macro_labels": ["SHADE", "FADE", "COUPLING", "SPACE"],
    },
    "ORCA": {
        "brightness": 0.38, "warmth": 0.45, "movement": 0.65, "density": 0.72,
        "space": 0.78, "aggression": 0.52,
        "character": "apex predator — pod communication, powerful sweep",
        "macro_labels": ["HUNT", "POD", "COUPLING", "SPACE"],
    },
    "OCTOPUS": {
        "brightness": 0.52, "warmth": 0.48, "movement": 0.82, "density": 0.65,
        "space": 0.55, "aggression": 0.45,
        "character": "8-arm intelligence — Wolfram CA tentacles, adaptive chaos",
        "macro_labels": ["ARMS", "ADAPT", "COUPLING", "SPACE"],
    },
    "OVERLAP": {
        "brightness": 0.48, "warmth": 0.55, "movement": 0.6, "density": 0.68,
        "space": 0.82, "aggression": 0.22,
        "character": "Lion's Mane FDN — knot-topology reverb, overlapping tails",
        "macro_labels": ["KNOT", "TAIL", "COUPLING", "SPACE"],
    },
    "OUTWIT": {
        "brightness": 0.55, "warmth": 0.42, "movement": 0.88, "density": 0.6,
        "space": 0.5, "aggression": 0.55,
        "character": "octopus CA — 8-arm cellular automata, outmaneuvering logic",
        "macro_labels": ["RULE", "ARMS", "COUPLING", "SPACE"],
    },
}

# ---------------------------------------------------------------------------
# Preset definitions
# ---------------------------------------------------------------------------
def make_preset(name, engines, tags, description, dna_override=None,
                macros=None, coupling=None, batch_tags=None):
    """Build a single .xometa dict."""
    primary_engine = engines[0]
    base_dna = ENGINE_DNA.get(primary_engine, {
        "brightness": 0.5, "warmth": 0.5, "movement": 0.5,
        "density": 0.5, "space": 0.5, "aggression": 0.3,
    })

    dna = {
        "brightness": base_dna.get("brightness", 0.5),
        "warmth": base_dna.get("warmth", 0.5),
        "movement": base_dna.get("movement", 0.5),
        "density": base_dna.get("density", 0.5),
        "space": base_dna.get("space", 0.5),
        "aggression": base_dna.get("aggression", 0.3),
    }
    if dna_override:
        dna.update(dna_override)

    macro_labels = ENGINE_DNA.get(primary_engine, {}).get(
        "macro_labels", ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"]
    )

    if macros is None:
        # Derive sensible macro defaults from DNA
        macros = {
            macro_labels[0]: round(dna["aggression"] * 0.7 + dna["brightness"] * 0.3, 2),
            macro_labels[1]: round(dna["movement"] * 0.8, 2),
            macro_labels[2]: round(0.0 if coupling is None else coupling, 2),
            macro_labels[3]: round(dna["space"] * 0.9, 2),
        }

    all_tags = list(tags)
    if batch_tags:
        all_tags += batch_tags
    all_tags = list(dict.fromkeys(all_tags))  # dedupe, preserve order

    return {
        "schema_version": 1,
        "name": name,
        "mood": "Family",
        "engines": engines,
        "author": "XO_OX",
        "version": "1.0.0",
        "description": description,
        "tags": all_tags,
        "macroLabels": macro_labels,
        "couplingIntensity": "None" if (coupling is None or coupling == 0.0) else (
            "Light" if coupling < 0.35 else ("Medium" if coupling < 0.65 else "Heavy")
        ),
        "tempo": None,
        "created": TODAY,
        "legacy": {"sourceInstrument": None, "sourceCategory": None, "sourcePresetName": None},
        "parameters": {},
        "coupling": None,
        "sequencer": None,
        "dna": dna,
    }


def filename(name):
    return name.replace(" ", "_").replace("+", "_").replace("/", "_") + ".xometa"


def write_preset(preset):
    path = OUTPUT_DIR / filename(preset["name"])
    if path.exists():
        print(f"  SKIP (exists): {path.name}")
        return
    with open(path, "w") as f:
        json.dump(preset, f, indent=2)
    print(f"  WRITE: {path.name}")


# ---------------------------------------------------------------------------
# Batch 1 — Signatures (20 presets)
# Each engine's pure canonical DNA
# ---------------------------------------------------------------------------
BATCH1_TAGS = ["family", "signature", "portrait", "canonical"]

BATCH1 = [
    make_preset(
        "Felix Neon Solo",
        ["OddfeliX"],
        BATCH1_TAGS + ["digital", "shimmer", "bright"],
        "feliX at full presence — the neon tetra in its tank. Snap crisp, bloom lit, "
        "digital shimmer riding a warm sub. This is what feliX sounds like when nobody else is in the room.",
        batch_tags=["batch1-signatures"],
    ),
    make_preset(
        "Oscar Warm Portrait",
        ["OddOscar"],
        BATCH1_TAGS + ["warm", "analog", "soft"],
        "Oscar alone — axolotl breathing slow. Morph at mid, bloom unhurried, every filter "
        "frequency soft as riverbed clay. This is the sound of regeneration.",
        batch_tags=["batch1-signatures"],
    ),
    make_preset(
        "The Dub Machine",
        ["OVERDUB"],
        BATCH1_TAGS + ["dub", "tape", "saturated"],
        "OVERDUB in its element — drive warm, wobble riding the 2, send VCA open. "
        "Tape hiss present, spring reverb trailing. The machine breathes.",
        batch_tags=["batch1-signatures"],
        dna_override={"warmth": 0.88, "space": 0.8, "movement": 0.68},
    ),
    make_preset(
        "Slow River Drift",
        ["ODYSSEY"],
        BATCH1_TAGS + ["drift", "open-water", "slow"],
        "ODYSSEY on a wide horizon — drift macro open, depth resonant. "
        "An exploratory wander that never rushes. The engine at its most ODYSSEY.",
        batch_tags=["batch1-signatures"],
    ),
    make_preset(
        "Chunky Chord Bob",
        ["OBLONG"],
        BATCH1_TAGS + ["chords", "warm", "harmonic"],
        "OBLONG at full mass — chord mode engaged, harmonic density rich. "
        "XOblongBob's warm plural voice doing what it was born for.",
        batch_tags=["batch1-signatures"],
        dna_override={"density": 0.78, "warmth": 0.8},
    ),
    make_preset(
        "Low End Theory",
        ["OBESE"],
        BATCH1_TAGS + ["sub", "bass", "heavy"],
        "OBESE maximum weight — sub dominant, saturation present. "
        "The bottom of the spectrum claimed entirely. Nothing apologetic here.",
        batch_tags=["batch1-signatures"],
        dna_override={"density": 0.9, "aggression": 0.62, "brightness": 0.2},
    ),
    make_preset(
        "NES Kid",
        ["OVERWORLD"],
        BATCH1_TAGS + ["chip", "retro", "pixel"],
        "OVERWORLD full ERA mode — NES square waves in the room. The pixelated nostalgia "
        "of a cartridge that still boots. Warmth through 8-bit limitation.",
        batch_tags=["batch1-signatures"],
        dna_override={"brightness": 0.72, "warmth": 0.42},
    ),
    make_preset(
        "Grain Cloud Portrait",
        ["OPAL"],
        BATCH1_TAGS + ["granular", "shimmer", "scatter"],
        "OPAL in full scatter — grain size micro, scatter wide, luminous cloud diffuse. "
        "The gem at rest, refracting quietly.",
        batch_tags=["batch1-signatures"],
        dna_override={"space": 0.78, "brightness": 0.65, "movement": 0.72},
    ),
    make_preset(
        "Machine Groove Portrait",
        ["ONSET"],
        BATCH1_TAGS + ["drums", "percussion", "groove"],
        "ONSET full kit — MACHINE macro engaged, PUNCH present. "
        "The rhythm architect laying down its signature groove.",
        batch_tags=["batch1-signatures"],
        dna_override={"movement": 0.85, "density": 0.75, "aggression": 0.68},
    ),
    make_preset(
        "Elliptical Pulse",
        ["ORBITAL"],
        BATCH1_TAGS + ["space", "pulse", "celestial"],
        "ORBITAL deep orbit — elliptical motion, resonant nodes, celestial timing. "
        "A sequencer that moves like a planet.",
        batch_tags=["batch1-signatures"],
        dna_override={"space": 0.85, "movement": 0.7},
    ),
    make_preset(
        "Living Metabolism",
        ["ORGANON"],
        BATCH1_TAGS + ["organic", "harmonic", "biology"],
        "ORGANON in metabolic state — logic processes humming, harmonic biology alive. "
        "The living instrument breathing its own air.",
        batch_tags=["batch1-signatures"],
    ),
    make_preset(
        "Lorenz Portrait",
        ["OUROBOROS"],
        BATCH1_TAGS + ["chaos", "feedback", "spiral"],
        "OUROBOROS tail-biting — Lorenz attractor spiraling at medium chaos. "
        "The self-devouring pattern at its most hypnotic.",
        batch_tags=["batch1-signatures"],
        dna_override={"movement": 0.88, "aggression": 0.45},
    ),
    make_preset(
        "Volcanic Glass",
        ["OBSIDIAN"],
        BATCH1_TAGS + ["dark", "cutting", "brutal"],
        "OBSIDIAN unsheathed — edge maximum, mass full, brightness nearly absent. "
        "The sharpest thing in the kit. Use with intent.",
        batch_tags=["batch1-signatures"],
        dna_override={"brightness": 0.18, "aggression": 0.7, "density": 0.82},
    ),
    make_preset(
        "Warm Fang",
        ["OVERBITE"],
        BATCH1_TAGS + ["bass", "character", "bite"],
        "OVERBITE signature — BITE forward, warmth underneath. "
        "Gallery code OVERBITE: bass-forward character with Fang White accent. "
        "The synth that bites back softly.",
        batch_tags=["batch1-signatures"],
    ),
    make_preset(
        "Paper Geometry",
        ["ORIGAMI"],
        BATCH1_TAGS + ["delicate", "precision", "airy"],
        "ORIGAMI at rest — the perfect fold. Tension geometric, air between planes. "
        "Delicate structure that holds its shape without effort.",
        batch_tags=["batch1-signatures"],
        dna_override={"brightness": 0.6, "space": 0.7, "density": 0.38},
    ),
    make_preset(
        "Ancient Resonance",
        ["ORACLE"],
        BATCH1_TAGS + ["voice", "resonant", "ancient"],
        "ORACLE speaking — harmonic prophecy at medium depth. "
        "The voice that knows. Deep resonance, unhurried delivery.",
        batch_tags=["batch1-signatures"],
        dna_override={"space": 0.72, "warmth": 0.62},
    ),
    make_preset(
        "Pinhole Dark",
        ["OBSCURA"],
        BATCH1_TAGS + ["dark", "veiled", "hidden"],
        "OBSCURA at full veil — frequencies shadowed, depth active. "
        "The beauty that lives behind the curtain. Patient, interior, true.",
        batch_tags=["batch1-signatures"],
        dna_override={"brightness": 0.28, "space": 0.72},
    ),
    make_preset(
        "Deep Column",
        ["OCEANIC"],
        BATCH1_TAGS + ["ocean", "deep", "vast"],
        "OCEANIC full depth — pressure present, swell resonant. "
        "The water column in its most oceanic state: vast, dark, alive.",
        batch_tags=["batch1-signatures"],
        dna_override={"space": 0.88, "movement": 0.75, "warmth": 0.65},
    ),
    make_preset(
        "Spotted Hunter",
        ["OCELOT"],
        BATCH1_TAGS + ["percussive", "agile", "feline"],
        "OCELOT in motion — HUNT macro engaged, agility full. "
        "Quick attack, precise placement, feline economy of movement.",
        batch_tags=["batch1-signatures"],
        dna_override={"movement": 0.82, "aggression": 0.5, "brightness": 0.7},
    ),
    make_preset(
        "Prismatic Light",
        ["OPTIC"],
        BATCH1_TAGS + ["bright", "spectral", "light"],
        "OPTIC at full spectrum — REFRACT open, brightness maximum. "
        "Light through a prism: every frequency present, each in its place.",
        batch_tags=["batch1-signatures"],
        dna_override={"brightness": 0.88, "space": 0.72, "warmth": 0.28},
    ),
]

# ---------------------------------------------------------------------------
# Batch 2 — Extended Family (20 presets): newer engines
# ---------------------------------------------------------------------------
BATCH2_TAGS = ["family", "extended-family", "portrait"]

BATCH2 = [
    make_preset(
        "Oblique Angle",
        ["OBLIQUE"],
        BATCH2_TAGS + ["harmonic", "slanted", "unexpected"],
        "OBLIQUE at its most itself — harmonics arriving from an unexpected direction. "
        "Not wrong, just off-axis. The perspective that reframes the room.",
        batch_tags=["batch2-extended"],
    ),
    make_preset(
        "Osprey Dive",
        ["OSPREY"],
        BATCH2_TAGS + ["clean", "sharp", "Mediterranean"],
        "OSPREY in full dive — CLARITY macro open, Mediterranean air carrying it. "
        "The clean predator falling toward the water.",
        batch_tags=["batch2-extended"],
        dna_override={"brightness": 0.75, "movement": 0.78, "aggression": 0.42},
    ),
    make_preset(
        "Osteria Golden Hour",
        ["OSTERIA"],
        BATCH2_TAGS + ["warm", "communal", "rich"],
        "OSTERIA at table — WARMTH full, texture present. "
        "The Italian gathering where nobody leaves hungry. Late afternoon light.",
        batch_tags=["batch2-extended"],
        dna_override={"warmth": 0.85, "density": 0.65, "space": 0.52},
    ),
    make_preset(
        "Deep Sea Glow",
        ["OWLFISH"],
        BATCH2_TAGS + ["luminescent", "deep", "patient"],
        "OWLFISH in the dark — GLOW macro lit, DEPTH resonant. "
        "Bioluminescence at 3000 meters. Patient. Necessary.",
        batch_tags=["batch2-extended"],
        dna_override={"brightness": 0.38, "space": 0.7, "movement": 0.38},
    ),
    make_preset(
        "Commune Drone",
        ["OHM"],
        BATCH2_TAGS + ["drone", "warm", "meditative"],
        "OHM COMMUNE axis — drone warmth sustained, MEDDLING at peace. "
        "The hippy dad who knows when to stop talking and just hum.",
        batch_tags=["batch2-extended"],
        dna_override={"warmth": 0.78, "movement": 0.28, "space": 0.75},
    ),
    make_preset(
        "Siphonophore Harp",
        ["ORPHICA"],
        BATCH2_TAGS + ["microsound", "shimmer", "delicate"],
        "ORPHICA microsound colony — SCATTER wide, SHIMMER present. "
        "Each grain a polyp. The organism that thinks it is a single being.",
        batch_tags=["batch2-extended"],
        dna_override={"brightness": 0.72, "space": 0.82, "density": 0.35},
    ),
    make_preset(
        "Wind Obligation",
        ["OBBLIGATO"],
        BATCH2_TAGS + ["wind", "dual", "interlocking"],
        "OBBLIGATO BOND macro — dual wind voices interlocked. "
        "The obligation fulfilled in harmony. Neither voice can leave.",
        batch_tags=["batch2-extended"],
        coupling=0.45,
        dna_override={"density": 0.55, "movement": 0.52},
    ),
    make_preset(
        "Brass Ensemble Grows",
        ["OTTONI"],
        BATCH2_TAGS + ["brass", "ensemble", "powerful"],
        "OTTONI GROW macro engaged — triple brass building. "
        "The crescendo of an ensemble that finds its size.",
        batch_tags=["batch2-extended"],
        dna_override={"density": 0.72, "aggression": 0.55, "warmth": 0.58},
    ),
    make_preset(
        "Afro-Latin Drama",
        ["OLE"],
        BATCH2_TAGS + ["rhythm", "fire", "communal"],
        "OLE DRAMA macro at peak — Afro-Latin trio in full voice. "
        "The circle that drives itself. Nobody sitting down.",
        batch_tags=["batch2-extended"],
        dna_override={"movement": 0.82, "aggression": 0.48, "warmth": 0.68},
    ),
    make_preset(
        "Watercolor Fade",
        ["OMBRE"],
        BATCH2_TAGS + ["gradient", "soft", "transition"],
        "OMBRE SHADE to FADE — tonal gradient drifting. "
        "A watercolor bleeding at the edges. The in-between as destination.",
        batch_tags=["batch2-extended"],
        dna_override={"movement": 0.5, "space": 0.75, "warmth": 0.65},
    ),
    make_preset(
        "Orca Pod Signal",
        ["ORCA"],
        BATCH2_TAGS + ["whale", "communication", "deep"],
        "ORCA POD macro active — echolocation and song. "
        "The apex predator communicating across kilometers of dark water.",
        batch_tags=["batch2-extended"],
        dna_override={"space": 0.82, "movement": 0.68, "aggression": 0.55},
    ),
    make_preset(
        "Eight Arms Working",
        ["OCTOPUS"],
        BATCH2_TAGS + ["chaos", "adaptive", "intelligent"],
        "OCTOPUS ARMS macro full — Wolfram CA in 8-part motion. "
        "The distributed intelligence that solves problems sideways.",
        batch_tags=["batch2-extended"],
        dna_override={"movement": 0.88, "density": 0.68, "aggression": 0.5},
    ),
    make_preset(
        "Lion's Mane Tails",
        ["OVERLAP"],
        BATCH2_TAGS + ["reverb", "knot", "spatial"],
        "OVERLAP KNOT topology — FDN tails overlapping. "
        "The jellyfish whose tendrils never fully separate. Endless interweave.",
        batch_tags=["batch2-extended"],
        dna_override={"space": 0.88, "density": 0.72, "movement": 0.62},
    ),
    make_preset(
        "Rule 90 Unfolds",
        ["OUTWIT"],
        BATCH2_TAGS + ["cellular-automata", "rule", "pattern"],
        "OUTWIT RULE 90 — the octopus outmaneuvering logic itself. "
        "Eight arms of cellular automata solving what shouldn't be solvable.",
        batch_tags=["batch2-extended"],
        dna_override={"movement": 0.9, "aggression": 0.58, "density": 0.62},
    ),
    make_preset(
        "Felix Extended Take",
        ["OddfeliX"],
        BATCH2_TAGS + ["digital", "neon", "evolution"],
        "feliX in the extended family portrait — older now, more complex. "
        "The snap sharper, the morph wider. Still neon, still tetra.",
        batch_tags=["batch2-extended"],
        dna_override={"brightness": 0.78, "movement": 0.52, "aggression": 0.25},
    ),
    make_preset(
        "Oscar Extended Warmth",
        ["OddOscar"],
        BATCH2_TAGS + ["analog", "warm", "deep"],
        "Oscar in the extended family — the axolotl grown large. "
        "More bloom, more morph depth. The warmth that took years to develop.",
        batch_tags=["batch2-extended"],
        dna_override={"warmth": 0.85, "density": 0.62, "space": 0.65},
    ),
    make_preset(
        "Orbital Return",
        ["ORBITAL"],
        BATCH2_TAGS + ["space", "elliptical", "return"],
        "ORBITAL completing its ellipse — the long orbit coming home. "
        "Extended resonance, slow pulse. The planet that remembers its path.",
        batch_tags=["batch2-extended"],
        dna_override={"space": 0.88, "movement": 0.62, "warmth": 0.48},
    ),
    make_preset(
        "Oracle Extended Vision",
        ["ORACLE"],
        BATCH2_TAGS + ["voice", "prophecy", "deep"],
        "ORACLE in the extended session — the vision wider now. "
        "Resonance deeper, space more generous. The ancient voice unguarded.",
        batch_tags=["batch2-extended"],
        dna_override={"space": 0.78, "warmth": 0.65, "movement": 0.58},
    ),
    make_preset(
        "Organon Extended",
        ["ORGANON"],
        BATCH2_TAGS + ["organic", "metabolism", "complex"],
        "ORGANON extended metabolism — logic processing more pathways. "
        "The biological instrument in a longer conversation with itself.",
        batch_tags=["batch2-extended"],
        dna_override={"density": 0.72, "movement": 0.48, "warmth": 0.65},
    ),
    make_preset(
        "Oceanic Extended Depth",
        ["OCEANIC"],
        BATCH2_TAGS + ["ocean", "pressure", "vast"],
        "OCEANIC at maximum depth — the extended family portrait taken "
        "at the bottom of the water column. Pressure, beauty, silence between waves.",
        batch_tags=["batch2-extended"],
        dna_override={"space": 0.92, "density": 0.65, "warmth": 0.68},
    ),
]

# ---------------------------------------------------------------------------
# Batch 3 — Reunion (20 presets): cross-generational duo portraits
# ---------------------------------------------------------------------------
BATCH3_TAGS = ["family", "reunion", "duo", "portrait"]

BATCH3 = [
    make_preset(
        "Felix and Oscar Together",
        ["OddfeliX", "OddOscar"],
        BATCH3_TAGS + ["digital", "analog", "polarity"],
        "feliX and Oscar — the original couple. Neon shimmer meets axolotl warmth. "
        "The tank contains both. Neither dominates. This is why the suite exists.",
        coupling=0.38,
        dna_override={"brightness": 0.58, "warmth": 0.65, "movement": 0.42, "space": 0.6},
        batch_tags=["batch3-reunion"],
    ),
    make_preset(
        "Dub and Grain",
        ["OVERDUB", "OPAL"],
        BATCH3_TAGS + ["dub", "granular", "texture"],
        "OVERDUB tape warmth + OPAL grain scatter — the reunion of textures. "
        "Tape hiss feeding grain clouds. The studio and the sky.",
        coupling=0.42,
        dna_override={"warmth": 0.72, "space": 0.78, "movement": 0.68, "density": 0.6},
        batch_tags=["batch3-reunion"],
    ),
    make_preset(
        "Machine and Mass",
        ["ONSET", "OBLONG"],
        BATCH3_TAGS + ["drums", "chords", "rhythm"],
        "ONSET percussion + OBLONG chord mass — the rhythm section reunion. "
        "The groove laid, the harmony placed. Production essentials.",
        coupling=0.28,
        dna_override={"density": 0.72, "movement": 0.78, "aggression": 0.52},
        batch_tags=["batch3-reunion"],
    ),
    make_preset(
        "Pixels and Paper",
        ["OVERWORLD", "ORIGAMI"],
        BATCH3_TAGS + ["chip", "delicate", "contrast"],
        "OVERWORLD chip voice + ORIGAMI paper fold — NES geometry meeting "
        "Japanese precision. Pixel and crease. The unlikely duo that works.",
        coupling=0.22,
        dna_override={"brightness": 0.62, "density": 0.45, "space": 0.62},
        batch_tags=["batch3-reunion"],
    ),
    make_preset(
        "Logic and Chaos",
        ["ORGANON", "OUROBOROS"],
        BATCH3_TAGS + ["organic", "chaos", "feedback"],
        "ORGANON biological logic + OUROBOROS Lorenz spiral — the organism "
        "and its shadow. Metabolism meeting entropy. The system and its undoing.",
        coupling=0.55,
        dna_override={"movement": 0.72, "density": 0.65, "space": 0.55},
        batch_tags=["batch3-reunion"],
    ),
    make_preset(
        "Shore and Sky",
        ["OSPREY", "OSTERIA"],
        BATCH3_TAGS + ["Mediterranean", "warm", "outdoor"],
        "OSPREY clean air + OSTERIA table warmth — the Mediterranean reunion. "
        "Sea hawk diving toward the harbor where dinner is ready.",
        coupling=0.32,
        dna_override={"brightness": 0.62, "warmth": 0.72, "space": 0.65},
        batch_tags=["batch3-reunion"],
    ),
    make_preset(
        "Drone and Harp",
        ["OHM", "ORPHICA"],
        BATCH3_TAGS + ["drone", "microsound", "meditative"],
        "OHM commune drone + ORPHICA siphonophore harp — the meditation circle. "
        "Sustained resonance beneath shimmering grain scatter. The practice.",
        coupling=0.4,
        dna_override={"warmth": 0.68, "space": 0.82, "movement": 0.48, "density": 0.45},
        batch_tags=["batch3-reunion"],
    ),
    make_preset(
        "Brass and Drama",
        ["OTTONI", "OLE"],
        BATCH3_TAGS + ["brass", "rhythm", "ensemble"],
        "OTTONI triple brass + OLE Afro-Latin trio — the full ensemble reunion. "
        "GROW meets DRAMA. The most joyful collision in the family.",
        coupling=0.48,
        dna_override={"density": 0.68, "movement": 0.78, "aggression": 0.52, "warmth": 0.62},
        batch_tags=["batch3-reunion"],
    ),
    make_preset(
        "Knot and Rule",
        ["OVERLAP", "OUTWIT"],
        BATCH3_TAGS + ["topology", "cellular-automata", "complex"],
        "OVERLAP FDN knots + OUTWIT Wolfram CA arms — the complex systems reunion. "
        "Topology and cellular automata in conversation. Both are right.",
        coupling=0.5,
        dna_override={"movement": 0.78, "density": 0.68, "space": 0.72, "aggression": 0.4},
        batch_tags=["batch3-reunion"],
    ),
    make_preset(
        "Oracle and Odyssey",
        ["ORACLE", "ODYSSEY"],
        BATCH3_TAGS + ["voice", "drift", "ancient"],
        "ORACLE ancient voice + ODYSSEY open-water drift — the prophet on the water. "
        "Resonance carried on current. Wisdom moving.",
        coupling=0.35,
        dna_override={"space": 0.78, "warmth": 0.56, "movement": 0.62},
        batch_tags=["batch3-reunion"],
    ),
    make_preset(
        "Gradient and Pod",
        ["OMBRE", "ORCA"],
        BATCH3_TAGS + ["whale", "gradient", "deep"],
        "OMBRE watercolor fade + ORCA pod signal — soft surface above, apex hunter below. "
        "The gradient that ends in apex predator.",
        coupling=0.3,
        dna_override={"space": 0.82, "movement": 0.62, "warmth": 0.52},
        batch_tags=["batch3-reunion"],
    ),
    make_preset(
        "Eight Arms and Prism",
        ["OCTOPUS", "OPTIC"],
        BATCH3_TAGS + ["chaos", "light", "intelligence"],
        "OCTOPUS distributed intelligence + OPTIC prismatic light — "
        "the problem solver and the spectrum. Eight arms catching every frequency.",
        coupling=0.42,
        dna_override={"brightness": 0.68, "movement": 0.82, "density": 0.58},
        batch_tags=["batch3-reunion"],
    ),
    make_preset(
        "Oblique and Ocelot",
        ["OBLIQUE", "OCELOT"],
        BATCH3_TAGS + ["agile", "unexpected", "hunter"],
        "OBLIQUE off-axis + OCELOT feline precision — the unexpected angle "
        "and the precise strike. Two ways of not going straight.",
        coupling=0.35,
        dna_override={"movement": 0.72, "aggression": 0.45, "brightness": 0.62},
        batch_tags=["batch3-reunion"],
    ),
    make_preset(
        "Shadow and Veil",
        ["OBSIDIAN", "OBSCURA"],
        BATCH3_TAGS + ["dark", "hidden", "deep"],
        "OBSIDIAN volcanic edge + OBSCURA pinhole dark — the two darknesses meeting. "
        "One cuts, one conceals. The shadow and the veil.",
        coupling=0.38,
        dna_override={"brightness": 0.22, "aggression": 0.55, "space": 0.65, "density": 0.72},
        batch_tags=["batch3-reunion"],
    ),
    make_preset(
        "Orbit and Obligation",
        ["ORBITAL", "OBBLIGATO"],
        BATCH3_TAGS + ["space", "wind", "celestial"],
        "ORBITAL elliptical pulse + OBBLIGATO dual wind — the planet and its breath. "
        "Celestial orbit carried by wind. BOND across the solar system.",
        coupling=0.45,
        dna_override={"space": 0.82, "movement": 0.62, "density": 0.52},
        batch_tags=["batch3-reunion"],
    ),
    make_preset(
        "Ocean and Serpent",
        ["OCEANIC", "OWLFISH"],
        BATCH3_TAGS + ["ocean", "deep", "luminescent"],
        "OCEANIC pressure swell + OWLFISH bioluminescence — the column and the creature. "
        "The deep ocean meeting the thing that lives in it.",
        coupling=0.4,
        dna_override={"space": 0.88, "warmth": 0.62, "movement": 0.55, "density": 0.62},
        batch_tags=["batch3-reunion"],
    ),
    make_preset(
        "Weight and Bite",
        ["OBESE", "OVERBITE"],
        BATCH3_TAGS + ["bass", "sub", "heavy"],
        "OBESE sub weight + OVERBITE warm fang — the low end family reunion. "
        "Mass below, character above. The bass section has arrived.",
        coupling=0.32,
        dna_override={"density": 0.82, "warmth": 0.68, "brightness": 0.25, "aggression": 0.58},
        batch_tags=["batch3-reunion"],
    ),
    make_preset(
        "Hippy Dad and the Axolotl",
        ["OHM", "OddOscar"],
        BATCH3_TAGS + ["drone", "warm", "generational"],
        "OHM commune warmth + OddOscar regenerative bloom — the generational reunion. "
        "Father drone holding the room; Oscar breathing slow underneath.",
        coupling=0.28,
        dna_override={"warmth": 0.82, "space": 0.72, "movement": 0.3, "aggression": 0.1},
        batch_tags=["batch3-reunion"],
    ),
    make_preset(
        "Paper and Neon",
        ["ORIGAMI", "OddfeliX"],
        BATCH3_TAGS + ["digital", "delicate", "contrast"],
        "ORIGAMI paper precision + OddfeliX neon shimmer — "
        "the fold that catches the light. Geometric quiet beneath digital sparkle.",
        coupling=0.25,
        dna_override={"brightness": 0.65, "space": 0.65, "density": 0.4, "movement": 0.48},
        batch_tags=["batch3-reunion"],
    ),
    make_preset(
        "Pixels and Machine",
        ["OVERWORLD", "ONSET"],
        BATCH3_TAGS + ["chip", "drums", "retro"],
        "OVERWORLD chip melody + ONSET percussion — the NES band reunion. "
        "The cartridge that has a drum kit. 8-bit groove, full production.",
        coupling=0.3,
        dna_override={"brightness": 0.62, "movement": 0.82, "density": 0.68, "aggression": 0.52},
        batch_tags=["batch3-reunion"],
    ),
]

# ---------------------------------------------------------------------------
# Write all presets
# ---------------------------------------------------------------------------
def main():
    all_presets = BATCH1 + BATCH2 + BATCH3
    print(f"Writing {len(all_presets)} Family presets to {OUTPUT_DIR}")
    written = 0
    skipped = 0
    for preset in all_presets:
        path = OUTPUT_DIR / filename(preset["name"])
        if path.exists():
            skipped += 1
            print(f"  SKIP (exists): {path.name}")
        else:
            with open(path, "w") as f:
                json.dump(preset, f, indent=2)
            written += 1
            print(f"  WRITE: {path.name}")
    print(f"\nDone. {written} written, {skipped} skipped.")


if __name__ == "__main__":
    main()
