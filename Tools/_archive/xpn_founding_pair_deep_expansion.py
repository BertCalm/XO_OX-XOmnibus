#!/usr/bin/env python3
"""
xpn_founding_pair_deep_expansion.py

Deepens coupling coverage for ODDFELIX (OddfeliX, snap_) and ODDOSCAR (OddOscar, morph_)
— the founding pair and anchor engines of XOceanus. Generates 84 Entangled presets covering
7 partner engines × 2 founding engines × 3 coupling variants each.

Batches:
  ODDFELIX × [OPTIC, OBLIQUE, OCELOT, OSPREY, OSTERIA, OCEANIC, OWLFISH]  = 21 presets
  ODDFELIX × [OHM, ORPHICA, OBBLIGATO, OTTONI, OLE, OVERLAP, OUTWIT]      = 21 presets
  ODDOSCAR × [OPTIC, OBLIQUE, OCELOT, OSPREY, OSTERIA, OCEANIC, OWLFISH]  = 21 presets
  ODDOSCAR × [OHM, ORPHICA, OBBLIGATO, OTTONI, OLE, OVERLAP, OUTWIT]      = 21 presets
Total: 84 presets

3 coupling variants per pair using different coupling types:
  v1 — HARMONIC_BLEND   (tonal integration)
  v2 — Amp->Filter      (dynamic morphology)
  v3 — LFO->Pitch       (rhythmic entanglement)

Skips existing files. Python stdlib only.
"""

import json
import os

PRESET_DIR = os.path.join(
    os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
    "Presets", "XOceanus", "Entangled"
)

# ── DNA baselines ────────────────────────────────────────────────────────────

FELIX_DNA = {
    "brightness": 0.85,
    "warmth": 0.2,
    "movement": 0.6,
    "density": 0.5,
    "space": 0.55,
    "aggression": 0.35,
}

OSCAR_DNA = {
    "brightness": 0.3,
    "warmth": 0.8,
    "movement": 0.5,
    "density": 0.65,
    "space": 0.5,
    "aggression": 0.3,
}

# ── Partner engine DNA offsets (blended toward partner character) ─────────────

PARTNER_DNA_OFFSETS = {
    "Optic":      {"brightness": +0.10, "movement": +0.15, "warmth": -0.05, "space": +0.10},
    "Oblique":    {"brightness": +0.05, "aggression": +0.10, "movement": +0.10, "density": -0.05},
    "Ocelot":     {"warmth": +0.10, "aggression": +0.15, "brightness": -0.05, "density": +0.05},
    "Osprey":     {"space": +0.15, "warmth": +0.05, "movement": -0.05, "density": -0.05},
    "Osteria":    {"warmth": +0.15, "density": +0.10, "brightness": -0.05, "aggression": +0.05},
    "Oceanic":    {"space": +0.10, "warmth": +0.10, "brightness": -0.05, "movement": +0.05},
    "Owlfish":    {"brightness": +0.10, "density": +0.05, "space": +0.05, "warmth": +0.05},
    "Ohm":        {"warmth": +0.10, "movement": +0.05, "aggression": -0.05, "space": +0.05},
    "Orphica":    {"brightness": +0.05, "space": +0.10, "movement": +0.05, "density": -0.05},
    "Obbligato":  {"warmth": +0.05, "movement": +0.10, "density": +0.05, "space": +0.05},
    "Ottoni":     {"warmth": +0.05, "brightness": +0.05, "aggression": +0.10, "density": +0.05},
    "Ole":        {"warmth": +0.10, "movement": +0.15, "aggression": +0.05, "brightness": -0.05},
    "Overlap":    {"space": +0.15, "density": +0.10, "movement": +0.05, "brightness": -0.05},
    "Outwit":     {"movement": +0.15, "aggression": +0.10, "brightness": +0.05, "density": +0.05},
}

# ── Coupling variant specs ────────────────────────────────────────────────────

# Each variant: (coupling_type, amount_felix, amount_oscar, macro_labels_suffix, intensity)
COUPLING_VARIANTS = [
    ("HARMONIC_BLEND", 0.65, 0.60, "BLEND",  "Deep"),
    ("Amp->Filter",    0.55, 0.50, "FILTER", "Moderate"),
    ("LFO->Pitch",     0.70, 0.65, "PITCH",  "Deep"),
]

# ── Per-engine sparse parameter hints ────────────────────────────────────────
# Only distinctive parameters that make each pair feel purposeful.
# The runtime fills gaps from engine defaults; these just anchor the character.

