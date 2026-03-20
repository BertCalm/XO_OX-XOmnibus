#!/usr/bin/env python3
"""OBRIX Wave 4 Preset Generator — 150 factory presets for the flagship reef engine.

Generates .xometa JSON files across 7 moods:
  Foundation (30): 22 Lesson + 8 basic
  Atmosphere (25): 20 Place + 5 ambient
  Entangled (20): coupling showcases
  Prism (25): Techno/Synthwave/genre production
  Flux (20): EDM/Lo-Fi movement
  Aether (15): experimental/drift/journey
  Family (15): OBRIX + fleet engine coupling
"""

import json
import os

PRESET_DIR = os.path.join(os.path.dirname(__file__), "..", "Presets", "XOmnibus")

# ── OBRIX defaults (65 params) ──────────────────────────────────────────────
DEFAULTS = {
    "obrix_src1Type": 1,       # Sine
    "obrix_src2Type": 0,       # Off
    "obrix_src1Tune": 0.0,
    "obrix_src2Tune": 0.0,
    "obrix_src1PW": 0.5,
    "obrix_src2PW": 0.5,
    "obrix_srcMix": 0.5,
    "obrix_proc1Type": 1,      # LP Filter
    "obrix_proc1Cutoff": 8000.0,
    "obrix_proc1Reso": 0.0,
    "obrix_proc2Type": 0,
    "obrix_proc2Cutoff": 4000.0,
    "obrix_proc2Reso": 0.0,
    "obrix_proc3Type": 0,
    "obrix_proc3Cutoff": 4000.0,
    "obrix_proc3Reso": 0.0,
    "obrix_ampAttack": 0.01,
    "obrix_ampDecay": 0.3,
    "obrix_ampSustain": 0.7,
    "obrix_ampRelease": 0.5,
    "obrix_mod1Type": 1,       # Envelope
    "obrix_mod1Target": 2,     # Filter Cutoff
    "obrix_mod1Depth": 0.5,
    "obrix_mod1Rate": 1.0,
    "obrix_mod2Type": 2,       # LFO
    "obrix_mod2Target": 2,     # Filter Cutoff
    "obrix_mod2Depth": 0.0,
    "obrix_mod2Rate": 1.0,
    "obrix_mod3Type": 3,       # Velocity
    "obrix_mod3Target": 4,     # Volume
    "obrix_mod3Depth": 0.5,
    "obrix_mod3Rate": 1.0,
    "obrix_mod4Type": 4,       # Aftertouch
    "obrix_mod4Target": 2,     # Filter Cutoff
    "obrix_mod4Depth": 0.0,
    "obrix_mod4Rate": 1.0,
    "obrix_fx1Type": 0,
    "obrix_fx1Mix": 0.0,
    "obrix_fx1Param": 0.3,
    "obrix_fx2Type": 0,
    "obrix_fx2Mix": 0.0,
    "obrix_fx2Param": 0.3,
    "obrix_fx3Type": 0,
    "obrix_fx3Mix": 0.0,
    "obrix_fx3Param": 0.3,
    "obrix_level": 0.8,
    "obrix_macroCharacter": 0.0,
    "obrix_macroMovement": 0.0,
    "obrix_macroCoupling": 0.0,
    "obrix_macroSpace": 0.0,
    "obrix_polyphony": 3,      # Poly8
    "obrix_pitchBendRange": 2.0,
    "obrix_glideTime": 0.0,
    "obrix_gestureType": 0,
    "obrix_flashTrigger": 0.0,
    "obrix_fmDepth": 0.0,
    "obrix_proc1Feedback": 0.0,
    "obrix_proc2Feedback": 0.0,
    "obrix_wtBank": 0,
    "obrix_unisonDetune": 0.0,
    "obrix_driftRate": 0.005,
    "obrix_driftDepth": 0.0,
    "obrix_journeyMode": 0.0,
    "obrix_distance": 0.0,
    "obrix_air": 0.5,
}

# Source type constants
OFF, SINE, SAW, SQUARE, TRI, NOISE, WT, PULSE, LOFI = range(9)
# Proc type constants
P_OFF, LP, HP, BP, FOLD, RING = range(6)
# Mod type constants
M_OFF, ENV, LFO, VEL, AT = range(5)
# Mod target constants
T_NONE, T_PITCH, T_CUTOFF, T_RESO, T_VOL, T_WTPOS, T_PW, T_FXMIX, T_PAN = range(9)
# FX type constants
FX_OFF, DELAY, CHORUS, REVERB = range(4)
# Voice modes
MONO, LEGATO, POLY4, POLY8 = range(4)


def make_preset(name, mood, overrides, dna, desc="", tags=None, engines=None,
                coupling=None, coupling_intensity="None"):
    """Build a full .xometa dict from name + param overrides."""
    params = dict(DEFAULTS)
    params.update(overrides)

    preset = {
        "schema_version": 1,
        "name": name,
        "mood": mood,
        "engines": engines or ["Obrix"],
        "author": "XO_OX Designs",
        "version": "1.0.0",
        "description": desc,
        "tags": tags or [],
        "macroLabels": ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"],
        "couplingIntensity": coupling_intensity,
        "tempo": None,
        "dna": {
            "brightness": dna[0], "warmth": dna[1], "movement": dna[2],
            "density": dna[3], "space": dna[4], "aggression": dna[5]
        },
        "parameters": {"Obrix": params},
    }
    if coupling:
        preset["coupling"] = {"pairs": coupling}
    return preset


def save(preset):
    """Write preset to the correct mood directory."""
    mood = preset["mood"]
    safe_name = preset["name"].replace(" ", "_").replace("'", "").replace(",", "")
    filename = f"Obrix_{safe_name}.xometa"
    path = os.path.join(PRESET_DIR, mood, filename)
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w") as f:
        json.dump(preset, f, indent=2)
    return path


# ══════════════════════════════════════════════════════════════════════════════
# 4A. LESSON PRESETS — 22 sequential presets teaching synthesis brick by brick
# ══════════════════════════════════════════════════════════════════════════════

