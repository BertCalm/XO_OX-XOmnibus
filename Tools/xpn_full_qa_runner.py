#!/usr/bin/env python3
"""
xpn_full_qa_runner.py — XO_OX Master QA Orchestrator

Chains all XO_OX QA tools against a .xpn pack in sequence and produces
a single unified report covering manifest, samples, cover art, deduplication,
and drum kit programs.

Tools invoked (as subprocesses):
    1. xpn_manifest_validator.py  --xpn {xpn} --format json
    2. xpn_sample_audit.py        --xpn {xpn} --format json
    3. xpn_cover_art_audit.py     --xpn {xpn} --format json
    4. xpn_dedup_checker.py       --check-xpn {xpn}  (stdout-based)
    5. xpn_kit_validator.py       {xpm} --format json  (per drum .xpm)

Drum program detection: filenames starting with "kit_" or "drum_", or XPM
files containing a <Pad> element.

Scoring: start at 100, deduct 10 per ERROR, 3 per WARNING.

Overall status:
    PASS  — no ERRORs, no WARNINGs
    WARN  — no ERRORs, at least one WARNING
    FAIL  — at least one ERROR

Usage:
    python xpn_full_qa_runner.py --xpn pack.xpn [--tools-dir ./Tools] \
        [--format text|json] [--strict]

Options:
    --xpn          Path to the .xpn ZIP pack to validate (required)
    --tools-dir    Directory containing the XO_OX tool scripts
                   (default: same directory as this script)
    --format       Output format: text (default) or json
    --strict       Exit with code 1 if overall status is WARN or FAIL
"""

import argparse
import json
import os
import re
import subprocess
import sys
import tempfile
import zipfile
from pathlib import Path
from typing import Any, Dict, List, Optional, Tuple

TOOL_TIMEOUT = 30  # seconds per subprocess call

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def _tools_dir_default() -> str:
    return str(Path(__file__).parent)


def _run(cmd: List[str], timeout: int = TOOL_TIMEOUT) -> Tuple[int, str, str]:
    """Run a subprocess and return (returncode, stdout, stderr)."""
    try:
        result = subprocess.run(
            cmd,
            capture_output=True,
            text=True,
            timeout=timeout,
        )
        return result.returncode, result.stdout, result.stderr
    except subprocess.TimeoutExpired:
        return -1, "", f"TIMEOUT after {timeout}s: {' '.join(cmd)}"
    except FileNotFoundError as exc:
        return -1, "", f"Tool not found: {exc}"


def _parse_json_output(stdout: str, tool_name: str) -> Optional[dict]:
    """Try to parse JSON from tool stdout; return None on failure."""
    try:
        return json.loads(stdout)
    except json.JSONDecodeError:
        return None


def _issues_from_json(data: Optional[dict]) -> List[dict]:
    """
    Extract a flat list of issue dicts from the varied JSON schemas
    produced by individual tools.  Each returned item has at minimum:
        {"severity": "ERROR"|"WARNING"|"INFO", "message": str}
    """
    if data is None:
        return []

    issues: List[dict] = []

    # xpn_manifest_validator: {"issues": [...], "status": ...}
    if "issues" in data and isinstance(data["issues"], list):
        for item in data["issues"]:
            if isinstance(item, dict):
                issues.append({
                    "severity": item.get("severity", "INFO"),
                    "message": item.get("message", str(item)),
                    "field": item.get("field", ""),
                })
        return issues

    # xpn_sample_audit / xpn_cover_art_audit: {"samples": [{..., "issues": [...]}]}
    for key in ("samples", "files", "programs"):
        if key in data and isinstance(data[key], list):
            for entry in data[key]:
                for item in entry.get("issues", []):
                    if isinstance(item, dict):
                        issues.append({
                            "severity": item.get("severity", "INFO"),
                            "message": f"[{entry.get('name', entry.get('file', '?'))}] "
                                       + item.get("message", str(item)),
                        })
            if issues:
                return issues

    # xpn_kit_validator: {"checks": [{"severity": ..., "message": ...}]}
    if "checks" in data and isinstance(data["checks"], list):
        for item in data["checks"]:
            if isinstance(item, dict) and item.get("severity") in ("FAIL", "WARN", "INFO"):
                issues.append({
                    "severity": "ERROR" if item.get("severity") == "FAIL" else item.get("severity", "INFO"),
                    "message": item.get("message", str(item)),
                })
        return issues

    return issues


def _status_from_issues(issues: List[dict]) -> str:
    severities = {i.get("severity", "INFO") for i in issues}
    if "ERROR" in severities or "FAIL" in severities:
        return "FAIL"
    if "WARNING" in severities or "WARN" in severities:
        return "WARN"
    return "PASS"


