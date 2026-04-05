import { create } from 'zustand';
import type {
  CloudProvider,
  CloudFile,
  CloudFolder,
  CloudStorageAdapter,
} from '@/lib/cloud/cloudStorageTypes';
import { AUDIO_MIME_TYPES } from '@/lib/cloud/cloudStorageTypes';
import { sanitizeErrorMessage } from '@/lib/sanitize';

// ---------------------------------------------------------------------------
// Stub adapter – returned when no real adapter is registered for a provider.
// Every method throws a descriptive error so the UI can show setup guidance.
// ---------------------------------------------------------------------------

function createStubAdapter(provider: CloudProvider): CloudStorageAdapter {
  const names: Record<CloudProvider, string> = {
    'google-drive': 'Google Drive',
    onedrive: 'OneDrive',
    dropbox: 'Dropbox',
  };

  const configHints: Record<CloudProvider, string> = {
    'google-drive':
      'Configure your Google Drive API key in settings to enable this integration.',
    onedrive:
      'Configure your OneDrive client ID in settings to enable this integration.',
    dropbox:
      'Configure your Dropbox app key in settings to enable this integration.',
  };

  const notConfigured = () => {
    throw new Error(configHints[provider]);
  };

  return {
    provider,
    displayName: names[provider],
    icon: provider,
    isConnected: false,
    connect: async () => notConfigured(),
    disconnect: () => {},
    listFiles: async () => notConfigured() as never,
    listFolders: async () => notConfigured() as never,
    downloadFile: async () => notConfigured() as never,
    uploadFile: async () => notConfigured() as never,
    createFolder: async () => notConfigured() as never,
  };
}

// ---------------------------------------------------------------------------
// Adapter registry – real adapters are registered at runtime once OAuth
// credentials are available.  Until then the stub adapter is used.
// ---------------------------------------------------------------------------

const adapterRegistry = new Map<CloudProvider, CloudStorageAdapter>();

/**
 * Register a concrete adapter (e.g. GoogleDriveAdapter) at app startup.
 * Callers in adapter implementation files should import and call this.
 */
export function registerCloudAdapter(adapter: CloudStorageAdapter): void {
  adapterRegistry.set(adapter.provider, adapter);
}

function getAdapter(provider: CloudProvider): CloudStorageAdapter {
  return adapterRegistry.get(provider) ?? createStubAdapter(provider);
}

// ---------------------------------------------------------------------------
// Store interface
// ---------------------------------------------------------------------------

interface CloudStore {
  // Connection state
  connectedProviders: Set<CloudProvider>;
  connectingProvider: CloudProvider | null;

  // File browser state
  currentProvider: CloudProvider | null;
  currentFolder: CloudFolder | null;
  folderStack: CloudFolder[];
  files: CloudFile[];
  folders: CloudFolder[];
  isLoading: boolean;
  error: string | null;

  // Selection
  selectedFileIds: Set<string>;

  // Search / filter
  searchQuery: string;

  // Actions – connection
  connect: (provider: CloudProvider) => Promise<void>;
  disconnect: (provider: CloudProvider) => void;

  // Actions – browsing
  browse: (provider: CloudProvider, folderId?: string) => Promise<void>;
  navigateUp: () => Promise<void>;
  navigateToFolder: (folder: CloudFolder) => Promise<void>;
  navigateToBreadcrumb: (index: number) => Promise<void>;
  refresh: () => Promise<void>;

  // Actions – selection
  toggleFileSelection: (fileId: string) => void;
  selectAllFiles: () => void;
  clearSelection: () => void;

  // Actions – search
  setSearchQuery: (query: string) => void;

  // Actions – import / export
  importFile: (file: CloudFile) => Promise<ArrayBuffer>;
  importSelected: () => Promise<ArrayBuffer[]>;
  exportToCloud: (
    provider: CloudProvider,
    name: string,
    data: ArrayBuffer,
    mimeType: string,
    folderId?: string,
  ) => Promise<CloudFile>;

  // Utility
  clearError: () => void;
  reset: () => void;
}