LESSONS = [
    # 1. One Sine — pure sine, no filter, no mod
    ("One Sine", {
        "obrix_proc1Type": P_OFF,
        "obrix_mod1Type": M_OFF, "obrix_mod1Depth": 0.0,
        "obrix_mod2Type": M_OFF, "obrix_mod3Type": M_OFF, "obrix_mod4Type": M_OFF,
    }, (0.3, 0.6, 0.0, 0.1, 0.0, 0.0),
     "A single sine wave. The simplest sound. The starting brick.",
     ["lesson", "sine", "simple", "beginner"]),

    # 2. Add Filter — introduce LP filter
    ("Add Filter", {
        "obrix_proc1Cutoff": 2000.0, "obrix_proc1Reso": 0.2,
        "obrix_mod1Type": M_OFF, "obrix_mod2Type": M_OFF,
        "obrix_mod3Type": M_OFF, "obrix_mod4Type": M_OFF,
    }, (0.25, 0.7, 0.0, 0.15, 0.0, 0.0),
     "Same sine, now through a low-pass filter. Notice how brightness changes.",
     ["lesson", "filter", "subtractive"]),

    # 3. First Sweep — envelope modulates cutoff
    ("First Sweep", {
        "obrix_proc1Cutoff": 800.0, "obrix_proc1Reso": 0.35,
        "obrix_mod1Depth": 0.7, "obrix_mod1Rate": 0.8,
        "obrix_mod2Type": M_OFF, "obrix_mod3Type": M_OFF, "obrix_mod4Type": M_OFF,
    }, (0.4, 0.65, 0.3, 0.2, 0.0, 0.1),
     "The envelope sweeps the filter open on each note. The voice begins to breathe.",
     ["lesson", "envelope", "sweep", "filter"]),

    # 4. Two Voices — add Source 2
    ("Two Voices", {
        "obrix_src2Type": SAW, "obrix_src2Tune": 7.0,
        "obrix_proc1Cutoff": 1500.0, "obrix_proc1Reso": 0.3,
        "obrix_mod1Depth": 0.5,
        "obrix_mod2Type": M_OFF, "obrix_mod3Type": M_OFF, "obrix_mod4Type": M_OFF,
    }, (0.45, 0.55, 0.15, 0.35, 0.0, 0.15),
     "A saw wave joins the sine, tuned a fifth above. Two sources, one filter.",
     ["lesson", "two-osc", "interval", "fifth"]),

    # 5. The Collision — split processor routing revealed
    ("The Collision", {
        "obrix_src1Type": SAW, "obrix_src2Type": SQUARE,
        "obrix_proc1Cutoff": 3000.0, "obrix_proc1Reso": 0.4,
        "obrix_proc2Type": HP, "obrix_proc2Cutoff": 800.0, "obrix_proc2Reso": 0.3,
        "obrix_proc3Type": FOLD, "obrix_proc3Cutoff": 5000.0,
        "obrix_mod1Depth": 0.6,
        "obrix_mod2Type": M_OFF, "obrix_mod3Type": M_OFF, "obrix_mod4Type": M_OFF,
    }, (0.55, 0.5, 0.2, 0.5, 0.0, 0.4),
     "LP on Source 1, HP on Source 2, wavefolder after the mix. The Constructive Collision.",
     ["lesson", "collision", "wavefolder", "split-routing"]),

    # 6. LFO Breathes — add LFO modulation
    ("LFO Breathes", {
        "obrix_src1Type": SAW,
        "obrix_proc1Cutoff": 2000.0, "obrix_proc1Reso": 0.35,
        "obrix_mod1Depth": 0.5,
        "obrix_mod2Depth": 0.3, "obrix_mod2Rate": 0.5,
        "obrix_mod3Type": M_OFF, "obrix_mod4Type": M_OFF,
    }, (0.4, 0.6, 0.5, 0.25, 0.0, 0.1),
     "An LFO slowly opens and closes the filter. The sound breathes.",
     ["lesson", "lfo", "modulation", "breathing"]),

    # 7. Velocity Shapes — D001 velocity-to-timbre
    ("Velocity Shapes", {
        "obrix_src1Type": SAW,
        "obrix_proc1Cutoff": 1200.0, "obrix_proc1Reso": 0.4,
        "obrix_mod1Depth": 0.6,
        "obrix_mod2Depth": 0.2, "obrix_mod2Rate": 0.8,
        "obrix_mod3Depth": 0.8,
        "obrix_mod4Type": M_OFF,
    }, (0.45, 0.55, 0.4, 0.3, 0.0, 0.2),
     "Play soft: dark and gentle. Play hard: bright and bold. Velocity shapes timbre.",
     ["lesson", "velocity", "dynamics", "expression"]),

    # 8. Aftertouch Speaks — pressure adds expression
    ("Aftertouch Speaks", {
        "obrix_src1Type": SAW,
        "obrix_proc1Cutoff": 1500.0, "obrix_proc1Reso": 0.35,
        "obrix_mod1Depth": 0.5,
        "obrix_mod2Depth": 0.2, "obrix_mod2Rate": 0.6,
        "obrix_mod3Depth": 0.7,
        "obrix_mod4Depth": 0.5,
    }, (0.45, 0.55, 0.45, 0.3, 0.0, 0.15),
     "Press into the keys after striking. Aftertouch opens the filter further.",
     ["lesson", "aftertouch", "expression", "pressure"]),

    # 9. Ring Mod Bells — metallic tones
    ("Ring Mod Bells", {
        "obrix_src1Type": SINE, "obrix_src2Type": SINE,
        "obrix_src2Tune": 19.0,
        "obrix_proc1Type": RING,
        "obrix_ampAttack": 0.001, "obrix_ampDecay": 1.5,
        "obrix_ampSustain": 0.0, "obrix_ampRelease": 1.5,
        "obrix_mod1Type": M_OFF, "obrix_mod2Type": M_OFF,
        "obrix_mod3Depth": 0.6, "obrix_mod4Type": M_OFF,
    }, (0.6, 0.3, 0.1, 0.25, 0.1, 0.2),
     "Two sines, an octave and a fifth apart, through ring mod. Metallic bell tones.",
     ["lesson", "ring-mod", "bells", "metallic"]),

    # 10. FM Depth — source-to-source FM
    ("FM Depth", {
        "obrix_src2Type": SINE,
        "obrix_fmDepth": 0.3,
        "obrix_proc1Cutoff": 6000.0,
        "obrix_mod1Depth": 0.4,
        "obrix_mod2Type": M_OFF,
        "obrix_mod3Depth": 0.6, "obrix_mod4Type": M_OFF,
    }, (0.55, 0.45, 0.2, 0.35, 0.0, 0.25),
     "Source 1 frequency-modulates Source 2. Velocity controls how much FM.",
     ["lesson", "fm", "frequency-modulation", "harmonic"]),

    # 11. Wavetable World — wavetable oscillator
    ("Wavetable World", {
        "obrix_src1Type": WT, "obrix_wtBank": 1,
        "obrix_src1PW": 0.7,
        "obrix_proc1Cutoff": 5000.0, "obrix_proc1Reso": 0.2,
        "obrix_mod1Depth": 0.4,
        "obrix_mod2Depth": 0.15, "obrix_mod2Rate": 0.3,
        "obrix_mod3Depth": 0.5, "obrix_mod4Type": M_OFF,
    }, (0.5, 0.55, 0.35, 0.3, 0.0, 0.1),
     "The Vocal wavetable. Pulse width morphs between sine and full formant character.",
     ["lesson", "wavetable", "vocal", "formant"]),

    # 12. Pulse Width — PWM synthesis
    ("Pulse Width", {
        "obrix_src1Type": PULSE, "obrix_src1PW": 0.3,
        "obrix_proc1Cutoff": 4000.0, "obrix_proc1Reso": 0.25,
        "obrix_mod1Depth": 0.5,
        "obrix_mod2Target": T_PW, "obrix_mod2Depth": 0.4, "obrix_mod2Rate": 0.4,
        "obrix_mod3Depth": 0.6, "obrix_mod4Type": M_OFF,
    }, (0.5, 0.6, 0.5, 0.25, 0.0, 0.1),
     "Pulse wave with LFO modulating width. Classic analog chorus-like motion.",
     ["lesson", "pulse", "pwm", "analog"]),

    # 13. Filter Feedback — self-oscillation territory
    ("Filter Feedback", {
        "obrix_src1Type": SAW,
        "obrix_proc1Cutoff": 1000.0, "obrix_proc1Reso": 0.6,
        "obrix_proc1Feedback": 0.55,
        "obrix_mod1Depth": 0.7, "obrix_mod1Rate": 1.2,
        "obrix_mod2Depth": 0.15, "obrix_mod2Rate": 0.3,
        "obrix_mod3Depth": 0.5, "obrix_mod4Type": M_OFF,
    }, (0.45, 0.5, 0.4, 0.35, 0.0, 0.35),
     "Filter output fed back through tanh saturation. Push feedback up to hear self-oscillation.",
     ["lesson", "feedback", "self-oscillation", "saturation"]),

    # 14. The Supersaw — unison detune
    ("The Supersaw", {
        "obrix_src1Type": SAW,
        "obrix_polyphony": MONO,
        "obrix_unisonDetune": 20.0,
        "obrix_proc1Cutoff": 6000.0, "obrix_proc1Reso": 0.15,
        "obrix_mod1Depth": 0.3,
        "obrix_mod2Type": M_OFF,
        "obrix_mod3Depth": 0.5, "obrix_mod4Type": M_OFF,
        "obrix_level": 0.6,
    }, (0.6, 0.5, 0.3, 0.7, 0.1, 0.3),
     "Mono mode + unison detune = 8 detuned saws. The festival lead brick.",
     ["lesson", "supersaw", "unison", "detune", "lead"]),

    # 15. Lo-Fi Character — naive saw
    ("Lo-Fi Character", {
        "obrix_src1Type": LOFI,
        "obrix_proc1Cutoff": 3000.0, "obrix_proc1Reso": 0.2,
        "obrix_mod1Depth": 0.4,
        "obrix_mod2Depth": 0.1, "obrix_mod2Rate": 0.4,
        "obrix_mod3Depth": 0.5, "obrix_mod4Type": M_OFF,
    }, (0.35, 0.65, 0.25, 0.2, 0.0, 0.15),
     "The naive saw — intentionally aliased for lo-fi warmth. Not a bug, a feature.",
     ["lesson", "lofi", "aliased", "character"]),

    # 16. Delay Echo — first effect
    ("Delay Echo", {
        "obrix_src1Type": SAW,
        "obrix_proc1Cutoff": 3000.0, "obrix_proc1Reso": 0.3,
        "obrix_mod1Depth": 0.5,
        "obrix_mod3Depth": 0.6, "obrix_mod4Type": M_OFF,
        "obrix_fx1Type": DELAY, "obrix_fx1Mix": 0.3, "obrix_fx1Param": 0.4,
    }, (0.45, 0.55, 0.4, 0.3, 0.3, 0.1),
     "Rhythmic delay echoes. Param controls delay time and feedback amount.",
     ["lesson", "delay", "echo", "fx"]),

    # 17. Chorus Width — stereo effect
    ("Chorus Width", {
        "obrix_src1Type": SAW,
        "obrix_proc1Cutoff": 4000.0, "obrix_proc1Reso": 0.2,
        "obrix_mod1Depth": 0.4,
        "obrix_mod3Depth": 0.5, "obrix_mod4Type": M_OFF,
        "obrix_fx1Type": CHORUS, "obrix_fx1Mix": 0.4, "obrix_fx1Param": 0.5,
    }, (0.5, 0.6, 0.5, 0.3, 0.4, 0.05),
     "Chorus widens the sound across stereo. A single saw becomes an ensemble.",
     ["lesson", "chorus", "stereo", "width"]),

    # 18. Reverb Space — spatial depth
    ("Reverb Space", {
        "obrix_src1Type": SAW,
        "obrix_ampAttack": 0.05, "obrix_ampRelease": 1.0,
        "obrix_proc1Cutoff": 3500.0, "obrix_proc1Reso": 0.25,
        "obrix_mod1Depth": 0.4,
        "obrix_mod3Depth": 0.5, "obrix_mod4Type": M_OFF,
        "obrix_fx1Type": REVERB, "obrix_fx1Mix": 0.35, "obrix_fx1Param": 0.5,
    }, (0.45, 0.6, 0.3, 0.3, 0.6, 0.05),
     "Reverb places the sound in a room. SPACE macro adds more.",
     ["lesson", "reverb", "space", "room"]),

    # 19. Macro Play — macros shape the sound
    ("Macro Play", {
        "obrix_src1Type": SAW, "obrix_src2Type": SQUARE,
        "obrix_proc1Cutoff": 2500.0, "obrix_proc1Reso": 0.35,
        "obrix_proc2Type": HP, "obrix_proc2Cutoff": 600.0,
        "obrix_mod1Depth": 0.5,
        "obrix_mod2Depth": 0.2, "obrix_mod2Rate": 0.5,
        "obrix_mod3Depth": 0.6,
        "obrix_fx1Type": CHORUS, "obrix_fx1Mix": 0.25, "obrix_fx1Param": 0.4,
        "obrix_fx2Type": REVERB, "obrix_fx2Mix": 0.2, "obrix_fx2Param": 0.4,
        "obrix_macroCharacter": 0.3, "obrix_macroMovement": 0.2,
        "obrix_macroSpace": 0.15,
    }, (0.5, 0.55, 0.5, 0.45, 0.35, 0.2),
     "CHARACTER adds cutoff + fold depth. MOVEMENT adds detune + LFO. SPACE adds reverb.",
     ["lesson", "macros", "performance", "expressive"]),

    # 20. Drift Ensemble — Schulze's slow ensemble
    ("Drift Ensemble", {
        "obrix_src1Type": SAW, "obrix_src2Type": SAW, "obrix_src2Tune": 0.1,
        "obrix_proc1Cutoff": 3000.0, "obrix_proc1Reso": 0.3,
        "obrix_proc2Type": LP, "obrix_proc2Cutoff": 4000.0,
        "obrix_mod1Depth": 0.4,
        "obrix_mod2Depth": 0.15, "obrix_mod2Rate": 0.3,
        "obrix_mod3Depth": 0.5,
        "obrix_driftRate": 0.008, "obrix_driftDepth": 0.45,
        "obrix_fx1Type": CHORUS, "obrix_fx1Mix": 0.3, "obrix_fx1Param": 0.5,
        "obrix_fx2Type": REVERB, "obrix_fx2Mix": 0.25, "obrix_fx2Param": 0.45,
    }, (0.45, 0.6, 0.65, 0.4, 0.45, 0.05),
     "Two saws drift apart over 2 minutes. Berlin School ensemble from one engine.",
     ["lesson", "drift", "ensemble", "berlin-school"]),

    # 21. Journey Begin — infinite sustain + drift
    ("Journey Begin", {
        "obrix_src1Type": SAW, "obrix_src2Type": WT, "obrix_wtBank": 3,
        "obrix_src2PW": 0.7,
        "obrix_proc1Cutoff": 2000.0, "obrix_proc1Reso": 0.3,
        "obrix_proc2Type": LP, "obrix_proc2Cutoff": 3000.0,
        "obrix_mod1Depth": 0.3,
        "obrix_mod2Depth": 0.2, "obrix_mod2Rate": 0.15,
        "obrix_driftRate": 0.005, "obrix_driftDepth": 0.5,
        "obrix_journeyMode": 1.0,
        "obrix_fx1Type": DELAY, "obrix_fx1Mix": 0.25, "obrix_fx1Param": 0.5,
        "obrix_fx2Type": REVERB, "obrix_fx2Mix": 0.3, "obrix_fx2Param": 0.55,
        "obrix_distance": 0.25, "obrix_air": 0.4,
    }, (0.4, 0.6, 0.7, 0.45, 0.55, 0.05),
     "Journey mode: notes sustain forever. Drift + delay create evolving textures.",
     ["lesson", "journey", "generative", "infinite"]),

    # 22. The Full Reef — everything combined
    ("The Full Reef", {
        "obrix_src1Type": SAW, "obrix_src2Type": WT, "obrix_wtBank": 1,
        "obrix_src2PW": 0.65,
        "obrix_proc1Cutoff": 2500.0, "obrix_proc1Reso": 0.4,
        "obrix_proc1Feedback": 0.2,
        "obrix_proc2Type": HP, "obrix_proc2Cutoff": 800.0, "obrix_proc2Reso": 0.25,
        "obrix_proc3Type": FOLD,
        "obrix_mod1Depth": 0.6, "obrix_mod1Rate": 0.9,
        "obrix_mod2Depth": 0.25, "obrix_mod2Rate": 0.4,
        "obrix_mod3Depth": 0.7,
        "obrix_mod4Depth": 0.4,
        "obrix_fmDepth": 0.15,
        "obrix_driftRate": 0.007, "obrix_driftDepth": 0.3,
        "obrix_fx1Type": DELAY, "obrix_fx1Mix": 0.2, "obrix_fx1Param": 0.35,
        "obrix_fx2Type": CHORUS, "obrix_fx2Mix": 0.3, "obrix_fx2Param": 0.45,
        "obrix_fx3Type": REVERB, "obrix_fx3Mix": 0.25, "obrix_fx3Param": 0.5,
        "obrix_macroCharacter": 0.2, "obrix_macroMovement": 0.15,
        "obrix_macroSpace": 0.1,
        "obrix_distance": 0.15, "obrix_air": 0.45,
    }, (0.55, 0.55, 0.6, 0.6, 0.5, 0.3),
     "Every brick type in play. The full reef, alive and breathing.",
     ["lesson", "full", "reef", "showcase", "complete"]),
]

# ── 8 Foundation basics (beyond Lesson) ──────────────────────────────────────

FOUNDATION_BASICS = [
    ("Clean Saw", {
        "obrix_src1Type": SAW, "obrix_proc1Cutoff": 12000.0,
        "obrix_mod1Depth": 0.2, "obrix_mod3Depth": 0.5,
    }, (0.7, 0.4, 0.1, 0.15, 0.0, 0.1),
     "Bright saw through open filter. The workhorse.", ["saw", "clean", "basic"]),

    ("Warm Pad", {
        "obrix_src1Type": SAW, "obrix_src2Type": SAW, "obrix_src2Tune": -0.08,
        "obrix_ampAttack": 0.8, "obrix_ampRelease": 1.5,
        "obrix_proc1Cutoff": 2000.0, "obrix_proc1Reso": 0.2,
        "obrix_mod1Depth": 0.3, "obrix_mod2Depth": 0.15, "obrix_mod2Rate": 0.2,
    }, (0.35, 0.7, 0.35, 0.3, 0.15, 0.0),
     "Two slightly detuned saws through warm filter. Classic pad.", ["pad", "warm", "basic"]),

    ("Pluck Init", {
        "obrix_src1Type": SAW,
        "obrix_ampAttack": 0.001, "obrix_ampDecay": 0.4, "obrix_ampSustain": 0.0,
        "obrix_proc1Cutoff": 5000.0, "obrix_proc1Reso": 0.3,
        "obrix_mod1Depth": 0.7, "obrix_mod1Rate": 0.6,
    }, (0.55, 0.45, 0.15, 0.2, 0.0, 0.2),
     "Quick attack, fast decay. A basic pluck sound.", ["pluck", "short", "basic"]),

    ("Sub Bass", {
        "obrix_src1Type": SINE, "obrix_polyphony": MONO,
        "obrix_proc1Cutoff": 500.0,
        "obrix_mod1Depth": 0.3, "obrix_mod1Rate": 0.5,
        "obrix_ampSustain": 0.9, "obrix_glideTime": 0.08,
    }, (0.15, 0.8, 0.1, 0.1, 0.0, 0.1),
     "Pure sine sub with mono glide. Foundation of any mix.", ["bass", "sub", "mono"]),

    ("Square Lead", {
        "obrix_src1Type": SQUARE, "obrix_polyphony": LEGATO,
        "obrix_proc1Cutoff": 4000.0, "obrix_proc1Reso": 0.3,
        "obrix_mod1Depth": 0.5, "obrix_glideTime": 0.05,
        "obrix_mod3Depth": 0.6,
    }, (0.5, 0.5, 0.2, 0.2, 0.0, 0.2),
     "Square wave lead with legato glide.", ["lead", "square", "legato"]),

    ("Triangle Keys", {
        "obrix_src1Type": TRI,
        "obrix_ampAttack": 0.005, "obrix_ampDecay": 0.6, "obrix_ampSustain": 0.4,
        "obrix_proc1Cutoff": 6000.0,
        "obrix_mod1Depth": 0.3,
    }, (0.4, 0.6, 0.1, 0.15, 0.0, 0.0),
     "Soft triangle wave keys. Gentle and musical.", ["keys", "triangle", "soft"]),

    ("Noise Hiss", {
        "obrix_src1Type": NOISE,
        "obrix_proc1Cutoff": 3000.0, "obrix_proc1Reso": 0.5,
        "obrix_ampAttack": 0.1, "obrix_ampRelease": 0.8,
        "obrix_mod1Depth": 0.6, "obrix_mod1Rate": 1.5,
    }, (0.5, 0.3, 0.3, 0.4, 0.1, 0.15),
     "Filtered noise for risers and textures.", ["noise", "texture", "riser"]),

    ("Dual Sine", {
        "obrix_src2Type": SINE, "obrix_src2Tune": 12.0,
        "obrix_proc1Type": P_OFF,
    }, (0.35, 0.55, 0.05, 0.2, 0.0, 0.0),
     "Two sines an octave apart. Pure harmonic simplicity.", ["sine", "octave", "pure"]),
]

# ══════════════════════════════════════════════════════════════════════════════
# 4C. PLACE PRESETS — 20 environment-named presets (Atmosphere mood)
# ══════════════════════════════════════════════════════════════════════════════

