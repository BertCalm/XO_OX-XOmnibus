#!/usr/bin/env python3
"""
XPN Pack Release Checklist — XO_OX Designs
Runs the 15-point pre-release checklist for a .xpn pack and generates a
formatted report combining automated checks with manual placeholders.

Usage:
    python xpn_pack_release_checklist.py <pack.xpn> [--tier signal|form|doctrine] \
        [--format text|markdown] [--output FILE]

Tiers and pack_score thresholds:
    signal   >= 70
    form     >= 80  (default)
    doctrine >= 90
"""

import argparse
import json
import math
import os
import re
import struct
import sys
import zipfile
from dataclasses import dataclass, field
from pathlib import Path
from typing import List, Optional, Tuple
from xml.etree import ElementTree as ET


# ---------------------------------------------------------------------------
# Constants
# ---------------------------------------------------------------------------

TIER_THRESHOLDS = {"signal": 70, "form": 80, "doctrine": 90}
CLIPPING_THRESHOLD_DBFS = -0.5          # peaks above this flag as clipping
CLIPPING_LINEAR = 10 ** (CLIPPING_THRESHOLD_DBFS / 20.0)  # ~0.9441
COVER_MIN_PX = 400
PROGRAM_NAME_MAX = 20
TUNING_TOLERANCE_CENTS = 50.0
VERSION_RE = re.compile(r"^\d+\.\d+\.\d+$")

PASS  = "PASS"
FAIL  = "FAIL"
SKIP  = "SKIP"   # could not verify (missing data)


# ---------------------------------------------------------------------------
# Result model
# ---------------------------------------------------------------------------

@dataclass
class CheckResult:
    number: int
    description: str
    status: str          # PASS / FAIL / SKIP
    detail: str = ""     # extra detail for the report line


# ---------------------------------------------------------------------------
# WAV helpers (stdlib only — no soundfile/scipy)
# ---------------------------------------------------------------------------

def _wav_peak(data: bytes) -> Optional[float]:
    """Return the absolute peak sample value (0.0–1.0) from raw WAV bytes."""
    try:
        if data[:4] != b"RIFF" or data[8:12] != b"WAVE":
            return None
        offset = 12
        fmt_channels = fmt_sampwidth = fmt_samplerate = None
        data_offset = data_size = None
        while offset + 8 <= len(data):
            chunk_id = data[offset:offset+4]
            chunk_size = struct.unpack_from("<I", data, offset+4)[0]
            if chunk_id == b"fmt ":
                audio_format = struct.unpack_from("<H", data, offset+8)[0]
                if audio_format not in (1, 3):   # PCM or IEEE float
                    return None
                fmt_channels  = struct.unpack_from("<H", data, offset+10)[0]
                fmt_sampwidth  = struct.unpack_from("<H", data, offset+22)[0]  # bits
            elif chunk_id == b"data":
                data_offset = offset + 8
                data_size   = chunk_size
                break
            offset += 8 + chunk_size + (chunk_size & 1)
        if fmt_sampwidth is None or data_offset is None or data_size == 0:
            return None
        raw = data[data_offset:data_offset + data_size]
        peak = 0.0
        if fmt_sampwidth == 16:
            n = len(raw) // 2
            samples = struct.unpack_from(f"<{n}h", raw, 0)
            peak = max(abs(s) / 32768.0 for s in samples) if samples else 0.0
        elif fmt_sampwidth == 24:
            peak = 0.0
            for i in range(0, len(raw) - 2, 3):
                val = struct.unpack_from("<i", raw[i:i+3] + (b"\xff" if raw[i+2] & 0x80 else b"\x00"))[0] >> 8
                peak = max(peak, abs(val) / 8388608.0)
        elif fmt_sampwidth == 32:
            # could be float or int — detect by audio_format (already checked above)
            n = len(raw) // 4
            try:
                samples = struct.unpack_from(f"<{n}f", raw, 0)
                peak = max(abs(s) for s in samples) if samples else 0.0
            except Exception:
                samples = struct.unpack_from(f"<{n}i", raw, 0)
                peak = max(abs(s) / 2147483648.0 for s in samples) if samples else 0.0
        else:
            return None
        return peak
    except Exception:
        return None


