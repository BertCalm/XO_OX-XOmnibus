#!/usr/bin/env python3
"""
XOutshine — Sample Pack Upgrade Pipeline
XO_OX Designs

Outshine the original.

Takes any sample pack (XPN archive, WAV folder, or loose samples) and
transforms it into a production-quality, expressive, dynamic MPC instrument
that outshines the source material.

The XO standard elevates everything it touches:
  - Classifies every sample (kick, snare, hat, bass, pad, lead, fx, etc.)
  - Adds velocity layers via amplitude + filter shaping
  - Generates round-robin variations via micro-variation (pitch, saturation, time)
  - Maps expression (aftertouch → filter, mod wheel → character)
  - Sets one-shot / choke groups / mute groups intelligently
  - LUFS-normalizes for consistent loudness
  - Applies fade guards for click-free playback
  - Removes DC offset
  - Adds TPDF dithering
  - Detects and sets loop points for sustained sounds
  - Generates XPM programs (drum + keygroup)
  - Packages as .xpn or organized WAV folder with XPMs

Usage:
    # Upgrade an existing XPN
    python xoutshine.py --input pack.xpn --output upgraded_pack.xpn

    # Upgrade a WAV folder
    python xoutshine.py --input ./my_samples/ --output ./my_pack.xpn

    # Upgrade a WAV folder → organized folder (not zipped)
    python xoutshine.py --input ./my_samples/ --output ./my_pack/ --format folder

    # Upgrade with specific options
    python xoutshine.py --input pack.xpn --output upgraded.xpn \
        --velocity-layers 5 \
        --round-robin 4 \
        --velocity-curve "boom-bap" \
        --lufs-target -14

Pipeline stages:
    1. INGEST    — Unpack XPN or scan WAV folder, catalog all samples
    2. CLASSIFY  — Auto-detect instrument type per sample (drum/melodic/pad/fx)
    3. ANALYZE   — Measure pitch, RMS, peak, DC offset, tail length, loop points
    4. ENHANCE   — Generate velocity layers, round-robin, fade guards, DC removal
    5. NORMALIZE — LUFS normalize, true-peak limit, TPDF dither
    6. MAP       — Build XPM programs with expression, choke groups, pad colors
    7. PACKAGE   — Assemble XPN archive or organized folder
    8. VALIDATE  — Run full QA suite on output

XO_OX Designs — Outshine the original.
"""

import argparse
import hashlib
import json
import math
import os
import random
import shutil
import struct
import sys
import tempfile
import wave
import xml.etree.ElementTree as ET
import zipfile
from collections import defaultdict
from pathlib import Path
from dataclasses import dataclass, asdict
from typing import Dict, List, Optional, Tuple

# Optional tqdm progress bars — degrade gracefully if not installed
try:
    from tqdm import tqdm as _tqdm
    HAS_TQDM = True
except ImportError:
    HAS_TQDM = False

# Optional numpy — used for fast FFT when available; falls back to pure-Python
try:
    import numpy as _np
    HAS_NUMPY = True
except ImportError:
    HAS_NUMPY = False

def tqdm(iterable, **kwargs):
    """Wrap tqdm if available, otherwise iterate transparently."""
    if HAS_TQDM:
        return _tqdm(iterable, **kwargs)
    desc = kwargs.get("desc", "")
    total = kwargs.get("total", None)
    if desc:
        print(f"  {desc}...")
    return iterable

# ─────────────────────────────────────────────────────────────────────────────
# Sonic DNA — 6-dimensional audio fingerprint
# ─────────────────────────────────────────────────────────────────────────────

@dataclass
class SonicDNA:
    """6D Sonic DNA fingerprint inferred from audio analysis."""
    brightness: float = 0.5   # spectral centroid / (sr/2), 0-1
    warmth: float = 0.5       # energy ratio of 200-800 Hz band / total
    movement: float = 0.5     # spectral flux (frame-to-frame change)
    density: float = 0.5      # spectral flatness (geometric/arithmetic mean)
    space: float = 0.5        # reverb tail ratio (energy in last 30%)
    aggression: float = 0.5   # crest factor normalized: >12dB=1.0, <3dB=0.0

    def to_dict(self) -> dict:
        return asdict(self)


def _radix2_fft(x: List[complex]) -> List[complex]:
    """In-place radix-2 Cooley-Tukey FFT. Input length must be a power of 2."""
    n = len(x)
    if n <= 1:
        return x

    # Bit-reversal permutation
    result = list(x)
    bits = int(math.log2(n))
    for i in range(n):
        j = 0
        for b in range(bits):
            j = (j << 1) | ((i >> b) & 1)
        if j > i:
            result[i], result[j] = result[j], result[i]

    # Butterfly stages
    length = 2
    while length <= n:
        half = length // 2
        w_base = -2.0 * math.pi / length
        for start in range(0, n, length):
            for k in range(half):
                angle = w_base * k
                w = complex(math.cos(angle), math.sin(angle))
                even = result[start + k]
                odd = result[start + k + half] * w
                result[start + k] = even + odd
                result[start + k + half] = even - odd
        length *= 2

    return result


def _magnitude_spectrum(mono: List[float], fft_size: int = 2048) -> List[float]:
    """Compute magnitude spectrum of a signal chunk using radix-2 FFT.

    Returns magnitudes for bins 0..fft_size//2 (inclusive).
    Uses numpy.fft.rfft when numpy is available for much faster batch processing.
    """
    n = len(mono)
    # Zero-pad or truncate to fft_size
    if n >= fft_size:
        segment = mono[:fft_size]
    else:
        segment = mono + [0.0] * (fft_size - n)

    # Apply Hann window
    windowed = [
        segment[i] * 0.5 * (1.0 - math.cos(2.0 * math.pi * i / (fft_size - 1)))
        for i in range(fft_size)
    ]

    n_bins = fft_size // 2 + 1
    if HAS_NUMPY:
        spectrum = _np.abs(_np.fft.rfft(windowed))[:n_bins]
        return list(spectrum)
    else:
        # Pure-Python fallback path
        spectrum = _radix2_fft([complex(s, 0) for s in windowed])
        return [abs(spectrum[i]) for i in range(n_bins)]


