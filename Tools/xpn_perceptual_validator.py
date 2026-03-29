"""XPN Perceptual Validator — spectral fingerprint QA for rendered samples.

Computes basic spectral features from WAV files using only stdlib (no numpy):
  - Spectral centroid (brightness proxy)
  - Zero-crossing rate (noisiness proxy)
  - RMS energy (loudness proxy)
  - Crest factor (transient character proxy)

These features are compared against expected Sonic DNA values to detect:
  - Silent renders (BlackHole routing failure)
  - Gain staging errors (too quiet or clipping)
  - Tonal mismatch (bright preset rendered dark, or vice versa)
  - Render-vs-live divergence

Usage (CLI):
    python xpn_perceptual_validator.py sample.wav
    python xpn_perceptual_validator.py sample.wav --dna '{"brightness": 0.7, "aggression": 0.4}'

Import:
    from xpn_perceptual_validator import compute_fingerprint, validate_against_dna
    fp = compute_fingerprint(Path("sample.wav"))
    result = validate_against_dna(fp, {"brightness": 0.7, "aggression": 0.4})
"""

import math
import struct
import sys
import json
import argparse
from pathlib import Path
from typing import Optional


# ---------------------------------------------------------------------------
# WAV decoding
# ---------------------------------------------------------------------------

def _read_pcm_samples(wav_path: Path) -> tuple[list[float], int]:
    """Read a WAV file and return (samples_as_float_list, sample_rate).

    Supports 16-bit, 24-bit, and 32-bit PCM. Returns mono mix-down; if the
    file is multi-channel the channels are averaged. Values are normalised to
    [-1.0, 1.0].

    Raises:
        ValueError: if the file is not PCM WAV or the format is unsupported.
        OSError: if the file cannot be opened.
    """
    with open(wav_path, "rb") as f:
        riff = f.read(4)
        if riff != b"RIFF":
            raise ValueError(f"Not a RIFF file: {wav_path}")
        f.read(4)  # file size — ignored
        wave_tag = f.read(4)
        if wave_tag != b"WAVE":
            raise ValueError(f"Not a WAVE file: {wav_path}")

        fmt_found = False
        num_channels = 1
        sample_rate = 44100
        bits_per_sample = 16
        data_bytes = b""

        while True:
            chunk_id = f.read(4)
            if len(chunk_id) < 4:
                break
            chunk_size_raw = f.read(4)
            if len(chunk_size_raw) < 4:
                break
            chunk_size = struct.unpack("<I", chunk_size_raw)[0]

            if chunk_id == b"fmt ":
                fmt_data = f.read(chunk_size)
                audio_fmt = struct.unpack("<H", fmt_data[0:2])[0]
                if audio_fmt != 1:
                    raise ValueError(
                        f"Unsupported audio format {audio_fmt} in {wav_path} "
                        f"(only PCM format=1 is supported)"
                    )
                num_channels = struct.unpack("<H", fmt_data[2:4])[0]
                sample_rate = struct.unpack("<I", fmt_data[4:8])[0]
                bits_per_sample = struct.unpack("<H", fmt_data[14:16])[0]
                fmt_found = True

            elif chunk_id == b"data":
                data_bytes = f.read(chunk_size)

            else:
                f.read(chunk_size)

            # WAV chunks are word-aligned
            if chunk_size % 2 != 0:
                f.read(1)

        if not fmt_found:
            raise ValueError(f"No fmt chunk found in {wav_path}")

    # Decode raw bytes to float samples
    bytes_per_sample = bits_per_sample // 8
    total_bytes = len(data_bytes)
    n_frames = total_bytes // (bytes_per_sample * num_channels)

    if n_frames == 0:
        return [], sample_rate

    if bits_per_sample == 16:
        n_total = n_frames * num_channels
        int_samples = struct.unpack(f"<{n_total}h", data_bytes[:n_total * 2])
        scale = 1.0 / 32768.0
        float_samples = [s * scale for s in int_samples]

    elif bits_per_sample == 24:
        float_samples = []
        idx = 0
        for _ in range(n_frames * num_channels):
            b = data_bytes[idx:idx + 3]
            if len(b) < 3:
                break
            unsigned = b[0] | (b[1] << 8) | (b[2] << 16)
            # Sign-extend from 24-bit
            if unsigned >= 0x800000:
                unsigned -= 0x1000000
            float_samples.append(unsigned / 8388608.0)
            idx += 3
        scale = 1.0  # already applied above

    elif bits_per_sample == 32:
        n_total = n_frames * num_channels
        int_samples = struct.unpack(f"<{n_total}i", data_bytes[:n_total * 4])
        scale = 1.0 / 2147483648.0
        float_samples = [s * scale for s in int_samples]

    else:
        raise ValueError(
            f"Unsupported bit depth {bits_per_sample} in {wav_path} "
            f"(supported: 16, 24, 32)"
        )

    # Mix down to mono if multi-channel
    if num_channels == 1:
        return float_samples, sample_rate

    mono = []
    for frame_idx in range(n_frames):
        base = frame_idx * num_channels
        frame_sum = sum(float_samples[base + ch] for ch in range(num_channels))
        mono.append(frame_sum / num_channels)
    return mono, sample_rate


