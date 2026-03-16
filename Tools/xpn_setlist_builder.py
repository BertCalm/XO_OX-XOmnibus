#!/usr/bin/env python3
"""
XPN Setlist Builder — XO_OX Designs
Set-long kit validation and loading order builder for MPC.

Parses a JSON setlist, opens each XPN ZIP, estimates RAM usage from XPM
program files, validates against device ceiling, and outputs:
  - setlist_validation.txt   per-pack RAM breakdown + PASS/FAIL
  - loading_order.txt        recommended load sequence (largest-first)
  - swap_guide.txt           mid-set swap plan when total > ceiling
  - setlist_summary.txt      one-page overview

Usage:
  python xpn_setlist_builder.py --setlist my_set.json [--device MPC_Live_III] [--output ./out/] [--dry-run]
  python xpn_setlist_builder.py --setlist my_set.json   # auto-detects device from JSON
"""

import argparse
import json
import os
import sys
import zipfile
import xml.etree.ElementTree as ET
from dataclasses import dataclass, field
from pathlib import Path
from typing import Dict, List, Optional

# ---------------------------------------------------------------------------
# Device profiles
# ---------------------------------------------------------------------------

DEVICE_PROFILES: Dict[str, Dict] = {
    "MPC_Live_III": {
        "ram_mb": 2800,
        "cores": 8,
        "load_speed": "fast",
        "description": "MPC Live III — 2800 MB ceiling, 8-core, fast load",
    },
    "MPC_X": {
        "ram_mb": 2400,
        "cores": 8,
        "load_speed": "normal",
        "description": "MPC X — 2400 MB ceiling, 8-core",
    },
    "MPC_One": {
        "ram_mb": 1200,
        "cores": 4,
        "load_speed": "slow",
        "description": "MPC One — 1200 MB ceiling, 4-core, slow load",
    },
}

# Conservative per-WAV estimate when actual file is not present in ZIP
DEFAULT_WAV_MB = 4.0
# Fallback when computing from WAV spec: 44100 Hz × 2ch × 2bytes × 4sec ≈ 0.67 MB
FALLBACK_WAV_MB = 44100 * 2 * 2 * 4 / (1024 * 1024)
# Base overhead charged per MPC program
PROGRAM_BASE_MB = 2.0

SEPARATOR = "=" * 72
THIN_SEP  = "-" * 72


# ---------------------------------------------------------------------------
# Data classes
# ---------------------------------------------------------------------------

@dataclass
class WavInfo:
    path: str
    size_mb: float
    estimated: bool     # True when size was not read from an actual WAV in the ZIP


@dataclass
class ProgramInfo:
    xpm_name: str
    wav_refs: List[WavInfo] = field(default_factory=list)

    @property
    def ram_mb(self) -> float:
        return PROGRAM_BASE_MB + sum(w.size_mb for w in self.wav_refs)

    @property
    def wav_count(self) -> int:
        return len(self.wav_refs)

    @property
    def estimated_wav_count(self) -> int:
        return sum(1 for w in self.wav_refs if w.estimated)


@dataclass
class PackInfo:
    name: str
    path: str
    programs: List[ProgramInfo] = field(default_factory=list)
    error: Optional[str] = None

    @property
    def total_ram_mb(self) -> float:
        return sum(p.ram_mb for p in self.programs)

    @property
    def program_count(self) -> int:
        return len(self.programs)

    @property
    def total_wav_refs(self) -> int:
        return sum(p.wav_count for p in self.programs)


# ---------------------------------------------------------------------------
# XPN / ZIP parsing
# ---------------------------------------------------------------------------

def _build_wav_index(zf: zipfile.ZipFile) -> Dict[str, float]:
    """Map lowercase path/basename -> uncompressed MB for every WAV in the ZIP."""
    index: Dict[str, float] = {}
    for entry in zf.namelist():
        if not entry.lower().endswith(".wav"):
            continue
        info = zf.getinfo(entry)
        size_mb = info.file_size / (1024 * 1024) if info.file_size > 0 else DEFAULT_WAV_MB
        index[entry.lower()] = size_mb
        basename = os.path.basename(entry).lower()
        if basename not in index:
            index[basename] = size_mb
    return index


