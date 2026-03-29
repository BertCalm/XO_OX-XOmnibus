"""Tests for xpn_perceptual_validator — spectral fingerprint QA module.

Run directly:
    python test_oxport_perceptual.py

Or via pytest:
    pytest test_oxport_perceptual.py -v

All test WAVs are generated in-memory using struct (16-bit PCM, 44100 Hz, mono)
and written to a temp file for the duration of the test, then deleted.
"""

import math
import os
import struct
import sys
import tempfile
from pathlib import Path

sys.path.insert(0, os.path.dirname(__file__))


# ---------------------------------------------------------------------------
# WAV writing helper (mirrors oxport._write_wav — no import dependency)
# ---------------------------------------------------------------------------

def _write_test_wav(path: str, samples_float: list[float],
                    sample_rate: int = 44100) -> None:
    """Write a mono 16-bit PCM WAV file from a list of float samples [-1, 1]."""
    pcm = [max(-32768, min(32767, int(s * 32767))) for s in samples_float]
    raw = struct.pack(f"<{len(pcm)}h", *pcm)
    data_size = len(raw)
    file_size = 36 + data_size

    with open(path, "wb") as f:
        f.write(b"RIFF")
        f.write(struct.pack("<I", file_size))
        f.write(b"WAVE")
        f.write(b"fmt ")
        f.write(struct.pack("<I", 16))
        f.write(struct.pack("<H", 1))          # PCM
        f.write(struct.pack("<H", 1))          # mono
        f.write(struct.pack("<I", sample_rate))
        f.write(struct.pack("<I", sample_rate * 2))  # byte rate
        f.write(struct.pack("<H", 2))          # block align
        f.write(struct.pack("<H", 16))         # bits per sample
        f.write(b"data")
        f.write(struct.pack("<I", data_size))
        f.write(raw)


def _sine_samples(freq_hz: float, duration_s: float = 1.0,
                  amplitude: float = 0.8,
                  sample_rate: int = 44100) -> list[float]:
    n = int(sample_rate * duration_s)
    return [amplitude * math.sin(2 * math.pi * freq_hz * i / sample_rate)
            for i in range(n)]


def _silence_samples(duration_s: float = 0.5,
                     sample_rate: int = 44100) -> list[float]:
    return [0.0] * int(sample_rate * duration_s)


def _noise_samples(n: int = 22050, amplitude: float = 0.5) -> list[float]:
    """Deterministic pseudo-noise: alternating +amplitude / -amplitude."""
    return [amplitude * (1 if i % 2 == 0 else -1) for i in range(n)]


# ---------------------------------------------------------------------------
# Fixtures: temp-file context manager
# ---------------------------------------------------------------------------

class _TempWav:
    """Context manager that writes a WAV file to a temp path and cleans up."""

    def __init__(self, samples: list[float], sample_rate: int = 44100):
        self._samples = samples
        self._sr = sample_rate
        self._path: str = ""

    def __enter__(self) -> Path:
        fd, self._path = tempfile.mkstemp(suffix=".wav")
        os.close(fd)
        _write_test_wav(self._path, self._samples, self._sr)
        return Path(self._path)

    def __exit__(self, *_) -> None:
        try:
            os.unlink(self._path)
        except OSError:
            pass


# ---------------------------------------------------------------------------
# Tests
# ---------------------------------------------------------------------------

def test_fingerprint_sine_is_bright():
    """A 4 kHz sine wave should have centroid > 3000 Hz and brightness_est > 0.5.

    The spectral centroid of a pure tone equals its frequency.  Given the
    mapping centroid=5000 Hz → brightness=1.0, a 4 kHz tone should resolve
    to brightness ≈ 0.79.
    """
    from xpn_perceptual_validator import compute_fingerprint

    with _TempWav(_sine_samples(4000, duration_s=0.5)) as wav:
        fp = compute_fingerprint(wav)

    assert fp["centroid_hz"] > 3000, (
        f"Expected centroid > 3000 Hz for 4 kHz sine, got {fp['centroid_hz']}"
    )
    assert fp["brightness_est"] > 0.5, (
        f"Expected brightness_est > 0.5 for 4 kHz sine, got {fp['brightness_est']}"
    )


def test_fingerprint_low_sine_is_dark():
    """A 100 Hz sine wave should have brightness_est < 0.2 (centroid near 100 Hz).

    Mapping: brightness = max(0, (centroid - 200) / 4800).
    At 100 Hz centroid the result clamps to 0.0.
    """
    from xpn_perceptual_validator import compute_fingerprint

    with _TempWav(_sine_samples(100, duration_s=1.0)) as wav:
        fp = compute_fingerprint(wav)

    assert fp["brightness_est"] < 0.2, (
        f"Expected brightness_est < 0.2 for 100 Hz sine, got {fp['brightness_est']} "
        f"(centroid={fp['centroid_hz']} Hz)"
    )


