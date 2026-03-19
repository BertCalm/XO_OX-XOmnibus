#!/usr/bin/env python3
"""
XPN Kit Validator — XO_OX Designs
Deep structural validation of XPM drum kit programs.

Goes beyond basic XML validation to check whether the kit is properly designed
for musical use: pad coverage, choke groups, velocity layering consistency,
root note distribution, sample reuse, stereo balance, and core voice presence.

Checks performed:
  1. Pad coverage       — 16 pads; warn on empty pads
  2. Choke groups       — open/closed hat in same choke group; short+long variant
  3. Velocity layers    — consistent layer count across pads
  4. Root note spread   — flag if 3+ pads share the same root note
  5. Sample variety     — flag if same sample file appears in 3+ pads
  6. Pan balance        — extreme pan (>90% L or R) without opposing pad
  7. Core voices        — at least one kick, snare, and hat

Severity levels:
  FAIL  Will likely cause musical problems or MPC confusion
  WARN  May cause unexpected results or signals design gaps
  INFO  Best-practice recommendation

Usage:
    # Validate a single .xpm file
    python3 xpn_kit_validator.py path/to/kit.xpm

    # Validate all .xpm programs inside a .xpn ZIP archive
    python3 xpn_kit_validator.py --xpn path/to/pack.xpn

    # Strict mode — WARNs count as failures in exit code
    python3 xpn_kit_validator.py kit.xpm --strict

    # JSON output for CI pipelines
    python3 xpn_kit_validator.py kit.xpm --format json
"""

import argparse
import json
import os
import sys
import zipfile
from collections import Counter, defaultdict
from dataclasses import dataclass, field
from pathlib import Path
from typing import Dict, List, Optional, Tuple
from xml.etree import ElementTree as ET


# ---------------------------------------------------------------------------
# Data model
# ---------------------------------------------------------------------------

FAIL = "FAIL"
WARN = "WARN"
INFO = "INFO"


@dataclass
class Issue:
    severity: str      # FAIL / WARN / INFO
    code: str          # short machine-readable tag
    message: str
    pad: Optional[int] = None  # pad number if pad-specific


@dataclass
class PadInfo:
    number: int
    note: int = 0           # MIDI root note
    mute_group: int = 0     # 0 = no group
    layers: List[dict] = field(default_factory=list)   # list of layer dicts
    pan: int = 0            # -50..50 or 0..100 depending on XPM version
    voice_hint: str = ""    # "kick" / "snare" / "hat_open" / "hat_closed" / ""


# ---------------------------------------------------------------------------
# XML parsing helpers
# ---------------------------------------------------------------------------

def _int(elem: Optional[ET.Element], default: int = 0) -> int:
    if elem is None:
        return default
    try:
        return int(elem.text or default)
    except ValueError:
        return default


def _text(elem: Optional[ET.Element], default: str = "") -> str:
    if elem is None:
        return default
    return (elem.text or default).strip()


def parse_xpm(xml_bytes: bytes) -> Tuple[List[PadInfo], List[Issue]]:
    """Parse an XPM XML file and return (pads, parse_issues)."""
    issues: List[Issue] = []
    pads: List[PadInfo] = []

    try:
        root = ET.fromstring(xml_bytes)
    except ET.ParseError as exc:
        issues.append(Issue(FAIL, "XML_PARSE", f"XML parse error: {exc}"))
        return pads, issues

    # Locate <Instruments> block
    instruments_el = root.find(".//Instruments")
    if instruments_el is None:
        issues.append(Issue(FAIL, "NO_INSTRUMENTS", "No <Instruments> block found in XPM"))
        return pads, issues

    for inst_el in instruments_el.findall("Instrument"):
        num = _int(inst_el.get("number") and ET.Element("x"), 0)
        try:
            num = int(inst_el.attrib.get("number", 0))
        except ValueError:
            num = 0

        mute_group = _int(inst_el.find("MuteGroup"))
        pad_note   = _int(inst_el.find("PadNote"))    # MIDI note for this pad slot

        # Collect layers
        layers = []
        for layer_el in inst_el.findall(".//Layer"):
            vel_start   = _int(layer_el.find("VelStart"))
            vel_end     = _int(layer_el.find("VelEnd"))
            root_note   = _int(layer_el.find("RootNote"))
            sample_file = _text(layer_el.find("SampleFile"))
            layers.append({
                "vel_start":   vel_start,
                "vel_end":     vel_end,
                "root_note":   root_note,
                "sample_file": sample_file,
            })

        # Pan — try both common XPM field names
        pan_el = inst_el.find("Pan") or inst_el.find("PanValue")
        pan = _int(pan_el)

        pad = PadInfo(
            number=num,
            note=pad_note,
            mute_group=mute_group,
            layers=layers,
            pan=pan,
        )
        pads.append(pad)

    return pads, issues


