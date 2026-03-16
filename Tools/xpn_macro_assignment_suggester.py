#!/usr/bin/env python3
"""
xpn_macro_assignment_suggester.py — Suggest M1-M4 macro assignments for .xometa presets.

XO_OX Macro Philosophy:
  M1 = CHARACTER  — primary timbral character (filter, shape, oscillator)
  M2 = MOVEMENT   — modulation and life (LFO rate, envelope times, vibrato)
  M3 = COUPLING   — coupling intensity or cross-engine interaction
  M4 = SPACE      — reverb, delay, room, stereo width

Usage:
    python xpn_macro_assignment_suggester.py <presets_dir_or_file>
    python xpn_macro_assignment_suggester.py <presets_dir_or_file> --engine OPAL
    python xpn_macro_assignment_suggester.py <presets_dir_or_file> --format json
    python xpn_macro_assignment_suggester.py <presets_dir_or_file> --fleet
"""

import argparse
import json
import os
import glob
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
            "pluck", "bow", "breath", "reed", "string",
        ],
        "boost_exact": ["cutoff", "filterCutoff", "oscShape", "waveform"],
    },
    "M2": {
        "name": "MOVEMENT",
        "keywords": [
            "lfo", "rate", "speed", "flutter", "vibrato", "modDepth",
            "envelope", "attack", "decay", "release", "sustain",
            "tremolo", "wobble", "drift", "pulse", "cycle", "tempo",
            "glide", "portamento", "ramp", "swell",
        ],
        "boost_exact": ["lfoRate", "lfoDepth", "modRate", "attack", "decay"],
    },
    "M3": {
        "name": "COUPLING",
        "keywords": [
            "coupling", "cross", "link", "send", "couplingIntensity",
            "entangle", "bond", "sync", "interact", "merge", "blend",
            "crosstalk", "exchange", "transfer", "mutual",
        ],
        "boost_exact": ["couplingIntensity", "sendAmount", "crossMod"],
    },
    "M4": {
        "name": "SPACE",
        "keywords": [
            "reverb", "delay", "room", "hall", "space", "width",
            "spread", "stereo", "ir", "ambience", "tail", "decay",
            "shimmer", "plate", "spring", "chamber", "echo",
            "diffuse", "size", "predelay",
        ],
        "boost_exact": ["reverbMix", "delayMix", "roomSize", "stereoWidth", "spread"],
    },
}

# ---------------------------------------------------------------------------
# Scoring
# ---------------------------------------------------------------------------

def _lower_tokens(param_id: str) -> list[str]:
    """Split camelCase / snake_case param ID into lowercase tokens."""
    import re
    # Remove engine prefix (up to first underscore)
    without_prefix = re.sub(r'^[a-z]+_', '', param_id)
    # Split camelCase
    tokens = re.sub(r'([A-Z])', r' \1', without_prefix).lower().split()
    # Also add the full lowercased string for substring matching
    tokens.append(without_prefix.lower())
    return tokens


def score_param_for_role(param_id: str, role_key: str) -> float:
    """Return a 0.0–1.0 score for how well param_id fits the macro role."""
    role = MACRO_ROLES[role_key]
    tokens = _lower_tokens(param_id)
    param_lower = param_id.lower()

    score = 0.0

    # Exact boost (highest priority)
    for exact in role["boost_exact"]:
        if exact.lower() in param_id or exact.lower() == param_id.split("_", 1)[-1].lower():
            score = max(score, 1.0)

    # Keyword substring match
    for kw in role["keywords"]:
        kw_l = kw.lower()
        if kw_l in param_lower:
            score = max(score, 0.7)
        for tok in tokens:
            if kw_l == tok:
                score = max(score, 0.8)
            elif kw_l in tok or tok in kw_l:
                score = max(score, 0.5)

    return round(score, 2)


def _flatten_param_ids(params: dict) -> List[str]:
    """
    Handle both flat {param_id: value} and nested {engine_name: {param_id: value}} formats.
    Returns a flat list of param IDs.
    """
    ids = []
    for key, value in params.items():
        if isinstance(value, dict):
            # Nested format: key is engine name, value is {param_id: float}
            ids.extend(value.keys())
        else:
            # Flat format: key is param_id
            ids.append(key)
    return ids


