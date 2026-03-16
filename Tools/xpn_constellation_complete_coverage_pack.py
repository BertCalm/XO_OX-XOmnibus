#!/usr/bin/env python3
"""Generate .xometa coupling presets completing coverage for OHM and OBBLIGATO.

Covers new pairs only — ones not produced by prior constellation packs:

  OHM × ODDFELIX, ODDOSCAR, OVERDUB, ODYSSEY, OBLONG, OBESE, ONSET,
         OVERWORLD, OPAL, ORBITAL, ORGANON, OUROBOROS, OBSIDIAN, OVERBITE,
         ORIGAMI, OBSCURA, OCEANIC                         (17 pairs)

  OBBLIGATO × ODDFELIX, ODDOSCAR, OVERDUB, OBLONG, OBESE, ONSET,
               OVERWORLD, OPAL, ORBITAL, ORGANON, OUROBOROS, OBSIDIAN,
               OVERBITE, ORIGAMI, OBSCURA, OCEANIC, OCELOT, OPTIC       (18 pairs)

Total target: 35 pairs × 2 variants = 70 presets.
All files land in Presets/XOmnibus/Entangled/. Existing files are skipped.

Usage:
    python3 Tools/xpn_constellation_complete_coverage_pack.py
    python3 Tools/xpn_constellation_complete_coverage_pack.py --dry-run
    python3 Tools/xpn_constellation_complete_coverage_pack.py --output-dir /tmp/test
"""

import argparse
import json
from pathlib import Path

# ---------------------------------------------------------------------------
# Constellation engine DNA baselines (from brief)
# ---------------------------------------------------------------------------

CONSTELLATION = {
    "OHM": {
        "prefix": "ohm_",
        "label": "hippy",
        "dna": {"brightness": 0.5, "warmth": 0.75, "movement": 0.6,
                "density": 0.5, "space": 0.7, "aggression": 0.2},
    },
    "OBBLIGATO": {
        "prefix": "obbl_",
        "label": "wind",
        "dna": {"brightness": 0.55, "warmth": 0.65, "movement": 0.55,
                "density": 0.65, "space": 0.5, "aggression": 0.35},
    },
}

# ---------------------------------------------------------------------------
# Partner engine DNA baselines
# ---------------------------------------------------------------------------

PARTNERS = {
    "ODDFELIX": {
        "prefix": "snap_",
        "dna": {"brightness": 0.7, "warmth": 0.5, "movement": 0.65,
                "density": 0.5, "space": 0.55, "aggression": 0.45},
    },
    "ODDOSCAR": {
        "prefix": "morph_",
        "dna": {"brightness": 0.45, "warmth": 0.7, "movement": 0.7,
                "density": 0.55, "space": 0.6, "aggression": 0.3},
    },
    "OVERDUB": {
        "prefix": "dub_",
        "dna": {"brightness": 0.35, "warmth": 0.75, "movement": 0.6,
                "density": 0.55, "space": 0.7, "aggression": 0.25},
    },
    "ODYSSEY": {
        "prefix": "drift_",
        "dna": {"brightness": 0.5, "warmth": 0.55, "movement": 0.65,
                "density": 0.5, "space": 0.65, "aggression": 0.3},
    },
    "OBLONG": {
        "prefix": "bob_",
        "dna": {"brightness": 0.6, "warmth": 0.65, "movement": 0.5,
                "density": 0.6, "space": 0.5, "aggression": 0.5},
    },
    "OBESE": {
        "prefix": "fat_",
        "dna": {"brightness": 0.65, "warmth": 0.6, "movement": 0.55,
                "density": 0.75, "space": 0.35, "aggression": 0.7},
    },
    "ONSET": {
        "prefix": "perc_",
        "dna": {"brightness": 0.5, "warmth": 0.55, "movement": 0.7,
                "density": 0.8, "space": 0.35, "aggression": 0.7},
    },
    "OVERWORLD": {
        "prefix": "ow_",
        "dna": {"brightness": 0.7, "warmth": 0.4, "movement": 0.65,
                "density": 0.6, "space": 0.45, "aggression": 0.55},
    },
    "OPAL": {
        "prefix": "opal_",
        "dna": {"brightness": 0.75, "warmth": 0.5, "movement": 0.7,
                "density": 0.45, "space": 0.8, "aggression": 0.2},
    },
    "ORBITAL": {
        "prefix": "orb_",
        "dna": {"brightness": 0.6, "warmth": 0.55, "movement": 0.6,
                "density": 0.55, "space": 0.6, "aggression": 0.45},
    },
    "ORGANON": {
        "prefix": "organon_",
        "dna": {"brightness": 0.5, "warmth": 0.6, "movement": 0.75,
                "density": 0.65, "space": 0.55, "aggression": 0.35},
    },
    "OUROBOROS": {
        "prefix": "ouro_",
        "dna": {"brightness": 0.55, "warmth": 0.45, "movement": 0.8,
                "density": 0.65, "space": 0.5, "aggression": 0.75},
    },
    "OBSIDIAN": {
        "prefix": "obsidian_",
        "dna": {"brightness": 0.25, "warmth": 0.35, "movement": 0.45,
                "density": 0.7, "space": 0.6, "aggression": 0.6},
    },
    "OVERBITE": {
        "prefix": "poss_",
        "dna": {"brightness": 0.55, "warmth": 0.65, "movement": 0.55,
                "density": 0.7, "space": 0.5, "aggression": 0.65},
    },
    "ORIGAMI": {
        "prefix": "origami_",
        "dna": {"brightness": 0.6, "warmth": 0.5, "movement": 0.6,
                "density": 0.55, "space": 0.55, "aggression": 0.4},
    },
    "OBSCURA": {
        "prefix": "obscura_",
        "dna": {"brightness": 0.35, "warmth": 0.45, "movement": 0.5,
                "density": 0.6, "space": 0.7, "aggression": 0.45},
    },
    "OCEANIC": {
        "prefix": "ocean_",
        "dna": {"brightness": 0.55, "warmth": 0.6, "movement": 0.65,
                "density": 0.6, "space": 0.75, "aggression": 0.3},
    },
    "OCELOT": {
        "prefix": "ocelot_",
        "dna": {"brightness": 0.65, "warmth": 0.55, "movement": 0.75,
                "density": 0.55, "space": 0.45, "aggression": 0.7},
    },
    "OPTIC": {
        "prefix": "opti_",
        "dna": {"brightness": 0.85, "warmth": 0.35, "movement": 0.65,
                "density": 0.45, "space": 0.55, "aggression": 0.4},
    },
}

