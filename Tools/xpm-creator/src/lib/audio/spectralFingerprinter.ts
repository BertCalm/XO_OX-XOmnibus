/**
 * Spectral Fingerprinter — Wave 2: Spectral Intelligence
 *
 * Manual FFT-based spectral analysis for audio buffers.
 * Does NOT use AnalyserNode (which has scheduling issues in OfflineAudioContext).
 * Implements a radix-2 Cooley-Tukey FFT and derives frequency-band energies,
 * peak frequency, spectral centroid, and transient sharpness.
 */

import type { SampleCategory } from '@/lib/audio/sampleCategorizer';

// ---------------------------------------------------------------------------
// Types
// ---------------------------------------------------------------------------

export interface SpectralFingerprint {
  /** Energy in 7 frequency bands (0-1 normalized). */
  subBass: number;   // 20-60 Hz
  bass: number;      // 60-250 Hz
  lowMid: number;    // 250-500 Hz
  mid: number;       // 500-2 kHz
  highMid: number;   // 2-4 kHz
  presence: number;  // 4-8 kHz
  air: number;       // 8-20 kHz
  /** Peak frequency in Hz. */
  peakFrequency: number;
  /** Spectral centroid in Hz (brightness measure). */
  spectralCentroid: number;
  /** Transient sharpness 0-1 (how steep the attack is). */
  transientSharpness: number;
  /** Sample duration in seconds. */
  duration: number;
}

// ---------------------------------------------------------------------------
// FFT helpers
// ---------------------------------------------------------------------------

/** Return the next power-of-two >= n. */
function nextPow2(n: number): number {
  let v = 1;
  while (v < n) v <<= 1;
  return v;
}

/**
 * In-place radix-2 Cooley-Tukey FFT.
 * `real` and `imag` arrays must have length that is a power of two.
 */
function fft(real: Float64Array, imag: Float64Array): void {
  const n = real.length;

  // Bit-reversal permutation
  let j = 0;
  for (let i = 0; i < n - 1; i++) {
    if (i < j) {
      // Swap real
      let tmp = real[i];
      real[i] = real[j];
      real[j] = tmp;
      // Swap imag
      tmp = imag[i];
      imag[i] = imag[j];
      imag[j] = tmp;
    }
    let k = n >> 1;
    while (k <= j) {
      j -= k;
      k >>= 1;
    }
    j += k;
  }

  // Cooley-Tukey butterfly stages
  for (let size = 2; size <= n; size *= 2) {
    const halfSize = size >> 1;
    const step = (2 * Math.PI) / size;
    for (let i = 0; i < n; i += size) {
      for (let k = 0; k < halfSize; k++) {
        const angle = -step * k;
        const twiddleReal = Math.cos(angle);
        const twiddleImag = Math.sin(angle);

        const evenIdx = i + k;
        const oddIdx = i + k + halfSize;

        const tReal = twiddleReal * real[oddIdx] - twiddleImag * imag[oddIdx];
        const tImag = twiddleReal * imag[oddIdx] + twiddleImag * real[oddIdx];

        real[oddIdx] = real[evenIdx] - tReal;
        imag[oddIdx] = imag[evenIdx] - tImag;
        real[evenIdx] = real[evenIdx] + tReal;
        imag[evenIdx] = imag[evenIdx] + tImag;
      }
    }
  }
}

// ---------------------------------------------------------------------------
// Analysis
// ---------------------------------------------------------------------------

/** FFT size used for spectral analysis. */
const FFT_SIZE = 4096;

/**
 * Analyse an AudioBuffer and return its SpectralFingerprint.
 *
 * Steps:
 * 1. Mix down to mono
 * 2. Apply Hann window to the first FFT_SIZE samples
 * 3. Compute FFT
 * 4. Derive magnitude spectrum, band energies, peak frequency,
 *    spectral centroid, and transient sharpness.
 */
