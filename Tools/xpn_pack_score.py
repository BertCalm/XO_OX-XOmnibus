#!/usr/bin/env python3
"""
xpn_pack_score.py — Automated XPN Pack Quality Scorer
Implements the automated criteria from the XPN Pack Quality Scoring Rubric v1.0.

Usage:
    python xpn_pack_score.py --xpn pack.xpn [--stems ./stems/] [--json]

Automated score: 40 points (Technical 25 + Documentation 15)
Human review:    60 points (Playability 25 + Sonic Identity 20 + Uniqueness 15)

Release gates (applied to extrapolated 100-pt score):
    85–100  PASS           — clear for release
    70–84   CONDITIONAL    — fix flagged items; soft launch to Patreon only
    0–69    FAIL           — return to development
"""

import argparse
import json
import math
import os
import struct
import sys
import zipfile
from dataclasses import dataclass, field, asdict
from pathlib import Path
from typing import Optional


# ---------------------------------------------------------------------------
# WAV inspection helpers
# ---------------------------------------------------------------------------

def _read_wav_header(data: bytes) -> Optional[dict]:
    """Parse a minimal WAV header; returns dict or None on parse failure."""
    if len(data) < 44:
        return None
    if data[:4] != b'RIFF' or data[8:12] != b'WAVE':
        return None

    # Scan for 'fmt ' chunk (may not be at offset 12 in all WAVs)
    pos = 12
    fmt = None
    while pos + 8 <= len(data):
        chunk_id = data[pos:pos+4]
        chunk_size = struct.unpack_from('<I', data, pos+4)[0]
        if chunk_id == b'fmt ':
            if pos + 8 + chunk_size < len(data) + 1 and chunk_size >= 16:
                audio_format, channels, sample_rate, byte_rate, block_align, bit_depth = \
                    struct.unpack_from('<HHIIHH', data, pos+8)
                fmt = {
                    'audio_format': audio_format,  # 1=PCM, 3=float
                    'channels': channels,
                    'sample_rate': sample_rate,
                    'bit_depth': bit_depth,
                }
            break
        pos += 8 + chunk_size
        if chunk_size % 2:
            pos += 1  # RIFF padding byte
    return fmt


def _read_wav_samples(data: bytes) -> Optional[list]:
    """
    Extract raw PCM samples as floats in [-1.0, 1.0].
    Supports 16-bit and 24-bit integer PCM only (matches rubric scope).
    Returns None if format unsupported or data chunk missing.
    """
    if len(data) < 44:
        return None
    if data[:4] != b'RIFF' or data[8:12] != b'WAVE':
        return None

    fmt_info = None
    data_chunk = None
    pos = 12
    while pos + 8 <= len(data):
        chunk_id = data[pos:pos+4]
        chunk_size = struct.unpack_from('<I', data, pos+4)[0]
        body_start = pos + 8
        body_end = body_start + chunk_size

        if chunk_id == b'fmt ' and chunk_size >= 16:
            audio_format, channels, sample_rate, byte_rate, block_align, bit_depth = \
                struct.unpack_from('<HHIIHH', data, body_start)
            fmt_info = {
                'audio_format': audio_format,
                'channels': channels,
                'bit_depth': bit_depth,
            }
        elif chunk_id == b'data':
            data_chunk = data[body_start:body_end]

        pos = body_end
        if chunk_size % 2:
            pos += 1

    if fmt_info is None or data_chunk is None:
        return None
    if fmt_info['audio_format'] != 1:  # PCM only
        return None

    bit_depth = fmt_info['bit_depth']
    channels = fmt_info['channels']

    if bit_depth == 16:
        n_frames = len(data_chunk) // (2 * channels)
        samples = []
        for i in range(n_frames):
            for ch in range(channels):
                offset = (i * channels + ch) * 2
                val = struct.unpack_from('<h', data_chunk, offset)[0]
                samples.append(val / 32768.0)
        return samples
    elif bit_depth == 24:
        frame_size = 3 * channels
        n_frames = len(data_chunk) // frame_size
        samples = []
        for i in range(n_frames):
            for ch in range(channels):
                offset = (i * channels + ch) * 3
                b0, b1, b2 = data_chunk[offset], data_chunk[offset+1], data_chunk[offset+2]
                raw = b0 | (b1 << 8) | (b2 << 16)
                if raw & 0x800000:
                    raw -= 0x1000000
                samples.append(raw / 8388608.0)
        return samples

    return None


