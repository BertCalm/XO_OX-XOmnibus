#!/usr/bin/env python3
"""
xpn_ouroboros_organon_deep_coupling_pack.py

Generates Entangled preset stubs for OUROBOROS × ORGANON deep coupling coverage.

OUROBOROS: Strange Attractor Red #FF2D2D — topology/chaos, B003 Leash Mechanism,
           B007 Velocity Coupling Outputs. Prefix: ouro_
ORGANON:   Bioluminescent Cyan #00CED1 — variational free energy metabolism, B011.
           Prefix: organon_

Generates:
  - 8 OUROBOROS×ORGANON marquee presets (chaos metabolism theme)
  - 20 OUROBOROS presets paired with other engines
  - 20 ORGANON presets paired with other engines
Total: ~48 presets. Skips files that already exist.

Output: Presets/XOceanus/Entangled/
"""

import json
import os
import sys

ENTANGLED_DIR = os.path.join(
    os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
    "Presets", "XOceanus", "Entangled"
)

# ---------------------------------------------------------------------------
# DNA baselines
# ---------------------------------------------------------------------------
OURO_DNA = {
    "brightness": 0.5, "warmth": 0.4, "movement": 0.85,
    "density": 0.7, "space": 0.4, "aggression": 0.75
}
ORGAN_DNA = {
    "brightness": 0.55, "warmth": 0.6, "movement": 0.7,
    "density": 0.65, "space": 0.55, "aggression": 0.35
}

def blend_dna(a, b, t=0.5):
    """Linear blend between two DNA dicts. t=0 → a, t=1 → b."""
    return {k: round(a[k] * (1 - t) + b[k] * t, 3) for k in a}

# ---------------------------------------------------------------------------
# Default parameter blocks
# ---------------------------------------------------------------------------
OURO_PARAMS_DEFAULT = {
    "ouro_topology": 1,
    "ouro_rate": 80.0,
    "ouro_chaosIndex": 0.5,
    "ouro_leash": 0.4,
    "ouro_theta": 0.0,
    "ouro_phi": 0.35,
    "ouro_damping": 0.3,
    "ouro_injection": 0.0
}

ORGAN_PARAMS_DEFAULT = {
    "organon_metabolicRate": 3.5,
    "organon_enzymeSelect": 600.0,
    "organon_catalystDrive": 1.2,
    "organon_dampingCoeff": 0.4,
    "organon_signalFlux": 0.8,
    "organon_phasonShift": 0.25,
    "organon_isotopeBalance": 0.5,
    "organon_lockIn": 0.0,
    "organon_membrane": 0.55,
    "organon_noiseColor": 0.6
}

# Engine parameter key → default value stubs for partner engines
PARTNER_PARAMS = {
    "OddfeliX":  {"snap_filterCutoff": 0.6, "snap_lfoRate": 0.5, "snap_resoAmount": 0.4},
    "OddOscar":  {"morph_morph": 0.5, "morph_filterCutoff": 0.55, "morph_envAttack": 0.3},
    "Overdub":   {"dub_oscWave": 0, "dub_sendAmount": 0.6, "dub_tapeDelay": 0.5},
    "Odyssey":   {"drift_oscA_mode": 0, "drift_filterCutoff": 0.55, "drift_lfoDepth": 0.5},
    "Oblong":    {"bob_fltCutoff": 0.6, "bob_drive": 0.45, "bob_lfoRate": 0.4},
    "Obese":     {"fat_satDrive": 0.55, "fat_filterCutoff": 0.5, "fat_mojoAnalog": 0.6},
    "Onset":     {"perc_noiseLevel": 0.6, "perc_decay": 0.4, "perc_pitch": 0.5},
    "Overworld": {"ow_era": 0.5, "ow_brightness": 0.5, "ow_lfoRate": 0.4},
    "Opal":      {"opal_grainSize": 0.5, "opal_density": 0.6, "opal_pitch": 0.0},
    "Orbital":   {"orb_brightness": 0.55, "orb_groupEnv": 0.5, "orb_filterCutoff": 0.5},
    "Obsidian":  {"obsidian_depth": 0.6, "obsidian_cutoff": 0.5, "obsidian_shimmer": 0.4},
    "Origami":   {"origami_foldPoint": 0.5, "origami_crease": 0.45, "origami_unfold": 0.3},
    "Oracle":    {"oracle_breakpoints": 4, "oracle_stochastic": 0.6, "oracle_maqam": 0},
    "Obscura":   {"obscura_stiffness": 0.5, "obscura_blur": 0.4, "obscura_grain": 0.6},
    "Oceanic":   {"ocean_separation": 0.5, "ocean_chromatophore": 0.6, "ocean_depth": 0.5},
    "Ocelot":    {"ocelot_biome": 0, "ocelot_sprint": 0.5, "ocelot_camouflage": 0.4},
    "Optic":     {"optic_pulseRate": 0.5, "optic_autoPulse": 1, "optic_brightness": 0.55},
    "Oblique":   {"oblq_prismColor": 0.5, "oblq_bounce": 0.6, "oblq_refraction": 0.45},
    "Osprey":    {"osprey_shoreBlend": 0.5, "osprey_soar": 0.6, "osprey_dive": 0.4},
    "Ohm":       {"ohm_macroMeddling": 0.5, "ohm_commune": 0.4, "ohm_jamDepth": 0.6},
}

