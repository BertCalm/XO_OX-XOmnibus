#!/usr/bin/env python3
"""
xpn_foundation_expansion_pack.py
XO_OX Foundation Mood Expansion Pack — 60 presets across 3 series.

Series:
  - Bedrock Bass (20): thick, grounded — density 0.7-1.0, space 0.1-0.4, aggression 0.4-0.7
  - Anchor Tone (20): warm, stable — warmth 0.6-0.9, density 0.5-0.8, movement 0.2-0.5
  - Work Horse (20): reliable utility — brightness varies, density 0.6-0.9, movement 0.4-0.7

Writes to Presets/XOceanus/Foundation/. Skips existing files.
"""

import json
import os
import sys

OUTPUT_DIR = os.path.join(
    os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
    "Presets", "XOceanus", "Foundation"
)

CREATED_DATE = "2026-03-16"

# ---------------------------------------------------------------------------
# Preset definitions
# ---------------------------------------------------------------------------

BEDROCK_BASS = [
    # (name, engine, description, dna, parameters, tags)
    (
        "Bedrock_Stomp",
        "Oblong",
        "Dense sub kick stomp. The floor beneath the floor.",
        {"brightness": 0.15, "warmth": 0.75, "movement": 0.2, "density": 0.95, "space": 0.1, "aggression": 0.65},
        {"Oblong": {"ob_drive": 0.7, "ob_body": 0.9, "ob_tone": 0.15, "ob_decay": 0.45, "ob_sub": 0.95}},
        ["foundation", "bedrock", "bass", "kick", "sub"],
    ),
    (
        "Tectonic_Mass",
        "Onset",
        "Kick and sub locked in continental-plate solidarity.",
        {"brightness": 0.1, "warmth": 0.8, "movement": 0.15, "density": 1.0, "space": 0.15, "aggression": 0.6},
        {"Onset": {"onset_kick_tune": 0.25, "onset_kick_decay": 0.5, "onset_kick_punch": 0.85, "onset_kick_sub": 0.95, "onset_sub_drive": 0.6}},
        ["foundation", "bedrock", "kick", "sub", "density"],
    ),
    (
        "Molten_Core",
        "Obese",
        "Hot sub pressure from the earth's center outward.",
        {"brightness": 0.2, "warmth": 0.85, "movement": 0.25, "density": 0.9, "space": 0.2, "aggression": 0.55},
        {"Obese": {"ob_sub_gain": 0.9, "ob_warmth": 0.85, "ob_saturation": 0.55, "ob_low_shelf": 0.8, "ob_presence": 0.15}},
        ["foundation", "bedrock", "sub", "bass", "warmth"],
    ),
    (
        "Overbite_Anchor",
        "Overbite",
        "Bass that bites down and refuses to let go.",
        {"brightness": 0.25, "warmth": 0.7, "movement": 0.3, "density": 0.85, "space": 0.2, "aggression": 0.65},
        {"Overbite": {"poss_filter_cutoff": 0.3, "poss_resonance": 0.4, "poss_drive": 0.65, "poss_amp_sustain": 0.9, "poss_sub_osc": 0.8}},
        ["foundation", "bedrock", "bass", "bite", "anchor"],
    ),
    (
        "Dub_Foundation",
        "Overdub",
        "The bass that holds the riddim steady. Roots.",
        {"brightness": 0.15, "warmth": 0.9, "movement": 0.2, "density": 0.8, "space": 0.25, "aggression": 0.45},
        {"Overdub": {"dub_delay_mix": 0.1, "dub_tape_drive": 0.5, "dub_bass_resonance": 0.7, "dub_send_vca": 0.4, "dub_warmth": 0.9}},
        ["foundation", "bedrock", "bass", "dub", "roots"],
    ),
    (
        "OddOscar_Ground",
        "OddOscar",
        "Oscar-polarity sub — the deepest pull of the spectrum.",
        {"brightness": 0.1, "warmth": 0.8, "movement": 0.2, "density": 0.9, "space": 0.15, "aggression": 0.5},
        {"OddOscar": {}},
        ["foundation", "bedrock", "bass", "sub", "oscar"],
    ),
    (
        "Orca_Depth_Charge",
        "Orca",
        "Low-frequency detonation. The orca surfaces briefly then plunges.",
        {"brightness": 0.15, "warmth": 0.7, "movement": 0.35, "density": 0.85, "space": 0.2, "aggression": 0.7},
        {"Orca": {}},
        ["foundation", "bedrock", "bass", "sub", "orca"],
    ),
    (
        "Ouroboros_Cycle",
        "Ouroboros",
        "Serpentine bass loop that grounds itself in perpetual return.",
        {"brightness": 0.2, "warmth": 0.75, "movement": 0.25, "density": 0.8, "space": 0.2, "aggression": 0.55},
        {"Ouroboros": {}},
        ["foundation", "bedrock", "bass", "loop", "cycle"],
    ),
    (
        "Orbital_Gravity",
        "Orbital",
        "Gravitational pull of a low orbit. Inescapable sub pressure.",
        {"brightness": 0.15, "warmth": 0.65, "movement": 0.3, "density": 0.9, "space": 0.15, "aggression": 0.6},
        {"Orbital": {}},
        ["foundation", "bedrock", "bass", "orbital", "gravity"],
    ),
    (
        "Oblong_Slab",
        "Oblong",
        "Thick rectangular mass of low-end. No apology.",
        {"brightness": 0.1, "warmth": 0.7, "movement": 0.15, "density": 1.0, "space": 0.1, "aggression": 0.65},
        {"Oblong": {"ob_drive": 0.6, "ob_body": 1.0, "ob_tone": 0.1, "ob_decay": 0.6, "ob_sub": 1.0}},
        ["foundation", "bedrock", "bass", "slab", "dense"],
    ),
    (
        "Onset_808_Pillar",
        "Onset",
        "808-style pillar kick. Structural, reliable, immovable.",
        {"brightness": 0.2, "warmth": 0.85, "movement": 0.2, "density": 0.9, "space": 0.2, "aggression": 0.5},
        {"Onset": {"onset_kick_tune": 0.35, "onset_kick_decay": 0.7, "onset_kick_punch": 0.7, "onset_kick_sub": 0.9, "onset_sub_drive": 0.45}},
        ["foundation", "bedrock", "808", "kick", "pillar"],
    ),
    (
        "Obese_Slow_Pressure",
        "Obese",
        "Slow-attack sub swell. Pressure builds like a diving bell.",
        {"brightness": 0.1, "warmth": 0.9, "movement": 0.2, "density": 0.85, "space": 0.25, "aggression": 0.45},
        {"Obese": {"ob_sub_gain": 0.85, "ob_warmth": 0.9, "ob_saturation": 0.4, "ob_low_shelf": 0.9, "ob_presence": 0.1}},
        ["foundation", "bedrock", "sub", "pressure", "slow"],
    ),
    (
        "Overbite_Fang_Root",
        "Overbite",
        "Root-position bass with a fang in the low-mid.",
        {"brightness": 0.3, "warmth": 0.7, "movement": 0.25, "density": 0.8, "space": 0.2, "aggression": 0.6},
        {"Overbite": {"poss_filter_cutoff": 0.35, "poss_resonance": 0.5, "poss_drive": 0.6, "poss_amp_sustain": 0.85, "poss_sub_osc": 0.75}},
        ["foundation", "bedrock", "bass", "fang", "root"],
    ),
    (
        "Overdub_Tape_Sub",
        "Overdub",
        "Tape-saturated sub. The warmth is load-bearing.",
        {"brightness": 0.15, "warmth": 0.95, "movement": 0.15, "density": 0.85, "space": 0.2, "aggression": 0.4},
        {"Overdub": {"dub_delay_mix": 0.05, "dub_tape_drive": 0.8, "dub_bass_resonance": 0.6, "dub_send_vca": 0.3, "dub_warmth": 0.95}},
        ["foundation", "bedrock", "tape", "sub", "warmth"],
    ),
    (
        "OddOscar_Bedrock",
        "OddOscar",
        "The pure Oscar descend. Sub below sub.",
        {"brightness": 0.05, "warmth": 0.75, "movement": 0.15, "density": 0.95, "space": 0.1, "aggression": 0.55},
        {"OddOscar": {}},
        ["foundation", "bedrock", "sub", "bass", "oscar", "deep"],
    ),
    (
        "Orca_Hull_Thud",
        "Orca",
        "The hull-knock resonance of a great vessel at rest.",
        {"brightness": 0.15, "warmth": 0.7, "movement": 0.2, "density": 0.9, "space": 0.25, "aggression": 0.55},
        {"Orca": {}},
        ["foundation", "bedrock", "bass", "thud", "vessel"],
    ),
    (
        "Ouroboros_Rootlock",
        "Ouroboros",
        "A bass note that eats its own tail — never moves from root.",
        {"brightness": 0.15, "warmth": 0.8, "movement": 0.15, "density": 0.85, "space": 0.15, "aggression": 0.5},
        {"Ouroboros": {}},
        ["foundation", "bedrock", "bass", "root", "locked"],
    ),
    (
        "Orbital_Station",
        "Orbital",
        "Geostationary bass. Always overhead, always present.",
        {"brightness": 0.2, "warmth": 0.65, "movement": 0.2, "density": 0.9, "space": 0.2, "aggression": 0.55},
        {"Orbital": {}},
        ["foundation", "bedrock", "bass", "orbital", "station"],
    ),
    (
        "Onset_Concrete_Kick",
        "Onset",
        "Pure concrete percussion. No reverb. No mercy.",
        {"brightness": 0.15, "warmth": 0.6, "movement": 0.15, "density": 1.0, "space": 0.1, "aggression": 0.7},
        {"Onset": {"onset_kick_tune": 0.2, "onset_kick_decay": 0.35, "onset_kick_punch": 0.95, "onset_kick_sub": 0.85, "onset_sub_drive": 0.7}},
        ["foundation", "bedrock", "kick", "concrete", "dry"],
    ),
    (
        "Oblong_Boulder_Drop",
        "Oblong",
        "A boulder dropped into still water. One deep resonant hit.",
        {"brightness": 0.1, "warmth": 0.7, "movement": 0.3, "density": 0.9, "space": 0.35, "aggression": 0.65},
        {"Oblong": {"ob_drive": 0.65, "ob_body": 0.9, "ob_tone": 0.1, "ob_decay": 0.5, "ob_sub": 0.9}},
        ["foundation", "bedrock", "bass", "impact", "boulder"],
    ),
]

