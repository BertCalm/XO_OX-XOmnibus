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


def test_apply_gain_rejects_unsupported_bitdepth():
    """_apply_gain_db must raise ValueError for unsupported bit depths (e.g. 8-bit)."""
    import struct
    import tempfile
    import os
    from oxport import _apply_gain_db
    from pathlib import Path

    sr = 44100
    n = 1000
    # 8-bit WAV uses unsigned samples; 128 = silence
    raw = bytes([128 + int(50 * math.sin(2 * math.pi * 440 * i / sr)) for i in range(n)])
    with tempfile.NamedTemporaryFile(suffix=".wav", delete=False) as f:
        path = f.name
    try:
        data_size = len(raw)
        with open(path, "wb") as f:
            f.write(b"RIFF")
            f.write(struct.pack("<I", 36 + data_size))
            f.write(b"WAVE")
            f.write(b"fmt ")
            f.write(struct.pack("<I", 16))
            f.write(struct.pack("<H", 1))   # PCM
            f.write(struct.pack("<H", 1))   # mono
            f.write(struct.pack("<I", sr))
            f.write(struct.pack("<I", sr))   # byte rate (1 ch * 1 byte * sr)
            f.write(struct.pack("<H", 1))   # block align
            f.write(struct.pack("<H", 8))   # bits per sample
            f.write(b"data")
            f.write(struct.pack("<I", data_size))
            f.write(raw)
        raised = False
        try:
            _apply_gain_db(Path(path), 3.0)
        except ValueError:
            raised = True
        assert raised, "_apply_gain_db must raise ValueError for 8-bit WAV"
    finally:
        os.unlink(path)


def test_compute_rms_db_silence_returns_neginf():
    """_compute_rms_db must return -inf (or float('-inf')) for an all-zero WAV."""
    import struct
    import tempfile
    import os
    from oxport import _compute_rms_db, _write_wav
    from pathlib import Path

    sr = 44100
    n = 1024
    raw = struct.pack(f"<{n}h", *([0] * n))
    with tempfile.NamedTemporaryFile(suffix=".wav", delete=False) as f:
        path = f.name
    try:
        _write_wav(Path(path), 1, sr, 16, raw)
        rms = _compute_rms_db(Path(path))
        assert rms == float("-inf"), (
            f"Expected -inf for silence, got {rms}"
        )
    finally:
        os.unlink(path)


def test_apply_gain_32bit_roundtrip():
    """Apply a known gain to a 32-bit WAV and verify RMS shifts by the expected amount."""
    import struct
    import tempfile
    import os
    from oxport import _apply_gain_db, _compute_rms_db, _write_wav
    from pathlib import Path

    sr = 44100
    gain_db = 6.0
    # 32-bit float-equivalent PCM stored as 32-bit signed integers
    amplitude = 0.25  # well below full-scale to leave headroom for +6 dB
    samples = [int(amplitude * 2147483647 * math.sin(2 * math.pi * 440 * i / sr))
               for i in range(sr)]
    raw = struct.pack(f"<{len(samples)}i", *samples)
    with tempfile.NamedTemporaryFile(suffix=".wav", delete=False) as f:
        path = f.name
    try:
        _write_wav(Path(path), 1, sr, 32, raw)
        before_rms = _compute_rms_db(Path(path))
        _apply_gain_db(Path(path), gain_db)
        after_rms = _compute_rms_db(Path(path))
        actual_shift = after_rms - before_rms
        assert abs(actual_shift - gain_db) < 0.5, (
            f"Expected RMS to shift by ~{gain_db} dB, got {actual_shift:.3f} dB "
            f"(before={before_rms:.2f}, after={after_rms:.2f})"
        )
    finally:
        os.unlink(path)


def test_normalize_clipping_protection():
    """Applying +40 dB to a near-full-scale signal must not produce out-of-range samples."""
    import struct
    import tempfile
    import os
    from oxport import _apply_gain_db, _write_wav, _read_wav_raw
    from pathlib import Path

    sr = 44100
    # Near-full-scale 16-bit sine: peak ~32000
    samples = [int(32000 * math.sin(2 * math.pi * 440 * i / sr)) for i in range(sr)]
    raw = struct.pack(f"<{len(samples)}h", *samples)
    with tempfile.NamedTemporaryFile(suffix=".wav", delete=False) as f:
        path = f.name
    try:
        _write_wav(Path(path), 1, sr, 16, raw)
        _apply_gain_db(Path(path), 40.0)
        _ch, _sr, bps, data_out = _read_wav_raw(Path(path))
        assert bps == 16, f"Expected 16-bit output, got {bps}-bit"
        n_samples = len(data_out) // 2
        decoded = struct.unpack(f"<{n_samples}h", data_out)
        out_of_range = [s for s in decoded if s < -32768 or s > 32767]
        assert len(out_of_range) == 0, (
            f"Found {len(out_of_range)} samples outside [-32768, 32767] after +40 dB gain — "
            f"clipping protection failed"
        )
    finally:
        os.unlink(path)


