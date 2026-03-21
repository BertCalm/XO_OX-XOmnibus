#!/usr/bin/env python3
"""
XPort Inspector — XO_OX Designs
The 5th Legendary Feature.

Inspects an exported XPN pack (or pre-export folder) and reports on the
completeness and quality of all four legendary export features:

  1. XDrip             — audio preview waveform presence
  2. DNA Inheritance   — 6D Sonic DNA populated in .xometa files
  3. Coupling Snapshots — coupling routes captured in .xometa files
  4. Rebirth Mode      — round-robin variations generated

Also validates core XPM compliance (the 3 critical rules) and produces an
overall readiness score (0–100) with a GO / NO-GO verdict.

Usage:
    # Inspect a .xpn archive
    python3 xport_inspector.py MyPack.xpn

    # Inspect a pre-export folder
    python3 xport_inspector.py ./export/MyPack/

    # Inspect a folder of .xometa preset files (pre-export)
    python3 xport_inspector.py Presets/XOmnibus/

    # JSON output for CI pipelines
    python3 xport_inspector.py MyPack.xpn --json

    # Verbose: show per-program detail
    python3 xport_inspector.py MyPack.xpn --verbose

XO_OX Designs — Inspect before you ship.
"""

import argparse
import json
import math
import os
import sys
import tempfile
import zipfile
from dataclasses import dataclass, field
from pathlib import Path
from typing import Dict, List, Optional, Tuple
from xml.etree import ElementTree as ET


# =============================================================================
# CONSTANTS
# =============================================================================

VERSION = "1.0.0"

# The 3 critical XPM rules — non-negotiable
XPM_RULE_KEYTRACK    = True
XPM_RULE_ROOT_NOTE   = 0
XPM_RULE_VEL_START   = 0   # empty layers must have VelStart=0

# Minimum DNA dimensions that should be non-default (0.5) for DNA Inheritance
# to be considered "active". We check that at least 3 of 6 dimensions are
# non-trivially set (i.e. not all 0.5).
DNA_ACTIVE_THRESHOLD = 0.02   # distance from 0.5 to count as "set"
DNA_ACTIVE_MIN_DIMS  = 3      # minimum non-trivial dimensions required

# Round-robin count threshold for Rebirth to be considered active
RR_REBIRTH_MIN = 2


# =============================================================================
# DATA MODELS
# =============================================================================

@dataclass
class XPMLayerInfo:
    sample_path: str
    vel_start: int
    vel_end: int
    root_note: int
    key_track: bool
    is_empty: bool          # True if no sample assigned
    round_robin_index: int  # 0-based


@dataclass
class XPMProgramInfo:
    name: str
    path: str                        # path inside ZIP (or on disk)
    kind: str                        # "drum" | "keygroup"
    layers: List[XPMLayerInfo] = field(default_factory=list)
    pad_count: int = 0
    has_preview: bool = False
    preview_path: str = ""
    # per-pad max round-robin index (drum) or global (keygroup)
    max_rr: int = 0


@dataclass
class XMetaInfo:
    name: str
    path: str
    engines: List[str]
    mood: str
    dna: Dict[str, float]           # brightness, warmth, movement, density, space, aggression
    coupling_pairs: List[dict]
    has_coupling: bool
    dna_active: bool                # True if DNA Inheritance fired


@dataclass
class InspectionReport:
    source: str
    programs: List[XPMProgramInfo]
    xmetas: List[XMetaInfo]
    # Legendary feature scores (0.0 – 1.0)
    drip_score: float       = 0.0
    dna_score: float        = 0.0
    coupling_score: float   = 0.0
    rebirth_score: float    = 0.0
    # XPM compliance
    xpm_violations: List[str] = field(default_factory=list)
    # Overall
    readiness_score: int    = 0     # 0-100
    verdict: str            = "NO-GO"
    findings: List[str]     = field(default_factory=list)
    warnings: List[str]     = field(default_factory=list)


# =============================================================================
# XPM PARSING
# =============================================================================

def _parse_bool(val: str) -> bool:
    return val.strip().lower() in ("true", "1", "yes")


