#!/usr/bin/env python3
"""
xpn_macro_curve_designer.py — XO_OX Macro Response Curve Designer

Design and visualize macro response curves for XPN programs.
Macros M1-M4 (CHARACTER, MOVEMENT, COUPLING, SPACE) map MPC CC 0-127
to underlying parameter ranges via curve shapes.

Usage:
  python xpn_macro_curve_designer.py --type log --min 0.0 --max 1.0 --params filter_cutoff resonance
  python xpn_macro_curve_designer.py --preset brightness --xml
  python xpn_macro_curve_designer.py --spec curves.json
"""

import argparse
import json
import math
import sys
from typing import List, Tuple, Dict, Any


# ---------------------------------------------------------------------------
# Curve math
# ---------------------------------------------------------------------------

def curve_linear(t: float) -> float:
    return t


def curve_log(t: float) -> float:
    """Logarithmic — fast rise, expressive at low end."""
    return math.log1p(t * (math.e - 1))


def curve_exp(t: float) -> float:
    """Exponential — slow start, dramatic finish."""
    return (math.exp(t) - 1) / (math.e - 1)


def curve_scurve(t: float) -> float:
    """S-curve (smooth step) — natural feel, avoids extremes."""
    return t * t * (3 - 2 * t)


def curve_step(t: float, steps: int = 4) -> float:
    """Step quantize — discrete jumps (useful for octave/mode macros)."""
    return math.floor(t * steps) / steps


CURVE_FUNCTIONS = {
    "linear": curve_linear,
    "log": curve_log,
    "exp": curve_exp,
    "scurve": curve_scurve,
    "step": curve_step,
}

CURVE_DESCRIPTIONS = {
    "linear": "Uniform response across full range",
    "log":    "Fast rise — expressive detail at low CC values",
    "exp":    "Slow start, dramatic finish — tension builds",
    "scurve": "S-curve smooth step — natural, avoids extremes",
    "step":   "Discrete jumps — octaves, modes, snap values",
}


# ---------------------------------------------------------------------------
# Preset library
# ---------------------------------------------------------------------------

PRESETS: Dict[str, Dict[str, Any]] = {
    "brightness": {
        "macro": "CHARACTER",
        "curve": "log",
        "min": 0.0,
        "max": 1.0,
        "params": [
            {"name": "filter_cutoff",  "min": 0.10, "max": 1.00},
            {"name": "filter_res",     "min": 0.00, "max": 0.55},
            {"name": "drive",          "min": 0.00, "max": 0.40},
        ],
        "description": "Logarithmic brightness sweep — filter opens fast, harmonics follow",
    },
    "warmth": {
        "macro": "CHARACTER",
        "curve": "scurve",
        "min": 0.0,
        "max": 1.0,
        "params": [
            {"name": "filter_cutoff",  "min": 0.60, "max": 0.20},   # inverted: warm = closed
            {"name": "saturation",     "min": 0.00, "max": 0.70},
            {"name": "chorus_depth",   "min": 0.00, "max": 0.35},
        ],
        "description": "S-curve warmth — smooth closure + saturation bloom",
    },
    "movement": {
        "macro": "MOVEMENT",
        "curve": "exp",
        "min": 0.0,
        "max": 1.0,
        "params": [
            {"name": "lfo_rate",       "min": 0.05, "max": 1.00},
            {"name": "lfo_depth",      "min": 0.00, "max": 0.80},
            {"name": "tremolo_depth",  "min": 0.00, "max": 0.60},
        ],
        "description": "Exponential movement — still at rest, alive at the top",
    },
    "space": {
        "macro": "SPACE",
        "curve": "linear",
        "min": 0.0,
        "max": 1.0,
        "params": [
            {"name": "reverb_size",    "min": 0.00, "max": 1.00},
            {"name": "reverb_mix",     "min": 0.00, "max": 0.75},
            {"name": "delay_mix",      "min": 0.00, "max": 0.50},
        ],
        "description": "Linear space — predictable, mix-friendly room sizing",
    },
}


