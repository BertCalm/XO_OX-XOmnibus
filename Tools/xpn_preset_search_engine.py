#!/usr/bin/env python3
"""
XOlokun Preset Search Engine — Full-text search across .xometa preset library

Searches preset name, description, tags, engines, mood, macro labels,
and parameter names simultaneously with relevance scoring.

Usage:
    python xpn_preset_search_engine.py <presets_dir> <query> [options]

Options:
    --engine FILTER     Filter by engine name (case-insensitive substring)
    --mood FILTER       Filter by mood name (case-insensitive substring)
    --top N             Return top N results (default: 10)
    --format text|json  Output format (default: text)

Examples:
    python xpn_preset_search_engine.py Presets/XOlokun "warm bass"
    python xpn_preset_search_engine.py Presets/XOlokun "pad" --engine OPAL --top 20
    python xpn_preset_search_engine.py Presets/XOlokun "dark" --mood Atmosphere --format json
"""

import json
import glob
import argparse
import os
import sys

# ---------------------------------------------------------------------------
# Scoring weights
# ---------------------------------------------------------------------------

SCORE_NAME_EXACT       = 50
SCORE_NAME_CONTAINS    = 30
SCORE_DESC_CONTAINS    = 15
SCORE_TAG_MATCH        = 20
SCORE_ENGINE_MATCH     = 10
SCORE_MACRO_MATCH      = 8
SCORE_MOOD_MATCH       = 5
SCORE_PARAM_MATCH      = 3


# ---------------------------------------------------------------------------
# Loading
# ---------------------------------------------------------------------------

def load_preset(path):
    """Load a single .xometa file. Returns dict or None on failure."""
    try:
        with open(path, "r", encoding="utf-8") as f:
            data = json.load(f)
        data["_path"] = path
        return data
    except (json.JSONDecodeError, OSError):
        return None


def load_all_presets(presets_dir):
    """Recursively load all .xometa files under presets_dir."""
    pattern = os.path.join(presets_dir, "**", "*.xometa")
    paths = glob.glob(pattern, recursive=True)
    presets = []
    for p in paths:
        data = load_preset(p)
        if data is not None:
            presets.append(data)
    return presets


# ---------------------------------------------------------------------------
# Scoring
# ---------------------------------------------------------------------------

def tokenize(query):
    """Split query into lowercase tokens."""
    return [t.lower() for t in query.split() if t]


def score_preset(preset, tokens):
    """
    Compute relevance score for a preset against a list of query tokens.
    Scores each token independently then sums.
    Returns total score (int).
    """
    if not tokens:
        return 0

    name        = (preset.get("name") or "").lower()
    description = (preset.get("description") or "").lower()
    mood        = (preset.get("mood") or "").lower()

    # Tags: list of strings
    tags_raw  = preset.get("tags") or []
    tags      = [str(t).lower() for t in tags_raw]
    tags_blob = " ".join(tags)

    # Engines: may be list of strings or list of dicts
    engines_raw = preset.get("engines") or []
    engine_names = []
    for e in engines_raw:
        if isinstance(e, str):
            engine_names.append(e.lower())
        elif isinstance(e, dict):
            for key in ("name", "id", "engine"):
                if key in e:
                    engine_names.append(str(e[key]).lower())
                    break
    engines_blob = " ".join(engine_names)

    # Macros: look for M1-M4 or macros array or macroLabels
    macro_labels = []
    for key in ("macroLabels", "macro_labels", "macros"):
        val = preset.get(key)
        if isinstance(val, list):
            macro_labels = [str(m).lower() for m in val]
            break
        elif isinstance(val, dict):
            macro_labels = [str(v).lower() for v in val.values()]
            break
    macros_blob = " ".join(macro_labels)

    # Parameter names: keys of parameters object
    params_raw = preset.get("parameters") or {}
    if isinstance(params_raw, dict):
        param_keys_blob = " ".join(k.lower() for k in params_raw.keys())
    else:
        param_keys_blob = ""

    total = 0

    for token in tokens:
        token_score = 0

        # Name scoring
        if name == token:
            token_score += SCORE_NAME_EXACT
        elif token in name:
            token_score += SCORE_NAME_CONTAINS

        # Description
        if token in description:
            token_score += SCORE_DESC_CONTAINS

        # Tags (per matching tag)
        for tag in tags:
            if token in tag or tag in token:
                token_score += SCORE_TAG_MATCH

        # Engine
        if token in engines_blob:
            token_score += SCORE_ENGINE_MATCH

        # Macros
        if token in macros_blob:
            token_score += SCORE_MACRO_MATCH

        # Mood
        if token in mood:
            token_score += SCORE_MOOD_MATCH

        # Parameter names
        if token in param_keys_blob:
            token_score += SCORE_PARAM_MATCH

        total += token_score

    return min(total, 100)


