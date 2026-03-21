#!/usr/bin/env python3
"""Identify and rename debug/pole-fill preset names to evocative 2-3 word names.

Detection patterns (a preset is "debug" if it matches ANY):
  1. ALL-CAPS with underscores/numbers: ^[A-Z0-9_\\s]{10,}$
  2. Starts with _ALL
  3. Contains mood codes + numbers: (FND|ATM|ENT|PRI|FLU|AET|FAM|SUB|FOU)\\s*\\d+
  4. 4+ word ALL-CAPS descriptor dumps
  5. Contains "5X" or "3X" prefix

Usage:
  python rename_debug_presets.py           # dry-run (default)
  python rename_debug_presets.py --dry-run # explicit dry-run
  python rename_debug_presets.py --apply   # rename files + update JSON name field
"""

import argparse
import json
import os
import pathlib
import re
import sys

REPO_ROOT = pathlib.Path(__file__).parent.parent
PRESET_DIR = REPO_ROOT / "Presets" / "XOmnibus"

# ---------------------------------------------------------------------------
# Detection patterns
# ---------------------------------------------------------------------------

_ALL_CAPS_RE = re.compile(r"^[A-Z0-9_\s]{10,}$")
_MOOD_CODE_RE = re.compile(
    r"(FND|ATM|ENT|PRI|FLU|AET|FAM|SUB|FOU)\s*\d+", re.IGNORECASE
)
_NX_PREFIX_RE = re.compile(r"^[35]X\s")


def is_debug_preset(name: str) -> tuple[bool, str]:
    """Return (True, reason) if the preset name is a debug/pole-fill name."""
    if not name:
        return False, ""

    # Pattern 2: starts with _ALL
    if name.startswith("_ALL"):
        return True, "_ALL prefix"

    # Pattern 5: 5X / 3X prefix
    if _NX_PREFIX_RE.match(name):
        return True, "5X/3X prefix"

    # Pattern 3: mood code + number anywhere
    if _MOOD_CODE_RE.search(name):
        return True, "mood code + number"

    # Pattern 4: 4+ ALL-CAPS words (check before pattern 1 — more specific)
    parts = name.split()
    if len(parts) >= 4 and all(p == p.upper() for p in parts):
        return True, "4+ CAPS word dump"

    # Pattern 1: ALL-CAPS, underscores, numbers, length ≥ 10
    if _ALL_CAPS_RE.match(name):
        return True, "all-caps identifier"

    return False, ""


# ---------------------------------------------------------------------------
# Name generation
# ---------------------------------------------------------------------------

WORD_POOLS = {
    "bright_high": [
        "Prismatic", "Crystal", "Aurora", "Neon", "Solar",
        "Phosphor", "Glint", "Shimmer", "Radiant", "Luminous",
    ],
    "bright_low": [
        "Shadow", "Umbra", "Dusk", "Obsidian", "Midnight",
        "Charcoal", "Matte", "Shroud", "Eclipse", "Penumbra",
    ],
    "warm_high": [
        "Amber", "Copper", "Velvet", "Honey", "Ember",
        "Sienna", "Mahogany", "Brass", "Cinnamon", "Terracotta",
    ],
    "warm_low": [
        "Frost", "Steel", "Mercury", "Arctic", "Chrome",
        "Titanium", "Ice", "Quartz", "Platinum", "Silver",
    ],
    "movement_high": [
        "Flux", "Drift", "Cascade", "Pulse", "Surge",
        "Ripple", "Undulation", "Current", "Eddy", "Spiral",
    ],
    "movement_low": [
        "Still", "Frozen", "Anchored", "Monolith", "Resting",
        "Suspended", "Poised", "Silent", "Calm", "Serene",
    ],
    "density_high": [
        "Dense", "Massive", "Saturated", "Layered", "Thick",
        "Heavy", "Opaque", "Solid", "Packed", "Rich",
    ],
    "density_low": [
        "Thin", "Sparse", "Ethereal", "Gossamer", "Wisp",
        "Filament", "Thread", "Trace", "Vapor", "Mist",
    ],
    "space_high": [
        "Vast", "Cathedral", "Ocean", "Cosmos", "Horizon",
        "Expanse", "Infinite", "Void", "Nebula", "Abyss",
    ],
    "space_low": [
        "Intimate", "Close", "Pocket", "Chamber", "Alcove",
        "Niche", "Cocoon", "Core", "Nucleus", "Hearth",
    ],
    "aggression_high": [
        "Blade", "Strike", "Fracture", "Impact", "Shatter",
        "Voltage", "Sting", "Pierce", "Barb", "Slash",
    ],
    "aggression_low": [
        "Gentle", "Caress", "Whisper", "Feather", "Silk",
        "Tender", "Lull", "Soothe", "Balm", "Grace",
    ],
}

