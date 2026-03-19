#!/usr/bin/env python3
"""
XPN Curiosity Engine — XO_OX Designs
The Happy Accident Machine.

Generates discovery manifests from intentional "wrong" approaches:
parameter extremes, DNA contradictions, forbidden configurations,
psychoacoustic boundary exploration, and Gaussian preset mutation.

The Curiosity Engine DISCOVERS. Other tools (xpn_drum_export.py,
xpn_monster_rancher.py) RENDER.

Modes:
  contradiction   — 16-pad kit where adjacent pads oppose each other's DNA
  extremes        — per-parameter musical description of min/max values
  forbidden       — scan preset library for accidental-beauty configurations
  perceptual      — kit targeting psychoacoustic thresholds (JND to infrasound)
  mutate          — Gaussian perturbation of a source preset, scored + ranked
  all             — run all modes and write everything to --output directory

Usage:
    python xpn_curiosity_engine.py --mode contradiction --seed-dna '{"aggression": 0.9}' --pads 16
    python xpn_curiosity_engine.py --mode extremes --engine ONSET
    python xpn_curiosity_engine.py --mode forbidden --presets ./Presets/XOmnibus/
    python xpn_curiosity_engine.py --mode perceptual --output ./kits/
    python xpn_curiosity_engine.py --mode mutate --preset ./Presets/XOmnibus/Foundation/Opal_Crystal_Drift.xometa --n 20
    python xpn_curiosity_engine.py --mode all --output ./curiosity_output/
"""

import argparse
import json
import math
import random
import sys
from pathlib import Path
from typing import Optional

# ---------------------------------------------------------------------------
# Paths
# ---------------------------------------------------------------------------

REPO_ROOT   = Path(__file__).parent.parent
PRESETS_DIR = REPO_ROOT / "Presets" / "XOmnibus"

# ---------------------------------------------------------------------------
# DNA dimensions and helpers
# ---------------------------------------------------------------------------

DNA_DIMENSIONS = ["brightness", "warmth", "aggression", "movement", "density", "space"]

DNA_OPPOSITES = {
    "brightness": "warmth",
    "warmth":     "brightness",
    "aggression": "space",
    "space":      "aggression",
    "movement":   "density",
    "density":    "movement",
}

def clamp(v: float, lo: float = 0.0, hi: float = 1.0) -> float:
    return max(lo, min(hi, v))

def dna_distance(a: dict, b: dict) -> float:
    """Euclidean distance in 6D DNA space."""
    return math.sqrt(sum((a.get(d, 0.5) - b.get(d, 0.5)) ** 2 for d in DNA_DIMENSIONS))

def invert_dna_dimensions(dna: dict, dims: list) -> dict:
    """Flip specified dimensions of a DNA dict (0→1, 1→0)."""
    result = dict(dna)
    for dim in dims:
        result[dim] = 1.0 - dna.get(dim, 0.5)
    return result

def dna_from_preset(preset: dict) -> dict:
    """Extract DNA dict from a loaded .xometa preset."""
    raw = preset.get("sonic_dna") or preset.get("dna") or {}
    return {d: float(raw.get(d, 0.5)) for d in DNA_DIMENSIONS}

def shares_dimensions(dna_a: dict, dna_b: dict, threshold: float = 0.15) -> int:
    """Count how many dimensions are 'similar' (within threshold)."""
    return sum(
        1 for d in DNA_DIMENSIONS
        if abs(dna_a.get(d, 0.5) - dna_b.get(d, 0.5)) < threshold
    )

# ---------------------------------------------------------------------------
# Preset library utilities
# ---------------------------------------------------------------------------

def load_all_presets(presets_dir: Path) -> list[dict]:
    """Recursively load all .xometa files from a directory."""
    presets = []
    for p in sorted(presets_dir.rglob("*.xometa")):
        try:
            with open(p) as f:
                data = json.load(f)
            data["_path"] = str(p)
            presets.append(data)
        except Exception:
            pass
    return presets

def find_closest_preset(target_dna: dict, presets: list[dict]) -> Optional[dict]:
    """Return the preset whose DNA is closest to target_dna."""
    if not presets:
        return None
    return min(presets, key=lambda p: dna_distance(target_dna, dna_from_preset(p)))

# ---------------------------------------------------------------------------
# Interestingness scoring (Mode 5 + general use)
# ---------------------------------------------------------------------------

def score_interestingness(params: dict, param_ranges: dict) -> tuple[float, list[str]]:
    """
    Score a parameter set for interestingness.
    Returns (score, list_of_reasons).
    param_ranges: {param_name: (min, max)}
    """
    score = 0.0
    reasons = []

    # Extract normalized values [0..1]
    normed = {}
    for k, v in params.items():
        lo, hi = param_ranges.get(k, (0.0, 1.0))
        if hi > lo:
            normed[k] = clamp((float(v) - lo) / (hi - lo))
        else:
            normed[k] = 0.5

    # Detect modulation-depth-like params
    mod_keys = [k for k in normed if any(s in k.lower() for s in
                ["depth", "amount", "lfo", "mod", "env", "vibrato", "tremolo"])]
    high_mod = [k for k in mod_keys if normed[k] > 0.7]
    if high_mod:
        score += 10.0 * len(high_mod)
        reasons.append(f"High modulation depth: {', '.join(high_mod[:3])}")

    # Mixed feliX/Oscar — some very bright + some very warm
    bright_keys = [k for k in normed if any(s in k.lower() for s in
                   ["cutoff", "bright", "treble", "high", "presence"])]
    warm_keys   = [k for k in normed if any(s in k.lower() for s in
                   ["warm", "sub", "bass", "low", "body", "resonance", "drive"])]
    has_bright = any(normed[k] > 0.75 for k in bright_keys)
    has_warm   = any(normed[k] > 0.75 for k in warm_keys)
    if has_bright and has_warm:
        score += 20.0
        reasons.append("Mixed feliX/Oscar contradiction: simultaneously bright and warm")

    # Cross-coupled complexity: count params near extremes
    edge_params = [k for k, v in normed.items() if v < 0.05 or v > 0.95]
    if edge_params:
        score += 5.0 * len(edge_params)
        reasons.append(f"{len(edge_params)} params at extremes: {', '.join(edge_params[:4])}")

    # Cross-coupling indicators: reverb + drive both high
    has_reverb = any(normed.get(k, 0) > 0.6 for k in normed
                     if any(s in k.lower() for s in ["reverb", "room", "hall", "space"]))
    has_drive  = any(normed.get(k, 0) > 0.7 for k in normed
                     if any(s in k.lower() for s in ["drive", "sat", "distort", "grit", "crunch"]))
    if has_reverb and has_drive:
        score += 15.0
        reasons.append("Complex interaction: heavy drive + heavy reverb simultaneously")

    # Boring penalty
    mid_band = [k for k, v in normed.items() if 0.35 < v < 0.65]
    if len(mid_band) > len(normed) * 0.75 and len(normed) > 4:
        score -= 20.0
        reasons.append("Boring: most parameters near center")

    return score, reasons

