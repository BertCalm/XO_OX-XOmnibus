'use client';

import React, { useState, useCallback, useEffect } from 'react';
import Button from '@/components/ui/Button';
import { isTauri } from '@/lib/tauri/tauriDetect';
import { detectMpcSdCard, exportToSdCard, formatBytes } from '@/lib/tauri/sdCardSync';
import type { SdCardInfo } from '@/lib/tauri/sdCardSync';
import { buildXpnPackage } from '@/lib/xpn/xpnPackager';
import type { XpnPackageConfig } from '@/lib/xpn/xpnTypes';
import { buildDrumProgram } from '@/lib/xpm/xpmDrumGenerator';
import { usePadStore } from '@/stores/padStore';
import { useAudioStore } from '@/stores/audioStore';
import { useToastStore } from '@/stores/toastStore';

export default function SdCardSyncPanel() {
  const [sdCard, setSdCard] = useState<SdCardInfo | null>(null);
  const [isScanning, setIsScanning] = useState(false);
  const [isExporting, setIsExporting] = useState(false);
  const [exportSuccess, setExportSuccess] = useState(false);
  const [error, setError] = useState<string | null>(null);

  const scanForCard = useCallback(async () => {
    setIsScanning(true);
    setError(null);
    setSdCard(null);

    try {
      const result = await detectMpcSdCard();
      setSdCard(result);
    } catch (err) {
      const message = err instanceof Error ? err.message : 'Detection failed';
      setError(message);
    } finally {
      setIsScanning(false);
    }
  }, []);

  // Auto-scan on mount when running in Tauri
  useEffect(() => {
    if (isTauri()) {
      scanForCard();
    }
  }, [scanForCard]);

  // Clear success feedback after 3 seconds
  useEffect(() => {
    if (!exportSuccess) return;
    const timer = setTimeout(() => setExportSuccess(false), 3000);
    return () => clearTimeout(timer);
  }, [exportSuccess]);

  const handleExport = useCallback(async () => {
    if (!sdCard) return;
    setIsExporting(true);
    setError(null);
    setExportSuccess(false);

    try {
      const { pads } = usePadStore.getState();
      const { samples } = useAudioStore.getState();
      const { addToast } = useToastStore.getState();

      // Check if there are any pads with samples
      const hasSamples = pads.some((p) => p.layers.some((l) => l.active && l.sampleId));
      if (!hasSamples) {
        addToast({ type: 'warning', title: 'No samples to export', message: 'Assign samples to pads first' });
        return;
      }

      // Build the XPM program content from current pad state
      const xpmContent = buildDrumProgram({
        programName: 'Program',
        padAssignments: pads,
      });

      // Collect all referenced sample buffers
      const sampleMap = new Map(samples.map((s) => [s.id, s]));
      const usedSamples: { fileName: string; data: ArrayBuffer }[] = [];
      const seenFiles = new Set<string>();

      for (const pad of pads) {
        for (const layer of pad.layers) {
          if (layer.active && layer.sampleId && layer.sampleFile && !seenFiles.has(layer.sampleFile)) {
            const sample = sampleMap.get(layer.sampleId);
            if (sample) {
              seenFiles.add(layer.sampleFile);
              usedSamples.push({ fileName: layer.sampleFile, data: sample.buffer });
            }
          }
        }
      }

      // Build the XPN package
      const config: XpnPackageConfig = {
        metadata: {
          title: 'SD Export',
          identifier: 'com.xoox.sdexport',
          description: 'Exported from XO_OX',
          creator: 'XO_OX',
          tags: [],
          version: '1.0',
        },
        programs: [{
          name: 'Program',
          xpmContent,
          groupName: 'Default',
          subName: 'Program',
        }],
        samples: usedSamples,
      };

      const blob = await buildXpnPackage(config);
      await exportToSdCard(blob, 'Program.xpn', sdCard);
      setExportSuccess(true);
      addToast({ type: 'success', title: 'Shipped to SD card', message: `${usedSamples.length} samples synced` });
    } catch (err) {
      const message = err instanceof Error ? err.message : 'Export failed';
      setError(message);
      useToastStore.getState().addToast({ type: 'error', title: 'SD export failed', message });
    } finally {
      setIsExporting(false);
    }
  }, [sdCard]);

  // Gate: only render in Tauri desktop environment
  if (!isTauri()) {
    return (
      <div className="p-3 bg-surface-alt rounded-lg border border-border">
        <p className="text-[10px] text-text-muted text-center">
          SD card sync is a desktop-only feature. Run this app via the Tauri desktop shell to enable it.
        </p>
      </div>
    );
  }

  return (
    <div className="space-y-3 p-3 bg-surface-alt rounded-lg border border-border">
      {/* Header */}
      <div className="flex items-center justify-between">
        <div className="flex items-center gap-1.5">
          <span className="text-sm" aria-hidden="true">
            {'\uD83D\uDCBE'}
          </span>
          <p className="text-[10px] text-text-muted uppercase tracking-wider font-semibold">
            SD Card Sync
          </p>
        </div>
        <Button
          variant="ghost"
          size="sm"
          loading={isScanning}
          onClick={scanForCard}
        >
          {isScanning ? 'Scanning...' : 'Rescan'}
        </Button>
      </div>

      {/* Error message */}
      {error && (
        <div className="px-2 py-1.5 rounded bg-red-500/10 border border-red-500/20">
          <p className="text-[10px] text-red-400">{error}</p>
        </div>
      )}

      {/* SD card info or empty state */}
      {sdCard ? (
        <div className="space-y-2">
          {/* Card details */}
          <div className="px-2 py-2 rounded bg-surface-bg border border-border space-y-1">
            <div className="flex items-center justify-between">
              <span className="text-[10px] text-text-secondary font-medium">
                {sdCard.label}
              </span>
              {sdCard.hasMpcStructure && (
                <span className="px-1.5 py-0.5 rounded text-[8px] font-semibold bg-accent-teal/10 text-accent-teal">
                  MPC
                </span>
              )}
            </div>
            <p className="text-[9px] text-text-muted">
              {sdCard.mountPoint}
            </p>
            <p className="text-[9px] text-text-muted">
              Available: {formatBytes(sdCard.availableSpace)}
            </p>
          </div>

          {/* Export button */}
          <Button
            variant="primary"
            size="sm"
            className="w-full"
            loading={isExporting}
            disabled={isExporting}
            onClick={handleExport}
          >
            {isExporting ? 'Exporting...' : 'Export to SD'}
          </Button>

          {/* Success feedback */}
          {exportSuccess && (
            <p className="text-[9px] text-accent-teal text-center">
              Export complete! Safely eject your SD card before removing it.
            </p>
          )}
        </div>
      ) : (
        !isScanning && (
          <div className="space-y-2 py-2">
            <p className="text-[10px] text-text-secondary text-center">
              No MPC SD card detected
            </p>
            <div className="px-2 space-y-1">
              <p className="text-[9px] text-text-muted">Tips:</p>
              <ul className="text-[9px] text-text-muted list-disc list-inside space-y-0.5">
                <li>Insert an SD card formatted by your MPC</li>
                <li>Ensure the card has an Expansions folder</li>
                <li>Try clicking Rescan after inserting</li>
              </ul>
            </div>
          </div>
        )
      )}
    </div>
  );
}
