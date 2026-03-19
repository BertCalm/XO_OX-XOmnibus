#!/usr/bin/env python3
"""
fix_bob_presets.py — Migrate XOblong (Oblong/Bob) preset parameters
to the canonical BobEngine schema.

Problem: 167 presets were authored against the old XOblong/BobEngine standalone
parameter schema (unprefixed, e.g. `oscA_wave`, `flt_cutoff`, `env_attack`) before
the `bob_` prefix convention was enforced in XOmnibus.

This script:
1. Finds all .xometa files with Oblong or Bob engine blocks
2. For each engine block, replaces ghost keys with canonical `bob_` equivalents
3. Drops ghost keys that have no canonical mapping in the current BobEngine
4. Writes back only files that were actually changed

Canonical BobEngine params (41 total):
  bob_oscA_wave, bob_oscA_shape, bob_oscA_tune, bob_oscA_drift,
  bob_oscB_wave, bob_oscB_detune, bob_oscB_blend, bob_oscB_sync, bob_oscB_fm,
  bob_texMode, bob_texLevel, bob_texTone, bob_texWidth,
  bob_fltMode, bob_fltCutoff, bob_fltReso, bob_fltChar, bob_fltDrive, bob_fltEnvAmt,
  bob_ampAttack, bob_ampDecay, bob_ampSustain, bob_ampRelease,
  bob_motAttack, bob_motDecay, bob_motSustain, bob_motRelease, bob_motDepth,
  bob_lfo1Rate, bob_lfo1Depth, bob_lfo1Shape, bob_lfo1Target,
  bob_curMode, bob_curAmount,
  bob_bobMode,
  bob_dustAmount, bob_dustTone,
  bob_level, bob_voiceMode, bob_glide, bob_polyphony

Usage:
  python3 Tools/fix_bob_presets.py [--dry-run]
"""

import json
import os
import glob
import argparse

# ---------------------------------------------------------------------------
# Mapping: old ghost param key → canonical bob_ param key
# Keys mapped to None are dropped (no canonical equivalent in current BobEngine).
# ---------------------------------------------------------------------------
GHOST_TO_CANONICAL = {
    # Osc A
    "oscA_wave":        "bob_oscA_wave",
    "oscA_shape":       "bob_oscA_shape",
    "oscA_tune":        "bob_oscA_tune",
    "oscA_drift":       "bob_oscA_drift",

    # Osc B
    "oscB_wave":        "bob_oscB_wave",
    "oscB_detune":      "bob_oscB_detune",
    "oscB_blend":       "bob_oscB_blend",
    "oscB_sync":        "bob_oscB_sync",
    "oscB_fm":          "bob_oscB_fm",

    # Filter (old short names)
    "flt_mode":         "bob_fltMode",
    "flt_cutoff":       "bob_fltCutoff",
    "flt_resonance":    "bob_fltReso",
    "flt_character":    "bob_fltChar",
    "flt_drive":        "bob_fltDrive",
    "flt_envAmt":       "bob_fltEnvAmt",

    # Amp envelope (old generic names)
    "env_attack":       "bob_ampAttack",
    "env_decay":        "bob_ampDecay",
    "env_sustain":      "bob_ampSustain",
    "env_release":      "bob_ampRelease",

    # Motion envelope
    "motEnv_attack":    "bob_motAttack",
    "motEnv_decay":     "bob_motDecay",
    "motEnv_sustain":   "bob_motSustain",
    "motEnv_release":   "bob_motRelease",
    "motEnv_depth":     "bob_motDepth",

    # LFO1
    "lfo1_rate":        "bob_lfo1Rate",
    "lfo1_depth":       "bob_lfo1Depth",
    "lfo1_shape":       "bob_lfo1Shape",
    "lfo1_target":      "bob_lfo1Target",

    # LFO2 — not in canonical schema (BobEngine has only LFO1)
    "lfo2_rate":        None,
    "lfo2_depth":       None,

    # Curiosity
    "cur_mode":         "bob_curMode",
    "cur_amount":       "bob_curAmount",

    # Bob Mode — note: old `bob_mode` was a float 0-1 character knob;
    # canonical `bob_bobMode` is the same concept, renamed.
    "bob_mode":         "bob_bobMode",

    # Dust
    "dust_amount":      "bob_dustAmount",
    "dust_tone":        "bob_dustTone",
    "dust_age":         None,   # not in canonical schema

    # Smear — not in canonical schema (smear was removed from BobEngine)
    "smear_mix":        None,
    "smear_depth":      None,
    "smear_width":      None,
    "smear_rate":       None,

    # Space/reverb — BobEngine exposes texture (tex_*) not a reverb tail;
    # `space_*` params are from old XOblong standalone and have no canonical mapping.
    "space_mix":        None,
    "space_size":       None,
    "space_decay":      None,
    "space_air":        None,

    # Texture — old `tex_*` prefix matches canonical `bob_tex*`
    "tex_mode":         "bob_texMode",
    "tex_level":        "bob_texLevel",
    "tex_tone":         "bob_texTone",
    "tex_width":        "bob_texWidth",

    # Voice
    "glide":            "bob_glide",
    "voice_mode":       "bob_voiceMode",

    # Stutter — not in canonical schema
    "stutter_rate":     None,
    "stutter_depth":    None,

    # Velocity filter — not in canonical schema
    "vel_filter":       None,
}

# Canonical param IDs for validation
CANONICAL_PARAMS = {
    "bob_oscA_wave", "bob_oscA_shape", "bob_oscA_tune", "bob_oscA_drift",
    "bob_oscB_wave", "bob_oscB_detune", "bob_oscB_blend", "bob_oscB_sync", "bob_oscB_fm",
    "bob_texMode", "bob_texLevel", "bob_texTone", "bob_texWidth",
    "bob_fltMode", "bob_fltCutoff", "bob_fltReso", "bob_fltChar", "bob_fltDrive", "bob_fltEnvAmt",
    "bob_ampAttack", "bob_ampDecay", "bob_ampSustain", "bob_ampRelease",
    "bob_motAttack", "bob_motDecay", "bob_motSustain", "bob_motRelease", "bob_motDepth",
    "bob_lfo1Rate", "bob_lfo1Depth", "bob_lfo1Shape", "bob_lfo1Target",
    "bob_curMode", "bob_curAmount",
    "bob_bobMode",
    "bob_dustAmount", "bob_dustTone",
    "bob_level", "bob_voiceMode", "bob_glide", "bob_polyphony",
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
                # Skip duplicates (canonical key already present from a prior migration)
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
        if engine_key not in ("Oblong", "Bob"):
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
    parser = argparse.ArgumentParser(description="Migrate Bob/Oblong preset schemas")
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
        has_engine = any(k in params for k in ("Oblong", "Bob"))
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
    print(f"Presets scanned with Oblong/Bob engine: {files_with_engine}")
    print(f"Presets changed: {files_changed}")
    print(f"Total params renamed: {total_renamed}")
    print(f"Total params dropped (no canonical mapping): {total_dropped}")
    if args.dry_run:
        print("(dry-run mode — no files were written)")


if __name__ == "__main__":
    main()