# ---------------------------------------------------------------------------
# Curve evaluation
# ---------------------------------------------------------------------------

def evaluate_curve(
    curve_type: str,
    out_min: float,
    out_max: float,
    points: int = 128,
) -> List[Tuple[int, float]]:
    """Return list of (cc_value, output_value) pairs."""
    fn = CURVE_FUNCTIONS[curve_type]
    result = []
    for i in range(points):
        t = i / (points - 1)
        y_norm = fn(t)
        y_norm = max(0.0, min(1.0, y_norm))
        y = out_min + y_norm * (out_max - out_min)
        cc = round(i * 127 / (points - 1))
        result.append((cc, y))
    return result


def evaluate_param_curve(
    curve_type: str,
    param_min: float,
    param_max: float,
    points: int = 128,
) -> List[Tuple[int, float]]:
    return evaluate_curve(curve_type, param_min, param_max, points)


# ---------------------------------------------------------------------------
# ASCII plot
# ---------------------------------------------------------------------------

PLOT_CHARS = ["*", "+", "#", "o", "x", "@", "%", "~"]
COLS = 64
ROWS = 20


def render_ascii_plot(
    curves: List[Tuple[str, List[Tuple[int, float]]]],
    title: str = "Macro Response Curve",
) -> str:
    """
    Render multiple named curves on a single ASCII plot.
    curves: list of (label, [(cc, value), ...])
    """
    # Find global y range across all curves
    all_vals = [v for _, pts in curves for _, v in pts]
    y_min = min(all_vals)
    y_max = max(all_vals)
    y_range = y_max - y_min if y_max != y_min else 1.0

    # Build grid
    grid = [[" "] * COLS for _ in range(ROWS)]

    # Plot each curve
    for ci, (label, pts) in enumerate(curves):
        char = PLOT_CHARS[ci % len(PLOT_CHARS)]
        for cc, val in pts:
            col = round(cc * (COLS - 1) / 127)
            row = ROWS - 1 - round((val - y_min) / y_range * (ROWS - 1))
            row = max(0, min(ROWS - 1, row))
            col = max(0, min(COLS - 1, col))
            # Only overwrite space (first curve wins on collision)
            if grid[row][col] == " ":
                grid[row][col] = char

    # Assemble output
    lines = []
    lines.append(f"  {title}")
    lines.append(f"  {'─' * COLS}")

    for r in range(ROWS):
        # Y-axis label every ~5 rows
        if r == 0:
            y_label = f"{y_max:5.2f}"
        elif r == ROWS - 1:
            y_label = f"{y_min:5.2f}"
        elif r == ROWS // 2:
            y_label = f"{(y_min + y_max) / 2:5.2f}"
        else:
            y_label = "     "
        lines.append(f"{y_label} │{''.join(grid[r])}│")

    lines.append(f"       └{'─' * COLS}┘")
    # X axis labels
    x_left  = "CC:0"
    x_mid   = "64"
    x_right = "127"
    pad_mid  = COLS // 2 - len(x_left) - len(x_mid) // 2
    pad_right = COLS - len(x_left) - pad_mid - len(x_mid) - len(x_right)
    lines.append(f"        {x_left}{' ' * pad_mid}{x_mid}{' ' * max(0, pad_right)}{x_right}")

    # Legend
    lines.append("")
    for ci, (label, _) in enumerate(curves):
        char = PLOT_CHARS[ci % len(PLOT_CHARS)]
        lines.append(f"  [{char}] {label}")

    return "\n".join(lines)


# ---------------------------------------------------------------------------
# XML generation
# ---------------------------------------------------------------------------

