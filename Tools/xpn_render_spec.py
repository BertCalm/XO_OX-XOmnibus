#!/usr/bin/env python3
"""
XPN Render Spec Generator — XO_OX Designs
Reads .xometa preset files and generates render specifications that tell
the producer exactly what WAV files to record for XPN export.

The render spec bridges the gap between XOceanus presets (live synthesis)
and MPC expansion packs (static samples). It reads the preset's engine,
Sonic DNA, and parameters to determine the optimal render strategy.

Engine-Specific Strategies (Atlas bridge §10):
  ONSET     → Drum program: 8 voices × 4 velocity layers = 32 WAVs
  ODYSSEY   → Keygroup: sample every minor 3rd, C1-C6 = 21 notes × 4 vel
  BOB       → Keygroup: sample C1-C5 (warm analog — narrow range)
  FAT/OBESE → Keygroup: C1-C3 bass focus (low register is the identity)
  MORPH     → 3 keygroups: Morph A, Mid, B (snapshot morph states)
  DUB       → Keygroup: dry + wet variants
  OVERWORLD → Keygroup: C1-C4 (chip register, narrow range)
  OPAL      → WAV stems: 30-60s texture renders (not pitched programs)

Usage:
    # Generate render spec for one preset
    python3 xpn_render_spec.py --preset /path/to/preset.xometa

    # Generate specs for all presets of an engine
    python3 xpn_render_spec.py --engine Onset --output-dir /path/to/specs

    # Generate full fleet render plan
    python3 xpn_render_spec.py --all --output-dir /path/to/specs
"""

import argparse
import json
import sys
from pathlib import Path
from typing import Optional

from engine_registry import get_all_engines
from xpn_voice_taxonomy import ONSET_VOICE_MAP  # canonical voice name map (QDD L4 fix)

REPO_ROOT   = Path(__file__).parent.parent
PRESETS_DIR = REPO_ROOT / "Presets" / "XOceanus"

# Standard chromatic sample points: every minor 3rd from C1 to C6
MINOR_3RD_NOTES = [
    "C1", "Eb1", "F#1", "A1",
    "C2", "Eb2", "F#2", "A2",
    "C3", "Eb3", "F#3", "A3",
    "C4", "Eb4", "F#4", "A4",
    "C5", "Eb5", "F#5", "A5",
    "C6",
]

