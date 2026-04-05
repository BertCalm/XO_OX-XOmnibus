#!/usr/bin/env python3
"""
Fix GitHub Issues #688 and #690 in BertCalm/XO_OX-XOmnibus

#688 (P1): 37% of presets missing macro data — M1-M4 faders undefined
#690 (P1): 63% of presets lack velocity-to-timbre parameters — D001 gap

This script:
1. Adds 'macros' block to presets that have 'macroLabels' but no 'macros'
   - Uses DNA-derived values (SPACE=dna.space, MOVEMENT~dna.movement,
     CHARACTER~blend of dna.brightness+dna.aggression, COUPLING=low default)
   - For non-standard labels: uses per-label averages from existing presets
2. Adds velocity-timbre parameters to presets missing them, for engines
   that have explicit vel params (discovered from source + presets)

Does NOT modify:
- Presets that already have meaningful macros (all-non-zero)
- Presets that already have velocity-timbre params
- Files in _quarantine or Quarantine directories
- Init.xometa files (leave them as-is)
"""

import os
import json
import sys
from pathlib import Path

REPO_ROOT = Path("/Users/joshuacramblet/Documents/GitHub/XO_OX-XOmnibus")

# ─────────────────────────────────────────────────────────────────────────────
# VELOCITY-TIMBRE PARAMETER REGISTRY
# Maps engine name → (param_name, default_value) for the canonical vel param.
# Only engines that have an EXPLICIT velocity-sensitivity parameter are included.
# Engines with hardcoded velocity in DSP (Origami, Oracle, Overdub, etc.)
# are intentionally excluded — they already apply vel, there's no knob to set.
# ─────────────────────────────────────────────────────────────────────────────
ENGINE_VEL_PARAMS = {
    # Engine name (as it appears in "engines" array) → (param_id, default_value)
    "OceanDeep":  ("deep_velCutoffAmt",        0.5),   # declared in OceandeepEngine.h
    "Orca":       ("orca_velCutoffAmt",         0.5),   # declared in OrcaEngine.h
    "Organism":   ("org_velCutoff",             0.5),   # declared in OrganismEngine.h
    "Overtone":   ("over_velBright",            0.45),  # declared in OvertoneEngine.h
    "Ombre":      ("ombre_velCutoffScale",       0.4),   # declared in OmbreEngine.h
    "Obiont":     ("obnt_velSens",              0.7),   # declared in ObiontEngine.h
    "Opal":       ("opal_ampVelSens",           0.4),   # declared in OpalEngine.h
    "Overbite":   ("poss_ampVelSens",           0.5),   # declared in BiteEngine.h
    "Opera":      ("opera_velToFilter",         0.4),   # declared in OperaEngine.h
    "Opcode":     ("opco_velToIndex",           0.5),   # declared in OpcodeEngine.h
    "Offering":   ("ofr_velToSnap",             0.6),   # declared in OfferingEngine.h
    "Ocelot":     ("ocelot_floorVelocity",      0.75),  # declared in OcelotParameters.h
    "Ouie":       ("ouie_velCutoffAmt",         0.5),   # found in presets
    "OpenSky":    ("sky_velSensitivity",        0.7),   # found in presets
    "Orphica":    ("orph_velocitySensitivity",  0.75),  # found in presets
    "Oware":      ("owr_malletVelocity",        0.75),  # found in presets
    "Owlfish":    ("owl_velocity",              0.65),  # found in presets
    "Oblong":     ("bob_velFilter",             0.5),   # found in presets
}

# For Ostinato, vel params are per-seat (osti_seat1_velSens through osti_seat8_velSens).
# We handle this specially below.
OSTINATO_VEL_SEATS = [
    "osti_seat1_velSens",
    "osti_seat2_velSens",
    "osti_seat3_velSens",
    "osti_seat4_velSens",
    "osti_seat5_velSens",
    "osti_seat6_velSens",
    "osti_seat7_velSens",
    "osti_seat8_velSens",
]
OSTINATO_VEL_DEFAULT = 0.7


