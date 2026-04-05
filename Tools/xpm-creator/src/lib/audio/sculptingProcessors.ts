/**
 * Experimental Sculpting Processors
 *
 * Inspired by the Torso S-4 sampler's granular engine and physical modeling.
 * These processors create entirely new sounds from source material.
 */

import { getAudioContext } from './audioContext';
import { detectPitch } from './pitchDetector';

// ---------------------------------------------------------------------------
// Granular Cloud Engine ☁️
// ---------------------------------------------------------------------------

export interface GranularConfig {
  /** Grain size in seconds (0.01 = 10ms, 0.1 = 100ms) */
  grainSize: number;
  /** Density: grains per second (10-200) */
  density: number;
  /** Pitch scatter range in semitones (0-24) */
  pitchScatter: number;
  /** Position randomization (0 = sequential, 1 = fully random) */
  positionRandom: number;
  /** Output duration multiplier (1 = same length, 5 = 5x longer) */
  durationMultiplier: number;
  /** Stereo spread of grains (0 = mono, 1 = full stereo) */
  stereoSpread: number;
  /** Fade shape: 0 = Hanning, 0.5 = Tukey, 1 = Triangular */
  windowShape: number;
  /** Reverse probability per grain (0-1) */
  reverseProbability: number;
}

/**
 * Create a Hanning window for grain envelope.
 * Prevents clicks at grain boundaries.
 */
function createGrainWindow(length: number, shape: number = 0): Float32Array {
  const window = new Float32Array(length);
  const divisor = length > 1 ? length - 1 : 1;
  for (let i = 0; i < length; i++) {
    const t = i / divisor;
    if (shape <= 0.25) {
      // Hanning window — smooth, bell-shaped
      window[i] = 0.5 * (1 - Math.cos(2 * Math.PI * t));
    } else if (shape <= 0.75) {
      // Tukey window — flat top with tapered edges
      const alpha = 0.5;
      if (t < alpha / 2) {
        window[i] = 0.5 * (1 + Math.cos(Math.PI * (2 * t / alpha - 1)));
      } else if (t > 1 - alpha / 2) {
        window[i] = 0.5 * (1 + Math.cos(Math.PI * (2 * t / alpha - 2 / alpha + 1)));
      } else {
        window[i] = 1.0;
      }
    } else {
      // Triangular window
      window[i] = 1 - Math.abs(2 * t - 1);
    }
  }
  return window;
}

/**
 * Render a granular cloud from a source sample.
 *
 * Takes the source audio and "shatters" it into tiny grains,
 * scattering them with randomized positions, pitches, and panning
 * to create ethereal, atmospheric textures.
 */
