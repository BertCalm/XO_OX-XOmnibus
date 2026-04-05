let audioCtx: AudioContext | null = null;

/**
 * Cached error from AudioContext creation — prevents repeated failed attempts.
 * If the first creation fails (e.g., browser has no Web Audio support),
 * every subsequent call throws the same error immediately.
 */
let creationError: Error | null = null;

export function getAudioContext(): AudioContext {
  // If a previous creation attempt failed permanently, re-throw
  if (creationError) throw creationError;

  // Re-create if the context doesn't exist or was closed
  // (closed contexts cannot be resumed — a new one must be created)
  if (!audioCtx || audioCtx.state === 'closed') {
    try {
      // Let the browser inherit the system's native sample rate (48kHz, 96kHz, etc.)
      // Hardcoding 44100 forces unnecessary resampling on non-44.1kHz interfaces.
      audioCtx = new AudioContext();
    } catch (e) {
      creationError = e instanceof Error
        ? e
        : new Error('AudioContext creation failed — your browser may not support Web Audio');
      throw creationError;
    }
  }

  if (audioCtx.state === 'suspended') {
    // Fire-and-forget resume attempt. Most callers just need the context
    // reference; ensureAudioContextRunning() is the awaitable version for
    // callers that need the context actively running before scheduling.
    audioCtx.resume().catch((err) => {
      console.warn(
        'AudioContext.resume() failed — audio may not play until a user gesture occurs:',
        err
      );
    });
  }
  return audioCtx;
}

/**
 * Ensure the AudioContext is in the 'running' state.
 * Must be called from a user gesture handler (click, keydown, etc.)
 * before scheduling audio. Without this, browsers with strict autoplay
 * policies will silently refuse to play any audio.
 */
export async function ensureAudioContextRunning(): Promise<AudioContext> {
  const ctx = getAudioContext();
  if (ctx.state === 'suspended') {
    await ctx.resume();
  }
  return ctx;
}

export function createOfflineContext(
  channels: number,
  length: number,
  sampleRate?: number
): OfflineAudioContext {
  // Derive sample rate from live AudioContext if not specified
  const effectiveRate = sampleRate ?? getAudioContext().sampleRate;
  // Guard: length must be > 0, channels must be >= 1
  if (length <= 0 || channels <= 0) {
    throw new Error(
      `Invalid OfflineAudioContext params: channels=${channels}, length=${length}`
    );
  }
  return new OfflineAudioContext(channels, length, effectiveRate);
}
