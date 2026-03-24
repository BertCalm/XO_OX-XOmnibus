#!/usr/bin/env python3
"""
xpn_adaptive_velocity.py — Auto-shapes velocity curves in XPM files based on
instrument classification.

Uses xpn_classify_instrument.py to analyze each WAV in a stems directory,
then rewrites <Instrument> velocity layer boundaries and <VelocityToFilter>
sensitivity values in the XPM XML.

CLI:
    python xpn_adaptive_velocity.py --xpm input.xpm --stems ./stems/ --output output.xpm

Dependency:
    xpn_classify_instrument.py  (must be in the same directory or on PYTHONPATH)
"""

import argparse
import os
import sys
import xml.etree.ElementTree as ET
from typing import Optional

# ---------------------------------------------------------------------------
# Import classifier
# ---------------------------------------------------------------------------
try:
    from xpn_classify_instrument import classify_wav
except ImportError:
    print("ERROR: Cannot import xpn_classify_instrument. "
          "Ensure xpn_classify_instrument.py is in the same directory or on PYTHONPATH.",
          file=sys.stderr)
    sys.exit(1)

# Optional: xpn_auto_dna for per-sample 6D DNA (enables Sonic DNA Velocity Sculpting)
try:
    from xpn_auto_dna import compute_dna as _compute_dna
    _DNA_AVAILABLE = True
except ImportError:
    _DNA_AVAILABLE = False


# ---------------------------------------------------------------------------
# Velocity curve definitions
# Boundaries lists define the VelStart of each layer (ascending).
# A 4-layer instrument gets 4 boundary points: [v0, v1, v2, v3]
# The last layer always ends at 127. The first always starts at 0.
# We re-distribute existing layers to match the target shape.
# ---------------------------------------------------------------------------

# Each entry: list of VelStart values for layers [L0, L1, L2, L3, L4...]
# These represent IDEAL 4-layer distributions (5 boundaries = 4 ranges).
VELOCITY_CURVES = {
    "kick":        [0, 40, 75, 100, 127],   # Heavy weighting toward upper layers
    "snare":       [0, 35, 65,  90, 127],   # Similar to kick, slightly more even
    "hihat":       [0, 32, 64,  96, 127],   # Even / equal distribution
    "cymbal":      [0, 32, 64,  96, 127],   # Treat like hihat
    "pad":         [0, 55, 85, 105, 127],   # Light weighting — most hits in 50-90
    "atmosphere":  [0, 55, 85, 105, 127],   # Same as pad
    "lead":        [0, 45, 75, 100, 127],   # Musical cluster at 60-100
    "melodic":     [0, 45, 75, 100, 127],   # Same as lead
    "bass":        [0, 40, 70,  95, 127],   # Low-end weighted
    "perc":        [0, 35, 65,  90, 127],   # Snare-like
    "fx":          [0, 32, 64,  96, 127],   # Even
    "unknown":     [0, 32, 64,  96, 127],   # Default equal division
}

# VelocityToFilter range: 0–127 in MPC XPM convention.
# feliX_oscar_bias is in [-1.0, +1.0]:
#   +1.0 (feliX/bright) → high filter sensitivity (e.g. 100)
#   -1.0 (Oscar/dark)   → low filter sensitivity (e.g. 10)
FILTER_SENSITIVITY_BASE = 55     # neutral bias maps to this
FILTER_SENSITIVITY_RANGE = 45    # ± this many units from base


