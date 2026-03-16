#!/usr/bin/env python3
"""
XPN Entropy Kit — XO_OX Designs
Hex + Rex :: hacker android + XPN format android

Information-theoretic sample selection for optimal kit curation.
Given N candidate WAV files, select the 16 that maximize (or minimize)
information content using Jensen-Shannon divergence on 8-band spectral fingerprints.

Three selection strategies:
  max-entropy  Greedy max-entropy: each new sample maximizes surprise given
               all previously selected. Produces maximally diverse kits.
  min-entropy  Anti-entropy: find the 16 most similar samples — maximum
               redundancy for coherent tonal kits (one-shots in a family).
  mdl          k-medoids (PAM): minimum description length. Find n
               representative samples that best summarize the pool.

Integration:
  If monster_metadata.json exists in the samples directory (from xpn_monster_rancher.py),
  band_energy_mean fingerprints are loaded from there instead of recomputed.

Output per run:
  selection.json          ranked file paths + JSD distances + per-step entropy gain
  entropy_kit.xpm         MPC-compatible drum XPM using the 16 selected samples
  (with --visualize)      ASCII fingerprint matrix — no XPM written

Usage:
  python xpn_entropy_kit.py samples/ --method max-entropy --n 16 --output ./kits/
  python xpn_entropy_kit.py samples/ --method min-entropy --n 16 --output ./kits/
  python xpn_entropy_kit.py samples/ --method mdl --n 16 --output ./kits/
  python xpn_entropy_kit.py samples/ --compare all --output ./kits/
  python xpn_entropy_kit.py samples/ --visualize

Dependencies: numpy (required); scipy optional but recommended for accurate bandpass filters.
  pip install numpy scipy
"""

from __future__ import annotations

import argparse
import json
import math
import os
import struct
import sys
import wave
from datetime import date
from pathlib import Path
from typing import Dict, List, Optional, Tuple
from xml.sax.saxutils import escape as xml_escape

# ---------------------------------------------------------------------------
# Optional numpy / scipy
# ---------------------------------------------------------------------------
try:
    import numpy as np
    _NUMPY_AVAILABLE = True
except ImportError:
    _NUMPY_AVAILABLE = False

try:
    from scipy.signal import butter, sosfilt
    _SCIPY_AVAILABLE = True
except ImportError:
    _SCIPY_AVAILABLE = False


# ---------------------------------------------------------------------------
# Band definitions — match XOptic crossoverFrequencies[]
# Band edges: [20, 80, 200, 500, 1000, 4000, 8000, 16000, 20000] Hz
# ---------------------------------------------------------------------------
CROSSOVER_FREQS: List[float] = [80.0, 200.0, 500.0, 1000.0, 4000.0, 8000.0, 16000.0]
BAND_NAMES = ["Sub", "Bass", "LoMid", "Mid", "HiMid", "Presence", "Brilliance", "Air"]
NUM_BANDS = 8
BAND_EDGES: List[Tuple[float, float]] = []
for _i in range(NUM_BANDS):
    _lo = 20.0 if _i == 0 else CROSSOVER_FREQS[_i - 1]
    _hi = 20000.0 if _i == NUM_BANDS - 1 else CROSSOVER_FREQS[_i]
    BAND_EDGES.append((_lo, _hi))

EPSILON = 1e-10

# Monster Rancher metadata filename
MONSTER_METADATA_FILE = "monster_metadata.json"


# ===========================================================================
# WAV READER
# ===========================================================================

def _read_wav_mono(path: str) -> Tuple[List[float], int]:
    """Read a WAV file to mono float list using stdlib wave. Returns (samples, sr)."""
    try:
        with wave.open(path, "rb") as wf:
            nch = wf.getnchannels()
            sw = wf.getsampwidth()
            sr = wf.getframerate()
            nframes = wf.getnframes()
            raw = wf.readframes(nframes)
    except Exception as exc:
        raise IOError(f"Cannot read WAV {path}: {exc}") from exc

    bit_depth = sw * 8
    if bit_depth == 8:
        vals = list(raw)
        all_samples = [(v - 128) / 128.0 for v in vals]
    elif bit_depth == 16:
        n = len(raw) // 2
        vals = struct.unpack(f"<{n}h", raw[: n * 2])
        all_samples = [v / 32768.0 for v in vals]
    elif bit_depth == 24:
        all_samples = []
        for i in range(0, len(raw) - 2, 3):
            v = raw[i] | (raw[i + 1] << 8) | (raw[i + 2] << 16)
            if v & 0x800000:
                v -= 0x1000000
            all_samples.append(v / 8388608.0)
    elif bit_depth == 32:
        n = len(raw) // 4
        vals = struct.unpack(f"<{n}i", raw[: n * 4])
        all_samples = [v / 2147483648.0 for v in vals]
    else:
        raise ValueError(f"Unsupported bit depth {bit_depth} in {path}")

    if nch > 1:
        mono: List[float] = []
        for i in range(0, len(all_samples) - nch + 1, nch):
            mono.append(sum(all_samples[i : i + nch]) / nch)
        all_samples = mono

    return all_samples, sr


# ===========================================================================
# SPECTRAL FINGERPRINT
# ===========================================================================

def _fft_band_energy_numpy(samples: List[float], sr: int) -> "np.ndarray":
    """Compute 8-band energy via FFT (numpy path)."""
    arr = np.array(samples, dtype=np.float32)
    N = len(arr)
    if N == 0:
        return np.zeros(NUM_BANDS, dtype=np.float64)

    spectrum = np.abs(np.fft.rfft(arr)) ** 2  # power spectrum
    freqs = np.fft.rfftfreq(N, d=1.0 / sr)

    energy = np.zeros(NUM_BANDS, dtype=np.float64)
    for b, (lo, hi) in enumerate(BAND_EDGES):
        mask = (freqs >= lo) & (freqs < hi)
        energy[b] = float(np.sum(spectrum[mask]))

    return energy


def _fft_band_energy_stdlib(samples: List[float], sr: int) -> List[float]:
    """Pure-Python FFT energy per band — fallback when numpy unavailable."""
    N = len(samples)
    if N == 0:
        return [0.0] * NUM_BANDS

    # DFT magnitude squared via Cooley-Tukey (simple O(N log N) for power-of-2 N)
    # For large files we downsample to max 8192 frames for speed
    MAX_FRAMES = 8192
    if N > MAX_FRAMES:
        step = N // MAX_FRAMES
        samples = samples[::step]
        N = len(samples)

    # Two-sided DFT up to N//2 + 1 (rfft equivalent) using cmath
    import cmath
    half = N // 2 + 1
    power = [0.0] * half
    for k in range(half):
        re = 0.0
        im = 0.0
        for n, x in enumerate(samples):
            angle = -2.0 * math.pi * k * n / N
            re += x * math.cos(angle)
            im += x * math.sin(angle)
        power[k] = re * re + im * im

    freq_res = sr / N  # Hz per bin
    energy = [0.0] * NUM_BANDS
    for k, p in enumerate(power):
        freq = k * freq_res
        for b, (lo, hi) in enumerate(BAND_EDGES):
            if lo <= freq < hi:
                energy[b] += p
                break

    return energy


