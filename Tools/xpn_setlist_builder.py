#!/usr/bin/env python3
"""
XPN Setlist Builder — XO_OX Designs
====================================
Generates a "setlist" — an ordered sequence of XPN packs optimised for a live
performance set, using Sonic DNA metadata to maximize arc, momentum, and tonal
variety.

Algorithm overview
------------------
1. Scan `--packs-dir` for .xpn files (ZIP archives).
2. Open each ZIP and read `bundle_manifest.json` to extract Sonic DNA
   (brightness, warmth, movement, density, space, aggression) and mood.
   Packs missing the manifest receive placeholder DNA (all 0.5) with a warning.
3. Select `--set-length` packs from the pool (defaults to all) by running a
   greedy search that maximises a weighted composite score:
     - contrast_weight  (default 0.5): reward large Euclidean distance between
       adjacent DNA vectors — prevents same-vibe runs.
     - arc_weight       (default 0.3): penalise mood placement that violates the
       expected performance arc:
         * slots 1–2 → Foundation preferred
         * slots 3–(N-2) → Entangled / Prism / Flux preferred
         * slots (N-1)–N → Atmosphere / Aether preferred
     - energy_weight    (default 0.2): reward builds where warmth + aggression
       rises toward the centre of the set then resolves toward the end.
4. Write the result as JSON and/or print a numbered human-readable list.

Usage
-----
  python xpn_setlist_builder.py --packs-dir ./dist --set-length 8 \\
      --output setlist.json --format json

  python xpn_setlist_builder.py --packs-dir ./dist --format text

  python xpn_setlist_builder.py --packs-dir ./dist --set-length 6 \\
      --output setlist.json --format both

Options
-------
  --packs-dir DIR    Directory containing .xpn files (required)
  --set-length N     Number of packs to include (default: all available)
  --output PATH      File path to write the setlist JSON (optional)
  --format           text | json | both  (default: both)
"""

import argparse
import json
import math
import os
import sys
import zipfile
from datetime import datetime, timezone
from typing import Dict, List, Optional, Tuple

# ---------------------------------------------------------------------------
# Constants
# ---------------------------------------------------------------------------

DNA_KEYS: Tuple[str, ...] = (
    "brightness",
    "warmth",
    "movement",
    "density",
    "space",
    "aggression",
)

PLACEHOLDER_DNA: Dict[str, float] = {k: 0.5 for k in DNA_KEYS}

# Mood categories used by the arc scorer
ARC_OPENING  = {"Foundation"}
ARC_MIDDLE   = {"Entangled", "Prism", "Flux"}
ARC_CLOSING  = {"Atmosphere", "Aether"}
ALL_MOODS    = ARC_OPENING | ARC_MIDDLE | ARC_CLOSING | {"Family"}

MANIFEST_PATH = "bundle_manifest.json"

SEPARATOR = "=" * 72
THIN_SEP  = "-" * 72


# ---------------------------------------------------------------------------
# Data helpers
# ---------------------------------------------------------------------------

def _dna_vector(dna: Dict[str, float]) -> List[float]:
    """Return DNA as an ordered list matching DNA_KEYS."""
    return [float(dna.get(k, 0.5)) for k in DNA_KEYS]


def _euclidean(a: List[float], b: List[float]) -> float:
    """Euclidean distance between two equal-length vectors (range 0–√6 ≈ 2.45)."""
    return math.sqrt(sum((x - y) ** 2 for x, y in zip(a, b)))


def _energy(dna: Dict[str, float]) -> float:
    """Scalar energy proxy: warmth + aggression (range 0–2)."""
    return dna.get("warmth", 0.5) + dna.get("aggression", 0.5)


def _normalised_distance(a: List[float], b: List[float]) -> float:
    """Distance normalised to [0, 1] using max possible distance √6."""
    return _euclidean(a, b) / math.sqrt(len(a))


# ---------------------------------------------------------------------------
# Pack loading
# ---------------------------------------------------------------------------

