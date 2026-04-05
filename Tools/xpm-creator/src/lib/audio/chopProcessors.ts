import { getAudioContext, createOfflineContext } from './audioContext';

/**
 * Apply a linear fade-in to the start of an AudioBuffer.
 */
export function applyFadeIn(buffer: AudioBuffer, durationSec: number): AudioBuffer {
  const ctx = getAudioContext();
  const result = ctx.createBuffer(buffer.numberOfChannels, buffer.length, buffer.sampleRate);
  const fadeSamples = Math.min(Math.floor(durationSec * buffer.sampleRate), buffer.length);

  // Guard: 0-length fade → no-op copy (prevents division by zero → NaN corruption)
  if (fadeSamples <= 0) {
    for (let ch = 0; ch < buffer.numberOfChannels; ch++) {
      result.getChannelData(ch).set(buffer.getChannelData(ch));
    }
    return result;
  }

  for (let ch = 0; ch < buffer.numberOfChannels; ch++) {
    const src = buffer.getChannelData(ch);
    const dest = result.getChannelData(ch);
    for (let i = 0; i < buffer.length; i++) {
      if (i < fadeSamples) {
        dest[i] = src[i] * (i / fadeSamples);
      } else {
        dest[i] = src[i];
      }
    }
  }

  return result;
}

/**
 * Apply a linear fade-out to the end of an AudioBuffer.
 */
export function applyFadeOut(buffer: AudioBuffer, durationSec: number): AudioBuffer {
  const ctx = getAudioContext();
  const result = ctx.createBuffer(buffer.numberOfChannels, buffer.length, buffer.sampleRate);
  const fadeSamples = Math.min(Math.floor(durationSec * buffer.sampleRate), buffer.length);

  // Guard: 0-length fade → no-op copy (prevents division by zero → NaN corruption)
  if (fadeSamples <= 0) {
    for (let ch = 0; ch < buffer.numberOfChannels; ch++) {
      result.getChannelData(ch).set(buffer.getChannelData(ch));
    }
    return result;
  }

  const fadeStart = buffer.length - fadeSamples;

  for (let ch = 0; ch < buffer.numberOfChannels; ch++) {
    const src = buffer.getChannelData(ch);
    const dest = result.getChannelData(ch);
    for (let i = 0; i < buffer.length; i++) {
      if (i >= fadeStart) {
        const fadePos = i - fadeStart;
        dest[i] = src[i] * (1 - fadePos / fadeSamples);
      } else {
        dest[i] = src[i];
      }
    }
  }

  return result;
}

/**
 * Reverse the audio data in an AudioBuffer.
 */
export function reverseBuffer(buffer: AudioBuffer): AudioBuffer {
  const ctx = getAudioContext();
  const result = ctx.createBuffer(buffer.numberOfChannels, buffer.length, buffer.sampleRate);

  for (let ch = 0; ch < buffer.numberOfChannels; ch++) {
    const src = buffer.getChannelData(ch);
    const dest = result.getChannelData(ch);
    for (let i = 0; i < buffer.length; i++) {
      dest[i] = src[buffer.length - 1 - i];
    }
  }

  return result;
}

/**
 * Apply a biquad filter to an AudioBuffer using offline rendering.
 */
export async function applyFilter(
  buffer: AudioBuffer,
  type: BiquadFilterType,
  cutoffHz: number,
  resonance: number = 1
): Promise<AudioBuffer> {
  const offlineCtx = createOfflineContext(
    buffer.numberOfChannels,
    buffer.length,
    buffer.sampleRate
  );

  const source = offlineCtx.createBufferSource();
  source.buffer = buffer;

  const filter = offlineCtx.createBiquadFilter();
  filter.type = type;
  filter.frequency.value = cutoffHz;
  filter.Q.value = resonance;

  source.connect(filter);
  filter.connect(offlineCtx.destination);
  source.start(0);

  return offlineCtx.startRendering();
}

/**
 * Apply a logarithmic (exponential decay) fade-out to the end of an AudioBuffer.
 * More natural-sounding than linear fade — mimics acoustic instrument decay.
 */
