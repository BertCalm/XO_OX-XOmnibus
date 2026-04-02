#!/usr/bin/env python3
"""Identify and rename debug/pole-fill preset names to evocative 2-3 word names.
Also supports a --normalize-case mode for ALL_CAPS legitimate names.

Detection patterns (a preset is "debug" if it matches ANY):
  1. Starts with _ALL prefix: _ALLHI_FND_01, _ALLLO_ATM_03
  2. Starts with 5X or 3X: 5X DARK COLD KINETIC DENSE
  3. Contains mood codes + numbers: (FND|FOU|ATM|ENT|PRI|FLU|AET|FAM|SUB)\\s*_?\\s*\\d+
  4. 4+ word ALL-CAPS descriptor dumps WITH a digit in the full string

These presets get NEW evocative names generated from DNA (existing word pool logic).

Case-normalisation mode (--normalize-case):
  Finds presets whose names are ALL_CAPS legitimate evocative names (not debug patterns)
  and normalises them to Title Case:
    COPPER_CATHEDRAL → Copper Cathedral
    BLACK_TIDE       → Black Tide
  Only normalises 2-3 word names, ≤ 30 chars, no numbers in the name.

Usage:
  python rename_debug_presets.py                            # dry-run: debug renames
  python rename_debug_presets.py --normalize-case           # dry-run: case normalizations
  python rename_debug_presets.py --apply                    # apply debug renames only
  python rename_debug_presets.py --apply --normalize-case   # apply both
"""

import argparse
import json
import os
import pathlib
import re
import sys

REPO_ROOT = pathlib.Path(__file__).parent.parent
PRESET_DIR = REPO_ROOT / "Presets" / "XOceanus"

# ---------------------------------------------------------------------------
# Detection patterns
# ---------------------------------------------------------------------------

_MOOD_CODE_RE = re.compile(
    r"(FND|ATM|ENT|PRI|FLU|AET|FAM|SUB|FOU)\s*_?\s*\d+", re.IGNORECASE
)
_NX_PREFIX_RE = re.compile(r"^[35]X\s")


def is_debug_preset(name: str) -> tuple[bool, str]:
    """Return (True, reason) if the preset name is a debug/pole-fill name."""
    if not name:
        return False, ""

    # Pattern 1: starts with _ALL
    if name.startswith("_ALL"):
        return True, "_ALL prefix"

    # Pattern 2: 5X / 3X prefix
    if _NX_PREFIX_RE.match(name):
        return True, "5X/3X prefix"

    # Pattern 3: mood code + number anywhere (optional separator between code and digit)
    if _MOOD_CODE_RE.search(name):
        return True, "mood code + number"

    # Pattern 4: 4+ ALL-CAPS words AND the string contains at least one digit
    #   e.g. "DARK COLD VAST VIOLENT FAM 4" — a descriptor dump with a trailing number
    #   Underscores count as word separators for splitting purposes.
    parts = re.split(r"[\s_]+", name)
    has_digit = bool(re.search(r"\d", name))
    if (
        len(parts) >= 4
        and all(p == p.upper() and p != "" for p in parts)
        and has_digit
    ):
        return True, "4+ CAPS word dump with digit"

    return False, ""


# ---------------------------------------------------------------------------
# Case normalisation helpers
# ---------------------------------------------------------------------------

_HAS_DIGIT_RE = re.compile(r"\d")


def is_normalizable_caps(name: str) -> bool:
    """Return True if name is an ALL_CAPS (or ALL CAPS) legitimate evocative name
    that should be normalised to Title Case.

    Rules:
    - NOT a debug pattern
    - 2–3 words (split on spaces or underscores)
    - Total length ≤ 30 chars (post-normalisation)
    - No digits
    - All parts are purely uppercase letters (no mixed-case already)
    """
    if not name:
        return False
    # Must not be a debug pattern
    hit, _ = is_debug_preset(name)
    if hit:
        return False
    # No digits
    if _HAS_DIGIT_RE.search(name):
        return False
    # Split on space/underscore
    parts = re.split(r"[\s_]+", name)
    # 2–3 words
    if not (2 <= len(parts) <= 3):
        return False
    # All parts must be non-empty and ALL CAPS (meaning they're uppercase and contain letters)
    for p in parts:
        if not p:
            return False
        if not p.isupper():
            return False
        if not p.isalpha():
            return False
    # Post-normalised name length check (Title Case with spaces)
    normalised = " ".join(p.capitalize() for p in parts)
    if len(normalised) > 30:
        return False
    return True


def normalize_name(name: str) -> str:
    """Convert ALL_CAPS or ALL CAPS name to Title Case with spaces."""
    parts = re.split(r"[\s_]+", name)
    return " ".join(p.capitalize() for p in parts if p)


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
# Main scan functions
# ---------------------------------------------------------------------------

