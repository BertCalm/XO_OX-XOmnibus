import { getAudioContext } from './audioContext';
import { adjustGain, applyFilter } from './chopProcessors';

export interface GhostNoteConfig {
  /** Volume reduction factor (0-1). Default 0.35 = ~65% quieter */
  volumeFactor?: number;
  /** Low-pass filter cutoff in Hz. Default 2000Hz */
  lpfCutoff?: number;
  /** Offset shift in ms (shifts the start point forward). Default 5ms */
  offsetMs?: number;
  /** Whether to slightly shorten the sample. Default true */
  trimTail?: boolean;
}

const DEFAULTS: Required<GhostNoteConfig> = {
  volumeFactor: 0.35,
  lpfCutoff: 2000,
  offsetMs: 5,
  trimTail: true,
};

/**
 * Generate a "ghost note" version of a drum sample.
 *
 * Ghost notes are softer, more muffled versions of a hit used for
 * realism in drum programming (especially snares and hi-hats).
 *
 * Processing chain:
 * 1. Reduce volume (simulates lower velocity)
 * 2. Apply low-pass filter (simulates softer hit = less high-freq content)
 * 3. Shift start point forward slightly (ghost notes have less attack)
 * 4. Optionally trim the tail shorter
 */
export async function generateGhostNote(
  buffer: AudioBuffer,
  config: GhostNoteConfig = {}
): Promise<AudioBuffer> {
  const opts = { ...DEFAULTS, ...config };
  const ctx = getAudioContext();
  const sampleRate = buffer.sampleRate;

  // 1. Shift the start point (skip the initial transient attack)
  const offsetSamples = Math.floor((opts.offsetMs / 1000) * sampleRate);
  const trimEnd = opts.trimTail
    ? Math.max(Math.floor(buffer.length * 0.75), buffer.length - Math.floor(0.1 * sampleRate))
    : buffer.length;
  const newLength = Math.max(trimEnd - offsetSamples, 1);

  const shifted = ctx.createBuffer(buffer.numberOfChannels, newLength, sampleRate);
  for (let ch = 0; ch < buffer.numberOfChannels; ch++) {
    const src = buffer.getChannelData(ch);
    const dest = shifted.getChannelData(ch);
    for (let i = 0; i < newLength; i++) {
      const srcIdx = i + offsetSamples;
      dest[i] = srcIdx < src.length ? src[srcIdx] : 0;
    }
  }

  // 2. Apply low-pass filter (remove brightness)
  const filtered = await applyFilter(shifted, 'lowpass', opts.lpfCutoff, 0.7);

  // 3. Reduce volume
  const quieter = adjustGain(filtered, opts.volumeFactor);

  // 4. Apply a tiny fade-in to smooth the shifted start (2ms)
  const fadeInSamples = Math.min(Math.floor(0.002 * sampleRate), quieter.length);
  const result = ctx.createBuffer(quieter.numberOfChannels, quieter.length, sampleRate);
  for (let ch = 0; ch < quieter.numberOfChannels; ch++) {
    const src = quieter.getChannelData(ch);
    const dest = result.getChannelData(ch);
    for (let i = 0; i < quieter.length; i++) {
      if (fadeInSamples > 0 && i < fadeInSamples) {
        dest[i] = src[i] * (i / fadeInSamples);
      } else {
        dest[i] = src[i];
      }
    }
  }

  return result;
}