# Map (dimension, high_or_low) → pool key
_DIM_POOL = {
    ("brightness", True):  "bright_high",
    ("brightness", False): "bright_low",
    ("warmth",     True):  "warm_high",
    ("warmth",     False): "warm_low",
    ("movement",   True):  "movement_high",
    ("movement",   False): "movement_low",
    ("density",    True):  "density_high",
    ("density",    False): "density_low",
    ("space",      True):  "space_high",
    ("space",      False): "space_low",
    ("aggression", True):  "aggression_high",
    ("aggression", False): "aggression_low",
}

ROMAN = {2: "II", 3: "III", 4: "IV", 5: "V", 6: "VI", 7: "VII", 8: "VIII",
         9: "IX", 10: "X"}


def _pick_word(pool_key: str, index: int) -> str:
    pool = WORD_POOLS[pool_key]
    return pool[index % len(pool)]


def _two_most_extreme(dna: dict) -> list[tuple[str, bool, float]]:
    """Return the 2 dimensions furthest from 0.5, as (dim, is_high, deviation)."""
    dims = ["brightness", "warmth", "movement", "density", "space", "aggression"]
    scored = []
    for d in dims:
        v = dna.get(d, 0.5)
        dev = abs(v - 0.5)
        is_high = v >= 0.5
        scored.append((d, is_high, dev))
    scored.sort(key=lambda x: x[2], reverse=True)
    return scored[:2]


def generate_name(dna: dict, index: int, existing_names: set) -> str:
    """Generate an evocative 2-word name from DNA, avoiding collisions."""
    extremes = _two_most_extreme(dna)

    if len(extremes) < 2:
        # Degenerate case — flat DNA
        base = "Neutral Wave"
        if base not in existing_names:
            return base
        for n in range(2, 20):
            candidate = f"Neutral Wave {ROMAN.get(n, str(n))}"
            if candidate not in existing_names:
                return candidate
        return f"Neutral Wave {index}"

    dim1, high1, _ = extremes[0]
    dim2, high2, _ = extremes[1]

    pool_key1 = _DIM_POOL[(dim1, high1)]
    pool_key2 = _DIM_POOL[(dim2, high2)]

    # Try combinations with varying index offsets to find an unused name
    for offset in range(len(WORD_POOLS[pool_key1])):
        w1 = _pick_word(pool_key1, index + offset)
        for offset2 in range(len(WORD_POOLS[pool_key2])):
            w2 = _pick_word(pool_key2, index + offset2)
            if w1 == w2:
                continue
            base = f"{w1} {w2}"
            if base not in existing_names:
                return base
            # Try roman numerals
            for n in range(2, 11):
                candidate = f"{base} {ROMAN.get(n, str(n))}"
                if candidate not in existing_names:
                    return candidate

    # Last resort: index-based
    fallback = f"Tone Field {index}"
    if fallback not in existing_names:
        return fallback
    n = 2
    while f"Tone Field {n}" in existing_names:
        n += 1
    return f"Tone Field {n}"


