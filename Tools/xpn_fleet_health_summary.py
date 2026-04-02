#!/usr/bin/env python3
"""
xpn_fleet_health_summary.py — XOceanus Fleet Health Dashboard

Single-pass summary of the entire .xometa preset library.
Produces a clean, columnar text report (no ASCII charts).

Usage:
    python xpn_fleet_health_summary.py --preset-dir ../Presets/XOceanus
    python xpn_fleet_health_summary.py --preset-dir ../Presets/XOceanus --output summary.txt
    python xpn_fleet_health_summary.py --preset-dir ../Presets/XOceanus --json
"""

import argparse
import json
import re
import sys
from collections import Counter, defaultdict
from pathlib import Path

# ── Constants ──────────────────────────────────────────────────────────────────

MOODS = {"Foundation", "Atmosphere", "Entangled", "Prism", "Flux", "Aether", "Family"}
DNA_DIMS = ["brightness", "warmth", "movement", "density", "space", "aggression"]

STOP_WORDS = {
    "a", "an", "the", "of", "in", "on", "at", "to", "for", "and", "or",
    "with", "by", "from", "is", "it", "its", "as", "be", "that", "this",
    "xo", "ox", "xo_ox",
}

JARGON_WORDS = {
    "lfo", "adsr", "vca", "vcf", "vco", "cutoff", "resonance", "reso",
    "detune", "glide", "midi", "osc", "env", "amp", "mod", "freq",
    "hz", "db", "bpm", "ms", "sync", "pwm", "fm", "am", "delay",
    "reverb", "attack", "decay", "sustain", "release",
}

# ── Single-pass collector ──────────────────────────────────────────────────────

def collect(preset_dir: Path) -> dict:
    """Walk all .xometa files and accumulate stats in one pass."""
    total = 0
    engine_counts: Counter = Counter()
    mood_counts: Counter = Counter()
    dna_sums = {d: 0.0 for d in DNA_DIMS}
    dna_complete = 0
    dna_missing = 0
    felix_count = 0   # brightness > warmth
    oscar_count = 0   # warmth > brightness
    equal_count = 0   # brightness == warmth

    entangled_count = 0
    coupling_pairs: set = set()   # frozenset of (engineA, engineB)

    long_name_count = 0           # name > 30 chars
    jargon_name_count = 0         # name contains jargon word
    word_counter: Counter = Counter()

    last_filename = ""            # filename sort proxy for recency

    for path in sorted(preset_dir.rglob("*.xometa")):
        try:
            data = json.loads(path.read_text(encoding="utf-8"))
        except (json.JSONDecodeError, OSError):
            continue

        total += 1
        last_filename = path.name

        # mood
        mood = data.get("mood", "Unknown")
        mood_counts[mood] += 1

        # engines
        engines = data.get("engines") or []
        if isinstance(engines, list):
            for e in engines:
                if isinstance(e, str):
                    engine_counts[e] += 1

        # dna
        dna = data.get("dna") or {}
        if isinstance(dna, dict) and all(d in dna for d in DNA_DIMS):
            dna_complete += 1
            for d in DNA_DIMS:
                val = dna.get(d)
                if isinstance(val, (int, float)):
                    dna_sums[d] += float(val)
            b = dna.get("brightness", 0.0)
            w = dna.get("warmth", 0.0)
            if b > w:
                felix_count += 1
            elif w > b:
                oscar_count += 1
            else:
                equal_count += 1
        else:
            dna_missing += 1

        # coupling
        if mood == "Entangled":
            entangled_count += 1
        coupling = data.get("coupling")
        if coupling and isinstance(coupling, dict):
            pairs = coupling.get("pairs") or []
            for pair in pairs:
                ea = pair.get("engineA") or pair.get("sourceEngine", "")
                eb = pair.get("engineB") or pair.get("targetEngine", "")
                if ea and eb:
                    coupling_pairs.add(frozenset({ea, eb}))

        # name health
        name = data.get("name", "")
        if len(name) > 30:
            long_name_count += 1

        name_lower = name.lower()
        name_words = re.findall(r"[a-z]+", name_lower)
        if any(w in JARGON_WORDS for w in name_words):
            jargon_name_count += 1

        for w in name_words:
            if w not in STOP_WORDS and len(w) > 2:
                word_counter[w] += 1

    return {
        "total": total,
        "engine_counts": engine_counts,
        "mood_counts": mood_counts,
        "dna_sums": dna_sums,
        "dna_complete": dna_complete,
        "dna_missing": dna_missing,
        "felix_count": felix_count,
        "oscar_count": oscar_count,
        "equal_count": equal_count,
        "entangled_count": entangled_count,
        "coupling_pairs": coupling_pairs,
        "long_name_count": long_name_count,
        "jargon_name_count": jargon_name_count,
        "word_counter": word_counter,
        "last_filename": last_filename,
    }


