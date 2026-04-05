import { create } from 'zustand';
import type { ExpressionConfig, ExpressionMode } from '@/lib/audio/expressionEngine';

// ---------------------------------------------------------------------------
// Defaults
// ---------------------------------------------------------------------------

// Freeze the default to prevent accidental mutation of the shared object
// returned by getExpression() when no pad-specific config exists.
const DEFAULT_EXPRESSION: Readonly<ExpressionConfig> = Object.freeze({
  mode: 'none' as const,
  intensity: 0.5,
  velocityThreshold: 0,
});

// ---------------------------------------------------------------------------
// Store
// ---------------------------------------------------------------------------

interface ExpressionState {
  padExpressions: Record<number, ExpressionConfig>;

  /** Get expression config for a pad (returns defaults if none set). */
  getExpression: (padIndex: number) => ExpressionConfig;

  /** Set full expression config for a pad. */
  setExpression: (padIndex: number, config: ExpressionConfig) => void;

  /** Set just the expression mode for a pad (keeps other settings). */
  setMode: (padIndex: number, mode: ExpressionMode) => void;

  /** Set just the intensity for a pad (keeps other settings). */
  setIntensity: (padIndex: number, intensity: number) => void;

  /** Set just the velocity threshold for a pad. */
  setVelocityThreshold: (padIndex: number, threshold: number) => void;

  /** Bulk-set expressions for multiple pads (e.g. from preset). */
  bulkSetExpressions: (updates: Record<number, ExpressionConfig>) => void;

  /** Reset a pad's expression to defaults. */
  resetExpression: (padIndex: number) => void;

  /** Clear all pad expressions. */
  clearAll: () => void;
}

export const useExpressionStore = create<ExpressionState>((set, get) => ({
  padExpressions: {},

  getExpression: (padIndex: number): ExpressionConfig => {
    // Return the stored reference directly (or the stable frozen default)
    // instead of spreading a new object on every call. This prevents
    // unnecessary allocations in the hot playback path (triggerPad).
    return get().padExpressions[padIndex] ?? DEFAULT_EXPRESSION;
  },

  setExpression: (padIndex, config) =>
    set((state) => ({
      padExpressions: {
        ...state.padExpressions,
        [padIndex]: { ...config },
      },
    })),

  setMode: (padIndex, mode) =>
    set((state) => {
      const current = state.padExpressions[padIndex] ?? { ...DEFAULT_EXPRESSION };
      return {
        padExpressions: {
          ...state.padExpressions,
          [padIndex]: { ...current, mode },
        },
      };
    }),

  setIntensity: (padIndex, intensity) =>
    set((state) => {
      const current = state.padExpressions[padIndex] ?? { ...DEFAULT_EXPRESSION };
      return {
        padExpressions: {
          ...state.padExpressions,
          [padIndex]: { ...current, intensity },
        },
      };
    }),

  setVelocityThreshold: (padIndex, threshold) =>
    set((state) => {
      const current = state.padExpressions[padIndex] ?? { ...DEFAULT_EXPRESSION };
      return {
        padExpressions: {
          ...state.padExpressions,
          [padIndex]: { ...current, velocityThreshold: threshold },
        },
      };
    }),

  bulkSetExpressions: (updates) =>
    set((state) => ({
      padExpressions: {
        ...state.padExpressions,
        ...Object.fromEntries(
          Object.entries(updates).map(([key, config]) => [
            key,
            { ...config },
          ]),
        ),
      },
    })),

  resetExpression: (padIndex) =>
    set((state) => {
      const { [padIndex]: _, ...rest } = state.padExpressions;
      return { padExpressions: rest };
    }),

  clearAll: () => set({ padExpressions: {} }),
}));
