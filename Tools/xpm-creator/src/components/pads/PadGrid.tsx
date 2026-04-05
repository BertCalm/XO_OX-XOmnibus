'use client';

import React, { useCallback, useState, useEffect, useRef, useMemo } from 'react';
import { v4 as uuid } from 'uuid';
import PadCell from './PadCell';
import { usePadStore } from '@/stores/padStore';
import { useAudioStore } from '@/stores/audioStore';
import { usePlaybackStore } from '@/stores/playbackStore';
import { decodeAudioFile, generateWaveformPeaks } from '@/lib/audio/audioUtils';
import { encodeWavAsync } from '@/lib/audio/wavEncoder';
import { playDropSound, playSuccessChime } from '@/lib/audio/uiSounds';
import ConfirmDialog from '@/components/ui/ConfirmDialog';
import ContextMenu, { type ContextMenuItem } from '@/components/ui/ContextMenu';
import Select from '@/components/ui/Select';
import UndoTimeline from '@/components/tools/UndoTimeline';
import { useHistoryStore } from '@/stores/historyStore';
import { useToastStore } from '@/stores/toastStore';
import { PAD_MAP_TEMPLATES } from '@/constants/padMapTemplates';
import type { AudioSample } from '@/types';
import type { PadSelectMode } from '@/stores/padStore';

const BANK_LABELS = ['A', 'B', 'C', 'D', 'E', 'F', 'G', 'H'];

/**
 * Spiral order for the celebration light show.
 */
const SPIRAL_ORDER = [
  12, 13, 14, 15,
  11, 7, 3,
  2, 1, 0,
  4, 8,
  9, 10,
  6, 5,
];

/** MPC layout: bottom-left to top-right (constant) */
const DISPLAY_ORDER = [
  12, 13, 14, 15,
  8, 9, 10, 11,
  4, 5, 6, 7,
  0, 1, 2, 3,
];

function computeVelocitySplits(count: number): Array<{ velStart: number; velEnd: number }> {
  if (count <= 0) return [];
  if (count === 1) return [{ velStart: 0, velEnd: 127 }];
  if (count === 2) return [{ velStart: 0, velEnd: 63 }, { velStart: 64, velEnd: 127 }];
  if (count === 3) return [{ velStart: 0, velEnd: 42 }, { velStart: 43, velEnd: 84 }, { velStart: 85, velEnd: 127 }];
  if (count === 4) return [{ velStart: 0, velEnd: 31 }, { velStart: 32, velEnd: 63 }, { velStart: 64, velEnd: 95 }, { velStart: 96, velEnd: 127 }];

  const splits: Array<{ velStart: number; velEnd: number }> = [];
  const rangePerLayer = 128 / count;
  for (let i = 0; i < count; i++) {
    const velStart = Math.round(i * rangePerLayer);
    const velEnd = i === count - 1 ? 127 : Math.round((i + 1) * rangePerLayer) - 1;
    splits.push({ velStart, velEnd });
  }
  return splits;
}

