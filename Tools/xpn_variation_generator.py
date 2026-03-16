#!/usr/bin/env python3
"""
XPN Variation Generator — XO_OX Designs
Given one XPN pack, generates N tonal/spatial/character variations without
requiring new renders.  All processing is pure Python + scipy/numpy.

4 variation types:

  tone      Apply gain + spectral shelving EQ to all WAV samples.
            feliX direction: brighten (+2dB above 8kHz, -2dB below 200Hz).
            Oscar direction: warm (+3dB below 200Hz, -2dB above 8kHz).
            Uses scipy IIR biquad shelving filters.

  pitch     Re-pitch all samples via resampling.
            Options: fifth_up (+7 semitones), detuned (-12 cents), octave (+1200 cents).
            NOTE: pitch variant uses resampling — introduces subtle aliasing
            that IS the variation (analog-tape pitch-shifted texture).
            Implementation: resample to shift pitch, then resample back to
            original length (time-stretching approximation via scipy).

  time      Change overall tempo feel via sample start trimming / padding.
            tight: trim first 5ms of attack (more staccato, forward transient feel).
            loose: add 10ms of silence at start (behind-the-beat, laid-back feel).
            Uses numpy array concatenation; sample rate read from WAV header.

  dna       Select different presets matching a different DNA target.
            Reads source XPN manifest to get engine name + preset DNA.
            Queries local .xometa preset library for 5 presets with similar
            engine but different DNA profile.
            Outputs dna_variant_suggestions.json — no audio processing.

Output naming:
    {original_name}_felix_tone.xpn
    {original_name}_oscar_tone.xpn
    {original_name}_fifth_up.xpn
    {original_name}_detuned.xpn
    {original_name}_tight.xpn
    {original_name}_loose.xpn

Usage:
    python xpn_variation_generator.py my_pack.xpn --type tone --direction felix --output ./variations/
    python xpn_variation_generator.py my_pack.xpn --type all --output ./variations/
    python xpn_variation_generator.py my_pack.xpn --type dna --presets ./Presets/XOmnibus/ --output ./variations/
"""

import argparse
import io
import json
import math
import os
import shutil
import struct
import sys
import tempfile
import zipfile
from pathlib import Path
from typing import Dict, List, Optional, Tuple


# ---------------------------------------------------------------------------
# Dependency check — scipy / numpy are optional for dna-only mode
# ---------------------------------------------------------------------------

def _import_numpy():
    try:
        import numpy as np
        return np
    except ImportError:
        print(
            "ERROR: numpy is required for audio processing variants.\n"
            "Install: pip install numpy",
            file=sys.stderr,
        )
        sys.exit(1)


def _import_scipy():
    try:
        from scipy import signal as sp_signal
        return sp_signal
    except ImportError:
        print(
            "ERROR: scipy is required for tone/pitch variants.\n"
            "Install: pip install scipy",
            file=sys.stderr,
        )
        sys.exit(1)


# ---------------------------------------------------------------------------
# WAV read / write (pure stdlib — avoids soundfile/librosa dependency)
# ---------------------------------------------------------------------------

WAV_FMT_PCM   = 1
WAV_FMT_FLOAT = 3


