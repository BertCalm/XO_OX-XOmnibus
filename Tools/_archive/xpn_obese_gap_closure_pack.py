#!/usr/bin/env python3
"""
xpn_obese_gap_closure_pack.py
Generate Entangled presets for all missing OBESE engine pairs.
OBESE character: Hot Pink #FF1493, extreme bass/saturation synth,
B015 MOJO Control (analog/digital axis), fat_ prefix.
DNA: high density (0.7-1.0), high warmth (0.6-0.9), variable aggression.
"""

import json
import os

_REPO_ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
ENTANGLED_DIR = os.path.join(_REPO_ROOT, "Presets", "XOceanus", "Entangled")

PRESETS = [
    # ── ORGANON (pipe organ harmonics) ─────────────────────────────────────
    {
        "filename": "Obese_Organon_Fat_Pipe.xometa",
        "name": "Fat Pipe",
        "engines": ["OBESE", "ORGANON"],
        "parameters": {
            "OBESE":   {"macro_character": 0.9, "macro_movement": 0.3, "macro_coupling": 0.8, "macro_space": 0.15},
            "ORGANON": {"macro_character": 0.6, "macro_movement": 0.4, "macro_coupling": 0.7, "macro_space": 0.5}
        },
        "coupling": {"type": "AMPLITUDE_MOD", "source": "OBESE", "target": "ORGANON", "amount": 0.82},
        "dna": {"brightness": 0.2, "warmth": 0.85, "movement": 0.35, "density": 0.95, "space": 0.2, "aggression": 0.8},
        "macros": {"CHARACTER": 0.9, "MOVEMENT": 0.3, "COUPLING": 0.8, "SPACE": 0.15},
        "tags": ["entangled", "obese", "organon", "bass", "organ", "fat"]
    },
    {
        "filename": "Obese_Organon_Mojo_Liturgy.xometa",
        "name": "Mojo Liturgy",
        "engines": ["OBESE", "ORGANON"],
        "parameters": {
            "OBESE":   {"macro_character": 0.75, "macro_movement": 0.6, "macro_coupling": 0.65, "macro_space": 0.3},
            "ORGANON": {"macro_character": 0.8, "macro_movement": 0.5, "macro_coupling": 0.6, "macro_space": 0.6}
        },
        "coupling": {"type": "FREQUENCY_SHIFT", "source": "ORGANON", "target": "OBESE", "amount": 0.55},
        "dna": {"brightness": 0.35, "warmth": 0.75, "movement": 0.55, "density": 0.85, "space": 0.4, "aggression": 0.65},
        "macros": {"CHARACTER": 0.75, "MOVEMENT": 0.6, "COUPLING": 0.65, "SPACE": 0.3},
        "tags": ["entangled", "obese", "organon", "bass", "organ", "mojo"]
    },

    # ── OUROBOROS (self-modulating feedback) ───────────────────────────────
    {
        "filename": "Obese_Ouroboros_Fat_Loop.xometa",
        "name": "Fat Loop",
        "engines": ["OBESE", "OUROBOROS"],
        "parameters": {
            "OBESE":     {"macro_character": 0.95, "macro_movement": 0.7, "macro_coupling": 0.9, "macro_space": 0.1},
            "OUROBOROS": {"macro_character": 0.85, "macro_movement": 0.8, "macro_coupling": 0.85, "macro_space": 0.2}
        },
        "coupling": {"type": "FEEDBACK_RING", "source": "OUROBOROS", "target": "OBESE", "amount": 0.88},
        "dna": {"brightness": 0.15, "warmth": 0.9, "movement": 0.75, "density": 1.0, "space": 0.1, "aggression": 0.95},
        "macros": {"CHARACTER": 0.95, "MOVEMENT": 0.7, "COUPLING": 0.9, "SPACE": 0.1},
        "tags": ["entangled", "obese", "ouroboros", "bass", "feedback", "extreme", "fat"]
    },
    {
        "filename": "Obese_Ouroboros_Mojo_Serpent.xometa",
        "name": "Mojo Serpent",
        "engines": ["OBESE", "OUROBOROS"],
        "parameters": {
            "OBESE":     {"macro_character": 0.8, "macro_movement": 0.55, "macro_coupling": 0.7, "macro_space": 0.25},
            "OUROBOROS": {"macro_character": 0.7, "macro_movement": 0.65, "macro_coupling": 0.75, "macro_space": 0.3}
        },
        "coupling": {"type": "AMPLITUDE_MOD", "source": "OBESE", "target": "OUROBOROS", "amount": 0.7},
        "dna": {"brightness": 0.25, "warmth": 0.8, "movement": 0.65, "density": 0.9, "space": 0.2, "aggression": 0.85},
        "macros": {"CHARACTER": 0.8, "MOVEMENT": 0.55, "COUPLING": 0.7, "SPACE": 0.25},
        "tags": ["entangled", "obese", "ouroboros", "bass", "mojo", "serpent"]
    },

    # ── ORIGAMI (fold/crease modulation) ───────────────────────────────────
    {
        "filename": "Obese_Origami_Fat_Fold.xometa",
        "name": "Fat Fold",
        "engines": ["OBESE", "ORIGAMI"],
        "parameters": {
            "OBESE":   {"macro_character": 0.85, "macro_movement": 0.4, "macro_coupling": 0.75, "macro_space": 0.2},
            "ORIGAMI": {"macro_character": 0.55, "macro_movement": 0.7, "macro_coupling": 0.65, "macro_space": 0.45}
        },
        "coupling": {"type": "WAVEFOLD", "source": "OBESE", "target": "ORIGAMI", "amount": 0.78},
        "dna": {"brightness": 0.3, "warmth": 0.8, "movement": 0.6, "density": 0.88, "space": 0.25, "aggression": 0.78},
        "macros": {"CHARACTER": 0.85, "MOVEMENT": 0.4, "COUPLING": 0.75, "SPACE": 0.2},
        "tags": ["entangled", "obese", "origami", "bass", "wavefold", "fat"]
    },
    {
        "filename": "Obese_Origami_Mojo_Crease.xometa",
        "name": "Mojo Crease",
        "engines": ["OBESE", "ORIGAMI"],
        "parameters": {
            "OBESE":   {"macro_character": 0.7, "macro_movement": 0.5, "macro_coupling": 0.6, "macro_space": 0.35},
            "ORIGAMI": {"macro_character": 0.75, "macro_movement": 0.6, "macro_coupling": 0.55, "macro_space": 0.55}
        },
        "coupling": {"type": "FREQUENCY_SHIFT", "source": "ORIGAMI", "target": "OBESE", "amount": 0.6},
        "dna": {"brightness": 0.4, "warmth": 0.7, "movement": 0.55, "density": 0.8, "space": 0.35, "aggression": 0.65},
        "macros": {"CHARACTER": 0.7, "MOVEMENT": 0.5, "COUPLING": 0.6, "SPACE": 0.35},
        "tags": ["entangled", "obese", "origami", "bass", "mojo", "crease"]
    },

    # ── ORACLE (spectral prediction) ───────────────────────────────────────
    {
        "filename": "Obese_Oracle_Fat_Vision.xometa",
        "name": "Fat Vision",
        "engines": ["OBESE", "ORACLE"],
        "parameters": {
            "OBESE":  {"macro_character": 0.9, "macro_movement": 0.45, "macro_coupling": 0.8, "macro_space": 0.15},
            "ORACLE": {"macro_character": 0.5, "macro_movement": 0.55, "macro_coupling": 0.7, "macro_space": 0.6}
        },
        "coupling": {"type": "SPECTRAL_BLEND", "source": "ORACLE", "target": "OBESE", "amount": 0.65},
        "dna": {"brightness": 0.25, "warmth": 0.85, "movement": 0.45, "density": 0.92, "space": 0.3, "aggression": 0.8},
        "macros": {"CHARACTER": 0.9, "MOVEMENT": 0.45, "COUPLING": 0.8, "SPACE": 0.15},
        "tags": ["entangled", "obese", "oracle", "bass", "spectral", "fat"]
    },
    {
        "filename": "Obese_Oracle_Mojo_Prophecy.xometa",
        "name": "Mojo Prophecy",
        "engines": ["OBESE", "ORACLE"],
        "parameters": {
            "OBESE":  {"macro_character": 0.75, "macro_movement": 0.6, "macro_coupling": 0.65, "macro_space": 0.3},
            "ORACLE": {"macro_character": 0.65, "macro_movement": 0.7, "macro_coupling": 0.6, "macro_space": 0.5}
        },
        "coupling": {"type": "AMPLITUDE_MOD", "source": "OBESE", "target": "ORACLE", "amount": 0.72},
        "dna": {"brightness": 0.35, "warmth": 0.75, "movement": 0.6, "density": 0.82, "space": 0.35, "aggression": 0.7},
        "macros": {"CHARACTER": 0.75, "MOVEMENT": 0.6, "COUPLING": 0.65, "SPACE": 0.3},
        "tags": ["entangled", "obese", "oracle", "bass", "mojo", "prophecy"]
    },

    # ── OBSCURA (dark spectral) ─────────────────────────────────────────────
    {
        "filename": "Obese_Obscura_Fat_Dark.xometa",
        "name": "Fat Dark",
        "engines": ["OBESE", "OBSCURA"],
        "parameters": {
            "OBESE":   {"macro_character": 0.92, "macro_movement": 0.35, "macro_coupling": 0.85, "macro_space": 0.1},
            "OBSCURA": {"macro_character": 0.88, "macro_movement": 0.4, "macro_coupling": 0.8, "macro_space": 0.2}
        },
        "coupling": {"type": "RING_MOD", "source": "OBESE", "target": "OBSCURA", "amount": 0.9},
        "dna": {"brightness": 0.05, "warmth": 0.9, "movement": 0.35, "density": 1.0, "space": 0.1, "aggression": 0.95},
        "macros": {"CHARACTER": 0.92, "MOVEMENT": 0.35, "COUPLING": 0.85, "SPACE": 0.1},
        "tags": ["entangled", "obese", "obscura", "bass", "dark", "extreme", "fat"]
    },
    {
        "filename": "Obese_Obscura_Mojo_Void.xometa",
        "name": "Mojo Void",
        "engines": ["OBESE", "OBSCURA"],
        "parameters": {
            "OBESE":   {"macro_character": 0.78, "macro_movement": 0.5, "macro_coupling": 0.7, "macro_space": 0.25},
            "OBSCURA": {"macro_character": 0.8, "macro_movement": 0.45, "macro_coupling": 0.65, "macro_space": 0.3}
        },
        "coupling": {"type": "FREQUENCY_SHIFT", "source": "OBSCURA", "target": "OBESE", "amount": 0.68},
        "dna": {"brightness": 0.1, "warmth": 0.85, "movement": 0.45, "density": 0.9, "space": 0.2, "aggression": 0.88},
        "macros": {"CHARACTER": 0.78, "MOVEMENT": 0.5, "COUPLING": 0.7, "SPACE": 0.25},
        "tags": ["entangled", "obese", "obscura", "bass", "mojo", "void"]
    },

    # ── OBLONG (stretched/detuned) ─────────────────────────────────────────
    {
        "filename": "Obese_Oblong_Fat_Stretch.xometa",
        "name": "Fat Stretch",
        "engines": ["OBESE", "OBLONG"],
        "parameters": {
            "OBESE":  {"macro_character": 0.88, "macro_movement": 0.5, "macro_coupling": 0.78, "macro_space": 0.2},
            "OBLONG": {"macro_character": 0.7, "macro_movement": 0.6, "macro_coupling": 0.65, "macro_space": 0.35}
        },
        "coupling": {"type": "AMPLITUDE_MOD", "source": "OBESE", "target": "OBLONG", "amount": 0.8},
        "dna": {"brightness": 0.2, "warmth": 0.82, "movement": 0.5, "density": 0.93, "space": 0.25, "aggression": 0.83},
        "macros": {"CHARACTER": 0.88, "MOVEMENT": 0.5, "COUPLING": 0.78, "SPACE": 0.2},
        "tags": ["entangled", "obese", "oblong", "bass", "stretch", "fat"]
    },
    {
        "filename": "Obese_Oblong_Mojo_Warp.xometa",
        "name": "Mojo Warp",
        "engines": ["OBESE", "OBLONG"],
        "parameters": {
            "OBESE":  {"macro_character": 0.72, "macro_movement": 0.65, "macro_coupling": 0.6, "macro_space": 0.3},
            "OBLONG": {"macro_character": 0.8, "macro_movement": 0.55, "macro_coupling": 0.7, "macro_space": 0.4}
        },
        "coupling": {"type": "PITCH_ENTANGLE", "source": "OBLONG", "target": "OBESE", "amount": 0.62},
        "dna": {"brightness": 0.3, "warmth": 0.72, "movement": 0.6, "density": 0.85, "space": 0.3, "aggression": 0.72},
        "macros": {"CHARACTER": 0.72, "MOVEMENT": 0.65, "COUPLING": 0.6, "SPACE": 0.3},
        "tags": ["entangled", "obese", "oblong", "bass", "mojo", "warp"]
    },

    # ── OVERDUB (tape delay character) ─────────────────────────────────────
    {
        "filename": "Obese_Overdub_Fat_Tape.xometa",
        "name": "Fat Tape",
        "engines": ["OBESE", "OVERDUB"],
        "parameters": {
            "OBESE":   {"macro_character": 0.85, "macro_movement": 0.55, "macro_coupling": 0.75, "macro_space": 0.3},
            "OVERDUB": {"macro_character": 0.65, "macro_movement": 0.7, "macro_coupling": 0.7, "macro_space": 0.55}
        },
        "coupling": {"type": "AMPLITUDE_MOD", "source": "OBESE", "target": "OVERDUB", "amount": 0.75},
        "dna": {"brightness": 0.3, "warmth": 0.88, "movement": 0.6, "density": 0.88, "space": 0.4, "aggression": 0.72},
        "macros": {"CHARACTER": 0.85, "MOVEMENT": 0.55, "COUPLING": 0.75, "SPACE": 0.3},
        "tags": ["entangled", "obese", "overdub", "bass", "tape", "fat"]
    },
    {
        "filename": "Obese_Overdub_Mojo_Dub.xometa",
        "name": "Mojo Dub",
        "engines": ["OBESE", "OVERDUB"],
        "parameters": {
            "OBESE":   {"macro_character": 0.9, "macro_movement": 0.4, "macro_coupling": 0.85, "macro_space": 0.2},
            "OVERDUB": {"macro_character": 0.55, "macro_movement": 0.75, "macro_coupling": 0.65, "macro_space": 0.7}
        },
        "coupling": {"type": "SIDECHAIN_DUCK", "source": "OBESE", "target": "OVERDUB", "amount": 0.85},
        "dna": {"brightness": 0.2, "warmth": 0.9, "movement": 0.55, "density": 0.95, "space": 0.5, "aggression": 0.8},
        "macros": {"CHARACTER": 0.9, "MOVEMENT": 0.4, "COUPLING": 0.85, "SPACE": 0.2},
        "tags": ["entangled", "obese", "overdub", "bass", "mojo", "dub"]
    },

    # ── OVERBITE (bass-forward character) ──────────────────────────────────
    {
        "filename": "Obese_Overbite_Fat_Fang.xometa",
        "name": "Fat Fang",
        "engines": ["OBESE", "OVERBITE"],
        "parameters": {
            "OBESE":    {"macro_character": 0.95, "macro_movement": 0.5, "macro_coupling": 0.88, "macro_space": 0.1},
            "OVERBITE": {"macro_character": 0.9, "macro_movement": 0.55, "macro_coupling": 0.82, "macro_space": 0.15}
        },
        "coupling": {"type": "RING_MOD", "source": "OVERBITE", "target": "OBESE", "amount": 0.92},
        "dna": {"brightness": 0.1, "warmth": 0.92, "movement": 0.5, "density": 1.0, "space": 0.1, "aggression": 0.98},
        "macros": {"CHARACTER": 0.95, "MOVEMENT": 0.5, "COUPLING": 0.88, "SPACE": 0.1},
        "tags": ["entangled", "obese", "overbite", "bass", "extreme", "fat", "fang"]
    },
    {
        "filename": "Obese_Overbite_Mojo_Bite.xometa",
        "name": "Mojo Bite",
        "engines": ["OBESE", "OVERBITE"],
        "parameters": {
            "OBESE":    {"macro_character": 0.8, "macro_movement": 0.6, "macro_coupling": 0.7, "macro_space": 0.25},
            "OVERBITE": {"macro_character": 0.75, "macro_movement": 0.65, "macro_coupling": 0.68, "macro_space": 0.2}
        },
        "coupling": {"type": "AMPLITUDE_MOD", "source": "OBESE", "target": "OVERBITE", "amount": 0.78},
        "dna": {"brightness": 0.2, "warmth": 0.82, "movement": 0.6, "density": 0.9, "space": 0.2, "aggression": 0.88},
        "macros": {"CHARACTER": 0.8, "MOVEMENT": 0.6, "COUPLING": 0.7, "SPACE": 0.25},
        "tags": ["entangled", "obese", "overbite", "bass", "mojo", "bite"]
    },

    # ── OPAL (granular) ────────────────────────────────────────────────────
    {
        "filename": "Obese_Opal_Fat_Grain.xometa",
        "name": "Fat Grain",
        "engines": ["OBESE", "OPAL"],
        "parameters": {
            "OBESE": {"macro_character": 0.88, "macro_movement": 0.45, "macro_coupling": 0.78, "macro_space": 0.2},
            "OPAL":  {"macro_character": 0.5, "macro_movement": 0.75, "macro_coupling": 0.65, "macro_space": 0.65}
        },
        "coupling": {"type": "GRANULAR_SYNC", "source": "OBESE", "target": "OPAL", "amount": 0.7},
        "dna": {"brightness": 0.35, "warmth": 0.78, "movement": 0.65, "density": 0.87, "space": 0.45, "aggression": 0.72},
        "macros": {"CHARACTER": 0.88, "MOVEMENT": 0.45, "COUPLING": 0.78, "SPACE": 0.2},
        "tags": ["entangled", "obese", "opal", "bass", "granular", "fat"]
    },
    {
        "filename": "Obese_Opal_Mojo_Cloud.xometa",
        "name": "Mojo Cloud",
        "engines": ["OBESE", "OPAL"],
        "parameters": {
            "OBESE": {"macro_character": 0.75, "macro_movement": 0.6, "macro_coupling": 0.65, "macro_space": 0.35},
            "OPAL":  {"macro_character": 0.6, "macro_movement": 0.8, "macro_coupling": 0.6, "macro_space": 0.75}
        },
        "coupling": {"type": "SIDECHAIN_DUCK", "source": "OBESE", "target": "OPAL", "amount": 0.65},
        "dna": {"brightness": 0.45, "warmth": 0.7, "movement": 0.75, "density": 0.78, "space": 0.6, "aggression": 0.6},
        "macros": {"CHARACTER": 0.75, "MOVEMENT": 0.6, "COUPLING": 0.65, "SPACE": 0.35},
        "tags": ["entangled", "obese", "opal", "bass", "mojo", "granular", "cloud"]
    },

    # ── ONSET (drum synthesis) ─────────────────────────────────────────────
    {
        "filename": "Obese_Onset_Fat_Kick.xometa",
        "name": "Fat Kick",
        "engines": ["OBESE", "ONSET"],
        "parameters": {
            "OBESE": {"macro_character": 0.92, "macro_movement": 0.4, "macro_coupling": 0.85, "macro_space": 0.1},
            "ONSET": {"macro_character": 0.85, "macro_movement": 0.55, "macro_coupling": 0.75, "macro_space": 0.2}
        },
        "coupling": {"type": "AMPLITUDE_MOD", "source": "ONSET", "target": "OBESE", "amount": 0.88},
        "dna": {"brightness": 0.15, "warmth": 0.88, "movement": 0.4, "density": 0.97, "space": 0.15, "aggression": 0.92},
        "macros": {"CHARACTER": 0.92, "MOVEMENT": 0.4, "COUPLING": 0.85, "SPACE": 0.1},
        "tags": ["entangled", "obese", "onset", "bass", "drums", "kick", "fat"]
    },
    {
        "filename": "Obese_Onset_Mojo_Machine.xometa",
        "name": "Mojo Machine",
        "engines": ["OBESE", "ONSET"],
        "parameters": {
            "OBESE": {"macro_character": 0.8, "macro_movement": 0.65, "macro_coupling": 0.72, "macro_space": 0.25},
            "ONSET": {"macro_character": 0.78, "macro_movement": 0.7, "macro_coupling": 0.68, "macro_space": 0.3}
        },
        "coupling": {"type": "SIDECHAIN_DUCK", "source": "ONSET", "target": "OBESE", "amount": 0.82},
        "dna": {"brightness": 0.25, "warmth": 0.8, "movement": 0.65, "density": 0.9, "space": 0.2, "aggression": 0.85},
        "macros": {"CHARACTER": 0.8, "MOVEMENT": 0.65, "COUPLING": 0.72, "SPACE": 0.25},
        "tags": ["entangled", "obese", "onset", "bass", "mojo", "machine"]
    },

    # ── OVERWORLD (chip synth) ─────────────────────────────────────────────
    {
        "filename": "Obese_Overworld_Fat_Chip.xometa",
        "name": "Fat Chip",
        "engines": ["OBESE", "OVERWORLD"],
        "parameters": {
            "OBESE":     {"macro_character": 0.9, "macro_movement": 0.5, "macro_coupling": 0.82, "macro_space": 0.15},
            "OVERWORLD": {"macro_character": 0.6, "macro_movement": 0.7, "macro_coupling": 0.65, "macro_space": 0.4}
        },
        "coupling": {"type": "RING_MOD", "source": "OVERWORLD", "target": "OBESE", "amount": 0.75},
        "dna": {"brightness": 0.55, "warmth": 0.72, "movement": 0.65, "density": 0.85, "space": 0.25, "aggression": 0.78},
        "macros": {"CHARACTER": 0.9, "MOVEMENT": 0.5, "COUPLING": 0.82, "SPACE": 0.15},
        "tags": ["entangled", "obese", "overworld", "bass", "chip", "fat"]
    },
    {
        "filename": "Obese_Overworld_Mojo_NES.xometa",
        "name": "Mojo NES",
        "engines": ["OBESE", "OVERWORLD"],
        "parameters": {
            "OBESE":     {"macro_character": 0.75, "macro_movement": 0.55, "macro_coupling": 0.68, "macro_space": 0.3},
            "OVERWORLD": {"macro_character": 0.72, "macro_movement": 0.65, "macro_coupling": 0.6, "macro_space": 0.5}
        },
        "coupling": {"type": "FREQUENCY_SHIFT", "source": "OVERWORLD", "target": "OBESE", "amount": 0.6},
        "dna": {"brightness": 0.65, "warmth": 0.65, "movement": 0.6, "density": 0.8, "space": 0.3, "aggression": 0.7},
        "macros": {"CHARACTER": 0.75, "MOVEMENT": 0.55, "COUPLING": 0.68, "SPACE": 0.3},
        "tags": ["entangled", "obese", "overworld", "bass", "mojo", "chip", "nes"]
    },

    # ── ORBITAL (orbital modulation) ───────────────────────────────────────
    {
        "filename": "Obese_Orbital_Fat_Orbit.xometa",
        "name": "Fat Orbit",
        "engines": ["OBESE", "ORBITAL"],
        "parameters": {
            "OBESE":   {"macro_character": 0.88, "macro_movement": 0.55, "macro_coupling": 0.8, "macro_space": 0.2},
            "ORBITAL": {"macro_character": 0.6, "macro_movement": 0.72, "macro_coupling": 0.68, "macro_space": 0.5}
        },
        "coupling": {"type": "AMPLITUDE_MOD", "source": "ORBITAL", "target": "OBESE", "amount": 0.77},
        "dna": {"brightness": 0.3, "warmth": 0.8, "movement": 0.65, "density": 0.88, "space": 0.35, "aggression": 0.75},
        "macros": {"CHARACTER": 0.88, "MOVEMENT": 0.55, "COUPLING": 0.8, "SPACE": 0.2},
        "tags": ["entangled", "obese", "orbital", "bass", "orbit", "fat"]
    },
    {
        "filename": "Obese_Orbital_Mojo_Gravity.xometa",
        "name": "Mojo Gravity",
        "engines": ["OBESE", "ORBITAL"],
        "parameters": {
            "OBESE":   {"macro_character": 0.78, "macro_movement": 0.5, "macro_coupling": 0.7, "macro_space": 0.28},
            "ORBITAL": {"macro_character": 0.65, "macro_movement": 0.78, "macro_coupling": 0.62, "macro_space": 0.6}
        },
        "coupling": {"type": "PITCH_ENTANGLE", "source": "ORBITAL", "target": "OBESE", "amount": 0.65},
        "dna": {"brightness": 0.35, "warmth": 0.75, "movement": 0.72, "density": 0.82, "space": 0.4, "aggression": 0.68},
        "macros": {"CHARACTER": 0.78, "MOVEMENT": 0.5, "COUPLING": 0.7, "SPACE": 0.28},
        "tags": ["entangled", "obese", "orbital", "bass", "mojo", "gravity"]
    },

    # ── OBSIDIAN (volcanic dark) ───────────────────────────────────────────
    {
        "filename": "Obese_Obsidian_Fat_Volcanic.xometa",
        "name": "Fat Volcanic",
        "engines": ["OBESE", "OBSIDIAN"],
        "parameters": {
            "OBESE":    {"macro_character": 0.95, "macro_movement": 0.35, "macro_coupling": 0.9, "macro_space": 0.08},
            "OBSIDIAN": {"macro_character": 0.92, "macro_movement": 0.3, "macro_coupling": 0.88, "macro_space": 0.12}
        },
        "coupling": {"type": "RING_MOD", "source": "OBESE", "target": "OBSIDIAN", "amount": 0.95},
        "dna": {"brightness": 0.05, "warmth": 0.92, "movement": 0.3, "density": 1.0, "space": 0.08, "aggression": 1.0},
        "macros": {"CHARACTER": 0.95, "MOVEMENT": 0.35, "COUPLING": 0.9, "SPACE": 0.08},
        "tags": ["entangled", "obese", "obsidian", "bass", "dark", "extreme", "volcanic", "fat"]
    },
    {
        "filename": "Obese_Obsidian_Mojo_Lava.xometa",
        "name": "Mojo Lava",
        "engines": ["OBESE", "OBSIDIAN"],
        "parameters": {
            "OBESE":    {"macro_character": 0.82, "macro_movement": 0.48, "macro_coupling": 0.75, "macro_space": 0.2},
            "OBSIDIAN": {"macro_character": 0.85, "macro_movement": 0.4, "macro_coupling": 0.78, "macro_space": 0.18}
        },
        "coupling": {"type": "FREQUENCY_SHIFT", "source": "OBSIDIAN", "target": "OBESE", "amount": 0.72},
        "dna": {"brightness": 0.12, "warmth": 0.88, "movement": 0.42, "density": 0.95, "space": 0.15, "aggression": 0.92},
        "macros": {"CHARACTER": 0.82, "MOVEMENT": 0.48, "COUPLING": 0.75, "SPACE": 0.2},
        "tags": ["entangled", "obese", "obsidian", "bass", "mojo", "lava"]
    },
]


