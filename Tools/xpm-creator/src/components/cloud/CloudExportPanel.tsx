'use client';

import React, { useState, useCallback } from 'react';
import Button from '@/components/ui/Button';
import Card from '@/components/ui/Card';
import ProgressBar from '@/components/ui/ProgressBar';
import { useCloudStore } from '@/stores/cloudStore';
import {
  CLOUD_PROVIDERS,
  type CloudProvider,
  type CloudFolder,
} from '@/lib/cloud/cloudStorageTypes';
import CloudBrowser from './CloudBrowser';

// ---------------------------------------------------------------------------
// Types
// ---------------------------------------------------------------------------

export type ExportFormat = 'xpm' | 'xpn' | 'wav';

interface ExportFormatOption {
  value: ExportFormat;
  label: string;
  description: string;
  mimeType: string;
  extension: string;
}

const EXPORT_FORMATS: ExportFormatOption[] = [
  {
    value: 'xpm',
    label: 'XPM Program',
    description: 'MPC program file with all pad assignments and settings',
    mimeType: 'application/octet-stream',
    extension: '.xpm',
  },
  {
    value: 'xpn',
    label: 'XPN Expansion',
    description: 'Full expansion pack with programs, samples, and cover art',
    mimeType: 'application/octet-stream',
    extension: '.xpn',
  },
  {
    value: 'wav',
    label: 'WAV Samples',
    description: 'Individual WAV audio samples from the current project',
    mimeType: 'audio/wav',
    extension: '.wav',
  },
];

// ---------------------------------------------------------------------------
// Provider icon mini (smaller for the destination list)
// ---------------------------------------------------------------------------

function ProviderDot({ provider }: { provider: CloudProvider }) {
  const info = CLOUD_PROVIDERS[provider];
  return (
    <span
      className="inline-block w-2.5 h-2.5 rounded-full flex-shrink-0"
      style={{ backgroundColor: info.brandColor }}
    />
  );
}

// ---------------------------------------------------------------------------
// Sub-components
// ---------------------------------------------------------------------------

function DestinationSelector({
  providers,
  selected,
  onSelect,
}: {
  providers: CloudProvider[];
  selected: CloudProvider | null;
  onSelect: (p: CloudProvider) => void;
}) {
  if (providers.length === 0) {
    return (
      <div className="flex flex-col items-center justify-center py-6 text-center
        border border-dashed border-border rounded-lg"
      >
        <p className="text-xs text-text-secondary">No cloud providers connected</p>
        <p className="text-[10px] text-text-muted mt-1">
          Open the Cloud Browser to connect Google Drive, OneDrive, or Dropbox
        </p>
      </div>
    );
  }

  return (
    <div className="space-y-1">
      {providers.map((p) => {
        const info = CLOUD_PROVIDERS[p];
        const isActive = selected === p;
        return (
          <button
            key={p}
            onClick={() => onSelect(p)}
            className={`flex items-center gap-2.5 w-full px-3 py-2 rounded-lg
              border transition-all duration-100 text-left
              ${isActive
                ? 'border-accent-teal bg-accent-teal-50'
                : 'border-border bg-surface hover:bg-surface-alt hover:border-border-hover'
              }`}
          >
            <ProviderDot provider={p} />
            <span className="text-xs font-medium text-text-primary">{info.displayName}</span>
            <span className="w-1.5 h-1.5 rounded-full bg-green-500 ml-auto" />
          </button>
        );
      })}
    </div>
  );
}

