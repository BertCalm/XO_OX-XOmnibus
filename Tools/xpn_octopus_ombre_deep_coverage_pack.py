#!/usr/bin/env python3
"""
xpn_octopus_ombre_deep_coverage_pack.py
Deep coverage expansion for OCTOPUS and OMBRE engines.

Generates:
  - 8 OCTOPUS×OMBRE marquee presets (shape-shifting memory theme)
  - 24 OCTOPUS presets paired with each of 24 partner engines
  - 24 OMBRE presets paired with same 24 partner engines

Total: ~56 presets written to Presets/XOmnibus/Entangled/
Skips files that already exist.
"""

import json
import os
import random
import math
from datetime import date

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
REPO_ROOT = os.path.dirname(SCRIPT_DIR)
OUTPUT_DIR = os.path.join(REPO_ROOT, "Presets", "XOmnibus", "Entangled")

TODAY = str(date.today())
VERSION = "1.0.0"
AUTHOR = "XO_OX Deep Coverage Pack 2026-03-16"

# ──────────────────────────────────────────────
# Engine DNA baselines (brightness, warmth, movement, density, space, aggression)
# ──────────────────────────────────────────────
ENGINE_DNA = {
    "OCTOPUS":    {"brightness": 0.7, "warmth": 0.4, "movement": 0.9, "density": 0.8, "space": 0.5, "aggression": 0.6},
    "OMBRE":      {"brightness": 0.4, "warmth": 0.6, "movement": 0.5, "density": 0.5, "space": 0.7, "aggression": 0.2},
    # Partners — approximate baselines
    "ODDFELIX":   {"brightness": 0.8, "warmth": 0.5, "movement": 0.7, "density": 0.6, "space": 0.5, "aggression": 0.5},
    "ODDOSCAR":   {"brightness": 0.6, "warmth": 0.8, "movement": 0.6, "density": 0.6, "space": 0.6, "aggression": 0.3},
    "OVERDUB":    {"brightness": 0.4, "warmth": 0.7, "movement": 0.5, "density": 0.5, "space": 0.7, "aggression": 0.3},
    "ODYSSEY":    {"brightness": 0.6, "warmth": 0.5, "movement": 0.7, "density": 0.6, "space": 0.6, "aggression": 0.4},
    "OBLONG":     {"brightness": 0.6, "warmth": 0.7, "movement": 0.5, "density": 0.7, "space": 0.4, "aggression": 0.4},
    "OBESE":      {"brightness": 0.7, "warmth": 0.6, "movement": 0.6, "density": 0.9, "space": 0.3, "aggression": 0.8},
    "ONSET":      {"brightness": 0.5, "warmth": 0.4, "movement": 0.8, "density": 0.7, "space": 0.4, "aggression": 0.7},
    "OVERWORLD":  {"brightness": 0.7, "warmth": 0.5, "movement": 0.6, "density": 0.6, "space": 0.5, "aggression": 0.5},
    "OPAL":       {"brightness": 0.6, "warmth": 0.5, "movement": 0.7, "density": 0.5, "space": 0.8, "aggression": 0.2},
    "ORBITAL":    {"brightness": 0.7, "warmth": 0.6, "movement": 0.5, "density": 0.6, "space": 0.6, "aggression": 0.4},
    "ORGANON":    {"brightness": 0.5, "warmth": 0.6, "movement": 0.6, "density": 0.7, "space": 0.6, "aggression": 0.3},
    "OUROBOROS":  {"brightness": 0.6, "warmth": 0.4, "movement": 0.9, "density": 0.7, "space": 0.4, "aggression": 0.7},
    "OBSIDIAN":   {"brightness": 0.9, "warmth": 0.4, "movement": 0.4, "density": 0.5, "space": 0.7, "aggression": 0.3},
    "OVERBITE":   {"brightness": 0.5, "warmth": 0.5, "movement": 0.6, "density": 0.7, "space": 0.4, "aggression": 0.7},
    "ORIGAMI":    {"brightness": 0.7, "warmth": 0.5, "movement": 0.7, "density": 0.6, "space": 0.5, "aggression": 0.5},
    "ORACLE":     {"brightness": 0.5, "warmth": 0.5, "movement": 0.6, "density": 0.6, "space": 0.7, "aggression": 0.3},
    "OBSCURA":    {"brightness": 0.4, "warmth": 0.5, "movement": 0.5, "density": 0.6, "space": 0.6, "aggression": 0.3},
    "OCEANIC":    {"brightness": 0.5, "warmth": 0.6, "movement": 0.6, "density": 0.6, "space": 0.8, "aggression": 0.3},
    "OCELOT":     {"brightness": 0.7, "warmth": 0.7, "movement": 0.7, "density": 0.6, "space": 0.4, "aggression": 0.6},
    "OPTIC":      {"brightness": 0.9, "warmth": 0.3, "movement": 0.8, "density": 0.4, "space": 0.5, "aggression": 0.4},
    "OBLIQUE":    {"brightness": 0.7, "warmth": 0.4, "movement": 0.8, "density": 0.6, "space": 0.5, "aggression": 0.5},
    "OSPREY":     {"brightness": 0.6, "warmth": 0.6, "movement": 0.6, "density": 0.5, "space": 0.7, "aggression": 0.4},
    "OSTERIA":    {"brightness": 0.5, "warmth": 0.8, "movement": 0.4, "density": 0.7, "space": 0.5, "aggression": 0.3},
    "OWLFISH":    {"brightness": 0.5, "warmth": 0.6, "movement": 0.5, "density": 0.6, "space": 0.6, "aggression": 0.4},
}

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