PLACE_PRESETS = [
    ("Greenhouse Dusk", {
        "obrix_src1Type": WT, "obrix_wtBank": 3, "obrix_src1PW": 0.6,
        "obrix_ampAttack": 1.2, "obrix_ampRelease": 2.0,
        "obrix_proc1Cutoff": 1800.0, "obrix_proc1Reso": 0.2,
        "obrix_mod1Depth": 0.3, "obrix_mod2Depth": 0.15, "obrix_mod2Rate": 0.1,
        "obrix_driftRate": 0.003, "obrix_driftDepth": 0.35,
        "obrix_distance": 0.3, "obrix_air": 0.35,
        "obrix_fx1Type": REVERB, "obrix_fx1Mix": 0.4, "obrix_fx1Param": 0.55,
    }, (0.3, 0.7, 0.4, 0.3, 0.65, 0.0),
     "Warm organic tones drifting in a humid glass room.", ["place", "greenhouse", "warm", "ambient"]),

    ("Concrete Stairwell", {
        "obrix_src1Type": SQUARE,
        "obrix_ampAttack": 0.002, "obrix_ampDecay": 0.8, "obrix_ampSustain": 0.1,
        "obrix_proc1Cutoff": 2500.0, "obrix_proc1Reso": 0.45,
        "obrix_proc1Feedback": 0.3,
        "obrix_mod1Depth": 0.6,
        "obrix_fx1Type": REVERB, "obrix_fx1Mix": 0.5, "obrix_fx1Param": 0.7,
        "obrix_fx2Type": DELAY, "obrix_fx2Mix": 0.2, "obrix_fx2Param": 0.3,
        "obrix_distance": 0.45, "obrix_air": 0.6,
    }, (0.45, 0.4, 0.25, 0.35, 0.7, 0.15),
     "Hard reflections in cold concrete. Each note echoes upward.", ["place", "stairwell", "reverb", "cold"]),

    ("Tide Pool Morning", {
        "obrix_src1Type": SINE, "obrix_src2Type": WT, "obrix_wtBank": 3,
        "obrix_src2PW": 0.55,
        "obrix_ampAttack": 0.6, "obrix_ampRelease": 1.5,
        "obrix_proc1Cutoff": 3000.0,
        "obrix_mod2Depth": 0.2, "obrix_mod2Rate": 0.15,
        "obrix_driftRate": 0.004, "obrix_driftDepth": 0.3,
        "obrix_fx1Type": CHORUS, "obrix_fx1Mix": 0.35, "obrix_fx1Param": 0.4,
        "obrix_fx2Type": REVERB, "obrix_fx2Mix": 0.3, "obrix_fx2Param": 0.45,
        "obrix_air": 0.4,
    }, (0.4, 0.65, 0.45, 0.3, 0.5, 0.0),
     "Gentle ripples of sound in early light. Organic beating partials.", ["place", "ocean", "morning", "gentle"]),

    ("Reef at Noon", {
        "obrix_src1Type": SAW, "obrix_src2Type": WT, "obrix_wtBank": 2,
        "obrix_src2PW": 0.7,
        "obrix_proc1Cutoff": 5000.0, "obrix_proc1Reso": 0.3,
        "obrix_proc2Type": BP, "obrix_proc2Cutoff": 2000.0, "obrix_proc2Reso": 0.4,
        "obrix_mod1Depth": 0.5, "obrix_mod2Depth": 0.2, "obrix_mod2Rate": 0.25,
        "obrix_driftRate": 0.006, "obrix_driftDepth": 0.25,
        "obrix_fx1Type": CHORUS, "obrix_fx1Mix": 0.25,
        "obrix_fx2Type": REVERB, "obrix_fx2Mix": 0.2,
    }, (0.55, 0.5, 0.5, 0.45, 0.35, 0.1),
     "Bright, alive, full of harmonic activity. The reef at peak light.", ["place", "reef", "bright", "active"]),

    ("Storm Drain Echo", {
        "obrix_src1Type": NOISE, "obrix_src2Type": SINE,
        "obrix_proc1Type": BP, "obrix_proc1Cutoff": 800.0, "obrix_proc1Reso": 0.6,
        "obrix_proc1Feedback": 0.4,
        "obrix_ampAttack": 0.05, "obrix_ampRelease": 1.0,
        "obrix_fmDepth": 0.2,
        "obrix_fx1Type": DELAY, "obrix_fx1Mix": 0.45, "obrix_fx1Param": 0.6,
        "obrix_fx2Type": REVERB, "obrix_fx2Mix": 0.35, "obrix_fx2Param": 0.65,
        "obrix_distance": 0.55, "obrix_air": 0.55,
    }, (0.35, 0.35, 0.35, 0.4, 0.7, 0.2),
     "Metallic resonance in a dark tunnel. Feedback and delay build atmosphere.", ["place", "dark", "industrial", "echo"]),

    ("Frozen Lake", {
        "obrix_src1Type": TRI, "obrix_src2Type": SINE, "obrix_src2Tune": 12.0,
        "obrix_ampAttack": 0.3, "obrix_ampRelease": 3.0,
        "obrix_proc1Cutoff": 1500.0, "obrix_proc1Reso": 0.15,
        "obrix_driftRate": 0.002, "obrix_driftDepth": 0.4,
        "obrix_fx1Type": REVERB, "obrix_fx1Mix": 0.5, "obrix_fx1Param": 0.7,
        "obrix_distance": 0.4, "obrix_air": 0.7,
    }, (0.3, 0.35, 0.3, 0.25, 0.7, 0.0),
     "Crystalline tones over vast frozen expanse. Cold air and long decay.", ["place", "frozen", "crystal", "cold"]),

    ("Cathedral Nave", {
        "obrix_src1Type": SINE, "obrix_src2Type": SINE, "obrix_src2Tune": 7.0,
        "obrix_ampAttack": 0.5, "obrix_ampRelease": 4.0, "obrix_ampSustain": 0.8,
        "obrix_proc1Type": P_OFF,
        "obrix_fx1Type": REVERB, "obrix_fx1Mix": 0.55, "obrix_fx1Param": 0.8,
        "obrix_distance": 0.5, "obrix_air": 0.45,
    }, (0.25, 0.6, 0.15, 0.2, 0.85, 0.0),
     "Pure fifths swelling in stone. Immense reverb, distant placement.", ["place", "cathedral", "sacred", "reverb"]),

    ("Bamboo Forest", {
        "obrix_src1Type": TRI, "obrix_src2Type": NOISE,
        "obrix_srcMix": 0.7,
        "obrix_ampAttack": 0.01, "obrix_ampDecay": 0.5, "obrix_ampSustain": 0.15,
        "obrix_proc1Cutoff": 4000.0, "obrix_proc1Reso": 0.35,
        "obrix_proc2Type": HP, "obrix_proc2Cutoff": 2000.0,
        "obrix_mod1Depth": 0.5,
        "obrix_fx1Type": DELAY, "obrix_fx1Mix": 0.2, "obrix_fx1Param": 0.25,
        "obrix_fx2Type": REVERB, "obrix_fx2Mix": 0.3, "obrix_fx2Param": 0.4,
        "obrix_air": 0.55,
    }, (0.5, 0.5, 0.3, 0.3, 0.45, 0.05),
     "Hollow knocking tones with wind noise. Each note clicks like bamboo.", ["place", "bamboo", "percussive", "nature"]),

    ("Subway Platform", {
        "obrix_src1Type": SAW, "obrix_src2Type": SQUARE,
        "obrix_proc1Cutoff": 1800.0, "obrix_proc1Reso": 0.5,
        "obrix_proc1Feedback": 0.35,
        "obrix_proc2Type": HP, "obrix_proc2Cutoff": 400.0,
        "obrix_mod1Depth": 0.5, "obrix_mod2Depth": 0.1, "obrix_mod2Rate": 0.08,
        "obrix_driftRate": 0.003, "obrix_driftDepth": 0.2,
        "obrix_fx1Type": DELAY, "obrix_fx1Mix": 0.3, "obrix_fx1Param": 0.45,
        "obrix_fx2Type": REVERB, "obrix_fx2Mix": 0.25, "obrix_fx2Param": 0.5,
        "obrix_distance": 0.35,
    }, (0.4, 0.45, 0.35, 0.45, 0.5, 0.2),
     "Underground rumble with distant echoes. Something approaching.", ["place", "subway", "urban", "tension"]),

    ("Mountain Pass", {
        "obrix_src1Type": WT, "obrix_wtBank": 0, "obrix_src1PW": 0.8,
        "obrix_ampAttack": 0.8, "obrix_ampRelease": 2.5,
        "obrix_proc1Cutoff": 2500.0, "obrix_proc1Reso": 0.15,
        "obrix_mod2Depth": 0.15, "obrix_mod2Rate": 0.08,
        "obrix_driftRate": 0.004, "obrix_driftDepth": 0.5,
        "obrix_fx1Type": REVERB, "obrix_fx1Mix": 0.45, "obrix_fx1Param": 0.6,
        "obrix_distance": 0.6, "obrix_air": 0.65,
    }, (0.35, 0.4, 0.4, 0.3, 0.7, 0.0),
     "Wide open space above the treeline. Cold air, slow drift, distant harmonics.", ["place", "mountain", "open", "vast"]),

    ("Engine Room", {
        "obrix_src1Type": SAW, "obrix_src2Type": PULSE, "obrix_src2PW": 0.25,
        "obrix_src2Tune": -12.0,
        "obrix_proc1Cutoff": 1200.0, "obrix_proc1Reso": 0.55,
        "obrix_proc1Feedback": 0.45,
        "obrix_proc2Type": LP, "obrix_proc2Cutoff": 800.0, "obrix_proc2Reso": 0.4,
        "obrix_mod1Depth": 0.3, "obrix_mod2Depth": 0.2, "obrix_mod2Rate": 0.5,
        "obrix_driftRate": 0.01, "obrix_driftDepth": 0.15,
        "obrix_air": 0.3,
    }, (0.35, 0.55, 0.45, 0.55, 0.2, 0.4),
     "Low throb of machinery. Filter feedback and pulse width create industrial warmth.", ["place", "industrial", "engine", "throb"]),

    ("Coral Garden", {
        "obrix_src1Type": WT, "obrix_wtBank": 3, "obrix_src1PW": 0.65,
        "obrix_src2Type": WT, "obrix_wtBank": 1, "obrix_src2PW": 0.55,
        "obrix_src2Tune": 5.0,
        "obrix_proc1Cutoff": 3500.0, "obrix_proc2Type": LP, "obrix_proc2Cutoff": 4000.0,
        "obrix_ampAttack": 0.4, "obrix_ampRelease": 1.8,
        "obrix_mod1Depth": 0.3, "obrix_mod2Depth": 0.2, "obrix_mod2Rate": 0.15,
        "obrix_driftRate": 0.005, "obrix_driftDepth": 0.4,
        "obrix_fx1Type": CHORUS, "obrix_fx1Mix": 0.3,
        "obrix_fx2Type": REVERB, "obrix_fx2Mix": 0.35, "obrix_fx2Param": 0.5,
        "obrix_distance": 0.2, "obrix_air": 0.4,
    }, (0.45, 0.65, 0.5, 0.45, 0.5, 0.0),
     "Organic and vocal wavetables a fourth apart. The reef in its element.", ["place", "coral", "underwater", "reef"]),

    ("Rooftop Dawn", {
        "obrix_src1Type": SINE, "obrix_src2Type": TRI, "obrix_src2Tune": 12.0,
        "obrix_ampAttack": 1.0, "obrix_ampRelease": 2.0,
        "obrix_proc1Type": P_OFF,
        "obrix_mod2Depth": 0.1, "obrix_mod2Rate": 0.05,
        "obrix_driftRate": 0.002, "obrix_driftDepth": 0.25,
        "obrix_fx1Type": REVERB, "obrix_fx1Mix": 0.4, "obrix_fx1Param": 0.5,
        "obrix_distance": 0.2, "obrix_air": 0.55,
    }, (0.35, 0.55, 0.2, 0.2, 0.55, 0.0),
     "First light over the city. Sine and triangle, octave apart, barely moving.", ["place", "dawn", "gentle", "urban"]),

    ("Cave Drip", {
        "obrix_src1Type": SINE,
        "obrix_ampAttack": 0.001, "obrix_ampDecay": 1.2, "obrix_ampSustain": 0.0,
        "obrix_proc1Cutoff": 6000.0, "obrix_proc1Reso": 0.5,
        "obrix_mod1Depth": 0.8, "obrix_mod1Rate": 0.4,
        "obrix_fx1Type": DELAY, "obrix_fx1Mix": 0.35, "obrix_fx1Param": 0.55,
        "obrix_fx2Type": REVERB, "obrix_fx2Mix": 0.5, "obrix_fx2Param": 0.75,
        "obrix_distance": 0.6, "obrix_air": 0.35,
    }, (0.5, 0.45, 0.2, 0.2, 0.8, 0.0),
     "Single drops resonating in darkness. Long reverb, distant echoes.", ["place", "cave", "drip", "minimal"]),

    ("Desert Night", {
        "obrix_src1Type": WT, "obrix_wtBank": 2, "obrix_src1PW": 0.75,
        "obrix_ampAttack": 0.5, "obrix_ampRelease": 2.5,
        "obrix_proc1Cutoff": 2000.0, "obrix_proc1Reso": 0.2,
        "obrix_driftRate": 0.003, "obrix_driftDepth": 0.45,
        "obrix_fx1Type": REVERB, "obrix_fx1Mix": 0.35, "obrix_fx1Param": 0.55,
        "obrix_distance": 0.5, "obrix_air": 0.7,
    }, (0.35, 0.35, 0.35, 0.25, 0.6, 0.05),
     "Metallic wavetable under cold stars. Vast space, slow drift.", ["place", "desert", "night", "cold"]),

    ("Harbor Fog", {
        "obrix_src1Type": SAW, "obrix_src2Type": LOFI,
        "obrix_src2Tune": -0.05,
        "obrix_ampAttack": 0.6, "obrix_ampRelease": 1.5,
        "obrix_proc1Cutoff": 1200.0, "obrix_proc1Reso": 0.15,
        "obrix_proc2Type": LP, "obrix_proc2Cutoff": 1500.0,
        "obrix_mod2Depth": 0.1, "obrix_mod2Rate": 0.06,
        "obrix_driftRate": 0.004, "obrix_driftDepth": 0.35,
        "obrix_fx1Type": CHORUS, "obrix_fx1Mix": 0.3,
        "obrix_fx2Type": REVERB, "obrix_fx2Mix": 0.4, "obrix_fx2Param": 0.55,
        "obrix_distance": 0.4, "obrix_air": 0.35,
    }, (0.25, 0.7, 0.35, 0.35, 0.6, 0.0),
     "Saw and lo-fi saw detuned in thick warm air. Foghorns in the mist.", ["place", "harbor", "fog", "warm"]),

    ("Library Silence", {
        "obrix_src1Type": TRI,
        "obrix_ampAttack": 0.3, "obrix_ampDecay": 1.0, "obrix_ampSustain": 0.2,
        "obrix_ampRelease": 1.5,
        "obrix_proc1Cutoff": 2500.0,
        "obrix_mod1Depth": 0.2,
        "obrix_fx1Type": REVERB, "obrix_fx1Mix": 0.2, "obrix_fx1Param": 0.3,
        "obrix_distance": 0.15, "obrix_air": 0.5, "obrix_level": 0.6,
    }, (0.35, 0.55, 0.1, 0.1, 0.3, 0.0),
     "Hushed triangle tones. A place where sound is precious.", ["place", "quiet", "intimate", "soft"]),

    ("Sunken Ship", {
        "obrix_src1Type": SAW, "obrix_src2Type": WT, "obrix_wtBank": 2,
        "obrix_src2PW": 0.75, "obrix_src2Tune": -12.0,
        "obrix_ampAttack": 0.8, "obrix_ampRelease": 2.0,
        "obrix_proc1Cutoff": 1200.0, "obrix_proc1Reso": 0.3,
        "obrix_proc2Type": LP, "obrix_proc2Cutoff": 800.0,
        "obrix_driftRate": 0.004, "obrix_driftDepth": 0.35,
        "obrix_fx1Type": REVERB, "obrix_fx1Mix": 0.4, "obrix_fx1Param": 0.6,
        "obrix_distance": 0.5, "obrix_air": 0.3,
    }, (0.25, 0.6, 0.35, 0.35, 0.6, 0.1),
     "Dark metallic hull tones buried in warm water. History resting.", ["place", "shipwreck", "dark", "underwater"]),

    ("Canopy Rain", {
        "obrix_src1Type": NOISE, "obrix_src2Type": TRI,
        "obrix_srcMix": 0.3,
        "obrix_proc1Type": LP, "obrix_proc1Cutoff": 4000.0,
        "obrix_proc2Type": P_OFF,
        "obrix_ampAttack": 0.2, "obrix_ampRelease": 1.0,
        "obrix_mod2Depth": 0.15, "obrix_mod2Rate": 0.1,
        "obrix_driftRate": 0.006, "obrix_driftDepth": 0.2,
        "obrix_fx1Type": DELAY, "obrix_fx1Mix": 0.2, "obrix_fx1Param": 0.25,
        "obrix_fx2Type": REVERB, "obrix_fx2Mix": 0.35, "obrix_fx2Param": 0.45,
        "obrix_air": 0.4,
    }, (0.4, 0.6, 0.35, 0.3, 0.5, 0.0),
     "Rain through leaves. Noise droplets over gentle triangle tones.", ["place", "rain", "forest", "gentle"]),

    ("Volcanic Vent", {
        "obrix_src1Type": NOISE, "obrix_src2Type": SAW, "obrix_src2Tune": -24.0,
        "obrix_srcMix": 0.6,
        "obrix_proc1Type": BP, "obrix_proc1Cutoff": 600.0, "obrix_proc1Reso": 0.7,
        "obrix_proc1Feedback": 0.5,
        "obrix_proc2Type": LP, "obrix_proc2Cutoff": 400.0,
        "obrix_mod1Depth": 0.4, "obrix_mod2Depth": 0.3, "obrix_mod2Rate": 0.15,
        "obrix_air": 0.25, "obrix_distance": 0.3,
    }, (0.3, 0.6, 0.45, 0.5, 0.3, 0.5),
     "Deep rumble through resonant filters. Pressurized heat.", ["place", "volcanic", "deep", "pressure"]),
]

