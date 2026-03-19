#!/usr/bin/env python3
"""
XOmnibus Documentation Validator — Cross-Source Consistency Checker

Validates documentation accuracy by cross-referencing multiple sources of truth:
  1. Engine list consistency across CLAUDE.md, validate_presets.py, PresetManager.h,
     and XOmnibusProcessor.cpp
  2. Engine count claims match actual engine lists
  3. Parameter prefix consistency between CLAUDE.md and PresetManager.h (frozen source)
  4. Cross-referenced doc files exist (master spec → subordinate docs)
  5. Key file references in CLAUDE.md point to files that exist

Usage:
    python3 validate_docs.py [--strict] [--verbose]

Options:
    --strict    Treat warnings as errors (exit 1 on any warning)
    --verbose   Show passing checks as well as failures

Exit codes:
    0  All checks passed
    1  One or more errors found (or warnings in --strict mode)
"""

import json
import os
import re
import sys
from pathlib import Path

# ---------------------------------------------------------------------------
# Paths (relative to repo root)
# ---------------------------------------------------------------------------

REPO_ROOT = Path(__file__).parent.parent.resolve()
CLAUDE_MD = REPO_ROOT / "CLAUDE.md"
PRESET_MANAGER_H = REPO_ROOT / "Source" / "Core" / "PresetManager.h"
PROCESSOR_CPP = REPO_ROOT / "Source" / "XOmnibusProcessor.cpp"
VALIDATE_PRESETS_PY = REPO_ROOT / "Tools" / "validate_presets.py"
MASTER_SPEC = REPO_ROOT / "Docs" / "xomnibus_master_specification.md"
SCHEMA_JSON = REPO_ROOT / "Docs" / "xometa_schema.json"
DOCS_DIR = REPO_ROOT / "Docs"

# ---------------------------------------------------------------------------
# Result tracking
# ---------------------------------------------------------------------------

class Results:
    def __init__(self):
        self.errors = []
        self.warnings = []
        self.passes = []

    def error(self, check: str, msg: str):
        self.errors.append(f"[{check}] {msg}")

    def warn(self, check: str, msg: str):
        self.warnings.append(f"[{check}] {msg}")

    def ok(self, check: str, msg: str):
        self.passes.append(f"[{check}] {msg}")


results = Results()

# ---------------------------------------------------------------------------
# Extraction helpers
# ---------------------------------------------------------------------------

def read_file(path: Path) -> str:
    """Read a file and return its contents, or empty string if missing."""
    if not path.exists():
        results.error("file_exists", f"Required file not found: {path.relative_to(REPO_ROOT)}")
        return ""
    return path.read_text(encoding="utf-8")


def extract_claude_engine_list(text: str) -> set:
    """Extract engine names from CLAUDE.md 'Engine modules (registered):' line."""
    m = re.search(r'\*\*Engine modules \(registered\):\*\*\s*(.+)', text)
    if not m:
        results.error("claude_engines", "Could not find 'Engine modules (registered):' in CLAUDE.md")
        return set()
    raw = m.group(1).strip()
    engines = {e.strip() for e in raw.split(",") if e.strip()}
    return engines


def extract_claude_engine_table(text: str) -> dict:
    """Extract engine short names from CLAUDE.md engine table (## Engine Modules)."""
    engines = {}
    in_table = False
    for line in text.splitlines():
        if "## Engine Modules" in line:
            in_table = True
            continue
        if in_table and line.startswith("###"):
            break
        if in_table and line.startswith("|") and "Short Name" not in line and "---" not in line:
            cols = [c.strip() for c in line.split("|")]
            if len(cols) >= 3:
                short_name = cols[1].strip()
                if short_name:
                    engines[short_name] = cols[2].strip() if len(cols) > 2 else ""
    return engines