PARTNERS = [
    "ODDFELIX", "ODDOSCAR", "OVERDUB", "ODYSSEY", "OBLONG", "OBESE",
    "ONSET", "OVERWORLD", "OPAL", "ORBITAL", "ORGANON", "OUROBOROS",
    "OBSIDIAN", "OVERBITE", "ORIGAMI", "ORACLE", "OBSCURA", "OCEANIC",
    "OCELOT", "OPTIC", "OBLIQUE", "OSPREY", "OSTERIA", "OWLFISH",
]

# Engine prefix lookup
ENGINE_PREFIX = {
    "OCTOPUS":   "octo_",
    "OMBRE":     "ombre_",
    "ODDFELIX":  "snap_",
    "ODDOSCAR":  "morph_",
    "OVERDUB":   "dub_",
    "ODYSSEY":   "drift_",
    "OBLONG":    "bob_",
    "OBESE":     "fat_",
    "ONSET":     "perc_",
    "OVERWORLD": "ow_",
    "OPAL":      "opal_",
    "ORBITAL":   "orb_",
    "ORGANON":   "organon_",
    "OUROBOROS": "ouro_",
    "OBSIDIAN":  "obsidian_",
    "OVERBITE":  "poss_",
    "ORIGAMI":   "origami_",
    "ORACLE":    "oracle_",
    "OBSCURA":   "obscura_",
    "OCEANIC":   "ocean_",
    "OCELOT":    "ocelot_",
    "OPTIC":     "optic_",
    "OBLIQUE":   "oblq_",
    "OSPREY":    "osprey_",
    "OSTERIA":   "osteria_",
    "OWLFISH":   "owl_",
}

# Engine name for "engines" array field (canonical display name)
ENGINE_DISPLAY = {
    "OCTOPUS":   "Octopus",
    "OMBRE":     "Ombre",
    "ODDFELIX":  "OddfeliX",
    "ODDOSCAR":  "OddOscar",
    "OVERDUB":   "Overdub",
    "ODYSSEY":   "Odyssey",
    "OBLONG":    "Oblong",
    "OBESE":     "Obese",
    "ONSET":     "Onset",
    "OVERWORLD": "Overworld",
    "OPAL":      "Opal",
    "ORBITAL":   "Orbital",
    "ORGANON":   "Organon",
    "OUROBOROS": "Ouroboros",
    "OBSIDIAN":  "Obsidian",
    "OVERBITE":  "Overbite",
    "ORIGAMI":   "Origami",
    "ORACLE":    "Oracle",
    "OBSCURA":   "Obscura",
    "OCEANIC":   "Oceanic",
    "OCELOT":    "Ocelot",
    "OPTIC":     "Optic",
    "OBLIQUE":   "Oblique",
    "OSPREY":    "Osprey",
    "OSTERIA":   "Osteria",
    "OWLFISH":   "Owlfish",
}

# ──────────────────────────────────────────────
# Preset name tables
# ──────────────────────────────────────────────