export function renderGranularCloud(
  source: AudioBuffer,
  config: Partial<GranularConfig> = {}
): AudioBuffer {
  const ctx = getAudioContext();
  const {
    grainSize = 0.04,
    density = 60,
    pitchScatter = 3,
    positionRandom = 0.7,
    durationMultiplier = 2.5,
    stereoSpread = 0.6,
    windowShape = 0,
    reverseProbability = 0.15,
  } = config;

  const sampleRate = source.sampleRate;
  const sourceLength = source.length;
  const sourceMono = source.getChannelData(0);
  const outputLength = Math.ceil(sourceLength * durationMultiplier);
  const numChannels = 2; // Always stereo output for spatial grains

  const result = ctx.createBuffer(numChannels, outputLength, sampleRate);
  const outL = result.getChannelData(0);
  const outR = result.getChannelData(1);

  // Floor of 64 samples prevents degenerate grains (grainSize=0.001 at 44.1 kHz
  // gives 44 samples — too short for windowing and pitch resampling).
  const grainSamples = Math.max(64, Math.floor(grainSize * sampleRate));
  const MAX_GRAINS = 2000;
  let totalGrains = Math.floor(density * (outputLength / sampleRate));
  totalGrains = Math.min(MAX_GRAINS, totalGrains);
  const window = createGrainWindow(grainSamples, windowShape);

  for (let g = 0; g < totalGrains; g++) {
    // Output position: semi-evenly distributed with jitter
    const basePos = (g / totalGrains) * outputLength;
    const jitter = (Math.random() - 0.5) * (outputLength / totalGrains) * 0.8;
    const outPos = Math.max(0, Math.floor(basePos + jitter));

    // Source position: blend between sequential and random
    const sequentialPos = Math.floor((g / totalGrains) * (sourceLength - grainSamples));
    const randomPos = Math.floor(Math.random() * Math.max(0, sourceLength - grainSamples));
    const srcPos = Math.floor(
      sequentialPos * (1 - positionRandom) + randomPos * positionRandom
    );

    // Pitch shift via resampling (simple interpolation)
    const pitchSemitones = (Math.random() - 0.5) * 2 * pitchScatter;
    const rate = Math.pow(2, pitchSemitones / 12);

    // Stereo panning per grain
    const pan = (Math.random() - 0.5) * 2 * stereoSpread;
    const gainL = Math.cos((pan + 1) * Math.PI / 4);
    const gainR = Math.sin((pan + 1) * Math.PI / 4);

    // Reverse this grain?
    const shouldReverse = Math.random() < reverseProbability;

    // Amplitude variation for natural feel
    const ampVariation = 0.6 + Math.random() * 0.4;

    // Write grain to output
    const grainLength = Math.floor(grainSamples / rate);
    for (let i = 0; i < grainLength; i++) {
      const outIdx = outPos + i;
      if (outIdx >= outputLength) break;

      // Source sample position with pitch resampling
      let srcIdx: number;
      if (shouldReverse) {
        srcIdx = srcPos + grainSamples - 1 - Math.floor(i * rate);
      } else {
        srcIdx = srcPos + Math.floor(i * rate);
      }

      if (srcIdx < 0 || srcIdx >= sourceLength) continue;

      // Window index mapped to grain
      const windowIdx = Math.floor((i / grainLength) * grainSamples);
      const windowGain = windowIdx < window.length ? window[windowIdx] : 0;

      const sample = sourceMono[srcIdx] * windowGain * ampVariation;

      // Accumulate with stereo panning
      outL[outIdx] += sample * gainL;
      outR[outIdx] += sample * gainR;
    }
  }

  // Normalize to prevent clipping
  let maxAmp = 0;
  for (let i = 0; i < outputLength; i++) {
    maxAmp = Math.max(maxAmp, Math.abs(outL[i]), Math.abs(outR[i]));
  }
  if (maxAmp > 0.95) {
    const normalize = 0.9 / maxAmp;
    for (let i = 0; i < outputLength; i++) {
      outL[i] *= normalize;
      outR[i] *= normalize;
    }
  }

  return result;
}

// ---------------------------------------------------------------------------
// Destructive Resonator 🎸
// ---------------------------------------------------------------------------

export interface ResonatorConfig {
  /** Resonant frequency in Hz (determines delay time: 50-2000Hz) */
  frequency: number;
  /** Feedback amount (0-0.99). Higher = longer ring */
  feedback: number;
  /** Damping: lowpass cutoff in Hz for the feedback loop (500-20000) */
  damping: number;
  /** Mix: dry/wet blend (0 = dry, 1 = fully resonant) */
  mix: number;
  /** Brightness: highpass frequency to remove mud (20-500Hz) */
  brightness: number;
}

/**
 * Apply a resonator effect simulating a vibrating body.
 *
 * Uses a feedback delay line with very short delay times
 * and damping to create metallic, wooden, or string-like
 * tonal character from any source material.
 *
 * Based on Karplus-Strong synthesis principles.
 */
