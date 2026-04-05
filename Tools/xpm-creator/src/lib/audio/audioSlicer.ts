import { getAudioContext } from './audioContext';
import type { ChopRegion } from '@/types';

/**
 * Find the nearest zero-crossing point to the given sample position.
 * A zero-crossing is where the waveform crosses from positive to negative or vice versa.
 * This prevents audible clicks at chop boundaries.
 *
 * @param data - The audio channel data
 * @param position - The target sample position
 * @param searchRadius - Number of samples to search in each direction (default 256)
 * @returns The nearest zero-crossing sample position
 */
export function findNearestZeroCrossing(
  data: Float32Array,
  position: number,
  searchRadius: number = 256
): number {
  const start = Math.max(0, position - searchRadius);
  const end = Math.min(data.length - 1, position + searchRadius);

  let bestPos = position;
  let bestDistance = searchRadius + 1;

  for (let i = start; i < end; i++) {
    // Check for sign change between consecutive samples (zero-crossing)
    if ((data[i] >= 0 && data[i + 1] < 0) || (data[i] < 0 && data[i + 1] >= 0)) {
      // Pick the sample closer to zero
      const crossPos = Math.abs(data[i]) <= Math.abs(data[i + 1]) ? i : i + 1;
      const distance = Math.abs(crossPos - position);
      if (distance < bestDistance) {
        bestDistance = distance;
        bestPos = crossPos;
      }
    }
  }

  return bestPos;
}

/**
 * Slice an AudioBuffer between startTime and endTime (seconds).
 * Applies a short micro-fade (32 samples ≈ 0.7ms @ 44.1kHz) at both
 * boundaries to prevent audible clicks from hard sample cuts.
 */
export function sliceAudioBuffer(
  source: AudioBuffer,
  startTime: number,
  endTime: number
): AudioBuffer {
  const ctx = getAudioContext();
  const sampleRate = source.sampleRate;
  const startSample = Math.max(0, Math.floor(startTime * sampleRate));
  const endSample = Math.min(source.length, Math.floor(endTime * sampleRate));
  const length = endSample - startSample;

  if (length <= 0) {
    throw new Error(
      `Invalid slice: start=${startTime.toFixed(3)}s end=${endTime.toFixed(3)}s produces ${length} samples`
    );
  }

  const sliced = ctx.createBuffer(
    source.numberOfChannels,
    length,
    sampleRate
  );

  // Micro-fade length: ~0.7ms, rate-adaptive (~31 samples at 44.1kHz,
  // ~34 at 48kHz, ~67 at 96kHz) — enough to eliminate clicks without
  // audibly affecting transient attacks.
  const fadeSamples = Math.min(Math.floor(0.0007 * source.sampleRate), Math.floor(length / 4));

  for (let ch = 0; ch < source.numberOfChannels; ch++) {
    const sourceData = source.getChannelData(ch);
    const destData = sliced.getChannelData(ch);
    for (let i = 0; i < length; i++) {
      let sample = sourceData[startSample + i] ?? 0;

      // Guard: skip fade math when fadeSamples is 0 (tiny slices)
      if (fadeSamples > 0) {
        // Fade in at start
        if (i < fadeSamples) {
          sample *= i / fadeSamples;
        }
        // Fade out at end — last sample reaches zero, first fade sample stays at full gain.
        // Use (fadeSamples - 1) as divisor to avoid off-by-one where
        // the first fade-out sample starts at (N-1)/N instead of 1.0.
        if (i >= length - fadeSamples) {
          const posInFade = i - (length - fadeSamples);
          sample *= fadeSamples > 1 ? 1 - posInFade / (fadeSamples - 1) : 0;
        }
      }

      destData[i] = sample;
    }
  }

  return sliced;
}

export function autoDetectTransients(
  buffer: AudioBuffer,
  threshold: number = 0.3,
  minGapMs: number = 100
): ChopRegion[] {
  const data = buffer.getChannelData(0);
  const sampleRate = buffer.sampleRate;
  const minGapSamples = Math.floor((minGapMs / 1000) * sampleRate);

  const energy: number[] = [];
  const windowSize = 512;
  for (let i = 0; i < data.length; i += windowSize) {
    const count = Math.min(windowSize, data.length - i);
    let sum = 0;
    for (let j = 0; j < count; j++) {
      sum += data[i + j] * data[i + j];
    }
    energy.push(Math.sqrt(sum / count));
  }

  // Compute energy deltas (first derivative) and threshold against max delta,
  // not max absolute energy. Comparing deltas to absolute energy would require
  // near-instantaneous jumps from silence to peak, missing most real transients.
  const diffs: number[] = [];
  for (let i = 1; i < energy.length; i++) {
    diffs.push(Math.max(0, energy[i] - energy[i - 1]));
  }
  // Find max diff with a loop instead of Math.max(...diffs) — spread syntax
  // throws RangeError on arrays > ~65K elements (long audio at 44.1kHz)
  let maxDiff = 0.0001; // floor avoids division by zero
  for (let i = 0; i < diffs.length; i++) {
    if (diffs[i] > maxDiff) maxDiff = diffs[i];
  }
  const thresholdValue = maxDiff * threshold;

  const onsets: number[] = [];
  let lastOnset = -minGapSamples;

  for (let i = 1; i < energy.length; i++) {
    const diff = energy[i] - energy[i - 1];
    const samplePos = i * windowSize;
    if (diff > thresholdValue && samplePos - lastOnset > minGapSamples) {
      // Snap to nearest zero-crossing for click-free chop boundaries
      const snappedPos = findNearestZeroCrossing(data, samplePos);
      onsets.push(snappedPos);
      lastOnset = snappedPos;
    }
  }

  const regions: ChopRegion[] = [];
  for (let i = 0; i < onsets.length; i++) {
    const start = onsets[i] / sampleRate;
    const end = i < onsets.length - 1
      ? onsets[i + 1] / sampleRate
      : buffer.duration;

    regions.push({
      id: `chop-${i}`,
      start,
      end,
      name: `Chop ${i + 1}`,
    });
  }

  return regions;
}

export function normalizeBuffer(buffer: AudioBuffer, targetPeak: number = 0.95): AudioBuffer {
  const ctx = getAudioContext();
  const normalized = ctx.createBuffer(
    buffer.numberOfChannels,
    buffer.length,
    buffer.sampleRate
  );

  let maxPeak = 0;
  for (let ch = 0; ch < buffer.numberOfChannels; ch++) {
    const data = buffer.getChannelData(ch);
    for (let i = 0; i < data.length; i++) {
      const abs = Math.abs(data[i]);
      if (abs > maxPeak) maxPeak = abs;
    }
  }

  // Floor at 1e-4 (≈ -80dB) to prevent astronomical gain on near-silent buffers
  const NORMALIZE_MIN_PEAK = 1e-4;
  const gain = maxPeak > NORMALIZE_MIN_PEAK ? targetPeak / maxPeak : 1;

  for (let ch = 0; ch < buffer.numberOfChannels; ch++) {
    const src = buffer.getChannelData(ch);
    const dest = normalized.getChannelData(ch);
    for (let i = 0; i < src.length; i++) {
      dest[i] = src[i] * gain;
    }
  }

  return normalized;
}