PARTNER_ENGINE_IDS = {
    "OddfeliX": "ODDFELIX", "OddOscar": "ODDOSCAR", "Overdub": "OVERDUB",
    "Odyssey": "ODYSSEY", "Oblong": "OBLONG", "Obese": "OBESE",
    "Onset": "ONSET", "Overworld": "OVERWORLD", "Opal": "OPAL",
    "Orbital": "ORBITAL", "Obsidian": "OBSIDIAN", "Origami": "ORIGAMI",
    "Oracle": "ORACLE", "Obscura": "OBSCURA", "Oceanic": "OCEANIC",
    "Ocelot": "OCELOT", "Optic": "OPTIC", "Oblique": "OBLIQUE",
    "Osprey": "OSPREY", "Ohm": "OHM",
}

COUPLING_TYPES = [
    "Audio->FM", "Audio->AM", "CV->Filter", "CV->Pitch",
    "Envelope->Env", "LFO->LFO", "Audio->Ring", "Chaos->Timbre"
]

# ---------------------------------------------------------------------------
# Marquee OUROBOROS×ORGANON preset definitions (8 presets)
# ---------------------------------------------------------------------------
MARQUEE_PRESETS = [
    {
        "name": "Serpent Digests Light",
        "description": "Ouroboros swallows its own tail while Organon metabolises the resulting photons. Chaos feeds bioluminescence.",
        "macros": {"CHARACTER": 0.7, "MOVEMENT": 0.85, "COUPLING": 0.8, "SPACE": 0.45},
        "coupling_type": "Chaos->Timbre",
        "coupling_amount": 0.75,
        "ouro_overrides": {"ouro_chaosIndex": 0.8, "ouro_leash": 0.25, "ouro_rate": 110.0},
        "organ_overrides": {"organon_metabolicRate": 5.5, "organon_signalFlux": 0.95, "organon_membrane": 0.7},
        "dna_t": 0.4,
        "tags": ["marquee", "chaos-metabolism", "bioluminescent", "self-reference"]
    },
    {
        "name": "Attractor Enzyme",
        "description": "Strange attractor geometry mapped onto enzymatic catalysis. Each loop through phase space activates a new catalyst.",
        "macros": {"CHARACTER": 0.6, "MOVEMENT": 0.75, "COUPLING": 0.9, "SPACE": 0.5},
        "coupling_type": "Audio->FM",
        "coupling_amount": 0.85,
        "ouro_overrides": {"ouro_topology": 2, "ouro_chaosIndex": 0.6, "ouro_phi": 0.6},
        "organ_overrides": {"organon_enzymeSelect": 900.0, "organon_catalystDrive": 1.8, "organon_isotopeBalance": 0.65},
        "dna_t": 0.5,
        "tags": ["marquee", "attractor", "enzyme", "fm-coupling"]
    },
    {
        "name": "Free Energy Serpent",
        "description": "Variational free energy minimisation navigating a chaotic potential landscape. Prediction errors propagate as audio.",
        "macros": {"CHARACTER": 0.5, "MOVEMENT": 0.8, "COUPLING": 0.7, "SPACE": 0.6},
        "coupling_type": "CV->Filter",
        "coupling_amount": 0.65,
        "ouro_overrides": {"ouro_injection": 0.5, "ouro_damping": 0.2, "ouro_leash": 0.45},
        "organ_overrides": {"organon_phasonShift": 0.6, "organon_lockIn": 0.3, "organon_noiseColor": 0.8},
        "dna_t": 0.55,
        "tags": ["marquee", "free-energy", "prediction", "cascade"]
    },
    {
        "name": "Ouroboric Bioluminescence",
        "description": "The serpent's glow cycles through deep-sea colour shifts. Self-reference illuminates the organism.",
        "macros": {"CHARACTER": 0.4, "MOVEMENT": 0.7, "COUPLING": 0.6, "SPACE": 0.7},
        "coupling_type": "LFO->LFO",
        "coupling_amount": 0.7,
        "ouro_overrides": {"ouro_theta": 0.5, "ouro_phi": 0.5, "ouro_rate": 60.0},
        "organ_overrides": {"organon_membrane": 0.8, "organon_noiseColor": 0.45, "organon_dampingCoeff": 0.5},
        "dna_t": 0.6,
        "tags": ["marquee", "bioluminescent", "lfo-sync", "atmospheric"]
    },
    {
        "name": "Topology of Hunger",
        "description": "The serpent's topological mode switches describe hunger cycles. Organon responds with metabolic urgency.",
        "macros": {"CHARACTER": 0.8, "MOVEMENT": 0.9, "COUPLING": 0.85, "SPACE": 0.35},
        "coupling_type": "Envelope->Env",
        "coupling_amount": 0.8,
        "ouro_overrides": {"ouro_topology": 0, "ouro_rate": 140.0, "ouro_chaosIndex": 0.9},
        "organ_overrides": {"organon_metabolicRate": 6.5, "organon_catalystDrive": 2.2, "organon_signalFlux": 1.0},
        "dna_t": 0.35,
        "tags": ["marquee", "aggressive", "metabolic", "high-energy"]
    },
    {
        "name": "Leash on Living Fire",
        "description": "B003 Leash Mechanism restrains chaos just enough for Organon to sustain coherent metabolic patterns.",
        "macros": {"CHARACTER": 0.65, "MOVEMENT": 0.7, "COUPLING": 0.75, "SPACE": 0.5},
        "coupling_type": "CV->Pitch",
        "coupling_amount": 0.6,
        "ouro_overrides": {"ouro_leash": 0.6, "ouro_chaosIndex": 0.7, "ouro_injection": 0.3},
        "organ_overrides": {"organon_lockIn": 0.6, "organon_isotopeBalance": 0.4, "organon_membrane": 0.65},
        "dna_t": 0.45,
        "tags": ["marquee", "leash", "controlled-chaos", "B003"]
    },
    {
        "name": "Velocity of Metabolism",
        "description": "B007 Velocity Coupling Outputs feed directly into Organon's metabolic rate. Harder playing accelerates the organism.",
        "macros": {"CHARACTER": 0.55, "MOVEMENT": 0.8, "COUPLING": 0.95, "SPACE": 0.45},
        "coupling_type": "Audio->AM",
        "coupling_amount": 0.9,
        "ouro_overrides": {"ouro_rate": 90.0, "ouro_chaosIndex": 0.55, "ouro_damping": 0.25},
        "organ_overrides": {"organon_metabolicRate": 4.5, "organon_enzymeSelect": 750.0, "organon_signalFlux": 0.85},
        "dna_t": 0.5,
        "tags": ["marquee", "velocity", "coupling", "B007", "B011"]
    },
    {
        "name": "Publishable Attractor",
        "description": "B011 meets B003: the organism's variational free energy landscape IS the strange attractor. Genuinely novel synthesis.",
        "macros": {"CHARACTER": 0.5, "MOVEMENT": 0.75, "COUPLING": 1.0, "SPACE": 0.55},
        "coupling_type": "Chaos->Timbre",
        "coupling_amount": 1.0,
        "ouro_overrides": {"ouro_topology": 3, "ouro_chaosIndex": 0.75, "ouro_leash": 0.5, "ouro_phi": 0.7},
        "organ_overrides": {"organon_phasonShift": 0.5, "organon_isotopeBalance": 0.6, "organon_lockIn": 0.5, "organon_noiseColor": 0.7},
        "dna_t": 0.5,
        "tags": ["marquee", "B003", "B011", "publishable", "deep-coupling", "flagship"]
    },
]