ANCHOR_TONE = [
    (
        "Osteria_Hearth",
        "Osteria",
        "The warm center of a tavern on a cold night. Anchor harmonic.",
        {"brightness": 0.4, "warmth": 0.85, "movement": 0.3, "density": 0.7, "space": 0.35, "aggression": 0.2},
        {"Osteria": {}},
        ["foundation", "anchor", "tone", "warm", "hearth"],
    ),
    (
        "Ohm_Commune_Root",
        "Ohm",
        "The drone that holds the commune together. Warm, inclusive.",
        {"brightness": 0.35, "warmth": 0.8, "movement": 0.25, "density": 0.65, "space": 0.4, "aggression": 0.15},
        {"Ohm": {}},
        ["foundation", "anchor", "tone", "drone", "commune"],
    ),
    (
        "Ottoni_Brass_Anchor",
        "Ottoni",
        "Triple brass voiced low and wide. The harmonic floor.",
        {"brightness": 0.45, "warmth": 0.75, "movement": 0.3, "density": 0.7, "space": 0.35, "aggression": 0.3},
        {"Ottoni": {}},
        ["foundation", "anchor", "tone", "brass", "harmonic"],
    ),
    (
        "Obbligato_Bond_Hold",
        "Obbligato",
        "The obligatory note that cannot be omitted. Structural warmth.",
        {"brightness": 0.4, "warmth": 0.8, "movement": 0.2, "density": 0.75, "space": 0.3, "aggression": 0.2},
        {"Obbligato": {}},
        ["foundation", "anchor", "tone", "obligatory", "warmth"],
    ),
    (
        "Organon_Pillar_Tone",
        "Organon",
        "Enzymatic steady-state. A tone in metabolic balance.",
        {"brightness": 0.3, "warmth": 0.85, "movement": 0.2, "density": 0.7, "space": 0.25, "aggression": 0.15},
        {"Organon": {"organon_metabolicRate": 0.15, "organon_enzymeSelect": 200.0, "organon_catalystDrive": 0.6, "organon_dampingCoeff": 0.15, "organon_signalFlux": 0.8}},
        ["foundation", "anchor", "tone", "organon", "stable"],
    ),
    (
        "Osprey_Hover_Tone",
        "Osprey",
        "The osprey hovers. Wind-stable, locked on target frequency.",
        {"brightness": 0.5, "warmth": 0.7, "movement": 0.35, "density": 0.6, "space": 0.4, "aggression": 0.25},
        {"Osprey": {}},
        ["foundation", "anchor", "tone", "hover", "stable"],
    ),
    (
        "Oceanic_Shelf_Tone",
        "Oceanic",
        "Continental shelf warmth. Mid-depth, wide, stable.",
        {"brightness": 0.3, "warmth": 0.9, "movement": 0.25, "density": 0.7, "space": 0.4, "aggression": 0.1},
        {"Oceanic": {}},
        ["foundation", "anchor", "tone", "oceanic", "shelf"],
    ),
    (
        "OddOscar_Anchor_Slow",
        "OddOscar",
        "Oscar in his slowest aspect. A held tone that lasts.",
        {"brightness": 0.2, "warmth": 0.8, "movement": 0.2, "density": 0.7, "space": 0.3, "aggression": 0.2},
        {"OddOscar": {}},
        ["foundation", "anchor", "tone", "oscar", "slow"],
    ),
    (
        "Owlfish_Lantern",
        "Owlfish",
        "The owlfish's photophore lit steady. A guiding warmth.",
        {"brightness": 0.55, "warmth": 0.8, "movement": 0.25, "density": 0.6, "space": 0.35, "aggression": 0.15},
        {"Owlfish": {}},
        ["foundation", "anchor", "tone", "owlfish", "light"],
    ),
    (
        "Osteria_Amber_Chord",
        "Osteria",
        "Amber-lit chord at closing time. Warm resolution.",
        {"brightness": 0.45, "warmth": 0.9, "movement": 0.3, "density": 0.65, "space": 0.4, "aggression": 0.15},
        {"Osteria": {}},
        ["foundation", "anchor", "chord", "amber", "resolve"],
    ),
    (
        "Ohm_Still_Point",
        "Ohm",
        "The still point at the center of the Om. Unmoved.",
        {"brightness": 0.25, "warmth": 0.85, "movement": 0.2, "density": 0.75, "space": 0.3, "aggression": 0.1},
        {"Ohm": {}},
        ["foundation", "anchor", "drone", "stillness", "center"],
    ),
    (
        "Ottoni_Low_Unison",
        "Ottoni",
        "All three brass in low unison. The anchor chord.",
        {"brightness": 0.35, "warmth": 0.8, "movement": 0.2, "density": 0.8, "space": 0.3, "aggression": 0.25},
        {"Ottoni": {}},
        ["foundation", "anchor", "brass", "unison", "chord"],
    ),
    (
        "Obbligato_Sustain_Arc",
        "Obbligato",
        "Long sustain across both wind voices. Harmonic arch.",
        {"brightness": 0.4, "warmth": 0.75, "movement": 0.25, "density": 0.7, "space": 0.35, "aggression": 0.2},
        {"Obbligato": {}},
        ["foundation", "anchor", "tone", "sustain", "arc"],
    ),
    (
        "Organon_Slow_Enzyme",
        "Organon",
        "Metabolic rate dialed to minimum. Long-hold enzymatic warmth.",
        {"brightness": 0.25, "warmth": 0.9, "movement": 0.15, "density": 0.75, "space": 0.25, "aggression": 0.1},
        {"Organon": {"organon_metabolicRate": 0.05, "organon_enzymeSelect": 180.0, "organon_catalystDrive": 0.5, "organon_dampingCoeff": 0.2, "organon_signalFlux": 0.75}},
        ["foundation", "anchor", "tone", "organon", "slow"],
    ),
    (
        "Osprey_Thermal_Hold",
        "Osprey",
        "Riding the thermal column. Effortless altitude maintenance.",
        {"brightness": 0.45, "warmth": 0.75, "movement": 0.3, "density": 0.65, "space": 0.4, "aggression": 0.2},
        {"Osprey": {}},
        ["foundation", "anchor", "tone", "thermal", "glide"],
    ),
    (
        "Oceanic_Thermocline",
        "Oceanic",
        "Temperature layer tone. Warm above, stable boundary.",
        {"brightness": 0.35, "warmth": 0.85, "movement": 0.2, "density": 0.7, "space": 0.35, "aggression": 0.1},
        {"Oceanic": {}},
        ["foundation", "anchor", "tone", "thermocline", "layer"],
    ),
    (
        "OddOscar_Root_Pad",
        "OddOscar",
        "Oscar as pad. Root-position warmth spread across the stereo field.",
        {"brightness": 0.3, "warmth": 0.85, "movement": 0.3, "density": 0.65, "space": 0.45, "aggression": 0.15},
        {"OddOscar": {}},
        ["foundation", "anchor", "pad", "oscar", "root"],
    ),
    (
        "Owlfish_Biolume_Hold",
        "Owlfish",
        "Bioluminescent shimmer held steady. Anchored radiance.",
        {"brightness": 0.6, "warmth": 0.7, "movement": 0.25, "density": 0.6, "space": 0.4, "aggression": 0.15},
        {"Owlfish": {}},
        ["foundation", "anchor", "tone", "bioluminescent", "hold"],
    ),
    (
        "Ohm_Harmonic_Series",
        "Ohm",
        "Pure harmonic series at rest. Overtones stacked on one root.",
        {"brightness": 0.5, "warmth": 0.75, "movement": 0.25, "density": 0.7, "space": 0.35, "aggression": 0.15},
        {"Ohm": {}},
        ["foundation", "anchor", "harmonic", "drone", "series"],
    ),
    (
        "Organon_Steady_State",
        "Organon",
        "Chemical steady-state. Everything in equilibrium.",
        {"brightness": 0.35, "warmth": 0.8, "movement": 0.2, "density": 0.75, "space": 0.3, "aggression": 0.1},
        {"Organon": {"organon_metabolicRate": 0.1, "organon_enzymeSelect": 220.0, "organon_catalystDrive": 0.55, "organon_dampingCoeff": 0.2, "organon_signalFlux": 0.8}},
        ["foundation", "anchor", "tone", "organon", "equilibrium"],
    ),
    (
        "Osprey_Dawn_Tone",
        "Osprey",
        "First light. The osprey calls once and the day begins.",
        {"brightness": 0.55, "warmth": 0.75, "movement": 0.35, "density": 0.6, "space": 0.45, "aggression": 0.2},
        {"Osprey": {}},
        ["foundation", "anchor", "tone", "dawn", "call"],
    ),
]

