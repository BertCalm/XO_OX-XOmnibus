import { create } from 'zustand';
import type { PadAssignment, PadLayer, LayerPlayMode, TriggerMode } from '@/types';
import { LAYER_DEFAULTS, MAX_LAYERS_PER_PAD, MAX_PADS } from '@/constants/mpcDefaults';
import { parseFilenameForMapping } from '@/lib/audio/filenameParser';
import { useHistoryStore } from '@/stores/historyStore';
import { useEnvelopeStore } from '@/stores/envelopeStore';
import { useModulationStore } from '@/stores/modulationStore';
import { useExpressionStore } from '@/stores/expressionStore';

/** Selection mode for multi-pad operations */
export type PadSelectMode = 'single' | 'add' | 'range';

type LayerNumber = PadLayer['number'];

function createDefaultLayer(number: LayerNumber): PadLayer {
  return {
    number,
    active: false,
    sampleId: null,
    sampleName: '',
    sampleFile: '',
    volume: LAYER_DEFAULTS.volume,
    pan: LAYER_DEFAULTS.pan,
    pitch: LAYER_DEFAULTS.pitch,
    tuneCoarse: LAYER_DEFAULTS.tuneCoarse,
    tuneFine: LAYER_DEFAULTS.tuneFine,
    velStart: LAYER_DEFAULTS.velStart,
    velEnd: LAYER_DEFAULTS.velEnd,
    rootNote: LAYER_DEFAULTS.rootNote,
    keyTrack: LAYER_DEFAULTS.keyTrack,
    pitchRandom: LAYER_DEFAULTS.pitchRandom,
    volumeRandom: LAYER_DEFAULTS.volumeRandom,
    panRandom: LAYER_DEFAULTS.panRandom,
    sliceStart: LAYER_DEFAULTS.sliceStart,
    sliceEnd: LAYER_DEFAULTS.sliceEnd,
    sliceLoop: LAYER_DEFAULTS.sliceLoop,
    sliceLoopStart: LAYER_DEFAULTS.sliceLoopStart,
    offset: LAYER_DEFAULTS.offset,
    direction: LAYER_DEFAULTS.direction,
    probability: LAYER_DEFAULTS.probability,
  };
}

function createDefaultPad(padNumber: number): PadAssignment {
  return {
    padNumber,
    layers: Array.from({ length: MAX_LAYERS_PER_PAD }, (_, i) =>
      createDefaultLayer((i + 1) as LayerNumber)
    ),
    playMode: 'simultaneous',
    triggerMode: 'oneshot',
    muteGroup: 0,
  };
}

/**
 * Reset per-pad data in envelope, modulation, and expression stores.
 * Called when a pad is cleared or needs its auxiliary state removed.
 */
function _resetPerPadStores(padIndex: number): void {
  const envState = useEnvelopeStore.getState();
  const nextEnv = { ...envState.padEnvelopes };
  delete nextEnv[padIndex];
  useEnvelopeStore.setState({ padEnvelopes: nextEnv });

  const modState = useModulationStore.getState();
  const nextMod = { ...modState.padModulation };
  delete nextMod[padIndex];
  useModulationStore.setState({ padModulation: nextMod });

  const expState = useExpressionStore.getState();
  const nextExp = { ...expState.padExpressions };
  delete nextExp[padIndex];
  useExpressionStore.setState({ padExpressions: nextExp });
}

/**
 * Swap per-pad data between two indices in envelope, modulation, and expression stores.
 */
