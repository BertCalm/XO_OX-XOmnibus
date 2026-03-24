#!/usr/bin/env python3
"""
fix_drift_presets.py — Migrate XOdyssey (Odyssey/Drift) preset parameters
to the canonical DriftEngine schema.

Problem: 202 presets were authored against the old XOdyssey standalone parameter
schema (unprefixed, e.g. `osc_a_mode`, `filt_a_cutoff`, `env_amp_attack`) before
the `drift_` prefix convention was enforced in XOlokun. Three presets also contain
`drift_osc_a_mode`-style double-prefixed params from a partial prior migration run.
One preset uses `odyssey_detune` (wrong prefix).

This script:
1. Finds all .xometa files with Odyssey or Drift engine blocks
2. For each engine block, replaces ghost keys with canonical `drift_` equivalents
3. Drops ghost keys that have no canonical mapping in the current DriftEngine
4. Writes back only files that were actually changed

Canonical DriftEngine params (38 total):
  drift_oscA_mode, drift_oscA_shape, drift_oscA_tune, drift_oscA_level,
  drift_oscA_detune, drift_oscA_pw, drift_oscA_fmDepth,
  drift_oscB_mode, drift_oscB_shape, drift_oscB_tune, drift_oscB_level,
  drift_oscB_detune, drift_oscB_pw, drift_oscB_fmDepth,
  drift_subLevel, drift_noiseLevel, drift_hazeAmount,
  drift_filterCutoff, drift_filterReso, drift_filterSlope, drift_filterEnvAmt,
  drift_formantMorph, drift_formantMix,
  drift_shimmerAmount, drift_shimmerTone,
  drift_attack, drift_decay, drift_sustain, drift_release,
  drift_lfoRate, drift_lfoDepth, drift_lfoDest,
  drift_driftDepth, drift_driftRate,
  drift_level, drift_voiceMode, drift_glide, drift_polyphony

Usage:
  python3 Tools/fix_drift_presets.py [--dry-run]
"""

import json
import os
import glob
import sys
import argparse

