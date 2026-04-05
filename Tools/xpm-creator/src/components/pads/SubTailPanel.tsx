'use client';

import React, { useState, useCallback } from 'react';
import Button from '@/components/ui/Button';
import Slider from '@/components/ui/Slider';
import { usePadStore } from '@/stores/padStore';
import { useAudioStore } from '@/stores/audioStore';
import { useToastStore } from '@/stores/toastStore';
import { getDecodedBuffer } from '@/lib/audio/audioBufferCache';
import { detectPitch } from '@/lib/audio/pitchDetector';
import { generateSubTail } from '@/lib/audio/sculptingProcessors';
import { encodeWav } from '@/lib/audio/wavEncoder';
import { generateWaveformPeaks } from '@/lib/audio/audioUtils';
import { v4 as uuid } from 'uuid';
import { useHistoryStore } from '@/stores/historyStore';

interface SubTailPanelProps {
  padIndex: number;
}

export default function SubTailPanel({ padIndex }: SubTailPanelProps) {
  const [frequency, setFrequency] = useState(60);
  const [level, setLevel] = useState(50);
  const [decayMs, setDecayMs] = useState(500);
  const [detectedPitch, setDetectedPitch] = useState<number | null>(null);
  const [isGenerating, setIsGenerating] = useState(false);

  const { addToast } = useToastStore();

  const handleAutoDetect = useCallback(async () => {
    try {
      const { pads, activeLayerIndex } = usePadStore.getState();
      const pad = pads[padIndex];
      const layer = pad?.layers[activeLayerIndex];
      if (!layer?.sampleId) {
        addToast({ type: 'warning', title: 'No sample loaded on active layer' });
        return;
      }

      const { samples } = useAudioStore.getState();
      const sample = samples.find((s) => s.id === layer.sampleId);
      if (!sample) {
        addToast({ type: 'error', title: 'Sample not found' });
        return;
      }

      const audioBuffer = await getDecodedBuffer(sample.id, sample.buffer);
      const pitch = detectPitch(audioBuffer);

      if (pitch !== null) {
        const rounded = Math.round(pitch);
        setDetectedPitch(rounded);
        setFrequency(Math.min(120, Math.max(30, rounded)));
        addToast({ type: 'success', title: `Detected pitch: ${rounded}Hz` });
      } else {
        setDetectedPitch(null);
        addToast({ type: 'info', title: 'Could not detect pitch — using manual frequency' });
      }
    } catch (error) {
      console.error('Pitch detection failed:', error);
      addToast({ type: 'error', title: 'Pitch detection failed', message: String(error) });
    }
  }, [padIndex, addToast]);

  const handleGenerate = useCallback(async () => {
    setIsGenerating(true);
    try {
      const { pads, activeLayerIndex } = usePadStore.getState();
      const pad = pads[padIndex];
      const layer = pad?.layers[activeLayerIndex];
      if (!layer?.sampleId) {
        addToast({ type: 'warning', title: 'No sample loaded on active layer' });
        return;
      }

      const { samples, addSample, updateSample } = useAudioStore.getState();
      const sample = samples.find((s) => s.id === layer.sampleId);
      if (!sample) {
        addToast({ type: 'error', title: 'Sample not found' });
        return;
      }

      const audioBuffer = await getDecodedBuffer(sample.id, sample.buffer);
      const { subBuffer, detectedFrequency } = generateSubTail(audioBuffer, {
        frequency,
        level: level / 100, // 0-100% → 0-1
        decayMs,
      });

      if (detectedFrequency !== null && detectedPitch === null) {
        setDetectedPitch(Math.round(detectedFrequency));
      }

      // Encode and add the sub-tail as a new sample, then assign to next empty layer
      const wavData = encodeWav(subBuffer, 16);
      const newPeaks = generateWaveformPeaks(subBuffer);
      const subSampleId = uuid();

      addSample({
        id: subSampleId,
        name: `${sample.name}_sub`,
        fileName: `${sample.name}_sub.WAV`,
        duration: subBuffer.duration,
        sampleRate: subBuffer.sampleRate,
        channels: subBuffer.numberOfChannels,
        bitDepth: 16,
        buffer: wavData,
        waveformPeaks: newPeaks,
        rootNote: 60,
        createdAt: Date.now(),
      });

      // Re-read pad state after async work — the pre-await `pad` reference
      // may be stale if another operation modified this pad during DSP.
      const freshPadState = usePadStore.getState();
      const freshPad = freshPadState.pads[padIndex];
      const freshActiveLayer = freshPadState.activeLayerIndex;
      const nextEmpty = freshPad
        ? freshPad.layers.findIndex(
            (l, i) => i > freshActiveLayer && (!l.active || !l.sampleId)
          )
        : -1;
      if (nextEmpty >= 0) {
        const history = useHistoryStore.getState();
        history.snapshot(usePadStore.getState().pads);

        const { assignSampleToLayer } = usePadStore.getState();
        assignSampleToLayer(
          padIndex,
          nextEmpty,
          subSampleId,
          `${sample.name}_sub`,
          `${sample.name}_sub.WAV`
        );

        history.pushState('Apply Sub Tail', usePadStore.getState().pads);

        addToast({ type: 'success', title: `Sub-tail added to Layer ${nextEmpty + 1}` });
      } else {
        addToast({
          type: 'warning',
          title: 'Sub-tail created in library',
          message: 'No empty layer available — drag it from the library manually',
        });
      }
    } catch (error) {
      console.error('Sub-tail generation failed:', error);
      addToast({ type: 'error', title: 'Sub-tail generation failed', message: String(error) });
    } finally {
      setIsGenerating(false);
    }
  }, [padIndex, frequency, level, decayMs, detectedPitch, addToast]);

  return (
    <div className="space-y-3 p-3 bg-surface-alt rounded-lg border border-border">
      {/* Header */}
      <div className="flex items-center gap-1.5">
        <span className="text-sm">{'\uD83D\uDD0A'}</span>
        <p className="text-[10px] text-text-muted uppercase tracking-wider font-semibold">
          Sub-Tail Generator
        </p>
        <span className="ml-auto text-[9px] text-text-muted font-mono">
          Pad {padIndex + 1}
        </span>
      </div>

      {/* Detected pitch info bar */}
      <div className="px-2 py-1.5 rounded bg-surface-bg border border-border text-[9px]">
        {detectedPitch !== null ? (
          <span className="text-accent-teal font-medium">
            Auto-detected: {detectedPitch}Hz
          </span>
        ) : (
          <span className="text-text-muted">No pitch detected</span>
        )}
      </div>

      {/* Frequency slider */}
      <Slider
        label="Sub Frequency"
        value={frequency}
        onChange={setFrequency}
        min={30}
        max={120}
        step={1}
        unit=" Hz"
      />

      {/* Level slider */}
      <Slider
        label="Sub Level"
        value={level}
        onChange={setLevel}
        min={0}
        max={100}
        step={1}
        unit="%"
      />

      {/* Decay slider */}
      <Slider
        label="Decay (ms)"
        value={decayMs}
        onChange={setDecayMs}
        min={100}
        max={2000}
        step={10}
        unit=" ms"
      />

      {/* Action buttons */}
      <div className="flex flex-col gap-1.5">
        <Button
          variant="secondary"
          size="sm"
          onClick={handleAutoDetect}
          className="w-full"
        >
          Auto-Detect Pitch
        </Button>
        <Button
          variant="primary"
          size="sm"
          disabled={isGenerating}
          loading={isGenerating}
          onClick={handleGenerate}
          className="w-full"
        >
          {isGenerating ? 'Generating...' : 'Generate Sub Tail'}
        </Button>
      </div>
    </div>
  );
}
