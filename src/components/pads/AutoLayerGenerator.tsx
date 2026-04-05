'use client';

import React, { useState, useCallback } from 'react';
import { usePadStore } from '@/stores/padStore';
import { useAudioStore } from '@/stores/audioStore';
import {
  generateVelocityLayers,
  EXPRESSION_MODES,
  type AutoLayerConfig,
  type AutoLayerMode,
} from '@/lib/audio/autoLayerGenerator';
import { getDecodedBuffer } from '@/lib/audio/audioBufferCache';
import Slider from '@/components/ui/Slider';
import Button from '@/components/ui/Button';
import { useToastStore } from '@/stores/toastStore';
import { useHistoryStore } from '@/stores/historyStore';

type DualMode = AutoLayerConfig['dualMode'];

const DUAL_MODES: { id: DualMode; label: string; description: string }[] = [
  { id: 'split', label: 'Split', description: 'A→soft, B→hard' },
  { id: 'interleave', label: 'Weave', description: 'A,B,A,B alternating' },
  { id: 'blend', label: 'Blend', description: 'Weighted random mix' },
];

export default function AutoLayerGenerator() {
  const pads = usePadStore((s) => s.pads);
  const activePadIndex = usePadStore((s) => s.activePadIndex);
  const assignSampleToLayer = usePadStore((s) => s.assignSampleToLayer);
  const setPadPlayMode = usePadStore((s) => s.setPadPlayMode);
  const samples = useAudioStore((s) => s.samples);
  const addSample = useAudioStore((s) => s.addSample);

  const [isOpen, setIsOpen] = useState(false);
  const [isGenerating, setIsGenerating] = useState(false);
  const [progress, setProgress] = useState(0);
  const [mode, setMode] = useState<AutoLayerMode>('velocity');
  const [variation, setVariation] = useState(0.75);
  const [humanize, setHumanize] = useState(0.4);
  const [dualMode, setDualMode] = useState<DualMode>('split');

  const pad = pads[activePadIndex];
  if (!pad) return null;

  // Find source samples from existing layers
  const activeLayers = pad.layers.filter((l) => l.active && l.sampleId);
  const sourceSamples = activeLayers
    .slice(0, 2)
    .map((l) => samples.find((s) => s.id === l.sampleId))
    .filter(Boolean);

  const canGenerate = sourceSamples.length >= 1 && !isGenerating;
  const activeMode = EXPRESSION_MODES.find((m) => m.id === mode) || EXPRESSION_MODES[0];

  const handleGenerate = useCallback(async () => {
    if (!canGenerate || sourceSamples.length === 0) return;

    setIsGenerating(true);
    setProgress(0);

    try {
      const decodedBuffers: AudioBuffer[] = [];
      for (const s of sourceSamples) {
        if (!s) continue;
        const decoded = await getDecodedBuffer(s.id, s.buffer);
        decodedBuffers.push(decoded);
      }

      if (decodedBuffers.length === 0) return;

      const baseName = sourceSamples[0]!.name.replace(/_ghost$/, '').substring(0, 16);
      const rootNote = sourceSamples[0]!.rootNote;

      const layers = await generateVelocityLayers(
        decodedBuffers,
        baseName,
        rootNote,
        { mode, variation, humanize, dualMode },
        (n) => setProgress(n)
      );

      // Add all generated samples to the audio store
      for (const layer of layers) {
        addSample(layer.sample);
      }

      // Assign each layer to the pad with velocity ranges
      // Use captured pad index and fresh store reference for stale closure safety
      const capturedPadIndex = activePadIndex;
      const store = usePadStore.getState();
      const history = useHistoryStore.getState();
      history.snapshot(usePadStore.getState().pads);

      for (const layer of layers) {
        store.assignSampleToLayer(
          capturedPadIndex,
          layer.layerIndex,
          layer.sample.id,
          layer.sample.name,
          layer.sample.fileName,
          layer.sample.rootNote
        );

        store.updateLayer(capturedPadIndex, layer.layerIndex, {
          velStart: layer.velStart,
          velEnd: layer.velEnd,
        });
      }

      // Set the appropriate play mode for the expression mode
      store.setPadPlayMode(capturedPadIndex, activeMode.playMode);

      history.pushState('Auto-generate layers', usePadStore.getState().pads);

      setIsOpen(false);
      useToastStore.getState().addToast({
        type: 'success',
        title: `${activeMode.name} layers generated`,
      });
    } catch (error) {
      console.error('Auto-layer generation failed:', error);
      useToastStore.getState().addToast({
        type: 'error',
        title: 'Layer generation failed',
        message: String(error),
      });
    } finally {
      setIsGenerating(false);
      setProgress(0);
    }
  }, [
    canGenerate,
    sourceSamples,
    mode,
    variation,
    humanize,
    dualMode,
    activePadIndex,
    activeMode.playMode,
    addSample,
    assignSampleToLayer,
    setPadPlayMode,
  ]);

  if (!isOpen) {
    return (
      <button
        onClick={() => setIsOpen(true)}
        disabled={sourceSamples.length === 0}
        className="px-2 py-1 rounded text-[10px] font-medium
          bg-surface-alt text-text-secondary hover:text-text-primary
          hover:bg-surface transition-all disabled:opacity-50
          border border-transparent hover:border-border"
        title="Generate 8 expressive layers from current sample(s)"
      >
        🎭 Expression Map
      </button>
    );
  }

  return (
    <div className="space-y-3 p-3 bg-surface-alt rounded-lg border border-border animate-in fade-in slide-in-from-top-1 duration-150">
      <div className="flex items-center justify-between">
        <p className="text-[10px] text-text-muted uppercase tracking-wider font-semibold">
          Auto Layer Generator
        </p>
        <button
          onClick={() => setIsOpen(false)}
          className="text-text-muted hover:text-text-primary text-xs"
          aria-label="Close Auto Layer Generator"
        >
          ✕
        </button>
      </div>

      {/* Source samples */}
      <div className="space-y-1">
        <p className="text-[10px] text-text-muted">
          Source{sourceSamples.length > 1 ? 's' : ''} ({sourceSamples.length}/2)
        </p>
        {sourceSamples.map((s, i) => (
          <div
            key={s?.id || i}
            className="flex items-center gap-1.5 px-2 py-1 rounded bg-surface text-[10px]"
          >
            <span className="text-accent-teal font-bold">{String.fromCharCode(65 + i)}</span>
            <span className="text-text-primary truncate">{s?.name}</span>
          </div>
        ))}
        {sourceSamples.length === 1 && (
          <p className="text-[9px] text-text-muted">
            Add a 2nd sample to Layer 2 for dual-source mode
          </p>
        )}
      </div>

      {/* Expression Mode selector */}
      <div className="space-y-1.5">
        <p className="text-[10px] text-text-muted uppercase tracking-wider">Expression Mode</p>
        <div className="grid grid-cols-5 gap-1">
          {EXPRESSION_MODES.map((m) => (
            <button
              key={m.id}
              onClick={() => setMode(m.id)}
              className={`px-1 py-1.5 rounded-lg text-center transition-all border
                ${mode === m.id
                  ? 'bg-accent-teal/10 border-accent-teal/30 text-accent-teal'
                  : 'bg-surface border-transparent text-text-muted hover:text-text-secondary'
                }`}
            >
              <span className="block text-xs">{m.icon}</span>
              <span className="block text-[7px] font-medium mt-0.5 leading-tight">{m.name}</span>
            </button>
          ))}
        </div>
        <p className="text-[9px] text-text-muted">{activeMode.description}</p>
      </div>

      {/* Variation + Humanize sliders */}
      <Slider label="Variation" value={variation} onChange={setVariation} min={0} max={1} step={0.05} />
      <Slider label="Humanize" value={humanize} onChange={setHumanize} min={0} max={1} step={0.05} />

      {/* Dual mode (only show if 2 sources) */}
      {sourceSamples.length >= 2 && (
        <div className="space-y-1.5">
          <p className="text-[10px] text-text-muted uppercase tracking-wider">Dual Mode</p>
          <div className="flex gap-1">
            {DUAL_MODES.map((dm) => (
              <button
                key={dm.id}
                onClick={() => setDualMode(dm.id)}
                className={`flex-1 px-2 py-1.5 rounded text-[10px] font-medium transition-all border
                  ${dualMode === dm.id
                    ? 'bg-accent-teal/10 border-accent-teal/30 text-accent-teal'
                    : 'bg-surface border-transparent text-text-muted hover:text-text-secondary'
                  }`}
              >
                <span className="block">{dm.label}</span>
                <span className="block text-[8px] opacity-60">{dm.description}</span>
              </button>
            ))}
          </div>
        </div>
      )}

      {/* Layer preview visualization */}
      <div className="space-y-1">
        <p className="text-[10px] text-text-muted uppercase tracking-wider">
          8-Layer Preview — {activeMode.playMode === 'cycle' ? 'Cycle Mode' : 'Velocity Mode'}
        </p>
        <div className="flex gap-0.5 h-8">
          {activeMode.labels.map((label, i) => {
            const opacity = mode === 'round-robin'
              ? 0.6 + Math.sin(i * 0.8) * 0.15 // Subtle wave for round-robin
              : 0.15 + (i / 7) * 0.85;
            const isSrcB =
              sourceSamples.length >= 2 &&
              (dualMode === 'split'
                ? i >= 4
                : dualMode === 'interleave'
                  ? i % 2 === 1
                  : i >= 3);
            return (
              <div
                key={i}
                className="flex-1 rounded-sm flex items-end justify-center pb-0.5"
                style={{
                  backgroundColor: isSrcB
                    ? `rgb(var(--color-accent-plum) / ${opacity})`
                    : `rgb(var(--color-accent-teal) / ${opacity})`,
                }}
              >
                <span className="text-[8px] font-mono text-text-muted">{label}</span>
              </div>
            );
          })}
        </div>
        <div className="flex justify-between text-[8px] text-text-muted px-0.5">
          {mode === 'round-robin' ? (
            <>
              <span>variation 1</span>
              <span>variation 8</span>
            </>
          ) : mode === 'spectral' ? (
            <>
              <span>20 Hz</span>
              <span>20 kHz</span>
            </>
          ) : (
            <>
              <span>vel 0</span>
              <span>vel 127</span>
            </>
          )}
        </div>
      </div>

      {/* Generate button */}
      <div className="flex gap-1 pt-1">
        <Button variant="primary" size="sm" disabled={!canGenerate} loading={isGenerating} onClick={handleGenerate}>
          {isGenerating ? `Generating... (${progress}/8)` : 'Generate 8 Layers'}
        </Button>
        <Button variant="ghost" size="sm" onClick={() => setIsOpen(false)} disabled={isGenerating}>
          Cancel
        </Button>
      </div>

      {isGenerating && (
        <div className="h-1 bg-surface rounded-full overflow-hidden">
          <div
            className="h-full bg-accent-teal transition-all duration-300"
            style={{ width: `${(progress / 8) * 100}%` }}
          />
        </div>
      )}
    </div>
  );
}
