#!/usr/bin/env python3
"""
xpn_community_qa.py — Headless QA orchestrator for XO_OX community pack submissions.

Usage:
    python xpn_community_qa.py --submission pack.submission.zip [--discord-output] [--strict]

Checks performed (in sequence):
    1. Structure check    — pack.yaml, LICENSE.txt, ≥1 .xometa or WAV present
    2. pack.yaml          — required fields: pack_name, contributor_handle, target_engines, xo_ox_version
    3. WAV audit          — sample rate, bit depth, clipping, silence
    4. XPM validation     — XML parse, KeyTrack=True, RootNote=0, no VelStart=0 ghost triggers
    5. SHA-256 dedup      — compare against known_hashes.json index
    6. Overall verdict    — PASS / REVIEW / REJECT

Outputs:
    qa_report.json  — written alongside the input submission ZIP
"""

import argparse
import hashlib
import json
import struct
import sys
import tempfile
import xml.etree.ElementTree as ET
import zipfile
from datetime import datetime, timezone
from pathlib import Path

# ---------------------------------------------------------------------------
# Constants
# ---------------------------------------------------------------------------

REQUIRED_YAML_FIELDS = ["pack_name", "contributor_handle", "target_engines", "xo_ox_version"]
VALID_SAMPLE_RATES = {44100, 48000}
VALID_BIT_DEPTHS = {16, 24}
CLIP_THRESHOLD = 0.999  # peak must be below this
KNOWN_HASHES_FILENAME = "known_hashes.json"

# ---------------------------------------------------------------------------
# Minimal YAML parser (stdlib only — no PyYAML)
# ---------------------------------------------------------------------------

def parse_yaml_minimal(text: str) -> dict:
    """
    Parse a simple flat/list YAML file without PyYAML.
    Handles:  key: value  and  key: [a, b, c]  and  key: "quoted"
    Does NOT handle nested mappings or multi-line blocks.
    """
    result = {}
    for raw_line in text.splitlines():
        line = raw_line.strip()
        if not line or line.startswith("#"):
            continue
        if ":" not in line:
            continue
        key, _, rest = line.partition(":")
        key = key.strip()
        rest = rest.strip()
        if not key:
            continue
        # Inline list: [a, b, c]
        if rest.startswith("[") and rest.endswith("]"):
            inner = rest[1:-1]
            items = [item.strip().strip('"').strip("'") for item in inner.split(",") if item.strip()]
            result[key] = items
        # Quoted string
        elif rest.startswith('"') and rest.endswith('"'):
            result[key] = rest[1:-1]
        elif rest.startswith("'") and rest.endswith("'"):
            result[key] = rest[1:-1]
        else:
            result[key] = rest
    return result


# ---------------------------------------------------------------------------
# WAV parsing (stdlib struct — no soundfile/scipy)
# ---------------------------------------------------------------------------

class WavInfo:
    __slots__ = ("path", "sample_rate", "bit_depth", "num_channels", "num_frames",
                 "peak", "error")

    def __init__(self):
        self.path = ""
        self.sample_rate = 0
        self.bit_depth = 0
        self.num_channels = 0
        self.num_frames = 0
        self.peak = 0.0
        self.error = None


