#!/usr/bin/env python3
"""
xpn_preset_browser.py — Terminal browser for .xometa preset files.

Scan a directory tree of XOmnibus .xometa presets and explore them via
five modes: list (filtered), search (keyword), dna (sorted by dimension),
show (full detail), and random (selection helper).

Usage:
    python xpn_preset_browser.py --presets-dir ./Presets/XOmnibus/ list --engine ONSET
    python xpn_preset_browser.py --presets-dir ./Presets/XOmnibus/ list --engine ONSET --mood Foundation --min-brightness 0.5
    python xpn_preset_browser.py --presets-dir ./Presets/XOmnibus/ search --query "bass heavy"
    python xpn_preset_browser.py --presets-dir ./Presets/XOmnibus/ dna --engine OBLONG --sort brightness
    python xpn_preset_browser.py --presets-dir ./Presets/XOmnibus/ show --preset "Iron Hat"
    python xpn_preset_browser.py --presets-dir ./Presets/XOmnibus/ random --engine OPAL --count 5
"""

import argparse
import glob
import json
import os
import random
import sys
from typing import Any, Dict, List, Optional, Tuple

DNA_DIMS = ("brightness", "warmth", "movement", "density", "space", "aggression")


# ---------------------------------------------------------------------------
# Loading
# ---------------------------------------------------------------------------

def load_presets(presets_dir: str) -> List[Dict[str, Any]]:
    """Recursively load all .xometa files under presets_dir."""
    pattern = os.path.join(presets_dir, "**", "*.xometa")
    paths = glob.glob(pattern, recursive=True)
    presets = []
    for path in sorted(paths):
        try:
            with open(path, "r", encoding="utf-8") as fh:
                data = json.load(fh)
            data["_path"] = path
            presets.append(data)
        except (json.JSONDecodeError, OSError):
            pass  # silently skip malformed files
    return presets


def get_dna(preset: Dict[str, Any]) -> Dict[str, float]:
    """Return the dna dict, supporting both 'dna' and 'sonic_dna'/'sonicDNA' keys."""
    for key in ("dna", "sonic_dna", "sonicDNA"):
        val = preset.get(key)
        if isinstance(val, dict):
            return val
    return {}


def get_engines(preset: Dict[str, Any]) -> List[str]:
    """Return engine list normalised to uppercase short names."""
    raw = preset.get("engines", [])
    if not isinstance(raw, list):
        return []
    return [str(e).upper() for e in raw]


def get_macros(preset: Dict[str, Any]) -> List[Any]:
    """Return macros list (up to 4 values)."""
    return preset.get("macros", preset.get("macroLabels", []))


# ---------------------------------------------------------------------------
# Filtering helpers
# ---------------------------------------------------------------------------

def apply_dna_filters(preset: Dict[str, Any], args: argparse.Namespace) -> bool:
    """Return True if preset passes all DNA filter flags."""
    dna = get_dna(preset)
    checks = [
        ("min_brightness",  "brightness",  lambda v, t: v >= t),
        ("max_brightness",  "brightness",  lambda v, t: v <= t),
        ("min_warmth",      "warmth",      lambda v, t: v >= t),
        ("max_warmth",      "warmth",      lambda v, t: v <= t),
        ("min_movement",    "movement",    lambda v, t: v >= t),
        ("max_movement",    "movement",    lambda v, t: v <= t),
        ("min_density",     "density",     lambda v, t: v >= t),
        ("max_density",     "density",     lambda v, t: v <= t),
        ("min_space",       "space",       lambda v, t: v >= t),
        ("max_space",       "space",       lambda v, t: v <= t),
        ("min_aggression",  "aggression",  lambda v, t: v >= t),
        ("max_aggression",  "aggression",  lambda v, t: v <= t),
    ]
    for flag, dim, test in checks:
        threshold = getattr(args, flag, None)
        if threshold is not None:
            val = dna.get(dim)
            if val is None or not test(float(val), threshold):
                return False
    return True


def filter_presets(
    presets: List[Dict[str, Any]],
    engine: Optional[str],
    mood: Optional[str],
    args: argparse.Namespace,
) -> List[Dict[str, Any]]:
    results = []
    for p in presets:
        if engine and engine.upper() not in get_engines(p):
            continue
        if mood and str(p.get("mood", "")).lower() != mood.lower():
            continue
        if not apply_dna_filters(p, args):
            continue
        results.append(p)
    return results


# ---------------------------------------------------------------------------
# Output helpers
# ---------------------------------------------------------------------------

def dna_bar(value: float, width: int = 20) -> str:
    """Render a simple ASCII progress bar for a 0–1 DNA value."""
    filled = int(round(value * width))
    return "[" + "#" * filled + "-" * (width - filled) + f"] {value:.2f}"


