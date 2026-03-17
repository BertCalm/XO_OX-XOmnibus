#!/usr/bin/env python3
"""
xpn_mood_arc_designer.py — Mood Arc Designer for XPN packs
XO_OX XOmnibus Tools Suite

Analyzes and optimizes the emotional journey across pad banks in an XPN pack.
Moods: Foundation, Atmosphere, Entangled, Prism, Flux, Aether, Family, Submerged
"""

import json
import argparse
import math
from pathlib import Path
from itertools import combinations
from typing import Optional

# ── Mood definitions ─────────────────────────────────────────────────────────

MOODS = ["Foundation", "Atmosphere", "Entangled", "Prism", "Flux", "Aether", "Family", "Submerged"]

# Grounded moods that feel like "landing" — good arc endings
GROUNDING_MOODS = {"Foundation", "Family"}

# Dissimilarity matrix: higher = more contrast between two moods.
# Symmetric. Values are subjective design weights 0.0–1.0.
DISSIMILARITY = {
    ("Foundation",  "Foundation"):  0.0,
    ("Foundation",  "Atmosphere"):  0.4,
    ("Foundation",  "Entangled"):   0.7,
    ("Foundation",  "Prism"):       0.6,
    ("Foundation",  "Flux"):        0.8,
    ("Foundation",  "Aether"):      0.9,
    ("Foundation",  "Family"):      0.2,
    ("Atmosphere",  "Atmosphere"):  0.0,
    ("Atmosphere",  "Entangled"):   0.5,
    ("Atmosphere",  "Prism"):       0.6,
    ("Atmosphere",  "Flux"):        0.7,
    ("Atmosphere",  "Aether"):      0.5,
    ("Atmosphere",  "Family"):      0.4,
    ("Entangled",   "Entangled"):   0.0,
    ("Entangled",   "Prism"):       0.5,
    ("Entangled",   "Flux"):        0.6,
    ("Entangled",   "Aether"):      0.7,
    ("Entangled",   "Family"):      0.6,
    ("Prism",       "Prism"):       0.0,
    ("Prism",       "Flux"):        0.4,
    ("Prism",       "Aether"):      0.6,
    ("Prism",       "Family"):      0.5,
    ("Flux",        "Flux"):        0.0,
    ("Flux",        "Aether"):      0.4,
    ("Flux",        "Family"):      0.7,
    ("Aether",      "Aether"):      0.0,
    ("Aether",      "Family"):      0.8,
    ("Family",      "Family"):      0.0,
    ("Foundation",  "Submerged"):   0.7,
    ("Atmosphere",  "Submerged"):   0.3,
    ("Entangled",   "Submerged"):   0.4,
    ("Prism",       "Submerged"):   0.6,
    ("Flux",        "Submerged"):   0.5,
    ("Aether",      "Submerged"):   0.3,
    ("Family",      "Submerged"):   0.7,
    ("Submerged",   "Submerged"):   0.0,
}

def dissimilarity(a: str, b: str) -> float:
    key = (a, b) if (a, b) in DISSIMILARITY else (b, a)
    return DISSIMILARITY.get(key, 0.5)


# ── Scoring ───────────────────────────────────────────────────────────────────

def score_variety(moods: list[str]) -> float:
    """Fraction of the 7 moods that appear at least once."""
    if not moods:
        return 0.0
    unique = len(set(moods))
    return unique / len(MOODS)


def score_contrast(moods: list[str]) -> float:
    """Mean dissimilarity between consecutive pads. Range 0–1."""
    if len(moods) < 2:
        return 0.0
    pairs = [dissimilarity(moods[i], moods[i + 1]) for i in range(len(moods) - 1)]
    return sum(pairs) / len(pairs)


def score_resolution(moods: list[str]) -> float:
    """1.0 if last mood is grounding, 0.5 if second-to-last is, else 0.0."""
    if not moods:
        return 0.0
    if moods[-1] in GROUNDING_MOODS:
        return 1.0
    if len(moods) >= 2 and moods[-2] in GROUNDING_MOODS:
        return 0.5
    return 0.0


def composite_score(moods: list[str], w_variety=0.3, w_contrast=0.5, w_resolution=0.2) -> float:
    v = score_variety(moods)
    c = score_contrast(moods)
    r = score_resolution(moods)
    return w_variety * v + w_contrast * c + w_resolution * r


def score_summary(moods: list[str]) -> dict:
    return {
        "variety":    round(score_variety(moods), 4),
        "contrast":   round(score_contrast(moods), 4),
        "resolution": round(score_resolution(moods), 4),
        "composite":  round(composite_score(moods), 4),
        "unique_moods": sorted(set(moods)),
    }


