#!/usr/bin/env python3
"""
xpn_movement_aggression_expander.py

Generates 60 presets targeting compressed movement/aggression DNA extremes:
  - 15 Glacial   (movement 0.0–0.1, aggression 0.0–0.1)  → Atmosphere
  - 15 Violent   (movement 0.9–1.0, aggression 0.9–1.0)  → Flux
  - 15 Tidal     (movement 0.85–1.0, aggression 0.0–0.15) → Flux
  - 15 Siege     (movement 0.0–0.15, aggression 0.85–1.0) → Foundation

Python stdlib only. Skips existing files.
"""

import json
import os
import random

BASE = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
PRESETS_DIR = os.path.join(BASE, "Presets", "XOlokun")

# ---------------------------------------------------------------------------
# Preset definitions: (name, engine, movement, aggression, brightness, warmth,
#                      density, space, tags, description)
# ---------------------------------------------------------------------------

GLACIAL_PRESETS = [
    # OBSIDIAN
    ("Obsidian Stillness",   "Obsidian",  0.02, 0.02, 0.25, 0.6,  0.3, 0.85, ["glacial", "dark", "obsidian", "still"],          "Volcanic glass frozen mid-pour — absolute silence at the edge of form."),
    ("Lava Glass Pause",     "Obsidian",  0.05, 0.04, 0.2,  0.65, 0.25,0.8,  ["glacial", "obsidian", "pause", "cold"],           "The moment lava surrenders heat and becomes mirror."),
    # OPAL
    ("Opal Suspension",      "Opal",      0.03, 0.01, 0.55, 0.45, 0.2, 0.9,  ["glacial", "opal", "granular", "serene"],          "Granules frozen in amber light — colour without motion."),
    ("Spectral Freeze",      "Opal",      0.08, 0.06, 0.6,  0.4,  0.15,0.88, ["glacial", "opal", "spectral", "freeze"],          "A single grain stretched across an ice shelf."),
    # OVERDUB
    ("Dub Cryogenic",        "Overdub",   0.04, 0.03, 0.3,  0.7,  0.35,0.82, ["glacial", "overdub", "dub", "cryogenic"],         "Tape echo cooled to near-zero — reverb trails frozen in place."),
    ("Spring Permafrost",    "Overdub",   0.07, 0.05, 0.28, 0.72, 0.3, 0.87, ["glacial", "overdub", "reverb", "still"],          "Spring reverb suspended beneath permafrost — resonance without decay."),
    # OMBRE
    ("Ombre Tundra",         "Ombre",     0.03, 0.02, 0.35, 0.55, 0.25,0.9,  ["glacial", "ombre", "tundra", "gradient"],         "A slow colour gradient across a frozen plain — barely shifting."),
    # ORGANON
    ("Organon Stasis",       "Organon",   0.05, 0.04, 0.4,  0.6,  0.4, 0.8,  ["glacial", "organon", "stasis", "harmonic"],       "Pipe organ drones locked in harmonic stasis — breath held forever."),
    ("Cathedral Ice",        "Organon",   0.08, 0.07, 0.38, 0.62, 0.45,0.78, ["glacial", "organon", "cathedral", "cold"],        "Stone and cold air — liturgical silence between the pillars."),
    # ORACLE
    ("Oracle Dormant",       "Oracle",    0.02, 0.01, 0.45, 0.5,  0.2, 0.92, ["glacial", "oracle", "dormant", "prophecy"],       "The oracle rests — vision suspended in crystalline silence."),
    ("Sibyl Frozen",         "Oracle",    0.06, 0.05, 0.42, 0.52, 0.18,0.9,  ["glacial", "oracle", "sibyl", "still"],            "Words not yet spoken, frozen on the oracle's tongue."),
    # OSPREY
    ("Osprey Thermals",      "Osprey",    0.04, 0.02, 0.5,  0.45, 0.2, 0.93, ["glacial", "osprey", "soaring", "still"],          "Held high on a thermal column — zero effort, zero movement."),
    ("Arctic Hover",         "Osprey",    0.07, 0.03, 0.48, 0.47, 0.22,0.91, ["glacial", "osprey", "arctic", "hover"],           "Wings locked in a katabatic wind — stillness through perfect opposition."),
    ("Frost Vigil",          "Osprey",    0.09, 0.08, 0.3,  0.6,  0.3, 0.82, ["glacial", "osprey", "frost", "vigil"],            "Roosting through the polar night — patient, motionless, present."),
    ("Ice Field Survey",     "Organon",   0.1,  0.09, 0.35, 0.55, 0.38,0.85, ["glacial", "organon", "survey", "vast"],           "Low harmonic survey across an unbroken white expanse."),
]

