#!/usr/bin/env python3
"""
xpn_xometa_schema_version_migrator.py

Migrates .xometa preset files between schema versions for the XO_OX XOlokun project.

Usage:
    python xpn_xometa_schema_version_migrator.py <dir> [--dry-run] [--migrate]
                                                         [--target-version 3]
                                                         [--output report.txt]
"""

import argparse
import json
import sys
from pathlib import Path
from typing import Dict, List, Optional, Tuple


CURRENT_VERSION = 3


# ---------------------------------------------------------------------------
# Version detection
# ---------------------------------------------------------------------------

def detect_version(data: dict) -> int:
    """Return the integer schema version of a parsed .xometa file."""
    if "sonicDNA" in data:
        return 0

    sv = data.get("schema_version")

    if sv == "3":
        return 3
    if sv == "2":
        return 2
    if sv == "1":
        return 1

    # No schema_version field — distinguish v1 from current by key presence
    if "version" in data:
        return 3  # has version field => current

    if "engines" in data:
        # array engines without schema_version => treat as v2
        return 2

    if "engine" in data:
        # single engine string without schema_version => v1
        return 1

    # Fallback: treat as v1 (oldest recognised format without sonicDNA)
    return 1


# ---------------------------------------------------------------------------
# Individual migration steps
# ---------------------------------------------------------------------------

def migrate_v0_to_v1(data: dict) -> dict:
    """v0 → v1: rename sonicDNA→dna, presetName→name, engineId→engine."""
    out = dict(data)

    if "sonicDNA" in out:
        out["dna"] = out.pop("sonicDNA")

    if "presetName" in out:
        out["name"] = out.pop("presetName")

    if "engineId" in out:
        out["engine"] = out.pop("engineId")

    out["schema_version"] = "1"
    return out


def migrate_v1_to_v2(data: dict) -> dict:
    """v1 → v2: engine (string) → engines (array), set schema_version: "2"."""
    out = dict(data)

    if "engine" in out and not isinstance(out["engine"], list):
        engine_val = out.pop("engine")
        out["engines"] = [engine_val] if engine_val else []

    out["schema_version"] = "2"
    return out


def migrate_v2_to_v3(data: dict) -> dict:
    """v2 → v3 (current): add version field, normalize macros, add tags."""
    out = dict(data)

    # Add version field if missing
    if "version" not in out:
        out["version"] = "1.0"

    # Normalize macros: if it's a list, convert to object with string keys
    if "macros" in out and isinstance(out["macros"], list):
        macros_list = out["macros"]
        macros_obj = {}
        for i, item in enumerate(macros_list):
            if isinstance(item, dict):
                # Use 'name' key as the object key if available, else index
                key = item.get("name", str(i))
                macros_obj[str(key)] = item
            else:
                macros_obj[str(i)] = item
        out["macros"] = macros_obj

    # Add empty tags array if missing
    if "tags" not in out:
        out["tags"] = []

    out["schema_version"] = "3"
    return out


# ---------------------------------------------------------------------------
# Multi-step migration chain
# ---------------------------------------------------------------------------

MIGRATION_STEPS = {
    0: migrate_v0_to_v1,
    1: migrate_v1_to_v2,
    2: migrate_v2_to_v3,
}


def migrate_to_version(data: dict, from_version: int, to_version: int) -> dict:
    """Apply migration steps from from_version up to to_version."""
    current = dict(data)
    for step in range(from_version, to_version):
        fn = MIGRATION_STEPS.get(step)
        if fn is None:
            raise ValueError(f"No migration step defined for v{step} → v{step + 1}")
        current = fn(current)
    return current


# ---------------------------------------------------------------------------
# File I/O helpers
# ---------------------------------------------------------------------------

def load_xometa(path: Path) -> Optional[Dict]:
    """Parse a .xometa JSON file. Returns None on error."""
    try:
        text = path.read_text(encoding="utf-8")
        return json.loads(text)
    except (json.JSONDecodeError, OSError) as exc:
        return None


