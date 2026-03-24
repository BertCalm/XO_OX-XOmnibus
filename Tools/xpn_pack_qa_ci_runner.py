#!/usr/bin/env python3
"""
xpn_pack_qa_ci_runner.py — Single-command CI quality gate for the XOlokun preset fleet.

Orchestrates all major validation checks and reports a unified PASS/FAIL verdict.
Designed to be run before every push.

Usage:
    python Tools/xpn_pack_qa_ci_runner.py
    python Tools/xpn_pack_qa_ci_runner.py --presets-dir Presets/XOlokun
    python Tools/xpn_pack_qa_ci_runner.py --fast
    python Tools/xpn_pack_qa_ci_runner.py --report report.json
    python Tools/xpn_pack_qa_ci_runner.py --tools-dir /path/to/Tools

Exit codes:
    0 — All critical checks PASS
    1 — One or more critical checks FAIL
"""

import argparse
import json
import os
import subprocess
import sys
import time
from pathlib import Path

# ---------------------------------------------------------------------------
# Check definitions
# ---------------------------------------------------------------------------

# Each entry:
#   name        — display name
#   script      — filename inside tools_dir
#   args_fn     — callable(args) -> list[str] of extra CLI args
#   critical    — if True, failure causes overall FAIL
#   fast_skip   — if True, skip in --fast mode
# ---------------------------------------------------------------------------

def _dedup_args(args: argparse.Namespace) -> list:
    cmd = ["--mode", "name"]
    if args.presets_dir:
        cmd += ["--presets-dir", str(args.presets_dir)]
    return cmd


def _completeness_args(args: argparse.Namespace) -> list:
    cmd = ["--min-score", "6"]
    if args.fast:
        # In fast mode, only scan Entangled/ to keep it quick
        entangled = (args.presets_dir / "Entangled") if args.presets_dir else None
        if entangled and entangled.is_dir():
            cmd += ["--presets-dir", str(entangled)]
            return cmd
    if args.presets_dir:
        cmd += ["--presets-dir", str(args.presets_dir)]
    return cmd


def _naming_args(args: argparse.Namespace) -> list:
    # xpn_pack_naming_validator.py requires a positional pack_dir
    # Use presets_dir root or cwd if not set
    target = str(args.presets_dir) if args.presets_dir else "."
    return [target]


def _velocity_args(args: argparse.Namespace) -> list:
    # xpn_velocity_curve_tester.py requires a positional .xpm file.
    # We run it against any .xpm we can find under presets_dir.
    # If none found we return None to signal SKIP.
    search_root = args.presets_dir if args.presets_dir else Path(".")
    xpms = list(search_root.rglob("*.xpm"))
    if not xpms:
        return None  # type: ignore[return-value]  # sentinel: skip
    # Run against the first xpm found; caller iterates if needed
    return [str(xpms[0])]


CHECKS = [
    {
        "name": "Dedup (name collisions)",
        "script": "xpn_preset_dedup_finder.py",
        "args_fn": _dedup_args,
        "critical": True,
        "fast_skip": False,
    },
    {
        "name": "Completeness audit (score ≥ 6)",
        "script": "xpn_xometa_completeness_auditor.py",
        "args_fn": _completeness_args,
        "critical": True,
        "fast_skip": False,  # fast mode changes scope, not skip
    },
    {
        "name": "Pack naming conventions",
        "script": "xpn_pack_naming_validator.py",
        "args_fn": _naming_args,
        "critical": False,
        "fast_skip": True,
    },
    {
        "name": "Velocity curve rules",
        "script": "xpn_velocity_curve_tester.py",
        "args_fn": _velocity_args,
        "critical": False,
        "fast_skip": True,
    },
]

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

STATUS_PASS = "PASS"
STATUS_FAIL = "FAIL"
STATUS_SKIP = "SKIP"

ANSI_GREEN  = "\033[32m"
ANSI_RED    = "\033[31m"
ANSI_YELLOW = "\033[33m"
ANSI_RESET  = "\033[0m"
ANSI_BOLD   = "\033[1m"

_USE_COLOR = sys.stdout.isatty()


def colorize(text: str, color: str) -> str:
    if not _USE_COLOR:
        return text
    return f"{color}{text}{ANSI_RESET}"


def status_str(status: str) -> str:
    if status == STATUS_PASS:
        return colorize(STATUS_PASS, ANSI_GREEN)
    if status == STATUS_FAIL:
        return colorize(STATUS_FAIL, ANSI_RED)
    return colorize(STATUS_SKIP, ANSI_YELLOW)


