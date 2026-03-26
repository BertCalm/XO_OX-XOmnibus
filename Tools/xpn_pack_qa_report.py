#!/usr/bin/env python3
"""
xpn_pack_qa_report.py — XO_OX Comprehensive Pack QA Report Generator

Top-of-stack orchestrator: runs all key validation checks against a pack
directory or .xpn ZIP and aggregates findings into a single master report.

Checks performed:
    1.  expansion.json exists and is valid JSON (required fields)
    2.  All XPM files parse as valid XML
    3.  Sample references: all samples cited in XPMs exist on disk
    4.  No duplicate program names across XPMs
    5.  Naming conventions: no generic names (Untitled, Default, New Program)
    6.  XPM rules: KeyTrack=True, RootNote=0, VelStart=0 on at least one
    7.  Cover art present (.png or .jpg in pack root)
    8.  Sample count: at least 1 WAV/AIF/FLAC found
    9.  ZIP structure: expansion.json at root level (when .xpn zip supplied)
    10. Preset count: .xometa files present

Scoring: start 100, FAIL = -15 pts each, WARN = -5 pts each.
Overall status: PASS (no FAILs), WARN (no FAILs but WARNs), FAIL (any FAIL).

Exit codes: 0 = PASS, 1 = WARN, 2 = FAIL  (CI-friendly)

Usage:
    python xpn_pack_qa_report.py <pack_dir_or_zip> [--format text|json] \
        [--output report.txt]
"""

import argparse
import json
import sys
import tempfile
import zipfile
import xml.etree.ElementTree as ET
from dataclasses import dataclass, field, asdict
from pathlib import Path
from typing import List, Optional, Tuple

# ---------------------------------------------------------------------------
# Data model
# ---------------------------------------------------------------------------

STATUS_PASS = "PASS"
STATUS_WARN = "WARN"
STATUS_FAIL = "FAIL"

SCORE_FAIL_PENALTY = 15
SCORE_WARN_PENALTY = 5

GENERIC_NAMES = {
    "untitled", "default", "new program", "new kit", "program", "kit",
    "sample", "unnamed", "test", "temp", "tmp",
}

REQUIRED_EXPANSION_FIELDS = ("name", "version", "description")

SAMPLE_EXTENSIONS = {".wav", ".aif", ".aiff", ".flac", ".mp3", ".ogg"}
IMAGE_EXTENSIONS  = {".png", ".jpg", ".jpeg"}


@dataclass
class CheckResult:
    check_id: int
    name: str
    status: str          # PASS / WARN / FAIL
    message: str
    file: Optional[str] = None
    detail: Optional[List[str]] = field(default=None)

    def to_dict(self) -> dict:
        d = asdict(self)
        d = {k: v for k, v in d.items() if v is not None}
        return d


@dataclass
class QAReport:
    pack_path: str
    checks: List[CheckResult] = field(default_factory=list)

    @property
    def overall_status(self) -> str:
        if any(c.status == STATUS_FAIL for c in self.checks):
            return STATUS_FAIL
        if any(c.status == STATUS_WARN for c in self.checks):
            return STATUS_WARN
        return STATUS_PASS

    @property
    def score(self) -> int:
        s = 100
        for c in self.checks:
            if c.status == STATUS_FAIL:
                s -= SCORE_FAIL_PENALTY
            elif c.status == STATUS_WARN:
                s -= SCORE_WARN_PENALTY
        return max(0, s)

    @property
    def exit_code(self) -> int:
        st = self.overall_status
        if st == STATUS_FAIL:
            return 2
        if st == STATUS_WARN:
            return 1
        return 0

    def add(self, result: CheckResult) -> None:
        self.checks.append(result)

    def to_dict(self) -> dict:
        return {
            "pack_path": self.pack_path,
            "overall_status": self.overall_status,
            "score": self.score,
            "checks": [c.to_dict() for c in self.checks],
        }


# ---------------------------------------------------------------------------
# Pack loader — handles both directory and .xpn ZIP
# ---------------------------------------------------------------------------

