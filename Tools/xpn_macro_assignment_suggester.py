#!/usr/bin/env python3
"""
xpn_macro_assignment_suggester.py — Suggest M1-M4 macro assignments for .xometa presets.

XO_OX Macro Philosophy:
  M1 = CHARACTER  — primary timbral character (filter, shape, oscillator)
  M2 = MOVEMENT   — modulation and life (LFO rate, envelope times, vibrato)
  M3 = COUPLING   — coupling intensity or cross-engine interaction
  M4 = SPACE      — reverb, delay, room, stereo width

Notes on .xometa format:
  - Two preset formats exist in the fleet:
      (a) Flat parameters: {"param_id": float, ...}
      (b) Nested parameters: {"EngineName": {"param_id": float, ...}}
  - The "macros" field stores label strings, e.g. {"M1": "PROWL", "M2": "FOLIAGE"}
    These labels correspond to parameter names in the flat/nested params.
  - Some presets use "macroLabels" instead of or alongside "macros".

Usage:
    python xpn_macro_assignment_suggester.py <presets_dir_or_file>
    python xpn_macro_assignment_suggester.py <presets_dir_or_file> --engine OPAL
    python xpn_macro_assignment_suggester.py <presets_dir_or_file> --format json
    python xpn_macro_assignment_suggester.py <presets_dir_or_file> --fleet
"""

import argparse
import glob
import json
import re
import sys
from pathlib import Path
from typing import Dict, List, Optional, Tuple

# ---------------------------------------------------------------------------
# Macro role definitions
# ---------------------------------------------------------------------------

MACRO_ROLES = {
    "M1": {
        "name": "CHARACTER",
        "keywords": [
            "cutoff", "filter", "tone", "brightness", "harmonics",
            "shape", "wave", "osc", "timbre", "color", "tilt",
            "bite", "density", "texture", "morph", "fold", "bend",
            "pluck", "bow", "breath", "reed", "string", "material",
            "grain", "spectral", "partial", "formant", "vowel",
        ],
        "boost_exact": ["cutoff", "filterCutoff", "oscShape", "waveform", "grainSize"],
    },
    "M2": {
        "name": "MOVEMENT",
        "keywords": [
            "lfo", "rate", "speed", "flutter", "vibrato", "modDepth",
            "envelope", "attack", "decay", "release", "sustain",
            "tremolo", "wobble", "drift", "pulse", "cycle", "tempo",
            "glide", "portamento", "ramp", "swell", "breathe", "shimmer",
            "creature", "chop", "swing",
        ],
        "boost_exact": ["lfoRate", "lfoDepth", "modRate", "attack", "decay", "driftRate"],
    },
    "M3": {
        "name": "COUPLING",
        "keywords": [
            "coupling", "cross", "link", "send", "couplingIntensity",
            "entangle", "bond", "sync", "interact", "merge", "blend",
            "crosstalk", "exchange", "transfer", "mutual", "couplingLevel",
        ],
        "boost_exact": ["couplingIntensity", "sendAmount", "crossMod", "couplingLevel"],
    },
    "M4": {
        "name": "SPACE",
        "keywords": [
            "reverb", "delay", "room", "hall", "space", "width",
            "spread", "stereo", "ambience", "tail",
            "shimmer", "plate", "spring", "chamber", "echo",
            "diffuse", "size", "predelay", "reverbMix", "delayMix",
        ],
        "boost_exact": ["reverbMix", "delayMix", "roomSize", "stereoWidth", "spread"],
    },
}

# ---------------------------------------------------------------------------
# Scoring helpers
# ---------------------------------------------------------------------------

def _lower_tokens(param_id: str) -> List[str]:
    """Split camelCase / snake_case param ID into lowercase tokens."""
    # Strip engine prefix (everything up to and including first underscore)
    without_prefix = re.sub(r'^[a-z]+_', '', param_id)
    # Split camelCase into words
    tokens = re.sub(r'([A-Z])', r' \1', without_prefix).lower().split()
    # Add whole string for substring matching
    tokens.append(without_prefix.lower())
    return tokens


