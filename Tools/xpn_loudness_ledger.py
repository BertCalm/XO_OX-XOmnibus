#!/usr/bin/env python3
"""Cross-Pack Loudness Ledger — records RMS dB proxy, true peak, crest factor per sample.

Phase 1: Data recording only. Each render populates the ledger.
Phase 2 (future): Use accumulated data to compute crest-compensated
normalization targets for perceptual loudness matching across all packs.

Note: The "rms_db_proxy" field is 20*log10(RMS), NOT true LUFS. Real LUFS
requires K-weighting + gating (ITU-R BS.1770). Install pyloudnorm for true LUFS.

The legend: "Every XO_OX pack sits at exactly the same perceived volume."
"""

import json
import math
import os
import struct
import sys
import tempfile
from pathlib import Path
from datetime import datetime

LEDGER_PATH = Path(__file__).parent / "loudness_ledger.json"


def load_ledger():
    """Load the persistent loudness ledger."""
    if LEDGER_PATH.exists():
        try:
            return json.loads(LEDGER_PATH.read_text())
        except json.JSONDecodeError:
            return {"packs": {}, "meta": {"created": datetime.now().isoformat()}}
    return {"packs": {}, "meta": {"created": datetime.now().isoformat()}}


def save_ledger(ledger):
    """Save the ledger back to disk."""
    ledger["meta"]["last_updated"] = datetime.now().isoformat()
    ledger["meta"]["total_samples"] = sum(
        len(samples) for samples in ledger["packs"].values()
    )
    temp_fd, temp_path = tempfile.mkstemp(suffix=".json", dir=str(LEDGER_PATH.parent))
    os.close(temp_fd)
    try:
        Path(temp_path).write_text(json.dumps(ledger, indent=2), encoding="utf-8")
        os.replace(temp_path, str(LEDGER_PATH))
    except Exception:
        try:
            os.unlink(temp_path)
        except OSError as exc:
            print(f"[WARN] Removing temp loudness ledger file {temp_path}: {exc}", file=sys.stderr)
        raise


def record_sample(pack_name, sample_name, loudness_db, true_peak, crest_factor,
                  category, engine=None):
    """Record a sample's loudness data after normalization.

    The 'loudness_db' parameter accepts either a true LUFS value (if pyloudnorm
    was used upstream) or the RMS dB proxy (20*log10(RMS)). It is stored under
    the key 'rms_db_proxy' to reflect that this value may not be true LUFS.
    """
    ledger = load_ledger()
    if pack_name not in ledger["packs"]:
        ledger["packs"][pack_name] = {}
    ledger["packs"][pack_name][sample_name] = {
        "rms_db_proxy": round(loudness_db, 2),
        "true_peak": round(true_peak, 2),
        "crest": round(crest_factor, 2),
        "category": category,
        "engine": engine,
        "recorded": datetime.now().isoformat(),
    }
    save_ledger(ledger)


def get_fleet_stats():
    """Get aggregate loudness statistics across all packs."""
    ledger = load_ledger()
    all_samples = []
    for pack_samples in ledger["packs"].values():
        all_samples.extend(pack_samples.values())
    if not all_samples:
        return None
    return {
        "total_samples": len(all_samples),
        "total_packs": len(ledger["packs"]),
        "avg_rms_db": 10 * math.log10(sum(10**(s["rms_db_proxy"]/10) for s in all_samples) / len(all_samples)) if all_samples else float("-inf"),
        "avg_crest": sum(s["crest"] for s in all_samples) / len(all_samples),
        "max_peak": max(s["true_peak"] for s in all_samples),
        "by_category": _stats_by_field(all_samples, "category"),
        "by_engine": _stats_by_field(all_samples, "engine"),
    }


def _stats_by_field(samples, field):
    """Group stats by a field (category or engine)."""
    groups = {}
    for s in samples:
        key = s.get(field, "unknown")
        if key not in groups:
            groups[key] = []
        groups[key].append(s)
    return {
        k: {
            "count": len(v),
            "avg_rms_db": round(10 * math.log10(sum(10**(s["rms_db_proxy"]/10) for s in v) / len(v)), 2) if v else float("-inf"),
            "avg_crest": round(sum(s["crest"] for s in v) / len(v), 2),
        }
        for k, v in groups.items()
    }


def compute_compensated_target(base_rms_db, crest_factor):
    """Phase 2: Compute crest-compensated RMS dB target.

    High crest (transient) = feels quieter -> boost target
    Low crest (sustained) = feels louder -> cut target
    """
    stats = get_fleet_stats()
    if stats is None or stats["total_samples"] < 10:
        return base_rms_db  # Not enough data yet, use default

    avg_crest = stats["avg_crest"]
    delta = (crest_factor - avg_crest) * 0.5  # 0.5 dB per dB of crest deviation
    return base_rms_db + max(-2.0, min(2.0, delta))


# ---------------------------------------------------------------------------
# Pure-stdlib WAV loudness measurement (no numpy/scipy dependency)
# ---------------------------------------------------------------------------