VIOLENT_PRESETS = [
    # OUROBOROS
    ("Ouroboros Frenzy",     "Ouroboros", 0.97, 0.97, 0.75, 0.35, 0.8, 0.2,  ["violent", "ouroboros", "chaos", "frenzy"],        "Self-devouring at maximum velocity — the serpent tears itself apart."),
    ("Feedback Apocalypse",  "Ouroboros", 0.94, 0.99, 0.8,  0.3,  0.85,0.15, ["violent", "ouroboros", "feedback", "apocalypse"], "Recursive destruction loop — signal consuming signal."),
    # ONSET
    ("Onset Eruption",       "Onset",     0.96, 0.98, 0.7,  0.4,  0.9, 0.1,  ["violent", "onset", "percussion", "eruption"],     "Every transient at maximum attack — volcanic drum battery."),
    ("Percussive Siege",     "Onset",     0.92, 0.95, 0.65, 0.45, 0.88,0.12, ["violent", "onset", "siege", "drums"],             "Wall of hits with zero mercy — pure rhythmic violence."),
    # ORCA
    ("Orca Breach",          "Orca",      0.99, 0.96, 0.6,  0.5,  0.75,0.18, ["violent", "orca", "breach", "impact"],            "Five-tonne body airborne — re-entry as sonic event."),
    ("Pack Hunt",            "Orca",      0.95, 0.98, 0.65, 0.45, 0.8, 0.15, ["violent", "orca", "pack", "predator"],            "Coordinated pursuit at full sprint — no prey escapes."),
    # OCTOPUS
    ("Octopus Thrash",       "Octopus",   0.93, 0.92, 0.55, 0.5,  0.7, 0.22, ["violent", "octopus", "thrash", "chaos"],          "Eight arms in maximal discoordination — beautiful structural violence."),
    ("Ink Cloud Burst",      "Octopus",   0.97, 0.94, 0.5,  0.55, 0.75,0.2,  ["violent", "octopus", "ink", "burst"],             "Defence as offence — dense chemical chaos at full release."),
    # OBESE
    ("Obese Maximum",        "Obese",     0.91, 0.97, 0.72, 0.38, 0.92,0.08, ["violent", "obese", "maximum", "heavy"],           "Absolute mass at absolute velocity — gravitational violence."),
    ("XObese Rampage",       "Obese",     0.95, 0.99, 0.78, 0.32, 0.9, 0.1,  ["violent", "obese", "rampage", "crushing"],        "Every sample slot detonating simultaneously — unhinged mass event."),
    # OVERBITE
    ("Overbite Fang Strike", "Overbite",  0.98, 0.98, 0.7,  0.42, 0.82,0.14, ["violent", "overbite", "strike", "bass"],          "Bass jaw snapping shut at terminal velocity — bite and hold."),
    ("Acid Surge",           "Overbite",  0.93, 0.96, 0.68, 0.44, 0.85,0.12, ["violent", "overbite", "acid", "surge"],           "Overbite filter fully open — acid wave at maximum aggression."),
    # OVERWORLD
    ("Chip Overload",        "Overworld", 0.96, 0.93, 0.85, 0.2,  0.78,0.18, ["violent", "overworld", "chip", "overload"],       "All three chip engines in simultaneous distortion overload."),
    ("NES Detonation",       "Overworld", 0.94, 0.91, 0.88, 0.18, 0.8, 0.16, ["violent", "overworld", "NES", "detonation"],      "2A03 pushed past technical limits — retro violence at 8-bit scale."),
    ("Genesis Fury",         "Overworld", 0.98, 0.95, 0.82, 0.25, 0.83,0.12, ["violent", "overworld", "genesis", "YM2612"],      "YM2612 FM feedback at maximum — Mega Drive hardware screaming."),
]

