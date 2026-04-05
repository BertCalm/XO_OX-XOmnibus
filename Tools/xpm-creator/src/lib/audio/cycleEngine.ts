import { getAudioContext } from './audioContext';
import { applyFilter, applyFadeIn, adjustGain } from './chopProcessors';
import { decodeArrayBuffer, generateWaveformPeaks } from './audioUtils';
import { encodeWav } from './wavEncoder';
import { v4 as uuid } from 'uuid';
import type { AudioSample } from '@/types';

// ---------------------------------------------------------------------------
// Types
// ---------------------------------------------------------------------------

export interface CycleEngineConfig {
  /** How much variation between round-robin pairs (0-1) */
  variationAmount: number;
  /** Processing applied to create velocity tier differences */
  velocityProcessing: 'filter-sweep' | 'volume-only' | 'full-dynamics';
  /** Additional humanization on each layer */
  humanize: boolean;
}

export const CYCLE_ENGINE_DEFAULTS: CycleEngineConfig = {
  variationAmount: 0.4,
  velocityProcessing: 'full-dynamics',
  humanize: true,
};

export interface CycleLayerResult {
  layerIndex: number; // 0-7
  sample: AudioSample;
  velStart: number;
  velEnd: number;
  volume: number;
  pan: number;
  tuneFine: number;
  pitchRandom: number;
  volumeRandom: number;
  probability: number;
}

// ---------------------------------------------------------------------------
// Velocity tier definitions for the 4-tier x 2-RR layout
// ---------------------------------------------------------------------------

export const VELOCITY_TIERS = [
  { name: 'Ghost', range: [0, 31] as const, volumeScale: 0.45, filterScale: 0.5 },
  { name: 'Soft', range: [32, 63] as const, volumeScale: 0.65, filterScale: 0.7 },
  { name: 'Medium', range: [64, 95] as const, volumeScale: 0.85, filterScale: 0.9 },
  { name: 'Hard', range: [96, 127] as const, volumeScale: 1.0, filterScale: 1.0 },
] as const;

// Filter cutoffs per tier for filter-sweep mode
const TIER_CUTOFFS: Record<string, number> = {
  Ghost: 3000,
  Soft: 6000,
  Medium: 12000,
  Hard: 20000,
};

// ---------------------------------------------------------------------------
// Internal processing helpers
// ---------------------------------------------------------------------------

/**
 * Apply subtle pitch shifting by resampling the buffer data.
 * Positive cents = higher pitch, negative cents = lower pitch.
 * This works by changing the effective playback rate and resampling.
 */
function applyMicroPitch(buffer: AudioBuffer, cents: number): AudioBuffer {
  if (cents === 0) return buffer;

  const ctx = getAudioContext();
  const ratio = Math.pow(2, cents / 1200);
  const newLength = Math.round(buffer.length / ratio);

  if (newLength <= 0 || newLength === buffer.length) return buffer;

  const result = ctx.createBuffer(
    buffer.numberOfChannels,
    newLength,
    buffer.sampleRate
  );

  for (let ch = 0; ch < buffer.numberOfChannels; ch++) {
    const src = buffer.getChannelData(ch);
    const dest = result.getChannelData(ch);
    for (let i = 0; i < newLength; i++) {
      const srcIndex = i * ratio;
      const srcFloor = Math.floor(srcIndex);
      const frac = srcIndex - srcFloor;
      const s0 = srcFloor < src.length ? src[srcFloor] : 0;
      const s1 = srcFloor + 1 < src.length ? src[srcFloor + 1] : 0;
      dest[i] = s0 + frac * (s1 - s0); // Linear interpolation
    }
  }

  return result;
}

/**
 * Apply tanh soft-clip saturation at a given threshold.
 */
function applySaturation(buffer: AudioBuffer, threshold: number): AudioBuffer {
  const ctx = getAudioContext();
  const result = ctx.createBuffer(
    buffer.numberOfChannels,
    buffer.length,
    buffer.sampleRate
  );

  for (let ch = 0; ch < buffer.numberOfChannels; ch++) {
    const src = buffer.getChannelData(ch);
    const dest = result.getChannelData(ch);
    for (let i = 0; i < buffer.length; i++) {
      // Drive the signal slightly and apply tanh soft-clip
      const driven = src[i] / threshold;
      dest[i] = Math.tanh(driven) * threshold;
    }
  }

  return result;
}

/**
 * Offset the sample start by a number of samples (trims the beginning).
 */
