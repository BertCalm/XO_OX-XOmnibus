#!/usr/bin/env python3
"""
xpn_dedup_checker.py — SHA-256 hash index for XO_OX sample deduplication.

Usage:
  python xpn_dedup_checker.py --index known_hashes.json
      Print index stats (count, last updated).

  python xpn_dedup_checker.py --add ./stems/ --index known_hashes.json [--pack-name "Pack Name"]
      Add all WAVs in directory (recursive) to the index.

  python xpn_dedup_checker.py --check ./new_stems/ --index known_hashes.json
      Check directory of new samples against index, report duplicates.

  python xpn_dedup_checker.py --check-xpn pack.xpn --index known_hashes.json
      Check all WAVs inside a .xpn ZIP against index, report duplicates.
"""

import argparse
import hashlib
import json
import os
import sys
import zipfile
from datetime import date
from pathlib import Path
from typing import Dict, List, Optional, Tuple


INDEX_VERSION = 1
DEFAULT_INDEX = "known_hashes.json"


# ---------------------------------------------------------------------------
# Index I/O
# ---------------------------------------------------------------------------

def load_index(path: str) -> dict:
    """Load existing index or return a fresh empty one."""
    if os.path.exists(path):
        with open(path, "r", encoding="utf-8") as f:
            data = json.load(f)
        # Basic version guard
        if data.get("version") != INDEX_VERSION:
            print(f"Warning: index version {data.get('version')} != expected {INDEX_VERSION}. Proceeding anyway.")
        return data
    return {
        "version": INDEX_VERSION,
        "updated": str(date.today()),
        "count": 0,
        "hashes": {},
    }


def save_index(index: dict, path: str) -> None:
    index["updated"] = str(date.today())
    index["count"] = len(index["hashes"])
    with open(path, "w", encoding="utf-8") as f:
        json.dump(index, f, indent=2)


# ---------------------------------------------------------------------------
# Hashing helpers
# ---------------------------------------------------------------------------

def sha256_file(filepath: str) -> str:
    h = hashlib.sha256()
    with open(filepath, "rb") as f:
        for chunk in iter(lambda: f.read(65536), b""):
            h.update(chunk)
    return h.hexdigest()


