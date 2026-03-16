#!/usr/bin/env python3
"""
xpn_preset_emotion_wheel.py
Map XO_OX presets onto a Plutchik emotion wheel using 6D Sonic DNA.

CLI:
    python xpn_preset_emotion_wheel.py --preset-dir <dir> [--engine FILTER] [--output report.txt]
"""

import argparse
import json
import math
import sys
from pathlib import Path
from typing import Dict, List, Optional


# ---------------------------------------------------------------------------
# Emotion definitions
# ---------------------------------------------------------------------------

EMOTIONS = [
    "Joy",
    "Trust",
    "Fear",
    "Surprise",
    "Sadness",
    "Disgust",
    "Anger",
    "Anticipation",
]

# Wheel positions in degrees (Plutchik order, evenly spaced)
EMOTION_DEGREES = {e: i * 45 for i, e in enumerate(EMOTIONS)}

# Accent colors (ANSI) for terminal rendering
EMOTION_COLORS = {
    "Joy":          "\033[93m",   # bright yellow
    "Trust":        "\033[92m",   # bright green
    "Fear":         "\033[32m",   # green
    "Surprise":     "\033[96m",   # cyan
    "Sadness":      "\033[94m",   # blue
    "Disgust":      "\033[35m",   # magenta
    "Anger":        "\033[91m",   # bright red
    "Anticipation": "\033[33m",   # orange/yellow
}
RESET = "\033[0m"


def clamp(v: float) -> float:
    return max(0.0, min(1.0, v))


def score_emotions(dna: dict) -> Dict[str, float]:
    """
    Compute a 0-1 score for each Plutchik emotion from 6D Sonic DNA.
    Scores are based on fuzzy weighted combinations of DNA dimensions.
    """
    b = clamp(dna.get("brightness", 0.5))
    w = clamp(dna.get("warmth", 0.5))
    m = clamp(dna.get("movement", 0.5))
    d = clamp(dna.get("density", 0.5))
    s = clamp(dna.get("space", 0.5))
    a = clamp(dna.get("aggression", 0.5))

    inv_a = 1.0 - a
    inv_b = 1.0 - b
    inv_m = 1.0 - m
    inv_w = 1.0 - w
    inv_d = 1.0 - d
    inv_s = 1.0 - s

    # Medium values reward proximity to 0.5
    def medium(v: float) -> float:
        return 1.0 - abs(v - 0.5) * 2.0

    scores = {
        # Joy: high brightness + high movement + low aggression
        "Joy":          (b * 0.4 + m * 0.35 + inv_a * 0.25),
        # Trust: high warmth + medium density + low aggression
        "Trust":        (w * 0.45 + medium(d) * 0.3 + inv_a * 0.25),
        # Fear: high aggression + high movement + low warmth
        "Fear":         (a * 0.4 + m * 0.35 + inv_w * 0.25),
        # Surprise: high movement + high space + low density
        "Surprise":     (m * 0.4 + s * 0.35 + inv_d * 0.25),
        # Sadness: low brightness + low movement + high space
        "Sadness":      (inv_b * 0.4 + inv_m * 0.35 + s * 0.25),
        # Disgust: high aggression + high density + low space
        "Disgust":      (a * 0.4 + d * 0.35 + inv_s * 0.25),
        # Anger: high aggression + high density + high brightness
        "Anger":        (a * 0.4 + d * 0.3 + b * 0.3),
        # Anticipation: high movement + medium brightness + medium aggression
        "Anticipation": (m * 0.4 + medium(b) * 0.3 + medium(a) * 0.3),
    }

    # Normalize so scores sum to 1 for comparability
    total = sum(scores.values())
    if total > 0:
        scores = {k: v / total for k, v in scores.items()}

    return scores


def classify(scores: dict[str, float]) -> str:
    return max(scores, key=lambda k: scores[k])


def is_ambiguous(scores: dict[str, float], threshold: float = 0.05) -> bool:
    """True if the top two emotions are within threshold of each other."""
    sorted_vals = sorted(scores.values(), reverse=True)
    if len(sorted_vals) < 2:
        return False
    return (sorted_vals[0] - sorted_vals[1]) < threshold


# ---------------------------------------------------------------------------
# Loading
# ---------------------------------------------------------------------------

def load_presets(preset_dir: Path, engine_filter: Optional[str]) -> List[dict]:
    presets = []
    for path in sorted(preset_dir.rglob("*.xometa")):
        try:
            data = json.loads(path.read_text(encoding="utf-8"))
        except (json.JSONDecodeError, OSError):
            continue

        dna = data.get("dna")
        if not dna:
            continue

        engines = data.get("engines", [])
        if engine_filter:
            if not any(engine_filter.lower() in e.lower() for e in engines):
                continue

        presets.append({
            "name":    data.get("name", path.stem),
            "mood":    data.get("mood", ""),
            "engines": engines,
            "tags":    data.get("tags", []),
            "dna":     dna,
            "path":    path,
        })

    return presets


