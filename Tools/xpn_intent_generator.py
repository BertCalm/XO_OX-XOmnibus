#!/usr/bin/env python3
"""
XPN Intent Generator — Create .xpn_intent JSON sidecar files for XPN packs.

PURPOSE:
    Generates the machine-readable design philosophy file that ships alongside
    every XPN pack. The .xpn_intent captures WHY a pack was designed the way
    it was — not just what samples it contains.

    Origin: Vision Quest 004 ("The Pipeline That Speaks First")

USAGE:
    # Generate intent from a render spec + pack metadata
    python xpn_intent_generator.py \
        --pack-name "XO_OX MPCe Percussion Vol. 1" \
        --engine ONSET \
        --corner-pattern dynamic_expression \
        --presets-dir ../Presets/XOmnibus/ \
        --output xpn_intent.json

    # Generate intent from an existing XPM program
    python xpn_intent_generator.py \
        --from-xpm Programs/MyKit_MPCe.xpm \
        --pack-name "My Kit" \
        --output xpn_intent.json

    # Generate minimal intent (just pack identity + intent summary)
    python xpn_intent_generator.py \
        --pack-name "My Pack" \
        --engine ONSET \
        --summary "Boom bap kit with vintage character" \
        --output xpn_intent.json

DEPENDENCIES:
    Python 3.8+, no external packages required.
"""

import argparse
import json
import os
import sys
from datetime import date
from pathlib import Path
from typing import Any, Dict, List, Optional


# ---------------------------------------------------------------------------
# Corner pattern templates
# ---------------------------------------------------------------------------

CORNER_PATTERNS = {
    "feliX_oscar_polarity": {
        "NW": {"role": "feliX_dry", "description": "Bright, precise, unprocessed",
               "sonic_character": "Clinical, clear, no reverb"},
        "NE": {"role": "feliX_wet", "description": "Bright, precise, effected",
               "sonic_character": "Clean reverb, modulation, spatial"},
        "SW": {"role": "oscar_dry", "description": "Dark, warm, unprocessed",
               "sonic_character": "Saturated, full low-mid, no reverb"},
        "SE": {"role": "oscar_wet", "description": "Dark, warm, effected",
               "sonic_character": "Tape saturation, organic space"}
    },
    "dynamic_expression": {
        "NW": {"role": "ghost_note", "description": "Quiet, muted, brush-adjacent",
               "sonic_character": "Low velocity, filtered, short decay"},
        "NE": {"role": "accent", "description": "Alternate strike — rimshot, edge hit",
               "sonic_character": "Bright, sharp attack, full body"},
        "SW": {"role": "standard", "description": "Center hit. Default voice.",
               "sonic_character": "Balanced, natural, full range"},
        "SE": {"role": "effect", "description": "Processed variant — flam, buzz, FX",
               "sonic_character": "Textured, layered, characterful"}
    },
    "era_corners": {
        "NW": {"role": "classic_analog", "description": "808 era, rounded, warm",
               "sonic_character": "Analog warmth, slow attack, deep sub"},
        "NE": {"role": "80s_digital", "description": "909/LinnDrum era, crisp",
               "sonic_character": "Digital clarity, open high-end"},
        "SW": {"role": "90s_gritty", "description": "SP-1200, compressed, bit-reduced",
               "sonic_character": "12-bit crunch, compressed dynamics"},
        "SE": {"role": "contemporary", "description": "Modern clean processing",
               "sonic_character": "Full range, pristine transients"}
    },
    "coupling_state": {
        "NW": {"role": "solo", "description": "Engine A alone, no coupling",
               "sonic_character": "Pure, unmodified engine character"},
        "NE": {"role": "light_coupling", "description": "Subtle cross-engine interaction",
               "sonic_character": "Gentle frequency locking, mild modulation"},
        "SW": {"role": "heavy_coupling", "description": "Deep entanglement",
               "sonic_character": "Complex cross-modulation, emergent timbres"},
        "SE": {"role": "full_coupling", "description": "Maximum interaction",
               "sonic_character": "Fully coupled — engines become one instrument"}
    },
    "instrument_articulation": {
        "NW": {"role": "legato", "description": "Smooth, connected playing",
               "sonic_character": "Soft attack, long sustain, gentle release"},
        "NE": {"role": "staccato", "description": "Short, detached notes",
               "sonic_character": "Quick attack, no sustain, immediate release"},
        "SW": {"role": "tremolo", "description": "Rapid repeated notes",
               "sonic_character": "Pulsing amplitude, rhythmic character"},
        "SE": {"role": "accent", "description": "Emphasized, forceful",
               "sonic_character": "Hard attack, peak dynamics, bright transient"}
    }
}