# ---------------------------------------------------------------------------
# Mode 1: DNA Contradiction Kit
# ---------------------------------------------------------------------------

# XPM skeleton for MPC compatibility (pad names pre-populated, no samples yet)
XPM_TEMPLATE = """<?xml version="1.0" encoding="UTF-8"?>
<MPCVObject>
  <Version>2.10</Version>
  <Program type="Drum">
    <ProgramName>{program_name}</ProgramName>
    <Instruments>
{instruments}
    </Instruments>
  </Program>
</MPCVObject>"""

XPM_INSTRUMENT_TEMPLATE = """      <Instrument number="{number}">
        <InstrumentName>{name}</InstrumentName>
        <MIDINote>{note}</MIDINote>
        <Layers/>
      </Instrument>"""

PAD_MIDI_NOTES = [
    36, 37, 38, 39, 40, 41, 42, 43,
    44, 45, 46, 47, 48, 49, 50, 51,
]

def mode_contradiction(seed_dna_input: str, n_pads: int, presets: list[dict],
                       output_dir: Path) -> dict:
    """
    Build a kit where every adjacent pad pair opposes at least 2 DNA dimensions.
    """
    # Parse seed DNA
    try:
        seed_overrides = json.loads(seed_dna_input) if seed_dna_input else {}
    except json.JSONDecodeError:
        seed_overrides = {}

    # Build seed from defaults, apply overrides
    seed = {d: 0.5 for d in DNA_DIMENSIONS}
    seed.update({k: float(v) for k, v in seed_overrides.items() if k in DNA_DIMENSIONS})

    # Generate pad DNA targets
    pad_targets = [dict(seed)]
    dims = DNA_DIMENSIONS.copy()
    random.seed(42)  # reproducible output

    for i in range(1, n_pads):
        prev = pad_targets[-1]
        # Pick 2 dimensions to invert — prefer opposites of previous inversions
        pair = random.sample(dims, 2)
        new_dna = invert_dna_dimensions(prev, pair)
        # Ensure at most 1 shared dimension with previous
        max_attempts = 20
        for _ in range(max_attempts):
            shared = shares_dimensions(prev, new_dna)
            if shared <= 1:
                break
            pair = random.sample(dims, 2)
            new_dna = invert_dna_dimensions(prev, pair)
        pad_targets.append(new_dna)

    # Match each target to a real preset
    pad_assignments = []
    used_names: set[str] = set()

    for i, target in enumerate(pad_targets):
        # Find closest not-yet-used preset
        candidates = [p for p in presets if p.get("name") not in used_names]
        if not candidates:
            candidates = presets  # allow reuse if library is exhausted
        chosen = find_closest_preset(target, candidates)
        if chosen:
            used_names.add(chosen.get("name", ""))
            pad_assignments.append({
                "pad": i + 1,
                "midi_note": PAD_MIDI_NOTES[i] if i < len(PAD_MIDI_NOTES) else 36 + i,
                "preset_name": chosen.get("name", "Unknown"),
                "engine": (chosen.get("engines") or ["Unknown"])[0],
                "target_dna": target,
                "actual_dna": dna_from_preset(chosen),
                "dna_distance": round(dna_distance(target, dna_from_preset(chosen)), 4),
                "source_path": chosen.get("_path", ""),
            })
        else:
            pad_assignments.append({
                "pad": i + 1,
                "midi_note": PAD_MIDI_NOTES[i] if i < len(PAD_MIDI_NOTES) else 36 + i,
                "preset_name": f"[No match — target DNA: {target}]",
                "engine": "Unknown",
                "target_dna": target,
                "actual_dna": {},
                "dna_distance": 99.0,
                "source_path": "",
            })

    # Build contradiction logic explanation
    contradiction_log = []
    for i in range(1, len(pad_assignments)):
        prev_dna = pad_assignments[i - 1]["actual_dna"]
        curr_dna = pad_assignments[i]["actual_dna"]
        opposites = [
            d for d in DNA_DIMENSIONS
            if abs(prev_dna.get(d, 0.5) - curr_dna.get(d, 0.5)) > 0.5
        ]
        contradiction_log.append({
            "from_pad": i,
            "to_pad":   i + 1,
            "inverted_dimensions": opposites,
            "description": (
                f"Pad {i} ({pad_assignments[i-1]['preset_name']}) → "
                f"Pad {i+1} ({pad_assignments[i]['preset_name']}): "
                f"flips {', '.join(opposites) if opposites else 'subtle shift'}"
            ),
        })

    manifest = {
        "mode": "contradiction",
        "seed_dna": seed,
        "n_pads": n_pads,
        "pad_assignments": pad_assignments,
        "contradiction_log": contradiction_log,
        "notes": (
            "Feed pad_assignments into xpn_drum_export.py or xpn_monster_rancher.py. "
            "Each pad is a rendered preset — no common DNA with its neighbors."
        ),
    }

    # Write JSON manifest
    output_dir.mkdir(parents=True, exist_ok=True)
    manifest_path = output_dir / "contradiction_kit.json"
    with open(manifest_path, "w") as f:
        json.dump(manifest, f, indent=2)
    print(f"[contradiction] Manifest → {manifest_path}")

    # Write XPM skeleton
    instruments_xml = "\n".join(
        XPM_INSTRUMENT_TEMPLATE.format(
            number=pad["pad"] - 1,
            name=pad["preset_name"][:32],
            note=pad["midi_note"],
        )
        for pad in pad_assignments
    )
    xpm_content = XPM_TEMPLATE.format(
        program_name="Curiosity_Contradiction",
        instruments=instruments_xml,
    )
    xpm_path = output_dir / "contradiction_kit.xpm"
    with open(xpm_path, "w") as f:
        f.write(xpm_content)
    print(f"[contradiction] XPM skeleton → {xpm_path}")

    return manifest

