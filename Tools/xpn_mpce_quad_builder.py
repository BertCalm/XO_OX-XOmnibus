#!/usr/bin/env python3
"""
MPCe Quad-Corner Builder — XOmnibus Engine Preset → 3D Pad Program Generator

PURPOSE:
    Build MPCe-native programs that use the quad-corner 3D pad system
    (X/Y/Z per pad) on MPC Live III and MPC XL. Each pad gets 4 presets
    assigned to its corners (NW / NE / SW / SE). Sliding a finger across
    the pad crossfades between the four corner presets in real time.

    This tool takes an XOmnibus engine name and a directory of .xometa
    preset files, then:

    1. Reads each preset's 6D Sonic DNA and macro defaults.
    2. Groups presets into sets of 4 per pad.
    3. Assigns corners so that OPPOSITE corners (NW↔SE, NE↔SW) are
       maximally different in DNA space — giving the widest morph range.
    4. Generates an XPM file with speculative <PadCornerAssignment>
       metadata for MPCe-native corner morphing.

CORNER LAYOUT (looking at the pad from above):

    NW ──── NE        X axis (left→right): CHARACTER morph
    │        │        Y axis (top→bottom): MOVEMENT morph
    │  pad   │        Z axis (pressure):   Expression / filter
    │        │
    SW ──── SE

    Opposite corners = max DNA distance:
      NW ↔ SE  (diagonal 1 — should be maximally different)
      NE ↔ SW  (diagonal 2 — should be maximally different)

SPECULATIVE FORMAT:
    The <PadCornerAssignment> XML schema is our best guess for how Akai
    will implement 3D pad preset morphing. The format is NOT finalized.
    Use --speculative (default: on) to include a warning in the output.
    When Akai publishes the official spec, this tool will be updated.

USAGE:
    python xpn_mpce_quad_builder.py \\
        --engine OPAL \\
        --presets-dir ../Presets/XOmnibus/opal/ \\
        --output-dir ./output/ \\
        --pad-count 16

    python xpn_mpce_quad_builder.py \\
        --engine OVERLAP \\
        --presets-dir ../Presets/XOmnibus/overlap/ \\
        --output-dir ./output/ \\
        --pad-count 4 \\
        --no-speculative

DEPENDENCIES:
    Python 3.8+, no external packages required.
"""

import argparse
import json
import math
import os
import sys
import xml.etree.ElementTree as ET
from dataclasses import dataclass, field
from itertools import combinations
from pathlib import Path
from typing import Dict, List, Optional, Tuple


# ---------------------------------------------------------------------------
# Data structures
# ---------------------------------------------------------------------------

@dataclass
class PresetInfo:
    """Metadata extracted from a single .xometa preset file."""
    name: str
    filepath: str
    engine: str = ""

    # 6D Sonic DNA (0.0-1.0 each)
    brightness: float = 0.5
    warmth: float = 0.5
    movement: float = 0.5
    density: float = 0.5
    space: float = 0.5
    aggression: float = 0.5

    # Macro defaults (0.0-1.0)
    character: float = 0.5
    movement_macro: float = 0.5
    coupling: float = 0.5
    space_macro: float = 0.5

    mood: str = ""
    tags: List[str] = field(default_factory=list)

    @property
    def dna_vector(self) -> Tuple[float, float, float, float, float, float]:
        """6D vector representing the preset's sonic identity."""
        return (
            self.brightness, self.warmth, self.movement,
            self.density, self.space, self.aggression,
        )


@dataclass
class CornerAssignment:
    """Four presets assigned to a single pad's corners."""
    pad_index: int  # 0-based
    nw: PresetInfo
    ne: PresetInfo
    sw: PresetInfo
    se: PresetInfo


# ---------------------------------------------------------------------------
# DNA distance
# ---------------------------------------------------------------------------

def dna_distance(a: PresetInfo, b: PresetInfo) -> float:
    """
    Euclidean distance in 6D Sonic DNA space.
    Higher = more different = better for opposite corners.

    The 6 dimensions are: brightness, warmth, movement, density, space,
    aggression. All are normalized 0.0-1.0, so max distance is sqrt(6).
    """
    va, vb = a.dna_vector, b.dna_vector
    return math.sqrt(sum((ai - bi) ** 2 for ai, bi in zip(va, vb)))


# ---------------------------------------------------------------------------
# Preset loading
# ---------------------------------------------------------------------------

