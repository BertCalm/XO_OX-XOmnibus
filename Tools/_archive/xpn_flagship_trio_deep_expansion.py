#!/usr/bin/env python3
"""Deep coupling-coverage expansion for OVERDUB, ODYSSEY, and OPAL.

These three flagship engines carry the most critical coupling relationships
in the fleet (OVERWORLD→OPAL, DRIFT→OPAL, OPAL→DUB) but have thin coverage
with many partner engines. This pack adds 2 presets per pair for:

  OVERDUB  ×  OPTIC OBLIQUE OCELOT OSPREY OSTERIA OCEANIC OWLFISH  (14)
  OVERDUB  ×  OHM ORPHICA OBBLIGATO OTTONI OLE OMBRE ORCA OCTOPUS  (16)
  ODYSSEY  ×  OPTIC OBLIQUE OCELOT OSPREY OSTERIA OCEANIC OWLFISH  (14)
  ODYSSEY  ×  OHM ORPHICA OBBLIGATO OTTONI OLE OMBRE ORCA OCTOPUS  (16)
  OPAL     ×  OPTIC OBLIQUE OCELOT OSPREY OSTERIA OCEANIC OWLFISH  (14)
  OPAL     ×  OHM ORPHICA OBBLIGATO OTTONI OLE OMBRE ORCA OCTOPUS  (16)

Total target: ~90 presets (skips any that already exist).

Usage:
    python3 Tools/xpn_flagship_trio_deep_expansion.py
    python3 Tools/xpn_flagship_trio_deep_expansion.py --dry-run
    python3 Tools/xpn_flagship_trio_deep_expansion.py --output-dir /tmp/test
"""

import argparse
import json
from pathlib import Path

# ---------------------------------------------------------------------------
# Engine DNA baselines
# ---------------------------------------------------------------------------