# ---------------------------------------------------------------------------
# Mode 2: Extreme Parameter Explorer
# ---------------------------------------------------------------------------

# Engine parameter space definitions.
# Format: engine_name → { param_id: (min, max, unit, min_description, max_description) }
# Covers the most musically significant parameters per engine.
ENGINE_PARAM_SPACE = {
    "Onset": {
        "perc_kickAttack":      (0, 50,  "ms",  "instant transient crack",           "slow swell — more thud than attack"),
        "perc_kickDecay":       (5, 500, "ms",  "clicky, hyper-tight",               "long tail, almost tonal kick"),
        "perc_kickPitch":       (20, 200, "Hz",  "sub-infra rumble",                  "high-pitched kick squeak"),
        "perc_kickPitchEnv":    (0, 1,   "norm", "static pitch — no fall",            "massive pitch sweep, classic 808 drop"),
        "perc_snareSnap":       (0, 1,   "norm", "no snap — pure body",               "all snap, almost no body — rim shot territory"),
        "perc_snareTone":       (0, 1,   "norm", "pure noise snare",                  "almost tonal snare — more like timbale"),
        "perc_chatDecay":       (1, 150, "ms",  "ultra-tight hat — tick",            "looser hat — machine gun risk"),
        "perc_ohatDecay":       (50, 2000,"ms",  "quick close — choked",              "long sustaining open hat ring"),
        "perc_clapRoom":        (0, 1,   "norm", "dry clap — acoustic stage absent",  "drenched room clap — cavernous"),
        "perc_fxPitch":         (0, 1,   "norm", "pitched low — floor tom territory", "pitched high — FM bell territory"),
        "perc_reverbSize":      (0, 1,   "norm", "no reverb — dry studio",            "infinite reverb tail — never decays"),
        "perc_masterDrive":     (0, 1,   "norm", "clean bus — transparent",           "total saturation — all voices clip together"),
    },
    "Odyssey": {
        "drift_oscA_mode":      (0, 4,   "int",  "saw wave — harmonically dense",     "noise — completely tonal dissolution"),
        "drift_filterCutoff":   (20, 20000,"Hz", "fully closed — near silence",       "fully open — raw oscillator, no filter character"),
        "drift_filterReso":     (0, 1,   "norm", "flat response — no emphasis",       "self-oscillation — filter becomes oscillator"),
        "drift_envAmount":      (-1, 1,  "bipolar","sweeps down — darker on attack",  "sweeps up — brighter on attack"),
        "drift_lfoRate":        (0.01,100,"Hz",  "0.01 Hz — glacier-slow sweep",      "100 Hz — audio-rate FM from LFO"),
        "drift_lfoDepth":       (0, 1,   "norm", "no modulation",                     "maximum modulation — timbral chaos"),
        "drift_portamento":     (0, 5,   "s",    "instant pitch — no glide",          "5-second glide — whale-song territory"),
        "drift_drift":          (0, 1,   "norm", "locked pitch",                      "massive drift — analog imprecision"),
        "drift_reverbMix":      (0, 1,   "norm", "dry — in your face",                "all reverb — source signal disappears"),
        "drift_delayTime":      (0, 2,   "s",    "very short delay — comb filter range","2-second delay — slapback canyon"),
    },
    "Opal": {
        "opal_grainSize":       (1, 500, "ms",  "micro-grains — spectral smearing",   "large grains — recognizable phrases"),
        "opal_grainDensity":    (1, 200, "/s",  "sparse — individual grains audible", "200/s — continuous shimmer"),
        "opal_grainPitch":      (-24, 24,"semitones","pitched down 2 octaves",         "pitched up 2 octaves"),
        "opal_grainSpread":     (0, 1,   "norm", "mono point-source",                  "full stereo explosion"),
        "opal_grainScatter":    (0, 1,   "norm", "ordered playback — no scatter",      "fully randomized — memory dissolved"),
        "opal_grainReverse":    (0, 1,   "norm", "all forward",                        "all reversed — backward granular"),
        "opal_filterCutoff":    (20, 20000,"Hz", "near silence",                       "raw granular brightness"),
        "opal_freezeAmount":    (0, 1,   "norm", "live granular — moving forward",     "frozen — infinite grain loop"),
        "opal_reverbSize":      (0, 1,   "norm", "dry — close-mic granular",           "cathedral size — 60s+ decay"),
        "opal_playbackRate":    (0.1, 4, "x",   "0.1x — extremely slow, time-stretched","4x — chipmunk speed"),
    },
    "Obese": {
        "fat_satDrive":         (0, 1,   "norm", "clean — no saturation",              "total ladder saturation — pure fuzz"),
        "fat_filterCutoff":     (20, 2000,"Hz",  "sub-only — fundamental vanishes",    "open ladder — full harmonics exposed"),
        "fat_filterReso":       (0, 1,   "norm", "flat ladder response",               "ladder self-oscillation — sine wave bass"),
        "fat_subLevel":         (0, 1,   "norm", "no sub octave",                      "pure sub focus — fundamental disappears"),
        "fat_mojoAnalog":       (0, 1,   "norm", "digital precision — clean",          "full analog warmth — detuned, thick"),
        "fat_compRatio":        (1, 20,  "x",    "no compression — dynamic",           "20:1 — total compression brick wall"),
        "fat_envAttack":        (0.1, 500,"ms",  "instant — click-in transient",       "500ms swell — no attack present"),
    },
    "Overworld": {
        "ow_era":               (0, 1,   "norm", "pure NES 2A03 — 8-bit",              "pure SNES SPC700 — sampled-quality chip"),
        "ow_glitchType":        (0, 12,  "int",  "no glitch — clean chip",             "maximum glitch — bit-corrupt territory"),
        "ow_glitchDepth":       (0, 1,   "norm", "clean output",                       "deep glitch — signal destruction"),
        "ow_filterCutoff":      (20, 20000,"Hz", "muffled retro — band-limited chip",  "raw chip output — no filtering"),
        "ow_lfoRate":           (0.01, 200,"Hz", "0.01 Hz tremolo — barely there",     "200 Hz — audio-rate chip vibrato (FM)"),
        "ow_reverbMix":         (0, 1,   "norm", "dry chip — zero room",               "wet chip — chip in a cathedral"),
        "ow_dutyWave":          (0, 1,   "norm", "narrow pulse — nasal/hollow",        "50% duty — warm square wave"),
    },
    "Overdub": {
        "dub_sendAmount":       (0, 1,   "norm", "dry signal only",                    "fully wet — signal is only reverb/echo"),
        "dub_tapeDelayTime":    (0.01, 2,"s",    "short delay — flutter/phasing",      "2s delay — echo building on echo"),
        "dub_tapeWow":          (0, 1,   "norm", "locked tape — pristine",             "maximum wow — seasick pitch waver"),
        "dub_springMix":        (0, 1,   "norm", "no spring — clean dub",              "full spring — metallic splash identity"),
        "dub_driveAmount":      (0, 1,   "norm", "clean — transparent signal chain",   "heavy drive — dub distortion aesthetic"),
        "dub_reverbDecay":      (0.1, 30,"s",    "tight room",                         "30-second reverb — never decays"),
        "dub_filterCutoff":     (100, 15000,"Hz","dark, muffled dub",                  "bright, open dub channel"),
    },
}

