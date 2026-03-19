#!/usr/bin/env python3
"""Shore Engines Coupling Pack — XOmnibus Entangled presets.

Generates coupling presets for the shore trinity:
  OSPREY  (osprey_ prefix, Azulejo Blue #1B4F8A)
  OSTERIA (osteria_ prefix, Porto Wine #722F37)
  OCEANIC (ocean_ prefix,  Phosphorescent Teal #00B4A0)

Outputs:
  - 6  three-way marquee presets (OSPREY × OSTERIA × OCEANIC)
  - 18 OSPREY   × partner presets
  - 18 OSTERIA  × partner presets
  - 18 OCEANIC  × partner presets
Total target: 60 presets written to Presets/XOmnibus/Entangled/
Skips any file that already exists.
"""

import json
import os

REPO_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
ENTANGLED_DIR = os.path.join(REPO_ROOT, "Presets", "XOmnibus", "Entangled")

# ── DNA baselines ────────────────────────────────────────────────────────────

DNA_OSPREY  = {"brightness": 0.65, "warmth": 0.55, "movement": 0.7,
               "density": 0.5,  "space": 0.75, "aggression": 0.35}
DNA_OSTERIA = {"brightness": 0.55, "warmth": 0.8,  "movement": 0.4,
               "density": 0.6,  "space": 0.5,  "aggression": 0.3}
DNA_OCEANIC = {"brightness": 0.6,  "warmth": 0.55, "movement": 0.65,
               "density": 0.55, "space": 0.7,  "aggression": 0.3}

# Partner engine DNA approximations (mid-blend with shore lead)
DNA_PARTNERS = {
    "ONSET":      {"brightness": 0.5,  "warmth": 0.45, "movement": 0.6,  "density": 0.7,  "space": 0.4,  "aggression": 0.6},
    "OBLONG":     {"brightness": 0.6,  "warmth": 0.7,  "movement": 0.45, "density": 0.65, "space": 0.5,  "aggression": 0.35},
    "OVERWORLD":  {"brightness": 0.75, "warmth": 0.35, "movement": 0.55, "density": 0.45, "space": 0.35, "aggression": 0.45},
    "OVERDUB":    {"brightness": 0.45, "warmth": 0.65, "movement": 0.5,  "density": 0.5,  "space": 0.65, "aggression": 0.2},
    "OPAL":       {"brightness": 0.65, "warmth": 0.5,  "movement": 0.75, "density": 0.45, "space": 0.8,  "aggression": 0.2},
    "ORBITAL":    {"brightness": 0.6,  "warmth": 0.45, "movement": 0.65, "density": 0.5,  "space": 0.75, "aggression": 0.25},
    "ORGANON":    {"brightness": 0.55, "warmth": 0.6,  "movement": 0.5,  "density": 0.6,  "space": 0.55, "aggression": 0.3},
    "OUROBOROS":  {"brightness": 0.5,  "warmth": 0.55, "movement": 0.7,  "density": 0.6,  "space": 0.6,  "aggression": 0.4},
    "OBSIDIAN":   {"brightness": 0.35, "warmth": 0.4,  "movement": 0.45, "density": 0.7,  "space": 0.55, "aggression": 0.55},
    "ORACLE":     {"brightness": 0.6,  "warmth": 0.6,  "movement": 0.55, "density": 0.5,  "space": 0.7,  "aggression": 0.25},
    "OPTIC":      {"brightness": 0.8,  "warmth": 0.35, "movement": 0.6,  "density": 0.4,  "space": 0.65, "aggression": 0.35},
    "OBLIQUE":    {"brightness": 0.55, "warmth": 0.5,  "movement": 0.65, "density": 0.5,  "space": 0.6,  "aggression": 0.4},
    "OCELOT":     {"brightness": 0.7,  "warmth": 0.55, "movement": 0.75, "density": 0.45, "space": 0.5,  "aggression": 0.5},
    "OHM":        {"brightness": 0.45, "warmth": 0.7,  "movement": 0.35, "density": 0.55, "space": 0.6,  "aggression": 0.2},
    "ORPHICA":    {"brightness": 0.65, "warmth": 0.55, "movement": 0.8,  "density": 0.35, "space": 0.85, "aggression": 0.15},
    "OBBLIGATO":  {"brightness": 0.55, "warmth": 0.65, "movement": 0.5,  "density": 0.5,  "space": 0.6,  "aggression": 0.25},
    "OTTONI":     {"brightness": 0.6,  "warmth": 0.6,  "movement": 0.55, "density": 0.55, "space": 0.55, "aggression": 0.45},
    "OLE":        {"brightness": 0.7,  "warmth": 0.65, "movement": 0.7,  "density": 0.5,  "space": 0.5,  "aggression": 0.4},
}


