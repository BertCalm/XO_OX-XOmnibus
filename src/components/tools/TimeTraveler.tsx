'use client';

import React, { useState, useCallback, useRef, useEffect } from 'react';
import { useAudioStore } from '@/stores/audioStore';
import { usePadStore } from '@/stores/padStore';
import { processEraFromBuffer, ERA_PRESETS } from '@/lib/audio/timeTraveler';
import type { EraMode, EraConfig } from '@/lib/audio/timeTraveler';
import Button from '@/components/ui/Button';
import Slider from '@/components/ui/Slider';
import { useToastStore } from '@/stores/toastStore';
import { invalidateCache } from '@/lib/audio/audioBufferCache';

export default function TimeTraveler() {
  const samples = useAudioStore((s) => s.samples);
  const updateSample = useAudioStore((s) => s.updateSample);
  const pads = usePadStore((s) => s.pads);
  const activePadIndex = usePadStore((s) => s.activePadIndex);
  const activeLayerIndex = usePadStore((s) => s.activeLayerIndex);

  const [isOpen, setIsOpen] = useState(false);
  const [selectedEra, setSelectedEra] = useState<EraMode>('1993-grit');
  const [intensity, setIntensity] = useState(75);
  const [isProcessing, setIsProcessing] = useState(false);
  const [progress, setProgress] = useState(0);
  const [totalSamples, setTotalSamples] = useState(0);
  const [successCount, setSuccessCount] = useState<number | null>(null);
  const [failedCount, setFailedCount] = useState(0);
  const [processMode, setProcessMode] = useState<'current' | 'all' | null>(null);

  const dropdownRef = useRef<HTMLDivElement>(null);

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

  // Reset success message when dropdown reopens
  useEffect(() => {
    if (isOpen) {
      setSuccessCount(null);
      setProcessMode(null);
    }
  }, [isOpen]);

  // Get the active layer's sample
  const pad = pads[activePadIndex];
  const layer = pad?.layers[activeLayerIndex];
  const activeSample = layer?.sampleId
    ? samples.find((s) => s.id === layer.sampleId)
    : null;

  const handleProcess = useCallback(
    async (mode: 'current' | 'all') => {
      if (isProcessing) return;

      const config: EraConfig = {
        mode: selectedEra,
        intensity: intensity / 100,
      };

      const samplesToProcess =
        mode === 'current' && activeSample ? [activeSample] : samples;

      if (samplesToProcess.length === 0) return;

      setIsProcessing(true);
      setProcessMode(mode);
      setProgress(0);
      setTotalSamples(samplesToProcess.length);
      setSuccessCount(null);
      setFailedCount(0);

      let processed = 0;
      let failed = 0;

      try {
        for (const sample of samplesToProcess) {
          try {
            const result = await processEraFromBuffer(
              sample.buffer,
              sample.bitDepth || 16,
              config
            );

            updateSample(sample.id, {
              buffer: result.buffer,
              waveformPeaks: result.peaks,
            });
            // Invalidate decode cache since the buffer content changed
            invalidateCache(sample.id);

            processed++;
          } catch (error) {
            console.error(`Era processing failed for "${sample.name}":`, error);
            failed++;
          }

          setProgress((prev) => prev + 1);
        }

        // Show summary toast
        const { addToast } = useToastStore.getState();
        const eraLabel = ERA_PRESETS.find((p) => p.id === selectedEra)?.year ?? selectedEra;
        if (failed === 0 && processed > 0) {
          addToast({ type: 'success', title: `${eraLabel} applied to ${processed} sample${processed !== 1 ? 's' : ''}` });
        } else if (processed > 0) {
          addToast({ type: 'warning', title: `${eraLabel}: ${processed} processed, ${failed} failed` });
        } else {
          addToast({ type: 'error', title: 'Era processing failed for all samples' });
        }
      } finally {
        setSuccessCount(processed);
        setFailedCount(failed);
        setIsProcessing(false);
      }
    },
    [samples, activeSample, isProcessing, selectedEra, intensity, updateSample]
  );

  const sampleCount = samples.length;
  const progressPercent =
    totalSamples > 0 ? (progress / totalSamples) * 100 : 0;

  const selectedPreset = ERA_PRESETS.find((p) => p.id === selectedEra);

  return (
    <div className="relative inline-block" ref={dropdownRef}>
      {/* Trigger button */}
      <button
        onClick={() => setIsOpen((prev) => !prev)}
        disabled={sampleCount === 0}
        className="px-2 py-1 rounded text-[10px] font-medium
          bg-surface-alt text-text-secondary hover:text-text-primary
          hover:bg-surface transition-all disabled:opacity-50
          border border-transparent hover:border-border"
        title={
          sampleCount === 0
            ? 'Import samples first to use era processing'
            : 'Apply vintage era-specific character processing to samples'
        }
      >
        &#9203; Time Traveler
      </button>

      {/* Dropdown popover */}
      {isOpen && (
        <div
          className="absolute z-50 mt-1 right-0 w-[380px]
            bg-surface border border-border rounded-lg shadow-card
            p-3 space-y-3 animate-in fade-in slide-in-from-top-2 duration-150"
          onKeyDown={(e) => { if (e.key === 'Escape') { e.stopPropagation(); setIsOpen(false); } }}
        >
          {/* Header */}
          <div className="flex items-center justify-between">
            <p className="text-[10px] text-text-muted uppercase tracking-wider font-semibold">
              The Time Traveler &mdash; Era Processing
            </p>
            <button
              onClick={() => setIsOpen(false)}
              className="text-text-muted hover:text-text-primary text-xs"
            >
              &#10005;
            </button>
          </div>

          {/* Era preset cards */}
          <div className="grid grid-cols-3 gap-2">
            {ERA_PRESETS.map((preset) => (
              <button
                key={preset.id}
                onClick={() => setSelectedEra(preset.id)}
                className={`p-2 rounded-lg text-left transition-all border
                  ${
                    selectedEra === preset.id
                      ? 'bg-accent-teal/10 border-accent-teal/40 shadow-sm'
                      : 'bg-surface-alt border-transparent hover:border-border'
                  }`}
              >
                <div className="text-center mb-1">
                  <span className="text-lg leading-none">{preset.icon}</span>
                </div>
                <p
                  className={`text-[15px] font-bold text-center leading-none mb-0.5 ${
                    selectedEra === preset.id
                      ? 'text-accent-teal'
                      : 'text-text-primary'
                  }`}
                >
                  {preset.year}
                </p>
                <p
                  className={`text-[9px] font-medium text-center leading-tight mb-1 ${
                    selectedEra === preset.id
                      ? 'text-accent-teal'
                      : 'text-text-secondary'
                  }`}
                >
                  {preset.name}
                </p>
                <p className="text-[8px] text-text-muted text-center leading-tight">
                  {preset.description}
                </p>
              </button>
            ))}
          </div>

          {/* Selected era details */}
          {selectedPreset && (
            <div className="px-2 py-1.5 bg-surface-alt rounded space-y-1">
              <p className="text-[9px] text-text-muted uppercase tracking-wider font-semibold">
                Processing Chain
              </p>
              <div className="flex flex-wrap gap-1">
                {selectedPreset.details.map((detail, i) => (
                  <span
                    key={i}
                    className="px-1.5 py-0.5 rounded bg-surface text-[8px] text-text-secondary
                      border border-border"
                  >
                    {detail}
                  </span>
                ))}
              </div>
            </div>
          )}

          {/* Intensity slider */}
          <Slider
            label="Intensity"
            value={intensity}
            onChange={setIntensity}
            min={0}
            max={100}
            step={1}
            unit="%"
          />

          {/* Sample info */}
          <div className="px-2 py-1.5 bg-surface-alt rounded text-center">
            <p className="text-[10px] text-text-secondary">
              {activeSample ? (
                <>
                  Active:{' '}
                  <span className="font-bold text-accent-teal">
                    {activeSample.name}
                  </span>
                  {' | '}
                  <span className="font-bold text-text-primary">
                    {sampleCount}
                  </span>{' '}
                  total sample{sampleCount !== 1 ? 's' : ''}
                </>
              ) : (
                <>
                  <span className="font-bold text-text-primary">
                    {sampleCount}
                  </span>{' '}
                  sample{sampleCount !== 1 ? 's' : ''} available
                </>
              )}
            </p>
          </div>

          {/* Progress bar (visible during processing) */}
          {isProcessing && (
            <div className="space-y-1">
              <div
                className="h-1.5 bg-surface-alt rounded-full overflow-hidden"
                role="progressbar"
                aria-valuenow={Math.round(progressPercent)}
                aria-valuemin={0}
                aria-valuemax={100}
                aria-label={`Era processing: ${progress} of ${totalSamples} samples`}
              >
                <div
                  className="h-full bg-accent-teal transition-all duration-200"
                  style={{ width: `${progressPercent}%` }}
                />
              </div>
              <p className="text-[9px] text-text-muted text-center">
                Processing {progress} / {totalSamples}...
              </p>
            </div>
          )}

          {/* Success message */}
          {successCount !== null && !isProcessing && (
            <div className="px-2 py-1.5 bg-accent-teal/10 border border-accent-teal/20 rounded text-center">
              <p className="text-[10px] text-accent-teal font-medium">
                {selectedPreset?.year} era applied to {successCount} sample
                {successCount !== 1 ? 's' : ''}
                {failedCount > 0 && (
                  <span className="text-amber-500"> ({failedCount} failed)</span>
                )}
              </p>
            </div>
          )}

          {/* Action buttons */}
          <div className="flex gap-2">
            <Button
              variant="primary"
              size="sm"
              disabled={!activeSample || isProcessing}
              loading={isProcessing && processMode === 'current'}
              onClick={() => handleProcess('current')}
              className="flex-1"
            >
              {isProcessing && processMode === 'current'
                ? 'Processing...'
                : 'Process Current'}
            </Button>
            <Button
              variant="secondary"
              size="sm"
              disabled={sampleCount === 0 || isProcessing}
              loading={isProcessing && processMode === 'all'}
              onClick={() => handleProcess('all')}
              className="flex-1"
            >
              {isProcessing && processMode === 'all'
                ? `${progress}/${totalSamples}`
                : `Process All (${sampleCount})`}
            </Button>
          </div>

          {/* Footer hint */}
          <p className="text-[8px] text-text-muted text-center px-2">
            Applies era-specific character processing. Each era simulates the
            sonic signature of its time period.
          </p>
        </div>
      )}
    </div>
  );
}
