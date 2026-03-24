#!/usr/bin/env python3
"""
OBRIX Macro Position Pass
=========================
Sets meaningful starting positions for the 4 OBRIX macros:
  CHARACTER, MOVEMENT, COUPLING, SPACE

Only touches presets where ALL 4 macros are currently 0.0.
Reads DNA + mood + key params; applies deterministic rules.
"""

import json
import os
import glob
from collections import defaultdict

PRESETS_DIR = "/Users/joshuacramblet/Documents/GitHub/XO_OX-XOlokun/Presets"


# ---------------------------------------------------------------------------
# Rule engine
# ---------------------------------------------------------------------------

def pick(lo, hi, dna_value=None, bias=0.5):
    """
    Pick a deterministic value within [lo, hi].
    bias=0.0 → low end, 0.5 → midpoint (default), 1.0 → high end.
    When dna_value is provided, bias scales with it linearly.
    """
    if dna_value is not None:
        # Map dna_value (0..1) proportionally into the range,
        # but clamp to the actual range.
        t = max(0.0, min(1.0, dna_value))
    else:
        t = bias
    return round(lo + t * (hi - lo), 3)


def compute_macros(d):
    """
    Given a loaded .xometa dict, return the four desired macro values
    as (character, movement, coupling, space).
    Returns None if the preset already has any non-zero macro (skip it).
    """
    params = d.get("parameters", {}).get("Obrix", {})
    dna    = d.get("dna", {})
    mood   = d.get("mood", "")

    # Read current macro values
    char_cur  = params.get("obrix_macroCharacter", 0.0)
    mov_cur   = params.get("obrix_macroMovement",  0.0)
    coup_cur  = params.get("obrix_macroCoupling",  0.0)
    space_cur = params.get("obrix_macroSpace",     0.0)

    if any(v != 0.0 for v in [char_cur, mov_cur, coup_cur, space_cur]):
        return None  # already intentionally set — skip

    brightness = dna.get("brightness", 0.0)
    aggression = dna.get("aggression", 0.0)
    density    = dna.get("density",    0.0)
    movement   = dna.get("movement",   0.0)
    space_dna  = dna.get("space",      0.0)

    mod1_type = params.get("obrix_mod1Type", 0)
    mod2_type = params.get("obrix_mod2Type", 0)

    # --- CHARACTER ---
    character = 0.0
    if aggression > 0.4 or density > 0.5:
        # Sound already has character; macro takes it further.
        # Stronger DNA → higher end of 0.25–0.45 range.
        driver = max(aggression, density)
        character = pick(0.25, 0.45, dna_value=driver)
    elif brightness > 0.6:
        character = 0.20

    # --- MOVEMENT ---
    move_val = 0.0
    if movement > 0.5:
        move_val = pick(0.25, 0.35, dna_value=movement)
    elif mood in ("Atmosphere", "Aether"):
        # Gentle always-on drift; higher end if both qualify
        move_val = pick(0.15, 0.25)
    elif mod1_type != 0 or mod2_type != 0:
        move_val = 0.20

    # --- COUPLING ---
    coupling = 0.0
    if mood == "Entangled":
        coupling = pick(0.50, 0.65)
    elif mood == "Family":
        coupling = pick(0.30, 0.45)

    # --- SPACE ---
    space = 0.0
    if space_dna > 0.5:
        space = pick(0.30, 0.50, dna_value=space_dna)
    elif mood == "Submerged":
        space = pick(0.40, 0.60)
    elif mood == "Aether":
        space = pick(0.25, 0.40)
    elif mood == "Atmosphere":
        space = pick(0.20, 0.35)

    return (character, move_val, coupling, space)


# ---------------------------------------------------------------------------
# Main pass
# ---------------------------------------------------------------------------

