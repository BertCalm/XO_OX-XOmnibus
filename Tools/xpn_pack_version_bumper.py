#!/usr/bin/env python3
"""
XPN Pack Version Bumper — XO_OX Designs
Bumps the semantic version in an .xpn expansion pack and optionally generates
a CHANGELOG entry.

Updates:
  - expansion.json        (field: "version")
  - bundle_manifest.json  (field: "version", if present)
  - All XPM files         (XML attr: Program Version="...")
  - CHANGELOG.md          (prepends entry, if --message provided)

Usage:
    python xpn_pack_version_bumper.py <pack.xpn> --bump patch|minor|major
    python xpn_pack_version_bumper.py <pack.xpn> --bump minor --message "Added 8 programs"
    python xpn_pack_version_bumper.py <pack.xpn> --bump patch --dry-run
    python xpn_pack_version_bumper.py <pack.xpn> --bump patch --output MyPack_v1.0.1.xpn
    python xpn_pack_version_bumper.py <pack.xpn> --bump patch --format json
"""

import argparse
import json
import os
import re
import shutil
import sys
import zipfile
from datetime import date
from typing import Optional
from xml.etree import ElementTree as ET


# ---------------------------------------------------------------------------
# Version helpers
# ---------------------------------------------------------------------------

def parse_version(v: str) -> tuple[int, int, int]:
    """Parse 'MAJOR.MINOR.PATCH' into a tuple of ints."""
    parts = v.strip().split(".")
    if len(parts) != 3:
        raise ValueError(f"Invalid version string: {v!r}  (expected MAJOR.MINOR.PATCH)")
    try:
        return (int(parts[0]), int(parts[1]), int(parts[2]))
    except ValueError:
        raise ValueError(f"Non-integer version component in: {v!r}")


def bump_version(v: str, bump: str) -> str:
    """Return bumped version string."""
    major, minor, patch = parse_version(v)
    if bump == "patch":
        patch += 1
    elif bump == "minor":
        minor += 1
        patch = 0
    elif bump == "major":
        major += 1
        minor = 0
        patch = 0
    else:
        raise ValueError(f"Unknown bump type: {bump!r}  (use patch/minor/major)")
    return f"{major}.{minor}.{patch}"


# ---------------------------------------------------------------------------
# ZIP / file helpers
# ---------------------------------------------------------------------------

def _is_xpm(name: str) -> bool:
    return name.lower().endswith(".xpm") and not name.startswith("__")


def _read_json(zf: zipfile.ZipFile, path: str) -> dict:
    with zf.open(path) as f:
        return json.load(f)


def _find_file(zf: zipfile.ZipFile, filename: str) -> Optional[str]:
    """Return the zip path for a given filename, or None if absent."""
    for name in zf.namelist():
        if name.split("/")[-1] == filename:
            return name
    return None


def _list_xpms(zf: zipfile.ZipFile) -> list[str]:
    return [n for n in zf.namelist() if _is_xpm(n)]


# ---------------------------------------------------------------------------
# Core bump logic
# ---------------------------------------------------------------------------

def collect_changes(
    zf: zipfile.ZipFile,
    old_ver: str,
    new_ver: str,
) -> dict:
    """
    Inspect the archive and return a structured description of every change
    that would be made.  Nothing is written here.

    Returns:
        {
            "old": str,
            "new": str,
            "pack_name": str,
            "expansion_path": str,
            "bundle_path": Optional[str],
            "xpm_changes": [{"path": str, "program_count": int}],
            "changelog_present": bool,
            "changelog_path": str | None,
        }
    """
    expansion_path = _find_file(zf, "expansion.json")
    if expansion_path is None:
        raise FileNotFoundError("expansion.json not found in archive.")

    exp_data = _read_json(zf, expansion_path)
    pack_name = exp_data.get("name", os.path.basename(zf.filename))

    bundle_path = _find_file(zf, "bundle_manifest.json")
    changelog_path = _find_file(zf, "CHANGELOG.md")

    xpm_changes = []
    for xpm_path in _list_xpms(zf):
        raw = zf.read(xpm_path)
        # Count occurrences of Version attr matching the old version
        count = len(re.findall(
            r'Version\s*=\s*["\']?' + re.escape(old_ver) + r'["\']?',
            raw.decode("utf-8", errors="replace"),
        ))
        xpm_changes.append({"path": xpm_path, "program_count": count})

    return {
        "old": old_ver,
        "new": new_ver,
        "pack_name": pack_name,
        "expansion_path": expansion_path,
        "bundle_path": bundle_path,
        "xpm_changes": xpm_changes,
        "changelog_present": changelog_path is not None,
        "changelog_path": changelog_path,
    }