# ── Report builder ─────────────────────────────────────────────────────────────

def build_report(stats: dict) -> str:
    total = stats["total"]
    if total == 0:
        return "No .xometa files found."

    lines = []

    def h(title: str):
        lines.append("")
        lines.append(f"{'─' * 60}")
        lines.append(f"  {title}")
        lines.append(f"{'─' * 60}")

    def row(label: str, value):
        lines.append(f"  {label:<38} {value}")

    def pct(n, d):
        return f"{n:>5}  ({100 * n / d:5.1f}%)" if d else f"{n:>5}  (  n/a)"

    # ── Header ────────────────────────────────────────────────────────────────
    lines.append("╔══════════════════════════════════════════════════════════╗")
    lines.append("║          XOceanus Fleet Health Summary                  ║")
    lines.append("╚══════════════════════════════════════════════════════════╝")

    # ── Overview ──────────────────────────────────────────────────────────────
    h("OVERVIEW")
    row("Total presets", total)
    row("Engines with presets", len(stats["engine_counts"]))
    row("Most recent (alpha proxy)", stats["last_filename"] or "—")

    # ── Per-engine ────────────────────────────────────────────────────────────
    h("PRESETS PER ENGINE  (sorted desc)")
    for eng, cnt in stats["engine_counts"].most_common():
        bar = cnt
        lines.append(f"  {eng:<28} {cnt:>5}  {pct(cnt, total)}")

    # ── Per-mood ──────────────────────────────────────────────────────────────
    h("PRESETS PER MOOD")
    mood_counts = stats["mood_counts"]
    all_moods = sorted(MOODS | set(mood_counts.keys()))
    for mood in all_moods:
        cnt = mood_counts.get(mood, 0)
        lines.append(f"  {mood:<28} {pct(cnt, total)}")

    # ── DNA centroid ──────────────────────────────────────────────────────────
    h("SONIC DNA  —  Fleet Centroid  (mean across all presets)")
    dc = stats["dna_complete"]
    row("Presets with complete DNA", f"{dc}  ({100 * dc / total:.1f}%)")
    row("Presets with missing DNA", stats["dna_missing"])
    if dc > 0:
        lines.append("")
        for d in DNA_DIMS:
            mean = stats["dna_sums"][d] / dc
            bar_len = round(mean * 20)
            bar = "█" * bar_len + "░" * (20 - bar_len)
            lines.append(f"  {d:<14} {mean:.3f}  [{bar}]")

    # ── feliX-Oscar balance ───────────────────────────────────────────────────
    h("FELIX-OSCAR POLARITY  (brightness vs warmth)")
    fx = stats["felix_count"]
    os_ = stats["oscar_count"]
    eq = stats["equal_count"]
    row("feliX  (brightness > warmth)", pct(fx, dc) if dc else "—")
    row("Oscar  (warmth > brightness)", pct(os_, dc) if dc else "—")
    row("Balanced  (brightness = warmth)", pct(eq, dc) if dc else "—")

    # ── Coupling coverage ─────────────────────────────────────────────────────
    h("COUPLING COVERAGE")
    row("Entangled mood presets", stats["entangled_count"])
    row("Unique engine pairs seen in coupling", len(stats["coupling_pairs"]))
    if stats["coupling_pairs"]:
        lines.append("")
        lines.append("  Engine pairs:")
        for pair in sorted(stats["coupling_pairs"], key=lambda p: sorted(p)):
            parts = sorted(pair)
            if len(parts) == 2:
                a, b = parts
                lines.append(f"    {a}  ↔  {b}")
            else:
                lines.append(f"    {parts[0]}  (self-coupling)")

    # ── Naming health ─────────────────────────────────────────────────────────
    h("PRESET NAMING HEALTH")
    lnc = stats["long_name_count"]
    jnc = stats["jargon_name_count"]
    row("Names over 30 chars", pct(lnc, total))
    row("Names containing jargon", pct(jnc, total))
    lines.append("")
    lines.append("  Top 5 name words (excl. stop words):")
    for word, cnt in stats["word_counter"].most_common(5):
        lines.append(f"    {word:<20}  {cnt:>4}x")

    lines.append("")
    lines.append(f"{'─' * 60}")
    lines.append("  End of report.")
    lines.append("")
    return "\n".join(lines)