# ─────────────────────────────────────────────────────────────────────────────
# MACRO LABEL AVERAGES (computed from 9,585 presets with both macros and DNA)
# Used as fallback for non-standard macro labels.
# ─────────────────────────────────────────────────────────────────────────────
LABEL_AVERAGES = {
    "CHARACTER": 0.499,
    "MOVEMENT":  0.456,
    "COUPLING":  0.267,
    "SPACE":     0.523,
    # Engine-specific standard labels
    "TENSION":   0.422,
    "KNOT":      0.454,
    "MUTATE":    0.271,
    "WEAVE":     0.533,
    "MACHINE":   0.436,
    "PUNCH":     0.530,
    "PRESSURE":  0.635,
    "FOLD":      0.376,
    "CREATURE":  0.243,
    "DIG":       0.534,
    "CITY":      0.416,
    "FLIP":      0.214,
    "DUST":      0.280,
    "WRECK":     0.256,
    "ABYSS":     0.603,
    "COLOR":     0.639,
    "PROWL":     0.450,
    "FOLIAGE":   0.482,
    "ECOSYSTEM": 0.478,
    "CANOPY":    0.469,
    "BOUNCE":    0.423,
    "BELLY":     0.552,
    "BITE":      0.304,
    "SCURRY":    0.239,
    "TRASH":     0.174,
    "SPECTRAL":  0.579,
    "PLAY DEAD": 0.299,
    "DRIFT":     0.543,
    "HAMMER":    0.366,
    "CARTILAGE": 0.503,
    "AMPULLAE":  0.595,
    "CURRENT":   0.532,
    "PLUCK":     0.442,
    "SURFACE":   0.536,
    "FRACTURE":  0.244,
    "DIVINE":    0.429,
    "GROUP":     0.173,
    "VELOCITY":  0.300,
    "MOTION":    0.523,
    "COUPLE":    0.384,
    "EVOLUTION": 0.571,
    "BREATH":    0.596,
    "DRAMA":     0.500,
    "BOND":      0.508,
    "GROW":      0.575,
    "EMBOUCHURE":0.555,
    "LAKE":      0.575,
    "FOREIGN":   0.136,
    "FUEGO":     0.534,
    "ISLA":      0.516,
    "SIDES":     0.475,
    "SPECTRUM":  0.565,
    "WIND":      0.472,
    "MISCHIEF":  0.247,
    "DEPTH":     0.545,
    "MEDDLING":  0.159,
    "COMMUNE":   0.112,
    "JAM":       0.458,
    "MEADOW":    0.609,
    "EVOLVE":    0.369,
    "ERA":       0.550,
    "CHAOS":     0.296,
    "HUNT":      0.396,
    "RULE":      0.415,
    "SEED":      0.278,
    "BLOOM":     0.393,
    "RISE":      0.504,
    "WIDTH":     0.681,
    "GLOW":      0.542,
    "AIR":       0.726,
    "SCATTER":   0.547,
    "PULSE":     0.430,
    "SOLVE":     0.430,
    "SYNAPSE":   0.564,
    "CHROMATOPHORE": 0.669,
    "DEN":       0.331,
    "VOICE":     0.535,
    "GRAVITY":   0.435,
    "ENTRAIN":   0.376,
    "GRIT":      0.353,
    # Additional labels
    "WARMTH":    0.5,
    "TIDE":      0.5,
    "ERA SWEEP": 0.5,
    "CRUSH":     0.35,
    "GLITCH":    0.2,
    "ORBIT":     0.45,
    "PHASE":     0.45,
    "SIGNAL":    0.5,
    "CYCLE":     0.45,
    "CUNNING":   0.35,
    "REFLEX":    0.4,
    "SPREAD":    0.4,
    "FLOW":      0.5,
    "VISION":    0.45,
    "VEIL":      0.4,
    "DARKNESS":  0.35,
    "SHIMMER":   0.5,
    "GRADIENT":  0.5,
    "TAPE":      0.4,
    "DRIVE":     0.45,
    "ODDITY":    0.4,
    "MASS":      0.45,
    "DENSITY":   0.45,
    "LOGIC":     0.4,
    "SYSTEM":    0.4,
    "REFRACT":   0.5,
    "PROPHECY":  0.45,
    "GRAIN":     0.4,
    "ECHO":      0.4,
    "PANIC":     0.3,
    "SEND":      0.4,
    "ORBIT":     0.45,
    "ANGLE":     0.4,
    "CASCADE":   0.45,
    "MINERAL":   0.45,
    "HAZE":      0.4,
    "FORMANT":   0.5,
    "PRISM":     0.5,
    "MORPH":     0.45,
    "ODYSSEY":   0.5,
    "HUNT":      0.4,
    "CHORUS":    0.5,
    "STAGE":     0.45,
    "IMPACT":    0.5,
    "RESONANCE": 0.5,
    "ACID SWEEP": 0.4,
    "BOB MODE":  0.5,
    "CURIOSITY": 0.5,
    "FILTER CHAR": 0.5,
    "BODY":      0.5,
    "FILTER OPEN": 0.4,
    "TWITCH":    0.3,
    "ATTACK":    0.45,
    "FILTER DRIFT": 0.4,
    "SLUMBER":   0.4,
    "TAIL":      0.45,
    "INVESTIGATE": 0.4,
    "SHAPE WARP": 0.3,
    "SYNC BITE": 0.3,
    "WEIGHT":    0.45,
    "ABBERATION": 0.35,
    "FOCUS":     0.5,
}


