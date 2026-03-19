#!/usr/bin/env python3
"""
xpn_entangled_continuation_ultra_diverse.py
Generates 80 Entangled mood presets with extreme DNA diversity.
16 corner combinations × 5 variants = 80 presets.
"""

import json
import os
import random

random.seed(42)

OUTPUT_DIR = os.path.join(
    os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
    "Presets", "XOmnibus", "Entangled"
)
os.makedirs(OUTPUT_DIR, exist_ok=True)

# ---------------------------------------------------------------------------
# Engine roster
# ---------------------------------------------------------------------------
ENGINES = [
    "ODDFELIX", "ODDOSCAR", "OBLONG", "OBESE", "OPAL", "ORBITAL", "ORGANON",
    "OUROBOROS", "OBSIDIAN", "ORIGAMI", "ORACLE", "OBSCURA", "OCEANIC", "OCELOT",
    "OPTIC", "OBLIQUE", "OSPREY", "OSTERIA", "OWLFISH", "OHM", "ORPHICA",
    "OBBLIGATO", "OTTONI", "OLE", "OVERDUB", "ODYSSEY", "OVERWORLD", "OVERBITE",
    "OMBRE", "ORCA", "OCTOPUS", "OVERLAP", "OUTWIT",
]

COUPLING_TYPES = [
    "RESONANCE_SHARE", "SPECTRAL_MORPH", "HARMONIC_FOLD",
    "CHAOS_INJECT", "VELOCITY_COUPLE",
    "PHASE_LOCK", "FORMANT_BRIDGE", "SUBHARMONIC_WEAVE",
    "TIMBRAL_BLEED", "PITCH_ENTANGLE",
]

# Prefer the five most interesting coupling types (higher weight)
COUPLING_WEIGHTS = [4, 4, 4, 4, 4, 1, 1, 1, 1, 1]

# Engine-specific macro label sets (character → macro 0, movement → macro 1,
# coupling → macro 2, space → macro 3) — approximated per engine family.
ENGINE_MACRO_LABELS = {
    "ODDFELIX":   ["FELIX", "DRIFT", "ENTANGLE", "FIELD"],
    "ODDOSCAR":   ["OSCAR", "STIR", "ENTANGLE", "DEPTH"],
    "OBLONG":     ["BOB", "FIDGET", "WEAVE", "CHAMBER"],
    "OBESE":      ["FAT", "PULSE", "BIND", "MASS"],
    "OPAL":       ["GRAIN", "SCATTER", "FUSE", "CLOUD"],
    "ORBITAL":    ["ORBIT", "SPIN", "HARMONIC", "SPHERE"],
    "ORGANON":    ["VOICE", "BREATH", "CHOIR", "NAVE"],
    "OUROBOROS":  ["COIL", "CYCLE", "FEED", "ABYSS"],
    "OBSIDIAN":   ["BLADE", "EDGE", "SHARD", "VOID"],
    "ORIGAMI":    ["FOLD", "CREASE", "LAYER", "PLANE"],
    "ORACLE":     ["SIGHT", "OMEN", "VISION", "ETHER"],
    "OBSCURA":    ["SHADOW", "VEIL", "BLUR", "MIST"],
    "OCEANIC":    ["TIDE", "SURGE", "CURRENT", "DEEP"],
    "OCELOT":     ["PROWL", "SPRING", "CLAW", "BRUSH"],
    "OPTIC":      ["LENS", "PRISM", "REFRACT", "RAY"],
    "OBLIQUE":    ["SLANT", "SHEAR", "SKEW", "DRIFT"],
    "OSPREY":     ["DIVE", "LIFT", "SOAR", "THERMAL"],
    "OSTERIA":    ["FERMENT", "SEASON", "SPICE", "CELLAR"],
    "OWLFISH":    ["DEPTH", "FEEDING", "DEFENSE", "PRESSURE"],
    "OHM":        ["COMMUNE", "MEDDLING", "RESONANCE", "SPACE"],
    "ORPHICA":    ["PLUCK", "SCATTER", "RESONATE", "HALL"],
    "OBBLIGATO":  ["WIND", "BOND", "FLOW", "AIR"],
    "OTTONI":     ["BRASS", "GROW", "BLAZE", "BELL"],
    "OLE":        ["DRAMA", "RHYTHM", "HEAT", "FLOOR"],
    "OVERDUB":    ["VOICE", "DRIVE", "DELAY", "SPRING"],
    "ODYSSEY":    ["WAVE", "WANDER", "GLIDE", "HALL"],
    "OVERWORLD":  ["ERA", "GLITCH", "CHIP", "CRT"],
    "OVERBITE":   ["FANG", "BASS", "BITE", "BODY"],
    "OMBRE":      ["SHADE", "GRADIENT", "FADE", "DRIFT"],
    "ORCA":       ["HUNT", "BREACH", "ECHO", "DEEP"],
    "OCTOPUS":    ["ARM", "INK", "CAMOUFLAGE", "DEEP"],
    "OVERLAP":    ["KNOT", "FOLD", "WEAVE", "HALL"],
    "OUTWIT":     ["CELL", "MUTATE", "EVOLVE", "CHAOS"],
}