def dna_to_velocity_curve(dna: dict, instrument_type: str, n_layers: int = 4) -> list:
    """Morph base velocity curve using 6D Sonic DNA.

    Legend Feature #1 — velocity curves shaped by the actual sonic character
    of each sample, not just its instrument category.

    Dark+warm sounds: compress lower layers, expand upper (hit harder to open up)
    Bright+aggressive sounds: expand lower layers (already present at soft hits)

    Args:
        dna: 6D Sonic DNA dict with keys brightness, warmth, aggression, etc.
        instrument_type: instrument category string (kick/snare/hihat etc.)
        n_layers: number of velocity layers in the XPM instrument (default 4)

    Returns:
        List of VelStart boundary values, length == n_layers, same contract as
        VELOCITY_CURVES entries.  First element is always 0, last always 127.
    """
    base = VELOCITY_CURVES.get(instrument_type, VELOCITY_CURVES.get("unknown", [0, 32, 64, 96, 127]))
    brightness = dna.get("brightness", 0.5)
    aggression = dna.get("aggression", 0.5)
    warmth = dna.get("warmth", 0.5)

    # Compression factor: positive = compress lower layers (need to hit harder)
    compression = (1.0 - brightness) * 0.4 + warmth * 0.2 - aggression * 0.3
    compression = max(-0.5, min(0.5, compression))

    # Shift midpoint layers by compression factor
    shifted = [base[0]]
    for i in range(1, len(base) - 1):
        offset = int(compression * (64 - base[i]) * 0.5)
        shifted.append(max(1, min(126, base[i] + offset)))
    shifted.append(base[-1])
    return shifted


def bias_to_velocity_filter(bias: float) -> int:
    """Map feliX_oscar_bias [-1, +1] to a VelocityToFilter value [10, 100]."""
    raw = FILTER_SENSITIVITY_BASE + int(bias * FILTER_SENSITIVITY_RANGE)
    return max(10, min(100, raw))


# ---------------------------------------------------------------------------
# Velocity boundary interpolation
# ---------------------------------------------------------------------------

def interpolate_boundaries(source_boundaries: list, target_5pt: list, n_layers: int) -> list:
    """
    Given a target 5-point curve (for a 4-layer instrument) and the actual
    number of layers, return a list of n_layers VelStart values (ascending,
    starting at 0, last layer ending at 127).

    Strategy: linearly rescale the target shape to fit n_layers.
    For fewer layers we sample the target at evenly-spaced positions.
    For more layers we interpolate between the target points.
    """
    if n_layers <= 0:
        return []
    if n_layers == 1:
        return [0]

    # Normalise target to [0.0, 1.0] scale (drop last value which is always 127)
    # target_5pt has 5 entries for 4 layers: [start0, start1, start2, start3, 127]
    # The "starts" are the first 4 entries.
    starts = target_5pt[:-1]  # [s0, s1, s2, s3]
    # Normalise to 0..1
    normed = [s / 127.0 for s in starts]

    # We need n_layers start values. Resample normed to n_layers points.
    result_normed = []
    for i in range(n_layers):
        # Position in the 4-slot reference grid
        t = i / (n_layers - 1) * (len(normed) - 1)
        lo = int(t)
        hi = min(lo + 1, len(normed) - 1)
        frac = t - lo
        val = normed[lo] * (1.0 - frac) + normed[hi] * frac
        result_normed.append(val)

    # Convert back to 0..127 integer range, ensuring ascending + unique
    result = [int(round(v * 127)) for v in result_normed]

    # Clamp and enforce strictly ascending with min gap of 1
    result[0] = 0
    for i in range(1, len(result)):
        result[i] = max(result[i - 1] + 1, result[i])
    # Last layer must fit within 127
    result[-1] = min(result[-1], 127 - (n_layers - 1 - (len(result) - 1)))

    return result


# ---------------------------------------------------------------------------
# WAV file discovery
# ---------------------------------------------------------------------------

def find_wav_in_stems(stems_dir: str, wav_filename: str) -> Optional[str]:
    """
    Search for wav_filename (basename) inside stems_dir recursively.
    Returns absolute path if found, None otherwise.
    """
    basename = os.path.basename(wav_filename)
    for root, _dirs, files in os.walk(stems_dir):
        for f in files:
            if f.lower() == basename.lower():
                return os.path.join(root, f)
    # Try exact path if it's already absolute/relative
    if os.path.isfile(wav_filename):
        return wav_filename
    # Try joining stems_dir + filename directly
    direct = os.path.join(stems_dir, basename)
    if os.path.isfile(direct):
        return direct
    return None


# ---------------------------------------------------------------------------
# XPM parsing helpers
# ---------------------------------------------------------------------------

def get_instrument_wav(instrument_el: ET.Element) -> Optional[str]:
    """Return the first SampleFile value found in an Instrument element."""
    # MPC XPM structure: Instrument > Layer > SampleFile
    for layer in instrument_el.findall(".//Layer"):
        sf = layer.find("SampleFile")
        if sf is not None and sf.text:
            return sf.text.strip()
    return None