def _dbfs(linear_amplitude: float) -> float:
    """Convert linear amplitude to dBFS. Returns -inf for 0."""
    if linear_amplitude <= 0.0:
        return -math.inf
    return 20.0 * math.log10(linear_amplitude)


def _peak_dbfs(samples: list) -> float:
    if not samples:
        return -math.inf
    return _dbfs(max(abs(s) for s in samples))


def _rms_dbfs(samples: list) -> float:
    if not samples:
        return -math.inf
    mean_sq = sum(s * s for s in samples) / len(samples)
    if mean_sq <= 0.0:
        return -math.inf
    return _dbfs(math.sqrt(mean_sq))


# ---------------------------------------------------------------------------
# Image dimension helper (PNG + JPEG, no external libs)
# ---------------------------------------------------------------------------

def _image_dimensions(data: bytes) -> Optional[tuple]:
    """Return (width, height) for PNG or JPEG, else None."""
    # PNG: 8-byte sig + 4-byte len + 'IHDR' + 4W + 4H
    if data[:8] == b'\x89PNG\r\n\x1a\n' and len(data) >= 24:
        w = struct.unpack_from('>I', data, 16)[0]
        h = struct.unpack_from('>I', data, 20)[0]
        return (w, h)
    # JPEG: starts with FFD8
    if data[:2] == b'\xff\xd8' and len(data) > 4:
        pos = 2
        while pos + 4 <= len(data):
            marker = data[pos:pos+2]
            if marker[0] != 0xff:
                break
            seg_len = struct.unpack_from('>H', data, pos+2)[0]
            # SOF markers: C0–C3, C5–C7, C9–CB, CD–CF
            if marker[1] in (0xC0, 0xC1, 0xC2, 0xC3, 0xC5, 0xC6, 0xC7,
                              0xC9, 0xCA, 0xCB, 0xCD, 0xCE, 0xCF):
                if pos + 9 <= len(data):
                    h = struct.unpack_from('>H', data, pos+5)[0]
                    w = struct.unpack_from('>H', data, pos+7)[0]
                    return (w, h)
            pos += 2 + seg_len
    return None


# ---------------------------------------------------------------------------
# YAML field extractor (stdlib only — no PyYAML dependency)
# ---------------------------------------------------------------------------

REQUIRED_YAML_FIELDS = {
    'name', 'engine', 'version', 'felix_oscar_axis',
    'mood_tags', 'dna_tags', 'coupling_suggestions', 'preset_count',
}


def _parse_yaml_fields(text: str) -> dict:
    """
    Minimal key extraction from pack.yaml.
    Handles simple `key: value` lines and block lists (`key:\n  - item`).
    Good enough for schema presence validation; not a full YAML parser.
    """
    fields = {}
    current_key = None
    current_list = []
    in_list = False

    for raw_line in text.splitlines():
        line = raw_line.strip()
        if not line or line.startswith('#'):
            continue

        if line.startswith('- ') and in_list and current_key:
            current_list.append(line[2:].strip())
            continue

        if ':' in line and not line.startswith('-'):
            # Flush previous list key
            if in_list and current_key:
                fields[current_key] = current_list
                current_list = []
                in_list = False

            key, _, value = line.partition(':')
            key = key.strip()
            value = value.strip()
            current_key = key

            if value == '' or value == '|' or value == '>':
                in_list = True
            else:
                fields[key] = value
                in_list = False

    # Final flush
    if in_list and current_key:
        fields[current_key] = current_list

    return fields