def extract_claude_prefix_table(text: str) -> dict:
    """Extract Engine ID → Parameter Prefix mapping from CLAUDE.md."""
    prefixes = {}
    in_table = False
    for line in text.splitlines():
        if line.startswith("|") and "Engine ID" in line and "Parameter Prefix" in line:
            in_table = True
            continue
        if in_table and line.startswith("|") and "---" not in line:
            cols = [c.strip() for c in line.split("|")]
            if len(cols) >= 3:
                engine_id = cols[1].strip()
                prefix_raw = cols[2].strip()
                prefix = prefix_raw.strip("`").rstrip("_")
                if engine_id and prefix:
                    prefixes[engine_id] = prefix
        elif in_table and not line.startswith("|") and line.strip():
            break
    return prefixes


def extract_claude_engine_count(text: str) -> int:
    """Extract the claimed engine count from CLAUDE.md."""
    m = re.search(r'\*\*(\d+)\s+engines\*\*\s+are registered', text)
    if not m:
        results.warn("claude_count", "Could not find engine count claim in CLAUDE.md")
        return -1
    return int(m.group(1))


def extract_claude_key_files(text: str) -> list:
    """Extract file paths from the Key Files table in CLAUDE.md."""
    files = []
    in_table = False
    for line in text.splitlines():
        if "## Key Files" in line:
            in_table = True
            continue
        if in_table and line.startswith("##"):
            break
        if in_table and line.startswith("|") and "Path" not in line and "---" not in line:
            cols = [c.strip() for c in line.split("|")]
            if len(cols) >= 2:
                path = cols[1].strip().strip("`")
                if path:
                    files.append(path)
    return files


def extract_preset_manager_prefixes(text: str) -> dict:
    """Extract engine→prefix mapping from frozenPrefixForEngine() in PresetManager.h."""
    prefixes = {}
    in_func = False
    for line in text.splitlines():
        if "frozenPrefixForEngine" in line:
            in_func = True
            continue
        if in_func:
            if line.strip().startswith("};"):
                break
            m = re.search(r'\{\s*"([^"]+)"\s*,\s*"([^"]+)"\s*\}', line)
            if m:
                prefixes[m.group(1)] = m.group(2)
    return prefixes


def extract_processor_engine_ids(text: str) -> set:
    """Extract engine IDs from registerEngine() calls in XOmnibusProcessor.cpp."""
    ids = set()
    for m in re.finditer(r'registerEngine\(\s*"([^"]+)"', text):
        ids.add(m.group(1))
    return ids


def extract_validate_presets_engines(text: str) -> set:
    """Extract VALID_ENGINES set from validate_presets.py."""
    m = re.search(r'VALID_ENGINES\s*=\s*\{([^}]+)\}', text, re.DOTALL)
    if not m:
        results.error("validate_presets", "Could not find VALID_ENGINES in validate_presets.py")
        return set()
    raw = m.group(1)
    engines = set()
    for s in re.findall(r'"([^"]+)"', raw):
        engines.add(s)
    return engines


def extract_validate_presets_prefixes(text: str) -> dict:
    """Extract ENGINE_PARAM_PREFIXES from validate_presets.py."""
    m = re.search(r'ENGINE_PARAM_PREFIXES\s*=\s*\{([^}]+)\}', text, re.DOTALL)
    if not m:
        return {}
    raw = m.group(1)
    prefixes = {}
    for pair in re.finditer(r'"([^"]+)"\s*:\s*"([^"]+)"', raw):
        prefixes[pair.group(1)] = pair.group(2).rstrip("_")
    return prefixes


def extract_schema_engines(text: str) -> set:
    """Extract engine enum from xometa_schema.json."""
    try:
        schema = json.loads(text)
    except json.JSONDecodeError:
        results.error("schema", "xometa_schema.json is not valid JSON")
        return set()
    try:
        engines = set(schema["properties"]["engines"]["items"]["enum"])
    except (KeyError, TypeError):
        results.error("schema", "Could not find engines enum in xometa_schema.json")
        return set()
    return engines


def extract_master_spec_doc_refs(text: str) -> set:
    """Extract doc file references from the master spec's Document Hierarchy table."""
    refs = set()
    for m in re.finditer(r'`([A-Za-z0-9_\-]+\.(md|json))`', text):
        refs.add(m.group(1))
    return refs


# ---------------------------------------------------------------------------
# Checks
# ---------------------------------------------------------------------------

