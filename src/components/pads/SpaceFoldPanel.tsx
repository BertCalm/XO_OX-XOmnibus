'use client';

import React, { useState, useCallback, useRef, useEffect, useMemo } from 'react';
import { usePadStore } from '@/stores/padStore';
import { calculateSpaceFoldPan, SPACE_FOLD_PRESETS } from '@/lib/audio/spaceFold';
import type { SpaceFoldConfig } from '@/lib/audio/spaceFold';
import Button from '@/components/ui/Button';

// ---------------------------------------------------------------------------
// Mini stereo field visualization
// ---------------------------------------------------------------------------

interface StereoFieldVizProps {
  /** Array of { velCenter (0-127), pan (0-1) } for each active layer */
  layers: { velCenter: number; pan: number }[];
}

function StereoFieldViz({ layers }: StereoFieldVizProps) {
  const width = 200;
  const height = 48;
  const padding = 8;
  const innerWidth = width - padding * 2;

  return (
    <svg
      width={width}
      height={height}
      viewBox={`0 0 ${width} ${height}`}
      className="block w-full"
    >
      {/* Center line */}
      <line
        x1={width / 2}
        y1={4}
        x2={width / 2}
        y2={height - 4}
        stroke="currentColor"
        strokeOpacity={0.15}
        strokeWidth={1}
        strokeDasharray="2 2"
      />
      {/* L/R labels */}
      <text x={padding} y={10} fill="currentColor" fillOpacity={0.3} fontSize={7} fontFamily="monospace">L</text>
      <text x={width - padding - 4} y={10} fill="currentColor" fillOpacity={0.3} fontSize={7} fontFamily="monospace">R</text>

      {/* Layer bars */}
      {layers.map((layer, i) => {
        // Y position: distribute layers vertically
        const y = 16 + ((height - 24) / Math.max(layers.length - 1, 1)) * i;
        // X position: pan 0 = full left, 0.5 = center, 1 = full right
        const x = padding + layer.pan * innerWidth;
        const centerX = width / 2;

        // Color: velocity gradient from cool (low vel) to hot (high vel)
        const t = layer.velCenter / 127;
        const hue = 200 - t * 200; // 200 (blue/cool) -> 0 (red/hot)
        const saturation = 70 + t * 20;
        const lightness = 50;

        return (
          <g key={i}>
            {/* Line from center to pan position */}
            <line
              x1={centerX}
              y1={y}
              x2={x}
              y2={y}
              stroke={`hsl(${hue}, ${saturation}%, ${lightness}%)`}
              strokeWidth={3}
              strokeLinecap="round"
              opacity={0.8}
            />
            {/* Dot at pan position */}
            <circle
              cx={x}
              cy={y}
              r={2.5}
              fill={`hsl(${hue}, ${saturation}%, ${lightness}%)`}
            />
          </g>
        );
      })}
    </svg>
  );
}

// ---------------------------------------------------------------------------
// SpaceFoldPanel
// ---------------------------------------------------------------------------

