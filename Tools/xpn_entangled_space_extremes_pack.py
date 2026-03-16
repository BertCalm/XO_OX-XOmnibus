#!/usr/bin/env python3
"""
xpn_entangled_space_extremes_pack.py
Generates 80 .xometa presets targeting Entangled mood with extreme space values.
- First 40: space 0.02–0.10 (XLOW, claustrophobic/intimate)
- Last 40: space 0.90–0.99 (XHIGH, vast/cosmic)
"""

import json
import os
import random

OUTPUT_DIR = "Presets/Entangled"

ENGINES = [
    "ODDFELIX", "ODDOSCAR", "OVERDUB", "ODYSSEY", "OBLONG", "OBESE",
    "ONSET", "OVERWORLD", "OPAL", "ORBITAL", "ORGANON", "OUROBOROS",
    "OBSIDIAN", "OVERBITE", "ORIGAMI", "ORACLE", "OBSCURA", "OCEANIC",
    "OCELOT", "OPTIC", "OBLIQUE", "OSPREY", "OSTERIA", "OWLFISH",
    "OHM", "ORPHICA", "OBBLIGATO", "OTTONI", "OLE", "OMBRE",
    "ORCA", "OCTOPUS", "OVERLAP", "OUTWIT"
]

COUPLING_TYPES = [
    "FREQUENCY_SHIFT", "AMPLITUDE_MOD", "FILTER_MOD", "PITCH_SYNC",
    "TIMBRE_BLEND", "ENVELOPE_LINK", "HARMONIC_FOLD", "CHAOS_INJECT",
    "RESONANCE_SHARE", "SPATIAL_COUPLE", "SPECTRAL_MORPH", "VELOCITY_COUPLE"
]

XLOW_NAMES = [
    "CLOSE QUARTERS", "PRESSURE BOX", "ZERO ROOM", "TIGHT CELL",
    "CRUSH CHAMBER", "AIRLESS VAULT", "SEALED CAVITY", "BONE PROXIMITY",
    "DEAD POCKET", "WALL PRESS", "MICRO ROOM", "SKIN CONTACT",
    "BREATH TRAP", "IRON LUNG", "PINHOLE WORLD", "COFFIN DRY",
    "CELL STASIS", "CONTACT MIC", "NO EXIT", "SQUEEZE POINT",
    "ANECHOIC HOLD", "SUBDERMAL", "ZERO DECAY", "COMPACT TERROR",
    "THROAT CLOSE", "DENSE CORE", "FIBER CAGE", "NERVE THREAD",
    "PACKED VESSEL", "CLAUSTRAL", "INNER WALL", "CHEST CAVITY",
    "BARE CONCRETE", "SILENT MASS", "HOLLOW BONE", "PACKED VOID",
    "CLENCH STATE", "BODILY HOLD", "COLD PRESS", "WEIGHT ROOM"
]

XHIGH_NAMES = [
    "INFINITE FIELD", "COSMIC VOID", "STELLAR DRIFT", "NEBULA BREATH",
    "DARK MATTER HAZE", "GALACTIC BLOOM", "SOLAR EXHALE", "EVENT HORIZON",
    "INTERSTELLAR WASH", "ORBITAL DECAY", "DEEP SPACE PULSE", "VOID CURRENT",
    "AURORA CASCADE", "IONOSPHERE", "PULSAR ECHO", "RADIANT EXPANSE",
    "COSMOS ADRIFT", "SUPERNOVA TAIL", "STAR NURSERY", "QUANTUM ETHER",
    "LIGHT YEAR", "MAGNETON FIELD", "EXOSPHERE", "CELESTIAL DRAIN",
    "HELIOSPHERE", "COLD VACUUM", "VAST SILENCE", "ASTRAL FLOAT",
    "GRAVITY WELL", "HYPERLANE", "COMET TRAIL", "NEBULAR TIDE",
    "RADIO TELESCOPE", "FLUX CLOUD", "FROZEN ORBIT", "ANTIMATTER BLOOM",
    "PHOTON STREAM", "QUASAR BREATH", "DISTANT SIGNAL", "COSMIC MEMBRANE"
]


def r(lo, hi):
    """Random float between lo and hi, rounded to 3 decimal places."""
    return round(random.uniform(lo, hi), 3)