export function applyResonator(
  source: AudioBuffer,
  config: Partial<ResonatorConfig> = {}
): AudioBuffer {
  const ctx = getAudioContext();
  const {
    frequency = 220,
    feedback = 0.85,
    damping = 4000,
    mix = 0.5,
    brightness = 80,
  } = config;

  const sampleRate = source.sampleRate;
  // Add a tail for the resonance to decay — cap at 2s to prevent OOM
  const clampedFeedback = Math.min(0.999, feedback);
  const tailSamples = Math.min(
    Math.floor(sampleRate * 0.5 * clampedFeedback),
    sampleRate * 2,
  );
  const maxOutputLength = sampleRate * 30;
  let outputLength = source.length + tailSamples;
  outputLength = Math.min(outputLength, maxOutputLength);
  const numChannels = source.numberOfChannels;

  const result = ctx.createBuffer(numChannels, outputLength, sampleRate);

  // Delay line length from frequency
  const delaySamples = Math.max(2, Math.round(sampleRate / frequency));

  for (let ch = 0; ch < numChannels; ch++) {
    const src = source.getChannelData(ch);
    const dest = result.getChannelData(ch);

    // Circular delay buffer
    const delayBuffer = new Float32Array(delaySamples);
    let writePos = 0;

    // One-pole lowpass for damping (simple IIR)
    const dampCoeff = Math.exp(-2 * Math.PI * damping / sampleRate);
    let dampState = 0;

    // One-pole highpass for brightness
    const brightCoeff = Math.exp(-2 * Math.PI * brightness / sampleRate);
    let brightPrev = 0;
    let brightState = 0;

    for (let i = 0; i < outputLength; i++) {
      const dry = i < src.length ? src[i] : 0;

      // Read from delay line
      const readPos = ((writePos - delaySamples + delayBuffer.length) % delayBuffer.length);
      const delayed = delayBuffer[readPos];

      // Damping filter (lowpass in feedback loop)
      dampState = dampState * dampCoeff + delayed * (1 - dampCoeff);
      // Flush denormals — IIR filter states decaying below ~1e-38
      // cause 10-100x CPU stall on x86 when FTZ isn't set.
      if (Math.abs(dampState) < 1e-15) dampState = 0;

      // Brightness filter (highpass to remove mud)
      brightState = brightCoeff * (brightState + dampState - brightPrev);
      brightPrev = dampState;
      // Flush both IIR states — brightPrev feeds back into the highpass
      // difference equation, so a denormal here causes the same CPU stall.
      if (Math.abs(brightState) < 1e-15) brightState = 0;
      if (Math.abs(brightPrev) < 1e-15) brightPrev = 0;

      // Feedback into delay line
      const inputToDelay = dry + brightState * feedback;
      delayBuffer[writePos] = Math.max(-1, Math.min(1, inputToDelay));

      writePos = (writePos + 1) % delayBuffer.length;

      // Mix dry and wet
      dest[i] = dry * (1 - mix) + delayed * mix;
    }
  }

  // Normalize
  let maxAmp = 0;
  for (let ch = 0; ch < numChannels; ch++) {
    const data = result.getChannelData(ch);
    for (let i = 0; i < outputLength; i++) {
      maxAmp = Math.max(maxAmp, Math.abs(data[i]));
    }
  }
  if (maxAmp > 0.95) {
    const normalize = 0.9 / maxAmp;
    for (let ch = 0; ch < numChannels; ch++) {
      const data = result.getChannelData(ch);
      for (let i = 0; i < outputLength; i++) {
        data[i] *= normalize;
      }
    }
  }

  return result;
}

// ---------------------------------------------------------------------------
// Spectral Freeze (Time-Stretch Smear) 🧊
// ---------------------------------------------------------------------------

export interface SpectralFreezeConfig {
  /** Start position as fraction of source (0-1) */
  freezePoint: number;
  /** Duration of the frozen slice in seconds */
  sliceDuration: number;
  /** Stretch factor (10 = 10x stretch, 100 = 100x) */
  stretchFactor: number;
  /** Smoothing: crossfade length between grains in the stretch (0.01-0.2) */
  smoothing: number;
}

/**
 * Spectral freeze / time-stretch smear.
 *
 * Grabs a tiny slice of audio and stretches it via granular overlap,
 * creating a "frozen" tonal texture. Like holding a sustain pedal on
 * a single moment in time.
 */
