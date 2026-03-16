#!/usr/bin/env python3
"""
xpn_preset_evolution_tracker.py
--------------------------------
Tracks XOmnibus preset fleet evolution over git history.

Usage:
    python xpn_preset_evolution_tracker.py [--repo-dir .] [--output timeline.txt] [--samples 50]
"""

import argparse
import json
import re
import subprocess
import sys
from collections import defaultdict
from datetime import datetime, timedelta
from pathlib import Path


# ---------------------------------------------------------------------------
# Git helpers
# ---------------------------------------------------------------------------

def run_git(args: list[str], cwd: Path) -> str:
    result = subprocess.run(
        ["git"] + args,
        cwd=str(cwd),
        capture_output=True,
        text=True,
    )
    if result.returncode != 0:
        raise RuntimeError(f"git {' '.join(args)} failed:\n{result.stderr.strip()}")
    return result.stdout.strip()


def get_commits_for_path(repo_dir: Path, path: str = "Presets/") -> list[dict]:
    """Return all commits (hash, date, subject) that touched *path*."""
    log = run_git(
        ["log", "--format=%H\x1f%ai\x1f%s", "--", path],
        cwd=repo_dir,
    )
    if not log:
        return []
    commits = []
    for line in log.splitlines():
        parts = line.split("\x1f", 2)
        if len(parts) != 3:
            continue
        sha, date_str, subject = parts
        try:
            # Python 3.9 fromisoformat doesn't handle " -0500" timezone offsets;
            # use strptime with %z which accepts "+HHMM" / "-HHMM" form.
            raw = date_str.strip()
            # git %ai format: "2026-03-16 04:06:11 -0500"
            # Collapse the space before tz offset so strptime %z works uniformly.
            raw_compact = re.sub(r" ([+-]\d{4})$", r"\1", raw)
            try:
                dt = datetime.strptime(raw_compact, "%Y-%m-%d %H:%M:%S%z")
            except ValueError:
                dt = datetime.strptime(raw_compact, "%Y-%m-%dT%H:%M:%S%z")
        except ValueError:
            continue
        commits.append({"sha": sha.strip(), "date": dt, "subject": subject.strip()})
    # oldest first
    commits.sort(key=lambda c: c["date"])
    return commits


def count_xometa_at_commit(repo_dir: Path, sha: str, preset_path: str = "Presets/") -> int:
    """Count .xometa files in the tree at *sha*."""
    try:
        output = run_git(
            ["ls-tree", "-r", "--name-only", sha, "--", preset_path],
            cwd=repo_dir,
        )
    except RuntimeError:
        return 0
    return sum(1 for line in output.splitlines() if line.endswith(".xometa"))


def list_engines_at_commit(repo_dir: Path, sha: str, preset_path: str = "Presets/") -> set[str]:
    """Return the set of engine names (top-level subdirs under preset_path) at *sha*."""
    try:
        output = run_git(
            ["ls-tree", "--name-only", sha, "--", preset_path],
            cwd=repo_dir,
        )
    except RuntimeError:
        return set()
    engines = set()
    for name in output.splitlines():
        name = name.strip().rstrip("/")
        if name:
            engines.add(name)
    return engines


# ---------------------------------------------------------------------------
# Analysis
# ---------------------------------------------------------------------------