# ---------------------------------------------------------------------------
# Corner definitions
# ---------------------------------------------------------------------------
# Each corner specifies XHIGH/XLOW for certain DNA dims; the rest are mid.
DNA_DIMS = ["brightness", "warmth", "movement", "density", "space", "aggression"]

def xlow():
    return round(random.uniform(0.02, 0.13), 4)

def xhigh():
    return round(random.uniform(0.87, 0.99), 4)

def mid():
    return round(random.uniform(0.30, 0.70), 4)

CORNERS = [
    # (label, {dim: 'H'/'L'/None})
    ("BRIGHT_WARM_STILL_DENSE",    {"brightness":"H","warmth":"H","movement":"L","density":"H"}),
    ("DARK_COLD_KINETIC_INTIMATE", {"brightness":"L","warmth":"L","movement":"H","space":"L"}),
    ("BRIGHT_COLD_VAST_VIOLENT",   {"brightness":"H","warmth":"L","space":"H","aggression":"H"}),
    ("DARK_HOT_SPARSE_GENTLE",     {"brightness":"L","warmth":"H","density":"L","aggression":"L"}),
    ("BRIGHT_KINETIC_DENSE_VIOLENT",{"brightness":"H","movement":"H","density":"H","aggression":"H"}),
    ("DARK_STILL_SPARSE_VAST",     {"brightness":"L","movement":"L","density":"L","space":"H"}),
    ("HOT_DENSE_VAST_VIOLENT",     {"warmth":"H","density":"H","space":"H","aggression":"H"}),
    ("COLD_STILL_INTIMATE_GENTLE", {"warmth":"L","movement":"L","space":"L","aggression":"L"}),
    ("BRIGHT_WARM_INTIMATE_VIOLENT",{"brightness":"H","warmth":"H","space":"L","aggression":"H"}),
    ("DARK_KINETIC_DENSE_GENTLE",  {"brightness":"L","movement":"H","density":"H","aggression":"L"}),
    ("COLD_SPARSE_VAST_VIOLENT",   {"warmth":"L","density":"L","space":"H","aggression":"H"}),
    ("BRIGHT_COLD_KINETIC_SPARSE", {"brightness":"H","warmth":"L","movement":"H","density":"L"}),
    ("DARK_HOT_VAST_VIOLENT",      {"brightness":"L","warmth":"H","space":"H","aggression":"H"}),
    ("BRIGHT_DENSE_VAST_GENTLE",   {"brightness":"H","density":"H","space":"H","aggression":"L"}),
    ("HOT_KINETIC_SPARSE_GENTLE",  {"warmth":"H","movement":"H","density":"L","aggression":"L"}),
    ("DARK_COLD_DENSE_INTIMATE",   {"brightness":"L","warmth":"L","density":"H","space":"L"}),
]

assert len(CORNERS) == 16, "Need exactly 16 corners"

# ---------------------------------------------------------------------------
# Tag generation
# ---------------------------------------------------------------------------
def tags_for_dna(dna: dict, corner_label: str) -> list:
    tags = ["entangled"]
    if dna["brightness"] >= 0.85:
        tags.append("bright")
    elif dna["brightness"] <= 0.15:
        tags.append("dark")
    if dna["warmth"] >= 0.85:
        tags.append("warm")
    elif dna["warmth"] <= 0.15:
        tags.append("cold")
    if dna["movement"] >= 0.85:
        tags.append("kinetic")
    elif dna["movement"] <= 0.15:
        tags.append("still")
    if dna["density"] >= 0.85:
        tags.append("dense")
    elif dna["density"] <= 0.15:
        tags.append("sparse")
    if dna["space"] >= 0.85:
        tags.append("vast")
    elif dna["space"] <= 0.15:
        tags.append("intimate")
    if dna["aggression"] >= 0.85:
        tags.append("violent")
    elif dna["aggression"] <= 0.15:
        tags.append("gentle")
    # Always add a coupling flavour tag
    tags.append("resonant")
    return tags

# ---------------------------------------------------------------------------
# Parameter generation helpers
# ---------------------------------------------------------------------------
def engine_params_for_dna(engine: str, dna: dict) -> dict:
    """Generate plausible engine parameters derived from DNA values."""
    b = dna["brightness"]
    w = dna["warmth"]
    m = dna["movement"]
    d = dna["density"]
    s = dna["space"]
    a = dna["aggression"]

    # Generic macro params present in virtually every engine
    params = {
        "macro_character": round(0.3 + 0.4 * w + 0.3 * b, 4),
        "macro_movement":  round(m, 4),
        "macro_coupling":  round(0.6 + 0.35 * a, 4),
        "macro_space":     round(s, 4),
        "filter_cutoff":   round(0.2 + 0.7 * b, 4),
        "filter_resonance":round(0.1 + 0.5 * a, 4),
        "amp_attack":      round(0.05 + 0.4 * (1.0 - m), 4),
        "amp_release":     round(0.1 + 0.6 * s, 4),
        "reverb_mix":      round(0.15 + 0.55 * s, 4),
        "reverb_size":     round(0.2 + 0.6 * s, 4),
        "drive":           round(0.05 + 0.8 * a, 4),
        "density":         round(d, 4),
        "output_level":    round(0.7 + 0.25 * d, 4),
        "coupling_level":  round(0.55 + 0.4 * a, 4),
    }

    # Add small random perturbation per variant (seed already set globally)
    for k in list(params.keys()):
        params[k] = round(min(1.0, max(0.0, params[k] + random.uniform(-0.04, 0.04))), 4)

    return params