export const useCloudStore = create<CloudStore>((set, get) => ({
  // --- initial state ---------------------------------------------------------
  connectedProviders: new Set(),
  connectingProvider: null,
  currentProvider: null,
  currentFolder: null,
  folderStack: [],
  files: [],
  folders: [],
  isLoading: false,
  error: null,
  selectedFileIds: new Set(),
  searchQuery: '',

  // --- connection ------------------------------------------------------------

  connect: async (provider) => {
    set({ connectingProvider: provider, error: null });
    try {
      const adapter = getAdapter(provider);
      await adapter.connect();
      set((state) => {
        const next = new Set(state.connectedProviders);
        next.add(provider);
        return {
          connectedProviders: next,
          connectingProvider: null,
        };
      });
      // After connecting, automatically browse root
      await get().browse(provider);
    } catch (err) {
      set({
        connectingProvider: null,
        error: sanitizeErrorMessage(
          err instanceof Error ? err.message : `Failed to connect to ${provider}`
        ),
      });
    }
  },

  disconnect: (provider) => {
    const adapter = getAdapter(provider);
    adapter.disconnect();
    set((state) => {
      const next = new Set(state.connectedProviders);
      next.delete(provider);
      return {
        connectedProviders: next,
        // If we were browsing this provider, reset browser state
        ...(state.currentProvider === provider
          ? {
              currentProvider: null,
              currentFolder: null,
              folderStack: [],
              files: [],
              folders: [],
              selectedFileIds: new Set<string>(),
              searchQuery: '',
            }
          : {}),
      };
    });
  },

  // --- browsing --------------------------------------------------------------

  browse: async (provider, folderId) => {
    set({ isLoading: true, error: null, selectedFileIds: new Set(), searchQuery: '' });
    try {
      const adapter = getAdapter(provider);
      const [files, folders] = await Promise.all([
        adapter.listFiles(folderId, [...AUDIO_MIME_TYPES]),
        adapter.listFolders(folderId),
      ]);
      set({
        currentProvider: provider,
        files,
        folders,
        isLoading: false,
      });
    } catch (err) {
      set({
        isLoading: false,
        error: sanitizeErrorMessage(
          err instanceof Error ? err.message : 'Failed to list files'
        ),
      });
    }
  },

  navigateToFolder: async (folder) => {
    const state = get();
    if (!state.currentProvider) return;

    // Push current folder onto breadcrumb stack
    const newStack = [...state.folderStack];
    if (state.currentFolder) {
      newStack.push(state.currentFolder);
    }
    set({ currentFolder: folder, folderStack: newStack });

    await get().browse(state.currentProvider, folder.id);
  },

  navigateUp: async () => {
    const state = get();
    if (!state.currentProvider) return;

    const newStack = [...state.folderStack];
    const parent = newStack.pop() ?? null;

    set({ currentFolder: parent, folderStack: newStack });
    await get().browse(state.currentProvider, parent?.id);
  },

  navigateToBreadcrumb: async (index) => {
    const state = get();
    if (!state.currentProvider) return;

    if (index < 0) {
      // Navigate to root
      set({ currentFolder: null, folderStack: [] });
      await get().browse(state.currentProvider);
      return;
    }

    const targetFolder = state.folderStack[index];
    if (!targetFolder) return;

    const newStack = state.folderStack.slice(0, index);
    set({ currentFolder: targetFolder, folderStack: newStack });
    await get().browse(state.currentProvider, targetFolder.id);
  },

  refresh: async () => {
    const state = get();
    if (!state.currentProvider) return;
    await get().browse(state.currentProvider, state.currentFolder?.id);
  },

  // --- selection -------------------------------------------------------------

  toggleFileSelection: (fileId) => {
    set((state) => {
      const next = new Set(state.selectedFileIds);
      if (next.has(fileId)) {
        next.delete(fileId);
      } else {
        next.add(fileId);
      }
      return { selectedFileIds: next };
    });
  },

  selectAllFiles: () => {
    set((state) => ({
      selectedFileIds: new Set(state.files.map((f) => f.id)),
    }));
  },

  clearSelection: () => {
    set({ selectedFileIds: new Set() });
  },

  // --- search ----------------------------------------------------------------

  setSearchQuery: (query) => set({ searchQuery: query }),

  // --- import / export -------------------------------------------------------

  importFile: async (file) => {
    set({ isLoading: true, error: null });
    try {
      const adapter = getAdapter(file.provider);
      const result = await adapter.downloadFile(file.id);
      set({ isLoading: false });
      return result;
    } catch (err) {
      const message = sanitizeErrorMessage(
        err instanceof Error ? err.message : 'Failed to download file'
      );
      set({ isLoading: false, error: message });
      throw new Error(message);
    }
  },

  importSelected: async () => {
    const state = get();
    if (!state.currentProvider) throw new Error('No provider selected');

    set({ isLoading: true, error: null });
    try {
      const adapter = getAdapter(state.currentProvider);
      const selected = state.files.filter((f) =>
        state.selectedFileIds.has(f.id),
      );

      const results: ArrayBuffer[] = [];
      const failures: string[] = [];
      for (const file of selected) {
        try {
          const buffer = await adapter.downloadFile(file.id);
          results.push(buffer);
        } catch (err) {
          failures.push(file.name ?? file.id);
        }
      }

      set({ isLoading: false });

      if (failures.length > 0 && results.length === 0) {
        const message = `All ${failures.length} file(s) failed to download`;
        set({ error: message }); // failure count only — no raw error content
        throw new Error(message);
      } else if (failures.length > 0) {
        set({ error: `${failures.length} file(s) failed: ${failures.join(', ')}` });
      }

      return results;
    } catch (err) {
      // Re-throw if it was our own error, otherwise wrap
      set({ isLoading: false });
      if (err instanceof Error && get().error) throw err;
      const message = sanitizeErrorMessage(
        err instanceof Error ? err.message : 'Failed to import files'
      );
      set({ error: message });
      throw new Error(message);
    }
  },

  exportToCloud: async (provider, name, data, mimeType, folderId) => {
    set({ isLoading: true, error: null });
    try {
      const adapter = getAdapter(provider);
      const result = await adapter.uploadFile(name, data, mimeType, folderId);
      set({ isLoading: false });
      return result;
    } catch (err) {
      const message = sanitizeErrorMessage(
        err instanceof Error ? err.message : `Failed to upload "${name}"`
      );
      set({ isLoading: false, error: message });
      throw new Error(message);
    }
  },

  // --- utility ---------------------------------------------------------------

  clearError: () => set({ error: null }),

  reset: () =>
    set({
      currentProvider: null,
      currentFolder: null,
      folderStack: [],
      files: [],
      folders: [],
      isLoading: false,
      error: null,
      selectedFileIds: new Set(),
      searchQuery: '',
    }),
}));
