#!/usr/bin/env python3
"""Generate OUTWIT×OVERLAP Entangled coupling preset stubs for XOceanus.

Seeds Entangled coupling coverage for the two newest installed engines:
  - OUTWIT   (Giant Pacific Octopus 8-arm Wolfram CA,  owit_ prefix)
  - OVERLAP  (Lion's Mane jellyfish FDN reverb,        olap_ prefix)

Output: 46 presets total
  -  6 OUTWIT × OVERLAP marquee presets
  - 20 OUTWIT-led presets (paired with 20 partner engines, one each)
  - 20 OVERLAP-led presets (paired with 20 partner engines, one each)

Usage:
    python3 Tools/xpn_outwit_overlap_coupling_pack.py
    python3 Tools/xpn_outwit_overlap_coupling_pack.py --dry-run
    python3 Tools/xpn_outwit_overlap_coupling_pack.py --seed 7 --output-dir /tmp/test
"""

import argparse
import json
import random
from pathlib import Path
from typing import Optional

# ---------------------------------------------------------------------------
# Coupling types (full 12-type repertoire)
# ---------------------------------------------------------------------------
COUPLING_TYPES = [
    "HARMONIC_BLEND",
    "FREQUENCY_MODULATION",
    "AMPLITUDE_MODULATION",
    "FILTER_COUPLING",
    "ENVELOPE_SHARING",
    "SPECTRAL_TRANSFER",
    "RHYTHMIC_SYNC",
    "CHAOS_INJECTION",
    "PITCH_TRACKING",
    "WAVETABLE_MORPH",
    "SPATIAL_BLEND",
    "GRANULAR_EXCHANGE",
]

# ---------------------------------------------------------------------------
# Engine DNA baselines — 6D Sonic DNA
# ---------------------------------------------------------------------------
OUTWIT_DNA = dict(
    brightness=0.60, warmth=0.45, movement=0.80,
    density=0.65,    space=0.55,  aggression=0.50,
)
OVERLAP_DNA = dict(
    brightness=0.65, warmth=0.55, movement=0.60,
    density=0.70,    space=0.85,  aggression=0.25,
)

PARTNER_DNA: dict[str, dict] = {
    "ODDFELIX":   dict(brightness=0.55, warmth=0.60, movement=0.50, density=0.55, space=0.50, aggression=0.45),
    "ODDOSCAR":   dict(brightness=0.50, warmth=0.70, movement=0.55, density=0.60, space=0.55, aggression=0.35),
    "ORCA":       dict(brightness=0.45, warmth=0.40, movement=0.65, density=0.70, space=0.60, aggression=0.75),
    "OCTOPUS":    dict(brightness=0.60, warmth=0.50, movement=0.75, density=0.65, space=0.50, aggression=0.60),
    "OMBRE":      dict(brightness=0.40, warmth=0.65, movement=0.45, density=0.55, space=0.65, aggression=0.30),
    "OHM":        dict(brightness=0.45, warmth=0.75, movement=0.55, density=0.60, space=0.70, aggression=0.30),
    "ORACLE":     dict(brightness=0.55, warmth=0.60, movement=0.50, density=0.60, space=0.70, aggression=0.30),
    "ODYSSEY":    dict(brightness=0.60, warmth=0.65, movement=0.70, density=0.50, space=0.75, aggression=0.25),
    "OPAL":       dict(brightness=0.70, warmth=0.50, movement=0.65, density=0.55, space=0.80, aggression=0.20),
    "ORGANON":    dict(brightness=0.60, warmth=0.55, movement=0.55, density=0.65, space=0.60, aggression=0.30),
    "OUROBOROS":  dict(brightness=0.45, warmth=0.50, movement=0.75, density=0.80, space=0.50, aggression=0.55),
    "ONSET":      dict(brightness=0.50, warmth=0.55, movement=0.70, density=0.80, space=0.35, aggression=0.70),
    "OBLONG":     dict(brightness=0.55, warmth=0.65, movement=0.55, density=0.65, space=0.50, aggression=0.45),
    "OVERDUB":    dict(brightness=0.35, warmth=0.75, movement=0.60, density=0.55, space=0.70, aggression=0.25),
    "OVERWORLD":  dict(brightness=0.70, warmth=0.30, movement=0.60, density=0.45, space=0.35, aggression=0.45),
    "OBBLIGATO":  dict(brightness=0.55, warmth=0.65, movement=0.60, density=0.65, space=0.60, aggression=0.45),
    "ORPHICA":    dict(brightness=0.80, warmth=0.50, movement=0.70, density=0.45, space=0.75, aggression=0.25),
    "OTTONI":     dict(brightness=0.60, warmth=0.60, movement=0.50, density=0.70, space=0.55, aggression=0.60),
    "OLE":        dict(brightness=0.65, warmth=0.70, movement=0.75, density=0.60, space=0.60, aggression=0.55),
}

