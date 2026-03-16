#!/usr/bin/env python3
"""
xpn_engine_tag_cloud_builder.py

Aggregate preset tags across the XOmnibus fleet and generate:
  - Per-engine top-15 tags with ASCII bar chart
  - Fleet-wide top-30 tags
  - Orphan tags (appear only once across fleet)
  - Under-tagged preset report (presets with 0-2 tags)

Usage:
  python3 Tools/xpn_engine_tag_cloud_builder.py
  python3 Tools/xpn_engine_tag_cloud_builder.py --engine Drift
  python3 Tools/xpn_engine_tag_cloud_builder.py --min-tags 3
  python3 Tools/xpn_engine_tag_cloud_builder.py --presets-dir /path/to/Presets/XOmnibus
"""

import argparse
import json
import os
import sys
from collections import Counter, defaultdict
from pathlib import Path


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def load_xometa_files(presets_dir: Path):
    """Yield (path, data) for every .xometa file under presets_dir."""
    for root, _dirs, files in os.walk(presets_dir):
        for fname in files:
            if fname.endswith(".xometa"):
                fpath = Path(root) / fname
                try:
                    with open(fpath, encoding="utf-8") as fh:
                        data = json.load(fh)
                    yield fpath, data
                except (json.JSONDecodeError, OSError) as exc:
                    print(f"  [WARN] Could not read {fpath}: {exc}", file=sys.stderr)


def engine_label(data: dict, fpath: Path) -> str:
    """Return a single engine label for grouping.

    Uses the first entry in the 'engines' list when present; falls back to
    the parent directory name (mood folder), then 'Unknown'.
    """
    engines = data.get("engines") or []
    if engines and isinstance(engines, list) and engines[0]:
        return str(engines[0])
    # Fallback: use mood field
    mood = data.get("mood")
    if mood:
        return str(mood)
    return fpath.parent.name or "Unknown"


def ascii_bar(count: int, max_count: int, width: int = 40) -> str:
    if max_count == 0:
        return ""
    filled = round(count / max_count * width)
    return "█" * filled + "░" * (width - filled)


def print_section(title: str):
    border = "=" * (len(title) + 4)
    print(f"\n{border}")
    print(f"  {title}")
    print(border)


def print_tag_table(counter: Counter, top_n: int, label: str = ""):
    if label:
        print(f"\n  {label}")
    entries = counter.most_common(top_n)
    if not entries:
        print("  (no tags found)")
        return
    max_count = entries[0][1]
    max_tag_len = max(len(t) for t, _ in entries)
    for tag, count in entries:
        bar = ascii_bar(count, max_count)
        print(f"  {tag:<{max_tag_len}}  {count:>4}  {bar}")


# ---------------------------------------------------------------------------
# Main analysis
# ---------------------------------------------------------------------------

def build_report(presets_dir: Path, engine_filter, min_tags: int):
    # Structures
    fleet_tags: Counter = Counter()
    engine_tags: dict[str, Counter] = defaultdict(Counter)
    under_tagged: list[tuple[str, str, int]] = []  # (engine, preset_name, tag_count)
    total_presets = 0
    missing_tag_count = 0  # presets with zero tags

    for fpath, data in load_xometa_files(presets_dir):
        eng = engine_label(data, fpath)

        if engine_filter and eng.lower() != engine_filter.lower():
            continue

        total_presets += 1
        preset_name = data.get("name") or fpath.stem
        tags = data.get("tags") or []

        if not isinstance(tags, list):
            tags = []

        tag_count = len(tags)

        if tag_count == 0:
            missing_tag_count += 1

        if tag_count <= min_tags:
            under_tagged.append((eng, preset_name, tag_count))

        for tag in tags:
            tag_lower = str(tag).lower().strip()
            if tag_lower:
                fleet_tags[tag_lower] += 1
                engine_tags[eng][tag_lower] += 1

    return fleet_tags, engine_tags, under_tagged, total_presets, missing_tag_count