def infer_dna(channels: List[List[float]], sr: int) -> SonicDNA:
    """Infer 6D Sonic DNA from audio characteristics.

    Uses spectral analysis (radix-2 FFT) to compute brightness, warmth,
    movement, density, space, and aggression.
    """
    if not channels or not channels[0]:
        return SonicDNA()

    # Work with mono (first channel)
    mono = channels[0]
    n = len(mono)
    fft_size = 2048

    if n < fft_size:
        # Too short for meaningful spectral analysis — return defaults
        # but still compute space and aggression which don't need FFT
        dna = SonicDNA()
        # Aggression from crest factor
        rms = math.sqrt(sum(s * s for s in mono) / max(1, n))
        peak = max(abs(s) for s in mono) if mono else 0
        if rms > 1e-10:
            crest_db = 20 * math.log10(max(peak, 1e-10) / rms)
            dna.aggression = max(0.0, min(1.0, (crest_db - 3.0) / 9.0))
        # Space from tail energy
        tail_start = int(n * 0.7)
        total_energy = sum(s * s for s in mono)
        tail_energy = sum(s * s for s in mono[tail_start:])
        if total_energy > 1e-10:
            dna.space = max(0.0, min(1.0, tail_energy / total_energy / 0.3))
        return dna

    # ── Brightness: spectral centroid / (sr/2) ──
    # Average centroid over multiple frames
    hop = fft_size // 2
    num_frames = max(1, (n - fft_size) // hop + 1)
    # Limit to ~20 frames for performance
    frame_step = max(1, num_frames // 20)
    centroids = []
    prev_magnitudes = None
    flux_values = []
    flatness_values = []

    freq_per_bin = sr / fft_size
    num_bins = fft_size // 2 + 1
    warmth_per_frame = []

    for frame_idx in range(0, num_frames, frame_step):
        offset = frame_idx * hop
        if offset + fft_size > n:
            break
        chunk = mono[offset:offset + fft_size]
        mags = _magnitude_spectrum(chunk, fft_size)

        # Spectral centroid
        mag_sum = sum(mags)
        if mag_sum > 1e-10:
            weighted_sum = sum(mags[i] * i * freq_per_bin for i in range(num_bins))
            centroid_hz = weighted_sum / mag_sum
            centroids.append(centroid_hz)

        # Spectral flux (frame-to-frame change)
        if prev_magnitudes is not None:
            flux = sum((mags[i] - prev_magnitudes[i]) ** 2 for i in range(num_bins))
            flux_values.append(math.sqrt(flux))
        prev_magnitudes = mags

        # Spectral flatness (geometric mean / arithmetic mean)
        # Use log-domain for geometric mean to avoid underflow
        positive_mags = [m for m in mags if m > 1e-10]
        if len(positive_mags) > num_bins * 0.5:
            log_sum = sum(math.log(m) for m in positive_mags)
            geo_mean = math.exp(log_sum / len(positive_mags))
            arith_mean = sum(positive_mags) / len(positive_mags)
            if arith_mean > 1e-10:
                flatness_values.append(geo_mean / arith_mean)

        # Warmth: 200-800 Hz band energy ratio (accumulated per frame)
        bin_200 = max(1, int(200 / freq_per_bin))
        bin_800 = min(num_bins - 1, int(800 / freq_per_bin))
        warm_energy = sum(m * m for m in mags[bin_200:bin_800 + 1])
        total_spectral_energy = sum(m * m for m in mags[1:])  # skip DC
        if total_spectral_energy > 1e-10:
            warmth_per_frame.append(max(0.0, min(1.0, warm_energy / total_spectral_energy)))

    # Brightness
    nyquist = sr / 2.0
    if centroids:
        avg_centroid = sum(centroids) / len(centroids)
        brightness = max(0.0, min(1.0, avg_centroid / nyquist))
    else:
        brightness = 0.5

    # ── Warmth: mean energy ratio of 200-800 Hz band across all analysis frames ──
    if warmth_per_frame:
        warmth = sum(warmth_per_frame) / len(warmth_per_frame)
    else:
        warmth = 0.5

    # ── Movement: spectral flux (normalized) ──
    if flux_values:
        # Normalize: typical flux range varies widely, use median-based scaling
        sorted_flux = sorted(flux_values)
        median_flux = sorted_flux[len(sorted_flux) // 2]
        # Scale so median flux maps to ~0.5
        # Use coefficient of variation for better discrimination
        mean_flux = sum(flux_values) / len(flux_values)
        if mean_flux > 1e-10:
            std_flux = math.sqrt(sum((f - mean_flux) ** 2 for f in flux_values) / len(flux_values))
            cv = std_flux / mean_flux
            movement = max(0.0, min(1.0, cv))   # clamp: CV > 1.0 maps to 1.0
        else:
            movement = 0.0
    else:
        movement = 0.0

    # ── Density: spectral flatness ──
    if flatness_values:
        density = max(0.0, min(1.0, sum(flatness_values) / len(flatness_values)))
    else:
        density = 0.5

    # ── Space: reverb tail ratio (energy in last 30% / total) ──
    tail_start = int(n * 0.7)
    total_energy = sum(s * s for s in mono)
    tail_energy = sum(s * s for s in mono[tail_start:])
    if total_energy > 1e-10:
        # 30% of length should contain 30% of energy for flat signals
        # Reverb tails have disproportionate tail energy relative to their amplitude
        raw_ratio = tail_energy / total_energy
        # Normalize: 0.3 ratio = 0.5 (flat), higher = more space
        space = max(0.0, min(1.0, raw_ratio / 0.6))
    else:
        space = 0.5

    # ── Aggression: crest factor (peak / RMS), normalized ──
    rms = math.sqrt(sum(s * s for s in mono) / max(1, n))
    peak = max(abs(s) for s in mono) if mono else 0
    if rms > 1e-10:
        crest_db = 20 * math.log10(max(peak, 1e-10) / rms)
        # >12dB = 1.0 (very dynamic/aggressive), <3dB = 0.0 (compressed/flat)
        aggression = max(0.0, min(1.0, (crest_db - 3.0) / 9.0))
    else:
        aggression = 0.5

    return SonicDNA(
        brightness=round(brightness, 3),
        warmth=round(warmth, 3),
        movement=round(movement, 3),
        density=round(density, 3),
        space=round(space, 3),
        aggression=round(aggression, 3),
    )


def select_velocity_curve_by_dna(dna: SonicDNA) -> str:
    """Select the best velocity curve based on DNA aggression."""
    if dna.aggression > 0.7:
        return "trap-hard"
    elif dna.aggression > 0.4:
        return "boom-bap"
    elif dna.aggression > 0.2:
        return "musical"
    else:
        return "neo-soul"


@dataclass
class DNAEnhancementParams:
    """Enhancement parameters derived from Sonic DNA."""
    velocity_curve: str = "musical"
    fade_out_ms: float = 10.0
    saturation_intensity: float = 0.03  # max tanh drive for round-robin
    loop_priority: bool = True          # whether to prioritize loop detection
    loop_crossfade_multiplier: float = 1.0  # multiplier for loop crossfade length
    rr_variation_width: float = 1.0     # multiplier for round-robin variation range
    apply_soft_lowpass: bool = False     # gentle low-pass on soft velocity layers


def adjust_enhancement_by_dna(dna: SonicDNA) -> DNAEnhancementParams:
    """Derive enhancement parameters from inferred Sonic DNA."""
    params = DNAEnhancementParams()

    # 1. Velocity curve from aggression
    params.velocity_curve = select_velocity_curve_by_dna(dna)

    # 2. Fade guard length from space
    if dna.space > 0.6:
        params.fade_out_ms = 50.0
        params.apply_soft_lowpass = True
    elif dna.space < 0.3:
        params.fade_out_ms = 5.0
    else:
        params.fade_out_ms = 10.0

    # 3. Saturation intensity from aggression + warmth
    if dna.aggression > 0.6 and dna.warmth < 0.4:
        params.saturation_intensity = 0.08  # harder saturation
    elif dna.warmth > 0.6:
        params.saturation_intensity = 0.015  # gentler, preserve warmth
    else:
        params.saturation_intensity = 0.03

    # 4. Loop detection priority from space + movement
    if dna.space > 0.5 and dna.movement > 0.5:
        params.loop_priority = True
        params.loop_crossfade_multiplier = 2.0  # longer crossfade
    elif dna.space < 0.3:
        params.loop_priority = False  # skip loop detection for one-shots

    # 5. Round-robin variation character from density
    if dna.density > 0.6:
        params.rr_variation_width = 0.5  # subtle micro-variations
    elif dna.density < 0.3:
        params.rr_variation_width = 1.5  # wider variations for simple sounds
    else:
        params.rr_variation_width = 1.0

    return params


# ─────────────────────────────────────────────────────────────────────────────
# Constants
# ─────────────────────────────────────────────────────────────────────────────

VERSION = "1.0.0"
BANNER = """
╔═══════════════════════════════════════════════════════════════╗
║                                                               ║
║  ██╗  ██╗ ██████╗ ██╗   ██╗████████╗███████╗██╗  ██╗██╗███╗  ██║
║  ╚██╗██╔╝██╔═══██╗██║   ██║╚══██╔══╝██╔════╝██║  ██║██║████╗ ██║
║   ╚███╔╝ ██║   ██║██║   ██║   ██║   ███████╗███████║██║██╔██╗██║
║   ██╔██╗ ██║   ██║██║   ██║   ██║   ╚════██║██╔══██║██║██║╚████║
║  ██╔╝ ██╗╚██████╔╝╚██████╔╝   ██║   ███████║██║  ██║██║██║ ╚███║
║  ╚═╝  ╚═╝ ╚═════╝  ╚═════╝    ╚═╝   ╚══════╝╚═╝  ╚═╝╚═╝╚═╝  ╚═║
║                                                               ║
║              Outshine the original.  v{ver}                ║
║              XO_OX Designs                                    ║
║                                                               ║
╚═══════════════════════════════════════════════════════════════╝
""".format(ver=VERSION)

# Sample classification categories
CATEGORY_KICK = "kick"
CATEGORY_SNARE = "snare"
CATEGORY_HIHAT_CLOSED = "hihat_closed"
CATEGORY_HIHAT_OPEN = "hihat_open"
CATEGORY_CLAP = "clap"
CATEGORY_TOM = "tom"
CATEGORY_PERC = "percussion"
CATEGORY_FX = "fx"
CATEGORY_BASS = "bass"
CATEGORY_PAD = "pad"
CATEGORY_LEAD = "lead"
CATEGORY_KEYS = "keys"
CATEGORY_PLUCK = "pluck"
CATEGORY_STRING = "string"
CATEGORY_UNKNOWN = "unknown"

DRUM_CATEGORIES = {CATEGORY_KICK, CATEGORY_SNARE, CATEGORY_HIHAT_CLOSED,
                   CATEGORY_HIHAT_OPEN, CATEGORY_CLAP, CATEGORY_TOM,
                   CATEGORY_PERC, CATEGORY_FX}

MELODIC_CATEGORIES = {CATEGORY_BASS, CATEGORY_PAD, CATEGORY_LEAD,
                      CATEGORY_KEYS, CATEGORY_PLUCK, CATEGORY_STRING}

# GM drum note mapping
GM_DRUM_MAP = {
    CATEGORY_KICK: 36, CATEGORY_SNARE: 38, CATEGORY_HIHAT_CLOSED: 42,
    CATEGORY_HIHAT_OPEN: 46, CATEGORY_CLAP: 39, CATEGORY_TOM: 41,
    CATEGORY_PERC: 43, CATEGORY_FX: 49,
}

# Velocity curves (name → list of (vel_start, vel_end, volume) tuples)
VELOCITY_CURVES = {
    "musical": [
        (1, 20, 0.30),    # ghost — barely touching
        (21, 50, 0.55),   # light — conversational
        (51, 90, 0.75),   # mid — the expressive sweet spot
        (91, 127, 0.95),  # hard — full commit
    ],
    "boom-bap": [
        (1, 15, 0.25),    # ghost — Dilla whisper
        (16, 45, 0.50),   # pocket — where the groove lives
        (46, 85, 0.78),   # push — driving the beat
        (86, 127, 1.00),  # crack — snare hits that slap
    ],
    "neo-soul": [
        (1, 30, 0.35),    # breath — wide ghost range for feel
        (31, 65, 0.60),   # touch — gentle expression
        (66, 95, 0.80),   # lean — pushing into it
        (96, 127, 0.95),  # full — still warm, never harsh
    ],
    "trap-hard": [
        (1, 10, 0.20),    # sub — barely there
        (11, 35, 0.55),   # hit — quick ramp
        (36, 70, 0.80),   # slam — most of range is loud
        (71, 127, 1.00),  # max — ceiling
    ],
    "linear": [
        (1, 31, 0.25),
        (32, 63, 0.50),
        (64, 95, 0.75),
        (96, 127, 1.00),
    ],
}

# Default pad colors (used when no engine color available)
DEFAULT_PAD_COLOR = "#E9C46A"  # XO Gold

# Loop crossfade applied at the loop splice point.
# MPC interprets this as a number of samples (not milliseconds).
# 100 samples ≈ 2.3 ms @ 44.1 kHz — short enough to avoid audible pre-echo.
LOOP_CROSSFADE_SAMPLES = 100


# ─────────────────────────────────────────────────────────────────────────────
# Audio I/O Helpers
# ─────────────────────────────────────────────────────────────────────────────

def read_wav(path: str) -> Tuple[List[List[float]], int, int]:
    """Read WAV file → (channels, sample_rate, bit_depth). Channels are float [-1, 1]."""
    with wave.open(path, "rb") as wf:
        nch = wf.getnchannels()
        sr = wf.getframerate()
        sw = wf.getsampwidth()
        nframes = wf.getnframes()
        raw = wf.readframes(nframes)

    bd = sw * 8
    channels = [[] for _ in range(nch)]
    if sw == 2:
        fmt = f"<{nframes * nch}h"
        samples = struct.unpack(fmt, raw)
        scale = 1.0 / 32768.0
        for i, s in enumerate(samples):
            channels[i % nch].append(s * scale)
    elif sw == 3:
        scale = 1.0 / 8388608.0
        for i in range(0, len(raw), 3):
            val = raw[i] | (raw[i + 1] << 8) | (raw[i + 2] << 16)
            if val & 0x800000:
                val -= 0x1000000
            ch = (i // 3) % nch
            channels[ch].append(val * scale)
    elif sw == 4:
        fmt = f"<{nframes * nch}i"
        samples = struct.unpack(fmt, raw)
        scale = 1.0 / 2147483648.0
        for i, s in enumerate(samples):
            channels[i % nch].append(s * scale)
    else:
        raise ValueError(f"Unsupported bit depth: {bd}")
    return channels, sr, bd


def write_wav(path: str, channels: List[List[float]], sr: int, bd: int = 24):
    """Write float channels to WAV file with TPDF dithering."""
    nch = len(channels)
    nframes = len(channels[0])
    sw = bd // 8

    with wave.open(path, "wb") as wf:
        wf.setnchannels(nch)
        wf.setsampwidth(sw)
        wf.setframerate(sr)

        data = bytearray()
        rng = random.Random(42)  # deterministic dither
        for i in range(nframes):
            for ch in range(nch):
                s = channels[ch][i]
                # TPDF dither
                if bd <= 24:
                    dither = (rng.random() - 0.5 + rng.random() - 0.5) / (2 ** (bd - 1))
                    s = max(-1.0, min(1.0, s + dither))
                if sw == 2:
                    val = max(-32768, min(32767, int(s * 32767)))
                    data.extend(struct.pack("<h", val))
                elif sw == 3:
                    val = max(-8388608, min(8388607, int(s * 8388607)))
                    if val < 0:
                        val += 0x1000000
                    data.append(val & 0xFF)
                    data.append((val >> 8) & 0xFF)
                    data.append((val >> 16) & 0xFF)
        wf.writeframes(bytes(data))


# ─────────────────────────────────────────────────────────────────────────────
# Stage 1: INGEST
# ─────────────────────────────────────────────────────────────────────────────

class SampleInfo:
    """Metadata for a single sample in the pipeline."""
    def __init__(self, path: str, original_name: str):
        self.path = path
        self.original_name = original_name
        self.category = CATEGORY_UNKNOWN
        self.pitch_hz: Optional[float] = None
        self.midi_note: Optional[int] = None
        self.rms_db: float = -100.0
        self.peak_db: float = -100.0
        self.dc_offset: float = 0.0
        self.duration_s: float = 0.0
        self.sample_rate: int = 44100
        self.bit_depth: int = 24
        self.channels: int = 2
        self.is_loopable: bool = False
        self.loop_start: int = 0
        self.loop_end: int = 0
        self.tail_length_s: float = 0.0
        self.dna: Optional[SonicDNA] = None

    def __repr__(self):
        return f"<Sample '{self.original_name}' cat={self.category} rms={self.rms_db:.1f}dB>"


def ingest(input_path: str, work_dir: str) -> List[SampleInfo]:
    """Stage 1: Unpack XPN or scan WAV folder, catalog all samples."""
    samples = []
    wav_dir = os.path.join(work_dir, "ingested")
    os.makedirs(wav_dir, exist_ok=True)

    if input_path.lower().endswith(".xpn") and zipfile.is_zipfile(input_path):
        # Unzip XPN
        print(f"  Unpacking XPN: {input_path}")
        with zipfile.ZipFile(input_path, "r") as zf:
            for member in zf.namelist():
                if member.lower().endswith(".wav"):
                    fname = os.path.basename(member)
                    dest = os.path.join(wav_dir, fname)
                    with zf.open(member) as src, open(dest, "wb") as dst:
                        dst.write(src.read())
                    samples.append(SampleInfo(dest, fname))
    elif os.path.isdir(input_path):
        # Scan WAV folder recursively
        print(f"  Scanning folder: {input_path}")
        for root, dirs, files in os.walk(input_path):
            for f in sorted(files):
                if f.lower().endswith((".wav", ".aif", ".aiff")):
                    src = os.path.join(root, f)
                    dest = os.path.join(wav_dir, f)
                    if not os.path.exists(dest):
                        shutil.copy2(src, dest)
                    samples.append(SampleInfo(dest, f))
    else:
        raise ValueError(f"Input must be a .xpn file or a directory: {input_path}")

    print(f"  Ingested {len(samples)} samples")
    return samples


# ─────────────────────────────────────────────────────────────────────────────
# Stage 2: CLASSIFY
# ─────────────────────────────────────────────────────────────────────────────

def classify_by_name(name: str) -> str:
    """Heuristic classification from filename."""
    nl = name.lower()
    if any(k in nl for k in ("kick", "kck", "bd", "bass_drum", "bassdrum")):
        return CATEGORY_KICK
    if any(k in nl for k in ("snare", "snr", "sd", "rim")):
        return CATEGORY_SNARE
    if any(k in nl for k in ("hihat", "hh", "hi_hat", "hat")):
        if any(k in nl for k in ("open", "opn", "oh")):
            return CATEGORY_HIHAT_OPEN
        return CATEGORY_HIHAT_CLOSED
    if any(k in nl for k in ("clap", "clp", "cp")):
        return CATEGORY_CLAP
    if any(k in nl for k in ("tom", "floor")):
        return CATEGORY_TOM
    if any(k in nl for k in ("perc", "shaker", "tamb", "conga", "bongo")):
        return CATEGORY_PERC
    if any(k in nl for k in ("fx", "riser", "sweep", "impact", "hit")):
        return CATEGORY_FX
    if any(k in nl for k in ("bass", "sub", "808")):
        return CATEGORY_BASS
    if any(k in nl for k in ("pad", "atmo", "ambient", "texture", "drone")):
        return CATEGORY_PAD
    if any(k in nl for k in ("lead", "synth", "arp")):
        return CATEGORY_LEAD
    if any(k in nl for k in ("key", "piano", "rhodes", "organ", "ep")):
        return CATEGORY_KEYS
    if any(k in nl for k in ("pluck", "guitar", "strum", "pick")):
        return CATEGORY_PLUCK
    if any(k in nl for k in ("string", "violin", "cello", "viola")):
        return CATEGORY_STRING
    return CATEGORY_UNKNOWN


def classify_by_audio(channels: List[List[float]], sr: int, duration: float) -> str:
    """Classify by audio characteristics when filename doesn't help.

    Uses ZCR for spectral proxy (low ZCR → low-frequency / bass content,
    high ZCR → noise / hat content) and decay shape to distinguish sustained
    pads from plucked one-shots.
    """
    if not channels or not channels[0]:
        return CATEGORY_UNKNOWN

    mono = channels[0]
    n = len(mono)

    # Zero-crossing rate — cheap spectral proxy used across all duration ranges
    zc = sum(1 for i in range(1, n) if (mono[i] >= 0) != (mono[i - 1] >= 0))
    zcr = zc / n * sr

    # Short = percussive (< 500 ms)
    if duration < 0.5:
        # Low ZCR → mostly low-frequency energy → kick / bass-drum
        # Very high ZCR → mostly high-frequency noise → closed hi-hat
        # Mid ZCR → broadband transient → snare / clap / tom
        if zcr < 500:
            return CATEGORY_KICK
        elif zcr > 3000:
            return CATEGORY_HIHAT_CLOSED
        else:
            return CATEGORY_SNARE

    # Medium = one-shot melodic or long percussion (0.5 s – 3 s)
    if duration < 3.0:
        # Decay shape: compare RMS of first quarter-second vs mid quarter-second
        mid = n // 2
        rms_start = math.sqrt(sum(s * s for s in mono[:sr // 4]) / max(1, sr // 4))
        rms_mid = math.sqrt(sum(s * s for s in mono[mid:mid + sr // 4]) / max(1, sr // 4))
        sustained = rms_mid > rms_start * 0.5

        if sustained:
            # Sustained sound in this range: use ZCR to guess pitch register.
            # Bass fundamentals (< ~200 Hz) cross zero fewer times per second
            # than mid/high melodic material.
            return CATEGORY_BASS if zcr < 200 else CATEGORY_LEAD
        return CATEGORY_PLUCK  # fast-decaying one-shot

    # Long = pad or ambient texture (>= 3 s)
    return CATEGORY_PAD


def classify(samples: List[SampleInfo]) -> List[SampleInfo]:
    """Stage 2: Auto-detect instrument type per sample."""
    for s in samples:
        # Try name-based first
        cat = classify_by_name(s.original_name)
        if cat != CATEGORY_UNKNOWN:
            s.category = cat
            continue

        # Fall back to audio analysis
        try:
            channels, sr, bd = read_wav(s.path)
            dur = len(channels[0]) / sr if channels and channels[0] else 0
            s.category = classify_by_audio(channels, sr, dur)
        except Exception:
            s.category = CATEGORY_UNKNOWN

    # Report distribution
    dist = defaultdict(int)
    for s in samples:
        dist[s.category] += 1
    print(f"  Classification: {dict(dist)}")
    return samples


# ─────────────────────────────────────────────────────────────────────────────
# Stage 3: ANALYZE
# ─────────────────────────────────────────────────────────────────────────────

def analyze(samples: List[SampleInfo], dna_inherit: bool = True) -> List[SampleInfo]:
    """Stage 3: Measure pitch, RMS, peak, DC offset, tail length, loop points.

    When dna_inherit is True, also infers 6D Sonic DNA from each sample's
    spectral characteristics for use in DNA-driven enhancement.
    """
    for s in samples:
        try:
            channels, sr, bd = read_wav(s.path)
        except Exception:
            continue

        s.sample_rate = sr
        s.bit_depth = bd
        s.channels = len(channels)

        if not channels or not channels[0]:
            continue

        mono = channels[0]
        n = len(mono)
        s.duration_s = n / sr

        # RMS and peak
        rms = math.sqrt(sum(x * x for x in mono) / max(1, n))
        peak = max(abs(x) for x in mono) if mono else 0
        s.rms_db = 20 * math.log10(max(rms, 1e-10))
        s.peak_db = 20 * math.log10(max(peak, 1e-10))

        # DC offset
        s.dc_offset = sum(mono) / n if n > 0 else 0

        # Tail length (time from last sample > -60dBFS to end)
        threshold = 10 ** (-60 / 20)
        last_active = n - 1
        for i in range(n - 1, -1, -1):
            if abs(mono[i]) > threshold:
                last_active = i
                break
        s.tail_length_s = (n - last_active) / sr

        # Loop detection for sustained sounds (> 2 seconds)
        if s.duration_s > 2.0 and s.category in MELODIC_CATEGORIES:
            # Find stable region (20%-80% of sample)
            start_idx = int(n * 0.2)
            end_idx = int(n * 0.8)
            # Find zero-crossings near boundaries
            loop_start = start_idx
            for i in range(start_idx, min(start_idx + sr, end_idx)):
                if i > 0 and mono[i] >= 0 > mono[i - 1]:
                    loop_start = i
                    break
            loop_end = end_idx
            for i in range(end_idx, max(end_idx - sr, start_idx), -1):
                if i > 0 and mono[i] >= 0 > mono[i - 1]:
                    loop_end = i
                    break
            if loop_end - loop_start > sr:  # at least 1 second loop
                s.is_loopable = True
                s.loop_start = loop_start
                s.loop_end = loop_end

        # Infer Sonic DNA from spectral characteristics
        if dna_inherit:
            s.dna = infer_dna(channels, sr)

    analyzed = sum(1 for s in samples if s.rms_db > -100)
    loopable = sum(1 for s in samples if s.is_loopable)
    dna_count = sum(1 for s in samples if s.dna is not None)
    print(f"  Analyzed {analyzed} samples, {loopable} loopable")
    if dna_inherit:
        print(f"  DNA inferred for {dna_count} samples")
    return samples


# ─────────────────────────────────────────────────────────────────────────────
# Stage 4: ENHANCE
# ─────────────────────────────────────────────────────────────────────────────

def apply_fade_guards(channels: List[List[float]], sr: int,
                      fade_in_ms: float = 2.0, fade_out_ms: float = 10.0):
    """Apply raised-cosine fade-in and fade-out to prevent clicks."""
    n = len(channels[0])
    fade_in_samples = int(sr * fade_in_ms / 1000)
    fade_out_samples = int(sr * fade_out_ms / 1000)

    for ch in channels:
        # Fade in
        for i in range(min(fade_in_samples, n)):
            gain = 0.5 * (1.0 - math.cos(math.pi * i / fade_in_samples))
            ch[i] *= gain
        # Fade out
        for i in range(min(fade_out_samples, n)):
            gain = 0.5 * (1.0 + math.cos(math.pi * i / fade_out_samples))
            idx = n - 1 - i
            if idx >= 0:
                ch[idx] *= gain


# ─────────────────────────────────────────────────────────────────────────────
# Rebirth Mode — XOmnibus-style DSP Transforms
# ─────────────────────────────────────────────────────────────────────────────

def rebirth_saturate(channels: List[List[float]], sr: int,
                     drive: float = 0.7, mojo: float = 0.5):
    """OBESE-style saturation + mojo chain.
    drive: 0-1 saturation amount
    mojo: 0-1 analog warmth (even harmonics)
    """
    # Map drive from 0-1 to a useful tanh range (1.0 - 8.0)
    d = 1.0 + drive * 7.0
    tanh_d = math.tanh(d)
    for ch in channels:
        for i in range(len(ch)):
            x = ch[i]
            # Normalized soft clip
            sat = math.tanh(d * x) / tanh_d
            # Even harmonic injection via mojo (x^2 preserves sign via abs trick)
            even = mojo * x * abs(x)
            ch[i] = max(-1.0, min(1.0, sat + even))


def rebirth_feedback_swell(channels: List[List[float]], sr: int,
                           swell_time: float = 0.05, feedback: float = 0.6):
    """OUROBOROS-style brief feedback swell on transient.
    Applies a short feedback delay burst at the attack portion.
    """
    swell_samples = int(sr * swell_time)
    # Delay time: ~5ms for metallic character
    delay_samples = max(1, int(sr * 0.005))
    for ch in channels:
        n = len(ch)
        region = min(swell_samples, n)
        # Ring buffer for feedback delay
        buf = [0.0] * delay_samples
        write_pos = 0
        for i in range(region):
            read_pos = write_pos
            delayed = buf[read_pos]
            out = ch[i] + feedback * delayed
            out = max(-1.0, min(1.0, out))
            buf[write_pos] = out
            write_pos = (write_pos + 1) % delay_samples
            # Fade the effect: full at start, zero at swell_samples
            fade = 1.0 - (i / region)
            ch[i] = ch[i] * (1.0 - fade) + out * fade


def rebirth_granular_scatter(channels: List[List[float]], sr: int,
                             grain_size: float = 0.02, scatter: float = 0.3):
    """OPAL-style granular micro-scatter.
    Chops audio into grains and applies micro pitch/time shifts.
    """
    grain_samples = max(1, int(sr * grain_size))
    rng = random.Random(42)
    for ch in channels:
        n = len(ch)
        result = [0.0] * n
        num_grains = (n + grain_samples - 1) // grain_samples
        for g in range(num_grains):
            start = g * grain_samples
            end = min(start + grain_samples, n)
            grain_len = end - start
            if grain_len < 2:
                result[start:end] = ch[start:end]
                continue

            # Extract grain
            grain = ch[start:end]

            # Per-grain pitch shift via resampling (scatter controls range in cents)
            cents = rng.uniform(-scatter * 50, scatter * 50)
            ratio = 2 ** (cents / 1200)
            resampled = []
            for i in range(grain_len):
                src_idx = i * ratio
                idx0 = int(src_idx)
                frac = src_idx - idx0
                if idx0 + 1 < grain_len:
                    resampled.append(grain[idx0] * (1 - frac) + grain[idx0 + 1] * frac)
                elif idx0 < grain_len:
                    resampled.append(grain[idx0])
                else:
                    resampled.append(0.0)

            # Per-grain time offset (shift placement by scatter amount)
            offset = int(rng.uniform(-scatter * grain_samples * 0.3,
                                     scatter * grain_samples * 0.3))
            for i in range(grain_len):
                dest = start + i + offset
                if 0 <= dest < n:
                    # Hann window for smooth overlap
                    w = 0.5 * (1.0 - math.cos(2.0 * math.pi * i / grain_len))
                    if i < len(resampled):
                        result[dest] += resampled[i] * w

        # Replace channel with scattered result
        for i in range(n):
            ch[i] = max(-1.0, min(1.0, result[i]))


def rebirth_fold(channels: List[List[float]], sr: int,
                 fold_point: float = 0.7):
    """ORIGAMI-style wavefolding.
    Folds audio at the fold point for harmonic enrichment.
    """
    fp = max(0.01, fold_point)
    fp4 = 4.0 * fp
    fp2 = 2.0 * fp
    for ch in channels:
        for i in range(len(ch)):
            x = ch[i]
            # Triangle fold: 4*|fmod(x - fp, 4*fp) - 2*fp| - fp
            shifted = x - fp
            # fmod that works for negative numbers
            m = shifted - fp4 * math.floor(shifted / fp4)
            folded = 4.0 * abs(m - fp2) - fp
            ch[i] = max(-1.0, min(1.0, folded))


def rebirth_spring_reverb(channels: List[List[float]], sr: int,
                          decay: float = 0.3, brightness: float = 0.6):
    """OVERDUB-style spring reverb impulse.
    Short, metallic spring reverb character.
    Schroeder reverb: 4 comb filters + 2 allpass filters.
    """
    # Comb filter delay times (in samples) — prime-ish for metallic character
    comb_delays = [int(sr * t) for t in (0.0297, 0.0371, 0.0411, 0.0437)]
    # Allpass delay times
    ap_delays = [int(sr * t) for t in (0.005, 0.0017)]

    # Comb feedback scaled by decay
    comb_fb = 0.7 + 0.25 * decay  # range 0.7 - 0.95

    for ch in channels:
        n = len(ch)
        dry = list(ch)

        # 4 parallel comb filters
        comb_outputs = []
        for delay in comb_delays:
            buf = [0.0] * max(1, delay)
            out = [0.0] * n
            wp = 0
            # One-pole damping filter state
            damp_state = 0.0
            damp_coeff = 1.0 - brightness * 0.7  # higher brightness = less damping
            for i in range(n):
                delayed = buf[wp]
                # Damping filter on feedback path
                damp_state = delayed * (1.0 - damp_coeff) + damp_state * damp_coeff
                if abs(damp_state) < 1e-18:
                    damp_state = 0.0
                buf[wp] = max(-1.0, min(1.0, ch[i] + comb_fb * damp_state))
                out[i] = delayed
                wp = (wp + 1) % max(1, delay)
            comb_outputs.append(out)

        # Mix comb outputs
        wet = [0.0] * n
        for i in range(n):
            wet[i] = sum(c[i] for c in comb_outputs) / len(comb_outputs)

        # 2 series allpass filters
        for delay in ap_delays:
            buf = [0.0] * max(1, delay)
            wp = 0
            ap_g = 0.5
            for i in range(n):
                delayed = buf[wp]
                inp = wet[i]
                ap_state = max(-1.0, min(1.0, inp + ap_g * delayed))
                if abs(ap_state) < 1e-18:
                    ap_state = 0.0
                buf[wp] = ap_state
                wet[i] = delayed - ap_g * inp
                wp = (wp + 1) % max(1, delay)

        # Mix wet back in (the wet/dry is handled by rebirth_transform, but
        # we still apply a gentle mix here to keep levels sane)
        for i in range(n):
            ch[i] = max(-1.0, min(1.0, dry[i] + 0.5 * wet[i]))


# Rebirth engine profiles — each maps to a specific combination of transforms
REBIRTH_PROFILES = {
    "OBESE": {
        "transforms": [("saturate", {"drive": (0.5, 0.9), "mojo": (0.3, 0.8)})],
        "description": "Fat saturation + analog warmth"
    },
    "OUROBOROS": {
        "transforms": [
            ("feedback_swell", {"swell_time": (0.02, 0.08), "feedback": (0.4, 0.8)}),
            ("saturate", {"drive": (0.2, 0.5), "mojo": (0.1, 0.3)})
        ],
        "description": "Feedback swell on transient + light saturation"
    },
    "OPAL": {
        "transforms": [("granular_scatter", {"grain_size": (0.01, 0.04), "scatter": (0.1, 0.5)})],
        "description": "Granular micro-scatter"
    },
    "ORIGAMI": {
        "transforms": [("fold", {"fold_point": (0.5, 0.9)})],
        "description": "Wavefolding harmonic enrichment"
    },
    "OVERDUB": {
        "transforms": [("spring_reverb", {"decay": (0.2, 0.5), "brightness": (0.4, 0.8)})],
        "description": "Spring reverb character"
    },
}

# Map transform names to functions
_REBIRTH_TRANSFORMS = {
    "saturate": rebirth_saturate,
    "feedback_swell": rebirth_feedback_swell,
    "granular_scatter": rebirth_granular_scatter,
    "fold": rebirth_fold,
    "spring_reverb": rebirth_spring_reverb,
}


def rebirth_transform(sample_info, channels: List[List[float]], sr: int,
                       profile_name: str, rng: random.Random,
                       intensity: float = 0.7) -> List[List[float]]:
    """Apply a rebirth engine profile to audio channels.

    1. Looks up the profile
    2. For each transform in the profile, randomizes parameters within the specified ranges
    3. Applies transforms in sequence
    4. Controls wet/dry mix based on intensity (0.0 = original, 1.0 = fully processed)
    """
    profile = REBIRTH_PROFILES.get(profile_name)
    if profile is None:
        print(f"[rebirth] Warning: no profile for engine '{profile_name}' — returning original", file=sys.stderr)
        return channels  # unmodified

    # Keep dry copy for wet/dry blend
    dry = [list(ch) for ch in channels]
    # Work on a copy for wet processing
    wet = [list(ch) for ch in channels]

    for transform_name, param_ranges in profile["transforms"]:
        func = _REBIRTH_TRANSFORMS.get(transform_name)
        if func is None:
            raise ValueError(f"Unknown rebirth transform: {transform_name}")

        # Randomize parameters within specified ranges
        params = {}
        for param_name, (lo, hi) in param_ranges.items():
            params[param_name] = rng.uniform(lo, hi)

        func(wet, sr, **params)

    # Wet/dry blend based on intensity
    n = len(channels[0])
    for ch_idx in range(len(channels)):
        for i in range(n):
            channels[ch_idx][i] = dry[ch_idx][i] * (1.0 - intensity) + wet[ch_idx][i] * intensity

    return channels


def remove_dc_offset(channels: List[List[float]]):
    """Remove DC offset from each channel."""
    for ch in channels:
        if not ch:
            continue
        dc = sum(ch) / len(ch)
        if abs(dc) > 1e-6:
            for i in range(len(ch)):
                ch[i] -= dc


def generate_round_robin(channels: List[List[float]], sr: int,
                         num_variations: int = 4) -> List[List[List[float]]]:
    """Generate round-robin variations via micro-variation."""
    variations = [channels]  # original is variation 0

    # Content-sensitive seed: length alone causes seed collisions for same-duration
    # samples (e.g. all 1-bar loops at the same BPM). Mix in 16 evenly-spaced
    # sample values so each unique waveform gets a unique RNG stream.
    n = len(channels[0]) if channels and channels[0] else 0
    step = max(1, n // 16)
    seed_data = (n,) + tuple(
        round(channels[0][i * step] * 1_000_000)
        for i in range(min(16, n // max(1, step)))
    )
    rng = random.Random(hash(seed_data))

    for v in range(1, num_variations):
        varied = []
        for ch in channels:
            new_ch = list(ch)
            n = len(new_ch)

            # Micro-pitch shift (±5 cents via resampling)
            cents = rng.uniform(-5, 5)
            ratio = 2 ** (cents / 1200)
            if abs(ratio - 1.0) > 0.0001:
                resampled = []
                for i in range(n):
                    src_idx = i * ratio
                    idx0 = int(src_idx)
                    frac = src_idx - idx0
                    if idx0 + 1 < n:
                        resampled.append(new_ch[idx0] * (1 - frac) + new_ch[idx0 + 1] * frac)
                    elif idx0 < n:
                        resampled.append(new_ch[idx0])
                    else:
                        resampled.append(0.0)
                new_ch = resampled[:n]

            # Micro-gain variation (±0.5 dB)
            gain_db = rng.uniform(-0.5, 0.5)
            gain = 10 ** (gain_db / 20)
            new_ch = [s * gain for s in new_ch]

            # Subtle saturation variation
            sat_amount = rng.uniform(0.0, 0.03)
            if sat_amount > 0.01:
                new_ch = [math.tanh(s * (1.0 + sat_amount)) / math.tanh(1.0 + sat_amount)
                          for s in new_ch]

            varied.append(new_ch)
        variations.append(varied)

    return variations


def create_velocity_layers(channels: List[List[float]], sr: int,
                           num_layers: int = 4) -> List[List[List[float]]]:
    """Create velocity layers by shaping amplitude and brightness."""
    layers = []
    for layer_idx in range(num_layers):
        t = layer_idx / max(1, num_layers - 1)  # 0.0 = softest, 1.0 = hardest
        shaped = []
        for ch in channels:
            new_ch = list(ch)
            n = len(new_ch)

            # Amplitude scaling (softer layers are quieter)
            amp = 0.2 + 0.8 * t  # range: 0.2 to 1.0
            new_ch = [s * amp for s in new_ch]

            # Simple brightness shaping (low-pass for soft layers)
            if t < 0.7:
                # One-pole low-pass: softer layers get more filtering
                cutoff_factor = 0.3 + 0.7 * t  # 0.3 to 1.0
                alpha = cutoff_factor  # simplified
                prev = 0.0
                for i in range(n):
                    new_ch[i] = prev + alpha * (new_ch[i] - prev)
                    prev = new_ch[i]

            shaped.append(new_ch)
        layers.append(shaped)
    return layers


def generate_round_robin_dna(channels: List[List[float]], sr: int,
                             num_variations: int = 4,
                             variation_width: float = 1.0,
                             saturation_intensity: float = 0.03) -> List[List[List[float]]]:
    """Generate round-robin variations with DNA-driven parameters.

    variation_width scales the pitch/gain variation range (0.5=subtle, 1.5=wide).
    saturation_intensity controls the max saturation drive per variation.
    """
    variations = [channels]  # original is variation 0

    n = len(channels[0]) if channels and channels[0] else 0
    step = max(1, n // 16)
    seed_data = (n,) + tuple(
        round(channels[0][i * step] * 1_000_000)
        for i in range(min(16, n // max(1, step)))
    )
    rng = random.Random(hash(seed_data))

    for v in range(1, num_variations):
        varied = []
        for ch in channels:
            new_ch = list(ch)
            n = len(new_ch)

            # Micro-pitch shift scaled by variation_width
            cents = rng.uniform(-5 * variation_width, 5 * variation_width)
            ratio = 2 ** (cents / 1200)
            if abs(ratio - 1.0) > 0.0001:
                resampled = []
                for i in range(n):
                    src_idx = i * ratio
                    idx0 = int(src_idx)
                    frac = src_idx - idx0
                    if idx0 + 1 < n:
                        resampled.append(new_ch[idx0] * (1 - frac) + new_ch[idx0 + 1] * frac)
                    elif idx0 < n:
                        resampled.append(new_ch[idx0])
                    else:
                        resampled.append(0.0)
                new_ch = resampled[:n]

            # Micro-gain variation scaled by variation_width
            gain_db = rng.uniform(-0.5 * variation_width, 0.5 * variation_width)
            gain = 10 ** (gain_db / 20)
            new_ch = [s * gain for s in new_ch]

            # Saturation variation driven by DNA intensity
            sat_amount = rng.uniform(0.0, saturation_intensity)
            if sat_amount > 0.005:
                new_ch = [math.tanh(s * (1.0 + sat_amount)) / math.tanh(1.0 + sat_amount)
                          for s in new_ch]

            varied.append(new_ch)
        variations.append(varied)

    return variations


def enhance(samples: List[SampleInfo], work_dir: str,
            num_round_robin: int = 4,
            num_velocity_layers: int = 4,
            dna_inherit: bool = True,
            rebirth: bool = False,
            rebirth_engine: str = "OBESE",
            rebirth_intensity: float = 0.7,
            rebirth_randomize: bool = True) -> Dict[str, dict]:
    """Stage 4: Generate velocity layers, round-robin, apply fade guards + DC removal.

    When dna_inherit is True and samples have inferred DNA, enhancement decisions
    (fade length, saturation, round-robin variation width, loop detection) are
    shaped by the sample's Sonic DNA fingerprint.

    When rebirth is True, samples are reprocessed through XOmnibus-style DSP
    transforms instead of preserving original character. Each round-robin
    variation gets different randomized parameters within the engine profile's
    ranges, creating natural variation like re-recording through analog gear.
    """
    enhanced_dir = os.path.join(work_dir, "enhanced")
    os.makedirs(enhanced_dir, exist_ok=True)

    programs = {}  # program_name → {category, samples: [{path, vel_layer, rr_idx, ...}]}

    for s in tqdm(samples, desc="Enhancing samples", unit="sample"):
        try:
            channels, sr, bd = read_wav(s.path)
        except Exception:
            continue

        base_name = os.path.splitext(s.original_name)[0]
        program_name = base_name[:30]  # Max 30 chars

        # Derive DNA-driven enhancement parameters
        dna_params = None
        if dna_inherit and s.dna is not None:
            dna_params = adjust_enhancement_by_dna(s.dna)

        # Apply fade guards and DC removal to original
        remove_dc_offset(channels)
        fade_out = dna_params.fade_out_ms if dna_params else 10.0
        apply_fade_guards(channels, sr, fade_out_ms=fade_out)

        # Generate velocity layers
        vel_layers = create_velocity_layers(channels, sr, num_velocity_layers)

        # For each velocity layer, generate round-robin variations
        layer_files = []
        for vel_idx, layer_channels in enumerate(vel_layers):
            if rebirth:
                # Rebirth Mode: generate round-robin variations by applying
                # the rebirth engine profile with different randomized params
                rr_variations = []
                for rr_idx in range(num_round_robin):
                    rr_ch = [list(ch) for ch in layer_channels]
                    # Each round-robin gets a unique seed for parameter randomization
                    if rebirth_randomize:
                        seed = hash((base_name, vel_idx, rr_idx))
                    else:
                        seed = hash((base_name, vel_idx, 0))
                    rr_rng = random.Random(seed)
                    rebirth_transform(s, rr_ch, sr, rebirth_engine, rr_rng,
                                      intensity=rebirth_intensity)
                    rr_variations.append(rr_ch)
            elif dna_params:
                rr_variations = generate_round_robin_dna(
                    layer_channels, sr, num_round_robin,
                    variation_width=dna_params.rr_variation_width,
                    saturation_intensity=dna_params.saturation_intensity,
                )
            else:
                rr_variations = generate_round_robin(layer_channels, sr, num_round_robin)
            for rr_idx, rr_channels in enumerate(rr_variations):
                fname = f"{base_name}__v{vel_idx + 1}__c{rr_idx + 1}.wav"
                fpath = os.path.join(enhanced_dir, fname)
                write_wav(fpath, rr_channels, sr, bd)
                layer_files.append({
                    "path": fpath,
                    "filename": fname,
                    "vel_layer": vel_idx,
                    "rr_index": rr_idx,
                    "sample_info": s,
                })

        programs[program_name] = {
            "category": s.category,
            "sample_info": s,
            "layers": layer_files,
            "num_velocity_layers": num_velocity_layers,
            "num_round_robin": num_round_robin,
            "sample_rate": sr,
            "dna": s.dna.to_dict() if s.dna else None,
            "dna_velocity_curve": dna_params.velocity_curve if dna_params else None,
        }

    print(f"  Enhanced {len(programs)} programs ({sum(len(p['layers']) for p in programs.values())} total files)")
    return programs


# ─────────────────────────────────────────────────────────────────────────────
# Stage 5: NORMALIZE
# ─────────────────────────────────────────────────────────────────────────────

def lufs_approximate(channels: List[List[float]], sr: int) -> float:
    """Simplified LUFS measurement (ITU-R BS.1770 approximation).

    Averages all channels before gating — critical for stereo files.
    Channel 0 alone under-reports loudness by ~3 dB on balanced stereo material.
    """
    if not channels or not channels[0]:
        return -100.0

    n = len(channels[0])
    nch = len(channels)
    window = int(0.4 * sr)  # 400ms windows

    # Mix to mono by averaging squared values across all channels (BS.1770 §2.1)
    mixed_sq = [
        sum(channels[ch][i] ** 2 for ch in range(nch)) / nch
        for i in range(n)
    ]

    if n < window:
        ms = sum(mixed_sq) / max(1, n)
        return 10 * math.log10(max(ms, 1e-10)) - 0.691

    # Compute gated mean square over 400ms windows
    powers = []
    for i in range(0, n - window, window // 4):  # 75% overlap
        ms = sum(mixed_sq[i:i + window]) / window
        powers.append(ms)

    if not powers:
        return -100.0

    # Absolute gate at -70 LUFS
    abs_gate = 10 ** ((-70 + 0.691) / 10)
    gated = [p for p in powers if p > abs_gate]
    if not gated:
        return -100.0

    # Relative gate at -10 LUFS from ungated mean
    mean_ungated = sum(gated) / len(gated)
    rel_gate = mean_ungated * 10 ** (-10 / 10)
    final = [p for p in gated if p > rel_gate]
    if not final:
        return -100.0

    mean_final = sum(final) / len(final)
    return 10 * math.log10(max(mean_final, 1e-10)) - 0.691


def normalize_programs(programs: Dict[str, dict], target_lufs: float = -14.0):
    """Stage 5: LUFS normalize all samples in each program."""
    for prog_name, prog in tqdm(programs.items(), desc="Normalizing programs", unit="prog"):
        # Find the loudest layer (highest velocity, first round-robin)
        loudest_layer = None
        for layer in prog["layers"]:
            if layer["rr_index"] == 0 and (loudest_layer is None or
                                            layer["vel_layer"] > loudest_layer["vel_layer"]):
                loudest_layer = layer

        if not loudest_layer:
            continue

        # Measure LUFS of loudest layer
        try:
            channels, sr, bd = read_wav(loudest_layer["path"])
        except Exception:
            continue

        current_lufs = lufs_approximate(channels, sr)
        if current_lufs < -80:
            continue  # too quiet, skip

        gain_db = target_lufs - current_lufs
        gain_db = max(-24, min(24, gain_db))  # safety clamp
        gain = 10 ** (gain_db / 20)

        # Apply same gain to ALL layers in this program (preserves velocity dynamics)
        for layer in prog["layers"]:
            try:
                ch, sr2, bd2 = read_wav(layer["path"])
                for c in ch:
                    for i in range(len(c)):
                        c[i] = max(-1.0, min(1.0, c[i] * gain))
                write_wav(layer["path"], ch, sr2, bd2)
            except Exception:
                continue

    normalized = sum(1 for p in programs.values() if p.get("layers"))
    print(f"  Normalized {normalized} programs to {target_lufs} LUFS")


# ─────────────────────────────────────────────────────────────────────────────
# Stage 6: MAP — Build XPM Programs
# ─────────────────────────────────────────────────────────────────────────────

def build_xpm_drum(prog_name: str, prog: dict,
                   velocity_curve: str = "musical") -> str:
    """Build an MPC drum program XPM for a percussion sample."""
    curve = VELOCITY_CURVES.get(velocity_curve, VELOCITY_CURVES["musical"])
    si = prog["sample_info"]
    note = GM_DRUM_MAP.get(si.category, 36)

    root = ET.Element("MPCVObject", type="com.akaipro.mpc.drum.program")
    ET.SubElement(root, "Version").text = "1.7"
    ET.SubElement(root, "ProgramName").text = prog_name

    instruments = ET.SubElement(root, "Instruments")
    inst = ET.SubElement(instruments, "Instrument", index="0")
    ET.SubElement(inst, "Note").text = str(note)
    ET.SubElement(inst, "TriggerMode").text = "OneShot"
    ET.SubElement(inst, "PadColor").text = DEFAULT_PAD_COLOR

    # Aftertouch → filter
    at = ET.SubElement(inst, "AfterTouch")
    ET.SubElement(at, "Destination").text = "FilterCutoff"
    ET.SubElement(at, "Amount").text = "30"

    # Mute groups for hats
    if si.category == CATEGORY_HIHAT_CLOSED:
        ET.SubElement(inst, "MuteGroup").text = "1"
    elif si.category == CATEGORY_HIHAT_OPEN:
        ET.SubElement(inst, "MuteGroup").text = "1"

    # Velocity layers
    layers = ET.SubElement(inst, "Layers")
    num_vel = prog["num_velocity_layers"]
    num_rr = prog["num_round_robin"]

    for vel_idx in range(num_vel):
        vs, ve, vol = curve[vel_idx] if vel_idx < len(curve) else (1, 127, 1.0)

        # Get round-robin files for this velocity layer
        rr_files = [l for l in prog["layers"]
                     if l["vel_layer"] == vel_idx]

        if num_rr > 1 and len(rr_files) > 1:
            # Round-robin: all files share full velocity range
            for rr_idx, rr_file in enumerate(rr_files):
                layer = ET.SubElement(layers, "Layer",
                                     index=str(vel_idx * num_rr + rr_idx))
                ET.SubElement(layer, "SampleName").text = rr_file["filename"]
                ET.SubElement(layer, "VelStart").text = str(vs)
                ET.SubElement(layer, "VelEnd").text = str(ve)
                ET.SubElement(layer, "Volume").text = f"{vol:.2f}"
                ET.SubElement(layer, "RootNote").text = "0"
                ET.SubElement(layer, "KeyTrack").text = "True"
                ET.SubElement(layer, "CycleType").text = "RoundRobin"
                ET.SubElement(layer, "CycleGroup").text = str(vel_idx + 1)
        else:
            layer = ET.SubElement(layers, "Layer", index=str(vel_idx))
            first_file = rr_files[0] if rr_files else prog["layers"][0]
            ET.SubElement(layer, "SampleName").text = first_file["filename"]
            ET.SubElement(layer, "VelStart").text = str(vs)
            ET.SubElement(layer, "VelEnd").text = str(ve)
            ET.SubElement(layer, "Volume").text = f"{vol:.2f}"
            ET.SubElement(layer, "RootNote").text = "0"
            ET.SubElement(layer, "KeyTrack").text = "True"

    # Pad empty layers
    total_layers = len(list(layers))
    for i in range(total_layers, 4):
        empty = ET.SubElement(layers, "Layer", index=str(i))
        ET.SubElement(empty, "SampleName").text = ""
        ET.SubElement(empty, "VelStart").text = "0"
        ET.SubElement(empty, "VelEnd").text = "0"

    ET.indent(root, space="  ")
    return ET.tostring(root, encoding="unicode", xml_declaration=True)


def build_xpm_keygroup(prog_name: str, prog: dict,
                       velocity_curve: str = "musical") -> str:
    """Build an MPC keygroup program XPM for a melodic sample."""
    curve = VELOCITY_CURVES.get(velocity_curve, VELOCITY_CURVES["musical"])
    si = prog["sample_info"]

    root = ET.Element("MPCVObject", type="com.akaipro.mpc.keygroup.program")
    ET.SubElement(root, "Version").text = "1.7"
    ET.SubElement(root, "ProgramName").text = prog_name

    # Expression mapping
    at = ET.SubElement(root, "AfterTouch")
    ET.SubElement(at, "Destination").text = "FilterCutoff"
    ET.SubElement(at, "Amount").text = "50"
    mw = ET.SubElement(root, "ModWheel")
    ET.SubElement(mw, "Destination").text = "FilterCutoff"
    ET.SubElement(mw, "Amount").text = "70"
    ET.SubElement(root, "PitchBendRange").text = "12"

    keygroups = ET.SubElement(root, "Keygroups")
    kg = ET.SubElement(keygroups, "Keygroup", index="0")
    ET.SubElement(kg, "LowNote").text = "0"
    ET.SubElement(kg, "HighNote").text = "127"

    layers = ET.SubElement(kg, "Layers")
    num_vel = prog["num_velocity_layers"]

    for vel_idx in range(num_vel):
        vs, ve, vol = curve[vel_idx] if vel_idx < len(curve) else (1, 127, 1.0)
        rr_files = [l for l in prog["layers"] if l["vel_layer"] == vel_idx]

        for rr_idx, rr_file in enumerate(rr_files):
            layer = ET.SubElement(layers, "Layer",
                                 index=str(vel_idx * len(rr_files) + rr_idx))
            ET.SubElement(layer, "SampleName").text = rr_file["filename"]
            ET.SubElement(layer, "VelStart").text = str(vs)
            ET.SubElement(layer, "VelEnd").text = str(ve)
            ET.SubElement(layer, "Volume").text = f"{vol:.2f}"
            ET.SubElement(layer, "RootNote").text = "0"
            ET.SubElement(layer, "KeyTrack").text = "True"
            ET.SubElement(layer, "TuneCoarse").text = "0"
            ET.SubElement(layer, "TuneFine").text = "0"

            if si.is_loopable:
                ET.SubElement(layer, "LoopStart").text = str(si.loop_start)
                ET.SubElement(layer, "LoopEnd").text = str(si.loop_end)
                ET.SubElement(layer, "LoopCrossfade").text = str(LOOP_CROSSFADE_SAMPLES)
            else:
                ET.SubElement(layer, "LoopStart").text = "-1"
                ET.SubElement(layer, "LoopEnd").text = "-1"

            if len(rr_files) > 1:
                ET.SubElement(layer, "CycleType").text = "RoundRobin"
                ET.SubElement(layer, "CycleGroup").text = str(vel_idx + 1)

    ET.indent(root, space="  ")
    return ET.tostring(root, encoding="unicode", xml_declaration=True)


def build_programs(programs: Dict[str, dict], work_dir: str,
                   velocity_curve: str = "musical",
                   dna_inherit: bool = True) -> str:
    """Stage 6: Build XPM programs for all enhanced samples.

    When dna_inherit is True, uses the DNA-selected velocity curve per program
    instead of the global velocity_curve argument.
    """
    xpm_dir = os.path.join(work_dir, "programs")
    os.makedirs(xpm_dir, exist_ok=True)

    for prog_name, prog in programs.items():
        si = prog["sample_info"]
        # Use DNA-selected velocity curve if available, else global
        curve = velocity_curve
        if dna_inherit and prog.get("dna_velocity_curve"):
            curve = prog["dna_velocity_curve"]

        if si.category in DRUM_CATEGORIES:
            xpm_content = build_xpm_drum(prog_name, prog, curve)
        else:
            xpm_content = build_xpm_keygroup(prog_name, prog, curve)

        xpm_path = os.path.join(xpm_dir, f"{prog_name}.xpm")
        with open(xpm_path, "w", encoding="utf-8") as f:
            f.write(xpm_content)
        prog["xpm_path"] = xpm_path

    print(f"  Built {len(programs)} XPM programs")
    return xpm_dir


# ─────────────────────────────────────────────────────────────────────────────
# DNA → .xometa export
# ─────────────────────────────────────────────────────────────────────────────

def write_xometa_files(programs: Dict[str, dict], work_dir: str) -> List[str]:
    """Write .xometa preset files with inferred Sonic DNA for each program.

    Returns a list of written .xometa file paths.
    """
    xometa_dir = os.path.join(work_dir, "presets")
    os.makedirs(xometa_dir, exist_ok=True)
    written = []

    for prog_name, prog in programs.items():
        dna_dict = prog.get("dna")
        si = prog.get("sample_info")
        if si is None:
            continue

        preset = {
            "format": "xometa",
            "version": 1,
            "name": prog_name,
            "category": si.category if si else "unknown",
            "source": "XOutshine",
            "sample_rate": prog.get("sample_rate", 44100),
            "velocity_layers": prog.get("num_velocity_layers", 4),
            "round_robin": prog.get("num_round_robin", 4),
        }

        if dna_dict:
            preset["dna"] = dna_dict

        if prog.get("dna_velocity_curve"):
            preset["velocity_curve"] = prog["dna_velocity_curve"]

        xometa_path = os.path.join(xometa_dir, f"{prog_name}.xometa")
        with open(xometa_path, "w", encoding="utf-8") as f:
            json.dump(preset, f, indent=2)
        written.append(xometa_path)
        prog["xometa_path"] = xometa_path

    if written:
        print(f"  Wrote {len(written)} .xometa preset files with DNA")
    return written


# ─────────────────────────────────────────────────────────────────────────────
# Stage 7: PACKAGE
# ─────────────────────────────────────────────────────────────────────────────

def write_manifest(work_dir: str, pack_name: str, num_programs: int) -> str:
    """Write XPN manifest file."""
    manifest_path = os.path.join(work_dir, "Manifest.xml")
    root = ET.Element("Expansion")
    ET.SubElement(root, "Name").text = pack_name
    ET.SubElement(root, "Author").text = "XOutshine by XO_OX Designs"
    ET.SubElement(root, "Version").text = "1.0"
    ET.SubElement(root, "Description").text = f"Upgraded by XOutshine — {num_programs} programs"
    ET.SubElement(root, "ProgramCount").text = str(num_programs)
    ET.indent(root, space="  ")
    content = ET.tostring(root, encoding="unicode", xml_declaration=True)
    with open(manifest_path, "w", encoding="utf-8") as f:
        f.write(content)
    return manifest_path


def package_xpn(programs: Dict[str, dict], work_dir: str,
                output_path: str, pack_name: str):
    """Stage 7: Assemble XPN archive."""
    manifest = write_manifest(work_dir, pack_name, len(programs))

    with zipfile.ZipFile(output_path, "w", zipfile.ZIP_DEFLATED) as zf:
        # Manifest
        zf.write(manifest, "Expansions/Manifest.xml")

        # Programs, presets, and samples
        for prog_name, prog in programs.items():
            if "xpm_path" in prog:
                zf.write(prog["xpm_path"], f"Programs/{prog_name}.xpm")
            if "xometa_path" in prog:
                zf.write(prog["xometa_path"], f"Presets/{prog_name}.xometa")
            for layer in prog["layers"]:
                zf.write(layer["path"], f"Samples/{prog_name}/{layer['filename']}")

    size_mb = os.path.getsize(output_path) / (1024 * 1024)
    print(f"  Packaged: {output_path} ({size_mb:.1f} MB)")


def package_folder(programs: Dict[str, dict], work_dir: str,
                   output_path: str, pack_name: str):
    """Stage 7 (alt): Assemble organized WAV folder with XPMs."""
    os.makedirs(output_path, exist_ok=True)

    manifest = write_manifest(work_dir, pack_name, len(programs))
    shutil.copy2(manifest, os.path.join(output_path, "Manifest.xml"))

    programs_dir = os.path.join(output_path, "Programs")
    samples_dir = os.path.join(output_path, "Samples")
    os.makedirs(programs_dir, exist_ok=True)

    presets_dir = os.path.join(output_path, "Presets")

    for prog_name, prog in programs.items():
        if "xpm_path" in prog:
            shutil.copy2(prog["xpm_path"], os.path.join(programs_dir, f"{prog_name}.xpm"))

        if "xometa_path" in prog:
            os.makedirs(presets_dir, exist_ok=True)
            shutil.copy2(prog["xometa_path"], os.path.join(presets_dir, f"{prog_name}.xometa"))

        prog_samples_dir = os.path.join(samples_dir, prog_name)
        os.makedirs(prog_samples_dir, exist_ok=True)
        for layer in prog["layers"]:
            shutil.copy2(layer["path"], os.path.join(prog_samples_dir, layer["filename"]))

    total_files = sum(len(p["layers"]) for p in programs.values())
    print(f"  Organized: {output_path} ({len(programs)} programs, {total_files} files)")


# ─────────────────────────────────────────────────────────────────────────────
# Stage 8: VALIDATE
# ─────────────────────────────────────────────────────────────────────────────

def validate(programs: Dict[str, dict], output_path: str) -> int:
    """Stage 8: Run QA checks on output."""
    issues = 0

    for prog_name, prog in programs.items():
        for layer in prog["layers"]:
            try:
                channels, sr, bd = read_wav(layer["path"])
            except Exception:
                print(f"    FAIL: Cannot read {layer['filename']}")
                issues += 1
                continue

            mono = channels[0] if channels else []
            if not mono:
                issues += 1
                continue

            # Check peak
            peak = max(abs(s) for s in mono)
            if peak >= 1.0:
                print(f"    WARN: Clipping in {layer['filename']} (peak={peak:.4f})")
                issues += 1

            # Check DC offset
            dc = abs(sum(mono) / len(mono))
            if dc > 0.005:
                print(f"    WARN: DC offset in {layer['filename']} ({dc:.4f})")
                issues += 1

            # Check fade guards (first 10 samples should be near zero)
            if len(mono) > 10 and abs(mono[0]) > 0.01:
                print(f"    WARN: No fade-in on {layer['filename']}")
                issues += 1

    score = max(0, 100 - issues * 5)
    status = "PASS" if score >= 80 else "WARN" if score >= 50 else "FAIL"
    print(f"  QA: {status} (score {score}/100, {issues} issues)")
    return issues


# ─────────────────────────────────────────────────────────────────────────────
# Main Pipeline
# ─────────────────────────────────────────────────────────────────────────────

_STAGE_MARKER_PREFIX = ".xoutshine_stage_"


def _stage_done(work_dir: str, stage: int) -> bool:
    return os.path.exists(os.path.join(work_dir, f"{_STAGE_MARKER_PREFIX}{stage}"))


def _mark_stage(work_dir: str, stage: int):
    open(os.path.join(work_dir, f"{_STAGE_MARKER_PREFIX}{stage}"), "w").close()


def run_pipeline(args):
    """Run the full XOutshine pipeline."""
    print(BANNER)

    pack_name = args.name or os.path.splitext(os.path.basename(args.input))[0]
    output_format = args.format or ("xpn" if args.output.lower().endswith(".xpn") else "folder")
    dry_run = getattr(args, "dry_run", False)
    resume = getattr(args, "resume", False)
    dna_inherit = getattr(args, "dna_inherit", True)

    # Rebirth Mode
    rebirth = getattr(args, "rebirth", False)
    rebirth_engine = getattr(args, "rebirth_engine", "OBESE")
    rebirth_intensity = getattr(args, "rebirth_intensity", 0.7)
    rebirth_randomize = getattr(args, "rebirth_randomize", True)

    if rebirth:
        profile = REBIRTH_PROFILES.get(rebirth_engine)
        profile_desc = profile["description"] if profile else "unknown"
        print(f"  REBIRTH MODE: engine={rebirth_engine} ({profile_desc})")
        print(f"  intensity={rebirth_intensity:.1f}, randomize={rebirth_randomize}")

    # Work directory: persistent if --work-dir specified, otherwise temporary
    explicit_work_dir = getattr(args, "work_dir", None)
    if explicit_work_dir:
        os.makedirs(explicit_work_dir, exist_ok=True)
        work_dir = explicit_work_dir
        _cleanup = False
    else:
        _tmp = tempfile.mkdtemp(prefix="xoutshine_")
        work_dir = _tmp
        _cleanup = True

    try:
        # ── Stage 1: INGEST ──────────────────────────────────────────────────
        print("[1/8] INGEST")
        state_path = os.path.join(work_dir, "samples_state.json")
        if resume and _stage_done(work_dir, 1) and os.path.exists(state_path):
            print("  Resuming: stage 1 already complete")
            with open(state_path) as f:
                state = json.load(f)
            samples = [SampleInfo(s["path"], s["original_name"]) for s in state]
            for s_obj, s_dict in zip(samples, state):
                s_obj.__dict__.update(s_dict)
        else:
            samples = ingest(args.input, work_dir)
            if not samples:
                print("  ERROR: No samples found.")
                return 1
            _mark_stage(work_dir, 1)

        if dry_run:
            print("\n[DRY RUN] Classification preview (stages 2–3 only):")
            samples = classify(samples)
            samples = analyze(samples, dna_inherit=dna_inherit)
            total_dur = sum(s.duration_s for s in samples)
            # Estimate: 4 vel layers × 4 RR × ~WAV size per file
            est_files = len(samples) * args.velocity_layers * args.round_robin
            # Rough size: avg duration × sr × channels × bytes (16-bit) with 0.8× compression
            avg_dur = total_dur / max(1, len(samples))
            est_bytes = est_files * avg_dur * 44100 * 2 * 3 * 0.8
            est_mb = est_bytes / (1024 * 1024)
            print(f"\n  Would process : {len(samples)} samples → {est_files} output files")
            print(f"  Est. pack size: ~{est_mb:.0f} MB")
            dist = defaultdict(int)
            for s in samples:
                dist[s.category] += 1
            print(f"  Category dist : {dict(dist)}")
            return 0

        # ── Stage 2: CLASSIFY ────────────────────────────────────────────────
        print("[2/8] CLASSIFY")
        if resume and _stage_done(work_dir, 2):
            print("  Resuming: stage 2 already complete")
        else:
            samples = classify(samples)
            _mark_stage(work_dir, 2)

        # ── Stage 3: ANALYZE ─────────────────────────────────────────────────
        print("[3/8] ANALYZE")
        if resume and _stage_done(work_dir, 3):
            print("  Resuming: stage 3 already complete")
        else:
            samples = analyze(samples, dna_inherit=dna_inherit)
            _mark_stage(work_dir, 3)

        # ── Stage 4: ENHANCE ─────────────────────────────────────────────────
        print("[4/8] ENHANCE")
        programs_path = os.path.join(work_dir, "programs_state.json")
        if resume and _stage_done(work_dir, 4) and os.path.exists(programs_path):
            print("  Resuming: stage 4 already complete")
            with open(programs_path) as f:
                programs = json.load(f)
        else:
            programs = enhance(samples, work_dir,
                               num_round_robin=args.round_robin,
                               num_velocity_layers=args.velocity_layers,
                               dna_inherit=dna_inherit,
                               rebirth=rebirth,
                               rebirth_engine=rebirth_engine,
                               rebirth_intensity=rebirth_intensity,
                               rebirth_randomize=rebirth_randomize)
            _mark_stage(work_dir, 4)

        # ── Stage 5: NORMALIZE ───────────────────────────────────────────────
        print("[5/8] NORMALIZE")
        if resume and _stage_done(work_dir, 5):
            print("  Resuming: stage 5 already complete")
        else:
            normalize_programs(programs, target_lufs=args.lufs_target)
            _mark_stage(work_dir, 5)

        # ── Stage 6: MAP ─────────────────────────────────────────────────────
        print("[6/8] MAP")
        if resume and _stage_done(work_dir, 6):
            print("  Resuming: stage 6 already complete")
        else:
            build_programs(programs, work_dir, velocity_curve=args.velocity_curve,
                          dna_inherit=dna_inherit)
            _mark_stage(work_dir, 6)

        # ── Write .xometa preset files with DNA ────────────────────────────
        if dna_inherit:
            write_xometa_files(programs, work_dir)

        # ── Stage 7: PACKAGE ─────────────────────────────────────────────────
        print("[7/8] PACKAGE")
        if output_format == "xpn":
            package_xpn(programs, work_dir, args.output, pack_name)
        else:
            package_folder(programs, work_dir, args.output, pack_name)

        # ── Stage 8: VALIDATE ────────────────────────────────────────────────
        print("[8/8] VALIDATE")
        issues = validate(programs, args.output)

    finally:
        if _cleanup:
            shutil.rmtree(work_dir, ignore_errors=True)

    print(f"\nXOutshine complete. {len(programs)} programs upgraded.")
    print("Outshine the original. ✧")
    return 0 if issues == 0 else 1


def main():
    parser = argparse.ArgumentParser(
        description="XOutshine — Sample Pack Upgrade Pipeline. Outshine the original.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  python xoutshine.py --input pack.xpn --output upgraded.xpn
  python xoutshine.py --input ./samples/ --output ./upgraded/ --format folder
  python xoutshine.py --input pack.xpn --output upgraded.xpn --velocity-curve boom-bap
  python xoutshine.py --input ./drums/ --output drums.xpn --round-robin 4 --velocity-layers 5

  # Preview what would be processed (no files written)
  python xoutshine.py --input ./samples/ --output out.xpn --dry-run

  # Resume an interrupted run
  python xoutshine.py --input pack.xpn --output upgraded.xpn --work-dir /tmp/xo_work --resume

  # Rebirth Mode — reprocess through OBESE saturation engine
  python xoutshine.py --input pack.xpn --output reborn.xpn --rebirth

  # Rebirth with ORIGAMI wavefolding at full intensity
  python xoutshine.py --input pack.xpn --output reborn.xpn --rebirth --rebirth-engine ORIGAMI --rebirth-intensity 1.0

  # Rebirth with consistent (non-randomized) round-robin params
  python xoutshine.py --input pack.xpn --output reborn.xpn --rebirth --no-rebirth-randomize

Velocity curves: musical (default), boom-bap, neo-soul, trap-hard, linear
Rebirth engines: OBESE, OUROBOROS, OPAL, ORIGAMI, OVERDUB
        """)

    parser.add_argument("--input", "-i", required=True,
                        help="Input .xpn file or WAV folder")
    parser.add_argument("--output", "-o", required=True,
                        help="Output .xpn file or folder path")
    parser.add_argument("--format", "-f", choices=["xpn", "folder"],
                        help="Output format (default: auto-detect from output path)")
    parser.add_argument("--name", "-n",
                        help="Pack name (default: derived from input)")
    parser.add_argument("--velocity-layers", type=int, default=4,
                        help="Number of velocity layers (default: 4)")
    parser.add_argument("--round-robin", type=int, default=4,
                        help="Number of round-robin variations (default: 4)")
    parser.add_argument("--velocity-curve", default="musical",
                        choices=list(VELOCITY_CURVES.keys()),
                        help="Velocity curve style (default: musical)")
    parser.add_argument("--lufs-target", type=float, default=-14.0,
                        help="Target LUFS level (default: -14)")
    parser.add_argument("--no-round-robin", action="store_true",
                        help="Disable round-robin generation")
    parser.add_argument("--dry-run", action="store_true",
                        help="Preview classification and estimated output size without processing")
    parser.add_argument("--resume", action="store_true",
                        help="Resume an interrupted run (requires --work-dir)")
    parser.add_argument("--work-dir",
                        help="Persistent working directory for resume support "
                             "(default: auto-managed temp dir, deleted on completion)")
    parser.add_argument("--dna-inherit", action="store_true", default=True,
                        dest="dna_inherit",
                        help="Enable DNA Inheritance: infer 6D Sonic DNA from audio and "
                             "use it to shape enhancement decisions (default: enabled)")
    parser.add_argument("--no-dna-inherit", action="store_false", dest="dna_inherit",
                        help="Disable DNA Inheritance (use generic enhancement)")
    parser.add_argument("--version", action="version", version=f"XOutshine {VERSION}")

    # Rebirth Mode flags
    parser.add_argument("--rebirth", action="store_true",
                        help="Enable Rebirth Mode: reprocess samples through XOmnibus-style "
                             "DSP transforms instead of preserving original character")
    parser.add_argument("--rebirth-engine", default="OBESE",
                        choices=list(REBIRTH_PROFILES.keys()),
                        help="Which engine profile to apply in Rebirth Mode (default: OBESE)")
    parser.add_argument("--rebirth-intensity", type=float, default=0.7,
                        help="Rebirth wet/dry blend: 0.0 = original, 1.0 = fully processed "
                             "(default: 0.7)")
    parser.add_argument("--rebirth-randomize", action=argparse.BooleanOptionalAction,
                        default=True,
                        help="Randomize transform parameters per round-robin variation "
                             "(default: enabled)")

    args = parser.parse_args()

    if args.no_round_robin:
        args.round_robin = 1

    return run_pipeline(args)


if __name__ == "__main__":
    sys.exit(main())
