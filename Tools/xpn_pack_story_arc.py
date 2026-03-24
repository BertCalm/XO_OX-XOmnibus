#!/usr/bin/env python3
"""
XOlokun Pack Story Arc Validator

Validates the narrative journey of a pack by analysing how preset DNA values
evolve across a 4-act structure (Setup → Development → Climax → Resolution).
Scores coherence, detects arc problems, suggests reordering, and renders an
ASCII arc plot.

Usage:
    python xpn_pack_story_arc.py --preset-dir <dir> [--optimize] [--output arc.txt]
"""

import argparse
import json
import math
import sys
from pathlib import Path

# ---------------------------------------------------------------------------
# Constants
# ---------------------------------------------------------------------------

DIMS = ["brightness", "warmth", "movement", "density", "space", "aggression"]

ACT_NAMES = ["Act 1 — Setup", "Act 2 — Development", "Act 3 — Climax", "Act 4 — Resolution"]
ACT_SHORT = ["Setup", "Develop", "Climax", "Resolve"]

# Expected DNA profile per act.  Values are (ideal, weight).
# weight controls how much each dimension contributes to the act score.
ACT_TARGETS: list[dict[str, tuple[float, float]]] = [
    # Act 1: accessible, clear character, low complexity
    {
        "brightness": (0.55, 1.0),
        "warmth":     (0.60, 1.0),
        "movement":   (0.25, 1.5),
        "density":    (0.35, 1.0),
        "space":      (0.50, 0.8),
        "aggression": (0.20, 1.5),
    },
    # Act 2: rising movement, varied brightness
    {
        "brightness": (0.55, 0.6),
        "warmth":     (0.50, 0.6),
        "movement":   (0.55, 1.5),
        "density":    (0.50, 1.0),
        "space":      (0.45, 0.7),
        "aggression": (0.45, 1.2),
    },
    # Act 3: high movement OR aggression, peak density
    {
        "brightness": (0.60, 0.7),
        "warmth":     (0.45, 0.6),
        "movement":   (0.75, 1.5),
        "density":    (0.70, 1.2),
        "space":      (0.40, 0.7),
        "aggression": (0.70, 1.5),
    },
    # Act 4: low aggression, high space or warmth (resolution)
    {
        "brightness": (0.45, 0.8),
        "warmth":     (0.65, 1.2),
        "movement":   (0.25, 1.2),
        "density":    (0.35, 1.0),
        "space":      (0.65, 1.5),
        "aggression": (0.20, 1.5),
    },
]

# ---------------------------------------------------------------------------
# Loading
# ---------------------------------------------------------------------------

def load_presets(preset_dir: Path) -> list[dict]:
    """Load .xometa files sorted by filename. Skip files without valid DNA."""
    files = sorted(preset_dir.glob("*.xometa"))
    if not files:
        # Try one level deep (e.g. mood sub-dirs)
        files = sorted(preset_dir.rglob("*.xometa"))

    presets = []
    skipped = 0
    for path in files:
        try:
            data = json.loads(path.read_text(encoding="utf-8"))
        except (json.JSONDecodeError, OSError) as exc:
            print(f"  WARN: could not parse {path.name}: {exc}", file=sys.stderr)
            skipped += 1
            continue

        dna = data.get("dna") or data.get("sonic_dna")
        if not dna or not all(d in dna for d in DIMS):
            skipped += 1
            continue

        presets.append({
            "name": data.get("name", path.stem),
            "path": path,
            "dna":  {d: float(dna[d]) for d in DIMS},
        })

    if skipped:
        print(f"  INFO: skipped {skipped} file(s) with missing/invalid DNA", file=sys.stderr)
    return presets


# ---------------------------------------------------------------------------
# Act assignment
# ---------------------------------------------------------------------------

def assign_acts(presets: list[dict]) -> list[list[dict]]:
    """Split preset list into 4 acts by position (25% each)."""
    n = len(presets)
    acts: list[list[dict]] = [[], [], [], []]
    for i, p in enumerate(presets):
        ratio = i / n
        act_idx = min(int(ratio * 4), 3)
        acts[act_idx].append(p)
    return acts


# ---------------------------------------------------------------------------
# DNA averaging
# ---------------------------------------------------------------------------

def avg_dna(presets: list[dict]) -> dict[str, float]:
    if not presets:
        return {d: 0.0 for d in DIMS}
    result = {}
    for dim in DIMS:
        result[dim] = sum(p["dna"][dim] for p in presets) / len(presets)
    return result


