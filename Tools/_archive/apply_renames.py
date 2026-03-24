#!/usr/bin/env python3
"""
Bulk rename script for XOlokun preset files, documentation, and tools.

Renames:
  - XOddCouple → OddfeliX (in presets, tools)
  - XOddCouple Engine X → OddfeliX (in docs)
  - XOddCouple Engine O → OddOscar (in docs)
  - XOddCouple (general) → OddfeliX/OddOscar (in docs)
  - XOblongBob → XOblong (everywhere)
  - Terracotta #C8553D → Neon Tetra Blue #00A6D6 (docs)
  - Teal #2A9D8F → Axolotl Gill Pink #E8839B (docs)
  - RGB (200, 85, 61) → (0, 166, 214) (tools)
  - RGB (42, 157, 143) → (232, 131, 155) (tools)
"""

import json
import re
from pathlib import Path
from collections import OrderedDict

ROOT = Path(__file__).resolve().parent.parent


# ─────────────────────────────────────────────
# Part 1: .xometa preset files
# ─────────────────────────────────────────────

def rename_xometa_files():
    """Rename XOddCouple→OddfeliX and XOblongBob→XOblong in .xometa files."""
    preset_dir = ROOT / "Presets" / "XOlokun"
    xometa_files = sorted(preset_dir.rglob("*.xometa"))

    modified_count = 0

    for path in xometa_files:
        original_text = path.read_text(encoding="utf-8")
        data = json.loads(original_text)
        changed = False

        # --- engines array ---
        if "engines" in data and isinstance(data["engines"], list):
            new_engines = []
            for e in data["engines"]:
                if e == "XOddCouple":
                    new_engines.append("OddfeliX")
                    changed = True
                elif e == "XOblongBob":
                    new_engines.append("XOblong")
                    changed = True
                else:
                    new_engines.append(e)
            data["engines"] = new_engines

        # --- parameters object: rename keys ---
        if "parameters" in data and isinstance(data["parameters"], dict):
            new_params = OrderedDict()
            for k, v in data["parameters"].items():
                if k == "XOddCouple":
                    new_params["OddfeliX"] = v
                    changed = True
                elif k == "XOblongBob":
                    new_params["XOblong"] = v
                    changed = True
                else:
                    new_params[k] = v
            data["parameters"] = dict(new_params)

        # --- legacy.sourceInstrument ---
        if "legacy" in data and isinstance(data["legacy"], dict):
            si = data["legacy"].get("sourceInstrument")
            if si == "XOddCouple":
                data["legacy"]["sourceInstrument"] = "OddfeliX"
                changed = True
            elif si == "XOblongBob":
                data["legacy"]["sourceInstrument"] = "XOblong"
                changed = True

        # --- tags ---
        if "tags" in data and isinstance(data["tags"], list):
            new_tags = []
            for t in data["tags"]:
                if "oddcouple" in t.lower() and "dyssey" not in t.lower():
                    new_tags.append(t.replace("oddcouple", "oddfelix").replace("OddCouple", "OddfeliX").replace("ODDCOUPLE", "ODDFELIX"))
                    if new_tags[-1] != t:
                        changed = True
                else:
                    new_tags.append(t)
            data["tags"] = new_tags

        # --- sequencer tracks: engine references ---
        if "sequencer" in data and isinstance(data.get("sequencer"), dict):
            tracks = data["sequencer"].get("tracks", [])
            for track in tracks:
                if track.get("engine") == "XOddCouple":
                    track["engine"] = "OddfeliX"
                    changed = True
                elif track.get("engine") == "XOblongBob":
                    track["engine"] = "XOblong"
                    changed = True

        # --- coupling: engine references ---
        if "coupling" in data and isinstance(data.get("coupling"), dict):
            coupling = data["coupling"]
            # Handle both flat and nested coupling structures
            def rename_coupling_dict(d):
                nonlocal changed
                for field in ("sourceEngine", "targetEngine", "engine", "engineA", "engineB"):
                    if d.get(field) == "XOddCouple":
                        d[field] = "OddfeliX"
                        changed = True
                    elif d.get(field) == "XOblongBob":
                        d[field] = "XOblong"
                        changed = True

            rename_coupling_dict(coupling)
            # Handle nested lists (entries, pairs, etc.)
            for list_key in ("entries", "pairs", "slots"):
                if isinstance(coupling.get(list_key), list):
                    for entry in coupling[list_key]:
                        if isinstance(entry, dict):
                            rename_coupling_dict(entry)

        # --- description: text replacement ---
        if "description" in data and isinstance(data["description"], str):
            new_desc = data["description"].replace("XOddCouple", "OddfeliX").replace("XOblongBob", "XOblong")
            if new_desc != data["description"]:
                data["description"] = new_desc
                changed = True

        if changed:
            out = json.dumps(data, indent=2, ensure_ascii=False) + "\n"
            path.write_text(out, encoding="utf-8")
            modified_count += 1

    return modified_count, len(xometa_files)