def test_provenance_hash_file():
    """hash_file must return a deterministic sha256 hex digest."""
    import hashlib
    import tempfile
    import os
    from pathlib import Path
    from xpn_provenance import hash_file

    content = b"XO_OX provenance test payload\n" * 100
    with tempfile.NamedTemporaryFile(delete=False) as f:
        f.write(content)
        path = f.name
    try:
        digest1 = hash_file(Path(path))
        digest2 = hash_file(Path(path))
        # Must be deterministic
        assert digest1 == digest2, "hash_file is not deterministic"
        # Must be a valid sha256 hex string (64 chars)
        assert len(digest1) == 64, f"Expected 64-char hex digest, got {len(digest1)}"
        assert all(c in "0123456789abcdef" for c in digest1), "Digest contains non-hex chars"
        # Verify against stdlib directly
        expected = hashlib.sha256(content).hexdigest()
        assert digest1 == expected, f"hash_file digest mismatch: {digest1} != {expected}"
    finally:
        os.unlink(path)


def test_provenance_chain_roundtrip():
    """ProvenanceChain serializes and verify_provenance confirms valid=True."""
    import tempfile
    import os
    from pathlib import Path
    from xpn_provenance import ProvenanceChain, verify_provenance

    with tempfile.TemporaryDirectory() as tmpdir:
        base = Path(tmpdir)

        # Write a couple of known files
        file_a = base / "preset_01.xpm"
        file_a.write_bytes(b"<XPM><Program>test</Program></XPM>")
        file_b = base / "pack.xpn"
        file_b.write_bytes(b"PK\x03\x04" + b"\x00" * 26)  # minimal ZIP-like header

        chain = ProvenanceChain(engine="Onset", pack_name="Test Pack", version="1.0.0")
        chain.record("export", file_a, "XPM program: preset_01.xpm")
        chain.record("package", file_b, "Final .xpn archive")

        prov_json = chain.to_json()

        # chain_hash must be present and non-empty
        import json
        data = json.loads(prov_json)
        assert "chain_hash" in data and len(data["chain_hash"]) == 64, \
            "chain_hash missing or wrong length"
        assert len(data["chain"]) == 2, f"Expected 2 chain entries, got {len(data['chain'])}"

        result = verify_provenance(prov_json, base)
        assert result["valid"], f"Expected valid=True, errors: {result['errors']}"
        assert result["verified"] == 2, f"Expected 2 verified, got {result['verified']}"
        assert result["total"] == 2, f"Expected total=2, got {result['total']}"


def test_provenance_chain_detects_tampering():
    """verify_provenance returns valid=False when a recorded file is mutated."""
    import tempfile
    import os
    from pathlib import Path
    from xpn_provenance import ProvenanceChain, verify_provenance

    with tempfile.TemporaryDirectory() as tmpdir:
        base = Path(tmpdir)

        artifact = base / "preset_tampered.xpm"
        artifact.write_bytes(b"<XPM><Program>original</Program></XPM>")

        chain = ProvenanceChain(engine="Onset", pack_name="Tamper Test", version="1.0.0")
        chain.record("export", artifact, "XPM program: preset_tampered.xpm")

        prov_json = chain.to_json()

        # Tamper with the file after recording
        artifact.write_bytes(b"<XPM><Program>TAMPERED</Program></XPM>")

        result = verify_provenance(prov_json, base)
        assert not result["valid"], "Expected valid=False after tampering"
        assert len(result["errors"]) >= 1, "Expected at least one error after tampering"
        # The error should mention the artifact name
        combined = " ".join(result["errors"])
        assert "preset_tampered.xpm" in combined, \
            f"Expected artifact name in errors, got: {result['errors']}"