# ---------------------------------------------------------------------------
# OUROBOROS solo-pair presets (20 presets, one per partner engine)
# ---------------------------------------------------------------------------
OURO_PAIR_PRESETS = [
    {
        "partner": "OddfeliX",
        "name": "Neon Attractor",
        "description": "Strange attractor geometry colours OddfeliX's neon tetra filter sweeps. Chaos traces fluorescent arcs.",
        "coupling_type": "CV->Filter",
        "coupling_amount": 0.65,
        "ouro_overrides": {"ouro_chaosIndex": 0.6, "ouro_rate": 85.0},
        "tags": ["attractor", "filter-chaos", "neon"]
    },
    {
        "partner": "OddOscar",
        "name": "Axolotl Loop",
        "description": "The serpent regenerates endlessly; the axolotl morphs through the shed skins. Mutual metamorphosis.",
        "coupling_type": "LFO->LFO",
        "coupling_amount": 0.7,
        "ouro_overrides": {"ouro_theta": 0.4, "ouro_phi": 0.45},
        "tags": ["regeneration", "morph", "loop"]
    },
    {
        "partner": "Overdub",
        "name": "Tape Serpent",
        "description": "Ouroboros feed loop recorded onto tape. Each revolution introduces vintage saturation and chaos.",
        "coupling_type": "Audio->AM",
        "coupling_amount": 0.6,
        "ouro_overrides": {"ouro_leash": 0.5, "ouro_rate": 70.0},
        "tags": ["tape", "feedback", "saturation"]
    },
    {
        "partner": "Odyssey",
        "name": "Drifting Attractor",
        "description": "Odyssey's oscillators drift in phase space carved by Ouroboros' strange attractor field.",
        "coupling_type": "CV->Pitch",
        "coupling_amount": 0.55,
        "ouro_overrides": {"ouro_topology": 1, "ouro_chaosIndex": 0.55},
        "tags": ["drift", "pitch-chaos", "phase-space"]
    },
    {
        "partner": "Oblong",
        "name": "Chaotic Drive",
        "description": "Ouroboros injects chaos into Oblong's drive circuit. The bass character mutates on every cycle.",
        "coupling_type": "Audio->FM",
        "coupling_amount": 0.7,
        "ouro_overrides": {"ouro_injection": 0.4, "ouro_rate": 95.0},
        "tags": ["bass", "drive", "injection"]
    },
    {
        "partner": "Obese",
        "name": "Mojo Serpent",
        "description": "The Leash Mechanism (B003) calibrates Obese's MOJO analog axis. Controlled chaos saturates the mix.",
        "coupling_type": "CV->Filter",
        "coupling_amount": 0.75,
        "ouro_overrides": {"ouro_leash": 0.35, "ouro_chaosIndex": 0.65},
        "tags": ["saturation", "mojo", "B003"]
    },
    {
        "partner": "Onset",
        "name": "Percussion Orbit",
        "description": "Strange attractor trajectories trigger Onset percussion events. Rhythm emerges from topology.",
        "coupling_type": "Chaos->Timbre",
        "coupling_amount": 0.8,
        "ouro_overrides": {"ouro_rate": 120.0, "ouro_chaosIndex": 0.75},
        "tags": ["percussion", "rhythm", "topology"]
    },
    {
        "partner": "Overworld",
        "name": "ERA Strange Loop",
        "description": "Overworld's ERA triangle navigated by chaotic phase coordinates. Chip eras cycle unpredictably.",
        "coupling_type": "CV->Filter",
        "coupling_amount": 0.6,
        "ouro_overrides": {"ouro_phi": 0.55, "ouro_theta": 0.4},
        "tags": ["era", "chip", "strange-loop"]
    },
    {
        "partner": "Opal",
        "name": "Granular Chaos",
        "description": "Grain size and position modulated by Ouroboros' attractor coordinates. Clouds form strange shapes.",
        "coupling_type": "LFO->LFO",
        "coupling_amount": 0.65,
        "ouro_overrides": {"ouro_damping": 0.4, "ouro_rate": 75.0},
        "tags": ["granular", "clouds", "chaos"]
    },
    {
        "partner": "Orbital",
        "name": "Orbital Attractor",
        "description": "Group Envelope System (B001) governed by attractor phase. Envelopes orbit the chaos core.",
        "coupling_type": "Envelope->Env",
        "coupling_amount": 0.7,
        "ouro_overrides": {"ouro_topology": 2, "ouro_chaosIndex": 0.6},
        "tags": ["orbital", "envelope", "B001"]
    },
    {
        "partner": "Obsidian",
        "name": "Fracture Attractor",
        "description": "Obsidian's crystal structure fractured by chaotic phase trajectories. Darkness refracts.",
        "coupling_type": "Audio->Ring",
        "coupling_amount": 0.65,
        "ouro_overrides": {"ouro_chaosIndex": 0.7, "ouro_phi": 0.5},
        "tags": ["crystal", "fracture", "ring-mod"]
    },
    {
        "partner": "Origami",
        "name": "Folded Phase Space",
        "description": "Each fold of Origami maps to a new basin in Ouroboros' phase space. Geometry begets geometry.",
        "coupling_type": "CV->Pitch",
        "coupling_amount": 0.6,
        "ouro_overrides": {"ouro_theta": 0.6, "ouro_phi": 0.4},
        "tags": ["fold", "geometry", "phase-space"]
    },
    {
        "partner": "Oracle",
        "name": "Stochastic Serpent",
        "description": "Oracle's GENDY stochastic synthesis seeded by Ouroboros' attractor state. Prophecy of chaos.",
        "coupling_type": "Chaos->Timbre",
        "coupling_amount": 0.75,
        "ouro_overrides": {"ouro_rate": 100.0, "ouro_leash": 0.45},
        "tags": ["stochastic", "GENDY", "prophecy"]
    },
    {
        "partner": "Obscura",
        "name": "Chaotic Blur",
        "description": "Obscura's photographic blur depth controlled by attractor position. Focus dissolves cyclically.",
        "coupling_type": "Audio->AM",
        "coupling_amount": 0.55,
        "ouro_overrides": {"ouro_damping": 0.35, "ouro_chaosIndex": 0.5},
        "tags": ["blur", "daguerreotype", "dissolve"]
    },
    {
        "partner": "Oceanic",
        "name": "Chaotic Chromatophore",
        "description": "Ouroboros topology maps to Oceanic's chromatophore modulator (B013). Colour change by chaos.",
        "coupling_type": "LFO->LFO",
        "coupling_amount": 0.7,
        "ouro_overrides": {"ouro_phi": 0.45, "ouro_rate": 80.0},
        "tags": ["chromatophore", "colour", "B013"]
    },
    {
        "partner": "Ocelot",
        "name": "Predator Attractor",
        "description": "Ocelot's sprint rhythm governed by attractor cycle period. Hunt sequences trace chaotic arcs.",
        "coupling_type": "CV->Filter",
        "coupling_amount": 0.65,
        "ouro_overrides": {"ouro_rate": 90.0, "ouro_leash": 0.4},
        "tags": ["predator", "sprint", "hunt"]
    },
    {
        "partner": "Optic",
        "name": "Visual Chaos",
        "description": "Optic visualiser mapped to Ouroboros attractor coordinates. Sound as geometry, geometry as light.",
        "coupling_type": "Audio->FM",
        "coupling_amount": 0.5,
        "ouro_overrides": {"ouro_chaosIndex": 0.5, "ouro_rate": 65.0},
        "tags": ["visual", "light", "geometry"]
    },
    {
        "partner": "Oblique",
        "name": "Prism Attractor",
        "description": "Oblique's prism bounce paths trace Lorenz attractor wings. Refraction as topology.",
        "coupling_type": "Audio->Ring",
        "coupling_amount": 0.7,
        "ouro_overrides": {"ouro_topology": 1, "ouro_phi": 0.6},
        "tags": ["prism", "refraction", "lorenz"]
    },
    {
        "partner": "Osprey",
        "name": "Coastal Chaos",
        "description": "ShoreSystem coastlines (B012) navigated by chaotic flight paths. The serpent dives like a raptor.",
        "coupling_type": "CV->Pitch",
        "coupling_amount": 0.6,
        "ouro_overrides": {"ouro_damping": 0.3, "ouro_chaosIndex": 0.6},
        "tags": ["shore", "coastal", "B012"]
    },
    {
        "partner": "Ohm",
        "name": "Commune Attractor",
        "description": "OHM's COMMUNE axis warped by strange attractor. Hippy Dad communal jam enters chaos territory.",
        "coupling_type": "LFO->LFO",
        "coupling_amount": 0.6,
        "ouro_overrides": {"ouro_leash": 0.55, "ouro_rate": 70.0},
        "tags": ["commune", "jam", "attractor"]
    },
]

