#!/usr/bin/env python3
"""
xpn_choke_group_assigner.py — Assign choke/mute groups to drum XPM programs.

Reads an existing XPM file, classifies each instrument's sample using
xpn_classify_instrument.py, applies a mute group preset, and writes the result.

Usage:
    python xpn_choke_group_assigner.py --xpm drum_kit.xpm --preset onset
    python xpn_choke_group_assigner.py --xpm drum_kit.xpm --preset standard --stems ./stems/
    python xpn_choke_group_assigner.py --xpm drum_kit.xpm --preset none --output clean.xpm

Presets:
    onset     — ONSET engine kits: CHat/OHat → group 1, Snare → group 2
    standard  — Generic kits: Closed/Open hat → 1, Snare/Ghost → 2, Ride → 3
    none      — Clear all mute groups (set to 0)

Pure stdlib — no external dependencies beyond xpn_classify_instrument.
"""

from __future__ import annotations

import sys
import os
import re
import argparse
import xml.etree.ElementTree as ET

# ---------------------------------------------------------------------------
# Classifier import
# ---------------------------------------------------------------------------

try:
    sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
    from xpn_classify_instrument import classify_wav
    CLASSIFIER_AVAILABLE = True
except ImportError:
    CLASSIFIER_AVAILABLE = False


# ---------------------------------------------------------------------------
# Preset rule tables
# ---------------------------------------------------------------------------

# onset preset — mirrors ONSET engine voice structure
# CHat = closed hat; OHat = open hat; snare variants → group 2
ONSET_RULES = {
    "chihat":   1,   # CHat voice
    "hihat":    1,   # generic hihat → treat as closed
    "cymbal":   0,   # cymbals stay ungrouped unless it's a hat
    "snare":    2,
    "kick":     0,
    "bass":     0,
    "pad":      0,
    "lead":     0,
    "perc":     0,
    "fx":       0,
    "unknown":  0,
}

# onset open-hat override: open hats land in group 1 too
# We detect "open" in the filename to distinguish chihat vs openhat.

# standard preset — generic drum kit with ride group
STANDARD_RULES = {
    "hihat":    1,   # closed hat default
    "chihat":   1,
    "cymbal":   3,   # ride-like cymbals
    "snare":    2,
    "kick":     0,
    "bass":     0,
    "pad":      0,
    "lead":     0,
    "perc":     0,
    "fx":       0,
    "unknown":  0,
}


# ---------------------------------------------------------------------------
# Filename-pattern heuristics (supplement the audio classifier)
# ---------------------------------------------------------------------------

# Maps regex patterns (against lowercase basename) → category override
FILENAME_PATTERNS = [
    (re.compile(r'kick|_kck|_bd_|bass.?drum'),              "kick"),
    (re.compile(r'snare|_sn_|_snr_'),                      "snare"),
    (re.compile(r'ghost.?snare|ghost.?sn|gs\d'),           "ghost_snare"),
    (re.compile(r'clap|_clp_'),                            "snare"),   # clap → snare group
    (re.compile(r'rim.?shot|rimshot|_rim_'),                "snare"),
    (re.compile(r'closed.?hat|c\.?hat|_cht_|closedhat'),   "chihat"),
    (re.compile(r'open.?hat|o\.?hat|_oht_|openhat'),       "openhat"),
    (re.compile(r'hi.?hat|hihat|_hh_'),                    "hihat"),
    (re.compile(r'ride|_rd_'),                             "ride"),
    (re.compile(r'crash|_cr_'),                            "cymbal"),
    (re.compile(r'cymbal|_cym_'),                          "cymbal"),
    (re.compile(r'tom|floor.?tom|rack.?tom'),              "perc"),
    (re.compile(r'perc|cowbell|clave|shaker|tamb'),         "perc"),
    (re.compile(r'_fx_|effect|glitch|noise'),               "fx"),
]


def classify_by_filename(path: str) -> str | None:
    """
    Return a category string from filename patterns, or None if no match.
    Checked before (and can override) audio-based classification.
    """
    base = os.path.splitext(os.path.basename(path))[0].lower()
    for pattern, category in FILENAME_PATTERNS:
        if pattern.search(base):
            return category
    return None


# ---------------------------------------------------------------------------
# Mute group assignment logic
# ---------------------------------------------------------------------------

def mute_group_for_onset(category: str, wav_path: str) -> int:
    """Apply onset preset rules."""
    if category == "openhat":
        return 1   # open hat choked by closed hat — same group
    if category == "chihat" or category == "hihat":
        return 1
    if category == "snare" or category == "ghost_snare":
        return 2
    return ONSET_RULES.get(category, 0)