def get_instrument_layers(instrument_el: ET.Element) -> list:
    """Return all Layer elements inside an Instrument (only non-empty ones with SampleFile)."""
    layers = []
    for layer in instrument_el.findall("Layer"):
        sf = layer.find("SampleFile")
        if sf is not None and sf.text and sf.text.strip():
            layers.append(layer)
    return layers


def set_layer_velocity_bounds(layers: list, vel_starts: list):
    """
    Apply vel_starts to layers. VelEnd of layer[i] = VelStart of layer[i+1] - 1.
    Last layer VelEnd = 127.
    """
    n = len(layers)
    for i, layer in enumerate(layers):
        vs = vel_starts[i]
        ve = (vel_starts[i + 1] - 1) if (i + 1 < n) else 127

        vel_start_el = layer.find("VelStart")
        vel_end_el = layer.find("VelEnd")

        if vel_start_el is None:
            vel_start_el = ET.SubElement(layer, "VelStart")
        if vel_end_el is None:
            vel_end_el = ET.SubElement(layer, "VelEnd")

        vel_start_el.text = str(vs)
        vel_end_el.text = str(ve)


def set_velocity_to_filter(instrument_el: ET.Element, value: int):
    """Set or create VelocityToFilter on the Instrument element."""
    vtf = instrument_el.find("VelocityToFilter")
    if vtf is None:
        vtf = ET.SubElement(instrument_el, "VelocityToFilter")
    vtf.text = str(value)


# ---------------------------------------------------------------------------
# Pretty-print indent (Python < 3.9 compat)
# ---------------------------------------------------------------------------

def indent_tree(elem: ET.Element, level: int = 0, indent: str = "  "):
    """Add pretty-print indentation to an ElementTree in-place."""
    pad = "\n" + level * indent
    child_pad = "\n" + (level + 1) * indent
    if len(elem):
        if not elem.text or not elem.text.strip():
            elem.text = child_pad
        if not elem.tail or not elem.tail.strip():
            elem.tail = pad
        for child in elem:
            indent_tree(child, level + 1, indent)
        # Last child tail
        if not child.tail or not child.tail.strip():
            child.tail = pad
    else:
        if level and (not elem.tail or not elem.tail.strip()):
            elem.tail = pad
    if not level:
        elem.tail = "\n"


# ---------------------------------------------------------------------------
# Main processing
# ---------------------------------------------------------------------------

