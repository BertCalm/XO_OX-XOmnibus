"""Core tests for oxport pipeline functions."""
import math
import struct
import tempfile
import os
import sys
sys.path.insert(0, os.path.dirname(__file__))


def test_compute_rms_db_sine():
    """A full-scale sine wave at 1kHz should measure ~-3.01 dBFS RMS."""
    from oxport import _compute_rms_db, _write_wav
    from pathlib import Path

    sr = 44100
    freq = 1000
    samples = [math.sin(2 * math.pi * freq * i / sr) for i in range(sr)]
    # Convert to 16-bit PCM
    raw = struct.pack(f"<{len(samples)}h", *[int(s * 32767) for s in samples])
    with tempfile.NamedTemporaryFile(suffix=".wav", delete=False) as f:
        path = f.name
    try:
        _write_wav(Path(path), 1, sr, 16, raw)
        rms = _compute_rms_db(Path(path))
        # Full-scale sine RMS = -3.01 dBFS
        assert -3.5 < rms < -2.5, f"Expected ~-3.01 dBFS, got {rms}"
    finally:
        os.unlink(path)


def test_wav_roundtrip_24bit():
    """Write a 24-bit WAV and read it back — data should be identical."""
    from oxport import _write_wav, _read_wav_raw
    from pathlib import Path

    sr = 48000
    n_samples = 1000
    # Create known 24-bit data: alternating positive/negative
    raw = b""
    for i in range(n_samples):
        val = (i * 100) % 8388607
        if i % 2:
            val = -val
        # Pack as 3-byte little-endian signed
        unsigned = val & 0xFFFFFF
        raw += struct.pack("<I", unsigned)[:3]
    with tempfile.NamedTemporaryFile(suffix=".wav", delete=False) as f:
        path = f.name
    try:
        _write_wav(Path(path), 1, sr, 24, raw)
        ch, sr_out, bps, data_out = _read_wav_raw(Path(path))
        assert ch == 1
        assert sr_out == sr
        assert bps == 24
        assert data_out == raw
    finally:
        os.unlink(path)


def test_engine_id_resolution():
    """All known spellings of 'onset' should resolve to the canonical 'Onset'."""
    from oxport import resolve_engine_name, DRUM_ENGINES

    for spelling in ["onset", "Onset", "ONSET", "OnsetEngine"]:
        resolved = resolve_engine_name(spelling)
        assert resolved == "Onset", (
            f"'{spelling}' resolved to '{resolved}', expected 'Onset'"
        )
        assert resolved in DRUM_ENGINES, (
            f"Canonical '{resolved}' not in DRUM_ENGINES"
        )


def test_apply_gain_identity():
    """Applying a gain near 0 dB should not significantly change the audio.

    _apply_gain_db skips when abs(gain_db) < 0.01, so we use 0.02 dB
    (just above threshold) and verify RMS changes by < 0.1 dBFS.
    """
    from oxport import _apply_gain_db, _compute_rms_db, _write_wav
    from pathlib import Path

    sr = 44100
    samples = [int(16000 * math.sin(2 * math.pi * 440 * i / sr)) for i in range(sr)]
    raw = struct.pack(f"<{len(samples)}h", *samples)
    with tempfile.NamedTemporaryFile(suffix=".wav", delete=False) as f:
        path = f.name
    try:
        _write_wav(Path(path), 1, sr, 16, raw)
        original_rms = _compute_rms_db(Path(path))
        _apply_gain_db(Path(path), 0.02)  # 0.02 dB — near-identity gain
        modified_rms = _compute_rms_db(Path(path))
        # 0.02 dB is below audible threshold; RMS shift should be < 0.1 dBFS
        assert abs(modified_rms - original_rms) < 0.1, (
            f"Near-zero gain changed RMS by {abs(modified_rms - original_rms):.3f} dBFS "
            f"(expected < 0.1)"
        )
    finally:
        os.unlink(path)


def test_normalize_target_reached():
    """After normalization, RMS should be close to target."""
    from oxport import _compute_rms_db, _apply_gain_db, _write_wav
    from pathlib import Path

    sr = 44100
    # Generate a quiet sine (~-20 dBFS)
    amplitude = 0.1  # ~-20 dBFS
    samples = [int(amplitude * 32767 * math.sin(2 * math.pi * 440 * i / sr))
               for i in range(sr)]
    raw = struct.pack(f"<{len(samples)}h", *samples)
    with tempfile.NamedTemporaryFile(suffix=".wav", delete=False) as f:
        path = f.name
    try:
        _write_wav(Path(path), 1, sr, 16, raw)
        current_rms = _compute_rms_db(Path(path))
        target = -14.0
        gain = target - current_rms
        _apply_gain_db(Path(path), gain)
        new_rms = _compute_rms_db(Path(path))
        assert abs(new_rms - target) < 1.0, (
            f"Expected ~{target} dBFS after normalization, got {new_rms:.2f}"
        )
    finally:
        os.unlink(path)


if __name__ == "__main__":
    tests = [
        test_compute_rms_db_sine,
        test_wav_roundtrip_24bit,
        test_engine_id_resolution,
        test_apply_gain_identity,
        test_normalize_target_reached,
    ]
    passed = 0
    failed = 0
    for t in tests:
        try:
            t()
            print(f"  PASS  {t.__name__}")
            passed += 1
        except Exception as e:
            print(f"  FAIL  {t.__name__}: {e}")
            failed += 1
    print(f"\n{passed} passed, {failed} failed")
    sys.exit(0 if failed == 0 else 1)
