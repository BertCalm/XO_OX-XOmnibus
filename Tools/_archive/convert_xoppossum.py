#!/usr/bin/env python3
"""
convert_xoppossum.py — Convert XOppossum presets to XOceanus .xometa format.

Reads source JSON from XOppossum Factory presets, applies parameter mapping
and value transformations, auto-assigns mood and 6D Sonic DNA, then writes
.xometa files to the appropriate XOceanus mood/Overbite subdirectory.
"""

import json
import math
import os
import re
import sys
from pathlib import Path

# ---------------------------------------------------------------------------
# Paths
# Accept SOURCE_DIR as argv[1]; default to ~/XOppossum/Presets/Factory.
# OUTPUT_BASE is derived relative to this script's location.
# ---------------------------------------------------------------------------
_DEFAULT_SOURCE = Path.home() / "XOppossum" / "Presets" / "Factory"
if len(sys.argv) > 1:
    SOURCE_DIR = Path(sys.argv[1])
else:
    SOURCE_DIR = _DEFAULT_SOURCE

if not SOURCE_DIR.is_dir():
    sys.exit(
        f"Error: source directory not found: {SOURCE_DIR}\n"
        f"Usage: python {sys.argv[0]} [/path/to/XOppossum/Presets/Factory]"
    )

OUTPUT_BASE = Path(__file__).resolve().parent.parent / "Presets" / "XOceanus"

# ---------------------------------------------------------------------------
# Curated preset list (filenames without extension)
# ---------------------------------------------------------------------------
CURATED = [
    "Whale_Song_Drone",
    "Aurora_Drone",
    "Solar_Wind_Drone",
    "Magnetic_Field_Drone",
    "Frozen_Lake_Drone",
    "Datastream_Drone",
    "Subterranean_Drone",
    "Ritual_Chant_Drone",
    "Feral_Howl",
    "Acid_Screech_Lead",
    "LFO3_Chaos",
    "Wire_Fang",
    "Scorched_Lead",
    "Molten_Lead",
    "Razor_Detune",
    "Grit_Solo",
    "Miami_Vice_Lead",
    "Velocity_Bite",
    "Wobble_Beast",
    "Reese_Wobble_Bass",
    "Ghost_Bass",
    "Fog_Horn_Bass",
    "Rubber_Band_Bass",
    "Post_Noise_Bass",
    "Thorn_Bass",
    "Tunnel_Bass",
    "Midnight_808",
    "Hammer_Bass",
    "Gutter_Bass",
    "Sub_Triangle_Bass",
    "Circuit_Bass",
    "Anvil_Bass",
    "Possum_Choir",
    "Neon_Rain",
    "Blade_Runner_Pad",
    "Fur_Blanket_Pad",
    "Crystal_Shimmer_Pad",
    "Smog_Layer_Pad",
    "Warm_Blanket_Pad",
    "Dark_Matter_Pad",
    "Swarm_Pad",
    "Phosphor",
    "Nocturnal_Caress",
    "Clav_Bite_Keys",
    "Velvet_Rhodes_Keys",
    "Soft_Rhodes_Keys",
    "Honky_Tonk_Keys",
    "Glass_Marimba",
    "Kalimba_Bell",
    "Digital_Harp",
    "Dulcimer_Ring",
    "Erhu_Lament",
    "Solo_Violin",
    "Tremolo_Strings",
    "Chamber_Strings",
    "Slow_Strings",
    "Ice_Violin",
    "Dark_Cello",
    "Fairlight_Stabs",
    "Chrome_Boulevard_80s",
    "Stranger_Things_80s",
    "Tron_Legacy_80s",
    "Cicada_Swarm",
    "Granular_Dust_Texture",
    "Corroded_Metal_Texture",
]

