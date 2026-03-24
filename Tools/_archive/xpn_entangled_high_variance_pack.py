#!/usr/bin/env python3
"""
xpn_entangled_high_variance_pack.py

Generates 60 Entangled presets with extreme DNA values to break the
clustering problem (current diversity score 0.057).

Each preset has at least 2 DNA dimensions locked into extreme zones
(0.0–0.2 or 0.8–1.0), forcing the fleet out of the 50-60/40-50 midrange rut.

4 Strategies × 15 presets each = 60 total:
  1. Bright Dark  — brightness 0.9+, warmth 0.1
  2. Warm Sparse  — warmth 0.9+, density 0.1
  3. Fast Cold    — movement 0.9+, warmth 0.1
  4. Still Aggressive — aggression 0.9+, movement 0.05
"""

import json
import os
import random

OUTPUT_DIR = os.path.join(
    os.path.dirname(__file__),
    "..", "Presets", "XOlokun", "Entangled"
)

# ---------------------------------------------------------------------------
# Strategy 1 — Bright Dark
# brightness 0.9+, warmth 0.1  (cold, crystalline radiance)
# ---------------------------------------------------------------------------
BRIGHT_DARK_PAIRS = [
    ("OPTIC", "ORACLE"),
    ("OVERWORLD", "OMBRE"),
    ("ODDFELIX", "OBSIDIAN"),
    ("ORIGAMI", "OVERDUB"),
    ("ONSET", "OBSCURA"),
]