def blend_dna(dna_a, dna_b, t=0.5):
    """Linear blend: t=0 → pure A, t=1 → pure B."""
    return {k: round(dna_a[k] * (1 - t) + dna_b[k] * t, 3) for k in dna_a}


def blend_dna3(dna_a, dna_b, dna_c, wa=0.34, wb=0.33, wc=0.33):
    return {k: round(dna_a[k] * wa + dna_b[k] * wb + dna_c[k] * wc, 3) for k in dna_a}


def make_preset(name, engines, coupling_pairs, dna, desc, tags,
                macros=None, intensity="Moderate"):
    if macros is None:
        macros = {"CHARACTER": 0.5, "MOVEMENT": 0.5, "COUPLING": 0.75, "SPACE": 0.5}
    return {
        "schema_version": 1,
        "name": name,
        "mood": "Entangled",
        "engines": engines,
        "author": "XO_OX Designs",
        "version": "1.0.0",
        "description": desc,
        "tags": ["entangled", "coupling", "shore"] + tags,
        "macroLabels": ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"],
        "macros": macros,
        "couplingIntensity": intensity,
        "tempo": None,
        "dna": dna,
        "parameters": {},
        "coupling": {"pairs": coupling_pairs},
        "sequencer": None,
    }


def write_preset(preset):
    os.makedirs(ENTANGLED_DIR, exist_ok=True)
    filename = preset["name"] + ".xometa"
    filepath = os.path.join(ENTANGLED_DIR, filename)
    if os.path.exists(filepath):
        return None  # skip existing
    with open(filepath, "w") as f:
        json.dump(preset, f, indent=2)
        f.write("\n")
    return filepath


# ── 3-Way Marquee Presets (6) ────────────────────────────────────────────────

