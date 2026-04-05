'use client';

import React, { useState, useCallback } from 'react';
import { usePadStore } from '@/stores/padStore';
import { useAudioStore } from '@/stores/audioStore';
import { useHistoryStore } from '@/stores/historyStore';
import { useToastStore } from '@/stores/toastStore';
import {
  generate8LayerCycle,
  VELOCITY_TIERS,
  CYCLE_ENGINE_DEFAULTS,
} from '@/lib/audio/cycleEngine';
import type { CycleEngineConfig } from '@/lib/audio/cycleEngine';
import Button from '@/components/ui/Button';
import Slider from '@/components/ui/Slider';

type VelocityProcessing = CycleEngineConfig['velocityProcessing'];

const PROCESSING_MODES: { id: VelocityProcessing; label: string; description: string }[] = [
  { id: 'filter-sweep', label: 'Filter Sweep', description: 'Lowpass filter scales with velocity' },
  { id: 'volume-only', label: 'Volume Only', description: 'Simple volume scaling per tier' },
  { id: 'full-dynamics', label: 'Full Dynamics', description: 'Filter + pitch + attack shaping' },
];

/** Tier color palette (cool to hot gradient) */
const TIER_COLORS = [
  'rgba(59, 130, 246, VAR)',   // Ghost — blue
  'rgba(16, 185, 129, VAR)',   // Soft — emerald
  'rgba(245, 158, 11, VAR)',   // Medium — amber
  'rgba(239, 68, 68, VAR)',    // Hard — red
];