def read_wav(data: bytes) -> Tuple["np.ndarray", int, int]:
    """
    Read WAV bytes.  Returns (samples_float32, sample_rate, n_channels).
    Supports PCM-16, PCM-24, PCM-32, IEEE float-32.
    samples shape: (n_frames, n_channels) — always float32 in [-1, +1].
    """
    np = _import_numpy()
    buf = io.BytesIO(data)

    riff_id = buf.read(4)
    if riff_id != b"RIFF":
        raise ValueError("Not a WAV file (missing RIFF header).")
    file_size = struct.unpack("<I", buf.read(4))[0]
    wave_id = buf.read(4)
    if wave_id != b"WAVE":
        raise ValueError("Not a WAV file (missing WAVE marker).")

    fmt_fmt = 0
    channels = 1
    sample_rate = 44100
    bits = 16
    data_bytes = b""

    while True:
        chunk_id = buf.read(4)
        if len(chunk_id) < 4:
            break
        chunk_size_raw = buf.read(4)
        if len(chunk_size_raw) < 4:
            break
        chunk_size = struct.unpack("<I", chunk_size_raw)[0]
        chunk_data = buf.read(chunk_size)
        if chunk_id == b"fmt ":
            fmt_fmt    = struct.unpack("<H", chunk_data[0:2])[0]
            channels   = struct.unpack("<H", chunk_data[2:4])[0]
            sample_rate= struct.unpack("<I", chunk_data[4:8])[0]
            bits       = struct.unpack("<H", chunk_data[14:16])[0]
        elif chunk_id == b"data":
            data_bytes = chunk_data

    n_frames = len(data_bytes) // (channels * (bits // 8))
    if fmt_fmt == WAV_FMT_PCM:
        if bits == 16:
            raw = np.frombuffer(data_bytes, dtype="<i2").reshape(-1, channels).astype(np.float32)
            samples = raw / 32768.0
        elif bits == 24:
            # 24-bit PCM: 3 bytes per sample, little-endian signed
            n_samples = len(data_bytes) // 3
            raw = np.zeros(n_samples, dtype=np.int32)
            for i in range(n_samples):
                b0, b1, b2 = data_bytes[i*3], data_bytes[i*3+1], data_bytes[i*3+2]
                val = b0 | (b1 << 8) | (b2 << 16)
                if val >= 0x800000:
                    val -= 0x1000000
                raw[i] = val
            samples = (raw.reshape(-1, channels).astype(np.float32)) / 8388608.0
        elif bits == 32:
            raw = np.frombuffer(data_bytes, dtype="<i4").reshape(-1, channels).astype(np.float32)
            samples = raw / 2147483648.0
        else:
            raise ValueError(f"Unsupported PCM bit depth: {bits}")
    elif fmt_fmt == WAV_FMT_FLOAT:
        if bits == 32:
            samples = np.frombuffer(data_bytes, dtype="<f4").reshape(-1, channels).copy()
        else:
            raise ValueError(f"Unsupported float bit depth: {bits}")
    else:
        raise ValueError(f"Unsupported WAV format code: {fmt_fmt}")

    return samples, sample_rate, channels


def write_wav(samples: "np.ndarray", sample_rate: int, n_channels: int) -> bytes:
    """
    Write float32 samples to WAV bytes (PCM-16, stereo or mono).
    samples shape: (n_frames, n_channels) or (n_frames,) for mono.
    """
    np = _import_numpy()
    if samples.ndim == 1:
        samples = samples.reshape(-1, 1)
    # Clip + convert to int16
    clipped = np.clip(samples, -1.0, 1.0)
    pcm = (clipped * 32767.0).astype(np.int16)
    pcm_bytes = pcm.tobytes()

    buf = io.BytesIO()
    data_size  = len(pcm_bytes)
    block_align = n_channels * 2
    byte_rate   = sample_rate * block_align

    buf.write(b"RIFF")
    buf.write(struct.pack("<I", 36 + data_size))
    buf.write(b"WAVE")
    buf.write(b"fmt ")
    buf.write(struct.pack("<I", 16))
    buf.write(struct.pack("<H", WAV_FMT_PCM))
    buf.write(struct.pack("<H", n_channels))
    buf.write(struct.pack("<I", sample_rate))
    buf.write(struct.pack("<I", byte_rate))
    buf.write(struct.pack("<H", block_align))
    buf.write(struct.pack("<H", 16))
    buf.write(b"data")
    buf.write(struct.pack("<I", data_size))
    buf.write(pcm_bytes)

    return buf.getvalue()


# ---------------------------------------------------------------------------
# XPN (ZIP) utilities
# ---------------------------------------------------------------------------

