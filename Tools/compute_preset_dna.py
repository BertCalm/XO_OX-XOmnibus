#!/usr/bin/env python3
"""
XOmnibus Sonic DNA Fingerprint Generator

Computes a 6-dimensional "DNA" vector for every .xometa preset:
  [brightness, warmth, movement, density, space, aggression]

Each dimension is 0.0–1.0. The DNA enables:
  - Find Similar / Find Opposite browsing
  - Preset morphing (interpolation between DNA vectors)
  - Preset breeding (genetic crossover + mutation)
  - 2D mood map visualization

Usage:
    python3 compute_preset_dna.py [--dry-run] [--report]
"""

import json
import math
import sys
from pathlib import Path

PRESET_DIR = Path(__file__).parent.parent / "Presets" / "XOmnibus"

# ---------------------------------------------------------------------------
# Utility
# ---------------------------------------------------------------------------

def clamp(v, lo=0.0, hi=1.0):
    return max(lo, min(hi, v))

def norm_freq(hz, lo=20.0, hi=20000.0):
    """Normalize a frequency (Hz) to 0.0–1.0 using log scale."""
    if hz <= lo: return 0.0
    if hz >= hi: return 1.0
    return math.log(hz / lo) / math.log(hi / lo)

def norm_range(val, lo, hi):
    """Normalize a value within a known range to 0.0–1.0."""
    if hi <= lo: return 0.5
    return clamp((val - lo) / (hi - lo))

def softmax(*values, scale=1.0):
    """Average multiple 0-1 signals, clamped."""
    if not values: return 0.0
    return clamp(sum(values) / len(values) * scale)

# ---------------------------------------------------------------------------
# Engine-specific DNA extractors
# ---------------------------------------------------------------------------

def dna_xoddcouple(p: dict) -> dict:
    """Compute DNA for OddfeliX presets."""
    # Brightness: filter cutoffs, presence of high modes
    x_cut = norm_freq(p.get("xFilterCutoff", 5000))
    o_cut = norm_freq(p.get("oFilterCutoff", 4000))
    brightness = clamp((x_cut + o_cut) / 2 * 1.1)

    # Warmth: coupling, sub, drift, bloom
    coupling = p.get("couplingAmount", 0.5)
    sub = p.get("oSub", 0.3)
    drift = p.get("oDrift", 0.2)
    bloom = norm_range(p.get("oBloom", 1.0), 0, 5)
    warmth = softmax(coupling * 0.6, sub, drift * 1.5, bloom, scale=1.2)

    # Movement: morph, coupling as modulation, delay wobble
    morph = norm_range(p.get("oMorph", 1.0), 0, 5)
    macro3 = p.get("macro3", 0.5)
    wobble = p.get("delayWobble", 0.2)
    movement = softmax(morph, macro3 * 0.8, wobble * 2, scale=1.1)

    # Density: polyphony, unison, detune, dual-engine levels
    x_uni = min(p.get("xUnison", 1), 4) / 4
    o_poly = min(p.get("oPolyphony", 3), 8) / 8
    detune = norm_range(p.get("xDetune", 10) + p.get("oDetune", 10), 0, 50)
    x_lvl = p.get("xLevel", 0.8)
    o_lvl = p.get("oLevel", 0.8)
    density = softmax(x_uni, o_poly, detune, (x_lvl + o_lvl) / 2, scale=1.0)

    # Space: reverb + delay
    rev_mix = p.get("reverbMix", 0.3)
    rev_size = norm_range(p.get("reverbSize", 0.8), 0, 2)
    del_mix = p.get("delayMix", 0.3)
    del_fb = p.get("delayFeedback", 0.5)
    space = softmax(rev_mix * 1.5, rev_size, del_mix * 1.3, del_fb * 0.8, scale=1.1)

    # Aggression: compressor, snap, resonance
    comp_ratio = norm_range(p.get("compRatio", 4), 1, 20)
    snap = p.get("xSnap", 0.5)
    x_reso = p.get("xFilterReso", 0.3)
    o_reso = p.get("oFilterReso", 0.3)
    aggression = softmax(comp_ratio * 0.7, snap * 0.8, x_reso, o_reso, scale=1.0)

    return {
        "brightness": round(brightness, 3),
        "warmth":     round(warmth, 3),
        "movement":   round(movement, 3),
        "density":    round(density, 3),
        "space":      round(space, 3),
        "aggression": round(aggression, 3),
    }


def dna_xoverdub(p: dict) -> dict:
    """Compute DNA for XOverdub presets."""
    # Brightness: filter cutoff
    brightness = norm_freq(p.get("filter_cutoff", 8000))

    # Warmth: sub, drift, low cutoff contributes
    sub = p.get("osc_sub_level", 0)
    drift = p.get("osc_drift", 0.05)
    low_cut = 1.0 - norm_freq(p.get("filter_cutoff", 8000))
    warmth = softmax(sub * 1.5, drift * 3, low_cut * 0.5, scale=1.0)

    # Movement: LFO depth, delay wow/wear
    lfo_d = p.get("lfo_depth", 0)
    wow = p.get("delay_wow", 0.15)
    wear = p.get("delay_wear", 0.3)
    movement = softmax(lfo_d * 2, wow * 2, wear * 1.5, scale=1.1)

    # Density: sub + noise + osc levels
    noise = p.get("osc_noise_level", 0)
    osc_lvl = p.get("osc_level", 0.8)
    density = softmax(sub * 1.5, noise * 2, osc_lvl, scale=0.9)

    # Space: send level, reverb, delay feedback
    send = p.get("send_level", 0.5)
    ret = p.get("return_level", 0.7)
    rev_mix = p.get("reverb_mix", 0.3)
    rev_size = p.get("reverb_size", 0.4)
    del_fb = p.get("delay_feedback", 0.5)
    space = softmax(send * ret, rev_mix * 1.5, rev_size, del_fb * 0.8, scale=1.2)

    # Aggression: drive, high resonance, self-oscillating feedback
    drive = norm_range(p.get("drive_amount", 1), 1, 10)
    reso = p.get("filter_resonance", 0)
    fb_excess = max(0, p.get("delay_feedback", 0.5) - 0.8) * 5  # self-oscillation
    aggression = softmax(drive * 1.5, reso * 1.3, fb_excess, scale=1.2)

    return {
        "brightness": round(brightness, 3),
        "warmth":     round(warmth, 3),
        "movement":   round(movement, 3),
        "density":    round(density, 3),
        "space":      round(space, 3),
        "aggression": round(aggression, 3),
    }


def dna_xodyssey(p: dict) -> dict:
    """Compute DNA for XOdyssey presets."""
    # Brightness: filter cutoff, shimmer
    cut = norm_freq(p.get("filt_a_cutoff", 4000))
    shimmer = p.get("shimmer_amount", 0) * 1.3
    shimmer_tone = p.get("shimmer_tone", 0.5)
    brightness = softmax(cut, shimmer * shimmer_tone, scale=1.1)

    # Warmth: haze saturation, drift, sub, formant mix
    haze = p.get("haze_amount", 0)
    drift = p.get("drift_depth", 0)
    sub = p.get("sub_level", 0)
    formant = p.get("filt_b_mix", 0)
    warmth = softmax(haze * 1.5, drift * 2, sub * 1.3, formant * 0.8, scale=1.1)

    # Movement: tidal, drift rate, LFO, fracture
    tidal = p.get("tidal_depth", 0)
    drift_r = p.get("drift_rate", 0)
    # Collect LFO depths
    lfo_movement = 0
    for i in range(1, 4):
        lfo_movement += p.get(f"lfo_{i}_depth", 0)
    fracture = 1.0 if p.get("fracture_enable", 0) > 0 else 0
    frac_int = p.get("fracture_intensity", 0)
    movement = softmax(tidal * 2, drift_r * 3, lfo_movement * 0.8,
                       fracture * frac_int * 1.5, scale=1.0)

    # Density: both osc active, detuning, supersaw mode
    a_lvl = p.get("osc_a_level", 0.7)
    b_lvl = p.get("osc_b_level", 0)
    a_det = p.get("osc_a_detune", 0)
    b_det = p.get("osc_b_detune", 0)
    is_supersaw = 1.0 if p.get("osc_a_mode", 0) == 2 or p.get("osc_b_mode", 0) == 2 else 0
    density = softmax(a_lvl * 0.5, b_lvl * 0.8, a_det * 1.5, b_det * 1.5,
                      is_supersaw * 0.6, sub * 0.5, scale=1.0)

    # Space: reverb, delay, chorus, phaser
    rev_mix = p.get("reverb_mix", 0)
    rev_size = p.get("reverb_size", 0)
    del_mix = p.get("delay_mix", 0)
    chorus = p.get("chorus_mix", 0) if p.get("chorus_enable", 0) else 0
    phaser = p.get("phaser_mix", 0) if p.get("phaser_enable", 0) else 0
    space = softmax(rev_mix * 1.5, rev_size, del_mix * 1.3, chorus, phaser * 0.8, scale=1.1)

    # Aggression: haze heavy, resonance, fracture, distortion
    reso = p.get("filt_a_reso", 0)
    drive = p.get("filt_a_drive", 0)
    aggression = softmax(haze * 1.2 if haze > 0.3 else 0,
                         reso * 1.5, drive * 1.5,
                         fracture * frac_int, scale=1.1)

    return {
        "brightness": round(brightness, 3),
        "warmth":     round(warmth, 3),
        "movement":   round(movement, 3),
        "density":    round(density, 3),
        "space":      round(space, 3),
        "aggression": round(aggression, 3),
    }


