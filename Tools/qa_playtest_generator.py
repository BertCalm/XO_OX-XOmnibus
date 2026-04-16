#!/usr/bin/env python3
"""
qa_playtest_generator.py — Logic Pro playtest session plan generator for XOceanus.

Reads engine names from engine_registry.py (canonical list) and preset counts from
Presets/XOceanus/{mood}/ directories, then emits a structured markdown checklist.

Usage:
    python3 Tools/qa_playtest_generator.py
    python3 Tools/qa_playtest_generator.py --engines 5
    python3 Tools/qa_playtest_generator.py --output /tmp/playtest.md
    python3 Tools/qa_playtest_generator.py --engines 15 --output Docs/qa/custom.md
"""

from __future__ import annotations

import argparse
import json
import os
import sys
from collections import defaultdict
from datetime import date
from pathlib import Path

# ---------------------------------------------------------------------------
# Repo-root detection
# ---------------------------------------------------------------------------

SCRIPT_DIR = Path(__file__).resolve().parent
REPO_ROOT = SCRIPT_DIR.parent

# Allow imports from Tools/ without modifying sys.path when running from repo root.
if str(SCRIPT_DIR) not in sys.path:
    sys.path.insert(0, str(SCRIPT_DIR))

# ---------------------------------------------------------------------------
# Defaults
# ---------------------------------------------------------------------------

DEFAULT_ENGINES_COUNT = 10
DEFAULT_OUTPUT = REPO_ROOT / "Docs" / "qa" / "playtest-checklist.md"
PRESETS_ROOT = REPO_ROOT / "Presets" / "XOceanus"
ENGINES_DIR = REPO_ROOT / "Source" / "Engines"

# Flagship engine (always included in deep-test regardless of preset count).
FLAGSHIP = "Obrix"


# ---------------------------------------------------------------------------
# Engine discovery
# ---------------------------------------------------------------------------

def get_engines_from_registry() -> list[str]:
    """Import engine_registry.py and return canonical engine names.

    Falls back to scanning Source/Engines/ directory names if the import fails.
    """
    try:
        from engine_registry import get_all_engines  # type: ignore[import]
        engines = get_all_engines()
        print(f"[engine source] engine_registry.py — {len(engines)} engines")
        return engines
    except ImportError:
        pass

    # Fallback: scan Source/Engines/ directory
    if ENGINES_DIR.is_dir():
        engines = sorted(
            d.name for d in ENGINES_DIR.iterdir()
            if d.is_dir() and not d.name.startswith("_") and not d.name.startswith(".")
        )
        print(
            f"[engine source] Source/Engines/ scan (engine_registry.py not importable) "
            f"— {len(engines)} engines"
        )
        return engines

    print(
        "[engine source] WARNING: Neither engine_registry.py import nor Source/Engines/ "
        "scan succeeded. Returning empty list.",
        file=sys.stderr,
    )
    return []


# ---------------------------------------------------------------------------
# Preset counting
# ---------------------------------------------------------------------------

def count_presets_per_engine(presets_root: Path) -> dict[str, int]:
    """Scan all mood subdirectories under presets_root and tally presets per engine.

    Each .xometa file is a JSON object with an "engines" array.  We read only
    that field; if the file is malformed or unreadable we skip it silently.

    Returns a dict mapping canonical engine name -> total preset count.
    """
    counts: dict[str, int] = defaultdict(int)

    if not presets_root.is_dir():
        print(
            f"[presets] WARNING: Presets directory not found at {presets_root}",
            file=sys.stderr,
        )
        return dict(counts)

    mood_dirs = [
        d for d in presets_root.iterdir()
        if d.is_dir() and not d.name.startswith("_") and not d.name.startswith(".")
    ]

    total_files = 0
    skipped = 0

    for mood_dir in mood_dirs:
        for preset_file in mood_dir.glob("*.xometa"):
            total_files += 1
            try:
                with open(preset_file, "r", encoding="utf-8") as fh:
                    data = json.load(fh)
                engines_in_preset = data.get("engines", [])
                for engine in engines_in_preset:
                    if isinstance(engine, str) and engine:
                        counts[engine] += 1
            except (json.JSONDecodeError, OSError):
                skipped += 1

    print(
        f"[presets] Scanned {total_files} .xometa files across {len(mood_dirs)} moods "
        f"({skipped} skipped due to parse errors)"
    )
    return dict(counts)