PARTNER_PARAMS = {
    "Optic": {
        "optic_pulseRate": 0.45,
        "optic_pulseDepth": 0.6,
        "optic_lfoRate": 0.3,
        "optic_brightness": 0.8,
        "optic_scanMode": 0,
    },
    "Oblique": {
        "oblq_prismColor": 0.6,
        "oblq_bounceRate": 0.5,
        "oblq_drive": 0.4,
        "oblq_filterCutoff": 3200.0,
        "oblq_lfoDepth": 0.35,
    },
    "Ocelot": {
        "ocelot_biome": 0.5,
        "ocelot_filterCutoff": 2800.0,
        "ocelot_filterReso": 0.4,
        "ocelot_drive": 0.45,
        "ocelot_lfoRate": 0.25,
    },
    "Osprey": {
        "osprey_shoreBlend": 0.5,
        "osprey_filterCutoff": 5000.0,
        "osprey_lfoRate": 0.15,
        "osprey_reverbMix": 0.4,
        "osprey_attack": 0.08,
    },
    "Osteria": {
        "osteria_qBassShore": 0.6,
        "osteria_filterCutoff": 1800.0,
        "osteria_filterReso": 0.35,
        "osteria_drive": 0.5,
        "osteria_lfoDepth": 0.3,
    },
    "Oceanic": {
        "ocean_separation": 0.55,
        "ocean_filterCutoff": 4000.0,
        "ocean_lfoRate": 0.1,
        "ocean_reverbMix": 0.5,
        "ocean_depth": 0.45,
    },
    "Owlfish": {
        "owl_filterCutoff": 3500.0,
        "owl_filterReso": 0.4,
        "owl_drive": 0.35,
        "owl_lfoRate": 0.2,
        "owl_mixturDepth": 0.5,
    },
    "Ohm": {
        "ohm_macroMeddling": 0.5,
        "ohm_filterCutoff": 4500.0,
        "ohm_lfoRate": 0.2,
        "ohm_reverbMix": 0.35,
        "ohm_drive": 0.25,
    },
    "Orphica": {
        "orph_pluckBrightness": 0.65,
        "orph_filterCutoff": 5500.0,
        "orph_lfoRate": 0.18,
        "orph_reverbMix": 0.4,
        "orph_decay": 0.8,
    },
    "Obbligato": {
        "obbl_breathA": 0.55,
        "obbl_filterCutoff": 3000.0,
        "obbl_lfoRate": 0.22,
        "obbl_drive": 0.3,
        "obbl_reverbMix": 0.4,
    },
    "Ottoni": {
        "otto_macroGrow": 0.5,
        "otto_filterCutoff": 2500.0,
        "otto_filterReso": 0.35,
        "otto_drive": 0.5,
        "otto_lfoRate": 0.2,
    },
    "Ole": {
        "ole_macroDrama": 0.6,
        "ole_filterCutoff": 3200.0,
        "ole_lfoRate": 0.3,
        "ole_drive": 0.4,
        "ole_reverbMix": 0.35,
    },
    "Overlap": {
        "olap_fdnMix": 0.6,
        "olap_filterCutoff": 4000.0,
        "olap_lfoRate": 0.12,
        "olap_reverbDecay": 0.7,
        "olap_drive": 0.25,
    },
    "Outwit": {
        "owit_caRule": 30,
        "owit_filterCutoff": 3000.0,
        "owit_lfoRate": 0.35,
        "owit_drive": 0.45,
        "owit_armDepth": 0.55,
    },
}

# ODDFELIX (snap_) sparse params
FELIX_BASE_PARAMS = {
    "snap_oscMode": 1,
    "snap_filterCutoff": 5500.0,
    "snap_filterReso": 0.45,
    "snap_filterEnvDepth": 0.65,
    "snap_decay": 0.18,
    "snap_level": 0.78,
    "snap_snap": 0.55,
    "snap_detune": 8.0,
}

# ODDOSCAR (morph_) sparse params
OSCAR_BASE_PARAMS = {
    "morph_morph": 0.5,
    "morph_filterCutoff": 2200.0,
    "morph_filterReso": 0.35,
    "morph_filterEnvDepth": 0.45,
    "morph_attack": 0.12,
    "morph_decay": 0.6,
    "morph_sustain": 0.55,
    "morph_release": 1.2,
    "morph_level": 0.75,
}

