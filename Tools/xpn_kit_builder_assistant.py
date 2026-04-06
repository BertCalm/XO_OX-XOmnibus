#!/usr/bin/env python3
"""
XPN Kit Builder Assistant — XO_OX Designs
Interactive assistant that guides the user through building a drum kit specification,
generating a structured kit_spec.json ready for the XPN build pipeline.

Usage:
  python xpn_kit_builder_assistant.py [--output kit_spec.json]
  python xpn_kit_builder_assistant.py --non-interactive --theme vintage --energy 3 --focus hip-hop
"""

import argparse
import json
import os
import sys

# ---------------------------------------------------------------------------
# Constants
# ---------------------------------------------------------------------------

THEMES = ["vintage", "modern", "lo-fi", "electronic", "acoustic", "hybrid"]

FOCUS_OPTIONS = ["hip-hop", "trap", "jazz", "rock", "experimental"]

VOICE_TYPES = [
    ("kick",        4),   # (name, default_choke_group)
    ("snare",       2),
    ("closed_hat",  1),
    ("open_hat",    1),
    ("clap",        3),
    ("perc",        0),
]

# DNA target matrices: (theme, energy) -> {brightness, warmth, movement, aggression}
# Energy 1-5; we interpolate between two anchor points per theme.
DNA_ANCHORS = {
    # theme: { energy_low: {...}, energy_high: {...} }
    "vintage":    {"low": (0.40, 0.80, 0.55, 0.25), "high": (0.60, 0.70, 0.80, 0.60)},
    "modern":     {"low": (0.55, 0.50, 0.60, 0.30), "high": (0.85, 0.40, 0.90, 0.80)},
    "lo-fi":      {"low": (0.30, 0.85, 0.45, 0.15), "high": (0.50, 0.75, 0.70, 0.40)},
    "electronic": {"low": (0.60, 0.30, 0.65, 0.35), "high": (0.90, 0.25, 0.95, 0.85)},
    "acoustic":   {"low": (0.45, 0.75, 0.50, 0.20), "high": (0.65, 0.60, 0.75, 0.55)},
    "hybrid":     {"low": (0.50, 0.55, 0.60, 0.30), "high": (0.75, 0.45, 0.85, 0.70)},
}


def compute_dna(theme: str, energy: int) -> dict:
    """Interpolate DNA targets from theme + energy (1-5)."""
    anchors = DNA_ANCHORS.get(theme, DNA_ANCHORS["modern"])
    lo = anchors["low"]
    hi = anchors["high"]
    t = (energy - 1) / 4.0  # normalise to [0, 1]
    keys = ["brightness", "warmth", "movement", "aggression"]
    return {k: round(lo[i] + t * (hi[i] - lo[i]), 2) for i, k in enumerate(keys)}


# ---------------------------------------------------------------------------
# Interactive helpers
# ---------------------------------------------------------------------------

def prompt(question: str, default: str = None) -> str:
    """Ask a question and return stripped input; use default if blank."""
    if default is not None:
        display = f"{question} [{default}]: "
    else:
        display = f"{question}: "
    raw = input(display).strip()
    return raw if raw else (default or "")


def prompt_choice(question: str, choices: list, default: str = None) -> str:
    """Prompt for a value from a fixed list; re-prompt until valid."""
    choices_str = "/".join(choices)
    while True:
        answer = prompt(f"{question} ({choices_str})", default)
        if answer in choices:
            return answer
        print(f"  Please choose one of: {choices_str}")


def prompt_int(question: str, lo: int, hi: int, default: int) -> int:
    """Prompt for an integer in [lo, hi]."""
    while True:
        raw = prompt(question, str(default))
        try:
            val = int(raw)
            if lo <= val <= hi:
                return val
        except ValueError as exc:
            print(f"[WARN] Parsing integer input '{raw}': {exc}", file=sys.stderr)
        print(f"  Please enter a number between {lo} and {hi}.")


def prompt_yn(question: str, default: bool = True) -> bool:
    """Yes/no prompt."""
    default_str = "y" if default else "n"
    while True:
        answer = prompt(f"{question} (y/n)", default_str).lower()
        if answer in ("y", "yes"):
            return True
        if answer in ("n", "no"):
            return False
        print("  Please answer y or n.")


# ---------------------------------------------------------------------------
# Interactive interview
# ---------------------------------------------------------------------------

