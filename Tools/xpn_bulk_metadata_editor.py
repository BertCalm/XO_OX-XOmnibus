#!/usr/bin/env python3
"""
XPN Bulk Metadata Editor — XO_OX Designs
Bulk-edits metadata fields across all manifest files inside one or many .xpn
pack archives, without touching samples or program audio content.

Editable fields
---------------
--version VERSION       Updates version in:
                          • Expansions/manifest  (plain-text key=value)
                          • expansion.json        (if present)
                          • bundle_manifest.json  (if present)
                          • Programs/*.xpm        (<Program version="…"> attr)
--author AUTHOR         Updates author in bundle_manifest.json (and manifest)
--mood MOOD             Updates mood in bundle_manifest.json
                        (validated against the 7 XO_OX moods)
--preset-count N        Updates preset_count in bundle_manifest.json
--set-dna k=v,k=v       Updates specific sonic_dna keys in bundle_manifest.json
                        (e.g. brightness=0.7,warmth=0.4); other keys unchanged
--add-tags tag1,tag2    Appends tags to bundle_manifest.json tags array
--set-status STATUS     Sets status field in bundle_manifest.json
                        (planned | beta | released)

IO
--
--xpn FILE              Single .xpn archive to edit
--xpns-dir DIR          Directory; edits all *.xpn files found (non-recursive)
--output-dir DIR        Write modified packs here (default: overwrite in-place
                        with a .xpn.bak backup beside the original)
--dry-run               Print what would change; write nothing

Usage examples
--------------
  python3 xpn_bulk_metadata_editor.py \\
      --xpns-dir ./dist/ --version 1.1.0 --author "XO_OX Designs" --dry-run

  python3 xpn_bulk_metadata_editor.py \\
      --xpn OpalPack.xpn \\
      --set-dna brightness=0.8,warmth=0.3 \\
      --add-tags ambient,layered \\
      --output-dir ./edited/

  python3 xpn_bulk_metadata_editor.py \\
      --xpns-dir ./dist/ \\
      --mood Foundation \\
      --set-status released
"""

import argparse
import io
import json
import os
import re
import shutil
import sys
import zipfile
from pathlib import Path

# ---------------------------------------------------------------------------
# Constants
# ---------------------------------------------------------------------------

KNOWN_MOODS = {"Foundation", "Atmosphere", "Entangled", "Prism", "Flux", "Aether", "Family"}
KNOWN_STATUSES = {"planned", "beta", "released"}

# ---------------------------------------------------------------------------
# Helpers — key=value manifest (Expansions/manifest plain-text)
# ---------------------------------------------------------------------------

def _parse_kv_manifest(text: str) -> dict:
    """Parse Name=value lines into a dict. Preserves order."""
    result = {}
    for line in text.splitlines():
        line = line.strip()
        if "=" in line:
            k, _, v = line.partition("=")
            result[k.strip()] = v.strip()
    return result


def _render_kv_manifest(data: dict) -> str:
    return "".join(f"{k}={v}\n" for k, v in data.items())


# ---------------------------------------------------------------------------
# Per-file edit logic
# ---------------------------------------------------------------------------

def _edit_kv_manifest(original: str, args) -> tuple[str, list[str], list[str]]:
    """Returns (new_text, changed_list, skipped_list)."""
    data = _parse_kv_manifest(original)
    changed, skipped = [], []

    if args.version is not None:
        key = "Version"
        old = data.get(key)
        if old == args.version:
            skipped.append(f"manifest.{key} already '{args.version}'")
        else:
            data[key] = args.version
            changed.append(f"manifest.{key}: '{old}' → '{args.version}'")

    if args.author is not None:
        key = "Author"
        old = data.get(key)
        if old == args.author:
            skipped.append(f"manifest.{key} already '{args.author}'")
        else:
            data[key] = args.author
            changed.append(f"manifest.{key}: '{old}' → '{args.author}'")

    return _render_kv_manifest(data), changed, skipped


