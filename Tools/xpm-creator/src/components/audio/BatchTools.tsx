'use client';

import React, { useCallback, useState } from 'react';
import { useAudioStore } from '@/stores/audioStore';
import { generateWaveformPeaks } from '@/lib/audio/audioUtils';
import { getDecodedBuffer, invalidateCache } from '@/lib/audio/audioBufferCache';
import { encodeWav } from '@/lib/audio/wavEncoder';
import {
  applyLogFadeOut,
  applySafetyFadeOut,
  trimSilence,
} from '@/lib/audio/chopProcessors';
import { normalizeBuffer } from '@/lib/audio/audioSlicer';
import { useToastStore } from '@/stores/toastStore';
import ConfirmDialog from '@/components/ui/ConfirmDialog';

type BatchOp = 'tail-tame' | 'safety-clip' | 'normalize' | 'trim-silence';

const BATCH_OPS: { id: BatchOp; label: string; icon: string; description: string }[] = [
  {
    id: 'tail-tame',
    label: 'Tame Tails',
    icon: '↘',
    description: 'Log fade-out on all samples (200ms)',
  },
  {
    id: 'safety-clip',
    label: 'Safety Clip',
    icon: '✂',
    description: '1ms fade-out to prevent clicks',
  },
  {
    id: 'normalize',
    label: 'Normalize All',
    icon: '▮',
    description: 'Peak normalize all samples to -0.5dB',
  },
  {
    id: 'trim-silence',
    label: 'Trim Silence',
    icon: '⌦',
    description: 'Remove trailing silence from all',
  },
];

export default function BatchTools() {
  const samples = useAudioStore((s) => s.samples);
  const updateSample = useAudioStore((s) => s.updateSample);
  const setProcessing = useAudioStore((s) => s.setProcessing);
  const setIsBatchProcessing = useAudioStore((s) => s.setIsBatchProcessing);
  const [processing, setLocalProcessing] = useState(false);
  const [pendingOp, setPendingOp] = useState<BatchOp | null>(null);

  const runBatch = useCallback(
    async (op: BatchOp) => {
      if (samples.length === 0 || processing) return;

      setLocalProcessing(true);
      setIsBatchProcessing(true);
      setProcessing(true, `Batch processing: ${op}...`);

      const opLabel = BATCH_OPS.find((o) => o.id === op)?.label ?? op;
      let succeeded = 0;
      let failed = 0;

      // Snapshot samples at batch start to avoid stale closure during async iteration.
      // Each iteration reads the *current* sample by ID, but we iterate a stable list.
      const sampleSnapshot = useAudioStore.getState().samples;

      try {
        for (let i = 0; i < sampleSnapshot.length; i++) {
          const sample = sampleSnapshot[i];
          setProcessing(true, `Processing ${i + 1}/${sampleSnapshot.length}: ${sample.name}`);

          try {
            // Decode the current buffer (cached for performance)
            const audioBuffer = await getDecodedBuffer(sample.id, sample.buffer);

            // Apply the operation
            let processed: AudioBuffer;
            switch (op) {
              case 'tail-tame':
                processed = applyLogFadeOut(audioBuffer, 0.2);
                break;
              case 'safety-clip':
                processed = applySafetyFadeOut(audioBuffer);
                break;
              case 'normalize':
                processed = normalizeBuffer(audioBuffer, 0.944); // -0.5dB
                break;
              case 'trim-silence':
                processed = trimSilence(audioBuffer, -60);
                break;
              default:
                processed = audioBuffer;
            }

            // Re-encode and update
            const newBuffer = encodeWav(processed, sample.bitDepth || 16);
            const newPeaks = generateWaveformPeaks(processed);

            updateSample(sample.id, {
              buffer: newBuffer,
              waveformPeaks: newPeaks,
              duration: processed.duration,
            });
            // Invalidate decode cache since the buffer content changed
            invalidateCache(sample.id);
            succeeded++;
          } catch (error) {
            console.error(`Batch ${op} failed for "${sample.name}":`, error);
            failed++;
          }
        }

        // Show summary toast
        const { addToast } = useToastStore.getState();
        if (failed === 0) {
          addToast({ type: 'success', title: `${opLabel}: ${succeeded} samples refined` });
        } else if (succeeded > 0) {
          addToast({ type: 'warning', title: `${opLabel}: ${succeeded} processed, ${failed} failed` });
        } else {
          addToast({ type: 'error', title: `${opLabel} failed for all samples` });
        }
      } finally {
        setProcessing(false);
        setIsBatchProcessing(false);
        setLocalProcessing(false);
      }
    },
    [samples, processing, updateSample, setProcessing, setIsBatchProcessing]
  );

  if (samples.length === 0) return null;

  return (
    <div className="space-y-1.5">
      <h4 className="text-[10px] font-semibold text-text-muted uppercase tracking-wider px-1">
        Batch Tools
      </h4>
      <div className="grid grid-cols-2 gap-1">
        {BATCH_OPS.map((op) => (
          <button
            key={op.id}
            onClick={() => setPendingOp(op.id)}
            disabled={processing}
            title={op.description}
            className="flex items-center gap-1.5 px-2 py-1.5 rounded text-[10px] font-medium
              bg-surface-alt text-text-secondary hover:text-text-primary hover:bg-surface
              transition-all disabled:opacity-50 disabled:cursor-not-allowed
              border border-transparent hover:border-border"
          >
            <span className="text-xs">{op.icon}</span>
            {op.label}
          </button>
        ))}
      </div>

      {/* Confirmation dialog for destructive batch operations */}
      <ConfirmDialog
        open={!!pendingOp}
        onConfirm={() => {
          if (pendingOp) {
            runBatch(pendingOp);
            setPendingOp(null);
          }
        }}
        onCancel={() => setPendingOp(null)}
        title={`Run "${BATCH_OPS.find((o) => o.id === pendingOp)?.label ?? ''}"?`}
        message={`This will permanently modify all ${samples.length} loaded samples. This cannot be undone.`}
        confirmLabel="Process All"
        variant="warning"
      />
    </div>
  );
}
