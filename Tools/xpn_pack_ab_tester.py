#!/usr/bin/env python3
"""
XPN Pack A/B Tester — XO_OX Designs
Compares two pack directories across 6 quality dimensions to determine
which version is better suited for release.

Dimensions (weights):
  1. Naming quality      20% — no jargon, 2-3 words, max 30 chars
  2. DNA variety         25% — mean pairwise Euclidean distance in 6D DNA space
  3. Mood balance        20% — Shannon entropy of mood distribution
  4. Engine coverage     15% — unique engine count
  5. Coupling density    15% — % of presets with Entangled mood
  6. Preset count         5% — raw count (diminishing returns above 150)

Exit codes:
  0 = Pack A wins
  1 = Pack B wins
  2 = Too close to call (within 5% of weighted total)

Usage:
    python3 xpn_pack_ab_tester.py <pack_a_dir> <pack_b_dir> [--format text|json] [--output FILE]
"""

import argparse
import json
import math
import sys
from pathlib import Path

# ---------------------------------------------------------------------------
# Constants
# ---------------------------------------------------------------------------

DNA_DIMS = ["brightness", "warmth", "movement", "density", "space", "aggression"]

JARGON_WORDS = {
    "osc", "lfo", "env", "adsr", "vco", "vcf", "vca", "fm", "am", "pm",
    "lpf", "hpf", "bpf", "eq", "cutoff", "resonance", "detune", "glide",
    "portamento", "arp", "midi", "bpm", "hz", "khz", "db", "pan",
    "reverb", "delay", "chorus", "flanger", "phaser", "distortion",
    "compressor", "limiter", "gate", "sidechain", "wet", "dry",
    "poly", "mono", "unison", "voice", "patch", "preset",
}

WEIGHTS = {
    "naming":    0.20,
    "dna":       0.25,
    "mood":      0.20,
    "engines":   0.15,
    "coupling":  0.15,
    "count":     0.05,
}

CLOSE_THRESHOLD = 0.05  # within 5% = too close to call

# ---------------------------------------------------------------------------
# Preset loading
# ---------------------------------------------------------------------------

def load_presets(pack_dir: Path) -> list[dict]:
    """Load all .xometa files from a pack directory (recursive)."""
    presets = []
    for path in sorted(pack_dir.rglob("*.xometa")):
        try:
            with open(path, "r", encoding="utf-8") as f:
                data = json.load(f)
            data["_path"] = str(path)
            presets.append(data)
        except (json.JSONDecodeError, OSError):
            pass  # skip malformed files
    return presets

# ---------------------------------------------------------------------------
# Dimension 1: Naming quality
# ---------------------------------------------------------------------------

def score_preset_name(name: str) -> tuple[float, list[str]]:
    """
    Score a single preset name 0-100.
    Deductions:
      -30 if > 30 chars
      -20 per jargon word found (max -40)
      -20 if word count < 2 or > 3
    Returns (score, list_of_issues).
    """
    issues = []
    score = 100.0

    if len(name) > 30:
        score -= 30
        issues.append(f"too long ({len(name)} chars)")

    words = name.lower().split()
    jargon_hits = [w for w in words if w in JARGON_WORDS]
    if jargon_hits:
        deduct = min(40, 20 * len(jargon_hits))
        score -= deduct
        issues.append(f"jargon: {', '.join(jargon_hits)}")

    if len(words) < 2:
        score -= 20
        issues.append("too short (< 2 words)")
    elif len(words) > 3:
        score -= 20
        issues.append(f"too many words ({len(words)})")

    return max(0.0, score), issues


def dim_naming(presets: list[dict]) -> tuple[float, dict]:
    """Score 0-100. Returns (score, detail)."""
    if not presets:
        return 0.0, {"error": "no presets"}

    scores = []
    failures = []
    for p in presets:
        name = p.get("name", "")
        s, issues = score_preset_name(name)
        scores.append(s)
        if issues:
            failures.append({"name": name, "issues": issues, "score": s})

    mean = sum(scores) / len(scores)
    return mean, {
        "mean_score": round(mean, 2),
        "preset_count": len(presets),
        "failing_count": len(failures),
        "worst": sorted(failures, key=lambda x: x["score"])[:5],
    }

