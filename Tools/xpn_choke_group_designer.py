#!/usr/bin/env python3
"""
xpn_choke_group_designer.py — XO_OX Choke Group Designer

Designs and validates choke group assignments for drum kit .xpn programs,
ensuring hi-hat muting, snare choke, and other choke relationships are
correctly configured.

XO_OX Choke Group Standard:
  MuteGroup 0 — No choke (default)
  MuteGroup 1 — Hi-hats (open hat muted by closed hat)
  MuteGroup 2 — Snare choke (rim shot mutes snare)
  MuteGroup 3 — Clap choke (snare + clap mute each other)
  MuteGroup 4 — Kick choke (sub kick chokes long kick)

In XPM XML: <MuteGroup>1</MuteGroup> inside each <Instrument> element.

Usage:
    # Analyze current choke assignments
    python xpn_choke_group_designer.py pack.xpn

    # Suggest choke assignments (no writes)
    python xpn_choke_group_designer.py pack.xpn --suggest

    # Apply suggestions and write to new file
    python xpn_choke_group_designer.py pack.xpn --apply --output fixed.xpn

    # Validate choke group consistency
    python xpn_choke_group_designer.py pack.xpn --validate

    # Target a specific program by name
    python xpn_choke_group_designer.py pack.xpn --program "Drum Kit A" --suggest

    # JSON output for pipelines
    python xpn_choke_group_designer.py pack.xpn --validate --format json

Pure stdlib — zipfile + xml.etree.ElementTree + argparse.
"""


import argparse
import io
import json
import os
import re
import sys
import zipfile
import xml.etree.ElementTree as ET
from dataclasses import dataclass, field
from typing import Dict, List, Optional, Tuple


# ---------------------------------------------------------------------------
# Constants — XO_OX Choke Group Standard
# ---------------------------------------------------------------------------

GROUP_NAMES = {
    0: "No choke",
    1: "Hi-hat",
    2: "Snare choke",
    3: "Clap choke",
    4: "Kick choke",
}

# GM MIDI note → voice role
GM_HAT_CLOSED  = {42, 44}
GM_HAT_OPEN    = {46}
GM_SNARE       = {38, 40}
GM_RIM         = {37}
GM_CLAP        = {39}
GM_KICK        = {35, 36}
GM_KICK_SUB    = {36}  # sub/electric kick — chokes long kick (35)

# Filename patterns → voice role
_FN_PATTERNS: List[Tuple[re.Pattern, str]] = [
    (re.compile(r"open.?hat|o\.?hat|_oht_|openhat"),          "hat_open"),
    (re.compile(r"closed.?hat|c\.?hat|_cht_|closedhat"),      "hat_closed"),
    (re.compile(r"hi.?hat|hihat|_hh_"),                        "hat_closed"),
    (re.compile(r"rim.?shot|rimshot|_rim_"),                   "rim"),
    (re.compile(r"ghost.?snare|ghost.?sn|gs\d"),               "snare"),
    (re.compile(r"clap|_clp_"),                                "clap"),
    (re.compile(r"snare|_sn_|_snr_"),                          "snare"),
    (re.compile(r"sub.?kick|sub.?bd|sub_bass.?drum"),          "kick_sub"),
    (re.compile(r"kick|_kck|_bd_|bass.?drum"),                 "kick"),
]


# ---------------------------------------------------------------------------
# Data models
# ---------------------------------------------------------------------------

@dataclass
class PadEntry:
    pad_id: str          # pad label e.g. "A01"
    number: int          # instrument number attribute
    note: int            # MIDI note (PadNote element)
    mute_group: int      # current MuteGroup value
    sample_name: str     # basename of first sample file (or "")
    voice_hint: str      # detected role: hat_open/hat_closed/snare/rim/clap/kick/kick_sub/""
    suggested_group: int = 0   # populated by suggest()


@dataclass
class ValidationIssue:
    severity: str   # WARN / INFO
    code: str
    message: str
    pad_id: Optional[str] = None


@dataclass
class ProgramResult:
    program_name: str
    xpm_path: str
    pads: List[PadEntry] = field(default_factory=list)
    issues: List[ValidationIssue] = field(default_factory=list)


# ---------------------------------------------------------------------------
# Voice detection helpers
# ---------------------------------------------------------------------------