class PackContext:
    """Resolved pack: always a local directory path after extraction."""

    def __init__(self, source: str) -> None:
        self._tmpdir: Optional[tempfile.TemporaryDirectory] = None
        self.source_path = Path(source)
        self.is_zip = self.source_path.suffix.lower() == ".xpn" and \
                      self.source_path.is_file()
        self.zip_root_has_expansion_json: Optional[bool] = None  # check 9

        if self.is_zip:
            self._tmpdir = tempfile.TemporaryDirectory(prefix="xpn_qa_")
            self._pack_dir = Path(self._tmpdir.name)
            self._extract_zip()
        else:
            self._pack_dir = self.source_path

    def _extract_zip(self) -> None:
        with zipfile.ZipFile(self.source_path, "r") as zf:
            names = zf.namelist()
            # Check 9: expansion.json at ZIP root
            self.zip_root_has_expansion_json = any(
                n == "expansion.json" or n.endswith("/expansion.json") and n.count("/") == 1
                for n in names
            )
            # More precise: root-level means no directory separator
            self.zip_root_has_expansion_json = "expansion.json" in names
            extract_root = self._pack_dir.resolve()
            for member in zf.infolist():
                dest = (extract_root / member.filename).resolve()
                if not str(dest).startswith(str(extract_root)):
                    raise ValueError(f"Zip slip detected: {member.filename}")
                zf.extract(member, extract_root)

    @property
    def pack_dir(self) -> Path:
        return self._pack_dir

    def cleanup(self) -> None:
        if self._tmpdir:
            self._tmpdir.cleanup()

    # -- convenience iterators --

    def all_files(self) -> List[Path]:
        return list(self._pack_dir.rglob("*"))

    def xpm_files(self) -> List[Path]:
        return list(self._pack_dir.rglob("*.xpm"))

    def sample_files(self) -> List[Path]:
        return [p for p in self._pack_dir.rglob("*")
                if p.suffix.lower() in SAMPLE_EXTENSIONS]

    def image_files_root(self) -> List[Path]:
        return [p for p in self._pack_dir.iterdir()
                if p.is_file() and p.suffix.lower() in IMAGE_EXTENSIONS]

    def xometa_files(self) -> List[Path]:
        return list(self._pack_dir.rglob("*.xometa"))

    def expansion_json_path(self) -> Optional[Path]:
        candidates = list(self._pack_dir.rglob("expansion.json"))
        return candidates[0] if candidates else None


# ---------------------------------------------------------------------------
# Individual checks
# ---------------------------------------------------------------------------

def check_expansion_json(ctx: PackContext) -> CheckResult:
    """Check 1: expansion.json exists and contains required fields."""
    p = ctx.expansion_json_path()
    if p is None:
        return CheckResult(
            check_id=1,
            name="expansion.json present & valid",
            status=STATUS_FAIL,
            message="expansion.json not found in pack.",
        )
    try:
        data = json.loads(p.read_text(encoding="utf-8"))
    except json.JSONDecodeError as exc:
        return CheckResult(
            check_id=1,
            name="expansion.json present & valid",
            status=STATUS_FAIL,
            message=f"expansion.json is not valid JSON: {exc}",
            file=str(p.relative_to(ctx.pack_dir)),
        )
    missing = [f for f in REQUIRED_EXPANSION_FIELDS if f not in data]
    if missing:
        return CheckResult(
            check_id=1,
            name="expansion.json present & valid",
            status=STATUS_FAIL,
            message=f"expansion.json missing required fields: {missing}",
            file=str(p.relative_to(ctx.pack_dir)),
        )
    return CheckResult(
        check_id=1,
        name="expansion.json present & valid",
        status=STATUS_PASS,
        message=f"expansion.json OK — name={data.get('name')!r}, version={data.get('version')!r}",
        file=str(p.relative_to(ctx.pack_dir)),
    )


