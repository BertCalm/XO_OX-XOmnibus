#!/usr/bin/env python3
"""Generate coupling presets deepening coverage for OBLONG, ONSET, and OVERWORLD.

Rhythm/chip engine trio:
  OBLONG    prefix: bob_   accent: Amber #E9A84A
  ONSET     prefix: perc_  accent: Electric Blue #0066FF
  OVERWORLD prefix: ow_    accent: Neon Green #39FF14

Outputs ~66 presets to Presets/XOceanus/Entangled/.
Skips any file that already exists.
"""

import json
import os

REPO_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
ENTANGLED_DIR = os.path.join(REPO_ROOT, "Presets", "XOceanus", "Entangled")

# ---------------------------------------------------------------------------
# DNA baselines
# ---------------------------------------------------------------------------
DNA_OBLONG = {"brightness": 0.65, "warmth": 0.65, "movement": 0.55,
               "density": 0.7, "space": 0.4, "aggression": 0.5}
DNA_ONSET  = {"brightness": 0.6,  "warmth": 0.5,  "movement": 0.75,
               "density": 0.7, "space": 0.4, "aggression": 0.6}
DNA_OW     = {"brightness": 0.8,  "warmth": 0.4,  "movement": 0.7,
               "density": 0.5, "space": 0.5, "aggression": 0.5}


def blend_dna(*dnas):
    """Average multiple DNA dicts."""
    keys = list(dnas[0].keys())
    return {k: round(sum(d[k] for d in dnas) / len(dnas), 3) for k in keys}


def nudge(dna, **overrides):
    d = dict(dna)
    d.update(overrides)
    return d


# ---------------------------------------------------------------------------
# Builder helpers
# ---------------------------------------------------------------------------

def make_preset(name, engines, desc, tags, dna,
                coupling_pairs, macro_labels=None,
                intensity="Moderate", tempo=None, parameters=None):
    return {
        "schema_version": 1,
        "name": name,
        "mood": "Entangled",
        "engines": list(engines),
        "author": "XO_OX Designs",
        "version": "1.0.0",
        "description": desc,
        "tags": ["coupling", "entangled"] + tags,
        "macroLabels": macro_labels or ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"],
        "couplingIntensity": intensity,
        "tempo": tempo,
        "dna": dna,
        "parameters": parameters or {},
        "coupling": {"pairs": coupling_pairs},
        "sequencer": None,
    }


def pair(engine_a, engine_b, ctype, amount):
    return {"engineA": engine_a, "engineB": engine_b, "type": ctype, "amount": amount}


def write_preset(preset, directory=ENTANGLED_DIR):
    os.makedirs(directory, exist_ok=True)
    filename = preset["name"] + ".xometa"
    filepath = os.path.join(directory, filename)
    if os.path.exists(filepath):
        print(f"  SKIP (exists): {filename}")
        return None
    with open(filepath, "w") as f:
        json.dump(preset, f, indent=2)
        f.write("\n")
    print(f"  WRITE: {filename}")
    return filepath


# ---------------------------------------------------------------------------
# 1. THREE-WAY MARQUEE: OBLONG × ONSET × OVERWORLD  (6 presets)
# ---------------------------------------------------------------------------

