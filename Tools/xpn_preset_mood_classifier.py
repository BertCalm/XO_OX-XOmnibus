#!/usr/bin/env python3
"""
xpn_preset_mood_classifier.py — XO_OX XOceanus Preset Mood Classifier

Auto-classifies .xometa presets with "Unknown" or missing mood assignments
by scoring them against 7 mood profiles using 6D Sonic DNA weights.

Usage:
    python xpn_preset_mood_classifier.py <dir> [--dry-run] [--fix] [--threshold 0.7] [--output report.txt]

Moods: Foundation, Atmosphere, Entangled, Prism, Flux, Aether, Family,
       Submerged, Coupling, Crystalline, Deep, Ethereal, Kinetic, Luminous, Organic, Shadow
"""

import json
import argparse
import sys
from pathlib import Path
from typing import Optional

# ---------------------------------------------------------------------------
# Mood Definitions
# Each mood maps DNA dimension names to weights (positive or negative).
# Weights are normalised inside score_mood() so profiles can be authored
# intuitively without summing to 1.
#
# DNA dimensions: brightness, warmth, movement, density, space, aggression
#
# Profile rationale (matches CLAUDE.md + XOceanus master spec):
#   Foundation  — stable/versatile: balanced DNA, penalise extremes
#   Atmosphere  — spatial/textural: high space, low density + aggression
#   Entangled   — coupling: handled by engine-count override (score kept neutral)
#   Prism       — vivid/character: high brightness OR warmth, mid movement
#   Flux        — dynamic/evolving: high movement + density
#   Aether      — ethereal: high space + brightness, low aggression + density
#   Family      — warm/accessible: high warmth, low aggression + movement
#   Shadow      — dark/menacing: low brightness + warmth, high aggression, chromatic anxiety
# ---------------------------------------------------------------------------

MOOD_WEIGHTS: dict[str, dict[str, float]] = {
    "Foundation": {
        "brightness":  0.15,
        "warmth":      0.20,
        "movement":    0.10,
        "density":     0.10,
        "space":       0.15,
        "aggression": -0.30,   # penalise high aggression (not grounded)
        # balance bonus applied separately
    },
    "Atmosphere": {
        "brightness":  0.05,
        "warmth":      0.05,
        "movement":   -0.20,   # low movement preferred
        "density":    -0.25,   # low density preferred
        "space":       0.50,   # high space is the defining trait
        "aggression": -0.30,
    },
    "Entangled": {
        # Mostly handled by engine-count override; keep a neutral profile
        # so lone-engine presets don't accidentally score high.
        "brightness":  0.05,
        "warmth":      0.05,
        "movement":    0.10,
        "density":     0.10,
        "space":       0.05,
        "aggression":  0.00,
    },
    "Prism": {
        "brightness":  0.40,   # vivid colour
        "warmth":      0.30,   # OR warmth
        "movement":    0.15,   # medium movement
        "density":     0.05,
        "space":       0.00,
        "aggression":  0.10,   # slight edge is OK for vivid character
    },
    "Flux": {
        "brightness":  0.05,
        "warmth":      0.05,
        "movement":    0.50,   # defining: high movement
        "density":     0.30,   # dense / layered
        "space":       0.00,
        "aggression":  0.10,
    },
    "Aether": {
        "brightness":  0.30,   # bright / luminous
        "warmth":      0.00,
        "movement":   -0.10,   # slow / still
        "density":    -0.20,   # sparse
        "space":       0.45,   # vast space
        "aggression": -0.35,   # very low aggression
    },
    "Family": {
        "brightness":  0.10,
        "warmth":      0.50,   # warm is the defining trait
        "movement":   -0.15,   # gentle, low movement
        "density":     0.05,
        "space":       0.10,
        "aggression": -0.40,   # low aggression essential
    },
    "Shadow": {
        "brightness": -0.40,   # low brightness defining trait
        "warmth":     -0.30,   # cold
        "movement":    0.10,   # variable movement
        "density":     0.20,   # dense, oppressive
        "space":       0.05,
        "aggression":  0.50,   # high aggression defining trait
    },
}

VALID_MOODS = set(MOOD_WEIGHTS.keys())
TIEBREAK_MARGIN = 0.1   # if top-2 scores differ by < this, suggest both


