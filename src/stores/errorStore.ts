'use client';

import { create } from 'zustand';
import { v4 as uuid } from 'uuid';
import { useToastStore } from '@/stores/toastStore';

export interface ErrorEntry {
  id: string;
  timestamp: number;
  section: string; // 'The Crate' | 'The Forge' | 'Program Editor' | 'The Breath' | 'Audio Engine' | 'Export' | etc
  error: string; // error.message
  stack?: string; // error.stack
  context?: Record<string, unknown>; // additional context like { sampleId, padIndex, action }
  resolved: boolean;
  workaround?: string; // suggested workaround
}

interface ErrorStore {
  errors: ErrorEntry[];
  logError: (section: string, error: Error | string, context?: Record<string, unknown>) => void;
  resolveError: (id: string) => void;
  clearResolved: () => void;
  getUnresolved: () => ErrorEntry[];
  exportErrorLog: () => string; // JSON export for sharing with developer
}

/* ------------------------------------------------------------------ */
/* Workaround suggestions based on common error patterns               */
/* ------------------------------------------------------------------ */

function suggestWorkaround(errorMessage: string): string | undefined {
  const msg = errorMessage.toLowerCase();

  if (msg.includes('decoding') || msg.includes('audio') || msg.includes('decode')) {
    return 'Try re-importing the sample in a different format (WAV recommended)';
  }
  if (msg.includes('memory') || msg.includes('allocation') || msg.includes('out of memory')) {
    return 'Try reducing the number of loaded samples or refreshing the page';
  }
  if (msg.includes('export') || msg.includes('zip') || msg.includes('package')) {
    return 'Check that all pads have valid samples assigned';
  }
  if (msg.includes('network') || msg.includes('fetch') || msg.includes('failed to fetch') || msg.includes('net::')) {
    return 'Check your internet connection and try again';
  }
  if (msg.includes('permission') || msg.includes('denied') || msg.includes('not allowed')) {
    return 'Check browser permissions for file system access or audio playback';
  }
  if (msg.includes('buffer') || msg.includes('arraybuffer')) {
    return 'The audio file may be corrupted. Try re-exporting from your DAW';
  }
  if (msg.includes('indexeddb') || msg.includes('storage') || msg.includes('quota')) {
    return 'Browser storage may be full. Try clearing unused projects or browser data';
  }

  return undefined;
}

/* ------------------------------------------------------------------ */
/* Store                                                               */
/* ------------------------------------------------------------------ */

export const useErrorStore = create<ErrorStore>((set, get) => ({
  errors: [],

  logError: (section, error, context) => {
    const errorMessage = typeof error === 'string' ? error : error.message;
    const errorStack = typeof error === 'string' ? undefined : error.stack;
    const workaround = suggestWorkaround(errorMessage);

    const entry: ErrorEntry = {
      id: uuid(),
      timestamp: Date.now(),
      section,
      error: errorMessage,
      stack: errorStack,
      context,
      resolved: false,
      workaround,
    };

    set((state) => {
      const MAX_ERRORS = 100;
      const updated = [...state.errors, entry];
      // FIFO eviction: drop oldest entries when limit is exceeded
      if (updated.length > MAX_ERRORS) {
        updated.splice(0, updated.length - MAX_ERRORS);
      }
      return { errors: updated };
    });

    // Fire a toast notification for the error
    useToastStore.getState().addToast({
      type: 'error',
      title: `Error in ${section}`,
      message: workaround
        ? `${errorMessage}\n${workaround}`
        : errorMessage,
      duration: 8000, // errors stay a bit longer
    });

    // Also log to console for development
    console.error(`[XO_OX Error] ${section}:`, errorMessage, context ?? '');
  },

  resolveError: (id) =>
    set((state) => ({
      errors: state.errors.map((e) =>
        e.id === id ? { ...e, resolved: true } : e
      ),
    })),

  clearResolved: () =>
    set((state) => ({
      errors: state.errors.filter((e) => !e.resolved),
    })),

  getUnresolved: () => get().errors.filter((e) => !e.resolved),

  exportErrorLog: () => {
    const { errors } = get();
    const exportData = {
      exportedAt: new Date().toISOString(),
      appVersion: '0.1.0',
      userAgent: typeof navigator !== 'undefined' ? navigator.userAgent : 'unknown',
      errorCount: errors.length,
      unresolvedCount: errors.filter((e) => !e.resolved).length,
      errors: errors.map((e) => ({
        id: e.id,
        timestamp: new Date(e.timestamp).toISOString(),
        section: e.section,
        error: e.error,
        stack: e.stack,
        context: e.context,
        resolved: e.resolved,
        workaround: e.workaround,
      })),
    };
    return JSON.stringify(exportData, null, 2);
  },
}));