def process_xpm(xpm_path: str, stems_dir: str, output_path: str):
    if not os.path.isfile(xpm_path):
        print(f"ERROR: XPM file not found: {xpm_path}", file=sys.stderr)
        sys.exit(1)

    if not os.path.isdir(stems_dir):
        print(f"ERROR: Stems directory not found: {stems_dir}", file=sys.stderr)
        sys.exit(1)

    tree = ET.parse(xpm_path)
    root = tree.getroot()

    instruments = root.findall(".//Instrument")
    if not instruments:
        print("WARNING: No <Instrument> blocks found in XPM.", file=sys.stderr)

    print(f"\n{'─'*72}")
    print(f"  xpn_adaptive_velocity — processing {len(instruments)} instrument(s)")
    print(f"  XPM   : {xpm_path}")
    print(f"  Stems : {stems_dir}")
    print(f"  Output: {output_path}")
    print(f"{'─'*72}\n")

    dna_mode = "DNA" if _DNA_AVAILABLE else "static"
    print(f"  Velocity sculpting: {dna_mode}")
    print()

    # Column header
    print(f"  {'#':<4} {'File':<30} {'Category':<14} {'Conf':>5}  {'Curve':<10} {'Filter':>6} {'DNA':>5}")
    print(f"  {'─'*4} {'─'*30} {'─'*14} {'─'*5}  {'─'*10} {'─'*6} {'─'*5}")

    for idx, instrument in enumerate(instruments):
        wav_ref = get_instrument_wav(instrument)
        if not wav_ref:
            print(f"  {idx+1:<4} {'(no SampleFile)':<30} {'—':<14} {'—':>5}  {'—':<10} {'—':>6} {'—':>5}")
            continue

        wav_path = find_wav_in_stems(stems_dir, wav_ref)

        sample_dna: Optional[dict] = None
        if not wav_path:
            print(f"  {idx+1:<4} {os.path.basename(wav_ref):<30} {'NOT FOUND':<14} {'—':>5}  {'default':<10} {'55':>6} {'—':>5}")
            category = "unknown"
            confidence = 0.0
            bias = 0.0
        else:
            result = classify_wav(wav_path)
            category = result.get("category", "unknown")
            confidence = result.get("confidence", 0.0)
            bias = result.get("feliX_oscar_bias", 0.0)

            # Legend Feature #1: compute per-sample 6D Sonic DNA for velocity sculpting
            if _DNA_AVAILABLE:
                try:
                    sample_dna = _compute_dna(wav_path)
                except Exception:
                    sample_dna = None

        # Resolve curve key (collapse aliases)
        curve_key = category if category in VELOCITY_CURVES else "unknown"

        # Use DNA-morphed curve when DNA is available; fall back to static lookup
        if sample_dna is not None:
            target_curve = dna_to_velocity_curve(sample_dna, curve_key)
        else:
            target_curve = VELOCITY_CURVES[curve_key]

        # Get active layers
        layers = get_instrument_layers(instrument)
        n_layers = len(layers)

        if n_layers > 0:
            # Derive existing boundaries (for reference — we replace them)
            existing_starts = []
            for layer in layers:
                vs_el = layer.find("VelStart")
                existing_starts.append(int(vs_el.text) if vs_el is not None and vs_el.text else 0)

            new_starts = interpolate_boundaries(existing_starts, target_curve, n_layers)
            set_layer_velocity_bounds(layers, new_starts)

        # Apply VelocityToFilter
        filter_val = bias_to_velocity_filter(bias)
        set_velocity_to_filter(instrument, filter_val)

        # Summary row
        short_name = os.path.basename(wav_ref)
        if len(short_name) > 29:
            short_name = short_name[:26] + "..."
        curve_label = curve_key if n_layers > 0 else "no layers"
        dna_tag = "yes" if sample_dna is not None else "no"
        print(f"  {idx+1:<4} {short_name:<30} {category:<14} {confidence:>4.0%}  {curve_label:<10} {filter_val:>6} {dna_tag:>5}")

    print(f"\n{'─'*72}")

    # Write output
    indent_tree(root)
    tree.write(output_path, encoding="unicode", xml_declaration=True)
    print(f"\n  Written → {output_path}\n")


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def main():
    parser = argparse.ArgumentParser(
        description="Auto-shape XPM velocity curves using instrument classification.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  python xpn_adaptive_velocity.py --xpm kit.xpm --stems ./stems/ --output kit_shaped.xpm
  python xpn_adaptive_velocity.py --xpm kit.xpm --stems /audio/samples/ --output out.xpm

Velocity curve presets applied per category:
  kick       [0, 40, 75, 100, 127]  — heavy upper weighting
  snare      [0, 35, 65,  90, 127]  — slightly more even than kick
  hihat      [0, 32, 64,  96, 127]  — even distribution
  cymbal     [0, 32, 64,  96, 127]  — even distribution
  pad        [0, 55, 85, 105, 127]  — light weighting, cluster at 50-90
  lead       [0, 45, 75, 100, 127]  — musical cluster at 60-100
  bass       [0, 40, 70,  95, 127]  — low-end weighted
  perc       [0, 35, 65,  90, 127]  — snare-like
  default    [0, 32, 64,  96, 127]  — equal division
        """,
    )
    parser.add_argument("--xpm",    required=True, help="Input XPM file path")
    parser.add_argument("--stems",  required=True, help="Directory containing WAV stems")
    parser.add_argument("--output", required=True, help="Output XPM file path")

    args = parser.parse_args()
    process_xpm(
        xpm_path=os.path.abspath(args.xpm),
        stems_dir=os.path.abspath(args.stems),
        output_path=os.path.abspath(args.output),
    )


if __name__ == "__main__":
    main()