# Engine → render strategy
ENGINE_STRATEGIES = {
    "Onset": {
        "program_type": "drum",
        # Voice names are engine-internal (see xpn_voice_taxonomy.ONSET_VOICE_MAP for
        # the mapping to XPN canonical pad names used by XPNDrumExporter.h).
        # "chat" → "closed_hat", "ohat" → "open_hat" in C++ land.
        # WAV filenames always use these internal names (e.g., preset_chat_v1.wav).
        "voices": list(ONSET_VOICE_MAP.keys()),  # ["kick","snare","chat","ohat","clap","tom","perc","fx"]
        "vel_layers": 4,
        "vel_suffixes": ["v1", "v2", "v3", "v4"],
        "notes": None,  # drums don't use notes
        "description": "8 drum voices × 4 velocity layers",
    },
    "Odyssey": {
        "program_type": "keygroup",
        "notes": MINOR_3RD_NOTES,
        "vel_layers": 4,
        "range": "C1-C6",
        "description": "Full chromatic sampling every minor 3rd",
    },
    "Overdub": {
        "program_type": "keygroup",
        "notes": MINOR_3RD_NOTES[:17],  # C1-C5
        "vel_layers": 2,
        "range": "C1-C5",
        "description": "Dub character — render dry and wet variants",
        "variants": ["dry", "wet"],
    },
    "Oblong": {
        "program_type": "keygroup",
        "notes": MINOR_3RD_NOTES[:17],  # C1-C5
        "vel_layers": 4,
        "range": "C1-C5",
        "description": "Warm analog — close-mic rendering",
    },
    # Guru Bin render rule: Render the saturation, not around it.
    # The driven ladder filter is OBESE's signature character.
    # NEVER back off drive to normalize output level.
    # Back off gain/volume instead. The distortion IS the sound.
    "Obese": {
        "program_type": "keygroup",
        "notes": MINOR_3RD_NOTES[:9],  # C1-C3
        "vel_layers": 4,
        "range": "C1-C3",
        "description": "Bass register focus — low end is the identity",
    },
    "OddfeliX": {
        "program_type": "keygroup",
        "notes": MINOR_3RD_NOTES,
        "vel_layers": 4,
        "range": "C1-C6",
        "description": "Full range — transient-heavy, preserve attacks",
    },
    "OddOscar": {
        "program_type": "keygroup",
        "notes": MINOR_3RD_NOTES,
        "vel_layers": 4,
        "range": "C1-C6",
        "description": "Morph states — render at 3 morph positions",
        "variants": ["morph_a", "morph_mid", "morph_b"],
    },
    "Overworld": {
        "program_type": "keygroup",
        "notes": MINOR_3RD_NOTES[:13],  # C1-C4
        "vel_layers": 2,
        "range": "C1-C4",
        "description": "Chip register — narrow range, NES/Genesis character",
    },
    "Opal": {
        "program_type": "stem",
        "duration_seconds": 30,
        "description": "Granular texture — render 30s WAV stems, not pitched programs",
    },
    "Overbite": {
        "program_type": "keygroup",
        "notes": MINOR_3RD_NOTES[:13],  # C1-C4
        "vel_layers": 4,
        "range": "C1-C4",
        "description": "Bass character — low register focus",
    },
    "Ohm": {
        "program_type": "keygroup",
        "notes": MINOR_3RD_NOTES[:17],  # C1-C5
        "vel_layers": 2,
        "range": "C1-C5",
        "description": "Sage commune — render at two intensities (gentle/full commune). "
                       "Set MEDDLING to 0.0 for calm variant, 0.7 for full commune variant. "
                       "Capture the hum sustain, not the attack. Hold 4+ seconds.",
        "variants": ["calm", "full"],
    },
    "Orphica": {
        "program_type": "keygroup",
        "notes": MINOR_3RD_NOTES,
        "vel_layers": 4,
        "range": "C1-C6",
        "description": "Siphonophore harp — pluck transient is everything. "
                       "Render dry, let the harp tail decay naturally. "
                       "3 second hold per note minimum.",
    },
    "Obbligato": {
        "program_type": "keygroup",
        "notes": MINOR_3RD_NOTES[:17],  # C1-C5
        "vel_layers": 4,
        "range": "C1-C5",
        "description": "Dual wind texture — render with BOND macro at 0.0 (A only), "
                       "0.5 (blended), and 1.0 (B only). Three variants capture the "
                       "harmonic blend range.",
        "variants": ["bond_a", "bond_blend", "bond_b"],
    },
    "Ottoni": {
        "program_type": "keygroup",
        "notes": MINOR_3RD_NOTES[:13],  # C1-C4
        "vel_layers": 4,
        "range": "C1-C4",
        "description": "Triple brass choir — render with GROW macro at minimum for "
                       "intimate solo brass, maximum for full ensemble swell. "
                       "Capture the attack transient — brass is about the slam.",
        "variants": ["grow_solo", "grow_ensemble"],
    },
    "Ole": {
        "program_type": "keygroup",
        "notes": MINOR_3RD_NOTES[:17],  # C1-C5
        "vel_layers": 4,
        "range": "C1-C5",
        "description": "Afro-Latin trio — render with DRAMA macro at 0.0 (restrained) "
                       "and 1.0 (full drama). Rhythm is the identity: capture the "
                       "attack in the first 50ms.",
        "variants": ["calm", "drama"],
    },
    "Orbital": {
        "program_type": "keygroup",
        "notes": MINOR_3RD_NOTES,
        "vel_layers": 4,
        "range": "C1-C6",
        "description": "Group envelope system — render with full sustain to capture "
                       "envelope interactions. Hold 3+ seconds per note.",
    },
    "Organon": {
        "program_type": "keygroup",
        "notes": MINOR_3RD_NOTES[:17],  # C1-C5
        "vel_layers": 4,
        "range": "C1-C5",
        "description": "Metabolic synthesis — render at low and high metabolic rates. "
                       "Hold 4+ seconds to capture the living evolution.",
        "variants": ["low_metabolism", "high_metabolism"],
    },
    "Ouroboros": {
        "program_type": "keygroup",
        "notes": MINOR_3RD_NOTES[:17],  # C1-C5
        "vel_layers": 4,
        "range": "C1-C5",
        "description": "Strange attractor chaos — render with leash tight (stable) and "
                       "leash loose (chaotic). Hold 3+ seconds for feedback to develop.",
        "variants": ["leash_tight", "leash_loose"],
    },
    "Obsidian": {
        "program_type": "keygroup",
        "notes": MINOR_3RD_NOTES,
        "vel_layers": 4,
        "range": "C1-C6",
        "description": "Crystal resonance — render dry, full range. The glassy "
                       "harmonics need clean captures.",
    },
    "Origami": {
        "program_type": "keygroup",
        "notes": MINOR_3RD_NOTES[:17],  # C1-C5
        "vel_layers": 4,
        "range": "C1-C5",
        "description": "Fold synthesis — render at fold point extremes (low fold = smooth, "
                       "high fold = complex harmonics).",
        "variants": ["fold_low", "fold_high"],
    },
    "Oracle": {
        "program_type": "keygroup",
        "notes": MINOR_3RD_NOTES[:17],  # C1-C5
        "vel_layers": 4,
        "range": "C1-C5",
        "description": "GENDY stochastic + maqam — render 3+ second holds to let the "
                       "stochastic walk develop. Capture the unpredictability.",
    },
    "Obscura": {
        "program_type": "keygroup",
        "notes": MINOR_3RD_NOTES[:17],  # C1-C5
        "vel_layers": 4,
        "range": "C1-C5",
        "description": "Physical modeling strings — render dry, hold 3+ seconds for the "
                       "stiffness and body resonance to bloom.",
    },
    "Oceanic": {
        "program_type": "keygroup",
        "notes": MINOR_3RD_NOTES[:17],  # C1-C5
        "vel_layers": 2,
        "range": "C1-C5",
        "description": "Chromatophore modulator — render at two separation states. "
                       "Hold 4+ seconds for the tidal evolution.",
        "variants": ["separated", "converged"],
    },
    "Ocelot": {
        "program_type": "keygroup",
        "notes": MINOR_3RD_NOTES[:17],  # C1-C5
        "vel_layers": 4,
        "range": "C1-C5",
        "description": "Biome synthesis — render at different biome settings to capture "
                       "timbral environment shifts.",
        "variants": ["biome_a", "biome_b"],
    },
    "Optic": {
        "program_type": "stem",
        "duration_seconds": 30,
        "description": "Visual modulation engine — zero-audio identity. Render 30s stems "
                       "of the AutoPulse rhythm output. This engine may produce silence "
                       "in some modes (visual-only); only render audio-producing presets.",
    },
    "Oblique": {
        "program_type": "keygroup",
        "notes": MINOR_3RD_NOTES,
        "vel_layers": 4,
        "range": "C1-C6",
        "description": "Prismatic bounce — full range, render dry. The prism reflections "
                       "need clean captures across the full keyboard.",
    },
    "Osprey": {
        "program_type": "keygroup",
        "notes": MINOR_3RD_NOTES[:17],  # C1-C5
        "vel_layers": 4,
        "range": "C1-C5",
        "description": "ShoreSystem coastline — render at different shore blend positions "
                       "to capture the cultural timbral shifts.",
        "variants": ["shore_near", "shore_far"],
    },
    "Osteria": {
        "program_type": "keygroup",
        "notes": MINOR_3RD_NOTES[:17],  # C1-C5
        "vel_layers": 4,
        "range": "C1-C5",
        "description": "ShoreSystem coastline — porto wine warmth. Render with natural "
                       "sustain, hold 2+ seconds per note.",
    },
    "Owlfish": {
        "program_type": "keygroup",
        "notes": MINOR_3RD_NOTES[:13],  # C1-C4
        "vel_layers": 4,
        "range": "C1-C4",
        "description": "Mixtur-Trautonium — abyssal register focus. Render with long "
                       "holds (4+ seconds) to capture the subharmonic series development.",
    },
    "Overlap": {
        "program_type": "keygroup",
        "notes": MINOR_3RD_NOTES[:17],  # C1-C5
        "vel_layers": 2,
        "range": "C1-C5",
        "description": "FDN reverb engine — render wet and dry variants at multiple room "
                       "sizes. Hold 4+ seconds for the reverb tail to develop fully.",
        "variants": ["dry", "wet_small", "wet_large"],
    },
    "Outwit": {
        "program_type": "keygroup",
        "notes": MINOR_3RD_NOTES[:17],  # C1-C5
        "vel_layers": 4,
        "range": "C1-C5",
        "description": "8-arm Wolfram CA — render at different rule states to capture "
                       "the cellular automata timbral evolution. Render at rule "
                       "extremes (simple=30, complex=110) and varied arm configs.",
        "variants": ["rule_simple", "rule_complex"],
    },
    "Ombre": {
        "program_type": "keygroup",
        "notes": MINOR_3RD_NOTES[:17],  # C1-C5
        "vel_layers": 2,
        "range": "C1-C5",
        "description": "Dual narrative — memory/forgetting engine. Render at both blend "
                       "extremes: full memory (0.0) and full forgetting (1.0). "
                       "Hold 3+ seconds for the narrative arc.",
        "variants": ["memory", "forgetting"],
    },
    "Orca": {
        "program_type": "keygroup",
        "notes": MINOR_3RD_NOTES[:17],  # C1-C5
        "vel_layers": 4,
        "range": "C1-C5",
        "description": "Wavetable + echolocation — render at different hunt macro positions "
                       "to capture the predatory timbral sweep. Include breach variant "
                       "at maximum intensity.",
        "variants": ["calm_hunt", "active_hunt", "breach"],
    },
    "Octopus": {
        "program_type": "keygroup",
        "notes": MINOR_3RD_NOTES[:17],  # C1-C5
        "vel_layers": 4,
        "range": "C1-C5",
        "description": "Decentralized 8-arm intelligence — render at different arm depth "
                       "settings and chromatophore states. Hold 3+ seconds for the "
                       "distributed modulation to develop.",
        "variants": ["shallow_arms", "deep_arms"],
    },
    # Concept engines (DSP pending — strategies ready for when they ship)
    "Ostinato": {
        "program_type": "drum",
        "voices": ["drum1", "drum2", "drum3", "drum4", "bell", "shaker", "bass", "accent"],
        "vel_layers": 4,
        "vel_suffixes": ["v1", "v2", "v3", "v4"],
        "notes": None,
        "description": "Communal drum circle — 8 voices × 4 velocity layers. Capture "
                       "the communal groove: each voice is a circle participant.",
    },
    "OpenSky": {
        "program_type": "keygroup",
        "notes": MINOR_3RD_NOTES,
        "vel_layers": 2,
        "range": "C1-C6",
        "description": "Euphoric shimmer — pure feliX energy. Full range, render with "
                       "long sustains (4+ seconds) to capture the shimmer tail.",
    },
    "OceanDeep": {
        "program_type": "keygroup",
        "notes": MINOR_3RD_NOTES[:9],  # C1-C3
        "vel_layers": 4,
        "range": "C1-C3",
        "description": "Abyssal bass — pure Oscar. Low register only, render with 4+ "
                       "second holds to capture the trench pressure.",
    },
    "Ouie": {
        "program_type": "keygroup",
        "notes": MINOR_3RD_NOTES[:17],  # C1-C5
        "vel_layers": 4,
        "range": "C1-C5",
        "description": "Duophonic hammerhead — render at STRIFE (0.0) and LOVE (1.0) "
                       "axis positions. Capture both characters.",
        "variants": ["strife", "love"],
    },

    # ── Theorem engines (2026-03-20+) ─────────────────────────────────────────
    "Obrix": {
        "program_type": "keygroup",
        "notes": MINOR_3RD_NOTES[:17],  # C1-C5
        "vel_layers": 4,
        "range": "C1-C5",
        "description": "Modular brick synthesis — coral reef ecology. Render with "
                       "Harmonic Field at 0.0 (dry brick) and 1.0 (full JI attractor).",
        "variants": ["field_off", "field_on"],
    },
    "Orbweave": {
        "program_type": "keygroup",
        "notes": MINOR_3RD_NOTES[:17],  # C1-C5
        "vel_layers": 2,
        "range": "C1-C5",
        "description": "Topological knot coupling — render at different knot types. "
                       "Hold 3+ seconds for the knot phase to evolve.",
        "variants": ["trefoil", "torus"],
    },
    "Overtone": {
        "program_type": "keygroup",
        "notes": MINOR_3RD_NOTES,
        "vel_layers": 4,
        "range": "C1-C6",
        "description": "Continued fraction spectral — render at different convergent "
                       "constants (π, φ, √2). Full range, clean captures.",
        "variants": ["pi_convergent", "phi_convergent"],
    },
    "Organism": {
        "program_type": "stem",
        "duration_seconds": 30,
        "description": "Cellular automata generative — render 30s stems per rule set. "
                       "Captures emergent coral colony patterns across rule changes.",
        "variants": ["rule_a", "rule_b"],
    },
    "Oxbow": {
        "program_type": "keygroup",
        "notes": MINOR_3RD_NOTES[:17],  # C1-C5
        "vel_layers": 2,
        "range": "C1-C5",
        "description": "Entangled reverb synth — render dry and wet variants. "
                       "Hold 4+ seconds for the FDN reverb tail to develop.",
        "variants": ["dry", "wet"],
    },
    "Oware": {
        "program_type": "keygroup",
        "notes": MINOR_3RD_NOTES[:17],  # C1-C5
        "vel_layers": 4,
        "range": "C1-C5",
        "description": "Tuned percussion — mallet physics. Render with different "
                       "material settings (wood/metal/stone). 2s hold per note.",
        "variants": ["wood", "metal", "stone"],
    },
    "Opera": {
        "program_type": "keygroup",
        "notes": MINOR_3RD_NOTES,
        "vel_layers": 4,
        "range": "C1-C6",
        "description": "Additive-vocal Kuramoto — render with Conductor arc active. "
                       "Hold 4+ seconds for the dramatic arc to develop.",
    },
    "Offering": {
        "program_type": "drum",
        "voices": ["kick", "snare", "chat", "ohat", "clap", "tom", "fx", "accent"],
        "vel_layers": 4,
        "vel_suffixes": ["v1", "v2", "v3", "v4"],
        "notes": None,
        "description": "Psychology-driven boom bap — 8 voices × 4 velocity layers. "
                       "Capture the Berlyne curiosity arc across velocity layers.",
    },
    "Osmosis": {
        "program_type": "stem",
        "duration_seconds": 30,
        "description": "External audio membrane — render 30s process stems. "
                       "Route external source through the membrane at low/high permeability.",
        "variants": ["low_permeability", "high_permeability"],
    },
    "Oxytocin": {
        "program_type": "keygroup",
        "notes": MINOR_3RD_NOTES[:17],  # C1-C5
        "vel_layers": 4,
        "range": "C1-C5",
        "description": "Circuit-modeling love-triangle — render long notes (4+ seconds) to "
                       "capture note-duration-as-synthesis (B040). Include intimacy/passion "
                       "axis variants.",
        "variants": ["short_notes", "long_notes"],
    },
    "Outlook": {
        "program_type": "keygroup",
        "notes": MINOR_3RD_NOTES,
        "vel_layers": 2,
        "range": "C1-C6",
        "description": "Panoramic dual wavetable — render with vista filter at different "
                       "horizon scan positions. Full range.",
        "variants": ["near_horizon", "far_horizon"],
    },
    "Obiont": {
        "program_type": "keygroup",
        "notes": MINOR_3RD_NOTES[:17],  # C1-C5
        "vel_layers": 2,
        "range": "C1-C5",
        "description": "Wolfram CA oscillator — render at different CA rule sets "
                       "(30 = chaotic, 110 = complex, 90 = symmetric). "
                       "8-voice poly — render chords for full texture.",
        "variants": ["rule_30", "rule_90", "rule_110"],
    },
    "Okeanos": {
        "program_type": "keygroup",
        "notes": MINOR_3RD_NOTES[:17],  # C1-C5
        "vel_layers": 4,
        "range": "C1-C5",
        "description": "Spice Route Rhodes — bioluminescent EP. Render with entropy at "
                       "low (clean) and high (evolved) settings.",
        "variants": ["clean", "evolved"],
    },
    "Outflow": {
        "program_type": "keygroup",
        "notes": MINOR_3RD_NOTES[:17],  # C1-C5
        "vel_layers": 4,
        "range": "C1-C5",
        "description": "Predictive fluid dynamics — render at different current speed "
                       "settings. Hold 3+ seconds for spatial evolution.",
        "variants": ["low_current", "high_current"],
    },

    # ── Kitchen Collection — Chef quad (organs) ────────────────────────────────
    "Oto": {
        "program_type": "keygroup",
        "notes": MINOR_3RD_NOTES[:17],  # C1-C5
        "vel_layers": 2,
        "range": "C1-C5",
        "description": "Pipe organ / melodica / harmonica / sho — render at 3 drawbar "
                       "configurations (sparse, mid, full).",
        "variants": ["sparse", "mid", "full"],
    },
    "Octave": {
        "program_type": "keygroup",
        "notes": MINOR_3RD_NOTES[:17],  # C1-C5
        "vel_layers": 2,
        "range": "C1-C5",
        "description": "Hammond tonewheel — render at slow and fast Leslie speeds. "
                       "Hold 2+ seconds per note to capture the Leslie rotation.",
        "variants": ["slow_leslie", "fast_leslie"],
    },
    "Oleg": {
        "program_type": "keygroup",
        "notes": MINOR_3RD_NOTES[:17],  # C1-C5
        "vel_layers": 4,
        "range": "C1-C5",
        "description": "Theatre pipe organ — render at different harmonics settings. "
                       "Capture the swell pedal dynamics across velocity layers.",
    },
    "Otis": {
        "program_type": "keygroup",
        "notes": MINOR_3RD_NOTES[:17],  # C1-C5
        "vel_layers": 4,
        "range": "C1-C5",
        "description": "Gospel organ — render with SOUL drive at low and high settings. "
                       "Include Leslie slow/fast variants. Hold 2+ seconds per note.",
        "variants": ["soul_low", "soul_high"],
    },

    # ── Kitchen Collection — Kitchen quad (pianos) ─────────────────────────────
    "Oven": {
        "program_type": "keygroup",
        "notes": MINOR_3RD_NOTES,
        "vel_layers": 4,
        "range": "C1-C6",
        "description": "Steinway grand — render with hammer brightness at low, mid, high. "
                       "Full range, clean room capture. Let decay ring naturally.",
        "variants": ["soft", "medium", "bright"],
    },
    "Ochre": {
        "program_type": "keygroup",
        "notes": MINOR_3RD_NOTES,
        "vel_layers": 4,
        "range": "C1-C6",
        "description": "Upright piano — wood resonance character. Render with caramel "
                       "saturation off (clean) and on (warm). Full range.",
        "variants": ["clean", "caramel"],
    },
    "Obelisk": {
        "program_type": "keygroup",
        "notes": MINOR_3RD_NOTES,
        "vel_layers": 4,
        "range": "C1-C6",
        "description": "Concert grand — string mass variations. Render full range with "
                       "long natural decay. This is the purity anchor of the quad.",
    },
    "Opaline": {
        "program_type": "keygroup",
        "notes": MINOR_3RD_NOTES,
        "vel_layers": 4,
        "range": "C1-C6",
        "description": "Prepared piano — render at different preparation densities "
                       "(minimal/mid/full). Let the prepared resonances sustain.",
        "variants": ["prep_low", "prep_mid", "prep_full"],
    },

    # ── Kitchen Collection — Cellar quad (bass) ────────────────────────────────
    "Ogre": {
        "program_type": "keygroup",
        "notes": MINOR_3RD_NOTES[:9],  # C0-C2 (sub bass)
        "vel_layers": 4,
        "range": "C0-C2",
        "description": "Sub bass — render in the deepest register. Hold 3+ seconds for "
                       "the tectonic LFO to complete a cycle. Soil typology variants.",
        "variants": ["soil_clay", "soil_rocky"],
    },
    "Olate": {
        "program_type": "keygroup",
        "notes": MINOR_3RD_NOTES[:17],  # C1-C5
        "vel_layers": 4,
        "range": "C1-C5",
        "description": "Fretless bass — render with terroir at different regional settings. "
                       "Capture glide slides between notes.",
        "variants": ["terroir_low", "terroir_high"],
    },
    "Oaken": {
        "program_type": "keygroup",
        "notes": MINOR_3RD_NOTES[:17],  # C1-C5
        "vel_layers": 4,
        "range": "C1-C5",
        "description": "Upright double bass — string age character. Render at new (bright) "
                       "and aged (warm) string settings.",
        "variants": ["new_strings", "aged_strings"],
    },
    "Omega": {
        "program_type": "keygroup",
        "notes": MINOR_3RD_NOTES[:9],  # C0-C3
        "vel_layers": 4,
        "range": "C0-C3",
        "description": "Synth bass — render deep register focus. Full analogue character: "
                       "capture filter sweeps via velocity layers.",
    },

    # ── Kitchen Collection — Garden quad (strings) ─────────────────────────────
    "Orchard": {
        "program_type": "keygroup",
        "notes": MINOR_3RD_NOTES,
        "vel_layers": 4,
        "range": "C1-C6",
        "description": "Bowed strings — render at different bow pressure settings. "
                       "Hold 3+ seconds for the sustain to bloom.",
        "variants": ["soft_bow", "full_bow"],
    },
    "Overgrow": {
        "program_type": "keygroup",
        "notes": MINOR_3RD_NOTES,
        "vel_layers": 2,
        "range": "C1-C6",
        "description": "Forest growth strings — render with growth rate at slow and fast. "
                       "Hold 4+ seconds for the organic evolution.",
        "variants": ["slow_growth", "fast_growth"],
    },
    "Osier": {
        "program_type": "keygroup",
        "notes": MINOR_3RD_NOTES[:17],  # C1-C5
        "vel_layers": 2,
        "range": "C1-C5",
        "description": "Wind instrument / willow reeds — render with wind density at low "
                       "and high. Capture the breathy character.",
        "variants": ["light_wind", "dense_wind"],
    },
    "Oxalis": {
        "program_type": "keygroup",
        "notes": MINOR_3RD_NOTES,
        "vel_layers": 2,
        "range": "C1-C6",
        "description": "Plucked strings / wood sorrel — render with leaf tension at "
                       "low (loose) and high (taut). Full range.",
        "variants": ["loose", "taut"],
    },

    # ── Kitchen Collection — Broth quad (pads) ────────────────────────────────
    "Overwash": {
        "program_type": "stem",
        "duration_seconds": 30,
        "description": "Tide foam — render 30s pad stems. Capture the slow tidal cycle "
                       "at different tide depth settings.",
        "variants": ["shallow_tide", "deep_tide"],
    },
    "Overworn": {
        "program_type": "stem",
        "duration_seconds": 30,
        "description": "Worn felt pads — render 30s stems. Capture the aged felt character "
                       "at different felt age settings.",
        "variants": ["fresh_felt", "worn_felt"],
    },
    "Overflow": {
        "program_type": "stem",
        "duration_seconds": 30,
        "description": "Deep current pads — render 30s stems. Capture different current "
                       "speeds for pad evolution.",
        "variants": ["slow_current", "fast_current"],
    },
    "Overcast": {
        "program_type": "stem",
        "duration_seconds": 30,
        "description": "Light slate gray pads — render 30s cloud stems. Different cloud "
                       "densities create distinct pad characters.",
        "variants": ["light_cloud", "dense_cloud"],
    },

    # ── Kitchen Collection — Fusion quad (EPs) ────────────────────────────────
    "Oasis": {
        "program_type": "keygroup",
        "notes": MINOR_3RD_NOTES[:17],  # C1-C5
        "vel_layers": 4,
        "range": "C1-C5",
        "description": "Desert spring EP — bioluminescent ecosystem. Render at different "
                       "spring depth settings. Capture the clean EP attack.",
        "variants": ["shallow_spring", "deep_spring"],
    },
    "Oddfellow": {
        "program_type": "keygroup",
        "notes": MINOR_3RD_NOTES[:17],  # C1-C5
        "vel_layers": 4,
        "range": "C1-C5",
        "description": "Fusion copper EP — render with spectral shift variations. "
                       "Capture the warm copper character.",
        "variants": ["shift_low", "shift_high"],
    },
    "Onkolo": {
        "program_type": "keygroup",
        "notes": MINOR_3RD_NOTES[:17],  # C1-C5
        "vel_layers": 4,
        "range": "C1-C5",
        "description": "Spectral amber EP — render with resonance core variations. "
                       "Capture the spectral fingerprint cache character.",
        "variants": ["core_low", "core_high"],
    },
    "Opcode": {
        "program_type": "keygroup",
        "notes": MINOR_3RD_NOTES[:17],  # C1-C5
        "vel_layers": 4,
        "range": "C1-C5",
        "description": "Cadet blue EP — render with code depth variations. "
                       "Capture the digital EP character.",
        "variants": ["depth_low", "depth_high"],
    },
}

