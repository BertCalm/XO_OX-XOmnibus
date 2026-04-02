#!/usr/bin/env python3
"""
site_audio_export.py — XO_OX Brand Site Audio Exporter

Processes raw WAV recordings from standalone synth plugins into web-ready
MP3 and OGG files for the XO-OX.org brand site engine previews.

Usage:
  1. Record 2-3 second clips from each engine's standalone app
  2. Save WAVs to Tools/site_audio_raw/ with engine name:
       oddfelix.wav, oddoscar.wav, overdub.wav, etc.
  3. Run:  python3 Tools/site_audio_export.py
  4. Output lands in site/audio/

Requirements:
  - ffmpeg (brew install ffmpeg)

Options:
  --input DIR     Input directory (default: Tools/site_audio_raw/)
  --output DIR    Output directory (default: site/audio/)
  --duration SEC  Max duration in seconds (default: 3.0)
  --fade-in SEC   Fade-in duration (default: 0.02)
  --fade-out SEC  Fade-out duration (default: 0.4)
  --normalize     Peak normalize to -1dB (default: on)
  --skip-ogg      Skip OGG generation (MP3 only)
  --list          List expected engine names and exit
"""

import argparse
import shutil
import subprocess
import sys
from pathlib import Path

from engine_registry import get_all_engines

# Canonical engine names: (lowercase_key, display_name)
# Sourced from engine_registry.py — do NOT maintain a local list here.
ENGINE_NAMES = [(name.lower(), name) for name in get_all_engines()]

SAMPLE_RATE = "44100"
CHANNELS = "2"

# Output format configurations: (extension, codec, quality_flags)
OUTPUT_FORMATS = [
    ("mp3", "libmp3lame", ["-b:a", "128k"]),
    ("ogg", "libvorbis", ["-q:a", "4"]),
]


def check_ffmpeg():
    """Verify ffmpeg is installed."""
    return shutil.which("ffmpeg") is not None


def _clamp_float(val: float, lo: float, hi: float, name: str) -> float:
    """Return val clamped to [lo, hi], raising ValueError if already out of range.

    argparse enforces type=float, but an adversarial config file loaded
    by a wrapper could bypass argparse. Explicit clamps keep ffmpeg filter
    chain strings predictable (issue #430).
    """
    if not (lo <= val <= hi):
        raise ValueError(f"{name}={val!r} is outside the allowed range [{lo}, {hi}]")
    return val


def process_wav(input_path, output_dir, engine_name, args):
    """Process a single WAV file into MP3 and OGG."""
    results = []

    # Guard ffmpeg filter chain parameters (issue #430).
    # argparse already enforces type=float, but clamp explicitly to keep
    # the constructed filter string predictable and well-formed.
    duration = _clamp_float(float(args.duration), 0.1, 300.0, "duration")
    fade_in  = _clamp_float(float(args.fade_in),  0.0,  10.0, "fade_in")
    fade_out = _clamp_float(float(args.fade_out), 0.0,  10.0, "fade_out")

    # Build ffmpeg filter chain
    filters = [f"atrim=0:{duration}"]
    if fade_in > 0:
        filters.append(f"afade=t=in:d={fade_in}")
    if fade_out > 0:
        filters.append(f"afade=t=out:st={duration - fade_out}:d={fade_out}")
    if args.normalize:
        filters.append("loudnorm=I=-16:TP=-1:LRA=11")
    filter_chain = ",".join(filters)

    formats = OUTPUT_FORMATS if not args.skip_ogg else OUTPUT_FORMATS[:1]
    for ext, codec, quality_flags in formats:
        out_path = output_dir / f"{engine_name}.{ext}"
        cmd = [
            "ffmpeg", "-y",
            "-i", str(input_path),
            "-af", filter_chain,
            "-codec:a", codec,
            *quality_flags,
            "-ar", SAMPLE_RATE,
            "-ac", CHANNELS,
            str(out_path),
        ]
        try:
            subprocess.run(cmd, capture_output=True, check=True)
            size_kb = out_path.stat().st_size / 1024
            results.append((ext.upper(), out_path, size_kb))
        except subprocess.CalledProcessError as e:
            print(f"  ERROR ({ext.upper()}): {e.stderr.decode()[-200:]}")
            if ext == "mp3":
                return results

    return results