# ---------------------------------------------------------------------------
# Analysis
# ---------------------------------------------------------------------------

def analyze(presets: list[dict]) -> dict:
    classified = []
    for p in presets:
        scores = score_emotions(p["dna"])
        emotion = classify(scores)
        ambiguous = is_ambiguous(scores)
        classified.append({**p, "scores": scores, "emotion": emotion, "ambiguous": ambiguous})

    # Distribution
    distribution: dict[str, list] = {e: [] for e in EMOTIONS}
    for p in classified:
        distribution[p["emotion"]].append(p)

    # Emotional center: weighted average of wheel positions
    total_weight = 0.0
    cx = cy = 0.0
    for p in classified:
        for emotion, score in p["scores"].items():
            deg = EMOTION_DEGREES[emotion]
            rad = math.radians(deg)
            cx += score * math.cos(rad)
            cy += score * math.sin(rad)
            total_weight += score

    if total_weight > 0:
        cx /= total_weight
        cy /= total_weight

    center_angle = math.degrees(math.atan2(cy, cx)) % 360
    center_magnitude = math.sqrt(cx ** 2 + cy ** 2)

    # Find nearest named emotion to center angle
    center_emotion = min(EMOTIONS, key=lambda e: abs(((EMOTION_DEGREES[e] - center_angle + 180) % 360) - 180))

    # Outliers: ambiguous presets
    outliers = [p for p in classified if p["ambiguous"]]

    # Per-engine breakdown
    engine_map: dict[str, dict[str, int]] = {}
    for p in classified:
        for eng in p["engines"]:
            if eng not in engine_map:
                engine_map[eng] = {e: 0 for e in EMOTIONS}
            engine_map[eng][p["emotion"]] += 1

    return {
        "classified":       classified,
        "distribution":     distribution,
        "center_angle":     center_angle,
        "center_magnitude": center_magnitude,
        "center_emotion":   center_emotion,
        "outliers":         outliers,
        "engine_map":       engine_map,
    }


# ---------------------------------------------------------------------------
# Rendering
# ---------------------------------------------------------------------------

BAR_WIDTH = 40


def render_bar(count: int, total: int, color: str = "", use_color: bool = True) -> str:
    if total == 0:
        return ""
    filled = round((count / total) * BAR_WIDTH)
    bar = "█" * filled + "░" * (BAR_WIDTH - filled)
    pct = count / total * 100
    if use_color and color:
        return f"{color}{bar}{RESET} {count:4d}  {pct:5.1f}%"
    return f"{bar} {count:4d}  {pct:5.1f}%"


def render_report(result: dict, use_color: bool = True) -> str:
    lines = []
    classified = result["classified"]
    distribution = result["distribution"]
    total = len(classified)

    lines.append("=" * 70)
    lines.append("  PLUTCHIK EMOTION WHEEL — PRESET DISTRIBUTION")
    lines.append("=" * 70)
    lines.append(f"  Total presets analyzed: {total}")
    lines.append("")

    lines.append("  EMOTION DISTRIBUTION")
    lines.append("  " + "-" * 65)
    lines.append(f"  {'Emotion':<14}  {'Bar':<{BAR_WIDTH}}  {'Count':>5}  {'  %':>6}")
    lines.append("  " + "-" * 65)

    for emotion in EMOTIONS:
        count = len(distribution[emotion])
        color = EMOTION_COLORS[emotion] if use_color else ""
        bar_str = render_bar(count, total, color, use_color)
        lines.append(f"  {emotion:<14}  {bar_str}")

    lines.append("")

    # ASCII radial sketch (simplified 2D compass)
    lines.append("  WHEEL COMPASS  (each * = ~1% of collection)")
    lines.append("")
    lines += _render_compass(distribution, total, use_color)
    lines.append("")

    # Emotional center
    center_emotion = result["center_emotion"]
    mag = result["center_magnitude"]
    angle = result["center_angle"]
    center_color = EMOTION_COLORS.get(center_emotion, "") if use_color else ""
    lines.append("  EMOTIONAL CENTER")
    lines.append("  " + "-" * 65)
    lines.append(f"  Dominant pull: {center_color}{center_emotion}{RESET if use_color else ''}")
    lines.append(f"  Wheel angle  : {angle:.1f}°")
    lines.append(f"  Cohesion     : {mag:.3f}  (1.0 = fully concentrated, ~0 = evenly spread)")
    lines.append("")

    # Outliers
    outliers = result["outliers"]
    lines.append(f"  AMBIGUOUS PRESETS ({len(outliers)}) — top two emotions within 5%")
    lines.append("  " + "-" * 65)
    if outliers:
        for p in outliers[:30]:
            top2 = sorted(p["scores"].items(), key=lambda x: -x[1])[:2]
            e1, s1 = top2[0]
            e2, s2 = top2[1]
            c1 = EMOTION_COLORS.get(e1, "") if use_color else ""
            c2 = EMOTION_COLORS.get(e2, "") if use_color else ""
            r = RESET if use_color else ""
            lines.append(
                f"  {p['name']:<40}  "
                f"{c1}{e1}{r} {s1:.3f} / {c2}{e2}{r} {s2:.3f}"
            )
        if len(outliers) > 30:
            lines.append(f"  ... and {len(outliers) - 30} more")
    else:
        lines.append("  None — every preset has a clear dominant emotion.")
    lines.append("")

    return "\n".join(lines)