function applySampleOffset(buffer: AudioBuffer, offsetSamples: number): AudioBuffer {
  if (offsetSamples <= 0 || offsetSamples >= buffer.length) return buffer;

  const ctx = getAudioContext();
  const newLength = buffer.length - offsetSamples;
  const result = ctx.createBuffer(
    buffer.numberOfChannels,
    newLength,
    buffer.sampleRate
  );

  for (let ch = 0; ch < buffer.numberOfChannels; ch++) {
    const src = buffer.getChannelData(ch);
    const dest = result.getChannelData(ch);
    for (let i = 0; i < newLength; i++) {
      dest[i] = src[i + offsetSamples];
    }
  }

  return result;
}

// ---------------------------------------------------------------------------
// Tier processing strategies
// ---------------------------------------------------------------------------

async function processFilterSweep(
  audioBuffer: AudioBuffer,
  tierName: string,
  volumeScale: number
): Promise<AudioBuffer> {
  const cutoff = TIER_CUTOFFS[tierName] ?? 20000;

  // Apply lowpass filter (skip if full bandwidth)
  let processed = audioBuffer;
  if (cutoff < 20000) {
    processed = await applyFilter(processed, 'lowpass', cutoff, 0.7);
  }

  // Scale volume
  if (volumeScale < 1.0) {
    processed = adjustGain(processed, volumeScale);
  }

  return processed;
}

function processVolumeOnly(
  audioBuffer: AudioBuffer,
  volumeScale: number
): AudioBuffer {
  if (volumeScale < 1.0) {
    return adjustGain(audioBuffer, volumeScale);
  }
  return audioBuffer;
}

async function processFullDynamics(
  audioBuffer: AudioBuffer,
  tierName: string,
  volumeScale: number
): Promise<AudioBuffer> {
  let processed = audioBuffer;

  switch (tierName) {
    case 'Ghost': {
      // Filter + slight pitch down (-2 cents) + slower attack (fade in first 5ms)
      const cutoff = TIER_CUTOFFS.Ghost;
      processed = await applyFilter(processed, 'lowpass', cutoff, 0.7);
      processed = applyMicroPitch(processed, -2);
      processed = applyFadeIn(processed, 0.005); // 5ms fade-in
      break;
    }
    case 'Soft': {
      // Filter + slight pitch down (-1 cent) + slight fade (2ms)
      const cutoff = TIER_CUTOFFS.Soft;
      processed = await applyFilter(processed, 'lowpass', cutoff, 0.7);
      processed = applyMicroPitch(processed, -1);
      processed = applyFadeIn(processed, 0.002); // 2ms fade-in
      break;
    }
    case 'Medium': {
      // No pitch shift, no fade, slight filter
      const cutoff = TIER_CUTOFFS.Medium;
      processed = await applyFilter(processed, 'lowpass', cutoff, 0.7);
      break;
    }
    case 'Hard': {
      // Slight pitch up (+1 cent), no filter, add subtle saturation
      processed = applyMicroPitch(processed, 1);
      processed = applySaturation(processed, 0.95);
      break;
    }
  }

  // Scale volume
  if (volumeScale < 1.0) {
    processed = adjustGain(processed, volumeScale);
  }

  return processed;
}

// ---------------------------------------------------------------------------
// Round-robin micro-variation
// ---------------------------------------------------------------------------

interface RRVariation {
  detuneCents: number;
  pan: number;
  sampleStartOffset: number;
  volumeAdjust: number;
}

function computeRRVariation(
  isA: boolean,
  variationAmount: number
): RRVariation {
  // Deterministic but different for A vs B
  // Micro-detune: A gets +1-3 cents, B gets -1-3 cents (scaled by variationAmount)
  const detuneBase = 1 + 2 * variationAmount; // 1 to 3 cents
  const detuneCents = isA ? detuneBase : -detuneBase;

  // Micro-pan: A gets 0.48, B gets 0.52 (scaled by variationAmount)
  const panOffset = 0.02 * variationAmount;
  const pan = isA ? 0.5 - panOffset : 0.5 + panOffset;

  // Sample start offset: A starts at 0, B starts at a small random offset
  // Using a pseudo-random but consistent offset scaled by variationAmount
  const sampleStartOffset = isA ? 0 : Math.round(2 + 13 * variationAmount);

  // Volume micro-variance: +/- 0.02 scaled by variationAmount
  const volumeAdjust = isA
    ? 1.0 + 0.02 * variationAmount
    : 1.0 - 0.02 * variationAmount;

  return { detuneCents, pan, sampleStartOffset, volumeAdjust };
}

function applyRRVariation(
  buffer: AudioBuffer,
  variation: RRVariation
): AudioBuffer {
  let processed = buffer;

  // Apply sample start offset (only for RR-B which has offset > 0)
  if (variation.sampleStartOffset > 0) {
    processed = applySampleOffset(processed, variation.sampleStartOffset);
  }

  // Apply micro-detune
  if (variation.detuneCents !== 0) {
    processed = applyMicroPitch(processed, variation.detuneCents);
  }

  // Apply micro volume adjustment
  if (Math.abs(variation.volumeAdjust - 1.0) > 0.001) {
    processed = adjustGain(processed, variation.volumeAdjust);
  }

  return processed;
}

