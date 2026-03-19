#!/usr/bin/env python3
"""
xpn_tide_tables_build_validator.py

Validates a TIDE TABLES XPN pack directory against the spec.
TIDE TABLES is the free gateway XPN pack with 4 programs:
  SURGE   — ONSET drum kit (DrumProgram)
  DRIFT   — ODYSSEY melodic (KeygroupProgram)
  CURRENT — coupling demo (references 2+ engine sources)
  SWELL   — OPAL foam (KeygroupProgram)

Usage:
  python xpn_tide_tables_build_validator.py --pack-dir /path/to/TIDE_TABLES
  python xpn_tide_tables_build_validator.py --pack-dir /path/to/TIDE_TABLES --strict

Exit codes:
  0 — all checks pass
  1 — one or more checks failed
"""

import argparse
import json
import sys
import xml.etree.ElementTree as ET
from pathlib import Path

# ── Constants ─────────────────────────────────────────────────────────────────

REQUIRED_PROGRAMS = ["SURGE.xpm", "DRIFT.xpm", "CURRENT.xpm", "SWELL.xpm"]
DRUM_PROGRAMS = {"SURGE.xpm"}
KEYGROUP_PROGRAMS = {"DRIFT.xpm", "SWELL.xpm"}
MAX_SAMPLE_SIZE_MB = 20
MAX_TOTAL_SIZE_MB = 200
MIN_SAMPLE_COUNT = 16
GUIDE_FILES = {"README.md", "TIDE_TABLES_GUIDE.md"}

ANSI_GREEN = "\033[92m"
ANSI_RED = "\033[91m"
ANSI_YELLOW = "\033[93m"
ANSI_CYAN = "\033[96m"
ANSI_BOLD = "\033[1m"
ANSI_RESET = "\033[0m"


# ── Helpers ───────────────────────────────────────────────────────────────────

def fmt_pass(msg: str) -> str:
    return f"  {ANSI_GREEN}PASS{ANSI_RESET}  {msg}"

def fmt_fail(msg: str) -> str:
    return f"  {ANSI_RED}FAIL{ANSI_RESET}  {msg}"

def fmt_warn(msg: str) -> str:
    return f"  {ANSI_YELLOW}WARN{ANSI_RESET}  {msg}"

def fmt_info(msg: str) -> str:
    return f"  {ANSI_CYAN}INFO{ANSI_RESET}  {msg}"

def bytes_to_mb(b: int) -> float:
    return b / (1024 * 1024)

def dir_size_bytes(path: Path) -> int:
    total = 0
    for f in path.rglob("*"):
        if f.is_file():
            total += f.stat().st_size
    return total

def parse_xpm(xpm_path: Path):
    """Parse an XPM file and return the root Element, or None on failure."""
    try:
        tree = ET.parse(xpm_path)
        return tree.getroot()
    except ET.ParseError as e:
        return None

def get_program_type(root):
    """Extract the Program Type attribute from an XPM root element."""
    if root is None:
        return None
    # XPM structure: <MPCVObject><Version/><Program type="DrumProgram">...</Program></MPCVObject>
    program = root.find(".//Program")
    if program is not None:
        return program.get("type")
    # Some XPM files have the type on the root
    return root.get("type")

def collect_sample_refs(root) -> list[str]:
    """Collect all SampleFile or FileName values referenced in an XPM."""
    if root is None:
        return []
    refs = []
    for elem in root.iter():
        tag = elem.tag.lower()
        if tag in ("samplefile", "filename") and elem.text and elem.text.strip():
            refs.append(elem.text.strip())
    return refs

