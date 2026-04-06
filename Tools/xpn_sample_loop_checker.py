#!/usr/bin/env python3
"""
xpn_sample_loop_checker.py — Check WAV loop point quality in a .xpn pack.

Usage:
    python xpn_sample_loop_checker.py <pack.xpn> [--format text|json] [--strict]

Checks per looped layer:
    1. Loop point existence in WAV SMPL chunk
    2. Loop range validity (LoopEnd > LoopStart, both within sample length)
    3. Boundary continuity (amplitude delta < 0.05 at loop junction)
    4. Zero-crossing proximity (within ±20 samples at start and end)
    5. Minimum loop length (> 1000 samples)

Exit codes (default):  0 = all clean
With --strict:         1 = WARNING present, 2 = POTENTIAL CLICK / ERROR present
"""

import argparse
import json
import os
import struct
import sys
import tempfile
import zipfile
import xml.etree.ElementTree as ET
from pathlib import Path
from typing import Optional


# ---------------------------------------------------------------------------
# Severity constants
# ---------------------------------------------------------------------------
CLEAN = "CLEAN"
WARNING = "WARNING"
ERROR = "POTENTIAL CLICK"

MIN_LOOP_SAMPLES = 1000
ZERO_CROSS_WINDOW = 20
BOUNDARY_WINDOW = 512
BOUNDARY_THRESHOLD = 0.05


# ---------------------------------------------------------------------------
# WAV helpers
# ---------------------------------------------------------------------------

def read_wav_header(path: str) -> Optional[dict]:
    """Return basic WAV metadata without using the wave module (handles edge cases)."""
    try:
        with open(path, "rb") as f:
            data = f.read(44)
        if len(data) < 44:
            return None
        riff, size, wave_id = struct.unpack_from("<4sI4s", data, 0)
        if riff != b"RIFF" or wave_id != b"WAVE":
            return None
        fmt_id, fmt_size = struct.unpack_from("<4sI", data, 12)
        if fmt_id != b"fmt ":
            return None
        audio_fmt, channels, sample_rate, byte_rate, block_align, bits = \
            struct.unpack_from("<HHIIHH", data, 20)
        return {
            "audio_fmt": audio_fmt,   # 1=PCM, 3=float
            "channels": channels,
            "sample_rate": sample_rate,
            "bits": bits,
            "sampwidth": bits // 8,
        }
    except Exception:
        return None


def read_smpl_loop_points(path: str) -> list:
    """Parse SMPL chunk from WAV file. Returns list of {start, end} dicts."""
    loops = []
    try:
        with open(path, "rb") as f:
            raw = f.read()
        pos = 12  # skip RIFF/WAVE header
        while pos + 8 <= len(raw):
            chunk_id = raw[pos:pos + 4]
            chunk_size = struct.unpack_from("<I", raw, pos + 4)[0]
            if chunk_id == b"smpl":
                # smpl chunk: 9 DWORD fields before loop data
                # offset 36 within chunk = sample loops count
                if chunk_size < 36:
                    break
                num_loops = struct.unpack_from("<I", raw, pos + 8 + 28)[0]
                loop_offset = pos + 8 + 36
                for i in range(num_loops):
                    if loop_offset + 24 > len(raw):
                        break
                    # Each loop: identifier(4) type(4) start(4) end(4) fraction(4) play_count(4)
                    _, loop_type, start, end = struct.unpack_from("<IIII", raw, loop_offset)
                    loops.append({"start": start, "end": end, "type": loop_type})
                    loop_offset += 24
                break
            pos += 8 + chunk_size
            if chunk_size % 2 != 0:
                pos += 1  # word-align
    except Exception as exc:
        print(f"[WARN] Parsing SMPL loop chunk from {path}: {exc}", file=sys.stderr)
    return loops