def check_xpm_xml(ctx: PackContext) -> Tuple[CheckResult, List[ET.ElementTree]]:
    """Check 2: all XPM files parse as valid XML. Returns (result, parsed_trees)."""
    xpms = ctx.xpm_files()
    if not xpms:
        return CheckResult(
            check_id=2,
            name="XPM files parse as valid XML",
            status=STATUS_WARN,
            message="No .xpm files found in pack.",
        ), []

    errors: List[str] = []
    trees: List[Tuple[Path, ET.ElementTree]] = []
    for xpm in xpms:
        try:
            tree = ET.parse(str(xpm))
            trees.append((xpm, tree))
        except ET.ParseError as exc:
            errors.append(f"{xpm.name}: {exc}")

    if errors:
        return CheckResult(
            check_id=2,
            name="XPM files parse as valid XML",
            status=STATUS_FAIL,
            message=f"{len(errors)} XPM(s) failed XML parse.",
            detail=errors,
        ), [t for _, t in trees]

    return CheckResult(
        check_id=2,
        name="XPM files parse as valid XML",
        status=STATUS_PASS,
        message=f"All {len(xpms)} XPM file(s) parse successfully.",
    ), [t for _, t in trees]


def check_sample_references(
    ctx: PackContext,
    xpm_trees: List[ET.ElementTree],
) -> CheckResult:
    """Check 3: all samples referenced in XPMs exist on disk."""
    sample_set = {p.name.lower() for p in ctx.sample_files()}
    sample_path_set = {p for p in ctx.sample_files()}
    # Build absolute lookup by name for fast check
    by_name = {}
    for p in sample_path_set:
        by_name[p.name.lower()] = p

    missing: List[str] = []
    for tree in xpm_trees:
        root = tree.getroot()
        # MPC XPM: <SampleFile> elements contain file paths
        for elem in root.iter("SampleFile"):
            raw = (elem.text or "").strip()
            if not raw:
                continue
            fname = Path(raw).name
            if fname.lower() not in by_name:
                # Try absolute-style path within pack
                abs_candidate = ctx.pack_dir / raw.lstrip("/\\")
                if not abs_candidate.exists():
                    missing.append(fname)

    if missing:
        unique_missing = sorted(set(missing))
        return CheckResult(
            check_id=3,
            name="Sample references exist on disk",
            status=STATUS_FAIL,
            message=f"{len(unique_missing)} referenced sample(s) not found.",
            detail=unique_missing[:20],  # cap detail list
        )
    return CheckResult(
        check_id=3,
        name="Sample references exist on disk",
        status=STATUS_PASS,
        message="All sample references resolved successfully.",
    )


def check_duplicate_program_names(
    ctx: PackContext,
    xpm_trees: List[ET.ElementTree],
) -> CheckResult:
    """Check 4: no duplicate program names."""
    names: List[str] = []
    for tree in xpm_trees:
        root = tree.getroot()
        # Try <Program name="..."> or <Name> child
        name_attr = root.get("name") or root.get("Name")
        if name_attr:
            names.append(name_attr.strip())
            continue
        name_elem = root.find(".//Name")
        if name_elem is not None and name_elem.text:
            names.append(name_elem.text.strip())

    seen: dict = {}
    for n in names:
        key = n.lower()
        seen[key] = seen.get(key, 0) + 1

    dupes = [n for n, count in seen.items() if count > 1]
    if dupes:
        return CheckResult(
            check_id=4,
            name="No duplicate program names",
            status=STATUS_FAIL,
            message=f"{len(dupes)} duplicate program name(s) found.",
            detail=dupes,
        )
    return CheckResult(
        check_id=4,
        name="No duplicate program names",
        status=STATUS_PASS,
        message=f"All {len(names)} program name(s) are unique.",
    )


def check_naming_conventions(
    ctx: PackContext,
    xpm_trees: List[ET.ElementTree],
) -> CheckResult:
    """Check 5: no generic/placeholder names."""
    offenders: List[str] = []
    for tree in xpm_trees:
        root = tree.getroot()
        candidates = []
        name_attr = root.get("name") or root.get("Name")
        if name_attr:
            candidates.append(name_attr.strip())
        name_elem = root.find(".//Name")
        if name_elem is not None and name_elem.text:
            candidates.append(name_elem.text.strip())
        for c in candidates:
            if c.lower() in GENERIC_NAMES:
                offenders.append(c)

    if offenders:
        return CheckResult(
            check_id=5,
            name="No generic program names",
            status=STATUS_FAIL,
            message=f"{len(offenders)} generic/placeholder name(s) found.",
            detail=sorted(set(offenders)),
        )
    return CheckResult(
        check_id=5,
        name="No generic program names",
        status=STATUS_PASS,
        message="No generic names detected.",
    )


