#!/usr/bin/env python3
"""
xpn_sample_loop_optimizer.py — XO_OX XOceanus Tool
Analyzes WAV files for loop optimization.
Finds good loop points for sustained instrument samples and validates existing loop markers.

Usage:
    python xpn_sample_loop_optimizer.py <wav_file> [--output report.txt] [--top 3]
"""

import argparse
import math
import struct
from pathlib import Path


# ---------------------------------------------------------------------------
# RIFF / WAV parsing
# ---------------------------------------------------------------------------

def read_riff_chunks(data: bytes) -> dict:
    """Parse RIFF file and return dict of chunk_id -> (offset, size, data)."""
    if len(data) < 12:
        raise ValueError("File too small to be a valid RIFF WAV.")
    riff_id, riff_size, wave_id = struct.unpack_from("<4sI4s", data, 0)
    if riff_id != b"RIFF" or wave_id != b"WAVE":
        raise ValueError("Not a valid RIFF WAVE file.")

    chunks = {}
    offset = 12
    while offset + 8 <= len(data):
        chunk_id, chunk_size = struct.unpack_from("<4sI", data, offset)
        chunk_start = offset + 8
        chunk_end = chunk_start + chunk_size
        chunk_data = data[chunk_start:chunk_end]
        key = chunk_id.decode("ascii", errors="replace").strip()
        chunks[key] = (chunk_start, chunk_size, chunk_data)
        # RIFF chunks are word-aligned
        offset = chunk_end + (chunk_size & 1)
    return chunks


def parse_fmt_chunk(fmt_data: bytes) -> dict:
    """Parse the fmt chunk and return audio format info."""
    if len(fmt_data) < 16:
        raise ValueError("fmt chunk too small.")
    audio_format, num_channels, sample_rate, byte_rate, block_align, bits_per_sample = \
        struct.unpack_from("<HHIIHH", fmt_data, 0)
    return {
        "audio_format": audio_format,
        "num_channels": num_channels,
        "sample_rate": sample_rate,
        "byte_rate": byte_rate,
        "block_align": block_align,
        "bits_per_sample": bits_per_sample,
    }