def read_frames_float(path: str, start_frame: int, count: int, header: dict) -> list:
    """Read `count` frames from `start_frame`, return as float [-1, 1] (channel 0 only)."""
    sampwidth = header["sampwidth"]
    channels = header["channels"]
    frame_size = sampwidth * channels
    byte_offset = 0

    # Find the data chunk
    try:
        with open(path, "rb") as f:
            raw_header = f.read(12)
            if raw_header[:4] != b"RIFF" or raw_header[8:12] != b"WAVE":
                return []
            pos = 12
            f.seek(pos)
            while True:
                chunk_head = f.read(8)
                if len(chunk_head) < 8:
                    return []
                cid = chunk_head[:4]
                csize = struct.unpack_from("<I", chunk_head, 4)[0]
                if cid == b"data":
                    byte_offset = f.tell()
                    break
                f.seek(csize, 1)
                if csize % 2 != 0:
                    f.seek(1, 1)

            f.seek(byte_offset + start_frame * frame_size)
            raw = f.read(count * frame_size)
    except Exception:
        return []

    samples = []
    actual_frames = len(raw) // frame_size
    audio_fmt = header.get("audio_fmt", 1)

    for i in range(actual_frames):
        frame_bytes = raw[i * frame_size: i * frame_size + frame_size]
        ch0 = frame_bytes[:sampwidth]
        if sampwidth == 2:
            val = struct.unpack("<h", ch0)[0] / 32768.0
        elif sampwidth == 3:
            val = struct.unpack("<i", ch0 + (b'\xff' if ch0[2] & 0x80 else b'\x00'))[0] / 8388608.0
        elif sampwidth == 4:
            if audio_fmt == 3:
                val = struct.unpack("<f", ch0)[0]
            else:
                val = struct.unpack("<i", ch0)[0] / 2147483648.0
        else:
            val = (ch0[0] - 128) / 128.0
        samples.append(val)

    return samples


def find_zero_crossing(path: str, frame: int, window: int, header: dict) -> Optional[int]:
    """Return sample offset (relative to frame) of nearest zero crossing within ±window."""
    start = max(0, frame - window)
    count = window * 2 + 1
    samples = read_frames_float(path, start, count, header)
    if not samples:
        return None

    center = frame - start  # index of the target frame in samples list
    best_offset = None
    best_dist = window + 1

    for i in range(len(samples) - 1):
        # Zero crossing between i and i+1
        if samples[i] * samples[i + 1] <= 0:
            offset = i - center
            dist = abs(offset)
            if dist < best_dist:
                best_dist = dist
                best_offset = offset

    return best_offset  # None if no crossing found in window


# ---------------------------------------------------------------------------
# XPM / XPN parsing
# ---------------------------------------------------------------------------

def parse_loop_from_element(elem: ET.Element) -> Optional[tuple]:
    """Extract (LoopStart, LoopEnd) from an XML element that may carry them."""
    ls = elem.get("LoopStart") or elem.get("loopStart")
    le = elem.get("LoopEnd") or elem.get("loopEnd")
    if ls is not None and le is not None:
        try:
            return int(ls), int(le)
        except ValueError:
            return None
    return None


def find_sample_path(name: str, extracted_dir: str) -> Optional[str]:
    """Locate a WAV file in the extracted pack directory."""
    # Try direct path first
    direct = os.path.join(extracted_dir, name)
    if os.path.exists(direct):
        return direct
    # Search recursively
    basename = os.path.basename(name)
    for root, _, files in os.walk(extracted_dir):
        if basename in files:
            return os.path.join(root, basename)
    return None


