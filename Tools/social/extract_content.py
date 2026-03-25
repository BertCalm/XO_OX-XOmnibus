#!/usr/bin/env python3
"""
extract_content.py — Weekly content extraction for XOlokun's Sustainable Social Engine.

Reads git log, preset counts, coupling cookbook recipes, scripture verses, and
breadcrumb notes from this week. Outputs a structured JSON file for format_posts.py.

Usage:
    python3 Tools/social/extract_content.py
    python3 Tools/social/extract_content.py --week 2026-W12
    python3 Tools/social/extract_content.py --dry-run
"""

from __future__ import annotations

import argparse
import json
import os
import random
import re
import subprocess
import sys
from datetime import datetime, timedelta
from pathlib import Path

# ─── Repo layout ────────────────────────────────────────────────────────────
PROJECT_ROOT = Path(__file__).parent.parent.parent
SOCIAL_DIR   = Path(__file__).parent
STATE_FILE   = SOCIAL_DIR / "output" / ".state.json"
OUTPUT_DIR   = SOCIAL_DIR / "output"

SCRIPTURE_FILE   = PROJECT_ROOT / "scripture" / "the-scripture.md"
RETREATS_DIR     = PROJECT_ROOT / "scripture" / "retreats"
COOKBOOK_FILE    = PROJECT_ROOT / "Docs" / "coupling-cookbook-v1.md"
PRESETS_ROOT     = PROJECT_ROOT / "Presets" / "XOlokun"
BREADCRUMBS_DIR  = SOCIAL_DIR / "breadcrumbs"


# ─── State helpers ───────────────────────────────────────────────────────────

def load_state() -> dict:
    """Load persistent state (recipe rotation, used verses, etc.)."""
    if STATE_FILE.exists():
        try:
            with open(STATE_FILE, "r", encoding="utf-8") as f:
                return json.load(f)
        except (json.JSONDecodeError, OSError):
            pass
    return {
        "last_recipe_index": -1,
        "used_verse_hashes": [],
        "week_counter": 0,
    }


def save_state(state: dict) -> None:
    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)
    with open(STATE_FILE, "w", encoding="utf-8") as f:
        json.dump(state, f, indent=2)


# ─── Git log ─────────────────────────────────────────────────────────────────

def get_git_changelog(since_days: int = 7) -> list[dict]:
    """Return list of {hash, date, message} for commits in the last N days."""
    since = (datetime.now() - timedelta(days=since_days)).strftime("%Y-%m-%d")
    try:
        result = subprocess.run(
            [
                "git", "-C", str(PROJECT_ROOT), "log",
                f"--since={since}",
                "--pretty=format:%H|%ad|%s",
                "--date=short",
            ],
            capture_output=True, text=True, check=True,
        )
        entries = []
        for line in result.stdout.strip().splitlines():
            if not line.strip():
                continue
            parts = line.split("|", 2)
            if len(parts) == 3:
                entries.append({"hash": parts[0], "date": parts[1], "message": parts[2]})
        return entries
    except (subprocess.CalledProcessError, FileNotFoundError):
        return []


def summarize_changelog(commits: list[dict]) -> str:
    """Build a concise human-readable changelog from commit list."""
    if not commits:
        return "No commits this week."
    lines = []
    for c in commits[:10]:  # cap at 10 for social readability
        lines.append(f"• {c['message']}")
    return "\n".join(lines)


# ─── Preset counting ─────────────────────────────────────────────────────────

def count_new_presets(since_days: int = 7) -> tuple[int, list[str]]:
    """Count .xometa files added in git in the last N days. Returns (count, [names])."""
    since = (datetime.now() - timedelta(days=since_days)).strftime("%Y-%m-%d")
    try:
        result = subprocess.run(
            [
                "git", "-C", str(PROJECT_ROOT), "log",
                f"--since={since}",
                "--diff-filter=A",
                "--name-only",
                "--pretty=format:",
            ],
            capture_output=True, text=True, check=True,
        )
        files = [
            line.strip() for line in result.stdout.splitlines()
            if line.strip().endswith(".xometa")
        ]
        names = [Path(f).stem.replace("_", " ") for f in files]
        return len(files), names
    except (subprocess.CalledProcessError, FileNotFoundError):
        return 0, []


def pick_spotlight_preset(new_preset_names: list[str]) -> str | None:
    """Pick a random preset from new additions; fall back to total pool."""
    if new_preset_names:
        return random.choice(new_preset_names)
    # Fall back: scan mood dirs
    pool = []
    if PRESETS_ROOT.exists():
        for mood_dir in PRESETS_ROOT.iterdir():
            if mood_dir.is_dir() and not mood_dir.name.startswith("_"):
                for f in mood_dir.glob("*.xometa"):
                    pool.append(f.stem.replace("_", " "))
    if pool:
        return random.choice(pool)
    return None