# ---------------------------------------------------------------------------
# ORGANON solo-pair presets (20 presets, one per partner engine)
# ---------------------------------------------------------------------------
ORGAN_PAIR_PRESETS = [
    {
        "partner": "OddfeliX",
        "name": "Neon Metabolism",
        "description": "OddfeliX filter luminosity modulated by Organon's metabolic cycles. Neon tetra bioluminescence.",
        "coupling_type": "CV->Filter",
        "coupling_amount": 0.6,
        "organ_overrides": {"organon_metabolicRate": 3.0, "organon_membrane": 0.6},
        "tags": ["metabolism", "neon", "filter"]
    },
    {
        "partner": "OddOscar",
        "name": "Axolotl Enzyme",
        "description": "OddOscar's morphing driven by Organon enzyme cascades. Metamorphosis as biochemistry.",
        "coupling_type": "Envelope->Env",
        "coupling_amount": 0.65,
        "organ_overrides": {"organon_enzymeSelect": 500.0, "organon_catalystDrive": 1.5},
        "tags": ["enzyme", "morph", "biochemistry"]
    },
    {
        "partner": "Overdub",
        "name": "Metabolic Tape",
        "description": "Overdub tape decay rate governed by Organon metabolic rhythm. The organism digests reverb tails.",
        "coupling_type": "Audio->AM",
        "coupling_amount": 0.6,
        "organ_overrides": {"organon_signalFlux": 0.75, "organon_dampingCoeff": 0.35},
        "tags": ["tape", "decay", "rhythm"]
    },
    {
        "partner": "Odyssey",
        "name": "Drifting Organism",
        "description": "Odyssey oscillators tuned to Organon's isotropic resonance frequencies. Drift as metabolic process.",
        "coupling_type": "CV->Pitch",
        "coupling_amount": 0.55,
        "organ_overrides": {"organon_isotopeBalance": 0.7, "organon_phasonShift": 0.45},
        "tags": ["drift", "resonance", "isotope"]
    },
    {
        "partner": "Oblong",
        "name": "Bass Metabolism",
        "description": "Oblong sub frequencies shaped by Organon metabolic envelope. The organism breathes bass.",
        "coupling_type": "Audio->FM",
        "coupling_amount": 0.65,
        "organ_overrides": {"organon_metabolicRate": 2.5, "organon_membrane": 0.7},
        "tags": ["bass", "breath", "membrane"]
    },
    {
        "partner": "Obese",
        "name": "Living Saturation",
        "description": "Organon's metabolic output drives Obese saturation curves. The organism overdrives itself into life.",
        "coupling_type": "Audio->FM",
        "coupling_amount": 0.7,
        "organ_overrides": {"organon_catalystDrive": 1.8, "organon_signalFlux": 0.9},
        "tags": ["saturation", "overdrive", "organism"]
    },
    {
        "partner": "Onset",
        "name": "Percussive Pulse",
        "description": "Onset percussion events triggered by Organon metabolic pulse. Drumming as biological rhythm.",
        "coupling_type": "Envelope->Env",
        "coupling_amount": 0.75,
        "organ_overrides": {"organon_metabolicRate": 4.0, "organon_lockIn": 0.4},
        "tags": ["percussion", "pulse", "rhythm"]
    },
    {
        "partner": "Overworld",
        "name": "Chip Organism",
        "description": "Overworld's chip synthesis textures modulated by Organon bioluminescent glow cycles.",
        "coupling_type": "LFO->LFO",
        "coupling_amount": 0.6,
        "organ_overrides": {"organon_noiseColor": 0.5, "organon_membrane": 0.55},
        "tags": ["chip", "bioluminescent", "glow"]
    },
    {
        "partner": "Opal",
        "name": "Granular Organism",
        "description": "Opal grain parameters breathe with Organon's metabolic rate. Clouds are living tissue.",
        "coupling_type": "LFO->LFO",
        "coupling_amount": 0.65,
        "organ_overrides": {"organon_metabolicRate": 3.5, "organon_phasonShift": 0.3},
        "tags": ["granular", "tissue", "breath"]
    },
    {
        "partner": "Orbital",
        "name": "Orbital Metabolism",
        "description": "Orbital group envelopes (B001) paced by Organon metabolic cycles. Orbits breathe organically.",
        "coupling_type": "Envelope->Env",
        "coupling_amount": 0.7,
        "organ_overrides": {"organon_dampingCoeff": 0.45, "organon_metabolicRate": 3.0},
        "tags": ["orbital", "envelope", "organic"]
    },
    {
        "partner": "Obsidian",
        "name": "Crystal Metabolism",
        "description": "Obsidian crystal shimmer modulated by Organon enzyme state. Mineral life forms.",
        "coupling_type": "Audio->Ring",
        "coupling_amount": 0.6,
        "organ_overrides": {"organon_enzymeSelect": 650.0, "organon_isotopeBalance": 0.55},
        "tags": ["crystal", "mineral", "shimmer"]
    },
    {
        "partner": "Origami",
        "name": "Paper Organism",
        "description": "Origami fold angles respond to Organon metabolic pressure. Organic origami.",
        "coupling_type": "CV->Filter",
        "coupling_amount": 0.55,
        "organ_overrides": {"organon_membrane": 0.65, "organon_signalFlux": 0.7},
        "tags": ["fold", "paper", "pressure"]
    },
    {
        "partner": "Oracle",
        "name": "Stochastic Metabolism",
        "description": "Oracle stochastic processes seeded by Organon's metabolic state vector. Prophecy is biological.",
        "coupling_type": "Chaos->Timbre",
        "coupling_amount": 0.7,
        "organ_overrides": {"organon_noiseColor": 0.7, "organon_phasonShift": 0.5},
        "tags": ["stochastic", "prophecy", "biological"]
    },
    {
        "partner": "Obscura",
        "name": "Living Blur",
        "description": "Obscura depth of field breathes in sync with Organon metabolism. Organism sees and unfocuses.",
        "coupling_type": "Audio->AM",
        "coupling_amount": 0.55,
        "organ_overrides": {"organon_dampingCoeff": 0.5, "organon_metabolicRate": 2.8},
        "tags": ["blur", "breath", "focus"]
    },
    {
        "partner": "Oceanic",
        "name": "Deep Sea Metabolism",
        "description": "Oceanic bioluminescence (B013) driven by Organon metabolic flux. Creatures of the deep glow.",
        "coupling_type": "LFO->LFO",
        "coupling_amount": 0.7,
        "organ_overrides": {"organon_signalFlux": 0.85, "organon_membrane": 0.75},
        "tags": ["deep-sea", "bioluminescent", "B013"]
    },
    {
        "partner": "Ocelot",
        "name": "Predator Metabolism",
        "description": "Ocelot camouflage patterns timed to Organon enzyme cycles. The cat hunts by biochemistry.",
        "coupling_type": "CV->Filter",
        "coupling_amount": 0.6,
        "organ_overrides": {"organon_enzymeSelect": 550.0, "organon_catalystDrive": 1.3},
        "tags": ["predator", "camouflage", "enzyme"]
    },
    {
        "partner": "Optic",
        "name": "Metabolic Light",
        "description": "Optic visual engine powered by Organon metabolic energy output. Life becomes visible.",
        "coupling_type": "Audio->FM",
        "coupling_amount": 0.5,
        "organ_overrides": {"organon_metabolicRate": 3.5, "organon_noiseColor": 0.55},
        "tags": ["light", "visual", "energy"]
    },
    {
        "partner": "Oblique",
        "name": "Prism Organism",
        "description": "Oblique prism refraction angles determined by Organon metabolic phase. Life refracts light.",
        "coupling_type": "Audio->Ring",
        "coupling_amount": 0.65,
        "organ_overrides": {"organon_phasonShift": 0.55, "organon_isotopeBalance": 0.6},
        "tags": ["prism", "refraction", "phase"]
    },
    {
        "partner": "Osprey",
        "name": "Shore Metabolism",
        "description": "ShoreSystem coastlines (B012) shaped by Organon tidal metabolic rhythms. Life at the boundary.",
        "coupling_type": "Envelope->Env",
        "coupling_amount": 0.6,
        "organ_overrides": {"organon_metabolicRate": 2.0, "organon_membrane": 0.6},
        "tags": ["shore", "tidal", "B012"]
    },
    {
        "partner": "Ohm",
        "name": "Commune Metabolism",
        "description": "OHM COMMUNE axis amplified by Organon metabolic surplus. Collective energy becomes biological.",
        "coupling_type": "LFO->LFO",
        "coupling_amount": 0.6,
        "organ_overrides": {"organon_signalFlux": 0.7, "organon_dampingCoeff": 0.4},
        "tags": ["commune", "collective", "energy"]
    },
]

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def make_preset(name, engines, mood, macros, dna, parameters, coupling, tags, description):
    return {
        "schema_version": 1,
        "name": name,
        "mood": mood,
        "engines": engines,
        "author": "XO_OX",
        "version": "1.0.0",
        "description": description,
        "tags": tags,
        "macroLabels": list(macros.keys()),
        "macros": macros,
        "couplingIntensity": "Deep",
        "tempo": None,
        "created": "2026-03-16",
        "legacy": {"sourceInstrument": None, "sourceCategory": None, "sourcePresetName": None},
        "parameters": parameters,
        "coupling": coupling,
        "sequencer": None,
        "dna": dna,
    }


