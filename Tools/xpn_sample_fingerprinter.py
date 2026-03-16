"""
xpn_sample_fingerprinter.py — Perceptual near-duplicate detector for .xpn sample packs.

SHA-256 only catches exact duplicates. This tool catches perceptual near-duplicates:
the same sample slightly processed, resampled, normalized, or pitch-shifted.

Algorithm:
  1. Read first 4096 PCM frames from each WAV (mono mix if stereo)
  2. Compute real DFT over those frames
  3. Divide frequency bins into 8 logarithmically-spaced bands
  4. Compute normalized RMS energy per band (0.0–1.0)
  5. Quantize each band to 4 bits (0–15) → pack into 32-bit integer fingerprint
  6. Compare all pairs via Hamming distance (bit difference count)
  7. Pairs with distance ≤ threshold are flagged as near-duplicates

Usage:
  python xpn_sample_fingerprinter.py --xpn pack.xpn [--threshold 4] [--format text|json] [--show-all]

  --xpn        Path to .xpn file (ZIP-based pack)
  --threshold  Hamming distance cutoff (default: 4 bits out of 32)
  --format     Output format: text (default) or json
  --show-all   Print full fingerprint table for all samples, not just duplicate pairs
"""

import argparse
import json
import math
import struct
import sys
import tempfile
import wave
import zipfile
from pathlib import Path

# ---------------------------------------------------------------------------
# Constants
# ---------------------------------------------------------------------------

FRAME_COUNT = 4096          # PCM frames to fingerprint
NUM_BANDS = 8               # Spectral bands
BITS_PER_BAND = 4           # Quantization depth per band (0–15)
FINGERPRINT_BITS = NUM_BANDS * BITS_PER_BAND   # 32 bits total


# ---------------------------------------------------------------------------
# PCM extraction
# ---------------------------------------------------------------------------

def read_pcm_frames(wav_path: str, max_frames: int = FRAME_COUNT) -> list[float]:
    """Return up to max_frames mono-mixed, normalized float samples from a WAV file."""
    with wave.open(wav_path, "rb") as wf:
        n_channels = wf.getnchannels()
        sampwidth = wf.getsampwidth()
        n_frames = wf.getnframes()
        frames_to_read = min(n_frames, max_frames)
        raw = wf.readframes(frames_to_read)

    # Parse raw bytes into integers
    if sampwidth == 1:
        # 8-bit WAV is unsigned (0–255), center at 128
        fmt = f"{len(raw)}B"
        samples_raw = list(struct.unpack(fmt, raw))
        samples_int = [(s - 128) for s in samples_raw]
        max_val = 128.0
    elif sampwidth == 2:
        n_samples = len(raw) // 2
        fmt = f"<{n_samples}h"
        samples_int = list(struct.unpack(fmt, raw))
        max_val = 32768.0
    elif sampwidth == 3:
        # 24-bit: unpack manually
        samples_int = []
        for i in range(0, len(raw) - 2, 3):
            b0, b1, b2 = raw[i], raw[i + 1], raw[i + 2]
            val = b0 | (b1 << 8) | (b2 << 16)
            if val >= 0x800000:
                val -= 0x1000000
            samples_int.append(val)
        max_val = 8388608.0
    elif sampwidth == 4:
        n_samples = len(raw) // 4
        fmt = f"<{n_samples}i"
        samples_int = list(struct.unpack(fmt, raw))
        max_val = 2147483648.0
    else:
        raise ValueError(f"Unsupported sample width: {sampwidth} bytes")

    # Normalize to [-1.0, 1.0]
    normalized = [s / max_val for s in samples_int]

    # Mono mix: average channels
    if n_channels > 1:
        mono = []
        for i in range(0, len(normalized) - (n_channels - 1), n_channels):
            frame = normalized[i:i + n_channels]
            mono.append(sum(frame) / n_channels)
        normalized = mono

    return normalized[:max_frames]


# ---------------------------------------------------------------------------
# Spectral analysis
# ---------------------------------------------------------------------------

