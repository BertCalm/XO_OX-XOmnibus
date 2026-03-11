#!/usr/bin/env python3
"""Generate XOuroboros .xometa preset files for XOmnibus.

Ouroboros is a chaotic attractor oscillator with 4 topologies (Lorenz, Rössler,
Chua, Aizawa). Only 8 parameters but immense sonic range from stable tones to
screaming chaos.

Generates 30 presets across all 6 moods:
  - Stable Orbits (8): Low chaos, musical tones
  - Bifurcation Edge (8): Mid chaos, complex timbres
  - Strange Attractors (6): High chaos, aggressive textures
  - Projection Walks (4): Theta/phi animation, spatial movement
  - Coupling Showcases (4): Cross-engine interaction
"""

import json
import math
import os
import random

random.seed(77)

PRESET_DIR = os.path.join(os.path.dirname(os.path.dirname(__file__)), "Presets", "XOmnibus")

DEFAULTS = {
    "ouro_topology": 0,
    "ouro_rate": 130.0,
    "ouro_chaosIndex": 0.3,
    "ouro_leash": 0.5,
    "ouro_theta": 0.0,
    "ouro_phi": 0.0,
    "ouro_damping": 0.3,
    "ouro_injection": 0.0,
}


def r(lo, hi):
    return round(random.uniform(lo, hi), 3)


def make_params(**overrides):
    p = dict(DEFAULTS)
    p.update(overrides)
    return p


def make_preset(name, mood, desc, tags, params, dna, engines=None,
                coupling_intensity="None", coupling=None):
    return {
        "schema_version": 1,
        "name": name,
        "mood": mood,
        "engines": engines or ["Ouroboros"],
        "author": "XO_OX Designs",
        "version": "1.0.0",
        "description": desc,
        "tags": tags,
        "macroLabels": ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"],
        "couplingIntensity": coupling_intensity,
        "tempo": None,
        "dna": dna,
        "parameters": {"Ouroboros": params},
        "coupling": coupling,
        "sequencer": None,
    }


def write_preset(preset):
    mood = preset["mood"]
    mood_dir = os.path.join(PRESET_DIR, mood)
    os.makedirs(mood_dir, exist_ok=True)
    filename = preset["name"] + ".xometa"
    filepath = os.path.join(mood_dir, filename)
    with open(filepath, "w") as f:
        json.dump(preset, f, indent=2)
        f.write("\n")
    return filepath


