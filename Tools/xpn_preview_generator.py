#!/usr/bin/env python3
"""
XPN Preview Audio Generator — XO_OX Designs
Generates the .mp3 preview audio file that MPC shows in the expansion browser
when a user is browsing XPN packs. Every commercial MPC expansion has one.

The preview file must have exactly the same base name as the .xpm program file,
with an .mp3 or .wav extension, placed in Programs/ alongside the .xpm.
(Rex's Rule #4: MyKit.xpm → MyKit.mp3)

Processing:
  1. Sample Selection — picks 4-8 representative samples from rendered WAVs
  2. Pattern Sequencing — generates a musical pattern (drum groove or arpeggio)
  3. Audio Rendering — mixes to stereo WAV, normalizes to -1 dB peak
  4. Encoding — converts to MP3 via ffmpeg/lame (fallback to WAV)

Requirements:
    Python 3.10+ (standard library only — uses the `wave` module)
    Optional: ffmpeg or lame on PATH for MP3 encoding

Usage:
    # Generate preview for a drum pack
    python3 xpn_preview_generator.py --samples ./rendered_samples/ \\
        --engine ONSET --pack "XO_OX ONSET Foundation" --output ./preview/

    # Melodic preview with custom tempo
    python3 xpn_preview_generator.py --samples ./rendered_samples/ \\
        --engine OPAL --pack "XO_OX OPAL Atmosphere" --output ./preview/ \\
        --pattern melodic --tempo 90

    # Dry run — show what would be generated
    python3 xpn_preview_generator.py --samples ./rendered_samples/ \\
        --engine ONSET --pack "XO_OX ONSET Foundation" --output ./preview/ \\
        --dry-run
"""

import argparse
import array
import math
import struct
import subprocess
import sys
import wave
from pathlib import Path
from typing import Optional


# =============================================================================
# CONSTANTS
# =============================================================================

# Engines whose packs are drum programs (pattern = drum groove)
DRUM_ENGINES = {"ONSET", "OBESE", "OVERBITE"}

# All other engines default to melodic/keygroup pattern
# (can be overridden with --pattern)

# Drum voice categories — matched by filename keywords (consistent with
# xpn_sample_categorizer.py keyword sets)
DRUM_VOICE_KEYWORDS = {
    "kick":  ["kick", "bd", "bassdrum", "bass_drum", "kik", "kck"],
    "snare": ["snare", "snr", "sd", "rimshot", "rim"],
    "chat":  ["closed_hat", "closedhat", "chat", "c_hat", "chh", "chihat"],
    "ohat":  ["open_hat", "openhat", "ohat", "o_hat", "ohh", "ohihat"],
    "clap":  ["clap", "clp", "handclap"],
    "tom":   ["tom", "ltom", "mtom", "htom"],
    "perc":  ["perc", "shaker", "tamb", "conga", "cowbell", "clave"],
    "fx":    ["fx", "sfx", "crash", "cymbal", "ride", "splash", "reverse"],
}

# Priority order for drum sample selection (core kit first)
DRUM_PRIORITY = ["kick", "snare", "chat", "ohat", "clap", "tom", "perc", "fx"]

# Standard sample rate for preview rendering
PREVIEW_SR = 44100


# =============================================================================
# WAV I/O (standard library only — no numpy/soundfile dependency)
# =============================================================================

