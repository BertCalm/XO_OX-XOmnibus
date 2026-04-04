#!/usr/bin/env python3
"""
generate_outlook_presets.py
---------------------------
Generates 92 factory presets for the Outlook panoramic pad/lead engine.

Outlook is a dual-wavetable synthesizer with horizon scanning, parallax stereo,
and a dual SVF "vista filter." 8 wave shapes per oscillator:
  0=Sine  1=Triangle  2=Saw  3=Square  4=Pulse  5=Super  6=Noise  7=Formant

horizonScan is INERT for wave shape 6 (Noise) — hold at default 0.5 for those.
For all other shapes, horizonScan is active and expressive.

Engine parameters (prefix: look_):
  look_waveShape1      [0–7 int]          Oscillator 1 wave shape
  look_waveShape2      [0–7 int]          Oscillator 2 wave shape
  look_oscMix          [0.0–1.0]          Osc 1/2 mix (0=full osc1, 1=full osc2)
  look_horizonScan     [0.0–1.0]          Horizon scan (inert for Noise waveform)
  look_parallaxAmount  [0.0–1.0]          Parallax stereo depth
  look_vistaLine       [0.0–1.0]          Vista filter "horizon line" (spectral aperture)
  look_resonance       [0.0–1.0]          Vista filter resonance
  look_filterEnvAmt    [0.0–1.0]          Filter envelope amount
  look_attack          [0.001–5.0 s]      Amp envelope attack
  look_decay           [0.001–5.0 s]      Amp envelope decay
  look_sustain         [0.0–1.0]          Amp envelope sustain
  look_release         [0.001–10.0 s]     Amp envelope release
  look_lfo1Rate        [0.01–20.0 Hz]     LFO 1 rate
  look_lfo1Depth       [0.0–1.0]          LFO 1 depth
  look_lfo2Rate        [0.01–20.0 Hz]     LFO 2 rate
  look_lfo2Depth       [0.0–1.0]          LFO 2 depth
  look_modWheelDepth   [0.0–1.0]          Mod wheel depth
  look_aftertouchDepth [0.0–1.0]          Aftertouch depth
  look_reverbMix       [0.0–1.0]          Reverb mix
  look_delayMix        [0.0–1.0]          Delay mix
  look_macroCharacter  [0.0–1.0]          Macro CHARACTER
  look_macroMovement   [0.0–1.0]          Macro MOVEMENT
  look_macroCoupling   [0.0–1.0]          Macro COUPLING
  look_macroSpace      [0.0–1.0]          Macro SPACE

Mood distribution (92 new presets):
  Ethereal:    20  (long release, parallax stereo, atmospheric)
  Luminous:    18  (panoramic sweeps, warm bright)
  Foundation:  15  (standard pad tones)
  Crystalline: 12  (high-register wavetable, narrow attack)
  Flux:        10  (horizon scanning, LFO-modulated)
  Deep:        10  (dark panoramic, sub-bass)
  Organic:      7  (natural, minimal processing)

8+ presets with aggression >= 0.7 (short attacks, high resonance).

Output: Presets/XOceanus/{mood}/Outlook/OUTLOOK_{name_slug}.xometa
"""

import json
import os
import re

REPO_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
PRESET_BASE = os.path.join(REPO_ROOT, "Presets", "XOceanus")

# Wave shape index reference (for comments in preset definitions)
# 0=Sine 1=Triangle 2=Saw 3=Square 4=Pulse 5=Super 6=Noise 7=Formant
SINE, TRIANGLE, SAW, SQUARE, PULSE, SUPER, NOISE, FORMANT = 0, 1, 2, 3, 4, 5, 6, 7
# horizonScan default when inert (both oscs = NOISE)
SCAN_DEFAULT = 0.5


def p(
    wave1, wave2, osc_mix, horizon_scan,
    parallax, vista_line, resonance, filter_env_amt,
    attack, decay, sustain, release,
    lfo1_rate, lfo1_depth, lfo2_rate, lfo2_depth,
    mod_wheel_depth, aftertouch_depth,
    reverb_mix, delay_mix,
    macro_character, macro_movement, macro_coupling, macro_space,
):
    """Return a fully-specified Outlook parameter dict (all 24 params).

    Rule: if BOTH oscillators are NOISE (6), horizonScan is forced to SCAN_DEFAULT.
    If only one osc is NOISE, the scan still affects the other — keep as specified.
    """
    # Clamp to valid ranges
    assert 0 <= wave1 <= 7, f"wave1 {wave1} out of [0,7]"
    assert 0 <= wave2 <= 7, f"wave2 {wave2} out of [0,7]"
    assert 0.001 <= attack <= 5.0, f"attack {attack} out of [0.001, 5.0]"
    assert 0.001 <= decay <= 5.0, f"decay {decay} out of [0.001, 5.0]"
    assert 0.001 <= release <= 10.0, f"release {release} out of [0.001, 10.0]"
    if wave1 == NOISE and wave2 == NOISE:
        horizon_scan = SCAN_DEFAULT

    return {
        "look_waveShape1": wave1,
        "look_waveShape2": wave2,
        "look_oscMix": osc_mix,
        "look_horizonScan": horizon_scan,
        "look_parallaxAmount": parallax,
        "look_vistaLine": vista_line,
        "look_resonance": resonance,
        "look_filterEnvAmt": filter_env_amt,
        "look_attack": attack,
        "look_decay": decay,
        "look_sustain": sustain,
        "look_release": release,
        "look_lfo1Rate": lfo1_rate,
        "look_lfo1Depth": lfo1_depth,
        "look_lfo2Rate": lfo2_rate,
        "look_lfo2Depth": lfo2_depth,
        "look_modWheelDepth": mod_wheel_depth,
        "look_aftertouchDepth": aftertouch_depth,
        "look_reverbMix": reverb_mix,
        "look_delayMix": delay_mix,
        "look_macroCharacter": macro_character,
        "look_macroMovement": macro_movement,
        "look_macroCoupling": macro_coupling,
        "look_macroSpace": macro_space,
    }


def dna(brightness, warmth, movement, density, space, aggression):
    return {
        "brightness": round(brightness, 3),
        "warmth": round(warmth, 3),
        "movement": round(movement, 3),
        "density": round(density, 3),
        "space": round(space, 3),
        "aggression": round(aggression, 3),
    }


def make_preset(
    name, mood, description, tags, params, sonic_dna,
    macro_labels=None, coupling_intensity="None", coupling_pairs=None,
):
    return {
        "schema_version": 1,
        "name": name,
        "mood": mood,
        "engines": ["Outlook"],
        "author": "Guru Bin",
        "version": "1.0.0",
        "description": description,
        "tags": tags,
        "macroLabels": macro_labels or ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"],
        "couplingIntensity": coupling_intensity,
        "tempo": None,
        "dna": sonic_dna,
        "parameters": {"Outlook": params},
        "coupling": {"pairs": coupling_pairs or []},
    }


def name_to_slug(name):
    """Convert preset name to filename-safe slug."""
    slug = re.sub(r"[^A-Za-z0-9 _]", "", name)
    slug = slug.replace(" ", "_")
    return slug


# =============================================================================
# PRESET DEFINITIONS
# =============================================================================

PRESETS = []


# ---------------------------------------------------------------------------
# ETHEREAL (20) — long release, high parallax, atmospheric, space-filling
# ---------------------------------------------------------------------------

