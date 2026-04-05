'use client';

import { create } from 'zustand';
import { v4 as uuid } from 'uuid';

export type ToastType = 'success' | 'error' | 'warning' | 'info';

export interface Toast {
  id: string;
  type: ToastType;
  title: string;
  message?: string;
  duration?: number; // ms, default 5000, 0 = sticky
  action?: {
    label: string;
    onClick: () => void;
  };
  dismissible?: boolean; // default true
  timestamp: number;
}

const MAX_VISIBLE_TOASTS = 5;

interface ToastStore {
  toasts: Toast[];
  addToast: (toast: Omit<Toast, 'id' | 'timestamp'>) => string;
  removeToast: (id: string) => void;
  clearAll: () => void;
}

export const useToastStore = create<ToastStore>((set) => ({
  toasts: [],

  addToast: (toast) => {
    const id = uuid();
    const newToast: Toast = {
      ...toast,
      id,
      timestamp: Date.now(),
      dismissible: toast.dismissible ?? true,
      duration: toast.duration ?? 5000,
    };

    set((state) => {
      const updated = [...state.toasts, newToast];
      // Enforce max visible limit — remove oldest when exceeded
      if (updated.length > MAX_VISIBLE_TOASTS) {
        return { toasts: updated.slice(updated.length - MAX_VISIBLE_TOASTS) };
      }
      return { toasts: updated };
    });

    return id;
  },

  removeToast: (id) =>
    set((state) => ({
      toasts: state.toasts.filter((t) => t.id !== id),
    })),

  clearAll: () => set({ toasts: [] }),
}));