def real_dft_magnitudes(samples: list[float]) -> list[float]:
    """
    Compute magnitude spectrum via real DFT (no FFT library needed).
    Returns N/2 magnitude values for N input samples.
    Only upper half of spectrum is meaningful for real signals.
    """
    n = len(samples)
    half = n // 2
    magnitudes = []
    two_pi_over_n = 2.0 * math.pi / n
    for k in range(half):
        re = 0.0
        im = 0.0
        freq = two_pi_over_n * k
        for t, x in enumerate(samples):
            angle = freq * t
            re += x * math.cos(angle)
            im -= x * math.sin(angle)
        magnitudes.append(math.sqrt(re * re + im * im))
    return magnitudes


def band_rms_energy(magnitudes: list[float], num_bands: int = NUM_BANDS) -> list[float]:
    """
    Divide magnitude spectrum into num_bands logarithmically-spaced bands.
    Return RMS energy per band, normalized so the max band = 1.0.
    """
    n = len(magnitudes)
    if n == 0:
        return [0.0] * num_bands

    # Log-spaced bin edges from bin 1 to bin n (avoid DC at 0)
    log_min = math.log(1.0)
    log_max = math.log(float(n))
    edges = [
        int(math.exp(log_min + (log_max - log_min) * i / num_bands))
        for i in range(num_bands + 1)
    ]
    # Ensure last edge reaches n
    edges[-1] = n

    band_energies = []
    for b in range(num_bands):
        lo = edges[b]
        hi = edges[b + 1]
        if hi <= lo:
            hi = lo + 1
        band_mags = magnitudes[lo:hi]
        if not band_mags:
            band_energies.append(0.0)
        else:
            rms = math.sqrt(sum(m * m for m in band_mags) / len(band_mags))
            band_energies.append(rms)

    # Normalize so max band = 1.0 (relative spectral shape, not absolute level)
    max_e = max(band_energies) if any(e > 0 for e in band_energies) else 1.0
    if max_e > 0:
        band_energies = [e / max_e for e in band_energies]

    return band_energies


# ---------------------------------------------------------------------------
# Fingerprint encoding / comparison
# ---------------------------------------------------------------------------

def encode_fingerprint(band_energies: list[float]) -> int:
    """Pack 8 × 4-bit band values into a 32-bit integer."""
    fingerprint = 0
    for i, energy in enumerate(band_energies):
        quantized = min(15, int(energy * 15.0 + 0.5))
        fingerprint |= (quantized & 0xF) << (i * BITS_PER_BAND)
    return fingerprint


def hamming_distance(a: int, b: int) -> int:
    """Count differing bits between two 32-bit fingerprints."""
    xor = (a ^ b) & 0xFFFFFFFF
    count = 0
    while xor:
        count += xor & 1
        xor >>= 1
    return count


def decode_bands(fingerprint: int) -> list[int]:
    """Extract 8 band quantized values from fingerprint integer."""
    return [(fingerprint >> (i * BITS_PER_BAND)) & 0xF for i in range(NUM_BANDS)]


# ---------------------------------------------------------------------------
# Pack processing
# ---------------------------------------------------------------------------

def fingerprint_xpn(xpn_path: str) -> list[dict]:
    """
    Extract WAV files from an .xpn pack, fingerprint each one.
    Returns list of dicts: {name, fingerprint, bands, error}.
    """
    results = []
    with zipfile.ZipFile(xpn_path, "r") as zf:
        wav_entries = [e for e in zf.namelist() if e.lower().endswith(".wav")]
        if not wav_entries:
            print("Warning: no .wav files found in pack.", file=sys.stderr)
            return results

        with tempfile.TemporaryDirectory() as tmpdir:
            for entry in sorted(wav_entries):
                name = Path(entry).name
                try:
                    extracted = zf.extract(entry, tmpdir)
                    samples = read_pcm_frames(extracted)
                    if not samples:
                        raise ValueError("Empty PCM data")
                    magnitudes = real_dft_magnitudes(samples)
                    band_energies = band_rms_energy(magnitudes)
                    fp = encode_fingerprint(band_energies)
                    results.append({
                        "name": name,
                        "path": entry,
                        "fingerprint": fp,
                        "bands": band_energies,
                        "error": None,
                    })
                except Exception as exc:
                    results.append({
                        "name": name,
                        "path": entry,
                        "fingerprint": None,
                        "bands": None,
                        "error": str(exc),
                    })
    return results