def load_presets(presets_dir: str, engine: str) -> List[PresetInfo]:
    """
    Scan presets_dir for .xometa preset files belonging to the given engine.

    Each .xometa file is a JSON object with fields like:
        name, engine/engines, dna {brightness, warmth, ...},
        macros {character, movement, coupling, space},
        mood, tags

    Presets whose engine field doesn't match the requested engine are skipped.
    """
    presets = []
    presets_path = Path(presets_dir)

    if not presets_path.exists():
        print(f"Warning: presets directory '{presets_dir}' not found.",
              file=sys.stderr)
        return presets

    # Collect .xometa and .json files
    candidates = sorted(
        p for p in presets_path.rglob("*")
        if p.suffix in (".xometa", ".json") and p.is_file()
    )

    for fp in candidates:
        try:
            with open(fp, "r", encoding="utf-8") as f:
                data = json.load(f)
        except (json.JSONDecodeError, UnicodeDecodeError):
            continue

        # Check engine match (flexible: engines array or engine string)
        preset_engines = data.get("engines", [])
        preset_engine = data.get("engine", "")
        if isinstance(preset_engines, list):
            engine_match = engine.upper() in [e.upper() for e in preset_engines]
        else:
            engine_match = False
        if not engine_match:
            engine_match = preset_engine.upper() == engine.upper()
        if not engine_match and preset_engines == [] and preset_engine == "":
            # No engine metadata — include it (might be engine-agnostic)
            engine_match = True
        if not engine_match:
            continue

        # Extract DNA
        dna = data.get("dna", {})
        macros = data.get("macros", {})

        presets.append(PresetInfo(
            name=data.get("name", fp.stem),
            filepath=str(fp),
            engine=engine,
            brightness=float(dna.get("brightness", 0.5)),
            warmth=float(dna.get("warmth", 0.5)),
            movement=float(dna.get("movement", 0.5)),
            density=float(dna.get("density", 0.5)),
            space=float(dna.get("space", 0.5)),
            aggression=float(dna.get("aggression", 0.5)),
            character=float(macros.get("character", 0.5)),
            movement_macro=float(macros.get("movement", 0.5)),
            coupling=float(macros.get("coupling", 0.5)),
            space_macro=float(macros.get("space", 0.5)),
            mood=data.get("mood", ""),
            tags=data.get("tags", []),
        ))

    return presets


# ---------------------------------------------------------------------------
# Corner assignment algorithm
# ---------------------------------------------------------------------------

def _find_max_distance_pair(group: List[PresetInfo]) -> Tuple[int, int]:
    """Find the pair of indices with the greatest DNA distance."""
    best_dist = -1.0
    best_pair = (0, 1)
    for i, j in combinations(range(len(group)), 2):
        d = dna_distance(group[i], group[j])
        if d > best_dist:
            best_dist = d
            best_pair = (i, j)
    return best_pair


