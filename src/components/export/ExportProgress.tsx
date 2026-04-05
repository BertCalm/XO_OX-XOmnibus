'use client';

import React, { useEffect, useRef } from 'react';
import ProgressBar, { StepProgress } from '@/components/ui/ProgressBar';
import { useExportStore } from '@/stores/exportStore';
import { playSuccessChime, playErrorThud } from '@/lib/audio/uiSounds';

export default function ExportProgress() {
  const isExporting = useExportStore((s) => s.isExporting);
  const steps = useExportStore((s) => s.steps);
  const overallProgress = useExportStore((s) => s.overallProgress);
  const startedAt = useExportStore((s) => s.startedAt);
  const completedAt = useExportStore((s) => s.completedAt);
  const error = useExportStore((s) => s.error);
  const hasChimed = useRef(false);

  // Play success chime when export completes, or error thud on failure
  useEffect(() => {
    if (completedAt && !error && !hasChimed.current) {
      hasChimed.current = true;
      playSuccessChime();
    } else if (error && !hasChimed.current) {
      hasChimed.current = true;
      playErrorThud();
    }
    // Reset when a new export starts
    if (isExporting) {
      hasChimed.current = false;
    }
  }, [completedAt, error, isExporting]);

  if (!isExporting && steps.length === 0) return null;

  const elapsed = startedAt
    ? ((completedAt || Date.now()) - startedAt) / 1000
    : 0;

  return (
    <div className="space-y-4">
      {/* Overall progress */}
      <div className="space-y-2">
        <ProgressBar
          progress={overallProgress}
          label={completedAt ? 'Forged ✓' : isExporting ? 'Forging...' : 'Ready'}
          detail={`${Math.round(overallProgress)}% \u2022 ${elapsed.toFixed(1)}s`}
          size="lg"
          gradient
        />
      </div>

      {/* Step details */}
      <div className="bg-surface-alt rounded-xl p-4">
        <StepProgress
          steps={steps.map((s) => ({
            label: s.label,
            status: s.status,
            detail: s.detail || (s.status === 'active' ? `${Math.round(s.progress)}%` : undefined),
          }))}
        />
      </div>

      {/* Error display */}
      {error && (
        <div className="bg-red-50 border border-red-200 rounded-lg p-3">
          <p className="text-sm text-red-600 font-medium">Export failed</p>
          <p className="text-xs text-red-500 mt-1">{error}</p>
        </div>
      )}

      {/* Completion message */}
      {completedAt && !error && (
        <div className="bg-accent-teal-50 border border-accent-teal/20 rounded-lg p-3 text-center">
          <p className="text-sm font-medium text-accent-teal">
            Forged by XO_OX ✦
          </p>
          <p className="text-xs text-accent-teal/70 mt-1">
            Your program has been forged and downloaded
          </p>
        </div>
      )}
    </div>
  );
}
