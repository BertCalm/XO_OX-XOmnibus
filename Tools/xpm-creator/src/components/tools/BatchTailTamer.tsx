'use client';

import React, { useState, useCallback, useRef, useEffect } from 'react';
import { useAudioStore } from '@/stores/audioStore';
import { invalidateCache } from '@/lib/audio/audioBufferCache';
import { tameTailFromBuffer, TAIL_TAME_DEFAULTS } from '@/lib/audio/tailTaming';
import type { TailTameConfig } from '@/lib/audio/tailTaming';
import Slider from '@/components/ui/Slider';
import Button from '@/components/ui/Button';

type CurveType = TailTameConfig['curveType'];

const CURVE_OPTIONS: { id: CurveType; label: string; description: string }[] = [
  { id: 'logarithmic', label: 'Log', description: 'Smooth natural decay' },
  { id: 'linear', label: 'Lin', description: 'Constant-rate fade' },
  { id: 'exponential', label: 'Exp', description: 'Fast initial drop' },
  { id: 'cosine', label: 'Cos', description: 'S-curve fade' },
];

export default function BatchTailTamer() {
  const samples = useAudioStore((s) => s.samples);
  const updateSample = useAudioStore((s) => s.updateSample);

  const [isOpen, setIsOpen] = useState(false);
  const [isProcessing, setIsProcessing] = useState(false);
  const [progress, setProgress] = useState(0);
  const [totalSamples, setTotalSamples] = useState(0);
  const [successCount, setSuccessCount] = useState<number | null>(null);
  const [failureCount, setFailureCount] = useState(0);

  // Config state
  const [fadeDurationMs, setFadeDurationMs] = useState(TAIL_TAME_DEFAULTS.fadeDurationMs);
  const [curveType, setCurveType] = useState<CurveType>(TAIL_TAME_DEFAULTS.curveType);
  const [antiClickEnabled, setAntiClickEnabled] = useState(true);
  const [antiClickFadeInMs, setAntiClickFadeInMs] = useState(TAIL_TAME_DEFAULTS.antiClickFadeInMs);

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
    }
  }, [isOpen]);

  const handleProcess = useCallback(async () => {
    if (samples.length === 0 || isProcessing) return;

    setIsProcessing(true);
    setProgress(0);
    setTotalSamples(samples.length);
    setSuccessCount(null);
    setFailureCount(0);

    const config: Partial<TailTameConfig> = {
      fadeDurationMs,
      curveType,
      antiClickFadeInMs: antiClickEnabled ? antiClickFadeInMs : 0,
    };

    let processed = 0;
    let failed = 0;

    for (const sample of samples) {
      try {
        const result = await tameTailFromBuffer(
          sample.buffer,
          sample.bitDepth || 16,
          config,
        );

        updateSample(sample.id, {
          buffer: result.buffer,
          waveformPeaks: result.peaks,
        });
        invalidateCache(sample.id);

        processed++;
      } catch (error) {
        console.error(`Tail taming failed for "${sample.name}":`, error);
        failed++;
      }

      setProgress((prev) => prev + 1);
    }

    setSuccessCount(processed);
    setFailureCount(failed);
    setIsProcessing(false);
  }, [
    samples,
    isProcessing,
    fadeDurationMs,
    curveType,
    antiClickEnabled,
    antiClickFadeInMs,
    updateSample,
  ]);

  const sampleCount = samples.length;
  const progressPercent = totalSamples > 0 ? (progress / totalSamples) * 100 : 0;

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
            ? 'Import samples first to use tail taming'
            : 'Apply fade-out to all sample tails to prevent clicks and pops'
        }
      >
        &#128263; Tail Tame
      </button>

      {/* Dropdown popover */}
      {isOpen && (
        <div
          className="absolute z-50 mt-1 left-0 w-[280px]
            bg-surface border border-border rounded-lg shadow-card
            p-3 space-y-3"
        >
          {/* Header */}
          <div className="flex items-center justify-between">
            <p className="text-[10px] text-text-muted uppercase tracking-wider font-semibold">
              Batch Tail Taming
            </p>
            <button
              onClick={() => setIsOpen(false)}
              className="text-text-muted hover:text-text-primary text-xs"
            >
              &#10005;
            </button>
          </div>

          {/* Fade duration slider */}
          <Slider
            label="Fade Duration"
            value={fadeDurationMs}
            onChange={setFadeDurationMs}
            min={5}
            max={100}
            step={1}
            unit="ms"
          />

          {/* Curve type selector */}
          <div className="space-y-1.5">
            <p className="text-[10px] text-text-muted uppercase tracking-wider">Curve Type</p>
            <div className="grid grid-cols-4 gap-1">
              {CURVE_OPTIONS.map((opt) => (
                <button
                  key={opt.id}
                  onClick={() => setCurveType(opt.id)}
                  className={`px-1.5 py-1.5 rounded-lg text-center transition-all border
                    ${curveType === opt.id
                      ? 'bg-accent-teal/10 border-accent-teal/30 text-accent-teal'
                      : 'bg-surface-alt border-transparent text-text-muted hover:text-text-secondary'
                    }`}
                  title={opt.description}
                >
                  <span className="block text-[9px] font-medium">{opt.label}</span>
                </button>
              ))}
            </div>
            <p className="text-[8px] text-text-muted">
              {CURVE_OPTIONS.find((o) => o.id === curveType)?.description}
            </p>
          </div>

          {/* Anti-click fade-in toggle */}
          <div className="space-y-1.5">
            <div className="flex items-center gap-2">
              <button
                onClick={() => setAntiClickEnabled((prev) => !prev)}
                className={`w-8 h-4 rounded-full relative transition-colors ${
                  antiClickEnabled ? 'bg-accent-teal' : 'bg-surface-alt'
                }`}
              >
                <span
                  className={`absolute top-0.5 w-3 h-3 rounded-full bg-white shadow-sm transition-transform ${
                    antiClickEnabled ? 'left-4' : 'left-0.5'
                  }`}
                />
              </button>
              <span className="text-[10px] text-text-secondary font-medium">
                Anti-click fade-in
              </span>
            </div>
            {antiClickEnabled && (
              <Slider
                label="Fade-in Duration"
                value={antiClickFadeInMs}
                onChange={setAntiClickFadeInMs}
                min={1}
                max={5}
                step={0.5}
                unit="ms"
              />
            )}
          </div>

          {/* Sample count */}
          <div className="px-2 py-1.5 bg-surface-alt rounded text-center">
            <p className="text-[10px] text-text-secondary">
              <span className="font-bold text-accent-teal">{sampleCount}</span>{' '}
              sample{sampleCount !== 1 ? 's' : ''} will be processed
            </p>
          </div>

          {/* Progress bar (visible during processing) */}
          {isProcessing && (
            <div className="space-y-1">
              <div className="h-1.5 bg-surface-alt rounded-full overflow-hidden">
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

          {/* Success/failure message */}
          {successCount !== null && !isProcessing && (
            <div className={`px-2 py-1.5 rounded text-center ${
              failureCount > 0
                ? 'bg-yellow-500/10 border border-yellow-500/20'
                : 'bg-accent-teal/10 border border-accent-teal/20'
            }`}>
              <p className={`text-[10px] font-medium ${
                failureCount > 0 ? 'text-yellow-400' : 'text-accent-teal'
              }`}>
                Tamed {successCount} sample{successCount !== 1 ? 's' : ''} successfully
                {failureCount > 0 && ` · ${failureCount} failed`}
              </p>
            </div>
          )}

          {/* Action button */}
          <Button
            variant="primary"
            size="sm"
            disabled={sampleCount === 0 || isProcessing}
            loading={isProcessing}
            onClick={handleProcess}
            className="w-full"
          >
            {isProcessing
              ? `Processing... (${progress}/${totalSamples})`
              : 'Tame All Tails'}
          </Button>

          {/* Footer hint */}
          <p className="text-[8px] text-text-muted text-center px-2">
            Applies a fade-out to every sample to prevent clicks and pops on MPC hardware.
          </p>
        </div>
      )}
    </div>
  );
}
