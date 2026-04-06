#!/usr/bin/env python3
"""
XPN Preset Variety Scorer

Scores how much variety a preset collection has across 5 dimensions:
  1. DNA Spread        — mean pairwise Euclidean distance in 6D sonic-DNA space
  2. Mood Balance      — Shannon entropy of the 7-mood distribution
  3. Naming Diversity  — unique words / total words across all preset names
  4. Engine Coverage   — fraction of the 34 known XO_OX engines represented
  5. Parameter Range   — average fraction of observed range utilised per numeric param

Combines into an overall Variety Score 0–100 with per-dimension letter grades,
identifies the weakest dimension, and prints actionable recommendations.

Usage:
    python xpn_preset_variety_scorer.py --preset-dir <dir>
    python xpn_preset_variety_scorer.py --preset-dir <dir> --engine OPAL
    python xpn_preset_variety_scorer.py --preset-dir <dir> --output report.txt
    python xpn_preset_variety_scorer.py --preset-dir <dir> --json
"""

import argparse
import json
import math
import sys
from collections import Counter, defaultdict
from pathlib import Path
from typing import Optional

# ---------------------------------------------------------------------------
# Constants
# ---------------------------------------------------------------------------

DIMS = ["brightness", "warmth", "movement", "density", "space", "aggression"]

MOODS = ["Foundation", "Atmosphere", "Entangled", "Prism", "Flux", "Aether", "Family",
         "Submerged", "Coupling", "Crystalline", "Deep", "Ethereal", "Kinetic", "Luminous",
         "Organic", "Shadow"]

ALL_ENGINES = [
    "OBLONG", "OVERBITE", "OVERDUB", "ODYSSEY", "ONSET", "OPAL",
    "OVERWORLD", "OHM", "ORPHICA", "OBBLIGATO", "OTTONI", "OLE",
    "OVERLAP", "OUTWIT", "OSTINATO", "OPENSKY", "OCEANDEEP", "OUIE",
    "ORGANON", "OUROBOROS", "DRIFT", "FAT", "SNAP", "MORPH",
    "OVERTONE", "KNOT", "ORGANISM", "SIGNAL", "ORACLE", "ORACLE2",
    "ORACLE3", "ORACLE4", "ORACLE5", "ORACLE6",
]  # 34 total

ENTANGLED_WARN_THRESHOLD = 0.10  # warn if Entangled < 10 % of collection

# Dimension weights for overall score (must sum to 1.0)
WEIGHTS = {
    "dna_spread":      0.25,
    "mood_balance":    0.20,
    "naming_diversity":0.20,
    "engine_coverage": 0.20,
    "param_range":     0.15,
}

# ---------------------------------------------------------------------------
# Loading
# ---------------------------------------------------------------------------

def load_presets(preset_dir: Path, engine_filter: Optional[str] = None) -> list[dict]:
    """Load all .xometa files from *preset_dir* (recursive)."""
    files = sorted(preset_dir.rglob("*.xometa"))
    if not files:
        print(f"WARN: no .xometa files found in {preset_dir}", file=sys.stderr)
        return []

    presets = []
    skipped = 0
    for fpath in files:
        try:
            data = json.loads(fpath.read_text(encoding="utf-8"))
        except Exception as exc:
            print(f"WARN: could not parse {fpath.name}: {exc}", file=sys.stderr)
            skipped += 1
            continue

        engines = data.get("engines", [])
        if engine_filter and engine_filter.upper() not in [e.upper() for e in engines]:
            continue

        preset = {
            "path":    str(fpath),
            "name":    data.get("name", fpath.stem),
            "mood":    data.get("mood", "Unknown"),
            "engines": engines,
            "dna":     data.get("dna") or data.get("sonic_dna") or {},
            "params":  data.get("params") or data.get("parameters") or {},
        }
        presets.append(preset)

    if skipped:
        print(f"WARN: skipped {skipped} unparseable file(s)", file=sys.stderr)
    return presets

# ---------------------------------------------------------------------------
# Dimension 1 — DNA Spread
# ---------------------------------------------------------------------------

