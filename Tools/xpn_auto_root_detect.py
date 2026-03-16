#!/usr/bin/env python3
"""
xpn_auto_root_detect.py — Auto-detect fundamental pitch of a WAV sample.

Returns the closest MIDI note number (0-127) for use as RootNote in XPM keygroup assignments.

CLI usage:
    python xpn_auto_root_detect.py sample.wav [--verbose]

Importable:
    from xpn_auto_root_detect import detect_root_note
"""

import wave
import struct
import math
import json
import argparse
import sys
from collections import Counter


# ---------------------------------------------------------------------------
# MIDI / frequency utilities
# ---------------------------------------------------------------------------

NOTE_NAMES = ["C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"]


def hz_to_midi(freq_hz: float) -> float:
    """Convert frequency in Hz to a (possibly fractional) MIDI note number."""
    if freq_hz <= 0:
        return 60.0
    return 69.0 + 12.0 * math.log2(freq_hz / 440.0)


def midi_to_hz(midi: int) -> float:
    """Convert MIDI note number to frequency in Hz."""
    return 440.0 * (2.0 ** ((midi - 69) / 12.0))


def midi_to_note_name(midi: int) -> str:
    """Convert MIDI note number to note name like 'C4', 'A#3'."""
    octave = (midi // 12) - 1
    name = NOTE_NAMES[midi % 12]
    return f"{name}{octave}"


# ---------------------------------------------------------------------------
# WAV loading
# ---------------------------------------------------------------------------

def load_wav_mono(path: str) -> tuple[list[float], int]:
    """
    Load a WAV file and return (samples, sample_rate).
    Samples are normalized floats in [-1.0, 1.0], mixed to mono.
    Supports 8-bit, 16-bit, 24-bit, and 32-bit PCM.
    """
    with wave.open(path, "rb") as wf:
        n_channels = wf.getnchannels()
        sample_rate = wf.getframerate()
        n_frames = wf.getnframes()
        sampwidth = wf.getsampwidth()  # bytes per sample
        raw = wf.readframes(n_frames)

    # Decode samples
    samples_interleaved = []
    if sampwidth == 1:
        # 8-bit PCM is unsigned
        for byte in raw:
            samples_interleaved.append((byte - 128) / 128.0)
    elif sampwidth == 2:
        n = len(raw) // 2
        for i in range(n):
            val = struct.unpack_from("<h", raw, i * 2)[0]
            samples_interleaved.append(val / 32768.0)
    elif sampwidth == 3:
        n = len(raw) // 3
        for i in range(n):
            b0, b1, b2 = raw[i*3], raw[i*3+1], raw[i*3+2]
            val = (b2 << 16) | (b1 << 8) | b0
            if val >= 0x800000:
                val -= 0x1000000
            samples_interleaved.append(val / 8388608.0)
    elif sampwidth == 4:
        n = len(raw) // 4
        for i in range(n):
            val = struct.unpack_from("<i", raw, i * 4)[0]
            samples_interleaved.append(val / 2147483648.0)
    else:
        raise ValueError(f"Unsupported sample width: {sampwidth} bytes")

    # Mix down to mono
    if n_channels == 1:
        mono = samples_interleaved
    else:
        mono = []
        for i in range(0, len(samples_interleaved), n_channels):
            frame = samples_interleaved[i:i + n_channels]
            mono.append(sum(frame) / n_channels)

    return mono, sample_rate


# ---------------------------------------------------------------------------
# Autocorrelation pitch detection (YIN-inspired, simplified)
# ---------------------------------------------------------------------------

def autocorrelation(samples: list[float], lag: int) -> float:
    """Compute autocorrelation of samples at a given lag."""
    n = len(samples) - lag
    if n <= 0:
        return 0.0
    total = 0.0
    for i in range(n):
        total += samples[i] * samples[i + lag]
    return total / n


def detect_pitch_in_window(
    samples: list[float],
    sample_rate: int,
    min_freq: float = 40.0,
    max_freq: float = 4200.0,
) -> tuple[float, float]:
    """
    Detect pitch in a single window of samples using autocorrelation.
    Returns (frequency_hz, confidence) where confidence is in [0, 1].
    """
    n = len(samples)

    # Lag bounds derived from frequency limits
    lag_min = max(1, int(sample_rate / max_freq))
    lag_max = min(n // 2, int(sample_rate / min_freq))

    if lag_min >= lag_max:
        return 0.0, 0.0

    # Lag 0 autocorrelation (energy normalization)
    r0 = autocorrelation(samples, 0)
    if r0 < 1e-10:
        return 0.0, 0.0  # silence

    # Compute autocorrelation over lag range
    best_lag = lag_min
    best_val = -1.0
    for lag in range(lag_min, lag_max + 1):
        r = autocorrelation(samples, lag)
        if r > best_val:
            best_val = r
            best_lag = lag

    confidence = best_val / r0
    freq = sample_rate / best_lag
    return freq, confidence


def detect_root_note(
    path: str,
    window_size: int = 2048,
    hop_size: int = 512,
    verbose: bool = False,
) -> dict:
    """
    Detect the fundamental pitch of a WAV file.

    Returns a dict:
        {
            "midi_note": int,
            "note_name": str,
            "frequency_hz": float,
            "confidence": float,
            "is_pitched": bool
        }
    """
    samples, sample_rate = load_wav_mono(path)

    if len(samples) == 0:
        return {
            "midi_note": 60,
            "note_name": "C4",
            "frequency_hz": 261.63,
            "confidence": 0.0,
            "is_pitched": False,
        }

    # Collect pitch estimates per window
    window_freqs = []
    window_confs = []
    window_count = 0

    pos = 0
    while pos + window_size <= len(samples):
        window = samples[pos: pos + window_size]
        freq, conf = detect_pitch_in_window(window, sample_rate)
        window_count += 1

        if verbose:
            midi_candidate = round(hz_to_midi(freq)) if freq > 0 else 60
            note_candidate = midi_to_note_name(max(0, min(127, midi_candidate)))
            print(
                f"  Window {window_count:4d} @ {pos:6d}: "
                f"freq={freq:7.2f} Hz  note={note_candidate:4s}  conf={conf:.3f}"
            )

        if conf > 0.0 and freq > 0.0:
            window_freqs.append(freq)
            window_confs.append(conf)

        pos += hop_size

    if verbose:
        print(f"\n  Total windows analyzed: {window_count}")
        print(f"  Windows with pitch candidates: {len(window_freqs)}")

    if not window_freqs:
        return {
            "midi_note": 60,
            "note_name": "C4",
            "frequency_hz": 261.63,
            "confidence": 0.0,
            "is_pitched": False,
        }

    # Convert each window frequency to MIDI, then vote by rounded note
    midi_votes: list[int] = []
    conf_by_midi: dict[int, list[float]] = {}

    for freq, conf in zip(window_freqs, window_confs):
        raw_midi = hz_to_midi(freq)
        midi_rounded = max(0, min(127, round(raw_midi)))
        midi_votes.append(midi_rounded)
        conf_by_midi.setdefault(midi_rounded, []).append(conf)

    # Find consensus MIDI note (most votes)
    vote_counts = Counter(midi_votes)
    best_midi = vote_counts.most_common(1)[0][0]

    # Average confidence for winning note
    best_conf = sum(conf_by_midi[best_midi]) / len(conf_by_midi[best_midi])

    # Overall mean confidence (weighted signal quality)
    overall_conf = sum(window_confs) / len(window_confs)
    final_conf = (best_conf + overall_conf) / 2.0

    is_pitched = final_conf >= 0.3

    if not is_pitched:
        if verbose:
            print(f"\n  Low confidence ({final_conf:.3f}) — defaulting to C4 (MIDI 60)")
        best_midi = 60

    best_freq = midi_to_hz(best_midi)
    note_name = midi_to_note_name(best_midi)

    if verbose:
        print(f"\n  Consensus MIDI note : {best_midi} ({note_name})")
        print(f"  Estimated frequency : {best_freq:.2f} Hz")
        print(f"  Confidence          : {final_conf:.3f}")
        print(f"  Is pitched          : {is_pitched}")

    return {
        "midi_note": best_midi,
        "note_name": note_name,
        "frequency_hz": round(best_freq, 2),
        "confidence": round(final_conf, 4),
        "is_pitched": is_pitched,
    }


# ---------------------------------------------------------------------------
# CLI entry point
# ---------------------------------------------------------------------------

def main():
    parser = argparse.ArgumentParser(
        description="Auto-detect fundamental pitch of a WAV sample and return MIDI root note."
    )
    parser.add_argument("wav_file", help="Path to WAV file")
    parser.add_argument(
        "--verbose", "-v", action="store_true", help="Print per-window analysis"
    )
    parser.add_argument(
        "--window", type=int, default=2048, help="Analysis window size in samples (default: 2048)"
    )
    parser.add_argument(
        "--hop", type=int, default=512, help="Hop size between windows in samples (default: 512)"
    )
    args = parser.parse_args()

    if verbose := args.verbose:
        print(f"Analyzing: {args.wav_file}\n")

    try:
        result = detect_root_note(
            args.wav_file,
            window_size=args.window,
            hop_size=args.hop,
            verbose=args.verbose,
        )
    except FileNotFoundError:
        print(f"Error: file not found: {args.wav_file}", file=sys.stderr)
        sys.exit(1)
    except Exception as e:
        print(f"Error: {e}", file=sys.stderr)
        sys.exit(1)

    if verbose:
        print()

    print(json.dumps(result, indent=2))


if __name__ == "__main__":
    main()