def _detect_voice(note: int, sample_name: str) -> str:
    """Return a voice role string based on MIDI note and sample filename."""
    # Filename wins if pattern matches
    base = os.path.splitext(sample_name)[0].lower() if sample_name else ""
    for pattern, role in _FN_PATTERNS:
        if pattern.search(base):
            return role
    # Fall back to GM note
    if note in GM_HAT_OPEN:
        return "hat_open"
    if note in GM_HAT_CLOSED:
        return "hat_closed"
    if note in GM_RIM:
        return "rim"
    if note in GM_CLAP:
        return "clap"
    if note in GM_SNARE:
        return "snare"
    if note in GM_KICK_SUB:
        return "kick_sub"
    if note in GM_KICK:
        return "kick"
    return ""


def _suggest_group(voice: str) -> int:
    """Map voice role to XO_OX choke group number."""
    return {
        "hat_open":   1,
        "hat_closed": 1,
        "snare":      2,
        "rim":        2,
        "clap":       3,
        "kick_sub":   4,
        "kick":       0,
        "":           0,
    }.get(voice, 0)


# ---------------------------------------------------------------------------
# XPM parsing
# ---------------------------------------------------------------------------

def _int_el(elem: Optional[ET.Element], default: int = 0) -> int:
    if elem is None or not (elem.text or "").strip():
        return default
    try:
        return int(elem.text.strip())
    except ValueError:
        return default


def _text_el(elem: Optional[ET.Element], default: str = "") -> str:
    if elem is None:
        return default
    return (elem.text or default).strip()


def _first_sample(inst_el: ET.Element) -> str:
    """Return basename of first SampleFile found in the instrument element."""
    for sf in inst_el.iter("SampleFile"):
        val = (sf.text or "").strip()
        if val:
            return os.path.basename(val)
    return ""


def parse_xpm_bytes(xml_bytes: bytes, xpm_path: str) -> ProgramResult:
    """Parse raw XPM bytes into a ProgramResult."""
    try:
        root = ET.fromstring(xml_bytes)
    except ET.ParseError as exc:
        result = ProgramResult(program_name=os.path.basename(xpm_path), xpm_path=xpm_path)
        result.issues.append(ValidationIssue("WARN", "XML_PARSE",
                                              f"XML parse error: {exc}"))
        return result

    # Program name from attribute or filename
    prog_name = (root.attrib.get("name") or
                 root.findtext("ProgramName") or
                 os.path.splitext(os.path.basename(xpm_path))[0])
    result = ProgramResult(program_name=prog_name, xpm_path=xpm_path)

    instruments_el = root.find(".//Instruments")
    if instruments_el is None:
        result.issues.append(ValidationIssue("WARN", "NO_INSTRUMENTS",
                                              "No <Instruments> block found"))
        return result

    pad_labels = _build_pad_labels()

    for inst_el in instruments_el.findall("Instrument"):
        try:
            number = int(inst_el.attrib.get("number", 0))
        except ValueError:
            number = 0

        mute_group = _int_el(inst_el.find("MuteGroup"))
        note       = _int_el(inst_el.find("PadNote"))
        sample     = _first_sample(inst_el)
        voice      = _detect_voice(note, sample)
        pad_id     = pad_labels.get(number, f"Pad{number:02d}")

        result.pads.append(PadEntry(
            pad_id=pad_id,
            number=number,
            note=note,
            mute_group=mute_group,
            sample_name=sample,
            voice_hint=voice,
        ))

    return result


def _build_pad_labels() -> Dict[int, str]:
    """Build pad number → label map (A01–D16, 1-indexed)."""
    labels: Dict[int, str] = {}
    rows = "ABCD"
    for row_idx, row in enumerate(rows):
        for col in range(1, 17):
            num = row_idx * 16 + col
            labels[num] = f"{row}{col:02d}"
    return labels


# ---------------------------------------------------------------------------
# XPN (ZIP) loading
# ---------------------------------------------------------------------------

def load_programs_from_xpn(xpn_path: str,
                            filter_name: Optional[str] = None
                            ) -> List[Tuple[str, bytes]]:
    """
    Return list of (xpm_zip_path, xml_bytes) for all XPM programs in the ZIP.
    Optionally filter by program name substring (case-insensitive).
    """
    results: List[Tuple[str, bytes]] = []
    with zipfile.ZipFile(xpn_path, "r") as zf:
        for name in zf.namelist():
            if not name.lower().endswith(".xpm"):
                continue
            if filter_name:
                # Match against the stem of the zip path
                stem = os.path.splitext(os.path.basename(name))[0]
                if filter_name.lower() not in stem.lower():
                    continue
            results.append((name, zf.read(name)))
    return results