def decode_samples(data: bytes, fmt: dict) -> list:
    """Decode raw PCM bytes to a list of float samples [-1.0, 1.0] (mono mix)."""
    bps = fmt["bits_per_sample"]
    channels = fmt["num_channels"]
    block = fmt["block_align"]

    if bps not in (8, 16, 24, 32):
        raise ValueError(f"Unsupported bit depth: {bps}")

    num_frames = len(data) // block
    samples = []

    for i in range(num_frames):
        frame_offset = i * block
        channel_vals = []
        for ch in range(channels):
            sample_offset = frame_offset + ch * (bps // 8)
            if bps == 8:
                raw = struct.unpack_from("<B", data, sample_offset)[0]
                val = (raw - 128) / 128.0
            elif bps == 16:
                raw = struct.unpack_from("<h", data, sample_offset)[0]
                val = raw / 32768.0
            elif bps == 24:
                b0, b1, b2 = struct.unpack_from("BBB", data, sample_offset)
                raw = (b2 << 16) | (b1 << 8) | b0
                if raw >= 0x800000:
                    raw -= 0x1000000
                val = raw / 8388608.0
            elif bps == 32:
                raw = struct.unpack_from("<i", data, sample_offset)[0]
                val = raw / 2147483648.0
            channel_vals.append(val)
        samples.append(sum(channel_vals) / len(channel_vals))

    return samples


def parse_smpl_chunk(smpl_data: bytes) -> dict:
    """Parse smpl chunk and return loop info."""
    if len(smpl_data) < 36:
        return {"loops": []}
    (manufacturer, product, sample_period, midi_unity_note, midi_pitch_fraction,
     smpte_format, smpte_offset, num_sample_loops, sampler_data) = \
        struct.unpack_from("<IIIIIIIII", smpl_data, 0)

    loops = []
    for i in range(num_sample_loops):
        loop_offset = 36 + i * 24
        if loop_offset + 24 > len(smpl_data):
            break
        cue_point_id, loop_type, loop_start, loop_end, fraction, play_count = \
            struct.unpack_from("<IIIIII", smpl_data, loop_offset)
        loops.append({
            "id": cue_point_id,
            "type": loop_type,
            "start": loop_start,
            "end": loop_end,
        })
    return {
        "midi_unity_note": midi_unity_note,
        "num_sample_loops": num_sample_loops,
        "loops": loops,
    }


# ---------------------------------------------------------------------------
# Analysis helpers
# ---------------------------------------------------------------------------

def rms(samples: list, start: int, length: int) -> float:
    end = min(start + length, len(samples))
    window = samples[start:end]
    if not window:
        return 0.0
    return math.sqrt(sum(s * s for s in window) / len(window))


def is_zero_crossing(samples: list, pos: int, threshold: float = 0.02) -> bool:
    if pos <= 0 or pos >= len(samples):
        return False
    return abs(samples[pos]) <= threshold


def find_zero_crossings(samples: list, threshold: float = 0.02) -> list:
    crossings = []
    for i in range(1, len(samples)):
        if samples[i - 1] * samples[i] <= 0 or abs(samples[i]) <= threshold:
            crossings.append(i)
    return crossings


def windowed_rms_stability(samples: list, start: int, length: int,
                            window_size: int = 1024) -> float:
    """Return coefficient of variation of RMS across windows. Lower = more stable."""
    rms_values = []
    pos = start
    while pos + window_size <= start + length and pos + window_size <= len(samples):
        rms_values.append(rms(samples, pos, window_size))
        pos += window_size
    if len(rms_values) < 2:
        return 0.0
    mean_rms = sum(rms_values) / len(rms_values)
    if mean_rms == 0.0:
        return 0.0
    variance = sum((v - mean_rms) ** 2 for v in rms_values) / len(rms_values)
    return math.sqrt(variance) / mean_rms


def discontinuity_score(samples: list, loop_start: int, loop_end: int,
                         window: int = 256) -> float:
    """Compare last `window` samples before loop_end to first `window` after loop_start."""
    pre_end = samples[max(0, loop_end - window):loop_end]
    post_start = samples[loop_start:loop_start + window]
    min_len = min(len(pre_end), len(post_start))
    if min_len == 0:
        return 1.0
    diff = sum((pre_end[i] - post_start[i]) ** 2 for i in range(min_len))
    return math.sqrt(diff / min_len)


def detect_sample_type(samples: list) -> str:
    """Classify as 'sustained' or 'transient' based on RMS stability after attack."""
    total = len(samples)
    if total < 2048:
        return "transient (too short)"
    attack_end = int(total * 0.20)
    sustain_rms = rms(samples, attack_end, total - attack_end)
    attack_rms = rms(samples, 0, attack_end)
    if attack_rms == 0:
        return "sustained"
    ratio = sustain_rms / attack_rms
    cv = windowed_rms_stability(samples, attack_end, total - attack_end)
    if ratio > 0.3 and cv < 0.30:
        return "sustained (loopable)"
    return "transient (not ideal for looping)"


# ---------------------------------------------------------------------------
# Validation and suggestion
# ---------------------------------------------------------------------------

def validate_existing_loops(samples: list, loops: list) -> list:
    results = []
    for loop in loops:
        start = loop["start"]
        end = loop["end"]
        issues = []
        notes = []

        # 1. Zero crossing at loop start
        if not is_zero_crossing(samples, start):
            issues.append(f"Loop start ({start}) is not at a zero-crossing "
                          f"(value={samples[start]:.4f}).")
        else:
            notes.append("Loop start is at a zero-crossing.")

        # 2. Matching amplitude
        start_rms = rms(samples, start, 256)
        end_rms = rms(samples, max(0, end - 256), 256)
        if start_rms > 0:
            amp_diff = abs(start_rms - end_rms) / start_rms
            if amp_diff > 0.05:
                issues.append(f"Amplitude mismatch: start RMS={start_rms:.4f}, "
                               f"end RMS={end_rms:.4f} (diff={amp_diff*100:.1f}%).")
            else:
                notes.append(f"Amplitude match OK (diff={amp_diff*100:.1f}%).")

        # 3. Loop region length
        loop_length = end - start
        if loop_length < 4096:
            issues.append(f"Loop region is only {loop_length} samples (min 4096 recommended).")
        else:
            notes.append(f"Loop length {loop_length} samples — OK.")

        # 4. Discontinuity check
        disc = discontinuity_score(samples, start, end)
        if disc > 0.05:
            issues.append(f"Discontinuity detected at loop junction (score={disc:.4f}).")
        else:
            notes.append(f"Loop junction is smooth (discontinuity score={disc:.4f}).")

        results.append({
            "loop_id": loop["id"],
            "start": start,
            "end": end,
            "issues": issues,
            "notes": notes,
            "valid": len(issues) == 0,
        })
    return results


def suggest_loop_points(samples: list, top_n: int = 3) -> list:
    """Find candidate loop start points with quality scores."""
    total = len(samples)
    if total < 8192:
        return []

    attack_end = int(total * 0.20)
    crossings = [zc for zc in find_zero_crossings(samples) if zc >= attack_end]

    candidates = []
    for zc in crossings:
        # Need at least 4096 samples for a loop region
        if zc + 4096 > total:
            break

        # Stability in a 4096-sample window starting here
        cv = windowed_rms_stability(samples, zc, 4096)
        if cv > 0.10:
            continue  # Not stable enough

        # Estimate a loop end ~4096-8192 samples ahead, also at zero crossing
        candidate_ends = [c for c in crossings if 4096 <= (c - zc) <= 32768]
        if not candidate_ends:
            continue
        loop_end = candidate_ends[len(candidate_ends) // 2]

        disc = discontinuity_score(samples, zc, loop_end)
        amp_match = abs(rms(samples, zc, 256) - rms(samples, max(0, loop_end - 256), 256))

        # Score: lower is better → invert for display
        raw_score = cv + disc + amp_match
        quality = max(0.0, 1.0 - raw_score * 10)

        candidates.append({
            "loop_start": zc,
            "suggested_end": loop_end,
            "loop_length": loop_end - zc,
            "stability_cv": cv,
            "discontinuity": disc,
            "quality_score": quality,
        })

        if len(candidates) >= top_n * 5:
            break

    # Sort by quality descending, return top_n
    candidates.sort(key=lambda c: c["quality_score"], reverse=True)
    return candidates[:top_n]


# ---------------------------------------------------------------------------
# Report formatting
# ---------------------------------------------------------------------------

def build_report(wav_path: Path, fmt: dict, samples: list,
                 smpl_info: dict | None, top_n: int) -> str:
    lines = []
    lines.append("=" * 60)
    lines.append("XPN Sample Loop Optimizer — XO_OX XOceanus")
    lines.append("=" * 60)
    lines.append(f"File       : {wav_path.name}")
    lines.append(f"Sample rate: {fmt['sample_rate']} Hz")
    lines.append(f"Channels   : {fmt['num_channels']}")
    lines.append(f"Bit depth  : {fmt['bits_per_sample']}-bit")
    lines.append(f"Frames     : {len(samples)}")
    duration = len(samples) / fmt["sample_rate"]
    lines.append(f"Duration   : {duration:.3f}s")
    lines.append("")

    sample_type = detect_sample_type(samples)
    lines.append(f"Sample type: {sample_type}")
    lines.append("")

    if smpl_info and smpl_info["loops"]:
        lines.append(f"Existing smpl chunk: {smpl_info['num_sample_loops']} loop(s) found.")
        lines.append(f"MIDI unity note    : {smpl_info['midi_unity_note']}")
        lines.append("")
        validation = validate_existing_loops(samples, smpl_info["loops"])
        for v in validation:
            status = "PASS" if v["valid"] else "FAIL"
            lines.append(f"  Loop {v['loop_id']}  [{status}]  "
                         f"start={v['start']}  end={v['end']}")
            for note in v["notes"]:
                lines.append(f"    OK  {note}")
            for issue in v["issues"]:
                lines.append(f"    !!  {issue}")
            lines.append("")
    else:
        lines.append("No smpl chunk found — suggesting loop points.")
        lines.append("")
        candidates = suggest_loop_points(samples, top_n)
        if not candidates:
            lines.append("  No suitable loop candidates found.")
            lines.append("  (Sample may be too short or too transient.)")
        else:
            lines.append(f"  Top {len(candidates)} candidate loop point(s):")
            lines.append("")
            for rank, c in enumerate(candidates, 1):
                lines.append(f"  Candidate #{rank}")
                lines.append(f"    Loop start      : {c['loop_start']}")
                lines.append(f"    Suggested end   : {c['suggested_end']}")
                lines.append(f"    Loop length     : {c['loop_length']} samples")
                lines.append(f"    Stability (CV)  : {c['stability_cv']:.4f}  "
                              f"(lower=better, threshold=0.10)")
                lines.append(f"    Discontinuity   : {c['discontinuity']:.4f}  "
                              f"(lower=better, threshold=0.05)")
                lines.append(f"    Quality score   : {c['quality_score']:.3f}  "
                              f"(higher=better, max=1.0)")
                lines.append("")

    lines.append("=" * 60)
    return "\n".join(lines)


# ---------------------------------------------------------------------------
# CLI entry point
# ---------------------------------------------------------------------------

def main():
    parser = argparse.ArgumentParser(
        description="Analyze WAV files for loop optimization (XO_OX XOceanus)."
    )
    parser.add_argument("wav_file", type=Path, help="Path to .wav file.")
    parser.add_argument("--output", type=Path, default=None,
                        help="Write report to this file instead of stdout.")
    parser.add_argument("--top", type=int, default=3,
                        help="Number of candidate loop points to suggest (default: 3).")
    args = parser.parse_args()

    wav_path: Path = args.wav_file.resolve()
    if not wav_path.exists():
        print(f"Error: file not found: {wav_path}")
        raise SystemExit(1)

    raw = wav_path.read_bytes()

    try:
        chunks = read_riff_chunks(raw)
    except ValueError as e:
        print(f"Error parsing RIFF: {e}")
        raise SystemExit(1)

    if "fmt " not in chunks:
        print("Error: no fmt chunk found.")
        raise SystemExit(1)

    fmt = parse_fmt_chunk(chunks["fmt "][2])

    if fmt["audio_format"] not in (1, 3):
        print(f"Warning: audio_format={fmt['audio_format']} — only PCM (1) fully supported.")

    if "data" not in chunks:
        print("Error: no data chunk found.")
        raise SystemExit(1)

    samples = decode_samples(chunks["data"][2], fmt)

    smpl_info = None
    if "smpl" in chunks:
        smpl_info = parse_smpl_chunk(chunks["smpl"][2])

    report = build_report(wav_path, fmt, samples, smpl_info, args.top)

    if args.output:
        args.output.write_text(report)
        print(f"Report written to {args.output}")
    else:
        print(report)


if __name__ == "__main__":
    main()