# ---------------------------------------------------------------------------
# Direct parameter renames (no value transformation)
# ---------------------------------------------------------------------------
DIRECT_RENAMES = {
    "osc_a_wave": "poss_oscAWaveform",
    "osc_a_shape": "poss_oscAShape",
    "osc_a_drift": "poss_oscADrift",
    "osc_b_wave": "poss_oscBWaveform",
    "osc_b_shape": "poss_oscBShape",
    "osc_b_instability": "poss_oscBInstability",
    "osc_interaction_mode": "poss_oscInteractMode",
    "osc_interaction_amt": "poss_oscInteractAmount",
    "osc_blend": "poss_oscMix",
    "sub_level": "poss_subLevel",
    "sub_shape": "poss_weightShape",
    "weight_amt": "poss_weightLevel",
    "noise_type": "poss_noiseType",
    "noise_routing": "poss_noiseRouting",
    "noise_level": "poss_noiseLevel",
    "noise_transient": "poss_noiseDecay",
    "filter_mode": "poss_filterMode",
    "filter_cutoff": "poss_filterCutoff",
    "filter_res": "poss_filterReso",
    "filter_keytrack": "poss_filterKeyTrack",
    "filter_drive": "poss_filterDrive",
    "filter_chew": "poss_chewAmount",
    "fur_amt": "poss_furAmount",
    "gnash_amt": "poss_gnashAmount",
    "trash_mode": "poss_trashMode",
    "trash_amt": "poss_trashAmount",
    "amp_attack": "poss_ampAttack",
    "amp_decay": "poss_ampDecay",
    "amp_sustain": "poss_ampSustain",
    "amp_release": "poss_ampRelease",
    "filt_env_vel_scale": "poss_ampVelSens",
    "filt_env_amt": "poss_filterEnvAmount",
    "filt_attack": "poss_filterAttack",
    "filt_decay": "poss_filterDecay",
    "filt_sustain": "poss_filterSustain",
    "filt_release": "poss_filterRelease",
    "mod_attack": "poss_modAttack",
    "mod_decay": "poss_modDecay",
    "mod_sustain": "poss_modSustain",
    "mod_release": "poss_modRelease",
    "lfo1_rate": "poss_lfo1Rate",
    "lfo1_depth": "poss_lfo1Depth",
    "lfo1_shape": "poss_lfo1Shape",
    "lfo1_sync": "poss_lfo1Sync",
    "lfo1_retrigger": "poss_lfo1Retrigger",
    "lfo2_rate": "poss_lfo2Rate",
    "lfo2_depth": "poss_lfo2Depth",
    "lfo2_shape": "poss_lfo2Shape",
    "lfo2_sync": "poss_lfo2Sync",
    "lfo2_retrigger": "poss_lfo2Retrigger",
    "lfo3_rate": "poss_lfo3Rate",
    "lfo3_depth": "poss_lfo3Depth",
    "lfo3_shape": "poss_lfo3Shape",
    "lfo3_sync": "poss_lfo3Sync",
    "lfo3_retrigger": "poss_lfo3Retrigger",
    "macro_belly": "poss_macroBelly",
    "macro_bite": "poss_macroBite",
    "macro_scurry": "poss_macroScurry",
    "macro_trash": "poss_macroTrash",
    "macro_play_dead": "poss_macroPlayDead",
    "fx_motion_mode": "poss_fxMotionType",
    "fx_motion_rate": "poss_fxMotionRate",
    "fx_motion_depth": "poss_fxMotionDepth",
    "fx_motion_mix": "poss_fxMotionMix",
    "fx_echo_mode": "poss_fxEchoType",
    "fx_echo_time": "poss_fxEchoTime",
    "fx_echo_feedback": "poss_fxEchoFeedback",
    "fx_echo_mix": "poss_fxEchoMix",
    "fx_space_mode": "poss_fxSpaceType",
    "fx_space_size": "poss_fxSpaceSize",
    "fx_space_decay": "poss_fxSpaceDecay",
    "fx_space_damp": "poss_fxSpaceDamping",
    "fx_space_mix": "poss_fxSpaceMix",
    "out_glue": "poss_fxFinishGlue",
    "out_clip": "poss_fxFinishClip",
    "out_width": "poss_fxFinishWidth",
    "voice_mode": "poss_polyphony",
    "glide_mode": "poss_glideMode",
    "unison_voices": "poss_unisonVoices",
    "unison_spread": "poss_unisonSpread",
}

