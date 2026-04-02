#!/usr/bin/env python3
"""Generate .xometa coupling presets for the 5 Constellation engines × 6 target engines.

Covers new external pairings not yet in previous coupling packs:
  OHM, ORPHICA, OTTONI, OLE  ×  OCELOT, OPTIC, OBLIQUE, ORCA, OCTOPUS, OMBRE

Note: OBBLIGATO × these engines already covered in prior packs (Entangled dir).

Total target: 48 presets (4 engines × 6 partners × 2 preset variants each)

Usage:
    python3 Tools/xpn_constellation_external_coupling_pack.py
    python3 Tools/xpn_constellation_external_coupling_pack.py --dry-run
    python3 Tools/xpn_constellation_external_coupling_pack.py --output-dir /tmp/test
"""

import argparse
import json
import os
from pathlib import Path

# ---------------------------------------------------------------------------
# Engine DNA baselines (per brief)
# ---------------------------------------------------------------------------

ENGINES = {
    "OHM": {
        "prefix": "ohm_",
        "label": "Hippy Dad jam",
        "color": "Sage green",
        "dna": {"brightness": 0.5, "warmth": 0.75, "movement": 0.6,
                "density": 0.5, "space": 0.7, "aggression": 0.2},
    },
    "ORPHICA": {
        "prefix": "orph_",
        "label": "Microsound harp",
        "color": "Siren Seafoam",
        "dna": {"brightness": 0.7, "warmth": 0.5, "movement": 0.8,
                "density": 0.4, "space": 0.6, "aggression": 0.15},
    },
    "OBBLIGATO": {
        "prefix": "obbl_",
        "label": "Dual wind",
        "color": "Seafoam Wind",
        "dna": {"brightness": 0.55, "warmth": 0.65, "movement": 0.6,
                "density": 0.65, "space": 0.6, "aggression": 0.45},
    },
    "OTTONI": {
        "prefix": "otto_",
        "label": "Triple brass",
        "color": "Patina",
        "dna": {"brightness": 0.65, "warmth": 0.6, "movement": 0.5,
                "density": 0.7, "space": 0.5, "aggression": 0.6},
    },
    "OLE": {
        "prefix": "ole_",
        "label": "Afro-Latin trio",
        "color": "Hibiscus",
        "dna": {"brightness": 0.7, "warmth": 0.7, "movement": 0.85,
                "density": 0.6, "space": 0.5, "aggression": 0.5},
    },
    # Partner engines
    "OCELOT": {
        "prefix": "ocel_",
        "label": "Feline predator",
        "color": "Ochre",
        "dna": {"brightness": 0.65, "warmth": 0.55, "movement": 0.75,
                "density": 0.55, "space": 0.45, "aggression": 0.7},
    },
    "OPTIC": {
        "prefix": "opti_",
        "label": "Visual signal",
        "color": "Prism Violet",
        "dna": {"brightness": 0.85, "warmth": 0.35, "movement": 0.65,
                "density": 0.45, "space": 0.55, "aggression": 0.4},
    },
    "OBLIQUE": {
        "prefix": "oblq_",
        "label": "Slant strategy",
        "color": "Ashen Slate",
        "dna": {"brightness": 0.45, "warmth": 0.5, "movement": 0.55,
                "density": 0.6, "space": 0.65, "aggression": 0.5},
    },
    "ORCA": {
        "prefix": "orca_",
        "label": "Apex predator pod",
        "color": "Orca White",
        "dna": {"brightness": 0.6, "warmth": 0.45, "movement": 0.7,
                "density": 0.65, "space": 0.6, "aggression": 0.75},
    },
    "OCTOPUS": {
        "prefix": "octp_",
        "label": "Eight-arm intelligence",
        "color": "Deep Ink",
        "dna": {"brightness": 0.5, "warmth": 0.55, "movement": 0.8,
                "density": 0.7, "space": 0.5, "aggression": 0.55},
    },
    "OMBRE": {
        "prefix": "ombr_",
        "label": "Gradient fade",
        "color": "Dusk Gradient",
        "dna": {"brightness": 0.6, "warmth": 0.6, "movement": 0.5,
                "density": 0.5, "space": 0.75, "aggression": 0.25},
    },
}

