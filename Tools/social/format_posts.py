#!/usr/bin/env python3
"""
format_posts.py — Format extracted content into platform-specific posts.

Reads a content JSON file (from extract_content.py) and template files,
then generates one post file per platform per depth zone.

Depth zones:
  Surface  — broad appeal, no jargon, max engagement
  Twilight — intermediate producers, some terminology
  Midnight — deep technical, presets + DSP
  Abyss    — lore, scripture, community inner circle

Usage:
    python3 Tools/social/format_posts.py
    python3 Tools/social/format_posts.py --content output/content-2026-W12.json
    python3 Tools/social/format_posts.py --week 2026-W12 --depth surface
"""

from __future__ import annotations

import argparse
import json
import re
import sys
from pathlib import Path
from datetime import datetime

# ─── Paths ───────────────────────────────────────────────────────────────────
PROJECT_ROOT  = Path(__file__).parent.parent.parent
SOCIAL_DIR    = Path(__file__).parent
TEMPLATES_DIR = SOCIAL_DIR / "templates"
OUTPUT_DIR    = SOCIAL_DIR / "output"

# Platform placeholder tokens used in templates
REDDIT_LINK_PLACEHOLDER  = "{reddit_link}"
DISCORD_LINK_PLACEHOLDER = "{discord_link}"
PATREON_LINK_PLACEHOLDER = "{patreon_link}"

# Real links (update when channels are live)
REDDIT_LINK  = "reddit.com/r/XO_OX"
DISCORD_LINK = "discord.gg/xoceanus"
PATREON_LINK = "patreon.com/cw/XO_OX"

DEPTH_ZONES = ["surface", "twilight", "midnight", "abyss"]

PLATFORMS = {
    "twitter":  {"template": "twitter.md",  "depth_zones": ["surface", "twilight"]},
    "reddit":   {"template": "reddit.md",   "depth_zones": ["twilight", "midnight"]},
    "discord":  {"template": "discord.md",  "depth_zones": ["surface", "twilight", "midnight", "abyss"]},
    "patreon":  {"template": "patreon.md",  "depth_zones": ["midnight"]},
    "forum":    {"template": "forum.md",    "depth_zones": ["twilight"]},
}


# ─── Template loading ─────────────────────────────────────────────────────────

def load_template(platform: str) -> str:
    """Load raw template markdown for a platform."""
    tpl_file = TEMPLATES_DIR / PLATFORMS[platform]["template"]
    if not tpl_file.exists():
        return ""
    return tpl_file.read_text(encoding="utf-8")


def extract_zone_block(template_text: str, depth_zone: str) -> str:
    """
    Extract the block for a given depth zone from a template file.
    Looks for:
        ## Surface Zone
        ... content ...
        ## Next Zone
    """
    label_map = {
        "surface":  "Surface Zone",
        "twilight": "Twilight Zone",
        "midnight": "Midnight Zone",
        "abyss":    "Abyss Zone",
    }
    label = label_map.get(depth_zone, depth_zone.title() + " Zone")
    # Match from the zone header to the next ## header or end
    pattern = rf"## {re.escape(label)}\s*\n(.*?)(?=\n## |\Z)"
    m = re.search(pattern, template_text, re.DOTALL | re.IGNORECASE)
    return m.group(1).strip() if m else ""


# ─── Variable substitution ────────────────────────────────────────────────────

def fill_template(template: str, content: dict, week_num: int) -> str:
    """
    Replace template placeholders with content values.
    Handles missing values gracefully (leaves placeholder in output for review).
    """
    recipe    = content.get("recipe") or {}
    presets   = content.get("presets") or {}
    changelog = content.get("changelog") or {}

    substitutions = {
        # Recipe fields
        "{recipe_name}":       recipe.get("name", "[recipe name]"),
        "{engine_a}":          recipe.get("engine_a_short", recipe.get("engine_a", "[Engine A]")),
        "{engine_b}":          recipe.get("engine_b_short", recipe.get("engine_b", "[Engine B]")),
        "{engine_a_full}":     recipe.get("engine_a", "[Engine A full name]"),
        "{engine_b_full}":     recipe.get("engine_b", "[Engine B full name]"),
        "{coupling_type}":     recipe.get("coupling_type", "[coupling type]"),
        "{amount}":            recipe.get("amount", "[amount]"),
        "{direction}":         recipe.get("direction", "A→B"),
        "{genre}":             recipe.get("genre", "[genre]"),
        "{why_it_works}":      recipe.get("why_it_works", "[why it works]"),
        "{why_it_works_short}": recipe.get("why_it_works_short", "[why it works]"),
        # Preset fields
        "{spotlight_preset}":  presets.get("spotlight") or "[preset name]",
        "{new_preset_count}":  str(presets.get("new_count", 0)),
        # Scripture
        "{scripture_verse}":   content.get("scripture_verse", "[verse]"),
        # Changelog
        "{changelog_summary}": changelog.get("summary", "[no changes this week]"),
        "{commit_count}":      str(changelog.get("commit_count", 0)),
        # Meta
        "{week_id}":           content.get("week", "[week]"),
        "{week_num}":          str(week_num),
        "{date}":              datetime.now().strftime("%Y-%m-%d"),
        # Breadcrumbs — first one if available
        "{breadcrumb}":        (content.get("breadcrumbs") or [{}])[0].get("content", "[breadcrumb note]"),
        # Links
        REDDIT_LINK_PLACEHOLDER:  REDDIT_LINK,
        DISCORD_LINK_PLACEHOLDER: DISCORD_LINK,
        PATREON_LINK_PLACEHOLDER: PATREON_LINK,
    }

    result = template
    for placeholder, value in substitutions.items():
        result = result.replace(placeholder, value)
    return result


