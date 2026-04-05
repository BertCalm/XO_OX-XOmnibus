'use client';

import React, { useMemo } from 'react';
import { useAudioStore } from '@/stores/audioStore';

/** MPC RAM limits in bytes */
const RAM_LIMITS = [
  { label: 'MPC One (2 GB)', bytes: 2 * 1024 * 1024 * 1024 },
  { label: 'MPC Live II (4 GB)', bytes: 4 * 1024 * 1024 * 1024 },
];

function formatBytes(bytes: number): string {
  if (bytes < 1024) return `${bytes} B`;
  if (bytes < 1024 * 1024) return `${(bytes / 1024).toFixed(1)} KB`;
  if (bytes < 1024 * 1024 * 1024) return `${(bytes / (1024 * 1024)).toFixed(1)} MB`;
  return `${(bytes / (1024 * 1024 * 1024)).toFixed(2)} GB`;
}

export default function RamEstimator() {
  const samples = useAudioStore((s) => s.samples);

  // Calculate total unique sample size (WAV buffers)
  const totalBytes = useMemo(() => {
    const uniqueIds = new Set<string>();
    let bytes = 0;
    for (const sample of samples) {
      if (!uniqueIds.has(sample.id)) {
        uniqueIds.add(sample.id);
        bytes += sample.buffer.byteLength;
      }
    }
    return bytes;
  }, [samples]);

  if (samples.length === 0) return null;

  return (
    <div className="space-y-2 p-3 bg-surface-alt rounded-lg border border-border">
      <div className="flex items-center justify-between">
        <p className="text-[10px] text-text-muted uppercase tracking-wider font-semibold">
          RAM Footprint
        </p>
        <p className="text-xs font-mono font-semibold text-text-primary">
          {formatBytes(totalBytes)}
        </p>
      </div>

      <p className="text-[10px] text-text-secondary">
        {samples.length} sample{samples.length !== 1 ? 's' : ''}
      </p>

      {/* Safety meters */}
      <div className="space-y-1.5">
        {RAM_LIMITS.map((limit) => {
          const ratio = totalBytes / limit.bytes;
          const pct = Math.min(ratio * 100, 100);
          const isWarning = ratio > 0.75;
          const isDanger = ratio > 0.9;

          return (
            <div key={limit.label} className="space-y-0.5">
              <div className="flex justify-between text-[9px]">
                <span className="text-text-muted">{limit.label}</span>
                <span
                  className={`font-mono font-medium ${
                    isDanger
                      ? 'text-red-500'
                      : isWarning
                        ? 'text-yellow-500'
                        : 'text-text-secondary'
                  }`}
                >
                  {pct.toFixed(1)}%
                </span>
              </div>
              <div className="h-1.5 bg-surface rounded-full overflow-hidden">
                <div
                  className={`h-full rounded-full transition-all duration-300 ${
                    isDanger
                      ? 'bg-red-500'
                      : isWarning
                        ? 'bg-yellow-500'
                        : 'bg-accent-teal'
                  }`}
                  style={{ width: `${pct}%` }}
                />
              </div>
            </div>
          );
        })}
      </div>
    </div>
  );
}
