#!/usr/bin/env python3
"""
XPN Coupling Pair Recommender — XO_OX Designs
Recommends the best engine pairs for new Entangled/coupling presets based on
sonic DNA compatibility, feliX/Oscar affinity rules, and existing coverage gaps.

Usage:
    python xpn_coupling_pair_recommender.py --preset-dir ./Presets/XOlokun/
    python xpn_coupling_pair_recommender.py --preset-dir ./Presets/XOlokun/ --engine OPAL
    python xpn_coupling_pair_recommender.py --preset-dir ./Presets/XOlokun/ --top 30
    python xpn_coupling_pair_recommender.py --preset-dir ./Presets/XOlokun/ --output recommendations.txt
"""

import argparse
import json
import math
import sys
from pathlib import Path
from typing import Optional

# =============================================================================
# Engine Registry — 34 engines, canonical names match .xometa "engines" arrays
# =============================================================================

# Canonical engine names as they appear in .xometa "engines" arrays.
# Legacy aliases (Snap, Morph, Dub, etc.) are resolved via LEGACY_ALIASES below.
ALL_ENGINES = [
    "OddfeliX", "OddOscar", "Overdub", "Odyssey", "Oblong", "Obese",
    "Onset", "Overworld", "Opal", "Orbital", "Organon", "Ouroboros",
    "Obsidian", "Overbite", "Origami", "Oracle", "Obscura", "Oceanic",
    "Ocelot", "Optic", "Oblique", "Osprey", "Osteria", "Owlfish",
    "Ohm", "Orphica", "Obbligato", "Ottoni", "Ole", "Overlap",
    "Outwit", "Ombre", "Orca", "Octopus",
]

# Short names (all-caps) → canonical engine name for display and filtering
SHORT_TO_CANONICAL = {e.upper(): e for e in ALL_ENGINES}
# Also map common alternate spellings
SHORT_TO_CANONICAL.update({
    "ODDFELIX": "OddfeliX",
    "ODDOSCAR": "OddOscar",
    "OBBLIGATO": "Obbligato",
    "OUROBOROS": "Ouroboros",
    "OVERWORLD": "Overworld",
    "OVERBITE": "Overbite",
    "OVERDUB": "Overdub",
    "ODYSSEY": "Odyssey",
    "ORGANON": "Organon",
    "ORIGAMI": "Origami",
    "OBSCURA": "Obscura",
    "OBSIDIAN": "Obsidian",
    "OCEANIC": "Oceanic",
    "OCTOPUS": "Octopus",
    "OSTERIA": "Osteria",
    "ORPHICA": "Orphica",
    "OWLFISH": "Owlfish",
    "OUTWIT": "Outwit",
    "OVERLAP": "Overlap",
    "OBLIQUE": "Oblique",
    "OSPREY": "Osprey",
    "OCELOT": "Ocelot",
    "ORACLE": "Oracle",
    "OMBRE": "Ombre",
})

# Legacy engine name aliases → canonical names (from PresetManager.h resolveEngineAlias)
LEGACY_ALIASES = {
    "Snap": "OddfeliX",
    "Morph": "OddOscar",
    "Dub": "Overdub",
    "Drift": "Odyssey",
    "Bob": "Oblong",
    "Fat": "Obese",
    "Bite": "Overbite",
}

# feliX polarity (high brightness, digital/sharp character)
FELIX_ENGINES = {
    "OddfeliX", "Overworld", "Optic", "Origami", "Oracle", "Obsidian",
    "Ouroboros", "Onset", "Outwit", "Oblique",
}

# Oscar polarity (high warmth, organic/analog character)
OSCAR_ENGINES = {
    "OddOscar", "Opal", "Overdub", "Oceanic", "Owlfish",
    "Oblong", "Orphica", "Obbligato", "Ohm", "Ombre",
}

