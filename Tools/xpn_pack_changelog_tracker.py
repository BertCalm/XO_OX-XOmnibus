#!/usr/bin/env python3
"""
xpn_pack_changelog_tracker.py — XPN Pack Changelog Tracker

Compares two versions of an XPN pack directory (or ZIP) and generates a
structured changelog entry in Keep-a-Changelog format. Prepends to an
existing CHANGELOG.md, or creates an "Initial Release" entry if no
previous version is provided.

Usage:
    python xpn_pack_changelog_tracker.py --pack <dir_or_zip> \
        [--prev-pack <dir_or_zip>] [--version 1.1.0] \
        [--output CHANGELOG.md] [--dry-run]
"""

import argparse
import json
import zipfile
import sys
from datetime import date
from pathlib import Path
from typing import Optional
from xml.etree import ElementTree as ET


# ---------------------------------------------------------------------------
# Pack abstraction — works with both directories and ZIPs
# ---------------------------------------------------------------------------

class PackReader:
    """Unified read interface for a pack directory or ZIP file."""

    def __init__(self, source: Path):
        self.source = source
        self._is_zip = source.suffix.lower() == ".zip"
        self._zip = None
        if self._is_zip:
            if not source.exists():
                raise FileNotFoundError(f"ZIP not found: {source}")
            self._zip = zipfile.ZipFile(source, "r")
        else:
            if not source.is_dir():
                raise NotADirectoryError(f"Pack directory not found: {source}")

    def close(self):
        if self._zip:
            self._zip.close()

    def __enter__(self):
        return self

    def __exit__(self, *_):
        self.close()

    def list_files(self) -> list[str]:
        """Return all file paths relative to pack root (normalised with /)."""
        if self._is_zip:
            return [i.filename for i in self._zip.infolist() if not i.is_dir()]
        return [
            str(p.relative_to(self.source)).replace("\\", "/")
            for p in self.source.rglob("*")
            if p.is_file()
        ]

    def read_text(self, rel_path: str) -> Optional[str]:
        """Return text content of a file, or None if missing."""
        if self._is_zip:
            try:
                return self._zip.read(rel_path).decode("utf-8", errors="replace")
            except KeyError:
                # Try stripping a leading directory component (common in ZIPs)
                parts = rel_path.split("/", 1)
                if len(parts) == 2:
                    try:
                        return self._zip.read(parts[1]).decode("utf-8", errors="replace")
                    except KeyError:
                        pass
                return None
        path = self.source / rel_path
        if path.exists():
            return path.read_text(encoding="utf-8", errors="replace")
        return None


# ---------------------------------------------------------------------------
# Pack metadata extraction
# ---------------------------------------------------------------------------

def parse_expansion_json(reader: PackReader) -> dict:
    """Parse expansion.json from the pack root."""
    text = reader.read_text("expansion.json")
    if not text:
        return {}
    try:
        return json.loads(text)
    except json.JSONDecodeError:
        return {}


def collect_programs(reader: PackReader) -> dict[str, dict]:
    """
    Return {program_name: {file, samples: [...]}} for all XPM programs.
    Samples are the WAV/AIFF paths referenced inside the XPM XML.
    """
    files = reader.list_files()
    xpm_files = [f for f in files if f.lower().endswith(".xpm")]
    programs: dict[str, dict] = {}

    for rel in xpm_files:
        name = Path(rel).stem
        samples = _extract_samples_from_xpm(reader, rel)
        programs[name] = {"file": rel, "samples": samples}

    return programs


def _extract_samples_from_xpm(reader: PackReader, rel_path: str) -> list[str]:
    """Extract SampleFile paths from an XPM XML file."""
    text = reader.read_text(rel_path)
    if not text:
        return []
    try:
        root = ET.fromstring(text)
    except ET.ParseError:
        return []

    samples = []
    for elem in root.iter():
        # MPC XPM format uses SampleFile or SamplePath attributes/elements
        for attr in ("SampleFile", "SamplePath", "File"):
            val = elem.get(attr)
            if val:
                samples.append(val)
        if elem.tag in ("SampleFile", "SamplePath", "File") and elem.text:
            samples.append(elem.text.strip())

    # Deduplicate while preserving order
    seen: set[str] = set()
    unique = []
    for s in samples:
        if s not in seen:
            seen.add(s)
            unique.append(s)
    return unique


def collect_samples(reader: PackReader) -> set[str]:
    """Return a set of all WAV/AIFF/FLAC sample file paths in the pack."""
    audio_exts = {".wav", ".aiff", ".aif", ".flac", ".mp3", ".ogg"}
    return {
        f for f in reader.list_files()
        if Path(f).suffix.lower() in audio_exts
    }


# ---------------------------------------------------------------------------
# Diff computation
# ---------------------------------------------------------------------------

def diff_expansion_json(prev: dict, curr: dict) -> list[str]:
    """Return human-readable lines describing metadata changes."""
    changes = []
    watched_keys = ["name", "version", "description", "tags", "author", "genre"]
    for key in watched_keys:
        pv = prev.get(key)
        cv = curr.get(key)
        if pv != cv:
            if pv is None:
                changes.append(f"expansion.json: `{key}` set to `{cv}`")
            elif cv is None:
                changes.append(f"expansion.json: `{key}` removed (was `{pv}`)")
            else:
                changes.append(f"expansion.json: `{key}` changed from `{pv}` to `{cv}`")
    return changes