def parse_wav(data: bytes, path: str = "") -> WavInfo:
    info = WavInfo()
    info.path = path
    try:
        if len(data) < 44:
            info.error = "file too short to be a valid WAV"
            return info
        if data[:4] != b"RIFF" or data[8:12] != b"WAVE":
            info.error = "not a valid RIFF/WAVE file"
            return info

        # Walk chunks to find fmt and data
        offset = 12
        fmt_found = False
        data_offset = -1
        data_size = 0

        while offset + 8 <= len(data):
            chunk_id = data[offset:offset + 4]
            chunk_size = struct.unpack_from("<I", data, offset + 4)[0]
            chunk_data_start = offset + 8

            if chunk_id == b"fmt ":
                if chunk_size < 16:
                    info.error = "fmt chunk too small"
                    return info
                audio_format = struct.unpack_from("<H", data, chunk_data_start)[0]
                if audio_format not in (1, 3):  # 1=PCM, 3=IEEE float
                    info.error = f"unsupported audio format {audio_format} (only PCM/float)"
                    return info
                info.num_channels = struct.unpack_from("<H", data, chunk_data_start + 2)[0]
                info.sample_rate = struct.unpack_from("<I", data, chunk_data_start + 4)[0]
                info.bit_depth = struct.unpack_from("<H", data, chunk_data_start + 14)[0]
                fmt_found = True

            elif chunk_id == b"data":
                data_offset = chunk_data_start
                data_size = chunk_size

            offset += 8 + chunk_size
            if chunk_size % 2 == 1:
                offset += 1  # padding byte

        if not fmt_found:
            info.error = "no fmt chunk found"
            return info
        if data_offset < 0:
            info.error = "no data chunk found"
            return info

        bytes_per_sample = info.bit_depth // 8
        if bytes_per_sample == 0:
            info.error = "bit depth is 0"
            return info

        info.num_frames = data_size // (bytes_per_sample * max(info.num_channels, 1))

        if info.num_frames == 0:
            info.peak = 0.0
            return info

        # Compute peak — scan up to 500k samples for speed
        sample_bytes = data[data_offset: data_offset + data_size]
        max_scan = min(len(sample_bytes), 500000 * bytes_per_sample)
        scanned = sample_bytes[:max_scan]

        peak = 0.0
        if info.bit_depth == 16:
            count = len(scanned) // 2
            samples = struct.unpack_from(f"<{count}h", scanned)
            peak = max(abs(s) for s in samples) / 32768.0 if samples else 0.0
        elif info.bit_depth == 24:
            peak_int = 0
            for i in range(0, len(scanned) - 2, 3):
                raw = scanned[i] | (scanned[i + 1] << 8) | (scanned[i + 2] << 16)
                if raw & 0x800000:
                    raw -= 0x1000000
                abs_raw = abs(raw)
                if abs_raw > peak_int:
                    peak_int = abs_raw
            peak = peak_int / 8388608.0
        elif info.bit_depth == 32:
            count = len(scanned) // 4
            samples = struct.unpack_from(f"<{count}f", scanned)
            peak = max(abs(s) for s in samples) if samples else 0.0
        else:
            info.error = f"unsupported bit depth {info.bit_depth} for peak analysis"
            return info

        info.peak = peak

    except Exception as exc:
        info.error = f"parse error: {exc}"

    return info


# ---------------------------------------------------------------------------
# Check implementations
# ---------------------------------------------------------------------------

def check_structure(extracted_root: Path) -> dict:
    """Hard check: pack.yaml, LICENSE.txt, ≥1 .xometa or .wav present."""
    issues = []
    warnings = []

    has_pack_yaml = any(extracted_root.rglob("pack.yaml"))
    has_license = any(extracted_root.rglob("LICENSE.txt"))
    xometa_files = list(extracted_root.rglob("*.xometa"))
    wav_files = list(extracted_root.rglob("*.wav")) + list(extracted_root.rglob("*.WAV"))

    if not has_pack_yaml:
        issues.append("MISSING: pack.yaml")
    if not has_license:
        issues.append("MISSING: LICENSE.txt")
    if not xometa_files and not wav_files:
        issues.append("MISSING: no .xometa or WAV files found")

    # Soft warnings
    cover_art = list(extracted_root.rglob("cover_art.png")) + list(extracted_root.rglob("cover_art.jpg"))
    if not cover_art:
        warnings.append("cover_art.png/jpg not found (required for final submission)")

    return {
        "status": "FAIL" if issues else ("WARN" if warnings else "PASS"),
        "hard_issues": issues,
        "warnings": warnings,
        "xometa_count": len(xometa_files),
        "wav_count": len(wav_files),
    }


