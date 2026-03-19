#!/usr/bin/env python3
"""
xpn_pack_compare_report.py — XPN Pack Side-by-Side Comparison Report
XO_OX Designs

Generates a structured comparison between two .xpn packs — ideal for v1.0 vs v1.1
or cross-engine comparisons. Reads both archives inline; no external QA files needed.

Usage:
    python xpn_pack_compare_report.py <pack_a.xpn> <pack_b.xpn>
    python xpn_pack_compare_report.py <pack_a.xpn> <pack_b.xpn> --format markdown
    python xpn_pack_compare_report.py <pack_a.xpn> <pack_b.xpn> --format json --output report.json

Compares:
  1. Metadata         — Name, Engine, Version, Mood, Tags
  2. Program counts   — total, by type (drum/melodic)
  3. Sonic DNA        — 6D side-by-side with delta, flagged if delta > 0.2
  4. Sample counts    — file count and total size
  5. Programs         — unique to A, unique to B, common to both
  6. Pack score       — inline composite score for each pack with grade

Exit codes:
    0  Report generated successfully
    1  One or both files could not be opened
"""

import argparse
import json
import sys
import zipfile
from pathlib import Path
from typing import Optional
from xml.etree import ElementTree as ET


# ---------------------------------------------------------------------------
# Constants
# ---------------------------------------------------------------------------

DNA_DIMENSIONS = ['brightness', 'warmth', 'movement', 'density', 'space', 'aggression']
DNA_DELTA_FLAG = 0.2   # highlight deltas above this threshold
BAR_WIDTH = 10         # number of chars in DNA bar

GRADE_TABLE = [(90, 'A'), (80, 'B'), (70, 'C'), (60, 'D'), (0, 'F')]

SAMPLE_EXTS = {'.wav', '.aif', '.aiff', '.flac', '.mp3', '.ogg'}


# ---------------------------------------------------------------------------
# Helpers — bar rendering
# ---------------------------------------------------------------------------

def _dna_bar(value: float) -> str:
    """Render a 0–1 float as a 10-char block bar (e.g. '███████░░░')."""
    value = max(0.0, min(1.0, value))
    filled = round(value * BAR_WIDTH)
    return '\u2588' * filled + '\u2591' * (BAR_WIDTH - filled)


def _delta_str(delta: float) -> str:
    """Format a delta value with explicit sign and flag if large."""
    flag = ' !' if abs(delta) > DNA_DELTA_FLAG else ''
    sign = '+' if delta >= 0 else '-'
    return f'{sign}{abs(delta):.2f}{flag}'


def _grade(score: int) -> str:
    for threshold, letter in GRADE_TABLE:
        if score >= threshold:
            return letter
    return 'F'


def _delta_label(b_val: int, a_val: int) -> str:
    """Return '(+4)' / '(-2)' / '' for integer counts."""
    diff = b_val - a_val
    if diff == 0:
        return ''
    return f' ({diff:+d})'


# ---------------------------------------------------------------------------
# Pack reader
# ---------------------------------------------------------------------------

