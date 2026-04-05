'use client';

import React, { useState, useRef, useEffect, useCallback } from 'react';
import { usePadStore } from '@/stores/padStore';
import { generateVibeUpdates, VIBE_PRESETS } from '@/lib/audio/vibeCheck';
import type { PadLayer } from '@/types';
import Slider from '@/components/ui/Slider';

/** Snapshot of a single layer's values before vibe-check was applied. */
interface LayerSnapshot {
  layerIndex: number;
  values: Partial<PadLayer>;
}

/** Snapshot of all pads' layer values before vibe-check was applied. */
interface VibeSnapshot {
  /** padIndex -> array of layer snapshots */
  [padIndex: number]: LayerSnapshot[];
}

export default function VibeCheckButton() {
  const [isOpen, setIsOpen] = useState(false);
  const [customAmount, setCustomAmount] = useState(50);
  const [appliedFeedback, setAppliedFeedback] = useState(false);
  const [snapshot, setSnapshot] = useState<VibeSnapshot | null>(null);
  const dropdownRef = useRef<HTMLDivElement>(null);

  const pads = usePadStore((s) => s.pads);
  const activePadIndex = usePadStore((s) => s.activePadIndex);
  const updateLayer = usePadStore((s) => s.updateLayer);
  const withHistory = usePadStore((s) => s.withHistory);

  // Close on outside click
  useEffect(() => {
    if (!isOpen) return;
    function handleClick(e: MouseEvent) {
      if (dropdownRef.current && !dropdownRef.current.contains(e.target as Node)) {
        setIsOpen(false);
      }
    }
    document.addEventListener('mousedown', handleClick);
    return () => document.removeEventListener('mousedown', handleClick);
  }, [isOpen]);

  // Animated feedback: clear pulse after 600ms
  useEffect(() => {
    if (!appliedFeedback) return;
    const t = setTimeout(() => setAppliedFeedback(false), 600);
    return () => clearTimeout(t);
  }, [appliedFeedback]);

  /**
   * Capture the current values of all properties that generateVibeUpdates may change
   * for a set of pads, so we can revert later.
   */
  const captureSnapshot = useCallback(
    (padIndices: number[]): VibeSnapshot => {
      const snap: VibeSnapshot = {};
      for (const pi of padIndices) {
        const pad = pads[pi];
        if (!pad) continue;
        const layerSnaps: LayerSnapshot[] = [];
        for (let li = 0; li < pad.layers.length; li++) {
          const layer = pad.layers[li];
          if (!layer.active || !layer.sampleId) continue;
          layerSnaps.push({
            layerIndex: li,
            values: {
              tuneFine: layer.tuneFine,
              volume: layer.volume,
              pan: layer.pan,
              pitchRandom: layer.pitchRandom,
              volumeRandom: layer.volumeRandom,
              panRandom: layer.panRandom,
              offset: layer.offset,
              probability: layer.probability,
            },
          });
        }
        snap[pi] = layerSnaps;
      }
      return snap;
    },
    [pads]
  );

  /**
   * Apply vibe-check randomization to a set of pads at the given intensity.
   */
  const applyVibe = useCallback(
    (padIndices: number[], amount: number) => {
      // Capture originals before applying (only if we haven't already)
      if (!snapshot) {
        setSnapshot(captureSnapshot(padIndices));
      }

      withHistory('Apply vibe randomization', () => {
        // Read fresh pads from store — the closed-over `pads` from React state
        // may be stale if other mutations occurred between render and this callback.
        const freshPads = usePadStore.getState().pads;
        for (const pi of padIndices) {
          const pad = freshPads[pi];
          if (!pad) continue;
          const updates = generateVibeUpdates(pad, amount);
          for (const { layerIndex, updates: layerUpdates } of updates) {
            updateLayer(pi, layerIndex, layerUpdates);
          }
        }
      });

      setAppliedFeedback(true);
    },
    [pads, updateLayer, snapshot, captureSnapshot, withHistory]
  );

  /**
   * Reset all pads to their pre-vibe-check values.
   */
  const resetAll = useCallback(() => {
    if (!snapshot) return;
    withHistory('Reset vibe', () => {
      for (const piStr of Object.keys(snapshot)) {
        const pi = Number(piStr);
        const layers = snapshot[pi];
        for (const { layerIndex, values } of layers) {
          updateLayer(pi, layerIndex, values);
        }
      }
    });
    setSnapshot(null);
    setAppliedFeedback(true);
  }, [snapshot, updateLayer, withHistory]);

  /** All pad indices */
  const allPadIndices = pads.map((_, i) => i);

  // Count pads that have at least one active sample
  const loadedPadCount = pads.filter((p) =>
    p.layers.some((l) => l.active && l.sampleId)
  ).length;

  if (!isOpen) {
    return (
      <button
        onClick={() => setIsOpen(true)}
        className={`px-2 py-1 rounded text-[10px] font-medium
          bg-surface-alt text-text-secondary hover:text-text-primary
          hover:bg-surface transition-all
          border border-transparent hover:border-border
          ${appliedFeedback ? 'animate-pulse ring-1 ring-accent-teal/50' : ''}`}
        title="Apply subtle random humanization across pad layers"
      >
        \u2728 Vibe Check
      </button>
    );
  }

  return (
    <div
      ref={dropdownRef}
      className="space-y-3 p-3 bg-surface-alt rounded-lg border border-border"
    >
      {/* Header */}
      <div className="flex items-center justify-between">
        <p className="text-[10px] text-text-muted uppercase tracking-wider font-semibold">
          Vibe Check
        </p>
        <button
          onClick={() => setIsOpen(false)}
          className="text-text-muted hover:text-text-primary text-xs"
          aria-label="Close Vibe Check panel"
        >
          \u2715
        </button>
      </div>

      {/* Preset buttons */}
      <div className="space-y-1.5">
        <p className="text-[10px] text-text-muted uppercase tracking-wider">Presets</p>
        <div className="grid grid-cols-3 gap-1">
          {VIBE_PRESETS.map((preset) => (
            <button
              key={preset.id}
              onClick={() => applyVibe(allPadIndices, preset.amount)}
              className="px-1 py-1.5 rounded-lg text-center transition-all border
                bg-surface border-transparent text-text-muted hover:text-text-secondary
                hover:border-accent-teal/30 hover:bg-accent-teal/5 active:scale-95"
            >
              <span className="block text-xs">{preset.icon}</span>
              <span className="block text-[8px] font-medium mt-0.5 leading-tight">
                {preset.name}
              </span>
              <span className="block text-[7px] opacity-50 leading-tight mt-0.5">
                {preset.description}
              </span>
            </button>
          ))}
        </div>
      </div>

      {/* Custom amount slider */}
      <Slider
        label="Custom Amount"
        value={customAmount}
        onChange={setCustomAmount}
        min={0}
        max={100}
        step={1}
        unit="%"
      />

      {/* Action buttons */}
      <div className="flex flex-col gap-1.5">
        <button
          onClick={() => applyVibe(allPadIndices, customAmount / 100)}
          disabled={loadedPadCount === 0}
          className="w-full px-2 py-1.5 rounded text-[10px] font-medium transition-all
            bg-accent-teal text-white hover:bg-accent-teal-dark
            disabled:opacity-50 disabled:pointer-events-none active:scale-[0.98]"
        >
          Apply to All Pads ({loadedPadCount})
        </button>

        <button
          onClick={() => applyVibe([activePadIndex], customAmount / 100)}
          disabled={
            !pads[activePadIndex]?.layers.some((l) => l.active && l.sampleId)
          }
          className="w-full px-2 py-1.5 rounded text-[10px] font-medium transition-all
            bg-accent-plum text-white hover:bg-accent-plum-dark
            disabled:opacity-50 disabled:pointer-events-none active:scale-[0.98]"
        >
          Apply to Current Pad
        </button>

        <button
          onClick={resetAll}
          disabled={!snapshot}
          className="w-full px-2 py-1.5 rounded text-[10px] font-medium transition-all
            bg-surface text-text-secondary border border-border
            hover:text-text-primary hover:bg-surface-alt
            disabled:opacity-50 disabled:pointer-events-none active:scale-[0.98]"
        >
          Reset All
        </button>
      </div>

      {/* Status line */}
      {snapshot && (
        <p className="text-[9px] text-accent-teal text-center">
          Vibe applied \u2014 originals saved for reset
        </p>
      )}
    </div>
  );
}