# ---------------------------------------------------------------------------
# Coupling types by partner character
# ---------------------------------------------------------------------------

COUPLING_TYPES = {
    "OCELOT": "PREDATOR_STALK",
    "OPTIC": "SPECTRAL_REFRACTION",
    "OBLIQUE": "ANGULAR_DEFLECT",
    "ORCA": "ECHOLOCATION_SYNC",
    "OCTOPUS": "NEURAL_WEAVE",
    "OMBRE": "GRADIENT_DISSOLVE",
}

# ---------------------------------------------------------------------------
# Preset definitions: (name, variant_a_desc, variant_b_desc, coupling_amount,
#                      mood_shift, macro_overrides_a, macro_overrides_b)
# Each 6-pack of pairs: 2 presets per pair = 12 per Constellation engine
# ---------------------------------------------------------------------------

# (source_engine, partner, name_a, desc_a, name_b, desc_b, coupling_a, coupling_b)
PRESET_PAIRS = [
    # OHM pairings
    ("OHM", "OCELOT",
     "Sage Pounce",
     "Warm folk groove ambushed by a sudden feline leap — tension coils then releases",
     "Grove Stalk",
     "Patient ohm drone held in readiness while ocelot circles at the forest edge",
     0.65, 0.5),
    ("OHM", "OPTIC",
     "Prism Commune",
     "Sage warmth refracts through a prismatic lens — folk harmonics split to spectral rays",
     "Photon Circle",
     "Communal chant processed through optical shimmer — shared breath caught in light",
     0.6, 0.7),
    ("OHM", "OBLIQUE",
     "Slant Hearth",
     "A warm fire seen at an angle — folk melody deflected into unexpected intervals",
     "Diagonal Supper",
     "Ohm family table set askew — familiar warmth arriving from an oblique direction",
     0.55, 0.45),
    ("OHM", "ORCA",
     "Pod Migration",
     "Sage groove carried on deep tidal current — orca pod navigates by ohm resonance",
     "Breach Bloom",
     "A moment of explosive joy where folk warmth erupts through cold ocean surface",
     0.7, 0.6),
    ("OHM", "OCTOPUS",
     "Eight Hands Jam",
     "Ohm rhythm section finds eight unexpected limbs — communal groove goes multi-armed",
     "Ink Circle",
     "Cloud of dark neural complexity envelops sage warmth — obscured but not lost",
     0.6, 0.75),
    ("OHM", "OMBRE",
     "Fading Embers",
     "Warm ohm communal fire slowly fades to gradient dusk — beauty in diminishment",
     "Sage Dissolve",
     "Folk wholeness softens into ombre blur — unity becoming gradient becoming mist",
     0.5, 0.65),

    # ORPHICA pairings
    ("ORPHICA", "OCELOT",
     "Harp & Claw",
     "Gossamer string plucks interrupted by sudden predatory stabs — beauty and danger",
     "Feline Resonance",
     "Ocelot stalks through a field of ringing crystal harmonics — each step a note",
     0.7, 0.55),
    ("ORPHICA", "OPTIC",
     "Spectrum Weave",
     "Microsound harp threads through prismatic refraction — each grain a different hue",
     "Crystal Signal",
     "High-frequency orphica overtones beamed through optical array — pure transmission",
     0.75, 0.8),
    ("ORPHICA", "OBLIQUE",
     "Slant Shimmer",
     "Harp glissando deflected mid-arc — oblique strategy reshapes the melodic curve",
     "Angled Grain",
     "Microsound particles scattered at oblique angles — cloud of deflected shimmer",
     0.6, 0.5),
    ("ORPHICA", "ORCA",
     "Song Breach",
     "Orphica microsong and orca click-train merge — two ancient sonic languages",
     "Echoic Harp",
     "Siren calls bounce back as echolocation — the ocean replies in grain-shimmer",
     0.65, 0.7),
    ("ORPHICA", "OCTOPUS",
     "Arm & String",
     "Eight tentacles drawn across harp strings simultaneously — impossible ensemble",
     "Neural Pluck",
     "Orphica grains processed through octopus neural net — distributed shimmer",
     0.7, 0.65),
    ("ORPHICA", "OMBRE",
     "Gradient Shimmer",
     "Crystal brightness softens through ombre gradient — spectral fade to warmth",
     "Dawn Dissolve",
     "Microsound dawn slowly dissolves into dusk palette — time-lapse of a day",
     0.55, 0.6),

    # OTTONI pairings
    ("OTTONI", "OCELOT",
     "Brass Ambush",
     "Triple brass fanfare interrupted by sudden ocelot pounce — stately meeting feral",
     "Hunt Fanfare",
     "Patina brass announces the hunt — ocelot responds with low menacing harmonic",
     0.65, 0.7),
    ("OTTONI", "OPTIC",
     "Refractive Brass",
     "Dense brass chord refracted through optical prism — harmonic partials separated",
     "Prismatic Fanfare",
     "Fanfare hits optical array and shatters into spectral components — glory dispersed",
     0.6, 0.75),
    ("OTTONI", "OBLIQUE",
     "Diagonal Brass",
     "Straight fanfare angled obliquely — expected resolution arrives from wrong direction",
     "Slant Patina",
     "Aged brass surface examined at an angle — new facets catch the light",
     0.5, 0.55),
    ("OTTONI", "ORCA",
     "Pod Brass",
     "Orca pod vocalizations harmonized by triple brass — cetacean choir with horns",
     "Breach Fanfare",
     "The orca breach is answered by brass flourish — ocean triumph declared in patina",
     0.7, 0.65),
    ("OTTONI", "OCTOPUS",
     "Eight-Bell Tower",
     "Ottoni bells multiplied across eight arms — impossible carillon of distributed brass",
     "Ink Fanfare",
     "Brass declaration shrouded in octopus ink cloud — triumph gone mysterious",
     0.65, 0.6),
    ("OTTONI", "OMBRE",
     "Patina Fade",
     "Aged brass patina slowly bleeds into gradient haze — history dissolving to gradient",
     "Brass Dusk",
     "Triple horn sunset — three voices converging to a single gradient horizon line",
     0.55, 0.7),

    # OLE pairings
    ("OLE", "OCELOT",
     "Jungle Drama",
     "Afro-Latin percussion meets the jungle cat — DRAMA macro triggers feline eruption",
     "Clave Stalk",
     "Ocelot moves through clave rhythm — each predatory step precisely on the pulse",
     0.75, 0.6),
    ("OLE", "OPTIC",
     "Prismatic Drama",
     "Ole percussion refracts through optical array — rhythm becomes light spectrum",
     "Festival Light",
     "Afro-Latin festival illuminated by prism beams — each drum hit a color event",
     0.65, 0.7),
    ("OLE", "OBLIQUE",
     "Off-Axis Rhythm",
     "Clave redirected obliquely — familiar African pulse arrives from unexpected angle",
     "Slant Drama",
     "DRAMA macro collides with oblique strategy — passion meets calculation",
     0.6, 0.55),
    ("OLE", "ORCA",
     "Pod Dance",
     "Afro-Latin percussion drives orca migration — drums become echolocation signal",
     "Ocean Carnival",
     "Orca breach timed to carnival rhythm — sea and celebration merge explosively",
     0.7, 0.75),
    ("OLE", "OCTOPUS",
     "Eight Limb Carnival",
     "Ole trio gains five more players — eight-arm groove with impossible polyrhythm",
     "Ink & Clave",
     "Octopus cloud conceals the rhythm — African pulse heard dimly through dark water",
     0.65, 0.7),
    ("OLE", "OMBRE",
     "Carnival Dusk",
     "Afro-Latin celebration winding down — the drums fade through gradient twilight",
     "Hibiscus Dissolve",
     "Hibiscus color bleeds into ombre spectrum — vivid joy softening to memory",
     0.55, 0.6),
]

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def blend_dna(dna_a, dna_b, weight_a=0.55):
    """Blend two DNA dicts, weighted toward source engine."""
    w_b = 1.0 - weight_a
    return {k: round(dna_a[k] * weight_a + dna_b[k] * w_b, 3) for k in dna_a}