def _read_pack(xpn_path: str) -> Optional[dict]:
    """
    Open a .xpn archive and extract all comparison data.
    Returns a dict on success, None on open failure.
    """
    try:
        zf = zipfile.ZipFile(xpn_path, 'r')
    except (FileNotFoundError, zipfile.BadZipFile) as exc:
        print(f'ERROR: Cannot open {xpn_path}: {exc}', file=sys.stderr)
        return None

    with zf:
        names = zf.namelist()

        # ── Manifest (expansion.json) ─────────────────────────────────────
        manifest = {}
        manifest_paths = [n for n in names if Path(n).name.lower() == 'expansion.json']
        if manifest_paths:
            try:
                raw = zf.read(manifest_paths[0])
                manifest = json.loads(raw.decode('utf-8', errors='replace'))
            except Exception:
                pass

        # ── Programs (XPM) ───────────────────────────────────────────────
        xpm_paths = [n for n in names if n.lower().endswith('.xpm')]
        programs = []
        drum_count = 0
        melodic_count = 0

        for xpm_path in xpm_paths:
            try:
                xml_data = zf.read(xpm_path)
                root = ET.fromstring(xml_data)
            except Exception:
                continue

            # Program name
            name_el = root.find('.//ProgramName')
            prog_name = ''
            if name_el is not None and name_el.text:
                prog_name = name_el.text.strip()
            if not prog_name:
                prog_name = root.get('name', Path(xpm_path).stem)

            # Type detection: presence of <Pad> elements = drum; <Keygroup> = melodic
            is_drum = bool(root.findall('.//Pad'))
            prog_type = 'drum' if is_drum else 'melodic'
            if is_drum:
                drum_count += 1
            else:
                melodic_count += 1

            programs.append({'name': prog_name, 'type': prog_type, 'path': xpm_path})

        # ── Samples ──────────────────────────────────────────────────────
        sample_paths = [
            n for n in names
            if ('.' + n.rsplit('.', 1)[-1].lower() if '.' in n else '') in SAMPLE_EXTS
        ]
        sample_total_bytes = 0
        for sp in sample_paths:
            try:
                info = zf.getinfo(sp)
                sample_total_bytes += info.file_size
            except Exception:
                pass

        # ── Sonic DNA ────────────────────────────────────────────────────
        # Try manifest first, then bundle_manifest.json
        dna = {}
        manifest_dna = manifest.get('sonic_dna') or manifest.get('SonicDNA') or {}
        if not manifest_dna:
            # Check bundle_manifest.json
            bm_paths = [n for n in names if Path(n).name.lower() == 'bundle_manifest.json']
            if bm_paths:
                try:
                    bm = json.loads(zf.read(bm_paths[0]).decode('utf-8', errors='replace'))
                    manifest_dna = bm.get('sonic_dna') or bm.get('SonicDNA') or {}
                except Exception:
                    pass

        for dim in DNA_DIMENSIONS:
            # Accept both lowercase and capitalized keys
            val = manifest_dna.get(dim) or manifest_dna.get(dim.capitalize())
            dna[dim] = float(val) if val is not None else None

    return {
        'path': xpn_path,
        'name': manifest.get('Name') or manifest.get('name') or Path(xpn_path).stem,
        'engine': manifest.get('Engine') or manifest.get('engine') or '',
        'version': manifest.get('Version') or manifest.get('version') or '',
        'mood': manifest.get('Mood') or manifest.get('mood') or '',
        'tags': manifest.get('Tags') or manifest.get('tags') or [],
        'programs': programs,
        'program_count': len(programs),
        'drum_count': drum_count,
        'melodic_count': melodic_count,
        'sample_count': len(sample_paths),
        'sample_bytes': sample_total_bytes,
        'dna': dna,
        'has_manifest': bool(manifest),
    }


# ---------------------------------------------------------------------------
# Inline pack scorer (simplified — mirrors xpn_pack_score.py model)
# ---------------------------------------------------------------------------

def _score_pack_inline(pack: dict, xpn_path: str) -> dict:
    """
    Run a lightweight inline composite score. Returns {score, grade, verdict}.
    Penalty model mirrors xpn_pack_score.py for consistency.
    """
    score = 100
    penalties = []

    if not pack['has_manifest']:
        score -= 20
        penalties.append('Manifest missing (-20)')

    if pack['program_count'] == 0:
        score -= 20
        penalties.append('No programs (-20)')

    # Long program names
    long_names = [p['name'] for p in pack['programs'] if len(p['name']) > 20]
    name_pen = min(len(long_names) * 2, 10)
    if name_pen:
        score -= name_pen
        penalties.append(f'Long names x{len(long_names)} (-{name_pen})')

    # Cover art check
    try:
        with zipfile.ZipFile(xpn_path, 'r') as zf:
            art = [n for n in zf.namelist()
                   if Path(n).suffix.lower() in ('.png', '.jpg', '.jpeg')]
        if not art:
            score -= 15
            penalties.append('No cover art (-15)')
    except Exception:
        pass

    # DNA completeness bonus hint (no penalty — just info)
    dna_missing = [d for d in DNA_DIMENSIONS if pack['dna'].get(d) is None]

    score = max(0, score)
    grade = _grade(score)
    verdict = 'PASS' if score >= 80 else 'FAIL'
    return {'score': score, 'grade': grade, 'verdict': verdict,
            'penalties': penalties, 'dna_missing': dna_missing}


