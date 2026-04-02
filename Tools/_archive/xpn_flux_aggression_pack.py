#!/usr/bin/env python3
"""
xpn_flux_aggression_pack.py
Generates 60 Flux mood presets targeting the full aggression spectrum.
- 20 Maximum Flux: aggression 0.85–1.0, movement 0.8–1.0
- 20 Controlled Flux: aggression 0.5–0.75, movement 0.7–0.9
- 20 Tension Flux: aggression 0.7–0.9, movement 0.1–0.3
Writes to Presets/XOceanus/Flux/. Skips existing files.
"""

import json
import os
import sys

PRESET_DIR = os.path.join(
    os.path.dirname(os.path.abspath(__file__)),
    "..", "Presets", "XOceanus", "Flux"
)

# ---------------------------------------------------------------------------
# Preset definitions
# Each entry: (name, engine, dna_dict, macros_dict, tags, description)
# dna keys: brightness, warmth, movement, density, space, aggression
# macros keys: CHARACTER, MOVEMENT, COUPLING, SPACE
# MOVEMENT macro mirrors movement DNA
# ---------------------------------------------------------------------------

MAXIMUM_FLUX = [
    (
        "Fracture Event",
        "Ouroboros",
        dict(brightness=0.85, warmth=0.2, movement=0.97, density=0.92, space=0.15, aggression=0.98),
        dict(CHARACTER=0.85, MOVEMENT=0.97, COUPLING=0.1, SPACE=0.15),
        ["flux", "maximum", "ouroboros", "chaos", "fracture"],
        "Ouroboros feedback loop at critical mass — topology collapses into pure event."
    ),
    (
        "Apex Strike",
        "Orca",
        dict(brightness=0.75, warmth=0.15, movement=0.94, density=0.88, space=0.12, aggression=0.96),
        dict(CHARACTER=0.9, MOVEMENT=0.94, COUPLING=0.05, SPACE=0.12),
        ["flux", "maximum", "orca", "predator", "strike"],
        "Orca breach at full velocity — echolocation burst into direct impact."
    ),
    (
        "Fat Riot",
        "Obese",
        dict(brightness=0.72, warmth=0.35, movement=0.91, density=0.95, space=0.08, aggression=0.95),
        dict(CHARACTER=0.95, MOVEMENT=0.91, COUPLING=0.0, SPACE=0.08),
        ["flux", "maximum", "obese", "saturation", "riot"],
        "MOJO slammed — every harmonic screaming at once, saturation absolute."
    ),
    (
        "Fang Override",
        "Overbite",
        dict(brightness=0.8, warmth=0.28, movement=0.88, density=0.9, space=0.1, aggression=0.97),
        dict(CHARACTER=0.88, MOVEMENT=0.88, COUPLING=0.0, SPACE=0.1),
        ["flux", "maximum", "overbite", "fang", "override"],
        "All five BITE macros pinned — the filter bites through bone."
    ),
    (
        "Ink Cloud Rage",
        "Octopus",
        dict(brightness=0.55, warmth=0.25, movement=0.96, density=0.94, space=0.18, aggression=0.93),
        dict(CHARACTER=0.82, MOVEMENT=0.96, COUPLING=0.15, SPACE=0.18),
        ["flux", "maximum", "octopus", "ink", "rage"],
        "Eight arms firing simultaneously — ink cloud as sonic weapon."
    ),
    (
        "Machine Gun Perc",
        "Onset",
        dict(brightness=0.9, warmth=0.1, movement=0.99, density=0.87, space=0.05, aggression=0.99),
        dict(CHARACTER=0.7, MOVEMENT=0.99, COUPLING=0.0, SPACE=0.05),
        ["flux", "maximum", "onset", "percussion", "machine"],
        "XVC at maximum cross-voice density — drum machine as weapon."
    ),
    (
        "Glitch Era Collapse",
        "Overworld",
        dict(brightness=0.95, warmth=0.05, movement=0.93, density=0.82, space=0.12, aggression=0.9),
        dict(CHARACTER=0.78, MOVEMENT=0.93, COUPLING=0.0, SPACE=0.12),
        ["flux", "maximum", "overworld", "glitch", "collapse"],
        "ERA triangle cornered in NES maximum — chip noise at the edge of meltdown."
    ),
    (
        "Phosphor Storm",
        "Optic",
        dict(brightness=0.99, warmth=0.05, movement=0.95, density=0.78, space=0.2, aggression=0.87),
        dict(CHARACTER=0.65, MOVEMENT=0.95, COUPLING=0.0, SPACE=0.2),
        ["flux", "maximum", "optic", "phosphor", "storm"],
        "AutoPulse at maximum rate — light as pure aggression, zero silence."
    ),
    (
        "Prism Shatter",
        "Oblique",
        dict(brightness=0.88, warmth=0.18, movement=0.9, density=0.85, space=0.22, aggression=0.91),
        dict(CHARACTER=0.82, MOVEMENT=0.9, COUPLING=0.08, SPACE=0.22),
        ["flux", "maximum", "oblique", "prism", "shatter"],
        "Prismatic bounce with walls collapsed — RTJ-mode at critical resonance."
    ),
    (
        "Fold Until Ruin",
        "Origami",
        dict(brightness=0.7, warmth=0.3, movement=0.85, density=0.88, space=0.1, aggression=0.92),
        dict(CHARACTER=0.88, MOVEMENT=0.85, COUPLING=0.0, SPACE=0.1),
        ["flux", "maximum", "origami", "fold", "ruin"],
        "Paper folded past its limit — vermillion crease becomes a wound."
    ),
    (
        "Predator Frequency",
        "Orca",
        dict(brightness=0.6, warmth=0.12, movement=0.92, density=0.93, space=0.07, aggression=1.0),
        dict(CHARACTER=0.95, MOVEMENT=0.92, COUPLING=0.0, SPACE=0.07),
        ["flux", "maximum", "orca", "predator", "frequency"],
        "Echolocation locked on target — subsonic pulse before the kill."
    ),
    (
        "Chaos Spine",
        "Ouroboros",
        dict(brightness=0.65, warmth=0.22, movement=0.98, density=0.96, space=0.06, aggression=0.94),
        dict(CHARACTER=0.9, MOVEMENT=0.98, COUPLING=0.2, SPACE=0.06),
        ["flux", "maximum", "ouroboros", "chaos", "spine"],
        "Strange attractor at peak density — the serpent eating itself at speed."
    ),
    (
        "Obese Meltdown",
        "Obese",
        dict(brightness=0.78, warmth=0.4, movement=0.87, density=0.97, space=0.05, aggression=0.96),
        dict(CHARACTER=1.0, MOVEMENT=0.87, COUPLING=0.0, SPACE=0.05),
        ["flux", "maximum", "obese", "meltdown", "saturation"],
        "BELLY maxed, MOJO maxed — thermal runaway in the harmonic chain."
    ),
    (
        "Venom Engine",
        "Overbite",
        dict(brightness=0.82, warmth=0.15, movement=0.82, density=0.89, space=0.09, aggression=0.98),
        dict(CHARACTER=0.92, MOVEMENT=0.82, COUPLING=0.0, SPACE=0.09),
        ["flux", "maximum", "overbite", "venom", "engine"],
        "BITE + TRASH running together — acid filter with possum venom backing."
    ),
    (
        "Chip Overload",
        "Overworld",
        dict(brightness=0.97, warmth=0.08, movement=0.88, density=0.8, space=0.1, aggression=0.88),
        dict(CHARACTER=0.8, MOVEMENT=0.88, COUPLING=0.0, SPACE=0.1),
        ["flux", "maximum", "overworld", "chip", "overload"],
        "NES 2A03 duty cycle at 0% — square wave as pure industrial force."
    ),
    (
        "Arm Cluster",
        "Octopus",
        dict(brightness=0.68, warmth=0.2, movement=0.94, density=0.91, space=0.15, aggression=0.89),
        dict(CHARACTER=0.78, MOVEMENT=0.94, COUPLING=0.18, SPACE=0.15),
        ["flux", "maximum", "octopus", "arms", "cluster"],
        "Wolfram CA rule 30 — unpredictable arm outputs all competing for dominance."
    ),
    (
        "Perc Barrage",
        "Onset",
        dict(brightness=0.88, warmth=0.12, movement=0.97, density=0.9, space=0.06, aggression=0.97),
        dict(CHARACTER=0.75, MOVEMENT=0.97, COUPLING=0.0, SPACE=0.06),
        ["flux", "maximum", "onset", "percussion", "barrage"],
        "All 8 voices triggered in a 32nd-note fusillade — rhythm as violence."
    ),
    (
        "Optic Overburn",
        "Optic",
        dict(brightness=1.0, warmth=0.0, movement=0.89, density=0.72, space=0.25, aggression=0.85),
        dict(CHARACTER=0.6, MOVEMENT=0.89, COUPLING=0.0, SPACE=0.25),
        ["flux", "maximum", "optic", "overburn", "light"],
        "Phosphor screen pushed past luminance limits — visual synthesis as pure white noise."
    ),
    (
        "Origami Rage",
        "Origami",
        dict(brightness=0.75, warmth=0.25, movement=0.86, density=0.87, space=0.08, aggression=0.95),
        dict(CHARACTER=0.9, MOVEMENT=0.86, COUPLING=0.0, SPACE=0.08),
        ["flux", "maximum", "origami", "rage", "fold"],
        "Fold point slammed forward — paper crane becomes shuriken."
    ),
    (
        "Oblique Cascade",
        "Oblique",
        dict(brightness=0.85, warmth=0.2, movement=0.93, density=0.88, space=0.17, aggression=0.9),
        dict(CHARACTER=0.85, MOVEMENT=0.93, COUPLING=0.1, SPACE=0.17),
        ["flux", "maximum", "oblique", "cascade", "prism"],
        "Bouncing prisms past Nyquist — harmonic cascade without ceiling."
    ),
]