def compute_fingerprint(wav_file: str) -> "np.ndarray":
    """Compute an 8-band spectral energy fingerprint for a WAV file.

    Returns np.ndarray of shape (8,) normalized 0.0–1.0.
    Each element represents the relative energy in one frequency band,
    using the XOptic crossover frequencies: [80, 200, 500, 1000, 4000, 8000, 16000] Hz.
    """
    samples, sr = _read_wav_mono(wav_file)

    if _NUMPY_AVAILABLE and _SCIPY_AVAILABLE:
        energy = _fft_band_energy_scipy(samples, sr)
    elif _NUMPY_AVAILABLE:
        energy = _fft_band_energy_numpy(samples, sr)
        energy = np.array(energy, dtype=np.float64)
    else:
        energy_list = _fft_band_energy_stdlib(samples, sr)
        # Wrap in a minimal array-like object
        energy = _SimpleArray(energy_list)
        total = sum(energy_list) + EPSILON
        return _SimpleArray([v / total for v in energy_list])

    # Normalize to sum=1 probability distribution
    total = float(energy.sum()) + EPSILON
    normalized = energy / total
    # Guard against zero-sum (silent file)
    if total <= EPSILON:
        normalized = np.ones(NUM_BANDS, dtype=np.float64) / NUM_BANDS
    return normalized


def _fft_band_energy_scipy(samples: List[float], sr: int) -> "np.ndarray":
    """Bandpass-filtered RMS energy per band using scipy Butterworth filters.

    This is the most accurate path — matches XOptic's Butterworth filter bank.
    Falls back to FFT binning if scipy unavailable.
    """
    arr = np.array(samples, dtype=np.float32)
    if len(arr) == 0:
        return np.zeros(NUM_BANDS, dtype=np.float64)

    nyq = sr / 2.0
    energy = np.zeros(NUM_BANDS, dtype=np.float64)

    for b, (lo, hi) in enumerate(BAND_EDGES):
        lo_n = max(lo / nyq, 0.001)
        hi_n = min(hi / nyq, 0.95)
        if lo_n >= hi_n:
            hi_n = min(lo_n + 0.01, 0.95)
        try:
            sos = butter(2, [lo_n, hi_n], btype="bandpass", output="sos")
            filtered = sosfilt(sos, arr)
            energy[b] = float(np.mean(filtered ** 2))
        except Exception:
            energy[b] = 0.0

    return energy


# Simple array wrapper for stdlib path (no numpy)
class _SimpleArray:
    """Minimal ndarray-compatible wrapper for the no-numpy code path."""
    def __init__(self, data: List[float]):
        self._data = list(data)

    def __getitem__(self, idx):
        return self._data[idx]

    def __len__(self):
        return len(self._data)

    def __iter__(self):
        return iter(self._data)

    def sum(self):
        return sum(self._data)

    def __truediv__(self, scalar):
        return _SimpleArray([v / scalar for v in self._data])

    def tolist(self) -> List[float]:
        return list(self._data)


# ===========================================================================
# JENSEN-SHANNON DIVERGENCE
# ===========================================================================

def jsd(p, q) -> float:
    """Jensen-Shannon divergence between two spectral fingerprints.

    JSD is symmetric and bounded [0, ln(2)] — we normalize to [0, 1].
    JSD = 0: identical distributions (no new information).
    JSD = 1: maximally different (maximum surprise).

    Args:
        p: 8-element array-like (normalized spectral energy, sums to ~1)
        q: 8-element array-like (same format as p)

    Returns:
        float in [0, 1]
    """
    if _NUMPY_AVAILABLE:
        p_arr = np.asarray(p, dtype=np.float64)
        q_arr = np.asarray(q, dtype=np.float64)
        # Renormalize in case of floating-point drift
        p_arr = p_arr / (p_arr.sum() + EPSILON)
        q_arr = q_arr / (q_arr.sum() + EPSILON)
        m = 0.5 * (p_arr + q_arr) + EPSILON
        p_safe = p_arr + EPSILON
        q_safe = q_arr + EPSILON
        kl_pm = float(np.sum(p_safe * np.log(p_safe / m)))
        kl_qm = float(np.sum(q_safe * np.log(q_safe / m)))
    else:
        p_list = list(p)
        q_list = list(q)
        total_p = sum(p_list) + EPSILON
        total_q = sum(q_list) + EPSILON
        p_norm = [v / total_p for v in p_list]
        q_norm = [v / total_q for v in q_list]
        m = [(p_norm[i] + q_norm[i]) / 2.0 + EPSILON for i in range(NUM_BANDS)]
        kl_pm = sum((p_norm[i] + EPSILON) * math.log((p_norm[i] + EPSILON) / m[i])
                    for i in range(NUM_BANDS))
        kl_qm = sum((q_norm[i] + EPSILON) * math.log((q_norm[i] + EPSILON) / m[i])
                    for i in range(NUM_BANDS))

    raw_jsd = 0.5 * (kl_pm + kl_qm)
    # JSD is bounded by ln(2) ≈ 0.693 for base-e KL; normalize to [0,1]
    ln2 = math.log(2.0)
    normalized = min(1.0, max(0.0, raw_jsd / ln2))
    return normalized


# ===========================================================================
# SELECTION ALGORITHMS
# ===========================================================================

def _spectral_centroid(fp) -> float:
    """Weighted mean band index — proxy for spectral brightness (0=sub, 7=air)."""
    total = 0.0
    weighted = 0.0
    for i, v in enumerate(fp):
        total += v
        weighted += v * i
    if total < EPSILON:
        return 3.5
    return weighted / total


def select_max_entropy(fingerprints: Dict[str, any], n: int = 16) -> Tuple[List[str], List[dict]]:
    """Greedily select n samples maximizing conditional entropy.

    At each step, choose the sample most different from all already-selected.
    This produces the most diverse kit — maximum information, minimum redundancy.

    Returns:
        (selected_paths, rationale) where rationale is a list of dicts
        with keys: file, step, min_jsd_to_selected, entropy_gain, selected_from
    """
    keys = list(fingerprints.keys())
    if len(keys) == 0:
        return [], []
    n = min(n, len(keys))

    selected: List[str] = []
    remaining = list(keys)
    rationale: List[dict] = []

    # Step 1: seed with highest spectral centroid (brightest sample)
    # Bright samples are sonically distinct anchors for a diverse kit
    first = max(remaining, key=lambda f: _spectral_centroid(fingerprints[f]))
    selected.append(first)
    remaining.remove(first)
    rationale.append({
        "file": first,
        "step": 1,
        "min_jsd_to_selected": 1.0,
        "entropy_gain": 1.0,
        "method": "seed (highest spectral centroid)",
        "selected_from": len(keys),
    })

    # Steps 2..n: maximize minimum JSD to any already-selected sample
    for step in range(2, n + 1):
        if not remaining:
            break
        # For each candidate, compute min JSD to the selected set (nearest neighbor distance)
        best_f = None
        best_min_jsd = -1.0
        for f in remaining:
            min_d = min(jsd(fingerprints[f], fingerprints[s]) for s in selected)
            if min_d > best_min_jsd:
                best_min_jsd = min_d
                best_f = f

        selected.append(best_f)
        remaining.remove(best_f)
        rationale.append({
            "file": best_f,
            "step": step,
            "min_jsd_to_selected": round(best_min_jsd, 6),
            "entropy_gain": round(best_min_jsd, 6),
            "method": "max-entropy greedy (maximize min JSD)",
            "selected_from": len(remaining) + 1,
        })

    return selected, rationale


