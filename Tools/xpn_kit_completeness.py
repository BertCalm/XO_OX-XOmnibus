#!/usr/bin/env python3
"""
xpn_kit_completeness.py — Score a drum kit's completeness by analyzing its instrument slots.

Parses an MPC .xpm file, locates sample paths, classifies each sample via
xpn_classify_instrument.py, then scores the kit against an archetype.

CLI:
    python xpn_kit_completeness.py --xpm kit.xpm --stems ./stems/ [--archetype full|minimal|percussion]

Archetypes:
    full       (default): required=kick,snare,hihat,cymbal; optional=bass,perc,fx
                          Score 100 if all required present + 2+ optional
    minimal:              required=kick,snare; optional=hihat,cymbal,perc
                          Score 100 if required present
    percussion:           required=perc (4+ distinct slots); optional=fx
                          Score based on variety

Output: overall score 0-100, per-slot breakdown, missing elements, suggestions.
Also flags: duplicate samples on adjacent pads, low dynamic range across slots.

Dependency: xpn_classify_instrument.py (must be in same directory or on PYTHONPATH)
"""

import sys
import os
import argparse
import json
import math
import struct
import wave
import xml.etree.ElementTree as ET
from pathlib import Path

# ---------------------------------------------------------------------------
# Import classify_wav from sibling module
# ---------------------------------------------------------------------------

_TOOLS_DIR = Path(__file__).parent.resolve()
if str(_TOOLS_DIR) not in sys.path:
    sys.path.insert(0, str(_TOOLS_DIR))

try:
    from xpn_classify_instrument import classify_wav
except ImportError as e:
    print(f"ERROR: Cannot import xpn_classify_instrument.py — {e}", file=sys.stderr)
    print("Make sure xpn_classify_instrument.py is in the same directory.", file=sys.stderr)
    sys.exit(1)


# ---------------------------------------------------------------------------
# Archetype definitions
# ---------------------------------------------------------------------------

ARCHETYPES = {
    "full": {
        "required": ["kick", "snare", "hihat", "cymbal"],
        "optional": ["bass", "perc", "fx"],
        "score_100_optional_min": 2,
        "description": "Full drum kit (kick + snare + hihat + cymbal + 2 optional)",
    },
    "minimal": {
        "required": ["kick", "snare"],
        "optional": ["hihat", "cymbal", "perc"],
        "score_100_optional_min": 0,
        "description": "Minimal drum kit (kick + snare required)",
    },
    "percussion": {
        "required": ["perc"],
        "optional": ["fx"],
        "score_100_optional_min": 0,
        "perc_min_slots": 4,
        "description": "Percussion-focused kit (4+ distinct perc slots required)",
    },
}

# Suggestions for missing elements
SUGGESTIONS = {
    "kick":   "Add a kick drum for foundational low-end punch.",
    "snare":  "Add a snare for the backbeat — the rhythmic spine of the kit.",
    "hihat":  "Add a closed hihat for rhythmic subdivision and groove.",
    "cymbal": "Add an open hihat or cymbal for full rhythmic vocabulary.",
    "bass":   "Add a bass stab or 808 for melodic low-end movement.",
    "perc":   "Add a percussion element (conga, rim, shaker) for texture.",
    "fx":     "Add an FX sample (riser, impact, sweep) for arrangement transitions.",
}

# RMS similarity threshold — if all slots' RMS are within this range it's flagged
RMS_RANGE_THRESHOLD = 0.04   # max - min RMS difference before warning


# ---------------------------------------------------------------------------
# XPM parsing
# ---------------------------------------------------------------------------

def parse_xpm(xpm_path: str) -> list[dict]:
    """
    Parse an MPC .xpm file (XML) and return a list of pad slot dicts:
        { "pad": int, "layer": int, "sample_file": str }
    Only slots with non-empty SampleFile entries are returned.
    """
    tree = ET.parse(xpm_path)
    root = tree.getroot()

    slots = []
    pad_index = 0

    # XPM structure: <MPCVObject> → <ProgramList> → <Program> → <PadList> → <Pad> → <Layers> → <Layer>
    for pad in root.iter("Pad"):
        pad_index_attr = pad.get("Index", str(pad_index))
        try:
            pad_num = int(pad_index_attr)
        except ValueError:
            pad_num = pad_index
        pad_index += 1

        layer_index = 0
        for layer in pad.iter("Layer"):
            sample_file_el = layer.find("SampleFile")
            if sample_file_el is not None:
                sample_file = (sample_file_el.text or "").strip()
                if sample_file:
                    slots.append({
                        "pad": pad_num,
                        "layer": layer_index,
                        "sample_file": sample_file,
                    })
            layer_index += 1

    return slots


