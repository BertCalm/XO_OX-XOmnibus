#!/usr/bin/env python3
"""
xpn_batch_export.py — XO_OX Batch XPN Export Orchestrator

Runs the full oxport.py pipeline for multiple engines in sequence (or in
parallel), automating multi-engine pack batch exports from a single JSON
config file.

Usage:
    python xpn_batch_export.py --config batch.json
    python xpn_batch_export.py --config batch.json --skip-failed
    python xpn_batch_export.py --config batch.json --dry-run
    python xpn_batch_export.py --config batch.json --parallel 3

Config file format:
    {
        "batch_name": "Kitchen_Collection_v1",
        "output_base_dir": "./dist/kitchen/",
        "jobs": [
            {"engine": "OBLONG",  "wavs_dir": "./wavs/oblong/",  "kit_mode": "smart",    "version": "1.0"},
            {"engine": "OBESE",   "wavs_dir": "./wavs/obese/",   "kit_mode": "velocity", "version": "1.0"},
            {"engine": "ONSET",   "wavs_dir": "./wavs/onset/",   "kit_mode": "cycle",    "version": "1.0"}
        ]
    }

Optional job fields (all map directly to oxport.py flags):
    pack_name, collection, skip, strict_qa, choke_preset, tuning
"""

import argparse
import json
import os
import subprocess
import sys
import time
from pathlib import Path
from typing import Optional

# ---------------------------------------------------------------------------
# Locate oxport.py relative to this file
# ---------------------------------------------------------------------------
TOOLS_DIR = Path(__file__).parent.resolve()
OXPORT = TOOLS_DIR / "oxport.py"


# ---------------------------------------------------------------------------
# Data structures
# ---------------------------------------------------------------------------

class JobResult:
    def __init__(self, engine: str):
        self.engine = engine
        self.start_time: float = 0.0
        self.end_time: float = 0.0
        self.exit_code: Optional[int] = None
        self.output_xpn: Optional[Path] = None
        self.stderr_tail: list[str] = []

    @property
    def duration(self) -> float:
        return self.end_time - self.start_time

    @property
    def passed(self) -> bool:
        return self.exit_code == 0

    @property
    def status_label(self) -> str:
        if self.exit_code is None:
            return "RUNNING"
        return "PASS" if self.passed else "FAIL"


# ---------------------------------------------------------------------------
# Command builder
# ---------------------------------------------------------------------------

def build_command(job: dict, output_base_dir: Path) -> list[str]:
    """Build the oxport.py subprocess command for a single job dict."""
    engine = job["engine"]
    output_dir = output_base_dir / engine.lower()

    cmd = [sys.executable, str(OXPORT), "run",
           "--engine", engine,
           "--output-dir", str(output_dir)]

    if "wavs_dir" in job and job["wavs_dir"]:
        cmd += ["--wavs-dir", str(job["wavs_dir"])]

    if "kit_mode" in job and job["kit_mode"]:
        cmd += ["--kit-mode", str(job["kit_mode"])]

    if "version" in job and job["version"]:
        cmd += ["--version", str(job["version"])]

    if "pack_name" in job and job["pack_name"]:
        cmd += ["--pack-name", str(job["pack_name"])]

    if "collection" in job and job["collection"]:
        cmd += ["--collection", str(job["collection"])]

    if "skip" in job and job["skip"]:
        cmd += ["--skip", str(job["skip"])]

    if job.get("strict_qa"):
        cmd.append("--strict-qa")

    if "choke_preset" in job and job["choke_preset"]:
        cmd += ["--choke-preset", str(job["choke_preset"])]

    if "tuning" in job and job["tuning"]:
        cmd += ["--tuning", str(job["tuning"])]

    return cmd


# ---------------------------------------------------------------------------
# Single-job runner (synchronous)
# ---------------------------------------------------------------------------

