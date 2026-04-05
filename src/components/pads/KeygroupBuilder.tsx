'use client';

import React, { useState, useCallback, useMemo, useRef } from 'react';
import { usePadStore } from '@/stores/padStore';
import { useAudioStore } from '@/stores/audioStore';
import {
  buildKeygroup,
  detectRootNote,
  suggestTargetRoot,
  getStackType,
  STACK_LABELS,
  type KeygroupSource,
  type KeygroupBuilderConfig,
} from '@/lib/audio/keygroupBuilder';
import {
  EXPRESSION_MODES,
  type AutoLayerMode,
  type AutoLayerConfig,
} from '@/lib/audio/autoLayerGenerator';
import { decodeArrayBuffer } from '@/lib/audio/audioUtils';
import { midiNoteToName } from '@/types';
import Slider from '@/components/ui/Slider';
import { useToastStore } from '@/stores/toastStore';
import Button from '@/components/ui/Button';
import { useHistoryStore } from '@/stores/historyStore';

type DualMode = AutoLayerConfig['dualMode'];

const DUAL_MODES: { id: DualMode; label: string; description: string }[] = [
  { id: 'split', label: 'Split', description: 'A→first, B→last' },
  { id: 'interleave', label: 'Weave', description: 'A,B,C,D alternating' },
  { id: 'blend', label: 'Blend', description: 'Weighted random mix' },
];

// Mini keyboard for visualization
const KEYBOARD_OCTAVE_NOTES = [
  { note: 0, isBlack: false, label: 'C' },
  { note: 1, isBlack: true,  label: 'C#' },
  { note: 2, isBlack: false, label: 'D' },
  { note: 3, isBlack: true,  label: 'D#' },
  { note: 4, isBlack: false, label: 'E' },
  { note: 5, isBlack: false, label: 'F' },
  { note: 6, isBlack: true,  label: 'F#' },
  { note: 7, isBlack: false, label: 'G' },
  { note: 8, isBlack: true,  label: 'G#' },
  { note: 9, isBlack: false, label: 'A' },
  { note: 10, isBlack: true, label: 'A#' },
  { note: 11, isBlack: false, label: 'B' },
];

