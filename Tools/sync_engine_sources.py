#!/usr/bin/env python3
"""
sync_engine_sources.py — Keep all engine-count / engine-roster references in sync.

Single source of truth: Docs/engines.json (hand-edited)
This script regenerates / updates:
  - Tools/engine_registry.py (Python mirror, auto-generated)
  - Sentinel counts in docs/markdown/html: <!-- ENGINE_COUNT -->N<!-- /ENGINE_COUNT -->

Modes:
  default            Regenerate downstream files from Docs/engines.json
  --bootstrap        One-time: rebuild Docs/engines.json from
                     Source/Core/PresetManager.h + Source/Engines/*/ dirs,
                     preserving metadata from the existing engines.json
  --check            Exit non-zero if anything is out of sync (dry-run)

Workflow after adding a new engine:
  1. Add its adapter under Source/Engines/NewEngine/NewEngineEngine.h
  2. Add it to PresetManager.h's frozenPrefixForEngine map
  3. Add a stub entry to Docs/engines.json
  4. Run: python Tools/sync_engine_sources.py
  5. Commit the updated engines.json, engine_registry.py, and any sentinel updates.
"""

from __future__ import annotations

import argparse
import json
import re
import sys
from datetime import date
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
ENGINES_JSON = REPO_ROOT / "Docs" / "engines.json"
PRESETMGR = REPO_ROOT / "Source" / "Core" / "PresetManager.h"
ENGINE_REGISTRY_PY = REPO_ROOT / "Tools" / "engine_registry.py"
ENGINES_DIR = REPO_ROOT / "Source" / "Engines"

# Legacy-named dirs in Source/Engines/ that map to canonical mixed-case engine IDs.
# (These dirs are kept for build compatibility; resolveEngineAlias() handles rename at load time.)
LEGACY_DIRS: dict[str, str] = {
    "Bite":  "Overbite",
    "Bob":   "Oblong",
    "Drift": "Odyssey",
    "Dub":   "Overdub",
    "Fat":   "Obese",
    "Morph": "OddOscar",
    "Snap":  "OddfeliX",
}

SENTINEL_PATTERN = re.compile(
    r"<!--\s*ENGINE_COUNT\s*-->[^<]*<!--\s*/ENGINE_COUNT\s*-->"
)
SENTINEL_DESIGNED_PATTERN = re.compile(
    r"<!--\s*ENGINE_COUNT_DESIGNED\s*-->[^<]*<!--\s*/ENGINE_COUNT_DESIGNED\s*-->"
)


# ---------------------------------------------------------------------------
# PresetManager.h parsing
# ---------------------------------------------------------------------------
def parse_frozen_prefix_map() -> dict[str, str]:
    """Extract {engine_id: prefix} from PresetManager.h's frozenPrefixForEngine map."""
    text = PRESETMGR.read_text()
    match = re.search(
        r"frozenPrefixForEngine.*?std::map<juce::String,\s*juce::String>\s*prefixes\{(.*?)\};",
        text, re.DOTALL,
    )
    if not match:
        raise RuntimeError("Could not locate frozenPrefixForEngine in PresetManager.h")
    entries = re.findall(r'\{"([^"]+)",\s*"([^"]+)"\}', match.group(1))
    return dict(entries)


def parse_valid_engine_names() -> list[str]:
    """Extract canonical engine IDs from validEngineNames (excluding X* and legacy aliases)."""
    text = PRESETMGR.read_text()
    match = re.search(
        r"inline const juce::StringArray validEngineNames\{(.*?)\};",
        text, re.DOTALL,
    )
    if not match:
        raise RuntimeError("Could not locate validEngineNames in PresetManager.h")
    names = re.findall(r'"([^"]+)"', match.group(1))
    legacy_short = set(LEGACY_DIRS.keys())
    return [
        n for n in names
        if not n.startswith("X") and n not in legacy_short
    ]


