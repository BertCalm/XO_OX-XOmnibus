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
import os
import subprocess
import sys
from pathlib import Path

# Canonical engine names (lowercase for filenames)
ENGINES = [
    "oddfelix",
    "oddoscar",
    "overdub",
    "odyssey",
    "oblong",
    "obese",
    "onset",
    "overworld",
    "opal",
    "overbite",
    "orbital",
    "organon",
    "ouroboros",
    "obsidian",
    "origami",
    "oracle",
    "obscura",
    "oceanic",
    "optic",
    "oblique",
    "ombre",
]

ENGINE_DISPLAY = {
    "oddfelix": "OddfeliX",
    "oddoscar": "OddOscar",
    "overdub": "Overdub",
    "odyssey": "Odyssey",
    "oblong": "Oblong",
    "obese": "Obese",
    "onset": "Onset",
    "overworld": "Overworld",
    "opal": "Opal",
    "overbite": "Overbite",
    "orbital": "Orbital",
    "organon": "Organon",
    "ouroboros": "Ouroboros",
    "obsidian": "Obsidian",
    "origami": "Origami",
    "oracle": "Oracle",
    "obscura": "Obscura",
    "oceanic": "Oceanic",
    "optic": "Optic",
    "oblique": "Oblique",
    "ombre": "Ombre",
}


def check_ffmpeg():
    """Verify ffmpeg is installed."""
    try:
        subprocess.run(
            ["ffmpeg", "-version"],
            capture_output=True,
            check=True,
        )
        return True
    except (subprocess.CalledProcessError, FileNotFoundError):
        return False


def process_wav(input_path, output_dir, engine_name, args):
    """Process a single WAV file into MP3 and OGG."""
    results = []

    # Build ffmpeg filter chain
    filters = []

    # Trim to max duration
    trim = f"atrim=0:{args.duration}"
    filters.append(trim)

    # Fade in/out
    if args.fade_in > 0:
        filters.append(f"afade=t=in:d={args.fade_in}")
    if args.fade_out > 0:
        filters.append(f"afade=t=out:st={args.duration - args.fade_out}:d={args.fade_out}")

    # Normalize
    if args.normalize:
        filters.append("loudnorm=I=-16:TP=-1:LRA=11")

    filter_chain = ",".join(filters)

    # MP3 output (128kbps CBR for consistent quality)
    mp3_path = output_dir / f"{engine_name}.mp3"
    mp3_cmd = [
        "ffmpeg", "-y",
        "-i", str(input_path),
        "-af", filter_chain,
        "-codec:a", "libmp3lame",
        "-b:a", "128k",
        "-ar", "44100",
        "-ac", "2",
        str(mp3_path),
    ]

    try:
        subprocess.run(mp3_cmd, capture_output=True, check=True)
        size_kb = mp3_path.stat().st_size / 1024
        results.append(("MP3", mp3_path, size_kb))
    except subprocess.CalledProcessError as e:
        print(f"  ERROR (MP3): {e.stderr.decode()[-200:]}")
        return results

    # OGG output (quality 4 ≈ 128kbps)
    if not args.skip_ogg:
        ogg_path = output_dir / f"{engine_name}.ogg"
        ogg_cmd = [
            "ffmpeg", "-y",
            "-i", str(input_path),
            "-af", filter_chain,
            "-codec:a", "libvorbis",
            "-q:a", "4",
            "-ar", "44100",
            "-ac", "2",
            str(ogg_path),
        ]

        try:
            subprocess.run(ogg_cmd, capture_output=True, check=True)
            size_kb = ogg_path.stat().st_size / 1024
            results.append(("OGG", ogg_path, size_kb))
        except subprocess.CalledProcessError as e:
            print(f"  ERROR (OGG): {e.stderr.decode()[-200:]}")

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
        for eng in ENGINES:
            print(f"  {eng}.wav  — {ENGINE_DISPLAY[eng]}")
        print(f"\n{len(ENGINES)} engines total")
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

    for eng in ENGINES:
        display = ENGINE_DISPLAY[eng]
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
    print(f"Processed: {processed}/{len(ENGINES)} engines")
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
