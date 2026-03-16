#!/usr/bin/env python3
"""
xpn_stems_checker.py — XO_OX MPCe Quad-Corner Pack Readiness Validator
=======================================================================
Validates a directory of stem WAV files for MPC expansion pack readiness.

Two validation modes:
  1. Per-file audio quality checks (sample rate, bit depth, levels, clipping)
  2. Quad-corner group validation (NW/NE/SW/SE naming, matching rates, lengths)

CLI:
  python xpn_stems_checker.py --stems ./stems/ --output report.json

For stems frequency-separation analysis (loop packs), see the companion spec
in Docs/specs/mpce_native_pack_design.md Section 2.3.

Author: Rex (XO_OX Tooling)
Date:   2026-03-16
"""

import argparse
import json
import math
import os
import struct
import sys
import wave
from dataclasses import dataclass, field, asdict
from datetime import datetime
from pathlib import Path
from typing import Dict, List, Optional, Tuple


# ---------------------------------------------------------------------------
# Constants
# ---------------------------------------------------------------------------

# Quad corner suffixes (NW/NE/SW/SE) — matched case-insensitively
CORNER_SUFFIXES = ["nw", "ne", "sw", "se"]

# Accepted sample rates per XPN pack spec
VALID_SAMPLE_RATES = {44100, 48000}

# Accepted bit depths per XPN pack spec
VALID_BIT_DEPTHS = {16, 24}

# RMS window size in samples (used for block-RMS analysis)
RMS_WINDOW = 4096

# Clipping threshold — any sample >= this absolute value (normalised 0..1) is clipped
CLIP_THRESHOLD = 0.9999

# Quad length tolerance: corners may differ by at most this fraction of the longest
QUAD_LENGTH_TOLERANCE = 0.05  # 5 %

# Scoring penalties (deducted from 100 per quad)
PENALTY_BAD_SAMPLE_RATE = 15
PENALTY_BAD_BIT_DEPTH = 10
PENALTY_RATE_MISMATCH = 20
PENALTY_LENGTH_MISMATCH = 15
PENALTY_MONO_IN_QUAD = 5       # advisory, not blocking
PENALTY_CLIPPING = 20
PENALTY_LOW_RMS = 10            # < -24 dBFS average
PENALTY_HIGH_PEAK = 5           # peak > -0.5 dBFS but not clipping


# ---------------------------------------------------------------------------
# Data structures
# ---------------------------------------------------------------------------

@dataclass
class WavInfo:
    """Raw WAV metadata extracted without third-party libraries."""
    path: str
    filename: str
    sample_rate: int
    bit_depth: int
    channels: int          # 1 = mono, 2 = stereo
    num_frames: int
    duration_sec: float

    # Level analysis
    rms_dbfs: float = 0.0
    peak_dbfs: float = 0.0
    dynamic_range_db: float = 0.0  # peak - RMS (higher = more dynamic)
    clipping_detected: bool = False
    clipping_count: int = 0        # number of clipped samples

    # Pass/fail
    issues: List[str] = field(default_factory=list)
    warnings: List[str] = field(default_factory=list)
    passed: bool = True


@dataclass
class QuadGroup:
    """A set of four corner files sharing the same base name."""
    base_name: str          # e.g. "kick" from kick_nw.wav
    corners: Dict[str, Optional[WavInfo]] = field(default_factory=dict)
    # corners keys: "nw", "ne", "sw", "se"

    issues: List[str] = field(default_factory=list)
    warnings: List[str] = field(default_factory=list)
    score: int = 100
    score_label: str = "UNKNOWN"
    passed: bool = True


@dataclass
class PackReport:
    """Top-level report for the entire stems directory."""
    analysis_date: str
    stems_dir: str
    total_wav_files: int
    total_quads_found: int
    orphan_files: List[str]   # WAVs that don't belong to any complete quad

    file_results: List[dict]
    quad_results: List[dict]

    overall_score: int = 0
    overall_label: str = "UNKNOWN"
    pack_ready: bool = False


# ---------------------------------------------------------------------------
# WAV parsing (pure stdlib — no numpy/scipy required)
# ---------------------------------------------------------------------------