# ---------------------------------------------------------------------------
# Coupling type selection by partner character
# ---------------------------------------------------------------------------

COUPLING_BY_PARTNER = {
    "ODDFELIX":  "HARMONIC_BLEND",
    "ODDOSCAR":  "MORPH_CHAIN",
    "OVERDUB":   "TAPE_ECHO_LOCK",
    "ODYSSEY":   "DRIFT_SYNC",
    "OBLONG":    "RESONANT_BRIDGE",
    "OBESE":     "SATURATION_FEED",
    "ONSET":     "RHYTHM_GATE",
    "OVERWORLD": "ERA_CROSSFADE",
    "OPAL":      "GRANULAR_WEAVE",
    "ORBITAL":   "ORBITAL_RING",
    "ORGANON":   "METABOLIC_SYNC",
    "OUROBOROS": "CHAOS_LEASH",
    "OBSIDIAN":  "CRYSTAL_RESONANCE",
    "OVERBITE":  "BITE_TRIGGER",
    "ORIGAMI":   "FOLD_MODULATE",
    "OBSCURA":   "DAGUERREOTYPE_BLUR",
    "OCEANIC":   "PHOSPHOR_PULSE",
    "OCELOT":    "PREDATOR_STALK",
    "OPTIC":     "SPECTRAL_REFRACTION",
}

# ---------------------------------------------------------------------------
# Preset definitions
# (source_engine, partner, name_a, desc_a, name_b, desc_b, coupling_a, coupling_b)
# variant_a: source → partner; variant_b: partner → source
# ---------------------------------------------------------------------------