def test_fingerprint_silence_detected():
    """An all-zero WAV should report rms_dbfs < -60 (effectively silent)."""
    from xpn_perceptual_validator import compute_fingerprint

    with _TempWav(_silence_samples()) as wav:
        fp = compute_fingerprint(wav)

    # rms_energy() returns float('-inf') for all-zero input; the fingerprint
    # rounds it but should remain well below -60 dBFS.
    rms = fp["rms_dbfs"]
    assert rms == float("-inf") or rms < -60.0, (
        f"Expected rms_dbfs < -60 for silence, got {rms}"
    )


def test_fingerprint_noise_has_high_zcr():
    """Alternating +/- noise should have ZCR close to 1.0 (crosses every sample).

    The alternating pattern [+A, -A, +A, -A, ...] crosses zero on every pair,
    so ZCR = (n-1)/(n-1) = 1.0.  After rounding, the value should be >= 0.9.
    """
    from xpn_perceptual_validator import compute_fingerprint

    with _TempWav(_noise_samples()) as wav:
        fp = compute_fingerprint(wav)

    assert fp["zcr"] > 0.3, (
        f"Expected zcr > 0.3 for alternating noise, got {fp['zcr']}"
    )


def test_validate_matching_dna_passes():
    """A fingerprint that matches the expected DNA within tolerance should be valid.

    We construct a fingerprint dict directly (no WAV I/O needed) and supply
    a DNA spec where the estimated values are well within ±0.35 tolerance.
    """
    from xpn_perceptual_validator import validate_against_dna

    fingerprint = {
        "centroid_hz":    1200.0,
        "zcr":            0.05,
        "rms_dbfs":       -12.0,
        "crest_db":       8.0,
        "brightness_est": 0.40,   # 200 + 0.40 * 4800 = 2120 Hz centroid
        "aggression_est": 0.14,   # crest = 6 + 0.14 * 14 ≈ 7.96 dB
    }
    expected_dna = {"brightness": 0.45, "aggression": 0.20}

    result = validate_against_dna(fingerprint, expected_dna, tolerance=0.35)

    assert result["valid"], (
        f"Expected valid=True for matching DNA, got warnings: {result['warnings']}"
    )
    assert result["warnings"] == [], (
        f"Expected no warnings, got: {result['warnings']}"
    )


def test_validate_mismatched_brightness_warns():
    """A bright fingerprint paired with a dark DNA spec must raise a brightness warning.

    brightness_est = 0.85 vs expected = 0.20 → delta = +0.65 > tolerance 0.35.
    """
    from xpn_perceptual_validator import validate_against_dna

    fingerprint = {
        "centroid_hz":    4280.0,
        "zcr":            0.18,
        "rms_dbfs":       -10.0,
        "crest_db":       7.0,
        "brightness_est": 0.85,   # very bright
        "aggression_est": 0.07,
    }
    expected_dna = {"brightness": 0.20, "aggression": 0.10}

    result = validate_against_dna(fingerprint, expected_dna, tolerance=0.35)

    assert not result["valid"], (
        "Expected valid=False for bright fingerprint vs dark DNA"
    )
    brightness_warnings = [w for w in result["warnings"] if "BRIGHTNESS" in w]
    assert len(brightness_warnings) >= 1, (
        f"Expected at least one BRIGHTNESS warning, got: {result['warnings']}"
    )
    assert result["brightness_delta"] > 0.35, (
        f"Expected brightness_delta > 0.35, got {result['brightness_delta']}"
    )


def test_validate_silent_render_warns():
    """A fingerprint with rms_dbfs < -60 must trigger a SILENT render warning."""
    from xpn_perceptual_validator import validate_against_dna

    fingerprint = {
        "centroid_hz":    0.0,
        "zcr":            0.0,
        "rms_dbfs":       float("-inf"),
        "crest_db":       0.0,
        "brightness_est": 0.0,
        "aggression_est": 0.0,
    }
    expected_dna = {"brightness": 0.5, "aggression": 0.3}

    result = validate_against_dna(fingerprint, expected_dna, tolerance=0.35)

    assert not result["valid"], (
        "Expected valid=False for silent fingerprint"
    )
    silent_warnings = [w for w in result["warnings"] if "SILENT" in w]
    assert len(silent_warnings) >= 1, (
        f"Expected at least one SILENT warning, got: {result['warnings']}"
    )


# ---------------------------------------------------------------------------
# Additional edge-case tests
# ---------------------------------------------------------------------------