def _parse_xpm_wavs(xpm_xml: str, wav_index: Dict[str, float]) -> List[WavInfo]:
    """Extract unique WAV references from XPM XML and resolve their sizes."""
    wavs: List[WavInfo] = []
    seen: set = set()
    try:
        root = ET.fromstring(xpm_xml)
    except ET.ParseError:
        return wavs

    for tag in ("SampleFile", "FileName", "File"):
        for elem in root.iter(tag):
            text = (elem.text or "").strip()
            if not text or not text.lower().endswith(".wav"):
                continue
            norm = text.replace("\\", "/")
            if norm in seen:
                continue
            seen.add(norm)

            # Try full path, then basename
            size_mb = wav_index.get(norm.lower())
            estimated = False
            if size_mb is None:
                size_mb = wav_index.get(os.path.basename(norm).lower())
            if size_mb is None:
                size_mb = DEFAULT_WAV_MB
                estimated = True

            wavs.append(WavInfo(path=norm, size_mb=size_mb, estimated=estimated))

    return wavs


def parse_xpn_zip(name: str, zip_path: str) -> PackInfo:
    """Open an XPN ZIP and build a PackInfo describing all programs and WAVs."""
    pack = PackInfo(name=name, path=zip_path)

    if not os.path.isfile(zip_path):
        pack.error = f"File not found: {zip_path}"
        return pack

    try:
        with zipfile.ZipFile(zip_path, "r") as zf:
            wav_index = _build_wav_index(zf)
            xpm_entries = [e for e in zf.namelist() if e.lower().endswith(".xpm")]

            if not xpm_entries:
                pack.error = "No XPM files found in ZIP"
                return pack

            for xpm_entry in xpm_entries:
                xpm_name = os.path.basename(xpm_entry)
                try:
                    xpm_xml = zf.read(xpm_entry).decode("utf-8", errors="replace")
                except Exception:
                    pack.programs.append(ProgramInfo(xpm_name=xpm_name))
                    continue
                wav_refs = _parse_xpm_wavs(xpm_xml, wav_index)
                pack.programs.append(ProgramInfo(xpm_name=xpm_name, wav_refs=wav_refs))

    except zipfile.BadZipFile as exc:
        pack.error = f"Bad ZIP file: {exc}"
    except Exception as exc:
        pack.error = f"Error reading pack: {exc}"

    return pack


# ---------------------------------------------------------------------------
# Formatting helpers
# ---------------------------------------------------------------------------

def _mb_str(mb: float) -> str:
    if mb >= 1000:
        return f"{mb / 1024:.2f} GB"
    return f"{mb:.1f} MB"


# ---------------------------------------------------------------------------
# Report builders
# ---------------------------------------------------------------------------

def build_validation_report(
    packs: List[PackInfo], device_name: str, ceiling_mb: float, set_name: str
) -> str:
    lines = [
        SEPARATOR,
        "  SETLIST VALIDATION REPORT",
        f"  Set: {set_name}",
        f"  Device: {device_name}  |  RAM Ceiling: {_mb_str(ceiling_mb)}",
        SEPARATOR,
        "",
    ]

    total_mb = 0.0
    for i, pack in enumerate(packs, 1):
        lines.append(f"[{i:02d}] {pack.name}")
        lines.append(f"     Path: {pack.path}")
        if pack.error:
            lines.append(f"     ERROR: {pack.error}")
            lines.append("")
            continue

        lines.append(f"     Programs : {pack.program_count}")
        lines.append(f"     WAV refs : {pack.total_wav_refs}")
        for prog in pack.programs:
            est_note = (
                f"  ({prog.estimated_wav_count} estimated)"
                if prog.estimated_wav_count
                else ""
            )
            lines.append(
                f"       {prog.xpm_name:<40}  {prog.wav_count:>3} WAVs"
                f"  {_mb_str(prog.ram_mb):>10}{est_note}"
            )
        lines.append(f"     Pack RAM : {_mb_str(pack.total_ram_mb)}")
        total_mb += pack.total_ram_mb
        lines.append("")

    headroom = ceiling_mb - total_mb
    lines += [
        THIN_SEP,
        f"  TOTAL ESTIMATED RAM : {_mb_str(total_mb)}",
        f"  DEVICE CEILING      : {_mb_str(ceiling_mb)}",
        f"  HEADROOM            : {_mb_str(headroom)}",
        "",
    ]

    if total_mb <= ceiling_mb:
        lines.append("  RESULT: PASS — entire setlist fits within device RAM.")
    else:
        over = total_mb - ceiling_mb
        lines.append(f"  RESULT: FAIL — exceeds ceiling by {_mb_str(over)}.")
        lines.append("  See swap_guide.txt for mid-set swap recommendations.")

    lines.append(SEPARATOR)
    return "\n".join(lines)