THREE_WAY = [
    (
        "Shore Trinity",
        ["OSPREY", "OSTERIA", "OCEANIC"],
        [
            {"engineA": "OSPREY",  "engineB": "OSTERIA",  "type": "Env->Filter",    "amount": 0.65},
            {"engineA": "OSTERIA", "engineB": "OCEANIC",  "type": "HARMONIC_BLEND", "amount": 0.6},
            {"engineA": "OCEANIC", "engineB": "OSPREY",   "type": "LFO->Pitch",     "amount": 0.5},
        ],
        blend_dna3(DNA_OSPREY, DNA_OSTERIA, DNA_OCEANIC),
        "The three shore engines in full communion — coastline, cuisine, and current as one.",
        ["trinity", "coastal", "three-way"],
        {"CHARACTER": 0.55, "MOVEMENT": 0.6, "COUPLING": 0.8, "SPACE": 0.65},
        "Deep",
    ),
    (
        "Tidal Table",
        ["OSPREY", "OSTERIA", "OCEANIC"],
        [
            {"engineA": "OCEANIC", "engineB": "OSPREY",  "type": "Env->Morph",     "amount": 0.7},
            {"engineA": "OSPREY",  "engineB": "OSTERIA", "type": "Audio->Filter",  "amount": 0.55},
        ],
        blend_dna3(DNA_OSPREY, DNA_OSTERIA, DNA_OCEANIC, 0.35, 0.45, 0.20),
        "Tidal surge shapes the osprey's dive; the shore feast follows the tide's schedule.",
        ["tidal", "coastal", "warmth"],
        {"CHARACTER": 0.6, "MOVEMENT": 0.5, "COUPLING": 0.7, "SPACE": 0.55},
        "Moderate",
    ),
    (
        "Phosphor Feast",
        ["OSPREY", "OSTERIA", "OCEANIC"],
        [
            {"engineA": "OCEANIC", "engineB": "OSTERIA", "type": "LFO->Amp",       "amount": 0.65},
            {"engineA": "OSTERIA", "engineB": "OSPREY",  "type": "Env->Filter",    "amount": 0.6},
        ],
        blend_dna3(DNA_OSPREY, DNA_OSTERIA, DNA_OCEANIC, 0.2, 0.5, 0.3),
        "Bioluminescent ocean feeds warm tavern warmth; a seabird circles the glow.",
        ["bioluminescent", "warmth", "coastal"],
        {"CHARACTER": 0.5, "MOVEMENT": 0.55, "COUPLING": 0.75, "SPACE": 0.6},
        "Moderate",
    ),
    (
        "Estuary Dawn",
        ["OSPREY", "OSTERIA", "OCEANIC"],
        [
            {"engineA": "OSPREY",  "engineB": "OCEANIC", "type": "Pitch->Filter",  "amount": 0.55},
            {"engineA": "OCEANIC", "engineB": "OSTERIA", "type": "HARMONIC_BLEND", "amount": 0.5},
        ],
        blend_dna3(DNA_OSPREY, DNA_OSTERIA, DNA_OCEANIC, 0.45, 0.2, 0.35),
        "Estuary at first light — salt air, the cook's fire, and the tide returning.",
        ["estuary", "dawn", "ambient"],
        {"CHARACTER": 0.45, "MOVEMENT": 0.5, "COUPLING": 0.65, "SPACE": 0.75},
        "Moderate",
    ),
    (
        "Rocky Shore Rite",
        ["OSPREY", "OSTERIA", "OCEANIC"],
        [
            {"engineA": "OSPREY",  "engineB": "OSTERIA",  "type": "Env->Morph",     "amount": 0.7},
            {"engineA": "OSPREY",  "engineB": "OCEANIC",  "type": "LFO->Filter",    "amount": 0.6},
            {"engineA": "OSTERIA", "engineB": "OCEANIC",  "type": "Env->Amp",       "amount": 0.55},
        ],
        blend_dna3(DNA_OSPREY, DNA_OSTERIA, DNA_OCEANIC, 0.5, 0.25, 0.25),
        "Windswept ritual on the rocks — flight, fire, and foam in deep entanglement.",
        ["ritual", "wind", "rocky"],
        {"CHARACTER": 0.65, "MOVEMENT": 0.7, "COUPLING": 0.85, "SPACE": 0.6},
        "Deep",
    ),
    (
        "Saltwater Solstice",
        ["OSPREY", "OSTERIA", "OCEANIC"],
        [
            {"engineA": "OCEANIC", "engineB": "OSPREY",   "type": "HARMONIC_BLEND", "amount": 0.75},
            {"engineA": "OSTERIA", "engineB": "OSPREY",   "type": "Env->Filter",    "amount": 0.6},
            {"engineA": "OCEANIC", "engineB": "OSTERIA",  "type": "LFO->Pitch",     "amount": 0.5},
        ],
        blend_dna3(DNA_OSPREY, DNA_OSTERIA, DNA_OCEANIC, 0.3, 0.3, 0.4),
        "Midsummer on the coast — maximum sun, maximum tide, maximum feast.",
        ["solstice", "celebration", "coastal"],
        {"CHARACTER": 0.7, "MOVEMENT": 0.65, "COUPLING": 0.8, "SPACE": 0.7},
        "Deep",
    ),
]

# ── Per-engine pairings (18 partners each) ───────────────────────────────────

PARTNERS = [
    "ONSET", "OBLONG", "OVERWORLD", "OVERDUB", "OPAL", "ORBITAL",
    "ORGANON", "OUROBOROS", "OBSIDIAN", "ORACLE", "OPTIC", "OBLIQUE",
    "OCELOT", "OHM", "ORPHICA", "OBBLIGATO", "OTTONI", "OLE",
]