def check_keytrack_rootnote_velstart(root) -> list[str]:
    """
    Return a list of violation strings for pads/layers that fail:
      KeyTrack != True, RootNote != 0, VelStart != 0
    """
    if root is None:
        return ["XPM parse failed — cannot check KeyTrack/RootNote/VelStart"]
    issues = []

    for kt in root.iter("KeyTrack"):
        val = (kt.text or "").strip()
        if val.lower() not in ("true", "1", "yes", ""):
            issues.append(f"KeyTrack={val!r} (expected True)")

    for rn in root.iter("RootNote"):
        val = (rn.text or "").strip()
        if val not in ("0", ""):
            issues.append(f"RootNote={val!r} (expected 0)")

    for vs in root.iter("VelStart"):
        val = (vs.text or "").strip()
        if val not in ("0", ""):
            issues.append(f"VelStart={val!r} (expected 0)")

    return issues


# ── Individual checks ─────────────────────────────────────────────────────────

def check_expansion_json(pack_dir: Path) -> tuple[bool, list[str]]:
    lines = []
    path = pack_dir / "expansion.json"
    if not path.exists():
        lines.append(fmt_fail("expansion.json not found"))
        return False, lines

    try:
        with open(path) as f:
            data = json.load(f)
    except json.JSONDecodeError as e:
        lines.append(fmt_fail(f"expansion.json JSON parse error: {e}"))
        return False, lines

    ok = True

    name = data.get("name", "")
    if name == "TIDE TABLES":
        lines.append(fmt_pass(f'expansion.json name="TIDE TABLES"'))
    else:
        lines.append(fmt_fail(f'expansion.json name={name!r} (expected "TIDE TABLES")'))
        ok = False

    exp_type = data.get("type", "")
    if exp_type == "Expansion":
        lines.append(fmt_pass('expansion.json type="Expansion"'))
    else:
        lines.append(fmt_fail(f'expansion.json type={exp_type!r} (expected "Expansion")'))
        ok = False

    tier = data.get("tier", "")
    if tier == "free":
        lines.append(fmt_pass('expansion.json tier="free"'))
    else:
        lines.append(fmt_fail(f'expansion.json tier={tier!r} (expected "free")'))
        ok = False

    version = data.get("version", "")
    if version:
        lines.append(fmt_pass(f'expansion.json version present: {version!r}'))
    else:
        lines.append(fmt_fail("expansion.json version field missing or empty"))
        ok = False

    return ok, lines


def check_program_files(pack_dir: Path) -> tuple[bool, list[str]]:
    lines = []
    ok = True

    # Gather all .xpm files in the pack root
    found_xpms = {f.name for f in pack_dir.glob("*.xpm")}
    # Also check Programs/ subdirectory (common MPC layout)
    programs_dir = pack_dir / "Programs"
    if programs_dir.exists():
        for f in programs_dir.glob("*.xpm"):
            found_xpms.add(f.name)

    missing = [p for p in REQUIRED_PROGRAMS if p not in found_xpms]
    extra = [p for p in found_xpms if p not in set(REQUIRED_PROGRAMS)]

    if not missing:
        lines.append(fmt_pass(f"All 4 required .xpm files present: {', '.join(REQUIRED_PROGRAMS)}"))
    else:
        for m in missing:
            lines.append(fmt_fail(f"Missing required program file: {m}"))
        ok = False

    if extra:
        for e in extra:
            lines.append(fmt_warn(f"Extra .xpm file found (not in spec): {e}"))

    return ok, lines


def _resolve_xpm_path(pack_dir: Path, filename: str):
    """Find an XPM by filename in pack root or Programs/ subdir."""
    candidates = [
        pack_dir / filename,
        pack_dir / "Programs" / filename,
    ]
    for c in candidates:
        if c.exists():
            return c
    return None


