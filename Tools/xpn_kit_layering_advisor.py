#!/usr/bin/env python3
"""
xpn_kit_layering_advisor.py — XO_OX XOmnibus Tool

Analyzes a set of WAV files (presumed to be velocity variants of the same hit)
and advises on optimal MPC keygroup velocity layering strategies.

Vibe's canonical 4-layer curve:
  Layer 1: 1–40   (pp — gentle ghost touches)
  Layer 2: 41–90  (mp/mf — musical centre of gravity)
  Layer 3: 91–110 (f — forward presence)
  Layer 4: 111–127 (ff — full commitment)

Usage:
  python xpn_kit_layering_advisor.py kick_soft.wav kick_med.wav kick_hard.wav
  python xpn_kit_layering_advisor.py *.wav --output advice.json
"""

import argparse
import json
import math
import struct
from pathlib import Path


# ---------------------------------------------------------------------------
# WAV parsing
# ---------------------------------------------------------------------------

WAV_HEADER_FMT = "<4sI4s4sIHHIIHH"
WAV_HEADER_SIZE = struct.calcsize(WAV_HEADER_FMT)

ANALYSIS_FRAME = 4096  # samples to read for brightness proxy


def parse_wav_header(path: Path) -> dict:
    """
    Parse RIFF/WAV header and return a dict with:
      riff_id, file_size, wave_id, fmt_id, fmt_size,
      audio_format, num_channels, sample_rate, byte_rate,
      block_align, bit_depth
    Raises ValueError if the file is not a valid PCM WAV.
    """
    with open(path, "rb") as f:
        raw = f.read(WAV_HEADER_SIZE)

    if len(raw) < WAV_HEADER_SIZE:
        raise ValueError(f"{path.name}: file too small to be a WAV")

    fields = struct.unpack(WAV_HEADER_FMT, raw)
    header = {
        "riff_id": fields[0],
        "file_size": fields[1],
        "wave_id": fields[2],
        "fmt_id": fields[3],
        "fmt_size": fields[4],
        "audio_format": fields[5],
        "num_channels": fields[6],
        "sample_rate": fields[7],
        "byte_rate": fields[8],
        "block_align": fields[9],
        "bit_depth": fields[10],
    }

    if header["riff_id"] != b"RIFF" or header["wave_id"] != b"WAVE":
        raise ValueError(f"{path.name}: not a RIFF/WAVE file")
    if header["fmt_id"] != b"fmt ":
        raise ValueError(f"{path.name}: missing fmt chunk")
    if header["audio_format"] not in (1, 3):
        raise ValueError(
            f"{path.name}: unsupported audio format {header['audio_format']} "
            "(only PCM=1 or IEEE float=3 supported)"
        )

    return header


def find_data_chunk(path: Path) -> tuple[int, int]:
    """
    Scan the WAV file for the 'data' chunk and return (offset, size).
    Offset points to the first sample byte.
    """
    with open(path, "rb") as f:
        f.seek(12)  # skip RIFF header
        while True:
            chunk_header = f.read(8)
            if len(chunk_header) < 8:
                raise ValueError(f"{path.name}: no data chunk found")
            chunk_id, chunk_size = struct.unpack("<4sI", chunk_header)
            if chunk_id == b"data":
                return f.tell(), chunk_size
            # skip this chunk
            f.seek(chunk_size, 1)


def read_samples_16bit(path: Path, offset: int, num_samples: int) -> list[int]:
    """
    Read up to num_samples 16-bit signed PCM samples starting at byte offset.
    Returns a flat list of integer sample values (all channels interleaved).
    """
    with open(path, "rb") as f:
        f.seek(offset)
        raw = f.read(num_samples * 2)

    count = len(raw) // 2
    if count == 0:
        return []
    return list(struct.unpack(f"<{count}h", raw[:count * 2]))


# ---------------------------------------------------------------------------
# Brightness analysis
# ---------------------------------------------------------------------------

def compute_rms(samples: list[int]) -> float:
    """Root-mean-square of 16-bit samples, normalised to 0.0–1.0."""
    if not samples:
        return 0.0
    mean_sq = sum(s * s for s in samples) / len(samples)
    return math.sqrt(mean_sq) / 32768.0


def compute_zero_crossing_rate(samples: list[int]) -> float:
    """
    Zero-crossing rate: number of sign changes / (N-1).
    Higher ZCR → more high-frequency content (brighter).
    Returns a value in [0.0, 1.0].
    """
    if len(samples) < 2:
        return 0.0
    crossings = sum(
        1 for i in range(1, len(samples)) if (samples[i - 1] >= 0) != (samples[i] >= 0)
    )
    return crossings / (len(samples) - 1)


def brightness_score(rms: float, zcr: float) -> float:
    """
    Combine RMS and ZCR into a single brightness proxy in [0.0, 1.0].
    Weighting: 40% RMS (loudness correlates with velocity energy),
               60% ZCR (spectral brightness proxy).
    """
    return 0.40 * rms + 0.60 * zcr