def score_param_for_role(param_id: str, role_key: str) -> float:
    """Return 0.0–1.0 score for how well param_id fits the macro role."""
    role = MACRO_ROLES[role_key]
    tokens = _lower_tokens(param_id)
    param_lower = param_id.lower()
    score = 0.0

    # Exact boost (highest priority): suffix of param matches known key params
    suffix = param_id.split("_", 1)[-1].lower() if "_" in param_id else param_lower
    for exact in role["boost_exact"]:
        if exact.lower() == suffix or exact.lower() in param_lower:
            score = max(score, 1.0)

    # Keyword matching
    for kw in role["keywords"]:
        kw_l = kw.lower()
        for tok in tokens:
            if kw_l == tok:
                score = max(score, 0.8)
            elif kw_l in tok or (len(kw_l) > 3 and tok in kw_l):
                score = max(score, 0.5)
        if kw_l in param_lower:
            score = max(score, 0.7)

    return round(score, 2)


# ---------------------------------------------------------------------------
# Parameter helpers
# ---------------------------------------------------------------------------

def _flatten_param_ids(params: dict) -> List[str]:
    """
    Handle both flat {param_id: value} and nested {EngineName: {param_id: value}} formats.
    Returns a flat list of all param IDs.
    """
    ids = []
    for key, value in params.items():
        if isinstance(value, dict):
            ids.extend(value.keys())
        else:
            ids.append(key)
    return ids


def suggest_for_params(param_ids: List[str]) -> Dict[str, List[Tuple[str, float]]]:
    """Return top-2 (param_id, score) candidates per macro role."""
    suggestions = {}
    for role_key in MACRO_ROLES:
        scored = [(pid, score_param_for_role(pid, role_key)) for pid in param_ids]
        scored = [(pid, s) for pid, s in scored if s > 0]
        scored.sort(key=lambda x: -x[1])
        suggestions[role_key] = scored[:2]
    return suggestions


# ---------------------------------------------------------------------------
# .xometa parsing
# ---------------------------------------------------------------------------

def load_xometa(path: str) -> dict:
    with open(path, "r", encoding="utf-8") as f:
        return json.load(f)


def get_macro_labels(data: dict) -> Dict[str, Optional[str]]:
    """
    Returns {M1: label_or_None, ...}.

    In the fleet, "macros" contains {M1: "LABEL_STRING"} — a human label.
    "macroLabels" is a list of 4 label strings.
    We normalize both into the same dict.
    """
    result = {"M1": None, "M2": None, "M3": None, "M4": None}

    macros = data.get("macros")
    if isinstance(macros, dict):
        for key in result:
            val = macros.get(key)
            if val and isinstance(val, str):
                result[key] = val

    # macroLabels list fallback (only fill slots still None)
    macro_labels_list = data.get("macroLabels")
    if isinstance(macro_labels_list, list):
        for i, label in enumerate(macro_labels_list[:4]):
            key = f"M{i+1}"
            if not result[key] and label:
                result[key] = label

    return result


def label_to_param(label: Optional[str], param_ids: List[str]) -> Optional[str]:
    """
    Try to find the param_id that corresponds to a macro label string.
    E.g. label="PROWL" → "ocelot_prowl"
    Matching is case-insensitive suffix match.
    """
    if not label:
        return None
    label_l = label.lower()
    for pid in param_ids:
        suffix = pid.split("_", 1)[-1].lower() if "_" in pid else pid.lower()
        if suffix == label_l:
            return pid
    return None


# ---------------------------------------------------------------------------
# Alignment check
# ---------------------------------------------------------------------------

def compute_alignment(
    current_labels: Dict[str, Optional[str]],
    suggestions: Dict[str, List[Tuple[str, float]]],
    param_ids: List[str],
) -> Dict[str, bool]:
    """
    True if the current macro's resolved param matches the top suggestion.
    """
    alignment = {}
    for role_key in MACRO_ROLES:
        label = current_labels.get(role_key)
        resolved = label_to_param(label, param_ids)
        top = suggestions.get(role_key, [])
        top_param = top[0][0] if top else None
        alignment[role_key] = bool(resolved and top_param and resolved == top_param)
    return alignment