def _wav_is_silent(data: bytes) -> Optional[bool]:
    """Return True if the WAV contains only silence (all zeros)."""
    peak = _wav_peak(data)
    if peak is None:
        return None
    return peak < 1e-6


# ---------------------------------------------------------------------------
# PNG dimension helper
# ---------------------------------------------------------------------------

def _png_dimensions(data: bytes) -> Optional[Tuple[int, int]]:
    """Return (width, height) from PNG header bytes, or None."""
    try:
        if data[:8] != b"\x89PNG\r\n\x1a\n":
            return None
        if data[12:16] != b"IHDR":
            return None
        width  = struct.unpack_from(">I", data, 16)[0]
        height = struct.unpack_from(">I", data, 20)[0]
        return width, height
    except Exception:
        return None


def _jpg_dimensions(data: bytes) -> Optional[Tuple[int, int]]:
    """Return (width, height) from JPEG SOF marker, or None."""
    try:
        if data[:2] != b"\xff\xd8":
            return None
        offset = 2
        while offset + 4 < len(data):
            if data[offset] != 0xFF:
                break
            marker = data[offset+1]
            length = struct.unpack_from(">H", data, offset+2)[0]
            if marker in (0xC0, 0xC1, 0xC2):
                height = struct.unpack_from(">H", data, offset+5)[0]
                width  = struct.unpack_from(">H", data, offset+7)[0]
                return width, height
            offset += 2 + length
        return None
    except Exception:
        return None


# ---------------------------------------------------------------------------
# XPM XML helpers
# ---------------------------------------------------------------------------

def _parse_xpm(data: bytes) -> Optional[ET.Element]:
    try:
        return ET.fromstring(data)
    except ET.ParseError:
        return None


def _get_layers(root: ET.Element) -> List[ET.Element]:
    """Return all Layer elements from an XPM tree."""
    return root.findall(".//Layer")


# ---------------------------------------------------------------------------
# pack_score calculation  (simplified heuristic matching xpn_qa_checker.py style)
# ---------------------------------------------------------------------------

def _compute_pack_score(
    expansion: dict,
    manifest: dict,
    programs: List[ET.Element],
    sample_peaks: dict,  # path -> float peak
) -> float:
    """
    Quick heuristic pack_score [0–100]:
      - 30 pts: expansion.json completeness
      - 20 pts: sonic_dna completeness
      - 20 pts: program count (>=10 full marks)
      - 15 pts: sample peak quality (no clipping)
      - 15 pts: program name quality (within limit)
    """
    score = 0.0

    # expansion.json completeness (30 pts)
    exp_fields = ["Name", "Engine", "Version", "Description", "Author"]
    filled = sum(1 for f in exp_fields if expansion.get(f, "").strip())
    score += 30 * (filled / len(exp_fields))

    # sonic_dna (20 pts)
    dna = manifest.get("sonic_dna", {}) if manifest else {}
    dna_dims = ["brightness", "warmth", "movement", "density", "space", "aggression"]
    filled_dna = sum(1 for d in dna_dims if d in dna)
    score += 20 * (filled_dna / len(dna_dims))

    # program count (20 pts — 10+ = full)
    prog_count = len(programs)
    score += min(20.0, 20 * prog_count / 10)

    # no clipping (15 pts)
    if sample_peaks:
        clean = sum(1 for p in sample_peaks.values() if p < CLIPPING_LINEAR)
        score += 15 * (clean / len(sample_peaks))
    else:
        score += 15  # no samples to check = no deduction

    # program names within limit (15 pts)
    if programs:
        ok = sum(1 for p in programs
                 if len((p.get("ProgramName") or "").strip()) <= PROGRAM_NAME_MAX)
        score += 15 * (ok / len(programs))
    else:
        score += 15

    return round(score, 1)


# ---------------------------------------------------------------------------
# Core checker
# ---------------------------------------------------------------------------