export function renderSpectralFreeze(
  source: AudioBuffer,
  config: Partial<SpectralFreezeConfig> = {}
): AudioBuffer {
  const ctx = getAudioContext();
  const {
    freezePoint = 0.1,
    sliceDuration = 0.05,
    stretchFactor = 40,
    smoothing = 0.05,
  } = config;

  const sampleRate = source.sampleRate;
  const sourceMono = source.getChannelData(0);

  // Extract the slice
  const sliceStart = Math.floor(freezePoint * source.length);
  const sliceSamples = Math.floor(sliceDuration * sampleRate);
  const sliceEnd = Math.min(sliceStart + sliceSamples, source.length);
  const actualSlice = sliceEnd - sliceStart;

  if (actualSlice < 64) {
    // Too short to freeze — return source
    return source;
  }

  // Output length
  const MAX_OUTPUT_LENGTH = sampleRate * 10;
  let outputLength = Math.floor(actualSlice * stretchFactor);
  outputLength = Math.min(outputLength, MAX_OUTPUT_LENGTH);
  const result = ctx.createBuffer(2, outputLength, sampleRate); // Stereo
  const outL = result.getChannelData(0);
  const outR = result.getChannelData(1);

  // Granular overlap-add stretching
  // Use 80% of the slice as grain size — this leaves room for position
  // jitter within the slice, creating the shimmering "frozen" texture.
  // Using grainSize = actualSlice would make the upper clamp (sliceEnd - grainSize)
  // collapse to sliceStart, eliminating all jitter.
  const grainSize = Math.max(64, Math.floor(actualSlice * 0.8));
  const window = createGrainWindow(grainSize, 0); // Hanning

  let outWrite = 0;
  let grainCount = 0;

  while (outWrite < outputLength) {
    // Each grain reads from the frozen slice with slight position jitter
    const jitter = Math.floor((Math.random() - 0.5) * actualSlice * 0.3);
    const readStart = Math.max(sliceStart, Math.min(sliceStart + jitter, sliceEnd - grainSize));

    // Slight stereo offset
    const panAngle = Math.sin(grainCount * 0.7) * 0.4;
    const gL = Math.cos((panAngle + 1) * Math.PI / 4);
    const gR = Math.sin((panAngle + 1) * Math.PI / 4);

    // Micro-pitch variation
    const pitchVar = 1 + (Math.sin(grainCount * 1.3) * 0.005);

    for (let i = 0; i < grainSize; i++) {
      const outIdx = outWrite + i;
      if (outIdx >= outputLength) break;

      const srcIdx = readStart + Math.floor(i * pitchVar);
      if (srcIdx >= source.length || srcIdx < 0) continue;

      const windowGain = window[i];
      const sample = sourceMono[srcIdx] * windowGain;

      outL[outIdx] += sample * gL;
      outR[outIdx] += sample * gR;
    }

    outWrite += Math.floor(grainSize * 0.5); // 50% overlap
    grainCount++;
  }

  // Apply fade-in and fade-out
  const fadeSamples = Math.min(Math.floor(0.05 * sampleRate), Math.floor(outputLength / 4));
  for (let i = 0; i < fadeSamples; i++) {
    const gain = i / fadeSamples;
    outL[i] *= gain;
    outR[i] *= gain;
    outL[outputLength - 1 - i] *= gain;
    outR[outputLength - 1 - i] *= gain;
  }

  // Normalize
  let maxAmp = 0;
  for (let i = 0; i < outputLength; i++) {
    maxAmp = Math.max(maxAmp, Math.abs(outL[i]), Math.abs(outR[i]));
  }
  if (maxAmp > 0.01) {
    const normalize = 0.85 / maxAmp;
    for (let i = 0; i < outputLength; i++) {
      outL[i] *= normalize;
      outR[i] *= normalize;
    }
  }

  return result;
}

// ---------------------------------------------------------------------------
// Transient Snap 🥊
// ---------------------------------------------------------------------------

/**
 * Apply a transient snap envelope to the first N ms of a sample.
 * Adds a sharp attack spike without raising peak levels —
 * increases perceived "crack" and presence.
 */
export function applyTransientSnap(
  source: AudioBuffer,
  snapMs: number = 8,
  intensity: number = 0.6
): AudioBuffer {
  const ctx = getAudioContext();
  const result = ctx.createBuffer(source.numberOfChannels, source.length, source.sampleRate);
  const snapSamples = Math.floor(snapMs * source.sampleRate / 1000);

  for (let ch = 0; ch < source.numberOfChannels; ch++) {
    const src = source.getChannelData(ch);
    const dest = result.getChannelData(ch);

    for (let i = 0; i < source.length; i++) {
      if (i < snapSamples) {
        // Exponential spike: fast rise then quick decay
        const t = i / snapSamples;
        const spike = Math.exp(-t * 4) * intensity; // Fast exponential decay
        const boost = 1 + spike * 2;
        dest[i] = Math.max(-1, Math.min(1, src[i] * boost));
      } else {
        dest[i] = src[i];
      }
    }
  }

  return result;
}

// ---------------------------------------------------------------------------
// Valve Bloom 💨
// ---------------------------------------------------------------------------

/**
 * Simulates the "bloom" effect of tube amplification.
 * Slow attack swell combined with low-frequency warmth.
 * Makes hits "breathe" into the mix.
 */