def open_xpn(xpn_path: Path) -> zipfile.ZipFile:
    """Open an XPN file as a ZipFile for reading."""
    if not zipfile.is_zipfile(xpn_path):
        raise ValueError(f"{xpn_path} is not a valid ZIP/XPN archive.")
    return zipfile.ZipFile(xpn_path, "r")


def read_manifest(zf: zipfile.ZipFile) -> Dict[str, str]:
    """
    Parse the Expansions/manifest plain-text key=value file.
    Returns dict of {key: value}.
    """
    manifest_path = None
    for name in zf.namelist():
        if name.lower().endswith("expansions/manifest") or name.lower() == "manifest":
            manifest_path = name
            break
        if "expansions" in name.lower() and name.lower().endswith("manifest"):
            manifest_path = name
            break

    if not manifest_path:
        return {}

    text = zf.read(manifest_path).decode("utf-8", errors="replace")
    result = {}
    for line in text.splitlines():
        if "=" in line:
            k, _, v = line.partition("=")
            result[k.strip()] = v.strip()
    return result


def list_wav_names(zf: zipfile.ZipFile) -> List[str]:
    """Return all WAV file paths inside the XPN archive."""
    return [n for n in zf.namelist() if n.lower().endswith(".wav")]


def write_xpn(
    source_zf: zipfile.ZipFile,
    output_path: Path,
    wav_replacements: Dict[str, bytes],
    manifest_override: Optional[Dict[str, str]] = None,
) -> None:
    """
    Write a new XPN archive to output_path, replacing WAV files in
    wav_replacements (keyed by archive path) and optionally updating the manifest.
    All other files are copied verbatim from source_zf.
    """
    output_path.parent.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(output_path, "w", compression=zipfile.ZIP_DEFLATED) as out_zf:
        manifest_written = False
        for item in source_zf.infolist():
            name_lower = item.filename.lower()
            if item.filename in wav_replacements:
                out_zf.writestr(item.filename, wav_replacements[item.filename])
            elif (
                manifest_override
                and ("expansions/manifest" in name_lower or name_lower == "manifest")
            ):
                lines = "\n".join(f"{k}={v}" for k, v in manifest_override.items())
                out_zf.writestr(item.filename, lines)
                manifest_written = True
            else:
                out_zf.writestr(item.filename, source_zf.read(item.filename))


# ---------------------------------------------------------------------------
# Variation 1: Tone (shelving EQ)
# ---------------------------------------------------------------------------

def _shelving_filter_coeffs(
    fc: float, gain_db: float, sample_rate: int, shelf_type: str
) -> Tuple[List[float], List[float]]:
    """
    Compute biquad IIR shelving filter coefficients.
    shelf_type: 'low' or 'high'.
    Returns (b, a) where b and a are length-3 coefficient lists.

    Formula: standard Audio EQ Cookbook (Robert Bristow-Johnson).
    """
    A  = 10 ** (gain_db / 40.0)
    w0 = 2 * math.pi * fc / sample_rate
    cos_w0 = math.cos(w0)
    sin_w0 = math.sin(w0)
    S  = 1.0  # shelf slope = 1 (maximally steep)
    alpha = sin_w0 / 2.0 * math.sqrt((A + 1.0/A) * (1.0/S - 1.0) + 2.0)

    if shelf_type == "low":
        b0 =      A * ((A+1) - (A-1)*cos_w0 + 2*math.sqrt(A)*alpha)
        b1 =  2 * A * ((A-1) - (A+1)*cos_w0)
        b2 =      A * ((A+1) - (A-1)*cos_w0 - 2*math.sqrt(A)*alpha)
        a0 =             (A+1) + (A-1)*cos_w0 + 2*math.sqrt(A)*alpha
        a1 =        -2 * ((A-1) + (A+1)*cos_w0)
        a2 =             (A+1) + (A-1)*cos_w0 - 2*math.sqrt(A)*alpha
    else:  # high
        b0 =      A * ((A+1) + (A-1)*cos_w0 + 2*math.sqrt(A)*alpha)
        b1 = -2 * A * ((A-1) + (A+1)*cos_w0)
        b2 =      A * ((A+1) + (A-1)*cos_w0 - 2*math.sqrt(A)*alpha)
        a0 =             (A+1) - (A-1)*cos_w0 + 2*math.sqrt(A)*alpha
        a1 =         2 * ((A-1) - (A+1)*cos_w0)
        a2 =             (A+1) - (A-1)*cos_w0 - 2*math.sqrt(A)*alpha

    b = [b0/a0, b1/a0, b2/a0]
    a = [1.0,   a1/a0, a2/a0]
    return b, a