BRIGHT_DARK_PRESETS = [
    # OPTIC + ORACLE (3 presets)
    {
        "name": "Glass Prophecy",
        "engines": ["OPTIC", "ORACLE"],
        "macros": {"CHARACTER": 0.8, "MOVEMENT": 0.4, "COUPLING": 0.8, "SPACE": 0.7},
        "sonic_dna": {"brightness": 0.93, "warmth": 0.08, "movement": 0.45, "density": 0.6, "space": 0.72, "aggression": 0.25},
        "coupling": {"type": "SPECTRAL_FOLD", "source": "OPTIC", "target": "ORACLE", "amount": 0.72},
        "tags": ["entangled", "bright", "cold", "crystalline", "prophetic"],
        "description": "Optic's icy prism splits Oracle's visions into cold glass harmonics.",
    },
    {
        "name": "Frozen Oracle",
        "engines": ["OPTIC", "ORACLE"],
        "macros": {"CHARACTER": 0.7, "MOVEMENT": 0.3, "COUPLING": 0.85, "SPACE": 0.8},
        "sonic_dna": {"brightness": 0.91, "warmth": 0.06, "movement": 0.3, "density": 0.4, "space": 0.82, "aggression": 0.15},
        "coupling": {"type": "HARMONIC_BLEND", "source": "ORACLE", "target": "OPTIC", "amount": 0.65},
        "tags": ["entangled", "bright", "cold", "sparse", "oracular"],
        "description": "Prophecy suspended in Arctic light — future seen through a frozen lens.",
    },
    {
        "name": "Prismatic Seer",
        "engines": ["OPTIC", "ORACLE"],
        "macros": {"CHARACTER": 0.85, "MOVEMENT": 0.55, "COUPLING": 0.7, "SPACE": 0.65},
        "sonic_dna": {"brightness": 0.96, "warmth": 0.09, "movement": 0.58, "density": 0.5, "space": 0.65, "aggression": 0.35},
        "coupling": {"type": "PHASE_LOCK", "source": "OPTIC", "target": "ORACLE", "amount": 0.6},
        "tags": ["entangled", "bright", "cold", "movement", "visionary"],
        "description": "Optic's prismatic scatter drives Oracle's prophecy into spectral motion.",
    },
    # OVERWORLD + OMBRE (3 presets)
    {
        "name": "Chip Ice Cathedral",
        "engines": ["OVERWORLD", "OMBRE"],
        "macros": {"CHARACTER": 0.6, "MOVEMENT": 0.45, "COUPLING": 0.75, "SPACE": 0.9},
        "sonic_dna": {"brightness": 0.92, "warmth": 0.07, "movement": 0.4, "density": 0.35, "space": 0.88, "aggression": 0.2},
        "coupling": {"type": "HARMONIC_BLEND", "source": "OVERWORLD", "target": "OMBRE", "amount": 0.68},
        "tags": ["entangled", "bright", "cold", "chiptune", "spacious"],
        "description": "NES crystallinity meets Ombre's gradient shadow in a vast frozen nave.",
    },
    {
        "name": "8-Bit Moonrise",
        "engines": ["OVERWORLD", "OMBRE"],
        "macros": {"CHARACTER": 0.75, "MOVEMENT": 0.6, "COUPLING": 0.65, "SPACE": 0.75},
        "sonic_dna": {"brightness": 0.9, "warmth": 0.1, "movement": 0.62, "density": 0.45, "space": 0.7, "aggression": 0.28},
        "coupling": {"type": "TIMBRAL_WARP", "source": "OMBRE", "target": "OVERWORLD", "amount": 0.55},
        "tags": ["entangled", "bright", "cold", "chiptune", "gradient"],
        "description": "Retro chip brightness sliding through Ombre's cool gradient spectrum.",
    },
    {
        "name": "Frost Gradient",
        "engines": ["OVERWORLD", "OMBRE"],
        "macros": {"CHARACTER": 0.5, "MOVEMENT": 0.35, "COUPLING": 0.9, "SPACE": 0.8},
        "sonic_dna": {"brightness": 0.94, "warmth": 0.06, "movement": 0.32, "density": 0.55, "space": 0.78, "aggression": 0.18},
        "coupling": {"type": "SPECTRAL_FOLD", "source": "OVERWORLD", "target": "OMBRE", "amount": 0.82},
        "tags": ["entangled", "bright", "cold", "dense-mid", "still"],
        "description": "Deep coupling freezes OVERWORLD's chip harmonics into Ombre's tonal gradients.",
    },
    # ODDFELIX + OBSIDIAN (3 presets)
    {
        "name": "Felix Black Mirror",
        "engines": ["ODDFELIX", "OBSIDIAN"],
        "macros": {"CHARACTER": 0.9, "MOVEMENT": 0.4, "COUPLING": 0.8, "SPACE": 0.5},
        "sonic_dna": {"brightness": 0.91, "warmth": 0.08, "movement": 0.42, "density": 0.7, "space": 0.5, "aggression": 0.5},
        "coupling": {"type": "PHASE_LOCK", "source": "ODDFELIX", "target": "OBSIDIAN", "amount": 0.75},
        "tags": ["entangled", "bright", "cold", "character", "obsidian"],
        "description": "OddFelix's bright eccentricity reflected back through Obsidian's volcanic black.",
    },
    {
        "name": "Volcanic Glare",
        "engines": ["ODDFELIX", "OBSIDIAN"],
        "macros": {"CHARACTER": 0.85, "MOVEMENT": 0.5, "COUPLING": 0.7, "SPACE": 0.45},
        "sonic_dna": {"brightness": 0.95, "warmth": 0.07, "movement": 0.5, "density": 0.65, "space": 0.45, "aggression": 0.6},
        "coupling": {"type": "HARMONIC_BLEND", "source": "OBSIDIAN", "target": "ODDFELIX", "amount": 0.6},
        "tags": ["entangled", "bright", "cold", "aggressive", "volcanic"],
        "description": "Cold white light striking obsidian — blinding and hard-edged.",
    },
    {
        "name": "Glass Fault Line",
        "engines": ["ODDFELIX", "OBSIDIAN"],
        "macros": {"CHARACTER": 0.7, "MOVEMENT": 0.65, "COUPLING": 0.75, "SPACE": 0.55},
        "sonic_dna": {"brightness": 0.9, "warmth": 0.09, "movement": 0.68, "density": 0.6, "space": 0.55, "aggression": 0.45},
        "coupling": {"type": "TIMBRAL_WARP", "source": "ODDFELIX", "target": "OBSIDIAN", "amount": 0.65},
        "tags": ["entangled", "bright", "cold", "movement", "tension"],
        "description": "Shatter-bright motion running along the fault between light and obsidian dark.",
    },
    # ORIGAMI + OVERDUB (3 presets)
    {
        "name": "Paper Echo Freeze",
        "engines": ["ORIGAMI", "OVERDUB"],
        "macros": {"CHARACTER": 0.6, "MOVEMENT": 0.35, "COUPLING": 0.8, "SPACE": 0.75},
        "sonic_dna": {"brightness": 0.92, "warmth": 0.08, "movement": 0.33, "density": 0.4, "space": 0.72, "aggression": 0.15},
        "coupling": {"type": "HARMONIC_BLEND", "source": "ORIGAMI", "target": "OVERDUB", "amount": 0.7},
        "tags": ["entangled", "bright", "cold", "paper", "delay"],
        "description": "Origami's crisp paper folds echoed cold through Overdub's tape delay.",
    },
    {
        "name": "Dub Crystal Fold",
        "engines": ["ORIGAMI", "OVERDUB"],
        "macros": {"CHARACTER": 0.65, "MOVEMENT": 0.5, "COUPLING": 0.75, "SPACE": 0.85},
        "sonic_dna": {"brightness": 0.95, "warmth": 0.06, "movement": 0.52, "density": 0.3, "space": 0.83, "aggression": 0.2},
        "coupling": {"type": "SPECTRAL_FOLD", "source": "OVERDUB", "target": "ORIGAMI", "amount": 0.6},
        "tags": ["entangled", "bright", "cold", "sparse", "spacious"],
        "description": "Geometric folds shimmering in cavernous dub space — sharp cold light.",
    },
    {
        "name": "Crane Delay Shrine",
        "engines": ["ORIGAMI", "OVERDUB"],
        "macros": {"CHARACTER": 0.55, "MOVEMENT": 0.45, "COUPLING": 0.85, "SPACE": 0.9},
        "sonic_dna": {"brightness": 0.9, "warmth": 0.1, "movement": 0.47, "density": 0.35, "space": 0.88, "aggression": 0.12},
        "coupling": {"type": "PHASE_LOCK", "source": "ORIGAMI", "target": "OVERDUB", "amount": 0.78},
        "tags": ["entangled", "bright", "cold", "meditative", "spacious"],
        "description": "A thousand paper cranes folded in cold cathedral reverb — still and luminous.",
    },
    # ONSET + OBSCURA (3 presets)
    {
        "name": "Drum Obscura Flash",
        "engines": ["ONSET", "OBSCURA"],
        "macros": {"CHARACTER": 0.75, "MOVEMENT": 0.7, "COUPLING": 0.7, "SPACE": 0.5},
        "sonic_dna": {"brightness": 0.93, "warmth": 0.07, "movement": 0.72, "density": 0.65, "space": 0.52, "aggression": 0.55},
        "coupling": {"type": "TIMBRAL_WARP", "source": "ONSET", "target": "OBSCURA", "amount": 0.65},
        "tags": ["entangled", "bright", "cold", "percussive", "obscure"],
        "description": "Onset's transient flash illuminating Obscura's shadowed chambers in cold white.",
    },
    {
        "name": "Stroboscopic Ritual",
        "engines": ["ONSET", "OBSCURA"],
        "macros": {"CHARACTER": 0.8, "MOVEMENT": 0.8, "COUPLING": 0.65, "SPACE": 0.55},
        "sonic_dna": {"brightness": 0.96, "warmth": 0.08, "movement": 0.82, "density": 0.7, "space": 0.5, "aggression": 0.65},
        "coupling": {"type": "PHASE_LOCK", "source": "ONSET", "target": "OBSCURA", "amount": 0.58},
        "tags": ["entangled", "bright", "cold", "aggressive", "ritual"],
        "description": "Percussion strobes carving cold light into darkness — mechanized and relentless.",
    },
    {
        "name": "Frozen Strike",
        "engines": ["ONSET", "OBSCURA"],
        "macros": {"CHARACTER": 0.65, "MOVEMENT": 0.6, "COUPLING": 0.75, "SPACE": 0.65},
        "sonic_dna": {"brightness": 0.9, "warmth": 0.09, "movement": 0.6, "density": 0.55, "space": 0.62, "aggression": 0.42},
        "coupling": {"type": "SPECTRAL_FOLD", "source": "OBSCURA", "target": "ONSET", "amount": 0.68},
        "tags": ["entangled", "bright", "cold", "percussive", "crystalline"],
        "description": "Each drum strike crystallised in Obscura's cold refraction.",
    },
]