# ---------------------------------------------------------------------------
# Spectral analysis — pure Python DFT via Goertzel algorithm
# ---------------------------------------------------------------------------

def _dft_magnitudes_goertzel(frame: list[float], n_bins: int,
                              fft_size: int) -> list[float]:
    """Compute DFT magnitudes for the first n_bins frequency bins using Goertzel.

    Goertzel evaluates the DFT at individual frequencies in O(N) each, making
    the total cost O(n_bins * N) instead of O(N^2) for a naive DFT. With a
    1024-point frame and 64 bins this is ~65K ops rather than 1M.

    The frame is Hann-windowed before analysis to reduce spectral leakage.
    """
    n = len(frame)
    # Apply Hann window in-place (copy to avoid mutating caller's list)
    windowed = [
        frame[i] * (0.5 - 0.5 * math.cos(2.0 * math.pi * i / n))
        for i in range(n)
    ]

    two_pi_over_n = 2.0 * math.pi / n
    magnitudes = []
    for k in range(n_bins):
        # Goertzel second-order IIR
        coeff = 2.0 * math.cos(two_pi_over_n * k)
        s_prev2 = 0.0
        s_prev1 = 0.0
        for sample in windowed:
            s = sample + coeff * s_prev1 - s_prev2
            s_prev2 = s_prev1
            s_prev1 = s
        # Final magnitude
        real = s_prev1 - s_prev2 * math.cos(two_pi_over_n * k)
        imag = s_prev2 * math.sin(two_pi_over_n * k)
        magnitudes.append(math.sqrt(real * real + imag * imag))

    return magnitudes


# ---------------------------------------------------------------------------
# Feature functions
# ---------------------------------------------------------------------------