def check_pack_yaml(extracted_root: Path) -> dict:
    """Hard check: required fields present in pack.yaml."""
    yaml_paths = list(extracted_root.rglob("pack.yaml"))
    if not yaml_paths:
        return {"status": "SKIP", "reason": "pack.yaml not found"}

    yaml_path = yaml_paths[0]
    try:
        content = yaml_path.read_text(encoding="utf-8", errors="replace")
    except Exception as exc:
        return {"status": "FAIL", "hard_issues": [f"Cannot read pack.yaml: {exc}"], "warnings": []}

    parsed = parse_yaml_minimal(content)
    missing = [f for f in REQUIRED_YAML_FIELDS if f not in parsed or not parsed[f]]
    warnings = []

    # Soft: contributor_email
    if "contributor_email" not in parsed:
        warnings.append("contributor_email not present (recommended)")

    # Soft: art_origin
    if "art_origin" not in parsed:
        warnings.append("art_origin not declared (required for cover art clearance)")

    return {
        "status": "FAIL" if missing else ("WARN" if warnings else "PASS"),
        "hard_issues": [f"MISSING required field: {f}" for f in missing],
        "warnings": warnings,
        "fields_found": list(parsed.keys()),
        "parsed": {k: parsed[k] for k in REQUIRED_YAML_FIELDS if k in parsed},
    }


def check_wav_files(extracted_root: Path) -> dict:
    """Audit each WAV: sample rate, bit depth, clipping, silence."""
    wav_files = list(extracted_root.rglob("*.wav")) + list(extracted_root.rglob("*.WAV"))
    if not wav_files:
        return {"status": "SKIP", "reason": "no WAV files in submission"}

    hard_issues = []
    warnings = []
    file_results = []

    for wav_path in wav_files:
        rel = wav_path.relative_to(extracted_root)
        data = wav_path.read_bytes()
        info = parse_wav(data, str(rel))
        result = {"file": str(rel)}

        if info.error:
            result["status"] = "FAIL"
            result["error"] = info.error
            hard_issues.append(f"{rel}: {info.error}")
            file_results.append(result)
            continue

        file_hard = []
        file_warn = []

        if info.sample_rate not in VALID_SAMPLE_RATES:
            file_hard.append(f"invalid sample rate {info.sample_rate}Hz (must be 44100 or 48000)")
        if info.bit_depth not in VALID_BIT_DEPTHS:
            file_hard.append(f"invalid bit depth {info.bit_depth}-bit (must be 16 or 24)")
        if info.num_frames == 0:
            file_hard.append("silent/empty file (0 sample frames)")
        if info.peak >= CLIP_THRESHOLD:
            file_hard.append(f"clipping detected (peak={info.peak:.4f} ≥ {CLIP_THRESHOLD})")
        if info.peak < 0.001 and info.num_frames > 0:
            file_warn.append(f"extremely quiet (peak={info.peak:.6f}) — may be silent")

        result.update({
            "sample_rate": info.sample_rate,
            "bit_depth": info.bit_depth,
            "channels": info.num_channels,
            "frames": info.num_frames,
            "peak": round(info.peak, 5),
            "status": "FAIL" if file_hard else ("WARN" if file_warn else "PASS"),
            "hard_issues": file_hard,
            "warnings": file_warn,
        })
        hard_issues.extend([f"{rel}: {i}" for i in file_hard])
        warnings.extend([f"{rel}: {w}" for w in file_warn])
        file_results.append(result)

    pass_count = sum(1 for r in file_results if r.get("status") == "PASS")
    fail_count = sum(1 for r in file_results if r.get("status") == "FAIL")
    warn_count = sum(1 for r in file_results if r.get("status") == "WARN")

    return {
        "status": "FAIL" if hard_issues else ("WARN" if warnings else "PASS"),
        "hard_issues": hard_issues,
        "warnings": warnings,
        "total": len(wav_files),
        "pass": pass_count,
        "warn": warn_count,
        "fail": fail_count,
        "files": file_results,
    }