PRESETS += [
    make_preset(
        "Parallax Dawn",
        "Ethereal",
        "The horizon at first light — sine and formant weave a slow-breathing aurora. Parallax spreads wide as notes climb higher. Best played slow.",
        ["atmospheric", "dawn", "parallax", "slow", "ethereal"],
        p(SINE, FORMANT, 0.45, 0.35,
          0.82, 0.72, 0.18, 0.3,
          1.8, 3.0, 0.75, 6.5,
          0.06, 0.22, 0.04, 0.15,
          0.65, 0.55, 0.68, 0.08,
          0.35, 0.15, 0.0, 0.72),
        dna(0.48, 0.55, 0.12, 0.32, 0.82, 0.08),
    ),
    make_preset(
        "Horizon Silk",
        "Ethereal",
        "Gossamer triangle-formant blend. The two oscillators breathe in slow cycles — one rising, one falling — creating perpetual silk shimmer.",
        ["silk", "shimmer", "triangle", "formant", "ethereal"],
        p(TRIANGLE, FORMANT, 0.5, 0.42,
          0.75, 0.68, 0.14, 0.25,
          2.2, 3.5, 0.8, 7.2,
          0.05, 0.18, 0.03, 0.12,
          0.6, 0.5, 0.72, 0.0,
          0.3, 0.1, 0.0, 0.75),
        dna(0.52, 0.5, 0.1, 0.28, 0.85, 0.06),
    ),
    make_preset(
        "Atmospheric Entry",
        "Ethereal",
        "Slow sine descent into the stratosphere. Low resonance, wide parallax, maximum reverb — the sound before landing.",
        ["atmospheric", "spacious", "entry", "wide", "sine"],
        p(SINE, SINE, 0.5, 0.2,
          0.88, 0.78, 0.1, 0.2,
          2.5, 4.0, 0.7, 8.0,
          0.04, 0.15, 0.07, 0.1,
          0.7, 0.45, 0.85, 0.0,
          0.25, 0.08, 0.0, 0.88),
        dna(0.4, 0.45, 0.08, 0.25, 0.92, 0.05),
    ),
    make_preset(
        "Stereo Cathedral",
        "Ethereal",
        "Super-saw chords with maximum parallax depth. High notes shimmer in opposite channels. Low notes anchor wide and grounded.",
        ["cathedral", "super", "stereo", "wide", "pad"],
        p(SUPER, SINE, 0.35, 0.65,
          0.9, 0.65, 0.22, 0.35,
          1.4, 2.5, 0.78, 7.0,
          0.07, 0.28, 0.05, 0.18,
          0.65, 0.58, 0.75, 0.05,
          0.42, 0.2, 0.0, 0.8),
        dna(0.55, 0.5, 0.18, 0.45, 0.88, 0.12),
    ),
    make_preset(
        "Celestial Arch",
        "Ethereal",
        "Formant pair with opposing scan directions. Two voices harmonically locking and separating — a celestial arch in frequency space.",
        ["formant", "celestial", "arch", "harmonic", "pad"],
        p(FORMANT, FORMANT, 0.5, 0.7,
          0.78, 0.62, 0.2, 0.28,
          1.6, 2.8, 0.72, 6.8,
          0.05, 0.2, 0.03, 0.14,
          0.6, 0.52, 0.7, 0.0,
          0.38, 0.12, 0.0, 0.76),
        dna(0.5, 0.52, 0.14, 0.35, 0.84, 0.08),
    ),
    make_preset(
        "Glacial Drift",
        "Ethereal",
        "Triangle into slow-scan saw — the ice shelf moving at imperceptible speed. 10-second release fades to white horizon.",
        ["glacial", "slow", "triangle", "saw", "drift"],
        p(TRIANGLE, SAW, 0.6, 0.15,
          0.7, 0.75, 0.12, 0.22,
          3.0, 4.5, 0.65, 9.5,
          0.03, 0.12, 0.05, 0.08,
          0.55, 0.4, 0.78, 0.0,
          0.2, 0.07, 0.0, 0.82),
        dna(0.38, 0.48, 0.07, 0.3, 0.9, 0.05),
    ),
    make_preset(
        "Aurora Veil Deep",
        "Ethereal",
        "Dense formant aurora — slow LFO conjunction creates breathing luminosity peaks. Parallax shifts the curtain left and right with register.",
        ["aurora", "formant", "luminous", "breathing", "veil"],
        p(FORMANT, TRIANGLE, 0.55, 0.55,
          0.82, 0.7, 0.16, 0.3,
          2.0, 3.2, 0.76, 7.5,
          0.04, 0.25, 0.06, 0.18,
          0.62, 0.55, 0.8, 0.0,
          0.32, 0.18, 0.0, 0.85),
        dna(0.52, 0.54, 0.15, 0.38, 0.87, 0.07),
    ),
    make_preset(
        "Memory Dissolve",
        "Ethereal",
        "Slow-attack sine with near-infinite sustain. The pad that exists before and after the note — memory outlasting the gesture.",
        ["memory", "sine", "slow-attack", "sustain", "dissolve"],
        p(SINE, TRIANGLE, 0.4, 0.25,
          0.72, 0.8, 0.08, 0.15,
          3.5, 5.0, 0.85, 8.5,
          0.03, 0.1, 0.04, 0.08,
          0.7, 0.48, 0.82, 0.0,
          0.22, 0.06, 0.0, 0.9),
        dna(0.42, 0.5, 0.06, 0.22, 0.93, 0.04),
    ),
    make_preset(
        "Vista Bloom",
        "Ethereal",
        "Sine oscillators with gently opening vista filter. The sound blooms like time-lapse petals — each note arrives closed and opens over 4 seconds.",
        ["bloom", "sine", "filter-env", "open", "organic"],
        p(SINE, SINE, 0.5, 0.3,
          0.68, 0.45, 0.15, 0.55,
          1.8, 3.0, 0.8, 6.0,
          0.05, 0.12, 0.07, 0.1,
          0.6, 0.5, 0.65, 0.0,
          0.35, 0.15, 0.0, 0.7),
        dna(0.5, 0.55, 0.12, 0.35, 0.78, 0.1),
    ),
    make_preset(
        "Cloud Architecture",
        "Ethereal",
        "Super and formant paired at equal mix — detuned shimmer carries formant vowel resonance. Wide parallax builds cloud-like spatial depth.",
        ["cloud", "super", "formant", "spatial", "pad"],
        p(SUPER, FORMANT, 0.5, 0.6,
          0.85, 0.68, 0.18, 0.25,
          1.5, 2.8, 0.74, 7.0,
          0.06, 0.22, 0.04, 0.15,
          0.65, 0.55, 0.78, 0.0,
          0.38, 0.18, 0.0, 0.82),
        dna(0.55, 0.5, 0.16, 0.42, 0.86, 0.08),
    ),
    make_preset(
        "Ether Column",
        "Ethereal",
        "Vertical pillar of pure sine tones in strict parallax. Low register earthbound, upper register dissolving into reverb sky.",
        ["column", "sine", "vertical", "parallax", "atmospheric"],
        p(SINE, SINE, 0.5, 0.1,
          0.95, 0.82, 0.06, 0.1,
          2.8, 4.2, 0.78, 9.0,
          0.04, 0.08, 0.03, 0.06,
          0.75, 0.5, 0.88, 0.0,
          0.15, 0.05, 0.0, 0.95),
        dna(0.35, 0.42, 0.05, 0.2, 0.95, 0.04),
    ),
    make_preset(
        "Luminous Ghost",
        "Ethereal",
        "Triangle-saw with half-lit vista. The sound arrives and departs like a ship's ghost — visible but not quite solid.",
        ["ghost", "triangle", "saw", "dim", "atmospheric"],
        p(TRIANGLE, SAW, 0.45, 0.4,
          0.75, 0.55, 0.2, 0.35,
          2.0, 3.5, 0.7, 7.8,
          0.05, 0.18, 0.04, 0.12,
          0.6, 0.5, 0.72, 0.0,
          0.3, 0.12, 0.0, 0.78),
        dna(0.45, 0.45, 0.12, 0.3, 0.85, 0.08),
    ),
    make_preset(
        "Slow Parallax",
        "Ethereal",
        "Minimal motion — near-maximum parallax with ultra-slow LFOs. The stereo field breathes once every 30 seconds.",
        ["slow", "parallax", "minimal", "breathing", "spatial"],
        p(SINE, FORMANT, 0.5, 0.22,
          0.92, 0.75, 0.08, 0.18,
          2.5, 4.0, 0.82, 8.0,
          0.02, 0.14, 0.015, 0.1,
          0.7, 0.5, 0.84, 0.0,
          0.18, 0.06, 0.0, 0.9),
        dna(0.4, 0.48, 0.07, 0.22, 0.92, 0.04),
    ),
    make_preset(
        "Spectral Shore",
        "Ethereal",
        "Formant pair scanning across the spectral horizon. Vowel resonances shift as notes move up — the shoreline recedes with altitude.",
        ["spectral", "formant", "shore", "vowel", "scan"],
        p(FORMANT, FORMANT, 0.5, 0.78,
          0.72, 0.6, 0.22, 0.32,
          1.4, 2.5, 0.72, 6.2,
          0.07, 0.25, 0.05, 0.18,
          0.62, 0.52, 0.68, 0.0,
          0.4, 0.18, 0.0, 0.75),
        dna(0.55, 0.5, 0.18, 0.4, 0.82, 0.1),
    ),
    make_preset(
        "Space Between",
        "Ethereal",
        "Pure sine with near-inaudible LFOs and maximum release. The sound occupies the space between notes — a pad made of silence's inverse.",
        ["space", "minimal", "sine", "silent", "between"],
        p(SINE, SINE, 0.5, 0.5,
          0.65, 0.85, 0.05, 0.08,
          3.8, 5.0, 0.9, 9.8,
          0.015, 0.06, 0.01, 0.04,
          0.65, 0.45, 0.9, 0.0,
          0.1, 0.04, 0.0, 0.95),
        dna(0.32, 0.4, 0.04, 0.18, 0.97, 0.03),
    ),
    make_preset(
        "Panoramic Haze",
        "Ethereal",
        "Super-saw with wide parallax and soft vista filter. Notes dissolve at edges — the haze at the edge of the panorama.",
        ["haze", "super", "panoramic", "wide", "soft"],
        p(SUPER, TRIANGLE, 0.45, 0.55,
          0.88, 0.62, 0.15, 0.28,
          1.8, 3.0, 0.76, 7.2,
          0.06, 0.2, 0.04, 0.14,
          0.62, 0.55, 0.76, 0.0,
          0.35, 0.15, 0.0, 0.82),
        dna(0.52, 0.5, 0.14, 0.38, 0.86, 0.08),
    ),
    make_preset(
        "Stratospheric Pad",
        "Ethereal",
        "Triangle at stratospheric register — slow attack, maximum parallax, near-silent LFOs. The pad that never fully arrives.",
        ["stratospheric", "triangle", "slow", "high", "register"],
        p(TRIANGLE, SINE, 0.55, 0.3,
          0.88, 0.72, 0.12, 0.2,
          3.2, 4.5, 0.75, 8.8,
          0.03, 0.1, 0.04, 0.08,
          0.68, 0.48, 0.85, 0.0,
          0.2, 0.07, 0.0, 0.9),
        dna(0.42, 0.45, 0.07, 0.25, 0.93, 0.05),
    ),
    make_preset(
        "Horizon Breath",
        "Ethereal",
        "One slow LFO conjunction cycle per phrase. When the two modulations align, the panorama opens briefly — then closes again.",
        ["breath", "lfo", "conjunction", "panorama", "slow"],
        p(SINE, FORMANT, 0.48, 0.45,
          0.78, 0.68, 0.14, 0.25,
          2.0, 3.2, 0.78, 7.0,
          0.05, 0.28, 0.08, 0.22,
          0.65, 0.55, 0.75, 0.0,
          0.3, 0.14, 0.0, 0.8),
        dna(0.45, 0.52, 0.15, 0.32, 0.86, 0.07),
    ),
    make_preset(
        "Translucent Field",
        "Ethereal",
        "Sine into formant at mid-mix. The sound is neither here nor there — transparent, occupying frequency without mass.",
        ["translucent", "sine", "formant", "transparent", "field"],
        p(SINE, FORMANT, 0.5, 0.38,
          0.8, 0.74, 0.1, 0.2,
          2.2, 3.8, 0.8, 7.5,
          0.04, 0.15, 0.05, 0.1,
          0.68, 0.5, 0.8, 0.0,
          0.25, 0.1, 0.0, 0.85),
        dna(0.45, 0.48, 0.1, 0.28, 0.88, 0.06),
    ),
    make_preset(
        "Infinite Recession",
        "Ethereal",
        "Maximum release, minimum attack. The pad that takes 10 seconds to fully arrive and another 10 to leave — temporal sculpture.",
        ["infinite", "slow", "release", "sculpture", "temporal"],
        p(TRIANGLE, FORMANT, 0.5, 0.28,
          0.85, 0.78, 0.08, 0.15,
          4.5, 5.0, 0.88, 10.0,
          0.02, 0.08, 0.03, 0.06,
          0.72, 0.48, 0.88, 0.0,
          0.12, 0.04, 0.0, 0.95),
        dna(0.38, 0.45, 0.05, 0.2, 0.96, 0.03),
    ),
]