def count_findings(stdout: str, returncode: int) -> int:
    """Heuristic: count lines that look like findings (warnings, errors, duplicates)."""
    if returncode == 0:
        return 0
    count = 0
    for line in stdout.splitlines():
        lower = line.lower()
        if any(kw in lower for kw in ("duplicate", "collision", "fail", "error", "warning",
                                       "missing", "below", "violation", "critical")):
            count += 1
    return max(count, 1) if returncode != 0 else 0


# ---------------------------------------------------------------------------
# Core runner
# ---------------------------------------------------------------------------

def run_check(check: dict, args: argparse.Namespace, tools_dir: Path) -> dict:
    """Run a single check. Returns a result dict."""
    script_path = tools_dir / check["script"]

    # SKIP: tool not found
    if not script_path.exists():
        return {
            "name": check["name"],
            "status": STATUS_SKIP,
            "elapsed": 0.0,
            "findings": 0,
            "stdout": "",
            "stderr": f"Tool not found: {script_path}",
            "returncode": None,
            "critical": check["critical"],
        }

    # SKIP: fast mode + fast_skip flag
    if args.fast and check["fast_skip"]:
        return {
            "name": check["name"],
            "status": STATUS_SKIP,
            "elapsed": 0.0,
            "findings": 0,
            "stdout": "",
            "stderr": "Skipped in --fast mode",
            "returncode": None,
            "critical": check["critical"],
        }

    # Build extra args
    extra = check["args_fn"](args)
    if extra is None:
        # args_fn returns None to signal SKIP (e.g., no .xpm files found)
        return {
            "name": check["name"],
            "status": STATUS_SKIP,
            "elapsed": 0.0,
            "findings": 0,
            "stdout": "",
            "stderr": "No target files found; skipping",
            "returncode": None,
            "critical": check["critical"],
        }

    cmd = [sys.executable, str(script_path)] + extra

    t0 = time.monotonic()
    try:
        result = subprocess.run(
            cmd,
            capture_output=True,
            text=True,
            cwd=str(script_path.parent.parent),  # repo root
        )
    except Exception as exc:
        elapsed = time.monotonic() - t0
        return {
            "name": check["name"],
            "status": STATUS_FAIL,
            "elapsed": round(elapsed, 2),
            "findings": 1,
            "stdout": "",
            "stderr": str(exc),
            "returncode": -1,
            "critical": check["critical"],
        }
    elapsed = time.monotonic() - t0

    status = STATUS_PASS if result.returncode == 0 else STATUS_FAIL
    findings = count_findings(result.stdout + result.stderr, result.returncode)

    return {
        "name": check["name"],
        "status": status,
        "elapsed": round(elapsed, 2),
        "findings": findings,
        "stdout": result.stdout,
        "stderr": result.stderr,
        "returncode": result.returncode,
        "critical": check["critical"],
    }


# ---------------------------------------------------------------------------
# Reporting
# ---------------------------------------------------------------------------

def print_banner(args: argparse.Namespace) -> None:
    mode_tag = " [FAST]" if args.fast else ""
    print(colorize(f"\n{'='*62}", ANSI_BOLD))
    print(colorize(f"  XOlokun Pack QA CI Runner{mode_tag}", ANSI_BOLD))
    print(colorize(f"{'='*62}", ANSI_BOLD))
    if args.presets_dir:
        print(f"  Presets dir : {args.presets_dir}")
    if args.tools_dir:
        print(f"  Tools dir   : {args.tools_dir}")
    print()


def print_results(results: list) -> None:
    # Column widths
    name_w    = max(len(r["name"]) for r in results) + 2
    status_w  = 6
    elapsed_w = 8
    count_w   = 10

    header = (
        f"  {'Check':<{name_w}}  {'Status':<{status_w}}  {'Elapsed':>{elapsed_w}}  "
        f"{'Findings':>{count_w}}"
    )
    sep = "  " + "-" * (name_w + status_w + elapsed_w + count_w + 8)

    print(colorize(header, ANSI_BOLD))
    print(sep)

    for r in results:
        crit_mark = "*" if r["critical"] and r["status"] == STATUS_FAIL else " "
        elapsed_str = f"{r['elapsed']:.2f}s" if r["elapsed"] else "-"
        line = (
            f"  {r['name']:<{name_w}}  {r['status']:<{status_w}}  "
            f"{elapsed_str:>{elapsed_w}}  {r['findings']:>{count_w}}{crit_mark}"
        )
        # Color the whole line based on status
        if r["status"] == STATUS_PASS:
            print(colorize(line, ANSI_GREEN))
        elif r["status"] == STATUS_FAIL:
            print(colorize(line, ANSI_RED))
        else:
            print(colorize(line, ANSI_YELLOW))

    print(sep)
    print("  * critical check failure")
    print()


