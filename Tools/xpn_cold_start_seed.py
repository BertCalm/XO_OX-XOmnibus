from __future__ import annotations

"""
xpn_cold_start_seed.py — Cold-Start Seed Protocol (Vision Quest 007)

Converts the existing XO_OX pack catalog (.xometa presets) into synthetic
curation session records that bootstrap the learning system by treating every
past pack design decision as a retroactive training example.

Usage:
    python xpn_cold_start_seed.py \
        --presets-dir ../Presets/XOmnibus/ \
        --profiles-dir ../profiles/ \
        --output builds/seed_sessions.json
"""

import argparse
import glob
import json
import math
import os
import sys
from datetime import date
from pathlib import Path
from typing import Any

try:
    import yaml
except ImportError:
    print("ERROR: PyYAML is required. Install with: pip install pyyaml", file=sys.stderr)
    sys.exit(1)

# ---------------------------------------------------------------------------
# Constants
# ---------------------------------------------------------------------------

DNA_DIMENSIONS: list[str] = [
    "brightness",
    "warmth",
    "movement",
    "density",
    "space",
    "aggression",
]

SEED_DATE: str = date.today().isoformat()

# Engines that are legacy aliases or non-canonical names to skip/map.
# These appear in the corpus but are not official O-word engines.
ALIAS_MAP: dict[str, str] = {
    "Octave": None,   # stale name — drop
    "Oleg": None,     # stale name — drop
    "Otis": None,     # stale name — drop
    "Oto": None,      # stale name — drop
}

# ---------------------------------------------------------------------------
# Loading helpers
# ---------------------------------------------------------------------------


def load_xometa(path: str) -> dict[str, Any] | None:
    """Load a single .xometa JSON file. Returns None on any parse error."""
    try:
        with open(path, "r", encoding="utf-8") as fh:
            data = json.load(fh)
        # Minimal schema check
        if not isinstance(data, dict):
            return None
        if "dna" not in data or not isinstance(data["dna"], dict):
            return None
        return data
    except Exception:
        return None


def load_all_presets(presets_dir: str) -> list[dict[str, Any]]:
    """
    Walk presets_dir recursively and load every .xometa file.
    Returns a flat list of valid preset dicts, each augmented with
    a `_path` and `_mood_folder` key.
    """
    pattern = os.path.join(presets_dir, "**", "*.xometa")
    paths = glob.glob(pattern, recursive=True)

    presets: list[dict[str, Any]] = []
    skipped = 0

    for p in paths:
        data = load_xometa(p)
        if data is None:
            skipped += 1
            continue
        # Derive mood from directory name (two levels up from file: mood/file.xometa)
        rel = os.path.relpath(p, presets_dir)
        parts = Path(rel).parts
        mood_folder = parts[0] if len(parts) >= 2 else "Unknown"
        data["_path"] = p
        data["_mood_folder"] = mood_folder
        presets.append(data)

    if skipped:
        print(f"  [warn] Skipped {skipped} unreadable/invalid .xometa files", file=sys.stderr)

    return presets


def load_profiles(profiles_dir: str) -> list[dict[str, Any]]:
    """
    Load all YAML profiles from profiles_dir.
    Returns list of parsed YAML dicts.  Missing dir → empty list.
    """
    if not os.path.isdir(profiles_dir):
        print(f"  [warn] Profiles directory not found: {profiles_dir}", file=sys.stderr)
        return []

    profiles: list[dict[str, Any]] = []
    for p in glob.glob(os.path.join(profiles_dir, "*.yaml")):
        try:
            with open(p, "r", encoding="utf-8") as fh:
                data = yaml.safe_load(fh)
            if isinstance(data, dict):
                profiles.append(data)
        except Exception as exc:
            print(f"  [warn] Could not load profile {p}: {exc}", file=sys.stderr)

    return profiles


# ---------------------------------------------------------------------------
# DNA helpers
# ---------------------------------------------------------------------------


def extract_dna(preset: dict[str, Any]) -> dict[str, float]:
    """
    Extract the 6D DNA vector from a preset.
    Missing dimensions default to 0.0.
    """
    raw = preset.get("dna", {})
    return {dim: float(raw.get(dim, 0.0)) for dim in DNA_DIMENSIONS}


def dna_centroid(dna_vectors: list[dict[str, float]]) -> dict[str, float]:
    """Compute mean DNA vector across a collection of presets."""
    if not dna_vectors:
        return {dim: 0.0 for dim in DNA_DIMENSIONS}
    centroid: dict[str, float] = {}
    for dim in DNA_DIMENSIONS:
        centroid[dim] = sum(v[dim] for v in dna_vectors) / len(dna_vectors)
    return centroid


def euclidean_distance(a: dict[str, float], b: dict[str, float]) -> float:
    """Euclidean distance between two DNA dicts (over shared DNA_DIMENSIONS)."""
    return math.sqrt(sum((a.get(d, 0.0) - b.get(d, 0.0)) ** 2 for d in DNA_DIMENSIONS))


