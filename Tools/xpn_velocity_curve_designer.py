#!/usr/bin/env python3
"""
XPN Velocity Curve Designer — XO_OX Designs
Generates velocity boundary configurations for XPM programs following
Vibe's musical velocity curve, and outputs them as XML snippets or JSON
for embedding in XPN builds.

Vibe's Musical Velocity Curve (canonical XO_OX standard):
  Layer 1 (ghost/whisper): VelStart=1,  VelEnd=40
  Layer 2 (medium):        VelStart=41, VelEnd=90
  Layer 3 (hard):          VelStart=91, VelEnd=110
  Layer 4 (accent/slam):   VelStart=111, VelEnd=127

Usage:
    # Default Vibe curve, text output
    python3 xpn_velocity_curve_designer.py

    # Specific preset
    python3 xpn_velocity_curve_designer.py --preset soft-top

    # Custom boundaries (N+1 values for N layers)
    python3 xpn_velocity_curve_designer.py --boundaries 1,50,100,115,127

    # Choose number of layers (with a compatible preset)
    python3 xpn_velocity_curve_designer.py --preset 2layer --layers 2

    # XML output
    python3 xpn_velocity_curve_designer.py --format xml

    # JSON output
    python3 xpn_velocity_curve_designer.py --format json
"""

import argparse
import json
import sys

# ---------------------------------------------------------------------------
# Preset boundary tables  (boundaries[i] = start of layer i+1)
# Length = num_layers + 1; first value always 1, last always 127
# ---------------------------------------------------------------------------
PRESETS = {
    "vibe":     [1, 20, 55, 90, 127],    # Ghost Council Modified zones (QDD Level 2, 2026-04-04)
    "linear":   [1, 32, 64, 96, 127],    # Evenly spaced (approx)
    "soft-top": [1, 60, 100, 115, 127],  # More range at soft end
    "hard-top": [1, 30, 70, 100, 127],   # More range at loud end
    "2layer":   [1, 63, 127],
    "3layer":   [1, 45, 90, 127],
}

LAYER_NAMES_BY_COUNT = {
    2: ["soft", "hard"],
    3: ["soft", "medium", "hard"],
    4: ["ghost", "medium", "hard", "accent"],
}

# Suggested per-layer volume defaults (mix-ready starting points)
VOLUME_SUGGESTIONS_4 = [85, 95, 100, 100]
VOLUME_SUGGESTIONS_3 = [88, 97, 100]
VOLUME_SUGGESTIONS_2 = [90, 100]


# ---------------------------------------------------------------------------
# Core helpers
# ---------------------------------------------------------------------------

def boundaries_to_layers(boundaries: list[int]) -> list[dict]:
    """Convert a boundary list to a list of layer dicts."""
    n = len(boundaries) - 1
    names = LAYER_NAMES_BY_COUNT.get(n, [f"layer{i+1}" for i in range(n)])
    if n == 4:
        vols = VOLUME_SUGGESTIONS_4
    elif n == 3:
        vols = VOLUME_SUGGESTIONS_3
    elif n == 2:
        vols = VOLUME_SUGGESTIONS_2
    else:
        vols = [100] * n

    layers = []
    for i in range(n):
        vel_start = boundaries[i] + (1 if i > 0 else 0)
        # For the first layer start is exactly boundaries[0] (=1)
        vel_start = boundaries[i] if i == 0 else boundaries[i] + 1
        vel_end = boundaries[i + 1]
        layers.append({
            "layer": i + 1,
            "name": names[i],
            "vel_start": vel_start,
            "vel_end": vel_end,
            "volume_suggestion": vols[i] if i < len(vols) else 100,
        })
    return layers


def parse_boundaries(raw: str) -> list[int]:
    """Parse a comma-separated boundary string and validate."""
    try:
        vals = [int(v.strip()) for v in raw.split(",")]
    except ValueError:
        sys.exit("Error: --boundaries must be comma-separated integers, e.g. 1,40,90,110,127")

    if len(vals) < 3:
        sys.exit("Error: Need at least 3 boundary values (2 layers minimum).")
    if vals[0] != 1:
        sys.exit("Error: First boundary value must be 1.")
    if vals[-1] != 127:
        sys.exit("Error: Last boundary value must be 127.")
    for a, b in zip(vals, vals[1:]):
        if b <= a:
            sys.exit("Error: Boundary values must be strictly increasing.")
    return vals