# ---------------------------------------------------------------------------
# Partner list (20 engines for each lead)
# ---------------------------------------------------------------------------
PARTNER_ENGINES = [
    "OVERLAP", "ODDFELIX", "ODDOSCAR", "ORCA", "OCTOPUS",
    "OMBRE",   "OHM",      "ORACLE",   "ODYSSEY", "OPAL",
    "ORGANON", "OUROBOROS","ONSET",    "OBLONG",  "OVERDUB",
    "OVERWORLD","OBBLIGATO","ORPHICA",  "OTTONI",  "OLE",
]

# OUTWIT-led partners: all 20 from the master list (OVERLAP is included — beyond the
# 6 marquee presets these are distinct named patches with different coupling types)
OUTWIT_PARTNER_ENGINES = list(PARTNER_ENGINES)

# OVERLAP-led partners: substitute OVERLAP slot with OUTWIT (same 20-engine count)
OVERLAP_PARTNER_ENGINES = ["OUTWIT"] + [e for e in PARTNER_ENGINES if e != "OVERLAP"]

# ---------------------------------------------------------------------------
# Evocative name vocabularies
# ---------------------------------------------------------------------------
OUTWIT_ADJECTIVES = [
    "Emergent", "Fractured", "Cellular", "Wolfram", "Branching",
    "Mutant",   "Evolved",   "Patterned","Signaling","Cryptic",
    "Adaptive", "Scattered", "Recursive","Liminal",  "Volatile",
    "Tendriled","Pulsed",    "Woven",    "Spawned",  "Encoded",
]
OUTWIT_NOUNS = [
    "Arms",    "Rule",    "Signal",  "Pattern", "Cell",
    "Frontier","Branch",  "Automata","Grid",    "Swarm",
    "Tide",    "Current", "Map",     "Matrix",  "Colony",
    "Network", "Circuit", "Thread",  "Lattice", "Bloom",
]

OVERLAP_ADJECTIVES = [
    "Diffuse",  "Trailing", "Cascading","Spreading","Blooming",
    "Frond",    "Tangled",  "Dissolving","Webbed",  "Propagating",
    "Drifting", "Unfurling","Floating",  "Hovering","Suspended",
    "Trailing", "Veiling",  "Reflecting","Refracting","Shimmering",
]
OVERLAP_NOUNS = [
    "Fronds",  "Web",    "Bloom",   "Cascade", "Tangle",
    "Tail",    "Decay",  "Diffuse", "Spread",  "Canopy",
    "Veil",    "Mirror", "Pool",    "Chamber", "Hall",
    "Membrane","Lattice","Fog",     "Haze",    "Cloud",
]