def _parse_xpm(xpm_path: str, xml_text: str) -> XPMProgramInfo:
    """Parse an XPM XML string and return a structured program info."""
    try:
        root = ET.fromstring(xml_text)
    except ET.ParseError as exc:
        prog = XPMProgramInfo(name=Path(xpm_path).stem, path=xpm_path, kind="unknown")
        return prog

    prog_name = root.findtext("Name") or Path(xpm_path).stem
    prog_type = root.tag  # "DrumProgram" or "KeygroupProgram"
    kind = "drum" if "Drum" in prog_type else "keygroup"

    prog = XPMProgramInfo(name=prog_name, path=xpm_path, kind=kind)

    layers: List[XPMLayerInfo] = []

    if kind == "drum":
        pads = root.findall(".//Pad")
        prog.pad_count = len(pads)
        for pad in pads:
            for layer_el in pad.findall("Layer"):
                sample_path = layer_el.findtext("SampleFile") or ""
                is_empty = (sample_path.strip() == "")
                try:
                    vel_start = int(layer_el.findtext("VelStart") or "0")
                    vel_end   = int(layer_el.findtext("VelEnd")   or "127")
                    rr_idx    = int(layer_el.findtext("RoundRobinIndex") or "0")
                except ValueError:
                    vel_start, vel_end, rr_idx = 0, 127, 0

                try:
                    root_note = int(layer_el.findtext("RootNote") or "0")
                except ValueError:
                    root_note = 0

                key_track_str = layer_el.findtext("KeyTrack") or "False"
                key_track = _parse_bool(key_track_str)

                layers.append(XPMLayerInfo(
                    sample_path=sample_path,
                    vel_start=vel_start,
                    vel_end=vel_end,
                    root_note=root_note,
                    key_track=key_track,
                    is_empty=is_empty,
                    round_robin_index=rr_idx,
                ))
        if layers:
            prog.max_rr = max(l.round_robin_index for l in layers)

    else:  # keygroup
        keygroups = root.findall(".//Keygroup")
        prog.pad_count = len(keygroups)
        for kg in keygroups:
            for layer_el in kg.findall("Layer"):
                sample_path = layer_el.findtext("SampleFile") or ""
                is_empty = (sample_path.strip() == "")
                try:
                    vel_start = int(layer_el.findtext("VelStart") or "0")
                    vel_end   = int(layer_el.findtext("VelEnd")   or "127")
                except ValueError:
                    vel_start, vel_end = 0, 127

                try:
                    root_note = int(layer_el.findtext("RootNote") or "0")
                except ValueError:
                    root_note = 0

                key_track_str = layer_el.findtext("KeyTrack") or "False"
                key_track = _parse_bool(key_track_str)

                layers.append(XPMLayerInfo(
                    sample_path=sample_path,
                    vel_start=vel_start,
                    vel_end=vel_end,
                    root_note=root_note,
                    key_track=key_track,
                    is_empty=is_empty,
                    round_robin_index=0,
                ))

    prog.layers = layers
    return prog


# =============================================================================
# XOMETA PARSING
# =============================================================================

def _parse_xometa(path: str, text: str) -> Optional[XMetaInfo]:
    """Parse a .xometa JSON string and return structured metadata."""
    try:
        data = json.loads(text)
    except json.JSONDecodeError:
        return None

    name    = data.get("name", Path(path).stem)
    engines = data.get("engines", [])
    mood    = data.get("mood", "")
    dna_raw = data.get("dna", {})

    dna = {
        "brightness":  float(dna_raw.get("brightness",  0.5)),
        "warmth":      float(dna_raw.get("warmth",      0.5)),
        "movement":    float(dna_raw.get("movement",    0.5)),
        "density":     float(dna_raw.get("density",     0.5)),
        "space":       float(dna_raw.get("space",       0.5)),
        "aggression":  float(dna_raw.get("aggression",  0.5)),
    }

    coupling_pairs = data.get("couplingPairs", [])
    has_coupling = len(coupling_pairs) > 0

    # DNA Inheritance check: at least DNA_ACTIVE_MIN_DIMS dimensions
    # significantly deviate from the 0.5 default
    active_dims = sum(
        1 for v in dna.values() if abs(v - 0.5) > DNA_ACTIVE_THRESHOLD
    )
    dna_active = (active_dims >= DNA_ACTIVE_MIN_DIMS)

    return XMetaInfo(
        name=name,
        path=path,
        engines=engines,
        mood=mood,
        dna=dna,
        coupling_pairs=coupling_pairs,
        has_coupling=has_coupling,
        dna_active=dna_active,
    )