def check_engine_count_consistency():
    """Verify engine count claims match actual engine lists."""
    check = "engine_count"

    claude_text = read_file(CLAUDE_MD)
    if not claude_text:
        return

    claimed_count = extract_claude_engine_count(claude_text)
    engine_list = extract_claude_engine_list(claude_text)
    engine_table = extract_claude_engine_table(claude_text)

    if claimed_count > 0:
        if len(engine_list) != claimed_count:
            results.error(check,
                f"CLAUDE.md claims {claimed_count} engines but 'Engine modules (registered)' "
                f"lists {len(engine_list)}: diff={engine_list}")
        else:
            results.ok(check, f"CLAUDE.md engine count ({claimed_count}) matches module list ({len(engine_list)})")

    if engine_table:
        if len(engine_table) != len(engine_list):
            missing_from_table = {e.upper() for e in engine_list} - set(engine_table.keys())
            missing_from_list = set(engine_table.keys()) - {e.upper() for e in engine_list}
            msg = (f"CLAUDE.md engine table has {len(engine_table)} entries but "
                   f"module list has {len(engine_list)}")
            if missing_from_table:
                msg += f". Missing from table: {sorted(missing_from_table)}"
            if missing_from_list:
                msg += f". Missing from list: {sorted(missing_from_list)}"
            results.warn(check, msg)
        else:
            results.ok(check, f"CLAUDE.md engine table count ({len(engine_table)}) matches module list")


def check_engine_list_cross_sources():
    """Compare engine lists across CLAUDE.md, XOmnibusProcessor.cpp, and PresetManager.h."""
    check = "engine_list_cross"

    claude_text = read_file(CLAUDE_MD)
    processor_text = read_file(PROCESSOR_CPP)
    pm_text = read_file(PRESET_MANAGER_H)

    if not claude_text or not processor_text or not pm_text:
        return

    claude_engines = extract_claude_engine_list(claude_text)
    processor_ids = extract_processor_engine_ids(processor_text)
    pm_prefixes = extract_preset_manager_prefixes(pm_text)
    pm_engines = set(pm_prefixes.keys())

    # Normalize CLAUDE.md engine names to title case for comparison with code
    # CLAUDE.md lists them as UPPERCASE short names, code uses mixed case IDs
    claude_upper = {e.upper() for e in claude_engines}
    processor_upper = {e.upper() for e in processor_ids}
    pm_upper = {e.upper() for e in pm_engines}

    # Check processor engines vs CLAUDE.md
    only_in_processor = processor_upper - claude_upper
    only_in_claude = claude_upper - processor_upper
    if only_in_processor:
        results.error(check,
            f"Engines in XOmnibusProcessor.cpp but not in CLAUDE.md module list: "
            f"{sorted(only_in_processor)}")
    if only_in_claude:
        results.error(check,
            f"Engines in CLAUDE.md module list but not in XOmnibusProcessor.cpp: "
            f"{sorted(only_in_claude)}")
    if not only_in_processor and not only_in_claude:
        results.ok(check,
            f"CLAUDE.md and XOmnibusProcessor.cpp engine lists match "
            f"({len(processor_ids)} engines)")

    # Check PresetManager prefixes cover all processor engines
    only_in_pm = pm_upper - processor_upper
    missing_from_pm = processor_upper - pm_upper
    if missing_from_pm:
        results.warn(check,
            f"Engines registered in processor but missing from PresetManager.h "
            f"frozenPrefixForEngine(): {sorted(missing_from_pm)}")
    if only_in_pm:
        results.warn(check,
            f"Engines in PresetManager.h frozenPrefixForEngine() but not registered "
            f"in processor: {sorted(only_in_pm)}")
    if not only_in_pm and not missing_from_pm:
        results.ok(check,
            f"PresetManager.h prefix entries match processor registrations "
            f"({len(pm_engines)} engines)")