def load_wav_samples(path: Path) -> tuple[list[float], int, int]:
    """
    Load a WAV file into a list of float samples (-1.0 to 1.0).
    Returns (samples_mono, sample_rate, n_channels).
    Multi-channel files are mixed to mono.
    """
    with wave.open(str(path), "rb") as wf:
        n_channels = wf.getnchannels()
        sample_width = wf.getsampwidth()
        sr = wf.getframerate()
        n_frames = wf.getnframes()
        raw = wf.readframes(n_frames)

    # Determine struct format
    if sample_width == 1:
        # 8-bit unsigned
        fmt = f"<{n_frames * n_channels}B"
        values = struct.unpack(fmt, raw)
        scale = 128.0
        offset = -1.0
    elif sample_width == 2:
        # 16-bit signed
        fmt = f"<{n_frames * n_channels}h"
        values = struct.unpack(fmt, raw)
        scale = 32768.0
        offset = 0.0
    elif sample_width == 3:
        # 24-bit signed — unpack manually
        values = []
        for i in range(0, len(raw), 3):
            b = raw[i:i+3]
            val = int.from_bytes(b, byteorder="little", signed=True)
            values.append(val)
        scale = 8388608.0
        offset = 0.0
    elif sample_width == 4:
        # 32-bit signed
        fmt = f"<{n_frames * n_channels}i"
        values = struct.unpack(fmt, raw)
        scale = 2147483648.0
        offset = 0.0
    else:
        raise ValueError(f"Unsupported sample width: {sample_width}")

    # Convert to float and mix to mono
    floats = [(v / scale) + offset for v in values]

    if n_channels > 1:
        mono = []
        for i in range(0, len(floats), n_channels):
            frame = floats[i:i + n_channels]
            mono.append(sum(frame) / n_channels)
        return mono, sr, n_channels
    return floats, sr, n_channels


def resample_linear(samples: list[float], src_sr: int, dst_sr: int) -> list[float]:
    """Simple linear interpolation resampler for sample rate conversion."""
    if src_sr == dst_sr:
        return samples
    ratio = src_sr / dst_sr
    out_len = int(len(samples) * dst_sr / src_sr)
    result = []
    for i in range(out_len):
        src_pos = i * ratio
        idx = int(src_pos)
        frac = src_pos - idx
        if idx + 1 < len(samples):
            val = samples[idx] * (1.0 - frac) + samples[idx + 1] * frac
        elif idx < len(samples):
            val = samples[idx]
        else:
            val = 0.0
        result.append(val)
    return result


def write_stereo_wav(path: Path, left: list[float], right: list[float],
                     sr: int = PREVIEW_SR):
    """Write interleaved stereo 16-bit WAV."""
    n_frames = min(len(left), len(right))
    with wave.open(str(path), "wb") as wf:
        wf.setnchannels(2)
        wf.setsampwidth(2)
        wf.setframerate(sr)

        buf = array.array("h", [0] * (n_frames * 2))
        for i in range(n_frames):
            l_val = max(-1.0, min(1.0, left[i]))
            r_val = max(-1.0, min(1.0, right[i]))
            buf[i * 2] = int(l_val * 32767)
            buf[i * 2 + 1] = int(r_val * 32767)
        wf.writeframes(buf.tobytes())


# =============================================================================
# SAMPLE SELECTION
# =============================================================================

def classify_sample(filename: str) -> Optional[str]:
    """Classify a WAV filename into a drum voice category."""
    stem = Path(filename).stem.lower()
    for voice, keywords in DRUM_VOICE_KEYWORDS.items():
        for kw in keywords:
            if kw in stem:
                return voice
    # Generic hat detection
    for kw in ["hat", "hihat", "hi_hat", "hh"]:
        if kw in stem:
            return "chat"  # default to closed hat if ambiguous
    return None


def select_drum_samples(samples_dir: Path) -> dict[str, Path]:
    """
    Pick the most representative sample per drum voice from the directory.
    Prefers samples tagged 'v4' (hard hit) or 'c1' (first round-robin) for
    maximum presence in the preview.
    """
    wavs = sorted(samples_dir.glob("*.wav")) + sorted(samples_dir.glob("*.WAV"))
    if not wavs:
        return {}

    # Group by voice
    by_voice: dict[str, list[Path]] = {}
    for wav in wavs:
        voice = classify_sample(wav.name)
        if voice:
            by_voice.setdefault(voice, []).append(wav)

    selected = {}
    for voice in DRUM_PRIORITY:
        candidates = by_voice.get(voice, [])
        if not candidates:
            continue
        # Prefer v4 (hardest hit) or c1 (first variant) for presence
        best = None
        for pref in ["_v4", "_v3", "_c1", "_v2", "_c2"]:
            for c in candidates:
                if pref in c.stem.lower():
                    best = c
                    break
            if best:
                break
        selected[voice] = best or candidates[0]

    return selected