def build_loading_order_report(
    packs: List[PackInfo], device_name: str, set_name: str
) -> str:
    valid   = [p for p in packs if not p.error]
    errored = [p for p in packs if p.error]
    ordered = sorted(valid, key=lambda p: p.total_ram_mb, reverse=True)

    lines = [
        SEPARATOR,
        "  RECOMMENDED LOADING ORDER",
        f"  Set: {set_name}  |  Device: {device_name}",
        "  Strategy: load largest packs first to minimise RAM fragmentation",
        SEPARATOR,
        "",
    ]

    cumulative = 0.0
    for rank, pack in enumerate(ordered, 1):
        cumulative += pack.total_ram_mb
        lines.append(
            f"  {rank:>2}. {pack.name:<36}  {_mb_str(pack.total_ram_mb):>10}"
            f"  (cumulative: {_mb_str(cumulative)})"
        )

    if errored:
        lines += ["", "  SKIPPED (errors):"]
        for pack in errored:
            lines.append(f"       {pack.name}  — {pack.error}")

    lines += [
        "",
        THIN_SEP,
        "  NOTE: On MPC One (slow load), front-load all packs before performance.",
        "  On MPC Live III / MPC X, packs can be swapped mid-set in under 3 sec.",
        SEPARATOR,
    ]
    return "\n".join(lines)


def build_swap_guide(
    packs: List[PackInfo], device_name: str, ceiling_mb: float, set_name: str
) -> str:
    valid   = [p for p in packs if not p.error]
    ordered = sorted(valid, key=lambda p: p.total_ram_mb, reverse=True)
    total_mb = sum(p.total_ram_mb for p in valid)

    lines = [
        SEPARATOR,
        "  MID-SET SWAP GUIDE",
        f"  Set: {set_name}  |  Device: {device_name}",
        SEPARATOR,
        "",
    ]

    if total_mb <= ceiling_mb:
        lines += [
            "  No swaps needed — all packs fit within device RAM.",
            SEPARATOR,
        ]
        return "\n".join(lines)

    over_mb = total_mb - ceiling_mb
    lines += [
        f"  Total estimated RAM : {_mb_str(total_mb)}",
        f"  Device ceiling      : {_mb_str(ceiling_mb)}",
        f"  Overage             : {_mb_str(over_mb)}",
        "",
        "  SWAP STRATEGY",
        "  Load the largest packs at set start. Swap smaller packs in as needed.",
        "  Suggested split:",
        "",
    ]

    # Greedy fill: fit as many large packs as possible into opening slot
    set_start: List[PackInfo] = []
    swap_in: List[PackInfo] = []
    running = 0.0
    for pack in ordered:
        if running + pack.total_ram_mb <= ceiling_mb:
            set_start.append(pack)
            running += pack.total_ram_mb
        else:
            swap_in.append(pack)

    lines.append("  LOAD AT SET START:")
    for i, pack in enumerate(set_start, 1):
        lines.append(f"    {i:>2}. {pack.name:<36}  {_mb_str(pack.total_ram_mb)}")

    lines += ["", "  SWAP IN MID-SET (unload a set-start pack first):"]
    for i, pack in enumerate(swap_in, 1):
        # Recommend unloading the set-start pack closest in size
        candidate = min(set_start, key=lambda p: abs(p.total_ram_mb - pack.total_ram_mb))
        lines += [
            f"    {i:>2}. Load  : {pack.name:<36}  {_mb_str(pack.total_ram_mb)}",
            f"        Unload: {candidate.name:<36}  {_mb_str(candidate.total_ram_mb)}",
            f"        Suggested swap point: after finishing songs that use {candidate.name}",
            f"        (e.g., after pad 8 in Bank B, unload {candidate.name}, load {pack.name})",
            "",
        ]

    lines += [
        THIN_SEP,
        "  TIP: Plan swap points between songs or during interludes.",
        "  TIP: On MPC One, pre-queue the swap before the last hit of a section.",
        SEPARATOR,
    ]
    return "\n".join(lines)