# ---------------------------------------------------------------------------
# Sample resolution — find actual WAV file on disk
# ---------------------------------------------------------------------------

def resolve_sample_path(sample_file: str, stems_dir: str, xpm_dir: str) -> str | None:
    """
    Try to locate the WAV file. Search order:
    1. Absolute path as-is
    2. Relative to stems_dir
    3. Relative to xpm_dir
    4. Filename only in stems_dir (basename match)
    Returns resolved path string or None if not found.
    """
    candidates = [
        sample_file,
        os.path.join(stems_dir, sample_file),
        os.path.join(xpm_dir, sample_file),
        os.path.join(stems_dir, os.path.basename(sample_file)),
        os.path.join(xpm_dir, os.path.basename(sample_file)),
    ]
    for candidate in candidates:
        if os.path.isfile(candidate):
            return candidate
    return None


# ---------------------------------------------------------------------------
# RMS computation (stdlib only — mirrors classify tool)
# ---------------------------------------------------------------------------

def _compute_rms_from_wav(wav_path: str) -> float:
    """Compute overall RMS of a WAV file (first 2 seconds max)."""
    try:
        with wave.open(wav_path, "rb") as wf:
            sr = wf.getframerate()
            sampwidth = wf.getsampwidth()
            n_channels = wf.getnchannels()
            max_frames = sr * 2   # first 2 seconds
            n_frames = min(wf.getnframes(), max_frames)
            raw = wf.readframes(n_frames)

        if sampwidth == 2:
            fmt = f"<{n_frames * n_channels}h"
            samples = list(struct.unpack(fmt, raw))
            max_val = 32768.0
        elif sampwidth == 1:
            samples = [b - 128 for b in raw]
            max_val = 128.0
        elif sampwidth == 4:
            fmt = f"<{n_frames * n_channels}i"
            samples = list(struct.unpack(fmt, raw))
            max_val = 2147483648.0
        else:
            return 0.0

        floats = [s / max_val for s in samples]
        if not floats:
            return 0.0
        return math.sqrt(sum(s * s for s in floats) / len(floats))
    except Exception:
        return 0.0


# ---------------------------------------------------------------------------
# Duplicate detection — compare sample filenames on adjacent pads
# ---------------------------------------------------------------------------

def find_adjacent_duplicates(slots_with_results: list[dict]) -> list[str]:
    """
    Flag slots where adjacent pad numbers reference the exact same sample filename.
    Returns list of warning strings.
    """
    warnings = []
    sorted_slots = sorted(slots_with_results, key=lambda s: (s["pad"], s["layer"]))

    for i in range(len(sorted_slots) - 1):
        a = sorted_slots[i]
        b = sorted_slots[i + 1]
        if (b["pad"] == a["pad"] + 1
                and os.path.basename(a["sample_file"]) == os.path.basename(b["sample_file"])):
            warnings.append(
                f"Pads {a['pad']} and {b['pad']} share the same sample: "
                f"{os.path.basename(a['sample_file'])}"
            )
    return warnings


# ---------------------------------------------------------------------------
# Scoring engine
# ---------------------------------------------------------------------------

