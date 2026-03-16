#!/usr/bin/env python3
"""
xpn_smart_trim.py — Automatic sample trimming and loop point detection tool.

Usage:
    python xpn_smart_trim.py input.wav --output trimmed.wav [--mode one-shot|loop|auto] [--verbose]

Pure stdlib only: wave, struct, math
"""

import wave
import struct
import math
import argparse
import os
import sys


# ---------------------------------------------------------------------------
# WAV I/O helpers
# ---------------------------------------------------------------------------

def read_wav(path):
    """Read a WAV file. Returns (samples_flat, n_channels, sample_rate, sample_width)."""
    with wave.open(path, 'rb') as wf:
        n_channels = wf.getnchannels()
        sample_rate = wf.getframerate()
        sample_width = wf.getsampwidth()  # bytes per sample per channel
        n_frames = wf.getnframes()
        raw = wf.readframes(n_frames)

    bits = sample_width * 8
    if bits == 16:
        fmt = f'<{n_frames * n_channels}h'
        samples = list(struct.unpack(fmt, raw))
        max_val = 32768.0
    elif bits == 24:
        # 24-bit: unpack manually
        samples = []
        for i in range(n_frames * n_channels):
            b = raw[i*3:(i+1)*3]
            val = struct.unpack('<i', b + (b'\xff' if b[2] & 0x80 else b'\x00'))[0]
            samples.append(val)
        max_val = 8388608.0
    elif bits == 32:
        fmt = f'<{n_frames * n_channels}i'
        samples = list(struct.unpack(fmt, raw))
        max_val = 2147483648.0
    else:
        raise ValueError(f"Unsupported bit depth: {bits}")

    # Normalize to float [-1.0, 1.0]
    float_samples = [s / max_val for s in samples]

    # Mix to mono for analysis (keep originals for write-back)
    if n_channels == 1:
        mono = float_samples
    else:
        mono = [
            sum(float_samples[i * n_channels + c] for c in range(n_channels)) / n_channels
            for i in range(n_frames)
        ]

    return mono, float_samples, n_channels, sample_rate, sample_width, n_frames


def write_wav(path, float_samples, n_channels, sample_rate, sample_width,
              start_frame=0, end_frame=None):
    """Write float samples back to WAV at original bit depth."""
    if end_frame is None:
        total_frames = len(float_samples) // n_channels
        end_frame = total_frames

    bits = sample_width * 8
    if bits == 16:
        max_val = 32767
        fmt_char = 'h'
    elif bits == 24:
        max_val = 8388607
        fmt_char = None  # special handling
    elif bits == 32:
        max_val = 2147483647
        fmt_char = 'i'
    else:
        raise ValueError(f"Unsupported bit depth: {bits}")

    slice_start = start_frame * n_channels
    slice_end = end_frame * n_channels
    sliced = float_samples[slice_start:slice_end]
    n_out_frames = end_frame - start_frame

    if bits == 24:
        raw_bytes = bytearray()
        for s in sliced:
            clamped = max(-1.0, min(1.0, s))
            ival = int(clamped * max_val)
            # Pack as 3 bytes little-endian signed
            b = struct.pack('<i', ival)[:3]
            raw_bytes.extend(b)
        raw = bytes(raw_bytes)
    else:
        ints = [int(max(-max_val - 1, min(max_val, s * (max_val + 1)))) for s in sliced]
        raw = struct.pack(f'<{len(ints)}{fmt_char}', *ints)

    with wave.open(path, 'wb') as wf:
        wf.setnchannels(n_channels)
        wf.setframerate(sample_rate)
        wf.setsampwidth(sample_width)
        wf.setnframes(n_out_frames)
        wf.writeframes(raw)


# ---------------------------------------------------------------------------
# DSP utilities
# ---------------------------------------------------------------------------