# ---------------------------------------------------------------------------
# Report builders
# ---------------------------------------------------------------------------

def _build_comparison(pack_a: dict, pack_b: dict,
                      score_a: dict, score_b: dict) -> dict:
    """Assemble a structured comparison dict."""
    names_a = {p['name'] for p in pack_a['programs']}
    names_b = {p['name'] for p in pack_b['programs']}
    only_a = sorted(names_a - names_b)
    only_b = sorted(names_b - names_a)
    common = sorted(names_a & names_b)

    dna_comparison = {}
    for dim in DNA_DIMENSIONS:
        va = pack_a['dna'].get(dim)
        vb = pack_b['dna'].get(dim)
        delta = (vb - va) if (va is not None and vb is not None) else None
        dna_comparison[dim] = {'a': va, 'b': vb, 'delta': delta}

    # Verdict sentence
    prog_diff = pack_b['program_count'] - pack_a['program_count']
    score_diff = score_b['score'] - score_a['score']
    verdict_parts = []
    if prog_diff > 0:
        verdict_parts.append(f'Pack B adds {prog_diff} program{"s" if prog_diff != 1 else ""}')
    elif prog_diff < 0:
        verdict_parts.append(f'Pack B removes {abs(prog_diff)} program{"s" if abs(prog_diff) != 1 else ""}')
    else:
        verdict_parts.append('Same program count')
    if score_diff > 0:
        verdict_parts.append(f'improves score by {score_diff} points')
    elif score_diff < 0:
        verdict_parts.append(f'score drops by {abs(score_diff)} points')
    else:
        verdict_parts.append('same score')
    verdict_sentence = ', '.join(verdict_parts) + '.'

    return {
        'pack_a': pack_a,
        'pack_b': pack_b,
        'score_a': score_a,
        'score_b': score_b,
        'dna_comparison': dna_comparison,
        'programs_only_a': only_a,
        'programs_only_b': only_b,
        'programs_common': common,
        'verdict': verdict_sentence,
    }


