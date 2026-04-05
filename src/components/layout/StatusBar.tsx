'use client';

import React, { useMemo } from 'react';
import { useAudioStore } from '@/stores/audioStore';
import { useProjectStore } from '@/stores/projectStore';
import { useExportStore } from '@/stores/exportStore';

/**
 * StatusBar — fixed bottom information bar showing project vitals.
 * Reads from audioStore (sample count, total duration),
 * projectStore (project name), and exportStore (export status).
 */
export default function StatusBar() {
  const samples = useAudioStore((s) => s.samples);
  const projectName = useProjectStore((s) => s.currentProject?.name);
  const isExporting = useExportStore((s) => s.isExporting);
  const overallProgress = useExportStore((s) => s.overallProgress);

  const totalDuration = useMemo(() => {
    const total = samples.reduce((sum, s) => sum + (s.duration || 0), 0);
    if (total === 0) return '0:00';
    const minutes = Math.floor(total / 60);
    const seconds = Math.floor(total % 60);
    return `${minutes}:${seconds.toString().padStart(2, '0')}`;
  }, [samples]);

  const sampleCount = samples.length;
  const favCount = useMemo(
    () => samples.filter((s) => s.isFavorite).length,
    [samples]
  );

  return (
    <div className="h-7 glass-panel border-t border-border flex items-center px-4 gap-4 text-[10px] text-text-muted select-none shrink-0">
      {/* Project name */}
      {projectName ? (
        <span className="font-medium text-text-secondary truncate max-w-[160px]">
          {projectName}
        </span>
      ) : (
        <span className="italic">No project</span>
      )}

      <span className="text-border">|</span>

      {/* Sample count + total duration */}
      <span>
        {sampleCount} sample{sampleCount !== 1 ? 's' : ''}
        {favCount > 0 && (
          <span className="ml-1 text-accent-teal">{favCount} ★</span>
        )}
      </span>

      <span className="text-border">|</span>

      <span>{totalDuration} total</span>

      {/* Export status — right-aligned */}
      <div className="ml-auto flex items-center gap-2">
        {isExporting ? (
          <span className="flex items-center gap-1.5 text-accent-teal font-medium">
            <span
              className="inline-block w-1.5 h-1.5 rounded-full bg-accent-teal animate-pulse"
            />
            Forging {Math.round(overallProgress)}%
          </span>
        ) : (
          <span className="text-text-muted">Ready</span>
        )}
      </div>
    </div>
  );
}