def load_pack_meta(xpn_path: str) -> Dict:
    """
    Open an .xpn ZIP and extract Sonic DNA + mood from bundle_manifest.json.
    Returns a dict with keys: name, path, mood, dna, placeholder_dna.
    Warns to stderr and uses placeholder DNA if the manifest is absent/malformed.
    """
    name = os.path.basename(xpn_path)
    result = {
        "name": name,
        "path": xpn_path,
        "mood": "Foundation",
        "dna": dict(PLACEHOLDER_DNA),
        "placeholder_dna": False,
    }

    if not os.path.isfile(xpn_path):
        print(f"WARNING: Pack not found — {xpn_path}", file=sys.stderr)
        result["placeholder_dna"] = True
        return result

    try:
        with zipfile.ZipFile(xpn_path, "r") as zf:
            if MANIFEST_PATH not in zf.namelist():
                print(
                    f"WARNING: No bundle_manifest.json in {name} — using placeholder DNA",
                    file=sys.stderr,
                )
                result["placeholder_dna"] = True
                return result

            raw = zf.read(MANIFEST_PATH).decode("utf-8", errors="replace")
            manifest = json.loads(raw)

    except zipfile.BadZipFile:
        print(f"WARNING: Bad ZIP file — {name} — using placeholder DNA", file=sys.stderr)
        result["placeholder_dna"] = True
        return result
    except json.JSONDecodeError as exc:
        print(
            f"WARNING: Malformed bundle_manifest.json in {name} ({exc}) — "
            "using placeholder DNA",
            file=sys.stderr,
        )
        result["placeholder_dna"] = True
        return result

    # Extract mood (accept top-level or nested under "metadata")
    mood_raw: str = (
        manifest.get("mood")
        or manifest.get("metadata", {}).get("mood")
        or "Foundation"
    )
    result["mood"] = mood_raw if mood_raw in ALL_MOODS else "Foundation"

    # Extract Sonic DNA
    dna_raw = (
        manifest.get("sonic_dna")
        or manifest.get("sonicDna")
        or manifest.get("dna")
        or {}
    )
    if not isinstance(dna_raw, dict):
        dna_raw = {}

    merged: Dict[str, float] = {}
    for k in DNA_KEYS:
        raw_val = dna_raw.get(k)
        try:
            merged[k] = max(0.0, min(1.0, float(raw_val))) if raw_val is not None else 0.5
        except (TypeError, ValueError):
            merged[k] = 0.5

    result["dna"] = merged

    if not dna_raw:
        print(
            f"WARNING: No Sonic DNA in manifest for {name} — using placeholder DNA",
            file=sys.stderr,
        )
        result["placeholder_dna"] = True

    return result


def scan_packs_dir(packs_dir: str) -> List[Dict]:
    """Return sorted list of pack meta dicts for every .xpn in packs_dir."""
    if not os.path.isdir(packs_dir):
        print(f"ERROR: --packs-dir not found: {packs_dir}", file=sys.stderr)
        sys.exit(1)

    xpn_files = sorted(
        os.path.join(packs_dir, f)
        for f in os.listdir(packs_dir)
        if f.lower().endswith(".xpn")
    )

    if not xpn_files:
        print(f"ERROR: No .xpn files found in {packs_dir}", file=sys.stderr)
        sys.exit(1)

    return [load_pack_meta(p) for p in xpn_files]


# ---------------------------------------------------------------------------
# Scoring
# ---------------------------------------------------------------------------

def arc_score_for_slot(mood: str, slot: int, total_slots: int) -> float:
    """
    Return 0.0–1.0 indicating how well `mood` fits position `slot` (1-indexed)
    in a set of `total_slots` packs.
    """
    frac = (slot - 1) / max(total_slots - 1, 1)  # 0.0 = first, 1.0 = last

    if frac <= 0.25:      # Opening quarter
        return 1.0 if mood in ARC_OPENING else 0.3
    elif frac >= 0.75:    # Closing quarter
        return 1.0 if mood in ARC_CLOSING else 0.3
    else:                 # Middle half
        return 1.0 if mood in ARC_MIDDLE else 0.5