def _decode_wav_samples(data: bytes, bps: int) -> list:
    """Decode raw PCM bytes to float samples in [-1, 1]."""
    bytes_per_sample = bps // 8
    n_samples = len(data) // bytes_per_sample
    if n_samples == 0:
        return []

    if bps == 16:
        fmt = f"<{n_samples}h"
        raw = struct.unpack(fmt, data[:n_samples * 2])
        scale = 1.0 / 32768.0
        return [s * scale for s in raw]
    elif bps == 24:
        samples = []
        for i in range(0, n_samples * 3, 3):
            b = data[i:i + 3]
            if len(b) < 3:
                break
            val = b[0] | (b[1] << 8) | (b[2] << 16)
            if val >= 0x800000:
                val -= 0x1000000
            samples.append(val / 8388608.0)
        return samples
    elif bps == 32:
        fmt = f"<{n_samples}i"
        raw = struct.unpack(fmt, data[:n_samples * 4])
        scale = 1.0 / 2147483648.0
        return [s * scale for s in raw]
    elif bps == 8:
        return [(b - 128) / 128.0 for b in data[:n_samples]]
    return []


def measure_wav(wav_path: Path) -> dict:
    """Measure RMS dB proxy, true peak, and crest factor for a WAV file.

    Returns dict with keys: rms_db_proxy, true_peak, crest_factor.
    Returns None on read error or silence.

    Notes:
    - rms_db_proxy: 20*log10(RMS), NOT true LUFS. True ITU-R BS.1770 LUFS
      requires a K-weighting filter and gating — those require numpy/pyloudnorm.
      This is a fast, dependency-free proxy suitable for cross-pack consistency.
    - True peak: per-sample max absolute value in dBFS (no inter-sample
      oversampling). Accurate enough for ledger recording purposes.
    - Crest factor: peak_dBFS - rms_dBFS (higher = more transient).
    """
    try:
        import struct as _struct  # already imported at module level, belt+suspenders
        with open(wav_path, "rb") as f:
            riff = f.read(4)
            if riff != b"RIFF":
                return None
            f.read(4)  # file size
            wave = f.read(4)
            if wave != b"WAVE":
                return None

            num_channels = sample_rate = bits_per_sample = 0
            data_bytes = b""
            fmt_found = False

            while True:
                chunk_id = f.read(4)
                if len(chunk_id) < 4:
                    break
                chunk_size_bytes = f.read(4)
                if len(chunk_size_bytes) < 4:
                    break
                chunk_size = struct.unpack("<I", chunk_size_bytes)[0]
                if chunk_id == b"fmt ":
                    fmt_data = f.read(chunk_size)
                    audio_fmt = struct.unpack("<H", fmt_data[0:2])[0]
                    if audio_fmt != 1:
                        return None  # not PCM
                    num_channels = struct.unpack("<H", fmt_data[2:4])[0]
                    sample_rate = struct.unpack("<I", fmt_data[4:8])[0]
                    bits_per_sample = struct.unpack("<H", fmt_data[14:16])[0]
                    fmt_found = True
                elif chunk_id == b"data":
                    data_bytes = f.read(chunk_size)
                else:
                    f.read(chunk_size)
                if chunk_size % 2 != 0:
                    f.read(1)

        if not fmt_found or not data_bytes:
            return None

    except (OSError, struct.error):
        return None

    samples = _decode_wav_samples(data_bytes, bits_per_sample)
    if not samples:
        return None

    # RMS dB proxy (20*log10(RMS) — NOT true LUFS)
    sum_sq = sum(s * s for s in samples)
    rms = math.sqrt(sum_sq / len(samples))
    if rms < 1e-10:
        return None  # silence — skip

    rms_db = 20.0 * math.log10(rms)

    # True peak (per-sample, no inter-sample oversampling)
    peak = max(abs(s) for s in samples)
    if peak < 1e-10:
        return None
    peak_db = 20.0 * math.log10(peak)

    # Crest factor
    crest_factor = peak_db - rms_db

    return {
        "rms_db_proxy": round(rms_db, 2),  # 20*log10(RMS) — NOT true LUFS
        "true_peak": round(peak_db, 2),    # dBFS
        "crest_factor": round(crest_factor, 2),  # dB
    }


if __name__ == "__main__":
    import sys
    if len(sys.argv) > 1 and sys.argv[1] == "stats":
        stats = get_fleet_stats()
        if stats:
            print("Fleet Loudness Stats:")
            print(f"  Total samples: {stats['total_samples']}")
            print(f"  Total packs: {stats['total_packs']}")
            print(f"  Avg RMS dB proxy: {stats['avg_rms_db']:.1f}")
            print(f"  Avg Crest: {stats['avg_crest']:.1f} dB")
            print(f"  Max True Peak: {stats['max_peak']:.1f} dBTP")
            print("\n  By Category:")
            for cat, data in stats["by_category"].items():
                print(f"    {cat}: {data['count']} samples, "
                      f"avg {data['avg_rms_db']:.1f} dB RMS, "
                      f"crest {data['avg_crest']:.1f} dB")
        else:
            print("No data in ledger yet. Run some exports first.")
    else:
        print("Usage: python3 xpn_loudness_ledger.py stats")
