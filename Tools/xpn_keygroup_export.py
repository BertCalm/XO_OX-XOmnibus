#!/usr/bin/env python3
"""
XPN Keygroup Exporter — XO_OX Designs
Generates Akai MPC Keygroup XPM programs from XO_OX WAV sample sets.

WAV naming convention expected:
    {preset_slug}__{NOTE}__{vel}.WAV
    e.g. Deep_Drift__C2__v1.WAV
         Deep_Drift__C2__v2.WAV   (velocity layer 2)
         Deep_Drift__Eb3__v1.WAV

NOTE format: note name + octave, e.g. C2, F#3, Bb4, Eb2
vel: v1–v4 (velocity layers, ascending velocity)

Round-robin (opt-in via --round-robin or cycle/random naming):
    {preset_slug}__{NOTE}__c1.WAV  (round-robin take 1)
    {preset_slug}__{NOTE}__c2.WAV  (round-robin take 2)

Release samples (opt-in via --release-layer):
    {preset_slug}__{NOTE}__rel.WAV

Velocity layer ranges (Ghost Council Modified standard, adopted 2026-04-04):
    v1 → 1–20    (Ghost)   VelStart=1 not 0 (Rex Rule #3)
    v2 → 21–55   (Light)
    v3 → 56–90   (Medium)
    v4 → 91–127  (Hard)
(Old even-split values were v1:1-31, v2:32-63, v3:64-95, v4:96-127)

XPM Rules (from CLAUDE.md — never break these):
    - KeyTrack  = True   (samples transpose across zones)
    - RootNote  = 0      (MPC auto-detect convention)
    - VelStart  = 0      on empty/unused layers (prevents ghost triggering)

Usage (standalone):
    python3 xpn_keygroup_export.py --wavs-dir /path/to/wavs \\
        --preset "Deep Drift" --engine Odyssey --output /path/to/out

    # With loop detection (sustained instruments)
    python3 xpn_keygroup_export.py --wavs-dir /path/to/wavs \\
        --preset "Deep Drift" --engine Odyssey --output /path/to/out \\
        --loop-detect --instrument-family strings

    # With round-robin support
    python3 xpn_keygroup_export.py --wavs-dir /path/to/wavs \\
        --preset "Deep Drift" --engine Odyssey --output /path/to/out \\
        --round-robin

    # With DNA-adaptive velocity curves (reads preset .xometa)
    python3 xpn_keygroup_export.py --wavs-dir /path/to/wavs \\
        --preset "Deep Drift" --engine Odyssey --output /path/to/out \\
        --dna-adaptive

    # With release layer
    python3 xpn_keygroup_export.py --wavs-dir /path/to/wavs \\
        --preset "Deep Drift" --engine Odyssey --output /path/to/out \\
        --release-layer

    # Zone strategy
    python3 xpn_keygroup_export.py --wavs-dir /path/to/wavs \\
        --preset "Deep Drift" --engine Odyssey --output /path/to/out \\
        --zone-strategy every-third

Usage (library):
    from xpn_keygroup_export import build_keygroup_wav_map, generate_keygroup_xpm
"""

import argparse
import re
import struct
import sys
from pathlib import Path
from typing import Dict, List, Optional, Tuple
from xml.sax.saxutils import escape as xml_escape

from xpn_velocity_standard import ZONES as _STANDARD_ZONES

REPO_ROOT   = Path(__file__).parent.parent
PRESETS_DIR = REPO_ROOT / "Presets" / "XOceanus"


# ---------------------------------------------------------------------------
# Note name → MIDI number
# ---------------------------------------------------------------------------

_NOTE_NAMES = {"C": 0, "D": 2, "E": 4, "F": 5, "G": 7, "A": 9, "B": 11}
_SHARP_FLAT = {"#": 1, "b": -1, "s": 1}  # "s" as fallback for Fs, Bs etc.

_NOTE_RE = re.compile(r'^([A-Ga-g])([#bs]?)(-?\d+)$')


def note_name_to_midi(note_str: str) -> Optional[int]:
    """Convert a note name like 'C2', 'F#3', 'Bb4' to MIDI number (C-1=0)."""
    m = _NOTE_RE.match(note_str.strip())
    if not m:
        return None
    letter, acc, octave = m.group(1).upper(), m.group(2), int(m.group(3))
    semitone = _NOTE_NAMES[letter] + _SHARP_FLAT.get(acc, 0)
    return (octave + 1) * 12 + semitone


