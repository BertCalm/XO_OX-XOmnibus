#!/usr/bin/env python3
"""
xpn_classify_instrument.py — Rule-based instrument classifier for WAV audio.

Analyzes a WAV file using only stdlib + wave module (no librosa, no sklearn).
Uses raw PCM math: DFT-based spectral centroid, transient detection, attack time,
and high-frequency energy ratio to classify drum/melodic/pad/FX instrument types.

Usage:
    python xpn_classify_instrument.py audio.wav
    python xpn_classify_instrument.py audio.wav --verbose

Import:
    from xpn_classify_instrument import classify_wav
    result = classify_wav("/path/to/audio.wav")

Output dict keys:
    category          : str  — kick/snare/hihat/cymbal/bass/lead/pad/perc/fx/unknown
    confidence        : float 0.0–1.0
    feliX_oscar_bias  : float -1.0 (Oscar/warm/dark) to +1.0 (feliX/bright/airy)
    transient_strength: float 0.0–1.0 (peak/RMS ratio, normalized)
    spectral_centroid_hz: float — frequency-weighted center of spectral mass

Shared dependency for: xpn_adaptive_velocity.py, xpn_adaptive_tags.py, oxport.py
See: Docs/specs/oxport_adaptive_intelligence_rnd.md, Ideas 1, 5, 7
"""

import sys
import wave
import math
import json
import struct
import argparse


# ---------------------------------------------------------------------------
# Constants / thresholds (tuned against drum machine samples and synth exports)
# ---------------------------------------------------------------------------

# DFT window — first N samples give us the attack transient spectrum.
# 2048 = ~46ms @ 44100, ~43ms @ 48000. Covers the initial transient well.
DFT_WINDOW = 2048

# RMS window for transient calculation: first 50ms worth of samples.
# We compute the transient_strength using a wider peak/RMS window.
TRANSIENT_WINDOW_MS = 50

# Attack time: samples to reach 80% of peak amplitude (absolute value).
ATTACK_TARGET = 0.80

# High-frequency boundary for HF energy ratio (Hz).
HF_BOUNDARY_HZ = 4000

# Spectral centroid thresholds (Hz) for rule-tree decisions.
CENTROID_KICK_MAX   =  600   # Kicks live very low (sub + low-mid thump)
CENTROID_BASS_MAX   = 1200   # Bass: below this with slow attack
CENTROID_MID_LOW    =  800   # Mid-low: snare body region
CENTROID_MID_HIGH   = 3500   # Mid-high: snare crack, upper harmonics
CENTROID_HIHAT_MIN  = 5000   # Hihats: mostly high-frequency content
CENTROID_CYMBAL_MIN = 4000   # Cymbals: wide but high centroid
CENTROID_PAD_MAX    = 2500   # Pads: often warm, mid-centric
CENTROID_LEAD_MIN   = 1500   # Leads: bright-ish upper mids

# Transient strength thresholds (normalized peak/RMS ratio, 0–1).
TRANSIENT_PERCUSSIVE = 0.60  # Strong transient (kick, snare, hihat)
TRANSIENT_MID        = 0.30  # Moderate (lead, pluck, bass hit)
TRANSIENT_SOFT       = 0.15  # Slow attack (pad, atmosphere)

# Attack time thresholds (fraction of sample rate for 50ms).
# Drums have very short attacks (< 5ms = ~220 samples @ 44100).
ATTACK_SHORT_MS  =  5   # ms
ATTACK_MEDIUM_MS = 25   # ms

# HF energy ratio thresholds.
HF_HIGH = 0.40   # Hihats/cymbals have lots of energy above 4kHz
HF_MID  = 0.15   # Snares crack has some; kicks have very little
HF_LOW  = 0.05   # Sub/bass dominated content


# ---------------------------------------------------------------------------
# WAV loading
# ---------------------------------------------------------------------------

