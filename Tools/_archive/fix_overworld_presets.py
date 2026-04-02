#!/usr/bin/env python3
"""
fix_overworld_presets.py
Fixes Overworld presets that use UPPER_SNAKE_CASE parameter keys
(e.g. ow_ERA, ow_PULSE_DUTY) — replacing them with the canonical
camelCase IDs registered by OverworldEngine / xoverworld::Parameters.
"""

import json
import os
import sys
from pathlib import Path

# Complete mapping: preset key → canonical engine key
# Built from XOverworld/src/engine/Parameters.h
KEY_MAP = {
    # ERA
    "ow_ERA":              "ow_era",
    "ow_ERA_Y":            "ow_eraY",
    "ow_VOICE_MODE":       "ow_voiceMode",
    "ow_MASTER_VOL":       "ow_masterVol",
    "ow_MASTER_TUNE":      "ow_masterTune",
    # NES
    "ow_PULSE_DUTY":       "ow_pulseDuty",
    "ow_PULSE_SWEEP":      "ow_pulseSweep",
    "ow_TRI_ENABLE":       "ow_triEnable",
    "ow_NOISE_MODE":       "ow_noiseMode",
    "ow_NOISE_PERIOD":     "ow_noisePeriod",
    "ow_DPCM_ENABLE":      "ow_dpcmEnable",
    "ow_DPCM_RATE":        "ow_dpcmRate",
    "ow_NES_MIX":          "ow_nesMix",
    # Genesis FM
    "ow_FM_ALGORITHM":     "ow_fmAlgorithm",
    "ow_FM_FEEDBACK":      "ow_fmFeedback",
    "ow_FM_OP1_LEVEL":     "ow_fmOp1Level",
    "ow_FM_OP2_LEVEL":     "ow_fmOp2Level",
    "ow_FM_OP3_LEVEL":     "ow_fmOp3Level",
    "ow_FM_OP4_LEVEL":     "ow_fmOp4Level",
    "ow_FM_OP1_MULT":      "ow_fmOp1Mult",
    "ow_FM_OP2_MULT":      "ow_fmOp2Mult",
    "ow_FM_OP3_MULT":      "ow_fmOp3Mult",
    "ow_FM_OP4_MULT":      "ow_fmOp4Mult",
    "ow_FM_OP1_DETUNE":    "ow_fmOp1Detune",
    "ow_FM_OP2_DETUNE":    "ow_fmOp2Detune",
    "ow_FM_OP3_DETUNE":    "ow_fmOp3Detune",
    "ow_FM_OP4_DETUNE":    "ow_fmOp4Detune",
    "ow_FM_ATTACK":        "ow_fmAttack",
    "ow_FM_DECAY":         "ow_fmDecay",
    "ow_FM_SUSTAIN":       "ow_fmSustain",
    "ow_FM_RELEASE":       "ow_fmRelease",
    "ow_FM_LFO_RATE":      "ow_fmLfoRate",
    "ow_FM_LFO_DEPTH":     "ow_fmLfoDepth",
    # SNES
    "ow_BRR_SAMPLE":       "ow_brrSample",
    "ow_BRR_INTERP":       "ow_brrInterp",
    "ow_SNES_ATTACK":      "ow_snesAttack",
    "ow_SNES_DECAY":       "ow_snesDecay",
    "ow_SNES_SUSTAIN":     "ow_snesSustain",
    "ow_SNES_RELEASE":     "ow_snesRelease",
    "ow_PITCH_MOD":        "ow_pitchMod",
    "ow_NOISE_REPLACE":    "ow_noiseReplace",
    # Echo / FIR
    "ow_ECHO_DELAY":       "ow_echoDelay",
    "ow_ECHO_FEEDBACK":    "ow_echoFeedback",
    "ow_ECHO_MIX":         "ow_echoMix",
    "ow_ECHO_FIR_0":       "ow_echoFir0",
    "ow_ECHO_FIR_1":       "ow_echoFir1",
    "ow_ECHO_FIR_2":       "ow_echoFir2",
    "ow_ECHO_FIR_3":       "ow_echoFir3",
    "ow_ECHO_FIR_4":       "ow_echoFir4",
    "ow_ECHO_FIR_5":       "ow_echoFir5",
    "ow_ECHO_FIR_6":       "ow_echoFir6",
    "ow_ECHO_FIR_7":       "ow_echoFir7",
    # Glitch
    "ow_GLITCH_AMOUNT":    "ow_glitchAmount",
    "ow_GLITCH_TYPE":      "ow_glitchType",
    "ow_GLITCH_RATE":      "ow_glitchRate",
    "ow_GLITCH_DEPTH":     "ow_glitchDepth",
    "ow_GLITCH_SYNC":      "ow_glitchSync",
    "ow_GLITCH_MIX":       "ow_glitchMix",
    # Amp envelope
    "ow_AMP_ATTACK":       "ow_ampAttack",
    "ow_AMP_DECAY":        "ow_ampDecay",
    "ow_AMP_SUSTAIN":      "ow_ampSustain",
    "ow_AMP_RELEASE":      "ow_ampRelease",
    # Filter
    "ow_FILTER_CUTOFF":    "ow_filterCutoff",
    "ow_FILTER_RESO":      "ow_filterReso",
    "ow_FILTER_TYPE":      "ow_filterType",
    # Crush
    "ow_CRUSH_BITS":       "ow_crushBits",
    "ow_CRUSH_RATE":       "ow_crushRate",
    "ow_CRUSH_MIX":        "ow_crushMix",
    # Era drift / porta / mem
    "ow_ERA_DRIFT_RATE":   "ow_eraDriftRate",
    "ow_ERA_DRIFT_DEPTH":  "ow_eraDriftDepth",
    "ow_ERA_DRIFT_SHAPE":  "ow_eraDriftShape",
    "ow_ERA_PORTA_TIME":   "ow_eraPortaTime",
    "ow_ERA_MEM_TIME":     "ow_eraMemTime",
    "ow_ERA_MEM_MIX":      "ow_eraMemMix",
    # Vertex
    "ow_VERTEX_A":         "ow_vertexA",
    "ow_VERTEX_B":         "ow_vertexB",
    "ow_VERTEX_C":         "ow_vertexC",
    # GB / PCE extras
    "ow_GB_WAVE_SLOT":     "ow_gbWaveSlot",
    "ow_GB_PULSE_DUTY":    "ow_gbPulseDuty",
    "ow_PCE_WAVE_SLOT":    "ow_pceWaveSlot",
}