export async function analyzeSpectralFingerprint(
  buffer: AudioBuffer,
): Promise<SpectralFingerprint> {
  const sampleRate = buffer.sampleRate;
  const duration = buffer.duration;

  // ---- 1. Get mono channel data ----
  const length = buffer.length;
  const mono = new Float32Array(length);

  if (buffer.numberOfChannels === 1) {
    buffer.copyFromChannel(mono, 0);
  } else {
    const left = new Float32Array(length);
    const right = new Float32Array(length);
    buffer.copyFromChannel(left, 0);
    buffer.copyFromChannel(right, 1);
    for (let i = 0; i < length; i++) {
      mono[i] = (left[i] + right[i]) * 0.5;
    }
  }

  // ---- 2. Window + zero-pad to FFT_SIZE ----
  const fftLen = nextPow2(Math.max(FFT_SIZE, 64));
  const real = new Float64Array(fftLen);
  const imag = new Float64Array(fftLen);

  const windowLen = Math.min(length, fftLen);
  // Guard: windowLen=1 would cause division by zero in (windowLen - 1)
  const hannDivisor = windowLen > 1 ? windowLen - 1 : 1;
  for (let i = 0; i < windowLen; i++) {
    // Hann window: 0.5 * (1 - cos(2*pi*i / (N-1)))
    const hann = 0.5 * (1 - Math.cos((2 * Math.PI * i) / hannDivisor));
    real[i] = mono[i] * hann;
  }
  // Remaining values are already 0 (zero-padded)

  // ---- 3. Compute FFT ----
  fft(real, imag);

  // ---- 4. Magnitude spectrum (only first half — positive frequencies) ----
  const halfLen = fftLen >> 1;
  const magnitudes = new Float64Array(halfLen);
  for (let i = 0; i < halfLen; i++) {
    magnitudes[i] = Math.sqrt(real[i] * real[i] + imag[i] * imag[i]);
  }

  // Frequency resolution: each bin corresponds to sampleRate / fftLen Hz
  const binHz = sampleRate / fftLen;

  // ---- 5. Band energy accumulation ----
  // Band boundaries in Hz
  const bands: { name: string; lo: number; hi: number }[] = [
    { name: 'subBass',  lo: 20,   hi: 60 },
    { name: 'bass',     lo: 60,   hi: 250 },
    { name: 'lowMid',   lo: 250,  hi: 500 },
    { name: 'mid',      lo: 500,  hi: 2000 },
    { name: 'highMid',  lo: 2000, hi: 4000 },
    { name: 'presence', lo: 4000, hi: 8000 },
    { name: 'air',      lo: 8000, hi: 20000 },
  ];

  const bandEnergy: number[] = new Array(bands.length).fill(0);

  for (let i = 0; i < halfLen; i++) {
    const freq = i * binHz;
    const mag = magnitudes[i];
    for (let b = 0; b < bands.length; b++) {
      if (freq >= bands[b].lo && freq < bands[b].hi) {
        bandEnergy[b] += mag * mag; // energy = magnitude^2
        break;
      }
    }
  }

  // Normalize band energies to 0-1 (relative to the max band)
  let maxEnergy = 0;
  for (let b = 0; b < bandEnergy.length; b++) {
    if (bandEnergy[b] > maxEnergy) maxEnergy = bandEnergy[b];
  }
  if (maxEnergy > 0) {
    for (let b = 0; b < bandEnergy.length; b++) {
      bandEnergy[b] = bandEnergy[b] / maxEnergy;
    }
  }

  // ---- 6. Peak frequency ----
  let peakMag = 0;
  let peakBin = 0;
  for (let i = 1; i < halfLen; i++) {
    if (magnitudes[i] > peakMag) {
      peakMag = magnitudes[i];
      peakBin = i;
    }
  }
  const peakFrequency = peakBin * binHz;

  // ---- 7. Spectral centroid ----
  let weightedSum = 0;
  let magSum = 0;
  for (let i = 0; i < halfLen; i++) {
    const freq = i * binHz;
    weightedSum += freq * magnitudes[i];
    magSum += magnitudes[i];
  }
  const spectralCentroid = magSum > 0 ? weightedSum / magSum : 0;

  // ---- 8. Transient sharpness ----
  // Analyse the amplitude envelope over the first 50 ms.
  // Compute in short frames and find the steepest rise.
  const attackSamples = Math.min(
    Math.floor(0.05 * sampleRate), // 50 ms worth of samples
    length,
  );
  const frameSize = Math.max(Math.floor(sampleRate * 0.001), 1); // ~1 ms frames
  const frameCount = Math.floor(attackSamples / frameSize);

  let maxSlope = 0;
  let prevRms = 0;

  for (let f = 0; f < frameCount; f++) {
    const start = f * frameSize;
    let sumSq = 0;
    for (let i = start; i < start + frameSize && i < length; i++) {
      sumSq += mono[i] * mono[i];
    }
    const rms = Math.sqrt(sumSq / frameSize);
    if (f > 0) {
      const slope = rms - prevRms;
      if (slope > maxSlope) maxSlope = slope;
    }
    prevRms = rms;
  }

  // Normalize transient sharpness to 0-1.
  // A maxSlope of ~0.3 or above is considered very sharp.
  const transientSharpness = Math.min(maxSlope / 0.3, 1);

  // ---- Build result ----
  return {
    subBass: bandEnergy[0],
    bass: bandEnergy[1],
    lowMid: bandEnergy[2],
    mid: bandEnergy[3],
    highMid: bandEnergy[4],
    presence: bandEnergy[5],
    air: bandEnergy[6],
    peakFrequency,
    spectralCentroid,
    transientSharpness,
    duration,
  };
}