# ---------------------------------------------------------------------------
# Cluster / distribution analysis
# ---------------------------------------------------------------------------


def infer_dna_clusters(
    dna_vectors: list[dict[str, float]],
) -> dict[str, dict[str, float]]:
    """
    Identify which DNA dimensions cluster together by computing pairwise
    Pearson correlations.  Returns a dict mapping each dimension to its most
    correlated partner and the correlation coefficient.

    For n < 3 we cannot compute meaningful correlations, so we return an
    empty dict.
    """
    n = len(dna_vectors)
    if n < 3:
        return {}

    # Compute means
    means = {dim: sum(v[dim] for v in dna_vectors) / n for dim in DNA_DIMENSIONS}

    # Compute pairwise Pearson correlations
    correlations: dict[str, dict[str, float]] = {}
    for d1 in DNA_DIMENSIONS:
        best_partner = ""
        best_corr = 0.0  # sentinel: 0.0 so any non-zero |r| beats it
        for d2 in DNA_DIMENSIONS:
            if d1 == d2:
                continue
            num = sum(
                (v[d1] - means[d1]) * (v[d2] - means[d2]) for v in dna_vectors
            )
            denom_a = math.sqrt(sum((v[d1] - means[d1]) ** 2 for v in dna_vectors))
            denom_b = math.sqrt(sum((v[d2] - means[d2]) ** 2 for v in dna_vectors))
            if denom_a < 1e-9 or denom_b < 1e-9:
                continue
            r = num / (denom_a * denom_b)
            if abs(r) > abs(best_corr):
                best_corr = r
                best_partner = d2
        if best_partner:
            correlations[d1] = {"partner": best_partner, "r": round(best_corr, 4)}

    return correlations


def mood_distribution(
    presets: list[dict[str, Any]],
) -> dict[str, int]:
    """
    Count presets per mood.  Uses the `mood` field in the preset JSON first;
    falls back to the directory name (_mood_folder).
    """
    dist: dict[str, int] = {}
    for p in presets:
        mood = p.get("mood") or p.get("_mood_folder") or "Unknown"
        dist[mood] = dist.get(mood, 0) + 1
    return dist


# ---------------------------------------------------------------------------
# Profile matching
# ---------------------------------------------------------------------------


def profile_dna_center(profile: dict[str, Any]) -> dict[str, float]:
    """
    Extract the DNA center vector from a YAML profile's genotype.dna_ranges.
    Falls back to 0.5 for any missing dimension.
    """
    ranges = (
        profile.get("genotype", {}).get("dna_ranges", {})
    )
    center: dict[str, float] = {}
    for dim in DNA_DIMENSIONS:
        dim_cfg = ranges.get(dim, {})
        if isinstance(dim_cfg, dict):
            center[dim] = float(dim_cfg.get("center", 0.5))
        else:
            center[dim] = 0.5
    return center


def match_profile(
    centroid: dict[str, float],
    profiles: list[dict[str, Any]],
) -> str:
    """
    Find the profile whose DNA center is nearest (Euclidean) to centroid.
    Returns the profile_id string, or "unmatched" if no profiles are loaded.
    """
    if not profiles:
        return "unmatched"

    best_id = "unmatched"
    best_dist = float("inf")

    for prof in profiles:
        prof_center = profile_dna_center(prof)
        dist = euclidean_distance(centroid, prof_center)
        if dist < best_dist:
            best_dist = dist
            best_id = prof.get("profile_id", prof.get("display_name", "unknown"))

    return best_id


# ---------------------------------------------------------------------------
# Session generation
# ---------------------------------------------------------------------------


def engine_canonical_name(raw: str) -> str:
    """
    Convert the engine name as stored in .xometa (CamelCase, e.g. 'OddfeliX')
    to the canonical short UPPER name (e.g. 'ODDFELIX') used in session records.
    """
    # Simple strategy: uppercase the whole string.
    # The CLAUDE.md defines e.g. "OddfeliX" → "ODDFELIX", "OceanDeep" → "OCEANDEEP".
    return raw.upper()


def build_engine_groups(
    presets: list[dict[str, Any]],
) -> dict[str, list[dict[str, Any]]]:
    """
    Group presets by their PRIMARY engine (first entry in the `engines` list).
    Multi-engine (coupling) presets are attributed to the first engine.
    Presets with no engine are skipped.
    Presets whose primary engine is in ALIAS_MAP (value=None) are skipped.
    """
    groups: dict[str, list[dict[str, Any]]] = {}

    for p in presets:
        engines_list = p.get("engines") or []
        if not engines_list:
            continue
        raw_engine = engines_list[0]
        # Check alias map
        if raw_engine in ALIAS_MAP:
            if ALIAS_MAP[raw_engine] is None:
                continue  # skip deprecated engine
            raw_engine = ALIAS_MAP[raw_engine]
        canonical = engine_canonical_name(raw_engine)
        groups.setdefault(canonical, []).append(p)

    return groups