# ---------------------------------------------------------------------------
# Suggest logic
# ---------------------------------------------------------------------------

def suggest(result: ProgramResult) -> None:
    """Populate suggested_group on each pad in-place."""
    for pad in result.pads:
        pad.suggested_group = _suggest_group(pad.voice_hint)


# ---------------------------------------------------------------------------
# Validate logic
# ---------------------------------------------------------------------------

def validate(result: ProgramResult) -> None:
    """Append ValidationIssues for choke group problems (modifies result.issues)."""
    from collections import defaultdict

    group_members: Dict[int, List[PadEntry]] = defaultdict(list)
    for pad in result.pads:
        if pad.mute_group > 0:
            group_members[pad.mute_group].append(pad)

    # Singleton groups (pointless)
    for group, members in group_members.items():
        if len(members) == 1:
            p = members[0]
            result.issues.append(ValidationIssue(
                "WARN", "SINGLETON_GROUP",
                f"MuteGroup {group} ({GROUP_NAMES.get(group, '?')}) has only 1 member — "
                f"choke requires ≥2 pads.",
                pad_id=p.pad_id,
            ))

    # Hat group without a pair (open hat with no closed, or vice versa)
    hat_group_pads = group_members.get(1, [])
    has_open  = any(p.voice_hint == "hat_open"   for p in hat_group_pads)
    has_closed = any(p.voice_hint in ("hat_closed",) for p in hat_group_pads)
    if hat_group_pads and not (has_open and has_closed):
        # Detect which is missing
        if has_open and not has_closed:
            msg = "MuteGroup 1 has open hat but no closed hat — choke partner missing."
        elif has_closed and not has_open:
            msg = "MuteGroup 1 has closed hat but no open hat — choke partner missing."
        else:
            msg = "MuteGroup 1 pads detected but hat voice roles unclear."
        result.issues.append(ValidationIssue("WARN", "HAT_PAIR_MISSING", msg))

    # Pad in multiple groups — not possible with a single MuteGroup value, but
    # guard against stale XML with group > 4
    for pad in result.pads:
        if pad.mute_group > 4:
            result.issues.append(ValidationIssue(
                "WARN", "GROUP_OUT_OF_RANGE",
                f"MuteGroup {pad.mute_group} exceeds XO_OX standard max of 4.",
                pad_id=pad.pad_id,
            ))

    # Voice / group mismatch (suggestion differs from current)
    suggest_copy = ProgramResult(result.program_name, result.xpm_path,
                                 list(result.pads))
    suggest(suggest_copy)
    for orig, suggested in zip(result.pads, suggest_copy.pads):
        if orig.mute_group != suggested.suggested_group and orig.voice_hint:
            result.issues.append(ValidationIssue(
                "INFO", "GROUP_MISMATCH",
                f"Pad {orig.pad_id} ({orig.voice_hint}) has MuteGroup={orig.mute_group}, "
                f"expected {suggested.suggested_group} per XO_OX standard.",
                pad_id=orig.pad_id,
            ))


# ---------------------------------------------------------------------------
# Apply logic — rewrite XPN ZIP
# ---------------------------------------------------------------------------

def _rewrite_xpm_bytes(xml_bytes: bytes, updated_pads: Dict[int, int]) -> bytes:
    """
    Return rewritten XPM XML bytes with MuteGroup values updated.
    updated_pads: {instrument_number: new_mute_group}
    """
    try:
        root = ET.fromstring(xml_bytes)
    except ET.ParseError:
        return xml_bytes  # pass through unchanged if parse fails

    instruments_el = root.find(".//Instruments")
    if instruments_el is None:
        return xml_bytes

    for inst_el in instruments_el.findall("Instrument"):
        try:
            number = int(inst_el.attrib.get("number", -1))
        except ValueError:
            continue
        if number not in updated_pads:
            continue
        mg_el = inst_el.find("MuteGroup")
        if mg_el is None:
            mg_el = ET.SubElement(inst_el, "MuteGroup")
        mg_el.text = str(updated_pads[number])

    # Serialize to bytes
    buf = io.BytesIO()
    tree = ET.ElementTree(root)
    tree.write(buf, encoding="unicode", xml_declaration=True)
    raw = buf.getvalue()
    # Normalise declaration to UTF-8 double-quote style
    raw = re.sub(r"<\?xml[^?]*\?>",
                 '<?xml version="1.0" encoding="UTF-8"?>',
                 raw, count=1)
    return raw.encode("utf-8")