def _read_wav_samples_as_float(wav_path: str) -> Tuple[List[float], int, int, int]:
    """
    Read all samples from a WAV file as normalised floats in [-1.0, 1.0].

    Returns: (samples_flat, sample_rate, bit_depth, channels)

    Supports 8, 16, 24, and 32-bit PCM WAV.
    Only reads the first channel pair to avoid excessive memory use for long files.
    For level analysis a representative mono downmix is used.
    """
    with wave.open(wav_path, "rb") as wf:
        nchannels = wf.getnchannels()
        sampwidth = wf.getsampwidth()     # bytes per sample
        framerate = wf.getframerate()
        nframes   = wf.getnframes()
        raw_bytes = wf.readframes(nframes)

    bit_depth = sampwidth * 8
    total_samples = nframes * nchannels

    # Unpack raw bytes into integers
    if sampwidth == 1:
        # 8-bit WAV is unsigned
        ints = list(struct.unpack(f"{total_samples}B", raw_bytes))
        floats = [(x - 128) / 128.0 for x in ints]
    elif sampwidth == 2:
        ints = list(struct.unpack(f"<{total_samples}h", raw_bytes))
        floats = [x / 32768.0 for x in ints]
    elif sampwidth == 3:
        # 24-bit: no native struct format — unpack 3 bytes at a time
        floats = []
        for i in range(0, len(raw_bytes), 3):
            b0, b1, b2 = raw_bytes[i], raw_bytes[i + 1], raw_bytes[i + 2]
            value = (b2 << 16) | (b1 << 8) | b0
            if value & 0x800000:          # sign extend
                value -= 0x1000000
            floats.append(value / 8388608.0)
    elif sampwidth == 4:
        ints = list(struct.unpack(f"<{total_samples}i", raw_bytes))
        floats = [x / 2147483648.0 for x in ints]
    else:
        raise ValueError(f"Unsupported bit depth: {sampwidth * 8}")

    # Downmix to mono for level analysis: average across channels
    if nchannels > 1:
        mono = []
        for frame_idx in range(nframes):
            frame_start = frame_idx * nchannels
            avg = sum(floats[frame_start:frame_start + nchannels]) / nchannels
            mono.append(avg)
    else:
        mono = floats

    return mono, framerate, bit_depth, nchannels


def _linear_to_dbfs(linear: float) -> float:
    """Convert a linear amplitude (0..1) to dBFS. Returns -inf for silence."""
    if linear <= 0.0:
        return -math.inf
    return 20.0 * math.log10(linear)


def _compute_rms(samples: List[float]) -> float:
    """Compute RMS of a float sample array. Returns linear amplitude."""
    if not samples:
        return 0.0
    sum_sq = sum(s * s for s in samples)
    return math.sqrt(sum_sq / len(samples))


def _compute_block_rms(samples: List[float], window: int = RMS_WINDOW) -> float:
    """
    Compute RMS over non-overlapping windows and return the average of block RMS values.
    This gives a more perceptually representative loudness estimate than whole-file RMS.
    """
    if not samples:
        return 0.0
    block_rms_values = []
    for start in range(0, len(samples), window):
        block = samples[start:start + window]
        block_rms_values.append(_compute_rms(block))
    return sum(block_rms_values) / len(block_rms_values)


def _analyze_levels(samples: List[float]) -> Tuple[float, float, bool, int]:
    """
    Analyse sample levels.

    Returns:
        rms_dbfs      — block-averaged RMS in dBFS
        peak_dbfs     — peak absolute amplitude in dBFS
        clipping      — True if any sample exceeds CLIP_THRESHOLD
        clip_count    — number of samples >= CLIP_THRESHOLD
    """
    rms_linear = _compute_block_rms(samples)
    peak_linear = max(abs(s) for s in samples) if samples else 0.0
    clip_count = sum(1 for s in samples if abs(s) >= CLIP_THRESHOLD)
    clipping = clip_count > 0

    rms_dbfs  = _linear_to_dbfs(rms_linear)
    peak_dbfs = _linear_to_dbfs(peak_linear)
    return rms_dbfs, peak_dbfs, clipping, clip_count


