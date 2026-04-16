#!/usr/bin/env python3
"""
engine_registry.py — Python view of the XOceanus engine roster.

AUTO-GENERATED from Docs/engines.json by Tools/sync_engine_sources.py.
DO NOT EDIT BY HAND — your changes will be overwritten on the next sync.

To modify the engine roster, edit Docs/engines.json and run:
    python Tools/sync_engine_sources.py

Implemented engines: 81
Total entries (including pending): 104

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
    return dict(_data()["_meta"].get("legacy_dir_aliases", {}))


if __name__ == "__main__":
    import sys
    n = get_engine_count()
    total = get_engine_count(include_pending=True)
    print(f"XOceanus: {n} engines implemented ({total} total including pending)")
    for eng in get_all_engines():
        prefix = get_prefix(eng) or "<no-prefix>"
        print(f"  {eng:20s} {prefix}")
