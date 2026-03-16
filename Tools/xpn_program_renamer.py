"""
xpn_program_renamer.py — Batch-rename XPM programs inside a .xpn ZIP archive.

Usage:
  # List all program names in a .xpn
  python xpn_program_renamer.py --xpn pack.xpn --list

  # Apply renames from a JSON map (dry run)
  python xpn_program_renamer.py --xpn pack.xpn --rename-map renames.json --dry-run

  # Apply renames and write to new file
  python xpn_program_renamer.py --xpn pack.xpn --rename-map renames.json --output renamed.xpn

  # Apply renames in-place (overwrites original)
  python xpn_program_renamer.py --xpn pack.xpn --rename-map renames.json --in-place

JSON rename map format:
  {"Old Program Name": "New Program Name", ...}

Constraints:
  - New names must be <= 20 characters (hard error).
  - Warns if name starts with a digit or has "Kit", "Pad", or "Preset" as the first word.
  - Also updates <PadLabel> elements whose text matches the old program name.
"""

import argparse
import json
import re
import shutil
import sys
import tempfile
import zipfile
from pathlib import Path
from typing import Optional

MAX_NAME_LEN = 20
WARN_FIRST_WORDS = {"kit", "pad", "preset"}


def warn(msg: str) -> None:
    print(f"  WARNING: {msg}", file=sys.stderr)


def error(msg: str) -> None:
    print(f"  ERROR: {msg}", file=sys.stderr)


def validate_new_name(new_name: str) -> bool:
    """Validate new program name. Returns True if valid, False on hard error."""
    if len(new_name) > MAX_NAME_LEN:
        error(f"'{new_name}' exceeds {MAX_NAME_LEN} chars ({len(new_name)}). Aborting.")
        return False
    if new_name and new_name[0].isdigit():
        warn(f"'{new_name}' starts with a digit — MPC may reject or misparse this name.")
    first_word = new_name.split()[0].lower() if new_name.split() else ""
    if first_word in WARN_FIRST_WORDS:
        warn(f"'{new_name}' starts with '{new_name.split()[0]}' — generic first word, consider a more distinctive name.")
    return True


def get_xpm_paths(zf: zipfile.ZipFile) -> "list[str]":
    """Return all .xpm member paths inside the ZIP."""
    return [name for name in zf.namelist() if name.lower().endswith(".xpm")]


def extract_program_name(xml_bytes: bytes) -> Optional[str]:
    """Extract the Program name attribute from XPM XML bytes."""
    # Use regex for speed and to preserve XML exactly as-is during list mode.
    match = re.search(rb'<Program\s[^>]*\bname="([^"]*)"', xml_bytes)
    if match:
        return match.group(1).decode("utf-8", errors="replace")
    return None


def apply_renames_to_xml(xml_bytes: bytes, old_name: str, new_name: str) -> "tuple[bytes, int]":
    """
    Apply a single rename (old_name -> new_name) to XPM XML bytes.
    Updates <Program name="..."> and any <PadLabel> text matching old_name.
    Returns (modified_bytes, change_count).
    """
    changes = 0

    # Parse preserving the byte string as-is via re for the Program name attribute.
    # We use regex replacement to avoid ElementTree reformatting the whole document.

    old_encoded = old_name.encode("utf-8")
    new_encoded = new_name.encode("utf-8")

    # 1. Replace <Program ... name="OldName" ...> (first occurrence only)
    pattern_prog = re.compile(
        rb'(<Program\s[^>]*\bname=")' + re.escape(old_encoded) + rb'"',
        re.DOTALL,
    )
    new_xml, n = pattern_prog.subn(rb'\1' + new_encoded + rb'"', xml_bytes, count=1)
    changes += n
    xml_bytes = new_xml

    # 2. Replace <PadLabel> elements whose full text matches old_name exactly.
    pattern_pad = re.compile(
        rb'(<PadLabel[^>]*>)' + re.escape(old_encoded) + rb'(</PadLabel>)',
        re.DOTALL,
    )
    new_xml, n = pattern_pad.subn(rb'\1' + new_encoded + rb'\2', xml_bytes)
    changes += n
    xml_bytes = new_xml

    return xml_bytes, changes


def cmd_list(xpn_path: Path) -> None:
    """--list mode: print all XPM program names."""
    with zipfile.ZipFile(xpn_path, "r") as zf:
        xpm_paths = get_xpm_paths(zf)
        if not xpm_paths:
            print("No .xpm files found in archive.")
            return
        print(f"Found {len(xpm_paths)} .xpm file(s) in {xpn_path.name}:\n")
        for xpm_path in sorted(xpm_paths):
            xml_bytes = zf.read(xpm_path)
            name = extract_program_name(xml_bytes)
            name_display = f'"{name}"' if name else "(name not found)"
            char_note = f"  [{len(name)}/{MAX_NAME_LEN} chars]" if name else ""
            print(f"  {xpm_path}")
            print(f"    Program name: {name_display}{char_note}")