function FormatSelector({
  selected,
  onSelect,
}: {
  selected: ExportFormat;
  onSelect: (f: ExportFormat) => void;
}) {
  return (
    <div className="space-y-1">
      {EXPORT_FORMATS.map((fmt) => {
        const isActive = selected === fmt.value;
        return (
          <button
            key={fmt.value}
            onClick={() => onSelect(fmt.value)}
            className={`flex items-start gap-2.5 w-full px-3 py-2 rounded-lg
              border transition-all duration-100 text-left
              ${isActive
                ? 'border-accent-teal bg-accent-teal-50'
                : 'border-border bg-surface hover:bg-surface-alt hover:border-border-hover'
              }`}
          >
            <div className="flex-1 min-w-0">
              <p className={`text-xs font-medium
                ${isActive ? 'text-text-primary' : 'text-text-secondary'}`}
              >
                {fmt.label}
                <span className="ml-1.5 text-[10px] text-text-muted font-normal">
                  {fmt.extension}
                </span>
              </p>
              <p className="text-[10px] text-text-muted mt-0.5">{fmt.description}</p>
            </div>
            <div
              className={`w-4 h-4 rounded-full border-2 flex items-center justify-center
                flex-shrink-0 mt-0.5 transition-colors
                ${isActive ? 'border-accent-teal' : 'border-border'}`}
            >
              {isActive && (
                <span className="w-2 h-2 rounded-full bg-accent-teal" />
              )}
            </div>
          </button>
        );
      })}
    </div>
  );
}

// ---------------------------------------------------------------------------
// Status types for export progress
// ---------------------------------------------------------------------------

type ExportStatus = 'idle' | 'exporting' | 'success' | 'error';

// ---------------------------------------------------------------------------
// Main component
// ---------------------------------------------------------------------------

interface CloudExportPanelProps {
  /** Project/file data to export. Callers provide this from the export pipeline. */
  getData?: () => Promise<ArrayBuffer>;
  /** Default file name (without extension) */
  defaultFileName?: string;
  /** Optional class name */
  className?: string;
}