def check_program_types(pack_dir: Path) -> tuple[bool, list[str]]:
    lines = []
    ok = True

    # SURGE must be DrumProgram
    for fname in DRUM_PROGRAMS:
        path = _resolve_xpm_path(pack_dir, fname)
        if path is None:
            lines.append(fmt_fail(f"{fname}: file not found — cannot check type"))
            ok = False
            continue
        root = parse_xpm(path)
        ptype = get_program_type(root)
        if ptype == "DrumProgram":
            lines.append(fmt_pass(f"{fname} type=DrumProgram"))
        else:
            lines.append(fmt_fail(f"{fname} type={ptype!r} (expected DrumProgram)"))
            ok = False

    # DRIFT and SWELL must be KeygroupProgram
    for fname in KEYGROUP_PROGRAMS:
        path = _resolve_xpm_path(pack_dir, fname)
        if path is None:
            lines.append(fmt_fail(f"{fname}: file not found — cannot check type"))
            ok = False
            continue
        root = parse_xpm(path)
        ptype = get_program_type(root)
        if ptype == "KeygroupProgram":
            lines.append(fmt_pass(f"{fname} type=KeygroupProgram"))
        else:
            lines.append(fmt_fail(f"{fname} type={ptype!r} (expected KeygroupProgram)"))
            ok = False

    return ok, lines


def check_current_coupling(pack_dir: Path) -> tuple[bool, list[str]]:
    """CURRENT.xpm must reference samples from at least 2 different engine source directories."""
    lines = []
    path = _resolve_xpm_path(pack_dir, "CURRENT.xpm")
    if path is None:
        lines.append(fmt_fail("CURRENT.xpm not found — cannot check coupling"))
        return False, lines

    root = parse_xpm(path)
    if root is None:
        lines.append(fmt_fail("CURRENT.xpm failed to parse — cannot check coupling"))
        return False, lines

    refs = collect_sample_refs(root)
    if not refs:
        lines.append(fmt_fail("CURRENT.xpm: no sample references found"))
        return False, lines

    # Derive source directories (top-level directory component of each path)
    source_dirs: set[str] = set()
    for ref in refs:
        # Normalise slashes and take first directory segment
        parts = ref.replace("\\", "/").split("/")
        if len(parts) > 1:
            source_dirs.add(parts[0])
        else:
            # Flat reference — use filename prefix heuristic
            stem = Path(ref).stem.lower()
            # Look for engine tags like onset_, opal_, drift_ etc.
            for tag in ("onset", "opal", "drift", "surge", "swell", "current",
                        "odyssey", "overworld", "dub", "overbite", "oblong"):
                if stem.startswith(tag):
                    source_dirs.add(tag)
                    break
            else:
                source_dirs.add(ref)  # treat as unique source if no match

    lines.append(fmt_info(f"CURRENT.xpm sample source dirs detected: {sorted(source_dirs)}"))

    if len(source_dirs) >= 2:
        lines.append(fmt_pass(f"CURRENT.xpm references {len(source_dirs)} distinct engine sources (coupling confirmed)"))
        return True, lines
    else:
        lines.append(fmt_fail(
            f"CURRENT.xpm references only {len(source_dirs)} engine source(s) — "
            "coupling demo requires at least 2 different engine sample sources"
        ))
        return False, lines


def check_xpm_params(pack_dir: Path) -> tuple[bool, list[str]]:
    """All XPM files must pass KeyTrack=True, RootNote=0, VelStart=0."""
    lines = []
    ok = True
    for fname in REQUIRED_PROGRAMS:
        path = _resolve_xpm_path(pack_dir, fname)
        if path is None:
            lines.append(fmt_warn(f"{fname}: not found — skipping param check"))
            continue
        root = parse_xpm(path)
        issues = check_keytrack_rootnote_velstart(root)
        if not issues:
            lines.append(fmt_pass(f"{fname}: KeyTrack/RootNote/VelStart all valid"))
        else:
            for issue in issues:
                lines.append(fmt_fail(f"{fname}: {issue}"))
            ok = False
    return ok, lines


def check_guide_file(pack_dir: Path) -> tuple[bool, list[str]]:
    lines = []
    found = [g for g in GUIDE_FILES if (pack_dir / g).exists()]
    if found:
        lines.append(fmt_pass(f"Guide file present: {', '.join(found)}"))
        return True, lines
    else:
        lines.append(fmt_fail(
            f"No guide file found. Expected one of: {', '.join(sorted(GUIDE_FILES))}"
        ))
        return False, lines