# ---------------------------------------------------------------------------
# Report formatting
# ---------------------------------------------------------------------------

def format_text_report(
    path: str,
    data: dict,
    suggestions: Dict[str, List[Tuple[str, float]]],
    current_labels: Dict[str, Optional[str]],
    param_ids: List[str],
) -> str:
    preset_name = data.get("name", Path(path).stem)
    engine_list = data.get("engines", [data.get("engine", "")])
    if isinstance(engine_list, str):
        engine_list = [engine_list]
    engine_tag = "/".join(e for e in engine_list if e) or Path(path).parts[-2]

    lines = [
        f"MACRO SUGGESTIONS \u2014 {engine_tag}/{preset_name}",
        "",
        "Existing macros:",
    ]

    for role_key, role_info in MACRO_ROLES.items():
        label = current_labels.get(role_key)
        resolved = label_to_param(label, param_ids)
        top = suggestions.get(role_key, [])
        top_param = top[0][0] if top else None

        if label:
            display = label
            if resolved:
                display += f" \u2192 {resolved}"
                if top_param and resolved == top_param:
                    display += "  \u2713 matches suggestion"
                else:
                    display += "  [diverges from suggestion]"
            else:
                display += "  (no matching param found)"
        else:
            display = "(empty)"

        slot = f"  {role_key} ({role_info['name']}):"
        lines.append(f"{slot:<24} {display}")

    lines.append("")
    lines.append("Suggestions:")

    for role_key, role_info in MACRO_ROLES.items():
        top = suggestions.get(role_key, [])
        label = current_labels.get(role_key)

        if not top:
            lines.append(f"  {role_key} {role_info['name']:<10} \u2192 (no candidates found)")
            continue

        parts = ", ".join(f"{p} (score {s:.1f})" for p, s in top)
        suffix = "  \u2014 currently empty!" if not label else ""
        lines.append(f"  {role_key} {role_info['name']:<10} \u2192 {parts}{suffix}")

    return "\n".join(lines)


def format_json_report(
    path: str,
    data: dict,
    suggestions: Dict[str, List[Tuple[str, float]]],
    current_labels: Dict[str, Optional[str]],
    param_ids: List[str],
) -> dict:
    preset_name = data.get("name", Path(path).stem)
    engine_list = data.get("engines", [data.get("engine", "")])
    if isinstance(engine_list, str):
        engine_list = [engine_list]
    alignment = compute_alignment(current_labels, suggestions, param_ids)
    return {
        "file": path,
        "preset": preset_name,
        "engines": [e for e in engine_list if e],
        "current_macro_labels": current_labels,
        "current_macro_params": {
            role_key: label_to_param(current_labels.get(role_key), param_ids)
            for role_key in MACRO_ROLES
        },
        "suggestions": {
            role_key: [{"param": p, "score": s} for p, s in candidates]
            for role_key, candidates in suggestions.items()
        },
        "alignment": alignment,
    }


# ---------------------------------------------------------------------------
# Fleet statistics
# ---------------------------------------------------------------------------

def print_fleet_summary(results: List[dict]) -> None:
    total = len(results)
    if total == 0:
        print("No presets analyzed.")
        return

    role_matches = {k: 0 for k in MACRO_ROLES}
    empty_counts = {k: 0 for k in MACRO_ROLES}

    for r in results:
        for role_key in MACRO_ROLES:
            if r["alignment"][role_key]:
                role_matches[role_key] += 1
            if not r["current_macro_labels"].get(role_key):
                empty_counts[role_key] += 1

    print(f"\nFLEET SUMMARY ({total} presets)")
    print("-" * 50)
    for role_key, role_info in MACRO_ROLES.items():
        pct_match = role_matches[role_key] / total * 100
        pct_empty = empty_counts[role_key] / total * 100
        print(
            f"  {role_key} {role_info['name']:<10}  "
            f"match {role_matches[role_key]:>5}/{total}  ({pct_match:5.1f}%)  "
            f"empty {empty_counts[role_key]:>5}/{total}  ({pct_empty:5.1f}%)"
        )