def _edit_json_file(original_bytes: bytes, filename: str, args) -> tuple[bytes, list[str], list[str]]:
    """
    Edit expansion.json or bundle_manifest.json.
    Returns (new_bytes, changed_list, skipped_list).
    """
    try:
        data = json.loads(original_bytes.decode("utf-8"))
    except (json.JSONDecodeError, UnicodeDecodeError) as exc:
        return original_bytes, [], [f"{filename}: parse error ({exc}) — skipped"]

    changed, skipped = [], []

    def _set(obj, key, new_val, label):
        old = obj.get(key)
        if old == new_val:
            skipped.append(f"{filename}.{label} already '{new_val}'")
        else:
            obj[key] = new_val
            changed.append(f"{filename}.{label}: '{old}' → '{new_val}'")

    if args.version is not None:
        if "version" in data:
            _set(data, "version", args.version, "version")

    if args.author is not None:
        if "author" in data:
            _set(data, "author", args.author, "author")

    if args.mood is not None:
        if "mood" in data:
            _set(data, "mood", args.mood, "mood")

    if args.preset_count is not None:
        if "preset_count" in data:
            _set(data, "preset_count", args.preset_count, "preset_count")

    if args.set_dna:
        dna = data.get("sonic_dna")
        if isinstance(dna, dict):
            for k, v in args.set_dna.items():
                old = dna.get(k)
                if old == v:
                    skipped.append(f"{filename}.sonic_dna.{k} already {v}")
                else:
                    dna[k] = v
                    changed.append(f"{filename}.sonic_dna.{k}: {old} → {v}")
        else:
            skipped.append(f"{filename}: no sonic_dna dict found")

    if args.add_tags:
        tags = data.get("tags")
        if isinstance(tags, list):
            new_tags = [t for t in args.add_tags if t not in tags]
            if new_tags:
                data["tags"] = tags + new_tags
                changed.append(f"{filename}.tags: added {new_tags}")
            else:
                skipped.append(f"{filename}.tags: all tags already present")
        else:
            skipped.append(f"{filename}: no tags array found")

    if args.set_status is not None:
        if "status" in data:
            _set(data, "status", args.set_status, "status")
        else:
            skipped.append(f"{filename}: no status field present — skipped")

    new_bytes = json.dumps(data, indent=2, ensure_ascii=False).encode("utf-8")
    return new_bytes, changed, skipped


def _edit_xpm(original_bytes: bytes, name: str, args) -> tuple[bytes, list[str], list[str]]:
    """Update <Program version="…"> attribute in an XPM XML file."""
    if args.version is None:
        return original_bytes, [], []

    text = original_bytes.decode("utf-8", errors="replace")
    pattern = re.compile(r'(<Program\b[^>]*\bversion=")[^"]*(")', re.IGNORECASE)
    changed, skipped = [], []

    def _replacer(m):
        old_ver = m.group(0).split('version="')[1].rstrip('"').split('"')[0]
        if old_ver == args.version:
            return m.group(0)
        changed.append(f"{name}: version '{old_ver}' → '{args.version}'")
        return m.group(1) + args.version + m.group(2)

    new_text = pattern.sub(_replacer, text)
    if not changed:
        if pattern.search(text):
            skipped.append(f"{name}: version already '{args.version}'")
    return new_text.encode("utf-8", errors="replace"), changed, skipped


# ---------------------------------------------------------------------------
# Per-archive processing
# ---------------------------------------------------------------------------

def _process_xpn(src_path: Path, dest_path: Path, args, dry_run: bool) -> bool:
    """
    Open src_path, apply edits, write to dest_path.
    Returns True if any changes were made.
    """
    print(f"\n{'[DRY RUN] ' if dry_run else ''}Processing: {src_path.name}")

    all_changed: list[str] = []
    all_skipped: list[str] = []

    if not zipfile.is_zipfile(src_path):
        print(f"  ERROR: {src_path} is not a valid ZIP/XPN archive — skipped")
        return False

    # Read all member bytes into memory so we can rewrite the archive
    with zipfile.ZipFile(src_path, "r") as zin:
        members = zin.namelist()
        member_bytes: dict[str, bytes] = {m: zin.read(m) for m in members}
        # Preserve ZIP metadata (compression, etc.)
        member_info: dict[str, zipfile.ZipInfo] = {m: zin.getinfo(m) for m in members}

    new_member_bytes: dict[str, bytes] = {}

    for member_name, raw in member_bytes.items():
        lower = member_name.lower()
        base = os.path.basename(member_name).lower()

        # Plain-text key=value manifest (Expansions/manifest, no extension)
        if base == "manifest" and "expansions/" in lower:
            text = raw.decode("utf-8", errors="replace")
            new_text, ch, sk = _edit_kv_manifest(text, args)
            all_changed.extend([f"  CHANGED  {x}" for x in ch])
            all_skipped.extend([f"  skipped  {x}" for x in sk])
            new_member_bytes[member_name] = new_text.encode("utf-8")

        # expansion.json
        elif base == "expansion.json":
            new_raw, ch, sk = _edit_json_file(raw, "expansion.json", args)
            all_changed.extend([f"  CHANGED  {x}" for x in ch])
            all_skipped.extend([f"  skipped  {x}" for x in sk])
            new_member_bytes[member_name] = new_raw

        # bundle_manifest.json
        elif base == "bundle_manifest.json":
            new_raw, ch, sk = _edit_json_file(raw, "bundle_manifest.json", args)
            all_changed.extend([f"  CHANGED  {x}" for x in ch])
            all_skipped.extend([f"  skipped  {x}" for x in sk])
            new_member_bytes[member_name] = new_raw

        # XPM program files
        elif base.endswith(".xpm"):
            new_raw, ch, sk = _edit_xpm(raw, member_name, args)
            all_changed.extend([f"  CHANGED  {x}" for x in ch])
            all_skipped.extend([f"  skipped  {x}" for x in sk])
            new_member_bytes[member_name] = new_raw

        else:
            new_member_bytes[member_name] = raw  # unchanged

    # Report
    if all_changed:
        for line in all_changed:
            print(line)
    else:
        print("  (no changes)")
    for line in all_skipped:
        print(line)

    if dry_run or not all_changed:
        return bool(all_changed)

    # Write output
    buf = io.BytesIO()
    with zipfile.ZipFile(buf, "w", zipfile.ZIP_DEFLATED) as zout:
        for member_name in members:  # preserve original order
            info = member_info[member_name]
            zout.writestr(info, new_member_bytes[member_name])

    dest_path.parent.mkdir(parents=True, exist_ok=True)

    # Backup if overwriting in place
    if dest_path == src_path and dest_path.exists():
        bak = dest_path.with_suffix(".xpn.bak")
        shutil.copy2(dest_path, bak)
        print(f"  backup → {bak.name}")

    dest_path.write_bytes(buf.getvalue())
    print(f"  wrote  → {dest_path}")
    return True


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def _parse_dna(raw: str) -> dict:
    """Parse 'brightness=0.7,warmth=0.4' into {brightness: 0.7, warmth: 0.4}."""
    result = {}
    for pair in raw.split(","):
        pair = pair.strip()
        if "=" not in pair:
            sys.exit(f"ERROR: --set-dna value '{pair}' must be key=float")
        k, _, v = pair.partition("=")
        try:
            result[k.strip()] = float(v.strip())
        except ValueError:
            sys.exit(f"ERROR: --set-dna value for '{k}' must be a float, got '{v}'")
    return result