# ---------------------------------------------------------------------------
# Main scan + rename
# ---------------------------------------------------------------------------

def scan_presets(preset_dir: pathlib.Path) -> list[tuple[pathlib.Path, dict, str]]:
    """Return list of (path, data, reason) for all debug presets."""
    results = []
    for root, _dirs, files in os.walk(preset_dir):
        for fname in sorted(files):
            if not fname.endswith(".xometa"):
                continue
            fpath = pathlib.Path(root) / fname
            try:
                data = json.loads(fpath.read_text(encoding="utf-8"))
            except Exception:
                continue
            name = data.get("name", "")
            hit, reason = is_debug_preset(name)
            if hit:
                results.append((fpath, data, reason))
    return results


def collect_all_names(preset_dir: pathlib.Path, skip_paths: set) -> set:
    """Collect all existing preset names, skipping paths we're about to rename."""
    names = set()
    for root, _dirs, files in os.walk(preset_dir):
        for fname in files:
            if not fname.endswith(".xometa"):
                continue
            fpath = pathlib.Path(root) / fname
            if fpath in skip_paths:
                continue
            try:
                data = json.loads(fpath.read_text(encoding="utf-8"))
                if n := data.get("name"):
                    names.add(n)
            except Exception:
                pass
    return names


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Rename debug/pole-fill preset names to evocative 2-3 word names."
    )
    parser.add_argument(
        "--apply",
        action="store_true",
        default=False,
        help="Apply renames (default is dry-run).",
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        default=False,
        help="Print renames without writing files (default behaviour).",
    )
    args = parser.parse_args()

    dry_run = not args.apply

    debug_presets = scan_presets(PRESET_DIR)

    if not debug_presets:
        print("No debug presets found.")
        return

    # Collect all names that exist in the fleet EXCLUDING the debug set
    skip_set = {fpath for fpath, _, _ in debug_presets}
    existing_names = collect_all_names(PRESET_DIR, skip_set)

    renames: list[tuple[str, str, pathlib.Path, pathlib.Path]] = []  # (old, new, old_path, new_path)

    for idx, (fpath, data, reason) in enumerate(debug_presets):
        old_name = data.get("name", "")
        dna = data.get("dna", {})
        new_name = generate_name(dna, idx, existing_names)
        existing_names.add(new_name)

        # Build new filename: spaces → underscores, keep .xometa extension
        new_stem = new_name.replace(" ", "_")
        new_fname = f"{new_stem}.xometa"
        new_path = fpath.parent / new_fname

        renames.append((old_name, new_name, fpath, new_path))

    # --- Report ---
    mode_label = "DRY RUN" if dry_run else "APPLYING"
    print(f"\n[{mode_label}] Debug presets found: {len(renames)}\n")

    for old_name, new_name, old_path, new_path in renames:
        rel_old = old_path.relative_to(PRESET_DIR)
        rel_new = new_path.relative_to(PRESET_DIR)
        print(f"  {str(rel_old)!s}")
        print(f"    -> {new_name}  ({str(rel_new)})")

    print(f"\nTotal: {len(renames)} presets to rename.\n")

    if dry_run:
        print("(Dry-run — no files written. Pass --apply to commit changes.)")
        return

    # --- Apply ---
    applied = 0
    errors = 0
    for old_name, new_name, old_path, new_path in renames:
        try:
            data = json.loads(old_path.read_text(encoding="utf-8"))
            data["name"] = new_name
            new_path.write_text(
                json.dumps(data, indent=2, ensure_ascii=False) + "\n",
                encoding="utf-8",
            )
            if old_path != new_path:
                old_path.unlink()
            applied += 1
        except Exception as exc:
            print(f"  ERROR renaming {old_path}: {exc}", file=sys.stderr)
            errors += 1

    print(f"Applied: {applied} renames  |  Errors: {errors}")


if __name__ == "__main__":
    main()