def run_interview(args) -> dict:
    """Walk the user through kit specification interactively."""
    print()
    print("=" * 60)
    print("  XPN Kit Builder Assistant — XO_OX Designs")
    print("=" * 60)
    print()

    # 1. Kit name
    kit_name = prompt("Kit name", "Untitled Kit")

    # 2. Theme
    theme = prompt_choice("Theme", THEMES, "vintage")

    # 3. Energy
    print("  Energy: 1 = ghost notes / quiet   5 = maximum aggression")
    energy = prompt_int("Energy level (1-5)", 1, 5, 3)

    # 4. Focus
    focus = prompt_choice("Key voice focus", FOCUS_OPTIONS, "hip-hop")

    print()
    print("── Voice Configuration ──")
    print("  For each drum voice, choose whether to include it, how many")
    print("  velocity layers, and any special notes for the sound designer.")
    print()

    voices = {}
    for voice_name, choke_group in VOICE_TYPES:
        include = prompt_yn(f"Include {voice_name}?", default=True)
        if not include:
            continue
        layers = prompt_int(
            f"  {voice_name} — velocity layers (2/3/4)", 2, 4, 4
        )
        notes_raw = prompt(
            f"  {voice_name} — special notes (press Enter to skip)", ""
        )
        voices[voice_name] = {
            "layers": layers,
            "notes": notes_raw if notes_raw else None,
            "choke_group": choke_group,
        }
        print()

    dna = compute_dna(theme, energy)

    spec = {
        "kit_name": kit_name,
        "theme": theme,
        "energy": energy,
        "focus": focus,
        "voices": voices,
        "dna_targets": dna,
    }
    return spec


# ---------------------------------------------------------------------------
# Non-interactive defaults
# ---------------------------------------------------------------------------

def build_default_spec(args) -> dict:
    """Build a spec from CLI args + sensible defaults (no prompts)."""
    theme = args.theme or "vintage"
    energy = args.energy or 3
    focus = args.focus or "hip-hop"
    kit_name = args.kit_name or f"{theme.title()} Kit"

    voices = {}
    for voice_name, choke_group in VOICE_TYPES:
        voices[voice_name] = {
            "layers": 4,
            "notes": None,
            "choke_group": choke_group,
        }

    dna = compute_dna(theme, energy)

    return {
        "kit_name": kit_name,
        "theme": theme,
        "energy": energy,
        "focus": focus,
        "voices": voices,
        "dna_targets": dna,
    }


# ---------------------------------------------------------------------------
# Output
# ---------------------------------------------------------------------------

def write_spec(spec: dict, output_path: str) -> None:
    """Write kit_spec.json to disk."""
    with open(output_path, "w", encoding="utf-8") as f:
        json.dump(spec, f, indent=2)
    print(f"\n  Saved: {os.path.abspath(output_path)}")


def print_summary(spec: dict) -> None:
    """Pretty-print the generated spec to stdout."""
    print()
    print("── Generated Kit Spec ──")
    print(json.dumps(spec, indent=2))
    print()


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def build_parser() -> argparse.ArgumentParser:
    p = argparse.ArgumentParser(
        description="XPN Kit Builder Assistant — interactive drum kit spec generator"
    )
    p.add_argument("--output", default="kit_spec.json",
                   help="Output path for kit_spec.json (default: kit_spec.json)")
    p.add_argument("--non-interactive", action="store_true",
                   help="Skip all prompts; use CLI args + defaults")
    p.add_argument("--theme", choices=THEMES,
                   help="Kit theme (non-interactive mode)")
    p.add_argument("--energy", type=int, choices=range(1, 6), metavar="1-5",
                   help="Energy level 1-5 (non-interactive mode)")
    p.add_argument("--focus", choices=FOCUS_OPTIONS,
                   help="Voice focus style (non-interactive mode)")
    p.add_argument("--kit-name", dest="kit_name",
                   help="Kit name (non-interactive mode)")
    return p


def main() -> int:
    parser = build_parser()
    args = parser.parse_args()

    if args.non_interactive:
        spec = build_default_spec(args)
    else:
        try:
            spec = run_interview(args)
        except (KeyboardInterrupt, EOFError):
            print("\n\n  Aborted.")
            return 1

    print_summary(spec)
    write_spec(spec, args.output)
    print("  Done. Pass this file to xpn_drum_export.py or xpn_kit_expander.py")
    print()
    return 0


if __name__ == "__main__":
    sys.exit(main())