# 8 OCTOPUS×OMBRE marquee preset names (shape-shifting memory theme)
MARQUEE_NAMES = [
    ("Ink Fading Into Dusk", "An octopus releases a memory-cloud; it bleeds purple into the failing light — eight arms forgetting in unison."),
    ("Chromatophore Reverie", "Skin-color dreams played back in slow motion; the octopus cannot tell if it is remembering or becoming."),
    ("Liminal Camouflage", "The boundary between hiding and being seen dissolves; shadow and signal trade places every bar."),
    ("Forgetting Arm By Arm", "Each arm carries a separate memory; one by one they go dark, leaving only the mauve residue of having been."),
    ("Twilight Intelligence", "Decentralized mind meets dusk-mind — eight semi-autonomous selves drifting toward the same horizon."),
    ("Shape-Shift At The Edge", "Transformation held at the cusp: the octopus is neither old shape nor new, the ombre neither day nor night."),
    ("Memory Chromatophore", "Pigment cells that fire in the pattern of a forgotten song; the body remembers what the mind let go."),
    ("Cephalopod Penumbra", "Where bright alien intelligence meets the soft half-shadow — the overlap is a new colour with no name."),
]

# OCTOPUS solo preset names per partner
OCTOPUS_PARTNER_NAMES = {
    "ODDFELIX":  ("Neon Arms Abroad",     "Eight arms hunting neon tetras through spectral light — a cascade of alien reflexes and schooling panic."),
    "ODDOSCAR":  ("Ink Meets Gill",       "Octopus ink diffuses through axolotl gill tissue; chemical speech between regeneration and camouflage."),
    "OVERDUB":   ("Tape Tentacle",        "An arm wraps itself around a tape loop and slowly squeezes the wow into the flutter — deliberate distortion."),
    "ODYSSEY":   ("Drifting Arm Cluster", "A loose colony of arms breaks free and orbits a drift oscillator — each arm a different harmonic partial."),
    "OBLONG":    ("Amber Sucker Array",   "Each sucker resonates at an amber harmonic; tapping Bob's warmth into eight separate adhesion events."),
    "OBESE":     ("Saturation Ink Cloud", "An ink cloud so dense it clips the water column — fat saturation feeding back through chromatophore logic."),
    "ONSET":     ("Percussive Discharge", "Every arm-strike triggers a percussion cell; the octopus becomes a drum machine with distributed nervous system."),
    "OVERWORLD": ("Chip Arm Signature",   "Eight arms spell out a NES arpeggiation pattern — alien intelligence transcribing chip-music in real time."),
    "OPAL":      ("Grain Chromatophore",  "Granular clouds seeded by chromatophore pulses; the octopus breathes and the opal scatters light."),
    "ORBITAL":   ("Radial Arm Orbit",     "Arms locked into orbital period ratios — a living solar system modulating its own warmth."),
    "ORGANON":   ("Metabolic Camouflage", "Skin pattern driven by metabolic rate — the faster the organon breathes, the more abstract the disguise."),
    "OUROBOROS": ("Arm Eating Arm",       "One arm follows another in an ouroboros loop; the attractor makes the predation rhythm inevitable."),
    "OBSIDIAN":  ("Crystal Ink Burst",    "Obsidian clarity shattered by a single ink cloud — eight fragments of crystal dispersing into acoustic black."),
    "OVERBITE":  ("Bite Reflex Arc",      "Arm touches prey → beak reflex fires → bite macro triggers → ink fires as adrenalin."),
    "ORIGAMI":   ("Folded Arm Lattice",   "Each arm folds along an origami crease, locking into harmonic geometry — alien origami in motion."),
    "ORACLE":    ("Stochastic Chromatics","GENDY breakpoints mapped to chromatophore state — the oracle's prophecy written in skin colour."),
    "OBSCURA":   ("Silver Ink Daguerreotype","A slow ink cloud exposed like a daguerreotype plate — the image develops over eight bars."),
    "OCEANIC":   ("Phosphor Eight",       "Bioluminescent teal meets chromatophore magenta; eight arms painting the water column in paired light."),
    "OCELOT":    ("Spot vs Sucker",       "Ocelot rosette pattern cross-coupled to sucker adhesion array — two camouflage grammars arguing."),
    "OPTIC":     ("Visual Nervous System","Optic pulse triggers arm motor commands; the visual engine becomes the octopus's distributed eye."),
    "OBLIQUE":   ("Prism Mantle",         "Oblique prismatic refraction mapped to mantle colour shifts — alien iridescence in eight directions."),
    "OSPREY":    ("Shore Grasp",          "An arm reaches from the shallows and seizes a shore-system harmonic — coast meeting cephalopod."),
    "OSTERIA":   ("Deep Tavern Arms",     "Eight arms wrapped around a wine-dark bass line — the octopus drinks the osteria's resonance."),
    "OWLFISH":   ("Abyssal Eight",        "Mixtur-Trautonium subharmonics coupled to arm-strike timing — deep-sea hunting by harmonic series."),
}

