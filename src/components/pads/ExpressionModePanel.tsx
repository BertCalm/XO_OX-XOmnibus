'use client';

import React from 'react';
import { useExpressionStore } from '@/stores/expressionStore';
import { EXPRESSION_MODE_INFO } from '@/lib/audio/expressionEngine';
import type { ExpressionMode } from '@/lib/audio/expressionEngine';
import Slider from '@/components/ui/Slider';

// ---------------------------------------------------------------------------
// Mode order
// ---------------------------------------------------------------------------

const EXPRESSION_MODES: ExpressionMode[] = [
  'none',
  'valve-bloom',
  'transient-snap',
  'space-fold',
  'bit-crush',
  'ring-mod',
  'tape-saturation',
  'sub-harmonics',
  'lo-fi',
  'stereo-spread',
];

// ---------------------------------------------------------------------------
// Props
// ---------------------------------------------------------------------------

interface ExpressionModePanelProps {
  padIndex: number;
}

// ---------------------------------------------------------------------------
// Component
// ---------------------------------------------------------------------------

const DEFAULT_EXPRESSION = { mode: 'none' as ExpressionMode, intensity: 0.5, velocityThreshold: 0 };

export default function ExpressionModePanel({ padIndex }: ExpressionModePanelProps) {
  // Subscribe to actual data — NOT the getter function (which is a stable ref
  // and would never trigger a re-render).
  const expression = useExpressionStore(
    (s) => s.padExpressions[padIndex] ?? DEFAULT_EXPRESSION,
  );
  const setMode = useExpressionStore((s) => s.setMode);
  const setIntensity = useExpressionStore((s) => s.setIntensity);
  const setVelocityThreshold = useExpressionStore((s) => s.setVelocityThreshold);
  const isActive = expression.mode !== 'none';

  const handleModeClick = (mode: ExpressionMode) => {
    // Toggle off if the same mode is clicked again
    if (expression.mode === mode) {
      setMode(padIndex, 'none');
    } else {
      setMode(padIndex, mode);
    }
  };

  return (
    <div className="space-y-3 p-3 bg-surface-alt rounded-lg border border-border">
      {/* Header */}
      <div className="flex items-center justify-between">
        <p className="text-[10px] text-text-muted uppercase tracking-wider font-semibold">
          Expression Mode
        </p>
        <span className="text-[9px] font-mono text-text-muted">
          Pad {padIndex + 1}
        </span>
      </div>

      {/* Mode Grid */}
      <div className="grid grid-cols-3 gap-1">
        {EXPRESSION_MODES.map((mode) => {
          const info = EXPRESSION_MODE_INFO[mode];
          const selected = expression.mode === mode;

          return (
            <button
              key={mode}
              onClick={() => handleModeClick(mode)}
              title={info.description}
              className={`flex flex-col items-center gap-0.5 py-1.5 px-2 rounded border
                text-[10px] font-medium transition-all duration-100
                ${
                  selected
                    ? 'bg-accent-teal/10 border-accent-teal text-accent-teal'
                    : 'bg-surface-bg border-border text-text-secondary hover:bg-surface-alt'
                }`}
            >
              <span className="text-sm leading-none">{info.icon}</span>
              <span className="leading-tight truncate w-full text-center">
                {info.label}
              </span>
            </button>
          );
        })}
      </div>

      {/* Parameter Sliders — only shown when a mode is active */}
      {isActive && (
        <div className="space-y-2 pt-1">
          <Slider
            label="Intensity"
            value={Math.round(expression.intensity * 100)}
            min={0}
            max={100}
            step={1}
            unit="%"
            onChange={(value) => setIntensity(padIndex, value / 100)}
          />

          <Slider
            label="Velocity Threshold"
            value={expression.velocityThreshold}
            min={0}
            max={127}
            step={1}
            onChange={(value) => setVelocityThreshold(padIndex, value)}
          />
        </div>
      )}
    </div>
  );
}