# ── 5 Atmosphere ambient extras ──────────────────────────────────────────────

ATMO_EXTRAS = [
    ("Slow Bloom", {
        "obrix_src1Type": WT, "obrix_wtBank": 3, "obrix_src1PW": 0.6,
        "obrix_ampAttack": 3.0, "obrix_ampRelease": 3.0,
        "obrix_proc1Cutoff": 1500.0, "obrix_mod1Depth": 0.2,
        "obrix_mod2Depth": 0.15, "obrix_mod2Rate": 0.05,
        "obrix_driftRate": 0.003, "obrix_driftDepth": 0.5,
        "obrix_fx1Type": REVERB, "obrix_fx1Mix": 0.45, "obrix_fx1Param": 0.6,
        "obrix_distance": 0.35,
    }, (0.3, 0.65, 0.4, 0.25, 0.65, 0.0),
     "Ultra-slow attack. The sound unfolds over seconds.", ["ambient", "slow", "bloom"]),

    ("Glass Breath", {
        "obrix_src1Type": SINE, "obrix_src2Type": WT, "obrix_wtBank": 2,
        "obrix_src2PW": 0.8,
        "obrix_ampAttack": 0.8, "obrix_ampRelease": 2.0,
        "obrix_proc1Type": P_OFF,
        "obrix_mod2Target": T_WTPOS, "obrix_mod2Depth": 0.3, "obrix_mod2Rate": 0.1,
        "obrix_driftRate": 0.004, "obrix_driftDepth": 0.3,
        "obrix_fx1Type": CHORUS, "obrix_fx1Mix": 0.35,
        "obrix_fx2Type": REVERB, "obrix_fx2Mix": 0.3,
        "obrix_air": 0.6,
    }, (0.4, 0.45, 0.35, 0.3, 0.55, 0.0),
     "Metallic wavetable morphing via LFO. Crystalline and alive.", ["ambient", "glass", "metallic"]),

    ("Deep Current", {
        "obrix_src1Type": SAW, "obrix_src2Type": SAW, "obrix_src2Tune": -12.0,
        "obrix_ampAttack": 1.5, "obrix_ampRelease": 2.5,
        "obrix_proc1Cutoff": 800.0, "obrix_proc1Reso": 0.25,
        "obrix_proc2Type": LP, "obrix_proc2Cutoff": 600.0,
        "obrix_mod2Depth": 0.2, "obrix_mod2Rate": 0.08,
        "obrix_driftRate": 0.005, "obrix_driftDepth": 0.4,
        "obrix_journeyMode": 1.0,
        "obrix_fx1Type": REVERB, "obrix_fx1Mix": 0.35,
        "obrix_distance": 0.45, "obrix_air": 0.3,
    }, (0.2, 0.7, 0.45, 0.35, 0.55, 0.05),
     "Journey mode: deep saws sustain and drift. An ocean floor current.", ["ambient", "deep", "journey", "drone"]),

    ("Shimmer Veil", {
        "obrix_src1Type": WT, "obrix_wtBank": 1, "obrix_src1PW": 0.7,
        "obrix_src2Type": SINE, "obrix_src2Tune": 19.0,
        "obrix_srcMix": 0.7,
        "obrix_ampAttack": 0.6, "obrix_ampRelease": 2.0,
        "obrix_proc1Cutoff": 5000.0,
        "obrix_driftRate": 0.006, "obrix_driftDepth": 0.3,
        "obrix_fx1Type": CHORUS, "obrix_fx1Mix": 0.4,
        "obrix_fx2Type": REVERB, "obrix_fx2Mix": 0.4, "obrix_fx2Param": 0.55,
    }, (0.55, 0.5, 0.45, 0.35, 0.6, 0.0),
     "Vocal wavetable with high sine shimmer. A veil of harmonics.", ["ambient", "shimmer", "vocal"]),

    ("Thermal Layer", {
        "obrix_src1Type": SINE, "obrix_src2Type": SINE,
        "obrix_src2Tune": 0.03,
        "obrix_ampAttack": 2.0, "obrix_ampSustain": 0.9, "obrix_ampRelease": 3.0,
        "obrix_proc1Type": P_OFF,
        "obrix_driftRate": 0.002, "obrix_driftDepth": 0.6,
        "obrix_journeyMode": 1.0,
        "obrix_fx1Type": REVERB, "obrix_fx1Mix": 0.3,
        "obrix_distance": 0.25,
    }, (0.2, 0.7, 0.35, 0.15, 0.5, 0.0),
     "Two sines 3 cents apart. Journey mode. Beating that never resolves.", ["ambient", "beating", "thermal", "drone"]),
]

# ══════════════════════════════════════════════════════════════════════════════
# 4B. GENRE PRESETS
# ══════════════════════════════════════════════════════════════════════════════

# --- TECHNO (12) → Prism ---
TECHNO = [
    ("Acid Bass", {"obrix_src1Type": SAW, "obrix_polyphony": MONO,
     "obrix_proc1Cutoff": 600.0, "obrix_proc1Reso": 0.7, "obrix_proc1Feedback": 0.3,
     "obrix_mod1Depth": 0.8, "obrix_mod1Rate": 0.6, "obrix_glideTime": 0.06,
     "obrix_ampDecay": 0.5, "obrix_ampSustain": 0.4},
     (0.45, 0.5, 0.4, 0.25, 0.0, 0.5), "Squelchy acid line with glide and feedback.", ["techno", "acid", "bass", "303"]),

    ("Modular Stab", {"obrix_src1Type": SAW, "obrix_src2Type": SQUARE,
     "obrix_ampAttack": 0.001, "obrix_ampDecay": 0.15, "obrix_ampSustain": 0.0,
     "obrix_proc1Cutoff": 4000.0, "obrix_proc1Reso": 0.4,
     "obrix_proc2Type": HP, "obrix_proc2Cutoff": 500.0,
     "obrix_mod1Depth": 0.7, "obrix_mod1Rate": 0.3},
     (0.6, 0.4, 0.15, 0.4, 0.0, 0.5), "Short bright stab. Two sources, split filtering.", ["techno", "stab", "short"]),

    ("Metallic Perc", {"obrix_src1Type": WT, "obrix_wtBank": 2, "obrix_src2Type": NOISE,
     "obrix_srcMix": 0.7,
     "obrix_ampAttack": 0.001, "obrix_ampDecay": 0.25, "obrix_ampSustain": 0.0,
     "obrix_proc1Cutoff": 3000.0, "obrix_proc1Reso": 0.5, "obrix_fmDepth": 0.2},
     (0.55, 0.3, 0.1, 0.35, 0.0, 0.35), "Metallic percussive hit with noise transient.", ["techno", "perc", "metallic"]),

    ("Filtered Drone", {"obrix_src1Type": SAW, "obrix_src2Type": PULSE, "obrix_src2PW": 0.3,
     "obrix_ampAttack": 0.5, "obrix_ampSustain": 0.9,
     "obrix_proc1Cutoff": 1000.0, "obrix_proc1Reso": 0.45, "obrix_proc1Feedback": 0.3,
     "obrix_mod2Depth": 0.25, "obrix_mod2Rate": 0.15,
     "obrix_driftRate": 0.008, "obrix_driftDepth": 0.2},
     (0.35, 0.55, 0.4, 0.4, 0.15, 0.25), "Low filtered drone with slow movement.", ["techno", "drone", "dark"]),

    ("Berlin Kick Layer", {"obrix_src1Type": SINE, "obrix_polyphony": MONO,
     "obrix_ampAttack": 0.001, "obrix_ampDecay": 0.35, "obrix_ampSustain": 0.0,
     "obrix_proc1Cutoff": 500.0,
     "obrix_mod1Target": T_PITCH, "obrix_mod1Depth": 0.9, "obrix_mod1Rate": 0.2},
     (0.3, 0.6, 0.1, 0.2, 0.0, 0.5), "Sine with pitch envelope. Layer under your kick.", ["techno", "kick", "sub", "layer"]),

    ("Dark Chord", {"obrix_src1Type": SAW, "obrix_src2Type": SAW, "obrix_src2Tune": -0.06,
     "obrix_proc1Cutoff": 1800.0, "obrix_proc1Reso": 0.3,
     "obrix_mod1Depth": 0.4, "obrix_mod2Depth": 0.15, "obrix_mod2Rate": 0.3,
     "obrix_fx1Type": REVERB, "obrix_fx1Mix": 0.15},
     (0.35, 0.55, 0.35, 0.35, 0.2, 0.2), "Dark detuned chord pad for warehouse sets.", ["techno", "pad", "dark", "chord"]),

    ("Rave Siren", {"obrix_src1Type": SAW, "obrix_polyphony": MONO,
     "obrix_proc1Cutoff": 8000.0, "obrix_proc1Reso": 0.5,
     "obrix_mod2Target": T_PITCH, "obrix_mod2Depth": 0.8, "obrix_mod2Rate": 3.0,
     "obrix_glideTime": 0.15, "obrix_unisonDetune": 15.0},
     (0.7, 0.4, 0.8, 0.3, 0.1, 0.6), "Fast pitch LFO siren with unison spread.", ["techno", "siren", "rave", "lead"]),

    ("Rumble Sub", {"obrix_src1Type": SINE, "obrix_src2Type": SINE, "obrix_src2Tune": -0.02,
     "obrix_polyphony": MONO, "obrix_proc1Cutoff": 200.0,
     "obrix_ampSustain": 0.9, "obrix_driftRate": 0.01, "obrix_driftDepth": 0.15},
     (0.1, 0.8, 0.2, 0.15, 0.0, 0.15), "Two sines beating at sub-bass. Visceral rumble.", ["techno", "sub", "rumble"]),

    ("Tensile Stab", {"obrix_src1Type": SQUARE, "obrix_src2Type": SAW,
     "obrix_ampAttack": 0.001, "obrix_ampDecay": 0.08, "obrix_ampSustain": 0.0,
     "obrix_proc1Cutoff": 6000.0, "obrix_proc1Reso": 0.35,
     "obrix_proc2Type": HP, "obrix_proc2Cutoff": 1000.0,
     "obrix_proc3Type": FOLD, "obrix_mod1Depth": 0.6},
     (0.6, 0.35, 0.1, 0.45, 0.0, 0.55), "Wavefolder post-mix stab. Aggressive and precise.", ["techno", "stab", "wavefolder"]),

    ("Hypnotic Arp", {"obrix_src1Type": PULSE, "obrix_src1PW": 0.35,
     "obrix_ampAttack": 0.001, "obrix_ampDecay": 0.2, "obrix_ampSustain": 0.0,
     "obrix_proc1Cutoff": 3000.0, "obrix_proc1Reso": 0.4,
     "obrix_mod1Depth": 0.6, "obrix_mod1Rate": 0.5,
     "obrix_fx1Type": DELAY, "obrix_fx1Mix": 0.3, "obrix_fx1Param": 0.4},
     (0.5, 0.45, 0.4, 0.25, 0.2, 0.25), "Pulse pluck with delay. Feed it arpeggios.", ["techno", "arp", "pluck", "delay"]),

    ("Industrial Clang", {"obrix_src1Type": WT, "obrix_wtBank": 2, "obrix_src2Type": NOISE,
     "obrix_srcMix": 0.6, "obrix_fmDepth": 0.4,
     "obrix_ampAttack": 0.001, "obrix_ampDecay": 0.4, "obrix_ampSustain": 0.0,
     "obrix_proc1Type": BP, "obrix_proc1Cutoff": 1500.0, "obrix_proc1Reso": 0.6},
     (0.5, 0.3, 0.1, 0.4, 0.05, 0.55), "Inharmonic metallic FM with noise attack.", ["techno", "industrial", "metallic", "clang"]),

    ("Warehouse Pad", {"obrix_src1Type": SAW, "obrix_src2Type": WT, "obrix_wtBank": 0,
     "obrix_src2PW": 0.7,
     "obrix_ampAttack": 0.8, "obrix_ampRelease": 1.5,
     "obrix_proc1Cutoff": 1500.0, "obrix_proc1Reso": 0.25,
     "obrix_proc2Type": LP, "obrix_proc2Cutoff": 2000.0,
     "obrix_driftRate": 0.006, "obrix_driftDepth": 0.25,
     "obrix_fx1Type": REVERB, "obrix_fx1Mix": 0.25},
     (0.35, 0.55, 0.4, 0.4, 0.3, 0.15), "Brooding pad for dark techno. Drift creates life.", ["techno", "pad", "warehouse", "dark"]),
]