# Engine family groupings for FAMILY pair bonuses
ENGINE_FAMILIES = {
    "Odd Pair":       {"OddfeliX", "OddOscar"},
    "Wind Section":   {"Obbligato", "Ottoni"},
    "Constellation":  {"Ohm", "Orphica", "Obbligato", "Ottoni", "Ole"},
    "Rhythm Core":    {"Onset", "Oblong", "Obese"},
    "Dub Complex":    {"Overdub", "Opal", "Overbite"},
    "Oracle Cluster": {"Oracle", "Organon", "Ouroboros"},
    "Shore Pair":     {"Osprey", "Osteria"},
    "Chaos Pair":     {"Ouroboros", "Outwit"},
    "Depth Pair":     {"Oceanic", "Owlfish"},
    "Topological":    {"Overlap", "Outwit"},
}

# Accent colors as hex RGB for color coherence scoring
ACCENT_COLORS = {
    "OddfeliX":  (0x00, 0xA6, 0xD6),
    "OddOscar":  (0xE8, 0x83, 0x9B),
    "Overdub":   (0x6B, 0x7B, 0x3A),
    "Odyssey":   (0x7B, 0x2D, 0x8B),
    "Oblong":    (0xE9, 0xA8, 0x4A),
    "Obese":     (0xFF, 0x14, 0x93),
    "Onset":     (0x00, 0x66, 0xFF),
    "Overworld": (0x39, 0xFF, 0x14),
    "Opal":      (0xA7, 0x8B, 0xFA),
    "Orbital":   (0xFF, 0x6B, 0x6B),
    "Organon":   (0x00, 0xCE, 0xD1),
    "Ouroboros": (0xFF, 0x2D, 0x2D),
    "Obsidian":  (0xE8, 0xE0, 0xD8),
    "Overbite":  (0xF0, 0xED, 0xE8),
    "Origami":   (0xE6, 0x39, 0x46),
    "Oracle":    (0x4B, 0x00, 0x82),
    "Obscura":   (0x8A, 0x9B, 0xA8),
    "Oceanic":   (0x00, 0xB4, 0xA0),
    "Ocelot":    (0xC5, 0x83, 0x2B),
    "Optic":     (0x00, 0xFF, 0x41),
    "Oblique":   (0xBF, 0x40, 0xFF),
    "Osprey":    (0x1B, 0x4F, 0x8A),
    "Osteria":   (0x72, 0x2F, 0x37),
    "Owlfish":   (0xB8, 0x86, 0x0B),
    "Ohm":       (0x87, 0xAE, 0x73),
    "Orphica":   (0x7F, 0xDB, 0xCA),
    "Obbligato": (0xFF, 0x8A, 0x7A),
    "Ottoni":    (0x5B, 0x8A, 0x72),
    "Ole":       (0xC9, 0x37, 0x7A),
    "Overlap":   (0x00, 0xFF, 0xB4),
    "Outwit":    (0xCC, 0x66, 0x00),
    "Ombre":     (0x7B, 0x6B, 0x8A),
    "Orca":      (0x1B, 0x28, 0x38),
    "Octopus":   (0xE0, 0x40, 0xFB),
}