# ---------------------------------------------------------------------------
# Per-file validation
# ---------------------------------------------------------------------------

def validate_wav_file(wav_path: str) -> WavInfo:
    """
    Validate a single WAV file and return a WavInfo with all findings populated.
    """
    path_obj = Path(wav_path)
    info = WavInfo(
        path=str(path_obj.resolve()),
        filename=path_obj.name,
        sample_rate=0,
        bit_depth=0,
        channels=0,
        num_frames=0,
        duration_sec=0.0,
    )

    # --- Parse WAV header ---
    try:
        with wave.open(wav_path, "rb") as wf:
            info.sample_rate  = wf.getframerate()
            info.bit_depth    = wf.getsampwidth() * 8
            info.channels     = wf.getnchannels()
            info.num_frames   = wf.getnframes()
            info.duration_sec = info.num_frames / info.sample_rate if info.sample_rate else 0.0
    except Exception as exc:
        info.issues.append(f"UNREADABLE: could not open WAV — {exc}")
        info.passed = False
        return info

    # --- Sample rate check ---
    if info.sample_rate not in VALID_SAMPLE_RATES:
        info.issues.append(
            f"SAMPLE_RATE: {info.sample_rate} Hz is not allowed "
            f"(must be {' or '.join(str(r) for r in sorted(VALID_SAMPLE_RATES))})"
        )
        info.passed = False

    # --- Bit depth check ---
    if info.bit_depth not in VALID_BIT_DEPTHS:
        info.issues.append(
            f"BIT_DEPTH: {info.bit_depth}-bit is not allowed "
            f"(must be {' or '.join(str(d) for d in sorted(VALID_BIT_DEPTHS))})"
        )
        info.passed = False

    # --- Stereo / mono advisory ---
    if info.channels == 1:
        info.warnings.append("MONO: file is mono — stereo preferred for pack content")
    elif info.channels > 2:
        info.warnings.append(f"MULTICHANNEL: {info.channels} channels detected — only stereo/mono expected")

    # --- Level analysis ---
    try:
        samples, _, _, _ = _read_wav_samples_as_float(wav_path)
        rms_dbfs, peak_dbfs, clipping, clip_count = _analyze_levels(samples)

        info.rms_dbfs  = round(rms_dbfs, 2)
        info.peak_dbfs = round(peak_dbfs, 2)
        info.dynamic_range_db = round(peak_dbfs - rms_dbfs, 2) if not math.isinf(rms_dbfs) else 0.0
        info.clipping_detected = clipping
        info.clipping_count    = clip_count

        # Clipping is a hard failure
        if clipping:
            info.issues.append(
                f"CLIPPING: {clip_count} sample(s) at or above {CLIP_THRESHOLD:.4f} — "
                "reduce output level before exporting"
            )
            info.passed = False

        # Low RMS advisory (< -24 dBFS average)
        if not math.isinf(rms_dbfs) and rms_dbfs < -24.0:
            info.warnings.append(
                f"LOW_RMS: average RMS is {rms_dbfs:.1f} dBFS — "
                "consider normalising or increasing gain for pack content"
            )

        # Peak close to ceiling (advisory only)
        if not math.isinf(peak_dbfs) and peak_dbfs > -0.5 and not clipping:
            info.warnings.append(
                f"HOT_PEAK: peak is {peak_dbfs:.2f} dBFS — "
                "very close to 0 dBFS; leave at least -0.5 dB headroom"
            )

        # Silence detection
        if math.isinf(rms_dbfs):
            info.issues.append("SILENCE: file appears to be completely silent")
            info.passed = False

    except Exception as exc:
        info.warnings.append(f"LEVEL_ANALYSIS_FAILED: {exc}")

    return info


# ---------------------------------------------------------------------------
# Quad-corner grouping
# ---------------------------------------------------------------------------

