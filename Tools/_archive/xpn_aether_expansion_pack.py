#!/usr/bin/env python3
"""
xpn_aether_expansion_pack.py — Generate 60 Aether mood expansion presets.

Categories:
  - 20 Transcendent:    space 0.8-1.0, brightness 0.7-0.95, aggression 0.0-0.1
  - 20 Dark Aether:     space 0.75-1.0, brightness 0.0-0.2, density 0.3-0.6
  - 20 Shifting Aether: space 0.7-0.95, movement 0.6-0.85, warmth varies

Output: Presets/XOceanus/Aether/  (skips existing files)

Parameter prefixes are frozen per CLAUDE.md engine table — never guess prefixes.
"""

import json
import os

OUTPUT_DIR = os.path.join(
    os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
    "Presets", "XOceanus", "Aether"
)

# ---------------------------------------------------------------------------
# Engine metadata — macroLabels and canonical parameter scaffolds.
# Prefixes sourced directly from CLAUDE.md engine table (frozen, never rename).
# ---------------------------------------------------------------------------

ENGINE_META = {
    # ── Transcendent engines ─────────────────────────────────────────────
    "Opal": {
        "macroLabels": ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"],
        "params": {
            "opal_grainSize": 0.7,
            "opal_density": 0.35,
            "opal_posScatter": 0.25,
            "opal_pitchScatter": 0.03,
            "opal_filterCutoff": 0.75,
            "opal_filterReso": 0.08,
            "opal_ampAttack": 4.0,
            "opal_ampDecay": 1.5,
            "opal_ampSustain": 0.9,
            "opal_ampRelease": 5.0,
            "opal_speed": 0.4,
            "opal_reverse": 0.0,
            "opal_macroCharacter": 0.0,
            "opal_macroMovement": 0.0,
            "opal_macroCoupling": 0.0,
            "opal_macroSpace": 0.0,
        },
    },
    "Oracle": {
        "macroLabels": ["VISION", "DEPTH", "COUPLING", "SPACE"],
        "params": {
            "oracle_spectralBlur": 0.7,
            "oracle_harmonicSpread": 0.6,
            "oracle_feedbackAmt": 0.35,
            "oracle_filterCutoff": 0.72,
            "oracle_filterReso": 0.15,
            "oracle_ampAttack": 3.0,
            "oracle_ampRelease": 6.0,
            "oracle_reverbMix": 0.8,
            "oracle_delayFeedback": 0.4,
            "oracle_macroVision": 0.0,
            "oracle_macroDepth": 0.0,
            "oracle_macroCoupling": 0.0,
            "oracle_macroSpace": 0.0,
        },
    },
    "OddfeliX": {
        "macroLabels": ["TENSION", "GLITCH", "COUPLING", "SPACE"],
        "params": {
            "snap_bitDepth": 0.85,
            "snap_sampleRate": 0.9,
            "snap_filterCutoff": 0.8,
            "snap_filterReso": 0.12,
            "snap_ampAttack": 2.0,
            "snap_ampRelease": 5.5,
            "snap_reverbMix": 0.75,
            "snap_delayFeedback": 0.3,
            "snap_macroTension": 0.0,
            "snap_macroGlitch": 0.0,
            "snap_macroCoupling": 0.0,
            "snap_macroSpace": 0.0,
        },
    },
    "Obsidian": {
        "macroLabels": ["EDGE", "DARKNESS", "COUPLING", "SPACE"],
        "params": {
            "obsidian_filterCutoff": 0.78,
            "obsidian_filterReso": 0.2,
            "obsidian_oscShape": 0.5,
            "obsidian_subLevel": 0.3,
            "obsidian_ampAttack": 2.5,
            "obsidian_ampDecay": 1.0,
            "obsidian_ampSustain": 0.85,
            "obsidian_ampRelease": 5.0,
            "obsidian_reverbMix": 0.7,
            "obsidian_delayFeedback": 0.35,
            "obsidian_macroEdge": 0.0,
            "obsidian_macroDarkness": 0.0,
            "obsidian_macroCoupling": 0.0,
            "obsidian_macroSpace": 0.0,
        },
    },
    "Osprey": {
        "macroLabels": ["DIVE", "LIFT", "COUPLING", "SPACE"],
        "params": {
            "osprey_filterCutoff": 0.82,
            "osprey_filterReso": 0.1,
            "osprey_wingSpan": 0.75,
            "osprey_altitude": 0.8,
            "osprey_ampAttack": 1.5,
            "osprey_ampRelease": 6.0,
            "osprey_reverbMix": 0.8,
            "osprey_delayFeedback": 0.25,
            "osprey_macroDive": 0.0,
            "osprey_macroLift": 0.0,
            "osprey_macroCoupling": 0.0,
            "osprey_macroSpace": 0.0,
        },
    },
    "Orphica": {
        "macroLabels": ["PLUCK", "BLOOM", "COUPLING", "SPACE"],
        "params": {
            "orph_microsoundDensity": 0.4,
            "orph_pluckBrightness": 0.75,
            "orph_filterCutoff": 0.8,
            "orph_filterReso": 0.1,
            "orph_ampAttack": 0.01,
            "orph_ampDecay": 3.5,
            "orph_ampSustain": 0.3,
            "orph_ampRelease": 5.0,
            "orph_reverbMix": 0.85,
            "orph_shimmerAmt": 0.6,
            "orph_macroPluck": 0.0,
            "orph_macroBloom": 0.0,
            "orph_macroCoupling": 0.0,
            "orph_macroSpace": 0.0,
        },
    },
    "Ohm": {
        "macroLabels": ["JAM", "MEDDLING", "COMMUNE", "MEADOW"],
        "params": {
            "ohm_dadInstrument": 2,
            "ohm_dadLevel": 0.7,
            "ohm_pluckBrightness": 0.5,
            "ohm_bowPressure": 0.0,
            "ohm_bowSpeed": 0.5,
            "ohm_bodyMaterial": 3,
            "ohm_sympatheticAmt": 0.5,
            "ohm_driftRate": 0.1,
            "ohm_driftDepth": 0.8,
            "ohm_damping": 0.99,
            "ohm_spectralFreeze": 0.6,
            "ohm_grainSize": 0.9,
            "ohm_grainDensity": 0.5,
            "ohm_grainScatter": 0.3,
            "ohm_reverbMix": 0.7,
            "ohm_delayFeedback": 0.3,
            "ohm_macroJam": 0.0,
            "ohm_macroMeddling": 0.0,
            "ohm_macroCommune": 0.0,
            "ohm_macroMeadow": 0.0,
        },
    },
    "OddOscar": {
        "macroLabels": ["TENSION", "GLITCH", "COUPLING", "SPACE"],
        "params": {
            "morph_bitDepth": 0.9,
            "morph_sampleRate": 0.85,
            "morph_filterCutoff": 0.7,
            "morph_filterReso": 0.1,
            "morph_ampAttack": 3.0,
            "morph_ampRelease": 6.0,
            "morph_reverbMix": 0.8,
            "morph_delayFeedback": 0.35,
            "morph_macroTension": 0.0,
            "morph_macroGlitch": 0.0,
            "morph_macroCoupling": 0.0,
            "morph_macroSpace": 0.0,
        },
    },
    # ── Dark Aether engines ───────────────────────────────────────────────
    "Ombre": {
        "macroLabels": ["GRAIN", "SHADE", "COUPLING", "SPACE"],
        "params": {
            "ombre_grainSize": 0.85,
            "ombre_grainDensity": 0.4,
            "ombre_filterCutoff": 0.2,
            "ombre_filterReso": 0.25,
            "ombre_ampAttack": 5.0,
            "ombre_ampRelease": 8.0,
            "ombre_reverbMix": 0.9,
            "ombre_delayFeedback": 0.5,
            "ombre_macroGrain": 0.0,
            "ombre_macroShade": 0.0,
            "ombre_macroCoupling": 0.0,
            "ombre_macroSpace": 0.0,
        },
    },
    "Overdub": {
        "macroLabels": ["DRIVE", "ECHO", "XOSEND", "SPACE"],
        "params": {
            "dub_driveAmt": 0.05,
            "dub_tapeSpeed": 0.5,
            "dub_tapeWow": 0.3,
            "dub_tapeFeedback": 0.55,
            "dub_springMix": 0.7,
            "dub_filterCutoff": 0.15,
            "dub_filterReso": 0.2,
            "dub_reverbSize": 0.95,
            "dub_reverbMix": 0.85,
            "dub_macroDrive": 0.0,
            "dub_macroEcho": 0.0,
            "dub_macroXosend": 0.0,
            "dub_macroSpace": 0.0,
        },
    },
    "Organon": {
        "macroLabels": ["BREATH", "SWELL", "COUPLING", "SPACE"],
        "params": {
            "organon_drawbar1": 0.6,
            "organon_drawbar2": 0.3,
            "organon_drawbar3": 0.5,
            "organon_rotarySpeed": 0.1,
            "organon_filterCutoff": 0.18,
            "organon_filterReso": 0.15,
            "organon_ampAttack": 4.0,
            "organon_ampRelease": 7.0,
            "organon_reverbMix": 0.88,
            "organon_macroBreathe": 0.0,
            "organon_macroSwell": 0.0,
            "organon_macroCoupling": 0.0,
            "organon_macroSpace": 0.0,
        },
    },
    "Odyssey": {
        "macroLabels": ["JOURNEY", "HORIZON", "COUPLING", "SPACE"],
        "params": {
            "drift_oscA_mode": 0,
            "drift_filterCutoff": 0.12,
            "drift_filterReso": 0.2,
            "drift_lfoRate": 0.05,
            "drift_lfoDepth": 0.4,
            "drift_ampAttack": 5.0,
            "drift_ampDecay": 2.0,
            "drift_ampSustain": 0.8,
            "drift_ampRelease": 8.0,
            "drift_reverbMix": 0.9,
            "drift_delayFeedback": 0.5,
            "drift_macroJourney": 0.0,
            "drift_macroHorizon": 0.0,
            "drift_macroCoupling": 0.0,
            "drift_macroSpace": 0.0,
        },
    },
    "Oceanic": {
        "macroLabels": ["DEPTH", "CURRENT", "COUPLING", "SPACE"],
        "params": {
            "ocean_filterCutoff": 0.1,
            "ocean_filterReso": 0.3,
            "ocean_separation": 0.7,
            "ocean_wavefolderAmt": 0.2,
            "ocean_ampAttack": 6.0,
            "ocean_ampRelease": 9.0,
            "ocean_reverbMix": 0.92,
            "ocean_delayFeedback": 0.55,
            "ocean_macroDepth": 0.0,
            "ocean_macroCurrent": 0.0,
            "ocean_macroCoupling": 0.0,
            "ocean_macroSpace": 0.0,
        },
    },
    "Owlfish": {
        "macroLabels": ["SCREECH", "HOLLOW", "COUPLING", "SPACE"],
        "params": {
            "owl_filterCutoff": 0.08,
            "owl_filterReso": 0.4,
            "owl_formantShift": 0.3,
            "owl_hollowAmt": 0.7,
            "owl_ampAttack": 4.0,
            "owl_ampRelease": 7.5,
            "owl_reverbMix": 0.88,
            "owl_macroScreech": 0.0,
            "owl_macroHollow": 0.0,
            "owl_macroCoupling": 0.0,
            "owl_macroSpace": 0.0,
        },
    },
    # ── Shifting Aether engines ───────────────────────────────────────────
    "Optic": {
        "macroLabels": ["REFRACTION", "SCATTER", "COUPLING", "SPACE"],
        "params": {
            "optic_pulseRate": 0.3,
            "optic_prismAmt": 0.7,
            "optic_scatterWidth": 0.65,
            "optic_filterCutoff": 0.6,
            "optic_filterReso": 0.15,
            "optic_lfoRate": 0.3,
            "optic_lfoDepth": 0.5,
            "optic_ampAttack": 1.0,
            "optic_ampRelease": 5.0,
            "optic_reverbMix": 0.75,
            "optic_macroRefraction": 0.0,
            "optic_macroScatter": 0.0,
            "optic_macroCoupling": 0.0,
            "optic_macroSpace": 0.0,
        },
    },
    "Oblique": {
        "macroLabels": ["ANGLE", "PHASE", "COUPLING", "SPACE"],
        "params": {
            "oblq_prismColor": 0.5,
            "oblq_phaseShift": 0.6,
            "oblq_filterCutoff": 0.55,
            "oblq_filterReso": 0.18,
            "oblq_lfoRate": 0.25,
            "oblq_lfoDepth": 0.55,
            "oblq_ampAttack": 0.5,
            "oblq_ampRelease": 4.5,
            "oblq_reverbMix": 0.72,
            "oblq_delayFeedback": 0.4,
            "oblq_macroAngle": 0.0,
            "oblq_macroPhase": 0.0,
            "oblq_macroCoupling": 0.0,
            "oblq_macroSpace": 0.0,
        },
    },
    "Origami": {
        "macroLabels": ["FOLD", "CREASE", "COUPLING", "SPACE"],
        "params": {
            "origami_foldPoint": 0.5,
            "origami_foldAmt": 0.6,
            "origami_creaseDepth": 0.45,
            "origami_filterCutoff": 0.5,
            "origami_filterReso": 0.12,
            "origami_lfoRate": 0.2,
            "origami_lfoDepth": 0.5,
            "origami_ampAttack": 1.0,
            "origami_ampRelease": 5.5,
            "origami_reverbMix": 0.78,
            "origami_macroFold": 0.0,
            "origami_macroCrease": 0.0,
            "origami_macroCoupling": 0.0,
            "origami_macroSpace": 0.0,
        },
    },
    "Octopus": {
        "macroLabels": ["STRETCH", "INK", "COUPLING", "SPACE"],
        "params": {
            "octo_armDepth": 0.5,
            "octo_stretchAmt": 0.7,
            "octo_inkDepth": 0.4,
            "octo_filterCutoff": 0.5,
            "octo_filterReso": 0.2,
            "octo_lfoRate": 0.35,
            "octo_lfoDepth": 0.6,
            "octo_ampAttack": 0.3,
            "octo_ampRelease": 4.0,
            "octo_reverbMix": 0.7,
            "octo_macroStretch": 0.0,
            "octo_macroInk": 0.0,
            "octo_macroCoupling": 0.0,
            "octo_macroSpace": 0.0,
        },
    },
    "Ouroboros": {
        "macroLabels": ["CYCLE", "DEVOUR", "COUPLING", "SPACE"],
        "params": {
            "ouro_topology": 0.5,
            "ouro_loopLength": 0.5,
            "ouro_feedbackAmt": 0.65,
            "ouro_filterCutoff": 0.55,
            "ouro_filterReso": 0.22,
            "ouro_lfoRate": 0.4,
            "ouro_lfoDepth": 0.65,
            "ouro_ampAttack": 0.5,
            "ouro_ampRelease": 5.0,
            "ouro_reverbMix": 0.8,
            "ouro_macroCycle": 0.0,
            "ouro_macroDevour": 0.0,
            "ouro_macroCoupling": 0.0,
            "ouro_macroSpace": 0.0,
        },
    },
    "Overworld": {
        "macroLabels": ["ERA", "CHIP", "COUPLING", "SPACE"],
        "params": {
            "ow_era": 0.5,
            "ow_eraBlend": 0.5,
            "ow_filterCutoff": 0.6,
            "ow_filterReso": 0.1,
            "ow_lfoRate": 0.3,
            "ow_lfoDepth": 0.5,
            "ow_ampAttack": 0.2,
            "ow_ampRelease": 4.0,
            "ow_reverbMix": 0.65,
            "ow_glitchAmt": 0.3,
            "ow_macroEra": 0.0,
            "ow_macroChip": 0.0,
            "ow_macroCoupling": 0.0,
            "ow_macroSpace": 0.0,
        },
    },
    "Onset": {
        "macroLabels": ["MACHINE", "PUNCH", "SPACE", "MUTATE"],
        "params": {
            "perc_noiseLevel": 0.4,
            "perc_kickTune": 0.5,
            "perc_snareTone": 0.4,
            "perc_hihatDecay": 0.3,
            "perc_reverbMix": 0.75,
            "perc_delayFeedback": 0.4,
            "perc_filterCutoff": 0.55,
            "perc_macroMachine": 0.0,
            "perc_macroPunch": 0.0,
            "perc_macroSpace": 0.0,
            "perc_macroMutate": 0.0,
        },
    },
    "Ole": {
        "macroLabels": ["DRAMA", "FIRE", "COUPLING", "SPACE"],
        "params": {
            "ole_voiceBlend": 0.6,
            "ole_rhythmAmt": 0.5,
            "ole_filterCutoff": 0.6,
            "ole_filterReso": 0.12,
            "ole_lfoRate": 0.35,
            "ole_lfoDepth": 0.55,
            "ole_ampAttack": 0.3,
            "ole_ampRelease": 4.5,
            "ole_reverbMix": 0.7,
            "ole_macroDrama": 0.0,
            "ole_macroFire": 0.0,
            "ole_macroCoupling": 0.0,
            "ole_macroSpace": 0.0,
        },
    },
}