# Approximate Sonic DNA fingerprints for engines that lack preset coverage
# (used as defaults when no presets are found for an engine)
# Dimensions: brightness, warmth, movement, density, space, aggression
ENGINE_DEFAULT_DNA = {
    "OddfeliX":  {"brightness": 0.85, "warmth": 0.3, "movement": 0.7, "density": 0.5, "space": 0.4, "aggression": 0.6},
    "OddOscar":  {"brightness": 0.3,  "warmth": 0.8, "movement": 0.5, "density": 0.6, "space": 0.5, "aggression": 0.3},
    "Overdub":   {"brightness": 0.3,  "warmth": 0.7, "movement": 0.6, "density": 0.4, "space": 0.8, "aggression": 0.2},
    "Odyssey":   {"brightness": 0.6,  "warmth": 0.4, "movement": 0.8, "density": 0.5, "space": 0.6, "aggression": 0.4},
    "Oblong":    {"brightness": 0.5,  "warmth": 0.6, "movement": 0.5, "density": 0.6, "space": 0.5, "aggression": 0.5},
    "Obese":     {"brightness": 0.5,  "warmth": 0.5, "movement": 0.4, "density": 0.9, "space": 0.3, "aggression": 0.8},
    "Onset":     {"brightness": 0.6,  "warmth": 0.3, "movement": 0.8, "density": 0.7, "space": 0.4, "aggression": 0.7},
    "Overworld": {"brightness": 0.9,  "warmth": 0.2, "movement": 0.7, "density": 0.4, "space": 0.5, "aggression": 0.5},
    "Opal":      {"brightness": 0.5,  "warmth": 0.7, "movement": 0.6, "density": 0.5, "space": 0.8, "aggression": 0.2},
    "Orbital":   {"brightness": 0.6,  "warmth": 0.5, "movement": 0.5, "density": 0.5, "space": 0.6, "aggression": 0.4},
    "Organon":   {"brightness": 0.4,  "warmth": 0.6, "movement": 0.9, "density": 0.7, "space": 0.5, "aggression": 0.3},
    "Ouroboros": {"brightness": 0.7,  "warmth": 0.3, "movement": 0.9, "density": 0.6, "space": 0.4, "aggression": 0.8},
    "Obsidian":  {"brightness": 0.7,  "warmth": 0.3, "movement": 0.3, "density": 0.3, "space": 0.7, "aggression": 0.2},
    "Overbite":  {"brightness": 0.4,  "warmth": 0.7, "movement": 0.4, "density": 0.8, "space": 0.3, "aggression": 0.7},
    "Origami":   {"brightness": 0.8,  "warmth": 0.3, "movement": 0.6, "density": 0.4, "space": 0.6, "aggression": 0.5},
    "Oracle":    {"brightness": 0.5,  "warmth": 0.5, "movement": 0.7, "density": 0.5, "space": 0.7, "aggression": 0.4},
    "Obscura":   {"brightness": 0.4,  "warmth": 0.5, "movement": 0.4, "density": 0.5, "space": 0.6, "aggression": 0.3},
    "Oceanic":   {"brightness": 0.4,  "warmth": 0.7, "movement": 0.6, "density": 0.5, "space": 0.8, "aggression": 0.2},
    "Ocelot":    {"brightness": 0.6,  "warmth": 0.6, "movement": 0.7, "density": 0.5, "space": 0.4, "aggression": 0.6},
    "Optic":     {"brightness": 0.95, "warmth": 0.1, "movement": 0.8, "density": 0.3, "space": 0.5, "aggression": 0.4},
    "Oblique":   {"brightness": 0.7,  "warmth": 0.4, "movement": 0.7, "density": 0.5, "space": 0.5, "aggression": 0.6},
    "Osprey":    {"brightness": 0.5,  "warmth": 0.5, "movement": 0.5, "density": 0.4, "space": 0.7, "aggression": 0.3},
    "Osteria":   {"brightness": 0.4,  "warmth": 0.7, "movement": 0.4, "density": 0.6, "space": 0.6, "aggression": 0.4},
    "Owlfish":   {"brightness": 0.4,  "warmth": 0.7, "movement": 0.5, "density": 0.5, "space": 0.7, "aggression": 0.3},
    "Ohm":       {"brightness": 0.4,  "warmth": 0.7, "movement": 0.5, "density": 0.4, "space": 0.7, "aggression": 0.2},
    "Orphica":   {"brightness": 0.6,  "warmth": 0.6, "movement": 0.7, "density": 0.4, "space": 0.8, "aggression": 0.2},
    "Obbligato": {"brightness": 0.5,  "warmth": 0.6, "movement": 0.6, "density": 0.5, "space": 0.6, "aggression": 0.3},
    "Ottoni":    {"brightness": 0.6,  "warmth": 0.5, "movement": 0.5, "density": 0.6, "space": 0.5, "aggression": 0.5},
    "Ole":       {"brightness": 0.6,  "warmth": 0.6, "movement": 0.8, "density": 0.5, "space": 0.5, "aggression": 0.5},
    "Overlap":   {"brightness": 0.5,  "warmth": 0.5, "movement": 0.6, "density": 0.7, "space": 0.8, "aggression": 0.3},
    "Outwit":    {"brightness": 0.5,  "warmth": 0.4, "movement": 0.9, "density": 0.6, "space": 0.4, "aggression": 0.6},
    "Ombre":     {"brightness": 0.4,  "warmth": 0.6, "movement": 0.4, "density": 0.5, "space": 0.6, "aggression": 0.3},
    "Orca":      {"brightness": 0.3,  "warmth": 0.3, "movement": 0.7, "density": 0.6, "space": 0.5, "aggression": 0.8},
    "Octopus":   {"brightness": 0.6,  "warmth": 0.4, "movement": 0.9, "density": 0.7, "space": 0.4, "aggression": 0.6},
}