def find_near_duplicates(records: list[dict], threshold: int) -> list[dict]:
    """Return all pairs where Hamming distance ≤ threshold."""
    valid = [r for r in records if r["fingerprint"] is not None]
    pairs = []
    for i in range(len(valid)):
        for j in range(i + 1, len(valid)):
            dist = hamming_distance(valid[i]["fingerprint"], valid[j]["fingerprint"])
            if dist <= threshold:
                pairs.append({
                    "file_a": valid[i]["name"],
                    "path_a": valid[i]["path"],
                    "file_b": valid[j]["name"],
                    "path_b": valid[j]["path"],
                    "distance": dist,
                    "fp_a": valid[i]["fingerprint"],
                    "fp_b": valid[j]["fingerprint"],
                })
    pairs.sort(key=lambda p: p["distance"])
    return pairs


# ---------------------------------------------------------------------------
# Output formatting
# ---------------------------------------------------------------------------

def format_bands(bands: list[float]) -> str:
    return " ".join(f"{v:.2f}" for v in bands)


def print_text(records: list[dict], pairs: list[dict], show_all: bool, threshold: int) -> None:
    if show_all:
        print(f"\n{'FILE':<40} {'HEX FP':>10}  BANDS (8 × normalized RMS)")
        print("-" * 80)
        for r in records:
            if r["error"]:
                print(f"  {'[ERROR] ' + r['name']:<40}  {r['error']}")
            else:
                hex_fp = f"0x{r['fingerprint']:08X}"
                print(f"  {r['name']:<40} {hex_fp:>10}  {format_bands(r['bands'])}")

    print(f"\nNear-duplicate pairs (Hamming distance ≤ {threshold} bits out of {FINGERPRINT_BITS}):")
    if not pairs:
        print("  None found.")
    else:
        print(f"  {'FILE A':<38} {'FILE B':<38} DIST")
        print("  " + "-" * 84)
        for p in pairs:
            print(f"  {p['file_a']:<38} {p['file_b']:<38} {p['distance']:>4} bits")

    valid = sum(1 for r in records if r["fingerprint"] is not None)
    errors = len(records) - valid
    print(f"\nSummary: {valid} samples fingerprinted, {errors} errors, {len(pairs)} near-duplicate pairs.")


def print_json(records: list[dict], pairs: list[dict]) -> None:
    output = {
        "fingerprints": [
            {
                "name": r["name"],
                "path": r["path"],
                "fingerprint_hex": f"0x{r['fingerprint']:08X}" if r["fingerprint"] is not None else None,
                "fingerprint_int": r["fingerprint"],
                "bands": [round(b, 4) for b in r["bands"]] if r["bands"] else None,
                "quantized_bands": decode_bands(r["fingerprint"]) if r["fingerprint"] is not None else None,
                "error": r["error"],
            }
            for r in records
        ],
        "near_duplicates": [
            {
                "file_a": p["file_a"],
                "file_b": p["file_b"],
                "hamming_distance": p["distance"],
                "fp_a_hex": f"0x{p['fp_a']:08X}",
                "fp_b_hex": f"0x{p['fp_b']:08X}",
            }
            for p in pairs
        ],
    }
    print(json.dumps(output, indent=2))


# ---------------------------------------------------------------------------
# CLI entry point
# ---------------------------------------------------------------------------

def main() -> None:
    parser = argparse.ArgumentParser(
        description="Perceptual near-duplicate detector for .xpn sample packs.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    parser.add_argument("--xpn", required=True, help="Path to .xpn pack file")
    parser.add_argument("--threshold", type=int, default=4,
                        help="Hamming distance threshold for near-duplicate detection (default: 4)")
    parser.add_argument("--format", choices=["text", "json"], default="text",
                        help="Output format (default: text)")
    parser.add_argument("--show-all", action="store_true",
                        help="Print full fingerprint table for all samples")
    args = parser.parse_args()

    xpn_path = args.xpn
    if not Path(xpn_path).exists():
        print(f"Error: file not found: {xpn_path}", file=sys.stderr)
        sys.exit(1)
    if not zipfile.is_zipfile(xpn_path):
        print(f"Error: not a valid ZIP/XPN file: {xpn_path}", file=sys.stderr)
        sys.exit(1)

    if args.format == "text":
        print(f"Fingerprinting samples in: {xpn_path}")

    records = fingerprint_xpn(xpn_path)
    pairs = find_near_duplicates(records, args.threshold)

    if args.format == "json":
        print_json(records, pairs)
    else:
        print_text(records, pairs, args.show_all, args.threshold)


if __name__ == "__main__":
    main()