def macro_defaults(engine_name, partner_name):
    """Return sensible macro defaults based on engine character."""
    base = {"CHARACTER": 0.5, "MOVEMENT": 0.5, "COUPLING": 0.65, "SPACE": 0.5}
    # Engine-specific macro tuning
    if engine_name == "OHM":
        base.update({"CHARACTER": 0.6, "MOVEMENT": 0.55, "SPACE": 0.65})
    elif engine_name == "ORPHICA":
        base.update({"CHARACTER": 0.55, "MOVEMENT": 0.7, "SPACE": 0.6})
    elif engine_name == "OTTONI":
        base.update({"CHARACTER": 0.65, "MOVEMENT": 0.45, "SPACE": 0.45})
    elif engine_name == "OLE":
        base.update({"CHARACTER": 0.7, "MOVEMENT": 0.8, "SPACE": 0.45})
    # Partner tweaks
    if partner_name == "OCELOT":
        base["CHARACTER"] = round(base["CHARACTER"] + 0.1, 2)
    elif partner_name == "OPTIC":
        base["SPACE"] = round(base["SPACE"] + 0.1, 2)
    elif partner_name == "ORCA":
        base["MOVEMENT"] = round(base["MOVEMENT"] + 0.1, 2)
    elif partner_name == "OMBRE":
        base["SPACE"] = round(base["SPACE"] + 0.15, 2)
    return {k: min(1.0, v) for k, v in base.items()}