ENGINES = {
    # Flagship trio
    "OVERDUB": {
        "prefix": "dub_",
        "label": "dub synth tape delay spring reverb",
        "color": "Olive",
        "dna": {"brightness": 0.4, "warmth": 0.65, "movement": 0.45,
                "density": 0.6, "space": 0.55, "aggression": 0.35},
    },
    "ODYSSEY": {
        "prefix": "drift_",
        "label": "wavetable FM odyssey",
        "color": "Violet",
        "dna": {"brightness": 0.55, "warmth": 0.5, "movement": 0.65,
                "density": 0.55, "space": 0.6, "aggression": 0.35},
    },
    "OPAL": {
        "prefix": "opal_",
        "label": "granular synthesis",
        "color": "Lavender",
        "dna": {"brightness": 0.5, "warmth": 0.55, "movement": 0.6,
                "density": 0.5, "space": 0.7, "aggression": 0.2},
    },
    # Partner engines
    "OPTIC": {
        "prefix": "optic_",
        "label": "visual signal",
        "color": "Phosphor Green",
        "dna": {"brightness": 0.85, "warmth": 0.35, "movement": 0.65,
                "density": 0.45, "space": 0.55, "aggression": 0.4},
    },
    "OBLIQUE": {
        "prefix": "oblq_",
        "label": "slant strategy",
        "color": "Prism Violet",
        "dna": {"brightness": 0.45, "warmth": 0.5, "movement": 0.55,
                "density": 0.6, "space": 0.65, "aggression": 0.5},
    },
    "OCELOT": {
        "prefix": "ocel_",
        "label": "feline predator",
        "color": "Ocelot Tawny",
        "dna": {"brightness": 0.65, "warmth": 0.55, "movement": 0.75,
                "density": 0.55, "space": 0.45, "aggression": 0.7},
    },
    "OSPREY": {
        "prefix": "osprey_",
        "label": "shore plunge hunter",
        "color": "Azulejo Blue",
        "dna": {"brightness": 0.6, "warmth": 0.5, "movement": 0.7,
                "density": 0.55, "space": 0.6, "aggression": 0.55},
    },
    "OSTERIA": {
        "prefix": "osteria_",
        "label": "porto wine tavern",
        "color": "Porto Wine",
        "dna": {"brightness": 0.4, "warmth": 0.75, "movement": 0.45,
                "density": 0.65, "space": 0.5, "aggression": 0.45},
    },
    "OCEANIC": {
        "prefix": "ocean_",
        "label": "phosphorescent tidal",
        "color": "Phosphorescent Teal",
        "dna": {"brightness": 0.55, "warmth": 0.5, "movement": 0.75,
                "density": 0.6, "space": 0.7, "aggression": 0.4},
    },
    "OWLFISH": {
        "prefix": "owl_",
        "label": "abyssal pipe organ",
        "color": "Abyssal Gold",
        "dna": {"brightness": 0.45, "warmth": 0.6, "movement": 0.4,
                "density": 0.65, "space": 0.65, "aggression": 0.3},
    },
    "OHM": {
        "prefix": "ohm_",
        "label": "hippy dad jam",
        "color": "Sage",
        "dna": {"brightness": 0.5, "warmth": 0.75, "movement": 0.6,
                "density": 0.5, "space": 0.7, "aggression": 0.2},
    },
    "ORPHICA": {
        "prefix": "orph_",
        "label": "microsound harp",
        "color": "Siren Seafoam",
        "dna": {"brightness": 0.7, "warmth": 0.5, "movement": 0.8,
                "density": 0.4, "space": 0.6, "aggression": 0.15},
    },
    "OBBLIGATO": {
        "prefix": "obbl_",
        "label": "dual wind",
        "color": "Rascal Coral",
        "dna": {"brightness": 0.55, "warmth": 0.65, "movement": 0.6,
                "density": 0.65, "space": 0.6, "aggression": 0.45},
    },
    "OTTONI": {
        "prefix": "otto_",
        "label": "triple brass",
        "color": "Patina",
        "dna": {"brightness": 0.65, "warmth": 0.6, "movement": 0.5,
                "density": 0.7, "space": 0.5, "aggression": 0.6},
    },
    "OLE": {
        "prefix": "ole_",
        "label": "afro-latin trio",
        "color": "Hibiscus",
        "dna": {"brightness": 0.7, "warmth": 0.7, "movement": 0.85,
                "density": 0.6, "space": 0.5, "aggression": 0.5},
    },
    "OMBRE": {
        "prefix": "ombr_",
        "label": "shadow gradient",
        "color": "Shadow Mauve",
        "dna": {"brightness": 0.45, "warmth": 0.55, "movement": 0.45,
                "density": 0.5, "space": 0.75, "aggression": 0.2},
    },
    "ORCA": {
        "prefix": "orca_",
        "label": "apex predator pod",
        "color": "Deep Ocean",
        "dna": {"brightness": 0.6, "warmth": 0.45, "movement": 0.7,
                "density": 0.65, "space": 0.6, "aggression": 0.75},
    },
    "OCTOPUS": {
        "prefix": "octo_",
        "label": "eight-arm intelligence",
        "color": "Chromatophore Magenta",
        "dna": {"brightness": 0.5, "warmth": 0.55, "movement": 0.8,
                "density": 0.7, "space": 0.5, "aggression": 0.55},
    },
}

# ---------------------------------------------------------------------------
# Coupling types keyed by partner
# ---------------------------------------------------------------------------

COUPLING_TYPES = {
    "OPTIC":     "SPECTRAL_REFRACTION",
    "OBLIQUE":   "ANGULAR_DEFLECT",
    "OCELOT":    "PREDATOR_STALK",
    "OSPREY":    "COASTAL_DIVE",
    "OSTERIA":   "HARMONIC_BLEND",
    "OCEANIC":   "TIDAL_MODULATION",
    "OWLFISH":   "RESONANT_PIPE",
    "OHM":       "SYMPATHETIC_RESONANCE",
    "ORPHICA":   "GRAIN_SCATTER",
    "OBBLIGATO": "BREATH_COUPLING",
    "OTTONI":    "HARMONIC_BLEND",
    "OLE":       "RHYTHMIC_LOCK",
    "OMBRE":     "GRADIENT_DISSOLVE",
    "ORCA":      "ECHOLOCATION_SYNC",
    "OCTOPUS":   "NEURAL_WEAVE",
}

# ---------------------------------------------------------------------------
# Preset definitions
# Each entry: (flagship, partner, name_a, desc_a, name_b, desc_b, coupling_a, coupling_b)
# Naming palette: tape / grain / drift / spring / delay / spool / echo / shimmer /
#                 haze / fog / dub / loop / rewind / fragment / ghost / veil / bloom
# ---------------------------------------------------------------------------

