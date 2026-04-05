'use client';

import React, { useMemo, useCallback } from 'react';
import Modal from '@/components/ui/Modal';
import Button from '@/components/ui/Button';
import { useCloudStore } from '@/stores/cloudStore';
import {
  CLOUD_PROVIDERS,
  isAudioFile,
  isAudioMimeType,
  formatCloudFileSize,
  formatCloudDate,
} from '@/lib/cloud/cloudStorageTypes';
import type {
  CloudProvider,
  CloudFile,
  CloudFolder,
} from '@/lib/cloud/cloudStorageTypes';

// ---------------------------------------------------------------------------
// Props
// ---------------------------------------------------------------------------

interface CloudBrowserProps {
  open: boolean;
  onClose: () => void;
  /** When set, the browser is in folder-select mode (for export destination) */
  folderSelectMode?: boolean;
  /** Called when user picks a folder in folder-select mode */
  onFolderSelect?: (folder: CloudFolder | null, provider: CloudProvider) => void;
  /** Called when user imports selected audio files */
  onImport?: (files: CloudFile[]) => void;
}

// ---------------------------------------------------------------------------
// Provider icons (inline SVG so we don't need external assets)
// ---------------------------------------------------------------------------

function GoogleDriveIcon({ size = 20 }: { size?: number }) {
  return (
    <svg width={size} height={size} viewBox="0 0 24 24" fill="none">
      <path d="M8.25 2.5L1.5 14.5h6.75L15 2.5H8.25z" fill="#4285F4" />
      <path d="M15 2.5l6.75 12H15L8.25 2.5H15z" fill="#FBBC04" />
      <path d="M1.5 14.5l3.375 6h13.5l3.375-6H1.5z" fill="#34A853" />
    </svg>
  );
}

function OneDriveIcon({ size = 20 }: { size?: number }) {
  return (
    <svg width={size} height={size} viewBox="0 0 24 24" fill="none">
      <path
        d="M10 7C7.24 7 5 9.24 5 12c0 .34.04.67.1 1H4a3 3 0 000 6h14a4 4 0 000-8c-.34 0-.67.04-1 .1A5.002 5.002 0 0010 7z"
        fill="#0078D4"
      />
    </svg>
  );
}

function DropboxIcon({ size = 20 }: { size?: number }) {
  return (
    <svg width={size} height={size} viewBox="0 0 24 24" fill="none">
      <path
        d="M12 2l-6 4 6 4-6 4 6 4 6-4-6-4 6-4-6-4z"
        fill="#0061FF"
      />
    </svg>
  );
}

const providerIcons: Record<CloudProvider, React.FC<{ size?: number }>> = {
  'google-drive': GoogleDriveIcon,
  onedrive: OneDriveIcon,
  dropbox: DropboxIcon,
};

// ---------------------------------------------------------------------------
// Sub-components
// ---------------------------------------------------------------------------

/** Individual provider card at the top of the browser */
function ProviderCard({
  provider,
  isConnected,
  isConnecting,
  onConnect,
  onSelect,
  isActive,
}: {
  provider: CloudProvider;
  isConnected: boolean;
  isConnecting: boolean;
  onConnect: () => void;
  onSelect: () => void;
  isActive: boolean;
}) {
  const info = CLOUD_PROVIDERS[provider];
  const Icon = providerIcons[provider];

  return (
    <button
      onClick={isConnected ? onSelect : onConnect}
      className={`flex items-center gap-3 p-4 rounded-xl border transition-all duration-150
        ${isActive
          ? 'border-accent-teal bg-accent-teal-50'
          : 'border-border bg-surface hover:bg-surface-alt hover:border-border-hover'
        }
        ${isConnecting ? 'opacity-70 pointer-events-none' : ''}
      `}
    >
      <div
        className="w-10 h-10 rounded-lg flex items-center justify-center flex-shrink-0"
        style={{ backgroundColor: `${info.brandColor}15` }}
      >
        <Icon size={22} />
      </div>

      <div className="flex-1 text-left min-w-0">
        <p className="text-sm font-medium text-text-primary">{info.displayName}</p>
        {isConnecting ? (
          <p className="text-xs text-accent-teal">Connecting...</p>
        ) : isConnected ? (
          <div className="flex items-center gap-1.5">
            <span className="w-1.5 h-1.5 rounded-full bg-green-500" />
            <span className="text-xs text-green-600">Connected</span>
          </div>
        ) : (
          <p className="text-xs text-text-muted">Connect</p>
        )}
      </div>
    </button>
  );
}

