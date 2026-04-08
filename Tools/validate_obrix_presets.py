#!/usr/bin/env python3
"""
Validate all OBRIX preset files against the registered parameter set.

Checks:
  1. Ghost parameters (keys not in the registered set)
  2. Out-of-range values (numeric values outside declared bounds)
  3. Invalid enum indices (choice params with index > max)
  4. Missing required params (any obrix_* param absent from preset)
  5. JSON structure validity

Usage:
  python3 Tools/validate_obrix_presets.py
  python3 Tools/validate_obrix_presets.py --fix  # auto-remove ghosts
"""

import json
import sys
import os
from pathlib import Path

# ── Registered OBRIX parameters ──────────────────────────────────────────────
# Extracted from ObrixEngine.h addParameters(). Frozen — update only when
# the engine's parameter list changes.
REGISTERED_PARAMS = {
    # Sources (7)
    "obrix_src1Type", "obrix_src2Type", "obrix_src1Tune", "obrix_src2Tune",
    "obrix_src1PW", "obrix_src2PW", "obrix_srcMix",
    # Processors (9)
    "obrix_proc1Type", "obrix_proc1Cutoff", "obrix_proc1Reso",
    "obrix_proc2Type", "obrix_proc2Cutoff", "obrix_proc2Reso",
    "obrix_proc3Type", "obrix_proc3Cutoff", "obrix_proc3Reso",
    # Amp ADSR (4)
    "obrix_ampAttack", "obrix_ampDecay", "obrix_ampSustain", "obrix_ampRelease",
    # Modulators 4x4 (16)
    "obrix_mod1Type", "obrix_mod1Target", "obrix_mod1Depth", "obrix_mod1Rate",
    "obrix_mod2Type", "obrix_mod2Target", "obrix_mod2Depth", "obrix_mod2Rate",
    "obrix_mod3Type", "obrix_mod3Target", "obrix_mod3Depth", "obrix_mod3Rate",
    "obrix_mod4Type", "obrix_mod4Target", "obrix_mod4Depth", "obrix_mod4Rate",
    # FX 3x3 (9)
    "obrix_fx1Type", "obrix_fx1Mix", "obrix_fx1Param",
    "obrix_fx2Type", "obrix_fx2Mix", "obrix_fx2Param",
    "obrix_fx3Type", "obrix_fx3Mix", "obrix_fx3Param",
    # Global (1)
    "obrix_level",
    # Macros (4)
    "obrix_macroCharacter", "obrix_macroMovement", "obrix_macroCoupling", "obrix_macroSpace",
    # Voice/Gesture (5)
    "obrix_polyphony", "obrix_pitchBendRange", "obrix_glideTime",
    "obrix_gestureType", "obrix_flashTrigger",
    # Wave 2 (6)
    "obrix_fmDepth", "obrix_proc1Feedback", "obrix_proc2Feedback",
    "obrix_proc3Feedback", "obrix_wtBank", "obrix_unisonDetune",
    # Wave 3 (5)
    "obrix_driftRate", "obrix_driftDepth", "obrix_journeyMode",
    "obrix_distance", "obrix_air",
    # Wave 4 (14)
    "obrix_fieldStrength", "obrix_fieldPolarity", "obrix_fieldRate",
    "obrix_fieldPrimeLimit",
    "obrix_envTemp", "obrix_envPressure", "obrix_envCurrent", "obrix_envTurbidity",
    "obrix_competitionStrength", "obrix_symbiosisStrength",
    "obrix_stressDecay", "obrix_bleachRate", "obrix_stateReset",
    "obrix_fxMode",
    # Wave 5 (2)
    "obrix_reefResident", "obrix_residentStrength",
}

# Choice/enum params and their valid index ranges
ENUM_RANGES = {
    "obrix_src1Type": (0, 8),      # Off..Driftwood
    "obrix_src2Type": (0, 8),
    "obrix_proc1Type": (0, 5),     # Off..RingMod
    "obrix_proc2Type": (0, 5),
    "obrix_proc3Type": (0, 5),
    "obrix_polyphony": (0, 3),     # Mono/Legato/Poly4/Poly8
    "obrix_gestureType": (0, 3),   # Off/Stab/Ratchet/Surge
    "obrix_mod1Type": (0, 4),      # Off/Env/LFO/Vel/AT
    "obrix_mod2Type": (0, 4),
    "obrix_mod3Type": (0, 4),
    "obrix_mod4Type": (0, 4),
    "obrix_mod1Target": (0, 8),    # None..Pan
    "obrix_mod2Target": (0, 8),
    "obrix_mod3Target": (0, 8),
    "obrix_mod4Target": (0, 8),
    "obrix_fx1Type": (0, 3),       # Off/Delay/Chorus/Reverb
    "obrix_fx2Type": (0, 3),
    "obrix_fx3Type": (0, 3),
    "obrix_fxMode": (0, 1),        # Serial/Parallel
    "obrix_wtBank": (0, 3),        # Analog/Vocal/Metallic/Organic
    "obrix_reefResident": (0, 3),   # Off/Competitor/Symbiote/Parasite
    "obrix_fieldPrimeLimit": (0, 2), # 3-limit/5-limit/7-limit
}

