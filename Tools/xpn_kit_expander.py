#!/usr/bin/env python3
"""
XPN Kit Expander — XO_OX Designs
Takes a simple drum kit (one flat WAV per voice) and expands it into a fully
dynamic, expressive set with velocity layers and/or round-robin variants.

The expander applies gentle DSP transforms so that each derived sample sounds
like a natural variation of the source — not a processed artifact.

Expansion Modes:
  velocity    4 velocity layers (pp/mp/mf/ff) derived from one source WAV.
              Transform chain per layer:
                v1 (ghost):  -12 dB, low-pass ~3 kHz, pitch -2 ct
                v2 (soft):   -6 dB,  low-pass ~6 kHz
                v3 (medium): -3 dB,  flat EQ
                v4 (hard):    0 dB,  soft clip + transient sharpening

  cycle       4 round-robin variants for machine-gun prevention.
              Transform chain per variant:
                c1: source unchanged
                c2: micro-pitch +5 ct, subtle 2 ms channel delay
                c3: -1 dB, 800 Hz notch (-2 dB), slight reverb tail smear
                c4: micro-pitch -4 ct, transient tail shortened 10%

  smart       Applies velocity to kick/snare/tom, cycle to hats/perc,
              random variants to clap/fx. Produces all needed WAVs for
              `xpn_drum_export.py --mode smart`.

  full        Both velocity (4 layers) AND cycle (4 variants) produced.
              Doubles the sample set — maximum expressiveness.

Requirements:
    pip install numpy scipy soundfile

Usage:
    # Expand single WAV into velocity layers
    python3 xpn_kit_expander.py --source kick.wav --voice kick \\
        --mode velocity --output-dir /path/to/wavs --preset "808 Reborn"

    # Expand a whole kit directory (one WAV per voice, any naming)
    python3 xpn_kit_expander.py --kit-dir /path/to/flat_kit \\
        --preset "808 Reborn" --mode smart --output-dir /path/to/out

    # Dry run: show what would be generated
    python3 xpn_kit_expander.py --kit-dir /path --preset "808 Reborn" \\
        --mode smart --dry-run

Flat kit input naming (auto-detected):
    {voice}.wav            e.g.  kick.wav, snare.wav, chat.wav
    {anything}_{voice}.wav e.g.  808_kick.wav, my_snare.wav
    The first file matching each voice name is used.
"""

import argparse
import os
import sys
import time
from concurrent.futures import ProcessPoolExecutor, as_completed
from pathlib import Path
from typing import List, Optional

import numpy as np

try:
    import soundfile as sf
    SF_AVAILABLE = True
except ImportError:
    SF_AVAILABLE = False

try:
    from scipy import signal as sig
    SCIPY_AVAILABLE = True
except ImportError:
    SCIPY_AVAILABLE = False

from xpn_drum_export import PAD_MAP, VEL_SUFFIXES, CYCLE_SUFFIXES, SMART_MODE, _resolve_mode

VOICE_NAMES = [v for _, v, _, _ in PAD_MAP]


# =============================================================================
# AUDIO I/O
# =============================================================================

def load_wav(path: Path):
    """Load WAV → (samples float32, sample_rate). Mono or stereo."""
    if not SF_AVAILABLE:
        raise RuntimeError("soundfile required: pip install soundfile")
    data, sr = sf.read(str(path), dtype="float32", always_2d=False)
    return data, sr


def save_wav(path: Path, data: np.ndarray, sr: int):
    """Write float32 array to 24-bit WAV."""
    if not SF_AVAILABLE:
        raise RuntimeError("soundfile required: pip install soundfile")
    # Clip to prevent clipping artifacts before write
    data = np.clip(data, -1.0, 1.0)
    sf.write(str(path), data, sr, subtype="PCM_24")


# =============================================================================
# DSP PRIMITIVES
# =============================================================================

def gain(data: np.ndarray, db: float) -> np.ndarray:
    """Apply gain in dB."""
    return data * (10 ** (db / 20.0))