def score_dna_spread(presets: list[dict]) -> tuple[float, dict]:
    """Mean pairwise Euclidean distance in 6D DNA space, normalised to [0, 1]."""
    vectors = []
    for p in presets:
        dna = p["dna"]
        if all(d in dna for d in DIMS):
            vectors.append([float(dna[d]) for d in DIMS])

    if len(vectors) < 2:
        return 0.0, {"note": "fewer than 2 presets with full DNA", "n_vectors": len(vectors)}

    total, count = 0.0, 0
    for i in range(len(vectors)):
        for j in range(i + 1, len(vectors)):
            dist = math.sqrt(sum((vectors[i][k] - vectors[j][k]) ** 2 for k in range(6)))
            total += dist
            count += 1

    mean_dist = total / count
    # Maximum possible distance in [0,1]^6 is sqrt(6) ≈ 2.449
    max_dist = math.sqrt(6)
    normalised = min(mean_dist / (max_dist * 0.5), 1.0)  # 0.5*max_dist is a realistic ceiling

    return normalised, {
        "mean_pairwise_distance": round(mean_dist, 4),
        "n_vectors":              len(vectors),
        "n_pairs":                count,
    }

# ---------------------------------------------------------------------------
# Dimension 2 — Mood Balance
# ---------------------------------------------------------------------------

def score_mood_balance(presets: list[dict]) -> tuple[float, dict]:
    """Shannon entropy of the mood distribution, normalised by log2(7)."""
    counts = Counter(p["mood"] for p in presets)
    total = sum(counts.values())
    if total == 0:
        return 0.0, {"note": "no presets"}

    entropy = 0.0
    for mood in MOODS:
        n = counts.get(mood, 0)
        if n > 0:
            p = n / total
            entropy -= p * math.log2(p)

    max_entropy = math.log2(len(MOODS))
    normalised = entropy / max_entropy if max_entropy > 0 else 0.0

    distribution = {m: counts.get(m, 0) for m in MOODS}
    distribution["Unknown"] = counts.get("Unknown", 0)

    entangled_frac = counts.get("Entangled", 0) / total if total else 0.0

    return normalised, {
        "entropy":          round(entropy, 4),
        "max_entropy":      round(max_entropy, 4),
        "distribution":     distribution,
        "entangled_frac":   round(entangled_frac, 4),
    }

# ---------------------------------------------------------------------------
# Dimension 3 — Naming Diversity
# ---------------------------------------------------------------------------

def _tokenise(name: str) -> list[str]:
    """Split preset name into lowercase word tokens (letters only)."""
    import re
    return [w.lower() for w in re.findall(r"[A-Za-z]+", name)]

def score_naming_diversity(presets: list[dict]) -> tuple[float, dict]:
    """unique_words / total_words across all preset names."""
    all_words: list[str] = []
    for p in presets:
        all_words.extend(_tokenise(p["name"]))

    if not all_words:
        return 0.0, {"note": "no name tokens found"}

    unique = len(set(all_words))
    total  = len(all_words)
    ratio  = unique / total

    word_freq = Counter(all_words)
    top_repeated = [(w, c) for w, c in word_freq.most_common(5) if c > 1]

    return ratio, {
        "unique_words":  unique,
        "total_words":   total,
        "ratio":         round(ratio, 4),
        "top_repeated":  top_repeated,
    }

# ---------------------------------------------------------------------------
# Dimension 4 — Engine Coverage
# ---------------------------------------------------------------------------

def score_engine_coverage(presets: list[dict]) -> tuple[float, dict]:
    """Fraction of the 34 known engines represented in the collection."""
    seen: set[str] = set()
    for p in presets:
        for e in p["engines"]:
            seen.add(e.upper())

    known_seen = seen & {e.upper() for e in ALL_ENGINES}
    coverage = len(known_seen) / len(ALL_ENGINES)

    missing = sorted({e for e in ALL_ENGINES if e.upper() not in seen})

    return coverage, {
        "engines_present": sorted(known_seen),
        "engines_missing": missing,
        "n_present":       len(known_seen),
        "n_total":         len(ALL_ENGINES),
        "unknown_engines": sorted(seen - {e.upper() for e in ALL_ENGINES}),
    }

# ---------------------------------------------------------------------------
# Dimension 5 — Parameter Range Utilisation
# ---------------------------------------------------------------------------