# =============================================================================
# PACK LOADING
# =============================================================================

def _load_xpn(zip_path: str) -> Tuple[List[XPMProgramInfo], List[XMetaInfo]]:
    """Open an .xpn ZIP archive and extract program + metadata info."""
    programs: List[XPMProgramInfo] = []
    xmetas: List[XMetaInfo] = []

    with zipfile.ZipFile(zip_path, "r") as zf:
        names = set(zf.namelist())
        preview_names = {n for n in names if n.lower().endswith(".wav") and "/Programs/" in n}

        for name in names:
            if name.endswith(".xpm"):
                xml_text = zf.read(name).decode("utf-8", errors="replace")
                prog = _parse_xpm(name, xml_text)

                # Check for a matching preview WAV alongside the XPM
                stem = Path(name).stem
                preview_candidate = str(Path(name).parent / (stem + ".wav"))
                prog.has_preview = (preview_candidate in names) or any(
                    p.endswith(stem + ".wav") for p in preview_names
                )
                if prog.has_preview:
                    prog.preview_path = preview_candidate

                programs.append(prog)

            elif name.endswith(".xometa"):
                text = zf.read(name).decode("utf-8", errors="replace")
                meta = _parse_xometa(name, text)
                if meta:
                    xmetas.append(meta)

    return programs, xmetas


def _load_folder(folder: str) -> Tuple[List[XPMProgramInfo], List[XMetaInfo]]:
    """Scan a folder tree for .xpm and .xometa files."""
    programs: List[XPMProgramInfo] = []
    xmetas: List[XMetaInfo] = []

    root = Path(folder)

    for xpm_path in sorted(root.rglob("*.xpm")):
        xml_text = xpm_path.read_text(encoding="utf-8", errors="replace")
        prog = _parse_xpm(str(xpm_path), xml_text)

        preview_path = xpm_path.parent / (xpm_path.stem + ".wav")
        prog.has_preview = preview_path.exists()
        if prog.has_preview:
            prog.preview_path = str(preview_path)

        programs.append(prog)

    for meta_path in sorted(root.rglob("*.xometa")):
        text = meta_path.read_text(encoding="utf-8", errors="replace")
        meta = _parse_xometa(str(meta_path), text)
        if meta:
            xmetas.append(meta)

    return programs, xmetas


# =============================================================================
# FEATURE SCORING
# =============================================================================

def _score_drip(programs: List[XPMProgramInfo]) -> Tuple[float, List[str]]:
    """Score XDrip preview coverage (0.0 – 1.0)."""
    if not programs:
        return 0.0, ["No programs found — cannot score XDrip"]
    with_preview = sum(1 for p in programs if p.has_preview)
    score = with_preview / len(programs)
    findings = []
    if score < 1.0:
        missing = [p.name for p in programs if not p.has_preview]
        findings.append(f"XDrip: {len(missing)} program(s) missing preview WAV: {', '.join(missing[:5])}" +
                         ("…" if len(missing) > 5 else ""))
    return score, findings


def _score_dna(xmetas: List[XMetaInfo]) -> Tuple[float, List[str]]:
    """Score DNA Inheritance completeness (0.0 – 1.0)."""
    if not xmetas:
        return 0.0, ["No .xometa files found — DNA Inheritance cannot be assessed"]
    active = sum(1 for m in xmetas if m.dna_active)
    score = active / len(xmetas)
    findings = []
    if score < 1.0:
        inactive = [m.name for m in xmetas if not m.dna_active]
        findings.append(f"DNA Inheritance: {len(inactive)} preset(s) have default/flat DNA: "
                         f"{', '.join(inactive[:5])}" + ("…" if len(inactive) > 5 else ""))
    return score, findings