/** Breadcrumb navigation bar */
function Breadcrumbs({
  folderStack,
  currentFolder,
  onNavigate,
}: {
  folderStack: CloudFolder[];
  currentFolder: CloudFolder | null;
  onNavigate: (index: number) => void;
}) {
  const crumbs = [
    { label: 'Home', index: -1 },
    ...folderStack.map((f, i) => ({ label: f.name, index: i })),
    ...(currentFolder ? [{ label: currentFolder.name, index: -2 }] : []),
  ];

  return (
    <div className="flex items-center gap-1 text-xs overflow-x-auto py-1 scrollbar-none">
      {crumbs.map((crumb, i) => {
        const isLast = i === crumbs.length - 1;
        return (
          <React.Fragment key={`${crumb.index}-${crumb.label}`}>
            {i > 0 && <span className="text-text-muted mx-0.5">/</span>}
            {isLast ? (
              <span className="font-medium text-text-primary truncate max-w-[120px]">
                {crumb.label}
              </span>
            ) : (
              <button
                onClick={() => onNavigate(crumb.index)}
                className="text-text-secondary hover:text-accent-teal transition-colors
                  truncate max-w-[120px]"
              >
                {crumb.label}
              </button>
            )}
          </React.Fragment>
        );
      })}
    </div>
  );
}

/** Audio waveform icon */
function AudioFileIcon() {
  return (
    <svg width="16" height="16" viewBox="0 0 16 16" fill="none" className="text-accent-teal">
      <rect x="2" y="5" width="1.5" height="6" rx="0.75" fill="currentColor" />
      <rect x="5" y="3" width="1.5" height="10" rx="0.75" fill="currentColor" />
      <rect x="8" y="4" width="1.5" height="8" rx="0.75" fill="currentColor" />
      <rect x="11" y="6" width="1.5" height="4" rx="0.75" fill="currentColor" />
    </svg>
  );
}

/** Folder icon */
function FolderIcon() {
  return (
    <svg width="16" height="16" viewBox="0 0 16 16" fill="none" className="text-text-muted">
      <path
        d="M2 4.5A1.5 1.5 0 013.5 3H6l1.5 1.5h5A1.5 1.5 0 0114 6v5.5a1.5 1.5 0 01-1.5 1.5h-9A1.5 1.5 0 012 11.5v-7z"
        fill="currentColor"
      />
    </svg>
  );
}

/** Generic file icon */
function FileIcon() {
  return (
    <svg width="16" height="16" viewBox="0 0 16 16" fill="none" className="text-text-muted">
      <path
        d="M4 2h5l3 3v8a1 1 0 01-1 1H4a1 1 0 01-1-1V3a1 1 0 011-1z"
        stroke="currentColor"
        strokeWidth="1.2"
        fill="none"
      />
      <path d="M9 2v3h3" stroke="currentColor" strokeWidth="1.2" fill="none" />
    </svg>
  );
}

/** A single row in the file list */
function FileRow({
  file,
  isSelected,
  onToggleSelect,
  onOpen,
  selectable,
}: {
  file: CloudFile;
  isSelected: boolean;
  onToggleSelect: () => void;
  onOpen: () => void;
  selectable: boolean;
}) {
  const isAudio = isAudioFile(file.name) || isAudioMimeType(file.mimeType);

  return (
    <div
      className={`group flex items-center gap-3 px-3 py-2.5 rounded-lg cursor-pointer
        transition-all duration-100
        ${isSelected
          ? 'bg-accent-teal-50 border border-accent-teal/20'
          : 'hover:bg-surface-alt border border-transparent'
        }
        ${isAudio ? '' : 'opacity-60'}
      `}
      onClick={isAudio ? onOpen : undefined}
    >
      {selectable && (
        <input
          type="checkbox"
          checked={isSelected}
          onChange={(e) => {
            e.stopPropagation();
            onToggleSelect();
          }}
          onClick={(e) => e.stopPropagation()}
          disabled={!isAudio}
          className="w-4 h-4 rounded border-border text-accent-teal
            focus:ring-accent-teal/30 flex-shrink-0"
        />
      )}

      <div className="flex-shrink-0">
        {isAudio ? <AudioFileIcon /> : <FileIcon />}
      </div>

      <div className="flex-1 min-w-0">
        <p className={`text-xs font-medium truncate
          ${isAudio ? 'text-text-primary' : 'text-text-secondary'}`}
        >
          {file.name}
        </p>
      </div>

      <span className="text-[10px] text-text-muted flex-shrink-0 hidden sm:block">
        {formatCloudFileSize(file.size)}
      </span>
      <span className="text-[10px] text-text-muted flex-shrink-0 hidden sm:block w-20 text-right">
        {formatCloudDate(file.modifiedTime)}
      </span>
    </div>
  );
}