def build_summary(
    packs: List[PackInfo], device_name: str, ceiling_mb: float, set_name: str
) -> str:
    valid        = [p for p in packs if not p.error]
    errored      = [p for p in packs if p.error]
    total_mb     = sum(p.total_ram_mb for p in valid)
    total_progs  = sum(p.program_count for p in valid)
    total_wavs   = sum(p.total_wav_refs for p in valid)
    passed       = total_mb <= ceiling_mb

    lines = [
        SEPARATOR,
        f"  SETLIST SUMMARY — {set_name}",
        SEPARATOR,
        f"  Device         : {device_name}",
        f"  RAM ceiling    : {_mb_str(ceiling_mb)}",
        f"  Packs          : {len(packs)}  ({len(errored)} errors)",
        f"  Programs       : {total_progs}",
        f"  WAV references : {total_wavs}",
        f"  Total RAM est. : {_mb_str(total_mb)}",
        f"  Headroom       : {_mb_str(ceiling_mb - total_mb)}",
        f"  Result         : {'PASS' if passed else 'FAIL — see swap_guide.txt'}",
        "",
    ]

    if errored:
        lines.append("  Packs with errors:")
        for p in errored:
            lines.append(f"    - {p.name}: {p.error}")
        lines.append("")

    lines.append("  Pack overview (largest first):")
    for pack in sorted(packs, key=lambda p: p.total_ram_mb, reverse=True):
        status = "ERROR" if pack.error else _mb_str(pack.total_ram_mb)
        lines.append(f"    {pack.name:<40}  {status}")

    lines.append(SEPARATOR)
    return "\n".join(lines)


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def resolve_path(base_dir: str, path: str) -> str:
    if os.path.isabs(path):
        return path
    return os.path.normpath(os.path.join(base_dir, path))


def main() -> int:
    parser = argparse.ArgumentParser(
        description="XPN Setlist Builder — MPC RAM validation and loading order tool"
    )
    parser.add_argument("--setlist", required=True, help="Path to setlist JSON file")
    parser.add_argument(
        "--device",
        choices=list(DEVICE_PROFILES.keys()),
        default=None,
        help="Device profile (overrides 'device' field in setlist JSON)",
    )
    parser.add_argument(
        "--output",
        default=".",
        help="Output directory for report files (default: current directory)",
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Print estimates to stdout without writing files",
    )
    args = parser.parse_args()

    setlist_path = os.path.abspath(args.setlist)
    if not os.path.isfile(setlist_path):
        print(f"ERROR: Setlist file not found: {setlist_path}", file=sys.stderr)
        return 1

    with open(setlist_path, "r", encoding="utf-8") as fh:
        try:
            setlist = json.load(fh)
        except json.JSONDecodeError as exc:
            print(f"ERROR: Invalid JSON in setlist: {exc}", file=sys.stderr)
            return 1

    set_name   = setlist.get("set_name", "Unnamed Set")
    packs_data = setlist.get("packs", [])

    # Resolve device: CLI flag > JSON field > default
    device_key = args.device or setlist.get("device")
    if device_key not in DEVICE_PROFILES:
        if device_key:
            print(
                f"WARNING: Unknown device '{device_key}'. Defaulting to MPC_Live_III.",
                file=sys.stderr,
            )
        device_key = "MPC_Live_III"

    ceiling_mb  = DEVICE_PROFILES[device_key]["ram_mb"]
    setlist_dir = os.path.dirname(setlist_path)

    # Parse each pack
    packs: List[PackInfo] = []
    for entry in packs_data:
        name     = entry.get("name", "Unknown")
        rel_path = entry.get("path", "")
        abs_path = resolve_path(setlist_dir, rel_path)
        print(f"Parsing: {name}  ({abs_path})")
        packs.append(parse_xpn_zip(name, abs_path))

    if not packs:
        print("WARNING: No packs found in setlist.", file=sys.stderr)

    # Build all four reports
    validation_txt = build_validation_report(packs, device_key, ceiling_mb, set_name)
    loading_txt    = build_loading_order_report(packs, device_key, set_name)
    swap_txt       = build_swap_guide(packs, device_key, ceiling_mb, set_name)
    summary_txt    = build_summary(packs, device_key, ceiling_mb, set_name)

    if args.dry_run:
        for report in (summary_txt, validation_txt, loading_txt, swap_txt):
            print()
            print(report)
        return 0

    out_dir = os.path.abspath(args.output)
    os.makedirs(out_dir, exist_ok=True)

    files = {
        "setlist_validation.txt": validation_txt,
        "loading_order.txt":      loading_txt,
        "swap_guide.txt":         swap_txt,
        "setlist_summary.txt":    summary_txt,
    }

    for filename, content in files.items():
        out_path = os.path.join(out_dir, filename)
        with open(out_path, "w", encoding="utf-8") as fh:
            fh.write(content)
            fh.write("\n")
        print(f"Written: {out_path}")

    print()
    print(summary_txt)
    return 0


if __name__ == "__main__":
    sys.exit(main())