def _score_coupling(xmetas: List[XMetaInfo]) -> Tuple[float, List[str]]:
    """Score Coupling Snapshot coverage (0.0 – 1.0).

    Only multi-engine presets are expected to have coupling data.
    Single-engine presets are excluded from the denominator.
    """
    multi_engine = [m for m in xmetas if len(m.engines) > 1]
    if not multi_engine:
        return 1.0, ["No multi-engine presets — Coupling Snapshot N/A (score: 1.0)"]

    coupled = sum(1 for m in multi_engine if m.has_coupling)
    score = coupled / len(multi_engine)
    findings = []
    if score < 1.0:
        uncoupled = [m.name for m in multi_engine if not m.has_coupling]
        findings.append(f"Coupling Snapshot: {len(uncoupled)} multi-engine preset(s) missing coupling data: "
                         f"{', '.join(uncoupled[:5])}" + ("…" if len(uncoupled) > 5 else ""))
    return score, findings


def _score_rebirth(programs: List[XPMProgramInfo]) -> Tuple[float, List[str]]:
    """Score Rebirth Mode round-robin coverage (0.0 – 1.0).

    Drum programs: at least one pad should have RR > 1.
    Keygroup programs: we check layer count as a proxy (≥2 layers).
    """
    if not programs:
        return 0.0, ["No programs found — Rebirth cannot be assessed"]

    rr_active = []
    rr_inactive = []
    for prog in programs:
        if prog.kind == "drum":
            if prog.max_rr >= RR_REBIRTH_MIN - 1:
                rr_active.append(prog.name)
            else:
                rr_inactive.append(prog.name)
        else:
            active_layers = sum(1 for l in prog.layers if not l.is_empty)
            if active_layers >= RR_REBIRTH_MIN:
                rr_active.append(prog.name)
            else:
                rr_inactive.append(prog.name)

    score = len(rr_active) / len(programs) if programs else 0.0
    findings = []
    if score < 1.0:
        findings.append(f"Rebirth Mode: {len(rr_inactive)} program(s) have no round-robin / multi-layer variation: "
                         f"{', '.join(rr_inactive[:5])}" + ("…" if len(rr_inactive) > 5 else ""))
    return score, findings


def _check_xpm_rules(programs: List[XPMProgramInfo]) -> List[str]:
    """Validate the 3 critical XPM rules across all programs."""
    violations: List[str] = []
    for prog in programs:
        for i, layer in enumerate(prog.layers):
            if layer.is_empty:
                if layer.vel_start != XPM_RULE_VEL_START:
                    violations.append(
                        f"[{prog.name}] Rule #3: empty layer {i} has VelStart={layer.vel_start} (must be 0)"
                    )
            else:
                if prog.kind == "keygroup":
                    if layer.root_note != XPM_RULE_ROOT_NOTE:
                        violations.append(
                            f"[{prog.name}] Rule #2: layer {i} RootNote={layer.root_note} (must be 0)"
                        )
                    if not layer.key_track:
                        violations.append(
                            f"[{prog.name}] Rule #1: layer {i} KeyTrack=False (must be True)"
                        )
    return violations


# =============================================================================
# REPORT ASSEMBLY
# =============================================================================

def _compute_readiness(drip: float, dna: float, coupling: float, rebirth: float,
                        xpm_violations: List[str]) -> Tuple[int, str]:
    """Compute 0-100 readiness score and GO / NO-GO verdict."""
    # Weighted average: each feature 20%, XPM compliance 20%
    xpm_score = 1.0 if not xpm_violations else max(0.0, 1.0 - len(xpm_violations) * 0.1)
    raw = (drip * 20 + dna * 20 + coupling * 20 + rebirth * 20 + xpm_score * 20)
    score = int(round(raw))
    # GO requires >= 80 and zero XPM CRITICAL violations
    verdict = "GO" if score >= 80 and not xpm_violations else "NO-GO"
    return score, verdict


