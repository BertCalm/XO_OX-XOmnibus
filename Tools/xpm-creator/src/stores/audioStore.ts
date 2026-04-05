import { create } from 'zustand';
import type { AudioSample } from '@/types';
import { parseFilenameForMapping } from '@/lib/audio/filenameParser';
import { usePadStore } from '@/stores/padStore';
import { deleteSample as deleteDbSample } from '@/lib/storage/db';
import { useToastStore } from '@/stores/toastStore';

interface AudioState {
  samples: AudioSample[];
  activeSampleId: string | null;
  isProcessing: boolean;
  processingMessage: string;
  /** Track sample IDs that have been modified since last persist to IndexedDB */
  dirtySampleIds: Set<string>;
  /** True while a batch operation is running — suppresses auto-save to avoid
   *  thrashing IndexedDB with partial writes during long loops. */
  isBatchProcessing: boolean;

  addSample: (sample: AudioSample) => void;
  removeSample: (id: string) => void;
  renameSample: (id: string, name: string) => void;
  setActiveSample: (id: string | null) => void;
  setSamples: (samples: AudioSample[]) => void;
  setIsBatchProcessing: (v: boolean) => void;

  updateSample: (id: string, updates: Partial<AudioSample>) => void;
  /** Get and clear the dirty sample IDs set (called by auto-save after persisting) */
  consumeDirtySampleIds: () => string[];
  /** Re-mark sample IDs as dirty without mutating the samples array (avoids re-render loops) */
  reMarkDirty: (ids: string[]) => void;

  /** Toggle the isFavorite flag on a sample */
  toggleFavorite: (id: string) => void;
  /** Add a tag to a sample (no-op if already present) */
  addTag: (id: string, tag: string) => void;
  /** Remove a tag from a sample */
  removeTag: (id: string, tag: string) => void;

  setProcessing: (isProcessing: boolean, message?: string) => void;
}

export const useAudioStore = create<AudioState>((set, get) => ({
  samples: [],
  activeSampleId: null,
  isProcessing: false,
  processingMessage: '',
  dirtySampleIds: new Set<string>(),
  isBatchProcessing: false,

  addSample: (sample) =>
    set((state) => {
      // Ensure tags/isFavorite defaults are set for samples created without them
      let enrichedSample: AudioSample = {
        ...sample,
        tags: sample.tags ?? [],
        isFavorite: sample.isFavorite ?? false,
      };
      // Auto-detect root note from filename if still at default (60)
      if (enrichedSample.rootNote === 60) {
        const parsed = parseFilenameForMapping(enrichedSample.fileName || enrichedSample.name);
        if (parsed.rootNote !== null) {
          enrichedSample = { ...enrichedSample, rootNote: parsed.rootNote };
        }
      }
      const newDirty = new Set(state.dirtySampleIds);
      newDirty.add(enrichedSample.id);
      return { samples: [...state.samples, enrichedSample], dirtySampleIds: newDirty };
    }),

  removeSample: (id) => {
    // Clean up dangling sampleId references in pad layers FIRST —
    // if done after removal, a race window exists where pads reference
    // a sample that no longer exists in audioStore.
    usePadStore.getState().clearSampleRefs(id);
    set((state) => {
      const newDirty = new Set(state.dirtySampleIds);
      newDirty.delete(id); // Remove ghost dirty ID for deleted sample
      return {
        samples: state.samples.filter((s) => s.id !== id),
        activeSampleId: state.activeSampleId === id ? null : state.activeSampleId,
        dirtySampleIds: newDirty,
      };
    });
    // Delete from IndexedDB — fire-and-forget so UI isn't blocked.
    // If this fails, the sample persists in IDB as an orphan but is
    // harmless (it won't be loaded because no project references it).
    deleteDbSample(id).catch((err) => {
      console.warn('Failed to delete sample from IndexedDB:', id, err);
      useToastStore.getState().addToast({
        type: 'warning',
        title: 'Sample cleanup failed',
        message: 'The sample file may persist in storage. Clear browser data if space is low.',
      });
    });
  },

  renameSample: (id, name) =>
    set((state) => {
      const newDirty = new Set(state.dirtySampleIds);
      newDirty.add(id);
      return {
        samples: state.samples.map((s) => (s.id === id ? { ...s, name } : s)),
        dirtySampleIds: newDirty,
      };
    }),

  setActiveSample: (id) => set({ activeSampleId: id }),
  setSamples: (samples) => set({ samples }),

  updateSample: (id, updates) =>
    set((state) => {
      const newDirty = new Set(state.dirtySampleIds);
      newDirty.add(id);
      return {
        samples: state.samples.map((s) => (s.id === id ? { ...s, ...updates } : s)),
        dirtySampleIds: newDirty,
      };
    }),

  consumeDirtySampleIds: () => {
    const ids = Array.from(get().dirtySampleIds);
    if (ids.length > 0) {
      set({ dirtySampleIds: new Set<string>() });
    }
    return ids;
  },

  reMarkDirty: (ids) => {
    if (ids.length === 0) return;
    set((state) => {
      const newDirty = new Set(state.dirtySampleIds);
      for (const id of ids) newDirty.add(id);
      // Only update dirtySampleIds — do NOT touch samples array to avoid re-render loops
      return { dirtySampleIds: newDirty };
    });
  },

  toggleFavorite: (id) =>
    set((state) => {
      const newDirty = new Set(state.dirtySampleIds);
      newDirty.add(id);
      return {
        samples: state.samples.map((s) =>
          s.id === id ? { ...s, isFavorite: !(s.isFavorite ?? false) } : s
        ),
        dirtySampleIds: newDirty,
      };
    }),

  addTag: (id, tag) =>
    set((state) => {
      const newDirty = new Set(state.dirtySampleIds);
      newDirty.add(id);
      return {
        samples: state.samples.map((s) => {
          if (s.id !== id) return s;
          const existing = s.tags ?? [];
          if (existing.includes(tag)) return s;
          return { ...s, tags: [...existing, tag] };
        }),
        dirtySampleIds: newDirty,
      };
    }),

  removeTag: (id, tag) =>
    set((state) => {
      const newDirty = new Set(state.dirtySampleIds);
      newDirty.add(id);
      return {
        samples: state.samples.map((s) => {
          if (s.id !== id) return s;
          const existing = s.tags ?? [];
          return { ...s, tags: existing.filter((t) => t !== tag) };
        }),
        dirtySampleIds: newDirty,
      };
    }),

  setProcessing: (isProcessing, message = '') =>
    set({ isProcessing, processingMessage: message }),

  setIsBatchProcessing: (v) => set({ isBatchProcessing: v }),
}));