def rms_db(samples):
    """Compute RMS level in dBFS for a list of float samples."""
    if not samples:
        return -120.0
    mean_sq = sum(s * s for s in samples) / len(samples)
    if mean_sq == 0.0:
        return -120.0
    return 10.0 * math.log10(mean_sq)


def amplitude_to_db(amplitude):
    if amplitude <= 0.0:
        return -120.0
    return 20.0 * math.log10(abs(amplitude))


def db_to_amplitude(db):
    return 10.0 ** (db / 20.0)


# ---------------------------------------------------------------------------
# 1. Silence trimming
# ---------------------------------------------------------------------------

def trim_silence(mono_samples, threshold_db=-60.0, sample_rate=44100, window=256):
    """
    Remove leading and trailing silence below threshold_db.

    Returns (start_index, end_index) in sample frames (not normalized).
    Recommended thresholds: -60 dBFS for percussion, -72 dBFS for pads.
    """
    n = len(mono_samples)
    threshold_amp = db_to_amplitude(threshold_db)

    # Find leading silence
    start = 0
    for i in range(0, n - window, window):
        chunk = mono_samples[i:i + window]
        peak = max(abs(s) for s in chunk)
        if peak >= threshold_amp:
            # Refine: walk back sample-by-sample
            start = max(0, i)
            while start > 0 and abs(mono_samples[start]) < threshold_amp:
                start -= 1
            break

    # Find trailing silence
    end = n
    for i in range(n - window, start, -window):
        chunk = mono_samples[i:i + window]
        peak = max(abs(s) for s in chunk)
        if peak >= threshold_amp:
            end = min(n, i + window)
            while end < n and abs(mono_samples[end - 1]) < threshold_amp:
                end -= 1
            break

    return start, end


# ---------------------------------------------------------------------------
# 2. Transient onset detection
# ---------------------------------------------------------------------------