def fmt_preset_line(preset: Dict[str, Any], show_mood: bool = True) -> str:
    name = preset.get("name", "(unnamed)")
    mood = preset.get("mood", "")
    engines = ", ".join(get_engines(preset))
    parts = [f"  {name:<32}", f"  [{engines}]"]
    if show_mood:
        parts.append(f"  {mood}")
    return "".join(parts)


def print_header(title: str) -> None:
    print()
    print(f"  {title}")
    print("  " + "-" * (len(title) + 2))


def print_count(n: int, label: str = "preset") -> None:
    s = "" if n == 1 else "s"
    print(f"\n  {n} {label}{s} found.\n")


# ---------------------------------------------------------------------------
# Commands
# ---------------------------------------------------------------------------

def cmd_list(presets: List[Dict[str, Any]], args: argparse.Namespace) -> None:
    engine = getattr(args, "engine", None)
    mood = getattr(args, "mood", None)
    results = filter_presets(presets, engine, mood, args)
    results.sort(key=lambda p: p.get("name", "").lower())

    title = "Presets"
    if engine:
        title += f" — {engine.upper()}"
    if mood:
        title += f" / {mood}"
    print_header(title)

    for p in results:
        print(fmt_preset_line(p, show_mood=(mood is None)))

    print_count(len(results))


def cmd_search(presets: List[Dict[str, Any]], args: argparse.Namespace) -> None:
    query = args.query.lower()
    tokens = query.split()
    results = []
    for p in presets:
        name = p.get("name", "").lower()
        desc = p.get("description", "").lower()
        tags = " ".join(p.get("tags", [])).lower()
        haystack = f"{name} {desc} {tags}"
        if all(t in haystack for t in tokens):
            results.append(p)
    results.sort(key=lambda p: p.get("name", "").lower())

    print_header(f'Search: "{args.query}"')
    for p in results:
        print(fmt_preset_line(p))
        desc = p.get("description", "")
        if desc:
            print(f"    {desc[:80]}")
    print_count(len(results))


def cmd_dna(presets: List[Dict[str, Any]], args: argparse.Namespace) -> None:
    engine = getattr(args, "engine", None)
    mood = getattr(args, "mood", None)
    sort_dim = (args.sort or "brightness").lower()
    if sort_dim not in DNA_DIMS:
        print(f"  Error: --sort must be one of: {', '.join(DNA_DIMS)}")
        sys.exit(1)

    results = filter_presets(presets, engine, mood, args)
    results.sort(key=lambda p: float(get_dna(p).get(sort_dim, 0.0)), reverse=True)

    title = f"DNA Sort: {sort_dim}"
    if engine:
        title += f"  ({engine.upper()})"
    print_header(title)

    col_w = 30
    print(f"  {'Preset':<{col_w}}  {sort_dim.capitalize():<26}  Engines")
    print("  " + "-" * 72)
    for p in results:
        name = p.get("name", "(unnamed)")[:col_w]
        val = float(get_dna(p).get(sort_dim, 0.0))
        engines = ", ".join(get_engines(p))
        bar = dna_bar(val, 18)
        print(f"  {name:<{col_w}}  {bar}  {engines}")

    print_count(len(results))


def cmd_show(presets: List[Dict[str, Any]], args: argparse.Namespace) -> None:
    target = args.preset.lower()
    matches = [p for p in presets if p.get("name", "").lower() == target]
    if not matches:
        # Fuzzy fallback: contains
        matches = [p for p in presets if target in p.get("name", "").lower()]
    if not matches:
        print(f"\n  No preset found matching: {args.preset}\n")
        sys.exit(1)
    if len(matches) > 1:
        print(f"\n  Multiple matches — showing first. Others: {[p['name'] for p in matches[1:]]}")
    p = matches[0]

    name = p.get("name", "(unnamed)")
    mood = p.get("mood", "—")
    engines = ", ".join(get_engines(p)) or "—"
    desc = p.get("description", "")
    tags = ", ".join(p.get("tags", [])) or "—"
    coupling = p.get("couplingIntensity", "—")

    print()
    print(f"  {'=' * 60}")
    print(f"  {name}")
    print(f"  {'=' * 60}")
    print(f"  Mood     : {mood}")
    print(f"  Engines  : {engines}")
    print(f"  Coupling : {coupling}")
    if desc:
        print(f"  Desc     : {desc}")
    print(f"  Tags     : {tags}")

    # Macros
    macros = get_macros(p)
    if macros:
        print()
        print("  Macros:")
        for i, m in enumerate(macros[:4], 1):
            print(f"    M{i}: {m}")

    # DNA
    dna = get_dna(p)
    if dna:
        print()
        print("  Sonic DNA:")
        for dim in DNA_DIMS:
            val = dna.get(dim)
            if val is not None:
                print(f"    {dim:<12} {dna_bar(float(val), 22)}")

    # Parameters
    params = p.get("parameters", {})
    if params:
        print()
        print("  Parameters:")
        for engine_key, engine_params in params.items():
            print(f"    [{engine_key}]")
            if isinstance(engine_params, dict):
                for k, v in engine_params.items():
                    print(f"      {k:<40} {v}")
            else:
                print(f"      {engine_params}")

    print()