def count_coupling_presets(presets_root: Path) -> list[str]:
    """Return file stems of presets in the Coupling mood directory."""
    coupling_dir = presets_root / "Coupling"
    if not coupling_dir.is_dir():
        return []
    return sorted(f.stem.replace("_", " ") for f in coupling_dir.glob("*.xometa"))


# ---------------------------------------------------------------------------
# Engine selection helpers
# ---------------------------------------------------------------------------

def select_deep_test_engines(
    all_engines: list[str],
    preset_counts: dict[str, int],
    n: int,
) -> list[tuple[str, int]]:
    """Return up to n engines for deep testing, always leading with the flagship.

    Ordering:
      1. FLAGSHIP (Obrix) first — always included if present.
      2. Remaining slots filled by preset count (descending), then alphabetically.

    Returns a list of (engine_name, preset_count) tuples.
    """
    engine_set = {e.lower(): e for e in all_engines}

    # Canonical casing for flagship
    flagship_canonical = engine_set.get(FLAGSHIP.lower(), FLAGSHIP)
    flagship_count = preset_counts.get(flagship_canonical, 0)

    # Sort remaining engines by preset count desc, then name asc
    others = [
        (e, preset_counts.get(e, 0))
        for e in all_engines
        if e.lower() != FLAGSHIP.lower()
    ]
    others.sort(key=lambda x: (-x[1], x[0].lower()))

    selected: list[tuple[str, int]] = []

    if flagship_canonical in all_engines or FLAGSHIP in all_engines:
        selected.append((flagship_canonical, flagship_count))

    for engine, count in others:
        if len(selected) >= n:
            break
        selected.append((engine, count))

    return selected[:n]


# ---------------------------------------------------------------------------
# Markdown generation
# ---------------------------------------------------------------------------

SECTION_DIVIDER = "\n---\n"


def cb(label: str) -> str:
    """Return a markdown checkbox item."""
    return f"- [ ] {label}"