def _build_parser() -> argparse.ArgumentParser:
    p = argparse.ArgumentParser(
        description="Bulk-edit metadata in .xpn expansion archives.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )

    # Source
    src = p.add_mutually_exclusive_group(required=True)
    src.add_argument("--xpn", metavar="FILE", help="Single .xpn file to edit")
    src.add_argument("--xpns-dir", metavar="DIR", help="Directory of .xpn files to edit")

    # Edit fields
    p.add_argument("--version", metavar="VERSION", help="New version string")
    p.add_argument("--author", metavar="AUTHOR", help="New author string")
    p.add_argument("--mood", metavar="MOOD", choices=sorted(KNOWN_MOODS),
                   help="New mood (Foundation|Atmosphere|Entangled|Prism|Flux|Aether|Family)")
    p.add_argument("--preset-count", metavar="N", type=int,
                   help="New preset_count integer")
    p.add_argument("--set-dna", metavar="k=v,...",
                   help="Update sonic_dna fields (e.g. brightness=0.7,warmth=0.4)")
    p.add_argument("--add-tags", metavar="tag1,tag2",
                   help="Comma-separated tags to append")
    p.add_argument("--set-status", metavar="STATUS", choices=sorted(KNOWN_STATUSES),
                   help="Set status field (planned|beta|released)")

    # IO
    p.add_argument("--output-dir", metavar="DIR",
                   help="Write modified packs here (default: overwrite in place)")
    p.add_argument("--dry-run", action="store_true",
                   help="Show what would change without writing")

    return p


def main():
    parser = _build_parser()
    args = parser.parse_args()

    # No edit flags at all → warn
    edit_flags = [args.version, args.author, args.mood, args.preset_count,
                  args.set_dna, args.add_tags, args.set_status]
    if not any(f is not None for f in edit_flags):
        parser.error("No edit flags specified. Provide at least one of: "
                     "--version, --author, --mood, --preset-count, "
                     "--set-dna, --add-tags, --set-status")

    # Normalise --set-dna
    if args.set_dna:
        args.set_dna = _parse_dna(args.set_dna)

    # Normalise --add-tags
    if args.add_tags:
        args.add_tags = [t.strip() for t in args.add_tags.split(",") if t.strip()]

    # Collect source paths
    if args.xpn:
        src_path = Path(args.xpn)
        if not src_path.is_file():
            sys.exit(f"ERROR: {src_path} not found")
        sources = [src_path]
    else:
        xpns_dir = Path(args.xpns_dir)
        if not xpns_dir.is_dir():
            sys.exit(f"ERROR: {xpns_dir} is not a directory")
        sources = sorted(xpns_dir.glob("*.xpn"))
        if not sources:
            sys.exit(f"ERROR: no .xpn files found in {xpns_dir}")

    output_dir = Path(args.output_dir) if args.output_dir else None

    total = modified = 0
    for src in sources:
        total += 1
        if output_dir:
            dest = output_dir / src.name
        else:
            dest = src  # overwrite in place (backup created inside _process_xpn)

        if _process_xpn(src, dest, args, dry_run=args.dry_run):
            modified += 1

    print(f"\nDone. {modified}/{total} pack(s) {'would be ' if args.dry_run else ''}modified.")


if __name__ == "__main__":
    main()