def scan_debug_presets(preset_dir: pathlib.Path) -> list[tuple[pathlib.Path, dict, str]]:
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


def scan_normalizable_presets(preset_dir: pathlib.Path) -> list[tuple[pathlib.Path, dict, str, str]]:
    """Return list of (path, data, old_name, new_name) for ALL_CAPS presets to normalise."""
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
            if is_normalizable_caps(name):
                new_name = normalize_name(name)
                results.append((fpath, data, name, new_name))
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
            except Exception as e:
                print(f'[WARN] Rename failed: {e}', file=sys.stderr)
    return names


# ---------------------------------------------------------------------------
# Report helpers
# ---------------------------------------------------------------------------

def _sample(items: list, n: int = 10) -> list:
    """Return up to n evenly-spaced items from a list for a representative sample."""
    if len(items) <= n:
        return items
    step = len(items) / n
    return [items[int(i * step)] for i in range(n)]


# ---------------------------------------------------------------------------
# main
# ---------------------------------------------------------------------------

def main() -> None:
    parser = argparse.ArgumentParser(
        description="Rename debug presets and/or normalise ALL_CAPS preset names."
    )
    parser.add_argument(
        "--apply",
        action="store_true",
        default=False,
        help="Apply changes (default is dry-run).",
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        default=False,
        help="Print changes without writing files (default behaviour).",
    )
    parser.add_argument(
        "--normalize-case",
        action="store_true",
        default=False,
        help="Show/apply Title Case normalisation for ALL_CAPS evocative names.",
    )
    args = parser.parse_args()

    dry_run = not args.apply

    # -----------------------------------------------------------------------
    # Debug rename pass
    # -----------------------------------------------------------------------
    debug_presets = scan_debug_presets(PRESET_DIR)

    if not debug_presets:
        print("No debug presets found.")
    else:
        skip_set = {fpath for fpath, _, _ in debug_presets}
        existing_names = collect_all_names(PRESET_DIR, skip_set)

        renames: list[tuple[str, str, pathlib.Path, pathlib.Path]] = []

        for idx, (fpath, data, reason) in enumerate(debug_presets):
            old_name = data.get("name", "")
            dna = data.get("dna", {})
            new_name = generate_name(dna, idx, existing_names)
            existing_names.add(new_name)

            new_stem = new_name.replace(" ", "_")
            new_fname = f"{new_stem}.xometa"
            new_path = fpath.parent / new_fname

            renames.append((old_name, new_name, fpath, new_path))

        mode_label = "DRY RUN" if dry_run else "APPLYING"
        print(f"\n[{mode_label}] Debug presets found: {len(renames)}")
        print(f"Sample (up to 10):")
        for old_name, new_name, old_path, new_path in _sample(renames):
            rel_old = old_path.relative_to(PRESET_DIR)
            print(f"  {old_name!r}  ->  {new_name!r}  ({rel_old})")

        print(f"\nTotal: {len(renames)} debug presets to rename.")

        if not dry_run:
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
            print(f"Applied: {applied} debug renames  |  Errors: {errors}")
        else:
            print("(Dry-run — no files written. Pass --apply to commit changes.)")

    # -----------------------------------------------------------------------
    # Case normalisation pass
    # -----------------------------------------------------------------------
    if args.normalize_case:
        normalizable = scan_normalizable_presets(PRESET_DIR)

        print()
        if not normalizable:
            print("No ALL_CAPS preset names found for normalisation.")
        else:
            mode_label = "DRY RUN" if dry_run else "APPLYING"
            print(f"[{mode_label}] ALL_CAPS presets to normalise: {len(normalizable)}")
            print(f"Sample (up to 10):")
            for fpath, data, old_name, new_name in _sample(normalizable):
                rel = fpath.relative_to(PRESET_DIR)
                print(f"  {old_name!r}  ->  {new_name!r}  ({rel})")

            print(f"\nTotal: {len(normalizable)} presets to normalise.")

            if not dry_run:
                applied = 0
                errors = 0
                for fpath, data, old_name, new_name in normalizable:
                    try:
                        # Re-read to avoid stale data if debug pass also touched files
                        current = json.loads(fpath.read_text(encoding="utf-8"))
                        current["name"] = new_name
                        # Also rename the file to match
                        new_stem = new_name.replace(" ", "_")
                        new_path = fpath.parent / f"{new_stem}.xometa"
                        new_path.write_text(
                            json.dumps(current, indent=2, ensure_ascii=False) + "\n",
                            encoding="utf-8",
                        )
                        if fpath != new_path:
                            fpath.unlink()
                        applied += 1
                    except Exception as exc:
                        print(f"  ERROR normalising {fpath}: {exc}", file=sys.stderr)
                        errors += 1
                print(f"Applied: {applied} case normalisations  |  Errors: {errors}")
            else:
                print("(Dry-run — no files written. Pass --apply to commit changes.)")


if __name__ == "__main__":
    main()