# ---------------------------------------------------------------------------
# LUMINOUS (18) — panoramic sweeps, warm bright, hopeful, open vistas
# ---------------------------------------------------------------------------

PRESETS += [
    make_preset(
        "Horizon Line",
        "Luminous",
        "The moment the sun clears the horizon — sine sweeping to full brightness on horizon scan. Warm parallax opens from center to edge.",
        ["horizon", "bright", "opening", "warm", "sunrise"],
        p(SINE, TRIANGLE, 0.45, 0.7,
          0.7, 0.8, 0.2, 0.45,
          0.4, 1.5, 0.8, 3.5,
          0.15, 0.3, 0.08, 0.2,
          0.7, 0.6, 0.45, 0.12,
          0.55, 0.35, 0.0, 0.55),
        dna(0.72, 0.62, 0.28, 0.45, 0.65, 0.15),
    ),
    make_preset(
        "Warm Panorama",
        "Luminous",
        "Super-saw with reduced detuning and golden vista filter. Brightness calibrated to match a late-afternoon sun angle.",
        ["warm", "panorama", "super", "golden", "afternoon"],
        p(SUPER, SAW, 0.5, 0.55,
          0.62, 0.75, 0.25, 0.4,
          0.5, 1.8, 0.75, 4.0,
          0.12, 0.32, 0.08, 0.2,
          0.68, 0.58, 0.4, 0.08,
          0.55, 0.3, 0.0, 0.5),
        dna(0.68, 0.65, 0.25, 0.52, 0.6, 0.18),
    ),
    make_preset(
        "Golden Sweep",
        "Luminous",
        "Slow horizon scan creating continuous spectral sweep. Triangle morphing toward saw as the scan opens — warm golden progression.",
        ["golden", "sweep", "triangle", "saw", "scan"],
        p(TRIANGLE, SAW, 0.55, 0.85,
          0.58, 0.7, 0.22, 0.42,
          0.6, 2.0, 0.72, 3.8,
          0.1, 0.28, 0.06, 0.18,
          0.65, 0.55, 0.42, 0.1,
          0.52, 0.28, 0.0, 0.48),
        dna(0.7, 0.6, 0.3, 0.5, 0.58, 0.2),
    ),
    make_preset(
        "Bright Veil",
        "Luminous",
        "High vista line with moderate resonance — a bright, slightly cutting pad. Sine purity in luminous condition.",
        ["bright", "veil", "sine", "cutting", "luminous"],
        p(SINE, SUPER, 0.4, 0.45,
          0.65, 0.88, 0.28, 0.38,
          0.8, 2.2, 0.78, 4.5,
          0.1, 0.25, 0.07, 0.16,
          0.65, 0.55, 0.35, 0.1,
          0.55, 0.3, 0.0, 0.45),
        dna(0.78, 0.55, 0.22, 0.45, 0.55, 0.22),
    ),
    make_preset(
        "Vista Sunrise",
        "Luminous",
        "Sequential LFOs at coprime rates open the panorama on a slowly breathing rhythm. The sound of morning arriving in stages.",
        ["sunrise", "lfo", "coprime", "breathing", "morning"],
        p(TRIANGLE, FORMANT, 0.5, 0.6,
          0.68, 0.78, 0.18, 0.35,
          0.7, 2.0, 0.76, 4.2,
          0.13, 0.35, 0.09, 0.22,
          0.68, 0.58, 0.5, 0.08,
          0.5, 0.28, 0.0, 0.6),
        dna(0.65, 0.58, 0.3, 0.42, 0.65, 0.15),
    ),
    make_preset(
        "Luminous Crown",
        "Luminous",
        "Sine-formant at bright vista — a crown of harmonic light. The formant vowel adds warmth without sacrificing clarity.",
        ["crown", "sine", "formant", "bright", "harmonic"],
        p(SINE, FORMANT, 0.42, 0.52,
          0.6, 0.85, 0.2, 0.35,
          0.5, 1.8, 0.78, 3.8,
          0.12, 0.28, 0.08, 0.18,
          0.68, 0.58, 0.38, 0.05,
          0.58, 0.3, 0.0, 0.5),
        dna(0.75, 0.6, 0.25, 0.42, 0.6, 0.18),
    ),
    make_preset(
        "Open Sky Lead",
        "Luminous",
        "Lead texture built on saw with brightness scan. Medium attack defines the note — not quite pad, not quite lead. Luminous open sky.",
        ["lead", "saw", "medium", "open", "sky"],
        p(SAW, TRIANGLE, 0.6, 0.65,
          0.45, 0.82, 0.3, 0.48,
          0.25, 1.2, 0.65, 2.8,
          0.18, 0.32, 0.1, 0.22,
          0.72, 0.62, 0.25, 0.12,
          0.62, 0.38, 0.0, 0.38),
        dna(0.78, 0.55, 0.35, 0.5, 0.5, 0.28),
    ),
    make_preset(
        "Day Break Chord",
        "Luminous",
        "Super-saw chord voice with luminous parallax. Slight detune gives chords air — this is the pad under the melody at sunrise.",
        ["daybreak", "super", "chord", "detune", "parallel"],
        p(SUPER, SINE, 0.45, 0.48,
          0.72, 0.78, 0.22, 0.38,
          0.6, 2.0, 0.78, 4.0,
          0.1, 0.28, 0.07, 0.18,
          0.65, 0.58, 0.45, 0.05,
          0.52, 0.28, 0.0, 0.58),
        dna(0.7, 0.58, 0.25, 0.5, 0.62, 0.2),
    ),
    make_preset(
        "Amber Horizon",
        "Luminous",
        "Warm triangle with mid-scan amber brightness. The filter sits slightly open — spectral amber catching late-day angle.",
        ["amber", "triangle", "warm", "afternoon", "filter"],
        p(TRIANGLE, SAW, 0.52, 0.42,
          0.58, 0.72, 0.25, 0.38,
          0.7, 2.2, 0.76, 4.0,
          0.1, 0.25, 0.07, 0.15,
          0.65, 0.55, 0.42, 0.08,
          0.5, 0.28, 0.0, 0.52),
        dna(0.65, 0.65, 0.22, 0.48, 0.6, 0.18),
    ),
    make_preset(
        "Luminous Sweep",
        "Luminous",
        "Maximum horizon scan movement on sine — full spectral sweep from fundamental to full harmonic stack. Bright and moving.",
        ["sweep", "sine", "maximum", "harmonic", "movement"],
        p(SINE, SINE, 0.5, 0.95,
          0.62, 0.8, 0.18, 0.35,
          0.6, 1.8, 0.75, 3.5,
          0.15, 0.35, 0.1, 0.22,
          0.7, 0.6, 0.38, 0.08,
          0.55, 0.32, 0.0, 0.5),
        dna(0.75, 0.55, 0.35, 0.45, 0.58, 0.2),
    ),
    make_preset(
        "Coastal Shimmer",
        "Luminous",
        "Super-saw coastal sound — wide parallax with bright formant shimmer adding sparkle above. The light on ocean from shore.",
        ["coastal", "shimmer", "super", "formant", "ocean"],
        p(SUPER, FORMANT, 0.45, 0.58,
          0.78, 0.82, 0.2, 0.32,
          0.55, 1.8, 0.78, 3.8,
          0.12, 0.3, 0.08, 0.2,
          0.68, 0.6, 0.48, 0.08,
          0.52, 0.3, 0.0, 0.6),
        dna(0.72, 0.58, 0.28, 0.5, 0.65, 0.18),
    ),
    make_preset(
        "Bright Meridian",
        "Luminous",
        "Noon-position pad — maximum brightness without harshness. Sine at full vista line, parallax at mid position. Confident and clean.",
        ["noon", "bright", "confident", "clean", "meridian"],
        p(SINE, TRIANGLE, 0.45, 0.38,
          0.55, 0.92, 0.15, 0.28,
          0.5, 1.5, 0.8, 3.5,
          0.12, 0.22, 0.08, 0.14,
          0.7, 0.58, 0.3, 0.05,
          0.6, 0.32, 0.0, 0.42),
        dna(0.82, 0.55, 0.22, 0.42, 0.52, 0.2),
    ),
    make_preset(
        "Sunlit Expanse",
        "Luminous",
        "Formant pair in bright position. Scan sweeping both voices creates a luminous vowel expanse — an open landscape of harmonic light.",
        ["sunlit", "formant", "expanse", "bright", "vowel"],
        p(FORMANT, FORMANT, 0.5, 0.68,
          0.68, 0.85, 0.18, 0.3,
          0.65, 2.0, 0.78, 4.0,
          0.1, 0.28, 0.07, 0.18,
          0.68, 0.58, 0.45, 0.05,
          0.52, 0.28, 0.0, 0.58),
        dna(0.7, 0.58, 0.25, 0.45, 0.62, 0.18),
    ),
    make_preset(
        "Vista Blanc",
        "Luminous",
        "White-spectrum super-saw at high brightness. Luminous without harshness — the visual equivalent of a perfectly exposed sky.",
        ["white", "super", "bright", "exposed", "airy"],
        p(SUPER, SUPER, 0.5, 0.72,
          0.65, 0.88, 0.18, 0.32,
          0.55, 1.8, 0.76, 3.8,
          0.1, 0.3, 0.07, 0.2,
          0.68, 0.58, 0.38, 0.05,
          0.5, 0.28, 0.0, 0.52),
        dna(0.8, 0.5, 0.28, 0.5, 0.58, 0.2),
    ),
    make_preset(
        "Prism Shimmer Lead",
        "Luminous",
        "Saw lead tone with luminous parallax enhancement. Cuts through mix while maintaining spatial depth — the lead that illuminates.",
        ["lead", "prism", "saw", "cut", "luminous"],
        p(SAW, SINE, 0.65, 0.6,
          0.5, 0.85, 0.28, 0.45,
          0.2, 1.0, 0.7, 2.5,
          0.2, 0.3, 0.12, 0.2,
          0.72, 0.62, 0.22, 0.15,
          0.65, 0.42, 0.0, 0.35),
        dna(0.82, 0.5, 0.38, 0.55, 0.45, 0.32),
    ),
    make_preset(
        "Luminous Thread",
        "Luminous",
        "Sine thread at bright position — minimal density, maximum clarity. The single note that illuminates a progression.",
        ["thread", "sine", "single", "clarity", "minimal"],
        p(SINE, SINE, 0.5, 0.32,
          0.48, 0.88, 0.12, 0.25,
          0.4, 1.5, 0.78, 3.2,
          0.1, 0.18, 0.07, 0.12,
          0.68, 0.55, 0.28, 0.05,
          0.58, 0.3, 0.0, 0.38),
        dna(0.8, 0.52, 0.2, 0.38, 0.5, 0.2),
    ),
    make_preset(
        "Golden Parallax",
        "Luminous",
        "Maximum parallax at luminous filter position. High notes split wide; low notes stay grounded. The stereo field is the melody.",
        ["golden", "parallax", "maximum", "stereo", "width"],
        p(TRIANGLE, FORMANT, 0.48, 0.62,
          0.95, 0.78, 0.2, 0.35,
          0.6, 2.0, 0.78, 4.0,
          0.1, 0.28, 0.07, 0.18,
          0.68, 0.6, 0.55, 0.08,
          0.5, 0.3, 0.0, 0.68),
        dna(0.68, 0.6, 0.25, 0.45, 0.72, 0.18),
    ),
    make_preset(
        "Bright Expanse",
        "Luminous",
        "Saw-super combination at bright vista. Wide, warm, slightly aggressive. The pad that owns the mix without apologizing.",
        ["bright", "saw", "super", "expanse", "bold"],
        p(SAW, SUPER, 0.5, 0.7,
          0.68, 0.82, 0.28, 0.42,
          0.55, 1.8, 0.75, 3.8,
          0.14, 0.32, 0.09, 0.22,
          0.7, 0.62, 0.35, 0.1,
          0.58, 0.35, 0.0, 0.5),
        dna(0.75, 0.58, 0.3, 0.55, 0.58, 0.25),
    ),
]