export function applyValveBloom(
  source: AudioBuffer,
  bloomMs: number = 80,
  warmth: number = 0.5
): AudioBuffer {
  const ctx = getAudioContext();
  const result = ctx.createBuffer(source.numberOfChannels, source.length, source.sampleRate);
  const bloomSamples = Math.floor(bloomMs * source.sampleRate / 1000);

  for (let ch = 0; ch < source.numberOfChannels; ch++) {
    const src = source.getChannelData(ch);
    const dest = result.getChannelData(ch);

    // Low-frequency swell envelope
    for (let i = 0; i < source.length; i++) {
      let sample = src[i];

      if (i < bloomSamples) {
        const t = i / bloomSamples;
        // Slow S-curve attack (logistic function)
        const envelope = 1 / (1 + Math.exp(-10 * (t - 0.4)));
        // Add low-frequency swell (simulated tube bloom)
        const lfSwell = Math.sin(t * Math.PI) * warmth * 0.15;
        sample = sample * envelope + sample * lfSwell;
      }

      // Gentle tube-style soft clipping
      if (warmth > 0) {
        const drive = 1 + warmth * 1.5;
        sample = Math.tanh(sample * drive) / Math.tanh(drive);
      }

      dest[i] = sample;
    }
  }

  return result;
}

// ---------------------------------------------------------------------------
// Sub-Tail Generator 🔊
// ---------------------------------------------------------------------------

export interface SubTailConfig {
  /** Sub frequency in Hz (30-120). If undefined, auto-detect from source. */
  frequency?: number;
  /** Sub level 0-1 (default 0.5). */
  level: number;
  /** Decay time in ms (100-2000, default 500). */
  decayMs: number;
  /** Phase-align the sub to the source's first zero-crossing (default true). */
  phaseAlign: boolean;
}

/**
 * Generate a sub-bass tail for a drum sample.
 *
 * Auto-detects the fundamental frequency of the source (or uses a manual
 * override), then synthesizes a sine wave at that frequency with an
 * exponential decay envelope. The sub is phase-aligned to the source's
 * first positive zero-crossing for seamless layering.
 *
 * Returns a new AudioBuffer containing just the sub-tail. The caller
 * can assign it to a separate layer for mixing control.
 */
export function generateSubTail(
  source: AudioBuffer,
  config: Partial<SubTailConfig> = {},
): { subBuffer: AudioBuffer; detectedFrequency: number | null } {
  const ctx = getAudioContext();
  const {
    level = 0.5,
    decayMs = 500,
    phaseAlign = true,
  } = config;

  const sampleRate = source.sampleRate;
  const sourceData = source.getChannelData(0);

  // --- Frequency determination ---
  let subFreq: number;
  let detectedFrequency: number | null = null;

  if (config.frequency && config.frequency > 0) {
    subFreq = config.frequency;
  } else {
    // Auto-detect from source
    const detected = detectPitch(source, 0.0, 0.3);
    if (detected !== null) {
      detectedFrequency = detected;
      // Use sub-octave if detected pitch is above 120Hz
      subFreq = detected > 120 ? detected / 2 : detected;
      // Clamp to reasonable sub range
      subFreq = Math.max(30, Math.min(120, subFreq));
    } else {
      // Fallback: default sub frequency
      subFreq = 60;
    }
  }

  // --- Phase alignment ---
  // Find the first positive zero-crossing in the source to align the sub's phase
  let phaseOffset = 0;
  if (phaseAlign) {
    for (let i = 1; i < Math.min(sourceData.length, sampleRate * 0.01); i++) {
      if (sourceData[i - 1] <= 0 && sourceData[i] > 0) {
        phaseOffset = i / sampleRate;
        break;
      }
    }
  }

  // --- Sub synthesis ---
  // Guard: decayMs=0 would produce tau=0 → exp(-t/0) = NaN
  const safeDecayMs = Math.max(1, decayMs);
  const decaySamples = Math.floor((safeDecayMs / 1000) * sampleRate);
  const tailLength = Math.max(decaySamples, Math.floor(sampleRate * 0.1));
  const subBuffer = ctx.createBuffer(1, tailLength, sampleRate);
  const subData = subBuffer.getChannelData(0);

  const omega = 2 * Math.PI * subFreq;
  // Exponential decay time constant: reach ~1% at safeDecayMs
  const tau = safeDecayMs / (1000 * Math.log(100));

  for (let i = 0; i < tailLength; i++) {
    const t = i / sampleRate;
    // Sine wave with phase alignment
    const sine = Math.sin(omega * (t + phaseOffset));
    // Exponential decay envelope
    const envelope = Math.exp(-t / tau);
    // Combine with level control
    subData[i] = sine * envelope * level;
  }

  // Soft fade-out on last 5ms to avoid click at end
  const fadeSamples = Math.min(Math.floor(0.005 * sampleRate), tailLength);
  for (let i = 0; i < fadeSamples; i++) {
    const gain = i / fadeSamples;
    subData[tailLength - 1 - i] *= gain;
  }

  return { subBuffer, detectedFrequency };
}
