#!/usr/bin/env python3
"""
XPN Entropy Kit — XO_OX Designs
Hex + Rex :: hacker android + XPN format android

Information-theoretic sample selection for optimal XPN kit curation.
Given N candidate WAV files, select k that maximize (or minimize)
information content using Jensen-Shannon divergence on 8-band spectral
fingerprints computed from raw PCM via stdlib only.

Three selection strategies:
  max-entropy   Greedy JSD: each new sample maximizes average pairwise
                divergence from the already-selected set.
                Produces maximally diverse kits.
  min-entropy   Anti-entropy: select the most spectrally similar samples
                for maximum coherence — tonal family kits.
  k-medoids     k-medoids clustering (PAM swap algorithm). Returns medoid
                samples as canonical cluster representatives.

Integration:
  If monster_metadata.json exists in the samples directory (produced by
  xpn_monster_rancher.py), band_energy_mean fingerprints are loaded from
  there instead of recomputed from PCM.

Output (inside XPN ZIP):
  {kit_name}.xpm          KeygroupProgram XML — 16 pads across C2–D#3
  entropy_report.txt      Selection rationale, diversity scores, band profiles

CLI:
  python xpn_entropy_kit.py --input ./samples/ --method max-entropy --count 16 --output ./out/
  python xpn_entropy_kit.py --input ./samples/ --method min-entropy --count 16 --output ./out/
  python xpn_entropy_kit.py --input ./samples/ --method k-medoids   --count 16 --output ./out/

Dependencies: pure stdlib (no numpy/scipy/soundfile/librosa).
"""


import argparse
import cmath
import json
import math
import os
import struct
import sys
import wave
import zipfile
from datetime import date
from pathlib import Path
from typing import Dict, List, Optional, Tuple
from xml.sax.saxutils import escape as xml_escape

# ---------------------------------------------------------------------------
# Band definitions — XOptic crossover frequencies
# [20, 80, 200, 500, 1000, 4000, 8000, 16000, 20000] Hz
# ---------------------------------------------------------------------------
CROSSOVER_FREQS: List[float] = [80.0, 200.0, 500.0, 1000.0, 4000.0, 8000.0, 16000.0]
BAND_NAMES = ["Sub", "Bass", "LoMid", "Mid", "HiMid", "Presence", "Brilliance", "Air"]
NUM_BANDS = 8
BAND_EDGES: List[Tuple[float, float]] = []
for _i in range(NUM_BANDS):
    _lo = 20.0 if _i == 0 else CROSSOVER_FREQS[_i - 1]
    _hi = 20000.0 if _i == NUM_BANDS - 1 else CROSSOVER_FREQS[_i]
    BAND_EDGES.append((_lo, _hi))

EPSILON = 1e-12
MONSTER_METADATA_FILE = "monster_metadata.json"

# MIDI layout for 16 pads: C2 (36) through D#3 (51)
PAD_ROOT_NOTES = list(range(36, 52))  # 16 pads

# ===========================================================================
# WAV READER — pure stdlib (wave + struct)
# ===========================================================================

def read_wav_mono(path: str) -> Tuple[List[float], int]:
    """Read a WAV file to a mono float list via stdlib wave + struct.
    Returns (samples, sample_rate). Handles 8/16/24/32-bit PCM.
    """
    try:
        with wave.open(path, "rb") as wf:
            nch    = wf.getnchannels()
            sw     = wf.getsampwidth()
            sr     = wf.getframerate()
            nframes = wf.getnframes()
            raw    = wf.readframes(nframes)
    except Exception as exc:
        raise IOError(f"Cannot read WAV '{path}': {exc}") from exc

    bit_depth = sw * 8
    if bit_depth == 8:
        all_samples: List[float] = [(v - 128) / 128.0 for v in raw]
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
        raise ValueError(f"Unsupported bit depth {bit_depth} in '{path}'")

    # Downmix to mono
    if nch > 1:
        mono: List[float] = []
        for i in range(0, len(all_samples) - nch + 1, nch):
            mono.append(sum(all_samples[i: i + nch]) / nch)
        all_samples = mono

    return all_samples, sr


# ===========================================================================
# SPECTRAL FINGERPRINT — pure stdlib DFT
# ===========================================================================

def _next_pow2(n: int) -> int:
    p = 1
    while p < n:
        p <<= 1
    return p