XY_DEFAULTS_DRUM = {
    "x_axis": {"target": "pan_or_reverb_send",
               "description": "Horizontal = spatial width"},
    "y_axis": {"target": "decay_or_release",
               "description": "Vertical = sustain length"}
}

XY_DEFAULTS_MELODIC = {
    "x_axis": {"target": "modulation_depth",
               "description": "Horizontal = vibrato/modulation"},
    "y_axis": {"target": "filter_cutoff",
               "description": "Vertical = brightness"}
}

GM_PAD_LAYOUT = [
    {"pad": "A01", "midi": 36, "gm": "Kick"},
    {"pad": "A02", "midi": 38, "gm": "Snare"},
    {"pad": "A03", "midi": 42, "gm": "Closed Hat"},
    {"pad": "A04", "midi": 46, "gm": "Open Hat"},
    {"pad": "A05", "midi": 39, "gm": "Clap"},
    {"pad": "A06", "midi": 45, "gm": "Low Tom"},
    {"pad": "A07", "midi": 37, "gm": "Rimshot"},
    {"pad": "A08", "midi": 49, "gm": "Crash"},
    {"pad": "A09", "midi": 51, "gm": "Ride"},
    {"pad": "A10", "midi": 47, "gm": "Mid Tom"},
    {"pad": "A11", "midi": 50, "gm": "High Tom"},
    {"pad": "A12", "midi": 43, "gm": "Floor Tom"},
    {"pad": "A13", "midi": 54, "gm": "Tambourine"},
    {"pad": "A14", "midi": 56, "gm": "Cowbell"},
    {"pad": "A15", "midi": 75, "gm": "Clave"},
    {"pad": "A16", "midi": 76, "gm": "Wood Block"},
]


# ---------------------------------------------------------------------------
# DNA range computation
# ---------------------------------------------------------------------------

def compute_dna_range(presets_dir: str, engine: str) -> Dict[str, Dict[str, float]]:
    """Scan .xometa files and compute min/max DNA across matching presets."""
    dims = ["brightness", "warmth", "movement", "density", "space", "aggression"]
    mins = {d: 1.0 for d in dims}
    maxs = {d: 0.0 for d in dims}
    count = 0

    presets_path = Path(presets_dir)
    for xometa in presets_path.rglob("*.xometa"):
        try:
            with open(xometa, "r") as f:
                data = json.load(f)
        except (json.JSONDecodeError, OSError):
            continue

        # Check engine match
        engines = data.get("engines", [])
        if not engines:
            eng = data.get("engine", "")
            engines = [eng] if eng else []

        if engine.upper() not in [e.upper() for e in engines]:
            continue

        dna = data.get("sonic_dna", data.get("dna", {}))
        if not dna:
            continue

        for dim in dims:
            val = dna.get(dim, None)
            if val is not None:
                mins[dim] = min(mins[dim], val)
                maxs[dim] = max(maxs[dim], val)
        count += 1

    if count == 0:
        return {d: {"min": 0.0, "max": 1.0} for d in dims}

    return {d: {"min": round(mins[d], 2), "max": round(maxs[d], 2)} for d in dims}


# ---------------------------------------------------------------------------
# Intent generation
# ---------------------------------------------------------------------------