def load_wav_mono(path: str) -> tuple[list[float], int]:
    """
    Load a WAV file and return (samples_mono, sample_rate).
    Samples are normalized to [-1.0, 1.0].
    Handles 8-bit, 16-bit, 24-bit, and 32-bit PCM.
    For stereo, takes the average of both channels (true mono mix).
    """
    with wave.open(path, 'rb') as wf:
        n_channels  = wf.getnchannels()
        sampwidth   = wf.getsampwidth()   # bytes per sample
        framerate   = wf.getframerate()
        n_frames    = wf.getnframes()
        raw_bytes   = wf.readframes(n_frames)

    # Decode raw bytes to integer samples based on bit depth.
    if sampwidth == 1:
        # 8-bit WAV is unsigned (0–255), center at 128.
        fmt = f"{n_frames * n_channels}B"
        raw = list(struct.unpack(fmt, raw_bytes))
        samples_int = [(s - 128) for s in raw]
        max_val = 128.0
    elif sampwidth == 2:
        # 16-bit signed little-endian.
        fmt = f"<{n_frames * n_channels}h"
        samples_int = list(struct.unpack(fmt, raw_bytes))
        max_val = 32768.0
    elif sampwidth == 3:
        # 24-bit: no direct struct format — unpack manually.
        samples_int = []
        for i in range(0, len(raw_bytes), 3):
            b0, b1, b2 = raw_bytes[i], raw_bytes[i+1], raw_bytes[i+2]
            val = (b2 << 16) | (b1 << 8) | b0
            if val >= 0x800000:
                val -= 0x1000000   # sign extend
            samples_int.append(val)
        max_val = 8388608.0
    elif sampwidth == 4:
        # 32-bit signed.
        fmt = f"<{n_frames * n_channels}i"
        samples_int = list(struct.unpack(fmt, raw_bytes))
        max_val = 2147483648.0
    else:
        raise ValueError(f"Unsupported sample width: {sampwidth} bytes")

    # Normalize to [-1.0, 1.0].
    samples_float = [s / max_val for s in samples_int]

    # Mix down to mono by averaging channels.
    if n_channels == 1:
        mono = samples_float
    else:
        mono = []
        for i in range(0, len(samples_float), n_channels):
            frame = samples_float[i : i + n_channels]
            mono.append(sum(frame) / n_channels)

    return mono, framerate


# ---------------------------------------------------------------------------
# Feature extraction — raw PCM math only
# ---------------------------------------------------------------------------

def compute_rms(samples: list[float]) -> float:
    """Root-mean-square energy of a sample window."""
    if not samples:
        return 0.0
    return math.sqrt(sum(s * s for s in samples) / len(samples))


def compute_peak(samples: list[float]) -> float:
    """Peak absolute amplitude in the sample window."""
    if not samples:
        return 0.0
    return max(abs(s) for s in samples)


def compute_transient_strength(mono: list[float], sample_rate: int) -> float:
    """
    Transient strength = peak / (RMS * sqrt(2)) in first 50ms, normalized to [0, 1].
    A pure sine wave gives ~0.5; a sharp impulse approaches 1.0.
    Kicks, snares, hihats all have high transient strength (>= 0.6).
    Pads have low transient strength (<= 0.2).
    """
    window_samples = int(sample_rate * TRANSIENT_WINDOW_MS / 1000)
    window = mono[:window_samples] if len(mono) >= window_samples else mono
    if not window:
        return 0.0
    peak = compute_peak(window)
    rms  = compute_rms(window)
    if rms < 1e-9:
        return 0.0
    # peak/RMS for a sine = sqrt(2) ≈ 1.414.  Normalize so sine = 0.5.
    ratio = (peak / rms) / (2.0 * math.sqrt(2))
    return min(1.0, ratio)


def compute_attack_time_ms(mono: list[float], sample_rate: int) -> float:
    """
    Number of milliseconds from sample start to reach ATTACK_TARGET (80%) of peak.
    Searches first 500ms to bound computation.
    Returns 0.0 if audio is essentially silent.
    """
    search_samples = min(len(mono), int(sample_rate * 0.5))
    if search_samples == 0:
        return 0.0
    peak = compute_peak(mono[:search_samples])
    if peak < 1e-9:
        return 0.0
    target = peak * ATTACK_TARGET
    for i in range(search_samples):
        if abs(mono[i]) >= target:
            return (i / sample_rate) * 1000.0
    return (search_samples / sample_rate) * 1000.0


def compute_dft_magnitudes(samples: list[float], n: int) -> list[float]:
    """
    Compute magnitude spectrum via DFT for the first N samples.
    Only computes the positive-frequency half (bins 0 .. N//2).
    Uses math.cos / math.sin — no numpy required.
    For N=2048 this is O(N²/2) ≈ 2M ops, fast enough for a single classify call.
    """
    n = min(n, len(samples))
    window = samples[:n]
    # Apply a Hann window to reduce spectral leakage.
    hann = [0.5 * (1.0 - math.cos(2.0 * math.pi * i / (n - 1))) for i in range(n)]
    windowed = [window[i] * hann[i] for i in range(n)]

    half = n // 2
    magnitudes = []
    two_pi_over_n = 2.0 * math.pi / n
    for k in range(half):
        re = 0.0
        im = 0.0
        for t in range(n):
            angle = two_pi_over_n * k * t
            re += windowed[t] * math.cos(angle)
            im -= windowed[t] * math.sin(angle)
        magnitudes.append(math.sqrt(re * re + im * im))
    return magnitudes