CONTROLLED_FLUX = [
    (
        "Orbital Surge",
        "Orbital",
        dict(brightness=0.7, warmth=0.45, movement=0.82, density=0.75, space=0.3, aggression=0.68),
        dict(CHARACTER=0.68, MOVEMENT=0.82, COUPLING=0.12, SPACE=0.3),
        ["flux", "controlled", "orbital", "surge", "structured"],
        "Group envelopes shaping the surge — energy channeled, not unleashed."
    ),
    (
        "Bob Momentum",
        "Oblong",
        dict(brightness=0.65, warmth=0.55, movement=0.78, density=0.72, space=0.28, aggression=0.62),
        dict(CHARACTER=0.72, MOVEMENT=0.78, COUPLING=0.08, SPACE=0.28),
        ["flux", "controlled", "oblong", "momentum", "amber"],
        "Amber warmth with purposeful forward motion — Bob in full stride."
    ),
    (
        "Ocelot Sprint",
        "Ocelot",
        dict(brightness=0.76, warmth=0.5, movement=0.85, density=0.7, space=0.25, aggression=0.7),
        dict(CHARACTER=0.74, MOVEMENT=0.85, COUPLING=0.0, SPACE=0.25),
        ["flux", "controlled", "ocelot", "sprint", "tawny"],
        "Biome set to savanna — ocelot at full speed, all spots in formation."
    ),
    (
        "Olé Charge",
        "Ole",
        dict(brightness=0.8, warmth=0.42, movement=0.87, density=0.78, space=0.22, aggression=0.65),
        dict(CHARACTER=0.76, MOVEMENT=0.87, COUPLING=0.05, SPACE=0.22),
        ["flux", "controlled", "ole", "charge", "afro-latin"],
        "DRAMA macro mid-throw — Afro-Latin trio locked in synchronized charge."
    ),
    (
        "Brass March",
        "Ottoni",
        dict(brightness=0.72, warmth=0.48, movement=0.8, density=0.82, space=0.2, aggression=0.72),
        dict(CHARACTER=0.8, MOVEMENT=0.8, COUPLING=0.1, SPACE=0.2),
        ["flux", "controlled", "ottoni", "brass", "march"],
        "GROW macro building — triple brass advancing in disciplined formation."
    ),
    (
        "Siphon Drive",
        "Orphica",
        dict(brightness=0.68, warmth=0.38, movement=0.75, density=0.68, space=0.32, aggression=0.6),
        dict(CHARACTER=0.62, MOVEMENT=0.75, COUPLING=0.15, SPACE=0.32),
        ["flux", "controlled", "orphica", "siphon", "harp"],
        "Microsound harp driven fast — plucks compressed into a continuous propulsive line."
    ),
    (
        "Shore Surge",
        "Osprey",
        dict(brightness=0.73, warmth=0.4, movement=0.88, density=0.73, space=0.27, aggression=0.67),
        dict(CHARACTER=0.7, MOVEMENT=0.88, COUPLING=0.08, SPACE=0.27),
        ["flux", "controlled", "osprey", "shore", "surge"],
        "ShoreSystem at high tide — coastal wind against structured rhythm."
    ),
    (
        "Oceanic Undertow",
        "Oceanic",
        dict(brightness=0.62, warmth=0.45, movement=0.83, density=0.8, space=0.35, aggression=0.58),
        dict(CHARACTER=0.65, MOVEMENT=0.83, COUPLING=0.12, SPACE=0.35),
        ["flux", "controlled", "oceanic", "undertow", "teal"],
        "Chromatophore modulator cycling fast — tidal pull with predictable return."
    ),
    (
        "Metabolic Push",
        "Organon",
        dict(brightness=0.66, warmth=0.52, movement=0.77, density=0.76, space=0.29, aggression=0.63),
        dict(CHARACTER=0.7, MOVEMENT=0.77, COUPLING=0.18, SPACE=0.29),
        ["flux", "controlled", "organon", "metabolic", "push"],
        "Metabolic rate elevated — variational free energy channeled into forward propulsion."
    ),
    (
        "Oracle Command",
        "Oracle",
        dict(brightness=0.58, warmth=0.35, movement=0.72, density=0.74, space=0.3, aggression=0.74),
        dict(CHARACTER=0.75, MOVEMENT=0.72, COUPLING=0.05, SPACE=0.3),
        ["flux", "controlled", "oracle", "command", "maqam"],
        "GENDY stochastic system with breakpoints tightened — prophecy that obeys a schedule."
    ),
    (
        "Orbital Drill",
        "Orbital",
        dict(brightness=0.74, warmth=0.38, movement=0.9, density=0.79, space=0.2, aggression=0.75),
        dict(CHARACTER=0.75, MOVEMENT=0.9, COUPLING=0.05, SPACE=0.2),
        ["flux", "controlled", "orbital", "drill", "focused"],
        "Group envelopes staggered for drilling rhythm — sustained intensity with internal order."
    ),
    (
        "Tawny Pounce",
        "Ocelot",
        dict(brightness=0.78, warmth=0.55, movement=0.84, density=0.69, space=0.22, aggression=0.64),
        dict(CHARACTER=0.68, MOVEMENT=0.84, COUPLING=0.0, SPACE=0.22),
        ["flux", "controlled", "ocelot", "pounce", "biome"],
        "Low crouch to explosive spring — every gesture deliberate, landing exact."
    ),
    (
        "Bob Ratchet",
        "Oblong",
        dict(brightness=0.62, warmth=0.6, movement=0.76, density=0.71, space=0.3, aggression=0.55),
        dict(CHARACTER=0.66, MOVEMENT=0.76, COUPLING=0.06, SPACE=0.3),
        ["flux", "controlled", "oblong", "ratchet", "amber"],
        "Amber filter ratcheting up — Bob's warmth at working tempo."
    ),
    (
        "Wind Ensemble Rush",
        "Obbligato",
        dict(brightness=0.7, warmth=0.44, movement=0.86, density=0.77, space=0.24, aggression=0.66),
        dict(CHARACTER=0.72, MOVEMENT=0.86, COUPLING=0.12, SPACE=0.24),
        ["flux", "controlled", "obbligato", "wind", "rush"],
        "BOND macro mid-engage — dual wind lines in close disciplined parallel."
    ),
    (
        "Osprey Dive",
        "Osprey",
        dict(brightness=0.77, warmth=0.36, movement=0.89, density=0.74, space=0.2, aggression=0.71),
        dict(CHARACTER=0.73, MOVEMENT=0.89, COUPLING=0.06, SPACE=0.2),
        ["flux", "controlled", "osprey", "dive", "azulejo"],
        "Shore wind behind the dive — controlled plummet with azulejo blue clarity."
    ),
    (
        "Tidal Engine",
        "Oceanic",
        dict(brightness=0.64, warmth=0.48, movement=0.81, density=0.82, space=0.32, aggression=0.6),
        dict(CHARACTER=0.67, MOVEMENT=0.81, COUPLING=0.14, SPACE=0.32),
        ["flux", "controlled", "oceanic", "tidal", "engine"],
        "Separation parameter mid-range — flocking behavior synchronized to a driving pulse."
    ),
    (
        "Prophecy Beat",
        "Oracle",
        dict(brightness=0.6, warmth=0.32, movement=0.74, density=0.73, space=0.28, aggression=0.73),
        dict(CHARACTER=0.77, MOVEMENT=0.74, COUPLING=0.0, SPACE=0.28),
        ["flux", "controlled", "oracle", "prophecy", "beat"],
        "Indigo oracle locking stochastic output to a groove — chaos serving rhythm."
    ),
    (
        "Metabolic Cycle",
        "Organon",
        dict(brightness=0.68, warmth=0.5, movement=0.79, density=0.75, space=0.31, aggression=0.61),
        dict(CHARACTER=0.68, MOVEMENT=0.79, COUPLING=0.2, SPACE=0.31),
        ["flux", "controlled", "organon", "metabolic", "cycle"],
        "Cellular coupling at medium rate — organism in aerobic flux, not anaerobic crisis."
    ),
    (
        "Olé Pivot",
        "Ole",
        dict(brightness=0.82, warmth=0.4, movement=0.83, density=0.76, space=0.26, aggression=0.63),
        dict(CHARACTER=0.74, MOVEMENT=0.83, COUPLING=0.08, SPACE=0.26),
        ["flux", "controlled", "ole", "pivot", "hibiscus"],
        "The pivot before the DRAMA — Afro-Latin swing at precise center of gravity."
    ),
    (
        "Orphica Cascade",
        "Orphica",
        dict(brightness=0.7, warmth=0.36, movement=0.87, density=0.65, space=0.33, aggression=0.57),
        dict(CHARACTER=0.6, MOVEMENT=0.87, COUPLING=0.18, SPACE=0.33),
        ["flux", "controlled", "orphica", "cascade", "seafoam"],
        "Siphonophore colony cascading in sequence — each unit firing in coordinated burst."
    ),
]

