#!/usr/bin/env python3
"""
xpn_pack_release_checklist.py
Pre-release checklist runner for XPN packs.
Verifies everything is in order before packaging and publishing a pack.

Usage:
    python xpn_pack_release_checklist.py --pack-dir ./MyPack
    python xpn_pack_release_checklist.py --pack-dir ./MyPack --strict
    python xpn_pack_release_checklist.py --pack-dir ./MyPack --json
"""

import argparse
import json
import re
import sys
from pathlib import Path

# ---------------------------------------------------------------------------
# Constants
# ---------------------------------------------------------------------------

REQUIRED_EXPANSION_FIELDS = {"name", "version", "description", "type", "tier"}

SONIC_DNA_KEYS = {"energy", "warmth", "darkness", "motion", "density", "brightness"}

VALID_MOODS = {
    "Foundation",
    "Atmosphere",
    "Entangled",
    "Prism",
    "Flux",
    "Aether",
    "Family",
}

FORBIDDEN_NAME_CHARS = set('[]/\\:*?"<>|')

SEMVER_RE = re.compile(r"^\d+\.\d+\.\d+$")

DRUM_TYPE_KEYWORDS = {"drums", "kits", "drum", "kit", "percussion", "beats"}

# ---------------------------------------------------------------------------
# Result helpers
# ---------------------------------------------------------------------------

PASS = "PASS"
FAIL = "FAIL"
WARN = "WARN"
SKIP = "SKIP"

# Checks that cause exit(1) on failure
CRITICAL_CHECK_IDS = {1, 2, 3, 4, 5, 7, 12, 13}


class CheckResult:
    def __init__(self, check_id: int, label: str, status: str, detail: str = ""):
        self.check_id = check_id
        self.label = label
        self.status = status
        self.detail = detail

    def to_dict(self):
        d = {"id": self.check_id, "label": self.label, "status": self.status}
        if self.detail:
            d["detail"] = self.detail
        return d


# ---------------------------------------------------------------------------
# Utility
# ---------------------------------------------------------------------------


def find_files(root: Path, suffix: str):
    return list(root.rglob(f"*{suffix}"))


def load_json_file(path: Path):
    with path.open("r", encoding="utf-8") as f:
        return json.load(f)


# ---------------------------------------------------------------------------
# Individual checks
# ---------------------------------------------------------------------------


def check_01_expansion_json(pack_dir: Path) -> CheckResult:
    """expansion.json exists and has required fields."""
    label = "expansion.json exists with required fields"
    exp_path = pack_dir / "expansion.json"
    if not exp_path.exists():
        return CheckResult(1, label, FAIL, "expansion.json not found in pack root")
    try:
        data = load_json_file(exp_path)
    except Exception as e:
        return CheckResult(1, label, FAIL, f"JSON parse error: {e}")
    missing = REQUIRED_EXPANSION_FIELDS - set(data.keys())
    if missing:
        return CheckResult(1, label, FAIL, f"Missing fields: {sorted(missing)}")
    return CheckResult(1, label, PASS)


def check_02_sonic_dna(xometa_files: list) -> CheckResult:
    """All .xometa files have valid sonic_dna (6 keys, values 0.0-1.0)."""
    label = "All .xometa files have valid sonic_dna"
    if not xometa_files:
        return CheckResult(2, label, SKIP, "No .xometa files found")
    errors = []
    for path in xometa_files:
        try:
            data = load_json_file(path)
        except Exception as e:
            errors.append(f"{path.name}: JSON parse error: {e}")
            continue
        dna = data.get("sonic_dna")
        if dna is None:
            errors.append(f"{path.name}: missing sonic_dna")
            continue
        if not isinstance(dna, dict):
            errors.append(f"{path.name}: sonic_dna is not an object")
            continue
        missing_keys = SONIC_DNA_KEYS - set(dna.keys())
        if missing_keys:
            errors.append(f"{path.name}: sonic_dna missing keys {sorted(missing_keys)}")
            continue
        bad_vals = [
            k for k, v in dna.items()
            if k in SONIC_DNA_KEYS
            and not (isinstance(v, (int, float)) and 0.0 <= float(v) <= 1.0)
        ]
        if bad_vals:
            errors.append(
                f"{path.name}: sonic_dna values out of [0,1] for keys {bad_vals}"
            )
    if errors:
        detail = "; ".join(errors[:5])
        if len(errors) > 5:
            detail += f" ... (+{len(errors) - 5} more)"
        return CheckResult(2, label, FAIL, detail)
    return CheckResult(2, label, PASS, f"{len(xometa_files)} file(s) validated")