# ---------------------------------------------------------------------------
# FOUNDATION (15) — archetypal pad tones, bread-and-butter Outlook sounds
# ---------------------------------------------------------------------------

PRESETS += [
    make_preset(
        "Vista Pad",
        "Foundation",
        "The fundamental Outlook pad — triangle and sine at balanced mix, moderate parallax, medium vista. Every other preset traces back to this.",
        ["foundation", "neutral", "triangle", "sine", "balanced"],
        p(TRIANGLE, SINE, 0.5, 0.5,
          0.5, 0.7, 0.3, 0.5,
          0.1, 0.3, 0.7, 0.8,
          0.5, 0.3, 0.3, 0.2,
          0.5, 0.5, 0.3, 0.0,
          0.5, 0.0, 0.0, 0.3),
        dna(0.55, 0.55, 0.2, 0.45, 0.55, 0.15),
    ),
    make_preset(
        "Horizon Standard",
        "Foundation",
        "Clean pad — saw and triangle at 60/40 mix with standard envelope. The production workhorse for panoramic pads.",
        ["standard", "saw", "triangle", "production", "clean"],
        p(SAW, TRIANGLE, 0.4, 0.5,
          0.55, 0.7, 0.28, 0.45,
          0.15, 0.5, 0.72, 1.2,
          0.4, 0.28, 0.25, 0.18,
          0.55, 0.5, 0.32, 0.05,
          0.5, 0.15, 0.0, 0.35),
        dna(0.6, 0.55, 0.25, 0.5, 0.52, 0.18),
    ),
    make_preset(
        "Pad One",
        "Foundation",
        "Sine pad at default settings — teaches the engine's natural voice. All parameters near center. Modify from here.",
        ["init", "sine", "default", "center", "template"],
        p(SINE, SINE, 0.5, 0.5,
          0.5, 0.7, 0.3, 0.5,
          0.1, 0.3, 0.7, 0.8,
          0.5, 0.3, 0.3, 0.2,
          0.5, 0.5, 0.3, 0.0,
          0.5, 0.0, 0.0, 0.3),
        dna(0.5, 0.5, 0.2, 0.4, 0.5, 0.15),
    ),
    make_preset(
        "Mid Horizon",
        "Foundation",
        "Super-saw at medium everything — the mid-position panoramic chord pad. Dense but not overwhelming, luminous but not harsh.",
        ["mid", "super", "balanced", "chord", "density"],
        p(SUPER, TRIANGLE, 0.5, 0.5,
          0.5, 0.7, 0.25, 0.4,
          0.3, 0.8, 0.72, 1.5,
          0.35, 0.28, 0.22, 0.18,
          0.55, 0.5, 0.32, 0.05,
          0.5, 0.15, 0.0, 0.35),
        dna(0.6, 0.52, 0.22, 0.52, 0.52, 0.18),
    ),
    make_preset(
        "Panoramic Lead",
        "Foundation",
        "Saw lead with parallax enabled — brighter than a pad, narrower than an arp. The lead that needs space.",
        ["lead", "saw", "parallax", "single", "focused"],
        p(SAW, SAW, 0.5, 0.55,
          0.45, 0.75, 0.3, 0.48,
          0.08, 0.4, 0.68, 1.0,
          0.5, 0.28, 0.3, 0.18,
          0.6, 0.55, 0.2, 0.1,
          0.55, 0.25, 0.0, 0.3),
        dna(0.68, 0.5, 0.28, 0.5, 0.48, 0.25),
    ),
    make_preset(
        "Standard Vista",
        "Foundation",
        "Triangle with moderate filter envelope and standard ADSR. Behaves predictably in any context — the universal Outlook preset.",
        ["standard", "triangle", "filter-env", "universal", "reliable"],
        p(TRIANGLE, TRIANGLE, 0.5, 0.5,
          0.5, 0.68, 0.3, 0.48,
          0.15, 0.5, 0.7, 1.0,
          0.4, 0.25, 0.25, 0.15,
          0.55, 0.5, 0.3, 0.05,
          0.5, 0.15, 0.0, 0.35),
        dna(0.55, 0.55, 0.22, 0.45, 0.52, 0.18),
    ),
    make_preset(
        "Formant Core",
        "Foundation",
        "Formant-sine blend as a functional pad core. The vowel resonance gives character without stealing focus. Sits under melodies.",
        ["formant", "core", "pad", "vowel", "foundational"],
        p(FORMANT, SINE, 0.45, 0.5,
          0.55, 0.7, 0.25, 0.4,
          0.2, 0.6, 0.72, 1.2,
          0.3, 0.22, 0.2, 0.15,
          0.55, 0.5, 0.32, 0.0,
          0.5, 0.15, 0.0, 0.38),
        dna(0.55, 0.58, 0.2, 0.45, 0.55, 0.15),
    ),
    make_preset(
        "Dual Horizon",
        "Foundation",
        "Two oscillators in opposite scan positions — maximum spectral interference at mid-filter. Clean but complex, never muddy.",
        ["dual", "interference", "scan", "complex", "clean"],
        p(SAW, TRIANGLE, 0.5, 0.5,
          0.5, 0.7, 0.28, 0.45,
          0.2, 0.6, 0.7, 1.2,
          0.4, 0.28, 0.25, 0.18,
          0.55, 0.5, 0.3, 0.05,
          0.5, 0.15, 0.0, 0.35),
        dna(0.6, 0.52, 0.25, 0.5, 0.55, 0.18),
    ),
    make_preset(
        "Pulse Pad",
        "Foundation",
        "Pulse oscillator pad — adjustable width via horizon scan creates evolving hollow chamber. Warm at low scan, thin at high.",
        ["pulse", "hollow", "scan", "width", "evolving"],
        p(PULSE, SINE, 0.55, 0.5,
          0.5, 0.7, 0.3, 0.45,
          0.2, 0.6, 0.7, 1.2,
          0.4, 0.28, 0.25, 0.18,
          0.55, 0.5, 0.3, 0.05,
          0.5, 0.15, 0.0, 0.35),
        dna(0.58, 0.52, 0.22, 0.48, 0.52, 0.18),
    ),
    make_preset(
        "Super Foundation",
        "Foundation",
        "Three-unison super-saw as the foundation chord texture. Slight scan detune separation keeps it from muddying.",
        ["super", "unison", "chord", "detuned", "foundation"],
        p(SUPER, SINE, 0.45, 0.48,
          0.55, 0.7, 0.25, 0.4,
          0.25, 0.7, 0.72, 1.3,
          0.35, 0.28, 0.22, 0.18,
          0.55, 0.5, 0.32, 0.05,
          0.5, 0.18, 0.0, 0.38),
        dna(0.62, 0.55, 0.22, 0.55, 0.55, 0.18),
    ),
    make_preset(
        "Horizon Chord",
        "Foundation",
        "Saw-square combination for rich chord texture. Square narrows to bite; saw carries the body. Panoramic width essential.",
        ["chord", "saw", "square", "rich", "texture"],
        p(SAW, SQUARE, 0.5, 0.55,
          0.6, 0.7, 0.28, 0.42,
          0.2, 0.6, 0.7, 1.2,
          0.4, 0.28, 0.25, 0.18,
          0.55, 0.5, 0.3, 0.05,
          0.5, 0.18, 0.0, 0.38),
        dna(0.62, 0.55, 0.25, 0.52, 0.55, 0.2),
    ),
    make_preset(
        "Foundation Sweep",
        "Foundation",
        "Slow horizon scan across saw and triangle. The pad that changes over time without using LFOs — pure oscillator scanning.",
        ["sweep", "slow", "scan", "saw", "organic"],
        p(SAW, TRIANGLE, 0.5, 0.78,
          0.55, 0.7, 0.25, 0.42,
          0.2, 0.6, 0.72, 1.5,
          0.3, 0.22, 0.2, 0.15,
          0.55, 0.5, 0.32, 0.05,
          0.5, 0.2, 0.0, 0.38),
        dna(0.62, 0.52, 0.28, 0.5, 0.55, 0.2),
    ),
    make_preset(
        "Anchor Pad",
        "Foundation",
        "Low-register pad — sine with closed vista line. Grounding, harmonic, the anchor beneath complex textures.",
        ["anchor", "low", "grounding", "sine", "harmonic"],
        p(SINE, TRIANGLE, 0.4, 0.3,
          0.48, 0.42, 0.28, 0.45,
          0.15, 0.5, 0.75, 1.2,
          0.4, 0.22, 0.25, 0.15,
          0.55, 0.5, 0.28, 0.0,
          0.5, 0.15, 0.0, 0.32),
        dna(0.42, 0.65, 0.2, 0.5, 0.5, 0.18),
    ),
    make_preset(
        "Vista Minor",
        "Foundation",
        "Minor-flavored pad — slightly darker vista with warm triangle. Moody without being dramatic. Sits under vocal leads.",
        ["minor", "dark", "triangle", "moody", "vocal"],
        p(TRIANGLE, SINE, 0.5, 0.38,
          0.5, 0.55, 0.28, 0.42,
          0.2, 0.6, 0.72, 1.2,
          0.4, 0.25, 0.25, 0.15,
          0.55, 0.5, 0.3, 0.0,
          0.48, 0.15, 0.0, 0.35),
        dna(0.48, 0.58, 0.22, 0.45, 0.52, 0.18),
    ),
    make_preset(
        "Classic Outlook",
        "Foundation",
        "The most direct expression of Outlook's identity — triangle at center scan, medium parallax, moderate reverb. The engine's calling card.",
        ["classic", "identity", "triangle", "balanced", "direct"],
        p(TRIANGLE, FORMANT, 0.5, 0.5,
          0.55, 0.7, 0.3, 0.5,
          0.1, 0.3, 0.7, 0.8,
          0.5, 0.3, 0.3, 0.2,
          0.5, 0.5, 0.3, 0.0,
          0.5, 0.0, 0.0, 0.3),
        dna(0.55, 0.55, 0.2, 0.45, 0.55, 0.15),
    ),
]

