#!/usr/bin/env python3
"""
XPN Pack Diff — XO_OX Designs
Diffs two versions of an .xpn expansion pack (v1.0 vs v1.1) for changelogs and QA.

Compares:
  - Programs (XPM files): added, removed, renamed (by similar name)
  - Per-program ProgramParameters: numeric value changes (old→new)
  - Samples: added, removed, changed (same filename, different SHA-256)
  - Manifest (bundle_manifest.json): version bump, description, sonic_dna shifts

Usage:
    python xpn_pack_diff.py --old OBESEv1.0.xpn --new OBESEv1.1.xpn
    python xpn_pack_diff.py --old OBESEv1.0.xpn --new OBESEv1.1.xpn --format json
"""

import argparse
import hashlib
import json
import re
import sys
import zipfile
from difflib import SequenceMatcher
from xml.etree import ElementTree as ET


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def _sha256_of_bytes(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def _similarity(a: str, b: str) -> float:
    return SequenceMatcher(None, a.lower(), b.lower()).ratio()


def _list_programs(zf: zipfile.ZipFile) -> dict[str, str]:
    """Return {basename: zippath} for all .xpm files in the archive."""
    result = {}
    for name in zf.namelist():
        if name.lower().endswith('.xpm') and not name.startswith('__'):
            base = name.split('/')[-1]
            result[base] = name
    return result


def _list_samples(zf: zipfile.ZipFile) -> dict[str, str]:
    """Return {basename: zippath} for all sample files (wav/aif/aiff/flac)."""
    EXTS = {'.wav', '.aif', '.aiff', '.flac', '.mp3', '.ogg'}
    result = {}
    for name in zf.namelist():
        ext = '.' + name.rsplit('.', 1)[-1].lower() if '.' in name else ''
        if ext in EXTS and not name.startswith('__'):
            base = name.split('/')[-1]
            result[base] = name
    return result


def _parse_program_params(zf: zipfile.ZipFile, zippath: str) -> dict[str, float]:
    """Parse <ProgramParameters> block from an XPM file, return {name: float}."""
    try:
        data = zf.read(zippath).decode('utf-8', errors='replace')
        root = ET.fromstring(data)
    except Exception:
        return {}

    params = {}
    for pp in root.iter('ProgramParameters'):
        for child in pp:
            tag = child.tag.strip()
            text = (child.text or '').strip()
            try:
                params[tag] = float(text)
            except ValueError:
                pass  # skip non-numeric params
    return params


def _load_manifest(zf: zipfile.ZipFile) -> dict:
    """Load bundle_manifest.json if present, else return empty dict."""
    for name in zf.namelist():
        if name.lower().endswith('bundle_manifest.json'):
            try:
                return json.loads(zf.read(name).decode('utf-8', errors='replace'))
            except Exception:
                return {}
    return {}


# ---------------------------------------------------------------------------
# Core diff logic
# ---------------------------------------------------------------------------

def diff_programs(old_zf, new_zf):
    """Diff XPM programs: added, removed, renamed, param changes."""
    old_progs = _list_programs(old_zf)
    new_progs = _list_programs(new_zf)

    old_names = set(old_progs)
    new_names = set(new_progs)

    removed_raw = old_names - new_names
    added_raw = new_names - old_names

    # Detect renames: best similarity match above threshold
    RENAME_THRESHOLD = 0.65
    renames = []  # list of (old_name, new_name, similarity)
    unmatched_removed = set(removed_raw)
    unmatched_added = set(added_raw)

    for old_n in list(unmatched_removed):
        best_score = 0.0
        best_new = None
        for new_n in unmatched_added:
            s = _similarity(old_n, new_n)
            if s > best_score:
                best_score = s
                best_new = new_n
        if best_new and best_score >= RENAME_THRESHOLD:
            renames.append((old_n, best_new, round(best_score, 2)))
            unmatched_removed.discard(old_n)
            unmatched_added.discard(best_new)

    # Param diffs for programs present in both (including rename pairs)
    param_changes = {}  # {program_name: {param: (old_val, new_val)}}

    # Same-name programs
    common = old_names & new_names
    for prog in common:
        old_params = _parse_program_params(old_zf, old_progs[prog])
        new_params = _parse_program_params(new_zf, new_progs[prog])
        changes = {}
        all_keys = set(old_params) | set(new_params)
        for k in all_keys:
            ov = old_params.get(k)
            nv = new_params.get(k)
            if ov != nv:
                changes[k] = (ov, nv)
        if changes:
            param_changes[prog] = changes

    # Renamed programs — diff their params too
    for old_n, new_n, _ in renames:
        old_params = _parse_program_params(old_zf, old_progs[old_n])
        new_params = _parse_program_params(new_zf, new_progs[new_n])
        changes = {}
        all_keys = set(old_params) | set(new_params)
        for k in all_keys:
            ov = old_params.get(k)
            nv = new_params.get(k)
            if ov != nv:
                changes[k] = (ov, nv)
        if changes:
            param_changes[f"{old_n} → {new_n}"] = changes

    return {
        'added': sorted(unmatched_added),
        'removed': sorted(unmatched_removed),
        'renamed': renames,
        'param_changes': param_changes,
    }


def diff_samples(old_zf, new_zf):
    """Diff sample files: added, removed, changed (hash mismatch)."""
    old_samps = _list_samples(old_zf)
    new_samps = _list_samples(new_zf)

    old_names = set(old_samps)
    new_names = set(new_samps)

    added = sorted(new_names - old_names)
    removed = sorted(old_names - new_names)

    changed = []
    for name in old_names & new_names:
        old_hash = _sha256_of_bytes(old_zf.read(old_samps[name]))
        new_hash = _sha256_of_bytes(new_zf.read(new_samps[name]))
        if old_hash != new_hash:
            changed.append(name)

    return {'added': added, 'removed': removed, 'changed': sorted(changed)}


def diff_manifest(old_zf, new_zf):
    """Diff bundle_manifest.json: version, description, sonic_dna."""
    old_m = _load_manifest(old_zf)
    new_m = _load_manifest(new_zf)

    result = {}

    # Version
    old_ver = old_m.get('version', old_m.get('pack_version', None))
    new_ver = new_m.get('version', new_m.get('pack_version', None))
    if old_ver != new_ver:
        result['version'] = (old_ver, new_ver)

    # Description
    old_desc = old_m.get('description', None)
    new_desc = new_m.get('description', None)
    if old_desc != new_desc:
        result['description'] = (old_desc, new_desc)

    # Sonic DNA shifts (any numeric key in sonic_dna dict)
    old_dna = old_m.get('sonic_dna', {})
    new_dna = new_m.get('sonic_dna', {})
    if old_dna or new_dna:
        dna_changes = {}
        all_dims = set(old_dna) | set(new_dna)
        for dim in sorted(all_dims):
            ov = old_dna.get(dim)
            nv = new_dna.get(dim)
            if ov != nv:
                dna_changes[dim] = (ov, nv)
        if dna_changes:
            result['sonic_dna'] = dna_changes

    return result


# ---------------------------------------------------------------------------
# Output formatters
# ---------------------------------------------------------------------------

def _fmt_val(v):
    if v is None:
        return '(missing)'
    if isinstance(v, float) and v == int(v):
        return str(int(v))
    return str(v)


def format_text(prog_diff, samp_diff, mani_diff, old_path, new_path):
    lines = []
    lines.append(f"XPN PACK DIFF")
    lines.append(f"  OLD: {old_path}")
    lines.append(f"  NEW: {new_path}")
    lines.append("")

    # Manifest
    lines.append("─── MANIFEST ───────────────────────────────────────────")
    if not mani_diff:
        lines.append("  No manifest changes.")
    else:
        if 'version' in mani_diff:
            ov, nv = mani_diff['version']
            lines.append(f"  version:     {_fmt_val(ov)}  →  {_fmt_val(nv)}")
        if 'description' in mani_diff:
            ov, nv = mani_diff['description']
            lines.append(f"  description: {_fmt_val(ov)}")
            lines.append(f"           →   {_fmt_val(nv)}")
        if 'sonic_dna' in mani_diff:
            lines.append("  sonic_dna changes:")
            for dim, (ov, nv) in mani_diff['sonic_dna'].items():
                lines.append(f"    {dim}: {_fmt_val(ov)} → {_fmt_val(nv)}")
    lines.append("")

    # Programs
    lines.append("─── PROGRAMS ───────────────────────────────────────────")
    if prog_diff['added']:
        lines.append(f"  Added ({len(prog_diff['added'])}):")
        for p in prog_diff['added']:
            lines.append(f"    + {p}")
    if prog_diff['removed']:
        lines.append(f"  Removed ({len(prog_diff['removed'])}):")
        for p in prog_diff['removed']:
            lines.append(f"    - {p}")
    if prog_diff['renamed']:
        lines.append(f"  Renamed ({len(prog_diff['renamed'])}):")
        for old_n, new_n, score in prog_diff['renamed']:
            lines.append(f"    ~ {old_n}  →  {new_n}  (similarity {score})")
    if prog_diff['param_changes']:
        lines.append(f"  Parameter changes ({len(prog_diff['param_changes'])} program(s)):")
        for prog, changes in sorted(prog_diff['param_changes'].items()):
            lines.append(f"    [{prog}]")
            for param, (ov, nv) in sorted(changes.items()):
                lines.append(f"      {param}: {_fmt_val(ov)} → {_fmt_val(nv)}")
    if not any([prog_diff['added'], prog_diff['removed'], prog_diff['renamed'], prog_diff['param_changes']]):
        lines.append("  No program changes.")
    lines.append("")

    # Samples
    lines.append("─── SAMPLES ────────────────────────────────────────────")
    if samp_diff['added']:
        lines.append(f"  Added ({len(samp_diff['added'])}):")
        for s in samp_diff['added']:
            lines.append(f"    + {s}")
    if samp_diff['removed']:
        lines.append(f"  Removed ({len(samp_diff['removed'])}):")
        for s in samp_diff['removed']:
            lines.append(f"    - {s}")
    if samp_diff['changed']:
        lines.append(f"  Changed / re-rendered ({len(samp_diff['changed'])}):")
        for s in samp_diff['changed']:
            lines.append(f"    ~ {s}")
    if not any(samp_diff.values()):
        lines.append("  No sample changes.")
    lines.append("")

    # Summary
    n_prog_modified = len(prog_diff['param_changes'])
    n_prog_added = len(prog_diff['added'])
    n_prog_removed = len(prog_diff['removed'])
    n_prog_renamed = len(prog_diff['renamed'])
    n_samp_added = len(samp_diff['added'])
    n_samp_removed = len(samp_diff['removed'])
    n_samp_changed = len(samp_diff['changed'])

    old_ver = mani_diff.get('version', (None, None))[0]
    new_ver = mani_diff.get('version', (None, None))[1]
    ver_str = f", version {_fmt_val(old_ver)}→{_fmt_val(new_ver)}" if 'version' in mani_diff else ""

    parts = []
    if n_prog_modified:  parts.append(f"{n_prog_modified} program(s) modified")
    if n_prog_added:     parts.append(f"{n_prog_added} program(s) added")
    if n_prog_removed:   parts.append(f"{n_prog_removed} program(s) removed")
    if n_prog_renamed:   parts.append(f"{n_prog_renamed} program(s) renamed")
    if n_samp_added:     parts.append(f"{n_samp_added} sample(s) added")
    if n_samp_removed:   parts.append(f"{n_samp_removed} sample(s) removed")
    if n_samp_changed:   parts.append(f"{n_samp_changed} sample(s) changed")
    if not parts:        parts.append("no changes detected")

    lines.append("─── SUMMARY ────────────────────────────────────────────")
    lines.append("  " + ", ".join(parts) + ver_str)

    return "\n".join(lines)


def format_json(prog_diff, samp_diff, mani_diff, old_path, new_path):
    n_prog_modified = len(prog_diff['param_changes'])
    old_ver = mani_diff.get('version', (None, None))[0]
    new_ver = mani_diff.get('version', (None, None))[1]

    out = {
        'old': old_path,
        'new': new_path,
        'manifest': {
            k: {'old': v[0], 'new': v[1]} for k, v in mani_diff.items()
            if k != 'sonic_dna'
        },
        'programs': {
            'added': prog_diff['added'],
            'removed': prog_diff['removed'],
            'renamed': [
                {'old': o, 'new': n, 'similarity': s}
                for o, n, s in prog_diff['renamed']
            ],
            'modified': {
                prog: {
                    param: {'old': ov, 'new': nv}
                    for param, (ov, nv) in changes.items()
                }
                for prog, changes in prog_diff['param_changes'].items()
            },
        },
        'samples': {
            'added': samp_diff['added'],
            'removed': samp_diff['removed'],
            'changed': samp_diff['changed'],
        },
        'summary': {
            'programs_modified': n_prog_modified,
            'programs_added': len(prog_diff['added']),
            'programs_removed': len(prog_diff['removed']),
            'programs_renamed': len(prog_diff['renamed']),
            'samples_added': len(samp_diff['added']),
            'samples_removed': len(samp_diff['removed']),
            'samples_changed': len(samp_diff['changed']),
            'version_old': old_ver,
            'version_new': new_ver,
        },
    }
    # Merge sonic_dna into manifest block if present
    if 'sonic_dna' in mani_diff:
        out['manifest']['sonic_dna'] = {
            dim: {'old': ov, 'new': nv}
            for dim, (ov, nv) in mani_diff['sonic_dna'].items()
        }
    return json.dumps(out, indent=2)


# ---------------------------------------------------------------------------
# Entry point
# ---------------------------------------------------------------------------

def main():
    parser = argparse.ArgumentParser(
        description='Diff two .xpn expansion pack versions.',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    parser.add_argument('--old', required=True, help='Path to older .xpn file')
    parser.add_argument('--new', required=True, help='Path to newer .xpn file')
    parser.add_argument('--format', choices=['text', 'json'], default='text',
                        help='Output format (default: text)')
    args = parser.parse_args()

    for path in (args.old, args.new):
        try:
            if not zipfile.is_zipfile(path):
                print(f"ERROR: {path} is not a valid ZIP/XPN file.", file=sys.stderr)
                sys.exit(1)
        except FileNotFoundError:
            print(f"ERROR: File not found: {path}", file=sys.stderr)
            sys.exit(1)

    with zipfile.ZipFile(args.old, 'r') as old_zf, \
         zipfile.ZipFile(args.new, 'r') as new_zf:

        prog_diff = diff_programs(old_zf, new_zf)
        samp_diff = diff_samples(old_zf, new_zf)
        mani_diff = diff_manifest(old_zf, new_zf)

    if args.format == 'json':
        print(format_json(prog_diff, samp_diff, mani_diff, args.old, args.new))
    else:
        print(format_text(prog_diff, samp_diff, mani_diff, args.old, args.new))


if __name__ == '__main__':
    main()