def compute_spectral_centroid_hz(magnitudes: list[float], sample_rate: int) -> float:
    """
    Spectral centroid = sum(freq * magnitude) / sum(magnitude).
    This is the 'center of mass' of the spectrum — low for bass, high for hihats.
    """
    n = len(magnitudes)
    if n == 0:
        return 0.0
    total_mag = sum(magnitudes)
    if total_mag < 1e-12:
        return 0.0
    # Bin k corresponds to frequency k * sample_rate / (2 * n).
    bin_hz = sample_rate / (2.0 * n)
    weighted_sum = sum(k * bin_hz * magnitudes[k] for k in range(n))
    return weighted_sum / total_mag


def compute_hf_energy_ratio(magnitudes: list[float], sample_rate: int) -> float:
    """
    Ratio of energy in bins above HF_BOUNDARY_HZ to total energy.
    Hihats/cymbals: > 0.40.  Kicks: < 0.05.  Snares: 0.10–0.30.
    """
    n = len(magnitudes)
    if n == 0:
        return 0.0
    total_energy = sum(m * m for m in magnitudes)
    if total_energy < 1e-20:
        return 0.0
    # Bin index for HF_BOUNDARY_HZ:  k = freq * 2 * n / sample_rate.
    cutoff_bin = int(HF_BOUNDARY_HZ * 2 * n / sample_rate)
    cutoff_bin = min(cutoff_bin, n - 1)
    hf_energy = sum(m * m for m in magnitudes[cutoff_bin:])
    return hf_energy / total_energy


# ---------------------------------------------------------------------------
# feliX/Oscar bias computation
# ---------------------------------------------------------------------------

def compute_felix_oscar_bias(centroid_hz: float, hf_ratio: float, attack_ms: float) -> float:
    """
    feliX (positive) = bright, airy, high-frequency dominant.
    Oscar (negative) = warm, dark, sub-bass dominant.

    Combines spectral centroid (normalized against 8kHz reference),
    HF energy ratio, and inverse attack speed.
    Result is clamped to [-1.0, +1.0].
    """
    # Centroid component: 0 Hz → -1.0, 8000 Hz → +1.0.
    centroid_component = (centroid_hz / 4000.0) - 1.0
    centroid_component = max(-1.0, min(1.0, centroid_component))

    # HF ratio component: 0.0 → -0.5, 0.8+ → +0.5.
    hf_component = (hf_ratio * 1.25) - 0.5
    hf_component = max(-0.5, min(0.5, hf_component))

    # Attack component: very fast attack = percussive/neutral (0),
    # very slow attack = pad-like / warm = slight Oscar lean.
    if attack_ms < 1.0:
        attack_component = 0.0      # Neutral for ultra-sharp transients
    elif attack_ms < 50.0:
        attack_component = 0.0
    else:
        # Long attacks → warm/Oscar lean (up to -0.3)
        attack_component = -min(0.3, (attack_ms - 50.0) / 500.0)

    bias = (centroid_component * 0.5) + (hf_component * 0.35) + (attack_component * 0.15)
    return max(-1.0, min(1.0, bias))


# ---------------------------------------------------------------------------
# Classification rule tree
# ---------------------------------------------------------------------------