# ---------------------------------------------------------------------------
# CRYSTALLINE (12) — high-register, sharp attacks, glassy harmonic content
# ---------------------------------------------------------------------------

PRESETS += [
    make_preset(
        "Crystal Horizon",
        "Crystalline",
        "Sine oscillators at high vista line — sharp attack reveals bright harmonic partials. Decays like fractured ice.",
        ["crystal", "sine", "bright", "sharp", "ice"],
        p(SINE, SINE, 0.5, 0.65,
          0.55, 0.92, 0.35, 0.42,
          0.008, 0.4, 0.55, 1.5,
          0.8, 0.3, 0.5, 0.2,
          0.55, 0.48, 0.25, 0.05,
          0.62, 0.2, 0.0, 0.35),
        dna(0.82, 0.42, 0.32, 0.35, 0.45, 0.28),
    ),
    make_preset(
        "Glass Lattice",
        "Crystalline",
        "High-resonance formant pair — the filter rings like struck glass. Fast attack, medium decay creates bell-like attack transients.",
        ["glass", "formant", "bell", "resonant", "sharp"],
        p(FORMANT, SINE, 0.4, 0.72,
          0.6, 0.88, 0.45, 0.38,
          0.006, 0.5, 0.45, 1.8,
          0.7, 0.25, 0.45, 0.18,
          0.52, 0.45, 0.22, 0.08,
          0.62, 0.22, 0.0, 0.38),
        dna(0.78, 0.38, 0.28, 0.32, 0.48, 0.32),
    ),
    make_preset(
        "Frost Prism",
        "Crystalline",
        "Triangle at full brightness scan — maximum harmonic blending creates prismatic frost texture. Attack crystallizes into decay.",
        ["frost", "triangle", "prism", "scan", "harmonic"],
        p(TRIANGLE, SAW, 0.5, 0.92,
          0.58, 0.9, 0.4, 0.38,
          0.005, 0.45, 0.5, 1.6,
          0.75, 0.28, 0.48, 0.2,
          0.55, 0.48, 0.22, 0.05,
          0.65, 0.22, 0.0, 0.35),
        dna(0.85, 0.38, 0.3, 0.35, 0.45, 0.3),
    ),
    make_preset(
        "Diamond Vista",
        "Crystalline",
        "High-register super-saw with maximum scan — the diamond-bright shimmer at the top of the frequency range.",
        ["diamond", "super", "high", "bright", "shimmer"],
        p(SUPER, FORMANT, 0.42, 0.85,
          0.62, 0.92, 0.38, 0.35,
          0.008, 0.4, 0.5, 1.5,
          0.8, 0.28, 0.5, 0.2,
          0.55, 0.48, 0.25, 0.08,
          0.65, 0.22, 0.0, 0.4),
        dna(0.88, 0.38, 0.3, 0.42, 0.48, 0.3),
    ),
    make_preset(
        "Icefield",
        "Crystalline",
        "Sine pair with narrow attack and long crystalline decay. Wide parallax mimics the vast spatial perception of ice fields.",
        ["ice", "field", "vast", "parallel", "cold"],
        p(SINE, TRIANGLE, 0.45, 0.45,
          0.82, 0.88, 0.3, 0.3,
          0.01, 0.8, 0.45, 2.5,
          0.65, 0.22, 0.42, 0.15,
          0.55, 0.48, 0.35, 0.0,
          0.55, 0.18, 0.0, 0.5),
        dna(0.78, 0.32, 0.2, 0.32, 0.62, 0.25),
    ),
    make_preset(
        "Sharp Meridian",
        "Crystalline",
        "Aggressive preset for Crystalline mood — high resonance, fast attack, saw oscillators. The crystalline edge.",
        ["sharp", "aggressive", "saw", "resonance", "edge"],
        p(SAW, SQUARE, 0.55, 0.75,
          0.42, 0.88, 0.65, 0.52,
          0.003, 0.3, 0.45, 0.8,
          1.2, 0.42, 0.8, 0.28,
          0.72, 0.65, 0.15, 0.15,
          0.75, 0.38, 0.0, 0.25),
        dna(0.88, 0.35, 0.42, 0.48, 0.35, 0.78),
    ),
    make_preset(
        "Vista Quartz",
        "Crystalline",
        "Quartz-pure formant at high vista — sine product formula creates a pristine, factored brightness.",
        ["quartz", "formant", "pure", "pristine", "bright"],
        p(FORMANT, FORMANT, 0.5, 0.8,
          0.55, 0.9, 0.38, 0.35,
          0.006, 0.45, 0.5, 1.6,
          0.7, 0.25, 0.45, 0.18,
          0.55, 0.48, 0.22, 0.05,
          0.62, 0.22, 0.0, 0.38),
        dna(0.82, 0.4, 0.28, 0.38, 0.48, 0.28),
    ),
    make_preset(
        "Faceted Light",
        "Crystalline",
        "Super-saw with high vista and scan — each unison voice facets the light differently. Crystalline multi-angle shimmer.",
        ["faceted", "super", "shimmer", "light", "multi"],
        p(SUPER, SINE, 0.4, 0.78,
          0.6, 0.9, 0.35, 0.32,
          0.008, 0.4, 0.5, 1.5,
          0.8, 0.28, 0.5, 0.2,
          0.55, 0.48, 0.25, 0.05,
          0.62, 0.22, 0.0, 0.42),
        dna(0.85, 0.38, 0.3, 0.45, 0.48, 0.28),
    ),
    make_preset(
        "Pulse Crystal",
        "Crystalline",
        "Pulse at narrow width via high scan — thin, crystalline strike. Fast attack creates percussive crystalline hits.",
        ["pulse", "narrow", "percussive", "strike", "thin"],
        p(PULSE, FORMANT, 0.5, 0.88,
          0.55, 0.88, 0.42, 0.38,
          0.004, 0.35, 0.42, 1.2,
          0.9, 0.32, 0.6, 0.22,
          0.58, 0.5, 0.2, 0.08,
          0.65, 0.25, 0.0, 0.32),
        dna(0.85, 0.35, 0.35, 0.42, 0.42, 0.35),
    ),
    make_preset(
        "Snow Lattice",
        "Crystalline",
        "Triangle-formant at bright position with wide parallax. The geometric precision of snow crystal structure as sound.",
        ["snow", "geometric", "lattice", "triangle", "formant"],
        p(TRIANGLE, FORMANT, 0.5, 0.62,
          0.75, 0.88, 0.32, 0.3,
          0.008, 0.5, 0.5, 1.8,
          0.7, 0.22, 0.45, 0.15,
          0.55, 0.48, 0.28, 0.0,
          0.58, 0.2, 0.0, 0.45),
        dna(0.8, 0.38, 0.25, 0.38, 0.55, 0.28),
    ),
    make_preset(
        "Arctic Glass",
        "Crystalline",
        "Cold crystalline pad — near-maximum vista brightness, zero warmth, precision parallax. Arctic glass sound.",
        ["arctic", "cold", "glass", "precision", "bright"],
        p(SINE, SUPER, 0.45, 0.55,
          0.7, 0.92, 0.28, 0.28,
          0.01, 0.6, 0.48, 2.0,
          0.6, 0.2, 0.4, 0.14,
          0.52, 0.45, 0.3, 0.0,
          0.58, 0.18, 0.0, 0.45),
        dna(0.82, 0.3, 0.22, 0.35, 0.58, 0.28),
    ),
    make_preset(
        "Crystal Attack",
        "Crystalline",
        "Aggressive crystal texture — short envelope, maximum attack transient, high resonance. Crystal as weapon.",
        ["aggressive", "attack", "short", "transient", "weapon"],
        p(SAW, TRIANGLE, 0.55, 0.82,
          0.48, 0.88, 0.58, 0.48,
          0.002, 0.25, 0.4, 0.6,
          1.5, 0.42, 1.0, 0.3,
          0.72, 0.65, 0.12, 0.12,
          0.72, 0.42, 0.0, 0.22),
        dna(0.88, 0.35, 0.45, 0.5, 0.35, 0.82),
    ),
]