def generate_intent(
    pack_name: str,
    engine: str,
    corner_pattern: str,
    summary: str = "",
    target_player: str = "",
    target_genre: Optional[List[str]] = None,
    presets_dir: str = "",
    mpce_native: bool = True,
    pad_count: int = 16,
) -> Dict[str, Any]:
    """Generate a complete .xpn_intent JSON structure."""

    pack_id = pack_name.lower().replace(" ", "-").replace("_", "-")
    pack_id = "".join(c for c in pack_id if c.isalnum() or c == "-")

    # Determine if drum or melodic
    is_drum = engine.upper() in ["ONSET", "OWARE", "OSTINATO"]
    xy_defaults = XY_DEFAULTS_DRUM if is_drum else XY_DEFAULTS_MELODIC

    # Get corner assignments
    corners = CORNER_PATTERNS.get(corner_pattern, CORNER_PATTERNS["dynamic_expression"])

    # Compute DNA range if presets dir provided
    if presets_dir and os.path.isdir(presets_dir):
        dna_range = compute_dna_range(presets_dir, engine)
    else:
        dna_range = {
            "brightness": {"min": 0.0, "max": 1.0},
            "warmth": {"min": 0.0, "max": 1.0},
            "movement": {"min": 0.0, "max": 1.0},
            "density": {"min": 0.0, "max": 1.0},
            "space": {"min": 0.0, "max": 1.0},
            "aggression": {"min": 0.0, "max": 1.0},
        }

    # Build experience arc
    if pad_count >= 16:
        experience_arc = {
            "description": "How the pads tell a story when played together.",
            "pad_groups": [
                {"pads": ["A01", "A02", "A03", "A04"],
                 "role": "core_kit",
                 "description": "Foundation instruments. The heartbeat."},
                {"pads": ["A05", "A06", "A07", "A08"],
                 "role": "color",
                 "description": "Accent instruments. Fills and punctuation."},
                {"pads": ["A09", "A10", "A11", "A12"],
                 "role": "texture",
                 "description": "Rhythmic detail. Shakers, bells, percussion."},
                {"pads": ["A13", "A14", "A15", "A16"],
                 "role": "atmosphere",
                 "description": "Space and transition. FX, sweeps, ambience."},
            ]
        }
    else:
        experience_arc = {
            "description": "Compact kit focused on core instruments.",
            "pad_groups": [
                {"pads": [f"A{i+1:02d}" for i in range(pad_count)],
                 "role": "core_kit",
                 "description": "All pads are primary instruments."}
            ]
        }

    # Velocity splits (Vibe's musical curve)
    velocity_splits = [
        {"layer": 1, "vel_start": 1, "vel_end": 20, "character": "ghost"},
        {"layer": 2, "vel_start": 21, "vel_end": 50, "character": "light"},
        {"layer": 3, "vel_start": 51, "vel_end": 90, "character": "medium"},
        {"layer": 4, "vel_start": 91, "vel_end": 127, "character": "hard"},
    ]

    total_samples = pad_count * 4 * 4  # pads * corners * velocity layers

    intent = {
        "$schema": "https://xo-ox.org/schemas/xpn-intent/v1.json",
        "schema_version": "1.0",

        "pack": {
            "name": pack_name,
            "id": pack_id,
            "version": "1.0.0",
            "author": "XO_OX Designs",
            "date": date.today().isoformat(),
            "engine": engine.upper(),
            "engine_family": [engine.upper()],
            "mpce_native": mpce_native,
        },

        "intent": {
            "summary": summary or f"{engine} pack designed for MPCe 3D pads with {corner_pattern.replace('_', ' ')} corner architecture.",
            "target_player": target_player or ("Live finger drummers and beat performers" if is_drum else "Keyboard players and melodic performers"),
            "target_genre": target_genre or ["hip-hop", "electronic", "neo-soul"],
            "design_philosophy": f"Each pad is a complete instrument with four {corner_pattern.replace('_', ' ')} corners. Designed for expressive performance.",
            "creative_direction": "",
            "not_for": "Step-sequenced programming where articulation variety isn't needed" if corner_pattern == "dynamic_expression" else "",
        },

        "pad_architecture": {
            "type": "mpce_quad_corner",
            "corner_pattern": corner_pattern,
            "corner_assignments": corners,
            "xy_modulation": xy_defaults,
            "z_axis": {
                "target": "filter_cutoff",
                "description": "Pressure after strike opens filter."
            },
        },

        "experience_arc": experience_arc,

        "sonic_dna_range": dna_range,

        "rendering": {
            "sample_rate": 44100,
            "bit_depth": 24,
            "velocity_layers": 4,
            "velocity_splits": velocity_splits,
            "corner_variants": 4,
            "total_samples_per_pad": 16,
            "total_samples": total_samples,
        },

        "compatibility": {
            "mpce_required": False,
            "mpce_enhanced": True,
            "standard_version_included": True,
            "minimum_firmware": "3.7",
            "hardware": ["MPC Live III", "MPC XL"],
            "fallback_hardware": ["MPC Live II", "MPC One", "MPC Key 61"],
            "fallback_description": "Standard version included for non-MPCe hardware.",
        },

        "provenance": {
            "xomnibus_version": "1.0",
            "source_presets": [],
            "quad_builder_version": "1.0",
            "render_spec": "",
            "vision_quest": "VQ 003 + VQ 004",
        }
    }

    return intent


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def main():
    parser = argparse.ArgumentParser(
        description="Generate .xpn_intent JSON sidecar for XPN packs",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Corner patterns:
  feliX_oscar_polarity    — XO_OX signature: bright/dark × dry/wet
  dynamic_expression      — Articulation: ghost/accent/standard/effect
  era_corners             — Time periods: 808/909/SP1200/modern
  coupling_state          — Coupling depth: solo/light/heavy/full
  instrument_articulation — Playing style: legato/staccato/tremolo/accent

Examples:
  python xpn_intent_generator.py \\
      --pack-name "XO_OX MPCe Percussion Vol. 1" \\
      --engine ONSET \\
      --corner-pattern dynamic_expression \\
      --presets-dir ../Presets/XOmnibus/ \\
      --output xpn_intent.json
        """
    )

    parser.add_argument("--pack-name", required=True, help="Pack display name")
    parser.add_argument("--engine", required=True, help="Primary engine (e.g., ONSET)")
    parser.add_argument("--corner-pattern", default="dynamic_expression",
                        choices=list(CORNER_PATTERNS.keys()),
                        help="Quad-corner architecture pattern")
    parser.add_argument("--summary", default="", help="One-line pack summary")
    parser.add_argument("--target-player", default="", help="Who this pack is for")
    parser.add_argument("--target-genre", nargs="+", default=None, help="Target genres")
    parser.add_argument("--presets-dir", default="", help="Path to .xometa presets (for DNA range)")
    parser.add_argument("--pad-count", type=int, default=16, choices=[4, 8, 16])
    parser.add_argument("--no-mpce", action="store_true", help="Mark as non-MPCe pack")
    parser.add_argument("--output", default="xpn_intent.json", help="Output file path")

    args = parser.parse_args()

    intent = generate_intent(
        pack_name=args.pack_name,
        engine=args.engine,
        corner_pattern=args.corner_pattern,
        summary=args.summary,
        target_player=args.target_player,
        target_genre=args.target_genre,
        presets_dir=args.presets_dir,
        mpce_native=not args.no_mpce,
        pad_count=args.pad_count,
    )

    output_path = Path(args.output)
    with open(output_path, "w") as f:
        json.dump(intent, f, indent=2)

    print(f"Generated .xpn_intent: {output_path}")
    print(f"  Pack: {intent['pack']['name']}")
    print(f"  Engine: {intent['pack']['engine']}")
    print(f"  Corner pattern: {intent['pad_architecture']['corner_pattern']}")
    print(f"  MPCe native: {intent['pack']['mpce_native']}")
    print(f"  Total samples needed: {intent['rendering']['total_samples']}")


if __name__ == "__main__":
    main()