def apply_tone_variant(
    samples: "np.ndarray",
    sample_rate: int,
    direction: str,
) -> "np.ndarray":
    """
    Apply spectral tilt to samples.
    direction: 'felix' (brighten) or 'oscar' (warm).

    feliX: +2dB high shelf at 8kHz, -2dB low shelf at 200Hz.
    Oscar: +3dB low shelf at 200Hz, -2dB high shelf at 8kHz.
    """
    np = _import_numpy()
    sp = _import_scipy()

    if direction == "felix":
        filters = [
            ("high", 8000.0, +2.0),
            ("low",  200.0,  -2.0),
        ]
    elif direction == "oscar":
        filters = [
            ("low",  200.0,  +3.0),
            ("high", 8000.0, -2.0),
        ]
    else:
        raise ValueError(f"Unknown tone direction: {direction!r} — use 'felix' or 'oscar'.")

    result = samples.copy().astype(np.float64)
    for shelf_type, fc, gain_db in filters:
        b, a = _shelving_filter_coeffs(fc, gain_db, sample_rate, shelf_type)
        if result.ndim == 1:
            result = sp.lfilter(b, a, result)
        else:
            for ch in range(result.shape[1]):
                result[:, ch] = sp.lfilter(b, a, result[:, ch])

    return result.astype(np.float32)


# ---------------------------------------------------------------------------
# Variation 2: Pitch (resampling)
# ---------------------------------------------------------------------------

def semitones_to_ratio(semitones: float) -> float:
    """Convert semitone offset to frequency ratio."""
    return 2.0 ** (semitones / 12.0)


PITCH_VARIANTS = {
    "fifth_up": {
        "semitones": 7.0,
        "description": "Up a perfect fifth (+7 semitones). Resampling introduces subtle aliasing — that IS the variation.",
    },
    "detuned": {
        "semitones": -12.0 / 100.0,  # -12 cents = -0.12 semitones
        "description": "Slightly flat by 12 cents. Resampled tape-wobble texture.",
    },
    "octave": {
        "semitones": 12.0,
        "description": "One octave up (+12 semitones). Resampling used; aliasing may add harmonics.",
    },
}


def apply_pitch_variant(
    samples: "np.ndarray",
    sample_rate: int,
    semitones: float,
) -> Tuple["np.ndarray", int]:
    """
    Pitch-shift samples by resampling.

    Strategy:
    1. Resample from original_length to target_length = original_length / ratio.
       This plays the sample back at a different speed, shifting pitch.
    2. The MPC will play this sample at its KeyTrack pitch — the XPM RootNote
       handles final pitch assignment.  We shift the raw audio to create a
       timbral variation, not a retuned keygroup.

    NOTE: This introduces aliasing when upsampling at integer ratios.
    That aliasing is the variation — analog-resample texture.

    Returns (shifted_samples, sample_rate) — sample_rate is UNCHANGED.
    Length will differ from input (shorter when pitching up, longer when down).
    To preserve approximate length: resample back to original length after shift.
    """
    np = _import_numpy()
    sp = _import_scipy()

    ratio = semitones_to_ratio(semitones)
    n_frames = samples.shape[0]
    n_channels = samples.shape[1] if samples.ndim > 1 else 1
    target_frames = max(1, int(round(n_frames / ratio)))

    if samples.ndim == 1:
        shifted = sp.resample(samples, target_frames)
        # Resample back to original length (time-stretching approximation):
        output = sp.resample(shifted, n_frames)
    else:
        channels_out = []
        for ch in range(n_channels):
            ch_data = samples[:, ch]
            shifted_ch = sp.resample(ch_data, target_frames)
            restored_ch = sp.resample(shifted_ch, n_frames)
            channels_out.append(restored_ch)
        output = np.stack(channels_out, axis=1)

    return output.astype(np.float32), sample_rate


