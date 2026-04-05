import { decodeArrayBuffer, generateWaveformPeaks } from './audioUtils';
import { encodeWavAsync } from './wavEncoder';
import { getAudioContext, createOfflineContext } from './audioContext';

export interface AirConfig {
  /** Type of air enhancement */
  mode: 'noise-burst' | 'high-shelf' | 'exciter';
  /** Frequency above which air is added (Hz) */
  airFrequency: number;
  /** Air intensity 0-1 */
  intensity: number;
  /** Duration of the air component in ms (for noise-burst mode) */
  burstDurationMs: number;
  /** Minimum velocity threshold (0-127) for the air layer to trigger */
  velocityThreshold: number;
}

export const AIR_DEFAULTS: AirConfig = {
  mode: 'high-shelf',
  airFrequency: 8000,
  intensity: 0.3,
  burstDurationMs: 80,
  velocityThreshold: 90,
};

export const AIR_PRESETS: {
  id: string;
  name: string;
  icon: string;
  description: string;
  config: Partial<AirConfig>;
}[] = [
  {
    id: 'silk',
    name: 'Silk Air',
    icon: '\u{1F32C}\uFE0F',
    description: 'Gentle high-shelf shimmer',
    config: { mode: 'high-shelf', airFrequency: 10000, intensity: 0.2 },
  },
  {
    id: 'crisp',
    name: 'Crisp Hit',
    icon: '\u2728',
    description: 'Noise burst on hard hits',
    config: { mode: 'noise-burst', airFrequency: 8000, intensity: 0.35, burstDurationMs: 60 },
  },
  {
    id: 'presence',
    name: 'Presence Boost',
    icon: '\u{1F50A}',
    description: 'Mid-air presence exciter',
    config: { mode: 'exciter', airFrequency: 5000, intensity: 0.4 },
  },
  {
    id: 'vintage',
    name: 'Tape Hiss',
    icon: '\u{1F4FC}',
    description: 'Colored noise texture',
    config: { mode: 'noise-burst', airFrequency: 4000, intensity: 0.15, burstDurationMs: 150 },
  },
];

/**
 * Generate a high-shelf boosted version of the sample.
 * Applies an offline BiquadFilter (highshelf) to boost frequencies above airFrequency.
 */
export async function generateHighShelfAir(
  sourceBuffer: AudioBuffer,
  config: AirConfig
): Promise<AudioBuffer> {
  const offlineCtx = createOfflineContext(
    sourceBuffer.numberOfChannels,
    sourceBuffer.length,
    sourceBuffer.sampleRate
  );

  const source = offlineCtx.createBufferSource();
  source.buffer = sourceBuffer;

  const filter = offlineCtx.createBiquadFilter();
  filter.type = 'highshelf';
  filter.frequency.value = config.airFrequency;
  filter.gain.value = config.intensity * 20; // intensity 0-1 maps to 0-20 dB boost

  source.connect(filter);
  filter.connect(offlineCtx.destination);
  source.start(0);

  return offlineCtx.startRendering();
}

/**
 * Generate a shaped noise burst that matches the source sample's duration.
 * The noise is filtered through a highpass at airFrequency and shaped with
 * an exponential decay envelope.
 */