def mute_group_for_standard(category: str, wav_path: str) -> int:
    """Apply standard preset rules."""
    if category == "openhat":
        return 1   # same group as closed hat
    if category == "chihat" or category == "hihat":
        return 1
    if category == "ghost_snare":
        return 2   # ghost snare choked with full snare
    if category == "snare":
        return 2
    if category == "ride":
        return 3
    if category == "cymbal":
        # crash cymbals are typically ungrouped; ride-like → 3
        base = os.path.basename(wav_path).lower()
        if "ride" in base:
            return 3
        return 0
    return STANDARD_RULES.get(category, 0)


def assign_mute_group(preset: str, category: str, wav_path: str) -> int:
    """Dispatch to the correct preset rule set."""
    if preset == "none":
        return 0
    if preset == "onset":
        return mute_group_for_onset(category, wav_path)
    if preset == "standard":
        return mute_group_for_standard(category, wav_path)
    raise ValueError(f"Unknown preset: {preset!r}")


# ---------------------------------------------------------------------------
# Category resolution: filename first, audio fallback
# ---------------------------------------------------------------------------

def resolve_category(wav_path: str, stems_dir: str | None, verbose: bool = False) -> tuple[str, float, str]:
    """
    Return (category, confidence, source) where source is 'filename' or 'audio'.
    Tries filename heuristics first. Falls back to audio classifier if available.
    """
    # 1. Filename heuristics
    fn_category = classify_by_filename(wav_path)
    if fn_category is not None:
        return fn_category, 1.0, "filename"

    # 2. Try to locate the WAV (may be relative to stems_dir)
    candidates = [wav_path]
    if stems_dir:
        basename = os.path.basename(wav_path)
        candidates.append(os.path.join(stems_dir, basename))
        # Also try stripping any leading path components
        for part in wav_path.replace("\\", "/").split("/"):
            if part.endswith(".wav"):
                candidates.append(os.path.join(stems_dir, part))

    resolved_wav = None
    for c in candidates:
        if os.path.isfile(c):
            resolved_wav = c
            break

    if resolved_wav and CLASSIFIER_AVAILABLE:
        try:
            result = classify_wav(resolved_wav)
            cat = result.get("category", "unknown")
            conf = result.get("confidence", 0.0)
            return cat, conf, "audio"
        except Exception as exc:
            if verbose:
                print(f"    [warn] Audio classification failed for {resolved_wav}: {exc}")

    # 3. No data — unknown
    return "unknown", 0.0, "none"


# ---------------------------------------------------------------------------
# XPM parsing and rewriting
# ---------------------------------------------------------------------------

def find_sample_file(instrument_elem: ET.Element) -> str | None:
    """
    Extract the first SampleFile path from an Instrument element.
    Checks direct children and nested Layer elements.
    """
    # Direct child
    sf = instrument_elem.find("SampleFile")
    if sf is not None and sf.text:
        return sf.text.strip()
    # Inside Layer elements
    for layer in instrument_elem.iter("Layer"):
        sf = layer.find("SampleFile")
        if sf is not None and sf.text:
            return sf.text.strip()
    return None


def get_or_create_mute_group(instrument_elem: ET.Element) -> ET.Element:
    """Return existing MuteGroup element or create one."""
    mg = instrument_elem.find("MuteGroup")
    if mg is None:
        mg = ET.SubElement(instrument_elem, "MuteGroup")
    return mg