def check_prefix_consistency():
    """Compare parameter prefixes between CLAUDE.md and PresetManager.h."""
    check = "prefix_consistency"

    claude_text = read_file(CLAUDE_MD)
    pm_text = read_file(PRESET_MANAGER_H)

    if not claude_text or not pm_text:
        return

    claude_prefixes = extract_claude_prefix_table(claude_text)
    pm_prefixes = extract_preset_manager_prefixes(pm_text)

    if not claude_prefixes:
        results.error(check, "Could not extract prefix table from CLAUDE.md")
        return
    if not pm_prefixes:
        results.error(check, "Could not extract frozenPrefixForEngine() from PresetManager.h")
        return

    mismatches = []
    for engine_id, claude_prefix in claude_prefixes.items():
        if engine_id in pm_prefixes:
            pm_prefix = pm_prefixes[engine_id]
            if claude_prefix != pm_prefix:
                mismatches.append(
                    f"  {engine_id}: CLAUDE.md='{claude_prefix}' vs PresetManager.h='{pm_prefix}'"
                )

    if mismatches:
        results.error(check,
            f"Parameter prefix mismatches between CLAUDE.md and PresetManager.h:\n"
            + "\n".join(mismatches))
    else:
        common = set(claude_prefixes.keys()) & set(pm_prefixes.keys())
        results.ok(check,
            f"All {len(common)} shared engine prefixes match between "
            f"CLAUDE.md and PresetManager.h")

    # Check for engines in PresetManager.h but missing from CLAUDE.md table
    missing_from_claude = set(pm_prefixes.keys()) - set(claude_prefixes.keys())
    if missing_from_claude:
        results.warn(check,
            f"Engines in PresetManager.h but missing from CLAUDE.md prefix table: "
            f"{sorted(missing_from_claude)}")

    # Check for engines in CLAUDE.md table but missing from PresetManager.h
    missing_from_pm = set(claude_prefixes.keys()) - set(pm_prefixes.keys())
    if missing_from_pm:
        results.warn(check,
            f"Engines in CLAUDE.md prefix table but missing from PresetManager.h: "
            f"{sorted(missing_from_pm)}")


def check_validate_presets_consistency():
    """Check that validate_presets.py ENGINE_PARAM_PREFIXES matches PresetManager.h."""
    check = "validate_presets_prefixes"

    vp_text = read_file(VALIDATE_PRESETS_PY)
    pm_text = read_file(PRESET_MANAGER_H)

    if not vp_text or not pm_text:
        return

    vp_prefixes = extract_validate_presets_prefixes(vp_text)
    pm_prefixes = extract_preset_manager_prefixes(pm_text)

    if not vp_prefixes:
        results.warn(check, "Could not extract ENGINE_PARAM_PREFIXES from validate_presets.py")
        return

    mismatches = []
    for engine_id, vp_prefix in vp_prefixes.items():
        if engine_id in pm_prefixes:
            pm_prefix = pm_prefixes[engine_id]
            if vp_prefix != pm_prefix:
                mismatches.append(
                    f"  {engine_id}: validate_presets.py='{vp_prefix}' vs "
                    f"PresetManager.h='{pm_prefix}'"
                )

    if mismatches:
        results.error(check,
            f"Prefix mismatches between validate_presets.py and PresetManager.h:\n"
            + "\n".join(mismatches))
    else:
        common = set(vp_prefixes.keys()) & set(pm_prefixes.keys())
        results.ok(check,
            f"All {len(common)} shared prefixes match between "
            f"validate_presets.py and PresetManager.h")


def check_master_spec_references():
    """Check that doc files referenced in the master spec exist."""
    check = "master_spec_refs"

    text = read_file(MASTER_SPEC)
    if not text:
        return

    refs = extract_master_spec_doc_refs(text)
    missing = []
    found = 0
    for ref in sorted(refs):
        if ref == "CLAUDE.md":
            path = REPO_ROOT / ref
        else:
            path = DOCS_DIR / ref
        if not path.exists():
            missing.append(ref)
        else:
            found += 1

    if missing:
        results.warn(check,
            f"Docs referenced in master spec but not found: {missing}")
    if found > 0:
        results.ok(check, f"{found} doc references in master spec verified present")