def _extract_base_and_corner(filename: str) -> Tuple[Optional[str], Optional[str]]:
    """
    Given a WAV filename, try to extract a base name and corner suffix.

    Accepted patterns (case-insensitive):
      kick_nw.wav     → ("kick", "nw")
      SNARE-NE.wav    → ("snare", "ne")
      pad_soft_SW.wav → ("pad_soft", "sw")

    Returns (None, None) if the file does not match the quad-corner pattern.
    """
    stem = Path(filename).stem.lower()   # strip .wav, lowercase

    for corner in CORNER_SUFFIXES:
        # Match suffix preceded by _ or -
        for sep in ("_", "-"):
            if stem.endswith(sep + corner):
                base = stem[: -(len(sep) + len(corner))]
                if base:
                    return base, corner

    return None, None


def group_into_quads(wav_infos: List[WavInfo]) -> Tuple[Dict[str, QuadGroup], List[WavInfo]]:
    """
    Group WavInfo objects by quad base name.

    Returns:
        quads   — dict keyed by base name, each value a QuadGroup
        orphans — WavInfo objects that do not belong to a complete quad
    """
    # First pass: bucket every file by (base_name, corner)
    buckets: Dict[str, Dict[str, WavInfo]] = {}
    orphan_list: List[WavInfo] = []

    for wi in wav_infos:
        base, corner = _extract_base_and_corner(wi.filename)
        if base is None:
            orphan_list.append(wi)
            continue
        if base not in buckets:
            buckets[base] = {}
        buckets[base][corner] = wi

    # Second pass: groups with all 4 corners are quads; incomplete groups → orphans
    quads: Dict[str, QuadGroup] = {}
    for base, corner_map in buckets.items():
        missing = [c for c in CORNER_SUFFIXES if c not in corner_map]
        if missing:
            # Not a complete quad — treat files as orphans
            for wi in corner_map.values():
                orphan_list.append(wi)
        else:
            quads[base] = QuadGroup(
                base_name=base,
                corners={c: corner_map[c] for c in CORNER_SUFFIXES},
            )

    return quads, orphan_list


# ---------------------------------------------------------------------------
# Quad-level validation
# ---------------------------------------------------------------------------

def validate_quad(quad: QuadGroup) -> QuadGroup:
    """
    Run cross-corner consistency checks and compute a readiness score (0-100).
    Mutates quad in place and returns it.
    """
    score = 100
    corners = quad.corners  # dict: "nw"|"ne"|"sw"|"se" → WavInfo

    # Collect per-corner data for comparisons
    rates   = {c: wi.sample_rate  for c, wi in corners.items() if wi}
    lengths = {c: wi.num_frames   for c, wi in corners.items() if wi}
    depths  = {c: wi.bit_depth    for c, wi in corners.items() if wi}

    # --- Check individual file pass/fail ---
    for corner, wi in corners.items():
        if wi and not wi.passed:
            quad.issues.append(
                f"{corner.upper()} has per-file issues: "
                + "; ".join(wi.issues)
            )
            # Deduct score proportionally — each bad file costs points
            if wi.clipping_detected:
                score -= PENALTY_CLIPPING
            if wi.sample_rate not in VALID_SAMPLE_RATES:
                score -= PENALTY_BAD_SAMPLE_RATE
            if wi.bit_depth not in VALID_BIT_DEPTHS:
                score -= PENALTY_BAD_BIT_DEPTH

    # --- Sample rate consistency across all 4 corners ---
    unique_rates = set(rates.values())
    if len(unique_rates) > 1:
        quad.issues.append(
            f"RATE_MISMATCH: corners have different sample rates: "
            + ", ".join(f"{c.upper()}={r}Hz" for c, r in rates.items())
        )
        score -= PENALTY_RATE_MISMATCH

    # --- Bit depth consistency ---
    unique_depths = set(depths.values())
    if len(unique_depths) > 1:
        quad.warnings.append(
            f"DEPTH_MISMATCH: corners have different bit depths: "
            + ", ".join(f"{c.upper()}={d}bit" for c, d in depths.items())
        )
        score -= PENALTY_BAD_BIT_DEPTH

    # --- Frame length consistency (within 5%) ---
    if lengths:
        max_len = max(lengths.values())
        min_len = min(lengths.values())
        if max_len > 0:
            length_diff_ratio = (max_len - min_len) / max_len
            if length_diff_ratio > QUAD_LENGTH_TOLERANCE:
                pct = length_diff_ratio * 100
                quad.issues.append(
                    f"LENGTH_MISMATCH: corners differ in length by {pct:.1f}% "
                    f"(tolerance is {QUAD_LENGTH_TOLERANCE * 100:.0f}%) — "
                    + ", ".join(
                        f"{c.upper()}={n}" for c, n in lengths.items()
                    )
                )
                score -= PENALTY_LENGTH_MISMATCH

    # --- Mono corners advisory ---
    mono_corners = [c for c, wi in corners.items() if wi and wi.channels == 1]
    if mono_corners:
        quad.warnings.append(
            f"MONO_CORNERS: {', '.join(c.upper() for c in mono_corners)} "
            "are mono — stereo preferred for quad-corner packs"
        )
        score -= PENALTY_MONO_IN_QUAD * len(mono_corners)

    # --- Low RMS advisory (propagate from individual files) ---
    for corner, wi in corners.items():
        if wi and wi.warnings:
            for w in wi.warnings:
                if "LOW_RMS" in w:
                    quad.warnings.append(f"{corner.upper()}: {w}")
                    score -= PENALTY_LOW_RMS

    # Clamp score to valid range
    score = max(0, min(100, score))
    quad.score = score
    quad.score_label = _score_label(score)
    quad.passed = score >= 60 and not any("RATE_MISMATCH" in i or "LENGTH_MISMATCH" in i for i in quad.issues)

    return quad


