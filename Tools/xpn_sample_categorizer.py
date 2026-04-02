#!/usr/bin/env python3
"""
XPN Sample Categorizer — XO_OX Designs
Auto-classify a folder of drum samples into voice types for kit building.

Takes a flat folder of WAVs (typical Drum Broker, Splice, sample pack layout)
and classifies each file into kick/snare/hat/clap/tom/perc/fx categories
using filename analysis + optional audio analysis.

Classification Methods:
  1. Filename keywords — fast, reliable for well-named packs
  2. Audio analysis   — spectral centroid + transient shape (requires scipy)

The output is a classification map that the drum export tools can use directly.

Usage:
    # Classify and print results
    python3 xpn_sample_categorizer.py --dir /path/to/samples

    # Classify and auto-build kit WAV map
    python3 xpn_sample_categorizer.py --dir /path/to/samples \\
        --preset "My Kit" --output-dir /path/to/organized

    # Use audio analysis for unlabeled samples
    python3 xpn_sample_categorizer.py --dir /path/to/samples --analyze
"""

import argparse
import json
import re
import shutil
import sys
from collections import defaultdict
from pathlib import Path
from typing import Optional

# Voice type keywords — ordered by specificity (most specific first)
VOICE_KEYWORDS = {
    "kick":  ["kick", "bd", "bass drum", "bassdrum", "bass_drum", "kik", "kck"],
    "snare": ["snare", "snr", "sd", "snaredrum", "rimshot", "rim"],
    "chat":  ["closed hat", "closed_hat", "closedhat", "ch", "cht", "c_hat",
              "closed hi", "closed_hi", "chihat", "chh"],
    "ohat":  ["open hat", "open_hat", "openhat", "oh", "oht", "o_hat",
              "open hi", "open_hi", "ohihat", "ohh"],
    "clap":  ["clap", "clp", "handclap", "hand_clap"],
    "tom":   ["tom", "ltom", "mtom", "htom", "low_tom", "mid_tom", "high_tom",
              "floor tom", "floor_tom"],
    "perc":  ["perc", "percussion", "shaker", "tambourine", "tamb", "conga",
              "bongo", "clave", "cowbell", "woodblock", "triangle", "guiro",
              "cabasa", "maracas", "agogo", "timbale"],
    "fx":    ["fx", "sfx", "effect", "noise", "riser", "sweep", "impact",
              "crash", "cymbal", "cym", "ride", "splash", "reverse", "rev",
              "vinyl", "crackle", "glitch", "stutter", "tape"],
}

# Hat detection: if "hat" or "hihat" appears without "open"/"closed" qualifier
HAT_GENERIC = ["hat", "hihat", "hi hat", "hi_hat", "hh"]


def classify_by_filename(filename: str) -> Optional[str]:
    """
    Classify a WAV filename into a voice type using word-boundary matching.
    Returns voice name or None if unclassifiable.
    """
    stem = Path(filename).stem.lower().replace("-", "_").replace(" ", "_")
    # Split into tokens for word-boundary matching (prevents "orch" matching "ch")
    tokens = set(re.split(r'[_\s\-\.]+', stem))
    # Also keep the full stem for multi-word keyword matching
    stem_joined = "_".join(tokens)

    # Check each voice type's keywords
    for voice, keywords in VOICE_KEYWORDS.items():
        for kw in keywords:
            kw_clean = kw.replace(" ", "_")
            # Multi-word keywords: check if they appear in the joined stem
            if "_" in kw_clean:
                if kw_clean in stem_joined or kw_clean in stem:
                    return voice
            else:
                # Single-word keywords: must be an exact token match
                # (prevents "ch" matching "orchestra", "oh" matching "method")
                if kw_clean in tokens:
                    return voice

    # Generic hat detection — default to closed if no open/closed qualifier
    for kw in HAT_GENERIC:
        kw_clean = kw.replace(" ", "_")
        if kw_clean in tokens or (kw_clean in stem and len(kw_clean) > 2):
            return "chat"

    return None


