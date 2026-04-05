'use client';

import React, { useCallback, useState, useMemo } from 'react';
import type { PadAssignment } from '@/types';

interface PadCellProps {
  pad: PadAssignment;
  index: number;
  globalIndex: number;
  isActive: boolean;
  isPlaying: boolean;
  isCelebrating?: boolean;
  isSelected?: boolean;
  isDecoding?: boolean;
  onClickPad: (globalIndex: number, e?: React.MouseEvent) => void;
  onPlayPad: (globalIndex: number, velocity: number) => void;
  onDropSample: (padIndex: number, sampleId: string, sampleName: string, sampleFile: string) => void;
  onDropFiles?: (padIndex: number, files: File[]) => void;
  onContextMenu?: (globalIndex: number, x: number, y: number) => void;
}

/** Audio MIME types and extensions we accept for layer stacking drops */
const AUDIO_EXTENSIONS = /\.(wav|aif|aiff|mp3|ogg|flac|m4a)$/i;

/** Layer dot color classes — hoisted to module scope (never changes) */
const LAYER_COLORS = [
  'bg-accent-teal',
  'bg-accent-plum',
  'bg-accent-teal/60',
  'bg-accent-plum/60',
  'bg-sky-400',
  'bg-amber-400',
  'bg-sky-400/60',
  'bg-amber-400/60',
];

function isAudioFile(file: File): boolean {
  if (file.type && file.type.startsWith('audio/')) return true;
  return AUDIO_EXTENSIONS.test(file.name);
}