def sha256_bytes(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def collect_wavs_from_dir(directory: str):
    """Yield (absolute_path, filename) for all WAV files under directory."""
    root = Path(directory)
    if not root.is_dir():
        print(f"Error: '{directory}' is not a directory.", file=sys.stderr)
        sys.exit(1)
    for p in sorted(root.rglob("*.wav")):
        yield str(p), p.name
    for p in sorted(root.rglob("*.WAV")):
        yield str(p), p.name


def collect_wavs_from_xpn(xpn_path: str):
    """Yield (hash, filename) for all WAV entries inside a .xpn ZIP."""
    if not os.path.exists(xpn_path):
        print(f"Error: '{xpn_path}' not found.", file=sys.stderr)
        sys.exit(1)
    results = []
    with zipfile.ZipFile(xpn_path, "r") as zf:
        for entry in zf.infolist():
            name_lower = entry.filename.lower()
            if name_lower.endswith(".wav"):
                data = zf.read(entry.filename)
                digest = sha256_bytes(data)
                filename = Path(entry.filename).name
                results.append((digest, filename))
    return results


# ---------------------------------------------------------------------------
# Filename similarity helpers
# ---------------------------------------------------------------------------

def base_name(filename: str) -> str:
    """Return stem without extension, lowercased."""
    return Path(filename).stem.lower()


def build_filename_map(index: dict) -> dict:
    """Build a map of base_name -> list of (hash, entry) for revision detection."""
    mapping: dict[str, list] = {}
    for digest, entry in index["hashes"].items():
        bn = base_name(entry["filename"])
        mapping.setdefault(bn, []).append((digest, entry))
    return mapping


# ---------------------------------------------------------------------------
# Commands
# ---------------------------------------------------------------------------

def cmd_stats(index_path: str) -> None:
    index = load_index(index_path)
    if index["count"] == 0 and not index["hashes"]:
        print(f"Index: {index_path}  (empty / not yet created)")
    else:
        print(f"Index path  : {index_path}")
        print(f"Version     : {index['version']}")
        print(f"Last updated: {index['updated']}")
        print(f"Total hashes: {index['count']}")

        # Pack provenance summary
        pack_counts: dict[str, int] = {}
        for entry in index["hashes"].values():
            pack = entry.get("pack") or "(untagged)"
            pack_counts[pack] = pack_counts.get(pack, 0) + 1
        if pack_counts:
            print("\nBy pack:")
            for pack, cnt in sorted(pack_counts.items(), key=lambda x: -x[1]):
                print(f"  {cnt:>5}  {pack}")


def cmd_add(directory: str, index_path: str, pack_name: Optional[str]) -> None:
    index = load_index(index_path)
    hashes = index["hashes"]
    added = 0
    skipped_dup = 0
    today = str(date.today())

    for filepath, filename in collect_wavs_from_dir(directory):
        digest = sha256_file(filepath)
        if digest in hashes:
            skipped_dup += 1
            continue
        hashes[digest] = {
            "filename": filename,
            "pack": pack_name or "",
            "added": today,
        }
        added += 1

    save_index(index, index_path)
    print(f"Added   : {added} new sample(s)")
    print(f"Skipped : {skipped_dup} exact duplicate(s) already in index")
    print(f"Index now contains {index['count']} hashes → {index_path}")


def cmd_check_dir(directory: str, index_path: str) -> None:
    index = load_index(index_path)
    hashes = index["hashes"]
    filename_map = build_filename_map(index)

    exact_dups = []
    revisions = []
    clean = 0

    for filepath, filename in collect_wavs_from_dir(directory):
        digest = sha256_file(filepath)
        if digest in hashes:
            existing = hashes[digest]
            exact_dups.append({
                "new_file": filepath,
                "matched_filename": existing["filename"],
                "pack": existing.get("pack") or "(untagged)",
                "added": existing.get("added", "?"),
                "hash": digest,
            })
        else:
            # Check for same base name but different hash (possible revision)
            bn = base_name(filename)
            if bn in filename_map:
                for existing_hash, existing_entry in filename_map[bn]:
                    revisions.append({
                        "new_file": filepath,
                        "new_hash": digest,
                        "existing_filename": existing_entry["filename"],
                        "existing_hash": existing_hash,
                        "pack": existing_entry.get("pack") or "(untagged)",
                    })
            else:
                clean += 1

    _print_check_report(exact_dups, revisions, clean)


def cmd_check_xpn(xpn_path: str, index_path: str) -> None:
    index = load_index(index_path)
    hashes = index["hashes"]
    filename_map = build_filename_map(index)

    exact_dups = []
    revisions = []
    clean = 0

    for digest, filename in collect_wavs_from_xpn(xpn_path):
        if digest in hashes:
            existing = hashes[digest]
            exact_dups.append({
                "new_file": filename,
                "matched_filename": existing["filename"],
                "pack": existing.get("pack") or "(untagged)",
                "added": existing.get("added", "?"),
                "hash": digest,
            })
        else:
            bn = base_name(filename)
            if bn in filename_map:
                for existing_hash, existing_entry in filename_map[bn]:
                    revisions.append({
                        "new_file": filename,
                        "new_hash": digest,
                        "existing_filename": existing_entry["filename"],
                        "existing_hash": existing_hash,
                        "pack": existing_entry.get("pack") or "(untagged)",
                    })
            else:
                clean += 1

    _print_check_report(exact_dups, revisions, clean)


def _print_check_report(exact_dups: list, revisions: list, clean: int) -> None:
    total = len(exact_dups) + len(revisions) + clean

    print(f"Checked : {total} sample(s)")
    print(f"Clean   : {clean}")
    print(f"Exact duplicates  : {len(exact_dups)}")
    print(f"Possible revisions: {len(revisions)}")

    if exact_dups:
        print("\n── EXACT DUPLICATES (same hash) ──────────────────────────────")
        for d in exact_dups:
            print(f"  NEW      : {d['new_file']}")
            print(f"  MATCHES  : {d['matched_filename']}  [{d['pack']}]  added {d['added']}")
            print(f"  HASH     : {d['hash'][:16]}...")
            print()

    if revisions:
        print("── POSSIBLE REVISIONS (same name, different hash) ────────────")
        for r in revisions:
            print(f"  NEW      : {r['new_file']}")
            print(f"    hash   : {r['new_hash'][:16]}...")
            print(f"  EXISTING : {r['existing_filename']}  [{r['pack']}]")
            print(f"    hash   : {r['existing_hash'][:16]}...")
            print()

    if not exact_dups and not revisions:
        print("\nAll samples are unique. No duplicates found.")


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def main() -> None:
    parser = argparse.ArgumentParser(
        description="XO_OX sample deduplication checker — SHA-256 hash index.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    parser.add_argument("--index", default=DEFAULT_INDEX, metavar="PATH",
                        help="Path to the JSON hash index (default: known_hashes.json)")
    parser.add_argument("--add", metavar="DIR",
                        help="Add all WAVs in DIR (recursive) to the index")
    parser.add_argument("--pack-name", metavar="NAME",
                        help="Tag newly added samples with this pack name (use with --add)")
    parser.add_argument("--check", metavar="DIR",
                        help="Check all WAVs in DIR against the index")
    parser.add_argument("--check-xpn", metavar="FILE",
                        help="Check all WAVs inside a .xpn ZIP against the index")

    args = parser.parse_args()

    # Validate mutual exclusivity of actions
    actions = [bool(args.add), bool(args.check), bool(args.check_xpn)]
    if sum(actions) > 1:
        parser.error("Only one of --add, --check, --check-xpn may be used at a time.")

    if args.pack_name and not args.add:
        parser.error("--pack-name requires --add.")

    if args.add:
        cmd_add(args.add, args.index, args.pack_name)
    elif args.check:
        cmd_check_dir(args.check, args.index)
    elif args.check_xpn:
        cmd_check_xpn(args.check_xpn, args.index)
    else:
        # Default: print stats
        cmd_stats(args.index)


if __name__ == "__main__":
    main()