# ── OSPREY pairings ──────────────────────────────────────────────────────────

OSPREY_PAIRS = [
    # (name, partner, coupling_type, amount, desc, extra_tags, macros_override)
    ("Osprey Strike Pattern",
     "ONSET", "Env->Filter", 0.7,
     "Drum hit triggers osprey's plunge; attack and impact fused into one gesture.",
     ["percussive", "attack"], None),

    ("Cliff Nest Sessions",
     "OBLONG", "Audio->Wavetable", 0.6,
     "OBLONG's warm strings nest inside osprey resonance — a cliff-side bower of sound.",
     ["strings", "warm", "nesting"], None),

    ("8-Bit Seabird",
     "OVERWORLD", "LFO->Pitch", 0.65,
     "Chiptune glitch drives osprey pitch swoops; coastal and retro in unlikely harmony.",
     ["chiptune", "glitch", "swooping"], {"CHARACTER": 0.6, "MOVEMENT": 0.75, "COUPLING": 0.7, "SPACE": 0.5}),

    ("Driftwood Delay",
     "OVERDUB", "Env->Amp", 0.6,
     "Tape echo trails follow the seabird's flight path across the bay.",
     ["tape", "echo", "atmospheric"], None),

    ("Granular Coastline",
     "OPAL", "HARMONIC_BLEND", 0.7,
     "Grain clouds scatter like sea spray over osprey's watchful glide.",
     ["granular", "texture", "spray"], {"CHARACTER": 0.5, "MOVEMENT": 0.65, "COUPLING": 0.8, "SPACE": 0.8}),

    ("Coastal Orbit",
     "ORBITAL", "LFO->Filter", 0.65,
     "Osprey thermals align with ORBITAL's slow spectral rotation.",
     ["spectral", "thermal", "drift"], None),

    ("Shore Logic",
     "ORGANON", "Env->Morph", 0.6,
     "ORGANON's modal logic shapes osprey's dive angle — precision meets instinct.",
     ["modal", "precision"], None),

    ("Ouroboros Shore",
     "OUROBOROS", "Audio->Filter", 0.65,
     "The serpent's circular breath underlies osprey's cyclical hunting pattern.",
     ["cyclic", "hunting", "deep"], {"CHARACTER": 0.55, "MOVEMENT": 0.6, "COUPLING": 0.75, "SPACE": 0.65}),

    ("Dark Water Hunter",
     "OBSIDIAN", "Env->Filter", 0.7,
     "Obsidian depths sharpen the seabird's silhouette against a leaden sky.",
     ["dark", "tension", "hunter"], {"CHARACTER": 0.6, "MOVEMENT": 0.55, "COUPLING": 0.75, "SPACE": 0.55}),

    ("Oracle Shore",
     "ORACLE", "LFO->Pitch", 0.55,
     "The oracle reads tide patterns; osprey intercepts the prophecy mid-dive.",
     ["prophetic", "resonant"], None),

    ("Prismatic Wingspan",
     "OPTIC", "Pitch->Filter", 0.65,
     "OPTIC prismatic scatter turns each wingbeat into a spectral event.",
     ["prismatic", "light", "aerial"], {"CHARACTER": 0.55, "MOVEMENT": 0.7, "COUPLING": 0.7, "SPACE": 0.7}),

    ("Oblique Approach",
     "OBLIQUE", "Audio->Wavetable", 0.6,
     "The indirect angle of attack — osprey and OBLIQUE share the diagonal path.",
     ["oblique", "approach", "wind"], None),

    ("Feline Shore",
     "OCELOT", "Env->Amp", 0.65,
     "Cat and raptor share the shoreline at dusk — speed and grace in coupling.",
     ["feline", "predator", "dusk"], {"CHARACTER": 0.65, "MOVEMENT": 0.75, "COUPLING": 0.7, "SPACE": 0.5}),

    ("Commune at the Water",
     "OHM", "HARMONIC_BLEND", 0.55,
     "OHM's meditative drone grounds osprey's altitude — sky meets sea floor.",
     ["meditative", "drone", "grounding"], {"CHARACTER": 0.45, "MOVEMENT": 0.45, "COUPLING": 0.65, "SPACE": 0.75}),

    ("Microsound Wing",
     "ORPHICA", "LFO->Filter", 0.6,
     "Orphica's grain harp scatters in osprey's wake — microsound aerodynamics.",
     ["granular", "aerial", "delicate"], {"CHARACTER": 0.5, "MOVEMENT": 0.65, "COUPLING": 0.7, "SPACE": 0.85}),

    ("Bound for Shore",
     "OBBLIGATO", "Env->Morph", 0.65,
     "OBBLIGATO's obligatory melodic line mirrors the seabird's required route home.",
     ["melodic", "obligatory", "homeward"], None),

    ("Brass Headland",
     "OTTONI", "Audio->Filter", 0.65,
     "Triple brass of OTTONI herald the osprey's return above the headland.",
     ["brass", "herald", "coastal"], {"CHARACTER": 0.6, "MOVEMENT": 0.6, "COUPLING": 0.7, "SPACE": 0.6}),

    ("Coastal Drama",
     "OLE", "HARMONIC_BLEND", 0.65,
     "Afro-Latin rhythm rides beneath osprey's theatric dive — coastal DRAMA.",
     ["afro-latin", "rhythm", "drama"], {"CHARACTER": 0.65, "MOVEMENT": 0.7, "COUPLING": 0.7, "SPACE": 0.55}),
]

