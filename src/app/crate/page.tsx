'use client';

import React from 'react';
import { usePadStore } from '@/stores/padStore';
import { usePlaybackStore } from '@/stores/playbackStore';

/**
 * Floating Crate — Minimal pad grid for the always-on-top mini-window.
 * Loaded at /crate route by the floating crate window.
 * Shows a compact 4x4 pad grid with drag handles for reordering.
 */
export default function CratePage() {
  const pads = usePadStore((s) => s.pads);
  const activePadIndex = usePadStore((s) => s.activePadIndex);
  const setActivePad = usePadStore((s) => s.setActivePad);
  const currentBank = usePadStore((s) => s.currentBank);
  const playingPads = usePlaybackStore((s) => s.playingPads);
  const triggerPad = usePlaybackStore((s) => s.triggerPad);

  const bankOffset = currentBank * 16;

  const handlePadClick = (padIndex: number) => {
    setActivePad(padIndex);
    triggerPad(padIndex);
  };

  return (
    <div className="min-h-screen bg-surface-bg p-2 select-none" data-tauri-drag-region>
      {/* Header */}
      <div className="flex items-center justify-between mb-2 px-1" data-tauri-drag-region>
        <span className="text-[10px] font-bold text-text-muted uppercase tracking-widest">
          XO_OX Crate
        </span>
        <span className="text-[10px] text-text-muted">
          Bank {String.fromCharCode(65 + currentBank)}
        </span>
      </div>

      {/* 4x4 Pad Grid */}
      <div className="grid grid-cols-4 gap-1">
        {Array.from({ length: 16 }, (_, i) => {
          const padIndex = bankOffset + i;
          const pad = pads[padIndex];
          const hasLayers = pad?.layers.some((l) => l.active && l.sampleId);
          const isActive = padIndex === activePadIndex;
          const isPlaying = !!playingPads[padIndex];

          return (
            <button
              key={padIndex}
              onClick={() => handlePadClick(padIndex)}
              className={`
                aspect-square rounded-md border text-[9px] font-medium
                transition-all duration-75 active:scale-95
                ${isPlaying
                  ? 'bg-accent-purple/30 border-accent-purple text-accent-purple shadow-sm shadow-accent-purple/20'
                  : isActive
                    ? 'bg-accent-teal/15 border-accent-teal text-accent-teal'
                    : hasLayers
                      ? 'bg-surface-alt border-border text-text-secondary hover:border-text-muted'
                      : 'bg-surface-bg border-border/50 text-text-muted/40'
                }
              `}
            >
              {i + 1}
            </button>
          );
        })}
      </div>

      {/* Active pad info */}
      {pads[activePadIndex] && (
        <div className="mt-2 px-1">
          <p className="text-[9px] text-text-muted truncate">
            Pad {activePadIndex - bankOffset + 1}:{' '}
            {pads[activePadIndex].layers[0]?.sampleName || 'Empty'}
          </p>
        </div>
      )}
    </div>
  );
}
