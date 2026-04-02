#!/usr/bin/env python3
"""
xpn_pack_health_dashboard.py — XO_OX Pack Catalog Health Dashboard
XO_OX Designs

Generates a comprehensive health overview for a directory of .xpn packs.
Inline pack scoring (no external imports), DNA fleet coverage, missing
content detection, size analysis, and a single VERDICT line.

Usage:
    python xpn_pack_health_dashboard.py <packs_dir> [--format text|json|markdown] [--output FILE]

Output modes:
    text      (default) ASCII dashboard with bar charts
    json      Machine-readable dict
    markdown  GitHub-flavored tables

Exit codes:
    0  HEALTHY  (0 critical issues)
    1  WARNING  (warnings only, no critical)
    2  CRITICAL (1+ critical issues)
    3  Fatal error (directory not found / no packs)
"""

import argparse
import json
import math
import os
import struct
import sys
import zipfile
from datetime import date
from pathlib import Path
from typing import Any, Optional


# ---------------------------------------------------------------------------
# Constants
# ---------------------------------------------------------------------------

DNA_DIMENSIONS = ["brightness", "warmth", "movement", "density", "space", "aggression"]

VALID_MOODS = {
    "Foundation", "Atmosphere", "Entangled", "Prism", "Flux", "Aether", "Family"
}

# Score threshold below which a pack is flagged
NEEDS_ATTENTION_THRESHOLD = 70
CRITICAL_THRESHOLD = 60
LARGE_PACK_MB = 100


# ---------------------------------------------------------------------------
# Minimal WAV / image helpers (mirrors xpn_pack_score.py, no import needed)
# ---------------------------------------------------------------------------