def coupling_intensity(amount: float) -> str:
    if amount >= 0.75:
        return "High"
    if amount >= 0.45:
        return "Medium"
    return "Low"

def description_for(corner_label: str, engine1: str, engine2: str, coupling_type: str) -> str:
    friendly = corner_label.replace("_", " ").title()
    ct = coupling_type.replace("_", " ").title()
    return (
        f"{friendly} character entanglement — {engine1} × {engine2} "
        f"via {ct}. Extreme DNA zones push sound design to the edges."
    )

# ---------------------------------------------------------------------------
# Main generation loop
# ---------------------------------------------------------------------------
all_presets = []

for corner_idx, (corner_label, corner_spec) in enumerate(CORNERS):
    # We need 5 variants per corner, each with different engine pair + coupling
    # Pre-select 5 diverse engine pairs for this corner
    used_pairs = set()
    variants = []
    attempts = 0
    while len(variants) < 5 and attempts < 200:
        attempts += 1
        e1, e2 = random.sample(ENGINES, 2)
        pair_key = tuple(sorted([e1, e2]))
        if pair_key in used_pairs:
            continue
        used_pairs.add(pair_key)
        variants.append((e1, e2))

    for v_idx, (engine1, engine2) in enumerate(variants):
        variant_num = v_idx + 1

        # Build DNA — extreme values for specified dims, mid for others
        dna = {}
        for dim in DNA_DIMS:
            spec = corner_spec.get(dim)
            if spec == "H":
                dna[dim] = xhigh()
            elif spec == "L":
                dna[dim] = xlow()
            else:
                dna[dim] = mid()

        # Pick coupling type (weighted toward interesting ones)
        coupling_type = random.choices(COUPLING_TYPES, weights=COUPLING_WEIGHTS, k=1)[0]
        coupling_amount = round(random.uniform(0.65, 0.99), 4)

        # Occasionally flip source/target direction
        if random.random() < 0.5:
            src, tgt = engine1, engine2
        else:
            src, tgt = engine2, engine1

        name = f"{corner_label}_ENT_{variant_num}"

        labels_e1 = ENGINE_MACRO_LABELS.get(engine1, ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"])
        labels_e2 = ENGINE_MACRO_LABELS.get(engine2, ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"])

        preset = {
            "schema_version": 1,
            "name": name,
            "mood": "Entangled",
            "engines": [engine1, engine2],
            "author": "XO_OX",
            "version": "1.0.0",
            "description": description_for(corner_label, engine1, engine2, coupling_type),
            "tags": tags_for_dna(dna, corner_label),
            "macroLabels": labels_e1,
            "couplingIntensity": coupling_intensity(coupling_amount),
            "dna": dna,
            "sonic_dna": dna,
            "parameters": {
                engine1: engine_params_for_dna(engine1, dna),
                engine2: engine_params_for_dna(engine2, dna),
            },
            "coupling": {
                "type": coupling_type,
                "source": src,
                "target": tgt,
                "amount": coupling_amount,
                "pairs": [
                    {
                        "source": src,
                        "target": tgt,
                        "type": coupling_type,
                        "amount": coupling_amount,
                    }
                ],
            },
            "macros": {
                "CHARACTER": round(dna["brightness"] * 0.5 + dna["warmth"] * 0.5, 4),
                "MOVEMENT":  dna["movement"],
                "COUPLING":  coupling_amount,
                "SPACE":     dna["space"],
            },
        }

        all_presets.append((name, preset))

print(f"Generated {len(all_presets)} presets in memory.")

# ---------------------------------------------------------------------------
# Write files
# ---------------------------------------------------------------------------
written = 0
for name, preset in all_presets:
    filename = f"{name}.xometa"
    filepath = os.path.join(OUTPUT_DIR, filename)
    with open(filepath, "w") as f:
        json.dump(preset, f, indent=2)
    written += 1

print(f"Written {written} files to: {OUTPUT_DIR}")

# Verify extreme-zone count per preset (≥4 dims must be XLOW or XHIGH)
violations = []
for name, preset in all_presets:
    dna = preset["dna"]
    extreme_count = sum(
        1 for v in dna.values()
        if v <= 0.15 or v >= 0.85
    )
    if extreme_count < 4:
        violations.append((name, extreme_count, dna))

if violations:
    print(f"\nWARNING: {len(violations)} presets have fewer than 4 extreme DNA dims:")
    for n, c, d in violations:
        print(f"  {n}: {c} extreme dims — {d}")
else:
    print("All 80 presets pass the ≥4 extreme DNA dimension check.")

print("\nDone.")
