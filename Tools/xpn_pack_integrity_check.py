#!/usr/bin/env python3
"""
XPN Pack Integrity Check — XO_OX Designs
Deep structural integrity check on a .xpn pack.

Verifies that all internal references are valid and the pack is self-consistent
before deploying to MPC hardware.

Checks performed:
  1. ZIP integrity        — can the .xpn be opened? are all entries readable?
  2. XPM → WAV refs      — every SampleFile path resolves to a WAV in the ZIP
  3. WAV → XPM coverage  — every WAV is referenced by at least one XPM program
  4. XPM validity         — all .xpm files parse as valid XML
  5. JSON validity        — expansion.json + bundle_manifest.json parse as valid JSON
  6. Required files       — at least one .xpm, expansion.json, at least one WAV
  7. Path case sensitivity — SampleFile case must match the actual ZIP entry
  8. Nested ZIP check     — .zip/.xpn inside the pack = CRITICAL (won't load on MPC)

Severity levels:
  CRITICAL  Pack will not load or will be missing audio
  ERROR     Pack is malformed but may partially work
  WARNING   Dead weight or potential hidden bugs (case sensitivity, unreferenced WAVs)

Usage:
    python xpn_pack_integrity_check.py path/to/pack.xpn
    python xpn_pack_integrity_check.py pack.xpn --format json
    python xpn_pack_integrity_check.py pack.xpn --strict
"""

import argparse
import json
import sys
import zipfile
from dataclasses import dataclass, field
from pathlib import Path
from typing import Dict, List, Optional, Set
from xml.etree import ElementTree as ET


# ---------------------------------------------------------------------------
# Data structures
# ---------------------------------------------------------------------------

CRITICAL = "CRITICAL"
ERROR = "ERROR"
WARNING = "WARNING"
INFO = "INFO"


@dataclass
class Issue:
    severity: str
    message: str
    detail: Optional[str] = None


@dataclass
class CheckResult:
    issues: List[Issue] = field(default_factory=list)

    def add(self, severity: str, message: str, detail: Optional[str] = None) -> None:
        self.issues.append(Issue(severity, message, detail))

    def critical_count(self) -> int:
        return sum(1 for i in self.issues if i.severity == CRITICAL)

    def error_count(self) -> int:
        return sum(1 for i in self.issues if i.severity == ERROR)

    def warning_count(self) -> int:
        return sum(1 for i in self.issues if i.severity == WARNING)


# ---------------------------------------------------------------------------
# Check implementations
# ---------------------------------------------------------------------------

def check_zip_integrity(path: str) -> tuple:
    """
    Returns (zipfile.ZipFile | None, total_size_bytes, entry_count, CheckResult).
    Attempts to read every entry to detect corruption.
    """
    result = CheckResult()
    try:
        zf = zipfile.ZipFile(path, "r")
    except zipfile.BadZipFile as e:
        result.add(CRITICAL, "Cannot open ZIP archive", str(e))
        return None, 0, 0, result
    except Exception as e:
        result.add(CRITICAL, "Unexpected error opening pack", str(e))
        return None, 0, 0, result

    entries = zf.infolist()
    total_size = sum(e.file_size for e in entries)
    bad_entries = []
    for entry in entries:
        try:
            with zf.open(entry) as f:
                while f.read(65536):
                    pass
        except Exception as e:
            bad_entries.append(f"{entry.filename}: {e}")

    if bad_entries:
        for msg in bad_entries:
            result.add(CRITICAL, "Corrupted ZIP entry", msg)

    return zf, total_size, len(entries), result


def collect_zip_entries(zf: zipfile.ZipFile) -> Dict[str, str]:
    """
    Returns a dict mapping lowercase entry name → actual entry name.
    Used for both exact lookups and case-sensitivity checks.
    """
    return {e.filename.lower(): e.filename for e in zf.infolist()}


def parse_xpm_sample_refs(zf: zipfile.ZipFile, xpm_name: str) -> tuple:
    """
    Returns (list_of_sample_paths, parse_error_or_None).
    SampleFile paths are returned as stored in the XML (may be relative or absolute).
    """
    try:
        with zf.open(xpm_name) as f:
            tree = ET.parse(f)
    except ET.ParseError as e:
        return [], str(e)
    except Exception as e:
        return [], str(e)

    root = tree.getroot()
    refs = []
    for elem in root.iter():
        if elem.tag == "SampleFile" and elem.text:
            refs.append(elem.text.strip())
        # Some XPM versions store sample path as attribute
        sample_path = elem.get("SampleFile") or elem.get("sampleFile")
        if sample_path:
            refs.append(sample_path.strip())

    return refs, None


