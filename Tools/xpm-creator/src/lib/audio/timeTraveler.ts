import { decodeArrayBuffer, generateWaveformPeaks } from './audioUtils';
import { encodeWavAsync } from './wavEncoder';
import { getAudioContext } from './audioContext';

export type EraMode = '1993-grit' | '2000-hifi' | '2024-crystal';

export interface EraConfig {
  mode: EraMode;
  /** How much of the era character to apply (0-1) */
  intensity: number;
}

export const ERA_PRESETS: {
  id: EraMode;
  name: string;
  icon: string;
  year: string;
  description: string;
  details: string[];
}[] = [
  {
    id: '1993-grit',
    name: 'SP-1200 Grit',
    icon: '\u{1F4DF}',
    year: '1993',
    description: 'Gritty 12-bit warmth \u2014 Pete Rock, DJ Premier era',
    details: ['26kHz sample rate', '12-bit depth', '15kHz brick-wall LP', 'Saturation drive'],
  },
  {
    id: '2000-hifi',
    name: 'MPC3000 Hi-Fi',
    icon: '\u{1F4BF}',
    year: '2000',
    description: 'Clean with presence \u2014 Timbaland, Neptunes polish',
    details: ['Haas stereo widening', 'High-shelf air boost', 'Tight compression', 'Bright finish'],
  },
  {
    id: '2024-crystal',
    name: 'Crystal Modern',
    icon: '\u{1F48E}',
    year: '2024',
    description: 'Ultra-clean with harmonic shimmer \u2014 contemporary production',
    details: ['Harmonic excitation', 'Transient preservation', 'Subtle stereo depth', 'Pristine clarity'],
  },
];

// ---------------------------------------------------------------------------
// 1993 Grit Processing
// ---------------------------------------------------------------------------

/**
 * Simulate vintage 12-bit sampler characteristics:
 * sample-rate reduction, bit-depth reduction, brick-wall lowpass, saturation.
 */
function process1993Grit(audioBuffer: AudioBuffer, intensity: number): AudioBuffer {
  const ctx = getAudioContext();
  const sampleRate = audioBuffer.sampleRate;
  const numChannels = audioBuffer.numberOfChannels;
  const length = audioBuffer.length;

  const output = ctx.createBuffer(numChannels, length, sampleRate);

  for (let ch = 0; ch < numChannels; ch++) {
    const inputData = audioBuffer.getChannelData(ch);
    const outputData = output.getChannelData(ch);

    // Copy source data to output first
    outputData.set(inputData);

    // --- 1. Sample rate reduction (zero-order hold) ---
    // At intensity 1.0 we simulate 26kHz; at 0 we keep original rate
    const targetRate = 26000 + (sampleRate - 26000) * (1 - intensity);
    const step = sampleRate / targetRate;

    if (step > 1) {
      let holdValue = outputData[0];
      let nextSwitch = 0;

      for (let i = 0; i < length; i++) {
        if (i >= nextSwitch) {
          holdValue = outputData[i];
          nextSwitch += step;
        }
        outputData[i] = holdValue;
      }
    }

    // --- 2. Bit depth reduction (12-bit = 4096 levels) ---
    // Mix quantized signal with original based on intensity
    const levels = 2048; // half of 4096
    for (let i = 0; i < length; i++) {
      const quantized = Math.round(outputData[i] * levels) / levels;
      outputData[i] = outputData[i] + (quantized - outputData[i]) * intensity;
    }

    // --- 3. Brick-wall lowpass at 15kHz (first-order IIR, matched-Z) ---
    const cutoffFreq = 15000;
    const a1 = Math.exp(-2 * Math.PI * cutoffFreq / sampleRate);
    let prevY = 0;

    for (let i = 0; i < length; i++) {
      const filtered = prevY * a1 + outputData[i] * (1 - a1);
      // Mix based on intensity
      outputData[i] = outputData[i] + (filtered - outputData[i]) * intensity;
      prevY = filtered;
      // Flush denormals — IIR filter states decaying below ~1e-38
      // cause 10-100x CPU stall on x86 when FTZ isn't set.
      if (Math.abs(prevY) < 1e-15) prevY = 0;
    }

    // --- 4. Saturation via tanh soft clip ---
    const drive = 1 + intensity * 0.5;
    for (let i = 0; i < length; i++) {
      outputData[i] = Math.tanh(outputData[i] * drive);
    }
  }

  return output;
}

// ---------------------------------------------------------------------------
// 2000 Hi-Fi Processing
// ---------------------------------------------------------------------------

/**
 * Polished modern production character:
 * Haas stereo widening, high-shelf air, gentle compression.
 */
