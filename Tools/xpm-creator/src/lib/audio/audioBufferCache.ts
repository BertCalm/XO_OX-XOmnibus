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

interface DecodingEntry {
  promise: Promise<AudioBuffer>;
  cancelled: boolean;
}

const decoding = new Map<string, DecodingEntry>();

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
  if (existing) return existing.promise;

  const entry: DecodingEntry = { promise: null as unknown as Promise<AudioBuffer>, cancelled: false };

  const promise = (async () => {
    try {
      const audioBuffer = await decodeArrayBuffer(rawBuffer);

      // If invalidateCache was called while this decode was in-flight, the
      // entry's cancelled flag will be true.  Do NOT write to the cache —
      // doing so would re-poison it with the stale/old buffer.
      if (!entry.cancelled) {
        const byteSize = estimateSize(audioBuffer);
        cache.set(sampleId, {
          buffer: audioBuffer,
          byteSize,
          lastAccessed: Date.now(),
        });
        currentCacheBytes += byteSize;
        evictIfNeeded();
      }

      return audioBuffer;
    } finally {
      // Always clean up the in-flight entry — even on decode failure or
      // cancellation — so future requests start a fresh decode.
      // Only remove our own entry; a newer entry (from a re-decode after
      // invalidation) must not be deleted.
      if (decoding.get(sampleId) === entry) {
        decoding.delete(sampleId);
      }
    }
  })();

  entry.promise = promise;
  decoding.set(sampleId, entry);
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
    const cacheEntry = cache.get(sampleId);
    if (cacheEntry) {
      currentCacheBytes -= cacheEntry.byteSize;
    }
    cache.delete(sampleId);

    // Mark any in-flight decode as cancelled so its completion handler does
    // not write the stale buffer back into the cache.  We also remove it from
    // the map so the next getDecodedBuffer call starts a fresh decode.
    const decodingEntry = decoding.get(sampleId);
    if (decodingEntry) {
      decodingEntry.cancelled = true;
      decoding.delete(sampleId);
    }
  } else {
    cache.clear();
    // Cancel and remove all in-flight decodes.
    decoding.forEach((entry) => { entry.cancelled = true; });
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