# Parameters to skip entirely
SKIP_PARAMS = {
    "fx_echo_tone",
    "osc_a_oct",
    "osc_b_oct",
    "osc_b_tune",
    "osc_b_asymmetry",
    "osc_b_reset",
    "osc_a_reset",
    "pitch_bend_range",
    "quality_mode",
    "velocity_curve",
    "master_tune",
    "osc_a_level",
    "osc_a_tune",
    "osc_b_level",
    "apvts_xml",
}

# Default params added to every converted preset
DEFAULTS = {
    "poss_chewFreq": 1000.0,
    "poss_chewMix": 0.5,
    "poss_driveAmount": 0.0,
    "poss_driveType": 0,
    "poss_weightOctave": 0,
    "poss_weightTune": 0.0,
    "poss_modEnvAmount": 0.0,
    "poss_modEnvDest": 0,
    "poss_lfo1Phase": 0.0,
    "poss_lfo2Phase": 0.0,
    "poss_lfo3Phase": 0.0,
    "poss_fxEchoSync": 0,
    "poss_fxFinishLowMono": 0.0,
    "poss_pan": 0.5,
}

# ---------------------------------------------------------------------------
# Value transformations
# ---------------------------------------------------------------------------

def db_to_linear(db: float) -> float:
    """Convert dB to linear approximation; 0 dB -> 0.7, clamp 0-1."""
    # 0 dB -> 0.7 per spec.  Use 0.7 * 10^(dB/20) then clamp.
    linear = 0.7 * (10.0 ** (db / 20.0))
    return max(0.0, min(1.0, linear))


def transform_value(src_key: str, value) -> tuple:
    """
    Return (dst_key, transformed_value) for special-cased parameters.
    Returns None if the parameter should be skipped.
    Returns (None, None) if handled entirely inside the function.
    """
    if src_key == "glide_time":
        return ("poss_glideTime", round(value / 1000.0, 6))
    if src_key == "unison_detune":
        return ("poss_unisonDetune", value / 50.0)
    if src_key == "sub_oct":
        return ("poss_subOctave", int(value) + 2)
    if src_key == "out_gain":
        return ("poss_level", round(db_to_linear(value), 4))
    # Mod slots
    m = re.match(r"mod_slot(\d+)_(src|amt|dst)$", src_key)
    if m:
        n = m.group(1)
        field = m.group(2)
        field_map = {"src": "Src", "amt": "Amt", "dst": "Dst"}
        return (f"poss_modSlot{n}{field_map[field]}", value)
    return None  # signal: not a special-case


# ---------------------------------------------------------------------------
# Preset name formatting
# ---------------------------------------------------------------------------

CATEGORY_SUFFIXES = re.compile(
    r"[_\s]+(Bass|Lead|Pad|Keys|Str|Drone|80s|Texture|Solo|Pluck|Ambient)$",
    re.IGNORECASE,
)


def format_preset_name(raw_name: str) -> str:
    """Clean and format preset name to XOceanus conventions."""
    name = raw_name.replace("_", " ").strip()
    name = CATEGORY_SUFFIXES.sub("", name).strip()
    # Truncate to 30 chars preserving whole words where possible
    if len(name) > 30:
        name = name[:30].rsplit(" ", 1)[0] if " " in name[:30] else name[:30]
    return name


# ---------------------------------------------------------------------------
# Mood assignment
# ---------------------------------------------------------------------------