def cmd_rename(
    xpn_path: Path,
    rename_map: dict[str, str],
    output_path: Optional[Path],
    in_place: bool,
    dry_run: bool,
) -> None:
    """--rename-map mode: apply renames and write output."""

    # Validate all new names up front — hard fail before touching files.
    print("Validating rename map...")
    for old_name, new_name in rename_map.items():
        print(f"  '{old_name}' -> '{new_name}'")
        if not validate_new_name(new_name):
            sys.exit(1)

    if dry_run:
        print("\n[DRY RUN] No files will be written.\n")

    # Determine output destination.
    if in_place:
        dest_path = xpn_path
    elif output_path:
        dest_path = output_path
    else:
        dest_path = xpn_path.with_stem(xpn_path.stem + "_renamed")

    total_xpm_changes = 0
    total_pad_changes = 0
    matched_xpms: list[str] = []

    with zipfile.ZipFile(xpn_path, "r") as zf_in:
        xpm_paths = get_xpm_paths(zf_in)
        all_members = zf_in.namelist()

        print(f"\nScanning {len(xpm_paths)} .xpm file(s)...\n")

        # Build a dict of member -> modified bytes for xpm files that need changes.
        modified: dict[str, bytes] = {}

        for xpm_path in xpm_paths:
            xml_bytes = zf_in.read(xpm_path)
            current_name = extract_program_name(xml_bytes)

            if current_name in rename_map:
                new_name = rename_map[current_name]
                updated_bytes, changes = apply_renames_to_xml(xml_bytes, current_name, new_name)

                prog_changes = changes - (changes - 1) if changes > 0 else 0  # at most 1 Program tag
                # Re-count pad changes separately for reporting.
                _, pad_n = apply_renames_to_xml(xml_bytes, current_name, new_name)
                # pad changes = total changes minus 1 (the Program tag itself)
                pad_changes = max(0, changes - 1)

                print(f"  {xpm_path}")
                print(f"    '{current_name}' -> '{new_name}'")
                print(f"    Program tag updated: {'yes' if changes > 0 else 'no'}")
                print(f"    PadLabel matches updated: {pad_changes}")

                modified[xpm_path] = updated_bytes
                matched_xpms.append(xpm_path)
                total_xpm_changes += 1
                total_pad_changes += pad_changes

        # Report any rename map keys that had no match.
        matched_names = set()
        for xpm_path in xpm_paths:
            name = extract_program_name(zf_in.read(xpm_path))
            if name in rename_map:
                matched_names.add(name)
        for old_name in rename_map:
            if old_name not in matched_names:
                warn(f"Rename key '{old_name}' did not match any program in the archive.")

        if not modified:
            print("\nNo matching programs found. Nothing to write.")
            return

        print(f"\nSummary: {total_xpm_changes} XPM(s) renamed, {total_pad_changes} PadLabel(s) updated.")

        if dry_run:
            print("[DRY RUN] Skipping write.")
            return

        # Write new ZIP, replacing modified members.
        tmp_fd, tmp_path_str = tempfile.mkstemp(suffix=".xpn")
        tmp_path = Path(tmp_path_str)
        try:
            with zipfile.ZipFile(xpn_path, "r") as zf_in, \
                 zipfile.ZipFile(tmp_path, "w", compression=zipfile.ZIP_DEFLATED) as zf_out:
                for member in all_members:
                    info = zf_in.getinfo(member)
                    if member in modified:
                        zf_out.writestr(info, modified[member])
                    else:
                        zf_out.writestr(info, zf_in.read(member))
            import os
            os.close(tmp_fd)
            shutil.move(str(tmp_path), str(dest_path))
            print(f"\nWritten to: {dest_path}")
        except Exception:
            import os
            try:
                os.close(tmp_fd)
            except Exception:
                pass
            tmp_path.unlink(missing_ok=True)
            raise


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Batch-rename XPM program names inside a .xpn ZIP archive.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    parser.add_argument("--xpn", required=True, metavar="FILE", help="Path to the .xpn ZIP file.")
    parser.add_argument("--list", action="store_true", help="List all program names and exit.")
    parser.add_argument("--rename-map", metavar="JSON", help="Path to rename map JSON file.")
    parser.add_argument("--output", metavar="FILE", help="Output .xpn path (default: <name>_renamed.xpn).")
    parser.add_argument("--in-place", action="store_true", help="Overwrite the input .xpn file.")
    parser.add_argument("--dry-run", action="store_true", help="Show changes without writing.")
    args = parser.parse_args()

    xpn_path = Path(args.xpn)
    if not xpn_path.exists():
        print(f"ERROR: File not found: {xpn_path}", file=sys.stderr)
        sys.exit(1)

    if args.list:
        cmd_list(xpn_path)
        return

    if args.rename_map:
        map_path = Path(args.rename_map)
        if not map_path.exists():
            print(f"ERROR: Rename map not found: {map_path}", file=sys.stderr)
            sys.exit(1)
        with open(map_path, "r", encoding="utf-8") as f:
            rename_map: dict[str, str] = json.load(f)
        if not isinstance(rename_map, dict):
            print("ERROR: Rename map must be a JSON object.", file=sys.stderr)
            sys.exit(1)

        output_path = Path(args.output) if args.output else None

        if args.in_place and output_path:
            print("ERROR: --in-place and --output are mutually exclusive.", file=sys.stderr)
            sys.exit(1)

        cmd_rename(
            xpn_path=xpn_path,
            rename_map=rename_map,
            output_path=output_path,
            in_place=args.in_place,
            dry_run=args.dry_run,
        )
        return

    parser.print_help()


if __name__ == "__main__":
    main()
