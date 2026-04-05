'use client';

import React, { useCallback, useState } from 'react';
import { usePadStore } from '@/stores/padStore';
import { useAudioStore } from '@/stores/audioStore';
import Slider from '@/components/ui/Slider';
import Button from '@/components/ui/Button';
import type { LayerPlayMode, TriggerMode } from '@/types';
import { midiNoteToName } from '@/types';
import { generateGhostNote } from '@/lib/audio/ghostNoteGenerator';
import { flipPhase } from '@/lib/audio/chopProcessors';
import { generateWaveformPeaks } from '@/lib/audio/audioUtils';
import { encodeWav } from '@/lib/audio/wavEncoder';
import { resamplePad } from '@/lib/audio/padResampler';
import { useEnvelopeStore } from '@/stores/envelopeStore';
import { getCachedBuffer, getDecodedBuffer, invalidateCache } from '@/lib/audio/audioBufferCache';
import { v4 as uuid } from 'uuid';
import type { AudioSample } from '@/types';
import AutoLayerGenerator from './AutoLayerGenerator';
import KeygroupBuilder from './KeygroupBuilder';
import VelocityCurveSelector from './VelocityCurveSelector';
import SpaceFoldPanel from './SpaceFoldPanel';
import VibeCheckButton from './VibeCheckButton';
import AutoChokePanel from './AutoChokePanel';
import CycleEnginePanel from './CycleEnginePanel';
import SpectralAirPanel from './SpectralAirPanel';
import BatchTailTamer from '@/components/tools/BatchTailTamer';
import TimeTraveler from '@/components/tools/TimeTraveler';
import SubTailPanel from './SubTailPanel';
import EraMasteringPanel from './EraMasteringPanel';
import ExpressionModePanel from './ExpressionModePanel';
import { useHistoryStore } from '@/stores/historyStore';
import { useToastStore } from '@/stores/toastStore';

// Module-scope constants — prevents recreating arrays on every render
const PLAY_MODES: { id: LayerPlayMode; label: string }[] = [
  { id: 'simultaneous', label: 'Simul' },
  { id: 'velocity', label: 'Vel' },
  { id: 'cycle', label: 'Cycle' },
  { id: 'random', label: 'Rand' },
];

const TRIGGER_MODES: { id: TriggerMode; label: string; desc: string }[] = [
  { id: 'oneshot', label: 'One Shot', desc: 'Plays to end' },
  { id: 'noteon', label: 'Note On', desc: 'Retriggers on hit' },
  { id: 'noteoff', label: 'Note Off', desc: 'Stops on release' },
];

