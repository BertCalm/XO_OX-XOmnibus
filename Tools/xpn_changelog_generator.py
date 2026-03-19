#!/usr/bin/env python3
"""
XPN Changelog Generator — XO_OX Designs
Generates a CHANGELOG.md for an XPN expansion pack.

Two modes:
  --diff-mode  --old pack_v1.0.xpn --new pack_v1.1.xpn
      Diffs the two packs, auto-detects version bump type, and generates
      a new changelog entry for the new version.

  --init-mode  --xpn pack_v1.0.xpn
      Generates the initial v1.0.0 entry listing all programs and sample count.

Version bump rules (auto-detected from diff):
  PATCH  — only sample changes, no program additions/removals/renames
  MINOR  — programs added, none removed (net growth)
  MAJOR  — programs removed or renamed (destructive or structural change)

Output:
  Writes to --output path (default: stdout).
  With --append: if CHANGELOG.md exists, the new entry is prepended.

Usage:
    python xpn_changelog_generator.py --diff-mode --old v1.0.xpn --new v1.1.xpn
    python xpn_changelog_generator.py --diff-mode --old v1.0.xpn --new v1.1.xpn --output CHANGELOG.md --append
    python xpn_changelog_generator.py --init-mode --xpn pack_v1.0.xpn --output CHANGELOG.md
"""

import argparse
import json
import os
import re
import sys
import zipfile
import hashlib
from datetime import date
from difflib import SequenceMatcher


# ---------------------------------------------------------------------------
# Helpers (self-contained; mirrors xpn_pack_diff.py logic, no import needed)
# ---------------------------------------------------------------------------

def _sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def _similarity(a: str, b: str) -> float:
    return SequenceMatcher(None, a.lower(), b.lower()).ratio()


def _list_programs(zf: zipfile.ZipFile) -> dict:
    """Return {basename: zippath} for all .xpm files."""
    result = {}
    for name in zf.namelist():
        if name.lower().endswith('.xpm') and not name.startswith('__'):
            base = name.split('/')[-1]
            result[base] = name
    return result


def _list_samples(zf: zipfile.ZipFile) -> dict:
    """Return {basename: zippath} for all audio sample files."""
    EXTS = {'.wav', '.aif', '.aiff', '.flac', '.mp3', '.ogg'}
    result = {}
    for name in zf.namelist():
        ext = ('.' + name.rsplit('.', 1)[-1].lower()) if '.' in name else ''
        if ext in EXTS and not name.startswith('__'):
            base = name.split('/')[-1]
            result[base] = name
    return result


def _load_manifest(zf: zipfile.ZipFile) -> dict:
    for name in zf.namelist():
        if name.lower().endswith('bundle_manifest.json'):
            try:
                return json.loads(zf.read(name).decode('utf-8', errors='replace'))
            except Exception:
                return {}
    return {}


def _pack_name(zf: zipfile.ZipFile, fallback_path: str) -> str:
    """Derive a human-readable pack name from manifest or filename."""
    m = _load_manifest(zf)
    name = m.get('name') or m.get('pack_name') or m.get('title')
    if name:
        return str(name)
    # Fall back to filename stem, strip version suffix
    stem = os.path.splitext(os.path.basename(fallback_path))[0]
    stem = re.sub(r'[_\-]?v?\d+[\.\d]*$', '', stem, flags=re.IGNORECASE)
    return stem.replace('_', ' ').replace('-', ' ').strip()


def _manifest_version(zf: zipfile.ZipFile):
    m = _load_manifest(zf)
    return m.get('version') or m.get('pack_version')


# ---------------------------------------------------------------------------
# Diff logic (programs + samples only — all we need for changelog)
# ---------------------------------------------------------------------------

def _diff_programs(old_zf, new_zf):
    old_progs = _list_programs(old_zf)
    new_progs = _list_programs(new_zf)
    old_names = set(old_progs)
    new_names = set(new_progs)

    removed_raw = old_names - new_names
    added_raw = new_names - old_names

    RENAME_THRESHOLD = 0.65
    renames = []
    unmatched_removed = set(removed_raw)
    unmatched_added = set(added_raw)

    for old_n in list(unmatched_removed):
        best_score, best_new = 0.0, None
        for new_n in unmatched_added:
            s = _similarity(old_n, new_n)
            if s > best_score:
                best_score, best_new = s, new_n
        if best_new and best_score >= RENAME_THRESHOLD:
            renames.append((old_n, best_new))
            unmatched_removed.discard(old_n)
            unmatched_added.discard(best_new)

    return {
        'added': sorted(unmatched_added),
        'removed': sorted(unmatched_removed),
        'renamed': renames,
        'common': sorted(old_names & new_names),
    }