def score_param_range(presets: list[dict]) -> tuple[float, dict]:
    """Average fraction of observed range utilised per numeric parameter."""
    param_values: dict[str, list[float]] = defaultdict(list)

    for p in presets:
        for key, val in p["params"].items():
            if isinstance(val, (int, float)):
                param_values[key].append(float(val))

    if not param_values:
        return 0.0, {"note": "no numeric parameters found"}

    fractions = []
    narrow_params = []
    for key, vals in param_values.items():
        if len(vals) < 2:
            continue
        lo, hi = min(vals), max(vals)
        span = hi - lo
        # We don't know the true parameter range; use observed range as proxy.
        # A param that spans its full observed range scores 1.0 by definition.
        # Instead, score based on coefficient of variation as a proxy for spread.
        mean = sum(vals) / len(vals)
        if mean == 0:
            continue
        variance = sum((v - mean) ** 2 for v in vals) / len(vals)
        std = math.sqrt(variance)
        cv = std / abs(mean) if mean != 0 else 0.0
        # Normalise CV to [0, 1]: CV >= 1.0 considered fully utilised
        fraction = min(cv, 1.0)
        fractions.append(fraction)
        if fraction < 0.10:
            narrow_params.append((key, round(fraction, 4), round(lo, 4), round(hi, 4)))

    if not fractions:
        return 0.0, {"note": "insufficient data for range scoring"}

    avg = sum(fractions) / len(fractions)
    narrow_params.sort(key=lambda x: x[1])

    return avg, {
        "n_params_scored": len(fractions),
        "avg_utilisation": round(avg, 4),
        "narrow_params":   narrow_params[:10],
    }

# ---------------------------------------------------------------------------
# Grading helpers
# ---------------------------------------------------------------------------

GRADE_THRESHOLDS = [
    (0.90, "A+"),
    (0.80, "A"),
    (0.70, "B"),
    (0.60, "C"),
    (0.50, "D"),
    (0.00, "F"),
]

def letter_grade(score: float) -> str:
    for threshold, grade in GRADE_THRESHOLDS:
        if score >= threshold:
            return grade
    return "F"

# ---------------------------------------------------------------------------
# Recommendations
# ---------------------------------------------------------------------------

RECOMMENDATIONS = {
    "dna_spread": [
        "Add presets that push extreme DNA values — very bright, very dark, very dense, near-silence.",
        "Run `xpn_dna_interpolator.py` to seed mid-range gaps between DNA poles.",
        "Ensure each engine has at least one preset at DNA extremes (0.0–0.2 and 0.8–1.0 per dim).",
    ],
    "mood_balance": [
        "Audit mood tags with `xpn_preset_mood_rebalancer.py` and redistribute under-represented moods.",
        "Each of the 7 moods should represent roughly 14 % of the collection for perfect balance.",
        "Write 10–15 presets explicitly targeting thin moods (e.g. Family, Prism).",
    ],
    "naming_diversity": [
        "Review top repeated words (shown in detail below) and replace with more evocative synonyms.",
        "Use `rename_weak_presets.py` to bulk-rename formulaic names.",
        "Aim for preset names that use unique adjective–noun pairs rather than engine-name prefixes.",
    ],
    "engine_coverage": [
        "Create at least one preset for every missing engine listed in the detail section.",
        "Use `generate_coupling_presets.py` to seed Entangled presets that cover multiple engines at once.",
        "Install and register any pending engines (OVERLAP, OUTWIT) to expand the available pool.",
    ],
    "param_range": [
        "Identify narrow parameters (shown in detail) and write presets that push them to extremes.",
        "Use `xpn_preset_variation_generator.py` to auto-generate parameter-spread variants.",
        "For macro-driven engines, create dedicated presets for each macro quadrant (min/max combos).",
    ],
}

# ---------------------------------------------------------------------------
# Report formatting
# ---------------------------------------------------------------------------