# ---------------------------------------------------------------------------
# Source/Engines/ directory inventory
# ---------------------------------------------------------------------------
def list_canonical_engine_dirs() -> set[str]:
    """Return the set of canonical engine IDs implied by Source/Engines/*/ directories.
    Legacy-named dirs are mapped to their canonical ID via LEGACY_DIRS."""
    result: set[str] = set()
    for path in ENGINES_DIR.iterdir():
        if not path.is_dir():
            continue
        name = path.name
        canonical = LEGACY_DIRS.get(name, name)
        result.add(canonical)
    return result


# ---------------------------------------------------------------------------
# engines.json I/O
# ---------------------------------------------------------------------------
def load_engines_json() -> dict:
    return json.loads(ENGINES_JSON.read_text())


def save_engines_json(data: dict) -> None:
    ENGINES_JSON.write_text(json.dumps(data, indent=2) + "\n")


def canonical_id_lookup(data: dict) -> dict[str, dict]:
    """Build case-insensitive lookup of existing engines.json entries."""
    out: dict[str, dict] = {}
    for entry in data.get("synthesis_engines", []):
        out[entry["id"].lower()] = entry
    return out


# ---------------------------------------------------------------------------
# Bootstrap: rebuild engines.json from PresetManager.h
# ---------------------------------------------------------------------------
def bootstrap() -> None:
    print("[bootstrap] Reading PresetManager.h frozenPrefixForEngine …")
    prefix_map = parse_frozen_prefix_map()
    print(f"[bootstrap]   {len(prefix_map)} engines found with frozen prefixes.")

    print("[bootstrap] Scanning Source/Engines/*/ …")
    dirs = list_canonical_engine_dirs()
    print(f"[bootstrap]   {len(dirs)} canonical engine directories (after alias mapping).")

    print("[bootstrap] Cross-referencing …")
    prefix_ids = set(prefix_map.keys())
    dirs_no_prefix = dirs - prefix_ids
    prefix_no_dir = prefix_ids - dirs
    if dirs_no_prefix:
        print(f"[bootstrap]   WARNING: dirs without frozen prefix: {sorted(dirs_no_prefix)}")
    if prefix_no_dir:
        print(f"[bootstrap]   WARNING: prefixes without dir: {sorted(prefix_no_dir)}")

    print("[bootstrap] Loading existing engines.json for metadata preservation …")
    try:
        existing_data = load_engines_json()
    except FileNotFoundError:
        existing_data = {}
    existing_lookup = canonical_id_lookup(existing_data)

    # Union: everything in prefix_map OR dirs should become an entry.
    all_ids = sorted(prefix_ids | dirs)

    new_engines: list[dict] = []
    for eng_id in all_ids:
        # Preserve existing metadata if present (match case-insensitively)
        prior = existing_lookup.get(eng_id.lower(), {})
        entry: dict = {"id": eng_id}
        # Metadata fields to preserve from prior entry
        for key in (
            "source_instrument", "role", "seance_score", "seance_score_note",
            "preset_count", "accent_color", "accent_name", "blessings", "notes",
        ):
            if key in prior:
                entry[key] = prior[key]
        # Authoritative fields from source
        if eng_id in prefix_map:
            entry["param_prefix"] = prefix_map[eng_id]
        entry["header"] = f"Source/Engines/{eng_id}/{eng_id}Engine.h"

        # Status: implemented (dir + prefix), pending_prefix, or pending_directory
        has_dir = eng_id in dirs
        has_prefix = eng_id in prefix_map
        if has_dir and has_prefix:
            entry["status"] = "implemented"
        elif has_dir and not has_prefix:
            entry["status"] = "pending_prefix"
        elif not has_dir and has_prefix:
            entry["status"] = "pending_directory"
        else:
            entry["status"] = "unknown"

        new_engines.append(entry)

    # Build new meta, preserving existing notes
    meta = existing_data.get("_meta", {})
    meta.setdefault("description", "Machine-readable engine roster for XOceanus. Single source of truth for engine count and metadata.")
    meta["last_sync"] = date.today().isoformat()
    meta["engine_count"] = sum(1 for e in new_engines if e["status"] == "implemented")
    meta["engine_count_total"] = len(new_engines)
    meta["status_values"] = ["implemented", "pending_prefix", "pending_directory"]
    meta["legacy_dir_aliases"] = LEGACY_DIRS
    meta["notes"] = (
        "This file is the single source of truth for engine roster and count. "
        "Parameter prefixes are FROZEN — never change after first registration. "
        "XOceanus has no fixed release cutoff. `status` reflects build state: "
        "'implemented' means the engine has both a Source/Engines/ directory AND "
        "a frozen prefix in PresetManager.h; 'pending_*' flags work-in-progress. "
        "Run Tools/sync_engine_sources.py to regenerate dependent files."
    )

    output: dict = {"_meta": meta, "synthesis_engines": new_engines}
    save_engines_json(output)
    print(f"[bootstrap] Wrote {ENGINES_JSON.relative_to(REPO_ROOT)}  "
          f"({len(new_engines)} total, {meta['engine_count']} implemented).")