def run_checks(xpn_path: Path, tier: str) -> Tuple[List[CheckResult], dict]:
    """
    Run all 11 automated checks. Returns (results, metadata).
    metadata keys: pack_name, engine, version, tier, score, threshold
    """
    results: List[CheckResult] = []
    threshold = TIER_THRESHOLDS[tier]

    if not xpn_path.exists():
        print(f"ERROR: File not found: {xpn_path}", file=sys.stderr)
        sys.exit(1)

    try:
        zf = zipfile.ZipFile(xpn_path, "r")
    except zipfile.BadZipFile:
        print(f"ERROR: Not a valid ZIP/XPN file: {xpn_path}", file=sys.stderr)
        sys.exit(1)

    namelist = zf.namelist()

    # ------------------------------------------------------------------
    # CHECK 1: expansion.json
    # ------------------------------------------------------------------
    expansion: dict = {}
    exp_names = [n for n in namelist if Path(n).name == "expansion.json"]
    if exp_names:
        try:
            expansion = json.loads(zf.read(exp_names[0]))
        except Exception:
            pass
        name  = expansion.get("Name", "").strip()
        eng   = expansion.get("Engine", "").strip()
        ver   = expansion.get("Version", "").strip()
        if name and eng and ver:
            results.append(CheckResult(
                1, "expansion.json — Name/Engine/Version populated", PASS,
                f'Name: "{name}", Engine: {eng}, Version: {ver}'
            ))
        else:
            missing = [f for f, v in [("Name", name), ("Engine", eng), ("Version", ver)] if not v]
            results.append(CheckResult(
                1, "expansion.json — Name/Engine/Version populated", FAIL,
                f"Missing fields: {', '.join(missing)}"
            ))
    else:
        results.append(CheckResult(
            1, "expansion.json — Name/Engine/Version populated", FAIL,
            "expansion.json not found in archive"
        ))

    pack_name = expansion.get("Name", xpn_path.stem)
    pack_eng  = expansion.get("Engine", "?")
    pack_ver  = expansion.get("Version", "?")

    # ------------------------------------------------------------------
    # CHECK 2: bundle_manifest.json + sonic_dna
    # ------------------------------------------------------------------
    manifest: dict = {}
    man_names = [n for n in namelist if Path(n).name == "bundle_manifest.json"]
    if man_names:
        try:
            manifest = json.loads(zf.read(man_names[0]))
        except Exception:
            pass
        dna = manifest.get("sonic_dna", {})
        dims = ["brightness", "warmth", "movement", "density", "space", "aggression"]
        present = [d for d in dims if d in dna]
        if len(present) >= 1:
            results.append(CheckResult(
                2, "bundle_manifest.json — sonic_dna populated", PASS,
                f"sonic_dna populated ({len(present)} dimensions)"
            ))
        else:
            results.append(CheckResult(
                2, "bundle_manifest.json — sonic_dna populated", FAIL,
                "sonic_dna missing or empty"
            ))
    else:
        results.append(CheckResult(
            2, "bundle_manifest.json — sonic_dna populated", FAIL,
            "bundle_manifest.json not found in archive"
        ))

    # ------------------------------------------------------------------
    # CHECK 3: Cover art >= 400px
    # ------------------------------------------------------------------
    art_names = [n for n in namelist
                 if Path(n).name.lower() in ("cover.png", "cover.jpg", "artwork.png", "artwork.jpg")
                 or re.search(r"cover\.(png|jpg)$", n.lower())]
    if art_names:
        art_data = zf.read(art_names[0])
        dims = _png_dimensions(art_data) or _jpg_dimensions(art_data)
        if dims:
            w, h = dims
            if w >= COVER_MIN_PX and h >= COVER_MIN_PX:
                results.append(CheckResult(
                    3, f"Cover art — {Path(art_names[0]).name}", PASS,
                    f"{w}×{h}px"
                ))
            else:
                results.append(CheckResult(
                    3, f"Cover art — {Path(art_names[0]).name}", FAIL,
                    f"{w}×{h}px (minimum {COVER_MIN_PX}×{COVER_MIN_PX}px required)"
                ))
        else:
            results.append(CheckResult(
                3, f"Cover art — {Path(art_names[0]).name}", SKIP,
                "Could not parse image dimensions"
            ))
    else:
        results.append(CheckResult(
            3, "Cover art", FAIL,
            "No cover image found (expected cover.png or cover.jpg)"
        ))

    # ------------------------------------------------------------------
    # Load all XPM programs
    # ------------------------------------------------------------------
    xpm_names = [n for n in namelist if n.lower().endswith(".xpm")]
    programs: List[ET.Element] = []
    program_names: List[str] = []
    for xn in xpm_names:
        try:
            root = _parse_xpm(zf.read(xn))
            if root is not None:
                programs.append(root)
                prog_name = (root.get("ProgramName") or
                             root.findtext("ProgramName") or
                             Path(xn).stem)
                program_names.append(prog_name.strip())
        except Exception:
            pass

    # ------------------------------------------------------------------
    # CHECK 4: Program names <= 20 chars
    # ------------------------------------------------------------------
    too_long = [(name, len(name)) for name in program_names if len(name) > PROGRAM_NAME_MAX]
    if too_long:
        examples = "; ".join(f'"{n}" ({c})' for n, c in too_long[:3])
        results.append(CheckResult(
            4, "Program names — all ≤ 20 chars", FAIL,
            f"{len(too_long)} name(s) exceed limit: {examples}"
        ))
    else:
        results.append(CheckResult(
            4, f"Program names — all ≤ 20 chars", PASS,
            f"{len(program_names)} program(s) checked"
        ))

    # ------------------------------------------------------------------
    # Collect samples and analyse audio
    # ------------------------------------------------------------------
    wav_names = [n for n in namelist if n.lower().endswith(".wav")]
    sample_peaks: dict = {}   # wav path -> linear peak
    for wn in wav_names:
        try:
            data = zf.read(wn)
            peak = _wav_peak(data)
            if peak is not None:
                sample_peaks[wn] = peak
        except Exception:
            pass

    # ------------------------------------------------------------------
    # CHECK 5: No sample clipping (peak < -0.5 dBFS)
    # ------------------------------------------------------------------
    clipping = {p: v for p, v in sample_peaks.items() if v >= CLIPPING_LINEAR}
    if not wav_names:
        results.append(CheckResult(5, "No sample clipping (peak < -0.5 dBFS)", SKIP,
                                   "No WAV files found in archive"))
    elif clipping:
        ex = list(clipping.keys())[:3]
        ex_str = "; ".join(f'"{Path(p).name}" ({20*math.log10(clipping[p]):.1f} dBFS)' for p in ex)
        results.append(CheckResult(5, "No sample clipping (peak < -0.5 dBFS)", FAIL,
                                   f"{len(clipping)} clipping sample(s): {ex_str}"))
    else:
        results.append(CheckResult(5, "No sample clipping (peak < -0.5 dBFS)", PASS,
                                   f"{len(sample_peaks)} sample(s) checked"))

    # ------------------------------------------------------------------
    # CHECK 6: No silent programs (at least 1 pad per program has a sample)
    # ------------------------------------------------------------------
    silent_programs = []
    for root, pname in zip(programs, program_names):
        layers = _get_layers(root)
        has_sample = any(
            (layer.get("SampleFile") or "").strip()
            for layer in layers
        )
        if not has_sample:
            silent_programs.append(pname)

    if not programs:
        results.append(CheckResult(6, "No silent programs", SKIP, "No XPM files found"))
    elif silent_programs:
        ex = "; ".join(f'"{p}"' for p in silent_programs[:3])
        results.append(CheckResult(6, "No silent programs", FAIL,
                                   f"{len(silent_programs)} program(s) have no samples: {ex}"))
    else:
        results.append(CheckResult(6, "No silent programs", PASS,
                                   f"{len(programs)} program(s) all have samples"))

    # ------------------------------------------------------------------
    # CHECK 7: Velocity layers contiguous (no gaps)
    # ------------------------------------------------------------------
    gap_programs = []
    for root, pname in zip(programs, program_names):
        layers = _get_layers(root)
        if not layers:
            continue
        try:
            vel_pairs = []
            for layer in layers:
                vs = layer.get("VelStart") or layer.get("velocityStart")
                ve = layer.get("VelEnd")   or layer.get("velocityEnd")
                sf = (layer.get("SampleFile") or "").strip()
                if vs is not None and ve is not None and sf:
                    vel_pairs.append((int(vs), int(ve)))
            if not vel_pairs:
                continue
            vel_pairs.sort()
            gap = False
            for i in range(1, len(vel_pairs)):
                if vel_pairs[i][0] != vel_pairs[i-1][1] + 1:
                    gap = True
                    break
            if gap:
                gap_programs.append(pname)
        except (ValueError, TypeError):
            pass

    if gap_programs:
        ex = "; ".join(f'"{p}"' for p in gap_programs[:3])
        results.append(CheckResult(7, "Velocity layers contiguous (no gaps)", FAIL,
                                   f"{len(gap_programs)} program(s) have gaps: {ex}"))
    else:
        results.append(CheckResult(7, "Velocity layers contiguous (no gaps)", PASS,
                                   f"{len(programs)} program(s) checked"))

    # ------------------------------------------------------------------
    # CHECK 8: Tuning within ±50 cents across all layers
    # ------------------------------------------------------------------
    tuning_violations = []
    for root, pname in zip(programs, program_names):
        layers = _get_layers(root)
        for layer in layers:
            tune = layer.get("Tune") or layer.get("tune") or layer.get("TuneCoarse")
            if tune is not None:
                try:
                    cents = float(tune) * 100 if abs(float(tune)) <= 1.0 else float(tune)
                    if abs(cents) > TUNING_TOLERANCE_CENTS:
                        tuning_violations.append((pname, cents))
                except ValueError:
                    pass
    if tuning_violations:
        ex = "; ".join(f'"{p}" ({c:+.0f}¢)' for p, c in tuning_violations[:3])
        results.append(CheckResult(8, f"Tuning within ±{TUNING_TOLERANCE_CENTS:.0f} cents", FAIL,
                                   f"{len(tuning_violations)} layer(s) out of range: {ex}"))
    else:
        results.append(CheckResult(8, f"Tuning within ±{TUNING_TOLERANCE_CENTS:.0f} cents", PASS,
                                   "All layers within tolerance"))

    # ------------------------------------------------------------------
    # CHECK 9: pack_score meets tier threshold
    # ------------------------------------------------------------------
    score = _compute_pack_score(expansion, manifest, programs, sample_peaks)
    score_status = PASS if score >= threshold else FAIL
    results.append(CheckResult(
        9, f"pack_score ≥ {threshold} ({tier.upper()})", score_status,
        f"Computed score: {score}/100"
    ))

    # ------------------------------------------------------------------
    # CHECK 10: Version format "X.Y.Z"
    # ------------------------------------------------------------------
    ver_str = expansion.get("Version", "").strip()
    if VERSION_RE.match(ver_str):
        results.append(CheckResult(10, "Version format (X.Y.Z)", PASS, f'"{ver_str}"'))
    else:
        results.append(CheckResult(10, "Version format (X.Y.Z)", FAIL,
                                   f'"{ver_str}" does not match X.Y.Z pattern'))

    # ------------------------------------------------------------------
    # CHECK 11: CHANGELOG present in zip
    # ------------------------------------------------------------------
    changelog_names = [n for n in namelist
                       if Path(n).name.upper().startswith("CHANGELOG")]
    if changelog_names:
        results.append(CheckResult(11, "CHANGELOG present", PASS,
                                   f"Found: {Path(changelog_names[0]).name}"))
    else:
        results.append(CheckResult(11, "CHANGELOG present", FAIL,
                                   "No CHANGELOG.md or CHANGELOG file found in archive"))

    zf.close()

    metadata = {
        "pack_name": pack_name,
        "engine":    pack_eng,
        "version":   pack_ver,
        "tier":      tier.upper(),
        "score":     score,
        "threshold": threshold,
    }
    return results, metadata