# ---------------------------------------------------------------------------
# Marquee preset definitions (OUTWIT × OVERLAP — 6 presets)
# ---------------------------------------------------------------------------
MARQUEE_PRESETS = [
    {
        "name": "The Grand Entanglement",
        "coupling_type": "CHAOS_INJECTION",
        "coupling_amount": 0.82,
        "dna": dict(brightness=0.63, warmth=0.50, movement=0.75, density=0.68, space=0.72, aggression=0.40),
        "description": "The ultimate aquatic coupling: Giant Pacific Octopus cellular automata injects chaos into Lion's Mane FDN — eight arms routing through an infinite reverb web",
        "tags": ["marquee", "flagship", "aquatic", "chaos", "fdn", "wolfram"],
    },
    {
        "name": "Arm Pattern Reflection",
        "coupling_type": "SPATIAL_BLEND",
        "coupling_amount": 0.70,
        "dna": dict(brightness=0.62, warmth=0.52, movement=0.68, density=0.67, space=0.78, aggression=0.35),
        "description": "OUTWIT's 8-arm pattern shapes OVERLAP's FDN node routing — cellular intelligence selects the reflection path through the jellyfish's tangled fronds",
        "tags": ["marquee", "spatial", "arms", "routing", "fdn"],
    },
    {
        "name": "Decay Feeds Evolution",
        "coupling_type": "ENVELOPE_SHARING",
        "coupling_amount": 0.75,
        "dna": dict(brightness=0.60, warmth=0.48, movement=0.72, density=0.66, space=0.80, aggression=0.32),
        "description": "OVERLAP decay length modulates OUTWIT generation rate — the longer the tail, the faster the evolution; reverb time as a cellular clock",
        "tags": ["marquee", "envelope", "decay", "generation", "clock"],
    },
    {
        "name": "Emergence Scatters Space",
        "coupling_type": "FREQUENCY_MODULATION",
        "coupling_amount": 0.78,
        "dna": dict(brightness=0.64, warmth=0.50, movement=0.78, density=0.63, space=0.76, aggression=0.42),
        "description": "OUTWIT emergence events scatter OVERLAP diffusion coefficients — unpredictable bloom in infinite space, each cellular birth rewrites the room",
        "tags": ["marquee", "emergence", "diffusion", "scatter", "fm"],
    },
    {
        "name": "Frond Traces the Frontier",
        "coupling_type": "PITCH_TRACKING",
        "coupling_amount": 0.68,
        "dna": dict(brightness=0.65, warmth=0.53, movement=0.65, density=0.68, space=0.82, aggression=0.28),
        "description": "OVERLAP's frond-spread traces OUTWIT's Wolfram frontier — the jellyfish draws the map the octopus has already written in cellular ink",
        "tags": ["marquee", "pitch", "tracking", "frontier", "frond"],
    },
    {
        "name": "Deep Water Intelligence",
        "coupling_type": "HARMONIC_BLEND",
        "coupling_amount": 0.72,
        "dna": dict(brightness=0.58, warmth=0.58, movement=0.70, density=0.72, space=0.83, aggression=0.33),
        "description": "Two abyssal intelligences in harmonic resonance — Wolfram rules translated into FDN mode frequencies; pattern and space become one organism",
        "tags": ["marquee", "harmonic", "abyssal", "deep-water", "intelligence"],
    },
]

