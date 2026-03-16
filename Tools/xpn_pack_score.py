#!/usr/bin/env python3
"""
xpn_pack_score.py — XPN Pack Composite Quality Scorer
XO_OX Designs

Computes a composite quality score (0–100) for a .xpn pack file by
aggregating multiple QA checks directly from the archive contents.
This is a scoring aggregator: it reads the ZIP, parses XPM XML and
expansion.json, inspects image dimensions, and samples WAV headers
for DC offset detection — no external QA output files required.

Usage:
    python xpn_pack_score.py <pack.xpn> [--format text|json] [--strict]

Exit codes:
    0  Score >= 80 (or --strict not set)
    1  Score < 80  (only when --strict is set)
    2  Score < 60  (only when --strict is set)
    3  Fatal error (file not found, not a valid ZIP)

Scoring model — 100 points, penalties applied:
    Manifest missing/invalid          -20
    Cover art missing                 -15
    Cover art below 400px             -5
    Sample errors (clipping/DC)       -5 per error  (max -20)
    Sample warnings                   -2 per warning (max -10)
    Pad coverage gaps (drums)         -5
    Velocity layer inconsistency      -3
    Tuning anomalies                  -3 per anomaly (max -9)
    No programs in pack               -20
    Program name >20 chars            -2 per violation (max -10)

Grades:
    90–100  A
    80–89   B
    70–79   C
    60–69   D
    <60     F
"""

import argparse
import json
import math
import struct
import sys
import zipfile
from pathlib import Path
from typing import Optional
from xml.etree import ElementTree as ET


# ---------------------------------------------------------------------------
# WAV helpers — DC offset + clipping detection
# ---------------------------------------------------------------------------

