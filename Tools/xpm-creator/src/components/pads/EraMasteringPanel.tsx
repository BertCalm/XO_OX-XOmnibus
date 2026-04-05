'use client';

import React, { useState } from 'react';
import { ERA_INFO, applyEraMastering } from '@/lib/audio/eraMastering';
import type { MasteringEra } from '@/lib/audio/eraMastering';
import { usePadStore } from '@/stores/padStore';
import { useAudioStore } from '@/stores/audioStore';
import { useToastStore } from '@/stores/toastStore';
import { generateWaveformPeaks } from '@/lib/audio/audioUtils';
import { getDecodedBuffer, invalidateCache } from '@/lib/audio/audioBufferCache';
import { encodeWavAsync } from '@/lib/audio/wavEncoder';
import Button from '@/components/ui/Button';
import ConfirmDialog from '@/components/ui/ConfirmDialog';
import Slider from '@/components/ui/Slider';

interface EraMasteringPanelProps {
  padIndex: number;
}

const ERA_KEYS: MasteringEra[] = ['1993', '2000', 'modern'];

/** Apply era mastering to a single sample by ID; returns true on success */
async function masterSampleInPlace(
  sampleId: string,
  era: MasteringEra,
  intensityPct: number,
): Promise<boolean> {
  const { samples, updateSample } = useAudioStore.getState();
  const sample = samples.find((s) => s.id === sampleId);
  if (!sample) return false;

  const audioBuffer = await getDecodedBuffer(sampleId, sample.buffer);
  const mastered = await applyEraMastering(audioBuffer, {
    era,
    intensity: intensityPct / 100,
  });

  const wavData = await encodeWavAsync(mastered, 16);
  const newPeaks = generateWaveformPeaks(mastered);

  updateSample(sampleId, {
    buffer: wavData,
    waveformPeaks: newPeaks,
    duration: mastered.duration,
  });
  // Invalidate decode cache since the buffer content changed
  invalidateCache(sampleId);

  return true;
}

export default function EraMasteringPanel({ padIndex }: EraMasteringPanelProps) {
  const [selectedEra, setSelectedEra] = useState<MasteringEra>('1993');
  const [intensity, setIntensity] = useState(50);
  const [isProcessing, setIsProcessing] = useState(false);
  const [showConfirmAll, setShowConfirmAll] = useState(false);

  const { addToast } = useToastStore();

  const handleApplyToPad = async () => {
    setIsProcessing(true);
    try {
      const { pads, activeLayerIndex } = usePadStore.getState();
      const pad = pads[padIndex];
      const layer = pad?.layers[activeLayerIndex];
      if (!layer?.sampleId) {
        addToast({ type: 'warning', title: 'No sample loaded on active layer' });
        return;
      }

      const ok = await masterSampleInPlace(layer.sampleId, selectedEra, intensity);
      if (ok) {
        addToast({ type: 'success', title: `${ERA_INFO[selectedEra].label} applied to Pad ${padIndex + 1}` });
      } else {
        addToast({ type: 'error', title: 'Sample not found' });
      }
    } catch (error) {
      console.error('Era mastering failed:', error);
      addToast({ type: 'error', title: 'Era mastering failed', message: String(error) });
    } finally {
      setIsProcessing(false);
    }
  };

  const handleApplyToAll = async () => {
    setIsProcessing(true);
    try {
      const { pads } = usePadStore.getState();
      let processed = 0;
      let failed = 0;

      for (let pi = 0; pi < pads.length; pi++) {
        const pad = pads[pi];
        for (const layer of pad.layers) {
          if (layer.active && layer.sampleId) {
            try {
              await masterSampleInPlace(layer.sampleId, selectedEra, intensity);
              processed++;
            } catch {
              failed++;
            }
          }
        }
      }

      if (processed > 0) {
        const msg = failed > 0 ? ` (${failed} failed)` : '';
        addToast({ type: 'success', title: `${ERA_INFO[selectedEra].label} applied to ${processed} samples${msg}` });
      } else {
        addToast({ type: 'warning', title: 'No samples to process' });
      }
    } catch (error) {
      console.error('Era mastering (all) failed:', error);
      addToast({ type: 'error', title: 'Era mastering failed', message: String(error) });
    } finally {
      setIsProcessing(false);
    }
  };

  return (
    <div className="space-y-3 p-3 bg-surface-alt rounded-lg border border-border">
      {/* Header */}
      <div className="flex items-center gap-1.5">
        <span className="text-sm">{'\uD83C\uDF9B\uFE0F'}</span>
        <p className="text-[10px] text-text-muted uppercase tracking-wider font-semibold">
          Era Mastering
        </p>
        <span className="ml-auto text-[9px] text-text-muted font-mono">
          Pad {padIndex + 1}
        </span>
      </div>

      {/* Era selector buttons */}
      <div className="flex gap-1.5">
        {ERA_KEYS.map((eraKey) => {
          const info = ERA_INFO[eraKey];
          const isActive = selectedEra === eraKey;

          return (
            <button
              key={eraKey}
              onClick={() => setSelectedEra(eraKey)}
              className={`flex-1 flex flex-col items-center gap-0.5 p-2 rounded-lg
                transition-all border text-center
                ${isActive
                  ? 'bg-accent-teal/10 border-accent-teal text-accent-teal'
                  : 'bg-surface-bg border-border text-text-secondary hover:bg-surface-alt'
                }`}
            >
              <span className="text-base">{info.icon}</span>
              <span className="text-[9px] font-semibold leading-tight">
                {info.label}
              </span>
              <span className="text-[8px] text-text-muted leading-tight">
                {info.description}
              </span>
            </button>
          );
        })}
      </div>

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

      {/* Action buttons */}
      <div className="flex flex-col gap-1.5">
        <Button
          variant="primary"
          size="sm"
          disabled={isProcessing}
          loading={isProcessing}
          onClick={handleApplyToPad}
          className="w-full"
        >
          {isProcessing ? 'Processing...' : 'Apply to Pad'}
        </Button>
        <Button
          variant="secondary"
          size="sm"
          disabled={isProcessing}
          onClick={() => setShowConfirmAll(true)}
          className="w-full"
        >
          Apply to All Pads
        </Button>
      </div>

      {/* Confirmation for Apply to All */}
      <ConfirmDialog
        open={showConfirmAll}
        onConfirm={() => {
          setShowConfirmAll(false);
          handleApplyToAll();
        }}
        onCancel={() => setShowConfirmAll(false)}
        title="Apply to All Pads?"
        message={`This will apply ${ERA_INFO[selectedEra].label} mastering at ${intensity}% intensity to every loaded sample across all pads. This cannot be undone.`}
        confirmLabel="Apply to All"
        variant="warning"
      />
    </div>
  );
}