def test_read_pcm_samples_16bit_roundtrip():
    """_read_pcm_samples must decode a 16-bit WAV to float samples in [-1, 1]."""
    from xpn_perceptual_validator import _read_pcm_samples

    sr = 22050
    n = 512
    original = [math.sin(2 * math.pi * 440 * i / sr) * 0.5 for i in range(n)]

    with _TempWav(original, sample_rate=sr) as wav:
        samples, sample_rate_out = _read_pcm_samples(wav)

    assert sample_rate_out == sr, f"Expected sr={sr}, got {sample_rate_out}"
    assert len(samples) == n, f"Expected {n} samples, got {len(samples)}"
    # Samples normalised: all in [-1, 1]
    assert all(-1.0 <= s <= 1.0 for s in samples), (
        "Some decoded samples are outside [-1.0, 1.0]"
    )
    # Round-trip should preserve amplitude within 16-bit quantisation error (~±0.00004)
    for i in range(n):
        assert abs(samples[i] - original[i]) < 0.001, (
            f"Sample {i}: decoded={samples[i]:.6f}, original={original[i]:.6f}, "
            f"delta={abs(samples[i] - original[i]):.6f}"
        )


def test_rms_energy_known_value():
    """A full-scale sine wave must measure ~-3.01 dBFS RMS (analytical result)."""
    from xpn_perceptual_validator import rms_energy

    sr = 44100
    samples = [math.sin(2 * math.pi * 1000 * i / sr) for i in range(sr)]
    rms = rms_energy(samples)
    # RMS of sin = 1/sqrt(2) ≈ 0.707 → 20*log10(0.707) ≈ -3.01 dBFS
    assert -3.5 < rms < -2.5, f"Expected ~-3.01 dBFS, got {rms:.3f}"


def test_crest_factor_sine_is_low():
    """A pure sine should have a crest factor of ~3 dB (peak/RMS = sqrt(2))."""
    from xpn_perceptual_validator import crest_factor

    sr = 44100
    samples = [math.sin(2 * math.pi * 440 * i / sr) for i in range(sr)]
    cf = crest_factor(samples)
    # Peak = 1.0, RMS = 1/sqrt(2) → crest = 20*log10(sqrt(2)) ≈ 3.01 dB
    assert 2.5 < cf < 4.0, f"Expected crest factor ~3.01 dB for sine, got {cf:.3f}"


def test_validate_rms_just_below_threshold():
    """A fingerprint at exactly -60.1 dBFS must trigger the SILENT warning."""
    from xpn_perceptual_validator import validate_against_dna

    fingerprint = {
        "centroid_hz":    500.0,
        "zcr":            0.02,
        "rms_dbfs":       -60.1,
        "crest_db":       6.0,
        "brightness_est": 0.06,
        "aggression_est": 0.0,
    }
    result = validate_against_dna(fingerprint, {"brightness": 0.06}, tolerance=0.35)

    silent_warnings = [w for w in result["warnings"] if "SILENT" in w]
    assert len(silent_warnings) >= 1, (
        f"Expected SILENT warning for rms=-60.1 dBFS, got: {result['warnings']}"
    )


def test_compute_fingerprint_end_to_end():
    """compute_fingerprint must return a dict with all required keys for a valid WAV."""
    from xpn_perceptual_validator import compute_fingerprint

    required_keys = {"centroid_hz", "zcr", "rms_dbfs", "crest_db",
                     "brightness_est", "aggression_est"}

    with _TempWav(_sine_samples(880, duration_s=0.25)) as wav:
        fp = compute_fingerprint(wav)

    missing = required_keys - fp.keys()
    assert not missing, f"Fingerprint missing keys: {missing}"
    # All values must be numeric
    for k, v in fp.items():
        assert isinstance(v, (int, float)), (
            f"Key '{k}' has non-numeric value: {v!r}"
        )


# ---------------------------------------------------------------------------
# Manual test runner
# ---------------------------------------------------------------------------

if __name__ == "__main__":
    tests = [
        test_fingerprint_sine_is_bright,
        test_fingerprint_low_sine_is_dark,
        test_fingerprint_silence_detected,
        test_fingerprint_noise_has_high_zcr,
        test_validate_matching_dna_passes,
        test_validate_mismatched_brightness_warns,
        test_validate_silent_render_warns,
        test_read_pcm_samples_16bit_roundtrip,
        test_rms_energy_known_value,
        test_crest_factor_sine_is_low,
        test_validate_rms_just_below_threshold,
        test_compute_fingerprint_end_to_end,
    ]

    passed = 0
    failed = 0
    for t in tests:
        try:
            t()
            print(f"  PASS  {t.__name__}")
            passed += 1
        except Exception as exc:
            import traceback
            print(f"  FAIL  {t.__name__}: {exc}")
            traceback.print_exc()
            failed += 1

    print(f"\n{passed} passed, {failed} failed")
    sys.exit(0 if failed == 0 else 1)
