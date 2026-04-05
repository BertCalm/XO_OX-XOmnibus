import { getAudioContext } from '@/lib/audio/audioContext';
import { applyHaasEffect } from '@/lib/audio/spaceFold';

// ---------------------------------------------------------------------------
// Types
// ---------------------------------------------------------------------------

export type MasteringEra = '1993' | '2000' | 'modern';

export interface EraMasteringConfig {
  era: MasteringEra;
  /** 0-1 intensity of the era effect */
  intensity: number;
}

// ---------------------------------------------------------------------------
// Era display info
// ---------------------------------------------------------------------------

export const ERA_INFO: Record<MasteringEra, { label: string; icon: string; description: string }> = {
  '1993': {
    label: '93 Grit',
    icon: '\u{1F4FC}',
    description: '12-bit quantization + 15kHz LPF + subtle saturation',
  },
  '2000': {
    label: '2K Sheen',
    icon: '\u{1F4BF}',
    description: 'Haas stereo depth + 10kHz air shelf boost',
  },
  modern: {
    label: 'Modern',
    icon: '\u{1F52E}',
    description: 'Clean, full-range with subtle limiting',
  },
};

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

/**
 * Clone an AudioBuffer into a new buffer with the same dimensions.
 * Copies all channel data so the original is never mutated.
 */
function cloneBuffer(source: AudioBuffer): AudioBuffer {
  const ctx = getAudioContext();
  const clone = ctx.createBuffer(
    source.numberOfChannels,
    source.length,
    source.sampleRate,
  );
  for (let ch = 0; ch < source.numberOfChannels; ch++) {
    clone.getChannelData(ch).set(source.getChannelData(ch));
  }
  return clone;
}

// ---------------------------------------------------------------------------
// 1993 — 12-bit quantization + 15 kHz LPF + subtle saturation
// ---------------------------------------------------------------------------

function apply1993(source: AudioBuffer, intensity: number): AudioBuffer {
  const output = cloneBuffer(source);
  const sampleRate = source.sampleRate;
  const alpha = Math.exp(-2 * Math.PI * 15000 / sampleRate);

  for (let ch = 0; ch < output.numberOfChannels; ch++) {
    const data = output.getChannelData(ch);
    let prev = 0;

    for (let i = 0; i < data.length; i++) {
      let sample = data[i];

      // --- 12-bit quantization (4096 levels) ---
      const quantized = Math.round(sample * 2048) / 2048;
      sample = sample * (1 - intensity) + quantized * intensity;

      // --- 15 kHz one-pole lowpass ---
      const filtered = prev * alpha + sample * (1 - alpha);
      prev = filtered;
      if (Math.abs(prev) < 1e-15) prev = 0; // flush denormals
      sample = sample * (1 - intensity) + filtered * intensity;

      // --- Subtle tanh saturation ---
      const drive = 1 + intensity * 1.5;
      const tanhDrive = Math.tanh(drive);
      sample = tanhDrive === 0 ? sample : Math.tanh(sample * drive) / tanhDrive;

      data[i] = sample;
    }
  }

  return output;
}

// ---------------------------------------------------------------------------
// 2000 — Haas stereo depth + 10 kHz air shelf boost
// ---------------------------------------------------------------------------

async function apply2000(source: AudioBuffer, intensity: number): Promise<AudioBuffer> {
  // --- Step 1: Haas effect for stereo widening ---
  const delayMs = 0.8 * intensity + 0.3;
  const width = intensity * 0.6;
  const haased = applyHaasEffect(source, delayMs, width);

  // --- Step 2: 10 kHz high-shelf air boost ---
  const output = cloneBuffer(haased);
  const sampleRate = output.sampleRate;
  const alpha = Math.exp(-2 * Math.PI * 10000 / sampleRate);
  const boostGain = Math.pow(10, (intensity * 3) / 20); // dB to linear

  for (let ch = 0; ch < output.numberOfChannels; ch++) {
    const data = output.getChannelData(ch);
    let prevLP = 0;

    for (let i = 0; i < data.length; i++) {
      const sample = data[i];

      // One-pole lowpass to extract everything below ~10 kHz
      const lp = prevLP * alpha + sample * (1 - alpha);
      prevLP = lp;
      if (Math.abs(prevLP) < 1e-15) prevLP = 0; // flush denormals

      // High-frequency content is the difference
      const hf = sample - lp;

      // Boost the high-frequency content and add back
      data[i] = lp + hf * boostGain;
    }
  }

  return output;
}

// ---------------------------------------------------------------------------
// Modern — Subtle soft-knee limiting + very light saturation
// ---------------------------------------------------------------------------

function applyModern(source: AudioBuffer, intensity: number): AudioBuffer {
  const output = cloneBuffer(source);
  const threshold = 0.95 - intensity * 0.1;

  for (let ch = 0; ch < output.numberOfChannels; ch++) {
    const data = output.getChannelData(ch);

    for (let i = 0; i < data.length; i++) {
      let sample = data[i];

      // --- Soft-knee limiting ---
      const abs = Math.abs(sample);
      if (abs > threshold) {
        const sign = sample >= 0 ? 1 : -1;
        const excess = abs - threshold;
        const knee = 1 - threshold;
        const compressed = threshold + excess / (1 + excess / knee);
        sample = sign * compressed;
      }

      // --- Very subtle saturation (intensity * 0.3) ---
      const drive = 1 + intensity * 0.3 * 1.5;
      const tanhDrive = Math.tanh(drive);
      sample = tanhDrive === 0 ? sample : Math.tanh(sample * drive) / tanhDrive;

      data[i] = sample;
    }
  }

  return output;
}

// ---------------------------------------------------------------------------
// Main entry point
// ---------------------------------------------------------------------------

/**
 * Apply era-specific mastering to an AudioBuffer.
 * Processes through sample-level DSP for non-realtime rendering.
 */
export async function applyEraMastering(
  source: AudioBuffer,
  config: EraMasteringConfig,
): Promise<AudioBuffer> {
  const intensity = Math.max(0, Math.min(1, config.intensity));

  // Bypass when intensity is effectively zero
  if (intensity < 0.001) {
    return cloneBuffer(source);
  }

  switch (config.era) {
    case '1993':
      return apply1993(source, intensity);
    case '2000':
      return apply2000(source, intensity);
    case 'modern':
      return applyModern(source, intensity);
    default: {
      // Exhaustive check — should never reach here
      const _exhaustive: never = config.era;
      throw new Error(`Unknown mastering era: ${_exhaustive}`);
    }
  }
}