# ---------------------------------------------------------------------------
# Strategy 2 — Warm Sparse
# warmth 0.9+, density 0.1  (rich tone, wide open space)
# ---------------------------------------------------------------------------
WARM_SPARSE_PAIRS = [
    ("OHM", "OPAL"),
    ("ORPHICA", "OSPREY"),
    ("OSTERIA", "OCEANIC"),
    ("OLE", "OVERDUB"),
    ("OTTONI", "ORGANON"),
]

WARM_SPARSE_PRESETS = [
    # OHM + OPAL (3 presets)
    {
        "name": "Commune of Grains",
        "engines": ["OHM", "OPAL"],
        "macros": {"CHARACTER": 0.5, "MOVEMENT": 0.4, "COUPLING": 0.75, "SPACE": 0.85},
        "sonic_dna": {"brightness": 0.45, "warmth": 0.93, "movement": 0.38, "density": 0.08, "space": 0.82, "aggression": 0.1},
        "coupling": {"type": "GRANULAR_SYNC", "source": "OHM", "target": "OPAL", "amount": 0.7},
        "tags": ["entangled", "warm", "sparse", "granular", "commune"],
        "description": "OHM's communal warmth scattered into Opal's granular mist — sparse and glowing.",
    },
    {
        "name": "Hippie Dust Cloud",
        "engines": ["OHM", "OPAL"],
        "macros": {"CHARACTER": 0.45, "MOVEMENT": 0.55, "COUPLING": 0.7, "SPACE": 0.9},
        "sonic_dna": {"brightness": 0.38, "warmth": 0.95, "movement": 0.55, "density": 0.09, "space": 0.88, "aggression": 0.08},
        "coupling": {"type": "HARMONIC_BLEND", "source": "OPAL", "target": "OHM", "amount": 0.62},
        "tags": ["entangled", "warm", "sparse", "movement", "dreamy"],
        "description": "Warm communal drones dissolving into slow granular drift — vast and inviting.",
    },
    {
        "name": "Meddling Warmth",
        "engines": ["OHM", "OPAL"],
        "macros": {"CHARACTER": 0.55, "MOVEMENT": 0.3, "COUPLING": 0.85, "SPACE": 0.78},
        "sonic_dna": {"brightness": 0.4, "warmth": 0.91, "movement": 0.28, "density": 0.07, "space": 0.75, "aggression": 0.12},
        "coupling": {"type": "SPECTRAL_FOLD", "source": "OHM", "target": "OPAL", "amount": 0.8},
        "tags": ["entangled", "warm", "sparse", "meditative", "ohm"],
        "description": "OHM's meddling warmth deeply folded into Opal's open grain field.",
    },
    # ORPHICA + OSPREY (3 presets)
    {
        "name": "Harp Thermals",
        "engines": ["ORPHICA", "OSPREY"],
        "macros": {"CHARACTER": 0.6, "MOVEMENT": 0.65, "COUPLING": 0.72, "SPACE": 0.88},
        "sonic_dna": {"brightness": 0.5, "warmth": 0.92, "movement": 0.62, "density": 0.1, "space": 0.85, "aggression": 0.1},
        "coupling": {"type": "TIMBRAL_WARP", "source": "ORPHICA", "target": "OSPREY", "amount": 0.65},
        "tags": ["entangled", "warm", "sparse", "harp", "soaring"],
        "description": "Orphica's microsound harp rising on Osprey's warm thermal columns.",
    },
    {
        "name": "Siphon Glide",
        "engines": ["ORPHICA", "OSPREY"],
        "macros": {"CHARACTER": 0.55, "MOVEMENT": 0.5, "COUPLING": 0.8, "SPACE": 0.82},
        "sonic_dna": {"brightness": 0.42, "warmth": 0.9, "movement": 0.48, "density": 0.08, "space": 0.8, "aggression": 0.07},
        "coupling": {"type": "PHASE_LOCK", "source": "OSPREY", "target": "ORPHICA", "amount": 0.72},
        "tags": ["entangled", "warm", "sparse", "gliding", "delicate"],
        "description": "Siphonophore warmth gliding through Osprey's open skies — weightless and amber.",
    },
    {
        "name": "Raptor Resonance",
        "engines": ["ORPHICA", "OSPREY"],
        "macros": {"CHARACTER": 0.65, "MOVEMENT": 0.7, "COUPLING": 0.68, "SPACE": 0.75},
        "sonic_dna": {"brightness": 0.55, "warmth": 0.94, "movement": 0.7, "density": 0.09, "space": 0.72, "aggression": 0.15},
        "coupling": {"type": "HARMONIC_BLEND", "source": "ORPHICA", "target": "OSPREY", "amount": 0.6},
        "tags": ["entangled", "warm", "sparse", "movement", "raptor"],
        "description": "Orphica's warm microsound resonating against Osprey's hunting arc.",
    },
    # OSTERIA + OCEANIC (3 presets)
    {
        "name": "Tavern Tide",
        "engines": ["OSTERIA", "OCEANIC"],
        "macros": {"CHARACTER": 0.7, "MOVEMENT": 0.35, "COUPLING": 0.78, "SPACE": 0.8},
        "sonic_dna": {"brightness": 0.35, "warmth": 0.93, "movement": 0.32, "density": 0.08, "space": 0.78, "aggression": 0.1},
        "coupling": {"type": "SPECTRAL_FOLD", "source": "OSTERIA", "target": "OCEANIC", "amount": 0.72},
        "tags": ["entangled", "warm", "sparse", "coastal", "nostalgic"],
        "description": "Osteria's warm hearth melting into Oceanic's open horizon — salt and embers.",
    },
    {
        "name": "Shore Siesta",
        "engines": ["OSTERIA", "OCEANIC"],
        "macros": {"CHARACTER": 0.6, "MOVEMENT": 0.25, "COUPLING": 0.85, "SPACE": 0.88},
        "sonic_dna": {"brightness": 0.3, "warmth": 0.96, "movement": 0.22, "density": 0.07, "space": 0.85, "aggression": 0.05},
        "coupling": {"type": "HARMONIC_BLEND", "source": "OCEANIC", "target": "OSTERIA", "amount": 0.78},
        "tags": ["entangled", "warm", "sparse", "still", "siesta"],
        "description": "An afternoon siesta at the water's edge — warm, nearly still, endlessly open.",
    },
    {
        "name": "Pelagic Hearth",
        "engines": ["OSTERIA", "OCEANIC"],
        "macros": {"CHARACTER": 0.65, "MOVEMENT": 0.45, "COUPLING": 0.72, "SPACE": 0.85},
        "sonic_dna": {"brightness": 0.4, "warmth": 0.91, "movement": 0.45, "density": 0.1, "space": 0.82, "aggression": 0.08},
        "coupling": {"type": "PHASE_LOCK", "source": "OSTERIA", "target": "OCEANIC", "amount": 0.65},
        "tags": ["entangled", "warm", "sparse", "depth", "hearth"],
        "description": "Deep-sea warmth radiating from a pelagic hearth — absurd and beautiful.",
    },
    # OLE + OVERDUB (3 presets)
    {
        "name": "Drama Delay",
        "engines": ["OLE", "OVERDUB"],
        "macros": {"CHARACTER": 0.85, "MOVEMENT": 0.5, "COUPLING": 0.75, "SPACE": 0.72},
        "sonic_dna": {"brightness": 0.48, "warmth": 0.92, "movement": 0.5, "density": 0.09, "space": 0.7, "aggression": 0.15},
        "coupling": {"type": "TIMBRAL_WARP", "source": "OLE", "target": "OVERDUB", "amount": 0.68},
        "tags": ["entangled", "warm", "sparse", "afro-latin", "drama"],
        "description": "Olé's DRAMA axis trailing warm echoes through Overdub's vast dub space.",
    },
    {
        "name": "Afro Amber Echo",
        "engines": ["OLE", "OVERDUB"],
        "macros": {"CHARACTER": 0.9, "MOVEMENT": 0.4, "COUPLING": 0.8, "SPACE": 0.8},
        "sonic_dna": {"brightness": 0.42, "warmth": 0.95, "movement": 0.38, "density": 0.08, "space": 0.78, "aggression": 0.12},
        "coupling": {"type": "HARMONIC_BLEND", "source": "OVERDUB", "target": "OLE", "amount": 0.72},
        "tags": ["entangled", "warm", "sparse", "afro-latin", "dub"],
        "description": "Olé's fiery warmth stretched into amber dub delays — sparse and deeply soulful.",
    },
    {
        "name": "Latin Reverb Soul",
        "engines": ["OLE", "OVERDUB"],
        "macros": {"CHARACTER": 0.8, "MOVEMENT": 0.6, "COUPLING": 0.7, "SPACE": 0.88},
        "sonic_dna": {"brightness": 0.45, "warmth": 0.9, "movement": 0.62, "density": 0.1, "space": 0.85, "aggression": 0.1},
        "coupling": {"type": "SPECTRAL_FOLD", "source": "OLE", "target": "OVERDUB", "amount": 0.6},
        "tags": ["entangled", "warm", "sparse", "movement", "soulful"],
        "description": "Latin rhythm warmth evaporating into wide reverb space — presence without clutter.",
    },
    # OTTONI + ORGANON (3 presets)
    {
        "name": "Brass Theology",
        "engines": ["OTTONI", "ORGANON"],
        "macros": {"CHARACTER": 0.7, "MOVEMENT": 0.35, "COUPLING": 0.82, "SPACE": 0.75},
        "sonic_dna": {"brightness": 0.5, "warmth": 0.93, "movement": 0.32, "density": 0.09, "space": 0.72, "aggression": 0.2},
        "coupling": {"type": "HARMONIC_BLEND", "source": "OTTONI", "target": "ORGANON", "amount": 0.75},
        "tags": ["entangled", "warm", "sparse", "brass", "organ"],
        "description": "Ottoni's triple brass warmth feeding Organon's theological resonance — sparse and stately.",
    },
    {
        "name": "Grow Slow Choir",
        "engines": ["OTTONI", "ORGANON"],
        "macros": {"CHARACTER": 0.65, "MOVEMENT": 0.45, "COUPLING": 0.78, "SPACE": 0.82},
        "sonic_dna": {"brightness": 0.45, "warmth": 0.91, "movement": 0.45, "density": 0.08, "space": 0.8, "aggression": 0.12},
        "coupling": {"type": "TIMBRAL_WARP", "source": "ORGANON", "target": "OTTONI", "amount": 0.68},
        "tags": ["entangled", "warm", "sparse", "choral", "evolving"],
        "description": "GROW macro expanding Ottoni's warmth through Organon's slow choral logic.",
    },
    {
        "name": "Resonant Brass Void",
        "engines": ["OTTONI", "ORGANON"],
        "macros": {"CHARACTER": 0.6, "MOVEMENT": 0.3, "COUPLING": 0.88, "SPACE": 0.9},
        "sonic_dna": {"brightness": 0.38, "warmth": 0.95, "movement": 0.28, "density": 0.07, "space": 0.88, "aggression": 0.08},
        "coupling": {"type": "PHASE_LOCK", "source": "OTTONI", "target": "ORGANON", "amount": 0.82},
        "tags": ["entangled", "warm", "sparse", "meditative", "brass"],
        "description": "Three brass voices locked into Organon's vast resonant void — warm and almost empty.",
    },
]