def diff_programs(
    prev_progs: dict[str, dict],
    curr_progs: dict[str, dict],
) -> tuple[list[str], list[str], list[str], list[str]]:
    """
    Returns (added, removed, renamed, modified_samples) lists.
    Rename detection: a program is renamed when prev has a removal and curr has
    an addition with an identical sample set.
    """
    prev_names = set(prev_progs)
    curr_names = set(curr_progs)

    raw_added = curr_names - prev_names
    raw_removed = prev_names - curr_names

    # Rename detection — match by sample fingerprint
    rename_pairs: list[tuple[str, str]] = []  # (old_name, new_name)
    prev_by_sig = {
        frozenset(v["samples"]): k
        for k, v in prev_progs.items()
        if k in raw_removed and v["samples"]
    }
    remaining_added = set(raw_added)
    for new_name in list(raw_added):
        sig = frozenset(curr_progs[new_name]["samples"])
        if sig and sig in prev_by_sig:
            old_name = prev_by_sig[sig]
            rename_pairs.append((old_name, new_name))
            raw_removed.discard(old_name)
            remaining_added.discard(new_name)

    renamed = [f"`{old}` → `{new}`" for old, new in rename_pairs]
    added = [f"`{n}`" for n in sorted(remaining_added)]
    removed = [f"`{n}`" for n in sorted(raw_removed)]

    # Detect programs with changed sample sets (excluding renames)
    common = prev_names & curr_names
    modified_samples: list[str] = []
    for name in sorted(common):
        prev_samps = set(prev_progs[name]["samples"])
        curr_samps = set(curr_progs[name]["samples"])
        added_s = curr_samps - prev_samps
        removed_s = prev_samps - curr_samps
        if added_s or removed_s:
            parts = []
            if added_s:
                parts.append(f"+{len(added_s)} sample(s)")
            if removed_s:
                parts.append(f"-{len(removed_s)} sample(s)")
            modified_samples.append(f"`{name}`: {', '.join(parts)}")

    return added, removed, renamed, modified_samples


def diff_loose_samples(
    prev_samples: set[str], curr_samples: set[str]
) -> tuple[list[str], list[str]]:
    """Return (added_samples, removed_samples) for loose audio files."""
    added = sorted(curr_samples - prev_samples)
    removed = sorted(prev_samples - curr_samples)
    return added, removed


# ---------------------------------------------------------------------------
# Semantic version suggestion
# ---------------------------------------------------------------------------

def suggest_version(
    current_version: Optional[str],
    prog_added: list,
    prog_removed: list,
    prog_renamed: list,
    sample_swaps: list,
    meta_changes: list,
) -> str:
    """
    Bump rules:
      major — any program removals
      minor — program additions or renames
      patch — sample swaps, metadata-only changes
    """
    if not current_version:
        return "1.0.0"

    parts = current_version.lstrip("v").split(".")
    try:
        major, minor, patch = int(parts[0]), int(parts[1]), int(parts[2])
    except (IndexError, ValueError):
        return current_version  # Can't parse — return as-is

    if prog_removed:
        return f"{major + 1}.0.0"
    if prog_added or prog_renamed:
        return f"{major}.{minor + 1}.0"
    if sample_swaps or meta_changes:
        return f"{major}.{minor}.{patch + 1}"
    return current_version  # No changes


# ---------------------------------------------------------------------------
# Changelog entry formatting
# ---------------------------------------------------------------------------

def format_section(title: str, items: list[str]) -> str:
    if not items:
        return ""
    lines = [f"### {title}"]
    for item in items:
        lines.append(f"- {item}")
    return "\n".join(lines) + "\n"


def build_changelog_entry(
    version: str,
    entry_date: str,
    added: list[str],
    removed: list[str],
    changed: list[str],
    notes: Optional[list] = None,
) -> str:
    lines = [f"## [{version}] - {entry_date}\n"]

    if notes:
        for note in notes:
            lines.append(note)
        lines.append("")

    sections = []
    if added:
        sections.append(format_section("Added", added))
    if removed:
        sections.append(format_section("Removed", removed))
    if changed:
        sections.append(format_section("Changed", changed))

    if not sections:
        lines.append("_No changes detected._\n")
    else:
        lines.extend(sections)

    return "\n".join(lines)


def build_initial_entry(
    version: str,
    entry_date: str,
    expansion: dict,
    programs: dict[str, dict],
    samples: set[str],
) -> str:
    pack_name = expansion.get("name", "Unknown Pack")
    description = expansion.get("description", "")

    added: list[str] = []
    added.append(f"**{pack_name}** initial release")
    if description:
        added.append(f"Description: {description}")
    added.append(f"{len(programs)} program(s): {', '.join(f'`{n}`' for n in sorted(programs))}")
    added.append(f"{len(samples)} audio sample(s)")

    tags = expansion.get("tags", [])
    if tags:
        added.append(f"Tags: {', '.join(tags)}")

    return build_changelog_entry(version, entry_date, added=added, removed=[], changed=[])