# ---------------------------------------------------------------------------
# Per-pair descriptions
# OUTWIT-led: 20 partners (OVERLAP is partner[0], then remaining 19)
# OVERLAP-led: 20 partners (OUTWIT is handled in marquee; use PARTNER_ENGINES list minus OVERLAP)
# ---------------------------------------------------------------------------
OUTWIT_LED_DESCS: dict[str, str] = {
    "OVERLAP":    "OUTWIT CA arm patterns route through OVERLAP FDN nodes — cellular topology becomes spatial topology",
    "ODDFELIX":   "Neon tetra flicker speed clocks OUTWIT generation steps — the fish's swim cycle is the cellular tick",
    "ODDOSCAR":   "Oscar's gill-pulse modulates OUTWIT arm count — axolotl respiration shapes the automata's reach",
    "ORCA":       "Orca echolocation data seeds OUTWIT initial row — sonar clicks become Wolfram seed patterns",
    "OCTOPUS":    "XOctopus chromatophore signal feeds OUTWIT rule mutation — color-shift logic mirrors cellular rule change",
    "OMBRE":      "Ombre memory/forgetting gradient sets OUTWIT mutation probability — what fades reshapes the pattern",
    "OHM":        "OHM commune warmth modulates OUTWIT evolve rate — the slower the jam, the slower the cellular drift",
    "ORACLE":     "Oracle harmonic breakpoints seed OUTWIT Wolfram rules — prophecy encoded as cellular law",
    "ODYSSEY":    "Odyssey tidal phase clocks OUTWIT generation — the tide turns, the automata evolves",
    "OPAL":       "Opal grain scatter probability feeds OUTWIT cell density — granular fragments birth cellular population",
    "ORGANON":    "Organon metabolic rate drives OUTWIT evolve speed — biological rhythm as cellular engine",
    "OUROBOROS":  "Ouroboros loop length sets OUTWIT rule lifespan — the serpent bites its tail inside the automata",
    "ONSET":      "Onset drum transients trigger OUTWIT rule mutations — every hit reshapes the cellular law",
    "OBLONG":     "Oblong's unstable tuning modulates OUTWIT arm pitch spread — wobble becomes pattern drift",
    "OVERDUB":    "Overdub tape delay time modulates OUTWIT generation clock — temporal smear, cellular memory",
    "OVERWORLD":  "Overworld ERA state selects OUTWIT rule set — each chip era carries a different cellular DNA",
    "OBBLIGATO":  "Obbligato dual wind BOND macro sets OUTWIT cell birth/death balance — the bond between winds is cellular life",
    "ORPHICA":    "Orphica microsound event density clocks OUTWIT — each grain becomes a cellular timestep",
    "OTTONI":     "Ottoni GROW macro drives OUTWIT branch complexity — brass growth as arm proliferation",
    "OLE":        "OLE DRAMA macro triggers OUTWIT rule catastrophe — the drama climax rewrites the cellular law",
}

OVERLAP_LED_DESCS: dict[str, str] = {
    "OUTWIT":     "OVERLAP frond-spread traces OUTWIT Wolfram frontier — the jellyfish maps the octopus's thought in reverb",
    "ODDFELIX":   "Neon tetra spectral brightness modulates OVERLAP filter cutoff — fin flash tunes the reverb color",
    "ODDOSCAR":   "Oscar axolotl morph depth drives OVERLAP diffusion density — the more the morph, the denser the web",
    "ORCA":       "Orca breach transient scatters OVERLAP pre-delay — the apex impact opens the room instantaneously",
    "OCTOPUS":    "Octopus ink cloud opacity sets OVERLAP damping — the denser the cloud, the darker the tail",
    "OMBRE":      "Ombre dual-narrative crossfade modulates OVERLAP early/late mix — memory and forgetting blend in time",
    "OHM":        "OHM commune drift rate syncs OVERLAP modulation rate — the jam's slow tempo rocks the FDN",
    "ORACLE":     "Oracle resonance feeds OVERLAP early reflections — prophecy echoes in the FDN's first bounce",
    "ODYSSEY":    "Odyssey tidal swell expands OVERLAP room size — tide and tail rise together toward the horizon",
    "OPAL":       "Opal grain density drives OVERLAP diffusion rate — the more fragments, the wider the frond-web",
    "ORGANON":    "Organon pipe resonance propagates through OVERLAP — the cathedral breathes inside the jellyfish",
    "OUROBOROS":  "Ouroboros loop phase modulates OVERLAP decay length — the coil's rotation is the reverb's clock",
    "ONSET":      "Onset rhythmic density modulates OVERLAP mod rate — drum patterns animate the FDN's internal flutter",
    "OBLONG":     "Oblong instability shapes OVERLAP modulation depth — the more unstable, the more the tail wanders",
    "OVERDUB":    "Overdub spring reverb feeds OVERLAP FDN input — two reverb characters layered into one endless space",
    "OVERWORLD":  "Overworld chip glitch triggers OVERLAP scatter burst — pixels dissolve into reverb fronds",
    "OBBLIGATO":  "Obbligato breath pressure drives OVERLAP pre-delay — each exhalation opens the room wider",
    "ORPHICA":    "Orphica pluck brightness modulates OVERLAP filter cutoff — string attack shapes the reverb tone",
    "OTTONI":     "Ottoni bell sustain feeds OVERLAP decay time — brass ring and FDN tail merge into one shimmer",
    "OLE":        "OLE groove displacement rhythmically pumps OVERLAP modulation — the rhythm section rides the reverb",
}