# ---------------------------------------------------------------------------
# FLUX (10) — LFO-modulated, scanning, kinetic, unstable panoramas
# ---------------------------------------------------------------------------

PRESETS += [
    make_preset(
        "Horizon Scanner",
        "Flux",
        "Fast coprime LFOs modulate the vista filter while horizon scan sweeps continuously. The panorama in permanent motion.",
        ["scanner", "lfo", "coprime", "motion", "filter"],
        p(TRIANGLE, SAW, 0.5, 0.72,
          0.6, 0.6, 0.4, 0.55,
          0.05, 0.4, 0.6, 1.5,
          1.2, 0.5, 0.79, 0.35,
          0.7, 0.6, 0.2, 0.1,
          0.4, 0.6, 0.0, 0.28),
        dna(0.62, 0.5, 0.72, 0.52, 0.42, 0.28),
    ),
    make_preset(
        "Turbulent Sky",
        "Flux",
        "Square and saw in flux — pulse-width and brightness simultaneously modulated. Unstable weather system as synthesis.",
        ["turbulent", "square", "saw", "weather", "unstable"],
        p(SQUARE, SAW, 0.5, 0.65,
          0.55, 0.62, 0.38, 0.5,
          0.06, 0.4, 0.58, 1.5,
          1.5, 0.55, 0.97, 0.38,
          0.7, 0.62, 0.18, 0.1,
          0.4, 0.65, 0.0, 0.25),
        dna(0.65, 0.48, 0.75, 0.55, 0.4, 0.32),
    ),
    make_preset(
        "Vista Noir",
        "Flux",
        "Dark, restless scanning — nearly closed filter pumping from fast LFO. Low brightness scan reveals deep oscillator modulations.",
        ["noir", "dark", "pumping", "fast", "restless"],
        p(SAW, TRIANGLE, 0.5, 0.55,
          0.5, 0.42, 0.48, 0.6,
          0.04, 0.35, 0.55, 1.2,
          2.0, 0.65, 1.3, 0.45,
          0.72, 0.65, 0.15, 0.12,
          0.38, 0.7, 0.0, 0.22),
        dna(0.42, 0.5, 0.78, 0.58, 0.38, 0.38),
    ),
    make_preset(
        "LFO Panorama",
        "Flux",
        "Two LFOs at near-identical rates creating slow beat frequency. The parallax slowly rotates as the beat cycle completes.",
        ["lfo", "beat", "rotation", "cycle", "panoramic"],
        p(TRIANGLE, FORMANT, 0.5, 0.6,
          0.68, 0.65, 0.35, 0.5,
          0.06, 0.4, 0.62, 1.5,
          0.95, 0.48, 1.05, 0.38,
          0.65, 0.58, 0.25, 0.08,
          0.42, 0.55, 0.0, 0.35),
        dna(0.58, 0.52, 0.68, 0.5, 0.48, 0.25),
    ),
    make_preset(
        "Thermal Scan",
        "Flux",
        "Horizon scan automation-ready preset — scan at high position, LFOs creating thermal column instabilities.",
        ["thermal", "scan", "column", "instability", "automation"],
        p(TRIANGLE, SAW, 0.5, 0.88,
          0.55, 0.58, 0.42, 0.55,
          0.05, 0.4, 0.58, 1.4,
          1.4, 0.52, 0.88, 0.38,
          0.68, 0.62, 0.18, 0.1,
          0.4, 0.62, 0.0, 0.25),
        dna(0.6, 0.5, 0.72, 0.55, 0.42, 0.3),
    ),
    make_preset(
        "Unstable Horizon",
        "Flux",
        "Pulse-width modulation on square oscillator with fast LFO — the horizon that cannot hold still.",
        ["unstable", "pulse", "square", "modulation", "fast"],
        p(SQUARE, SINE, 0.55, 0.72,
          0.52, 0.62, 0.4, 0.52,
          0.05, 0.38, 0.58, 1.3,
          1.8, 0.58, 1.1, 0.42,
          0.7, 0.62, 0.15, 0.1,
          0.38, 0.65, 0.0, 0.22),
        dna(0.62, 0.48, 0.75, 0.55, 0.38, 0.32),
    ),
    make_preset(
        "Aurora Motion",
        "Flux",
        "Aurora-like flux — LFO conjunction creates brightness peaks in irregular rhythm. The northern lights as kinetic sound.",
        ["aurora", "conjunction", "flux", "kinetic", "irregular"],
        p(SINE, FORMANT, 0.48, 0.65,
          0.68, 0.68, 0.32, 0.48,
          0.06, 0.45, 0.65, 1.8,
          0.82, 0.52, 0.59, 0.38,
          0.68, 0.6, 0.28, 0.05,
          0.42, 0.58, 0.0, 0.38),
        dna(0.62, 0.52, 0.68, 0.5, 0.5, 0.25),
    ),
    make_preset(
        "Sweep Storm",
        "Flux",
        "Super-saw with maximum horizon scan speed — the sweep storm, three detuned saws all scanning simultaneously.",
        ["storm", "super", "sweep", "maximum", "detuned"],
        p(SUPER, SAW, 0.5, 0.82,
          0.58, 0.6, 0.38, 0.52,
          0.05, 0.4, 0.6, 1.5,
          1.6, 0.55, 1.0, 0.4,
          0.72, 0.65, 0.2, 0.1,
          0.42, 0.68, 0.0, 0.28),
        dna(0.65, 0.5, 0.78, 0.58, 0.42, 0.32),
    ),
    make_preset(
        "Kinetic Panorama",
        "Flux",
        "Saw and super-saw in flux — constant motion through the panoramic field. Wide parallax catches every movement.",
        ["kinetic", "saw", "super", "motion", "wide"],
        p(SAW, SUPER, 0.5, 0.75,
          0.72, 0.62, 0.38, 0.52,
          0.05, 0.4, 0.62, 1.5,
          1.3, 0.52, 0.82, 0.38,
          0.7, 0.62, 0.22, 0.1,
          0.4, 0.62, 0.0, 0.32),
        dna(0.62, 0.5, 0.75, 0.55, 0.45, 0.3),
    ),
    make_preset(
        "Dynamic Swell",
        "Flux",
        "Slow LFO-modulated swell — the panorama expands and contracts on a 12-second cycle. Macro MOVEMENT opens the scan amplitude.",
        ["swell", "slow", "expand", "contract", "cycle"],
        p(TRIANGLE, FORMANT, 0.5, 0.55,
          0.65, 0.65, 0.28, 0.42,
          0.8, 2.0, 0.72, 3.5,
          0.08, 0.42, 0.12, 0.32,
          0.65, 0.58, 0.32, 0.05,
          0.4, 0.45, 0.0, 0.45),
        dna(0.55, 0.52, 0.52, 0.48, 0.52, 0.2),
    ),
]

# ---------------------------------------------------------------------------
# DEEP (10) — dark panoramic, sub-bass present, heavy resonance, subterranean
# ---------------------------------------------------------------------------

