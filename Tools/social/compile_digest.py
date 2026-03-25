#!/usr/bin/env python3
"""
compile_digest.py — Monthly digest compiler for XOlokun.

Reads the last 4 weeks of output/week-{N}/ directories and compiles
a Patreon-ready monthly digest. Also supports custom date ranges.

Usage:
    python3 Tools/social/compile_digest.py
    python3 Tools/social/compile_digest.py --month 2026-04
    python3 Tools/social/compile_digest.py --weeks 1 2 3 4
    python3 Tools/social/compile_digest.py --month 2026-03 --format discord
    python3 Tools/social/compile_digest.py --dry-run
"""

from __future__ import annotations

import argparse
import json
import re
import sys
from datetime import datetime, date
from pathlib import Path

# ─── Paths ───────────────────────────────────────────────────────────────────
PROJECT_ROOT = Path(__file__).parent.parent.parent
SOCIAL_DIR   = Path(__file__).parent
OUTPUT_DIR   = SOCIAL_DIR / "output"
TEMPLATES_DIR = SOCIAL_DIR / "templates"

PATREON_TEMPLATE_FILE = TEMPLATES_DIR / "patreon.md"

SUPPORTED_FORMATS = ["patreon", "discord", "plain"]


# ─── Week directory helpers ──────────────────────────────────────────────────

def list_week_dirs() -> list[Path]:
    """Return all output/week-NNN/ directories sorted by number."""
    dirs = sorted(
        OUTPUT_DIR.glob("week-*"),
        key=lambda p: int(p.name.split("-")[1]) if p.name.split("-")[1].isdigit() else 0,
    )
    return [d for d in dirs if d.is_dir()]


def load_week_index(week_dir: Path) -> dict | None:
    index_path = week_dir / "index.json"
    if not index_path.exists():
        return None
    try:
        with open(index_path, "r", encoding="utf-8") as f:
            return json.load(f)
    except (json.JSONDecodeError, OSError):
        return None


def load_content_for_week(week_id: str) -> dict | None:
    content_file = OUTPUT_DIR / f"content-{week_id}.json"
    if not content_file.exists():
        return None
    try:
        with open(content_file, "r", encoding="utf-8") as f:
            return json.load(f)
    except (json.JSONDecodeError, OSError):
        return None


def load_post_text(week_dir: Path, platform: str, zone: str) -> str:
    """Load a specific post text from a week directory."""
    post_file = week_dir / f"{platform}-{zone}.txt"
    if post_file.exists():
        return post_file.read_text(encoding="utf-8").strip()
    return ""


# ─── Month filtering ─────────────────────────────────────────────────────────

def weeks_in_month(month_str: str) -> list[str]:
    """
    Return ISO week IDs (YYYY-WNN) that fall within a given calendar month.
    month_str format: "2026-04"
    """
    try:
        year, month = int(month_str[:4]), int(month_str[5:7])
    except (ValueError, IndexError):
        return []

    weeks = set()
    # Walk every day of the month
    for day in range(1, 32):
        try:
            d = date(year, month, day)
        except ValueError:
            break
        iso = d.isocalendar()
        weeks.add(f"{iso[0]}-W{iso[1]:02d}")

    return sorted(weeks)


def get_last_n_week_dirs(n: int) -> list[Path]:
    """Return the most recent N week directories."""
    return list_week_dirs()[-n:]


def get_week_dirs_for_month(month_str: str) -> list[Path]:
    """Return week directories whose week_id falls within the given month."""
    target_weeks = set(weeks_in_month(month_str))
    result = []
    for week_dir in list_week_dirs():
        index = load_week_index(week_dir)
        if index and index.get("week_id") in target_weeks:
            result.append(week_dir)
    return result


def get_specific_week_dirs(week_nums: list[int]) -> list[Path]:
    """Return week directories for specific week numbers."""
    result = []
    for wnum in week_nums:
        candidate = OUTPUT_DIR / f"week-{wnum:03d}"
        if candidate.exists():
            result.append(candidate)
    return result


# ─── Digest assembly ─────────────────────────────────────────────────────────