# ---------------------------------------------------------------------------
# Variation 3: Time (attack trim / pad)
# ---------------------------------------------------------------------------

TIME_VARIANTS = {
    "tight": {
        "trim_ms":    5.0,
        "pad_ms":     0.0,
        "description": "Trim first 5ms of attack. Tighter, more staccato transient feel.",
    },
    "loose": {
        "trim_ms":    0.0,
        "pad_ms":     10.0,
        "description": "Add 10ms of silence at start. Looser, behind-the-beat feel.",
    },
}


def apply_time_variant(
    samples: "np.ndarray",
    sample_rate: int,
    variant: str,
) -> "np.ndarray":
    """
    Apply time variant: trim or pad the start of a sample.
    variant: 'tight' or 'loose'.
    """
    np = _import_numpy()
    tv = TIME_VARIANTS[variant]
    trim_frames = int(round(tv["trim_ms"] * sample_rate / 1000.0))
    pad_frames  = int(round(tv["pad_ms"]  * sample_rate / 1000.0))

    n_channels = samples.shape[1] if samples.ndim > 1 else 1

    result = samples
    if trim_frames > 0 and result.shape[0] > trim_frames:
        result = result[trim_frames:]

    if pad_frames > 0:
        if result.ndim == 1:
            silence = np.zeros(pad_frames, dtype=np.float32)
            result = np.concatenate([silence, result])
        else:
            silence = np.zeros((pad_frames, n_channels), dtype=np.float32)
            result = np.concatenate([silence, result], axis=0)

    return result.astype(np.float32)


# ---------------------------------------------------------------------------
# Variation 4: DNA suggestions (no audio processing)
# ---------------------------------------------------------------------------

DNA_KEYS = ["brightness", "warmth", "movement", "density", "space", "aggression"]

DNA_DISTANCE_THRESHOLD = 2.5  # Euclidean distance in 6D DNA space


def _dna_distance(a: Dict[str, float], b: Dict[str, float]) -> float:
    """Euclidean distance between two DNA dicts in 6D space."""
    return math.sqrt(sum((a.get(k, 0) - b.get(k, 0)) ** 2 for k in DNA_KEYS))


def _opposite_dna(dna: Dict[str, float]) -> Dict[str, float]:
    """Compute a DNA profile that is 'opposite' on all 6 axes (invert from midpoint 5)."""
    return {k: round(10.0 - dna.get(k, 5.0), 1) for k in DNA_KEYS}


def find_dna_variants(
    source_engine: str,
    source_dna: Dict[str, float],
    presets_dir: Path,
    n: int = 5,
) -> List[Dict]:
    """
    Search local .xometa preset library for presets that:
    1. Use the same engine (or any engine for cross-engine variants).
    2. Have a DNA profile meaningfully different from the source (distance > threshold).

    Returns list of up to n dicts with preset info + DNA distance.
    """
    if not presets_dir.exists():
        return []

    candidates = []
    for xometa_path in presets_dir.rglob("*.xometa"):
        try:
            preset = json.loads(xometa_path.read_text(encoding="utf-8", errors="replace"))
        except Exception:
            continue

        preset_engines = preset.get("engines", [])
        if not isinstance(preset_engines, list):
            preset_engines = [preset_engines]

        engine_match = any(
            source_engine.lower() in str(e).lower()
            for e in preset_engines
        )
        if not engine_match:
            continue

        preset_dna_raw = preset.get("sonic_dna") or preset.get("dna") or {}
        preset_dna = {k: float(preset_dna_raw.get(k, 5.0)) for k in DNA_KEYS}

        dist = _dna_distance(source_dna, preset_dna)
        if dist > DNA_DISTANCE_THRESHOLD:
            candidates.append({
                "preset_name":  preset.get("name", xometa_path.stem),
                "engine":       preset_engines[0] if preset_engines else "unknown",
                "dna":          preset_dna,
                "dna_distance": round(dist, 3),
                "file":         str(xometa_path),
                "mood":         preset.get("mood", ""),
            })

    # Sort by distance descending (most different first)
    candidates.sort(key=lambda c: -c["dna_distance"])
    return candidates[:n]