# ---------------------------------------------------------------------------
# DNA blend helper
# ---------------------------------------------------------------------------
def _blend(dna_a: dict, dna_b: dict, weight_a: float = 0.6) -> dict:
    weight_b = 1.0 - weight_a
    keys = set(dna_a) | set(dna_b)
    return {
        k: round(dna_a.get(k, 0.5) * weight_a + dna_b.get(k, 0.5) * weight_b, 3)
        for k in keys
    }

# ---------------------------------------------------------------------------
# Parameter stubs
# ---------------------------------------------------------------------------
def _outwit_params(rng: random.Random) -> dict:
    return {
        "owit_rule":          rng.choice([30, 90, 110, 184, 22, 54, 45, 73]),
        "owit_arms":          rng.randint(4, 8),
        "owit_cellSize":      round(rng.uniform(0.10, 0.50), 3),
        "owit_density":       round(rng.uniform(0.40, 0.75), 3),
        "owit_evolveRate":    round(rng.uniform(0.30, 0.70), 3),
        "owit_mutateProb":    round(rng.uniform(0.05, 0.30), 3),
        "owit_pitchSpread":   round(rng.uniform(0.20, 0.60), 3),
        "owit_filterCutoff":  round(rng.uniform(0.35, 0.75), 3),
        "owit_filterReso":    round(rng.uniform(0.10, 0.45), 3),
        "owit_ampAttack":     round(rng.uniform(0.01, 0.15), 3),
        "owit_ampRelease":    round(rng.uniform(0.20, 0.80), 3),
        "owit_outputLevel":   round(rng.uniform(0.65, 0.85), 3),
        "owit_couplingLevel": round(rng.uniform(0.50, 0.80), 3),
        "owit_couplingBus":   0,
    }

def _overlap_params(rng: random.Random) -> dict:
    return {
        "olap_size":          round(rng.uniform(0.55, 0.95), 3),
        "olap_decay":         round(rng.uniform(0.50, 0.90), 3),
        "olap_damping":       round(rng.uniform(0.25, 0.65), 3),
        "olap_diffusion":     round(rng.uniform(0.50, 0.88), 3),
        "olap_modDepth":      round(rng.uniform(0.10, 0.45), 3),
        "olap_modRate":       round(rng.uniform(0.05, 0.35), 3),
        "olap_preDelay":      round(rng.uniform(0.00, 0.25), 3),
        "olap_earlyMix":      round(rng.uniform(0.20, 0.55), 3),
        "olap_lateMix":       round(rng.uniform(0.40, 0.80), 3),
        "olap_filterCutoff":  round(rng.uniform(0.40, 0.80), 3),
        "olap_filterReso":    round(rng.uniform(0.10, 0.40), 3),
        "olap_outputLevel":   round(rng.uniform(0.65, 0.85), 3),
        "olap_couplingLevel": round(rng.uniform(0.55, 0.80), 3),
        "olap_couplingBus":   0,
    }