def process_xpm(xpm_path: str, preset: str, stems_dir: str | None,
                output_path: str, verbose: bool) -> None:
    """
    Main processing: parse XPM, assign mute groups, write output.
    """
    # Read raw text so we can preserve formatting as much as possible
    with open(xpm_path, "r", encoding="utf-8", errors="replace") as fh:
        raw_xml = fh.read()

    # Parse
    try:
        tree = ET.parse(xpm_path)
    except ET.ParseError as exc:
        print(f"ERROR: Failed to parse {xpm_path}: {exc}", file=sys.stderr)
        sys.exit(1)

    root = tree.getroot()

    # Find Instruments container
    instruments_elem = root.find(".//Instruments")
    if instruments_elem is None:
        print("ERROR: No <Instruments> block found in XPM.", file=sys.stderr)
        sys.exit(1)

    instrument_list = instruments_elem.findall("Instrument")
    if not instrument_list:
        print("WARNING: No <Instrument> elements found.", file=sys.stderr)

    # Header
    print(f"\nXPN Choke Group Assigner")
    print(f"  XPM    : {xpm_path}")
    print(f"  Preset : {preset.upper()}")
    print(f"  Output : {output_path}")
    print(f"  Pads   : {len(instrument_list)}")
    print()

    col_w = 4, 38, 16, 8, 6
    header = (
        f"{'Pad':<{col_w[0]}}  "
        f"{'Sample File':<{col_w[1]}}  "
        f"{'Detected Type':<{col_w[2]}}  "
        f"{'Source':<{col_w[3]}}  "
        f"{'Group':>{col_w[4]}}"
    )
    print(header)
    print("-" * (sum(col_w) + 8))

    changes = 0

    for inst in instrument_list:
        pad_num_attr = inst.get("number", "?")

        # Get sample path
        sample_file = find_sample_file(inst)
        if not sample_file:
            display_name = "<no sample>"
            category = "unknown"
            conf = 0.0
            source = "none"
        else:
            display_name = os.path.basename(sample_file)
            category, conf, source = resolve_category(sample_file, stems_dir, verbose)

        group = assign_mute_group(preset, category, sample_file or "")

        # Update the XML element
        mg_elem = get_or_create_mute_group(inst)
        old_val = mg_elem.text or "0"
        mg_elem.text = str(group)
        if old_val != str(group):
            changes += 1

        # Truncate display name if too long
        trunc_name = display_name
        if len(trunc_name) > col_w[1]:
            trunc_name = "…" + trunc_name[-(col_w[1] - 1):]

        group_str = str(group) if group > 0 else "-"
        conf_str = f"({conf:.0%})" if source == "audio" else ""
        type_display = f"{category} {conf_str}".strip()

        print(
            f"{pad_num_attr:<{col_w[0]}}  "
            f"{trunc_name:<{col_w[1]}}  "
            f"{type_display:<{col_w[2]}}  "
            f"{source:<{col_w[3]}}  "
            f"{group_str:>{col_w[4]}}"
        )

    print()
    print(f"Summary: {len(instrument_list)} pads processed, {changes} group(s) changed.")

    # Write output
    # ET.write strips the XML declaration — add it back
    tree.write(output_path, encoding="unicode", xml_declaration=True)

    # ET writes <?xml version='1.0' encoding='us-ascii'?> — normalise to UTF-8 double-quote style
    with open(output_path, "r", encoding="utf-8") as fh:
        written = fh.read()
    if written.startswith("<?xml"):
        written = re.sub(
            r"<\?xml[^?]*\?>",
            '<?xml version="1.0" encoding="UTF-8"?>',
            written,
            count=1
        )
        with open(output_path, "w", encoding="utf-8") as fh:
            fh.write(written)

    print(f"Written: {output_path}")


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def build_parser() -> argparse.ArgumentParser:
    p = argparse.ArgumentParser(
        description="Assign choke/mute groups to drum XPM programs.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Presets:
  onset     ONSET engine kits — CHat/OHat → group 1, Snare variants → group 2
  standard  Generic kits — Closed/Open hat → 1, Snare/Ghost → 2, Ride → 3
  none      Remove all choke groups (set everything to 0)

Examples:
  python xpn_choke_group_assigner.py --xpm kick_snare_hat.xpm --preset onset
  python xpn_choke_group_assigner.py --xpm kit.xpm --preset standard --stems ./stems/
  python xpn_choke_group_assigner.py --xpm kit.xpm --preset none --output kit_clean.xpm
""",
    )
    p.add_argument("--xpm",    required=True,  help="Input .xpm file path")
    p.add_argument("--preset", required=True,  choices=["onset", "standard", "none"],
                   help="Mute group preset to apply")
    p.add_argument("--stems",  default=None,   help="Directory containing WAV stems (optional)")
    p.add_argument("--output", default=None,   help="Output .xpm path (default: overwrites input)")
    p.add_argument("--verbose", action="store_true", help="Show classification debug output")
    return p


def main() -> None:
    parser = build_parser()
    args = parser.parse_args()

    xpm_path = os.path.abspath(args.xpm)
    if not os.path.isfile(xpm_path):
        print(f"ERROR: XPM not found: {xpm_path}", file=sys.stderr)
        sys.exit(1)

    stems_dir = os.path.abspath(args.stems) if args.stems else None
    if stems_dir and not os.path.isdir(stems_dir):
        print(f"WARNING: stems directory not found: {stems_dir}", file=sys.stderr)
        stems_dir = None

    output_path = os.path.abspath(args.output) if args.output else xpm_path

    if not CLASSIFIER_AVAILABLE and args.preset != "none":
        print(
            "WARNING: xpn_classify_instrument not found — falling back to filename "
            "heuristics only. Place xpn_classify_instrument.py in the same directory "
            "for audio-based classification.",
            file=sys.stderr
        )

    process_xpm(xpm_path, args.preset, stems_dir, output_path, args.verbose)


if __name__ == "__main__":
    main()