def _yaml_field_score(fields: dict) -> tuple:
    """Returns (score, missing_fields)."""
    missing = [f for f in REQUIRED_YAML_FIELDS if f not in fields or not fields[f]]
    n_missing = len(missing)
    if n_missing == 0:
        return 5, []
    elif n_missing <= 2:
        return 2, missing
    else:
        return 0, missing


def _yaml_coupling_valid(fields: dict) -> bool:
    """coupling_suggestions field is present and non-empty."""
    val = fields.get('coupling_suggestions', '')
    if isinstance(val, list):
        return len(val) > 0
    return bool(str(val).strip())


# ---------------------------------------------------------------------------
# Result containers
# ---------------------------------------------------------------------------

@dataclass
class CriterionResult:
    name: str
    category: str
    max_pts: int
    scored_pts: int
    automated: bool
    notes: str = ''


@dataclass
class ScoreReport:
    pack_path: str
    stems_path: Optional[str]
    criteria: list = field(default_factory=list)
    automated_total: int = 0
    automated_max: int = 40
    extrapolated_score: float = 0.0
    verdict: str = ''
    human_checklist: list = field(default_factory=list)
    errors: list = field(default_factory=list)


# ---------------------------------------------------------------------------
# Core scorer
# ---------------------------------------------------------------------------