const PadCell = React.memo(function PadCell({
  pad,
  index,
  globalIndex,
  isActive,
  isPlaying,
  isCelebrating = false,
  isSelected = false,
  isDecoding = false,
  onClickPad,
  onPlayPad,
  onDropSample,
  onDropFiles,
  onContextMenu,
}: PadCellProps) {
  const activeLayers = useMemo(
    () => pad.layers.filter((l) => l.active && l.sampleId),
    [pad.layers]
  );
  const hasContent = activeLayers.length > 0;

  // Drag-over state for visual feedback
  const [isDragOver, setIsDragOver] = useState(false);
  const [dragFileCount, setDragFileCount] = useState(0);

  const handleDragEnter = useCallback((e: React.DragEvent) => {
    e.preventDefault();
    e.stopPropagation();

    const items = e.dataTransfer.items;
    let fileCount = 0;
    if (items && items.length > 0) {
      for (let i = 0; i < items.length; i++) {
        if (items[i].kind === 'file') fileCount++;
      }
    }
    if (fileCount === 0 && e.dataTransfer.types.includes('Files')) {
      fileCount = items ? items.length : 1;
    }

    setIsDragOver(true);
    setDragFileCount(fileCount);
  }, []);

  const handleDragOver = useCallback((e: React.DragEvent) => {
    e.preventDefault();
    e.stopPropagation();
    e.dataTransfer.dropEffect = 'copy';
  }, []);

  const handleDragLeave = useCallback((e: React.DragEvent) => {
    e.preventDefault();
    e.stopPropagation();
    const rect = e.currentTarget.getBoundingClientRect();
    const x = e.clientX;
    const y = e.clientY;
    if (x <= rect.left || x >= rect.right || y <= rect.top || y >= rect.bottom) {
      setIsDragOver(false);
      setDragFileCount(0);
    }
  }, []);

  const handleDrop = useCallback(
    (e: React.DragEvent) => {
      e.preventDefault();
      e.stopPropagation();
      setIsDragOver(false);
      setDragFileCount(0);

      // Check for OS file drops first
      if (e.dataTransfer.files && e.dataTransfer.files.length > 0 && onDropFiles) {
        const audioFiles: File[] = [];
        for (let i = 0; i < e.dataTransfer.files.length; i++) {
          const file = e.dataTransfer.files[i];
          if (isAudioFile(file)) audioFiles.push(file);
        }
        if (audioFiles.length > 0) {
          onDropFiles(index, audioFiles.slice(0, 8));
          return;
        }
      }

      // Fall back to internal app sample drag
      const sampleId = e.dataTransfer.getData('sampleId');
      const sampleName = e.dataTransfer.getData('sampleName');
      const sampleFile = e.dataTransfer.getData('sampleFile');
      if (sampleId) {
        onDropSample(index, sampleId, sampleName, sampleFile);
      }
    },
    [index, onDropSample, onDropFiles]
  );

  const handleClick = useCallback((e?: React.MouseEvent) => {
    onClickPad(globalIndex, e);
    // Only play on plain clicks — modifier keys indicate multi-select, not triggering
    if (hasContent && !e?.shiftKey && !e?.metaKey && !e?.ctrlKey) {
      // Derive velocity from vertical click position within the pad:
      //   top of pad   → velocity 1  (soft)
      //   bottom of pad → velocity 127 (hard)
      // This makes the layer system accessible to mouse users without extra UI.
      let velocity = 100; // default when no mouse event (e.g. keyboard activation)
      if (e) {
        const rect = e.currentTarget.getBoundingClientRect();
        const relY = e.clientY - rect.top;
        const ratio = Math.max(0, Math.min(1, relY / rect.height));
        velocity = Math.max(1, Math.round(ratio * 127));
      }
      onPlayPad(globalIndex, velocity);
    }
  }, [globalIndex, onClickPad, onPlayPad, hasContent]);

  const handleKeyDown = useCallback(
    (e: React.KeyboardEvent) => {
      if (e.key === 'Enter' || e.key === ' ') {
        e.preventDefault();
        handleClick();
      }
    },
    [handleClick]
  );

  const dragOverLabel = useMemo(() => {
    if (!isDragOver) return null;
    if (dragFileCount > 1) {
      const capped = Math.min(dragFileCount, 8);
      return `Drop ${capped} file${capped > 1 ? 's' : ''} for layers`;
    }
    if (dragFileCount === 1) return 'Drop 1 file';
    return 'Drop files';
  }, [isDragOver, dragFileCount]);

  const handleContextMenu = useCallback(
    (e: React.MouseEvent) => {
      e.preventDefault();
      e.stopPropagation();
      onContextMenu?.(globalIndex, e.clientX, e.clientY);
    },
    [globalIndex, onContextMenu]
  );

  return (
    <div
      className={`pad-cell relative
        ${isActive ? 'pad-cell-active' : ''}
        ${hasContent ? 'pad-cell-loaded' : ''}
        ${isPlaying ? 'pad-cell-playing' : ''}
        ${isCelebrating ? 'pad-cell-celebrate' : ''}
        ${isSelected ? 'pad-cell-selected' : ''}
        ${isDragOver ? 'pad-cell-dragover ring-2 ring-accent-teal ring-inset' : ''}`}
      role="button"
      tabIndex={0}
      aria-label={`Pad ${index + 1}${hasContent ? `, ${activeLayers[0].sampleName}` : ', empty'}${isSelected ? ' (selected)' : ''}`}
      aria-pressed={isActive}
      onClick={handleClick}
      onKeyDown={handleKeyDown}
      onContextMenu={handleContextMenu}
      onDragEnter={handleDragEnter}
      onDragOver={handleDragOver}
      onDragLeave={handleDragLeave}
      onDrop={handleDrop}
    >
      {/* Decoding overlay */}
      {isDecoding && (
        <div className="absolute inset-0 z-10 flex flex-col items-center justify-center bg-surface/80 rounded pointer-events-none">
          <div className="w-4 h-4 border-2 border-accent-teal border-t-transparent rounded-full animate-spin" />
          <span className="text-[9px] font-medium text-accent-teal mt-1">Decoding…</span>
        </div>
      )}

      {/* Drag-over overlay with layer count */}
      {isDragOver && !isDecoding && (
        <div className="absolute inset-0 z-10 flex flex-col items-center justify-center bg-accent-teal/20 rounded pointer-events-none">
          <span className="text-[10px] font-semibold text-accent-teal text-center leading-tight px-1">
            {dragOverLabel}
          </span>
          <span className="text-[8px] text-accent-teal/70 mt-0.5">
            Layer {activeLayers.length + 1}/8
          </span>
        </div>
      )}

      {/* Pad number */}
      <span className="absolute top-1 left-1.5 text-[10px] font-mono text-text-muted">
        {index + 1}
      </span>

      {/* Trigger mode indicator */}
      {hasContent && pad.triggerMode !== 'oneshot' && (
        <span className="absolute top-0.5 right-1 text-[8px] font-medium text-text-muted" title={`Trigger: ${pad.triggerMode}`}>
          {pad.triggerMode === 'noteon' ? '⟳' : '⏹'}
        </span>
      )}

      {/* Mute group indicator */}
      {pad.muteGroup > 0 && (
        <span
          className="absolute top-0.5 right-1 text-[7px] font-bold leading-none px-0.5 rounded bg-accent-plum/20 text-accent-plum"
          style={{ top: hasContent && pad.triggerMode !== 'oneshot' ? '12px' : '2px' }}
          title={`Mute Group ${pad.muteGroup}`}
        >
          M{pad.muteGroup}
        </span>
      )}

      {/* Layer indicators */}
      {hasContent && (
        <div className="absolute bottom-1 left-1/2 -translate-x-1/2 flex gap-0.5">
          {activeLayers.map((_, i) => (
            <div
              key={i}
              className={`w-1.5 h-1.5 rounded-full transition-colors ${LAYER_COLORS[i] || 'bg-border'}`}
            />
          ))}
        </div>
      )}

      {/* Sample name preview */}
      {hasContent && !isDragOver && (
        <div className="absolute inset-0 flex items-center justify-center p-1">
          <span className="text-[9px] text-text-secondary text-center leading-tight truncate max-w-full px-1">
            {activeLayers[0].sampleName}
            {activeLayers.length > 1 && ` +${activeLayers.length - 1}`}
          </span>
        </div>
      )}
    </div>
  );
});

export default PadCell;
