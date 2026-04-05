/**
 * WAV Encoder Web Worker
 *
 * Offloads WAV encoding to a background thread to avoid blocking the UI.
 * Accepts channel data and parameters, returns an encoded WAV ArrayBuffer.
 */

export interface WavEncodeMessage {
  type: 'encode';
  id: string;
  channels: Float32Array[];
  sampleRate: number;
  bitDepth: number;
  length: number;
}

export interface WavEncodeResult {
  type: 'result';
  id: string;
  buffer: ArrayBuffer;
}

export interface WavEncodeError {
  type: 'error';
  id: string;
  error: string;
}

function writeString(view: DataView, offset: number, str: string): void {
  for (let i = 0; i < str.length; i++) {
    view.setUint8(offset + i, str.charCodeAt(i));
  }
}

function encodeWavWorker(
  channels: Float32Array[],
  sampleRate: number,
  bitDepth: number,
  length: number
): ArrayBuffer {
  if (bitDepth !== 16 && bitDepth !== 24) {
    throw new Error(`Unsupported bit depth: ${bitDepth}. Only 16 and 24 are supported.`);
  }
  if (length <= 0 || channels.length === 0) {
    throw new Error('Cannot encode empty audio data to WAV');
  }
  const numChannels = channels.length;
  const bytesPerSample = bitDepth / 8;
  const dataLength = length * numChannels * bytesPerSample;
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
    for (let i = 0; i < length; i++) {
      for (let ch = 0; ch < numChannels; ch++) {
        const sample = Math.max(-1, Math.min(1, channels[ch][i]));
        const int16 = Math.round(sample < 0 ? sample * 0x8000 : sample * 0x7FFF);
        view.setInt16(offset, int16, true);
        offset += 2;
      }
    }
  } else if (bitDepth === 24) {
    for (let i = 0; i < length; i++) {
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

  return arrayBuffer;
}

// Worker message handler
self.onmessage = (e: MessageEvent<WavEncodeMessage>) => {
  const { type, id, channels, sampleRate, bitDepth, length } = e.data;

  if (type !== 'encode') return;

  try {
    const buffer = encodeWavWorker(channels, sampleRate, bitDepth, length);
    const result: WavEncodeResult = { type: 'result', id, buffer };
    (self as unknown as Worker).postMessage(result, [buffer]); // Transfer ownership for zero-copy
  } catch (err) {
    const error: WavEncodeError = {
      type: 'error',
      id,
      error: err instanceof Error ? err.message : String(err),
    };
    (self as unknown as Worker).postMessage(error);
  }
};