def dna_xoblongbob(p: dict) -> dict:
    """Compute DNA for XOblong presets."""
    # Brightness: filter cutoff, character
    cut = norm_freq(p.get("flt_cutoff", 3000))
    char = p.get("flt_character", 0)
    brightness = softmax(cut, char * 0.5, scale=1.1)

    # Warmth: drift, bob_mode, low cutoff, space
    drift = p.get("oscA_drift", 0)
    bob = p.get("bob_mode", 0)
    space_mix = p.get("space_mix", 0)
    low_cut = 1.0 - norm_freq(p.get("flt_cutoff", 3000))
    warmth = softmax(drift * 2, bob * 1.5, space_mix * 0.5, low_cut * 0.3, scale=1.1)

    # Movement: curiosity system, LFOs, motion envelope
    cur = p.get("cur_amount", 0)
    cur_active = 1.0 if p.get("cur_mode", -1) >= 0 else 0
    lfo1 = p.get("lfo1_depth", 0)
    lfo2 = p.get("lfo2_depth", 0)
    mot_depth = p.get("motEnv_depth", 0)
    movement = softmax(cur * cur_active * 2, lfo1 * 1.5, lfo2 * 1.5,
                       mot_depth * 1.3, scale=1.0)

    # Density: oscB blend, detune, texture
    blend = p.get("oscB_blend", 0)
    detune = norm_range(p.get("oscB_detune", 0), 0, 20)
    tex = p.get("tex_level", 0)
    fm = p.get("oscB_fm", 0)
    density = softmax(blend * 1.3, detune, tex * 1.5, fm * 1.5, scale=1.0)

    # Space: space reverb, smear chorus
    sp_mix = p.get("space_mix", 0)
    sp_size = p.get("space_size", 0)
    smear = p.get("smear_mix", 0)
    space = softmax(sp_mix * 1.5, sp_size, smear * 1.3, scale=1.1)

    # Aggression: resonance, drive, dust, stutter
    reso = p.get("flt_resonance", 0)
    flt_drv = p.get("flt_drive", 0)
    dust = p.get("dust_amount", 0)
    stutter = p.get("stutter_depth", 0)
    aggression = softmax(reso * 1.3, flt_drv * 1.5, dust * 1.5, stutter * 2, scale=1.0)

    return {
        "brightness": round(brightness, 3),
        "warmth":     round(warmth, 3),
        "movement":   round(movement, 3),
        "density":    round(density, 3),
        "space":      round(space, 3),
        "aggression": round(aggression, 3),
    }


def dna_oddoscar(p: dict) -> dict:
    """Compute DNA for OddOscar presets (morph_ prefix)."""
    brightness = norm_freq(p.get("morph_filterCutoff", 5000))
    warmth = softmax(p.get("morph_drift", 0.2), p.get("morph_sub", 0.3),
                     p.get("morph_saturation", 0.1) * 1.5, scale=1.1)
    morph = p.get("morph_morph", 0.5)
    lfo = p.get("morph_lfoDepth", 0.3)
    movement = softmax(morph * 1.5, lfo * 1.3, p.get("morph_modDepth", 0.2), scale=1.1)
    density = softmax(p.get("morph_unison", 0.25), p.get("morph_detune", 0.2),
                      p.get("morph_oscLevel", 0.8), scale=1.0)
    space = softmax(p.get("morph_reverbMix", 0.3) * 1.5,
                    p.get("morph_delayMix", 0.2) * 1.3,
                    p.get("morph_reverbSize", 0.4), scale=1.1)
    aggression = softmax(p.get("morph_filterReso", 0.3) * 1.3,
                         p.get("morph_drive", 0.1) * 1.5,
                         p.get("morph_noiseLevel", 0.0) * 2, scale=1.0)
    return {k: round(v, 3) for k, v in zip(
        ["brightness","warmth","movement","density","space","aggression"],
        [brightness, warmth, movement, density, space, aggression])}


def dna_obese(p: dict) -> dict:
    """Compute DNA for XObese presets (fat_ prefix)."""
    brightness = norm_freq(p.get("fat_filterCutoff", 3000))
    warmth = softmax(p.get("fat_satDrive", 0.5) * 1.3, p.get("fat_analogDrift", 0.2),
                     (1.0 - norm_freq(p.get("fat_filterCutoff", 3000))) * 0.8, scale=1.2)
    movement = softmax(p.get("fat_lfoRate", 0.3), p.get("fat_lfoDepth", 0.2),
                       p.get("fat_modDepth", 0.2), scale=1.1)
    density = softmax(p.get("fat_oscCount", 0.5), p.get("fat_unisonVoices", 0.3),
                      p.get("fat_subLevel", 0.4) * 1.3, scale=1.0)
    space = softmax(p.get("fat_reverbMix", 0.2) * 1.5, p.get("fat_delayMix", 0.1),
                    p.get("fat_release", 0.3), scale=1.1)
    aggression = softmax(p.get("fat_satDrive", 0.5) * 1.5, p.get("fat_bitcrush", 0.0) * 2,
                         p.get("fat_filterReso", 0.3) * 1.3, scale=1.2)
    return {k: round(v, 3) for k, v in zip(
        ["brightness","warmth","movement","density","space","aggression"],
        [brightness, warmth, movement, density, space, aggression])}


def dna_onset(p: dict) -> dict:
    """Compute DNA for XOnset presets (perc_ prefix)."""
    brightness = norm_freq(p.get("perc_filterCutoff", 6000))
    warmth = softmax(p.get("perc_bodyDecay", 0.3), p.get("perc_analogDrift", 0.1),
                     (1.0 - norm_freq(p.get("perc_filterCutoff", 6000))) * 0.5, scale=1.0)
    movement = softmax(p.get("perc_lfoRate", 0.2), p.get("perc_lfoDepth", 0.15),
                       p.get("perc_xvcDepth", 0.2) * 1.5, scale=1.1)
    density = softmax(p.get("perc_noiseLevel", 0.3) * 1.5, p.get("perc_layerCount", 0.5),
                      p.get("perc_circuitBlend", 0.5), scale=1.0)
    space = softmax(p.get("perc_reverbMix", 0.2) * 1.5, p.get("perc_delayMix", 0.1),
                    p.get("perc_release", 0.2), scale=1.1)
    aggression = softmax(p.get("perc_transientSnap", 0.6) * 1.5,
                         p.get("perc_noiseLevel", 0.3) * 1.3,
                         p.get("perc_filterReso", 0.2), scale=1.2)
    return {k: round(v, 3) for k, v in zip(
        ["brightness","warmth","movement","density","space","aggression"],
        [brightness, warmth, movement, density, space, aggression])}