# ---------------------------------------------------------------------------
# CHANGELOG.md read / prepend
# ---------------------------------------------------------------------------

CHANGELOG_HEADER = """\
# Changelog

All notable changes to this pack are documented in this file.
Format follows [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

"""


def prepend_entry(changelog_path: Path, new_entry: str) -> str:
    """Return the full updated changelog text."""
    if changelog_path.exists():
        existing = changelog_path.read_text(encoding="utf-8")
        # Strip existing header block if present so we don't duplicate it
        if existing.startswith("# Changelog"):
            # Find the first ## entry and keep everything from there
            idx = existing.find("\n## ")
            if idx != -1:
                existing_entries = existing[idx + 1:]
            else:
                existing_entries = ""
        else:
            existing_entries = existing
        return CHANGELOG_HEADER + new_entry + "\n---\n\n" + existing_entries.lstrip()
    else:
        return CHANGELOG_HEADER + new_entry


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Generate a Keep-a-Changelog entry for an XPN pack."
    )
    parser.add_argument(
        "--pack", required=True, type=Path,
        help="Current pack directory or ZIP file.",
    )
    parser.add_argument(
        "--prev-pack", type=Path, default=None,
        help="Previous version pack directory or ZIP (omit for initial release).",
    )
    parser.add_argument(
        "--version", type=str, default=None,
        help="Version string for the new entry (e.g. 1.1.0). "
             "Auto-suggested from expansion.json + diff if omitted.",
    )
    parser.add_argument(
        "--output", type=Path, default=None,
        help="Path to CHANGELOG.md. Defaults to <pack>/CHANGELOG.md.",
    )
    parser.add_argument(
        "--dry-run", action="store_true",
        help="Print the new entry to stdout without writing any files.",
    )
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    today = date.today().isoformat()

    # --- Load current pack ---
    with PackReader(args.pack) as curr_reader:
        curr_expansion = parse_expansion_json(curr_reader)
        curr_programs = collect_programs(curr_reader)
        curr_samples = collect_samples(curr_reader)

        # --- Initial release path ---
        if args.prev_pack is None:
            version = args.version or curr_expansion.get("version", "1.0.0")
            entry = build_initial_entry(
                version, today, curr_expansion, curr_programs, curr_samples
            )
            print(f"[Initial release] version {version}, {len(curr_programs)} programs, "
                  f"{len(curr_samples)} samples.")
        else:
            # --- Diff path ---
            with PackReader(args.prev_pack) as prev_reader:
                prev_expansion = parse_expansion_json(prev_reader)
                prev_programs = collect_programs(prev_reader)
                prev_samples = collect_samples(prev_reader)

            meta_changes = diff_expansion_json(prev_expansion, curr_expansion)
            prog_added, prog_removed, prog_renamed, prog_sample_mods = diff_programs(
                prev_programs, curr_programs
            )
            loose_added, loose_removed = diff_loose_samples(prev_samples, curr_samples)

            # Build section lists
            added_items: list[str] = []
            removed_items: list[str] = []
            changed_items: list[str] = []

            if prog_added:
                added_items += [f"Program {p}" for p in prog_added]
            if loose_added:
                added_items += [f"Sample `{Path(s).name}`" for s in loose_added]

            if prog_removed:
                removed_items += [f"Program {p}" for p in prog_removed]
            if loose_removed:
                removed_items += [f"Sample `{Path(s).name}`" for s in loose_removed]

            if prog_renamed:
                changed_items += [f"Program renamed: {r}" for r in prog_renamed]
            if prog_sample_mods:
                changed_items += [f"Program modified: {m}" for m in prog_sample_mods]
            if meta_changes:
                changed_items += meta_changes

            # Version resolution
            prev_version = prev_expansion.get("version") or args.version
            suggested = suggest_version(
                prev_version, prog_added, prog_removed, prog_renamed,
                prog_sample_mods, meta_changes
            )
            version = args.version or curr_expansion.get("version") or suggested
            if version == prev_version and not args.version:
                version = suggested  # Force bump even if expansion.json wasn't updated

            entry = build_changelog_entry(
                version, today,
                added=added_items,
                removed=removed_items,
                changed=changed_items,
            )

            total_changes = len(added_items) + len(removed_items) + len(changed_items)
            print(f"[Diff] {total_changes} change(s) detected → version {version}")

    # --- Output ---
    if args.dry_run:
        print("\n" + "=" * 60)
        print(entry)
        print("=" * 60)
        print("(dry-run: nothing written)")
        return

    # Resolve output path
    if args.output:
        changelog_path = args.output
    elif args.pack.is_dir():
        changelog_path = args.pack / "CHANGELOG.md"
    else:
        changelog_path = args.pack.parent / "CHANGELOG.md"

    full_text = prepend_entry(changelog_path, entry)
    changelog_path.write_text(full_text, encoding="utf-8")
    print(f"Written: {changelog_path}")


if __name__ == "__main__":
    main()