def _parse_wav_frames(data: bytes, max_frames: int = 512) -> Optional[list]:
    """
    Return up to max_frames PCM samples (float, mixed to mono) from the
    start and end of the data chunk.  Supports 16-bit and 24-bit integer PCM.
    Returns None if format is unsupported or the file is malformed.
    """
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

        pos = body_end + (chunk_size % 2)  # RIFF pad byte

    if fmt_info is None or data_chunk is None:
        return None
    if fmt_info['audio_format'] != 1:  # PCM only
        return None

    bit_depth = fmt_info['bit_depth']
    channels = fmt_info['channels']
    if bit_depth not in (16, 24):
        return None

    frame_bytes = (bit_depth // 8) * channels
    total_frames = len(data_chunk) // frame_bytes
    if total_frames == 0:
        return None

    # Read first half and last half of max_frames budget
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
            else:  # 24-bit
                b0, b1, b2 = (data_chunk[offset],
                               data_chunk[offset + 1],
                               data_chunk[offset + 2])
                raw = b0 | (b1 << 8) | (b2 << 16)
                if raw & 0x800000:
                    raw -= 0x1000000
                frame_sum += raw / 8388608.0
        samples.append(frame_sum / channels)

    return samples


def _check_wav_issues(data: bytes) -> tuple:
    """
    Return (errors, warnings) lists for a single WAV file.
    Errors: clipping (peak >= -0.5 dBFS), DC offset (mean abs > 0.05).
    Warnings: near-clipping (-3 to -0.5 dBFS), very low DC (0.02–0.05).
    """
    errors, warnings = [], []
    samples = _parse_wav_frames(data, max_frames=1024)
    if samples is None:
        return errors, warnings

    peak = max(abs(s) for s in samples) if samples else 0.0
    mean_abs = sum(abs(s) for s in samples) / len(samples) if samples else 0.0

    # Clipping: peak >= -0.5 dBFS  (linear ~0.944)
    CLIP_LINEAR = 10 ** (-0.5 / 20)  # ≈ 0.944
    NEAR_CLIP_LINEAR = 10 ** (-3.0 / 20)  # ≈ 0.708

    if peak >= CLIP_LINEAR:
        errors.append('clipping')
    elif peak >= NEAR_CLIP_LINEAR:
        warnings.append('near-clipping')

    # DC offset
    if mean_abs > 0.05:
        errors.append('DC offset')
    elif mean_abs > 0.02:
        warnings.append('high DC')

    return errors, warnings


# ---------------------------------------------------------------------------
# Image dimension helper — PNG + JPEG, no external libs
# ---------------------------------------------------------------------------

def _image_dimensions(data: bytes) -> Optional[tuple]:
    """Return (width, height) for PNG or JPEG binary data, else None."""
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
            # SOF markers carry image dimensions
            if marker in (0xC0, 0xC1, 0xC2, 0xC3, 0xC5, 0xC6, 0xC7,
                          0xC9, 0xCA, 0xCB, 0xCD, 0xCE, 0xCF):
                if pos + 9 <= len(data):
                    h = struct.unpack_from('>H', data, pos + 5)[0]
                    w = struct.unpack_from('>H', data, pos + 7)[0]
                    return (w, h)
            pos += 2 + seg_len
    return None


# ---------------------------------------------------------------------------
# XPM XML analysis
# ---------------------------------------------------------------------------

def _analyze_xpm(xml_data: bytes) -> dict:
    """
    Parse an XPM program XML and return analysis dict with keys:
      program_name, pad_count, pads_with_samples, velocity_layers_per_pad,
      root_notes, tuning_anomalies, name_length_ok
    Returns None on parse error.
    """
    try:
        root = ET.fromstring(xml_data)
    except ET.ParseError:
        return None

    # Program name
    program_name = ''
    name_el = root.find('.//ProgramName')
    if name_el is not None and name_el.text:
        program_name = name_el.text.strip()
    if not program_name:
        # Also try attribute
        program_name = root.get('name', '')

    # Pads: look for <Pad> elements
    pads = root.findall('.//Pad')
    pad_count = len(pads)

    pads_with_samples = 0
    velocity_layer_counts = []
    root_notes = []
    tuning_anomalies = []

    for pad in pads:
        layers = pad.findall('.//Layer') or pad.findall('.//SampleLayer')
        # Count non-empty layers
        active_layers = []
        for layer in layers:
            sample = layer.get('SampleFile', '') or layer.get('file', '')
            if not sample:
                # Check child element
                sf = layer.find('SampleFile')
                if sf is not None and sf.text:
                    sample = sf.text.strip()
            if sample:
                active_layers.append(layer)

        if active_layers:
            pads_with_samples += 1
            velocity_layer_counts.append(len(active_layers))

            # Collect root notes for tuning anomaly detection
            for layer in active_layers:
                rn = layer.get('RootNote', '') or layer.get('rootNote', '')
                if rn:
                    try:
                        root_notes.append(int(rn))
                    except ValueError:
                        pass

                # Tuning: check Tune/tune attribute for extreme values
                tune_str = layer.get('Tune', '') or layer.get('tune', '')
                if tune_str:
                    try:
                        tune_val = float(tune_str)
                        # Flag tuning outside ±50 cents as anomalous
                        if abs(tune_val) > 50:
                            tuning_anomalies.append(
                                f"tune={tune_val:.1f} on pad {pad.get('number', '?')}"
                            )
                    except ValueError:
                        pass

    # Velocity layer consistency: flag if spread > 1 across pads
    vel_consistent = True
    if len(velocity_layer_counts) > 1:
        if max(velocity_layer_counts) - min(velocity_layer_counts) > 1:
            vel_consistent = False

    return {
        'program_name': program_name,
        'pad_count': pad_count,
        'pads_with_samples': pads_with_samples,
        'velocity_layer_counts': velocity_layer_counts,
        'vel_consistent': vel_consistent,
        'root_notes': root_notes,
        'tuning_anomalies': tuning_anomalies,
        'name_length_ok': len(program_name) <= 20,
    }


# ---------------------------------------------------------------------------
# Penalty record
# ---------------------------------------------------------------------------

class Penalty:
    def __init__(self, check: str, points: int, detail: str = ''):
        self.check = check
        self.points = points   # positive integer = points deducted
        self.detail = detail

    def to_dict(self) -> dict:
        return {'check': self.check, 'points': self.points, 'detail': self.detail}


# ---------------------------------------------------------------------------
# Main scorer
# ---------------------------------------------------------------------------

def score_pack(xpn_path: str) -> dict:
    """
    Analyse a .xpn file and return a result dict:
      score, grade, penalties, verdict, errors
    """
    penalties = []
    errors = []

    # --- Open archive ---
    try:
        zf = zipfile.ZipFile(xpn_path, 'r')
    except FileNotFoundError:
        return {'score': 0, 'grade': 'F',
                'penalties': [], 'verdict': 'FAIL',
                'errors': [f'File not found: {xpn_path}']}
    except zipfile.BadZipFile:
        return {'score': 0, 'grade': 'F',
                'penalties': [], 'verdict': 'FAIL',
                'errors': [f'Not a valid ZIP/XPN file: {xpn_path}']}

    with zf:
        names = zf.namelist()

        # ── 1. Manifest (expansion.json) ─────────────────────────────────
        manifest_names = [n for n in names
                          if Path(n).name.lower() == 'expansion.json']
        if not manifest_names:
            penalties.append(Penalty('Manifest missing/invalid', 20,
                                     'expansion.json not found in pack'))
        else:
            try:
                raw = zf.read(manifest_names[0])
                manifest = json.loads(raw.decode('utf-8', errors='replace'))
                # Basic validity: must have at least a Name or name field
                if not manifest.get('Name') and not manifest.get('name'):
                    penalties.append(Penalty('Manifest missing/invalid', 20,
                                             'expansion.json has no Name field'))
            except (json.JSONDecodeError, Exception) as exc:
                penalties.append(Penalty('Manifest missing/invalid', 20,
                                         f'expansion.json unreadable: {exc}'))

        # ── 2. Cover art ─────────────────────────────────────────────────
        art_files = [
            n for n in names
            if Path(n).suffix.lower() in ('.png', '.jpg', '.jpeg')
        ]
        if not art_files:
            penalties.append(Penalty('Cover art missing', 15,
                                     'No PNG/JPEG found in pack'))
        else:
            # Use the largest-named image at root level, or any match
            root_images = [n for n in art_files
                           if '/' not in n or n.count('/') == 0]
            best = root_images[0] if root_images else art_files[0]
            try:
                img_data = zf.read(best)
                dims = _image_dimensions(img_data)
                if dims is not None:
                    w, h = dims
                    if min(w, h) < 400:
                        penalties.append(Penalty('Cover art below 400px', 5,
                                                 f'{best}: {w}×{h} px'))
                # If dims is None we cannot determine size — no penalty
            except Exception as exc:
                errors.append(f'Could not read cover art {best}: {exc}')

        # ── 3. WAV sample errors and warnings ────────────────────────────
        wav_files = [n for n in names if n.lower().endswith('.wav')]
        sample_error_count = 0
        sample_warn_count = 0
        for wav_name in wav_files:
            try:
                wav_data = zf.read(wav_name)
            except Exception as exc:
                errors.append(f'Could not read {wav_name}: {exc}')
                continue
            wav_errors, wav_warns = _check_wav_issues(wav_data)
            for e in wav_errors:
                if sample_error_count < 4:  # cap at 4 → max -20
                    penalties.append(Penalty(
                        'Sample error', 5,
                        f'{Path(wav_name).name}: {e}'))
                sample_error_count += 1
            for w in wav_warns:
                if sample_warn_count < 5:  # cap at 5 → max -10
                    penalties.append(Penalty(
                        'Sample warning', 2,
                        f'{Path(wav_name).name}: {w}'))
                sample_warn_count += 1

        # ── 4. XPM program analysis ───────────────────────────────────────
        xpm_files = [n for n in names if n.lower().endswith('.xpm')]

        if not xpm_files:
            penalties.append(Penalty('No programs in pack', 20,
                                     'No .xpm files found'))
        else:
            all_vel_consistent = True
            name_violations = 0
            tuning_anomaly_count = 0
            pad_gap_found = False

            for xpm_name in xpm_files:
                try:
                    xml_data = zf.read(xpm_name)
                except Exception as exc:
                    errors.append(f'Could not read {xpm_name}: {exc}')
                    continue

                analysis = _analyze_xpm(xml_data)
                if analysis is None:
                    errors.append(f'Could not parse XML in {xpm_name}')
                    continue

                # Pad coverage gaps (drum kits: expect pads A01–A16 = 16 pads)
                if analysis['pad_count'] > 0:
                    # Flag if fewer than 12 of 16 expected pads have samples
                    filled = analysis['pads_with_samples']
                    total = analysis['pad_count']
                    if total >= 12 and filled < 12:
                        pad_gap_found = True

                # Velocity layer inconsistency
                if not analysis['vel_consistent']:
                    all_vel_consistent = False

                # Tuning anomalies (max 3 penalties = -9)
                for anomaly in analysis['tuning_anomalies']:
                    if tuning_anomaly_count < 3:
                        penalties.append(Penalty(
                            'Tuning anomaly', 3,
                            f'{Path(xpm_name).name}: {anomaly}'))
                    tuning_anomaly_count += 1

                # Program name length (max 5 violations = -10)
                if not analysis['name_length_ok']:
                    if name_violations < 5:
                        penalties.append(Penalty(
                            'Program name >20 chars', 2,
                            f'"{analysis["program_name"]}" '
                            f'({len(analysis["program_name"])} chars)'))
                    name_violations += 1

            if pad_gap_found:
                penalties.append(Penalty('Pad coverage gaps', 5,
                                         'Fewer than 12/16 pads filled in a drum kit'))

            if not all_vel_consistent:
                penalties.append(Penalty('Velocity layer inconsistency', 3,
                                         'Pads have mismatched velocity layer counts'))

    # ── Compute final score ───────────────────────────────────────────────
    total_penalty = sum(p.points for p in penalties)
    score = max(0, 100 - total_penalty)

    if score >= 90:
        grade = 'A'
    elif score >= 80:
        grade = 'B'
    elif score >= 70:
        grade = 'C'
    elif score >= 60:
        grade = 'D'
    else:
        grade = 'F'

    if score >= 80:
        verdict = 'PASS'
    elif score >= 60:
        verdict = 'WARN'
    else:
        verdict = 'FAIL'

    return {
        'score': score,
        'grade': grade,
        'penalties': [p.to_dict() for p in penalties],
        'verdict': verdict,
        'errors': errors,
    }


# ---------------------------------------------------------------------------
# Output formatters
# ---------------------------------------------------------------------------

def _format_text(result: dict, pack_path: str) -> str:
    lines = []
    W = 68
    lines.append('=' * W)
    lines.append('  XPN PACK SCORE')
    lines.append(f'  Pack  : {pack_path}')
    lines.append(f'  Score : {result["score"]}/100  (Grade {result["grade"]})')
    lines.append(f'  Verdict: {result["verdict"]}')
    lines.append('=' * W)

    if result['errors']:
        lines.append('')
        lines.append('ERRORS:')
        for e in result['errors']:
            lines.append(f'  ! {e}')

    penalties = result['penalties']
    if penalties:
        lines.append('')
        lines.append('PENALTIES:')
        lines.append(f'  {"Check":<35}  {"Pts":>4}  Detail')
        lines.append('  ' + '-' * (W - 2))
        for p in penalties:
            detail = p['detail'][:40] if p['detail'] else ''
            lines.append(f'  {p["check"]:<35}  -{p["points"]:>3}  {detail}')
        total = sum(p['points'] for p in penalties)
        lines.append('  ' + '-' * (W - 2))
        lines.append(f'  {"Total deducted":<35}  -{total:>3}')
    else:
        lines.append('')
        lines.append('  No penalties — perfect score.')

    lines.append('')
    lines.append('GRADE SCALE:')
    lines.append('  90–100 = A   80–89 = B   70–79 = C   60–69 = D   <60 = F')
    lines.append('  PASS (>=80) | WARN (60–79) | FAIL (<60)')
    lines.append('=' * W)
    return '\n'.join(lines)


def _format_json(result: dict) -> str:
    # Return a clean subset matching the documented JSON schema
    output = {
        'score': result['score'],
        'grade': result['grade'],
        'penalties': result['penalties'],
        'verdict': result['verdict'],
    }
    if result['errors']:
        output['errors'] = result['errors']
    return json.dumps(output, indent=2)


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def main():
    parser = argparse.ArgumentParser(
        description='Compute a composite quality score for a .xpn pack file.',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    parser.add_argument('pack', metavar='PACK.XPN',
                        help='Path to the .xpn pack file.')
    parser.add_argument('--format', choices=['text', 'json'], default='text',
                        help='Output format (default: text).')
    parser.add_argument('--strict', action='store_true',
                        help='Exit 1 if score < 80; exit 2 if score < 60.')
    args = parser.parse_args()

    result = score_pack(args.pack)

    if args.format == 'json':
        print(_format_json(result))
    else:
        print(_format_text(result, args.pack))

    # Exit code
    if result['errors'] and result['score'] == 0 and not result['penalties']:
        # Fatal error (file not found / bad zip)
        sys.exit(3)

    if args.strict:
        if result['score'] < 60:
            sys.exit(2)
        if result['score'] < 80:
            sys.exit(1)

    sys.exit(0)


if __name__ == '__main__':
    main()