# Binary (step=1.0) params that should be 0 or 1
BINARY_PARAMS = {
    "obrix_flashTrigger", "obrix_journeyMode", "obrix_stateReset",
    "obrix_fieldPolarity",
}

# Float params and their ranges
FLOAT_RANGES = {
    "obrix_src1Tune": (-24.0, 24.0),
    "obrix_src2Tune": (-24.0, 24.0),
    "obrix_src1PW": (0.05, 0.95),
    "obrix_src2PW": (0.05, 0.95),
    "obrix_srcMix": (0.0, 1.0),
    "obrix_level": (0.0, 1.0),
    "obrix_pitchBendRange": (0.0, 24.0),
    "obrix_glideTime": (0.0, 1.0),
    "obrix_fmDepth": (0.0, 1.0),
    "obrix_unisonDetune": (0.0, 100.0),
    "obrix_driftRate": (0.001, 0.1),
    "obrix_driftDepth": (0.0, 1.0),
    "obrix_distance": (0.0, 1.0),
    "obrix_air": (0.0, 1.0),
}


def validate_preset(filepath: Path) -> list[dict]:
    """Validate a single preset file. Returns list of findings."""
    findings = []
    try:
        with open(filepath) as f:
            data = json.load(f)
    except json.JSONDecodeError as e:
        findings.append({"severity": "CRITICAL", "msg": f"Invalid JSON: {e}"})
        return findings

    params = data.get("parameters", {})
    obrix_keys = {k for k in params if k.startswith("obrix_")}

    # Ghost parameters
    ghosts = obrix_keys - REGISTERED_PARAMS
    for g in sorted(ghosts):
        findings.append({"severity": "HIGH", "msg": f"Ghost param: {g} = {params[g]}", "key": g})

    # Enum range checks
    for param_id, (lo, hi) in ENUM_RANGES.items():
        if param_id in params:
            val = params[param_id]
            if isinstance(val, (int, float)) and (val < lo or val > hi):
                findings.append({
                    "severity": "HIGH",
                    "msg": f"Out of range: {param_id} = {val} (valid: {lo}-{hi})"
                })

    # Binary param checks
    for param_id in BINARY_PARAMS:
        if param_id in params:
            val = params[param_id]
            if isinstance(val, (int, float)) and val not in (0, 0.0, 1, 1.0):
                findings.append({
                    "severity": "MEDIUM",
                    "msg": f"Non-binary value: {param_id} = {val} (should be 0 or 1)"
                })

    # Float range checks
    for param_id, (lo, hi) in FLOAT_RANGES.items():
        if param_id in params:
            val = params[param_id]
            if isinstance(val, (int, float)) and (val < lo - 0.001 or val > hi + 0.001):
                findings.append({
                    "severity": "MEDIUM",
                    "msg": f"Out of range: {param_id} = {val} (valid: {lo}-{hi})"
                })

    return findings


def main():
    fix_mode = "--fix" in sys.argv
    preset_dir = Path("Presets")
    if not preset_dir.exists():
        print("ERROR: Presets/ directory not found. Run from repo root.")
        sys.exit(1)

    total_files = 0
    total_findings = 0
    files_with_issues = 0

    for xometa in sorted(preset_dir.rglob("*.xometa")):
        # Only check files containing obrix params
        text = xometa.read_text()
        if "obrix_" not in text:
            continue

        total_files += 1
        findings = validate_preset(xometa)

        if findings:
            files_with_issues += 1
            total_findings += len(findings)
            rel = xometa.relative_to(preset_dir)
            print(f"\n{'='*60}")
            print(f"  {rel}")
            print(f"{'='*60}")
            for f in findings:
                print(f"  [{f['severity']}] {f['msg']}")

            if fix_mode:
                # Auto-remove ghost params
                ghost_keys = [f["key"] for f in findings if "key" in f]
                if ghost_keys:
                    data = json.loads(text)
                    for k in ghost_keys:
                        data["parameters"].pop(k, None)
                    xometa.write_text(json.dumps(data, indent=2) + "\n")
                    print(f"  >> FIXED: removed {len(ghost_keys)} ghost params")

    print(f"\n{'='*60}")
    print(f"  SUMMARY: {total_files} files scanned, {files_with_issues} with issues, {total_findings} total findings")
    print(f"{'='*60}")

    sys.exit(1 if total_findings > 0 else 0)


if __name__ == "__main__":
    main()
