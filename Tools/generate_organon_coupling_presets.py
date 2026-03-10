#!/usr/bin/env python3
"""Generate cross-engine coupling presets featuring XOrganon + other engines."""

import json
import os

PRESET_DIR = os.path.join(os.path.dirname(os.path.dirname(__file__)), "Presets", "XOmnibus")
DATE = "2026-03-10"


def make_preset(name, mood, desc, tags, engines, params, dna,
                coupling_pairs, coupling_intensity="Moderate",
                macro_labels=None, tempo=None):
    if macro_labels is None:
        macro_labels = ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"]
    return {
        "schema_version": 1,
        "name": name,
        "mood": mood,
        "engines": engines,
        "author": "XO_OX",
        "version": "1.0.0",
        "description": desc,
        "tags": tags,
        "macroLabels": macro_labels,
        "couplingIntensity": coupling_intensity,
        "tempo": tempo,
        "created": DATE,
        "legacy": {"sourceInstrument": None, "sourceCategory": None, "sourcePresetName": None},
        "parameters": params,
        "coupling": {"pairs": coupling_pairs},
        "sequencer": None,
        "dna": dna
    }


def dna(b, w, m, d, s, a):
    return {"brightness": b, "warmth": w, "movement": m, "density": d, "space": s, "aggression": a}


def write_preset(preset):
    mood_dir = os.path.join(PRESET_DIR, preset["mood"])
    os.makedirs(mood_dir, exist_ok=True)
    filename = preset["name"].replace(" ", "_").replace("/", "-") + ".xometa"
    filepath = os.path.join(mood_dir, filename)
    with open(filepath, "w") as f:
        json.dump(preset, f, indent=2)
    return filepath


def cp(engineA, engineB, ctype, amount):
    """Coupling pair shorthand."""
    return {"engineA": engineA, "engineB": engineB, "type": ctype, "amount": amount}


# Standard Organon params template
def org(mr=1.0, es=1000.0, cd=0.5, dc=0.3, sf=0.5, ps=0.0, ib=0.5, li=0.0, mb=0.2, nc=0.5):
    return {
        "organon_metabolicRate": mr, "organon_enzymeSelect": es,
        "organon_catalystDrive": cd, "organon_dampingCoeff": dc,
        "organon_signalFlux": sf, "organon_phasonShift": ps,
        "organon_isotopeBalance": ib, "organon_lockIn": li,
        "organon_membrane": mb, "organon_noiseColor": nc,
    }