# ── OSTERIA pairings ─────────────────────────────────────────────────────────

OSTERIA_PAIRS = [
    ("Kitchen Percussion",
     "ONSET", "Env->Filter", 0.65,
     "Drum hits become kitchen percussion — pots, cleavers, the heartbeat of the shore tavern.",
     ["percussive", "kitchen", "warmth"], {"CHARACTER": 0.55, "MOVEMENT": 0.55, "COUPLING": 0.7, "SPACE": 0.4}),

    ("Warm String Tavern",
     "OBLONG", "Audio->Wavetable", 0.6,
     "OBLONG strings fill the osteria with folk warmth on a cold harbour evening.",
     ["strings", "folk", "warm"], {"CHARACTER": 0.55, "MOVEMENT": 0.4, "COUPLING": 0.65, "SPACE": 0.5}),

    ("Pixel Feast",
     "OVERWORLD", "Env->Amp", 0.5,
     "Chiptune glitch decorates the tavern table — retro colour in a warm room.",
     ["chiptune", "playful", "warm"], None),

    ("Dub Cellar",
     "OVERDUB", "LFO->Filter", 0.65,
     "Tape echo wraps the cellar in warm amber delay — dub bass from the cask.",
     ["tape", "dub", "bass"], {"CHARACTER": 0.5, "MOVEMENT": 0.45, "COUPLING": 0.7, "SPACE": 0.6}),

    ("Grain Wine",
     "OPAL", "HARMONIC_BLEND", 0.6,
     "Grain clouds carry the bouquet of OSTERIA — wine-dark and atmospheric.",
     ["granular", "wine", "aromatic"], {"CHARACTER": 0.5, "MOVEMENT": 0.5, "COUPLING": 0.7, "SPACE": 0.55}),

    ("Orbital Taverna",
     "ORBITAL", "LFO->Pitch", 0.55,
     "ORBITAL's slow rotation turns the taverna ceiling into a slow-moving night sky.",
     ["spectral", "ambient", "circular"], None),

    ("Modal Kitchen",
     "ORGANON", "Env->Morph", 0.6,
     "ORGANON's modal voice becomes the cook's humming — logic hidden in warmth.",
     ["modal", "vocal", "warmth"], None),

    ("Serpent in the Cellar",
     "OUROBOROS", "Audio->Filter", 0.6,
     "Ouroboros cycles beneath floorboards of the osteria — ancient depths in an old building.",
     ["cyclic", "deep", "ancient"], {"CHARACTER": 0.5, "MOVEMENT": 0.45, "COUPLING": 0.7, "SPACE": 0.55}),

    ("Dark Shore Broth",
     "OBSIDIAN", "Env->Filter", 0.65,
     "Obsidian shadow thickens the broth — dark tones enrich the shore feast.",
     ["dark", "deep", "dense"], {"CHARACTER": 0.5, "MOVEMENT": 0.35, "COUPLING": 0.7, "SPACE": 0.5}),

    ("Oracle's Menu",
     "ORACLE", "LFO->Amp", 0.55,
     "Today's catch decided by oracle — resonance reads the tidal charts.",
     ["prophetic", "resonant", "seasonal"], None),

    ("Prism on the Table",
     "OPTIC", "Pitch->Filter", 0.6,
     "OPTIC light through a carafe scatters spectral colour across the tavern table.",
     ["prismatic", "light", "warm"], None),

    ("Oblique Recipe",
     "OBLIQUE", "Audio->Wavetable", 0.55,
     "The unexpected ingredient — OBLIQUE introduces an angle the kitchen didn't plan for.",
     ["oblique", "surprising", "warmth"], None),

    ("Ocelot by the Fire",
     "OCELOT", "Env->Amp", 0.6,
     "The cat curls near the hearth of the osteria — feline warmth by the shore fire.",
     ["feline", "fire", "warmth"], {"CHARACTER": 0.6, "MOVEMENT": 0.45, "COUPLING": 0.65, "SPACE": 0.5}),

    ("Harbour Commune",
     "OHM", "HARMONIC_BLEND", 0.55,
     "OHM's communal drone gathers at the harbour table — everyone eats, everyone hums.",
     ["communal", "drone", "harbour"], {"CHARACTER": 0.45, "MOVEMENT": 0.35, "COUPLING": 0.6, "SPACE": 0.6}),

    ("Harp in the Rafters",
     "ORPHICA", "LFO->Filter", 0.6,
     "Orphica's microsound harp floats in the rafters of the old stone taverna.",
     ["harp", "delicate", "stone"], {"CHARACTER": 0.5, "MOVEMENT": 0.5, "COUPLING": 0.65, "SPACE": 0.7}),

    ("Obligatory Starter",
     "OBBLIGATO", "Env->Morph", 0.6,
     "The obligatory first course — OBBLIGATO sets the harmonic table before the main arrives.",
     ["melodic", "opening", "harmonic"], None),

    ("Brass Announcement",
     "OTTONI", "Audio->Filter", 0.65,
     "OTTONI brass announce the evening's catch from the doorway of the shore taverna.",
     ["brass", "announcement", "warm"], {"CHARACTER": 0.65, "MOVEMENT": 0.5, "COUPLING": 0.7, "SPACE": 0.5}),

    ("Afro Shore Feast",
     "OLE", "HARMONIC_BLEND", 0.7,
     "OLE's Afro-Latin pulse transforms the osteria into a coastal celebration.",
     ["afro-latin", "celebration", "feast"], {"CHARACTER": 0.7, "MOVEMENT": 0.65, "COUPLING": 0.75, "SPACE": 0.5}),
]