def score_mood(dna: dict, mood: str) -> float:
    """
    Compute a raw mood score for a given DNA dict.
    DNA values are expected in [0, 1].
    Returns a score in approximately [0, 1] after min-max normalisation
    is applied across all moods externally.
    """
    weights = MOOD_WEIGHTS[mood]
    raw = 0.0
    total_positive = sum(w for w in weights.values() if w > 0)
    if total_positive == 0:
        return 0.0

    for dim, weight in weights.items():
        value = float(dna.get(dim, 0.5))
        if weight >= 0:
            raw += weight * value
        else:
            # Negative weights reward *low* values
            raw += abs(weight) * (1.0 - value)

    # Normalise by sum of abs(weights) so scores are in [0, 1]
    total_weight = sum(abs(w) for w in weights.values())
    return raw / total_weight if total_weight > 0 else 0.0


def classify(dna: dict, engine_count: int) -> "tuple[list, float, str]":
    """
    Returns (suggested_moods, confidence, reasoning).
    suggested_moods may contain 1 or 2 entries (tiebreak).
    """
    # Special rule: multi-engine presets → Entangled
    if engine_count >= 2:
        return (
            ["Entangled"],
            1.0,
            f"Multi-engine preset ({engine_count} engines) — Entangled override applied.",
        )

    scores = {mood: score_mood(dna, mood) for mood in MOOD_WEIGHTS}

    # Exclude Entangled from single-engine scoring
    scores["Entangled"] = 0.0

    sorted_moods = sorted(scores.items(), key=lambda x: x[1], reverse=True)
    top_mood, top_score = sorted_moods[0]
    second_mood, second_score = sorted_moods[1]

    suggestions: list[str]
    if (top_score - second_score) < TIEBREAK_MARGIN and top_score > 0:
        suggestions = [top_mood, second_mood]
        confidence = top_score  # confidence reflects top score
        reasoning = (
            f"Tie: {top_mood} ({top_score:.2f}) vs {second_mood} ({second_score:.2f}). "
            f"DNA: " + _dna_summary(dna)
        )
    else:
        suggestions = [top_mood]
        confidence = top_score
        reasoning = (
            f"Best match: {top_mood} ({top_score:.2f}). Runner-up: {second_mood} ({second_score:.2f}). "
            f"DNA: " + _dna_summary(dna)
        )

    return suggestions, confidence, reasoning


def _dna_summary(dna: dict) -> str:
    parts = [f"{k}={v:.2f}" for k, v in dna.items()]
    return ", ".join(parts)


def load_preset(path: Path) -> Optional[dict]:
    """Load and parse a .xometa file. Returns None on error."""
    try:
        with path.open("r", encoding="utf-8") as f:
            return json.load(f)
    except (json.JSONDecodeError, OSError) as exc:
        return None


def needs_classification(preset: dict) -> bool:
    """True if the preset lacks a mood or has mood = 'Unknown'."""
    mood = preset.get("mood")
    return mood is None or str(mood).strip().lower() in ("unknown", "")


def scan_directory(root: Path) -> list[Path]:
    """Recursively find all .xometa files."""
    return sorted(root.rglob("*.xometa"))


def apply_fix(path: Path, preset: dict, suggested_mood: str) -> bool:
    """Write the suggested mood back to the preset file. Returns True on success."""
    preset["mood"] = suggested_mood
    try:
        with path.open("w", encoding="utf-8") as f:
            json.dump(preset, f, indent=2, ensure_ascii=False)
            f.write("\n")
        return True
    except OSError:
        return False