# ---------------------------------------------------------------------------
# Output formatters
# ---------------------------------------------------------------------------

BAR_CHARS = {
    "ghost":  "░",
    "soft":   "░",
    "medium": "█",
    "hard":   "▓",
    "accent": "█",
}
BAR_FILL_DEFAULT = "█"

MAX_BAR_WIDTH = 64  # total columns for the 1-127 range


def _bar(layer: dict) -> str:
    vel_range = layer["vel_end"] - layer["vel_start"] + 1
    width = max(1, round(vel_range / 127 * MAX_BAR_WIDTH))
    char = BAR_CHARS.get(layer["name"], BAR_FILL_DEFAULT)
    return char * width


def format_text(layers: list[dict], preset_label: str) -> str:
    lines = []
    lines.append(f"Velocity Curve: {preset_label}")
    lines.append("")
    lines.append("Velocity Map:")

    label_width = max(len(f"Layer {l['layer']} [{l['name']}]:") for l in layers)
    for layer in layers:
        label = f"Layer {layer['layer']} [{layer['name']}]:"
        bar = _bar(layer)
        range_str = f"({layer['vel_start']}-{layer['vel_end']})"
        lines.append(f"  {label:<{label_width}}  {bar}  {range_str}")

    lines.append("")
    lines.append("Layer Summary:")
    for layer in layers:
        width = layer["vel_end"] - layer["vel_start"] + 1
        pct = width / 127 * 100
        lines.append(
            f"  Layer {layer['layer']} [{layer['name']:7}]  "
            f"vel {layer['vel_start']:>3}-{layer['vel_end']:>3}  "
            f"width={width:>3}  ({pct:.1f}%)  vol={layer['volume_suggestion']}"
        )
    return "\n".join(lines)


def format_xml(layers: list[dict], preset_label: str) -> str:
    lines = []
    lines.append(f"<!-- Velocity curve: {preset_label} -->")
    lines.append("<!-- Paste inside each <KeyGroup> or <Layer> block in your .xpm -->")
    lines.append("")
    for layer in layers:
        lines.append(f"<!-- Layer {layer['layer']}: {layer['name']} ({layer['vel_start']}-{layer['vel_end']}) -->")
        lines.append(
            f'<Layer VelStart="{layer["vel_start"]}" VelEnd="{layer["vel_end"]}"'
            f' Volume="{layer["volume_suggestion"]}" Pan="0"/>'
        )
        lines.append("")
    return "\n".join(lines).rstrip()


def format_json(layers: list[dict], preset_label: str) -> str:
    payload = {
        "preset": preset_label,
        "layer_count": len(layers),
        "layers": layers,
    }
    return json.dumps(payload, indent=2)


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def main() -> None:
    parser = argparse.ArgumentParser(
        description="XPN Velocity Curve Designer — XO_OX Designs",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    parser.add_argument(
        "--preset",
        choices=list(PRESETS.keys()),
        default="vibe",
        help="Named velocity curve preset (default: vibe)",
    )
    parser.add_argument(
        "--boundaries",
        metavar="1,40,90,110,127",
        help="Custom boundary points, comma-separated. Overrides --preset.",
    )
    parser.add_argument(
        "--layers",
        type=int,
        choices=[2, 3, 4],
        help="Number of layers (informational; must match boundary count)",
    )
    parser.add_argument(
        "--format",
        choices=["text", "xml", "json"],
        default="text",
        help="Output format (default: text)",
    )
    args = parser.parse_args()

    # Resolve boundaries
    if args.boundaries:
        boundaries = parse_boundaries(args.boundaries)
        preset_label = f"custom ({args.boundaries})"
    else:
        boundaries = PRESETS[args.preset]
        preset_label = args.preset

    # Optional layer-count consistency check
    n_layers = len(boundaries) - 1
    if args.layers and args.layers != n_layers:
        sys.exit(
            f"Error: --layers {args.layers} does not match the {n_layers} layers "
            f"implied by the chosen boundaries/preset."
        )

    layers = boundaries_to_layers(boundaries)

    # Render
    if args.format == "xml":
        print(format_xml(layers, preset_label))
    elif args.format == "json":
        print(format_json(layers, preset_label))
    else:
        print(format_text(layers, preset_label))


if __name__ == "__main__":
    main()