def _score_label(score: int) -> str:
    """Map a numeric score (0-100) to a human-readable label."""
    if score >= 90:
        return "EXCELLENT"
    elif score >= 75:
        return "GOOD"
    elif score >= 60:
        return "ACCEPTABLE"
    elif score >= 40:
        return "POOR"
    else:
        return "FAIL"


# ---------------------------------------------------------------------------
# Overall pack score
# ---------------------------------------------------------------------------

def compute_pack_score(
    file_results: List[WavInfo],
    quad_results: List[QuadGroup],
    orphan_count: int,
) -> Tuple[int, str]:
    """
    Compute an overall pack readiness score.

    Weighting:
      - 60 % from average quad score (core concern)
      - 30 % from per-file pass rate
      - 10 % deduction if orphan files exist (incomplete quads)
    """
    # Quad component
    if quad_results:
        quad_avg = sum(q.score for q in quad_results) / len(quad_results)
    else:
        quad_avg = 0.0

    # File component
    if file_results:
        pass_rate = sum(1 for f in file_results if f.passed) / len(file_results)
    else:
        pass_rate = 0.0

    # Orphan penalty: up to -10 points if there are orphan files relative to total
    total = len(file_results) or 1
    orphan_penalty = min(10, int((orphan_count / total) * 10))

    score = int(0.6 * quad_avg + 0.3 * pass_rate * 100 - orphan_penalty)
    score = max(0, min(100, score))
    return score, _score_label(score)


# ---------------------------------------------------------------------------
# Scanning
# ---------------------------------------------------------------------------

def scan_stems_directory(stems_dir: str) -> List[WavInfo]:
    """
    Recursively find all WAV files in stems_dir and validate each one.
    Returns a list of WavInfo objects sorted by filename.
    """
    stems_path = Path(stems_dir)
    if not stems_path.exists():
        print(f"[ERROR] Directory not found: {stems_dir}", file=sys.stderr)
        sys.exit(1)

    wav_paths = sorted(stems_path.rglob("*.wav")) + sorted(stems_path.rglob("*.WAV"))
    # deduplicate (rglob may return both on case-insensitive FS)
    seen = set()
    unique_wavs = []
    for p in wav_paths:
        resolved = str(p.resolve())
        if resolved not in seen:
            seen.add(resolved)
            unique_wavs.append(p)

    results = []
    for wav_path in unique_wavs:
        info = validate_wav_file(str(wav_path))
        results.append(info)

    return results


# ---------------------------------------------------------------------------
# Console reporting
# ---------------------------------------------------------------------------

_PASS  = "PASS"
_FAIL  = "FAIL"
_WARN  = "WARN"

def _fmt_bool(b: bool) -> str:
    return _PASS if b else _FAIL