def classify_features(
    centroid_hz: float,
    transient_strength: float,
    attack_ms: float,
    hf_ratio: float,
    sample_duration_ms: float,
) -> tuple[str, float]:
    """
    Rule tree that maps audio features to instrument category + confidence.

    Rule priority (highest to lowest):
        1. Kick: very low centroid + strong transient + short attack
        2. HiHat: very high centroid + very high HF ratio + short attack
        3. Cymbal: high centroid + high HF ratio + longer duration
        4. Snare: mid centroid + strong transient + moderate HF
        5. Bass: low centroid + slow attack (melodic bass notes)
        6. Pad: low-mid centroid + weak transient + slow attack
        7. Lead: mid-high centroid + moderate transient
        8. Perc: catch-all for transient content that doesn't fit above
        9. FX: long duration + low transient + unusual centroid
        10. Unknown: fallback
    """
    # --- Rule 1: KICK ---
    # Low spectral centroid (thump lives in sub/low-mid), strong transient,
    # very short attack (the "click" of a kick).
    if (centroid_hz < CENTROID_KICK_MAX
            and transient_strength >= TRANSIENT_PERCUSSIVE
            and attack_ms < ATTACK_SHORT_MS * 3):   # < 15ms
        confidence = _score([
            centroid_hz < CENTROID_KICK_MAX,          # low centroid required
            centroid_hz < 400,                         # bonus: very sub
            transient_strength > 0.75,                 # bonus: punchy
            attack_ms < ATTACK_SHORT_MS,               # bonus: ultra fast
            hf_ratio < HF_MID,                         # kicks should be low
        ])
        return "kick", confidence

    # --- Rule 2: HIHAT ---
    # Very high centroid + dominant HF energy + short sharp attack.
    if (centroid_hz >= CENTROID_HIHAT_MIN
            and hf_ratio >= HF_HIGH
            and transient_strength >= TRANSIENT_MID):
        confidence = _score([
            centroid_hz >= CENTROID_HIHAT_MIN,
            centroid_hz >= 6000,                       # bonus: ultra bright
            hf_ratio >= 0.55,                          # bonus: nearly all HF
            transient_strength >= TRANSIENT_PERCUSSIVE,
            attack_ms < ATTACK_SHORT_MS * 2,
        ])
        return "hihat", confidence

    # --- Rule 3: CYMBAL ---
    # High centroid, high HF, but longer sustain than hihat (ride, crash).
    if (centroid_hz >= CENTROID_CYMBAL_MIN
            and hf_ratio >= HF_HIGH * 0.8
            and sample_duration_ms > 200):
        confidence = _score([
            centroid_hz >= CENTROID_CYMBAL_MIN,
            hf_ratio >= HF_HIGH,
            sample_duration_ms > 400,                  # crash sustains longer
            transient_strength >= TRANSIENT_MID,
        ])
        return "cymbal", confidence

    # --- Rule 4: SNARE ---
    # Mid-range centroid (body + crack), strong transient, moderate HF
    # from the snare wire noise.
    if (CENTROID_MID_LOW <= centroid_hz <= CENTROID_MID_HIGH * 1.5
            and transient_strength >= TRANSIENT_MID
            and hf_ratio >= HF_MID):
        confidence = _score([
            transient_strength >= TRANSIENT_PERCUSSIVE,
            CENTROID_MID_LOW <= centroid_hz <= CENTROID_MID_HIGH,
            hf_ratio >= HF_MID,
            hf_ratio < HF_HIGH,                        # not as bright as hihat
            attack_ms < ATTACK_SHORT_MS * 4,           # still fast
        ])
        return "snare", confidence

    # --- Rule 5: BASS ---
    # Low centroid (sub/bass harmonics), but NOT a kick — slower attack
    # or longer duration suggests melodic bass content.
    if (centroid_hz <= CENTROID_BASS_MAX
            and (attack_ms > ATTACK_SHORT_MS * 3 or sample_duration_ms > 300)):
        confidence = _score([
            centroid_hz <= CENTROID_BASS_MAX,
            centroid_hz <= 600,                        # bonus: deep bass
            hf_ratio <= HF_LOW * 2,                    # limited HF for bass
            sample_duration_ms > 200,
            transient_strength < TRANSIENT_PERCUSSIVE, # bass isn't all transient
        ])
        return "bass", confidence

    # --- Rule 6: PAD ---
    # Slow attack is the defining feature of a pad. Low-mid centroid typical,
    # weak transient, sustained character.
    if (attack_ms > ATTACK_MEDIUM_MS
            and transient_strength <= TRANSIENT_SOFT * 1.5
            and sample_duration_ms > 500):
        confidence = _score([
            attack_ms > ATTACK_MEDIUM_MS * 2,          # very slow
            transient_strength <= TRANSIENT_SOFT,
            centroid_hz <= CENTROID_PAD_MAX,           # warm centroid
            sample_duration_ms > 1000,
            hf_ratio <= HF_MID,                        # not bright
        ])
        return "pad", confidence

    # --- Rule 7: LEAD ---
    # Moderate to fast attack, mid-high centroid, moderate transient.
    # Synth leads, melodic hits.
    if (centroid_hz >= CENTROID_LEAD_MIN
            and transient_strength >= TRANSIENT_SOFT
            and attack_ms <= ATTACK_MEDIUM_MS * 2):
        confidence = _score([
            centroid_hz >= CENTROID_LEAD_MIN,
            centroid_hz >= 2500,                       # bonus: brighter lead
            TRANSIENT_SOFT <= transient_strength <= TRANSIENT_PERCUSSIVE,
            attack_ms < ATTACK_MEDIUM_MS,
            sample_duration_ms > 100,
        ])
        return "lead", confidence

    # --- Rule 8: PERC ---
    # Catch-all for transient percussion that didn't fit the drum archetypes.
    # Things like: rim shots, woodblocks, shakers, congas, bongos.
    if transient_strength >= TRANSIENT_MID:
        confidence = _score([
            transient_strength >= TRANSIENT_MID,
            attack_ms < ATTACK_MEDIUM_MS,
            sample_duration_ms < 500,                  # shorter = more percussive
        ])
        return "perc", max(0.35, min(0.65, confidence))

    # --- Rule 9: FX ---
    # Texture, risers, impacts, sweeps: long duration, low or unusual transient,
    # broad centroid range. Catch before unknown.
    if sample_duration_ms > 1000 and transient_strength <= TRANSIENT_SOFT:
        return "fx", 0.45

    # --- Rule 10: UNKNOWN ---
    return "unknown", 0.20