def write_preset(preset_def: dict) -> tuple[str, bool]:
    """Write a single preset. Returns (filename, was_written)."""
    filename = preset_def["filename"]
    filepath = os.path.join(ENTANGLED_DIR, filename)

    if os.path.exists(filepath):
        return filename, False

    data = {
        "name": preset_def["name"],
        "version": "1.0",
        "mood": "Entangled",
        "engines": preset_def["engines"],
        "parameters": preset_def["parameters"],
        "coupling": preset_def["coupling"],
        "dna": preset_def["dna"],
        "macros": preset_def["macros"],
        "tags": preset_def["tags"],
    }

    with open(filepath, "w") as f:
        json.dump(data, f, indent=2)

    return filename, True


def main():
    os.makedirs(ENTANGLED_DIR, exist_ok=True)
    written = []
    skipped = []

    for preset in PRESETS:
        filename, was_written = write_preset(preset)
        if was_written:
            written.append(filename)
        else:
            skipped.append(filename)

    print(f"\n=== OBESE Gap Closure Pack ===")
    print(f"Written : {len(written)}")
    print(f"Skipped (already exist): {len(skipped)}")

    if written:
        print("\nNew presets:")
        for f in written:
            print(f"  + {f}")

    if skipped:
        print("\nSkipped (already existed):")
        for f in skipped:
            print(f"  ~ {f}")

    # Report coverage
    covered_partners = set()
    for p in PRESETS:
        for eng in p["engines"]:
            if eng != "OBESE":
                covered_partners.add(eng)

    print(f"\nPartners now covered by this pack ({len(covered_partners)}):")
    for p in sorted(covered_partners):
        print(f"  {p}")


if __name__ == "__main__":
    main()