def find_transient_onset(mono_samples, window=256):
    """
    Find sample index where transient begins (energy rise > 10x baseline).

    Computes a rolling RMS baseline over the first few windows, then scans
    forward until RMS exceeds 10x that baseline. Returns the sample index.
    """
    n = len(mono_samples)
    if n < window * 2:
        return 0

    # Baseline: average of first 4 windows or until signal, whichever is quieter
    baseline_windows = min(4, n // window)
    baseline_energies = []
    for i in range(baseline_windows):
        chunk = mono_samples[i * window:(i + 1) * window]
        energy = sum(s * s for s in chunk) / len(chunk)
        baseline_energies.append(energy)
    baseline = sum(baseline_energies) / len(baseline_energies) if baseline_energies else 1e-12
    baseline = max(baseline, 1e-12)  # avoid divide-by-zero

    threshold_energy = baseline * 10.0

    # Scan for onset
    for i in range(0, n - window, window // 2):
        chunk = mono_samples[i:i + window]
        energy = sum(s * s for s in chunk) / len(chunk)
        if energy >= threshold_energy:
            # Refine: walk back to find precise onset
            onset = i
            while onset > 0 and (mono_samples[onset] ** 2) < threshold_energy * 0.1:
                onset -= 1
            return max(0, onset)

    return 0


# ---------------------------------------------------------------------------
# 3. One-shot vs loop detection
# ---------------------------------------------------------------------------

def detect_one_shot_vs_loop(mono_samples, sample_rate):
    """
    Returns "one-shot" if:
      - RMS decays >40 dB from peak within 2 seconds, OR
      - Duration < 1.5 seconds
    Returns "loop" otherwise.
    """
    duration_s = len(mono_samples) / sample_rate

    if duration_s < 1.5:
        return "one-shot"

    # Find peak RMS in 50ms windows
    window = int(sample_rate * 0.05)
    if window < 1:
        return "one-shot"

    peak_rms_db = -120.0
    peak_window_idx = 0
    for i in range(0, len(mono_samples) - window, window):
        chunk = mono_samples[i:i + window]
        db = rms_db(chunk)
        if db > peak_rms_db:
            peak_rms_db = db
            peak_window_idx = i

    # Check RMS 2 seconds after the peak
    two_sec_samples = int(sample_rate * 2.0)
    check_start = peak_window_idx + two_sec_samples
    if check_start + window <= len(mono_samples):
        check_chunk = mono_samples[check_start:check_start + window]
        check_db = rms_db(check_chunk)
        decay = peak_rms_db - check_db
        if decay > 40.0:
            return "one-shot"

    return "loop"


# ---------------------------------------------------------------------------
# 4. Loop point detection — zero crossing snap
# ---------------------------------------------------------------------------

def find_loop_points_zero_crossing(mono_samples, target_loop_end_samples):
    """
    Snap target_loop_end_samples to nearest zero crossing.
    Loop start is set to 0 (or a small offset to skip attack transient).
    Returns (loop_start, loop_end).
    """
    n = len(mono_samples)
    target = max(0, min(target_loop_end_samples, n - 1))

    # Search ±2000 samples for nearest zero crossing
    search_radius = 2000
    best_idx = target
    best_dist = abs(mono_samples[target])

    lo = max(1, target - search_radius)
    hi = min(n - 1, target + search_radius)

    for i in range(lo, hi):
        # Zero crossing: sign change between adjacent samples
        if mono_samples[i - 1] * mono_samples[i] <= 0:
            dist = abs(i - target)
            amplitude = abs(mono_samples[i])
            score = dist + amplitude * 500  # prefer close + near-zero amplitude
            if score < best_dist + abs(best_idx - target) * 1.0:
                best_idx = i
                best_dist = score

    loop_end = best_idx

    # Loop start: skip first ~10ms to avoid attack click
    loop_start_offset = int(len(mono_samples) * 0.05)  # 5% in
    loop_start = loop_start_offset

    # Also snap loop_start to a zero crossing
    search_lo = max(1, loop_start - 1000)
    search_hi = min(n - 1, loop_start + 1000)
    best_start = loop_start
    best_start_score = float('inf')
    for i in range(search_lo, search_hi):
        if mono_samples[i - 1] * mono_samples[i] <= 0:
            dist = abs(i - loop_start)
            if dist < best_start_score:
                best_start_score = dist
                best_start = i

    return best_start, loop_end


# ---------------------------------------------------------------------------
# 5. Loop point detection — RMS matching
# ---------------------------------------------------------------------------

def find_loop_points_rms_match(mono_samples, sample_rate, window_ms=50):
    """
    Find window near end that best RMS-matches the first window.
    Returns (loop_start, loop_end) where loop_start is the matched window
    near the end, and loop_end is len(mono_samples) - 1 (or near it).
    """
    n = len(mono_samples)
    window = int(sample_rate * window_ms / 1000.0)
    if window < 16 or n < window * 4:
        return 0, n - 1

    # Reference: first window RMS
    ref_chunk = mono_samples[:window]
    ref_rms = rms_db(ref_chunk)

    # Search in last 60% of the sample for best RMS match
    search_start = n // 2
    best_idx = search_start
    best_diff = float('inf')

    step = max(1, window // 4)
    for i in range(search_start, n - window, step):
        chunk = mono_samples[i:i + window]
        chunk_rms = rms_db(chunk)
        diff = abs(chunk_rms - ref_rms)
        if diff < best_diff:
            best_diff = diff
            best_idx = i

    loop_start = best_idx

    # Loop end: snap to zero crossing near end of file
    target_end = n - int(sample_rate * 0.005)  # 5ms before end
    _, loop_end = find_loop_points_zero_crossing(mono_samples, target_end)

    return loop_start, loop_end


# ---------------------------------------------------------------------------
# XPM field recommendation printer
# ---------------------------------------------------------------------------

def print_xpm_recommendations(start_frame, end_frame, loop_start, loop_end,
                               loop_on, mode, verbose=False):
    """Print XPM XML field recommendations."""
    print("\n--- XPM Field Recommendations ---")
    print(f"  Mode detected:      {mode}")
    print(f"  <SampleStart>       {start_frame}</SampleStart>")
    print(f"  <SampleEnd>         {end_frame}</SampleEnd>")
    if loop_on:
        print(f"  <LoopStart>         {loop_start}</LoopStart>")
        print(f"  <LoopEnd>           {loop_end}</LoopEnd>")
        print(f"  <LoopOn>            True</LoopOn>")
    else:
        print(f"  <LoopStart>         0</LoopStart>")
        print(f"  <LoopEnd>           0</LoopEnd>")
        print(f"  <LoopOn>            False</LoopOn>")
    print("---------------------------------\n")

    if verbose and loop_on:
        loop_len = loop_end - loop_start
        print(f"  Loop length (samples): {loop_len}")
        print(f"  Loop length (frames):  {loop_len} frames")


# ---------------------------------------------------------------------------
# Main processing pipeline
# ---------------------------------------------------------------------------

def process(input_path, output_path, mode='auto', verbose=False):
    if verbose:
        print(f"Reading: {input_path}")

    mono, float_samples, n_channels, sample_rate, sample_width, n_frames = read_wav(input_path)

    if verbose:
        duration_s = n_frames / sample_rate
        bits = sample_width * 8
        print(f"  Channels:    {n_channels}")
        print(f"  Sample rate: {sample_rate} Hz")
        print(f"  Bit depth:   {bits}-bit")
        print(f"  Duration:    {duration_s:.3f}s ({n_frames} frames)")
        overall_rms = rms_db(mono)
        peak = max(abs(s) for s in mono) if mono else 0.0
        print(f"  RMS:         {overall_rms:.1f} dBFS")
        print(f"  Peak:        {amplitude_to_db(peak):.1f} dBFS")

    # --- Step 1: Detect mode ---
    if mode == 'auto':
        detected_mode = detect_one_shot_vs_loop(mono, sample_rate)
        if verbose:
            print(f"\nAuto-detected mode: {detected_mode}")
    else:
        detected_mode = mode
        if verbose:
            print(f"\nUsing specified mode: {detected_mode}")

    # --- Step 2: Trim silence ---
    threshold = -60.0 if detected_mode == 'one-shot' else -72.0
    trim_start, trim_end = trim_silence(mono, threshold_db=threshold, sample_rate=sample_rate)

    if verbose:
        trim_start_s = trim_start / sample_rate
        trim_end_s = trim_end / sample_rate
        removed_start_ms = (trim_start / sample_rate) * 1000
        removed_end_ms = ((n_frames - trim_end) / sample_rate) * 1000
        print(f"\nSilence trimming (threshold {threshold:.0f} dBFS):")
        print(f"  Trim start:  sample {trim_start} ({trim_start_s:.4f}s) — removed {removed_start_ms:.1f}ms from head")
        print(f"  Trim end:    sample {trim_end} ({trim_end_s:.4f}s) — removed {removed_end_ms:.1f}ms from tail")

    trimmed_mono = mono[trim_start:trim_end]

    # --- Step 3a: One-shot — align transient ---
    loop_start = 0
    loop_end = 0
    loop_on = False

    if detected_mode == 'one-shot':
        transient_idx = find_transient_onset(trimmed_mono, window=256)
        # Adjust start to transient onset
        final_start = trim_start + transient_idx
        final_end = trim_end

        if verbose:
            onset_ms = transient_idx / sample_rate * 1000
            print(f"\nTransient onset:     sample +{transient_idx} ({onset_ms:.1f}ms into trimmed audio)")
            print(f"Final range:         {final_start} → {final_end}")

    # --- Step 3b: Loop — find loop points ---
    else:
        final_start = trim_start
        final_end = trim_end
        loop_len = len(trimmed_mono)

        # Try RMS-match first, then zero-crossing as fallback
        lp_start_rms, lp_end_rms = find_loop_points_rms_match(
            trimmed_mono, sample_rate, window_ms=50
        )

        # Also get zero-crossing loop end near the trim_end
        target_zc = loop_len - int(sample_rate * 0.01)
        lp_start_zc, lp_end_zc = find_loop_points_zero_crossing(trimmed_mono, target_zc)

        # Prefer RMS-match start with zero-crossing end
        loop_start = trim_start + lp_start_rms
        loop_end = trim_start + lp_end_zc
        loop_on = True

        if verbose:
            print(f"\nLoop points (RMS-match start + ZC end):")
            print(f"  loop_start (absolute): {loop_start}")
            print(f"  loop_end   (absolute): {loop_end}")
            print(f"  Loop length: {loop_end - loop_start} samples "
                  f"({(loop_end - loop_start) / sample_rate:.3f}s)")

            # Also report ZC-only option
            print(f"\nAlternative (zero-crossing only):")
            print(f"  loop_start: {trim_start + lp_start_zc}")
            print(f"  loop_end:   {trim_start + lp_end_zc}")

    # --- Step 4: Write output ---
    write_wav(output_path, float_samples, n_channels, sample_rate, sample_width,
              start_frame=final_start, end_frame=final_end)

    out_frames = final_end - final_start
    if verbose:
        out_duration = out_frames / sample_rate
        print(f"\nOutput written: {output_path}")
        print(f"  Duration:    {out_duration:.3f}s ({out_frames} frames)")

    # --- Step 5: Print XPM recommendations ---
    # XPM values are relative to the trimmed/written file (0-based)
    xpm_sample_start = 0
    xpm_sample_end = out_frames - 1
    if loop_on:
        xpm_loop_start = loop_start - final_start
        xpm_loop_end = loop_end - final_start
    else:
        xpm_loop_start = 0
        xpm_loop_end = 0

    print_xpm_recommendations(
        xpm_sample_start, xpm_sample_end,
        xpm_loop_start, xpm_loop_end,
        loop_on, detected_mode, verbose=verbose
    )

    return {
        'mode': detected_mode,
        'sample_start': xpm_sample_start,
        'sample_end': xpm_sample_end,
        'loop_start': xpm_loop_start,
        'loop_end': xpm_loop_end,
        'loop_on': loop_on,
        'sample_rate': sample_rate,
        'out_frames': out_frames,
    }


# ---------------------------------------------------------------------------
# CLI entry point
# ---------------------------------------------------------------------------

def main():
    parser = argparse.ArgumentParser(
        description='xpn_smart_trim — Automatic sample trimming and loop point detection.'
    )
    parser.add_argument('input', help='Input WAV file path')
    parser.add_argument('--output', '-o', default=None,
                        help='Output WAV file path (default: input_trimmed.wav)')
    parser.add_argument('--mode', choices=['one-shot', 'loop', 'auto'], default='auto',
                        help='Trim mode: one-shot, loop, or auto-detect (default: auto)')
    parser.add_argument('--verbose', '-v', action='store_true',
                        help='Print detailed analysis info')
    args = parser.parse_args()

    if not os.path.isfile(args.input):
        print(f"Error: input file not found: {args.input}", file=sys.stderr)
        sys.exit(1)

    if args.output is None:
        base, ext = os.path.splitext(args.input)
        args.output = base + '_trimmed' + ext

    try:
        result = process(args.input, args.output, mode=args.mode, verbose=args.verbose)
        if not args.verbose:
            print(f"Trimmed: {args.input} → {args.output} [{result['mode']}] "
                  f"{result['out_frames']} frames @ {result['sample_rate']} Hz")
    except Exception as e:
        print(f"Error: {e}", file=sys.stderr)
        if args.verbose:
            import traceback
            traceback.print_exc()
        sys.exit(1)


if __name__ == '__main__':
    main()