# OMBRE solo preset names per partner
OMBRE_PARTNER_NAMES = {
    "ODDFELIX":  ("Neon Fading",          "Neon pigment draining slowly from a schooling memory — the fish forget their colours at dusk."),
    "ODDOSCAR":  ("Gill Shadow",          "Axolotl gill tissue as twilight membrane — breath cycles slow toward the shadow boundary."),
    "OVERDUB":   ("Dub At Twilight",      "Tape echo stretched across the dusk border — the delay throws memory forward while forgetting pulls back."),
    "ODYSSEY":   ("Drift Into Evening",   "Wavetable position as a sundial hand — the oscillator drifts from afternoon gold to evening violet."),
    "OBLONG":    ("Amber Dusk Swell",     "Bob's amber warmth pooling at the low end of the shadow gradient — heat settling into night."),
    "OBESE":     ("Saturated Twilight",   "Saturation as the visible evidence of day → night compression — too much light caught in too small a moment."),
    "ONSET":     ("Decay Silhouette",     "Percussion decay curves traced against the ombre gradient — each hit leaves a darkening tail."),
    "OVERWORLD": ("8-Bit Dusk",           "Chip-music quantized palette transitioning from NES-bright to Genesis-shadowed — a console sunset."),
    "OPAL":      ("Grain Memory Fade",    "Granular position scrubbing backward through a memory — each grain a slightly older moment."),
    "ORBITAL":   ("Red Shift Ombre",      "Orbital warmth red-shifting as it crosses the shadow boundary — the planet cools from the outside in."),
    "ORGANON":   ("Metabolic Dusk",       "Organon metabolic clock as the sun — rate slows, shadow deepens, the organ breathes its last daylight."),
    "OUROBOROS": ("Loop Into Dark",       "The ouroboros biting its own tail marks the day-night junction — chaos at the tipping point."),
    "OBSIDIAN":  ("Light Into Crystal",   "Obsidian's crystalline brightness absorbed into the shadow — light becoming hardness becoming dark."),
    "OVERBITE":  ("Bite At Dusk",         "The bite macro triggers at the day/night boundary — instinct sharpening as vision fails."),
    "ORIGAMI":   ("Shadow Fold",          "Each fold crease catches light differently as the gradient turns — a paper lantern being slowly extinguished."),
    "ORACLE":    ("Prophecy At Dusk",     "Breakpoints predict the shadow's exact moment of arrival — the oracle sees the sun's angle."),
    "OBSCURA":   ("Double Exposure Dusk", "Two temporal states superimposed: the obscura's daguerreotype of now against the ombre's memory of before."),
    "OCEANIC":   ("Tidal Shadow",         "Sea level marks the ombre gradient's midpoint — phosphorescent teal beneath, mauve above."),
    "OCELOT":    ("Spotted Dusk",         "Rosette patterns dissolve into shadow at the golden hour — predator camouflage shifting from biome to darkness."),
    "OPTIC":     ("Pulse Into Night",     "Optic's phosphor-green pulses fade through the shadow gradient — light becoming less of itself."),
    "OBLIQUE":   ("Prismatic Dusk",       "Oblique refractions at low solar angle — a prism spectrum collapsing to a single shadow band."),
    "OSPREY":    ("Shore At Dusk",        "Coastal light gradients matching the ombre's memory model — the shoreline remembers its own tides."),
    "OSTERIA":   ("Wine-Dark Evening",    "Porto wine deep tones meeting shadow mauve — two darkness grammars finding common ground."),
    "OWLFISH":   ("Abyssal Dusk",         "Abyssal gold bleaching toward shadow as depth pressure increases — the ombre of water itself."),
}

# ──────────────────────────────────────────────
# Helper functions
# ──────────────────────────────────────────────

def jitter(val, amount=0.1, rng=None):
    """Add random jitter ±amount, clamped to [0, 1]."""
    if rng is None:
        rng = random
    return round(max(0.0, min(1.0, val + rng.uniform(-amount, amount))), 3)