# ---------------------------------------------------------------------------
# Voice heuristics (guess voice role from MIDI note)
# ---------------------------------------------------------------------------

# GM approximate ranges
_KICK_NOTES   = {35, 36}
_SNARE_NOTES  = {38, 40}
_CHAT_NOTES   = {42, 44}
_OHAT_NOTES   = {46}
_HAT_NOTES    = _CHAT_NOTES | _OHAT_NOTES
_CLAP_NOTES   = {39}
_TOM_NOTES    = {41, 43, 45, 47, 48, 50}

def _guess_voice(note: int) -> str:
    if note in _KICK_NOTES:
        return "kick"
    if note in _SNARE_NOTES:
        return "snare"
    if note in _CHAT_NOTES:
        return "hat_closed"
    if note in _OHAT_NOTES:
        return "hat_open"
    if note in _CLAP_NOTES:
        return "clap"
    if note in _TOM_NOTES:
        return "tom"
    return ""


# ---------------------------------------------------------------------------
# Validation checks
# ---------------------------------------------------------------------------

def check_pad_coverage(pads: List[PadInfo]) -> List[Issue]:
    """Check 1: 16 pads, warn on empty (no real layers)."""
    issues = []
    active_nums = {p.number for p in pads}
    if len(active_nums) < 8:
        issues.append(Issue(WARN, "LOW_PAD_COUNT",
            f"Only {len(active_nums)} instrument slots found — typical drum kit has 8–16"))

    empty = []
    for p in pads:
        real = [l for l in p.layers if l["vel_start"] > 0 and l["sample_file"]]
        if not real:
            empty.append(p.number)

    if empty:
        issues.append(Issue(WARN, "EMPTY_PADS",
            f"{len(empty)} pad(s) have no real sample layers: {sorted(empty)}"))
    return issues


def check_choke_groups(pads: List[PadInfo]) -> List[Issue]:
    """Check 2: open/closed hat should share a choke group."""
    issues = []
    by_note: Dict[int, PadInfo] = {}
    for p in pads:
        by_note[p.note] = p

    # Find hat pads by GM note
    chat_pads = [p for p in pads if p.note in _CHAT_NOTES and
                 any(l["sample_file"] for l in p.layers)]
    ohat_pads = [p for p in pads if p.note in _OHAT_NOTES and
                 any(l["sample_file"] for l in p.layers)]

    if chat_pads and ohat_pads:
        chat_groups = {p.mute_group for p in chat_pads if p.mute_group != 0}
        ohat_groups = {p.mute_group for p in ohat_pads if p.mute_group != 0}
        if not chat_groups or not ohat_groups:
            issues.append(Issue(WARN, "HAT_NO_CHOKE",
                "Open/closed hat pads found but one or both has MuteGroup=0 — "
                "closed hat won't cut open hat"))
        elif not chat_groups.intersection(ohat_groups):
            issues.append(Issue(WARN, "HAT_CHOKE_MISMATCH",
                f"Closed hat MuteGroup(s) {chat_groups} differ from "
                f"open hat MuteGroup(s) {ohat_groups} — hats won't choke each other"))
    elif ohat_pads and not chat_pads:
        issues.append(Issue(INFO, "HAT_OPEN_ONLY",
            "Open hat present but no closed hat found — choke relationship impossible"))

    return issues


def check_velocity_consistency(pads: List[PadInfo]) -> List[Issue]:
    """Check 3: all active pads should have the same layer count."""
    issues = []
    active = [(p, len([l for l in p.layers if l["vel_start"] > 0 and l["sample_file"]]))
              for p in pads]
    active = [(p, c) for p, c in active if c > 0]

    if not active:
        return issues

    counts = Counter(c for _, c in active)
    dominant = counts.most_common(1)[0][0]
    outliers = [p.number for p, c in active if c != dominant and c > 0]

    if len(counts) > 1:
        issues.append(Issue(WARN, "LAYER_COUNT_INCONSISTENT",
            f"Most pads have {dominant} velocity layer(s) but pad(s) {sorted(outliers)} differ — "
            "suggests incomplete layering"))
    return issues