def run_job(job: dict, output_base_dir: Path, dry_run: bool) -> JobResult:
    """Execute one job and return a populated JobResult."""
    engine = job["engine"]
    result = JobResult(engine)
    cmd = build_command(job, output_base_dir)

    if dry_run:
        print(f"  [DRY-RUN] {' '.join(cmd)}")
        result.exit_code = 0
        result.start_time = result.end_time = time.monotonic()
        return result

    print(f"  -> {engine}: starting …")
    result.start_time = time.monotonic()
    try:
        proc = subprocess.run(
            cmd,
            capture_output=True,
            text=True,
        )
        result.exit_code = proc.returncode
        stderr_lines = proc.stderr.splitlines()
        result.stderr_tail = stderr_lines[-10:] if len(stderr_lines) > 10 else stderr_lines
    except Exception as exc:
        result.exit_code = 1
        result.stderr_tail = [f"Exception launching subprocess: {exc}"]
    finally:
        result.end_time = time.monotonic()

    # Locate the output .xpn — check expected path
    expected_xpn_dir = output_base_dir / engine.lower()
    if expected_xpn_dir.exists():
        xpns = sorted(expected_xpn_dir.glob("*.xpn"))
        if xpns:
            result.output_xpn = xpns[-1]

    status = "PASS" if result.passed else "FAIL"
    print(f"  <- {engine}: {status} ({result.duration:.1f}s)")
    return result


# ---------------------------------------------------------------------------
# Parallel runner
# ---------------------------------------------------------------------------

def run_jobs_parallel(jobs: list[dict], output_base_dir: Path,
                      dry_run: bool, max_parallel: int,
                      skip_failed: bool) -> list[JobResult]:
    """
    Run up to max_parallel jobs concurrently using subprocess + polling.
    Returns results in original job order.
    """
    results: list[Optional[JobResult]] = [None] * len(jobs)
    # (index, job, Popen, start_time, stderr_accumulator)
    active: list[tuple[int, dict, subprocess.Popen, float, list[str]]] = []
    job_queue = list(enumerate(jobs))

    def _launch(idx: int, job: dict):
        cmd = build_command(job, output_base_dir)
        if dry_run:
            print(f"  [DRY-RUN] {' '.join(cmd)}")
            r = JobResult(job["engine"])
            r.exit_code = 0
            r.start_time = r.end_time = time.monotonic()
            results[idx] = r
            return None
        print(f"  -> {job['engine']}: starting …")
        proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
        return proc

    while job_queue or active:
        # Fill up active slots
        while job_queue and len(active) < max_parallel:
            idx, job = job_queue.pop(0)
            proc = _launch(idx, job)
            if proc is not None:
                active.append((idx, job, proc, time.monotonic(), []))

        # Poll active processes
        still_running = []
        for idx, job, proc, start_t, stderr_buf in active:
            ret = proc.poll()
            if ret is None:
                still_running.append((idx, job, proc, start_t, stderr_buf))
            else:
                # Collect remaining stderr
                _, stderr_data = proc.communicate()
                stderr_buf.extend(stderr_data.splitlines())

                r = JobResult(job["engine"])
                r.exit_code = ret
                r.start_time = start_t
                r.end_time = time.monotonic()
                r.stderr_tail = stderr_buf[-10:]

                expected_xpn_dir = output_base_dir / job["engine"].lower()
                if expected_xpn_dir.exists():
                    xpns = sorted(expected_xpn_dir.glob("*.xpn"))
                    if xpns:
                        r.output_xpn = xpns[-1]

                results[idx] = r
                status = "PASS" if r.passed else "FAIL"
                print(f"  <- {job['engine']}: {status} ({r.duration:.1f}s)")

                if not r.passed and not skip_failed:
                    # Cancel remaining
                    for _, _, p, _, _ in still_running:
                        p.terminate()
                    print("\n[BATCH] Job failed and --skip-failed not set. Aborting.")
                    # Populate remaining results as aborted
                    for remaining_idx, remaining_job in job_queue:
                        ar = JobResult(remaining_job["engine"])
                        ar.exit_code = -1
                        ar.stderr_tail = ["Aborted — earlier job failed"]
                        results[remaining_idx] = ar
                    job_queue.clear()
                    still_running.clear()
                    break

        active = still_running
        if active:
            time.sleep(0.25)

    return [r for r in results if r is not None]


# ---------------------------------------------------------------------------
# Summary table
# ---------------------------------------------------------------------------

def human_size(path: Optional[Path]) -> str:
    if path is None or not path.exists():
        return "—"
    size = path.stat().st_size
    for unit in ("B", "KB", "MB", "GB"):
        if size < 1024:
            return f"{size:.0f} {unit}"
        size /= 1024
    return f"{size:.1f} GB"


