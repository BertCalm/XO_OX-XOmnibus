#!/usr/bin/env python3
"""
generate_opcode_presets.py
--------------------------
Generates 143 factory presets for the Opcode FM engine.
All presets are hand-designed with musically intentional parameter values.

Algorithm modes:
  0 = Series  (Classic DX EP — modulator feeds carrier)
  1 = Parallel (Organ-like — both oscillators sum)
  2 = Feedback (Metallic/harsh — self-modulating feedback loop)

Output: Presets/XOceanus/{mood}/Opcode/{name}.xometa
"""

import json
import os

REPO_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
PRESET_BASE = os.path.join(REPO_ROOT, "Presets", "XOceanus")


def p(
    algorithm,
    ratio,
    index,
    feedback,
    vel_to_index,
    brightness,
    attack,
    decay,
    sustain,
    release,
    mod_attack,
    mod_decay,
    mod_sustain,
    filter_env_amt,
    filter_sustain,
    migration,
    bend_range,
    macro_character,
    macro_movement,
    macro_coupling,
    macro_space,
    lfo1_rate,
    lfo1_depth,
    lfo1_shape,
    lfo2_rate,
    lfo2_depth,
    lfo2_shape,
):
    """Return a fully-specified Opcode parameter dict (all 27 params)."""
    return {
        "opco_algorithm": algorithm,
        "opco_ratio": ratio,
        "opco_index": index,
        "opco_feedback": feedback,
        "opco_velToIndex": vel_to_index,
        "opco_brightness": brightness,
        "opco_attack": attack,
        "opco_decay": decay,
        "opco_sustain": sustain,
        "opco_release": release,
        "opco_modAttack": mod_attack,
        "opco_modDecay": mod_decay,
        "opco_modSustain": mod_sustain,
        "opco_filterEnvAmt": filter_env_amt,
        "opco_filterSustain": filter_sustain,
        "opco_migration": migration,
        "opco_bendRange": bend_range,
        "opco_macroCharacter": macro_character,
        "opco_macroMovement": macro_movement,
        "opco_macroCoupling": macro_coupling,
        "opco_macroSpace": macro_space,
        "opco_lfo1Rate": lfo1_rate,
        "opco_lfo1Depth": lfo1_depth,
        "opco_lfo1Shape": lfo1_shape,
        "opco_lfo2Rate": lfo2_rate,
        "opco_lfo2Depth": lfo2_depth,
        "opco_lfo2Shape": lfo2_shape,
    }


def dna(brightness, warmth, movement, density, space, aggression):
    return {
        "brightness": brightness,
        "warmth": warmth,
        "movement": movement,
        "density": density,
        "space": space,
        "aggression": aggression,
    }


def make_preset(
    name,
    mood,
    description,
    tags,
    params,
    sonic_dna,
    coupling_intensity="None",
    coupling_pairs=None,
):
    return {
        "schema_version": 1,
        "name": name,
        "mood": mood,
        "engines": ["Opcode"],
        "author": "XO_OX Designs",
        "version": "1.0.0",
        "description": description,
        "tags": tags,
        "macroLabels": ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"],
        "couplingIntensity": coupling_intensity,
        "tempo": None,
        "dna": sonic_dna,
        "parameters": {"Opcode": params},
        "coupling": {"pairs": coupling_pairs or []},
        "sequencer": None,
    }


# =============================================================================
# PRESET DEFINITIONS
# =============================================================================

PRESETS = []

# ---------------------------------------------------------------------------
# FOUNDATION (25) — Classic DX EP tones
# ---------------------------------------------------------------------------

PRESETS += [
    make_preset(
        "Suitcase Morning",
        "Foundation",
        "Warm FM electric piano with a soft attack bloom, evoking the velvet low-end of a Rhodes Suitcase through FM glass.",
        ["fm", "ep", "warm", "dx7", "foundation"],
        p(0, 1.4, 0.65, 0.0, 0.55, 8000.0, 0.004, 1.4, 0.42, 0.55, 0.001, 0.3, 0.12, 0.28, 0.3, 0.0, 2, 0.55, 0.1, 0.0, 0.45, 0.5, 0.03, 0, 1.2, 0.04, 0),
        dna(0.55, 0.55, 0.12, 0.4, 0.45, 0.12),
    ),
    make_preset(
        "FM Rhodes Classic",
        "Foundation",
        "Direct DX7 EP reference — algorithm 0 with ratio 2, mod decay fast enough to kill the overtones on sustain. Exactly that sound.",
        ["fm", "dx7", "ep", "classic", "foundation"],
        p(0, 2.0, 0.72, 0.0, 0.6, 10000.0, 0.005, 1.2, 0.45, 0.6, 0.001, 0.35, 0.15, 0.3, 0.25, 0.0, 2, 0.5, 0.0, 0.0, 0.4, 0.5, 0.0, 0, 1.0, 0.0, 0),
        dna(0.6, 0.4, 0.1, 0.4, 0.45, 0.1),
    ),
    make_preset(
        "Digital Ivory",
        "Foundation",
        "Pure series FM with near-unity ratio — the bright, attack-forward digital piano voice that dominated synth workstations of the late 80s.",
        ["fm", "piano", "digital", "bright", "foundation"],
        p(0, 1.0, 0.8, 0.0, 0.65, 12000.0, 0.003, 1.5, 0.35, 0.65, 0.001, 0.22, 0.08, 0.22, 0.2, 0.0, 2, 0.5, 0.25, 0.0, 0.5, 0.4, 0.04, 0, 1.5, 0.05, 0),
        dna(0.72, 0.25, 0.12, 0.42, 0.5, 0.3),
    ),
    make_preset(
        "Honey Treble",
        "Foundation",
        "Algorithm 0 with a 3:1 ratio softened by a warm brightness filter — the treble that melts, golden and unhurried.",
        ["fm", "ep", "warm", "bright", "foundation"],
        p(0, 3.0, 0.55, 0.0, 0.5, 9500.0, 0.006, 1.1, 0.5, 0.65, 0.001, 0.28, 0.2, 0.25, 0.28, 0.0, 2, 0.6, 0.05, 0.0, 0.5, 0.6, 0.02, 0, 1.8, 0.03, 0),
        dna(0.58, 0.5, 0.1, 0.38, 0.5, 0.1),
    ),
    make_preset(
        "Stage EP Clean",
        "Foundation",
        "Stage-ready FM electric piano, slightly compressed response, velocity-sensitive index opens warmth on harder hits.",
        ["fm", "ep", "stage", "clean", "foundation"],
        p(0, 1.8, 0.6, 0.0, 0.7, 9000.0, 0.004, 1.0, 0.5, 0.5, 0.001, 0.3, 0.18, 0.2, 0.22, 0.0, 2, 0.45, 0.0, 0.0, 0.35, 0.5, 0.0, 0, 1.0, 0.0, 0),
        dna(0.56, 0.45, 0.08, 0.42, 0.38, 0.12),
    ),
    make_preset(
        "Glass Tines",
        "Foundation",
        "The glassy high-end of FM tines — ratio 2.8, mid brightness, short mod decay. Each note has a crystalline sting at the attack.",
        ["fm", "ep", "glass", "tines", "foundation"],
        p(0, 2.8, 0.62, 0.0, 0.58, 11000.0, 0.003, 1.3, 0.38, 0.55, 0.001, 0.2, 0.1, 0.3, 0.2, 0.0, 2, 0.55, 0.15, 0.0, 0.45, 0.5, 0.03, 0, 2.0, 0.03, 0),
        dna(0.65, 0.32, 0.1, 0.4, 0.42, 0.15),
    ),
    make_preset(
        "Seventies Studio",
        "Foundation",
        "Soft, woody FM piano evoking the 70s session studio — low index, long decay, the warmth of magnetic tape.",
        ["fm", "ep", "vintage", "studio", "foundation"],
        p(0, 1.2, 0.45, 0.0, 0.4, 7000.0, 0.006, 1.8, 0.55, 0.8, 0.001, 0.45, 0.22, 0.18, 0.25, 0.0, 2, 0.45, 0.0, 0.0, 0.55, 0.4, 0.02, 0, 0.8, 0.02, 0),
        dna(0.42, 0.62, 0.1, 0.35, 0.58, 0.08),
    ),
    make_preset(
        "Neon Clav",
        "Foundation",
        "FM clavinet-style — high ratio 4:1, short decay, punchy velocity response. Funky series algorithm with attitude.",
        ["fm", "clav", "funky", "punchy", "foundation"],
        p(0, 4.0, 0.75, 0.0, 0.8, 13000.0, 0.002, 0.6, 0.2, 0.25, 0.001, 0.15, 0.05, 0.35, 0.15, 0.0, 2, 0.7, 0.2, 0.0, 0.3, 0.5, 0.0, 0, 1.0, 0.0, 0),
        dna(0.78, 0.22, 0.15, 0.5, 0.25, 0.42),
    ),
    make_preset(
        "Pop Piano 1989",
        "Foundation",
        "The mid-range pop FM piano of the late 80s — ratio 1.5, moderate index, a touch of release. Every chart hit used this.",
        ["fm", "piano", "pop", "80s", "foundation"],
        p(0, 1.5, 0.58, 0.0, 0.55, 10500.0, 0.004, 1.1, 0.45, 0.55, 0.001, 0.28, 0.15, 0.22, 0.22, 0.0, 2, 0.5, 0.0, 0.0, 0.45, 0.5, 0.0, 0, 1.2, 0.0, 0),
        dna(0.62, 0.38, 0.1, 0.4, 0.42, 0.18),
    ),
    make_preset(
        "Felt Hammer",
        "Foundation",
        "Low-index FM with gentle attack — mimics a felt-dampened digital piano. Warm, restrained, deeply reliable.",
        ["fm", "piano", "felt", "soft", "foundation"],
        p(0, 1.0, 0.4, 0.0, 0.45, 7500.0, 0.007, 2.0, 0.5, 0.75, 0.001, 0.4, 0.25, 0.15, 0.28, 0.0, 2, 0.4, 0.0, 0.0, 0.55, 0.3, 0.02, 0, 0.8, 0.02, 0),
        dna(0.38, 0.68, 0.1, 0.32, 0.6, 0.06),
    ),
    make_preset(
        "Carrier Wave",
        "Foundation",
        "Textbook series FM — modulator perfectly tuned to carrier, ratio 2:1, index sweeps harmonics with velocity. The ur-FM preset.",
        ["fm", "textbook", "classic", "dx7", "foundation"],
        p(0, 2.0, 0.5, 0.0, 0.6, 9000.0, 0.004, 1.3, 0.4, 0.5, 0.001, 0.3, 0.1, 0.2, 0.2, 0.0, 2, 0.5, 0.1, 0.0, 0.4, 0.5, 0.02, 0, 1.0, 0.02, 0),
        dna(0.55, 0.45, 0.1, 0.4, 0.4, 0.12),
    ),
    make_preset(
        "Broadcast EP",
        "Foundation",
        "Clean, compressed FM piano suited for broadcast — balanced frequency response, moderate velocity, sits in any mix.",
        ["fm", "ep", "broadcast", "clean", "foundation"],
        p(0, 2.5, 0.55, 0.0, 0.5, 8500.0, 0.004, 1.2, 0.48, 0.5, 0.001, 0.32, 0.18, 0.2, 0.22, 0.0, 2, 0.48, 0.05, 0.0, 0.38, 0.5, 0.02, 0, 1.0, 0.02, 0),
        dna(0.57, 0.43, 0.1, 0.4, 0.38, 0.1),
    ),
    make_preset(
        "Low Register",
        "Foundation",
        "FM piano optimised for the lower octaves — ratio 0.8, warm brightness ceiling, full sustain. Bass notes bloom.",
        ["fm", "bass-register", "piano", "warm", "foundation"],
        p(0, 0.8, 0.5, 0.0, 0.5, 6500.0, 0.006, 2.2, 0.55, 0.9, 0.001, 0.5, 0.25, 0.2, 0.3, 0.0, 2, 0.4, 0.0, 0.0, 0.6, 0.3, 0.02, 0, 0.6, 0.02, 0),
        dna(0.3, 0.72, 0.1, 0.38, 0.65, 0.06),
    ),
    make_preset(
        "Post-Punk Keys",
        "Foundation",
        "Punchy, slightly overdriven FM piano character from post-punk productions — tight decay, aggressive velocity.",
        ["fm", "ep", "punk", "aggressive", "foundation"],
        p(0, 2.0, 0.82, 0.05, 0.75, 11000.0, 0.003, 0.7, 0.3, 0.3, 0.001, 0.2, 0.08, 0.4, 0.18, 0.0, 2, 0.65, 0.15, 0.0, 0.28, 0.5, 0.02, 0, 1.0, 0.0, 0),
        dna(0.68, 0.28, 0.12, 0.5, 0.28, 0.48),
    ),
    make_preset(
        "Poly Comp",
        "Foundation",
        "Algorithm 1 parallel FM — both operators summing creates an organ-piano hybrid with natural chord bloom.",
        ["fm", "parallel", "organ", "piano", "foundation"],
        p(1, 1.0, 0.4, 0.0, 0.4, 8000.0, 0.005, 1.5, 0.55, 0.65, 0.001, 0.35, 0.25, 0.18, 0.28, 0.0, 2, 0.5, 0.05, 0.0, 0.5, 0.4, 0.02, 0, 1.0, 0.02, 0),
        dna(0.48, 0.52, 0.1, 0.45, 0.5, 0.08),
    ),
    make_preset(
        "Reed Tone",
        "Foundation",
        "Reed organ-via-FM — ratio 1.5, parallel algorithm, slightly longer attack. The breath of a 2-foot pipe.",
        ["fm", "reed", "organ", "soft", "foundation"],
        p(1, 1.5, 0.35, 0.0, 0.35, 7000.0, 0.012, 1.8, 0.6, 0.7, 0.002, 0.4, 0.3, 0.15, 0.3, 0.0, 2, 0.42, 0.05, 0.0, 0.52, 0.3, 0.02, 0, 0.8, 0.02, 0),
        dna(0.38, 0.65, 0.1, 0.45, 0.55, 0.06),
    ),
    make_preset(
        "DX Vibraphone",
        "Foundation",
        "Classic FM vibraphone patch — fast attack, long ring decay, bright harmonics. Soft bowing behind the notes.",
        ["fm", "vibraphone", "mallet", "bright", "foundation"],
        p(0, 3.5, 0.6, 0.0, 0.45, 13500.0, 0.002, 3.5, 0.1, 1.2, 0.001, 0.15, 0.05, 0.28, 0.12, 0.0, 2, 0.55, 0.2, 0.0, 0.55, 0.8, 0.04, 0, 1.2, 0.0, 0),
        dna(0.72, 0.28, 0.18, 0.38, 0.62, 0.12),
    ),
    make_preset(
        "Marimba Voice",
        "Foundation",
        "Woody FM marimba — fast transient, short decay, low index leaves fundamentals clean. Tropical and resonant.",
        ["fm", "marimba", "mallet", "woody", "foundation"],
        p(0, 2.0, 0.42, 0.0, 0.5, 8000.0, 0.001, 0.8, 0.08, 0.35, 0.001, 0.12, 0.03, 0.2, 0.1, 0.0, 2, 0.45, 0.1, 0.0, 0.35, 0.5, 0.0, 0, 0.8, 0.0, 0),
        dna(0.52, 0.48, 0.08, 0.35, 0.35, 0.1),
    ),
    make_preset(
        "Synth Brass DX",
        "Foundation",
        "DX7-style brass voicing — parallel algorithm 1, wide ratio spread, velocity controls brightness. Punchy ensemble brass.",
        ["fm", "brass", "dx7", "ensemble", "foundation"],
        p(1, 2.0, 0.7, 0.0, 0.7, 12000.0, 0.008, 0.8, 0.55, 0.4, 0.002, 0.25, 0.3, 0.4, 0.35, 0.0, 2, 0.7, 0.2, 0.0, 0.35, 0.4, 0.0, 0, 1.5, 0.05, 0),
        dna(0.72, 0.35, 0.12, 0.55, 0.35, 0.45),
    ),
    make_preset(
        "Celesta Shimmer",
        "Foundation",
        "High-ratio FM celesta — ratio 5:1, bright attack, decays cleanly. The tinny shimmer of a music-box mechanism.",
        ["fm", "celesta", "bright", "mallet", "foundation"],
        p(0, 5.0, 0.52, 0.0, 0.4, 15000.0, 0.002, 2.5, 0.05, 0.9, 0.001, 0.1, 0.02, 0.35, 0.1, 0.0, 2, 0.6, 0.15, 0.0, 0.55, 0.6, 0.03, 0, 2.0, 0.0, 0),
        dna(0.82, 0.18, 0.12, 0.32, 0.55, 0.15),
    ),
    make_preset(
        "Warm Pad Layer",
        "Foundation",
        "Series FM as a warm pad foundation — low ratio, low index, slow attack. Designed to sit under a lead, not compete.",
        ["fm", "pad", "warm", "layer", "foundation"],
        p(0, 0.5, 0.3, 0.0, 0.3, 6000.0, 0.06, 3.0, 0.65, 1.5, 0.01, 0.6, 0.4, 0.12, 0.35, 0.0, 2, 0.35, 0.1, 0.0, 0.7, 0.2, 0.05, 0, 0.5, 0.05, 0),
        dna(0.28, 0.72, 0.15, 0.35, 0.72, 0.04),
    ),
    make_preset(
        "Tine Bite",
        "Foundation",
        "The hard attack bite of tines — high index at note-on drops rapidly. You hear the hammer before you hear the note.",
        ["fm", "ep", "tines", "attack", "foundation"],
        p(0, 2.0, 0.9, 0.0, 0.8, 11000.0, 0.003, 0.9, 0.35, 0.45, 0.001, 0.18, 0.06, 0.38, 0.18, 0.0, 2, 0.65, 0.2, 0.0, 0.35, 0.5, 0.02, 0, 1.0, 0.0, 0),
        dna(0.68, 0.32, 0.12, 0.48, 0.35, 0.38),
    ),
    make_preset(
        "Cream Sustain",
        "Foundation",
        "Smooth sustaining FM with slow mod decay — harmonic content stays rich throughout the note. Creamy and unhurried.",
        ["fm", "sustain", "smooth", "warm", "foundation"],
        p(0, 1.5, 0.55, 0.0, 0.45, 8500.0, 0.005, 2.5, 0.6, 1.0, 0.001, 0.8, 0.45, 0.22, 0.38, 0.0, 2, 0.5, 0.05, 0.0, 0.58, 0.3, 0.02, 0, 0.8, 0.02, 0),
        dna(0.5, 0.58, 0.1, 0.42, 0.6, 0.08),
    ),
    make_preset(
        "Envelope Artist",
        "Foundation",
        "Slow attack, long decay, high sustain — a sustained FM texture that lives in the middle of any arrangement.",
        ["fm", "envelope", "lush", "sustain", "foundation"],
        p(0, 1.8, 0.52, 0.0, 0.4, 9000.0, 0.025, 2.8, 0.6, 1.2, 0.005, 0.7, 0.42, 0.18, 0.35, 0.0, 2, 0.45, 0.1, 0.0, 0.65, 0.25, 0.04, 0, 0.6, 0.04, 0),
        dna(0.5, 0.6, 0.12, 0.4, 0.68, 0.06),
    ),
    make_preset(
        "Algorithm Zero",
        "Foundation",
        "Pure demonstration of algorithm 0 — textbook modulator-to-carrier chain, unity ratio, index 1.0. Teaching tool and tone generator.",
        ["fm", "algorithm", "textbook", "pure", "foundation"],
        p(0, 1.0, 1.0, 0.0, 0.5, 10000.0, 0.003, 1.0, 0.5, 0.5, 0.001, 0.3, 0.15, 0.25, 0.25, 0.0, 2, 0.5, 0.0, 0.0, 0.4, 0.5, 0.0, 0, 1.0, 0.0, 0),
        dna(0.6, 0.4, 0.1, 0.45, 0.4, 0.2),
    ),
]