def check_03_preset_names(xometa_files: list) -> CheckResult:
    """All .xometa files have non-empty name (<=30 chars)."""
    label = "All .xometa preset names non-empty and <= 30 chars"
    if not xometa_files:
        return CheckResult(3, label, SKIP, "No .xometa files found")
    errors = []
    for path in xometa_files:
        try:
            data = load_json_file(path)
        except Exception:
            continue
        name = data.get("name", "")
        if not name or not str(name).strip():
            errors.append(f"{path.name}: empty name")
        elif len(str(name)) > 30:
            errors.append(
                f"{path.name}: name '{name}' exceeds 30 chars ({len(str(name))})"
            )
    if errors:
        detail = "; ".join(errors[:5])
        if len(errors) > 5:
            detail += f" ... (+{len(errors) - 5} more)"
        return CheckResult(3, label, FAIL, detail)
    return CheckResult(3, label, PASS)


def check_04_mood_field(xometa_files: list) -> CheckResult:
    """All .xometa files have mood field (one of 7 valid moods)."""
    label = "All .xometa files have valid mood field"
    if not xometa_files:
        return CheckResult(4, label, SKIP, "No .xometa files found")
    errors = []
    for path in xometa_files:
        try:
            data = load_json_file(path)
        except Exception:
            continue
        mood = data.get("mood")
        if mood is None:
            errors.append(f"{path.name}: missing mood")
        elif mood not in VALID_MOODS:
            errors.append(f"{path.name}: invalid mood '{mood}'")
    if errors:
        detail = "; ".join(errors[:5])
        if len(errors) > 5:
            detail += f" ... (+{len(errors) - 5} more)"
        return CheckResult(4, label, FAIL, detail)
    return CheckResult(4, label, PASS)


def check_05_no_duplicate_names(xometa_files: list) -> CheckResult:
    """No duplicate preset names within the pack."""
    label = "No duplicate preset names"
    if not xometa_files:
        return CheckResult(5, label, SKIP, "No .xometa files found")
    seen: dict = {}
    dupes = []
    for path in xometa_files:
        try:
            data = load_json_file(path)
        except Exception:
            continue
        name = str(data.get("name", "")).strip()
        if not name:
            continue
        if name in seen:
            dupes.append(f"'{name}' ({path.name} & {seen[name]})")
        else:
            seen[name] = path.name
    if dupes:
        detail = "Duplicates: " + "; ".join(dupes[:5])
        if len(dupes) > 5:
            detail += f" ... (+{len(dupes) - 5} more)"
        return CheckResult(5, label, FAIL, detail)
    return CheckResult(5, label, PASS, f"{len(seen)} unique preset names")


def check_06_mood_coverage(xometa_files: list) -> CheckResult:
    """At least 1 preset per mood represented (if >20 presets)."""
    label = "All 7 moods represented (for packs with >20 presets)"
    count = len(xometa_files)
    if count <= 20:
        return CheckResult(
            6, label, SKIP, f"Only {count} preset(s) — mood coverage check not required"
        )
    moods_present: set = set()
    for path in xometa_files:
        try:
            data = load_json_file(path)
        except Exception:
            continue
        mood = data.get("mood")
        if mood in VALID_MOODS:
            moods_present.add(mood)
    missing = VALID_MOODS - moods_present
    if missing:
        return CheckResult(
            6, label, WARN, f"Missing moods: {sorted(missing)}"
        )
    return CheckResult(6, label, PASS, f"All 7 moods covered across {count} presets")