def format_dry_run_text(changes: dict, bump: str, message: Optional[str]) -> str:
    lines = [
        f"VERSION BUMP — {changes['pack_name']}",
        f"  Current: {changes['old']}",
        f"  New:     {changes['new']} ({bump})",
        "",
        "Files to update:",
        f"  {changes['expansion_path']}: version \"{changes['old']}\" → \"{changes['new']}\"",
    ]

    if changes["bundle_path"]:
        lines.append(
            f"  {changes['bundle_path']}: version \"{changes['old']}\" → \"{changes['new']}\""
        )

    for xc in changes["xpm_changes"]:
        n = xc["program_count"]
        label = f"{n} program{'s' if n != 1 else ''}"
        lines.append(
            f"  {xc['path']}: Version attr \"{changes['old']}\" → \"{changes['new']}\" ({label})"
        )

    if message:
        today = date.today().isoformat()
        lines += [
            "",
            "CHANGELOG entry to prepend:",
            f"## [{changes['new']}] — {today}",
            f"- {message}",
        ]
    elif changes["changelog_present"]:
        lines.append("")
        lines.append(
            f"  (CHANGELOG.md present but no --message given — file will not be modified)"
        )

    return "\n".join(lines)


def format_dry_run_json(changes: dict, bump: str, message: Optional[str]) -> str:
    today = date.today().isoformat()
    payload = {
        "pack_name": changes["pack_name"],
        "bump": bump,
        "old_version": changes["old"],
        "new_version": changes["new"],
        "files": {
            "expansion_json": changes["expansion_path"],
            "bundle_manifest": changes["bundle_path"],
            "xpm_files": [xc["path"] for xc in changes["xpm_changes"]],
        },
        "changelog_entry": None,
    }
    if message:
        payload["changelog_entry"] = {
            "date": today,
            "version": changes["new"],
            "message": message,
        }
    return json.dumps(payload, indent=2)


# ---------------------------------------------------------------------------
# Write helpers
# ---------------------------------------------------------------------------

def _bump_json_version(raw: bytes, old_ver: str, new_ver: str) -> bytes:
    """Replace "version": "OLD" with "version": "NEW" in JSON bytes."""
    text = raw.decode("utf-8")
    pattern = r'("version"\s*:\s*")' + re.escape(old_ver) + r'"'
    replacement = r'\g<1>' + new_ver + '"'
    updated = re.sub(pattern, replacement, text, count=1)
    return updated.encode("utf-8")


def _bump_xpm_version(raw: bytes, old_ver: str, new_ver: str) -> bytes:
    """Replace Version="OLD" in XPM XML bytes (handles both quoted styles)."""
    text = raw.decode("utf-8")
    pattern = r'(Version\s*=\s*["\']?)' + re.escape(old_ver) + r'(["\']?)'
    updated = re.sub(pattern, r'\g<1>' + new_ver + r'\2', text)
    return updated.encode("utf-8")


def _build_changelog_entry(new_ver: str, message: str) -> str:
    today = date.today().isoformat()
    return f"## [{new_ver}] — {today}\n- {message}\n\n"


def write_bumped_xpn(
    src_path: str,
    dst_path: str,
    changes: dict,
    message: Optional[str],
) -> None:
    """Write a new .xpn with all version fields updated."""
    old_ver = changes["old"]
    new_ver = changes["new"]

    with zipfile.ZipFile(src_path, "r") as src_zf, \
         zipfile.ZipFile(dst_path, "w", compression=zipfile.ZIP_DEFLATED) as dst_zf:

        for item in src_zf.infolist():
            raw = src_zf.read(item.filename)
            base = item.filename.split("/")[-1]

            if item.filename == changes["expansion_path"]:
                raw = _bump_json_version(raw, old_ver, new_ver)

            elif changes["bundle_path"] and item.filename == changes["bundle_path"]:
                raw = _bump_json_version(raw, old_ver, new_ver)

            elif _is_xpm(item.filename):
                raw = _bump_xpm_version(raw, old_ver, new_ver)

            elif base == "CHANGELOG.md" and message:
                entry = _build_changelog_entry(new_ver, message).encode("utf-8")
                raw = entry + raw

            dst_zf.writestr(item, raw)

        # If CHANGELOG.md was absent and a message was given, create it
        if message and not changes["changelog_present"]:
            entry = _build_changelog_entry(new_ver, message).encode("utf-8")
            dst_zf.writestr("CHANGELOG.md", entry)