MARQUEE = [
    make_preset(
        name="Amber Circuit Jam",
        engines=["Oblong", "Onset", "Overworld"],
        desc=(
            "The three-rhythm conclave. OBLONG's curiosity engine drives groove irregularity, "
            "ONSET punches in hard transients, OVERWORLD delivers NES/Genesis era pixel melody. "
            "Every hit spawns a glitch shimmer from the chip layer."
        ),
        tags=["oblong", "onset", "overworld", "marquee", "chiptune", "beat", "groove"],
        dna=blend_dna(DNA_OBLONG, DNA_ONSET, DNA_OW),
        macro_labels=["GROOVE", "PUNCH", "ERA", "SPACE"],
        coupling_pairs=[
            pair("Onset",    "Overworld", "Rhythm->Blend",   0.65),
            pair("Oblong",   "Onset",     "Amp->Filter",      0.55),
            pair("Overworld","Oblong",    "Pitch->Character", 0.45),
        ],
        intensity="Deep",
        tempo=120,
        parameters={
            "Oblong":    {"bob_groove": 0.6, "bob_swing": 0.35, "bob_density": 0.7,
                          "bob_curiousRate": 0.5, "bob_amberMix": 0.65},
            "Onset":     {"perc_v1_level": 0.85, "perc_v1_decay": 0.45,
                          "perc_v2_level": 0.7,  "perc_v2_decay": 0.3,
                          "perc_v3_level": 0.55, "perc_macro_punch": 0.7},
            "Overworld": {"ow_eraCross": 0.5, "ow_glitch": 0.3, "ow_arpRate": 90.0,
                          "ow_oscWave": 1, "ow_filterCut": 6000.0},
        },
    ),

    make_preset(
        name="8-Bit Riddim Council",
        engines=["Oblong", "Onset", "Overworld"],
        desc=(
            "Dancehall-influenced beat architecture. ONSET lays the one-drop pattern, "
            "OBLONG decorates offbeats with amber warmth, OVERWORLD's FM bass occupies "
            "the low register. Dub space grows between hits."
        ),
        tags=["oblong", "onset", "overworld", "marquee", "riddim", "dub", "chiptune"],
        dna=nudge(blend_dna(DNA_OBLONG, DNA_ONSET, DNA_OW), space=0.55, warmth=0.55),
        macro_labels=["RIDDIM", "WEIGHT", "FM DEPTH", "DUB SPACE"],
        coupling_pairs=[
            pair("Onset",    "Oblong",    "Amp->Filter",    0.6),
            pair("Oblong",   "Overworld", "Rhythm->Blend",  0.5),
            pair("Overworld","Onset",     "Pitch->Velocity",0.4),
        ],
        intensity="Moderate",
        tempo=100,
        parameters={
            "Oblong":    {"bob_swing": 0.5, "bob_density": 0.55, "bob_warmth": 0.7},
            "Onset":     {"perc_v1_level": 0.9, "perc_v2_level": 0.5,
                          "perc_macro_punch": 0.65, "perc_macro_space": 0.5},
            "Overworld": {"ow_eraCross": 0.25, "ow_subOct": 1,
                          "ow_filterCut": 4000.0, "ow_reverbMix": 0.35},
        },
    ),

    make_preset(
        name="Pixel Polyrhythm Engine",
        engines=["Oblong", "Onset", "Overworld"],
        desc=(
            "Polyrhythmic grid where each engine runs a different subdivision. "
            "OBLONG holds the triplet, ONSET lands the 16th grid, OVERWORLD "
            "marks the bar with a chip melody phrase. Dense interlocking machinery."
        ),
        tags=["oblong", "onset", "overworld", "marquee", "polyrhythm", "grid", "chiptune"],
        dna=nudge(blend_dna(DNA_OBLONG, DNA_ONSET, DNA_OW), density=0.82, movement=0.8),
        macro_labels=["DENSITY", "MACHINE", "GLITCH", "SPACE"],
        coupling_pairs=[
            pair("Oblong",   "Onset",     "Rhythm->Blend",  0.7),
            pair("Onset",    "Overworld", "Amp->Filter",    0.6),
            pair("Overworld","Oblong",    "Env->Character", 0.5),
        ],
        intensity="Deep",
        tempo=128,
        parameters={
            "Oblong":    {"bob_groove": 0.7, "bob_density": 0.8, "bob_curiousRate": 0.6},
            "Onset":     {"perc_v1_level": 0.8, "perc_v3_level": 0.65,
                          "perc_v4_level": 0.55, "perc_macro_punch": 0.75},
            "Overworld": {"ow_eraCross": 0.6, "ow_arpRate": 120.0, "ow_glitch": 0.45,
                          "ow_filterCut": 8000.0},
        },
    ),

    make_preset(
        name="NES Boom Bap",
        engines=["Oblong", "Onset", "Overworld"],
        desc=(
            "Classic boom-bap hip-hop skeleton fused with NES-era nostalgia. "
            "ONSET delivers the kick-snare foundation, OBLONG handles the "
            "amber hat groove, OVERWORLD plays the sample-chop melody role."
        ),
        tags=["oblong", "onset", "overworld", "marquee", "boom-bap", "hip-hop", "nes"],
        dna=nudge(blend_dna(DNA_OBLONG, DNA_ONSET, DNA_OW), warmth=0.6, brightness=0.65),
        macro_labels=["SWING", "BOOM", "CHOP", "AIR"],
        coupling_pairs=[
            pair("Onset",    "Oblong",    "Rhythm->Blend",   0.55),
            pair("Oblong",   "Overworld", "Amp->Filter",     0.5),
            pair("Overworld","Onset",     "Pitch->Character",0.35),
        ],
        intensity="Moderate",
        tempo=90,
        parameters={
            "Oblong":    {"bob_swing": 0.6, "bob_density": 0.5, "bob_amberMix": 0.6},
            "Onset":     {"perc_v1_level": 0.9, "perc_v2_level": 0.8,
                          "perc_macro_punch": 0.7, "perc_macro_space": 0.35},
            "Overworld": {"ow_eraCross": 0.35, "ow_oscWave": 0, "ow_arpRate": 70.0,
                          "ow_filterCut": 5000.0, "ow_reverbMix": 0.25},
        },
    ),

    make_preset(
        name="Glitch Foundry",
        engines=["Oblong", "Onset", "Overworld"],
        desc=(
            "Industrial glitch production unit. OVERWORLD's glitch events cascade "
            "into OBLONG's character pipeline, ONSET provides the metronome of "
            "destruction. Every bar ends differently. Beautiful chaos."
        ),
        tags=["oblong", "onset", "overworld", "marquee", "glitch", "industrial", "chaos"],
        dna=nudge(blend_dna(DNA_OBLONG, DNA_ONSET, DNA_OW), aggression=0.72, space=0.45),
        macro_labels=["CHAOS", "PUNCH", "GLITCH", "DECAY"],
        coupling_pairs=[
            pair("Overworld","Oblong",    "Env->Character", 0.75),
            pair("Oblong",   "Onset",     "Amp->Filter",    0.65),
            pair("Onset",    "Overworld", "Rhythm->Blend",  0.7),
        ],
        intensity="Deep",
        tempo=140,
        parameters={
            "Oblong":    {"bob_groove": 0.55, "bob_density": 0.75, "bob_curiousRate": 0.8},
            "Onset":     {"perc_v1_level": 0.85, "perc_v5_level": 0.7,
                          "perc_macro_punch": 0.85, "perc_macro_machine": 0.7},
            "Overworld": {"ow_glitch": 0.8, "ow_eraCross": 0.7, "ow_filterCut": 9000.0,
                          "ow_oscWave": 2, "ow_reverbMix": 0.2},
        },
    ),

    make_preset(
        name="Amber Cartridge",
        engines=["Oblong", "Onset", "Overworld"],
        desc=(
            "Warm lo-fi cartridge aesthetic. OBLONG's amber character coats every "
            "element in analog warmth, ONSET provides dusty drum hits, OVERWORLD "
            "runs a mellow SNES-era pad melody underneath. "
            "Nostalgia in crystalline form."
        ),
        tags=["oblong", "onset", "overworld", "marquee", "lo-fi", "snes", "amber", "warm"],
        dna=nudge(blend_dna(DNA_OBLONG, DNA_ONSET, DNA_OW), warmth=0.7, aggression=0.35,
                  space=0.55, brightness=0.58),
        macro_labels=["WARMTH", "DUST", "ERA", "SPACE"],
        coupling_pairs=[
            pair("Oblong",   "Overworld", "Amp->Filter",    0.55),
            pair("Overworld","Onset",     "Pitch->Velocity",0.45),
            pair("Onset",    "Oblong",    "Rhythm->Blend",  0.4),
        ],
        intensity="Moderate",
        tempo=85,
        parameters={
            "Oblong":    {"bob_warmth": 0.8, "bob_swing": 0.4, "bob_amberMix": 0.75,
                          "bob_density": 0.5},
            "Onset":     {"perc_v1_level": 0.75, "perc_v2_level": 0.7,
                          "perc_macro_punch": 0.5, "perc_macro_space": 0.55},
            "Overworld": {"ow_eraCross": 0.2, "ow_oscWave": 3, "ow_reverbMix": 0.45,
                          "ow_filterCut": 4500.0, "ow_glitch": 0.1},
        },
    ),
]

# ---------------------------------------------------------------------------
# 2. OBLONG × PARTNER  (20 presets — one per partner listed)
# ---------------------------------------------------------------------------