def dna_overworld(p: dict) -> dict:
    """Compute DNA for XOverworld presets (ow_ prefix)."""
    brightness = softmax(p.get("ow_brightness", 0.5), p.get("ow_shimmer", 0.2), scale=1.1)
    warmth = softmax(p.get("ow_analogWarmth", 0.4), p.get("ow_era", 0.5) * 0.8,
                     p.get("ow_drift", 0.15), scale=1.1)
    movement = softmax(p.get("ow_lfoRate", 0.3), p.get("ow_lfoDepth", 0.2),
                       p.get("ow_arpRate", 0.0) * 1.5, scale=1.1)
    density = softmax(p.get("ow_layerCount", 0.5), p.get("ow_bitDepth", 0.7),
                      p.get("ow_chordVoices", 0.3), scale=1.0)
    space = softmax(p.get("ow_reverbMix", 0.4) * 1.5, p.get("ow_delayMix", 0.2),
                    p.get("ow_release", 0.4), scale=1.1)
    aggression = softmax(p.get("ow_bitcrush", 0.0) * 2, p.get("ow_noiseLevel", 0.1),
                         p.get("ow_filterReso", 0.2), scale=1.0)
    return {k: round(v, 3) for k, v in zip(
        ["brightness","warmth","movement","density","space","aggression"],
        [brightness, warmth, movement, density, space, aggression])}


def dna_opal(p: dict) -> dict:
    """Compute DNA for XOpal presets (opal_ prefix)."""
    brightness = softmax(p.get("opal_brightness", 0.5),
                         norm_freq(p.get("opal_filterCutoff", 5000)), scale=1.1)
    warmth = softmax(p.get("opal_warmth", 0.4), p.get("opal_drift", 0.15),
                     p.get("opal_saturation", 0.1), scale=1.1)
    movement = softmax(p.get("opal_grainRate", 0.4) * 1.3, p.get("opal_lfoDepth", 0.2),
                       p.get("opal_scatter", 0.3), scale=1.1)
    density = softmax(p.get("opal_grainSize", 0.4), p.get("opal_grainDensity", 0.5) * 1.3,
                      p.get("opal_layers", 0.3), scale=1.0)
    space = softmax(p.get("opal_reverbMix", 0.4) * 1.5, p.get("opal_delayMix", 0.2),
                    p.get("opal_shimmer", 0.2), scale=1.1)
    aggression = softmax(p.get("opal_filterReso", 0.2), p.get("opal_crush", 0.0) * 2,
                         p.get("opal_noiseLevel", 0.1), scale=1.0)
    return {k: round(v, 3) for k, v in zip(
        ["brightness","warmth","movement","density","space","aggression"],
        [brightness, warmth, movement, density, space, aggression])}


def dna_orbital(p: dict) -> dict:
    """Compute DNA for XOrbital presets (orb_ prefix)."""
    brightness = softmax(p.get("orb_brightness", 0.5),
                         norm_freq(p.get("orb_filterCutoff", 4000)), scale=1.1)
    warmth = softmax(p.get("orb_analogDrift", 0.2), p.get("orb_saturation", 0.15),
                     (1.0 - p.get("orb_brightness", 0.5)) * 0.6, scale=1.1)
    movement = softmax(p.get("orb_lfoRate", 0.3), p.get("orb_lfoDepth", 0.2),
                       p.get("orb_envModDepth", 0.3), scale=1.1)
    density = softmax(p.get("orb_voiceCount", 0.5), p.get("orb_unisonDetune", 0.2),
                      p.get("orb_groupEnvDepth", 0.4) * 1.3, scale=1.0)
    space = softmax(p.get("orb_reverbMix", 0.3) * 1.5, p.get("orb_delayMix", 0.2),
                    p.get("orb_release", 0.4), scale=1.1)
    aggression = softmax(p.get("orb_filterReso", 0.3) * 1.3, p.get("orb_drive", 0.1) * 1.5,
                         p.get("orb_noiseLevel", 0.05), scale=1.0)
    return {k: round(v, 3) for k, v in zip(
        ["brightness","warmth","movement","density","space","aggression"],
        [brightness, warmth, movement, density, space, aggression])}


def dna_organon(p: dict) -> dict:
    """Compute DNA for XOrganon presets (organon_ prefix)."""
    brightness = softmax(p.get("organon_brightness", 0.5),
                         norm_freq(p.get("organon_filterCutoff", 4000)), scale=1.1)
    warmth = softmax(p.get("organon_metabolicRate", 0.5) * 0.8,
                     p.get("organon_drift", 0.15), p.get("organon_saturation", 0.1), scale=1.1)
    movement = softmax(p.get("organon_metabolicRate", 0.5) * 1.3,
                       p.get("organon_lfoDepth", 0.2),
                       p.get("organon_freeEnergy", 0.3) * 1.5, scale=1.1)
    density = softmax(p.get("organon_complexity", 0.5), p.get("organon_voiceCount", 0.4),
                      p.get("organon_layerDensity", 0.4), scale=1.0)
    space = softmax(p.get("organon_reverbMix", 0.3) * 1.5, p.get("organon_delayMix", 0.2),
                    p.get("organon_release", 0.3), scale=1.1)
    aggression = softmax(p.get("organon_filterReso", 0.2), p.get("organon_entropy", 0.2) * 1.5,
                         p.get("organon_noiseLevel", 0.1), scale=1.0)
    return {k: round(v, 3) for k, v in zip(
        ["brightness","warmth","movement","density","space","aggression"],
        [brightness, warmth, movement, density, space, aggression])}


def dna_ouroboros(p: dict) -> dict:
    """Compute DNA for XOuroboros presets (ouro_ prefix)."""
    brightness = softmax(p.get("ouro_brightness", 0.4),
                         norm_freq(p.get("ouro_filterCutoff", 3500)), scale=1.1)
    warmth = softmax(p.get("ouro_feedback", 0.4) * 0.8, p.get("ouro_drift", 0.2),
                     p.get("ouro_saturation", 0.2) * 1.3, scale=1.1)
    movement = softmax(p.get("ouro_chaosRate", 0.4) * 1.5, p.get("ouro_lfoDepth", 0.2),
                       p.get("ouro_leashLength", 0.5) * 0.8, scale=1.1)
    density = softmax(p.get("ouro_topology", 0.5), p.get("ouro_feedback", 0.4) * 1.3,
                      p.get("ouro_voiceCount", 0.3), scale=1.0)
    space = softmax(p.get("ouro_reverbMix", 0.3) * 1.5, p.get("ouro_delayMix", 0.2),
                    p.get("ouro_release", 0.3), scale=1.1)
    aggression = softmax(p.get("ouro_chaosAmount", 0.4) * 1.5,
                         p.get("ouro_filterReso", 0.3) * 1.3,
                         p.get("ouro_distortion", 0.2) * 1.5, scale=1.2)
    return {k: round(v, 3) for k, v in zip(
        ["brightness","warmth","movement","density","space","aggression"],
        [brightness, warmth, movement, density, space, aggression])}


def dna_obsidian(p: dict) -> dict:
    """Compute DNA for XObsidian presets (obsidian_ prefix)."""
    brightness = softmax(p.get("obsidian_brightness", 0.4),
                         norm_freq(p.get("obsidian_filterCutoff", 3000)), scale=1.1)
    warmth = softmax(p.get("obsidian_depth", 0.5) * 0.8, p.get("obsidian_drift", 0.15),
                     p.get("obsidian_saturation", 0.15), scale=1.1)
    movement = softmax(p.get("obsidian_lfoRate", 0.2), p.get("obsidian_lfoDepth", 0.15),
                       p.get("obsidian_modDepth", 0.2), scale=1.0)
    density = softmax(p.get("obsidian_layerCount", 0.4), p.get("obsidian_depth", 0.5) * 1.3,
                      p.get("obsidian_voiceCount", 0.3), scale=1.0)
    space = softmax(p.get("obsidian_reverbMix", 0.4) * 1.5, p.get("obsidian_delayMix", 0.2),
                    p.get("obsidian_release", 0.4) * 1.3, scale=1.1)
    aggression = softmax(p.get("obsidian_filterReso", 0.2),
                         p.get("obsidian_noiseLevel", 0.1) * 1.5,
                         p.get("obsidian_crush", 0.0) * 2, scale=1.0)
    return {k: round(v, 3) for k, v in zip(
        ["brightness","warmth","movement","density","space","aggression"],
        [brightness, warmth, movement, density, space, aggression])}