def build_report(results: list[dict]) -> str:
    """Format the classification results as a human-readable report."""
    lines = [
        "XOceanus Preset Mood Classifier — Report",
        "=" * 60,
        f"Total flagged presets: {len(results)}",
        "",
    ]

    if not results:
        lines.append("No presets require mood classification. All clean.")
        return "\n".join(lines)

    for r in results:
        lines.append(f"Preset : {r['name']}")
        lines.append(f"  File       : {r['path']}")
        lines.append(f"  Current    : {r['current_mood']}")
        lines.append(f"  Suggested  : {' / '.join(r['suggested'])}")
        lines.append(f"  Confidence : {r['confidence']:.2f}")
        lines.append(f"  Reasoning  : {r['reasoning']}")
        if r.get("fixed"):
            lines.append(f"  Action     : FIXED -> {r['applied_mood']}")
        elif r.get("fix_skipped"):
            lines.append(f"  Action     : skipped (confidence {r['confidence']:.2f} < threshold)")
        else:
            lines.append(f"  Action     : dry-run (no changes made)")
        lines.append("")

    # Summary statistics
    fixed = [r for r in results if r.get("fixed")]
    skipped = [r for r in results if r.get("fix_skipped")]
    dry = [r for r in results if not r.get("fixed") and not r.get("fix_skipped")]

    lines += [
        "-" * 60,
        f"Fixed      : {len(fixed)}",
        f"Skipped    : {len(skipped)} (below threshold)",
        f"Dry-run    : {len(dry)}",
    ]
    return "\n".join(lines)


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Classify .xometa presets with Unknown or missing mood assignments.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  python xpn_preset_mood_classifier.py Presets/
  python xpn_preset_mood_classifier.py Presets/ --fix
  python xpn_preset_mood_classifier.py Presets/ --fix --threshold 0.6 --output report.txt
        """,
    )
    parser.add_argument("directory", type=Path, help="Root directory to scan for .xometa files")
    parser.add_argument(
        "--dry-run",
        action="store_true",
        default=True,
        help="Report only, no file changes (default)",
    )
    parser.add_argument(
        "--fix",
        action="store_true",
        default=False,
        help="Apply suggestions in-place for high-confidence classifications",
    )
    parser.add_argument(
        "--threshold",
        type=float,
        default=0.7,
        metavar="FLOAT",
        help="Minimum confidence (0-1) required to auto-fix (default: 0.7)",
    )
    parser.add_argument(
        "--output",
        type=Path,
        default=None,
        metavar="FILE",
        help="Write report to file instead of stdout",
    )
    parser.add_argument(
        "--all",
        action="store_true",
        default=False,
        help="Classify ALL presets regardless of current mood (audit mode)",
    )

    args = parser.parse_args()

    # --fix overrides --dry-run implicitly
    do_fix = args.fix
    threshold = max(0.0, min(1.0, args.threshold))

    root = args.directory.resolve()
    if not root.is_dir():
        print(f"ERROR: '{root}' is not a directory.", file=sys.stderr)
        return 1

    preset_paths = scan_directory(root)
    print(f"Scanned {len(preset_paths)} .xometa files in '{root}'", file=sys.stderr)

    results: list[dict] = []
    parse_errors: list[Path] = []

    for path in preset_paths:
        preset = load_preset(path)
        if preset is None:
            parse_errors.append(path)
            continue

        current_mood = preset.get("mood", None)

        # Skip well-classified presets unless --all mode
        if not args.all and not needs_classification(preset):
            continue

        dna = preset.get("dna") or {}
        engines = preset.get("engines") or []
        engine_count = len(engines)

        suggested, confidence, reasoning = classify(dna, engine_count)

        result: dict = {
            "name": preset.get("name", path.stem),
            "path": str(path),
            "current_mood": current_mood if current_mood else "(missing)",
            "suggested": suggested,
            "confidence": confidence,
            "reasoning": reasoning,
            "fixed": False,
            "fix_skipped": False,
            "applied_mood": None,
        }

        if do_fix:
            if confidence >= threshold and len(suggested) == 1:
                success = apply_fix(path, preset, suggested[0])
                if success:
                    result["fixed"] = True
                    result["applied_mood"] = suggested[0]
                else:
                    print(f"WARNING: Failed to write '{path}'", file=sys.stderr)
            else:
                result["fix_skipped"] = True
        # else: dry-run, no action

        results.append(result)

    report = build_report(results)

    if parse_errors:
        report += "\n\nPARSE ERRORS (files skipped):\n"
        for p in parse_errors:
            report += f"  {p}\n"

    if args.output:
        try:
            args.output.write_text(report, encoding="utf-8")
            print(f"Report written to '{args.output}'", file=sys.stderr)
        except OSError as exc:
            print(f"ERROR: Could not write report: {exc}", file=sys.stderr)
            return 1
    else:
        print(report)

    return 0


if __name__ == "__main__":
    sys.exit(main())