WORK_HORSE = [
    (
        "Overworld_Chip_Workhorse",
        "Overworld",
        "NES-era lead. Reliable, punchy, does the job every time.",
        {"brightness": 0.7, "warmth": 0.4, "movement": 0.5, "density": 0.7, "space": 0.35, "aggression": 0.5},
        {"Overworld": {}},
        ["foundation", "workhorse", "chip", "lead", "reliable"],
    ),
    (
        "Onset_Drum_Kit_Core",
        "Onset",
        "All eight voices balanced. The kit that runs every session.",
        {"brightness": 0.5, "warmth": 0.55, "movement": 0.6, "density": 0.8, "space": 0.3, "aggression": 0.55},
        {"Onset": {"onset_kick_tune": 0.3, "onset_kick_punch": 0.75, "onset_snare_snap": 0.7, "onset_hat_decay": 0.4, "onset_clap_tone": 0.6}},
        ["foundation", "workhorse", "drums", "kit", "core"],
    ),
    (
        "Oblong_Utility_Bass",
        "Oblong",
        "No character, all function. The bass that fits every context.",
        {"brightness": 0.35, "warmth": 0.65, "movement": 0.45, "density": 0.75, "space": 0.25, "aggression": 0.45},
        {"Oblong": {"ob_drive": 0.45, "ob_body": 0.75, "ob_tone": 0.35, "ob_decay": 0.5, "ob_sub": 0.7}},
        ["foundation", "workhorse", "bass", "utility", "versatile"],
    ),
    (
        "Oracle_Signal_Tone",
        "Oracle",
        "Clear predictive tone. Bright and honest, no surprises.",
        {"brightness": 0.65, "warmth": 0.5, "movement": 0.5, "density": 0.7, "space": 0.3, "aggression": 0.4},
        {"Oracle": {}},
        ["foundation", "workhorse", "tone", "clear", "signal"],
    ),
    (
        "Origami_Fold_Pad",
        "Origami",
        "Crisp folded pad. Adaptable to any mix geometry.",
        {"brightness": 0.55, "warmth": 0.6, "movement": 0.5, "density": 0.7, "space": 0.4, "aggression": 0.3},
        {"Origami": {}},
        ["foundation", "workhorse", "pad", "fold", "adaptable"],
    ),
    (
        "Optic_Bright_Lead",
        "Optic",
        "Clean optical lead. Cuts through without fighting.",
        {"brightness": 0.8, "warmth": 0.4, "movement": 0.55, "density": 0.65, "space": 0.3, "aggression": 0.45},
        {"Optic": {}},
        ["foundation", "workhorse", "lead", "bright", "optic"],
    ),
    (
        "Orbital_Steady_Pulse",
        "Orbital",
        "Repeating orbital pulse. Reliable as a clock satellite.",
        {"brightness": 0.5, "warmth": 0.55, "movement": 0.6, "density": 0.75, "space": 0.35, "aggression": 0.4},
        {"Orbital": {}},
        ["foundation", "workhorse", "pulse", "steady", "clock"],
    ),
    (
        "Ocelot_Quick_Tone",
        "Ocelot",
        "Fast attack, clean decay. The note that shows up on time.",
        {"brightness": 0.6, "warmth": 0.5, "movement": 0.65, "density": 0.7, "space": 0.25, "aggression": 0.5},
        {"Ocelot": {}},
        ["foundation", "workhorse", "tone", "fast", "reliable"],
    ),
    (
        "Oblique_Utility_Chord",
        "Oblique",
        "Slightly off-angle chord voicing that sits in every mix.",
        {"brightness": 0.5, "warmth": 0.6, "movement": 0.45, "density": 0.7, "space": 0.35, "aggression": 0.3},
        {"Oblique": {}},
        ["foundation", "workhorse", "chord", "oblique", "utility"],
    ),
    (
        "Overworld_Square_Bass",
        "Overworld",
        "Genesis-era square wave bass. Genre-neutral foundation.",
        {"brightness": 0.55, "warmth": 0.45, "movement": 0.45, "density": 0.8, "space": 0.2, "aggression": 0.55},
        {"Overworld": {}},
        ["foundation", "workhorse", "bass", "square", "chip"],
    ),
    (
        "Oracle_Resonant_Mid",
        "Oracle",
        "Mid-range resonance locked in. The tone that fills the hole.",
        {"brightness": 0.55, "warmth": 0.6, "movement": 0.5, "density": 0.75, "space": 0.3, "aggression": 0.4},
        {"Oracle": {}},
        ["foundation", "workhorse", "mid", "resonant", "fill"],
    ),
    (
        "Origami_Stack_Layer",
        "Origami",
        "Folded harmonic stack. Layerable, unobtrusive, supportive.",
        {"brightness": 0.45, "warmth": 0.65, "movement": 0.4, "density": 0.8, "space": 0.35, "aggression": 0.25},
        {"Origami": {}},
        ["foundation", "workhorse", "layer", "stack", "support"],
    ),
    (
        "Onset_Hi_Hat_Engine",
        "Onset",
        "Hat patterns dialed in. The rhythmic workhorse of the kit.",
        {"brightness": 0.75, "warmth": 0.35, "movement": 0.7, "density": 0.65, "space": 0.25, "aggression": 0.5},
        {"Onset": {"onset_hat_decay": 0.3, "onset_hat_tone": 0.7, "onset_ohat_decay": 0.6, "onset_hat_level": 0.8}},
        ["foundation", "workhorse", "hats", "rhythm", "kit"],
    ),
    (
        "Optic_Lens_Sweep",
        "Optic",
        "Slow filter sweep, optically controlled. Earns its keep.",
        {"brightness": 0.6, "warmth": 0.5, "movement": 0.55, "density": 0.7, "space": 0.35, "aggression": 0.35},
        {"Optic": {}},
        ["foundation", "workhorse", "sweep", "filter", "optical"],
    ),
    (
        "Oblong_Pluck_Anchor",
        "Oblong",
        "Plucked bass note. Short, definite, supports harmony without drama.",
        {"brightness": 0.4, "warmth": 0.6, "movement": 0.55, "density": 0.75, "space": 0.2, "aggression": 0.5},
        {"Oblong": {"ob_drive": 0.4, "ob_body": 0.7, "ob_tone": 0.4, "ob_decay": 0.25, "ob_sub": 0.65}},
        ["foundation", "workhorse", "pluck", "bass", "anchor"],
    ),
    (
        "Orbital_Gate_Pulse",
        "Orbital",
        "Gated orbital rhythm. Regular, metronomic, dependable.",
        {"brightness": 0.45, "warmth": 0.5, "movement": 0.65, "density": 0.75, "space": 0.3, "aggression": 0.45},
        {"Orbital": {}},
        ["foundation", "workhorse", "gate", "pulse", "rhythm"],
    ),
    (
        "Ocelot_Sharp_Stab",
        "Ocelot",
        "Quick stab tone. The exclamation point the groove needs.",
        {"brightness": 0.7, "warmth": 0.4, "movement": 0.6, "density": 0.7, "space": 0.2, "aggression": 0.6},
        {"Ocelot": {}},
        ["foundation", "workhorse", "stab", "sharp", "quick"],
    ),
    (
        "Overworld_Arp_Run",
        "Overworld",
        "Classic chip arpeggio. Runs all day without complaint.",
        {"brightness": 0.75, "warmth": 0.4, "movement": 0.7, "density": 0.65, "space": 0.3, "aggression": 0.5},
        {"Overworld": {}},
        ["foundation", "workhorse", "arp", "chip", "run"],
    ),
    (
        "Oracle_Pad_Sustain",
        "Oracle",
        "Long-sustain pad tone. Predictably warm, never intrudes.",
        {"brightness": 0.45, "warmth": 0.7, "movement": 0.4, "density": 0.7, "space": 0.4, "aggression": 0.2},
        {"Oracle": {}},
        ["foundation", "workhorse", "pad", "sustain", "warm"],
    ),
    (
        "Oblique_Low_Texture",
        "Oblique",
        "Low-frequency texture layer. The floor of the arrangement.",
        {"brightness": 0.3, "warmth": 0.65, "movement": 0.45, "density": 0.8, "space": 0.3, "aggression": 0.35},
        {"Oblique": {}},
        ["foundation", "workhorse", "texture", "low", "layer"],
    ),
    (
        "Origami_Clean_Tone",
        "Origami",
        "Unfolded and flat. The cleanest utility tone in the library.",
        {"brightness": 0.5, "warmth": 0.55, "movement": 0.45, "density": 0.75, "space": 0.3, "aggression": 0.3},
        {"Origami": {}},
        ["foundation", "workhorse", "tone", "clean", "utility"],
    ),
]