PRESET_PAIRS = [
    # ── OHM pairings ────────────────────────────────────────────────────────

    ("OHM", "ODDFELIX",
     "Sage Neon Circle",
     "Warm ohm commune illuminated by neon tetra flash — folk warmth and electric shimmer share the same hearth",
     "Tetra Folk Swim",
     "Neon arc cuts through sage haze — OddfeliX sharp edges softened by ohm's communal gravity",
     0.6, 0.55),

    ("OHM", "ODDOSCAR",
     "Axolotl Commune",
     "Regenerative axolotl morphology breathes through ohm's warm family circle — tender and strange together",
     "Sage Gill Drift",
     "Ohm warmth feeds OddOscar's gill-pink oscillation — folk slow-burn becomes tender mutation",
     0.55, 0.65),

    ("OHM", "OVERDUB",
     "Tape Circle",
     "Warm commune recorded to tape — ohm groove preserved in saturated analogue warmth",
     "Dubbed Sage",
     "Overdub spring reverb wraps ohm drone — folk ritual heard through the haze of a worn cassette",
     0.6, 0.7),

    ("OHM", "ODYSSEY",
     "Drifting Commune",
     "Ohm family gathered then scattered across tonal drift — warmth persists as everything shifts",
     "Sage Voyage",
     "Odyssey modal wandering guided by ohm's gravitational pull — drift with a home to return to",
     0.55, 0.6),

    ("OHM", "OBLONG",
     "Amber Hearth",
     "Oblong amber warmth and ohm sage warmth double-stack — two hearths, one room",
     "Bob's Folk Lesson",
     "Oblong teaches ohm a few bar-chord tricks — character synth meets hippy jam session",
     0.6, 0.5),

    ("OHM", "OBESE",
     "Saturation Circle",
     "Ohm commune runs through fat saturation drive — communal warmth becomes communal fire",
     "Sage Mojo",
     "Obese MOJO axis modulated by ohm's MEDDLING macro — hippy wisdom meets the hot pink machine",
     0.65, 0.55),

    ("OHM", "ONSET",
     "Drum Circle Ritual",
     "Ohm groove finally finds its percussion section — sage drone locked to kick and clap",
     "Rhythmic Commune",
     "Onset rhythms shaped by ohm MEDDLING macro — the drum machine joins the commune",
     0.7, 0.6),

    ("OHM", "OVERWORLD",
     "Chiptune Hearth",
     "ERA triangle crossfades through ohm communal warmth — retro game music finds its soul",
     "Sage Bit",
     "Ohm warmth modulates overworld era selection — folk spirit channels 16-bit nostalgia",
     0.6, 0.65),

    ("OHM", "OPAL",
     "Granular Commune",
     "Opal grain clouds dispersed through ohm's sage warmth — microsound made communal",
     "Folk Grains",
     "Ohm groove granularized — each moment of warmth scattered and suspended in space",
     0.65, 0.75),

    ("OHM", "ORBITAL",
     "Rotating Hearth",
     "Orbital group envelope cycles through ohm commune — the fire rotates, always warm",
     "Sage Ring",
     "Ohm's COMMUNE axis imprinted on orbital ring modulation — shared breathing in orbit",
     0.6, 0.55),

    ("OHM", "ORGANON",
     "Metabolic Folk",
     "Organon's variational free energy metabolism fed by ohm commune warmth — biology meets hippy jam",
     "Breathing Circle",
     "Ohm slow LFO breathes through organon metabolic rate — the commune has a heartbeat",
     0.55, 0.65),

    ("OHM", "OUROBOROS",
     "Chaos Commune",
     "Ouroboros strange attractor leashed to ohm's warm gravitational centre — chaos with a hearth",
     "Serpent Folk",
     "Ohm groove becomes the leash for self-devouring chaos — folk holds the serpent",
     0.7, 0.6),

    ("OHM", "OBSIDIAN",
     "Dark Crystal Hearth",
     "Obsidian crystal white depth filtered through ohm sage warmth — coldness made human",
     "Sage Void",
     "Ohm commune swallowed by obsidian depth — warmth persists as a memory in the crystal",
     0.55, 0.65),

    ("OHM", "OVERBITE",
     "Gentle Fang",
     "Overbite's BITE macro tempered by ohm commune — the possum rests in sage warmth",
     "Sage Scurry",
     "Ohm MEDDLING tames the SCURRY macro — play dead dissolves into folk meditation",
     0.6, 0.55),

    ("OHM", "ORIGAMI",
     "Folded Commune",
     "Origami fold point applied to ohm's melodic arc — warmth creased into geometric beauty",
     "Sage Paper",
     "Ohm groove folded once, twice, three times — communal heat concentrated at the crease",
     0.6, 0.65),

    ("OHM", "OBSCURA",
     "Daguerreotype Folk",
     "Ohm commune captured through obscura daguerreotype process — folk warmth made antique",
     "Sage Stiffness",
     "Obscura string stiffness modulated by ohm's slow communal breathe — photography meets folk",
     0.55, 0.6),

    ("OHM", "OCEANIC",
     "Phosphor Commune",
     "Ohm warmth meeting oceanic phosphorescent pulse — campfire at the edge of a glowing sea",
     "Sage Shore",
     "Oceanic shoreline separation tuned to ohm COMMUNE macro — the sea joins the circle",
     0.65, 0.6),

    # ── OBBLIGATO pairings ───────────────────────────────────────────────────

    ("OBBLIGATO", "ODDFELIX",
     "Reed & Neon",
     "Dual wind breath intertwined with neon tetra arc — obligate weave of air and electricity",
     "Tetra Wind Bond",
     "OddfeliX sharp modulation obligated to OBBLIGATO's breath pattern — neon must follow the reed",
     0.65, 0.55),

    ("OBBLIGATO", "ODDOSCAR",
     "Axolotl Breath",
     "Axolotl's regenerative morph fed through dual wind channel — tender gill-song obligated to air",
     "Coral Gill",
     "OBBLIGATO BOND macro tied to OddOscar morph depth — wind must breathe with the axolotl",
     0.6, 0.65),

    ("OBBLIGATO", "OVERDUB",
     "Dubbed Wind",
     "Dual wind obligated to tape echo returns — every breath echoes back a beat later",
     "Spring Reed",
     "Overdub spring reverb activated by OBBLIGATO breath gate — the spring only splashes when the reed speaks",
     0.65, 0.7),

    ("OBBLIGATO", "OBLONG",
     "Amber Reed",
     "Oblong amber character obligated to OBBLIGATO breath rhythm — amber must breathe with the wind",
     "Bob Needs Air",
     "OBBLIGATO dual wind feeds oblong filter — the bass synth finally has breath",
     0.6, 0.55),

    ("OBBLIGATO", "OBESE",
     "Saturated Breath",
     "Dual wind runs through obese saturation — obligated coral warmth turned molten",
     "Fat Reed",
     "Obese MOJO axis obligated to OBBLIGATO breath envelope — saturation breathes",
     0.65, 0.6),

    ("OBBLIGATO", "ONSET",
     "Breath Gate",
     "Onset percussive hits gated by OBBLIGATO breath pattern — drums speak only when wind allows",
     "Reed Rhythm",
     "OBBLIGATO attack shaped by onset kick pattern — the wind is rhythmically forced",
     0.7, 0.6),

    ("OBBLIGATO", "OVERWORLD",
     "Era Wind",
     "Overworld ERA triangle obligated to dual wind breath — chip music breathes again",
     "Chiptune Reed",
     "OBBLIGATO breath modulates overworld era crossfade — each exhale shifts the era",
     0.6, 0.65),

    ("OBBLIGATO", "OPAL",
     "Grain Breath",
     "Opal grain cloud shaped by OBBLIGATO breath envelope — each breath a new grain cluster",
     "Coral Grain",
     "OBBLIGATO BOND macro tied to opal grain size — the tighter the bond, the smaller the grain",
     0.65, 0.7),

    ("OBBLIGATO", "ORBITAL",
     "Orbital Reed",
     "Orbital group envelope obligated to OBBLIGATO breath phase — the orbit breathes in sync",
     "Rotating Wind",
     "OBBLIGATO dual-channel breath mapped to orbital ring rotation — wind becomes orbit",
     0.6, 0.55),

    ("OBBLIGATO", "ORGANON",
     "Metabolic Breath",
     "Organon metabolic rate obligated to OBBLIGATO breath tempo — biology must keep pace with the reed",
     "Breathing Organ",
     "OBBLIGATO BOND macro tied to organon metabolic variational energy — wind feeds the metabolism",
     0.55, 0.65),

    ("OBBLIGATO", "OUROBOROS",
     "Obligate Serpent",
     "Ouroboros chaos leash implemented as OBBLIGATO breath gating — chaos breathes on cue",
     "Serpent Reed",
     "OBBLIGATO dual wind tone obligated to ouroboros topology — the reed bends to the serpent's geometry",
     0.7, 0.6),

    ("OBBLIGATO", "OBSIDIAN",
     "Crystal Wind",
     "Dual wind breath filtered through obsidian crystal depth — warm coral turned cold clarity",
     "Dark Reed",
     "OBBLIGATO breath gate opens into obsidian void — obligated warmth swallowed by crystal silence",
     0.6, 0.65),

    ("OBBLIGATO", "OVERBITE",
     "Reed & Fang",
     "Overbite BITE macro obligated to OBBLIGATO breath attack — the possum bites on the downbeat",
     "Coral Trash",
     "OBBLIGATO BOND macro linked to overbite TRASH macro — necessary connection, chaotic content",
     0.65, 0.55),

    ("OBBLIGATO", "ORIGAMI",
     "Folded Reed",
     "Origami fold point obligated to OBBLIGATO breath phase — the paper folds when the wind blows",
     "Coral Crease",
     "OBBLIGATO dual-wind channel mapped to origami fold/unfold cycle — breath becomes geometry",
     0.6, 0.65),

    ("OBBLIGATO", "OBSCURA",
     "Photographic Breath",
     "Obscura daguerreotype stiffness obligated to OBBLIGATO breath — each breath exposes the plate",
     "Coral Blur",
     "OBBLIGATO BOND macro tied to obscura blur depth — the tighter the bond, the sharper the image",
     0.55, 0.6),

    ("OBBLIGATO", "OCEANIC",
     "Shore Breath",
     "Oceanic shoreline separation obligated to OBBLIGATO breath rhythm — tide follows the reed",
     "Coral Shore",
     "OBBLIGATO dual-wind tones mapped to oceanic phosphor pulse — breath illuminates the deep",
     0.65, 0.6),

    ("OBBLIGATO", "OCELOT",
     "Predator Breath",
     "Ocelot stalk rhythm obligated to OBBLIGATO breath gate — the cat moves only when the wind allows",
     "Coral Pounce",
     "OBBLIGATO BOND macro triggers ocelot pounce envelope — breath obligates the ambush",
     0.65, 0.7),

    ("OBBLIGATO", "OPTIC",
     "Optical Reed",
     "Optic AutoPulse rate obligated to OBBLIGATO breath tempo — light pulses to the reed's rhythm",
     "Coral Prism",
     "OBBLIGATO dual-wind spectrum refracted through optic prism — air becomes light spectrum",
     0.6, 0.7),
]

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def blend_dna(dna_a, dna_b, weight_a=0.55):
    """Blend two DNA dicts weighted toward source engine."""
    w_b = 1.0 - weight_a
    return {k: round(dna_a[k] * weight_a + dna_b[k] * w_b, 3) for k in dna_a}