# "Interesting extreme" heuristic: parameters where extreme ≠ silence/noise
INTERESTING_EXTREMES = {
    "drift_filterReso",     # self-oscillation at max
    "drift_lfoRate",        # audio-rate FM at max
    "opal_grainDensity",    # continuous shimmer at max
    "opal_grainSize",       # micro-grains at min
    "opal_freezeAmount",    # frozen granular at max
    "fat_filterReso",       # ladder self-oscillation
    "perc_kickPitchEnv",    # massive 808 drop
    "ow_lfoRate",           # audio-rate chip FM
    "dub_reverbDecay",      # room that never decays
    "drift_portamento",     # whale-song glide
    "drift_envAmount",      # bipolar — downward sweep
    "perc_ohatDecay",       # long ring
    "fat_satDrive",         # full ladder saturation is musical
}

def mode_extremes(engine_name: str, output_dir: Path) -> dict:
    """
    Generate extreme parameter report for a given engine.
    """
    # Normalize engine name to key
    engine_key = engine_name.strip()
    # Try direct match first, then case-insensitive
    if engine_key not in ENGINE_PARAM_SPACE:
        for k in ENGINE_PARAM_SPACE:
            if k.lower() == engine_key.lower():
                engine_key = k
                break

    if engine_key not in ENGINE_PARAM_SPACE:
        available = ", ".join(sorted(ENGINE_PARAM_SPACE.keys()))
        print(f"[extremes] Unknown engine '{engine_name}'. Available: {available}", file=sys.stderr)
        engine_key = list(ENGINE_PARAM_SPACE.keys())[0]
        print(f"[extremes] Falling back to {engine_key}", file=sys.stderr)

    params = ENGINE_PARAM_SPACE[engine_key]
    report = []

    for param_id, (lo, hi, unit, min_desc, max_desc) in params.items():
        is_interesting_min = param_id in INTERESTING_EXTREMES and (
            "near silence" not in min_desc and "no " not in min_desc.lower()
        )
        is_interesting_max = param_id in INTERESTING_EXTREMES

        # Determine recommended "accident" value
        if param_id in INTERESTING_EXTREMES:
            # If max is interesting, go to max; if min described as interesting, go to min
            if "FM" in max_desc or "self-oscillation" in max_desc or "frozen" in max_desc:
                accident_value = hi
                accident_note  = "audio-rate modulation / resonance singularity — FM territory"
            elif "downward" in min_desc or "sub-infra" in min_desc:
                accident_value = lo
                accident_note  = "subterranean register — felt not heard"
            else:
                accident_value = hi
                accident_note  = "maximum character — the edge of what this parameter is for"
        else:
            accident_value = None
            accident_note  = "both extremes tend toward silence or featureless noise"

        report.append({
            "param_id":           param_id,
            "range":              [lo, hi],
            "unit":               unit,
            "min_description":    min_desc,
            "max_description":    max_desc,
            "interesting_min":    is_interesting_min,
            "interesting_max":    is_interesting_max,
            "accident_value":     accident_value,
            "accident_note":      accident_note,
        })

    manifest = {
        "mode":    "extremes",
        "engine":  engine_key,
        "params":  report,
        "summary": (
            f"{len(report)} parameters analyzed for {engine_key}. "
            f"{sum(1 for r in report if r['interesting_max'] or r['interesting_min'])} "
            f"flagged as 'interesting extreme' (novel timbre, not silence/noise)."
        ),
        "notes": (
            "accident_value is the recommended 'wrong' value to try first. "
            "Combine multiple accident_values in a single patch for maximum chaos."
        ),
    }

    output_dir.mkdir(parents=True, exist_ok=True)
    out_path = output_dir / f"extreme_parameters_{engine_key.lower()}.json"
    with open(out_path, "w") as f:
        json.dump(manifest, f, indent=2)
    print(f"[extremes] Report → {out_path}")
    return manifest

# ---------------------------------------------------------------------------
# Mode 3: Forbidden Configuration Finder
# ---------------------------------------------------------------------------

# Rules: each rule is (label, description, test_fn)
# test_fn(params_flat: dict) → bool (True = rule triggered)

def _get_flat_params(preset: dict) -> dict:
    """Flatten all engine parameter dicts into a single {key: value} dict."""
    flat = {}
    for engine_params in preset.get("parameters", {}).values():
        if isinstance(engine_params, dict):
            flat.update(engine_params)
    return flat