def assign_corners(presets: List[PresetInfo],
                   pad_count: int) -> List[CornerAssignment]:
    """
    Assign presets to pad corners using DNA distance maximization.

    Algorithm:
    1. Need 4 presets per pad. Total needed = pad_count * 4.
       If fewer presets are available, pads are filled round-robin with
       repeats. If more are available, the first pad_count * 4 are used
       (sorted by DNA diversity — most spread-out presets first).

    2. For each group of 4:
       a. Find the pair with the greatest DNA distance.
          Assign them to the NW/SE diagonal (opposite corners).
       b. The remaining two go to the NE/SW diagonal.
       c. Among the NE/SW pair, maximize their distance too.

    This ensures the widest morph range on each pad's X/Y surface.
    Opposite corners are always maximally different.
    """
    assignments = []

    if len(presets) == 0:
        print("Error: no presets found.", file=sys.stderr)
        return assignments

    needed = pad_count * 4

    # If we have more presets than needed, pick the most diverse subset.
    # Simple greedy: sort by distance from the centroid (most extreme first).
    if len(presets) > needed:
        centroid = tuple(
            sum(p.dna_vector[d] for p in presets) / len(presets)
            for d in range(6)
        )
        # Create a dummy preset at the centroid for distance measurement
        centroid_preset = PresetInfo(name="__centroid__", filepath="")
        (centroid_preset.brightness, centroid_preset.warmth,
         centroid_preset.movement, centroid_preset.density,
         centroid_preset.space, centroid_preset.aggression) = centroid

        presets_ranked = sorted(
            presets,
            key=lambda p: dna_distance(p, centroid_preset),
            reverse=True,
        )
        pool = presets_ranked[:needed]
    else:
        # Pad with repeats if not enough
        pool = presets * ((needed // len(presets)) + 1)
        pool = pool[:needed]

    for pad_idx in range(pad_count):
        group = pool[pad_idx * 4 : (pad_idx + 1) * 4]

        # Find the most different pair -> NW and SE (main diagonal)
        nw_idx, se_idx = _find_max_distance_pair(group)

        # Remaining two -> NE and SW (secondary diagonal)
        others = [k for k in range(4) if k not in (nw_idx, se_idx)]

        # Among the other two, also maximize: the one more similar to NW
        # goes to NE (adjacent to NW on X axis), the other to SW.
        if len(others) == 2:
            d0_to_nw = dna_distance(group[others[0]], group[nw_idx])
            d1_to_nw = dna_distance(group[others[1]], group[nw_idx])
            if d0_to_nw < d1_to_nw:
                ne_idx, sw_idx = others[0], others[1]
            else:
                ne_idx, sw_idx = others[1], others[0]
        else:
            ne_idx, sw_idx = others[0], others[0]

        assignments.append(CornerAssignment(
            pad_index=pad_idx,
            nw=group[nw_idx],
            ne=group[ne_idx],
            sw=group[sw_idx],
            se=group[se_idx],
        ))

    return assignments


# ---------------------------------------------------------------------------
# XPM generation (speculative MPCe XML schema)
# ---------------------------------------------------------------------------

def generate_xpm(
    engine: str,
    assignments: List[CornerAssignment],
    output_dir: str,
    speculative: bool = True,
) -> str:
    """
    Generate an XPM file with quad-corner pad preset assignments.

    SPECULATIVE TAGS:
    The following XML elements are NOT part of any published Akai schema.
    They represent our best guess for how MPCe 3D pad morphing will work:

        <PadCornerAssignment>     — container for one pad's 4 corners
        <Corner position="NW">   — one corner assignment
        <PresetRef>               — reference to the engine preset
        <MacroSnapshot>           — macro values frozen at this corner
        <DNAVector>               — 6D sonic DNA for interpolation
        <PadAxisRouting>          — X/Y/Z axis mapping
        <CornerInterpolation>     — morphing algorithm hint

    When Akai publishes the real format, replace these stubs with the
    official tags. The corner assignment ALGORITHM (DNA distance
    maximization) will remain the same regardless of XML format.

    Returns the path to the written file.
    """
    root = ET.Element("MPCVObject")
    root.set("Version", "2.0")

    if speculative:
        root.append(ET.Comment(
            " SPECULATIVE FORMAT: MPCe quad-corner preset morphing schema "
            "is not finalized by Akai. Do not ship until official spec is "
            "published. Generated by xpn_mpce_quad_builder.py — XO_OX Designs. "
        ))

    program = ET.SubElement(root, "Program")
    ET.SubElement(program, "Name").text = f"{engine} Quad-Corner Program"
    ET.SubElement(program, "Engine").text = engine
    ET.SubElement(program, "Type").text = "MPCe-QuadCorner"
    ET.SubElement(program, "PadCount").text = str(len(assignments))

    # [SPECULATIVE] Global axis routing
    routing = ET.SubElement(program, "PadAxisRouting")
    routing.append(ET.Comment(" X/Y control corner crossfade, Z is expression "))
    ET.SubElement(routing, "XAxis").text = "CornerCrossfade_X"
    ET.SubElement(routing, "YAxis").text = "CornerCrossfade_Y"
    ET.SubElement(routing, "ZAxis").text = "Expression"

    # [SPECULATIVE] Interpolation mode
    interp = ET.SubElement(program, "CornerInterpolation")
    ET.SubElement(interp, "Mode").text = "bilinear"
    interp.append(ET.Comment(
        " bilinear: standard 2D lerp between 4 corners. "
        "Other candidates: spherical, weighted-nearest, dna-guided "
    ))

    # Pad assignments
    pads_el = ET.SubElement(program, "Pads")

    for assignment in assignments:
        pad_el = ET.SubElement(pads_el, "Pad")
        pad_el.set("index", str(assignment.pad_index))
        ET.SubElement(pad_el, "MidiNote").text = str(37 + assignment.pad_index)

        # [SPECULATIVE] Corner assignments
        corners_el = ET.SubElement(pad_el, "PadCornerAssignment")

        for corner_name, preset in [
            ("NW", assignment.nw),
            ("NE", assignment.ne),
            ("SW", assignment.sw),
            ("SE", assignment.se),
        ]:
            corner_el = ET.SubElement(corners_el, "Corner")
            corner_el.set("position", corner_name)

            ET.SubElement(corner_el, "PresetRef").text = preset.name
            ET.SubElement(corner_el, "PresetFile").text = (
                os.path.basename(preset.filepath)
            )

            # Macro snapshot at this corner
            macros_el = ET.SubElement(corner_el, "MacroSnapshot")
            ET.SubElement(macros_el, "CHARACTER").text = f"{preset.character:.3f}"
            ET.SubElement(macros_el, "MOVEMENT").text = (
                f"{preset.movement_macro:.3f}"
            )
            ET.SubElement(macros_el, "COUPLING").text = f"{preset.coupling:.3f}"
            ET.SubElement(macros_el, "SPACE").text = f"{preset.space_macro:.3f}"

            # DNA vector for interpolation engine
            dna_el = ET.SubElement(corner_el, "DNAVector")
            ET.SubElement(dna_el, "brightness").text = (
                f"{preset.brightness:.3f}"
            )
            ET.SubElement(dna_el, "warmth").text = f"{preset.warmth:.3f}"
            ET.SubElement(dna_el, "movement").text = f"{preset.movement:.3f}"
            ET.SubElement(dna_el, "density").text = f"{preset.density:.3f}"
            ET.SubElement(dna_el, "space").text = f"{preset.space:.3f}"
            ET.SubElement(dna_el, "aggression").text = (
                f"{preset.aggression:.3f}"
            )

        # Distance summary for this pad
        diag1 = dna_distance(assignment.nw, assignment.se)
        diag2 = dna_distance(assignment.ne, assignment.sw)
        pad_el.append(ET.Comment(
            f" Diagonal distances: NW-SE={diag1:.3f}  NE-SW={diag2:.3f} "
        ))

    # Write output
    output_path = Path(output_dir)
    output_path.mkdir(parents=True, exist_ok=True)
    filename = f"{engine}_QuadCorner.xpm"
    filepath = output_path / filename

    tree = ET.ElementTree(root)
    ET.indent(tree, space="  ")
    tree.write(str(filepath), encoding="unicode", xml_declaration=True)

    return str(filepath)


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def main():
    parser = argparse.ArgumentParser(
        description=(
            "MPCe Quad-Corner Builder: generate XPM programs that assign "
            "4 engine presets to each pad's quad corners (NW/NE/SW/SE) "
            "for 3D pad morphing on MPC Live III and MPC XL. Opposite "
            "corners are chosen to be maximally different in 6D Sonic DNA "
            "space, giving the widest morph range per pad."
        ),
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    parser.add_argument(
        "--engine",
        required=True,
        help="XOmnibus engine name (e.g. OPAL, OVERLAP, ONSET, OCEANIC)",
    )
    parser.add_argument(
        "--presets-dir",
        required=True,
        help="Directory containing .xometa preset files for the engine",
    )
    parser.add_argument(
        "--output-dir",
        default="./output",
        help="Directory to write the generated XPM (default: ./output)",
    )
    parser.add_argument(
        "--pad-count",
        type=int,
        default=16,
        choices=[4, 8, 16],
        help="Number of pads to assign (default: 16, needs 4 presets each)",
    )
    parser.add_argument(
        "--speculative",
        action="store_true",
        default=True,
        help=(
            "Mark output as speculative format (default: on). The MPCe "
            "quad-corner XPM schema is not finalized by Akai. Adds a "
            "warning comment to the output XML."
        ),
    )
    parser.add_argument(
        "--no-speculative",
        action="store_false",
        dest="speculative",
        help=(
            "Omit the speculative warning. Use only if the official Akai "
            "schema has been confirmed and this tool has been updated."
        ),
    )

    args = parser.parse_args()

    # Load presets
    print(f"Loading {args.engine} presets from {args.presets_dir}...")
    presets = load_presets(args.presets_dir, args.engine)

    if not presets:
        print(
            f"No presets found for engine '{args.engine}' in "
            f"'{args.presets_dir}'. Check the directory and engine name.",
            file=sys.stderr,
        )
        sys.exit(1)

    needed = args.pad_count * 4
    print(f"Found {len(presets)} presets. Need {needed} for {args.pad_count} "
          f"pads (4 per pad).")
    if len(presets) < needed:
        print(f"Note: only {len(presets)} presets available — some will "
              f"repeat across pads.")

    # Assign corners by DNA distance
    print("Assigning corners by DNA distance maximization...")
    assignments = assign_corners(presets, args.pad_count)

    # Generate XPM
    print("Generating XPM...")
    output_file = generate_xpm(
        engine=args.engine,
        assignments=assignments,
        output_dir=args.output_dir,
        speculative=args.speculative,
    )

    print(f"\nOutput: {output_file}")
    if args.speculative:
        print("Note: output uses SPECULATIVE MPCe format (--speculative).")

    # Summary table
    print(f"\n{'Pad':>4}  {'NW (top-left)':20s}  {'NE (top-right)':20s}  "
          f"{'SW (bot-left)':20s}  {'SE (bot-right)':20s}  "
          f"{'NW-SE dist':>10s}")
    print("-" * 102)
    for a in assignments:
        dist = dna_distance(a.nw, a.se)
        print(
            f"{a.pad_index:4d}  "
            f"{a.nw.name:20s}  {a.ne.name:20s}  "
            f"{a.sw.name:20s}  {a.se.name:20s}  "
            f"{dist:10.3f}"
        )


if __name__ == "__main__":
    main()