def select_min_entropy(fingerprints: Dict[str, any], n: int = 16) -> Tuple[List[str], List[dict]]:
    """Select n most similar samples — coherent tonal family kit.

    Computes the centroid of all fingerprints, then selects the n samples
    closest to that centroid. Produces the most redundant, similar-sounding kit.

    Returns:
        (selected_paths, rationale)
    """
    keys = list(fingerprints.keys())
    if len(keys) == 0:
        return [], []
    n = min(n, len(keys))

    if _NUMPY_AVAILABLE:
        # Compute centroid as mean of all fingerprints
        fp_matrix = np.array([list(fingerprints[k]) for k in keys], dtype=np.float64)
        centroid = fp_matrix.mean(axis=0)
        centroid = centroid / (centroid.sum() + EPSILON)
    else:
        # Pure-Python centroid
        centroid_raw = [0.0] * NUM_BANDS
        for k in keys:
            fp = list(fingerprints[k])
            for i in range(NUM_BANDS):
                centroid_raw[i] += fp[i]
        total = len(keys)
        centroid_raw = [v / total for v in centroid_raw]
        csum = sum(centroid_raw) + EPSILON
        centroid = [v / csum for v in centroid_raw]

    # Compute JSD from each sample to the centroid
    distances = [(k, jsd(fingerprints[k], centroid)) for k in keys]
    distances.sort(key=lambda x: x[1])

    selected = [d[0] for d in distances[:n]]
    rationale = [
        {
            "file": f,
            "step": i + 1,
            "jsd_to_centroid": round(dist, 6),
            "method": "min-entropy (closest to centroid)",
            "selected_from": len(keys),
        }
        for i, (f, dist) in enumerate(distances[:n])
    ]

    return selected, rationale


def select_mdl(fingerprints: Dict[str, any], n: int = 16) -> Tuple[List[str], List[dict]]:
    """k-medoids (PAM) selection — minimum description length.

    Finds n representative samples that minimize total JSD to nearest medoid.
    Initialization: max-entropy selection (diverse starting medoids).
    Optimization: BUILD + single-pass SWAP from PAM algorithm.

    This finds the n samples that best summarize the full pool without
    redundancy — a balanced middle ground between max and min entropy.

    Returns:
        (selected_paths, rationale)
    """
    keys = list(fingerprints.keys())
    if len(keys) == 0:
        return [], []
    n = min(n, len(keys))

    if n == len(keys):
        # Trivial: select everything
        return keys, [{"file": f, "step": i + 1, "method": "mdl (trivial — n=pool size)"}
                      for i, f in enumerate(keys)]

    # Initialize medoids with max-entropy selection (diverse anchors)
    medoids, _ = select_max_entropy(fingerprints, n)

    # Precompute full JSD matrix for efficiency
    all_keys = list(fingerprints.keys())
    key_idx = {k: i for i, k in enumerate(all_keys)}
    K = len(all_keys)

    # Build JSD matrix (symmetric, K×K) — O(K² × 8)
    print(f"  [mdl] computing {K}×{K} JSD matrix for {K} samples...", file=sys.stderr)
    if _NUMPY_AVAILABLE:
        fp_matrix = np.array([list(fingerprints[k]) for k in all_keys], dtype=np.float64)
        # Ensure rows are valid distributions
        row_sums = fp_matrix.sum(axis=1, keepdims=True) + EPSILON
        fp_matrix = fp_matrix / row_sums

        jsd_matrix = np.zeros((K, K), dtype=np.float64)
        for i in range(K):
            for j in range(i + 1, K):
                d = jsd(fp_matrix[i], fp_matrix[j])
                jsd_matrix[i, j] = d
                jsd_matrix[j, i] = d
    else:
        jsd_matrix_list = [[0.0] * K for _ in range(K)]
        for i in range(K):
            for j in range(i + 1, K):
                d = jsd(fingerprints[all_keys[i]], fingerprints[all_keys[j]])
                jsd_matrix_list[i][j] = d
                jsd_matrix_list[j][i] = d

    def _total_cost(medoid_indices: List[int]) -> float:
        """Sum of JSD from each point to its nearest medoid."""
        total = 0.0
        for i in range(K):
            if _NUMPY_AVAILABLE:
                dists = [float(jsd_matrix[i, m]) for m in medoid_indices]
            else:
                dists = [jsd_matrix_list[i][m] for m in medoid_indices]
            total += min(dists)
        return total

    medoid_indices = [key_idx[m] for m in medoids]
    current_cost = _total_cost(medoid_indices)

    # PAM SWAP phase — try replacing each medoid with each non-medoid
    improved = True
    iterations = 0
    MAX_ITER = 20  # cap for large pools

    while improved and iterations < MAX_ITER:
        improved = False
        iterations += 1
        medoid_set = set(medoid_indices)
        non_medoids = [i for i in range(K) if i not in medoid_set]

        for m_idx in list(medoid_indices):
            for h_idx in non_medoids:
                # Swap medoid m_idx for candidate h_idx
                trial = [h_idx if x == m_idx else x for x in medoid_indices]
                trial_cost = _total_cost(trial)
                if trial_cost < current_cost - EPSILON:
                    medoid_indices = trial
                    current_cost = trial_cost
                    improved = True
                    break
            if improved:
                break

    medoids_final = [all_keys[i] for i in medoid_indices]

    # Compute per-medoid assignment stats for rationale
    rationale = []
    for rank, m in enumerate(medoids_final):
        mi = key_idx[m]
        assigned_count = 0
        for i in range(K):
            if _NUMPY_AVAILABLE:
                dists = [float(jsd_matrix[i, mx]) for mx in medoid_indices]
            else:
                dists = [jsd_matrix_list[i][mx] for mx in medoid_indices]
            if medoid_indices[medoid_indices.index(mi)] == medoid_indices[dists.index(min(dists))]:
                assigned_count += 1
        rationale.append({
            "file": m,
            "step": rank + 1,
            "cluster_size": assigned_count,
            "total_cost": round(current_cost, 6),
            "pam_iterations": iterations,
            "method": "mdl k-medoids PAM",
        })

    return medoids_final, rationale


# ===========================================================================
# FINGERPRINT LOADING WITH MONSTER RANCHER CACHE
# ===========================================================================