def _parse_wav_samples(data: bytes, max_frames: int = 512) -> Optional[list]:
    if len(data) < 44 or data[:4] != b'RIFF' or data[8:12] != b'WAVE':
        return None
    fmt_info = None
    data_chunk = None
    pos = 12
    while pos + 8 <= len(data):
        chunk_id = data[pos:pos + 4]
        chunk_size = struct.unpack_from('<I', data, pos + 4)[0]
        body_start = pos + 8
        body_end = body_start + chunk_size
        if chunk_id == b'fmt ' and chunk_size >= 16:
            audio_format, channels, _sr, _br, _ba, bit_depth = \
                struct.unpack_from('<HHIIHH', data, body_start)
            fmt_info = {'audio_format': audio_format,
                        'channels': channels, 'bit_depth': bit_depth}
        elif chunk_id == b'data':
            data_chunk = data[body_start:body_end]
        pos = body_end + (chunk_size % 2)
    if fmt_info is None or data_chunk is None:
        return None
    if fmt_info['audio_format'] != 1:
        return None
    bit_depth = fmt_info['bit_depth']
    channels = fmt_info['channels']
    if bit_depth not in (16, 24):
        return None
    frame_bytes = (bit_depth // 8) * channels
    total_frames = len(data_chunk) // frame_bytes
    if total_frames == 0:
        return None
    half = max_frames // 2
    indices = list(range(min(half, total_frames)))
    if total_frames > half:
        tail_start = max(half, total_frames - half)
        indices += list(range(tail_start, total_frames))
    samples = []
    for fi in indices:
        frame_sum = 0.0
        for ch in range(channels):
            offset = fi * frame_bytes + ch * (bit_depth // 8)
            if bit_depth == 16:
                val = struct.unpack_from('<h', data_chunk, offset)[0]
                frame_sum += val / 32768.0
            else:
                b0, b1, b2 = (data_chunk[offset],
                               data_chunk[offset + 1],
                               data_chunk[offset + 2])
                raw = b0 | (b1 << 8) | (b2 << 16)
                if raw & 0x800000:
                    raw -= 0x1000000
                frame_sum += raw / 8388608.0
        samples.append(frame_sum / channels)
    return samples


def _wav_issues(data: bytes) -> tuple:
    errors, warnings = [], []
    samples = _parse_wav_samples(data, max_frames=1024)
    if samples is None:
        return errors, warnings
    peak = max(abs(s) for s in samples) if samples else 0.0
    mean_abs = sum(abs(s) for s in samples) / len(samples) if samples else 0.0
    CLIP = 10 ** (-0.5 / 20)
    NEAR_CLIP = 10 ** (-3.0 / 20)
    if peak >= CLIP:
        errors.append('clipping')
    elif peak >= NEAR_CLIP:
        warnings.append('near-clipping')
    if mean_abs > 0.05:
        errors.append('DC offset')
    elif mean_abs > 0.02:
        warnings.append('high DC')
    return errors, warnings


def _image_dims(data: bytes) -> Optional[tuple]:
    if data[:8] == b'\x89PNG\r\n\x1a\n' and len(data) >= 24:
        w = struct.unpack_from('>I', data, 16)[0]
        h = struct.unpack_from('>I', data, 20)[0]
        return (w, h)
    if data[:2] == b'\xff\xd8' and len(data) > 4:
        pos = 2
        while pos + 4 <= len(data):
            if data[pos] != 0xff:
                break
            marker = data[pos + 1]
            seg_len = struct.unpack_from('>H', data, pos + 2)[0]
            if marker in (0xC0, 0xC1, 0xC2, 0xC3, 0xC5, 0xC6, 0xC7,
                          0xC9, 0xCA, 0xCB, 0xCD, 0xCE, 0xCF):
                if pos + 9 <= len(data):
                    h = struct.unpack_from('>H', data, pos + 5)[0]
                    w = struct.unpack_from('>H', data, pos + 7)[0]
                    return (w, h)
            pos += 2 + seg_len
    return None


# ---------------------------------------------------------------------------
# Inline pack scorer — returns score 0-100
# ---------------------------------------------------------------------------

def _score_pack(zf: zipfile.ZipFile) -> int:
    """Compute pack quality score 0–100 from an open ZipFile."""
    names = zf.namelist()
    penalty = 0

    # Manifest
    manifest_names = [n for n in names if Path(n).name.lower() == 'expansion.json']
    if not manifest_names:
        penalty += 20
    else:
        try:
            raw = zf.read(manifest_names[0])
            mdata = json.loads(raw.decode('utf-8', errors='replace'))
            if not mdata.get('Name') and not mdata.get('name'):
                penalty += 20
        except Exception:
            penalty += 20

    # Cover art
    art_files = [n for n in names if Path(n).suffix.lower() in ('.png', '.jpg', '.jpeg')]
    if not art_files:
        penalty += 15
    else:
        root_images = [n for n in art_files if n.count('/') <= 1]
        best = root_images[0] if root_images else art_files[0]
        try:
            img_data = zf.read(best)
            dims = _image_dims(img_data)
            if dims and min(dims) < 400:
                penalty += 5
        except Exception as e:
            print(f'[WARN] Preset parse failed for {best}: {e}', file=sys.stderr)

    # WAV issues
    wav_files = [n for n in names if n.lower().endswith('.wav')]
    sample_err = 0
    sample_warn = 0
    for wav_name in wav_files:
        try:
            wav_data = zf.read(wav_name)
        except Exception:
            continue
        errs, warns = _wav_issues(wav_data)
        for _ in errs:
            if sample_err < 4:
                penalty += 5
            sample_err += 1
        for _ in warns:
            if sample_warn < 5:
                penalty += 2
            sample_warn += 1

    # Programs
    xpm_files = [n for n in names if n.lower().endswith('.xpm')]
    if not xpm_files:
        penalty += 20

    return max(0, 100 - penalty)


def _grade(score: int) -> str:
    if score >= 90:
        return 'A'
    if score >= 80:
        return 'B'
    if score >= 70:
        return 'C'
    if score >= 60:
        return 'D'
    return 'F'


# ---------------------------------------------------------------------------
# Pack reader — extracts all metadata needed for the dashboard
# ---------------------------------------------------------------------------

def read_pack(path: Path) -> dict[str, Any]:
    result: dict[str, Any] = {
        'path': path,
        'name': path.stem,
        'size_bytes': path.stat().st_size,
        'engine': None,
        'mood': None,
        'programs': 0,
        'samples': 0,
        'score': None,
        'grade': None,
        'has_cover_art': False,
        'has_expansion_json': False,
        'has_changelog': False,
        'dna': {d: None for d in DNA_DIMENSIONS},
        'error': None,
    }

    try:
        with zipfile.ZipFile(path, 'r') as zf:
            names = zf.namelist()
            names_lower = [n.lower() for n in names]

            # Quick membership checks
            result['has_cover_art'] = any(
                n.endswith(('.png', '.jpg', '.jpeg')) for n in names_lower
            )
            result['has_expansion_json'] = any(
                Path(n).name == 'expansion.json' for n in names_lower
            )
            result['has_changelog'] = any(
                'changelog' in n for n in names_lower
            )

            # Program + sample count
            result['programs'] = sum(1 for n in names_lower if n.endswith('.xpm'))
            result['samples'] = sum(1 for n in names_lower if n.endswith('.wav'))

            # Manifest — try expansion.json first, then bundle_manifest.json
            manifest_data: dict = {}
            for candidate in ('expansion.json', 'bundle_manifest.json'):
                matches = [n for n in names if Path(n).name.lower() == candidate]
                if matches:
                    try:
                        raw = zf.read(matches[0])
                        manifest_data = json.loads(raw.decode('utf-8', errors='replace'))
                        break
                    except Exception:
                        pass

            if manifest_data:
                result['name'] = manifest_data.get('Name') or manifest_data.get(
                    'name') or manifest_data.get('pack_name') or path.stem

                eng_raw = (manifest_data.get('Engine') or manifest_data.get('engine')
                           or manifest_data.get('engines'))
                if isinstance(eng_raw, list):
                    result['engine'] = [str(e).upper() for e in eng_raw]
                elif isinstance(eng_raw, str):
                    result['engine'] = eng_raw.upper()

                mood_raw = manifest_data.get('Mood') or manifest_data.get('mood')
                if isinstance(mood_raw, list):
                    result['mood'] = [str(m).capitalize() for m in mood_raw]
                elif isinstance(mood_raw, str):
                    result['mood'] = mood_raw.capitalize()

                # Sonic DNA
                dna_block = (manifest_data.get('sonic_dna')
                             or manifest_data.get('sonicDNA')
                             or manifest_data.get('SonicDNA') or {})
                if not dna_block:
                    dna_block = {d: manifest_data.get(d) for d in DNA_DIMENSIONS if d in manifest_data}
                for dim in DNA_DIMENSIONS:
                    raw_val = dna_block.get(dim)
                    try:
                        result['dna'][dim] = max(0.0, min(10.0, float(raw_val)))
                    except (TypeError, ValueError):
                        result['dna'][dim] = None

            # Score
            result['score'] = _score_pack(zf)
            result['grade'] = _grade(result['score'])

    except zipfile.BadZipFile:
        result['error'] = 'Not a valid ZIP/XPN archive'
    except Exception as exc:
        result['error'] = str(exc)

    return result


def scan_directory(directory: Path) -> list[dict[str, Any]]:
    packs = sorted(directory.glob('*.xpn'))
    if not packs:
        packs = sorted(directory.glob('**/*.xpn'))
    return [read_pack(p) for p in packs]


# ---------------------------------------------------------------------------
# Dashboard computation
# ---------------------------------------------------------------------------

def compute_dashboard(packs: list[dict[str, Any]]) -> dict[str, Any]:
    valid = [p for p in packs if not p['error']]
    total_size = sum(p['size_bytes'] for p in packs)
    total_programs = sum(p['programs'] for p in valid)
    total_samples = sum(p['samples'] for p in valid)

    # Engine distribution
    engine_counts: dict[str, int] = {}
    for p in valid:
        engines = p['engine'] if isinstance(p['engine'], list) else (
            [p['engine']] if p['engine'] else ['Unknown'])
        for e in engines:
            engine_counts[e] = engine_counts.get(e, 0) + 1

    # Mood distribution
    mood_counts: dict[str, int] = {}
    for p in valid:
        moods = p['mood'] if isinstance(p['mood'], list) else (
            [p['mood']] if p['mood'] else ['Unknown'])
        for m in moods:
            mood_counts[m] = mood_counts.get(m, 0) + 1

    # Quality distribution
    grade_counts = {'A': 0, 'B': 0, 'C': 0, 'D': 0, 'F': 0}
    needs_attention: list[dict] = []
    for p in valid:
        if p['grade']:
            grade_counts[p['grade']] = grade_counts.get(p['grade'], 0) + 1
        if p['score'] is not None and p['score'] < NEEDS_ATTENTION_THRESHOLD:
            needs_attention.append({'name': p['name'], 'score': p['score'], 'grade': p['grade']})

    # DNA coverage
    dna_stats: dict[str, Any] = {}
    for dim in DNA_DIMENSIONS:
        values = [p['dna'][dim] for p in valid if p['dna'].get(dim) is not None]
        if values:
            lo = round(min(values), 2)
            hi = round(max(values), 2)
            mean_val = round(sum(values) / len(values), 2)
            coverage_range = round(hi - lo, 2)
            dna_stats[dim] = {
                'min': lo, 'max': hi, 'mean': mean_val,
                'range': coverage_range,
                'poor': coverage_range < 0.4,
                'count': len(values),
            }
        else:
            dna_stats[dim] = {
                'min': None, 'max': None, 'mean': None,
                'range': 0.0, 'poor': True, 'count': 0,
            }

    # feliX/Oscar quadrant balance (brightness × warmth, scale 0–10 → midpoint 5)
    quadrants: dict[str, int] = {
        'bright-warm': 0, 'bright-cool': 0, 'dark-warm': 0, 'dark-cool': 0
    }
    for p in valid:
        b = p['dna'].get('brightness')
        w = p['dna'].get('warmth')
        if b is not None and w is not None:
            qb = 'bright' if b >= 5 else 'dark'
            qw = 'warm' if w >= 5 else 'cool'
            quadrants[f'{qb}-{qw}'] += 1

    # Missing content
    no_cover = [p['name'] for p in valid if not p['has_cover_art']]
    no_expansion = [p['name'] for p in valid if not p['has_expansion_json']]
    no_changelog = [p['name'] for p in valid if not p['has_changelog']]

    # Size analysis
    sorted_by_size = sorted(valid, key=lambda p: p['size_bytes'], reverse=True)
    top5 = [{'name': p['name'], 'size_mb': round(p['size_bytes'] / 1_048_576, 1)}
            for p in sorted_by_size[:5]]
    over_100mb = [{'name': p['name'], 'size_mb': round(p['size_bytes'] / 1_048_576, 1)}
                  for p in valid if p['size_bytes'] > LARGE_PACK_MB * 1_048_576]

    # Verdict
    critical_count = len(needs_attention)
    warning_count = len(no_changelog) + sum(
        1 for s in dna_stats.values() if s['poor']
    )
    if critical_count > 0:
        verdict = 'CRITICAL'
    elif warning_count > 0:
        verdict = 'WARNING'
    else:
        verdict = 'HEALTHY'

    # Distinct engine count for header
    engine_list = sorted(engine_counts.keys())

    return {
        'generated': str(date.today()),
        'total_packs': len(packs),
        'valid_packs': len(valid),
        'errored_packs': [{'name': p['name'], 'error': p['error']} for p in packs if p['error']],
        'total_programs': total_programs,
        'total_samples': total_samples,
        'total_size_bytes': total_size,
        'engine_distribution': dict(sorted(engine_counts.items(), key=lambda x: -x[1])),
        'engine_list': engine_list,
        'mood_distribution': dict(sorted(mood_counts.items(), key=lambda x: -x[1])),
        'grade_counts': grade_counts,
        'needs_attention': needs_attention,
        'dna_stats': dna_stats,
        'quadrants': quadrants,
        'no_cover_art': no_cover,
        'no_expansion_json': no_expansion,
        'no_changelog': no_changelog,
        'top5_by_size': top5,
        'over_100mb': over_100mb,
        'verdict': verdict,
        'critical_count': critical_count,
        'warning_count': warning_count,
    }


# ---------------------------------------------------------------------------
# Text formatter
# ---------------------------------------------------------------------------

BAR_WIDTH = 20
SEP = '\u2501' * 43  # ━━━


def _bar(count: int, max_count: int, width: int = BAR_WIDTH) -> str:
    if max_count == 0:
        return '\u2591' * width
    filled = round(count / max_count * width)
    return '\u2588' * filled + '\u2591' * (width - filled)


def _mb(b: int) -> str:
    mb = b / 1_048_576
    if mb >= 1000:
        return f'{mb / 1024:.1f} GB'
    return f'{mb:.1f} MB'


def format_text(d: dict[str, Any]) -> str:
    lines: list[str] = []

    engine_count = len(d['engine_list'])
    lines += [
        '',
        'XO_OX PACK CATALOG HEALTH DASHBOARD',
        f'Generated: {d["generated"]}  |  {d["total_packs"]} packs  |  {engine_count} engine(s)',
        '',
        SEP,
    ]

    # ── Catalog overview ─────────────────────────────────────────────────────
    lines += [
        'CATALOG OVERVIEW',
        f'  Packs: {d["total_packs"]:>5}  |  '
        f'Programs: {d["total_programs"]:>5}  |  '
        f'Samples: {d["total_samples"]:>6}',
        f'  Total size: {_mb(d["total_size_bytes"])}',
        '',
    ]

    if d['errored_packs']:
        lines.append(f'  ! {len(d["errored_packs"])} pack(s) could not be read:')
        for ep in d['errored_packs']:
            lines.append(f'    - {ep["name"]}: {ep["error"]}')
        lines.append('')

    # ── Engine distribution ───────────────────────────────────────────────────
    lines.append('ENGINE DISTRIBUTION')
    eng = d['engine_distribution']
    max_eng = max(eng.values(), default=1)
    for name, cnt in eng.items():
        bar = _bar(cnt, max_eng)
        lines.append(f'  {name:<12} {cnt:>3} packs  {bar}')
    lines.append('')

    # ── Mood distribution ─────────────────────────────────────────────────────
    lines.append('MOOD DISTRIBUTION')
    mood = d['mood_distribution']
    max_mood = max(mood.values(), default=1)
    for name, cnt in mood.items():
        bar = _bar(cnt, max_mood)
        lines.append(f'  {name:<14} {cnt:>3} packs  {bar}')
    lines.append('')

    # ── Quality distribution ──────────────────────────────────────────────────
    lines.append('QUALITY DISTRIBUTION')
    gc = d['grade_counts']
    total_scored = sum(gc.values())
    grade_groups = [
        ('A (90-100)', gc.get('A', 0)),
        ('B (80-89) ', gc.get('B', 0)),
        ('C (70-79) ', gc.get('C', 0)),
        ('D/F (<70) ', gc.get('D', 0) + gc.get('F', 0)),
    ]
    max_g = max(c for _, c in grade_groups) or 1
    for label, cnt in grade_groups:
        bar = _bar(cnt, max_g)
        lines.append(f'  {label}  {bar}  {cnt}')
    if d['needs_attention']:
        lines.append(f'  Needs attention (< {NEEDS_ATTENTION_THRESHOLD}):')
        for p in d['needs_attention']:
            lines.append(f'    ! {p["name"]}  score={p["score"]}  grade={p["grade"]}')
    lines.append('')

    # ── DNA coverage ──────────────────────────────────────────────────────────
    lines.append('DNA COVERAGE')
    for dim in DNA_DIMENSIONS:
        s = d['dna_stats'][dim]
        if s['count'] == 0:
            lines.append(f'  {dim:<12}  no data')
            continue
        flag = '  (< 0.4)' if s['poor'] else ''
        warn_char = '\u26a0' if s['poor'] else '\u2713'
        lines.append(
            f'  {dim:<12}  min={s["min"]:.2f}  max={s["max"]:.2f}'
            f'  range={s["range"]:.2f}  {warn_char}{flag}'
        )
    lines.append('')

    # feliX/Oscar quadrant balance
    q = d['quadrants']
    total_q = sum(q.values())
    lines.append('FELIX/OSCAR QUADRANT BALANCE')
    for quad, cnt in q.items():
        pct = f'{cnt / total_q * 100:.0f}%' if total_q else '-'
        bar = _bar(cnt, max(q.values()) or 1, width=12)
        lines.append(f'  {quad:<14}  {bar}  {cnt} ({pct})')
    lines.append('')

    # ── Missing content ───────────────────────────────────────────────────────
    lines.append('MISSING CONTENT')
    _mc = lambda label, items: (
        f'  {label}: {len(items)} packs'
        + (' \u26a0' if items else '')
    )
    lines.append(_mc('cover art      ', d['no_cover_art']))
    lines.append(_mc('expansion.json ', d['no_expansion_json']))
    lines.append(_mc('CHANGELOG      ', d['no_changelog']))
    if d['no_changelog']:
        for n in d['no_changelog']:
            lines.append(f'    - {n}')
    lines.append('')

    # ── Size analysis ─────────────────────────────────────────────────────────
    lines.append('SIZE ANALYSIS')
    lines.append('  Largest 5 packs:')
    for p in d['top5_by_size']:
        lines.append(f'    {p["name"]:<30}  {p["size_mb"]:>7.1f} MB')
    if d['over_100mb']:
        lines.append(f'  Packs > {LARGE_PACK_MB} MB (may need optimization):')
        for p in d['over_100mb']:
            lines.append(f'    ! {p["name"]}  {p["size_mb"]:.1f} MB')
    lines.append('')

    # ── Verdict ───────────────────────────────────────────────────────────────
    lines.append(SEP)
    lines.append(
        f'VERDICT: {d["verdict"]}'
        f'  ({d["critical_count"]} critical, {d["warning_count"]} warnings)'
    )
    lines.append('')
    return '\n'.join(lines)


# ---------------------------------------------------------------------------
# JSON formatter
# ---------------------------------------------------------------------------

def format_json(d: dict[str, Any]) -> str:
    # Convert Path objects to strings for serialization
    serializable = json.loads(json.dumps(d, default=str))
    return json.dumps(serializable, indent=2)


# ---------------------------------------------------------------------------
# Markdown formatter
# ---------------------------------------------------------------------------

def format_markdown(d: dict[str, Any]) -> str:
    lines: list[str] = []
    engine_count = len(d['engine_list'])

    lines += [
        f'# XO_OX Pack Catalog Health Dashboard',
        f'',
        f'**Generated:** {d["generated"]}  '
        f'**Packs:** {d["total_packs"]}  '
        f'**Engines:** {engine_count}  '
        f'**Verdict:** `{d["verdict"]}`',
        '',
        '## Catalog Overview',
        '',
        f'| Metric | Value |',
        f'|--------|-------|',
        f'| Packs | {d["total_packs"]} |',
        f'| Programs | {d["total_programs"]} |',
        f'| Samples | {d["total_samples"]} |',
        f'| Total Size | {_mb(d["total_size_bytes"])} |',
        '',
        '## Engine Distribution',
        '',
        '| Engine | Packs |',
        '|--------|-------|',
    ]
    for name, cnt in d['engine_distribution'].items():
        lines.append(f'| {name} | {cnt} |')

    lines += [
        '',
        '## Quality Distribution',
        '',
        '| Grade | Count |',
        '|-------|-------|',
    ]
    gc = d['grade_counts']
    for grade, cnt in [('A', gc.get('A', 0)), ('B', gc.get('B', 0)),
                        ('C', gc.get('C', 0)), ('D', gc.get('D', 0)),
                        ('F', gc.get('F', 0))]:
        lines.append(f'| {grade} | {cnt} |')

    if d['needs_attention']:
        lines += ['', '### Needs Attention', '', '| Pack | Score | Grade |',
                  '|------|-------|-------|']
        for p in d['needs_attention']:
            lines.append(f'| {p["name"]} | {p["score"]} | {p["grade"]} |')

    lines += [
        '',
        '## DNA Coverage',
        '',
        '| Dimension | Min | Max | Range | Status |',
        '|-----------|-----|-----|-------|--------|',
    ]
    for dim in DNA_DIMENSIONS:
        s = d['dna_stats'][dim]
        if s['count'] == 0:
            lines.append(f'| {dim} | — | — | — | no data |')
        else:
            status = 'poor coverage' if s['poor'] else 'ok'
            lines.append(
                f'| {dim} | {s["min"]:.2f} | {s["max"]:.2f}'
                f' | {s["range"]:.2f} | {status} |'
            )

    lines += [
        '',
        '## Missing Content',
        '',
        '| Check | Count |',
        '|-------|-------|',
        f'| Cover art | {len(d["no_cover_art"])} |',
        f'| expansion.json | {len(d["no_expansion_json"])} |',
        f'| CHANGELOG | {len(d["no_changelog"])} |',
        '',
        '## Size Analysis',
        '',
        '### Top 5 Largest Packs',
        '',
        '| Pack | Size |',
        '|------|------|',
    ]
    for p in d['top5_by_size']:
        lines.append(f'| {p["name"]} | {p["size_mb"]} MB |')

    if d['over_100mb']:
        lines += ['', f'### Packs > {LARGE_PACK_MB} MB', '', '| Pack | Size |', '|------|------|']
        for p in d['over_100mb']:
            lines.append(f'| {p["name"]} | {p["size_mb"]} MB |')

    lines += [
        '',
        '---',
        f'**VERDICT: {d["verdict"]}** — '
        f'{d["critical_count"]} critical, {d["warning_count"]} warnings',
    ]

    return '\n'.join(lines)


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def main():
    parser = argparse.ArgumentParser(
        description='Generate a health dashboard for a directory of .xpn packs.',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    parser.add_argument('packs_dir', metavar='PACKS_DIR',
                        help='Directory containing .xpn files.')
    parser.add_argument('--format', choices=['text', 'json', 'markdown'],
                        default='text',
                        help='Output format (default: text).')
    parser.add_argument('--output', metavar='FILE',
                        help='Write output to FILE instead of stdout.')
    args = parser.parse_args()

    packs_dir = Path(args.packs_dir)
    if not packs_dir.is_dir():
        print(f'Error: {packs_dir} is not a directory.', file=sys.stderr)
        sys.exit(3)

    packs = scan_directory(packs_dir)
    if not packs:
        print(f'No .xpn files found in {packs_dir}.', file=sys.stderr)
        sys.exit(3)

    dashboard = compute_dashboard(packs)

    if args.format == 'json':
        output = format_json(dashboard)
    elif args.format == 'markdown':
        output = format_markdown(dashboard)
    else:
        output = format_text(dashboard)

    if args.output:
        Path(args.output).write_text(output, encoding='utf-8')
        print(f'Dashboard written to {args.output}')
    else:
        print(output)

    verdict = dashboard['verdict']
    if verdict == 'CRITICAL':
        sys.exit(2)
    if verdict == 'WARNING':
        sys.exit(1)
    sys.exit(0)


if __name__ == '__main__':
    main()