# --- SYNTHWAVE (12) → Prism ---
SYNTHWAVE = [
    ("Saw Lead", {"obrix_src1Type": SAW, "obrix_polyphony": LEGATO,
     "obrix_proc1Cutoff": 5000.0, "obrix_proc1Reso": 0.2,
     "obrix_mod1Depth": 0.4, "obrix_glideTime": 0.04,
     "obrix_fx1Type": CHORUS, "obrix_fx1Mix": 0.3, "obrix_fx1Param": 0.4},
     (0.6, 0.5, 0.35, 0.25, 0.3, 0.2), "Classic bright saw lead with chorus. 1984.", ["synthwave", "lead", "saw", "80s"]),

    ("Analog Brass", {"obrix_src1Type": SAW, "obrix_src2Type": SAW, "obrix_src2Tune": -0.04,
     "obrix_ampAttack": 0.08, "obrix_proc1Cutoff": 3000.0, "obrix_proc1Reso": 0.3,
     "obrix_mod1Depth": 0.5, "obrix_fx1Type": CHORUS, "obrix_fx1Mix": 0.25},
     (0.55, 0.55, 0.3, 0.35, 0.2, 0.2), "Detuned saws with slow attack. Brass section.", ["synthwave", "brass", "analog"]),

    ("Arp Pulse", {"obrix_src1Type": PULSE, "obrix_src1PW": 0.4,
     "obrix_ampAttack": 0.001, "obrix_ampDecay": 0.25, "obrix_ampSustain": 0.1,
     "obrix_proc1Cutoff": 4000.0, "obrix_proc1Reso": 0.35,
     "obrix_mod1Depth": 0.6, "obrix_fx1Type": DELAY, "obrix_fx1Mix": 0.25, "obrix_fx1Param": 0.35},
     (0.5, 0.5, 0.35, 0.2, 0.2, 0.15), "Pulse arp with delay. Retro sequencer vibes.", ["synthwave", "arp", "pulse"]),

    ("Dark Pad", {"obrix_src1Type": SAW, "obrix_src2Type": SQUARE,
     "obrix_ampAttack": 1.0, "obrix_ampRelease": 2.0,
     "obrix_proc1Cutoff": 1200.0, "obrix_proc1Reso": 0.2,
     "obrix_proc2Type": LP, "obrix_proc2Cutoff": 1500.0,
     "obrix_mod2Depth": 0.15, "obrix_mod2Rate": 0.2,
     "obrix_fx1Type": CHORUS, "obrix_fx1Mix": 0.3,
     "obrix_fx2Type": REVERB, "obrix_fx2Mix": 0.3},
     (0.3, 0.6, 0.35, 0.4, 0.45, 0.05), "Lush dark pad. Saw + square through warm filters.", ["synthwave", "pad", "dark", "lush"]),

    ("Neon Lead", {"obrix_src1Type": SAW, "obrix_unisonDetune": 12.0, "obrix_polyphony": MONO,
     "obrix_proc1Cutoff": 6000.0, "obrix_proc1Reso": 0.25,
     "obrix_mod1Depth": 0.3, "obrix_glideTime": 0.03,
     "obrix_fx1Type": CHORUS, "obrix_fx1Mix": 0.35,
     "obrix_fx2Type": DELAY, "obrix_fx2Mix": 0.2, "obrix_fx2Param": 0.35},
     (0.65, 0.45, 0.4, 0.5, 0.3, 0.25), "Unison supersaw lead. Neon glow.", ["synthwave", "lead", "supersaw", "neon"]),

    ("Bass Pulse", {"obrix_src1Type": PULSE, "obrix_src1PW": 0.25, "obrix_polyphony": MONO,
     "obrix_proc1Cutoff": 1500.0, "obrix_proc1Reso": 0.35,
     "obrix_mod1Depth": 0.5, "obrix_glideTime": 0.05},
     (0.4, 0.55, 0.2, 0.2, 0.0, 0.25), "Narrow pulse bass with glide. Retro foundation.", ["synthwave", "bass", "pulse"]),

    ("Choir Pad", {"obrix_src1Type": WT, "obrix_wtBank": 1, "obrix_src1PW": 0.65,
     "obrix_src2Type": SAW, "obrix_src2Tune": -0.05,
     "obrix_ampAttack": 0.8, "obrix_ampRelease": 1.5,
     "obrix_proc1Cutoff": 3000.0, "obrix_mod2Depth": 0.15, "obrix_mod2Rate": 0.2,
     "obrix_fx1Type": CHORUS, "obrix_fx1Mix": 0.4,
     "obrix_fx2Type": REVERB, "obrix_fx2Mix": 0.3},
     (0.45, 0.6, 0.4, 0.35, 0.5, 0.0), "Vocal wavetable + saw. Synthetic choir.", ["synthwave", "choir", "pad", "vocal"]),

    ("Gated Rhythm", {"obrix_src1Type": SAW, "obrix_src2Type": SQUARE,
     "obrix_ampAttack": 0.001, "obrix_ampDecay": 0.1, "obrix_ampSustain": 0.6,
     "obrix_ampRelease": 0.05,
     "obrix_proc1Cutoff": 3500.0, "obrix_proc1Reso": 0.3,
     "obrix_proc2Type": HP, "obrix_proc2Cutoff": 500.0},
     (0.5, 0.45, 0.2, 0.35, 0.0, 0.3), "Tight gated sound. Perfect for rhythmic patterns.", ["synthwave", "gated", "rhythm"]),

    ("Sunset Keys", {"obrix_src1Type": TRI, "obrix_src2Type": SINE, "obrix_src2Tune": 12.0,
     "obrix_ampAttack": 0.02, "obrix_ampDecay": 0.8, "obrix_ampSustain": 0.3,
     "obrix_proc1Cutoff": 4000.0,
     "obrix_fx1Type": CHORUS, "obrix_fx1Mix": 0.25,
     "obrix_fx2Type": REVERB, "obrix_fx2Mix": 0.2},
     (0.45, 0.6, 0.25, 0.2, 0.35, 0.0), "Gentle triangle + sine octave. Beach sunset.", ["synthwave", "keys", "gentle", "sunset"]),

    ("Retro Pluck", {"obrix_src1Type": SAW,
     "obrix_ampAttack": 0.001, "obrix_ampDecay": 0.3, "obrix_ampSustain": 0.0,
     "obrix_proc1Cutoff": 5000.0, "obrix_proc1Reso": 0.35,
     "obrix_mod1Depth": 0.7, "obrix_mod1Rate": 0.4,
     "obrix_fx1Type": DELAY, "obrix_fx1Mix": 0.25, "obrix_fx1Param": 0.3},
     (0.55, 0.45, 0.3, 0.2, 0.2, 0.15), "Bright saw pluck with delay. Arp food.", ["synthwave", "pluck", "retro"]),

    ("Wide Strings", {"obrix_src1Type": SAW, "obrix_src2Type": SAW,
     "obrix_src2Tune": 0.06,
     "obrix_ampAttack": 0.3, "obrix_ampRelease": 1.0,
     "obrix_proc1Cutoff": 3500.0, "obrix_proc1Reso": 0.15,
     "obrix_proc2Type": LP, "obrix_proc2Cutoff": 4000.0,
     "obrix_mod2Depth": 0.1, "obrix_mod2Rate": 0.25,
     "obrix_fx1Type": CHORUS, "obrix_fx1Mix": 0.45},
     (0.5, 0.55, 0.4, 0.35, 0.35, 0.05), "Detuned saws + deep chorus = string ensemble.", ["synthwave", "strings", "ensemble"]),

    ("Drive Pulse", {"obrix_src1Type": PULSE, "obrix_src1PW": 0.35,
     "obrix_src2Type": SAW, "obrix_src2Tune": -12.0,
     "obrix_polyphony": MONO, "obrix_proc1Cutoff": 2500.0, "obrix_proc1Reso": 0.4,
     "obrix_mod1Depth": 0.5, "obrix_glideTime": 0.04},
     (0.45, 0.5, 0.25, 0.3, 0.0, 0.35), "Pulse + sub octave saw. Midnight drive bass.", ["synthwave", "bass", "drive"]),
]

# Prism genre extra (1 to reach 25)
PRISM_EXTRA = [
    ("Collision Chord", {"obrix_src1Type": SAW, "obrix_src2Type": SQUARE,
     "obrix_proc1Cutoff": 3000.0, "obrix_proc1Reso": 0.35,
     "obrix_proc2Type": HP, "obrix_proc2Cutoff": 800.0,
     "obrix_proc3Type": FOLD,
     "obrix_mod1Depth": 0.4,
     "obrix_fx1Type": CHORUS, "obrix_fx1Mix": 0.3,
     "obrix_fx2Type": REVERB, "obrix_fx2Mix": 0.2},
     (0.5, 0.5, 0.3, 0.5, 0.3, 0.3), "The Constructive Collision as a playable chord sound.", ["genre", "chord", "collision"]),
]

# --- EDM (12) → Flux ---
EDM = [
    ("Festival Supersaw", {"obrix_src1Type": SAW, "obrix_polyphony": MONO,
     "obrix_unisonDetune": 25.0, "obrix_proc1Cutoff": 8000.0, "obrix_proc1Reso": 0.15,
     "obrix_mod1Depth": 0.2, "obrix_fx1Type": REVERB, "obrix_fx1Mix": 0.15,
     "obrix_level": 0.6},
     (0.65, 0.45, 0.3, 0.8, 0.2, 0.4), "Massive unison supersaw. Stadium energy.", ["edm", "supersaw", "festival", "lead"]),

    ("Pluck Chord", {"obrix_src1Type": SAW,
     "obrix_ampAttack": 0.001, "obrix_ampDecay": 0.2, "obrix_ampSustain": 0.0,
     "obrix_proc1Cutoff": 6000.0, "obrix_proc1Reso": 0.3,
     "obrix_mod1Depth": 0.6, "obrix_fx1Type": REVERB, "obrix_fx1Mix": 0.2},
     (0.55, 0.45, 0.15, 0.2, 0.25, 0.15), "Fast decay saw pluck for chord stabs.", ["edm", "pluck", "chord"]),

    ("Sidechain Pad", {"obrix_src1Type": SAW, "obrix_src2Type": SAW, "obrix_src2Tune": -0.05,
     "obrix_ampAttack": 0.02, "obrix_proc1Cutoff": 2500.0, "obrix_proc1Reso": 0.2,
     "obrix_mod2Depth": 0.15, "obrix_mod2Rate": 0.15,
     "obrix_fx1Type": CHORUS, "obrix_fx1Mix": 0.3,
     "obrix_fx2Type": REVERB, "obrix_fx2Mix": 0.25},
     (0.4, 0.55, 0.4, 0.35, 0.4, 0.05), "Warm pad designed for sidechain pumping.", ["edm", "pad", "sidechain"]),

    ("Drop Bass", {"obrix_src1Type": SAW, "obrix_polyphony": MONO,
     "obrix_proc1Cutoff": 1000.0, "obrix_proc1Reso": 0.5,
     "obrix_proc1Feedback": 0.3,
     "obrix_mod1Depth": 0.7, "obrix_mod1Rate": 0.5, "obrix_glideTime": 0.08},
     (0.35, 0.5, 0.3, 0.3, 0.0, 0.5), "Heavy filtered bass for drops. Feedback adds grit.", ["edm", "bass", "drop"]),

    ("Bright Lead", {"obrix_src1Type": SAW, "obrix_unisonDetune": 10.0, "obrix_polyphony": MONO,
     "obrix_proc1Cutoff": 10000.0, "obrix_proc1Reso": 0.1,
     "obrix_glideTime": 0.02,
     "obrix_fx1Type": DELAY, "obrix_fx1Mix": 0.15, "obrix_fx1Param": 0.3},
     (0.7, 0.4, 0.25, 0.4, 0.15, 0.3), "Bright unison lead with subtle delay.", ["edm", "lead", "bright"]),

    ("Riser Build", {"obrix_src1Type": NOISE, "obrix_src2Type": SAW,
     "obrix_srcMix": 0.6,
     "obrix_proc1Cutoff": 500.0, "obrix_proc1Reso": 0.4,
     "obrix_mod1Target": T_CUTOFF, "obrix_mod1Depth": 0.9, "obrix_mod1Rate": 0.1,
     "obrix_ampAttack": 4.0, "obrix_ampSustain": 0.8},
     (0.4, 0.4, 0.5, 0.4, 0.15, 0.3), "Slow filter sweep riser. Building tension.", ["edm", "riser", "build", "fx"]),

    ("Wobble Bass", {"obrix_src1Type": SAW, "obrix_polyphony": MONO,
     "obrix_proc1Cutoff": 800.0, "obrix_proc1Reso": 0.6,
     "obrix_mod2Target": T_CUTOFF, "obrix_mod2Depth": 0.7, "obrix_mod2Rate": 4.0,
     "obrix_glideTime": 0.05},
     (0.4, 0.45, 0.8, 0.3, 0.0, 0.5), "Fast LFO wobble on cutoff. Dubstep territory.", ["edm", "wobble", "bass", "dubstep"]),

    ("Synth Stab", {"obrix_src1Type": SAW, "obrix_src2Type": SAW, "obrix_src2Tune": 7.0,
     "obrix_ampAttack": 0.001, "obrix_ampDecay": 0.12, "obrix_ampSustain": 0.0,
     "obrix_proc1Cutoff": 5000.0, "obrix_proc1Reso": 0.3,
     "obrix_proc2Type": LP, "obrix_proc2Cutoff": 6000.0},
     (0.6, 0.4, 0.1, 0.4, 0.0, 0.35), "Two saws a fifth apart. Quick stab.", ["edm", "stab", "chord"]),

    ("FM Bell", {"obrix_src1Type": SINE, "obrix_src2Type": SINE, "obrix_src2Tune": 19.0,
     "obrix_fmDepth": 0.4,
     "obrix_ampAttack": 0.001, "obrix_ampDecay": 1.0, "obrix_ampSustain": 0.0,
     "obrix_proc1Cutoff": 8000.0,
     "obrix_fx1Type": REVERB, "obrix_fx1Mix": 0.25},
     (0.6, 0.35, 0.1, 0.25, 0.3, 0.1), "FM bell tone. Clean and crystalline.", ["edm", "bell", "fm", "clean"]),

    ("Filtered Arp", {"obrix_src1Type": PULSE, "obrix_src1PW": 0.3,
     "obrix_ampAttack": 0.001, "obrix_ampDecay": 0.15, "obrix_ampSustain": 0.0,
     "obrix_proc1Cutoff": 3500.0, "obrix_proc1Reso": 0.4,
     "obrix_mod1Depth": 0.6,
     "obrix_fx1Type": DELAY, "obrix_fx1Mix": 0.3, "obrix_fx1Param": 0.35},
     (0.5, 0.45, 0.35, 0.2, 0.2, 0.2), "Pulse arp with delay tail. Feed it 16ths.", ["edm", "arp", "pulse", "delay"]),

    ("Growl Lead", {"obrix_src1Type": SAW, "obrix_src2Type": SAW, "obrix_src2Tune": 0.08,
     "obrix_polyphony": MONO,
     "obrix_proc1Cutoff": 2000.0, "obrix_proc1Reso": 0.55, "obrix_proc1Feedback": 0.4,
     "obrix_proc3Type": FOLD,
     "obrix_mod2Depth": 0.4, "obrix_mod2Rate": 6.0,
     "obrix_glideTime": 0.03},
     (0.45, 0.4, 0.7, 0.5, 0.0, 0.65), "Feedback + wavefolder + fast LFO. Aggressive.", ["edm", "growl", "aggressive", "lead"]),

    ("Trance Gate", {"obrix_src1Type": SAW, "obrix_src2Type": SQUARE,
     "obrix_ampAttack": 0.001, "obrix_ampDecay": 0.08, "obrix_ampSustain": 0.7,
     "obrix_ampRelease": 0.08,
     "obrix_proc1Cutoff": 4000.0, "obrix_proc1Reso": 0.25,
     "obrix_fx1Type": DELAY, "obrix_fx1Mix": 0.2, "obrix_fx1Param": 0.3,
     "obrix_fx2Type": REVERB, "obrix_fx2Mix": 0.15},
     (0.55, 0.45, 0.25, 0.35, 0.25, 0.2), "Tight gate for trance sequences.", ["edm", "trance", "gate"]),
]