def run():
    pattern = os.path.join(PRESETS_DIR, "**", "Obrix_*.xometa")
    files   = sorted(glob.glob(pattern, recursive=True))

    total       = len(files)
    skipped     = 0   # already had non-zero macros
    all_zero    = 0   # all macros zero — candidates
    updated     = 0   # actually changed (at least one macro set)

    # Track what changed
    changed_character = 0
    changed_movement  = 0
    changed_coupling  = 0
    changed_space     = 0
    changed_per_mood  = defaultdict(int)
    changed_by_rule   = defaultdict(int)  # which rules fired

    for fp in files:
        with open(fp) as f:
            d = json.load(f)

        result = compute_macros(d)

        if result is None:
            skipped += 1
            continue

        all_zero += 1
        character, movement, coupling, space = result

        if character == 0.0 and movement == 0.0 and coupling == 0.0 and space == 0.0:
            # No rule fired — leave untouched
            continue

        # Apply changes
        params = d["parameters"]["Obrix"]
        dna    = d.get("dna", {})
        mood   = d.get("mood", "")

        params["obrix_macroCharacter"] = character
        params["obrix_macroMovement"]  = movement
        params["obrix_macroCoupling"]  = coupling
        params["obrix_macroSpace"]     = space

        with open(fp, "w") as f:
            json.dump(d, f, indent=2)
            f.write("\n")

        updated += 1
        changed_per_mood[mood] += 1

        if character != 0.0:
            changed_character += 1
            # Which sub-rule?
            aggression = dna.get("aggression", 0.0)
            density    = dna.get("density",    0.0)
            brightness = dna.get("brightness", 0.0)
            if aggression > 0.4 or density > 0.5:
                changed_by_rule["CHARACTER: aggression/density"] += 1
            elif brightness > 0.6:
                changed_by_rule["CHARACTER: brightness"] += 1

        if movement != 0.0:
            changed_movement += 1
            mov_dna   = dna.get("movement", 0.0)
            mod1_type = d["parameters"]["Obrix"].get("obrix_mod1Type", 0)
            mod2_type = d["parameters"]["Obrix"].get("obrix_mod2Type", 0)
            if mov_dna > 0.5:
                changed_by_rule["MOVEMENT: dna.movement>0.5"] += 1
            elif mood in ("Atmosphere", "Aether"):
                changed_by_rule[f"MOVEMENT: mood={mood}"] += 1
            elif mod1_type != 0 or mod2_type != 0:
                changed_by_rule["MOVEMENT: lfo active"] += 1

        if coupling != 0.0:
            changed_coupling += 1
            changed_by_rule[f"COUPLING: mood={mood}"] += 1

        if space != 0.0:
            changed_space += 1
            space_dna = dna.get("space", 0.0)
            if space_dna > 0.5:
                changed_by_rule["SPACE: dna.space>0.5"] += 1
            elif mood == "Submerged":
                changed_by_rule["SPACE: mood=Submerged"] += 1
            elif mood == "Aether":
                changed_by_rule["SPACE: mood=Aether"] += 1
            elif mood == "Atmosphere":
                changed_by_rule["SPACE: mood=Atmosphere"] += 1

    # --- Report ---
    print("=" * 60)
    print("OBRIX MACRO POSITION PASS — COMPLETE")
    print("=" * 60)
    print(f"Total Obrix presets found  : {total}")
    print(f"Skipped (had non-zero macros): {skipped}")
    print(f"Candidates (all-zero macros): {all_zero}")
    print(f"Updated (at least 1 macro set): {updated}")
    print(f"Left at all-zero (no rule fired): {all_zero - updated}")
    print()
    print("— Macro breakdown —")
    print(f"  CHARACTER set  : {changed_character}")
    print(f"  MOVEMENT set   : {changed_movement}")
    print(f"  COUPLING set   : {changed_coupling}")
    print(f"  SPACE set      : {changed_space}")
    print()
    print("— Updates by mood —")
    for mood, count in sorted(changed_per_mood.items(), key=lambda x: -x[1]):
        print(f"  {mood:<14} : {count}")
    print()
    print("— Updates by rule —")
    for rule, count in sorted(changed_by_rule.items(), key=lambda x: -x[1]):
        print(f"  {rule:<40} : {count}")
    print("=" * 60)


if __name__ == "__main__":
    run()