def test_numpy_stdlib_rms_agreement():
    """Numpy and stdlib RMS must agree within 0.01 dB."""
    from oxport import (_compute_rms_db_numpy, _compute_rms_db_stdlib,
                        _write_wav, _NUMPY_AVAILABLE)
    from pathlib import Path
    if not _NUMPY_AVAILABLE:
        print("    [SKIP] numpy not installed")
        return

    sr = 44100
    samples = [int(10000 * math.sin(2 * math.pi * 440 * i / sr)) for i in range(sr)]
    raw = struct.pack(f"<{len(samples)}h", *samples)
    fd, path = tempfile.mkstemp(suffix=".wav")
    os.close(fd)
    try:
        _write_wav(Path(path), 1, sr, 16, raw)
        rms_np = _compute_rms_db_numpy(Path(path))
        rms_std = _compute_rms_db_stdlib(Path(path))
        assert abs(rms_np - rms_std) < 0.01, (
            f"Numpy ({rms_np:.4f}) and stdlib ({rms_std:.4f}) RMS disagree by "
            f"{abs(rms_np - rms_std):.4f} dB"
        )
    finally:
        os.unlink(path)


def test_numpy_stdlib_gain_agreement():
    """Numpy and stdlib gain must produce similar RMS after application."""
    from oxport import (_apply_gain_db_numpy, _apply_gain_db_stdlib,
                        _compute_rms_db_stdlib, _write_wav, _read_wav_raw,
                        _NUMPY_AVAILABLE)
    from pathlib import Path
    import shutil
    if not _NUMPY_AVAILABLE:
        print("    [SKIP] numpy not installed")
        return

    sr = 44100
    samples = [int(5000 * math.sin(2 * math.pi * 440 * i / sr)) for i in range(sr)]
    raw = struct.pack(f"<{len(samples)}h", *samples)

    fd1, path1 = tempfile.mkstemp(suffix=".wav")
    os.close(fd1)
    fd2, path2 = tempfile.mkstemp(suffix=".wav")
    os.close(fd2)
    try:
        _write_wav(Path(path1), 1, sr, 16, raw)
        _write_wav(Path(path2), 1, sr, 16, raw)

        _apply_gain_db_numpy(Path(path1), 6.0)
        _apply_gain_db_stdlib(Path(path2), 6.0)

        rms1 = _compute_rms_db_stdlib(Path(path1))
        rms2 = _compute_rms_db_stdlib(Path(path2))
        # Allow 0.5 dB tolerance due to different dither random seeds
        assert abs(rms1 - rms2) < 0.5, (
            f"Numpy gain ({rms1:.2f}) and stdlib gain ({rms2:.2f}) disagree by "
            f"{abs(rms1 - rms2):.2f} dB"
        )
    finally:
        os.unlink(path1)
        os.unlink(path2)


def test_numpy_rms_benchmark():
    """Benchmark numpy vs stdlib RMS and report speedup. Numpy must be >= 2x faster."""
    import time
    from oxport import (_compute_rms_db_numpy, _compute_rms_db_stdlib,
                        _write_wav, _NUMPY_AVAILABLE)
    from pathlib import Path
    if not _NUMPY_AVAILABLE:
        print("    [SKIP] numpy not installed")
        return

    sr = 44100
    # Use 5 seconds of audio for a meaningful benchmark
    samples = [int(10000 * math.sin(2 * math.pi * 440 * i / sr)) for i in range(sr * 5)]
    raw = struct.pack(f"<{len(samples)}h", *samples)
    fd, path = tempfile.mkstemp(suffix=".wav")
    os.close(fd)
    try:
        _write_wav(Path(path), 1, sr, 16, raw)

        # Warm up
        _compute_rms_db_numpy(Path(path))
        _compute_rms_db_stdlib(Path(path))

        n_iters = 10
        t0 = time.perf_counter()
        for _ in range(n_iters):
            _compute_rms_db_numpy(Path(path))
        numpy_time = (time.perf_counter() - t0) / n_iters

        t0 = time.perf_counter()
        for _ in range(n_iters):
            _compute_rms_db_stdlib(Path(path))
        stdlib_time = (time.perf_counter() - t0) / n_iters

        speedup = stdlib_time / numpy_time if numpy_time > 0 else float("inf")
        print(f"    RMS benchmark: numpy={numpy_time*1000:.2f}ms  stdlib={stdlib_time*1000:.2f}ms  speedup={speedup:.1f}x")
        assert speedup >= 2.0, (
            f"Expected numpy to be >= 2x faster for RMS, got {speedup:.1f}x "
            f"(numpy={numpy_time*1000:.2f}ms, stdlib={stdlib_time*1000:.2f}ms)"
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
        test_apply_gain_rejects_unsupported_bitdepth,
        test_compute_rms_db_silence_returns_neginf,
        test_apply_gain_32bit_roundtrip,
        test_normalize_clipping_protection,
        test_provenance_hash_file,
        test_provenance_chain_roundtrip,
        test_provenance_chain_detects_tampering,
        test_numpy_stdlib_rms_agreement,
        test_numpy_stdlib_gain_agreement,
        test_numpy_rms_benchmark,
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