OBLONG_PAIRS = [
    # (partner_engine, name, desc, tags, coupling_pairs, dna_overrides, parameters)
    ("Optic", "Amber Lens Groove",
     "OBLONG's rhythmic curiosity feeds OPTIC's lens-flare modulation. "
     "Every bob-pattern shift bends the light differently. Warm stroboscopic groove.",
     ["oblong", "optic", "beat", "warm", "lens"],
     [pair("Oblong","Optic","Rhythm->Blend",0.6), pair("Optic","Oblong","Env->Character",0.4)],
     {"warmth":0.7,"brightness":0.7},
     {"Oblong":{"bob_groove":0.6,"bob_swing":0.35,"bob_amberMix":0.65},
      "Optic":{"opt_lensFlare":0.5,"opt_aberration":0.3,"opt_filterCut":5500.0}}),

    ("Oblique", "Bob Bounce Trap",
     "OBLONG swings through OBLIQUE's bounce engine. Hat rhythms become prismatic "
     "ricochet sequences — every hi-hat fires a spectral echo tail.",
     ["oblong", "oblique", "beat", "bounce", "prism"],
     [pair("Oblong","Oblique","Amp->Filter",0.65), pair("Oblique","Oblong","Pitch->Character",0.35)],
     {"movement":0.7,"brightness":0.7},
     {"Oblong":{"bob_groove":0.65,"bob_density":0.65,"bob_swing":0.4},
      "Oblique":{"oblq_bounceGravity":0.55,"oblq_bounceCnt":8.0,"oblq_prismMix":0.45}}),

    ("Ocelot", "Floor Tom Stalk",
     "OCELOT's predator-gate threshold triggered by OBLONG's low-frequency pulses. "
     "Deep amber floor tom meets hunting-cat envelope snap.",
     ["oblong", "ocelot", "beat", "drum", "predator"],
     [pair("Oblong","Ocelot","Amp->Filter",0.6), pair("Ocelot","Oblong","Env->Character",0.45)],
     {"warmth":0.65,"density":0.72},
     {"Oblong":{"bob_density":0.7,"bob_warmth":0.65,"bob_groove":0.5},
      "Ocelot":{"oct_gate":0.6,"oct_attack":0.001,"oct_decay":0.4,"oct_ratio":4.0}}),

    ("Osprey", "Dive Pattern",
     "OBLONG's irregular bob-patterns become OSPREY dive-bomb velocity curves. "
     "Rhythmic curiosity meets aerial predator precision.",
     ["oblong", "osprey", "beat", "dive", "velocity"],
     [pair("Oblong","Osprey","Rhythm->Blend",0.55), pair("Osprey","Oblong","Pitch->Character",0.4)],
     {"movement":0.68,"brightness":0.6},
     {"Oblong":{"bob_curiousRate":0.6,"bob_groove":0.55,"bob_density":0.6},
      "Osprey":{"osp_pitchDive":0.5,"osp_diveTail":0.4,"osp_wingSpan":0.35}}),

    ("Osteria", "Amber Kitchen Rhythm",
     "Warm tavern percussion — OBLONG's groove regularity feeds OSTERIA's "
     "communal pulse. Handpan meets amber warmth in a shared space.",
     ["oblong", "osteria", "beat", "communal", "warm"],
     [pair("Oblong","Osteria","Rhythm->Blend",0.5), pair("Osteria","Oblong","Amp->Filter",0.4)],
     {"warmth":0.72,"density":0.65,"space":0.5},
     {"Oblong":{"bob_swing":0.4,"bob_warmth":0.7,"bob_groove":0.5},
      "Osteria":{"ost_communalPulse":0.55,"ost_roomMix":0.4,"ost_warmth":0.65}}),

    ("Oceanic", "Amber Tide Machine",
     "OBLONG's amber density acts as the breathing mechanism for OCEANIC's "
     "wave engine. Rhythmic inhale and exhale across the water column.",
     ["oblong", "oceanic", "beat", "tide", "underwater"],
     [pair("Oblong","Oceanic","Amp->Filter",0.6), pair("Oceanic","Oblong","Env->Character",0.45)],
     {"warmth":0.65,"space":0.55,"density":0.65},
     {"Oblong":{"bob_density":0.65,"bob_warmth":0.65,"bob_groove":0.45},
      "Oceanic":{"oce_tideRate":0.4,"oce_depth":0.55,"oce_filterCut":3500.0}}),

    ("Owlfish", "Subharmonic Groove",
     "OWLFISH's deep subharmonic partials entangle with OBLONG's midrange amber "
     "groove. Orbital curiosity meets bioluminescent depth.",
     ["oblong", "owlfish", "beat", "subharmonic", "deep"],
     [pair("Oblong","Owlfish","Rhythm->Blend",0.55), pair("Owlfish","Oblong","Pitch->Character",0.5)],
     {"warmth":0.65,"density":0.72,"space":0.45},
     {"Oblong":{"bob_groove":0.55,"bob_warmth":0.65,"bob_density":0.7},
      "Owlfish":{"owl_subMix":0.55,"owl_subDiv1":1,"owl_subDiv2":2,"owl_mixtur":0.35}}),

    ("Ohm", "Hippy Amber Jam",
     "OBLONG's groove curiosity feeds OHM's communal jam engine. "
     "Beatnik percussion meets commune consciousness in amber amber light.",
     ["oblong", "ohm", "beat", "communal", "groove"],
     [pair("Oblong","Ohm","Rhythm->Blend",0.5), pair("Ohm","Oblong","Amp->Filter",0.4)],
     {"warmth":0.68,"movement":0.62,"space":0.5},
     {"Oblong":{"bob_swing":0.45,"bob_groove":0.5,"bob_warmth":0.65},
      "Ohm":{"ohm_commune":0.55,"ohm_meddling":0.35,"ohm_warmth":0.6}}),

    ("Orphica", "Microsound Amber Harp",
     "ORPHICA's microsound harp strings respond to OBLONG's amber pulse triggers. "
     "Percussive plucks scatter into microsonic clouds.",
     ["oblong", "orphica", "beat", "microsound", "harp"],
     [pair("Oblong","Orphica","Amp->Filter",0.6), pair("Orphica","Oblong","Env->Character",0.4)],
     {"brightness":0.7,"warmth":0.62,"movement":0.65},
     {"Oblong":{"bob_curiousRate":0.5,"bob_groove":0.5,"bob_density":0.6},
      "Orphica":{"orph_grainSize":0.3,"orph_scatter":0.55,"orph_harpDensity":0.6}}),

    ("Obbligato", "Amber Wind Rhythm",
     "OBBLIGATO's dual-wind voices are punctuated by OBLONG's groove accents. "
     "Percussion becomes phrase-breath markers. BOND macro tightens the partnership.",
     ["oblong", "obbligato", "beat", "wind", "bond"],
     [pair("Oblong","Obbligato","Rhythm->Blend",0.55), pair("Obbligato","Oblong","Pitch->Character",0.4)],
     {"warmth":0.65,"movement":0.65},
     {"Oblong":{"bob_groove":0.55,"bob_warmth":0.6,"bob_density":0.6},
      "Obbligato":{"obbl_bond":0.65,"obbl_breathRate":0.5,"obbl_windMix":0.55}}),

    ("Ottoni", "Brass Amber Pocket",
     "OTTONI's triple brass attack gates are triggered and shaped by OBLONG's "
     "amber pocket groove. GROW macro expands with each drum accent.",
     ["oblong", "ottoni", "beat", "brass", "grow"],
     [pair("Oblong","Ottoni","Amp->Filter",0.65), pair("Ottoni","Oblong","Env->Character",0.45)],
     {"warmth":0.65,"brightness":0.68,"aggression":0.55},
     {"Oblong":{"bob_groove":0.6,"bob_density":0.65,"bob_amberMix":0.6},
      "Ottoni":{"otto_grow":0.6,"otto_brassAttack":0.2,"otto_ensemble":0.55}}),

    ("Ole", "Afro-Amber Ritual",
     "OLE's Afro-Latin trio speaks directly to OBLONG's rhythmic curiosity. "
     "DRAMA macro climbs as the groove intensifies. Call and response architecture.",
     ["oblong", "ole", "beat", "afro-latin", "drama", "ritual"],
     [pair("Oblong","Ole","Rhythm->Blend",0.65), pair("Ole","Oblong","Amp->Filter",0.5)],
     {"warmth":0.7,"movement":0.72,"aggression":0.55},
     {"Oblong":{"bob_swing":0.5,"bob_groove":0.65,"bob_density":0.65},
      "Ole":{"ole_drama":0.6,"ole_clave":0.55,"ole_percMix":0.65}}),

    ("Ombre", "Amber Dusk Percussion",
     "OMBRE's gradient shadow-tone pairs with OBLONG's amber warmth for "
     "twilight percussion textures. Groove dissolves into silhouette.",
     ["oblong", "ombre", "beat", "dusk", "gradient", "warm"],
     [pair("Oblong","Ombre","Amp->Filter",0.55), pair("Ombre","Oblong","Env->Character",0.4)],
     {"warmth":0.7,"brightness":0.55,"space":0.5},
     {"Oblong":{"bob_warmth":0.7,"bob_groove":0.45,"bob_density":0.6},
      "Ombre":{"ombr_gradient":0.55,"ombr_shadow":0.4,"ombr_filterCut":4000.0}}),

    ("Orca", "Amber Breach Rhythm",
     "ORCA's dramatic breach events punctuate OBLONG's rolling amber groove. "
     "High-impact transient explosions between steady mid-tempo pulses.",
     ["oblong", "orca", "beat", "breach", "high-impact"],
     [pair("Oblong","Orca","Rhythm->Blend",0.6), pair("Orca","Oblong","Amp->Filter",0.55)],
     {"warmth":0.6,"aggression":0.6,"movement":0.68},
     {"Oblong":{"bob_groove":0.6,"bob_density":0.65,"bob_swing":0.35},
      "Orca":{"orca_breach":0.65,"orca_tail":0.4,"orca_impact":0.7}}),

    ("Octopus", "Amber Sucker Grid",
     "OCTOPUS's 8-arm trigger grid receives OBLONG's groove information. "
     "Each arm decorates the beat with a different textural element.",
     ["oblong", "octopus", "beat", "grid", "arms"],
     [pair("Oblong","Octopus","Rhythm->Blend",0.65), pair("Octopus","Oblong","Env->Character",0.45)],
     {"density":0.75,"movement":0.7,"aggression":0.55},
     {"Oblong":{"bob_density":0.7,"bob_groove":0.6,"bob_curiousRate":0.55},
      "Octopus":{"oct_armBalance":0.5,"oct_intelRate":0.55,"oct_inkDensity":0.4}}),

    ("Overlap", "Amber FDN Pulse",
     "OBLONG's amber groove drives OVERLAP's knot-topology FDN. "
     "Each beat pulse extends into a tangled diffusion cloud. Rhythm as architecture.",
     ["oblong", "overlap", "beat", "fdn", "reverb", "topology"],
     [pair("Oblong","Overlap","Amp->Filter",0.6), pair("Overlap","Oblong","Pitch->Character",0.4)],
     {"warmth":0.65,"space":0.6,"movement":0.6},
     {"Oblong":{"bob_groove":0.5,"bob_warmth":0.65,"bob_density":0.6},
      "Overlap":{"olap_diffusion":0.6,"olap_knotTopo":0.55,"olap_decay":0.65}}),

    ("Outwit", "Amber Wolfram Beat",
     "OBLONG's groove pattern seeds OUTWIT's 8-arm Wolfram cellular automaton. "
     "Simple amber pulse generates complex rhythmic evolution.",
     ["oblong", "outwit", "beat", "wolfram", "automaton"],
     [pair("Oblong","Outwit","Rhythm->Blend",0.65), pair("Outwit","Oblong","Env->Character",0.5)],
     {"density":0.72,"movement":0.7,"aggression":0.55},
     {"Oblong":{"bob_groove":0.6,"bob_density":0.65,"bob_curiousRate":0.6},
      "Outwit":{"owit_wolfRule":30,"owit_armDensity":0.55,"owit_caRate":0.6}}),

    ("Oracle", "Amber Prophecy Groove",
     "OBLONG lays the temporal groove, ORACLE interprets the pattern. "
     "Every four bars, the oracle reshapes the pocket. Prescient rhythm.",
     ["oblong", "oracle", "beat", "prophecy", "evolving"],
     [pair("Oblong","Oracle","Rhythm->Blend",0.55), pair("Oracle","Oblong","Env->Character",0.5)],
     {"warmth":0.65,"movement":0.65,"space":0.5},
     {"Oblong":{"bob_groove":0.55,"bob_swing":0.4,"bob_density":0.6},
      "Oracle":{"orc_prophecy":0.55,"orc_morphRate":0.4,"orc_resonance":0.5}}),

    ("Organon", "Amber Logic Beat",
     "ORGANON's logical structure organizes OBLONG's amber curiosity. "
     "Rule-based beat intelligence meets warm groove character.",
     ["oblong", "organon", "beat", "logic", "structure"],
     [pair("Oblong","Organon","Amp->Filter",0.55), pair("Organon","Oblong","Pitch->Character",0.45)],
     {"warmth":0.65,"density":0.7,"movement":0.62},
     {"Oblong":{"bob_warmth":0.65,"bob_density":0.65,"bob_groove":0.55},
      "Organon":{"org_logic":0.55,"org_density":0.6,"org_filterCut":5000.0}}),

    ("Ouroboros", "Amber Strange Loop",
     "OUROBOROS feeds its output back through OBLONG's character filter. "
     "The groove serpent eats its own tail in amber warmth. Eternal return.",
     ["oblong", "ouroboros", "beat", "loop", "serpent", "recursive"],
     [pair("Oblong","Ouroboros","Rhythm->Blend",0.6), pair("Ouroboros","Oblong","Amp->Filter",0.65)],
     {"warmth":0.65,"density":0.72,"movement":0.65,"aggression":0.52},
     {"Oblong":{"bob_groove":0.6,"bob_density":0.7,"bob_amberMix":0.65},
      "Ouroboros":{"ouro_feedback":0.6,"ouro_serpentine":0.55,"ouro_decay":0.5}}),
]

