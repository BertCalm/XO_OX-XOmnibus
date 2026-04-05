'use client';

import React, { useState, useCallback, useRef } from 'react';
import { parseXpmXml, analyzeKit } from '@/lib/xpm/xpmParser';
import { extractXpn, extractSingleXpm } from '@/lib/xpn/xpnExtractor';
import { parsedKitToPads, upcycleKit, UPCYCLE_PRESETS } from '@/lib/xpm/kitUpcycler';
import type { UpcycleOption, UpcycleConfig, ParsedKitResult } from '@/lib/xpm/kitUpcycler';
import type { ParsedXpmKit, KitAnalysis } from '@/lib/xpm/xpmParser';
import type { ExtractedExpansion } from '@/lib/xpn/xpnExtractor';
import type { PadAssignment, AudioSample } from '@/types';
import type { PadEnvelopeSettings } from '@/stores/envelopeStore';
import type { XpmModulationConfig } from '@/lib/xpm/xpmTypes';
import { usePadStore } from '@/stores/padStore';
import { useEnvelopeStore } from '@/stores/envelopeStore';
import { useModulationStore } from '@/stores/modulationStore';
import { useExpressionStore } from '@/stores/expressionStore';
import { useAudioStore } from '@/stores/audioStore';
import { useToastStore } from '@/stores/toastStore';
import { useHistoryStore } from '@/stores/historyStore';
import { decodeArrayBuffer, generateWaveformPeaks } from '@/lib/audio/audioUtils';
import { encodeWavAsync } from '@/lib/audio/wavEncoder';
import { v4 as uuid } from 'uuid';
import Card, { CardHeader, CardTitle } from '@/components/ui/Card';
import Button from '@/components/ui/Button';
import Slider from '@/components/ui/Slider';
import ProgressBar from '@/components/ui/ProgressBar';

// ---------------------------------------------------------------------------
// Types
// ---------------------------------------------------------------------------

type ImportStage = 'drop' | 'analyzing' | 'analysis' | 'upcycling' | 'results';

interface ImportedProgram {
  name: string;
  kit: ParsedXpmKit;
  analysis: KitAnalysis;
  pads: PadAssignment[];
  /** Envelope/filter settings extracted from the kit (no longer discarded). */
  envelopes: Record<number, PadEnvelopeSettings>;
  /** Modulation configs extracted from the kit (no longer discarded). */
  modulations: Record<number, XpmModulationConfig>;
}

// ---------------------------------------------------------------------------
// All individual upcycle options for custom selection
// ---------------------------------------------------------------------------

const ALL_OPTIONS: { id: UpcycleOption; label: string; description: string }[] = [
  { id: 'auto-ghost', label: 'Auto Ghost', description: 'Generate ghost note layers for dynamics' },
  { id: 'expression-inject', label: 'Expression Inject', description: 'Add velocity-sensitive dynamics' },
  { id: 'humanize', label: 'Humanize', description: 'Add timing/pitch/volume randomization' },
  { id: 'velocity-split', label: 'Velocity Split', description: 'Create velocity layers from single layers' },
  { id: 'tail-tame', label: 'Tail Tame', description: 'Flag for anti-click fade-out processing' },
  { id: 'stereo-widen', label: 'Stereo Widen', description: 'Apply Space Fold stereo imaging' },
];

// ---------------------------------------------------------------------------
// Component
// ---------------------------------------------------------------------------