def _format_text(cmp: dict) -> str:
    a = cmp['pack_a']
    b = cmp['pack_b']
    sa = cmp['score_a']
    sb = cmp['score_b']

    W = 22  # label column width
    CA = 22  # pack A value column width
    CB = 22  # pack B value column width

    SEP = '\u2550' * 68

    lines = []
    lines.append('COMPARISON REPORT')
    lines.append(SEP)

    # Header row
    lines.append(f'{"":>{W}}  {"PACK A":<{CA}}{"PACK B":<{CB}}')
    lines.append('-' * 68)

    def row(label: str, val_a: str, val_b: str) -> str:
        return f'{label:<{W}}  {val_a:<{CA}}{val_b:<{CB}}'

    lines.append(row('Name', a['name'][:CA].strip(), b['name'][:CB].strip()))
    lines.append(row('Engine', a['engine'] or '—', b['engine'] or '—'))
    lines.append(row('Version', a['version'] or '—', b['version'] or '—'))
    lines.append(row('Mood', a['mood'] or '—', b['mood'] or '—'))

    tags_a = ', '.join(a['tags'][:3]) if a['tags'] else '—'
    tags_b = ', '.join(b['tags'][:3]) if b['tags'] else '—'
    lines.append(row('Tags', tags_a[:CA], tags_b[:CB]))

    lines.append('-' * 68)

    pc_b_label = str(b['program_count']) + _delta_label(b['program_count'], a['program_count'])
    lines.append(row('Programs', str(a['program_count']), pc_b_label))

    dc_b_label = str(b['drum_count']) + _delta_label(b['drum_count'], a['drum_count'])
    lines.append(row('  Drum', str(a['drum_count']), dc_b_label))

    mc_b_label = str(b['melodic_count']) + _delta_label(b['melodic_count'], a['melodic_count'])
    lines.append(row('  Melodic', str(a['melodic_count']), mc_b_label))

    sc_b_label = str(b['sample_count']) + _delta_label(b['sample_count'], a['sample_count'])
    lines.append(row('Samples', str(a['sample_count']), sc_b_label))

    def _fmt_bytes(n: int) -> str:
        if n >= 1_000_000:
            return f'{n / 1_000_000:.1f} MB'
        if n >= 1_000:
            return f'{n / 1_000:.1f} KB'
        return f'{n} B'

    size_a = _fmt_bytes(a['sample_bytes'])
    size_b = _fmt_bytes(b['sample_bytes'])
    lines.append(row('  Total size', size_a, size_b))

    # DNA
    lines.append('')
    lines.append('SONIC DNA')
    lines.append('-' * 68)
    dna = cmp['dna_comparison']
    for dim in DNA_DIMENSIONS:
        d = dna[dim]
        va = d['a']
        vb = d['b']
        delta = d['delta']

        if va is not None:
            bar_a = f'{va:.2f}  {_dna_bar(va)}'
        else:
            bar_a = 'N/A'
        if vb is not None:
            bar_b = f'{vb:.2f}  {_dna_bar(vb)}'
        else:
            bar_b = 'N/A'
        delta_s = _delta_str(delta) if delta is not None else 'N/A'

        lines.append(f'{dim.capitalize():<{W}}  {bar_a:<{CA}}{bar_b:<{CB}} {delta_s}')

    # Programs
    lines.append('')
    lines.append('PROGRAMS')
    lines.append('-' * 68)

    only_a = cmp['programs_only_a']
    only_b = cmp['programs_only_b']
    common = cmp['programs_common']

    def _prog_list(lst: list, max_show: int = 6) -> str:
        if not lst:
            return '(none)'
        shown = ', '.join(lst[:max_show])
        if len(lst) > max_show:
            shown += f', … +{len(lst) - max_show} more'
        return shown

    lines.append(f'  Only in A ({len(only_a)}): {_prog_list(only_a)}')
    lines.append(f'  Only in B ({len(only_b)}): {_prog_list(only_b)}')
    lines.append(f'  Common   ({len(common)}): {_prog_list(common)}')

    # Scores
    lines.append('')
    lines.append('SCORES')
    lines.append('-' * 68)
    lines.append(f'  Pack A: {sa["score"]}/100 ({sa["grade"]}) {sa["verdict"]}')
    if sa['penalties']:
        for p in sa['penalties']:
            lines.append(f'           - {p}')
    lines.append(f'  Pack B: {sb["score"]}/100 ({sb["grade"]}) {sb["verdict"]}')
    if sb['penalties']:
        for p in sb['penalties']:
            lines.append(f'           - {p}')

    # Verdict
    lines.append('')
    lines.append(SEP)
    lines.append(f'VERDICT: {cmp["verdict"]}')

    return '\n'.join(lines)


def _format_markdown(cmp: dict) -> str:
    a = cmp['pack_a']
    b = cmp['pack_b']
    sa = cmp['score_a']
    sb = cmp['score_b']

    lines = ['# XPN Pack Comparison Report', '']
    lines.append('| | Pack A | Pack B |')
    lines.append('|---|---|---|')

    def mrow(label: str, va: str, vb: str):
        lines.append(f'| **{label}** | {va} | {vb} |')

    mrow('Name', a['name'], b['name'])
    mrow('Engine', a['engine'] or '—', b['engine'] or '—')
    mrow('Version', a['version'] or '—', b['version'] or '—')
    mrow('Mood', a['mood'] or '—', b['mood'] or '—')
    mrow('Programs', str(a['program_count']),
         str(b['program_count']) + _delta_label(b['program_count'], a['program_count']))
    mrow('Samples', str(a['sample_count']),
         str(b['sample_count']) + _delta_label(b['sample_count'], a['sample_count']))
    mrow('Score', f'{sa["score"]}/100 ({sa["grade"]}) {sa["verdict"]}',
         f'{sb["score"]}/100 ({sb["grade"]}) {sb["verdict"]}')

    lines.append('')
    lines.append('## Sonic DNA')
    lines.append('')
    lines.append('| Dimension | Pack A | | Pack B | | Delta |')
    lines.append('|---|---|---|---|---|---|')
    for dim in DNA_DIMENSIONS:
        d = cmp['dna_comparison'][dim]
        va = d['a']
        vb = d['b']
        delta = d['delta']
        bar_a = _dna_bar(va) if va is not None else 'N/A'
        bar_b = _dna_bar(vb) if vb is not None else 'N/A'
        val_a = f'{va:.2f}' if va is not None else '—'
        val_b = f'{vb:.2f}' if vb is not None else '—'
        delta_s = _delta_str(delta) if delta is not None else '—'
        lines.append(f'| {dim.capitalize()} | {val_a} | `{bar_a}` | {val_b} | `{bar_b}` | {delta_s} |')

    lines.append('')
    lines.append('## Programs')
    lines.append('')
    only_a = cmp['programs_only_a']
    only_b = cmp['programs_only_b']
    common = cmp['programs_common']
    lines.append(f'**Only in Pack A ({len(only_a)}):** ' +
                 (', '.join(only_a) if only_a else '_(none)_'))
    lines.append('')
    lines.append(f'**Only in Pack B ({len(only_b)}):** ' +
                 (', '.join(only_b) if only_b else '_(none)_'))
    lines.append('')
    lines.append(f'**Common ({len(common)}):** ' +
                 (', '.join(common[:10]) + (f', …+{len(common)-10} more' if len(common) > 10 else '')
                  if common else '_(none)_'))

    lines.append('')
    lines.append('---')
    lines.append(f'**Verdict:** {cmp["verdict"]}')

    return '\n'.join(lines)