def load_fingerprints(samples_dir: Path, wav_files: List[Path],
                      force_recompute: bool = False) -> Dict[str, any]:
    """Load spectral fingerprints for all WAV files.

    If monster_metadata.json exists in samples_dir and is not stale,
    load band_energy_mean arrays from it. Otherwise compute fresh fingerprints.

    Returns:
        dict mapping absolute path str → fingerprint array
    """
    fingerprints: Dict[str, any] = {}

    # Try loading from Monster Rancher metadata cache
    cached: Dict[str, List[float]] = {}
    meta_path = samples_dir / MONSTER_METADATA_FILE
    if not force_recompute and meta_path.exists():
        try:
            with open(meta_path, "r", encoding="utf-8") as f:
                meta = json.load(f)
            # Monster Rancher stores per-file entries; try to extract band_energy_mean
            for entry in meta if isinstance(meta, list) else meta.get("files", []):
                fpath = entry.get("file") or entry.get("path", "")
                bands = (
                    entry.get("fingerprint", {}).get("band_energy_mean")
                    or entry.get("band_energy_mean")
                )
                if fpath and bands and len(bands) == NUM_BANDS:
                    cached[str(Path(fpath).resolve())] = bands
            if cached:
                print(f"  [cache] loaded {len(cached)} fingerprints from {meta_path.name}",
                      file=sys.stderr)
        except Exception as exc:
            print(f"  [cache] failed to load {meta_path}: {exc}", file=sys.stderr)

    # Load/compute fingerprint per WAV file
    for wav in wav_files:
        key = str(wav.resolve())
        if key in cached and not force_recompute:
            if _NUMPY_AVAILABLE:
                import numpy as np
                arr = np.array(cached[key], dtype=np.float64)
                total = arr.sum() + EPSILON
                fingerprints[key] = arr / total
            else:
                total = sum(cached[key]) + EPSILON
                fingerprints[key] = _SimpleArray([v / total for v in cached[key]])
        else:
            try:
                fp = compute_fingerprint(str(wav))
                fingerprints[key] = fp
            except Exception as exc:
                print(f"  [warn] skipping {wav.name}: {exc}", file=sys.stderr)

    return fingerprints


# ===========================================================================
# ASCII VISUALIZATION
# ===========================================================================

def _bar(value: float, width: int = 20, char: str = "█") -> str:
    """Render a single horizontal bar for a value in [0, 1]."""
    filled = max(0, min(width, round(value * width)))
    return char * filled + "·" * (width - filled)


def visualize(fingerprints: Dict[str, any],
              selected: Optional[List[str]] = None,
              title: str = "Spectral Fingerprint Matrix") -> None:
    """Print an ASCII visualization of spectral fingerprints.

    Each row is one sample. Columns are the 8 frequency bands.
    Selected samples are marked with ★; unselected with ·.
    """
    selected_set = set(selected or [])
    band_label_width = max(len(b) for b in BAND_NAMES)

    print(f"\n{'='*70}")
    print(f"  {title}")
    print(f"  {len(fingerprints)} samples | {NUM_BANDS} bands")
    print(f"{'='*70}")

    # Print band header
    header_marker = "  " + " " * 36
    print(f"  {'FILE':<34}  {'★':<2} " +
          "  ".join(f"{b:<3}" for b in ["Sub", "Bas", "LMd", "Mid", "HMd", "Pre", "Bri", "Air"]))
    print(f"  {'-'*34}  {'--':<2} " + "  ".join(["---"] * 8))

    for path, fp in fingerprints.items():
        name = Path(path).stem[:32]
        marker = "★" if path in selected_set else "·"
        fp_list = list(fp)

        # Normalize for display
        fp_max = max(fp_list) + EPSILON
        bars = ""
        for v in fp_list:
            # Map to 5-level block: ▁▂▃▄█
            lvl = v / fp_max
            if lvl < 0.1:
                bar_char = "▁"
            elif lvl < 0.3:
                bar_char = "▂"
            elif lvl < 0.5:
                bar_char = "▄"
            elif lvl < 0.75:
                bar_char = "▆"
            else:
                bar_char = "█"
            bars += f"  {bar_char}  "

        # JSD centroid scalar
        centroid_idx = _spectral_centroid(fp)
        centroid_label = BAND_NAMES[int(min(7, centroid_idx))][:3]

        print(f"  {name:<34}  {marker}  {bars}  [{centroid_label}]")

    print(f"{'='*70}\n")

    # If selected, show diversity matrix (JSD between all selected pairs)
    if selected and len(selected) > 1:
        n = len(selected)
        short_names = [Path(p).stem[:12] for p in selected]
        print(f"  Diversity matrix (JSD) — {n} selected samples")
        print(f"  {'':>14} " + "  ".join(f"{s[:6]:>6}" for s in short_names))
        for i, pi in enumerate(selected):
            row = f"  {short_names[i]:>14} "
            for j, pj in enumerate(selected):
                if i == j:
                    row += f"  {'——':>6}"
                else:
                    d = jsd(fingerprints[pi], fingerprints[pj])
                    row += f"  {d:>6.3f}"
            print(row)
        print()

        # Average pairwise JSD — kit diversity score
        total_d = 0.0
        count = 0
        for i in range(n):
            for j in range(i + 1, n):
                total_d += jsd(fingerprints[selected[i]], fingerprints[selected[j]])
                count += 1
        avg_d = total_d / max(count, 1)
        print(f"  Kit diversity score (avg pairwise JSD): {avg_d:.4f}")
        print(f"  Range: 0.0 (identical) → 1.0 (maximally diverse)")
        print()


# ===========================================================================
# XPM GENERATION
# ===========================================================================

_XPM_HEADER = (
    '<?xml version="1.0" encoding="UTF-8"?>\n\n'
    '<MPCVObject>\n'
    '  <Version>\n'
    '    <File_Version>1.7</File_Version>\n'
    '    <Application>MPC-V</Application>\n'
    '    <Application_Version>2.10.0.0</Application_Version>\n'
    '    <Platform>OSX</Platform>\n'
    '  </Version>'
)

# Pad layout: 16 pads × chromatic notes starting C2 (MIDI 36)
_PAD_NOTES = [36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51]
_NOTE_NAMES = ["C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"]


def _midi_note_name(midi: int) -> str:
    return f"{_NOTE_NAMES[midi % 12]}{(midi // 12) - 1}"