TIDAL_PRESETS = [
    # OHM
    ("Ohm Tidal Drone",      "Ohm",       0.92, 0.06, 0.4,  0.65, 0.5, 0.75, ["tidal", "ohm", "drone", "gentle"],               "Hippy-dad sine waves cycling at tidal rhythm — fast but at peace."),
    ("Commune Surge",        "Ohm",       0.88, 0.1,  0.38, 0.67, 0.45,0.78, ["tidal", "ohm", "commune", "surge"],              "The commune breathes in waves — collective motion without tension."),
    # ORPHICA
    ("Orphica Cascade",      "Orphica",   0.95, 0.08, 0.55, 0.5,  0.3, 0.85, ["tidal", "orphica", "granular", "cascade"],        "Microsound grains cascading like surf across a siphonophore colony."),
    ("Harp Tide",            "Orphica",   0.9,  0.05, 0.6,  0.45, 0.25,0.88, ["tidal", "orphica", "harp", "tide"],               "String grains swept by a benevolent current — fast, musical, serene."),
    # OLE
    ("Olé Wave",             "Ole",       0.93, 0.12, 0.58, 0.5,  0.55,0.65, ["tidal", "ole", "latin", "wave"],                  "Afro-Cuban rhythmic tide at full flow — drama without violence."),
    ("Clave Surge",          "Ole",       0.87, 0.09, 0.62, 0.48, 0.6, 0.62, ["tidal", "ole", "clave", "surge"],                 "Clave pattern riding the swell — momentum without aggression."),
    # ORIGAMI
    ("Origami Unfurl",       "Origami",   0.91, 0.07, 0.5,  0.55, 0.35,0.8,  ["tidal", "origami", "unfurl", "fold"],             "Paper folds releasing in rapid sequence — kinetic grace at speed."),
    ("Paper Cascade",        "Origami",   0.96, 0.11, 0.48, 0.57, 0.3, 0.82, ["tidal", "origami", "paper", "cascade"],           "A thousand cranes unfolding simultaneously — motion as meditation."),
    # OBLIQUE
    ("Oblique Rip Current",  "Oblique",   0.89, 0.08, 0.45, 0.5,  0.4, 0.72, ["tidal", "oblique", "rip", "current"],             "Lateral energy at full speed — oblique force without malice."),
    ("Cross-Swell Glide",    "Oblique",   0.94, 0.13, 0.42, 0.52, 0.38,0.75, ["tidal", "oblique", "cross-swell", "glide"],       "Two tidal bodies intersecting — combined motion, zero impact."),
    # OCELOT
    ("Ocelot Sprint",        "Ocelot",    0.97, 0.1,  0.65, 0.45, 0.45,0.7,  ["tidal", "ocelot", "sprint", "agile"],             "Forest cat at full gallop — effortless maximum velocity."),
    ("Dappled Rush",         "Ocelot",    0.85, 0.07, 0.7,  0.42, 0.4, 0.73, ["tidal", "ocelot", "dappled", "rush"],             "Light through canopy at sprint pace — bright, fast, harmless."),
    # OPTIC
    ("Optic Flow",           "Optic",     0.93, 0.09, 0.72, 0.38, 0.35,0.78, ["tidal", "optic", "flow", "light"],                "Visual field streaming past at speed — all movement, no tension."),
    ("Photon Tide",          "Optic",     0.9,  0.06, 0.78, 0.35, 0.3, 0.82, ["tidal", "optic", "photon", "tide"],               "Coherent light cycling through the spectrum — wave nature dominant."),
    ("Saccade River",        "Optic",     0.87, 0.14, 0.75, 0.4,  0.32,0.76, ["tidal", "optic", "saccade", "river"],             "Rapid eye movements as water — perception at tidal velocity."),
]