def format_report(
    presets:        list[dict],
    scores:         dict[str, float],
    details:        dict[str, dict],
    overall:        float,
    weakest_dim:    str,
    engine_filter:  Optional[str],
) -> str:
    lines = []
    sep  = "─" * 60
    sep2 = "═" * 60

    lines.append(sep2)
    lines.append("  XPN PRESET VARIETY SCORER")
    lines.append(sep2)

    filter_note = f"  Engine filter : {engine_filter}" if engine_filter else "  Engine filter : (none — all engines)"
    lines.append(filter_note)
    lines.append(f"  Presets scored: {len(presets)}")
    lines.append(sep2)
    lines.append("")

    lines.append(f"  OVERALL VARIETY SCORE: {overall:.1f} / 100   [{letter_grade(overall / 100)}]")
    lines.append("")
    lines.append(sep)
    lines.append("  DIMENSION BREAKDOWN")
    lines.append(sep)

    dim_labels = {
        "dna_spread":       "DNA Spread        (25%)",
        "mood_balance":     "Mood Balance      (20%)",
        "naming_diversity": "Naming Diversity  (20%)",
        "engine_coverage":  "Engine Coverage   (20%)",
        "param_range":      "Parameter Range   (15%)",
    }

    for dim, label in dim_labels.items():
        raw   = scores[dim]
        grade = letter_grade(raw)
        bar_len = int(raw * 20)
        bar = "█" * bar_len + "░" * (20 - bar_len)
        lines.append(f"  {label}  [{bar}]  {raw * 100:5.1f}  {grade}")

    lines.append("")
    lines.append(sep)
    lines.append("  DETAIL")
    lines.append(sep)

    # DNA Spread
    d = details["dna_spread"]
    lines.append("  DNA Spread:")
    if "note" in d:
        lines.append(f"    {d['note']}")
    else:
        lines.append(f"    Mean pairwise distance : {d['mean_pairwise_distance']:.4f}")
        lines.append(f"    Presets with full DNA  : {d['n_vectors']}  ({d['n_pairs']} pairs)")

    # Mood Balance
    d = details["mood_balance"]
    lines.append("")
    lines.append("  Mood Balance:")
    if "note" in d:
        lines.append(f"    {d['note']}")
    else:
        lines.append(f"    Entropy : {d['entropy']:.4f}  (max {d['max_entropy']:.4f})")
        for mood in MOODS:
            count = d["distribution"].get(mood, 0)
            pct   = count / len(presets) * 100 if presets else 0
            flag  = " ← LOW" if mood == "Entangled" and d["entangled_frac"] < ENTANGLED_WARN_THRESHOLD else ""
            lines.append(f"      {mood:<12} {count:4d}  ({pct:5.1f}%){flag}")
        if d["distribution"].get("Unknown", 0):
            lines.append(f"      {'Unknown':<12} {d['distribution']['Unknown']:4d}")
        if d["entangled_frac"] < ENTANGLED_WARN_THRESHOLD:
            lines.append("")
            lines.append(f"  ⚠  WARNING: Entangled mood is {d['entangled_frac']*100:.1f}% of collection")
            lines.append("     (threshold: 10%). Coupling presets are under-represented.")

    # Naming Diversity
    d = details["naming_diversity"]
    lines.append("")
    lines.append("  Naming Diversity:")
    if "note" in d:
        lines.append(f"    {d['note']}")
    else:
        lines.append(f"    Unique words  : {d['unique_words']}  /  Total words: {d['total_words']}")
        lines.append(f"    Ratio         : {d['ratio']:.4f}")
        if d["top_repeated"]:
            lines.append("    Most repeated words:")
            for word, cnt in d["top_repeated"]:
                lines.append(f"      '{word}' appears {cnt}x")

    # Engine Coverage
    d = details["engine_coverage"]
    lines.append("")
    lines.append("  Engine Coverage:")
    lines.append(f"    Engines present : {d['n_present']} / {d['n_total']}")
    if d["engines_missing"]:
        missing_str = ", ".join(d["engines_missing"])
        lines.append(f"    Missing engines : {missing_str}")
    if d["unknown_engines"]:
        lines.append(f"    Unrecognised    : {', '.join(d['unknown_engines'])}")

    # Parameter Range
    d = details["param_range"]
    lines.append("")
    lines.append("  Parameter Range Utilisation:")
    if "note" in d:
        lines.append(f"    {d['note']}")
    else:
        lines.append(f"    Parameters scored  : {d['n_params_scored']}")
        lines.append(f"    Average utilisation: {d['avg_utilisation']:.4f}")
        if d["narrow_params"]:
            lines.append("    Narrowest parameters (top 10):")
            for pname, frac, lo, hi in d["narrow_params"]:
                lines.append(f"      {pname:<30}  util={frac:.3f}  range=[{lo}, {hi}]")

    # Recommendations
    lines.append("")
    lines.append(sep)
    lines.append(f"  WEAKEST DIMENSION: {weakest_dim.replace('_', ' ').upper()}")
    lines.append("  RECOMMENDATIONS:")
    for rec in RECOMMENDATIONS.get(weakest_dim, []):
        lines.append(f"    • {rec}")

    lines.append("")
    lines.append(sep2)
    return "\n".join(lines)