def safe_filename(name):
    for ch in '/\\:*?"<>|':
        name = name.replace(ch, "_")
    return name + ".xometa"


def write_preset(preset_dict, output_dir):
    filename = safe_filename(preset_dict["name"])
    path = os.path.join(output_dir, filename)
    if os.path.exists(path):
        print(f"  SKIP (exists): {filename}")
        return False
    with open(path, "w", encoding="utf-8") as f:
        json.dump(preset_dict, f, indent=2, ensure_ascii=False)
        f.write("\n")
    print(f"  WRITE: {filename}")
    return True


def merge(base, overrides):
    result = dict(base)
    result.update(overrides or {})
    return result


# ---------------------------------------------------------------------------
# Build marquee presets
# ---------------------------------------------------------------------------

def build_marquee_presets():
    presets = []
    for spec in MARQUEE_PRESETS:
        ouro_params = merge(OURO_PARAMS_DEFAULT, spec.get("ouro_overrides", {}))
        organ_params = merge(ORGAN_PARAMS_DEFAULT, spec.get("organ_overrides", {}))
        dna = blend_dna(OURO_DNA, ORGAN_DNA, spec.get("dna_t", 0.5))
        macros = spec["macros"]
        coupling = {
            "pairs": [{
                "engineA": "Ouroboros",
                "engineB": "Organon",
                "type": spec["coupling_type"],
                "amount": spec["coupling_amount"]
            }]
        }
        preset = make_preset(
            name=spec["name"],
            engines=["Ouroboros", "Organon"],
            mood="Entangled",
            macros=macros,
            dna=dna,
            parameters={"Ouroboros": ouro_params, "Organon": organ_params},
            coupling=coupling,
            tags=spec.get("tags", []) + ["entangled", "ouroboros-organon"],
            description=spec["description"],
        )
        presets.append(preset)
    return presets