export default function SpaceFoldPanel() {
  const [isOpen, setIsOpen] = useState(false);
  const [selectedPresetId, setSelectedPresetId] = useState<string>('subtle');
  const dropdownRef = useRef<HTMLDivElement>(null);

  const pads = usePadStore((s) => s.pads);
  const activePadIndex = usePadStore((s) => s.activePadIndex);
  const updateLayer = usePadStore((s) => s.updateLayer);
  const withHistory = usePadStore((s) => s.withHistory);

  const pad = pads[activePadIndex];
  const activeLayers = useMemo(
    () => (pad ? pad.layers.filter((l) => l.active && l.sampleId) : []),
    [pad]
  );
  const layerCount = activeLayers.length;

  const selectedPreset = SPACE_FOLD_PRESETS.find((p) => p.id === selectedPresetId) || SPACE_FOLD_PRESETS[0];

  // Close dropdown on outside click
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

  // Compute preview pan values for the visualization
  const previewLayers = useMemo(() => {
    if (!pad || layerCount === 0) return [];

    const panResults = calculateSpaceFoldPan(pad, selectedPreset.config);
    return panResults.map((result) => {
      const layer = pad.layers[result.layerIndex];
      const velCenter = (layer.velStart + layer.velEnd) / 2;
      return { velCenter, pan: result.pan };
    });
  }, [pad, layerCount, selectedPreset]);

  // Apply Space Fold to a single pad — reads fresh state to avoid stale closure
  const applyToPad = useCallback(
    (padIndex: number, freshPads: typeof pads) => {
      const targetPad = freshPads[padIndex];
      if (!targetPad) return;

      const panResults = calculateSpaceFoldPan(targetPad, selectedPreset.config);
      for (const result of panResults) {
        updateLayer(padIndex, result.layerIndex, { pan: result.pan });
      }
    },
    [selectedPreset, updateLayer],
  );

  const handleApplyCurrent = useCallback(() => {
    withHistory('Apply Space Fold', () => {
      const freshPads = usePadStore.getState().pads;
      applyToPad(activePadIndex, freshPads);
    });
    setIsOpen(false);
  }, [applyToPad, activePadIndex, withHistory]);

  const handleApplyAll = useCallback(() => {
    withHistory('Apply Space Fold to all', () => {
      const freshPads = usePadStore.getState().pads;
      for (let i = 0; i < freshPads.length; i++) {
        const p = freshPads[i];
        const hasActiveLayers = p.layers.some((l) => l.active && l.sampleId);
        if (hasActiveLayers) {
          applyToPad(i, freshPads);
        }
      }
    });
    setIsOpen(false);
  }, [applyToPad, withHistory]);

  if (!pad) return null;

  return (
    <div className="relative inline-block" ref={dropdownRef}>
      {/* Trigger button */}
      <button
        onClick={() => setIsOpen((prev) => !prev)}
        disabled={layerCount === 0}
        className="px-2 py-1 rounded text-[10px] font-medium
          bg-surface-alt text-text-secondary hover:text-text-primary
          hover:bg-surface transition-all disabled:opacity-50
          border border-transparent hover:border-border"
        title={
          layerCount === 0
            ? 'Assign at least one sample to use Space Fold'
            : 'Apply velocity-based stereo imaging to layers'
        }
      >
        {'\u{1F30C}'} Space Fold
      </button>

      {/* Dropdown popover */}
      {isOpen && (
        <div
          className="absolute z-50 mt-1 left-0 w-[280px]
            bg-surface border border-border rounded-lg shadow-card
            p-2 space-y-2"
        >
          {/* Header */}
          <div className="flex items-center justify-between px-1">
            <p className="text-[10px] text-text-muted uppercase tracking-wider font-semibold">
              Space Fold Stereo Imaging
            </p>
            <span className="text-[9px] text-text-muted">
              {layerCount} layer{layerCount !== 1 ? 's' : ''}
            </span>
          </div>

          {/* Preset grid (2x2) */}
          <div className="grid grid-cols-2 gap-1.5">
            {SPACE_FOLD_PRESETS.map((preset) => (
              <button
                key={preset.id}
                onClick={() => setSelectedPresetId(preset.id)}
                className={`group flex flex-col items-start gap-0.5 p-2 rounded-lg
                  transition-all border text-left
                  ${selectedPresetId === preset.id
                    ? 'bg-accent-teal/10 border-accent-teal/30 text-accent-teal'
                    : 'bg-surface-alt border-transparent text-text-muted hover:text-text-secondary hover:border-border'
                  }`}
                title={preset.description}
              >
                <div className="flex items-center gap-1.5">
                  <span className="text-sm">{preset.icon}</span>
                  <span className="text-[9px] font-semibold leading-tight">
                    {preset.name}
                  </span>
                </div>
                <span className="text-[8px] opacity-60 leading-tight">
                  {preset.description}
                </span>
              </button>
            ))}
          </div>

          {/* Stereo field visualization */}
          {previewLayers.length > 0 && (
            <div className="space-y-1">
              <p className="text-[10px] text-text-muted uppercase tracking-wider px-1">
                Stereo Field Preview
              </p>
              <div className="bg-surface-alt rounded-md p-1.5 text-text-muted">
                <StereoFieldViz layers={previewLayers} />
              </div>
              <div className="flex justify-between text-[8px] text-text-muted px-2">
                <span>Left</span>
                <span>Center</span>
                <span>Right</span>
              </div>
            </div>
          )}

          {/* Velocity zone info */}
          {layerCount > 0 && (
            <div className="px-1">
              <p className="text-[9px] text-text-muted">
                Velocity zones:{' '}
                {activeLayers.map((l, i) => (
                  <span key={i}>
                    {i > 0 && ', '}
                    <span className="text-text-secondary font-mono">
                      {l.velStart}-{l.velEnd}
                    </span>
                  </span>
                ))}
              </p>
            </div>
          )}

          {/* Action buttons */}
          <div className="flex gap-1 pt-1">
            <Button
              variant="primary"
              size="sm"
              disabled={layerCount === 0}
              onClick={handleApplyCurrent}
            >
              Apply to Current Pad
            </Button>
            <Button
              variant="secondary"
              size="sm"
              disabled={layerCount === 0}
              onClick={handleApplyAll}
            >
              Apply to All Loaded
            </Button>
          </div>

          {/* Footer hint */}
          <p className="text-[8px] text-text-muted text-center px-2">
            Sets pan per layer based on velocity zone. Harder hits sound physically wider.
          </p>
        </div>
      )}
    </div>
  );
}