export default function KeygroupBuilder() {
  const activePadIndex = usePadStore((s) => s.activePadIndex);
  const assignSampleToLayer = usePadStore((s) => s.assignSampleToLayer);
  const setPadPlayMode = usePadStore((s) => s.setPadPlayMode);
  const updateLayer = usePadStore((s) => s.updateLayer);
  const samples = useAudioStore((s) => s.samples);
  const addSample = useAudioStore((s) => s.addSample);

  const [isOpen, setIsOpen] = useState(false);
  const [isBuilding, setIsBuilding] = useState(false);
  const [isDecoding, setIsDecoding] = useState(false);
  const [progress, setProgress] = useState(0);
  const [progressMsg, setProgressMsg] = useState('');

  // Source samples (1-4)
  const [sources, setSources] = useState<KeygroupSource[]>([]);
  const [targetRoot, setTargetRoot] = useState(60);
  const [mode, setMode] = useState<AutoLayerMode>('velocity');
  const [variation, setVariation] = useState(0.75);
  const [humanize, setHumanize] = useState(0.4);
  const [dualMode, setDualMode] = useState<DualMode>('interleave');

  const fileInputRef = useRef<HTMLInputElement>(null);

  const stackType = getStackType(sources.length);
  const stackInfo = STACK_LABELS[stackType];
  const activeMode = EXPRESSION_MODES.find((m) => m.id === mode) || EXPRESSION_MODES[0];

  // Handle file drop
  const handleFiles = useCallback(async (files: FileList) => {
    // Read current sources length via functional update to avoid stale closure
    // (sources captured at render time may be outdated after async decoding)
    const toProcess = Array.from(files).slice(0, 4); // decode up to 4, trim later
    if (toProcess.length === 0) return;

    setIsDecoding(true);
    const newSources: KeygroupSource[] = [];

    for (const file of toProcess) {
      try {
        const arrayBuffer = await file.arrayBuffer();
        const decoded = await decodeArrayBuffer(arrayBuffer);
        const wavBuffer = arrayBuffer;
        const rootNote = detectRootNote(file.name);

        const sample: KeygroupSource['sample'] = {
          id: crypto.randomUUID(),
          name: file.name.replace(/\.(wav|mp3|flac|m4a|ogg|aif|aiff)$/i, ''),
          fileName: file.name,
          duration: decoded.duration,
          sampleRate: decoded.sampleRate,
          channels: decoded.numberOfChannels,
          bitDepth: 16,
          buffer: wavBuffer,
          rootNote,
          createdAt: Date.now(),
        };

        newSources.push({
          sample,
          rootNote,
          buffer: decoded,
        });
      } catch (err) {
        console.error(`Failed to decode ${file.name}:`, err);
        useToastStore.getState().addToast({
          type: 'warning',
          title: `Could not decode ${file.name}`,
        });
      }
    }

    if (newSources.length > 0) {
      // Use functional update to read latest sources (avoids stale closure)
      setSources((prev) => {
        const updated = [...prev, ...newSources].slice(0, 4);
        setTargetRoot(suggestTargetRoot(updated));
        return updated;
      });
    }
    setIsDecoding(false);
  }, []);

  const handleDrop = useCallback((e: React.DragEvent) => {
    e.preventDefault();
    e.stopPropagation();
    if (e.dataTransfer.files.length > 0) {
      handleFiles(e.dataTransfer.files);
    }
  }, [handleFiles]);

  const handleDragOver = useCallback((e: React.DragEvent) => {
    e.preventDefault();
    e.stopPropagation();
  }, []);

  const removeSource = useCallback((index: number) => {
    setSources((prev) => {
      const updated = prev.filter((_, i) => i !== index);
      if (updated.length > 0) {
        setTargetRoot(suggestTargetRoot(updated));
      }
      return updated;
    });
  }, []);

  const updateSourceRoot = useCallback((index: number, newRoot: number) => {
    setSources((prev) => {
      const updated = [...prev];
      updated[index] = { ...updated[index], rootNote: newRoot };
      return updated;
    });
  }, []);

  // Build the keygroup
  const handleBuild = useCallback(async () => {
    if (sources.length === 0 || isBuilding) return;

    setIsBuilding(true);
    setProgress(0);
    setProgressMsg('Starting...');

    try {
      const config: KeygroupBuilderConfig = {
        sources,
        targetRoot,
        mode,
        variation,
        humanize,
        dualMode,
        programName: sources[0].sample.name.substring(0, 16),
      };

      const result = await buildKeygroup(config, (step, pct) => {
        setProgressMsg(step);
        setProgress(pct);
      });

      // Add all generated samples to audioStore
      for (const sample of result.samples) {
        addSample(sample);
      }

      // Assign layers to the captured pad (stale closure safety)
      const capturedPadIdx = activePadIndex;
      const store = usePadStore.getState();
      const history = useHistoryStore.getState();
      history.snapshot(usePadStore.getState().pads);

      for (const assignment of result.layerAssignments) {
        for (const layer of assignment.layers) {
          store.assignSampleToLayer(
            capturedPadIdx,
            layer.layerIndex,
            layer.sample.id,
            layer.sample.name,
            layer.sample.fileName,
            layer.sample.rootNote
          );

          store.updateLayer(capturedPadIdx, layer.layerIndex, {
            velStart: layer.velStart,
            velEnd: layer.velEnd,
            keyTrack: true, // Enable keytrack for chromatic playback
          });
        }

        store.setPadPlayMode(capturedPadIdx, assignment.playMode);
      }

      history.pushState('Build keygroup', usePadStore.getState().pads);

      setIsOpen(false);
      setSources([]);
      useToastStore.getState().addToast({
        type: 'success',
        title: 'Keygroup built successfully',
      });
    } catch (error) {
      console.error('Keygroup build failed:', error);
      useToastStore.getState().addToast({
        type: 'error',
        title: 'Keygroup build failed',
        message: String(error),
      });
    } finally {
      setIsBuilding(false);
      setProgress(0);
      setProgressMsg('');
    }
  }, [
    sources, targetRoot, mode, variation, humanize, dualMode,
    activePadIndex, addSample, assignSampleToLayer, setPadPlayMode,
    isBuilding,
  ]);

  // Pitch shift visualization
  const pitchShifts = useMemo(() => {
    return sources.map((s) => ({
      name: s.sample.name,
      from: s.rootNote,
      to: targetRoot,
      semitones: targetRoot - s.rootNote,
    }));
  }, [sources, targetRoot]);

  // Mini keyboard visualization: highlight target root
  const miniKeyboard = useMemo(() => {
    const targetOctave = Math.floor(targetRoot / 12) - 2;
    const targetNoteInOctave = targetRoot % 12;
    return { targetOctave, targetNoteInOctave };
  }, [targetRoot]);

  if (!isOpen) {
    return (
      <button
        onClick={() => setIsOpen(true)}
        className="px-3 py-2 rounded-lg text-xs font-semibold
          bg-gradient-to-r from-accent-teal/10 to-accent-plum/10
          text-text-secondary hover:text-text-primary
          hover:from-accent-teal/20 hover:to-accent-plum/20
          transition-all border border-transparent hover:border-border"
        title="Build a keygroup from 1-4 samples with pitch matching and expression layers"
      >
        🎹 Keygroup Builder
      </button>
    );
  }

  return (
    <div className="space-y-3 p-3 bg-surface-alt rounded-lg border border-border">
      {/* Header */}
      <div className="flex items-center justify-between">
        <div className="flex items-center gap-2">
          <p className="text-[10px] text-text-muted uppercase tracking-wider font-semibold">
            Keygroup Builder
          </p>
          {sources.length > 0 && (
            <span className="px-1.5 py-0.5 rounded text-[9px] font-bold bg-accent-teal/10 text-accent-teal">
              {stackInfo.icon} {stackInfo.label}
            </span>
          )}
        </div>
        <button
          onClick={() => { setIsOpen(false); setSources([]); }}
          className="text-text-muted hover:text-text-primary text-xs"
          aria-label="Close Keygroup Builder"
        >
          ✕
        </button>
      </div>

      {/* Drop Zone */}
      <div
        onDrop={handleDrop}
        onDragOver={handleDragOver}
        onClick={() => sources.length < 4 && fileInputRef.current?.click()}
        className={`border-2 border-dashed rounded-lg p-4 text-center cursor-pointer
          transition-all hover:border-accent-teal/50 hover:bg-accent-teal/5
          ${sources.length >= 4
            ? 'border-border/50 opacity-50 cursor-not-allowed'
            : 'border-border'
          }`}
      >
        <input
          ref={fileInputRef}
          type="file"
          accept=".wav,.mp3,.flac,.m4a,.ogg,.aif,.aiff"
          multiple
          className="hidden"
          onChange={(e) => e.target.files && handleFiles(e.target.files)}
        />
        <p className="text-[10px] text-text-muted">
          {isDecoding
            ? 'Decoding audio…'
            : sources.length >= 4
              ? 'Maximum 4 sources reached'
              : sources.length === 0
                ? 'Drop up to 4 samples here'
                : `Drop up to ${4 - sources.length} more (${sources.length}/4 loaded)`
          }
        </p>
        <p className="text-[8px] text-text-muted mt-1">
          WAV, MP3, FLAC, OGG, AIF
        </p>
      </div>

      {/* Source List with Root Note Detection */}
      {sources.length > 0 && (
        <div className="space-y-1.5">
          <p className="text-[10px] text-text-muted uppercase tracking-wider">
            Sources — {stackInfo.description}
          </p>
          {sources.map((src, i) => {
            const shift = targetRoot - src.rootNote;
            return (
              <div
                key={src.sample.id}
                className="flex items-center gap-2 px-2 py-1.5 rounded bg-surface"
              >
                <span className="text-accent-teal font-bold text-[10px] w-3">
                  {String.fromCharCode(65 + i)}
                </span>
                <span className="text-text-primary text-[10px] truncate flex-1">
                  {src.sample.name}
                </span>

                {/* Root note selector */}
                <div className="flex items-center gap-1">
                  <button
                    onClick={() => updateSourceRoot(i, Math.max(0, src.rootNote - 1))}
                    className="text-[9px] text-text-muted hover:text-text-primary px-0.5"
                  >
                    ◀
                  </button>
                  <span className="text-[9px] font-mono text-accent-plum min-w-[28px] text-center">
                    {midiNoteToName(src.rootNote)}
                  </span>
                  <button
                    onClick={() => updateSourceRoot(i, Math.min(127, src.rootNote + 1))}
                    className="text-[9px] text-text-muted hover:text-text-primary px-0.5"
                  >
                    ▶
                  </button>
                </div>

                {/* Pitch shift indicator */}
                {shift !== 0 && (
                  <span className={`text-[8px] font-mono ${shift > 0 ? 'text-accent-teal' : 'text-accent-coral'}`}>
                    {shift > 0 ? '+' : ''}{shift}st
                  </span>
                )}

                <button
                  onClick={() => removeSource(i)}
                  className="text-[9px] text-text-muted hover:text-accent-coral ml-1"
                >
                  ✕
                </button>
              </div>
            );
          })}
        </div>
      )}

      {/* Target Root Note */}
      {sources.length > 0 && (
        <div className="space-y-1.5">
          <p className="text-[10px] text-text-muted uppercase tracking-wider">
            Target Root — All sources pitch-matched here
          </p>
          <div className="flex items-center gap-2">
            <button
              onClick={() => setTargetRoot(Math.max(0, targetRoot - 12))}
              className="px-1.5 py-0.5 rounded text-[9px] bg-surface text-text-muted hover:text-text-primary"
            >
              -Oct
            </button>
            <button
              onClick={() => setTargetRoot(Math.max(0, targetRoot - 1))}
              className="px-1.5 py-0.5 rounded text-[9px] bg-surface text-text-muted hover:text-text-primary"
            >
              -1
            </button>
            <div className="flex-1 text-center">
              <span className="text-sm font-bold text-accent-teal">
                {midiNoteToName(targetRoot)}
              </span>
              <span className="text-[8px] text-text-muted ml-1">
                (MIDI {targetRoot})
              </span>
            </div>
            <button
              onClick={() => setTargetRoot(Math.min(127, targetRoot + 1))}
              className="px-1.5 py-0.5 rounded text-[9px] bg-surface text-text-muted hover:text-text-primary"
            >
              +1
            </button>
            <button
              onClick={() => setTargetRoot(Math.min(127, targetRoot + 12))}
              className="px-1.5 py-0.5 rounded text-[9px] bg-surface text-text-muted hover:text-text-primary"
            >
              +Oct
            </button>
          </div>

          {/* Mini Keyboard Visualization */}
          <div className="flex gap-px h-8 px-1">
            {Array.from({ length: 24 }, (_, i) => {
              const octaveStart = Math.max(0, targetRoot - 12);
              const midiNote = octaveStart + i;
              if (midiNote > 127) return null;
              const noteInOctave = midiNote % 12;
              const noteInfo = KEYBOARD_OCTAVE_NOTES[noteInOctave];
              const isTarget = midiNote === targetRoot;
              const isSourceRoot = sources.some((s) => s.rootNote === midiNote);

              if (noteInfo.isBlack) {
                return (
                  <div
                    key={i}
                    className={`w-2.5 h-5 rounded-b-sm -mx-1 z-10 transition-colors
                      ${isTarget
                        ? 'bg-accent-teal'
                        : isSourceRoot
                          ? 'bg-accent-plum'
                          : 'bg-gray-800'
                      }`}
                    title={midiNoteToName(midiNote)}
                  />
                );
              }

              return (
                <div
                  key={i}
                  className={`flex-1 rounded-b-sm transition-colors flex items-end justify-center pb-0.5
                    ${isTarget
                      ? 'bg-accent-teal/30 border border-accent-teal'
                      : isSourceRoot
                        ? 'bg-accent-plum/20 border border-accent-plum/40'
                        : 'bg-surface border border-border/50'
                    }`}
                  title={midiNoteToName(midiNote)}
                >
                  {(isTarget || isSourceRoot) && (
                    <span className="text-[5px] font-mono text-text-muted">
                      {midiNoteToName(midiNote)}
                    </span>
                  )}
                </div>
              );
            })}
          </div>
          <div className="flex justify-between text-[7px] text-text-muted px-1">
            <span>🟢 target root</span>
            <span>🟣 source roots</span>
          </div>
        </div>
      )}

      {/* Expression Mode Selector */}
      {sources.length > 0 && (
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
      )}

      {/* Variation + Humanize */}
      {sources.length > 0 && (
        <>
          <Slider label="Variation" value={variation} onChange={setVariation} min={0} max={1} step={0.05} />
          <Slider label="Humanize" value={humanize} onChange={setHumanize} min={0} max={1} step={0.05} />
        </>
      )}

      {/* Multi-source distribution mode */}
      {sources.length >= 2 && (
        <div className="space-y-1.5">
          <p className="text-[10px] text-text-muted uppercase tracking-wider">
            Source Distribution ({sources.length} voices)
          </p>
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

      {/* Layer Preview */}
      {sources.length > 0 && (
        <div className="space-y-1">
          <p className="text-[10px] text-text-muted uppercase tracking-wider">
            8-Layer Preview — {activeMode.playMode === 'cycle' ? 'Cycle Mode' : 'Velocity Mode'}
          </p>
          <div className="flex gap-0.5 h-8">
            {activeMode.labels.map((label, i) => {
              const opacity = activeMode.playMode === 'cycle'
                ? 0.6 + Math.sin(i * 0.8) * 0.15
                : 0.15 + (i / 7) * 0.85;

              // Color by source assignment
              const numSrc = sources.length;
              let sourceIdx = 0;
              if (numSrc >= 2) {
                if (dualMode === 'interleave') sourceIdx = i % numSrc;
                else if (dualMode === 'split') {
                  if (numSrc === 2) sourceIdx = i < 4 ? 0 : 1;
                  else if (numSrc === 3) sourceIdx = i < 3 ? 0 : i < 6 ? 1 : 2;
                  else sourceIdx = i < 2 ? 0 : i < 4 ? 1 : i < 6 ? 2 : 3;
                }
                else sourceIdx = i % numSrc;
              }

              const colors = [
                'var(--color-accent-teal)',
                'var(--color-accent-plum)',
                'var(--color-accent-coral)',
                'var(--color-accent-gold)',
              ];

              return (
                <div
                  key={i}
                  className="flex-1 rounded-sm flex items-end justify-center pb-0.5"
                  style={{
                    backgroundColor: `rgb(${colors[sourceIdx]} / ${opacity})`,
                  }}
                >
                  <span className="text-[6px] font-mono text-text-muted">{label}</span>
                </div>
              );
            })}
          </div>
          <div className="flex justify-between text-[8px] text-text-muted px-0.5">
            {activeMode.playMode === 'cycle' ? (
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
      )}

      {/* Pitch Match Summary */}
      {sources.length > 1 && pitchShifts.some((p) => p.semitones !== 0) && (
        <div className="space-y-1 p-2 rounded bg-surface text-[9px]">
          <p className="text-text-muted uppercase tracking-wider font-semibold">Pitch Matching</p>
          {pitchShifts.map((ps, i) => (
            <div key={i} className="flex items-center gap-1.5">
              <span className="text-accent-teal font-bold">{String.fromCharCode(65 + i)}</span>
              <span className="text-text-secondary truncate flex-1">{ps.name}</span>
              <span className="font-mono text-text-muted">
                {midiNoteToName(ps.from)}
              </span>
              <span className="text-text-muted">→</span>
              <span className="font-mono text-accent-teal">
                {midiNoteToName(ps.to)}
              </span>
              {ps.semitones !== 0 && (
                <span className={`font-mono ${ps.semitones > 0 ? 'text-accent-teal' : 'text-accent-coral'}`}>
                  ({ps.semitones > 0 ? '+' : ''}{ps.semitones})
                </span>
              )}
            </div>
          ))}
        </div>
      )}

      {/* Build Button */}
      {sources.length > 0 && (
        <div className="flex gap-1 pt-1">
          <Button
            variant="primary"
            size="sm"
            disabled={isBuilding}
            onClick={handleBuild}
          >
            {isBuilding
              ? `Building... (${Math.round(progress)}%)`
              : `Build ${stackInfo.label} Keygroup`
            }
          </Button>
          <Button
            variant="ghost"
            size="sm"
            onClick={() => { setIsOpen(false); setSources([]); }}
            disabled={isBuilding}
          >
            Cancel
          </Button>
        </div>
      )}

      {/* Progress bar */}
      {isBuilding && (
        <div className="space-y-1">
          <div className="h-1.5 bg-surface rounded-full overflow-hidden">
            <div
              className="h-full bg-gradient-to-r from-accent-teal to-accent-plum transition-all duration-300"
              style={{ width: `${progress}%` }}
            />
          </div>
          <p className="text-[8px] text-text-muted text-center">{progressMsg}</p>
        </div>
      )}
    </div>
  );
}