def _render_compass(distribution: dict, total: int, use_color: bool) -> list[str]:
    """
    Render a crude 15×31 ASCII compass with emotion labels at cardinal points.
    Radius of each spoke is proportional to count.
    """
    if total == 0:
        return ["  (no data)"]

    ROWS, COLS = 15, 61
    grid = [[" "] * COLS for _ in range(ROWS)]

    cx, cy = COLS // 2, ROWS // 2

    # Draw center
    grid[cy][cx] = "+"

    # Max radius (fit within grid)
    max_r = min(cx - 2, cy - 1)

    for emotion in EMOTIONS:
        count = len(distribution[emotion])
        if count == 0:
            continue
        frac = count / total
        r = max(1, round(frac * max_r * 2.5))  # amplify for visibility
        r = min(r, max_r)
        deg = EMOTION_DEGREES[emotion]
        rad = math.radians(deg)

        # Draw spoke dots
        steps = r * 2
        for step in range(1, steps + 1):
            t = step / steps
            gx = round(cx + t * r * math.cos(rad) * 1.8)  # x stretch for terminal
            gy = round(cy - t * r * math.sin(rad))
            if 0 <= gy < ROWS and 0 <= gx < COLS:
                grid[gy][gx] = "·" if step < steps else "●"

        # Label at tip
        lx = round(cx + (r + 2) * math.cos(rad) * 1.8)
        ly = round(cy - (r + 2) * math.sin(rad))
        label = emotion[:3].upper()
        for i, ch in enumerate(label):
            gx = lx + i
            if 0 <= ly < ROWS and 0 <= gx < COLS:
                grid[ly][gx] = ch

    return ["  " + "".join(row) for row in grid]


def render_engine_breakdown(engine_map: dict, use_color: bool = True) -> str:
    lines = []
    lines.append("")
    lines.append("  PER-ENGINE BREAKDOWN")
    lines.append("  " + "-" * 65)

    for engine in sorted(engine_map):
        counts = engine_map[engine]
        eng_total = sum(counts.values())
        if eng_total == 0:
            continue
        lines.append(f"\n  [{engine}]  ({eng_total} presets)")
        for emotion in EMOTIONS:
            c = counts[emotion]
            if c == 0:
                continue
            color = EMOTION_COLORS[emotion] if use_color else ""
            bar = render_bar(c, eng_total, color, use_color)
            lines.append(f"    {emotion:<14}  {bar}")

    return "\n".join(lines)


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def build_parser() -> argparse.ArgumentParser:
    p = argparse.ArgumentParser(
        description="Map XO_OX presets onto a Plutchik emotion wheel using 6D Sonic DNA.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    p.add_argument(
        "--preset-dir",
        required=True,
        type=Path,
        help="Directory to recursively search for .xometa files.",
    )
    p.add_argument(
        "--engine",
        default=None,
        help="Filter to presets that include this engine name (case-insensitive substring).",
    )
    p.add_argument(
        "--output",
        type=Path,
        default=None,
        help="Write plain-text report to this file (no ANSI color).",
    )
    p.add_argument(
        "--no-color",
        action="store_true",
        help="Disable ANSI color in terminal output.",
    )
    p.add_argument(
        "--engine-breakdown",
        action="store_true",
        help="Include per-engine emotion breakdown in the report.",
    )
    return p


def main() -> None:
    parser = build_parser()
    args = parser.parse_args()

    preset_dir: Path = args.preset_dir
    if not preset_dir.is_dir():
        print(f"Error: --preset-dir '{preset_dir}' is not a directory.", file=sys.stderr)
        sys.exit(1)

    use_color = not args.no_color and args.output is None

    print(f"Loading presets from: {preset_dir}", file=sys.stderr)
    presets = load_presets(preset_dir, args.engine)

    if not presets:
        msg = "No .xometa files with DNA found"
        if args.engine:
            msg += f" matching engine filter '{args.engine}'"
        print(msg + ".", file=sys.stderr)
        sys.exit(0)

    print(f"Analyzing {len(presets)} presets...", file=sys.stderr)
    result = analyze(presets)

    report = render_report(result, use_color=use_color)

    if args.engine_breakdown:
        report += render_engine_breakdown(result["engine_map"], use_color=use_color)
        report += "\n"

    print(report)

    if args.output:
        plain_report = render_report(result, use_color=False)
        if args.engine_breakdown:
            plain_report += render_engine_breakdown(result["engine_map"], use_color=False)
            plain_report += "\n"
        args.output.write_text(plain_report, encoding="utf-8")
        print(f"\nReport saved to: {args.output}", file=sys.stderr)


if __name__ == "__main__":
    main()
