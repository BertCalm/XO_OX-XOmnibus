#!/usr/bin/env python3
"""
XPN .xometa → XPM Exporter — XO_OX Designs
Exports XOceanus .xometa preset files to Akai MPC Keygroup XPM programs.

This implements the params_sidecar concept in a simplified form: instead of
requiring XOceanus to be running, it embeds preset parameters directly into
XPM Q-Link assignments (4 macros = CC16–CC19).  The resulting XPM is a
"parameter snapshot" program — no audio samples — useful as a patch recall
card inside any MPC project that references the XOceanus AU plugin.

The full preset JSON is base64-encoded and embedded in an XML comment block
for round-trip recovery.

Usage (single file):
    python xpn_xometa_to_xpm_exporter.py preset.xometa
    python xpn_xometa_to_xpm_exporter.py preset.xometa --output-dir /path/to/out

Usage (batch — whole directory):
    python xpn_xometa_to_xpm_exporter.py Presets/XOceanus/Foundation --batch
    python xpn_xometa_to_xpm_exporter.py Presets/XOceanus/ --batch --output-dir /tmp/xpms

Stdlib only: json, xml.etree.ElementTree, argparse, pathlib, base64, sys
"""

import argparse
import base64
import json
import sys
import xml.etree.ElementTree as ET
from pathlib import Path

# ---------------------------------------------------------------------------
# Constants
# ---------------------------------------------------------------------------

REPO_ROOT = Path(__file__).resolve().parent.parent
PRESETS_DIR = REPO_ROOT / "Presets" / "XOceanus"

# MPC CC numbers for Q-Link knobs 1–4 (standard MPC assignment)
QLINK_CCS = [16, 17, 18, 19]

# Default macro labels if the preset omits them
DEFAULT_MACRO_LABELS = ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"]

# MIDI note numbers for full-range keygroup (C-2 = 0 … G8 = 127)
KEYGROUP_LOW  = 0
KEYGROUP_HIGH = 127
KEYGROUP_ROOT = 60  # middle C


# ---------------------------------------------------------------------------
# .xometa parsing
# ---------------------------------------------------------------------------

def load_xometa(path: Path) -> dict:
    """Load and return a .xometa JSON file as a dict."""
    try:
        with path.open("r", encoding="utf-8") as fh:
            return json.load(fh)
    except json.JSONDecodeError as exc:
        raise ValueError(f"Invalid JSON in {path}: {exc}") from exc


def extract_macro_info(meta: dict) -> list[dict]:
    """
    Return a list of 4 macro dicts, each containing:
        label      : str   — display name (e.g. "CHARACTER")
        param_ids  : list  — parameter IDs that map to this macro slot
        value      : float — normalised 0.0–1.0 (first param value found, or 0.5)
        cc         : int   — MIDI CC number
    """
    labels: list[str] = meta.get("macroLabels") or DEFAULT_MACRO_LABELS
    # Pad / trim to exactly 4
    labels = (labels + DEFAULT_MACRO_LABELS)[:4]

    # Collect all parameter values across all engines
    all_params: dict[str, float] = {}
    for engine_params in (meta.get("parameters") or {}).values():
        if isinstance(engine_params, dict):
            all_params.update(engine_params)

    macros: list[dict] = []
    for idx, label in enumerate(labels):
        # Try to find parameter IDs that contain the macro index (m1–m4) or
        # the macro label token.  This is heuristic — the real mapping lives
        # inside the plugin, but we make a best-effort match for the sidecar.
        macro_slot = f"m{idx + 1}"
        label_lower = label.lower()
        matched_ids: list[str] = []
        first_value: float = 0.5

        for param_id, pval in all_params.items():
            pid_lower = param_id.lower()
            if macro_slot in pid_lower or label_lower in pid_lower:
                matched_ids.append(param_id)
                if first_value == 0.5:
                    first_value = float(pval)

        # If no label-based match, fall back to the Nth parameter in insertion order
        if not matched_ids and all_params:
            param_list = list(all_params.items())
            if idx < len(param_list):
                pid, pval = param_list[idx]
                matched_ids = [pid]
                first_value = float(pval)

        # Clamp to 0.0–1.0 range (values above 1 are raw plugin-unit values;
        # normalise to 0–1 for CC purposes using simple range guess)
        if first_value > 1.0 or first_value < 0.0:
            # Attempt naive normalisation: assume 0–20000 range for frequency,
            # 0–1 for everything else — we can't know without the plugin schema.
            if first_value > 1.0:
                first_value = min(1.0, first_value / 20000.0)
            else:
                first_value = 0.0

        macros.append({
            "label":     label,
            "param_ids": matched_ids,
            "value":     max(0.0, min(1.0, first_value)),
            "cc":        QLINK_CCS[idx],
        })

    return macros


def normalised_to_cc(value: float) -> int:
    """Map 0.0–1.0 to MIDI CC value 0–127."""
    return int(round(value * 127.0))


