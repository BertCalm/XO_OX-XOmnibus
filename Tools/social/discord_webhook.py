#!/usr/bin/env python3
"""
discord_webhook.py — Post a message to Discord via webhook URL.

Uses only Python stdlib (urllib.request) — no pip installs required.

Usage:
    python3 Tools/social/discord_webhook.py "Your message here"
    python3 Tools/social/discord_webhook.py --stdin
    python3 Tools/social/daily_signal.py | python3 Tools/social/discord_webhook.py --stdin
    python3 Tools/social/discord_webhook.py --file output/week-001/discord-surface.txt

Environment variables:
    DISCORD_WEBHOOK_URL  — required (set in shell or .env sourced before running)

Optional env vars:
    DISCORD_USERNAME     — override the bot display name (default: XOceanus)
    DISCORD_AVATAR_URL   — override the bot avatar URL

Exit codes:
    0 — success
    1 — missing webhook URL or other configuration error
    2 — HTTP error from Discord API
"""

from __future__ import annotations

import argparse
import json
import os
import sys
import urllib.error
import urllib.request
from datetime import datetime
from pathlib import Path

# ─── Paths ───────────────────────────────────────────────────────────────────
PROJECT_ROOT = Path(__file__).parent.parent.parent
SOCIAL_DIR   = Path(__file__).parent
OUTPUT_DIR   = SOCIAL_DIR / "output"
LOG_FILE     = OUTPUT_DIR / "discord-webhook.log"

# Default bot identity
DEFAULT_USERNAME   = "XOceanus"
DEFAULT_AVATAR_URL = ""  # empty = Discord default

# Discord webhook API endpoint is provided via env var
MAX_MESSAGE_LENGTH = 2000  # Discord hard limit


# ─── Webhook poster ──────────────────────────────────────────────────────────

def post_to_discord(
    message: str,
    webhook_url: str,
    username: str = DEFAULT_USERNAME,
    avatar_url: str = DEFAULT_AVATAR_URL,
) -> dict:
    """
    Post a message to Discord via webhook.
    Returns the response dict (empty on 204, or error dict on failure).
    Raises urllib.error.HTTPError on non-2xx responses.
    """
    if len(message) > MAX_MESSAGE_LENGTH:
        # Split at last newline before limit
        split_at = message.rfind("\n", 0, MAX_MESSAGE_LENGTH - 10)
        if split_at == -1:
            split_at = MAX_MESSAGE_LENGTH - 10
        parts = [message[:split_at], message[split_at:].lstrip()]
    else:
        parts = [message]

    responses = []
    for i, part in enumerate(parts):
        payload: dict = {"content": part}
        if username:
            payload["username"] = username
        if avatar_url:
            payload["avatar_url"] = avatar_url

        data = json.dumps(payload).encode("utf-8")
        req = urllib.request.Request(
            url=webhook_url,
            data=data,
            headers={
                "Content-Type":  "application/json",
                "User-Agent":    "XOceanus-SocialBot/1.0",
            },
            method="POST",
        )

        try:
            with urllib.request.urlopen(req, timeout=15) as resp:
                status = resp.status
                body   = resp.read().decode("utf-8") if resp.length else ""
                try:
                    parsed = json.loads(body) if body else {}
                except json.JSONDecodeError:
                    parsed = {"raw": body}
                responses.append({"status": status, "part": i + 1, "response": parsed})
        except urllib.error.HTTPError as exc:
            body = exc.read().decode("utf-8", errors="replace")
            raise urllib.error.HTTPError(
                exc.url, exc.code,
                f"Discord API error {exc.code}: {body}",
                exc.headers, None,
            ) from exc

    return {"parts_sent": len(responses), "responses": responses}


# ─── Logging ─────────────────────────────────────────────────────────────────