DNA_DIMENSIONS = ["brightness", "warmth", "movement", "density", "space", "aggression"]


# =============================================================================
# Helpers
# =============================================================================

def resolve_engine_name(raw: str) -> Optional[str]:
    """Resolve any engine name variant to canonical form."""
    if raw in ALL_ENGINES:
        return raw
    if raw in LEGACY_ALIASES:
        return LEGACY_ALIASES[raw]
    upper = raw.upper()
    if upper in SHORT_TO_CANONICAL:
        return SHORT_TO_CANONICAL[upper]
    return None


def canonical_pair(a: str, b: str) -> tuple:
    """Return a sorted tuple so (A, B) == (B, A)."""
    return tuple(sorted((a, b)))


def color_distance(e1: str, e2: str) -> float:
    """Euclidean RGB distance (0=identical, ~441=maximally distant)."""
    c1 = ACCENT_COLORS.get(e1, (128, 128, 128))
    c2 = ACCENT_COLORS.get(e2, (128, 128, 128))
    return math.sqrt(sum((a - b) ** 2 for a, b in zip(c1, c2)))


def dna_contrast_score(dna_a: dict, dna_b: dict) -> float:
    """
    Score how contrastingly complementary two DNA profiles are (0.0–1.0).
    High score = opposite on multiple axes = interesting tension.
    """
    diffs = [abs(dna_a.get(d, 0.5) - dna_b.get(d, 0.5)) for d in DNA_DIMENSIONS]
    # Count axes with significant opposition (diff > 0.3)
    opposing_axes = sum(1 for d in diffs if d > 0.3)
    avg_diff = sum(diffs) / len(diffs)
    # Weight: number of opposing axes counts more than raw average
    return min(1.0, (opposing_axes / len(DNA_DIMENSIONS)) * 0.6 + avg_diff * 0.4)


def dna_unison_score(dna_a: dict, dna_b: dict) -> float:
    """
    Score how similar two DNA profiles are (0.0–1.0).
    High score = same mood neighborhood = reinforcing texture.
    """
    diffs = [abs(dna_a.get(d, 0.5) - dna_b.get(d, 0.5)) for d in DNA_DIMENSIONS]
    avg_diff = sum(diffs) / len(diffs)
    return max(0.0, 1.0 - avg_diff * 2.0)


def felix_oscar_score(e1: str, e2: str) -> float:
    """
    Bonus for feliX+Oscar polarity cross-pairings (0.0 or 0.3).
    """
    is_felix_a = e1 in FELIX_ENGINES
    is_oscar_a = e1 in OSCAR_ENGINES
    is_felix_b = e2 in FELIX_ENGINES
    is_oscar_b = e2 in OSCAR_ENGINES
    if (is_felix_a and is_oscar_b) or (is_oscar_a and is_felix_b):
        return 0.3
    return 0.0


def family_score(e1: str, e2: str) -> float:
    """Bonus if both engines share a family grouping."""
    for members in ENGINE_FAMILIES.values():
        if e1 in members and e2 in members:
            return 0.4
    return 0.0


def color_coherence_score(e1: str, e2: str) -> float:
    """
    Score for accent color proximity (0.0–0.2).
    Close colors suggest visual coherence in the Gallery Model.
    Max RGB distance ~441 for pure (0,0,0)↔(255,255,255).
    """
    dist = color_distance(e1, e2)
    normalized = dist / 441.0
    return max(0.0, (1.0 - normalized) * 0.2)


def coverage_penalty(count: int) -> float:
    """
    Penalty for pairs that already have Entangled presets.
    0 presets → 0 penalty, 1-2 → small, 3+ → significant.
    """
    if count == 0:
        return 0.0
    return min(0.5, count * 0.15)