# ─── Coupling cookbook ───────────────────────────────────────────────────────

def parse_cookbook_recipes() -> list[dict]:
    """
    Parse coupling-cookbook-v1.md for recipe blocks.
    Returns list of dicts with: name, engine_a, engine_b, coupling_type, amount,
    why_it_works, genre, direction.
    """
    if not COOKBOOK_FILE.exists():
        return []

    text = COOKBOOK_FILE.read_text(encoding="utf-8")
    recipes = []

    # Each recipe starts with "#### Recipe NN — "
    recipe_blocks = re.split(r"(?=#{3,4} Recipe \d+)", text)
    for block in recipe_blocks:
        if not re.match(r"#{3,4} Recipe \d+", block.strip()):
            continue

        def extract_table_field(pattern: str, default: str = "") -> str:
            m = re.search(pattern, block)
            return m.group(1).strip() if m else default

        name       = extract_table_field(r"\*\*Name\*\*\s*\|\s*(.+?)\s*\|")
        engine_a   = extract_table_field(r"\*\*Engine A\*\*\s*\|\s*(.+?)\s*\|")
        engine_b   = extract_table_field(r"\*\*Engine B\*\*\s*\|\s*(.+?)\s*\|")
        route1_type = extract_table_field(r"\*\*Route 1 Type\*\*\s*\|\s*`?(.+?)`?\s*\|")
        route1_amt  = extract_table_field(r"\*\*Route 1 Amount\*\*\s*\|\s*(.+?)\s*\|")
        direction  = extract_table_field(r"\*\*Direction\*\*\s*\|\s*(.+?)\s*\|")
        genre      = extract_table_field(r"\*\*Genre\*\*\s*\|\s*(.+?)\s*\|")

        why_match = re.search(r"\*\*Why It Works:\*\*\s*(.+?)(?:\n\n|\*\*Sweet Spot|\Z)", block, re.DOTALL)
        why = why_match.group(1).strip() if why_match else ""
        # Truncate for social
        why_short = why[:160].rstrip() + ("…" if len(why) > 160 else "")

        # Extract just the engine short name in parens e.g. "ONSET (XOnset) — Slot 1" → "ONSET"
        def short_name(raw: str) -> str:
            m = re.match(r"([A-Z][A-Z0-9]+)", raw)
            return m.group(1) if m else raw.split(" ")[0]

        if name:
            recipes.append({
                "name": name,
                "engine_a": engine_a,
                "engine_a_short": short_name(engine_a),
                "engine_b": engine_b,
                "engine_b_short": short_name(engine_b),
                "coupling_type": route1_type,
                "amount": route1_amt,
                "direction": direction,
                "genre": genre,
                "why_it_works": why,
                "why_it_works_short": why_short,
            })
    return recipes


def pick_next_recipe(recipes: list[dict], state: dict) -> tuple[dict | None, int]:
    """Round-robin through recipes. Returns (recipe, new_index)."""
    if not recipes:
        return None, -1
    next_idx = (state.get("last_recipe_index", -1) + 1) % len(recipes)
    return recipes[next_idx], next_idx


# ─── Scripture verses ─────────────────────────────────────────────────────────

def _collect_verses_from_file(path: Path) -> list[str]:
    """Extract individual verse/line candidates from a markdown file."""
    lines = path.read_text(encoding="utf-8").splitlines()
    verses = []
    for line in lines:
        stripped = line.strip()
        # Filter blockquotes and plain long lines
        if stripped.startswith(">"):
            stripped = stripped.lstrip("> ").strip()
        if 20 <= len(stripped) <= 280 and not stripped.startswith("#") and not stripped.startswith("|"):
            verses.append(stripped)
    return verses


def collect_all_verses() -> list[str]:
    """Read scripture file + all retreat files for verse candidates."""
    verses = []
    if SCRIPTURE_FILE.exists():
        verses.extend(_collect_verses_from_file(SCRIPTURE_FILE))
    if RETREATS_DIR.exists():
        for md_file in sorted(RETREATS_DIR.glob("*.md")):
            verses.extend(_collect_verses_from_file(md_file))
    # Deduplicate
    seen = set()
    unique = []
    for v in verses:
        if v not in seen:
            seen.add(v)
            unique.append(v)
    return unique