def suggest_for_params(param_ids: List[str]) -> Dict[str, List[Tuple[str, float]]]:
    """
    For each macro role, return top 2 (param_id, score) candidates (score > 0).
    """
    suggestions = {}
    for role_key in MACRO_ROLES:
        scored = []
        for pid in param_ids:
            s = score_param_for_role(pid, role_key)
            if s > 0:
                scored.append((pid, s))
        scored.sort(key=lambda x: -x[1])
        suggestions[role_key] = scored[:2]
    return suggestions

# ---------------------------------------------------------------------------
# .xometa parsing
# ---------------------------------------------------------------------------

def load_xometa(path: str) -> dict:
    with open(path, "r", encoding="utf-8") as f:
        return json.load(f)


def get_macro_assignments(data: dict) -> Dict[str, Optional[str]]:
    """
    Returns {M1: param_id_or_None, M2: ..., M3: ..., M4: ...}.
    Handles both list-of-objects and dict styles.
    """
    macros = data.get("macros", {})
    result = {"M1": None, "M2": None, "M3": None, "M4": None}

    if isinstance(macros, dict):
        for key in ["M1", "M2", "M3", "M4"]:
            val = macros.get(key)
            if isinstance(val, dict):
                result[key] = val.get("parameter") or val.get("param") or val.get("id")
            elif isinstance(val, str) and val:
                result[key] = val
    elif isinstance(macros, list):
        for i, item in enumerate(macros[:4]):
            key = f"M{i+1}"
            if isinstance(item, dict):
                result[key] = item.get("parameter") or item.get("param") or item.get("id")
            elif isinstance(item, str) and item:
                result[key] = item

    return result

# ---------------------------------------------------------------------------
# Report formatting
# ---------------------------------------------------------------------------

def format_text_report(path: str, data: dict, suggestions: dict, current: dict) -> str:
    preset_name = data.get("name", Path(path).stem)
    engine_list = data.get("engines", [])
    engine_tag = "/".join(engine_list) if engine_list else Path(path).parts[-2]

    lines = [
        f"MACRO SUGGESTIONS — {engine_tag}/{preset_name}",
        "",
        "Existing macros:",
    ]

    for role_key, role_info in MACRO_ROLES.items():
        assigned = current.get(role_key)
        top_suggestions = suggestions.get(role_key, [])
        top_param = top_suggestions[0][0] if top_suggestions else None

        match_mark = ""
        if assigned:
            if top_param and assigned == top_param:
                match_mark = "  \u2713 matches suggestion"
            else:
                match_mark = "  [currently unassigned to suggested param]"
        else:
            match_mark = "  (empty)"

        label = f"  {role_key} ({role_info['name']}):"
        value = assigned if assigned else "(empty)"
        lines.append(f"{label:<22} {value}{match_mark}")

    lines.append("")
    lines.append("Suggestions:")

    for role_key, role_info in MACRO_ROLES.items():
        top = suggestions.get(role_key, [])
        assigned = current.get(role_key)

        if not top:
            lines.append(f"  {role_key} {role_info['name']:<10} \u2192 (no candidates found)")
            continue

        parts = ", ".join(f"{p} (score {s:.1f})" for p, s in top)
        suffix = ""
        if not assigned:
            suffix = "  \u2014 currently empty!"
        lines.append(f"  {role_key} {role_info['name']:<10} \u2192 {parts}{suffix}")

    return "\n".join(lines)


def format_json_report(path: str, data: dict, suggestions: dict, current: dict) -> dict:
    preset_name = data.get("name", Path(path).stem)
    engine_list = data.get("engines", [])
    return {
        "file": path,
        "preset": preset_name,
        "engines": engine_list,
        "current_macros": current,
        "suggestions": {
            role_key: [{"param": p, "score": s} for p, s in candidates]
            for role_key, candidates in suggestions.items()
        },
        "alignment": {
            role_key: (
                current.get(role_key) == suggestions[role_key][0][0]
                if suggestions.get(role_key) and current.get(role_key)
                else False
            )
            for role_key in MACRO_ROLES
        },
    }

# ---------------------------------------------------------------------------
# Fleet statistics
# ---------------------------------------------------------------------------

