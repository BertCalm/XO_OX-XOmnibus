/**
 * Pitch detection for AudioBuffers using autocorrelation.
 * Zero-dependency implementation — no external packages needed.
 *
 * Analyzes a segment of the audio to find the fundamental frequency,
 * then converts it to the nearest MIDI note number.
 */

const MIN_FREQ = 27.5;   // A0 — lowest piano note
const MAX_FREQ = 4186;   // C8 — highest piano note

/**
 * Detect the fundamental frequency of an AudioBuffer using autocorrelation.
 *
 * @param buffer - The AudioBuffer to analyze
 * @param analysisStartSec - Start position for analysis window (default: 0.05s to skip attack)
 * @param analysisDurationSec - Duration of the analysis window (default: 0.5s)
 * @returns The detected frequency in Hz, or null if no clear pitch found
 */
export function detectPitch(
  buffer: AudioBuffer,
  analysisStartSec: number = 0.05,
  analysisDurationSec: number = 0.5
): number | null {
  const sampleRate = buffer.sampleRate;
  const data = buffer.getChannelData(0);

  // Determine analysis window (skip the attack transient for cleaner detection)
  const startSample = Math.min(
    Math.floor(analysisStartSec * sampleRate),
    data.length - 1
  );
  const windowLength = Math.min(
    Math.floor(analysisDurationSec * sampleRate),
    data.length - startSample
  );

  // Need at least 2 full cycles of the lowest frequency we detect
  const minSamples = Math.ceil((sampleRate / MIN_FREQ) * 2);
  if (windowLength < minSamples) return null;

  // Extract the analysis window
  const window = data.subarray(startSample, startSample + windowLength);

  // Check if there's enough signal (not silence)
  let rms = 0;
  for (let i = 0; i < window.length; i++) {
    rms += window[i] * window[i];
  }
  rms = Math.sqrt(rms / window.length);
  if (rms < 0.01) return null; // Too quiet — probably silence

  // Autocorrelation-based pitch detection
  const freq = autocorrelate(window, sampleRate);
  if (freq === null || freq < MIN_FREQ || freq > MAX_FREQ) return null;

  return freq;
}

/**
 * Autocorrelation algorithm for pitch detection.
 * Based on the McLeod / normalized difference approach.
 */
function autocorrelate(data: Float32Array, sampleRate: number): number | null {
  const size = data.length;
  const maxLag = Math.floor(sampleRate / MIN_FREQ);
  const minLag = Math.floor(sampleRate / MAX_FREQ);

  // Compute the normalized square difference function (NSDF)
  // This is more robust than raw autocorrelation for pitch detection.
  // Allocate maxLag + 1 so index maxLag is valid — the peak-detection
  // loop reads nsdf[tau + 1] and parabolic interpolation reads nsdf[bestLag + 1].
  const nsdf = new Float32Array(maxLag + 1);

  for (let tau = minLag; tau < maxLag && tau < size / 2; tau++) {
    let acf = 0;  // autocorrelation
    let energy = 0; // normalization energy

    for (let i = 0; i < size - tau; i++) {
      acf += data[i] * data[i + tau];
      energy += data[i] * data[i] + data[i + tau] * data[i + tau];
    }

    nsdf[tau] = energy > 0 ? (2 * acf) / energy : 0;
  }

  // Find the first peak in the NSDF above a threshold
  // A peak is where nsdf[i] > nsdf[i-1] and nsdf[i] > nsdf[i+1]
  const threshold = 0.5; // Confidence threshold
  let bestLag = -1;
  let bestVal = threshold;

  // First, we need to skip past the initial decline from lag=0
  // Look for the first zero-crossing or minimum in the NSDF
  let passedFirstDip = false;

  // Start from minLag + 1 so nsdf[tau - 1] always references a written slot
  for (let tau = minLag + 1; tau < maxLag - 1 && tau < size / 2; tau++) {
    if (!passedFirstDip) {
      if (nsdf[tau] < 0 || (nsdf[tau] < nsdf[tau - 1] && nsdf[tau] < 0.1)) {
        passedFirstDip = true;
        // Fall through — this tau might be (or be adjacent to) the first peak.
        // Using `continue` here would skip it and potentially miss the fundamental.
      } else {
        continue;
      }
    }

    // Look for peaks
    if (nsdf[tau] > nsdf[tau - 1] && nsdf[tau] >= nsdf[tau + 1]) {
      if (nsdf[tau] > bestVal) {
        bestVal = nsdf[tau];
        bestLag = tau;
        // If we found a very confident peak, stop looking
        if (bestVal > 0.9) break;
      }
    }
  }

  if (bestLag <= 0) return null;

  // Parabolic interpolation for sub-sample accuracy
  const prev = nsdf[bestLag - 1];
  const curr = nsdf[bestLag];
  const next = nsdf[bestLag + 1];
  const delta = (prev - next) / (2 * (prev - 2 * curr + next));
  const refinedLag = bestLag + (isFinite(delta) ? delta : 0);

  return sampleRate / refinedLag;
}

/**
 * Convert a frequency in Hz to the nearest MIDI note number.
 */
export function frequencyToMidiNote(freq: number): number {
  const note = Math.round(12 * Math.log2(freq / 440) + 69);
  // Guard: Math.log2(NaN/0/negative) produces NaN/Infinity — reject these
  if (!Number.isFinite(note)) return -1;
  return note;
}

/**
 * Convert a MIDI note to its frequency in Hz.
 */
export function midiNoteToFrequency(note: number): number {
  return 440 * Math.pow(2, (note - 69) / 12);
}

/**
 * Detect the root MIDI note of an AudioBuffer.
 * Returns the MIDI note number (0-127), or null if no pitch detected.
 *
 * @param buffer - The AudioBuffer to analyze
 * @returns { midiNote, frequency, confidence } or null
 */
export function detectRootNote(
  buffer: AudioBuffer
): { midiNote: number; frequency: number } | null {
  const freq = detectPitch(buffer);
  if (freq === null) return null;

  const midiNote = frequencyToMidiNote(freq);
  if (midiNote < 0 || midiNote > 127) return null;

  return {
    midiNote,
    frequency: freq,
  };
}