# ── Preset naming tables ──────────────────────────────────────────────────────

# ODDFELIX names: bright, clinical, electric, snap, tetra, neon-themed
FELIX_NAME_TABLE = {
    # (partner, variant_idx) -> preset name
    ("Optic", 0): "Neon Scan Pulse",
    ("Optic", 1): "Phosphor Snap",
    ("Optic", 2): "Electric Beam Lock",

    ("Oblique", 0): "Prism Tetra",
    ("Oblique", 1): "Clinical Bounce",
    ("Oblique", 2): "Neon Refract",

    ("Ocelot", 0): "Snap Safari",
    ("Ocelot", 1): "Tetra Territory",
    ("Ocelot", 2): "Electric Prowl",

    ("Osprey", 0): "Neon Shore Glide",
    ("Osprey", 1): "Snap Coastline",
    ("Osprey", 2): "Clinical Uplift",

    ("Osteria", 0): "Blue Cellar Snap",
    ("Osteria", 1): "Neon Porto Press",
    ("Osteria", 2): "Electric Cask",

    ("Oceanic", 0): "Neon Depth Charge",
    ("Oceanic", 1): "Phosphor Tide",
    ("Oceanic", 2): "Snap Biolume",

    ("Owlfish", 0): "Mixtur Neon",
    ("Owlfish", 1): "Clinical Trautonium",
    ("Owlfish", 2): "Tetra Abyssal",

    ("Ohm", 0): "Neon Commune",
    ("Ohm", 1): "Snap Meddling",
    ("Ohm", 2): "Electric Sage",

    ("Orphica", 0): "Neon Pluck Siren",
    ("Orphica", 1): "Clinical Harp",
    ("Orphica", 2): "Phosphor Siphon",

    ("Obbligato", 0): "Snap Bond Wind",
    ("Obbligato", 1): "Neon Breath Pair",
    ("Obbligato", 2): "Electric Duet",

    ("Ottoni", 0): "Neon Brass Grow",
    ("Ottoni", 1): "Clinical Triplet",
    ("Ottoni", 2): "Snap Patina",

    ("Ole", 0): "Neon Drama Flash",
    ("Ole", 1): "Electric Afro Snap",
    ("Ole", 2): "Tetra Ole Fire",

    ("Overlap", 0): "Neon Knot Flood",
    ("Overlap", 1): "Snap FDN Wash",
    ("Overlap", 2): "Clinical Tangle",

    ("Outwit", 0): "Neon CA Cascade",
    ("Outwit", 1): "Electric Arm Fire",
    ("Outwit", 2): "Snap Wolfram",
}

# ODDOSCAR names: warm, organic, gill, axolotl, soft, morph-themed
OSCAR_NAME_TABLE = {
    ("Optic", 0): "Gill Scan Bloom",
    ("Optic", 1): "Axolotl Pulse",
    ("Optic", 2): "Morph Light Field",

    ("Oblique", 0): "Warm Prism Gill",
    ("Oblique", 1): "Organic Bounce",
    ("Oblique", 2): "Morph Refract Soft",

    ("Ocelot", 0): "Axolotl Biome",
    ("Ocelot", 1): "Gill Territory",
    ("Ocelot", 2): "Morph Stalk",

    ("Osprey", 0): "Warm Shore Drift",
    ("Osprey", 1): "Gill Coastline",
    ("Osprey", 2): "Organic Uplift",

    ("Osteria", 0): "Warm Cellar Morph",
    ("Osteria", 1): "Gill Porto Pour",
    ("Osteria", 2): "Organic Cask",

    ("Oceanic", 0): "Gill Deep Current",
    ("Oceanic", 1): "Axolotl Tide",
    ("Oceanic", 2): "Morph Biolume Soft",

    ("Owlfish", 0): "Warm Mixtur Gill",
    ("Owlfish", 1): "Organic Trautonium",
    ("Owlfish", 2): "Morph Abyssal Glow",

    ("Ohm", 0): "Gill Commune Warm",
    ("Ohm", 1): "Axolotl Meddling",
    ("Ohm", 2): "Morph Sage Soft",

    ("Orphica", 0): "Gill Pluck Bloom",
    ("Orphica", 1): "Warm Harp Morph",
    ("Orphica", 2): "Organic Siphon",

    ("Obbligato", 0): "Gill Bond Wind",
    ("Obbligato", 1): "Warm Breath Pair",
    ("Obbligato", 2): "Morph Duet Soft",

    ("Ottoni", 0): "Axolotl Brass Grow",
    ("Ottoni", 1): "Gill Triplet Warm",
    ("Ottoni", 2): "Morph Patina",

    ("Ole", 0): "Warm Drama Bloom",
    ("Ole", 1): "Gill Afro Morph",
    ("Ole", 2): "Organic Ole Fire",

    ("Overlap", 0): "Gill Knot Flood",
    ("Overlap", 1): "Warm FDN Wash",
    ("Overlap", 2): "Morph Tangle Soft",

    ("Outwit", 0): "Gill CA Bloom",
    ("Outwit", 1): "Axolotl Arm Flow",
    ("Outwit", 2): "Morph Wolfram Soft",
}