def energy_curve_score(
    ordered_packs: List[Dict],
    candidate: Dict,
    slot: int,
    total_slots: int,
) -> float:
    """
    Score based on whether the candidate's energy (warmth + aggression) follows
    an arc that builds to the centre and resolves toward the end.
    Expected energy at each slot follows a tent function peaking at the midpoint.
    """
    frac = (slot - 1) / max(total_slots - 1, 1)
    # Tent function: rises linearly to 0.5 then falls
    expected_energy = 1.0 - abs(frac - 0.5) * 2.0   # 0 at edges, 1 at centre

    actual_energy = _energy(candidate["dna"]) / 2.0  # normalise to [0, 1]
    diff = abs(actual_energy - expected_energy)
    return max(0.0, 1.0 - diff * 2.0)


# ---------------------------------------------------------------------------
# Greedy setlist construction
# ---------------------------------------------------------------------------

def build_setlist(
    pool: List[Dict],
    set_length: int,
    contrast_weight: float = 0.5,
    arc_weight: float = 0.3,
    energy_weight: float = 0.2,
) -> List[Dict]:
    """
    Greedy selection: pick `set_length` packs from `pool` that maximise the
    weighted composite score at each slot.

    Returns ordered list of slot dicts (pack meta + slot + contrast_to_prev).
    """
    if set_length >= len(pool):
        # Use all packs; still optimise ordering
        candidates = list(pool)
    else:
        candidates = list(pool)

    selected: List[Dict] = []
    remaining = list(candidates)

    for slot in range(1, set_length + 1):
        if not remaining:
            break

        best_pack: Optional[Dict] = None
        best_score = -1.0

        prev_vec = _dna_vector(selected[-1]["dna"]) if selected else None

        for pack in remaining:
            vec = _dna_vector(pack["dna"])

            # Contrast: normalised distance from previous pack (0 on first slot)
            if prev_vec is not None:
                contrast = _normalised_distance(prev_vec, vec)
            else:
                contrast = 0.5  # neutral for the opener

            arc    = arc_score_for_slot(pack["mood"], slot, set_length)
            energy = energy_curve_score(selected, pack, slot, set_length)

            score = (
                contrast_weight * contrast
                + arc_weight    * arc
                + energy_weight * energy
            )

            if score > best_score:
                best_score = best_pack_contrast = contrast
                best_score = score
                best_pack = pack
                best_pack_contrast = contrast

        if best_pack is None:
            break

        selected.append(best_pack)
        remaining.remove(best_pack)
        best_pack["_contrast_to_prev"] = best_pack_contrast  # type: ignore[assignment]

    return selected


# ---------------------------------------------------------------------------
# Output formatters
# ---------------------------------------------------------------------------

def compute_output(ordered: List[Dict]) -> Dict:
    """Build the final output dict from the ordered pack list."""
    slots = []
    total_contrast = 0.0
    arc_scores = []

    for i, pack in enumerate(ordered):
        slot_num = i + 1
        contrast = pack.get("_contrast_to_prev", 0.0)
        if slot_num > 1:
            total_contrast += contrast

        arc_s = arc_score_for_slot(pack["mood"], slot_num, len(ordered))
        arc_scores.append(arc_s)

        slots.append({
            "slot": slot_num,
            "pack": pack["name"],
            "mood": pack["mood"],
            "dna": {k: round(v, 4) for k, v in pack["dna"].items()},
            "contrast_to_prev": round(contrast, 4) if slot_num > 1 else None,
            "placeholder_dna": pack.get("placeholder_dna", False),
        })

    avg_arc = sum(arc_scores) / len(arc_scores) if arc_scores else 0.0

    return {
        "generated": datetime.now(timezone.utc).isoformat(),
        "set_length": len(ordered),
        "packs": slots,
        "arc_score": round(avg_arc, 4),
        "total_contrast": round(total_contrast, 4),
    }