def normalize_sample_path(raw_path: str) -> str:
    """
    Strip leading slashes / drive letters and normalize separators so the path
    can be matched against ZIP entry names (which use forward slashes).
    """
    # Replace Windows backslashes
    p = raw_path.replace("\\", "/")
    # Strip leading slash
    p = p.lstrip("/")
    return p


def check_xpm_wav_references(
    zf: zipfile.ZipFile,
    xpm_names: List[str],
    wav_names_lower: Set[str],
    entry_map: Dict[str, str],
) -> tuple:
    """
    Returns (xpm_result, all_referenced_wav_lower).
    xpm_result contains CRITICAL issues for missing WAVs, parse errors.
    Case-mismatch issues are included here too.
    """
    result = CheckResult()
    all_referenced_lower: Set[str] = set()
    total_refs = 0
    missing_refs = 0

    for xpm_name in xpm_names:
        refs, parse_err = parse_xpm_sample_refs(zf, xpm_name)
        if parse_err:
            result.add(CRITICAL, f"XPM parse error: {xpm_name}", parse_err)
            continue

        for raw_ref in refs:
            total_refs += 1
            norm = normalize_sample_path(raw_ref)
            norm_lower = norm.lower()
            all_referenced_lower.add(norm_lower)

            if norm_lower in wav_names_lower:
                # Check case match
                actual = entry_map.get(norm_lower)
                if actual and actual != norm:
                    result.add(
                        WARNING,
                        f"Case mismatch in {Path(xpm_name).name}",
                        f"XPM says '{norm}' but ZIP has '{actual}' — MPC on Linux will fail",
                    )
            else:
                missing_refs += 1
                result.add(
                    CRITICAL,
                    f"Missing WAV referenced in {Path(xpm_name).name}",
                    norm,
                )

    if total_refs > 0 and missing_refs == 0:
        # Attach summary as a synthetic INFO-level (won't affect severity counts)
        result._total_refs = total_refs  # type: ignore[attr-defined]
    result._total_refs = total_refs  # type: ignore[attr-defined]
    result._missing_refs = missing_refs  # type: ignore[attr-defined]

    return result, all_referenced_lower


def check_wav_coverage(
    wav_names_in_zip: List[str],
    referenced_lower: Set[str],
) -> tuple:
    """
    Returns (result, list_of_unreferenced_wav_names, dead_weight_bytes_estimate).
    We can't easily get file sizes per-entry here so we return the paths only.
    """
    result = CheckResult()
    unreferenced = []
    for wav in wav_names_in_zip:
        if wav.lower() not in referenced_lower:
            unreferenced.append(wav)
            result.add(WARNING, "Unreferenced WAV (dead weight)", wav)
    return result, unreferenced


def check_json_files(zf: zipfile.ZipFile, all_entries: List[str]) -> CheckResult:
    result = CheckResult()
    json_targets = ["expansion.json", "bundle_manifest.json"]
    entries_lower = {e.lower(): e for e in all_entries}

    for target in json_targets:
        actual = entries_lower.get(target)
        if actual is None:
            if target == "expansion.json":
                result.add(ERROR, f"Missing required file: {target}")
            # bundle_manifest.json is optional — skip silently
            continue
        try:
            with zf.open(actual) as f:
                json.load(f)
        except json.JSONDecodeError as e:
            result.add(CRITICAL, f"Invalid JSON: {actual}", str(e))
        except Exception as e:
            result.add(CRITICAL, f"Error reading {actual}", str(e))

    return result


def check_nested_zips(all_entries: List[str]) -> CheckResult:
    result = CheckResult()
    for entry in all_entries:
        lower = entry.lower()
        if lower.endswith(".zip") or lower.endswith(".xpn"):
            result.add(CRITICAL, "Nested ZIP/XPN inside pack (won't load on MPC)", entry)
    return result


def check_required_files(
    xpm_names: List[str],
    wav_names: List[str],
    all_entries: List[str],
) -> CheckResult:
    result = CheckResult()
    entries_lower = {e.lower() for e in all_entries}

    if not xpm_names:
        result.add(CRITICAL, "No .xpm programs found in pack")
    if not wav_names:
        result.add(CRITICAL, "No WAV samples found in pack")
    if "expansion.json" not in entries_lower:
        # Already reported in check_json_files; skip duplicate
        pass

    return result


# ---------------------------------------------------------------------------
# Orchestration
# ---------------------------------------------------------------------------