# ---------------------------------------------------------------------------
# XPM generation
# ---------------------------------------------------------------------------

def indent_xml(elem: ET.Element, level: int = 0) -> None:
    """Add pretty-print indentation in-place (stdlib ET has no pretty_print)."""
    pad = "\n" + "  " * level
    if len(elem):
        if not elem.text or not elem.text.strip():
            elem.text = pad + "  "
        if not elem.tail or not elem.tail.strip():
            elem.tail = pad
        for child in elem:
            indent_xml(child, level + 1)
        if not child.tail or not child.tail.strip():  # type: ignore[reportPossiblyUnbound]
            child.tail = pad
    else:
        if level and (not elem.tail or not elem.tail.strip()):
            elem.tail = pad


def _sub(parent: ET.Element, tag: str, text: str = "", **attribs) -> ET.Element:
    """Shortcut: create sub-element with optional text and attributes."""
    el = ET.SubElement(parent, tag, **attribs)
    if text:
        el.text = text
    return el


def generate_xpm(meta: dict, source_path: Path) -> str:
    """
    Build and return the XPM XML string for the given .xometa preset.

    The XPM contains:
    - 1 Keygroup spanning the full MIDI range (no samples)
    - 4 Q-Link assignments mapped to CC16–CC19 with macro labels
    - A Metadata XML comment embedding the full preset as base64
    - UserComment = engine(s) + preset description
    """
    preset_name: str   = meta.get("name") or source_path.stem
    engines: list[str] = meta.get("engines") or ["Unknown"]
    description: str   = meta.get("description") or ""
    mood: str          = meta.get("mood") or ""
    macros             = extract_macro_info(meta)

    # --- Build user comment (150 chars max for MPC field) ---
    engine_str  = "/".join(engines)
    user_comment = f"{engine_str} | {mood} | {description}"[:150]

    # --- Base64-encode the full preset JSON for round-trip recovery ---
    preset_json_bytes = json.dumps(meta, ensure_ascii=False, indent=2).encode("utf-8")
    preset_b64        = base64.b64encode(preset_json_bytes).decode("ascii")

    # --- Root element ---
    root = ET.Element("MPCVObject")
    root.set("Version", "2.1")

    # Program header
    prog = _sub(root, "Program", Type="KeygroupProgram")
    _sub(prog, "Name", preset_name)
    _sub(prog, "UserComment", user_comment)
    _sub(prog, "ProgramType", "KeygroupProgram")
    _sub(prog, "NumKeygroupsInProgram", "1")

    # --- Q-Link assignments ---
    qlinks_el = _sub(prog, "QLinkAssignments")
    for m in macros:
        cc_val = normalised_to_cc(m["value"])
        ql = _sub(qlinks_el, "QLinkAssignment")
        _sub(ql, "Label",        m["label"])
        _sub(ql, "CC",           str(m["cc"]))
        _sub(ql, "DefaultValue", str(cc_val))
        _sub(ql, "MinValue",     "0")
        _sub(ql, "MaxValue",     "127")
        # Embed param IDs as a comma-separated hint (non-standard, informational)
        if m["param_ids"]:
            _sub(ql, "ParamHint", ",".join(m["param_ids"]))

    # --- Single keygroup, full range, no samples ---
    keygroups_el = _sub(prog, "Keygroups")
    kg = _sub(keygroups_el, "Keygroup")
    _sub(kg, "LowNote",       str(KEYGROUP_LOW))
    _sub(kg, "HighNote",      str(KEYGROUP_HIGH))
    _sub(kg, "RootNote",      str(KEYGROUP_ROOT))
    _sub(kg, "KeyTrack",      "True")
    _sub(kg, "NumLayers",     "1")

    layer = _sub(kg, "Layer", Num="1")
    _sub(layer, "SampleName",  "")
    _sub(layer, "SampleFile",  "")
    _sub(layer, "VelStart",    "1")
    _sub(layer, "VelEnd",      "127")
    _sub(layer, "Volume",      "1.0")
    _sub(layer, "Pan",         "0.5")
    _sub(layer, "Pitch",       "0.0")

    # --- Metadata block: embed full preset JSON as base64 ---
    metadata_el = _sub(prog, "Metadata")
    _sub(metadata_el, "Source",       "XOceanus")
    _sub(metadata_el, "SchemaVersion", str(meta.get("schema_version", 1)))
    _sub(metadata_el, "Engine",        engine_str)
    _sub(metadata_el, "Mood",          mood)
    _sub(metadata_el, "PresetJSON_b64", preset_b64)

    # --- Sonic DNA block (direct copy for quick reference) ---
    dna_src: dict = meta.get("dna") or {}
    if dna_src:
        dna_el = _sub(prog, "SonicDNA")
        for key, val in dna_src.items():
            _sub(dna_el, key.capitalize(), str(round(float(val), 4)))

    # --- Pretty print ---
    indent_xml(root)
    xml_declaration = '<?xml version="1.0" encoding="UTF-8"?>\n'
    sidecar_comment = (
        "<!-- XOceanus Parameter Snapshot XPM\n"
        "     Generated by xpn_xometa_to_xpm_exporter.py\n"
        "     Q-Links: CC16=CHARACTER  CC17=MOVEMENT  CC18=COUPLING  CC19=SPACE\n"
        "     Full preset JSON embedded in Metadata/PresetJSON_b64 (base64)\n"
        "     To restore: base64.b64decode(PresetJSON_b64).decode('utf-8')\n"
        "-->\n"
    )
    xml_body = ET.tostring(root, encoding="unicode", xml_declaration=False)
    return xml_declaration + sidecar_comment + xml_body + "\n"