def varied_non_space():
    """Return brightness/warmth/movement/density/aggression with intentional spread."""
    dims = []
    for _ in range(5):
        zone = random.choice(["low", "mid_low", "mid_high", "high"])
        if zone == "low":
            dims.append(r(0.05, 0.25))
        elif zone == "mid_low":
            dims.append(r(0.25, 0.45))
        elif zone == "mid_high":
            dims.append(r(0.55, 0.75))
        else:
            dims.append(r(0.75, 0.99))
    return dims


def engine_params(space_val):
    """Generate per-engine macro parameters with given space value."""
    return {
        "macro_character": r(0.05, 0.99),
        "macro_movement": r(0.05, 0.99),
        "macro_coupling": r(0.05, 0.99),
        "macro_space": round(space_val, 3)
    }


def build_preset(name, eng1, eng2, space_val, coupling_type, tags):
    # Slightly vary engine-level space around the target
    spread = 0.03
    sp1 = round(max(0.01, min(0.99, space_val + random.uniform(-spread, spread))), 3)
    sp2 = round(max(0.01, min(0.99, space_val + random.uniform(-spread, spread))), 3)

    b, w, mv, d, ag = varied_non_space()

    source, target = (eng1, eng2) if random.random() > 0.5 else (eng2, eng1)

    return {
        "name": name,
        "version": "1.0",
        "mood": "Entangled",
        "engines": [eng1, eng2],
        "parameters": {
            eng1: engine_params(sp1),
            eng2: engine_params(sp2)
        },
        "coupling": {
            "type": coupling_type,
            "source": source,
            "target": target,
            "amount": r(0.3, 0.95)
        },
        "dna": {
            "brightness": b,
            "warmth": w,
            "movement": mv,
            "density": d,
            "space": round(space_val, 3),
            "aggression": ag
        },
        "macros": {
            "CHARACTER": r(0.05, 0.99),
            "MOVEMENT": r(0.05, 0.99),
            "COUPLING": r(0.05, 0.99),
            "SPACE": round(space_val, 3)
        },
        "tags": tags
    }


def main():
    random.seed(42)
    os.makedirs(OUTPUT_DIR, exist_ok=True)

    # Build engine pairs — cycle through all 34 engines ensuring good coverage
    def engine_pairs(count):
        pairs = []
        engines = ENGINES[:]
        random.shuffle(engines)
        idx = 0
        used = set()
        while len(pairs) < count:
            e1 = engines[idx % len(engines)]
            idx += 1
            # Pick e2 different from e1, prefer not recently used together
            candidates = [e for e in engines if e != e1]
            random.shuffle(candidates)
            e2 = candidates[0]
            pairs.append((e1, e2))
        return pairs

    xlow_pairs = engine_pairs(40)
    xhigh_pairs = engine_pairs(40)

    # Distribute coupling types evenly (12 types across 80 presets)
    couplings_all = (COUPLING_TYPES * 7)[:80]
    random.shuffle(couplings_all)
    xlow_couplings = couplings_all[:40]
    xhigh_couplings = couplings_all[40:]

    written = 0
    skipped = 0

    # --- XLOW batch (space 0.02–0.10) ---
    for i in range(40):
        name = XLOW_NAMES[i]
        eng1, eng2 = xlow_pairs[i]
        space_val = r(0.02, 0.10)
        coupling = xlow_couplings[i]
        tags = ["entangled", "claustrophobic", "intimate", "xlow"]

        preset = build_preset(name, eng1, eng2, space_val, coupling, tags)
        slug = name.replace(" ", "_").upper()
        filepath = os.path.join(OUTPUT_DIR, f"{slug}.xometa")

        if os.path.exists(filepath):
            skipped += 1
            continue

        with open(filepath, "w") as f:
            json.dump(preset, f, indent=2)
        written += 1

    # --- XHIGH batch (space 0.90–0.99) ---
    for i in range(40):
        name = XHIGH_NAMES[i]
        eng1, eng2 = xhigh_pairs[i]
        space_val = r(0.90, 0.99)
        coupling = xhigh_couplings[i]
        tags = ["entangled", "vast", "cosmic", "xhigh"]

        preset = build_preset(name, eng1, eng2, space_val, coupling, tags)
        slug = name.replace(" ", "_").upper()
        filepath = os.path.join(OUTPUT_DIR, f"{slug}.xometa")

        if os.path.exists(filepath):
            skipped += 1
            continue

        with open(filepath, "w") as f:
            json.dump(preset, f, indent=2)
        written += 1

    print(f"Done. Written: {written} | Skipped (already exist): {skipped}")
    print(f"Output directory: {os.path.abspath(OUTPUT_DIR)}")


if __name__ == "__main__":
    main()