def lowpass(data: np.ndarray, sr: int, cutoff_hz: float,
            order: int = 4) -> np.ndarray:
    """Butterworth low-pass filter."""
    if not SCIPY_AVAILABLE:
        # Fallback: crude moving-average approximation
        kernel_size = max(1, int(sr / cutoff_hz / 2))
        kernel = np.ones(kernel_size) / kernel_size
        if data.ndim == 1:
            return np.convolve(data, kernel, mode="same")
        return np.column_stack([
            np.convolve(data[:, ch], kernel, mode="same")
            for ch in range(data.shape[1])
        ])
    nyq = sr / 2.0
    cutoff = min(cutoff_hz / nyq, 0.99)
    b, a = sig.butter(order, cutoff, btype="low")
    if data.ndim == 1:
        return sig.filtfilt(b, a, data).astype(np.float32)
    return np.column_stack([
        sig.filtfilt(b, a, data[:, ch]).astype(np.float32)
        for ch in range(data.shape[1])
    ])


def notch(data: np.ndarray, sr: int, freq_hz: float,
          depth_db: float = -2.0, q: float = 2.0) -> np.ndarray:
    """Notch filter at freq_hz."""
    if not SCIPY_AVAILABLE:
        return data  # skip if scipy missing
    w0 = freq_hz / (sr / 2.0)
    w0 = min(w0, 0.99)
    b, a = sig.iirnotch(w0, q)
    gain_factor = 10 ** (depth_db / 20.0)
    # Blend notched signal with original
    notched = (
        sig.filtfilt(b, a, data, axis=0).astype(np.float32)
        if data.ndim > 1
        else sig.filtfilt(b, a, data).astype(np.float32)
    )
    # Full notch = just notched; partial = blend
    alpha = abs(depth_db) / 6.0  # 0..1 across 6 dB
    alpha = min(alpha, 1.0)
    return (data * (1 - alpha) + notched * alpha).astype(np.float32)


def micro_pitch_shift(data: np.ndarray, sr: int, cents: float) -> np.ndarray:
    """
    Approximate pitch shift via resampling (maintains duration).
    Accurate enough for small shifts (±30 cents); avoids the full
    phase-vocoder cost of larger shifts.
    """
    if abs(cents) < 0.5:
        return data
    ratio = 2 ** (cents / 1200.0)
    original_len = len(data)
    # Resample to shifted length
    shifted_len = int(round(original_len * ratio))
    if data.ndim == 1:
        shifted = np.interp(
            np.linspace(0, original_len - 1, shifted_len),
            np.arange(original_len),
            data,
        ).astype(np.float32)
    else:
        channels = [
            np.interp(
                np.linspace(0, original_len - 1, shifted_len),
                np.arange(original_len),
                data[:, ch],
            ).astype(np.float32)
            for ch in range(data.shape[1])
        ]
        shifted = np.column_stack(channels)

    # Trim or pad to original length
    if shifted_len >= original_len:
        return shifted[:original_len]
    pad_len = original_len - shifted_len
    if data.ndim == 1:
        return np.concatenate([shifted, np.zeros(pad_len, dtype=np.float32)])
    return np.vstack([shifted,
                      np.zeros((pad_len, data.shape[1]), dtype=np.float32)])


def channel_delay(data: np.ndarray, sr: int, delay_ms: float,
                  channel: int = 1) -> np.ndarray:
    """
    Apply a short delay to one channel (stereo spread without pitch change).
    If mono, converts to stereo first.
    """
    delay_samples = int(round(delay_ms * sr / 1000.0))
    if delay_samples == 0:
        return data

    # Ensure stereo
    if data.ndim == 1:
        data = np.column_stack([data, data])

    result = data.copy()
    ch = min(channel, data.shape[1] - 1)
    if delay_samples > 0:
        result[delay_samples:, ch] = data[:-delay_samples, ch]
        result[:delay_samples, ch] = 0.0
    return result.astype(np.float32)


def soft_clip(data: np.ndarray, drive: float = 1.5) -> np.ndarray:
    """Gentle waveshaper — adds odd harmonics without hard clipping."""
    driven = data * drive
    # tanh waveshaper: smooth saturation, preserves transient shape
    clipped = np.tanh(driven)
    # Normalize back to approximate original level
    return (clipped / drive).astype(np.float32)