PRESETS += [
    make_preset(
        "Dark Panorama",
        "Deep",
        "Low-register sine pad — vista line nearly closed, warm resonance, slow breathing. The panorama at night.",
        ["dark", "low", "warm", "night", "closed"],
        p(SINE, TRIANGLE, 0.5, 0.25,
          0.62, 0.28, 0.38, 0.45,
          0.5, 1.8, 0.72, 3.0,
          0.08, 0.28, 0.05, 0.2,
          0.55, 0.5, 0.25, 0.0,
          0.35, 0.15, 0.0, 0.38),
        dna(0.25, 0.72, 0.2, 0.55, 0.5, 0.15),
    ),
    make_preset(
        "Subterranean Vista",
        "Deep",
        "Sub-register panoramic — deep below the horizon line. Heavy low resonance, near-closed filter, cavernous parallax.",
        ["sub", "cavernous", "low", "resonant", "underground"],
        p(SINE, SINE, 0.5, 0.2,
          0.7, 0.22, 0.45, 0.5,
          0.8, 2.5, 0.68, 4.0,
          0.06, 0.22, 0.04, 0.15,
          0.5, 0.45, 0.22, 0.0,
          0.3, 0.12, 0.0, 0.45),
        dna(0.2, 0.75, 0.18, 0.58, 0.55, 0.12),
    ),
    make_preset(
        "Trench Sound",
        "Deep",
        "Deep ocean trench texture — triangle with full sub-bass presence, dark vista, wide parallax. Depth as space.",
        ["trench", "deep", "bass", "dark", "ocean"],
        p(TRIANGLE, SAW, 0.45, 0.28,
          0.75, 0.25, 0.42, 0.48,
          0.6, 2.0, 0.7, 3.5,
          0.07, 0.25, 0.05, 0.18,
          0.5, 0.45, 0.28, 0.0,
          0.32, 0.15, 0.0, 0.5),
        dna(0.22, 0.78, 0.2, 0.62, 0.58, 0.15),
    ),
    make_preset(
        "Abyss Pad",
        "Deep",
        "Super-saw deep — detuned mass at low register, minimum brightness. Impenetrable wall of deep panoramic sound.",
        ["abyss", "super", "wall", "impenetrable", "mass"],
        p(SUPER, SINE, 0.5, 0.35,
          0.68, 0.2, 0.45, 0.42,
          0.8, 2.5, 0.72, 4.0,
          0.06, 0.2, 0.04, 0.14,
          0.5, 0.45, 0.25, 0.0,
          0.32, 0.12, 0.0, 0.42),
        dna(0.2, 0.78, 0.18, 0.65, 0.55, 0.12),
    ),
    make_preset(
        "Shadow Panorama",
        "Deep",
        "Formant at deep position — vowel resonances in the sub-bass register. Voices from below the horizon.",
        ["shadow", "formant", "vowel", "deep", "sub-bass"],
        p(FORMANT, TRIANGLE, 0.45, 0.3,
          0.65, 0.25, 0.42, 0.45,
          0.7, 2.2, 0.7, 3.8,
          0.07, 0.22, 0.05, 0.15,
          0.5, 0.45, 0.28, 0.0,
          0.32, 0.15, 0.0, 0.48),
        dna(0.22, 0.75, 0.2, 0.6, 0.55, 0.15),
    ),
    make_preset(
        "Deep Scan",
        "Deep",
        "Horizon scan active in deep register — dark spectral sweep from fundamental upward. The vista opens slowly from below.",
        ["deep", "scan", "spectral", "dark", "sweep"],
        p(SAW, TRIANGLE, 0.5, 0.72,
          0.62, 0.3, 0.45, 0.5,
          0.5, 1.8, 0.7, 3.2,
          0.08, 0.28, 0.05, 0.18,
          0.52, 0.48, 0.28, 0.05,
          0.35, 0.2, 0.0, 0.45),
        dna(0.3, 0.7, 0.25, 0.6, 0.52, 0.2),
    ),
    make_preset(
        "Night Horizon",
        "Deep",
        "Dark formant pair with near-minimum vista — the horizon at the darkest point of night. Wide, cold, and far.",
        ["night", "formant", "cold", "dark", "distant"],
        p(FORMANT, FORMANT, 0.5, 0.42,
          0.72, 0.22, 0.38, 0.42,
          1.0, 3.0, 0.68, 4.5,
          0.05, 0.18, 0.035, 0.12,
          0.5, 0.45, 0.3, 0.0,
          0.28, 0.12, 0.0, 0.52),
        dna(0.2, 0.72, 0.15, 0.58, 0.62, 0.12),
    ),
    make_preset(
        "Pressure Pad",
        "Deep",
        "High resonance at deep register — water pressure as sound. The filter ring is felt before it is heard.",
        ["pressure", "resonance", "felt", "physical", "deep"],
        p(SINE, SAW, 0.45, 0.28,
          0.62, 0.3, 0.65, 0.55,
          0.6, 2.0, 0.68, 3.5,
          0.08, 0.25, 0.05, 0.18,
          0.52, 0.48, 0.3, 0.0,
          0.35, 0.18, 0.0, 0.45),
        dna(0.28, 0.72, 0.22, 0.65, 0.52, 0.2),
    ),
    make_preset(
        "Foundation Deep",
        "Deep",
        "Triangle deep pad — architectural, load-bearing. The sound that holds everything else up.",
        ["foundation", "triangle", "architectural", "bass", "support"],
        p(TRIANGLE, SINE, 0.4, 0.22,
          0.65, 0.28, 0.38, 0.42,
          0.5, 1.8, 0.72, 3.2,
          0.06, 0.2, 0.04, 0.14,
          0.5, 0.45, 0.25, 0.0,
          0.3, 0.12, 0.0, 0.42),
        dna(0.22, 0.78, 0.18, 0.62, 0.55, 0.12),
    ),
    make_preset(
        "Abyss Resonance",
        "Deep",
        "Aggressive deep preset — high resonance filter sweep on sub-bass. Growling, physical, pressure-wave aggression.",
        ["aggressive", "resonance", "growl", "bass", "physical"],
        p(SAW, SQUARE, 0.5, 0.55,
          0.58, 0.32, 0.72, 0.62,
          0.3, 1.2, 0.65, 2.5,
          0.15, 0.42, 0.1, 0.28,
          0.65, 0.62, 0.25, 0.1,
          0.42, 0.38, 0.0, 0.35),
        dna(0.32, 0.68, 0.38, 0.68, 0.45, 0.72),
    ),
]

# ---------------------------------------------------------------------------
# ORGANIC (7) — natural, minimal processing, acoustic-flavored, unpolished
# ---------------------------------------------------------------------------

PRESETS += [
    make_preset(
        "Natural Horizon",
        "Organic",
        "Triangle at minimal processing — no scan, minimal LFO, dry reverb. The panoramic pad without technology.",
        ["natural", "minimal", "triangle", "acoustic", "dry"],
        p(TRIANGLE, SINE, 0.5, 0.12,
          0.38, 0.65, 0.2, 0.35,
          0.05, 0.5, 0.7, 1.2,
          0.15, 0.12, 0.1, 0.08,
          0.45, 0.4, 0.18, 0.0,
          0.45, 0.1, 0.0, 0.25),
        dna(0.45, 0.62, 0.12, 0.38, 0.45, 0.1),
    ),
    make_preset(
        "Breath of Vista",
        "Organic",
        "Formant at organic position — vowel resonance without processing. Sounds like a singer sustaining a single note.",
        ["breath", "formant", "vocal", "organic", "unprocessed"],
        p(FORMANT, SINE, 0.42, 0.3,
          0.42, 0.62, 0.18, 0.3,
          0.08, 0.6, 0.72, 1.5,
          0.12, 0.15, 0.08, 0.1,
          0.45, 0.4, 0.2, 0.0,
          0.42, 0.12, 0.0, 0.28),
        dna(0.48, 0.65, 0.12, 0.38, 0.45, 0.1),
    ),
    make_preset(
        "Wooden Vista",
        "Organic",
        "Triangle at warm position — analog warmth, slightly closed vista. The pad that sounds like it was recorded rather than synthesized.",
        ["wooden", "warm", "analog", "recorded", "natural"],
        p(TRIANGLE, TRIANGLE, 0.5, 0.2,
          0.45, 0.55, 0.22, 0.35,
          0.1, 0.6, 0.72, 1.5,
          0.12, 0.12, 0.08, 0.08,
          0.45, 0.42, 0.2, 0.0,
          0.45, 0.1, 0.0, 0.28),
        dna(0.42, 0.68, 0.1, 0.4, 0.45, 0.1),
    ),
    make_preset(
        "Raw Parallax",
        "Organic",
        "Super-saw without polish — the natural stereo field of three voices without parallax enhancement. Raw panoramic texture.",
        ["raw", "super", "natural", "unpolished", "stereo"],
        p(SUPER, TRIANGLE, 0.5, 0.38,
          0.35, 0.65, 0.22, 0.35,
          0.12, 0.6, 0.72, 1.5,
          0.15, 0.15, 0.1, 0.1,
          0.45, 0.42, 0.22, 0.0,
          0.48, 0.12, 0.0, 0.28),
        dna(0.52, 0.58, 0.12, 0.48, 0.42, 0.12),
    ),
    make_preset(
        "Field Recording",
        "Organic",
        "Sine-noise blend — minimal noise component adds presence without texture. Suggests recording artifacts without replicating them.",
        ["field", "noise", "presence", "analog", "recording"],
        p(SINE, NOISE, 0.85, SCAN_DEFAULT,
          0.42, 0.65, 0.18, 0.3,
          0.08, 0.6, 0.72, 1.5,
          0.12, 0.1, 0.08, 0.07,
          0.45, 0.4, 0.2, 0.0,
          0.42, 0.1, 0.0, 0.28),
        dna(0.45, 0.6, 0.1, 0.38, 0.45, 0.1),
    ),
    make_preset(
        "Organic Scan",
        "Organic",
        "Slow horizon scan on formant — the organic character of voice resonances moving through the spectrum. Unpredictable, alive.",
        ["scan", "formant", "organic", "alive", "vocal"],
        p(FORMANT, TRIANGLE, 0.48, 0.55,
          0.42, 0.6, 0.2, 0.32,
          0.08, 0.6, 0.7, 1.5,
          0.1, 0.18, 0.07, 0.12,
          0.45, 0.42, 0.22, 0.0,
          0.42, 0.12, 0.0, 0.3),
        dna(0.48, 0.62, 0.15, 0.4, 0.45, 0.1),
    ),
    make_preset(
        "Vista Earth",
        "Organic",
        "Triangle and sine, minimal effects, low attack. The panoramic synth pad of the Earth — fundamental, unadorned, permanent.",
        ["earth", "minimal", "permanent", "triangle", "fundamental"],
        p(TRIANGLE, SINE, 0.5, 0.15,
          0.4, 0.6, 0.18, 0.3,
          0.08, 0.6, 0.75, 1.5,
          0.1, 0.1, 0.07, 0.07,
          0.42, 0.38, 0.18, 0.0,
          0.4, 0.08, 0.0, 0.25),
        dna(0.42, 0.68, 0.1, 0.38, 0.42, 0.08),
    ),
]

