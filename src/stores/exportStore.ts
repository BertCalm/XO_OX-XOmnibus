import { create } from 'zustand';
import type { ExportStep } from '@/types';
import { DEFAULT_BIT_DEPTH } from '@/constants/mpcDefaults';

export type ExportBitDepth = 16 | 24;

interface ExportState {
  isExporting: boolean;
  steps: ExportStep[];
  overallProgress: number;
  startedAt: number | null;
  completedAt: number | null;
  error: string | null;
  bitDepth: ExportBitDepth;

  setBitDepth: (bitDepth: ExportBitDepth) => void;
  startExport: (steps: ExportStep[]) => void;
  updateStep: (stepId: string, updates: Partial<ExportStep>) => void;
  completeStep: (stepId: string) => void;
  failStep: (stepId: string, error: string) => void;
  setOverallProgress: (progress: number) => void;
  finishExport: () => void;
  failExport: (error: string) => void;
  resetExport: () => void;
}

export const useExportStore = create<ExportState>((set) => ({
  isExporting: false,
  steps: [],
  overallProgress: 0,
  startedAt: null,
  completedAt: null,
  error: null,
  bitDepth: DEFAULT_BIT_DEPTH as ExportBitDepth,

  setBitDepth: (bitDepth) => set({ bitDepth }),

  startExport: (steps) =>
    set({
      isExporting: true,
      steps,
      overallProgress: 0,
      startedAt: Date.now(),
      completedAt: null,
      error: null,
    }),

  updateStep: (stepId, updates) =>
    set((state) => ({
      steps: state.steps.map((s) => (s.id === stepId ? { ...s, ...updates } : s)),
    })),

  completeStep: (stepId) =>
    set((state) => ({
      steps: state.steps.map((s) =>
        s.id === stepId ? { ...s, status: 'complete' as const, progress: 100 } : s
      ),
    })),

  failStep: (stepId, error) =>
    set((state) => ({
      steps: state.steps.map((s) =>
        s.id === stepId ? { ...s, status: 'error' as const, detail: error } : s
      ),
    })),

  setOverallProgress: (progress) => set({ overallProgress: progress }),

  finishExport: () =>
    set({
      isExporting: false,
      overallProgress: 100,
      completedAt: Date.now(),
    }),

  failExport: (error) =>
    set({
      isExporting: false,
      error,
    }),

  resetExport: () =>
    set({
      isExporting: false,
      steps: [],
      overallProgress: 0,
      startedAt: null,
      completedAt: null,
      error: null,
    }),
}));