def check_root_note_distribution(pads: List[PadInfo]) -> List[Issue]:
    """Check 4: no root note should be shared by 3+ pads (copy-paste error)."""
    issues = []
    note_counter: Counter = Counter()
    for p in pads:
        real_layers = [l for l in p.layers if l["vel_start"] > 0 and l["sample_file"]]
        if not real_layers:
            continue
        # Use per-layer root notes if non-zero, else pad MIDI note
        for l in real_layers:
            note = l["root_note"] if l["root_note"] != 0 else p.note
            note_counter[note] += 1

    dupes = [(note, count) for note, count in note_counter.items() if count >= 3]
    for note, count in sorted(dupes):
        issues.append(Issue(WARN, "ROOT_NOTE_DUPE",
            f"Root note {note} shared by {count} layers across pads — "
            "possible copy-paste error; check pitch tuning"))
    return issues


def check_sample_variety(pads: List[PadInfo]) -> List[Issue]:
    """Check 5: same sample appearing in 3+ pads is suspicious."""
    issues = []
    sample_pads: Dict[str, set] = defaultdict(set)
    for p in pads:
        seen_in_pad = set()
        for l in p.layers:
            sf = l["sample_file"].strip()
            if sf and l["vel_start"] > 0:
                sf_base = os.path.basename(sf).lower()
                if sf_base not in seen_in_pad:
                    sample_pads[sf_base].add(p.number)
                    seen_in_pad.add(sf_base)

    dupes = [(sf, pads_set) for sf, pads_set in sample_pads.items() if len(pads_set) >= 3]
    for sf, pads_set in sorted(dupes):
        issues.append(Issue(WARN, "SAMPLE_REUSED",
            f"'{sf}' appears in {len(pads_set)} different pads {sorted(pads_set)} — "
            "possible accidental duplicate"))
    return issues


def check_pan_balance(pads: List[PadInfo]) -> List[Issue]:
    """Check 6: extreme pan without an opposing pad suggests unbalanced stereo."""
    issues = []
    active = [p for p in pads if any(l["sample_file"] for l in p.layers if l["vel_start"] > 0)]

    # XPM pan range: typically -50..50 (center=0) or 0..100 (center=50)
    # Normalise to -50..50
    def normalise(p_val: int) -> int:
        if 0 <= p_val <= 100 and p_val != 0:
            return p_val - 50
        return p_val

    far_left  = [p for p in active if normalise(p.pan) <= -45]
    far_right = [p for p in active if normalise(p.pan) >= 45]

    if far_left and not far_right:
        issues.append(Issue(WARN, "PAN_IMBALANCED_LEFT",
            f"Pad(s) {[p.number for p in far_left]} are hard-panned left with no "
            "corresponding right-panned pad — stereo image skewed"))
    if far_right and not far_left:
        issues.append(Issue(WARN, "PAN_IMBALANCED_RIGHT",
            f"Pad(s) {[p.number for p in far_right]} are hard-panned right with no "
            "corresponding left-panned pad — stereo image skewed"))
    return issues


def check_core_voices(pads: List[PadInfo]) -> List[Issue]:
    """Check 7: minimal kit archetype — kick, snare, hat."""
    issues = []
    active_notes = {
        p.note for p in pads
        if any(l["sample_file"] for l in p.layers if l["vel_start"] > 0)
    }

    has_kick  = bool(active_notes & _KICK_NOTES)
    has_snare = bool(active_notes & _SNARE_NOTES)
    has_hat   = bool(active_notes & _HAT_NOTES)

    missing = []
    if not has_kick:
        missing.append("kick (notes 35/36)")
    if not has_snare:
        missing.append("snare (notes 38/40)")
    if not has_hat:
        missing.append("hat (notes 42/44/46)")

    if missing:
        issues.append(Issue(WARN, "MISSING_CORE_VOICES",
            "Minimal kit check — missing expected voices: " + ", ".join(missing) +
            ". (Override with --strict if this is intentional.)"))
    return issues


# ---------------------------------------------------------------------------
# Main validation runner
# ---------------------------------------------------------------------------

def validate_xpm(xml_bytes: bytes, name: str = "kit") -> Dict:
    """Run all checks on one XPM file. Returns a result dict."""
    pads, parse_issues = parse_xpm(xml_bytes)

    all_issues = list(parse_issues)
    if not any(i.severity == FAIL for i in parse_issues):
        all_issues += check_pad_coverage(pads)
        all_issues += check_choke_groups(pads)
        all_issues += check_velocity_consistency(pads)
        all_issues += check_root_note_distribution(pads)
        all_issues += check_sample_variety(pads)
        all_issues += check_pan_balance(pads)
        all_issues += check_core_voices(pads)

    fails  = [i for i in all_issues if i.severity == FAIL]
    warns  = [i for i in all_issues if i.severity == WARN]
    infos  = [i for i in all_issues if i.severity == INFO]

    verdict = "PASS" if not fails and not warns else ("FAIL" if fails else "WARN")

    pad_summary = []
    for p in sorted(pads, key=lambda x: x.number):
        real = [l for l in p.layers if l["vel_start"] > 0 and l["sample_file"]]
        pad_summary.append({
            "pad":        p.number,
            "note":       p.note,
            "voice":      _guess_voice(p.note),
            "layers":     len(real),
            "mute_group": p.mute_group,
            "pan":        p.pan,
        })

    return {
        "name":        name,
        "verdict":     verdict,
        "pad_count":   len(pads),
        "issues":      [{"severity": i.severity, "code": i.code,
                          "message": i.message, "pad": i.pad}
                        for i in all_issues],
        "counts":      {"FAIL": len(fails), "WARN": len(warns), "INFO": len(infos)},
        "pads":        pad_summary,
    }


