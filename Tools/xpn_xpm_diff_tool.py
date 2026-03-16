#!/usr/bin/env python3
"""
XPN XPM Diff Tool — XO_OX Designs
Diffs two MPC-compatible XPM program files and shows what changed.
Useful for tracking program evolution across pack versions.

Usage:
    python xpn_xpm_diff_tool.py <old.xpm> <new.xpm> [--brief] [--output diff.txt]
"""

import argparse
import sys
import xml.etree.ElementTree as ET
from pathlib import Path


# ---------------------------------------------------------------------------
# Parsing helpers
# ---------------------------------------------------------------------------

PROGRAM_METADATA_ATTRS = [
    "name", "type", "RootNote", "auditionEnabled", "auditionVolume",
]

INSTRUMENT_ATTRS = [
    "volume", "pan", "tune", "cutoff", "resonance", "filterEnable",
    "filterType", "attack", "decay", "sustain", "release",
    "velocityToLevel", "velocityToAttack", "velocityToFilter",
    "chokeGroup", "muteGroup", "oneShot", "polyphony",
    "ignoreBaseNote", "loopMode",
]

LAYER_ATTRS = [
    "SampleFile", "VelStart", "VelEnd", "SampleStart", "SampleEnd",
    "LoopStart", "LoopEnd", "Tune", "VolumeOffset", "PanOffset",
    "CycleType", "CycleGroup",
]

QLINK_ATTRS = [
    "assignType", "assignParam", "low", "high",
]


def parse_program(path: Path) -> dict:
    """Parse an XPM file into a structured dict."""
    tree = ET.parse(path)
    root = tree.getroot()

    # The root element IS the program element in MPC XPM format
    program_el = root if root.tag == "Program" else root.find("Program")
    if program_el is None:
        program_el = root

    data = {
        "metadata": {},
        "keygroups": [],
        "pad_note_map": {},
        "pad_group_map": {},
        "qlinks": [],
    }

    # --- Metadata ---
    for attr in PROGRAM_METADATA_ATTRS:
        val = program_el.get(attr)
        if val is not None:
            data["metadata"][attr] = val
    # Capture any extra top-level attributes
    for k, v in program_el.attrib.items():
        if k not in data["metadata"]:
            data["metadata"][k] = v

    # --- Keygroups ---
    for kg_el in program_el.findall("Keygroup"):
        kg = _parse_keygroup(kg_el)
        data["keygroups"].append(kg)

    # --- PadNoteMap ---
    pnm_el = program_el.find("PadNoteMap")
    if pnm_el is not None:
        for entry in pnm_el.findall("Entry"):
            pad = entry.get("pad") or entry.get("Pad")
            note = entry.get("note") or entry.get("Note")
            sample = entry.get("sample") or entry.get("Sample") or entry.get("SampleFile", "")
            if pad is not None:
                data["pad_note_map"][pad] = {"note": note, "sample": sample}

    # --- PadGroupMap ---
    pgm_el = program_el.find("PadGroupMap")
    if pgm_el is not None:
        for entry in pgm_el.findall("Entry"):
            pad = entry.get("pad") or entry.get("Pad")
            group = entry.get("group") or entry.get("Group", "")
            if pad is not None:
                data["pad_group_map"][pad] = group

    # --- Q-Links ---
    qlinks_el = program_el.find("QLinks")
    if qlinks_el is None:
        qlinks_el = program_el.find("Qlinks")
    if qlinks_el is not None:
        for idx, ql_el in enumerate(qlinks_el):
            ql = {"index": idx}
            for attr in QLINK_ATTRS:
                val = ql_el.get(attr)
                if val is not None:
                    ql[attr] = val
            ql.update(ql_el.attrib)
            data["qlinks"].append(ql)

    return data


def _parse_keygroup(kg_el: ET.Element) -> dict:
    kg = {
        "name": kg_el.get("name") or kg_el.get("Name", ""),
        "attrs": dict(kg_el.attrib),
        "layers": [],
        "instrument": {},
    }
    # Instrument sub-element
    instr_el = kg_el.find("Instrument")
    if instr_el is not None:
        for attr in INSTRUMENT_ATTRS:
            val = instr_el.get(attr)
            if val is not None:
                kg["instrument"][attr] = val
        kg["instrument"].update(instr_el.attrib)

    # Layers
    layers_el = kg_el.find("Layers")
    if layers_el is None:
        # Some formats put layers directly under keygroup
        layer_els = kg_el.findall("Layer")
    else:
        layer_els = layers_el.findall("Layer")

    for layer_el in layer_els:
        layer = {}
        for attr in LAYER_ATTRS:
            val = layer_el.get(attr)
            if val is not None:
                layer[attr] = val
        layer.update(layer_el.attrib)
        kg["layers"].append(layer)

    return kg


# ---------------------------------------------------------------------------
# Diff engine
# ---------------------------------------------------------------------------