TENSION_FLUX = [
    (
        "Obsidian Stare",
        "Obsidian",
        dict(brightness=0.15, warmth=0.12, movement=0.12, density=0.88, space=0.18, aggression=0.92),
        dict(CHARACTER=0.88, MOVEMENT=0.12, COUPLING=0.0, SPACE=0.18),
        ["flux", "tension", "obsidian", "still", "intense"],
        "Crystal surface with interior rage — total stillness, maximum internal pressure."
    ),
    (
        "Daguerreotype Freeze",
        "Obscura",
        dict(brightness=0.3, warmth=0.28, movement=0.15, density=0.82, space=0.2, aggression=0.82),
        dict(CHARACTER=0.82, MOVEMENT=0.15, COUPLING=0.0, SPACE=0.2),
        ["flux", "tension", "obscura", "daguerreotype", "freeze"],
        "Silver plate under pressure — the moment before the image burns through."
    ),
    (
        "Dub Coil",
        "Overdub",
        dict(brightness=0.4, warmth=0.55, movement=0.2, density=0.78, space=0.35, aggression=0.78),
        dict(CHARACTER=0.75, MOVEMENT=0.2, COUPLING=0.05, SPACE=0.35),
        ["flux", "tension", "overdub", "coil", "spring"],
        "Spring reverb wound tight — olive body holding enormous delayed potential."
    ),
    (
        "Opal Pressure",
        "Opal",
        dict(brightness=0.45, warmth=0.38, movement=0.18, density=0.85, space=0.22, aggression=0.85),
        dict(CHARACTER=0.8, MOVEMENT=0.18, COUPLING=0.08, SPACE=0.22),
        ["flux", "tension", "opal", "pressure", "granular"],
        "Granular cloud suspended — grain scatter frozen at maximum density, waiting."
    ),
    (
        "Ombre Fault Line",
        "Ombre",
        dict(brightness=0.35, warmth=0.42, movement=0.22, density=0.8, space=0.25, aggression=0.88),
        dict(CHARACTER=0.84, MOVEMENT=0.22, COUPLING=0.1, SPACE=0.25),
        ["flux", "tension", "ombre", "fault", "mauve"],
        "Memory and forgetting locked in opposition — the shadow before the rupture."
    ),
    (
        "Abyssal Hold",
        "Owlfish",
        dict(brightness=0.2, warmth=0.18, movement=0.1, density=0.9, space=0.15, aggression=0.9),
        dict(CHARACTER=0.9, MOVEMENT=0.1, COUPLING=0.0, SPACE=0.15),
        ["flux", "tension", "owlfish", "abyssal", "hold"],
        "Mixtur-Trautonium with bowed pressure — subharmonic mass at dead stop."
    ),
    (
        "Commune Standoff",
        "Ohm",
        dict(brightness=0.55, warmth=0.6, movement=0.25, density=0.76, space=0.28, aggression=0.72),
        dict(CHARACTER=0.72, MOVEMENT=0.25, COUPLING=0.15, SPACE=0.28),
        ["flux", "tension", "ohm", "commune", "standoff"],
        "MEDDLING vs COMMUNE frozen mid-axis — the communal argument at peak intensity."
    ),
    (
        "Oscar Suppress",
        "Oddoscar",
        dict(brightness=0.38, warmth=0.48, movement=0.28, density=0.84, space=0.2, aggression=0.84),
        dict(CHARACTER=0.84, MOVEMENT=0.28, COUPLING=0.05, SPACE=0.2),
        ["flux", "tension", "oddoscar", "suppress", "axolotl"],
        "Axolotl gill pink beneath still water — morph parameter caged at maximum saturation."
    ),
    (
        "Odyssey Lock",
        "Odyssey",
        dict(brightness=0.48, warmth=0.3, movement=0.16, density=0.87, space=0.22, aggression=0.87),
        dict(CHARACTER=0.87, MOVEMENT=0.16, COUPLING=0.0, SPACE=0.22),
        ["flux", "tension", "odyssey", "lock", "violet"],
        "Drift envelopes frozen at peak sustain — violet pressure behind glass."
    ),
    (
        "Origami Tension",
        "Origami",
        dict(brightness=0.62, warmth=0.28, movement=0.2, density=0.86, space=0.18, aggression=0.86),
        dict(CHARACTER=0.86, MOVEMENT=0.2, COUPLING=0.0, SPACE=0.18),
        ["flux", "tension", "origami", "tension", "fold"],
        "Paper creased to its limit — fold point held back from the final tear."
    ),
    (
        "Crystal Threat",
        "Obsidian",
        dict(brightness=0.12, warmth=0.08, movement=0.08, density=0.92, space=0.14, aggression=0.95),
        dict(CHARACTER=0.92, MOVEMENT=0.08, COUPLING=0.0, SPACE=0.14),
        ["flux", "tension", "obsidian", "crystal", "threat"],
        "Pure obsidian density with zero release — structural threat without a single movement."
    ),
    (
        "Plate Compression",
        "Obscura",
        dict(brightness=0.28, warmth=0.32, movement=0.18, density=0.85, space=0.18, aggression=0.8),
        dict(CHARACTER=0.8, MOVEMENT=0.18, COUPLING=0.0, SPACE=0.18),
        ["flux", "tension", "obscura", "plate", "compression"],
        "Stiffness model at maximum tension — silver daguerreotype plate about to crack."
    ),
    (
        "Spring Threat",
        "Overdub",
        dict(brightness=0.42, warmth=0.5, movement=0.25, density=0.77, space=0.38, aggression=0.75),
        dict(CHARACTER=0.73, MOVEMENT=0.25, COUPLING=0.0, SPACE=0.38),
        ["flux", "tension", "overdub", "spring", "threat"],
        "Tape delay held at loop boundary — the moment before the spring snaps back."
    ),
    (
        "Grain Storm Frozen",
        "Opal",
        dict(brightness=0.42, warmth=0.4, movement=0.15, density=0.88, space=0.2, aggression=0.82),
        dict(CHARACTER=0.82, MOVEMENT=0.15, COUPLING=0.1, SPACE=0.2),
        ["flux", "tension", "opal", "grain", "frozen"],
        "Granular storm arrested mid-scatter — lavender clouds of maximum density, no escape velocity."
    ),
    (
        "Fault Memory",
        "Ombre",
        dict(brightness=0.3, warmth=0.45, movement=0.28, density=0.83, space=0.22, aggression=0.84),
        dict(CHARACTER=0.82, MOVEMENT=0.28, COUPLING=0.12, SPACE=0.22),
        ["flux", "tension", "ombre", "memory", "fault"],
        "Two narrative layers in maximum conflict — forgetting fighting memory at fault line."
    ),
    (
        "Owlfish Stalk",
        "Owlfish",
        dict(brightness=0.18, warmth=0.22, movement=0.12, density=0.89, space=0.16, aggression=0.88),
        dict(CHARACTER=0.88, MOVEMENT=0.12, COUPLING=0.0, SPACE=0.16),
        ["flux", "tension", "owlfish", "stalk", "abyssal"],
        "Gold bioluminescence suspended — every subharmonic coiled, not one emitted."
    ),
    (
        "Ohm Threshold",
        "Ohm",
        dict(brightness=0.52, warmth=0.62, movement=0.22, density=0.74, space=0.3, aggression=0.74),
        dict(CHARACTER=0.74, MOVEMENT=0.22, COUPLING=0.18, SPACE=0.3),
        ["flux", "tension", "ohm", "threshold", "sage"],
        "COMMUNE macro at decision threshold — the commune before it either harmonizes or fractures."
    ),
    (
        "Oscar Pressure",
        "Oddoscar",
        dict(brightness=0.35, warmth=0.52, movement=0.26, density=0.86, space=0.19, aggression=0.86),
        dict(CHARACTER=0.86, MOVEMENT=0.26, COUPLING=0.08, SPACE=0.19),
        ["flux", "tension", "oddoscar", "pressure", "morph"],
        "Morph fully loaded but unmoved — gill pink compressed under motionless surface."
    ),
    (
        "Violet Siege",
        "Odyssey",
        dict(brightness=0.45, warmth=0.28, movement=0.2, density=0.9, space=0.2, aggression=0.9),
        dict(CHARACTER=0.9, MOVEMENT=0.2, COUPLING=0.0, SPACE=0.2),
        ["flux", "tension", "odyssey", "siege", "violet"],
        "Odyssey osc stack at max density, minimal movement — the violet siege before the drift."
    ),
    (
        "Fold Standstill",
        "Origami",
        dict(brightness=0.6, warmth=0.3, movement=0.14, density=0.85, space=0.16, aggression=0.83),
        dict(CHARACTER=0.85, MOVEMENT=0.14, COUPLING=0.0, SPACE=0.16),
        ["flux", "tension", "origami", "standstill", "vermillion"],
        "The crane mid-fold — vermillion crease visible, hands arrested, paper at maximum stress."
    ),
]

