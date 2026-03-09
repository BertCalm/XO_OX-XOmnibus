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
    """Compute DNA for XOddCouple presets."""
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
    """Compute DNA for XOblongBob presets."""
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


# Engine dispatcher
DNA_FUNCTIONS = {
    "XOddCouple": dna_xoddcouple,
    "XOverdub":   dna_xoverdub,
    "XOdyssey":   dna_xodyssey,
    "XOblongBob": dna_xoblongbob,
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
            # Engine not supported yet (XObese, XOnset)
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