def _score(conditions: list[bool]) -> float:
    """
    Compute a confidence score from a list of boolean conditions.
    The first condition is required (weight 0.4).
    Remaining conditions are bonuses that push confidence toward 1.0.
    Returns a value in [0.40, 1.0].
    """
    if not conditions:
        return 0.40
    base = 0.40 if conditions[0] else 0.10
    bonus_conditions = conditions[1:]
    if not bonus_conditions:
        return base
    bonus_per = 0.60 / len(bonus_conditions)
    bonus = sum(bonus_per for c in bonus_conditions if c)
    return min(1.0, base + bonus)


# ---------------------------------------------------------------------------
# Public API
# ---------------------------------------------------------------------------

def classify_wav(wav_path: str, verbose: bool = False) -> dict:
    """
    Analyze a WAV file and return an instrument classification dict.

    Returns:
        {
            "category":            str,   # kick/snare/hihat/cymbal/bass/lead/pad/perc/fx/unknown
            "confidence":          float, # 0.0–1.0
            "feliX_oscar_bias":    float, # -1.0 (Oscar/warm) to +1.0 (feliX/bright)
            "transient_strength":  float, # 0.0–1.0
            "spectral_centroid_hz":float, # frequency-weighted spectral center of mass
        }
    """
    # Load audio.
    mono, sample_rate = load_wav_mono(wav_path)
    if not mono:
        return {
            "category": "unknown",
            "confidence": 0.0,
            "feliX_oscar_bias": 0.0,
            "transient_strength": 0.0,
            "spectral_centroid_hz": 0.0,
        }

    sample_duration_ms = (len(mono) / sample_rate) * 1000.0

    # Feature extraction.
    dft_n       = min(DFT_WINDOW, len(mono))
    magnitudes  = compute_dft_magnitudes(mono, dft_n)
    centroid_hz = compute_spectral_centroid_hz(magnitudes, sample_rate)
    hf_ratio    = compute_hf_energy_ratio(magnitudes, sample_rate)
    transient   = compute_transient_strength(mono, sample_rate)
    attack_ms   = compute_attack_time_ms(mono, sample_rate)
    bias        = compute_felix_oscar_bias(centroid_hz, hf_ratio, attack_ms)

    if verbose:
        print(f"  sample_rate:      {sample_rate} Hz")
        print(f"  duration:         {sample_duration_ms:.1f} ms")
        print(f"  spectral_centroid:{centroid_hz:.1f} Hz")
        print(f"  hf_ratio:         {hf_ratio:.3f}")
        print(f"  transient_strength:{transient:.3f}")
        print(f"  attack_ms:        {attack_ms:.2f} ms")
        print(f"  feliX_oscar_bias: {bias:.3f}")

    category, confidence = classify_features(
        centroid_hz, transient, attack_ms, hf_ratio, sample_duration_ms
    )

    return {
        "category":             category,
        "confidence":           round(confidence, 3),
        "feliX_oscar_bias":     round(bias, 3),
        "transient_strength":   round(transient, 3),
        "spectral_centroid_hz": round(centroid_hz, 1),
    }


# ---------------------------------------------------------------------------
# CLI entry point
# ---------------------------------------------------------------------------

def main():
    parser = argparse.ArgumentParser(
        description="Rule-based instrument classifier — WAV → category (stdlib only)"
    )
    parser.add_argument("wav", help="Path to WAV file")
    parser.add_argument(
        "--verbose", "-v", action="store_true",
        help="Print intermediate feature values before JSON output"
    )
    args = parser.parse_args()

    if args.verbose:
        print(f"Analyzing: {args.wav}")

    result = classify_wav(args.wav, verbose=args.verbose)
    print(json.dumps(result, indent=2))


if __name__ == "__main__":
    main()