# --- LO-FI (8) → Flux ---
LOFI = [
    ("Dusty Sub", {"obrix_src1Type": LOFI, "obrix_polyphony": MONO,
     "obrix_proc1Cutoff": 800.0, "obrix_proc1Reso": 0.15,
     "obrix_ampSustain": 0.85, "obrix_glideTime": 0.06},
     (0.2, 0.7, 0.1, 0.15, 0.0, 0.1), "Lo-fi saw sub with mono glide. Warm aliasing.", ["lofi", "sub", "dusty"]),

    ("Detuned Pluck", {"obrix_src1Type": LOFI, "obrix_src2Type": SAW, "obrix_src2Tune": 0.08,
     "obrix_ampAttack": 0.001, "obrix_ampDecay": 0.5, "obrix_ampSustain": 0.0,
     "obrix_proc1Cutoff": 2500.0, "obrix_proc1Reso": 0.3,
     "obrix_mod1Depth": 0.5},
     (0.4, 0.55, 0.15, 0.25, 0.0, 0.1), "Lo-fi + clean saw detuned. Vintage pluck.", ["lofi", "pluck", "detuned"]),

    ("Wobbly Pad", {"obrix_src1Type": LOFI, "obrix_src2Type": LOFI, "obrix_src2Tune": -0.1,
     "obrix_ampAttack": 0.6, "obrix_ampRelease": 1.5,
     "obrix_proc1Cutoff": 1500.0,
     "obrix_mod2Depth": 0.3, "obrix_mod2Rate": 0.3,
     "obrix_driftRate": 0.008, "obrix_driftDepth": 0.3,
     "obrix_fx1Type": CHORUS, "obrix_fx1Mix": 0.25},
     (0.3, 0.65, 0.5, 0.3, 0.2, 0.05), "Two detuned lo-fi saws with drift. Warm wobble.", ["lofi", "pad", "wobbly"]),

    ("Tape Lead", {"obrix_src1Type": LOFI, "obrix_polyphony": LEGATO,
     "obrix_proc1Cutoff": 3000.0, "obrix_proc1Reso": 0.25,
     "obrix_mod1Depth": 0.4, "obrix_glideTime": 0.04,
     "obrix_driftRate": 0.01, "obrix_driftDepth": 0.2,
     "obrix_fx1Type": DELAY, "obrix_fx1Mix": 0.2, "obrix_fx1Param": 0.35},
     (0.4, 0.6, 0.35, 0.2, 0.15, 0.1), "Lo-fi lead with drift. Warped cassette vibes.", ["lofi", "lead", "tape"]),

    ("Vinyl Keys", {"obrix_src1Type": TRI, "obrix_src2Type": LOFI, "obrix_src2Tune": 12.0,
     "obrix_srcMix": 0.7,
     "obrix_ampDecay": 0.6, "obrix_ampSustain": 0.3,
     "obrix_proc1Cutoff": 3000.0,
     "obrix_fx1Type": CHORUS, "obrix_fx1Mix": 0.2},
     (0.4, 0.6, 0.2, 0.2, 0.15, 0.0), "Triangle + lo-fi octave. Crackly old keys.", ["lofi", "keys", "vinyl"]),

    ("Haze Chord", {"obrix_src1Type": LOFI, "obrix_src2Type": LOFI,
     "obrix_src2Tune": 7.0,
     "obrix_proc1Cutoff": 2000.0, "obrix_proc1Reso": 0.2,
     "obrix_mod2Depth": 0.1, "obrix_mod2Rate": 0.15,
     "obrix_driftRate": 0.005, "obrix_driftDepth": 0.25,
     "obrix_fx1Type": REVERB, "obrix_fx1Mix": 0.2},
     (0.3, 0.65, 0.3, 0.3, 0.25, 0.0), "Two lo-fi saws a fifth apart. Hazy nostalgia.", ["lofi", "chord", "haze"]),

    ("Warm Noise", {"obrix_src1Type": NOISE, "obrix_src2Type": LOFI,
     "obrix_srcMix": 0.4,
     "obrix_proc1Cutoff": 1500.0, "obrix_proc1Reso": 0.3,
     "obrix_ampAttack": 0.3, "obrix_ampRelease": 1.0,
     "obrix_mod2Depth": 0.2, "obrix_mod2Rate": 0.2, "obrix_air": 0.3},
     (0.3, 0.65, 0.3, 0.3, 0.1, 0.05), "Filtered noise + lo-fi saw. Warm texture.", ["lofi", "noise", "texture"]),

    ("Bit Crush Feel", {"obrix_src1Type": LOFI,
     "obrix_proc1Cutoff": 4000.0, "obrix_proc1Reso": 0.4,
     "obrix_proc3Type": FOLD,
     "obrix_mod1Depth": 0.4, "obrix_mod2Depth": 0.2, "obrix_mod2Rate": 0.4,
     "obrix_macroCharacter": 0.3},
     (0.4, 0.5, 0.4, 0.3, 0.0, 0.25), "Lo-fi saw through wavefolder. Digital crunch.", ["lofi", "bitcrush", "gritty"]),
]

# --- EXPERIMENTAL (10) → Aether ---
EXPERIMENTAL = [
    ("Feedback Drone", {"obrix_src1Type": SINE,
     "obrix_proc1Cutoff": 800.0, "obrix_proc1Reso": 0.8, "obrix_proc1Feedback": 0.7,
     "obrix_ampAttack": 1.0, "obrix_ampSustain": 0.9,
     "obrix_mod2Depth": 0.1, "obrix_mod2Rate": 0.05,
     "obrix_driftRate": 0.003, "obrix_driftDepth": 0.4,
     "obrix_journeyMode": 1.0},
     (0.3, 0.5, 0.35, 0.35, 0.2, 0.3), "Self-oscillating filter drone. Journey mode. Let it evolve.", ["experimental", "drone", "feedback", "journey"]),

    ("Noise Riser", {"obrix_src1Type": NOISE,
     "obrix_proc1Type": BP, "obrix_proc1Cutoff": 400.0, "obrix_proc1Reso": 0.7,
     "obrix_mod1Target": T_CUTOFF, "obrix_mod1Depth": 0.9, "obrix_mod1Rate": 0.08,
     "obrix_ampAttack": 3.0, "obrix_ampSustain": 0.8},
     (0.4, 0.35, 0.3, 0.35, 0.1, 0.3), "Bandpass sweep through noise. Long build.", ["experimental", "riser", "noise", "sweep"]),

    ("Wavefolder Growl", {"obrix_src1Type": SAW, "obrix_src2Type": SQUARE,
     "obrix_proc1Cutoff": 1500.0, "obrix_proc1Reso": 0.5, "obrix_proc1Feedback": 0.4,
     "obrix_proc3Type": FOLD,
     "obrix_mod1Depth": 0.6, "obrix_macroCharacter": 0.5,
     "obrix_polyphony": MONO, "obrix_glideTime": 0.06},
     (0.45, 0.4, 0.3, 0.45, 0.0, 0.65), "Wavefolder + feedback. Push CHARACTER macro for more.", ["experimental", "growl", "wavefolder", "aggressive"]),

    ("FM Ghost", {"obrix_src1Type": SINE, "obrix_src2Type": SINE, "obrix_src2Tune": 7.02,
     "obrix_fmDepth": 0.6,
     "obrix_ampAttack": 0.3, "obrix_ampRelease": 2.0,
     "obrix_proc1Type": P_OFF,
     "obrix_driftRate": 0.004, "obrix_driftDepth": 0.5,
     "obrix_fx1Type": REVERB, "obrix_fx1Mix": 0.4, "obrix_fx1Param": 0.6},
     (0.5, 0.35, 0.4, 0.3, 0.55, 0.15), "Deep FM with inharmonic ratio. Ghostly presence.", ["experimental", "fm", "ghost", "inharmonic"]),

    ("Ring Cascade", {"obrix_src1Type": WT, "obrix_wtBank": 2, "obrix_src2Type": SINE,
     "obrix_src2Tune": 14.0,
     "obrix_proc1Type": RING, "obrix_fmDepth": 0.2,
     "obrix_ampDecay": 0.8, "obrix_ampSustain": 0.2,
     "obrix_fx1Type": DELAY, "obrix_fx1Mix": 0.35, "obrix_fx1Param": 0.5},
     (0.55, 0.3, 0.3, 0.4, 0.3, 0.35), "Ring mod + FM + metallic wavetable. Cascading harmonics.", ["experimental", "ring-mod", "cascade"]),

    ("Drift Meditation", {"obrix_src1Type": WT, "obrix_wtBank": 3, "obrix_src1PW": 0.6,
     "obrix_src2Type": SINE, "obrix_src2Tune": 0.05,
     "obrix_ampAttack": 2.0, "obrix_ampSustain": 0.9, "obrix_ampRelease": 3.0,
     "obrix_proc1Type": P_OFF,
     "obrix_driftRate": 0.002, "obrix_driftDepth": 0.7,
     "obrix_journeyMode": 1.0,
     "obrix_fx1Type": REVERB, "obrix_fx1Mix": 0.35,
     "obrix_distance": 0.3},
     (0.3, 0.6, 0.5, 0.25, 0.55, 0.0), "Maximum drift depth. Journey mode. Close your eyes.", ["experimental", "meditation", "drift", "journey"]),

    ("Collision Study", {"obrix_src1Type": SAW, "obrix_src2Type": PULSE, "obrix_src2PW": 0.2,
     "obrix_proc1Type": HP, "obrix_proc1Cutoff": 2000.0, "obrix_proc1Reso": 0.4,
     "obrix_proc2Type": LP, "obrix_proc2Cutoff": 1500.0, "obrix_proc2Reso": 0.5,
     "obrix_proc2Feedback": 0.3,
     "obrix_proc3Type": RING,
     "obrix_mod1Depth": 0.5, "obrix_mod4Depth": 0.4},
     (0.45, 0.4, 0.3, 0.55, 0.0, 0.45), "HP on source 1, LP+feedback on source 2, ring mod post-mix. Pure collision.", ["experimental", "collision", "complex"]),

    ("Breath Machine", {"obrix_src1Type": NOISE, "obrix_src2Type": WT, "obrix_wtBank": 1,
     "obrix_src2PW": 0.6, "obrix_srcMix": 0.4,
     "obrix_proc1Type": BP, "obrix_proc1Cutoff": 1200.0, "obrix_proc1Reso": 0.5,
     "obrix_mod2Target": T_CUTOFF, "obrix_mod2Depth": 0.5, "obrix_mod2Rate": 0.15,
     "obrix_air": 0.35},
     (0.35, 0.5, 0.5, 0.4, 0.15, 0.2), "Noise through vocal wavetable resonance. Mechanical breathing.", ["experimental", "breath", "noise", "vocal"]),

    ("Frozen Time", {"obrix_src1Type": SINE, "obrix_src2Type": TRI,
     "obrix_src2Tune": 4.98,
     "obrix_ampAttack": 3.0, "obrix_ampSustain": 0.95, "obrix_ampRelease": 5.0,
     "obrix_proc1Type": P_OFF,
     "obrix_driftRate": 0.001, "obrix_driftDepth": 0.8,
     "obrix_journeyMode": 1.0,
     "obrix_fx1Type": REVERB, "obrix_fx1Mix": 0.5, "obrix_fx1Param": 0.75,
     "obrix_distance": 0.5, "obrix_air": 0.6},
     (0.25, 0.4, 0.4, 0.2, 0.8, 0.0), "Slowest drift. Journey mode. Time stops.", ["experimental", "frozen", "time", "glacial"]),

    ("Alien Signal", {"obrix_src1Type": WT, "obrix_wtBank": 2,
     "obrix_src2Type": SINE, "obrix_src2Tune": 3.14,
     "obrix_fmDepth": 0.5,
     "obrix_proc1Type": BP, "obrix_proc1Cutoff": 2000.0, "obrix_proc1Reso": 0.6,
     "obrix_mod2Target": T_PITCH, "obrix_mod2Depth": 0.3, "obrix_mod2Rate": 2.5,
     "obrix_fx1Type": DELAY, "obrix_fx1Mix": 0.3, "obrix_fx1Param": 0.45},
     (0.5, 0.3, 0.6, 0.4, 0.25, 0.35), "Inharmonic FM + fast pitch LFO. Something from deep space.", ["experimental", "alien", "inharmonic", "fm"]),
]