# ---------------------------------------------------------------------------
# CRYSTALLINE (20) — Pure FM bells, glass sine
# ---------------------------------------------------------------------------

PRESETS += [
    make_preset(
        "Glass Harmonica",
        "Crystalline",
        "High ratio, high index FM bell — the ringing overtones of a crystal glass set spinning. Haunting and pure.",
        ["fm", "bell", "glass", "crystalline", "bright"],
        p(0, 7.0, 0.65, 0.0, 0.4, 16000.0, 0.001, 4.0, 0.05, 2.5, 0.001, 0.2, 0.02, 0.45, 0.08, 0.0, 2, 0.6, 0.15, 0.0, 0.7, 0.7, 0.04, 0, 3.0, 0.0, 0),
        dna(0.88, 0.12, 0.12, 0.35, 0.78, 0.12),
    ),
    make_preset(
        "Ice Chime",
        "Crystalline",
        "Ratio 4:1 bell with high brightness — each note shatters like breaking ice. Cold, clear, irresistible.",
        ["fm", "bell", "ice", "bright", "crystalline"],
        p(0, 4.0, 0.72, 0.0, 0.35, 17000.0, 0.001, 3.0, 0.05, 1.8, 0.001, 0.18, 0.02, 0.5, 0.08, 0.0, 2, 0.65, 0.1, 0.0, 0.65, 0.8, 0.05, 0, 3.5, 0.0, 0),
        dna(0.9, 0.08, 0.12, 0.32, 0.72, 0.1),
    ),
    make_preset(
        "Prism Bell",
        "Crystalline",
        "Spectral FM bell with brightness 18kHz — the upper partials ring like light through a prism. Ratio 6, slow decay.",
        ["fm", "bell", "prism", "spectral", "crystalline"],
        p(0, 6.0, 0.6, 0.0, 0.3, 18000.0, 0.001, 4.5, 0.03, 3.0, 0.001, 0.22, 0.01, 0.5, 0.06, 0.0, 2, 0.7, 0.12, 0.0, 0.75, 0.5, 0.04, 0, 2.5, 0.0, 0),
        dna(0.92, 0.06, 0.1, 0.3, 0.82, 0.1),
    ),
    make_preset(
        "Sine Jewel",
        "Crystalline",
        "Minimal FM — near-zero index, pure sine foundation. The jewel at the center of all complex FM sounds.",
        ["fm", "sine", "pure", "minimal", "crystalline"],
        p(0, 1.0, 0.08, 0.0, 0.1, 14000.0, 0.004, 2.0, 0.3, 1.5, 0.001, 0.3, 0.1, 0.1, 0.12, 0.0, 2, 0.3, 0.05, 0.0, 0.6, 0.5, 0.02, 0, 1.0, 0.0, 0),
        dna(0.6, 0.4, 0.08, 0.2, 0.62, 0.04),
    ),
    make_preset(
        "Crystal Tower",
        "Crystalline",
        "Stacked FM partials at high ratio — multiple harmonic layers stack in perfect crystalline alignment. Monumental.",
        ["fm", "bell", "tower", "layered", "crystalline"],
        p(0, 8.0, 0.55, 0.0, 0.3, 17500.0, 0.001, 5.0, 0.03, 4.0, 0.001, 0.25, 0.01, 0.4, 0.06, 0.0, 2, 0.65, 0.1, 0.0, 0.85, 0.6, 0.03, 0, 2.0, 0.0, 0),
        dna(0.9, 0.08, 0.1, 0.28, 0.88, 0.08),
    ),
    make_preset(
        "Water Chime",
        "Crystalline",
        "Ratio 3.5 FM bell with gentle LFO tremolo — like a wind chime reflected in a still pool. Clear and slightly moving.",
        ["fm", "bell", "water", "chime", "crystalline"],
        p(0, 3.5, 0.58, 0.0, 0.32, 15000.0, 0.001, 3.2, 0.05, 2.0, 0.001, 0.2, 0.02, 0.38, 0.08, 0.0, 2, 0.55, 0.15, 0.0, 0.72, 0.6, 0.06, 0, 0.4, 0.05, 0),
        dna(0.82, 0.15, 0.18, 0.32, 0.75, 0.1),
    ),
    make_preset(
        "Icy Resonance",
        "Crystalline",
        "Sub-zero FM — ratio 9:1 with bright filter, index 0.45. The highest overtones only, floating above silence.",
        ["fm", "resonance", "icy", "bright", "crystalline"],
        p(0, 9.0, 0.45, 0.0, 0.25, 18000.0, 0.001, 3.8, 0.04, 2.8, 0.001, 0.15, 0.01, 0.42, 0.05, 0.0, 2, 0.6, 0.1, 0.0, 0.78, 0.5, 0.03, 0, 4.0, 0.0, 0),
        dna(0.92, 0.05, 0.1, 0.28, 0.8, 0.08),
    ),
    make_preset(
        "Bright Sine Pad",
        "Crystalline",
        "Parallel algorithm with two bright partials summing — the purest FM pad, no harmonic clutter, just sine on sine.",
        ["fm", "pad", "sine", "bright", "crystalline"],
        p(1, 1.0, 0.2, 0.0, 0.2, 15000.0, 0.015, 3.0, 0.5, 2.0, 0.001, 0.4, 0.3, 0.12, 0.25, 0.0, 2, 0.45, 0.08, 0.0, 0.75, 0.2, 0.04, 0, 0.3, 0.04, 0),
        dna(0.72, 0.28, 0.12, 0.3, 0.8, 0.05),
    ),
    make_preset(
        "Quartz Pulse",
        "Crystalline",
        "Tight FM bell with very short mod decay — the quartz-like quality of a struck crystal: instant clarity, instant silence.",
        ["fm", "bell", "quartz", "tight", "crystalline"],
        p(0, 5.0, 0.7, 0.0, 0.5, 16500.0, 0.001, 2.2, 0.04, 1.4, 0.001, 0.12, 0.01, 0.48, 0.06, 0.0, 2, 0.68, 0.12, 0.0, 0.55, 0.7, 0.04, 0, 5.0, 0.0, 0),
        dna(0.88, 0.1, 0.12, 0.38, 0.58, 0.12),
    ),
    make_preset(
        "Temple Bell",
        "Crystalline",
        "Slow-decay resonant FM bell — high ratio, velocity shapes brightness, long reverberant tail. The call of a monastery bell.",
        ["fm", "bell", "temple", "resonant", "crystalline"],
        p(0, 6.5, 0.58, 0.0, 0.28, 16000.0, 0.001, 6.0, 0.02, 5.0, 0.001, 0.3, 0.01, 0.45, 0.06, 0.0, 12, 0.55, 0.1, 0.0, 0.9, 0.4, 0.03, 0, 1.5, 0.0, 0),
        dna(0.85, 0.1, 0.08, 0.28, 0.92, 0.06),
    ),
    make_preset(
        "Frost Harp",
        "Crystalline",
        "Algorithm 0 with ratio 4.5 — FM harp simulation with icy brightness. Plucked shimmer, lingering overtones.",
        ["fm", "harp", "frost", "plucked", "crystalline"],
        p(0, 4.5, 0.62, 0.0, 0.4, 17000.0, 0.001, 2.8, 0.06, 2.2, 0.001, 0.16, 0.02, 0.4, 0.08, 0.0, 2, 0.6, 0.12, 0.0, 0.68, 0.6, 0.04, 0, 2.8, 0.0, 0),
        dna(0.87, 0.1, 0.1, 0.32, 0.7, 0.1),
    ),
    make_preset(
        "Diamond Point",
        "Crystalline",
        "Maximum brightness FM — filter at 20kHz, ratio 10:1, short fast chime. The point of a diamond cutting glass.",
        ["fm", "bell", "diamond", "extreme", "crystalline"],
        p(0, 10.0, 0.48, 0.0, 0.3, 20000.0, 0.001, 1.8, 0.03, 1.2, 0.001, 0.1, 0.01, 0.5, 0.05, 0.0, 2, 0.72, 0.08, 0.0, 0.5, 0.8, 0.03, 0, 6.0, 0.0, 0),
        dna(0.95, 0.04, 0.1, 0.3, 0.52, 0.12),
    ),
    make_preset(
        "Glacier Pad",
        "Crystalline",
        "Slow-moving bright FM pad — very slow attack, sustained brightness. Like a glacier: enormous, bright, unhurried.",
        ["fm", "pad", "glacier", "slow", "crystalline"],
        p(0, 2.0, 0.3, 0.0, 0.2, 16000.0, 0.08, 4.0, 0.55, 3.0, 0.01, 0.8, 0.4, 0.2, 0.35, 0.0, 2, 0.5, 0.12, 0.0, 0.88, 0.1, 0.04, 0, 0.2, 0.04, 0),
        dna(0.78, 0.22, 0.12, 0.3, 0.9, 0.05),
    ),
    make_preset(
        "Bone Flute",
        "Crystalline",
        "Pure FM flute — parallel algorithm, near-unity ratio. Breathy upper harmonics via LFO1. Ancient and simple.",
        ["fm", "flute", "pure", "breath", "crystalline"],
        p(1, 1.0, 0.28, 0.0, 0.35, 14000.0, 0.01, 1.5, 0.45, 0.8, 0.001, 0.35, 0.28, 0.18, 0.22, 0.0, 2, 0.4, 0.15, 0.0, 0.55, 0.3, 0.06, 0, 5.0, 0.08, 0),
        dna(0.7, 0.3, 0.22, 0.28, 0.55, 0.06),
    ),
    make_preset(
        "Noon Bells",
        "Crystalline",
        "Mid-register FM bell cluster — ratio 5.5, moderate sustain, velocity-mapped brightness. The church tower at noon.",
        ["fm", "bell", "noon", "church", "crystalline"],
        p(0, 5.5, 0.62, 0.0, 0.42, 15500.0, 0.001, 3.5, 0.05, 2.8, 0.001, 0.2, 0.02, 0.42, 0.08, 0.0, 5, 0.6, 0.12, 0.0, 0.72, 0.6, 0.04, 0, 2.0, 0.0, 0),
        dna(0.85, 0.12, 0.1, 0.32, 0.75, 0.1),
    ),
    make_preset(
        "Spectral Chord",
        "Crystalline",
        "Algorithm 1 parallel — both operators at spread ratios creating natural FM chord. Crystalline harmony generator.",
        ["fm", "parallel", "chord", "spectral", "crystalline"],
        p(1, 2.5, 0.45, 0.0, 0.28, 16000.0, 0.004, 3.2, 0.2, 2.5, 0.001, 0.35, 0.1, 0.3, 0.15, 0.0, 2, 0.55, 0.1, 0.0, 0.75, 0.35, 0.04, 0, 0.8, 0.04, 0),
        dna(0.82, 0.18, 0.12, 0.35, 0.75, 0.08),
    ),
    make_preset(
        "Ring Tone Pure",
        "Crystalline",
        "The ringing partial FM voice — bright, quick attack, slow linear release. No vibrato. No adornment. Just the ring.",
        ["fm", "bell", "ring", "pure", "crystalline"],
        p(0, 3.0, 0.7, 0.0, 0.4, 16500.0, 0.001, 2.8, 0.04, 2.0, 0.001, 0.16, 0.01, 0.45, 0.07, 0.0, 2, 0.62, 0.1, 0.0, 0.68, 0.6, 0.03, 0, 2.5, 0.0, 0),
        dna(0.87, 0.1, 0.1, 0.33, 0.68, 0.1),
    ),
    make_preset(
        "Mineral Tone",
        "Crystalline",
        "Series FM with ratio 7.5 — the overtone series of a struck mineral. Inharmonic partials with long decay.",
        ["fm", "bell", "mineral", "inharmonic", "crystalline"],
        p(0, 7.5, 0.52, 0.0, 0.3, 17000.0, 0.001, 4.2, 0.03, 3.5, 0.001, 0.22, 0.01, 0.42, 0.06, 0.0, 2, 0.62, 0.1, 0.0, 0.82, 0.5, 0.03, 0, 1.8, 0.0, 0),
        dna(0.88, 0.1, 0.08, 0.3, 0.82, 0.08),
    ),
    make_preset(
        "Fractured Light",
        "Crystalline",
        "High-index FM bell with subtle LFO shimmer — the light that fractures through faceted glass. Alive but measured.",
        ["fm", "bell", "shimmer", "bright", "crystalline"],
        p(0, 4.0, 0.8, 0.0, 0.35, 17500.0, 0.001, 3.0, 0.04, 2.5, 0.001, 0.18, 0.01, 0.48, 0.07, 0.0, 2, 0.68, 0.14, 0.0, 0.72, 0.7, 0.06, 0, 0.6, 0.05, 0),
        dna(0.9, 0.08, 0.2, 0.35, 0.72, 0.12),
    ),
    make_preset(
        "Singing Bowl",
        "Crystalline",
        "Tibetan singing bowl via FM — ratio 2.76 (inharmonic), slow sustain, long release, slight LFO shimmer. Meditative.",
        ["fm", "bell", "bowl", "meditative", "crystalline"],
        p(0, 2.76, 0.5, 0.0, 0.25, 13000.0, 0.002, 5.0, 0.08, 4.5, 0.001, 0.28, 0.03, 0.32, 0.1, 0.0, 2, 0.45, 0.12, 0.0, 0.88, 0.15, 0.04, 0, 0.25, 0.05, 0),
        dna(0.65, 0.35, 0.15, 0.28, 0.92, 0.04),
    ),
]

