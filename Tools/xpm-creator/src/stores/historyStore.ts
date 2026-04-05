import { create } from 'zustand';
import type { PadAssignment } from '@/types';
import type { PadEnvelopeSettings } from './envelopeStore';
import type { ExpressionConfig } from '@/lib/audio/expressionEngine';
import type { XpmModulationConfig } from '@/lib/xpm/xpmTypes';

/**
 * History uses a diff-based (command) pattern to minimize memory usage.
 * Instead of storing full 128-pad snapshots, we store only the pads that changed.
 *
 * Auxiliary per-pad stores (envelope, modulation, expression) are also captured
 * alongside pad diffs so that undo/redo restores the full per-pad state atomically.
 */

interface PadDiff {
  index: number;
  pad: PadAssignment; // snapshot of the pad at this index before the change
}

/** Sparse snapshot of auxiliary per-pad state for changed indices only */
interface AuxSnapshot {
  envelopes: Record<number, PadEnvelopeSettings | undefined>;
  modulations: Record<number, XpmModulationConfig | undefined>;
  expressions: Record<number, ExpressionConfig | undefined>;
}

interface HistoryEntry {
  timestamp: number;
  description: string;
  /** Only the changed pads + their indices (sparse diff) */
  diffs: PadDiff[];
  /** Auxiliary store state before the change (for undo) */
  auxBefore?: AuxSnapshot;
  /** Auxiliary store state after the change (for redo) */
  auxAfter?: AuxSnapshot;
  /** Optional callback invoked when this entry is undone (e.g., to clean up imported samples) */
  onUndo?: () => void;
  /** Optional callback invoked when this entry is redone */
  onRedo?: () => void;
}

interface HistoryState {
  past: HistoryEntry[];
  future: HistoryEntry[];
  maxHistory: number;
  /** Snapshot of pads state before current batch of changes (for diff computation) */
  _lastSnapshot: PadAssignment[] | null;
  /** Auxiliary store snapshot captured at snapshot() time */
  _lastAuxSnapshot: AuxSnapshot | null;

  /** Take a snapshot before making changes. Call this before the first mutation. */
  snapshot: (pads: PadAssignment[]) => void;
  /** Push diffs between the snapshot and current state. Clears redo stack. */
  pushState: (description: string, pads: PadAssignment[], callbacks?: { onUndo?: () => void; onRedo?: () => void }) => void;
  /** Undo: returns patches to apply, or null if nothing to undo. */
  undo: (currentPads: PadAssignment[]) => PadAssignment[] | null;
  /** Redo: returns patches to apply, or null if nothing to redo. */
  redo: (currentPads: PadAssignment[]) => PadAssignment[] | null;
  canUndo: () => boolean;
  canRedo: () => boolean;
  clear: () => void;
}

/**
 * Capture a full snapshot of all auxiliary per-pad stores.
 * Called lazily (not at import time) to avoid circular dependency issues.
 */
function captureAuxSnapshot(): AuxSnapshot {
  // Dynamic imports to avoid circular dependencies — these stores
  // are singletons so getState() returns the current values.
  const { useEnvelopeStore } = require('./envelopeStore');
  const { useModulationStore } = require('./modulationStore');
  const { useExpressionStore } = require('./expressionStore');

  return {
    envelopes: structuredClone(useEnvelopeStore.getState().padEnvelopes),
    modulations: structuredClone(useModulationStore.getState().padModulation),
    expressions: structuredClone(useExpressionStore.getState().padExpressions),
  };
}

/**
 * Extract only the entries for specific pad indices from a full aux snapshot.
 */
function sparseAux(full: AuxSnapshot, indices: number[]): AuxSnapshot {
  const envelopes: AuxSnapshot['envelopes'] = {};
  const modulations: AuxSnapshot['modulations'] = {};
  const expressions: AuxSnapshot['expressions'] = {};
  for (const i of indices) {
    envelopes[i] = full.envelopes[i];
    modulations[i] = full.modulations[i];
    expressions[i] = full.expressions[i];
  }
  return { envelopes, modulations, expressions };
}

/**
 * Restore auxiliary stores from a sparse snapshot.
 */