def _diff_samples(old_zf, new_zf):
    old_s = _list_samples(old_zf)
    new_s = _list_samples(new_zf)
    old_names = set(old_s)
    new_names = set(new_s)

    added = sorted(new_names - old_names)
    removed = sorted(old_names - new_names)
    changed = sorted(
        name for name in old_names & new_names
        if _sha256(old_zf.read(old_s[name])) != _sha256(new_zf.read(new_s[name]))
    )
    return {'added': added, 'removed': removed, 'changed': changed}


# ---------------------------------------------------------------------------
# Version bump detection
# ---------------------------------------------------------------------------

def _detect_bump(prog_diff, samp_diff) -> str:
    """Return 'MAJOR', 'MINOR', or 'PATCH'."""
    if prog_diff['removed'] or prog_diff['renamed']:
        return 'MAJOR'
    if prog_diff['added']:
        return 'MINOR'
    return 'PATCH'


def _bump_version(version_str: str, bump: str) -> str:
    """Increment a semver string by bump type. Falls back to 1.0.0 on parse failure."""
    if not version_str:
        defaults = {'MAJOR': '2.0.0', 'MINOR': '1.1.0', 'PATCH': '1.0.1'}
        return defaults[bump]
    parts = re.split(r'[\.\-]', str(version_str).lstrip('v'))
    try:
        major, minor, patch = int(parts[0]), int(parts[1]) if len(parts) > 1 else 0, int(parts[2]) if len(parts) > 2 else 0
    except (ValueError, IndexError):
        return '1.0.0'
    if bump == 'MAJOR':
        return f'{major + 1}.0.0'
    if bump == 'MINOR':
        return f'{major}.{minor + 1}.0'
    return f'{major}.{minor}.{patch + 1}'


# ---------------------------------------------------------------------------
# Changelog entry formatters
# ---------------------------------------------------------------------------

def _strip_xpm_ext(name: str) -> str:
    return name[:-4] if name.lower().endswith('.xpm') else name


def _format_diff_entry(pack_name: str, new_version: str, prog_diff, samp_diff) -> str:
    today = date.today().isoformat()
    lines = [f'## v{new_version} — {today}']

    # ### Added
    added_lines = []
    if prog_diff['added']:
        names = ', '.join(_strip_xpm_ext(p) for p in prog_diff['added'])
        count = len(prog_diff['added'])
        added_lines.append(f'- {count} new program{"s" if count > 1 else ""}: {names}')
    if samp_diff['added']:
        added_lines.append(f'- {len(samp_diff["added"])} new sample{"s" if len(samp_diff["added"]) > 1 else ""} added')
    if added_lines:
        lines.append('### Added')
        lines.extend(added_lines)

    # ### Removed
    removed_lines = []
    if prog_diff['removed']:
        names = ', '.join(_strip_xpm_ext(p) for p in prog_diff['removed'])
        count = len(prog_diff['removed'])
        removed_lines.append(f'- {count} program{"s" if count > 1 else ""} removed: {names}')
    if samp_diff['removed']:
        removed_lines.append(f'- {len(samp_diff["removed"])} sample{"s" if len(samp_diff["removed"]) > 1 else ""} removed')
    if removed_lines:
        lines.append('### Removed')
        lines.extend(removed_lines)

    # ### Changed
    changed_lines = []
    if prog_diff['renamed']:
        for old_n, new_n in prog_diff['renamed']:
            changed_lines.append(f'- Program renamed: {_strip_xpm_ext(old_n)} → {_strip_xpm_ext(new_n)}')
    if samp_diff['changed']:
        count = len(samp_diff['changed'])
        changed_lines.append(f'- {count} sample{"s" if count > 1 else ""} re-rendered / updated')
    if changed_lines:
        lines.append('### Changed')
        lines.extend(changed_lines)

    # If nothing at all changed, note it
    any_change = added_lines or removed_lines or changed_lines
    if not any_change:
        lines.append('- No content changes detected (metadata or manifest update only)')

    return '\n'.join(lines)