def extract_loops_from_xpm(xpm_path: str, extracted_dir: str) -> list:
    """
    Parse an XPM file and return a list of loop entries:
        {program, keygroup, layer, loop_start, loop_end, wav_path}
    """
    entries = []
    try:
        tree = ET.parse(xpm_path)
        root = tree.getroot()
    except ET.ParseError:
        return entries

    program_name = root.get("Name", Path(xpm_path).stem)

    for kg_idx, kg in enumerate(root.iter("Keygroup")):
        kg_name = kg.get("Name", f"Keygroup {kg_idx + 1}")
        # Check for loop on the Keygroup element itself
        kg_loop = parse_loop_from_element(kg)

        for layer_idx, layer in enumerate(kg.iter("Layer")):
            layer_name = layer.get("Name", f"Layer {layer_idx + 1}")
            sample_file = layer.get("SampleFile") or layer.get("Sample") or ""

            # Check Loop sub-element
            loop_elem = layer.find("Loop")
            loop_coords: Optional[tuple] = None

            if loop_elem is not None:
                loop_coords = parse_loop_from_element(loop_elem)

            # Fallback: attributes on Layer or SampleData
            if loop_coords is None:
                loop_coords = parse_loop_from_element(layer)

            for sd in layer.iter("SampleData"):
                if loop_coords is None:
                    loop_coords = parse_loop_from_element(sd)
                if not sample_file:
                    sample_file = sd.get("SampleFile") or sd.get("Sample") or ""

            # Also inherit from keygroup if none found at layer level
            if loop_coords is None and kg_loop is not None:
                loop_coords = kg_loop

            if loop_coords is None:
                continue  # no loop defined

            wav_path = find_sample_path(sample_file, extracted_dir) if sample_file else None

            entries.append({
                "program": program_name,
                "keygroup": kg_name,
                "layer": layer_name,
                "loop_start": loop_coords[0],
                "loop_end": loop_coords[1],
                "wav_path": wav_path,
                "sample_file": sample_file,
            })

    return entries


# ---------------------------------------------------------------------------
# Loop analysis
# ---------------------------------------------------------------------------

def check_loop(entry: dict) -> dict:
    """
    Run all loop checks on a single entry.
    Returns entry dict with added "checks" list and "status".
    """
    ls = entry["loop_start"]
    le = entry["loop_end"]
    wav_path = entry["wav_path"]

    checks = []
    status = CLEAN

    def add(severity: str, key: str, message: str):
        nonlocal status
        checks.append({"severity": severity, "key": key, "message": message})
        if severity == ERROR:
            status = ERROR
        elif severity == WARNING and status == CLEAN:
            status = WARNING

    # --- Check: wav file accessible ---
    if not wav_path or not os.path.exists(wav_path):
        add(WARNING, "WAV_MISSING", f"WAV file not found: {entry['sample_file']}")
        return {**entry, "checks": checks, "status": status}

    header = read_wav_header(wav_path)
    if header is None:
        add(WARNING, "WAV_UNREADABLE", "Could not parse WAV header")
        return {**entry, "checks": checks, "status": status}

    n_frames = _get_n_frames(wav_path)

    # --- 1. SMPL chunk presence ---
    smpl_loops = read_smpl_loop_points(wav_path)
    if smpl_loops:
        checks.append({"severity": "INFO", "key": "SMPL_CHUNK",
                        "message": f"WAV has SMPL chunk with {len(smpl_loops)} loop(s)"})
    else:
        checks.append({"severity": "INFO", "key": "SMPL_CHUNK",
                        "message": "No SMPL chunk in WAV (loop points from XPM only)"})

    # --- 2. Loop range validity ---
    if le <= ls:
        add(ERROR, "LOOP_RANGE", f"LoopEnd ({le}) must be > LoopStart ({ls})")
        return {**entry, "checks": checks, "status": status}

    loop_len = le - ls
    loop_ms = round(loop_len / header["sample_rate"] * 1000, 1)
    checks.append({"severity": "INFO", "key": "LOOP_RANGE",
                    "message": f"Loop: {ls} → {le} ({loop_ms}ms, {loop_len} samples)"})

    if n_frames > 0:
        if ls >= n_frames:
            add(ERROR, "LOOP_OUT_OF_BOUNDS", f"LoopStart ({ls}) >= file length ({n_frames})")
            return {**entry, "checks": checks, "status": status}
        if le > n_frames:
            add(ERROR, "LOOP_OUT_OF_BOUNDS", f"LoopEnd ({le}) > file length ({n_frames})")
            return {**entry, "checks": checks, "status": status}

    # --- 3. Minimum loop length ---
    if loop_len <= MIN_LOOP_SAMPLES:
        add(WARNING, "SHORT_LOOP",
            f"Loop length {loop_len} samples ({loop_ms}ms) — very short, likely clicks")

    # --- 4. Zero-crossing at LoopStart ---
    zc_start = find_zero_crossing(wav_path, ls, ZERO_CROSS_WINDOW, header)
    if zc_start is None:
        add(WARNING, "ZC_START", f"No zero crossing within ±{ZERO_CROSS_WINDOW} samples of LoopStart")
    elif zc_start == 0:
        checks.append({"severity": "INFO", "key": "ZC_START",
                        "message": "Start zero-crossing: at loop point (offset 0)"})
    else:
        msg = f"Start zero-crossing: offset {zc_start:+d} samples"
        sev = "INFO" if abs(zc_start) <= 5 else WARNING
        if sev == WARNING:
            msg += " — consider nudging loop start to zero crossing"
        if sev == WARNING:
            add(sev, "ZC_START", msg)
        else:
            checks.append({"severity": sev, "key": "ZC_START", "message": msg})

    # --- 5. Zero-crossing at LoopEnd ---
    zc_end = find_zero_crossing(wav_path, le, ZERO_CROSS_WINDOW, header)
    if zc_end is None:
        add(WARNING, "ZC_END", f"No zero crossing within ±{ZERO_CROSS_WINDOW} samples of LoopEnd")
    elif zc_end == 0:
        checks.append({"severity": "INFO", "key": "ZC_END",
                        "message": "End zero-crossing: at loop point (offset 0)"})
    else:
        msg = f"End zero-crossing: offset {zc_end:+d} samples"
        sev = "INFO" if abs(zc_end) <= 5 else WARNING
        if sev == WARNING:
            msg += " — consider nudging loop end to zero crossing"
        if sev == WARNING:
            add(sev, "ZC_END", msg)
        else:
            checks.append({"severity": sev, "key": "ZC_END", "message": msg})

    # --- 6. Boundary continuity ---
    # Read 512 samples just before LoopEnd and 512 samples just after LoopStart
    pre_end_start = max(0, le - BOUNDARY_WINDOW)
    pre_end = read_frames_float(wav_path, pre_end_start, BOUNDARY_WINDOW, header)

    post_start_start = ls
    post_start = read_frames_float(wav_path, post_start_start, BOUNDARY_WINDOW, header)

    if pre_end and post_start:
        # Compare last sample before loop end vs first sample after loop start
        delta = abs(pre_end[-1] - post_start[0])
        if delta < BOUNDARY_THRESHOLD:
            checks.append({"severity": "INFO", "key": "BOUNDARY",
                            "message": f"Boundary continuity: delta {delta:.4f} (good)"})
        else:
            add(ERROR, "BOUNDARY",
                f"Boundary discontinuity: delta {delta:.4f} > {BOUNDARY_THRESHOLD} — likely click")
    else:
        checks.append({"severity": "INFO", "key": "BOUNDARY",
                        "message": "Could not read boundary samples for continuity check"})

    return {**entry, "checks": checks, "status": status}


