/**
 * Keygroup Export Web Worker
 *
 * Runs pitch-shifting and WAV encoding for every note in the keygroup range
 * off the main thread so the UI stays responsive during long exports.
 *
 * Pitch shifting is implemented in pure JS (no Web Audio API / OfflineAudioContext)
 * because those APIs are unavailable in Worker scope.
 *
 * Algorithm mirrors pitchShifter.ts:
 *   Pass 1 — resample channels at the playback-rate ratio (changes duration)
 *   Pass 2 — resample back to original length (restores duration, keeps pitch)
 */

// ---------------------------------------------------------------------------
// Message types
// ---------------------------------------------------------------------------

export interface WorkerInput {
  type: 'start';
  channelData: Float32Array[];
  sampleRate: number;
  numberOfChannels: number;
  sourceLength: number;
  lowNote: number;
  highNote: number;
  rootNote: number;
  bitDepth: number;
}

export interface WorkerCancel {
  type: 'cancel';
}

export interface WorkerProgress {
  type: 'progress';
  note: number;
  total: number;
  percent: number;
}

export interface WorkerNoteComplete {
  type: 'noteComplete';
  note: number;
  wavData: ArrayBuffer;
}

export interface WorkerComplete {
  type: 'complete';
}

export interface WorkerError {
  type: 'error';
  message: string;
}

type WorkerOutput = WorkerProgress | WorkerNoteComplete | WorkerComplete | WorkerError;

// ---------------------------------------------------------------------------
// Pure-JS pitch shifting (no OfflineAudioContext)
// ---------------------------------------------------------------------------

/**
 * Pitch-shift a single channel of audio by `semitones` using two linear-
 * interpolation resampling passes.  Mirrors the algorithm in pitchShifter.ts
 * but runs entirely in synchronous JS — safe for Worker scope.
 *
 * Pass 1: resample at `rate` (shortens/lengthens audio, changing pitch + duration)
 * Pass 2: resample back to `sourceLength` (restores duration, keeps new pitch)
 */
function pitchShiftChannel(
  src: Float32Array,
  sourceLength: number,
  semitones: number
): Float32Array {
  if (semitones === 0) {
    // Zero-copy for root note — return a view of the same data
    return src.slice(0, sourceLength);
  }

  const clampedSemitones = Math.max(-48, Math.min(48, semitones));
  const rate = Math.pow(2, clampedSemitones / 12);

  // Pass 1: resample at `rate` — produces shiftedLength samples
  const shiftedLength = Math.max(1, Math.ceil(sourceLength / rate));
  const pass1 = new Float32Array(shiftedLength);
  for (let i = 0; i < shiftedLength; i++) {
    const srcIdx = i * rate;
    const lo = Math.floor(srcIdx);
    const frac = srcIdx - lo;
    const s0 = lo < src.length ? src[lo] : 0;
    const s1 = lo + 1 < src.length ? src[lo + 1] : 0;
    pass1[i] = s0 + frac * (s1 - s0);
  }

  // Pass 2: resample back to sourceLength
  const ratio = shiftedLength / sourceLength;
  const dest = new Float32Array(sourceLength);
  for (let i = 0; i < sourceLength; i++) {
    const srcIdx = i * ratio;
    const lo = Math.floor(srcIdx);
    const frac = srcIdx - lo;
    const s0 = lo < pass1.length ? pass1[lo] : 0;
    const s1 = lo + 1 < pass1.length ? pass1[lo + 1] : 0;
    dest[i] = s0 + frac * (s1 - s0);
  }

  return dest;
}

// ---------------------------------------------------------------------------
// WAV encoding (inlined from wavEncoder.worker.ts — no imports in workers)
// ---------------------------------------------------------------------------

function writeString(view: DataView, offset: number, str: string): void {
  for (let i = 0; i < str.length; i++) {
    view.setUint8(offset + i, str.charCodeAt(i));
  }
}