# ---------------------------------------------------------------------------
# KINETIC (20) — Percussive FM, staccato
# ---------------------------------------------------------------------------

PRESETS += [
    make_preset(
        "Punch Clock",
        "Kinetic",
        "Staccato FM hit with near-instant attack and short decay. Algorithm 0, high index, velocity shapes the punch. On the beat.",
        ["fm", "percussive", "staccato", "punchy", "kinetic"],
        p(0, 2.0, 0.85, 0.05, 0.8, 12000.0, 0.001, 0.4, 0.05, 0.15, 0.001, 0.12, 0.03, 0.45, 0.1, 0.0, 2, 0.75, 0.3, 0.0, 0.2, 0.5, 0.0, 0, 1.0, 0.0, 0),
        dna(0.72, 0.25, 0.15, 0.55, 0.18, 0.72),
    ),
    make_preset(
        "Staccato Engine",
        "Kinetic",
        "Algorithm 2 feedback for a harsh staccato engine sound — very short envelope, self-modulating carrier at ratio 1.",
        ["fm", "feedback", "staccato", "harsh", "kinetic"],
        p(2, 1.0, 0.7, 0.35, 0.7, 13000.0, 0.001, 0.35, 0.0, 0.12, 0.001, 0.1, 0.0, 0.5, 0.05, 0.0, 2, 0.8, 0.35, 0.0, 0.15, 0.5, 0.0, 0, 1.0, 0.0, 0),
        dna(0.78, 0.15, 0.15, 0.6, 0.15, 0.82),
    ),
    make_preset(
        "Neon Trigger",
        "Kinetic",
        "High-ratio FM with sharp gate — ratio 8, index 0.8, zero sustain. Each trigger fires a neon-bright spectral burst.",
        ["fm", "trigger", "neon", "bright", "kinetic"],
        p(0, 8.0, 0.8, 0.0, 0.75, 16000.0, 0.001, 0.3, 0.0, 0.1, 0.001, 0.08, 0.0, 0.55, 0.05, 0.0, 2, 0.82, 0.3, 0.0, 0.12, 0.5, 0.0, 0, 2.0, 0.0, 0),
        dna(0.88, 0.08, 0.12, 0.55, 0.12, 0.85),
    ),
    make_preset(
        "Metal Blip",
        "Kinetic",
        "Algorithm 2 feedback blip — ratio 2, feedback 0.3, very short. The metallic click of a relay switching.",
        ["fm", "feedback", "metal", "blip", "kinetic"],
        p(2, 2.0, 0.65, 0.3, 0.65, 14000.0, 0.001, 0.28, 0.0, 0.1, 0.001, 0.08, 0.0, 0.48, 0.05, 0.0, 2, 0.78, 0.3, 0.0, 0.12, 0.5, 0.0, 0, 1.5, 0.0, 0),
        dna(0.8, 0.12, 0.12, 0.58, 0.12, 0.82),
    ),
    make_preset(
        "Drum Synth A",
        "Kinetic",
        "FM drum voice — low ratio 0.5, high index, pitch envelope implicit in mod decay. Electronic drum with body.",
        ["fm", "drum", "percussive", "electronic", "kinetic"],
        p(0, 0.5, 0.9, 0.0, 0.7, 8000.0, 0.001, 0.5, 0.0, 0.2, 0.001, 0.18, 0.0, 0.4, 0.08, 0.0, 2, 0.7, 0.25, 0.0, 0.2, 0.5, 0.0, 0, 0.5, 0.0, 0),
        dna(0.45, 0.55, 0.12, 0.65, 0.2, 0.72),
    ),
    make_preset(
        "Clap Machine",
        "Kinetic",
        "Noisy FM clap via feedback algorithm — ratio 1, high feedback, short burst. Rhythm machine snare in FM form.",
        ["fm", "clap", "noise", "feedback", "kinetic"],
        p(2, 1.0, 0.8, 0.45, 0.75, 14500.0, 0.001, 0.2, 0.0, 0.1, 0.001, 0.06, 0.0, 0.6, 0.05, 0.0, 2, 0.85, 0.35, 0.0, 0.1, 0.5, 0.0, 0, 2.0, 0.0, 0),
        dna(0.82, 0.1, 0.12, 0.62, 0.1, 0.88),
    ),
    make_preset(
        "Sine Percussion",
        "Kinetic",
        "Pure FM pitch percussion — low index, tight decay. Sounds like a sine-wave tom struck once. Clean transient.",
        ["fm", "percussion", "sine", "clean", "kinetic"],
        p(0, 1.0, 0.5, 0.0, 0.5, 10000.0, 0.001, 0.6, 0.0, 0.25, 0.001, 0.2, 0.0, 0.25, 0.08, 0.0, 2, 0.6, 0.2, 0.0, 0.22, 0.5, 0.0, 0, 0.8, 0.0, 0),
        dna(0.58, 0.38, 0.1, 0.5, 0.22, 0.55),
    ),
    make_preset(
        "Steel Dart",
        "Kinetic",
        "High-ratio FM with instant attack and very short release — piercing, staccato. Like throwing darts of steel.",
        ["fm", "steel", "short", "aggressive", "kinetic"],
        p(0, 6.0, 0.78, 0.0, 0.7, 16000.0, 0.001, 0.25, 0.0, 0.1, 0.001, 0.08, 0.0, 0.5, 0.05, 0.0, 2, 0.82, 0.3, 0.0, 0.1, 0.5, 0.0, 0, 3.0, 0.0, 0),
        dna(0.88, 0.08, 0.1, 0.58, 0.1, 0.88),
    ),
    make_preset(
        "Digital Conga",
        "Kinetic",
        "FM conga simulation — ratio 0.75, decaying mod, velocity-mapped index. The digital version of a struck conga skin.",
        ["fm", "conga", "percussion", "digital", "kinetic"],
        p(0, 0.75, 0.7, 0.0, 0.65, 9000.0, 0.001, 0.7, 0.0, 0.3, 0.001, 0.22, 0.0, 0.38, 0.08, 0.0, 2, 0.65, 0.22, 0.0, 0.28, 0.5, 0.0, 0, 0.8, 0.0, 0),
        dna(0.52, 0.45, 0.1, 0.6, 0.28, 0.65),
    ),
    make_preset(
        "Zap Sequence",
        "Kinetic",
        "Short feedback zap — ratio 3, algorithm 2, moderate feedback. Sounds like a laser or electrostatic discharge.",
        ["fm", "zap", "feedback", "laser", "kinetic"],
        p(2, 3.0, 0.72, 0.4, 0.7, 15000.0, 0.001, 0.22, 0.0, 0.12, 0.001, 0.07, 0.0, 0.52, 0.05, 0.0, 2, 0.8, 0.35, 0.0, 0.12, 0.5, 0.0, 0, 2.0, 0.0, 0),
        dna(0.85, 0.1, 0.12, 0.6, 0.12, 0.85),
    ),
    make_preset(
        "Tabla Strike",
        "Kinetic",
        "FM tabla hit — low ratio, moderate index, decaying mod envelope. The crisp strike of a tabla at tempo.",
        ["fm", "tabla", "percussion", "world", "kinetic"],
        p(0, 0.6, 0.65, 0.0, 0.6, 9500.0, 0.001, 0.55, 0.0, 0.22, 0.001, 0.18, 0.0, 0.35, 0.1, 0.0, 2, 0.62, 0.2, 0.0, 0.22, 0.5, 0.0, 0, 0.5, 0.0, 0),
        dna(0.55, 0.42, 0.1, 0.58, 0.22, 0.62),
    ),
    make_preset(
        "Click Track FM",
        "Kinetic",
        "Ultra-short FM click — ratio 4, zero sustain, zero release. The FM click track. Nothing but the onset.",
        ["fm", "click", "short", "transient", "kinetic"],
        p(0, 4.0, 0.9, 0.0, 0.8, 14000.0, 0.001, 0.15, 0.0, 0.05, 0.001, 0.05, 0.0, 0.55, 0.03, 0.0, 2, 0.85, 0.3, 0.0, 0.08, 0.5, 0.0, 0, 5.0, 0.0, 0),
        dna(0.8, 0.12, 0.1, 0.58, 0.08, 0.9),
    ),
    make_preset(
        "Kettle FM",
        "Kinetic",
        "FM kettledrum — ratio 1.5, mod decay 0.4s. Low brightness gives the body, velocity-index creates the attack crack.",
        ["fm", "kettle", "drum", "orchestral", "kinetic"],
        p(0, 1.5, 0.75, 0.0, 0.7, 7000.0, 0.001, 1.2, 0.0, 0.5, 0.001, 0.4, 0.0, 0.4, 0.1, 0.0, 2, 0.65, 0.2, 0.0, 0.45, 0.5, 0.0, 0, 0.6, 0.0, 0),
        dna(0.4, 0.6, 0.1, 0.6, 0.45, 0.65),
    ),
    make_preset(
        "Rim Crack",
        "Kinetic",
        "High-brightness, ultra-short FM — the crack of a rim shot. Feedback algorithm, ratio 2, tight burst.",
        ["fm", "rimshot", "crack", "percussion", "kinetic"],
        p(2, 2.0, 0.82, 0.32, 0.8, 17000.0, 0.001, 0.18, 0.0, 0.08, 0.001, 0.06, 0.0, 0.58, 0.04, 0.0, 2, 0.88, 0.32, 0.0, 0.08, 0.5, 0.0, 0, 2.5, 0.0, 0),
        dna(0.88, 0.08, 0.1, 0.62, 0.08, 0.9),
    ),
    make_preset(
        "Bounce Pad",
        "Kinetic",
        "Staccato parallel FM — short envelope, LFO creates bounce rhythm. Algorithm 1, each note has its own pulse.",
        ["fm", "parallel", "staccato", "bounce", "kinetic"],
        p(1, 2.0, 0.6, 0.0, 0.55, 11000.0, 0.002, 0.45, 0.0, 0.18, 0.001, 0.12, 0.0, 0.38, 0.08, 0.0, 2, 0.7, 0.4, 0.0, 0.18, 0.5, 0.08, 3, 2.0, 0.0, 0),
        dna(0.65, 0.32, 0.45, 0.52, 0.18, 0.65),
    ),
    make_preset(
        "Hyperion Pulse",
        "Kinetic",
        "Fast LFO-gated FM pulse — rate 8Hz on LFO1, algorithm 0. Creates a mechanical tremolo-pulse at high speed.",
        ["fm", "pulse", "fast", "lfo", "kinetic"],
        p(0, 2.0, 0.7, 0.0, 0.65, 12000.0, 0.002, 0.5, 0.25, 0.22, 0.001, 0.15, 0.1, 0.4, 0.15, 0.0, 2, 0.72, 0.5, 0.0, 0.22, 8.0, 0.5, 3, 2.0, 0.0, 0),
        dna(0.72, 0.22, 0.72, 0.58, 0.22, 0.72),
    ),
    make_preset(
        "Forge Strike",
        "Kinetic",
        "Metallic FM forge hit — algorithm 2, ratio 3, feedback 0.4. Heavy, industrial, rhythmically precise.",
        ["fm", "metallic", "forge", "industrial", "kinetic"],
        p(2, 3.0, 0.85, 0.4, 0.75, 14000.0, 0.001, 0.45, 0.02, 0.18, 0.001, 0.14, 0.01, 0.55, 0.06, 0.0, 2, 0.82, 0.3, 0.0, 0.18, 0.5, 0.0, 0, 1.5, 0.0, 0),
        dna(0.8, 0.18, 0.12, 0.65, 0.18, 0.85),
    ),
    make_preset(
        "Telephone Bell",
        "Kinetic",
        "Inharmonic FM ring — ratio 2.67 creates the classic telephone partial. Short, repeating, insistent.",
        ["fm", "bell", "telephone", "inharmonic", "kinetic"],
        p(0, 2.67, 0.68, 0.0, 0.5, 13500.0, 0.001, 0.5, 0.02, 0.35, 0.001, 0.14, 0.01, 0.4, 0.08, 0.0, 2, 0.72, 0.2, 0.0, 0.35, 0.7, 0.03, 0, 3.0, 0.0, 0),
        dna(0.78, 0.2, 0.1, 0.48, 0.35, 0.6),
    ),
    make_preset(
        "Snapper",
        "Kinetic",
        "Ultra-fast FM snap — ratio 5, algorithm 2, feedback 0.25. A single harmonic snap. The finger at the end of the phrase.",
        ["fm", "snap", "short", "percussive", "kinetic"],
        p(2, 5.0, 0.75, 0.25, 0.7, 16000.0, 0.001, 0.12, 0.0, 0.06, 0.001, 0.04, 0.0, 0.52, 0.04, 0.0, 2, 0.85, 0.28, 0.0, 0.06, 0.5, 0.0, 0, 4.0, 0.0, 0),
        dna(0.85, 0.08, 0.1, 0.6, 0.06, 0.9),
    ),
    make_preset(
        "Chop Engine",
        "Kinetic",
        "Parallel algorithm percussive chop — both operators short and punchy. Layered FM transient attack for rhythmic chops.",
        ["fm", "chop", "parallel", "rhythm", "kinetic"],
        p(1, 2.5, 0.72, 0.0, 0.72, 13000.0, 0.001, 0.38, 0.0, 0.15, 0.001, 0.12, 0.0, 0.45, 0.08, 0.0, 2, 0.78, 0.3, 0.0, 0.15, 0.5, 0.0, 0, 2.0, 0.0, 0),
        dna(0.78, 0.18, 0.12, 0.6, 0.15, 0.8),
    ),
]