def build_dna_suggestions(
    xpn_path: Path,
    presets_dir: Optional[Path],
    output_dir: Path,
) -> Path:
    """
    Read source XPN manifest, extract engine/DNA, find variant presets,
    write dna_variant_suggestions.json.
    Returns path to written JSON.
    """
    with open_xpn(xpn_path) as zf:
        manifest = read_manifest(zf)

        # Try to find a .xometa reference inside the XPN for DNA data
        source_dna: Dict[str, float] = {k: 5.0 for k in DNA_KEYS}
        source_engine = manifest.get("Engine", manifest.get("engine", "unknown"))
        for name in zf.namelist():
            if name.lower().endswith(".xometa"):
                try:
                    meta = json.loads(zf.read(name).decode("utf-8", errors="replace"))
                    raw_dna = meta.get("sonic_dna") or meta.get("dna") or {}
                    if raw_dna:
                        source_dna = {k: float(raw_dna.get(k, 5.0)) for k in DNA_KEYS}
                    engines = meta.get("engines", [])
                    if engines:
                        source_engine = str(engines[0]) if isinstance(engines, list) else str(engines)
                    break
                except Exception:
                    pass

    suggestions = []
    if presets_dir:
        suggestions = find_dna_variants(source_engine, source_dna, presets_dir)

    opposite = _opposite_dna(source_dna)

    payload = {
        "source_xpn":    str(xpn_path),
        "source_engine": source_engine,
        "source_dna":    source_dna,
        "target_dna":    opposite,
        "instructions": (
            "These presets share the same engine but have meaningfully different "
            "DNA profiles.  Use them as render targets for new XPN packs that "
            "provide tonal contrast to the source."
        ),
        "suggestions": suggestions,
    }

    output_dir.mkdir(parents=True, exist_ok=True)
    out_path = output_dir / f"{xpn_path.stem}_dna_variant_suggestions.json"
    out_path.write_text(json.dumps(payload, indent=2), encoding="utf-8")
    return out_path


# ---------------------------------------------------------------------------
# Main variation pipeline
# ---------------------------------------------------------------------------

def _process_wav_in_xpn(
    zf: zipfile.ZipFile,
    wav_name: str,
    processor,          # callable(samples, sample_rate, channels) -> (samples, sample_rate)
) -> bytes:
    """Read WAV from ZipFile, apply processor, return new WAV bytes."""
    raw = zf.read(wav_name)
    samples, sr, channels = read_wav(raw)
    new_samples, new_sr = processor(samples, sr, channels)
    return write_wav(new_samples, new_sr, channels)