class XPNPackScorer:

    # Rubric: "dead" threshold is -60 dBFS in spec text; heading says -72 dBFS.
    # Using -60 dBFS as written in the measurement description (more conservative).
    DEAD_THRESHOLD_DBFS = -60.0
    CLIP_THRESHOLD_DBFS = -0.5
    VALID_SAMPLE_RATES = {44100, 48000}
    VALID_BIT_DEPTHS = {16, 24}
    LEVEL_SPREAD_TIGHT_DB = 6.0
    LEVEL_SPREAD_WIDE_DB = 9.0
    COVER_ART_MIN_DIM = 500   # rubric says 500×500 automated; 1000×1000 brand standard

    def __init__(self, xpn_path: str, stems_path: Optional[str] = None):
        self.xpn_path = xpn_path
        self.stems_path = stems_path
        self.report = ScoreReport(pack_path=xpn_path, stems_path=stems_path)

    # ------------------------------------------------------------------
    # Entry point
    # ------------------------------------------------------------------

    def run(self) -> ScoreReport:
        try:
            with zipfile.ZipFile(self.xpn_path, 'r') as zf:
                names = zf.namelist()
                self._score_technical(zf, names)
                self._score_documentation(zf, names)
        except zipfile.BadZipFile:
            self.report.errors.append(f"Not a valid ZIP/XPN file: {self.xpn_path}")
        except FileNotFoundError:
            self.report.errors.append(f"File not found: {self.xpn_path}")

        self._build_human_checklist()
        self._compute_totals()
        return self.report

    # ------------------------------------------------------------------
    # Technical (25 pts)
    # ------------------------------------------------------------------

    def _score_technical(self, zf: zipfile.ZipFile, names: list):
        wav_files = [n for n in names if n.lower().endswith('.wav')]

        if not wav_files:
            # All technical criteria score 0 if no WAVs found
            for crit_name, max_pts in [
                ('Sample Rate Compliance', 5),
                ('Bit Depth Compliance', 5),
                ('No Clipping', 5),
                ('Consistent Levels Across Kit', 5),
                ('No Silent or Dead Samples', 5),
            ]:
                self.report.criteria.append(CriterionResult(
                    name=crit_name, category='Technical',
                    max_pts=max_pts, scored_pts=0, automated=True,
                    notes='No WAV files found in pack.',
                ))
            return

        # Load all WAV data once
        wav_data = {}
        for name in wav_files:
            try:
                wav_data[name] = zf.read(name)
            except Exception as e:
                self.report.errors.append(f"Could not read {name}: {e}")

        self._check_sample_rate(wav_data)
        self._check_bit_depth(wav_data)
        self._check_clipping(wav_data)
        self._check_level_consistency(wav_data)
        self._check_silent_samples(wav_data)

    def _check_sample_rate(self, wav_data: dict):
        bad = []
        for name, data in wav_data.items():
            fmt = _read_wav_header(data)
            if fmt is None:
                bad.append(f"{name} (unreadable)")
            elif fmt['sample_rate'] not in self.VALID_SAMPLE_RATES:
                bad.append(f"{name} ({fmt['sample_rate']} Hz)")

        score = 5 if not bad else 0
        notes = 'All files compliant.' if not bad else \
            f"{len(bad)} non-compliant file(s): {', '.join(bad[:5])}" + \
            (' ...' if len(bad) > 5 else '')
        self.report.criteria.append(CriterionResult(
            name='Sample Rate Compliance', category='Technical',
            max_pts=5, scored_pts=score, automated=True, notes=notes,
        ))

    def _check_bit_depth(self, wav_data: dict):
        bad = []
        for name, data in wav_data.items():
            fmt = _read_wav_header(data)
            if fmt is None:
                bad.append(f"{name} (unreadable)")
            elif fmt['audio_format'] not in (1,):  # must be integer PCM
                bad.append(f"{name} (format={fmt['audio_format']}, not integer PCM)")
            elif fmt['bit_depth'] not in self.VALID_BIT_DEPTHS:
                bad.append(f"{name} ({fmt['bit_depth']}-bit)")

        score = 5 if not bad else 0
        notes = 'All files compliant.' if not bad else \
            f"{len(bad)} non-compliant file(s): {', '.join(bad[:5])}" + \
            (' ...' if len(bad) > 5 else '')
        self.report.criteria.append(CriterionResult(
            name='Bit Depth Compliance', category='Technical',
            max_pts=5, scored_pts=score, automated=True, notes=notes,
        ))

    def _check_clipping(self, wav_data: dict):
        clipping = []
        for name, data in wav_data.items():
            samples = _read_wav_samples(data)
            if samples is None:
                continue
            if _peak_dbfs(samples) >= self.CLIP_THRESHOLD_DBFS:
                clipping.append(name)

        n = len(clipping)
        if n == 0:
            score, notes = 5, 'Zero files clipping.'
        elif n <= 2:
            score = 2
            notes = f"{n} file(s) at or above {self.CLIP_THRESHOLD_DBFS} dBFS: {', '.join(clipping)}"
        else:
            score = 0
            notes = f"{n} file(s) clipping: {', '.join(clipping[:5])}" + (' ...' if n > 5 else '')

        self.report.criteria.append(CriterionResult(
            name='No Clipping', category='Technical',
            max_pts=5, scored_pts=score, automated=True, notes=notes,
        ))

    def _check_level_consistency(self, wav_data: dict):
        rms_vals = {}
        for name, data in wav_data.items():
            samples = _read_wav_samples(data)
            if samples is None:
                continue
            rms = _rms_dbfs(samples)
            if rms > -math.inf:
                rms_vals[name] = rms

        if len(rms_vals) < 2:
            self.report.criteria.append(CriterionResult(
                name='Consistent Levels Across Kit', category='Technical',
                max_pts=5, scored_pts=5, automated=True,
                notes='Fewer than 2 measurable files — consistency check skipped (full score awarded).',
            ))
            return

        rms_list = list(rms_vals.values())
        spread = max(rms_list) - min(rms_list)
        loudest = max(rms_vals, key=rms_vals.get)
        quietest = min(rms_vals, key=rms_vals.get)

        if spread <= self.LEVEL_SPREAD_TIGHT_DB:
            score = 5
            notes = f"Spread {spread:.1f} dB (within ±6 dB). Loudest: {loudest} ({rms_vals[loudest]:.1f} dBFS), Quietest: {quietest} ({rms_vals[quietest]:.1f} dBFS)."
        elif spread <= self.LEVEL_SPREAD_WIDE_DB:
            score = 2
            notes = f"Spread {spread:.1f} dB (±6–9 dB). Loudest: {loudest} ({rms_vals[loudest]:.1f} dBFS), Quietest: {quietest} ({rms_vals[quietest]:.1f} dBFS)."
        else:
            score = 0
            notes = f"Spread {spread:.1f} dB (exceeds ±9 dB). Loudest: {loudest} ({rms_vals[loudest]:.1f} dBFS), Quietest: {quietest} ({rms_vals[quietest]:.1f} dBFS)."

        self.report.criteria.append(CriterionResult(
            name='Consistent Levels Across Kit', category='Technical',
            max_pts=5, scored_pts=score, automated=True, notes=notes,
        ))

    def _check_silent_samples(self, wav_data: dict):
        dead = []
        for name, data in wav_data.items():
            samples = _read_wav_samples(data)
            if samples is None:
                continue
            if _rms_dbfs(samples) <= self.DEAD_THRESHOLD_DBFS:
                dead.append(name)

        score = 5 if not dead else 0
        notes = 'No dead/silent files.' if not dead else \
            f"{len(dead)} dead file(s) below {self.DEAD_THRESHOLD_DBFS} dBFS RMS: {', '.join(dead[:5])}" + \
            (' ...' if len(dead) > 5 else '')
        self.report.criteria.append(CriterionResult(
            name='No Silent or Dead Samples', category='Technical',
            max_pts=5, scored_pts=score, automated=True, notes=notes,
        ))

    # ------------------------------------------------------------------
    # Documentation (15 pts)
    # ------------------------------------------------------------------

    def _score_documentation(self, zf: zipfile.ZipFile, names: list):
        self._check_pack_yaml(zf, names)
        self._check_cover_art(zf, names)
        self._check_readme(zf, names)

    def _check_pack_yaml(self, zf: zipfile.ZipFile, names: list):
        yaml_candidates = [n for n in names if Path(n).name == 'pack.yaml']
        if not yaml_candidates:
            self.report.criteria.append(CriterionResult(
                name='pack.yaml Complete', category='Documentation',
                max_pts=5, scored_pts=0, automated=True,
                notes='pack.yaml not found in pack.',
            ))
            return

        try:
            text = zf.read(yaml_candidates[0]).decode('utf-8', errors='replace')
        except Exception as e:
            self.report.criteria.append(CriterionResult(
                name='pack.yaml Complete', category='Documentation',
                max_pts=5, scored_pts=0, automated=True,
                notes=f'Could not read pack.yaml: {e}',
            ))
            return

        fields = _parse_yaml_fields(text)
        score, missing = _yaml_field_score(fields)

        # Bonus check: coupling_suggestions non-empty (used in Uniqueness human note)
        coupling_ok = _yaml_coupling_valid(fields)
        if not coupling_ok and 'coupling_suggestions' not in [m for m in missing]:
            # Field present but empty — counts as missing for Uniqueness rubric
            self.report.errors.append(
                "coupling_suggestions field is present but empty — "
                "Uniqueness criterion 'Coupling Documentation Present' will score 0."
            )

        notes = 'All required fields present.' if not missing else \
            f"Missing/empty field(s): {', '.join(missing)}"
        self.report.criteria.append(CriterionResult(
            name='pack.yaml Complete', category='Documentation',
            max_pts=5, scored_pts=score, automated=True, notes=notes,
        ))

    def _check_cover_art(self, zf: zipfile.ZipFile, names: list):
        art_files = [
            n for n in names
            if Path(n).suffix.lower() in ('.png', '.jpg', '.jpeg')
            and (str(Path(n).parent) in ('', '.'))
        ]
        # Also accept any PNG/JPG at any depth whose stem contains 'cover' or 'art'
        art_files += [
            n for n in names
            if Path(n).suffix.lower() in ('.png', '.jpg', '.jpeg')
            and any(kw in Path(n).stem.lower() for kw in ('cover', 'art', 'artwork'))
            and n not in art_files
        ]
        # De-dup
        art_files = list(dict.fromkeys(art_files))

        if not art_files:
            self.report.criteria.append(CriterionResult(
                name='Cover Art Present', category='Documentation',
                max_pts=5, scored_pts=0, automated=True,
                notes='No PNG/JPG found at pack root or matching cover/art naming.',
            ))
            return

        # Check dimensions of the first qualifying image
        best_name = art_files[0]
        try:
            img_data = zf.read(best_name)
            dims = _image_dimensions(img_data)
        except Exception as e:
            self.report.criteria.append(CriterionResult(
                name='Cover Art Present', category='Documentation',
                max_pts=5, scored_pts=2, automated=True,
                notes=f'Art file found ({best_name}) but could not read dimensions: {e}',
            ))
            return

        if dims is None:
            notes = f'Art file found ({best_name}) but dimensions unreadable — manual verify required.'
            score = 2
        else:
            w, h = dims
            min_dim = self.COVER_ART_MIN_DIM
            if w >= min_dim and h >= min_dim:
                score = 5
                notes = f'{best_name} — {w}×{h} px (meets ≥{min_dim}×{min_dim} automated threshold). ' \
                        f'Note: brand standard requires 1000×1000 and on-brand art — human review required for full 5 pts.'
            else:
                score = 0
                notes = f'{best_name} — {w}×{h} px (below {min_dim}×{min_dim} minimum).'

        self.report.criteria.append(CriterionResult(
            name='Cover Art Present', category='Documentation',
            max_pts=5, scored_pts=score, automated=True, notes=notes,
        ))

    def _check_readme(self, zf: zipfile.ZipFile, names: list):
        readme_names = {'MPCE_SETUP.md', 'README.md', 'readme.md', 'Readme.md'}
        found = [n for n in names if Path(n).name in readme_names]

        if not found:
            self.report.criteria.append(CriterionResult(
                name='README / MPCE_SETUP.md Present', category='Documentation',
                max_pts=5, scored_pts=0, automated=True,
                notes='Neither MPCE_SETUP.md nor README.md found.',
            ))
            return

        try:
            text = zf.read(found[0]).decode('utf-8', errors='replace')
        except Exception as e:
            self.report.criteria.append(CriterionResult(
                name='README / MPCE_SETUP.md Present', category='Documentation',
                max_pts=5, scored_pts=0, automated=True,
                notes=f'Found {found[0]} but could not read it: {e}',
            ))
            return

        word_count = len(text.split())
        if word_count >= 100:
            score = 5
            notes = f'{found[0]} present — {word_count} words (substantive).'
        else:
            score = 2
            notes = f'{found[0]} present but skeletal — only {word_count} words (< 100 required).'

        self.report.criteria.append(CriterionResult(
            name='README / MPCE_SETUP.md Present', category='Documentation',
            max_pts=5, scored_pts=score, automated=True, notes=notes,
        ))

    # ------------------------------------------------------------------
    # Human review checklist (60 pts)
    # ------------------------------------------------------------------

    def _build_human_checklist(self):
        self.report.human_checklist = [
            {
                'category': 'Playability',
                'criteria': [
                    {'name': 'Velocity Response Is Musically Useful',
                     'pts': 10,
                     'guidance': 'Play each pad pp→ff. Must produce audible timbre shift or level arc. '
                                 'All pads musical: 10 pts. Dead zones on 1-2 pads: 6 pts. Flat on 3+: 0 pts.'},
                    {'name': 'All 16 Pads Respond Meaningfully',
                     'pts': 5,
                     'guidance': 'Every assigned pad triggers a distinct, intentional sound. '
                                 'All intentional: 5 pts. 1-2 weak/duplicate: 2 pts. 3+: 0 pts.'},
                    {'name': 'Loop Points Clean',
                     'pts': 5,
                     'guidance': 'Looped samples only. Audition at slow tempo — zero clicks/thumps. '
                                 'All clean: 5 pts. One click: 2 pts. Two+: 0 pts. N/A (one-shots): full 5 pts.'},
                    {'name': 'One-Shot Samples Decay Naturally',
                     'pts': 5,
                     'guidance': 'Final 10% of file must be envelope decay, not a hard cut. '
                                 'All clean: 5 pts. 1-2 truncated: 2 pts. 3+: 0 pts.'},
                ],
            },
            {
                'category': 'Sonic Identity',
                'criteria': [
                    {'name': 'Clear feliX-Oscar Placement',
                     'pts': 5,
                     'guidance': 'Overall character lands unambiguously on feliX-Oscar spectrum or documents deliberate tension. '
                                 'Clear + documented: 5 pts. Ambiguous but defensible: 2 pts. None: 0 pts.'},
                    {'name': 'Consistent Character Across Kit',
                     'pts': 5,
                     'guidance': 'Play full kit as a groove. All pads share same space + aesthetic logic. '
                                 'Coherent: 5 pts. 1-2 foreign pads: 2 pts. Incoherent: 0 pts.'},
                    {'name': 'DNA Tags Accurate',
                     'pts': 5,
                     'guidance': 'Cross-reference pack.yaml dna_tags against XPN Tools DNA vocabulary. '
                                 'All justified: 5 pts. 1 questionable: 3 pts. 2+ unjustified: 0 pts.'},
                    {'name': 'Engine Character Preserved in Renders',
                     'pts': 5,
                     'guidance': 'Compare renders against live engine at identical parameters. '
                                 'Renders faithful: 5 pts. Minor loss: 2 pts. Unrecognizable: 0 pts.'},
                ],
            },
            {
                'category': 'Uniqueness',
                'criteria': [
                    {'name': 'Not Duplicating Existing XO_OX Packs',
                     'pts': 5,
                     'guidance': 'Review against full XO_OX catalog. Must offer distinct tonal or rhythmic territory. '
                                 'Clearly distinct: 5 pts. Adjacent but defensible: 2 pts. Duplicate: 0 pts.'},
                    {'name': 'At Least 1 Signature Moment Preset',
                     'pts': 5,
                     'guidance': 'One preset must be immediately striking — demo-ready, stops a producer mid-scroll. '
                                 'Signature present: 5 pts. Solid but no standout: 2 pts. Nothing memorable: 0 pts.'},
                    {'name': 'Coupling Documentation Present',
                     'pts': 5,
                     'guidance': 'pack.yaml must list at least one coupling_suggestions entry pointing to a valid XOmnibus engine. '
                                 'Valid coupling: 5 pts. Field present but empty: 0 pts.'},
                ],
            },
        ]

    # ------------------------------------------------------------------
    # Totals + verdict
    # ------------------------------------------------------------------

    def _compute_totals(self):
        automated_total = sum(c.scored_pts for c in self.report.criteria if c.automated)
        automated_max = sum(c.max_pts for c in self.report.criteria if c.automated)
        self.report.automated_total = automated_total
        self.report.automated_max = automated_max

        # Extrapolate: assume human-review criteria score proportionally the same
        # This is a conservative signal (not a guarantee), used for gate guidance only.
        if automated_max > 0:
            auto_pct = automated_total / automated_max
        else:
            auto_pct = 0.0
        human_max = 60
        extrapolated = automated_total + (auto_pct * human_max)
        self.report.extrapolated_score = round(extrapolated, 1)

        if self.report.extrapolated_score >= 85:
            self.report.verdict = 'PASS (extrapolated)'
        elif self.report.extrapolated_score >= 70:
            self.report.verdict = 'CONDITIONAL (extrapolated)'
        else:
            self.report.verdict = 'FAIL (extrapolated)'