# ---------------------------------------------------------------------------
# Scoring
# ---------------------------------------------------------------------------

def act_score(avg: dict[str, float], target: dict[str, tuple[float, float]]) -> float:
    """Return 0–100 score for how closely avg DNA matches the act target."""
    total_weight = sum(w for _, w in target.values())
    weighted_error = 0.0
    for dim in DIMS:
        ideal, weight = target[dim]
        error = abs(avg[dim] - ideal)
        weighted_error += weight * error
    # max possible weighted error if all dims are at worst distance (1.0)
    max_error = total_weight * 1.0
    normalised = weighted_error / max_error
    return round((1.0 - normalised) * 100, 1)


def narrative_coherence(act_avgs: list[dict[str, float]]) -> float:
    """
    Compute 0–100 coherence score across all 4 acts.
    Average of per-act scores weighted toward acts 1, 3, 4 (most
    narratively critical).
    """
    weights = [1.2, 0.8, 1.2, 1.2]
    total_w = sum(weights)
    score = 0.0
    for i, (avg, w) in enumerate(zip(act_avgs, weights)):
        score += w * act_score(avg, ACT_TARGETS[i])
    return round(score / total_w, 1)


# ---------------------------------------------------------------------------
# Arc problem detection
# ---------------------------------------------------------------------------

def detect_problems(act_avgs: list[dict[str, float]], acts: list[list[dict]]) -> list[str]:
    problems = []

    a1, a2, a3, a4 = act_avgs

    # Climax too early — Act 2 more intense than Act 3
    a2_energy = (a2["movement"] + a2["aggression"]) / 2
    a3_energy = (a3["movement"] + a3["aggression"]) / 2
    if a2_energy > a3_energy + 0.10:
        problems.append(
            f"CLIMAX TOO EARLY — Act 2 energy ({a2_energy:.2f}) exceeds Act 3 ({a3_energy:.2f}). "
            "Move high-energy presets from Act 2 to Act 3."
        )

    # No resolution — Act 4 has high aggression
    if a4["aggression"] > 0.50:
        problems.append(
            f"NO RESOLUTION — Act 4 avg aggression is {a4['aggression']:.2f} (>0.50). "
            "Replace or reorder high-aggression Act 4 presets with calmer ones."
        )

    # Flat arc — all acts have very similar movement + aggression
    energies = [(a["movement"] + a["aggression"]) / 2 for a in act_avgs]
    span = max(energies) - min(energies)
    if span < 0.12:
        problems.append(
            f"FLAT ARC — Energy range across acts is only {span:.2f} (ideally >0.20). "
            "Introduce more contrast between acts."
        )

    # Empty acts
    for i, act in enumerate(acts):
        if len(act) == 0:
            problems.append(f"EMPTY ACT — {ACT_NAMES[i]} has no presets.")

    # Act 1 too aggressive
    if a1["aggression"] > 0.45:
        problems.append(
            f"ACT 1 TOO HARSH — avg aggression {a1['aggression']:.2f}. "
            "Setup should welcome the listener; move aggressive presets later."
        )

    return problems


# ---------------------------------------------------------------------------
# Reorder suggestions
# ---------------------------------------------------------------------------

def suggest_reordering(acts: list[list[dict]]) -> list[str]:
    """Suggest moving individual presets to better-fitting acts."""
    suggestions = []

    def energy(p: dict) -> float:
        return (p["dna"]["movement"] + p["dna"]["aggression"]) / 2

    def softness(p: dict) -> float:
        return (p["dna"]["space"] + p["dna"]["warmth"]) / 2 - p["dna"]["aggression"]

    # Act 1 bad fits: high-energy presets that belong in Act 3
    for p in acts[0]:
        if energy(p) > 0.60:
            suggestions.append(
                f"  Move '{p['name']}' from Act 1 → Act 3 (energy={energy(p):.2f} too high for Setup)"
            )

    # Act 2 bad fits: very high energy (belongs in Act 3) or very soft (belongs in Act 1)
    for p in acts[1]:
        if energy(p) > 0.72:
            suggestions.append(
                f"  Move '{p['name']}' from Act 2 → Act 3 (energy={energy(p):.2f} peaks too early)"
            )
        elif energy(p) < 0.18 and softness(p) > 0.35:
            suggestions.append(
                f"  Move '{p['name']}' from Act 2 → Act 1 (too soft for Development)"
            )

    # Act 3 bad fits: very soft presets that water down the climax
    for p in acts[2]:
        if energy(p) < 0.28:
            suggestions.append(
                f"  Move '{p['name']}' from Act 3 → Act 4 (energy={energy(p):.2f} too low for Climax)"
            )

    # Act 4 bad fits: aggressive presets that prevent resolution
    for p in acts[3]:
        if p["dna"]["aggression"] > 0.55:
            suggestions.append(
                f"  Move '{p['name']}' from Act 4 → Act 3 (aggression={p['dna']['aggression']:.2f} prevents Resolution)"
            )

    return suggestions