// ---------------------------------------------------------------------------
// Spectral classifier
// ---------------------------------------------------------------------------

/**
 * Classify a sample into a SampleCategory based on its spectral fingerprint.
 *
 * Returns the predicted category and a confidence value (0-1).
 */
export function classifyBySpectrum(
  fingerprint: SpectralFingerprint,
): { category: SampleCategory; confidence: number } {
  const {
    subBass,
    bass,
    lowMid: _lowMid,
    mid,
    highMid,
    presence,
    air,
    spectralCentroid,
    transientSharpness,
    duration,
  } = fingerprint;

  const isSharpTransient = transientSharpness > 0.5;

  // --- Kick: dominant low end + sharp transient + low centroid ---
  if (subBass + bass > 0.7 && isSharpTransient && spectralCentroid < 200) {
    return { category: 'kick', confidence: 0.8 };
  }

  // --- Snare: high-mid snap + sharp transient + mid presence ---
  if (highMid > 0.4 && isSharpTransient && mid > 0.3) {
    return { category: 'snare', confidence: 0.75 };
  }

  // --- Closed hat: high freq + short + sharp transient ---
  if (presence + air > 0.6 && duration < 0.3 && isSharpTransient) {
    return { category: 'hat-closed', confidence: 0.8 };
  }

  // --- Open hat / cymbal: high freq + longer duration ---
  if (presence + air > 0.5 && duration > 0.3) {
    // Cymbals tend to be longer than open hats
    if (duration > 1.0) {
      return { category: 'cymbal', confidence: 0.7 };
    }
    return { category: 'hat-open', confidence: 0.7 };
  }

  // --- Bass: low end dominant without sharp transient ---
  if (subBass + bass > 0.5 && !isSharpTransient) {
    return { category: 'bass', confidence: 0.65 };
  }

  // --- Perc: mid + high-mid, moderate duration ---
  if (mid + highMid > 0.5 && duration > 0.05 && duration < 2.0) {
    return { category: 'perc', confidence: 0.6 };
  }

  // --- Unknown ---
  return { category: 'unknown', confidence: 0.3 };
}