def main():
    parser = argparse.ArgumentParser(
        description="XO_OX Brand Site Audio Exporter"
    )
    parser.add_argument(
        "--input",
        default="Tools/site_audio_raw",
        help="Input directory with WAV files",
    )
    parser.add_argument(
        "--output",
        default="site/audio",
        help="Output directory for web-ready files",
    )
    parser.add_argument(
        "--duration",
        type=float,
        default=3.0,
        help="Max clip duration in seconds",
    )
    parser.add_argument(
        "--fade-in",
        type=float,
        default=0.02,
        help="Fade-in duration in seconds",
    )
    parser.add_argument(
        "--fade-out",
        type=float,
        default=0.4,
        help="Fade-out duration in seconds",
    )
    parser.add_argument(
        "--normalize",
        action="store_true",
        default=True,
        help="Loudness normalize (default: on)",
    )
    parser.add_argument(
        "--no-normalize",
        action="store_false",
        dest="normalize",
        help="Disable loudness normalization",
    )
    parser.add_argument(
        "--skip-ogg",
        action="store_true",
        default=False,
        help="Skip OGG generation",
    )
    parser.add_argument(
        "--list",
        action="store_true",
        help="List expected engine filenames and exit",
    )

    args = parser.parse_args()

    if args.list:
        print("Expected WAV files (place in Tools/site_audio_raw/):\n")
        for key, display in ENGINE_NAMES:
            print(f"  {key}.wav  — {display}")
        print(f"\n{len(ENGINE_NAMES)} engines total")
        return

    # Check ffmpeg
    if not check_ffmpeg():
        print("ERROR: ffmpeg not found. Install with: brew install ffmpeg")
        sys.exit(1)

    # Resolve paths relative to repo root
    repo_root = Path(__file__).resolve().parent.parent
    input_dir = repo_root / args.input
    output_dir = repo_root / args.output

    if not input_dir.exists():
        print(f"Input directory not found: {input_dir}")
        print(f"Create it and add WAV files:")
        print(f"  mkdir -p {input_dir}")
        print(f"  # Then record clips from each standalone app")
        print(f"\nRun with --list to see expected filenames.")
        sys.exit(1)

    output_dir.mkdir(parents=True, exist_ok=True)

    # Scan for WAV files
    found = {}
    for f in input_dir.iterdir():
        if f.suffix.lower() == ".wav":
            name = f.stem.lower()
            found[name] = f

    if not found:
        print(f"No WAV files found in {input_dir}")
        print("Run with --list to see expected filenames.")
        sys.exit(1)

    # Process
    print(f"XO_OX Site Audio Export")
    print(f"{'=' * 50}")
    print(f"Input:    {input_dir}")
    print(f"Output:   {output_dir}")
    print(f"Duration: {args.duration}s | Fade: {args.fade_in}s in, {args.fade_out}s out")
    print(f"Normalize: {'yes (loudnorm -16 LUFS)' if args.normalize else 'no'}")
    print(f"Formats:  MP3{'' if args.skip_ogg else ' + OGG'}")
    print(f"{'=' * 50}\n")

    total_size = 0
    processed = 0
    missing = []

    for eng, display in ENGINE_NAMES:
        if eng in found:
            print(f"  {display:12s} ", end="", flush=True)
            results = process_wav(found[eng], output_dir, eng, args)
            if results:
                sizes = " | ".join(f"{fmt}: {sz:.0f}KB" for fmt, _, sz in results)
                total_size += sum(sz for _, _, sz in results)
                print(f"OK  ({sizes})")
                processed += 1
            else:
                print("FAILED")
        else:
            missing.append(display)

    print(f"\n{'=' * 50}")
    print(f"Processed: {processed}/{len(ENGINE_NAMES)} engines")
    print(f"Total size: {total_size:.0f}KB ({total_size/1024:.1f}MB)")

    if missing:
        print(f"\nMissing ({len(missing)}):")
        for m in missing:
            print(f"  - {m}")

    if processed > 0:
        print(f"\nFiles ready in: {output_dir}/")
        print("The brand site will auto-detect and use these samples.")


if __name__ == "__main__":
    main()