# ---------------------------------------------------------------------------

def make_preset(name, engine, dna, macros, tags, description):
    # Engine IDs in presets use Title Case (matching existing files)
    engine_title = engine
    return {
        "schema_version": 1,
        "author": "XO_OX Designs",
        "version": "1.0",
        "name": name,
        "mood": "Flux",
        "engines": [engine_title],
        "couplingIntensity": "None",
        "tempo": None,
        "macroLabels": ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"],
        "macros": macros,
        "coupling": {"pairs": []},
        "sequencer": None,
        "dna": dna,
        "parameters": {},
        "tags": tags,
        "description": description,
    }


def filename_from_name(name):
    # Replace spaces with underscores, keep alphanumeric + underscore + hyphen
    safe = "".join(c if c.isalnum() or c in " -'" else "" for c in name)
    safe = safe.strip().replace(" ", "_")
    return safe + ".xometa"


def main():
    os.makedirs(PRESET_DIR, exist_ok=True)

    all_groups = [
        ("Maximum Flux", MAXIMUM_FLUX),
        ("Controlled Flux", CONTROLLED_FLUX),
        ("Tension Flux", TENSION_FLUX),
    ]

    written = 0
    skipped = 0

    for group_name, presets in all_groups:
        print(f"\n--- {group_name} ---")
        for entry in presets:
            name, engine, dna, macros, tags, description = entry
            fname = filename_from_name(name)
            fpath = os.path.join(PRESET_DIR, fname)

            if os.path.exists(fpath):
                print(f"  SKIP  {fname}")
                skipped += 1
                continue

            preset = make_preset(name, engine, dna, macros, tags, description)
            with open(fpath, "w", encoding="utf-8") as f:
                json.dump(preset, f, indent=2)
                f.write("\n")

            print(f"  WRITE {fname}")
            written += 1

    print(f"\nDone. Written: {written}, Skipped: {skipped}")


if __name__ == "__main__":
    main()
