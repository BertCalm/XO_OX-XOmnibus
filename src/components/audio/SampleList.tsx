'use client';

import React, { useState, useMemo, useCallback, useRef } from 'react';
import { useAudioStore } from '@/stores/audioStore';
import { formatDuration, formatFileSize } from '@/lib/audio/audioUtils';
import { midiNoteToName } from '@/types';
import { useToastStore } from '@/stores/toastStore';

type FilterMode = 'all' | 'favorites';

function SkeletonRow() {
  return (
    <div className="flex items-center gap-2 px-3 py-2 rounded-lg" aria-hidden="true">
      <div className="w-10 h-6 flex-shrink-0 rounded skeleton-shimmer" />
      <div className="flex-1 min-w-0 space-y-1.5">
        <div className="h-3 w-24 rounded skeleton-shimmer" />
        <div className="h-2.5 w-16 rounded skeleton-shimmer" />
      </div>
      <div className="w-6 h-6 rounded skeleton-shimmer" />
    </div>
  );
}

const SampleList = React.memo(function SampleList() {
  const samples = useAudioStore((s) => s.samples);
  const activeSampleId = useAudioStore((s) => s.activeSampleId);
  const setActiveSample = useAudioStore((s) => s.setActiveSample);
  const removeSample = useAudioStore((s) => s.removeSample);
  const renameSample = useAudioStore((s) => s.renameSample);
  const toggleFavorite = useAudioStore((s) => s.toggleFavorite);
  const isProcessing = useAudioStore((s) => s.isProcessing);
  const [editingId, setEditingId] = useState<string | null>(null);
  const [editName, setEditName] = useState('');
  const [filterMode, setFilterMode] = useState<FilterMode>('all');
  const [tagFilter, setTagFilter] = useState<string | null>(null);
  /** Ref flag to prevent onBlur from saving when Escape was pressed */
  const editCancelledRef = useRef(false);

  const handleStartEdit = (id: string, name: string) => {
    editCancelledRef.current = false;
    setEditingId(id);
    setEditName(name);
  };

  const handleSaveEdit = (id: string) => {
    if (editCancelledRef.current) {
      editCancelledRef.current = false;
      setEditingId(null);
      return;
    }
    if (editName.trim()) {
      renameSample(id, editName.trim());
      useToastStore.getState().addToast({ type: 'info', title: `Renamed to "${editName.trim()}"` });
    }
    setEditingId(null);
  };

  const handleToggleFavorite = useCallback(
    (e: React.MouseEvent, id: string) => {
      e.stopPropagation();
      toggleFavorite(id);
    },
    [toggleFavorite]
  );

  // Collect all unique tags for filter dropdown
  const allTags = useMemo(() => {
    const tagSet = new Set<string>();
    samples.forEach((s) => s.tags?.forEach((t) => tagSet.add(t)));
    return Array.from(tagSet).sort();
  }, [samples]);

  // Filtered samples
  const filteredSamples = useMemo(() => {
    let result = samples;
    if (filterMode === 'favorites') {
      result = result.filter((s) => s.isFavorite);
    }
    if (tagFilter) {
      result = result.filter((s) => s.tags?.includes(tagFilter));
    }
    return result;
  }, [samples, filterMode, tagFilter]);

  const favCount = useMemo(
    () => samples.filter((s) => s.isFavorite).length,
    [samples]
  );

  if (samples.length === 0) {
    // Show skeleton rows while importing
    if (isProcessing) {
      return (
        <div className="space-y-1" aria-label="Loading samples" role="status">
          <SkeletonRow />
          <SkeletonRow />
          <SkeletonRow />
        </div>
      );
    }
    return (
      <div className="flex flex-col items-center justify-center py-8 text-center">
        <div className="w-10 h-10 rounded-full bg-surface-alt flex items-center justify-center mb-2">
          <svg width="20" height="20" viewBox="0 0 20 20" fill="none" className="text-text-muted">
            <path d="M10 4v6l4 2" stroke="currentColor" strokeWidth="1.5" strokeLinecap="round" />
            <circle cx="10" cy="10" r="7" stroke="currentColor" strokeWidth="1.5" />
          </svg>
        </div>
        <p className="text-sm text-text-secondary">No samples yet</p>
        <p className="text-xs text-text-muted mt-1">Import audio files to get started</p>
      </div>
    );
  }

  return (
    <div className="space-y-2">
      {/* Filter bar */}
      <div className="flex items-center gap-1.5 px-1">
        <button
          onClick={() => { setFilterMode('all'); setTagFilter(null); }}
          className={`px-2 py-0.5 rounded text-[10px] font-medium transition-all
            ${filterMode === 'all' && !tagFilter
              ? 'bg-accent-teal text-white'
              : 'text-text-secondary hover:text-text-primary'
            }`}
        >
          All ({samples.length})
        </button>
        <button
          onClick={() => setFilterMode(filterMode === 'favorites' ? 'all' : 'favorites')}
          className={`px-2 py-0.5 rounded text-[10px] font-medium transition-all
            ${filterMode === 'favorites'
              ? 'bg-amber-500 text-white'
              : 'text-text-secondary hover:text-text-primary'
            }`}
        >
          ★ {favCount}
        </button>
        {allTags.length > 0 && (
          <select
            value={tagFilter ?? ''}
            onChange={(e) => setTagFilter(e.target.value || null)}
            className="ml-auto appearance-none px-1.5 py-0.5 rounded bg-surface border border-border
              text-[10px] text-text-secondary cursor-pointer hover:border-text-muted
              focus:outline-none focus:ring-1 focus:ring-accent-teal/30"
          >
            <option value="">All tags</option>
            {allTags.map((tag) => (
              <option key={tag} value={tag}>{tag}</option>
            ))}
          </select>
        )}
      </div>

      {/* Sample list */}
      <div className="space-y-1" role="listbox" aria-label="Sample library">
        {filteredSamples.map((sample) => (
          <div
            key={sample.id}
            role="option"
            aria-selected={activeSampleId === sample.id}
            tabIndex={0}
            className={`group flex items-center gap-2 px-3 py-2 rounded-lg cursor-pointer
              transition-all duration-100
              ${activeSampleId === sample.id
                ? 'bg-accent-teal-50 border border-accent-teal/20'
                : 'hover:bg-surface-alt border border-transparent'
              }`}
            onClick={() => setActiveSample(sample.id)}
            onKeyDown={(e) => {
              if (e.key === 'Enter' || e.key === ' ') {
                e.preventDefault();
                setActiveSample(sample.id);
              }
              if (e.key === 'Delete' || e.key === 'Backspace') {
                e.preventDefault();
                removeSample(sample.id);
              }
            }}
            draggable
            onDragStart={(e) => {
              e.dataTransfer.setData('sampleId', sample.id);
              e.dataTransfer.setData('sampleName', sample.name);
              e.dataTransfer.setData('sampleFile', sample.fileName);
            }}
          >
            {/* Favorite toggle */}
            <button
              onClick={(e) => handleToggleFavorite(e, sample.id)}
              aria-label={sample.isFavorite ? `Unfavorite ${sample.name}` : `Favorite ${sample.name}`}
              className={`text-sm flex-shrink-0 transition-colors ${
                sample.isFavorite
                  ? 'text-amber-500'
                  : 'text-transparent group-hover:text-text-muted/40'
              }`}
            >
              ★
            </button>

            {/* Waveform mini preview — single <path> instead of N <rect> elements */}
            <div className="w-10 h-6 flex-shrink-0 bg-surface-alt rounded overflow-hidden">
              {sample.waveformPeaks && sample.waveformPeaks.length > 0 && (
                <svg viewBox={`0 0 ${sample.waveformPeaks.length} 20`} className="w-full h-full" preserveAspectRatio="none">
                  <path
                    d={sample.waveformPeaks
                      .map((peak, i) => {
                        const y = 10 - peak * 9;
                        const h = peak * 18;
                        return `M${i},${y}v${h}`;
                      })
                      .join('')}
                    className={activeSampleId === sample.id ? 'stroke-accent-teal' : 'stroke-gray-400'}
                    strokeWidth="1"
                    fill="none"
                  />
                </svg>
              )}
            </div>

            <div className="flex-1 min-w-0">
              {editingId === sample.id ? (
                <input
                  value={editName}
                  onChange={(e) => setEditName(e.target.value)}
                  onBlur={() => handleSaveEdit(sample.id)}
                  onKeyDown={(e) => {
                    if (e.key === 'Enter') handleSaveEdit(sample.id);
                    if (e.key === 'Escape') {
                      editCancelledRef.current = true;
                      setEditingId(null);
                    }
                  }}
                  autoFocus
                  className="input-field text-xs py-0.5 px-1"
                  onClick={(e) => e.stopPropagation()}
                />
              ) : (
                <p className="text-xs font-medium text-text-primary truncate"
                  onDoubleClick={() => handleStartEdit(sample.id, sample.name)}>
                  {sample.name}
                </p>
              )}
              <div className="flex items-center gap-2 text-[10px] text-text-muted">
                <span>{formatDuration(sample.duration)}</span>
                <span>{formatFileSize(sample.buffer.byteLength)}</span>
                <span>{midiNoteToName(sample.rootNote)}</span>
              </div>
              {/* Tag chips */}
              {sample.tags && sample.tags.length > 0 && (
                <div className="flex flex-wrap gap-0.5 mt-0.5">
                  {sample.tags.map((tag) => (
                    <span
                      key={tag}
                      className="inline-block px-1 py-px rounded text-[8px] font-medium bg-accent-plum/10 text-accent-plum"
                    >
                      {tag}
                    </span>
                  ))}
                </div>
              )}
            </div>

            <button
              aria-label={`Remove ${sample.name}`}
              className="opacity-0 group-hover:opacity-100 focus:opacity-100 p-1 rounded text-text-muted
                hover:text-red-500 hover:bg-red-50 transition-all"
              onClick={(e) => {
                e.stopPropagation();
                removeSample(sample.id);
              }}
            >
              <svg width="14" height="14" viewBox="0 0 14 14" fill="none">
                <path d="M4 4l6 6M10 4l-6 6" stroke="currentColor" strokeWidth="1.5" strokeLinecap="round" />
              </svg>
            </button>
          </div>
        ))}

        {/* Filter empty state */}
        {filteredSamples.length === 0 && samples.length > 0 && (
          <p className="text-[10px] text-text-muted text-center py-4 italic">
            No samples match the current filter
          </p>
        )}
      </div>
    </div>
  );
});

export default SampleList;