def cmd_random(presets: List[Dict[str, Any]], args: argparse.Namespace) -> None:
    engine = getattr(args, "engine", None)
    mood = getattr(args, "mood", None)
    count = args.count or 5
    pool = filter_presets(presets, engine, mood, args)

    if not pool:
        print(f"\n  No presets found for the given filters.\n")
        sys.exit(1)

    count = min(count, len(pool))
    selected = random.sample(pool, count)
    selected.sort(key=lambda p: p.get("name", "").lower())

    title = f"Random {count} preset{'s' if count != 1 else ''}"
    if engine:
        title += f" from {engine.upper()}"
    print_header(title)

    for p in selected:
        print(fmt_preset_line(p))
        dna = get_dna(p)
        if dna:
            snippet = "  ".join(
                f"{d[0].upper()}:{dna[d]:.2f}" for d in DNA_DIMS if d in dna
            )
            print(f"    DNA  {snippet}")
        desc = p.get("description", "")
        if desc:
            print(f"    {desc[:80]}")

    print()


# ---------------------------------------------------------------------------
# Argument parsing
# ---------------------------------------------------------------------------

def add_dna_filters(parser: argparse.ArgumentParser) -> None:
    for dim in DNA_DIMS:
        parser.add_argument(f"--min-{dim}", type=float, dest=f"min_{dim}", default=None,
                            metavar="0.0-1.0", help=f"Min {dim} DNA value")
        parser.add_argument(f"--max-{dim}", type=float, dest=f"max_{dim}", default=None,
                            metavar="0.0-1.0", help=f"Max {dim} DNA value")


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Terminal browser for XOmnibus .xometa preset files.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    parser.add_argument("--presets-dir", default="./Presets/XOmnibus/",
                        help="Root directory to scan for .xometa files (default: ./Presets/XOmnibus/)")

    subs = parser.add_subparsers(dest="command", required=True)

    # list
    p_list = subs.add_parser("list", help="Filtered list of preset names")
    p_list.add_argument("--engine", help="Filter by engine name (e.g. ONSET)")
    p_list.add_argument("--mood", help="Filter by mood (e.g. Foundation)")
    add_dna_filters(p_list)

    # search
    p_search = subs.add_parser("search", help="Keyword search in names and descriptions")
    p_search.add_argument("--query", required=True, help="Search terms (space-separated)")

    # dna
    p_dna = subs.add_parser("dna", help="List presets sorted by a DNA dimension")
    p_dna.add_argument("--engine", help="Filter by engine name")
    p_dna.add_argument("--mood", help="Filter by mood")
    p_dna.add_argument("--sort", default="brightness",
                       choices=list(DNA_DIMS), help="DNA dimension to sort by (default: brightness)")
    add_dna_filters(p_dna)

    # show
    p_show = subs.add_parser("show", help="Full details for a single preset")
    p_show.add_argument("--preset", required=True, help='Preset name (exact or partial match)')

    # random
    p_rand = subs.add_parser("random", help="Pick N random presets from a filtered pool")
    p_rand.add_argument("--engine", help="Filter by engine name")
    p_rand.add_argument("--mood", help="Filter by mood")
    p_rand.add_argument("--count", type=int, default=5, help="Number of presets to pick (default: 5)")
    add_dna_filters(p_rand)

    return parser


# ---------------------------------------------------------------------------
# Entry point
# ---------------------------------------------------------------------------

def main() -> None:
    parser = build_parser()
    args = parser.parse_args()

    presets_dir = os.path.expanduser(args.presets_dir)
    if not os.path.isdir(presets_dir):
        print(f"\n  Error: presets directory not found: {presets_dir}\n")
        sys.exit(1)

    presets = load_presets(presets_dir)
    if not presets:
        print(f"\n  No .xometa files found under: {presets_dir}\n")
        sys.exit(1)

    dispatch = {
        "list": cmd_list,
        "search": cmd_search,
        "dna": cmd_dna,
        "show": cmd_show,
        "random": cmd_random,
    }
    dispatch[args.command](presets, args)


if __name__ == "__main__":
    main()