def build_preset(name, engine_name, partner_name, description, coupling_amount, variant_index):
    """Build a single .xometa preset dict."""
    eng = ENGINES[engine_name]
    part = ENGINES[partner_name]
    blended_dna = blend_dna(eng["dna"], part["dna"], weight_a=0.6)
    macros = macro_defaults(engine_name, partner_name)
    macros["COUPLING"] = round(coupling_amount, 2)

    # Direction alternates between variants
    if variant_index == 0:
        source, target = engine_name, partner_name
    else:
        source, target = partner_name, engine_name

    return {
        "name": name,
        "version": "1.0",
        "engines": [engine_name, partner_name],
        "mood": "Entangled",
        "macros": macros,
        "sonic_dna": blended_dna,
        "parameters": {},
        "coupling": {
            "type": COUPLING_TYPES[partner_name],
            "source": source,
            "target": target,
            "amount": coupling_amount,
        },
        "tags": ["entangled", "coupling", "constellation", engine_name.lower(),
                 partner_name.lower(), eng["label"].split()[0].lower()],
        "description": description,
    }


def safe_filename(name):
    """Convert preset name to safe filename."""
    safe = "".join(c if c.isalnum() or c in " _-" else "_" for c in name)
    return safe.strip() + ".xometa"


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main():
    parser = argparse.ArgumentParser(description="Generate Constellation external coupling presets")
    parser.add_argument("--dry-run", action="store_true", help="Print filenames without writing")
    parser.add_argument(
        "--output-dir",
        default=str(Path(__file__).parent.parent / "Presets" / "XOceanus" / "Entangled"),
        help="Output directory for .xometa files",
    )
    args = parser.parse_args()

    out_dir = Path(args.output_dir)
    if not args.dry_run:
        out_dir.mkdir(parents=True, exist_ok=True)

    written = 0
    skipped = 0

    for (engine_name, partner_name,
         name_a, desc_a, name_b, desc_b,
         coupling_a, coupling_b) in PRESET_PAIRS:

        for idx, (name, desc, coupling) in enumerate([
            (name_a, desc_a, coupling_a),
            (name_b, desc_b, coupling_b),
        ]):
            preset = build_preset(name, engine_name, partner_name, desc, coupling, idx)
            filename = safe_filename(name)
            filepath = out_dir / filename

            if filepath.exists():
                print(f"  SKIP  {filename}")
                skipped += 1
                continue

            if args.dry_run:
                print(f"  DRY   {filename}  ({engine_name} × {partner_name})")
                written += 1
            else:
                filepath.write_text(json.dumps(preset, indent=2) + "\n", encoding="utf-8")
                print(f"  WRITE {filename}")
                written += 1

    action = "would write" if args.dry_run else "wrote"
    print(f"\nDone — {action} {written} preset(s), skipped {skipped} existing.")


if __name__ == "__main__":
    main()