def _get_n_frames(wav_path: str) -> int:
    """Return total frame count by scanning the data chunk."""
    try:
        with open(wav_path, "rb") as f:
            if f.read(4) != b"RIFF":
                return 0
            f.seek(12)
            while True:
                head = f.read(8)
                if len(head) < 8:
                    break
                cid = head[:4]
                csize = struct.unpack_from("<I", head, 4)[0]
                if cid == b"data":
                    # need header to get frame size
                    hdr = read_wav_header(wav_path)
                    if hdr:
                        frame_size = hdr["sampwidth"] * hdr["channels"]
                        return csize // frame_size if frame_size else 0
                    return 0
                f.seek(csize, 1)
                if csize % 2 != 0:
                    f.seek(1, 1)
    except Exception as exc:
        print(f"[WARN] Scanning data chunk for frame count in {wav_path}: {exc}", file=sys.stderr)
    return 0


# ---------------------------------------------------------------------------
# Report rendering
# ---------------------------------------------------------------------------

def severity_symbol(sev: str) -> str:
    return {"INFO": "  ", WARNING: "!", ERROR: "X"}.get(sev, " ")


def render_text(results: list, xpn_name: str) -> str:
    lines = []
    lines.append(f"LOOP CHECK — {xpn_name}")
    lines.append("")

    looped = [r for r in results if r.get("loop_start") is not None]
    if not looped:
        lines.append("No loop points found in any XPM program.")
        return "\n".join(lines)

    # Count programs with loops
    programs = sorted(set(r["program"] for r in looped))
    all_programs = sorted(set(r["program"] for r in results)) if results else []
    lines.append(f"Programs with loops: {len(programs)} of {len(all_programs)}")
    lines.append("")

    for result in looped:
        prog = result["program"]
        kg = result["keygroup"]
        layer = result["layer"]
        ls = result["loop_start"]
        le = result["loop_end"]
        status = result.get("status", CLEAN)

        lines.append(f"  {prog} / {kg} / {layer}")

        for check in result.get("checks", []):
            sev = check["severity"]
            sym = "+" if sev == "INFO" else ("!" if sev == WARNING else "X")
            lines.append(f"    [{sym}] {check['message']}")

        lines.append(f"    Status: {status}")
        lines.append("")

    # Summary
    errors = [r for r in looped if r.get("status") == ERROR]
    warnings = [r for r in looped if r.get("status") == WARNING]
    clean = [r for r in looped if r.get("status") == CLEAN]

    lines.append(f"Summary: {len(clean)} clean, {len(warnings)} warnings, {len(errors)} potential clicks")
    return "\n".join(lines)