FORBIDDEN_RULES = [
    (
        "Ghost Transient",
        "Attack < 1ms AND Decay < 5ms — almost no sound duration. "
        "Sometimes produces a pure click or air pressure pop. Beautiful as texture layer.",
        lambda p: (
            any(p.get(k, 999) < 1.0 for k in p if "attack" in k.lower())
            and any(p.get(k, 999) < 5.0 for k in p if "decay" in k.lower())
        ),
    ),
    (
        "Sub-Fundamental Filter",
        "Filter cutoff set below the expected fundamental pitch (< 80Hz for mid-register sounds). "
        "Creates a null zone — the note disappears into silence. Surprisingly evocative.",
        lambda p: any(p.get(k, 9999) < 80.0 for k in p
                      if "cutoff" in k.lower() and "filter" in k.lower()),
    ),
    (
        "Audio-Rate LFO (FM Territory)",
        "LFO rate > 20Hz — exits modulation territory and enters FM synthesis. "
        "Sidebands appear, timbre becomes metallic or clangorous.",
        lambda p: any(p.get(k, 0) > 20.0 for k in p
                      if "lfo" in k.lower() and "rate" in k.lower()),
    ),
    (
        "Infinite Room",
        "Reverb time / size > 10 seconds (or normalized > 0.95). "
        "The room never decays — pad sounds never resolve. Drone territory.",
        lambda p: any(
            (p.get(k, 0) > 10.0 if "time" in k.lower() else p.get(k, 0) > 0.95)
            for k in p if "reverb" in k.lower() and
            any(s in k.lower() for s in ["size", "time", "decay", "length"])
        ),
    ),
    (
        "Total Saturation",
        "Drive/saturation parameter > 90% (normalized > 0.9). "
        "Near-complete waveform folding. Harmonics dominate; fundamental may vanish.",
        lambda p: any(p.get(k, 0) > 0.9 for k in p
                      if any(s in k.lower() for s in ["drive", "sat", "distort", "grit", "crunch"])),
    ),
    (
        "Resonance Singularity",
        "Filter resonance > 0.92 (normalized) — approaching or at self-oscillation. "
        "Filter becomes a sine-wave oscillator. Note-following creates pitched filter.",
        lambda p: any(p.get(k, 0) > 0.92 for k in p
                      if any(s in k.lower() for s in ["reso", "resonance", "q", "peak"])),
    ),
    (
        "Maximum Portamento",
        "Portamento/glide > 3 seconds. Notes become whale-song glissandos. "
        "Melody and harmony dissolve — only contour survives.",
        lambda p: any(p.get(k, 0) > 3.0 for k in p
                      if any(s in k.lower() for s in ["port", "glide", "slide"])),
    ),
    (
        "Frozen Granular",
        "Granular freeze > 0.9 with grain density > 100/s. "
        "Source position locked; dense grain cloud of a single moment in time.",
        lambda p: (
            any(p.get(k, 0) > 0.9 for k in p if "freeze" in k.lower())
            and any(p.get(k, 0) > 100 for k in p if "density" in k.lower())
        ),
    ),
    (
        "Inverted Envelope",
        "Envelope amount < 0 (bipolar parameter). Filter sweeps DOWN on note attack — "
        "gets darker as the transient hits. Counter-intuitive; often compelling.",
        lambda p: any(p.get(k, 0) < 0 for k in p
                      if any(s in k.lower() for s in ["envamount", "env_amount", "envamt"])),
    ),
    (
        "Maximum Unison Spread",
        "Unison detune > 40 cents or detune normalized > 0.85. "
        "Voices smear across a narrow frequency band — chord-like shimmer from one note.",
        lambda p: any(p.get(k, 0) > 40.0 for k in p if "detune" in k.lower()),
    ),
]

def mode_forbidden(presets_dir: Path, output_dir: Path) -> dict:
    """
    Scan preset library for presets matching forbidden configuration rules.
    """
    presets = load_all_presets(presets_dir)
    findings = []

    for preset in presets:
        flat = _get_flat_params(preset)
        triggered = []
        for label, description, test_fn in FORBIDDEN_RULES:
            try:
                if test_fn(flat):
                    triggered.append({"rule": label, "description": description})
            except Exception:
                pass

        if triggered:
            # Score interestingness — only numeric params
            numeric_flat = {}
            for k, v in flat.items():
                try:
                    numeric_flat[k] = float(v)
                except (TypeError, ValueError):
                    pass
            param_ranges = {k: (0.0, 1.0) for k in numeric_flat}
            score, reasons = score_interestingness(numeric_flat, param_ranges)
            findings.append({
                "preset_name":   preset.get("name", "Unknown"),
                "engine":        (preset.get("engines") or ["Unknown"])[0],
                "mood":          preset.get("mood", "Unknown"),
                "source_path":   preset.get("_path", ""),
                "rules_triggered": triggered,
                "interestingness_score": round(score, 1),
                "interestingness_reasons": reasons,
                "verdict": (
                    "accidentally_interesting" if score > 15 else
                    "worth_trying" if score > 0 else
                    "borderline"
                ),
            })

    # Sort by interestingness
    findings.sort(key=lambda x: x["interestingness_score"], reverse=True)

    manifest = {
        "mode":      "forbidden",
        "scanned":   len(presets),
        "triggered": len(findings),
        "rules":     [{"label": r[0], "description": r[1]} for r in FORBIDDEN_RULES],
        "findings":  findings,
        "notes": (
            "Each finding is a real preset that violates a 'common sense' rule. "
            "verdict='accidentally_interesting' means the violation creates novel character. "
            "verdict='worth_trying' means the violation is detectable but may be intentional."
        ),
    }

    output_dir.mkdir(parents=True, exist_ok=True)
    out_path = output_dir / "forbidden_configurations.json"
    with open(out_path, "w") as f:
        json.dump(manifest, f, indent=2)
    print(f"[forbidden] Scanned {len(presets)} presets → {len(findings)} findings → {out_path}")
    return manifest

# ---------------------------------------------------------------------------
# Mode 4: Perceptual Boundary Kit
# ---------------------------------------------------------------------------