# ===========================================================================
# SNAP + ORGANON — Percussive feeding
# ===========================================================================
PRESETS = [
    # --- SNAP feeds ORGANON ---
    make_preset(
        "Percussive Bloom", "Entangled",
        "SNAP percussion feeds Organon bursts of energy. Rhythmic harmonic blooms.",
        ["snap", "organon", "percussion", "bloom", "coupled"],
        ["Snap", "XOrganon"],
        {
            "Snap": {
                "snap_oscMode": 2, "snap_filterCutoff": 4000.0, "snap_resonance": 0.3,
                "snap_attack": 0.001, "snap_decay": 0.15, "snap_sustain": 0.0,
                "snap_release": 0.1, "snap_level": 0.8
            },
            "XOrganon": org(mr=3.0, es=3000.0, cd=0.8, dc=0.3, sf=0.9, ib=0.6)
        },
        dna(0.55, 0.35, 0.6, 0.5, 0.2, 0.35),
        [cp("Snap", "XOrganon", "Audio->FM", 0.7)],
        "Deep",
        ["PERCUSS", "BLOOM", "COUPLING", "SPACE"]
    ),

    make_preset(
        "Drum Digestion", "Entangled",
        "Organon slowly digests SNAP hits into evolving pad harmonics.",
        ["drum", "digest", "snap", "organon", "slow"],
        ["Snap", "XOrganon"],
        {
            "Snap": {
                "snap_oscMode": 0, "snap_filterCutoff": 2000.0, "snap_resonance": 0.5,
                "snap_attack": 0.001, "snap_decay": 0.3, "snap_sustain": 0.0,
                "snap_release": 0.2, "snap_level": 0.7
            },
            "XOrganon": org(mr=0.5, es=800.0, cd=0.6, dc=0.1, sf=0.8, ib=0.35, mb=0.4)
        },
        dna(0.3, 0.6, 0.4, 0.45, 0.4, 0.15),
        [cp("Snap", "XOrganon", "Audio->FM", 0.5)],
        "Moderate",
        ["PERCUSS", "DIGEST", "COUPLING", "SPACE"]
    ),

    # --- DUB feeds ORGANON ---
    make_preset(
        "Recycled Echoes", "Entangled",
        "DUB delay tails become recycled nutrients for Organon. Feedback ecology.",
        ["dub", "organon", "echo", "recycled", "feedback"],
        ["Dub", "XOrganon"],
        {
            "Dub": {
                "dub_oscWave": 1, "dub_level": 0.6, "dub_filterCutoff": 1500.0,
                "dub_filterReso": 0.35, "dub_attack": 0.1, "dub_decay": 1.5,
                "dub_sustain": 0.6, "dub_release": 0.8,
                "dub_delayTime": 0.375, "dub_delayFeedback": 0.55,
                "dub_delayWear": 0.3, "dub_delayMix": 0.4,
                "dub_reverbSize": 0.7, "dub_reverbMix": 0.3
            },
            "XOrganon": org(mr=0.6, es=1200.0, cd=0.5, dc=0.12, sf=0.8, ps=0.15, ib=0.45, mb=0.35)
        },
        dna(0.4, 0.55, 0.45, 0.5, 0.4, 0.1),
        [cp("Dub", "XOrganon", "Audio->FM", 0.6)],
        "Deep",
        ["DUB TONE", "ORGANISM", "COUPLING", "SPACE"]
    ),

    make_preset(
        "Dub Ecology", "Atmosphere",
        "DUB and Organon form a closed feedback loop. Self-sustaining texture.",
        ["dub", "organon", "ecology", "feedback", "self-sustaining"],
        ["Dub", "XOrganon"],
        {
            "Dub": {
                "dub_oscWave": 2, "dub_level": 0.5, "dub_filterCutoff": 800.0,
                "dub_filterReso": 0.4, "dub_attack": 0.5, "dub_decay": 3.0,
                "dub_sustain": 0.5, "dub_release": 2.0,
                "dub_delayTime": 0.5, "dub_delayFeedback": 0.65,
                "dub_delayWear": 0.4, "dub_delayMix": 0.5,
                "dub_reverbSize": 0.85, "dub_reverbMix": 0.5
            },
            "XOrganon": org(mr=0.3, es=600.0, cd=0.4, dc=0.08, sf=0.7, ps=0.1, ib=0.3, mb=0.5)
        },
        dna(0.25, 0.65, 0.4, 0.55, 0.55, 0.08),
        [
            cp("Dub", "XOrganon", "Audio->FM", 0.5),
            cp("XOrganon", "Dub", "Env->Morph", 0.3)
        ],
        "Deep",
        ["DUB ECHO", "METABOLISM", "COUPLING", "SPACE"]
    ),

    # --- MORPH feeds ORGANON ---
    make_preset(
        "Wavetable Diet", "Entangled",
        "MORPH wavetable pads become Organon's diet. Metabolized lush harmonics.",
        ["morph", "organon", "wavetable", "lush", "metabolized"],
        ["Morph", "XOrganon"],
        {
            "Morph": {
                "morph_waveA": 0, "morph_waveB": 2, "morph_position": 0.3,
                "morph_detune": 8.0, "morph_filterCutoff": 3000.0,
                "morph_filterReso": 0.25, "morph_attack": 0.3,
                "morph_decay": 1.0, "morph_sustain": 0.7, "morph_release": 1.5,
                "morph_level": 0.6
            },
            "XOrganon": org(mr=0.8, es=1500.0, cd=0.6, dc=0.15, sf=0.85, ib=0.55, mb=0.3)
        },
        dna(0.5, 0.55, 0.35, 0.55, 0.3, 0.1),
        [cp("Morph", "XOrganon", "Audio->FM", 0.65)],
        "Deep",
        ["MORPH PAD", "ORGANISM", "COUPLING", "SPACE"]
    ),

    make_preset(
        "Spectral Symbiosis", "Entangled",
        "MORPH and Organon exchange spectral material. Co-evolving texture.",
        ["morph", "organon", "spectral", "symbiosis", "co-evolving"],
        ["Morph", "XOrganon"],
        {
            "Morph": {
                "morph_waveA": 1, "morph_waveB": 3, "morph_position": 0.5,
                "morph_detune": 5.0, "morph_filterCutoff": 5000.0,
                "morph_filterReso": 0.2, "morph_attack": 0.5,
                "morph_decay": 2.0, "morph_sustain": 0.8, "morph_release": 2.0,
                "morph_level": 0.55
            },
            "XOrganon": org(mr=0.5, es=2000.0, cd=0.5, dc=0.1, sf=0.75, ps=0.1, ib=0.6, mb=0.4)
        },
        dna(0.55, 0.5, 0.4, 0.5, 0.4, 0.08),
        [
            cp("Morph", "XOrganon", "Audio->FM", 0.5),
            cp("XOrganon", "Morph", "Env->Morph", 0.25)
        ],
        "Deep",
        ["MORPH WAVE", "ORGANISM", "COUPLING", "SPACE"]
    ),

    # --- DRIFT feeds ORGANON ---
    make_preset(
        "Evolutionary Sweep", "Entangled",
        "DRIFT's long envelopes sweep Organon's isotope balance. Spectral co-evolution.",
        ["drift", "organon", "sweep", "evolution", "spectral"],
        ["Drift", "XOrganon"],
        {
            "Drift": {
                "drift_osc_a_mode": 1, "drift_osc_b_mode": 2,
                "drift_haze_amount": 0.3, "drift_delay_enable": 1.0,
                "drift_reverb_size": 0.7, "drift_master_gain": -3.0
            },
            "XOrganon": org(mr=0.4, es=1000.0, cd=0.45, dc=0.1, sf=0.7, ps=0.08, ib=0.5, mb=0.4)
        },
        dna(0.45, 0.55, 0.5, 0.45, 0.45, 0.08),
        [cp("Drift", "XOrganon", "Env->Morph", 0.6)],
        "Moderate",
        ["DRIFT ENV", "ORGANISM", "COUPLING", "SPACE"]
    ),

    make_preset(
        "Voyage Organism", "Atmosphere",
        "DRIFT navigates while Organon metabolizes the journey. Living travel pad.",
        ["drift", "organon", "voyage", "living", "travel"],
        ["Drift", "XOrganon"],
        {
            "Drift": {
                "drift_osc_a_mode": 0, "drift_haze_amount": 0.5,
                "drift_reverb_size": 0.85, "drift_master_gain": -4.0
            },
            "XOrganon": org(mr=0.3, es=800.0, cd=0.4, dc=0.08, sf=0.65, ps=0.12, ib=0.4, mb=0.5)
        },
        dna(0.35, 0.6, 0.45, 0.4, 0.55, 0.05),
        [
            cp("Drift", "XOrganon", "Audio->FM", 0.4),
            cp("Drift", "XOrganon", "Env->Morph", 0.3)
        ],
        "Moderate",
        ["DRIFT PATH", "METABOLISM", "COUPLING", "SPACE"]
    ),

    # --- ONSET feeds ORGANON ---
    make_preset(
        "Impact Nutrition", "Entangled",
        "ONSET drum hits trigger metabolic spikes. Organic percussion-to-pad.",
        ["onset", "organon", "drum", "impact", "organic"],
        ["Onset", "XOrganon"],
        {
            "Onset": {
                "onset_drumKit": 0, "onset_level": 0.8,
                "onset_attack": 0.001, "onset_decay": 0.2,
                "onset_filterCutoff": 5000.0, "onset_filterReso": 0.3
            },
            "XOrganon": org(mr=4.0, es=3500.0, cd=1.0, dc=0.35, sf=0.9, ib=0.65)
        },
        dna(0.6, 0.3, 0.55, 0.5, 0.15, 0.4),
        [cp("Onset", "XOrganon", "Audio->FM", 0.75)],
        "Deep",
        ["DRUM HIT", "BLOOM", "COUPLING", "SPACE"]
    ),

    make_preset(
        "Rhythmic Feeding", "Flux",
        "ONSET provides rhythmic nutrients. Organon locked to groove tempo.",
        ["onset", "organon", "rhythm", "tempo", "feeding"],
        ["Onset", "XOrganon"],
        {
            "Onset": {
                "onset_drumKit": 1, "onset_level": 0.7,
                "onset_attack": 0.001, "onset_decay": 0.15
            },
            "XOrganon": org(mr=2.0, es=1500.0, cd=0.7, dc=0.25, sf=0.8, ps=0.3, ib=0.5, li=0.7)
        },
        dna(0.45, 0.4, 0.7, 0.5, 0.2, 0.3),
        [
            cp("Onset", "XOrganon", "Audio->FM", 0.6),
            cp("Onset", "XOrganon", "Rhythm->Blend", 0.5)
        ],
        "Deep",
        ["DRUM FEED", "METABOLISM", "COUPLING", "SPACE"],
        tempo=120
    ),

    # --- BOB feeds ORGANON ---
    make_preset(
        "Analog Nutrition", "Entangled",
        "BOB's warm analog waveforms become high-quality nutrients for Organon.",
        ["bob", "organon", "analog", "warm", "nutrition"],
        ["Bob", "XOrganon"],
        {
            "Bob": {
                "bob_oscA_wave": 0, "bob_oscA_shape": 0.5, "bob_oscA_drift": 0.15,
                "bob_oscB_wave": 1, "bob_oscB_detune": 6.0, "bob_oscB_blend": 0.4,
                "bob_fltCutoff": 2000.0, "bob_fltReso": 0.35, "bob_fltChar": 0.6,
                "bob_ampAttack": 0.1, "bob_ampDecay": 1.0, "bob_ampSustain": 0.6,
                "bob_ampRelease": 0.8, "bob_level": 0.6
            },
            "XOrganon": org(mr=0.7, es=1000.0, cd=0.55, dc=0.12, sf=0.8, ib=0.45, mb=0.3)
        },
        dna(0.4, 0.65, 0.3, 0.5, 0.3, 0.1),
        [cp("Bob", "XOrganon", "Audio->FM", 0.55)],
        "Moderate",
        ["BOB WARMTH", "ORGANISM", "COUPLING", "SPACE"]
    ),

    make_preset(
        "Curious Machine", "Entangled",
        "BOB and Organon in mutual curiosity. Each modulates the other's character.",
        ["bob", "organon", "curious", "mutual", "machine"],
        ["Bob", "XOrganon"],
        {
            "Bob": {
                "bob_oscA_wave": 2, "bob_oscA_shape": 0.3,
                "bob_oscB_wave": 0, "bob_oscB_blend": 0.5,
                "bob_fltCutoff": 3000.0, "bob_fltReso": 0.4,
                "bob_ampAttack": 0.05, "bob_ampDecay": 0.5, "bob_ampSustain": 0.7,
                "bob_ampRelease": 0.5, "bob_curMode": 2, "bob_curAmount": 0.5,
                "bob_level": 0.55
            },
            "XOrganon": org(mr=1.5, es=2000.0, cd=0.6, dc=0.2, sf=0.75, ps=0.15, ib=0.55)
        },
        dna(0.5, 0.5, 0.45, 0.5, 0.25, 0.15),
        [
            cp("Bob", "XOrganon", "Audio->FM", 0.45),
            cp("XOrganon", "Bob", "Amp->Filter", 0.3)
        ],
        "Deep",
        ["BOB CHAR", "ORGANISM", "COUPLING", "SPACE"]
    ),

    # --- FAT feeds ORGANON ---
    make_preset(
        "Supersize Organism", "Entangled",
        "FAT's massive harmonics become an all-you-can-eat buffet for Organon.",
        ["fat", "organon", "massive", "supersize", "dense"],
        ["Fat", "XOrganon"],
        {
            "Fat": {
                "fat_oscMode": 1, "fat_unisonVoices": 7, "fat_detune": 0.3,
                "fat_filterCutoff": 4000.0, "fat_filterReso": 0.25,
                "fat_attack": 0.1, "fat_decay": 0.8, "fat_sustain": 0.7,
                "fat_release": 1.0, "fat_level": 0.6
            },
            "XOrganon": org(mr=1.5, es=2000.0, cd=0.8, dc=0.2, sf=0.9, ib=0.6)
        },
        dna(0.55, 0.45, 0.35, 0.8, 0.2, 0.25),
        [cp("Fat", "XOrganon", "Audio->FM", 0.7)],
        "Deep",
        ["FAT MASS", "ORGANISM", "COUPLING", "SPACE"]
    ),

    # --- Triple-engine presets ---
    make_preset(
        "Food Chain", "Entangled",
        "SNAP feeds BOB feeds Organon. Three-tier metabolic food chain.",
        ["snap", "bob", "organon", "food-chain", "three-tier"],
        ["Snap", "Bob", "XOrganon"],
        {
            "Snap": {
                "snap_oscMode": 1, "snap_filterCutoff": 3000.0,
                "snap_attack": 0.001, "snap_decay": 0.1, "snap_level": 0.7
            },
            "Bob": {
                "bob_oscA_wave": 0, "bob_oscA_shape": 0.4,
                "bob_fltCutoff": 1500.0, "bob_fltReso": 0.3,
                "bob_ampAttack": 0.05, "bob_ampDecay": 0.5, "bob_ampSustain": 0.5,
                "bob_ampRelease": 0.5, "bob_level": 0.5
            },
            "XOrganon": org(mr=0.8, es=1200.0, cd=0.6, dc=0.15, sf=0.85, ib=0.5, mb=0.3)
        },
        dna(0.45, 0.5, 0.5, 0.6, 0.3, 0.2),
        [
            cp("Snap", "Bob", "Audio->FM", 0.4),
            cp("Bob", "XOrganon", "Audio->FM", 0.6)
        ],
        "Deep",
        ["PREDATOR", "PREY", "COUPLING", "SPACE"]
    ),

    make_preset(
        "Ecosystem", "Entangled",
        "DUB, MORPH, and Organon form a self-sustaining sonic ecosystem.",
        ["dub", "morph", "organon", "ecosystem", "self-sustaining"],
        ["Dub", "Morph", "XOrganon"],
        {
            "Dub": {
                "dub_oscWave": 1, "dub_level": 0.5, "dub_filterCutoff": 1200.0,
                "dub_delayTime": 0.375, "dub_delayFeedback": 0.5,
                "dub_delayMix": 0.35, "dub_reverbSize": 0.6, "dub_reverbMix": 0.25,
                "dub_attack": 0.2, "dub_decay": 2.0, "dub_sustain": 0.5, "dub_release": 1.5
            },
            "Morph": {
                "morph_waveA": 0, "morph_waveB": 2, "morph_position": 0.4,
                "morph_filterCutoff": 2500.0, "morph_attack": 0.3,
                "morph_sustain": 0.6, "morph_level": 0.5
            },
            "XOrganon": org(mr=0.5, es=1000.0, cd=0.5, dc=0.1, sf=0.75, ps=0.1, ib=0.45, mb=0.4)
        },
        dna(0.4, 0.55, 0.45, 0.6, 0.4, 0.1),
        [
            cp("Dub", "XOrganon", "Audio->FM", 0.4),
            cp("Morph", "XOrganon", "Audio->FM", 0.35),
            cp("XOrganon", "Morph", "Env->Morph", 0.2)
        ],
        "Deep",
        ["DUB ECHO", "MORPH PAD", "ORGANISM", "SPACE"]
    ),

    # --- Atmosphere/Aether specials ---
    make_preset(
        "Symbiotic Cathedral", "Atmosphere",
        "DRIFT's vast space coupled with Organon's living harmonics. Sacred biology.",
        ["drift", "organon", "cathedral", "sacred", "vast"],
        ["Drift", "XOrganon"],
        {
            "Drift": {
                "drift_osc_a_mode": 0, "drift_haze_amount": 0.6,
                "drift_reverb_size": 0.95, "drift_master_gain": -5.0
            },
            "XOrganon": org(mr=0.2, es=1200.0, cd=0.4, dc=0.06, sf=0.6, ib=0.45, mb=0.65)
        },
        dna(0.4, 0.55, 0.3, 0.4, 0.7, 0.05),
        [cp("Drift", "XOrganon", "Audio->FM", 0.35)],
        "Subtle",
        ["DRIFT SPACE", "ORGANISM", "COUPLING", "REVERB"]
    ),

    make_preset(
        "Glacial Metabolism", "Aether",
        "Ultra-slow Organon fed by MORPH's glacial wavetable. Minutes-long evolution.",
        ["morph", "organon", "glacial", "ultra-slow", "minutes"],
        ["Morph", "XOrganon"],
        {
            "Morph": {
                "morph_waveA": 0, "morph_waveB": 1, "morph_position": 0.2,
                "morph_filterCutoff": 1500.0, "morph_attack": 3.0,
                "morph_decay": 10.0, "morph_sustain": 0.8, "morph_release": 5.0,
                "morph_level": 0.4
            },
            "XOrganon": org(mr=0.1, es=800.0, cd=0.35, dc=0.04, sf=0.5, ib=0.4, mb=0.55)
        },
        dna(0.35, 0.55, 0.2, 0.35, 0.55, 0.03),
        [cp("Morph", "XOrganon", "Audio->FM", 0.3)],
        "Subtle",
        ["MORPH WAVE", "GLACIAL", "COUPLING", "SPACE"]
    ),

    # --- Prism specials ---
    make_preset(
        "Metabolic Lead", "Prism",
        "SNAP attack feeds Organon into a hyper-metabolic crystalline lead.",
        ["snap", "organon", "lead", "crystal", "hyper"],
        ["Snap", "XOrganon"],
        {
            "Snap": {
                "snap_oscMode": 3, "snap_filterCutoff": 6000.0, "snap_resonance": 0.4,
                "snap_attack": 0.001, "snap_decay": 0.05, "snap_level": 0.75
            },
            "XOrganon": org(mr=8.0, es=5000.0, cd=1.5, dc=0.4, sf=0.9, ib=0.8)
        },
        dna(0.8, 0.15, 0.5, 0.55, 0.1, 0.6),
        [cp("Snap", "XOrganon", "Audio->FM", 0.8)],
        "Deep",
        ["SNAP HIT", "HYPER", "COUPLING", "SPACE"]
    ),

    # --- Flux specials ---
    make_preset(
        "Cardiac Coupling", "Flux",
        "BOB's warm pulse synced to Organon's metabolic heartbeat.",
        ["bob", "organon", "cardiac", "heartbeat", "sync"],
        ["Bob", "XOrganon"],
        {
            "Bob": {
                "bob_oscA_wave": 0, "bob_oscA_shape": 0.4, "bob_oscA_drift": 0.1,
                "bob_fltCutoff": 1500.0, "bob_fltReso": 0.35,
                "bob_ampAttack": 0.05, "bob_ampDecay": 0.3, "bob_ampSustain": 0.5,
                "bob_ampRelease": 0.3, "bob_level": 0.55,
                "bob_lfo1Rate": 1.0, "bob_lfo1Depth": 0.2
            },
            "XOrganon": org(mr=1.2, es=800.0, cd=0.5, dc=0.15, sf=0.7, ps=0.3, ib=0.4, li=0.8)
        },
        dna(0.35, 0.6, 0.6, 0.45, 0.2, 0.1),
        [
            cp("Bob", "XOrganon", "Audio->FM", 0.4),
            cp("Bob", "XOrganon", "Rhythm->Blend", 0.5)
        ],
        "Moderate",
        ["BOB PULSE", "HEARTBEAT", "COUPLING", "SPACE"],
        tempo=72
    ),
]


def main():
    names = [p["name"] for p in PRESETS]
    dupes = [n for n in names if names.count(n) > 1]
    if dupes:
        print(f"WARNING: Duplicate names: {set(dupes)}")

    count_by_mood = {}
    for preset in PRESETS:
        write_preset(preset)
        mood = preset["mood"]
        count_by_mood[mood] = count_by_mood.get(mood, 0) + 1

    print(f"Generated {len(PRESETS)} cross-engine coupling presets:")
    for mood, count in sorted(count_by_mood.items()):
        print(f"  {mood}: {count}")
    print(f"\nEngine pairings:")
    pairings = {}
    for p in PRESETS:
        engines = sorted(e for e in p["engines"] if e != "XOrganon")
        key = " + ".join(engines) + " + Organon"
        pairings[key] = pairings.get(key, 0) + 1
    for pair, count in sorted(pairings.items()):
        print(f"  {pair}: {count}")


if __name__ == "__main__":
    main()
