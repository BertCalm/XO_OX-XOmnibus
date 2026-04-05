import { create } from 'zustand';
import type { InstrumentEnvelope } from '@/types';
import { XPM_DEFAULTS } from '@/constants/mpcDefaults';

export interface PadEnvelopeSettings {
  volumeEnvelope: InstrumentEnvelope;
  filterType: number; // 0=off, 1=lowpass, 2=bandpass, 3=highpass
  filterCutoff: number; // 0-1
  filterResonance: number; // 0-1
  filterEnvAmount: number; // -1 to +1 (bipolar: negative = inverted sweep)
  filterEnvelope: InstrumentEnvelope;
}

const DEFAULT_ENVELOPE: InstrumentEnvelope = {
  attack: XPM_DEFAULTS.volumeAttack,
  hold: XPM_DEFAULTS.volumeHold,
  decay: XPM_DEFAULTS.volumeDecay,
  sustain: XPM_DEFAULTS.volumeSustain,
  release: XPM_DEFAULTS.volumeRelease,
};

// Freeze the default so callers can't mutate the shared object —
// mutations here would corrupt the return value for ALL unset pads.
const DEFAULT_PAD_ENVELOPE_SETTINGS: Readonly<PadEnvelopeSettings> = Object.freeze({
  volumeEnvelope: Object.freeze({ ...DEFAULT_ENVELOPE }),
  filterType: XPM_DEFAULTS.filterType,
  filterCutoff: XPM_DEFAULTS.filterCutoff,
  filterResonance: XPM_DEFAULTS.filterResonance,
  filterEnvAmount: XPM_DEFAULTS.filterEnvAmt,
  filterEnvelope: Object.freeze({ ...DEFAULT_ENVELOPE }),
});

interface EnvelopeState {
  padEnvelopes: Record<number, PadEnvelopeSettings>;

  getEnvelope: (padIndex: number) => PadEnvelopeSettings;
  setVolumeEnvelope: (padIndex: number, envelope: InstrumentEnvelope) => void;
  setFilterEnvelope: (padIndex: number, envelope: InstrumentEnvelope) => void;
  setFilterSettings: (padIndex: number, updates: Partial<Pick<PadEnvelopeSettings, 'filterType' | 'filterCutoff' | 'filterResonance' | 'filterEnvAmount'>>) => void;
  resetEnvelope: (padIndex: number) => void;
  /** Clear all envelopes — used when loading a new project to prevent stale data from the previous project */
  clearAll: () => void;
}

export const useEnvelopeStore = create<EnvelopeState>((set, get) => ({
  padEnvelopes: {},

  /** Returns the envelope for a pad, or a stable frozen default if none is set. */
  getEnvelope: (padIndex: number): PadEnvelopeSettings => {
    return get().padEnvelopes[padIndex] ?? DEFAULT_PAD_ENVELOPE_SETTINGS;
  },

  setVolumeEnvelope: (padIndex, envelope) =>
    set((state) => {
      const current = state.padEnvelopes[padIndex] || { ...DEFAULT_PAD_ENVELOPE_SETTINGS };
      return {
        padEnvelopes: {
          ...state.padEnvelopes,
          [padIndex]: { ...current, volumeEnvelope: { ...envelope } },
        },
      };
    }),

  setFilterEnvelope: (padIndex, envelope) =>
    set((state) => {
      const current = state.padEnvelopes[padIndex] || { ...DEFAULT_PAD_ENVELOPE_SETTINGS };
      return {
        padEnvelopes: {
          ...state.padEnvelopes,
          [padIndex]: { ...current, filterEnvelope: { ...envelope } },
        },
      };
    }),

  setFilterSettings: (padIndex, updates) =>
    set((state) => {
      const current = state.padEnvelopes[padIndex] || { ...DEFAULT_PAD_ENVELOPE_SETTINGS };
      return {
        padEnvelopes: {
          ...state.padEnvelopes,
          [padIndex]: { ...current, ...updates },
        },
      };
    }),

  resetEnvelope: (padIndex) =>
    set((state) => {
      const { [padIndex]: _, ...rest } = state.padEnvelopes;
      return { padEnvelopes: rest };
    }),

  clearAll: () => set({ padEnvelopes: {} }),
}));