def run_checks(pack_path: str) -> dict:
    """
    Run all checks and return a structured result dict.
    """
    pack_name = Path(pack_path).name
    results = {
        "pack": pack_name,
        "zip_ok": False,
        "total_size_mb": 0.0,
        "entry_count": 0,
        "xpm_count": 0,
        "wav_count": 0,
        "total_sample_refs": 0,
        "missing_refs": 0,
        "unreferenced_wavs": [],
        "issues": [],
    }

    # 1. ZIP integrity
    zf, total_bytes, entry_count, zip_result = check_zip_integrity(pack_path)
    results["issues"].extend(zip_result.issues)

    if zf is None:
        results["verdict"] = CRITICAL
        return results

    results["zip_ok"] = True
    results["total_size_mb"] = round(total_bytes / (1024 * 1024), 1)
    results["entry_count"] = entry_count

    all_entries = [e.filename for e in zf.infolist()]
    entry_map = collect_zip_entries(zf)

    xpm_names = [e for e in all_entries if e.lower().endswith(".xpm")]
    wav_names = [e for e in all_entries if e.lower().endswith(".wav")]
    wav_names_lower: Set[str] = {w.lower() for w in wav_names}

    results["xpm_count"] = len(xpm_names)
    results["wav_count"] = len(wav_names)

    # 2. Required files
    req_result = check_required_files(xpm_names, wav_names, all_entries)
    results["issues"].extend(req_result.issues)

    # 3. Nested ZIPs
    nested_result = check_nested_zips(all_entries)
    results["issues"].extend(nested_result.issues)

    # 4. JSON validity
    json_result = check_json_files(zf, all_entries)
    results["issues"].extend(json_result.issues)

    # 5. XPM XML validity + WAV reference checks (combined)
    xpm_ref_result, referenced_lower = check_xpm_wav_references(
        zf, xpm_names, wav_names_lower, entry_map
    )
    results["issues"].extend(xpm_ref_result.issues)
    results["total_sample_refs"] = getattr(xpm_ref_result, "_total_refs", 0)
    results["missing_refs"] = getattr(xpm_ref_result, "_missing_refs", 0)

    # 6. WAV coverage (dead weight)
    wav_cov_result, unreferenced = check_wav_coverage(wav_names, referenced_lower)
    results["issues"].extend(wav_cov_result.issues)
    results["unreferenced_wavs"] = unreferenced

    zf.close()

    # Determine verdict
    criticals = sum(1 for i in results["issues"] if i.severity == CRITICAL)
    warnings = sum(1 for i in results["issues"] if i.severity in (WARNING, ERROR))

    if criticals > 0:
        results["verdict"] = CRITICAL
    elif warnings > 0:
        results["verdict"] = "WARN"
    else:
        results["verdict"] = "PASS"

    results["critical_count"] = criticals
    results["warning_count"] = warnings

    return results


# ---------------------------------------------------------------------------
# Output formatters
# ---------------------------------------------------------------------------

SEVERITY_ICON = {
    CRITICAL: "✗",
    ERROR: "✗",
    WARNING: "⚠",
    INFO: "ℹ",
}