PRESETS = [
    # ---- Stable Orbits (8) ----
    ("Lorenz Tone", "Foundation", "Stable Lorenz orbit. Musical, slightly metallic tone.",
     ["chaos", "stable", "lorenz", "tone"],
     {"ouro_topology": 0, "ouro_rate": 220.0, "ouro_chaosIndex": 0.1,
      "ouro_leash": 0.7, "ouro_damping": 0.5},
     {"brightness": 0.5, "warmth": 0.5, "movement": 0.15, "density": 0.3, "space": 0.05, "aggression": 0.1}),

    ("Aizawa Hum", "Foundation", "Aizawa attractor at low chaos. Warm complex fundamental.",
     ["chaos", "warm", "aizawa", "hum"],
     {"ouro_topology": 3, "ouro_rate": 110.0, "ouro_chaosIndex": 0.08,
      "ouro_leash": 0.8, "ouro_damping": 0.6},
     {"brightness": 0.3, "warmth": 0.7, "movement": 0.1, "density": 0.3, "space": 0.0, "aggression": 0.05}),

    ("Chua Whistle", "Prism", "Chua circuit at near-periodic orbit. Eerie whistle.",
     ["chaos", "eerie", "chua", "whistle"],
     {"ouro_topology": 2, "ouro_rate": 440.0, "ouro_chaosIndex": 0.15,
      "ouro_leash": 0.6, "ouro_damping": 0.4, "ouro_theta": 0.5},
     {"brightness": 0.6, "warmth": 0.3, "movement": 0.2, "density": 0.2, "space": 0.1, "aggression": 0.15}),

    ("Orbit Drone", "Atmosphere", "Low Lorenz orbit with high damping. Deep slow drone.",
     ["chaos", "drone", "deep", "slow"],
     {"ouro_topology": 0, "ouro_rate": 55.0, "ouro_chaosIndex": 0.12,
      "ouro_leash": 0.9, "ouro_damping": 0.8},
     {"brightness": 0.15, "warmth": 0.8, "movement": 0.15, "density": 0.2, "space": 0.3, "aggression": 0.0}),

    ("Rossler Bell", "Prism", "Rössler at low chaos. Bell-like inharmonic partials.",
     ["chaos", "bell", "rossler", "inharmonic"],
     {"ouro_topology": 1, "ouro_rate": 330.0, "ouro_chaosIndex": 0.1,
      "ouro_leash": 0.5, "ouro_damping": 0.3, "ouro_theta": 0.8},
     {"brightness": 0.7, "warmth": 0.3, "movement": 0.2, "density": 0.3, "space": 0.15, "aggression": 0.1}),

    ("Phase Lock", "Foundation", "Lorenz fully leashed. Clean pitched chaos oscillator.",
     ["chaos", "pitched", "clean", "locked"],
     {"ouro_topology": 0, "ouro_rate": 261.6, "ouro_chaosIndex": 0.2,
      "ouro_leash": 1.0, "ouro_damping": 0.4},
     {"brightness": 0.5, "warmth": 0.4, "movement": 0.1, "density": 0.3, "space": 0.0, "aggression": 0.2}),

    ("Gentle Attractor", "Atmosphere", "Aizawa with heavy damping. Soft, breathing tone.",
     ["chaos", "gentle", "soft", "breathing"],
     {"ouro_topology": 3, "ouro_rate": 165.0, "ouro_chaosIndex": 0.15,
      "ouro_leash": 0.6, "ouro_damping": 0.85, "ouro_phi": 0.3},
     {"brightness": 0.25, "warmth": 0.75, "movement": 0.25, "density": 0.2, "space": 0.2, "aggression": 0.0}),

    ("Crystal Orbit", "Prism", "Chua at precise bifurcation point. Shimmering periodicity.",
     ["chaos", "crystal", "precise", "shimmer"],
     {"ouro_topology": 2, "ouro_rate": 523.0, "ouro_chaosIndex": 0.18,
      "ouro_leash": 0.7, "ouro_damping": 0.25, "ouro_theta": 1.2, "ouro_phi": 0.5},
     {"brightness": 0.75, "warmth": 0.2, "movement": 0.3, "density": 0.3, "space": 0.1, "aggression": 0.15}),

    # ---- Bifurcation Edge (8) ----
    ("Edge Walker", "Entangled", "Lorenz at bifurcation edge. Unpredictable timbral shifts.",
     ["chaos", "edge", "unpredictable", "shifting"],
     {"ouro_topology": 0, "ouro_rate": 200.0, "ouro_chaosIndex": 0.5,
      "ouro_leash": 0.4, "ouro_damping": 0.35},
     {"brightness": 0.5, "warmth": 0.35, "movement": 0.6, "density": 0.4, "space": 0.1, "aggression": 0.3}),

    ("Chua Circuit", "Entangled", "Chua at mid-chaos. Double-scroll attractor timbres.",
     ["chaos", "chua", "circuit", "complex"],
     {"ouro_topology": 2, "ouro_rate": 300.0, "ouro_chaosIndex": 0.45,
      "ouro_leash": 0.35, "ouro_damping": 0.3},
     {"brightness": 0.55, "warmth": 0.3, "movement": 0.55, "density": 0.45, "space": 0.05, "aggression": 0.35}),

    ("Rossler Fold", "Entangled", "Rössler period-doubling cascade. Rich evolving harmonics.",
     ["chaos", "rossler", "evolving", "harmonics"],
     {"ouro_topology": 1, "ouro_rate": 180.0, "ouro_chaosIndex": 0.55,
      "ouro_leash": 0.3, "ouro_damping": 0.4, "ouro_theta": -0.5},
     {"brightness": 0.45, "warmth": 0.4, "movement": 0.6, "density": 0.5, "space": 0.1, "aggression": 0.25}),

    ("Aizawa Morph", "Entangled", "Aizawa near transition. Sound morphs between states.",
     ["chaos", "aizawa", "morph", "transition"],
     {"ouro_topology": 3, "ouro_rate": 150.0, "ouro_chaosIndex": 0.48,
      "ouro_leash": 0.45, "ouro_damping": 0.5, "ouro_phi": 0.7},
     {"brightness": 0.4, "warmth": 0.5, "movement": 0.55, "density": 0.4, "space": 0.15, "aggression": 0.2}),

    ("Butterfly Effect", "Entangled", "Lorenz at the butterfly wing. Sensitive to everything.",
     ["chaos", "lorenz", "sensitive", "butterfly"],
     {"ouro_topology": 0, "ouro_rate": 250.0, "ouro_chaosIndex": 0.42,
      "ouro_leash": 0.25, "ouro_damping": 0.3, "ouro_theta": 0.3, "ouro_phi": -0.3},
     {"brightness": 0.5, "warmth": 0.35, "movement": 0.7, "density": 0.4, "space": 0.1, "aggression": 0.3}),

    ("Period Three", "Prism", "Chua at period-3 window. Strange musical regularity.",
     ["chaos", "periodic", "strange", "musical"],
     {"ouro_topology": 2, "ouro_rate": 380.0, "ouro_chaosIndex": 0.35,
      "ouro_leash": 0.5, "ouro_damping": 0.3, "ouro_theta": 0.9},
     {"brightness": 0.6, "warmth": 0.3, "movement": 0.45, "density": 0.35, "space": 0.1, "aggression": 0.25}),

    ("Torus Knot", "Prism", "Rössler quasi-periodic orbit. Interleaving patterns.",
     ["chaos", "rossler", "pattern", "quasi-periodic"],
     {"ouro_topology": 1, "ouro_rate": 280.0, "ouro_chaosIndex": 0.38,
      "ouro_leash": 0.55, "ouro_damping": 0.35, "ouro_phi": 1.0},
     {"brightness": 0.55, "warmth": 0.35, "movement": 0.5, "density": 0.4, "space": 0.1, "aggression": 0.2}),

    ("Leash Tension", "Flux", "Lorenz fighting the leash. Tension between order and chaos.",
     ["chaos", "tension", "fighting", "dramatic"],
     {"ouro_topology": 0, "ouro_rate": 220.0, "ouro_chaosIndex": 0.6,
      "ouro_leash": 0.7, "ouro_damping": 0.2},
     {"brightness": 0.55, "warmth": 0.25, "movement": 0.65, "density": 0.5, "space": 0.05, "aggression": 0.45}),

    # ---- Strange Attractors (6) ----
    ("Lorenz Scream", "Flux", "Maximum Lorenz chaos. Unleashed, raw, screaming.",
     ["chaos", "extreme", "lorenz", "scream"],
     {"ouro_topology": 0, "ouro_rate": 300.0, "ouro_chaosIndex": 0.9,
      "ouro_leash": 0.1, "ouro_damping": 0.1},
     {"brightness": 0.7, "warmth": 0.1, "movement": 0.9, "density": 0.7, "space": 0.0, "aggression": 0.9}),

    ("Chua Fury", "Flux", "Chua at maximum chaos. Harsh digital destruction.",
     ["chaos", "extreme", "chua", "harsh"],
     {"ouro_topology": 2, "ouro_rate": 400.0, "ouro_chaosIndex": 0.95,
      "ouro_leash": 0.05, "ouro_damping": 0.05, "ouro_theta": 1.5},
     {"brightness": 0.8, "warmth": 0.05, "movement": 0.95, "density": 0.8, "space": 0.0, "aggression": 0.95}),

    ("Aizawa Storm", "Flux", "Full Aizawa chaos with no damping. Bright tornado.",
     ["chaos", "extreme", "aizawa", "storm"],
     {"ouro_topology": 3, "ouro_rate": 350.0, "ouro_chaosIndex": 0.85,
      "ouro_leash": 0.15, "ouro_damping": 0.08, "ouro_phi": -1.0},
     {"brightness": 0.65, "warmth": 0.1, "movement": 0.85, "density": 0.65, "space": 0.0, "aggression": 0.85}),

    ("Dark Attractor", "Aether", "Low-frequency Lorenz chaos. Sub-bass rumble from chaos.",
     ["chaos", "dark", "sub", "rumble"],
     {"ouro_topology": 0, "ouro_rate": 40.0, "ouro_chaosIndex": 0.7,
      "ouro_leash": 0.2, "ouro_damping": 0.6},
     {"brightness": 0.1, "warmth": 0.5, "movement": 0.6, "density": 0.4, "space": 0.2, "aggression": 0.4}),

    ("Chaos Breath", "Aether", "Rössler chaos with heavy damping. Warm chaotic breathing.",
     ["chaos", "warm", "breath", "damped"],
     {"ouro_topology": 1, "ouro_rate": 80.0, "ouro_chaosIndex": 0.65,
      "ouro_leash": 0.3, "ouro_damping": 0.75},
     {"brightness": 0.2, "warmth": 0.6, "movement": 0.5, "density": 0.3, "space": 0.3, "aggression": 0.2}),

    ("Event Horizon", "Aether", "Lorenz at the edge of stability. Deep, vast, unknowable.",
     ["chaos", "deep", "vast", "horizon"],
     {"ouro_topology": 0, "ouro_rate": 55.0, "ouro_chaosIndex": 0.5,
      "ouro_leash": 0.15, "ouro_damping": 0.7, "ouro_theta": 0.5, "ouro_phi": -0.5},
     {"brightness": 0.15, "warmth": 0.6, "movement": 0.45, "density": 0.25, "space": 0.5, "aggression": 0.15}),

    # ---- Projection Walks (4) ----
    ("Theta Sweep", "Prism", "Fixed orbit, theta rotating. Harmonic content morphs smoothly.",
     ["chaos", "sweep", "harmonic", "morphing"],
     {"ouro_topology": 0, "ouro_rate": 220.0, "ouro_chaosIndex": 0.25,
      "ouro_leash": 0.6, "ouro_damping": 0.35, "ouro_theta": 0.0, "ouro_phi": 0.0},
     {"brightness": 0.5, "warmth": 0.4, "movement": 0.5, "density": 0.35, "space": 0.1, "aggression": 0.15}),

    ("Stereo Sculpture", "Atmosphere", "Phi rotation creates stereo field movement.",
     ["chaos", "stereo", "spatial", "sculpture"],
     {"ouro_topology": 3, "ouro_rate": 165.0, "ouro_chaosIndex": 0.2,
      "ouro_leash": 0.5, "ouro_damping": 0.5, "ouro_theta": 0.3, "ouro_phi": 0.0},
     {"brightness": 0.35, "warmth": 0.55, "movement": 0.45, "density": 0.3, "space": 0.3, "aggression": 0.05}),

    ("Rotation Matrix", "Entangled", "Both theta+phi animated. Sound rotates in 3D space.",
     ["chaos", "rotation", "3d", "animated"],
     {"ouro_topology": 1, "ouro_rate": 200.0, "ouro_chaosIndex": 0.3,
      "ouro_leash": 0.4, "ouro_damping": 0.4, "ouro_theta": 0.5, "ouro_phi": 0.8},
     {"brightness": 0.45, "warmth": 0.4, "movement": 0.6, "density": 0.35, "space": 0.2, "aggression": 0.15}),

    ("Kaleidoscope View", "Prism", "Chua viewed from rapidly shifting projection angles.",
     ["chaos", "kaleidoscope", "shifting", "bright"],
     {"ouro_topology": 2, "ouro_rate": 350.0, "ouro_chaosIndex": 0.35,
      "ouro_leash": 0.5, "ouro_damping": 0.25, "ouro_theta": 1.0, "ouro_phi": -0.8},
     {"brightness": 0.65, "warmth": 0.25, "movement": 0.55, "density": 0.4, "space": 0.1, "aggression": 0.25}),

    # ---- Coupling Showcases (4) ----
    ("Chaos Injection", "Entangled", "External audio perturbs Lorenz attractor. Wild modulation.",
     ["coupling", "injection", "wild", "modulation"],
     {"ouro_topology": 0, "ouro_rate": 200.0, "ouro_chaosIndex": 0.4,
      "ouro_leash": 0.3, "ouro_damping": 0.3, "ouro_injection": 0.6},
     {"brightness": 0.5, "warmth": 0.3, "movement": 0.75, "density": 0.5, "space": 0.1, "aggression": 0.4}),

    ("Attractor LFO", "Entangled", "Ouroboros dx/dt output as complex LFO source.",
     ["coupling", "lfo", "complex", "modulation"],
     {"ouro_topology": 1, "ouro_rate": 4.0, "ouro_chaosIndex": 0.3,
      "ouro_leash": 0.1, "ouro_damping": 0.5},
     {"brightness": 0.3, "warmth": 0.5, "movement": 0.6, "density": 0.2, "space": 0.15, "aggression": 0.1}),

    ("Lorenz Feed", "Entangled", "Maximum injection — external audio drives the ODE completely.",
     ["coupling", "feed", "driven", "reactive"],
     {"ouro_topology": 0, "ouro_rate": 180.0, "ouro_chaosIndex": 0.5,
      "ouro_leash": 0.2, "ouro_damping": 0.25, "ouro_injection": 1.0},
     {"brightness": 0.55, "warmth": 0.25, "movement": 0.8, "density": 0.5, "space": 0.05, "aggression": 0.5}),

    ("Dual Chaos", "Entangled", "Two Ouroboros instances on opposite topologies cross-injecting.",
     ["coupling", "dual", "cross", "chaos"],
     {"ouro_topology": 2, "ouro_rate": 250.0, "ouro_chaosIndex": 0.55,
      "ouro_leash": 0.25, "ouro_damping": 0.3, "ouro_injection": 0.5,
      "ouro_theta": 0.7, "ouro_phi": -0.4},
     {"brightness": 0.55, "warmth": 0.3, "movement": 0.8, "density": 0.55, "space": 0.1, "aggression": 0.45}),
]


def main():
    count = 0
    for name, mood, desc, tags, overrides, dna in PRESETS:
        params = make_params(**overrides)
        engines = ["Ouroboros"]
        coupling = None
        ci = "None"
        if "coupling" in tags:
            ci = "Moderate"
        preset = make_preset(name, mood, desc, tags, params, dna,
                            engines=engines, coupling_intensity=ci, coupling=coupling)
        path = write_preset(preset)
        count += 1
        print(f"  [{count:3d}] {mood:12s} | {name}")

    print(f"\nGenerated {count} Ouroboros presets.")


if __name__ == "__main__":
    main()
