#!/usr/bin/env python3
"""Generate cross-engine coupling presets for XOlokun.

Fills gaps in coupling coverage identified by validate_presets.py.
Focuses on pairs involving newer engines (Overworld, Overbite, Organon, Ouroboros)
that currently have zero coupling presets with the core engines.

Each preset pairs two engines with a musically meaningful coupling type.
"""

import json
import os

PRESET_DIR = os.path.join(os.path.dirname(os.path.dirname(__file__)), "Presets", "XOlokun")


def make_coupling_preset(name, mood, desc, tags, engine_a, engine_b,
                          coupling_type, coupling_amount, dna):
    return {
        "schema_version": 1,
        "name": name,
        "mood": mood,
        "engines": [engine_a, engine_b],
        "author": "XO_OX Designs",
        "version": "1.0.0",
        "description": desc,
        "tags": ["coupling"] + tags,
        "macroLabels": ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"],
        "couplingIntensity": "Moderate" if abs(coupling_amount) < 0.7 else "Deep",
        "tempo": None,
        "dna": dna,
        "parameters": {},
        "coupling": {
            "pairs": [{
                "engineA": engine_a,
                "engineB": engine_b,
                "type": coupling_type,
                "amount": coupling_amount
            }]
        },
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


# Each tuple: (name, mood, desc, tags, engineA, engineB, couplingType, amount, dna)
COUPLING_PRESETS = [
    # ---- Overworld pairs (8) ----
    ("Chip Warmth", "Entangled",
     "OddfeliX pluck character feeds Overworld filter. Retro meets organic.",
     ["oddfelix", "overworld", "warm"],
     "OddfeliX", "Overworld", "Amp->Filter", 0.6,
     {"brightness": 0.5, "warmth": 0.5, "movement": 0.4, "density": 0.4, "space": 0.2, "aggression": 0.2}),

    ("Morph Pixels", "Entangled",
     "OddOscar morph position modulated by Overworld glitch events.",
     ["oddoscar", "overworld", "morph"],
     "Overworld", "OddOscar", "Env->Morph", 0.5,
     {"brightness": 0.5, "warmth": 0.4, "movement": 0.6, "density": 0.4, "space": 0.2, "aggression": 0.3}),

    ("Chip Echo", "Entangled",
     "Overdub tape echo processes Overworld chiptune. Lo-fi meets lo-fi.",
     ["overdub", "overworld", "echo"],
     "Overworld", "Overdub", "Audio->Wavetable", 0.65,
     {"brightness": 0.4, "warmth": 0.5, "movement": 0.5, "density": 0.4, "space": 0.5, "aggression": 0.15}),

    ("Era Drift", "Entangled",
     "Odyssey tidal system modulates Overworld ERA blend position.",
     ["odyssey", "overworld", "morph"],
     "Odyssey", "Overworld", "Env->Morph", 0.55,
     {"brightness": 0.45, "warmth": 0.5, "movement": 0.6, "density": 0.4, "space": 0.3, "aggression": 0.15}),

    ("Bob Meets NES", "Entangled",
     "Oblong curiosity system explores Overworld ERA space.",
     ["oblong", "overworld", "curious"],
     "Oblong", "Overworld", "Env->Morph", 0.5,
     {"brightness": 0.4, "warmth": 0.55, "movement": 0.5, "density": 0.4, "space": 0.3, "aggression": 0.1}),

    ("Fat Chip", "Entangled",
     "Obese amplitude drives Overworld filter. Fat rhythmic chiptune.",
     ["obese", "overworld", "rhythm"],
     "Obese", "Overworld", "Amp->Filter", 0.6,
     {"brightness": 0.45, "warmth": 0.4, "movement": 0.5, "density": 0.5, "space": 0.15, "aggression": 0.35}),

    ("Drum Pixels", "Entangled",
     "Onset rhythm drives Overworld ERA blend. Beat-synced era morphing.",
     ["onset", "overworld", "rhythm"],
     "Onset", "Overworld", "Rhythm->Blend", 0.6,
     {"brightness": 0.5, "warmth": 0.35, "movement": 0.7, "density": 0.5, "space": 0.15, "aggression": 0.3}),

    ("Glitch Inject", "Entangled",
     "Overworld glitch audio injected into Organon additive partials.",
     ["overworld", "organon", "glitch"],
     "Overworld", "Organon", "Audio->Wavetable", 0.55,
     {"brightness": 0.5, "warmth": 0.4, "movement": 0.6, "density": 0.5, "space": 0.2, "aggression": 0.3}),

    # ---- Overbite pairs (8) ----
    ("Snap Bite", "Entangled",
     "OddfeliX snap attack triggers Overbite filter envelope. Pluck meets growl.",
     ["oddfelix", "overbite", "pluck"],
     "OddfeliX", "Overbite", "Amp->Filter", 0.55,
     {"brightness": 0.45, "warmth": 0.45, "movement": 0.4, "density": 0.5, "space": 0.1, "aggression": 0.35}),

    ("Morph Gnash", "Entangled",
     "OddOscar morph drives Overbite gnash amount. Wavetable controls distortion.",
     ["oddoscar", "overbite", "morph"],
     "OddOscar", "Overbite", "Env->Morph", 0.5,
     {"brightness": 0.4, "warmth": 0.4, "movement": 0.5, "density": 0.5, "space": 0.1, "aggression": 0.4}),

    ("Dub Bite", "Entangled",
     "Overdub send feeds Overbite audio input. Tape-treated bass.",
     ["overdub", "overbite", "dub"],
     "Overdub", "Overbite", "Audio->Wavetable", 0.6,
     {"brightness": 0.3, "warmth": 0.6, "movement": 0.4, "density": 0.5, "space": 0.4, "aggression": 0.25}),

    ("Odyssey Fang", "Entangled",
     "Odyssey pad amplitude opens Overbite filter. Lush meets aggressive.",
     ["odyssey", "overbite", "pad"],
     "Odyssey", "Overbite", "Amp->Filter", 0.55,
     {"brightness": 0.4, "warmth": 0.5, "movement": 0.4, "density": 0.5, "space": 0.3, "aggression": 0.3}),

    ("Bob Gnaw", "Entangled",
     "Oblong curiosity modulates Overbite filter. Exploring distortion space.",
     ["oblong", "overbite", "curious"],
     "Oblong", "Overbite", "Amp->Filter", 0.5,
     {"brightness": 0.35, "warmth": 0.5, "movement": 0.5, "density": 0.45, "space": 0.2, "aggression": 0.25}),

    ("Fat Growl", "Entangled",
     "Obese drives Overbite. Maximum low-end aggression.",
     ["obese", "overbite", "bass"],
     "Obese", "Overbite", "Amp->Filter", 0.7,
     {"brightness": 0.25, "warmth": 0.5, "movement": 0.4, "density": 0.65, "space": 0.1, "aggression": 0.6}),

    ("Drum Bite", "Entangled",
     "Onset rhythm gates Overbite filter. Percussive bass textures.",
     ["onset", "overbite", "rhythm"],
     "Onset", "Overbite", "Rhythm->Blend", 0.55,
     {"brightness": 0.4, "warmth": 0.4, "movement": 0.6, "density": 0.5, "space": 0.1, "aggression": 0.4}),

    ("Bite World", "Entangled",
     "Overbite bass processed through Overworld chip filter. Dark retro growl.",
     ["overbite", "overworld", "dark"],
     "Overbite", "Overworld", "Audio->Wavetable", 0.6,
     {"brightness": 0.3, "warmth": 0.45, "movement": 0.4, "density": 0.5, "space": 0.15, "aggression": 0.45}),

    # ---- Ouroboros pairs (6) ----
    ("Chaos Filter", "Entangled",
     "OddfeliX amplitude modulates Ouroboros damping. Playing controls chaos warmth.",
     ["oddfelix", "ouroboros", "chaos"],
     "OddfeliX", "Ouroboros", "Amp->Filter", 0.5,
     {"brightness": 0.5, "warmth": 0.35, "movement": 0.6, "density": 0.4, "space": 0.1, "aggression": 0.3}),

    ("Morph Attractor", "Entangled",
     "OddOscar morph drives Ouroboros projection theta. Wavetable rotates chaos.",
     ["oddoscar", "ouroboros", "morph"],
     "OddOscar", "Ouroboros", "Env->Morph", 0.5,
     {"brightness": 0.45, "warmth": 0.4, "movement": 0.65, "density": 0.4, "space": 0.15, "aggression": 0.25}),

    ("Chaos Dub", "Entangled",
     "Ouroboros chaos audio feeds Overdub echo chain. Strange attractor reverb.",
     ["ouroboros", "overdub", "echo"],
     "Ouroboros", "Overdub", "Audio->Wavetable", 0.55,
     {"brightness": 0.4, "warmth": 0.45, "movement": 0.6, "density": 0.4, "space": 0.5, "aggression": 0.2}),

    ("Chaos Drift", "Entangled",
     "Odyssey tidal depth modulates Ouroboros chaos index. Waves of chaos.",
     ["odyssey", "ouroboros", "tidal"],
     "Odyssey", "Ouroboros", "Env->Decay", 0.55,
     {"brightness": 0.4, "warmth": 0.45, "movement": 0.65, "density": 0.4, "space": 0.25, "aggression": 0.25}),

    ("Fat Chaos", "Entangled",
     "Obese drives Ouroboros injection. Heavy signal perturbs the attractor.",
     ["obese", "ouroboros", "heavy"],
     "Obese", "Ouroboros", "Audio->FM", 0.65,
     {"brightness": 0.4, "warmth": 0.35, "movement": 0.7, "density": 0.55, "space": 0.1, "aggression": 0.5}),

    ("Drum Chaos", "Entangled",
     "Onset rhythm modulates Ouroboros chaos index. Beat-synced bifurcation.",
     ["onset", "ouroboros", "rhythm"],
     "Onset", "Ouroboros", "Rhythm->Blend", 0.6,
     {"brightness": 0.45, "warmth": 0.35, "movement": 0.75, "density": 0.5, "space": 0.1, "aggression": 0.4}),

    # ---- Organon pairs (4) ----
    ("Organ Bite", "Entangled",
     "Organon tones drive Overbite filter. Additive meets subtractive.",
     ["organon", "overbite", "additive"],
     "Organon", "Overbite", "Amp->Filter", 0.55,
     {"brightness": 0.45, "warmth": 0.5, "movement": 0.4, "density": 0.5, "space": 0.2, "aggression": 0.3}),

    ("Chaos Organ", "Entangled",
     "Ouroboros dx/dt modulates Organon partial amplitudes. Chaos shapes timbre.",
     ["organon", "ouroboros", "chaos"],
     "Ouroboros", "Organon", "Env->Morph", 0.5,
     {"brightness": 0.5, "warmth": 0.4, "movement": 0.6, "density": 0.4, "space": 0.2, "aggression": 0.2}),

    ("Snap Organ", "Entangled",
     "OddfeliX pluck triggers Organon partial cascade. Pluck into organ bloom.",
     ["oddfelix", "organon", "pluck"],
     "OddfeliX", "Organon", "Amp->Filter", 0.5,
     {"brightness": 0.5, "warmth": 0.5, "movement": 0.4, "density": 0.4, "space": 0.25, "aggression": 0.15}),

    ("Organ Dub", "Entangled",
     "Organon audio through Overdub echo chain. Additive organ with tape echo.",
     ["organon", "overdub", "echo"],
     "Organon", "Overdub", "Audio->Wavetable", 0.55,
     {"brightness": 0.4, "warmth": 0.55, "movement": 0.4, "density": 0.4, "space": 0.5, "aggression": 0.1}),
]


def main():
    count = 0
    for name, mood, desc, tags, ea, eb, ctype, amount, dna in COUPLING_PRESETS:
        preset = make_coupling_preset(name, mood, desc, tags, ea, eb, ctype, amount, dna)
        path = write_preset(preset)
        count += 1
        print(f"  [{count:3d}] {ea:12s} <-> {eb:12s} | {name}")

    print(f"\nGenerated {count} coupling presets.")


if __name__ == "__main__":
    main()