def get_pair_category(e1: str, e2: str, dna_a: dict, dna_b: dict) -> str:
    """Classify a pair as CONTRAST, UNISON, or FAMILY."""
    fam = family_score(e1, e2)
    if fam > 0:
        return "FAMILY"
    contrast = dna_contrast_score(dna_a, dna_b)
    unison = dna_unison_score(dna_a, dna_b)
    if contrast >= unison:
        return "CONTRAST"
    return "UNISON"


def build_rationale(e1: str, e2: str, dna_a: dict, dna_b: dict,
                    coverage: int, score: float, category: str) -> str:
    """Generate a human-readable rationale for a recommendation."""
    parts = []

    if category == "FAMILY":
        for fam_name, members in ENGINE_FAMILIES.items():
            if e1 in members and e2 in members:
                parts.append(f"Same family ({fam_name})")
                break

    fx_bonus = felix_oscar_score(e1, e2)
    if fx_bonus > 0:
        fe = e1 if e1 in FELIX_ENGINES else e2
        os = e1 if e1 in OSCAR_ENGINES else e2
        parts.append(f"feliX({fe}) + Oscar({os}) polarity")

    if category == "CONTRAST":
        diffs = {d: abs(dna_a.get(d, 0.5) - dna_b.get(d, 0.5)) for d in DNA_DIMENSIONS}
        top = sorted(diffs, key=lambda d: -diffs[d])[:2]
        descs = []
        for dim in top:
            hi = e1 if dna_a.get(dim, 0.5) > dna_b.get(dim, 0.5) else e2
            lo = e2 if hi == e1 else e1
            descs.append(f"high {dim} ({hi}) vs low ({lo})")
        parts.append("Tension: " + ", ".join(descs))
    elif category == "UNISON":
        common = []
        for dim in DNA_DIMENSIONS:
            v1, v2 = dna_a.get(dim, 0.5), dna_b.get(dim, 0.5)
            if abs(v1 - v2) < 0.2 and (v1 + v2) / 2 > 0.55:
                common.append(dim)
        if common:
            parts.append("Shared texture: " + ", ".join(common[:3]))

    if coverage == 0:
        parts.append("No existing Entangled coverage")
    else:
        parts.append(f"{coverage} existing preset(s)")

    return " | ".join(parts) if parts else f"Score {score:.3f}"


# =============================================================================
# Preset scanning
# =============================================================================

def load_xometa(path: Path) -> Optional[dict]:
    """Load and parse a .xometa file; return None on error."""
    try:
        with open(path, "r", encoding="utf-8") as f:
            return json.load(f)
    except (json.JSONDecodeError, OSError):
        return None


def collect_engine_dna(preset_dir: Path) -> dict:
    """
    Walk all .xometa files outside Entangled/ and build per-engine average DNA.
    Returns: {canonical_engine_name: {dim: avg_value, ...}}
    """
    engine_dna_accum: dict[str, dict[str, list]] = {}

    for meta_path in preset_dir.rglob("*.xometa"):
        # Skip Entangled presets — they are coupling presets, not single-engine
        if "Entangled" in meta_path.parts:
            continue
        data = load_xometa(meta_path)
        if not data:
            continue
        dna = data.get("dna")
        if not dna:
            continue
        engines_raw = data.get("engines", [])
        if len(engines_raw) != 1:
            continue  # only solo presets carry a single engine's pure DNA
        engine = resolve_engine_name(engines_raw[0])
        if not engine:
            continue
        if engine not in engine_dna_accum:
            engine_dna_accum[engine] = {d: [] for d in DNA_DIMENSIONS}
        for dim in DNA_DIMENSIONS:
            val = dna.get(dim)
            if isinstance(val, (int, float)):
                engine_dna_accum[engine][dim].append(float(val))

    # Average per engine
    result = {}
    for engine, dims in engine_dna_accum.items():
        result[engine] = {}
        for dim, vals in dims.items():
            if vals:
                result[engine][dim] = sum(vals) / len(vals)
            else:
                result[engine][dim] = ENGINE_DEFAULT_DNA.get(engine, {}).get(dim, 0.5)

    # Fill in defaults for engines with no solo presets found
    for engine in ALL_ENGINES:
        if engine not in result:
            result[engine] = dict(ENGINE_DEFAULT_DNA.get(engine, {d: 0.5 for d in DNA_DIMENSIONS}))

    return result