def fix_params(params: dict) -> tuple[dict, int]:
    """
    Replace UPPER_SNAKE_CASE keys with camelCase equivalents.
    Returns (fixed_dict, number_of_keys_changed).
    """
    fixed = {}
    changed = 0
    for k, v in params.items():
        canonical = KEY_MAP.get(k, k)
        if canonical != k:
            changed += 1
        fixed[canonical] = v
    return fixed, changed


def fix_preset(path: Path) -> int:
    """
    Load, fix, and overwrite a single .xometa file.
    Returns number of keys changed (0 = no change needed).
    """
    with open(path, "r", encoding="utf-8") as f:
        data = json.load(f)

    total_changed = 0

    # Flat preset format: {"parameters": {...}}
    if "parameters" in data and isinstance(data["parameters"], dict):
        params = data["parameters"]
        # Multi-engine nested format: {"parameters": {"Overworld": {...}}}
        if all(isinstance(v, dict) for v in params.values()):
            for engine_key, engine_params in params.items():
                fixed, n = fix_params(engine_params)
                if n:
                    data["parameters"][engine_key] = fixed
                    total_changed += n
        else:
            fixed, n = fix_params(params)
            if n:
                data["parameters"] = fixed
                total_changed += n

    if total_changed:
        with open(path, "w", encoding="utf-8") as f:
            json.dump(data, f, indent=2, ensure_ascii=False)
            f.write("\n")

    return total_changed


def main():
    # Search both the XOceanus Presets directory and the XOverworld Factory directory
    search_dirs = [
        Path("/Users/joshuacramblet/Documents/GitHub/XO_OX-XOceanus/Presets"),
        Path("/Users/joshuacramblet/Documents/GitHub/XOverworld/Presets"),
    ]

    total_files = 0
    total_fixed_files = 0
    total_keys = 0

    for search_dir in search_dirs:
        if not search_dir.exists():
            print(f"[skip] {search_dir} does not exist")
            continue
        for xometa in sorted(search_dir.rglob("*.xometa")):
            total_files += 1
            n = fix_preset(xometa)
            if n:
                total_fixed_files += 1
                total_keys += n
                print(f"  fixed {n:3d} key(s)  {xometa.name}")

    print()
    print(f"Scanned : {total_files} preset files")
    print(f"Fixed   : {total_fixed_files} files")
    print(f"Keys    : {total_keys} keys renamed")


if __name__ == "__main__":
    main()
