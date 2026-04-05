'use client';

import React, { useCallback } from 'react';
import { useHistoryStore } from '@/stores/historyStore';
import { usePadStore } from '@/stores/padStore';
import Tooltip from '@/components/ui/Tooltip';

export default function UndoTimeline() {
  const past = useHistoryStore((s) => s.past);
  const future = useHistoryStore((s) => s.future);
  const undo = useHistoryStore((s) => s.undo);
  const redo = useHistoryStore((s) => s.redo);
  const pads = usePadStore((s) => s.pads);
  const setPads = usePadStore((s) => s.setPads);

  const totalEntries = past.length + future.length;

  const handleJump = useCallback(
    (targetPastIndex: number) => {
      const currentPastLength = useHistoryStore.getState().past.length;
      let currentPads = usePadStore.getState().pads;

      if (targetPastIndex < currentPastLength) {
        // Jump backward — undo (currentPastLength - targetPastIndex) times
        const count = currentPastLength - targetPastIndex;
        for (let i = 0; i < count; i++) {
          const result = useHistoryStore.getState().undo(currentPads);
          if (result) {
            currentPads = result;
            setPads(currentPads);
          }
        }
      } else if (targetPastIndex > currentPastLength) {
        // Jump forward — redo (targetPastIndex - currentPastLength) times
        const count = targetPastIndex - currentPastLength;
        for (let i = 0; i < count; i++) {
          const result = useHistoryStore.getState().redo(currentPads);
          if (result) {
            currentPads = result;
            setPads(currentPads);
          }
        }
      }
    },
    [setPads]
  );

  // Don't render anything if there's no history
  if (totalEntries === 0) return null;

  return (
    <div className="flex items-center gap-1 px-1" aria-label="Undo timeline">
      {/* Undo button */}
      <button
        onClick={() => {
          const currentPads = usePadStore.getState().pads;
          const result = undo(currentPads);
          if (result) setPads(result);
        }}
        disabled={past.length === 0}
        className="p-1 rounded text-text-muted hover:text-text-primary disabled:opacity-30 disabled:pointer-events-none transition-colors"
        aria-label="Undo"
        title="Undo (⌘Z)"
      >
        <svg width="12" height="12" viewBox="0 0 12 12" fill="none">
          <path d="M3 4l-2 2 2 2" stroke="currentColor" strokeWidth="1.2" strokeLinecap="round" strokeLinejoin="round" />
          <path d="M1 6h7a3 3 0 010 6H6" stroke="currentColor" strokeWidth="1.2" strokeLinecap="round" />
        </svg>
      </button>

      {/* Timeline dots */}
      <div className="flex items-center gap-0.5 flex-1 overflow-x-auto scrollbar-thin py-1">
        {past.map((entry, i) => (
          <Tooltip key={`past-${i}`} content={entry.description} position="top">
            <button
              onClick={() => handleJump(i)}
              className="w-2 h-2 rounded-full bg-accent-teal/40 hover:bg-accent-teal/70 transition-colors flex-shrink-0"
              aria-label={`Undo to: ${entry.description}`}
            />
          </Tooltip>
        ))}

        {/* Current position indicator */}
        <span
          className="w-2.5 h-2.5 rounded-full flex-shrink-0 progress-gradient ring-1 ring-accent-teal/30"
          title="Current state"
          aria-label="Current position"
        />

        {future.map((entry, i) => (
          <Tooltip key={`future-${i}`} content={entry.description} position="top">
            <button
              onClick={() => handleJump(past.length + 1 + i)}
              className="w-2 h-2 rounded-full bg-accent-plum/30 hover:bg-accent-plum/60 transition-colors flex-shrink-0"
              aria-label={`Redo to: ${entry.description}`}
            />
          </Tooltip>
        ))}
      </div>

      {/* Redo button */}
      <button
        onClick={() => {
          const currentPads = usePadStore.getState().pads;
          const result = redo(currentPads);
          if (result) setPads(result);
        }}
        disabled={future.length === 0}
        className="p-1 rounded text-text-muted hover:text-text-primary disabled:opacity-30 disabled:pointer-events-none transition-colors"
        aria-label="Redo"
        title="Redo (⌘⇧Z)"
      >
        <svg width="12" height="12" viewBox="0 0 12 12" fill="none">
          <path d="M9 4l2 2-2 2" stroke="currentColor" strokeWidth="1.2" strokeLinecap="round" strokeLinejoin="round" />
          <path d="M11 6H4a3 3 0 000 6h2" stroke="currentColor" strokeWidth="1.2" strokeLinecap="round" />
        </svg>
      </button>
    </div>
  );
}