def scan_entangled_coverage(preset_dir: Path) -> dict:
    """
    Scan all Entangled .xometa files and return a dict of:
      {canonical_pair_tuple: count_of_presets}
    """
    coverage: dict[tuple, int] = {}

    entangled_dirs = [
        preset_dir / "Entangled",
        preset_dir / "XOlokun" / "Entangled",
    ]
    search_roots = [d for d in entangled_dirs if d.exists()]
    if not search_roots:
        # fallback: search anywhere under preset_dir with mood==Entangled
        search_roots = [preset_dir]

    for root in search_roots:
        for meta_path in root.rglob("*.xometa"):
            data = load_xometa(meta_path)
            if not data:
                continue
            if data.get("mood") not in ("Entangled", "entangled"):
                if root == preset_dir:
                    # In the fallback case only filter by mood
                    continue
            engines_raw = data.get("engines", [])
            if len(engines_raw) < 2:
                continue
            resolved = [resolve_engine_name(e) for e in engines_raw]
            resolved = [e for e in resolved if e]
            # Build pairs from all combinations present in the preset
            seen = set()
            for i in range(len(resolved)):
                for j in range(i + 1, len(resolved)):
                    p = canonical_pair(resolved[i], resolved[j])
                    if p not in seen:
                        seen.add(p)
                        coverage[p] = coverage.get(p, 0) + 1

    return coverage


# =============================================================================
# Scoring
# =============================================================================

def score_pair(e1: str, e2: str, dna_a: dict, dna_b: dict,
               coverage: int) -> dict:
    """Compute a full compatibility score and metadata for one engine pair."""
    contrast = dna_contrast_score(dna_a, dna_b)
    unison = dna_unison_score(dna_a, dna_b)
    fx_bonus = felix_oscar_score(e1, e2)
    fam_bonus = family_score(e1, e2)
    color_bonus = color_coherence_score(e1, e2)
    penalty = coverage_penalty(coverage)

    # Composite score — feliX/Oscar and family bonuses are strong signals
    raw = (
        max(contrast, unison) * 0.45
        + fx_bonus
        + fam_bonus
        + color_bonus
        - penalty
    )
    score = max(0.0, min(1.0, raw))

    category = get_pair_category(e1, e2, dna_a, dna_b)
    rationale = build_rationale(e1, e2, dna_a, dna_b, coverage, score, category)

    return {
        "pair": (e1, e2),
        "score": score,
        "category": category,
        "contrast": contrast,
        "unison": unison,
        "felix_oscar_bonus": fx_bonus,
        "family_bonus": fam_bonus,
        "color_bonus": color_bonus,
        "coverage_penalty": penalty,
        "existing_presets": coverage,
        "rationale": rationale,
    }


# =============================================================================
# Main recommendation engine
# =============================================================================

def recommend(preset_dir: Path, engine_filter: Optional[str] = None,
              top_n: int = 20, quiet: bool = False) -> list:
    """
    Run full recommendation pipeline and return sorted list of scored pairs.
    """
    def info(msg: str = "") -> None:
        if not quiet:
            print(msg, file=sys.stderr)

    info(f"Scanning presets in: {preset_dir}")

    engine_dna = collect_engine_dna(preset_dir)
    coverage_map = scan_entangled_coverage(preset_dir)

    total_entangled = sum(coverage_map.values())
    covered_pairs = len(coverage_map)
    total_possible = len(ALL_ENGINES) * (len(ALL_ENGINES) - 1) // 2
    uncovered = total_possible - covered_pairs

    info(f"Engines registered: {len(ALL_ENGINES)}")
    info(f"Total possible pairs: {total_possible}")
    info(f"Pairs with Entangled presets: {covered_pairs}  ({total_entangled} total presets)")
    info(f"Pairs with zero coverage: {uncovered}")
    info()

    # Resolve engine filter to canonical name
    filter_canonical = None
    if engine_filter:
        filter_canonical = resolve_engine_name(engine_filter)
        if not filter_canonical:
            print(f"WARNING: engine '{engine_filter}' not recognized. Ignoring filter.", file=sys.stderr)
        else:
            info(f"Filtering to pairs involving: {filter_canonical}")
            info()

    # Score every pair
    results = []
    for i, e1 in enumerate(ALL_ENGINES):
        for j in range(i + 1, len(ALL_ENGINES)):
            e2 = ALL_ENGINES[j]
            if filter_canonical and filter_canonical not in (e1, e2):
                continue
            pair_key = canonical_pair(e1, e2)
            cov = coverage_map.get(pair_key, 0)
            dna_a = engine_dna.get(e1, ENGINE_DEFAULT_DNA.get(e1, {}))
            dna_b = engine_dna.get(e2, ENGINE_DEFAULT_DNA.get(e2, {}))
            scored = score_pair(e1, e2, dna_a, dna_b, cov)
            results.append(scored)

    # Sort by score descending
    results.sort(key=lambda r: -r["score"])
    return results[:top_n]


