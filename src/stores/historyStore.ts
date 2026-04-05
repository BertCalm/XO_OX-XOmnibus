import { create } from 'zustand';
import type { PadAssignment } from '@/types';

/**
 * History uses a diff-based (command) pattern to minimize memory usage.
 * Instead of storing full 128-pad snapshots, we store only the pads that changed.
 */

interface PadDiff {
  index: number;
  pad: PadAssignment; // snapshot of the pad at this index before the change
}

interface HistoryEntry {
  timestamp: number;
  description: string;
  /** Only the changed pads + their indices (sparse diff) */
  diffs: PadDiff[];
}

interface HistoryState {
  past: HistoryEntry[];
  future: HistoryEntry[];
  maxHistory: number;
  /** Snapshot of pads state before current batch of changes (for diff computation) */
  _lastSnapshot: PadAssignment[] | null;

  /** Take a snapshot before making changes. Call this before the first mutation. */
  snapshot: (pads: PadAssignment[]) => void;
  /** Push diffs between the snapshot and current state. Clears redo stack. */
  pushState: (description: string, pads: PadAssignment[]) => void;
  /** Undo: returns patches to apply, or null if nothing to undo. */
  undo: (currentPads: PadAssignment[]) => PadAssignment[] | null;
  /** Redo: returns patches to apply, or null if nothing to redo. */
  redo: (currentPads: PadAssignment[]) => PadAssignment[] | null;
  canUndo: () => boolean;
  canRedo: () => boolean;
  clear: () => void;
}

function clonePad(pad: PadAssignment): PadAssignment {
  return { ...pad, layers: pad.layers.map((l) => ({ ...l })) };
}

function computeDiffs(before: PadAssignment[], after: PadAssignment[]): PadDiff[] {
  const diffs: PadDiff[] = [];
  const len = Math.max(before.length, after.length);
  for (let i = 0; i < len; i++) {
    // Quick reference check: if same object, skip
    if (before[i] === after[i]) continue;
    // Store the before state for any changed index. For added pads
    // (before[i] undefined), store a sentinel empty pad so undo can
    // restore the "absent" state.
    const padBefore = before[i] ?? {
      padNumber: i,
      layers: [],
      playMode: 'simultaneous' as const,
      triggerMode: 'oneshot' as const,
      muteGroup: 0,
    };
    diffs.push({ index: i, pad: clonePad(padBefore) });
  }
  return diffs;
}

function applyDiffs(pads: PadAssignment[], diffs: PadDiff[]): PadAssignment[] {
  const result = [...pads];
  for (const diff of diffs) {
    result[diff.index] = clonePad(diff.pad);
  }
  return result;
}

export const useHistoryStore = create<HistoryState>((set, get) => ({
  past: [],
  future: [],
  maxHistory: 50,
  _lastSnapshot: null,

  snapshot: (pads) => {
    // Deep-clone the snapshot so subsequent in-place mutations to the original
    // array don't corrupt the history baseline (Zustand stores without immer
    // share object references).
    set({ _lastSnapshot: pads.map(clonePad) });
  },

  pushState: (description, pads) => {
    set((state) => {
      const before = state._lastSnapshot ?? pads;
      const diffs = computeDiffs(before, pads);

      // If nothing changed, don't push
      if (diffs.length === 0) return { _lastSnapshot: null };

      const entry: HistoryEntry = {
        timestamp: Date.now(),
        description,
        diffs,
      };

      const newPast = [...state.past, entry];
      if (newPast.length > state.maxHistory) {
        newPast.splice(0, newPast.length - state.maxHistory);
      }

      return { past: newPast, future: [], _lastSnapshot: null };
    });
  },

  undo: (currentPads) => {
    const state = get();
    if (state.past.length === 0) return null;

    const entry = state.past[state.past.length - 1];

    // Compute forward diffs (current → before) for the redo stack
    // Guard: skip diffs referencing indices beyond the current pads array
    const forwardDiffs: PadDiff[] = entry.diffs
      .filter((d) => d.index < currentPads.length && currentPads[d.index] != null)
      .map((d) => ({
        index: d.index,
        pad: clonePad(currentPads[d.index]),
      }));

    const redoEntry: HistoryEntry = {
      timestamp: Date.now(),
      description: entry.description,
      diffs: forwardDiffs,
    };

    set({
      past: state.past.slice(0, -1),
      future: [...state.future, redoEntry],
    });

    return applyDiffs(currentPads, entry.diffs);
  },

  redo: (currentPads) => {
    const state = get();
    if (state.future.length === 0) return null;

    const entry = state.future[state.future.length - 1];

    // Compute backward diffs for the undo stack
    // Guard: skip diffs referencing indices beyond the current pads array
    const backwardDiffs: PadDiff[] = entry.diffs
      .filter((d) => d.index < currentPads.length && currentPads[d.index] != null)
      .map((d) => ({
        index: d.index,
        pad: clonePad(currentPads[d.index]),
      }));

    const undoEntry: HistoryEntry = {
      timestamp: Date.now(),
      description: entry.description,
      diffs: backwardDiffs,
    };

    set({
      future: state.future.slice(0, -1),
      past: [...state.past, undoEntry],
    });

    return applyDiffs(currentPads, entry.diffs);
  },

  canUndo: () => get().past.length > 0,
  canRedo: () => get().future.length > 0,

  clear: () => set({ past: [], future: [], _lastSnapshot: null }),
}));