def check_xpm_files(extracted_root: Path) -> dict:
    """Validate .xpm XML files: parse, KeyTrack=True, RootNote=0, no ghost VelStart=0."""
    xpm_files = list(extracted_root.rglob("*.xpm"))
    if not xpm_files:
        return {"status": "SKIP", "reason": "no .xpm files in submission"}

    hard_issues = []
    warnings = []
    file_results = []

    for xpm_path in xpm_files:
        rel = xpm_path.relative_to(extracted_root)
        result = {"file": str(rel)}
        try:
            content = xpm_path.read_bytes()
            root = ET.fromstring(content)
        except ET.ParseError as exc:
            result["status"] = "FAIL"
            result["error"] = f"XML parse error: {exc}"
            hard_issues.append(f"{rel}: XML parse error: {exc}")
            file_results.append(result)
            continue
        except Exception as exc:
            result["status"] = "FAIL"
            result["error"] = str(exc)
            hard_issues.append(f"{rel}: {exc}")
            file_results.append(result)
            continue

        file_hard = []
        file_warn = []

        # KeyTrack check — should appear on Instrument or KeyGroup level
        keytrack_vals = [el.get("KeyTrack") for el in root.iter() if el.get("KeyTrack") is not None]
        for val in keytrack_vals:
            if val.strip().lower() != "true":
                file_hard.append(f"KeyTrack={val!r} (must be True)")

        # RootNote check
        root_note_vals = [el.get("RootNote") for el in root.iter() if el.get("RootNote") is not None]
        for val in root_note_vals:
            if val.strip() != "0":
                file_hard.append(f"RootNote={val!r} (must be 0)")

        # Ghost trigger check: layers with VelStart=0 that have no SampleFile
        for layer in root.iter("Layer"):
            vel_start = layer.get("VelStart", "").strip()
            sample_file = layer.get("SampleFile", "").strip()
            if vel_start == "0" and not sample_file:
                file_hard.append("empty layer with VelStart=0 found (ghost trigger risk)")

        result.update({
            "status": "FAIL" if file_hard else ("WARN" if file_warn else "PASS"),
            "hard_issues": file_hard,
            "warnings": file_warn,
        })
        hard_issues.extend([f"{rel}: {i}" for i in file_hard])
        warnings.extend([f"{rel}: {w}" for w in file_warn])
        file_results.append(result)

    return {
        "status": "FAIL" if hard_issues else ("WARN" if warnings else "PASS"),
        "hard_issues": hard_issues,
        "warnings": warnings,
        "total": len(xpm_files),
        "files": file_results,
    }


def sha256_of_file(path: Path) -> str:
    h = hashlib.sha256()
    with open(path, "rb") as f:
        for chunk in iter(lambda: f.read(65536), b""):
            h.update(chunk)
    return h.hexdigest()


def check_dedup(extracted_root: Path, script_dir: Path) -> dict:
    """Compare file SHA-256s against known_hashes.json index."""
    index_path = script_dir / KNOWN_HASHES_FILENAME
    known_hashes: dict = {}
    index_loaded = False

    if index_path.exists():
        try:
            known_hashes = json.loads(index_path.read_text(encoding="utf-8"))
            index_loaded = True
        except Exception as exc:
            return {
                "status": "WARN",
                "warnings": [f"Could not load {KNOWN_HASHES_FILENAME}: {exc}"],
                "hard_issues": [],
                "duplicates": [],
            }
    else:
        return {
            "status": "SKIP",
            "reason": f"{KNOWN_HASHES_FILENAME} not found alongside script — dedup skipped",
            "hard_issues": [],
            "warnings": [],
            "duplicates": [],
        }

    hard_issues = []
    duplicates = []
    checked = 0

    for file_path in extracted_root.rglob("*"):
        if not file_path.is_file():
            continue
        digest = sha256_of_file(file_path)
        rel = str(file_path.relative_to(extracted_root))
        checked += 1
        if digest in known_hashes:
            match_info = known_hashes[digest]
            msg = f"{rel} → duplicate of {match_info!r}"
            duplicates.append({"file": rel, "sha256": digest, "known_as": match_info})
            hard_issues.append(msg)

    return {
        "status": "FAIL" if hard_issues else "PASS",
        "hard_issues": hard_issues,
        "warnings": [],
        "duplicates": duplicates,
        "files_checked": checked,
        "index_entries": len(known_hashes),
    }


# ---------------------------------------------------------------------------
# Verdict computation
# ---------------------------------------------------------------------------

GATE_NAMES = ["structure", "pack_yaml", "wav_audit", "xpm_validation", "dedup"]


def compute_verdict(gates: dict, strict: bool) -> str:
    """PASS / REVIEW / REJECT based on gate results."""
    for gate_name in GATE_NAMES:
        gate = gates.get(gate_name, {})
        status = gate.get("status", "SKIP")
        if status == "FAIL":
            return "REJECT"

    if strict:
        for gate_name in GATE_NAMES:
            gate = gates.get(gate_name, {})
            if gate.get("warnings"):
                return "REJECT"

    # Check for any warnings → REVIEW
    for gate_name in GATE_NAMES:
        gate = gates.get(gate_name, {})
        if gate.get("warnings") or gate.get("status") == "WARN":
            return "REVIEW"

    return "PASS"