function _swapPerPadStores(padA: number, padB: number): void {
  const envState = useEnvelopeStore.getState();
  const nextEnv = { ...envState.padEnvelopes };
  const envA = nextEnv[padA];
  const envB = nextEnv[padB];
  if (envA) nextEnv[padB] = _deepCloneEnvelope(envA); else delete nextEnv[padB];
  if (envB) nextEnv[padA] = _deepCloneEnvelope(envB); else delete nextEnv[padA];
  useEnvelopeStore.setState({ padEnvelopes: nextEnv });

  const modState = useModulationStore.getState();
  const nextMod = { ...modState.padModulation };
  const modA = nextMod[padA];
  const modB = nextMod[padB];
  if (modA) nextMod[padB] = _deepCloneModulation(modA); else delete nextMod[padB];
  if (modB) nextMod[padA] = _deepCloneModulation(modB); else delete nextMod[padA];
  useModulationStore.setState({ padModulation: nextMod });

  const expState = useExpressionStore.getState();
  const nextExp = { ...expState.padExpressions };
  const expA = nextExp[padA];
  const expB = nextExp[padB];
  if (expA) nextExp[padB] = { ...expA }; else delete nextExp[padB];
  if (expB) nextExp[padA] = { ...expB }; else delete nextExp[padA];
  useExpressionStore.setState({ padExpressions: nextExp });
}

/**
 * Copy per-pad data from source index to target index in envelope, modulation, and expression stores.
 * Used by pastePad to transfer auxiliary state alongside pad assignment data.
 */
function _deepCloneEnvelope(src: ReturnType<typeof useEnvelopeStore.getState>['padEnvelopes'][number]) {
  return {
    ...src,
    volumeEnvelope: { ...src.volumeEnvelope },
    filterEnvelope: { ...src.filterEnvelope },
  };
}

function _deepCloneModulation(src: ReturnType<typeof useModulationStore.getState>['padModulation'][number]) {
  return {
    ...src,
    routes: src.routes.map((r) => ({ ...r })),
    lfo1: src.lfo1 ? { ...src.lfo1 } : undefined,
  };
}

function _copyPerPadStores(srcIdx: number, dstIdx: number): void {
  const envState = useEnvelopeStore.getState();
  const nextEnv = { ...envState.padEnvelopes };
  const envSrc = envState.padEnvelopes[srcIdx];
  if (envSrc) nextEnv[dstIdx] = _deepCloneEnvelope(envSrc); else delete nextEnv[dstIdx];
  useEnvelopeStore.setState({ padEnvelopes: nextEnv });

  const modState = useModulationStore.getState();
  const nextMod = { ...modState.padModulation };
  const modSrc = modState.padModulation[srcIdx];
  if (modSrc) nextMod[dstIdx] = _deepCloneModulation(modSrc); else delete nextMod[dstIdx];
  useModulationStore.setState({ padModulation: nextMod });

  const expState = useExpressionStore.getState();
  const nextExp = { ...expState.padExpressions };
  const expSrc = expState.padExpressions[srcIdx];
  if (expSrc) nextExp[dstIdx] = { ...expSrc }; else delete nextExp[dstIdx];
  useExpressionStore.setState({ padExpressions: nextExp });
}

/**
 * Module-level snapshot of per-pad store data captured at copy time.
 * Prevents stale paste when envelope/modulation/expression stores
 * are modified between copy and paste operations.
 */
let _copiedPadStoreSnapshot: {
  envelope?: ReturnType<typeof useEnvelopeStore.getState>['padEnvelopes'][number];
  modulation?: ReturnType<typeof useModulationStore.getState>['padModulation'][number];
  expression?: ReturnType<typeof useExpressionStore.getState>['padExpressions'][number];
} | null = null;

interface PadState {
  pads: PadAssignment[];
  activePadIndex: number;
  activeLayerIndex: number;
  currentBank: number;
  copiedPad: PadAssignment | null;
  /** Source pad index for clipboard — used to copy per-pad store data on paste */
  copiedPadIndex: number | null;

  /** Indices of currently multi-selected pads (for batch operations) */
  selectedPadIndices: Set<number>;

  initializePads: (count: number) => void;
  setActivePad: (index: number) => void;
  setActiveLayer: (index: number) => void;
  setCurrentBank: (bank: number) => void;

  /** Select a pad in single, add (Cmd+click), or range (Shift+click) mode */
  selectPad: (index: number, mode: PadSelectMode) => void;
  /** Clear all multi-pad selections */
  clearSelection: () => void;
  /** Apply a MIDI note map to all pads (from pad map templates) */
  applyPadMap: (noteMap: number[]) => void;

