import { create } from 'zustand';
import { persist } from 'zustand/middleware';

interface LayoutState {
  /** Right panel width in pixels */
  panelWidth: number;

  setPanelWidth: (width: number) => void;
  /** Whether Liquid Glass material effects are enabled (default true) */
  glassEnabled: boolean;
  setGlassEnabled: (enabled: boolean) => void;
  resetLayout: () => void;
}

const DEFAULT_PANEL_WIDTH = 320;
export const MIN_PANEL_WIDTH = 200;
export const MAX_PANEL_WIDTH = 480;

export const useLayoutStore = create<LayoutState>()(
  persist(
    (set) => ({
      panelWidth: DEFAULT_PANEL_WIDTH,

      setPanelWidth: (width) =>
        set({ panelWidth: Math.max(MIN_PANEL_WIDTH, Math.min(MAX_PANEL_WIDTH, width)) }),

      glassEnabled: true,

      setGlassEnabled: (enabled) => set({ glassEnabled: enabled }),

      resetLayout: () =>
        set({ panelWidth: DEFAULT_PANEL_WIDTH, glassEnabled: true }),
    }),
    {
      name: 'xo-ox-layout',
    }
  )
);