# ---------------------------------------------------------------------------
# Strategy 3 — Fast Cold
# movement 0.9+, warmth 0.1  (kinetic and icy)
# ---------------------------------------------------------------------------
FAST_COLD_PAIRS = [
    ("OUROBOROS", "OPTIC"),
    ("OBLIQUE", "ONSET"),
    ("OCTOPUS", "OVERWORLD"),
    ("ORCA", "ODDFELIX"),
    ("OCELOT", "ORIGAMI"),
]

FAST_COLD_PRESETS = [
    # OUROBOROS + OPTIC (3 presets)
    {
        "name": "Serpent Light Speed",
        "engines": ["OUROBOROS", "OPTIC"],
        "macros": {"CHARACTER": 0.7, "MOVEMENT": 0.95, "COUPLING": 0.75, "SPACE": 0.55},
        "sonic_dna": {"brightness": 0.75, "warmth": 0.08, "movement": 0.93, "density": 0.5, "space": 0.52, "aggression": 0.6},
        "coupling": {"type": "PHASE_LOCK", "source": "OUROBOROS", "target": "OPTIC", "amount": 0.7},
        "tags": ["entangled", "cold", "fast", "cyclical", "kinetic"],
        "description": "Ouroboros cycles spinning at light speed through Optic's cold prismatic field.",
    },
    {
        "name": "Infinite Cold Loop",
        "engines": ["OUROBOROS", "OPTIC"],
        "macros": {"CHARACTER": 0.65, "MOVEMENT": 1.0, "COUPLING": 0.7, "SPACE": 0.5},
        "sonic_dna": {"brightness": 0.7, "warmth": 0.07, "movement": 0.98, "density": 0.55, "space": 0.48, "aggression": 0.7},
        "coupling": {"type": "TIMBRAL_WARP", "source": "OPTIC", "target": "OUROBOROS", "amount": 0.65},
        "tags": ["entangled", "cold", "fast", "infinite", "aggressive"],
        "description": "The ouroboros consumes itself in cold refracted light — absolute maximum velocity.",
    },
    {
        "name": "Cryo Serpentine",
        "engines": ["OUROBOROS", "OPTIC"],
        "macros": {"CHARACTER": 0.75, "MOVEMENT": 0.92, "COUPLING": 0.8, "SPACE": 0.6},
        "sonic_dna": {"brightness": 0.8, "warmth": 0.09, "movement": 0.92, "density": 0.45, "space": 0.58, "aggression": 0.55},
        "coupling": {"type": "SPECTRAL_FOLD", "source": "OUROBOROS", "target": "OPTIC", "amount": 0.75},
        "tags": ["entangled", "cold", "fast", "serpentine", "prism"],
        "description": "Frozen snake motion fracturing into prismatic cold at maximum cycle speed.",
    },
    # OBLIQUE + ONSET (3 presets)
    {
        "name": "Angular Percussion Storm",
        "engines": ["OBLIQUE", "ONSET"],
        "macros": {"CHARACTER": 0.75, "MOVEMENT": 0.93, "COUPLING": 0.72, "SPACE": 0.45},
        "sonic_dna": {"brightness": 0.65, "warmth": 0.07, "movement": 0.93, "density": 0.7, "space": 0.42, "aggression": 0.75},
        "coupling": {"type": "HARMONIC_BLEND", "source": "OBLIQUE", "target": "ONSET", "amount": 0.68},
        "tags": ["entangled", "cold", "fast", "angular", "percussive"],
        "description": "Oblique's angular cuts driving Onset's drums into a cold relentless storm.",
    },
    {
        "name": "Cold Machine Drill",
        "engines": ["OBLIQUE", "ONSET"],
        "macros": {"CHARACTER": 0.8, "MOVEMENT": 0.97, "COUPLING": 0.68, "SPACE": 0.4},
        "sonic_dna": {"brightness": 0.6, "warmth": 0.06, "movement": 0.97, "density": 0.75, "space": 0.38, "aggression": 0.82},
        "coupling": {"type": "PHASE_LOCK", "source": "ONSET", "target": "OBLIQUE", "amount": 0.62},
        "tags": ["entangled", "cold", "fast", "aggressive", "industrial"],
        "description": "Oblique mechanics drilling cold percussion at near-maximum velocity.",
    },
    {
        "name": "Slant Transient Hail",
        "engines": ["OBLIQUE", "ONSET"],
        "macros": {"CHARACTER": 0.7, "MOVEMENT": 0.9, "COUPLING": 0.75, "SPACE": 0.5},
        "sonic_dna": {"brightness": 0.68, "warmth": 0.09, "movement": 0.9, "density": 0.65, "space": 0.48, "aggression": 0.68},
        "coupling": {"type": "TIMBRAL_WARP", "source": "OBLIQUE", "target": "ONSET", "amount": 0.7},
        "tags": ["entangled", "cold", "fast", "transient", "kinetic"],
        "description": "Transient hail falling at a slant — cold, dense, oblique and unrelenting.",
    },
    # OCTOPUS + OVERWORLD (3 presets)
    {
        "name": "8-Arm Pixel Blitz",
        "engines": ["OCTOPUS", "OVERWORLD"],
        "macros": {"CHARACTER": 0.8, "MOVEMENT": 0.95, "COUPLING": 0.7, "SPACE": 0.5},
        "sonic_dna": {"brightness": 0.72, "warmth": 0.08, "movement": 0.95, "density": 0.6, "space": 0.48, "aggression": 0.65},
        "coupling": {"type": "SPECTRAL_FOLD", "source": "OCTOPUS", "target": "OVERWORLD", "amount": 0.65},
        "tags": ["entangled", "cold", "fast", "octopus", "chiptune"],
        "description": "Eight CA arms controlling OVERWORLD's chip oscillators at blitz speed — cold and hyper.",
    },
    {
        "name": "Cephalopod Overdrive",
        "engines": ["OCTOPUS", "OVERWORLD"],
        "macros": {"CHARACTER": 0.85, "MOVEMENT": 0.98, "COUPLING": 0.65, "SPACE": 0.42},
        "sonic_dna": {"brightness": 0.78, "warmth": 0.06, "movement": 0.98, "density": 0.68, "space": 0.4, "aggression": 0.78},
        "coupling": {"type": "PHASE_LOCK", "source": "OVERWORLD", "target": "OCTOPUS", "amount": 0.6},
        "tags": ["entangled", "cold", "fast", "aggressive", "overdrive"],
        "description": "Cephalopod intelligence at maximum overdrive — chip reality fractured by cold speed.",
    },
    {
        "name": "Wolfram Pixel Rush",
        "engines": ["OCTOPUS", "OVERWORLD"],
        "macros": {"CHARACTER": 0.75, "MOVEMENT": 0.92, "COUPLING": 0.72, "SPACE": 0.55},
        "sonic_dna": {"brightness": 0.7, "warmth": 0.09, "movement": 0.91, "density": 0.55, "space": 0.52, "aggression": 0.6},
        "coupling": {"type": "HARMONIC_BLEND", "source": "OCTOPUS", "target": "OVERWORLD", "amount": 0.68},
        "tags": ["entangled", "cold", "fast", "ca", "chiptune"],
        "description": "Wolfram cellular automata rushing through NES registers at icy velocity.",
    },
    # ORCA + ODDFELIX (3 presets)
    {
        "name": "Echolocation Sprint",
        "engines": ["ORCA", "ODDFELIX"],
        "macros": {"CHARACTER": 0.85, "MOVEMENT": 0.94, "COUPLING": 0.72, "SPACE": 0.52},
        "sonic_dna": {"brightness": 0.65, "warmth": 0.07, "movement": 0.94, "density": 0.5, "space": 0.5, "aggression": 0.6},
        "coupling": {"type": "TIMBRAL_WARP", "source": "ORCA", "target": "ODDFELIX", "amount": 0.68},
        "tags": ["entangled", "cold", "fast", "orca", "echolocation"],
        "description": "Orca's cold echolocation clicks merged with OddFelix's eccentric sprint.",
    },
    {
        "name": "Pod Velocity",
        "engines": ["ORCA", "ODDFELIX"],
        "macros": {"CHARACTER": 0.9, "MOVEMENT": 0.96, "COUPLING": 0.68, "SPACE": 0.45},
        "sonic_dna": {"brightness": 0.6, "warmth": 0.08, "movement": 0.96, "density": 0.58, "space": 0.43, "aggression": 0.7},
        "coupling": {"type": "SPECTRAL_FOLD", "source": "ODDFELIX", "target": "ORCA", "amount": 0.62},
        "tags": ["entangled", "cold", "fast", "pod", "eccentric"],
        "description": "The entire pod moving at OddFelix-driven speeds through Arctic cold.",
    },
    {
        "name": "Arctic Eccentric Hunt",
        "engines": ["ORCA", "ODDFELIX"],
        "macros": {"CHARACTER": 0.8, "MOVEMENT": 0.91, "COUPLING": 0.75, "SPACE": 0.55},
        "sonic_dna": {"brightness": 0.62, "warmth": 0.09, "movement": 0.91, "density": 0.45, "space": 0.52, "aggression": 0.55},
        "coupling": {"type": "PHASE_LOCK", "source": "ORCA", "target": "ODDFELIX", "amount": 0.7},
        "tags": ["entangled", "cold", "fast", "hunting", "arctic"],
        "description": "An eccentric Arctic hunt — predator speed through feliX-cold waters.",
    },
    # OCELOT + ORIGAMI (3 presets)
    {
        "name": "Paper Leopard Sprint",
        "engines": ["OCELOT", "ORIGAMI"],
        "macros": {"CHARACTER": 0.75, "MOVEMENT": 0.93, "COUPLING": 0.7, "SPACE": 0.55},
        "sonic_dna": {"brightness": 0.58, "warmth": 0.08, "movement": 0.93, "density": 0.4, "space": 0.52, "aggression": 0.65},
        "coupling": {"type": "HARMONIC_BLEND", "source": "OCELOT", "target": "ORIGAMI", "amount": 0.65},
        "tags": ["entangled", "cold", "fast", "feline", "origami"],
        "description": "Ocelot's cold sprint unfolding Origami's paper geometry at full velocity.",
    },
    {
        "name": "Crease and Claw",
        "engines": ["OCELOT", "ORIGAMI"],
        "macros": {"CHARACTER": 0.8, "MOVEMENT": 0.97, "COUPLING": 0.65, "SPACE": 0.48},
        "sonic_dna": {"brightness": 0.55, "warmth": 0.07, "movement": 0.97, "density": 0.5, "space": 0.45, "aggression": 0.72},
        "coupling": {"type": "TIMBRAL_WARP", "source": "ORIGAMI", "target": "OCELOT", "amount": 0.6},
        "tags": ["entangled", "cold", "fast", "aggressive", "tension"],
        "description": "Claws through cold paper at maximum speed — tension and release at velocity.",
    },
    {
        "name": "Spotted Fold Rush",
        "engines": ["OCELOT", "ORIGAMI"],
        "macros": {"CHARACTER": 0.7, "MOVEMENT": 0.91, "COUPLING": 0.72, "SPACE": 0.58},
        "sonic_dna": {"brightness": 0.6, "warmth": 0.09, "movement": 0.91, "density": 0.42, "space": 0.55, "aggression": 0.58},
        "coupling": {"type": "PHASE_LOCK", "source": "OCELOT", "target": "ORIGAMI", "amount": 0.68},
        "tags": ["entangled", "cold", "fast", "spotted", "kinetic"],
        "description": "Spotted pattern rushing through cold Origami folds — spotted light fragmenting in motion.",
    },
]