def compute_macro_value(label: str, dna: dict) -> float:
    """
    Compute a sensible macro value from DNA dimensions.

    Correlation analysis from 9,585 presets showed:
      SPACE = exactly dna.space (1.0 corr)
      MOVEMENT ~= dna.movement (0.69 corr)
      CHARACTER ~= blend of brightness+aggression (0.43 each)
      COUPLING = low default (~0.27 avg), higher for multi-engine

    For non-standard labels: use per-label average from existing presets.
    """
    brightness = dna.get("brightness", 0.5)
    warmth = dna.get("warmth", 0.5)
    movement = dna.get("movement", 0.5)
    density = dna.get("density", 0.5)
    space = dna.get("space", 0.5)
    aggression = dna.get("aggression", 0.5)

    label_upper = label.upper()

    # Standard 4 macros: DNA-driven
    if label_upper == "CHARACTER":
        return round(min(1.0, max(0.0, (brightness * 0.5 + aggression * 0.5))), 3)
    elif label_upper == "MOVEMENT":
        return round(min(1.0, max(0.0, movement)), 3)
    elif label_upper == "COUPLING":
        return round(0.2, 3)  # Low default for single-engine (COUPLING fader avg is 0.267)
    elif label_upper == "SPACE":
        return round(min(1.0, max(0.0, space)), 3)

    # DNA-adjacent standard labels
    elif label_upper in ("SPECTRAL", "COLOR", "BRIGHTNESS"):
        return round(min(1.0, max(0.0, brightness)), 3)
    elif label_upper in ("WARMTH", "TIDE", "DEPTH"):
        return round(min(1.0, max(0.0, warmth)), 3)
    elif label_upper in ("DENSITY", "MASS", "GRAIN"):
        return round(min(1.0, max(0.0, density)), 3)
    elif label_upper in ("DRIVE", "CRUSH", "GRIT", "BITE", "AGGRESSION", "IMPACT"):
        return round(min(1.0, max(0.0, aggression)), 3)
    elif label_upper in ("MOTION", "EVOLVE", "EVOLUTION", "DRIFT", "FLOW"):
        return round(min(1.0, max(0.0, movement)), 3)
    elif label_upper in ("AIR", "RISE", "WIDTH", "GLOW"):
        return round(min(1.0, max(0.0, space * 0.8 + 0.2)), 3)
    elif label_upper == "PRESSURE":
        return round(min(1.0, max(0.0, density * 0.7 + aggression * 0.3)), 3)

    # Use per-label average for all other labels
    if label_upper in LABEL_AVERAGES:
        return LABEL_AVERAGES[label_upper]
    # Unknown label: use neutral default
    return 0.45


def should_skip_file(path: str) -> bool:
    """Skip quarantined files and source Init.xometa files."""
    p = path.lower()
    if "_quarantine" in p or "/quarantine/" in p:
        return True
    # Skip Init.xometa files in Source/Engines (they're init patches, not real presets)
    if "source/engines" in p and os.path.basename(path).lower() == "init.xometa":
        return True
    return False