def score_kit(slots_with_results: list[dict], archetype: str) -> dict:
    """
    Score the kit against the chosen archetype.

    slots_with_results: list of dicts, each with keys:
        pad, layer, sample_file, resolved_path (or None), classification (dict or None)

    Returns:
        {
            "score": int 0-100,
            "archetype": str,
            "archetype_description": str,
            "categories_found": list[str],
            "required_present": list[str],
            "required_missing": list[str],
            "optional_present": list[str],
            "optional_missing": list[str],
            "suggestions": list[str],
        }
    """
    arch = ARCHETYPES[archetype]

    # Collect all categories that appear in classified slots
    found_categories: set[str] = set()
    for slot in slots_with_results:
        cl = slot.get("classification")
        if cl and cl.get("category") not in (None, "unknown"):
            found_categories.add(cl["category"])

    required = arch["required"]
    optional = arch["optional"]

    required_present = [c for c in required if c in found_categories]
    required_missing = [c for c in required if c not in found_categories]
    optional_present = [c for c in optional if c in found_categories]
    optional_missing = [c for c in optional if c not in found_categories]

    # --- Scoring ---
    if archetype == "percussion":
        # Count distinct pad slots classified as "perc"
        perc_slots = [
            s for s in slots_with_results
            if s.get("classification") and s["classification"].get("category") == "perc"
        ]
        distinct_perc = len(set(s["pad"] for s in perc_slots))
        perc_min = arch.get("perc_min_slots", 4)

        if distinct_perc == 0:
            score = 0
        elif distinct_perc < perc_min:
            # Partial: scale up to 60 based on how close to perc_min
            score = int(60 * distinct_perc / perc_min)
        else:
            # Base 70 for meeting perc_min, +10 per optional category (up to 30)
            base = 70
            optional_bonus = min(30, len(optional_present) * 15)
            # Bonus for extra variety beyond perc_min
            variety_bonus = min(10, (distinct_perc - perc_min) * 2)
            score = min(100, base + optional_bonus + variety_bonus)

    else:
        # full / minimal
        opt_min = arch.get("score_100_optional_min", 0)

        if not required_missing:
            # All required present
            if len(optional_present) >= opt_min:
                score = 100
            else:
                # Lose up to 20 points for missing optional (below opt_min)
                missing_opt = opt_min - len(optional_present)
                score = max(80, 100 - (missing_opt * 10))
        else:
            # Missing some required — scale down from 80 based on ratio present
            present_ratio = len(required_present) / len(required)
            base = int(80 * present_ratio)
            # Add small optional bonus
            base += min(10, len(optional_present) * 3)
            score = min(79, base)   # Cap at 79 when required is incomplete

    # Suggestions
    suggestions = [SUGGESTIONS[c] for c in required_missing if c in SUGGESTIONS]
    if archetype != "percussion":
        for c in optional_missing:
            if c in SUGGESTIONS:
                suggestions.append(SUGGESTIONS[c])

    return {
        "score": max(0, min(100, score)),
        "archetype": archetype,
        "archetype_description": arch["description"],
        "categories_found": sorted(found_categories),
        "required_present": required_present,
        "required_missing": required_missing,
        "optional_present": optional_present,
        "optional_missing": optional_missing,
        "suggestions": suggestions,
    }


# ---------------------------------------------------------------------------
# Dynamic range check
# ---------------------------------------------------------------------------

def check_dynamic_range(slots_with_results: list[dict]) -> list[str]:
    """
    If all resolved slots have RMS within RMS_RANGE_THRESHOLD of each other,
    flag a low dynamic range warning.
    """
    rms_values = [s["rms"] for s in slots_with_results if s.get("rms") is not None]
    if len(rms_values) < 3:
        return []

    rms_min = min(rms_values)
    rms_max = max(rms_values)
    if rms_max - rms_min < RMS_RANGE_THRESHOLD:
        return [
            f"Low dynamic range detected: all slot RMS values are within "
            f"{(rms_max - rms_min):.4f} of each other "
            f"(min={rms_min:.4f}, max={rms_max:.4f}). "
            "Consider varying velocity layers or sample levels for a more expressive kit."
        ]
    return []


# ---------------------------------------------------------------------------
# Main analysis pipeline
# ---------------------------------------------------------------------------

def analyze_kit(xpm_path: str, stems_dir: str, archetype: str) -> dict:
    """
    Full pipeline: parse XPM → resolve samples → classify → score → report.
    Returns the complete analysis dict.
    """
    xpm_dir = str(Path(xpm_path).parent.resolve())
    stems_dir = str(Path(stems_dir).resolve()) if stems_dir else xpm_dir

    # 1. Parse XPM
    try:
        raw_slots = parse_xpm(xpm_path)
    except ET.ParseError as e:
        return {"error": f"Failed to parse XPM: {e}"}
    except FileNotFoundError:
        return {"error": f"XPM file not found: {xpm_path}"}

    if not raw_slots:
        return {"error": "No sample slots found in XPM file."}

    # 2. Resolve, classify, measure RMS for each slot
    slots_with_results = []
    for slot in raw_slots:
        resolved = resolve_sample_path(slot["sample_file"], stems_dir, xpm_dir)
        entry = dict(slot)
        entry["resolved_path"] = resolved

        if resolved:
            try:
                classification = classify_wav(resolved)
            except Exception as exc:
                classification = {
                    "category": "unknown",
                    "confidence": 0.0,
                    "error": str(exc),
                }
            entry["classification"] = classification
            entry["rms"] = _compute_rms_from_wav(resolved)
        else:
            entry["classification"] = None
            entry["rms"] = None

        slots_with_results.append(entry)

    # 3. Score
    score_result = score_kit(slots_with_results, archetype)

    # 4. Flags
    dup_warnings = find_adjacent_duplicates(slots_with_results)
    dyn_warnings = check_dynamic_range(slots_with_results)

    # 5. Per-slot breakdown (clean for output)
    slot_breakdown = []
    for s in slots_with_results:
        cl = s.get("classification") or {}
        slot_breakdown.append({
            "pad": s["pad"],
            "layer": s["layer"],
            "sample_file": os.path.basename(s["sample_file"]),
            "resolved": s["resolved_path"] is not None,
            "category": cl.get("category", "unresolved"),
            "confidence": cl.get("confidence", 0.0),
            "rms": round(s["rms"], 4) if s["rms"] is not None else None,
        })

    return {
        "xpm": os.path.basename(xpm_path),
        "archetype": archetype,
        "overall_score": score_result["score"],
        "archetype_description": score_result["archetype_description"],
        "categories_found": score_result["categories_found"],
        "required_present": score_result["required_present"],
        "required_missing": score_result["required_missing"],
        "optional_present": score_result["optional_present"],
        "optional_missing": score_result["optional_missing"],
        "suggestions": score_result["suggestions"],
        "warnings": dup_warnings + dyn_warnings,
        "slot_breakdown": slot_breakdown,
        "total_slots": len(slots_with_results),
        "resolved_slots": sum(1 for s in slots_with_results if s["resolved_path"]),
    }