# ---------------------------------------------------------------------------
# JSON output
# ---------------------------------------------------------------------------

def build_json_output(
    presets:      list[dict],
    scores:       dict[str, float],
    details:      dict[str, dict],
    overall:      float,
    weakest_dim:  str,
    engine_filter: Optional[str],
) -> dict:
    return {
        "summary": {
            "overall_score":   round(overall, 2),
            "overall_grade":   letter_grade(overall / 100),
            "n_presets":       len(presets),
            "engine_filter":   engine_filter,
            "weakest_dim":     weakest_dim,
        },
        "dimensions": {
            dim: {
                "score":     round(scores[dim], 4),
                "score_100": round(scores[dim] * 100, 2),
                "grade":     letter_grade(scores[dim]),
                "weight":    WEIGHTS[dim],
                "detail":    details[dim],
            }
            for dim in WEIGHTS
        },
        "warnings": {
            "low_entangled": (
                details["mood_balance"].get("entangled_frac", 1.0) < ENTANGLED_WARN_THRESHOLD
            )
        },
        "recommendations": RECOMMENDATIONS.get(weakest_dim, []),
    }

# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main() -> None:
    parser = argparse.ArgumentParser(
        description="Score preset variety across 5 dimensions for XO_OX collections.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument(
        "--preset-dir", required=True, type=Path,
        help="Directory to scan recursively for .xometa files.",
    )
    parser.add_argument(
        "--engine", default=None,
        help="Filter to presets that include this engine (e.g. OPAL).",
    )
    parser.add_argument(
        "--output", default=None, type=Path,
        help="Write report text to this file instead of stdout.",
    )
    parser.add_argument(
        "--json", action="store_true",
        help="Output machine-readable JSON instead of human report.",
    )
    args = parser.parse_args()

    preset_dir: Path = args.preset_dir
    if not preset_dir.is_dir():
        print(f"ERROR: {preset_dir} is not a directory.", file=sys.stderr)
        sys.exit(1)

    presets = load_presets(preset_dir, engine_filter=args.engine)
    if not presets:
        print("ERROR: no presets loaded — nothing to score.", file=sys.stderr)
        sys.exit(1)

    # Compute all 5 dimensions
    dna_score,  dna_detail  = score_dna_spread(presets)
    mood_score, mood_detail = score_mood_balance(presets)
    name_score, name_detail = score_naming_diversity(presets)
    eng_score,  eng_detail  = score_engine_coverage(presets)
    prm_score,  prm_detail  = score_param_range(presets)

    scores = {
        "dna_spread":       dna_score,
        "mood_balance":     mood_score,
        "naming_diversity": name_score,
        "engine_coverage":  eng_score,
        "param_range":      prm_score,
    }
    details = {
        "dna_spread":       dna_detail,
        "mood_balance":     mood_detail,
        "naming_diversity": name_detail,
        "engine_coverage":  eng_detail,
        "param_range":      prm_detail,
    }

    overall = sum(scores[dim] * WEIGHTS[dim] for dim in WEIGHTS) * 100
    weakest_dim = min(scores, key=lambda d: scores[d])

    if args.json:
        out = json.dumps(
            build_json_output(presets, scores, details, overall, weakest_dim, args.engine),
            indent=2,
        )
        if args.output:
            args.output.write_text(out, encoding="utf-8")
            print(f"JSON written to {args.output}")
        else:
            print(out)
    else:
        report = format_report(presets, scores, details, overall, weakest_dim, args.engine)
        if args.output:
            args.output.write_text(report, encoding="utf-8")
            print(f"Report written to {args.output}")
        else:
            print(report)


if __name__ == "__main__":
    main()