# ---------------------------------------------------------------------------
# Dimension 2: DNA variety
# ---------------------------------------------------------------------------

def dna_vector(preset: dict):
    dna = preset.get("dna")
    if not isinstance(dna, dict):
        return None
    vec = [dna.get(d, 0.0) for d in DNA_DIMS]
    return vec


def euclidean(a: list[float], b: list[float]) -> float:
    return math.sqrt(sum((x - y) ** 2 for x, y in zip(a, b)))


def dim_dna_variety(presets: list[dict]) -> tuple[float, dict]:
    """Mean pairwise distance, normalised to 0-100 (max possible ≈ sqrt(6) ≈ 2.449)."""
    vectors = [v for p in presets if (v := dna_vector(p)) is not None]
    n = len(vectors)
    if n < 2:
        return 0.0, {"error": "insufficient DNA data", "vectors_found": n}

    total = 0.0
    pairs = 0
    # Sample up to 500 pairs for performance on large packs
    import random
    random.seed(42)
    indices = list(range(n))
    if n > 32:
        sample_size = min(500, n * (n - 1) // 2)
        sampled = [(random.randrange(n), random.randrange(n)) for _ in range(sample_size * 2)]
        sampled = [(i, j) for i, j in sampled if i != j][:sample_size]
    else:
        sampled = [(i, j) for i in range(n) for j in range(i + 1, n)]

    for i, j in sampled:
        total += euclidean(vectors[i], vectors[j])
        pairs += 1

    mean_dist = total / pairs if pairs else 0.0
    max_dist = math.sqrt(len(DNA_DIMS))  # ≈ 2.449
    score = min(100.0, (mean_dist / max_dist) * 100.0)

    return score, {
        "mean_pairwise_distance": round(mean_dist, 4),
        "vectors_found": n,
        "pairs_sampled": pairs,
        "score": round(score, 2),
    }

# ---------------------------------------------------------------------------
# Dimension 3: Mood balance (Shannon entropy)
# ---------------------------------------------------------------------------

KNOWN_MOODS = {"Foundation", "Atmosphere", "Entangled", "Prism", "Flux", "Aether", "Family",
               "Submerged", "Coupling", "Crystalline", "Deep", "Ethereal", "Kinetic", "Luminous",
               "Organic", "Shadow"}


def dim_mood_balance(presets: list[dict]) -> tuple[float, dict]:
    """Shannon entropy of mood distribution, normalised to 0-100 (max = log2(7) ≈ 2.807)."""
    from collections import Counter
    counts = Counter(p.get("mood", "Unknown") for p in presets)
    total = sum(counts.values())
    if total == 0:
        return 0.0, {"error": "no presets"}

    entropy = 0.0
    for c in counts.values():
        p = c / total
        if p > 0:
            entropy -= p * math.log2(p)

    max_entropy = math.log2(len(KNOWN_MOODS))  # log2(7) ≈ 2.807
    score = min(100.0, (entropy / max_entropy) * 100.0)

    return score, {
        "entropy": round(entropy, 4),
        "distribution": dict(counts),
        "score": round(score, 2),
    }

# ---------------------------------------------------------------------------
# Dimension 4: Engine coverage
# ---------------------------------------------------------------------------

def dim_engine_coverage(presets: list[dict]) -> tuple[float, dict]:
    """Unique engine count, normalised to 0-100 (31 registered engines = 100)."""
    MAX_ENGINES = 31
    engines: set[str] = set()
    for p in presets:
        for e in p.get("engines", []):
            engines.add(e)

    score = min(100.0, (len(engines) / MAX_ENGINES) * 100.0)
    return score, {
        "unique_engines": len(engines),
        "engine_list": sorted(engines),
        "score": round(score, 2),
    }

# ---------------------------------------------------------------------------
# Dimension 5: Coupling density
# ---------------------------------------------------------------------------

def dim_coupling_density(presets: list[dict]) -> tuple[float, dict]:
    """
    % of presets that are Entangled mood.
    Sweet spot is 20-35%. Score peaks at 27.5%, falls off on either side.
    Uses a tent function centred at 0.275 with width 0.15 each side.
    Score = 100 at peak, 0 at 0% or 50%+.
    """
    if not presets:
        return 0.0, {"error": "no presets"}

    entangled = sum(1 for p in presets if p.get("mood") == "Entangled")
    ratio = entangled / len(presets)

    PEAK = 0.275
    WIDTH = 0.175
    distance = abs(ratio - PEAK)
    score = max(0.0, 100.0 * (1.0 - distance / WIDTH))

    return score, {
        "entangled_count": entangled,
        "total_count": len(presets),
        "ratio": round(ratio, 4),
        "score": round(score, 2),
    }

# ---------------------------------------------------------------------------
# Dimension 6: Preset count
# ---------------------------------------------------------------------------

def dim_preset_count(presets: list[dict]) -> tuple[float, dict]:
    """
    Score based on count. Target: 100-150. Diminishing returns above 150.
    < 50 = low, 100-150 = ideal (100 pts), > 200 = capped at 85.
    """
    n = len(presets)
    if n == 0:
        score = 0.0
    elif n < 50:
        score = (n / 50) * 60.0
    elif n < 100:
        score = 60.0 + ((n - 50) / 50) * 40.0
    elif n <= 150:
        score = 100.0
    else:
        # Diminishing returns: each preset above 150 adds less
        excess = n - 150
        score = max(60.0, 100.0 - (excess / 100.0) * 40.0)

    note = ""
    if n > 150:
        note = f"Diminishing returns above 150 (you have {n})"
    elif n < 100:
        note = f"Below target of 100+ presets"

    return score, {
        "count": n,
        "score": round(score, 2),
        "note": note,
    }

# ---------------------------------------------------------------------------
# Comparison engine
# ---------------------------------------------------------------------------

DIMENSION_LABELS = {
    "naming":   "Naming Quality",
    "dna":      "DNA Variety",
    "mood":     "Mood Balance",
    "engines":  "Engine Coverage",
    "coupling": "Coupling Density",
    "count":    "Preset Count",
}

DIM_FNS = {
    "naming":   dim_naming,
    "dna":      dim_dna_variety,
    "mood":     dim_mood_balance,
    "engines":  dim_engine_coverage,
    "coupling": dim_coupling_density,
    "count":    dim_preset_count,
}


def run_comparison(pack_a: Path, pack_b: Path) -> dict:
    presets_a = load_presets(pack_a)
    presets_b = load_presets(pack_b)

    dimensions = {}
    weighted_a = 0.0
    weighted_b = 0.0

    for key, fn in DIM_FNS.items():
        score_a, detail_a = fn(presets_a)
        score_b, detail_b = fn(presets_b)
        weight = WEIGHTS[key]

        if score_a > score_b:
            winner = "A"
        elif score_b > score_a:
            winner = "B"
        else:
            winner = "tie"

        dimensions[key] = {
            "label":    DIMENSION_LABELS[key],
            "weight":   weight,
            "score_a":  round(score_a, 2),
            "score_b":  round(score_b, 2),
            "winner":   winner,
            "detail_a": detail_a,
            "detail_b": detail_b,
        }

        weighted_a += score_a * weight
        weighted_b += score_b * weight

    weighted_a = round(weighted_a, 4)
    weighted_b = round(weighted_b, 4)
    diff = abs(weighted_a - weighted_b)

    if diff / max(weighted_a, weighted_b, 1) <= CLOSE_THRESHOLD:
        verdict = "too_close"
        exit_code = 2
    elif weighted_a > weighted_b:
        verdict = "A"
        exit_code = 0
    else:
        verdict = "B"
        exit_code = 1

    return {
        "pack_a": str(pack_a),
        "pack_b": str(pack_b),
        "preset_counts": {"a": len(presets_a), "b": len(presets_b)},
        "dimensions": dimensions,
        "weighted_total": {"a": weighted_a, "b": weighted_b},
        "verdict": verdict,
        "exit_code": exit_code,
    }

# ---------------------------------------------------------------------------
# Formatting
# ---------------------------------------------------------------------------

def format_text(result: dict) -> str:
    lines = []
    sep = "=" * 64

    lines.append(sep)
    lines.append("  XPN PACK A/B COMPARISON REPORT")
    lines.append(sep)
    lines.append(f"  Pack A: {result['pack_a']}")
    lines.append(f"  Pack B: {result['pack_b']}")
    lines.append(f"  Presets — A: {result['preset_counts']['a']}  |  B: {result['preset_counts']['b']}")
    lines.append(sep)
    lines.append("")

    for key, dim in result["dimensions"].items():
        w_pct = int(dim["weight"] * 100)
        lines.append(f"  {dim['label'].upper()}  (weight {w_pct}%)")
        lines.append(f"    Pack A: {dim['score_a']:6.1f} / 100")
        lines.append(f"    Pack B: {dim['score_b']:6.1f} / 100")
        lines.append(f"    Winner: {dim['winner']}")

        # Dimension-specific notes
        da = dim["detail_a"]
        db = dim["detail_b"]
        if key == "naming":
            lines.append(f"    Failing presets — A: {da.get('failing_count', '?')}  |  B: {db.get('failing_count', '?')}")
        elif key == "dna":
            lines.append(f"    Mean dist — A: {da.get('mean_pairwise_distance', '?')}  |  B: {db.get('mean_pairwise_distance', '?')}")
        elif key == "mood":
            lines.append(f"    Entropy — A: {da.get('entropy', '?')}  |  B: {db.get('entropy', '?')}")
        elif key == "engines":
            lines.append(f"    Unique engines — A: {da.get('unique_engines', '?')}  |  B: {db.get('unique_engines', '?')}")
        elif key == "coupling":
            lines.append(f"    Entangled ratio — A: {da.get('ratio', '?')}  |  B: {db.get('ratio', '?')}")
        elif key == "count":
            if da.get("note"):
                lines.append(f"    A note: {da['note']}")
            if db.get("note"):
                lines.append(f"    B note: {db['note']}")
        lines.append("")

    lines.append(sep)
    wt = result["weighted_total"]
    lines.append(f"  WEIGHTED TOTAL — A: {wt['a']:.2f}  |  B: {wt['b']:.2f}")
    verdict = result["verdict"]
    if verdict == "too_close":
        lines.append("  VERDICT: TOO CLOSE TO CALL (within 5%)")
    else:
        lines.append(f"  VERDICT: PACK {verdict} WINS")
    lines.append(sep)

    return "\n".join(lines)

# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def main():
    parser = argparse.ArgumentParser(
        description="A/B test two XO_OX preset pack directories across 6 quality dimensions."
    )
    parser.add_argument("pack_a", type=Path, help="Path to Pack A directory")
    parser.add_argument("pack_b", type=Path, help="Path to Pack B directory")
    parser.add_argument(
        "--format", choices=["text", "json"], default="text",
        help="Output format (default: text)"
    )
    parser.add_argument(
        "--output", type=Path, default=None,
        help="Write report to file instead of stdout"
    )
    args = parser.parse_args()

    if not args.pack_a.is_dir():
        print(f"ERROR: Pack A directory not found: {args.pack_a}", file=sys.stderr)
        sys.exit(3)
    if not args.pack_b.is_dir():
        print(f"ERROR: Pack B directory not found: {args.pack_b}", file=sys.stderr)
        sys.exit(3)

    result = run_comparison(args.pack_a, args.pack_b)

    if args.format == "json":
        output = json.dumps(result, indent=2)
    else:
        output = format_text(result)

    if args.output:
        args.output.write_text(output, encoding="utf-8")
        print(f"Report written to {args.output}")
    else:
        print(output)

    sys.exit(result["exit_code"])


if __name__ == "__main__":
    main()