def check_samples(pack_dir: Path, strict: bool) -> tuple[bool, list[str]]:
    lines = []
    ok = True

    # Find samples directory (common layouts)
    samples_dirs = []
    for candidate in ["Samples", "samples", "Audio", "audio"]:
        d = pack_dir / candidate
        if d.exists() and d.is_dir():
            samples_dirs.append(d)

    if not samples_dirs:
        lines.append(fmt_fail("No samples/ directory found in pack root"))
        return False, lines

    wav_files: list[Path] = []
    for sd in samples_dirs:
        wav_files.extend(sd.rglob("*.wav"))
        wav_files.extend(sd.rglob("*.WAV"))
        wav_files.extend(sd.rglob("*.aif"))
        wav_files.extend(sd.rglob("*.aiff"))

    lines.append(fmt_info(f"Sample files found: {len(wav_files)}"))

    if len(wav_files) >= MIN_SAMPLE_COUNT:
        lines.append(fmt_pass(f"Sample count {len(wav_files)} >= {MIN_SAMPLE_COUNT} minimum"))
    else:
        lines.append(fmt_fail(f"Only {len(wav_files)} sample files found — need at least {MIN_SAMPLE_COUNT}"))
        ok = False

    # Per-file size check
    oversized = []
    for wav in wav_files:
        size_mb = bytes_to_mb(wav.stat().st_size)
        if size_mb > MAX_SAMPLE_SIZE_MB:
            oversized.append((wav.name, size_mb))

    if not oversized:
        lines.append(fmt_pass(f"All samples under {MAX_SAMPLE_SIZE_MB}MB limit"))
    else:
        for name, size in oversized:
            lines.append(fmt_fail(f"Sample over {MAX_SAMPLE_SIZE_MB}MB: {name} ({size:.1f}MB)"))
        ok = False

    return ok, lines


def check_total_size(pack_dir: Path) -> tuple[bool, list[str], float]:
    lines = []
    total_bytes = dir_size_bytes(pack_dir)
    total_mb = bytes_to_mb(total_bytes)

    if total_mb <= MAX_TOTAL_SIZE_MB:
        lines.append(fmt_pass(f"Total pack size {total_mb:.1f}MB <= {MAX_TOTAL_SIZE_MB}MB limit"))
        return True, lines, total_mb
    else:
        lines.append(fmt_fail(f"Total pack size {total_mb:.1f}MB exceeds {MAX_TOTAL_SIZE_MB}MB limit"))
        return False, lines, total_mb


def build_file_inventory(pack_dir: Path) -> list[str]:
    lines = []
    lines.append(f"\n  {'File':<50} {'Size':>10}")
    lines.append(f"  {'-'*50} {'-'*10}")
    for f in sorted(pack_dir.rglob("*")):
        if f.is_file():
            rel = f.relative_to(pack_dir)
            size_mb = bytes_to_mb(f.stat().st_size)
            lines.append(f"  {str(rel):<50} {size_mb:>9.2f}M")
    return lines


# ── Main ──────────────────────────────────────────────────────────────────────