# ---------------------------------------------------------------------------
# LUMINOUS (15) — Smooth sine pads, warm FM
# ---------------------------------------------------------------------------

PRESETS += [
    make_preset(
        "Warm Algorithm",
        "Luminous",
        "Algorithm 1 parallel at low index — both operators add gently, creating a warm, rounded pad character.",
        ["fm", "warm", "pad", "parallel", "luminous"],
        p(1, 1.0, 0.25, 0.0, 0.25, 7500.0, 0.02, 2.5, 0.55, 1.2, 0.003, 0.5, 0.35, 0.15, 0.3, 0.0, 2, 0.42, 0.1, 0.0, 0.65, 0.3, 0.04, 0, 0.5, 0.04, 0),
        dna(0.38, 0.72, 0.12, 0.35, 0.65, 0.04),
    ),
    make_preset(
        "Honey Tone",
        "Luminous",
        "Warm FM with low brightness filter and slow LFO — golden, unhurried. The sweetest algorithm 0 patch.",
        ["fm", "honey", "warm", "slow", "luminous"],
        p(0, 1.5, 0.4, 0.0, 0.35, 7000.0, 0.015, 2.8, 0.58, 1.5, 0.003, 0.55, 0.38, 0.15, 0.32, 0.0, 2, 0.42, 0.08, 0.0, 0.68, 0.25, 0.04, 0, 0.4, 0.04, 0),
        dna(0.32, 0.75, 0.12, 0.32, 0.68, 0.04),
    ),
    make_preset(
        "Velvet Shore",
        "Luminous",
        "Slow pad FM with algorithm 1 — warm, pillowy texture. Low index keeps it smooth, high brightness gives a presence peak.",
        ["fm", "pad", "velvet", "smooth", "luminous"],
        p(1, 1.0, 0.22, 0.0, 0.2, 9000.0, 0.04, 3.0, 0.6, 2.0, 0.005, 0.6, 0.4, 0.18, 0.35, 0.0, 2, 0.4, 0.1, 0.0, 0.75, 0.2, 0.04, 0, 0.3, 0.04, 0),
        dna(0.5, 0.68, 0.12, 0.3, 0.78, 0.03),
    ),
    make_preset(
        "Digital Sunrise",
        "Luminous",
        "Parallel FM with slow attack — the sun rising through FM synthesis. Gradually brightening, rich in upper partials.",
        ["fm", "sunrise", "slow-attack", "bright", "luminous"],
        p(1, 1.5, 0.35, 0.0, 0.28, 11000.0, 0.055, 3.5, 0.55, 2.5, 0.008, 0.65, 0.38, 0.22, 0.32, 0.0, 2, 0.5, 0.12, 0.0, 0.82, 0.15, 0.04, 0, 0.25, 0.04, 0),
        dna(0.58, 0.55, 0.12, 0.32, 0.85, 0.05),
    ),
    make_preset(
        "Silk Road",
        "Luminous",
        "Smooth series FM with long release — a textural whisper of silk moving across keys. Ratio 1.2, low index.",
        ["fm", "silk", "smooth", "pad", "luminous"],
        p(0, 1.2, 0.3, 0.0, 0.25, 8000.0, 0.025, 3.2, 0.52, 2.2, 0.004, 0.6, 0.38, 0.14, 0.3, 0.0, 2, 0.38, 0.08, 0.0, 0.72, 0.2, 0.04, 0, 0.35, 0.03, 0),
        dna(0.4, 0.7, 0.1, 0.28, 0.75, 0.03),
    ),
    make_preset(
        "Noon Light",
        "Luminous",
        "Warm FM pad at moderate brightness — not harsh, not dark. The even light of a clear noon sky.",
        ["fm", "pad", "warm", "balanced", "luminous"],
        p(0, 2.0, 0.38, 0.0, 0.3, 9500.0, 0.03, 2.8, 0.55, 1.8, 0.004, 0.55, 0.35, 0.18, 0.32, 0.0, 2, 0.45, 0.08, 0.0, 0.7, 0.25, 0.04, 0, 0.4, 0.04, 0),
        dna(0.52, 0.58, 0.1, 0.32, 0.72, 0.04),
    ),
    make_preset(
        "Ochre Pad",
        "Luminous",
        "Earth-toned FM pad — low brightness, moderate index, long warm sustain. Grounded like ochre pigment.",
        ["fm", "pad", "earth", "warm", "luminous"],
        p(0, 1.0, 0.32, 0.0, 0.28, 6500.0, 0.04, 3.5, 0.6, 2.5, 0.006, 0.65, 0.42, 0.14, 0.35, 0.0, 2, 0.38, 0.08, 0.0, 0.78, 0.2, 0.04, 0, 0.3, 0.03, 0),
        dna(0.28, 0.78, 0.1, 0.32, 0.82, 0.03),
    ),
    make_preset(
        "FM Choir",
        "Luminous",
        "Parallel algorithm with slow LFO vibrato — FM approximation of a lush choir. Voices shimmer slightly.",
        ["fm", "choir", "parallel", "lush", "luminous"],
        p(1, 1.0, 0.35, 0.0, 0.3, 10000.0, 0.04, 2.5, 0.58, 1.8, 0.006, 0.5, 0.38, 0.2, 0.3, 0.0, 2, 0.5, 0.15, 0.0, 0.72, 0.25, 0.06, 0, 6.0, 0.06, 0),
        dna(0.55, 0.55, 0.22, 0.42, 0.72, 0.05),
    ),
    make_preset(
        "Dreaming Sine",
        "Luminous",
        "Near-zero index FM pad — almost pure sine. A dreaming sound at the threshold of audible harmony.",
        ["fm", "sine", "dream", "minimal", "luminous"],
        p(0, 1.0, 0.12, 0.0, 0.1, 8500.0, 0.05, 4.0, 0.55, 3.0, 0.008, 0.7, 0.45, 0.1, 0.28, 0.0, 2, 0.32, 0.08, 0.0, 0.85, 0.15, 0.03, 0, 0.2, 0.03, 0),
        dna(0.42, 0.72, 0.1, 0.22, 0.88, 0.02),
    ),
    make_preset(
        "Amber Organ",
        "Luminous",
        "Warm parallel FM with sustained pipe-like quality — slow attack, full sustain. Amber-lit church.",
        ["fm", "organ", "warm", "parallel", "luminous"],
        p(1, 1.5, 0.4, 0.0, 0.32, 8000.0, 0.015, 2.0, 0.65, 1.5, 0.003, 0.45, 0.4, 0.18, 0.32, 0.0, 2, 0.48, 0.08, 0.0, 0.62, 0.3, 0.03, 0, 0.5, 0.03, 0),
        dna(0.42, 0.68, 0.1, 0.42, 0.62, 0.04),
    ),
    make_preset(
        "Cloud Mallet",
        "Luminous",
        "High-ratio FM with slow attack transformation — the mallet strike blooms into a cloud of partials.",
        ["fm", "mallet", "cloud", "slow", "luminous"],
        p(0, 4.0, 0.45, 0.0, 0.35, 12000.0, 0.04, 3.0, 0.35, 2.5, 0.01, 0.5, 0.2, 0.25, 0.22, 0.0, 2, 0.55, 0.12, 0.0, 0.78, 0.2, 0.04, 0, 0.35, 0.04, 0),
        dna(0.62, 0.42, 0.12, 0.35, 0.78, 0.06),
    ),
    make_preset(
        "Warm String Pad",
        "Luminous",
        "Low-index FM string simulation — parallel algorithm, gentle filter envelope, warm presence. Non-realistic but expressive.",
        ["fm", "strings", "pad", "warm", "luminous"],
        p(1, 1.0, 0.3, 0.0, 0.3, 9000.0, 0.03, 2.5, 0.58, 2.0, 0.005, 0.5, 0.38, 0.18, 0.32, 0.0, 2, 0.45, 0.12, 0.0, 0.72, 0.3, 0.05, 0, 0.8, 0.05, 0),
        dna(0.5, 0.62, 0.15, 0.35, 0.72, 0.04),
    ),
    make_preset(
        "Caramel Sync",
        "Luminous",
        "Warm FM with slight feedback for richness — algorithm 2 at low feedback 0.08. Caramel harmonic texture.",
        ["fm", "warm", "feedback", "rich", "luminous"],
        p(2, 1.5, 0.45, 0.08, 0.35, 8500.0, 0.02, 2.8, 0.55, 1.8, 0.003, 0.55, 0.38, 0.18, 0.32, 0.0, 2, 0.48, 0.1, 0.0, 0.68, 0.3, 0.04, 0, 0.5, 0.04, 0),
        dna(0.48, 0.62, 0.12, 0.38, 0.68, 0.08),
    ),
    make_preset(
        "Linen Texture",
        "Luminous",
        "Low-brightness FM pad with slow LFO breath — the matte texture of linen in a quiet room.",
        ["fm", "pad", "linen", "matte", "luminous"],
        p(0, 1.0, 0.28, 0.0, 0.22, 6000.0, 0.04, 3.5, 0.58, 2.5, 0.006, 0.65, 0.42, 0.12, 0.35, 0.0, 2, 0.35, 0.06, 0.0, 0.82, 0.2, 0.04, 0, 0.25, 0.04, 0),
        dna(0.25, 0.78, 0.12, 0.28, 0.85, 0.02),
    ),
    make_preset(
        "Soft Shimmer",
        "Luminous",
        "Parallel algorithm with dual LFO shimmering — soft, layered FM wash. Luminous and gently alive.",
        ["fm", "shimmer", "parallel", "lush", "luminous"],
        p(1, 1.5, 0.32, 0.0, 0.25, 10000.0, 0.035, 3.0, 0.55, 2.2, 0.005, 0.55, 0.38, 0.2, 0.3, 0.0, 2, 0.45, 0.15, 0.0, 0.78, 0.2, 0.06, 0, 0.45, 0.06, 0),
        dna(0.55, 0.58, 0.2, 0.32, 0.8, 0.04),
    ),
]