# ---------------------------------------------------------------------------
# Filtering
# ---------------------------------------------------------------------------

def apply_filters(presets, engine_filter, mood_filter):
    """Apply optional engine and mood substring filters."""
    filtered = presets

    if engine_filter:
        ef = engine_filter.lower()
        result = []
        for p in filtered:
            engines_raw = p.get("engines") or []
            names = []
            for e in engines_raw:
                if isinstance(e, str):
                    names.append(e.lower())
                elif isinstance(e, dict):
                    for key in ("name", "id", "engine"):
                        if key in e:
                            names.append(str(e[key]).lower())
                            break
            if any(ef in n for n in names):
                result.append(p)
        filtered = result

    if mood_filter:
        mf = mood_filter.lower()
        filtered = [p for p in filtered if mf in (p.get("mood") or "").lower()]

    return filtered


# ---------------------------------------------------------------------------
# Output helpers
# ---------------------------------------------------------------------------

def get_engine_display(preset):
    """Return a short engine name string for display."""
    engines_raw = preset.get("engines") or []
    names = []
    for e in engines_raw:
        if isinstance(e, str):
            names.append(e)
        elif isinstance(e, dict):
            for key in ("name", "id", "engine"):
                if key in e:
                    names.append(str(e[key]))
                    break
    return "+".join(names[:2]) if names else "—"


def description_snippet(preset, length=80):
    """Return first `length` chars of description."""
    desc = (preset.get("description") or "").strip()
    if len(desc) <= length:
        return desc
    return desc[:length - 3].rstrip() + "..."


def format_text(results, query, total_count):
    """Format results as human-readable text."""
    lines = []
    lines.append(f'\nSEARCH: "{query}" in {total_count} presets')
    lines.append(f"Found {len(results)} matches\n")

    for rank, (score, preset) in enumerate(results, 1):
        name    = preset.get("name") or "(unnamed)"
        engine  = get_engine_display(preset)
        mood    = preset.get("mood") or "—"
        tags_raw = preset.get("tags") or []
        tags    = ", ".join(str(t) for t in tags_raw[:5])
        snippet = description_snippet(preset)

        tag_str = f"  tags: {tags}" if tags else ""
        lines.append(f"{rank:>2}. [{score:>3}] {name:<28} {engine:<12} {mood:<12}{tag_str}")
        if snippet:
            lines.append(f'    "{snippet}"')
        lines.append("")

    return "\n".join(lines)


def format_json(results):
    """Format results as JSON array."""
    output = []
    for score, preset in results:
        output.append({
            "score":               score,
            "name":                preset.get("name") or "",
            "engine":              get_engine_display(preset),
            "mood":                preset.get("mood") or "",
            "path":                preset.get("_path") or "",
            "tags":                preset.get("tags") or [],
            "description_snippet": description_snippet(preset),
        })
    return json.dumps(output, indent=2)


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main():
    parser = argparse.ArgumentParser(
        description="Full-text search across XOlokun .xometa preset library"
    )
    parser.add_argument("presets_dir", help="Path to presets directory (searched recursively)")
    parser.add_argument("query", help="Search query (space-separated tokens)")
    parser.add_argument("--engine", metavar="FILTER", help="Filter by engine name (substring)")
    parser.add_argument("--mood",   metavar="FILTER", help="Filter by mood name (substring)")
    parser.add_argument("--top",    metavar="N", type=int, default=10,
                        help="Return top N results (default: 10)")
    parser.add_argument("--format", choices=["text", "json"], default="text",
                        help="Output format: text or json (default: text)")
    args = parser.parse_args()

    # Validate directory
    if not os.path.isdir(args.presets_dir):
        print(f"Error: directory not found: {args.presets_dir}", file=sys.stderr)
        sys.exit(1)

    # Load
    all_presets = load_all_presets(args.presets_dir)
    if not all_presets:
        print(f"No .xometa files found in: {args.presets_dir}", file=sys.stderr)
        sys.exit(1)

    # Filter
    candidates = apply_filters(all_presets, args.engine, args.mood)

    # Score
    tokens = tokenize(args.query)
    scored = []
    for preset in candidates:
        s = score_preset(preset, tokens)
        if s > 0:
            scored.append((s, preset))

    # Sort descending by score, then alphabetically by name as tiebreaker
    scored.sort(key=lambda x: (-x[0], (x[1].get("name") or "").lower()))

    # Trim to top N
    results = scored[:args.top]

    # Output
    if args.format == "json":
        print(format_json(results))
    else:
        print(format_text(results, args.query, len(all_presets)))


if __name__ == "__main__":
    main()