SIEGE_PRESETS = [
    # OBLONG
    ("Oblong Rampart",       "Oblong",    0.05, 0.92, 0.3,  0.65, 0.8, 0.25, ["siege", "oblong", "rampart", "heavy"],            "Immovable structure at maximum density — the wall before the gate."),
    ("Bob Battering Ram",    "Oblong",    0.08, 0.97, 0.28, 0.67, 0.85,0.2,  ["siege", "oblong", "battering", "crushing"],       "Static mass with single purpose — pressure without pace."),
    # OTTONI
    ("Ottoni Bombardment",   "Ottoni",    0.06, 0.95, 0.5,  0.5,  0.75,0.22, ["siege", "ottoni", "brass", "bombardment"],        "Triple brass locked in low unison — harmonic siege engine."),
    ("GROW Maximum Crush",   "Ottoni",    0.1,  0.98, 0.48, 0.52, 0.78,0.18, ["siege", "ottoni", "GROW", "crush"],               "GROW macro at maximum — brass swell without forward motion."),
    # OBBLIGATO
    ("Obbligato Bond Siege", "Obbligato", 0.07, 0.93, 0.35, 0.6,  0.7, 0.28, ["siege", "obbligato", "BOND", "siege"],            "BOND macro holding both wind voices in aggressive stasis."),
    ("Reed Wall",            "Obbligato", 0.12, 0.88, 0.38, 0.58, 0.65,0.32, ["siege", "obbligato", "reed", "wall"],             "Dual woodwind sustain as structural force — held without movement."),
    # OBSIDIAN
    ("Obsidian Fortress",    "Obsidian",  0.04, 0.96, 0.15, 0.72, 0.9, 0.15, ["siege", "obsidian", "fortress", "dark"],          "Volcanic glass as architecture — impenetrable and still."),
    ("Black Glass Siege",    "Obsidian",  0.09, 0.91, 0.12, 0.75, 0.88,0.18, ["siege", "obsidian", "glass", "crushing"],         "Dense black refraction under maximum pressure — no give, no motion."),
    # OWLFISH
    ("Owlfish Anchor",       "Owlfish",   0.06, 0.94, 0.4,  0.55, 0.72,0.28, ["siege", "owlfish", "anchor", "deep"],             "Owlfish planted in seafloor — pure downward force without drift."),
    ("Demersal Pressure",    "Owlfish",   0.11, 0.89, 0.38, 0.57, 0.7, 0.3,  ["siege", "owlfish", "demersal", "pressure"],       "Bottom-dwelling weight at maximum aggression — crushing stillness."),
    # OSTERIA
    ("Osteria Standoff",     "Osteria",   0.05, 0.9,  0.45, 0.6,  0.68,0.32, ["siege", "osteria", "standoff", "heavy"],          "The tavern in the siege town — unmoving through the bombardment."),
    ("Stone Table",          "Osteria",   0.1,  0.95, 0.42, 0.62, 0.72,0.26, ["siege", "osteria", "stone", "table"],             "Unmovable mass at the centre of the room — dense, still, dominant."),
    # ORBITAL
    ("Orbital Lock",         "Orbital",   0.08, 0.91, 0.35, 0.5,  0.75,0.22, ["siege", "orbital", "lock", "gravity"],            "Gravitational lock — orbital mechanics as static crushing force."),
    ("Lagrange Siege",       "Orbital",   0.13, 0.94, 0.32, 0.52, 0.78,0.2,  ["siege", "orbital", "lagrange", "siege"],          "Positioned at L1 — maximum aggression through perfect stillness."),
    ("Apoapsis Hold",        "Orbital",   0.15, 0.87, 0.3,  0.55, 0.7, 0.25, ["siege", "orbital", "apoapsis", "hold"],           "Orbit at its farthest point — zero velocity, maximum potential energy."),
]