# ---------------------------------------------------------------------------
# 8+ AGGRESSION PRESETS — verify the count below
# Aggressive presets (aggression >= 0.7) are distributed across moods:
#   Crystalline: Sharp Meridian (0.78), Crystal Attack (0.82)
#   Deep: Abyss Resonance (0.72)
#   Luminous: Prism Shimmer Lead (0.32) — not aggressive enough; see below
# Additional aggressive presets added to hit 8+ target:
# ---------------------------------------------------------------------------

# Check: we have Sharp Meridian (0.78), Crystal Attack (0.82), Abyss Resonance (0.72)
# We need 5 more at >= 0.70. Adding to appropriate moods:

PRESETS += [
    # Flux aggressive
    make_preset(
        "Razor Sweep",
        "Flux",
        "High-speed scan on saw oscillators — fast attack, high resonance, aggressive filter pumping. Danger at the horizon.",
        ["razor", "aggressive", "saw", "fast", "resonance"],
        p(SAW, SAW, 0.5, 0.88,
          0.48, 0.7, 0.75, 0.62,
          0.003, 0.2, 0.5, 0.8,
          3.0, 0.65, 2.0, 0.48,
          0.8, 0.72, 0.12, 0.15,
          0.72, 0.75, 0.0, 0.18),
        dna(0.75, 0.42, 0.82, 0.62, 0.28, 0.82),
    ),
    # Foundation aggressive
    make_preset(
        "Attack Vista",
        "Foundation",
        "Immediate attack, saw oscillators, high resonance. The Outlook engine at its most confrontational — panoramic aggression.",
        ["attack", "aggressive", "saw", "confrontational", "panoramic"],
        p(SAW, SQUARE, 0.5, 0.72,
          0.48, 0.8, 0.72, 0.58,
          0.003, 0.25, 0.6, 1.0,
          2.0, 0.55, 1.3, 0.42,
          0.78, 0.72, 0.12, 0.15,
          0.7, 0.68, 0.0, 0.2),
        dna(0.78, 0.42, 0.58, 0.62, 0.3, 0.78),
    ),
    # Luminous aggressive
    make_preset(
        "Bright Strike",
        "Luminous",
        "Fast attack, high brightness — a luminous percussion hit with long panoramic tail. Aggressive entry, beautiful decay.",
        ["bright", "aggressive", "fast", "strike", "lead"],
        p(SAW, TRIANGLE, 0.6, 0.78,
          0.5, 0.88, 0.62, 0.52,
          0.003, 0.35, 0.55, 2.0,
          1.5, 0.45, 0.95, 0.32,
          0.78, 0.68, 0.18, 0.12,
          0.72, 0.55, 0.0, 0.3),
        dna(0.85, 0.42, 0.55, 0.55, 0.42, 0.78),
    ),
    # Ethereal aggressive (short attack, high resonance)
    make_preset(
        "Shear Veil",
        "Ethereal",
        "Aggressive ethereal — high resonance with wide parallax and long release. The brutal note that decays into beauty.",
        ["shear", "aggressive", "resonant", "ethereal", "brutal"],
        p(TRIANGLE, SUPER, 0.5, 0.62,
          0.82, 0.72, 0.78, 0.55,
          0.008, 0.5, 0.6, 5.0,
          0.5, 0.35, 0.35, 0.25,
          0.72, 0.65, 0.6, 0.05,
          0.55, 0.4, 0.0, 0.72),
        dna(0.62, 0.48, 0.35, 0.55, 0.72, 0.72),
    ),
    # Deep aggressive
    make_preset(
        "Seismic Vista",
        "Deep",
        "Sub-bass resonance at maximum aggression — high filter Q at low frequency creates seismic vibration. Physical, threatening.",
        ["seismic", "sub-bass", "resonance", "physical", "deep"],
        p(SINE, SAW, 0.45, 0.32,
          0.58, 0.2, 0.82, 0.68,
          0.2, 0.8, 0.65, 2.0,
          0.2, 0.48, 0.13, 0.35,
          0.72, 0.68, 0.2, 0.1,
          0.42, 0.48, 0.0, 0.32),
        dna(0.3, 0.68, 0.48, 0.72, 0.42, 0.82),
    ),
]

# =============================================================================
# VALIDATE AND WRITE PRESETS
# =============================================================================

def validate_preset(preset):
    """Basic sanity checks on a preset before writing."""
    name = preset["name"]
    params = preset["parameters"]["Outlook"]
    dna_vals = preset["dna"]

    # Check all 24 params present
    required_params = [
        "look_waveShape1", "look_waveShape2", "look_oscMix", "look_horizonScan",
        "look_parallaxAmount", "look_vistaLine", "look_resonance", "look_filterEnvAmt",
        "look_attack", "look_decay", "look_sustain", "look_release",
        "look_lfo1Rate", "look_lfo1Depth", "look_lfo2Rate", "look_lfo2Depth",
        "look_modWheelDepth", "look_aftertouchDepth",
        "look_reverbMix", "look_delayMix",
        "look_macroCharacter", "look_macroMovement", "look_macroCoupling", "look_macroSpace",
    ]
    for rp in required_params:
        assert rp in params, f"{name}: missing param {rp}"

    # Range checks
    for k, v in params.items():
        if "waveShape" in k:
            assert 0 <= v <= 7, f"{name}: {k}={v} out of [0,7]"
        elif k in ("look_attack", "look_decay"):
            assert 0.001 <= v <= 5.0, f"{name}: {k}={v} out of range"
        elif k == "look_release":
            assert 0.001 <= v <= 10.0, f"{name}: {k}={v} out of range"
        elif k == "look_lfo1Rate" or k == "look_lfo2Rate":
            assert 0.01 <= v <= 20.0, f"{name}: {k}={v} out of [0.01,20.0]"
        elif k not in ("look_waveShape1", "look_waveShape2"):
            assert 0.0 <= v <= 1.0, f"{name}: {k}={v} out of [0,1]"

    # DNA checks
    for dna_key in ["brightness", "warmth", "movement", "density", "space", "aggression"]:
        v = dna_vals[dna_key]
        assert 0.0 <= v <= 1.0, f"{name}: DNA {dna_key}={v} out of [0,1]"

    # Noise scan check — if BOTH oscs are noise, scan must be 0.5
    if params["look_waveShape1"] == NOISE and params["look_waveShape2"] == NOISE:
        assert params["look_horizonScan"] == SCAN_DEFAULT, \
            f"{name}: both oscs NOISE but horizonScan != {SCAN_DEFAULT}"


def write_preset(preset):
    mood = preset["mood"]
    name = preset["name"]
    slug = name_to_slug(name)
    filename = f"OUTLOOK_{slug}.xometa"
    out_dir = os.path.join(PRESET_BASE, mood, "Outlook")
    os.makedirs(out_dir, exist_ok=True)
    filepath = os.path.join(out_dir, filename)
    with open(filepath, "w", encoding="utf-8") as f:
        json.dump(preset, f, indent=2)
        f.write("\n")
    return filepath


if __name__ == "__main__":
    # Validate all presets
    print(f"Validating {len(PRESETS)} presets...")
    for preset in PRESETS:
        validate_preset(preset)
    print("All presets valid.")

    # Count by mood
    mood_counts = {}
    for preset in PRESETS:
        mood = preset["mood"]
        mood_counts[mood] = mood_counts.get(mood, 0) + 1

    print("\nMood distribution:")
    for mood, count in sorted(mood_counts.items()):
        print(f"  {mood:15s}: {count:3d}")
    print(f"  {'TOTAL':15s}: {len(PRESETS):3d}")

    # Count aggressive presets
    aggressive = [p for p in PRESETS if p["dna"]["aggression"] >= 0.7]
    print(f"\nPresets with aggression >= 0.7: {len(aggressive)}")
    for ap in aggressive:
        print(f"  {ap['name']:30s}  ({ap['mood']})  agg={ap['dna']['aggression']}")

    # Check for duplicate names
    names = [p["name"] for p in PRESETS]
    seen = set()
    dupes = []
    for n in names:
        if n in seen:
            dupes.append(n)
        seen.add(n)
    if dupes:
        print(f"\nWARNING: Duplicate names: {dupes}")
    else:
        print("\nNo duplicate names.")

    # Write all presets
    print("\nWriting presets...")
    written = 0
    for preset in PRESETS:
        path = write_preset(preset)
        written += 1

    print(f"\nDone. Wrote {written} presets.")
    print(f"Output: {PRESET_BASE}/{{mood}}/Outlook/OUTLOOK_{{name}}.xometa")
