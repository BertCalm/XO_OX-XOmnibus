import { decodeArrayBuffer } from './audioUtils';

/**
 * LRU audio buffer cache with configurable max entry count.
 * Prevents unbounded memory growth from decoded AudioBuffers.
 */

const MAX_CACHE_ENTRIES = 64;
const MAX_CACHE_BYTES = 256 * 1024 * 1024; // 256 MB

let currentCacheBytes = 0;

interface CacheEntry {
  buffer: AudioBuffer;
  byteSize: number;
  lastAccessed: number;
}

const cache = new Map<string, CacheEntry>();
const decoding = new Map<string, Promise<AudioBuffer>>();

function estimateSize(buffer: AudioBuffer): number {
  return buffer.length * buffer.numberOfChannels * 4; // Float32 = 4 bytes
}

/** Evict least-recently-used entries when cache exceeds max entry count or byte budget. */
function evictIfNeeded(): void {
  if (cache.size <= MAX_CACHE_ENTRIES && currentCacheBytes <= MAX_CACHE_BYTES) return;

  const entries = Array.from(cache.entries()).sort(
    (a, b) => a[1].lastAccessed - b[1].lastAccessed
  );

  while (
    (cache.size > MAX_CACHE_ENTRIES || currentCacheBytes > MAX_CACHE_BYTES) &&
    entries.length > 0
  ) {
    const [key, entry] = entries.shift()!;
    currentCacheBytes -= entry.byteSize;
    cache.delete(key);
  }
}

/**
 * Get a decoded AudioBuffer, decoding on first access and caching.
 * Subsequent calls return the cached buffer instantly.
 */
export async function getDecodedBuffer(
  sampleId: string,
  rawBuffer: ArrayBuffer
): Promise<AudioBuffer> {
  const cached = cache.get(sampleId);
  if (cached) {
    cached.lastAccessed = Date.now();
    return cached.buffer;
  }

  // If already decoding, wait for the existing promise
  const existing = decoding.get(sampleId);
  if (existing) return existing;

  const promise = (async () => {
    try {
      const audioBuffer = await decodeArrayBuffer(rawBuffer);

      const byteSize = estimateSize(audioBuffer);
      cache.set(sampleId, {
        buffer: audioBuffer,
        byteSize,
        lastAccessed: Date.now(),
      });
      currentCacheBytes += byteSize;
      evictIfNeeded();

      return audioBuffer;
    } finally {
      // Always clean up the in-flight promise — even on decode failure.
      // Without this, a rejected promise stays in the decoding map forever,
      // causing every subsequent request for this sampleId to return the
      // already-rejected promise.
      decoding.delete(sampleId);
    }
  })();

  decoding.set(sampleId, promise);
  return promise;
}

/**
 * Synchronous lookup — returns null if not yet decoded.
 * Updates LRU timestamp on access.
 */
export function getCachedBuffer(sampleId: string): AudioBuffer | null {
  const entry = cache.get(sampleId);
  if (entry) {
    entry.lastAccessed = Date.now();
    return entry.buffer;
  }
  return null;
}

/**
 * Pre-decode a buffer into the cache (fire-and-forget).
 */
function preDecodeBuffer(sampleId: string, rawBuffer: ArrayBuffer): void {
  if (cache.has(sampleId) || decoding.has(sampleId)) return;
  getDecodedBuffer(sampleId, rawBuffer).catch((err) => {
    console.warn(`Pre-decode failed for ${sampleId}:`, err);
  });
}

/**
 * Invalidate one or all cached buffers.
 */
export function invalidateCache(sampleId?: string): void {
  if (sampleId) {
    const entry = cache.get(sampleId);
    if (entry) {
      currentCacheBytes -= entry.byteSize;
    }
    cache.delete(sampleId);
    decoding.delete(sampleId);
  } else {
    cache.clear();
    decoding.clear();
    currentCacheBytes = 0;
  }
}

/** Current number of cached buffers. */
function getCacheSize(): number {
  return cache.size;
}

/** Total approximate bytes of cached audio data. */
function getCacheTotalBytes(): number {
  let total = 0;
  cache.forEach((entry) => {
    total += entry.byteSize;
  });
  return total;
}