def sample_commits(commits: list[dict], n_samples: int) -> list[dict]:
    """Return up to *n_samples* evenly-spaced commits + always include first and last."""
    if len(commits) <= n_samples:
        return list(commits)
    step = max(1, len(commits) // n_samples)
    sampled = commits[::step]
    if commits[-1] not in sampled:
        sampled.append(commits[-1])
    return sampled


def build_growth_curve(
    repo_dir: Path,
    commits: list[dict],
    preset_path: str,
    n_samples: int,
) -> list[dict]:
    """For a sampled subset of commits, record (date, sha, subject, count)."""
    sampled = sample_commits(commits, n_samples)
    curve = []
    prev_count = 0
    for c in sampled:
        count = count_xometa_at_commit(repo_dir, c["sha"], preset_path)
        delta = count - prev_count
        curve.append({
            "sha": c["sha"],
            "date": c["date"],
            "subject": c["subject"],
            "count": count,
            "delta": delta,
        })
        prev_count = count
    return curve


def find_inflection_points(curve: list[dict], threshold: int = 50) -> list[dict]:
    """Commits where preset count grew by more than *threshold*."""
    return [pt for pt in curve if pt["delta"] >= threshold]


def detect_engine_additions(repo_dir: Path, commits: list[dict], preset_path: str) -> list[dict]:
    """Walk commits and record when a new engine directory first appeared."""
    seen: set[str] = set()
    events = []
    for c in commits:
        engines = list_engines_at_commit(repo_dir, c["sha"], preset_path)
        new = engines - seen
        if new:
            events.append({
                "date": c["date"],
                "sha": c["sha"],
                "subject": c["subject"],
                "new_engines": sorted(new),
            })
            seen |= new
    return events


def detect_waves(commits: list[dict], gap_hours: float = 12.0) -> list[dict]:
    """Group commits into 'waves' (build sessions) separated by gaps >= *gap_hours*."""
    if not commits:
        return []
    waves = []
    current = [commits[0]]
    for c in commits[1:]:
        delta = (c["date"] - current[-1]["date"]).total_seconds() / 3600
        if delta > gap_hours:
            waves.append(current)
            current = [c]
        else:
            current.append(c)
    waves.append(current)
    result = []
    for w in waves:
        result.append({
            "start": w[0]["date"],
            "end": w[-1]["date"],
            "commits": len(w),
            "subjects": [c["subject"] for c in w],
        })
    return result


# ---------------------------------------------------------------------------
# ASCII chart
# ---------------------------------------------------------------------------

def ascii_bar_chart(curve: list[dict], width: int = 50) -> list[str]:
    """Render a horizontal ASCII bar chart of preset counts over time."""
    if not curve:
        return []
    max_count = max(pt["count"] for pt in curve) or 1
    lines = []
    lines.append(f"{'Date':<12}  {'Count':>6}  Chart")
    lines.append("-" * (12 + 2 + 6 + 2 + width + 4))
    for pt in curve:
        bar_len = int(pt["count"] / max_count * width)
        bar = "█" * bar_len
        date_str = pt["date"].strftime("%Y-%m-%d")
        lines.append(f"{date_str:<12}  {pt['count']:>6}  {bar}")
    return lines


# ---------------------------------------------------------------------------
# Report
# ---------------------------------------------------------------------------

def build_report(
    repo_dir: Path,
    preset_path: str,
    n_samples: int,
) -> str:
    lines: list[str] = []

    def h(title: str):
        lines.append("")
        lines.append("=" * 70)
        lines.append(f"  {title}")
        lines.append("=" * 70)

    def add(text: str = ""):
        lines.append(text)

    h("XOmnibus Preset Fleet — Evolution Tracker")
    add(f"Repo     : {repo_dir.resolve()}")
    add(f"Path     : {preset_path}")
    add(f"Generated: {datetime.now().strftime('%Y-%m-%d %H:%M')}")

    # --- fetch commits ---
    add()
    add("Fetching commit history…")
    all_commits = get_commits_for_path(repo_dir, preset_path)
    if not all_commits:
        add("No commits found touching that path. Exiting.")
        return "\n".join(lines)
    add(f"Total commits touching {preset_path}: {len(all_commits)}")

    # --- growth curve ---
    add()
    add(f"Sampling {n_samples} commits for growth curve…")
    curve = build_growth_curve(repo_dir, all_commits, preset_path, n_samples)

    h("GROWTH CURVE (date → preset count)")
    for row in ascii_bar_chart(curve):
        add(row)

    # --- inflection points ---
    h("INFLECTION POINTS  (commits adding ≥50 presets)")
    inflections = find_inflection_points(curve, threshold=50)
    if inflections:
        for pt in inflections:
            add(f"  {pt['date'].strftime('%Y-%m-%d')}  +{pt['delta']:>4}  {pt['sha'][:8]}  {pt['subject'][:60]}")
    else:
        add("  None found in sampled commits (try --samples with a larger value).")

    # --- statistics ---
    h("GROWTH STATISTICS")
    first = curve[0]
    last = curve[-1]
    span_days = (last["date"] - first["date"]).days or 1
    add(f"  First sampled commit : {first['date'].strftime('%Y-%m-%d')}  ({first['count']} presets)")
    add(f"  Latest sampled commit: {last['date'].strftime('%Y-%m-%d')}  ({last['count']} presets)")
    add(f"  Net growth           : +{last['count'] - first['count']} presets over {span_days} days")
    add(f"  Avg per day          : {(last['count'] - first['count']) / span_days:.1f}")
    if inflections:
        biggest = max(inflections, key=lambda p: p["delta"])
        add(f"  Biggest single jump  : +{biggest['delta']} on {biggest['date'].strftime('%Y-%m-%d')} ({biggest['subject'][:55]})")

    # --- engine additions ---
    h("ENGINE ADDITION TIMELINE")
    engine_events = detect_engine_additions(repo_dir, all_commits, preset_path)
    if engine_events:
        for ev in engine_events:
            engines_str = ", ".join(ev["new_engines"])
            add(f"  {ev['date'].strftime('%Y-%m-%d')}  {ev['sha'][:8]}  +[{engines_str}]")
            add(f"             {ev['subject'][:62]}")
    else:
        add("  (no engine directory changes detected in sampled commits)")

    # --- waves ---
    h("BUILD WAVES  (commit clusters within 12-hour windows)")
    waves = detect_waves(all_commits, gap_hours=12.0)
    add(f"  {len(waves)} wave(s) detected across {len(all_commits)} total commits")
    add()
    for i, w in enumerate(waves, 1):
        duration_h = (w["end"] - w["start"]).total_seconds() / 3600
        add(f"  Wave {i:>3} — {w['start'].strftime('%Y-%m-%d %H:%M')} → {w['end'].strftime('%H:%M')}  "
            f"({w['commits']} commits, {duration_h:.1f}h)")
        for s in w["subjects"][:3]:
            add(f"           · {s[:65]}")
        if len(w["subjects"]) > 3:
            add(f"           … +{len(w['subjects']) - 3} more")

    # --- full commit log ---
    h("FULL COMMIT LOG  (all commits touching Presets/)")
    add(f"  {'Date':<12}  {'SHA':<9}  Subject")
    add(f"  {'-'*12}  {'-'*9}  {'-'*45}")
    for c in reversed(all_commits):  # newest first for readability
        add(f"  {c['date'].strftime('%Y-%m-%d')}  {c['sha'][:8]}  {c['subject'][:64]}")

    add()
    add("=" * 70)
    add("  End of report")
    add("=" * 70)
    return "\n".join(lines)


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def main():
    parser = argparse.ArgumentParser(
        description="Track XOmnibus preset fleet evolution via git history.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    parser.add_argument(
        "--repo-dir",
        default=".",
        help="Path to the git repository root (default: current directory).",
    )
    parser.add_argument(
        "--preset-path",
        default="Presets/",
        help="Subdirectory within the repo to analyse (default: Presets/).",
    )
    parser.add_argument(
        "--output",
        default=None,
        help="Write report to this file (default: print to stdout).",
    )
    parser.add_argument(
        "--samples",
        type=int,
        default=50,
        help="Number of commits to sample for the growth curve (default: 50).",
    )
    args = parser.parse_args()

    repo_dir = Path(args.repo_dir).resolve()
    if not (repo_dir / ".git").exists():
        # Walk up to find repo root
        candidate = repo_dir
        while candidate != candidate.parent:
            if (candidate / ".git").exists():
                repo_dir = candidate
                break
            candidate = candidate.parent
        else:
            print(f"ERROR: No git repository found at or above {args.repo_dir}", file=sys.stderr)
            sys.exit(1)

    try:
        report = build_report(
            repo_dir=repo_dir,
            preset_path=args.preset_path,
            n_samples=args.samples,
        )
    except RuntimeError as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        sys.exit(1)

    if args.output:
        out_path = Path(args.output)
        out_path.write_text(report, encoding="utf-8")
        print(f"Report written to {out_path.resolve()}")
    else:
        print(report)


if __name__ == "__main__":
    main()