def has_meaningful_macros(d: dict) -> bool:
    """Return True if preset already has a non-empty, non-all-zero macros block."""
    macros = d.get("macros")
    if not isinstance(macros, dict) or len(macros) == 0:
        return False
    # All-zero = init stub = not meaningful
    if all(isinstance(v, (int, float)) and v == 0 for v in macros.values()):
        return False
    return True


def has_velocity_timbre(d: dict) -> bool:
    """Return True if preset already has a velocity-timbre parameter."""
    params = d.get("parameters", {})
    for eng, eng_params in params.items():
        if not isinstance(eng_params, dict):
            continue
        for pk, pv in eng_params.items():
            pl = pk.lower()
            # Check for known vel param name patterns with non-zero value
            is_vel_param = (
                ("vel" in pl and "level" not in pl) or
                "velbright" in pl or
                "velsens" in pl or
                "velcutoff" in pl or
                "veltofilter" in pl or
                "veltobright" in pl or
                "veltobody" in pl or
                "veltosnap" in pl or
                "veltoattack" in pl or
                "veltoeffort" in pl or
                "veltoindex" in pl or
                "floorvelocity" in pl
            )
            if is_vel_param and isinstance(pv, (int, float)) and pv > 0:
                return True
    return False


def get_vel_param_additions(d: dict) -> dict:
    """
    Return dict of {engine_name: {param_name: value}} for velocity params to add.
    Only adds params for engines that have explicit vel params AND the preset
    doesn't already have one.
    """
    engines = d.get("engines", [])
    if not engines:
        return {}
    params = d.get("parameters", {})
    additions = {}

    for engine in engines:
        eng_params = params.get(engine, {})
        if not isinstance(eng_params, dict):
            continue

        # Check if this engine already has a vel param in this preset
        already_has = False
        for pk, pv in eng_params.items():
            pl = pk.lower()
            is_vel = (
                ("vel" in pl and "level" not in pl) or
                "floorvelocity" in pl
            )
            if is_vel and isinstance(pv, (int, float)) and pv > 0:
                already_has = True
                break
        if already_has:
            continue

        # Special case: Ostinato (per-seat vel params OR flat osti_velFilter)
        if engine == "Ostinato":
            # Check if this is a per-seat preset or a flat preset
            has_seats = any("seat" in k for k in eng_params)
            if has_seats:
                # Only add seats that exist in this preset
                seat_additions = {}
                for seat_param in OSTINATO_VEL_SEATS:
                    # Only add vel param for seats that have a corresponding level param
                    seat_num = seat_param.split("_")[1]  # e.g., "seat1"
                    level_param = f"osti_{seat_num}_level"
                    if level_param in eng_params:
                        seat_additions[seat_param] = OSTINATO_VEL_DEFAULT
                if seat_additions:
                    additions[engine] = seat_additions
            else:
                # Flat Ostinato preset: use osti_velFilter
                additions[engine] = {"osti_velFilter": 0.5}
            continue

        # Standard case: engine has a single vel param
        if engine in ENGINE_VEL_PARAMS:
            param_name, default_val = ENGINE_VEL_PARAMS[engine]
            # Scale default by DNA aggression/movement for expressivity
            dna = d.get("dna", {})
            aggression = dna.get("aggression", 0.5)
            movement = dna.get("movement", 0.5)
            # Pads/atmospheres get lower vel sensitivity (0.3-0.5)
            # Leads/basses get higher (0.5-0.8)
            # Percussion gets highest (0.6-0.9)
            mood = d.get("mood", "").lower()
            if mood in ("foundation", "atmosphere", "aether", "submerged", "ethereal"):
                scale = 0.3 + aggression * 0.2  # 0.3-0.5
            elif mood in ("kinetic", "flux", "prism"):
                scale = 0.5 + aggression * 0.3  # 0.5-0.8
            else:
                scale = 0.4 + aggression * 0.25  # 0.4-0.65

            final_val = round(min(0.9, max(0.2, default_val * scale / 0.5)), 3)
            # Clamp to reasonable range
            final_val = round(min(0.85, max(0.2, final_val)), 3)
            additions[engine] = {param_name: final_val}

    return additions