# =============================================================================
# Output formatting
# =============================================================================

def format_recommendations(recs: list, top_n: int) -> str:
    lines = []
    lines.append("=" * 72)
    lines.append("  XO_OX Coupling Pair Recommender — Top Recommendations")
    lines.append("=" * 72)
    lines.append("")

    category_order = ["FAMILY", "CONTRAST", "UNISON"]
    grouped: dict[str, list] = {c: [] for c in category_order}
    for r in recs:
        grouped.setdefault(r["category"], []).append(r)

    rank = 1
    for cat in category_order:
        cat_recs = grouped.get(cat, [])
        if not cat_recs:
            continue
        if cat == "CONTRAST":
            label = "CONTRAST PAIRS — Tension / Opposing Polarity"
        elif cat == "UNISON":
            label = "UNISON PAIRS — Reinforcement / Shared Texture"
        else:
            label = "FAMILY PAIRS — Shared Lineage / Conceptual Bond"
        lines.append(f"  {label}")
        lines.append("  " + "-" * 68)
        for r in cat_recs:
            e1, e2 = r["pair"]
            lines.append(
                f"  #{rank:>2}  {e1} + {e2}"
            )
            lines.append(
                f"       Score: {r['score']:.3f}  "
                f"(contrast={r['contrast']:.2f}, unison={r['unison']:.2f}, "
                f"existing={r['existing_presets']})"
            )
            lines.append(f"       {r['rationale']}")
            lines.append("")
            rank += 1

    lines.append("=" * 72)
    lines.append(f"  {len(recs)} recommendations shown.")
    lines.append("=" * 72)
    return "\n".join(lines)


# =============================================================================
# CLI
# =============================================================================

def parse_args():
    p = argparse.ArgumentParser(
        description="Recommend engine coupling pairs for new Entangled presets.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    p.add_argument(
        "--preset-dir", required=True, type=Path,
        help="Root preset directory (e.g. ./Presets/XOlokun/)",
    )
    p.add_argument(
        "--engine", default=None,
        help="Filter: show only pairs involving this engine (e.g. OPAL, Overdub).",
    )
    p.add_argument(
        "--top", type=int, default=20,
        metavar="N",
        help="Number of recommendations to show (default: 20).",
    )
    p.add_argument(
        "--output", type=Path, default=None,
        help="Write recommendations to this file (default: stdout).",
    )
    p.add_argument(
        "--json", action="store_true",
        help="Output raw JSON instead of formatted text.",
    )
    return p.parse_args()


def main():
    args = parse_args()

    if not args.preset_dir.exists():
        print(f"ERROR: preset-dir not found: {args.preset_dir}", file=sys.stderr)
        sys.exit(1)

    recs = recommend(args.preset_dir, engine_filter=args.engine, top_n=args.top,
                     quiet=args.json)

    if args.json:
        # Serialize tuples as lists for JSON
        output_data = []
        for r in recs:
            row = dict(r)
            row["pair"] = list(r["pair"])
            output_data.append(row)
        output = json.dumps(output_data, indent=2)
    else:
        output = format_recommendations(recs, args.top)

    if args.output:
        args.output.write_text(output, encoding="utf-8")
        print(f"Recommendations written to: {args.output}")
    else:
        print(output)


if __name__ == "__main__":
    main()