def blend_dna(engine_a, engine_b, rng):
    """50/50 blend of two engine DNA dicts, then jitter each field."""
    keys = ["brightness", "warmth", "movement", "density", "space", "aggression"]
    result = {}
    a = ENGINE_DNA[engine_a]
    b = ENGINE_DNA[engine_b]
    for k in keys:
        mid = (a[k] + b[k]) / 2.0
        result[k] = jitter(mid, 0.1, rng)
    return result


def make_macro_values(engine_a, engine_b, rng):
    """Generate macro values influenced by blended DNA."""
    dna = blend_dna(engine_a, engine_b, rng)
    return {
        "CHARACTER": round(jitter((dna["aggression"] + dna["warmth"]) / 2.0, 0.1, rng), 3),
        "MOVEMENT":  round(jitter(dna["movement"], 0.1, rng), 3),
        "COUPLING":  round(rng.uniform(0.55, 0.85), 3),
        "SPACE":     round(jitter(dna["space"], 0.1, rng), 3),
    }


def make_octopus_params(rng):
    """Generate plausible OCTOPUS parameter values."""
    return {
        "octo_armCount":    round(rng.choice([4.0, 6.0, 8.0]), 1),
        "octo_armSpread":   round(rng.uniform(0.4, 0.9), 3),
        "octo_armBaseRate": round(rng.uniform(0.3, 0.9), 3),
        "octo_armDepth":    round(rng.uniform(0.5, 0.9), 3),
        "octo_chromaSens":  round(rng.uniform(0.4, 0.9), 3),
        "octo_chromaSpeed": round(rng.uniform(0.2, 0.8), 3),
        "octo_chromaMorph": round(rng.uniform(0.3, 0.8), 3),
        "octo_chromaDepth": round(rng.uniform(0.4, 0.8), 3),
        "octo_chromaFreq":  round(rng.uniform(800.0, 3200.0), 1),
        "octo_inkThreshold":round(rng.uniform(0.3, 0.7), 3),
        "octo_inkDensity":  round(rng.uniform(0.4, 0.9), 3),
        "octo_inkDecay":    round(rng.uniform(0.3, 0.8), 3),
        "octo_inkMix":      round(rng.uniform(0.2, 0.6), 3),
        "octo_shiftMicro":  round(rng.uniform(0.3, 0.8), 3),
        "octo_shiftGlide":  round(rng.uniform(0.0, 0.4), 3),
        "octo_shiftDrift":  round(rng.uniform(0.3, 0.7), 3),
        "octo_suckerReso":  round(rng.uniform(0.3, 0.8), 3),
        "octo_suckerFreq":  round(rng.uniform(1200.0, 5000.0), 1),
        "octo_suckerDecay": round(rng.uniform(0.05, 0.25), 3),
        "octo_suckerMix":   round(rng.uniform(0.2, 0.7), 3),
        "octo_wtPosition":  round(rng.uniform(0.1, 0.9), 3),
        "octo_wtScanRate":  round(rng.uniform(0.1, 0.8), 3),
        "octo_filterCutoff":round(rng.uniform(2000.0, 12000.0), 1),
        "octo_filterReso":  round(rng.uniform(0.2, 0.7), 3),
        "octo_level":       round(rng.uniform(0.65, 0.85), 3),
        "octo_ampAttack":   round(rng.uniform(0.005, 0.08), 4),
        "octo_ampDecay":    round(rng.uniform(0.2, 0.8), 3),
        "octo_ampSustain":  round(rng.uniform(0.4, 0.8), 3),
        "octo_ampRelease":  round(rng.uniform(0.4, 1.5), 3),
        "octo_modAttack":   round(rng.uniform(0.005, 0.05), 4),
        "octo_modDecay":    round(rng.uniform(0.1, 0.5), 3),
        "octo_modSustain":  round(rng.uniform(0.3, 0.7), 3),
        "octo_modRelease":  round(rng.uniform(0.3, 0.9), 3),
        "octo_lfo1Rate":    round(rng.uniform(0.3, 3.5), 3),
        "octo_lfo1Depth":   round(rng.uniform(0.2, 0.7), 3),
        "octo_lfo1Shape":   round(rng.choice([0.0, 0.25, 0.5, 0.75, 1.0]), 2),
        "octo_lfo2Rate":    round(rng.uniform(0.1, 2.0), 3),
        "octo_lfo2Depth":   round(rng.uniform(0.1, 0.6), 3),
        "octo_lfo2Shape":   round(rng.choice([0.0, 0.25, 0.5, 0.75, 1.0]), 2),
    }