def run(args):
    presets_dir = Path(args.presets_dir)
    if not presets_dir.is_dir():
        print(f"[ERROR] Presets directory not found: {presets_dir}", file=sys.stderr)
        sys.exit(1)

    engine_filter = args.engine or None
    min_tags = args.min_tags  # report presets with <= this many tags

    print(f"Scanning: {presets_dir}")
    if engine_filter:
        print(f"Engine filter: {engine_filter}")
    print(f"Under-tagged threshold: <= {min_tags} tags")

    fleet_tags, engine_tags, under_tagged, total_presets, missing_tag_count = build_report(
        presets_dir, engine_filter, min_tags
    )

    # ------------------------------------------------------------------
    # Per-engine reports
    # ------------------------------------------------------------------
    print_section("PER-ENGINE TAG CLOUDS (top 15)")

    sorted_engines = sorted(engine_tags.keys())
    for eng in sorted_engines:
        ctr = engine_tags[eng]
        total_eng_tags = sum(ctr.values())
        unique_eng_tags = len(ctr)
        print(f"\n  ── {eng} ──  ({total_eng_tags} tag uses, {unique_eng_tags} unique)")
        print_tag_table(ctr, top_n=15)

    # ------------------------------------------------------------------
    # Fleet-wide top 30
    # ------------------------------------------------------------------
    print_section("FLEET-WIDE TOP 30 TAGS")
    print_tag_table(fleet_tags, top_n=30)

    # ------------------------------------------------------------------
    # Orphan tags
    # ------------------------------------------------------------------
    print_section("ORPHAN TAGS (appear only once)")
    orphans = sorted(tag for tag, cnt in fleet_tags.items() if cnt == 1)
    if orphans:
        col_width = 20
        cols = 4
        for i in range(0, len(orphans), cols):
            row = orphans[i : i + cols]
            print("  " + "  ".join(f"{t:<{col_width}}" for t in row))
        print(f"\n  Total orphan tags: {len(orphans)}")
    else:
        print("  None — every tag appears more than once.")

    # ------------------------------------------------------------------
    # Under-tagged presets
    # ------------------------------------------------------------------
    print_section(f"UNDER-TAGGED PRESETS (<= {min_tags} tags)")
    if under_tagged:
        under_tagged.sort(key=lambda x: (x[2], x[0], x[1]))
        max_eng_len = max(len(e) for e, _, _ in under_tagged)
        for eng, name, count in under_tagged:
            tag_label = "no tags" if count == 0 else f"{count} tag{'s' if count != 1 else ''}"
            print(f"  [{tag_label:<7}]  {eng:<{max_eng_len}}  {name}")
        print(f"\n  Total under-tagged: {len(under_tagged)} of {total_presets} presets")
    else:
        print(f"  All {total_presets} presets have more than {min_tags} tags.")

    # ------------------------------------------------------------------
    # Summary
    # ------------------------------------------------------------------
    print_section("FLEET SUMMARY")
    print(f"  Total presets scanned : {total_presets}")
    print(f"  Presets with 0 tags   : {missing_tag_count}")
    print(f"  Unique tags (fleet)   : {len(fleet_tags)}")
    print(f"  Total tag uses        : {sum(fleet_tags.values())}")
    if total_presets:
        avg = sum(fleet_tags.values()) / total_presets
        print(f"  Avg tags per preset   : {avg:.1f}")
    print(f"  Orphan tags           : {len([t for t, c in fleet_tags.items() if c == 1])}")
    if engine_tags:
        print(f"  Engines represented   : {len(engine_tags)}")


# ---------------------------------------------------------------------------
# Entry point
# ---------------------------------------------------------------------------

def main():
    default_presets = (
        Path(__file__).parent.parent / "Presets" / "XOmnibus"
    )

    parser = argparse.ArgumentParser(
        description="Generate tag frequency clouds for the XOmnibus preset fleet."
    )
    parser.add_argument(
        "--presets-dir",
        default=str(default_presets),
        help="Path to Presets/XOmnibus directory (default: auto-detected relative to script)",
    )
    parser.add_argument(
        "--engine",
        default=None,
        help="Filter to a single engine name (case-insensitive, e.g. 'Drift')",
    )
    parser.add_argument(
        "--min-tags",
        type=int,
        default=2,
        help="Report presets with this many tags or fewer as under-tagged (default: 2)",
    )

    args = parser.parse_args()
    run(args)
    sys.exit(0)


if __name__ == "__main__":
    main()