def pick_scripture_verse(state: dict) -> tuple[str, str]:
    """
    Pick an unused verse. Returns (verse_text, verse_hash).
    Falls back to re-using old verses if all exhausted.
    """
    verses = collect_all_verses()
    if not verses:
        return "The deep is quiet. Listen.", "fallback"

    used = set(state.get("used_verse_hashes", []))
    # Simple hash: first 40 chars stripped
    def vhash(v: str) -> str:
        return v[:40].lower().replace(" ", "_")

    available = [v for v in verses if vhash(v) not in used]
    if not available:
        # Reset and start over
        available = verses
        state["used_verse_hashes"] = []

    chosen = random.choice(available)
    return chosen, vhash(chosen)


# ─── Breadcrumbs ─────────────────────────────────────────────────────────────

def read_breadcrumbs(since_days: int = 7) -> list[dict]:
    """
    Read .txt files from breadcrumbs/ that were modified in the last N days.
    Returns list of {filename, content}.
    """
    if not BREADCRUMBS_DIR.exists():
        return []

    cutoff = datetime.now() - timedelta(days=since_days)
    crumbs = []
    for txt_file in sorted(BREADCRUMBS_DIR.glob("*.txt")):
        mtime = datetime.fromtimestamp(txt_file.stat().st_mtime)
        if mtime >= cutoff:
            content = txt_file.read_text(encoding="utf-8").strip()
            if content:
                crumbs.append({"filename": txt_file.name, "content": content})
    return crumbs


# ─── Week ID ─────────────────────────────────────────────────────────────────

def week_id(dt: datetime | None = None) -> str:
    """Return ISO week string like '2026-W12'."""
    d = dt or datetime.now()
    return f"{d.year}-W{d.isocalendar()[1]:02d}"


# ─── Main extraction ──────────────────────────────────────────────────────────

def extract(target_week: str | None = None, dry_run: bool = False) -> dict:
    wid = target_week or week_id()
    state = load_state()

    # 1. Git changelog
    commits = get_git_changelog(since_days=7)
    changelog_summary = summarize_changelog(commits)

    # 2. New presets
    preset_count, preset_names = count_new_presets(since_days=7)
    spotlight = pick_spotlight_preset(preset_names)

    # 3. Coupling recipe (round-robin)
    recipes = parse_cookbook_recipes()
    recipe, next_recipe_idx = pick_next_recipe(recipes, state)

    # 4. Scripture verse
    verse, verse_hash = pick_scripture_verse(state)

    # 5. Breadcrumbs
    crumbs = read_breadcrumbs(since_days=7)

    # Build output
    content = {
        "week":              wid,
        "generated_at":      datetime.now().isoformat(),
        "changelog": {
            "commits":       commits,
            "summary":       changelog_summary,
            "commit_count":  len(commits),
        },
        "presets": {
            "new_count":     preset_count,
            "new_names":     preset_names,
            "spotlight":     spotlight,
        },
        "recipe":            recipe,
        "scripture_verse":   verse,
        "breadcrumbs":       crumbs,
        "week_counter":      state.get("week_counter", 0) + 1,
    }

    # Persist state
    if not dry_run:
        state["last_recipe_index"] = next_recipe_idx
        state["used_verse_hashes"] = list(
            set(state.get("used_verse_hashes", [])) | {verse_hash}
        )
        state["week_counter"] = content["week_counter"]
        save_state(state)

        output_path = OUTPUT_DIR / f"content-{wid}.json"
        OUTPUT_DIR.mkdir(parents=True, exist_ok=True)
        with open(output_path, "w", encoding="utf-8") as f:
            json.dump(content, f, indent=2, ensure_ascii=False)
        print(f"[extract_content] Wrote {output_path}", file=sys.stderr)

    return content


# ─── CLI ─────────────────────────────────────────────────────────────────────

def main() -> None:
    parser = argparse.ArgumentParser(
        description="Extract weekly content for XOlokun social pipeline.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    parser.add_argument(
        "--week",
        metavar="YYYY-WNN",
        help="Target week (default: current week)",
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Print result to stdout without writing files or updating state.",
    )
    args = parser.parse_args()

    content = extract(target_week=args.week, dry_run=args.dry_run)

    if args.dry_run:
        json.dump(content, sys.stdout, indent=2, ensure_ascii=False)
        print()
    else:
        # Print brief summary
        wid = content["week"]
        cc  = content["changelog"]["commit_count"]
        pc  = content["presets"]["new_count"]
        sp  = content["presets"]["spotlight"] or "none"
        rn  = content["recipe"]["name"] if content["recipe"] else "none"
        print(f"Week {wid} | {cc} commits | {pc} new presets | spotlight: '{sp}' | recipe: '{rn}'")


if __name__ == "__main__":
    main()