def check_07_relative_sample_paths(xpm_files: list) -> CheckResult:
    """All sample file paths in .xpm files are relative (not absolute)."""
    label = "All .xpm sample paths are relative (not absolute)"
    if not xpm_files:
        return CheckResult(7, label, SKIP, "No .xpm files found")

    # Match XML element content or attribute values starting with absolute path indicators
    abs_path_re = re.compile(
        r"(?:<[^>]*>|=\s*\"|=\s*')(?:\s*)(/(?!/)|\b[A-Za-z]:\\)",
        re.IGNORECASE,
    )
    errors = []
    for path in xpm_files:
        try:
            content = path.read_text(encoding="utf-8", errors="replace")
        except Exception as e:
            errors.append(f"{path.name}: read error: {e}")
            continue
        if abs_path_re.search(content):
            errors.append(f"{path.name}: contains absolute sample path(s)")
    if errors:
        detail = "; ".join(errors[:5])
        if len(errors) > 5:
            detail += f" ... (+{len(errors) - 5} more)"
        return CheckResult(7, label, FAIL, detail)
    return CheckResult(7, label, PASS, f"{len(xpm_files)} .xpm file(s) checked")


def check_08_readme_exists(pack_dir: Path) -> CheckResult:
    """README.md or WORKFLOW_GUIDE.md exists in pack root."""
    label = "README.md or WORKFLOW_GUIDE.md present"
    candidates = [
        "README.md", "readme.md", "README.txt",
        "WORKFLOW_GUIDE.md", "workflow_guide.md",
    ]
    for name in candidates:
        if (pack_dir / name).exists():
            return CheckResult(8, label, PASS, f"Found {name}")
    return CheckResult(8, label, WARN, "No README.md or WORKFLOW_GUIDE.md found in pack root")


def check_09_cover_art(pack_dir: Path) -> CheckResult:
    """Cover art file exists (cover.png / artwork.png / any *.png)."""
    label = "Cover art file present"
    priority_names = ["cover.png", "artwork.png", "Cover.png", "Artwork.png"]
    for name in priority_names:
        if (pack_dir / name).exists():
            return CheckResult(9, label, PASS, f"Found {name}")
    pngs = list(pack_dir.glob("*.png"))
    if pngs:
        return CheckResult(9, label, PASS, f"Found {pngs[0].name}")
    return CheckResult(9, label, WARN, "No cover art (.png) found in pack root")


def check_10_xometa_size(xometa_files: list) -> CheckResult:
    """No .xometa file exceeds 50KB."""
    label = "No .xometa file exceeds 50KB"
    limit = 50 * 1024
    bloated = []
    for path in xometa_files:
        size = path.stat().st_size
        if size > limit:
            bloated.append(f"{path.name} ({size // 1024}KB)")
    if bloated:
        return CheckResult(
            10, label, WARN, "Oversized files: " + ", ".join(bloated[:5])
        )
    return CheckResult(10, label, PASS, f"All {len(xometa_files)} file(s) within 50KB limit")


def check_11_minimum_presets(xometa_files: list) -> CheckResult:
    """Pack has at least 4 presets."""
    label = "Pack has at least 4 presets"
    count = len(xometa_files)
    if count < 4:
        return CheckResult(
            11, label, WARN, f"Only {count} preset(s) found (minimum recommended: 4)"
        )
    return CheckResult(11, label, PASS, f"{count} preset(s) found")


def check_12_forbidden_chars(xometa_files: list) -> CheckResult:
    """No preset name contains forbidden chars ([]/\\:*?\"<>|)."""
    label = 'No preset names contain forbidden characters ([]/\\\\:*?"<>|)'
    if not xometa_files:
        return CheckResult(12, label, SKIP, "No .xometa files found")
    errors = []
    for path in xometa_files:
        try:
            data = load_json_file(path)
        except Exception:
            continue
        name = str(data.get("name", ""))
        bad = [c for c in name if c in FORBIDDEN_NAME_CHARS]
        if bad:
            unique_bad = list(dict.fromkeys(bad))
            errors.append(f"'{name}' contains {unique_bad}")
    if errors:
        detail = "; ".join(errors[:5])
        if len(errors) > 5:
            detail += f" ... (+{len(errors) - 5} more)"
        return CheckResult(12, label, FAIL, detail)
    return CheckResult(12, label, PASS)