# ---------------------------------------------------------------------------
# ASCII visualisation
# ---------------------------------------------------------------------------

PLOT_HEIGHT = 10  # rows
PLOT_WIDTH  = 20  # chars per act column

DIM_SYMBOLS = {
    "movement":   ("M", "\033[36m"),   # cyan
    "aggression": ("A", "\033[31m"),   # red
    "brightness": ("B", "\033[33m"),   # yellow
    "space":      ("S", "\033[34m"),   # blue
    "warmth":     ("W", "\033[35m"),   # magenta
    "density":    ("D", "\033[32m"),   # green
}
RESET = "\033[0m"


def _bar(value: float, width: int, symbol: str, color: str, use_color: bool) -> str:
    filled = round(value * width)
    bar = symbol * filled + "." * (width - filled)
    if use_color:
        return f"{color}{bar}{RESET}"
    return bar


def ascii_arc(act_avgs: list[dict[str, float]], use_color: bool = True) -> str:
    lines = []
    sep = "+" + ("-" * (PLOT_WIDTH + 2) + "+") * 4

    # Header
    lines.append(sep)
    header_parts = []
    for name in ACT_SHORT:
        cell = name.center(PLOT_WIDTH + 2)
        header_parts.append(cell)
    lines.append("|" + "|".join(header_parts) + "|")
    lines.append(sep)

    # One row per tracked dimension
    for dim in ["movement", "aggression", "brightness", "space", "warmth", "density"]:
        symbol, color = DIM_SYMBOLS[dim]
        label = f"{symbol}={dim[:6]}"
        row_parts = []
        for avg in act_avgs:
            val = avg[dim]
            bar = _bar(val, PLOT_WIDTH - 6, symbol, color, use_color)
            cell = f" {label:<8} {bar} "
            row_parts.append(cell)
        lines.append("|" + "|".join(row_parts) + "|")

    lines.append(sep)

    # Legend
    lines.append("")
    lines.append("Legend: bar width = 0.0 (empty) → 1.0 (full)  |  symbol fills indicate avg DNA value")
    for dim in ["movement", "aggression", "brightness", "space", "warmth", "density"]:
        symbol, color = DIM_SYMBOLS[dim]
        col = color if use_color else ""
        rst = RESET if use_color else ""
        lines.append(f"  {col}{symbol}{rst} = {dim}")

    return "\n".join(lines)


# ---------------------------------------------------------------------------
# Report assembly
# ---------------------------------------------------------------------------

def build_report(
    preset_dir: Path,
    presets: list[dict],
    acts: list[list[dict]],
    act_avgs: list[dict[str, float]],
    problems: list[str],
    suggestions: list[str],
    coherence: float,
    use_color: bool,
) -> str:
    lines = []

    lines.append("=" * 72)
    lines.append("XOlokun Pack Story Arc Report")
    lines.append(f"Directory : {preset_dir}")
    lines.append(f"Presets   : {len(presets)}")
    lines.append(f"Coherence : {coherence}/100")
    lines.append("=" * 72)
    lines.append("")

    # Per-act summary
    lines.append("── ACT BREAKDOWN ──────────────────────────────────────────────────────")
    for i, (act, avg) in enumerate(zip(acts, act_avgs)):
        score = act_score(avg, ACT_TARGETS[i])
        lines.append(f"\n{ACT_NAMES[i]}  [{len(act)} presets]  score={score}/100")
        # DNA values
        for dim in DIMS:
            bar_w = 20
            filled = round(avg[dim] * bar_w)
            bar = "#" * filled + "." * (bar_w - filled)
            lines.append(f"  {dim:<12} [{bar}] {avg[dim]:.2f}")
        # List presets
        if act:
            names = ", ".join(p["name"] for p in act[:6])
            if len(act) > 6:
                names += f", … (+{len(act)-6} more)"
            lines.append(f"  Presets : {names}")

    lines.append("")
    lines.append("── NARRATIVE ARC (ASCII) ──────────────────────────────────────────────")
    lines.append("")
    lines.append(ascii_arc(act_avgs, use_color=use_color))
    lines.append("")

    if problems:
        lines.append("── ARC PROBLEMS ───────────────────────────────────────────────────────")
        for prob in problems:
            lines.append(f"  ⚠  {prob}")
        lines.append("")
    else:
        lines.append("── ARC PROBLEMS ───────────────────────────────────────────────────────")
        lines.append("  No arc problems detected.")
        lines.append("")

    if suggestions:
        lines.append("── REORDER SUGGESTIONS ────────────────────────────────────────────────")
        for s in suggestions:
            lines.append(s)
        lines.append("")
    else:
        lines.append("── REORDER SUGGESTIONS ────────────────────────────────────────────────")
        lines.append("  No reordering needed.")
        lines.append("")

    # Optimised order hint
    return "\n".join(lines)


