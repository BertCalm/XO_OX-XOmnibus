#!/usr/bin/env python3
"""
xpn_overbite_gap_closure_pack.py

Scans Presets/XOmnibus/Entangled/ for existing OVERBITE pairs,
identifies uncovered partners from the target list, and generates
2 presets per missing partner.

OVERBITE character: Fang White #F0EDE8, bass-forward character synth,
B008 Five-Macro System (BELLY/BITE/SCURRY/TRASH/PLAY DEAD), poss_ prefix.
"""

import json
import os
import sys

ENTANGLED_DIR = os.path.join(
    os.path.dirname(__file__), "..", "Presets", "XOmnibus", "Entangled"
)

TARGET_PARTNERS = [
    "ORGANON", "OUROBOROS", "ORIGAMI", "ORACLE", "OBSCURA",
    "OCEANIC", "OCELOT", "OPTIC", "OBLIQUE", "OSPREY",
    "OSTERIA", "OWLFISH", "OHM", "ORPHICA", "OBBLIGATO",
    "OTTONI", "OLE", "OMBRE", "ORCA", "OCTOPUS",
    "OVERLAP", "OUTWIT",
]

COUPLING_TYPES = [
    "FREQUENCY_SHIFT", "AMPLITUDE_MOD", "FILTER_MOD", "PITCH_SYNC",
    "TIMBRE_BLEND", "ENVELOPE_LINK", "HARMONIC_FOLD", "CHAOS_INJECT",
    "RESONANCE_SHARE", "SPATIAL_COUPLE", "SPECTRAL_MORPH", "VELOCITY_COUPLE",
]

# Per-partner preset definitions: (name, coupling_type, overbite_params, partner_params, dna, macros, tags_extra)
PRESET_DATA = {
    "ORIGAMI": [
        {
            "name": "Crease Predator",
            "coupling": "SPECTRAL_MORPH",
            "ob_params": {"macro_character": 0.85, "macro_movement": 0.3, "macro_coupling": 0.7, "macro_space": 0.2},
            "pt_params": {"macro_character": 0.4, "macro_movement": 0.75, "macro_coupling": 0.65, "macro_space": 0.5},
            "dna": {"brightness": 0.15, "warmth": 0.8, "movement": 0.55, "density": 0.9, "space": 0.2, "aggression": 0.75},
            "macros": {"CHARACTER": 0.85, "MOVEMENT": 0.3, "COUPLING": 0.7, "SPACE": 0.2},
            "tags": ["entangled", "overbite", "origami", "bass", "fold", "spectral"],
            "coupling_amount": 0.72,
            "coupling_source": "OVERBITE",
        },
        {
            "name": "Paper Fang",
            "coupling": "ENVELOPE_LINK",
            "ob_params": {"macro_character": 0.6, "macro_movement": 0.65, "macro_coupling": 0.8, "macro_space": 0.35},
            "pt_params": {"macro_character": 0.55, "macro_movement": 0.5, "macro_coupling": 0.75, "macro_space": 0.45},
            "dna": {"brightness": 0.25, "warmth": 0.7, "movement": 0.65, "density": 0.82, "space": 0.3, "aggression": 0.6},
            "macros": {"CHARACTER": 0.6, "MOVEMENT": 0.65, "COUPLING": 0.8, "SPACE": 0.35},
            "tags": ["entangled", "overbite", "origami", "bass", "envelope", "texture"],
            "coupling_amount": 0.65,
            "coupling_source": "ORIGAMI",
        },
    ],
    "OBSCURA": [
        {
            "name": "Dark Jaw",
            "coupling": "CHAOS_INJECT",
            "ob_params": {"macro_character": 0.9, "macro_movement": 0.45, "macro_coupling": 0.8, "macro_space": 0.15},
            "pt_params": {"macro_character": 0.7, "macro_movement": 0.6, "macro_coupling": 0.7, "macro_space": 0.25},
            "dna": {"brightness": 0.1, "warmth": 0.85, "movement": 0.5, "density": 0.95, "space": 0.15, "aggression": 0.9},
            "macros": {"CHARACTER": 0.9, "MOVEMENT": 0.45, "COUPLING": 0.8, "SPACE": 0.15},
            "tags": ["entangled", "overbite", "obscura", "bass", "chaos", "dark", "dense"],
            "coupling_amount": 0.85,
            "coupling_source": "OVERBITE",
        },
        {
            "name": "Murk Bite",
            "coupling": "HARMONIC_FOLD",
            "ob_params": {"macro_character": 0.75, "macro_movement": 0.55, "macro_coupling": 0.65, "macro_space": 0.3},
            "pt_params": {"macro_character": 0.6, "macro_movement": 0.7, "macro_coupling": 0.6, "macro_space": 0.4},
            "dna": {"brightness": 0.2, "warmth": 0.75, "movement": 0.6, "density": 0.88, "space": 0.25, "aggression": 0.72},
            "macros": {"CHARACTER": 0.75, "MOVEMENT": 0.55, "COUPLING": 0.65, "SPACE": 0.3},
            "tags": ["entangled", "overbite", "obscura", "bass", "harmonic", "murk"],
            "coupling_amount": 0.68,
            "coupling_source": "OBSCURA",
        },
    ],
}