# ---------------------------------------------------------------------------
# 3. ONSET × PARTNER  (20 presets)
# ---------------------------------------------------------------------------

ONSET_PAIRS = [
    ("Optic", "Flash Trigger Lens",
     "ONSET's transient flashes modulate OPTIC's lens-flare intensity. "
     "Kick triggers bloom, snare triggers aberration. Visual rhythm.",
     ["onset", "optic", "drum", "transient", "lens"],
     [pair("Onset","Optic","Amp->Filter",0.7), pair("Optic","Onset","Env->Character",0.4)],
     {"brightness":0.72,"movement":0.78,"aggression":0.62},
     {"Onset":{"perc_v1_level":0.85,"perc_v2_level":0.75,"perc_macro_punch":0.7},
      "Optic":{"opt_lensFlare":0.65,"opt_aberration":0.45,"opt_filterCut":7000.0}}),

    ("Oblique", "Snare Ricochet Array",
     "ONSET snare events trigger OBLIQUE's full bounce cascade. "
     "Standard trap snare explodes into prismatic ricochet geometry.",
     ["onset", "oblique", "drum", "bounce", "snare"],
     [pair("Onset","Oblique","Amp->Filter",0.75), pair("Oblique","Onset","Pitch->Character",0.4)],
     {"brightness":0.72,"movement":0.82,"aggression":0.65},
     {"Onset":{"perc_v2_level":0.85,"perc_v2_decay":0.3,"perc_macro_punch":0.75},
      "Oblique":{"oblq_bounceGravity":0.6,"oblq_bounceCnt":12.0,"oblq_prismMix":0.5,"oblq_prismColor":0.7}}),

    ("Ocelot", "Predator Beat Gate",
     "OCELOT's gate mechanism locked to ONSET's kick pattern. "
     "Sidechain-style predator envelope sculpts each drum hit's dynamics.",
     ["onset", "ocelot", "drum", "gate", "sidechain"],
     [pair("Onset","Ocelot","Amp->Filter",0.7), pair("Ocelot","Onset","Env->Character",0.5)],
     {"density":0.75,"movement":0.78,"aggression":0.68},
     {"Onset":{"perc_v1_level":0.9,"perc_v1_decay":0.5,"perc_macro_punch":0.8},
      "Ocelot":{"oct_gate":0.7,"oct_attack":0.001,"oct_ratio":6.0,"oct_release":0.2}}),

    ("Osprey", "Dive Bomb Fill",
     "OSPREY's pitch-dive triggered per ONSET fill pattern. "
     "Tom rolls become cascading aerial dives. Dynamic drum fills.",
     ["onset", "osprey", "drum", "fill", "pitch-dive"],
     [pair("Onset","Osprey","Rhythm->Blend",0.6), pair("Osprey","Onset","Pitch->Velocity",0.5)],
     {"movement":0.82,"brightness":0.65,"aggression":0.6},
     {"Onset":{"perc_v4_level":0.75,"perc_v5_level":0.7,"perc_macro_punch":0.65},
      "Osprey":{"osp_pitchDive":0.65,"osp_diveTail":0.5,"osp_velocity":0.6}}),

    ("Osteria", "Communal Drum Circle",
     "ONSET provides the foundational beat, OSTERIA adds the communal "
     "handpan and frame drum layers. Shared groove space, collective percussion.",
     ["onset", "osteria", "drum", "communal", "handpan"],
     [pair("Onset","Osteria","Rhythm->Blend",0.55), pair("Osteria","Onset","Amp->Filter",0.4)],
     {"warmth":0.58,"movement":0.75,"space":0.52},
     {"Onset":{"perc_v1_level":0.8,"perc_v2_level":0.65,"perc_macro_punch":0.6},
      "Osteria":{"ost_communalPulse":0.6,"ost_handpanMix":0.55,"ost_roomMix":0.45}}),

    ("Oceanic", "Underwater Impact",
     "ONSET transients filter through OCEANIC's pressure model. "
     "Drum hits compress and expand as if heard from underwater. Sonar percussion.",
     ["onset", "oceanic", "drum", "underwater", "sonar"],
     [pair("Onset","Oceanic","Amp->Filter",0.65), pair("Oceanic","Onset","Env->Character",0.5)],
     {"space":0.58,"density":0.68,"movement":0.75},
     {"Onset":{"perc_v1_level":0.85,"perc_v3_level":0.6,"perc_macro_punch":0.7},
      "Oceanic":{"oce_depth":0.6,"oce_pressure":0.5,"oce_filterCut":3000.0}}),

    ("Owlfish", "Bass Drum Depth",
     "OWLFISH's deep subharmonic partials triggered at drum frequency grid. "
     "Every kick spawns a tuned subharm cloud. Beat becomes bass.",
     ["onset", "owlfish", "drum", "bass", "subharmonic"],
     [pair("Onset","Owlfish","Amp->Filter",0.7), pair("Owlfish","Onset","Pitch->Character",0.5)],
     {"density":0.75,"warmth":0.55,"movement":0.75},
     {"Onset":{"perc_v1_level":0.9,"perc_v1_decay":0.55,"perc_macro_punch":0.75},
      "Owlfish":{"owl_subMix":0.65,"owl_subDiv1":1,"owl_subLevel1":0.75,"owl_mixtur":0.4}}),

    ("Ohm", "Commune Drums",
     "OHM's commune axis expanded by ONSET rhythm. The beat is the ritual. "
     "Drum pattern becomes a communal invocation. MEDDLING macro activates.",
     ["onset", "ohm", "drum", "commune", "ritual"],
     [pair("Onset","Ohm","Rhythm->Blend",0.6), pair("Ohm","Onset","Amp->Filter",0.45)],
     {"warmth":0.55,"movement":0.78,"space":0.48},
     {"Onset":{"perc_v1_level":0.8,"perc_v2_level":0.65,"perc_macro_punch":0.65},
      "Ohm":{"ohm_commune":0.65,"ohm_meddling":0.5,"ohm_ritualDepth":0.55}}),

    ("Orphica", "Grain Scatter Drums",
     "ORPHICA's grain engine scatters microsonic clouds on every ONSET hit. "
     "Percussion becomes temporal texture — a siphonophore colony of sound.",
     ["onset", "orphica", "drum", "granular", "scatter"],
     [pair("Onset","Orphica","Amp->Filter",0.65), pair("Orphica","Onset","Env->Character",0.45)],
     {"movement":0.8,"brightness":0.65,"density":0.72},
     {"Onset":{"perc_v1_level":0.8,"perc_v3_level":0.6,"perc_macro_punch":0.7},
      "Orphica":{"orph_grainSize":0.25,"orph_scatter":0.65,"orph_harpDensity":0.55}}),

    ("Obbligato", "Wind Accent Drums",
     "OBBLIGATO wind voices triggered as drum accents. On-beat = Pipe A, "
     "off-beat = Pipe B. BOND macro couples the accent spacing.",
     ["onset", "obbligato", "drum", "wind", "accent"],
     [pair("Onset","Obbligato","Rhythm->Blend",0.6), pair("Obbligato","Onset","Pitch->Character",0.4)],
     {"warmth":0.55,"movement":0.75,"density":0.68},
     {"Onset":{"perc_v1_level":0.8,"perc_v2_level":0.7,"perc_macro_punch":0.65},
      "Obbligato":{"obbl_bond":0.6,"obbl_breathRate":0.55,"obbl_windMix":0.6}}),

    ("Ottoni", "Brass Hit Architecture",
     "OTTONI triple-brass stabs orchestrated by ONSET drum grid. "
     "Bar 1 = kick brass hit, bar 2 = snare brass response. GROW macro climbs.",
     ["onset", "ottoni", "drum", "brass", "orchestral"],
     [pair("Onset","Ottoni","Amp->Filter",0.7), pair("Ottoni","Onset","Env->Character",0.45)],
     {"brightness":0.7,"aggression":0.62,"movement":0.75},
     {"Onset":{"perc_v1_level":0.85,"perc_v2_level":0.75,"perc_macro_punch":0.75},
      "Ottoni":{"otto_grow":0.65,"otto_brassAttack":0.15,"otto_ensemble":0.6}}),

    ("Ole", "Clave Machine",
     "ONSET drum hits follow CLAVE pattern, OLE responds with Afro-Latin "
     "counter-rhythms. DRAMA macro scales with syncopation depth.",
     ["onset", "ole", "drum", "clave", "afro-latin"],
     [pair("Onset","Ole","Rhythm->Blend",0.7), pair("Ole","Onset","Amp->Filter",0.55)],
     {"warmth":0.55,"movement":0.82,"aggression":0.6},
     {"Onset":{"perc_v1_level":0.8,"perc_v3_level":0.7,"perc_macro_punch":0.7},
      "Ole":{"ole_drama":0.65,"ole_clave":0.65,"ole_percMix":0.7}}),

    ("Ombre", "Decay Gradient Hit",
     "Each ONSET hit triggers an OMBRE gradient decay. The drum "
     "silhouette grows darker as the tail fades. Cinematic rhythm.",
     ["onset", "ombre", "drum", "decay", "cinematic"],
     [pair("Onset","Ombre","Amp->Filter",0.6), pair("Ombre","Onset","Env->Character",0.4)],
     {"brightness":0.55,"space":0.55,"movement":0.72},
     {"Onset":{"perc_v1_level":0.8,"perc_v1_decay":0.6,"perc_macro_space":0.5},
      "Ombre":{"ombr_gradient":0.6,"ombr_shadow":0.5,"ombr_filterCut":4500.0}}),

    ("Orca", "Breach Drum Surge",
     "ORCA breach events synchronized to ONSET's climax trigger. "
     "The biggest hit in the bar detonates the full breach. Peak drama.",
     ["onset", "orca", "drum", "surge", "impact"],
     [pair("Onset","Orca","Amp->Filter",0.75), pair("Orca","Onset","Pitch->Velocity",0.5)],
     {"aggression":0.7,"movement":0.78,"brightness":0.65},
     {"Onset":{"perc_v1_level":0.9,"perc_macro_punch":0.85,"perc_macro_machine":0.65},
      "Orca":{"orca_breach":0.75,"orca_impact":0.8,"orca_tail":0.35}}),

    ("Octopus", "Eight-Arm Pattern",
     "ONSET pattern distributed across OCTOPUS's 8 arms. Kick = arm 1, "
     "snare = arm 2, hats = arms 3–5. Intelligence distributed.",
     ["onset", "octopus", "drum", "distributed", "arms"],
     [pair("Onset","Octopus","Rhythm->Blend",0.7), pair("Octopus","Onset","Env->Character",0.5)],
     {"density":0.78,"movement":0.78,"aggression":0.62},
     {"Onset":{"perc_v1_level":0.85,"perc_v2_level":0.75,"perc_v3_level":0.6,"perc_macro_punch":0.7},
      "Octopus":{"oct_armBalance":0.55,"oct_intelRate":0.6,"oct_inkDensity":0.45}}),

    ("Overlap", "FDN Drum Room",
     "ONSET drum transients feed OVERLAP's Lion's Mane FDN room. "
     "Every hit blooms into a knot-topology reverb space. Architecture of decay.",
     ["onset", "overlap", "drum", "reverb", "fdn"],
     [pair("Onset","Overlap","Amp->Filter",0.65), pair("Overlap","Onset","Pitch->Character",0.4)],
     {"space":0.65,"movement":0.75,"density":0.68},
     {"Onset":{"perc_v1_level":0.85,"perc_v2_level":0.7,"perc_macro_space":0.55},
      "Overlap":{"olap_diffusion":0.65,"olap_knotTopo":0.6,"olap_decay":0.7}}),

    ("Outwit", "Wolfram Drum Evolution",
     "ONSET seeds OUTWIT's Wolfram CA rules. The drum pattern becomes "
     "the initial condition — the automaton evolves it bar by bar.",
     ["onset", "outwit", "drum", "wolfram", "cellular-automata"],
     [pair("Onset","Outwit","Rhythm->Blend",0.7), pair("Outwit","Onset","Env->Character",0.55)],
     {"density":0.75,"movement":0.8,"aggression":0.62},
     {"Onset":{"perc_v1_level":0.8,"perc_v3_level":0.65,"perc_macro_machine":0.65},
      "Outwit":{"owit_wolfRule":110,"owit_armDensity":0.6,"owit_caRate":0.65}}),

    ("Oracle", "Prophetic Downbeat",
     "ORACLE's prophecy system reads ONSET's grid and predicts the next "
     "accent. Drums that know where they're going before they get there.",
     ["onset", "oracle", "drum", "prophetic", "evolving"],
     [pair("Onset","Oracle","Rhythm->Blend",0.6), pair("Oracle","Onset","Env->Character",0.5)],
     {"movement":0.78,"space":0.52,"density":0.68},
     {"Onset":{"perc_v1_level":0.8,"perc_v2_level":0.7,"perc_macro_punch":0.65},
      "Oracle":{"orc_prophecy":0.6,"orc_morphRate":0.5,"orc_resonance":0.55}}),

    ("Organon", "Logical Percussion Engine",
     "ORGANON's rule-based intelligence structures ONSET output. "
     "Every drum hit has a grammatical role — subject, predicate, object.",
     ["onset", "organon", "drum", "logical", "structured"],
     [pair("Onset","Organon","Amp->Filter",0.6), pair("Organon","Onset","Pitch->Character",0.5)],
     {"density":0.72,"movement":0.75,"brightness":0.6},
     {"Onset":{"perc_v1_level":0.8,"perc_v2_level":0.7,"perc_macro_punch":0.65},
      "Organon":{"org_logic":0.6,"org_density":0.65,"org_filterCut":5500.0}}),

    ("Ouroboros", "Serpent Beat Loop",
     "OUROBOROS feeds drum energy back into ONSET's envelope floor. "
     "The beat sustains itself, growing with each cycle. Eternal groove.",
     ["onset", "ouroboros", "drum", "loop", "serpent", "feedback"],
     [pair("Onset","Ouroboros","Rhythm->Blend",0.65), pair("Ouroboros","Onset","Amp->Filter",0.7)],
     {"density":0.75,"movement":0.78,"aggression":0.65},
     {"Onset":{"perc_v1_level":0.85,"perc_v2_level":0.75,"perc_macro_punch":0.7},
      "Ouroboros":{"ouro_feedback":0.65,"ouro_serpentine":0.6,"ouro_decay":0.45}}),
]