def analyze_wav(path: Path) -> dict:
    """
    Full analysis of a WAV file. Returns a dict with header info,
    duration, RMS, ZCR, brightness score.
    """
    header = parse_wav_header(path)
    data_offset, data_size = find_data_chunk(path)

    bytes_per_sample = header["bit_depth"] // 8
    total_samples_all_ch = data_size // bytes_per_sample
    total_frames = total_samples_all_ch // max(header["num_channels"], 1)
    duration = total_frames / header["sample_rate"] if header["sample_rate"] > 0 else 0.0

    # Read analysis frame (interleaved); use only channel 0 for analysis
    n_read = min(ANALYSIS_FRAME * header["num_channels"], total_samples_all_ch)

    if header["bit_depth"] == 16:
        raw_samples = read_samples_16bit(path, data_offset, n_read)
        # Demix to mono (channel 0)
        nc = header["num_channels"]
        mono = raw_samples[::nc] if nc > 1 else raw_samples
    else:
        # For 24-bit or 32-bit float, fall back to zero (brightness will be 0)
        # — still report metadata
        mono = []

    rms = compute_rms(mono)
    zcr = compute_zero_crossing_rate(mono)
    score = brightness_score(rms, zcr)

    return {
        "file": path.name,
        "path": str(path.resolve()),
        "sample_rate": header["sample_rate"],
        "bit_depth": header["bit_depth"],
        "num_channels": header["num_channels"],
        "total_frames": total_frames,
        "duration_sec": round(duration, 4),
        "analysis_samples": len(mono),
        "rms": round(rms, 6),
        "zero_crossing_rate": round(zcr, 6),
        "brightness_score": round(score, 6),
    }


# ---------------------------------------------------------------------------
# Layer assignment logic
# ---------------------------------------------------------------------------

# Vibe's canonical 4-layer velocity zones
VIBE_4_LAYER = [
    {"layer": 1, "label": "pp",    "vel_low": 1,   "vel_high": 40},
    {"layer": 2, "label": "mp/mf", "vel_low": 41,  "vel_high": 90},
    {"layer": 3, "label": "f",     "vel_low": 91,  "vel_high": 110},
    {"layer": 4, "label": "ff",    "vel_low": 111, "vel_high": 127},
]

SPLIT_2_LAYER = [
    {"layer": 1, "label": "soft",  "vel_low": 1,  "vel_high": 90},
    {"layer": 2, "label": "hard",  "vel_low": 91, "vel_high": 127},
]

SPLIT_3_LAYER = [
    {"layer": 1, "label": "soft",   "vel_low": 1,   "vel_high": 60},
    {"layer": 2, "label": "medium", "vel_low": 61,  "vel_high": 100},
    {"layer": 3, "label": "hard",   "vel_low": 101, "vel_high": 127},
]


def assign_layers(analyses: list[dict]) -> dict:
    """
    Sort files by brightness score (softest → brightest) and assign
    velocity layers based on sample count. Returns full advice dict.
    """
    count = len(analyses)

    # Sort by brightness ascending (quietest/softest first)
    sorted_analyses = sorted(analyses, key=lambda a: a["brightness_score"])

    if count == 1:
        zones = [{"layer": 1, "label": "full range", "vel_low": 1, "vel_high": 127}]
        note = (
            "Single sample detected. Assigned to full velocity range. "
            "Consider recording 2–4 velocity variants for dynamic expressiveness. "
            "Vibe's preferred minimum is 2 layers (soft + hard)."
        )
    elif count == 2:
        zones = SPLIT_2_LAYER
        note = (
            "2 samples: split at velocity 90/91. "
            "Softer sample covers pp→mp, harder sample covers f→ff."
        )
    elif count == 3:
        zones = SPLIT_3_LAYER
        note = (
            "3 samples: split at 60/61 and 100/101. "
            "Covers soft, medium and hard zones. "
            "Consider adding a 4th (ff) layer for Vibe's canonical curve."
        )
    else:
        # 4 or more: use top 4 by brightness for canonical curve
        zones = VIBE_4_LAYER
        if count > 4:
            note = (
                f"{count} samples provided; using the 4 most representative "
                "(sorted by brightness). Extras are listed below as unused. "
                "MPC keygroup programs support a maximum of 4 velocity layers."
            )
        else:
            note = (
                "4 samples: Vibe's canonical 4-layer curve applied. "
                "pp (1–40) / mp-mf (41–90) / f (91–110) / ff (111–127)."
            )

    # Assign samples to zones (up to 4)
    assignments = []
    used_files = []
    n_layers = len(zones)
    for i, zone in enumerate(zones):
        sample = sorted_analyses[i] if i < len(sorted_analyses) else None
        assignment = dict(zone)
        if sample:
            assignment["file"] = sample["file"]
            assignment["brightness_score"] = sample["brightness_score"]
            assignment["rms"] = sample["rms"]
            assignment["zero_crossing_rate"] = sample["zero_crossing_rate"]
            assignment["duration_sec"] = sample["duration_sec"]
            used_files.append(sample["file"])
        assignments.append(assignment)

    unused = [a for a in sorted_analyses[n_layers:]]

    # Estimated dynamic range: brightness difference between softest and hardest used sample
    used_scores = [a["brightness_score"] for a in sorted_analyses[:n_layers]]
    dynamic_range_estimate = round(used_scores[-1] - used_scores[0], 6) if len(used_scores) > 1 else 0.0

    # Dynamic range quality assessment
    if dynamic_range_estimate >= 0.15:
        dynamic_quality = "excellent — layers should sound meaningfully distinct"
    elif dynamic_range_estimate >= 0.08:
        dynamic_quality = "good — layers have noticeable timbral contrast"
    elif dynamic_range_estimate >= 0.03:
        dynamic_quality = "moderate — subtle velocity response; consider re-recording with more velocity contrast"
    else:
        dynamic_quality = "low — samples may be too similar; velocity layering benefit is minimal"

    return {
        "summary": {
            "sample_count": count,
            "layers_assigned": n_layers,
            "scheme": f"{n_layers}-layer",
            "note": note,
            "dynamic_range_brightness": dynamic_range_estimate,
            "dynamic_quality": dynamic_quality,
        },
        "layer_assignments": assignments,
        "unused_samples": [
            {
                "file": u["file"],
                "brightness_score": u["brightness_score"],
                "reason": "exceeds MPC 4-layer maximum",
            }
            for u in unused
        ],
        "all_analyses": sorted_analyses,
    }


