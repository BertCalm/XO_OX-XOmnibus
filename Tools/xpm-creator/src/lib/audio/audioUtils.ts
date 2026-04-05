import { getAudioContext, ensureAudioContextRunning } from './audioContext';

export async function decodeAudioFile(file: File, timeoutMs: number = 15000): Promise<AudioBuffer> {
  if (file.size === 0) {
    throw new Error(`"${file.name}" is empty (0 bytes)`);
  }
  if (file.size < 44) {
    throw new Error(`"${file.name}" is too small to be valid audio (${file.size} bytes)`);
  }
  const arrayBuffer = await file.arrayBuffer();
  // Ensure context is running — on browsers with strict autoplay policy
  // (Safari/iOS), a suspended context can hang decodeAudioData indefinitely.
  const ctx = await ensureAudioContextRunning();
  // Clone the buffer so decodeAudioData's detach doesn't affect the original
  const decodePromise = ctx.decodeAudioData(arrayBuffer.slice(0));

  let timer: ReturnType<typeof setTimeout> | undefined;
  const timeoutPromise = new Promise<never>((_, reject) => {
    timer = setTimeout(() => reject(new Error(`Decode timed out for "${file.name}" — file may be too large or corrupt`)), timeoutMs);
  });

  try {
    const result = await Promise.race([decodePromise, timeoutPromise]);
    return result;
  } catch (e) {
    const msg = e instanceof Error ? e.message : String(e);
    if (msg.includes('timed out')) throw e;
    throw new Error(`Failed to decode "${file.name}": corrupt or unsupported format`);
  } finally {
    // Always clear the timeout to prevent orphaned timers and unhandled rejections
    if (timer !== undefined) clearTimeout(timer);
  }
}

export async function decodeArrayBuffer(
  buffer: ArrayBuffer,
  timeoutMs: number = 15000,
): Promise<AudioBuffer> {
  // Guard: reject detached or empty ArrayBuffers early with a clear message
  // instead of letting decodeAudioData fail opaquely.
  if (!buffer || buffer.byteLength === 0) {
    throw new Error(
      'Cannot decode audio: buffer is empty or detached. ' +
      'The sample data may have been lost — try re-importing the audio file.'
    );
  }

  // Ensure context is running — same Safari autoplay concern as decodeAudioFile
  const ctx = await ensureAudioContextRunning();
  const decodePromise = ctx.decodeAudioData(buffer.slice(0));

  let timer: ReturnType<typeof setTimeout> | undefined;
  const timeoutPromise = new Promise<never>((_, reject) => {
    timer = setTimeout(() => reject(new Error('Audio decode timed out — file may be too large or corrupt')), timeoutMs);
  });

  try {
    return await Promise.race([decodePromise, timeoutPromise]);
  } finally {
    if (timer !== undefined) clearTimeout(timer);
  }
}

function audioBufferToFloat32(buffer: AudioBuffer): Float32Array[] {
  const channels: Float32Array[] = [];
  for (let i = 0; i < buffer.numberOfChannels; i++) {
    channels.push(buffer.getChannelData(i));
  }
  return channels;
}

export function generateWaveformPeaks(
  buffer: AudioBuffer,
  numPeaks: number = 200
): number[] {
  if (buffer.length === 0 || buffer.numberOfChannels === 0) {
    return new Array(numPeaks).fill(0);
  }
  const data = buffer.getChannelData(0);
  const blockSize = Math.max(1, Math.floor(data.length / numPeaks));
  const peaks: number[] = [];

  for (let i = 0; i < numPeaks; i++) {
    let max = 0;
    const start = i * blockSize;
    // Clamp inner loop to avoid reading past the end of the typed array
    const end = Math.min(start + blockSize, data.length);
    for (let k = start; k < end; k++) {
      const abs = Math.abs(data[k]);
      if (abs > max) max = abs;
    }
    peaks.push(max);
  }

  return peaks;
}

export function formatDuration(seconds: number): string {
  const mins = Math.floor(seconds / 60);
  const secs = Math.floor(seconds % 60);
  const ms = Math.floor((seconds % 1) * 100);
  return `${mins}:${secs.toString().padStart(2, '0')}.${ms.toString().padStart(2, '0')}`;
}

export function formatFileSize(bytes: number): string {
  if (bytes < 1024) return `${bytes} B`;
  if (bytes < 1024 * 1024) return `${(bytes / 1024).toFixed(1)} KB`;
  return `${(bytes / (1024 * 1024)).toFixed(1)} MB`;
}

export function getAcceptedAudioFormats(): string {
  return '.wav,.mp3,.flac,.m4a,.mp4,.aac,.ogg';
}

export function isVideoFile(file: File): boolean {
  return file.type.startsWith('video/') || file.name.endsWith('.mp4');
}