def check_claude_key_files():
    """Check that files listed in CLAUDE.md Key Files table exist."""
    check = "key_files"

    text = read_file(CLAUDE_MD)
    if not text:
        return

    files = extract_claude_key_files(text)
    missing = []
    found = 0
    for f in files:
        # Skip glob/template paths like 'Source/Engines/*/' or 'Presets/XOmnibus/{mood}/'
        if "*" in f or "{" in f:
            parent = f.split("*")[0].split("{")[0].rstrip("/")
            path = REPO_ROOT / parent
            if path.exists():
                found += 1
            else:
                missing.append(f)
            continue
        path = REPO_ROOT / f
        if not path.exists():
            missing.append(f)
        else:
            found += 1

    if missing:
        results.warn(check,
            f"Key files listed in CLAUDE.md but not found: {missing}")
    if found > 0:
        results.ok(check, f"{found} key file references in CLAUDE.md verified present")


def check_schema_engine_coverage():
    """Check that xometa_schema.json engine list is up to date."""
    check = "schema_engines"

    schema_text = read_file(SCHEMA_JSON)
    processor_text = read_file(PROCESSOR_CPP)

    if not schema_text or not processor_text:
        return

    schema_engines = extract_schema_engines(schema_text)
    processor_ids = extract_processor_engine_ids(processor_text)

    if not schema_engines:
        return

    if len(schema_engines) < len(processor_ids):
        results.warn(check,
            f"xometa_schema.json engines enum has {len(schema_engines)} entries but "
            f"XOmnibusProcessor.cpp registers {len(processor_ids)} engines. "
            f"Schema may need updating.")
    else:
        results.ok(check,
            f"xometa_schema.json engine count ({len(schema_engines)}) covers "
            f"registered engines ({len(processor_ids)})")


def check_validate_presets_engine_coverage():
    """Check that validate_presets.py VALID_ENGINES covers all registered engines."""
    check = "vp_engine_coverage"

    vp_text = read_file(VALIDATE_PRESETS_PY)
    processor_text = read_file(PROCESSOR_CPP)

    if not vp_text or not processor_text:
        return

    vp_engines = extract_validate_presets_engines(vp_text)
    processor_ids = extract_processor_engine_ids(processor_text)

    missing = processor_ids - vp_engines
    if missing:
        results.warn(check,
            f"Engines registered in processor but missing from "
            f"validate_presets.py VALID_ENGINES: {sorted(missing)}")
    else:
        results.ok(check,
            f"validate_presets.py VALID_ENGINES covers all {len(processor_ids)} "
            f"registered engines")


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main():
    import argparse
    parser = argparse.ArgumentParser(
        description="XOmnibus Documentation Validator — cross-source consistency checks",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    parser.add_argument("--strict", action="store_true",
                        help="Treat warnings as errors")
    parser.add_argument("--verbose", action="store_true",
                        help="Show passing checks as well")
    args = parser.parse_args()

    print("=" * 70)
    print("XOmnibus Documentation Validator")
    print("=" * 70)
    print()

    # Run all checks
    check_engine_count_consistency()
    check_engine_list_cross_sources()
    check_prefix_consistency()
    check_validate_presets_consistency()
    check_master_spec_references()
    check_claude_key_files()
    check_schema_engine_coverage()
    check_validate_presets_engine_coverage()

    # Report results
    if args.verbose and results.passes:
        print("PASSED:")
        for p in results.passes:
            print(f"  ✓ {p}")
        print()

    if results.warnings:
        print("WARNINGS:")
        for w in results.warnings:
            print(f"  ⚠ {w}")
        print()

    if results.errors:
        print("ERRORS:")
        for e in results.errors:
            print(f"  ✗ {e}")
        print()

    # Summary
    total = len(results.passes) + len(results.warnings) + len(results.errors)
    print("=" * 70)
    print(f"RESULTS: {len(results.passes)} passed, "
          f"{len(results.warnings)} warnings, {len(results.errors)} errors "
          f"(total {total} checks)")
    print("=" * 70)

    if results.errors:
        return 1
    if args.strict and results.warnings:
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