# ---------------------------------------------------------------------------
# Preset factory
# ---------------------------------------------------------------------------
def make_preset(
    name: str,
    engine_a: str,
    engine_b: str,
    dna: dict,
    coupling_type: str,
    coupling_amount: float,
    rng: random.Random,
    description: str = "",
    tags: Optional[list] = None,
) -> dict:
    coupling_amount_r = round(coupling_amount, 3)
    intensity = "Deep" if coupling_amount_r >= 0.70 else "Moderate"

    params: dict = {}
    if engine_a == "OUTWIT" or engine_b == "OUTWIT":
        params["OUTWIT"] = _outwit_params(rng)
    if engine_a == "OVERLAP" or engine_b == "OVERLAP":
        params["OVERLAP"] = _overlap_params(rng)

    preset_tags = ["entangled", "coupling", engine_a.lower(), engine_b.lower()] + (tags or [])

    return {
        "schema_version": 1,
        "name": name,
        "version": "1.0",
        "mood": "Entangled",
        "engines": [engine_a, engine_b],
        "author": "XO_OX",
        "macros": {
            "CHARACTER": round(rng.uniform(0.35, 0.65), 2),
            "MOVEMENT":  round(rng.uniform(0.40, 0.70), 2),
            "COUPLING":  round(coupling_amount_r * 1.0, 2),
            "SPACE":     round(dna.get("space", 0.5), 2),
        },
        "sonic_dna": dna,
        "parameters": params,
        "coupling": {
            "type":   coupling_type,
            "source": engine_a,
            "target": engine_b,
            "amount": coupling_amount_r,
        },
        "couplingIntensity": intensity,
        "tags": list(dict.fromkeys(preset_tags)),  # dedupe preserving order
        "description": description or f"{engine_a} × {engine_b} — {coupling_type}",
    }

# ---------------------------------------------------------------------------
# Build all presets
# ---------------------------------------------------------------------------

def build_presets(rng: random.Random) -> list[dict]:
    presets: list[dict] = []

    # --- 6 OUTWIT × OVERLAP marquee presets ---
    for spec in MARQUEE_PRESETS:
        preset = make_preset(
            name=spec["name"],
            engine_a="OUTWIT",
            engine_b="OVERLAP",
            dna=spec["dna"],
            coupling_type=spec["coupling_type"],
            coupling_amount=spec["coupling_amount"],
            rng=rng,
            description=spec["description"],
            tags=spec["tags"],
        )
        presets.append(preset)

    # --- 20 OUTWIT-led presets ---
    outwit_adj = list(OUTWIT_ADJECTIVES)
    outwit_noun = list(OUTWIT_NOUNS)
    rng.shuffle(outwit_adj)
    rng.shuffle(outwit_noun)
    coupling_cycle = list(COUPLING_TYPES)
    rng.shuffle(coupling_cycle)

    for i, partner in enumerate(OUTWIT_PARTNER_ENGINES):
        partner_dna = PARTNER_DNA.get(partner, {})
        blended = _blend(OUTWIT_DNA, partner_dna, weight_a=0.60)
        coupling_type = coupling_cycle[i % len(coupling_cycle)]
        coupling_amount = round(rng.uniform(0.55, 0.88), 3)
        adj = outwit_adj[i % len(outwit_adj)]
        noun = outwit_noun[i % len(outwit_noun)]
        preset_name = f"{adj} {noun}"
        description = OUTWIT_LED_DESCS.get(partner, f"OUTWIT cellular automata couples with {partner}")
        preset = make_preset(
            name=preset_name,
            engine_a="OUTWIT",
            engine_b=partner,
            dna=blended,
            coupling_type=coupling_type,
            coupling_amount=coupling_amount,
            rng=rng,
            description=description,
            tags=["outwit-led", "wolfram", "cellular-automata", partner.lower()],
        )
        presets.append(preset)

    # --- 20 OVERLAP-led presets ---
    overlap_adj = list(OVERLAP_ADJECTIVES)
    overlap_noun = list(OVERLAP_NOUNS)
    rng.shuffle(overlap_adj)
    rng.shuffle(overlap_noun)
    rng.shuffle(coupling_cycle)

    for i, partner in enumerate(OVERLAP_PARTNER_ENGINES):
        partner_dna = PARTNER_DNA.get(partner, {})
        blended = _blend(OVERLAP_DNA, partner_dna, weight_a=0.60)
        coupling_type = coupling_cycle[i % len(coupling_cycle)]
        coupling_amount = round(rng.uniform(0.55, 0.88), 3)
        adj = overlap_adj[i % len(overlap_adj)]
        noun = overlap_noun[i % len(overlap_noun)]
        preset_name = f"{adj} {noun}"
        description = OVERLAP_LED_DESCS.get(partner, f"OVERLAP FDN reverb couples with {partner}")
        preset = make_preset(
            name=preset_name,
            engine_a="OVERLAP",
            engine_b=partner,
            dna=blended,
            coupling_type=coupling_type,
            coupling_amount=coupling_amount,
            rng=rng,
            description=description,
            tags=["overlap-led", "fdn", "reverb", partner.lower()],
        )
        presets.append(preset)

    return presets

# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def main() -> None:
    parser = argparse.ArgumentParser(
        description="Generate OUTWIT×OVERLAP Entangled coupling presets for XOceanus."
    )
    repo_root = Path(__file__).resolve().parent.parent
    default_out = repo_root / "Presets" / "XOceanus" / "Entangled"

    parser.add_argument(
        "--output-dir",
        type=Path,
        default=default_out,
        help=f"Output directory (default: {default_out})",
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Print preset names without writing files",
    )
    parser.add_argument(
        "--seed",
        type=int,
        default=42,
        help="Random seed for reproducibility (default: 42)",
    )
    parser.add_argument(
        "--overwrite",
        action="store_true",
        help="Overwrite existing .xometa files (default: skip)",
    )
    args = parser.parse_args()

    rng = random.Random(args.seed)
    presets = build_presets(rng)

    if args.dry_run:
        print(f"[dry-run] Would write {len(presets)} presets to: {args.output_dir}")
        print()
        # Marquee = first 6 (hardcoded spec), OUTWIT-led = next 20, OVERLAP-led = last 20
        groups = [
            ("MARQUEE (OUTWIT×OVERLAP)", presets[:6]),
            ("OUTWIT-led (20)",          presets[6:26]),
            ("OVERLAP-led (20)",         presets[26:]),
        ]
        for label, group in groups:
            print(f"  {label} ({len(group)} presets):")
            for p in group:
                engines = " × ".join(p["engines"])
                ctype = p["coupling"]["type"]
                amount = p["coupling"]["amount"]
                print(f"    {p['name']:<40}  [{engines}]  {ctype}  ({amount:.2f})")
            print()
        return

    args.output_dir.mkdir(parents=True, exist_ok=True)
    written = 0
    skipped = 0
    for preset in presets:
        filename = preset["name"] + ".xometa"
        filepath = args.output_dir / filename
        if filepath.exists() and not args.overwrite:
            skipped += 1
            continue
        with open(filepath, "w", encoding="utf-8") as fh:
            json.dump(preset, fh, indent=2)
            fh.write("\n")
        written += 1

    marquee_count = 6
    outwit_led    = 20
    overlap_led   = len(presets) - 6 - 20

    print(f"OUTWIT×OVERLAP marquee   : {marquee_count}")
    print(f"OUTWIT-led presets       : {outwit_led}")
    print(f"OVERLAP-led presets      : {overlap_led}")
    print(f"Total generated          : {len(presets)}")
    print(f"Written                  : {written}")
    print(f"Skipped (already exist)  : {skipped}  (use --overwrite to replace)")
    print(f"Output                   : {args.output_dir}")


if __name__ == "__main__":
    main()