def _format_init_entry(pack_name: str, xpn_path: str) -> str:
    today = date.today().isoformat()
    with zipfile.ZipFile(xpn_path, 'r') as zf:
        progs = _list_programs(zf)
        samps = _list_samples(zf)
        version = _manifest_version(zf) or '1.0.0'

    prog_count = len(progs)
    samp_count = len(samps)

    lines = [
        f'## v{version} — {today}',
        f'- Initial release: {prog_count} program{"s" if prog_count != 1 else ""}, {samp_count} sample{"s" if samp_count != 1 else ""}',
    ]
    return '\n'.join(lines)


def _build_full_changelog(pack_name: str, entry: str, existing_path=None) -> str:
    header = f'# {pack_name} — Changelog\n'
    new_block = entry + '\n'

    if existing_path and os.path.isfile(existing_path):
        with open(existing_path, 'r', encoding='utf-8') as f:
            existing = f.read()
        # Strip the existing header line if present (we'll re-add ours)
        existing_body = re.sub(r'^# .+?— Changelog\n+', '', existing, count=1)
        return header + '\n' + new_block + '\n' + existing_body.lstrip('\n')

    return header + '\n' + new_block


# ---------------------------------------------------------------------------
# Entry point
# ---------------------------------------------------------------------------

def main():
    parser = argparse.ArgumentParser(
        description='Generate a CHANGELOG.md for an XPN expansion pack.',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    mode = parser.add_mutually_exclusive_group(required=True)
    mode.add_argument('--diff-mode', action='store_true',
                      help='Diff two pack versions and generate a new changelog entry')
    mode.add_argument('--init-mode', action='store_true',
                      help='Generate initial changelog entry from a single pack')

    parser.add_argument('--old', help='Path to older .xpn file (--diff-mode)')
    parser.add_argument('--new', help='Path to newer .xpn file (--diff-mode)')
    parser.add_argument('--xpn', help='Path to .xpn file (--init-mode)')
    parser.add_argument('--output', default=None,
                        help='Output path for CHANGELOG.md (default: stdout)')
    parser.add_argument('--append', action='store_true',
                        help='Prepend new entry if --output file already exists')

    args = parser.parse_args()

    # Validate arguments
    if args.diff_mode:
        if not args.old or not args.new:
            parser.error('--diff-mode requires --old and --new')
        for label, path in (('--old', args.old), ('--new', args.new)):
            if not os.path.isfile(path):
                print(f'ERROR: File not found: {path} ({label})', file=sys.stderr)
                sys.exit(1)
            if not zipfile.is_zipfile(path):
                print(f'ERROR: Not a valid ZIP/XPN file: {path} ({label})', file=sys.stderr)
                sys.exit(1)
    else:  # init-mode
        if not args.xpn:
            parser.error('--init-mode requires --xpn')
        if not os.path.isfile(args.xpn):
            print(f'ERROR: File not found: {args.xpn}', file=sys.stderr)
            sys.exit(1)
        if not zipfile.is_zipfile(args.xpn):
            print(f'ERROR: Not a valid ZIP/XPN file: {args.xpn}', file=sys.stderr)
            sys.exit(1)

    # Generate entry
    if args.diff_mode:
        with zipfile.ZipFile(args.old, 'r') as old_zf, \
             zipfile.ZipFile(args.new, 'r') as new_zf:
            pack_name = _pack_name(new_zf, args.new)
            prog_diff = _diff_programs(old_zf, new_zf)
            samp_diff = _diff_samples(old_zf, new_zf)
            old_version = _manifest_version(old_zf)
            bump = _detect_bump(prog_diff, samp_diff)
            new_version = _bump_version(old_version, bump)

        print(f'Detected bump type: {bump}  ({old_version or "unknown"} → {new_version})',
              file=sys.stderr)
        entry = _format_diff_entry(pack_name, new_version, prog_diff, samp_diff)
    else:
        with zipfile.ZipFile(args.xpn, 'r') as zf:
            pack_name = _pack_name(zf, args.xpn)
        entry = _format_init_entry(pack_name, args.xpn)

    existing_path = args.output if args.append else None
    changelog = _build_full_changelog(pack_name, entry, existing_path)

    if args.output:
        with open(args.output, 'w', encoding='utf-8') as f:
            f.write(changelog)
        print(f'Wrote changelog to {args.output}', file=sys.stderr)
    else:
        print(changelog)


if __name__ == '__main__':
    main()
