#!/usr/bin/env python3
"""
XOsmosis — Sample Pack Upgrade Pipeline
XO_OX Designs

Quality that seeps in.

Takes any sample pack (XPN archive, WAV folder, or loose samples) and
transforms it into a production-quality, expressive, dynamic MPC instrument.

The XO standard permeates through:
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
    python xosmosis.py --input pack.xpn --output upgraded_pack.xpn

    # Upgrade a WAV folder
    python xosmosis.py --input ./my_samples/ --output ./my_pack.xpn

    # Upgrade a WAV folder → organized folder (not zipped)
    python xosmosis.py --input ./my_samples/ --output ./my_pack/ --format folder

    # Upgrade with specific options
    python xosmosis.py --input pack.xpn --output upgraded.xpn \
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

XO_OX Designs — Quality that seeps in.
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
from typing import Dict, List, Optional, Tuple

# ─────────────────────────────────────────────────────────────────────────────
# Constants
# ─────────────────────────────────────────────────────────────────────────────

VERSION = "1.0.0"
BANNER = """
╔═══════════════════════════════════════════════════════════════╗
║                                                               ║
║    ██╗  ██╗ ██████╗ ███████╗███╗   ███╗ ██████╗ ███████╗    ║
║    ╚██╗██╔╝██╔═══██╗██╔════╝████╗ ████║██╔═══██╗██╔════╝    ║
║     ╚███╔╝ ██║   ██║███████╗██╔████╔██║██║   ██║███████╗    ║
║     ██╔██╗ ██║   ██║╚════██║██║╚██╔╝██║██║   ██║╚════██║    ║
║    ██╔╝ ██╗╚██████╔╝███████║██║ ╚═╝ ██║╚██████╔╝███████║    ║
║    ╚═╝  ╚═╝ ╚═════╝ ╚══════╝╚═╝     ╚═╝ ╚═════╝ ╚══════╝    ║
║                                                               ║
║              Quality that seeps in.  v{ver}                ║
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
    """Classify by audio characteristics when filename doesn't help."""
    if not channels or not channels[0]:
        return CATEGORY_UNKNOWN

    mono = channels[0]
    n = len(mono)

    # Short = percussive
    if duration < 0.5:
        # Analyze spectral content roughly via zero-crossing rate
        zc = sum(1 for i in range(1, n) if (mono[i] >= 0) != (mono[i - 1] >= 0))
        zcr = zc / n * sr
        if zcr < 500:
            return CATEGORY_KICK
        elif zcr > 3000:
            return CATEGORY_HIHAT_CLOSED
        else:
            return CATEGORY_SNARE

    # Medium = melodic one-shot or percussion
    if duration < 3.0:
        # Check for sustained energy (pad vs pluck)
        mid = n // 2
        rms_start = math.sqrt(sum(s * s for s in mono[:sr // 4]) / max(1, sr // 4))
        rms_mid = math.sqrt(sum(s * s for s in mono[mid:mid + sr // 4]) / max(1, sr // 4))
        if rms_mid > rms_start * 0.5:
            return CATEGORY_BASS  # sustained
        return CATEGORY_PLUCK  # decaying

    # Long = pad or texture
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

def analyze(samples: List[SampleInfo]) -> List[SampleInfo]:
    """Stage 3: Measure pitch, RMS, peak, DC offset, tail length, loop points."""
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

    analyzed = sum(1 for s in samples if s.rms_db > -100)
    loopable = sum(1 for s in samples if s.is_loopable)
    print(f"  Analyzed {analyzed} samples, {loopable} loopable")
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
    rng = random.Random(hash(str(len(channels[0]))))

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


def enhance(samples: List[SampleInfo], work_dir: str,
            num_round_robin: int = 4,
            num_velocity_layers: int = 4) -> Dict[str, dict]:
    """Stage 4: Generate velocity layers, round-robin, apply fade guards + DC removal."""
    enhanced_dir = os.path.join(work_dir, "enhanced")
    os.makedirs(enhanced_dir, exist_ok=True)

    programs = {}  # program_name → {category, samples: [{path, vel_layer, rr_idx, ...}]}

    for s in samples:
        try:
            channels, sr, bd = read_wav(s.path)
        except Exception:
            continue

        base_name = os.path.splitext(s.original_name)[0]
        program_name = base_name[:30]  # Max 30 chars

        # Apply fade guards and DC removal to original
        remove_dc_offset(channels)
        apply_fade_guards(channels, sr)

        # Generate velocity layers
        vel_layers = create_velocity_layers(channels, sr, num_velocity_layers)

        # For each velocity layer, generate round-robin variations
        layer_files = []
        for vel_idx, layer_channels in enumerate(vel_layers):
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
        }

    print(f"  Enhanced {len(programs)} programs ({sum(len(p['layers']) for p in programs.values())} total files)")
    return programs


# ─────────────────────────────────────────────────────────────────────────────
# Stage 5: NORMALIZE
# ─────────────────────────────────────────────────────────────────────────────

def lufs_approximate(channels: List[List[float]], sr: int) -> float:
    """Simplified LUFS measurement (ITU-R BS.1770 approximation)."""
    if not channels or not channels[0]:
        return -100.0

    mono = channels[0]
    n = len(mono)
    window = int(0.4 * sr)  # 400ms windows

    if n < window:
        rms = math.sqrt(sum(s * s for s in mono) / max(1, n))
        return 20 * math.log10(max(rms, 1e-10)) - 0.691

    # Compute gated mean square over 400ms windows
    powers = []
    for i in range(0, n - window, window // 4):  # 75% overlap
        chunk = mono[i:i + window]
        ms = sum(s * s for s in chunk) / window
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
    for prog_name, prog in programs.items():
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
                ET.SubElement(layer, "LoopCrossfade").text = "100"
            else:
                ET.SubElement(layer, "LoopStart").text = "-1"
                ET.SubElement(layer, "LoopEnd").text = "-1"

            if len(rr_files) > 1:
                ET.SubElement(layer, "CycleType").text = "RoundRobin"
                ET.SubElement(layer, "CycleGroup").text = str(vel_idx + 1)

    ET.indent(root, space="  ")
    return ET.tostring(root, encoding="unicode", xml_declaration=True)


def build_programs(programs: Dict[str, dict], work_dir: str,
                   velocity_curve: str = "musical") -> str:
    """Stage 6: Build XPM programs for all enhanced samples."""
    xpm_dir = os.path.join(work_dir, "programs")
    os.makedirs(xpm_dir, exist_ok=True)

    for prog_name, prog in programs.items():
        si = prog["sample_info"]
        if si.category in DRUM_CATEGORIES:
            xpm_content = build_xpm_drum(prog_name, prog, velocity_curve)
        else:
            xpm_content = build_xpm_keygroup(prog_name, prog, velocity_curve)

        xpm_path = os.path.join(xpm_dir, f"{prog_name}.xpm")
        with open(xpm_path, "w", encoding="utf-8") as f:
            f.write(xpm_content)
        prog["xpm_path"] = xpm_path

    print(f"  Built {len(programs)} XPM programs")
    return xpm_dir


# ─────────────────────────────────────────────────────────────────────────────
# Stage 7: PACKAGE
# ─────────────────────────────────────────────────────────────────────────────

def write_manifest(work_dir: str, pack_name: str, num_programs: int) -> str:
    """Write XPN manifest file."""
    manifest_path = os.path.join(work_dir, "Manifest.xml")
    root = ET.Element("Expansion")
    ET.SubElement(root, "Name").text = pack_name
    ET.SubElement(root, "Author").text = "XOsmosis by XO_OX Designs"
    ET.SubElement(root, "Version").text = "1.0"
    ET.SubElement(root, "Description").text = f"Upgraded by XOsmosis — {num_programs} programs"
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

        # Programs and samples
        for prog_name, prog in programs.items():
            if "xpm_path" in prog:
                zf.write(prog["xpm_path"], f"Programs/{prog_name}.xpm")
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

    for prog_name, prog in programs.items():
        if "xpm_path" in prog:
            shutil.copy2(prog["xpm_path"], os.path.join(programs_dir, f"{prog_name}.xpm"))

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

def run_pipeline(args):
    """Run the full XOsmosis pipeline."""
    print(BANNER)

    pack_name = args.name or os.path.splitext(os.path.basename(args.input))[0]
    output_format = args.format or ("xpn" if args.output.lower().endswith(".xpn") else "folder")

    with tempfile.TemporaryDirectory(prefix="xosmosis_") as work_dir:
        # Stage 1: INGEST
        print("[1/8] INGEST")
        samples = ingest(args.input, work_dir)
        if not samples:
            print("  ERROR: No samples found.")
            return 1

        # Stage 2: CLASSIFY
        print("[2/8] CLASSIFY")
        samples = classify(samples)

        # Stage 3: ANALYZE
        print("[3/8] ANALYZE")
        samples = analyze(samples)

        # Stage 4: ENHANCE
        print("[4/8] ENHANCE")
        programs = enhance(samples, work_dir,
                          num_round_robin=args.round_robin,
                          num_velocity_layers=args.velocity_layers)

        # Stage 5: NORMALIZE
        print("[5/8] NORMALIZE")
        normalize_programs(programs, target_lufs=args.lufs_target)

        # Stage 6: MAP
        print("[6/8] MAP")
        build_programs(programs, work_dir, velocity_curve=args.velocity_curve)

        # Stage 7: PACKAGE
        print("[7/8] PACKAGE")
        if output_format == "xpn":
            package_xpn(programs, work_dir, args.output, pack_name)
        else:
            package_folder(programs, work_dir, args.output, pack_name)

        # Stage 8: VALIDATE
        print("[8/8] VALIDATE")
        issues = validate(programs, args.output)

    print(f"\nXOsmosis complete. {len(programs)} programs upgraded.")
    print("Quality that seeps in. ✧")
    return 0 if issues == 0 else 1


def main():
    parser = argparse.ArgumentParser(
        description="XOsmosis — Sample Pack Upgrade Pipeline. Quality that seeps in.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  python xosmosis.py --input pack.xpn --output upgraded.xpn
  python xosmosis.py --input ./samples/ --output ./upgraded/ --format folder
  python xosmosis.py --input pack.xpn --output upgraded.xpn --velocity-curve boom-bap
  python xosmosis.py --input ./drums/ --output drums.xpn --round-robin 4 --velocity-layers 5

Velocity curves: musical (default), boom-bap, neo-soul, trap-hard, linear
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
    parser.add_argument("--version", action="version", version=f"XOsmosis {VERSION}")

    args = parser.parse_args()

    if args.no_round_robin:
        args.round_robin = 1

    return run_pipeline(args)


if __name__ == "__main__":
    sys.exit(main())