def make_ombre_params(rng):
    """Generate plausible OMBRE parameter values."""
    return {
        "ombre_blend":          round(rng.uniform(0.2, 0.8), 3),
        "ombre_memoryDepth":    round(rng.uniform(0.4, 0.9), 3),
        "ombre_forgetRate":     round(rng.uniform(0.1, 0.7), 3),
        "ombre_shadowAmount":   round(rng.uniform(0.3, 0.8), 3),
        "ombre_duskPosition":   round(rng.uniform(0.2, 0.8), 3),
        "ombre_narrativeA":     round(rng.uniform(0.4, 0.9), 3),
        "ombre_narrativeB":     round(rng.uniform(0.1, 0.6), 3),
        "ombre_crossfadeRate":  round(rng.uniform(0.05, 0.5), 3),
        "ombre_toneA":          round(rng.uniform(2000.0, 8000.0), 1),
        "ombre_toneB":          round(rng.uniform(500.0, 3000.0), 1),
        "ombre_filterCutoff":   round(rng.uniform(1000.0, 9000.0), 1),
        "ombre_filterReso":     round(rng.uniform(0.1, 0.5), 3),
        "ombre_level":          round(rng.uniform(0.65, 0.82), 3),
        "ombre_ampAttack":      round(rng.uniform(0.02, 0.2), 4),
        "ombre_ampDecay":       round(rng.uniform(0.3, 1.0), 3),
        "ombre_ampSustain":     round(rng.uniform(0.4, 0.8), 3),
        "ombre_ampRelease":     round(rng.uniform(0.6, 2.0), 3),
        "ombre_modAttack":      round(rng.uniform(0.01, 0.1), 4),
        "ombre_modDecay":       round(rng.uniform(0.2, 0.8), 3),
        "ombre_modSustain":     round(rng.uniform(0.3, 0.7), 3),
        "ombre_modRelease":     round(rng.uniform(0.5, 1.5), 3),
        "ombre_lfo1Rate":       round(rng.uniform(0.05, 1.5), 3),
        "ombre_lfo1Depth":      round(rng.uniform(0.2, 0.6), 3),
        "ombre_lfo2Rate":       round(rng.uniform(0.02, 0.8), 3),
        "ombre_lfo2Depth":      round(rng.uniform(0.1, 0.5), 3),
        "ombre_spaceMix":       round(rng.uniform(0.4, 0.8), 3),
        "ombre_reverbSize":     round(rng.uniform(0.4, 0.9), 3),
        "ombre_reverbDamp":     round(rng.uniform(0.3, 0.7), 3),
    }


def make_generic_partner_params(engine_id, rng):
    """Return a minimal parameter block for a partner engine (2-3 key params)."""
    prefix = ENGINE_PREFIX.get(engine_id, engine_id.lower() + "_")
    # Just a handful of representative params — enough to anchor the coupling
    return {
        f"{prefix}level":       round(rng.uniform(0.65, 0.85), 3),
        f"{prefix}filterCutoff": round(rng.uniform(1500.0, 10000.0), 1),
    }


def pick_coupling(i, source, target):
    ctype = COUPLING_TYPES[i % len(COUPLING_TYPES)]
    amount = round(0.5 + (i % 7) * 0.05, 2)
    return {
        "type": ctype,
        "source": ENGINE_DISPLAY.get(source, source),
        "target": ENGINE_DISPLAY.get(target, target),
        "amount": amount,
    }


def build_preset(name, engines, description, tags, dna, macros, params, coupling):
    return {
        "schema_version": 1,
        "name": name,
        "mood": "Entangled",
        "engines": [ENGINE_DISPLAY.get(e, e) for e in engines],
        "author": AUTHOR,
        "version": VERSION,
        "description": description,
        "tags": tags,
        "macroLabels": ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"],
        "couplingIntensity": "Moderate",
        "tempo": None,
        "created": TODAY,
        "parameters": params,
        "coupling": {"pairs": [coupling]},
        "sequencer": None,
        "dna": dna,
    }


def safe_filename(name):
    """Convert preset name to safe filename."""
    safe = name.replace(" ", "_").replace("/", "-").replace(":", "").replace("'", "")
    return safe