# ---------------------------------------------------------------------------
# Builder
# ---------------------------------------------------------------------------

def build_preset(name, engine, description, dna, parameters, tags, series_tags):
    """Build a .xometa dict in the canonical XOceanus format."""
    return {
        "schema_version": 1,
        "name": name.replace("_", " "),
        "mood": "Foundation",
        "engines": [engine],
        "author": "XO_OX",
        "version": "1.0.0",
        "description": description,
        "tags": list(set(tags + series_tags)),
        "macroLabels": ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"],
        "couplingIntensity": "None",
        "tempo": None,
        "created": CREATED_DATE,
        "legacy": {
            "sourceInstrument": None,
            "sourceCategory": None,
            "sourcePresetName": None,
        },
        "parameters": parameters,
        "coupling": None,
        "sequencer": None,
        "dna": dna,
    }


def write_preset(preset_data, output_dir):
    filename = preset_data["name"].replace(" ", "_") + ".xometa"
    filepath = os.path.join(output_dir, filename)
    if os.path.exists(filepath):
        return False, filepath
    with open(filepath, "w", encoding="utf-8") as f:
        json.dump(preset_data, f, indent=2)
        f.write("\n")
    return True, filepath


def main():
    os.makedirs(OUTPUT_DIR, exist_ok=True)

    all_series = [
        (BEDROCK_BASS, ["bedrock-bass", "bass", "foundation"]),
        (ANCHOR_TONE, ["anchor-tone", "tone", "foundation"]),
        (WORK_HORSE, ["work-horse", "utility", "foundation"]),
    ]

    series_names = ["Bedrock Bass", "Anchor Tone", "Work Horse"]
    written = 0
    skipped = 0
    total = 0

    for (series, extra_tags), series_name in zip(all_series, series_names):
        series_written = 0
        series_skipped = 0
        for name, engine, description, dna, parameters, tags in series:
            preset = build_preset(name, engine, description, dna, parameters, tags, extra_tags)
            ok, path = write_preset(preset, OUTPUT_DIR)
            total += 1
            if ok:
                written += 1
                series_written += 1
                print(f"  WROTE  {os.path.basename(path)}")
            else:
                skipped += 1
                series_skipped += 1
                print(f"  SKIP   {os.path.basename(path)}  (already exists)")
        print(f"[{series_name}] {series_written} written, {series_skipped} skipped")

    print()
    print(f"Done. {written}/{total} presets written to {OUTPUT_DIR}")
    print(f"      {skipped} skipped (already existed).")


if __name__ == "__main__":
    main()
