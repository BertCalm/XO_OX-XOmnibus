#!/usr/bin/env python3
"""
daily_signal.py — Daily scripture verse auto-poster for XOlokun.

Reads all .md files in scripture/retreats/ and scripture/the-scripture.md,
extracts verse candidates (20–280 chars), rotates through them without repeats,
and outputs one formatted verse to stdout.

Usage:
    python3 Tools/social/daily_signal.py
    python3 Tools/social/daily_signal.py --format discord
    python3 Tools/social/daily_signal.py --format twitter
    python3 Tools/social/daily_signal.py --format plain
    python3 Tools/social/daily_signal.py --preview 5    # show 5 upcoming verses

Cron setup (8am daily):
    0 8 * * * cd /path/to/repo && python3 Tools/social/daily_signal.py >> Tools/social/output/daily-signals.log

Pipe to Discord:
    python3 Tools/social/daily_signal.py | python3 Tools/social/discord_webhook.py --stdin
"""

from __future__ import annotations

import argparse
import json
import random
import re
import sys
from datetime import datetime
from pathlib import Path

# ─── Paths ───────────────────────────────────────────────────────────────────
PROJECT_ROOT   = Path(__file__).parent.parent.parent
SOCIAL_DIR     = Path(__file__).parent
OUTPUT_DIR     = SOCIAL_DIR / "output"
STATE_FILE     = OUTPUT_DIR / ".daily_signal_state.json"
SCRIPTURE_FILE = PROJECT_ROOT / "scripture" / "the-scripture.md"
RETREATS_DIR   = PROJECT_ROOT / "scripture" / "retreats"

# Minimum/maximum verse character lengths
VERSE_MIN = 20
VERSE_MAX = 280

# ─── Format templates ─────────────────────────────────────────────────────────
FORMAT_TEMPLATES = {
    "plain": "{verse}",
    "twitter": '"{verse}"\n— Guru Bin, The Book of Bin\n#XOlokun #synthesis #SoundDesign',
    "discord": (
        "**Daily Signal** 🐚\n"
        "━━━━━━━━━━━━━━━━━━━━━\n"
        "*\"{verse}\"*\n"
        "━━━━━━━━━━━━━━━━━━━━━\n"
        "— Guru Bin | The Book of Bin"
    ),
    "patreon": (
        "**Today's Signal**\n\n"
        "> {verse}\n\n"
        "*— Guru Bin, The Book of Bin*"
    ),
}


# ─── State helpers ───────────────────────────────────────────────────────────

def load_state() -> dict:
    if STATE_FILE.exists():
        try:
            with open(STATE_FILE, "r", encoding="utf-8") as f:
                return json.load(f)
        except (json.JSONDecodeError, OSError):
            pass
    return {"used_hashes": [], "total_sent": 0, "last_sent": None}


def save_state(state: dict) -> None:
    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)
    with open(STATE_FILE, "w", encoding="utf-8") as f:
        json.dump(state, f, indent=2)


# ─── Verse collection ────────────────────────────────────────────────────────

def _verse_hash(verse: str) -> str:
    """Stable short hash for deduplication."""
    return verse[:50].lower().replace(" ", "_").replace(".", "")


def _extract_verses_from_text(text: str) -> list[str]:
    """
    Extract verse candidates from markdown text.
    Accepts:
      - blockquote lines (> ...)
      - plain prose lines between VERSE_MIN and VERSE_MAX chars
    Rejects:
      - headings (#)
      - table rows (|)
      - code fences (```)
      - bullet lists (-/*/+)
      - lines that are purely punctuation or dashes
    """
    verses = []
    in_code_block = False
    for raw_line in text.splitlines():
        line = raw_line.strip()

        # Track code fences
        if line.startswith("```"):
            in_code_block = not in_code_block
            continue
        if in_code_block:
            continue

        # Unwrap blockquote
        if line.startswith(">"):
            line = re.sub(r"^>+\s*", "", line).strip()

        # Skip structural markdown
        if not line:
            continue
        if line.startswith("#"):
            continue
        if line.startswith("|"):
            continue
        if re.match(r"^[-*+]\s", line):
            continue
        if re.match(r"^[*_]{1,3}[^*_]", line) and line.endswith("*"):
            # Italic/bold single-word or label line — skip
            continue
        # Skip separator lines
        if re.match(r"^[-=_]{3,}$", line):
            continue
        # Skip lines that are just metadata or short headers
        if re.match(r"^\*\*[^*]+\*\*\s*$", line):
            continue

        length = len(line)
        if VERSE_MIN <= length <= VERSE_MAX:
            verses.append(line)

    return verses