# ── JSON output ────────────────────────────────────────────────────────────────

def build_json(stats: dict) -> str:
    total = stats["total"]
    dc = stats["dna_complete"]
    centroid = (
        {d: round(stats["dna_sums"][d] / dc, 4) for d in DNA_DIMS}
        if dc > 0
        else {d: None for d in DNA_DIMS}
    )
    out = {
        "total_presets": total,
        "engines_with_presets": len(stats["engine_counts"]),
        "per_engine": dict(stats["engine_counts"].most_common()),
        "per_mood": {m: stats["mood_counts"].get(m, 0) for m in sorted(MOODS | set(stats["mood_counts"]))},
        "dna": {
            "complete": dc,
            "missing": stats["dna_missing"],
            "centroid": centroid,
        },
        "felix_oscar": {
            "felix_pct": round(100 * stats["felix_count"] / dc, 2) if dc else None,
            "oscar_pct": round(100 * stats["oscar_count"] / dc, 2) if dc else None,
            "balanced_pct": round(100 * stats["equal_count"] / dc, 2) if dc else None,
        },
        "coupling": {
            "entangled_presets": stats["entangled_count"],
            "unique_engine_pairs": len(stats["coupling_pairs"]),
            "pairs": [sorted(list(p)) for p in sorted(stats["coupling_pairs"], key=lambda p: sorted(p))],
        },
        "naming_health": {
            "long_name_count": stats["long_name_count"],
            "long_name_pct": round(100 * stats["long_name_count"] / total, 2) if total else 0,
            "jargon_name_count": stats["jargon_name_count"],
            "jargon_name_pct": round(100 * stats["jargon_name_count"] / total, 2) if total else 0,
            "top_5_words": dict(stats["word_counter"].most_common(5)),
        },
        "last_filename_alpha": stats["last_filename"],
    }
    return json.dumps(out, indent=2)


# ── CLI ────────────────────────────────────────────────────────────────────────

def main():
    parser = argparse.ArgumentParser(
        description="XOceanus fleet health dashboard — single pass over all .xometa files."
    )
    parser.add_argument(
        "--preset-dir",
        required=True,
        type=Path,
        help="Root directory containing .xometa preset files (searched recursively).",
    )
    parser.add_argument(
        "--output",
        type=Path,
        default=None,
        help="Write report to this file instead of stdout.",
    )
    parser.add_argument(
        "--json",
        action="store_true",
        dest="as_json",
        help="Output machine-readable JSON instead of the text dashboard.",
    )
    args = parser.parse_args()

    if not args.preset_dir.is_dir():
        print(f"Error: preset-dir '{args.preset_dir}' is not a directory.", file=sys.stderr)
        sys.exit(1)

    stats = collect(args.preset_dir)

    report = build_json(stats) if args.as_json else build_report(stats)

    if args.output:
        args.output.write_text(report, encoding="utf-8")
        print(f"Report written to {args.output}")
    else:
        print(report)


if __name__ == "__main__":
    main()