PERCEPTUAL_PADS = [
    # Pad 1–4: Pitch/time perception thresholds
    {
        "pad": 1, "name": "JND Pitch",
        "threshold": "Just-Noticeable Difference in pitch: 1 cent above unison",
        "target_hz": None,
        "target_cents_detune": 1,
        "description": (
            "Barely-there variation. Two voices detuned by 1 cent. "
            "Most listeners cannot consciously detect the difference — "
            "but the sound is subtly 'wider' and more organic than a true unison."
        ),
        "recommended_params": {"detune": 1, "unison": 2},
        "psychoacoustic_note": "JND for pitch ≈ 1–5 cents depending on frequency and training.",
    },
    {
        "pad": 2, "name": "Flutter Fusion",
        "threshold": "Amplitude modulation at ~30Hz — flutter fuses into roughness",
        "target_hz": 30,
        "description": (
            "Below ~20Hz: discrete flutter is audible as rhythm. "
            "Above ~30Hz: the flutter fuses — perceived as roughness or 'buzzing'. "
            "30Hz is the perceptual boundary. At exactly this rate, the timbre wavers between states."
        ),
        "recommended_params": {"lfo_rate": 30, "lfo_depth": 0.4, "lfo_target": "amplitude"},
        "psychoacoustic_note": "Roughness onset ≈ 15–40Hz depending on frequency region.",
    },
    {
        "pad": 3, "name": "Haas Boundary",
        "threshold": "35ms delay — Haas precedence effect onset",
        "target_delay_ms": 35,
        "description": (
            "Below 35ms: the delay fuses with the source — perceived as one widened sound. "
            "Above 35ms: the delay becomes a distinct echo. "
            "At exactly 35ms: the sound is ambiguously fused/separate — stereo image is unstable."
        ),
        "recommended_params": {"delay_time_ms": 35, "delay_level": 0.85, "delay_pan": 1.0},
        "psychoacoustic_note": "Haas effect / precedence effect: 1–35ms fusion zone.",
    },
    {
        "pad": 4, "name": "Masking Threshold",
        "threshold": "A sound that disappears behind a louder one",
        "description": (
            "High-level broadband noise at 80dB masks a 1kHz tone up to ~50dB. "
            "Use a dense pad/noise layer at high level. Program a secondary voice "
            "that is technically present but perceptually inaudible — 'invisible harmonic'."
        ),
        "recommended_params": {"noise_level": 0.9, "tone_level": 0.15, "tone_hz": 1000},
        "psychoacoustic_note": "Simultaneous masking: masker raises threshold of masked signal by up to 50dB.",
    },
    # Pad 5–8: Absolute threshold frequencies (quietest audible levels per octave band)
    {
        "pad": 5, "name": "Threshold 125Hz",
        "threshold": "Absolute threshold of hearing at 125Hz: ~22dB SPL",
        "target_hz": 125,
        "description": (
            "At 125Hz the ear requires ~22dB SPL to detect a tone. "
            "Set this pad's level to the quietest possible audible bass tone. "
            "A presence felt as much as heard."
        ),
        "recommended_params": {"fundamental_hz": 125, "level_normalized": 0.12},
    },
    {
        "pad": 6, "name": "Threshold 1kHz",
        "threshold": "Absolute threshold of hearing at 1kHz: ~0dB SPL (reference)",
        "target_hz": 1000,
        "description": (
            "1kHz is the reference point for hearing sensitivity. "
            "The ear is most sensitive here. Extremely quiet 1kHz tones are audible. "
            "Design for near-silence — a ghost tone."
        ),
        "recommended_params": {"fundamental_hz": 1000, "level_normalized": 0.04},
    },
    {
        "pad": 7, "name": "Threshold 4kHz",
        "threshold": "Absolute threshold of hearing at 4kHz: ~-10dB SPL (most sensitive region)",
        "target_hz": 4000,
        "description": (
            "4kHz is near the ear canal resonance peak — the most sensitive frequency region. "
            "Extremely quiet 4kHz content is audible when 60Hz content at the same SPL is not. "
            "A whisper in the presence range."
        ),
        "recommended_params": {"fundamental_hz": 4000, "level_normalized": 0.03},
    },
    {
        "pad": 8, "name": "Threshold 10kHz",
        "threshold": "Absolute threshold of hearing at 10kHz: ~20dB SPL",
        "target_hz": 10000,
        "description": (
            "Sensitivity drops rapidly above 4kHz. "
            "A 10kHz tone must be ~20dB louder than a 1kHz tone to be perceived as equally loud. "
            "Design for a barely-audible high shimmer — more air than tone."
        ),
        "recommended_params": {"fundamental_hz": 10000, "level_normalized": 0.15},
    },
    # Pad 9–12: Pain threshold approach
    {
        "pad": 9,  "name": "Danger 100dB",
        "threshold": "100dB SPL — threshold of discomfort",
        "target_db_spl": 100,
        "description": (
            "At 100dB the sound becomes uncomfortable. Aural fatigue begins. "
            "In context: a pad that makes the listener lean back. Design for maximum density/energy."
        ),
        "recommended_params": {"output_level": 0.85, "drive": 0.7, "compression_ratio": 8},
    },
    {
        "pad": 10, "name": "Danger 110dB",
        "threshold": "110dB SPL — pain threshold approach",
        "target_db_spl": 110,
        "description": (
            "110dB is above the recommended maximum safe exposure level (OSHA: 2min/day). "
            "Design for maximum aggression: near-total saturation, high density, compressed."
        ),
        "recommended_params": {"output_level": 0.92, "drive": 0.85, "compression_ratio": 12},
    },
    {
        "pad": 11, "name": "Danger 115dB",
        "threshold": "115dB SPL — OSHA 15-second safe limit",
        "target_db_spl": 115,
        "description": (
            "At 115dB OSHA limits exposure to 15 seconds. "
            "Design for maximum physical impact: full saturation, sub-bass reinforcement, maximum transient."
        ),
        "recommended_params": {"output_level": 0.97, "drive": 0.95, "sub_boost": 0.9},
    },
    {
        "pad": 12, "name": "Pain Threshold 120dB",
        "threshold": "120dB SPL — pain threshold",
        "target_db_spl": 120,
        "description": (
            "120dB is the nominal pain threshold. Live concert peak levels. "
            "Design for absolute maximum energy in the mix. "
            "This pad should be used with extreme caution in any mastered context."
        ),
        "recommended_params": {"output_level": 1.0, "drive": 1.0, "limiter": "on", "sub_boost": 1.0},
    },
    # Pad 13–16: Infrasound boundary (felt not heard)
    {
        "pad": 13, "name": "Infrasound 20Hz",
        "threshold": "20Hz — lower boundary of human hearing",
        "target_hz": 20,
        "description": (
            "20Hz is the canonical lower limit of hearing. Most people cannot hear a pure 20Hz tone "
            "— but they feel it as pressure, unease, or slight nausea (the Vic Tandy effect). "
            "Design for physical presence, not pitch."
        ),
        "recommended_params": {"fundamental_hz": 20, "level_normalized": 0.9, "harmonic_2": 0.3},
        "psychoacoustic_note": "The Vic Tandy effect: 19Hz standing waves cause unease in spaces.",
    },
    {
        "pad": 14, "name": "Infrasound 15Hz",
        "threshold": "15Hz — below audible hearing",
        "target_hz": 15,
        "description": (
            "15Hz is unambiguously infrasound. Perceived only as physical sensation or "
            "as a slow amplitude modulation if harmonics are present. "
            "Design for pressure — the sound the body hears, not the ears."
        ),
        "recommended_params": {"fundamental_hz": 15, "level_normalized": 0.95, "harmonic_2": 0.5, "harmonic_3": 0.2},
    },
    {
        "pad": 15, "name": "Infrasound 10Hz",
        "threshold": "10Hz — deep infrasound",
        "target_hz": 10,
        "description": (
            "10Hz infrasound. The fundamental is inaudible. If harmonics (20Hz, 30Hz, 40Hz) "
            "are present they define the timbre. The 10Hz base creates rhythmic beating "
            "between harmonics — felt as subtle physical pulse."
        ),
        "recommended_params": {"fundamental_hz": 10, "harmonic_2": 0.8, "harmonic_3": 0.6, "harmonic_4": 0.4},
    },
    {
        "pad": 16, "name": "Infrasound 5Hz",
        "threshold": "5Hz — extreme infrasound / geological territory",
        "target_hz": 5,
        "description": (
            "5Hz is the frequency of seismic activity and slow geological processes. "
            "Completely inaudible as pitch. Only harmonics survive in the hearing range. "
            "Design as a rhythmic foundation — 5Hz = 300 BPM in a sense the ear cannot parse."
        ),
        "recommended_params": {"fundamental_hz": 5, "harmonic_2": 0.9, "harmonic_3": 0.7, "harmonic_4": 0.5, "harmonic_5": 0.3},
    },
]