export default function PadLayerEditor() {
  const pads = usePadStore((s) => s.pads);
  const activePadIndex = usePadStore((s) => s.activePadIndex);
  const activeLayerIndex = usePadStore((s) => s.activeLayerIndex);
  const setActiveLayer = usePadStore((s) => s.setActiveLayer);
  const updateLayer = usePadStore((s) => s.updateLayer);
  const removeLayerSample = usePadStore((s) => s.removeLayerSample);
  const setPadPlayMode = usePadStore((s) => s.setPadPlayMode);
  const setPadTriggerMode = usePadStore((s) => s.setPadTriggerMode);
  const assignSampleToLayer = usePadStore((s) => s.assignSampleToLayer);
  const withHistory = usePadStore((s) => s.withHistory);

  const samples = useAudioStore((s) => s.samples);
  const addSample = useAudioStore((s) => s.addSample);
  const updateSample = useAudioStore((s) => s.updateSample);
  const [isGenerating, setIsGenerating] = useState(false);
  const [showAdvancedTools, setShowAdvancedTools] = useState(false);

  const pad = pads[activePadIndex];
  if (!pad) return null;

  const layer = pad.layers[activeLayerIndex];
  if (!layer) return null;

  // Use module-scope constants to avoid recreating arrays every render
  const playModes = PLAY_MODES;
  const triggerModes = TRIGGER_MODES;

  return (
    <div className="space-y-4 p-4">
      <div>
        <span className="label">Pad {activePadIndex + 1}</span>
        <div className="flex gap-1 mb-3">
          {playModes.map((mode) => (
            <button
              key={mode.id}
              aria-pressed={pad.playMode === mode.id}
              onClick={() => withHistory('Change play mode', () => setPadPlayMode(activePadIndex, mode.id))}
              className={`px-2 py-1 rounded text-xs font-medium transition-all
                ${pad.playMode === mode.id
                  ? 'bg-accent-teal text-white'
                  : 'bg-surface-alt text-text-secondary hover:text-text-primary'
                }`}
            >
              {mode.label}
            </button>
          ))}
        </div>
        <div className="flex gap-1 mb-3">
          {triggerModes.map((mode) => (
            <button
              key={mode.id}
              aria-pressed={pad.triggerMode === mode.id}
              onClick={() => withHistory('Change trigger mode', () => setPadTriggerMode(activePadIndex, mode.id))}
              className={`px-2 py-1 rounded text-xs font-medium transition-all
                ${pad.triggerMode === mode.id
                  ? 'bg-accent-plum text-white'
                  : 'bg-surface-alt text-text-secondary hover:text-text-primary'
                }`}
              title={mode.desc}
            >
              {mode.label}
            </button>
          ))}
        </div>
      </div>

      {/* Layer tabs */}
      <div className="flex gap-1" role="tablist" aria-label="Sample layers">
        {pad.layers.map((l, i) => (
          <button
            key={i}
            role="tab"
            aria-selected={activeLayerIndex === i}
            aria-label={l.active && l.sampleId ? `Layer ${i + 1}: ${l.sampleName}` : `Layer ${i + 1}: empty`}
            tabIndex={activeLayerIndex === i ? 0 : -1}
            onClick={() => setActiveLayer(i)}
            className={`flex-1 px-2 py-1.5 rounded-lg text-xs font-medium
              transition-all border
              ${activeLayerIndex === i
                ? 'bg-surface border-accent-teal text-accent-teal shadow-card'
                : l.active && l.sampleId
                  ? 'bg-accent-plum-50 border-accent-plum/20 text-accent-plum'
                  : 'bg-surface-alt border-transparent text-text-muted'
              }`}
          >
            L{i + 1}
          </button>
        ))}
      </div>

      {/* Layer content */}
      <div className="space-y-3">
        {layer.active && layer.sampleId ? (
          <>
            <div className="flex items-center justify-between">
              <div className="min-w-0">
                <p className="text-xs font-medium text-text-primary truncate">
                  {layer.sampleName}
                </p>
                <p className="text-[10px] text-text-muted">
                  Root: {midiNoteToName(layer.rootNote)}
                </p>
              </div>
              <Button
                variant="ghost"
                size="sm"
                onClick={() => withHistory('Remove layer sample', () => removeLayerSample(activePadIndex, activeLayerIndex))}
              >
                Remove
              </Button>
            </div>

            <Slider
              label="Volume"
              value={layer.volume}
              onChange={(v) => updateLayer(activePadIndex, activeLayerIndex, { volume: v })}
              min={0}
              max={1}
              step={0.01}
            />

            <Slider
              label={`Pan ${layer.pan < 0.49 ? 'L' + Math.round((0.5 - layer.pan) * 200) : layer.pan > 0.51 ? 'R' + Math.round((layer.pan - 0.5) * 200) : 'C'}`}
              value={layer.pan}
              onChange={(v) => updateLayer(activePadIndex, activeLayerIndex, { pan: v })}
              min={0}
              max={1}
              step={0.01}
            />

            <div className="grid grid-cols-2 gap-2">
              <Slider
                label="Vel Start"
                value={layer.velStart}
                onChange={(v) => updateLayer(activePadIndex, activeLayerIndex, { velStart: v })}
                min={0}
                max={127}
                step={1}
              />
              <Slider
                label="Vel End"
                value={layer.velEnd}
                onChange={(v) => updateLayer(activePadIndex, activeLayerIndex, { velEnd: v })}
                min={0}
                max={127}
                step={1}
              />
            </div>

            <Slider
              label="Tune Fine"
              value={layer.tuneFine}
              onChange={(v) => updateLayer(activePadIndex, activeLayerIndex, { tuneFine: v })}
              min={-100}
              max={100}
              step={1}
              unit=" cents"
            />

            <Slider
              label="Tune Coarse"
              value={layer.tuneCoarse}
              onChange={(v) => updateLayer(activePadIndex, activeLayerIndex, { tuneCoarse: v })}
              min={-36}
              max={36}
              step={1}
              unit=" st"
            />

            <Slider
              label={`Root Note (${midiNoteToName(layer.rootNote)})`}
              value={layer.rootNote}
              onChange={(v) => updateLayer(activePadIndex, activeLayerIndex, { rootNote: v })}
              min={0}
              max={127}
              step={1}
            />

            <Slider
              label="Probability"
              value={layer.probability}
              onChange={(v) => updateLayer(activePadIndex, activeLayerIndex, { probability: v })}
              min={0}
              max={100}
              step={1}
              unit="%"
            />

            {/* Layer tools */}
            <div className="pt-2 border-t border-border space-y-1.5">
              <button
                onClick={() => setShowAdvancedTools(!showAdvancedTools)}
                className="flex items-center gap-1 w-full text-left"
              >
                <svg width="10" height="10" viewBox="0 0 10 10" className={`text-text-muted transition-transform ${showAdvancedTools ? 'rotate-90' : ''}`}>
                  <path d="M3 1l4 4-4 4" stroke="currentColor" strokeWidth="1.5" strokeLinecap="round" fill="none"/>
                </svg>
                <span className="text-[10px] text-text-muted uppercase tracking-wider">
                  Advanced Tools (16 tools)
                </span>
              </button>
              {showAdvancedTools && (
              <div className="space-y-3">
              <div className="flex gap-1 flex-wrap">
                {/* Ghost Note Generator */}
                <button
                  disabled={isGenerating}
                  onClick={async () => {
                    if (!layer.sampleId) return;
                    const sample = samples.find((s) => s.id === layer.sampleId);
                    if (!sample) return;

                    // Capture indices NOW — they may change during async gap
                    const capturedPadIdx = activePadIndex;
                    const capturedLayerIdx = activeLayerIndex;

                    setIsGenerating(true);
                    try {
                      const audioBuffer = await getDecodedBuffer(sample.id, sample.buffer);
                      const ghostBuffer = await generateGhostNote(audioBuffer);
                      const ghostWav = encodeWav(ghostBuffer, 16);
                      const ghostPeaks = generateWaveformPeaks(ghostBuffer);

                      const ghostSample: AudioSample = {
                        id: uuid(),
                        name: `${sample.name}_ghost`,
                        fileName: `${sample.name}_ghost.WAV`,
                        duration: ghostBuffer.duration,
                        sampleRate: ghostBuffer.sampleRate,
                        channels: ghostBuffer.numberOfChannels,
                        bitDepth: 16,
                        buffer: ghostWav,
                        waveformPeaks: ghostPeaks,
                        rootNote: sample.rootNote,
                        createdAt: Date.now(),
                      };

                      addSample(ghostSample);

                      // Use captured indices — reads live pad state but with stable pad/layer index
                      const ghostStore = usePadStore.getState();
                      const capturedPad = ghostStore.pads[capturedPadIdx];
                      const nextEmpty = capturedPad
                        ? capturedPad.layers.findIndex(
                            (l, i) => i > capturedLayerIdx && (!l.active || !l.sampleId)
                          )
                        : -1;
                      if (nextEmpty >= 0) {
                        const history = useHistoryStore.getState();
                        history.snapshot(usePadStore.getState().pads);

                        ghostStore.assignSampleToLayer(
                          capturedPadIdx,
                          nextEmpty,
                          ghostSample.id,
                          ghostSample.name,
                          ghostSample.fileName,
                          ghostSample.rootNote
                        );
                        // Set ghost layer velocity range (lower half)
                        ghostStore.updateLayer(capturedPadIdx, capturedLayerIdx, { velStart: 64, velEnd: 127 });
                        ghostStore.updateLayer(capturedPadIdx, nextEmpty, { velStart: 0, velEnd: 63 });
                        // Switch pad to velocity mode
                        ghostStore.setPadPlayMode(capturedPadIdx, 'velocity');

                        history.pushState('Generate ghost note', usePadStore.getState().pads);

                        useToastStore.getState().addToast({
                          type: 'success',
                          title: `Ghost note added to Layer ${nextEmpty + 1}`,
                        });
                      } else {
                        useToastStore.getState().addToast({
                          type: 'warning',
                          title: 'No empty layer for ghost note',
                          message: 'Ghost sample saved to library — drag it manually',
                        });
                      }
                    } catch (error) {
                      console.error('Ghost note generation failed:', error);
                      useToastStore.getState().addToast({
                        type: 'error',
                        title: 'Ghost note generation failed',
                        message: String(error),
                      });
                    }
                    setIsGenerating(false);
                  }}
                  className="px-2 py-1 rounded text-[10px] font-medium
                    bg-surface-alt text-text-secondary hover:text-text-primary
                    hover:bg-surface transition-all disabled:opacity-50
                    border border-transparent hover:border-border"
                  title="Generate a ghost note version on the next layer"
                >
                  👻 Ghost Note
                </button>

                {/* Phase Flip */}
                <button
                  disabled={isGenerating}
                  onClick={async () => {
                    if (!layer.sampleId) return;
                    const sample = samples.find((s) => s.id === layer.sampleId);
                    if (!sample) return;

                    setIsGenerating(true);
                    try {
                      const audioBuffer = await getDecodedBuffer(sample.id, sample.buffer);
                      const flipped = flipPhase(audioBuffer);
                      const flippedWav = encodeWav(flipped, sample.bitDepth || 16);
                      const flippedPeaks = generateWaveformPeaks(flipped);

                      updateSample(sample.id, {
                        buffer: flippedWav,
                        waveformPeaks: flippedPeaks,
                      });

                      // Evict the old decoded AudioBuffer so playback uses the flipped version
                      invalidateCache(sample.id);

                      useToastStore.getState().addToast({
                        type: 'success',
                        title: 'Phase flipped',
                      });
                    } catch (error) {
                      console.error('Phase flip failed:', error);
                      useToastStore.getState().addToast({
                        type: 'error',
                        title: 'Phase flip failed',
                        message: String(error),
                      });
                    }
                    setIsGenerating(false);
                  }}
                  className="px-2 py-1 rounded text-[10px] font-medium
                    bg-surface-alt text-text-secondary hover:text-text-primary
                    hover:bg-surface transition-all disabled:opacity-50
                    border border-transparent hover:border-border"
                  title="Invert the polarity of this sample"
                >
                  ⟁ Flip Phase
                </button>

                {/* Velocity Curve Presets */}
                <VelocityCurveSelector />

                {/* Space Fold Stereo Imaging */}
                <SpaceFoldPanel />

                {/* Expression Map — Auto Layer Generator */}
                <AutoLayerGenerator />

                {/* Keygroup Builder */}
                <KeygroupBuilder />

                {/* Vibe Check Randomizer */}
                <VibeCheckButton />

                {/* Spectral Air Injector */}
                <SpectralAirPanel />

                {/* 8-Layer Cycle Engine */}
                <CycleEnginePanel />

                {/* Auto-Choke Intelligence */}
                <AutoChokePanel />

                {/* Batch Tail Tamer */}
                <BatchTailTamer />

                {/* Time Traveler — Era Processing */}
                <TimeTraveler />

                {/* Resample Pad */}
                <button
                  disabled={isGenerating}
                  onClick={async () => {
                    // Capture pad index at invocation time (stale closure safety)
                    const capturedPadIdx = activePadIndex;
                    const padState = usePadStore.getState();
                    const currentPad = padState.pads[capturedPadIdx];
                    if (!currentPad) return;
                    const activeLayers = currentPad.layers.filter((l) => l.active && l.sampleId);
                    if (activeLayers.length === 0) return;

                    setIsGenerating(true);
                    try {
                      // Ensure all layer buffers are decoded and cached
                      const audioStoreState = useAudioStore.getState();
                      for (const lyr of activeLayers) {
                        if (lyr.sampleId && !getCachedBuffer(lyr.sampleId)) {
                          const s = audioStoreState.samples.find((sm) => sm.id === lyr.sampleId);
                          if (s) await getDecodedBuffer(lyr.sampleId, s.buffer);
                        }
                      }

                      const envSettings = useEnvelopeStore.getState().getEnvelope(capturedPadIdx);
                      const sampleLookup = (sampleId: string) => getCachedBuffer(sampleId);

                      const rendered = await resamplePad(currentPad, envSettings, sampleLookup, 127);
                      const wavData = encodeWav(rendered, 16);
                      const newPeaks = generateWaveformPeaks(rendered);

                      const resampledSample: AudioSample = {
                        id: uuid(),
                        name: `Pad${capturedPadIdx + 1}_resample`,
                        fileName: `Pad${capturedPadIdx + 1}_resample.WAV`,
                        duration: rendered.duration,
                        sampleRate: rendered.sampleRate,
                        channels: rendered.numberOfChannels,
                        bitDepth: 16,
                        buffer: wavData,
                        waveformPeaks: newPeaks,
                        rootNote: 60,
                        createdAt: Date.now(),
                      };

                      addSample(resampledSample);

                      // Assign resampled result back to the pad: clear pad, assign to layer 0
                      const store = usePadStore.getState();
                      const history = useHistoryStore.getState();
                      history.snapshot(usePadStore.getState().pads);

                      store.clearPad(capturedPadIdx);
                      store.assignSampleToLayer(
                        capturedPadIdx,
                        0,
                        resampledSample.id,
                        resampledSample.name,
                        resampledSample.fileName,
                      );

                      history.pushState('Resample pad', usePadStore.getState().pads);

                      useToastStore.getState().addToast({
                        type: 'success',
                        title: `Pad ${capturedPadIdx + 1} resampled to single layer`,
                      });
                    } catch (error) {
                      console.error('Pad resample failed:', error);
                      useToastStore.getState().addToast({
                        type: 'error',
                        title: 'Pad resample failed',
                        message: String(error),
                      });
                    }
                    setIsGenerating(false);
                  }}
                  className="px-2 py-1 rounded text-[10px] font-medium
                    bg-surface-alt text-text-secondary hover:text-text-primary
                    hover:bg-surface transition-all disabled:opacity-50
                    border border-transparent hover:border-border"
                  title="Render this pad's complete audio (all layers + envelope + filter) to a new sample"
                >
                  🔄 Resample Pad
                </button>
              </div>

              {/* Sub-Tail Generator */}
              <SubTailPanel padIndex={activePadIndex} />

              {/* Era Mastering */}
              <EraMasteringPanel padIndex={activePadIndex} />

              {/* Expression Mode */}
              <ExpressionModePanel padIndex={activePadIndex} />
              </div>
              )}
            </div>
          </>
        ) : (
          <div className="text-center py-4">
            {samples.length === 0 ? (
              <>
                {/* No samples loaded anywhere — guide user to import first */}
                <div className="space-y-3">
                  <div className="w-10 h-10 rounded-full bg-surface-alt flex items-center justify-center mx-auto">
                    <svg width="20" height="20" viewBox="0 0 24 24" fill="none" className="text-text-muted">
                      <path d="M12 16V4M8 8l4-4 4 4" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round" />
                      <path d="M20 16v2a2 2 0 01-2 2H6a2 2 0 01-2-2v-2" stroke="currentColor" strokeWidth="2" strokeLinecap="round" />
                    </svg>
                  </div>
                  <div>
                    <p className="text-xs font-medium text-text-secondary">No samples loaded yet</p>
                    <p className="text-[10px] text-text-muted mt-1">
                      Import samples in <span className="font-semibold text-text-secondary">The Crate</span> first, then drag them here
                    </p>
                  </div>
                  <div className="pt-1">
                    <p className="text-[10px] text-text-muted">
                      Or use <span className="font-semibold text-text-secondary">Kit Builder</span> to auto-create a drum kit
                    </p>
                  </div>
                </div>

                {/* Keygroup Builder — also available when no sample loaded */}
                <div className="mt-3 flex justify-center">
                  <KeygroupBuilder />
                </div>
              </>
            ) : (
              <>
                <p className="text-xs text-text-muted">
                  Drag a sample here or drop onto the pad
                </p>

                {/* Keygroup Builder — also available when no sample loaded */}
                <div className="mt-3 flex justify-center">
                  <KeygroupBuilder />
                </div>

                {/* Quick-assign from sample list */}
                <div className="mt-3 space-y-1">
                  <div className="flex items-center justify-between">
                    <p className="text-[10px] text-text-muted uppercase tracking-wider">Quick assign</p>
                    <span className="text-[10px] text-text-muted">{samples.length} sample{samples.length !== 1 ? 's' : ''}</span>
                  </div>
                  <div className="max-h-48 overflow-y-auto scrollbar-thin space-y-0.5">
                    {samples.map((s) => (
                      <button
                        key={s.id}
                        onClick={() => {
                          withHistory('Quick assign sample', () => {
                            const { assignSampleToLayer: assign } = usePadStore.getState();
                            assign(
                              activePadIndex,
                              activeLayerIndex,
                              s.id,
                              s.name,
                              s.fileName,
                              s.rootNote
                            );
                          });
                        }}
                        className="w-full flex items-center gap-1.5 text-left px-2 py-1 rounded text-xs text-text-secondary
                          hover:bg-surface-alt hover:text-text-primary transition-colors"
                      >
                        <div className="w-1 h-1 rounded-full bg-accent-teal flex-shrink-0" />
                        <span className="truncate">{s.name}</span>
                      </button>
                    ))}
                  </div>
                </div>
              </>
            )}
          </div>
        )}
      </div>
    </div>
  );
}