def _deduct_score(issues: List[dict]) -> int:
    deduction = 0
    for i in issues:
        sev = i.get("severity", "INFO")
        if sev in ("ERROR", "FAIL"):
            deduction += 10
        elif sev in ("WARNING", "WARN"):
            deduction += 3
    return deduction


# ---------------------------------------------------------------------------
# Individual section runners
# ---------------------------------------------------------------------------

def run_manifest(xpn: str, tools_dir: str) -> dict:
    tool = os.path.join(tools_dir, "xpn_manifest_validator.py")
    rc, stdout, stderr = _run([sys.executable, tool, "--xpn", xpn, "--format", "json"])
    data = _parse_json_output(stdout, "manifest")
    if data is None:
        issues = [{"severity": "ERROR", "message": f"Tool failed or non-JSON output. stderr: {stderr.strip()[:200]}"}]
    else:
        issues = _issues_from_json(data)
    return {"status": _status_from_issues(issues), "issues": issues}


def run_samples(xpn: str, tools_dir: str) -> dict:
    tool = os.path.join(tools_dir, "xpn_sample_audit.py")
    rc, stdout, stderr = _run([sys.executable, tool, "--xpn", xpn, "--format", "json"])
    data = _parse_json_output(stdout, "samples")
    if data is None:
        issues = [{"severity": "ERROR", "message": f"Tool failed or non-JSON output. stderr: {stderr.strip()[:200]}"}]
    else:
        issues = _issues_from_json(data)
    return {"status": _status_from_issues(issues), "issues": issues}


def run_cover_art(xpn: str, tools_dir: str) -> dict:
    tool = os.path.join(tools_dir, "xpn_cover_art_audit.py")
    rc, stdout, stderr = _run([sys.executable, tool, "--xpn", xpn, "--format", "json"])
    data = _parse_json_output(stdout, "cover_art")
    if data is None:
        issues = [{"severity": "ERROR", "message": f"Tool failed or non-JSON output. stderr: {stderr.strip()[:200]}"}]
    else:
        issues = _issues_from_json(data)
    return {"status": _status_from_issues(issues), "issues": issues}


def run_dedup(xpn: str, tools_dir: str) -> dict:
    tool = os.path.join(tools_dir, "xpn_dedup_checker.py")
    rc, stdout, stderr = _run([sys.executable, tool, "--check-xpn", xpn])
    issues: List[dict] = []
    for line in stdout.splitlines():
        line = line.strip()
        if not line:
            continue
        # dedup_checker marks duplicates with "DUPLICATE" or "WARN" in output
        low = line.lower()
        if "duplicate" in low or "duplicate" in low:
            issues.append({"severity": "WARNING", "message": line})
        elif "error" in low:
            issues.append({"severity": "ERROR", "message": line})
    if rc not in (0, 1) and rc != 0:
        if stderr.strip():
            issues.append({"severity": "ERROR", "message": f"dedup tool error: {stderr.strip()[:200]}"})
    return {"status": _status_from_issues(issues), "issues": issues}


def _is_drum_xpm(name: str, content: bytes) -> bool:
    """Return True if this XPM should be validated as a drum program."""
    basename = os.path.basename(name).lower()
    if basename.startswith("kit_") or basename.startswith("drum_"):
        return True
    try:
        text = content.decode("utf-8", errors="replace")
        if "<Pad>" in text or "<pad>" in text.lower():
            return True
    except Exception as e:
        print(f'[WARN] Report write failed: {e}', file=sys.stderr)
    return False


def run_kit_programs(xpn: str, tools_dir: str) -> dict:
    tool = os.path.join(tools_dir, "xpn_kit_validator.py")
    all_issues: List[dict] = []
    programs_checked = 0

    try:
        with zipfile.ZipFile(xpn, "r") as zf:
            xpm_names = [n for n in zf.namelist() if n.lower().endswith(".xpm")]
            with tempfile.TemporaryDirectory() as tmpdir:
                for xpm_name in xpm_names:
                    content = zf.read(xpm_name)
                    if not _is_drum_xpm(xpm_name, content):
                        continue
                    out_path = os.path.join(tmpdir, os.path.basename(xpm_name))
                    with open(out_path, "wb") as fh:
                        fh.write(content)
                    programs_checked += 1
                    rc, stdout, stderr = _run(
                        [sys.executable, tool, out_path, "--format", "json"]
                    )
                    data = _parse_json_output(stdout, "kit_validator")
                    if data is None:
                        all_issues.append({
                            "severity": "ERROR",
                            "message": f"[{os.path.basename(xpm_name)}] kit validator failed: {stderr.strip()[:200]}",
                        })
                    else:
                        for issue in _issues_from_json(data):
                            issue.setdefault("message", "")
                            issue["message"] = f"[{os.path.basename(xpm_name)}] {issue['message']}"
                            all_issues.append(issue)
    except zipfile.BadZipFile as exc:
        all_issues.append({"severity": "ERROR", "message": f"Cannot open .xpn: {exc}"})

    if programs_checked == 0:
        all_issues.append({"severity": "INFO", "message": "No drum kit programs detected in pack."})

    return {"status": _status_from_issues(all_issues), "issues": all_issues}