PRESET_PAIRS = [

    # ─── OVERDUB × OPTIC ────────────────────────────────────────────────────
    ("OVERDUB", "OPTIC",
     "Tape Phosphor",
     "Dub delay trails caught in optical scan — each echo a phosphorescent afterimage",
     "Spring Signal",
     "Spring reverb bloom refracts through an optical array — resonance split to spectrum",
     0.65, 0.7),

    # ─── OVERDUB × OBLIQUE ──────────────────────────────────────────────────
    ("OVERDUB", "OBLIQUE",
     "Slant Dub",
     "Tape loop deflected mid-play — the expected echo arrives from a crooked angle",
     "Diagonal Reverb",
     "Spring trail bent obliquely at the peak — dub warmth arriving from wrong direction",
     0.6, 0.55),

    # ─── OVERDUB × OCELOT ───────────────────────────────────────────────────
    ("OVERDUB", "OCELOT",
     "Jungle Tape",
     "Dub loop stalked through dense foliage — feline tension coiling inside the delay",
     "Spring Pounce",
     "A sudden ocelot leap interrupts the spring reverb bloom — tension released in echo",
     0.7, 0.6),

    # ─── OVERDUB × OSPREY ───────────────────────────────────────────────────
    ("OVERDUB", "OSPREY",
     "Coastal Dub",
     "Shore winds carry the dub delay out to sea — osprey plunge timed to the echo",
     "Dive Spool",
     "Tape spool unspools at the moment of the plunge — shore-blue reverb expanding outward",
     0.65, 0.55),

    # ─── OVERDUB × OSTERIA ──────────────────────────────────────────────────
    ("OVERDUB", "OSTERIA",
     "Porto Dub Session",
     "Late-night tavern acoustics soaked in dub delay — warm wine-dark echo saturating wood",
     "Cellar Spring",
     "Osteria cellar reverb blends with dub spring — ancient stone meeting oil-drum resonance",
     0.6, 0.7),

    # ─── OVERDUB × OCEANIC ──────────────────────────────────────────────────
    ("OVERDUB", "OCEANIC",
     "Tidal Echo",
     "Dub delay cycles with tidal rhythm — each repeat a wave receding up the shore",
     "Phosphor Dub",
     "Oceanic phosphorescence lights the spring trail from within — bioluminescent reverb",
     0.65, 0.6),

    # ─── OVERDUB × OWLFISH ──────────────────────────────────────────────────
    ("OVERDUB", "OWLFISH",
     "Abyssal Spool",
     "Tape spool descends to abyssal depth — owlfish pipe harmonics resonate the delay",
     "Pipe Spring",
     "Spring reverb rings through Mixtur-Trautonium pipes — deep-water dub resonance",
     0.6, 0.65),

    # ─── OVERDUB × OHM ──────────────────────────────────────────────────────
    ("OVERDUB", "OHM",
     "Commune Echo",
     "Folk warmth held inside a dub delay loop — communal presence echoing forward in time",
     "Sage Tape",
     "Tape saturation mellows an ohm drone — warmth and grain dissolving into each other",
     0.55, 0.6),

    # ─── OVERDUB × ORPHICA ──────────────────────────────────────────────────
    ("OVERDUB", "ORPHICA",
     "Harp Spool",
     "Orphica grain-shimmer drawn through dub tape — each grain a loop frozen in amber",
     "Crystal Echo",
     "Spring reverb carries microsound particles forward — shimmer locked inside the bloom",
     0.65, 0.7),

    # ─── OVERDUB × OBBLIGATO ────────────────────────────────────────────────
    ("OVERDUB", "OBBLIGATO",
     "Reed Delay",
     "Dual wind phrase caught in dub loop — breath suspended in tape saturation",
     "Bond Spring",
     "Spring reverb rings between two reed voices — the obligate bond sustained in echo",
     0.6, 0.55),

    # ─── OVERDUB × OTTONI ───────────────────────────────────────────────────
    ("OVERDUB", "OTTONI",
     "Brass Tape",
     "Triple brass fanfare fed to dub delay — patina aged further by tape saturation",
     "Patina Echo",
     "Spring reverb adds age to ottoni harmonics — brass weathered into warm dub texture",
     0.65, 0.6),

    # ─── OVERDUB × OLE ──────────────────────────────────────────────────────
    ("OVERDUB", "OLE",
     "Clave Loop",
     "Afro-Latin clave locked in dub loop — percussion building through tape repeats",
     "Carnival Delay",
     "Ole drama macro feeds dub delay — festival percussion echoing through the night",
     0.7, 0.65),

    # ─── OVERDUB × OMBRE ────────────────────────────────────────────────────
    ("OVERDUB", "OMBRE",
     "Dub At Dusk",
     "Dub loop fades through ombre gradient — each echo dimmer than the last at twilight",
     "Tape Dissolve",
     "Spring trail softens into shadow mauve — the reverb bloom dissolving to memory",
     0.55, 0.6),

    # ─── OVERDUB × ORCA ─────────────────────────────────────────────────────
    ("OVERDUB", "ORCA",
     "Echolocation Dub",
     "Orca click train processed through dub delay — cetacean sonar rebuilt as echo chamber",
     "Deep Spring",
     "Spring reverb descends to ocean depth — bloom expanding in cold dark pressure",
     0.7, 0.65),

    # ─── OVERDUB × OCTOPUS ──────────────────────────────────────────────────
    ("OVERDUB", "OCTOPUS",
     "Eight-Arm Spool",
     "Tape spool operated by eight independent arms — parallel loops weaving in phase",
     "Ink Echo",
     "Octopus ink cloud swallows the spring reverb — dub warmth diffusing through dark water",
     0.65, 0.7),

    # ─── ODYSSEY × OPTIC ────────────────────────────────────────────────────
    ("ODYSSEY", "OPTIC",
     "Drift Signal",
     "Wavetable odyssey caught in optical scan — FM harmonics refracted to phosphor columns",
     "Spectral Voyage",
     "Odyssey FM sideband spectrum beamed through optical prism — journey charted in light",
     0.7, 0.75),

    # ─── ODYSSEY × OBLIQUE ──────────────────────────────────────────────────
    ("ODYSSEY", "OBLIQUE",
     "Angled Voyage",
     "The odyssey path deflected mid-arc — wavetable trajectory arriving from slant angle",
     "Drift Fragment",
     "Oblique strategy intercepts drift FM — calculated redirection of an uncontrolled journey",
     0.6, 0.55),

    # ─── ODYSSEY × OCELOT ───────────────────────────────────────────────────
    ("ODYSSEY", "OCELOT",
     "Feline Drift",
     "FM odyssey stalked through jungle — ocelot ambush reshapes the wavetable trajectory",
     "Voyage Stalk",
     "Predatory tension threads through drifting harmonics — the journey watched from the dark",
     0.65, 0.7),

    # ─── ODYSSEY × OSPREY ───────────────────────────────────────────────────
    ("ODYSSEY", "OSPREY",
     "Shore Odyssey",
     "Drift FM lands at the shoreline — osprey plunge marks the destination of the voyage",
     "Coastal Drift",
     "Wavetable tide pushes the odyssey toward shore — drift current meeting the azure dive",
     0.65, 0.6),

    # ─── ODYSSEY × OSTERIA ──────────────────────────────────────────────────
    ("ODYSSEY", "OSTERIA",
     "Porto Voyage",
     "Drift odyssey docks at a wine-dark tavern — FM warmth soaked in osteria resonance",
     "Cellar Drift",
     "Wavetable FM drifts through stone cellar acoustics — odyssey warmth deepening in wood",
     0.6, 0.65),

    # ─── ODYSSEY × OCEANIC ──────────────────────────────────────────────────
    ("ODYSSEY", "OCEANIC",
     "Tidal Drift",
     "Wavetable odyssey carried on oceanic current — FM harmonics reshaped by tide pressure",
     "Phosphor Voyage",
     "Drift trajectory lit by bioluminescent wake — oceanic phosphorescence marking the path",
     0.65, 0.7),

    # ─── ODYSSEY × OWLFISH ──────────────────────────────────────────────────
    ("ODYSSEY", "OWLFISH",
     "Abyssal Drift",
     "Odyssey FM descends below the thermocline — owlfish pipe harmonics resonate the voyage",
     "Deep Voyage",
     "Mixtur-Trautonium partials intercept wavetable drift — ancient deep-water resonance",
     0.6, 0.55),

    # ─── ODYSSEY × OHM ──────────────────────────────────────────────────────
    ("ODYSSEY", "OHM",
     "Drift Commune",
     "FM odyssey passes through communal fire — ohm warmth slowing the wavetable drift",
     "Sage Voyage",
     "Odyssey trajectory rooted in sage warmth — the drift grounded by ohm resonance",
     0.55, 0.6),

    # ─── ODYSSEY × ORPHICA ──────────────────────────────────────────────────
    ("ODYSSEY", "ORPHICA",
     "Grain Voyage",
     "Orphica microsound cloud seeded into drift FM — grain particles carried on the odyssey",
     "Shimmer Drift",
     "Wavetable shimmer and orphica harp shimmer braided — two high-frequency journeys merged",
     0.7, 0.65),

    # ─── ODYSSEY × OBBLIGATO ────────────────────────────────────────────────
    ("ODYSSEY", "OBBLIGATO",
     "Wind Drift",
     "Dual wind phrase carried on FM current — breath and wavetable drift together outward",
     "Obligate Voyage",
     "The obligate bond sustained across drift distance — two reeds in FM orbit",
     0.6, 0.55),

    # ─── ODYSSEY × OTTONI ───────────────────────────────────────────────────
    ("ODYSSEY", "OTTONI",
     "Brass Drift",
     "Triple brass harmonics drawn into FM wavetable — patina aged by odyssey spectral warp",
     "Patina Voyage",
     "Ottoni density grounds the drift trajectory — brass weight slowing the FM journey",
     0.65, 0.6),

    # ─── ODYSSEY × OLE ──────────────────────────────────────────────────────
    ("ODYSSEY", "OLE",
     "Clave Drift",
     "Afro-Latin clave rhythm modulates FM drift — percussion reshaping the odyssey path",
     "Carnival Voyage",
     "Ole drama macro erupts inside drift FM — the journey interrupted by celebration",
     0.7, 0.65),

    # ─── ODYSSEY × OMBRE ────────────────────────────────────────────────────
    ("ODYSSEY", "OMBRE",
     "Drift Into Evening",
     "Odyssey wavetable fades through ombre gradient — the voyage dissolving to shadow",
     "Violet Dissolve",
     "Drift violet and shadow mauve bleed together — FM journey softening at the horizon",
     0.55, 0.6),

    # ─── ODYSSEY × ORCA ─────────────────────────────────────────────────────
    ("ODYSSEY", "ORCA",
     "Pod Drift",
     "Orca pod navigates by drift FM — wavetable odyssey mapped as echolocation terrain",
     "Echoic Voyage",
     "Click train and FM sideband merge — cetacean intelligence guiding the odyssey path",
     0.7, 0.65),

    # ─── ODYSSEY × OCTOPUS ──────────────────────────────────────────────────
    ("ODYSSEY", "OCTOPUS",
     "Eight-Arm Drift",
     "Wavetable drift distributed across eight arms — each limb pursuing a separate odyssey",
     "Neural Voyage",
     "Octopus neural net processes FM trajectory — decentralized intelligence routing the drift",
     0.65, 0.7),

    # ─── OPAL × OPTIC ───────────────────────────────────────────────────────
    ("OPAL", "OPTIC",
     "Grain Signal",
     "Granular particles scattered through optical scan — each grain a photon burst",
     "Crystal Spectrum",
     "Opal shimmer refracted through prismatic lens — grain cloud split to spectral rays",
     0.75, 0.8),

    # ─── OPAL × OBLIQUE ─────────────────────────────────────────────────────
    ("OPAL", "OBLIQUE",
     "Slant Grain",
     "Granular cloud deflected by oblique strategy — grain particles arriving from wrong angle",
     "Angled Cloud",
     "Opal grain scatter redirected mid-flight — oblique intelligence reshaping the granular field",
     0.6, 0.55),

    # ─── OPAL × OCELOT ──────────────────────────────────────────────────────
    ("OPAL", "OCELOT",
     "Grain Stalk",
     "Granular cloud stalked through jungle canopy — ocelot tension threading the grain field",
     "Canopy Shimmer",
     "Opal shimmer filters through jungle foliage — feline presence shaping the grain density",
     0.65, 0.7),

    # ─── OPAL × OSPREY ──────────────────────────────────────────────────────
    ("OPAL", "OSPREY",
     "Coastal Grain",
     "Granular particles carried on shore wind — osprey dive scattering the opal cloud",
     "Plunge Shimmer",
     "Grain cloud breaks at the waterline — osprey plunge moment frozen in opal shimmer",
     0.65, 0.6),

    # ─── OPAL × OSTERIA ─────────────────────────────────────────────────────
    ("OPAL", "OSTERIA",
     "Tavern Grain",
     "Granular warmth soaked in osteria resonance — opal shimmer pooling in stone acoustics",
     "Porto Shimmer",
     "Porto wine-dark warmth diffuses through opal grain field — granular tavern texture",
     0.6, 0.65),

    # ─── OPAL × OCEANIC ─────────────────────────────────────────────────────
    ("OPAL", "OCEANIC",
     "Tidal Grain",
     "Granular particles swept on oceanic current — opal cloud dispersed by tidal pressure",
     "Phosphor Shimmer",
     "Bioluminescent wake illuminates the grain field — opal shimmer as living ocean light",
     0.65, 0.75),

    # ─── OPAL × OWLFISH ─────────────────────────────────────────────────────
    ("OPAL", "OWLFISH",
     "Abyssal Grain",
     "Opal grain cloud descends past the thermocline — owlfish pipes resonating through particles",
     "Pipe Shimmer",
     "Mixtur-Trautonium harmonics scatter the grain field — abyssal tone meeting granular light",
     0.6, 0.55),

    # ─── OPAL × OHM ─────────────────────────────────────────────────────────
    ("OPAL", "OHM",
     "Commune Grain",
     "Communal ohm warmth seeded into granular field — folk presence distributed across cloud",
     "Sage Shimmer",
     "Opal shimmer and sage warmth dissolve into each other — granular community at rest",
     0.55, 0.65),

    # ─── OPAL × ORPHICA ─────────────────────────────────────────────────────
    ("OPAL", "ORPHICA",
     "Harp Grain",
     "Orphica harp plucks seeding the granular cloud — microsound and grain in conversation",
     "Shimmer Cloud",
     "Opal grain shimmer and orphica crystal shimmer braided — two high-frequency textures merged",
     0.7, 0.75),

    # ─── OPAL × OBBLIGATO ───────────────────────────────────────────────────
    ("OPAL", "OBBLIGATO",
     "Breath Grain",
     "Dual wind breath dispersed into granular field — exhale becoming grain cloud scatter",
     "Bond Shimmer",
     "The obligate bond sustained in opal shimmer — two voices dissolved into grain texture",
     0.6, 0.55),

    # ─── OPAL × OTTONI ──────────────────────────────────────────────────────
    ("OPAL", "OTTONI",
     "Brass Grain",
     "Triple brass harmonics granularized — patina aged to grain shimmer texture",
     "Patina Shimmer",
     "Ottoni density grounds the opal grain field — brass weight settling the shimmer cloud",
     0.65, 0.6),

    # ─── OPAL × OLE ─────────────────────────────────────────────────────────
    ("OPAL", "OLE",
     "Clave Grain",
     "Afro-Latin clave rhythm scattered across grain field — percussion becoming texture",
     "Festival Shimmer",
     "Ole drama macro erupts inside opal cloud — the carnival explosion granularized",
     0.7, 0.65),

    # ─── OPAL × OMBRE ───────────────────────────────────────────────────────
    ("OPAL", "OMBRE",
     "Grain Memory Fade",
     "Granular cloud softens through ombre gradient — each grain dimmer at the horizon",
     "Shimmer Dissolve",
     "Opal shimmer bleeds into shadow mauve — lavender grain dissolving to twilight",
     0.55, 0.65),

    # ─── OPAL × ORCA ────────────────────────────────────────────────────────
    ("OPAL", "ORCA",
     "Echoic Grain",
     "Orca click train seeds the granular field — echolocation returns as opal shimmer",
     "Pod Cloud",
     "Opal grain cloud navigated by orca pod — granular terrain mapped by echolocation",
     0.7, 0.65),

    # ─── OPAL × OCTOPUS ─────────────────────────────────────────────────────
    ("OPAL", "OCTOPUS",
     "Chromatophore Grain",
     "Octopus chromatophore patterns distributed across grain field — skin-color as texture",
     "Neural Shimmer",
     "Eight-arm intelligence routing the granular cloud — opal shimmer processed through neural net",
     0.65, 0.7),
]

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def blend_dna(dna_a, dna_b, weight_a=0.6):
    """Blend two DNA dicts, weighted toward the flagship engine."""
    w_b = 1.0 - weight_a
    return {k: round(dna_a[k] * weight_a + dna_b[k] * w_b, 3) for k in dna_a}