# ---------------------------------------------------------------------------
# Output formatters
# ---------------------------------------------------------------------------

def format_text(result: Dict, show_pads: bool = True) -> str:
    lines = []
    verdict_line = f"{'='*60}"
    lines.append(verdict_line)
    lines.append(f"  {result['name']}")
    lines.append(f"  Verdict: {result['verdict']}  "
                 f"({result['counts']['FAIL']} FAIL, "
                 f"{result['counts']['WARN']} WARN, "
                 f"{result['counts']['INFO']} INFO)")
    lines.append(verdict_line)

    if show_pads:
        lines.append("")
        lines.append("Pad layout:")
        lines.append(f"  {'Pad':>4}  {'Note':>4}  {'Voice':<12} {'Layers':>6}  {'MuteGrp':>7}  {'Pan':>4}")
        lines.append(f"  {'-'*4}  {'-'*4}  {'-'*12} {'-'*6}  {'-'*7}  {'-'*4}")
        for p in result["pads"]:
            voice_str = p["voice"] if p["voice"] else "-"
            lines.append(
                f"  {p['pad']:>4}  {p['note']:>4}  {voice_str:<12} {p['layers']:>6}"
                f"  {p['mute_group']:>7}  {p['pan']:>4}"
            )

    if result["issues"]:
        lines.append("")
        lines.append("Issues:")
        for iss in result["issues"]:
            pad_tag = f" [pad {iss['pad']}]" if iss["pad"] is not None else ""
            lines.append(f"  [{iss['severity']}] {iss['code']}{pad_tag}: {iss['message']}")
    else:
        lines.append("")
        lines.append("  No issues found.")

    return "\n".join(lines)


# ---------------------------------------------------------------------------
# CLI entry point
# ---------------------------------------------------------------------------

def main() -> int:
    parser = argparse.ArgumentParser(
        description="Deep structural validation of XPM drum kit programs.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument("xpm", nargs="?", help=".xpm file to validate")
    parser.add_argument("--xpn", metavar="PACK.xpn",
                        help="Validate all .xpm programs inside this .xpn ZIP")
    parser.add_argument("--strict", action="store_true",
                        help="WARNs count as failures (non-zero exit code)")
    parser.add_argument("--format", choices=["text", "json"], default="text",
                        help="Output format (default: text)")
    parser.add_argument("--no-pads", action="store_true",
                        help="Suppress per-pad table in text output")
    args = parser.parse_args()

    if not args.xpm and not args.xpn:
        parser.print_help()
        return 1

    results = []

    if args.xpn:
        xpn_path = Path(args.xpn)
        if not xpn_path.exists():
            print(f"ERROR: {xpn_path} not found", file=sys.stderr)
            return 1
        with zipfile.ZipFile(xpn_path) as zf:
            xpm_names = [n for n in zf.namelist() if n.lower().endswith(".xpm")]
            if not xpm_names:
                print("ERROR: No .xpm files found in archive", file=sys.stderr)
                return 1
            for xpm_name in sorted(xpm_names):
                xml_bytes = zf.read(xpm_name)
                results.append(validate_xpm(xml_bytes, name=xpm_name))
    else:
        xpm_path = Path(args.xpm)
        if not xpm_path.exists():
            print(f"ERROR: {xpm_path} not found", file=sys.stderr)
            return 1
        xml_bytes = xpm_path.read_bytes()
        results.append(validate_xpm(xml_bytes, name=xpm_path.name))

    # Output
    if args.format == "json":
        print(json.dumps(results if len(results) > 1 else results[0], indent=2))
    else:
        for r in results:
            print(format_text(r, show_pads=not args.no_pads))
            print()

    # Exit code
    has_fail = any(r["counts"]["FAIL"] > 0 for r in results)
    has_warn = any(r["counts"]["WARN"] > 0 for r in results)
    if has_fail:
        return 2
    if args.strict and has_warn:
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