def inspect(source: str) -> InspectionReport:
    """Run a full inspection on a .xpn archive or folder and return a report."""
    if source.endswith(".xpn") and zipfile.is_zipfile(source):
        programs, xmetas = _load_xpn(source)
    elif os.path.isdir(source):
        programs, xmetas = _load_folder(source)
    else:
        raise ValueError(f"Input must be a .xpn archive or a directory, got: {source}")

    drip_score,    drip_findings    = _score_drip(programs)
    dna_score,     dna_findings     = _score_dna(xmetas)
    coupling_score, coupling_findings = _score_coupling(xmetas)
    rebirth_score, rebirth_findings = _score_rebirth(programs)
    xpm_violations                  = _check_xpm_rules(programs)

    all_findings = drip_findings + dna_findings + coupling_findings + rebirth_findings
    all_warnings = [f"XPM Rule Violation: {v}" for v in xpm_violations]

    readiness, verdict = _compute_readiness(
        drip_score, dna_score, coupling_score, rebirth_score, xpm_violations
    )

    return InspectionReport(
        source=source,
        programs=programs,
        xmetas=xmetas,
        drip_score=drip_score,
        dna_score=dna_score,
        coupling_score=coupling_score,
        rebirth_score=rebirth_score,
        xpm_violations=xpm_violations,
        readiness_score=readiness,
        verdict=verdict,
        findings=all_findings,
        warnings=all_warnings,
    )


# =============================================================================
# DISPLAY
# =============================================================================

_RESET  = "\033[0m"
_BOLD   = "\033[1m"
_GREEN  = "\033[32m"
_YELLOW = "\033[33m"
_RED    = "\033[31m"
_CYAN   = "\033[36m"
_DIM    = "\033[2m"

_NO_COLOR = not sys.stdout.isatty()


def _c(text: str, code: str) -> str:
    if _NO_COLOR:
        return text
    return f"{code}{text}{_RESET}"


def _bar(score: float, width: int = 20) -> str:
    filled = int(round(score * width))
    bar = "█" * filled + "░" * (width - filled)
    pct = int(round(score * 100))
    if pct >= 80:
        color = _GREEN
    elif pct >= 50:
        color = _YELLOW
    else:
        color = _RED
    return _c(f"[{bar}] {pct:3d}%", color)


def _feature_row(label: str, score: float, findings: List[str]) -> None:
    status_icon = "✓" if score >= 0.8 else ("~" if score >= 0.5 else "✗")
    color = _GREEN if score >= 0.8 else (_YELLOW if score >= 0.5 else _RED)
    print(f"  {_c(status_icon, color)} {label:<28s} {_bar(score)}")
    for f in findings:
        print(f"    {_c('↳', _DIM)} {f}")