export function applyLogFadeOut(buffer: AudioBuffer, durationSec: number): AudioBuffer {
  const ctx = getAudioContext();
  const result = ctx.createBuffer(buffer.numberOfChannels, buffer.length, buffer.sampleRate);
  const fadeSamples = Math.min(Math.floor(durationSec * buffer.sampleRate), buffer.length);

  // Guard: 0-length fade → no-op copy (prevents division by zero → NaN corruption)
  if (fadeSamples <= 0) {
    for (let ch = 0; ch < buffer.numberOfChannels; ch++) {
      result.getChannelData(ch).set(buffer.getChannelData(ch));
    }
    return result;
  }

  const fadeStart = buffer.length - fadeSamples;

  for (let ch = 0; ch < buffer.numberOfChannels; ch++) {
    const src = buffer.getChannelData(ch);
    const dest = result.getChannelData(ch);
    for (let i = 0; i < buffer.length; i++) {
      if (i >= fadeStart) {
        const progress = (i - fadeStart) / fadeSamples; // 0 → 1
        // Exponential curve: decays quickly at start, tapers gently
        const gain = Math.pow(1 - progress, 3);
        dest[i] = src[i] * gain;
      } else {
        dest[i] = src[i];
      }
    }
  }

  return result;
}

/**
 * Apply a 1ms safety fade-out to prevent digital clicks at the end of one-shot samples.
 */
export function applySafetyFadeOut(buffer: AudioBuffer): AudioBuffer {
  return applyFadeOut(buffer, 0.001);
}

/**
 * Flip the phase (invert polarity) of an AudioBuffer.
 * Multiplies all sample values by -1.
 */
export function flipPhase(buffer: AudioBuffer): AudioBuffer {
  const ctx = getAudioContext();
  const result = ctx.createBuffer(buffer.numberOfChannels, buffer.length, buffer.sampleRate);

  for (let ch = 0; ch < buffer.numberOfChannels; ch++) {
    const src = buffer.getChannelData(ch);
    const dest = result.getChannelData(ch);
    for (let i = 0; i < buffer.length; i++) {
      dest[i] = -src[i];
    }
  }

  return result;
}

/**
 * Trim silence from the end of an AudioBuffer.
 * Finds the last sample above the silence threshold and cuts everything after.
 */
export function trimSilence(
  buffer: AudioBuffer,
  thresholdDb: number = -60
): AudioBuffer {
  const ctx = getAudioContext();
  const threshold = Math.pow(10, thresholdDb / 20);

  // Find the last non-silent sample across all channels
  let lastLoudSample = 0;
  for (let ch = 0; ch < buffer.numberOfChannels; ch++) {
    const data = buffer.getChannelData(ch);
    for (let i = data.length - 1; i >= 0; i--) {
      if (Math.abs(data[i]) > threshold) {
        lastLoudSample = Math.max(lastLoudSample, i);
        break;
      }
    }
  }

  // Add a small buffer (10ms) after the last loud sample
  const endSample = Math.min(
    lastLoudSample + Math.floor(0.01 * buffer.sampleRate),
    buffer.length
  );

  if (endSample >= buffer.length - 1) return buffer; // Nothing to trim
  // Guard: don't create a 0-length buffer (e.g., fully-silent short sample)
  if (endSample <= 0) return buffer;

  const result = ctx.createBuffer(buffer.numberOfChannels, endSample, buffer.sampleRate);
  for (let ch = 0; ch < buffer.numberOfChannels; ch++) {
    const src = buffer.getChannelData(ch);
    const dest = result.getChannelData(ch);
    for (let i = 0; i < endSample; i++) {
      dest[i] = src[i];
    }
  }

  return result;
}

/**
 * Adjust the gain/volume of an AudioBuffer.
 */
export function adjustGain(buffer: AudioBuffer, gainFactor: number): AudioBuffer {
  const ctx = getAudioContext();
  const result = ctx.createBuffer(buffer.numberOfChannels, buffer.length, buffer.sampleRate);

  for (let ch = 0; ch < buffer.numberOfChannels; ch++) {
    const src = buffer.getChannelData(ch);
    const dest = result.getChannelData(ch);
    for (let i = 0; i < buffer.length; i++) {
      dest[i] = Math.max(-1, Math.min(1, src[i] * gainFactor));
    }
  }

  return result;
}