function restoreAuxSnapshot(aux: AuxSnapshot): void {
  const { useEnvelopeStore } = require('./envelopeStore');
  const { useModulationStore } = require('./modulationStore');
  const { useExpressionStore } = require('./expressionStore');

  const envState = { ...useEnvelopeStore.getState().padEnvelopes };
  for (const [key, val] of Object.entries(aux.envelopes)) {
    const idx = Number(key);
    if (val === undefined) {
      delete envState[idx];
    } else {
      envState[idx] = structuredClone(val);
    }
  }
  useEnvelopeStore.setState({ padEnvelopes: envState });

  const modState = { ...useModulationStore.getState().padModulation };
  for (const [key, val] of Object.entries(aux.modulations)) {
    const idx = Number(key);
    if (val === undefined) {
      delete modState[idx];
    } else {
      modState[idx] = structuredClone(val);
    }
  }
  useModulationStore.setState({ padModulation: modState });

  const expState = { ...useExpressionStore.getState().padExpressions };
  for (const [key, val] of Object.entries(aux.expressions)) {
    const idx = Number(key);
    if (val === undefined) {
      delete expState[idx];
    } else {
      expState[idx] = structuredClone(val);
    }
  }
  useExpressionStore.setState({ padExpressions: expState });
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
  _lastAuxSnapshot: null,

  snapshot: (pads) => {
    // Deep-clone the snapshot so subsequent in-place mutations to the original
    // array don't corrupt the history baseline (Zustand stores without immer
    // share object references).
    set({
      _lastSnapshot: pads.map(clonePad),
      _lastAuxSnapshot: captureAuxSnapshot(),
    });
  },

  pushState: (description, pads, callbacks) => {
    // Capture current auxiliary state BEFORE entering set() — the aux stores
    // may have been mutated between snapshot() and pushState() by the caller.
    const auxAfterFull = captureAuxSnapshot();

    set((state) => {
      const before = state._lastSnapshot ?? pads;
      const diffs = computeDiffs(before, pads);

      // If nothing changed (pads), still check if auxiliary stores changed
      if (diffs.length === 0) return { _lastSnapshot: null, _lastAuxSnapshot: null };

      const changedIndices = diffs.map((d) => d.index);
      const auxBefore = state._lastAuxSnapshot
        ? sparseAux(state._lastAuxSnapshot, changedIndices)
        : undefined;
      const auxAfter = sparseAux(auxAfterFull, changedIndices);

      const entry: HistoryEntry = {
        timestamp: Date.now(),
        description,
        diffs,
        auxBefore,
        auxAfter,
        onUndo: callbacks?.onUndo,
        onRedo: callbacks?.onRedo,
      };

      const newPast = [...state.past, entry];
      if (newPast.length > state.maxHistory) {
        newPast.splice(0, newPast.length - state.maxHistory);
      }

      return { past: newPast, future: [], _lastSnapshot: null, _lastAuxSnapshot: null };
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

    // Capture current auxiliary state for redo
    const changedIndices = entry.diffs.map((d) => d.index);
    const currentAux = sparseAux(captureAuxSnapshot(), changedIndices);

    const redoEntry: HistoryEntry = {
      timestamp: Date.now(),
      description: entry.description,
      diffs: forwardDiffs,
      auxBefore: currentAux,
      auxAfter: entry.auxBefore,
      // Swap callbacks: redo of an undone entry should call the original onRedo
      onUndo: entry.onRedo,
      onRedo: entry.onUndo,
    };

    set({
      past: state.past.slice(0, -1),
      future: [...state.future, redoEntry],
    });

    // Restore auxiliary stores to their pre-mutation state
    if (entry.auxBefore) {
      restoreAuxSnapshot(entry.auxBefore);
    }

    // Invoke undo callback (e.g., clean up imported samples)
    entry.onUndo?.();

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

    // Capture current auxiliary state for undo
    const changedIndices = entry.diffs.map((d) => d.index);
    const currentAux = sparseAux(captureAuxSnapshot(), changedIndices);

    const undoEntry: HistoryEntry = {
      timestamp: Date.now(),
      description: entry.description,
      diffs: backwardDiffs,
      auxBefore: currentAux,
      auxAfter: entry.auxAfter,
      // Swap callbacks: undo of a redone entry should call the original onUndo
      onUndo: entry.onRedo,
      onRedo: entry.onUndo,
    };

    set({
      future: state.future.slice(0, -1),
      past: [...state.past, undoEntry],
    });

    // Restore auxiliary stores to their post-mutation state
    if (entry.auxAfter) {
      restoreAuxSnapshot(entry.auxAfter);
    }

    // Invoke redo callback
    entry.onRedo?.();

    return applyDiffs(currentPads, entry.diffs);
  },

  canUndo: () => get().past.length > 0,
  canRedo: () => get().future.length > 0,

  clear: () => set({ past: [], future: [], _lastSnapshot: null, _lastAuxSnapshot: null }),
}));
