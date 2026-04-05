'use client';

import React, { useState, useCallback, useRef, useEffect } from 'react';
import { usePadStore } from '@/stores/padStore';
import { useAudioStore } from '@/stores/audioStore';
import { generateAirLayer, AIR_PRESETS, AIR_DEFAULTS } from '@/lib/audio/spectralAir';
import type { AirConfig } from '@/lib/audio/spectralAir';
import Button from '@/components/ui/Button';
import Slider from '@/components/ui/Slider';
import { v4 as uuid } from 'uuid';
import type { AudioSample } from '@/types';
import { useToastStore } from '@/stores/toastStore';
import { useHistoryStore } from '@/stores/historyStore';

export default function SpectralAirPanel() {
  const [isOpen, setIsOpen] = useState(false);
  const [config, setConfig] = useState<AirConfig>({ ...AIR_DEFAULTS });
  const [selectedPresetId, setSelectedPresetId] = useState<string | null>(null);
  const [isProcessing, setIsProcessing] = useState(false);
  const [successMessage, setSuccessMessage] = useState<string | null>(null);
  const dropdownRef = useRef<HTMLDivElement>(null);

  const pads = usePadStore((s) => s.pads);
  const activePadIndex = usePadStore((s) => s.activePadIndex);
  const activeLayerIndex = usePadStore((s) => s.activeLayerIndex);

  const samples = useAudioStore((s) => s.samples);
  const addSample = useAudioStore((s) => s.addSample);

  const pad = pads[activePadIndex];
  const layer = pad?.layers[activeLayerIndex];
  const hasSample = layer?.active && layer?.sampleId;

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

  // Clear success message after a delay
  useEffect(() => {
    if (!successMessage) return;
    const timer = setTimeout(() => setSuccessMessage(null), 3000);
    return () => clearTimeout(timer);
  }, [successMessage]);

  const handlePresetSelect = useCallback((presetId: string) => {
    const preset = AIR_PRESETS.find((p) => p.id === presetId);
    if (!preset) return;
    setSelectedPresetId(presetId);
    setConfig((prev) => ({ ...prev, ...preset.config }));
  }, []);

  const handleInject = useCallback(async () => {
    if (!layer?.sampleId) return;
    const sample = samples.find((s) => s.id === layer.sampleId);
    if (!sample) return;

    // Capture pad/layer at invocation time (prevents stale closure if user switches pads)
    const capturedPadIndex = activePadIndex;
    const capturedLayerIndex = activeLayerIndex;

    setIsProcessing(true);
    setSuccessMessage(null);

    try {
      const { airWav, airBuffer, peaks } = await generateAirLayer(sample.buffer, config);

      // Create new AudioSample from the result
      const modeLabel = config.mode === 'noise-burst' ? 'burst' : config.mode === 'high-shelf' ? 'air' : 'excite';
      const airSample: AudioSample = {
        id: uuid(),
        name: `${sample.name}_${modeLabel}`,
        fileName: `${sample.name}_${modeLabel}.WAV`,
        duration: airBuffer.duration,
        sampleRate: airBuffer.sampleRate,
        channels: airBuffer.numberOfChannels,
        bitDepth: 16,
        buffer: airWav,
        waveformPeaks: peaks,
        rootNote: sample.rootNote,
        createdAt: Date.now(),
      };

      // Add to audio store
      addSample(airSample);

      // Re-read pad state fresh (in case layers changed during async processing)
      const freshPads = usePadStore.getState().pads;
      const freshPad = freshPads[capturedPadIndex];

      // Find next empty layer on the captured pad
      const nextEmpty = freshPad
        ? freshPad.layers.findIndex(
            (l, i) => i > capturedLayerIndex && (!l.active || !l.sampleId)
          )
        : -1;

      if (nextEmpty >= 0) {
        // Assign air sample to the next empty layer
        const store = usePadStore.getState();
        const history = useHistoryStore.getState();
        history.snapshot(usePadStore.getState().pads);

        store.assignSampleToLayer(
          capturedPadIndex,
          nextEmpty,
          airSample.id,
          airSample.name,
          airSample.fileName,
          airSample.rootNote
        );

        // Set the air layer's velStart to the configured velocity threshold
        store.updateLayer(capturedPadIndex, nextEmpty, {
          velStart: config.velocityThreshold,
          velEnd: 127,
        });

        // Set the original layer's velEnd to threshold - 1
        store.updateLayer(capturedPadIndex, capturedLayerIndex, {
          velStart: 0,
          velEnd: Math.max(0, config.velocityThreshold - 1),
        });

        // Set pad play mode to velocity
        store.setPadPlayMode(capturedPadIndex, 'velocity');

        history.pushState('Inject spectral air', usePadStore.getState().pads);

        setSuccessMessage(`Air layer injected on L${nextEmpty + 1} (vel ${config.velocityThreshold}-127)`);
      } else {
        setSuccessMessage('Air sample created but no empty layer available');
      }
    } catch (error) {
      console.error('Spectral Air injection failed:', error);
      useToastStore.getState().addToast({
        type: 'error',
        title: 'Spectral Air injection failed',
        message: error instanceof Error ? error.message : String(error),
      });
    } finally {
      setIsProcessing(false);
    }
  }, [
    layer,
    samples,
    config,
    activeLayerIndex,
    activePadIndex,
    addSample,
  ]);

  if (!pad) return null;

  return (
    <div className="relative inline-block" ref={dropdownRef}>
      {/* Trigger button */}
      <button
        onClick={() => setIsOpen((prev) => !prev)}
        disabled={!hasSample}
        className="px-2 py-1 rounded text-[10px] font-medium
          bg-surface-alt text-text-secondary hover:text-text-primary
          hover:bg-surface transition-all disabled:opacity-50
          border border-transparent hover:border-border"
        title={
          !hasSample
            ? 'Assign a sample first to use Spectral Air'
            : 'Add a tuned air layer for high-velocity hits'
        }
      >
        {'\u{1F32C}\uFE0F'} Air Inject
      </button>

      {/* Dropdown popover */}
      {isOpen && (
        <div
          className="absolute z-50 mt-1 left-0 w-[280px]
            bg-surface border border-border rounded-lg shadow-card
            p-2 space-y-2 animate-in fade-in slide-in-from-top-2 duration-150"
          onKeyDown={(e) => { if (e.key === 'Escape') { e.stopPropagation(); setIsOpen(false); } }}
        >
          {/* Header */}
          <div className="flex items-center justify-between px-1">
            <p className="text-[10px] text-text-muted uppercase tracking-wider font-semibold">
              Spectral Air Injector
            </p>
            <button
              onClick={() => setIsOpen(false)}
              className="text-text-muted hover:text-text-primary text-xs"
              aria-label="Close Spectral Air panel"
            >
              {'\u2715'}
            </button>
          </div>

          {/* Preset grid (2x2) */}
          <div className="grid grid-cols-2 gap-1.5">
            {AIR_PRESETS.map((preset) => (
              <button
                key={preset.id}
                onClick={() => handlePresetSelect(preset.id)}
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

          {/* Custom controls */}
          <div className="space-y-2 px-1">
            <p className="text-[10px] text-text-muted uppercase tracking-wider">
              Custom Settings
            </p>

            {/* Mode selector (3 buttons) */}
            <div className="flex gap-1">
              {([
                { id: 'high-shelf' as const, label: 'High-Shelf' },
                { id: 'noise-burst' as const, label: 'Noise Burst' },
                { id: 'exciter' as const, label: 'Exciter' },
              ]).map((mode) => (
                <button
                  key={mode.id}
                  onClick={() => {
                    setConfig((prev) => ({ ...prev, mode: mode.id }));
                    setSelectedPresetId(null);
                  }}
                  className={`flex-1 px-2 py-1 rounded text-[9px] font-medium transition-all
                    ${config.mode === mode.id
                      ? 'bg-accent-teal text-white'
                      : 'bg-surface-alt text-text-secondary hover:text-text-primary'
                    }`}
                >
                  {mode.label}
                </button>
              ))}
            </div>

            {/* Air Frequency slider */}
            <Slider
              label="Air Frequency"
              value={config.airFrequency}
              onChange={(v) => {
                setConfig((prev) => ({ ...prev, airFrequency: v }));
                setSelectedPresetId(null);
              }}
              min={2000}
              max={15000}
              step={100}
              unit=" Hz"
            />

            {/* Intensity slider */}
            <Slider
              label="Intensity"
              value={Math.round(config.intensity * 100)}
              onChange={(v) => {
                setConfig((prev) => ({ ...prev, intensity: v / 100 }));
                setSelectedPresetId(null);
              }}
              min={0}
              max={100}
              step={1}
              unit="%"
            />

            {/* Velocity Threshold slider */}
            <Slider
              label="Velocity Threshold"
              value={config.velocityThreshold}
              onChange={(v) => {
                setConfig((prev) => ({ ...prev, velocityThreshold: v }));
                setSelectedPresetId(null);
              }}
              min={60}
              max={127}
              step={1}
            />

            {/* Burst Duration slider (only visible in noise-burst mode) */}
            {config.mode === 'noise-burst' && (
              <Slider
                label="Burst Duration"
                value={config.burstDurationMs}
                onChange={(v) => {
                  setConfig((prev) => ({ ...prev, burstDurationMs: v }));
                  setSelectedPresetId(null);
                }}
                min={20}
                max={200}
                step={1}
                unit=" ms"
              />
            )}
          </div>

          {/* Inject button */}
          <div className="px-1 pt-1">
            <Button
              variant="primary"
              size="sm"
              disabled={!hasSample || isProcessing}
              loading={isProcessing}
              onClick={handleInject}
              className="w-full"
            >
              {isProcessing ? 'Processing...' : 'Inject Air Layer'}
            </Button>
          </div>

          {/* Success / status message */}
          {successMessage && (
            <p className={`text-[9px] text-center px-2 ${
              successMessage.includes('failed') || successMessage.includes('no empty')
                ? 'text-amber-400'
                : 'text-accent-teal'
            }`}>
              {successMessage}
            </p>
          )}

          {/* Footer hint */}
          <p className="text-[8px] text-text-muted text-center px-2">
            Adds a complementary air layer that triggers only on high-velocity hits.
          </p>
        </div>
      )}
    </div>
  );
}