# ── Anti-pattern detection ────────────────────────────────────────────────────

PADS_PER_BANK = 16  # MPC standard bank size

def detect_antipatterns(programs: list[dict]) -> list[str]:
    warnings = []
    moods = [p["mood"] for p in programs]

    # 1. Mood clustering: 3+ same mood in a row
    run_mood, run_len, run_start = None, 0, 0
    for i, m in enumerate(moods):
        if m == run_mood:
            run_len += 1
            if run_len == 3:
                warnings.append(
                    f"MOOD_CLUSTER: '{m}' appears 3+ times in a row "
                    f"starting at pad {run_start + 1} ({programs[run_start]['program_name']})"
                )
        else:
            run_mood, run_len, run_start = m, 1, i

    # 2. Mood desert: >6 consecutive pads without Entangled
    desert_len = 0
    desert_start = None
    for i, m in enumerate(moods):
        if m != "Entangled":
            if desert_len == 0:
                desert_start = i
            desert_len += 1
            if desert_len == 7:
                warnings.append(
                    f"MOOD_DESERT: No 'Entangled' for {desert_len}+ pads "
                    f"starting at pad {desert_start + 1} ({programs[desert_start]['program_name']})"
                )
        else:
            desert_len = 0
            desert_start = None

    # 3. Aether pile: >4 Aether pads in the same bank
    n_banks = math.ceil(len(programs) / PADS_PER_BANK)
    for bank_idx in range(n_banks):
        bank_label = chr(ord("A") + bank_idx)
        start = bank_idx * PADS_PER_BANK
        end = min(start + PADS_PER_BANK, len(programs))
        bank_moods = [programs[i]["mood"] for i in range(start, end)]
        aether_count = bank_moods.count("Aether")
        if aether_count > 4:
            warnings.append(
                f"AETHER_PILE: Bank {bank_label} has {aether_count} Aether pads (max recommended: 4)"
            )

    return warnings


# ── Greedy swap optimizer ─────────────────────────────────────────────────────

def greedy_optimize(programs: list[dict], max_passes: int = 20) -> list[dict]:
    """
    Greedy pairwise swap: repeatedly find the swap of any two programs that
    most improves composite contrast score, until no improvement is found
    or max_passes is exhausted.
    Returns a new ordered list (does not mutate input).
    """
    current = list(programs)
    current_moods = [p["mood"] for p in current]
    current_score = composite_score(current_moods)

    for _ in range(max_passes):
        best_gain = 0.0
        best_i, best_j = -1, -1

        for i, j in combinations(range(len(current)), 2):
            candidate = list(current)
            candidate[i], candidate[j] = candidate[j], candidate[i]
            candidate_moods = [p["mood"] for p in candidate]
            s = composite_score(candidate_moods)
            gain = s - current_score
            if gain > best_gain:
                best_gain = gain
                best_i, best_j = i, j

        if best_i == -1:
            break  # No improving swap found

        current[best_i], current[best_j] = current[best_j], current[best_i]
        current_moods = [p["mood"] for p in current]
        current_score = composite_score(current_moods)

    return current


# ── Formatting helpers ────────────────────────────────────────────────────────