def dna_overbite(p: dict) -> dict:
    """Compute DNA for XOverbite presets (poss_ prefix)."""
    brightness = softmax(p.get("poss_brightness", 0.5),
                         norm_freq(p.get("poss_filterCutoff", 5000)), scale=1.1)
    warmth = softmax(p.get("poss_drift", 0.15), p.get("poss_saturation", 0.2),
                     p.get("poss_bellyMacro", 0.3) * 0.8, scale=1.1)
    movement = softmax(p.get("poss_lfoRate", 0.3), p.get("poss_lfoDepth", 0.2),
                       p.get("poss_scurryMacro", 0.3) * 1.5, scale=1.1)
    density = softmax(p.get("poss_voiceCount", 0.4), p.get("poss_layerBlend", 0.4),
                      p.get("poss_trashMacro", 0.2) * 0.8, scale=1.0)
    space = softmax(p.get("poss_reverbMix", 0.3) * 1.5, p.get("poss_delayMix", 0.2),
                    p.get("poss_release", 0.3), scale=1.1)
    aggression = softmax(p.get("poss_biteDepth", 0.4) * 1.5,
                         p.get("poss_filterReso", 0.3) * 1.3,
                         p.get("poss_trashMacro", 0.2) * 1.5, scale=1.2)
    return {k: round(v, 3) for k, v in zip(
        ["brightness","warmth","movement","density","space","aggression"],
        [brightness, warmth, movement, density, space, aggression])}


def dna_origami(p: dict) -> dict:
    """Compute DNA for XOrigami presets (origami_ prefix)."""
    brightness = softmax(p.get("origami_brightness", 0.5),
                         norm_freq(p.get("origami_filterCutoff", 4500)), scale=1.1)
    warmth = softmax(p.get("origami_paperWeight", 0.4) * 0.8, p.get("origami_drift", 0.15),
                     p.get("origami_saturation", 0.1), scale=1.1)
    movement = softmax(p.get("origami_foldRate", 0.3) * 1.5, p.get("origami_lfoDepth", 0.2),
                       p.get("origami_modDepth", 0.2), scale=1.1)
    density = softmax(p.get("origami_foldCount", 0.4) * 1.3, p.get("origami_layers", 0.3),
                      p.get("origami_voiceCount", 0.3), scale=1.0)
    space = softmax(p.get("origami_reverbMix", 0.3) * 1.5, p.get("origami_delayMix", 0.2),
                    p.get("origami_release", 0.3), scale=1.1)
    aggression = softmax(p.get("origami_foldPoint", 0.4) * 1.5,
                         p.get("origami_filterReso", 0.2) * 1.3,
                         p.get("origami_crease", 0.2) * 1.5, scale=1.1)
    return {k: round(v, 3) for k, v in zip(
        ["brightness","warmth","movement","density","space","aggression"],
        [brightness, warmth, movement, density, space, aggression])}


def dna_oracle(p: dict) -> dict:
    """Compute DNA for XOracle presets (oracle_ prefix)."""
    brightness = softmax(p.get("oracle_brightness", 0.4),
                         norm_freq(p.get("oracle_filterCutoff", 3500)), scale=1.1)
    warmth = softmax(p.get("oracle_drift", 0.2), p.get("oracle_saturation", 0.15),
                     p.get("oracle_harmonicWarmth", 0.3), scale=1.1)
    movement = softmax(p.get("oracle_stochasticRate", 0.4) * 1.5,
                       p.get("oracle_lfoDepth", 0.2),
                       p.get("oracle_breakpoints", 0.3) * 1.3, scale=1.1)
    density = softmax(p.get("oracle_breakpoints", 0.3) * 1.3,
                      p.get("oracle_voiceCount", 0.3),
                      p.get("oracle_complexity", 0.4), scale=1.0)
    space = softmax(p.get("oracle_reverbMix", 0.3) * 1.5, p.get("oracle_delayMix", 0.2),
                    p.get("oracle_release", 0.3), scale=1.1)
    aggression = softmax(p.get("oracle_filterReso", 0.2),
                         p.get("oracle_stochasticIntensity", 0.3) * 1.5,
                         p.get("oracle_noiseLevel", 0.15) * 1.3, scale=1.1)
    return {k: round(v, 3) for k, v in zip(
        ["brightness","warmth","movement","density","space","aggression"],
        [brightness, warmth, movement, density, space, aggression])}


def dna_obscura(p: dict) -> dict:
    """Compute DNA for XObscura presets (obscura_ prefix)."""
    brightness = softmax(p.get("obscura_brightness", 0.35),
                         norm_freq(p.get("obscura_filterCutoff", 3000)), scale=1.0)
    warmth = softmax(p.get("obscura_drift", 0.2), p.get("obscura_vintage", 0.4) * 1.3,
                     p.get("obscura_saturation", 0.15), scale=1.1)
    movement = softmax(p.get("obscura_lfoRate", 0.2), p.get("obscura_lfoDepth", 0.15),
                       p.get("obscura_modDepth", 0.2), scale=1.0)
    density = softmax(p.get("obscura_stiffness", 0.4) * 1.3, p.get("obscura_voiceCount", 0.3),
                      p.get("obscura_layerCount", 0.3), scale=1.0)
    space = softmax(p.get("obscura_reverbMix", 0.4) * 1.5, p.get("obscura_delayMix", 0.3) * 1.3,
                    p.get("obscura_release", 0.4), scale=1.1)
    aggression = softmax(p.get("obscura_filterReso", 0.2),
                         p.get("obscura_noiseLevel", 0.15) * 1.3,
                         p.get("obscura_grainCrunch", 0.1) * 1.5, scale=1.0)
    return {k: round(v, 3) for k, v in zip(
        ["brightness","warmth","movement","density","space","aggression"],
        [brightness, warmth, movement, density, space, aggression])}


def dna_oceanic(p: dict) -> dict:
    """Compute DNA for XOceanic presets (ocean_ prefix)."""
    brightness = softmax(p.get("ocean_brightness", 0.5),
                         norm_freq(p.get("ocean_filterCutoff", 4500)), scale=1.1)
    warmth = softmax(p.get("ocean_drift", 0.2), p.get("ocean_saturation", 0.1),
                     p.get("ocean_tidalWarmth", 0.3), scale=1.1)
    movement = softmax(p.get("ocean_tidalRate", 0.4) * 1.5, p.get("ocean_lfoDepth", 0.2),
                       p.get("ocean_chromatophoreRate", 0.3) * 1.3, scale=1.1)
    density = softmax(p.get("ocean_separation", 0.4) * 1.3, p.get("ocean_voiceCount", 0.4),
                      p.get("ocean_layerCount", 0.3), scale=1.0)
    space = softmax(p.get("ocean_reverbMix", 0.4) * 1.5, p.get("ocean_delayMix", 0.2),
                    p.get("ocean_release", 0.4) * 1.3, scale=1.1)
    aggression = softmax(p.get("ocean_filterReso", 0.2),
                         p.get("ocean_currentForce", 0.2) * 1.5,
                         p.get("ocean_noiseLevel", 0.15), scale=1.0)
    return {k: round(v, 3) for k, v in zip(
        ["brightness","warmth","movement","density","space","aggression"],
        [brightness, warmth, movement, density, space, aggression])}