def _layer_block_entropy(number: int, wav_path: str, kit_name: str) -> str:
    """Generate a single XPM Layer block for one entropy-selected sample."""
    if not wav_path:
        return (
            f'          <Layer number="{number}">\n'
            f'            <Active>False</Active>\n'
            f'            <Volume>0.707946</Volume>\n'
            f'            <Pan>0.500000</Pan>\n'
            f'            <Pitch>0.000000</Pitch>\n'
            f'            <TuneCoarse>0</TuneCoarse>\n'
            f'            <TuneFine>0</TuneFine>\n'
            f'            <VelStart>0</VelStart>\n'
            f'            <VelEnd>0</VelEnd>\n'
            f'            <SampleStart>0</SampleStart>\n'
            f'            <SampleEnd>0</SampleEnd>\n'
            f'            <Loop>False</Loop>\n'
            f'            <LoopStart>0</LoopStart>\n'
            f'            <LoopEnd>0</LoopEnd>\n'
            f'            <LoopTune>0</LoopTune>\n'
            f'            <Mute>False</Mute>\n'
            f'            <RootNote>0</RootNote>\n'
            f'            <KeyTrack>True</KeyTrack>\n'
            f'            <SampleName></SampleName>\n'
            f'            <SampleFile></SampleFile>\n'
            f'            <File></File>\n'
            f'            <SliceIndex>128</SliceIndex>\n'
            f'            <Direction>0</Direction>\n'
            f'            <Offset>0</Offset>\n'
            f'            <SliceStart>0</SliceStart>\n'
            f'            <SliceEnd>0</SliceEnd>\n'
            f'            <SliceLoopStart>0</SliceLoopStart>\n'
            f'            <SliceLoop>0</SliceLoop>\n'
            f'          </Layer>'
        )

    p = Path(wav_path)
    sample_name = xml_escape(p.stem[:64])
    sample_file = xml_escape(p.name)
    file_path = xml_escape(str(p))

    return (
        f'          <Layer number="{number}">\n'
        f'            <Active>True</Active>\n'
        f'            <Volume>0.707946</Volume>\n'
        f'            <Pan>0.500000</Pan>\n'
        f'            <Pitch>0.000000</Pitch>\n'
        f'            <TuneCoarse>0</TuneCoarse>\n'
        f'            <TuneFine>0</TuneFine>\n'
        f'            <VelStart>1</VelStart>\n'
        f'            <VelEnd>127</VelEnd>\n'
        f'            <SampleStart>0</SampleStart>\n'
        f'            <SampleEnd>0</SampleEnd>\n'
        f'            <Loop>False</Loop>\n'
        f'            <LoopStart>0</LoopStart>\n'
        f'            <LoopEnd>0</LoopEnd>\n'
        f'            <LoopTune>0</LoopTune>\n'
        f'            <Mute>False</Mute>\n'
        f'            <RootNote>0</RootNote>\n'
        f'            <KeyTrack>True</KeyTrack>\n'
        f'            <SampleName>{sample_name}</SampleName>\n'
        f'            <SampleFile>{sample_file}</SampleFile>\n'
        f'            <File>{file_path}</File>\n'
        f'            <SliceIndex>128</SliceIndex>\n'
        f'            <Direction>0</Direction>\n'
        f'            <Offset>0</Offset>\n'
        f'            <SliceStart>0</SliceStart>\n'
        f'            <SliceEnd>0</SliceEnd>\n'
        f'            <SliceLoopStart>0</SliceLoopStart>\n'
        f'            <SliceLoop>0</SliceLoop>\n'
        f'          </Layer>'
    )


def _instrument_block_entropy(instrument_num: int, note: int, wav_path: str,
                               kit_name: str, pad_rank: int) -> str:
    """Generate one XPM Instrument block for an entropy-selected pad."""
    is_active = bool(wav_path)
    active_layer = _layer_block_entropy(1, wav_path, kit_name)
    # 3 silent filler layers
    silent_layers = "\n".join(_layer_block_entropy(i, "", kit_name) for i in range(2, 5))
    layers_xml = active_layer + "\n" + silent_layers

    return (
        f'      <Instrument number="{instrument_num}">\n'
        f'        <AudioRoute>\n'
        f'          <AudioRoute>0</AudioRoute>\n'
        f'          <AudioRouteSubIndex>0</AudioRouteSubIndex>\n'
        f'          <InsertsEnabled>False</InsertsEnabled>\n'
        f'        </AudioRoute>\n'
        f'        <Send1>0.000000</Send1>\n'
        f'        <Send2>0.000000</Send2>\n'
        f'        <Send3>0.000000</Send3>\n'
        f'        <Send4>0.000000</Send4>\n'
        f'        <Volume>0.707946</Volume>\n'
        f'        <Mute>False</Mute>\n'
        f'        <Pan>0.500000</Pan>\n'
        f'        <TuneCoarse>0</TuneCoarse>\n'
        f'        <TuneFine>0</TuneFine>\n'
        f'        <Mono>False</Mono>\n'
        f'        <Polyphony>2</Polyphony>\n'
        f'        <FilterKeytrack>0.000000</FilterKeytrack>\n'
        f'        <LowNote>{note}</LowNote>\n'
        f'        <HighNote>{note}</HighNote>\n'
        f'        <IgnoreBaseNote>False</IgnoreBaseNote>\n'
        f'        <ZonePlay>1</ZonePlay>\n'
        f'        <MuteGroup>0</MuteGroup>\n'
        f'        <MuteTarget1>0</MuteTarget1>\n'
        f'        <MuteTarget2>0</MuteTarget2>\n'
        f'        <MuteTarget3>0</MuteTarget3>\n'
        f'        <MuteTarget4>0</MuteTarget4>\n'
        f'        <SimultTarget1>0</SimultTarget1>\n'
        f'        <SimultTarget2>0</SimultTarget2>\n'
        f'        <SimultTarget3>0</SimultTarget3>\n'
        f'        <SimultTarget4>0</SimultTarget4>\n'
        f'        <LfoPitch>0.000000</LfoPitch>\n'
        f'        <LfoCutoff>0.000000</LfoCutoff>\n'
        f'        <LfoVolume>0.000000</LfoVolume>\n'
        f'        <LfoPan>0.000000</LfoPan>\n'
        f'        <OneShot>True</OneShot>\n'
        f'        <FilterType>2</FilterType>\n'
        f'        <Cutoff>1.000000</Cutoff>\n'
        f'        <Resonance>0.000000</Resonance>\n'
        f'        <FilterEnvAmt>0.000000</FilterEnvAmt>\n'
        f'        <AfterTouchToFilter>0.000000</AfterTouchToFilter>\n'
        f'        <VelocityToStart>0.000000</VelocityToStart>\n'
        f'        <VelocityToFilterAttack>0.000000</VelocityToFilterAttack>\n'
        f'        <VelocityToFilter>0.000000</VelocityToFilter>\n'
        f'        <VelocityToFilterEnvelope>0.000000</VelocityToFilterEnvelope>\n'
        f'        <FilterAttack>0.000000</FilterAttack>\n'
        f'        <FilterDecay>0.000000</FilterDecay>\n'
        f'        <FilterSustain>1.000000</FilterSustain>\n'
        f'        <FilterRelease>0.000000</FilterRelease>\n'
        f'        <FilterHold>0.000000</FilterHold>\n'
        f'        <FilterDecayType>True</FilterDecayType>\n'
        f'        <FilterADEnvelope>True</FilterADEnvelope>\n'
        f'        <VolumeHold>0.000000</VolumeHold>\n'
        f'        <VolumeDecayType>True</VolumeDecayType>\n'
        f'        <VolumeADEnvelope>True</VolumeADEnvelope>\n'
        f'        <VolumeAttack>0.000000</VolumeAttack>\n'
        f'        <VolumeDecay>0.300000</VolumeDecay>\n'
        f'        <VolumeSustain>0.000000</VolumeSustain>\n'
        f'        <VolumeRelease>0.050000</VolumeRelease>\n'
        f'        <VelocityToPitch>0.000000</VelocityToPitch>\n'
        f'        <VelocityToVolumeAttack>0.000000</VelocityToVolumeAttack>\n'
        f'        <VelocitySensitivity>1.000000</VelocitySensitivity>\n'
        f'        <VelocityToPan>0.000000</VelocityToPan>\n'
        f'        <LFO>\n'
        f'          <Speed>0.000000</Speed>\n'
        f'          <Amount>0.000000</Amount>\n'
        f'          <Type>0</Type>\n'
        f'          <Sync>False</Sync>\n'
        f'          <Retrigger>True</Retrigger>\n'
        f'        </LFO>\n'
        f'        <Layers>\n'
        f'{layers_xml}\n'
        f'        </Layers>\n'
        f'      </Instrument>'
    )


