#!/usr/bin/env python3
"""
xpn_macro_performance_recorder.py
XO_OX XOceanus — Macro Performance Script Tool

Records and replays macro performance scripts, embeds automation hints
in .xometa files, and generates ASCII performance scores.

Usage:
    python xpn_macro_performance_recorder.py --script perf.json
    python xpn_macro_performance_recorder.py --script perf.json --preset preset.xometa --embed
    python xpn_macro_performance_recorder.py --script perf.json --output perf_report.txt
"""

import json
import argparse
import math
from pathlib import Path


CANONICAL_MACROS = {"CHARACTER", "MOVEMENT", "COUPLING", "SPACE"}
BEATS_PER_BAR = 4


# ---------------------------------------------------------------------------
# Validation
# ---------------------------------------------------------------------------

def validate_script(script: dict) -> list[str]:
    """Return a list of error strings; empty list means valid."""
    errors = []

    for field in ("preset", "bpm", "duration_bars", "events"):
        if field not in script:
            errors.append(f"Missing required field: '{field}'")

    if errors:
        return errors

    bpm = script["bpm"]
    duration_bars = script["duration_bars"]

    if not isinstance(bpm, (int, float)) or bpm <= 0:
        errors.append(f"bpm must be a positive number, got: {bpm!r}")

    if not isinstance(duration_bars, int) or duration_bars < 1:
        errors.append(f"duration_bars must be a positive integer, got: {duration_bars!r}")

    if not isinstance(script["events"], list):
        errors.append("'events' must be a list")
        return errors

    for i, event in enumerate(script["events"]):
        prefix = f"Event[{i}]"

        macro = event.get("macro", "")
        if macro not in CANONICAL_MACROS:
            errors.append(
                f"{prefix}: unknown macro '{macro}' — "
                f"must be one of {sorted(CANONICAL_MACROS)}"
            )

        value = event.get("value")
        if not isinstance(value, (int, float)) or not (0.0 <= value <= 1.0):
            errors.append(f"{prefix}: value must be 0.0–1.0, got: {value!r}")

        bar = event.get("bar")
        if not isinstance(bar, int) or bar < 1:
            errors.append(f"{prefix}: bar must be a positive integer, got: {bar!r}")
        elif isinstance(duration_bars, int) and bar > duration_bars:
            errors.append(
                f"{prefix}: bar {bar} exceeds duration_bars {duration_bars}"
            )

        beat = event.get("beat")
        if not isinstance(beat, int) or not (1 <= beat <= BEATS_PER_BAR):
            errors.append(
                f"{prefix}: beat must be 1–{BEATS_PER_BAR}, got: {beat!r}"
            )

    return errors


# ---------------------------------------------------------------------------
# Time conversion
# ---------------------------------------------------------------------------

def bar_beat_to_ms(bar: int, beat: int, bpm: float) -> float:
    """Convert bar+beat (1-indexed) to milliseconds."""
    beats_elapsed = (bar - 1) * BEATS_PER_BAR + (beat - 1)
    return (beats_elapsed / bpm) * 60_000.0


def render_events(script: dict) -> list[dict]:
    """Return events enriched with 'time_ms', sorted by time."""
    bpm = script["bpm"]
    rendered = []
    for event in script["events"]:
        ms = bar_beat_to_ms(event["bar"], event["beat"], bpm)
        rendered.append({**event, "time_ms": ms})
    return sorted(rendered, key=lambda e: (e["time_ms"], e["macro"]))


# ---------------------------------------------------------------------------
# ASCII performance score
# ---------------------------------------------------------------------------

def generate_score(script: dict, rendered: list[dict]) -> str:
    """
    Produce a time × macro grid.

    Columns = bars (1 to duration_bars).
    Rows = macros (CHARACTER / MOVEMENT / COUPLING / SPACE).
    Each cell shows the value set at that bar (first beat event wins),
    or '----' if unchanged.
    """
    duration_bars = script["duration_bars"]
    macro_order = ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"]

    # Build lookup: (bar, macro) -> value (first event at that bar wins)
    lookup: dict[tuple[int, str], float] = {}
    for e in rendered:
        key = (e["bar"], e["macro"])
        if key not in lookup:
            lookup[key] = e["value"]

    # Header
    col_w = 7
    bar_labels = "".join(f"Bar{b:<{col_w - 3}}" for b in range(1, duration_bars + 1))
    header = f"{'Macro':<12}{bar_labels}"
    separator = "-" * len(header)

    lines = [
        f"Performance Score — {script['preset']}",
        f"BPM: {script['bpm']}  |  Duration: {duration_bars} bars",
        separator,
        header,
        separator,
    ]

    for macro in macro_order:
        row = f"{macro:<12}"
        for bar in range(1, duration_bars + 1):
            cell = lookup.get((bar, macro))
            if cell is not None:
                row += f"{cell:<{col_w}.2f}"
            else:
                row += f"{'----':<{col_w}}"
        lines.append(row)

    lines.append(separator)
    return "\n".join(lines)


# ---------------------------------------------------------------------------
# Macro density analysis
# ---------------------------------------------------------------------------