def generate_qlink_xml(
    macro_index: int,
    macro_name: str,
    curve_type: str,
    params: List[Dict[str, Any]],
    cc_channel: int = 1,
) -> str:
    """Generate MPC Q-Link XML snippet for a macro curve assignment."""
    cc_number = 16 + macro_index  # Q-Links typically CC16-CC19
    lines = [
        f'<!-- Macro {macro_index + 1}: {macro_name} | Curve: {curve_type} -->',
        f'<QLink index="{macro_index}">',
        f'  <Name>{macro_name}</Name>',
        f'  <CCNumber>{cc_number}</CCNumber>',
        f'  <CCChannel>{cc_channel}</CCChannel>',
        f'  <CurveType>{curve_type.upper()}</CurveType>',
        f'  <Assignments>',
    ]
    for i, p in enumerate(params):
        p_min = p.get("min", 0.0)
        p_max = p.get("max", 1.0)
        lines.append(f'    <Assignment index="{i}">')
        lines.append(f'      <Parameter>{p["name"]}</Parameter>')
        lines.append(f'      <RangeMin>{p_min:.4f}</RangeMin>')
        lines.append(f'      <RangeMax>{p_max:.4f}</RangeMax>')
        lines.append(f'    </Assignment>')
    lines += [
        f'  </Assignments>',
        f'</QLink>',
    ]
    return "\n".join(lines)


# ---------------------------------------------------------------------------
# Single curve workflow
# ---------------------------------------------------------------------------

def run_single_curve(args: argparse.Namespace) -> None:
    curve_type = args.type
    out_min = args.min
    out_max = args.max
    points = args.points
    param_names = args.params or ["output"]
    macro_name = args.macro or "M1"

    print(f"\nCurve type : {curve_type} — {CURVE_DESCRIPTIONS[curve_type]}")
    print(f"Output range: [{out_min:.3f} → {out_max:.3f}]")
    print(f"Parameters  : {', '.join(param_names)}")
    print()

    # Build per-param curves (all share the same curve shape, different ranges)
    # When no explicit per-param ranges given, all share out_min/out_max
    plot_curves = []
    for pname in param_names:
        pts = evaluate_curve(curve_type, out_min, out_max, points)
        plot_curves.append((pname, pts))

    print(render_ascii_plot(plot_curves, title=f"{macro_name} — {curve_type}"))

    if args.xml:
        print("\n" + "─" * 66)
        print("Q-Link XML snippet:")
        print("─" * 66)
        param_dicts = [{"name": n, "min": out_min, "max": out_max} for n in param_names]
        print(generate_qlink_xml(0, macro_name, curve_type, param_dicts))


# ---------------------------------------------------------------------------
# Preset workflow
# ---------------------------------------------------------------------------

def run_preset(preset_name: str, show_xml: bool) -> None:
    if preset_name not in PRESETS:
        print(f"Unknown preset '{preset_name}'. Available: {', '.join(PRESETS)}")
        sys.exit(1)

    spec = PRESETS[preset_name]
    curve_type = spec["curve"]
    macro_name = spec["macro"]
    params = spec["params"]

    print(f"\nPreset      : {preset_name}")
    print(f"Macro       : {macro_name}")
    print(f"Curve       : {curve_type} — {CURVE_DESCRIPTIONS[curve_type]}")
    print(f"Description : {spec['description']}")
    print()

    plot_curves = []
    for p in params:
        pts = evaluate_param_curve(curve_type, p["min"], p["max"])
        plot_curves.append((p["name"], pts))

    print(render_ascii_plot(plot_curves, title=f"{macro_name} ({preset_name}) — {curve_type}"))

    if show_xml:
        print("\n" + "─" * 66)
        print("Q-Link XML snippet:")
        print("─" * 66)
        print(generate_qlink_xml(0, macro_name, curve_type, params))


# ---------------------------------------------------------------------------
# Batch / spec file workflow
# ---------------------------------------------------------------------------