# ---------------------------------------------------------------------------
# FLUX (15) — Evolving LFO-modulated FM
# ---------------------------------------------------------------------------

PRESETS += [
    make_preset(
        "Phase Walker",
        "Flux",
        "LFO1 slowly sweeps the FM index — as the LFO cycles, the harmonic content walks through different phases. Hypnotic.",
        ["fm", "lfo", "evolving", "modulation", "flux"],
        p(0, 2.0, 0.6, 0.0, 0.45, 10000.0, 0.01, 2.0, 0.5, 1.5, 0.002, 0.4, 0.3, 0.3, 0.28, 0.0, 2, 0.55, 0.6, 0.0, 0.6, 0.3, 0.55, 0, 0.8, 0.08, 0),
        dna(0.58, 0.42, 0.65, 0.42, 0.62, 0.15),
    ),
    make_preset(
        "Index Drift",
        "Flux",
        "FM index swept by a slow triangle LFO — the harmonic density breathes in and out over 5 seconds.",
        ["fm", "drift", "lfo", "breathing", "flux"],
        p(0, 1.5, 0.55, 0.0, 0.35, 9000.0, 0.02, 2.5, 0.52, 2.0, 0.003, 0.5, 0.35, 0.25, 0.3, 0.0, 2, 0.5, 0.55, 0.0, 0.72, 0.18, 0.5, 1, 0.5, 0.08, 0),
        dna(0.52, 0.52, 0.6, 0.38, 0.72, 0.12),
    ),
    make_preset(
        "Tremolo Machine",
        "Flux",
        "Fast amplitude LFO at 5Hz on a parallel FM tone — classic tremolo but entirely in FM domain. Rate syncs to groove.",
        ["fm", "tremolo", "lfo", "rhythmic", "flux"],
        p(1, 1.0, 0.4, 0.0, 0.35, 9500.0, 0.008, 1.8, 0.5, 1.2, 0.002, 0.4, 0.3, 0.2, 0.25, 0.0, 2, 0.5, 0.65, 0.0, 0.55, 5.0, 0.55, 0, 0.5, 0.0, 0),
        dna(0.55, 0.45, 0.7, 0.4, 0.55, 0.18),
    ),
    make_preset(
        "Spectral Wobble",
        "Flux",
        "LFO2 modulates FM index — creates a rhythmic wobble of harmonic complexity. FM synthesis as movement.",
        ["fm", "wobble", "modulated", "rhythmic", "flux"],
        p(0, 2.0, 0.8, 0.0, 0.5, 9000.0, 0.005, 1.0, 0.5, 0.5, 0.001, 0.3, 0.3, 0.35, 0.15, 0.0, 2, 0.5, 0.6, 0.0, 0.3, 0.3, 0.0, 0, 3.0, 0.3, 0),
        dna(0.55, 0.45, 0.7, 0.5, 0.4, 0.25),
    ),
    make_preset(
        "Ribbon Memory",
        "Flux",
        "Slow FM sweep — LFO1 sine at 0.1Hz sweeps ratio slowly. Each note carries the memory of a different harmonic space.",
        ["fm", "sweep", "slow", "ribbon", "flux"],
        p(0, 2.0, 0.5, 0.0, 0.4, 10000.0, 0.015, 3.0, 0.5, 2.5, 0.003, 0.6, 0.38, 0.22, 0.32, 0.0, 2, 0.5, 0.5, 0.0, 0.75, 0.12, 0.45, 0, 0.5, 0.06, 0),
        dna(0.55, 0.5, 0.55, 0.38, 0.75, 0.1),
    ),
    make_preset(
        "Arpeggio Engine",
        "Flux",
        "Fast LFO2 creates a pseudo-arpeggio tremolo from sustained FM — ratio 3, LFO2 rate 8Hz square wave.",
        ["fm", "arpeggio", "lfo", "rhythmic", "flux"],
        p(0, 3.0, 0.65, 0.0, 0.55, 11000.0, 0.003, 1.5, 0.45, 0.8, 0.001, 0.3, 0.25, 0.35, 0.2, 0.0, 2, 0.65, 0.7, 0.0, 0.35, 0.4, 0.1, 3, 8.0, 0.5, 3),
        dna(0.65, 0.38, 0.75, 0.5, 0.35, 0.32),
    ),
    make_preset(
        "Breath Motion",
        "Flux",
        "Dual LFO cross-modulation — LFO1 shapes index, LFO2 shapes brightness. The preset breathes with its own independent life.",
        ["fm", "breath", "lfo", "evolving", "flux"],
        p(0, 1.5, 0.5, 0.0, 0.38, 9500.0, 0.02, 2.5, 0.5, 1.8, 0.003, 0.5, 0.35, 0.28, 0.3, 0.0, 2, 0.5, 0.55, 0.0, 0.7, 0.25, 0.45, 0, 0.8, 0.35, 1),
        dna(0.52, 0.52, 0.62, 0.38, 0.7, 0.1),
    ),
    make_preset(
        "FM Autowah",
        "Flux",
        "LFO-driven filter envelope modulation on FM — simulates an autowah effect through FM brightness cycling.",
        ["fm", "autowah", "filter", "rhythmic", "flux"],
        p(0, 2.0, 0.7, 0.0, 0.6, 11000.0, 0.003, 1.0, 0.45, 0.6, 0.001, 0.25, 0.2, 0.5, 0.35, 0.0, 2, 0.65, 0.65, 0.0, 0.4, 4.0, 0.55, 3, 0.5, 0.0, 0),
        dna(0.65, 0.38, 0.72, 0.48, 0.4, 0.28),
    ),
    make_preset(
        "Orbiting Partial",
        "Flux",
        "Ratio slightly off-integer creates beating partial — ratio 2.05, the orbit of a slightly eccentric moon.",
        ["fm", "detuned", "beating", "orbital", "flux"],
        p(0, 2.05, 0.55, 0.0, 0.45, 10000.0, 0.005, 2.0, 0.48, 1.5, 0.001, 0.4, 0.3, 0.25, 0.28, 0.0, 2, 0.5, 0.4, 0.0, 0.65, 0.2, 0.04, 0, 0.5, 0.04, 0),
        dna(0.58, 0.48, 0.42, 0.4, 0.65, 0.12),
    ),
    make_preset(
        "Harmonic Tide",
        "Flux",
        "Very slow LFO creates harmonic tides — over 10 seconds the partial content rises and recedes like ocean water.",
        ["fm", "tide", "slow", "harmonic", "flux"],
        p(0, 1.8, 0.55, 0.0, 0.35, 9000.0, 0.03, 3.5, 0.55, 3.0, 0.005, 0.7, 0.42, 0.22, 0.35, 0.0, 2, 0.45, 0.45, 0.0, 0.85, 0.12, 0.42, 1, 0.1, 0.3, 0),
        dna(0.5, 0.55, 0.5, 0.35, 0.85, 0.08),
    ),
    make_preset(
        "Random Walk",
        "Flux",
        "LFO1 random shape — each cycle creates a new FM brightness state. Unpredictable within a musical range.",
        ["fm", "random", "lfo", "unpredictable", "flux"],
        p(0, 2.0, 0.55, 0.0, 0.45, 11000.0, 0.01, 2.0, 0.48, 1.5, 0.002, 0.4, 0.3, 0.3, 0.28, 0.0, 2, 0.55, 0.6, 0.0, 0.65, 0.8, 0.5, 4, 0.5, 0.08, 0),
        dna(0.62, 0.45, 0.68, 0.42, 0.65, 0.15),
    ),
    make_preset(
        "Vibrato Lead",
        "Flux",
        "FM lead with prominent LFO1 sine vibrato — classic keyboard vibrato at 6Hz. Expressive, alive, and slightly retro.",
        ["fm", "lead", "vibrato", "expressive", "flux"],
        p(0, 2.0, 0.62, 0.0, 0.55, 12000.0, 0.004, 1.2, 0.45, 0.8, 0.001, 0.3, 0.2, 0.32, 0.22, 0.0, 2, 0.6, 0.55, 0.0, 0.45, 6.0, 0.45, 0, 1.0, 0.08, 0),
        dna(0.65, 0.38, 0.55, 0.45, 0.45, 0.22),
    ),
    make_preset(
        "Slow Bloom",
        "Flux",
        "FM pad where index increases slowly via LFO — the sound blooms from clean to complex over 8 seconds.",
        ["fm", "bloom", "slow", "evolving", "flux"],
        p(0, 1.5, 0.4, 0.0, 0.3, 9500.0, 0.06, 4.0, 0.55, 3.5, 0.01, 0.8, 0.45, 0.2, 0.35, 0.0, 2, 0.48, 0.5, 0.0, 0.88, 0.12, 0.5, 1, 0.5, 0.06, 0),
        dna(0.52, 0.52, 0.55, 0.38, 0.88, 0.08),
    ),
    make_preset(
        "Chorus FM",
        "Flux",
        "Dual LFO creates FM chorus effect — LFO1 slow sine, LFO2 slightly faster triangle. Rich dimensional motion.",
        ["fm", "chorus", "lfo", "dimensional", "flux"],
        p(1, 1.0, 0.38, 0.0, 0.3, 10000.0, 0.02, 2.5, 0.52, 2.0, 0.003, 0.5, 0.38, 0.22, 0.3, 0.0, 2, 0.48, 0.5, 0.0, 0.72, 0.25, 0.3, 0, 0.35, 0.3, 1),
        dna(0.55, 0.52, 0.55, 0.38, 0.72, 0.08),
    ),
    make_preset(
        "Waveform Drift",
        "Flux",
        "Sawtooth LFO on index — creates a rhythmic harmonic ramp. The waveform drifts between sparse and dense cycles.",
        ["fm", "sawtooth", "lfo", "ramp", "flux"],
        p(0, 2.5, 0.6, 0.0, 0.5, 10500.0, 0.005, 1.8, 0.48, 1.2, 0.001, 0.35, 0.28, 0.32, 0.25, 0.0, 2, 0.58, 0.6, 0.0, 0.52, 1.2, 0.55, 2, 0.5, 0.1, 0),
        dna(0.62, 0.42, 0.65, 0.45, 0.52, 0.22),
    ),
]

