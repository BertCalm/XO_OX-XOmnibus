'use client';

import React, { useState, useCallback, useRef, useEffect } from 'react';
import { useAudioStore } from '@/stores/audioStore';
import { getDecodedBuffer } from '@/lib/audio/audioBufferCache';
import { getAudioContext } from '@/lib/audio/audioContext';
import { useToastStore } from '@/stores/toastStore';
import Card, { CardHeader, CardTitle } from '@/components/ui/Card';
import Button from '@/components/ui/Button';
import type { AudioSample } from '@/types';

type Slot = 'a' | 'b';

export default function ABCompare() {
  const samples = useAudioStore((s) => s.samples);
  const [slotA, setSlotA] = useState<string | null>(null);
  const [slotB, setSlotB] = useState<string | null>(null);
  const [activeSlot, setActiveSlot] = useState<Slot>('a');
  const [isPlaying, setIsPlaying] = useState(false);
  const sourceRef = useRef<AudioBufferSourceNode | null>(null);

  const sampleA = samples.find((s) => s.id === slotA) ?? null;
  const sampleB = samples.find((s) => s.id === slotB) ?? null;

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

  const handleDrop = useCallback(
    (slot: Slot) => (e: React.DragEvent) => {
      e.preventDefault();
      const sampleId = e.dataTransfer.getData('sampleId');
      if (sampleId) {
        if (slot === 'a') setSlotA(sampleId);
        else setSlotB(sampleId);
      }
    },
    []
  );

  const handleDragOver = useCallback((e: React.DragEvent) => {
    e.preventDefault();
    e.dataTransfer.dropEffect = 'copy';
  }, []);

  const stopPlayback = useCallback(() => {
    if (sourceRef.current) {
      try { sourceRef.current.stop(); } catch { /* already stopped */ }
      try { sourceRef.current.disconnect(); } catch { /* already disconnected */ }
      sourceRef.current = null;
    }
    setIsPlaying(false);
  }, []);

  const play = useCallback(
    async (slot: Slot) => {
      stopPlayback();
      const sample = slot === 'a' ? sampleA : sampleB;
      if (!sample) return;

      try {
        const ctx = getAudioContext();
        const buffer = await getDecodedBuffer(sample.id, sample.buffer);
        const source = ctx.createBufferSource();
        source.buffer = buffer;
        source.connect(ctx.destination);
        // Guard: only update state if this source is still the active one —
        // prevents a stale ended callback from clearing isPlaying for a new playback.
        // Disconnect the AudioNode to free graph resources (GC can't collect
        // connected nodes — they stay in the audio graph indefinitely).
        source.onended = () => {
          if (sourceRef.current === source) {
            try { source.disconnect(); } catch { /* already disconnected */ }
            sourceRef.current = null;
            setIsPlaying(false);
          }
        };
        source.start();
        sourceRef.current = source;
        setIsPlaying(true);
        setActiveSlot(slot);
      } catch (error) {
        console.error('A/B playback failed:', error);
        useToastStore.getState().addToast({
          type: 'error',
          title: 'Playback failed',
          message: 'Click the page first to enable audio, or the sample may be corrupt.',
        });
      }
    },
    [sampleA, sampleB, stopPlayback]
  );

  const toggleSlot = useCallback(() => {
    const next = activeSlot === 'a' ? 'b' : 'a';
    setActiveSlot(next);
    if (isPlaying) play(next);
  }, [activeSlot, isPlaying, play]);

  return (
    <Card padding="sm">
      <CardHeader>
        <CardTitle>A/B Compare</CardTitle>
      </CardHeader>
      <div className="grid grid-cols-2 gap-2 p-2">
        {/* Slot A */}
        <div
          role="button"
          tabIndex={0}
          aria-label={sampleA ? `Slot A: ${sampleA.name} — click to play` : 'Slot A: drop a sample here to compare'}
          onDrop={handleDrop('a')}
          onDragOver={handleDragOver}
          onKeyDown={(e) => { if ((e.key === 'Enter' || e.key === ' ') && sampleA) { e.preventDefault(); play('a'); } }}
          className={`relative rounded-lg border-2 border-dashed p-3 text-center transition-all min-h-[60px]
            flex flex-col items-center justify-center
            focus:outline-none focus:ring-2 focus:ring-accent-teal
            ${activeSlot === 'a' ? 'border-accent-teal bg-accent-teal/5' : 'border-border'}
            ${!sampleA ? 'cursor-crosshair' : 'cursor-pointer'}`}
          onClick={() => sampleA && play('a')}
        >
          <span className="text-[10px] font-bold text-text-muted uppercase mb-1">A</span>
          {sampleA ? (
            <>
              <p className="text-xs font-medium text-text-primary truncate max-w-full">{sampleA.name}</p>
              <p className="text-[9px] text-text-muted">{(sampleA.duration).toFixed(2)}s</p>
            </>
          ) : (
            <p className="text-[10px] text-text-muted italic">Drop sample here</p>
          )}
        </div>

        {/* Slot B */}
        <div
          role="button"
          tabIndex={0}
          aria-label={sampleB ? `Slot B: ${sampleB.name} — click to play` : 'Slot B: drop a sample here to compare'}
          onDrop={handleDrop('b')}
          onDragOver={handleDragOver}
          onKeyDown={(e) => { if ((e.key === 'Enter' || e.key === ' ') && sampleB) { e.preventDefault(); play('b'); } }}
          className={`relative rounded-lg border-2 border-dashed p-3 text-center transition-all min-h-[60px]
            flex flex-col items-center justify-center
            focus:outline-none focus:ring-2 focus:ring-accent-plum
            ${activeSlot === 'b' ? 'border-accent-plum bg-accent-plum/5' : 'border-border'}
            ${!sampleB ? 'cursor-crosshair' : 'cursor-pointer'}`}
          onClick={() => sampleB && play('b')}
        >
          <span className="text-[10px] font-bold text-text-muted uppercase mb-1">B</span>
          {sampleB ? (
            <>
              <p className="text-xs font-medium text-text-primary truncate max-w-full">{sampleB.name}</p>
              <p className="text-[9px] text-text-muted">{(sampleB.duration).toFixed(2)}s</p>
            </>
          ) : (
            <p className="text-[10px] text-text-muted italic">Drop sample here</p>
          )}
        </div>
      </div>

      {/* Controls */}
      <div className="flex items-center justify-center gap-2 p-2">
        <Button
          variant="secondary"
          size="sm"
          onClick={toggleSlot}
          disabled={!sampleA || !sampleB}
        >
          A ⇄ B
        </Button>
        {isPlaying && (
          <Button variant="secondary" size="sm" onClick={stopPlayback}>
            Stop
          </Button>
        )}
      </div>
    </Card>
  );
}