def check_xpm_rules(
    ctx: PackContext,
    xpm_trees: List[ET.ElementTree],
) -> CheckResult:
    """Check 6: KeyTrack=True, RootNote=0, VelStart=0 on at least one program."""
    if not xpm_trees:
        return CheckResult(
            check_id=6,
            name="XPM rules (KeyTrack/RootNote/VelStart)",
            status=STATUS_WARN,
            message="No XPM trees to validate.",
        )

    issues: List[str] = []
    keytrack_ok = False
    rootnote_ok = False
    velstart_ok = False

    for tree in xpm_trees:
        root = tree.getroot()
        # KeyTrack
        for elem in root.iter("KeyTrack"):
            if (elem.text or "").strip().lower() == "true":
                keytrack_ok = True
        # RootNote
        for elem in root.iter("RootNote"):
            if (elem.text or "").strip() == "0":
                rootnote_ok = True
        # VelStart (layer velocity start)
        for elem in root.iter("VelStart"):
            if (elem.text or "").strip() == "0":
                velstart_ok = True

    if not keytrack_ok:
        issues.append("No program has KeyTrack=True")
    if not rootnote_ok:
        issues.append("No program has RootNote=0")
    if not velstart_ok:
        issues.append("No layer has VelStart=0")

    if issues:
        return CheckResult(
            check_id=6,
            name="XPM rules (KeyTrack/RootNote/VelStart)",
            status=STATUS_WARN,
            message=f"{len(issues)} XPM rule(s) not satisfied.",
            detail=issues,
        )
    return CheckResult(
        check_id=6,
        name="XPM rules (KeyTrack/RootNote/VelStart)",
        status=STATUS_PASS,
        message="KeyTrack=True, RootNote=0, VelStart=0 all confirmed.",
    )


def check_cover_art(ctx: PackContext) -> CheckResult:
    """Check 7: pack root contains at least one image file."""
    images = ctx.image_files_root()
    if not images:
        return CheckResult(
            check_id=7,
            name="Cover art present",
            status=STATUS_WARN,
            message="No .png/.jpg found in pack root directory.",
        )
    return CheckResult(
        check_id=7,
        name="Cover art present",
        status=STATUS_PASS,
        message=f"Cover art found: {images[0].name}",
        file=images[0].name,
    )


def check_sample_count(ctx: PackContext) -> CheckResult:
    """Check 8: at least 1 sample file found."""
    samples = ctx.sample_files()
    if not samples:
        return CheckResult(
            check_id=8,
            name="Sample count >= 1",
            status=STATUS_FAIL,
            message="No audio sample files found in pack.",
        )
    return CheckResult(
        check_id=8,
        name="Sample count >= 1",
        status=STATUS_PASS,
        message=f"{len(samples)} sample file(s) found.",
    )


def check_zip_structure(ctx: PackContext) -> CheckResult:
    """Check 9: if input was a ZIP, expansion.json must be at root level."""
    if not ctx.is_zip:
        return CheckResult(
            check_id=9,
            name="ZIP structure (expansion.json at root)",
            status=STATUS_PASS,
            message="Input is a directory — ZIP structure check skipped.",
        )
    if ctx.zip_root_has_expansion_json:
        return CheckResult(
            check_id=9,
            name="ZIP structure (expansion.json at root)",
            status=STATUS_PASS,
            message="expansion.json found at ZIP root level.",
        )
    return CheckResult(
        check_id=9,
        name="ZIP structure (expansion.json at root)",
        status=STATUS_FAIL,
        message="expansion.json not at ZIP root level (MPC requires root placement).",
    )


def check_preset_count(ctx: PackContext) -> CheckResult:
    """Check 10: count .xometa preset files."""
    presets = ctx.xometa_files()
    if not presets:
        return CheckResult(
            check_id=10,
            name="Preset count (.xometa)",
            status=STATUS_WARN,
            message="No .xometa preset files found (optional but recommended).",
        )
    return CheckResult(
        check_id=10,
        name="Preset count (.xometa)",
        status=STATUS_PASS,
        message=f"{len(presets)} .xometa preset file(s) found.",
    )