# ---------------------------------------------------------------------------
# Manual checks (placeholders)
# ---------------------------------------------------------------------------

MANUAL_CHECKS = [
    (12, "Ear test — play all programs on hardware MPC"),
    (13, "DNA accuracy — A/B test brightness/warmth against 3 known packs"),
    (14, "Pack story holds — can a blind listener identify the theme?"),
    (15, "Naming review — all program names evocative and consistent"),
]


# ---------------------------------------------------------------------------
# Formatters
# ---------------------------------------------------------------------------

def _status_symbol_text(status: str) -> str:
    if status == PASS:
        return "✓"
    if status == FAIL:
        return "✗"
    return "~"


def _status_symbol_md(status: str) -> str:
    if status == PASS:
        return "- [x]"
    if status == FAIL:
        return "- [ ] ~~FAIL~~"
    return "- [ ] ~~SKIP~~"


def format_text(results: List[CheckResult], metadata: dict) -> str:
    lines = []
    title = (f"XO_OX Pack Release Checklist — "
             f"{metadata['pack_name']} v{metadata['version']} [{metadata['tier']}]")
    bar   = "═" * len(title)
    lines.append(title)
    lines.append(bar)
    lines.append("")

    lines.append("AUTOMATED CHECKS")
    for r in results:
        sym  = _status_symbol_text(r.status)
        detail = f" — {r.detail}" if r.detail else ""
        lines.append(f"  {sym}  {r.number}. {r.description}{detail}")
    lines.append("")

    lines.append("MANUAL CHECKS")
    for num, desc in MANUAL_CHECKS:
        lines.append(f"  [ ] {num}. {desc}")
    lines.append("")

    # Summary
    passed   = sum(1 for r in results if r.status == PASS)
    failed   = sum(1 for r in results if r.status == FAIL)
    skipped  = sum(1 for r in results if r.status == SKIP)
    auto_total = len(results)
    lines.append(f"RESULT: {passed}/{auto_total} automated ✓"
                 + (f" | {skipped} skipped" if skipped else "")
                 + f" | {len(MANUAL_CHECKS)}/{len(MANUAL_CHECKS)} manual pending")

    if failed == 0:
        lines.append("STATUS: READY — all automated checks pass")
    else:
        lines.append(f"STATUS: BLOCKED — fix {failed} error{'s' if failed != 1 else ''} before release")

    return "\n".join(lines)