def print_failure_detail(results: list) -> None:
    for r in results:
        if r["status"] != STATUS_FAIL:
            continue
        print(colorize(f"\n--- {r['name']} output ---", ANSI_BOLD))
        combined = (r["stdout"] or "") + (r["stderr"] or "")
        # Print up to 60 lines of output to avoid flooding the terminal
        lines = combined.splitlines()
        if len(lines) > 60:
            for line in lines[:60]:
                print(f"  {line}")
            print(colorize(f"  ... ({len(lines) - 60} more lines truncated)", ANSI_YELLOW))
        else:
            for line in lines:
                print(f"  {line}")


def overall_verdict(results: list) -> bool:
    """Return True (PASS) if all critical checks passed."""
    for r in results:
        if r["critical"] and r["status"] == STATUS_FAIL:
            return False
    return True


def write_report(results: list, path: Path, overall: bool) -> None:
    report = {
        "overall": STATUS_PASS if overall else STATUS_FAIL,
        "timestamp": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
        "checks": [
            {
                "name": r["name"],
                "status": r["status"],
                "elapsed_sec": r["elapsed"],
                "findings": r["findings"],
                "critical": r["critical"],
                "returncode": r["returncode"],
                "stdout": r["stdout"],
                "stderr": r["stderr"],
            }
            for r in results
        ],
    }
    path.write_text(json.dumps(report, indent=2))
    print(f"  Report written to: {path}")


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def build_parser() -> argparse.ArgumentParser:
    p = argparse.ArgumentParser(
        description="CI quality gate — runs all major validation checks on the preset fleet.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    p.add_argument(
        "--presets-dir",
        type=Path,
        default=None,
        metavar="DIR",
        help="Path to Presets/XOlokun/ (auto-detected if not set).",
    )
    p.add_argument(
        "--tools-dir",
        type=Path,
        default=None,
        metavar="DIR",
        help="Path to Tools/ directory (default: directory of this script).",
    )
    p.add_argument(
        "--fast",
        action="store_true",
        help="Fast mode: skip slow checks; completeness scan limited to Entangled/ only.",
    )
    p.add_argument(
        "--report",
        type=Path,
        default=None,
        metavar="FILE",
        help="Write JSON report of all findings to this file.",
    )
    p.add_argument(
        "--verbose",
        action="store_true",
        help="Print full output from each check even if it passes.",
    )
    return p


def auto_detect_paths(args: argparse.Namespace) -> None:
    """Fill in tools_dir and presets_dir from the script's own location if not set."""
    script_path = Path(__file__).resolve()

    if args.tools_dir is None:
        args.tools_dir = script_path.parent

    if args.presets_dir is None:
        # Walk up from Tools/ to find repo root, then look for Presets/XOlokun
        repo_root = args.tools_dir.parent
        candidates = [
            repo_root / "Presets" / "XOlokun",
            repo_root / "Presets",
        ]
        for c in candidates:
            if c.is_dir():
                args.presets_dir = c
                break
        # If still None, leave it None — individual tools will use their own defaults


def main() -> int:
    parser = build_parser()
    args = parser.parse_args()
    auto_detect_paths(args)

    print_banner(args)

    results = []
    total_checks = len(CHECKS)
    for i, check in enumerate(CHECKS, 1):
        label = f"[{i}/{total_checks}] {check['name']}"
        print(f"  Running: {label} ...", end="", flush=True)
        result = run_check(check, args, args.tools_dir)
        results.append(result)
        status_label = status_str(result["status"])
        print(f"\r  {label:<55}  {status_label}")

        if args.verbose and result["stdout"]:
            for line in result["stdout"].splitlines():
                print(f"      {line}")

    print()
    print_results(results)

    # Always show failure detail for failed checks
    failed = [r for r in results if r["status"] == STATUS_FAIL]
    if failed:
        print_failure_detail(results)

    overall = overall_verdict(results)

    if args.report:
        write_report(results, args.report, overall)

    # Final verdict
    print()
    if overall:
        print(colorize("  OVERALL: PASS", ANSI_GREEN + ANSI_BOLD))
    else:
        crit_fails = [r["name"] for r in results if r["critical"] and r["status"] == STATUS_FAIL]
        print(colorize("  OVERALL: FAIL", ANSI_RED + ANSI_BOLD))
        print(colorize(f"  Critical failures: {', '.join(crit_fails)}", ANSI_RED))
    print()

    return 0 if overall else 1


if __name__ == "__main__":
    sys.exit(main())