export default function CloudExportPanel({
  getData,
  defaultFileName = 'Untitled',
  className = '',
}: CloudExportPanelProps) {
  const { connectedProviders, exportToCloud } = useCloudStore();

  const providers = Array.from(connectedProviders) as CloudProvider[];

  // --- local state ---
  const [selectedProvider, setSelectedProvider] = useState<CloudProvider | null>(null);
  const [selectedFormat, setSelectedFormat] = useState<ExportFormat>('xpm');
  const [fileName, setFileName] = useState(defaultFileName);
  const [destinationFolder, setDestinationFolder] = useState<CloudFolder | null>(null);
  const [showFolderBrowser, setShowFolderBrowser] = useState(false);
  const [status, setStatus] = useState<ExportStatus>('idle');
  const [progress, setProgress] = useState(0);
  const [errorMessage, setErrorMessage] = useState<string | null>(null);

  const formatInfo = EXPORT_FORMATS.find((f) => f.value === selectedFormat)!;
  const fullFileName = `${fileName}${formatInfo.extension}`;

  const canExport =
    selectedProvider !== null &&
    fileName.trim().length > 0 &&
    getData !== undefined &&
    status !== 'exporting';

  // --- handlers ---

  const handleFolderSelect = useCallback(
    (folder: CloudFolder | null, _provider: CloudProvider) => {
      setDestinationFolder(folder);
      setShowFolderBrowser(false);
    },
    [],
  );

  const handleExport = useCallback(async () => {
    if (!canExport || !selectedProvider || !getData) return;

    setStatus('exporting');
    setProgress(0);
    setErrorMessage(null);

    let progressInterval: ReturnType<typeof setInterval> | undefined;
    try {
      // Simulate progress ticks while waiting for data generation
      progressInterval = setInterval(() => {
        setProgress((prev) => Math.min(prev + 8, 80));
      }, 200);

      const data = await getData();
      clearInterval(progressInterval);
      progressInterval = undefined;
      setProgress(85);

      await exportToCloud(
        selectedProvider,
        fullFileName,
        data,
        formatInfo.mimeType,
        destinationFolder?.id,
      );

      setProgress(100);
      setStatus('success');
    } catch (err) {
      // Clear the progress ticker on failure — without this, the interval
      // runs forever after a failed export, causing phantom state updates.
      if (progressInterval) clearInterval(progressInterval);
      setStatus('error');
      setErrorMessage(
        err instanceof Error ? err.message : 'Export failed. Please try again.',
      );
    }
  }, [canExport, selectedProvider, getData, fullFileName, formatInfo.mimeType, destinationFolder, exportToCloud]);

  const handleReset = useCallback(() => {
    setStatus('idle');
    setProgress(0);
    setErrorMessage(null);
  }, []);

  return (
    <>
      <Card className={className} padding="lg">
        <div className="space-y-5">
          {/* Header */}
          <div className="flex items-center gap-2">
            <svg width="18" height="18" viewBox="0 0 18 18" fill="none" className="text-accent-teal">
              <path
                d="M9 12V3m0 0L6 6m3-3l3 3"
                stroke="currentColor"
                strokeWidth="1.5"
                strokeLinecap="round"
                strokeLinejoin="round"
              />
              <path
                d="M3 12v2a2 2 0 002 2h8a2 2 0 002-2v-2"
                stroke="currentColor"
                strokeWidth="1.5"
                strokeLinecap="round"
              />
            </svg>
            <h3 className="text-sm font-semibold text-text-primary">Export to Cloud</h3>
          </div>

          {/* Destination provider */}
          <div>
            <label className="block text-xs font-medium text-text-secondary mb-2">
              Destination
            </label>
            <DestinationSelector
              providers={providers}
              selected={selectedProvider}
              onSelect={setSelectedProvider}
            />
          </div>

          {/* Export format */}
          <div>
            <label className="block text-xs font-medium text-text-secondary mb-2">
              Format
            </label>
            <FormatSelector selected={selectedFormat} onSelect={setSelectedFormat} />
          </div>

          {/* File name */}
          <div>
            <label className="block text-xs font-medium text-text-secondary mb-2">
              File Name
            </label>
            <div className="flex items-center gap-2">
              <input
                type="text"
                value={fileName}
                onChange={(e) => setFileName(e.target.value)}
                placeholder="Enter file name..."
                className="flex-1 px-3 py-2 text-xs bg-surface border border-border rounded-lg
                  text-text-primary placeholder:text-text-muted
                  focus:outline-none focus:border-accent-teal/50 focus:ring-1
                  focus:ring-accent-teal/20 transition-colors"
              />
              <span className="text-xs text-text-muted flex-shrink-0">
                {formatInfo.extension}
              </span>
            </div>
          </div>

          {/* Folder selection */}
          <div>
            <label className="block text-xs font-medium text-text-secondary mb-2">
              Folder
            </label>
            <button
              onClick={() => setShowFolderBrowser(true)}
              disabled={!selectedProvider}
              className="flex items-center gap-2 w-full px-3 py-2 rounded-lg border border-border
                bg-surface text-left hover:bg-surface-alt hover:border-border-hover
                disabled:opacity-50 disabled:pointer-events-none transition-colors"
            >
              <svg width="14" height="14" viewBox="0 0 14 14" fill="none" className="text-text-muted flex-shrink-0">
                <path
                  d="M2 4a1 1 0 011-1h2.5l1 1H11a1 1 0 011 1v5a1 1 0 01-1 1H3a1 1 0 01-1-1V4z"
                  fill="currentColor"
                />
              </svg>
              <span className="text-xs text-text-primary truncate flex-1">
                {destinationFolder ? destinationFolder.name : 'Root folder'}
              </span>
              <span className="text-[10px] text-text-muted flex-shrink-0">Browse</span>
            </button>
          </div>

          {/* Progress / status */}
          {status === 'exporting' && (
            <div className="space-y-2">
              <ProgressBar
                progress={progress}
                label="Uploading..."
                detail={`${Math.round(progress)}%`}
                size="sm"
              />
            </div>
          )}

          {status === 'success' && (
            <div className="flex items-center gap-2 p-3 rounded-lg bg-green-50 border border-green-200">
              <svg width="16" height="16" viewBox="0 0 16 16" fill="none" className="text-green-500 flex-shrink-0">
                <circle cx="8" cy="8" r="6" stroke="currentColor" strokeWidth="1.5" />
                <path d="M5.5 8l2 2 3.5-3.5" stroke="currentColor" strokeWidth="1.5" strokeLinecap="round" strokeLinejoin="round" />
              </svg>
              <div className="flex-1 min-w-0">
                <p className="text-xs font-medium text-green-700">Export complete</p>
                <p className="text-[10px] text-green-600 mt-0.5 truncate">
                  {fullFileName} uploaded to{' '}
                  {selectedProvider ? CLOUD_PROVIDERS[selectedProvider].displayName : 'cloud'}
                  {destinationFolder ? ` / ${destinationFolder.name}` : ''}
                </p>
              </div>
              <button
                onClick={handleReset}
                className="text-green-400 hover:text-green-600 transition-colors flex-shrink-0"
              >
                <svg width="14" height="14" viewBox="0 0 14 14" fill="none">
                  <path d="M4 4l6 6M10 4l-6 6" stroke="currentColor" strokeWidth="1.5" strokeLinecap="round" />
                </svg>
              </button>
            </div>
          )}

          {status === 'error' && (
            <div className="flex items-start gap-2 p-3 rounded-lg bg-red-50 border border-red-200">
              <svg width="16" height="16" viewBox="0 0 16 16" fill="none" className="text-red-500 flex-shrink-0 mt-0.5">
                <circle cx="8" cy="8" r="6" stroke="currentColor" strokeWidth="1.5" />
                <path d="M8 5v3" stroke="currentColor" strokeWidth="1.5" strokeLinecap="round" />
                <circle cx="8" cy="11" r="0.75" fill="currentColor" />
              </svg>
              <div className="flex-1 min-w-0">
                <p className="text-xs font-medium text-red-700">Export failed</p>
                <p className="text-[10px] text-red-600 mt-0.5">{errorMessage}</p>
              </div>
              <button
                onClick={handleReset}
                className="text-red-400 hover:text-red-600 transition-colors flex-shrink-0"
              >
                <svg width="14" height="14" viewBox="0 0 14 14" fill="none">
                  <path d="M4 4l6 6M10 4l-6 6" stroke="currentColor" strokeWidth="1.5" strokeLinecap="round" />
                </svg>
              </button>
            </div>
          )}

          {/* Export button */}
          <Button
            variant="primary"
            size="md"
            className="w-full"
            disabled={!canExport}
            loading={status === 'exporting'}
            onClick={handleExport}
            icon={
              <svg width="14" height="14" viewBox="0 0 14 14" fill="none">
                <path
                  d="M7 10V2m0 0L4 5m3-3l3 3"
                  stroke="currentColor"
                  strokeWidth="1.5"
                  strokeLinecap="round"
                  strokeLinejoin="round"
                />
                <path
                  d="M2 10v1.5a1 1 0 001 1h8a1 1 0 001-1V10"
                  stroke="currentColor"
                  strokeWidth="1.5"
                  strokeLinecap="round"
                />
              </svg>
            }
          >
            Export to Cloud
          </Button>

          {/* Setup hint when no providers are connected */}
          {providers.length === 0 && (
            <p className="text-[10px] text-text-muted text-center leading-relaxed">
              Configure your cloud storage API keys in settings, then connect a
              provider through the Cloud Browser to enable cloud export.
            </p>
          )}
        </div>
      </Card>

      {/* Folder browser modal (reuses CloudBrowser in folder-select mode) */}
      <CloudBrowser
        open={showFolderBrowser}
        onClose={() => setShowFolderBrowser(false)}
        folderSelectMode
        onFolderSelect={handleFolderSelect}
      />
    </>
  );
}