def print_fleet_summary(results: list[dict]) -> None:
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
            if not r["current_macros"].get(role_key):
                empty_counts[role_key] += 1

    print(f"\nFLEET SUMMARY ({total} presets)")
    print("-" * 42)
    for role_key, role_info in MACRO_ROLES.items():
        pct_match = role_matches[role_key] / total * 100
        pct_empty = empty_counts[role_key] / total * 100
        print(
            f"  {role_key} {role_info['name']:<10}  "
            f"match {role_matches[role_key]:>4}/{total}  ({pct_match:5.1f}%)  "
            f"empty {empty_counts[role_key]:>4}/{total}  ({pct_empty:5.1f}%)"
        )

# ---------------------------------------------------------------------------
# File discovery
# ---------------------------------------------------------------------------

def collect_files(target: str, engine_filter: Optional[str]) -> List[str]:
    p = Path(target)
    if p.is_file():
        return [str(p)]

    pattern = str(p / "**" / "*.xometa")
    files = glob.glob(pattern, recursive=True)

    if engine_filter:
        ef = engine_filter.upper()
        filtered = []
        for f in files:
            parts = Path(f).parts
            # Check parent directory name or engines field (cheap path check first)
            if ef in [part.upper() for part in parts]:
                filtered.append(f)
                continue
            try:
                data = load_xometa(f)
                engines = [e.upper() for e in data.get("engines", [])]
                if ef in engines:
                    filtered.append(f)
            except Exception:
                pass
        files = filtered

    return sorted(files)

# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main() -> None:
    parser = argparse.ArgumentParser(
        description="Suggest M1-M4 macro assignments for .xometa presets."
    )
    parser.add_argument("target", help="Path to a .xometa file or directory of presets")
    parser.add_argument("--engine", metavar="ENGINE", help="Filter by engine name (e.g. OPAL)")
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
        param_ids = list(params.keys())

        suggestions = suggest_for_params(param_ids)
        current = get_macro_assignments(data)

        if args.format == "json":
            json_results.append(format_json_report(filepath, data, suggestions, current))
        else:
            if not first:
                print("\n" + "=" * 60 + "\n")
            print(format_text_report(filepath, data, suggestions, current))
            first = False

    if args.format == "json":
        if args.fleet:
            output = {"presets": json_results, "fleet_summary": _fleet_dict(json_results)}
        else:
            output = json_results if len(json_results) > 1 else (json_results[0] if json_results else {})
        print(json.dumps(output, indent=2))
    elif args.fleet:
        print_fleet_summary(json_results if args.format == "json" else _build_fleet_data(files, args))


def _build_fleet_data(files: List[str], args) -> List[dict]:
    """Re-parse files to build fleet data when in text mode + --fleet."""
    results = []
    for filepath in files:
        try:
            data = load_xometa(filepath)
            params = data.get("parameters", {})
            suggestions = suggest_for_params(list(params.keys()))
            current = get_macro_assignments(data)
            results.append(format_json_report(filepath, data, suggestions, current))
        except Exception:
            pass
    return results


def _fleet_dict(results: list[dict]) -> dict:
    total = len(results)
    role_matches = {k: 0 for k in MACRO_ROLES}
    empty_counts = {k: 0 for k in MACRO_ROLES}
    for r in results:
        for role_key in MACRO_ROLES:
            if r["alignment"][role_key]:
                role_matches[role_key] += 1
            if not r["current_macros"].get(role_key):
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


# Fix: re-route fleet printing in text mode through the right data path
def _patched_main() -> None:
    parser = argparse.ArgumentParser(
        description="Suggest M1-M4 macro assignments for .xometa presets."
    )
    parser.add_argument("target", help="Path to a .xometa file or directory of presets")
    parser.add_argument("--engine", metavar="ENGINE", help="Filter by engine name (e.g. OPAL)")
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
        current = get_macro_assignments(data)
        report_dict = format_json_report(filepath, data, suggestions, current)
        json_results.append(report_dict)

        if args.format == "text":
            if not first:
                print("\n" + "=" * 60 + "\n")
            print(format_text_report(filepath, data, suggestions, current))
            first = False

    if args.format == "json":
        if args.fleet:
            output = {"presets": json_results, "fleet_summary": _fleet_dict(json_results)}
        else:
            output = json_results if len(json_results) > 1 else (json_results[0] if json_results else {})
        print(json.dumps(output, indent=2))
    elif args.fleet:
        print_fleet_summary(json_results)


if __name__ == "__main__":
    _patched_main()