def compute_exit_code(results: list, strict: bool) -> int:
    if not strict:
        return 0
    has_error = any(r.get("status") == ERROR for r in results)
    has_warning = any(r.get("status") == WARNING for r in results)
    if has_error:
        return 2
    if has_warning:
        return 1
    return 0


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main() -> int:
    parser = argparse.ArgumentParser(
        description="Check WAV loop point quality in a .xpn pack."
    )
    parser.add_argument("xpn", help="Path to .xpn pack file")
    parser.add_argument("--format", choices=["text", "json"], default="text",
                        help="Output format (default: text)")
    parser.add_argument("--strict", action="store_true",
                        help="Exit 1 on warnings, exit 2 on errors/potential clicks")
    args = parser.parse_args()

    xpn_path = args.xpn
    if not os.path.exists(xpn_path):
        print(f"ERROR: File not found: {xpn_path}", file=sys.stderr)
        return 1

    xpn_name = Path(xpn_path).name

    with tempfile.TemporaryDirectory() as tmpdir:
        # Extract pack
        try:
            with zipfile.ZipFile(xpn_path, "r") as zf:
                dest = Path(tmpdir).resolve()
                for member in zf.infolist():
                    target = (dest / member.filename).resolve()
                    if not target.is_relative_to(dest):
                        raise ValueError(
                            f"ZipSlip blocked: {member.filename!r} escapes {dest}"
                        )
                    zf.extract(member, dest)
        except zipfile.BadZipFile:
            print(f"ERROR: Not a valid .xpn (zip) file: {xpn_path}", file=sys.stderr)
            return 1

        # Find all XPM files
        xpm_files = []
        for root, _, files in os.walk(tmpdir):
            for f in files:
                if f.lower().endswith(".xpm"):
                    xpm_files.append(os.path.join(root, f))

        if not xpm_files:
            print("No .xpm files found in pack.", file=sys.stderr)
            return 1

        # Gather all loop entries
        all_entries = []
        for xpm_path in sorted(xpm_files):
            entries = extract_loops_from_xpm(xpm_path, tmpdir)
            all_entries.extend(entries)

        # Run checks
        results = [check_loop(e) for e in all_entries]

        # Also include entries with no loops for program count
        all_programs_with_loops = set(r["program"] for r in results if r.get("loop_start") is not None)
        all_programs = set()
        for xpm_path in sorted(xpm_files):
            try:
                tree = ET.parse(xpm_path)
                root = tree.getroot()
                all_programs.add(root.get("Name", Path(xpm_path).stem))
            except ET.ParseError as exc:
                print(f"[WARN] Parsing XPM file {Path(xpm_path).name} for program count: {exc}", file=sys.stderr)

        if args.format == "json":
            output = {
                "pack": xpn_name,
                "total_programs": len(all_programs),
                "programs_with_loops": len(all_programs_with_loops),
                "loop_entries": results,
            }
            print(json.dumps(output, indent=2))
        else:
            # Patch results to include all_programs count for text renderer
            print(render_text(results, xpn_name))

    return compute_exit_code(results, args.strict)


if __name__ == "__main__":
    sys.exit(main())
