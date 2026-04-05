import { create } from 'zustand';
import { persist } from 'zustand/middleware';

interface LayoutState {
  /** Sidebar width in pixels (used when sidebar is expanded) */
  sidebarWidth: number;
  /** Right panel width in pixels */
  panelWidth: number;

  setSidebarWidth: (width: number) => void;
  setPanelWidth: (width: number) => void;
  /** Whether Liquid Glass material effects are enabled (default true) */
  glassEnabled: boolean;
  setGlassEnabled: (enabled: boolean) => void;
  resetLayout: () => void;
}

const DEFAULT_SIDEBAR_WIDTH = 224;
const DEFAULT_PANEL_WIDTH = 320;
export const MIN_SIDEBAR_WIDTH = 64;
export const MAX_SIDEBAR_WIDTH = 320;
export const MIN_PANEL_WIDTH = 200;
export const MAX_PANEL_WIDTH = 480;

export const useLayoutStore = create<LayoutState>()(
  persist(
    (set) => ({
      sidebarWidth: DEFAULT_SIDEBAR_WIDTH,
      panelWidth: DEFAULT_PANEL_WIDTH,

      setSidebarWidth: (width) =>
        set({ sidebarWidth: Math.max(MIN_SIDEBAR_WIDTH, Math.min(MAX_SIDEBAR_WIDTH, width)) }),

      setPanelWidth: (width) =>
        set({ panelWidth: Math.max(MIN_PANEL_WIDTH, Math.min(MAX_PANEL_WIDTH, width)) }),

      glassEnabled: true,

      setGlassEnabled: (enabled) => set({ glassEnabled: enabled }),

      resetLayout: () =>
        set({ sidebarWidth: DEFAULT_SIDEBAR_WIDTH, panelWidth: DEFAULT_PANEL_WIDTH, glassEnabled: true }),
    }),
    {
      name: 'xo-ox-layout',
    }
  )
);