# Default strategy for engines not explicitly listed
DEFAULT_STRATEGY = {
    "program_type": "keygroup",
    "notes": MINOR_3RD_NOTES,
    "vel_layers": 4,
    "range": "C1-C6",
    "description": "Standard keygroup sampling every minor 3rd",
}


def _resolve_engine_name(engine_id: str) -> str:
    """Resolve engine ID from preset 'engines' array to canonical display name.

    Handles legacy preset-internal engine IDs (Snap, Bob, etc.) that predate
    the O-prefix convention.  Case-variant normalisation for CLI input is
    handled by ENGINE_ALIASES in oxport.py (which now also sources from
    engine_registry.py).  The full canonical name set lives in engine_registry.
    """
    aliases = {
        "Snap": "OddfeliX", "Morph": "OddOscar",
        "Dub": "Overdub", "Drift": "Odyssey",
        "Bob": "Oblong", "Fat": "Obese", "Bite": "Overbite",
        "Olé": "Ole", "Ouïe": "Ouie",
    }
    return aliases.get(engine_id, engine_id)


def generate_render_spec(xometa_path: Path) -> dict:
    """
    Read an .xometa preset and generate a complete render specification.

    Returns a dict with all the information needed to render WAVs for XPN.
    """
    with open(xometa_path) as f:
        preset = json.load(f)

    name     = preset.get("name", xometa_path.stem)
    engines  = preset.get("engines", [])
    engine   = _resolve_engine_name(engines[0]) if engines else "Unknown"
    dna      = preset.get("dna", {})
    mood     = preset.get("mood", "")
    slug     = name.replace(" ", "_")

    strategy = ENGINE_STRATEGIES.get(engine, DEFAULT_STRATEGY).copy()

    # Adapt velocity layers based on Sonic DNA
    aggression = dna.get("aggression", 0.5)
    space      = dna.get("space", 0.5)

    spec = {
        "preset_name": name,
        "preset_slug": slug,
        "engine": engine,
        "mood": mood,
        "dna": dna,
        "program_type": strategy["program_type"],
        "strategy_description": strategy["description"],
        "source_file": str(xometa_path),
    }

    if strategy["program_type"] == "drum":
        # Drum render spec
        wav_list = []
        for voice in strategy["voices"]:
            for i in range(strategy["vel_layers"]):
                suf = strategy["vel_suffixes"][i]
                wav_list.append({
                    "filename": f"{slug}_{voice}_{suf}.wav",
                    "voice": voice,
                    "velocity_layer": i + 1,
                    "render_note": "N/A (trigger voice directly)",
                })
        spec["wav_count"] = len(wav_list)
        spec["wavs"] = wav_list
        spec["render_instructions"] = (
            f"In XOceanus, load preset '{name}'. For each voice (kick, snare, etc.), "
            f"trigger at 4 velocity levels (ghost=10, light=38, medium=73, hard=109) and record "
            f"the output as individual WAV files."
        )

    elif strategy["program_type"] == "stem":
        # Texture stem spec
        spec["duration_seconds"] = strategy.get("duration_seconds", 30)
        spec["wav_count"] = 1
        spec["wavs"] = [{
            "filename": f"{slug}_stem.wav",
            "duration": f"{strategy.get('duration_seconds', 30)}s",
            "render_note": "Play freely — capture the texture",
        }]
        spec["render_instructions"] = (
            f"In XOceanus, load preset '{name}'. Play or hold notes for "
            f"{strategy.get('duration_seconds', 30)} seconds, capturing the evolving "
            f"granular texture. Record as a single stereo WAV."
        )

    else:
        # Keygroup render spec
        notes = strategy.get("notes", MINOR_3RD_NOTES)
        vel_layers = strategy.get("vel_layers", 4)
        variants = strategy.get("variants", [None])

        wav_list = []
        for variant in variants:
            for note in notes:
                for v in range(1, vel_layers + 1):
                    if variant:
                        fname = f"{slug}_{variant}__{note}__v{v}.WAV"
                    else:
                        fname = f"{slug}__{note}__v{v}.WAV"
                    wav_list.append({
                        "filename": fname,
                        "note": note,
                        "velocity_layer": v,
                        "variant": variant,
                    })

        spec["wav_count"] = len(wav_list)
        spec["notes"] = notes
        spec["range"] = strategy.get("range", "C1-C6")
        spec["vel_layers"] = vel_layers
        spec["wavs"] = wav_list

        # Space-aware render tip
        space_note = ""
        if space > 0.7:
            space_note = (
                " This is a spacious preset — render with 2-3s natural tail, "
                "let the user add MPC reverb for more."
            )
        elif space < 0.3:
            space_note = " This is a dry preset — render with no reverb."

        spec["render_instructions"] = (
            f"In XOceanus, load preset '{name}'. "
            f"For each note ({notes[0]}–{notes[-1]}), hold the note for 2 seconds "
            f"and render {vel_layers} velocity levels. "
            f"Name files: {slug}__NOTE__v1.WAV through v{vel_layers}.WAV"
            f"{space_note}"
        )

    return spec


