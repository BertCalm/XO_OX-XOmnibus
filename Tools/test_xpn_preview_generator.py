"""Tests for xpn_preview_generator — MP3 preview pipeline stage.

Covers:
  1. ffmpeg invocation correctness — mock subprocess, check exact args
  2. validate_mp3() passes on real MP3, fails on WAV-with-.mp3-extension
  3. Pack-assembly gate fails if any .xpm lacks a matching .mp3

Decision D-U6=a: Python CLI post-processor invoking system ffmpeg.
Bible §11: every .xpm must have a matching 192kbps CBR .mp3.
"""
import array
import os
import struct
import sys
import tempfile
import wave
from pathlib import Path
from unittest.mock import MagicMock, call, patch

import pytest

sys.path.insert(0, os.path.dirname(__file__))


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def _write_minimal_wav(path: Path, duration_s: float = 0.1,
                       sr: int = 44100, n_channels: int = 1,
                       bit_depth: int = 16) -> None:
    """Write a minimal valid WAV file containing a sine tone."""
    n_frames = int(duration_s * sr)
    with wave.open(str(path), "wb") as wf:
        wf.setnchannels(n_channels)
        wf.setsampwidth(bit_depth // 8)
        wf.setframerate(sr)
        buf = array.array("h", [0] * (n_frames * n_channels))
        import math
        for i in range(n_frames):
            val = int(32767 * 0.5 * math.sin(2 * math.pi * 440 * i / sr))
            for ch in range(n_channels):
                buf[i * n_channels + ch] = val
        wf.writeframes(buf.tobytes())


def _write_fake_mp3_extension(path: Path) -> None:
    """Write a WAV file with an .mp3 extension (the broken C++ path symptom)."""
    wav_path = path.with_suffix(".wav_tmp")
    _write_minimal_wav(wav_path)
    wav_path.rename(path)


# ---------------------------------------------------------------------------
# Test 1: ffmpeg invocation correctness
# ---------------------------------------------------------------------------

class TestEncodeMP3Invocation:
    """encode_mp3() must call ffmpeg with the exact 192kbps CBR command form
    specified in bible §11:
        ffmpeg -y -i input.wav -codec:a libmp3lame -b:a 192k output.mp3
    """

    def test_ffmpeg_called_with_correct_args(self, tmp_path):
        """encode_mp3() invokes ffmpeg with -codec:a libmp3lame -b:a 192k."""
        from xpn_preview_generator import encode_mp3

        wav_path = tmp_path / "preview.wav"
        mp3_path = tmp_path / "preview.mp3"
        _write_minimal_wav(wav_path)

        mock_result = MagicMock()
        mock_result.returncode = 0
        mock_result.stderr = ""

        with patch("subprocess.run", return_value=mock_result) as mock_run:
            encode_mp3(wav_path, mp3_path, bitrate=192)

        mock_run.assert_called_once()
        args_used = mock_run.call_args[0][0]

        assert args_used[0] == "ffmpeg", "First arg must be 'ffmpeg'"
        assert "-y" in args_used, "Must pass -y to overwrite without prompt"
        assert "-i" in args_used, "Must pass -i flag"
        assert str(wav_path) in args_used, "Must include WAV input path"
        assert "-codec:a" in args_used, "Must specify -codec:a (not deprecated -acodec)"
        assert "libmp3lame" in args_used, "Must specify libmp3lame encoder"
        assert "-b:a" in args_used, "Must pass -b:a bitrate flag"
        assert "192k" in args_used, "Must pass 192k bitrate (bible §11)"
        assert str(mp3_path) in args_used, "Must include MP3 output path"

    def test_ffmpeg_non_zero_exit_raises_runtime_error(self, tmp_path):
        """encode_mp3() must raise RuntimeError when ffmpeg returns non-zero,
        surfacing stderr so the user can diagnose the failure."""
        from xpn_preview_generator import encode_mp3

        wav_path = tmp_path / "preview.wav"
        mp3_path = tmp_path / "preview.mp3"
        _write_minimal_wav(wav_path)

        mock_result = MagicMock()
        mock_result.returncode = 1
        mock_result.stderr = "ffmpeg: No such encoder 'libmp3lame'"

        with patch("subprocess.run", return_value=mock_result):
            with pytest.raises(RuntimeError) as exc_info:
                encode_mp3(wav_path, mp3_path)

        err = str(exc_info.value)
        assert "exit code 1" in err, "Error must include the exit code"
        assert "libmp3lame" in err, "Error must echo ffmpeg stderr"

    def test_bitrate_allowlist_rejects_invalid(self, tmp_path):
        """encode_mp3() must reject bitrates not in the standard MP3 allowlist."""
        from xpn_preview_generator import encode_mp3

        wav_path = tmp_path / "preview.wav"
        mp3_path = tmp_path / "preview.mp3"
        _write_minimal_wav(wav_path)

        with pytest.raises(ValueError, match="Invalid MP3 bitrate"):
            encode_mp3(wav_path, mp3_path, bitrate=999)

    def test_ffmpeg_timeout_raises_runtime_error(self, tmp_path):
        """encode_mp3() must raise RuntimeError (not TimeoutExpired) on timeout."""
        import subprocess
        from xpn_preview_generator import encode_mp3

        wav_path = tmp_path / "preview.wav"
        mp3_path = tmp_path / "preview.mp3"
        _write_minimal_wav(wav_path)

        with patch("subprocess.run", side_effect=subprocess.TimeoutExpired("ffmpeg", 60)):
            with pytest.raises(RuntimeError, match="timed out"):
                encode_mp3(wav_path, mp3_path)

    def test_ffmpeg_not_found_raises_runtime_error(self, tmp_path):
        """encode_mp3() must raise RuntimeError with install hint when ffmpeg is absent."""
        from xpn_preview_generator import encode_mp3

        wav_path = tmp_path / "preview.wav"
        mp3_path = tmp_path / "preview.mp3"
        _write_minimal_wav(wav_path)

        with patch("subprocess.run", side_effect=FileNotFoundError):
            with pytest.raises(RuntimeError, match="not found on PATH"):
                encode_mp3(wav_path, mp3_path)


# ---------------------------------------------------------------------------
# Test 2: validate_mp3() — real MP3 passes, WAV-with-mp3-extension fails
# ---------------------------------------------------------------------------

class TestValidateMP3:
    """validate_mp3() must distinguish real MP3 files from WAV imposters
    by running ffprobe and checking the reported codec_name."""

    def test_returns_true_for_real_mp3(self, tmp_path):
        """ffprobe reports 'mp3' → validate_mp3() returns True."""
        from xpn_preview_generator import validate_mp3

        mp3_path = tmp_path / "real.mp3"
        mp3_path.touch()

        mock_result = MagicMock()
        mock_result.returncode = 0
        mock_result.stdout = "mp3\n"

        with patch("subprocess.run", return_value=mock_result) as mock_run:
            result = validate_mp3(mp3_path)

        assert result is True

        # Verify the exact ffprobe command form
        args = mock_run.call_args[0][0]
        assert args[0] == "ffprobe"
        assert "-select_streams" in args
        assert "a:0" in args
        assert "-show_entries" in args
        assert "stream=codec_name" in args
        assert "-of" in args
        assert str(mp3_path) in args

    def test_returns_false_for_wav_with_mp3_extension(self, tmp_path):
        """ffprobe reports 'pcm_s16le' → validate_mp3() returns False.

        This is the exact symptom of the broken C++ exporter described in
        issue #1188: WAV bytes written with .mp3 extension.
        """
        from xpn_preview_generator import validate_mp3

        fake_mp3 = tmp_path / "broken.mp3"
        _write_fake_mp3_extension(fake_mp3)

        # Simulate ffprobe detecting the actual WAV codec
        mock_result = MagicMock()
        mock_result.returncode = 0
        mock_result.stdout = "pcm_s16le\n"

        with patch("subprocess.run", return_value=mock_result):
            result = validate_mp3(fake_mp3)

        assert result is False, (
            "validate_mp3() must return False when ffprobe reports a non-MP3 codec. "
            "WAV-with-.mp3-extension (the broken C++ exporter symptom) must be rejected."
        )

    def test_returns_false_for_missing_file(self, tmp_path):
        """validate_mp3() returns False (not an exception) for a missing file."""
        from xpn_preview_generator import validate_mp3

        absent = tmp_path / "absent.mp3"
        result = validate_mp3(absent)
        assert result is False

    def test_returns_false_on_ffprobe_timeout(self, tmp_path):
        """validate_mp3() returns False (not an exception) on ffprobe timeout."""
        import subprocess
        from xpn_preview_generator import validate_mp3

        mp3_path = tmp_path / "timeout.mp3"
        mp3_path.touch()

        with patch("subprocess.run",
                   side_effect=subprocess.TimeoutExpired("ffprobe", 15)):
            result = validate_mp3(mp3_path)

        assert result is False

    def test_codec_name_case_insensitive(self, tmp_path):
        """validate_mp3() should accept 'MP3' or 'mp3' from ffprobe output."""
        from xpn_preview_generator import validate_mp3

        mp3_path = tmp_path / "real.mp3"
        mp3_path.touch()

        for codec_output in ("mp3", "MP3", "mp3\n", " mp3 \n"):
            mock_result = MagicMock()
            mock_result.returncode = 0
            mock_result.stdout = codec_output

            with patch("subprocess.run", return_value=mock_result):
                assert validate_mp3(mp3_path) is True, (
                    f"validate_mp3() should return True for codec output {codec_output!r}"
                )


# ---------------------------------------------------------------------------
# Test 3: Pack assembly gate — fails if any .xpm lacks matching .mp3
# ---------------------------------------------------------------------------

class TestPackageStageMP3Gate:
    """_stage_package() must abort with RuntimeError when any .xpm in the
    bundle is missing a matching .mp3 or has an invalid one.

    This test exercises the gate directly by constructing a minimal
    PipelineContext and calling _stage_package() with mocked internals.
    """

    def _make_ctx(self, tmp_path: Path, engine: str = "ONSET") -> object:
        """Construct a minimal PipelineContext-like namespace for testing."""
        from oxport import PipelineContext

        ctx = PipelineContext(
            engine=engine,
            output_dir=tmp_path,
        )
        ctx.dry_run = False
        ctx.programs_dir = tmp_path / "Programs"
        ctx.programs_dir.mkdir(parents=True, exist_ok=True)
        ctx.build_dir = tmp_path / engine
        ctx.build_dir.mkdir(parents=True, exist_ok=True)
        ctx.pack_name = "Test Pack"
        ctx.version = "1.0"
        ctx.engine = engine
        ctx.tuning = None
        return ctx

    def _write_dummy_xpm(self, programs_dir: Path, name: str) -> Path:
        """Write a minimal .xpm stub file."""
        xpm = programs_dir / f"{name}.xpm"
        xpm.write_text(
            '<?xml version="1.0" ?><Program Type="Drum" name="{name}"/>'.format(
                name=name
            ),
            encoding="utf-8",
        )
        return xpm

    def test_package_aborts_when_mp3_missing(self, tmp_path):
        """_stage_package() raises RuntimeError when a .xpm has no matching .mp3."""
        ctx = self._make_ctx(tmp_path)

        # Create an XPM but no matching MP3
        xpm_path = self._write_dummy_xpm(ctx.programs_dir, "KickKit")
        ctx.xpm_paths = [xpm_path]

        with pytest.raises(RuntimeError) as exc_info:
            # Patch validate_mp3 to ensure it's not the cause of failure
            # (the file doesn't exist so the exists() check should catch it first)
            from oxport import _stage_package
            _stage_package(ctx)

        err = str(exc_info.value)
        assert "KickKit" in err, "Error must name the offending program"
        assert "Missing" in err or "missing" in err.lower(), (
            "Error must indicate that the .mp3 is missing"
        )
        assert "bible §11" in err or "xpm" in err.lower(), (
            "Error must reference the spec requirement"
        )

    def test_package_aborts_when_mp3_is_wav_imposter(self, tmp_path):
        """_stage_package() raises RuntimeError when .mp3 exists but fails ffprobe validation."""
        ctx = self._make_ctx(tmp_path)

        # Create an XPM with a WAV-disguised-as-MP3 (the broken C++ symptom)
        xpm_path = self._write_dummy_xpm(ctx.programs_dir, "SnareKit")
        fake_mp3 = ctx.programs_dir / "SnareKit.mp3"
        _write_fake_mp3_extension(fake_mp3)
        ctx.xpm_paths = [xpm_path]

        # validate_mp3() will call ffprobe; mock it to return WAV codec
        mock_result = MagicMock()
        mock_result.returncode = 0
        mock_result.stdout = "pcm_s16le\n"

        with patch("subprocess.run", return_value=mock_result):
            from oxport import _stage_package
            with pytest.raises(RuntimeError) as exc_info:
                _stage_package(ctx)

        err = str(exc_info.value)
        assert "SnareKit" in err, "Error must name the offending program"
        assert "invalid" in err.lower() or "Invalid" in err, (
            "Error must indicate the .mp3 file is invalid"
        )

    def test_package_succeeds_when_all_xpms_have_valid_mp3(self, tmp_path):
        """_stage_package() does not raise when all XPMs have valid .mp3 files."""
        ctx = self._make_ctx(tmp_path)

        for name in ("KickKit", "SnareKit"):
            xpm_path = self._write_dummy_xpm(ctx.programs_dir, name)

        ctx.xpm_paths = list(ctx.programs_dir.glob("*.xpm"))

        # Mock validate_mp3 to return True for all files
        # Mock package_xpn to avoid actual packaging work
        mock_ffprobe = MagicMock()
        mock_ffprobe.returncode = 0
        mock_ffprobe.stdout = "mp3\n"

        # Create real .mp3 stubs so exists() passes
        for xpm in ctx.xpm_paths:
            (ctx.programs_dir / xpm.with_suffix(".mp3").name).write_bytes(b"\xff\xfb")

        mock_pkg = MagicMock(return_value=tmp_path / "test.xpn")

        with patch("subprocess.run", return_value=mock_ffprobe), \
             patch("xpn_packager.package_xpn", mock_pkg), \
             patch("xpn_packager.XPNMetadata", MagicMock()):
            from oxport import _stage_package
            # Should not raise
            _stage_package(ctx)

    def test_package_gate_runs_even_when_preview_stage_skipped(self, tmp_path):
        """The MP3 gate in _stage_package() must run even if the preview stage
        was skipped (--skip preview), catching the case where a previous build
        had no previews.
        """
        ctx = self._make_ctx(tmp_path)

        # XPM with no MP3 — simulates skipping preview stage
        xpm_path = self._write_dummy_xpm(ctx.programs_dir, "HatKit")
        ctx.xpm_paths = [xpm_path]
        # No .mp3 file created

        from oxport import _stage_package
        with pytest.raises(RuntimeError) as exc_info:
            _stage_package(ctx)

        err = str(exc_info.value)
        assert "HatKit" in err


# ---------------------------------------------------------------------------
# Test 4: preflight_check() — loud failure when tools are missing
# ---------------------------------------------------------------------------

class TestPreflightCheck:
    """preflight_check() must raise RuntimeError with a clear install hint
    when ffmpeg or ffprobe is missing or MP3 support is absent."""

    def test_raises_when_ffmpeg_missing(self):
        """preflight_check() raises RuntimeError when ffmpeg is not on PATH."""
        from xpn_preview_generator import preflight_check

        # Simulate ffmpeg returning non-zero (not found)
        mock_result = MagicMock()
        mock_result.returncode = 127  # command not found

        with patch("subprocess.run", return_value=mock_result):
            with pytest.raises(RuntimeError) as exc_info:
                preflight_check()

        err = str(exc_info.value)
        assert "ffmpeg" in err.lower()
        assert "brew install ffmpeg" in err or "apt install ffmpeg" in err, (
            "Error must include an install hint"
        )

    def test_passes_when_both_tools_available(self):
        """preflight_check() does not raise when ffmpeg + ffprobe + mp3 are available."""
        from xpn_preview_generator import preflight_check

        mock_result = MagicMock()
        mock_result.returncode = 0
        mock_result.stdout = "DEA libmp3lame  MP3 (MPEG audio layer 3)"

        with patch("subprocess.run", return_value=mock_result):
            preflight_check()  # must not raise


# ---------------------------------------------------------------------------
# Test 5: generate_preview_for_xpm() — integration contract
# ---------------------------------------------------------------------------

class TestGeneratePreviewForXPM:
    """generate_preview_for_xpm() pipeline entry point contract tests."""

    def test_raises_when_wavs_dir_missing(self, tmp_path):
        """generate_preview_for_xpm() raises FileNotFoundError for absent wavs_dir."""
        from xpn_preview_generator import generate_preview_for_xpm

        xpm = tmp_path / "Kit.xpm"
        xpm.write_text('<Program/>', encoding="utf-8")
        absent_dir = tmp_path / "does_not_exist"

        with pytest.raises(FileNotFoundError, match="WAV source directory not found"):
            generate_preview_for_xpm(
                xpm_path=xpm,
                wavs_dir=absent_dir,
                engine="ONSET",
            )

    def test_raises_when_generate_preview_returns_none(self, tmp_path):
        """generate_preview_for_xpm() raises RuntimeError when no samples are found."""
        from xpn_preview_generator import generate_preview_for_xpm

        xpm = tmp_path / "Kit.xpm"
        xpm.write_text('<Program/>', encoding="utf-8")
        empty_dir = tmp_path / "wavs"
        empty_dir.mkdir()
        # No WAV files in empty_dir → generate_preview returns None

        with pytest.raises(RuntimeError, match="no output"):
            generate_preview_for_xpm(
                xpm_path=xpm,
                wavs_dir=empty_dir,
                engine="ONSET",
            )

    def test_mp3_placed_adjacent_to_xpm(self, tmp_path):
        """generate_preview_for_xpm() writes the .mp3 in the same directory as the .xpm."""
        from xpn_preview_generator import generate_preview_for_xpm

        programs_dir = tmp_path / "Programs"
        programs_dir.mkdir()
        wavs_dir = tmp_path / "Samples"
        wavs_dir.mkdir()

        # Write a real WAV so the generator has something to sequence
        _write_minimal_wav(wavs_dir / "kick_v1.wav", duration_s=0.5)

        xpm = programs_dir / "MyKit.xpm"
        xpm.write_text('<Program/>', encoding="utf-8")

        # Mock ffmpeg encoding (writes a real file) and ffprobe validation
        def _fake_ffmpeg(cmd, **kwargs):
            m = MagicMock()
            m.returncode = 0
            m.stderr = ""
            m.stdout = ""
            # If this looks like an ffmpeg encode call, create the output file
            if "ffmpeg" in cmd[0] and "-codec:a" in cmd:
                out = Path(cmd[-1])
                out.write_bytes(b"\xff\xfb\x90\x00" * 64)  # fake mp3 header
            elif "ffprobe" in cmd[0]:
                m.stdout = "mp3\n"
            return m

        with patch("subprocess.run", side_effect=_fake_ffmpeg):
            result = generate_preview_for_xpm(
                xpm_path=xpm,
                wavs_dir=wavs_dir,
                engine="ONSET",
            )

        assert result.parent == programs_dir, (
            "MP3 must be placed adjacent to the .xpm file"
        )
        assert result.stem == "MyKit", (
            "MP3 must share the same base name as the .xpm"
        )
        assert result.suffix == ".mp3", "Output must have .mp3 extension"