# ---------------------------------------------------------------------------
# CLI entry point
# ---------------------------------------------------------------------------

def main() -> None:
    parser = argparse.ArgumentParser(
        description="Bump the version in an .xpn expansion pack.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument("pack", help="Path to input .xpn file")
    parser.add_argument(
        "--bump",
        required=True,
        choices=["patch", "minor", "major"],
        help="Which version component to increment",
    )
    parser.add_argument(
        "--message",
        default=None,
        metavar="MSG",
        help="Changelog note — if given, generates/prepends a CHANGELOG.md entry",
    )
    parser.add_argument(
        "--output",
        default=None,
        metavar="OUTPUT.xpn",
        help="Output path (default: overwrites input after creating a .bak backup)",
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Show what would change without writing any files",
    )
    parser.add_argument(
        "--format",
        choices=["text", "json"],
        default="text",
        help="Output format for --dry-run (default: text)",
    )
    args = parser.parse_args()

    if not os.path.isfile(args.pack):
        print(f"ERROR: File not found: {args.pack}", file=sys.stderr)
        sys.exit(1)

    # --- Read current version from expansion.json ---
    try:
        with zipfile.ZipFile(args.pack, "r") as zf:
            expansion_path = _find_file(zf, "expansion.json")
            if expansion_path is None:
                print("ERROR: expansion.json not found in archive.", file=sys.stderr)
                sys.exit(1)
            exp_data = _read_json(zf, expansion_path)
    except zipfile.BadZipFile:
        print(f"ERROR: Not a valid ZIP/XPN file: {args.pack}", file=sys.stderr)
        sys.exit(1)

    current_ver = exp_data.get("version")
    if not current_ver:
        print("ERROR: 'version' field missing from expansion.json.", file=sys.stderr)
        sys.exit(1)

    try:
        new_ver = bump_version(current_ver, args.bump)
    except ValueError as e:
        print(f"ERROR: {e}", file=sys.stderr)
        sys.exit(1)

    # --- Collect change plan ---
    with zipfile.ZipFile(args.pack, "r") as zf:
        changes = collect_changes(zf, current_ver, new_ver)

    # --- Dry run ---
    if args.dry_run:
        if args.format == "json":
            print(format_dry_run_json(changes, args.bump, args.message))
        else:
            print(format_dry_run_text(changes, args.bump, args.message))
        return

    # --- Determine output path ---
    if args.output:
        out_path = args.output
        backup_path = None
    else:
        out_path = args.pack
        backup_path = args.pack + ".bak"
        shutil.copy2(args.pack, backup_path)

    # Write to a temp file first, then rename (atomic-ish)
    tmp_path = out_path + ".tmp"
    try:
        write_bumped_xpn(args.pack, tmp_path, changes, args.message)
        os.replace(tmp_path, out_path)
    except Exception as e:
        if os.path.exists(tmp_path):
            os.remove(tmp_path)
        print(f"ERROR: Write failed — {e}", file=sys.stderr)
        sys.exit(1)

    # --- Summary ---
    if args.format == "json":
        summary = {
            "status": "ok",
            "old_version": current_ver,
            "new_version": new_ver,
            "output": out_path,
            "backup": backup_path,
            "changelog_written": args.message is not None,
        }
        print(json.dumps(summary, indent=2))
    else:
        print(f"Bumped {changes['pack_name']}  {current_ver} → {new_ver}  ({args.bump})")
        print(f"  Output: {out_path}")
        if backup_path:
            print(f"  Backup: {backup_path}")
        if args.message:
            print(f"  CHANGELOG.md entry written.")


if __name__ == "__main__":
    main()