# ---------------------------------------------------------------------------
# Pretty-print output
# ---------------------------------------------------------------------------

def print_report(result: dict) -> None:
    """Print a human-readable completeness report to stdout."""
    if "error" in result:
        print(f"ERROR: {result['error']}")
        return

    bar_filled = int(result["overall_score"] / 5)
    bar = "█" * bar_filled + "░" * (20 - bar_filled)

    print()
    print(f"  Kit Completeness Report — {result['xpm']}")
    print(f"  Archetype : {result['archetype']} — {result['archetype_description']}")
    print(f"  Score     : {result['overall_score']:3d}/100  [{bar}]")
    print(f"  Slots     : {result['resolved_slots']}/{result['total_slots']} resolved")
    print()

    print("  Categories found  :", ", ".join(result["categories_found"]) or "(none)")
    if result["required_present"]:
        print("  Required (OK)     :", ", ".join(result["required_present"]))
    if result["required_missing"]:
        print("  Required (MISSING):", ", ".join(result["required_missing"]))
    if result["optional_present"]:
        print("  Optional (found)  :", ", ".join(result["optional_present"]))
    if result["optional_missing"]:
        print("  Optional (absent) :", ", ".join(result["optional_missing"]))

    if result["suggestions"]:
        print()
        print("  Suggestions:")
        for s in result["suggestions"]:
            print(f"    • {s}")

    if result["warnings"]:
        print()
        print("  Flags:")
        for w in result["warnings"]:
            print(f"    ⚠ {w}")

    print()
    print("  Per-Slot Breakdown:")
    print(f"  {'Pad':>4}  {'Lyr':>3}  {'Category':<10}  {'Conf':>5}  {'RMS':>7}  Sample")
    print("  " + "-" * 65)
    for slot in result["slot_breakdown"]:
        rms_str = f"{slot['rms']:.4f}" if slot["rms"] is not None else "  N/A "
        resolved_mark = "" if slot["resolved"] else " [NOT FOUND]"
        print(
            f"  {slot['pad']:>4}  {slot['layer']:>3}  "
            f"{slot['category']:<10}  {slot['confidence']:>5.2f}  "
            f"{rms_str:>7}  {slot['sample_file']}{resolved_mark}"
        )
    print()


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def main():
    parser = argparse.ArgumentParser(
        description="Score a drum kit's completeness from an MPC .xpm file (stdlib only)"
    )
    parser.add_argument("--xpm", required=True, help="Path to .xpm kit file")
    parser.add_argument(
        "--stems", default="",
        help="Directory containing WAV samples (default: same directory as .xpm)"
    )
    parser.add_argument(
        "--archetype", choices=["full", "minimal", "percussion"], default="full",
        help="Scoring archetype (default: full)"
    )
    parser.add_argument(
        "--json", action="store_true",
        help="Output raw JSON instead of formatted report"
    )
    args = parser.parse_args()

    stems_dir = args.stems if args.stems else str(Path(args.xpm).parent)
    result = analyze_kit(args.xpm, stems_dir, args.archetype)

    if args.json:
        print(json.dumps(result, indent=2))
    else:
        print_report(result)


if __name__ == "__main__":
    main()
