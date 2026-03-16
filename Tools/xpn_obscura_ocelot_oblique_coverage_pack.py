#!/usr/bin/env python3
"""
xpn_obscura_ocelot_oblique_coverage_pack.py
Deep coverage expansion for OBSCURA, OCELOT, and OBLIQUE engines.

Generates:
  - 6 three-way marquee presets (OBSCURA × OCELOT × OBLIQUE)
  - 20 OBSCURA presets paired with new partners
  - 20 OCELOT presets paired with same partners
  - 20 OBLIQUE presets paired with same partners

Total: ~66 presets written to Presets/XOmnibus/Entangled/
Skips files that already exist.
"""

import json
import os
import random
import math
from datetime import date

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
REPO_ROOT  = os.path.dirname(SCRIPT_DIR)
OUTPUT_DIR = os.path.join(REPO_ROOT, "Presets", "XOmnibus", "Entangled")

TODAY   = str(date.today())
VERSION = "1.0.0"
AUTHOR  = "XO_OX Deep Coverage Pack 2026-03-16"

# ──────────────────────────────────────────────
# Engine DNA baselines
# ──────────────────────────────────────────────
ENGINE_DNA = {
    "OBSCURA":    {"brightness": 0.45, "warmth": 0.5,  "movement": 0.4,  "density": 0.6,  "space": 0.55, "aggression": 0.25},
    "OCELOT":     {"brightness": 0.6,  "warmth": 0.65, "movement": 0.6,  "density": 0.65, "space": 0.5,  "aggression": 0.5},
    "OBLIQUE":    {"brightness": 0.75, "warmth": 0.45, "movement": 0.8,  "density": 0.6,  "space": 0.6,  "aggression": 0.55},
    # Partners
    "ONSET":      {"brightness": 0.5,  "warmth": 0.4,  "movement": 0.8,  "density": 0.7,  "space": 0.4,  "aggression": 0.7},
    "OVERWORLD":  {"brightness": 0.7,  "warmth": 0.5,  "movement": 0.6,  "density": 0.6,  "space": 0.5,  "aggression": 0.5},
    "OPTIC":      {"brightness": 0.9,  "warmth": 0.3,  "movement": 0.8,  "density": 0.4,  "space": 0.5,  "aggression": 0.4},
    "OSPREY":     {"brightness": 0.6,  "warmth": 0.6,  "movement": 0.6,  "density": 0.5,  "space": 0.7,  "aggression": 0.4},
    "OSTERIA":    {"brightness": 0.5,  "warmth": 0.8,  "movement": 0.4,  "density": 0.7,  "space": 0.5,  "aggression": 0.3},
    "OCEANIC":    {"brightness": 0.5,  "warmth": 0.6,  "movement": 0.6,  "density": 0.6,  "space": 0.8,  "aggression": 0.3},
    "OWLFISH":    {"brightness": 0.5,  "warmth": 0.6,  "movement": 0.5,  "density": 0.6,  "space": 0.6,  "aggression": 0.4},
    "OHM":        {"brightness": 0.5,  "warmth": 0.7,  "movement": 0.4,  "density": 0.6,  "space": 0.6,  "aggression": 0.3},
    "ORPHICA":    {"brightness": 0.7,  "warmth": 0.5,  "movement": 0.7,  "density": 0.4,  "space": 0.7,  "aggression": 0.2},
    "OBBLIGATO":  {"brightness": 0.5,  "warmth": 0.6,  "movement": 0.5,  "density": 0.7,  "space": 0.5,  "aggression": 0.35},
    "OTTONI":     {"brightness": 0.6,  "warmth": 0.55, "movement": 0.5,  "density": 0.7,  "space": 0.5,  "aggression": 0.55},
    "OLE":        {"brightness": 0.7,  "warmth": 0.7,  "movement": 0.75, "density": 0.65, "space": 0.4,  "aggression": 0.6},
    "OMBRE":      {"brightness": 0.4,  "warmth": 0.6,  "movement": 0.5,  "density": 0.5,  "space": 0.7,  "aggression": 0.2},
    "ORCA":       {"brightness": 0.4,  "warmth": 0.35, "movement": 0.65, "density": 0.7,  "space": 0.55, "aggression": 0.75},
    "OCTOPUS":    {"brightness": 0.7,  "warmth": 0.4,  "movement": 0.9,  "density": 0.8,  "space": 0.5,  "aggression": 0.6},
    "OVERLAP":    {"brightness": 0.55, "warmth": 0.5,  "movement": 0.6,  "density": 0.75, "space": 0.65, "aggression": 0.35},
    "OUTWIT":     {"brightness": 0.65, "warmth": 0.4,  "movement": 0.85, "density": 0.7,  "space": 0.5,  "aggression": 0.65},
    "ODDFELIX":   {"brightness": 0.8,  "warmth": 0.5,  "movement": 0.7,  "density": 0.6,  "space": 0.5,  "aggression": 0.5},
    "ODDOSCAR":   {"brightness": 0.6,  "warmth": 0.8,  "movement": 0.6,  "density": 0.6,  "space": 0.6,  "aggression": 0.3},
    "ODYSSEY":    {"brightness": 0.6,  "warmth": 0.5,  "movement": 0.7,  "density": 0.6,  "space": 0.6,  "aggression": 0.4},
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

# The 20 partner engines for the solo-pair presets
PARTNERS = [
    "ONSET", "OVERWORLD", "OPTIC", "OSPREY", "OSTERIA",
    "OCEANIC", "OWLFISH", "OHM", "ORPHICA", "OBBLIGATO",
    "OTTONI", "OLE", "OMBRE", "ORCA", "OCTOPUS",
    "OVERLAP", "OUTWIT", "ODDFELIX", "ODDOSCAR", "ODYSSEY",
]

ENGINE_PREFIX = {
    "OBSCURA":   "obscura_",
    "OCELOT":    "ocelot_",
    "OBLIQUE":   "oblq_",
    "ONSET":     "perc_",
    "OVERWORLD": "ow_",
    "OPTIC":     "optic_",
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
    "ODDFELIX":  "snap_",
    "ODDOSCAR":  "morph_",
    "ODYSSEY":   "drift_",
}

ENGINE_DISPLAY = {
    "OBSCURA":   "Obscura",
    "OCELOT":    "Ocelot",
    "OBLIQUE":   "Oblique",
    "ONSET":     "Onset",
    "OVERWORLD": "Overworld",
    "OPTIC":     "Optic",
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
    "ODDFELIX":  "OddfeliX",
    "ODDOSCAR":  "OddOscar",
    "ODYSSEY":   "Odyssey",
}

# ──────────────────────────────────────────────
# Preset name tables
# ──────────────────────────────────────────────

# 6 three-way marquee: OBSCURA × OCELOT × OBLIQUE
MARQUEE_NAMES = [
    (
        "Silver Tawny Prism",
        "A daguerreotype of a stalking ocelot photographed through a prism — three natures "
        "captured in one chemical-optical instant: stillness, predation, refraction.",
    ),
    (
        "Biome Through Ground Glass",
        "Jungle canopy seen through a ground-glass viewfinder while a prism splits the "
        "light — the ocelot is a colour, the forest is silver, the angle is everything.",
    ),
    (
        "Stalker In The Emulsion",
        "Latent image of a cat mid-leap, mid-refraction, mid-disappearance — three "
        "half-visible things asserting themselves into a single vanishing point.",
    ),
    (
        "Tawny Daguerreotype Bounce",
        "The silver plate records not the subject but the angle at which it moved — "
        "ocelot rosettes as diffraction grating, jungle heat as developer fluid.",
    ),
    (
        "Oblique Safari",
        "An expedition conducted entirely at non-orthogonal angles: the daguerreotype "
        "tilts, the ocelot sidesteps, the prism refracts what neither can name.",
    ),
    (
        "Prism Rosette Exposure",
        "Sunlight through a prism lands on a daguerreotype mid-exposure; the ocelot "
        "walks through the spectrum — each spot a different wavelength of tawny.",
    ),
]

# OBSCURA solo preset names per partner
OBSCURA_PARTNER_NAMES = {
    "ONSET":     ("Silver Drum Plate",       "Drum transients exposed onto a daguerreotype plate — each hit a separate latent image accumulating into density."),
    "OVERWORLD": ("Chip Ghost Exposure",     "NES palette quantized to daguerreotype silver tones — four shades of phosphorescent memory on a pixel grid."),
    "OPTIC":     ("Photographic Pulse",      "Optic pulses as frame-advance on a daguerreotype strip — each flash burns a slightly different stiffness onto the plate."),
    "OSPREY":    ("Coastal Albumen",         "Shore-line salt air reacting with albumen plate — the osprey's cry develops the image from the outside in."),
    "OSTERIA":   ("Wine-Stained Daguerreotype","Porto tannins as developer; the image blooms wine-dark out of silver — depth coming up from the bass register."),
    "OCEANIC":   ("Bioluminescent Plate",    "Phosphorescent teal pressed into silver — an ocean creature leaving its light-signature on the collodion surface."),
    "OWLFISH":   ("Subharmonic Exposure",    "Mixtur-Trautonium subharmonics as chemical developer — the deeper the frequency, the richer the silver tone."),
    "OHM":       ("Commune Daguerreotype",   "A circle of people photographed in slow silver light — the sage resonance of togetherness preserved in emulsion."),
    "ORPHICA":   ("Harp Plate",              "Each pluck a separate spark on the silver plate — the harp's decay curve tracing its own arc in daguerreotype time."),
    "OBBLIGATO": ("Breath Exposure",         "Wind-instrument breath as bellows for the camera — pneumatic pressure determining the depth of the latent image."),
    "OTTONI":    ("Brass Patina Plate",      "Brass oxidation and silver chemistry meeting at the surface — the trumpet's harmonics leaving a patina watermark."),
    "OLE":       ("Drama Exposure",          "Hibiscus pigment as red-channel developer; the flamenco stomp determines the exposure time — drama measured in silver."),
    "OMBRE":     ("Double Memory Plate",     "Two temporal states superimposed on a single plate — the obscura's latent image bleeding into the ombre's forgetting."),
    "ORCA":      ("Apex Plate",              "Echolocation click mapped to shutter speed — the orca's hunting pulse determining how long the silver is exposed to light."),
    "OCTOPUS":   ("Ink Cloud Exposure",      "Cephalopod ink as dark-room chemical — the cloud developing the image from the outside edges inward."),
    "OVERLAP":   ("FDN Silver Web",          "FDN feedback mapped to developer concentration — each reflection building the daguerreotype one delay-bounce at a time."),
    "OUTWIT":    ("Cellular Exposure",       "Wolfram CA rule applied to halftone grid — cellular automata determining which silver grains activate on the plate."),
    "ODDFELIX":  ("Neon Tetra Plate",        "Bioluminescent tetra scales as UV light source — the school's movement burning a latent image in real time."),
    "ODDOSCAR":  ("Gill Silver",             "Axolotl gill filaments as silver threads on the plate — regeneration leaving its map in the emulsion."),
    "ODYSSEY":   ("Wavetable Photograph",    "Wavetable position as lens aperture — each scan position a slightly different exposure of the same daguerreotype subject."),
}

# OCELOT solo preset names per partner
OCELOT_PARTNER_NAMES = {
    "ONSET":     ("Savanna Drum Hunt",       "Percussion patterns mapped to ocelot stalk-rhythm — each kick a territorial marker, each snare a prey-contact reflex."),
    "OVERWORLD": ("Chip Rosette",            "NES sprite-flicker mapped to rosette pattern shift — the ocelot's coat as a tile-map updating at 60fps."),
    "OPTIC":     ("Phosphor Spot",           "Phosphor-green pulse triggering biome shift — the jungle flickers between forest, shore, and open savanna with each flash."),
    "OSPREY":    ("Shore Predator",          "Osprey's coastal data feeding ocelot biome range — a predator learning the boundary between land and tide."),
    "OSTERIA":   ("Jungle Tavern",           "Deep bass warmth of the osteria mapping to the ocelot's low-frequency rumble — two territorial resonances."),
    "OCEANIC":   ("Biome Coast",             "Chromatophore-teal meeting rosette-tawny at the water's edge — the ocelot drinks the ocean's chemical language."),
    "OWLFISH":   ("Subharmonic Stalk",       "Mixtur-Trautonium deep tones as ground vibration — the ocelot reads the subharmonics through its paws."),
    "OHM":       ("Commune Predator",        "Sage commune frequency modulating ocelot territorial range — the cat circling a campfire at a respectful distance."),
    "ORPHICA":   ("Harp Ambush",             "Orphica pluck triggering ocelot ambush macro — the microsound harp's high partials as small-prey frequency signature."),
    "OBBLIGATO": ("Breath Stalk",            "Wind-instrument breath pacing the ocelot's stalking rhythm — inhale on approach, exhale on strike."),
    "OTTONI":    ("Brass Roar",              "Triple brass GROW macro mapped to ocelot territorial call — the trumpet's power curve as predator assertion."),
    "OLE":       ("Hibiscus Hunter",         "Afro-Latin rhythm as the ocelot's movement pattern — DRAMA macro determining whether the hunt is visible or hidden."),
    "OMBRE":     ("Dusk Rosette",            "Biome-shift at the golden hour — the ocelot's coat changing pattern as light fails, ombre gradient as twilight camouflage."),
    "ORCA":      ("Apex Exchange",           "Two apex predators exchanging hunting-frequency data — echolocation and biome-shift as parallel sensory grammars."),
    "OCTOPUS":   ("Chromatophore Rosette",   "Ocelot rosette and octopus chromatophore as competing camouflage systems cross-coupled to a single texture output."),
    "OVERLAP":   ("FDN Territory",           "FDN room resonance defining the ocelot's territorial range — each decay time a different jungle corridor."),
    "OUTWIT":    ("CA Biome Map",            "Wolfram CA rule governing which biome tiles are active — the ocelot navigating a generative landscape."),
    "ODDFELIX":  ("Neon Prey",               "Neon tetra schooling pattern as prey-movement data — the ocelot reading spectral light as motion signature."),
    "ODDOSCAR":  ("Axolotl Encounter",       "Two biome-edge species meeting in the shallows — regeneration meeting predation in a low-aggression standoff."),
    "ODYSSEY":   ("Drifting Territory",      "Wavetable drift slowly shifting the ocelot's biome — the territory morphing from jungle to shore to open scrub."),
}

# OBLIQUE solo preset names per partner
OBLIQUE_PARTNER_NAMES = {
    "ONSET":     ("Ricochet Kit",            "RTJ percussion patterns bouncing off prism walls — each drum hit refracting into three rhythmic afterimages."),
    "OVERWORLD": ("Chip Prism",              "NES arpeggiation refracted through a prism — each semitone a different spectral band of the same chip sequence."),
    "OPTIC":     ("Phosphor Refraction",     "Optic pulse entering the prism from an oblique angle — the refraction angle determining which harmonic exits."),
    "OSPREY":    ("Coastal Angle",           "Shore-system harmonics arriving at non-orthogonal incidence — the prism bending the coast's musical language sideways."),
    "OSTERIA":   ("Wine-Prism Bass",         "Porto wine bass tones refracting upward through the prism — deep warmth split into its spectral constituents."),
    "OCEANIC":   ("Teal Refraction",         "Phosphorescent teal bioluminescence entering the prism edge-on — each wavelength exiting at a different depth."),
    "OWLFISH":   ("Subharmonic Prism",       "Mixtur-Trautonium subharmonics as the prism's optical medium — the lower the frequency, the greater the refraction index."),
    "OHM":       ("Commune Refraction",      "Sage commune drone entering the prism — the group's collective frequency split into its individual voice-harmonics."),
    "ORPHICA":   ("Harp Spectrum",           "Microsound harp pluck dispersed through the prism — each grain a separate wavelength scattered across the stereo field."),
    "OBBLIGATO": ("Breath Angle",            "Wind-instrument breath as the light source for the prism — the angle of attack determining the spread of overtones."),
    "OTTONI":    ("Brass Dispersion",        "Triple brass harmonics hitting the prism simultaneously — GROW macro controlling how wide the spectrum fans out."),
    "OLE":       ("Drama Spectrum",          "DRAMA macro as prism rotation — the further it turns, the more the Afro-Latin rhythm separates into its constituent colors."),
    "OMBRE":     ("Shadow Prism",            "Ombre gradient as the darkening medium inside the prism — light entering full-spectrum and exiting monochromatic mauve."),
    "ORCA":      ("Echolocation Prism",      "Orca click train refracted through the prism — the hunt sonar dispersed into a full-spectrum rhythmic constellation."),
    "OCTOPUS":   ("Mantle Prism",            "Chromatophore state as prism refractive index — the octopus choosing which wavelengths pass through its skin."),
    "OVERLAP":   ("FDN Diffraction",         "FDN feedback paths as prism facets — each delay tap a separate refractive surface bending the signal further."),
    "OUTWIT":    ("CA Prism Rule",           "Wolfram CA rule as the prism's internal geometry — each generation recomputing the refraction lattice."),
    "ODDFELIX":  ("Neon Spectrum",           "Neon tetra iridescence entering the prism — schooling-pattern harmonics dispersed into a full prismatic arc."),
    "ODDOSCAR":  ("Gill Refraction",         "Axolotl gill-branch geometry as a diffraction grating — regenerative growth determining the spectral output angle."),
    "ODYSSEY":   ("Drifting Prism",          "Wavetable drift slowly rotating the prism — the spectral output continuously shifting as the oscillator morphs."),
}

# ──────────────────────────────────────────────
# Helper functions
# ──────────────────────────────────────────────

def jitter(val, amount=0.1, rng=None):
    """Add random jitter ±amount, clamped to [0, 1]."""
    if rng is None:
        rng = random
    return round(max(0.0, min(1.0, val + rng.uniform(-amount, amount))), 3)


def blend_dna(engines, rng):
    """Blend DNA across 2 or 3 engines with equal weight, then jitter."""
    keys = ["brightness", "warmth", "movement", "density", "space", "aggression"]
    result = {}
    for k in keys:
        mid = sum(ENGINE_DNA[e][k] for e in engines) / len(engines)
        result[k] = jitter(mid, 0.1, rng)
    return result


def make_macro_values(engines, rng):
    """Generate macro values influenced by blended DNA."""
    dna = blend_dna(engines, rng)
    return {
        "CHARACTER": round(jitter((dna["aggression"] + dna["warmth"]) / 2.0, 0.1, rng), 3),
        "MOVEMENT":  round(jitter(dna["movement"], 0.1, rng), 3),
        "COUPLING":  round(rng.uniform(0.55, 0.85), 3),
        "SPACE":     round(jitter(dna["space"], 0.1, rng), 3),
    }


def make_obscura_params(rng):
    """Generate plausible OBSCURA parameter values (string stiffness model)."""
    return {
        "obscura_stiffness":      round(rng.uniform(0.2, 0.85), 3),
        "obscura_dampening":      round(rng.uniform(0.1, 0.7), 3),
        "obscura_exciterType":    round(rng.choice([0.0, 0.33, 0.66, 1.0]), 2),
        "obscura_exciterAmount":  round(rng.uniform(0.3, 0.9), 3),
        "obscura_plateBlend":     round(rng.uniform(0.0, 0.8), 3),
        "obscura_silverTone":     round(rng.uniform(0.3, 0.9), 3),
        "obscura_exposureTime":   round(rng.uniform(0.1, 0.8), 3),
        "obscura_chemicalDepth":  round(rng.uniform(0.2, 0.85), 3),
        "obscura_grainSize":      round(rng.uniform(0.1, 0.7), 3),
        "obscura_filterCutoff":   round(rng.uniform(1200.0, 9000.0), 1),
        "obscura_filterReso":     round(rng.uniform(0.1, 0.55), 3),
        "obscura_level":          round(rng.uniform(0.65, 0.82), 3),
        "obscura_ampAttack":      round(rng.uniform(0.01, 0.15), 4),
        "obscura_ampDecay":       round(rng.uniform(0.4, 1.2), 3),
        "obscura_ampSustain":     round(rng.uniform(0.4, 0.75), 3),
        "obscura_ampRelease":     round(rng.uniform(0.5, 2.0), 3),
        "obscura_modAttack":      round(rng.uniform(0.005, 0.08), 4),
        "obscura_modDecay":       round(rng.uniform(0.2, 0.8), 3),
        "obscura_modSustain":     round(rng.uniform(0.3, 0.7), 3),
        "obscura_modRelease":     round(rng.uniform(0.4, 1.2), 3),
        "obscura_lfo1Rate":       round(rng.uniform(0.05, 2.0), 3),
        "obscura_lfo1Depth":      round(rng.uniform(0.1, 0.55), 3),
        "obscura_lfo1Shape":      round(rng.choice([0.0, 0.25, 0.5, 0.75, 1.0]), 2),
        "obscura_lfo2Rate":       round(rng.uniform(0.02, 1.0), 3),
        "obscura_lfo2Depth":      round(rng.uniform(0.05, 0.4), 3),
        "obscura_lfo2Shape":      round(rng.choice([0.0, 0.25, 0.5, 0.75, 1.0]), 2),
    }


def make_ocelot_params(rng):
    """Generate plausible OCELOT parameter values (biome-shifting)."""
    return {
        "ocelot_biome":           round(rng.uniform(0.0, 1.0), 3),
        "ocelot_biomeShiftRate":  round(rng.uniform(0.05, 0.7), 3),
        "ocelot_biomeBlend":      round(rng.uniform(0.2, 0.9), 3),
        "ocelot_rosetteDensity":  round(rng.uniform(0.3, 0.9), 3),
        "ocelot_rosetteSize":     round(rng.uniform(0.2, 0.8), 3),
        "ocelot_stalkDepth":      round(rng.uniform(0.3, 0.85), 3),
        "ocelot_stalkRate":       round(rng.uniform(0.1, 0.6), 3),
        "ocelot_ambushThreshold": round(rng.uniform(0.5, 0.9), 3),
        "ocelot_territorialRange":round(rng.uniform(0.3, 0.9), 3),
        "ocelot_filterCutoff":    round(rng.uniform(1800.0, 10000.0), 1),
        "ocelot_filterReso":      round(rng.uniform(0.15, 0.65), 3),
        "ocelot_level":           round(rng.uniform(0.65, 0.85), 3),
        "ocelot_ampAttack":       round(rng.uniform(0.005, 0.1), 4),
        "ocelot_ampDecay":        round(rng.uniform(0.2, 0.9), 3),
        "ocelot_ampSustain":      round(rng.uniform(0.45, 0.8), 3),
        "ocelot_ampRelease":      round(rng.uniform(0.3, 1.4), 3),
        "ocelot_modAttack":       round(rng.uniform(0.005, 0.06), 4),
        "ocelot_modDecay":        round(rng.uniform(0.1, 0.55), 3),
        "ocelot_modSustain":      round(rng.uniform(0.3, 0.7), 3),
        "ocelot_modRelease":      round(rng.uniform(0.3, 1.0), 3),
        "ocelot_lfo1Rate":        round(rng.uniform(0.1, 3.0), 3),
        "ocelot_lfo1Depth":       round(rng.uniform(0.2, 0.7), 3),
        "ocelot_lfo1Shape":       round(rng.choice([0.0, 0.25, 0.5, 0.75, 1.0]), 2),
        "ocelot_lfo2Rate":        round(rng.uniform(0.05, 1.5), 3),
        "ocelot_lfo2Depth":       round(rng.uniform(0.1, 0.5), 3),
        "ocelot_lfo2Shape":       round(rng.choice([0.0, 0.25, 0.5, 0.75, 1.0]), 2),
    }


def make_oblique_params(rng):
    """Generate plausible OBLIQUE parameter values (RTJ × Funk × Tame Impala)."""
    return {
        "oblq_prismColor":        round(rng.uniform(0.0, 1.0), 3),
        "oblq_prismAngle":        round(rng.uniform(0.1, 0.9), 3),
        "oblq_refractIndex":      round(rng.uniform(0.3, 0.9), 3),
        "oblq_dispersion":        round(rng.uniform(0.2, 0.85), 3),
        "oblq_bounceDepth":       round(rng.uniform(0.3, 0.9), 3),
        "oblq_bounceDecay":       round(rng.uniform(0.2, 0.75), 3),
        "oblq_funkGroove":        round(rng.uniform(0.4, 1.0), 3),
        "oblq_rtjAggression":     round(rng.uniform(0.3, 0.85), 3),
        "oblq_psychWidth":        round(rng.uniform(0.4, 0.95), 3),
        "oblq_phaseShift":        round(rng.uniform(0.0, 1.0), 3),
        "oblq_filterCutoff":      round(rng.uniform(2000.0, 12000.0), 1),
        "oblq_filterReso":        round(rng.uniform(0.15, 0.65), 3),
        "oblq_level":             round(rng.uniform(0.65, 0.85), 3),
        "oblq_ampAttack":         round(rng.uniform(0.005, 0.08), 4),
        "oblq_ampDecay":          round(rng.uniform(0.15, 0.7), 3),
        "oblq_ampSustain":        round(rng.uniform(0.45, 0.8), 3),
        "oblq_ampRelease":        round(rng.uniform(0.25, 1.2), 3),
        "oblq_modAttack":         round(rng.uniform(0.005, 0.05), 4),
        "oblq_modDecay":          round(rng.uniform(0.1, 0.5), 3),
        "oblq_modSustain":        round(rng.uniform(0.35, 0.7), 3),
        "oblq_modRelease":        round(rng.uniform(0.3, 1.0), 3),
        "oblq_lfo1Rate":          round(rng.uniform(0.2, 4.0), 3),
        "oblq_lfo1Depth":         round(rng.uniform(0.2, 0.75), 3),
        "oblq_lfo1Shape":         round(rng.choice([0.0, 0.25, 0.5, 0.75, 1.0]), 2),
        "oblq_lfo2Rate":          round(rng.uniform(0.1, 2.5), 3),
        "oblq_lfo2Depth":         round(rng.uniform(0.1, 0.6), 3),
        "oblq_lfo2Shape":         round(rng.choice([0.0, 0.25, 0.5, 0.75, 1.0]), 2),
    }


def make_params_for(engine, rng):
    """Dispatch to the correct param generator."""
    if engine == "OBSCURA":
        return make_obscura_params(rng)
    if engine == "OCELOT":
        return make_ocelot_params(rng)
    if engine == "OBLIQUE":
        return make_oblique_params(rng)
    # Generic stub for partners — a handful of representative params
    pfx = ENGINE_PREFIX.get(engine, engine.lower() + "_")
    return {
        f"{pfx}filterCutoff": round(rng.uniform(1000.0, 10000.0), 1),
        f"{pfx}filterReso":   round(rng.uniform(0.1, 0.6), 3),
        f"{pfx}level":        round(rng.uniform(0.65, 0.85), 3),
        f"{pfx}ampAttack":    round(rng.uniform(0.005, 0.1), 4),
        f"{pfx}ampDecay":     round(rng.uniform(0.2, 0.9), 3),
        f"{pfx}ampSustain":   round(rng.uniform(0.4, 0.8), 3),
        f"{pfx}ampRelease":   round(rng.uniform(0.3, 1.5), 3),
        f"{pfx}lfo1Rate":     round(rng.uniform(0.1, 3.0), 3),
        f"{pfx}lfo1Depth":    round(rng.uniform(0.1, 0.6), 3),
    }


def build_preset(name, engines, description, rng, coupling_override=None):
    """Build a complete .xometa dict."""
    dna   = blend_dna(engines, rng)
    macro = make_macro_values(engines, rng)
    params = {}
    for e in engines:
        params.update(make_params_for(e, rng))

    coup_type = coupling_override or rng.choice(COUPLING_TYPES)
    source, target = engines[0], engines[-1]

    tags = ["entangled", "coupling"]
    if "OBSCURA" in engines:
        tags.append("daguerreotype")
    if "OCELOT" in engines:
        tags.append("biome-shifting")
    if "OBLIQUE" in engines:
        tags.append("prismatic")

    return {
        "name":        name,
        "version":     VERSION,
        "engines":     [ENGINE_DISPLAY[e] for e in engines],
        "mood":        "Entangled",
        "macros":      macro,
        "sonic_dna":   dna,
        "parameters":  params,
        "coupling": {
            "type":   coup_type,
            "source": ENGINE_DISPLAY[source],
            "target": ENGINE_DISPLAY[target],
            "amount": round(rng.uniform(0.5, 0.85), 3),
        },
        "tags":        tags,
        "description": description,
    }


def safe_filename(name):
    """Convert preset name to a safe filename."""
    return name.replace("/", "-").replace("\\", "-") + ".xometa"


def write_preset(preset):
    """Write a preset to OUTPUT_DIR, skipping if already exists."""
    fname = safe_filename(preset["name"])
    path  = os.path.join(OUTPUT_DIR, fname)
    if os.path.exists(path):
        return False, path
    with open(path, "w", encoding="utf-8") as fh:
        json.dump(preset, fh, indent=2, ensure_ascii=False)
    return True, path


# ──────────────────────────────────────────────
# Main
# ──────────────────────────────────────────────

def main():
    os.makedirs(OUTPUT_DIR, exist_ok=True)
    rng   = random.Random(20260316)   # deterministic seed for reproducibility
    written  = 0
    skipped  = 0
    total    = 0

    # ── 1. Six three-way marquee presets ──────────────────────────────────
    marquee_coupling_types = [
        "SPECTRAL_TRANSFER",
        "HARMONIC_BLEND",
        "WAVETABLE_MORPH",
        "FREQUENCY_MODULATION",
        "SPATIAL_BLEND",
        "FILTER_COUPLING",
    ]
    for i, (name, desc) in enumerate(MARQUEE_NAMES):
        engines = ["OBSCURA", "OCELOT", "OBLIQUE"]
        preset  = build_preset(name, engines, desc, rng,
                               coupling_override=marquee_coupling_types[i % len(marquee_coupling_types)])
        ok, path = write_preset(preset)
        total += 1
        if ok:
            written += 1
            print(f"  [WRITE]  {os.path.basename(path)}")
        else:
            skipped += 1
            print(f"  [SKIP]   {os.path.basename(path)}")

    # ── 2. OBSCURA × each of 20 partners ─────────────────────────────────
    for partner in PARTNERS:
        name, desc = OBSCURA_PARTNER_NAMES[partner]
        engines    = ["OBSCURA", partner]
        preset     = build_preset(name, engines, desc, rng)
        ok, path   = write_preset(preset)
        total += 1
        if ok:
            written += 1
            print(f"  [WRITE]  {os.path.basename(path)}")
        else:
            skipped += 1
            print(f"  [SKIP]   {os.path.basename(path)}")

    # ── 3. OCELOT × each of 20 partners ──────────────────────────────────
    for partner in PARTNERS:
        name, desc = OCELOT_PARTNER_NAMES[partner]
        engines    = ["OCELOT", partner]
        preset     = build_preset(name, engines, desc, rng)
        ok, path   = write_preset(preset)
        total += 1
        if ok:
            written += 1
            print(f"  [WRITE]  {os.path.basename(path)}")
        else:
            skipped += 1
            print(f"  [SKIP]   {os.path.basename(path)}")

    # ── 4. OBLIQUE × each of 20 partners ─────────────────────────────────
    for partner in PARTNERS:
        name, desc = OBLIQUE_PARTNER_NAMES[partner]
        engines    = ["OBLIQUE", partner]
        preset     = build_preset(name, engines, desc, rng)
        ok, path   = write_preset(preset)
        total += 1
        if ok:
            written += 1
            print(f"  [WRITE]  {os.path.basename(path)}")
        else:
            skipped += 1
            print(f"  [SKIP]   {os.path.basename(path)}")

    print()
    print(f"Done — {total} presets processed: {written} written, {skipped} skipped.")
    print(f"Output: {OUTPUT_DIR}")


if __name__ == "__main__":
    main()
