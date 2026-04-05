import { create } from 'zustand';
import { persist } from 'zustand/middleware';
import type { ProgramType } from '@/types';
import type { ExportBitDepth } from './exportStore';

export interface ExportPreset {
  id: string;
  name: string;
  programType: ProgramType;
  bitDepth: ExportBitDepth;
  createdAt: number;
}

interface ExportPresetState {
  presets: ExportPreset[];
  activePresetId: string | null;

  addPreset: (preset: ExportPreset) => void;
  removePreset: (id: string) => void;
  applyPreset: (id: string) => ExportPreset | null;
  clearActivePreset: () => void;
}

export const useExportPresetStore = create<ExportPresetState>()(
  persist(
    (set, get) => ({
      presets: [
        {
          id: 'default-drum-16',
          name: 'Drum 16-bit',
          programType: 'Drum',
          bitDepth: 16,
          createdAt: 0,
        },
        {
          id: 'default-drum-24',
          name: 'Drum 24-bit HQ',
          programType: 'Drum',
          bitDepth: 24,
          createdAt: 0,
        },
        {
          id: 'default-keygroup-16',
          name: 'Keygroup 16-bit',
          programType: 'Keygroup',
          bitDepth: 16,
          createdAt: 0,
        },
        {
          id: 'default-keygroup-24',
          name: 'Keygroup 24-bit HQ',
          programType: 'Keygroup',
          bitDepth: 24,
          createdAt: 0,
        },
      ],
      activePresetId: null,

      addPreset: (preset) =>
        set((state) => ({
          presets: [...state.presets, preset],
          activePresetId: preset.id,
        })),

      removePreset: (id) =>
        set((state) => ({
          presets: state.presets.filter((p) => p.id !== id),
          activePresetId: state.activePresetId === id ? null : state.activePresetId,
        })),

      applyPreset: (id) => {
        const preset = get().presets.find((p) => p.id === id);
        if (preset) {
          set({ activePresetId: id });
        }
        return preset ?? null;
      },

      clearActivePreset: () => set({ activePresetId: null }),
    }),
    {
      name: 'xo-ox-export-presets',
      partialize: (state) => ({
        presets: state.presets,
        // activePresetId is intentionally excluded — per-session choice
      }),
    }
  )
);