def generate_tone_variant(
    xpn_path: Path,
    direction: str,
    output_dir: Path,
) -> Path:
    """Generate feliX or Oscar tone variant XPN."""
    np = _import_numpy()

    suffix = "felix_tone" if direction == "felix" else "oscar_tone"
    out_path = output_dir / f"{xpn_path.stem}_{suffix}.xpn"
    output_dir.mkdir(parents=True, exist_ok=True)

    with open_xpn(xpn_path) as zf:
        wav_names = list_wav_names(zf)
        replacements: Dict[str, bytes] = {}

        for wname in wav_names:
            raw = zf.read(wname)
            try:
                samples, sr, channels = read_wav(raw)
            except Exception as e:
                print(f"    WARN: skipping {wname} — {e}", file=sys.stderr)
                continue
            toned = apply_tone_variant(samples, sr, direction)
            replacements[wname] = write_wav(toned, sr, channels)

        manifest = read_manifest(zf)
        original_name = manifest.get("Name", xpn_path.stem)
        label = "feliX Tone (Bright)" if direction == "felix" else "Oscar Tone (Warm)"
        manifest["Name"] = f"{original_name} [{label}]"
        manifest["Description"] = (
            f"Tone variant — {label}.  "
            + ("High shelf +2dB @ 8kHz, low shelf -2dB @ 200Hz."
               if direction == "felix"
               else "Low shelf +3dB @ 200Hz, high shelf -2dB @ 8kHz.")
        )

        write_xpn(zf, out_path, replacements, manifest)

    return out_path


def generate_pitch_variant(
    xpn_path: Path,
    pitch_variant: str,
    output_dir: Path,
) -> Path:
    """Generate pitch-shifted XPN variant."""
    pv = PITCH_VARIANTS[pitch_variant]
    out_path = output_dir / f"{xpn_path.stem}_{pitch_variant}.xpn"
    output_dir.mkdir(parents=True, exist_ok=True)

    with open_xpn(xpn_path) as zf:
        wav_names = list_wav_names(zf)
        replacements: Dict[str, bytes] = {}

        for wname in wav_names:
            raw = zf.read(wname)
            try:
                samples, sr, channels = read_wav(raw)
            except Exception as e:
                print(f"    WARN: skipping {wname} — {e}", file=sys.stderr)
                continue
            shifted, new_sr = apply_pitch_variant(samples, sr, pv["semitones"])
            replacements[wname] = write_wav(shifted, new_sr, channels)

        manifest = read_manifest(zf)
        original_name = manifest.get("Name", xpn_path.stem)
        label = {
            "fifth_up": "+Fifth (Resampled)",
            "detuned":  "Detuned -12¢ (Resampled)",
            "octave":   "+Octave (Resampled)",
        }[pitch_variant]
        manifest["Name"] = f"{original_name} [{label}]"
        manifest["Description"] = (
            f"Pitch variant — {pv['description']}"
        )

        write_xpn(zf, out_path, replacements, manifest)

    return out_path


def generate_time_variant(
    xpn_path: Path,
    time_variant: str,
    output_dir: Path,
) -> Path:
    """Generate tight or loose time variant XPN."""
    tv = TIME_VARIANTS[time_variant]
    out_path = output_dir / f"{xpn_path.stem}_{time_variant}.xpn"
    output_dir.mkdir(parents=True, exist_ok=True)

    with open_xpn(xpn_path) as zf:
        wav_names = list_wav_names(zf)
        replacements: Dict[str, bytes] = {}

        for wname in wav_names:
            raw = zf.read(wname)
            try:
                samples, sr, channels = read_wav(raw)
            except Exception as e:
                print(f"    WARN: skipping {wname} — {e}", file=sys.stderr)
                continue
            timed = apply_time_variant(samples, sr, time_variant)
            replacements[wname] = write_wav(timed, sr, channels)

        manifest = read_manifest(zf)
        original_name = manifest.get("Name", xpn_path.stem)
        label = "Tight (5ms trim)" if time_variant == "tight" else "Loose (10ms pad)"
        manifest["Name"] = f"{original_name} [{label}]"
        manifest["Description"] = f"Time variant — {tv['description']}"

        write_xpn(zf, out_path, replacements, manifest)

    return out_path


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

AUDIO_VARIANTS_ALL = [
    ("tone",  "felix"),
    ("tone",  "oscar"),
    ("pitch", "fifth_up"),
    ("pitch", "detuned"),
    ("time",  "tight"),
    ("time",  "loose"),
]