# ---------------------------------------------------------------------------
# I/O helpers
# ---------------------------------------------------------------------------

def export_one(xometa_path: Path, output_dir: Path) -> Path:
    """Convert a single .xometa file → .xpm; return output path."""
    meta        = load_xometa(xometa_path)
    xpm_content = generate_xpm(meta, xometa_path)

    output_dir.mkdir(parents=True, exist_ok=True)
    out_path = output_dir / (xometa_path.stem + ".xpm")
    out_path.write_text(xpm_content, encoding="utf-8")
    return out_path


def collect_xometa_files(root: Path) -> list[Path]:
    """Recursively collect all .xometa files under root."""
    return sorted(root.rglob("*.xometa"))


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def build_parser() -> argparse.ArgumentParser:
    p = argparse.ArgumentParser(
        prog="xpn_xometa_to_xpm_exporter",
        description=(
            "Export XOceanus .xometa preset files to Akai MPC Keygroup XPM programs.\n"
            "Embeds 4-macro Q-Link assignments (CC16–CC19) and full preset JSON\n"
            "as base64 inside a <Metadata> block for round-trip recovery."
        ),
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    p.add_argument(
        "input",
        metavar="PRESET_OR_DIR",
        help="Path to a .xometa file or a directory (use --batch for directory).",
    )
    p.add_argument(
        "--output-dir", "-o",
        metavar="DIR",
        default=None,
        help=(
            "Output directory for generated .xpm files. "
            "Defaults to same directory as input file, "
            "or ./xpm_output/ for batch mode."
        ),
    )
    p.add_argument(
        "--batch", "-b",
        action="store_true",
        help=(
            "Process all .xometa files found recursively under PRESET_OR_DIR. "
            "Mirror the subdirectory structure inside --output-dir."
        ),
    )
    p.add_argument(
        "--verbose", "-v",
        action="store_true",
        help="Print each exported file path.",
    )
    return p


def main() -> int:
    parser = build_parser()
    args   = parser.parse_args()

    input_path = Path(args.input).expanduser().resolve()

    # ---- Single-file mode ------------------------------------------------
    if not args.batch:
        if input_path.is_dir():
            print(
                f"ERROR: '{input_path}' is a directory — use --batch to process all files.",
                file=sys.stderr,
            )
            return 1
        if not input_path.exists():
            print(f"ERROR: File not found: {input_path}", file=sys.stderr)
            return 1
        if input_path.suffix.lower() != ".xometa":
            print(f"WARNING: '{input_path}' does not have a .xometa extension.", file=sys.stderr)

        output_dir = Path(args.output_dir).expanduser().resolve() if args.output_dir \
                     else input_path.parent

        try:
            out = export_one(input_path, output_dir)
        except (ValueError, OSError) as exc:
            print(f"ERROR: {exc}", file=sys.stderr)
            return 1

        print(f"Exported: {out}")
        return 0

    # ---- Batch mode -------------------------------------------------------
    if not input_path.is_dir():
        print(f"ERROR: --batch requires a directory, got: {input_path}", file=sys.stderr)
        return 1

    output_root = Path(args.output_dir).expanduser().resolve() if args.output_dir \
                  else Path.cwd() / "xpm_output"

    files = collect_xometa_files(input_path)
    if not files:
        print(f"No .xometa files found under: {input_path}", file=sys.stderr)
        return 1

    ok_count  = 0
    err_count = 0

    for xometa_path in files:
        # Mirror subdirectory structure relative to input root
        rel        = xometa_path.relative_to(input_path)
        target_dir = output_root / rel.parent

        try:
            out = export_one(xometa_path, target_dir)
            ok_count += 1
            if args.verbose:
                print(f"  OK  {out}")
        except (ValueError, OSError) as exc:
            err_count += 1
            print(f"  ERR {xometa_path.name}: {exc}", file=sys.stderr)

    summary_line = f"Batch complete: {ok_count} exported"
    if err_count:
        summary_line += f", {err_count} failed"
    summary_line += f" → {output_root}"
    print(summary_line)

    return 0 if err_count == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