def collect_all_verses() -> list[str]:
    """Gather all verse candidates from scripture + retreats."""
    verses = []

    if SCRIPTURE_FILE.exists():
        verses.extend(_extract_verses_from_text(SCRIPTURE_FILE.read_text(encoding="utf-8")))

    if RETREATS_DIR.exists():
        for md_file in sorted(RETREATS_DIR.glob("*.md")):
            try:
                verses.extend(_extract_verses_from_text(md_file.read_text(encoding="utf-8")))
            except OSError:
                continue

    # Deduplicate preserving order
    seen: set[str] = set()
    unique: list[str] = []
    for v in verses:
        h = _verse_hash(v)
        if h not in seen:
            seen.add(h)
            unique.append(v)

    return unique


# ─── Verse picker ─────────────────────────────────────────────────────────────

def pick_verse(state: dict) -> tuple[str, str]:
    """
    Select an unused verse (random within unused pool).
    Returns (verse_text, verse_hash).
    Resets used pool when all verses have been sent.
    """
    verses = collect_all_verses()

    if not verses:
        fallback = "The deep is quiet. Listen for what the silence teaches."
        return fallback, _verse_hash(fallback)

    used = set(state.get("used_hashes", []))
    available = [v for v in verses if _verse_hash(v) not in used]

    if not available:
        # Full rotation complete — start over
        available = verses
        state["used_hashes"] = []

    chosen = random.choice(available)
    return chosen, _verse_hash(chosen)


# ─── Formatting ──────────────────────────────────────────────────────────────

def format_verse(verse: str, fmt: str) -> str:
    """Apply format template to verse text."""
    template = FORMAT_TEMPLATES.get(fmt, FORMAT_TEMPLATES["plain"])
    return template.replace("{verse}", verse)


# ─── Preview ─────────────────────────────────────────────────────────────────

def preview_upcoming(n: int, fmt: str) -> None:
    """Print N upcoming verses without updating state."""
    state = load_state()
    verses = collect_all_verses()
    used   = set(state.get("used_hashes", []))
    available = [v for v in verses if _verse_hash(v) not in used]

    if not available:
        available = verses  # Would reset

    sample = random.sample(available, min(n, len(available)))
    for i, verse in enumerate(sample, 1):
        print(f"\n── Preview {i}/{n} ──")
        print(format_verse(verse, fmt))
    print(f"\n[{len(available)} unused verses remaining of {len(verses)} total]")


# ─── Log helper ──────────────────────────────────────────────────────────────

def log_to_file(verse: str, fmt: str) -> None:
    """Append the sent verse to the daily-signals.log."""
    log_path = OUTPUT_DIR / "daily-signals.log"
    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)
    ts = datetime.now().isoformat()
    entry = f"\n[{ts}] format={fmt}\n{verse}\n{'─'*40}"
    with open(log_path, "a", encoding="utf-8") as f:
        f.write(entry + "\n")


# ─── CLI ─────────────────────────────────────────────────────────────────────

def main() -> None:
    parser = argparse.ArgumentParser(
        description="Output a daily scripture verse for XOlokun social channels.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    parser.add_argument(
        "--format", "-f",
        choices=list(FORMAT_TEMPLATES.keys()),
        default="discord",
        help="Output format (default: discord)",
    )
    parser.add_argument(
        "--preview",
        metavar="N",
        type=int,
        help="Preview N upcoming verses without updating state.",
    )
    parser.add_argument(
        "--no-log",
        action="store_true",
        help="Skip writing to daily-signals.log",
    )
    parser.add_argument(
        "--stats",
        action="store_true",
        help="Print verse pool statistics and exit.",
    )
    args = parser.parse_args()

    if args.stats:
        state  = load_state()
        verses = collect_all_verses()
        used   = len(state.get("used_hashes", []))
        print(f"Total verses: {len(verses)}")
        print(f"Used so far:  {used}")
        print(f"Remaining:    {max(0, len(verses) - used)}")
        print(f"Total sent:   {state.get('total_sent', 0)}")
        print(f"Last sent:    {state.get('last_sent', 'never')}")
        return

    if args.preview:
        preview_upcoming(args.preview, args.format)
        return

    state = load_state()
    verse, vhash = pick_verse(state)
    formatted = format_verse(verse, args.format)

    # Output to stdout
    print(formatted)

    # Update state
    used_hashes = list(set(state.get("used_hashes", [])) | {vhash})
    state["used_hashes"]  = used_hashes
    state["total_sent"]   = state.get("total_sent", 0) + 1
    state["last_sent"]    = datetime.now().isoformat()
    save_state(state)

    # Log
    if not args.no_log:
        log_to_file(formatted, args.format)


if __name__ == "__main__":
    main()