def apply_suggestions(xpn_path: str, output_path: str,
                      filter_name: Optional[str],
                      programs: List[ProgramResult]) -> None:
    """
    Write a new XPN ZIP with suggested MuteGroup values applied to matching programs.
    Non-XPM entries are copied verbatim.
    """
    # Build lookup: xpm_zip_path → {pad_number: suggested_group}
    update_map: Dict[str, Dict[int, int]] = {}
    for prog in programs:
        pad_updates = {p.number: p.suggested_group for p in prog.pads}
        update_map[prog.xpm_path] = pad_updates

    with zipfile.ZipFile(xpn_path, "r") as zf_in, \
         zipfile.ZipFile(output_path, "w", compression=zipfile.ZIP_DEFLATED) as zf_out:
        for item in zf_in.infolist():
            data = zf_in.read(item.filename)
            if item.filename in update_map:
                data = _rewrite_xpm_bytes(data, update_map[item.filename])
            zf_out.writestr(item, data)

    print(f"Written: {output_path}")


# ---------------------------------------------------------------------------
# Output formatters
# ---------------------------------------------------------------------------

def _group_label(g: int) -> str:
    name = GROUP_NAMES.get(g, "?")
    return f"{g} ({name})" if g else "0 (none)"


def print_analysis(prog: ProgramResult) -> None:
    print(f"\nCHOKE ANALYSIS — {prog.program_name}")
    print(f"  XPM: {prog.xpm_path}")
    print()
    col = (6, 6, 16, 20, 22)
    hdr = (f"{'Pad':<{col[0]}}  {'Note':>{col[1]}}  {'Voice':<{col[2]}}  "
           f"{'Sample':<{col[3]}}  {'MuteGroup':<{col[4]}}")
    print(hdr)
    print("-" * (sum(col) + 8))
    for p in prog.pads:
        note_str = str(p.note) if p.note else "-"
        samp = p.sample_name
        if len(samp) > col[3]:
            samp = "…" + samp[-(col[3] - 1):]
        grp_str = _group_label(p.mute_group)
        # Checkmark if hat pair in same group
        marker = ""
        if p.mute_group == 1 and p.voice_hint in ("hat_open", "hat_closed"):
            marker = " ✓"
        print(f"{p.pad_id:<{col[0]}}  {note_str:>{col[1]}}  "
              f"{p.voice_hint or '':< {col[2]}}  {samp:<{col[3]}}  "
              f"{grp_str}{marker}")


def print_suggestions(prog: ProgramResult) -> None:
    print(f"\nSUGGESTED CHANGES — {prog.program_name}")
    changes = [p for p in prog.pads if p.mute_group != p.suggested_group]
    if not changes:
        print("  No changes needed — all pads match XO_OX standard.")
        return
    for p in changes:
        print(f"  Pad {p.pad_id} ({p.voice_hint or 'unknown'}): "
              f"MuteGroup {p.mute_group} → {p.suggested_group} "
              f"({GROUP_NAMES.get(p.suggested_group, '?')})")
    print(f"\n  {len(changes)} pad(s) would be updated.")


def print_validation(prog: ProgramResult) -> None:
    print(f"\nVALIDATION — {prog.program_name}")
    if not prog.issues:
        print("  ✓ No issues found.")
        return
    for iss in prog.issues:
        pad_tag = f" [Pad {iss.pad_id}]" if iss.pad_id else ""
        print(f"  {iss.severity} {iss.code}{pad_tag}: {iss.message}")
    warn_count = sum(1 for i in prog.issues if i.severity == "WARN")
    info_count = sum(1 for i in prog.issues if i.severity == "INFO")
    print(f"\n  {warn_count} WARN, {info_count} INFO")