def _fmt_dbfs(v: float) -> str:
    if math.isinf(v):
        return "-inf dBFS"
    return f"{v:.1f} dBFS"


def print_console_report(
    file_results: List[WavInfo],
    quads: Dict[str, QuadGroup],
    orphans: List[WavInfo],
    overall_score: int,
    overall_label: str,
) -> None:
    """Print a human-readable report to stdout."""
    sep_major = "=" * 72
    sep_minor = "-" * 72

    print()
    print(sep_major)
    print("  XO_OX XPN STEMS CHECKER — MPCe Quad-Corner Pack Readiness Report")
    print(f"  {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
    print(sep_major)

    # ---- Per-file summary ----
    print()
    print("  PER-FILE ANALYSIS")
    print(sep_minor)

    for fi in file_results:
        status = _fmt_bool(fi.passed)
        stereo_label = {1: "MONO", 2: "STEREO"}.get(fi.channels, f"{fi.channels}CH")
        print(
            f"  [{status:4s}] {fi.filename:<40s} "
            f"{fi.sample_rate}Hz / {fi.bit_depth}bit / {stereo_label} / "
            f"{fi.duration_sec:.2f}s"
        )
        print(
            f"         RMS: {_fmt_dbfs(fi.rms_dbfs):>12s}  "
            f"Peak: {_fmt_dbfs(fi.peak_dbfs):>12s}  "
            f"DR: {fi.dynamic_range_db:>5.1f} dB  "
            f"Clip: {'YES ⚠' if fi.clipping_detected else 'No'}"
        )
        for issue in fi.issues:
            print(f"         [ISSUE] {issue}")
        for warn in fi.warnings:
            print(f"         [WARN]  {warn}")

    # ---- Quad group summary ----
    print()
    print("  QUAD-CORNER GROUP VALIDATION")
    print(sep_minor)

    if not quads:
        print("  No complete quad groups found.")
        print("  Ensure files follow the NW/NE/SW/SE naming convention:")
        print("    e.g. kick_nw.wav, kick_ne.wav, kick_sw.wav, kick_se.wav")
    else:
        for base, quad in sorted(quads.items()):
            status = _fmt_bool(quad.passed)
            print(
                f"  [{status:4s}] {base:<30s} Score: {quad.score:>3d}/100  [{quad.score_label}]"
            )
            # Show corner file names
            for corner in CORNER_SUFFIXES:
                wi = quad.corners.get(corner)
                if wi:
                    print(f"           {corner.upper()}: {wi.filename}")
            for issue in quad.issues:
                print(f"         [ISSUE] {issue}")
            for warn in quad.warnings:
                print(f"         [WARN]  {warn}")

    # ---- Orphan files ----
    if orphans:
        print()
        print("  ORPHAN FILES (not part of any complete quad)")
        print(sep_minor)
        for wi in orphans:
            print(f"  {wi.filename}")
        print()
        print("  Orphan files will not be included in quad-corner programs.")
        print("  Rename to include _nw/_ne/_sw/_se suffix to join a quad group.")

    # ---- Overall score ----
    print()
    print(sep_major)
    total_files = len(file_results)
    passed_files = sum(1 for f in file_results if f.passed)
    print(f"  FILES:   {passed_files}/{total_files} passed individual checks")
    print(f"  QUADS:   {len(quads)} complete groups found  ({len(orphans)} orphan file(s))")
    print(f"  SCORE:   {overall_score}/100  [{overall_label}]")

    if overall_score >= 90:
        print("  VERDICT: Pack is ready for MPCe release.")
    elif overall_score >= 75:
        print("  VERDICT: Pack is GOOD — address warnings before shipping.")
    elif overall_score >= 60:
        print("  VERDICT: Pack is ACCEPTABLE — fix issues before claiming MPCe-native badge.")
    elif overall_score >= 40:
        print("  VERDICT: Pack is POOR — significant issues must be resolved.")
    else:
        print("  VERDICT: FAIL — do not ship until critical issues are resolved.")

    print(sep_major)
    print()


# ---------------------------------------------------------------------------
# JSON serialisation helpers
# ---------------------------------------------------------------------------

def _wav_info_to_dict(wi: WavInfo) -> dict:
    return {
        "filename": wi.filename,
        "path": wi.path,
        "sample_rate": wi.sample_rate,
        "bit_depth": wi.bit_depth,
        "channels": wi.channels,
        "num_frames": wi.num_frames,
        "duration_sec": wi.duration_sec,
        "rms_dbfs": wi.rms_dbfs if not math.isinf(wi.rms_dbfs) else None,
        "peak_dbfs": wi.peak_dbfs if not math.isinf(wi.peak_dbfs) else None,
        "dynamic_range_db": wi.dynamic_range_db,
        "clipping_detected": wi.clipping_detected,
        "clipping_count": wi.clipping_count,
        "issues": wi.issues,
        "warnings": wi.warnings,
        "passed": wi.passed,
    }


def _quad_to_dict(quad: QuadGroup) -> dict:
    corners_out = {}
    for corner in CORNER_SUFFIXES:
        wi = quad.corners.get(corner)
        corners_out[corner] = wi.filename if wi else None
    return {
        "base_name": quad.base_name,
        "corners": corners_out,
        "score": quad.score,
        "score_label": quad.score_label,
        "passed": quad.passed,
        "issues": quad.issues,
        "warnings": quad.warnings,
    }


def build_json_report(
    stems_dir: str,
    file_results: List[WavInfo],
    quads: Dict[str, QuadGroup],
    orphans: List[WavInfo],
    overall_score: int,
    overall_label: str,
) -> dict:
    return {
        "analysis_date": datetime.now().strftime("%Y-%m-%d"),
        "stems_dir": str(Path(stems_dir).resolve()),
        "total_wav_files": len(file_results),
        "total_quads_found": len(quads),
        "orphan_files": [wi.filename for wi in orphans],
        "file_results": [_wav_info_to_dict(wi) for wi in file_results],
        "quad_results": [_quad_to_dict(q) for q in quads.values()],
        "overall_score": overall_score,
        "overall_label": overall_label,
        "pack_ready": overall_score >= 75,
    }


# ---------------------------------------------------------------------------
# CLI entry point
# ---------------------------------------------------------------------------

def main() -> None:
    parser = argparse.ArgumentParser(
        description=(
            "xpn_stems_checker.py — Validate WAV stem files for "
            "MPCe quad-corner pack readiness."
        )
    )
    parser.add_argument(
        "--stems",
        required=True,
        metavar="DIR",
        help="Directory containing stem WAV files to validate.",
    )
    parser.add_argument(
        "--output",
        metavar="FILE",
        default=None,
        help="Optional path for JSON report output (e.g. report.json).",
    )
    parser.add_argument(
        "--quiet",
        action="store_true",
        help="Suppress console output (useful when only the JSON report is needed).",
    )

    args = parser.parse_args()

    # --- Scan and validate ---
    print(f"Scanning: {args.stems}", file=sys.stderr)
    file_results = scan_stems_directory(args.stems)

    if not file_results:
        print("[ERROR] No WAV files found in the specified directory.", file=sys.stderr)
        sys.exit(1)

    print(f"Found {len(file_results)} WAV file(s). Analysing...", file=sys.stderr)

    # --- Group into quads ---
    quads, orphans = group_into_quads(file_results)

    # --- Validate quads ---
    for quad in quads.values():
        validate_quad(quad)

    # --- Overall score ---
    overall_score, overall_label = compute_pack_score(
        file_results, list(quads.values()), len(orphans)
    )

    # --- Console output ---
    if not args.quiet:
        print_console_report(
            file_results, quads, orphans, overall_score, overall_label
        )

    # --- JSON output ---
    if args.output:
        report = build_json_report(
            args.stems, file_results, quads, orphans, overall_score, overall_label
        )
        output_path = Path(args.output)
        output_path.parent.mkdir(parents=True, exist_ok=True)
        with open(output_path, "w", encoding="utf-8") as fh:
            json.dump(report, fh, indent=2)
        print(f"JSON report written to: {output_path}", file=sys.stderr)

    # Exit with non-zero code if pack is not ready (useful for CI/pipeline gating)
    sys.exit(0 if overall_score >= 75 else 1)


if __name__ == "__main__":
    main()