def _format_json(cmp: dict) -> str:
    output = {
        'pack_a': {
            'path': cmp['pack_a']['path'],
            'name': cmp['pack_a']['name'],
            'engine': cmp['pack_a']['engine'],
            'version': cmp['pack_a']['version'],
            'mood': cmp['pack_a']['mood'],
            'tags': cmp['pack_a']['tags'],
            'program_count': cmp['pack_a']['program_count'],
            'drum_count': cmp['pack_a']['drum_count'],
            'melodic_count': cmp['pack_a']['melodic_count'],
            'sample_count': cmp['pack_a']['sample_count'],
            'sample_bytes': cmp['pack_a']['sample_bytes'],
            'dna': cmp['pack_a']['dna'],
        },
        'pack_b': {
            'path': cmp['pack_b']['path'],
            'name': cmp['pack_b']['name'],
            'engine': cmp['pack_b']['engine'],
            'version': cmp['pack_b']['version'],
            'mood': cmp['pack_b']['mood'],
            'tags': cmp['pack_b']['tags'],
            'program_count': cmp['pack_b']['program_count'],
            'drum_count': cmp['pack_b']['drum_count'],
            'melodic_count': cmp['pack_b']['melodic_count'],
            'sample_count': cmp['pack_b']['sample_count'],
            'sample_bytes': cmp['pack_b']['sample_bytes'],
            'dna': cmp['pack_b']['dna'],
        },
        'score_a': cmp['score_a'],
        'score_b': cmp['score_b'],
        'dna_comparison': cmp['dna_comparison'],
        'programs_only_a': cmp['programs_only_a'],
        'programs_only_b': cmp['programs_only_b'],
        'programs_common': cmp['programs_common'],
        'verdict': cmp['verdict'],
    }
    return json.dumps(output, indent=2)


# ---------------------------------------------------------------------------
# CLI entry point
# ---------------------------------------------------------------------------

def main():
    parser = argparse.ArgumentParser(
        description='Compare two .xpn expansion packs side-by-side.')
    parser.add_argument('pack_a', help='Path to Pack A (.xpn)')
    parser.add_argument('pack_b', help='Path to Pack B (.xpn)')
    parser.add_argument('--format', choices=['text', 'json', 'markdown'],
                        default='text', help='Output format (default: text)')
    parser.add_argument('--output', metavar='FILE',
                        help='Write report to file instead of stdout')
    args = parser.parse_args()

    pack_a = _read_pack(args.pack_a)
    pack_b = _read_pack(args.pack_b)

    if pack_a is None or pack_b is None:
        sys.exit(1)

    score_a = _score_pack_inline(pack_a, args.pack_a)
    score_b = _score_pack_inline(pack_b, args.pack_b)

    cmp = _build_comparison(pack_a, pack_b, score_a, score_b)

    if args.format == 'json':
        report = _format_json(cmp)
    elif args.format == 'markdown':
        report = _format_markdown(cmp)
    else:
        report = _format_text(cmp)

    if args.output:
        Path(args.output).write_text(report, encoding='utf-8')
        print(f'Report written to {args.output}')
    else:
        print(report)


if __name__ == '__main__':
    main()