# ---------------------------------------------------------------------------
# Mapping: old ghost param key → canonical drift_ param key
# Keys mapped to None are dropped (no canonical equivalent).
# ---------------------------------------------------------------------------
GHOST_TO_CANONICAL = {
    # Osc A
    "osc_a_mode":           "drift_oscA_mode",
    "osc_a_shape":          "drift_oscA_shape",
    "osc_a_tune":           "drift_oscA_tune",
    "osc_a_level":          "drift_oscA_level",
    "osc_a_detune":         "drift_oscA_detune",
    "osc_a_pw":             "drift_oscA_pw",
    "osc_a_fm_depth":       "drift_oscA_fmDepth",
    "osc_a_fm_ratio":       None,   # FM ratio absorbed into fmDepth; no direct mapping
    "osc_a_fine":           None,   # fine tune removed in canonical schema

    # Osc B
    "osc_b_mode":           "drift_oscB_mode",
    "osc_b_shape":          "drift_oscB_shape",
    "osc_b_tune":           "drift_oscB_tune",
    "osc_b_level":          "drift_oscB_level",
    "osc_b_detune":         "drift_oscB_detune",
    "osc_b_pw":             "drift_oscB_pw",
    "osc_b_fm_depth":       "drift_oscB_fmDepth",
    "osc_b_fm_ratio":       None,   # no direct mapping
    "osc_b_fine":           None,   # fine tune removed

    # Sub / noise
    "sub_level":            "drift_subLevel",
    "sub_octave":           None,   # not in canonical schema
    "sub_shape":            None,   # not in canonical schema
    "noise_level":          "drift_noiseLevel",
    "noise_type":           None,   # not in canonical schema

    # Haze / character
    "haze_amount":          "drift_hazeAmount",

    # Filter
    "filt_a_cutoff":        "drift_filterCutoff",
    "filt_a_reso":          "drift_filterReso",
    "filt_a_slope":         "drift_filterSlope",
    "filt_a_env_amt":       "drift_filterEnvAmt",
    "filt_a_drive":         None,   # not in canonical schema
    "filt_b_reso":          None,   # filter B removed; no mapping
    "filt_b_mix":           None,   # no mapping
    "filt_b_morph":         "drift_formantMorph",  # formant morph is the canonical equivalent

    # Shimmer
    "shimmer_amount":       "drift_shimmerAmount",
    "shimmer_tone":         "drift_shimmerTone",

    # Amp envelope (old standalone naming)
    "env_amp_attack":       "drift_attack",
    "env_amp_decay":        "drift_decay",
    "env_amp_sustain":      "drift_sustain",
    "env_amp_release":      "drift_release",

    # Filter envelope (old standalone — no separate filter env in canonical schema)
    "env_filter_attack":    None,
    "env_filter_decay":     None,
    "env_filter_sustain":   None,
    "env_filter_release":   None,

    # LFO
    "lfo_1_rate":           "drift_lfoRate",
    "lfo_1_depth":          "drift_lfoDepth",
    "lfo_1_shape":          None,   # shape not exposed in canonical schema

    # Drift signature
    "drift_depth":          "drift_driftDepth",
    "drift_rate":           "drift_driftRate",

    # Voice / global
    "voice_mode":           "drift_voiceMode",
    "voice_glide_time":     "drift_glide",
    "global_spread":        None,   # not in canonical schema
    "master_gain":          "drift_level",  # best approximation

    # Mod matrix (not in canonical schema)
    "mod_1_source":         None,
    "mod_1_dest":           None,
    "mod_1_amount":         None,
    "mod_2_source":         None,
    "mod_2_dest":           None,
    "mod_2_amount":         None,

    # Macro (not in canonical schema)
    "macro_journey":        None,

    # Tidal (not in canonical schema)
    "tidal_depth":          None,
    "tidal_rate":           None,

    # Fracture (not in canonical schema)
    "fracture_enable":      None,
    "fracture_intensity":   None,
    "fracture_rate":        None,

    # FX — old standalone had embedded fx; canonical schema does not
    "reverb_enable":        None,
    "reverb_size":          None,
    "reverb_mix":           None,
    "reverb_damping":       None,
    "delay_enable":         None,
    "delay_time":           None,
    "delay_feedback":       None,
    "delay_mix":            None,
    "delay_mode":           None,
    "chorus_enable":        None,
    "chorus_rate":          None,
    "chorus_depth":         None,
    "chorus_mix":           None,
    "phaser_enable":        None,
    "phaser_rate":          None,
    "phaser_depth":         None,
    "phaser_mix":           None,
    "phaser_feedback":      None,

    # Wrong-prefix variants
    "odyssey_detune":       "drift_oscA_detune",  # closest canonical; detune is per-osc in new schema
    "drift_osc_a_mode":     "drift_oscA_mode",
    "drift_haze_amount":    "drift_hazeAmount",
    "drift_reverb_size":    None,   # reverb not in canonical schema
    "drift_master_gain":    "drift_level",
}

# Canonical param IDs for validation
CANONICAL_PARAMS = {
    "drift_oscA_mode", "drift_oscA_shape", "drift_oscA_tune", "drift_oscA_level",
    "drift_oscA_detune", "drift_oscA_pw", "drift_oscA_fmDepth",
    "drift_oscB_mode", "drift_oscB_shape", "drift_oscB_tune", "drift_oscB_level",
    "drift_oscB_detune", "drift_oscB_pw", "drift_oscB_fmDepth",
    "drift_subLevel", "drift_noiseLevel", "drift_hazeAmount",
    "drift_filterCutoff", "drift_filterReso", "drift_filterSlope", "drift_filterEnvAmt",
    "drift_formantMorph", "drift_formantMix",
    "drift_shimmerAmount", "drift_shimmerTone",
    "drift_attack", "drift_decay", "drift_sustain", "drift_release",
    "drift_lfoRate", "drift_lfoDepth", "drift_lfoDest",
    "drift_driftDepth", "drift_driftRate",
    "drift_level", "drift_voiceMode", "drift_glide", "drift_polyphony",
}


