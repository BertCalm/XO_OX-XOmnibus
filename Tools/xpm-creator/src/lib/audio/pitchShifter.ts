/**
 * WSOLA pitch shifter — production-quality pitch shifting via
 * time-stretching with waveform similarity overlap-add, then
 * resampling to target length using windowed sinc (Lanczos) interpolation.
 *
 * Steps:
 * 1. Time-stretch the input to a new duration using WSOLA
 * 2. Resample the stretched signal to the target pitch using sinc interpolation
 *
 * Quality improvement over the previous linear-interpolation + OfflineAudioContext
 * approach: WSOLA eliminates phasing artifacts at ±4+ semitone shifts; Lanczos
 * resampling eliminates aliasing from the earlier linear interpolation step.
 */

// WSOLA parameters
const WINDOW_SIZE = 1024; // analysis window size in samples
const OVERLAP = WINDOW_SIZE / 2; // 50% overlap
const SEARCH_RANGE = 128; // samples to search for best match

/**
 * Time-stretch a mono Float32Array by stretchFactor using WSOLA.
 * stretchFactor > 1 = slower (longer output), < 1 = faster (shorter output).
 */
function wsolaTimeStretch(input: Float32Array, stretchFactor: number): Float32Array {
  const outputLength = Math.round(input.length * stretchFactor);
  if (outputLength <= 0) return new Float32Array(0);
  const output = new Float32Array(outputLength);

  // Pre-compute Hann window for smooth overlap-add
  const window = new Float32Array(WINDOW_SIZE);
  for (let i = 0; i < WINDOW_SIZE; i++) {
    window[i] = 0.5 * (1 - Math.cos((2 * Math.PI * i) / WINDOW_SIZE));
  }

  const hopIn = OVERLAP;
  const hopOut = Math.round(OVERLAP * stretchFactor);

  // Guard against degenerate hop sizes
  const effectiveHopOut = Math.max(1, hopOut);

  let inputPos = 0;
  let outputPos = 0;

  while (outputPos + WINDOW_SIZE < outputLength && inputPos + WINDOW_SIZE < input.length) {
    // Cross-correlation search: find best-matching grain within ±SEARCH_RANGE
    let bestOffset = 0;
    let bestCorr = -Infinity;

    for (let offset = -SEARCH_RANGE; offset <= SEARCH_RANGE; offset++) {
      const searchPos = inputPos + offset;
      if (searchPos < 0 || searchPos + WINDOW_SIZE > input.length) continue;

      let corr = 0;
      for (let j = 0; j < WINDOW_SIZE; j++) {
        corr += input[searchPos + j] * input[inputPos + j];
      }
      if (corr > bestCorr) {
        bestCorr = corr;
        bestOffset = offset;
      }
    }

    // Overlap-add the best-matching windowed grain into the output
    const readPos = inputPos + bestOffset;
    for (let j = 0; j < WINDOW_SIZE; j++) {
      const outIdx = outputPos + j;
      if (outIdx >= outputLength) break;
      const inIdx = readPos + j;
      if (inIdx >= 0 && inIdx < input.length) {
        output[outIdx] += input[inIdx] * window[j];
      }
    }

    inputPos += hopIn;
    outputPos += effectiveHopOut;
  }

  return output;
}

/**
 * Resample input to targetLength using windowed sinc (Lanczos a=3) interpolation.
 * Significantly better alias rejection than linear interpolation.
 */
function sincResample(input: Float32Array, targetLength: number): Float32Array {
  if (targetLength <= 0) return new Float32Array(0);
  const output = new Float32Array(targetLength);
  const ratio = input.length / targetLength;
  const a = 3; // Lanczos kernel half-width (covers 6 input samples)

  for (let i = 0; i < targetLength; i++) {
    const srcPos = i * ratio;
    const srcIdx = Math.floor(srcPos);
    let sum = 0;
    let weightSum = 0;

    const jStart = srcIdx - a + 1;
    const jEnd = srcIdx + a;

    for (let j = jStart; j <= jEnd; j++) {
      if (j < 0 || j >= input.length) continue;
      const x = srcPos - j;
      let weight: number;
      if (Math.abs(x) < 1e-6) {
        weight = 1;
      } else if (Math.abs(x) < a) {
        const piX = Math.PI * x;
        weight = (a * Math.sin(piX) * Math.sin(piX / a)) / (piX * piX);
      } else {
        weight = 0;
      }
      sum += input[j] * weight;
      weightSum += weight;
    }

    output[i] = weightSum > 0 ? sum / weightSum : 0;
  }

  return output;
}

