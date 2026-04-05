'use client';

import React, { useState, useRef, useCallback, useEffect } from 'react';
import { getAudioContext } from '@/lib/audio/audioContext';
import { getDecodedBuffer } from '@/lib/audio/audioBufferCache';
import Button from '@/components/ui/Button';
import { useToastStore } from '@/stores/toastStore';

interface SamplePlayerProps {
  sampleId: string;
  buffer: ArrayBuffer;
  name: string;
}

const SamplePlayer = React.memo(function SamplePlayer({ sampleId, buffer, name }: SamplePlayerProps) {
  const [isPlaying, setIsPlaying] = useState(false);
  const sourceRef = useRef<AudioBufferSourceNode | null>(null);

  // Stop and disconnect audio source on unmount to prevent leaked AudioNodes
  useEffect(() => {
    return () => {
      if (sourceRef.current) {
        try { sourceRef.current.stop(); } catch { /* already stopped */ }
        try { sourceRef.current.disconnect(); } catch { /* already disconnected */ }
        sourceRef.current = null;
      }
    };
  }, []);

  const handlePlay = useCallback(async () => {
    // Use sourceRef as source of truth instead of isPlaying state to avoid
    // stale closure during the async getDecodedBuffer gap.
    if (sourceRef.current) {
      try { sourceRef.current.stop(); } catch { /* already stopped */ }
      try { sourceRef.current.disconnect(); } catch { /* already disconnected */ }
      sourceRef.current = null;
      setIsPlaying(false);
      return;
    }

    try {
      const ctx = getAudioContext();
      // Use the LRU cache — avoids re-decoding on every play click
      const audioBuffer = await getDecodedBuffer(sampleId, buffer);

      // Guard: user may have clicked stop during async decode
      if (sourceRef.current) return;

      const source = ctx.createBufferSource();
      source.buffer = audioBuffer;
      source.connect(ctx.destination);
      source.onended = () => {
        // Guard: only update state if this source is still the active one
        if (sourceRef.current === source) {
          try { source.disconnect(); } catch { /* already disconnected */ }
          sourceRef.current = null;
          setIsPlaying(false);
        }
      };
      source.start(0);
      sourceRef.current = source;
      setIsPlaying(true);
    } catch (error) {
      console.error('Playback failed:', error);
      useToastStore.getState().addToast({
        type: 'error',
        title: 'Playback failed',
        message: err instanceof Error ? err.message : 'Click anywhere to activate audio, then try again.',
      });
      setIsPlaying(false);
    }
  }, [sampleId, buffer]);

  return (
    <Button
      variant="ghost"
      size="sm"
      onClick={handlePlay}
      aria-label={isPlaying ? `Stop ${name}` : `Play ${name}`}
      icon={
        isPlaying ? (
          <svg width="14" height="14" viewBox="0 0 14 14" fill="none">
            <rect x="3" y="2" width="3" height="10" rx="1" fill="currentColor" />
            <rect x="8" y="2" width="3" height="10" rx="1" fill="currentColor" />
          </svg>
        ) : (
          <svg width="14" height="14" viewBox="0 0 14 14" fill="none">
            <path d="M3 2.5v9l8-4.5-8-4.5z" fill="currentColor" />
          </svg>
        )
      }
    >
      {isPlaying ? 'Stop' : 'Play'}
    </Button>
  );
});

export default SamplePlayer;