# ---------------------------------------------------------------------------
# Preset definitions
# dna keys: brightness, warmth, movement, density, space, aggression
# macros keys must match the engine's macroLabels (case-insensitive key matching)
# param_overrides: merged on top of engine base params
# ---------------------------------------------------------------------------

PRESETS = [
    # =====================================================================
    # TRANSCENDENT (20)
    # Engines: OPAL, ORACLE, ODDFELIX, OBSIDIAN, OSPREY, ORPHICA, OHM, ODDOSCAR
    # space 0.8-1.0, brightness 0.7-0.95, aggression 0.0-0.1
    # =====================================================================
    {
        "name": "Ether Gate",
        "engine": "Opal",
        "subcategory": "Transcendent",
        "dna": {"brightness": 0.88, "warmth": 0.3, "movement": 0.15, "density": 0.2, "space": 0.97, "aggression": 0.02},
        "description": "Grains dissolve at the threshold between silence and sound. The gate opens slowly — nine seconds of nothing, then pure crystalline shimmer that vanishes before you can name it.",
        "tags": ["aether", "transcendent", "granular", "vast", "crystalline"],
        "param_overrides": {"opal_grainSize": 0.9, "opal_density": 0.15, "opal_posScatter": 0.35, "opal_pitchScatter": 0.02, "opal_filterCutoff": 0.88, "opal_ampAttack": 9.0, "opal_ampRelease": 8.0, "opal_speed": 0.2},
    },
    {
        "name": "Above All Weather",
        "engine": "Oracle",
        "subcategory": "Transcendent",
        "dna": {"brightness": 0.82, "warmth": 0.25, "movement": 0.2, "density": 0.25, "space": 0.95, "aggression": 0.03},
        "description": "Spectral harmonics hover where no weather reaches. The oracle sees only light — refracted, diffused, infinite.",
        "tags": ["aether", "transcendent", "spectral", "elevated", "spacious"],
        "param_overrides": {"oracle_spectralBlur": 0.85, "oracle_harmonicSpread": 0.75, "oracle_filterCutoff": 0.85, "oracle_reverbMix": 0.92, "oracle_ampAttack": 4.5, "oracle_ampRelease": 8.0},
    },
    {
        "name": "Crystalline Departure",
        "engine": "OddfeliX",
        "subcategory": "Transcendent",
        "dna": {"brightness": 0.91, "warmth": 0.15, "movement": 0.1, "density": 0.18, "space": 0.98, "aggression": 0.0},
        "description": "The digital artifact becomes transcendence. Bit reduction so extreme it strips material reality — what remains is pure light frequency.",
        "tags": ["aether", "transcendent", "digital", "pure", "crystalline"],
        "param_overrides": {"snap_bitDepth": 0.95, "snap_filterCutoff": 0.92, "snap_reverbMix": 0.95, "snap_ampAttack": 3.0, "snap_ampRelease": 7.0},
    },
    {
        "name": "Glass Heaven",
        "engine": "Obsidian",
        "subcategory": "Transcendent",
        "dna": {"brightness": 0.87, "warmth": 0.2, "movement": 0.12, "density": 0.22, "space": 0.93, "aggression": 0.01},
        "description": "Obsidian ground so fine it becomes translucent. The volcanic glass catches only the highest frequencies — a cathedral of cooled light.",
        "tags": ["aether", "transcendent", "glass", "refined", "celestial"],
        "param_overrides": {"obsidian_filterCutoff": 0.9, "obsidian_reverbMix": 0.88, "obsidian_ampAttack": 3.5, "obsidian_ampRelease": 7.0},
    },
    {
        "name": "Thermal Column",
        "engine": "Osprey",
        "subcategory": "Transcendent",
        "dna": {"brightness": 0.85, "warmth": 0.35, "movement": 0.25, "density": 0.2, "space": 0.92, "aggression": 0.04},
        "description": "The osprey finds the invisible column of rising heat and circles without effort — altitude gained through absolute stillness of wing.",
        "tags": ["aether", "transcendent", "elevation", "soaring", "natural"],
        "param_overrides": {"osprey_altitude": 0.95, "osprey_reverbMix": 0.88, "osprey_ampAttack": 2.0, "osprey_ampRelease": 7.5},
    },
    {
        "name": "Siphonophore Light",
        "engine": "Orphica",
        "subcategory": "Transcendent",
        "dna": {"brightness": 0.9, "warmth": 0.2, "movement": 0.18, "density": 0.15, "space": 0.96, "aggression": 0.0},
        "description": "The microsound harp plays only the overtones of silence. Each pluck is a bioluminescent pulse — brief, cold, perfect.",
        "tags": ["aether", "transcendent", "microsound", "bioluminescent", "delicate"],
        "param_overrides": {"orph_microsoundDensity": 0.2, "orph_pluckBrightness": 0.9, "orph_filterCutoff": 0.92, "orph_shimmerAmt": 0.85, "orph_reverbMix": 0.93, "orph_ampDecay": 5.0, "orph_ampRelease": 7.0},
    },
    {
        "name": "Meadow At Elevation",
        "engine": "Ohm",
        "subcategory": "Transcendent",
        "dna": {"brightness": 0.78, "warmth": 0.5, "movement": 0.22, "density": 0.28, "space": 0.91, "aggression": 0.02},
        "description": "The dad's jam at 4,000 meters. In-laws cannot follow. The theremin traces mountain air with no gravity in its signal path.",
        "tags": ["aether", "transcendent", "folk", "elevation", "pastoral"],
        "param_overrides": {"ohm_spectralFreeze": 0.75, "ohm_grainSize": 0.95, "ohm_grainDensity": 0.3, "ohm_reverbMix": 0.82, "ohm_sympatheticAmt": 0.7},
    },
    {
        "name": "Metamorphosis Final",
        "engine": "OddOscar",
        "subcategory": "Transcendent",
        "dna": {"brightness": 0.8, "warmth": 0.28, "movement": 0.15, "density": 0.2, "space": 0.94, "aggression": 0.0},
        "description": "The final metamorphosis — the organism has transcended its glitch origins. What remains is pure signal, unrecognizable from its source.",
        "tags": ["aether", "transcendent", "digital", "evolved", "pure"],
        "param_overrides": {"morph_filterCutoff": 0.85, "morph_reverbMix": 0.9, "morph_ampAttack": 4.0, "morph_ampRelease": 7.5},
    },
    {
        "name": "Void Bloom",
        "engine": "Opal",
        "subcategory": "Transcendent",
        "dna": {"brightness": 0.76, "warmth": 0.4, "movement": 0.12, "density": 0.18, "space": 0.99, "aggression": 0.01},
        "description": "A flower that blooms in vacuum. No air carries its perfume — the grain exists only as memory of itself. Maximum space, minimum presence.",
        "tags": ["aether", "transcendent", "granular", "void", "sparse"],
        "param_overrides": {"opal_grainSize": 0.95, "opal_density": 0.08, "opal_posScatter": 0.5, "opal_filterCutoff": 0.72, "opal_ampAttack": 8.0, "opal_ampRelease": 10.0},
    },
    {
        "name": "Spectral Ascent",
        "engine": "Oracle",
        "subcategory": "Transcendent",
        "dna": {"brightness": 0.93, "warmth": 0.1, "movement": 0.3, "density": 0.22, "space": 0.9, "aggression": 0.05},
        "description": "The oracle's vision climbs frequencies that no ear can follow. The ascent continues past the limit of perception — you feel the destination rather than hear it.",
        "tags": ["aether", "transcendent", "spectral", "ascending", "visionary"],
        "param_overrides": {"oracle_spectralBlur": 0.9, "oracle_harmonicSpread": 0.85, "oracle_filterCutoff": 0.95, "oracle_reverbMix": 0.88, "oracle_ampAttack": 2.0, "oracle_ampRelease": 6.0},
    },
    {
        "name": "Frozen Altitude",
        "engine": "Osprey",
        "subcategory": "Transcendent",
        "dna": {"brightness": 0.7, "warmth": 0.15, "movement": 0.08, "density": 0.15, "space": 0.98, "aggression": 0.0},
        "description": "Ice crystals form on the wings at maximum altitude. The osprey hangs motionless in the blue-black sky above the clouds.",
        "tags": ["aether", "transcendent", "frozen", "altitude", "stillness"],
        "param_overrides": {"osprey_altitude": 0.98, "osprey_filterCutoff": 0.75, "osprey_reverbMix": 0.95, "osprey_ampAttack": 5.0, "osprey_ampRelease": 9.0},
    },
    {
        "name": "Harp Of Stars",
        "engine": "Orphica",
        "subcategory": "Transcendent",
        "dna": {"brightness": 0.95, "warmth": 0.15, "movement": 0.25, "density": 0.2, "space": 0.93, "aggression": 0.0},
        "description": "The siphonophore's strings are stellar filaments. Each pluck travels light-years. The resonance outlasts the star that created it.",
        "tags": ["aether", "transcendent", "stellar", "harp", "cosmic"],
        "param_overrides": {"orph_pluckBrightness": 0.95, "orph_filterCutoff": 0.96, "orph_shimmerAmt": 0.9, "orph_reverbMix": 0.9, "orph_ampDecay": 6.0, "orph_ampRelease": 8.0},
    },
    {
        "name": "Pure Elevation",
        "engine": "Obsidian",
        "subcategory": "Transcendent",
        "dna": {"brightness": 0.75, "warmth": 0.3, "movement": 0.2, "density": 0.25, "space": 0.88, "aggression": 0.04},
        "description": "Obsidian freed of darkness. The volcanic glass becomes a window — everything that passed through it left its high frequencies behind.",
        "tags": ["aether", "transcendent", "elevated", "refined", "volcanic"],
        "param_overrides": {"obsidian_filterCutoff": 0.82, "obsidian_reverbMix": 0.82, "obsidian_ampAttack": 2.0, "obsidian_ampRelease": 6.0},
    },
    {
        "name": "Dad Cosmic Threshold",
        "engine": "Ohm",
        "subcategory": "Transcendent",
        "dna": {"brightness": 0.82, "warmth": 0.4, "movement": 0.2, "density": 0.25, "space": 0.95, "aggression": 0.01},
        "description": "The commune has reached maximum absorption. The dad's spectral freeze holds the last overtone of every instrument simultaneously. No more notes — only resonance.",
        "tags": ["aether", "transcendent", "communion", "spectral", "frozen"],
        "param_overrides": {"ohm_spectralFreeze": 0.95, "ohm_grainSize": 1.0, "ohm_grainDensity": 0.15, "ohm_reverbMix": 0.9, "ohm_sympatheticAmt": 0.9},
    },
    {
        "name": "Prism Dissolution",
        "engine": "OddfeliX",
        "subcategory": "Transcendent",
        "dna": {"brightness": 0.94, "warmth": 0.12, "movement": 0.18, "density": 0.16, "space": 0.97, "aggression": 0.0},
        "description": "The glitch dissolves completely. feliX stripped of all character — only the highest partials remain, scattered like light through a prism that is itself dissolving.",
        "tags": ["aether", "transcendent", "prism", "dissolved", "pure"],
        "param_overrides": {"snap_bitDepth": 0.98, "snap_sampleRate": 0.98, "snap_filterCutoff": 0.94, "snap_reverbMix": 0.94, "snap_ampAttack": 4.0, "snap_ampRelease": 8.0},
    },
    {
        "name": "Scatter Into Light",
        "engine": "Opal",
        "subcategory": "Transcendent",
        "dna": {"brightness": 0.86, "warmth": 0.22, "movement": 0.35, "density": 0.2, "space": 0.91, "aggression": 0.02},
        "description": "Maximum position scatter — the grains have no assigned location. They drift through the source material like photons through atmosphere, emerging as diffuse luminosity.",
        "tags": ["aether", "transcendent", "scatter", "diffuse", "luminous"],
        "param_overrides": {"opal_grainSize": 0.6, "opal_density": 0.22, "opal_posScatter": 0.95, "opal_pitchScatter": 0.0, "opal_filterCutoff": 0.88, "opal_ampAttack": 3.0, "opal_ampRelease": 6.0},
    },
    {
        "name": "The Final Overtone",
        "engine": "Orphica",
        "subcategory": "Transcendent",
        "dna": {"brightness": 0.92, "warmth": 0.18, "movement": 0.1, "density": 0.12, "space": 0.98, "aggression": 0.0},
        "description": "The series continues past the audible into the felt. What blooms here is the overtone that closes the harmonic series — infinite frequency, zero amplitude.",
        "tags": ["aether", "transcendent", "overtones", "infinite", "harmonic"],
        "param_overrides": {"orph_microsoundDensity": 0.1, "orph_pluckBrightness": 1.0, "orph_filterCutoff": 0.98, "orph_shimmerAmt": 1.0, "orph_reverbMix": 0.96, "orph_ampDecay": 8.0, "orph_ampRelease": 10.0},
    },
    {
        "name": "Oscar Unbound",
        "engine": "OddOscar",
        "subcategory": "Transcendent",
        "dna": {"brightness": 0.88, "warmth": 0.2, "movement": 0.2, "density": 0.18, "space": 0.96, "aggression": 0.0},
        "description": "Oscar at full reduction — every rough edge smoothed by transcendence. The sawtooth remembers it was once a sine wave.",
        "tags": ["aether", "transcendent", "evolved", "pure", "cosmic"],
        "param_overrides": {"morph_bitDepth": 0.95, "morph_filterCutoff": 0.88, "morph_reverbMix": 0.92, "morph_ampAttack": 5.0, "morph_ampRelease": 8.0},
    },
    {
        "name": "Osprey Above Clouds",
        "engine": "Osprey",
        "subcategory": "Transcendent",
        "dna": {"brightness": 0.79, "warmth": 0.38, "movement": 0.3, "density": 0.22, "space": 0.87, "aggression": 0.06},
        "description": "The cloud layer is now below. Up here the sky darkens toward blue-black and the osprey's silhouette is visible against the void.",
        "tags": ["aether", "transcendent", "elevation", "above", "vast"],
        "param_overrides": {"osprey_altitude": 0.88, "osprey_filterCutoff": 0.78, "osprey_reverbMix": 0.85, "osprey_ampAttack": 1.5, "osprey_ampRelease": 6.5},
    },
    {
        "name": "Oracle Beyond Vision",
        "engine": "Oracle",
        "subcategory": "Transcendent",
        "dna": {"brightness": 0.77, "warmth": 0.3, "movement": 0.15, "density": 0.28, "space": 0.85, "aggression": 0.07},
        "description": "Vision expanded past the frequency of sight. The oracle's harmonics map a spectrum no instrument was designed to reach.",
        "tags": ["aether", "transcendent", "beyond", "spectral", "oracle"],
        "param_overrides": {"oracle_spectralBlur": 0.75, "oracle_harmonicSpread": 0.7, "oracle_filterCutoff": 0.8, "oracle_reverbMix": 0.83, "oracle_ampAttack": 3.5, "oracle_ampRelease": 7.0},
    },

    # =====================================================================
    # DARK AETHER (20)
    # Engines: OMBRE, OVERDUB, OBSIDIAN, ORACLE, ORGANON, ODYSSEY, OCEANIC, OWLFISH
    # space 0.75-1.0, brightness 0.0-0.2, density 0.3-0.6
    # =====================================================================
    {
        "name": "Abyssal Canopy",
        "engine": "Ombre",
        "subcategory": "Dark Aether",
        "dna": {"brightness": 0.08, "warmth": 0.3, "movement": 0.1, "density": 0.4, "space": 0.92, "aggression": 0.02},
        "description": "The grain cloud forms a ceiling above the abyss. No light passes through. The canopy absorbs all spectra — what emerges below is pure shadow in enormous space.",
        "tags": ["aether", "dark-aether", "grain", "abyssal", "vast", "dark"],
        "param_overrides": {"ombre_filterCutoff": 0.05, "ombre_filterReso": 0.3, "ombre_grainDensity": 0.45, "ombre_reverbMix": 0.93, "ombre_ampAttack": 7.0, "ombre_ampRelease": 9.0},
    },
    {
        "name": "Tape At Event Horizon",
        "engine": "Overdub",
        "subcategory": "Dark Aether",
        "dna": {"brightness": 0.06, "warmth": 0.5, "movement": 0.12, "density": 0.45, "space": 0.95, "aggression": 0.03},
        "description": "Tape delay feedback approaching the event horizon. Time dilates. The echo stretches into thermal noise across vast reverberant space.",
        "tags": ["aether", "dark-aether", "tape", "feedback", "vast", "dark"],
        "param_overrides": {"dub_tapeSpeed": 0.08, "dub_tapeWow": 0.6, "dub_tapeFeedback": 0.88, "dub_filterCutoff": 0.06, "dub_reverbMix": 0.92, "dub_reverbSize": 0.99},
    },
    {
        "name": "Void Glass",
        "engine": "Obsidian",
        "subcategory": "Dark Aether",
        "dna": {"brightness": 0.05, "warmth": 0.15, "movement": 0.08, "density": 0.35, "space": 0.97, "aggression": 0.01},
        "description": "Obsidian that reflects no light. The surface is so dark it functions as a window into the void — enormous reverberant space, no treble, no warmth.",
        "tags": ["aether", "dark-aether", "obsidian", "void", "vast", "dark"],
        "param_overrides": {"obsidian_filterCutoff": 0.04, "obsidian_reverbMix": 0.94, "obsidian_ampAttack": 5.0, "obsidian_ampRelease": 9.0},
    },
    {
        "name": "Dark Oracle",
        "engine": "Oracle",
        "subcategory": "Dark Aether",
        "dna": {"brightness": 0.1, "warmth": 0.2, "movement": 0.18, "density": 0.42, "space": 0.9, "aggression": 0.05},
        "description": "The oracle sees only darkness and interprets it as prophecy. Spectral analysis of the void. Depth that cannot be measured.",
        "tags": ["aether", "dark-aether", "oracle", "prophetic", "deep", "vast"],
        "param_overrides": {"oracle_spectralBlur": 0.9, "oracle_filterCutoff": 0.08, "oracle_reverbMix": 0.9, "oracle_ampAttack": 5.0, "oracle_ampRelease": 9.0},
    },
    {
        "name": "Cathedral Of Night",
        "engine": "Organon",
        "subcategory": "Dark Aether",
        "dna": {"brightness": 0.07, "warmth": 0.45, "movement": 0.08, "density": 0.5, "space": 0.93, "aggression": 0.01},
        "description": "The organ breathes at minimum speed — a single sustained breath filling a stone cathedral at midnight. Bass registers only, stretching the dark to its limits.",
        "tags": ["aether", "dark-aether", "organ", "cathedral", "nocturnal", "vast"],
        "param_overrides": {"organon_drawbar1": 0.9, "organon_drawbar2": 0.7, "organon_drawbar3": 0.1, "organon_rotarySpeed": 0.02, "organon_filterCutoff": 0.05, "organon_reverbMix": 0.95, "organon_ampAttack": 6.0, "organon_ampRelease": 8.0},
    },
    {
        "name": "Frozen Crossing",
        "engine": "Odyssey",
        "subcategory": "Dark Aether",
        "dna": {"brightness": 0.12, "warmth": 0.2, "movement": 0.07, "density": 0.38, "space": 0.96, "aggression": 0.02},
        "description": "The journey suspended at absolute zero. The odyssey continues but no motion is visible — only the vast dark between destinations.",
        "tags": ["aether", "dark-aether", "frozen", "journey", "vast", "dark"],
        "param_overrides": {"drift_filterCutoff": 0.08, "drift_lfoRate": 0.02, "drift_lfoDepth": 0.2, "drift_reverbMix": 0.94, "drift_ampAttack": 7.0, "drift_ampRelease": 10.0},
    },
    {
        "name": "Hadal Zone Signal",
        "engine": "Oceanic",
        "subcategory": "Dark Aether",
        "dna": {"brightness": 0.04, "warmth": 0.25, "movement": 0.15, "density": 0.55, "space": 0.88, "aggression": 0.04},
        "description": "Signal from the deepest trench. The water column above is eleven kilometers of weight. Only the lowest frequencies survive the journey — everything else was crushed ascending.",
        "tags": ["aether", "dark-aether", "hadal", "deep", "oceanic", "vast"],
        "param_overrides": {"ocean_filterCutoff": 0.03, "ocean_wavefolderAmt": 0.15, "ocean_reverbMix": 0.9, "ocean_ampAttack": 6.0, "ocean_ampRelease": 10.0},
    },
    {
        "name": "Owlfish Hollow",
        "engine": "Owlfish",
        "subcategory": "Dark Aether",
        "dna": {"brightness": 0.09, "warmth": 0.35, "movement": 0.12, "density": 0.4, "space": 0.91, "aggression": 0.03},
        "description": "The owlfish has found the hollow place in the ocean where sound becomes architecture. It inhabits the formant completely — a living resonator in the dark.",
        "tags": ["aether", "dark-aether", "hollow", "formant", "deep", "dark"],
        "param_overrides": {"owl_filterCutoff": 0.06, "owl_hollowAmt": 0.9, "owl_reverbMix": 0.9, "owl_ampAttack": 5.0, "owl_ampRelease": 8.5},
    },
    {
        "name": "Shadow Grain",
        "engine": "Ombre",
        "subcategory": "Dark Aether",
        "dna": {"brightness": 0.15, "warmth": 0.4, "movement": 0.2, "density": 0.55, "space": 0.85, "aggression": 0.05},
        "description": "Dense grain cloud with all brightness removed. The shadow has texture — you can feel its weight against the enormous empty space behind it.",
        "tags": ["aether", "dark-aether", "grain", "shadow", "dense", "vast"],
        "param_overrides": {"ombre_grainDensity": 0.6, "ombre_filterCutoff": 0.12, "ombre_reverbMix": 0.88, "ombre_ampAttack": 4.0, "ombre_ampRelease": 7.0},
    },
    {
        "name": "Spring In Void",
        "engine": "Overdub",
        "subcategory": "Dark Aether",
        "dna": {"brightness": 0.18, "warmth": 0.4, "movement": 0.25, "density": 0.48, "space": 0.93, "aggression": 0.04},
        "description": "Spring reverb coil submerged in deep darkness. The metallic resonance stripped of attack — only the ring of the coil in empty space.",
        "tags": ["aether", "dark-aether", "spring", "reverb", "void", "metallic"],
        "param_overrides": {"dub_springMix": 0.95, "dub_filterCutoff": 0.15, "dub_reverbMix": 0.9, "dub_tapeFeedback": 0.45, "dub_ampAttack": 3.0, "dub_ampRelease": 8.0},
    },
    {
        "name": "Organ Night Mass",
        "engine": "Organon",
        "subcategory": "Dark Aether",
        "dna": {"brightness": 0.13, "warmth": 0.55, "movement": 0.15, "density": 0.45, "space": 0.9, "aggression": 0.02},
        "description": "The night mass requires no congregation. The organ plays for stone walls and vaulted darkness. The rotary barely turns — a held breath of sacred bass.",
        "tags": ["aether", "dark-aether", "organ", "mass", "sacred", "vast"],
        "param_overrides": {"organon_drawbar1": 0.85, "organon_drawbar2": 0.5, "organon_rotarySpeed": 0.05, "organon_filterCutoff": 0.12, "organon_reverbMix": 0.92, "organon_ampAttack": 5.0, "organon_ampRelease": 9.0},
    },
    {
        "name": "Sunless Horizon",
        "engine": "Odyssey",
        "subcategory": "Dark Aether",
        "dna": {"brightness": 0.19, "warmth": 0.3, "movement": 0.22, "density": 0.35, "space": 0.94, "aggression": 0.03},
        "description": "The horizon exists but holds no sunrise. The odyssey navigates by dark stars — direction without light, purpose without warmth.",
        "tags": ["aether", "dark-aether", "sunless", "navigation", "vast", "journey"],
        "param_overrides": {"drift_filterCutoff": 0.16, "drift_lfoRate": 0.04, "drift_lfoDepth": 0.3, "drift_reverbMix": 0.92, "drift_ampAttack": 5.0, "drift_ampRelease": 9.0},
    },
    {
        "name": "Deep Current Dark",
        "engine": "Oceanic",
        "subcategory": "Dark Aether",
        "dna": {"brightness": 0.1, "warmth": 0.35, "movement": 0.3, "density": 0.5, "space": 0.82, "aggression": 0.06},
        "description": "Thermohaline circulation moving in absolute darkness. The current is immense and slow — a river that never surfaces, never illuminates.",
        "tags": ["aether", "dark-aether", "thermohaline", "current", "deep", "moving"],
        "param_overrides": {"ocean_filterCutoff": 0.07, "ocean_separation": 0.8, "ocean_reverbMix": 0.85, "ocean_delayFeedback": 0.6, "ocean_ampAttack": 4.0, "ocean_ampRelease": 8.0},
    },
    {
        "name": "Obsidian Waters",
        "engine": "Obsidian",
        "subcategory": "Dark Aether",
        "dna": {"brightness": 0.17, "warmth": 0.3, "movement": 0.18, "density": 0.42, "space": 0.86, "aggression": 0.04},
        "description": "The volcanic lake. Black water over black stone. A stillness that contains enormous potential — the obsidian surface holds reflections of stars that no longer exist.",
        "tags": ["aether", "dark-aether", "obsidian", "lake", "volcanic", "reflective"],
        "param_overrides": {"obsidian_filterCutoff": 0.14, "obsidian_reverbMix": 0.87, "obsidian_ampAttack": 4.0, "obsidian_ampRelease": 7.5},
    },
    {
        "name": "Oracle Darkness Speaks",
        "engine": "Oracle",
        "subcategory": "Dark Aether",
        "dna": {"brightness": 0.08, "warmth": 0.28, "movement": 0.25, "density": 0.48, "space": 0.87, "aggression": 0.06},
        "description": "The prophecy arrives without light. The oracle's spectral voice carries no treble — only the fundamental truth, dense and dark, from the deepest octave.",
        "tags": ["aether", "dark-aether", "oracle", "prophecy", "dark", "deep"],
        "param_overrides": {"oracle_spectralBlur": 0.8, "oracle_filterCutoff": 0.06, "oracle_reverbMix": 0.88, "oracle_ampAttack": 4.0, "oracle_ampRelease": 8.5},
    },
    {
        "name": "Owlfish Trench",
        "engine": "Owlfish",
        "subcategory": "Dark Aether",
        "dna": {"brightness": 0.16, "warmth": 0.42, "movement": 0.2, "density": 0.55, "space": 0.78, "aggression": 0.07},
        "description": "The owlfish descends into the trench where the resonance is structural — the water column itself is the instrument. Dense dark hollow with no reflection.",
        "tags": ["aether", "dark-aether", "trench", "hollow", "dense", "owlfish"],
        "param_overrides": {"owl_filterCutoff": 0.12, "owl_formantShift": 0.2, "owl_hollowAmt": 0.85, "owl_reverbMix": 0.82, "owl_ampAttack": 4.0, "owl_ampRelease": 7.0},
    },
    {
        "name": "Ombre Black Fog",
        "engine": "Ombre",
        "subcategory": "Dark Aether",
        "dna": {"brightness": 0.05, "warmth": 0.35, "movement": 0.15, "density": 0.6, "space": 0.9, "aggression": 0.03},
        "description": "Maximum shade, nearly zero brightness. The grain fog is impenetrable — a black weather system with no edges, suspended in enormous reverberant space.",
        "tags": ["aether", "dark-aether", "fog", "grain", "impenetrable", "vast"],
        "param_overrides": {"ombre_filterCutoff": 0.03, "ombre_grainDensity": 0.62, "ombre_reverbMix": 0.91, "ombre_ampAttack": 6.0, "ombre_ampRelease": 9.0},
    },
    {
        "name": "Dub Void Wash",
        "engine": "Overdub",
        "subcategory": "Dark Aether",
        "dna": {"brightness": 0.12, "warmth": 0.55, "movement": 0.18, "density": 0.4, "space": 0.96, "aggression": 0.02},
        "description": "Dub reduction to pure reverberant space. The signal is gone — what remains is the room the signal once occupied. Warm darkness, infinite decay.",
        "tags": ["aether", "dark-aether", "dub", "void", "reverberant", "warm"],
        "param_overrides": {"dub_tapeFeedback": 0.3, "dub_filterCutoff": 0.1, "dub_reverbMix": 0.97, "dub_reverbSize": 1.0, "dub_ampAttack": 4.0, "dub_ampRelease": 9.0},
    },
    {
        "name": "Oceanic Night Migration",
        "engine": "Oceanic",
        "subcategory": "Dark Aether",
        "dna": {"brightness": 0.07, "warmth": 0.45, "movement": 0.38, "density": 0.45, "space": 0.88, "aggression": 0.04},
        "description": "The deep migration that happens only in darkness. Schools of bioluminescence navigating by pressure gradient — movement felt not seen.",
        "tags": ["aether", "dark-aether", "migration", "bioluminescent", "nocturnal", "deep"],
        "param_overrides": {"ocean_filterCutoff": 0.05, "ocean_separation": 0.75, "ocean_reverbMix": 0.9, "ocean_delayFeedback": 0.5, "ocean_ampAttack": 3.5, "ocean_ampRelease": 7.5},
    },
    {
        "name": "Odyssey Night Watch",
        "engine": "Odyssey",
        "subcategory": "Dark Aether",
        "dna": {"brightness": 0.14, "warmth": 0.5, "movement": 0.12, "density": 0.32, "space": 0.92, "aggression": 0.02},
        "description": "The watch at 0300 hours. No stars visible. The ship navigates by instinct and the faint phosphorescence of its own wake.",
        "tags": ["aether", "dark-aether", "nocturnal", "navigation", "journey", "watch"],
        "param_overrides": {"drift_filterCutoff": 0.11, "drift_lfoRate": 0.03, "drift_reverbMix": 0.93, "drift_ampAttack": 5.0, "drift_ampRelease": 8.5},
    },

    # =====================================================================
    # SHIFTING AETHER (20)
    # Engines: OPTIC, OBLIQUE, ORIGAMI, OCTOPUS, OUROBOROS, OVERWORLD, ONSET, OLE
    # space 0.7-0.95, movement 0.6-0.85, warmth varies
    # =====================================================================
    {
        "name": "Refraction Drift",
        "engine": "Optic",
        "subcategory": "Shifting Aether",
        "dna": {"brightness": 0.72, "warmth": 0.35, "movement": 0.68, "density": 0.3, "space": 0.88, "aggression": 0.05},
        "description": "Light bending through atmosphere in constant flux. The prism drifts — each pass through changes the angle slightly. No two moments of refraction are identical.",
        "tags": ["aether", "shifting-aether", "refraction", "drift", "movement", "spectral"],
        "param_overrides": {"optic_prismAmt": 0.72, "optic_scatterWidth": 0.55, "optic_lfoRate": 0.35, "optic_lfoDepth": 0.6, "optic_reverbMix": 0.82},
    },
    {
        "name": "Phase Dissolve",
        "engine": "Oblique",
        "subcategory": "Shifting Aether",
        "dna": {"brightness": 0.58, "warmth": 0.45, "movement": 0.75, "density": 0.28, "space": 0.9, "aggression": 0.04},
        "description": "Phase cancellation creating moving negative spaces. The oblique angle shifts — voids open and close in the spectral field like breathing architecture.",
        "tags": ["aether", "shifting-aether", "phase", "dissolve", "movement", "architectural"],
        "param_overrides": {"oblq_phaseShift": 0.8, "oblq_lfoRate": 0.3, "oblq_lfoDepth": 0.7, "oblq_reverbMix": 0.85},
    },
    {
        "name": "Infinite Fold",
        "engine": "Origami",
        "subcategory": "Shifting Aether",
        "dna": {"brightness": 0.62, "warmth": 0.5, "movement": 0.7, "density": 0.32, "space": 0.87, "aggression": 0.06},
        "description": "The paper folds infinitely — each crease reveals a new geometry. The model is never complete. Warmth from the folding, space from the unfolded remainder.",
        "tags": ["aether", "shifting-aether", "origami", "fold", "geometric", "movement"],
        "param_overrides": {"origami_foldAmt": 0.72, "origami_lfoRate": 0.25, "origami_lfoDepth": 0.65, "origami_reverbMix": 0.82},
    },
    {
        "name": "Eight Arm Drift",
        "engine": "Octopus",
        "subcategory": "Shifting Aether",
        "dna": {"brightness": 0.48, "warmth": 0.6, "movement": 0.8, "density": 0.38, "space": 0.85, "aggression": 0.07},
        "description": "Eight arms in independent motion — each one a separate oscillator drifting through the space. The CA arms never synchronize. Pure movement in warm darkness.",
        "tags": ["aether", "shifting-aether", "octopus", "arms", "independent", "movement"],
        "param_overrides": {"octo_stretchAmt": 0.82, "octo_lfoRate": 0.4, "octo_lfoDepth": 0.7, "octo_reverbMix": 0.78},
    },
    {
        "name": "Eternal Consumption",
        "engine": "Ouroboros",
        "subcategory": "Shifting Aether",
        "dna": {"brightness": 0.55, "warmth": 0.55, "movement": 0.78, "density": 0.42, "space": 0.82, "aggression": 0.08},
        "description": "The cycle has been running so long the distinction between devour and regenerate has disappeared. Pure feedback becoming pure motion — the serpent's orbit.",
        "tags": ["aether", "shifting-aether", "ouroboros", "cycle", "feedback", "eternal"],
        "param_overrides": {"ouro_feedbackAmt": 0.72, "ouro_lfoRate": 0.45, "ouro_lfoDepth": 0.72, "ouro_reverbMix": 0.78},
    },
    {
        "name": "NES Sky Scroll",
        "engine": "Overworld",
        "subcategory": "Shifting Aether",
        "dna": {"brightness": 0.68, "warmth": 0.4, "movement": 0.75, "density": 0.3, "space": 0.88, "aggression": 0.05},
        "description": "The NES sky layer scrolling across the overworld. Pixel clouds in permanent lateral motion. The chip hum carries no weight — only the shimmer of 8-bit atmosphere.",
        "tags": ["aether", "shifting-aether", "chip", "NES", "scrolling", "atmospheric"],
        "param_overrides": {"ow_era": 0.1, "ow_eraBlend": 0.15, "ow_lfoRate": 0.35, "ow_lfoDepth": 0.6, "ow_reverbMix": 0.82, "ow_glitchAmt": 0.15},
    },
    {
        "name": "Spectral Drum Ghost",
        "engine": "Onset",
        "subcategory": "Shifting Aether",
        "dna": {"brightness": 0.52, "warmth": 0.5, "movement": 0.65, "density": 0.35, "space": 0.9, "aggression": 0.08},
        "description": "The drum machine ghost — all transients removed, only the ring and reverberant bloom of each voice. The kit plays itself in negative space.",
        "tags": ["aether", "shifting-aether", "drums", "ghost", "reverberant", "movement"],
        "param_overrides": {"perc_reverbMix": 0.92, "perc_filterCutoff": 0.45, "perc_delayFeedback": 0.5, "perc_hihatDecay": 0.8},
    },
    {
        "name": "Latin Cosmos",
        "engine": "Ole",
        "subcategory": "Shifting Aether",
        "dna": {"brightness": 0.65, "warmth": 0.65, "movement": 0.72, "density": 0.35, "space": 0.88, "aggression": 0.06},
        "description": "The Afro-Latin trio at cosmological tempo — so slow the rhythm is felt as weather pattern rather than beat. Drama and warmth diffused into vast celestial motion.",
        "tags": ["aether", "shifting-aether", "latin", "cosmic", "slow", "warm"],
        "param_overrides": {"ole_rhythmAmt": 0.25, "ole_lfoRate": 0.1, "ole_lfoDepth": 0.5, "ole_reverbMix": 0.82},
    },
    {
        "name": "Scatter Prism Walk",
        "engine": "Optic",
        "subcategory": "Shifting Aether",
        "dna": {"brightness": 0.82, "warmth": 0.28, "movement": 0.82, "density": 0.25, "space": 0.92, "aggression": 0.04},
        "description": "Maximum scatter with the prism in motion. The spectral bands walk independently — each frequency component on its own trajectory through vast bright space.",
        "tags": ["aether", "shifting-aether", "scatter", "walk", "prism", "bright"],
        "param_overrides": {"optic_prismAmt": 0.85, "optic_scatterWidth": 0.82, "optic_lfoRate": 0.45, "optic_lfoDepth": 0.8, "optic_reverbMix": 0.88},
    },
    {
        "name": "Oblique Weather",
        "engine": "Oblique",
        "subcategory": "Shifting Aether",
        "dna": {"brightness": 0.45, "warmth": 0.6, "movement": 0.78, "density": 0.38, "space": 0.85, "aggression": 0.06},
        "description": "The weather system approaching from an oblique angle. The pressure front warps spectral content. What arrives is never what was launched.",
        "tags": ["aether", "shifting-aether", "weather", "oblique", "warm", "movement"],
        "param_overrides": {"oblq_phaseShift": 0.65, "oblq_prismColor": 0.6, "oblq_lfoRate": 0.38, "oblq_lfoDepth": 0.78, "oblq_reverbMix": 0.8, "oblq_filterCutoff": 0.45},
    },
    {
        "name": "Fold Sequence",
        "engine": "Origami",
        "subcategory": "Shifting Aether",
        "dna": {"brightness": 0.7, "warmth": 0.42, "movement": 0.65, "density": 0.28, "space": 0.93, "aggression": 0.04},
        "description": "Each fold opens a new spectral window. The sequence has no prescribed end — the paper finds its own geometry across vast resonant space.",
        "tags": ["aether", "shifting-aether", "sequence", "fold", "spectral", "vast"],
        "param_overrides": {"origami_foldAmt": 0.65, "origami_creaseDepth": 0.35, "origami_lfoRate": 0.22, "origami_reverbMix": 0.88},
    },
    {
        "name": "Octopus Color Shift",
        "engine": "Octopus",
        "subcategory": "Shifting Aether",
        "dna": {"brightness": 0.62, "warmth": 0.55, "movement": 0.85, "density": 0.4, "space": 0.78, "aggression": 0.09},
        "description": "Chromatophores in rapid state change — the octopus's signature color shift as sound. Eight simultaneous tonal colors cycling at different rates.",
        "tags": ["aether", "shifting-aether", "octopus", "color", "shift", "rapid"],
        "param_overrides": {"octo_stretchAmt": 0.88, "octo_lfoRate": 0.5, "octo_lfoDepth": 0.8, "octo_reverbMix": 0.72},
    },
    {
        "name": "Serpent Modulation",
        "engine": "Ouroboros",
        "subcategory": "Shifting Aether",
        "dna": {"brightness": 0.6, "warmth": 0.48, "movement": 0.72, "density": 0.38, "space": 0.87, "aggression": 0.07},
        "description": "The serpent's modulation depth — tail entering mouth creates a modulation index that perpetually redefines the spectrum. Circular, spacious, inevitable.",
        "tags": ["aether", "shifting-aether", "serpent", "modulation", "circular", "vast"],
        "param_overrides": {"ouro_feedbackAmt": 0.65, "ouro_lfoRate": 0.38, "ouro_lfoDepth": 0.7, "ouro_reverbMix": 0.83},
    },
    {
        "name": "Genesis FM Drift",
        "engine": "Overworld",
        "subcategory": "Shifting Aether",
        "dna": {"brightness": 0.58, "warmth": 0.48, "movement": 0.68, "density": 0.32, "space": 0.85, "aggression": 0.06},
        "description": "YM2612 FM operators in continuous drift. The Genesis palette shifted toward the aetheric — brass stabs become slow moving clouds of metallic shimmer.",
        "tags": ["aether", "shifting-aether", "genesis", "FM", "drift", "metallic"],
        "param_overrides": {"ow_era": 0.6, "ow_eraBlend": 0.6, "ow_lfoRate": 0.28, "ow_lfoDepth": 0.62, "ow_reverbMix": 0.8},
    },
    {
        "name": "Mutant Percussion Cloud",
        "engine": "Onset",
        "subcategory": "Shifting Aether",
        "dna": {"brightness": 0.45, "warmth": 0.55, "movement": 0.78, "density": 0.42, "space": 0.88, "aggression": 0.1},
        "description": "Mutation at maximum — the kit voices have drifted from their physical origins into tonal clouds. What was a kick is now a bass shimmer. What was a hat is now crystalline mist.",
        "tags": ["aether", "shifting-aether", "mutation", "drums", "cloud", "evolved"],
        "param_overrides": {"perc_reverbMix": 0.88, "perc_filterCutoff": 0.52, "perc_delayFeedback": 0.55, "perc_hihatDecay": 0.9},
    },
    {
        "name": "Drama Cosmos",
        "engine": "Ole",
        "subcategory": "Shifting Aether",
        "dna": {"brightness": 0.72, "warmth": 0.7, "movement": 0.68, "density": 0.32, "space": 0.9, "aggression": 0.07},
        "description": "The drama fully engaged but at cosmic tempo. The trio's passion diffused across enormous space — each gesture arrives seconds after it was initiated.",
        "tags": ["aether", "shifting-aether", "drama", "cosmic", "warm", "vast"],
        "param_overrides": {"ole_rhythmAmt": 0.35, "ole_lfoRate": 0.15, "ole_lfoDepth": 0.62, "ole_reverbMix": 0.85},
    },
    {
        "name": "Optic Slow Bloom",
        "engine": "Optic",
        "subcategory": "Shifting Aether",
        "dna": {"brightness": 0.65, "warmth": 0.5, "movement": 0.62, "density": 0.28, "space": 0.87, "aggression": 0.04},
        "description": "Refraction in slow motion — the bloom takes minutes to complete. Each spectral band opens at a different rate. Warmth and brightness intersect at the threshold.",
        "tags": ["aether", "shifting-aether", "bloom", "slow", "spectral", "warm"],
        "param_overrides": {"optic_prismAmt": 0.55, "optic_lfoRate": 0.15, "optic_lfoDepth": 0.58, "optic_reverbMix": 0.82, "optic_filterCutoff": 0.62},
    },
    {
        "name": "Fold Cascade",
        "engine": "Origami",
        "subcategory": "Shifting Aether",
        "dna": {"brightness": 0.55, "warmth": 0.65, "movement": 0.8, "density": 0.38, "space": 0.82, "aggression": 0.08},
        "description": "The cascade — each fold triggers the next faster. Warmth builds from the friction of paper against paper. The shape changes faster than the eye can follow.",
        "tags": ["aether", "shifting-aether", "cascade", "fold", "warm", "accelerating"],
        "param_overrides": {"origami_foldAmt": 0.82, "origami_creaseDepth": 0.65, "origami_lfoRate": 0.45, "origami_lfoDepth": 0.75, "origami_reverbMix": 0.78},
    },
    {
        "name": "Ouroboros Thaw",
        "engine": "Ouroboros",
        "subcategory": "Shifting Aether",
        "dna": {"brightness": 0.7, "warmth": 0.62, "movement": 0.62, "density": 0.3, "space": 0.93, "aggression": 0.05},
        "description": "The serpent warming after winter stasis. The cycle resumes at half speed — luminous, spacious, the feedback loop re-establishing its ancient rhythm.",
        "tags": ["aether", "shifting-aether", "thaw", "warming", "bright", "vast"],
        "param_overrides": {"ouro_feedbackAmt": 0.5, "ouro_lfoRate": 0.28, "ouro_lfoDepth": 0.58, "ouro_reverbMix": 0.88, "ouro_filterCutoff": 0.68},
    },
    {
        "name": "SNES Cloud Layer",
        "engine": "Overworld",
        "subcategory": "Shifting Aether",
        "dna": {"brightness": 0.75, "warmth": 0.55, "movement": 0.7, "density": 0.28, "space": 0.92, "aggression": 0.04},
        "description": "SNES SPC700 cloud samples — the warm, slightly compressed sky of 16-bit overworlds. The echo DSP creates halos around every note. Movement is continuous and luminous.",
        "tags": ["aether", "shifting-aether", "SNES", "cloud", "warm", "luminous"],
        "param_overrides": {"ow_era": 0.9, "ow_eraBlend": 0.9, "ow_lfoRate": 0.32, "ow_lfoDepth": 0.65, "ow_reverbMix": 0.87},
    },
    {
        "name": "Octopus Ink Bloom",
        "engine": "Octopus",
        "subcategory": "Shifting Aether",
        "dna": {"brightness": 0.38, "warmth": 0.7, "movement": 0.75, "density": 0.45, "space": 0.82, "aggression": 0.08},
        "description": "The ink cloud released and expanding in slow motion — an octopus signature expanding through the water column, warm and dark, in constant motion.",
        "tags": ["aether", "shifting-aether", "ink", "bloom", "warm", "expanding"],
        "param_overrides": {"octo_inkDepth": 0.8, "octo_armDepth": 0.6, "octo_lfoRate": 0.3, "octo_lfoDepth": 0.72, "octo_reverbMix": 0.8, "octo_filterCutoff": 0.38},
    },
]