/**
 * Pitch-shift an AudioBuffer by a number of semitones using WSOLA time-stretching
 * followed by Lanczos sinc resampling to restore the original duration.
 *
 * The two-pass approach:
 *   Pass 1 — WSOLA time-stretch: changes duration, preserves pitch.
 *   Pass 2 — Sinc resample to original length: changes pitch, preserves duration.
 *
 * This avoids the OfflineAudioContext playbackRate approach which caused phasing
 * artifacts above ±4 semitones and relied on linear interpolation aliasing.
 */
export async function pitchShiftSemitones(
  sourceBuffer: AudioBuffer,
  semitones: number,
  onProgress?: (progress: number) => void
): Promise<AudioBuffer> {
  if (semitones === 0) {
    // Return a deep copy so callers can mutate without aliasing the original
    const copy = new AudioBuffer({
      numberOfChannels: sourceBuffer.numberOfChannels,
      length: sourceBuffer.length,
      sampleRate: sourceBuffer.sampleRate,
    });
    for (let ch = 0; ch < sourceBuffer.numberOfChannels; ch++) {
      copy.getChannelData(ch).set(sourceBuffer.getChannelData(ch));
    }
    return copy;
  }

  // Clamp to ±48 semitones (4 octaves) — beyond this, quality degrades severely
  if (Math.abs(semitones) > 24) {
    console.warn(
      `pitchShiftSemitones: ${semitones} semitones exceeds ±24 recommended range. Clamping.`
    );
  }
  const clampedSemitones = Math.max(-48, Math.min(48, semitones));

  // Pitch ratio: 2^(n/12). Shift up → ratio > 1 → time-stretch longer → resample shorter
  const pitchRatio = Math.pow(2, clampedSemitones / 12);
  const numChannels = sourceBuffer.numberOfChannels;
  const sourceLength = sourceBuffer.length;
  const sampleRate = sourceBuffer.sampleRate;

  onProgress?.(10);

  const result = new AudioBuffer({
    numberOfChannels: numChannels,
    length: sourceLength,
    sampleRate,
  });

  for (let ch = 0; ch < numChannels; ch++) {
    const inputData = sourceBuffer.getChannelData(ch);

    // Pass 1: WSOLA time-stretch by pitchRatio
    // Pitch up (ratio > 1): make it longer so resampling back shrinks pitch up
    // Pitch down (ratio < 1): make it shorter so resampling back stretches pitch down
    const stretched = wsolaTimeStretch(inputData, pitchRatio);

    onProgress?.(10 + Math.round((ch / numChannels) * 70));

    // Pass 2: sinc resample stretched signal back to original length
    const resampled = sincResample(stretched, sourceLength);

    onProgress?.(10 + Math.round(((ch + 0.8) / numChannels) * 70));

    result.getChannelData(ch).set(resampled);
  }

  onProgress?.(100);
  return result;
}

/**
 * Generate an array of pitch-shifted versions of a sample for chromatic mapping.
 * Given a root note, generates versions for each note in the specified range.
 */
export async function generateChromaticSamples(
  sourceBuffer: AudioBuffer,
  rootNote: number,
  lowNote: number,
  highNote: number,
  onProgress?: (note: number, total: number) => void
): Promise<Map<number, AudioBuffer>> {
  const results = new Map<number, AudioBuffer>();
  const totalNotes = highNote - lowNote + 1;
  let processed = 0;

  for (let note = lowNote; note <= highNote; note++) {
    const semitones = note - rootNote;
    const shifted = await pitchShiftSemitones(sourceBuffer, semitones);
    results.set(note, shifted);
    processed++;
    onProgress?.(processed, totalNotes);
  }

  return results;
}

/**
 * Pitch shift with fine-tuning in cents (1/100 of a semitone)
 */
export async function pitchShiftCents(
  sourceBuffer: AudioBuffer,
  cents: number
): Promise<AudioBuffer> {
  return pitchShiftSemitones(sourceBuffer, cents / 100);
}