export default function CycleEnginePanel() {
  const pads = usePadStore((s) => s.pads);
  const activePadIndex = usePadStore((s) => s.activePadIndex);
  const activeLayerIndex = usePadStore((s) => s.activeLayerIndex);

  const samples = useAudioStore((s) => s.samples);
  const addSample = useAudioStore((s) => s.addSample);

  const [isOpen, setIsOpen] = useState(false);
  const [isBuilding, setIsBuilding] = useState(false);
  const [progress, setProgress] = useState(0);
  const [success, setSuccess] = useState(false);
  const [error, setError] = useState<string | null>(null);

  // Config state
  const [variationAmount, setVariationAmount] = useState(CYCLE_ENGINE_DEFAULTS.variationAmount);
  const [velocityProcessing, setVelocityProcessing] = useState<VelocityProcessing>(
    CYCLE_ENGINE_DEFAULTS.velocityProcessing
  );
  const [humanize, setHumanize] = useState(CYCLE_ENGINE_DEFAULTS.humanize);

  const pad = pads[activePadIndex];
  if (!pad) return null;

  const layer = pad.layers[activeLayerIndex];
  const currentSample = layer?.sampleId
    ? samples.find((s) => s.id === layer.sampleId)
    : null;

  // Check if any layer already has content
  const hasExistingLayers = pad.layers.some((l) => l.active && l.sampleId);

  const handleBuild = useCallback(async () => {
    if (!currentSample) return;

    // Capture pad index at invocation time (prevents stale closure)
    const capturedPadIndex = activePadIndex;

    setIsBuilding(true);
    setProgress(0);
    setSuccess(false);
    setError(null);

    try {
      // Snapshot pad state for undo support (8-layer build is destructive)
      const history = useHistoryStore.getState();
      history.snapshot(usePadStore.getState().pads);

      const config: Partial<CycleEngineConfig> = {
        variationAmount,
        velocityProcessing,
        humanize,
      };

      const results = await generate8LayerCycle(
        currentSample.buffer,
        currentSample.name,
        config,
        (pct) => setProgress(pct)
      );

      // Add all generated samples to the audio store and assign to pad
      const store = usePadStore.getState();
      for (const result of results) {
        addSample(result.sample);

        store.assignSampleToLayer(
          capturedPadIndex,
          result.layerIndex,
          result.sample.id,
          result.sample.name,
          result.sample.fileName,
          result.sample.rootNote
        );

        store.updateLayer(capturedPadIndex, result.layerIndex, {
          velStart: result.velStart,
          velEnd: result.velEnd,
          volume: result.volume,
          pan: result.pan,
          tuneFine: result.tuneFine,
          pitchRandom: result.pitchRandom,
          volumeRandom: result.volumeRandom,
          probability: result.probability,
        });
      }

      // Set pad play mode to velocity (MPC alternates RR within each velocity zone)
      store.setPadPlayMode(capturedPadIndex, 'velocity');

      // Push history entry so the build can be undone with Cmd+Z
      history.pushState('8-Layer Cycle build', usePadStore.getState().pads);

      setSuccess(true);
    } catch (err) {
      console.error('8-Layer Cycle build failed:', err);
      const msg = err instanceof Error ? err.message : 'Build failed';
      setError(msg);
      useToastStore.getState().addToast({
        type: 'error',
        title: 'Cycle Engine failed',
        message: msg,
      });
    } finally {
      // Reset progress BEFORE hiding the bar — avoids a visible flash
      // where progress drops to 0% while isBuilding is still true.
      setProgress(0);
      setIsBuilding(false);
    }
  }, [
    currentSample,
    variationAmount,
    velocityProcessing,
    humanize,
    activePadIndex,
    addSample,
  ]);

  // Collapsed button
  if (!isOpen) {
    return (
      <button
        onClick={() => {
          setIsOpen(true);
          setSuccess(false);
          setError(null);
        }}
        disabled={!currentSample}
        className="px-2 py-1 rounded text-[10px] font-medium
          bg-surface-alt text-text-secondary hover:text-text-primary
          hover:bg-surface transition-all disabled:opacity-50
          border border-transparent hover:border-border"
        title="Generate 8 layers: 4 velocity tiers x 2 round-robin variations"
      >
        {'\u{1F504}'} 8-Layer Cycle
      </button>
    );
  }

  // Expanded panel
  return (
    <div className="space-y-3 p-3 bg-surface-alt rounded-lg border border-border animate-in fade-in slide-in-from-top-1 duration-150">
      {/* Header */}
      <div className="flex items-center justify-between">
        <div>
          <p className="text-[10px] text-text-muted uppercase tracking-wider font-semibold">
            8-Layer Cycle Engine
          </p>
          <p className="text-[9px] text-text-muted mt-0.5">
            4 velocity tiers &times; 2 round-robin &mdash; eliminates machine-gun effect
          </p>
        </div>
        <button
          onClick={() => setIsOpen(false)}
          className="text-text-muted hover:text-text-primary text-xs"
          aria-label="Close Cycle Engine panel"
        >
          {'\u2715'}
        </button>
      </div>

      {/* Source sample display */}
      {currentSample && (
        <div className="flex items-center gap-1.5 px-2 py-1 rounded bg-surface text-[10px]">
          <span className="text-accent-teal font-bold">SRC</span>
          <span className="text-text-primary truncate">{currentSample.name}</span>
        </div>
      )}

      {/* 4x2 Layer grid visualization */}
      <div className="space-y-1">
        <p className="text-[10px] text-text-muted uppercase tracking-wider">Layer Map</p>
        <div className="grid grid-cols-[auto_1fr_1fr] gap-1 text-[9px]">
          {/* Column headers */}
          <div className="text-text-muted text-right pr-1" />
          <div className="text-center text-text-muted font-medium">RR-A</div>
          <div className="text-center text-text-muted font-medium">RR-B</div>

          {/* Tier rows */}
          {VELOCITY_TIERS.map((tier, tIdx) => {
            const color = TIER_COLORS[tIdx];
            return (
              <React.Fragment key={tier.name}>
                {/* Tier label */}
                <div className="text-right pr-1.5 py-1 text-text-muted font-medium whitespace-nowrap">
                  {tier.name}
                  <span className="text-[7px] opacity-60 ml-0.5">
                    {tier.range[0]}-{tier.range[1]}
                  </span>
                </div>
                {/* RR-A cell */}
                <div
                  className="rounded px-1.5 py-1 text-center font-mono"
                  style={{ backgroundColor: color.replace('VAR', '0.15') }}
                >
                  <span style={{ color: color.replace('VAR', '0.9') }}>
                    L{tIdx * 2 + 1}
                  </span>
                </div>
                {/* RR-B cell */}
                <div
                  className="rounded px-1.5 py-1 text-center font-mono"
                  style={{ backgroundColor: color.replace('VAR', '0.10') }}
                >
                  <span style={{ color: color.replace('VAR', '0.7') }}>
                    L{tIdx * 2 + 2}
                  </span>
                </div>
              </React.Fragment>
            );
          })}
        </div>
      </div>

      {/* Variation Amount slider */}
      <Slider
        label="Variation Amount"
        value={variationAmount * 100}
        onChange={(v) => setVariationAmount(v / 100)}
        min={0}
        max={100}
        step={1}
        unit="%"
      />

      {/* Velocity Processing selector */}
      <div className="space-y-1.5">
        <p className="text-[10px] text-text-muted uppercase tracking-wider">Velocity Processing</p>
        <div className="flex gap-1">
          {PROCESSING_MODES.map((pm) => (
            <button
              key={pm.id}
              onClick={() => setVelocityProcessing(pm.id)}
              className={`flex-1 px-1.5 py-1.5 rounded text-[9px] font-medium transition-all border
                ${velocityProcessing === pm.id
                  ? 'bg-accent-teal/10 border-accent-teal/30 text-accent-teal'
                  : 'bg-surface border-transparent text-text-muted hover:text-text-secondary'
                }`}
              title={pm.description}
            >
              {pm.label}
            </button>
          ))}
        </div>
        <p className="text-[8px] text-text-muted">
          {PROCESSING_MODES.find((m) => m.id === velocityProcessing)?.description}
        </p>
      </div>

      {/* Humanize toggle */}
      <div className="flex items-center justify-between">
        <div>
          <p className="text-[10px] text-text-secondary font-medium">Humanize</p>
          <p className="text-[8px] text-text-muted">Adds pitch/volume randomization per layer</p>
        </div>
        <button
          onClick={() => setHumanize(!humanize)}
          className={`w-9 h-5 rounded-full transition-all relative
            ${humanize ? 'bg-accent-teal' : 'bg-surface'}`}
        >
          <span
            className={`absolute top-0.5 w-4 h-4 rounded-full bg-white shadow transition-all
              ${humanize ? 'left-[18px]' : 'left-0.5'}`}
          />
        </button>
      </div>

      {/* Warning if layers exist */}
      {hasExistingLayers && !success && (
        <div className="px-2 py-1.5 rounded bg-amber-500/10 border border-amber-500/20 text-[9px] text-amber-600">
          Will replace all existing layers on this pad
        </div>
      )}

      {/* Error message */}
      {error && (
        <div className="px-2 py-1.5 rounded bg-red-500/10 border border-red-500/20 text-[9px] text-red-500">
          {error}
        </div>
      )}

      {/* Success message */}
      {success && (
        <div className="px-2 py-1.5 rounded bg-emerald-500/10 border border-emerald-500/20 text-[9px] text-emerald-600">
          Built 8 layers: {VELOCITY_TIERS.map((t) => t.name).join(' / ')} &mdash; each with 2 round-robin variations. Play mode set to Velocity.
        </div>
      )}

      {/* Build button + progress */}
      <div className="flex gap-1 pt-1">
        <Button
          variant="primary"
          size="sm"
          disabled={!currentSample || isBuilding}
          loading={isBuilding}
          onClick={handleBuild}
        >
          {isBuilding ? `Building... (${progress}/9)` : 'Build 8 Layers'}
        </Button>
        <Button
          variant="ghost"
          size="sm"
          onClick={() => setIsOpen(false)}
          disabled={isBuilding}
        >
          Cancel
        </Button>
      </div>

      {isBuilding && (
        <div className="h-1 bg-surface rounded-full overflow-hidden">
          <div
            className="h-full bg-accent-teal transition-all duration-300"
            style={{ width: `${(progress / 9) * 100}%` }}
          />
        </div>
      )}
    </div>
  );
}