class DiffResult:
    def __init__(self):
        self.lines: list[str] = []
        self.added = 0
        self.removed = 0
        self.modified = 0
        self.unchanged = 0

    def add(self, line: str):
        self.lines.append(line)

    def render(self, brief: bool = False) -> str:
        if brief:
            return self._summary()
        return "\n".join(self.lines) + "\n\n" + self._summary()

    def _summary(self) -> str:
        total = self.added + self.removed + self.modified + self.unchanged
        return (
            f"Summary: {self.added} keygroup(s) added, {self.removed} removed, "
            f"{self.modified} modified, {self.unchanged} unchanged "
            f"(of {total} total keygroups compared)"
        )


def diff_programs(old: dict, new: dict, old_path: str, new_path: str) -> DiffResult:
    result = DiffResult()
    result.add(f"--- {old_path}")
    result.add(f"+++ {new_path}")
    result.add("")

    # --- Metadata ---
    _diff_metadata(old["metadata"], new["metadata"], result)

    # --- Keygroups ---
    _diff_keygroups(old["keygroups"], new["keygroups"], result)

    # --- PadNoteMap ---
    if old["pad_note_map"] or new["pad_note_map"]:
        _diff_pad_map(old["pad_note_map"], new["pad_note_map"], result)

    # --- Q-Links ---
    if old["qlinks"] or new["qlinks"]:
        _diff_qlinks(old["qlinks"], new["qlinks"], result)

    return result


def _diff_metadata(old_meta: dict, new_meta: dict, result: DiffResult):
    all_keys = sorted(set(old_meta) | set(new_meta))
    header_printed = False
    for k in all_keys:
        ov = old_meta.get(k)
        nv = new_meta.get(k)
        if ov != nv:
            if not header_printed:
                result.add("## Program Metadata")
                header_printed = True
            if ov is None:
                result.add(f"  + {k}: {nv}")
            elif nv is None:
                result.add(f"  - {k}: {ov}")
            else:
                result.add(f"  ~ {k}: {ov!r} → {nv!r}")
    if header_printed:
        result.add("")


def _diff_keygroups(old_kgs: list, new_kgs: list, result: DiffResult):
    result.add("## Keygroups")

    # Build lookup by name for named keygroups; fall back to positional
    old_by_name = {kg["name"]: kg for kg in old_kgs if kg["name"]}
    new_by_name = {kg["name"]: kg for kg in new_kgs if kg["name"]}

    old_names = [kg["name"] for kg in old_kgs]
    new_names = [kg["name"] for kg in new_kgs]

    # Count old keygroups — named ones or positional
    if old_by_name and new_by_name:
        # Match by name
        all_names = list(dict.fromkeys(old_names + new_names))  # preserve order
        for name in all_names:
            in_old = name in old_by_name
            in_new = name in new_by_name
            if in_old and not in_new:
                result.add(f"  - Removed keygroup: {name!r}")
                result.removed += 1
            elif in_new and not in_old:
                result.add(f"  + Added keygroup: {name!r}")
                result.added += 1
            else:
                changes = _diff_single_keygroup(old_by_name[name], new_by_name[name])
                if changes:
                    result.add(f"  ~ Changed keygroup {name!r}:")
                    for c in changes:
                        result.add(f"      {c}")
                    result.modified += 1
                else:
                    result.unchanged += 1
    else:
        # Positional matching
        max_len = max(len(old_kgs), len(new_kgs))
        for i in range(max_len):
            if i >= len(old_kgs):
                label = new_kgs[i]["name"] or f"[index {i}]"
                result.add(f"  + Added keygroup: {label!r}")
                result.added += 1
            elif i >= len(new_kgs):
                label = old_kgs[i]["name"] or f"[index {i}]"
                result.add(f"  - Removed keygroup: {label!r}")
                result.removed += 1
            else:
                label = old_kgs[i]["name"] or f"[index {i}]"
                changes = _diff_single_keygroup(old_kgs[i], new_kgs[i])
                if changes:
                    result.add(f"  ~ Changed keygroup {label!r}:")
                    for c in changes:
                        result.add(f"      {c}")
                    result.modified += 1
                else:
                    result.unchanged += 1

    result.add("")