def log_post(message: str, result: dict, dry_run: bool = False) -> None:
    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)
    ts = datetime.now().isoformat()
    prefix = "[DRY RUN] " if dry_run else ""
    entry = (
        f"\n[{ts}] {prefix}Posted to Discord\n"
        f"Message ({len(message)} chars): {message[:80]}{'…' if len(message) > 80 else ''}\n"
        f"Result: {json.dumps(result)}\n"
        f"{'─' * 40}"
    )
    with open(LOG_FILE, "a", encoding="utf-8") as f:
        f.write(entry + "\n")


# ─── CLI ─────────────────────────────────────────────────────────────────────

def main() -> None:
    parser = argparse.ArgumentParser(
        description="Post a message to Discord via webhook.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )

    source = parser.add_mutually_exclusive_group()
    source.add_argument(
        "message",
        nargs="?",
        help="Message text to send.",
    )
    source.add_argument(
        "--stdin",
        action="store_true",
        help="Read message from stdin.",
    )
    source.add_argument(
        "--file",
        metavar="PATH",
        help="Read message from a text file.",
    )

    parser.add_argument(
        "--webhook-url",
        metavar="URL",
        help="Discord webhook URL (overrides DISCORD_WEBHOOK_URL env var).",
    )
    parser.add_argument(
        "--username",
        default=os.environ.get("DISCORD_USERNAME", DEFAULT_USERNAME),
        help=f"Override bot display name (default: {DEFAULT_USERNAME}).",
    )
    parser.add_argument(
        "--avatar-url",
        default=os.environ.get("DISCORD_AVATAR_URL", DEFAULT_AVATAR_URL),
        help="Override bot avatar URL.",
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Print what would be sent without actually posting.",
    )
    parser.add_argument(
        "--no-log",
        action="store_true",
        help="Skip writing to discord-webhook.log.",
    )

    args = parser.parse_args()

    # ── Resolve message ──────────────────────────────────────────────────────
    if args.stdin:
        message = sys.stdin.read().strip()
    elif args.file:
        file_path = Path(args.file)
        if not file_path.exists():
            file_path = SOCIAL_DIR / args.file
        if not file_path.exists():
            print(f"[discord_webhook] ERROR: File not found: {args.file}", file=sys.stderr)
            sys.exit(1)
        message = file_path.read_text(encoding="utf-8").strip()
    elif args.message:
        message = args.message
    else:
        parser.print_help()
        sys.exit(1)

    if not message:
        print("[discord_webhook] ERROR: Empty message.", file=sys.stderr)
        sys.exit(1)

    # ── Resolve webhook URL ──────────────────────────────────────────────────
    webhook_url = args.webhook_url or os.environ.get("DISCORD_WEBHOOK_URL", "")
    if not webhook_url:
        print(
            "[discord_webhook] ERROR: No webhook URL provided.\n"
            "Set DISCORD_WEBHOOK_URL environment variable or pass --webhook-url.",
            file=sys.stderr,
        )
        sys.exit(1)

    # ── Dry run ──────────────────────────────────────────────────────────────
    if args.dry_run:
        print("[DRY RUN] Would post to Discord:")
        print(f"  Username: {args.username}")
        print(f"  Webhook:  {webhook_url[:40]}…")
        print(f"  Message ({len(message)} chars):")
        print("  " + "\n  ".join(message.splitlines()))
        if not args.no_log:
            log_post(message, {"dry_run": True}, dry_run=True)
        return

    # ── Post ─────────────────────────────────────────────────────────────────
    try:
        result = post_to_discord(
            message=message,
            webhook_url=webhook_url,
            username=args.username,
            avatar_url=args.avatar_url,
        )
        parts = result.get("parts_sent", 1)
        print(f"[discord_webhook] Posted {parts} message part(s) successfully.", file=sys.stderr)
        if not args.no_log:
            log_post(message, result)
    except urllib.error.HTTPError as exc:
        print(f"[discord_webhook] ERROR: {exc}", file=sys.stderr)
        if not args.no_log:
            log_post(message, {"error": str(exc)})
        sys.exit(2)
    except Exception as exc:
        print(f"[discord_webhook] UNEXPECTED ERROR: {exc}", file=sys.stderr)
        sys.exit(2)


if __name__ == "__main__":
    main()