def mode_perceptual(output_dir: Path) -> dict:
    """
    Generate a 16-pad kit manifest targeting specific psychoacoustic thresholds.
    """
    manifest = {
        "mode": "perceptual",
        "description": (
            "16-pad kit targeting psychoacoustic thresholds. "
            "Each pad is tuned to a specific perceptual boundary: "
            "JND pitch, flutter fusion, Haas effect, masking, "
            "absolute hearing thresholds (125Hz–10kHz), "
            "pain threshold approach (100–120dB), "
            "and infrasound (20Hz–5Hz)."
        ),
        "pads": PERCEPTUAL_PADS,
        "reference_sources": [
            "Fletcher & Munson (1933) — equal-loudness contours",
            "Haas (1949) — precedence effect / Haas effect (1–35ms fusion zone)",
            "Vic Tandy + Tony Lawrence (1998) — infrasound and unease (19Hz)",
            "Zwicker & Fastl — Psychoacoustics: Facts and Models",
            "OSHA 1910.95 — occupational noise exposure limits",
        ],
        "notes": (
            "These are parameter TARGETS, not rendered WAV files. "
            "Feed into xpn_drum_export.py or xpn_keygroup_export.py with matching engine presets. "
            "Pads 13–16 (infrasound) require a synthesizer capable of sub-20Hz fundamental output; "
            "most engines will produce only the harmonic content in practice."
        ),
    }

    output_dir.mkdir(parents=True, exist_ok=True)
    out_path = output_dir / "perceptual_boundary_kit.json"
    with open(out_path, "w") as f:
        json.dump(manifest, f, indent=2)
    print(f"[perceptual] 16-pad manifest → {out_path}")
    return manifest

# ---------------------------------------------------------------------------
# Mode 5: Preset Mutation Harvester
# ---------------------------------------------------------------------------

def _extract_numeric_params(preset: dict) -> tuple[dict, dict]:
    """
    Extract flat numeric parameters + infer ranges.
    Returns (params_flat, param_ranges).
    param_ranges: {key: (min_seen, max_seen)} — uses 0..1 default, expanded by value.
    """
    flat = {}
    for engine_params in preset.get("parameters", {}).values():
        if isinstance(engine_params, dict):
            for k, v in engine_params.items():
                try:
                    flat[k] = float(v)
                except (TypeError, ValueError):
                    pass

    # Build ranges: params in [0,1] stay [0,1]; outside → expand
    ranges = {}
    for k, v in flat.items():
        lo, hi = 0.0, 1.0
        # Common known wide-range params
        if any(s in k.lower() for s in ["hz", "freq", "cutoff"]):
            lo, hi = 20.0, 20000.0
        elif any(s in k.lower() for s in ["attack", "decay", "release"]):
            lo, hi = 0.0, 5000.0
        elif any(s in k.lower() for s in ["time", "delay"]):
            lo, hi = 0.0, 10.0
        elif any(s in k.lower() for s in ["ratio", "bpm", "rate"]) and v > 1.0:
            lo, hi = 0.0, 200.0
        # Expand based on actual value
        lo = min(lo, v * 0.1 if v > 0 else lo)
        hi = max(hi, v * 2.0 if v > hi else hi)
        ranges[k] = (lo, hi)

    return flat, ranges

def _perturb_params(params: dict, ranges: dict, sigma: float = 0.1,
                    rng: random.Random = None) -> dict:
    """
    Perturb each numeric parameter by sigma × parameter_range (Gaussian).
    Clamps result to [lo, hi].
    """
    if rng is None:
        rng = random.Random()
    result = {}
    for k, v in params.items():
        lo, hi = ranges.get(k, (0.0, 1.0))
        span = hi - lo
        delta = rng.gauss(0, sigma * span)
        result[k] = clamp(v + delta, lo, hi)
    return result

def _euclidean_distance(a: dict, b: dict, keys: list) -> float:
    """Euclidean distance between two param dicts over given keys."""
    return math.sqrt(sum((a.get(k, 0) - b.get(k, 0)) ** 2 for k in keys))

