#!/usr/bin/env python3
"""
xpn_odyssey_oblong_obese_coverage_pack.py
Generate ~90 Entangled mood .xometa preset stubs covering new coupling pairs
for ODYSSEY, OBLONG, and OBESE engines.

ODYSSEY × ORGANON/OUROBOROS/OBSIDIAN/ORIGAMI/ORACLE: 5 pairs × 6 = 30 presets
OBLONG  × ORGANON/OUROBOROS/ORIGAMI/ORACLE/OBSCURA:  5 pairs × 6 = 30 presets
OBESE   × ORGANON/OUROBOROS/ORIGAMI/ORACLE/OBSCURA:  5 pairs × 6 = 30 presets

Total: 90 presets written to Presets/XOmnibus/Entangled/
Skips files that already exist.
"""

import json
import os
import re
import random
from datetime import date

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
REPO_ROOT  = os.path.dirname(SCRIPT_DIR)
OUTPUT_DIR = os.path.join(REPO_ROOT, "Presets", "XOmnibus", "Entangled")

TODAY   = str(date.today())
VERSION = "1.0"
AUTHOR  = "XO_OX Coverage Pack 2026-03-16"

# ──────────────────────────────────────────────────────────────
# Engine DNA baselines
# ──────────────────────────────────────────────────────────────
ENGINE_DNA = {
    "ODYSSEY":   {"brightness": 0.55, "warmth": 0.5,  "movement": 0.65, "density": 0.55, "space": 0.6,  "aggression": 0.35},
    "OBLONG":    {"brightness": 0.65, "warmth": 0.65, "movement": 0.55, "density": 0.7,  "space": 0.4,  "aggression": 0.5},
    "OBESE":     {"brightness": 0.5,  "warmth": 0.7,  "movement": 0.6,  "density": 0.8,  "space": 0.4,  "aggression": 0.7},
    # Partners
    "OBSCURA":   {"brightness": 0.4,  "warmth": 0.5,  "movement": 0.5,  "density": 0.6,  "space": 0.6,  "aggression": 0.3},
    "ORIGAMI":   {"brightness": 0.7,  "warmth": 0.5,  "movement": 0.7,  "density": 0.6,  "space": 0.5,  "aggression": 0.5},
    "OCELOT":    {"brightness": 0.7,  "warmth": 0.7,  "movement": 0.7,  "density": 0.6,  "space": 0.4,  "aggression": 0.6},
    "OPTIC":     {"brightness": 0.9,  "warmth": 0.3,  "movement": 0.8,  "density": 0.4,  "space": 0.5,  "aggression": 0.4},
    "OBLIQUE":   {"brightness": 0.7,  "warmth": 0.4,  "movement": 0.8,  "density": 0.6,  "space": 0.5,  "aggression": 0.5},
    "OSPREY":    {"brightness": 0.6,  "warmth": 0.6,  "movement": 0.6,  "density": 0.5,  "space": 0.7,  "aggression": 0.4},
    "OSTERIA":   {"brightness": 0.5,  "warmth": 0.8,  "movement": 0.4,  "density": 0.7,  "space": 0.5,  "aggression": 0.3},
    "OCEANIC":   {"brightness": 0.5,  "warmth": 0.6,  "movement": 0.6,  "density": 0.6,  "space": 0.8,  "aggression": 0.3},
    "OWLFISH":   {"brightness": 0.5,  "warmth": 0.6,  "movement": 0.5,  "density": 0.6,  "space": 0.6,  "aggression": 0.4},
    "OHM":       {"brightness": 0.5,  "warmth": 0.7,  "movement": 0.45, "density": 0.55, "space": 0.65, "aggression": 0.2},
    "ORPHICA":   {"brightness": 0.65, "warmth": 0.55, "movement": 0.6,  "density": 0.5,  "space": 0.7,  "aggression": 0.25},
    "OBBLIGATO": {"brightness": 0.55, "warmth": 0.6,  "movement": 0.5,  "density": 0.65, "space": 0.55, "aggression": 0.3},
    "OTTONI":    {"brightness": 0.6,  "warmth": 0.55, "movement": 0.55, "density": 0.7,  "space": 0.5,  "aggression": 0.4},
    "OLE":       {"brightness": 0.7,  "warmth": 0.65, "movement": 0.75, "density": 0.6,  "space": 0.45, "aggression": 0.55},
    "OMBRE":     {"brightness": 0.4,  "warmth": 0.6,  "movement": 0.5,  "density": 0.5,  "space": 0.7,  "aggression": 0.2},
    "ORCA":      {"brightness": 0.45, "warmth": 0.4,  "movement": 0.7,  "density": 0.75, "space": 0.55, "aggression": 0.75},
    "OCTOPUS":   {"brightness": 0.7,  "warmth": 0.4,  "movement": 0.9,  "density": 0.8,  "space": 0.5,  "aggression": 0.6},
    "OVERLAP":   {"brightness": 0.55, "warmth": 0.5,  "movement": 0.65, "density": 0.7,  "space": 0.65, "aggression": 0.35},
    "OUTWIT":    {"brightness": 0.6,  "warmth": 0.45, "movement": 0.85, "density": 0.75, "space": 0.45, "aggression": 0.65},
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

ENGINE_PREFIX = {
    "ODYSSEY":   "drift_",
    "OBLONG":    "bob_",
    "OBESE":     "fat_",
    "OBSCURA":   "obscura_",
    "ORIGAMI":   "origami_",
    "OCELOT":    "ocelot_",
    "OPTIC":     "optic_",
    "OBLIQUE":   "oblq_",
    "OSPREY":    "osprey_",
    "OSTERIA":   "osteria_",
    "OCEANIC":   "ocean_",
    "OWLFISH":   "owl_",
    "OHM":       "ohm_",
    "ORPHICA":   "orph_",
    "OBBLIGATO": "obbl_",
    "OTTONI":    "otto_",
    "OLE":       "ole_",
    "OMBRE":     "ombre_",
    "ORCA":      "orca_",
    "OCTOPUS":   "octo_",
    "OVERLAP":   "olap_",
    "OUTWIT":    "owit_",
}

ENGINE_DISPLAY = {
    "ODYSSEY":   "Odyssey",
    "OBLONG":    "Oblong",
    "OBESE":     "Obese",
    "OBSCURA":   "Obscura",
    "ORIGAMI":   "Origami",
    "OCELOT":    "Ocelot",
    "OPTIC":     "Optic",
    "OBLIQUE":   "Oblique",
    "OSPREY":    "Osprey",
    "OSTERIA":   "Osteria",
    "OCEANIC":   "Oceanic",
    "OWLFISH":   "Owlfish",
    "OHM":       "Ohm",
    "ORPHICA":   "Orphica",
    "OBBLIGATO": "Obbligato",
    "OTTONI":    "Ottoni",
    "OLE":       "Ole",
    "OMBRE":     "Ombre",
    "ORCA":      "Orca",
    "OCTOPUS":   "Octopus",
    "OVERLAP":   "Overlap",
    "OUTWIT":    "Outwit",
}

# ──────────────────────────────────────────────────────────────
# Partner lists (new pairs only)
# ──────────────────────────────────────────────────────────────

ODYSSEY_PARTNERS = [
    "OBSCURA", "ORIGAMI", "OCELOT", "OPTIC", "OBLIQUE", "OSPREY",
    "OSTERIA", "OCEANIC", "OWLFISH", "OHM", "ORPHICA", "OBBLIGATO",
    "OTTONI", "OLE", "OMBRE", "ORCA", "OCTOPUS",
]  # 17 partners

OBLONG_PARTNERS = [
    "OPTIC", "OBLIQUE", "OSPREY", "OSTERIA", "OCEANIC", "OWLFISH",
    "OHM", "ORPHICA", "OTTONI", "OLE", "OMBRE", "ORCA", "OCTOPUS",
]  # 13 partners

OBESE_PARTNERS = [
    "OPTIC", "OBLIQUE", "OCELOT", "OSPREY", "OSTERIA", "OCEANIC",
    "OWLFISH", "OHM", "ORPHICA", "OBBLIGATO", "OTTONI", "OLE",
    "OMBRE", "ORCA", "OCTOPUS", "OVERLAP", "OUTWIT",
]  # 17 partners

# ──────────────────────────────────────────────────────────────
# Preset name tables  (2 variants per pair = A, B)
# Each entry: (name_A, desc_A, name_B, desc_B)
# ──────────────────────────────────────────────────────────────

ODYSSEY_NAMES = {
    # partner: (nameA, descA, nameB, descB)
    "OBSCURA":   (
        "Silver Drift",
        "Daguerreotype silver plates the wavetable's slow oscillation — a violet image developing in the chemical dark.",
        "Violet Exposure",
        "The drift oscillator locks exposure time; each bar a slightly longer chemical bath, each tone more permanent.",
    ),
    "ORIGAMI":   (
        "Folded Frequency",
        "Wavetable position as a crease angle — the fold topology reshapes every harmonic at once.",
        "Drift Origami",
        "A drifting FM ratio traces a kite-fold pattern; the paper moves but the crease stays fixed.",
    ),
    "OCELOT":    (
        "Spotted Drift",
        "Ocelot rosette patterns modulate the wavetable scan rate — predator camouflage at FM speed.",
        "Biome Shift",
        "The ocelot crosses from humid forest to dry plain; each biome a different wavetable position.",
    ),
    "OPTIC":     (
        "Phosphor Drift",
        "Phosphor-green pulse rate clocks the drift oscillator — visual timing becoming pitch timing.",
        "Visual FM",
        "AutoPulse geometry drives FM depth; the optic pattern IS the modulation index.",
    ),
    "OBLIQUE":   (
        "Prism Drift",
        "Violet meets prism violet — two purples at oblique angles, refracting into a third colour.",
        "Oblique Wave",
        "Wavetable morphs along the oblique bounce axis; each reflection a new spectral slice.",
    ),
    "OSPREY":    (
        "Coastal Drift",
        "A wavetable oscillator carried on shore-system wind — five coastlines, five timbral zones.",
        "Osprey Wavetable",
        "Shore blend position scans the wavetable; each tide a different harmonic landscape.",
    ),
    "OSTERIA":   (
        "Porto Violet",
        "Porto wine depth meeting wavetable violet — a slow FM swell warming from the bottom up.",
        "Drift Taverna",
        "The osteria's resonant Q tunes the FM carrier; every round ordered, the pitch drifts warmer.",
    ),
    "OCEANIC":   (
        "Deep Drift",
        "Phosphorescent teal traces the wavetable scan path — bioluminescence lighting each harmonic.",
        "Tidal Wavetable",
        "Oceanic separation vector becomes wavetable position — the tide IN the oscillator.",
    ),
    "OWLFISH":   (
        "Abyssal Drift",
        "Mixtur-Trautonium subharmonics lock the FM ratio — depth-pressure tuning the overtone series.",
        "Drift Subharmonic",
        "Wavetable position coupled to subharmonic register — the deeper the scan, the lower the floor.",
    ),
    "OHM":       (
        "Sage Drift",
        "Ohm's communal warmth slows the wavetable scan — a hippy-dad groove underneath violet FM.",
        "Meddling Wave",
        "MEDDLING macro phase-resets the drift oscillator — interference as intentional community.",
    ),
    "ORPHICA":   (
        "Harp Drift",
        "Microsound harp plucks the wavetable at siphonophore timing — each colony member a partial.",
        "Drift Siren",
        "Orphica's pluck brightness tracks wavetable brightness — the harp teaches the oscillator colour.",
    ),
    "OBBLIGATO": (
        "Wind Drift",
        "Dual wind voices set the FM ratio envelope — breath A rises, breath B sustains, drift glides.",
        "Obligatory Wave",
        "Wavetable scan locked to obbligato BOND macro tension — the harmonic cannot escape the wind.",
    ),
    "OTTONI":    (
        "Brass Drift",
        "Triple-brass GROW macro expands FM depth — the orchestra swells and the wavetable opens.",
        "Drift Patina",
        "Patina green tunes the FM carrier via Ottoni formant shift — aged metal, aged wave.",
    ),
    "OLE":       (
        "Drama Wave",
        "OLE DRAMA macro depth-modulates wavetable position — every accent a new timbral reveal.",
        "Afro Drift",
        "Afro-Latin rhythm patterns clock the FM LFO — polyrhythm driving spectral motion.",
    ),
    "OMBRE":     (
        "Twilight Wavetable",
        "Wavetable position as a sundial — the scan moves from dawn partials to dusk partials.",
        "Violet Dusk",
        "Two purples: drift violet inside, ombre shadow mauve outside — one fading into the other.",
    ),
    "ORCA":      (
        "Echolocation Drift",
        "Orca echolocation timing becomes FM clock rate — each return pulse a new ratio.",
        "Breach Wave",
        "Wavetable resets at breach events — apex-predator attack transients resetting the scan.",
    ),
    "OCTOPUS":   (
        "Arm Drift",
        "Eight arms each hold a different wavetable phase — decentralized oscillators drifting apart.",
        "Chromatophore Drift",
        "Chromatophore fire events trigger wavetable jumps — skin colour driving spectral colour.",
    ),
}

OBLONG_NAMES = {
    "OPTIC":     (
        "Amber Phosphor",
        "Phosphor-green pulse rate clocks Bob's envelope — visual timing becoming bass attack timing.",
        "Bob Optical",
        "AutoPulse geometry modulates Bob's filter cutoff — light patterns shaping the low end.",
    ),
    "OBLIQUE":   (
        "Amber Prism",
        "Oblique prismatic refraction bends Bob's harmonic content — warm amber through a prism.",
        "Bob Bounce",
        "Bass line follows the oblique bounce axis — each reflection a syncopated groove.",
    ),
    "OSPREY":    (
        "Shore Bass",
        "Shore-system blend feeds Bob's sub register — five coastlines, five bass timbres.",
        "Amber Coast",
        "Osprey's cultural data tunes Bob's formant — coastal character in the low end.",
    ),
    "OSTERIA":   (
        "Taverna Bass",
        "Porto wine resonance meets Bob's bass warmth — deep tavern frequency.",
        "Amber Porto",
        "Osteria Q-bass shores up Bob's low-mid — two depth systems, one groove.",
    ),
    "OCEANIC":   (
        "Tidal Bob",
        "Oceanic separation vector modulates Bob's filter sweep — tidal bass motion.",
        "Amber Deep",
        "Phosphorescent teal filters through amber warmth — bioluminescent low end.",
    ),
    "OWLFISH":   (
        "Subharmonic Bob",
        "Mixtur-Trautonium subharmonics lock Bob's oscillator register — abyssal bass depth.",
        "Amber Abyss",
        "Owlfish subharmonic series tunes Bob's chord stack — deep-sea harmony.",
    ),
    "OHM":       (
        "Commune Bass",
        "OHM community warmth amplifies Bob's amber glow — hippy-dad bass council.",
        "Sage Bob",
        "COMMUNE macro deepens Bob's sub layer — collective low-end intelligence.",
    ),
    "ORPHICA":   (
        "Harp Bob",
        "Microsound harp plucks Bob's upper partials — siphonophore colony meets character bass.",
        "Amber Siren",
        "Orphica pluck brightness shapes Bob's formant peak — harp and bass sharing harmonic space.",
    ),
    "OTTONI":    (
        "Brass Bob",
        "Ottoni GROW macro swells Bob's harmonic density — bass section expanding.",
        "Amber Patina",
        "Triple-brass formant shifts tune Bob's mid register — patina over amber.",
    ),
    "OLE":       (
        "Drama Bob",
        "OLE DRAMA macro strikes Bob's accent layer — Afro-Latin accent bass.",
        "Amber Ritual",
        "OLE rhythm patterns clock Bob's arpeggiator — polyrhythm in the low end.",
    ),
    "OMBRE":     (
        "Amber Dusk",
        "Shadow mauve gradient dims Bob's amber warmth — the bass cooling toward night.",
        "Bob Twilight",
        "Ombre narrative arc shapes Bob's filter envelope — memory and bass, fading together.",
    ),
    "ORCA":      (
        "Predator Bob",
        "Orca hunt macro triggers Bob's attack transient — apex-predator bass strike.",
        "Amber Breach",
        "Orca breach events reset Bob's LFO — each leap a new bass rhythm.",
    ),
    "OCTOPUS":   (
        "Arm Bass",
        "Eight arms each hold a phase of Bob's bass oscillator — decentralized low end.",
        "Amber Eight",
        "Chromatophore pulses trigger Bob's filter modulation — skin colour driving bass colour.",
    ),
}

OBESE_NAMES = {
    "OPTIC":     (
        "Phosphor Fat",
        "AutoPulse rate modulates MOJO drive — visual pulse controlling saturation intensity.",
        "Fat Optical",
        "Phosphor-green clock resets the analog/digital axis position — light setting the drive character.",
    ),
    "OBLIQUE":   (
        "Prism Saturation",
        "Oblique bounce geometry routes saturation through prismatic spectral shaping.",
        "Fat Prism",
        "Prismatic refraction colours the distortion overtone series — analog drive through a prism.",
    ),
    "OCELOT":    (
        "Spotted Mojo",
        "Ocelot biome ID selects saturation algorithm — each biome a different distortion character.",
        "Fat Predator",
        "MOJO drive threshold tracks ocelot velocity sensitivity — predator attack in the gain stage.",
    ),
    "OSPREY":    (
        "Shore Mojo",
        "Shore-system coastal data tunes MOJO character — five coastlines, five drive voices.",
        "Fat Coast",
        "Osprey cultural blend shapes the EQ curve of the saturation — geography as tone stack.",
    ),
    "OSTERIA":   (
        "Porto Mojo",
        "Porto wine resonance fattens the saturation low shelf — deep tavern drive.",
        "Fat Taverna",
        "Osteria Q-bass feeds forward into MOJO gain stage — resonant deep-fry.",
    ),
    "OCEANIC":   (
        "Tidal Mojo",
        "Oceanic separation vector modulates B015 axis position — tidal analog/digital sweep.",
        "Fat Phosphor",
        "Bioluminescent teal brightness shapes MOJO harmonic content — ocean light through hot pink.",
    ),
    "OWLFISH":   (
        "Abyssal Mojo",
        "Mixtur-Trautonium subharmonics set MOJO floor frequency — deep-sea saturation.",
        "Fat Abyss",
        "Subharmonic series drives the distortion octave multiply — abyssal fat.",
    ),
    "OHM":       (
        "Commune Mojo",
        "OHM MEDDLING macro interferes with MOJO threshold — collective gain politics.",
        "Fat Sage",
        "Ohm communal warmth softens MOJO aggression — sage counselling the hot pink.",
    ),
    "ORPHICA":   (
        "Harp Mojo",
        "Orphica pluck transient triggers MOJO envelope — microsound attack driving saturation.",
        "Fat Siren",
        "Siphonophore timing gates the MOJO release — colony rhythm in the gain stage.",
    ),
    "OBBLIGATO": (
        "Wind Mojo",
        "Obbligato BOND macro tension sets MOJO drive — the wind holds the saturation taut.",
        "Fat Obligato",
        "Dual wind formants shape MOJO EQ response — breath sculpting the distortion.",
    ),
    "OTTONI":    (
        "Brass Mojo",
        "Ottoni GROW macro expands MOJO harmonic density — saturation filling the brass section.",
        "Fat Patina",
        "Triple-brass attack locks MOJO transient shaper timing — patina distortion.",
    ),
    "OLE":       (
        "Drama Mojo",
        "OLE DRAMA macro fires MOJO accent layer — Afro-Latin saturation strike.",
        "Fat Ritual",
        "OLE polyrhythm clocks MOJO gate pattern — rhythmic drive from ceremony.",
    ),
    "OMBRE":     (
        "Twilight Mojo",
        "Ombre shadow gradient dims MOJO saturation amount — drive fading toward night.",
        "Fat Dusk",
        "Narrative arc shapes MOJO envelope length — memory decaying through hot pink.",
    ),
    "ORCA":      (
        "Apex Mojo",
        "Orca hunt macro triggers MOJO maximum gain — apex-predator drive event.",
        "Fat Breach",
        "Breach events flip the analog/digital axis — each leap a timbral inversion.",
    ),
    "OCTOPUS":   (
        "Ink Mojo",
        "Octopus ink threshold gates MOJO release time — dense ink, long saturation tail.",
        "Fat Eight",
        "Eight chromatophore channels route into MOJO input — alien overdrive.",
    ),
    "OVERLAP":   (
        "Knot Mojo",
        "FDN feedback topology reshapes MOJO harmonics — Lion's Mane saturation.",
        "Fat Overlap",
        "Overlap node density modulates MOJO drive floor — knot topology in the gain chain.",
    ),
    "OUTWIT":    (
        "Arm Mojo",
        "Eight Wolfram CA arms each feed a MOJO stage — eight-arm saturation matrix.",
        "Fat Outwit",
        "CA rule selection changes MOJO character — cellular automata as drive algorithm.",
    ),
}

# ──────────────────────────────────────────────────────────────
# Helpers
# ──────────────────────────────────────────────────────────────

def jitter(val, amount=0.08, rng=None):
    if rng is None:
        rng = random
    return round(max(0.0, min(1.0, val + rng.uniform(-amount, amount))), 3)


def blend_dna(engine_a, engine_b, rng):
    keys = ["brightness", "warmth", "movement", "density", "space", "aggression"]
    a = ENGINE_DNA[engine_a]
    b = ENGINE_DNA[engine_b]
    return {k: jitter((a[k] + b[k]) / 2.0, 0.08, rng) for k in keys}


def make_macros(engine_a, engine_b, rng):
    dna = blend_dna(engine_a, engine_b, rng)
    return {
        "CHARACTER": round(jitter((dna["aggression"] + dna["warmth"]) / 2.0, 0.08, rng), 3),
        "MOVEMENT":  round(jitter(dna["movement"], 0.08, rng), 3),
        "COUPLING":  round(rng.uniform(0.55, 0.85), 3),
        "SPACE":     round(jitter(dna["space"], 0.08, rng), 3),
    }


def make_odyssey_params(rng):
    return {
        "drift_oscA_mode":      round(rng.choice([0.0, 0.25, 0.5, 0.75, 1.0]), 2),
        "drift_oscA_pitch":     round(rng.uniform(-0.5, 0.5), 3),
        "drift_oscA_level":     round(rng.uniform(0.5, 0.85), 3),
        "drift_oscB_mode":      round(rng.choice([0.0, 0.25, 0.5, 0.75, 1.0]), 2),
        "drift_oscB_pitch":     round(rng.uniform(-0.5, 0.5), 3),
        "drift_oscB_level":     round(rng.uniform(0.4, 0.8), 3),
        "drift_wtPosition":     round(rng.uniform(0.1, 0.9), 3),
        "drift_wtMorph":        round(rng.uniform(0.2, 0.8), 3),
        "drift_fmRatio":        round(rng.choice([1.0, 1.5, 2.0, 3.0, 4.0, 7.0]), 1),
        "drift_fmDepth":        round(rng.uniform(0.1, 0.7), 3),
        "drift_filterCutoff":   round(rng.uniform(1200.0, 10000.0), 1),
        "drift_filterReso":     round(rng.uniform(0.1, 0.65), 3),
        "drift_filterEnvAmt":   round(rng.uniform(0.2, 0.7), 3),
        "drift_ampAttack":      round(rng.uniform(0.005, 0.15), 4),
        "drift_ampDecay":       round(rng.uniform(0.2, 1.0), 3),
        "drift_ampSustain":     round(rng.uniform(0.35, 0.8), 3),
        "drift_ampRelease":     round(rng.uniform(0.3, 1.5), 3),
        "drift_modAttack":      round(rng.uniform(0.005, 0.08), 4),
        "drift_modDecay":       round(rng.uniform(0.1, 0.6), 3),
        "drift_modSustain":     round(rng.uniform(0.2, 0.7), 3),
        "drift_modRelease":     round(rng.uniform(0.3, 1.2), 3),
        "drift_lfo1Rate":       round(rng.uniform(0.05, 4.0), 3),
        "drift_lfo1Depth":      round(rng.uniform(0.1, 0.7), 3),
        "drift_lfo1Shape":      round(rng.choice([0.0, 0.25, 0.5, 0.75, 1.0]), 2),
        "drift_lfo2Rate":       round(rng.uniform(0.03, 2.0), 3),
        "drift_lfo2Depth":      round(rng.uniform(0.1, 0.55), 3),
        "drift_level":          round(rng.uniform(0.65, 0.85), 3),
        "drift_chorus":         round(rng.uniform(0.0, 0.5), 3),
        "drift_reverbMix":      round(rng.uniform(0.1, 0.55), 3),
    }


def make_oblong_params(rng):
    return {
        "bob_fltCutoff":        round(rng.uniform(800.0, 8000.0), 1),
        "bob_fltReso":          round(rng.uniform(0.1, 0.6), 3),
        "bob_fltEnvAmt":        round(rng.uniform(0.2, 0.75), 3),
        "bob_oscWave":          round(rng.choice([0.0, 0.33, 0.67, 1.0]), 2),
        "bob_oscDetune":        round(rng.uniform(-0.3, 0.3), 3),
        "bob_subOscLevel":      round(rng.uniform(0.0, 0.7), 3),
        "bob_distDrive":        round(rng.uniform(0.0, 0.5), 3),
        "bob_distMix":          round(rng.uniform(0.0, 0.45), 3),
        "bob_ampAttack":        round(rng.uniform(0.003, 0.05), 4),
        "bob_ampDecay":         round(rng.uniform(0.1, 0.8), 3),
        "bob_ampSustain":       round(rng.uniform(0.4, 0.85), 3),
        "bob_ampRelease":       round(rng.uniform(0.1, 1.0), 3),
        "bob_modAttack":        round(rng.uniform(0.003, 0.05), 4),
        "bob_modDecay":         round(rng.uniform(0.1, 0.5), 3),
        "bob_modSustain":       round(rng.uniform(0.3, 0.7), 3),
        "bob_modRelease":       round(rng.uniform(0.2, 0.9), 3),
        "bob_lfo1Rate":         round(rng.uniform(0.1, 5.0), 3),
        "bob_lfo1Depth":        round(rng.uniform(0.1, 0.65), 3),
        "bob_lfo1Shape":        round(rng.choice([0.0, 0.25, 0.5, 0.75, 1.0]), 2),
        "bob_chordMode":        round(rng.choice([0.0, 0.33, 0.67, 1.0]), 2),
        "bob_scaleMode":        round(rng.choice([0.0, 0.2, 0.4, 0.6, 0.8, 1.0]), 2),
        "bob_level":            round(rng.uniform(0.65, 0.85), 3),
        "bob_reverbMix":        round(rng.uniform(0.05, 0.4), 3),
    }


def make_obese_params(rng):
    return {
        "fat_satDrive":         round(rng.uniform(0.3, 0.9), 3),
        "fat_satMix":           round(rng.uniform(0.4, 0.9), 3),
        "fat_mojoControl":      round(rng.uniform(0.2, 0.8), 3),
        "fat_analogDigital":    round(rng.uniform(0.0, 1.0), 3),
        "fat_filterCutoff":     round(rng.uniform(500.0, 7000.0), 1),
        "fat_filterReso":       round(rng.uniform(0.1, 0.6), 3),
        "fat_filterEnvAmt":     round(rng.uniform(0.3, 0.8), 3),
        "fat_oscWave":          round(rng.choice([0.0, 0.33, 0.67, 1.0]), 2),
        "fat_oscDetune":        round(rng.uniform(-0.4, 0.4), 3),
        "fat_subOscLevel":      round(rng.uniform(0.2, 0.8), 3),
        "fat_distType":         round(rng.choice([0.0, 0.25, 0.5, 0.75, 1.0]), 2),
        "fat_ampAttack":        round(rng.uniform(0.003, 0.05), 4),
        "fat_ampDecay":         round(rng.uniform(0.05, 0.6), 3),
        "fat_ampSustain":       round(rng.uniform(0.4, 0.9), 3),
        "fat_ampRelease":       round(rng.uniform(0.05, 0.8), 3),
        "fat_modAttack":        round(rng.uniform(0.003, 0.04), 4),
        "fat_modDecay":         round(rng.uniform(0.05, 0.4), 3),
        "fat_modSustain":       round(rng.uniform(0.3, 0.7), 3),
        "fat_modRelease":       round(rng.uniform(0.1, 0.7), 3),
        "fat_lfo1Rate":         round(rng.uniform(0.1, 6.0), 3),
        "fat_lfo1Depth":        round(rng.uniform(0.1, 0.7), 3),
        "fat_lfo1Shape":        round(rng.choice([0.0, 0.25, 0.5, 0.75, 1.0]), 2),
        "fat_level":            round(rng.uniform(0.6, 0.82), 3),
        "fat_reverbMix":        round(rng.uniform(0.0, 0.35), 3),
    }


def make_partner_params(engine_id, rng):
    prefix = ENGINE_PREFIX.get(engine_id, engine_id.lower() + "_")
    return {
        f"{prefix}level":        round(rng.uniform(0.65, 0.85), 3),
        f"{prefix}filterCutoff": round(rng.uniform(1500.0, 10000.0), 1),
    }


def pick_coupling(idx, source, target):
    ctype  = COUPLING_TYPES[idx % len(COUPLING_TYPES)]
    amount = round(0.5 + (idx % 7) * 0.05, 2)
    return {
        "type":   ctype,
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
    safe = name.replace(" ", "_").replace("/", "-").replace(":", "").replace("'", "").replace("–", "-")
    return safe


def write_preset(preset, filename):
    path = os.path.join(OUTPUT_DIR, filename)
    if os.path.exists(path):
        return False
    with open(path, "w", encoding="utf-8") as f:
        json.dump(preset, f, indent=2, ensure_ascii=False)
        f.write("\n")
    return True


# ──────────────────────────────────────────────────────────────
# Generator: 2 variants per pair
# ──────────────────────────────────────────────────────────────

def generate_engine_pairs(engine_id, partners, name_table, param_fn, rng, seed_base, file_prefix):
    """Generate 2 presets per (engine_id × partner) pair."""
    count = 0
    for i, partner in enumerate(partners):
        names = name_table.get(partner)
        if names is None:
            print(f"  [WARN] No name entry for {engine_id}×{partner} — skipping")
            continue
        name_a, desc_a, name_b, desc_b = names

        for variant_idx, (name, desc) in enumerate([(name_a, desc_a), (name_b, desc_b)]):
            seed = seed_base + i * 10 + variant_idx
            rng.seed(seed)

            dna    = blend_dna(engine_id, partner, rng)
            macros = make_macros(engine_id, partner, rng)

            engine_params  = param_fn(rng)
            partner_params = make_partner_params(partner, rng)
            params = {
                ENGINE_DISPLAY.get(engine_id, engine_id): engine_params,
                ENGINE_DISPLAY.get(partner, partner):     partner_params,
            }

            coupling = pick_coupling(seed_base + i + variant_idx, engine_id, partner)

            tags = [
                "entangled", "coupling",
                engine_id.lower(), partner.lower(),
            ]

            preset = build_preset(name, [engine_id, partner], desc, tags, dna, macros, params, coupling)
            fname  = f"{file_prefix}_{partner.capitalize()}_{safe_filename(name)}.xometa"
            written = write_preset(preset, fname)
            status  = "wrote" if written else "skipped"
            print(f"  [{status}] {fname}")
            if written:
                count += 1
    return count


# ──────────────────────────────────────────────────────────────
# Main
# ──────────────────────────────────────────────────────────────

def main():
    os.makedirs(OUTPUT_DIR, exist_ok=True)
    rng = random.Random()

    print("\n=== ODYSSEY Coverage Pack (17 partners × 2 = 34 presets) ===")
    a = generate_engine_pairs(
        "ODYSSEY", ODYSSEY_PARTNERS, ODYSSEY_NAMES, make_odyssey_params,
        rng, seed_base=5000, file_prefix="Odyssey",
    )

    print("\n=== OBLONG Coverage Pack (13 partners × 2 = 26 presets) ===")
    b = generate_engine_pairs(
        "OBLONG", OBLONG_PARTNERS, OBLONG_NAMES, make_oblong_params,
        rng, seed_base=6000, file_prefix="Oblong",
    )

    print("\n=== OBESE Coverage Pack (17 partners × 2 = 34 presets) ===")
    c = generate_engine_pairs(
        "OBESE", OBESE_PARTNERS, OBESE_NAMES, make_obese_params,
        rng, seed_base=7000, file_prefix="Obese",
    )

    total = a + b + c
    print(f"\nDone — {total} new presets written to {OUTPUT_DIR}")
    print(f"  ODYSSEY: {a}  |  OBLONG: {b}  |  OBESE: {c}")


if __name__ == "__main__":
    main()