# ---------------------------------------------------------------------------
# CLI output
# ---------------------------------------------------------------------------

def _print_report(report: ScoreReport):
    W = 72
    print('=' * W)
    print(f"  XPN PACK SCORE REPORT")
    print(f"  Pack : {report.pack_path}")
    if report.stems_path:
        print(f"  Stems: {report.stems_path}")
    print('=' * W)

    if report.errors:
        print("\n[ERRORS]")
        for e in report.errors:
            print(f"  ! {e}")

    # Automated criteria grouped by category
    categories = {}
    for c in report.criteria:
        categories.setdefault(c.category, []).append(c)

    print("\n── AUTOMATED CRITERIA ──────────────────────────────────────────────")
    for cat, crits in categories.items():
        cat_total = sum(c.scored_pts for c in crits)
        cat_max = sum(c.max_pts for c in crits)
        print(f"\n  {cat.upper()}  [{cat_total}/{cat_max}]")
        for c in crits:
            bar = '✓' if c.scored_pts == c.max_pts else ('~' if c.scored_pts > 0 else '✗')
            print(f"    {bar} {c.name:<40}  {c.scored_pts:>2}/{c.max_pts}")
            if c.notes:
                # Wrap notes at 66 chars
                words = c.notes.split()
                line = '        '
                for w in words:
                    if len(line) + len(w) + 1 > 70:
                        print(line)
                        line = '        ' + w
                    else:
                        line += (' ' if line.strip() else '') + w
                if line.strip():
                    print(line)

    print(f"\n  AUTOMATED SUBTOTAL: {report.automated_total}/{report.automated_max} pts")
    print(f"  (Extrapolated full score assuming proportional human review: "
          f"{report.extrapolated_score}/100)")

    print("\n── HUMAN REVIEW CHECKLIST ─────────────────────────────────────────")
    print("  The following 60 points require human evaluation.\n")
    for section in report.human_checklist:
        sec_max = sum(c['pts'] for c in section['criteria'])
        print(f"  {section['category'].upper()}  [__/{sec_max}]")
        for c in section['criteria']:
            print(f"    [ ] {c['name']:<43}  __/{c['pts']}")
            # Print guidance wrapped
            words = c['guidance'].split()
            line = '        '
            for w in words:
                if len(line) + len(w) + 1 > 70:
                    print(line)
                    line = '        ' + w
                else:
                    line += (' ' if line.strip() else '') + w
            if line.strip():
                print(line)
            print()

    print("── VERDICT ─────────────────────────────────────────────────────────")
    print(f"  {report.verdict}")
    print()
    print("  Release gates:")
    print("    85–100 → PASS           (clear for release)")
    print("    70–84  → CONDITIONAL    (Patreon soft launch; fix flagged items first)")
    print("    0–69   → FAIL           (return to development)")
    print()
    print("  NOTE: Extrapolated score assumes human criteria scale with automated")
    print("  pass rate. Complete the human checklist above for the real score.")
    print('=' * W)