def macro_defaults(engine_name, partner_name):
    """Return sensible macro defaults based on engine and partner character."""
    base = {"CHARACTER": 0.5, "MOVEMENT": 0.5, "COUPLING": 0.65, "SPACE": 0.5}

    if engine_name == "OHM":
        base.update({"CHARACTER": 0.6, "MOVEMENT": 0.55, "SPACE": 0.65})
    elif engine_name == "OBBLIGATO":
        base.update({"CHARACTER": 0.55, "MOVEMENT": 0.55, "SPACE": 0.5})

    # Partner tweaks
    if partner_name in ("ONSET", "OBESE", "OUROBOROS"):
        base["CHARACTER"] = round(min(1.0, base["CHARACTER"] + 0.1), 2)
    if partner_name in ("OPAL", "OCEANIC", "OBSCURA", "OBSIDIAN"):
        base["SPACE"] = round(min(1.0, base["SPACE"] + 0.1), 2)
    if partner_name in ("ODDFELIX", "ODYSSEY", "OVERWORLD", "ORGANON"):
        base["MOVEMENT"] = round(min(1.0, base["MOVEMENT"] + 0.05), 2)

    return base


def build_preset(name, engine_name, partner_name, description, coupling_amount, variant_index):
    """Build a single .xometa preset dict."""
    eng_dna = CONSTELLATION[engine_name]["dna"]
    part_dna = PARTNERS[partner_name]["dna"]
    blended = blend_dna(eng_dna, part_dna, weight_a=0.6)

    macros = macro_defaults(engine_name, partner_name)
    macros["COUPLING"] = round(coupling_amount, 2)

    if variant_index == 0:
        source, target = engine_name, partner_name
    else:
        source, target = partner_name, engine_name

    label = CONSTELLATION[engine_name]["label"]

    return {
        "name": name,
        "version": "1.0",
        "engines": [engine_name, partner_name],
        "mood": "Entangled",
        "macros": macros,
        "sonic_dna": blended,
        "parameters": {},
        "coupling": {
            "type": COUPLING_BY_PARTNER[partner_name],
            "source": source,
            "target": target,
            "amount": coupling_amount,
        },
        "tags": [
            "entangled", "coupling", "constellation",
            engine_name.lower(), partner_name.lower(), label,
        ],
        "description": description,
    }


def safe_filename(name):
    """Convert preset name to a safe .xometa filename."""
    safe = "".join(c if c.isalnum() or c in " _-" else "_" for c in name)
    return safe.strip() + ".xometa"


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main():
    parser = argparse.ArgumentParser(
        description="Generate OHM + OBBLIGATO complete-coverage coupling presets"
    )
    parser.add_argument("--dry-run", action="store_true",
                        help="Print filenames without writing files")
    parser.add_argument(
        "--output-dir",
        default=str(Path(__file__).parent.parent / "Presets" / "XOmnibus" / "Entangled"),
        help="Output directory for .xometa files (default: Presets/XOmnibus/Entangled/)",
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