def _diff_single_keygroup(old_kg: dict, new_kg: dict) -> list[str]:
    changes = []

    # Top-level keygroup attributes (VelStart, VelEnd, etc.)
    all_attr_keys = sorted(set(old_kg["attrs"]) | set(new_kg["attrs"]) - {"name", "Name"})
    for k in all_attr_keys:
        ov = old_kg["attrs"].get(k)
        nv = new_kg["attrs"].get(k)
        if ov != nv:
            if ov is None:
                changes.append(f"+ {k}: {nv}")
            elif nv is None:
                changes.append(f"- {k}: {ov}")
            else:
                changes.append(f"~ {k}: {ov} → {nv}")

    # Instrument attributes
    all_instr_keys = sorted(set(old_kg["instrument"]) | set(new_kg["instrument"]))
    for k in all_instr_keys:
        ov = old_kg["instrument"].get(k)
        nv = new_kg["instrument"].get(k)
        if ov != nv:
            if ov is None:
                changes.append(f"+ instrument.{k}: {nv}")
            elif nv is None:
                changes.append(f"- instrument.{k}: {ov}")
            else:
                changes.append(f"~ instrument.{k}: {ov} → {nv}")

    # Layers
    old_layers = old_kg["layers"]
    new_layers = new_kg["layers"]
    n_layers = max(len(old_layers), len(new_layers))
    for li in range(n_layers):
        if li >= len(old_layers):
            sf = new_layers[li].get("SampleFile", "?")
            changes.append(f"+ layer[{li}] added: {sf!r}")
        elif li >= len(new_layers):
            sf = old_layers[li].get("SampleFile", "?")
            changes.append(f"- layer[{li}] removed: {sf!r}")
        else:
            all_layer_keys = sorted(set(old_layers[li]) | set(new_layers[li]))
            for k in all_layer_keys:
                ov = old_layers[li].get(k)
                nv = new_layers[li].get(k)
                if ov != nv:
                    if ov is None:
                        changes.append(f"+ layer[{li}].{k}: {nv}")
                    elif nv is None:
                        changes.append(f"- layer[{li}].{k}: {ov}")
                    else:
                        changes.append(f"~ layer[{li}].{k}: {ov} → {nv}")

    return changes


def _diff_pad_map(old_map: dict, new_map: dict, result: DiffResult):
    all_pads = sorted(set(old_map) | set(new_map), key=lambda x: int(x) if x.isdigit() else x)
    header_printed = False
    for pad in all_pads:
        ov = old_map.get(pad)
        nv = new_map.get(pad)
        if ov != nv:
            if not header_printed:
                result.add("## Pad Note Map")
                header_printed = True
            if ov is None:
                result.add(f"  + Pad {pad}: note={nv['note']} sample={nv['sample']!r}")
            elif nv is None:
                result.add(f"  - Pad {pad}: note={ov['note']} sample={ov['sample']!r}")
            else:
                if ov.get("note") != nv.get("note"):
                    result.add(f"  ~ Pad {pad} note: {ov['note']} → {nv['note']}")
                if ov.get("sample") != nv.get("sample"):
                    result.add(f"  ~ Pad {pad} sample: {ov['sample']!r} → {nv['sample']!r}")
    if header_printed:
        result.add("")


def _diff_qlinks(old_qlinks: list, new_qlinks: list, result: DiffResult):
    n = max(len(old_qlinks), len(new_qlinks))
    header_printed = False
    for i in range(n):
        if i >= len(old_qlinks):
            nq = new_qlinks[i]
            if not header_printed:
                result.add("## Q-Link Assignments")
                header_printed = True
            result.add(f"  + Q-Link {i}: {nq}")
        elif i >= len(new_qlinks):
            oq = old_qlinks[i]
            if not header_printed:
                result.add("## Q-Link Assignments")
                header_printed = True
            result.add(f"  - Q-Link {i}: {oq}")
        else:
            oq = old_qlinks[i]
            nq = new_qlinks[i]
            all_keys = sorted(set(oq) | set(nq) - {"index"})
            ql_changes = []
            for k in all_keys:
                ov = oq.get(k)
                nv = nq.get(k)
                if ov != nv:
                    ql_changes.append(f"{k}: {ov!r} → {nv!r}")
            if ql_changes:
                if not header_printed:
                    result.add("## Q-Link Assignments")
                    header_printed = True
                result.add(f"  ~ Q-Link {i}: " + ", ".join(ql_changes))
    if header_printed:
        result.add("")


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def main():
    parser = argparse.ArgumentParser(
        description="Diff two MPC XPM program files and report what changed.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    parser.add_argument("old", help="Path to the old/original XPM file")
    parser.add_argument("new", help="Path to the new/updated XPM file")
    parser.add_argument(
        "--brief", action="store_true",
        help="Print summary only (no per-keygroup detail)"
    )
    parser.add_argument(
        "--output", metavar="FILE",
        help="Write diff output to FILE instead of stdout"
    )
    args = parser.parse_args()

    old_path = Path(args.old)
    new_path = Path(args.new)

    for p in (old_path, new_path):
        if not p.exists():
            print(f"Error: file not found: {p}", file=sys.stderr)
            sys.exit(1)

    try:
        old_data = parse_program(old_path)
    except ET.ParseError as e:
        print(f"Error parsing {old_path}: {e}", file=sys.stderr)
        sys.exit(1)

    try:
        new_data = parse_program(new_path)
    except ET.ParseError as e:
        print(f"Error parsing {new_path}: {e}", file=sys.stderr)
        sys.exit(1)

    result = diff_programs(old_data, new_data, str(old_path), str(new_path))
    output = result.render(brief=args.brief)

    if args.output:
        out_path = Path(args.output)
        out_path.write_text(output, encoding="utf-8")
        print(f"Diff written to {out_path}")
    else:
        print(output)


if __name__ == "__main__":
    main()