def _fft_recursive(x: List[complex]) -> List[complex]:
    """Cooley-Tukey radix-2 FFT. len(x) must be a power of 2."""
    N = len(x)
    if N <= 1:
        return x
    even = _fft_recursive(x[0::2])
    odd  = _fft_recursive(x[1::2])
    T = [cmath.exp(-2j * math.pi * k / N) * odd[k] for k in range(N // 2)]
    return [even[k] + T[k] for k in range(N // 2)] + \
           [even[k] - T[k] for k in range(N // 2)]


def compute_band_energy(samples: List[float], sr: int) -> List[float]:
    """Compute 8-band spectral energy via stdlib FFT (power spectrum binning).

    Down-samples to at most MAX_FFT_SIZE frames for speed, pads to power-of-2,
    then bins the one-sided power spectrum into the 8 frequency bands.
    Returns a list of 8 floats (unnormalized energy).
    """
    MAX_FFT_SIZE = 8192

    N = len(samples)
    if N == 0:
        return [0.0] * NUM_BANDS

    # Decimate to max size (keep every k-th sample)
    if N > MAX_FFT_SIZE:
        step = N // MAX_FFT_SIZE
        samples = samples[::step]
        N = len(samples)

    # Zero-pad to next power of 2
    fft_size = _next_pow2(N)
    padded: List[complex] = [complex(samples[i], 0.0) if i < N else 0j
                              for i in range(fft_size)]

    spectrum = _fft_recursive(padded)

    # One-sided power spectrum (bins 0 … fft_size//2)
    half = fft_size // 2 + 1
    freq_res = sr / fft_size  # Hz per bin

    energy = [0.0] * NUM_BANDS
    for k in range(half):
        freq = k * freq_res
        power = abs(spectrum[k]) ** 2
        for b, (lo, hi) in enumerate(BAND_EDGES):
            if lo <= freq < hi:
                energy[b] += power
                break

    return energy


def normalize_to_distribution(energy: List[float]) -> List[float]:
    """Normalize 8-band energy to a probability distribution (sums to 1)."""
    total = sum(energy) + EPSILON
    return [v / total for v in energy]


def compute_fingerprint(wav_path: str) -> List[float]:
    """Compute 8-band spectral fingerprint for a WAV file.
    Returns a normalized probability distribution (8 floats, sum ≈ 1).
    """
    samples, sr = read_wav_mono(wav_path)
    energy = compute_band_energy(samples, sr)
    fp = normalize_to_distribution(energy)
    # Guard: if all bands silent, return uniform
    if sum(energy) <= EPSILON:
        return [1.0 / NUM_BANDS] * NUM_BANDS
    return fp


# ===========================================================================
# LOAD FINGERPRINTS (with Monster Rancher cache integration)
# ===========================================================================

def load_fingerprints(
    samples_dir: Path,
    wav_files: List[Path],
    force_recompute: bool = False,
) -> Dict[str, List[float]]:
    """Load fingerprints — from monster_metadata.json when available, else compute.

    monster_metadata.json format (produced by xpn_monster_rancher.py):
      { "filename.wav": { "band_energy_mean": [f0, f1, ..., f7], ... }, ... }

    Returns dict: { absolute_path_str -> fingerprint_list }
    """
    cached: Dict[str, List[float]] = {}

    if not force_recompute:
        meta_path = samples_dir / MONSTER_METADATA_FILE
        if meta_path.exists():
            try:
                with open(meta_path, "r", encoding="utf-8") as f:
                    meta = json.load(f)
                for entry_name, data in meta.items():
                    if "band_energy_mean" in data:
                        raw = data["band_energy_mean"]
                        if isinstance(raw, list) and len(raw) == NUM_BANDS:
                            # Find matching wav file (by basename)
                            for wav in wav_files:
                                if wav.name == entry_name or wav.stem == Path(entry_name).stem:
                                    cached[str(wav.resolve())] = normalize_to_distribution(
                                        [float(v) for v in raw]
                                    )
                                    break
                print(f"[entropy-kit] loaded {len(cached)} cached fingerprints "
                      f"from {MONSTER_METADATA_FILE}", file=sys.stderr)
            except Exception as exc:
                print(f"[entropy-kit] warning: could not parse {MONSTER_METADATA_FILE}: {exc}",
                      file=sys.stderr)

    fingerprints: Dict[str, List[float]] = {}
    need_compute = []
    for wav in wav_files:
        key = str(wav.resolve())
        if key in cached:
            fingerprints[key] = cached[key]
        else:
            need_compute.append(wav)

    if need_compute:
        print(f"[entropy-kit] computing fingerprints for {len(need_compute)} WAV files...",
              file=sys.stderr)
        for i, wav in enumerate(need_compute):
            key = str(wav.resolve())
            try:
                fingerprints[key] = compute_fingerprint(key)
                if (i + 1) % 20 == 0 or (i + 1) == len(need_compute):
                    print(f"  [{i + 1}/{len(need_compute)}] {wav.name}", file=sys.stderr)
            except Exception as exc:
                print(f"  [skip] {wav.name}: {exc}", file=sys.stderr)

    return fingerprints


# ===========================================================================
# JENSEN-SHANNON DIVERGENCE
# ===========================================================================

def jsd(p: List[float], q: List[float]) -> float:
    """Jensen-Shannon divergence between two probability distributions.
    Returns a value in [0, 1] (using base-2 log, max = 1.0).
    """
    m = [(a + b) / 2.0 for a, b in zip(p, q)]

    def kl(a: List[float], b: List[float]) -> float:
        s = 0.0
        for ai, bi in zip(a, b):
            if ai > EPSILON and bi > EPSILON:
                s += ai * math.log2(ai / bi)
        return s

    return max(0.0, (kl(p, m) + kl(q, m)) / 2.0)


def avg_pairwise_jsd(keys: List[str], fps: Dict[str, List[float]]) -> float:
    """Average pairwise JSD for a set of samples."""
    n = len(keys)
    if n < 2:
        return 0.0
    total = 0.0
    count = 0
    for i in range(n):
        for j in range(i + 1, n):
            total += jsd(fps[keys[i]], fps[keys[j]])
            count += 1
    return total / count if count > 0 else 0.0


# ===========================================================================
# SELECTION ALGORITHMS
# ===========================================================================

def select_max_entropy(
    fingerprints: Dict[str, List[float]],
    k: int,
) -> Tuple[List[str], List[float]]:
    """Greedy Jensen-Shannon max-entropy selection.

    At each step, pick the sample that maximizes the average pairwise JSD
    with the already-selected set. Seed: sample with highest entropy.

    Returns (selected_keys, per_step_gain).
    """
    keys = list(fingerprints.keys())
    if not keys:
        return [], []
    k = min(k, len(keys))

    # Seed: highest Shannon entropy (most spread-out distribution)
    def shannon(p: List[float]) -> float:
        return -sum(v * math.log2(v + EPSILON) for v in p)

    seed = max(keys, key=lambda kk: shannon(fingerprints[kk]))
    selected = [seed]
    remaining = [kk for kk in keys if kk != seed]
    gains: List[float] = [shannon(fingerprints[seed])]

    while len(selected) < k and remaining:
        # For each candidate, compute avg JSD with current selected set
        best_key = remaining[0]
        best_score = -1.0
        for cand in remaining:
            fp_c = fingerprints[cand]
            score = sum(jsd(fp_c, fingerprints[s]) for s in selected) / len(selected)
            if score > best_score:
                best_score = score
                best_key = cand
        selected.append(best_key)
        remaining.remove(best_key)
        gains.append(best_score)

    return selected, gains


def select_min_entropy(
    fingerprints: Dict[str, List[float]],
    k: int,
) -> Tuple[List[str], List[float]]:
    """Anti-entropy: select the k most spectrally similar (coherent) samples.

    Seed: sample closest to the centroid of all fingerprints.
    At each step, add the sample with minimum average JSD to the selected set.

    Returns (selected_keys, per_step_similarity_scores).
    """
    keys = list(fingerprints.keys())
    if not keys:
        return [], []
    k = min(k, len(keys))

    # Centroid
    centroid = [0.0] * NUM_BANDS
    for fp in fingerprints.values():
        for b in range(NUM_BANDS):
            centroid[b] += fp[b]
    total = sum(centroid) + EPSILON
    centroid = [v / total for v in centroid]

    # Seed: closest to centroid
    seed = min(keys, key=lambda kk: jsd(fingerprints[kk], centroid))
    selected = [seed]
    remaining = [kk for kk in keys if kk != seed]
    scores: List[float] = [jsd(fingerprints[seed], centroid)]

    while len(selected) < k and remaining:
        best_key = remaining[0]
        best_score = float("inf")
        for cand in remaining:
            fp_c = fingerprints[cand]
            score = sum(jsd(fp_c, fingerprints[s]) for s in selected) / len(selected)
            if score < best_score:
                best_score = score
                best_key = cand
        selected.append(best_key)
        remaining.remove(best_key)
        scores.append(best_score)

    return selected, scores


def _assign_clusters(keys: List[str], medoid_keys: List[str],
                     fps: Dict[str, List[float]]) -> List[int]:
    """Assign each sample to its nearest medoid."""
    assignments = []
    for kk in keys:
        dists = [jsd(fps[kk], fps[m]) for m in medoid_keys]
        assignments.append(dists.index(min(dists)))
    return assignments


def _cluster_cost(keys: List[str], medoid_keys: List[str],
                  fps: Dict[str, List[float]]) -> float:
    """Total within-cluster JSD cost."""
    assignments = _assign_clusters(keys, medoid_keys, fps)
    return sum(jsd(fps[keys[i]], fps[medoid_keys[a]])
               for i, a in enumerate(assignments))


def select_k_medoids(
    fingerprints: Dict[str, List[float]],
    k: int,
) -> Tuple[List[str], List[float]]:
    """k-medoids (PAM swap algorithm) — returns canonical representative samples.

    Initialization: k-medoids++ style — first medoid is sample nearest centroid,
    subsequent medoids are chosen to maximize distance from existing medoids.

    Returns (medoid_keys, per_medoid_avg_cluster_jsd).
    """
    keys = list(fingerprints.keys())
    if not keys:
        return [], []
    k = min(k, len(keys))
    if k == len(keys):
        return keys[:], [0.0] * k

    # Initialization: k-means++ style seeding
    centroid = [0.0] * NUM_BANDS
    for fp in fingerprints.values():
        for b in range(NUM_BANDS):
            centroid[b] += fp[b]
    total = sum(centroid) + EPSILON
    centroid = [v / total for v in centroid]

    medoids: List[str] = [min(keys, key=lambda kk: jsd(fingerprints[kk], centroid))]
    for _ in range(k - 1):
        # Choose sample that maximizes min distance to existing medoids
        remaining = [kk for kk in keys if kk not in medoids]
        if not remaining:
            break
        next_m = max(remaining,
                     key=lambda kk: min(jsd(fingerprints[kk], fingerprints[m])
                                        for m in medoids))
        medoids.append(next_m)

    # PAM swap phase
    MAX_ITERS = 20
    improved = True
    iters = 0
    current_cost = _cluster_cost(keys, medoids, fingerprints)

    while improved and iters < MAX_ITERS:
        improved = False
        iters += 1
        non_medoids = [kk for kk in keys if kk not in medoids]
        for mi, m in enumerate(medoids):
            for nm in non_medoids:
                candidate = medoids[:]
                candidate[mi] = nm
                cost = _cluster_cost(keys, candidate, fingerprints)
                if cost < current_cost - EPSILON:
                    medoids = candidate
                    current_cost = cost
                    improved = True
                    break
            if improved:
                break

    # Per-medoid cluster quality
    assignments = _assign_clusters(keys, medoids, fingerprints)
    cluster_jsds: Dict[int, List[float]] = {i: [] for i in range(len(medoids))}
    for i, a in enumerate(assignments):
        cluster_jsds[a].append(jsd(fingerprints[keys[i]], fingerprints[medoids[a]]))
    scores = [
        sum(cluster_jsds[i]) / max(len(cluster_jsds[i]), 1)
        for i in range(len(medoids))
    ]

    return medoids, scores


# ===========================================================================
# XPN / KEYGROUP XML BUILDER
# ===========================================================================

# MIDI note 36 = C2, 51 = D#3 (16 pads)
_NOTE_NAMES_CHROMATIC = ["C", "C#", "D", "D#", "E", "F",
                          "F#", "G", "G#", "A", "A#", "B"]


def midi_to_note_name(midi: int) -> str:
    """Convert MIDI note number to name string (C-1=0 convention)."""
    octave = (midi // 12) - 1
    note = _NOTE_NAMES_CHROMATIC[midi % 12]
    return f"{note}{octave}"


def build_keygroup_xml(
    kit_name: str,
    sample_paths: List[str],
) -> str:
    """Build a KeygroupProgram XPM XML string for up to 16 samples.

    Each sample gets its own keygroup spanning its root note only.
    XPN golden rules:
      - KeyTrack=True
      - RootNote=0 (MPC auto-detect)
      - Empty layer VelStart=0
    """
    # Assign pads — up to 16 samples
    pads = sample_paths[:16]
    n_pads = len(pads)

    # Keygroup zone range: each pad owns 1 semitone at its root
    # Root notes: C2(36) .. D#3(51) for 16 pads
    root_notes = PAD_ROOT_NOTES[:n_pads]

    lines = []
    lines.append('<?xml version="1.0" encoding="UTF-8"?>')
    lines.append(f'<MPCVObject Version="2.1" Creator="XPN Entropy Kit">')
    lines.append(f'  <Instrument type="KeygroupProgram">')
    lines.append(f'    <ProgramName>{xml_escape(kit_name)}</ProgramName>')
    lines.append(f'    <NumKeygroupsPerLayer>{n_pads}</NumKeygroupsPerLayer>')
    lines.append(f'    <KeyGroups>')

    for idx, (path, root) in enumerate(zip(pads, root_notes)):
        fname = os.path.basename(path)
        note_name = midi_to_note_name(root)
        # Each keygroup spans just this note (LowNote = HighNote = root)
        low_note  = root
        high_note = root

        lines.append(f'      <KeyGroup number="{idx + 1}">')
        lines.append(f'        <KeyLow>{low_note}</KeyLow>')
        lines.append(f'        <KeyHigh>{high_note}</KeyHigh>')
        lines.append(f'        <KeyTrack>True</KeyTrack>')
        lines.append(f'        <RootNote>0</RootNote>')
        lines.append(f'        <Layer number="1">')
        lines.append(f'          <SampleFile>{xml_escape(fname)}</SampleFile>')
        lines.append(f'          <RootNote>{root}</RootNote>')
        lines.append(f'          <KeyTrack>True</KeyTrack>')
        lines.append(f'          <VelStart>0</VelStart>')
        lines.append(f'          <VelEnd>127</VelEnd>')
        lines.append(f'        </Layer>')
        # Empty layers 2–4
        for layer_n in (2, 3, 4):
            lines.append(f'        <Layer number="{layer_n}">')
            lines.append(f'          <SampleFile></SampleFile>')
            lines.append(f'          <VelStart>0</VelStart>')
            lines.append(f'          <VelEnd>0</VelEnd>')
            lines.append(f'        </Layer>')
        lines.append(f'      </KeyGroup>')

    lines.append(f'    </KeyGroups>')
    lines.append(f'  </Instrument>')
    lines.append(f'</MPCVObject>')

    return "\n".join(lines)


# ===========================================================================
# ENTROPY REPORT
# ===========================================================================

def build_entropy_report(
    method: str,
    kit_name: str,
    all_wav_count: int,
    selected: List[str],
    scores: List[float],
    fingerprints: Dict[str, List[float]],
) -> str:
    """Build a human-readable entropy_report.txt for the ZIP."""
    lines = []
    lines.append("XPN Entropy Kit — Selection Report")
    lines.append("XO_OX Designs")
    lines.append("=" * 60)
    lines.append(f"Date:          {date.today()}")
    lines.append(f"Kit name:      {kit_name}")
    lines.append(f"Method:        {method}")
    lines.append(f"Pool size:     {all_wav_count} WAV files")
    lines.append(f"Selected:      {len(selected)} samples")
    lines.append("")

    method_desc = {
        "max-entropy": (
            "Greedy Jensen-Shannon divergence — each sample maximises\n"
            "  average pairwise spectral surprise vs. the already-selected set.\n"
            "  Produces maximally diverse kits."
        ),
        "min-entropy": (
            "Anti-entropy / coherence — each sample minimises\n"
            "  average pairwise spectral distance from the selected set.\n"
            "  Produces spectrally similar, family-coherent kits."
        ),
        "k-medoids": (
            "k-medoids clustering (PAM swap algorithm) — returns\n"
            "  canonical medoid representatives from k clusters.\n"
            "  Balanced coverage of the spectral space."
        ),
    }
    lines.append("Method description:")
    lines.append(f"  {method_desc.get(method, method)}")
    lines.append("")

    # Diversity summary
    avg_jsd = avg_pairwise_jsd(selected, fingerprints)
    tier = "HIGH" if avg_jsd > 0.6 else "MEDIUM" if avg_jsd > 0.35 else "LOW"
    lines.append(f"Avg pairwise JSD:  {avg_jsd:.4f}  ({tier} diversity)")
    lines.append("")

    # Per-sample table
    lines.append("Selected Samples:")
    lines.append(f"  {'#':<4} {'Score':>8}  {'File'}")
    lines.append(f"  {'-'*4} {'-'*8}  {'-'*40}")
    for i, (path, score) in enumerate(zip(selected, scores)):
        fname = os.path.basename(path)
        lines.append(f"  {i + 1:<4} {score:>8.4f}  {fname}")
    lines.append("")

    # Band profile table
    lines.append("Spectral Band Profiles (selected samples):")
    header = f"  {'File':<28}" + "".join(f"  {n:>9}" for n in BAND_NAMES)
    lines.append(header)
    lines.append("  " + "-" * (28 + 11 * NUM_BANDS))
    for path in selected:
        fname = os.path.basename(path)[:26]
        fp = fingerprints.get(path, [1.0 / NUM_BANDS] * NUM_BANDS)
        row = f"  {fname:<28}" + "".join(f"  {v:>9.4f}" for v in fp)
        lines.append(row)
    lines.append("")

    # ASCII diversity matrix
    lines.append("Pairwise JSD Matrix (selected):")
    short_names = [os.path.basename(p)[:12] for p in selected]
    col_w = 7
    header_row = "  " + " " * 14 + "".join(f"{n[:col_w]:>{col_w + 1}}" for n in short_names)
    lines.append(header_row)
    for i, pi in enumerate(selected):
        row_label = f"  {short_names[i]:<14}"
        cells = []
        for j, pj in enumerate(selected):
            if i == j:
                cells.append(f"{'---':>{col_w + 1}}")
            else:
                d = jsd(fingerprints[pi], fingerprints[pj])
                cells.append(f"{d:>{col_w + 1}.4f}")
        lines.append(row_label + "".join(cells))
    lines.append("")
    lines.append("JSD interpretation: 0.0 = identical, 1.0 = maximally different")
    lines.append("")
    lines.append("Generated by xpn_entropy_kit.py — XO_OX Designs")

    return "\n".join(lines)


# ===========================================================================
# ZIP PACKAGER
# ===========================================================================

def write_xpn_zip(
    output_path: Path,
    kit_name: str,
    xpm_xml: str,
    report_txt: str,
    sample_paths: List[str],
) -> None:
    """Write the XPN ZIP containing the XPM program and entropy report.

    Samples are stored at the root of the ZIP (MPC convention).
    """
    output_path.parent.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(str(output_path), "w", zipfile.ZIP_DEFLATED) as zf:
        # XPM program
        xpm_name = f"{kit_name}.xpm"
        zf.writestr(xpm_name, xpm_xml.encode("utf-8"))

        # Entropy report
        zf.writestr("entropy_report.txt", report_txt.encode("utf-8"))

        # Sample WAV files (copy into root of ZIP)
        for path in sample_paths:
            fname = os.path.basename(path)
            try:
                zf.write(path, fname)
            except FileNotFoundError:
                print(f"[warn] sample not found, skipping: {path}", file=sys.stderr)


# ===========================================================================
# DISCOVERY
# ===========================================================================

def discover_wavs(samples_path: Path) -> List[Path]:
    """Recursively discover all WAV files under samples_path."""
    wavs = sorted(samples_path.rglob("*.wav")) + sorted(samples_path.rglob("*.WAV"))
    seen: set = set()
    unique: List[Path] = []
    for w in wavs:
        key = str(w.resolve()).lower()
        if key not in seen:
            seen.add(key)
            unique.append(w)
    return unique


# ===========================================================================
# MAIN RUNNER
# ===========================================================================

def run(
    input_dir: Path,
    method: str,
    count: int,
    output_dir: Path,
    kit_name: str = "",
    force_recompute: bool = False,
) -> Path:
    """Full pipeline: discover → fingerprint → select → package.
    Returns path to written XPN ZIP.
    """
    # 1. Discover WAV files
    wav_files = discover_wavs(input_dir)
    if not wav_files:
        print(f"[error] no WAV files found in {input_dir}", file=sys.stderr)
        sys.exit(1)
    print(f"[entropy-kit] found {len(wav_files)} WAV files", file=sys.stderr)

    # 2. Fingerprints
    fingerprints = load_fingerprints(input_dir, wav_files, force_recompute)
    if not fingerprints:
        print("[error] no fingerprints computed — check WAV files", file=sys.stderr)
        sys.exit(1)
    print(f"[entropy-kit] {len(fingerprints)} fingerprints ready", file=sys.stderr)

    k = min(count, len(fingerprints))
    if k != count:
        print(f"[warn] requested count={count} but only {len(fingerprints)} samples; "
              f"using count={k}", file=sys.stderr)

    # 3. Selection
    print(f"[entropy-kit] running {method} selection (k={k})...", file=sys.stderr)
    if method == "max-entropy":
        selected, scores = select_max_entropy(fingerprints, k)
    elif method == "min-entropy":
        selected, scores = select_min_entropy(fingerprints, k)
    elif method == "k-medoids":
        selected, scores = select_k_medoids(fingerprints, k)
    else:
        print(f"[error] unknown method: {method}", file=sys.stderr)
        sys.exit(1)

    if not selected:
        print("[error] selection returned no samples", file=sys.stderr)
        sys.exit(1)

    # Derive kit name
    resolved_name = kit_name or input_dir.resolve().name or "entropy_kit"
    resolved_name = resolved_name.replace(" ", "_")

    # 4. Build XPM XML
    xpm_xml = build_keygroup_xml(resolved_name, selected)

    # 5. Build report
    report_txt = build_entropy_report(
        method=method,
        kit_name=resolved_name,
        all_wav_count=len(wav_files),
        selected=selected,
        scores=scores,
        fingerprints=fingerprints,
    )

    # 6. Print report to stdout for user
    print("\n" + report_txt, file=sys.stdout)

    # 7. Write ZIP
    output_dir.mkdir(parents=True, exist_ok=True)
    zip_name = f"{resolved_name}_{method.replace('-', '_')}.xpn.zip"
    zip_path = output_dir / zip_name
    write_xpn_zip(zip_path, resolved_name, xpm_xml, report_txt, selected)
    print(f"\n[entropy-kit] written: {zip_path}", file=sys.stderr)

    return zip_path


# ===========================================================================
# CLI
# ===========================================================================

def main() -> None:
    parser = argparse.ArgumentParser(
        description="XPN Entropy Kit — information-theoretic sample curation for MPC.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=(
            "Examples:\n"
            "  python xpn_entropy_kit.py --input ./samples/ --method max-entropy "
            "--count 16 --output ./out/\n"
            "  python xpn_entropy_kit.py --input ./samples/ --method k-medoids "
            "--count 8 --output ./out/\n"
        ),
    )
    parser.add_argument("--input", type=Path, required=True,
                        help="Directory of WAV files (searched recursively)")
    parser.add_argument("--method",
                        choices=["max-entropy", "min-entropy", "k-medoids"],
                        default="max-entropy",
                        help="Selection strategy (default: max-entropy)")
    parser.add_argument("--count", type=int, default=16,
                        help="Number of samples to select (default: 16)")
    parser.add_argument("--output", type=Path, default=Path("./out"),
                        help="Output directory for XPN ZIP (default: ./out)")
    parser.add_argument("--kit-name", type=str, default="",
                        help="Kit name override (default: derived from input dir name)")
    parser.add_argument("--force-recompute", action="store_true",
                        help="Ignore cached Monster Rancher fingerprints, recompute all")

    args = parser.parse_args()

    if not args.input.exists() or not args.input.is_dir():
        print(f"[error] --input directory not found: {args.input}", file=sys.stderr)
        sys.exit(1)

    run(
        input_dir=args.input,
        method=args.method,
        count=args.count,
        output_dir=args.output,
        kit_name=args.kit_name,
        force_recompute=args.force_recompute,
    )


if __name__ == "__main__":
    main()
