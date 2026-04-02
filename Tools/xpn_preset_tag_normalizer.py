#!/usr/bin/env python3
"""
XOceanus Preset Tag Normalizer — Standardize tags across .xometa preset files

Scans .xometa files, normalizes tags to canonical lowercase vocabulary,
maps known variants/misspellings, flags unknowns, and optionally applies changes.

Usage:
    python3 xpn_preset_tag_normalizer.py <presets_dir> [options]

Options:
    --engine FILTER     Only process presets whose path contains FILTER
    --dry-run           Show proposed changes (default behavior)
    --apply             Write normalized tags back to files (creates .bak backups)
    --add-tag TAG       Add TAG to the approved vocabulary (printed for reference)
    --format text|json  Output format (default: text)
"""

import argparse
import json
import glob
import shutil
import sys
from collections import defaultdict, Counter
from pathlib import Path
from datetime import datetime
from typing import Dict, List, Optional, Set, Tuple

# ---------------------------------------------------------------------------
# Approved tag vocabulary
# ---------------------------------------------------------------------------

APPROVED_TAGS = {
    # Sonic character
    "bright", "warm", "dark", "clean", "dirty", "harsh", "smooth", "silky",
    "gritty", "crisp",
    # Mood / feel
    "atmospheric", "cinematic", "organic", "electronic", "vintage", "modern",
    "lo-fi", "raw", "polished",
    # Functional
    "bass", "lead", "pad", "drum", "percussion", "fx", "texture", "chord",
    "arp", "melody",
    # Genre
    "hip-hop", "trap", "jazz", "ambient", "experimental", "classical", "dub",
    "reggae", "soul", "funk",
    # Technical
    "layered", "mono", "stereo", "pitched", "unpitched", "sustained",
    "transient", "evolving", "static",
    # XO_OX specific
    "feliX", "oscar", "coupling", "entangled", "MPCe", "doctrine",
}

# ---------------------------------------------------------------------------
# Variant → canonical mapping (applied after lowercasing)
# ---------------------------------------------------------------------------

VARIANTS: Dict[str, str] = {
    # lo-fi
    "lo fi":    "lo-fi",
    "lofi":     "lo-fi",
    "lo_fi":    "lo-fi",
    # hip-hop
    "hiphop":   "hip-hop",
    "hip hop":  "hip-hop",
    "hip_hop":  "hip-hop",
    # common case / spelling variants
    "atmos":    "atmospheric",
    "cine":     "cinematic",
    "eletro":   "electronic",
    "electro":  "electronic",
    "synth":    "electronic",
    "brght":    "bright",
    "drk":      "dark",
    "brt":      "bright",
    "smth":     "smooth",
    "grty":     "gritty",
    "prc":      "percussion",
    "perc":     "percussion",
    "drums":    "drum",
    "chords":   "chord",
    "arpegio":  "arp",
    "arpeggio": "arp",
    "arped":    "arp",
    "leads":    "lead",
    "pads":     "pad",
    "textures": "texture",
    "melodies": "melody",
    "mpc":      "MPCe",
    "mpce":     "MPCe",
    "felix":    "feliX",
    "xfelix":   "feliX",
    "Oscar":    "oscar",
}

# Tags that are already canonical when lowercased, but need exact case preserved
CASE_SENSITIVE_TAGS = {"feliX", "MPCe"}
# Build a lookup: lowercase → canonical (for case-sensitive tags)
LOWERCASE_TO_CANONICAL = {t.lower(): t for t in CASE_SENSITIVE_TAGS}