def write_xometa(path: Path, data: dict) -> None:
    """Write data back to a .xometa file with consistent formatting."""
    path.write_text(json.dumps(data, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")


# ---------------------------------------------------------------------------
# Directory scan
# ---------------------------------------------------------------------------

def scan_directory(root: Path) -> List[Tuple[Path, Dict, int]]:
    """
    Recursively find all .xometa files under root.
    Returns list of (path, parsed_data, version) tuples.
    Skips files that fail to parse.
    """
    results = []
    for xometa_path in sorted(root.rglob("*.xometa")):
        data = load_xometa(xometa_path)
        if data is None:
            print(f"  [WARN] Could not parse {xometa_path} — skipping", file=sys.stderr)
            continue
        version = detect_version(data)
        results.append((xometa_path, data, version))
    return results


# ---------------------------------------------------------------------------
# Reporting
# ---------------------------------------------------------------------------

def build_report(
    entries: List[Tuple[Path, Dict, int]],
    target_version: int,
    migrated: Optional[List[Path]] = None,
    is_dry_run: bool = True,
) -> str:
    lines = []

    # Version distribution
    version_counts: dict[int, int] = {}
    for _, _, v in entries:
        version_counts[v] = version_counts.get(v, 0) + 1

    total = len(entries)
    lines.append("=" * 60)
    lines.append("XOmeta Schema Version Migrator — Report")
    lines.append("=" * 60)
    lines.append(f"Total .xometa files found : {total}")
    lines.append(f"Target schema version      : v{target_version}")
    lines.append("")
    lines.append("Version Distribution:")
    for v in sorted(version_counts):
        label = "(current)" if v == CURRENT_VERSION else ""
        lines.append(f"  v{v} : {version_counts[v]:4d} file(s) {label}")

    needs_migration: List[Tuple[Path, Dict, int]] = [(p, d, v) for p, d, v in entries if v < target_version]
    already_current: List[Path] = [p for p, _, v in entries if v >= target_version]

    lines.append("")
    lines.append(f"Already at v{target_version}+           : {len(already_current)}")
    lines.append(f"Need migration to v{target_version}    : {len(needs_migration)}")

    if needs_migration:
        lines.append("")
        if is_dry_run:
            lines.append("Files that WOULD be migrated (dry-run):")
        else:
            lines.append("Files migrated:")
        for p, _, v in needs_migration:
            status = ""
            if migrated is not None:
                status = " [OK]" if p in migrated else " [SKIPPED]"
            lines.append(f"  v{v} → v{target_version}  {p}{status}")

    lines.append("")
    if is_dry_run:
        lines.append("(Dry-run mode — no files were modified. Use --migrate to apply.)")
    else:
        count = len(migrated) if migrated else 0
        lines.append(f"Migration complete. {count} file(s) written.")

    lines.append("=" * 60)
    return "\n".join(lines)


# ---------------------------------------------------------------------------
# CLI entry point
# ---------------------------------------------------------------------------

def main() -> int:
    parser = argparse.ArgumentParser(
        description="Migrate .xometa preset files between schema versions.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    parser.add_argument(
        "directory",
        type=Path,
        help="Root directory to scan for .xometa files (searched recursively).",
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        default=False,
        help="Report version distribution and migration plan without writing files.",
    )
    parser.add_argument(
        "--migrate",
        action="store_true",
        default=False,
        help="Apply all migrations in-place.",
    )
    parser.add_argument(
        "--target-version",
        type=int,
        default=CURRENT_VERSION,
        metavar="N",
        help=f"Migrate to this schema version (default: {CURRENT_VERSION}).",
    )
    parser.add_argument(
        "--output",
        type=Path,
        default=None,
        metavar="FILE",
        help="Write report to this file in addition to stdout.",
    )

    args = parser.parse_args()

    if not args.directory.is_dir():
        print(f"Error: '{args.directory}' is not a directory.", file=sys.stderr)
        return 1

    if args.target_version < 1 or args.target_version > CURRENT_VERSION:
        print(
            f"Error: --target-version must be between 1 and {CURRENT_VERSION}.",
            file=sys.stderr,
        )
        return 1

    if not args.dry_run and not args.migrate:
        # Default to dry-run when neither flag given
        args.dry_run = True
        print("No action flag given — defaulting to --dry-run.\n")

    # Scan
    print(f"Scanning {args.directory} …")
    entries = scan_directory(args.directory)

    if not entries:
        print("No .xometa files found.")
        return 0

    migrated_paths: List[Path] = []

    if args.migrate:
        needs_migration = [(p, d, v) for p, d, v in entries if v < args.target_version]
        for path, data, version in needs_migration:
            try:
                migrated_data = migrate_to_version(data, version, args.target_version)
                write_xometa(path, migrated_data)
                migrated_paths.append(path)
            except Exception as exc:
                print(f"  [ERROR] Failed to migrate {path}: {exc}", file=sys.stderr)

    report = build_report(
        entries,
        target_version=args.target_version,
        migrated=migrated_paths if args.migrate else None,
        is_dry_run=args.dry_run,
    )

    print(report)

    if args.output:
        try:
            args.output.write_text(report + "\n", encoding="utf-8")
            print(f"\nReport written to {args.output}")
        except OSError as exc:
            print(f"[WARN] Could not write report to {args.output}: {exc}", file=sys.stderr)

    return 0


if __name__ == "__main__":
    sys.exit(main())