def dna_ocelot(p: dict) -> dict:
    """Compute DNA for XOcelot presets (ocelot_ prefix)."""
    brightness = softmax(p.get("ocelot_brightness", 0.5),
                         norm_freq(p.get("ocelot_filterCutoff", 4000)), scale=1.1)
    warmth = softmax(p.get("ocelot_drift", 0.2), p.get("ocelot_saturation", 0.15),
                     p.get("ocelot_biome", 0.5) * 0.6, scale=1.1)
    movement = softmax(p.get("ocelot_lfoRate", 0.3), p.get("ocelot_lfoDepth", 0.2),
                       p.get("ocelot_prowlRate", 0.3) * 1.5, scale=1.1)
    density = softmax(p.get("ocelot_voiceCount", 0.3), p.get("ocelot_layerBlend", 0.4),
                      p.get("ocelot_patternDensity", 0.4) * 1.3, scale=1.0)
    space = softmax(p.get("ocelot_reverbMix", 0.3) * 1.5, p.get("ocelot_delayMix", 0.2),
                    p.get("ocelot_release", 0.3), scale=1.1)
    aggression = softmax(p.get("ocelot_filterReso", 0.3) * 1.3,
                         p.get("ocelot_clawDepth", 0.2) * 1.5,
                         p.get("ocelot_noiseLevel", 0.1), scale=1.1)
    return {k: round(v, 3) for k, v in zip(
        ["brightness","warmth","movement","density","space","aggression"],
        [brightness, warmth, movement, density, space, aggression])}


def dna_optic(p: dict) -> dict:
    """Compute DNA for XOptic presets (optic_ prefix)."""
    brightness = softmax(p.get("optic_brightness", 0.6),
                         p.get("optic_phosphorLevel", 0.5) * 1.3, scale=1.1)
    warmth = softmax(p.get("optic_warmth", 0.3), p.get("optic_drift", 0.1),
                     p.get("optic_saturation", 0.1), scale=1.0)
    movement = softmax(p.get("optic_pulseRate", 0.4) * 1.5, p.get("optic_lfoDepth", 0.2),
                       p.get("optic_autoPulseDepth", 0.3) * 1.3, scale=1.1)
    density = softmax(p.get("optic_layerCount", 0.3), p.get("optic_complexity", 0.4),
                      p.get("optic_voiceCount", 0.3), scale=1.0)
    space = softmax(p.get("optic_reverbMix", 0.3) * 1.5, p.get("optic_delayMix", 0.2),
                    p.get("optic_release", 0.3), scale=1.1)
    aggression = softmax(p.get("optic_filterReso", 0.2),
                         p.get("optic_strobeIntensity", 0.2) * 1.5,
                         p.get("optic_noiseLevel", 0.05), scale=1.0)
    return {k: round(v, 3) for k, v in zip(
        ["brightness","warmth","movement","density","space","aggression"],
        [brightness, warmth, movement, density, space, aggression])}


def dna_oblique(p: dict) -> dict:
    """Compute DNA for XOblique presets (oblq_ prefix)."""
    brightness = softmax(p.get("oblq_brightness", 0.5),
                         p.get("oblq_prismColor", 0.5) * 1.3, scale=1.1)
    warmth = softmax(p.get("oblq_drift", 0.15), p.get("oblq_saturation", 0.15),
                     (1.0 - p.get("oblq_prismColor", 0.5)) * 0.5, scale=1.1)
    movement = softmax(p.get("oblq_bounceRate", 0.4) * 1.5, p.get("oblq_lfoDepth", 0.2),
                       p.get("oblq_refractDepth", 0.3) * 1.3, scale=1.1)
    density = softmax(p.get("oblq_layerCount", 0.4), p.get("oblq_voiceCount", 0.3),
                      p.get("oblq_prismFacets", 0.4) * 1.3, scale=1.0)
    space = softmax(p.get("oblq_reverbMix", 0.3) * 1.5, p.get("oblq_delayMix", 0.2),
                    p.get("oblq_release", 0.3), scale=1.1)
    aggression = softmax(p.get("oblq_filterReso", 0.3) * 1.3,
                         p.get("oblq_distortion", 0.2) * 1.5,
                         p.get("oblq_noiseLevel", 0.1), scale=1.1)
    return {k: round(v, 3) for k, v in zip(
        ["brightness","warmth","movement","density","space","aggression"],
        [brightness, warmth, movement, density, space, aggression])}


def dna_osprey(p: dict) -> dict:
    """Compute DNA for XOsprey presets (osprey_ prefix)."""
    brightness = softmax(p.get("osprey_brightness", 0.5),
                         norm_freq(p.get("osprey_filterCutoff", 4000)), scale=1.1)
    warmth = softmax(p.get("osprey_shoreBlend", 0.4) * 0.8, p.get("osprey_drift", 0.2),
                     p.get("osprey_saturation", 0.15), scale=1.1)
    movement = softmax(p.get("osprey_windRate", 0.3) * 1.5, p.get("osprey_lfoDepth", 0.2),
                       p.get("osprey_tideModDepth", 0.3) * 1.3, scale=1.1)
    density = softmax(p.get("osprey_voiceCount", 0.3), p.get("osprey_layerBlend", 0.4),
                      p.get("osprey_shoreComplexity", 0.3), scale=1.0)
    space = softmax(p.get("osprey_reverbMix", 0.4) * 1.5, p.get("osprey_delayMix", 0.2),
                    p.get("osprey_release", 0.4) * 1.3, scale=1.1)
    aggression = softmax(p.get("osprey_filterReso", 0.2),
                         p.get("osprey_galeForce", 0.2) * 1.5,
                         p.get("osprey_noiseLevel", 0.15), scale=1.0)
    return {k: round(v, 3) for k, v in zip(
        ["brightness","warmth","movement","density","space","aggression"],
        [brightness, warmth, movement, density, space, aggression])}


def dna_osteria(p: dict) -> dict:
    """Compute DNA for XOsteria presets (osteria_ prefix)."""
    brightness = softmax(p.get("osteria_brightness", 0.4),
                         norm_freq(p.get("osteria_filterCutoff", 3500)), scale=1.1)
    warmth = softmax(p.get("osteria_drift", 0.2), p.get("osteria_saturation", 0.2) * 1.3,
                     p.get("osteria_qBassShore", 0.4) * 0.8, scale=1.2)
    movement = softmax(p.get("osteria_lfoRate", 0.25), p.get("osteria_lfoDepth", 0.2),
                       p.get("osteria_modDepth", 0.2), scale=1.0)
    density = softmax(p.get("osteria_voiceCount", 0.3), p.get("osteria_layerBlend", 0.4),
                      p.get("osteria_bodyResonance", 0.3) * 1.3, scale=1.0)
    space = softmax(p.get("osteria_reverbMix", 0.35) * 1.5, p.get("osteria_delayMix", 0.2),
                    p.get("osteria_release", 0.35), scale=1.1)
    aggression = softmax(p.get("osteria_filterReso", 0.25) * 1.3,
                         p.get("osteria_drive", 0.15) * 1.5,
                         p.get("osteria_noiseLevel", 0.1), scale=1.0)
    return {k: round(v, 3) for k, v in zip(
        ["brightness","warmth","movement","density","space","aggression"],
        [brightness, warmth, movement, density, space, aggression])}


def dna_owlfish(p: dict) -> dict:
    """Compute DNA for XOwlfish presets (owl_ prefix)."""
    brightness = softmax(p.get("owl_brightness", 0.4),
                         norm_freq(p.get("owl_filterCutoff", 3000)), scale=1.1)
    warmth = softmax(p.get("owl_drift", 0.2), p.get("owl_saturation", 0.2) * 1.3,
                     p.get("owl_subharmonic", 0.3), scale=1.2)
    movement = softmax(p.get("owl_trautoniumRate", 0.3) * 1.5, p.get("owl_lfoDepth", 0.2),
                       p.get("owl_modDepth", 0.2), scale=1.1)
    density = softmax(p.get("owl_mixturVoices", 0.5) * 1.3, p.get("owl_voiceCount", 0.3),
                      p.get("owl_layerBlend", 0.3), scale=1.0)
    space = softmax(p.get("owl_reverbMix", 0.35) * 1.5, p.get("owl_delayMix", 0.2),
                    p.get("owl_release", 0.35), scale=1.1)
    aggression = softmax(p.get("owl_filterReso", 0.25) * 1.3,
                         p.get("owl_distortion", 0.15) * 1.5,
                         p.get("owl_noiseLevel", 0.15) * 1.3, scale=1.1)
    return {k: round(v, 3) for k, v in zip(
        ["brightness","warmth","movement","density","space","aggression"],
        [brightness, warmth, movement, density, space, aggression])}


