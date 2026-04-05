'use client';

import React, { useState, useCallback, useMemo, useRef, useEffect } from 'react';
import { usePadStore } from '@/stores/padStore';
import {
  VELOCITY_CURVES,
  applyVelocityCurveToLayers,
} from '@/lib/audio/velocityCurves';
import type { VelocityCurveType, VelocityCurve } from '@/lib/audio/velocityCurves';

// ---------------------------------------------------------------------------
// Mini SVG curve preview
// ---------------------------------------------------------------------------

/** Sample a curve function at N points and return an SVG polyline path string. */
function curveSvgPath(
  curve: VelocityCurve,
  width: number,
  height: number,
  samples = 10,
): string {
  const points: string[] = [];
  for (let i = 0; i <= samples; i++) {
    const velocity = Math.round((i / samples) * 127);
    const value = curve.fn(velocity);
    const x = (i / samples) * width;
    const y = height - value * height;
    points.push(`${x.toFixed(1)},${y.toFixed(1)}`);
  }
  return `M${points.join(' L')}`;
}

function CurvePreview({ curve, size = { w: 32, h: 20 } }: { curve: VelocityCurve; size?: { w: number; h: number } }) {
  const d = useMemo(() => curveSvgPath(curve, size.w, size.h), [curve, size.w, size.h]);
  return (
    <svg
      width={size.w}
      height={size.h}
      viewBox={`0 0 ${size.w} ${size.h}`}
      className="block"
    >
      {/* faint axis lines */}
      <line x1={0} y1={size.h} x2={size.w} y2={size.h} stroke="currentColor" strokeOpacity={0.15} strokeWidth={0.5} />
      <line x1={0} y1={0} x2={0} y2={size.h} stroke="currentColor" strokeOpacity={0.15} strokeWidth={0.5} />
      {/* curve */}
      <path d={d} fill="none" stroke="currentColor" strokeWidth={1.5} strokeLinecap="round" strokeLinejoin="round" />
    </svg>
  );
}

// ---------------------------------------------------------------------------
// VelocityCurveSelector
// ---------------------------------------------------------------------------

export default function VelocityCurveSelector() {
  const [isOpen, setIsOpen] = useState(false);
  const dropdownRef = useRef<HTMLDivElement>(null);

  const pads = usePadStore((s) => s.pads);
  const activePadIndex = usePadStore((s) => s.activePadIndex);
  const updateLayer = usePadStore((s) => s.updateLayer);
  const setPadPlayMode = usePadStore((s) => s.setPadPlayMode);
  const withHistory = usePadStore((s) => s.withHistory);

  const pad = pads[activePadIndex];
  const activeLayers = pad ? pad.layers.filter((l) => l.active && l.sampleId) : [];
  const layerCount = activeLayers.length;

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

  const handleSelectCurve = useCallback(
    (curveType: VelocityCurveType) => {
      if (!pad || layerCount === 0) return;

      withHistory('Apply velocity curve', () => {
        const zones = applyVelocityCurveToLayers(layerCount, curveType);

        // Map zones onto active layers only
        let zoneIdx = 0;
        for (let i = 0; i < pad.layers.length; i++) {
          const layer = pad.layers[i];
          if (layer.active && layer.sampleId && zoneIdx < zones.length) {
            const zone = zones[zoneIdx];
            updateLayer(activePadIndex, i, {
              velStart: zone.velStart,
              velEnd: zone.velEnd,
              volume: zone.volume,
            });
            zoneIdx++;
          }
        }

        // Switch pad to velocity play mode
        setPadPlayMode(activePadIndex, 'velocity');
      });
      setIsOpen(false);
    },
    [pad, activePadIndex, layerCount, updateLayer, setPadPlayMode, withHistory],
  );

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
            ? 'Assign at least one sample to use velocity curves'
            : 'Apply a velocity curve preset to active layers'
        }
      >
        &#9889; Vel Curve
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
              Velocity Curve Presets
            </p>
            <span className="text-[9px] text-text-muted">
              {layerCount} layer{layerCount !== 1 ? 's' : ''}
            </span>
          </div>

          {/* 3x4 grid of curve presets */}
          <div className="grid grid-cols-3 gap-1.5">
            {VELOCITY_CURVES.map((curve) => (
              <button
                key={curve.id}
                onClick={() => handleSelectCurve(curve.id)}
                className="group flex flex-col items-center gap-1 p-1.5 rounded-lg
                  bg-surface-alt hover:bg-accent-teal/10 border border-transparent
                  hover:border-accent-teal/30 transition-all text-text-muted
                  hover:text-accent-teal"
                title={curve.description}
              >
                {/* Mini SVG curve */}
                <CurvePreview curve={curve} />
                {/* Label */}
                <span className="text-[8px] font-medium leading-tight text-center truncate w-full">
                  {curve.name}
                </span>
              </button>
            ))}
          </div>

          {/* Footer hint */}
          <p className="text-[8px] text-text-muted text-center px-2">
            Sets velocity ranges and volume for each active layer.
            Pad will switch to Velocity play mode.
          </p>
        </div>
      )}
    </div>
  );
}