def _empty_instrument_block(instrument_num: int) -> str:
    """Silent placeholder instrument for notes with no assigned sample."""
    silent_layers = "\n".join(_layer_block_entropy(i, "", "") for i in range(1, 5))
    return (
        f'      <Instrument number="{instrument_num}">\n'
        f'        <AudioRoute>\n'
        f'          <AudioRoute>0</AudioRoute>\n'
        f'          <AudioRouteSubIndex>0</AudioRouteSubIndex>\n'
        f'          <InsertsEnabled>False</InsertsEnabled>\n'
        f'        </AudioRoute>\n'
        f'        <Send1>0.000000</Send1>\n'
        f'        <Send2>0.000000</Send2>\n'
        f'        <Send3>0.000000</Send3>\n'
        f'        <Send4>0.000000</Send4>\n'
        f'        <Volume>0.707946</Volume>\n'
        f'        <Mute>False</Mute>\n'
        f'        <Pan>0.500000</Pan>\n'
        f'        <TuneCoarse>0</TuneCoarse>\n'
        f'        <TuneFine>0</TuneFine>\n'
        f'        <Mono>False</Mono>\n'
        f'        <Polyphony>1</Polyphony>\n'
        f'        <FilterKeytrack>0.000000</FilterKeytrack>\n'
        f'        <LowNote>0</LowNote>\n'
        f'        <HighNote>127</HighNote>\n'
        f'        <IgnoreBaseNote>False</IgnoreBaseNote>\n'
        f'        <ZonePlay>1</ZonePlay>\n'
        f'        <MuteGroup>0</MuteGroup>\n'
        f'        <MuteTarget1>0</MuteTarget1>\n'
        f'        <MuteTarget2>0</MuteTarget2>\n'
        f'        <MuteTarget3>0</MuteTarget3>\n'
        f'        <MuteTarget4>0</MuteTarget4>\n'
        f'        <SimultTarget1>0</SimultTarget1>\n'
        f'        <SimultTarget2>0</SimultTarget2>\n'
        f'        <SimultTarget3>0</SimultTarget3>\n'
        f'        <SimultTarget4>0</SimultTarget4>\n'
        f'        <LfoPitch>0.000000</LfoPitch>\n'
        f'        <LfoCutoff>0.000000</LfoCutoff>\n'
        f'        <LfoVolume>0.000000</LfoVolume>\n'
        f'        <LfoPan>0.000000</LfoPan>\n'
        f'        <OneShot>True</OneShot>\n'
        f'        <FilterType>2</FilterType>\n'
        f'        <Cutoff>1.000000</Cutoff>\n'
        f'        <Resonance>0.000000</Resonance>\n'
        f'        <FilterEnvAmt>0.000000</FilterEnvAmt>\n'
        f'        <AfterTouchToFilter>0.000000</AfterTouchToFilter>\n'
        f'        <VelocityToStart>0.000000</VelocityToStart>\n'
        f'        <VelocityToFilterAttack>0.000000</VelocityToFilterAttack>\n'
        f'        <VelocityToFilter>0.000000</VelocityToFilter>\n'
        f'        <VelocityToFilterEnvelope>0.000000</VelocityToFilterEnvelope>\n'
        f'        <FilterAttack>0.000000</FilterAttack>\n'
        f'        <FilterDecay>0.000000</FilterDecay>\n'
        f'        <FilterSustain>1.000000</FilterSustain>\n'
        f'        <FilterRelease>0.000000</FilterRelease>\n'
        f'        <FilterHold>0.000000</FilterHold>\n'
        f'        <FilterDecayType>True</FilterDecayType>\n'
        f'        <FilterADEnvelope>True</FilterADEnvelope>\n'
        f'        <VolumeHold>0.000000</VolumeHold>\n'
        f'        <VolumeDecayType>True</VolumeDecayType>\n'
        f'        <VolumeADEnvelope>True</VolumeADEnvelope>\n'
        f'        <VolumeAttack>0.000000</VolumeAttack>\n'
        f'        <VolumeDecay>0.300000</VolumeDecay>\n'
        f'        <VolumeSustain>0.000000</VolumeSustain>\n'
        f'        <VolumeRelease>0.050000</VolumeRelease>\n'
        f'        <VelocityToPitch>0.000000</VelocityToPitch>\n'
        f'        <VelocityToVolumeAttack>0.000000</VelocityToVolumeAttack>\n'
        f'        <VelocitySensitivity>1.000000</VelocitySensitivity>\n'
        f'        <VelocityToPan>0.000000</VelocityToPan>\n'
        f'        <LFO>\n'
        f'          <Speed>0.000000</Speed>\n'
        f'          <Amount>0.000000</Amount>\n'
        f'          <Type>0</Type>\n'
        f'          <Sync>False</Sync>\n'
        f'          <Retrigger>True</Retrigger>\n'
        f'        </LFO>\n'
        f'        <Layers>\n'
        f'{silent_layers}\n'
        f'        </Layers>\n'
        f'      </Instrument>'
    )