# ---------------------------------------------------------------------------
# Strategy 4 — Still Aggressive
# aggression 0.9+, movement 0.05  (violence held perfectly still)
# ---------------------------------------------------------------------------
STILL_AGGRESSIVE_PAIRS = [
    ("OBESE", "OBSIDIAN"),
    ("OVERBITE", "ORACLE"),
    ("ORBITAL", "ORCA"),
    ("ORGANON", "OUROBOROS"),
    ("ONSET", "OBLONG"),
]

STILL_AGGRESSIVE_PRESETS = [
    # OBESE + OBSIDIAN (3 presets)
    {
        "name": "Mass Monolith",
        "engines": ["OBESE", "OBSIDIAN"],
        "macros": {"CHARACTER": 0.9, "MOVEMENT": 0.05, "COUPLING": 0.85, "SPACE": 0.4},
        "sonic_dna": {"brightness": 0.2, "warmth": 0.4, "movement": 0.05, "density": 0.9, "space": 0.35, "aggression": 0.95},
        "coupling": {"type": "SPECTRAL_FOLD", "source": "OBESE", "target": "OBSIDIAN", "amount": 0.82},
        "tags": ["entangled", "aggressive", "still", "massive", "monolithic"],
        "description": "XObese's low-end mass fused with Obsidian's volcanic density — immovable and crushing.",
    },
    {
        "name": "Dead Weight Obsidian",
        "engines": ["OBESE", "OBSIDIAN"],
        "macros": {"CHARACTER": 0.95, "MOVEMENT": 0.04, "COUPLING": 0.88, "SPACE": 0.35},
        "sonic_dna": {"brightness": 0.15, "warmth": 0.45, "movement": 0.04, "density": 0.95, "space": 0.3, "aggression": 0.97},
        "coupling": {"type": "PHASE_LOCK", "source": "OBSIDIAN", "target": "OBESE", "amount": 0.85},
        "tags": ["entangled", "aggressive", "still", "heavy", "obsidian"],
        "description": "Obsidian's hardness bearing down on XObese's mass — completely static, devastatingly heavy.",
    },
    {
        "name": "Low Fault Pressure",
        "engines": ["OBESE", "OBSIDIAN"],
        "macros": {"CHARACTER": 0.85, "MOVEMENT": 0.06, "COUPLING": 0.8, "SPACE": 0.45},
        "sonic_dna": {"brightness": 0.18, "warmth": 0.5, "movement": 0.06, "density": 0.88, "space": 0.42, "aggression": 0.93},
        "coupling": {"type": "HARMONIC_BLEND", "source": "OBESE", "target": "OBSIDIAN", "amount": 0.78},
        "tags": ["entangled", "aggressive", "still", "tectonic", "pressure"],
        "description": "Geological pressure between tectonic bass and obsidian crust — barely moving, maximum force.",
    },
    # OVERBITE + ORACLE (3 presets)
    {
        "name": "Fang Prophecy",
        "engines": ["OVERBITE", "ORACLE"],
        "macros": {"CHARACTER": 0.9, "MOVEMENT": 0.05, "COUPLING": 0.82, "SPACE": 0.5},
        "sonic_dna": {"brightness": 0.35, "warmth": 0.55, "movement": 0.05, "density": 0.75, "space": 0.48, "aggression": 0.96},
        "coupling": {"type": "TIMBRAL_WARP", "source": "OVERBITE", "target": "ORACLE", "amount": 0.75},
        "tags": ["entangled", "aggressive", "still", "bass", "oracle"],
        "description": "Overbite's fang-sharp attack biting into Oracle's still prophecy — venom suspended in time.",
    },
    {
        "name": "Still Oracle Venom",
        "engines": ["OVERBITE", "ORACLE"],
        "macros": {"CHARACTER": 0.85, "MOVEMENT": 0.04, "COUPLING": 0.88, "SPACE": 0.55},
        "sonic_dna": {"brightness": 0.3, "warmth": 0.48, "movement": 0.04, "density": 0.8, "space": 0.52, "aggression": 0.98},
        "coupling": {"type": "SPECTRAL_FOLD", "source": "ORACLE", "target": "OVERBITE", "amount": 0.8},
        "tags": ["entangled", "aggressive", "still", "prophecy", "venom"],
        "description": "Oracle's vision delivered through Overbite's fangs — the prophecy is violence.",
    },
    {
        "name": "Bass Oracle Bite",
        "engines": ["OVERBITE", "ORACLE"],
        "macros": {"CHARACTER": 0.88, "MOVEMENT": 0.06, "COUPLING": 0.78, "SPACE": 0.45},
        "sonic_dna": {"brightness": 0.4, "warmth": 0.5, "movement": 0.06, "density": 0.72, "space": 0.43, "aggression": 0.93},
        "coupling": {"type": "PHASE_LOCK", "source": "OVERBITE", "target": "ORACLE", "amount": 0.72},
        "tags": ["entangled", "aggressive", "still", "bass", "locked"],
        "description": "BITE code locked to Oracle's still vision — aggressive ground state.",
    },
    # ORBITAL + ORCA (3 presets)
    {
        "name": "Apex Orbit",
        "engines": ["ORBITAL", "ORCA"],
        "macros": {"CHARACTER": 0.8, "MOVEMENT": 0.05, "COUPLING": 0.85, "SPACE": 0.55},
        "sonic_dna": {"brightness": 0.45, "warmth": 0.35, "movement": 0.05, "density": 0.7, "space": 0.52, "aggression": 0.94},
        "coupling": {"type": "HARMONIC_BLEND", "source": "ORBITAL", "target": "ORCA", "amount": 0.78},
        "tags": ["entangled", "aggressive", "still", "orbital", "apex"],
        "description": "Orbital's locked rotation containing Orca's apex predator force — still, massive, circling.",
    },
    {
        "name": "Geostationary Predator",
        "engines": ["ORBITAL", "ORCA"],
        "macros": {"CHARACTER": 0.85, "MOVEMENT": 0.04, "COUPLING": 0.9, "SPACE": 0.48},
        "sonic_dna": {"brightness": 0.4, "warmth": 0.3, "movement": 0.04, "density": 0.78, "space": 0.45, "aggression": 0.97},
        "coupling": {"type": "SPECTRAL_FOLD", "source": "ORCA", "target": "ORBITAL", "amount": 0.85},
        "tags": ["entangled", "aggressive", "still", "orbital", "predator"],
        "description": "A predator in geostationary orbit — watching, still, ferociously present.",
    },
    {
        "name": "Cold Ellipse",
        "engines": ["ORBITAL", "ORCA"],
        "macros": {"CHARACTER": 0.75, "MOVEMENT": 0.06, "COUPLING": 0.82, "SPACE": 0.6},
        "sonic_dna": {"brightness": 0.38, "warmth": 0.28, "movement": 0.06, "density": 0.65, "space": 0.58, "aggression": 0.92},
        "coupling": {"type": "PHASE_LOCK", "source": "ORBITAL", "target": "ORCA", "amount": 0.75},
        "tags": ["entangled", "aggressive", "still", "cold", "elliptical"],
        "description": "Orca's aggression tracing a cold orbital ellipse — force in geometric stasis.",
    },
    # ORGANON + OUROBOROS (3 presets)
    {
        "name": "Logic Serpent Lock",
        "engines": ["ORGANON", "OUROBOROS"],
        "macros": {"CHARACTER": 0.75, "MOVEMENT": 0.05, "COUPLING": 0.88, "SPACE": 0.5},
        "sonic_dna": {"brightness": 0.35, "warmth": 0.42, "movement": 0.05, "density": 0.8, "space": 0.48, "aggression": 0.95},
        "coupling": {"type": "TIMBRAL_WARP", "source": "ORGANON", "target": "OUROBOROS", "amount": 0.82},
        "tags": ["entangled", "aggressive", "still", "logic", "serpent"],
        "description": "Organon's logical system imprisoning Ouroboros in a perfectly still cycle of force.",
    },
    {
        "name": "Infinite Still Tension",
        "engines": ["ORGANON", "OUROBOROS"],
        "macros": {"CHARACTER": 0.8, "MOVEMENT": 0.04, "COUPLING": 0.85, "SPACE": 0.45},
        "sonic_dna": {"brightness": 0.32, "warmth": 0.45, "movement": 0.04, "density": 0.85, "space": 0.42, "aggression": 0.98},
        "coupling": {"type": "SPECTRAL_FOLD", "source": "OUROBOROS", "target": "ORGANON", "amount": 0.78},
        "tags": ["entangled", "aggressive", "still", "infinite", "tension"],
        "description": "The snake eating itself according to formal logical rules — infinite aggression held motionless.",
    },
    {
        "name": "Serpent Syllogism",
        "engines": ["ORGANON", "OUROBOROS"],
        "macros": {"CHARACTER": 0.7, "MOVEMENT": 0.06, "COUPLING": 0.82, "SPACE": 0.52},
        "sonic_dna": {"brightness": 0.38, "warmth": 0.4, "movement": 0.06, "density": 0.75, "space": 0.5, "aggression": 0.93},
        "coupling": {"type": "HARMONIC_BLEND", "source": "ORGANON", "target": "OUROBOROS", "amount": 0.75},
        "tags": ["entangled", "aggressive", "still", "syllogism", "cyclic"],
        "description": "A logical proof whose conclusion is violence, held in perfect grammatical stillness.",
    },
    # ONSET + OBLONG (3 presets)
    {
        "name": "Drum Column Freeze",
        "engines": ["ONSET", "OBLONG"],
        "macros": {"CHARACTER": 0.85, "MOVEMENT": 0.05, "COUPLING": 0.82, "SPACE": 0.4},
        "sonic_dna": {"brightness": 0.3, "warmth": 0.45, "movement": 0.05, "density": 0.82, "space": 0.38, "aggression": 0.95},
        "coupling": {"type": "PHASE_LOCK", "source": "ONSET", "target": "OBLONG", "amount": 0.78},
        "tags": ["entangled", "aggressive", "still", "percussive", "monolith"],
        "description": "Onset's percussion locked into Oblong's elongated mass — a frozen drum monolith.",
    },
    {
        "name": "Oblong Assault Stasis",
        "engines": ["ONSET", "OBLONG"],
        "macros": {"CHARACTER": 0.9, "MOVEMENT": 0.04, "COUPLING": 0.88, "SPACE": 0.38},
        "sonic_dna": {"brightness": 0.25, "warmth": 0.42, "movement": 0.04, "density": 0.88, "space": 0.35, "aggression": 0.97},
        "coupling": {"type": "SPECTRAL_FOLD", "source": "OBLONG", "target": "ONSET", "amount": 0.85},
        "tags": ["entangled", "aggressive", "still", "heavy", "dense"],
        "description": "Full assault locked in stasis — every drum hit absorbed into Oblong's stationary weight.",
    },
    {
        "name": "Impact Suspended",
        "engines": ["ONSET", "OBLONG"],
        "macros": {"CHARACTER": 0.8, "MOVEMENT": 0.06, "COUPLING": 0.8, "SPACE": 0.45},
        "sonic_dna": {"brightness": 0.28, "warmth": 0.48, "movement": 0.06, "density": 0.8, "space": 0.42, "aggression": 0.93},
        "coupling": {"type": "TIMBRAL_WARP", "source": "ONSET", "target": "OBLONG", "amount": 0.72},
        "tags": ["entangled", "aggressive", "still", "impact", "suspension"],
        "description": "Maximum impact suspended in time — aggression held at the moment of strike.",
    },
]