def aggregate_week_data(week_dirs: list[Path]) -> dict:
    """
    Aggregate content data across all week directories.
    Returns a summary dict for template rendering.
    """
    total_commits  = 0
    total_presets  = 0
    all_spotlights = []
    all_recipes    = []
    all_verses     = []
    all_crumbs     = []
    week_ids       = []

    for week_dir in week_dirs:
        index = load_week_index(week_dir)
        if not index:
            continue
        wid = index.get("week_id", "")
        week_ids.append(wid)

        content = load_content_for_week(wid)
        if not content:
            continue

        total_commits += content.get("changelog", {}).get("commit_count", 0)
        total_presets += content.get("presets", {}).get("new_count", 0)

        sp = content.get("presets", {}).get("spotlight")
        if sp:
            all_spotlights.append(sp)

        recipe = content.get("recipe")
        if recipe and recipe.get("name"):
            all_recipes.append(recipe)

        verse = content.get("scripture_verse")
        if verse:
            all_verses.append(verse)

        all_crumbs.extend(content.get("breadcrumbs") or [])

    return {
        "week_ids":       week_ids,
        "week_count":     len(week_dirs),
        "total_commits":  total_commits,
        "total_presets":  total_presets,
        "spotlights":     all_spotlights,
        "recipes":        all_recipes,
        "verses":         all_verses,
        "breadcrumbs":    all_crumbs,
    }


# ─── Patreon digest renderer ──────────────────────────────────────────────────

def render_patreon_digest(data: dict, month_label: str) -> str:
    """Render a full Patreon monthly digest post."""
    week_range = ""
    if data["week_ids"]:
        first, last = data["week_ids"][0], data["week_ids"][-1]
        week_range  = first if first == last else f"{first} → {last}"

    recipes_block = ""
    for recipe in data["recipes"]:
        name = recipe.get("name", "Unknown Recipe")
        ea   = recipe.get("engine_a_short", recipe.get("engine_a", "?"))
        eb   = recipe.get("engine_b_short", recipe.get("engine_b", "?"))
        ct   = recipe.get("coupling_type", "?")
        why  = recipe.get("why_it_works_short", "")
        recipes_block += f"**{name}** — {ea} × {eb} via {ct}\n{why}\n\n"

    verses_block = ""
    for v in data["verses"]:
        verses_block += f"> {v}\n\n"

    spotlights_block = ""
    for sp in data["spotlights"]:
        spotlights_block += f"• {sp}\n"

    crumbs_block = ""
    for crumb in data["breadcrumbs"]:
        crumbs_block += f"> {crumb.get('content', '').splitlines()[0]}\n"

    digest = f"""# XOlokun Monthly Dispatch — {month_label}

*{data['week_count']} weeks | {week_range}*

---

## This Month in XOlokun

This month we shipped **{data['total_commits']} commits** across the fleet and added **{data['total_presets']} new presets** to the factory library.

---

## Coupling Recipes of the Month

{recipes_block.strip() or "No recipes spotlighted this month."}

---

## Preset Spotlights

{spotlights_block.strip() or "No spotlights this month."}

---

## From the Book of Bin

{verses_block.strip() or "The deep is quiet this month."}

---

## Notes from the Studio

{crumbs_block.strip() or "Nothing in the breadcrumbs this month — just the work."}

---

*XOlokun — for all. Free and open-source.*
*patreon.com/cw/XO_OX | discord.gg/xolokun | reddit.com/r/XO_OX*
"""
    return digest.strip()


def render_discord_digest(data: dict, month_label: str) -> str:
    """Render a condensed Discord-formatted monthly summary."""
    recipe_lines = ""
    for recipe in data["recipes"]:
        ea = recipe.get("engine_a_short", "?")
        eb = recipe.get("engine_b_short", "?")
        ct = recipe.get("coupling_type", "?")
        name = recipe.get("name", "Recipe")
        recipe_lines += f"  • **{name}**: {ea} × {eb} | `{ct}`\n"

    verse = data["verses"][0] if data["verses"] else "The deep is quiet."

    return f"""**XOlokun Monthly Dispatch — {month_label}** 🐚

{data['week_count']} weeks | {data['total_commits']} commits | {data['total_presets']} new presets

**Coupling Recipes:**
{recipe_lines.strip() or "  (none this month)"}

**Verse of the Month:**
*"{verse}"*

Full dispatch on Patreon → patreon.com/cw/XO_OX"""


