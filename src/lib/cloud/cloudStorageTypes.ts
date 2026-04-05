/**
 * Cloud Storage Integration Types
 *
 * Defines the provider abstraction layer for Google Drive, OneDrive, and Dropbox.
 * Each provider implements CloudStorageAdapter so the rest of the app can interact
 * with any cloud service through a single, uniform interface.
 */

export type CloudProvider = 'google-drive' | 'onedrive' | 'dropbox';

/** Audio MIME types recognized when browsing cloud folders */
export const AUDIO_MIME_TYPES = [
  'audio/wav',
  'audio/x-wav',
  'audio/wave',
  'audio/mp3',
  'audio/mpeg',
  'audio/aiff',
  'audio/x-aiff',
  'audio/flac',
  'audio/ogg',
  'audio/mp4',
] as const;

/** File extensions treated as audio when MIME type is unavailable */
export const AUDIO_EXTENSIONS = [
  '.wav',
  '.mp3',
  '.aiff',
  '.aif',
  '.flac',
  '.ogg',
  '.m4a',
] as const;

/** A file stored in a cloud provider */
export interface CloudFile {
  id: string;
  name: string;
  mimeType: string;
  size: number;
  modifiedTime: string;
  path: string;
  provider: CloudProvider;
  downloadUrl?: string;
}

/** A folder in a cloud provider */
export interface CloudFolder {
  id: string;
  name: string;
  path: string;
  provider: CloudProvider;
}

/** Provider metadata shown in the UI */
export interface CloudProviderInfo {
  provider: CloudProvider;
  displayName: string;
  brandColor: string;
  description: string;
}

/**
 * Adapter interface every cloud provider must implement.
 *
 * When no real OAuth credentials are configured the store will use a stub
 * adapter that surfaces helpful setup instructions instead of throwing.
 */
export interface CloudStorageAdapter {
  provider: CloudProvider;
  displayName: string;
  icon: string;
  isConnected: boolean;
  connect: () => Promise<void>;
  disconnect: () => void;
  listFiles: (folderId?: string, mimeTypes?: string[]) => Promise<CloudFile[]>;
  listFolders: (parentId?: string) => Promise<CloudFolder[]>;
  downloadFile: (fileId: string) => Promise<ArrayBuffer>;
  uploadFile: (
    name: string,
    data: ArrayBuffer,
    mimeType: string,
    folderId?: string,
  ) => Promise<CloudFile>;
  createFolder: (name: string, parentId?: string) => Promise<CloudFolder>;
}

/** Static info for every supported provider */
export const CLOUD_PROVIDERS: Record<CloudProvider, CloudProviderInfo> = {
  'google-drive': {
    provider: 'google-drive',
    displayName: 'Google Drive',
    brandColor: '#4285F4',
    description: 'Connect your Google Drive to import and export audio files.',
  },
  onedrive: {
    provider: 'onedrive',
    displayName: 'OneDrive',
    brandColor: '#0078D4',
    description:
      'Connect your Microsoft OneDrive to import and export audio files.',
  },
  dropbox: {
    provider: 'dropbox',
    displayName: 'Dropbox',
    brandColor: '#0061FF',
    description: 'Connect your Dropbox to import and export audio files.',
  },
};

/** Check whether a filename looks like an audio file */
export function isAudioFile(filename: string): boolean {
  const lower = filename.toLowerCase();
  return AUDIO_EXTENSIONS.some((ext) => lower.endsWith(ext));
}

/** Check whether a MIME type is an audio type */
export function isAudioMimeType(mimeType: string): boolean {
  return (AUDIO_MIME_TYPES as readonly string[]).includes(mimeType);
}

/** Human-readable file size */
export function formatCloudFileSize(bytes: number): string {
  if (bytes === 0) return '0 B';
  const units = ['B', 'KB', 'MB', 'GB'];
  const i = Math.floor(Math.log(bytes) / Math.log(1024));
  const size = bytes / Math.pow(1024, i);
  return `${size.toFixed(i === 0 ? 0 : 1)} ${units[i]}`;
}

/** Format an ISO date string to a short locale representation */
export function formatCloudDate(iso: string): string {
  try {
    return new Date(iso).toLocaleDateString(undefined, {
      month: 'short',
      day: 'numeric',
      year: 'numeric',
    });
  } catch {
    return iso;
  }
}