def _fleet_dict(results: List[dict]) -> dict:
    total = len(results)
    role_matches = {k: 0 for k in MACRO_ROLES}
    empty_counts = {k: 0 for k in MACRO_ROLES}
    for r in results:
        for role_key in MACRO_ROLES:
            if r["alignment"][role_key]:
                role_matches[role_key] += 1
            if not r["current_macro_labels"].get(role_key):
                empty_counts[role_key] += 1
    return {
        "total_presets": total,
        "by_macro": {
            role_key: {
                "name": MACRO_ROLES[role_key]["name"],
                "matching": role_matches[role_key],
                "empty": empty_counts[role_key],
                "match_pct": round(role_matches[role_key] / total * 100, 1) if total else 0,
                "empty_pct": round(empty_counts[role_key] / total * 100, 1) if total else 0,
            }
            for role_key in MACRO_ROLES
        },
    }


# ---------------------------------------------------------------------------
# File discovery
# ---------------------------------------------------------------------------

def collect_files(target: str, engine_filter: Optional[str]) -> List[str]:
    p = Path(target)
    if p.is_file():
        return [str(p)]

    files = glob.glob(str(p / "**" / "*.xometa"), recursive=True)

    if engine_filter:
        ef = engine_filter.upper()
        filtered = []
        for f in files:
            # Cheap path check first
            if ef in [part.upper() for part in Path(f).parts]:
                filtered.append(f)
                continue
            try:
                d = load_xometa(f)
                engines = d.get("engines", d.get("engine", []))
                if isinstance(engines, str):
                    engines = [engines]
                if ef in [e.upper() for e in engines]:
                    filtered.append(f)
            except Exception as exc:
                print(f"[WARN] Loading preset metadata from {f.name} for engine filter: {exc}", file=sys.stderr)
        files = filtered

    return sorted(files)


# ---------------------------------------------------------------------------
# Entry point
# ---------------------------------------------------------------------------

def main() -> None:
    parser = argparse.ArgumentParser(
        description="Suggest M1-M4 macro assignments for .xometa presets."
    )
    parser.add_argument("target", help="Path to a .xometa file or directory of presets")
    parser.add_argument("--engine", metavar="ENGINE",
                        help="Filter by engine name (e.g. OPAL)")
    parser.add_argument("--format", choices=["text", "json"], default="text",
                        help="Output format (default: text)")
    parser.add_argument("--fleet", action="store_true",
                        help="Show fleet-wide summary statistics after individual reports")
    args = parser.parse_args()

    files = collect_files(args.target, args.engine)
    if not files:
        print(f"No .xometa files found at: {args.target}", file=sys.stderr)
        sys.exit(1)

    json_results = []
    first = True

    for filepath in files:
        try:
            data = load_xometa(filepath)
        except Exception as e:
            print(f"ERROR reading {filepath}: {e}", file=sys.stderr)
            continue

        params = data.get("parameters", {})
        param_ids = _flatten_param_ids(params)
        suggestions = suggest_for_params(param_ids)
        current_labels = get_macro_labels(data)
        report = format_json_report(filepath, data, suggestions, current_labels, param_ids)
        json_results.append(report)

        if args.format == "text":
            if not first:
                print("\n" + "=" * 60 + "\n")
            print(format_text_report(filepath, data, suggestions, current_labels, param_ids))
            first = False

    if args.format == "json":
        if args.fleet:
            output = {"presets": json_results, "fleet_summary": _fleet_dict(json_results)}
        else:
            output = json_results if len(json_results) > 1 else (json_results[0] if json_results else {})
        print(json.dumps(output, indent=2))

    if args.fleet:
        print_fleet_summary(json_results)


if __name__ == "__main__":
    main()