def midi_to_note_name(midi: int) -> str:
    """Convert MIDI number to note name (C-1=0 convention)."""
    names = ["C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"]
    octave = (midi // 12) - 1
    note = names[midi % 12]
    return f"{note}{octave}"


# ---------------------------------------------------------------------------
# Velocity layer definitions (instrument-family-aware)
# ---------------------------------------------------------------------------

# Ghost Council Modified standard zones — derived from xpn_velocity_standard.
# Old VEL_LAYERS_EVEN was v1:1-31, v2:32-63, v3:64-95, v4:96-127 (replaced 2026-04-04).
VEL_LAYERS_EVEN = {
    "v1": (_STANDARD_ZONES[0][0], _STANDARD_ZONES[0][1]),   # Ghost:  1–20
    "v2": (_STANDARD_ZONES[1][0], _STANDARD_ZONES[1][1]),   # Light: 21–55
    "v3": (_STANDARD_ZONES[2][0], _STANDARD_ZONES[2][1]),   # Medium: 56–90
    "v4": (_STANDARD_ZONES[3][0], _STANDARD_ZONES[3][1]),   # Hard:  91–127
}

# Ghost Council Modified musical curve — wider mid range, narrow ghost.
# Old values were (1,20), (21,50), (51,90), (91,127) — zone 2 end 50→55,
# zone 3 start 51→56 (replaced 2026-04-04).
VEL_LAYERS_MUSICAL = [
    (_STANDARD_ZONES[0][0], _STANDARD_ZONES[0][1], 0.30),   # Ghost  (1–20,  15%)
    (_STANDARD_ZONES[1][0], _STANDARD_ZONES[1][1], 0.55),   # Light  (21–55, 26%)
    (_STANDARD_ZONES[2][0], _STANDARD_ZONES[2][1], 0.75),   # Medium (56–90, 27%)
    (_STANDARD_ZONES[3][0], _STANDARD_ZONES[3][1], 0.95),   # Hard   (91–127, 29%)
]

# --- Instrument-family velocity curves ---
# Each tuple: (vel_start, vel_end, volume, character_label)
# Character labels document the sonic intent per family.

# Piano: hammer weight — gentle touches to FFF hammering
VEL_LAYERS_PIANO = [
    (1,   20, 0.28, "pianissimo — barely touching the keys"),
    (21,  55, 0.52, "mezzo-piano — gentle, expressive"),
    (56,  95, 0.76, "forte — energetic, bright attack"),
    (96, 127, 0.96, "fortissimo — full hammer weight"),
]

# Strings: bow pressure — sul tasto through col legno
VEL_LAYERS_STRINGS = [
    (1,   25, 0.32, "sul tasto — gentle near fingerboard"),
    (26,  60, 0.58, "normale — standard bowing"),
    (61,  95, 0.80, "sul ponticello — bright, intense near bridge"),
    (96, 127, 0.97, "col legno — wood-of-bow, percussive"),
]

# Brass: lip tension — warm to split tone
VEL_LAYERS_BRASS = [
    (1,   22, 0.30, "warm — relaxed embouchure, soft tone"),
    (23,  58, 0.56, "bright — centered, focused projection"),
    (59,  92, 0.78, "blaring — high pressure, forward edge"),
    (93, 127, 0.97, "split tone — extreme lip tension, multiphonic"),
]

# Woodwind: breath pressure — gentle through overblown
VEL_LAYERS_WOODWIND = [
    (1,   20, 0.30, "gentle — little breath, soft tone"),
    (21,  55, 0.57, "full — centered, resonant tone"),
    (56,  90, 0.78, "pushed — aggressive, slightly edged"),
    (91, 127, 0.97, "overblown — extreme pressure, harsh edge"),
]

# World instruments: linear, even splits (musical default)
VEL_LAYERS_WORLD = [
    (1,   25, 0.30, "ghost — lightest touch"),
    (26,  60, 0.56, "light — expressive playing"),
    (61,  95, 0.78, "medium — full engagement"),
    (96, 127, 0.97, "hard — maximum attack"),
]

# Organ: velocity insensitive (no layer variation), but keep structure
VEL_LAYERS_ORGAN = [
    (1,   20, 0.60, "soft drawbar — quieter registration"),
    (21,  55, 0.72, "medium drawbar — standard registration"),
    (56,  90, 0.85, "full drawbar — all stops out"),
    (91, 127, 0.97, "full + overdrive — Leslie fast + drive"),
]

# Pads/atmospheric: smooth crossfades, wide gentle zones
VEL_LAYERS_PADS = [
    (1,   30, 0.35, "whisper — barely there"),
    (31,  65, 0.60, "present — comfortable bed"),
    (66,  98, 0.82, "prominent — forward texture"),
    (99, 127, 0.98, "full — complete saturation"),
]

# Bass: punchy bottom end
VEL_LAYERS_BASS = [
    (1,   18, 0.28, "ghost — barely audible pop"),
    (19,  52, 0.54, "soft — fingerstyle gentle"),
    (53,  90, 0.76, "medium — punchy fingerstyle"),
    (91, 127, 0.97, "hard — slap / plectrum full force"),
]

# Map family name → velocity layer table
FAMILY_VEL_LAYERS: Dict[str, List[tuple]] = {
    "piano":     VEL_LAYERS_PIANO,
    "strings":   VEL_LAYERS_STRINGS,
    "brass":     VEL_LAYERS_BRASS,
    "woodwind":  VEL_LAYERS_WOODWIND,
    "world":     VEL_LAYERS_WORLD,
    "organ":     VEL_LAYERS_ORGAN,
    "pads":      VEL_LAYERS_PADS,
    "bass":      VEL_LAYERS_BASS,
    "default":   VEL_LAYERS_MUSICAL,
}

# Standard 4-layer fallback (backward compat, simple tuples without label).
# Old values were v1:1-31, v2:32-63, v3:64-95, v4:96-127 (replaced 2026-04-04).
VEL_LAYERS = {
    "v1": (_STANDARD_ZONES[0][0], _STANDARD_ZONES[0][1]),   # Ghost:  1–20
    "v2": (_STANDARD_ZONES[1][0], _STANDARD_ZONES[1][1]),   # Light: 21–55
    "v3": (_STANDARD_ZONES[2][0], _STANDARD_ZONES[2][1]),   # Medium: 56–90
    "v4": (_STANDARD_ZONES[3][0], _STANDARD_ZONES[3][1]),   # Hard:  91–127
}

# Single-layer fallback (no velocity switching)
VEL_SINGLE = {"v1": (1, 127)}


def _vel_range(vel_tag: str, all_vel_tags: List[str]) -> Tuple[int, int]:
    """Return (vel_start, vel_end) for a velocity tag given its peers."""
    if vel_tag in VEL_LAYERS and len(all_vel_tags) > 1:
        return VEL_LAYERS[vel_tag]
    return (0, 127)


def _family_vel_layers(instrument_family: str) -> List[tuple]:
    """Return (vel_start, vel_end, volume) list for a given instrument family."""
    family = (instrument_family or "default").lower()
    table = FAMILY_VEL_LAYERS.get(family, VEL_LAYERS_MUSICAL)
    # Normalize: each entry may have 3 or 4 elements — return (vs, ve, vol) triples
    return [(row[0], row[1], row[2]) for row in table]


# ---------------------------------------------------------------------------
# Velocity crossfade
# ---------------------------------------------------------------------------
#
# Hard velocity boundaries cause audible "switching" between layers instead of
# smooth morphing. On MPC hardware, when adjacent layers overlap in velocity
# range, both layers sound simultaneously at complementary volumes in the
# overlap zone — producing a natural timbral crossfade.
#
# Only applied for instrument families where timbral crossfade matters.
# Drums keep hard splits because transient character changes are intentionally
# abrupt — crossfading would blur the attack.
# ---------------------------------------------------------------------------

# Families that benefit from velocity crossfade (timbral variation across layers)
_CROSSFADE_FAMILIES = {"piano", "strings", "brass", "pads", "woodwind", "organ", "world", "bass", "default"}


def _apply_vel_crossfade(
    layers: List[Tuple[int, int, float]],
    crossfade: int,
    instrument_family: str = "default",
) -> List[Tuple[int, int, float]]:
    """
    Apply velocity crossfade overlap to adjacent velocity layers.

    When crossfade > 0, adjacent layers overlap by that many velocity units.
    In the overlap zone both layers sound at complementary volumes on MPC,
    producing a smooth morph instead of an abrupt timbral switch.

    Example with crossfade=3 on Ghost Council Modified 4-layer zones:
        Before: v1: 1-20,  v2: 21-55,  v3: 56-90,  v4: 91-127
        After:  v1: 1-23,  v2: 18-58,  v3: 53-93,  v4: 88-127

    Drums/percussion always get hard splits (no crossfade applied).

    Args:
        layers: list of (vel_start, vel_end, volume) tuples
        crossfade: velocity units to overlap on each side of a boundary
        instrument_family: family name — drums skip crossfade regardless
    """
    family = (instrument_family or "default").lower()

    # Drums and percussive families: hard splits by design
    if family in ("drums", "percussion") or family not in _CROSSFADE_FAMILIES:
        return list(layers)

    if crossfade <= 0 or len(layers) < 2:
        return list(layers)

    result: List[Tuple[int, int, float]] = []
    for i, (vs, ve, vol) in enumerate(layers):
        new_vs = vs
        new_ve = ve

        # Extend start downward into previous layer's territory
        if i > 0:
            new_vs = max(1, vs - crossfade)

        # Extend end upward into next layer's territory
        if i < len(layers) - 1:
            new_ve = min(127, ve + crossfade)

        result.append((new_vs, new_ve, vol))

    return result


# ---------------------------------------------------------------------------
# DNA-Adaptive Velocity Curves (ported from xpn_drum_export.py)
# ---------------------------------------------------------------------------
#
# Sonic DNA (6D) shapes the velocity response per preset:
#   aggression  → shifts split points downward (hotter curve)
#   warmth      → softens crossovers between adjacent layers
#   density     → compresses/expands volume scaling across layers
#   brightness  → scales VelocitySensitivity for transient-prominent voices
#
# All modulation is bounded to produce valid 1-127 ranges.

_DEFAULT_DNA = {
    "brightness": 0.5, "warmth": 0.5, "movement": 0.5,
    "density": 0.5, "space": 0.5, "aggression": 0.5,
}


def _dna_adapt_velocity_layers(dna: dict,
                                base_family: str = "default") -> List[Tuple[int, int, float]]:
    """
    Return a DNA-modified velocity layer list: [(vel_start, vel_end, volume), ...].

    Starts from the instrument-family base curve and applies DNA-driven adjustments.
    Returns a new list — never mutates the module-level constants.
    """
    aggression = dna.get("aggression", 0.5)
    warmth     = dna.get("warmth", 0.5)
    density    = dna.get("density", 0.5)

    base = _family_vel_layers(base_family)
    # Extract boundaries: [vs0, ve0, ve1, ve2, ve3] → 5 boundary points
    # boundaries[0]=vs[0], boundaries[1..4]=ve[0..3]
    boundaries = [base[0][0]] + [row[1] for row in base]   # 5 values

    # --- Aggression: shift split points downward (hotter curve) ---
    aggr_bias = (aggression - 0.5) * 2.0   # -1.0 .. +1.0
    shift_amounts = [
        int(round(aggr_bias * -4)),   # ghost/light boundary
        int(round(aggr_bias * -6)),   # light/mid boundary
        int(round(aggr_bias * -8)),   # mid/hard boundary
    ]
    adjusted = list(boundaries)
    for i, shift in enumerate(shift_amounts):
        adjusted[i + 1] = boundaries[i + 1] + shift

    # Clamp to maintain ordering with minimum 4-wide bands
    min_width = 4
    for i in range(1, 4):
        adjusted[i] = max(adjusted[i], adjusted[i - 1] + min_width)
    for i in range(3, 0, -1):
        adjusted[i] = min(adjusted[i], adjusted[i + 1] - min_width)

    # --- Warmth: widen crossover zone in the expressive mid band ---
    warmth_nudge = int(round((warmth - 0.5) * 4))   # -2 .. +2
    adjusted[2] = max(adjusted[1] + min_width,
                      min(adjusted[2] - warmth_nudge, adjusted[3] - min_width))
    adjusted[3] = max(adjusted[2] + min_width,
                      min(adjusted[3] + warmth_nudge, adjusted[4] - min_width))

    # --- Density: compress/expand volume scaling ---
    base_vols = [row[2] for row in base]
    density_bias = (density - 0.5) * 2.0   # -1.0 .. +1.0
    center_vol = 0.65
    adapted_vols = []
    for v in base_vols:
        compressed = center_vol + (v - center_vol) * (1.0 - density_bias * 0.3)
        adapted_vols.append(max(0.10, min(0.99, compressed)))

    # Build final layers
    layers = []
    for i in range(4):
        vs = adjusted[i] if i == 0 else adjusted[i] + 1
        ve = adjusted[i + 1]
        layers.append((vs, ve, adapted_vols[i]))

    return layers


def _load_preset_dna(preset_name: str) -> dict:
    """
    Load the 6D Sonic DNA from a preset's .xometa file.
    Returns _DEFAULT_DNA if the preset or DNA is not found.
    """
    import json as _json
    for xmeta in PRESETS_DIR.rglob("*.xometa"):
        try:
            with open(xmeta) as f:
                data = _json.load(f)
            if data.get("name") == preset_name:
                dna = data.get("dna", {})
                if dna:
                    return {**_DEFAULT_DNA, **dna}
        except (KeyError, _json.JSONDecodeError, OSError):
            continue
    return dict(_DEFAULT_DNA)


# ---------------------------------------------------------------------------
# WAV analysis helpers
# ---------------------------------------------------------------------------

def _read_wav_samples(wav_path: Path) -> Optional[Tuple[int, int, List[float]]]:
    """
    Read a WAV file and return (sample_rate, num_frames, samples_normalized).
    Returns None if the file cannot be read or is not a simple PCM WAV.
    Supports 16-bit and 24-bit PCM only.
    """
    try:
        data = wav_path.read_bytes()
        if data[:4] != b"RIFF" or data[8:12] != b"WAVE":
            return None

        offset = 12
        sample_rate = channels = bits = num_frames = 0
        fmt_found = False
        audio_bytes = b""

        while offset + 8 <= len(data):
            chunk_id   = data[offset:offset + 4]
            chunk_size = struct.unpack_from("<I", data, offset + 4)[0]
            chunk_data = data[offset + 8: offset + 8 + chunk_size]
            offset += 8 + chunk_size

            if chunk_id == b"fmt ":
                fmt_found = True
                audio_format = struct.unpack_from("<H", chunk_data, 0)[0]
                if audio_format not in (1, 3):   # 1=PCM, 3=float
                    return None
                channels    = struct.unpack_from("<H", chunk_data, 2)[0]
                sample_rate = struct.unpack_from("<I", chunk_data, 4)[0]
                bits        = struct.unpack_from("<H", chunk_data, 14)[0]
            elif chunk_id == b"data":
                audio_bytes = chunk_data

        if not fmt_found or not audio_bytes or channels < 1 or sample_rate == 0:
            return None

        # Convert to mono float samples
        if bits == 16:
            n_samples = len(audio_bytes) // 2
            raw = struct.unpack_from(f"<{n_samples}h", audio_bytes)
            samples = [s / 32768.0 for s in raw]
        elif bits == 24:
            raw = []
            for i in range(0, len(audio_bytes) - 2, 3):
                val = audio_bytes[i] | (audio_bytes[i + 1] << 8) | (audio_bytes[i + 2] << 16)
                if val & 0x800000:
                    val -= 0x1000000
                raw.append(val)
            samples = [s / 8388608.0 for s in raw]
        else:
            return None

        # Mix down to mono if stereo
        if channels > 1:
            mono = []
            for i in range(0, len(samples) - channels + 1, channels):
                mono.append(sum(samples[i:i + channels]) / channels)
            samples = mono

        num_frames = len(samples)
        return sample_rate, num_frames, samples
    except Exception:
        return None


def _is_percussive(samples: List[float], sample_rate: int) -> bool:
    """
    Heuristic: if the signal decays to < 10% of peak within 500ms, treat as percussive.
    Sustained instruments (strings, pads, organs) will hold energy much longer.
    """
    if not samples:
        return True
    peak = max(abs(s) for s in samples)
    if peak < 1e-6:
        return True
    threshold = peak * 0.10
    decay_frame = int(sample_rate * 0.5)   # 500ms
    if len(samples) <= decay_frame:
        return True
    rms_after = (sum(s * s for s in samples[decay_frame:decay_frame + sample_rate // 4]) /
                 (sample_rate // 4)) ** 0.5
    return rms_after < threshold


def _find_zero_crossing(samples: List[float], start: int,
                        direction: int = 1, window: int = 512) -> int:
    """
    Search for a zero-crossing near `start` within `window` samples.
    direction: +1 searches forward, -1 searches backward.
    Returns the closest zero-crossing frame, or `start` if none found.
    """
    best = start
    for offset in range(window):
        i = start + offset * direction
        if i <= 0 or i >= len(samples) - 1:
            break
        if samples[i] * samples[i + 1] <= 0:
            # Found sign change — pick the sample closer to zero
            best = i if abs(samples[i]) < abs(samples[i + 1]) else i + 1
            break
    return best


def detect_loop_points(wav_path: Path,
                       xfade_ms: int = 100) -> Optional[Tuple[bool, int, int, int]]:
    """
    Analyze a WAV file and return (should_loop, loop_start, loop_end, xfade_samples).

    Should_loop is True for sustained instruments, False for percussive ones.
    Loop points are placed in the stable sustain region with zero-crossing alignment.
    Returns None if the file cannot be analyzed.

    xfade_ms: crossfade length in milliseconds (default 100ms, range 50-200ms).
    """
    result = _read_wav_samples(wav_path)
    if result is None:
        return None
    sample_rate, num_frames, samples = result

    percussive = _is_percussive(samples, sample_rate)
    if percussive:
        return (False, 0, 0, 0)

    # Sustained: find the stable middle third of the sample as the sustain region
    # Avoid the attack (first 10%) and the release tail (last 20%)
    attack_end  = max(int(num_frames * 0.10), int(sample_rate * 0.05))
    release_start = int(num_frames * 0.80)
    sustain_center = (attack_end + release_start) // 2

    # Loop region: 200ms–2000ms of sustain material
    min_loop_frames = int(sample_rate * 0.20)
    max_loop_frames = int(sample_rate * 2.00)
    half_loop = min(max_loop_frames // 2,
                    max(min_loop_frames // 2, (release_start - attack_end) // 3))

    raw_start = sustain_center - half_loop
    raw_end   = sustain_center + half_loop

    # Clamp within safe bounds
    raw_start = max(raw_start, attack_end)
    raw_end   = min(raw_end, release_start)
    if raw_end - raw_start < min_loop_frames:
        raw_end = raw_start + min_loop_frames

    # Align to zero crossings for click-free looping
    loop_start = _find_zero_crossing(samples, raw_start, direction=1)
    loop_end   = _find_zero_crossing(samples, raw_end,   direction=-1)

    if loop_end <= loop_start:
        loop_end = raw_end

    # Crossfade length — clamp between 50ms and 200ms
    xfade_ms   = max(50, min(200, xfade_ms))
    xfade_samp = int(sample_rate * xfade_ms / 1000)

    return (True, loop_start, loop_end, xfade_samp)


# ---------------------------------------------------------------------------
# WAV map builder
# ---------------------------------------------------------------------------

def build_keygroup_wav_map(wavs_dir: Path, preset_slug: str) -> Dict[str, str]:
    """
    Scan wavs_dir for files matching {preset_slug}__*__*.WAV.
    Returns {stem: filename} mapping.

    stem format: {preset_slug}__{NOTE}__{vel}
    Supports velocity layers (v1-v4), round-robin (c1-c4), release (rel).
    """
    wav_map: Dict[str, str] = {}
    pattern = f"{preset_slug}__*.WAV"
    for wf in sorted(wavs_dir.glob(pattern)):
        wav_map[wf.stem] = wf.name
    # Also accept lowercase .wav
    for wf in sorted(wavs_dir.glob(f"{preset_slug}__*.wav")):
        if wf.stem not in wav_map:
            wav_map[wf.stem] = wf.name
    return wav_map


def _parse_stem(stem: str) -> Optional[Tuple[str, int, str]]:
    """
    Parse a WAV stem like 'Deep_Drift__C2__v1' or 'Deep_Drift__C2__rel'.
    Returns (note_str, midi_num, vel_tag) or None if unparseable.
    vel_tag may be: v1-v4, c1-c4, rel
    """
    parts = stem.split("__")
    if len(parts) < 3:
        return None
    note_str = parts[-2]
    vel_tag  = parts[-1]
    midi = note_name_to_midi(note_str)
    if midi is None:
        # Try interpreting as a raw MIDI number
        try:
            midi = int(note_str)
            note_str = midi_to_note_name(midi)
        except ValueError:
            return None
    return (note_str, midi, vel_tag)


# ---------------------------------------------------------------------------
# Zone layout
# ---------------------------------------------------------------------------

def _compute_zones(
    root_midi_notes: List[int],
    full_range: Tuple[int, int] = (0, 127),
    strategy: str = "midpoint",
) -> List[Tuple[int, int, int]]:
    """
    Given a sorted list of root MIDI notes, compute (low, root, high) zone
    triples that span [full_range[0], full_range[1]] with no gaps.

    strategy:
        "midpoint"    — zone boundary is midpoint between adjacent roots (default)
        "chromatic"   — each root gets exactly 1 semitone (unisons only; usually wrong)
        "every-third" — zones are sized for samples taken every 3 semitones
        "per-octave"  — zones cover one octave each (12 semitones)
    """
    if not root_midi_notes:
        return []

    notes = sorted(set(root_midi_notes))

    if strategy == "chromatic":
        zones = []
        lo = full_range[0]
        for i, root in enumerate(notes):
            hi = root if i < len(notes) - 1 else full_range[1]
            zones.append((lo, root, hi))
            lo = hi + 1
        return zones

    if strategy == "every-third":
        # Each sample covers ±1 semitone around root
        zones = []
        lo = full_range[0]
        for i, root in enumerate(notes):
            hi = (root + 1) if i < len(notes) - 1 else full_range[1]
            zones.append((lo, root, hi))
            lo = hi + 1
        return zones

    if strategy == "per-octave":
        zones = []
        lo = full_range[0]
        for i, root in enumerate(notes):
            if i < len(notes) - 1:
                hi = notes[i + 1] - 1
            else:
                hi = full_range[1]
            zones.append((lo, root, hi))
            lo = hi + 1
        return zones

    # Default: midpoint strategy
    zones = []
    lo = full_range[0]
    for i, root in enumerate(notes):
        if i < len(notes) - 1:
            next_root = notes[i + 1]
            hi = (root + next_root) // 2
        else:
            hi = full_range[1]
        zones.append((lo, root, hi))
        lo = hi + 1
    return zones


# ---------------------------------------------------------------------------
# XPM XML generation
# ---------------------------------------------------------------------------

_XPM_HEADER = """<?xml version="1.0" encoding="UTF-8"?>
<MPCVObject>
  <Version>
    <File_Version>1.7</File_Version>
    <Application>MPC-V</Application>
    <Application_Version>2.10.0.0</Application_Version>
    <Platform>OSX</Platform>
  </Version>"""


def _fmt(v: float) -> str:
    return f"{v:.6f}"


def generate_keygroup_xpm(
    preset_name: str,
    engine: str,
    wav_map: Dict[str, str],
    instrument_family: str = "default",
    loop_detect: bool = False,
    wavs_dir: Optional[Path] = None,
    round_robin: bool = False,
    release_layer: bool = False,
    zone_strategy: str = "midpoint",
    dna_adaptive: bool = False,
    dna: Optional[dict] = None,
    vel_curve: str = "musical",
    vel_crossfade: int = 3,
) -> str:
    """
    Generate a complete Keygroup XPM from a wav_map.

    wav_map: {stem: filename} as returned by build_keygroup_wav_map()

    Parameters (all opt-in):
        instrument_family: piano/strings/brass/woodwind/world/organ/pads/bass/default
        loop_detect:       analyze WAV waveforms to detect loop points
        wavs_dir:          path to WAV files (required if loop_detect=True)
        round_robin:       enable round-robin (c1-c4 suffix) layer support
        release_layer:     enable release trigger layer (rel suffix)
        zone_strategy:     midpoint/chromatic/every-third/per-octave
        dna_adaptive:      apply DNA-adaptive velocity curves
        dna:               Sonic DNA dict (loaded from .xometa if None + dna_adaptive)
        vel_curve:         "musical" (Vibe curve) or "even" (equal splits)
        vel_crossfade:     velocity units to overlap between adjacent layers (default 3).
                           Set to 0 for hard splits (--vel-crossfade 0). Only applied for
                           timbral families (piano, strings, brass, pads, etc.) — drums
                           always use hard splits.
                           MIGRATION NOTE (2026-03-18, C-KAI-001): Default changed from 0
                           to 3. Existing scripts that relied on hard velocity splits will
                           now produce overlapping layers for timbral instruments. Pass
                           --vel-crossfade 0 to restore previous behavior.

    If wav_map is empty, returns a single-instrument program with no layers
    that MPC can load (though it will be silent until WAVs are present).
    """
    prog_name   = xml_escape(f"XO_OX-{engine}-{preset_name}"[:32])
    preset_slug = preset_name.replace(" ", "_")

    # --- Determine velocity layers to use ---
    if dna_adaptive:
        if dna is None:
            dna = _load_preset_dna(preset_name)
        active_vel_layers = _dna_adapt_velocity_layers(dna, base_family=instrument_family)
    else:
        active_vel_layers = _family_vel_layers(instrument_family)

    # Even curve override (backward compat).
    # Old values were (1,31),(32,63),(64,95),(96,127) — updated 2026-04-04.
    if vel_curve == "even" and not dna_adaptive:
        active_vel_layers = [
            (_STANDARD_ZONES[0][0], _STANDARD_ZONES[0][1], 0.55),
            (_STANDARD_ZONES[1][0], _STANDARD_ZONES[1][1], 0.70),
            (_STANDARD_ZONES[2][0], _STANDARD_ZONES[2][1], 0.85),
            (_STANDARD_ZONES[3][0], _STANDARD_ZONES[3][1], 0.97),
        ]

    # --- Apply velocity crossfade (overlap adjacent layers for smooth morphing) ---
    active_vel_layers = _apply_vel_crossfade(
        active_vel_layers, vel_crossfade, instrument_family
    )

    # --- CycleType constants ---
    CYCLE_TYPE_ROUNDROBIN = "RoundRobin"

    # --- Parse all stems ---
    # Group by MIDI note: {midi: {vel_tag: (stem, filename)}}
    by_note: Dict[int, Dict[str, Tuple[str, str]]] = {}
    for stem, filename in wav_map.items():
        parsed = _parse_stem(stem)
        if not parsed:
            continue
        _, midi, vel_tag = parsed
        if midi not in by_note:
            by_note[midi] = {}
        by_note[midi][vel_tag] = (stem, filename)

    # Separate release samples out of main vel layers
    release_by_note: Dict[int, Tuple[str, str]] = {}
    if release_layer:
        for midi, layers in by_note.items():
            if "rel" in layers:
                release_by_note[midi] = layers.pop("rel")

    # --- Detect round-robin vs velocity mode per note ---
    # Round-robin notes use c1/c2/c3/c4 tags; velocity notes use v1/v2/v3/v4
    def _is_rr_note(vel_tags: List[str]) -> bool:
        return round_robin and any(t.startswith("c") for t in vel_tags)

    # --- Compute zones ---
    root_notes = sorted(by_note.keys())
    zones = _compute_zones(root_notes, strategy=zone_strategy) if root_notes else [(0, 60, 127)]

    # --- Build instrument XML ---
    instruments_xml = ""
    num_instruments = 0
    inst_idx = 0

    for low, root, high in zones:
        vel_layers_here = by_note.get(root, {})
        all_vel_tags     = sorted(vel_layers_here.keys())
        has_samples      = bool(vel_layers_here)
        is_rr            = _is_rr_note(all_vel_tags)

        # --- Loop point detection ---
        loop_str         = "False"
        loop_start_val   = 0
        loop_end_val     = 0
        loop_xfade_val   = 0

        if loop_detect and has_samples and wavs_dir is not None:
            # Use the first available sample for this root note
            first_tag = all_vel_tags[0] if all_vel_tags else None
            if first_tag:
                first_filename = vel_layers_here[first_tag][1]
                wav_path = wavs_dir / first_filename
                if wav_path.exists():
                    lp = detect_loop_points(wav_path)
                    if lp is not None:
                        should_loop, ls, le, lx = lp
                        if should_loop:
                            loop_str       = "True"
                            loop_start_val = ls
                            loop_end_val   = le
                            loop_xfade_val = lx

        layers_xml = ""

        if has_samples:
            if is_rr:
                # Round-robin: all c1-c4 share same velocity range (full 1-127),
                # differ by CycleType/CycleGroup to enable round-robin playback.
                rr_tags = sorted(t for t in all_vel_tags if t.startswith("c"))
                rr_group = inst_idx + 1   # unique group per zone
                for layer_num, rr_tag in enumerate(rr_tags, start=1):
                    stem, filename = vel_layers_here[rr_tag]
                    file_path = f"Samples/{preset_slug}/{filename}" if filename else ""
                    layers_xml += (
                        f"            <Layer number=\"{layer_num}\">\n"
                        f"              <Active>True</Active>\n"
                        f"              <Volume>{_fmt(0.707946)}</Volume>\n"
                        f"              <Pan>0.500000</Pan>\n"
                        f"              <Pitch>0.000000</Pitch>\n"
                        f"              <TuneCoarse>0</TuneCoarse>\n"
                        f"              <TuneFine>0</TuneFine>\n"
                        f"              <SampleName>{xml_escape(stem)}</SampleName>\n"
                        f"              <SampleFile>{xml_escape(filename)}</SampleFile>\n"
                        f"              <File>{xml_escape(file_path)}</File>\n"
                        f"              <RootNote>0</RootNote>\n"
                        f"              <KeyTrack>True</KeyTrack>\n"
                        f"              <OneShot>False</OneShot>\n"
                        f"              <Loop>{loop_str}</Loop>\n"
                        f"              <LoopStart>{loop_start_val}</LoopStart>\n"
                        f"              <LoopEnd>{loop_end_val}</LoopEnd>\n"
                        f"              <LoopXFade>{loop_xfade_val}</LoopXFade>\n"
                        f"              <VolumeAttack>{_fmt(0)}</VolumeAttack>\n"
                        f"              <VolumeDecay>{_fmt(0)}</VolumeDecay>\n"
                        f"              <VolumeSustain>{_fmt(1)}</VolumeSustain>\n"
                        f"              <VolumeRelease>{_fmt(0.5)}</VolumeRelease>\n"
                        f"              <VelStart>1</VelStart>\n"
                        f"              <VelEnd>127</VelEnd>\n"
                        f"              <CycleType>{CYCLE_TYPE_ROUNDROBIN}</CycleType>\n"
                        f"              <CycleGroup>{rr_group}</CycleGroup>\n"
                        f"            </Layer>\n"
                    )
            else:
                # Standard velocity-layered mode
                # Map v1/v2/v3/v4 tags to the active velocity layer table
                v_tags_sorted = sorted(
                    (t for t in all_vel_tags if t.startswith("v")),
                    key=lambda t: int(t[1:]) if t[1:].isdigit() else 99,
                )
                for layer_num, vel_tag in enumerate(v_tags_sorted, start=1):
                    stem, filename = vel_layers_here[vel_tag]
                    file_path = f"Samples/{preset_slug}/{filename}" if filename else ""
                    # Map layer index to active_vel_layers
                    layer_idx = min(layer_num - 1, len(active_vel_layers) - 1)
                    vel_start, vel_end, vol = active_vel_layers[layer_idx]
                    layers_xml += (
                        f"            <Layer number=\"{layer_num}\">\n"
                        f"              <Active>True</Active>\n"
                        f"              <Volume>{_fmt(vol)}</Volume>\n"
                        f"              <Pan>0.500000</Pan>\n"
                        f"              <Pitch>0.000000</Pitch>\n"
                        f"              <TuneCoarse>0</TuneCoarse>\n"
                        f"              <TuneFine>0</TuneFine>\n"
                        f"              <SampleName>{xml_escape(stem)}</SampleName>\n"
                        f"              <SampleFile>{xml_escape(filename)}</SampleFile>\n"
                        f"              <File>{xml_escape(file_path)}</File>\n"
                        f"              <RootNote>0</RootNote>\n"
                        f"              <KeyTrack>True</KeyTrack>\n"
                        f"              <OneShot>False</OneShot>\n"
                        f"              <Loop>{loop_str}</Loop>\n"
                        f"              <LoopStart>{loop_start_val}</LoopStart>\n"
                        f"              <LoopEnd>{loop_end_val}</LoopEnd>\n"
                        f"              <LoopXFade>{loop_xfade_val}</LoopXFade>\n"
                        f"              <VolumeAttack>{_fmt(0)}</VolumeAttack>\n"
                        f"              <VolumeDecay>{_fmt(0)}</VolumeDecay>\n"
                        f"              <VolumeSustain>{_fmt(1)}</VolumeSustain>\n"
                        f"              <VolumeRelease>{_fmt(0.5)}</VolumeRelease>\n"
                        f"              <VelStart>{vel_start}</VelStart>\n"
                        f"              <VelEnd>{vel_end}</VelEnd>\n"
                        f"            </Layer>\n"
                    )
        else:
            # Empty placeholder — Rex Rule #3: VelStart=0, Active=False
            layers_xml = (
                "            <Layer number=\"1\">\n"
                "              <Active>False</Active>\n"
                "              <SampleName></SampleName>\n"
                "              <SampleFile></SampleFile>\n"
                "              <File></File>\n"
                "              <VelStart>0</VelStart>\n"
                "              <VelEnd>0</VelEnd>\n"
                "            </Layer>\n"
            )

        active_str = "True" if has_samples else "False"
        instruments_xml += (
            f"      <Instrument number=\"{inst_idx}\">\n"
            f"        <Active>{active_str}</Active>\n"
            f"        <Volume>1.000000</Volume>\n"
            f"        <Pan>0.500000</Pan>\n"
            f"        <Tune>0</Tune>\n"
            f"        <Transpose>0</Transpose>\n"
            f"        <VolumeAttack>{_fmt(0)}</VolumeAttack>\n"
            f"        <VolumeHold>{_fmt(0)}</VolumeHold>\n"
            f"        <VolumeDecay>{_fmt(0)}</VolumeDecay>\n"
            f"        <VolumeSustain>{_fmt(1)}</VolumeSustain>\n"
            f"        <VolumeRelease>{_fmt(0.5)}</VolumeRelease>\n"
            f"        <FilterType>2</FilterType>\n"
            f"        <Cutoff>1.000000</Cutoff>\n"
            f"        <Resonance>0.000000</Resonance>\n"
            f"        <FilterEnvAmt>0.000000</FilterEnvAmt>\n"
            f"        <LowNote>{low}</LowNote>\n"
            f"        <HighNote>{high}</HighNote>\n"
            f"        <RootNote>0</RootNote>\n"
            f"        <KeyTrack>True</KeyTrack>\n"
            f"        <OneShot>False</OneShot>\n"
            f"        <Layers>\n"
            f"{layers_xml}"
            f"        </Layers>\n"
            f"      </Instrument>\n"
        )
        inst_idx += 1
        num_instruments += 1

        # --- Release trigger instrument (one per zone, after main instrument) ---
        if release_layer and root in release_by_note:
            rel_stem, rel_filename = release_by_note[root]
            rel_path = f"Samples/{preset_slug}/{rel_filename}" if rel_filename else ""
            rel_layers_xml = (
                f"            <Layer number=\"1\">\n"
                f"              <Active>True</Active>\n"
                f"              <Volume>{_fmt(0.707946)}</Volume>\n"
                f"              <Pan>0.500000</Pan>\n"
                f"              <Pitch>0.000000</Pitch>\n"
                f"              <TuneCoarse>0</TuneCoarse>\n"
                f"              <TuneFine>0</TuneFine>\n"
                f"              <SampleName>{xml_escape(rel_stem)}</SampleName>\n"
                f"              <SampleFile>{xml_escape(rel_filename)}</SampleFile>\n"
                f"              <File>{xml_escape(rel_path)}</File>\n"
                f"              <RootNote>0</RootNote>\n"
                f"              <KeyTrack>True</KeyTrack>\n"
                f"              <OneShot>True</OneShot>\n"
                f"              <Loop>False</Loop>\n"
                f"              <LoopStart>0</LoopStart>\n"
                f"              <LoopEnd>0</LoopEnd>\n"
                f"              <LoopXFade>0</LoopXFade>\n"
                f"              <VolumeAttack>{_fmt(0)}</VolumeAttack>\n"
                f"              <VolumeDecay>{_fmt(0)}</VolumeDecay>\n"
                f"              <VolumeSustain>{_fmt(1)}</VolumeSustain>\n"
                f"              <VolumeRelease>{_fmt(0.1)}</VolumeRelease>\n"
                f"              <VelStart>1</VelStart>\n"
                f"              <VelEnd>127</VelEnd>\n"
                f"            </Layer>\n"
            )
            instruments_xml += (
                f"      <Instrument number=\"{inst_idx}\">\n"
                f"        <Active>True</Active>\n"
                f"        <Volume>0.500000</Volume>\n"
                f"        <Pan>0.500000</Pan>\n"
                f"        <Tune>0</Tune>\n"
                f"        <Transpose>0</Transpose>\n"
                f"        <VolumeAttack>{_fmt(0)}</VolumeAttack>\n"
                f"        <VolumeHold>{_fmt(0)}</VolumeHold>\n"
                f"        <VolumeDecay>{_fmt(0)}</VolumeDecay>\n"
                f"        <VolumeSustain>{_fmt(1)}</VolumeSustain>\n"
                f"        <VolumeRelease>{_fmt(0.1)}</VolumeRelease>\n"
                f"        <FilterType>2</FilterType>\n"
                f"        <Cutoff>0.800000</Cutoff>\n"
                f"        <Resonance>0.000000</Resonance>\n"
                f"        <FilterEnvAmt>0.000000</FilterEnvAmt>\n"
                f"        <LowNote>{low}</LowNote>\n"
                f"        <HighNote>{high}</HighNote>\n"
                f"        <RootNote>0</RootNote>\n"
                f"        <KeyTrack>True</KeyTrack>\n"
                f"        <OneShot>True</OneShot>\n"
                f"        <Layers>\n"
                f"{rel_layers_xml}"
                f"        </Layers>\n"
                f"      </Instrument>\n"
            )
            inst_idx += 1
            num_instruments += 1

    # Standardized XOceanus macro → MPC Q-Link mapping:
    #   Q1 → CHARACTER  (FilterCutoff — timbral character)
    #   Q2 → MOVEMENT   (LFO Rate — motion/modulation depth)
    #   Q3 → COUPLING   (Send2/AuxSend — cross-feed, closest MPC analog to coupling)
    #   Q4 → SPACE      (Send1/Reverb — spatial depth)
    # Label names are ≤10 chars for MPC XL OLED display compatibility.
    qlink_xml = (
        '    <QLinks>\n'
        '      <QLink number="1">\n'
        '        <Name>CHARACTER</Name>\n'
        '        <Parameter>FilterCutoff</Parameter>\n'
        '        <Min>0.200000</Min>\n'
        '        <Max>1.000000</Max>\n'
        '      </QLink>\n'
        '      <QLink number="2">\n'
        '        <Name>MOVEMENT</Name>\n'
        '        <Parameter>LFORate</Parameter>\n'
        '        <Min>0.000000</Min>\n'
        '        <Max>1.000000</Max>\n'
        '      </QLink>\n'
        '      <QLink number="3">\n'
        '        <Name>COUPLING</Name>\n'
        '        <Parameter>Send2</Parameter>\n'
        '        <Min>0.000000</Min>\n'
        '        <Max>0.700000</Max>\n'
        '      </QLink>\n'
        '      <QLink number="4">\n'
        '        <Name>SPACE</Name>\n'
        '        <Parameter>Send1</Parameter>\n'
        '        <Min>0.000000</Min>\n'
        '        <Max>0.700000</Max>\n'
        '      </QLink>\n'
        '    </QLinks>\n'
    )

    # Expression mapping: AfterTouch → FilterCutoff, ModWheel → FilterCutoff
    expression_xml = (
        '    <ExpressionMappings>\n'
        '      <ExpressionMapping number="1">\n'
        '        <Source>AfterTouch</Source>\n'
        '        <Destination>FilterCutoff</Destination>\n'
        '        <Amount>50</Amount>\n'
        '      </ExpressionMapping>\n'
        '      <ExpressionMapping number="2">\n'
        '        <Source>ModWheel</Source>\n'
        '        <Destination>FilterCutoff</Destination>\n'
        '        <Amount>70</Amount>\n'
        '      </ExpressionMapping>\n'
        '    </ExpressionMappings>\n'
    )

    xpm = (
        f"{_XPM_HEADER}\n"
        f"  <Program type=\"Keygroup\">\n"
        f"    <Name>{prog_name}</Name>\n"
        f"    <KeygroupNumKeygroups>{num_instruments}</KeygroupNumKeygroups>\n"
        f"    <KeygroupPitchBendRange>12</KeygroupPitchBendRange>\n"
        f"    <KeygroupWheelToLfo>0.000000</KeygroupWheelToLfo>\n"
        f"{expression_xml}"
        f"{qlink_xml}"
        f"    <Instruments>\n"
        f"{instruments_xml}"
        f"    </Instruments>\n"
        f"  </Program>\n"
        f"</MPCVObject>\n"
    )
    return xpm


# ---------------------------------------------------------------------------
# Standalone CLI
# ---------------------------------------------------------------------------

def main():
    parser = argparse.ArgumentParser(
        description="Generate Akai MPC Keygroup XPM from XO_OX WAV samples"
    )
    # Core args
    parser.add_argument("--wavs-dir",  required=True,
                        help="Directory containing WAV files")
    parser.add_argument("--preset",    required=True,
                        help="Preset name (e.g. 'Deep Drift')")
    parser.add_argument("--engine",    default="Unknown",
                        help="Engine name (e.g. Odyssey)")
    parser.add_argument("--output",    required=True,
                        help="Output directory for .xpm file")
    parser.add_argument("--dry-run",   action="store_true",
                        help="Print XPM to stdout, do not write file")

    # Feature flags (opt-in, backward compatible)
    parser.add_argument("--instrument-family",
                        choices=list(FAMILY_VEL_LAYERS.keys()),
                        default="default",
                        help="Instrument family for velocity curves "
                             "(piano/strings/brass/woodwind/world/organ/pads/bass/default)")
    parser.add_argument("--loop-detect", action="store_true",
                        help="Analyze WAV waveforms to detect loop points for sustained instruments")
    parser.add_argument("--loop-xfade-ms", type=int, default=100,
                        metavar="MS",
                        help="Loop crossfade length in milliseconds (50-200, default 100)")
    parser.add_argument("--round-robin", action="store_true",
                        help="Enable round-robin layer support (c1-c4 naming)")
    parser.add_argument("--release-layer", action="store_true",
                        help="Enable release trigger layer (rel naming)")
    parser.add_argument("--zone-strategy",
                        choices=["midpoint", "chromatic", "every-third", "per-octave"],
                        default="midpoint",
                        help="Zone boundary calculation strategy (default: midpoint)")
    parser.add_argument("--dna-adaptive", action="store_true",
                        help="Apply DNA-adaptive velocity curves from preset .xometa")
    parser.add_argument("--vel-curve",
                        choices=["musical", "even"],
                        default="musical",
                        help="Velocity curve style when not using DNA-adaptive or family modes")
    parser.add_argument("--vel-crossfade", type=int, default=3, metavar="N",
                        help="Velocity units to overlap between adjacent layers (default 3). "
                             "Set to 0 for hard splits. Only applies to timbral families "
                             "(piano/strings/brass/pads etc.), not drums.")

    args = parser.parse_args()

    wavs_dir   = Path(args.wavs_dir)
    output_dir = Path(args.output)
    preset_slug = args.preset.replace(" ", "_")

    wav_map = build_keygroup_wav_map(wavs_dir, preset_slug)
    print(f"Found {len(wav_map)} WAV files for '{args.preset}'")

    xpm = generate_keygroup_xpm(
        preset_name=args.preset,
        engine=args.engine,
        wav_map=wav_map,
        instrument_family=args.instrument_family,
        loop_detect=args.loop_detect,
        wavs_dir=wavs_dir if args.loop_detect else None,
        round_robin=args.round_robin,
        release_layer=args.release_layer,
        zone_strategy=args.zone_strategy,
        dna_adaptive=args.dna_adaptive,
        vel_curve=args.vel_curve,
        vel_crossfade=args.vel_crossfade,
    )

    if args.dry_run:
        print(xpm)
        return

    output_dir.mkdir(parents=True, exist_ok=True)
    out_path = output_dir / f"{preset_slug}.xpm"
    out_path.write_text(xpm, encoding="utf-8")
    print(f"Written: {out_path}")


if __name__ == "__main__":
    main()