function process2000HiFi(audioBuffer: AudioBuffer, intensity: number): AudioBuffer {
  const ctx = getAudioContext();
  const sampleRate = audioBuffer.sampleRate;
  const numChannels = audioBuffer.numberOfChannels;
  const length = audioBuffer.length;

  // Ensure stereo output (duplicate mono if needed)
  const outChannels = Math.max(2, numChannels);
  const output = ctx.createBuffer(outChannels, length, sampleRate);

  // Get source channels (duplicate mono for stereo processing)
  const srcLeft = audioBuffer.getChannelData(0);
  const srcRight = numChannels >= 2 ? audioBuffer.getChannelData(1) : srcLeft;

  const outLeft = output.getChannelData(0);
  const outRight = output.getChannelData(1);

  // Copy source data
  outLeft.set(srcLeft);
  outRight.set(srcRight);

  // --- 1. Haas stereo widening ---
  // Delay right channel by 0.4ms * intensity
  const delaySamples = Math.round(0.0004 * intensity * sampleRate);

  if (delaySamples > 0) {
    // Shift right channel forward by delaySamples (copy from end to start)
    const delayedRight = new Float32Array(length);
    for (let i = 0; i < length; i++) {
      const srcIdx = i - delaySamples;
      delayedRight[i] = srcIdx >= 0 ? outRight[srcIdx] : 0;
    }
    outRight.set(delayedRight);
  }

  // --- 2. High-shelf air boost at 8kHz (matched-Z one-pole LP) ---
  // Isolate high frequencies via first-order highpass, amplify, then add back.
  const shelfFreq = 8000;
  const lpA1 = Math.exp(-2 * Math.PI * shelfFreq / sampleRate);
  const boostGain = 1 + intensity * 3;

  for (let ch = 0; ch < outChannels; ch++) {
    const data = output.getChannelData(ch);
    let lpPrev = 0;

    for (let i = 0; i < length; i++) {
      const lpVal = lpPrev * lpA1 + data[i] * (1 - lpA1);
      const hpVal = data[i] - lpVal;
      data[i] = lpVal + hpVal * boostGain;
      lpPrev = lpVal;
      if (Math.abs(lpPrev) < 1e-15) lpPrev = 0; // flush denormals

      // Clamp to prevent clipping
      data[i] = Math.max(-1, Math.min(1, data[i]));
    }
  }

  // --- 3. Gentle compression ---
  // Reduce dynamic range above 0.7 threshold
  const threshold = 0.7;
  const ratio = 0.5; // 2:1 compression above threshold

  for (let ch = 0; ch < outChannels; ch++) {
    const data = output.getChannelData(ch);

    for (let i = 0; i < length; i++) {
      const absVal = Math.abs(data[i]);
      if (absVal > threshold) {
        const sign = data[i] >= 0 ? 1 : -1;
        data[i] = sign * (threshold + (absVal - threshold) * ratio);
      }
    }
  }

  return output;
}

// ---------------------------------------------------------------------------
// 2024 Crystal Processing
// ---------------------------------------------------------------------------

/**
 * Ultra-clean contemporary processing:
 * harmonic excitation, transient preservation, subtle stereo depth.
 */
function process2024Crystal(audioBuffer: AudioBuffer, intensity: number): AudioBuffer {
  const ctx = getAudioContext();
  const sampleRate = audioBuffer.sampleRate;
  const numChannels = audioBuffer.numberOfChannels;
  const length = audioBuffer.length;

  const output = ctx.createBuffer(numChannels, length, sampleRate);

  for (let ch = 0; ch < numChannels; ch++) {
    const inputData = audioBuffer.getChannelData(ch);
    const outputData = output.getChannelData(ch);

    // Copy source data
    outputData.set(inputData);

    // --- 1. Harmonic excitation ---
    // Generate subtle 2nd and 3rd harmonics
    for (let i = 0; i < length; i++) {
      const s = outputData[i];
      const secondHarmonic = s * s * 0.05 * intensity;
      const thirdHarmonic = s * s * s * 0.02 * intensity;
      outputData[i] = s + secondHarmonic + thirdHarmonic;
    }

    // --- 2. Transient preservation ---
    // Detect transients (large sample-to-sample differences) and boost them
    const transientThreshold = 0.05;
    const transientBoost = 1 + 0.1 * intensity;

    for (let i = 1; i < length; i++) {
      const diff = Math.abs(outputData[i] - outputData[i - 1]);
      if (diff > transientThreshold) {
        outputData[i] *= transientBoost;
      }
    }

    // Clamp all values
    for (let i = 0; i < length; i++) {
      outputData[i] = Math.max(-1, Math.min(1, outputData[i]));
    }
  }

  // --- 3. Subtle stereo depth (decorrelation) ---
  // Only apply if stereo
  if (numChannels >= 2) {
    const leftData = output.getChannelData(0);
    const rightData = output.getChannelData(1);
    const decorrelationAmount = 0.002 * intensity;

    for (let i = 1; i < length; i++) {
      // Slight phase offset between channels
      const leftOrig = leftData[i];
      const rightOrig = rightData[i];
      leftData[i] = Math.max(-1, Math.min(1, leftOrig + rightOrig * decorrelationAmount));
      rightData[i] = Math.max(-1, Math.min(1, rightOrig - leftOrig * decorrelationAmount));
    }
  }

  return output;
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

/**
 * Apply era-specific processing to an AudioBuffer.
 * Returns a new AudioBuffer (non-destructive).
 */
export function processEra(audioBuffer: AudioBuffer, config: EraConfig): AudioBuffer {
  const intensity = Math.max(0, Math.min(1, config.intensity));

  switch (config.mode) {
    case '1993-grit':
      return process1993Grit(audioBuffer, intensity);
    case '2000-hifi':
      return process2000HiFi(audioBuffer, intensity);
    case '2024-crystal':
      return process2024Crystal(audioBuffer, intensity);
    default:
      return audioBuffer;
  }
}

/**
 * Process from raw buffer -- decode, apply era processing, re-encode.
 * Returns { buffer: ArrayBuffer, peaks: number[] }
 */
export async function processEraFromBuffer(
  rawBuffer: ArrayBuffer,
  bitDepth: number,
  config: EraConfig
): Promise<{ buffer: ArrayBuffer; peaks: number[] }> {
  const audioBuffer = await decodeArrayBuffer(rawBuffer);
  const processed = processEra(audioBuffer, config);
  const buffer = await encodeWavAsync(processed, bitDepth);
  const peaks = generateWaveformPeaks(processed);
  return { buffer, peaks };
}