/** A folder row in the list */
function FolderRow({
  folder,
  onOpen,
  folderSelectMode,
  onSelectFolder,
  isSelected,
}: {
  folder: CloudFolder;
  onOpen: () => void;
  folderSelectMode?: boolean;
  onSelectFolder?: () => void;
  isSelected?: boolean;
}) {
  return (
    <div
      className={`group flex items-center gap-3 px-3 py-2.5 rounded-lg cursor-pointer
        transition-all duration-100
        ${isSelected
          ? 'bg-accent-teal-50 border border-accent-teal/20'
          : 'hover:bg-surface-alt border border-transparent'
        }
      `}
      onClick={onOpen}
    >
      <div className="flex-shrink-0">
        <FolderIcon />
      </div>
      <div className="flex-1 min-w-0">
        <p className="text-xs font-medium text-text-primary truncate">{folder.name}</p>
      </div>
      {folderSelectMode && (
        <Button
          variant="ghost"
          size="sm"
          onClick={(e) => {
            e.stopPropagation();
            onSelectFolder?.();
          }}
        >
          Select
        </Button>
      )}
    </div>
  );
}

/** Loading skeleton rows */
function LoadingSkeleton() {
  return (
    <div className="space-y-2 animate-pulse">
      {Array.from({ length: 5 }).map((_, i) => (
        <div key={i} className="flex items-center gap-3 px-3 py-2.5">
          <div className="w-4 h-4 rounded bg-surface-alt" />
          <div className="flex-1 h-3 rounded bg-surface-alt" />
          <div className="w-12 h-3 rounded bg-surface-alt hidden sm:block" />
          <div className="w-16 h-3 rounded bg-surface-alt hidden sm:block" />
        </div>
      ))}
    </div>
  );
}

/** Empty state when no files match */
function EmptyState({ hasSearch }: { hasSearch: boolean }) {
  return (
    <div className="flex flex-col items-center justify-center py-12 text-center">
      <div className="w-12 h-12 rounded-full bg-surface-alt flex items-center justify-center mb-3">
        <svg width="24" height="24" viewBox="0 0 24 24" fill="none" className="text-text-muted">
          <path
            d="M3 7v10a2 2 0 002 2h14a2 2 0 002-2V9a2 2 0 00-2-2h-6l-2-2H5a2 2 0 00-2 2z"
            stroke="currentColor"
            strokeWidth="1.5"
          />
          <path d="M10 14h4" stroke="currentColor" strokeWidth="1.5" strokeLinecap="round" />
        </svg>
      </div>
      <p className="text-sm text-text-secondary">
        {hasSearch ? 'No files match your search' : 'No audio files found in this folder'}
      </p>
      <p className="text-xs text-text-muted mt-1">
        {hasSearch
          ? 'Try a different search term'
          : 'Navigate to a folder containing WAV, MP3, AIFF, or FLAC files'}
      </p>
    </div>
  );
}