def main(argv: Optional[List[str]] = None) -> int:
    parser = argparse.ArgumentParser(
        description="XPN Variation Generator — tonal/spatial/character pack variations.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    parser.add_argument("xpn", metavar="pack.xpn",
                        help="Source XPN pack file.")
    parser.add_argument(
        "--type",
        choices=["tone", "pitch", "time", "dna", "all"],
        default="all",
        help=(
            "Variation type. "
            "'all' generates all 6 audio variants. "
            "'dna' outputs preset suggestions only (no audio)."
        ),
    )
    parser.add_argument(
        "--direction",
        choices=["felix", "oscar"],
        default=None,
        help="Tone direction (required when --type=tone).",
    )
    parser.add_argument(
        "--pitch",
        choices=["fifth_up", "detuned", "octave"],
        default=None,
        help="Pitch variant (required when --type=pitch).",
    )
    parser.add_argument(
        "--time",
        choices=["tight", "loose"],
        default=None,
        help="Time variant (required when --type=time).",
    )
    parser.add_argument(
        "--output", default="./variations/",
        help="Output directory. Default: ./variations/",
    )
    parser.add_argument(
        "--presets", default=None,
        help="Path to local .xometa preset library (for --type=dna).",
    )

    args = parser.parse_args(argv)

    xpn_path = Path(args.xpn)
    if not xpn_path.exists():
        print(f"ERROR: {xpn_path} not found.", file=sys.stderr)
        return 1

    output_dir = Path(args.output)
    presets_dir = Path(args.presets) if args.presets else None

    print(f"\nXPN Variation Generator — {xpn_path.name}")
    print(f"Output: {output_dir}\n")

    errors = 0

    def run_tone(direction: str) -> None:
        nonlocal errors
        try:
            out = generate_tone_variant(xpn_path, direction, output_dir)
            print(f"  [tone/{direction:<6}]  →  {out.name}")
        except Exception as e:
            print(f"  [tone/{direction:<6}]  ERROR: {e}", file=sys.stderr)
            errors += 1

    def run_pitch(variant: str) -> None:
        nonlocal errors
        try:
            out = generate_pitch_variant(xpn_path, variant, output_dir)
            print(f"  [pitch/{variant:<10}]  →  {out.name}")
        except Exception as e:
            print(f"  [pitch/{variant:<10}]  ERROR: {e}", file=sys.stderr)
            errors += 1

    def run_time(variant: str) -> None:
        nonlocal errors
        try:
            out = generate_time_variant(xpn_path, variant, output_dir)
            print(f"  [time/{variant:<7}]  →  {out.name}")
        except Exception as e:
            print(f"  [time/{variant:<7}]  ERROR: {e}", file=sys.stderr)
            errors += 1

    def run_dna() -> None:
        nonlocal errors
        try:
            out = build_dna_suggestions(xpn_path, presets_dir, output_dir)
            print(f"  [dna           ]  →  {out.name}")
        except Exception as e:
            print(f"  [dna           ]  ERROR: {e}", file=sys.stderr)
            errors += 1

    if args.type == "tone":
        directions = [args.direction] if args.direction else ["felix", "oscar"]
        for d in directions:
            run_tone(d)

    elif args.type == "pitch":
        variants = [args.pitch] if args.pitch else ["fifth_up", "detuned", "octave"]
        for v in variants:
            run_pitch(v)

    elif args.type == "time":
        variants = [args.time] if args.time else ["tight", "loose"]
        for v in variants:
            run_time(v)

    elif args.type == "dna":
        run_dna()

    elif args.type == "all":
        run_tone("felix")
        run_tone("oscar")
        run_pitch("fifth_up")
        run_pitch("detuned")
        run_time("tight")
        run_time("loose")
        run_dna()

    print(f"\nDone.  {errors} error(s)." if errors else "\nDone.")
    return 1 if errors else 0


if __name__ == "__main__":
    sys.exit(main())
