import { isTauri } from './tauriDetect';

export interface FloatingCrateConfig {
  width?: number;   // default 300
  height?: number;  // default 400
  alwaysOnTop?: boolean; // default true
}

/**
 * Open a floating crate mini-window.
 * Only available when running in Tauri.
 * The crate window loads the /crate route which shows a minimal pad grid.
 */
export async function openFloatingCrate(config?: FloatingCrateConfig): Promise<boolean> {
  if (!isTauri()) return false;

  try {
    // Dynamic import to avoid bundler errors in web mode
    const { WebviewWindow } = await import('@tauri-apps/api/webviewWindow');

    const { width = 300, height = 400, alwaysOnTop = true } = config || {};

    // Check if crate window already exists
    const existing = await WebviewWindow.getByLabel('floating-crate');
    if (existing) {
      await existing.setFocus();
      return true;
    }

    // Create new window
    const crateWindow = new WebviewWindow('floating-crate', {
      url: '/crate',
      title: 'XO_OX Crate',
      width,
      height,
      alwaysOnTop,
      decorations: true,
      resizable: true,
      minimizable: true,
      maximizable: false,
      center: false,
      x: 50,
      y: 50,
    });

    // Wait for window creation
    await crateWindow.once('tauri://created', () => {
      console.log('Floating crate window created');
    });

    return true;
  } catch (error) {
    console.error('Failed to open floating crate:', error);
    return false;
  }
}

/**
 * Close the floating crate window if it's open.
 */
export async function closeFloatingCrate(): Promise<void> {
  if (!isTauri()) return;

  try {
    const { WebviewWindow } = await import('@tauri-apps/api/webviewWindow');
    const existing = await WebviewWindow.getByLabel('floating-crate');
    if (existing) {
      await existing.close();
    }
  } catch {
    // Window may already be closed
  }
}

/**
 * Check if the floating crate window is currently open.
 */
export async function isCrateOpen(): Promise<boolean> {
  if (!isTauri()) return false;

  try {
    const { WebviewWindow } = await import('@tauri-apps/api/webviewWindow');
    const existing = await WebviewWindow.getByLabel('floating-crate');
    return existing !== null;
  } catch {
    return false;
  }
}
