/**
 * Detects whether the app is running inside a Tauri desktop shell
 * or in a standard browser environment.
 *
 * This enables progressive enhancement:
 * - In Tauri: use native file dialogs, direct file system access
 * - In browser: use standard web APIs (File API, IndexedDB, downloads)
 */

export function isTauri(): boolean {
  if (typeof window === 'undefined') return false;
  return '__TAURI_INTERNALS__' in window;
}

/**
 * Dynamically import Tauri APIs only when running in Tauri.
 * Prevents bundler errors when running as a web app.
 */
async function getTauriDialog() {
  if (!isTauri()) return null;
  return await import('@tauri-apps/api/core');
}