def print_report(report: InspectionReport, verbose: bool = False) -> None:
    """Pretty-print the inspection report to stdout."""
    source_name = Path(report.source).name or report.source
    prog_count  = len(report.programs)
    meta_count  = len(report.xmetas)

    print()
    print(_c("╔══════════════════════════════════════════════════════╗", _CYAN))
    print(_c("║  XPort Inspector  —  XO_OX Designs                  ║", _CYAN))
    print(_c("╚══════════════════════════════════════════════════════╝", _RESET))
    print()
    print(f"  Source   : {_c(source_name, _BOLD)}")
    print(f"  Programs : {prog_count}  (.xometa presets: {meta_count})")
    print()

    # ── Legendary Features ──────────────────────────────────────────────────
    print(_c("  ─── Legendary Features ─────────────────────────────", _DIM))
    _feature_row("XDrip (preview WAV)",     report.drip_score,     [])
    _feature_row("DNA Inheritance",         report.dna_score,      [])
    _feature_row("Coupling Snapshots",      report.coupling_score, [])
    _feature_row("Rebirth Mode (RR/layers)",report.rebirth_score,  [])
    print()

    # ── XPM Compliance ───────────────────────────────────────────────────
    print(_c("  ─── XPM Compliance (3 Critical Rules) ──────────────", _DIM))
    if not report.xpm_violations:
        print(f"  {_c('✓', _GREEN)} All XPM rules pass")
    else:
        for v in report.xpm_violations[:10]:
            print(f"  {_c('✗', _RED)} {v}")
        if len(report.xpm_violations) > 10:
            print(f"  {_c('…', _DIM)} and {len(report.xpm_violations) - 10} more violations")
    print()

    # ── Findings ─────────────────────────────────────────────────────────
    if report.findings:
        print(_c("  ─── Findings ────────────────────────────────────────", _DIM))
        for f in report.findings:
            print(f"  {_c('●', _YELLOW)} {f}")
        print()

    # ── Verbose: program table ────────────────────────────────────────────
    if verbose and report.programs:
        print(_c("  ─── Program Detail ──────────────────────────────────", _DIM))
        col_w = max(20, max(len(p.name) for p in report.programs) + 2)
        hdr = f"  {'Program':<{col_w}} {'Kind':<10} {'Pads':<6} {'Layers':<8} {'Preview':<9} {'RR Max'}"
        print(_c(hdr, _DIM))
        print(_c("  " + "─" * (col_w + 45), _DIM))
        for prog in report.programs:
            active_layers = sum(1 for l in prog.layers if not l.is_empty)
            drip_mark = _c("✓", _GREEN) if prog.has_preview else _c("✗", _RED)
            rr_mark = str(prog.max_rr + 1) if prog.kind == "drum" else "n/a"
            print(f"  {prog.name:<{col_w}} {prog.kind:<10} {prog.pad_count:<6} "
                  f"{active_layers:<8} {drip_mark}        {rr_mark}")
        print()

    if verbose and report.xmetas:
        print(_c("  ─── DNA Detail ──────────────────────────────────────", _DIM))
        col_w = max(20, max(len(m.name) for m in report.xmetas) + 2)
        hdr = f"  {'Preset':<{col_w}} {'Engines':<20} {'DNA':<6} {'Coupled'}"
        print(_c(hdr, _DIM))
        print(_c("  " + "─" * (col_w + 40), _DIM))
        for m in report.xmetas:
            dna_mark = _c("✓", _GREEN) if m.dna_active else _c("✗", _RED)
            coup_mark = _c("✓", _GREEN) if m.has_coupling else ("—" if len(m.engines) <= 1 else _c("✗", _RED))
            eng_str = "+".join(m.engines[:2]) + ("…" if len(m.engines) > 2 else "")
            print(f"  {m.name:<{col_w}} {eng_str:<20} {dna_mark}      {coup_mark}")
        print()

    # ── Verdict ──────────────────────────────────────────────────────────
    score_bar = _bar(report.readiness_score / 100)
    verdict_color = _GREEN if report.verdict == "GO" else _RED
    print(_c("  ─── Readiness ───────────────────────────────────────", _DIM))
    print(f"  Score  : {score_bar}")
    print(f"  Verdict: {_c(_BOLD + report.verdict + _RESET, verdict_color)}")
    print()


# =============================================================================
# JSON OUTPUT
# =============================================================================

def report_to_dict(report: InspectionReport) -> dict:
    return {
        "source": report.source,
        "programs": len(report.programs),
        "xmetas": len(report.xmetas),
        "legendary_features": {
            "xdrip_preview":       round(report.drip_score,     3),
            "dna_inheritance":     round(report.dna_score,      3),
            "coupling_snapshots":  round(report.coupling_score, 3),
            "rebirth_mode":        round(report.rebirth_score,  3),
        },
        "xpm_violations": report.xpm_violations,
        "findings": report.findings,
        "warnings": report.warnings,
        "readiness_score": report.readiness_score,
        "verdict": report.verdict,
    }


# =============================================================================
# CLI ENTRY POINT
# =============================================================================

def main() -> int:
    parser = argparse.ArgumentParser(
        description="XPort Inspector — XO_OX Designs. Inspect an XPN pack for export readiness.",
        epilog=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument("input",
        help="Path to a .xpn archive, export folder, or Presets/ directory")
    parser.add_argument("--json", action="store_true",
        help="Output machine-readable JSON instead of human-readable report")
    parser.add_argument("--verbose", "-v", action="store_true",
        help="Show per-program and per-preset detail tables")
    parser.add_argument("--version", action="version", version=f"%(prog)s {VERSION}")
    args = parser.parse_args()

    input_path = args.input
    if not os.path.exists(input_path):
        print(f"Error: path not found: {input_path}", file=sys.stderr)
        return 2

    try:
        report = inspect(input_path)
    except ValueError as exc:
        print(f"Error: {exc}", file=sys.stderr)
        return 2

    if args.json:
        print(json.dumps(report_to_dict(report), indent=2))
    else:
        print_report(report, verbose=args.verbose)

    # Exit codes: 0 = GO, 1 = NO-GO
    return 0 if report.verdict == "GO" else 1


if __name__ == "__main__":
    sys.exit(main())