# ── Description templates ─────────────────────────────────────────────────────

COUPLING_TYPE_DESCRIPTIONS = {
    "HARMONIC_BLEND": {
        True:  "OddfeliX's neon-bright harmonics dissolve into {partner}'s character — "
               "a clinical-electric tonal union that sharpens both engines into one coherent voice.",
        False: "OddOscar's warm gill-pink morphology absorbs {partner}'s timbre — "
               "organic resonance blending into a single breathing, living texture.",
    },
    "Amp->Filter": {
        True:  "OddfeliX's transient snap drives {partner}'s filter dynamics — "
               "every neon-bright attack sculpts the partner's tonal response in real time.",
        False: "OddOscar's envelope contour shapes {partner}'s filter — "
               "axolotl gill warmth breathing in and out through the partner's resonant body.",
    },
    "LFO->Pitch": {
        True:  "OddfeliX's electric LFO modulates {partner}'s pitch — "
               "tetra-blue oscillation locking the pair into a synchronized rhythmic entanglement.",
        False: "OddOscar's slow organic LFO nudges {partner}'s pitch — "
               "a warm morphological drift that pulls both engines into a shared tidal rhythm.",
    },
}

PARTNER_DESC_FRAGMENTS = {
    "Optic":     "XOptic's visual-pulse modulation",
    "Oblique":   "XOblique's prismatic bounce engine",
    "Ocelot":    "XOcelot's biome-driven filter",
    "Osprey":    "XOsprey's shore-blend resonance",
    "Osteria":   "XOsteria's Porto-wine warmth",
    "Oceanic":   "XOceanic's chromatophore depth",
    "Owlfish":   "XOwlfish's Mixtur-Trautonium oscillator",
    "Ohm":       "XOhm's Meddling macro commune",
    "Orphica":   "XOrphica's siphonophore micro-pluck",
    "Obbligato": "XObbligato's dual-wind bond",
    "Ottoni":    "XOttoni's triple-brass grow",
    "Ole":       "XOlé's Afro-Latin drama",
    "Overlap":   "XOverlap's FDN knot-topology",
    "Outwit":    "XOutwit's 8-arm Wolfram CA",
}

# ── Macro labels ──────────────────────────────────────────────────────────────

def macro_labels(founding_short, partner, coupling_suffix):
    return [founding_short, partner.upper(), coupling_suffix, "SPACE"]


# ── DNA blending ──────────────────────────────────────────────────────────────

def blend_dna(base_dna, partner_offsets, blend_weight=0.35):
    result = {}
    for k, v in base_dna.items():
        offset = partner_offsets.get(k, 0.0)
        raw = v + offset * blend_weight
        result[k] = round(max(0.0, min(1.0, raw)), 3)
    return result


# ── Tags ──────────────────────────────────────────────────────────────────────

COUPLING_TYPE_TAGS = {
    "HARMONIC_BLEND": ["harmonic", "blend", "tonal"],
    "Amp->Filter":    ["dynamic", "filter", "envelope"],
    "LFO->Pitch":     ["lfo", "pitch", "rhythmic"],
}

def build_tags(founding_id, partner, coupling_type):
    tags = (
        ["entangled", "coupling", founding_id.lower().replace(" ", ""), partner.lower()]
        + COUPLING_TYPE_TAGS.get(coupling_type, [])
    )
    return list(dict.fromkeys(tags))  # deduplicate, preserve order


# ── Macro values ──────────────────────────────────────────────────────────────