def dna_ohm(p: dict) -> dict:
    """Compute DNA for XOhm presets (ohm_ prefix)."""
    brightness = softmax(p.get("ohm_brightness", 0.5),
                         norm_freq(p.get("ohm_filterCutoff", 4000)), scale=1.1)
    warmth = softmax(p.get("ohm_analogDrift", 0.2), p.get("ohm_saturation", 0.15),
                     p.get("ohm_macroMeddling", 0.3) * 0.5, scale=1.1)
    movement = softmax(p.get("ohm_lfoRate", 0.3), p.get("ohm_lfoDepth", 0.2),
                       p.get("ohm_macroMeddling", 0.3) * 1.3, scale=1.1)
    density = softmax(p.get("ohm_voiceCount", 0.3), p.get("ohm_layerBlend", 0.4),
                      p.get("ohm_impedance", 0.4) * 1.3, scale=1.0)
    space = softmax(p.get("ohm_reverbMix", 0.3) * 1.5, p.get("ohm_delayMix", 0.2),
                    p.get("ohm_release", 0.3), scale=1.1)
    aggression = softmax(p.get("ohm_filterReso", 0.2),
                         p.get("ohm_resistance", 0.2) * 1.5,
                         p.get("ohm_noiseLevel", 0.1), scale=1.0)
    return {k: round(v, 3) for k, v in zip(
        ["brightness","warmth","movement","density","space","aggression"],
        [brightness, warmth, movement, density, space, aggression])}


def dna_orphica(p: dict) -> dict:
    """Compute DNA for XOrphica presets (orph_ prefix)."""
    brightness = softmax(p.get("orph_pluckBrightness", 0.5) * 1.3,
                         norm_freq(p.get("orph_filterCutoff", 5000)), scale=1.1)
    warmth = softmax(p.get("orph_drift", 0.15), p.get("orph_bodyResonance", 0.3) * 1.3,
                     p.get("orph_saturation", 0.1), scale=1.1)
    movement = softmax(p.get("orph_lfoRate", 0.3), p.get("orph_lfoDepth", 0.2),
                       p.get("orph_tremoloDepth", 0.2) * 1.5, scale=1.1)
    density = softmax(p.get("orph_voiceCount", 0.3), p.get("orph_stringCount", 0.4) * 1.3,
                      p.get("orph_layerBlend", 0.3), scale=1.0)
    space = softmax(p.get("orph_reverbMix", 0.35) * 1.5, p.get("orph_delayMix", 0.2),
                    p.get("orph_release", 0.4) * 1.3, scale=1.1)
    aggression = softmax(p.get("orph_filterReso", 0.2),
                         p.get("orph_pluckAttack", 0.3) * 1.3,
                         p.get("orph_noiseLevel", 0.1), scale=1.0)
    return {k: round(v, 3) for k, v in zip(
        ["brightness","warmth","movement","density","space","aggression"],
        [brightness, warmth, movement, density, space, aggression])}


def dna_obbligato(p: dict) -> dict:
    """Compute DNA for XObbligato presets (obbl_ prefix)."""
    brightness = softmax(p.get("obbl_brightness", 0.5),
                         norm_freq(p.get("obbl_filterCutoff", 4500)), scale=1.1)
    warmth = softmax(p.get("obbl_breathA", 0.4) * 0.8, p.get("obbl_drift", 0.15),
                     p.get("obbl_saturation", 0.15), scale=1.1)
    movement = softmax(p.get("obbl_lfoRate", 0.3), p.get("obbl_lfoDepth", 0.2),
                       p.get("obbl_vibratoDepth", 0.3) * 1.5, scale=1.1)
    density = softmax(p.get("obbl_voiceCount", 0.3), p.get("obbl_layerBlend", 0.4),
                      p.get("obbl_breathDensity", 0.3) * 1.3, scale=1.0)
    space = softmax(p.get("obbl_reverbMix", 0.35) * 1.5, p.get("obbl_delayMix", 0.2),
                    p.get("obbl_release", 0.35), scale=1.1)
    aggression = softmax(p.get("obbl_filterReso", 0.2),
                         p.get("obbl_overblowIntensity", 0.2) * 1.5,
                         p.get("obbl_noiseLevel", 0.15) * 1.3, scale=1.0)
    return {k: round(v, 3) for k, v in zip(
        ["brightness","warmth","movement","density","space","aggression"],
        [brightness, warmth, movement, density, space, aggression])}


def dna_ottoni(p: dict) -> dict:
    """Compute DNA for XOttoni presets (otto_ prefix)."""
    brightness = softmax(p.get("otto_brightness", 0.5),
                         norm_freq(p.get("otto_filterCutoff", 4000)), scale=1.1)
    warmth = softmax(p.get("otto_drift", 0.2), p.get("otto_brassWarmth", 0.4) * 1.3,
                     p.get("otto_saturation", 0.15), scale=1.2)
    movement = softmax(p.get("otto_lfoRate", 0.25), p.get("otto_lfoDepth", 0.2),
                       p.get("otto_macroGrow", 0.3) * 1.5, scale=1.1)
    density = softmax(p.get("otto_voiceCount", 0.4), p.get("otto_sectionSize", 0.4) * 1.3,
                      p.get("otto_layerBlend", 0.3), scale=1.0)
    space = softmax(p.get("otto_reverbMix", 0.35) * 1.5, p.get("otto_delayMix", 0.2),
                    p.get("otto_release", 0.35), scale=1.1)
    aggression = softmax(p.get("otto_filterReso", 0.25) * 1.3,
                         p.get("otto_brassBlare", 0.3) * 1.5,
                         p.get("otto_noiseLevel", 0.1), scale=1.1)
    return {k: round(v, 3) for k, v in zip(
        ["brightness","warmth","movement","density","space","aggression"],
        [brightness, warmth, movement, density, space, aggression])}


def dna_ole(p: dict) -> dict:
    """Compute DNA for XOle presets (ole_ prefix)."""
    brightness = softmax(p.get("ole_brightness", 0.5),
                         norm_freq(p.get("ole_filterCutoff", 4500)), scale=1.1)
    warmth = softmax(p.get("ole_drift", 0.2), p.get("ole_saturation", 0.2) * 1.3,
                     p.get("ole_flamencoWarmth", 0.4) * 0.8, scale=1.2)
    movement = softmax(p.get("ole_lfoRate", 0.3), p.get("ole_lfoDepth", 0.2),
                       p.get("ole_macroDrama", 0.3) * 1.5, scale=1.1)
    density = softmax(p.get("ole_voiceCount", 0.3), p.get("ole_layerBlend", 0.4),
                      p.get("ole_rhythmDensity", 0.3) * 1.3, scale=1.0)
    space = softmax(p.get("ole_reverbMix", 0.3) * 1.5, p.get("ole_delayMix", 0.2),
                    p.get("ole_release", 0.3), scale=1.1)
    aggression = softmax(p.get("ole_filterReso", 0.25) * 1.3,
                         p.get("ole_rasgueadoIntensity", 0.3) * 1.5,
                         p.get("ole_noiseLevel", 0.1), scale=1.1)
    return {k: round(v, 3) for k, v in zip(
        ["brightness","warmth","movement","density","space","aggression"],
        [brightness, warmth, movement, density, space, aggression])}


