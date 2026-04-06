#!/usr/bin/env python3
"""
XOceanus Preset Mood Rebalancer

Analyzes mood distribution across .xometa presets for an engine and suggests/applies
mood reassignments to achieve a healthier balance across the 7 XOceanus moods.

Usage:
    python xpn_preset_mood_rebalancer.py <presets_dir> [--engine FILTER] [--suggest] [--apply] [--format text|json]

Modes:
    (default)   Analyze — show current distribution vs. targets
    --suggest   Identify candidates for reassignment from OVER → UNDER moods
    --apply     Write suggested reassignments to .xometa files (creates .bak backups)
"""

import argparse
import glob
import json
import shutil
import sys
from pathlib import Path

# ---------------------------------------------------------------------------
# Mood targets (min%, max%)
# ---------------------------------------------------------------------------
MOODS = ["Foundation", "Atmosphere", "Entangled", "Prism", "Flux", "Aether", "Family",
         "Submerged", "Coupling", "Crystalline", "Deep", "Ethereal", "Kinetic", "Luminous",
         "Organic", "Shadow"]

TARGETS = {
    "Foundation":  (20, 30),
    "Atmosphere":  (15, 25),
    "Entangled":   (15, 25),
    "Prism":       (10, 20),
    "Flux":        (10, 20),
    "Aether":      ( 5, 15),
    "Family":      ( 5, 10),
}

# ---------------------------------------------------------------------------
# Reassignment rules: (from_mood, to_mood, description, test_fn)
# test_fn receives the parsed preset dict and returns True if it's a candidate
# ---------------------------------------------------------------------------
def _has_coupling(p):
    pairs = p.get("coupling", {}).get("pairs", [])
    return isinstance(pairs, list) and len(pairs) > 0

def _has_coupling_intensity(p):
    ci = p.get("couplingIntensity", "None")
    return isinstance(ci, str) and ci.lower() not in ("none", "")

def _high_movement(p):
    dna = p.get("dna") or p.get("sonic_dna") or {}
    return dna.get("movement", 0.0) >= 0.65

def _high_aggression(p):
    dna = p.get("dna") or p.get("sonic_dna") or {}
    return dna.get("aggression", 0.0) > 0.7

def _high_modulation_density(p):
    """Presets with high movement AND high density lean Flux."""
    dna = p.get("dna") or p.get("sonic_dna") or {}
    return dna.get("movement", 0.0) >= 0.65 and dna.get("density", 0.0) >= 0.6

def _has_sequencer(p):
    return p.get("sequencer") is not None

def _low_aggression_high_space(p):
    dna = p.get("dna") or p.get("sonic_dna") or {}
    return dna.get("aggression", 1.0) < 0.3 and dna.get("space", 0.0) >= 0.6

REASSIGNMENT_RULES = [
    # (from, to, description, test_fn)
    ("Foundation", "Entangled", "has active coupling pairs",          _has_coupling),
    ("Foundation", "Entangled", "has non-None coupling intensity",    _has_coupling_intensity),
    ("Foundation", "Prism",     "high movement DNA (≥0.65) — expressive", _high_movement),
    ("Foundation", "Flux",      "high movement + density — modulation heavy", _high_modulation_density),
    ("Atmosphere", "Aether",    "aggression > 0.7 — too extreme for atmospheric", _high_aggression),
    ("Atmosphere", "Entangled", "has active coupling pairs",          _has_coupling),
    ("Prism",      "Flux",      "high movement + density — better as Flux", _high_modulation_density),
    ("Flux",       "Entangled", "has active coupling — coupling-driven movement", _has_coupling),
    ("Foundation", "Family",    "has sequencer — tutorial/reference use", _has_sequencer),
    ("Atmosphere", "Foundation","low aggression + high space — approachable pad", _low_aggression_high_space),
]