# ---------------------------------------------------------------------------
# Regenerate Tools/engine_registry.py
# ---------------------------------------------------------------------------
def regenerate_engine_registry_py(data: dict) -> None:
    """Rewrite Tools/engine_registry.py as a thin JSON-backed module."""
    count = data["_meta"]["engine_count"]
    total = data["_meta"]["engine_count_total"]

    content = f'''#!/usr/bin/env python3
"""
engine_registry.py — Python view of the XOceanus engine roster.

AUTO-GENERATED from Docs/engines.json by Tools/sync_engine_sources.py.
DO NOT EDIT BY HAND — your changes will be overwritten on the next sync.

To modify the engine roster, edit Docs/engines.json and run:
    python Tools/sync_engine_sources.py

Implemented engines: {count}
Total entries (including pending): {total}

Usage:
    from engine_registry import get_all_engines, get_prefix, is_valid_engine
"""
from __future__ import annotations

import json
from functools import lru_cache
from pathlib import Path

_ENGINES_JSON = Path(__file__).resolve().parent.parent / "Docs" / "engines.json"


@lru_cache(maxsize=1)
def _data() -> dict:
    return json.loads(_ENGINES_JSON.read_text())


def get_all_engines(*, include_pending: bool = False) -> list[str]:
    """Return canonical engine IDs. By default only 'implemented' status."""
    entries = _data()["synthesis_engines"]
    if include_pending:
        return [e["id"] for e in entries]
    return [e["id"] for e in entries if e.get("status") == "implemented"]


def get_prefix(engine_id: str) -> str | None:
    """Return the frozen parameter prefix for an engine (with trailing underscore), or None."""
    for e in _data()["synthesis_engines"]:
        if e["id"] == engine_id:
            return e.get("param_prefix")
    return None


def get_engine(engine_id: str) -> dict | None:
    """Return the full metadata dict for an engine, or None."""
    for e in _data()["synthesis_engines"]:
        if e["id"] == engine_id:
            return dict(e)
    return None


def is_valid_engine(name: str) -> bool:
    """True if name is a canonical engine ID (implemented or pending)."""
    return any(e["id"] == name for e in _data()["synthesis_engines"])


def get_engine_count(*, include_pending: bool = False) -> int:
    """Return the engine count. By default only 'implemented' status."""
    return len(get_all_engines(include_pending=include_pending))


def get_legacy_aliases() -> dict[str, str]:
    """Return legacy-dir → canonical-id mapping (for Source/Engines/*/ directories)."""
    return dict(_data()["_meta"].get("legacy_dir_aliases", {{}}))


if __name__ == "__main__":
    import sys
    n = get_engine_count()
    total = get_engine_count(include_pending=True)
    print(f"XOceanus: {{n}} engines implemented ({{total}} total including pending)")
    for eng in get_all_engines():
        prefix = get_prefix(eng) or "<no-prefix>"
        print(f"  {{eng:20s}} {{prefix}}")
'''
    ENGINE_REGISTRY_PY.write_text(content)
    print(f"[sync]  wrote {ENGINE_REGISTRY_PY.relative_to(REPO_ROOT)}")