def dna_overlap(p: dict) -> dict:
    """Compute DNA for XOverlap presets (overlap_ prefix)."""
    brightness = softmax(p.get("overlap_brightness", 0.5),
                         norm_freq(p.get("overlap_filterCutoff", 4000)), scale=1.1)
    warmth = softmax(p.get("overlap_drift", 0.2), p.get("overlap_saturation", 0.15),
                     p.get("overlap_blendWarmth", 0.3), scale=1.1)
    movement = softmax(p.get("overlap_lfoRate", 0.3), p.get("overlap_lfoDepth", 0.2),
                       p.get("overlap_crossfadeRate", 0.3) * 1.5, scale=1.1)
    density = softmax(p.get("overlap_voiceCount", 0.4), p.get("overlap_layerCount", 0.5) * 1.3,
                      p.get("overlap_overlapAmount", 0.4), scale=1.0)
    space = softmax(p.get("overlap_reverbMix", 0.35) * 1.5, p.get("overlap_delayMix", 0.2),
                    p.get("overlap_release", 0.35), scale=1.1)
    aggression = softmax(p.get("overlap_filterReso", 0.2),
                         p.get("overlap_collisionForce", 0.2) * 1.5,
                         p.get("overlap_noiseLevel", 0.1), scale=1.0)
    return {k: round(v, 3) for k, v in zip(
        ["brightness","warmth","movement","density","space","aggression"],
        [brightness, warmth, movement, density, space, aggression])}


def dna_outwit(p: dict) -> dict:
    """Compute DNA for XOutwit presets (outwit_ prefix)."""
    brightness = softmax(p.get("outwit_brightness", 0.5),
                         norm_freq(p.get("outwit_filterCutoff", 4500)), scale=1.1)
    warmth = softmax(p.get("outwit_drift", 0.15), p.get("outwit_saturation", 0.15),
                     p.get("outwit_analogWarmth", 0.3), scale=1.1)
    movement = softmax(p.get("outwit_lfoRate", 0.35), p.get("outwit_lfoDepth", 0.25),
                       p.get("outwit_trickRate", 0.3) * 1.5, scale=1.1)
    density = softmax(p.get("outwit_voiceCount", 0.3), p.get("outwit_layerBlend", 0.4),
                      p.get("outwit_complexity", 0.4) * 1.3, scale=1.0)
    space = softmax(p.get("outwit_reverbMix", 0.3) * 1.5, p.get("outwit_delayMix", 0.2),
                    p.get("outwit_release", 0.3), scale=1.1)
    aggression = softmax(p.get("outwit_filterReso", 0.25) * 1.3,
                         p.get("outwit_glitchIntensity", 0.2) * 1.5,
                         p.get("outwit_noiseLevel", 0.1), scale=1.1)
    return {k: round(v, 3) for k, v in zip(
        ["brightness","warmth","movement","density","space","aggression"],
        [brightness, warmth, movement, density, space, aggression])}


def dna_ombre(p: dict) -> dict:
    """Compute DNA for XOmbre presets (ombre_ prefix)."""
    brightness = softmax(p.get("ombre_brightness", 0.4),
                         norm_freq(p.get("ombre_filterCutoff", 3500)), scale=1.1)
    warmth = softmax(p.get("ombre_drift", 0.2), p.get("ombre_saturation", 0.2) * 1.3,
                     p.get("ombre_memoryWarmth", 0.4) * 0.8, scale=1.2)
    movement = softmax(p.get("ombre_blend", 0.4) * 1.5, p.get("ombre_lfoDepth", 0.2),
                       p.get("ombre_narrativeRate", 0.3) * 1.3, scale=1.1)
    density = softmax(p.get("ombre_voiceCount", 0.3), p.get("ombre_layerBlend", 0.4),
                      p.get("ombre_dualNarrativeDepth", 0.4) * 1.3, scale=1.0)
    space = softmax(p.get("ombre_reverbMix", 0.4) * 1.5, p.get("ombre_delayMix", 0.3) * 1.3,
                    p.get("ombre_release", 0.4), scale=1.1)
    aggression = softmax(p.get("ombre_filterReso", 0.2),
                         p.get("ombre_forgettingRate", 0.2) * 1.5,
                         p.get("ombre_noiseLevel", 0.15), scale=1.0)
    return {k: round(v, 3) for k, v in zip(
        ["brightness","warmth","movement","density","space","aggression"],
        [brightness, warmth, movement, density, space, aggression])}


def dna_orca(p: dict) -> dict:
    """Compute DNA for XOrca presets (orca_ prefix)."""
    brightness = softmax(p.get("orca_brightness", 0.4),
                         norm_freq(p.get("orca_filterCutoff", 3000)), scale=1.1)
    warmth = softmax(p.get("orca_drift", 0.2), p.get("orca_saturation", 0.2) * 1.3,
                     p.get("orca_depthWarmth", 0.3), scale=1.1)
    movement = softmax(p.get("orca_huntMacro", 0.4) * 1.5, p.get("orca_lfoDepth", 0.2),
                       p.get("orca_echoLocationRate", 0.3) * 1.3, scale=1.1)
    density = softmax(p.get("orca_wavetablePosition", 0.4) * 1.3,
                      p.get("orca_voiceCount", 0.3),
                      p.get("orca_podSize", 0.3), scale=1.0)
    space = softmax(p.get("orca_reverbMix", 0.35) * 1.5, p.get("orca_delayMix", 0.2),
                    p.get("orca_release", 0.4) * 1.3, scale=1.1)
    aggression = softmax(p.get("orca_breachIntensity", 0.4) * 1.5,
                         p.get("orca_filterReso", 0.3) * 1.3,
                         p.get("orca_predatorDrive", 0.3) * 1.5, scale=1.2)
    return {k: round(v, 3) for k, v in zip(
        ["brightness","warmth","movement","density","space","aggression"],
        [brightness, warmth, movement, density, space, aggression])}


def dna_octopus(p: dict) -> dict:
    """Compute DNA for XOctopus presets (octo_ prefix)."""
    brightness = softmax(p.get("octo_brightness", 0.5),
                         norm_freq(p.get("octo_filterCutoff", 4000)), scale=1.1)
    warmth = softmax(p.get("octo_drift", 0.15), p.get("octo_saturation", 0.15),
                     p.get("octo_chromatophoreWarmth", 0.3), scale=1.1)
    movement = softmax(p.get("octo_armDepth", 0.4) * 1.5, p.get("octo_lfoDepth", 0.2),
                       p.get("octo_tentacleRate", 0.3) * 1.3, scale=1.1)
    density = softmax(p.get("octo_armCount", 0.5) * 1.3, p.get("octo_voiceCount", 0.3),
                      p.get("octo_inkCloudDensity", 0.3), scale=1.0)
    space = softmax(p.get("octo_reverbMix", 0.35) * 1.5, p.get("octo_delayMix", 0.2),
                    p.get("octo_release", 0.35), scale=1.1)
    aggression = softmax(p.get("octo_filterReso", 0.25) * 1.3,
                         p.get("octo_inkCloudIntensity", 0.3) * 1.5,
                         p.get("octo_chromatophoreFlash", 0.2) * 1.3, scale=1.1)
    return {k: round(v, 3) for k, v in zip(
        ["brightness","warmth","movement","density","space","aggression"],
        [brightness, warmth, movement, density, space, aggression])}


# Engine dispatcher
DNA_FUNCTIONS = {
    "OddfeliX":   dna_xoddcouple,
    "OddOscar":   dna_oddoscar,
    "XOverdub":   dna_xoverdub,
    "XOdyssey":   dna_xodyssey,
    "XOblong":    dna_xoblongbob,
    "XObese":     dna_obese,
    "XOnset":     dna_onset,
    "XOverworld": dna_overworld,
    "XOpal":      dna_opal,
    "XOrbital":   dna_orbital,
    "XOrganon":   dna_organon,
    "XOuroboros":  dna_ouroboros,
    "XObsidian":  dna_obsidian,
    "XOverbite":  dna_overbite,
    "XOrigami":   dna_origami,
    "XOracle":    dna_oracle,
    "XObscura":   dna_obscura,
    "XOceanic":   dna_oceanic,
    "XOcelot":    dna_ocelot,
    "XOptic":     dna_optic,
    "XOblique":   dna_oblique,
    "XOsprey":    dna_osprey,
    "XOsteria":   dna_osteria,
    "XOwlfish":   dna_owlfish,
    "XOhm":       dna_ohm,
    "XOrphica":   dna_orphica,
    "XObbligato": dna_obbligato,
    "XOttoni":    dna_ottoni,
    "XOle":       dna_ole,
    "XOverlap":   dna_overlap,
    "XOutwit":    dna_outwit,
    "XOmbre":     dna_ombre,
    "XOrca":      dna_orca,
    "XOctopus":   dna_octopus,
}