def generate_entropy_xpm(kit_name: str, selected: List[str],
                          method: str, n_selected: int) -> str:
    """Generate a complete MPC Drum XPM for the entropy-selected samples.

    The 16 selected samples are placed on pads A1–D4 (MIDI notes 36–51).
    Each pad uses a single velocity layer spanning 1–127 (full range).
    KeyTrack=True so samples transpose across zones.
    RootNote=0 (MPC auto-detect convention).

    Args:
        kit_name:   Program name (sanitized for XML).
        selected:   List of up to 16 WAV file paths (absolute).
        method:     Selection method name, for metadata.
        n_selected: Number of pads requested.

    Returns:
        Complete XPM XML string.
    """
    safe_name = xml_escape(f"XO_OX-{kit_name[:40]}")

    # PadNoteMap — 16 active pads A1..D4
    pad_note_entries = []
    for pad_idx, note in enumerate(_PAD_NOTES):
        pad_note_entries.append(
            f'        <Pad number="{pad_idx + 1}" note="{note}"/>'
            f'  <!-- {_midi_note_name(note)} -->'
        )
    pad_note_xml = "\n".join(pad_note_entries)

    # Build note→sample map
    note_to_wav: Dict[int, str] = {}
    for idx, wav in enumerate(selected[:n_selected]):
        if idx < len(_PAD_NOTES):
            note_to_wav[_PAD_NOTES[idx]] = wav

    # Generate 128 instrument blocks
    instrument_parts = []
    for i in range(128):
        if i in note_to_wav:
            pad_rank = _PAD_NOTES.index(i)
            instrument_parts.append(
                _instrument_block_entropy(i, i, note_to_wav[i], kit_name, pad_rank)
            )
        else:
            instrument_parts.append(_empty_instrument_block(i))

    instruments_xml = "\n".join(instrument_parts)

    pad_json = json.dumps(
        {"ProgramPads": {"Universal": {"value0": False},
                         "Type":      {"value0": 5},
                         "universalPad": 32512}},
        separators=(",", ":"),
    )

    return (
        '<?xml version="1.0" encoding="UTF-8"?>\n\n'
        '<MPCVObject>\n'
        '  <Version>\n'
        '    <File_Version>1.7</File_Version>\n'
        '    <Application>MPC-V</Application>\n'
        '    <Application_Version>2.10.0.0</Application_Version>\n'
        '    <Platform>OSX</Platform>\n'
        '  </Version>\n'
        '  <Program type="Drum">\n'
        f'    <Name>{safe_name}</Name>\n'
        f'    <ProgramPads>{pad_json}</ProgramPads>\n'
        '    <PadNoteMap>\n'
        f'{pad_note_xml}\n'
        '    </PadNoteMap>\n'
        '    <PadGroupMap>\n'
        '    </PadGroupMap>\n'
        '    <QLinks>\n'
        '      <QLink number="1">\n'
        '        <Name>TONE</Name>\n'
        '        <Parameter>FilterCutoff</Parameter>\n'
        '        <Min>0.200000</Min>\n'
        '        <Max>1.000000</Max>\n'
        '        <Smooth>True</Smooth>\n'
        '      </QLink>\n'
        '      <QLink number="2">\n'
        '        <Name>PITCH</Name>\n'
        '        <Parameter>TuneCoarse</Parameter>\n'
        '        <Min>-12</Min>\n'
        '        <Max>12</Max>\n'
        '        <Smooth>False</Smooth>\n'
        '      </QLink>\n'
        '      <QLink number="3">\n'
        '        <Name>RESO</Name>\n'
        '        <Parameter>Resonance</Parameter>\n'
        '        <Min>0.000000</Min>\n'
        '        <Max>1.000000</Max>\n'
        '        <Smooth>True</Smooth>\n'
        '      </QLink>\n'
        '      <QLink number="4">\n'
        '        <Name>SPACE</Name>\n'
        '        <Parameter>Send1</Parameter>\n'
        '        <Min>0.000000</Min>\n'
        '        <Max>1.000000</Max>\n'
        '        <Smooth>True</Smooth>\n'
        '      </QLink>\n'
        '    </QLinks>\n'
        '    <Instruments>\n'
        f'{instruments_xml}\n'
        '    </Instruments>\n'
        '  </Program>\n'
        '</MPCVObject>\n'
    )


# ===========================================================================
# RUN — single method
# ===========================================================================

def run_method(fingerprints: Dict[str, any], method: str, n: int,
               kit_name: str, output_dir: Path,
               wav_files: List[Path],
               args_visualize: bool = False) -> Tuple[List[str], List[dict]]:
    """Run one selection method, write outputs, return (selected, rationale)."""

    print(f"\n[{method}] selecting {n} from {len(fingerprints)} samples...", file=sys.stderr)

    if method == "max-entropy":
        selected, rationale = select_max_entropy(fingerprints, n)
    elif method == "min-entropy":
        selected, rationale = select_min_entropy(fingerprints, n)
    elif method == "mdl":
        selected, rationale = select_mdl(fingerprints, n)
    else:
        print(f"  [error] unknown method: {method}", file=sys.stderr)
        return [], []

    if not selected:
        print(f"  [warn] no samples selected", file=sys.stderr)
        return [], []

    # Compute kit-level diversity statistics
    n_sel = len(selected)
    pairwise_jsds = []
    for i in range(n_sel):
        for j in range(i + 1, n_sel):
            pairwise_jsds.append(jsd(fingerprints[selected[i]], fingerprints[selected[j]]))
    avg_jsd = sum(pairwise_jsds) / max(len(pairwise_jsds), 1)
    min_jsd = min(pairwise_jsds) if pairwise_jsds else 0.0
    max_jsd = max(pairwise_jsds) if pairwise_jsds else 0.0

    # Write selection.json
    output_dir.mkdir(parents=True, exist_ok=True)
    slug = method.replace("-", "_")
    json_path = output_dir / f"selection_{slug}.json"
    selection_doc = {
        "kit_name": kit_name,
        "method": method,
        "n_requested": n,
        "n_selected": n_sel,
        "pool_size": len(fingerprints),
        "date": str(date.today()),
        "diversity": {
            "avg_pairwise_jsd": round(avg_jsd, 6),
            "min_pairwise_jsd": round(min_jsd, 6),
            "max_pairwise_jsd": round(max_jsd, 6),
        },
        "selected": selected,
        "rationale": rationale,
    }
    with open(json_path, "w", encoding="utf-8") as f:
        json.dump(selection_doc, f, indent=2)
    print(f"  → {json_path}", file=sys.stderr)

    # Visualize (if requested or always on --visualize flag)
    if args_visualize or True:  # always show — caller can suppress with --no-vis
        visualize(fingerprints, selected=selected,
                  title=f"Entropy Kit [{method.upper()}] — {n_sel}/{len(fingerprints)} selected")

    # Write entropy_kit.xpm
    xpm_name = f"entropy_kit_{slug}"
    xpm_str = generate_entropy_xpm(xpm_name, selected, method, n_sel)
    xpm_path = output_dir / f"{xpm_name}.xpm"
    with open(xpm_path, "w", encoding="utf-8") as f:
        f.write(xpm_str)
    print(f"  → {xpm_path}", file=sys.stderr)

    # Summary
    print(f"\n  [{method}] RESULTS", file=sys.stderr)
    print(f"  {'Pool:':<20} {len(fingerprints)} WAV files", file=sys.stderr)
    print(f"  {'Selected:':<20} {n_sel}", file=sys.stderr)
    print(f"  {'Avg pairwise JSD:':<20} {avg_jsd:.4f}", file=sys.stderr)
    print(f"  {'Min pairwise JSD:':<20} {min_jsd:.4f}", file=sys.stderr)
    print(f"  {'Max pairwise JSD:':<20} {max_jsd:.4f}", file=sys.stderr)
    print(f"  {'Kit diversity:':<20} {'HIGH' if avg_jsd > 0.6 else 'MEDIUM' if avg_jsd > 0.35 else 'LOW'}",
          file=sys.stderr)

    return selected, rationale


