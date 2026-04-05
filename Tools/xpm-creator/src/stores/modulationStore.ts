import { create } from 'zustand';
import type { XpmModulationConfig, XpmModulationRoute } from '@/lib/xpm/xpmTypes';
import { MODULATION_PRESETS } from '@/lib/xpm/modulationPresets';

// ---------------------------------------------------------------------------
// Defaults
// ---------------------------------------------------------------------------

const DEFAULT_MODULATION: XpmModulationConfig = {
  routes: [],
};

// ---------------------------------------------------------------------------
// Store
// ---------------------------------------------------------------------------

interface ModulationState {
  padModulation: Record<number, XpmModulationConfig>;

  /** Get modulation config for a pad (returns empty routes if none set). */
  getModulation: (padIndex: number) => XpmModulationConfig;

  /** Set full modulation config for a pad. */
  setModulation: (padIndex: number, config: XpmModulationConfig) => void;

  /** Add a single route to a pad's modulation config. */
  addRoute: (padIndex: number, route: XpmModulationRoute) => void;

  /** Remove a route by index from a pad's modulation config. */
  removeRoute: (padIndex: number, routeIndex: number) => void;

  /** Apply a modulation preset (by id) to a pad. */
  applyPreset: (padIndex: number, presetId: string) => void;

  /** Bulk-set modulation for multiple pads (e.g. from kit import). */
  bulkSetModulation: (updates: Record<number, XpmModulationConfig>) => void;

  /** Reset a pad's modulation to defaults. */
  resetModulation: (padIndex: number) => void;

  /** Clear all pad modulations. */
  clearAll: () => void;
}

export const useModulationStore = create<ModulationState>((set, get) => ({
  padModulation: {},

  getModulation: (padIndex: number): XpmModulationConfig => {
    const existing = get().padModulation[padIndex];
    if (existing) return existing;
    return { ...DEFAULT_MODULATION, routes: [] };
  },

  setModulation: (padIndex, config) =>
    set((state) => ({
      padModulation: {
        ...state.padModulation,
        [padIndex]: { ...config, routes: config.routes.map((r) => ({ ...r })) },
      },
    })),

  addRoute: (padIndex, route) =>
    set((state) => {
      const current = state.padModulation[padIndex] ?? { ...DEFAULT_MODULATION, routes: [] };
      return {
        padModulation: {
          ...state.padModulation,
          [padIndex]: {
            ...current,
            routes: [...current.routes, { ...route }],
          },
        },
      };
    }),

  removeRoute: (padIndex, routeIndex) =>
    set((state) => {
      const current = state.padModulation[padIndex];
      if (!current) return {};
      return {
        padModulation: {
          ...state.padModulation,
          [padIndex]: {
            ...current,
            routes: current.routes.filter((_, i) => i !== routeIndex),
          },
        },
      };
    }),

  applyPreset: (padIndex, presetId) => {
    const preset = MODULATION_PRESETS.find((p) => p.id === presetId);
    if (!preset) return;

    set((state) => ({
      padModulation: {
        ...state.padModulation,
        [padIndex]: {
          routes: preset.config.routes.map((r) => ({ ...r })),
          lfo1: preset.config.lfo1 ? { ...preset.config.lfo1 } : undefined,
        },
      },
    }));
  },

  bulkSetModulation: (updates) =>
    set((state) => ({
      padModulation: {
        ...state.padModulation,
        ...Object.fromEntries(
          Object.entries(updates).map(([key, config]) => [
            key,
            {
              routes: config.routes.map((r) => ({ ...r })),
              lfo1: config.lfo1 ? { ...config.lfo1 } : undefined,
            },
          ]),
        ),
      },
    })),

  resetModulation: (padIndex) =>
    set((state) => {
      const { [padIndex]: _, ...rest } = state.padModulation;
      return { padModulation: rest };
    }),

  clearAll: () => set({ padModulation: {} }),
}));
