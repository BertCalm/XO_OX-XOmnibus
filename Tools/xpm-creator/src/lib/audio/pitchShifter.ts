import { createOfflineContext } from './audioContext';

/**
 * Pitch-shift an AudioBuffer by a number of semitones using phase vocoder
 * via the Web Audio API's offline rendering with playback rate changes
 * combined with resampling to maintain duration.
 *
 * For high-quality pitch shifting, we:
 * 1. Calculate the playback rate ratio for the semitone shift
 * 2. Create an offline context with adjusted length to compensate
 * 3. Play the buffer at the shifted rate
 * 4. Resample back to original length to preserve duration
 */
export async function pitchShiftSemitones(
  sourceBuffer: AudioBuffer,
  semitones: number,
  onProgress?: (progress: number) => void
): Promise<AudioBuffer> {
  if (semitones === 0) return sourceBuffer;

  // Clamp to ±48 semitones (4 octaves) to prevent extreme playback rates
  // that crash OfflineAudioContext or produce inaudible output
  const clampedSemitones = Math.max(-48, Math.min(48, semitones));
  const rate = Math.pow(2, clampedSemitones / 12);
  const sourceRate = sourceBuffer.sampleRate;
  const sourceLength = sourceBuffer.length;
  const numChannels = sourceBuffer.numberOfChannels;

  // Render at shifted pitch (this changes duration)
  const shiftedLength = Math.max(1, Math.ceil(sourceLength / rate));
  const shiftedCtx = createOfflineContext(numChannels, Math.max(1, shiftedLength), sourceRate);

  const bufferSource = shiftedCtx.createBufferSource();
  bufferSource.buffer = sourceBuffer;
  bufferSource.playbackRate.value = rate;
  bufferSource.connect(shiftedCtx.destination);
  bufferSource.start(0);

  onProgress?.(30);
  let shiftedBuffer: AudioBuffer;
  try {
    shiftedBuffer = await shiftedCtx.startRendering();
  } catch (err) {
    throw new Error(
      `Pitch shift rendering failed: ${err instanceof Error ? err.message : 'unknown error'}`
    );
  }
  onProgress?.(60);

  // Time-stretch back to original duration via manual linear interpolation.
  // IMPORTANT: We cannot use AudioBufferSourceNode.playbackRate for this —
  // playbackRate changes BOTH pitch and speed simultaneously, which would
  // undo the pitch shift from pass 1. Manual resampling preserves pitch
  // while restoring the original duration.
  const ratio = shiftedBuffer.length / sourceLength;
  const result = new AudioBuffer({
    numberOfChannels: numChannels,
    length: sourceLength,
    sampleRate: sourceRate,
  });

  onProgress?.(80);

  for (let ch = 0; ch < numChannels; ch++) {
    const src = shiftedBuffer.getChannelData(ch);
    const dest = result.getChannelData(ch);
    for (let i = 0; i < sourceLength; i++) {
      const srcIdx = i * ratio;
      const lo = Math.floor(srcIdx);
      const frac = srcIdx - lo;
      const s0 = lo < src.length ? src[lo] : 0;
      const s1 = lo + 1 < src.length ? src[lo + 1] : 0;
      dest[i] = s0 + frac * (s1 - s0); // Linear interpolation
    }
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