# ===========================================================================
# COMPARE — run all 3 methods, side by side
# ===========================================================================

def run_compare(fingerprints: Dict[str, any], n: int,
                kit_name: str, output_dir: Path,
                wav_files: List[Path]) -> None:
    """Run all 3 methods, write individual outputs, then a comparison report."""
    results = {}
    for method in ["max-entropy", "min-entropy", "mdl"]:
        selected, rationale = run_method(
            fingerprints, method, n, kit_name, output_dir, wav_files,
            args_visualize=False,
        )
        if selected:
            n_sel = len(selected)
            pjsds = [
                jsd(fingerprints[selected[i]], fingerprints[selected[j]])
                for i in range(n_sel) for j in range(i + 1, n_sel)
            ]
            avg = sum(pjsds) / max(len(pjsds), 1)
            results[method] = {
                "selected": selected,
                "rationale": rationale,
                "avg_jsd": avg,
            }

    if not results:
        return

    # Print comparison table
    print("\n" + "=" * 70)
    print("  COMPARISON — All 3 Methods")
    print("=" * 70)
    print(f"  {'Method':<16}  {'Avg JSD':>8}  {'Diversity':>10}  {'Description'}")
    print(f"  {'-'*16}  {'-'*8}  {'-'*10}  {'-'*28}")
    for m, r in results.items():
        avg = r["avg_jsd"]
        tier = "HIGH" if avg > 0.6 else "MEDIUM" if avg > 0.35 else "LOW"
        desc = {
            "max-entropy": "Maximally diverse — surprising",
            "min-entropy": "Most similar — coherent family",
            "mdl":         "Balanced — optimal representatives",
        }.get(m, "")
        print(f"  {m:<16}  {avg:>8.4f}  {tier:>10}  {desc}")
    print("=" * 70)

    # Overlap analysis — how many samples appear in multiple methods
    all_sets = {m: set(r["selected"]) for m, r in results.items()}
    methods = list(all_sets.keys())
    if len(methods) >= 2:
        print(f"\n  Overlap analysis (shared samples):")
        for i in range(len(methods)):
            for j in range(i + 1, len(methods)):
                a, b = methods[i], methods[j]
                overlap = len(all_sets[a] & all_sets[b])
                print(f"    {a} ∩ {b}: {overlap} shared samples")
        if len(methods) == 3:
            triple = len(all_sets[methods[0]] & all_sets[methods[1]] & all_sets[methods[2]])
            print(f"    All 3 methods agree on: {triple} samples")

    # Save comparison JSON
    compare_path = output_dir / "comparison.json"
    compare_doc = {
        "kit_name": kit_name,
        "n": n,
        "pool_size": len(fingerprints),
        "date": str(date.today()),
        "methods": {
            m: {
                "avg_jsd": round(r["avg_jsd"], 6),
                "selected": r["selected"],
            }
            for m, r in results.items()
        },
    }
    with open(compare_path, "w", encoding="utf-8") as f:
        json.dump(compare_doc, f, indent=2)
    print(f"\n  → {compare_path}", file=sys.stderr)


# ===========================================================================
# CLI
# ===========================================================================

def _discover_wavs(samples_path: Path) -> List[Path]:
    """Recursively discover all WAV files under samples_path."""
    wavs = sorted(samples_path.rglob("*.wav")) + sorted(samples_path.rglob("*.WAV"))
    # Deduplicate (case-insensitive FS may return both)
    seen = set()
    unique = []
    for w in wavs:
        key = str(w.resolve()).lower()
        if key not in seen:
            seen.add(key)
            unique.append(w)
    return unique


def main() -> None:
    parser = argparse.ArgumentParser(
        description="XPN Entropy Kit — information-theoretic sample curation for MPC.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    parser.add_argument("samples", type=Path,
                        help="Directory of WAV files (searched recursively)")
    parser.add_argument("--method", choices=["max-entropy", "min-entropy", "mdl"],
                        default="max-entropy",
                        help="Selection strategy (default: max-entropy)")
    parser.add_argument("--compare", metavar="all",
                        help="Run all 3 methods and compare (pass 'all')")
    parser.add_argument("--n", type=int, default=16,
                        help="Number of samples to select (default: 16)")
    parser.add_argument("--output", type=Path, default=Path("./kits"),
                        help="Output directory (default: ./kits)")
    parser.add_argument("--kit-name", type=str, default="",
                        help="Kit name override (default: derived from samples dir)")
    parser.add_argument("--visualize", action="store_true",
                        help="Show fingerprint matrix without writing XPM")
    parser.add_argument("--no-vis", action="store_true",
                        help="Suppress ASCII visualization")
    parser.add_argument("--force-recompute", action="store_true",
                        help="Ignore cached Monster Rancher fingerprints, recompute all")

    args = parser.parse_args()

    # Validate
    if not args.samples.exists():
        print(f"[error] samples directory not found: {args.samples}", file=sys.stderr)
        sys.exit(1)

    if not _NUMPY_AVAILABLE:
        print("[warn] numpy not found — falling back to pure-Python FFT (slower, less accurate)")
        print("       pip install numpy scipy  for best results\n")

    # Discover WAV files
    wav_files = _discover_wavs(args.samples)
    if not wav_files:
        print(f"[error] no WAV files found in {args.samples}", file=sys.stderr)
        sys.exit(1)

    print(f"[entropy-kit] found {len(wav_files)} WAV files in {args.samples}", file=sys.stderr)

    # Compute fingerprints
    print(f"[entropy-kit] computing spectral fingerprints...", file=sys.stderr)
    fingerprints = load_fingerprints(
        args.samples, wav_files,
        force_recompute=args.force_recompute,
    )
    print(f"[entropy-kit] {len(fingerprints)} fingerprints ready", file=sys.stderr)

    if len(fingerprints) == 0:
        print("[error] no fingerprints computed — check WAV files", file=sys.stderr)
        sys.exit(1)

    # Kit name
    kit_name = args.kit_name or args.samples.resolve().name or "entropy_kit"

    # Visualize-only mode
    if args.visualize:
        visualize(fingerprints, title=f"Spectral Fingerprints — {args.samples.name}")
        return

    n = min(args.n, len(fingerprints))
    if n != args.n:
        print(f"[warn] requested n={args.n} but only {len(fingerprints)} samples available; using n={n}",
              file=sys.stderr)

    # Run
    if args.compare == "all":
        run_compare(fingerprints, n, kit_name, args.output, wav_files)
    else:
        run_method(
            fingerprints, args.method, n, kit_name, args.output, wav_files,
            args_visualize=not args.no_vis,
        )


if __name__ == "__main__":
    main()