  assignSampleToLayer: (
    padIndex: number,
    layerIndex: number,
    sampleId: string,
    sampleName: string,
    sampleFile: string,
    rootNote?: number
  ) => void;
  removeLayerSample: (padIndex: number, layerIndex: number) => void;
  updateLayer: (padIndex: number, layerIndex: number, updates: Partial<PadLayer>) => void;
  updatePad: (padIndex: number, updates: Partial<PadAssignment>) => void;
  setPadPlayMode: (padIndex: number, mode: LayerPlayMode) => void;
  setPadTriggerMode: (padIndex: number, mode: TriggerMode) => void;
  copyPad: (padIndex: number) => void;
  pastePad: (padIndex: number) => void;
  swapPads: (padA: number, padB: number) => void;
  clearPad: (padIndex: number) => void;

  /** Clear all layer references to a deleted sample (called by audioStore.removeSample) */
  clearSampleRefs: (sampleId: string) => void;

  /** Replace the entire pads array (used by undo/redo) */
  setPads: (pads: PadAssignment[]) => void;

  /**
   * Wraps a synchronous pad mutation with undo history recording.
   * Takes a snapshot before fn(), runs fn(), then pushes the new state.
   * For async mutations, use historyStore.snapshot/pushState directly.
   */
  withHistory: (description: string, fn: () => void) => void;
}