def format_markdown(results: List[CheckResult], metadata: dict) -> str:
    lines = []
    lines.append(f"# XO_OX Pack Release Checklist")
    lines.append(f"**{metadata['pack_name']}** v{metadata['version']} | "
                 f"Engine: {metadata['engine']} | Tier: **{metadata['tier']}**")
    lines.append("")

    lines.append("## Automated Checks")
    lines.append("")
    for r in results:
        check = "x" if r.status == PASS else " "
        detail = f" — _{r.detail}_" if r.detail else ""
        lines.append(f"- [{check}] **{r.number}.** {r.description}{detail}")
    lines.append("")

    lines.append("## Manual Checks")
    lines.append("")
    for num, desc in MANUAL_CHECKS:
        lines.append(f"- [ ] **{num}.** {desc}")
    lines.append("")

    passed  = sum(1 for r in results if r.status == PASS)
    failed  = sum(1 for r in results if r.status == FAIL)
    skipped = sum(1 for r in results if r.status == SKIP)
    auto_total = len(results)

    lines.append("---")
    lines.append(f"**Score:** {metadata['score']}/100 (threshold: {metadata['threshold']})")
    lines.append(f"**Result:** {passed}/{auto_total} automated checks pass"
                 + (f" | {skipped} skipped" if skipped else "")
                 + f" | {len(MANUAL_CHECKS)} manual checks pending")
    if failed == 0:
        lines.append("**Status:** ✅ READY — all automated checks pass")
    else:
        lines.append(f"**Status:** ❌ BLOCKED — {failed} error{'s' if failed != 1 else ''} must be fixed before release")

    return "\n".join(lines)


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def main():
    parser = argparse.ArgumentParser(
        description="XO_OX Pack Release Checklist — 15-point pre-release verification"
    )
    parser.add_argument("pack", help="Path to the .xpn pack file")
    parser.add_argument(
        "--tier", choices=["signal", "form", "doctrine"], default="form",
        help="Pack tier (sets pack_score threshold: signal=70, form=80, doctrine=90)"
    )
    parser.add_argument(
        "--format", choices=["text", "markdown"], default="text",
        dest="fmt", help="Output format (default: text)"
    )
    parser.add_argument(
        "--output", metavar="FILE", default=None,
        help="Write output to FILE instead of stdout"
    )
    args = parser.parse_args()

    xpn_path = Path(args.pack)
    results, metadata = run_checks(xpn_path, args.tier)

    if args.fmt == "markdown":
        report = format_markdown(results, metadata)
    else:
        report = format_text(results, metadata)

    if args.output:
        Path(args.output).write_text(report, encoding="utf-8")
        print(f"Report written to {args.output}")
    else:
        print(report)

    # Exit code: 0 = all pass, 1 = failures
    failed = sum(1 for r in results if r.status == FAIL)
    sys.exit(0 if failed == 0 else 1)


if __name__ == "__main__":
    main()