# ---------------------------------------------------------------------------
# Report assembly
# ---------------------------------------------------------------------------

def build_report(xpn: str, sections: Dict[str, dict]) -> dict:
    all_issues = [i for sec in sections.values() for i in sec["issues"]]
    raw_score = 100 - sum(_deduct_score(sec["issues"]) for sec in sections.values())
    score = max(0, raw_score)

    statuses = {sec["status"] for sec in sections.values()}
    if "FAIL" in statuses:
        overall = "FAIL"
    elif "WARN" in statuses:
        overall = "WARN"
    else:
        overall = "PASS"

    return {
        "pack": os.path.basename(xpn),
        "overall": overall,
        "score": score,
        "sections": sections,
    }


# ---------------------------------------------------------------------------
# Output formatters
# ---------------------------------------------------------------------------

PASS_ICON  = "[PASS]"
WARN_ICON  = "[WARN]"
FAIL_ICON  = "[FAIL]"
INFO_ICON  = "[INFO]"

STATUS_ICON = {"PASS": PASS_ICON, "WARN": WARN_ICON, "FAIL": FAIL_ICON}
SEV_ICON    = {"ERROR": FAIL_ICON, "FAIL": FAIL_ICON, "WARNING": WARN_ICON,
               "WARN": WARN_ICON, "INFO": INFO_ICON}

SECTION_LABELS = {
    "manifest":     "Manifest",
    "samples":      "Samples",
    "cover_art":    "Cover Art",
    "dedup":        "Deduplication",
    "kit_programs": "Kit Programs",
}


def format_text(report: dict) -> str:
    lines: List[str] = []
    sep = "-" * 60

    lines.append(sep)
    lines.append(f"  XO_OX Full QA Report")
    lines.append(f"  Pack  : {report['pack']}")
    lines.append(f"  Score : {report['score']}/100")
    lines.append(f"  Status: {STATUS_ICON.get(report['overall'], report['overall'])} {report['overall']}")
    lines.append(sep)

    for key, label in SECTION_LABELS.items():
        sec = report["sections"].get(key, {})
        status = sec.get("status", "PASS")
        icon = STATUS_ICON.get(status, status)
        issues = sec.get("issues", [])
        non_info = [i for i in issues if i.get("severity", "INFO") not in ("INFO",)]
        lines.append(f"  {icon}  {label}")
        if non_info:
            for i in non_info:
                sev = i.get("severity", "INFO")
                tag = SEV_ICON.get(sev, f"[{sev}]")
                msg = i.get("message", "")
                lines.append(f"         {tag} {msg}")
        else:
            info_items = [i for i in issues if i.get("severity") == "INFO"]
            if info_items:
                for i in info_items[:3]:
                    lines.append(f"         {INFO_ICON} {i.get('message', '')}")
            else:
                lines.append(f"         No issues found.")

    lines.append(sep)
    return "\n".join(lines)


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def main() -> int:
    parser = argparse.ArgumentParser(
        description="XO_OX Master QA Orchestrator — runs all QA tools against a .xpn pack.",
    )
    parser.add_argument("--xpn", required=True, help="Path to .xpn pack")
    parser.add_argument(
        "--tools-dir",
        default=None,
        help="Directory containing XO_OX tool scripts (default: same dir as this script)",
    )
    parser.add_argument(
        "--format",
        choices=["text", "json"],
        default="text",
        help="Output format (default: text)",
    )
    parser.add_argument(
        "--strict",
        action="store_true",
        help="Exit with code 1 if overall status is WARN or FAIL",
    )
    args = parser.parse_args()

    xpn = os.path.abspath(args.xpn)
    if not os.path.isfile(xpn):
        print(f"ERROR: .xpn file not found: {xpn}", file=sys.stderr)
        return 2

    tools_dir = os.path.abspath(args.tools_dir) if args.tools_dir else _tools_dir_default()

    sections: Dict[str, dict] = {}
    sections["manifest"]     = run_manifest(xpn, tools_dir)
    sections["samples"]      = run_samples(xpn, tools_dir)
    sections["cover_art"]    = run_cover_art(xpn, tools_dir)
    sections["dedup"]        = run_dedup(xpn, tools_dir)
    sections["kit_programs"] = run_kit_programs(xpn, tools_dir)

    report = build_report(xpn, sections)

    if args.format == "json":
        print(json.dumps(report, indent=2))
    else:
        print(format_text(report))

    if args.strict and report["overall"] in ("WARN", "FAIL"):
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