def write_preset(preset, filename):
    path = os.path.join(OUTPUT_DIR, filename)
    if os.path.exists(path):
        return False  # skip
    with open(path, "w", encoding="utf-8") as f:
        json.dump(preset, f, indent=2, ensure_ascii=False)
        f.write("\n")
    return True


# ──────────────────────────────────────────────
# Generator functions
# ──────────────────────────────────────────────

def generate_marquee(rng):
    """Generate 8 OCTOPUS×OMBRE marquee presets."""
    count = 0
    for i, (name, description) in enumerate(MARQUEE_NAMES):
        rng.seed(1000 + i)
        dna = blend_dna("OCTOPUS", "OMBRE", rng)
        macros = make_macro_values("OCTOPUS", "OMBRE", rng)
        params = {
            "Octopus": make_octopus_params(rng),
            "Ombre":   make_ombre_params(rng),
        }
        coupling = pick_coupling(i, "OCTOPUS", "OMBRE")
        tags = ["entangled", "coupling", "octopus", "ombre", "memory", "shape-shifting", "marquee"]
        preset = build_preset(name, ["OCTOPUS", "OMBRE"], description, tags, dna, macros, params, coupling)
        fname = f"Octopus_Ombre_{safe_filename(name)}.xometa"
        written = write_preset(preset, fname)
        status = "wrote" if written else "skipped"
        print(f"  [{status}] {fname}")
        if written:
            count += 1
    return count


def generate_octopus_partner_presets(rng):
    """Generate 24 OCTOPUS presets with each partner."""
    count = 0
    for i, partner in enumerate(PARTNERS):
        rng.seed(2000 + i)
        name, description = OCTOPUS_PARTNER_NAMES[partner]
        dna = blend_dna("OCTOPUS", partner, rng)
        macros = make_macro_values("OCTOPUS", partner, rng)
        partner_params = make_generic_partner_params(partner, rng)
        params = {
            "Octopus": make_octopus_params(rng),
            ENGINE_DISPLAY.get(partner, partner): partner_params,
        }
        coupling = pick_coupling(i + 4, "OCTOPUS", partner)
        tags = ["entangled", "coupling", "octopus", partner.lower(), "alien", "decentralized"]
        preset = build_preset(name, ["OCTOPUS", partner], description, tags, dna, macros, params, coupling)
        fname = f"Octopus_{partner.capitalize()}_{safe_filename(name)}.xometa"
        written = write_preset(preset, fname)
        status = "wrote" if written else "skipped"
        print(f"  [{status}] {fname}")
        if written:
            count += 1
    return count


def generate_ombre_partner_presets(rng):
    """Generate 24 OMBRE presets with each partner."""
    count = 0
    for i, partner in enumerate(PARTNERS):
        rng.seed(3000 + i)
        name, description = OMBRE_PARTNER_NAMES[partner]
        dna = blend_dna("OMBRE", partner, rng)
        macros = make_macro_values("OMBRE", partner, rng)
        partner_params = make_generic_partner_params(partner, rng)
        params = {
            "Ombre": make_ombre_params(rng),
            ENGINE_DISPLAY.get(partner, partner): partner_params,
        }
        coupling = pick_coupling(i + 8, "OMBRE", partner)
        tags = ["entangled", "coupling", "ombre", partner.lower(), "memory", "twilight", "liminal"]
        preset = build_preset(name, ["OMBRE", partner], description, tags, dna, macros, params, coupling)
        fname = f"Ombre_{partner.capitalize()}_{safe_filename(name)}.xometa"
        written = write_preset(preset, fname)
        status = "wrote" if written else "skipped"
        print(f"  [{status}] {fname}")
        if written:
            count += 1
    return count


# ──────────────────────────────────────────────
# Main
# ──────────────────────────────────────────────

def main():
    os.makedirs(OUTPUT_DIR, exist_ok=True)

    rng = random.Random()

    print("\n=== OCTOPUS×OMBRE Marquee Presets (8) ===")
    m = generate_marquee(rng)

    print("\n=== OCTOPUS Partner Presets (24) ===")
    o = generate_octopus_partner_presets(rng)

    print("\n=== OMBRE Partner Presets (24) ===")
    b = generate_ombre_partner_presets(rng)

    total = m + o + b
    print(f"\n✓ Done — {total} new presets written to {OUTPUT_DIR}")
    print(f"  Marquee: {m}  |  OCTOPUS: {o}  |  OMBRE: {b}")


if __name__ == "__main__":
    main()