def classify_by_audio(wav_path: Path) -> Optional[str]:
    """
    Classify a WAV file by audio analysis.
    Uses spectral centroid and transient shape to guess the drum type.

    Returns voice name or None if can't determine.
    Requires: numpy, soundfile, scipy
    """
    try:
        import numpy as np
        import soundfile as sf
    except ImportError:
        return None

    try:
        data, sr = sf.read(str(wav_path), dtype="float32")
    except Exception:
        return None

    if data.ndim > 1:
        data = data.mean(axis=1)

    if len(data) < 100:
        return None

    # --- Spectral centroid (brightness indicator) ---
    fft = np.abs(np.fft.rfft(data[:min(len(data), sr)]))
    freqs = np.fft.rfftfreq(min(len(data), sr), 1.0 / sr)
    if fft.sum() > 0:
        centroid = np.sum(freqs * fft) / np.sum(fft)
    else:
        centroid = 0

    # --- Duration ---
    duration_ms = len(data) / sr * 1000

    # --- Transient sharpness (how quickly the signal rises) ---
    envelope = np.abs(data)
    peak_idx = np.argmax(envelope[:len(envelope) // 4])  # peak in first quarter
    attack_ms = peak_idx / sr * 1000

    # --- Classification heuristics ---
    # Kick: low centroid (<500 Hz), short attack, medium-long duration
    if centroid < 500 and duration_ms > 50:
        return "kick"

    # Hat: high centroid (>3000 Hz), short duration (<300ms)
    if centroid > 3000 and duration_ms < 300:
        return "chat"

    # Hat (open): high centroid, longer duration
    if centroid > 3000 and duration_ms >= 300:
        return "ohat"

    # Snare: mid centroid (500-3000 Hz), sharp attack
    if 500 <= centroid <= 3000 and attack_ms < 5 and duration_ms < 500:
        return "snare"

    # Clap: mid centroid, slightly longer attack (multi-transient)
    if 500 <= centroid <= 3000 and attack_ms >= 5 and duration_ms < 400:
        return "clap"

    # FX: very long or very high centroid
    if duration_ms > 1000 or centroid > 6000:
        return "fx"

    # Tom: low-mid centroid, medium duration
    if centroid < 1500 and 100 < duration_ms < 600:
        return "tom"

    # Default to perc for anything else with audible content
    if envelope.max() > 0.01:
        return "perc"

    return None


def categorize_folder(
    folder: Path,
    use_audio_analysis: bool = False,
) -> dict:
    """
    Scan a folder and classify all WAV files.

    Returns:
        {
            "kick":  [Path, Path, ...],
            "snare": [Path, ...],
            "chat":  [...],
            "ohat":  [...],
            "clap":  [...],
            "tom":   [...],
            "perc":  [...],
            "fx":    [...],
            "unknown": [...],
        }
    """
    result = defaultdict(list)

    # Deduplicate: macOS case-insensitive FS returns same files for *.wav and *.WAV
    seen = set()
    wavs = []
    for ext in ["*.wav", "*.WAV"]:
        for wf in sorted(folder.glob(ext)):
            if wf.name.lower() not in seen:
                seen.add(wf.name.lower())
                wavs.append(wf)

    for wav in wavs:
        voice = classify_by_filename(wav.name)

        if voice is None and use_audio_analysis:
            voice = classify_by_audio(wav)

        if voice:
            result[voice].append(wav)
        else:
            result["unknown"].append(wav)

    return dict(result)


def build_kit_wav_map(
    categories: dict,
    preset_slug: str,
    max_layers: int = 4,
) -> dict:
    """
    Select the best samples per voice type and build a wav_map
    compatible with xpn_drum_export.

    Strategy:
      - Pick up to max_layers samples per voice
      - Map as velocity layers (v1=quietest file, v4=loudest)
      - If only 1 sample, it becomes v1 (suitable for kit_expander upscaling)

    Returns {stem: filename} map.
    """
    wav_map = {}

    for voice in ["kick", "snare", "chat", "ohat", "clap", "tom", "perc", "fx"]:
        samples = categories.get(voice, [])
        if not samples:
            continue

        selected = samples[:max_layers]

        if len(selected) == 1:
            # Single sample — name as v1, let kit_expander upscale
            key = f"{preset_slug}_{voice}_v1"
            wav_map[key] = selected[0].name
        else:
            # Multiple samples — assign as velocity layers
            for i, wav in enumerate(selected):
                suffix = f"v{i + 1}"
                key = f"{preset_slug}_{voice}_{suffix}"
                wav_map[key] = wav.name

    return wav_map


def organize_samples(
    categories: dict,
    preset_name: str,
    output_dir: Path,
    max_layers: int = 4,
    dry_run: bool = False,
) -> dict:
    """
    Copy and rename samples into the standard XO_OX naming convention.
    Returns the wav_map.
    """
    preset_slug = preset_name.replace(" ", "_")
    if not dry_run:
        output_dir.mkdir(parents=True, exist_ok=True)

    wav_map = {}
    for voice in ["kick", "snare", "chat", "ohat", "clap", "tom", "perc", "fx"]:
        samples = categories.get(voice, [])
        selected = samples[:max_layers]

        for i, wav in enumerate(selected):
            suffix = f"v{i + 1}" if len(selected) > 1 else "v1"
            new_name = f"{preset_slug}_{voice}_{suffix}.wav"

            if not dry_run:
                shutil.copy2(wav, output_dir / new_name)

            wav_map[f"{preset_slug}_{voice}_{suffix}"] = new_name
            label = "[DRY]" if dry_run else "     "
            print(f"  {label} {wav.name:40s} → {new_name}  [{voice}]")

    return wav_map


def print_classification(categories: dict):
    """Pretty-print the classification results."""
    total = sum(len(v) for v in categories.values())
    print(f"\nClassified {total} samples:\n")

    for voice in ["kick", "snare", "chat", "ohat", "clap", "tom", "perc", "fx", "unknown"]:
        samples = categories.get(voice, [])
        if not samples:
            continue
        icon = "?" if voice == "unknown" else "✓"
        print(f"  {icon} {voice:8s} ({len(samples)})")
        for s in samples[:6]:
            print(f"             {s.name}")
        if len(samples) > 6:
            print(f"             ... +{len(samples) - 6} more")
    print()


# =============================================================================
# CLI
# =============================================================================

def main():
    parser = argparse.ArgumentParser(
        description="XPN Sample Categorizer — auto-classify drum samples",
    )
    parser.add_argument("--dir", required=True,
                        help="Folder of WAV samples to classify")
    parser.add_argument("--analyze", action="store_true",
                        help="Use audio analysis for unclassified samples")
    parser.add_argument("--preset", metavar="NAME",
                        help="Preset name — if provided, organizes and renames files")
    parser.add_argument("--output-dir", metavar="DIR",
                        help="Output directory for organized samples")
    parser.add_argument("--max-layers", type=int, default=4,
                        help="Max samples per voice (default: 4)")
    parser.add_argument("--dry-run", action="store_true")
    parser.add_argument("--json", action="store_true",
                        help="Output classification as JSON")
    args = parser.parse_args()

    folder = Path(args.dir)
    if not folder.exists():
        print(f"ERROR: {folder} does not exist")
        return 1

    categories = categorize_folder(folder, use_audio_analysis=args.analyze)

    if args.json:
        json_out = {k: [str(v) for v in vs] for k, vs in categories.items()}
        print(json.dumps(json_out, indent=2))
        return 0

    print_classification(categories)

    if args.preset:
        output_dir = Path(args.output_dir) if args.output_dir else folder / "organized"
        print(f"Organizing as '{args.preset}':\n")
        wav_map = organize_samples(
            categories, args.preset, output_dir,
            max_layers=args.max_layers, dry_run=args.dry_run,
        )
        print(f"\n  {len(wav_map)} files {'would be ' if args.dry_run else ''}organized")
        print(f"  Output: {output_dir}")

        # Save wav_map for downstream tools
        if not args.dry_run:
            map_path = output_dir / "wav_map.json"
            with open(map_path, "w") as f:
                json.dump(wav_map, f, indent=2)
            print(f"  WAV map: {map_path}")

    return 0


if __name__ == "__main__":
    sys.exit(main())