def normalize_tag(raw: str) -> Tuple[str, Optional[str]]:
    """
    Normalize a single tag.
    Returns (normalized_tag, reason) where reason describes what changed,
    or (normalized_tag, None) if no change.
    """
    stripped = raw.strip()
    lowered = stripped.lower()

    # Step 1: apply variant mapping (on lowercased form)
    if lowered in VARIANTS:
        canonical = VARIANTS[lowered]
        reason = "variant" if lowered != stripped.lower() else "variant"
        # If raw was already the canonical, no change needed
        if stripped == canonical:
            return canonical, None
        return canonical, "variant"

    # Step 2: restore exact case for case-sensitive tags
    if lowered in LOWERCASE_TO_CANONICAL:
        canonical = LOWERCASE_TO_CANONICAL[lowered]
        if stripped == canonical:
            return canonical, None
        return canonical, "case"

    # Step 3: lowercase everything else
    if stripped != lowered:
        return lowered, "case"

    return lowered, None


def normalize_tags(raw_tags: list) -> Tuple[List[str], List[Tuple[str, str, str]]]:
    """
    Normalize a list of tags.
    Returns (normalized_list, changes) where changes is list of (original, normalized, reason).
    Deduplication is applied after normalization.
    """
    if not isinstance(raw_tags, list):
        return [], []

    changes: List[Tuple[str, str, str]] = []
    seen: Set[str] = set()
    result: List[str] = []

    for raw in raw_tags:
        if not isinstance(raw, str):
            continue
        normalized, reason = normalize_tag(raw)
        if reason is not None:
            changes.append((raw, normalized, reason))
        if normalized not in seen:
            seen.add(normalized)
            result.append(normalized)
        else:
            # duplicate (possibly after normalization)
            if reason is None:
                changes.append((raw, normalized, "duplicate"))

    return result, changes


def is_unknown(tag: str) -> bool:
    """Return True if tag is not in the approved vocabulary."""
    # Check both exact and lowercased
    return tag not in APPROVED_TAGS and tag.lower() not in {t.lower() for t in APPROVED_TAGS}


# ---------------------------------------------------------------------------
# File scanning
# ---------------------------------------------------------------------------

def scan_directory(presets_dir: Path, engine_filter: Optional[str]) -> List[Path]:
    """Return sorted list of .xometa files, optionally filtered by engine name."""
    pattern = str(presets_dir / "**" / "*.xometa")
    files = [Path(p) for p in glob.glob(pattern, recursive=True)]
    if engine_filter:
        files = [f for f in files if engine_filter.lower() in str(f).lower()]
    return sorted(files)


def load_xometa(path: Path) -> Optional[dict]:
    try:
        with open(path, "r", encoding="utf-8") as fh:
            return json.load(fh)
    except (json.JSONDecodeError, OSError) as e:
        print(f"  [ERROR] Could not read {path}: {e}", file=sys.stderr)
        return None


def save_xometa(path: Path, data: dict, backup: bool = True) -> None:
    if backup:
        shutil.copy2(path, path.with_suffix(".xometa.bak"))
    with open(path, "w", encoding="utf-8") as fh:
        json.dump(data, fh, indent=2, ensure_ascii=False)
        fh.write("\n")


# ---------------------------------------------------------------------------
# Main audit logic
# ---------------------------------------------------------------------------