def assign_mood(category: str, tags: list, params: dict) -> str:
    """Determine XOceanus mood from category, tags, and parameter values."""
    cat_lower = (category or "").lower()
    tag_lower = " ".join(tags).lower()
    combined = (cat_lower + " " + tag_lower).lower()

    # Aggression override: heavy gnash/trash -> Kinetic
    gnash = params.get("gnash_amt", 0.0)
    trash = params.get("trash_amt", 0.0)
    if gnash > 0.5 or trash > 0.5:
        return "Kinetic"

    mapping = [
        (["bass", "sub", "808"],                "Foundation"),
        (["lead", "solo", "mono"],               "Kinetic"),
        (["pad", "ambient"],                     "Atmosphere"),
        (["drone"],                              "Deep"),
        (["keys", "pluck"],                      "Luminous"),
        (["str", "string", "violin", "cello",
          "viola", "marimba", "harp", "kalimba",
          "dulcimer", "erhu"],                   "Organic"),
        (["80s", "retro", "fairlight"],          "Flux"),
        (["texture", "fx", "experiment",
          "granular", "noise", "corroded",
          "swarm", "cicada", "chaos"],           "Prism"),
    ]
    for keywords, mood in mapping:
        if any(kw in combined for kw in keywords):
            return mood

    return "Foundation"  # fallback


# ---------------------------------------------------------------------------
# 6D Sonic DNA calculation
# ---------------------------------------------------------------------------

def calc_dna(params: dict) -> dict:
    """Derive 6D Sonic DNA from source XOppossum parameter values."""

    def clamp(v: float) -> float:
        return max(0.0, min(1.0, v))

    # brightness: log-normalise filter_cutoff (20–20000 Hz) + osc interaction
    cutoff = params.get("filter_cutoff", 1000.0)
    cutoff_norm = math.log(max(cutoff, 20.0) / 20.0) / math.log(20000.0 / 20.0)
    interact = params.get("osc_interaction_amt", 0.0)
    brightness = clamp(cutoff_norm * 0.85 + interact * 0.15)

    # warmth: fur + (1 - resonance)
    fur = params.get("fur_amt", 0.0)
    reso = params.get("filter_res", 0.0)
    warmth = clamp(fur * 0.7 + (1.0 - reso) * 0.3)

    # movement: max LFO depth + motion depth
    lfo_max = max(
        params.get("lfo1_depth", 0.0),
        params.get("lfo2_depth", 0.0),
        params.get("lfo3_depth", 0.0),
    )
    motion = params.get("fx_motion_depth", 0.0)
    movement = clamp(lfo_max * 0.6 + motion * 0.4)

    # density: unison + noise + sub
    unison = params.get("unison_voices", 1)
    unison_norm = min(float(unison), 8.0) / 8.0
    noise = params.get("noise_level", 0.0)
    sub = params.get("sub_level", 0.0)
    density = clamp(unison_norm * 0.4 + noise * 0.3 + sub * 0.3)

    # space: reverb mix + echo mix + reverb size
    space_mix = params.get("fx_space_mix", 0.0)
    echo_mix = params.get("fx_echo_mix", 0.0)
    space_size = params.get("fx_space_size", 0.0)
    space = clamp(space_mix * 0.5 + echo_mix * 0.3 + space_size * 0.2)

    # aggression: gnash + trash + drive
    gnash = params.get("gnash_amt", 0.0)
    trash = params.get("trash_amt", 0.0)
    drive = params.get("filter_drive", 0.0)
    aggression = clamp(gnash * 0.4 + trash * 0.3 + drive * 0.3)

    return {
        "brightness": round(brightness, 3),
        "warmth": round(warmth, 3),
        "movement": round(movement, 3),
        "density": round(density, 3),
        "space": round(space, 3),
        "aggression": round(aggression, 3),
    }


# ---------------------------------------------------------------------------
# Tag generation
# ---------------------------------------------------------------------------

def build_tags(category: str, src_tags: list, mood: str) -> list:
    """Build a short curated tag list for the .xometa file."""
    tags = set()
    tags.add(mood.lower())
    if category:
        tags.add(category.lower())
    for t in src_tags:
        cleaned = t.lower().replace(" ", "-")
        if len(cleaned) <= 20:
            tags.add(cleaned)
    return sorted(list(tags))[:8]  # cap at 8 tags


# ---------------------------------------------------------------------------
# Core conversion
# ---------------------------------------------------------------------------