# ---------------------------------------------------------------------------
# DEEP (15) — Low ratio sub FM, dark
# ---------------------------------------------------------------------------

PRESETS += [
    make_preset(
        "Subterranean Pulse",
        "Deep",
        "Low ratio 0.5 FM — the fundamental dominates, modulation creates sub-harmonic rumble. Below music, below language.",
        ["fm", "sub", "low", "deep", "dark"],
        p(0, 0.5, 0.55, 0.0, 0.45, 4000.0, 0.005, 2.5, 0.5, 1.5, 0.001, 0.6, 0.3, 0.2, 0.3, 0.0, 2, 0.38, 0.08, 0.0, 0.65, 0.2, 0.04, 0, 0.5, 0.03, 0),
        dna(0.18, 0.85, 0.1, 0.5, 0.65, 0.25),
    ),
    make_preset(
        "Ocean Floor",
        "Deep",
        "Very dark FM pad — brightness 2500 Hz, ratio 0.7, slow attack. The pressure and silence of the deep ocean floor.",
        ["fm", "ocean", "dark", "deep", "sub"],
        p(0, 0.7, 0.45, 0.0, 0.35, 2500.0, 0.06, 4.0, 0.6, 3.0, 0.01, 0.8, 0.45, 0.15, 0.4, 0.0, 2, 0.3, 0.06, 0.0, 0.88, 0.12, 0.04, 0, 0.2, 0.04, 0),
        dna(0.1, 0.92, 0.1, 0.4, 0.92, 0.08),
    ),
    make_preset(
        "Midnight Index",
        "Deep",
        "High index FM at midnight dark filter — 3000 Hz ceiling traps the harmonics inside. Dense, warm, claustrophobic.",
        ["fm", "dark", "dense", "warm", "deep"],
        p(0, 1.5, 0.85, 0.0, 0.6, 3000.0, 0.01, 3.0, 0.55, 2.0, 0.002, 0.6, 0.4, 0.2, 0.38, 0.0, 2, 0.45, 0.08, 0.0, 0.78, 0.2, 0.04, 0, 0.5, 0.04, 0),
        dna(0.15, 0.88, 0.1, 0.65, 0.8, 0.2),
    ),
    make_preset(
        "Drift Bass",
        "Deep",
        "FM bass voice — algorithm 0, ratio 1:1, bright filter closed for sub emphasis. Drifts below the chord.",
        ["fm", "bass", "drift", "sub", "deep"],
        p(0, 1.0, 0.7, 0.0, 0.55, 5000.0, 0.003, 1.5, 0.45, 0.8, 0.001, 0.4, 0.2, 0.25, 0.28, 0.0, 2, 0.45, 0.1, 0.0, 0.55, 0.3, 0.04, 0, 0.8, 0.03, 0),
        dna(0.28, 0.72, 0.1, 0.55, 0.55, 0.3),
    ),
    make_preset(
        "Dark Parallel",
        "Deep",
        "Algorithm 1 at low brightness — both operators sum in the sub region. Dark organ parallel voice.",
        ["fm", "parallel", "dark", "organ", "deep"],
        p(1, 0.8, 0.35, 0.0, 0.3, 4500.0, 0.01, 2.5, 0.55, 1.8, 0.002, 0.55, 0.4, 0.15, 0.32, 0.0, 2, 0.35, 0.06, 0.0, 0.72, 0.2, 0.03, 0, 0.4, 0.03, 0),
        dna(0.22, 0.82, 0.1, 0.48, 0.72, 0.1),
    ),
    make_preset(
        "Pressure Front",
        "Deep",
        "Heavy FM pad with high index and dark filter — atmospheric pressure as sound. Ratio 1.2, brightness 3500.",
        ["fm", "pressure", "heavy", "dark", "deep"],
        p(0, 1.2, 0.8, 0.0, 0.55, 3500.0, 0.04, 3.5, 0.58, 2.5, 0.006, 0.7, 0.45, 0.18, 0.4, 0.0, 2, 0.4, 0.08, 0.0, 0.85, 0.15, 0.04, 0, 0.3, 0.04, 0),
        dna(0.18, 0.88, 0.1, 0.68, 0.85, 0.22),
    ),
    make_preset(
        "Tar Pit",
        "Deep",
        "Low-end FM with high migration — the sound slowly shifts below its starting point. Dense like crude oil.",
        ["fm", "sub", "migration", "dark", "deep"],
        p(0, 0.6, 0.75, 0.0, 0.5, 3000.0, 0.015, 3.0, 0.5, 2.0, 0.003, 0.7, 0.4, 0.2, 0.38, 0.08, 2, 0.38, 0.08, 0.0, 0.78, 0.15, 0.04, 0, 0.3, 0.04, 0),
        dna(0.15, 0.88, 0.1, 0.65, 0.78, 0.28),
    ),
    make_preset(
        "Obsidian Tone",
        "Deep",
        "Smooth dark FM — obsidian-black brightness at 2200 Hz, ratio 1, slow sustained. Volcanic glass tone.",
        ["fm", "obsidian", "dark", "smooth", "deep"],
        p(0, 1.0, 0.4, 0.0, 0.3, 2200.0, 0.03, 4.0, 0.55, 3.5, 0.005, 0.75, 0.48, 0.12, 0.4, 0.0, 2, 0.32, 0.06, 0.0, 0.9, 0.12, 0.03, 0, 0.2, 0.03, 0),
        dna(0.1, 0.92, 0.1, 0.42, 0.92, 0.06),
    ),
    make_preset(
        "Trench Voice",
        "Deep",
        "Algorithm 2 feedback at extremely dark brightness — the voice from the ocean trench. Low ratio, heavy feedback rumble.",
        ["fm", "feedback", "trench", "dark", "deep"],
        p(2, 0.75, 0.65, 0.28, 0.5, 2800.0, 0.02, 3.5, 0.5, 2.5, 0.004, 0.7, 0.42, 0.18, 0.4, 0.0, 2, 0.42, 0.1, 0.0, 0.85, 0.15, 0.04, 0, 0.4, 0.04, 0),
        dna(0.12, 0.85, 0.12, 0.6, 0.85, 0.38),
    ),
    make_preset(
        "Bass Feedback",
        "Deep",
        "Algorithm 2 used as bass — self-modulating carrier at ratio 1, moderate feedback. Generates thick harmonic bass.",
        ["fm", "bass", "feedback", "thick", "deep"],
        p(2, 1.0, 0.6, 0.22, 0.55, 5500.0, 0.003, 1.8, 0.45, 1.0, 0.001, 0.45, 0.28, 0.28, 0.32, 0.0, 2, 0.48, 0.1, 0.0, 0.55, 0.3, 0.03, 0, 0.6, 0.03, 0),
        dna(0.32, 0.68, 0.1, 0.62, 0.55, 0.45),
    ),
    make_preset(
        "Fault Line",
        "Deep",
        "FM pad with very low attack transient — the slow crack of geological fault. Dark, slow, inevitably heavy.",
        ["fm", "dark", "slow", "heavy", "deep"],
        p(0, 0.8, 0.72, 0.0, 0.45, 3500.0, 0.08, 4.5, 0.55, 3.5, 0.01, 0.85, 0.48, 0.15, 0.42, 0.0, 2, 0.35, 0.06, 0.0, 0.9, 0.1, 0.04, 0, 0.2, 0.04, 0),
        dna(0.18, 0.88, 0.1, 0.55, 0.9, 0.18),
    ),
    make_preset(
        "Low Resonance",
        "Deep",
        "Dark FM bass with moderate filter resonance — bright filter but bass-focused ratio. The filter peak sings.",
        ["fm", "bass", "resonance", "deep", "filter"],
        p(0, 1.0, 0.65, 0.0, 0.5, 4500.0, 0.004, 2.0, 0.45, 1.2, 0.001, 0.5, 0.3, 0.38, 0.35, 0.0, 2, 0.5, 0.1, 0.0, 0.62, 0.25, 0.04, 0, 0.6, 0.03, 0),
        dna(0.28, 0.72, 0.1, 0.58, 0.62, 0.35),
    ),
    make_preset(
        "Magma Chamber",
        "Deep",
        "FM feedback with very low ratio 0.5 — the seismic low of a magma chamber. Slow-moving, dense, ancient.",
        ["fm", "magma", "sub", "feedback", "deep"],
        p(2, 0.5, 0.65, 0.2, 0.5, 3200.0, 0.03, 4.0, 0.52, 3.0, 0.005, 0.8, 0.45, 0.18, 0.42, 0.0, 2, 0.38, 0.08, 0.0, 0.88, 0.12, 0.04, 0, 0.3, 0.04, 0),
        dna(0.15, 0.88, 0.1, 0.68, 0.88, 0.35),
    ),
    make_preset(
        "Cave Echo",
        "Deep",
        "Dark FM with slow LFO and long release — the natural reverb of a deep cave in FM form.",
        ["fm", "cave", "dark", "echo", "deep"],
        p(0, 1.2, 0.5, 0.0, 0.38, 4000.0, 0.02, 3.0, 0.5, 4.0, 0.004, 0.65, 0.4, 0.15, 0.38, 0.0, 2, 0.38, 0.08, 0.0, 0.92, 0.15, 0.04, 0, 0.3, 0.04, 0),
        dna(0.2, 0.85, 0.1, 0.45, 0.92, 0.1),
    ),
    make_preset(
        "Depth Charge",
        "Deep",
        "Explosive FM sub hit — high index, low ratio, instant attack and long boom decay. Percussion from the deep.",
        ["fm", "sub", "bass", "explosion", "deep"],
        p(0, 0.5, 0.95, 0.0, 0.8, 3800.0, 0.001, 2.5, 0.0, 1.5, 0.001, 0.35, 0.0, 0.4, 0.15, 0.0, 2, 0.55, 0.2, 0.0, 0.55, 0.5, 0.0, 0, 0.5, 0.0, 0),
        dna(0.22, 0.78, 0.1, 0.72, 0.55, 0.78),
    ),
]

