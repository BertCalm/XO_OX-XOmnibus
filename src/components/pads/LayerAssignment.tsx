'use client';

import React from 'react';
import { usePadStore } from '@/stores/padStore';
import { useAudioStore } from '@/stores/audioStore';
import Button from '@/components/ui/Button';
import { MAX_LAYERS_PER_PAD } from '@/constants/mpcDefaults';

export default function LayerAssignment() {
  const pads = usePadStore((s) => s.pads);
  const activePadIndex = usePadStore((s) => s.activePadIndex);
  const assignSampleToLayer = usePadStore((s) => s.assignSampleToLayer);
  const withHistory = usePadStore((s) => s.withHistory);
  const samples = useAudioStore((s) => s.samples);

  const pad = pads[activePadIndex];
  if (!pad) return null;

  const activeLayers = pad.layers.filter((l) => l.active && l.sampleId);
  const canAddLayer = activeLayers.length < MAX_LAYERS_PER_PAD;

  const handleAssignSample = (sampleId: string, sampleName: string, sampleFile: string, rootNote: number) => {
    if (!canAddLayer) return;
    const emptyIndex = pad.layers.findIndex((l) => !l.active || !l.sampleId);
    if (emptyIndex >= 0) {
      withHistory('Assign sample to layer', () =>
        assignSampleToLayer(activePadIndex, emptyIndex, sampleId, sampleName, sampleFile, rootNote)
      );
    }
  };

  return (
    <div className="space-y-2">
      <div className="flex items-center justify-between">
        <span className="label">Assign to Pad {activePadIndex + 1}</span>
        <span className="text-[10px] text-text-muted">
          {activeLayers.length}/{MAX_LAYERS_PER_PAD} layers
        </span>
      </div>

      {!canAddLayer && (
        <p className="text-xs text-amber-600 bg-amber-50 px-2 py-1 rounded">
          Maximum {MAX_LAYERS_PER_PAD} layers reached
        </p>
      )}

      <div className="space-y-1 max-h-40 overflow-auto scrollbar-thin">
        {samples.map((sample) => (
          <button
            key={sample.id}
            disabled={!canAddLayer}
            onClick={() => handleAssignSample(sample.id, sample.name, sample.fileName, sample.rootNote)}
            className="w-full flex items-center gap-2 px-2 py-1.5 rounded-lg text-left
              hover:bg-surface-alt transition-colors disabled:opacity-50 disabled:pointer-events-none"
          >
            <div className="w-1.5 h-1.5 rounded-full bg-accent-teal" />
            <span className="text-xs text-text-primary truncate flex-1">{sample.name}</span>
            <svg width="12" height="12" viewBox="0 0 12 12" fill="none" className="text-text-muted">
              <path d="M6 3v6M3 6h6" stroke="currentColor" strokeWidth="1.5" strokeLinecap="round" />
            </svg>
          </button>
        ))}
      </div>
    </div>
  );
}