# ---------------------------------------------------------------------------
# Assemble all 4 strategies
# ---------------------------------------------------------------------------
STRATEGIES = [
    ("Bright Dark",      BRIGHT_DARK_PRESETS),
    ("Warm Sparse",      WARM_SPARSE_PRESETS),
    ("Fast Cold",        FAST_COLD_PRESETS),
    ("Still Aggressive", STILL_AGGRESSIVE_PRESETS),
]


def sanitize_filename(name: str) -> str:
    return "".join(c if c.isalnum() or c in " _-" else "_" for c in name).strip()


def write_preset(preset: dict, strategy_name: str) -> tuple[str, bool]:
    filename = sanitize_filename(preset["name"]) + ".xometa"
    out_path = os.path.join(OUTPUT_DIR, filename)

    if os.path.exists(out_path):
        return out_path, False  # skipped

    payload = {
        "name": preset["name"],
        "version": "1.0",
        "engines": preset["engines"],
        "mood": "Entangled",
        "strategy": strategy_name,
        "macros": preset["macros"],
        "sonic_dna": preset["sonic_dna"],
        "parameters": {},
        "coupling": preset["coupling"],
        "tags": preset["tags"],
        "description": preset["description"],
    }

    os.makedirs(OUTPUT_DIR, exist_ok=True)
    with open(out_path, "w", encoding="utf-8") as f:
        json.dump(payload, f, indent=2)
        f.write("\n")

    return out_path, True


