#!/usr/bin/env python3
"""
fix_dub_presets.py — Migrate XOverdub (Overdub/Dub) preset parameters
to the canonical DubEngine schema.

Problem: 41 presets were authored against the old XOverdub standalone parameter
schema (unprefixed, e.g. `osc_wave`, `filter_cutoff`, `env_attack`, `send_level`)
before the `dub_` prefix convention was enforced in XOlokun.

This script:
1. Finds all .xometa files with Overdub or Dub engine blocks
2. For each engine block, replaces ghost keys with canonical `dub_` equivalents
3. Drops ghost keys that have no canonical mapping in the current DubEngine
4. Writes back only files that were actually changed

Canonical DubEngine params (37 total):
  dub_oscWave, dub_oscOctave, dub_oscTune, dub_oscPwm,
  dub_subLevel, dub_noiseLevel, dub_drift, dub_level,
  dub_filterMode, dub_filterCutoff, dub_filterReso, dub_filterEnvAmt,
  dub_attack, dub_decay, dub_sustain, dub_release,
  dub_pitchEnvDepth, dub_pitchEnvDecay,
  dub_lfoRate, dub_lfoDepth, dub_lfoDest,
  dub_sendLevel, dub_returnLevel, dub_dryLevel,
  dub_driveAmount,
  dub_delayTime, dub_delayFeedback, dub_delayWear, dub_delayWow, dub_delayMix,
  dub_reverbSize, dub_reverbDamp, dub_reverbMix,
  dub_voiceMode, dub_glide, dub_polyphony

Usage:
  python3 Tools/fix_dub_presets.py [--dry-run]
"""

import json
import os
import glob
import argparse

# ---------------------------------------------------------------------------
# Mapping: old ghost param key → canonical dub_ param key
# Keys mapped to None are dropped (no canonical equivalent in current DubEngine).
# ---------------------------------------------------------------------------
GHOST_TO_CANONICAL = {
    # Osc
    "osc_wave":         "dub_oscWave",
    "osc_octave":       "dub_oscOctave",
    "osc_pwm":          "dub_oscPwm",
    "osc_drift":        "dub_drift",
    "osc_level":        "dub_level",        # osc level → engine level (best mapping)
    "osc_sub_level":    "dub_subLevel",
    "osc_noise_level":  "dub_noiseLevel",

    # Filter
    "filter_cutoff":    "dub_filterCutoff",
    "filter_resonance": "dub_filterReso",
    "filter_env_amt":   "dub_filterEnvAmt",

    # Amp envelope
    "env_attack":       "dub_attack",
    "env_decay":        "dub_decay",
    "env_sustain":      "dub_sustain",
    "env_release":      "dub_release",

    # Pitch envelope
    "pitch_env_depth":  "dub_pitchEnvDepth",
    "pitch_env_decay":  "dub_pitchEnvDecay",

    # LFO
    "lfo_rate":         "dub_lfoRate",
    "lfo_depth":        "dub_lfoDepth",
    "lfo_dest":         "dub_lfoDest",

    # Send/return
    "send_level":       "dub_sendLevel",
    "return_level":     "dub_returnLevel",

    # Drive
    "drive_amount":     "dub_driveAmount",

    # Delay
    "delay_time":       "dub_delayTime",
    "delay_feedback":   "dub_delayFeedback",
    "delay_wear":       "dub_delayWear",
    "delay_wow":        "dub_delayWow",
    "delay_mix":        "dub_delayMix",
    "delay_sync":       None,   # delay sync not in canonical schema

    # Reverb
    "reverb_size":      "dub_reverbSize",
    "reverb_damp":      "dub_reverbDamp",
    "reverb_mix":       "dub_reverbMix",

    # Voice
    "voice_mode":       "dub_voiceMode",
    "voice_glide":      "dub_glide",

    # Wrong-prefix `dub_` variants (partially migrated by an old tool)
    "dub_oscMode":      None,   # no canonical `dub_oscMode`; wave is `dub_oscWave`
    "dub_oscShape":     None,   # no canonical `dub_oscShape`
    "dub_sendAmount":   "dub_sendLevel",  # sendAmount → sendLevel
}

# Canonical param IDs for validation
CANONICAL_PARAMS = {
    "dub_oscWave", "dub_oscOctave", "dub_oscTune", "dub_oscPwm",
    "dub_subLevel", "dub_noiseLevel", "dub_drift", "dub_level",
    "dub_filterMode", "dub_filterCutoff", "dub_filterReso", "dub_filterEnvAmt",
    "dub_attack", "dub_decay", "dub_sustain", "dub_release",
    "dub_pitchEnvDepth", "dub_pitchEnvDecay",
    "dub_lfoRate", "dub_lfoDepth", "dub_lfoDest",
    "dub_sendLevel", "dub_returnLevel", "dub_dryLevel",
    "dub_driveAmount",
    "dub_delayTime", "dub_delayFeedback", "dub_delayWear", "dub_delayWow", "dub_delayMix",
    "dub_reverbSize", "dub_reverbDamp", "dub_reverbMix",
    "dub_voiceMode", "dub_glide", "dub_polyphony",
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
                # Skip duplicates (canonical key already present)
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
        if engine_key not in ("Overdub", "Dub"):
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
    parser = argparse.ArgumentParser(description="Migrate Dub/Overdub preset schemas")
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
        has_engine = any(k in params for k in ("Overdub", "Dub"))
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
    print(f"Presets scanned with Overdub/Dub engine: {files_with_engine}")
    print(f"Presets changed: {files_changed}")
    print(f"Total params renamed: {total_renamed}")
    print(f"Total params dropped (no canonical mapping): {total_dropped}")
    if args.dry_run:
        print("(dry-run mode — no files were written)")


if __name__ == "__main__":
    main()