# ---------------------------------------------------------------------------
# Optimise (simple greedy sort per act)
# ---------------------------------------------------------------------------

def optimised_order(presets: list[dict]) -> list[dict]:
    """
    Return presets sorted so that low-energy presets come first and
    high-energy ones cluster around the 60-75% mark, with a cool-down tail.
    Uses a simple energy function.
    """

    def energy(p: dict) -> float:
        return (p["dna"]["movement"] * 1.5 + p["dna"]["aggression"] * 1.5
                + p["dna"]["density"] - p["dna"]["space"]) / 4

    n = len(presets)
    # Target energy curve: rises to peak at 65%, falls to low
    def target_energy(idx: int) -> float:
        t = idx / max(n - 1, 1)
        # Gaussian peak at t=0.65
        return math.exp(-((t - 0.65) ** 2) / (2 * 0.08 ** 2))

    targets = [target_energy(i) for i in range(n)]
    sorted_by_energy = sorted(presets, key=energy)

    # Greedily assign preset with closest energy to each target slot
    assigned = [None] * n
    available = list(sorted_by_energy)
    for slot_idx in sorted(range(n), key=lambda i: targets[i]):
        t = targets[slot_idx]
        best = min(available, key=lambda p: abs(energy(p) - t))
        assigned[slot_idx] = best
        available.remove(best)

    return assigned  # type: ignore[return-value]


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def main() -> None:
    parser = argparse.ArgumentParser(
        description="Validate the story arc of an XOlokun preset pack."
    )
    parser.add_argument(
        "--preset-dir",
        required=True,
        type=Path,
        help="Directory containing .xometa preset files.",
    )
    parser.add_argument(
        "--optimize",
        action="store_true",
        help="Print a suggested optimised preset order.",
    )
    parser.add_argument(
        "--output",
        type=Path,
        default=None,
        help="Write report to this file (plain text, no colour codes).",
    )
    parser.add_argument(
        "--no-color",
        action="store_true",
        help="Disable ANSI colour codes in terminal output.",
    )
    args = parser.parse_args()

    preset_dir: Path = args.preset_dir
    if not preset_dir.exists():
        print(f"ERROR: preset-dir does not exist: {preset_dir}", file=sys.stderr)
        sys.exit(1)

    presets = load_presets(preset_dir)
    if len(presets) < 4:
        print(
            f"ERROR: need at least 4 presets with valid DNA, found {len(presets)}.",
            file=sys.stderr,
        )
        sys.exit(1)

    acts      = assign_acts(presets)
    act_avgs  = [avg_dna(act) for act in acts]
    coherence = narrative_coherence(act_avgs)
    problems  = detect_problems(act_avgs, acts)
    suggestions = suggest_reordering(acts)

    use_color_terminal = not args.no_color and sys.stdout.isatty()

    report = build_report(
        preset_dir,
        presets,
        acts,
        act_avgs,
        problems,
        suggestions,
        coherence,
        use_color=use_color_terminal,
    )

    print(report)

    if args.output:
        # Write plain text without ANSI codes
        plain_report = build_report(
            preset_dir,
            presets,
            acts,
            act_avgs,
            problems,
            suggestions,
            coherence,
            use_color=False,
        )
        args.output.write_text(plain_report, encoding="utf-8")
        print(f"Report written to {args.output}")

    if args.optimize:
        opt = optimised_order(presets)
        print("\n── OPTIMISED ORDER ────────────────────────────────────────────────────")
        for i, p in enumerate(opt, 1):
            e = (p["dna"]["movement"] * 1.5 + p["dna"]["aggression"] * 1.5
                 + p["dna"]["density"] - p["dna"]["space"]) / 4
            print(f"  {i:>3}. {p['name']:<36}  energy={e:.2f}")
        print()


if __name__ == "__main__":
    main()