# ── OCEANIC pairings ─────────────────────────────────────────────────────────

OCEANIC_PAIRS = [
    ("Wave Machine",
     "ONSET", "Env->Filter", 0.7,
     "Drum machine mapped to wave topology — each hit a different depth of water column.",
     ["percussive", "depth", "waves"], {"CHARACTER": 0.55, "MOVEMENT": 0.65, "COUPLING": 0.75, "SPACE": 0.55}),

    ("Deep String Current",
     "OBLONG", "Audio->Wavetable", 0.65,
     "OBLONG strings drawn into OCEANIC current — warmth beneath the thermocline.",
     ["strings", "deep", "current"], {"CHARACTER": 0.5, "MOVEMENT": 0.5, "COUPLING": 0.7, "SPACE": 0.7}),

    ("Pixel Bioluminescence",
     "OVERWORLD", "LFO->Pitch", 0.6,
     "Chiptune phosphorescence — Overworld glitch lights the midnight water column.",
     ["chiptune", "bioluminescent", "depth"], None),

    ("Dub Pressure",
     "OVERDUB", "Env->Amp", 0.65,
     "Tape dub pressure increases with depth — bass as atmospheric compression.",
     ["tape", "dub", "pressure"], {"CHARACTER": 0.5, "MOVEMENT": 0.55, "COUPLING": 0.75, "SPACE": 0.65}),

    ("Grain Ocean",
     "OPAL", "HARMONIC_BLEND", 0.75,
     "Grain spray over open ocean — OPAL granularity in OCEANIC's phosphor light.",
     ["granular", "spray", "phosphor"], {"CHARACTER": 0.5, "MOVEMENT": 0.7, "COUPLING": 0.8, "SPACE": 0.85}),

    ("Spectral Current",
     "ORBITAL", "LFO->Filter", 0.7,
     "ORBITAL frequency rotation mirrors deep-ocean gyre — spectral and vast.",
     ["spectral", "gyre", "vast"], {"CHARACTER": 0.5, "MOVEMENT": 0.65, "COUPLING": 0.75, "SPACE": 0.8}),

    ("Deep Modal",
     "ORGANON", "Env->Morph", 0.65,
     "ORGANON's modal logic meets abyssal pressure — voices compressed by depth.",
     ["modal", "deep", "compressed"], None),

    ("Serpent Current",
     "OUROBOROS", "Audio->Filter", 0.7,
     "Ouroboros cycles in the open ocean — a current that feeds itself forever.",
     ["cyclic", "ouroboros", "abyssal"], {"CHARACTER": 0.5, "MOVEMENT": 0.7, "COUPLING": 0.8, "SPACE": 0.7}),

    ("Obsidian Trench",
     "OBSIDIAN", "Env->Filter", 0.75,
     "Oceanic phosphorescence against obsidian black of the trench — maximum depth contrast.",
     ["dark", "trench", "abyss"], {"CHARACTER": 0.45, "MOVEMENT": 0.5, "COUPLING": 0.8, "SPACE": 0.7}),

    ("Oracle Tide",
     "ORACLE", "LFO->Pitch", 0.6,
     "Oracle reads the tide table; oceanic pulse obeys the ancient prediction.",
     ["prophetic", "tidal", "resonant"], None),

    ("Prismatic Water Column",
     "OPTIC", "Pitch->Filter", 0.7,
     "OPTIC prisms refract inside the water column — light bending with depth.",
     ["prismatic", "refraction", "depth"], {"CHARACTER": 0.55, "MOVEMENT": 0.65, "COUPLING": 0.75, "SPACE": 0.75}),

    ("Oblique Drift",
     "OBLIQUE", "Audio->Wavetable", 0.6,
     "OBLIQUE's lateral drift catches the surface current — not against the tide, sideways.",
     ["oblique", "drift", "surface"], None),

    ("Pelagic Cat",
     "OCELOT", "Env->Amp", 0.65,
     "The feline leaps — OCELOT's speed meets oceanic roll in open-water play.",
     ["feline", "pelagic", "agile"], {"CHARACTER": 0.6, "MOVEMENT": 0.75, "COUPLING": 0.7, "SPACE": 0.6}),

    ("OHM Depth",
     "OHM", "HARMONIC_BLEND", 0.6,
     "OHM's drone holds the tone of the water column — the ocean's fundamental frequency.",
     ["drone", "fundamental", "depth"], {"CHARACTER": 0.45, "MOVEMENT": 0.4, "COUPLING": 0.7, "SPACE": 0.8}),

    ("Orphica at Depth",
     "ORPHICA", "LFO->Filter", 0.65,
     "Orphica's grain harp dissolves in OCEANIC depth — microsound at pressure.",
     ["granular", "depth", "dissolving"], {"CHARACTER": 0.5, "MOVEMENT": 0.65, "COUPLING": 0.7, "SPACE": 0.85}),

    ("Obligatory Current",
     "OBBLIGATO", "Env->Morph", 0.65,
     "OBBLIGATO's required melodic line runs like a current — inescapable direction.",
     ["melodic", "current", "inevitable"], None),

    ("Brass Below",
     "OTTONI", "Audio->Filter", 0.65,
     "OTTONI brass resonance transforms underwater — pressure changes the timbre.",
     ["brass", "underwater", "resonant"], {"CHARACTER": 0.6, "MOVEMENT": 0.55, "COUPLING": 0.7, "SPACE": 0.65}),

    ("Oceanic Drama",
     "OLE", "HARMONIC_BLEND", 0.7,
     "OLE's Afro-Latin percussion drives OCEANIC swell — the ocean has rhythm, always.",
     ["afro-latin", "swell", "rhythm"], {"CHARACTER": 0.65, "MOVEMENT": 0.7, "COUPLING": 0.75, "SPACE": 0.6}),
]