function encodeWavFromChannels(
  channels: Float32Array[],
  sampleRate: number,
  bitDepth: number,
  length: number
): ArrayBuffer {
  const numChannels = channels.length;
  const bytesPerSample = bitDepth / 8;
  const dataLength = length * numChannels * bytesPerSample;
  const totalLength = 44 + dataLength;

  const arrayBuffer = new ArrayBuffer(totalLength);
  const view = new DataView(arrayBuffer);

  writeString(view, 0, 'RIFF');
  view.setUint32(4, totalLength - 8, true);
  writeString(view, 8, 'WAVE');
  writeString(view, 12, 'fmt ');
  view.setUint32(16, 16, true);
  view.setUint16(20, 1, true);
  view.setUint16(22, numChannels, true);
  view.setUint32(24, sampleRate, true);
  view.setUint32(28, sampleRate * numChannels * bytesPerSample, true);
  view.setUint16(32, numChannels * bytesPerSample, true);
  view.setUint16(34, bitDepth, true);
  writeString(view, 36, 'data');
  view.setUint32(40, dataLength, true);

  let offset = 44;
  if (bitDepth === 16) {
    for (let i = 0; i < length; i++) {
      for (let ch = 0; ch < numChannels; ch++) {
        const sample = Math.max(-1, Math.min(1, channels[ch][i] ?? 0));
        const int16 = Math.round(sample < 0 ? sample * 0x8000 : sample * 0x7FFF);
        view.setInt16(offset, int16, true);
        offset += 2;
      }
    }
  } else {
    // 24-bit
    for (let i = 0; i < length; i++) {
      for (let ch = 0; ch < numChannels; ch++) {
        const sample = Math.max(-1, Math.min(1, channels[ch][i] ?? 0));
        const val = sample < 0 ? sample * 0x800000 : sample * 0x7FFFFF;
        const intVal = Math.max(-0x800000, Math.min(0x7FFFFF, Math.round(val)));
        view.setUint8(offset, intVal & 0xFF);
        view.setUint8(offset + 1, (intVal >> 8) & 0xFF);
        view.setUint8(offset + 2, (intVal >> 16) & 0xFF);
        offset += 3;
      }
    }
  }

  return arrayBuffer;
}

// ---------------------------------------------------------------------------
// Worker message handler
// ---------------------------------------------------------------------------

let cancelled = false;

self.onmessage = async (e: MessageEvent<WorkerInput | WorkerCancel>) => {
  const msg = e.data;

  if (msg.type === 'cancel') {
    cancelled = true;
    return;
  }

  if (msg.type !== 'start') return;

  cancelled = false;
  const {
    channelData,
    sampleRate,
    numberOfChannels,
    sourceLength,
    lowNote,
    highNote,
    rootNote,
    bitDepth,
  } = msg;

  const totalNotes = highNote - lowNote + 1;

  try {
    for (let note = lowNote; note <= highNote; note++) {
      if (cancelled) return;

      const semitones = note - rootNote;
      const noteIndex = note - lowNote;

      // Pitch-shift all channels
      const shiftedChannels: Float32Array[] = [];
      for (let ch = 0; ch < numberOfChannels; ch++) {
        shiftedChannels.push(pitchShiftChannel(channelData[ch], sourceLength, semitones));
      }

      // Encode to WAV
      const wavData = encodeWavFromChannels(shiftedChannels, sampleRate, bitDepth, sourceLength);

      // Post progress
      const percent = ((noteIndex + 1) / totalNotes) * 100;
      const progressMsg: WorkerProgress = {
        type: 'progress',
        note,
        total: totalNotes,
        percent,
      };
      (self as unknown as Worker).postMessage(progressMsg);

      // Post the completed note's WAV data (transfer ownership for zero-copy)
      const noteCompleteMsg: WorkerNoteComplete = {
        type: 'noteComplete',
        note,
        wavData,
      };
      (self as unknown as Worker).postMessage(noteCompleteMsg, [wavData]);
    }

    const completeMsg: WorkerComplete = { type: 'complete' };
    (self as unknown as Worker).postMessage(completeMsg);
  } catch (err) {
    const errorMsg: WorkerError = {
      type: 'error',
      message: err instanceof Error ? err.message : String(err),
    };
    (self as unknown as Worker).postMessage(errorMsg);
  }
};
