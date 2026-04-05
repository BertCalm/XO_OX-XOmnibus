import { isTauri } from './tauriDetect';

export interface SdCardInfo {
  /** Mount point path (e.g. /Volumes/MPC or D:\) */
  mountPoint: string;
  /** Volume label */
  label: string;
  /** Whether MPC directory structure was detected */
  hasMpcStructure: boolean;
  /** Available space in bytes */
  availableSpace: number;
}

/**
 * Detect MPC-formatted SD cards by scanning mounted volumes.
 * Looks for characteristic MPC directory structure:
 * - /[root]/Expansions/
 * - /[root]/Projects/
 *
 * Only available in Tauri mode.
 * All filesystem operations are delegated to the Rust backend via invoke.
 */
export async function detectMpcSdCard(): Promise<SdCardInfo | null> {
  if (!isTauri()) return null;

  try {
    const { invoke } = await import('@tauri-apps/api/core');

    // Call Rust command to list volumes
    const volumes = await invoke<{ mountPoint: string; label: string; availableSpace: number }[]>(
      'list_volumes'
    );

    // Check each volume for MPC directory structure via Rust backend
    for (let i = 0; i < volumes.length; i++) {
      const vol = volumes[i];
      const expansionsPath = `${vol.mountPoint}/Expansions`;

      let hasMpcStructure = false;
      try {
        hasMpcStructure = await invoke<boolean>('path_exists', { path: expansionsPath });
      } catch {
        // Permission denied or path doesn't exist
      }

      if (hasMpcStructure) {
        return {
          mountPoint: vol.mountPoint,
          label: vol.label || 'SD Card',
          hasMpcStructure: true,
          availableSpace: vol.availableSpace,
        };
      }
    }

    return null;
  } catch (error) {
    console.error('SD card detection failed:', error);
    return null;
  }
}

/**
 * Export an XPN blob directly to an MPC SD card.
 * Writes to the /Expansions/ directory.
 * All filesystem operations are delegated to the Rust backend via invoke.
 */
export async function exportToSdCard(
  xpnBlob: Blob,
  fileName: string,
  sdCard: SdCardInfo,
): Promise<void> {
  if (!isTauri()) throw new Error('SD card export is only available in Tauri mode');

  const { invoke } = await import('@tauri-apps/api/core');

  const expansionsDir = `${sdCard.mountPoint}/Expansions`;

  // Ensure Expansions directory exists via Rust backend
  const dirExists = await invoke<boolean>('path_exists', { path: expansionsDir });
  if (!dirExists) {
    await invoke('create_dir', { path: expansionsDir, recursive: true });
  }

  // Convert blob to Uint8Array (array of numbers for Tauri serialization)
  const arrayBuffer = await xpnBlob.arrayBuffer();
  const data = Array.from(new Uint8Array(arrayBuffer));

  // Sanitize fileName to prevent path traversal attacks
  const safeFileName = fileName.split('/').pop()?.split('\\').pop() ?? fileName;

  // Write the file via Rust backend
  const filePath = `${expansionsDir}/${safeFileName}`;
  await invoke('write_binary_file', { path: filePath, data });
}

/**
 * Format bytes to human-readable string.
 */
export function formatBytes(bytes: number): string {
  if (bytes === 0) return '0 B';
  const sizes = ['B', 'KB', 'MB', 'GB'];
  const i = Math.floor(Math.log(bytes) / Math.log(1024));
  return `${(bytes / Math.pow(1024, i)).toFixed(1)} ${sizes[i]}`;
}