def pad_label(idx: int) -> str:
    bank = chr(ord("A") + idx // PADS_PER_BANK)
    pad_num = (idx % PADS_PER_BANK) + 1
    return f"{bank}{pad_num:02d}"


def format_arc(programs: list[dict]) -> str:
    lines = []
    for i, p in enumerate(programs):
        label = pad_label(i)
        mood = p.get("mood", "?")
        name = p.get("program_name", "")
        preset = p.get("preset_name", "")
        centroid = p.get("dna_centroid", "")
        centroid_str = f"  [{centroid}]" if centroid else ""
        lines.append(f"  {label}  {mood:<12}  {name}  /  {preset}{centroid_str}")
    return "\n".join(lines)


def format_scores(s: dict) -> str:
    lines = [
        f"  Variety:    {s['variety']:.3f}  ({len(s['unique_moods'])}/7 moods — {', '.join(s['unique_moods'])})",
        f"  Contrast:   {s['contrast']:.3f}",
        f"  Resolution: {s['resolution']:.3f}",
        f"  Composite:  {s['composite']:.3f}",
    ]
    return "\n".join(lines)


def build_report(
    programs: list[dict],
    optimized: Optional[list] = None,
) -> str:
    sections = []

    # Header
    sections.append("=" * 72)
    sections.append("XPN MOOD ARC DESIGNER — XO_OX XOmnibus Tools")
    sections.append("=" * 72)

    # Original arc
    sections.append(f"\nPACK: {len(programs)} programs across "
                    f"{math.ceil(len(programs) / PADS_PER_BANK)} bank(s)\n")
    sections.append("ORIGINAL ARC")
    sections.append("-" * 40)
    sections.append(format_arc(programs))

    orig_moods = [p["mood"] for p in programs]
    orig_scores = score_summary(orig_moods)
    sections.append("\nSCORES (original)")
    sections.append(format_scores(orig_scores))

    # Anti-patterns
    warnings = detect_antipatterns(programs)
    sections.append("\nANTI-PATTERN ANALYSIS")
    sections.append("-" * 40)
    if warnings:
        for w in warnings:
            sections.append(f"  ⚠  {w}")
    else:
        sections.append("  No anti-patterns detected.")

    # Optimized arc
    if optimized is not None:
        opt_moods = [p["mood"] for p in optimized]
        opt_scores = score_summary(opt_moods)
        improvement = opt_scores["composite"] - orig_scores["composite"]

        sections.append("\nSUGGESTED REORDERING (greedy swap optimizer)")
        sections.append("-" * 40)
        sections.append(format_arc(optimized))

        sections.append("\nSCORES (optimized)")
        sections.append(format_scores(opt_scores))

        sections.append(f"\nSCORE IMPROVEMENT: {improvement:+.4f} composite "
                        f"({orig_scores['composite']:.4f} → {opt_scores['composite']:.4f})")

        opt_warnings = detect_antipatterns(optimized)
        sections.append("\nANTI-PATTERN ANALYSIS (optimized)")
        sections.append("-" * 40)
        if opt_warnings:
            for w in opt_warnings:
                sections.append(f"  ⚠  {w}")
        else:
            sections.append("  No anti-patterns detected.")

    sections.append("\n" + "=" * 72)
    return "\n".join(sections)


# ── Validation ────────────────────────────────────────────────────────────────

def validate_program(p: dict, idx: int) -> list[str]:
    errors = []
    for field in ("program_name", "preset_name", "mood"):
        if field not in p:
            errors.append(f"Program {idx + 1}: missing field '{field}'")
    mood = p.get("mood", "")
    if mood and mood not in MOODS:
        errors.append(
            f"Program {idx + 1} ({p.get('program_name', '?')}): "
            f"unknown mood '{mood}'. Valid: {', '.join(MOODS)}"
        )
    return errors


def load_spec(path: Path) -> list[dict]:
    with path.open() as f:
        data = json.load(f)

    # Accept either {"programs": [...]} or bare list
    if isinstance(data, list):
        programs = data
    elif isinstance(data, dict) and "programs" in data:
        programs = data["programs"]
    else:
        raise ValueError("Spec JSON must be a list of programs or {'programs': [...]}")

    all_errors = []
    for i, p in enumerate(programs):
        all_errors.extend(validate_program(p, i))

    if all_errors:
        raise ValueError("Spec validation errors:\n" + "\n".join(f"  {e}" for e in all_errors))

    return programs


# ── CLI entry point ───────────────────────────────────────────────────────────

def main():
    parser = argparse.ArgumentParser(
        description="XPN Mood Arc Designer — analyze and optimize XPN pack emotional journeys",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  python xpn_mood_arc_designer.py --spec my_pack.json
  python xpn_mood_arc_designer.py --spec my_pack.json --optimize
  python xpn_mood_arc_designer.py --spec my_pack.json --optimize --output arc_report.txt

Spec JSON format:
  {
    "programs": [
      {
        "program_name": "DRIFT A1",
        "preset_name":  "Fog Pulse",
        "mood":         "Atmosphere",
        "dna_centroid": "pad:60 vel:85 atk:slow rel:long"
      },
      ...
    ]
  }

Valid moods: Foundation, Atmosphere, Entangled, Prism, Flux, Aether, Family
""",
    )
    parser.add_argument("--spec", required=True, type=Path, help="Path to pack spec JSON")
    parser.add_argument(
        "--optimize", action="store_true",
        help="Run greedy swap optimizer and suggest improved ordering"
    )
    parser.add_argument(
        "--output", type=Path, default=None,
        help="Write report to file instead of (or in addition to) stdout"
    )
    args = parser.parse_args()

    if not args.spec.exists():
        parser.error(f"Spec file not found: {args.spec}")

    try:
        programs = load_spec(args.spec)
    except (json.JSONDecodeError, ValueError) as e:
        parser.error(str(e))

    optimized = greedy_optimize(programs) if args.optimize else None
    report = build_report(programs, optimized)

    print(report)

    if args.output:
        args.output.write_text(report)
        print(f"\nReport written to: {args.output}")


if __name__ == "__main__":
    main()