export function generateNoiseBurst(
  sampleRate: number,
  channels: number,
  config: AirConfig
): AudioBuffer {
  const ctx = getAudioContext();
  // Guard: burstDurationMs ≤ 0 → minimum 1 sample to avoid createBuffer(0) crash
  const burstLength = Math.max(1, Math.floor((config.burstDurationMs / 1000) * sampleRate));
  const buffer = ctx.createBuffer(channels, burstLength, sampleRate);

  for (let ch = 0; ch < channels; ch++) {
    const data = buffer.getChannelData(ch);

    // Fill with white noise
    for (let i = 0; i < burstLength; i++) {
      data[i] = Math.random() * 2 - 1;
    }

    // Apply first-order highpass filter (matched-Z, pole derived from airFrequency).
    // a = exp(-2π * fc / sr). Filter equation: y = 0.5*(1+a)*(x - prevX) + a*prevY
    const fc = config.airFrequency ?? 8000;
    const a = Math.exp(-2 * Math.PI * fc / sampleRate);
    let prevX = 0;
    let prevY = 0;
    for (let i = 0; i < burstLength; i++) {
      const x = data[i];
      const y = 0.5 * (1 + a) * (x - prevX) + a * prevY;
      data[i] = y;
      prevX = x;
      prevY = y;
    }

    // Apply exponential decay envelope scaled by intensity
    const duration = burstLength / sampleRate;
    for (let i = 0; i < burstLength; i++) {
      const t = i / sampleRate;
      const gain = Math.exp(-3 * t / duration) * config.intensity;
      data[i] = Math.max(-1, Math.min(1, data[i] * gain));
    }
  }

  return buffer;
}

/**
 * Generate an "excited" version -- phase-inverted high-frequency content
 * mixed back to create harmonic enhancement.
 */
export async function generateExciterAir(
  sourceBuffer: AudioBuffer,
  config: AirConfig
): Promise<AudioBuffer> {
  const numChannels = sourceBuffer.numberOfChannels;
  const length = sourceBuffer.length;
  const sampleRate = sourceBuffer.sampleRate;

  // Step 1: Isolate high-frequency content via highpass filter
  const offlineCtx = createOfflineContext(numChannels, length, sampleRate);
  const source = offlineCtx.createBufferSource();
  source.buffer = sourceBuffer;

  const highpass = offlineCtx.createBiquadFilter();
  highpass.type = 'highpass';
  highpass.frequency.value = config.airFrequency;
  highpass.Q.value = 0.7;

  source.connect(highpass);
  highpass.connect(offlineCtx.destination);
  source.start(0);

  const highContent = await offlineCtx.startRendering();

  // Step 2: Apply soft clipping (tanh) to generate harmonics and mix at reduced volume
  const ctx = getAudioContext();
  const result = ctx.createBuffer(numChannels, length, sampleRate);
  const mixLevel = config.intensity * 0.5;

  for (let ch = 0; ch < numChannels; ch++) {
    const srcData = sourceBuffer.getChannelData(ch);
    const highData = highContent.getChannelData(ch);
    const destData = result.getChannelData(ch);

    for (let i = 0; i < length; i++) {
      // Soft-clip the high-frequency content to generate harmonics
      const excited = Math.tanh(highData[i] * 3);
      // Mix excited content with the original
      destData[i] = Math.max(-1, Math.min(1, srcData[i] + excited * mixLevel));
    }
  }

  return result;
}

/**
 * Main entry: generate air layer sample from source.
 * Returns the processed AudioBuffer ready for encoding.
 */
export async function generateAirLayer(
  sourceBuffer: ArrayBuffer,
  config?: Partial<AirConfig>
): Promise<{ airBuffer: AudioBuffer; airWav: ArrayBuffer; peaks: number[] }> {
  const mergedConfig: AirConfig = { ...AIR_DEFAULTS, ...config };

  // Decode source to AudioBuffer
  const audioBuffer = await decodeArrayBuffer(sourceBuffer);

  let airBuffer: AudioBuffer;

  switch (mergedConfig.mode) {
    case 'high-shelf':
      airBuffer = await generateHighShelfAir(audioBuffer, mergedConfig);
      break;
    case 'noise-burst':
      airBuffer = generateNoiseBurst(
        audioBuffer.sampleRate,
        audioBuffer.numberOfChannels,
        mergedConfig
      );
      break;
    case 'exciter':
      airBuffer = await generateExciterAir(audioBuffer, mergedConfig);
      break;
    default:
      airBuffer = await generateHighShelfAir(audioBuffer, mergedConfig);
  }

  // Encode the result to WAV (async — non-blocking)
  const airWav = await encodeWavAsync(airBuffer, 16);

  // Generate waveform peaks for visualization
  const peaks = generateWaveformPeaks(airBuffer);

  return { airBuffer, airWav, peaks };
}