# ---------------------------------------------------------------------------
# ORGANIC (10) — Natural harmonics, woody
# ---------------------------------------------------------------------------

PRESETS += [
    make_preset(
        "Kora Tone",
        "Organic",
        "West African kora via FM — ratio 2.5, moderate index, resonant lower body. Plucked string with a ghost partial.",
        ["fm", "kora", "plucked", "african", "organic"],
        p(0, 2.5, 0.5, 0.0, 0.45, 8500.0, 0.001, 1.8, 0.08, 1.2, 0.001, 0.22, 0.04, 0.28, 0.1, 0.0, 2, 0.52, 0.1, 0.0, 0.52, 0.5, 0.03, 0, 0.8, 0.0, 0),
        dna(0.52, 0.55, 0.1, 0.42, 0.52, 0.12),
    ),
    make_preset(
        "Wood Resonance",
        "Organic",
        "Low-index FM with warm filter — the resonant body of a wooden instrument. Algorithm 1, both partials warm.",
        ["fm", "wood", "resonance", "warm", "organic"],
        p(1, 1.2, 0.3, 0.0, 0.35, 7000.0, 0.004, 1.5, 0.35, 0.8, 0.001, 0.35, 0.18, 0.18, 0.2, 0.0, 2, 0.48, 0.08, 0.0, 0.48, 0.3, 0.03, 0, 0.5, 0.03, 0),
        dna(0.4, 0.65, 0.1, 0.42, 0.48, 0.08),
    ),
    make_preset(
        "Thumb Piano",
        "Organic",
        "FM kalimba/mbira — ratio 3:1, fast transient, warm mid filter, long decay. Metal tine on a wooden board.",
        ["fm", "kalimba", "mbira", "plucked", "organic"],
        p(0, 3.0, 0.55, 0.0, 0.4, 8000.0, 0.001, 2.5, 0.06, 1.8, 0.001, 0.18, 0.03, 0.25, 0.1, 0.0, 2, 0.5, 0.12, 0.0, 0.6, 0.5, 0.03, 0, 0.8, 0.0, 0),
        dna(0.5, 0.58, 0.1, 0.38, 0.6, 0.1),
    ),
    make_preset(
        "Reed Grass",
        "Organic",
        "FM reed instrument — parallel algorithm, slow LFO breath, moderate brightness. Wind through tall grass.",
        ["fm", "reed", "wind", "breath", "organic"],
        p(1, 1.0, 0.32, 0.0, 0.3, 9000.0, 0.01, 1.8, 0.52, 1.0, 0.002, 0.38, 0.3, 0.18, 0.25, 0.0, 2, 0.45, 0.12, 0.0, 0.55, 0.3, 0.06, 0, 5.5, 0.08, 0),
        dna(0.5, 0.58, 0.2, 0.38, 0.55, 0.06),
    ),
    make_preset(
        "Struck Wood",
        "Organic",
        "FM wooden percussion — fast transient, low ratio 1.5, mid-warm brightness. Wood block with fundamental pitch.",
        ["fm", "wood", "percussion", "mallet", "organic"],
        p(0, 1.5, 0.6, 0.0, 0.55, 7500.0, 0.001, 0.6, 0.0, 0.3, 0.001, 0.15, 0.02, 0.28, 0.08, 0.0, 2, 0.55, 0.12, 0.0, 0.3, 0.5, 0.0, 0, 0.5, 0.0, 0),
        dna(0.45, 0.55, 0.1, 0.5, 0.3, 0.35),
    ),
    make_preset(
        "Plucked Vine",
        "Organic",
        "Series FM with percussive mod decay — like plucking a vine or stretched sinew. Organic, taut, immediate.",
        ["fm", "plucked", "organic", "string", "natural"],
        p(0, 2.0, 0.58, 0.0, 0.5, 9000.0, 0.001, 1.2, 0.08, 0.8, 0.001, 0.2, 0.04, 0.3, 0.1, 0.0, 2, 0.52, 0.1, 0.0, 0.42, 0.5, 0.02, 0, 0.6, 0.0, 0),
        dna(0.55, 0.52, 0.1, 0.45, 0.42, 0.2),
    ),
    make_preset(
        "Bamboo Breath",
        "Organic",
        "FM shakuhachi simulation — parallel algorithm with slow breath LFO. The breath of bamboo over a notch.",
        ["fm", "shakuhachi", "bamboo", "breath", "organic"],
        p(1, 1.0, 0.28, 0.0, 0.35, 10000.0, 0.012, 1.5, 0.48, 1.2, 0.002, 0.35, 0.28, 0.2, 0.22, 0.0, 2, 0.42, 0.15, 0.0, 0.58, 0.25, 0.07, 0, 5.8, 0.1, 0),
        dna(0.55, 0.52, 0.25, 0.32, 0.58, 0.06),
    ),
    make_preset(
        "Gong Decay",
        "Organic",
        "Inharmonic FM gong — ratio 2.756, slow decay, migration creates pitch sag. The temple gong after the mallet.",
        ["fm", "gong", "inharmonic", "resonant", "organic"],
        p(0, 2.756, 0.65, 0.0, 0.35, 11000.0, 0.001, 5.0, 0.03, 4.5, 0.001, 0.3, 0.01, 0.38, 0.08, 0.05, 2, 0.5, 0.12, 0.0, 0.85, 0.3, 0.04, 0, 0.3, 0.04, 0),
        dna(0.6, 0.42, 0.12, 0.42, 0.85, 0.12),
    ),
    make_preset(
        "Stone Chime",
        "Organic",
        "Dense inharmonic FM chime — the stony clunk of lithophone plates. Ratio 3.7, natural imperfect resonance.",
        ["fm", "chime", "stone", "lithophone", "organic"],
        p(0, 3.7, 0.55, 0.0, 0.38, 10000.0, 0.001, 3.0, 0.04, 2.5, 0.001, 0.22, 0.02, 0.32, 0.08, 0.0, 2, 0.52, 0.1, 0.0, 0.72, 0.5, 0.03, 0, 0.8, 0.0, 0),
        dna(0.58, 0.48, 0.1, 0.42, 0.72, 0.12),
    ),
    make_preset(
        "Natural Harmonic",
        "Organic",
        "Algorithm 0 at a 3:2 ratio — the perfect fifth relationship as FM. The most consonant FM voice possible.",
        ["fm", "harmonic", "consonant", "natural", "organic"],
        p(0, 1.5, 0.42, 0.0, 0.35, 9000.0, 0.003, 2.0, 0.45, 1.5, 0.001, 0.4, 0.25, 0.2, 0.25, 0.0, 2, 0.48, 0.08, 0.0, 0.62, 0.4, 0.03, 0, 0.8, 0.03, 0),
        dna(0.52, 0.58, 0.1, 0.38, 0.62, 0.08),
    ),
]

# ---------------------------------------------------------------------------
# ETHEREAL (8) — Long release, soft washes
# ---------------------------------------------------------------------------

PRESETS += [
    make_preset(
        "Celestial Wash",
        "Ethereal",
        "Very long release FM wash — brightness slowly filters out during decay. Like a note held across an ocean.",
        ["fm", "wash", "long-release", "ethereal", "ambient"],
        p(0, 1.5, 0.3, 0.0, 0.22, 9500.0, 0.06, 4.0, 0.52, 5.0, 0.01, 0.8, 0.42, 0.18, 0.38, 0.0, 2, 0.4, 0.1, 0.0, 0.92, 0.1, 0.04, 0, 0.2, 0.04, 0),
        dna(0.5, 0.62, 0.1, 0.3, 0.95, 0.03),
    ),
    make_preset(
        "Dream Horizon",
        "Ethereal",
        "Slow parallel FM pad — 80ms attack, 5 second release, slow LFO. The horizon at the edge of sleep.",
        ["fm", "dream", "pad", "slow", "ethereal"],
        p(1, 1.0, 0.28, 0.0, 0.2, 10000.0, 0.08, 4.5, 0.55, 5.0, 0.012, 0.9, 0.45, 0.15, 0.35, 0.0, 2, 0.38, 0.12, 0.0, 0.95, 0.12, 0.05, 0, 0.2, 0.05, 0),
        dna(0.52, 0.62, 0.15, 0.28, 0.98, 0.02),
    ),
    make_preset(
        "Void Tone",
        "Ethereal",
        "Near-silence FM — minimal index, near-zero attack movement, long release. The voice from the void between stars.",
        ["fm", "ambient", "void", "minimal", "ethereal"],
        p(0, 1.0, 0.15, 0.0, 0.1, 8000.0, 0.1, 5.0, 0.5, 5.0, 0.015, 1.0, 0.5, 0.1, 0.3, 0.0, 2, 0.3, 0.06, 0.0, 0.98, 0.08, 0.04, 0, 0.12, 0.04, 0),
        dna(0.42, 0.65, 0.1, 0.22, 0.98, 0.02),
    ),
    make_preset(
        "Ghost Partial",
        "Ethereal",
        "A barely-there FM voice — high ratio but near-zero index. Ghosts of upper partials, barely sustained.",
        ["fm", "ghost", "partial", "quiet", "ethereal"],
        p(0, 6.0, 0.12, 0.0, 0.08, 12000.0, 0.05, 4.0, 0.3, 5.0, 0.008, 0.7, 0.3, 0.15, 0.2, 0.0, 2, 0.28, 0.08, 0.0, 0.95, 0.12, 0.04, 0, 0.2, 0.04, 0),
        dna(0.62, 0.38, 0.1, 0.2, 0.95, 0.02),
    ),
    make_preset(
        "Astral Bell",
        "Ethereal",
        "Long-decay FM bell — ratio 4, brightness high, 5-second release. A bell that refuses to stop ringing.",
        ["fm", "bell", "long", "astral", "ethereal"],
        p(0, 4.0, 0.55, 0.0, 0.28, 15000.0, 0.001, 6.0, 0.02, 5.0, 0.001, 0.25, 0.01, 0.42, 0.06, 0.0, 2, 0.55, 0.1, 0.0, 0.98, 0.3, 0.04, 0, 0.8, 0.0, 0),
        dna(0.8, 0.2, 0.1, 0.28, 0.98, 0.06),
    ),
    make_preset(
        "Stillness",
        "Ethereal",
        "Algorithm 0 at near silence — slow attack, no decay movement, pure sustained tone. The sound of being completely still.",
        ["fm", "still", "minimal", "sustain", "ethereal"],
        p(0, 1.0, 0.2, 0.0, 0.12, 7500.0, 0.12, 5.0, 0.55, 5.0, 0.02, 1.0, 0.55, 0.08, 0.32, 0.0, 2, 0.32, 0.05, 0.0, 0.98, 0.08, 0.03, 0, 0.1, 0.03, 0),
        dna(0.38, 0.72, 0.08, 0.22, 0.98, 0.02),
    ),
    make_preset(
        "Floating World",
        "Ethereal",
        "Parallel FM with maximum release and slow LFO — floats freely in harmonic space. Ukiyo in sound.",
        ["fm", "floating", "parallel", "ambient", "ethereal"],
        p(1, 1.5, 0.25, 0.0, 0.18, 9000.0, 0.07, 4.5, 0.52, 5.0, 0.01, 0.85, 0.45, 0.15, 0.35, 0.0, 2, 0.38, 0.1, 0.0, 0.98, 0.1, 0.04, 0, 0.18, 0.04, 0),
        dna(0.48, 0.65, 0.12, 0.28, 0.98, 0.02),
    ),
    make_preset(
        "Memory Residue",
        "Ethereal",
        "FM decay tail only — minimal attack, instant mute to very long release shimmer. The memory that remains after a sound.",
        ["fm", "memory", "decay", "long", "ethereal"],
        p(0, 3.0, 0.38, 0.0, 0.22, 13000.0, 0.002, 4.0, 0.08, 5.0, 0.001, 0.35, 0.05, 0.32, 0.1, 0.0, 2, 0.42, 0.1, 0.0, 0.98, 0.2, 0.04, 0, 0.3, 0.04, 0),
        dna(0.68, 0.32, 0.1, 0.28, 0.98, 0.05),
    ),
]