# ---------------------------------------------------------------------------
# Build helpers
# ---------------------------------------------------------------------------

def make_preset(name, engine, movement, aggression, brightness, warmth,
                density, space, tags, description, mood):
    macro_movement = round(movement, 3)
    # CHARACTER: high aggression → high, high warmth → high
    character = round((aggression * 0.5 + warmth * 0.5), 3)
    # COUPLING: mild default
    coupling = round(random.uniform(0.1, 0.35), 2)
    # SPACE maps from space DNA
    space_macro = round(space, 2)

    return {
        "schema_version": 1,
        "author": "XO_OX Designs",
        "version": "1.0",
        "name": name,
        "mood": mood,
        "engines": [engine],
        "couplingIntensity": "None",
        "tempo": None,
        "macroLabels": ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"],
        "macros": {
            "CHARACTER": character,
            "MOVEMENT": macro_movement,
            "COUPLING": coupling,
            "SPACE": space_macro,
        },
        "coupling": {"pairs": []},
        "sequencer": None,
        "dna": {
            "brightness": round(brightness, 3),
            "warmth": round(warmth, 3),
            "movement": round(movement, 3),
            "density": round(density, 3),
            "space": round(space, 3),
            "aggression": round(aggression, 3),
        },
        "parameters": {},
        "tags": tags,
        "description": description,
    }


def slug(name):
    """Convert preset name to safe filename."""
    return name.replace(" ", "_").replace("/", "-").replace("'", "")


def write_preset(preset_def, mood_dir, mood):
    name, engine, movement, aggression, brightness, warmth, density, space, tags, desc = preset_def
    data = make_preset(name, engine, movement, aggression, brightness, warmth,
                       density, space, tags, desc, mood)
    os.makedirs(mood_dir, exist_ok=True)
    filepath = os.path.join(mood_dir, f"{slug(name)}.xometa")
    if os.path.exists(filepath):
        print(f"  SKIP (exists): {filepath}")
        return False
    with open(filepath, "w", encoding="utf-8") as f:
        json.dump(data, f, indent=2, ensure_ascii=False)
    print(f"  WROTE: {filepath}")
    return True


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main():
    random.seed(42)

    atmosphere_dir = os.path.join(PRESETS_DIR, "Atmosphere")
    flux_dir       = os.path.join(PRESETS_DIR, "Flux")
    foundation_dir = os.path.join(PRESETS_DIR, "Foundation")

    groups = [
        ("GLACIAL",  GLACIAL_PRESETS,  atmosphere_dir, "Atmosphere"),
        ("VIOLENT",  VIOLENT_PRESETS,  flux_dir,       "Flux"),
        ("TIDAL",    TIDAL_PRESETS,    flux_dir,       "Flux"),
        ("SIEGE",    SIEGE_PRESETS,    foundation_dir, "Foundation"),
    ]

    total_written = 0
    total_skipped = 0

    for group_name, presets, dest_dir, mood in groups:
        print(f"\n--- {group_name} ({mood}) ---")
        written = skipped = 0
        for p in presets:
            ok = write_preset(p, dest_dir, mood)
            if ok:
                written += 1
            else:
                skipped += 1
        print(f"  {written} written, {skipped} skipped")
        total_written += written
        total_skipped += skipped

    print(f"\nDone. Total written: {total_written}, skipped: {total_skipped}")

    # Summary table
    print("\nDNA range verification:")
    for group_name, presets, _, mood in groups:
        mvs = [p[2] for p in presets]
        ags = [p[3] for p in presets]
        print(f"  {group_name:8s}  movement [{min(mvs):.2f}–{max(mvs):.2f}]  aggression [{min(ags):.2f}–{max(ags):.2f}]  count={len(presets)}")


if __name__ == "__main__":
    main()