export default function KitImporter() {
  const [stage, setStage] = useState<ImportStage>('drop');
  const [isDragOver, setIsDragOver] = useState(false);
  const [error, setError] = useState<string | null>(null);
  const [progressLabel, setProgressLabel] = useState('');
  const [progressPct, setProgressPct] = useState(0);

  // Import state
  const [expansion, setExpansion] = useState<ExtractedExpansion | null>(null);
  const [programs, setPrograms] = useState<ImportedProgram[]>([]);
  const [selectedProgramIdx, setSelectedProgramIdx] = useState(0);

  // Upcycle config
  const [selectedOptions, setSelectedOptions] = useState<Set<UpcycleOption>>(new Set());
  const [intensity, setIntensity] = useState(0.5);
  const [upcycleChanges, setUpcycleChanges] = useState<{ padIndex: number; description: string }[]>([]);

  // Loose audio files dropped alongside an XPM (folder drop scenario)
  const [looseAudioFiles, setLooseAudioFiles] = useState<Map<string, ArrayBuffer>>(new Map());

  const fileInputRef = useRef<HTMLInputElement>(null);

  // -------------------------------------------------------------------------
  // File handling
  // -------------------------------------------------------------------------

  const processFiles = useCallback(async (files: File[]) => {
    setError(null);
    setStage('analyzing');
    setProgressPct(0);

    try {
      // Separate files by type
      const audioExts = ['.wav', '.aif', '.aiff', '.mp3', '.flac', '.m4a', '.ogg'];
      const xpmFiles: File[] = [];
      const xpnFiles: File[] = [];
      const audioFiles: File[] = [];

      for (const file of files) {
        const ext = '.' + (file.name.toLowerCase().split('.').pop() ?? '');
        if (ext === '.xpn') xpnFiles.push(file);
        else if (ext === '.xpm') xpmFiles.push(file);
        else if (audioExts.includes(ext)) audioFiles.push(file);
      }

      // Collect loose audio files into a Map<filename, ArrayBuffer>
      const looseAudio = new Map<string, ArrayBuffer>();
      for (const audioFile of audioFiles) {
        setProgressLabel(`Reading ${audioFile.name}...`);
        const buf = await audioFile.arrayBuffer();
        looseAudio.set(audioFile.name, buf);
      }
      setLooseAudioFiles(looseAudio);

      if (xpnFiles.length > 0) {
        // XPN expansion file — process the first one
        const file = xpnFiles[0];
        const buffer = await file.arrayBuffer();

        const extracted = await extractXpn(buffer, (step, pct) => {
          setProgressLabel(step);
          setProgressPct(pct);
        });

        setExpansion(extracted);

        const importedPrograms: ImportedProgram[] = extracted.programs.map((prog) => {
          const analysis = analyzeKit(prog.kit);
          const result = parsedKitToPads(prog.kit);
          return {
            name: prog.name,
            kit: prog.kit,
            analysis,
            pads: result.pads,
            envelopes: result.envelopes,
            modulations: result.modulations,
          };
        });

        if (importedPrograms.length === 0) {
          throw new Error('No valid programs found in XPN file');
        }

        setPrograms(importedPrograms);
        setSelectedProgramIdx(0);
        setStage('analysis');
      } else if (xpmFiles.length > 0) {
        // Single XPM program (+ optional loose audio files)
        const file = xpmFiles[0];
        const buffer = await file.arrayBuffer();
        const { kit } = await extractSingleXpm(buffer);
        const analysis = analyzeKit(kit);
        const result = parsedKitToPads(kit);

        setExpansion(null);
        setPrograms([{
          name: kit.name,
          kit,
          analysis,
          pads: result.pads,
          envelopes: result.envelopes,
          modulations: result.modulations,
        }]);
        setSelectedProgramIdx(0);
        setStage('analysis');

        if (looseAudio.size > 0) {
          useToastStore.getState().addToast({
            type: 'info',
            title: `${looseAudio.size} audio file${looseAudio.size !== 1 ? 's' : ''} detected`,
            message: 'They will be linked to matching pads when you load the kit.',
          });
        }
      } else if (audioFiles.length > 0) {
        throw new Error('Please include an .xpm or .xpn file alongside your audio files.');
      } else {
        throw new Error('No .xpm or .xpn files found. Please drop a kit file.');
      }
    } catch (err) {
      setError(err instanceof Error ? err.message : 'Failed to import file');
      setStage('drop');
    }
  }, []);

  const handleDrop = useCallback(
    (e: React.DragEvent) => {
      e.preventDefault();
      setIsDragOver(false);

      const files = Array.from(e.dataTransfer.files);
      if (files.length > 0) processFiles(files);
    },
    [processFiles],
  );

  const handleFileSelect = useCallback(
    (e: React.ChangeEvent<HTMLInputElement>) => {
      const files = Array.from(e.target.files ?? []);
      if (files.length > 0) processFiles(files);
      // Reset input so same file can be re-selected
      if (e.target) e.target.value = '';
    },
    [processFiles],
  );

  // -------------------------------------------------------------------------
  // Upcycle
  // -------------------------------------------------------------------------

  const applyPreset = useCallback((presetId: string) => {
    const preset = UPCYCLE_PRESETS.find((p) => p.id === presetId);
    if (!preset) return;

    setSelectedOptions(new Set(preset.options));
    setIntensity(preset.intensity);
  }, []);

  const toggleOption = useCallback((option: UpcycleOption) => {
    setSelectedOptions((prev) => {
      const next = new Set(prev);
      if (next.has(option)) {
        next.delete(option);
      } else {
        next.add(option);
      }
      return next;
    });
  }, []);

  const runUpcycle = useCallback(() => {
    if (programs.length === 0) return;

    const program = programs[selectedProgramIdx];
    const config: UpcycleConfig = {
      options: Array.from(selectedOptions),
      intensity,
    };

    setStage('upcycling');
    setProgressLabel('Applying transformations...');
    setProgressPct(50);

    // Use requestAnimationFrame to allow UI to update before heavy computation
    requestAnimationFrame(() => {
      try {
        const result = upcycleKit(program.pads, program.analysis, config);

        setUpcycleChanges(result.changes);
        setStage('results');

        // Store the upcycled pads for loading
        setPrograms((prev) => {
          const updated = [...prev];
          updated[selectedProgramIdx] = {
            ...updated[selectedProgramIdx],
            pads: result.pads,
          };
          return updated;
        });
      } catch (err) {
        setError(err instanceof Error ? err.message : 'Upcycle failed');
        setStage('analysis');
      }
    });
  }, [programs, selectedProgramIdx, selectedOptions, intensity]);

  const loadIntoPadStore = useCallback(async () => {
    if (programs.length === 0) return;

    const program = programs[selectedProgramIdx];
    const audioStore = useAudioStore.getState();
    const toast = useToastStore.getState();

    // ------------------------------------------------------------------
    // 1. Import audio samples and build filename → sampleId lookup
    // ------------------------------------------------------------------
    const fileToSampleId = new Map<string, string>();

    // Determine available raw sample buffers (XPN extraction or loose files)
    const rawSamples: Map<string, ArrayBuffer> =
      expansion?.samples ?? looseAudioFiles;

    if (rawSamples.size > 0) {
      // Decode each raw audio buffer, convert to WAV, add to audioStore
      setStage('analyzing');
      setProgressLabel('Importing samples...');
      setProgressPct(0);

      const entries = Array.from(rawSamples.entries());
      for (let i = 0; i < entries.length; i++) {
        const [fileName, rawBuffer] = entries[i];
        setProgressLabel(`Decoding ${fileName}...`);
        setProgressPct(Math.round(((i + 1) / entries.length) * 90));

        try {
          const audioBuffer = await decodeArrayBuffer(rawBuffer);
          const wavData = await encodeWavAsync(audioBuffer, 16);
          const peaks = generateWaveformPeaks(audioBuffer);
          const baseName = fileName.replace(/\.[^.]+$/, '');
          const sampleId = uuid();

          const sample: AudioSample = {
            id: sampleId,
            name: baseName,
            fileName,
            duration: audioBuffer.duration,
            sampleRate: audioBuffer.sampleRate,
            channels: audioBuffer.numberOfChannels,
            bitDepth: 16,
            buffer: wavData,
            waveformPeaks: peaks,
            rootNote: 60,
            createdAt: Date.now(),
          };

          audioStore.addSample(sample);
          // Map both exact filename and lowercase for case-insensitive matching
          fileToSampleId.set(fileName, sampleId);
          fileToSampleId.set(fileName.toLowerCase(), sampleId);
        } catch (err) {
          console.warn(`[KitImporter] Failed to decode sample: ${fileName}`, err);
          toast.addToast({
            type: 'warning',
            title: `Skipped ${fileName}`,
            message: err instanceof Error ? err.message : 'Could not decode audio',
          });
        }
      }
    }

    // Fall back to matching against samples already loaded in audioStore
    if (fileToSampleId.size === 0) {
      const existing = audioStore.samples;
      for (const s of existing) {
        fileToSampleId.set(s.fileName, s.id);
        fileToSampleId.set(s.fileName.toLowerCase(), s.id);
        // Also index by name (without extension) for fuzzy matching
        const baseName = s.fileName.replace(/\.[^.]+$/, '');
        fileToSampleId.set(baseName.toLowerCase(), s.id);
      }
    }

    // ------------------------------------------------------------------
    // 2. Resolve sampleId on each pad layer
    // ------------------------------------------------------------------
    const resolvedPads = program.pads.map((pad) => ({
      ...pad,
      layers: pad.layers.map((layer) => {
        if (!layer.active || (!layer.sampleFile && !layer.sampleName)) {
          return layer;
        }
        // Try matching by sampleFile first, then sampleName
        const candidates = [
          layer.sampleFile,
          layer.sampleFile?.toLowerCase(),
          layer.sampleName,
          layer.sampleName?.toLowerCase(),
          // Try without extension
          layer.sampleFile?.replace(/\.[^.]+$/, '')?.toLowerCase(),
          layer.sampleName?.replace(/\.[^.]+$/, '')?.toLowerCase(),
        ].filter(Boolean) as string[];

        for (const candidate of candidates) {
          const matchedId = fileToSampleId.get(candidate);
          if (matchedId) {
            return { ...layer, sampleId: matchedId };
          }
        }
        return layer; // No match — sampleId stays null
      }),
    }));

    // ------------------------------------------------------------------
    // 3. Load resolved pads into padStore
    // ------------------------------------------------------------------
    setProgressLabel('Loading pads...');
    setProgressPct(95);

    // Clear stale per-pad settings from previously loaded kits to prevent
    // cross-kit contamination (pads not present in new kit would retain old settings)
    useExpressionStore.getState().clearAll();
    useEnvelopeStore.getState().clearAll();
    useModulationStore.getState().clearAll();

    const history = useHistoryStore.getState();
    history.snapshot(usePadStore.getState().pads);

    const padCount = Math.max(16, resolvedPads.length);
    const padStoreActions = usePadStore.getState();
    padStoreActions.initializePads(padCount);

    for (let i = 0; i < resolvedPads.length; i++) {
      padStoreActions.updatePad(i, resolvedPads[i]);
    }

    history.pushState('Import kit', usePadStore.getState().pads);

    // Populate envelope store with extracted envelope/filter settings
    const envelopeStore = useEnvelopeStore.getState();
    for (const [padIdxStr, envSettings] of Object.entries(program.envelopes)) {
      const padIdx = parseInt(padIdxStr, 10);
      envelopeStore.setVolumeEnvelope(padIdx, envSettings.volumeEnvelope);
      envelopeStore.setFilterEnvelope(padIdx, envSettings.filterEnvelope);
      envelopeStore.setFilterSettings(padIdx, {
        filterType: envSettings.filterType,
        filterCutoff: envSettings.filterCutoff,
        filterResonance: envSettings.filterResonance,
        filterEnvAmount: envSettings.filterEnvAmount,
      });
    }

    // Populate modulation store with extracted modulation configs
    const modulationStore = useModulationStore.getState();
    if (Object.keys(program.modulations).length > 0) {
      modulationStore.bulkSetModulation(program.modulations);
    }

    // ------------------------------------------------------------------
    // 4. Report results
    // ------------------------------------------------------------------
    const resolvedCount = resolvedPads.reduce(
      (sum, pad) => sum + pad.layers.filter((l) => l.active && l.sampleId).length,
      0,
    );
    const activeCount = resolvedPads.reduce(
      (sum, pad) => sum + pad.layers.filter((l) => l.active && (l.sampleFile || l.sampleName)).length,
      0,
    );
    const unresolvedCount = activeCount - resolvedCount;

    if (unresolvedCount > 0) {
      toast.addToast({
        type: 'warning',
        title: `${unresolvedCount} sample${unresolvedCount !== 1 ? 's' : ''} not found`,
        message: 'Import the missing audio files into The Crate, then re-import the kit.',
      });
    }

    if (resolvedCount > 0 || rawSamples.size > 0) {
      toast.addToast({
        type: 'success',
        title: 'Kit loaded',
        message: `${resolvedCount} sample${resolvedCount !== 1 ? 's' : ''} linked to ${resolvedPads.length} pads`,
      });
    }

    // Reset to drop stage
    setStage('drop');
    setPrograms([]);
    setExpansion(null);
    setLooseAudioFiles(new Map());
    setUpcycleChanges([]);
    setSelectedOptions(new Set());
    setIntensity(0.5);
  }, [programs, selectedProgramIdx, expansion, looseAudioFiles]);

  const loadWithoutUpcycle = useCallback(async () => {
    try {
      await loadIntoPadStore();
    } catch (err) {
      setError(err instanceof Error ? err.message : 'Failed to load kit');
      setStage('drop');
    }
  }, [loadIntoPadStore]);

  const loadUpcycledKit = useCallback(async () => {
    try {
      await loadIntoPadStore();
    } catch (err) {
      setError(err instanceof Error ? err.message : 'Failed to load upcycled kit');
      setStage('drop');
    }
  }, [loadIntoPadStore]);

  const resetImporter = useCallback(() => {
    setStage('drop');
    setPrograms([]);
    setExpansion(null);
    setLooseAudioFiles(new Map());
    setError(null);
    setUpcycleChanges([]);
    setSelectedOptions(new Set());
    setIntensity(0.5);
  }, []);

  // -------------------------------------------------------------------------
  // Derived state
  // -------------------------------------------------------------------------

  const currentProgram = programs[selectedProgramIdx] ?? null;
  const currentAnalysis = currentProgram?.analysis ?? null;

  // -------------------------------------------------------------------------
  // Render helpers
  // -------------------------------------------------------------------------

  const renderDropZone = () => (
    <div
      role="button"
      tabIndex={0}
      aria-label="Import kit file — drop .xpm or .xpn files here, or click to browse"
      onDragOver={(e) => {
        e.preventDefault();
        setIsDragOver(true);
      }}
      onDragLeave={() => setIsDragOver(false)}
      onDrop={handleDrop}
      onClick={() => fileInputRef.current?.click()}
      onKeyDown={(e) => {
        if (e.key === 'Enter' || e.key === ' ') {
          e.preventDefault();
          fileInputRef.current?.click();
        }
      }}
      className={`relative flex flex-col items-center justify-center gap-4 p-12
        border-2 border-dashed rounded-xl cursor-pointer transition-all duration-200
        focus:outline-none focus:ring-2 focus:ring-accent-teal focus:ring-offset-2 focus:ring-offset-surface-bg
        ${isDragOver
          ? 'border-accent-teal bg-accent-teal/5 scale-[1.01]'
          : 'border-border hover:border-accent-teal/50 hover:bg-surface-alt/50'
        }`}
    >
      <input
        ref={fileInputRef}
        type="file"
        accept=".xpm,.xpn,.wav,.aif,.aiff,.mp3,.flac"
        multiple
        onChange={handleFileSelect}
        className="hidden"
      />

      {/* Upload icon */}
      <div className={`w-16 h-16 rounded-full flex items-center justify-center transition-colors
        ${isDragOver ? 'bg-accent-teal/10' : 'bg-surface-alt'}`}
      >
        <svg width="32" height="32" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="1.5" strokeLinecap="round" strokeLinejoin="round" className="text-text-secondary">
          <path d="M21 15v4a2 2 0 01-2 2H5a2 2 0 01-2-2v-4" />
          <polyline points="17 8 12 3 7 8" />
          <line x1="12" y1="3" x2="12" y2="15" />
        </svg>
      </div>

      <div className="text-center">
        <p className="text-sm font-semibold text-text-primary">Import Kit for Upcycling</p>
        <p className="text-xs text-text-secondary mt-1">
          Drop .xpm/.xpn files here, or a folder with .xpm + samples
        </p>
      </div>

      <div className="flex gap-2">
        <span className="px-2 py-0.5 text-[10px] font-mono bg-surface-alt rounded text-text-muted">.xpn</span>
        <span className="px-2 py-0.5 text-[10px] font-mono bg-surface-alt rounded text-text-muted">.xpm</span>
        <span className="px-2 py-0.5 text-[10px] font-mono bg-surface-alt rounded text-text-muted">.wav</span>
      </div>
    </div>
  );

  const renderAnalyzing = () => (
    <div className="flex flex-col items-center gap-4 py-8">
      <div className="w-10 h-10 rounded-full progress-gradient flex items-center justify-center">
        <div className="w-4 h-4 rounded-full bg-white animate-pulse" />
      </div>
      <p className="text-sm font-medium text-text-primary">Analyzing kit...</p>
      <ProgressBar
        progress={progressPct}
        detail={progressLabel}
        size="md"
        className="max-w-xs"
      />
    </div>
  );

  const renderHealthMeter = (score: number) => {
    const getColor = () => {
      if (score >= 70) return 'bg-accent-teal';
      if (score >= 40) return 'bg-yellow-500';
      return 'bg-orange-500';
    };

    const getLabel = () => {
      if (score >= 70) return 'Expressive';
      if (score >= 40) return 'Moderate';
      return 'Basic';
    };

    return (
      <div className="space-y-1.5">
        <div className="flex items-center justify-between">
          <span className="text-xs font-medium text-text-secondary">Expressiveness</span>
          <span className="text-xs font-mono text-text-muted">{score}% - {getLabel()}</span>
        </div>
        <div className="w-full h-2.5 bg-surface-alt rounded-full overflow-hidden">
          <div
            className={`h-full rounded-full transition-all duration-700 ease-out ${getColor()}`}
            style={{ width: `${score}%` }}
          />
        </div>
      </div>
    );
  };

  const renderAnalysis = () => {
    if (!currentProgram || !currentAnalysis) return null;

    return (
      <div className="space-y-5">
        {/* Header with kit info */}
        <div className="flex items-start justify-between">
          <div>
            <h4 className="text-base font-semibold text-text-primary">{currentProgram.name}</h4>
            <div className="flex items-center gap-2 mt-1">
              <span className="px-2 py-0.5 text-[10px] font-mono bg-accent-teal/10 text-accent-teal rounded">
                {currentProgram.kit.type}
              </span>
              <span className="text-xs text-text-muted">
                {currentAnalysis.padsWithSamples}/{currentAnalysis.totalPads} pads loaded
              </span>
              {currentAnalysis.maxLayersUsed > 1 && (
                <span className="text-xs text-text-muted">
                  Max {currentAnalysis.maxLayersUsed} layers
                </span>
              )}
            </div>
          </div>
          <Button variant="ghost" size="sm" onClick={resetImporter}>
            Cancel
          </Button>
        </div>

        {/* Program selector (for XPN with multiple programs) */}
        {programs.length > 1 && (
          <div>
            <label className="text-xs font-medium text-text-secondary mb-1.5 block">
              Program ({programs.length} found)
            </label>
            <select
              value={selectedProgramIdx}
              onChange={(e) => setSelectedProgramIdx(parseInt(e.target.value, 10))}
              className="w-full px-3 py-2 text-sm bg-surface-alt border border-border rounded-lg
                text-text-primary focus:outline-none focus:ring-1 focus:ring-accent-teal"
            >
              {programs.map((prog, i) => (
                <option key={i} value={i}>
                  {prog.name} ({prog.analysis.padsWithSamples} pads)
                </option>
              ))}
            </select>
          </div>
        )}

        {/* Expansion metadata */}
        {expansion && (
          <div className="px-3 py-2 bg-surface-alt/50 rounded-lg">
            <p className="text-xs text-text-secondary">
              <span className="font-medium">Expansion:</span> {expansion.metadata.title}
              {expansion.metadata.manufacturer && ` by ${expansion.metadata.manufacturer}`}
            </p>
            {expansion.metadata.description && (
              <p className="text-xs text-text-muted mt-0.5 line-clamp-2">
                {expansion.metadata.description}
              </p>
            )}
          </div>
        )}

        {/* Health meter */}
        {renderHealthMeter(currentAnalysis.healthScore)}

        {/* Feature badges */}
        <div className="flex flex-wrap gap-1.5">
          <FeatureBadge active={currentAnalysis.hasVelocitySplits} label="Velocity Splits" />
          <FeatureBadge active={currentAnalysis.hasModulation} label="Modulation" />
          <FeatureBadge active={currentAnalysis.hasEnvelopeCustomization} label="Custom Envelopes" />
          <FeatureBadge active={currentAnalysis.hasFilterSettings} label="Filters" />
          <FeatureBadge active={currentAnalysis.hasPitchRandomization || currentAnalysis.hasVolumeRandomization} label="Humanization" />
          <FeatureBadge active={currentAnalysis.maxLayersUsed > 1} label="Multi-Layer" />
        </div>

        {/* Suggestions */}
        {currentAnalysis.suggestions.length > 0 && (
          <div className="space-y-1.5">
            <p className="text-xs font-medium text-text-secondary">Suggestions</p>
            <ul className="space-y-1">
              {currentAnalysis.suggestions.map((suggestion, i) => (
                <li key={i} className="flex items-start gap-2 text-xs text-text-muted">
                  <span className="text-accent-teal mt-px flex-shrink-0">
                    <svg width="12" height="12" viewBox="0 0 12 12" fill="none">
                      <path d="M2 6h8M6 2l4 4-4 4" stroke="currentColor" strokeWidth="1.5" strokeLinecap="round" strokeLinejoin="round" />
                    </svg>
                  </span>
                  {suggestion}
                </li>
              ))}
            </ul>
          </div>
        )}

        <div className="border-t border-border pt-4" />

        {/* Upcycle Presets */}
        <div>
          <p className="text-xs font-medium text-text-secondary mb-2">Upcycle Presets</p>
          <div className="grid grid-cols-3 gap-2">
            {UPCYCLE_PRESETS.map((preset) => {
              const isSelected =
                preset.options.length === selectedOptions.size &&
                preset.options.every((o) => selectedOptions.has(o)) &&
                Math.abs(intensity - preset.intensity) < 0.01;

              return (
                <button
                  key={preset.id}
                  onClick={() => applyPreset(preset.id)}
                  className={`flex flex-col items-center gap-1 p-3 rounded-lg border transition-all
                    ${isSelected
                      ? 'border-accent-teal bg-accent-teal/5 ring-1 ring-accent-teal/20'
                      : 'border-border hover:border-accent-teal/30 hover:bg-surface-alt/50'
                    }`}
                >
                  <span className="text-lg">{preset.icon}</span>
                  <span className="text-xs font-medium text-text-primary">{preset.name}</span>
                  <span className="text-[10px] text-text-muted text-center leading-tight">
                    {preset.description}
                  </span>
                </button>
              );
            })}
          </div>
        </div>

        {/* Custom Options */}
        <div>
          <p className="text-xs font-medium text-text-secondary mb-2">Custom Options</p>
          <div className="grid grid-cols-2 gap-1.5">
            {ALL_OPTIONS.map((option) => (
              <label
                key={option.id}
                className={`flex items-start gap-2 p-2 rounded-lg cursor-pointer transition-colors
                  ${selectedOptions.has(option.id)
                    ? 'bg-accent-teal/5'
                    : 'hover:bg-surface-alt/50'
                  }`}
              >
                <input
                  type="checkbox"
                  checked={selectedOptions.has(option.id)}
                  onChange={() => toggleOption(option.id)}
                  className="mt-0.5 rounded border-border text-accent-teal focus:ring-accent-teal"
                />
                <div>
                  <span className="text-xs font-medium text-text-primary">{option.label}</span>
                  <p className="text-[10px] text-text-muted leading-tight">{option.description}</p>
                </div>
              </label>
            ))}
          </div>
        </div>

        {/* Intensity Slider */}
        <Slider
          value={Math.round(intensity * 100)}
          onChange={(v) => setIntensity(v / 100)}
          min={0}
          max={100}
          step={1}
          label="Intensity"
          unit="%"
        />

        {/* Action Buttons */}
        <div className="flex gap-2">
          <Button
            variant="primary"
            size="md"
            onClick={runUpcycle}
            disabled={selectedOptions.size === 0}
            className="flex-1"
          >
            Upcycle Kit
          </Button>
          <Button
            variant="secondary"
            size="md"
            onClick={loadWithoutUpcycle}
          >
            Load As-Is
          </Button>
        </div>
      </div>
    );
  };

  const renderUpcycling = () => (
    <div className="flex flex-col items-center gap-4 py-8">
      <div className="w-10 h-10 rounded-full progress-gradient flex items-center justify-center">
        <div className="w-4 h-4 rounded-full bg-white animate-pulse" />
      </div>
      <p className="text-sm font-medium text-text-primary">Upcycling kit...</p>
      <ProgressBar
        progress={progressPct}
        detail={progressLabel}
        size="md"
        className="max-w-xs"
      />
    </div>
  );

  const renderResults = () => (
    <div className="space-y-4">
      <div className="flex items-center justify-between">
        <div>
          <h4 className="text-base font-semibold text-text-primary">Upcycle Complete</h4>
          <p className="text-xs text-text-muted mt-0.5">
            {upcycleChanges.length} modification{upcycleChanges.length !== 1 ? 's' : ''} applied to {currentProgram?.name}
          </p>
        </div>
        <Button variant="ghost" size="sm" onClick={() => setStage('analysis')}>
          Back
        </Button>
      </div>

      {/* Changes log */}
      <div className="max-h-60 overflow-y-auto space-y-1 pr-1">
        {upcycleChanges.map((change, i) => (
          <div key={i} className="flex items-start gap-2 py-1.5 px-2 rounded bg-surface-alt/50">
            <span className="text-[10px] font-mono text-accent-teal bg-accent-teal/10 px-1.5 py-0.5 rounded flex-shrink-0">
              Pad {change.padIndex + 1}
            </span>
            <span className="text-xs text-text-secondary">{change.description}</span>
          </div>
        ))}
        {upcycleChanges.length === 0 && (
          <p className="text-xs text-text-muted text-center py-4">
            No modifications were needed - kit is already well configured.
          </p>
        )}
      </div>

      {/* Load button */}
      <div className="flex gap-2">
        <Button variant="primary" size="md" onClick={loadUpcycledKit} className="flex-1">
          Load Upcycled Kit
        </Button>
        <Button variant="secondary" size="md" onClick={resetImporter}>
          Start Over
        </Button>
      </div>
    </div>
  );

  // -------------------------------------------------------------------------
  // Main render
  // -------------------------------------------------------------------------

  return (
    <Card elevated className="w-full">
      <CardHeader>
        <CardTitle>Kit Importer</CardTitle>
        {stage !== 'drop' && (
          <span className="text-[10px] font-mono text-text-muted uppercase tracking-wider">
            {stage === 'analyzing' ? 'Analyzing...' :
             stage === 'analysis' ? 'Ready' :
             stage === 'upcycling' ? 'Processing...' :
             'Complete'}
          </span>
        )}
      </CardHeader>

      {/* Error message */}
      {error && (
        <div className="mb-4 px-3 py-2 bg-red-500/10 border border-red-500/20 rounded-lg">
          <p className="text-xs text-red-400">{error}</p>
        </div>
      )}

      {stage === 'drop' && renderDropZone()}
      {stage === 'analyzing' && renderAnalyzing()}
      {stage === 'analysis' && renderAnalysis()}
      {stage === 'upcycling' && renderUpcycling()}
      {stage === 'results' && renderResults()}
    </Card>
  );
}

// ---------------------------------------------------------------------------
// Sub-components
// ---------------------------------------------------------------------------

function FeatureBadge({ active, label }: { active: boolean; label: string }) {
  return (
    <span
      className={`inline-flex items-center gap-1 px-2 py-0.5 rounded-full text-[10px] font-medium
        ${active
          ? 'bg-accent-teal/10 text-accent-teal'
          : 'bg-surface-alt text-text-muted'
        }`}
    >
      {active ? (
        <svg width="10" height="10" viewBox="0 0 10 10" fill="none">
          <path d="M2 5l2 2 4-4" stroke="currentColor" strokeWidth="1.5" strokeLinecap="round" strokeLinejoin="round" />
        </svg>
      ) : (
        <svg width="10" height="10" viewBox="0 0 10 10" fill="none">
          <path d="M3 3l4 4M7 3l-4 4" stroke="currentColor" strokeWidth="1.5" strokeLinecap="round" />
        </svg>
      )}
      {label}
    </span>
  );
}
