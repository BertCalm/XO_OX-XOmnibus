import { DEFAULT_BIT_DEPTH } from '@/constants/mpcDefaults';
import { v4 as uuid } from 'uuid';

/**
 * Synchronous WAV encoder — works on the main thread.
 * Use encodeWavAsync() for large files to avoid UI blocking.
 */
export function encodeWav(
  buffer: AudioBuffer,
  bitDepth: number = DEFAULT_BIT_DEPTH
): ArrayBuffer {
  // Validate bit depth — only 16 and 24 are supported
  if (bitDepth !== 16 && bitDepth !== 24) {
    throw new Error(`Unsupported bit depth: ${bitDepth}. Only 16 and 24 are supported.`);
  }
  if (buffer.length === 0) {
    throw new Error('Cannot encode empty AudioBuffer to WAV');
  }
  const numChannels = buffer.numberOfChannels;
  const sampleRate = buffer.sampleRate;
  const bytesPerSample = bitDepth / 8;
  const dataLength = buffer.length * numChannels * bytesPerSample;
  const headerLength = 44;
  const totalLength = headerLength + dataLength;

  // Guard: WAV RIFF size field is uint32 — max ~4GB. Also prevent
  // RangeError from ArrayBuffer allocation on huge files.
  const MAX_WAV_BYTES = 0xFFFF_FFFE; // 2^32 - 2 (RIFF limit minus 8-byte header)
  if (totalLength > MAX_WAV_BYTES) {
    throw new Error(
      `WAV file too large (${(totalLength / 1e9).toFixed(1)} GB). ` +
      'Reduce sample count or duration — WAV format supports up to ~4 GB.'
    );
  }

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

  const channels: Float32Array[] = [];
  for (let ch = 0; ch < numChannels; ch++) {
    channels.push(buffer.getChannelData(ch));
  }

  let offset = 44;
  if (bitDepth === 16) {
    for (let i = 0; i < buffer.length; i++) {
      for (let ch = 0; ch < numChannels; ch++) {
        const sample = Math.max(-1, Math.min(1, channels[ch][i]));
        // Math.round prevents truncation-toward-zero DC bias on negative samples
        const int16 = Math.round(sample < 0 ? sample * 0x8000 : sample * 0x7FFF);
        view.setInt16(offset, int16, true);
        offset += 2;
      }
    }
  } else if (bitDepth === 24) {
    for (let i = 0; i < buffer.length; i++) {
      for (let ch = 0; ch < numChannels; ch++) {
        const sample = Math.max(-1, Math.min(1, channels[ch][i]));
        const val = sample < 0 ? sample * 0x800000 : sample * 0x7FFFFF;
        // Clamp after round to prevent overflow at exact ±1.0 boundaries
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

/**
 * Chunked async WAV encoder — runs on the main thread but yields between
 * chunks so the browser can handle UI events between batches.
 *
 * Processes CHUNK_SIZE samples at a time, then yields via setTimeout(0).
 * This keeps frame rate responsive during long encodes when no Worker is
 * available (e.g., unsupported environment or Worker load failure).
 */
const CHUNK_SIZE = 16_384; // ~370ms of audio at 44.1kHz per chunk

function encodeWavChunked(
  buffer: AudioBuffer,
  bitDepth: number = DEFAULT_BIT_DEPTH
): Promise<ArrayBuffer> {
  return new Promise<ArrayBuffer>((resolve, reject) => {
    if (bitDepth !== 16 && bitDepth !== 24) {
      reject(new Error(`Unsupported bit depth: ${bitDepth}. Only 16 and 24 are supported.`));
      return;
    }
    if (buffer.length === 0) {
      reject(new Error('Cannot encode empty AudioBuffer to WAV'));
      return;
    }

    const numChannels = buffer.numberOfChannels;
    const sampleRate = buffer.sampleRate;
    const bytesPerSample = bitDepth / 8;
    const dataLength = buffer.length * numChannels * bytesPerSample;
    const headerLength = 44;
    const totalLength = headerLength + dataLength;

    const MAX_WAV_BYTES = 0xFFFF_FFFE;
    if (totalLength > MAX_WAV_BYTES) {
      reject(new Error(
        `WAV file too large (${(totalLength / 1e9).toFixed(1)} GB). ` +
        'Reduce sample count or duration — WAV format supports up to ~4 GB.'
      ));
      return;
    }

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

    const channels: Float32Array[] = [];
    for (let ch = 0; ch < numChannels; ch++) {
      channels.push(buffer.getChannelData(ch));
    }

    let sampleIndex = 0;
    let offset = 44;
    const totalSamples = buffer.length;

    function processChunk() {
      const end = Math.min(sampleIndex + CHUNK_SIZE, totalSamples);

      if (bitDepth === 16) {
        for (let i = sampleIndex; i < end; i++) {
          for (let ch = 0; ch < numChannels; ch++) {
            const sample = Math.max(-1, Math.min(1, channels[ch][i]));
            const int16 = Math.round(sample < 0 ? sample * 0x8000 : sample * 0x7FFF);
            view.setInt16(offset, int16, true);
            offset += 2;
          }
        }
      } else {
        for (let i = sampleIndex; i < end; i++) {
          for (let ch = 0; ch < numChannels; ch++) {
            const sample = Math.max(-1, Math.min(1, channels[ch][i]));
            const val = sample < 0 ? sample * 0x800000 : sample * 0x7FFFFF;
            const intVal = Math.max(-0x800000, Math.min(0x7FFFFF, Math.round(val)));
            view.setUint8(offset, intVal & 0xFF);
            view.setUint8(offset + 1, (intVal >> 8) & 0xFF);
            view.setUint8(offset + 2, (intVal >> 16) & 0xFF);
            offset += 3;
          }
        }
      }

      sampleIndex = end;

      if (sampleIndex >= totalSamples) {
        resolve(arrayBuffer);
      } else {
        // Yield to the event loop before processing the next chunk
        setTimeout(processChunk, 0);
      }
    }

    setTimeout(processChunk, 0);
  });
}

/**
 * Async WAV encoder — non-blocking on the main thread.
 *
 * Primary path: offloads encoding to a Web Worker (zero-copy via Transferable).
 * Fallback path: chunked encoding on the main thread, yielding via setTimeout(0)
 * between 16 384-sample chunks so the browser can handle UI events and avoid
 * the 3+ second freeze seen with the fully-synchronous encodeWav().
 *
 * Use this function everywhere WAV encoding is needed. The synchronous
 * encodeWav() should only be used in contexts where async is not possible
 * (e.g., synchronous utility functions that have no async callers).
 */
let wavWorker: Worker | null = null;
const pendingJobs = new Map<string, { resolve: (buf: ArrayBuffer) => void; reject: (err: Error) => void }>();

function getWavWorker(): Worker | null {
  if (typeof window === 'undefined') return null;

  if (!wavWorker) {
    try {
      wavWorker = new Worker(
        new URL('./wavEncoder.worker.ts', import.meta.url),
        { type: 'module' }
      );
      wavWorker.onmessage = (e) => {
        const { type, id, buffer, error } = e.data;
        const job = pendingJobs.get(id);
        if (!job) return;
        pendingJobs.delete(id);
        if (type === 'result') {
          job.resolve(buffer);
        } else {
          job.reject(new Error(error || 'WAV encoding failed'));
        }
      };
      wavWorker.onerror = (e) => {
        console.warn('WAV worker error, falling back to chunked main-thread encoding:', e.message);
        // Reject all pending jobs
        pendingJobs.forEach((job, id) => {
          job.reject(new Error('Worker failed'));
          pendingJobs.delete(id);
        });
        wavWorker = null;
      };
    } catch {
      return null;
    }
  }
  return wavWorker;
}

export async function encodeWavAsync(
  buffer: AudioBuffer,
  bitDepth: number = DEFAULT_BIT_DEPTH
): Promise<ArrayBuffer> {
  const worker = getWavWorker();
  if (!worker) {
    // Fallback: chunked encoding on the main thread — yields between chunks
    // to keep the UI responsive instead of blocking for 3+ seconds.
    return encodeWavChunked(buffer, bitDepth);
  }

  const id = uuid();
  // IMPORTANT: Copy channel data before transferring to prevent neutering
  // the AudioBuffer's internal ArrayBuffers. buffer.getChannelData() returns
  // a live view into the AudioBuffer — transferring its backing ArrayBuffer
  // would corrupt the source AudioBuffer for any subsequent reads.
  const channels: Float32Array[] = [];
  for (let ch = 0; ch < buffer.numberOfChannels; ch++) {
    channels.push(buffer.getChannelData(ch).slice());
  }

  return new Promise<ArrayBuffer>((resolve, reject) => {
    pendingJobs.set(id, { resolve, reject });

    // Transfer the copied channel data for zero-copy to worker
    const transferable = channels.map((ch) => ch.buffer);
    worker.postMessage(
      {
        type: 'encode',
        id,
        channels,
        sampleRate: buffer.sampleRate,
        bitDepth,
        length: buffer.length,
      },
      transferable
    );
  });
}

function encodeWavFromFloat32(
  data: Float32Array,
  sampleRate: number,
  numChannels: number = 1,
  bitDepth: number = DEFAULT_BIT_DEPTH
): ArrayBuffer {
  if (bitDepth !== 16 && bitDepth !== 24) {
    throw new Error(`Unsupported bit depth: ${bitDepth}. Only 16 and 24 are supported.`);
  }
  if (data.length === 0) {
    throw new Error('Cannot encode empty Float32Array to WAV');
  }
  const bytesPerSample = bitDepth / 8;
  const samplesPerChannel = Math.floor(data.length / numChannels);
  // Align to full frames to prevent writing beyond the allocated ArrayBuffer
  const alignedLength = samplesPerChannel * numChannels;
  const dataLength = samplesPerChannel * numChannels * bytesPerSample;
  const headerLength = 44;
  const totalLength = headerLength + dataLength;

  const MAX_WAV_BYTES = 0xFFFF_FFFE;
  if (totalLength > MAX_WAV_BYTES) {
    throw new Error(
      `WAV file too large (${(totalLength / 1e9).toFixed(1)} GB). ` +
      'Reduce sample count or duration — WAV format supports up to ~4 GB.'
    );
  }

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
    for (let i = 0; i < alignedLength; i++) {
      const sample = Math.max(-1, Math.min(1, data[i]));
      const int16 = Math.round(sample < 0 ? sample * 0x8000 : sample * 0x7FFF);
      view.setInt16(offset, int16, true);
      offset += 2;
    }
  } else if (bitDepth === 24) {
    for (let i = 0; i < alignedLength; i++) {
      const sample = Math.max(-1, Math.min(1, data[i]));
      const val = sample < 0 ? sample * 0x800000 : sample * 0x7FFFFF;
      const intVal = Math.max(-0x800000, Math.min(0x7FFFFF, Math.round(val)));
      view.setUint8(offset, intVal & 0xFF);
      view.setUint8(offset + 1, (intVal >> 8) & 0xFF);
      view.setUint8(offset + 2, (intVal >> 16) & 0xFF);
      offset += 3;
    }
  }

  return arrayBuffer;
}

function writeString(view: DataView, offset: number, str: string): void {
  for (let i = 0; i < str.length; i++) {
    view.setUint8(offset + i, str.charCodeAt(i));
  }
}