def macro_defaults(flagship, partner):
    """Sensible macro defaults per flagship character."""
    base = {"CHARACTER": 0.5, "MOVEMENT": 0.5, "COUPLING": 0.65, "SPACE": 0.5}

    if flagship == "OVERDUB":
        base.update({"CHARACTER": 0.55, "MOVEMENT": 0.4, "SPACE": 0.6})
    elif flagship == "ODYSSEY":
        base.update({"CHARACTER": 0.5, "MOVEMENT": 0.65, "SPACE": 0.6})
    elif flagship == "OPAL":
        base.update({"CHARACTER": 0.45, "MOVEMENT": 0.6, "SPACE": 0.7})

    # Partner nudges
    if partner == "OCELOT":
        base["CHARACTER"] = round(min(1.0, base["CHARACTER"] + 0.1), 2)
    elif partner in ("OPTIC", "ORPHICA"):
        base["SPACE"] = round(min(1.0, base["SPACE"] + 0.1), 2)
    elif partner in ("ORCA", "OLE", "OCEANIC"):
        base["MOVEMENT"] = round(min(1.0, base["MOVEMENT"] + 0.1), 2)
    elif partner in ("OMBRE", "OWLFISH"):
        base["SPACE"] = round(min(1.0, base["SPACE"] + 0.1), 2)

    return base