def output_json(programs: List[ProgramResult], mode: str) -> None:
    out = []
    for prog in programs:
        entry: dict = {"program": prog.program_name, "xpm": prog.xpm_path}
        if mode in ("analyze", "suggest"):
            entry["pads"] = [
                {
                    "pad_id": p.pad_id,
                    "note": p.note,
                    "voice": p.voice_hint,
                    "sample": p.sample_name,
                    "mute_group": p.mute_group,
                    **({"suggested_group": p.suggested_group} if mode == "suggest" else {}),
                }
                for p in prog.pads
            ]
        if mode == "validate":
            entry["issues"] = [
                {"severity": i.severity, "code": i.code,
                 "message": i.message, "pad_id": i.pad_id}
                for i in prog.issues
            ]
        out.append(entry)
    print(json.dumps(out, indent=2))


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def build_parser() -> argparse.ArgumentParser:
    p = argparse.ArgumentParser(
        description="XO_OX Choke Group Designer — analyze, suggest, apply, and validate "
                    "choke group assignments in drum kit .xpn programs.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
XO_OX Choke Group Standard:
  MuteGroup 0  No choke (default)
  MuteGroup 1  Hi-hats (open hat muted by closed hat)
  MuteGroup 2  Snare choke (rim shot mutes snare)
  MuteGroup 3  Clap choke (snare + clap mute each other)
  MuteGroup 4  Kick choke (sub kick chokes long kick)

Examples:
  python xpn_choke_group_designer.py pack.xpn
  python xpn_choke_group_designer.py pack.xpn --suggest
  python xpn_choke_group_designer.py pack.xpn --apply --output fixed.xpn
  python xpn_choke_group_designer.py pack.xpn --validate
  python xpn_choke_group_designer.py pack.xpn --program "Drum Kit A" --suggest
  python xpn_choke_group_designer.py pack.xpn --validate --format json
""",
    )
    p.add_argument("xpn", help="Input .xpn file path")
    p.add_argument("--program", default=None,
                   help="Filter by program name (substring match, case-insensitive)")
    p.add_argument("--suggest",  action="store_true",
                   help="Show suggested choke group changes based on GM notes + sample names")
    p.add_argument("--apply",    action="store_true",
                   help="Write suggested changes to output .xpn (requires --output or overwrites)")
    p.add_argument("--validate", action="store_true",
                   help="Check choke group consistency and flag issues")
    p.add_argument("--output",   default=None,
                   help="Output .xpn path (required with --apply; default: overwrites input)")
    p.add_argument("--format",   choices=["text", "json"], default="text",
                   help="Output format (default: text)")
    return p


def main() -> None:
    parser = build_parser()
    args = parser.parse_args()

    xpn_path = os.path.abspath(args.xpn)
    if not os.path.isfile(xpn_path):
        print(f"ERROR: File not found: {xpn_path}", file=sys.stderr)
        sys.exit(1)

    if not zipfile.is_zipfile(xpn_path):
        print(f"ERROR: Not a valid ZIP/XPN file: {xpn_path}", file=sys.stderr)
        sys.exit(1)

    # Load programs
    entries = load_programs_from_xpn(xpn_path, filter_name=args.program)
    if not entries:
        msg = f"No XPM programs found"
        if args.program:
            msg += f" matching '{args.program}'"
        print(f"ERROR: {msg} in {os.path.basename(xpn_path)}", file=sys.stderr)
        sys.exit(1)

    programs: List[ProgramResult] = []
    for zip_path, xml_bytes in entries:
        prog = parse_xpm_bytes(xml_bytes, zip_path)
        programs.append(prog)

    # Determine active mode
    do_suggest  = args.suggest or args.apply
    do_validate = args.validate

    if do_suggest:
        for prog in programs:
            suggest(prog)

    if do_validate:
        for prog in programs:
            validate(prog)

    # Determine display mode label for JSON
    if do_validate:
        json_mode = "validate"
    elif do_suggest:
        json_mode = "suggest"
    else:
        json_mode = "analyze"

    # Output
    if args.format == "json":
        output_json(programs, json_mode)
    else:
        for prog in programs:
            print_analysis(prog)
            if do_suggest:
                print_suggestions(prog)
            if do_validate:
                print_validation(prog)

    # Apply
    if args.apply:
        output_path = os.path.abspath(args.output) if args.output else xpn_path
        apply_suggestions(xpn_path, output_path, args.program, programs)
        total_changes = sum(
            sum(1 for p in prog.pads if p.mute_group != p.suggested_group)
            for prog in programs
        )
        if args.format == "text":
            print(f"\nApplied {total_changes} choke group update(s) across "
                  f"{len(programs)} program(s).")


if __name__ == "__main__":
    main()
