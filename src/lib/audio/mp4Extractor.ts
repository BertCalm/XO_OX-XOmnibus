import { FFmpeg } from '@ffmpeg/ffmpeg';
import { fetchFile, toBlobURL } from '@ffmpeg/util';

let ffmpeg: FFmpeg | null = null;

/** Timeout (ms) for fetching FFmpeg WASM — prevents indefinite hangs on slow connections. */
const FFMPEG_LOAD_TIMEOUT_MS = 30_000;

async function getFFmpeg(): Promise<FFmpeg> {
  if (ffmpeg && ffmpeg.loaded) return ffmpeg;

  // Check network connectivity before attempting a multi-MB WASM fetch
  if (typeof navigator !== 'undefined' && !navigator.onLine) {
    throw new Error('Cannot load video decoder — you appear to be offline.');
  }

  const instance = new FFmpeg();
  const baseURL = 'https://unpkg.com/@ffmpeg/core@0.12.6/dist/umd';

  const loadPromise = (async () => {
    const coreURL = await toBlobURL(`${baseURL}/ffmpeg-core.js`, 'text/javascript');
    const wasmURL = await toBlobURL(`${baseURL}/ffmpeg-core.wasm`, 'application/wasm');
    await instance.load({ coreURL, wasmURL });
  })();

  let timeoutId: ReturnType<typeof setTimeout>;
  const timeoutPromise = new Promise<never>((_, reject) => {
    timeoutId = setTimeout(() => {
      reject(new Error('FFmpeg WASM load timed out after 30s. Check your connection and try again.'));
    }, FFMPEG_LOAD_TIMEOUT_MS);
  });

  try {
    await Promise.race([loadPromise, timeoutPromise]);
  } catch (err) {
    // Don't cache a broken instance — next call will retry
    throw new Error(
      `FFmpeg load failed: ${err instanceof Error ? err.message : 'network error'}. ` +
      'Check your internet connection and try again.'
    );
  } finally {
    clearTimeout(timeoutId!);
  }

  ffmpeg = instance;
  return ffmpeg;
}

export async function extractAudioFromVideo(
  file: File,
  onProgress?: (progress: number) => void
): Promise<ArrayBuffer> {
  const ff = await getFFmpeg();

  const inputName = 'input' + getExtension(file.name);
  const outputName = 'output.wav';

  await ff.writeFile(inputName, await fetchFile(file));

  // Attach progress listener scoped to this extraction — remove after to
  // prevent listener accumulation on the long-lived FFmpeg instance.
  const progressHandler = onProgress
    ? ({ progress }: { progress: number }) => { onProgress(Math.min(progress * 100, 100)); }
    : null;
  if (progressHandler) ff.on('progress', progressHandler);

  try {
    await ff.exec([
      '-i', inputName,
      '-vn',
      '-acodec', 'pcm_s16le',
      // Omit -ar to preserve the source's native sample rate —
      // avoids unnecessary resampling (common: 48kHz video → 44.1kHz → 48kHz context).
      '-ac', '2',
      outputName,
    ]);
  } finally {
    if (progressHandler) ff.off('progress', progressHandler);
  }

  const data = await ff.readFile(outputName);

  await ff.deleteFile(inputName);
  await ff.deleteFile(outputName);

  if (data instanceof Uint8Array) {
    // data.buffer returns the *entire* backing ArrayBuffer, which may be
    // larger than the Uint8Array's view when FFmpeg returns a sub-view.
    // Slice to the exact region to avoid feeding garbage to the decoder.
    return data.buffer.slice(data.byteOffset, data.byteOffset + data.byteLength) as ArrayBuffer;
  }
  throw new Error('Unexpected output from FFmpeg');
}

function getExtension(filename: string): string {
  const idx = filename.lastIndexOf('.');
  return idx >= 0 ? filename.substring(idx) : '.mp4';
}