def migrate_engine_block(params: dict) -> tuple[dict, int, int]:
    """
    Migrate a single engine parameter block.

    Returns (new_params, num_renamed, num_dropped).
    """
    new_params = {}
    num_renamed = 0
    num_dropped = 0

    for key, value in params.items():
        if key in CANONICAL_PARAMS:
            # Already canonical — keep as-is
            new_params[key] = value
        elif key in GHOST_TO_CANONICAL:
            canonical = GHOST_TO_CANONICAL[key]
            if canonical is None:
                num_dropped += 1  # drop: no canonical mapping
            else:
                if canonical not in new_params:
                    new_params[canonical] = value
                    num_renamed += 1
                # If canonical key already exists (e.g. from a prior partial migration),
                # skip the duplicate to avoid overwriting the already-correct value.
        else:
            # Unknown key — keep (may be a newer param we don't know about)
            new_params[key] = value

    return new_params, num_renamed, num_dropped


def process_preset(path: str, dry_run: bool) -> tuple[bool, int, int]:
    """
    Process a single preset file.

    Returns (was_changed, total_renamed, total_dropped).
    """
    with open(path, encoding="utf-8") as f:
        data = json.load(f)

    parameters = data.get("parameters", {})
    total_renamed = 0
    total_dropped = 0
    changed = False

    for engine_key in list(parameters.keys()):
        if engine_key not in ("Odyssey", "Drift"):
            continue
        old_block = parameters[engine_key]
        new_block, renamed, dropped = migrate_engine_block(old_block)
        if renamed or dropped:
            parameters[engine_key] = new_block
            total_renamed += renamed
            total_dropped += dropped
            changed = True

    if changed and not dry_run:
        with open(path, "w", encoding="utf-8") as f:
            json.dump(data, f, indent=2, ensure_ascii=False)
            f.write("\n")

    return changed, total_renamed, total_dropped


def main():
    parser = argparse.ArgumentParser(description="Migrate Drift/Odyssey preset schemas")
    parser.add_argument("--dry-run", action="store_true",
                        help="Report changes without writing files")
    args = parser.parse_args()

    script_dir = os.path.dirname(os.path.abspath(__file__))
    repo_root = os.path.dirname(script_dir)
    preset_dir = os.path.join(repo_root, "Presets")

    all_files = glob.glob(os.path.join(preset_dir, "**/*.xometa"), recursive=True)
    all_files.sort()

    files_changed = 0
    total_renamed = 0
    total_dropped = 0
    files_with_engine = 0

    for fpath in all_files:
        try:
            with open(fpath, encoding="utf-8") as f:
                data = json.load(f)
        except Exception as e:
            print(f"SKIP (parse error): {fpath}: {e}")
            continue

        params = data.get("parameters", {})
        has_engine = any(k in params for k in ("Odyssey", "Drift"))
        if not has_engine:
            continue
        files_with_engine += 1

        changed, renamed, dropped = process_preset(fpath, args.dry_run)
        if changed:
            files_changed += 1
            total_renamed += renamed
            total_dropped += dropped
            rel = os.path.relpath(fpath, repo_root)
            mode = "[DRY RUN]" if args.dry_run else "[FIXED]"
            print(f"{mode} {rel}  (renamed={renamed}, dropped={dropped})")

    print()
    print(f"Presets scanned with Odyssey/Drift engine: {files_with_engine}")
    print(f"Presets changed: {files_changed}")
    print(f"Total params renamed: {total_renamed}")
    print(f"Total params dropped (no canonical mapping): {total_dropped}")
    if args.dry_run:
        print("(dry-run mode — no files were written)")


if __name__ == "__main__":
    main()