# ---------------------------------------------------------------------------
# Build OUROBOROS pair presets
# ---------------------------------------------------------------------------

def build_ouro_pair_presets():
    presets = []
    for spec in OURO_PAIR_PRESETS:
        partner = spec["partner"]
        partner_engine_id = PARTNER_ENGINE_IDS[partner]
        ouro_params = merge(OURO_PARAMS_DEFAULT, spec.get("ouro_overrides", {}))
        partner_params = dict(PARTNER_PARAMS.get(partner, {}))

        # Blend DNA: OUROBOROS anchors, partner is unknown — use moderate blend
        dna = blend_dna(OURO_DNA, ORGAN_DNA, 0.3)  # OURO-heavy

        macros = {"CHARACTER": 0.6, "MOVEMENT": 0.75, "COUPLING": 0.65, "SPACE": 0.5}
        coupling = {
            "pairs": [{
                "engineA": "Ouroboros",
                "engineB": partner,
                "type": spec["coupling_type"],
                "amount": spec["coupling_amount"]
            }]
        }
        preset = make_preset(
            name=spec["name"],
            engines=["Ouroboros", partner],
            mood="Entangled",
            macros=macros,
            dna=dna,
            parameters={"Ouroboros": ouro_params, partner: partner_params},
            coupling=coupling,
            tags=spec.get("tags", []) + ["entangled", "ouroboros", partner_engine_id.lower()],
            description=spec["description"],
        )
        presets.append(preset)
    return presets


