#!/usr/bin/env python3
"""
generate_week.py — Master weekly content generation script.

Runs extract_content.py then format_posts.py in sequence.
Creates output/week-{N}/ with all posts ready for review.

Usage:
    python3 Tools/social/generate_week.py
    python3 Tools/social/generate_week.py --week 2026-W12
    python3 Tools/social/generate_week.py --dry-run
    python3 Tools/social/generate_week.py --depth surface
    python3 Tools/social/generate_week.py --post-discord  # also posts surface Discord post

Sunday assembly workflow:
    1. Drop any notes into Tools/social/breadcrumbs/note-YYYY-MM-DD.txt
    2. Run: python3 Tools/social/generate_week.py
    3. Review output/week-NNN/ directory
    4. Edit any posts you want to adjust
    5. Schedule/post each platform file
"""

from __future__ import annotations

import argparse
import json
import sys
import os
from datetime import datetime
from pathlib import Path

# ─── Paths ───────────────────────────────────────────────────────────────────
PROJECT_ROOT = Path(__file__).parent.parent.parent
SOCIAL_DIR   = Path(__file__).parent
OUTPUT_DIR   = SOCIAL_DIR / "output"

# Import sibling scripts as modules
sys.path.insert(0, str(SOCIAL_DIR))
import extract_content
import format_posts


# ─── Week summary printer ─────────────────────────────────────────────────────

def print_week_summary(content: dict, week_dir: Path) -> None:
    """Print a formatted assembly summary to stdout."""
    sep = "─" * 60
    wid   = content.get("week", "?")
    wnum  = content.get("week_counter", 1)
    cc    = content["changelog"]["commit_count"]
    pc    = content["presets"]["new_count"]
    sp    = content["presets"]["spotlight"] or "none"
    verse = content.get("scripture_verse", "")[:80]
    crumbs = content.get("breadcrumbs") or []
    recipe = content.get("recipe") or {}

    print(f"\n{sep}")
    print(f"  XOceanus Weekly Assembly — {wid} (Week #{wnum})")
    print(sep)
    print(f"  Commits:        {cc}")
    print(f"  New presets:    {pc}")
    print(f"  Spotlight:      {sp}")
    print(f"  Recipe:         {recipe.get('name', 'none')}")
    print(f"  Verse preview:  {verse}{'…' if len(verse) == 80 else ''}")
    print(f"  Breadcrumbs:    {len(crumbs)} note(s)")
    print(sep)

    # List generated files
    index_path = week_dir / "index.json"
    if index_path.exists():
        with open(index_path) as f:
            index = json.load(f)
        print(f"\n  Generated {len(index.get('files', []))} post files in:")
        print(f"  {week_dir.relative_to(PROJECT_ROOT)}/")
        for fp in index.get("files", []):
            filename = Path(fp).name
            print(f"    ✓ {filename}")
    print()


# ─── Discord auto-post ────────────────────────────────────────────────────────

def post_surface_to_discord(week_dir: Path) -> bool:
    """
    Post the discord-surface.txt to Discord via discord_webhook.py.
    Returns True on success.
    """
    post_file = week_dir / "discord-surface.txt"
    if not post_file.exists():
        print("[generate_week] No discord-surface.txt found — skipping Discord post.", file=sys.stderr)
        return False

    # Import and call discord_webhook directly to avoid subprocess
    import discord_webhook as dw
    webhook_url = os.environ.get("DISCORD_WEBHOOK_URL", "")
    if not webhook_url:
        print(
            "[generate_week] DISCORD_WEBHOOK_URL not set — skipping Discord post.\n"
            "  Set the env var and re-run: export DISCORD_WEBHOOK_URL=https://discord.com/api/webhooks/...",
            file=sys.stderr,
        )
        return False

    message = post_file.read_text(encoding="utf-8").strip()
    try:
        result = dw.post_to_discord(message=message, webhook_url=webhook_url)
        print(f"[generate_week] Discord post sent ({result.get('parts_sent', 1)} part(s)).")
        return True
    except Exception as exc:
        print(f"[generate_week] Discord post failed: {exc}", file=sys.stderr)
        return False


# ─── Main ────────────────────────────────────────────────────────────────────

def main() -> None:
    parser = argparse.ArgumentParser(
        description="Master weekly content generation for XOceanus social pipeline.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    parser.add_argument(
        "--week",
        metavar="YYYY-WNN",
        help="Target week (default: current week).",
    )
    parser.add_argument(
        "--depth",
        choices=format_posts.DEPTH_ZONES,
        help="Only generate posts for this depth zone.",
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Extract and format but do not write files or update state.",
    )
    parser.add_argument(
        "--post-discord",
        action="store_true",
        help="After generating, post discord-surface.txt to Discord via webhook.",
    )
    parser.add_argument(
        "--extract-only",
        action="store_true",
        help="Run extraction only; skip post formatting.",
    )
    parser.add_argument(
        "--format-only",
        action="store_true",
        help="Run formatting only (requires existing content JSON for the week).",
    )
    args = parser.parse_args()

    print(f"[generate_week] Starting weekly assembly at {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")

    # ── Step 1: Extract ──────────────────────────────────────────────────────
    if not args.format_only:
        print("[generate_week] Step 1/2: Extracting content…")
        content = extract_content.extract(
            target_week=args.week,
            dry_run=args.dry_run,
        )
        wid = content["week"]
        print(f"[generate_week]   Week:    {wid}")
        print(f"[generate_week]   Commits: {content['changelog']['commit_count']}")
        print(f"[generate_week]   Presets: {content['presets']['new_count']} new")
    else:
        # Load existing content for the week
        wid = args.week or extract_content.week_id()
        content_file = OUTPUT_DIR / f"content-{wid}.json"
        if not content_file.exists():
            print(
                f"[generate_week] ERROR: No content file for {wid}. "
                "Run without --format-only first.",
                file=sys.stderr,
            )
            sys.exit(1)
        with open(content_file) as f:
            content = json.load(f)
        print(f"[generate_week] Step 1/2: Skipped (--format-only). Loaded {content_file.name}")

    if args.extract_only:
        print("[generate_week] Step 2/2: Skipped (--extract-only).")
        print(f"[generate_week] Content saved to: output/content-{content['week']}.json")
        return

    # ── Step 2: Format ───────────────────────────────────────────────────────
    print("[generate_week] Step 2/2: Formatting posts…")
    posts = format_posts.generate_posts(content, target_depth=args.depth)

    if args.dry_run:
        for platform, zones in posts.items():
            for zone, text in zones.items():
                print(f"\n{'='*60}")
                print(f"[DRY RUN] PLATFORM: {platform.upper()} | ZONE: {zone.upper()}")
                print('='*60)
                print(text)
        print("\n[generate_week] Dry run complete — no files written.")
        return

    week_dir = format_posts.write_posts(
        week_id=content["week"],
        posts=posts,
        week_num=content.get("week_counter", 1),
    )

    # ── Summary ──────────────────────────────────────────────────────────────
    print_week_summary(content, week_dir)

    # ── Optional Discord post ─────────────────────────────────────────────────
    if args.post_discord:
        print("[generate_week] Posting to Discord…")
        post_surface_to_discord(week_dir)

    print("[generate_week] Assembly complete.")


if __name__ == "__main__":
    main()