def build_checklist(
    all_engines: list[str],
    preset_counts: dict[str, int],
    deep_engines: list[tuple[str, int]],
    coupling_presets: list[str],
    n_deep: int,
) -> str:
    today = date.today().isoformat()
    total_presets = sum(preset_counts.values())
    # Each preset that references N engines is counted N times above; this
    # gives a weighted total.  The raw file count is a better summary stat —
    # compute it separately.
    total_preset_files = sum(1 for _ in PRESETS_ROOT.rglob("*.xometa")) if PRESETS_ROOT.is_dir() else "?"

    lines: list[str] = []

    # ── Header ──────────────────────────────────────────────────────────────
    lines += [
        "# XOceanus Logic Pro QA Playtest Checklist",
        "",
        f"> Generated: {today}  ",
        f"> Engines in registry: **{len(all_engines)}**  ",
        f"> Total factory presets: **{total_preset_files}** (.xometa files)  ",
        f"> Deep-test engines: **{n_deep}** (flagship + top by preset count)  ",
        "",
        "## How to use this checklist",
        "",
        "1. Open Logic Pro and instantiate the XOceanus AU plugin on an instrument track.",
        "2. Work through sections top to bottom — Section 1 first (smoke test all engines), "
        "then Section 2 (deep functional test) for the priority subset.",
        "3. Mark each checkbox when the test passes.  Add inline notes after the checkbox "
        "text if you observe anything unexpected.",
        "4. File a bug for any item that cannot be checked off.",
        "",
        SECTION_DIVIDER,
    ]

    # ── Section 1: Basic Load Test ──────────────────────────────────────────
    lines += [
        "## Section 1 — Basic Load Test (all engines)",
        "",
        "For each engine: load it in Slot 1, confirm the plugin opens without crashing, "
        "the UI renders correctly, all knobs respond to mouse input, and the preset name "
        "is displayed.",
        "",
    ]

    for engine in all_engines:
        count = preset_counts.get(engine, 0)
        preset_note = f" ({count} preset{'s' if count != 1 else ''})"
        lines.append(cb(f"Load **{engine}**{preset_note} — no crash, UI renders, knobs respond, preset name displays"))

    lines.append("")
    lines.append(SECTION_DIVIDER)

    # ── Section 2: Deep Playtest ─────────────────────────────────────────────
    lines += [
        f"## Section 2 — Deep Playtest (Priority engines — top {n_deep} by preset count)",
        "",
        "For each engine below, run the full functional checklist.  "
        "The flagship (**Obrix**) must pass every item before release.",
        "",
    ]

    for engine, count in deep_engines:
        lines += [
            f"### {engine} ({count} presets)",
            "",
            cb(f"**{engine}** — play C3 note: sound produces, no click on attack"),
            cb(f"**{engine}** — play chord C3-E3-G3: polyphony works, no voice stealing artifacts"),
            cb(f"**{engine}** — hold sustain pedal (CC#64), play notes, release pedal: notes release cleanly"),
            cb(f"**{engine}** — pitch bend full up: smooth sweep upward"),
            cb(f"**{engine}** — pitch bend full down: smooth sweep downward, returns to center on release"),
            cb(f"**{engine}** — MIDI panic (CC#123): all voices stop immediately, no hanging notes"),
            cb(f"**{engine}** — load 5 different presets: each sounds distinct, no artifacts on preset switch"),
            cb(f"**{engine}** — bypass plugin toggle: clean bypass, no level change or click"),
            cb(f"**{engine}** — CPU meter at idle (single engine): stays below 15%"),
            cb(f"**{engine}** — CPU meter while playing chord: stays below 40%"),
            "",
        ]

    lines.append(SECTION_DIVIDER)

    # ── Section 3: Coupling Test ─────────────────────────────────────────────
    top_coupling = coupling_presets[:5]
    lines += [
        "## Section 3 — Coupling Test (top 5 coupling presets)",
        "",
        "Load each preset from the **Coupling** mood.  Both engines must appear in "
        "the coupling matrix UI and the coupling effect must be clearly audible.",
        "",
    ]

    if top_coupling:
        for preset_name in top_coupling:
            lines += [
                cb(f"Load coupling preset **{preset_name}** — both engines visible in UI"),
                cb(f"**{preset_name}** — play notes: coupling effect audible"),
                cb(f"**{preset_name}** — adjust coupling amount knob: smooth transition, no glitches"),
                "",
            ]
    else:
        lines += [
            "_No coupling presets found in Presets/XOceanus/Coupling/. "
            "Verify the directory exists and contains .xometa files._",
            "",
        ]

    lines.append(SECTION_DIVIDER)

    # ── Section 4: Export Test ───────────────────────────────────────────────
    lines += [
        "## Section 4 — Export Test",
        "",
        cb("Export one preset as WAV via XPN export — file plays back correctly, no truncation or silence"),
        cb("Render a 4-bar MIDI clip to audio (Logic Pro offline bounce) — result matches live playback tonally"),
        "",
        SECTION_DIVIDER,
    ]

    # ── Section 5: MIDI Verification (Issue #101) ────────────────────────────
    lines += [
        "## Section 5 — MIDI Verification (Issue #101)",
        "",
        "These tests verify MIDI contract correctness fleet-wide.  "
        "Run with any engine that has standard MIDI implementation.",
        "",
        "### Pitch Bend",
        cb("Pitch bend range — play note, bend wheel full up: should be exactly +2 semitones"),
        cb("Pitch bend range — play note, bend wheel full down: should be exactly -2 semitones"),
        cb("Pitch bend return — release wheel: returns precisely to center (no residual detune)"),
        "",
        "### Sustain Pedal (CC#64)",
        cb("Basic sustain — hold pedal, play note, release note, release pedal: note sustains then releases cleanly"),
        cb("Multi-note sustain — hold pedal, play 8 notes rapidly, release pedal: all 8 notes release, no stuck voices"),
        cb("Sustain interlock — hold pedal, play note, press pedal again while held: no double-trigger or click"),
        "",
        "### All Notes / All Sound Off",
        cb("All Notes Off (CC#123) — play chord, send CC#123: immediate silence, all voices stop"),
        cb("All Sound Off (CC#120) — play chord with reverb tail active, send CC#120: "
           "immediate silence including FX tails"),
        "",
        "### Expression & Modulation",
        cb("Channel pressure / aftertouch — if supported: play note, apply aftertouch: smooth, continuous response"),
        cb("Mod wheel (CC#1) — sweep from 0 to 127: maps to expected parameter, smooth response, no zipper noise"),
        "",
        "### Voice Integrity",
        cb("Voice stealing — exceed polyphony limit rapidly: oldest/quietest voice stolen gracefully, no crash"),
        cb("Note off timing — play rapid staccato notes at 240 BPM: no orphaned notes"),
        "",
        SECTION_DIVIDER,
    ]

    # ── Section 6: Stress Test ────────────────────────────────────────────────
    lines += [
        "## Section 6 — Stress Test",
        "",
        cb("4-engine coupling matrix — load 4 engines in coupling matrix: CPU stays below 60%, no xrun/dropout"),
        cb("Rapid preset switching — click through 20 presets in succession quickly: no crash, no frozen UI"),
        cb("Editor open/close — open and close the plugin editor 10 times in a row: no memory leak "
           "(verify in Activity Monitor — RSS should not grow continuously)"),
        cb("Session persistence — save Logic Pro session with plugin loaded, close Logic, reopen: "
           "all engine assignments, parameter values, and preset names restored correctly"),
        cb("Hot-swap under load — while playing a MIDI loop, change the engine in Slot 1: "
           "50ms crossfade, no click, no crash"),
        cb("Ghost Slot activation — load all 4 engines of a single Kitchen Collection quad "
           "(e.g., OTO + OCTAVE + OLEG + OTIS): Ghost Slot appears in UI"),
        "",
        SECTION_DIVIDER,
    ]

    # ── Sign-off ──────────────────────────────────────────────────────────────
    lines += [
        "## Sign-off",
        "",
        "| Role | Name | Date | Result |",
        "|------|------|------|--------|",
        "| Tester | | | |",
        "| QA Lead | | | |",
        "| Release Approver | | | |",
        "",
        "_All sections must be fully checked before a release build is submitted to distribution._",
        "",
    ]

    return "\n".join(lines)


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description=(
            "Generate a Logic Pro QA playtest checklist for the XOceanus synth plugin. "
            "Reads engine names from engine_registry.py and preset counts from "
            "Presets/XOceanus/."
        ),
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    parser.add_argument(
        "--engines",
        type=int,
        default=DEFAULT_ENGINES_COUNT,
        metavar="N",
        help=f"Number of engines to include in Section 2 deep-test (default: {DEFAULT_ENGINES_COUNT}). "
             "Obrix (flagship) always counts as one of the N slots.",
    )
    parser.add_argument(
        "--output",
        type=Path,
        default=DEFAULT_OUTPUT,
        metavar="PATH",
        help=f"Output path for the generated markdown file (default: {DEFAULT_OUTPUT})",
    )
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    n_deep: int = max(1, args.engines)
    output_path: Path = args.output.resolve()

    print("=" * 60)
    print("XOceanus QA Playtest Generator")
    print("=" * 60)

    # 1. Discover engines
    all_engines = get_engines_from_registry()
    if not all_engines:
        print("ERROR: No engines discovered. Aborting.", file=sys.stderr)
        sys.exit(1)

    # 2. Count presets per engine
    preset_counts = count_presets_per_engine(PRESETS_ROOT)

    # 3. Select deep-test engines
    deep_engines = select_deep_test_engines(all_engines, preset_counts, n_deep)

    # 4. Fetch coupling preset names
    coupling_presets = count_coupling_presets(PRESETS_ROOT)

    # 5. Build checklist markdown
    checklist = build_checklist(
        all_engines=all_engines,
        preset_counts=preset_counts,
        deep_engines=deep_engines,
        coupling_presets=coupling_presets,
        n_deep=n_deep,
    )

    # 6. Write output
    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_text(checklist, encoding="utf-8")

    # 7. Summary
    print()
    print("=" * 60)
    print("Generation complete")
    print("=" * 60)
    print(f"  Output:              {output_path}")
    print(f"  Total engines:       {len(all_engines)}")
    print(f"  Deep-test engines:   {n_deep}")
    print(f"  Coupling presets:    {len(coupling_presets)}")
    print(f"  Section 1 items:     {len(all_engines)}")
    print(f"  Section 2 items:     {n_deep * 10}  ({n_deep} engines × 10 checks)")
    print(f"  Section 5 items:     12  (MIDI verification — Issue #101)")
    print(f"  Section 6 items:     6   (stress tests)")
    print()
    print("Deep-test engines selected:")
    for i, (engine, count) in enumerate(deep_engines, 1):
        tag = " [flagship]" if engine.lower() == FLAGSHIP.lower() else ""
        print(f"  {i:2}. {engine:<20} {count:>5} presets{tag}")
    print()


if __name__ == "__main__":
    main()