def run_validation(pack_dir: Path, strict: bool) -> int:
    print(f"\n{ANSI_BOLD}{ANSI_CYAN}═══ TIDE TABLES Build Validator ═══{ANSI_RESET}")
    print(f"  Pack directory: {pack_dir}\n")

    if not pack_dir.exists() or not pack_dir.is_dir():
        print(fmt_fail(f"Pack directory does not exist: {pack_dir}"))
        print()
        print(f"  {ANSI_YELLOW}TIDE TABLES pack not yet built.{ANSI_RESET}")
        print()
        print("  To build the pack, run:")
        print(f"    python Tools/xpn_free_pack_pipeline.py --output-dir {pack_dir}")
        print()
        print("  The pipeline will generate:")
        print("    • SURGE.xpm   — ONSET drum kit")
        print("    • DRIFT.xpm   — ODYSSEY melodic program")
        print("    • CURRENT.xpm — coupling demo (ONSET + ODYSSEY)")
        print("    • SWELL.xpm   — OPAL foam program")
        print("    • Samples/    — all referenced WAV files")
        print("    • expansion.json")
        print("    • README.md / TIDE_TABLES_GUIDE.md")
        return 1

    checks: list[tuple[str, bool, list[str]]] = []
    total_mb = 0.0

    # 1. expansion.json
    ok, lines = check_expansion_json(pack_dir)
    checks.append(("expansion.json metadata", ok, lines))

    # 2. Program files present
    ok, lines = check_program_files(pack_dir)
    checks.append(("Program files (4 required)", ok, lines))

    # 3. Program types
    ok, lines = check_program_types(pack_dir)
    checks.append(("Program types (DrumProgram / KeygroupProgram)", ok, lines))

    # 4. CURRENT coupling
    ok, lines = check_current_coupling(pack_dir)
    checks.append(("CURRENT.xpm coupling (2+ engine sources)", ok, lines))

    # 5. XPM params
    ok, lines = check_xpm_params(pack_dir)
    checks.append(("XPM params (KeyTrack / RootNote / VelStart)", ok, lines))

    # 6. Guide file
    ok, lines = check_guide_file(pack_dir)
    checks.append(("Guide / README file", ok, lines))

    # 7 & 8. Sample files + per-file size
    ok, lines = check_samples(pack_dir, strict)
    checks.append(("Sample files (count + size)", ok, lines))

    # 9. Total size
    ok, lines, total_mb = check_total_size(pack_dir)
    checks.append(("Total pack size", ok, lines))

    # ── Print results ──────────────────────────────────────────────────────────
    all_passed = True
    for check_name, passed, detail_lines in checks:
        status = f"{ANSI_GREEN}PASS{ANSI_RESET}" if passed else f"{ANSI_RED}FAIL{ANSI_RESET}"
        print(f"{ANSI_BOLD}[{status}{ANSI_BOLD}] {check_name}{ANSI_RESET}")
        for line in detail_lines:
            print(line)
        print()
        if not passed:
            all_passed = False

    # ── File inventory ─────────────────────────────────────────────────────────
    print(f"{ANSI_BOLD}File Inventory:{ANSI_RESET}")
    for line in build_file_inventory(pack_dir):
        print(line)
    print()
    print(f"  Total pack size: {total_mb:.2f}MB")
    print()

    # ── Summary ────────────────────────────────────────────────────────────────
    passed_count = sum(1 for _, ok, _ in checks if ok)
    total_count = len(checks)

    if all_passed:
        print(f"{ANSI_BOLD}{ANSI_GREEN}ALL {total_count} CHECKS PASSED — TIDE TABLES build is valid.{ANSI_RESET}")
        return 0
    else:
        failed_count = total_count - passed_count
        print(
            f"{ANSI_BOLD}{ANSI_RED}{failed_count} of {total_count} checks FAILED.{ANSI_RESET}"
            f"  ({passed_count} passed)"
        )
        if not strict:
            print(f"  Run with {ANSI_YELLOW}--strict{ANSI_RESET} to treat all warnings as errors.")
        return 1


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Validate a TIDE TABLES XPN pack directory against the spec.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    parser.add_argument(
        "--pack-dir",
        required=True,
        metavar="DIR",
        help="Path to the TIDE TABLES pack directory to validate",
    )
    parser.add_argument(
        "--strict",
        action="store_true",
        help="Treat warnings as errors (stricter validation)",
    )
    args = parser.parse_args()

    pack_dir = Path(args.pack_dir).expanduser().resolve()
    return run_validation(pack_dir, strict=args.strict)


if __name__ == "__main__":
    sys.exit(main())