# ---------------------------------------------------------------------------
# Report formatters
# ---------------------------------------------------------------------------

STATUS_EMOJI = {"PASS": "✅", "FAIL": "❌", "WARN": "⚠️", "SKIP": "ℹ️", "REVIEW": "⚠️", "REJECT": "❌"}


def gate_label(name: str) -> str:
    labels = {
        "structure": "Structure",
        "pack_yaml": "pack.yaml",
        "wav_audit": "WAV Audit",
        "xpm_validation": "XPM Validation",
        "dedup": "Dedup Check",
    }
    return labels.get(name, name)


def format_discord(report: dict) -> str:
    verdict = report["verdict"]
    verdict_emoji = STATUS_EMOJI.get(verdict, "❓")
    submission = report.get("submission_file", "unknown")
    ts = report.get("timestamp", "")

    lines = [
        f"**XO_OX Community Pack QA Report**",
        f"Submission: `{submission}`  |  {ts}",
        f"",
        f"**Verdict: {verdict_emoji} {verdict}**",
        f"",
        f"**Gate Results:**",
    ]

    for gate_name in GATE_NAMES:
        gate = report["gates"].get(gate_name)
        if gate is None:
            continue
        status = gate.get("status", "SKIP")
        emoji = STATUS_EMOJI.get(status, "❓")
        label = gate_label(gate_name)
        lines.append(f"  {emoji} **{label}** — {status}")

        for issue in gate.get("hard_issues", []):
            lines.append(f"    ❌ {issue}")
        for warning in gate.get("warnings", []):
            lines.append(f"    ⚠️ {warning}")

        # Gate-specific summary stats
        if gate_name == "wav_audit" and "total" in gate:
            lines.append(f"    ℹ️ {gate['total']} WAV(s): {gate.get('pass',0)} pass / {gate.get('warn',0)} warn / {gate.get('fail',0)} fail")
        if gate_name == "xpm_validation" and "total" in gate:
            lines.append(f"    ℹ️ {gate['total']} XPM(s) checked")
        if gate_name == "dedup" and gate.get("status") != "SKIP":
            lines.append(f"    ℹ️ {gate.get('files_checked', 0)} files vs {gate.get('index_entries', 0)} known hashes")

    if verdict == "PASS":
        lines += ["", "✅ All automated checks passed. Ready for human curator review."]
    elif verdict == "REVIEW":
        lines += ["", "⚠️ Soft warnings present — human review required before acceptance."]
    else:
        lines += ["", "❌ Hard failures detected. Submission auto-rejected."]

    return "\n".join(lines)


def format_plain(report: dict) -> str:
    verdict = report["verdict"]
    submission = report.get("submission_file", "unknown")
    ts = report.get("timestamp", "")
    lines = [
        "=" * 60,
        "XO_OX Community Pack QA Report",
        f"Submission : {submission}",
        f"Timestamp  : {ts}",
        f"Verdict    : {verdict}",
        "=" * 60,
    ]
    for gate_name in GATE_NAMES:
        gate = report["gates"].get(gate_name)
        if gate is None:
            continue
        status = gate.get("status", "SKIP")
        label = gate_label(gate_name)
        lines.append(f"\n[{status}] {label}")
        for issue in gate.get("hard_issues", []):
            lines.append(f"  FAIL: {issue}")
        for warning in gate.get("warnings", []):
            lines.append(f"  WARN: {warning}")
        if gate_name == "wav_audit" and "total" in gate:
            lines.append(f"  {gate['total']} WAV(s): {gate.get('pass',0)} pass / {gate.get('warn',0)} warn / {gate.get('fail',0)} fail")
        if gate_name == "xpm_validation" and "total" in gate:
            lines.append(f"  {gate['total']} XPM(s) checked")
        if gate_name == "dedup" and gate.get("status") not in ("SKIP",):
            lines.append(f"  {gate.get('files_checked', 0)} files checked against {gate.get('index_entries', 0)} known hashes")
        if gate.get("reason"):
            lines.append(f"  (skipped: {gate['reason']})")
    lines.append("\n" + "=" * 60)
    return "\n".join(lines)