def render_plain_digest(data: dict, month_label: str) -> str:
    """Plain text version."""
    return (
        f"XOlokun Monthly Dispatch — {month_label}\n"
        f"Weeks: {', '.join(data['week_ids'])}\n"
        f"Commits: {data['total_commits']}  |  New presets: {data['total_presets']}\n"
        f"Recipes: {', '.join(r.get('name', '?') for r in data['recipes']) or 'none'}\n"
    )


def render_digest(data: dict, month_label: str, fmt: str) -> str:
    if fmt == "discord":
        return render_discord_digest(data, month_label)
    if fmt == "plain":
        return render_plain_digest(data, month_label)
    return render_patreon_digest(data, month_label)


# ─── Output writer ────────────────────────────────────────────────────────────

def write_digest(digest_text: str, month_label: str, fmt: str) -> Path:
    """Write digest to output/digests/."""
    digest_dir = OUTPUT_DIR / "digests"
    digest_dir.mkdir(parents=True, exist_ok=True)
    slug = month_label.replace(" ", "-").lower()
    out_file = digest_dir / f"digest-{slug}-{fmt}.txt"
    out_file.write_text(digest_text, encoding="utf-8")
    return out_file


# ─── CLI ─────────────────────────────────────────────────────────────────────

def main() -> None:
    parser = argparse.ArgumentParser(
        description="Compile monthly digest from weekly social content.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    parser.add_argument(
        "--month",
        metavar="YYYY-MM",
        help="Month to compile (default: current month, last 4 weeks).",
    )
    parser.add_argument(
        "--weeks",
        metavar="N",
        nargs="+",
        type=int,
        help="Specific week numbers to include (e.g. --weeks 1 2 3 4).",
    )
    parser.add_argument(
        "--last",
        metavar="N",
        type=int,
        default=4,
        help="Use the last N weeks (default: 4). Ignored if --month or --weeks given.",
    )
    parser.add_argument(
        "--format", "-f",
        choices=SUPPORTED_FORMATS,
        default="patreon",
        help="Output format (default: patreon).",
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Print digest to stdout without writing files.",
    )
    args = parser.parse_args()

    # ── Resolve week directories ─────────────────────────────────────────────
    if args.weeks:
        week_dirs = get_specific_week_dirs(args.weeks)
        month_label = f"Weeks {args.weeks[0]}–{args.weeks[-1]}"
    elif args.month:
        week_dirs = get_week_dirs_for_month(args.month)
        month_label = args.month
    else:
        week_dirs = get_last_n_week_dirs(args.last)
        month_label = datetime.now().strftime("%B %Y")

    if not week_dirs:
        print(
            "[compile_digest] No week directories found. "
            "Run generate_week.py first.",
            file=sys.stderr,
        )
        sys.exit(1)

    print(f"[compile_digest] Compiling {len(week_dirs)} week(s): {month_label}", file=sys.stderr)

    # ── Aggregate ────────────────────────────────────────────────────────────
    data = aggregate_week_data(week_dirs)
    digest_text = render_digest(data, month_label, args.format)

    # ── Output ───────────────────────────────────────────────────────────────
    if args.dry_run:
        print(digest_text)
        return

    out_file = write_digest(digest_text, month_label, args.format)
    print(f"[compile_digest] Wrote digest to {out_file.relative_to(PROJECT_ROOT)}")

    # Brief stats
    print(
        f"[compile_digest] Summary: {data['week_count']} weeks | "
        f"{data['total_commits']} commits | {data['total_presets']} new presets | "
        f"{len(data['recipes'])} recipes | {len(data['verses'])} verses"
    )


if __name__ == "__main__":
    main()