# ---------------------------------------------------------------------------
# Build + write
# ---------------------------------------------------------------------------

def build_preset(spec):
    engine = spec["engine"]
    meta = ENGINE_META[engine]
    labels = meta["macroLabels"]
    params = dict(meta["params"])
    params.update(spec.get("param_overrides", {}))

    return {
        "schema_version": 1,
        "name": spec["name"],
        "mood": "Aether",
        "engines": [engine],
        "author": "XPN Aether Expansion — 2026-03-16",
        "version": "1.0.0",
        "description": spec["description"],
        "tags": spec["tags"],
        "macroLabels": labels,
        "couplingIntensity": "None",
        "dna": spec["dna"],
        "parameters": {engine: params},
        "coupling": {"pairs": []},
        "sequencer": None,
    }


def filename_from_spec(spec):
    safe_name = spec["name"].replace(" ", "_").replace("/", "-").replace("'", "")
    return f"{spec['engine']}_{safe_name}.xometa"


def main():
    os.makedirs(OUTPUT_DIR, exist_ok=True)
    existing = set(os.listdir(OUTPUT_DIR))

    written = 0
    skipped = 0
    by_sub = {"Transcendent": 0, "Dark Aether": 0, "Shifting Aether": 0}

    for spec in PRESETS:
        fname = filename_from_spec(spec)
        if fname in existing:
            skipped += 1
            continue
        preset = build_preset(spec)
        path = os.path.join(OUTPUT_DIR, fname)
        with open(path, "w") as f:
            json.dump(preset, f, indent=2)
            f.write("\n")
        written += 1
        by_sub[spec["subcategory"]] += 1

    print("Aether Expansion Pack complete.")
    print(f"  Total defined : {len(PRESETS)}")
    print(f"  Written       : {written}")
    print(f"  Skipped (exist): {skipped}")
    print("  By subcategory (written):")
    for sub, count in by_sub.items():
        print(f"    {sub}: {count}")
    print(f"  Output: {OUTPUT_DIR}")


if __name__ == "__main__":
    main()