def print_summary(results: list[JobResult], batch_name: str, wall_time: float):
    col_w = [10, 8, 10, 50, 10]
    header = f"{'Engine':<{col_w[0]}}  {'Status':<{col_w[1]}}  {'Duration':<{col_w[2]}}  {'Output File':<{col_w[3]}}  {'Size':<{col_w[4]}}"
    divider = "-" * len(header)

    print()
    print(f"=== Batch Summary: {batch_name} ===")
    print(divider)
    print(header)
    print(divider)

    passed = 0
    failed = 0
    for r in results:
        xpn_name = r.output_xpn.name if r.output_xpn else "—"
        size_str = human_size(r.output_xpn)
        dur_str = f"{r.duration:.1f}s"
        print(f"{r.engine:<{col_w[0]}}  {r.status_label:<{col_w[1]}}  {dur_str:<{col_w[2]}}  {xpn_name:<{col_w[3]}}  {size_str:<{col_w[4]}}")
        if r.passed:
            passed += 1
        else:
            failed += 1

    print(divider)
    print(f"Total: {len(results)} jobs | Passed: {passed} | Failed: {failed} | Wall time: {wall_time:.1f}s")

    # Show stderr for failed jobs
    for r in results:
        if not r.passed and r.stderr_tail:
            print(f"\n--- {r.engine} stderr (last {len(r.stderr_tail)} lines) ---")
            for line in r.stderr_tail:
                print(f"  {line}")

    print()


# ---------------------------------------------------------------------------
# Entry point
# ---------------------------------------------------------------------------

def main():
    parser = argparse.ArgumentParser(
        description="Batch XPN export — runs oxport.py for multiple engines from a JSON config."
    )
    parser.add_argument("--config",       required=True, metavar="FILE",
                        help="Path to batch config JSON file.")
    parser.add_argument("--skip-failed",  action="store_true",
                        help="Continue batch even if a job fails.")
    parser.add_argument("--dry-run",      action="store_true",
                        help="Print commands without executing them.")
    parser.add_argument("--parallel",     type=int, default=1, metavar="N",
                        help="Run up to N jobs concurrently (default: 1 = sequential).")
    args = parser.parse_args()

    # Load config
    config_path = Path(args.config).resolve()
    if not config_path.exists():
        print(f"ERROR: Config file not found: {config_path}", file=sys.stderr)
        sys.exit(1)

    with open(config_path) as f:
        config = json.load(f)

    batch_name = config.get("batch_name", "batch")
    output_base_dir = Path(config.get("output_base_dir", "./dist")).resolve()
    jobs = config.get("jobs", [])

    if not jobs:
        print("ERROR: No jobs defined in config.", file=sys.stderr)
        sys.exit(1)

    if not OXPORT.exists():
        print(f"ERROR: oxport.py not found at {OXPORT}", file=sys.stderr)
        sys.exit(1)

    print(f"[BATCH] {batch_name} — {len(jobs)} job(s), parallel={args.parallel}, dry_run={args.dry_run}")
    print(f"[BATCH] Output base: {output_base_dir}")
    print()

    wall_start = time.monotonic()

    if args.parallel > 1:
        results = run_jobs_parallel(jobs, output_base_dir, args.dry_run,
                                    args.parallel, args.skip_failed)
    else:
        results = []
        for job in jobs:
            r = run_job(job, output_base_dir, args.dry_run)
            results.append(r)
            if not r.passed and not args.skip_failed:
                print("\n[BATCH] Job failed and --skip-failed not set. Aborting.")
                # Mark remaining as aborted
                remaining_engines = [j["engine"] for j in jobs[len(results):]]
                for eng in remaining_engines:
                    ar = JobResult(eng)
                    ar.exit_code = -1
                    ar.stderr_tail = ["Aborted — earlier job failed"]
                    results.append(ar)
                break

    wall_time = time.monotonic() - wall_start
    print_summary(results, batch_name, wall_time)

    failed_count = sum(1 for r in results if not r.passed)
    sys.exit(0 if failed_count == 0 else 1)


if __name__ == "__main__":
    main()