def run_spec_file(spec_path: str) -> None:
    try:
        with open(spec_path) as f:
            spec_data = json.load(f)
    except FileNotFoundError:
        print(f"Spec file not found: {spec_path}")
        sys.exit(1)
    except json.JSONDecodeError as e:
        print(f"Invalid JSON in spec file: {e}")
        sys.exit(1)

    curves_spec = spec_data if isinstance(spec_data, list) else spec_data.get("curves", [])

    print(f"Batch mode — {len(curves_spec)} curve(s) from {spec_path}\n")

    for idx, entry in enumerate(curves_spec):
        curve_type  = entry.get("curve", "linear")
        macro_name  = entry.get("macro", f"M{idx + 1}")
        params      = entry.get("params", [{"name": "output", "min": 0.0, "max": 1.0}])
        description = entry.get("description", "")
        show_xml    = entry.get("xml", True)

        if curve_type not in CURVE_FUNCTIONS:
            print(f"[{idx}] Unknown curve type '{curve_type}', skipping.")
            continue

        print(f"{'═' * 66}")
        print(f"  Curve {idx + 1}/{len(curves_spec)}: {macro_name} | {curve_type}")
        if description:
            print(f"  {description}")
        print()

        plot_curves = []
        for p in params:
            pts = evaluate_param_curve(curve_type, p.get("min", 0.0), p.get("max", 1.0))
            plot_curves.append((p["name"], pts))

        print(render_ascii_plot(plot_curves, title=f"{macro_name} — {curve_type}"))

        if show_xml:
            print("\nQ-Link XML:")
            print(generate_qlink_xml(idx % 4, macro_name, curve_type, params))

        print()


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def build_parser() -> argparse.ArgumentParser:
    p = argparse.ArgumentParser(
        description="XO_OX XPN Macro Curve Designer — design M1-M4 response curves",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  # Single log curve, two params
  python xpn_macro_curve_designer.py --type log --min 0.1 --max 1.0 --params filter_cutoff resonance

  # Built-in preset with XML output
  python xpn_macro_curve_designer.py --preset brightness --xml

  # All four presets at once
  python xpn_macro_curve_designer.py --preset warmth
  python xpn_macro_curve_designer.py --preset movement --xml
  python xpn_macro_curve_designer.py --preset space

  # Batch mode from JSON spec
  python xpn_macro_curve_designer.py --spec curves.json

Preset curve library:
  brightness  log    — CHARACTER: fast filter open, log response
  warmth      scurve — CHARACTER: S-curve warmth + saturation
  movement    exp    — MOVEMENT: exponential LFO acceleration
  space       linear — SPACE: linear reverb/delay mix
        """,
    )
    p.add_argument("--type",   choices=list(CURVE_FUNCTIONS), default="linear",
                   help="Curve shape (default: linear)")
    p.add_argument("--min",    type=float, default=0.0,
                   help="Output minimum value (default: 0.0)")
    p.add_argument("--max",    type=float, default=1.0,
                   help="Output maximum value (default: 1.0)")
    p.add_argument("--points", type=int,   default=128,
                   help="Number of sample points (default: 128)")
    p.add_argument("--params", nargs="+",  metavar="PARAM",
                   help="Parameter name(s) this macro controls")
    p.add_argument("--macro",  default="M1",
                   help="Macro name label, e.g. CHARACTER (default: M1)")
    p.add_argument("--xml",    action="store_true",
                   help="Emit Q-Link XML snippet")
    p.add_argument("--preset", choices=list(PRESETS),
                   help="Use a built-in preset curve (overrides --type/--min/--max)")
    p.add_argument("--spec",   metavar="FILE",
                   help="Batch mode: path to JSON curve spec file")
    p.add_argument("--list-presets", action="store_true",
                   help="List all available preset curves and exit")
    return p


def main() -> None:
    parser = build_parser()
    args = parser.parse_args()

    if args.list_presets:
        print("\nAvailable preset curves:\n")
        for name, spec in PRESETS.items():
            print(f"  {name:<12} curve={spec['curve']:<8} macro={spec['macro']:<12} {spec['description']}")
        print()
        sys.exit(0)

    if args.spec:
        run_spec_file(args.spec)
        return

    if args.preset:
        run_preset(args.preset, args.xml)
        return

    run_single_curve(args)


if __name__ == "__main__":
    main()