export default function PadGrid() {
  // Granular Zustand selectors — only re-render when these specific values change
  const pads = usePadStore((s) => s.pads);
  const activePadIndex = usePadStore((s) => s.activePadIndex);
  const currentBank = usePadStore((s) => s.currentBank);
  const copiedPad = usePadStore((s) => s.copiedPad);
  const setActivePad = usePadStore((s) => s.setActivePad);
  const setCurrentBank = usePadStore((s) => s.setCurrentBank);
  const assignSampleToLayer = usePadStore((s) => s.assignSampleToLayer);
  const updateLayer = usePadStore((s) => s.updateLayer);
  const setPadPlayMode = usePadStore((s) => s.setPadPlayMode);
  const copyPad = usePadStore((s) => s.copyPad);
  const pastePad = usePadStore((s) => s.pastePad);
  const clearPad = usePadStore((s) => s.clearPad);
  const selectedPadIndices = usePadStore((s) => s.selectedPadIndices);
  const selectPad = usePadStore((s) => s.selectPad);
  const clearSelection = usePadStore((s) => s.clearSelection);
  const applyPadMap = usePadStore((s) => s.applyPadMap);
  const withHistory = usePadStore((s) => s.withHistory);

  const addSample = useAudioStore((s) => s.addSample);
  const playingPads = usePlaybackStore((s) => s.playingPads);
  const triggerPad = usePlaybackStore((s) => s.triggerPad);

  const [showCopied, setShowCopied] = useState(false);
  const [showClearConfirm, setShowClearConfirm] = useState(false);
  const [contextMenu, setContextMenu] = useState<{ x: number; y: number; padIndex: number } | null>(null);
  const [activePadMap, setActivePadMap] = useState('default');
  const [decodingPadIndex, setDecodingPadIndex] = useState<number | null>(null);

  const handleCopy = useCallback(() => {
    copyPad(activePadIndex);
    setShowCopied(true);
  }, [copyPad, activePadIndex]);

  const handlePaste = useCallback(() => {
    withHistory('Paste pad', () => pastePad(activePadIndex));
    useToastStore.getState().addToast({ type: 'success', title: `Pasted to Pad ${(activePadIndex % 16) + 1}` });
  }, [pastePad, activePadIndex, withHistory]);

  const activePadLayerCount = useMemo(() => {
    const pad = pads[activePadIndex];
    return pad ? pad.layers.filter((l) => l.active && l.sampleId).length : 0;
  }, [pads, activePadIndex]);

  const handleClear = useCallback(() => {
    if (activePadLayerCount > 0) {
      setShowClearConfirm(true);
    } else {
      withHistory('Clear pad', () => clearPad(activePadIndex));
    }
  }, [clearPad, activePadIndex, activePadLayerCount, withHistory]);

  const confirmClear = useCallback(() => {
    setShowClearConfirm(false);
    withHistory('Clear pad', () => clearPad(activePadIndex));
  }, [clearPad, activePadIndex, withHistory]);

  useEffect(() => {
    if (!showCopied) return;
    const timer = setTimeout(() => setShowCopied(false), 2000);
    return () => clearTimeout(timer);
  }, [showCopied]);

  const bankOffset = currentBank * 16;
  const visiblePads = useMemo(
    () => pads.slice(bankOffset, bankOffset + 16),
    [pads, bankOffset]
  );

  // Per-bank fill counts (how many of the 16 pads have sample content)
  const bankFillCounts = useMemo(() => {
    return BANK_LABELS.map((_, bankIdx) => {
      const offset = bankIdx * 16;
      let filled = 0;
      for (let p = 0; p < 16; p++) {
        const pad = pads[offset + p];
        if (pad && pad.layers.some((l) => l.active && l.sampleId)) filled++;
      }
      return filled;
    });
  }, [pads]);

  // ---------------------------------------------------------------------------
  // Light Show Celebration
  // ---------------------------------------------------------------------------
  const [celebratingPadIdx, setCelebratingPadIdx] = useState<number | null>(null);
  const wasFull = useRef(false);
  /** Track the last bank to prevent spurious celebration on bank switch */
  const lastBankRef = useRef(currentBank);

  const allPadsFilled = useMemo(
    () => visiblePads.every((pad) => pad.layers.some((l) => l.active && l.sampleId)),
    [visiblePads]
  );

  useEffect(() => {
    // Bank switch — update tracking without triggering celebration.
    // Without this guard, switching to a previously-filled bank
    // would fire the celebration animation again.
    if (lastBankRef.current !== currentBank) {
      lastBankRef.current = currentBank;
      wasFull.current = allPadsFilled;
      return;
    }

    if (allPadsFilled && !wasFull.current) {
      playSuccessChime();
      let step = 0;
      const interval = setInterval(() => {
        if (step < SPIRAL_ORDER.length) {
          setCelebratingPadIdx(SPIRAL_ORDER[step]);
          step++;
        } else {
          setCelebratingPadIdx(null);
          clearInterval(interval);
        }
      }, 50);
      wasFull.current = true;
      return () => {
        clearInterval(interval);
        setCelebratingPadIdx(null);
      };
    }
    wasFull.current = allPadsFilled;
  }, [allPadsFilled, currentBank]);

  // ---------------------------------------------------------------------------
  // Stable callback handlers — PadCell passes its index, avoiding inline closures
  // ---------------------------------------------------------------------------

  const handleClickPad = useCallback(
    (globalIndex: number, e?: React.MouseEvent) => {
      // Multi-select: Shift+click = range, Cmd/Ctrl+click = add
      if (e?.shiftKey) {
        selectPad(globalIndex, 'range');
      } else if (e?.metaKey || e?.ctrlKey) {
        selectPad(globalIndex, 'add');
      } else {
        clearSelection();
      }
      setActivePad(globalIndex);
    },
    [setActivePad, selectPad, clearSelection]
  );

  const handlePlayPad = useCallback(
    (globalIndex: number, velocity: number) => {
      triggerPad(globalIndex, velocity);
    },
    [triggerPad]
  );

  // Context menu for individual pads
  const handlePadContextMenu = useCallback(
    (globalIndex: number, x: number, y: number) => {
      setActivePad(globalIndex);
      setContextMenu({ x, y, padIndex: globalIndex });
    },
    [setActivePad]
  );

  const contextMenuItems = useMemo<ContextMenuItem[]>(() => {
    if (!contextMenu) return [];
    const idx = contextMenu.padIndex;
    const pad = pads[idx];
    const hasContent = pad?.layers.some((l) => l.active && l.sampleId);
    return [
      { id: 'play', label: 'Play Pad', action: () => triggerPad(idx), disabled: !hasContent },
      { id: 'copy', label: 'Copy', shortcut: '⌘C', action: () => { copyPad(idx); setShowCopied(true); } },
      { id: 'paste', label: 'Paste', shortcut: '⌘V', action: () => { withHistory('Paste pad', () => pastePad(idx)); useToastStore.getState().addToast({ type: 'success', title: `Pasted to Pad ${(idx % 16) + 1}` }); }, disabled: !copiedPad },
      { id: 'div1', label: '', divider: true, action: () => {} },
      { id: 'clear', label: 'Clear Pad', danger: true, action: () => withHistory('Clear pad', () => clearPad(idx)), disabled: !hasContent },
    ];
  }, [contextMenu, pads, copiedPad, triggerPad, copyPad, pastePad, clearPad, withHistory]);

  // Pad map template handler
  const handlePadMapChange = useCallback(
    (e: React.ChangeEvent<HTMLSelectElement>) => {
      const templateId = e.target.value;
      setActivePadMap(templateId);
      const template = PAD_MAP_TEMPLATES.find((t) => t.id === templateId);
      if (template) {
        withHistory('Apply pad map template', () => applyPadMap(template.noteMap));
        useToastStore.getState().addToast({ type: 'info', title: `Pad map "${template.name}" loaded` });
      }
    },
    [applyPadMap, withHistory]
  );

  // Batch actions for multi-selected pads
  const selectionCount = selectedPadIndices.size;
  const handleBatchClear = useCallback(() => {
    withHistory('Clear selected pads', () => {
      selectedPadIndices.forEach((idx) => clearPad(idx));
    });
    clearSelection();
    useToastStore.getState().addToast({ type: 'info', title: `${selectionCount} pads wiped clean` });
  }, [selectedPadIndices, selectionCount, clearPad, clearSelection, withHistory]);

  const handleDropSample = useCallback(
    (padIndex: number, sampleId: string, sampleName: string, sampleFile: string) => {
      const globalIndex = bankOffset + padIndex;
      // Read live pad state to avoid stale closure after rapid consecutive drops
      const livePad = usePadStore.getState().pads[globalIndex];
      const firstEmptyLayer = livePad.layers.findIndex((l) => !l.active || !l.sampleId);
      const layerIndex = firstEmptyLayer >= 0 ? firstEmptyLayer : 0;
      // Look up sample rootNote from audioStore to preserve pitch detection result
      const audioSamples = useAudioStore.getState().samples;
      const sampleData = audioSamples.find((s) => s.id === sampleId);
      withHistory('Assign sample', () =>
        assignSampleToLayer(globalIndex, layerIndex, sampleId, sampleName, sampleFile, sampleData?.rootNote)
      );
      playDropSound();
    },
    [bankOffset, assignSampleToLayer, withHistory]
  );

  const handleDropFiles = useCallback(
    async (padIndex: number, files: File[]) => {
      const globalIndex = bankOffset + padIndex;
      const filesToProcess = files.slice(0, 8);
      setDecodingPadIndex(globalIndex);

      const results = await Promise.allSettled(
        filesToProcess.map(async (file) => {
          const audioBuffer = await decodeAudioFile(file);
          const waveformPeaks = generateWaveformPeaks(audioBuffer);
          const wavBuffer = await encodeWavAsync(audioBuffer, 16);
          const sampleId = uuid();
          const sample: AudioSample = {
            id: sampleId,
            name: file.name.replace(/\.[^/.]+$/, ''),
            fileName: file.name,
            duration: audioBuffer.duration,
            sampleRate: audioBuffer.sampleRate,
            channels: audioBuffer.numberOfChannels,
            bitDepth: 16,
            buffer: wavBuffer,
            waveformPeaks,
            rootNote: 60,
            createdAt: Date.now(),
          };
          return { sample, file };
        })
      );

      const processedSamples: AudioSample[] = [];
      const failedCount = results.filter((r) => r.status === 'rejected').length;
      for (const result of results) {
        if (result.status === 'fulfilled') {
          processedSamples.push(result.value.sample);
        }
      }
      if (processedSamples.length === 0) {
        setDecodingPadIndex(null);
        useToastStore.getState().addToast({
          type: 'error',
          title: `Failed to decode ${failedCount} file${failedCount !== 1 ? 's' : ''}`,
          message: 'Unsupported format or corrupt audio',
        });
        return;
      }
      if (failedCount > 0) {
        useToastStore.getState().addToast({
          type: 'warning',
          title: `${processedSamples.length} loaded, ${failedCount} failed`,
        });
      }

      for (const sample of processedSamples) addSample(sample);

      // Snapshot pads for undo before applying mutations
      const history = useHistoryStore.getState();
      history.snapshot(usePadStore.getState().pads);

      for (let i = 0; i < processedSamples.length; i++) {
        const sample = processedSamples[i];
        assignSampleToLayer(globalIndex, i, sample.id, sample.name, sample.fileName);
      }

      const splits = computeVelocitySplits(processedSamples.length);
      for (let i = 0; i < splits.length; i++) {
        updateLayer(globalIndex, i, { velStart: splits[i].velStart, velEnd: splits[i].velEnd });
      }

      if (processedSamples.length > 1) setPadPlayMode(globalIndex, 'velocity');

      history.pushState('Drop files to pad', usePadStore.getState().pads);
      setDecodingPadIndex(null);
      playDropSound();
      setActivePad(globalIndex);
    },
    [bankOffset, addSample, assignSampleToLayer, updateLayer, setPadPlayMode, setActivePad]
  );

  return (
    <div className="space-y-3">
      {/* Bank selector with fill indicators */}
      <div className="flex items-center gap-1">
        {BANK_LABELS.map((label, i) => (
          <button
            key={label}
            onClick={() => setCurrentBank(i)}
            aria-pressed={currentBank === i}
            aria-label={`Bank ${label} — ${bankFillCounts[i]} of 16 pads filled`}
            className={`flex flex-col items-center gap-0.5 px-2.5 py-1 rounded text-xs font-mono font-medium transition-all
              ${currentBank === i
                ? 'bg-accent-teal text-white'
                : 'bg-surface-alt text-text-secondary hover:text-text-primary'
              }`}
          >
            {label}
            {/* Mini 4×4 dot grid — each dot = one pad */}
            <div className="grid grid-cols-4 gap-px">
              {Array.from({ length: 16 }, (_, padIdx) => {
                const pad = pads[i * 16 + padIdx];
                const isFilled = pad?.layers.some((l) => l.active && l.sampleId);
                return (
                  <span
                    key={padIdx}
                    className={`block w-[3px] h-[3px] rounded-full transition-colors ${
                      isFilled
                        ? currentBank === i
                          ? 'bg-white/80'
                          : 'bg-accent-teal'
                        : currentBank === i
                          ? 'bg-white/20'
                          : 'bg-border'
                    }`}
                  />
                );
              })}
            </div>
          </button>
        ))}
      </div>

      {/* Pad toolbar + pad map selector */}
      <div className="flex items-center gap-1.5 flex-wrap">
        <button onClick={handleCopy} className="px-2 py-1 rounded text-[10px] font-medium bg-surface-alt text-text-secondary hover:text-text-primary hover:bg-surface transition-all" title="Copy active pad" aria-label={`Copy pad ${activePadIndex + 1}`}>Copy</button>
        <button onClick={handlePaste} disabled={!copiedPad} className="px-2 py-1 rounded text-[10px] font-medium bg-surface-alt text-text-secondary hover:text-text-primary hover:bg-surface transition-all disabled:opacity-50 disabled:pointer-events-none" title="Paste to active pad" aria-label={`Paste to pad ${activePadIndex + 1}`}>Paste</button>
        <button onClick={handleClear} className="px-2 py-1 rounded text-[10px] font-medium bg-surface-alt text-text-secondary hover:text-red-400 hover:bg-red-500/10 transition-all" title="Clear active pad" aria-label={`Clear pad ${activePadIndex + 1}`}>Clear</button>
        {(showCopied || copiedPad) && (
          <span className={`ml-1 text-[9px] font-medium transition-opacity duration-300 ${showCopied ? 'text-accent-teal opacity-100' : 'text-text-muted opacity-70'}`}>
            {showCopied ? 'Copied!' : 'Clipboard ready'}
          </span>
        )}
        {/* Pad map template selector */}
        <div className="ml-auto">
          <Select
            options={PAD_MAP_TEMPLATES.map((t) => ({ value: t.id, label: t.name }))}
            value={activePadMap}
            onChange={handlePadMapChange}
            size="sm"
            aria-label="Pad map template"
          />
        </div>
      </div>

      {/* Batch action toolbar (visible when multi-selecting) */}
      {selectionCount > 1 && (
        <div className="flex items-center gap-1.5 px-2 py-1.5 bg-accent-plum-50 border border-accent-plum/20 rounded-lg">
          <span className="text-[10px] font-medium text-accent-plum">{selectionCount} pads selected</span>
          <div className="flex-1" />
          <button onClick={handleBatchClear} className="px-2 py-0.5 rounded text-[10px] font-medium text-red-500 hover:bg-red-500/10 transition-all">Clear All</button>
          <button onClick={clearSelection} className="px-2 py-0.5 rounded text-[10px] font-medium text-text-muted hover:text-text-primary transition-all">Deselect</button>
        </div>
      )}

      {/* 4x4 Pad Grid */}
      <div className="grid grid-cols-4 gap-2">
        {DISPLAY_ORDER.map((padIdx) => {
          const globalIdx = bankOffset + padIdx;
          return (
            <PadCell
              key={globalIdx}
              pad={visiblePads[padIdx] || pads[globalIdx]}
              index={padIdx}
              globalIndex={globalIdx}
              isActive={activePadIndex === globalIdx}
              isPlaying={!!playingPads[globalIdx]}
              isCelebrating={celebratingPadIdx === padIdx}
              isSelected={selectedPadIndices.has(globalIdx)}
              isDecoding={decodingPadIndex === globalIdx}
              onClickPad={handleClickPad}
              onPlayPad={handlePlayPad}
              onDropSample={handleDropSample}
              onDropFiles={handleDropFiles}
              onContextMenu={handlePadContextMenu}
            />
          );
        })}
      </div>

      {/* Empty state hint */}
      {!allPadsFilled && !visiblePads.some((p) => p.layers.some((l) => l.active && l.sampleId)) && (
        <p className="text-[9px] text-text-muted text-center mt-1 italic">
          Drag audio files onto pads or import samples above
        </p>
      )}

      {/* Undo Timeline */}
      <UndoTimeline />

      {/* Clear Pad confirmation */}
      <ConfirmDialog
        open={showClearConfirm}
        onConfirm={confirmClear}
        onCancel={() => setShowClearConfirm(false)}
        title={`Clear Pad ${activePadIndex + 1}?`}
        message={`This will remove all ${activePadLayerCount} layer(s) and their settings from this pad. Press Cmd+Z to undo after clearing.`}
        confirmLabel="Clear Pad"
        variant="danger"
      />

      {/* Pad context menu */}
      {contextMenu && (
        <ContextMenu
          items={contextMenuItems}
          x={contextMenu.x}
          y={contextMenu.y}
          onClose={() => setContextMenu(null)}
        />
      )}
    </div>
  );
}