# ─────────────────────────────────────────────
# Part 2: Documentation files (Docs/*.md)
# ─────────────────────────────────────────────

def rename_docs():
    """Apply string replacements in Docs/*.md files."""
    docs_dir = ROOT / "Docs"
    md_files = sorted(docs_dir.glob("*.md"))

    # Order matters: specific patterns first, then generic
    replacements = [
        ("XOddCouple Engine X", "OddfeliX"),
        ("XOddCouple Engine O", "OddOscar"),
        ("XOddCouple", "OddfeliX/OddOscar"),
        ("XOblongBob", "XOblong"),
        ("Terracotta `#C8553D`", "Neon Tetra Blue `#00A6D6`"),
        ("`#C8553D`", "`#00A6D6`"),
        ("Teal `#2A9D8F`", "Axolotl Gill Pink `#E8839B`"),
        ("`#2A9D8F`", "`#E8839B`"),
    ]

    modified_count = 0

    for path in md_files:
        original = path.read_text(encoding="utf-8")
        text = original

        for old, new in replacements:
            text = text.replace(old, new)

        if text != original:
            path.write_text(text, encoding="utf-8")
            modified_count += 1

    return modified_count, len(md_files)


# ─────────────────────────────────────────────
# Part 3: Tools/*.py files
# ─────────────────────────────────────────────

def rename_tools():
    """Apply renames in Tools/*.py files."""
    tools_dir = ROOT / "Tools"

    target_files = [
        "compute_preset_dna.py",
        "extract_cpp_presets.py",
        "migrate_xocmeta_to_xometa.py",
        "generate_onset_presets.py",
        "xpn_cover_art.py",
    ]

    # String replacements (order matters)
    string_replacements = [
        ("XOddCouple", "OddfeliX"),
        ("XOblongBob", "XOblong"),
    ]

    # RGB replacements for xpn_cover_art.py
    rgb_replacements = [
        # Terracotta → Neon Tetra Blue
        ("(200,  85,  61)", "(0,  166, 214)"),
        ("(200, 85, 61)", "(0, 166, 214)"),
        # Teal → Axolotl Gill Pink
        ("(42,  157, 143)", "(232, 131, 155)"),
        ("(42, 157, 143)", "(232, 131, 155)"),
    ]

    modified_count = 0

    for fname in target_files:
        path = tools_dir / fname
        if not path.exists():
            print(f"  WARNING: {path} not found, skipping")
            continue

        original = path.read_text(encoding="utf-8")
        text = original

        for old, new in string_replacements:
            text = text.replace(old, new)

        for old, new in rgb_replacements:
            text = text.replace(old, new)

        if text != original:
            path.write_text(text, encoding="utf-8")
            modified_count += 1

    return modified_count, len(target_files)


# ─────────────────────────────────────────────
# Main
# ─────────────────────────────────────────────

if __name__ == "__main__":
    print("=" * 60)
    print("XOlokun Bulk Rename Script")
    print("=" * 60)

    print("\n[1/3] Renaming .xometa preset files...")
    mod, total = rename_xometa_files()
    print(f"  Modified {mod} of {total} .xometa files")

    print("\n[2/3] Renaming documentation files...")
    mod, total = rename_docs()
    print(f"  Modified {mod} of {total} .md files")

    print("\n[3/3] Renaming Tools/*.py files...")
    mod, total = rename_tools()
    print(f"  Modified {mod} of {total} target .py files")

    print("\nDone.")