def convert_preset(src_path: Path) -> dict:
    """Load a single XOppossum JSON and return an XOceanus .xometa dict."""
    with open(src_path, "r", encoding="utf-8") as f:
        raw = json.load(f)

    state = raw.get("state", raw)  # handle both wrapped and flat
    metadata = state.get("metadata", {})
    src_params = state.get("parameters", {})

    raw_name = metadata.get("preset_name", src_path.stem.replace("_", " "))
    category = metadata.get("category", "")
    src_tags = metadata.get("tags", [])
    description = metadata.get("description", "")
    author = metadata.get("author", "XO_OX Designs")

    preset_name = format_preset_name(raw_name)
    mood = assign_mood(category, src_tags, src_params)
    dna = calc_dna(src_params)
    tags = build_tags(category, src_tags, mood)

    # --- Build parameter block ---
    dst_params = {}

    for src_key, value in src_params.items():
        if src_key in SKIP_PARAMS:
            continue

        # Try special-case transforms first
        result = transform_value(src_key, value)
        if result is not None:
            dst_key, dst_val = result
            dst_params[dst_key] = dst_val
            continue

        # Direct rename
        if src_key in DIRECT_RENAMES:
            dst_params[DIRECT_RENAMES[src_key]] = value
            continue

        # Unknown params: skip silently (don't pollute the output)

    # Inject defaults (only if not already set by source data)
    for def_key, def_val in DEFAULTS.items():
        if def_key not in dst_params:
            dst_params[def_key] = def_val

    # Ensure the Overbite macro labels are always present
    xometa = {
        "schema_version": 1,
        "name": preset_name,
        "mood": mood,
        "engines": ["Overbite"],
        "author": "XO_OX Designs",
        "version": "1.0.0",
        "description": description,
        "tags": tags,
        "macroLabels": ["BELLY", "BITE", "SCURRY", "TRASH"],
        "couplingIntensity": "None",
        "tempo": None,
        "dna": dna,
        "parameters": {
            "Overbite": dst_params
        },
        "coupling": {
            "pairs": []
        },
        "sequencer": None,
    }

    return xometa, mood


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main():
    converted = []
    missing = []
    failed = []
    mood_counts: dict = {}

    for stem in CURATED:
        src_path = SOURCE_DIR / f"{stem}.json"
        if not src_path.exists():
            missing.append(stem)
            continue

        try:
            xometa, mood = convert_preset(src_path)
        except Exception as exc:
            failed.append((stem, str(exc)))
            continue

        # Determine output path
        out_dir = OUTPUT_BASE / mood / "Overbite"
        out_dir.mkdir(parents=True, exist_ok=True)
        preset_name = xometa["name"]
        # Sanitize filename: replace spaces with underscores, strip problematic chars
        safe_filename = re.sub(r"[^\w\s\-]", "", preset_name)
        safe_filename = re.sub(r"\s+", "_", safe_filename.strip())
        out_path = out_dir / f"{safe_filename}.xometa"

        with open(out_path, "w", encoding="utf-8") as f:
            json.dump(xometa, f, indent=2, ensure_ascii=False)
            f.write("\n")

        mood_counts[mood] = mood_counts.get(mood, 0) + 1
        converted.append((stem, mood, str(out_path)))

    # --- Report ---
    print(f"\n{'='*60}")
    print(f"XOppossum → XOceanus Conversion Report")
    print(f"{'='*60}")
    print(f"  Converted:  {len(converted)}/{len(CURATED)}")
    print(f"  Missing:    {len(missing)}")
    print(f"  Failed:     {len(failed)}")
    print()

    print("Mood distribution:")
    for mood, count in sorted(mood_counts.items()):
        print(f"  {mood:<15} {count:>3}")

    if missing:
        print(f"\nMissing source files ({len(missing)}):")
        for m in missing:
            print(f"  - {m}.json")

    if failed:
        print(f"\nConversion errors ({len(failed)}):")
        for stem, err in failed:
            print(f"  - {stem}: {err}")

    print(f"\nConverted presets:")
    for stem, mood, path in converted:
        print(f"  [{mood:<12}] {stem} → {Path(path).name}")

    print(f"\n{'='*60}\n")


if __name__ == "__main__":
    main()
