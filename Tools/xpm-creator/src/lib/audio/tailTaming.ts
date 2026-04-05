import { decodeArrayBuffer, generateWaveformPeaks } from './audioUtils';
import { encodeWavAsync } from './wavEncoder';
import { getAudioContext } from './audioContext';

export interface TailTameConfig {
  /** Fade duration in milliseconds (default 30ms) */
  fadeDurationMs: number;
  /** Fade curve type */
  curveType: 'logarithmic' | 'linear' | 'exponential' | 'cosine';
  /** Whether to also apply a tiny fade-in to prevent start clicks (default 2ms) */
  antiClickFadeInMs: number;
}

export const TAIL_TAME_DEFAULTS: TailTameConfig = {
  fadeDurationMs: 30,
  curveType: 'logarithmic',
  antiClickFadeInMs: 2,
};

/**
 * Compute the gain for a given fade position.
 * @param t — normalised position in the fade (0 = fade start, 1 = fade end / silence)
 */
function fadeGain(t: number, curveType: TailTameConfig['curveType']): number {
  switch (curveType) {
    case 'logarithmic':
      return Math.log(1 + (1 - t) * 9) / Math.log(10);
    case 'linear':
      return 1 - t;
    case 'exponential':
      return Math.pow(1 - t, 3);
    case 'cosine':
      return (1 + Math.cos(t * Math.PI)) / 2;
    default:
      return 1 - t;
  }
}

/**
 * Apply tail taming to an AudioBuffer.
 * Returns a new AudioBuffer with fade-out (and optional fade-in) applied.
 */
export function tameTail(
  audioBuffer: AudioBuffer,
  config?: Partial<TailTameConfig>,
): AudioBuffer {
  const cfg: TailTameConfig = { ...TAIL_TAME_DEFAULTS, ...config };

  const sampleRate = audioBuffer.sampleRate;
  const numChannels = audioBuffer.numberOfChannels;
  const length = audioBuffer.length;

  // Create a new AudioBuffer with identical parameters
  const ctx = getAudioContext();
  const output = ctx.createBuffer(numChannels, length, sampleRate);

  // Number of samples for fade-out and fade-in
  const fadeOutSamples = Math.min(
    Math.round((cfg.fadeDurationMs / 1000) * sampleRate),
    length,
  );
  const fadeInSamples = Math.min(
    Math.round((cfg.antiClickFadeInMs / 1000) * sampleRate),
    length,
  );

  for (let ch = 0; ch < numChannels; ch++) {
    const inputData = audioBuffer.getChannelData(ch);
    const outputData = output.getChannelData(ch);

    // Copy all channel data first
    outputData.set(inputData);

    // Apply fade-in (linear ramp over first antiClickFadeInMs)
    if (fadeInSamples > 0) {
      for (let i = 0; i < fadeInSamples; i++) {
        const t = i / fadeInSamples; // 0 → 1
        outputData[i] *= t;
      }
    }

    // Apply fade-out using the selected curve
    if (fadeOutSamples > 0) {
      const fadeStart = length - fadeOutSamples;
      for (let i = 0; i < fadeOutSamples; i++) {
        const t = i / fadeOutSamples; // 0 → 1
        const gain = fadeGain(t, cfg.curveType);
        outputData[fadeStart + i] *= gain;
      }
    }
  }

  return output;
}

/**
 * Process a raw WAV ArrayBuffer — decode, tame, re-encode.
 * Returns { buffer: ArrayBuffer, peaks: number[] }
 */
export async function tameTailFromBuffer(
  rawBuffer: ArrayBuffer,
  bitDepth: number,
  config?: Partial<TailTameConfig>,
): Promise<{ buffer: ArrayBuffer; peaks: number[] }> {
  const audioBuffer = await decodeArrayBuffer(rawBuffer);
  const tamed = tameTail(audioBuffer, config);
  const buffer = await encodeWavAsync(tamed, bitDepth);
  const peaks = generateWaveformPeaks(tamed);
  return { buffer, peaks };
}