export const usePadStore = create<PadState>((set, get) => ({
  pads: Array.from({ length: MAX_PADS }, (_, i) => createDefaultPad(i)),
  activePadIndex: 0,
  activeLayerIndex: 0,
  currentBank: 0,
  copiedPad: null,
  copiedPadIndex: null,
  selectedPadIndices: new Set<number>(),

  initializePads: (count) => {
    set({
      pads: Array.from({ length: count }, (_, i) => createDefaultPad(i)),
      activePadIndex: 0,
      activeLayerIndex: 0,
      selectedPadIndices: new Set<number>(),
    });
    // Clear per-pad stores so stale data from a previous project doesn't leak
    useEnvelopeStore.getState().clearAll();
    useModulationStore.getState().clearAll();
    useExpressionStore.getState().clearAll();
  },

  selectPad: (index, mode) =>
    set((state) => {
      if (index < 0 || index >= state.pads.length) return {};
      const next = new Set(mode === 'single' ? [] : state.selectedPadIndices);

      if (mode === 'add') {
        // Toggle: Cmd+click adds or removes from selection
        if (next.has(index)) {
          next.delete(index);
        } else {
          next.add(index);
        }
      } else if (mode === 'range') {
        // Shift+click: select range from activePadIndex to clicked index
        const start = Math.min(state.activePadIndex, index);
        const end = Math.max(state.activePadIndex, index);
        for (let i = start; i <= end; i++) {
          next.add(i);
        }
      } else {
        // Single: replace selection with just this pad
        next.add(index);
      }

      return { selectedPadIndices: next };
    }),

  clearSelection: () => set({ selectedPadIndices: new Set<number>() }),

  applyPadMap: (noteMap) =>
    set((state) => {
      const pads = state.pads.map((pad, i) => {
        const note = i < noteMap.length ? noteMap[i] : i;
        // Only update if the note actually changed
        if (pad.padNumber === note) return pad;
        return { ...pad, padNumber: note };
      });
      return { pads };
    }),

  setActivePad: (index) => set((state) => {
    if (index < 0 || index >= state.pads.length) return {};
    return { activePadIndex: index };
  }),
  setActiveLayer: (index) => set(() => {
    if (index < 0 || index >= MAX_LAYERS_PER_PAD) return {};
    return { activeLayerIndex: index };
  }),
  setCurrentBank: (bank) => set(() => {
    if (bank < 0 || bank >= MAX_PADS / 16) return {};
    return { currentBank: bank };
  }),

  assignSampleToLayer: (padIndex, layerIndex, sampleId, sampleName, sampleFile, rootNote) =>
    set((state) => {
      if (padIndex < 0 || padIndex >= state.pads.length) return {};
      if (layerIndex < 0 || layerIndex >= MAX_LAYERS_PER_PAD) return {};
      const pads = [...state.pads];
      const pad = { ...pads[padIndex] };
      const layers = [...pad.layers];

      // Auto-detect root note from filename if not explicitly provided
      let resolvedRootNote = rootNote ?? null;
      if (resolvedRootNote === null) {
        const parsed = parseFilenameForMapping(sampleFile || sampleName);
        resolvedRootNote = parsed.rootNote;
      }

      layers[layerIndex] = {
        ...layers[layerIndex],
        active: true,
        sampleId,
        sampleName,
        sampleFile,
        rootNote: resolvedRootNote ?? layers[layerIndex].rootNote,
      };
      pad.layers = layers;
      pads[padIndex] = pad;
      return { pads };
    }),

  removeLayerSample: (padIndex, layerIndex) =>
    set((state) => {
      if (padIndex < 0 || padIndex >= state.pads.length) return {};
      if (layerIndex < 0 || layerIndex >= MAX_LAYERS_PER_PAD) return {};
      const pads = [...state.pads];
      const pad = { ...pads[padIndex] };
      const layers = [...pad.layers];
      layers[layerIndex] = createDefaultLayer((layerIndex + 1) as LayerNumber);
      pad.layers = layers;
      pads[padIndex] = pad;
      return { pads };
    }),

  updateLayer: (padIndex, layerIndex, updates) =>
    set((state) => {
      if (padIndex < 0 || padIndex >= state.pads.length) return {};
      if (layerIndex < 0 || layerIndex >= MAX_LAYERS_PER_PAD) return {};
      const pads = [...state.pads];
      const pad = { ...pads[padIndex] };
      const layers = [...pad.layers];
      layers[layerIndex] = { ...layers[layerIndex], ...updates };
      pad.layers = layers;
      pads[padIndex] = pad;
      return { pads };
    }),

  updatePad: (padIndex, updates) =>
    set((state) => {
      if (padIndex < 0 || padIndex >= state.pads.length) return {};
      const pads = [...state.pads];
      pads[padIndex] = { ...pads[padIndex], ...updates };
      return { pads };
    }),

  setPadPlayMode: (padIndex, mode) =>
    set((state) => {
      if (padIndex < 0 || padIndex >= state.pads.length) return {};
      const pads = [...state.pads];
      pads[padIndex] = { ...pads[padIndex], playMode: mode };
      return { pads };
    }),

  setPadTriggerMode: (padIndex, mode) =>
    set((state) => {
      if (padIndex < 0 || padIndex >= state.pads.length) return {};
      const pads = [...state.pads];
      pads[padIndex] = { ...pads[padIndex], triggerMode: mode };
      return { pads };
    }),

  copyPad: (padIndex) => {
    // Bounds check BEFORE snapshotting auxiliary stores — out-of-bounds
    // padIndex would write undefined data that corrupts the paste target.
    const currentPads = get().pads;
    if (padIndex < 0 || padIndex >= currentPads.length) return;

    // Snapshot per-pad store data at copy time — prevents stale paste
    // when envelope/modulation/expression stores change between copy and paste.
    const envSrc = useEnvelopeStore.getState().padEnvelopes[padIndex];
    const modSrc = useModulationStore.getState().padModulation[padIndex];
    const expSrc = useExpressionStore.getState().padExpressions[padIndex];
    _copiedPadStoreSnapshot = {
      envelope: envSrc ? _deepCloneEnvelope(envSrc) : undefined,
      modulation: modSrc ? _deepCloneModulation(modSrc) : undefined,
      expression: expSrc ? { ...expSrc } : undefined,
    };

    set((state) => {
      if (padIndex < 0 || padIndex >= state.pads.length) return {};
      const source = state.pads[padIndex];
      const copiedPad: PadAssignment = {
        ...source,
        layers: source.layers.map((layer) => ({ ...layer })),
      };
      return { copiedPad, copiedPadIndex: padIndex };
    });
  },

  pastePad: (padIndex) => {
    const { copiedPad } = get();
    if (!copiedPad) return;
    set((state) => {
      if (padIndex < 0 || padIndex >= state.pads.length) return {};
      const pads = [...state.pads];
      const targetPadNumber = pads[padIndex].padNumber;
      pads[padIndex] = {
        ...copiedPad,
        padNumber: targetPadNumber,
        layers: copiedPad.layers.map((layer) => ({ ...layer })),
      };
      return { pads };
    });
    // Restore per-pad stores from the snapshot captured at copy time
    // (not from live state, which may have changed since copy).
    if (_copiedPadStoreSnapshot) {
      const snap = _copiedPadStoreSnapshot;
      const nextEnv = { ...useEnvelopeStore.getState().padEnvelopes };
      if (snap.envelope) nextEnv[padIndex] = _deepCloneEnvelope(snap.envelope);
      else delete nextEnv[padIndex];
      useEnvelopeStore.setState({ padEnvelopes: nextEnv });

      const nextMod = { ...useModulationStore.getState().padModulation };
      if (snap.modulation) nextMod[padIndex] = _deepCloneModulation(snap.modulation);
      else delete nextMod[padIndex];
      useModulationStore.setState({ padModulation: nextMod });

      const nextExp = { ...useExpressionStore.getState().padExpressions };
      if (snap.expression) nextExp[padIndex] = { ...snap.expression };
      else delete nextExp[padIndex];
      useExpressionStore.setState({ padExpressions: nextExp });
    }
  },

  swapPads: (padA, padB) => {
    set((state) => {
      if (padA < 0 || padA >= state.pads.length) return {};
      if (padB < 0 || padB >= state.pads.length) return {};
      const pads = [...state.pads];
      const padAData = {
        ...pads[padA],
        layers: pads[padA].layers.map((layer) => ({ ...layer })),
      };
      const padBData = {
        ...pads[padB],
        layers: pads[padB].layers.map((layer) => ({ ...layer })),
      };
      // Preserve each position's padNumber
      const padANumber = padAData.padNumber;
      const padBNumber = padBData.padNumber;
      pads[padA] = { ...padBData, padNumber: padANumber };
      pads[padB] = { ...padAData, padNumber: padBNumber };
      return { pads };
    });
    // Also swap per-pad envelope/modulation/expression data
    _swapPerPadStores(padA, padB);
  },

  clearPad: (padIndex) => {
    set((state) => {
      if (padIndex < 0 || padIndex >= state.pads.length) return {};
      const pads = [...state.pads];
      pads[padIndex] = createDefaultPad(pads[padIndex].padNumber);
      return { pads };
    });
    // Clear per-pad stores to prevent ghost envelope/modulation/expression
    // on new content assigned to this pad
    _resetPerPadStores(padIndex);
  },

  clearSampleRefs: (sampleId) => {
    const padsNowEmpty: number[] = [];
    set((state) => {
      let anyChanged = false;
      const pads = state.pads.map((pad, padIndex) => {
        let padChanged = false;
        const layers = pad.layers.map((layer) => {
          if (layer.sampleId === sampleId) {
            padChanged = true;
            return { ...layer, sampleId: null, active: false, sampleName: '', sampleFile: '' };
          }
          return layer;
        });
        if (padChanged) {
          anyChanged = true;
          const updated = { ...pad, layers };
          // Track pads that have no active layers after removal
          const hasAnyActive = layers.some((l) => l.active && l.sampleId);
          if (!hasAnyActive) padsNowEmpty.push(padIndex);
          return updated;
        }
        return pad;
      });
      return anyChanged ? { pads } : {};
    });
    // Clean up per-pad stores for pads that are now fully empty
    for (const idx of padsNowEmpty) {
      _resetPerPadStores(idx);
    }
  },

  setPads: (pads) => set({ pads }),

  withHistory: (description, fn) => {
    const history = useHistoryStore.getState();
    history.snapshot(get().pads);
    // eslint-disable-next-line @typescript-eslint/no-confusing-void-expression
    const result: unknown = fn();
    // Guard: if fn returns a Promise, pushState captures mid-async state.
    // Warn in dev so callers use snapshot/pushState directly for async ops.
    if (result != null && typeof (result as Promise<unknown>).then === 'function') {
      console.warn(
        `withHistory("${description}"): fn returned a Promise. ` +
        'Use historyStore.snapshot/pushState directly for async mutations.'
      );
    }
    history.pushState(description, get().pads);
  },
}));