/** Error banner */
function ErrorBanner({ message, onDismiss }: { message: string; onDismiss: () => void }) {
  return (
    <div className="flex items-start gap-2 p-3 rounded-lg bg-red-50 border border-red-200 mb-3">
      <svg width="16" height="16" viewBox="0 0 16 16" fill="none" className="text-red-500 flex-shrink-0 mt-0.5">
        <circle cx="8" cy="8" r="6" stroke="currentColor" strokeWidth="1.5" />
        <path d="M8 5v3" stroke="currentColor" strokeWidth="1.5" strokeLinecap="round" />
        <circle cx="8" cy="11" r="0.75" fill="currentColor" />
      </svg>
      <div className="flex-1 min-w-0">
        <p className="text-xs text-red-700">{message}</p>
      </div>
      <button
        onClick={onDismiss}
        className="text-red-400 hover:text-red-600 transition-colors flex-shrink-0"
      >
        <svg width="14" height="14" viewBox="0 0 14 14" fill="none">
          <path d="M4 4l6 6M10 4l-6 6" stroke="currentColor" strokeWidth="1.5" strokeLinecap="round" />
        </svg>
      </button>
    </div>
  );
}

// ---------------------------------------------------------------------------
// Main component
// ---------------------------------------------------------------------------

export default function CloudBrowser({
  open,
  onClose,
  folderSelectMode = false,
  onFolderSelect,
  onImport,
}: CloudBrowserProps) {
  const {
    connectedProviders,
    connectingProvider,
    currentProvider,
    currentFolder,
    folderStack,
    files,
    folders,
    isLoading,
    error,
    selectedFileIds,
    searchQuery,
    connect,
    disconnect,
    browse,
    navigateUp,
    navigateToFolder,
    navigateToBreadcrumb,
    refresh,
    toggleFileSelection,
    selectAllFiles,
    clearSelection,
    setSearchQuery,
    clearError,
    reset,
  } = useCloudStore();

  // Filter files by search query
  const filteredFiles = useMemo(() => {
    if (!searchQuery.trim()) return files;
    const q = searchQuery.toLowerCase();
    return files.filter((f) => f.name.toLowerCase().includes(q));
  }, [files, searchQuery]);

  const filteredFolders = useMemo(() => {
    if (!searchQuery.trim()) return folders;
    const q = searchQuery.toLowerCase();
    return folders.filter((f) => f.name.toLowerCase().includes(q));
  }, [folders, searchQuery]);

  const selectedFiles = useMemo(
    () => files.filter((f) => selectedFileIds.has(f.id)),
    [files, selectedFileIds],
  );

  const audioFiles = useMemo(
    () => filteredFiles.filter((f) => isAudioFile(f.name) || isAudioMimeType(f.mimeType)),
    [filteredFiles],
  );

  const handleClose = useCallback(() => {
    reset();
    onClose();
  }, [reset, onClose]);

  const handleImportSelected = useCallback(() => {
    if (selectedFiles.length > 0 && onImport) {
      onImport(selectedFiles);
      handleClose();
    }
  }, [selectedFiles, onImport, handleClose]);

  const handleSelectProvider = useCallback(
    (provider: CloudProvider) => {
      browse(provider);
    },
    [browse],
  );

  const handleSelectCurrentFolder = useCallback(() => {
    if (currentProvider && onFolderSelect) {
      onFolderSelect(currentFolder, currentProvider);
      handleClose();
    }
  }, [currentProvider, currentFolder, onFolderSelect, handleClose]);

  const providerList: CloudProvider[] = ['google-drive', 'onedrive', 'dropbox'];
  const hasBrowser = currentProvider !== null;

  return (
    <Modal
      open={open}
      onClose={handleClose}
      title={folderSelectMode ? 'Select Destination Folder' : 'Import from Cloud'}
      size="xl"
    >
      <div className="space-y-4">
        {/* Provider cards */}
        <div className="grid grid-cols-3 gap-3">
          {providerList.map((provider) => (
            <ProviderCard
              key={provider}
              provider={provider}
              isConnected={connectedProviders.has(provider)}
              isConnecting={connectingProvider === provider}
              onConnect={() => connect(provider)}
              onSelect={() => handleSelectProvider(provider)}
              isActive={currentProvider === provider}
            />
          ))}
        </div>

        {/* Error banner */}
        {error && <ErrorBanner message={error} onDismiss={clearError} />}

        {/* File browser area */}
        {hasBrowser && (
          <div className="border border-border rounded-xl overflow-hidden">
            {/* Toolbar */}
            <div className="flex items-center gap-2 px-3 py-2 border-b border-border bg-surface-alt/50">
              {/* Back button */}
              <button
                onClick={navigateUp}
                disabled={!currentFolder}
                className="p-1.5 rounded-lg text-text-muted hover:text-text-primary
                  hover:bg-surface-alt disabled:opacity-30 disabled:pointer-events-none
                  transition-colors"
                title="Go back"
              >
                <svg width="16" height="16" viewBox="0 0 16 16" fill="none">
                  <path d="M10 4L6 8l4 4" stroke="currentColor" strokeWidth="1.5" strokeLinecap="round" strokeLinejoin="round" />
                </svg>
              </button>

              {/* Breadcrumbs */}
              <div className="flex-1 min-w-0">
                <Breadcrumbs
                  folderStack={folderStack}
                  currentFolder={currentFolder}
                  onNavigate={navigateToBreadcrumb}
                />
              </div>

              {/* Refresh */}
              <button
                onClick={refresh}
                disabled={isLoading}
                className="p-1.5 rounded-lg text-text-muted hover:text-text-primary
                  hover:bg-surface-alt transition-colors disabled:opacity-30"
                title="Refresh"
              >
                <svg
                  width="16"
                  height="16"
                  viewBox="0 0 16 16"
                  fill="none"
                  className={isLoading ? 'animate-spin' : ''}
                >
                  <path
                    d="M13.5 8A5.5 5.5 0 112.5 8a5.5 5.5 0 0111 0z"
                    stroke="currentColor"
                    strokeWidth="1.5"
                    strokeLinecap="round"
                    strokeDasharray="3 3"
                  />
                </svg>
              </button>

              {/* Disconnect */}
              <button
                onClick={() => currentProvider && disconnect(currentProvider)}
                className="p-1.5 rounded-lg text-text-muted hover:text-red-500
                  hover:bg-red-50 transition-colors"
                title="Disconnect"
              >
                <svg width="16" height="16" viewBox="0 0 16 16" fill="none">
                  <path
                    d="M10 3l3 3-3 3"
                    stroke="currentColor"
                    strokeWidth="1.5"
                    strokeLinecap="round"
                    strokeLinejoin="round"
                  />
                  <path
                    d="M13 6H7a3 3 0 00-3 3v2"
                    stroke="currentColor"
                    strokeWidth="1.5"
                    strokeLinecap="round"
                  />
                </svg>
              </button>
            </div>

            {/* Search bar */}
            <div className="px-3 py-2 border-b border-border">
              <div className="relative">
                <svg
                  width="14"
                  height="14"
                  viewBox="0 0 14 14"
                  fill="none"
                  className="absolute left-2.5 top-1/2 -translate-y-1/2 text-text-muted"
                >
                  <circle cx="6" cy="6" r="4" stroke="currentColor" strokeWidth="1.3" />
                  <path d="M9 9l3 3" stroke="currentColor" strokeWidth="1.3" strokeLinecap="round" />
                </svg>
                <input
                  type="text"
                  value={searchQuery}
                  onChange={(e) => setSearchQuery(e.target.value)}
                  placeholder="Filter files and folders..."
                  className="w-full pl-8 pr-3 py-1.5 text-xs bg-surface border border-border
                    rounded-lg text-text-primary placeholder:text-text-muted
                    focus:outline-none focus:border-accent-teal/50 focus:ring-1
                    focus:ring-accent-teal/20 transition-colors"
                />
                {searchQuery && (
                  <button
                    onClick={() => setSearchQuery('')}
                    className="absolute right-2 top-1/2 -translate-y-1/2 text-text-muted
                      hover:text-text-primary transition-colors"
                  >
                    <svg width="12" height="12" viewBox="0 0 12 12" fill="none">
                      <path d="M3 3l6 6M9 3l-6 6" stroke="currentColor" strokeWidth="1.3" strokeLinecap="round" />
                    </svg>
                  </button>
                )}
              </div>
            </div>

            {/* Select all / count header */}
            {!folderSelectMode && audioFiles.length > 0 && (
              <div className="flex items-center justify-between px-3 py-1.5 border-b border-border">
                <div className="flex items-center gap-2">
                  <button
                    onClick={selectedFileIds.size === audioFiles.length ? clearSelection : selectAllFiles}
                    className="text-[10px] text-accent-teal hover:text-accent-teal-dark
                      transition-colors font-medium"
                  >
                    {selectedFileIds.size === audioFiles.length ? 'Deselect all' : 'Select all'}
                  </button>
                  {selectedFileIds.size > 0 && (
                    <span className="text-[10px] text-text-muted">
                      {selectedFileIds.size} selected
                    </span>
                  )}
                </div>
                <span className="text-[10px] text-text-muted">
                  {audioFiles.length} audio {audioFiles.length === 1 ? 'file' : 'files'}
                </span>
              </div>
            )}

            {/* File / folder list */}
            <div className="max-h-72 overflow-y-auto px-1 py-1">
              {isLoading ? (
                <LoadingSkeleton />
              ) : filteredFolders.length === 0 && filteredFiles.length === 0 ? (
                <EmptyState hasSearch={!!searchQuery.trim()} />
              ) : (
                <div className="space-y-0.5">
                  {/* Folders first */}
                  {filteredFolders.map((folder) => (
                    <FolderRow
                      key={folder.id}
                      folder={folder}
                      onOpen={() => navigateToFolder(folder)}
                      folderSelectMode={folderSelectMode}
                      onSelectFolder={() =>
                        onFolderSelect && currentProvider &&
                        onFolderSelect(folder, currentProvider)
                      }
                    />
                  ))}

                  {/* Files */}
                  {filteredFiles.map((file) => (
                    <FileRow
                      key={file.id}
                      file={file}
                      isSelected={selectedFileIds.has(file.id)}
                      onToggleSelect={() => toggleFileSelection(file.id)}
                      onOpen={() => {
                        if (!folderSelectMode) {
                          toggleFileSelection(file.id);
                        }
                      }}
                      selectable={!folderSelectMode}
                    />
                  ))}
                </div>
              )}
            </div>
          </div>
        )}

        {/* Not-connected placeholder */}
        {!hasBrowser && !error && (
          <div className="flex flex-col items-center justify-center py-8 text-center">
            <div className="w-12 h-12 rounded-full bg-surface-alt flex items-center justify-center mb-3">
              <svg width="24" height="24" viewBox="0 0 24 24" fill="none" className="text-text-muted">
                <path
                  d="M12 16v-4m0 0V8m0 4h4m-4 0H8"
                  stroke="currentColor"
                  strokeWidth="1.5"
                  strokeLinecap="round"
                />
                <circle cx="12" cy="12" r="9" stroke="currentColor" strokeWidth="1.5" />
              </svg>
            </div>
            <p className="text-sm text-text-secondary">Connect a cloud provider to get started</p>
            <p className="text-xs text-text-muted mt-1">
              Select Google Drive, OneDrive, or Dropbox above
            </p>
          </div>
        )}

        {/* Footer actions */}
        <div className="flex items-center justify-between pt-2 border-t border-border">
          <Button variant="ghost" size="sm" onClick={handleClose}>
            Cancel
          </Button>

          <div className="flex items-center gap-2">
            {folderSelectMode ? (
              <Button
                variant="primary"
                size="sm"
                onClick={handleSelectCurrentFolder}
                disabled={!currentProvider}
              >
                {currentFolder
                  ? `Select "${currentFolder.name}"`
                  : 'Select Root Folder'}
              </Button>
            ) : (
              <Button
                variant="primary"
                size="sm"
                onClick={handleImportSelected}
                disabled={selectedFileIds.size === 0}
                icon={
                  <svg width="14" height="14" viewBox="0 0 14 14" fill="none">
                    <path
                      d="M7 2v8m0 0l3-3m-3 3L4 7"
                      stroke="currentColor"
                      strokeWidth="1.5"
                      strokeLinecap="round"
                      strokeLinejoin="round"
                    />
                    <path
                      d="M2 11h10"
                      stroke="currentColor"
                      strokeWidth="1.5"
                      strokeLinecap="round"
                    />
                  </svg>
                }
              >
                Import {selectedFileIds.size > 0 ? `(${selectedFileIds.size})` : 'Selected'}
              </Button>
            )}
          </div>
        </div>
      </div>
    </Modal>
  );
}