MACRO_PROFILES = [
    # variant 0 — HARMONIC_BLEND — wide CHARACTER, full COUPLING, spacious
    {"CHARACTER": 0.65, "MOVEMENT": 0.5,  "COUPLING": 0.75, "SPACE": 0.6},
    # variant 1 — Amp->Filter — dynamic, moderate coupling
    {"CHARACTER": 0.5,  "MOVEMENT": 0.65, "COUPLING": 0.55, "SPACE": 0.45},
    # variant 2 — LFO->Pitch — rhythmic, deep coupling
    {"CHARACTER": 0.55, "MOVEMENT": 0.75, "COUPLING": 0.80, "SPACE": 0.5},
]


# ── Preset builder ────────────────────────────────────────────────────────────

def build_preset(founding_engine_id, founding_short, partner,
                 founding_base_params, founding_dna,
                 variant_idx):

    coupling_type, felix_amount, oscar_amount, coupling_suffix, intensity = COUPLING_VARIANTS[variant_idx]
    amount = felix_amount if founding_short == "SNAP" else oscar_amount

    is_felix = founding_short == "SNAP"
    name_table = FELIX_NAME_TABLE if is_felix else OSCAR_NAME_TABLE
    name = name_table[(partner, variant_idx)]

    dna = blend_dna(founding_dna, PARTNER_DNA_OFFSETS.get(partner, {}))

    desc_template = COUPLING_TYPE_DESCRIPTIONS[coupling_type][is_felix]
    description = desc_template.replace("{partner}", PARTNER_DESC_FRAGMENTS.get(partner, partner))

    tags = build_tags(founding_short, partner, coupling_type)
    labels = macro_labels(founding_short, partner, coupling_suffix)
    macros = MACRO_PROFILES[variant_idx]

    params = {
        founding_engine_id: dict(founding_base_params),
        partner: dict(PARTNER_PARAMS.get(partner, {})),
    }

    preset = {
        "schema_version": 1,
        "name": name,
        "mood": "Entangled",
        "engines": [founding_engine_id, partner],
        "author": "XO_OX Designs",
        "version": "1.0.0",
        "description": description,
        "tags": tags,
        "macroLabels": labels,
        "macros": macros,
        "couplingIntensity": intensity,
        "tempo": None,
        "dna": dna,
        "parameters": params,
        "coupling": {
            "pairs": [
                {
                    "engineA": founding_engine_id,
                    "engineB": partner,
                    "type": coupling_type,
                    "amount": round(amount, 3),
                }
            ]
        },
        "sequencer": None,
    }
    return preset


# ── Write helper ──────────────────────────────────────────────────────────────

def write_preset(preset, output_dir):
    filename = preset["name"] + ".xometa"
    filepath = os.path.join(output_dir, filename)
    if os.path.exists(filepath):
        return filepath, False  # skip existing
    with open(filepath, "w") as f:
        json.dump(preset, f, indent=2)
        f.write("\n")
    return filepath, True


# ── Main ──────────────────────────────────────────────────────────────────────

BATCH_A_PARTNERS = ["Optic", "Oblique", "Ocelot", "Osprey", "Osteria", "Oceanic", "Owlfish"]
BATCH_B_PARTNERS = ["Ohm", "Orphica", "Obbligato", "Ottoni", "Ole", "Overlap", "Outwit"]

ALL_PARTNERS = BATCH_A_PARTNERS + BATCH_B_PARTNERS

FOUNDING_ENGINES = [
    # (engine_id_in_preset, short_label, base_params, dna)
    ("OddfeliX", "SNAP",  FELIX_BASE_PARAMS, FELIX_DNA),
    ("OddOscar",  "MORPH", OSCAR_BASE_PARAMS, OSCAR_DNA),
]


def main():
    os.makedirs(PRESET_DIR, exist_ok=True)

    written = 0
    skipped = 0
    written_names = []

    for engine_id, short, base_params, base_dna in FOUNDING_ENGINES:
        for partner in ALL_PARTNERS:
            for v_idx in range(3):
                preset = build_preset(
                    founding_engine_id=engine_id,
                    founding_short=short,
                    partner=partner,
                    founding_base_params=base_params,
                    founding_dna=base_dna,
                    variant_idx=v_idx,
                )
                filepath, was_written = write_preset(preset, PRESET_DIR)
                if was_written:
                    written += 1
                    written_names.append(preset["name"])
                    print(f"  WROTE  {preset['name']}.xometa")
                else:
                    skipped += 1
                    print(f"  SKIP   {preset['name']}.xometa (exists)")

    print()
    print(f"Done. {written} presets written, {skipped} skipped.")
    print(f"Output dir: {PRESET_DIR}")


if __name__ == "__main__":
    main()