# ---------------------------------------------------------------------------
# Sentinel count updates
# ---------------------------------------------------------------------------
# Files with sentinel comments that should be auto-updated.
# Sentinel format:
#   <!-- ENGINE_COUNT -->77<!-- /ENGINE_COUNT -->
# The script finds every occurrence in these files and replaces the inner N
# with the implemented engine count.
SENTINEL_FILES: list[str] = [
    "CLAUDE.md",
    "README.md",
    "CHANGELOG.md",
    "SDK/README.md",
    "Docs/MANIFEST.md",
    "site/index.html",
    "site/updates.html",
    "site/feed.xml",
    "site/expedition.html",
    "site/press-kit/index.html",
    "site/press-kit/press-release.md",
]


def _apply_sentinel(text: str, pattern: re.Pattern, tag: str, n: int) -> tuple[str, int]:
    """Replace every sentinel of the given tag with the count N. Returns (new_text, count_found)."""
    matches = pattern.findall(text)
    if not matches:
        return text, 0
    replacement = f"<!-- {tag} -->{n}<!-- /{tag} -->"
    return pattern.sub(replacement, text), len(matches)


def update_sentinels(implemented: int, designed: int, check_only: bool = False) -> bool:
    """Update both ENGINE_COUNT (implemented) and ENGINE_COUNT_DESIGNED (full fleet design)
    sentinel markers across SENTINEL_FILES."""
    ok = True
    for rel in SENTINEL_FILES:
        path = REPO_ROOT / rel
        if not path.exists():
            continue
        text = path.read_text()
        new_text, n_impl = _apply_sentinel(text, SENTINEL_PATTERN, "ENGINE_COUNT", implemented)
        new_text, n_des = _apply_sentinel(new_text, SENTINEL_DESIGNED_PATTERN, "ENGINE_COUNT_DESIGNED", designed)
        if n_impl == 0 and n_des == 0:
            continue
        tag = f"{n_impl} impl + {n_des} designed marker(s)"
        if new_text == text:
            print(f"[sync]  sentinel OK:  {rel} ({tag})")
            continue
        if check_only:
            print(f"[check] sentinel STALE: {rel}")
            ok = False
        else:
            path.write_text(new_text)
            print(f"[sync]  sentinel updated: {rel} ({tag} → {implemented}/{designed})")
    return ok


def claude_md_table_drift_check(data: dict) -> bool:
    """Verify CLAUDE.md's parameter-prefix table matches engines.json roster and prefixes."""
    path = REPO_ROOT / "CLAUDE.md"
    if not path.exists():
        return True
    text = path.read_text()
    pattern = re.compile(r'^\| ([A-Z][a-zA-Z]+)(?:\s*\([^)]+\))?\s*\|\s*`([a-z0-9_]+_)`', re.MULTILINE)
    claude_map = dict(pattern.findall(text))
    json_map = {e["id"]: e.get("param_prefix") for e in data["synthesis_engines"]}
    json_ids = set(json_map) - {None}

    ok = True
    claude_only = set(claude_map) - set(json_map)
    json_only = set(json_map) - set(claude_map)
    if claude_only:
        print(f"[check] DRIFT: in CLAUDE.md param-prefix table but not engines.json: {sorted(claude_only)}")
        ok = False
    if json_only:
        print(f"[check] DRIFT: in engines.json but missing from CLAUDE.md table: {sorted(json_only)}")
        ok = False
    for eid in sorted(set(claude_map) & set(json_map)):
        if claude_map[eid] != json_map[eid]:
            print(f"[check] DRIFT: {eid} prefix CLAUDE.md='{claude_map[eid]}' vs engines.json='{json_map[eid]}'")
            ok = False
    return ok