# ---------------------------------------------------------------------------
# Load / filter
# ---------------------------------------------------------------------------
def load_presets(presets_dir: str, engine_filter) -> list[dict]:
    pattern = str(Path(presets_dir) / "**" / "*.xometa")
    files = sorted(glob.glob(pattern, recursive=True))
    presets = []
    for f in files:
        try:
            with open(f) as fh:
                data = json.load(fh)
        except (json.JSONDecodeError, IOError) as e:
            print(f"  WARN: could not parse {f}: {e}", file=sys.stderr)
            continue
        data["_path"] = f

        if engine_filter:
            engines = [e.upper() for e in (data.get("engines") or [])]
            if engine_filter.upper() not in engines:
                continue
        presets.append(data)
    return presets


# ---------------------------------------------------------------------------
# Analysis
# ---------------------------------------------------------------------------
def analyze(presets: list[dict]) -> dict:
    total = len(presets)
    counts = {m: 0 for m in MOODS}
    unknown = 0
    for p in presets:
        m = p.get("mood", "")
        if m in counts:
            counts[m] += 1
        else:
            unknown += 1

    pcts = {m: (counts[m] / total * 100) if total else 0 for m in MOODS}
    statuses = {}
    for m in MOODS:
        lo, hi = TARGETS[m]
        pct = pcts[m]
        if pct > hi:
            statuses[m] = "OVER"
        elif pct < lo:
            statuses[m] = "UNDER"
        else:
            statuses[m] = "OK"

    return {
        "total": total,
        "unknown_mood": unknown,
        "counts": counts,
        "pcts": pcts,
        "statuses": statuses,
    }


# ---------------------------------------------------------------------------
# Suggestions
# ---------------------------------------------------------------------------
def suggest(presets: list[dict], stats: dict) -> list[dict]:
    """Return list of suggestion dicts."""
    over_moods  = {m for m, s in stats["statuses"].items() if s == "OVER"}
    under_moods = {m for m, s in stats["statuses"].items() if s == "UNDER"}

    # Track how many times we've suggested moving FROM each mood so we don't
    # suggest so many moves that an OVER mood becomes UNDER
    from_counts: dict[str, int] = {m: 0 for m in MOODS}
    suggestions = []

    for p in presets:
        current_mood = p.get("mood", "")
        if current_mood not in over_moods:
            continue

        for (from_m, to_m, reason, test_fn) in REASSIGNMENT_RULES:
            if from_m != current_mood:
                continue
            if to_m not in under_moods:
                continue

            # Don't suggest more moves from this mood than the surplus
            lo, hi = TARGETS[current_mood]
            total = stats["total"]
            surplus = max(0, stats["counts"][current_mood] - round(hi / 100 * total))
            if from_counts[current_mood] >= surplus:
                break

            if test_fn(p):
                suggestions.append({
                    "path":         p["_path"],
                    "name":         p.get("name", Path(p["_path"]).stem),
                    "from_mood":    current_mood,
                    "to_mood":      to_m,
                    "reason":       reason,
                })
                from_counts[current_mood] += 1
                break  # one suggestion per preset

    return suggestions


# ---------------------------------------------------------------------------
# Apply
# ---------------------------------------------------------------------------
def apply_suggestions(suggestions: list[dict]) -> tuple[int, int]:
    applied = 0
    failed = 0
    for s in suggestions:
        path = s["path"]
        bak  = path + ".bak"
        try:
            shutil.copy2(path, bak)
            with open(path) as fh:
                data = json.load(fh)
            data["mood"] = s["to_mood"]
            with open(path, "w") as fh:
                json.dump(data, fh, indent=2)
            applied += 1
        except (IOError, json.JSONDecodeError) as e:
            print(f"  ERROR applying {path}: {e}", file=sys.stderr)
            failed += 1
    return applied, failed


# ---------------------------------------------------------------------------
# Rendering
# ---------------------------------------------------------------------------
BAR_WIDTH = 20