def audit(
    presets_dir: Path,
    engine_filter: Optional[str],
    apply: bool,
    format_: str,
    extra_vocab: Set[str],
) -> None:
    effective_vocab = APPROVED_TAGS | extra_vocab
    files = scan_directory(presets_dir, engine_filter)
    total = len(files)

    if total == 0:
        print(f"No .xometa files found in {presets_dir}")
        return

    # Collect results
    file_results: List[dict] = []   # {path, changes, unknowns, normalized_tags}
    unknown_counter: Counter = Counter()
    changed_count = 0

    for path in files:
        data = load_xometa(path)
        if data is None:
            continue

        raw_tags = data.get("tags", [])
        normalized, changes = normalize_tags(raw_tags)

        # Detect unknowns in the normalized list
        unknowns = [t for t in normalized if t not in effective_vocab
                    and t.lower() not in {v.lower() for v in effective_vocab}]
        for u in unknowns:
            unknown_counter[u] += 1

        has_changes = bool(changes)
        if has_changes:
            changed_count += 1

        file_results.append({
            "path": path,
            "changes": changes,
            "unknowns": unknowns,
            "normalized_tags": normalized,
            "original_tags": raw_tags,
            "data": data,
        })

        if apply and has_changes:
            data["tags"] = normalized
            save_xometa(path, data, backup=True)

    # ---------------------------------------------------------------------------
    # Output
    # ---------------------------------------------------------------------------
    if format_ == "json":
        output = {
            "timestamp": datetime.now().isoformat(),
            "total_presets": total,
            "changed_presets": changed_count,
            "applied": apply,
            "unknown_tags": dict(unknown_counter.most_common()),
            "files": [
                {
                    "path": str(r["path"]),
                    "changes": [
                        {"from": c[0], "to": c[1], "reason": c[2]}
                        for c in r["changes"]
                    ],
                    "unknowns": r["unknowns"],
                }
                for r in file_results
                if r["changes"] or r["unknowns"]
            ],
        }
        print(json.dumps(output, indent=2))
        return

    # Text output
    mode_label = "APPLIED" if apply else "DRY RUN"
    print(f"\nTAG NORMALIZATION AUDIT — {total} presets  [{mode_label}]")
    print(f"{'='*60}")

    if changed_count == 0 and not unknown_counter:
        print("\nAll tags are clean. No changes needed.")
    else:
        print(f"\nChanges {'applied' if apply else 'proposed'}: {changed_count} presets\n")

        for r in file_results:
            if not r["changes"] and not r["unknowns"]:
                continue
            rel = r["path"].relative_to(presets_dir) if r["path"].is_relative_to(presets_dir) else r["path"]
            print(f"  {rel}")
            for orig, norm, reason in r["changes"]:
                if reason == "duplicate":
                    print(f'    "{orig}" — DUPLICATE (removed)')
                else:
                    print(f'    "{orig}" → "{norm}"  ({reason})')
            for u in r["unknowns"]:
                print(f'    "{u}" — UNKNOWN TAG (not in vocabulary)')
            print()

    if unknown_counter:
        top = unknown_counter.most_common(10)
        print(f"Unknown tags fleet-wide (top {len(top)}):")
        for tag, count in top:
            print(f'  "{tag}"  {count} use{"s" if count != 1 else ""}')
        print()

    if apply:
        print(f"Changes written. Backups saved as .xometa.bak")
    elif changed_count > 0 or unknown_counter:
        print(f"To apply: rerun with --apply")


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def main() -> None:
    parser = argparse.ArgumentParser(
        description="Normalize tags across XOceanus .xometa preset files."
    )
    parser.add_argument(
        "presets_dir",
        type=Path,
        help="Root directory to scan for .xometa files",
    )
    parser.add_argument(
        "--engine",
        metavar="FILTER",
        help="Only process files whose path contains FILTER (case-insensitive)",
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        default=False,
        help="Show proposed changes without writing (default)",
    )
    parser.add_argument(
        "--apply",
        action="store_true",
        default=False,
        help="Write normalized tags to files (creates .bak backups)",
    )
    parser.add_argument(
        "--add-tag",
        metavar="TAG",
        action="append",
        dest="extra_tags",
        default=[],
        help="Add TAG to approved vocabulary for this run (can repeat)",
    )
    parser.add_argument(
        "--format",
        choices=["text", "json"],
        default="text",
        help="Output format (default: text)",
    )
    args = parser.parse_args()

    if not args.presets_dir.exists():
        print(f"Error: directory not found: {args.presets_dir}", file=sys.stderr)
        sys.exit(1)

    if args.apply and args.dry_run:
        print("Error: --dry-run and --apply are mutually exclusive.", file=sys.stderr)
        sys.exit(1)

    if args.extra_tags and args.format == "text":
        print(f"Vocabulary extended with: {', '.join(args.extra_tags)}\n")

    audit(
        presets_dir=args.presets_dir,
        engine_filter=args.engine,
        apply=args.apply,
        format_=args.format,
        extra_vocab=set(args.extra_tags),
    )


if __name__ == "__main__":
    main()