# ---------------------------------------------------------------------------
# COUPLING (8) — Coupling showcase presets
# ---------------------------------------------------------------------------

PRESETS += [
    make_preset(
        "Coupling Ready",
        "Coupling",
        "Moderate-everything FM designed to showcase coupling input. All macro knobs at mid — coupling drives the character.",
        ["fm", "coupling", "modular", "open", "coupling"],
        p(0, 2.0, 0.5, 0.0, 0.5, 10000.0, 0.005, 1.5, 0.5, 1.0, 0.001, 0.35, 0.28, 0.28, 0.28, 0.0, 2, 0.5, 0.5, 0.5, 0.5, 1.0, 0.1, 0, 1.0, 0.1, 0),
        dna(0.55, 0.5, 0.45, 0.45, 0.5, 0.25),
        "Medium",
    ),
    make_preset(
        "Index Receiver",
        "Coupling",
        "FM patch optimised to receive coupling signals into the index pathway. Coupling shapes harmonic density dynamically.",
        ["fm", "coupling", "modulation", "index", "coupling"],
        p(0, 2.0, 0.55, 0.0, 0.45, 10000.0, 0.005, 1.8, 0.48, 1.2, 0.001, 0.38, 0.25, 0.3, 0.28, 0.0, 2, 0.5, 0.55, 0.65, 0.45, 1.0, 0.08, 0, 0.8, 0.08, 0),
        dna(0.58, 0.48, 0.62, 0.45, 0.5, 0.22),
        "Medium",
    ),
    make_preset(
        "Entangled EP",
        "Coupling",
        "FM EP designed for coupling with drum engines — velocity coupling input modulates index for velocity-sensitive dynamics.",
        ["fm", "ep", "coupling", "velocity", "coupling"],
        p(0, 2.0, 0.62, 0.0, 0.65, 10000.0, 0.004, 1.2, 0.45, 0.6, 0.001, 0.3, 0.18, 0.3, 0.22, 0.0, 2, 0.5, 0.5, 0.6, 0.4, 0.5, 0.04, 0, 1.0, 0.04, 0),
        dna(0.6, 0.42, 0.55, 0.48, 0.42, 0.25),
        "Medium",
    ),
    make_preset(
        "Coupling Bell",
        "Coupling",
        "FM bell that brightens dramatically with coupling input — coupling scales brightness in real time. Spectral bell-as-responder.",
        ["fm", "bell", "coupling", "dynamic", "coupling"],
        p(0, 5.0, 0.55, 0.0, 0.35, 14000.0, 0.001, 3.5, 0.04, 2.5, 0.001, 0.18, 0.01, 0.45, 0.08, 0.0, 2, 0.6, 0.5, 0.65, 0.72, 0.6, 0.04, 0, 2.0, 0.04, 0),
        dna(0.82, 0.18, 0.45, 0.35, 0.72, 0.15),
        "Medium",
    ),
    make_preset(
        "Phase Coupler",
        "Coupling",
        "Parallel FM designed for phase coupling — LFO rates lock to incoming coupling signal. Rhythmic synchronisation voice.",
        ["fm", "parallel", "coupling", "phase", "coupling"],
        p(1, 1.5, 0.45, 0.0, 0.4, 10000.0, 0.008, 2.0, 0.5, 1.5, 0.002, 0.4, 0.32, 0.25, 0.28, 0.0, 2, 0.5, 0.55, 0.7, 0.55, 2.0, 0.35, 0, 1.5, 0.35, 0),
        dna(0.57, 0.48, 0.65, 0.42, 0.55, 0.2),
        "Strong",
    ),
    make_preset(
        "Symbiote FM",
        "Coupling",
        "Low-index FM that feeds off incoming coupling energy — the symbiote: quiet alone, dramatic when coupled.",
        ["fm", "symbiote", "coupling", "reactive", "coupling"],
        p(0, 2.0, 0.3, 0.0, 0.35, 9000.0, 0.01, 2.0, 0.45, 1.5, 0.002, 0.45, 0.3, 0.2, 0.28, 0.0, 2, 0.45, 0.5, 0.75, 0.55, 0.8, 0.08, 0, 0.5, 0.08, 0),
        dna(0.5, 0.55, 0.75, 0.35, 0.55, 0.1),
        "Strong",
    ),
    make_preset(
        "Chaos Coupler",
        "Coupling",
        "Feedback FM designed for chaotic coupling — algorithm 2, moderate feedback, coupling drives index to extreme states.",
        ["fm", "feedback", "coupling", "chaos", "coupling"],
        p(2, 1.5, 0.55, 0.22, 0.55, 12000.0, 0.004, 1.5, 0.45, 1.0, 0.001, 0.35, 0.25, 0.35, 0.25, 0.0, 2, 0.6, 0.55, 0.75, 0.42, 1.5, 0.12, 0, 1.0, 0.12, 0),
        dna(0.65, 0.38, 0.72, 0.55, 0.42, 0.55),
        "Strong",
    ),
    make_preset(
        "Coupling Lead",
        "Coupling",
        "FM lead configured for maximum coupling expressivity — all coupling macro at 0.8. Coupling reshapes every dimension.",
        ["fm", "lead", "coupling", "expressive", "coupling"],
        p(0, 2.0, 0.65, 0.0, 0.6, 12000.0, 0.003, 1.0, 0.42, 0.65, 0.001, 0.28, 0.2, 0.35, 0.25, 0.0, 2, 0.65, 0.55, 0.8, 0.45, 1.0, 0.1, 0, 2.0, 0.1, 0),
        dna(0.65, 0.38, 0.8, 0.48, 0.45, 0.38),
        "Strong",
    ),
]

# ---------------------------------------------------------------------------
# PRISM (7) — Bright metallic harmonics, high aggression
# ---------------------------------------------------------------------------

PRESETS += [
    make_preset(
        "Metallic Crown",
        "Prism",
        "Algorithm 2 feedback FM at ratio 3 — the metallic crown of overtones. Harsh, shimmering, unavoidable.",
        ["fm", "metallic", "feedback", "harsh", "prism"],
        p(2, 3.0, 0.88, 0.45, 0.75, 17000.0, 0.001, 0.8, 0.15, 0.5, 0.001, 0.2, 0.05, 0.6, 0.1, 0.0, 2, 0.85, 0.35, 0.0, 0.25, 0.5, 0.0, 0, 2.0, 0.0, 0),
        dna(0.92, 0.05, 0.15, 0.65, 0.25, 0.92),
    ),
    make_preset(
        "Prism Scatter",
        "Prism",
        "High-index FM scatter — ratio 5, index 0.9, algorithm 0. Partials scatter like light through a prism. Brilliant and aggressive.",
        ["fm", "bright", "scatter", "prism", "metallic"],
        p(0, 5.0, 0.92, 0.0, 0.8, 18000.0, 0.001, 0.6, 0.1, 0.4, 0.001, 0.15, 0.03, 0.55, 0.08, 0.0, 2, 0.88, 0.3, 0.0, 0.22, 0.5, 0.0, 0, 3.0, 0.0, 0),
        dna(0.95, 0.04, 0.12, 0.62, 0.22, 0.9),
    ),
    make_preset(
        "Hard Sync FM",
        "Prism",
        "Algorithm 2 with high feedback at ratio 2 — creates hard-sync-like harmonic stacking. Aggressive and spectral.",
        ["fm", "hard-sync", "feedback", "aggressive", "prism"],
        p(2, 2.0, 0.82, 0.52, 0.78, 16000.0, 0.001, 0.9, 0.2, 0.5, 0.001, 0.22, 0.08, 0.55, 0.1, 0.0, 2, 0.88, 0.32, 0.0, 0.28, 0.5, 0.0, 0, 2.5, 0.0, 0),
        dna(0.9, 0.06, 0.12, 0.68, 0.28, 0.88),
    ),
    make_preset(
        "Spectrum Burst",
        "Prism",
        "Simultaneous parallel FM at extreme index — both operators at high level create spectral burst on attack.",
        ["fm", "parallel", "spectral", "burst", "prism"],
        p(1, 3.0, 0.9, 0.0, 0.85, 17500.0, 0.001, 0.7, 0.12, 0.4, 0.001, 0.18, 0.04, 0.6, 0.1, 0.0, 2, 0.88, 0.3, 0.0, 0.25, 0.5, 0.0, 0, 4.0, 0.0, 0),
        dna(0.93, 0.05, 0.12, 0.65, 0.25, 0.9),
    ),
    make_preset(
        "Razor Wire",
        "Prism",
        "Algorithm 2, ratio 4, feedback 0.6 — the FM equivalent of razor wire. Maximum harmonic aggression.",
        ["fm", "razor", "feedback", "extreme", "prism"],
        p(2, 4.0, 0.85, 0.6, 0.8, 18000.0, 0.001, 0.55, 0.08, 0.3, 0.001, 0.15, 0.03, 0.65, 0.08, 0.0, 2, 0.92, 0.3, 0.0, 0.18, 0.5, 0.0, 0, 3.0, 0.0, 0),
        dna(0.95, 0.04, 0.1, 0.7, 0.18, 0.95),
    ),
    make_preset(
        "Copper Harmonic",
        "Prism",
        "High-index FM at ratio 6 — metallic copper-like harmonics. Bright but with a warm undertone from filter position.",
        ["fm", "metallic", "copper", "harmonic", "prism"],
        p(0, 6.0, 0.78, 0.0, 0.7, 16500.0, 0.001, 1.0, 0.1, 0.5, 0.001, 0.22, 0.04, 0.5, 0.08, 0.0, 2, 0.82, 0.28, 0.0, 0.28, 0.5, 0.0, 0, 2.0, 0.0, 0),
        dna(0.88, 0.1, 0.1, 0.6, 0.28, 0.82),
    ),
    make_preset(
        "Light Speed",
        "Prism",
        "Maximum brightness parallel FM — both operators at full index, ratio 2, filter 20kHz. The speed of light as FM.",
        ["fm", "parallel", "extreme", "bright", "prism"],
        p(1, 2.0, 0.95, 0.0, 0.9, 20000.0, 0.001, 0.5, 0.08, 0.3, 0.001, 0.12, 0.02, 0.65, 0.08, 0.0, 2, 0.95, 0.3, 0.0, 0.18, 0.5, 0.0, 0, 5.0, 0.0, 0),
        dna(0.98, 0.02, 0.1, 0.7, 0.18, 0.95),
    ),
]


# =============================================================================
# WRITE TO DISK
# =============================================================================

def safe_filename(name):
    """Convert preset name to safe filename."""
    return name.replace("/", "-").replace("\\", "-").replace(":", "-").replace("*", "x")


def write_presets(preset_list):
    written = 0
    skipped = 0
    for preset in preset_list:
        mood = preset["mood"]
        name = preset["name"]
        filename = safe_filename(name) + ".xometa"
        dir_path = os.path.join(PRESET_BASE, mood, "Opcode")
        os.makedirs(dir_path, exist_ok=True)
        file_path = os.path.join(dir_path, filename)
        # Don't overwrite existing presets
        if os.path.exists(file_path):
            skipped += 1
            continue
        with open(file_path, "w", encoding="utf-8") as f:
            json.dump(preset, f, indent=2)
            f.write("\n")
        written += 1
    return written, skipped


def mood_report(preset_list):
    from collections import Counter
    moods = Counter(p["mood"] for p in preset_list)
    print("\nMood distribution:")
    for mood, count in sorted(moods.items()):
        print(f"  {mood:14s}: {count:3d}")
    print(f"  {'TOTAL':14s}: {sum(moods.values()):3d}")


def aggression_report(preset_list):
    high_agg = [p for p in preset_list if p["dna"]["aggression"] >= 0.7]
    print(f"\nAggression ≥ 0.7: {len(high_agg)} presets")
    for p in sorted(high_agg, key=lambda x: -x["dna"]["aggression"]):
        print(f"  [{p['dna']['aggression']:.2f}] {p['name']} ({p['mood']})")


if __name__ == "__main__":
    print(f"Generating {len(PRESETS)} Opcode factory presets...")
    written, skipped = write_presets(PRESETS)
    print(f"Written: {written}  Skipped (already exist): {skipped}")
    mood_report(PRESETS)
    aggression_report(PRESETS)
    print("\nDone.")