def bar(pct: float) -> str:
    filled = round(pct / 100 * BAR_WIDTH)
    return "█" * filled + "░" * (BAR_WIDTH - filled)

STATUS_SYMBOL = {"OK": "✓", "OVER": "OVER", "UNDER": "UNDER"}

def render_analysis_text(stats: dict, engine_label: str) -> None:
    total = stats["total"]
    print(f"\nMOOD BALANCE — {engine_label} ({total} presets)")
    print("─" * 70)
    print(f"{'Mood':<12} {'Bar':<{BAR_WIDTH+2}} {'%':>5}  {'Target':>12}  Status")
    print("─" * 70)
    for m in MOODS:
        pct    = stats["pcts"][m]
        lo, hi = TARGETS[m]
        status = stats["statuses"][m]
        sym    = STATUS_SYMBOL[status]
        print(f"{m:<12} {bar(pct):<{BAR_WIDTH+2}} {pct:>4.0f}%  {lo}-{hi}%{'':<5}  {sym}")
    print("─" * 70)
    if stats["unknown_mood"]:
        print(f"  Note: {stats['unknown_mood']} preset(s) have unrecognised mood values")

def render_suggestions_text(suggestions: list[dict]) -> None:
    if not suggestions:
        print("\nNo reassignment candidates found.")
        return
    print(f"\nSUGGESTED REASSIGNMENTS ({len(suggestions)})")
    print("─" * 70)
    for s in suggestions:
        name = s["name"][:36]
        print(f"  {name:<38} {s['from_mood']:<12} → {s['to_mood']:<12}")
        print(f"    reason: {s['reason']}")
    print("─" * 70)

def render_apply_text(applied: int, failed: int) -> None:
    print(f"\nAPPLIED: {applied} reassignment(s) written  |  FAILED: {failed}")
    if applied:
        print("  Backups saved as <preset>.xometa.bak")


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------
def main():
    parser = argparse.ArgumentParser(
        description="Analyze and rebalance XOceanus preset mood distribution"
    )
    parser.add_argument("presets_dir",
                        help="Root directory to scan for .xometa files (recursive)")
    parser.add_argument("--engine", metavar="NAME",
                        help="Filter by engine name (e.g. OPAL, Odyssey)")
    parser.add_argument("--suggest", action="store_true",
                        help="Show reassignment candidates for OVER moods")
    parser.add_argument("--apply", action="store_true",
                        help="Write suggested reassignments to files (implies --suggest)")
    parser.add_argument("--format", choices=["text", "json"], default="text",
                        help="Output format (default: text)")
    args = parser.parse_args()

    if args.apply:
        args.suggest = True

    presets = load_presets(args.presets_dir, args.engine)
    if not presets:
        print(f"No .xometa presets found in {args.presets_dir}", file=sys.stderr)
        sys.exit(1)

    engine_label = args.engine.upper() if args.engine else "all engines"
    stats        = analyze(presets)
    suggestions  = suggest(presets, stats) if args.suggest else []
    apply_result = None
    if args.apply and suggestions:
        applied, failed = apply_suggestions(suggestions)
        apply_result = {"applied": applied, "failed": failed}
        # Re-analyse after changes
        presets_after = load_presets(args.presets_dir, args.engine)
        stats_after   = analyze(presets_after)
    else:
        stats_after = None

    if args.format == "json":
        out = {
            "engine":      engine_label,
            "analysis":    stats,
            "suggestions": suggestions,
        }
        if apply_result:
            out["apply_result"] = apply_result
            out["analysis_after"] = stats_after
        print(json.dumps(out, indent=2))
    else:
        render_analysis_text(stats, engine_label)
        if args.suggest:
            render_suggestions_text(suggestions)
        if apply_result:
            render_apply_text(apply_result["applied"], apply_result["failed"])
            if stats_after:
                print("\nUPDATED DISTRIBUTION:")
                render_analysis_text(stats_after, engine_label)


if __name__ == "__main__":
    main()