# ---------------------------------------------------------------------------
# Reporting
# ---------------------------------------------------------------------------

def print_report(advice: dict) -> None:
    summary = advice["summary"]
    print("\n" + "=" * 60)
    print("  XO_OX Kit Layering Advisor — Results")
    print("=" * 60)
    print(f"  Samples analysed : {summary['sample_count']}")
    print(f"  Scheme           : {summary['scheme']}")
    print(f"  Dynamic range    : {summary['dynamic_range_brightness']:.4f}  ({summary['dynamic_quality']})")
    print(f"\n  Note: {summary['note']}")

    print("\n" + "-" * 60)
    print("  Layer Assignments (sorted soft → bright)")
    print("-" * 60)
    for a in advice["layer_assignments"]:
        file_label = a.get("file", "[no sample]")
        bright = a.get("brightness_score", "—")
        rms    = a.get("rms", "—")
        zcr    = a.get("zero_crossing_rate", "—")
        dur    = a.get("duration_sec", "—")
        print(
            f"  Layer {a['layer']} [{a['label']:6s}]  vel {a['vel_low']:3d}–{a['vel_high']:3d}"
            f"  →  {file_label}"
        )
        if isinstance(bright, float):
            print(
                f"           brightness={bright:.4f}  rms={rms:.4f}"
                f"  zcr={zcr:.4f}  dur={dur}s"
            )

    if advice["unused_samples"]:
        print("\n" + "-" * 60)
        print("  Unused Samples")
        print("-" * 60)
        for u in advice["unused_samples"]:
            print(f"  {u['file']}  (brightness={u['brightness_score']:.4f})  — {u['reason']}")

    print("\n" + "=" * 60 + "\n")


# ---------------------------------------------------------------------------
# CLI entry point
# ---------------------------------------------------------------------------

def main() -> None:
    parser = argparse.ArgumentParser(
        description=(
            "Analyse WAV files and advise on MPC keygroup velocity layering. "
            "Files should be velocity variants of the same instrument hit."
        )
    )
    parser.add_argument(
        "wav_files",
        nargs="+",
        metavar="WAV",
        help="One or more WAV files to analyse.",
    )
    parser.add_argument(
        "--output",
        "-o",
        metavar="FILE",
        default=None,
        help="Write advice as JSON to this path (e.g. advice.json).",
    )
    args = parser.parse_args()

    paths = [Path(p) for p in args.wav_files]

    # Validate existence
    errors = []
    for p in paths:
        if not p.exists():
            errors.append(f"File not found: {p}")
        elif not p.suffix.lower() == ".wav":
            errors.append(f"Not a .wav file: {p}")
    if errors:
        for e in errors:
            print(f"ERROR: {e}")
        raise SystemExit(1)

    # Analyse each file
    analyses = []
    for p in paths:
        try:
            result = analyze_wav(p)
            analyses.append(result)
        except ValueError as exc:
            print(f"WARNING: Skipping {p.name} — {exc}")
        except Exception as exc:
            print(f"WARNING: Unexpected error analysing {p.name} — {exc}")

    if not analyses:
        print("ERROR: No valid WAV files could be analysed.")
        raise SystemExit(1)

    # Generate advice
    advice = assign_layers(analyses)

    # Print human-readable report
    print_report(advice)

    # Optionally write JSON
    if args.output:
        out_path = Path(args.output)
        with open(out_path, "w", encoding="utf-8") as f:
            json.dump(advice, f, indent=2)
        print(f"  Advice written to: {out_path.resolve()}\n")


if __name__ == "__main__":
    main()