def scan_covered_partners(entangled_dir: str) -> set:
    covered = set()
    for fname in os.listdir(entangled_dir):
        if not fname.endswith(".xometa"):
            continue
        path = os.path.join(entangled_dir, fname)
        try:
            with open(path) as f:
                d = json.load(f)
            engines = [e.upper() for e in d.get("engines", [])]
            if "OVERBITE" in engines:
                for e in engines:
                    if e != "OVERBITE":
                        covered.add(e)
        except Exception:
            pass
    return covered


def build_preset(partner: str, preset_def: dict) -> dict:
    ob_params = preset_def["ob_params"]
    pt_params = preset_def["pt_params"]
    return {
        "name": preset_def["name"],
        "version": "1.0",
        "mood": "Entangled",
        "engines": ["OVERBITE", partner],
        "parameters": {
            "OVERBITE": ob_params,
            partner: pt_params,
        },
        "coupling": {
            "type": preset_def["coupling"],
            "source": preset_def["coupling_source"],
            "target": partner if preset_def["coupling_source"] == "OVERBITE" else "OVERBITE",
            "amount": preset_def["coupling_amount"],
        },
        "dna": preset_def["dna"],
        "macros": preset_def["macros"],
        "tags": preset_def["tags"],
    }


def safe_filename(name: str) -> str:
    return name.replace("/", "_").replace("\\", "_").replace(":", "_")


def main():
    entangled_dir = os.path.realpath(ENTANGLED_DIR)
    if not os.path.isdir(entangled_dir):
        print(f"ERROR: Entangled directory not found: {entangled_dir}")
        sys.exit(1)

    covered = scan_covered_partners(entangled_dir)
    missing = [p for p in TARGET_PARTNERS if p not in covered]

    print(f"Covered OVERBITE partners ({len(covered)}): {sorted(covered)}")
    print(f"Missing partners ({len(missing)}): {missing}")

    if not missing:
        print("All target partners are already covered. Nothing to generate.")
        return

    generated = []
    skipped = []

    for partner in missing:
        presets = PRESET_DATA.get(partner)
        if not presets:
            print(f"WARNING: No preset data defined for {partner} — skipping.")
            continue

        for preset_def in presets:
            preset = build_preset(partner, preset_def)
            fname = f"{safe_filename(preset['name'])}.xometa"
            fpath = os.path.join(entangled_dir, fname)

            if os.path.exists(fpath):
                print(f"  SKIP (exists): {fname}")
                skipped.append(fname)
                continue

            with open(fpath, "w") as f:
                json.dump(preset, f, indent=2)
            print(f"  WROTE: {fname}")
            generated.append(fname)

    print(f"\nDone. Generated: {len(generated)}, Skipped: {len(skipped)}")
    for g in generated:
        print(f"  + {g}")


if __name__ == "__main__":
    main()