def check_13_semver(pack_dir: Path) -> CheckResult:
    """expansion.json version follows semver (N.N.N)."""
    label = "expansion.json version follows semver (N.N.N)"
    exp_path = pack_dir / "expansion.json"
    if not exp_path.exists():
        return CheckResult(13, label, FAIL, "expansion.json not found — cannot check version")
    try:
        data = load_json_file(exp_path)
    except Exception as e:
        return CheckResult(13, label, FAIL, f"JSON parse error: {e}")
    version = str(data.get("version", "")).strip()
    if not version:
        return CheckResult(13, label, FAIL, "version field is empty or missing")
    if not SEMVER_RE.match(version):
        return CheckResult(
            13, label, FAIL, f"'{version}' does not match N.N.N semver format"
        )
    return CheckResult(13, label, PASS, f"version = {version}")


def check_14_drum_kit_xpm(pack_dir: Path, xpm_files: list) -> CheckResult:
    """At least one .xpm exists if pack claims to have drums/kits."""
    label = "Drum/kit pack includes .xpm program file(s)"
    exp_path = pack_dir / "expansion.json"
    is_drum_pack = False
    if exp_path.exists():
        try:
            data = load_json_file(exp_path)
            pack_type = str(data.get("type", "")).lower()
            pack_name = str(data.get("name", "")).lower()
            if any(kw in pack_type or kw in pack_name for kw in DRUM_TYPE_KEYWORDS):
                is_drum_pack = True
        except Exception:
            pass
    if not is_drum_pack:
        return CheckResult(
            14, label, SKIP, "Pack does not appear to be a drum/kit pack"
        )
    if not xpm_files:
        return CheckResult(
            14, label, WARN, "Pack claims drums/kits but no .xpm files found"
        )
    return CheckResult(14, label, PASS, f"{len(xpm_files)} .xpm file(s) found")


def check_15_no_junk_files(pack_dir: Path) -> CheckResult:
    """No .DS_Store or Thumbs.db files present."""
    label = "No .DS_Store or Thumbs.db files present"
    junk_names = {".DS_Store", "Thumbs.db", "desktop.ini", "ehthumbs.db"}
    found = []
    for item in pack_dir.rglob("*"):
        if item.name in junk_names:
            try:
                found.append(str(item.relative_to(pack_dir)))
            except ValueError:
                found.append(item.name)
    if found:
        return CheckResult(
            15, label, WARN, "Junk files found: " + ", ".join(found[:10])
        )
    return CheckResult(15, label, PASS)


# ---------------------------------------------------------------------------
# Runner
# ---------------------------------------------------------------------------


def run_checks(pack_dir: Path, strict: bool) -> list:
    xometa_files = find_files(pack_dir, ".xometa")
    xpm_files = find_files(pack_dir, ".xpm")

    results = [
        check_01_expansion_json(pack_dir),
        check_02_sonic_dna(xometa_files),
        check_03_preset_names(xometa_files),
        check_04_mood_field(xometa_files),
        check_05_no_duplicate_names(xometa_files),
        check_06_mood_coverage(xometa_files),
        check_07_relative_sample_paths(xpm_files),
        check_08_readme_exists(pack_dir),
        check_09_cover_art(pack_dir),
        check_10_xometa_size(xometa_files),
        check_11_minimum_presets(xometa_files),
        check_12_forbidden_chars(xometa_files),
        check_13_semver(pack_dir),
        check_14_drum_kit_xpm(pack_dir, xpm_files),
        check_15_no_junk_files(pack_dir),
    ]

    if strict:
        for r in results:
            if r.status == WARN:
                r.status = FAIL

    return results


# ---------------------------------------------------------------------------
# Output
# ---------------------------------------------------------------------------

STATUS_SYMBOLS = {PASS: "v", FAIL: "X", WARN: "!", SKIP: "-"}
STATUS_LABELS  = {PASS: " PASS", FAIL: " FAIL", WARN: " WARN", SKIP: " SKIP"}