def format_text(data: dict) -> str:
    lines = []
    lines.append(f"INTEGRITY CHECK — {data['pack']}")
    lines.append("")

    if not data["zip_ok"]:
        for issue in data["issues"]:
            lines.append(f"  {SEVERITY_ICON.get(issue.severity, '?')} {issue.message}")
            if issue.detail:
                lines.append(f"    {issue.detail}")
        lines.append("")
        lines.append(f"VERDICT: CRITICAL (pack could not be opened)")
        return "\n".join(lines)

    lines.append(
        f"ZIP: ✓ Opened ({data['total_size_mb']} MB, {data['entry_count']} entries)"
    )
    lines.append("")

    # XPM references section
    lines.append("XPM References:")
    ref_issues = [
        i for i in data["issues"]
        if "Missing WAV" in i.message or "Case mismatch" in i.message or "XPM parse" in i.message
    ]
    total = data["total_sample_refs"]
    missing = data["missing_refs"]
    unref = data["unreferenced_wavs"]

    if missing == 0 and total > 0:
        lines.append(f"  ✓ All {total} SampleFile references resolved")
    elif missing > 0:
        lines.append(f"  ✗ {missing} of {total} SampleFile references are MISSING")
    elif total == 0 and data["xpm_count"] > 0:
        lines.append("  ⚠ No SampleFile references found in XPM files")

    if unref:
        lines.append(
            f"  ⚠ {len(unref)} WAV file(s) unreferenced (dead weight)"
        )
        for wav in unref[:10]:
            lines.append(f"    {wav}")
        if len(unref) > 10:
            lines.append(f"    ... and {len(unref) - 10} more")

    for issue in ref_issues:
        lines.append(f"  {SEVERITY_ICON.get(issue.severity, '?')} {issue.message}")
        if issue.detail:
            lines.append(f"    {issue.detail}")

    lines.append("")

    # JSON section
    json_issues = [i for i in data["issues"] if "JSON" in i.message or "expansion.json" in i.message or "bundle_manifest" in i.message]
    lines.append("JSON Files:")
    all_entries_lower = []  # we don't have this here; rely on issues
    json_ok_msgs = []
    if not any("expansion.json" in i.message for i in data["issues"]):
        json_ok_msgs.append("expansion.json — valid JSON")
    if not any("bundle_manifest" in i.message for i in data["issues"]):
        json_ok_msgs.append("bundle_manifest.json — valid JSON (if present)")
    for msg in json_ok_msgs:
        lines.append(f"  ✓ {msg}")
    for issue in json_issues:
        lines.append(f"  {SEVERITY_ICON.get(issue.severity, '?')} {issue.message}")
        if issue.detail:
            lines.append(f"    {issue.detail}")

    lines.append("")

    # Required files section
    lines.append("Required Files:")
    if data["xpm_count"] > 0:
        lines.append(f"  ✓ {data['xpm_count']} .xpm program(s) found")
    else:
        lines.append("  ✗ No .xpm programs found")
    if data["wav_count"] > 0:
        lines.append(f"  ✓ {data['wav_count']} WAV sample(s) found")
    else:
        lines.append("  ✗ No WAV samples found")

    # Other issues
    other_issues = [
        i for i in data["issues"]
        if i not in ref_issues
        and i not in json_issues
        and "Missing WAV" not in i.message
        and "WAV" not in i.message
        and "XPM" not in i.message
        and "xpm" not in i.message
        and "JSON" not in i.message
        and "expansion.json" not in i.message
        and "bundle_manifest" not in i.message
    ]
    if other_issues:
        lines.append("")
        lines.append("Other Issues:")
        for issue in other_issues:
            lines.append(f"  {SEVERITY_ICON.get(issue.severity, '?')} [{issue.severity}] {issue.message}")
            if issue.detail:
                lines.append(f"    {issue.detail}")

    lines.append("")
    verdict = data.get("verdict", "UNKNOWN")
    crit = data.get("critical_count", 0)
    warn = data.get("warning_count", 0)
    lines.append(f"VERDICT: {verdict} ({crit} critical, {warn} warnings)")

    return "\n".join(lines)


def format_json(data: dict) -> str:
    output = {
        "pack": data["pack"],
        "zip_ok": data["zip_ok"],
        "total_size_mb": data["total_size_mb"],
        "entry_count": data["entry_count"],
        "xpm_count": data["xpm_count"],
        "wav_count": data["wav_count"],
        "total_sample_refs": data["total_sample_refs"],
        "missing_refs": data["missing_refs"],
        "unreferenced_wavs": data["unreferenced_wavs"],
        "verdict": data.get("verdict", "UNKNOWN"),
        "critical_count": data.get("critical_count", 0),
        "warning_count": data.get("warning_count", 0),
        "issues": [
            {
                "severity": i.severity,
                "message": i.message,
                "detail": i.detail,
            }
            for i in data["issues"]
        ],
    }
    return json.dumps(output, indent=2)


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def main() -> None:
    parser = argparse.ArgumentParser(
        description="Deep structural integrity check on a .xpn pack.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument("pack", help="Path to the .xpn pack file")
    parser.add_argument(
        "--format",
        choices=["text", "json"],
        default="text",
        help="Output format (default: text)",
    )
    parser.add_argument(
        "--strict",
        action="store_true",
        help="Exit 1 on WARNING/ERROR, exit 2 on CRITICAL",
    )
    args = parser.parse_args()

    pack_path = args.pack
    if not Path(pack_path).exists():
        print(f"Error: file not found: {pack_path}", file=sys.stderr)
        sys.exit(2)

    data = run_checks(pack_path)

    if args.format == "json":
        print(format_json(data))
    else:
        print(format_text(data))

    verdict = data.get("verdict", "UNKNOWN")
    if args.strict:
        if verdict == CRITICAL:
            sys.exit(2)
        elif verdict == "WARN":
            sys.exit(1)
    else:
        if verdict == CRITICAL:
            sys.exit(2)


if __name__ == "__main__":
    main()