def build_preset(name, flagship, partner, description, coupling_amount, variant_index):
    """Build a single .xometa preset dict."""
    fl = ENGINES[flagship]
    pa = ENGINES[partner]
    blended_dna = blend_dna(fl["dna"], pa["dna"], weight_a=0.6)
    macros = macro_defaults(flagship, partner)
    macros["COUPLING"] = round(coupling_amount, 2)

    source = flagship if variant_index == 0 else partner
    target = partner if variant_index == 0 else flagship

    tags = [
        "entangled", "coupling", "flagship",
        flagship.lower(), partner.lower(),
        fl["label"].split()[0].lower(),
    ]

    return {
        "name": name,
        "version": "1.0",
        "engines": [flagship, partner],
        "mood": "Entangled",
        "macros": macros,
        "sonic_dna": blended_dna,
        "parameters": {},
        "coupling": {
            "type": COUPLING_TYPES[partner],
            "source": source,
            "target": target,
            "amount": coupling_amount,
        },
        "tags": tags,
        "description": description,
    }


def safe_filename(name):
    """Convert preset name to safe filename."""
    safe = "".join(c if c.isalnum() or c in " _-" else "_" for c in name)
    return safe.strip() + ".xometa"


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main():
    parser = argparse.ArgumentParser(
        description="Generate flagship trio deep coupling-coverage presets"
    )
    parser.add_argument("--dry-run", action="store_true",
                        help="Print filenames without writing")
    parser.add_argument(
        "--output-dir",
        default=str(Path(__file__).parent.parent / "Presets" / "XOceanus" / "Entangled"),
        help="Output directory for .xometa files",
    )
    args = parser.parse_args()

    out_dir = Path(args.output_dir)
    if not args.dry_run:
        out_dir.mkdir(parents=True, exist_ok=True)

    written = 0
    skipped = 0

    for (flagship, partner,
         name_a, desc_a, name_b, desc_b,
         coupling_a, coupling_b) in PRESET_PAIRS:

        for idx, (name, desc, coupling) in enumerate([
            (name_a, desc_a, coupling_a),
            (name_b, desc_b, coupling_b),
        ]):
            preset = build_preset(name, flagship, partner, desc, coupling, idx)
            filename = safe_filename(name)
            filepath = out_dir / filename

            if filepath.exists():
                print(f"  SKIP  {filename}")
                skipped += 1
                continue

            if args.dry_run:
                print(f"  DRY   {filename}  ({flagship} × {partner})")
                written += 1
            else:
                filepath.write_text(
                    json.dumps(preset, indent=2) + "\n", encoding="utf-8"
                )
                print(f"  WRITE {filename}")
                written += 1

    print(f"\nDone — {written} written, {skipped} skipped.")


if __name__ == "__main__":
    main()