def print_results(results: list, pack_dir: Path, strict: bool):
    print()
    print("=" * 68)
    print("  XPN Pack Release Checklist")
    print(f"  Pack : {pack_dir}")
    if strict:
        print("  Mode : STRICT (warnings treated as failures)")
    print("=" * 68)

    passed = 0
    failed_critical = []
    warnings_or_noncrit_fails = []

    for r in results:
        sym = STATUS_SYMBOLS[r.status]
        lbl = STATUS_LABELS[r.status]
        crit_tag = " [CRITICAL]" if r.check_id in CRITICAL_CHECK_IDS else "          "
        print(f"  {sym}{lbl}  [{r.check_id:02d}]{crit_tag} {r.label}")
        if r.detail:
            print(f"           {r.detail}")
        if r.status == PASS:
            passed += 1
        elif r.status == FAIL:
            if r.check_id in CRITICAL_CHECK_IDS:
                failed_critical.append(r)
            else:
                warnings_or_noncrit_fails.append(r)
        elif r.status == WARN:
            warnings_or_noncrit_fails.append(r)

    skipped = sum(1 for r in results if r.status == SKIP)
    total_applicable = len(results) - skipped
    release_ready = (
        len(failed_critical) == 0
        and (not strict or len(warnings_or_noncrit_fails) == 0)
    )

    print()
    print("-" * 68)
    print(f"  Results : {passed}/{total_applicable} checks passed  ({skipped} skipped)")
    if failed_critical:
        print(f"  Critical failures : {len(failed_critical)}")
    if warnings_or_noncrit_fails:
        label = "Non-critical failures" if strict else "Warnings"
        print(f"  {label} : {len(warnings_or_noncrit_fails)}")
    print(f"  Release-ready     : {'YES' if release_ready else 'NO'}")
    print("=" * 68)
    print()


def print_json_results(results: list, pack_dir: Path, strict: bool):
    passed = sum(1 for r in results if r.status == PASS)
    skipped = sum(1 for r in results if r.status == SKIP)
    failed_critical = [
        r for r in results
        if r.status == FAIL and r.check_id in CRITICAL_CHECK_IDS
    ]
    warnings_or_noncrit_fails = [
        r for r in results
        if (r.status == WARN)
        or (r.status == FAIL and r.check_id not in CRITICAL_CHECK_IDS)
    ]
    release_ready = (
        len(failed_critical) == 0
        and (not strict or len(warnings_or_noncrit_fails) == 0)
    )

    output = {
        "pack_dir": str(pack_dir),
        "strict": strict,
        "summary": {
            "total": len(results),
            "passed": passed,
            "skipped": skipped,
            "failed_critical": len(failed_critical),
            "warnings_or_noncrit_fails": len(warnings_or_noncrit_fails),
            "release_ready": release_ready,
        },
        "checks": [r.to_dict() for r in results],
    }
    print(json.dumps(output, indent=2))


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Pre-release checklist for XPN packs.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  python xpn_pack_release_checklist.py --pack-dir ./MyPack
  python xpn_pack_release_checklist.py --pack-dir ./MyPack --strict
  python xpn_pack_release_checklist.py --pack-dir ./MyPack --json

CRITICAL checks (exit 1 on failure): 1, 2, 3, 4, 5, 7, 12, 13
WARNING checks  (exit 0 by default): 6, 8, 9, 10, 11, 14, 15
        """,
    )
    parser.add_argument(
        "--pack-dir",
        required=True,
        metavar="DIR",
        help="Path to the root directory of the XPN pack",
    )
    parser.add_argument(
        "--strict",
        action="store_true",
        help="Treat WARN results as failures (exit 1 if any warnings)",
    )
    parser.add_argument(
        "--json",
        action="store_true",
        dest="output_json",
        help="Output results as JSON instead of formatted text",
    )
    return parser


def main() -> None:
    parser = build_parser()
    args = parser.parse_args()

    pack_dir = Path(args.pack_dir).resolve()
    if not pack_dir.is_dir():
        print(
            f"ERROR: --pack-dir '{pack_dir}' is not a directory or does not exist.",
            file=sys.stderr,
        )
        sys.exit(1)

    results = run_checks(pack_dir, strict=args.strict)

    if args.output_json:
        print_json_results(results, pack_dir, strict=args.strict)
    else:
        print_results(results, pack_dir, strict=args.strict)

    # Exit 1 if any critical check failed
    for r in results:
        if r.status == FAIL and r.check_id in CRITICAL_CHECK_IDS:
            sys.exit(1)

    # In strict mode, any remaining FAIL (promoted from WARN) also exits 1
    if args.strict:
        for r in results:
            if r.status == FAIL:
                sys.exit(1)

    sys.exit(0)


if __name__ == "__main__":
    main()