def fix_preset(path: str, stats: dict) -> bool:
    """
    Fix a single preset file. Returns True if any changes were made.
    """
    if should_skip_file(path):
        return False

    try:
        with open(path, "r", encoding="utf-8") as f:
            d = json.load(f)
    except (json.JSONDecodeError, IOError) as e:
        stats["errors"].append(f"{path}: {e}")
        return False

    changed = False
    changes = []

    # ── Issue #688: Add macros block ──────────────────────────────────────────
    if not has_meaningful_macros(d):
        macro_labels = d.get("macroLabels", [])
        if macro_labels and isinstance(macro_labels, list) and len(macro_labels) >= 2:
            dna = d.get("dna", {})
            # Determine if this is a multi-engine preset for COUPLING adjustment
            engines = d.get("engines", [])
            new_macros = {}
            for label in macro_labels:
                val = compute_macro_value(label, dna)
                # For multi-engine presets, set COUPLING higher
                if label.upper() == "COUPLING" and len(engines) > 1:
                    coupling_amount = 0.0
                    for pair in d.get("coupling", {}).get("pairs", []):
                        coupling_amount = max(coupling_amount, pair.get("amount", 0))
                    # Map coupling amount to macro value (0.3-0.7)
                    val = round(min(0.7, max(0.2, coupling_amount * 0.6 + 0.15)), 3)
                new_macros[label] = val
            d["macros"] = new_macros
            changed = True
            changes.append(f"#688: added macros {new_macros}")
            stats["macros_fixed"] += 1

    # ── Issue #690: Add velocity-timbre params ────────────────────────────────
    if not has_velocity_timbre(d):
        vel_additions = get_vel_param_additions(d)
        if vel_additions:
            params = d.get("parameters", {})
            for engine, eng_vel_params in vel_additions.items():
                if engine not in params:
                    params[engine] = {}
                params[engine].update(eng_vel_params)
            d["parameters"] = params
            changed = True
            changes.append(f"#690: added vel params {vel_additions}")
            stats["vel_fixed"] += 1

    if changed:
        try:
            with open(path, "w", encoding="utf-8") as f:
                json.dump(d, f, indent=2, ensure_ascii=False)
                f.write("\n")
            stats["files_modified"] += 1
            if len(stats.get("change_log", [])) < 20:
                stats.setdefault("change_log", []).append(
                    f"{os.path.basename(path)}: {'; '.join(changes)}"
                )
        except IOError as e:
            stats["errors"].append(f"{path}: write error: {e}")
            return False

    return changed


def main():
    print("Scanning for .xometa files...")

    all_files = []
    for root, dirs, files in os.walk(REPO_ROOT):
        # Skip hidden dirs and node_modules
        dirs[:] = [
            d for d in dirs
            if not d.startswith(".") and d != "node_modules"
        ]
        for f in files:
            if f.endswith(".xometa"):
                all_files.append(os.path.join(root, f))

    print(f"Found {len(all_files)} .xometa files")

    stats = {
        "total": len(all_files),
        "files_modified": 0,
        "macros_fixed": 0,
        "vel_fixed": 0,
        "errors": [],
        "change_log": [],
    }

    for i, path in enumerate(all_files):
        if i % 1000 == 0:
            print(f"  Progress: {i}/{len(all_files)} ({stats['files_modified']} modified so far)...")
        fix_preset(path, stats)

    print("\n" + "=" * 60)
    print("RESULTS")
    print("=" * 60)
    print(f"Total files scanned:      {stats['total']}")
    print(f"Files modified:           {stats['files_modified']}")
    print(f"Macros added (#688):      {stats['macros_fixed']}")
    print(f"Vel params added (#690):  {stats['vel_fixed']}")
    print(f"Errors:                   {len(stats['errors'])}")

    if stats["errors"]:
        print("\nERRORS:")
        for e in stats["errors"][:10]:
            print(f"  {e}")

    if stats["change_log"]:
        print("\nSAMPLE CHANGES (first 20):")
        for c in stats["change_log"]:
            print(f"  {c}")

    return stats


if __name__ == "__main__":
    main()