def format_text(output: Dict) -> str:
    """Render the setlist as a human-readable numbered list."""
    lines = [
        SEPARATOR,
        "  XPN SETLIST — XO_OX DESIGNS",
        f"  Generated : {output['generated']}",
        f"  Set length: {output['set_length']}  |  "
        f"Arc score: {output['arc_score']:.2f}  |  "
        f"Total contrast: {output['total_contrast']:.2f}",
        SEPARATOR,
        "",
    ]

    for entry in output["packs"]:
        dna = entry["dna"]
        dna_str = "  ".join(f"{k[:3].upper()}: {v:.2f}" for k, v in dna.items())
        contrast_str = (
            f"  Δ contrast: {entry['contrast_to_prev']:.2f}"
            if entry["contrast_to_prev"] is not None
            else ""
        )
        placeholder_note = "  [placeholder DNA]" if entry.get("placeholder_dna") else ""

        lines.append(f"  [{entry['slot']:02d}]  {entry['pack']}")
        lines.append(f"        Mood: {entry['mood']}{contrast_str}{placeholder_note}")
        lines.append(f"        DNA: {dna_str}")
        lines.append("")

    lines += [
        THIN_SEP,
        f"  Arc score     : {output['arc_score']:.3f}  "
        "(1.0 = perfect opening→middle→closing arc)",
        f"  Total contrast: {output['total_contrast']:.3f}  "
        "(higher = more varied DNA across set)",
        SEPARATOR,
    ]
    return "\n".join(lines)


# ---------------------------------------------------------------------------
# CLI entry point
# ---------------------------------------------------------------------------

def main() -> int:
    parser = argparse.ArgumentParser(
        description="XPN Setlist Builder — performance order optimised by Sonic DNA"
    )
    parser.add_argument(
        "--packs-dir",
        required=True,
        metavar="DIR",
        help="Directory containing .xpn files",
    )
    parser.add_argument(
        "--set-length",
        type=int,
        default=0,
        metavar="N",
        help="Number of packs to include (default: all available)",
    )
    parser.add_argument(
        "--output",
        default=None,
        metavar="PATH",
        help="File path to write the setlist JSON (optional)",
    )
    parser.add_argument(
        "--format",
        choices=["text", "json", "both"],
        default="both",
        help="Output format (default: both)",
    )
    parser.add_argument(
        "--contrast-weight",
        type=float,
        default=0.5,
        metavar="W",
        help="Weight for contrast scoring 0–1 (default: 0.5)",
    )
    parser.add_argument(
        "--arc-weight",
        type=float,
        default=0.3,
        metavar="W",
        help="Weight for arc/mood scoring 0–1 (default: 0.3)",
    )
    parser.add_argument(
        "--energy-weight",
        type=float,
        default=0.2,
        metavar="W",
        help="Weight for energy-curve scoring 0–1 (default: 0.2)",
    )
    args = parser.parse_args()

    # Validate weights
    total_weight = args.contrast_weight + args.arc_weight + args.energy_weight
    if not math.isclose(total_weight, 1.0, abs_tol=0.01):
        print(
            f"WARNING: Weights sum to {total_weight:.3f} (expected 1.0). "
            "Results may be unexpected.",
            file=sys.stderr,
        )

    # Load packs
    packs_dir = os.path.abspath(args.packs_dir)
    pool = scan_packs_dir(packs_dir)
    print(f"Found {len(pool)} pack(s) in {packs_dir}")

    set_length = args.set_length if args.set_length > 0 else len(pool)
    set_length = min(set_length, len(pool))

    if set_length == 0:
        print("ERROR: No packs to sequence.", file=sys.stderr)
        return 1

    print(f"Building setlist: {set_length} slot(s) …")

    ordered = build_setlist(
        pool,
        set_length,
        contrast_weight=args.contrast_weight,
        arc_weight=args.arc_weight,
        energy_weight=args.energy_weight,
    )

    output = compute_output(ordered)

    # Write JSON
    if args.output and args.format in ("json", "both"):
        out_path = os.path.abspath(args.output)
        with open(out_path, "w", encoding="utf-8") as fh:
            json.dump(output, fh, indent=2)
            fh.write("\n")
        print(f"Setlist JSON written: {out_path}")

    # Print text
    if args.format in ("text", "both"):
        print()
        print(format_text(output))
    elif args.format == "json" and not args.output:
        # json-only with no --output: print to stdout
        print(json.dumps(output, indent=2))

    return 0


if __name__ == "__main__":
    sys.exit(main())