# ---------------------------------------------------------------------------
# Report runners
# ---------------------------------------------------------------------------

def run_all_checks(ctx: PackContext) -> QAReport:
    report = QAReport(pack_path=str(ctx.source_path))

    report.add(check_expansion_json(ctx))

    xpm_result, xpm_trees = check_xpm_xml(ctx)
    report.add(xpm_result)

    report.add(check_sample_references(ctx, xpm_trees))
    report.add(check_duplicate_program_names(ctx, xpm_trees))
    report.add(check_naming_conventions(ctx, xpm_trees))
    report.add(check_xpm_rules(ctx, xpm_trees))
    report.add(check_cover_art(ctx))
    report.add(check_sample_count(ctx))
    report.add(check_zip_structure(ctx))
    report.add(check_preset_count(ctx))

    return report


# ---------------------------------------------------------------------------
# Formatters
# ---------------------------------------------------------------------------

STATUS_ICON = {STATUS_PASS: "✓", STATUS_WARN: "⚠", STATUS_FAIL: "✗"}
STATUS_LABEL = {STATUS_PASS: "PASS", STATUS_WARN: "WARN", STATUS_FAIL: "FAIL"}


def format_text(report: QAReport) -> str:
    lines = []
    sep = "=" * 70
    lines.append(sep)
    lines.append("  XO_OX XPN Pack QA Report")
    lines.append(f"  Pack : {report.pack_path}")
    lines.append(f"  Score: {report.score}/100   Status: {report.overall_status}")
    lines.append(sep)

    for c in report.checks:
        icon = STATUS_ICON.get(c.status, "?")
        label = STATUS_LABEL.get(c.status, c.status)
        file_str = f"  [{c.file}]" if c.file else ""
        lines.append(f"  {icon} [{label:4s}] Check {c.check_id:02d}: {c.name}")
        lines.append(f"           {c.message}{file_str}")
        if c.detail:
            for d in c.detail[:10]:
                lines.append(f"             • {d}")
            if len(c.detail) > 10:
                lines.append(f"             … and {len(c.detail) - 10} more")
        lines.append("")

    lines.append(sep)
    fail_count = sum(1 for c in report.checks if c.status == STATUS_FAIL)
    warn_count = sum(1 for c in report.checks if c.status == STATUS_WARN)
    pass_count = sum(1 for c in report.checks if c.status == STATUS_PASS)
    lines.append(
        f"  Summary: {pass_count} PASS  {warn_count} WARN  {fail_count} FAIL"
        f"   Overall: {report.overall_status}   Score: {report.score}/100"
    )
    lines.append(sep)
    return "\n".join(lines)


def format_json(report: QAReport) -> str:
    return json.dumps(report.to_dict(), indent=2)


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def main() -> int:
    parser = argparse.ArgumentParser(
        description="XO_OX XPN Pack Comprehensive QA Report Generator",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=(
            "Exit codes:\n"
            "  0 = PASS\n"
            "  1 = WARN  (no failures, but warnings present)\n"
            "  2 = FAIL  (one or more checks failed)\n"
        ),
    )
    parser.add_argument(
        "pack",
        metavar="PACK",
        help="Path to pack directory or .xpn ZIP file",
    )
    parser.add_argument(
        "--format",
        choices=["text", "json"],
        default="text",
        help="Output format (default: text)",
    )
    parser.add_argument(
        "--output",
        metavar="FILE",
        default=None,
        help="Write report to FILE instead of stdout",
    )
    args = parser.parse_args()

    pack_path = Path(args.pack)
    if not pack_path.exists():
        print(f"ERROR: pack path does not exist: {pack_path}", file=sys.stderr)
        return 2

    ctx = PackContext(str(pack_path))
    try:
        report = run_all_checks(ctx)
    finally:
        ctx.cleanup()

    output = format_json(report) if args.format == "json" else format_text(report)

    if args.output:
        Path(args.output).write_text(output, encoding="utf-8")
        print(f"Report written to: {args.output}", file=sys.stderr)
    else:
        print(output)

    return report.exit_code


if __name__ == "__main__":
    sys.exit(main())