def generate_session(
    engine: str,
    engine_presets: list[dict[str, Any]],
    profiles: list[dict[str, Any]],
    session_index: int,
) -> dict[str, Any]:
    """
    Build a single synthetic curation session record for one engine.
    """
    dna_vectors = [extract_dna(p) for p in engine_presets]
    centroid = dna_centroid(dna_vectors)
    clusters = infer_dna_clusters(dna_vectors)
    mood_dist = mood_distribution(engine_presets)
    matched_profile = match_profile(centroid, profiles)

    # Preset names (use 'name' field; fall back to filename stem)
    preset_names: list[str] = []
    for p in engine_presets:
        name = p.get("name")
        if not name:
            name = Path(p["_path"]).stem
        preset_names.append(name)

    session_id = f"seed-{engine.lower()}-{session_index:03d}"

    return {
        "session_id": session_id,
        "session_type": "retroactive_seed",
        "engine": engine,
        "presets_selected": preset_names,
        "inferred_profile": matched_profile,
        "dna_centroid": {dim: round(centroid[dim], 4) for dim in DNA_DIMENSIONS},
        "dna_clusters": clusters,
        "mood_distribution": mood_dist,
        "total_presets": len(engine_presets),
        "seed_date": SEED_DATE,
        "source": "existing_catalog",
    }


# ---------------------------------------------------------------------------
# CLI entry point
# ---------------------------------------------------------------------------


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Cold-Start Seed Protocol — convert .xometa catalog to synthetic curation sessions"
    )
    parser.add_argument(
        "--presets-dir",
        default=os.path.join(os.path.dirname(__file__), "..", "Presets", "XOmnibus"),
        help="Root directory containing mood-subfolder .xometa files",
    )
    parser.add_argument(
        "--profiles-dir",
        default=os.path.join(os.path.dirname(__file__), "..", "profiles"),
        help="Directory containing .yaml profile definitions",
    )
    parser.add_argument(
        "--output",
        default=os.path.join(os.path.dirname(__file__), "..", "builds", "seed_sessions.json"),
        help="Output path for seed_sessions.json",
    )
    args = parser.parse_args()

    presets_dir = os.path.abspath(args.presets_dir)
    profiles_dir = os.path.abspath(args.profiles_dir)
    output_path = os.path.abspath(args.output)

    print(f"[cold-start seed] Scanning presets: {presets_dir}")
    print(f"[cold-start seed] Loading profiles: {profiles_dir}")
    print(f"[cold-start seed] Output:           {output_path}")
    print()

    # ---- Load data ----
    all_presets = load_all_presets(presets_dir)
    profiles = load_profiles(profiles_dir)

    print(f"  Loaded {len(all_presets):,} presets")
    print(f"  Loaded {len(profiles)} profiles: {[p.get('profile_id', '?') for p in profiles]}")
    print()

    # ---- Group by engine ----
    engine_groups = build_engine_groups(all_presets)
    engines_sorted = sorted(engine_groups.keys())
    print(f"  Engines found: {len(engines_sorted)}")
    print()

    # ---- Generate sessions ----
    sessions: list[dict[str, Any]] = []
    profile_match_tally: dict[str, list[str]] = {}  # profile_id → [engine, ...]

    for idx, engine in enumerate(engines_sorted, start=1):
        ep = engine_groups[engine]
        session = generate_session(engine, ep, profiles, idx)
        sessions.append(session)
        prof = session["inferred_profile"]
        profile_match_tally.setdefault(prof, []).append(engine)

    # ---- Write output ----
    os.makedirs(os.path.dirname(output_path), exist_ok=True)
    output_doc = {
        "schema_version": 1,
        "generated_at": SEED_DATE,
        "source": "xpn_cold_start_seed.py",
        "total_engines": len(sessions),
        "total_presets": len(all_presets),
        "profiles_used": [p.get("profile_id", "?") for p in profiles],
        "sessions": sessions,
    }

    with open(output_path, "w", encoding="utf-8") as fh:
        json.dump(output_doc, fh, indent=2, ensure_ascii=False)

    # ---- Summary ----
    print("=" * 60)
    print("COLD-START SEED COMPLETE")
    print("=" * 60)
    print(f"  Engines processed : {len(sessions)}")
    print(f"  Total presets     : {len(all_presets):,}")
    print(f"  Output            : {output_path}")
    print()
    print("  Per-engine preset counts (top 20):")
    top_engines = sorted(engine_groups.items(), key=lambda kv: -len(kv[1]))[:20]
    for eng, ep in top_engines:
        print(f"    {eng:<20} {len(ep):>5} presets")
    print()
    print("  Profile → engines matched:")
    for prof_id, matched_engines in sorted(profile_match_tally.items()):
        print(f"    {prof_id:<30} → {', '.join(sorted(matched_engines))}")
    print()


if __name__ == "__main__":
    main()