def analyze_density(script: dict, rendered: list[dict]) -> str:
    """Return a short density report; flag scripts with < 2 events per 4 bars."""
    duration_bars = script["duration_bars"]
    total_events = len(rendered)
    windows = math.ceil(duration_bars / 4)
    density = total_events / max(windows, 1)

    lines = ["Macro Density Analysis"]
    lines.append(f"  Total events   : {total_events}")
    lines.append(f"  Duration       : {duration_bars} bars ({windows} × 4-bar windows)")
    lines.append(f"  Events/4 bars  : {density:.2f}")

    if density < 2.0:
        lines.append(
            "  WARNING: Low density — fewer than 2 events per 4-bar window. "
            "Consider adding more automation points for an expressive performance."
        )
    else:
        lines.append("  Density OK.")

    # Per-macro summary
    lines.append("")
    lines.append("  Per-macro event counts:")
    for macro in sorted(CANONICAL_MACROS):
        count = sum(1 for e in rendered if e["macro"] == macro)
        lines.append(f"    {macro:<12}: {count}")

    return "\n".join(lines)


# ---------------------------------------------------------------------------
# MPC Q-Link automation CSV (experimental)
# ---------------------------------------------------------------------------

def generate_mpc_csv(rendered: list[dict]) -> str:
    """
    EXPERIMENTAL — MPC Q-Link automation CSV format.
    MPC does not have a documented import format for Q-Link automation;
    this CSV is a best-effort representation for future tooling.

    Columns: TimeMs, MacroName, Value (0–127 MPC range)
    """
    lines = [
        "# EXPERIMENTAL: MPC Q-Link Automation CSV",
        "# MPC does not officially support CSV automation import.",
        "# This file is provided as a data reference for custom integration.",
        "TimeMs,MacroName,MpcValue(0-127)",
    ]
    for e in rendered:
        mpc_val = round(e["value"] * 127)
        lines.append(f"{e['time_ms']:.1f},{e['macro']},{mpc_val}")
    return "\n".join(lines)


# ---------------------------------------------------------------------------
# .xometa embedding
# ---------------------------------------------------------------------------

def embed_in_xometa(xometa_path: Path, script: dict, rendered: list[dict]) -> None:
    """Add/replace 'performance_hints' key in a .xometa JSON file."""
    data = json.loads(xometa_path.read_text(encoding="utf-8"))

    data["performance_hints"] = {
        "bpm": script["bpm"],
        "duration_bars": script["duration_bars"],
        "events": [
            {
                "bar": e["bar"],
                "beat": e["beat"],
                "macro": e["macro"],
                "value": e["value"],
                "time_ms": round(e["time_ms"], 2),
            }
            for e in rendered
        ],
    }

    xometa_path.write_text(
        json.dumps(data, indent=2, ensure_ascii=False),
        encoding="utf-8",
    )
    print(f"Embedded performance_hints into {xometa_path}")


# ---------------------------------------------------------------------------
# Report assembly
# ---------------------------------------------------------------------------

def build_report(script: dict, rendered: list[dict]) -> str:
    sections = []

    sections.append("=" * 60)
    sections.append("XO_OX MACRO PERFORMANCE RECORDER REPORT")
    sections.append("=" * 60)
    sections.append(f"Preset    : {script['preset']}")
    sections.append(f"BPM       : {script['bpm']}")
    sections.append(f"Duration  : {script['duration_bars']} bars")
    sections.append(f"Events    : {len(rendered)}")
    sections.append("")

    sections.append(generate_score(script, rendered))
    sections.append("")

    sections.append(analyze_density(script, rendered))
    sections.append("")

    sections.append("Rendered Timeline (ms)")
    sections.append("-" * 40)
    for e in rendered:
        sections.append(
            f"  {e['time_ms']:>8.1f} ms  |  Bar {e['bar']} Beat {e['beat']}"
            f"  |  {e['macro']:<12}  ->  {e['value']:.3f}"
        )
    sections.append("")

    sections.append(generate_mpc_csv(rendered))
    sections.append("=" * 60)

    return "\n".join(sections)


# ---------------------------------------------------------------------------
# CLI entry point
# ---------------------------------------------------------------------------

def main() -> None:
    parser = argparse.ArgumentParser(
        description="XO_OX Macro Performance Recorder — script, visualize, and embed macro automation."
    )
    parser.add_argument(
        "--script", required=True, type=Path,
        help="Path to performance script JSON file."
    )
    parser.add_argument(
        "--preset", type=Path, default=None,
        help="Path to .xometa file (required if --embed is set)."
    )
    parser.add_argument(
        "--output", type=Path, default=None,
        help="Write report to this text file instead of stdout."
    )
    parser.add_argument(
        "--embed", action="store_true",
        help="Embed performance_hints into the .xometa file (requires --preset)."
    )
    args = parser.parse_args()

    # Load script
    if not args.script.exists():
        parser.error(f"Script file not found: {args.script}")

    try:
        script = json.loads(args.script.read_text(encoding="utf-8"))
    except json.JSONDecodeError as exc:
        parser.error(f"Invalid JSON in script file: {exc}")

    # Validate
    errors = validate_script(script)
    if errors:
        print("Validation FAILED:")
        for err in errors:
            print(f"  • {err}")
        raise SystemExit(1)

    # Render
    rendered = render_events(script)

    # Build report
    report = build_report(script, rendered)

    if args.output:
        args.output.write_text(report, encoding="utf-8")
        print(f"Report written to {args.output}")
    else:
        print(report)

    # Embed
    if args.embed:
        if not args.preset:
            parser.error("--embed requires --preset <path/to/preset.xometa>")
        if not args.preset.exists():
            parser.error(f".xometa file not found: {args.preset}")
        embed_in_xometa(args.preset, script, rendered)


if __name__ == "__main__":
    main()