# ---------------------------------------------------------------------------
# 4. OVERWORLD × PARTNER  (20 presets)
# ---------------------------------------------------------------------------

OVERWORLD_PAIRS = [
    ("Optic", "Pixel Lens Flare",
     "OVERWORLD's chip melody events trigger OPTIC's lens phenomena. "
     "High notes create bloom, low notes create vignette. Visual chiptune.",
     ["overworld", "optic", "chip", "visual", "lens"],
     [pair("Overworld","Optic","Amp->Filter",0.65), pair("Optic","Overworld","Env->Character",0.45)],
     {"brightness":0.78,"movement":0.72,"aggression":0.48},
     {"Overworld":{"ow_eraCross":0.5,"ow_oscWave":1,"ow_filterCut":7000.0,"ow_arpRate":100.0},
      "Optic":{"opt_lensFlare":0.6,"opt_aberration":0.4,"opt_filterCut":8000.0}}),

    ("Oblique", "Chip Bounce Geometry",
     "OVERWORLD chip arpeggios feed into OBLIQUE's bounce engine. "
     "Each note in the sequence becomes a different bounce angle. Geometry in pixels.",
     ["overworld", "oblique", "chip", "bounce", "arp"],
     [pair("Overworld","Oblique","Pitch->Character",0.65), pair("Oblique","Overworld","Amp->Filter",0.45)],
     {"brightness":0.76,"movement":0.78,"space":0.5},
     {"Overworld":{"ow_arpRate":90.0,"ow_eraCross":0.4,"ow_oscWave":1},
      "Oblique":{"oblq_bounceGravity":0.5,"oblq_bounceCnt":10.0,"oblq_prismMix":0.5}}),

    ("Ocelot", "Glitch Gate Hunt",
     "OVERWORLD's glitch events trigger OCELOT's predator gate mechanism. "
     "Random pixel corruption becomes precision gate hunting.",
     ["overworld", "ocelot", "chip", "glitch", "gate"],
     [pair("Overworld","Ocelot","Env->Character",0.7), pair("Ocelot","Overworld","Amp->Filter",0.5)],
     {"brightness":0.72,"aggression":0.65,"movement":0.72},
     {"Overworld":{"ow_glitch":0.65,"ow_eraCross":0.55,"ow_filterCut":8000.0},
      "Ocelot":{"oct_gate":0.65,"oct_attack":0.002,"oct_ratio":5.0,"oct_release":0.15}}),

    ("Osprey", "Era Sweep Dive",
     "OVERWORLD's ERA crossfade modulates OSPREY's dive pitch curve. "
     "Every console-generation shift launches a new aerial maneuver.",
     ["overworld", "osprey", "chip", "era", "pitch-dive"],
     [pair("Overworld","Osprey","Env->Morph",0.65), pair("Osprey","Overworld","Pitch->Character",0.45)],
     {"brightness":0.75,"movement":0.75,"space":0.48},
     {"Overworld":{"ow_eraCross":0.6,"ow_glitch":0.25,"ow_arpRate":80.0},
      "Osprey":{"osp_pitchDive":0.55,"osp_diveTail":0.45,"osp_wingSpan":0.4}}),

    ("Osteria", "Tavern Chip Music",
     "OVERWORLD delivers the chiptune melody, OSTERIA adds the communal "
     "warmth of a physical venue. Imagine a retro arcade inside a warm taverna.",
     ["overworld", "osteria", "chip", "communal", "warm"],
     [pair("Overworld","Osteria","Pitch->Character",0.5), pair("Osteria","Overworld","Amp->Filter",0.45)],
     {"warmth":0.55,"brightness":0.72,"space":0.55},
     {"Overworld":{"ow_eraCross":0.3,"ow_oscWave":0,"ow_arpRate":75.0,"ow_reverbMix":0.3},
      "Osteria":{"ost_communalPulse":0.5,"ost_warmth":0.6,"ost_roomMix":0.5}}),

    ("Oceanic", "Deep Pixel Pressure",
     "OVERWORLD's bright chip tones filtered through OCEANIC's pressure model. "
     "The further into the water column, the more the pixels dissolve.",
     ["overworld", "oceanic", "chip", "depth", "underwater"],
     [pair("Overworld","Oceanic","Amp->Filter",0.6), pair("Oceanic","Overworld","Env->Character",0.5)],
     {"brightness":0.65,"space":0.6,"density":0.58},
     {"Overworld":{"ow_eraCross":0.45,"ow_filterCut":6000.0,"ow_reverbMix":0.35},
      "Oceanic":{"oce_depth":0.55,"oce_pressure":0.45,"oce_filterCut":4000.0}}),

    ("Owlfish", "Orbital Chip Bass",
     "OVERWORLD's chip melody floats over OWLFISH's orbital subharmonics. "
     "NES squares above, bioluminescent subs below. Full frequency architecture.",
     ["overworld", "owlfish", "chip", "bass", "subharmonic"],
     [pair("Overworld","Owlfish","Pitch->Character",0.6), pair("Owlfish","Overworld","Amp->Filter",0.5)],
     {"density":0.65,"brightness":0.7,"space":0.52},
     {"Overworld":{"ow_eraCross":0.4,"ow_oscWave":1,"ow_arpRate":85.0},
      "Owlfish":{"owl_subMix":0.6,"owl_subDiv1":2,"owl_subLevel1":0.7,"owl_mixtur":0.4}}),

    ("Ohm", "Commune Chip Jam",
     "OVERWORLD's game-music arpeggio feeds OHM's communal jam structure. "
     "The chip synth becomes the communal instrument everyone plays together.",
     ["overworld", "ohm", "chip", "commune", "jam"],
     [pair("Overworld","Ohm","Pitch->Character",0.55), pair("Ohm","Overworld","Rhythm->Blend",0.45)],
     {"warmth":0.58,"brightness":0.72,"movement":0.7},
     {"Overworld":{"ow_eraCross":0.35,"ow_arpRate":80.0,"ow_glitch":0.15},
      "Ohm":{"ohm_commune":0.6,"ohm_meddling":0.4,"ohm_warmth":0.55}}),

    ("Orphica", "Grain Pixel Clouds",
     "ORPHICA's microsound engine granularizes OVERWORLD's chip tone output. "
     "Retro pixels dissolve into impressionistic sound-grain clusters.",
     ["overworld", "orphica", "chip", "granular", "texture"],
     [pair("Overworld","Orphica","Amp->Filter",0.6), pair("Orphica","Overworld","Env->Character",0.5)],
     {"brightness":0.72,"movement":0.72,"space":0.55},
     {"Overworld":{"ow_eraCross":0.5,"ow_oscWave":1,"ow_filterCut":7000.0},
      "Orphica":{"orph_grainSize":0.2,"orph_scatter":0.6,"orph_harpDensity":0.5}}),

    ("Obbligato", "Chip Wind Duo",
     "OVERWORLD provides the chip melody line, OBBLIGATO adds obligatory "
     "wind accompaniment. Two voices that cannot exist without each other.",
     ["overworld", "obbligato", "chip", "wind", "duo"],
     [pair("Overworld","Obbligato","Pitch->Character",0.6), pair("Obbligato","Overworld","Rhythm->Blend",0.45)],
     {"warmth":0.52,"brightness":0.72,"movement":0.72},
     {"Overworld":{"ow_eraCross":0.4,"ow_oscWave":0,"ow_arpRate":90.0},
      "Obbligato":{"obbl_bond":0.65,"obbl_breathRate":0.55,"obbl_windMix":0.6}}),

    ("Ottoni", "Brass Chip Fanfare",
     "OVERWORLD's 8-bit melody triggers OTTONI brass chord stabs. "
     "The video-game hero theme gets a full brass orchestra response.",
     ["overworld", "ottoni", "chip", "brass", "fanfare"],
     [pair("Overworld","Ottoni","Pitch->Character",0.65), pair("Ottoni","Overworld","Amp->Filter",0.45)],
     {"brightness":0.78,"aggression":0.58,"movement":0.72},
     {"Overworld":{"ow_eraCross":0.45,"ow_oscWave":1,"ow_arpRate":100.0},
      "Ottoni":{"otto_grow":0.6,"otto_brassAttack":0.2,"otto_ensemble":0.65}}),

    ("Ole", "Afro Chip Carnival",
     "OVERWORLD's chip game-music energy feeds OLE's Afro-Latin trio. "
     "Pixel carnival — 8-bit meets clave, congas, and drama.",
     ["overworld", "ole", "chip", "afro-latin", "carnival"],
     [pair("Overworld","Ole","Rhythm->Blend",0.65), pair("Ole","Overworld","Amp->Filter",0.5)],
     {"brightness":0.75,"movement":0.8,"warmth":0.5},
     {"Overworld":{"ow_eraCross":0.5,"ow_arpRate":110.0,"ow_glitch":0.2},
      "Ole":{"ole_drama":0.65,"ole_clave":0.6,"ole_percMix":0.65}}),

    ("Ombre", "ERA Gradient Fade",
     "OVERWORLD's ERA crossfade modulates OMBRE's gradient shadow. "
     "NES bright → SNES warm → Genesis dark — each era is a deeper shadow.",
     ["overworld", "ombre", "chip", "era", "gradient"],
     [pair("Overworld","Ombre","Env->Morph",0.65), pair("Ombre","Overworld","Amp->Filter",0.45)],
     {"brightness":0.62,"space":0.55,"movement":0.68},
     {"Overworld":{"ow_eraCross":0.5,"ow_filterCut":5500.0,"ow_reverbMix":0.35},
      "Ombre":{"ombr_gradient":0.6,"ombr_shadow":0.45,"ombr_filterCut":5000.0}}),

    ("Orca", "Chip Breach Surge",
     "OVERWORLD's climax chip phrase triggers ORCA breach event. "
     "When the video-game music peaks, the whale breaches. Cathartic.",
     ["overworld", "orca", "chip", "surge", "climax"],
     [pair("Overworld","Orca","Amp->Filter",0.7), pair("Orca","Overworld","Env->Character",0.5)],
     {"brightness":0.72,"aggression":0.65,"movement":0.75},
     {"Overworld":{"ow_eraCross":0.65,"ow_glitch":0.5,"ow_filterCut":9000.0},
      "Orca":{"orca_breach":0.7,"orca_impact":0.75,"orca_tail":0.4}}),

    ("Octopus", "8-Arm Pixel Grid",
     "OVERWORLD's 8-bit arpeggio notes distributed across OCTOPUS's 8 arms. "
     "Each arm handles one note of the sequence. Distributed pixel intelligence.",
     ["overworld", "octopus", "chip", "arp", "distributed"],
     [pair("Overworld","Octopus","Rhythm->Blend",0.65), pair("Octopus","Overworld","Env->Character",0.5)],
     {"density":0.7,"movement":0.75,"brightness":0.72},
     {"Overworld":{"ow_arpRate":95.0,"ow_eraCross":0.5,"ow_oscWave":1},
      "Octopus":{"oct_armBalance":0.55,"oct_intelRate":0.65,"oct_inkDensity":0.4}}),

    ("Overlap", "Chip Reverb Architecture",
     "OVERWORLD's bright chip tones bloom in OVERLAP's knot-topology FDN. "
     "Simple 8-bit notes expand into architectural reverb cathedrals.",
     ["overworld", "overlap", "chip", "reverb", "topology"],
     [pair("Overworld","Overlap","Amp->Filter",0.65), pair("Overlap","Overworld","Pitch->Character",0.45)],
     {"space":0.68,"brightness":0.72,"movement":0.68},
     {"Overworld":{"ow_eraCross":0.4,"ow_oscWave":0,"ow_reverbMix":0.2},
      "Overlap":{"olap_diffusion":0.65,"olap_knotTopo":0.6,"olap_decay":0.72}}),

    ("Outwit", "Wolfram Chip Evolution",
     "OVERWORLD seeds OUTWIT's Wolfram automaton with ERA position. "
     "Console generation drives rule-set evolution. Chip history as CA state.",
     ["overworld", "outwit", "chip", "wolfram", "evolving"],
     [pair("Overworld","Outwit","Env->Morph",0.65), pair("Outwit","Overworld","Rhythm->Blend",0.5)],
     {"density":0.65,"movement":0.75,"brightness":0.72},
     {"Overworld":{"ow_eraCross":0.6,"ow_glitch":0.35,"ow_arpRate":85.0},
      "Outwit":{"owit_wolfRule":90,"owit_armDensity":0.55,"owit_caRate":0.6}}),

    ("Oracle", "Console Prophecy",
     "ORACLE reads OVERWORLD's ERA blend and prophesies the next era shift. "
     "The chip synth knows its own console history. Self-aware retro.",
     ["overworld", "oracle", "chip", "era", "prophecy"],
     [pair("Overworld","Oracle","Env->Morph",0.6), pair("Oracle","Overworld","Pitch->Character",0.5)],
     {"brightness":0.7,"movement":0.72,"space":0.52},
     {"Overworld":{"ow_eraCross":0.55,"ow_arpRate":80.0,"ow_glitch":0.2},
      "Oracle":{"orc_prophecy":0.6,"orc_morphRate":0.45,"orc_resonance":0.5}}),

    ("Organon", "Logic Chip Structure",
     "ORGANON's rule system governs OVERWORLD's glitch probability. "
     "Logical structure contains pixel chaos — order policing entropy.",
     ["overworld", "organon", "chip", "logic", "structure"],
     [pair("Overworld","Organon","Amp->Filter",0.6), pair("Organon","Overworld","Env->Character",0.5)],
     {"brightness":0.7,"density":0.65,"movement":0.7},
     {"Overworld":{"ow_eraCross":0.5,"ow_glitch":0.4,"ow_filterCut":7500.0},
      "Organon":{"org_logic":0.6,"org_density":0.6,"org_filterCut":6000.0}}),

    ("Ouroboros", "Pixel Serpent Loop",
     "OUROBOROS loops OVERWORLD's chip phrase in self-consuming patterns. "
     "The 8-bit serpent eats its own arpeggio. Retro eternity.",
     ["overworld", "ouroboros", "chip", "loop", "serpent", "recursive"],
     [pair("Overworld","Ouroboros","Rhythm->Blend",0.65), pair("Ouroboros","Overworld","Amp->Filter",0.65)],
     {"brightness":0.72,"density":0.68,"movement":0.72,"aggression":0.52},
     {"Overworld":{"ow_eraCross":0.5,"ow_arpRate":90.0,"ow_glitch":0.3},
      "Ouroboros":{"ouro_feedback":0.65,"ouro_serpentine":0.6,"ouro_decay":0.5}}),
]