# ---------------------------------------------------------------------------
# Build ORGANON pair presets
# ---------------------------------------------------------------------------

def build_organ_pair_presets():
    presets = []
    for spec in ORGAN_PAIR_PRESETS:
        partner = spec["partner"]
        partner_engine_id = PARTNER_ENGINE_IDS[partner]
        organ_params = merge(ORGAN_PARAMS_DEFAULT, spec.get("organ_overrides", {}))
        partner_params = dict(PARTNER_PARAMS.get(partner, {}))

        # ORGANON-heavy DNA blend
        dna = blend_dna(OURO_DNA, ORGAN_DNA, 0.7)

        macros = {"CHARACTER": 0.5, "MOVEMENT": 0.65, "COUPLING": 0.65, "SPACE": 0.55}
        coupling = {
            "pairs": [{
                "engineA": "Organon",
                "engineB": partner,
                "type": spec["coupling_type"],
                "amount": spec["coupling_amount"]
            }]
        }
        preset = make_preset(
            name=spec["name"],
            engines=["Organon", partner],
            mood="Entangled",
            macros=macros,
            dna=dna,
            parameters={"Organon": organ_params, partner: partner_params},
            coupling=coupling,
            tags=spec.get("tags", []) + ["entangled", "organon", partner_engine_id.lower()],
            description=spec["description"],
        )
        presets.append(preset)
    return presets


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main():
    os.makedirs(ENTANGLED_DIR, exist_ok=True)

    all_presets = (
        build_marquee_presets() +
        build_ouro_pair_presets() +
        build_organ_pair_presets()
    )

    written = 0
    skipped = 0

    print(f"\nGenerating {len(all_presets)} OUROBOROS/ORGANON Entangled presets")
    print(f"Output: {ENTANGLED_DIR}\n")

    print("=== MARQUEE: OUROBOROS × ORGANON (8) ===")
    for preset in all_presets[:8]:
        if write_preset(preset, ENTANGLED_DIR):
            written += 1
        else:
            skipped += 1

    print(f"\n=== OUROBOROS PAIRS (20) ===")
    for preset in all_presets[8:28]:
        if write_preset(preset, ENTANGLED_DIR):
            written += 1
        else:
            skipped += 1

    print(f"\n=== ORGANON PAIRS (20) ===")
    for preset in all_presets[28:]:
        if write_preset(preset, ENTANGLED_DIR):
            written += 1
        else:
            skipped += 1

    print(f"\nDone. Written: {written}  Skipped (already exist): {skipped}")
    print(f"Total presets targeted: {len(all_presets)}")


if __name__ == "__main__":
    main()