# ─── Depth tagger ────────────────────────────────────────────────────────────

def tag_content_depth(content: dict) -> str:
    """
    Heuristically decide which depth zone this week's content is best suited for.
    Returns one of: surface, twilight, midnight, abyss
    """
    recipe  = content.get("recipe") or {}
    crumbs  = content.get("breadcrumbs") or []
    commits = content.get("changelog", {}).get("commit_count", 0)

    # If there's a recipe with a complex coupling type → twilight/midnight
    coupling = recipe.get("coupling_type", "")
    complex_couplings = {"KnotTopology", "TriangularCoupling", "Audio->FM", "Chaos->Pitch"}
    if any(c in coupling for c in complex_couplings):
        return "midnight"

    # Heavy commit week → twilight
    if commits >= 10:
        return "twilight"

    # Breadcrumb with a personal note → abyss
    if crumbs:
        return "abyss"

    # Default
    return "surface"


# ─── Post generation ──────────────────────────────────────────────────────────

def generate_posts(content: dict, target_depth: str | None = None) -> dict[str, dict[str, str]]:
    """
    Generate all posts for all platforms and relevant depth zones.
    Returns: {platform: {depth_zone: post_text}}
    """
    results: dict[str, dict[str, str]] = {}
    week_num = content.get("week_counter", 1)
    primary_depth = target_depth or tag_content_depth(content)

    for platform, cfg in PLATFORMS.items():
        template_raw = load_template(platform)
        if not template_raw:
            continue
        results[platform] = {}

        for zone in cfg["depth_zones"]:
            if target_depth and zone != target_depth:
                continue
            block = extract_zone_block(template_raw, zone)
            if not block:
                continue
            filled = fill_template(block, content, week_num)
            results[platform][zone] = filled

    return results


# ─── Output writer ───────────────────────────────────────────────────────────

def write_posts(week_id: str, posts: dict, week_num: int) -> Path:
    """Write each post to output/week-{N}/{platform}-{zone}.txt"""
    week_dir = OUTPUT_DIR / f"week-{week_num:03d}"
    week_dir.mkdir(parents=True, exist_ok=True)

    written = []
    for platform, zones in posts.items():
        for zone, text in zones.items():
            filename = f"{platform}-{zone}.txt"
            out_path = week_dir / filename
            out_path.write_text(text, encoding="utf-8")
            written.append(str(out_path.relative_to(PROJECT_ROOT)))

    # Write an index file
    index_path = week_dir / "index.json"
    index = {
        "week_id": week_id,
        "week_num": week_num,
        "generated_at": datetime.now().isoformat(),
        "files": written,
    }
    with open(index_path, "w", encoding="utf-8") as f:
        json.dump(index, f, indent=2)

    return week_dir


# ─── CLI ─────────────────────────────────────────────────────────────────────

def resolve_content_file(week_arg: str | None, content_arg: str | None) -> Path | None:
    if content_arg:
        p = Path(content_arg)
        return p if p.exists() else SOCIAL_DIR / content_arg
    if week_arg:
        return OUTPUT_DIR / f"content-{week_arg}.json"
    # Latest
    candidates = sorted(OUTPUT_DIR.glob("content-*.json"))
    return candidates[-1] if candidates else None


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Format extracted content into platform posts.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    parser.add_argument(
        "--content", metavar="FILE",
        help="Path to content JSON file (default: latest in output/)",
    )
    parser.add_argument(
        "--week", metavar="YYYY-WNN",
        help="Week ID to load (e.g. 2026-W12)",
    )
    parser.add_argument(
        "--depth",
        choices=DEPTH_ZONES,
        help="Only generate posts for this depth zone.",
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Print posts to stdout instead of writing files.",
    )
    args = parser.parse_args()

    content_file = resolve_content_file(args.week, args.content)
    if not content_file or not content_file.exists():
        print(
            "[format_posts] ERROR: No content JSON found. "
            "Run extract_content.py first.",
            file=sys.stderr,
        )
        sys.exit(1)

    with open(content_file, "r", encoding="utf-8") as f:
        content = json.load(f)

    posts = generate_posts(content, target_depth=args.depth)

    if args.dry_run:
        for platform, zones in posts.items():
            for zone, text in zones.items():
                print(f"\n{'='*60}")
                print(f"PLATFORM: {platform.upper()} | ZONE: {zone.upper()}")
                print('='*60)
                print(text)
        return

    week_id_str = content.get("week", "unknown")
    week_num    = content.get("week_counter", 1)
    week_dir    = write_posts(week_id_str, posts, week_num)

    total = sum(len(z) for z in posts.values())
    print(f"[format_posts] Wrote {total} posts to {week_dir}")


if __name__ == "__main__":
    main()