# ---------------------------------------------------------------------------
# Main orchestrator
# ---------------------------------------------------------------------------

def run_qa(submission_path: Path, script_dir: Path, strict: bool) -> dict:
    timestamp = datetime.now(timezone.utc).isoformat()
    report = {
        "submission_file": submission_path.name,
        "submission_path": str(submission_path),
        "timestamp": timestamp,
        "strict": strict,
        "gates": {},
        "verdict": "REJECT",
    }

    if not submission_path.exists():
        report["fatal"] = f"Submission file not found: {submission_path}"
        return report

    if not zipfile.is_zipfile(submission_path):
        report["fatal"] = "Submission is not a valid ZIP file"
        return report

    with tempfile.TemporaryDirectory(prefix="xo_ox_qa_") as tmpdir:
        extract_root = Path(tmpdir) / "submission"
        extract_root.mkdir()
        try:
            with zipfile.ZipFile(submission_path, "r") as zf:
                zf.extractall(extract_root)
        except Exception as exc:
            report["fatal"] = f"Failed to extract ZIP: {exc}"
            return report

        # Run gates in sequence
        try:
            report["gates"]["structure"] = check_structure(extract_root)
        except Exception as exc:
            report["gates"]["structure"] = {"status": "FAIL", "hard_issues": [f"Internal error: {exc}"], "warnings": []}

        try:
            report["gates"]["pack_yaml"] = check_pack_yaml(extract_root)
        except Exception as exc:
            report["gates"]["pack_yaml"] = {"status": "FAIL", "hard_issues": [f"Internal error: {exc}"], "warnings": []}

        try:
            report["gates"]["wav_audit"] = check_wav_files(extract_root)
        except Exception as exc:
            report["gates"]["wav_audit"] = {"status": "FAIL", "hard_issues": [f"Internal error: {exc}"], "warnings": []}

        try:
            report["gates"]["xpm_validation"] = check_xpm_files(extract_root)
        except Exception as exc:
            report["gates"]["xpm_validation"] = {"status": "FAIL", "hard_issues": [f"Internal error: {exc}"], "warnings": []}

        try:
            report["gates"]["dedup"] = check_dedup(extract_root, script_dir)
        except Exception as exc:
            report["gates"]["dedup"] = {"status": "WARN", "hard_issues": [], "warnings": [f"Internal error: {exc}"]}

    report["verdict"] = compute_verdict(report["gates"], strict)
    return report


def main():
    parser = argparse.ArgumentParser(
        description="XO_OX Community Pack QA Orchestrator",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    parser.add_argument(
        "--submission", required=True, metavar="ZIP",
        help="Path to the .submission.zip file",
    )
    parser.add_argument(
        "--discord-output", action="store_true",
        help="Format report as Discord-ready embed text",
    )
    parser.add_argument(
        "--strict", action="store_true",
        help="Treat all warnings as failures (REJECT instead of REVIEW)",
    )
    args = parser.parse_args()

    submission_path = Path(args.submission).resolve()
    script_dir = Path(__file__).resolve().parent

    print(f"Running QA on: {submission_path}", file=sys.stderr)

    report = run_qa(submission_path, script_dir, strict=args.strict)

    # Write qa_report.json alongside the submission
    report_path = submission_path.parent / (submission_path.stem + ".qa_report.json")
    try:
        report_path.write_text(json.dumps(report, indent=2, ensure_ascii=False), encoding="utf-8")
        print(f"Report written: {report_path}", file=sys.stderr)
    except Exception as exc:
        print(f"WARNING: Could not write qa_report.json: {exc}", file=sys.stderr)

    # Print formatted output
    if report.get("fatal"):
        print(f"\nFATAL: {report['fatal']}")
        sys.exit(2)

    if args.discord_output:
        print(format_discord(report))
    else:
        print(format_plain(report))

    # Exit code: 0=PASS, 1=REVIEW, 2=REJECT
    verdict = report.get("verdict", "REJECT")
    if verdict == "PASS":
        sys.exit(0)
    elif verdict == "REVIEW":
        sys.exit(1)
    else:
        sys.exit(2)


if __name__ == "__main__":
    main()