def verify_extremes(preset: dict, strategy_name: str) -> list[str]:
    """Return list of violations if the strategy's extreme values aren't met."""
    dna = preset["sonic_dna"]
    violations = []

    if strategy_name == "Bright Dark":
        if dna["brightness"] < 0.9:
            violations.append(f"brightness={dna['brightness']} < 0.9")
        if dna["warmth"] > 0.15:
            violations.append(f"warmth={dna['warmth']} > 0.15")

    elif strategy_name == "Warm Sparse":
        if dna["warmth"] < 0.9:
            violations.append(f"warmth={dna['warmth']} < 0.9")
        if dna["density"] > 0.15:
            violations.append(f"density={dna['density']} > 0.15")

    elif strategy_name == "Fast Cold":
        if dna["movement"] < 0.9:
            violations.append(f"movement={dna['movement']} < 0.9")
        if dna["warmth"] > 0.15:
            violations.append(f"warmth={dna['warmth']} > 0.15")

    elif strategy_name == "Still Aggressive":
        if dna["aggression"] < 0.9:
            violations.append(f"aggression={dna['aggression']} < 0.9")
        if dna["movement"] > 0.1:
            violations.append(f"movement={dna['movement']} > 0.1")

    return violations


def main():
    total_written = 0
    total_skipped = 0
    total_violations = 0

    for strategy_name, presets in STRATEGIES:
        written = 0
        skipped = 0
        strategy_violations = []

        for preset in presets:
            violations = verify_extremes(preset, strategy_name)
            if violations:
                strategy_violations.append((preset["name"], violations))
                total_violations += len(violations)

            path, was_written = write_preset(preset, strategy_name)
            if was_written:
                written += 1
                total_written += 1
            else:
                skipped += 1
                total_skipped += 1

        print(f"[{strategy_name:20s}] written={written:2d}  skipped={skipped:2d}  violations={len(strategy_violations)}")
        for name, v in strategy_violations:
            print(f"  !! {name}: {', '.join(v)}")

    print()
    print(f"Total written : {total_written}")
    print(f"Total skipped : {total_skipped}")
    print(f"DNA violations: {total_violations}")
    print(f"Output dir    : {os.path.abspath(OUTPUT_DIR)}")

    if total_violations == 0:
        print("All extreme DNA values verified — clustering problem targeted.")
    else:
        print("WARNING: Some presets failed extreme value checks — review above.")


if __name__ == "__main__":
    main()