def select_melodic_samples(samples_dir: Path, max_samples: int = 6
                           ) -> list[Path]:
    """
    Pick representative melodic samples. Prefers root-note samples at
    different velocity layers, or just the first N WAVs sorted by name.
    """
    wavs = sorted(samples_dir.glob("*.wav")) + sorted(samples_dir.glob("*.WAV"))
    if not wavs:
        return []

    # Try to find samples with note names or velocity tags
    tagged = []
    untagged = []
    for w in wavs:
        stem = w.stem.lower()
        # Prefer "foundation" mood samples if available
        if "foundation" in stem:
            tagged.insert(0, w)
        elif any(tag in stem for tag in ["_v3", "_v4", "_c1", "root", "_c2"]):
            tagged.append(w)
        else:
            untagged.append(w)

    # Take from tagged first, then fill from untagged
    result = tagged[:max_samples]
    remaining = max_samples - len(result)
    if remaining > 0:
        # Spread evenly across untagged
        step = max(1, len(untagged) // remaining)
        for i in range(0, len(untagged), step):
            if len(result) >= max_samples:
                break
            result.append(untagged[i])

    return result[:max_samples]


# =============================================================================
# PATTERN SEQUENCING
# =============================================================================

def generate_drum_pattern(tempo: float, duration: float, sr: int,
                          samples: dict[str, list[float]]
                          ) -> tuple[list[float], list[float]]:
    """
    Generate a 2-bar drum groove looped to fill the target duration.

    Pattern (per bar, in 16th-note grid positions 0-15):
      Kick:  0, 8           (beats 1 and 3)
      Snare: 4, 12          (beats 2 and 4)
      CHat:  0,2,4,6,8,10,12,14  (8th notes)
      OHat:  6, 14          (upbeat 8ths — replaces chat on those positions)
      Clap:  12             (beat 4 of bar 2 only — adds variation)

    Bar 2 adds slight variation: ghost kick on position 10.
    """
    beat_dur = 60.0 / tempo  # seconds per beat
    sixteenth = beat_dur / 4.0
    bar_samples = int(beat_dur * 4 * sr)  # samples per bar
    total_samples = int(duration * sr)

    # Build 2 bars of pattern
    two_bar_len = bar_samples * 2
    left = [0.0] * two_bar_len
    right = [0.0] * two_bar_len

    def place(sample: list[float], bar: int, position: int,
              gain: float = 1.0, pan: float = 0.5):
        """Place a sample at a 16th-note position within a bar."""
        start = bar * bar_samples + int(position * sixteenth * sr)
        l_gain = gain * (1.0 - pan) * 2.0
        r_gain = gain * pan * 2.0
        for i, s in enumerate(sample):
            idx = start + i
            if idx < two_bar_len:
                left[idx] += s * l_gain
                right[idx] += s * r_gain

    # Place drum hits
    kick = samples.get("kick", [])
    snare = samples.get("snare", [])
    chat = samples.get("chat", [])
    ohat = samples.get("ohat", [])
    clap = samples.get("clap", [])
    tom = samples.get("tom", [])

    for bar in range(2):
        # Kick on 1 and 3
        if kick:
            place(kick, bar, 0, gain=0.9, pan=0.5)
            place(kick, bar, 8, gain=0.85, pan=0.5)

        # Snare on 2 and 4
        if snare:
            place(snare, bar, 4, gain=0.8, pan=0.5)
            place(snare, bar, 12, gain=0.8, pan=0.5)

        # Closed hats on 8ths (skip positions where open hat plays)
        ohat_positions = {6, 14}
        if chat:
            for pos in range(0, 16, 2):
                if pos not in ohat_positions:
                    place(chat, bar, pos, gain=0.5, pan=0.45)

        # Open hat on upbeats
        if ohat:
            for pos in ohat_positions:
                place(ohat, bar, pos, gain=0.45, pan=0.55)

    # Bar 2 variations
    if clap:
        place(clap, 1, 12, gain=0.6, pan=0.55)
    if kick:
        place(kick, 1, 10, gain=0.4, pan=0.5)  # ghost kick
    if tom:
        place(tom, 1, 14, gain=0.5, pan=0.6)

    # Loop the 2-bar pattern to fill duration
    out_l = [0.0] * total_samples
    out_r = [0.0] * total_samples
    for i in range(total_samples):
        j = i % two_bar_len
        out_l[i] = left[j]
        out_r[i] = right[j]

    return out_l, out_r


def generate_melodic_pattern(tempo: float, duration: float, sr: int,
                             samples: list[list[float]]
                             ) -> tuple[list[float], list[float]]:
    """
    Generate an arpeggiated melodic figure looped to fill duration.

    Uses the provided samples as different "notes" in an arpeggio pattern:
    root → 3rd → 5th → octave → 5th → 3rd (up-down pattern).
    Each note is placed on 8th-note positions with velocity variation.
    """
    if not samples:
        total = int(duration * sr)
        return [0.0] * total, [0.0] * total

    beat_dur = 60.0 / tempo
    eighth = beat_dur / 2.0
    eighth_samples = int(eighth * sr)
    total_samples = int(duration * sr)

    # Build arpeggio pattern indices (up-down through available samples)
    n = len(samples)
    if n >= 4:
        pattern = [0, 1, 2, 3, 2, 1]
    elif n >= 3:
        pattern = [0, 1, 2, 1]
    elif n >= 2:
        pattern = [0, 1, 0, 1]
    else:
        pattern = [0]

    # Velocity variation per step (subtle dynamics)
    velocities = [0.8, 0.6, 0.7, 0.9, 0.65, 0.55]

    # Panning variation (slight stereo movement)
    pans = [0.4, 0.55, 0.6, 0.45, 0.5, 0.35]

    # Build one cycle
    cycle_len = len(pattern) * eighth_samples
    left = [0.0] * cycle_len
    right = [0.0] * cycle_len

    for step_idx, sample_idx in enumerate(pattern):
        sample = samples[sample_idx % n]
        start = step_idx * eighth_samples
        gain = velocities[step_idx % len(velocities)]
        pan = pans[step_idx % len(pans)]
        l_gain = gain * (1.0 - pan) * 2.0
        r_gain = gain * pan * 2.0

        for i, s in enumerate(sample):
            idx = start + i
            if idx < cycle_len:
                left[idx] += s * l_gain
                right[idx] += s * r_gain

    # Loop to fill duration
    out_l = [0.0] * total_samples
    out_r = [0.0] * total_samples
    for i in range(total_samples):
        j = i % cycle_len
        out_l[i] = left[j]
        out_r[i] = right[j]

    return out_l, out_r


# =============================================================================
# POST-PROCESSING
# =============================================================================

def apply_fades(left: list[float], right: list[float], sr: int,
                fade_in_s: float = 0.5, fade_out_s: float = 2.0):
    """Apply fade-in and fade-out to stereo buffers (in-place)."""
    n = len(left)
    fade_in_samples = min(int(fade_in_s * sr), n // 4)
    fade_out_samples = min(int(fade_out_s * sr), n // 2)

    # Fade in (linear)
    for i in range(fade_in_samples):
        gain = i / fade_in_samples
        left[i] *= gain
        right[i] *= gain

    # Fade out (cosine — smoother tail)
    for i in range(fade_out_samples):
        t = i / fade_out_samples
        gain = 0.5 * (1.0 + math.cos(math.pi * t))
        idx = n - fade_out_samples + i
        if idx >= 0:
            left[idx] *= gain
            right[idx] *= gain


def normalize_to_db(left: list[float], right: list[float],
                    target_db: float = -1.0):
    """Normalize stereo buffers to target peak dB (in-place)."""
    peak = 0.0
    for i in range(len(left)):
        peak = max(peak, abs(left[i]), abs(right[i]))

    if peak < 1e-10:
        return  # silence — nothing to normalize

    target_linear = 10.0 ** (target_db / 20.0)
    gain = target_linear / peak

    for i in range(len(left)):
        left[i] *= gain
        right[i] *= gain


# =============================================================================
# MP3 ENCODING
# =============================================================================

def encode_mp3(wav_path: Path, mp3_path: Path, bitrate: int = 192) -> bool:
    """
    Convert WAV to MP3 using ffmpeg or lame.
    Returns True on success, False if no encoder found.
    """
    # Try ffmpeg first
    try:
        result = subprocess.run(
            ["ffmpeg", "-y", "-i", str(wav_path),
             "-b:a", f"{bitrate}k", "-q:a", "2", str(mp3_path)],
            capture_output=True, text=True, timeout=30,
        )
        if result.returncode == 0:
            return True
    except (FileNotFoundError, subprocess.TimeoutExpired):
        pass

    # Try lame
    try:
        result = subprocess.run(
            ["lame", "-b", str(bitrate), str(wav_path), str(mp3_path)],
            capture_output=True, text=True, timeout=30,
        )
        if result.returncode == 0:
            return True
    except (FileNotFoundError, subprocess.TimeoutExpired):
        pass

    return False


# =============================================================================
# MAIN GENERATOR
# =============================================================================

def detect_pattern_type(engine: str) -> str:
    """Auto-detect pattern type from engine name."""
    if engine.upper() in DRUM_ENGINES:
        return "drum"
    return "melodic"


def generate_preview(
    samples_dir: Path,
    engine: str,
    pack_name: str,
    output_dir: Path,
    tempo: float = 120.0,
    duration: float = 30.0,
    output_format: str = "mp3",
    pattern_type: str = "auto",
    dry_run: bool = False,
) -> Optional[Path]:
    """
    Generate a preview audio file for an XPN expansion pack.

    Args:
        samples_dir:    Directory of rendered WAV samples
        engine:         Engine name (e.g. "ONSET", "OPAL")
        pack_name:      Pack name (used for output filename)
        output_dir:     Where to write the preview file
        tempo:          BPM for the pattern
        duration:       Target duration in seconds
        output_format:  "mp3" or "wav"
        pattern_type:   "drum", "melodic", or "auto"
        dry_run:        If True, show plan without generating

    Returns:
        Path to the generated preview file, or None on failure.
    """
    # Resolve pattern type
    if pattern_type == "auto":
        pattern_type = detect_pattern_type(engine)

    # Determine output filename — matches the program .xpm base name
    # (Rex's Rule #4: MyKit.xpm → MyKit.mp3)
    pack_slug = pack_name.replace(" ", "_")

    print(f"\n  ╔══════════════════════════════════════════════╗")
    print(f"  ║  XPN Preview Generator — XO_OX Designs       ║")
    print(f"  ╚══════════════════════════════════════════════╝")
    print(f"\n  Engine:   {engine}")
    print(f"  Pack:     {pack_name}")
    print(f"  Pattern:  {pattern_type}")
    print(f"  Tempo:    {tempo} BPM")
    print(f"  Duration: {duration}s")
    print(f"  Format:   {output_format}")

    # --- Sample Selection ---
    if pattern_type == "drum":
        selected = select_drum_samples(samples_dir)
        if not selected:
            print("\n  [ERROR] No drum samples found — cannot generate preview")
            return None
        print(f"\n  Selected {len(selected)} drum voices:")
        for voice, path in selected.items():
            print(f"    {voice:6s} → {path.name}")
    else:
        selected_paths = select_melodic_samples(samples_dir)
        if not selected_paths:
            print("\n  [ERROR] No melodic samples found — cannot generate preview")
            return None
        print(f"\n  Selected {len(selected_paths)} melodic samples:")
        for p in selected_paths:
            print(f"    → {p.name}")

    if dry_run:
        ext = output_format if output_format in ("mp3", "wav") else "mp3"
        print(f"\n  [DRY RUN] Would generate: {pack_slug}.{ext}")
        print(f"  [DRY RUN] Output dir: {output_dir}")
        return None

    # --- Load Samples ---
    print("\n  Loading samples...")
    sr = PREVIEW_SR

    if pattern_type == "drum":
        loaded: dict[str, list[float]] = {}
        for voice, path in selected.items():
            try:
                samples_data, file_sr, _ = load_wav_samples(path)
                if file_sr != sr:
                    samples_data = resample_linear(samples_data, file_sr, sr)
                loaded[voice] = samples_data
                print(f"    {voice:6s}  {len(samples_data)} samples @ {file_sr}Hz")
            except Exception as e:
                print(f"    [WARN] Failed to load {path.name}: {e}")
    else:
        loaded_list: list[list[float]] = []
        for path in selected_paths:
            try:
                samples_data, file_sr, _ = load_wav_samples(path)
                if file_sr != sr:
                    samples_data = resample_linear(samples_data, file_sr, sr)
                loaded_list.append(samples_data)
                print(f"    {path.name}  {len(samples_data)} samples @ {file_sr}Hz")
            except Exception as e:
                print(f"    [WARN] Failed to load {path.name}: {e}")

    # --- Sequencing ---
    print("\n  Sequencing pattern...")
    if pattern_type == "drum":
        if not loaded:
            print("  [ERROR] No samples loaded successfully")
            return None
        left, right = generate_drum_pattern(tempo, duration, sr, loaded)
    else:
        if not loaded_list:
            print("  [ERROR] No samples loaded successfully")
            return None
        left, right = generate_melodic_pattern(tempo, duration, sr, loaded_list)

    # --- Post-Processing ---
    print("  Applying fades...")
    apply_fades(left, right, sr, fade_in_s=0.5, fade_out_s=2.0)

    print("  Normalizing to -1 dB...")
    normalize_to_db(left, right, target_db=-1.0)

    # --- Write Output ---
    output_dir.mkdir(parents=True, exist_ok=True)

    wav_path = output_dir / f"{pack_slug}.wav"
    print(f"\n  Writing WAV: {wav_path.name}")
    write_stereo_wav(wav_path, left, right, sr)

    final_path = wav_path

    if output_format == "mp3":
        mp3_path = output_dir / f"{pack_slug}.mp3"
        print(f"  Encoding MP3: {mp3_path.name}")
        if encode_mp3(wav_path, mp3_path):
            final_path = mp3_path
            # Remove intermediate WAV
            wav_path.unlink()
            print("  MP3 encoding successful")
        else:
            print("  [WARN] No MP3 encoder found (ffmpeg/lame) — keeping WAV")
            print("         Install ffmpeg: brew install ffmpeg")
            final_path = wav_path

    size_kb = final_path.stat().st_size / 1024
    unit = "KB" if size_kb < 1024 else "MB"
    size_val = size_kb if size_kb < 1024 else size_kb / 1024
    print(f"\n  Output: {final_path}  ({size_val:.1f} {unit})")
    print(f"  Duration: {duration}s @ {tempo} BPM")
    print(f"  Preview ready for XPN packager ✓")

    return final_path


# =============================================================================
# CLI
# =============================================================================

def main():
    parser = argparse.ArgumentParser(
        description="XPN Preview Audio Generator — generate MPC expansion "
                    "browser preview files",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    parser.add_argument("--samples",  required=True, metavar="DIR",
                        help="Directory of rendered WAV samples")
    parser.add_argument("--engine",   required=True,
                        help="Engine name (e.g. ONSET, OPAL, OVERWORLD)")
    parser.add_argument("--pack",     required=True,
                        help="Pack name (used for output filename)")
    parser.add_argument("--output",   required=True, metavar="DIR",
                        help="Output directory for preview file")
    parser.add_argument("--tempo",    type=float, default=120.0,
                        help="Pattern tempo in BPM (default: 120)")
    parser.add_argument("--duration", type=float, default=30.0,
                        help="Target duration in seconds (default: 30)")
    parser.add_argument("--format",   choices=["mp3", "wav"], default="mp3",
                        dest="output_format",
                        help="Output format (default: mp3, fallback to wav)")
    parser.add_argument("--pattern",  choices=["drum", "melodic", "auto"],
                        default="auto",
                        help="Pattern type (default: auto-detect from engine)")
    parser.add_argument("--dry-run",  action="store_true",
                        help="Show what would be generated without processing")
    args = parser.parse_args()

    samples_dir = Path(args.samples)
    if not samples_dir.is_dir():
        print(f"  [ERROR] Samples directory not found: {samples_dir}")
        return 1

    result = generate_preview(
        samples_dir=samples_dir,
        engine=args.engine,
        pack_name=args.pack,
        output_dir=Path(args.output),
        tempo=args.tempo,
        duration=args.duration,
        output_format=args.output_format,
        pattern_type=args.pattern,
        dry_run=args.dry_run,
    )

    if result is None and not args.dry_run:
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