# ---------------------------------------------------------------------------
# MAIN
# ---------------------------------------------------------------------------

def build_from_pair_spec(anchor_engine, anchor_dna, pairs_spec):
    """Convert a pair spec list into full preset dicts."""
    presets = []
    for (partner, name, desc, tags, coupling_pairs, dna_overrides, params) in pairs_spec:
        base = blend_dna(anchor_dna, anchor_dna)  # anchor weighted 2x by doubling
        dna = nudge(base, **dna_overrides)
        p = make_preset(
            name=name,
            engines=[anchor_engine, partner],
            desc=desc,
            tags=tags,
            dna=dna,
            coupling_pairs=coupling_pairs,
            parameters=params,
            intensity="Deep" if any(cp["amount"] >= 0.65 for cp in coupling_pairs) else "Moderate",
        )
        presets.append(p)
    return presets


def main():
    os.makedirs(ENTANGLED_DIR, exist_ok=True)

    written = 0
    skipped = 0

    all_presets = (
        MARQUEE
        + build_from_pair_spec("Oblong",    DNA_OBLONG, OBLONG_PAIRS)
        + build_from_pair_spec("Onset",     DNA_ONSET,  ONSET_PAIRS)
        + build_from_pair_spec("Overworld", DNA_OW,     OVERWORLD_PAIRS)
    )

    print(f"\nGenerating {len(all_presets)} presets → {ENTANGLED_DIR}\n")

    for preset in all_presets:
        result = write_preset(preset)
        if result:
            written += 1
        else:
            skipped += 1

    print(f"\nDone. {written} written, {skipped} skipped (already existed).")
    print(f"Total presets attempted: {written + skipped}")


if __name__ == "__main__":
    main()