# --- AETHER extras (5) ---
AETHER_EXTRAS = [
    ("Infinite Reef", {"obrix_src1Type": WT, "obrix_wtBank": 3, "obrix_src2Type": WT, "obrix_wtBank": 1,
     "obrix_src1PW": 0.65, "obrix_src2PW": 0.55, "obrix_src2Tune": 3.0,
     "obrix_ampAttack": 2.0, "obrix_ampSustain": 0.9,
     "obrix_proc1Cutoff": 2000.0, "obrix_proc2Type": LP, "obrix_proc2Cutoff": 2500.0,
     "obrix_driftRate": 0.003, "obrix_driftDepth": 0.6,
     "obrix_journeyMode": 1.0,
     "obrix_fx1Type": CHORUS, "obrix_fx1Mix": 0.3,
     "obrix_fx2Type": REVERB, "obrix_fx2Mix": 0.35,
     "obrix_distance": 0.25, "obrix_air": 0.4},
     (0.4, 0.6, 0.55, 0.4, 0.55, 0.0), "The reef growing forever. Journey + max drift.", ["aether", "reef", "infinite", "journey"]),

    ("Void Pulse", {"obrix_src1Type": PULSE, "obrix_src1PW": 0.15,
     "obrix_proc1Type": HP, "obrix_proc1Cutoff": 3000.0,
     "obrix_proc1Feedback": 0.5,
     "obrix_mod2Target": T_PW, "obrix_mod2Depth": 0.6, "obrix_mod2Rate": 0.08,
     "obrix_driftRate": 0.004, "obrix_driftDepth": 0.4,
     "obrix_fx1Type": DELAY, "obrix_fx1Mix": 0.4, "obrix_fx1Param": 0.55,
     "obrix_distance": 0.5, "obrix_air": 0.65},
     (0.4, 0.3, 0.5, 0.3, 0.5, 0.2), "Narrow pulse through HP feedback. Cold and distant.", ["aether", "pulse", "void", "cold"]),

    ("Coral Memory", {"obrix_src1Type": SINE, "obrix_src2Type": WT, "obrix_wtBank": 3,
     "obrix_src2PW": 0.7, "obrix_src2Tune": 12.0,
     "obrix_ampAttack": 1.5, "obrix_ampRelease": 3.0,
     "obrix_mod2Depth": 0.1, "obrix_mod2Rate": 0.04,
     "obrix_driftRate": 0.002, "obrix_driftDepth": 0.5,
     "obrix_fx1Type": REVERB, "obrix_fx1Mix": 0.45, "obrix_fx1Param": 0.65,
     "obrix_distance": 0.35, "obrix_air": 0.45},
     (0.35, 0.55, 0.3, 0.25, 0.7, 0.0), "Sine + organic octave. A memory of the reef.", ["aether", "memory", "organic", "ambient"]),

    ("Storm Inside", {"obrix_src1Type": NOISE, "obrix_src2Type": SAW,
     "obrix_srcMix": 0.3,
     "obrix_proc1Type": BP, "obrix_proc1Cutoff": 1000.0, "obrix_proc1Reso": 0.6,
     "obrix_proc1Feedback": 0.5,
     "obrix_proc2Type": LP, "obrix_proc2Cutoff": 600.0, "obrix_proc2Reso": 0.4,
     "obrix_mod2Depth": 0.4, "obrix_mod2Rate": 0.1,
     "obrix_driftRate": 0.008, "obrix_driftDepth": 0.35,
     "obrix_air": 0.3},
     (0.35, 0.5, 0.5, 0.5, 0.2, 0.45), "Resonant noise storm over dark saw. Inner turbulence.", ["aether", "storm", "noise", "dark"]),

    ("Last Light", {"obrix_src1Type": TRI, "obrix_src2Type": SINE, "obrix_src2Tune": 7.0,
     "obrix_ampAttack": 3.0, "obrix_ampRelease": 5.0, "obrix_ampSustain": 0.8,
     "obrix_proc1Type": P_OFF,
     "obrix_driftRate": 0.001, "obrix_driftDepth": 0.6,
     "obrix_journeyMode": 1.0,
     "obrix_fx1Type": REVERB, "obrix_fx1Mix": 0.5, "obrix_fx1Param": 0.8,
     "obrix_distance": 0.6, "obrix_air": 0.55},
     (0.3, 0.5, 0.3, 0.2, 0.85, 0.0), "Triangle + fifth. Maximum distance and reverb. Fading away.", ["aether", "fade", "distant", "final"]),
]

# ══════════════════════════════════════════════════════════════════════════════
# ENTANGLED (20) — coupling showcases (single-engine but coupling-ready)
# ══════════════════════════════════════════════════════════════════════════════

ENTANGLED = [
    ("Coupling Donor", {"obrix_src1Type": SAW, "obrix_src2Type": WT, "obrix_wtBank": 0,
     "obrix_src2PW": 0.7,
     "obrix_proc1Cutoff": 3000.0, "obrix_proc1Reso": 0.3,
     "obrix_proc2Type": LP, "obrix_proc2Cutoff": 4000.0,
     "obrix_mod1Depth": 0.5, "obrix_mod3Depth": 0.6, "obrix_macroCoupling": 0.5},
     (0.5, 0.5, 0.35, 0.4, 0.15, 0.2), "Designed to feed coupling output to other engines.", ["entangled", "coupling", "donor"]),

    ("Coupling Receiver", {"obrix_src1Type": SINE,
     "obrix_proc1Cutoff": 1500.0, "obrix_proc1Reso": 0.4,
     "obrix_mod1Depth": 0.3, "obrix_macroCoupling": 0.8},
     (0.35, 0.55, 0.4, 0.2, 0.1, 0.1), "Simple sine, high coupling sensitivity. Let other engines shape it.", ["entangled", "coupling", "receiver"]),

    ("Reef Sympathy", {"obrix_src1Type": WT, "obrix_wtBank": 3, "obrix_src1PW": 0.6,
     "obrix_proc1Cutoff": 2500.0, "obrix_mod1Depth": 0.4,
     "obrix_driftRate": 0.005, "obrix_driftDepth": 0.3,
     "obrix_macroCoupling": 0.6,
     "obrix_fx1Type": REVERB, "obrix_fx1Mix": 0.2},
     (0.4, 0.6, 0.45, 0.3, 0.3, 0.05), "Organic wavetable with drift. Sympathetic to fleet coupling.", ["entangled", "reef", "sympathy"]),

    ("Brick Bridge", {"obrix_src1Type": SAW, "obrix_src2Type": SQUARE,
     "obrix_proc1Cutoff": 2000.0, "obrix_proc1Reso": 0.35,
     "obrix_proc2Type": HP, "obrix_proc2Cutoff": 1000.0,
     "obrix_proc3Type": FOLD,
     "obrix_macroCoupling": 0.7, "obrix_macroCharacter": 0.3},
     (0.5, 0.45, 0.3, 0.5, 0.1, 0.35), "Full collision routing with high coupling. Bridges between engines.", ["entangled", "bridge", "collision"]),

    ("FM Resonator", {"obrix_src1Type": SINE, "obrix_src2Type": SINE,
     "obrix_fmDepth": 0.4, "obrix_src2Tune": 5.0,
     "obrix_proc1Cutoff": 4000.0, "obrix_proc1Reso": 0.5,
     "obrix_macroCoupling": 0.6},
     (0.5, 0.4, 0.3, 0.35, 0.1, 0.2), "FM pair tuned to resonate with coupling input.", ["entangled", "fm", "resonator"]),

    ("Texture Mesh", {"obrix_src1Type": NOISE, "obrix_src2Type": WT, "obrix_wtBank": 2,
     "obrix_srcMix": 0.4, "obrix_src2PW": 0.75,
     "obrix_proc1Type": BP, "obrix_proc1Cutoff": 1500.0, "obrix_proc1Reso": 0.5,
     "obrix_proc2Type": LP, "obrix_proc2Cutoff": 3000.0,
     "obrix_macroCoupling": 0.5, "obrix_driftRate": 0.006, "obrix_driftDepth": 0.25},
     (0.4, 0.45, 0.4, 0.45, 0.15, 0.15), "Noise + metallic wavetable mesh. Coupling adds movement.", ["entangled", "texture", "mesh"]),

    ("Pulse Echo Net", {"obrix_src1Type": PULSE, "obrix_src1PW": 0.3,
     "obrix_fx1Type": DELAY, "obrix_fx1Mix": 0.35, "obrix_fx1Param": 0.45,
     "obrix_proc1Cutoff": 3000.0, "obrix_proc1Reso": 0.35,
     "obrix_mod1Depth": 0.5, "obrix_macroCoupling": 0.6},
     (0.45, 0.5, 0.4, 0.3, 0.3, 0.15), "Pulse with delay. Coupling modulates the echo pattern.", ["entangled", "pulse", "delay", "echo"]),

    ("Drift Weave", {"obrix_src1Type": SAW, "obrix_src2Type": SAW, "obrix_src2Tune": 0.07,
     "obrix_driftRate": 0.005, "obrix_driftDepth": 0.5,
     "obrix_proc1Cutoff": 2500.0, "obrix_proc1Reso": 0.25,
     "obrix_proc2Type": LP, "obrix_proc2Cutoff": 3000.0,
     "obrix_macroCoupling": 0.5,
     "obrix_fx1Type": CHORUS, "obrix_fx1Mix": 0.25},
     (0.4, 0.55, 0.55, 0.35, 0.2, 0.05), "Drifting saws woven with coupling modulation.", ["entangled", "drift", "weave"]),

    ("Journey Pair", {"obrix_src1Type": WT, "obrix_wtBank": 1, "obrix_src1PW": 0.65,
     "obrix_journeyMode": 1.0,
     "obrix_driftRate": 0.004, "obrix_driftDepth": 0.4,
     "obrix_ampAttack": 1.0, "obrix_ampSustain": 0.9,
     "obrix_proc1Cutoff": 2000.0,
     "obrix_macroCoupling": 0.7,
     "obrix_fx1Type": REVERB, "obrix_fx1Mix": 0.3},
     (0.35, 0.6, 0.5, 0.3, 0.45, 0.0), "Journey mode vocal wavetable. Designed for paired coupling.", ["entangled", "journey", "pair"]),

    ("Spatial Knot", {"obrix_src1Type": SINE, "obrix_src2Type": TRI, "obrix_src2Tune": 5.0,
     "obrix_distance": 0.4, "obrix_air": 0.6,
     "obrix_fx1Type": REVERB, "obrix_fx1Mix": 0.35,
     "obrix_macroCoupling": 0.8},
     (0.35, 0.45, 0.25, 0.25, 0.6, 0.0), "Distant sound with cold air. Coupling pulls it closer.", ["entangled", "spatial", "knot"]),

    ("Collision Feed", {"obrix_src1Type": SAW, "obrix_src2Type": SQUARE,
     "obrix_proc1Cutoff": 2500.0, "obrix_proc1Reso": 0.4,
     "obrix_proc2Type": BP, "obrix_proc2Cutoff": 1500.0, "obrix_proc2Reso": 0.5,
     "obrix_proc3Type": RING,
     "obrix_mod1Depth": 0.5, "obrix_macroCoupling": 0.6},
     (0.45, 0.4, 0.3, 0.55, 0.1, 0.4), "Full collision: LP, BP, ring mod. Rich coupling feed.", ["entangled", "collision", "ring-mod"]),

    ("Harmonic Cage", {"obrix_src1Type": SINE, "obrix_src2Type": SINE,
     "obrix_fmDepth": 0.5, "obrix_src2Tune": 12.0,
     "obrix_proc1Cutoff": 6000.0, "obrix_proc1Reso": 0.3,
     "obrix_mod1Depth": 0.4, "obrix_macroCoupling": 0.5},
     (0.55, 0.4, 0.2, 0.3, 0.1, 0.15), "FM harmonics caged by filter. Coupling releases them.", ["entangled", "fm", "harmonic"]),

    ("Feedback Loop", {"obrix_src1Type": SAW,
     "obrix_proc1Cutoff": 1200.0, "obrix_proc1Reso": 0.6,
     "obrix_proc1Feedback": 0.5,
     "obrix_mod1Depth": 0.5, "obrix_mod2Depth": 0.15, "obrix_mod2Rate": 0.2,
     "obrix_macroCoupling": 0.7},
     (0.4, 0.5, 0.4, 0.35, 0.1, 0.3), "Self-oscillating filter. Coupling input steers the resonance.", ["entangled", "feedback", "resonance"]),

    ("Sub Coupling", {"obrix_src1Type": SINE, "obrix_polyphony": MONO,
     "obrix_proc1Cutoff": 400.0, "obrix_ampSustain": 0.9,
     "obrix_macroCoupling": 0.9},
     (0.1, 0.7, 0.2, 0.1, 0.0, 0.1), "Pure sub. Maximum coupling sensitivity. Let the fleet shape it.", ["entangled", "sub", "coupling"]),

    ("Wavetable Link", {"obrix_src1Type": WT, "obrix_wtBank": 2, "obrix_src1PW": 0.8,
     "obrix_src2Type": WT, "obrix_wtBank": 0, "obrix_src2PW": 0.6,
     "obrix_proc1Cutoff": 3500.0, "obrix_proc2Type": LP, "obrix_proc2Cutoff": 4000.0,
     "obrix_macroCoupling": 0.5, "obrix_driftRate": 0.005, "obrix_driftDepth": 0.2},
     (0.5, 0.5, 0.4, 0.4, 0.15, 0.15), "Two wavetable banks linked. Coupling adds cross-modulation.", ["entangled", "wavetable", "dual"]),

    ("Gesture Couple", {"obrix_src1Type": SAW,
     "obrix_proc1Cutoff": 2000.0, "obrix_proc1Reso": 0.35,
     "obrix_mod1Depth": 0.5,
     "obrix_gestureType": 2, "obrix_macroCoupling": 0.6,
     "obrix_fx1Type": DELAY, "obrix_fx1Mix": 0.2, "obrix_fx1Param": 0.3},
     (0.45, 0.5, 0.35, 0.3, 0.2, 0.2), "FLASH gesture + coupling. Trigger creates coupled bursts.", ["entangled", "gesture", "flash"]),

    ("Distance Couple", {"obrix_src1Type": WT, "obrix_wtBank": 1, "obrix_src1PW": 0.7,
     "obrix_ampAttack": 0.5, "obrix_ampRelease": 1.5,
     "obrix_proc1Cutoff": 3000.0,
     "obrix_distance": 0.5, "obrix_air": 0.55,
     "obrix_macroCoupling": 0.7,
     "obrix_fx1Type": REVERB, "obrix_fx1Mix": 0.3},
     (0.4, 0.5, 0.3, 0.3, 0.55, 0.0), "Distant vocal wavetable. Coupling overcomes the distance.", ["entangled", "distance", "spatial"]),

    ("Unison Couple", {"obrix_src1Type": SAW, "obrix_polyphony": MONO,
     "obrix_unisonDetune": 15.0,
     "obrix_proc1Cutoff": 4000.0, "obrix_proc1Reso": 0.2,
     "obrix_macroCoupling": 0.5, "obrix_level": 0.65},
     (0.55, 0.45, 0.3, 0.6, 0.1, 0.2), "Unison supersaw optimized for coupling output.", ["entangled", "unison", "supersaw"]),

    ("Macro Entangle", {
        "obrix_src1Type": SAW, "obrix_src2Type": SQUARE,
        "obrix_proc1Cutoff": 2500.0, "obrix_proc1Reso": 0.3,
        "obrix_proc2Type": HP, "obrix_proc2Cutoff": 1000.0,
        "obrix_macroCharacter": 0.4, "obrix_macroMovement": 0.3,
        "obrix_macroCoupling": 0.6, "obrix_macroSpace": 0.2,
        "obrix_fx1Type": CHORUS, "obrix_fx1Mix": 0.25,
        "obrix_fx2Type": REVERB, "obrix_fx2Mix": 0.2},
     (0.5, 0.5, 0.4, 0.4, 0.3, 0.2), "All four macros active. Coupling responds to every axis.", ["entangled", "macros", "all-four"]),

    ("Journey Entangle", {"obrix_src1Type": SAW, "obrix_src2Type": SINE,
     "obrix_src2Tune": 7.0,
     "obrix_journeyMode": 1.0,
     "obrix_driftRate": 0.005, "obrix_driftDepth": 0.4,
     "obrix_proc1Cutoff": 2000.0, "obrix_proc1Reso": 0.3,
     "obrix_macroCoupling": 0.8,
     "obrix_fx1Type": DELAY, "obrix_fx1Mix": 0.25,
     "obrix_fx2Type": REVERB, "obrix_fx2Mix": 0.2},
     (0.4, 0.55, 0.55, 0.35, 0.4, 0.05), "Journey mode + high coupling. Infinite entanglement.", ["entangled", "journey", "infinite"]),
]