def mode_mutate(preset_path: Path, n: int, output_dir: Path) -> dict:
    """
    Load a source preset, generate N Gaussian mutations, score + rank them.
    """
    if not preset_path.exists():
        print(f"[mutate] Preset not found: {preset_path}", file=sys.stderr)
        sys.exit(1)

    with open(preset_path) as f:
        source_preset = json.load(f)

    source_name = source_preset.get("name", preset_path.stem)
    source_params, param_ranges = _extract_numeric_params(source_preset)
    param_keys = list(source_params.keys())

    if not param_keys:
        print(f"[mutate] No numeric parameters found in preset: {preset_path}", file=sys.stderr)
        sys.exit(1)

    mutations = []
    rng = random.Random(12345)  # reproducible

    for i in range(n):
        mutated = _perturb_params(source_params, param_ranges, sigma=0.1, rng=rng)
        dist    = _euclidean_distance(source_params, mutated, param_keys)
        score, reasons = score_interestingness(mutated, param_ranges)
        mutations.append({
            "mutation_id": i + 1,
            "params":      {k: round(v, 6) for k, v in mutated.items()},
            "distance_from_source": round(dist, 4),
            "interestingness_score": round(score, 1),
            "interestingness_reasons": reasons,
        })

    # Rank by: most different AND most interesting
    # Combined score = (normalized_distance * 0.6) + (normalized_interest * 0.4)
    max_dist     = max(m["distance_from_source"] for m in mutations) or 1.0
    max_interest = max(abs(m["interestingness_score"]) for m in mutations) or 1.0

    for m in mutations:
        norm_dist  = m["distance_from_source"] / max_dist
        norm_int   = max(0, m["interestingness_score"]) / max_interest
        m["combined_score"] = round(norm_dist * 0.6 + norm_int * 0.4, 4)

    mutations.sort(key=lambda x: x["combined_score"], reverse=True)
    top5 = mutations[:5]

    manifest = {
        "mode":         "mutate",
        "source_preset": source_name,
        "source_path":   str(preset_path),
        "n_mutations":   n,
        "sigma":         0.1,
        "param_count":   len(param_keys),
        "scoring": {
            "combined_score": "0.6 × normalized_distance + 0.4 × normalized_interestingness",
            "goal": "Most different from source while still being interesting (not just random noise)",
        },
        "top_5_mutations": top5,
        "all_mutations":   mutations,
        "notes": (
            "top_5_mutations are the recommended 'happy accidents'. "
            "Each mutation is a complete parameter set — copy into a new .xometa file "
            "and use xpn_render_spec.py to determine the render strategy."
        ),
    }

    output_dir.mkdir(parents=True, exist_ok=True)
    out_path = output_dir / f"mutation_harvest_{preset_path.stem}.json"
    with open(out_path, "w") as f:
        json.dump(manifest, f, indent=2)

    print(f"[mutate] {n} mutations → top 5 scored → {out_path}")
    print(f"  Source: {source_name}  ({len(param_keys)} params)")
    for i, m in enumerate(top5):
        print(f"  #{i+1} mutation_{m['mutation_id']:02d}: "
              f"dist={m['distance_from_source']:.3f} "
              f"interest={m['interestingness_score']:.1f} "
              f"combined={m['combined_score']:.3f}")

    return manifest

# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def build_parser() -> argparse.ArgumentParser:
    p = argparse.ArgumentParser(
        description="XPN Curiosity Engine — the happy accident machine",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    p.add_argument("--mode", required=True,
                   choices=["contradiction", "extremes", "forbidden",
                            "perceptual", "mutate", "all"],
                   help="Which discovery mode to run")
    p.add_argument("--output", default="./curiosity_output",
                   help="Output directory for JSON manifests (default: ./curiosity_output)")
    # Mode-specific
    p.add_argument("--seed-dna", default="{}",
                   help='JSON dict of DNA overrides for contradiction mode. '
                        'E.g.: \'{"aggression": 0.9, "brightness": 0.2}\'')
    p.add_argument("--pads", type=int, default=16,
                   help="Number of pads for contradiction kit (default: 16)")
    p.add_argument("--engine", default="Onset",
                   help="Engine name for extremes mode (default: Onset). "
                        f"Available: {', '.join(sorted(ENGINE_PARAM_SPACE.keys()))}")
    p.add_argument("--presets", default=None,
                   help="Path to preset library root for forbidden mode "
                        "(default: auto-detect from repo structure)")
    p.add_argument("--preset", default=None,
                   help="Path to a single .xometa preset for mutate mode")
    p.add_argument("--n", type=int, default=20,
                   help="Number of mutations for mutate mode (default: 20)")
    return p


def main() -> None:
    parser = build_parser()
    args   = parser.parse_args()

    output_dir = Path(args.output)

    # Resolve presets directory
    if args.presets:
        presets_dir = Path(args.presets)
    else:
        presets_dir = PRESETS_DIR

    if not presets_dir.exists():
        print(f"[warn] Presets directory not found: {presets_dir}. "
              "Contradiction and forbidden modes will have no preset matches.", file=sys.stderr)

    mode = args.mode

    if mode in ("contradiction", "all"):
        presets = load_all_presets(presets_dir) if presets_dir.exists() else []
        if not presets and mode == "contradiction":
            print("[warn] No presets loaded — contradiction kit will have placeholder names.",
                  file=sys.stderr)
        mode_contradiction(args.seed_dna, args.pads, presets, output_dir)

    if mode in ("extremes", "all"):
        if mode == "all":
            for engine_name in ENGINE_PARAM_SPACE:
                mode_extremes(engine_name, output_dir)
        else:
            mode_extremes(args.engine, output_dir)

    if mode in ("forbidden", "all"):
        if presets_dir.exists():
            mode_forbidden(presets_dir, output_dir)
        else:
            print(f"[forbidden] Presets dir not found: {presets_dir} — skipping.", file=sys.stderr)

    if mode in ("perceptual", "all"):
        mode_perceptual(output_dir)

    if mode in ("mutate", "all"):
        if mode == "mutate":
            if not args.preset:
                print("[mutate] --preset is required for mutate mode.", file=sys.stderr)
                sys.exit(1)
            mode_mutate(Path(args.preset), args.n, output_dir)
        else:
            # In 'all' mode: pick first available preset as demo
            presets = load_all_presets(presets_dir) if presets_dir.exists() else []
            if presets:
                demo_path = Path(presets[0]["_path"])
                print(f"[all/mutate] Demo mutation on: {demo_path.name}")
                mode_mutate(demo_path, args.n, output_dir)
            else:
                print("[all/mutate] No presets available for demo mutation — skipping.",
                      file=sys.stderr)

    print(f"\n[curiosity] All output written to: {output_dir.resolve()}")


if __name__ == "__main__":
    main()