# ── Build + write ────────────────────────────────────────────────────────────

def build_three_way_presets():
    presets = []
    for (name, engines, coupling_pairs, dna, desc, tags, macros, intensity) in THREE_WAY:
        p = make_preset(name, engines, coupling_pairs, dna, desc, tags, macros, intensity)
        presets.append(p)
    return presets


def build_osprey_presets():
    presets = []
    for (name, partner, ctype, amount, desc, extra_tags, macros) in OSPREY_PAIRS:
        partner_dna = DNA_PARTNERS.get(partner, DNA_PARTNERS["OPAL"])
        dna = blend_dna(DNA_OSPREY, partner_dna, 0.4)
        coupling_pairs = [{"engineA": "OSPREY", "engineB": partner, "type": ctype, "amount": amount}]
        p = make_preset(name, ["OSPREY", partner], coupling_pairs, dna, desc,
                        ["osprey", partner.lower()] + extra_tags,
                        macros,
                        "Deep" if amount >= 0.7 else "Moderate")
        presets.append(p)
    return presets


def build_osteria_presets():
    presets = []
    for (name, partner, ctype, amount, desc, extra_tags, macros) in OSTERIA_PAIRS:
        partner_dna = DNA_PARTNERS.get(partner, DNA_PARTNERS["OPAL"])
        dna = blend_dna(DNA_OSTERIA, partner_dna, 0.4)
        coupling_pairs = [{"engineA": "OSTERIA", "engineB": partner, "type": ctype, "amount": amount}]
        p = make_preset(name, ["OSTERIA", partner], coupling_pairs, dna, desc,
                        ["osteria", partner.lower()] + extra_tags,
                        macros,
                        "Deep" if amount >= 0.7 else "Moderate")
        presets.append(p)
    return presets


def build_oceanic_presets():
    presets = []
    for (name, partner, ctype, amount, desc, extra_tags, macros) in OCEANIC_PAIRS:
        partner_dna = DNA_PARTNERS.get(partner, DNA_PARTNERS["OPAL"])
        dna = blend_dna(DNA_OCEANIC, partner_dna, 0.4)
        coupling_pairs = [{"engineA": "OCEANIC", "engineB": partner, "type": ctype, "amount": amount}]
        p = make_preset(name, ["OCEANIC", partner], coupling_pairs, dna, desc,
                        ["oceanic", partner.lower()] + extra_tags,
                        macros,
                        "Deep" if amount >= 0.7 else "Moderate")
        presets.append(p)
    return presets


def main():
    all_presets = (
        build_three_way_presets()
        + build_osprey_presets()
        + build_osteria_presets()
        + build_oceanic_presets()
    )

    written = 0
    skipped = 0
    for p in all_presets:
        result = write_preset(p)
        if result:
            print(f"  WROTE   {p['name']}")
            written += 1
        else:
            print(f"  SKIPPED {p['name']}  (already exists)")
            skipped += 1

    print(f"\nDone. {written} written, {skipped} skipped. Total: {len(all_presets)} presets.")


if __name__ == "__main__":
    main()