def _json_report(report: ScoreReport) -> str:
    d = {
        'pack_path': report.pack_path,
        'stems_path': report.stems_path,
        'automated_total': report.automated_total,
        'automated_max': report.automated_max,
        'extrapolated_score': report.extrapolated_score,
        'verdict': report.verdict,
        'errors': report.errors,
        'criteria': [
            {
                'name': c.name,
                'category': c.category,
                'max_pts': c.max_pts,
                'scored_pts': c.scored_pts,
                'automated': c.automated,
                'notes': c.notes,
            }
            for c in report.criteria
        ],
        'human_checklist': report.human_checklist,
    }
    return json.dumps(d, indent=2)


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main():
    parser = argparse.ArgumentParser(
        description='Automated XPN pack quality scorer (40-pt automated, 60-pt human checklist).',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    parser.add_argument('--xpn', required=True, metavar='PACK.XPN',
                        help='Path to the .xpn pack file (ZIP archive).')
    parser.add_argument('--stems', metavar='DIR',
                        help='Optional path to stems directory (reserved for future stem analysis).')
    parser.add_argument('--json', action='store_true',
                        help='Output machine-readable JSON instead of human-readable report.')
    args = parser.parse_args()

    scorer = XPNPackScorer(xpn_path=args.xpn, stems_path=args.stems)
    report = scorer.run()

    if args.json:
        print(_json_report(report))
    else:
        _print_report(report)

    # Exit code for CI: 0=PASS, 1=CONDITIONAL, 2=FAIL, 3=ERROR
    if report.errors and not report.criteria:
        sys.exit(3)
    verdict = report.verdict.upper()
    if 'PASS' in verdict and 'CONDITIONAL' not in verdict:
        sys.exit(0)
    elif 'CONDITIONAL' in verdict:
        sys.exit(1)
    else:
        sys.exit(2)


if __name__ == '__main__':
    main()
