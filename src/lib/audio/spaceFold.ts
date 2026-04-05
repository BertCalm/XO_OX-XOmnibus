import { getAudioContext } from './audioContext';
import type { PadAssignment, PadLayer } from '@/types';

export interface SpaceFoldConfig {
  /** Base stereo width at minimum velocity (0 = mono, 1 = full stereo) */
  minWidth: number;
  /** Maximum stereo width at max velocity (0 = mono, 1 = full stereo) */
  maxWidth: number;
  /** Curve type for width progression */
  curve: 'linear' | 'exponential' | 'logarithmic';
}

export const SPACE_FOLD_PRESETS: {
  id: string;
  name: string;
  icon: string;
  description: string;
  config: SpaceFoldConfig;
}[] = [
  {
    id: 'subtle',
    name: 'Gentle Spread',
    icon: '\u{1F305}',
    description: 'Subtle widening on hard hits',
    config: { minWidth: 0.0, maxWidth: 0.3, curve: 'linear' },
  },
  {
    id: 'dramatic',
    name: 'Stadium',
    icon: '\u{1F3DF}\uFE0F',
    description: 'Big stereo explosion on climax',
    config: { minWidth: 0.0, maxWidth: 0.7, curve: 'exponential' },
  },
  {
    id: 'wide',
    name: 'Panoramic',
    icon: '\u{1F30C}',
    description: 'Always wide, wider on hard',
    config: { minWidth: 0.3, maxWidth: 0.9, curve: 'linear' },
  },
  {
    id: 'haas',
    name: 'Haas Depth',
    icon: '\u{1F50A}',
    description: 'Phase-based depth illusion',
    config: { minWidth: 0.1, maxWidth: 0.5, curve: 'logarithmic' },
  },
];

/**
 * Apply a curve function to a normalized [0,1] input.
 */
function applyCurve(t: number, curve: SpaceFoldConfig['curve']): number {
  const clamped = Math.max(0, Math.min(1, t));
  switch (curve) {
    case 'exponential':
      return clamped * clamped;
    case 'logarithmic':
      return clamped === 0 ? 0 : Math.log(1 + clamped * 9) / Math.log(10);
    case 'linear':
    default:
      return clamped;
  }
}

/**
 * Calculate pan offsets for velocity-split layers.
 * Returns layer updates with pan values that create stereo width.
 * Layers at lower velocity ranges get narrower pan, higher get wider.
 *
 * Pan is expressed as 0-1 where 0.5 = center (matching MPC conventions).
 */
export function calculateSpaceFoldPan(
  pad: PadAssignment,
  config: SpaceFoldConfig,
): { layerIndex: number; pan: number }[] {
  // Collect active layers with their original indices, sorted by velStart
  const activeLayers: { index: number; layer: PadLayer }[] = [];
  for (let i = 0; i < pad.layers.length; i++) {
    const layer = pad.layers[i];
    if (layer.active && layer.sampleId) {
      activeLayers.push({ index: i, layer });
    }
  }

  if (activeLayers.length === 0) return [];

  // Sort by velocity zone start (lowest first)
  activeLayers.sort((a, b) => a.layer.velStart - b.layer.velStart);

  const results: { layerIndex: number; pan: number }[] = [];

  if (activeLayers.length === 1) {
    // Single layer: apply a slight width based on minWidth (offset from center)
    const width = config.minWidth;
    const pan = 0.5 - width * 0.25; // Slight left offset for stereo interest
    results.push({ layerIndex: activeLayers[0].index, pan: Math.max(0, Math.min(1, pan)) });
    return results;
  }

  for (let i = 0; i < activeLayers.length; i++) {
    const { index, layer } = activeLayers[i];

    // Calculate velocity zone center as a normalized position in [0, 1]
    const velCenter = (layer.velStart + layer.velEnd) / 2;
    const normalizedVel = velCenter / 127;

    // Apply curve to determine width at this velocity point
    const curvedT = applyCurve(normalizedVel, config.curve);
    const width = config.minWidth + (config.maxWidth - config.minWidth) * curvedT;

    // Convert width to a pan offset from center (0.5).
    // Alternate layers left and right to create stereo spread.
    // Even-indexed layers in the sorted order go left, odd go right.
    const panOffset = width * 0.5; // max offset = 0.5 (full L or R)

    let pan: number;
    if (i % 2 === 0) {
      // Left side
      pan = 0.5 - panOffset;
    } else {
      // Right side
      pan = 0.5 + panOffset;
    }

    // Clamp pan to [0, 1]
    pan = Math.max(0, Math.min(1, pan));

    results.push({ layerIndex: index, pan });
  }

  return results;
}

/**
 * Apply Haas effect to an AudioBuffer for stereo widening.
 * Adds a tiny delay (0.3-1.5ms) to one channel relative to the other.
 * Input must be stereo (2 channels). For mono, duplicates to stereo first.
 *
 * @param audioBuffer - Source AudioBuffer (mono or stereo)
 * @param delayMs - Delay in milliseconds (0.3 - 1.5ms typical)
 * @param width - Mix between dry and widened signal (0.0 - 1.0)
 * @returns New AudioBuffer with Haas effect applied (always stereo)
 */
export function applyHaasEffect(
  audioBuffer: AudioBuffer,
  delayMs: number,
  width: number,
): AudioBuffer {
  const ctx = getAudioContext();
  const sampleRate = audioBuffer.sampleRate;
  const length = audioBuffer.length;

  // Clamp parameters
  const clampedDelay = Math.max(0.1, Math.min(3.0, delayMs));
  const clampedWidth = Math.max(0, Math.min(1, width));

  // Calculate delay in samples
  const delaySamples = Math.round((clampedDelay / 1000) * sampleRate);

  // Get or create stereo channel data
  let leftDry: Float32Array;
  let rightDry: Float32Array;

  if (audioBuffer.numberOfChannels >= 2) {
    leftDry = new Float32Array(audioBuffer.getChannelData(0));
    rightDry = new Float32Array(audioBuffer.getChannelData(1));
  } else {
    // Mono: duplicate to both channels
    const monoData = audioBuffer.getChannelData(0);
    leftDry = new Float32Array(monoData);
    rightDry = new Float32Array(monoData);
  }

  // Create the output buffer (stereo)
  const outputBuffer = ctx.createBuffer(2, length, sampleRate);
  const outputLeft = outputBuffer.getChannelData(0);
  const outputRight = outputBuffer.getChannelData(1);

  // Build the "wet" right channel: shift samples forward by delaySamples
  const rightWet = new Float32Array(length);
  for (let i = 0; i < length; i++) {
    const srcIdx = i - delaySamples;
    rightWet[i] = srcIdx >= 0 && srcIdx < length ? rightDry[srcIdx] : 0;
  }

  // Mix dry and wet signals
  // Left channel: stays dry (no delay)
  // Right channel: mix between original and delayed version
  const dryMix = 1 - clampedWidth;
  const wetMix = clampedWidth;

  for (let i = 0; i < length; i++) {
    outputLeft[i] = leftDry[i];
    outputRight[i] = rightDry[i] * dryMix + rightWet[i] * wetMix;
  }

  return outputBuffer;
}