def compute_dna(xometa: dict) -> dict:
    """Compute the DNA vector for a preset by analyzing its parameters per engine."""
    engines = xometa.get("engines", [])
    all_params = xometa.get("parameters", {})

    if not engines:
        return {"brightness": 0.5, "warmth": 0.5, "movement": 0.5,
                "density": 0.5, "space": 0.5, "aggression": 0.5}

    # For single-engine presets, use the engine-specific function
    if len(engines) == 1:
        engine = engines[0]
        params = all_params.get(engine, {})
        fn = DNA_FUNCTIONS.get(engine)
        if fn:
            return fn(params)

    # For multi-engine presets, average the DNA across engines
    dna_vectors = []
    for engine in engines:
        params = all_params.get(engine, {})
        fn = DNA_FUNCTIONS.get(engine)
        if fn:
            dna_vectors.append(fn(params))

    if not dna_vectors:
        return {"brightness": 0.5, "warmth": 0.5, "movement": 0.5,
                "density": 0.5, "space": 0.5, "aggression": 0.5}

    # Average
    dims = ["brightness", "warmth", "movement", "density", "space", "aggression"]
    result = {}
    for dim in dims:
        avg = sum(d[dim] for d in dna_vectors) / len(dna_vectors)
        result[dim] = round(avg, 3)
    return result


# ---------------------------------------------------------------------------
# Similarity & search
# ---------------------------------------------------------------------------

def euclidean_distance(a: dict, b: dict) -> float:
    dims = ["brightness", "warmth", "movement", "density", "space", "aggression"]
    return math.sqrt(sum((a[d] - b[d]) ** 2 for d in dims))


def invert_dna(dna: dict) -> dict:
    """Return the 'opposite' DNA vector."""
    return {k: round(1.0 - v, 3) for k, v in dna.items()}


def find_similar(target_dna: dict, all_presets: list, n: int = 5) -> list:
    """Find the N most similar presets by DNA distance."""
    scored = []
    for preset in all_presets:
        if "dna" not in preset:
            continue
        dist = euclidean_distance(target_dna, preset["dna"])
        scored.append((dist, preset["name"], preset.get("mood", "?")))
    scored.sort()
    return scored[:n]


def find_opposite(target_dna: dict, all_presets: list, n: int = 5) -> list:
    """Find the N most opposite presets (closest to inverted DNA)."""
    inv = invert_dna(target_dna)
    return find_similar(inv, all_presets, n)


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main():
    dry_run = "--dry-run" in sys.argv
    report = "--report" in sys.argv

    xometa_files = sorted(PRESET_DIR.rglob("*.xometa"))
    print(f"Found {len(xometa_files)} .xometa files\n")

    all_presets = []
    stats = {"processed": 0, "skipped": 0, "updated": 0}

    # Dimension accumulators for distribution report
    dim_sums = {"brightness": 0, "warmth": 0, "movement": 0,
                "density": 0, "space": 0, "aggression": 0}
    dim_mins = {k: 1.0 for k in dim_sums}
    dim_maxs = {k: 0.0 for k in dim_sums}

    for fpath in xometa_files:
        try:
            xometa = json.loads(fpath.read_text())
        except json.JSONDecodeError:
            print(f"  SKIP (bad JSON): {fpath.name}")
            stats["skipped"] += 1
            continue

        engines = xometa.get("engines", [])
        if not any(e in DNA_FUNCTIONS for e in engines):
            # Engine not recognized
            stats["skipped"] += 1
            continue

        dna = compute_dna(xometa)
        xometa["dna"] = dna
        all_presets.append(xometa)

        # Track stats
        for dim, val in dna.items():
            dim_sums[dim] += val
            dim_mins[dim] = min(dim_mins[dim], val)
            dim_maxs[dim] = max(dim_maxs[dim], val)

        if not dry_run:
            with open(fpath, "w") as f:
                json.dump(xometa, f, indent=2)
            stats["updated"] += 1
        else:
            stats["updated"] += 1

        stats["processed"] += 1

    n = stats["processed"] or 1

    print(f"{'=' * 60}")
    print(f"DNA FINGERPRINT REPORT")
    print(f"{'=' * 60}")
    print(f"Processed: {stats['processed']}")
    print(f"Skipped:   {stats['skipped']}")
    print(f"Updated:   {stats['updated']}")
    print(f"\nDNA Distribution (avg / min / max):")
    for dim in ["brightness", "warmth", "movement", "density", "space", "aggression"]:
        avg = dim_sums[dim] / n
        print(f"  {dim:12s}: {avg:.3f}  [{dim_mins[dim]:.3f} – {dim_maxs[dim]:.3f}]")

    if report and all_presets:
        # Show some interesting findings
        print(f"\n{'─' * 60}")
        print("SONIC EXTREMES")
        print(f"{'─' * 60}")

        for dim in ["brightness", "warmth", "movement", "density", "space", "aggression"]:
            sorted_by = sorted(all_presets, key=lambda p: p["dna"][dim])
            lowest = sorted_by[0]
            highest = sorted_by[-1]
            print(f"\n  {dim.upper()}:")
            print(f"    Lowest:  {lowest['name']} ({lowest['dna'][dim]:.3f}) [{lowest['engines'][0]}]")
            print(f"    Highest: {highest['name']} ({highest['dna'][dim]:.3f}) [{highest['engines'][0]}]")

        # Find the most unique preset (highest avg distance from all others)
        print(f"\n{'─' * 60}")
        print("MOST UNIQUE PRESETS (highest avg distance from all others)")
        print(f"{'─' * 60}")
        uniqueness = []
        for preset in all_presets:
            dists = [euclidean_distance(preset["dna"], other["dna"])
                     for other in all_presets if other["name"] != preset["name"]]
            avg_dist = sum(dists) / len(dists) if dists else 0
            uniqueness.append((avg_dist, preset["name"], preset["engines"][0], preset["dna"]))
        uniqueness.sort(reverse=True)
        for dist, name, engine, dna in uniqueness[:10]:
            print(f"  {dist:.3f}  {name} [{engine}]")
            print(f"         B={dna['brightness']:.2f} W={dna['warmth']:.2f} "
                  f"M={dna['movement']:.2f} D={dna['density']:.2f} "
                  f"S={dna['space']:.2f} A={dna['aggression']:.2f}")

        # Find the most common sonic neighborhood
        print(f"\n{'─' * 60}")
        print("MOST CROWDED SONIC NEIGHBORHOODS")
        print(f"{'─' * 60}")
        crowded = []
        for preset in all_presets:
            nearby = sum(1 for other in all_presets
                        if other["name"] != preset["name"]
                        and euclidean_distance(preset["dna"], other["dna"]) < 0.15)
            crowded.append((nearby, preset["name"], preset["engines"][0]))
        crowded.sort(reverse=True)
        for count, name, engine in crowded[:10]:
            print(f"  {count:3d} neighbors  {name} [{engine}]")

        # Show a sample "Find Similar" demo
        print(f"\n{'─' * 60}")
        print("DEMO: Find Similar to 'Astral Pad' [XOdyssey]")
        print(f"{'─' * 60}")
        astral = next((p for p in all_presets if p["name"] == "Astral Pad"), None)
        if astral:
            similar = find_similar(astral["dna"], all_presets, 6)
            for dist, name, mood in similar:
                if name == "Astral Pad": continue
                print(f"  {dist:.3f}  {name} ({mood})")

            print(f"\n  OPPOSITE of 'Astral Pad':")
            opposite = find_opposite(astral["dna"], all_presets, 5)
            for dist, name, mood in opposite:
                print(f"  {dist:.3f}  {name} ({mood})")

    return 0


if __name__ == "__main__":
    sys.exit(main())