// ---------------------------------------------------------------------------
// Main generator
// ---------------------------------------------------------------------------

/**
 * Generate all 8 layers from a single source sample.
 * Returns 8 CycleLayerResult entries ready for pad assignment.
 *
 * Layout:
 *   Layer 1-2 = Ghost (vel 0-31), RR-A and RR-B
 *   Layer 3-4 = Soft  (vel 32-63), RR-A and RR-B
 *   Layer 5-6 = Medium (vel 64-95), RR-A and RR-B
 *   Layer 7-8 = Hard  (vel 96-127), RR-A and RR-B
 */
export async function generate8LayerCycle(
  sourceBuffer: ArrayBuffer,
  sourceName: string,
  config?: Partial<CycleEngineConfig>,
  onProgress?: (pct: number) => void
): Promise<CycleLayerResult[]> {
  const cfg: CycleEngineConfig = { ...CYCLE_ENGINE_DEFAULTS, ...config };
  const results: CycleLayerResult[] = [];

  // 1. Decode the source ArrayBuffer to AudioBuffer
  const audioBuffer = await decodeArrayBuffer(sourceBuffer);
  onProgress?.(1);

  // Clean base name for generated files
  const baseName = sourceName
    .replace(/\.(wav|mp3|flac|m4a|aac|ogg)$/i, '')
    .substring(0, 20);

  // 2. For each tier (4 tiers)
  for (let tierIdx = 0; tierIdx < VELOCITY_TIERS.length; tierIdx++) {
    const tier = VELOCITY_TIERS[tierIdx];

    // Apply tier-appropriate processing
    let tierBuffer: AudioBuffer;
    switch (cfg.velocityProcessing) {
      case 'filter-sweep':
        tierBuffer = await processFilterSweep(
          audioBuffer,
          tier.name,
          tier.volumeScale
        );
        break;
      case 'volume-only':
        tierBuffer = processVolumeOnly(audioBuffer, tier.volumeScale);
        break;
      case 'full-dynamics':
      default:
        tierBuffer = await processFullDynamics(
          audioBuffer,
          tier.name,
          tier.volumeScale
        );
        break;
    }

    // 3. For each RR variation (2 per tier)
    for (let rrIdx = 0; rrIdx < 2; rrIdx++) {
      const isA = rrIdx === 0;
      const layerIndex = tierIdx * 2 + rrIdx; // 0-7

      // Compute and apply micro-variations
      const rrVariation = computeRRVariation(isA, cfg.variationAmount);
      const variedBuffer = applyRRVariation(tierBuffer, rrVariation);

      // Encode to WAV
      const wavBuffer = encodeWav(variedBuffer, 16);

      // Generate waveform peaks for visualization
      const peaks = generateWaveformPeaks(variedBuffer);

      // Build file name: BaseName_TierAbbrev_RR-A/B.WAV
      const rrLabel = isA ? 'RR-A' : 'RR-B';
      const tierAbbrev = tier.name.substring(0, 3).toUpperCase();
      const fileName = `${baseName}_${tierAbbrev}_${rrLabel}.WAV`;
      const sampleName = `${baseName}_${tierAbbrev}_${rrLabel}`;

      // Create AudioSample
      const sample: AudioSample = {
        id: uuid(),
        name: sampleName,
        fileName,
        duration: variedBuffer.duration,
        sampleRate: variedBuffer.sampleRate,
        channels: variedBuffer.numberOfChannels,
        bitDepth: 16,
        buffer: wavBuffer,
        waveformPeaks: peaks,
        rootNote: 60,
        createdAt: Date.now(),
      };

      // Compute humanization values
      let pitchRandom = 0;
      let volumeRandom = 0;
      let probability = 100;

      if (cfg.humanize) {
        // Slight pitch randomization (0.01-0.02)
        pitchRandom = 0.01 + 0.01 * cfg.variationAmount;
        // Slight volume randomization (0.02-0.04)
        volumeRandom = 0.02 + 0.02 * cfg.variationAmount;
        // Slight trigger variation on RR layers (95-100)
        probability = Math.round(95 + 5 * (1 - cfg.variationAmount));
      }

      results.push({
        layerIndex,
        sample,
        velStart: tier.range[0],
        velEnd: tier.range[1],
        volume: tier.volumeScale * rrVariation.volumeAdjust,
        pan: rrVariation.pan,
        tuneFine: Math.round(rrVariation.detuneCents),
        pitchRandom,
        volumeRandom,
        probability,
      });

      // Report progress: 1 initial + 8 layers = progress from 1 to 9
      onProgress?.(2 + layerIndex);
    }
  }

  return results;
}
