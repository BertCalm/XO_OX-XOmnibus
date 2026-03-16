"""
xpn_pack_readme_updater.py — XO_OX Pack README Auto-Section Updater

Reads an existing pack README.md and regenerates only the machine-managed
section wrapped with:
    <!-- XO_OX:AUTO:BEGIN -->
    ...
    <!-- XO_OX:AUTO:END -->

Human-written content outside those markers is never modified.

Usage:
    python xpn_pack_readme_updater.py --readme ./README.md [options]

Options:
    --readme PATH           Path to the README.md file (required)
    --manifest PATH         Path to bundle_manifest.json
    --version VERSION       Override version string (e.g. 1.1.0)
    --preset-count N        Override preset count
    --engine ENGINE         Override engine name (e.g. OBESE)
    --force                 If auto markers are absent, append section at end
    --dry-run               Print result to stdout without writing file

Manifest JSON schema (all fields optional if CLI overrides provided):
    {
        "pack_name": "Aquatic Depths Vol. 1",
        "engine": "OBESE",
        "mood": "Foundation",
        "version": "1.0.0",
        "sonic_dna": {
            "brightness": 0.8,
            "warmth": 0.6,
            "density": 0.4
        },
        "preset_count": 32
    }
"""

import argparse
import json
import sys
from datetime import datetime, timezone
from pathlib import Path

MARKER_BEGIN = "<!-- XO_OX:AUTO:BEGIN -->"
MARKER_END = "<!-- XO_OX:AUTO:END -->"

COMPATIBLE_MPC_MODELS = [
    "MPC One",
    "MPC Live II",
    "MPC Live III",
    "MPC X",
    "Force",
    "MPC Studio",
]

BAR_WIDTH = 10


def ascii_bar(value: float) -> str:
    """Render a float 0.0–1.0 as a 10-char filled/empty bar."""
    filled = round(value * BAR_WIDTH)
    filled = max(0, min(BAR_WIDTH, filled))
    return "█" * filled + "░" * (BAR_WIDTH - filled)


def build_auto_section(data: dict) -> str:
    """Build the machine-managed section string from merged data dict."""
    pack_name = data.get("pack_name", "Unnamed Pack")
    engine = data.get("engine", "UNKNOWN")
    version = data.get("version", "0.0.0")
    mood = data.get("mood", "")
    preset_count = data.get("preset_count", 0)
    sonic_dna = data.get("sonic_dna", {})
    timestamp = datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")

    lines = []
    lines.append(MARKER_BEGIN)
    lines.append(f"<!-- Generated: {timestamp} -->")
    lines.append("")

    # Header badge line
    mood_str = f" · {mood}" if mood else ""
    lines.append(f"## {pack_name}  `v{version}`  `{engine}`{mood_str}")
    lines.append("")

    # Preset count
    lines.append(f"**Presets:** {preset_count}")
    lines.append("")

    # Sonic DNA
    if sonic_dna:
        lines.append("**Sonic DNA**")
        lines.append("```")
        for trait, value in sonic_dna.items():
            try:
                fval = float(value)
            except (TypeError, ValueError):
                fval = 0.0
            bar = ascii_bar(fval)
            lines.append(f"{trait:<12}{bar}  {fval:.1f}")
        lines.append("```")
        lines.append("")

    # Compatible models
    lines.append("**Compatible MPC Models**")
    for model in COMPATIBLE_MPC_MODELS:
        lines.append(f"- {model}")
    lines.append("")

    lines.append(MARKER_END)

    return "\n".join(lines)


def load_manifest(path: str) -> dict:
    """Load and return manifest JSON, raising on parse errors."""
    p = Path(path)
    if not p.exists():
        print(f"ERROR: Manifest not found: {path}", file=sys.stderr)
        sys.exit(1)
    with open(p, "r", encoding="utf-8") as f:
        try:
            return json.load(f)
        except json.JSONDecodeError as e:
            print(f"ERROR: Failed to parse manifest JSON: {e}", file=sys.stderr)
            sys.exit(1)


def merge_data(manifest_data: dict, args: argparse.Namespace) -> dict:
    """Merge manifest fields with CLI overrides (CLI wins)."""
    data = dict(manifest_data)
    if args.version:
        data["version"] = args.version
    if args.preset_count is not None:
        data["preset_count"] = args.preset_count
    if args.engine:
        data["engine"] = args.engine
    return data


def update_readme(content: str, new_section: str, force: bool) -> tuple[str, bool]:
    """
    Replace the auto section in content with new_section.
    Returns (updated_content, appended_flag).
    appended_flag is True when markers were absent and --force caused an append.
    Raises ValueError if markers are absent and force is False.
    """
    begin_idx = content.find(MARKER_BEGIN)
    end_idx = content.find(MARKER_END)

    if begin_idx == -1 or end_idx == -1:
        if not force:
            raise ValueError(
                "Auto-section markers not found in README. "
                "Use --force to append the section at the end."
            )
        # Append mode: add a blank line separator then the section
        separator = "" if content.endswith("\n") else "\n"
        return content + separator + "\n" + new_section + "\n", True

    # Replace everything from BEGIN marker up to and including END marker
    end_of_section = end_idx + len(MARKER_END)
    updated = content[:begin_idx] + new_section + content[end_of_section:]
    return updated, False


def main():
    parser = argparse.ArgumentParser(
        description="Update machine-managed section of an XO_OX pack README.md"
    )
    parser.add_argument("--readme", required=True, help="Path to README.md")
    parser.add_argument("--manifest", help="Path to bundle_manifest.json")
    parser.add_argument("--version", help="Override version string")
    parser.add_argument("--preset-count", type=int, dest="preset_count", help="Override preset count")
    parser.add_argument("--engine", help="Override engine name")
    parser.add_argument(
        "--force",
        action="store_true",
        help="Append auto section if markers are absent",
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        dest="dry_run",
        help="Print result to stdout without writing",
    )
    args = parser.parse_args()

    # Load README
    readme_path = Path(args.readme)
    if not readme_path.exists():
        print(f"ERROR: README not found: {args.readme}", file=sys.stderr)
        sys.exit(1)
    content = readme_path.read_text(encoding="utf-8")

    # Load manifest if provided
    manifest_data = {}
    if args.manifest:
        manifest_data = load_manifest(args.manifest)

    # Merge data
    data = merge_data(manifest_data, args)

    # Build new auto section
    new_section = build_auto_section(data)

    # Update README content
    try:
        updated_content, was_appended = update_readme(content, new_section, args.force)
    except ValueError as e:
        print(f"WARNING: {e}", file=sys.stderr)
        sys.exit(1)

    if was_appended:
        print("NOTE: Markers were absent — auto section appended at end of file.", file=sys.stderr)

    if args.dry_run:
        print(updated_content, end="")
    else:
        readme_path.write_text(updated_content, encoding="utf-8")
        print(f"Updated: {readme_path}")


if __name__ == "__main__":
    main()