# ---------------------------------------------------------------------------
# Drift check
# ---------------------------------------------------------------------------
def drift_check(data: dict) -> bool:
    """Verify engines.json is consistent with PresetManager.h and Source/Engines/.

    Status-aware: 'implemented' engines must be in both runtime sources;
    'designed' engines should be absent from runtime (prefix reserved in design
    registry / CLAUDE.md only)."""
    ok = True
    impl_ids = {e["id"] for e in data["synthesis_engines"] if e.get("status") == "implemented"}
    designed_ids = {e["id"] for e in data["synthesis_engines"] if e.get("status") == "designed"}
    prefix_map = parse_frozen_prefix_map()
    prefix_ids = set(prefix_map.keys())
    dir_ids = list_canonical_engine_dirs()

    # Implemented status: must be in runtime + dirs
    impl_missing_prefix = impl_ids - prefix_ids
    impl_missing_dir = impl_ids - dir_ids
    if impl_missing_prefix:
        print(f"[check] DRIFT: status=implemented but not in PresetManager.h frozenPrefixForEngine: {sorted(impl_missing_prefix)}")
        ok = False
    if impl_missing_dir:
        print(f"[check] DRIFT: status=implemented but no Source/Engines/ dir: {sorted(impl_missing_dir)}")
        ok = False

    # Unregistered: in runtime/dirs but not engines.json at all
    all_json = impl_ids | designed_ids
    only_prefix = prefix_ids - all_json
    only_dir = dir_ids - all_json
    if only_prefix:
        print(f"[check] DRIFT: in PresetManager.h frozenPrefixForEngine but not engines.json: {sorted(only_prefix)}")
        ok = False
    if only_dir:
        print(f"[check] DRIFT: Source/Engines/ dir but not engines.json: {sorted(only_dir)}")
        ok = False

    # Designed status: should NOT have a Source/Engines/ dir yet (that'd make it implemented)
    designed_with_dir = designed_ids & dir_ids
    if designed_with_dir:
        print(f"[check] DRIFT: status=designed but has Source/Engines/ dir (update status to 'implemented'?): {sorted(designed_with_dir)}")
        ok = False

    # Per-engine prefix agreement (only for engines present in both)
    for e in data["synthesis_engines"]:
        eid = e["id"]
        exp_prefix = prefix_map.get(eid)
        got_prefix = e.get("param_prefix")
        if exp_prefix and got_prefix and exp_prefix != got_prefix:
            print(f"[check] DRIFT: {eid} prefix: engines.json='{got_prefix}' "
                  f"PresetManager.h='{exp_prefix}'")
            ok = False

    return ok


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------
def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--bootstrap", action="store_true",
                    help="Rebuild engines.json from PresetManager.h (one-time migration)")
    ap.add_argument("--check", action="store_true",
                    help="Exit non-zero if anything is out of sync (dry-run)")
    args = ap.parse_args()

    if args.bootstrap:
        bootstrap()
        print()

    data = load_engines_json()
    implemented_count = data["_meta"]["engine_count"]
    designed_count = data["_meta"].get("engine_count_designed", data["_meta"].get("engine_count_total", implemented_count))

    print(f"[sync]  implemented: {implemented_count}  |  designed (fleet design): {designed_count}")

    # Drift check
    drift_ok = drift_check(data)
    table_ok = claude_md_table_drift_check(data)

    # Sentinels
    sentinel_ok = update_sentinels(implemented_count, designed_count, check_only=args.check)

    if not args.check:
        regenerate_engine_registry_py(data)

    if args.check:
        if drift_ok and table_ok and sentinel_ok:
            print("[check] OK — engine sources are in sync.")
            return 0
        print("[check] FAIL — drift detected.")
        return 1

    print("[sync]  done.")
    return 0 if (drift_ok and table_ok) else 1


if __name__ == "__main__":
    sys.exit(main())