# ══════════════════════════════════════════════════════════════════════════════
# FAMILY (15) — OBRIX + other engine coupling presets
# ══════════════════════════════════════════════════════════════════════════════

def make_family(name, partner, overrides, dna, desc, tags, coupling_type="AudioToFM", amount=0.5):
    return make_preset(name, "Family", overrides, dna, desc, tags,
                       engines=["Obrix", partner],
                       coupling=[{"engineA": "Obrix", "engineB": partner,
                                  "type": coupling_type, "amount": amount}],
                       coupling_intensity="Medium")

FAMILY_PRESETS = []
partners = [
    ("Overdub", "Dub shadow beneath the reef.", {"obrix_src1Type": SAW, "obrix_proc1Cutoff": 2500.0, "obrix_mod1Depth": 0.4, "obrix_fx1Type": DELAY, "obrix_fx1Mix": 0.2}, (0.45, 0.55, 0.35, 0.35, 0.3, 0.15), ["family", "overdub", "dub"]),
    ("Odyssey", "Reef meets drift. Two wanderers.", {"obrix_src1Type": WT, "obrix_wtBank": 3, "obrix_src1PW": 0.6, "obrix_driftRate": 0.005, "obrix_driftDepth": 0.3}, (0.4, 0.55, 0.45, 0.3, 0.25, 0.1), ["family", "odyssey", "drift"]),
    ("Onset", "Bricks and drums. Percussive reef.", {"obrix_src1Type": SINE, "obrix_ampDecay": 0.2, "obrix_ampSustain": 0.0, "obrix_fmDepth": 0.3}, (0.5, 0.4, 0.2, 0.35, 0.1, 0.35), ["family", "onset", "drums"]),
    ("Overworld", "Chip coral. 8-bit reef.", {"obrix_src1Type": SQUARE, "obrix_proc1Cutoff": 6000.0, "obrix_ampDecay": 0.3, "obrix_ampSustain": 0.2}, (0.55, 0.4, 0.2, 0.25, 0.1, 0.2), ["family", "overworld", "chip"]),
    ("Opal", "Granular reef. Texture layers.", {"obrix_src1Type": WT, "obrix_wtBank": 2, "obrix_src1PW": 0.75, "obrix_driftRate": 0.004, "obrix_driftDepth": 0.35, "obrix_fx1Type": REVERB, "obrix_fx1Mix": 0.25}, (0.45, 0.5, 0.4, 0.4, 0.4, 0.1), ["family", "opal", "granular"]),
    ("Organon", "Metabolic reef. Living architecture.", {"obrix_src1Type": SAW, "obrix_src2Type": WT, "obrix_wtBank": 3, "obrix_driftRate": 0.003, "obrix_driftDepth": 0.4, "obrix_journeyMode": 1.0}, (0.4, 0.6, 0.5, 0.4, 0.35, 0.05), ["family", "organon", "metabolic"]),
    ("Ouroboros", "Infinite loop reef. Strange attractors.", {"obrix_src1Type": SAW, "obrix_proc1Cutoff": 1500.0, "obrix_proc1Reso": 0.5, "obrix_proc1Feedback": 0.4, "obrix_mod2Depth": 0.3, "obrix_mod2Rate": 0.3}, (0.4, 0.45, 0.45, 0.4, 0.15, 0.3), ["family", "ouroboros", "feedback"]),
    ("Oblong", "Warm amber meets jade reef.", {"obrix_src1Type": SAW, "obrix_src2Type": SAW, "obrix_src2Tune": -0.05, "obrix_proc1Cutoff": 3000.0, "obrix_fx1Type": CHORUS, "obrix_fx1Mix": 0.3}, (0.45, 0.6, 0.35, 0.3, 0.25, 0.1), ["family", "oblong", "warm"]),
    ("Obese", "Fat reef. Maximum saturation.", {"obrix_src1Type": SAW, "obrix_proc1Cutoff": 1500.0, "obrix_proc1Reso": 0.5, "obrix_proc1Feedback": 0.5, "obrix_proc3Type": FOLD, "obrix_macroCharacter": 0.5}, (0.4, 0.5, 0.3, 0.5, 0.1, 0.55), ["family", "obese", "fat", "saturation"]),
    ("Oracle", "Prophetic reef. Stochastic harmonics.", {"obrix_src1Type": SINE, "obrix_fmDepth": 0.5, "obrix_src2Type": SINE, "obrix_src2Tune": 7.02, "obrix_driftRate": 0.004, "obrix_driftDepth": 0.4}, (0.5, 0.4, 0.4, 0.3, 0.2, 0.15), ["family", "oracle", "prophetic"]),
    ("Oceanic", "Reef in the deep ocean.", {"obrix_src1Type": WT, "obrix_wtBank": 3, "obrix_src1PW": 0.6, "obrix_proc1Cutoff": 1500.0, "obrix_driftRate": 0.003, "obrix_driftDepth": 0.5, "obrix_distance": 0.4, "obrix_air": 0.35}, (0.3, 0.65, 0.45, 0.3, 0.55, 0.0), ["family", "oceanic", "deep"]),
    ("Overlap", "Entangled reef topology.", {"obrix_src1Type": SAW, "obrix_src2Type": WT, "obrix_wtBank": 2, "obrix_proc1Cutoff": 2500.0, "obrix_macroCoupling": 0.7}, (0.45, 0.5, 0.35, 0.4, 0.2, 0.15), ["family", "overlap", "entangled"]),
    ("Origami", "Folded reef. Paper and coral.", {"obrix_src1Type": SAW, "obrix_proc3Type": FOLD, "obrix_proc1Cutoff": 3000.0, "obrix_mod1Depth": 0.5, "obrix_macroCharacter": 0.4}, (0.5, 0.45, 0.25, 0.4, 0.1, 0.3), ["family", "origami", "fold"]),
    ("OceanDeep", "Trench reef. Pressure and darkness.", {"obrix_src1Type": SAW, "obrix_proc1Cutoff": 800.0, "obrix_proc1Reso": 0.4, "obrix_driftRate": 0.002, "obrix_driftDepth": 0.5, "obrix_distance": 0.6, "obrix_air": 0.25}, (0.2, 0.65, 0.4, 0.35, 0.45, 0.1), ["family", "oceandeep", "trench"]),
    ("Osprey", "Shore reef. Where water meets land.", {"obrix_src1Type": WT, "obrix_wtBank": 0, "obrix_src1PW": 0.7, "obrix_proc1Cutoff": 3500.0, "obrix_fx1Type": REVERB, "obrix_fx1Mix": 0.25, "obrix_air": 0.55}, (0.45, 0.5, 0.3, 0.3, 0.4, 0.05), ["family", "osprey", "shore"]),
]

for partner_name, desc, overrides, dna, tags in partners:
    name = f"Reef x {partner_name}"
    FAMILY_PRESETS.append(make_preset(name, "Family", overrides, dna, desc, tags,
        engines=["Obrix", partner_name],
        coupling=[{"engineA": "Obrix", "engineB": partner_name,
                   "type": "AudioToFM", "amount": 0.5}],
        coupling_intensity="Medium"))


# ══════════════════════════════════════════════════════════════════════════════
# GENERATE ALL PRESETS
# ══════════════════════════════════════════════════════════════════════════════

def main():
    count = 0
    # Lessons (Foundation)
    for name, overrides, dna, desc, tags in LESSONS:
        save(make_preset(name, "Foundation", overrides, dna, desc, tags))
        count += 1
    # Foundation basics
    for name, overrides, dna, desc, tags in FOUNDATION_BASICS:
        save(make_preset(name, "Foundation", overrides, dna, desc, tags))
        count += 1
    # Place presets (Atmosphere)
    for name, overrides, dna, desc, tags in PLACE_PRESETS:
        save(make_preset(name, "Atmosphere", overrides, dna, desc, tags))
        count += 1
    # Atmosphere extras
    for name, overrides, dna, desc, tags in ATMO_EXTRAS:
        save(make_preset(name, "Atmosphere", overrides, dna, desc, tags))
        count += 1
    # Techno (Prism)
    for name, overrides, dna, desc, tags in TECHNO:
        save(make_preset(name, "Prism", overrides, dna, desc, tags))
        count += 1
    # Synthwave (Prism)
    for name, overrides, dna, desc, tags in SYNTHWAVE:
        save(make_preset(name, "Prism", overrides, dna, desc, tags))
        count += 1
    # Prism extra
    for name, overrides, dna, desc, tags in PRISM_EXTRA:
        save(make_preset(name, "Prism", overrides, dna, desc, tags))
        count += 1
    # EDM (Flux)
    for name, overrides, dna, desc, tags in EDM:
        save(make_preset(name, "Flux", overrides, dna, desc, tags))
        count += 1
    # Lo-Fi (Flux)
    for name, overrides, dna, desc, tags in LOFI:
        save(make_preset(name, "Flux", overrides, dna, desc, tags))
        count += 1
    # Experimental (Aether)
    for name, overrides, dna, desc, tags in EXPERIMENTAL:
        save(make_preset(name, "Aether", overrides, dna, desc, tags))
        count += 1
    # Aether extras
    for name, overrides, dna, desc, tags in AETHER_EXTRAS:
        save(make_preset(name, "Aether", overrides, dna, desc, tags))
        count += 1
    # Entangled
    for name, overrides, dna, desc, tags in ENTANGLED:
        save(make_preset(name, "Entangled", overrides, dna, desc, tags))
        count += 1
    # Family (pre-built)
    for preset in FAMILY_PRESETS:
        save(preset)
        count += 1

    print(f"Generated {count} OBRIX presets across 7 moods")
    # Distribution check
    from collections import Counter
    moods = Counter()
    moods["Foundation"] = len(LESSONS) + len(FOUNDATION_BASICS)
    moods["Atmosphere"] = len(PLACE_PRESETS) + len(ATMO_EXTRAS)
    moods["Prism"] = len(TECHNO) + len(SYNTHWAVE) + len(PRISM_EXTRA)
    moods["Flux"] = len(EDM) + len(LOFI)
    moods["Aether"] = len(EXPERIMENTAL) + len(AETHER_EXTRAS)
    moods["Entangled"] = len(ENTANGLED)
    moods["Family"] = len(FAMILY_PRESETS)
    for mood, n in sorted(moods.items()):
        print(f"  {mood}: {n}")
    print(f"  TOTAL: {sum(moods.values())}")


if __name__ == "__main__":
    main()