def print_render_spec(spec: dict):
    """Pretty-print a render specification."""
    print(f"\n{'='*60}")
    print(f"  RENDER SPEC: {spec['preset_name']}")
    print(f"  Engine: {spec['engine']}  |  Type: {spec['program_type']}")
    print(f"  Strategy: {spec['strategy_description']}")
    print(f"{'='*60}")
    print(f"\n  WAV files needed: {spec['wav_count']}")

    if spec.get("range"):
        print(f"  Note range: {spec['range']}")
    if spec.get("vel_layers"):
        print(f"  Velocity layers: {spec['vel_layers']}")

    print(f"\n  Instructions:")
    print(f"  {spec['render_instructions']}")

    if spec["wav_count"] <= 40:
        print(f"\n  File list:")
        for w in spec["wavs"]:
            print(f"    {w['filename']}")
    else:
        print(f"\n  First 10 of {spec['wav_count']} files:")
        for w in spec["wavs"][:10]:
            print(f"    {w['filename']}")
        print(f"    ... +{spec['wav_count'] - 10} more")

    print()


# =============================================================================
# CLI
# =============================================================================

def main():
    parser = argparse.ArgumentParser(
        description="XPN Render Spec Generator — .xometa → render instructions",
    )
    src = parser.add_mutually_exclusive_group(required=True)
    src.add_argument("--preset", metavar="PATH",
                     help="Single .xometa file")
    src.add_argument("--engine", metavar="NAME",
                     help="Generate specs for all presets of an engine")
    src.add_argument("--all", action="store_true",
                     help="Generate specs for all presets")

    parser.add_argument("--output-dir", metavar="DIR",
                        help="Save specs as JSON files")
    parser.add_argument("--json", action="store_true",
                        help="Output as JSON to stdout")
    args = parser.parse_args()

    # Validate --engine argument against the shared registry
    if args.engine:
        all_names = get_all_engines()
        match = next(
            (n for n in all_names if n.lower() == args.engine.lower()), None
        )
        if match is None:
            parser.error(
                f"Unknown engine: {args.engine!r}.  "
                f"Valid engines: {', '.join(all_names)}"
            )
        args.engine = match  # normalise to canonical case

    specs = []

    if args.preset:
        spec = generate_render_spec(Path(args.preset))
        specs.append(spec)
    else:
        # Scan preset library
        for mood_dir in sorted(PRESETS_DIR.iterdir()):
            if not mood_dir.is_dir():
                continue
            for engine_dir in sorted(mood_dir.iterdir()):
                if not engine_dir.is_dir():
                    continue
                if args.engine and engine_dir.name.lower() != args.engine.lower():
                    continue
                for xmeta in sorted(engine_dir.glob("*.xometa")):
                    try:
                        spec = generate_render_spec(xmeta)
                        specs.append(spec)
                    except Exception as e:
                        print(f"  [WARN] {xmeta.name}: {e}")

    if args.json:
        print(json.dumps(specs, indent=2))
        return 0

    if args.output_dir:
        out = Path(args.output_dir)
        out.mkdir(parents=True, exist_ok=True)
        for spec in specs:
            fname = f"{spec['preset_slug']}_render_spec.json"
            with open(out / fname, "w") as f:
                json.dump(spec, f, indent=2)
        print(f"Wrote {len(specs)} render specs to {out}")
    else:
        for spec in specs:
            print_render_spec(spec)

    return 0


if __name__ == "__main__":
    sys.exit(main())