def spectral_centroid(samples: list[float], sample_rate: int,
                      fft_size: int = 1024, n_bins: int = 128) -> float:
    """Compute spectral centroid in Hz.

    Takes a single Hann-windowed frame from the middle of the signal and
    computes the magnitude-weighted mean frequency using Goertzel DFT.

    Args:
        samples:     Mono float samples in [-1, 1].
        sample_rate: Sample rate in Hz.
        fft_size:    DFT frame size (power of 2 recommended).
        n_bins:      Number of positive-frequency bins to evaluate.
                     Using fewer bins trades accuracy for speed; 128 bins
                     gives adequate resolution up to ~5.5 kHz at 44100 Hz.

    Returns:
        Frequency in Hz. Higher = brighter. Returns 0.0 for silence.
    """
    if len(samples) < fft_size:
        # Pad with zeros rather than returning 0 immediately
        frame = samples + [0.0] * (fft_size - len(samples))
    else:
        mid = len(samples) // 2
        start = max(0, mid - fft_size // 2)
        frame = list(samples[start:start + fft_size])
        if len(frame) < fft_size:
            frame = frame + [0.0] * (fft_size - len(frame))

    n_bins = min(n_bins, fft_size // 2)
    magnitudes = _dft_magnitudes_goertzel(frame, n_bins, fft_size)

    total_mag = sum(magnitudes)
    if total_mag < 1e-10:
        return 0.0

    freq_resolution = sample_rate / fft_size
    centroid = sum(k * freq_resolution * magnitudes[k] for k in range(n_bins)) / total_mag
    return centroid


def zero_crossing_rate(samples: list[float]) -> float:
    """Fraction of adjacent sample pairs that cross zero. Range: [0.0, 1.0].

    Higher values indicate noisier or brighter signals. A pure tone produces
    ZCR proportional to 2 * freq / sample_rate. White noise approaches 0.5.
    """
    if len(samples) < 2:
        return 0.0
    crossings = sum(
        1 for i in range(1, len(samples))
        if (samples[i] >= 0.0) != (samples[i - 1] >= 0.0)
    )
    return crossings / (len(samples) - 1)


def rms_energy(samples: list[float]) -> float:
    """RMS energy of the signal in dBFS.

    Returns float('-inf') for silence (all-zero or empty input).
    """
    if not samples:
        return float("-inf")
    mean_sq = sum(s * s for s in samples) / len(samples)
    if mean_sq < 1e-20:
        return float("-inf")
    return 20.0 * math.log10(math.sqrt(mean_sq))


def crest_factor(samples: list[float]) -> float:
    """Peak-to-RMS ratio in dB.

    Higher = more transient character. Sustained tones typically yield 3–6 dB;
    percussive hits commonly exceed 12–20 dB.

    Returns 0.0 for silence or empty input.
    """
    if not samples:
        return 0.0
    peak = max(abs(s) for s in samples)
    mean_sq = sum(s * s for s in samples) / len(samples)
    rms = math.sqrt(mean_sq)
    if rms < 1e-10 or peak < 1e-10:
        return 0.0
    return 20.0 * math.log10(peak / rms)


# ---------------------------------------------------------------------------
# High-level fingerprint
# ---------------------------------------------------------------------------

def compute_fingerprint(wav_path: Path) -> dict:
    """Compute a perceptual fingerprint for a WAV file.

    Reads the WAV, mixes to mono, then derives four acoustic features and
    maps two of them to estimated Sonic DNA axes (brightness, aggression).

    Returns:
        {
            "centroid_hz":      float,   # spectral centroid in Hz
            "zcr":              float,   # zero-crossing rate [0.0, 1.0]
            "rms_dbfs":         float,   # RMS energy in dBFS
            "crest_db":         float,   # crest factor in dB
            "brightness_est":   float,   # estimated DNA brightness [0.0, 1.0]
            "aggression_est":   float,   # estimated DNA aggression [0.0, 1.0]
        }

    Brightness mapping:
        200 Hz → 0.0 (very dark), 5000 Hz → 1.0 (very bright).
        Centroid is read from a single mid-signal frame, so transient-heavy
        material may read slightly brighter than steady state.

    Aggression mapping:
        crest < 6 dB  → 0.0 (sustained / pad-like)
        crest > 20 dB → 1.0 (highly transient / percussive)
    """
    samples, sr = _read_pcm_samples(wav_path)

    centroid = spectral_centroid(samples, sr)
    zcr = zero_crossing_rate(samples)
    rms = rms_energy(samples)
    crest = crest_factor(samples)

    # Map features to estimated DNA values [0.0, 1.0]
    # Centroid: 200 Hz = 0.0 (dark), 5000 Hz = 1.0 (bright)
    brightness_est = max(0.0, min(1.0, (centroid - 200.0) / 4800.0))
    # Aggression: crest < 6 dB = 0.0 (sustained), crest > 20 dB = 1.0 (transient)
    aggression_est = max(0.0, min(1.0, (crest - 6.0) / 14.0))

    return {
        "centroid_hz":    round(centroid, 1),
        "zcr":            round(zcr, 4),
        "rms_dbfs":       round(rms, 1),
        "crest_db":       round(crest, 1),
        "brightness_est": round(brightness_est, 3),
        "aggression_est": round(aggression_est, 3),
    }


# ---------------------------------------------------------------------------
# DNA validation
# ---------------------------------------------------------------------------

def validate_against_dna(fingerprint: dict, expected_dna: dict,
                         tolerance: float = 0.35) -> dict:
    """Compare a perceptual fingerprint against expected Sonic DNA values.

    Checks for:
      - Silent render  (rms < -60 dBFS)
      - Brightness mismatch  (|brightness_est - expected_brightness| > tolerance)
      - Aggression mismatch  (|aggression_est - expected_aggression| > tolerance)

    Args:
        fingerprint:  Output dict from compute_fingerprint().
        expected_dna: Dict with any subset of the 6D DNA keys. Only
                      "brightness" and "aggression" are evaluated here;
                      remaining dimensions require more complex multi-frame
                      analysis and are beyond the scope of this module.
        tolerance:    Maximum allowed absolute delta before a mismatch warning
                      is raised. Default 0.35 (35 percentage points on a 0–1
                      scale). Tighten to 0.20 for strict QA; loosen to 0.50
                      for coarse smoke-tests.

    Returns:
        {
            "valid":              bool,
            "warnings":           list[str],
            "brightness_delta":   float,  # fingerprint.brightness_est - expected
            "aggression_delta":   float,  # fingerprint.aggression_est - expected
        }
    """
    warnings: list[str] = []

    # --- Silent render check -------------------------------------------
    rms = fingerprint.get("rms_dbfs", float("-inf"))
    if rms == float("-inf") or rms < -60.0:
        warnings.append(
            f"SILENT: RMS={rms:.1f} dBFS — likely render failure "
            f"(check BlackHole routing or audio interface selection)"
        )

    # --- Brightness mismatch -------------------------------------------
    expected_brightness = float(expected_dna.get("brightness", 0.5))
    brightness_delta = fingerprint["brightness_est"] - expected_brightness
    if abs(brightness_delta) > tolerance:
        direction = "brighter" if brightness_delta > 0 else "darker"
        warnings.append(
            f"BRIGHTNESS MISMATCH: rendered {direction} than expected "
            f"(got {fingerprint['brightness_est']:.2f}, "
            f"expected {expected_brightness:.2f}, "
            f"delta={brightness_delta:+.2f})"
        )

    # --- Aggression mismatch -------------------------------------------
    expected_aggression = float(expected_dna.get("aggression", 0.5))
    aggression_delta = fingerprint["aggression_est"] - expected_aggression
    if abs(aggression_delta) > tolerance:
        direction = "more transient" if aggression_delta > 0 else "more sustained"
        warnings.append(
            f"AGGRESSION MISMATCH: rendered {direction} than expected "
            f"(got {fingerprint['aggression_est']:.2f}, "
            f"expected {expected_aggression:.2f}, "
            f"delta={aggression_delta:+.2f})"
        )

    return {
        "valid":            len(warnings) == 0,
        "warnings":         warnings,
        "brightness_delta": round(brightness_delta, 3),
        "aggression_delta": round(aggression_delta, 3),
    }


# ---------------------------------------------------------------------------
# Batch validation helper
# ---------------------------------------------------------------------------

def validate_batch(wav_paths: list[Path], expected_dna: dict,
                   tolerance: float = 0.35,
                   verbose: bool = True) -> dict:
    """Validate a list of WAV files against a shared Sonic DNA spec.

    Returns:
        {
            "total":   int,
            "passed":  int,
            "failed":  int,
            "results": list[{"path": str, "fingerprint": dict, "validation": dict}]
        }
    """
    results = []
    passed = 0
    failed = 0

    for wav_path in wav_paths:
        try:
            fp = compute_fingerprint(wav_path)
            validation = validate_against_dna(fp, expected_dna, tolerance)
        except (ValueError, OSError) as e:
            fp = {}
            validation = {
                "valid": False,
                "warnings": [f"READ ERROR: {e}"],
                "brightness_delta": 0.0,
                "aggression_delta": 0.0,
            }

        if validation["valid"]:
            passed += 1
            if verbose:
                print(f"  PASS  {wav_path.name}  "
                      f"centroid={fp.get('centroid_hz', '?')} Hz  "
                      f"rms={fp.get('rms_dbfs', '?')} dBFS")
        else:
            failed += 1
            if verbose:
                print(f"  FAIL  {wav_path.name}")
                for w in validation["warnings"]:
                    print(f"        {w}")

        results.append({
            "path":        str(wav_path),
            "fingerprint": fp,
            "validation":  validation,
        })

    return {
        "total":   len(wav_paths),
        "passed":  passed,
        "failed":  failed,
        "results": results,
    }


# ---------------------------------------------------------------------------
# CLI entry point
# ---------------------------------------------------------------------------

def _build_arg_parser() -> argparse.ArgumentParser:
    p = argparse.ArgumentParser(
        description="XPN Perceptual Validator — spectral fingerprint QA for rendered samples."
    )
    p.add_argument("wav", nargs="+", help="WAV file(s) to analyse")
    p.add_argument(
        "--dna",
        default="{}",
        metavar="JSON",
        help='Expected Sonic DNA as JSON, e.g. \'{"brightness": 0.7, "aggression": 0.4}\'',
    )
    p.add_argument(
        "--tolerance",
        type=float,
        default=0.35,
        help="Maximum allowed brightness/aggression delta (default 0.35)",
    )
    p.add_argument(
        "--json",
        action="store_true",
        dest="output_json",
        help="Emit results as JSON instead of human-readable text",
    )
    return p


def main(argv: Optional[list[str]] = None) -> int:
    parser = _build_arg_parser()
    args = parser.parse_args(argv)

    try:
        expected_dna = json.loads(args.dna)
    except json.JSONDecodeError as e:
        print(f"ERROR: Invalid --dna JSON: {e}", file=sys.stderr)
        return 2

    wav_paths = [Path(p) for p in args.wav]

    if args.output_json:
        # Collect all results and dump as JSON
        results = []
        for wav_path in wav_paths:
            try:
                fp = compute_fingerprint(wav_path)
                validation = validate_against_dna(fp, expected_dna, args.tolerance)
            except (ValueError, OSError) as e:
                fp = {}
                validation = {
                    "valid": False,
                    "warnings": [f"READ ERROR: {e}"],
                    "brightness_delta": 0.0,
                    "aggression_delta": 0.0,
                }
            results.append({
                "path":        str(wav_path),
                "fingerprint": fp,
                "validation":  validation,
            })
        print(json.dumps(results, indent=2))
        failed = sum(1 for r in results if not r["validation"]["valid"])
        return 0 if failed == 0 else 1

    # Human-readable output
    batch = validate_batch(wav_paths, expected_dna, args.tolerance, verbose=True)
    print(f"\n{batch['passed']}/{batch['total']} passed")
    return 0 if batch["failed"] == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