def sharpen_transient(data: np.ndarray, sr: int,
                      attack_ms: float = 5.0) -> np.ndarray:
    """
    Boost the attack transient using an exponential decay curve.
    (Guru Bin: "linear ramp doesn't exist in nature — use exponential")
    """
    attack_samples = int(attack_ms * sr / 1000.0)
    attack_samples = min(attack_samples, len(data) // 4)
    result = data.copy()
    if attack_samples > 0:
        # Exponential decay: mirrors how acoustic energy dissipates
        ramp = 1.0 + 0.3 * np.exp(-np.linspace(0, 4, attack_samples))
        if data.ndim == 1:
            result[:attack_samples] *= ramp
        else:
            result[:attack_samples] *= ramp[:, np.newaxis]
    return np.clip(result, -1.0, 1.0).astype(np.float32)


def soften_attack(data: np.ndarray, sr: int,
                  fade_ms: float = 1.5) -> np.ndarray:
    """
    Soften the attack transient with a brief fade-in.
    Ghost notes have rounded transients — the pad is barely touched.
    (Guru Bin: "a ghost note's attack is fundamentally different from
    a hard hit that's been turned down")
    """
    fade_samples = int(fade_ms * sr / 1000.0)
    fade_samples = min(fade_samples, len(data) // 4)
    result = data.copy()
    if fade_samples > 0:
        ramp = np.linspace(0.0, 1.0, fade_samples) ** 0.5  # sqrt for gentle curve
        if data.ndim == 1:
            result[:fade_samples] *= ramp
        else:
            result[:fade_samples] *= ramp[:, np.newaxis]
    return result.astype(np.float32)


def shorten_tail(data: np.ndarray, factor: float = 0.90) -> np.ndarray:
    """
    Fade out the last (1-factor) of the sample more aggressively.
    Creates a tighter, slightly different decay character.
    """
    keep_len = int(len(data) * factor)
    fade_len = len(data) - keep_len
    if fade_len <= 0:
        return data
    fade = np.linspace(1.0, 0.0, fade_len) ** 2
    result = data.copy()
    if data.ndim == 1:
        result[keep_len:] *= fade
    else:
        result[keep_len:] *= fade[:, np.newaxis]
    return result.astype(np.float32)


def smear_tail(data: np.ndarray, sr: int, smear_ms: float = 8.0) -> np.ndarray:
    """
    Add a very short, diffuse reverb tail — blurs the decay slightly.
    Simulated with a decaying comb filter (no IR needed).
    """
    delay_samples = int(smear_ms * sr / 1000.0)
    if delay_samples < 1 or not SCIPY_AVAILABLE:
        return data
    fb = 0.25  # feedback amount — keep low to avoid buildup
    b = np.zeros(delay_samples + 1)
    b[0] = 1.0
    b[-1] = fb * 0.3
    a = [1.0, *([0.0] * (delay_samples - 1)), -fb]
    if data.ndim == 1:
        return sig.lfilter(b, a, data).astype(np.float32)
    return np.column_stack([
        sig.lfilter(b, a, data[:, ch]).astype(np.float32)
        for ch in range(data.shape[1])
    ])


# =============================================================================
# TRANSFORM CHAINS
# =============================================================================

def derive_velocity_layers(data: np.ndarray, sr: int,
                            voice: str = "") -> dict:
    """
    Derive 4 velocity layers from a single source sample.
    Returns {vel_suffix: np.ndarray}.

    The transforms model what actually changes physically when you
    hit a drum harder: transient energy, filter brightness, and amplitude.
    """
    # Voice-aware transient duration (Guru Bin: kicks are slower, hats are snappier)
    attack_ms = {"kick": 8.0, "tom": 7.0, "chat": 3.0, "ohat": 4.0}.get(voice, 5.0)

    return {
        # v1 ghost: quiet + dark + rounded attack (Guru: "ghost transients are fundamentally different")
        "v1": soften_attack(
            lowpass(gain(micro_pitch_shift(data, sr, -2.0), -12.0), sr, 3000),
            sr, fade_ms=1.5),
        "v2": lowpass(gain(data, -6.0), sr, 6000),
        "v3": gain(data, -3.0),
        "v4": sharpen_transient(soft_clip(gain(data, 0.0), drive=1.4), sr,
                                attack_ms=attack_ms),
    }


def derive_cycle_variants(data: np.ndarray, sr: int,
                           voice: str = "") -> dict:
    """
    Derive 4 round-robin variants from a single source sample.
    Returns {cycle_suffix: np.ndarray}.

    Each variant has just enough difference that the ear doesn't detect
    the repetition pattern — without changing the essential character.
    """
    # Vibe: skip channel_delay on kick/snare to preserve center stereo image
    center_voices = {"kick", "snare", "tom"}
    c2_pitch = micro_pitch_shift(data, sr, +5.0)
    if voice not in center_voices:
        c2_pitch = channel_delay(c2_pitch, sr, 2.0)

    return {
        "c1": data.copy(),
        "c2": c2_pitch,
        "c3": smear_tail(notch(gain(data, -1.0), sr, 800, depth_db=-2.0), sr),
        "c4": shorten_tail(micro_pitch_shift(data, sr, -4.0), 0.90),
    }


# =============================================================================
# KIT FINDER
# =============================================================================

def find_voice_wav(kit_dir: Path, voice_name: str) -> Optional[Path]:
    """
    Find the WAV for a voice in a flat kit directory.
    Accepts: {voice}.wav, {anything}_{voice}.wav, {voice}_{anything}.wav
    """
    # Exact name first
    exact = kit_dir / f"{voice_name}.wav"
    if exact.exists():
        return exact
    # Suffix match
    for wav in sorted(kit_dir.glob("*.wav")):
        stem = wav.stem.lower()
        if stem == voice_name or stem.endswith(f"_{voice_name}") or stem.startswith(f"{voice_name}_"):
            return wav
    return None


# =============================================================================
# EXPANDER CORE
# =============================================================================

def expand_voice(source: Path, voice_name: str, preset_slug: str,
                 expand_mode: str, output_dir: Path,
                 dry_run: bool = False) -> List[str]:
    """
    Expand a single source WAV into derived samples.
    Returns list of filenames written.
    """
    written = []

    if not dry_run:
        if not SF_AVAILABLE:
            raise RuntimeError("soundfile required: pip install soundfile")
        data, sr = load_wav(source)

    effective = _resolve_mode(expand_mode, voice_name)

    if effective in ("cycle", "random", "random-norepeat"):
        if dry_run:
            variants = {s: None for s in CYCLE_SUFFIXES}
        else:
            variants = derive_cycle_variants(data, sr, voice_name)
        for suffix, derived in variants.items():
            fname = f"{preset_slug}_{voice_name}_{suffix}.wav"
            if not dry_run:
                save_wav(output_dir / fname, derived, sr)
            print(f"  {'[DRY]' if dry_run else '     '} {fname}")
            written.append(fname)

    else:  # velocity
        if dry_run:
            layers = {s: None for s in VEL_SUFFIXES}
        else:
            layers = derive_velocity_layers(data, sr, voice_name)
        for suffix, derived in layers.items():
            fname = f"{preset_slug}_{voice_name}_{suffix}.wav"
            if not dry_run:
                save_wav(output_dir / fname, derived, sr)
            print(f"  {'[DRY]' if dry_run else '     '} {fname}")
            written.append(fname)

    return written


def _expand_voice_worker(source_path: str, voice_name: str, preset_slug: str,
                         expand_mode: str, output_dir_str: str) -> List[str]:
    """
    Worker function for parallel voice expansion.
    Accepts and returns only picklable types (strings, lists) so it can
    run in a subprocess via ProcessPoolExecutor.
    """
    source = Path(source_path)
    output_dir = Path(output_dir_str)
    return expand_voice(source, voice_name, preset_slug,
                        expand_mode, output_dir, dry_run=False)


def expand_kit(kit_dir: Path, preset_name: str, expand_mode: str,
               output_dir: Path, dry_run: bool = False,
               workers: int = 1) -> dict:
    """
    Expand an entire flat kit directory into a full set of derived WAVs.
    Returns summary dict.

    workers > 1 enables parallel per-voice processing via ProcessPoolExecutor.
    Each voice is independent (no shared state), so parallelism is safe.
    """
    preset_slug = preset_name.replace(" ", "_")
    if not dry_run:
        output_dir.mkdir(parents=True, exist_ok=True)

    summary = {"preset": preset_name, "mode": expand_mode,
               "voices_found": [], "voices_missing": [], "files_written": []}

    # Collect work items: (source_path, voice_name, effective_mode)
    work_items = []
    for voice_name in VOICE_NAMES:
        source = find_voice_wav(kit_dir, voice_name)
        effective = _resolve_mode(expand_mode, voice_name)
        mode_label = f"[{effective}]"

        if source is None:
            print(f"  [SKIP] {voice_name:6s} — no source WAV found")
            summary["voices_missing"].append(voice_name)
            continue

        print(f"\n  {voice_name:6s} {mode_label:20s} ← {source.name}")
        summary["voices_found"].append(voice_name)
        work_items.append((source, voice_name))

    t_start = time.perf_counter()

    use_parallel = workers > 1 and not dry_run and len(work_items) > 1

    if use_parallel:
        # Parallel per-voice expansion — each voice's DSP is CPU-bound and
        # independent (no shared buffers). ProcessPoolExecutor sidesteps the
        # GIL for Butterworth filters, resampling, and convolution.
        actual_workers = min(workers, len(work_items))
        print(f"\n  [PARALLEL] Using {actual_workers} worker processes")
        with ProcessPoolExecutor(max_workers=actual_workers) as pool:
            futures = {}
            for source, voice_name in work_items:
                fut = pool.submit(
                    _expand_voice_worker,
                    str(source), voice_name, preset_slug,
                    expand_mode, str(output_dir),
                )
                futures[fut] = voice_name

            for fut in as_completed(futures):
                voice_name = futures[fut]
                try:
                    written = fut.result()
                    summary["files_written"].extend(written)
                except Exception as exc:
                    print(f"  [ERROR] {voice_name}: {exc}")
    else:
        # Sequential expansion (original behavior, also used for dry runs)
        for source, voice_name in work_items:
            written = expand_voice(source, voice_name, preset_slug,
                                   expand_mode, output_dir, dry_run)
            summary["files_written"].extend(written)

    elapsed = time.perf_counter() - t_start
    mode_str = "parallel" if use_parallel else "sequential"
    print(f"\n  Total files {'(would be) ' if dry_run else ''}written: "
          f"{len(summary['files_written'])}  "
          f"({mode_str}, {elapsed:.2f}s)")
    return summary


# =============================================================================
# CLI
# =============================================================================

def main():
    parser = argparse.ArgumentParser(
        description="XPN Kit Expander — derive dynamic WAV sets from flat kits",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    src = parser.add_mutually_exclusive_group(required=True)
    src.add_argument("--source",  metavar="WAV",
                     help="Single source WAV to expand")
    src.add_argument("--kit-dir", metavar="DIR",
                     help="Directory of flat kit WAVs (one per voice)")

    parser.add_argument("--preset",     required=True,
                        help="Preset name (used in output filenames)")
    parser.add_argument("--voice",      metavar="VOICE",
                        help="Voice name when using --source (kick/snare/etc.)")
    parser.add_argument("--mode",       default="smart",
                        choices=["velocity", "cycle", "random",
                                 "random-norepeat", "smart"],
                        help="Expansion mode (default: smart)")
    parser.add_argument("--output-dir", default=".",
                        help="Where to write expanded WAV files")
    parser.add_argument("--dry-run",    action="store_true",
                        help="Show what would be written without processing")
    parser.add_argument("--workers",    type=int,
                        default=max(1, os.cpu_count() - 1),
                        help="Number of parallel worker processes "
                             f"(default: {max(1, os.cpu_count() - 1)}, "
                             f"i.e. cpu_count-1)")
    args = parser.parse_args()

    if not SF_AVAILABLE and not args.dry_run:
        print("ERROR: soundfile is required — pip install soundfile")
        return 1
    if not SCIPY_AVAILABLE:
        print("WARNING: scipy not installed — some DSP transforms will be approximate")
        print("         pip install scipy  (recommended)")

    output_dir = Path(args.output_dir)

    if args.source:
        if not args.voice:
            parser.error("--voice is required when using --source")
        if args.voice not in VOICE_NAMES:
            print(f"WARNING: '{args.voice}' not a standard voice name. "
                  f"Known voices: {', '.join(VOICE_NAMES)}")
        if not args.dry_run:
            output_dir.mkdir(parents=True, exist_ok=True)
        preset_slug = args.preset.replace(" ", "_")
        print(f"Expanding {args.source} → {args.voice}  mode={args.mode}")
        expand_voice(Path(args.source), args.voice, preset_slug,
                     args.mode, output_dir, args.dry_run)
    else:
        kit_dir = Path(args.kit_dir)
        print(f"Expanding kit: {kit_dir}  preset='{args.preset}'  mode={args.mode}"
              f"  workers={args.workers}")
        expand_kit(kit_dir, args.preset, args.mode, output_dir, args.dry_run,
                   workers=args.workers)

    return 0


if __name__ == "__main__":
    sys.exit(main())
