'use client';

import React, { useState, useCallback } from 'react';
import Card, { CardHeader, CardTitle } from '@/components/ui/Card';
import Button from '@/components/ui/Button';
import Tabs from '@/components/ui/Tabs';
import ExportProgress from './ExportProgress';
import KeygroupEditor from '@/components/program/KeygroupEditor';
import DrumEditor from '@/components/program/DrumEditor';
import ProgramEditor from '@/components/program/ProgramEditor';
import { useAudioStore } from '@/stores/audioStore';
import { usePadStore } from '@/stores/padStore';
import { useExportStore, type ExportBitDepth } from '@/stores/exportStore';
import { buildKeygroupProgram } from '@/lib/xpm/xpmKeygroupGenerator';
import { buildDrumProgram } from '@/lib/xpm/xpmDrumGenerator';
import { downloadXpmWithSamples } from '@/lib/xpn/xpnPackager';
import { getDecodedBuffer } from '@/lib/audio/audioBufferCache';
import { encodeWav } from '@/lib/audio/wavEncoder';
import { useToastStore } from '@/stores/toastStore';
import type { ExportStep, ProgramType } from '@/types';
import { getProcessingMessage } from '@/lib/audio/processingMessages';
import { useExportPresetStore } from '@/stores/exportPresetStore';
import Select from '@/components/ui/Select';

export default function XpmExporter() {
  const [programType, setProgramType] = useState<ProgramType>('Keygroup');
  const samples = useAudioStore((s) => s.samples);
  const pads = usePadStore((s) => s.pads);
  const bitDepth = useExportStore((s) => s.bitDepth);
  const setBitDepth = useExportStore((s) => s.setBitDepth);

  // Export presets
  const presets = useExportPresetStore((s) => s.presets);
  const activePresetId = useExportPresetStore((s) => s.activePresetId);
  const applyPreset = useExportPresetStore((s) => s.applyPreset);

  const handlePresetChange = useCallback(
    (e: React.ChangeEvent<HTMLSelectElement>) => {
      const presetId = e.target.value;
      if (!presetId) return;
      const preset = applyPreset(presetId);
      if (preset) {
        setProgramType(preset.programType);
        setBitDepth(preset.bitDepth);
      }
    },
    [applyPreset, setBitDepth]
  );
  const startExport = useExportStore((s) => s.startExport);
  const updateStep = useExportStore((s) => s.updateStep);
  const completeStep = useExportStore((s) => s.completeStep);
  const setOverallProgress = useExportStore((s) => s.setOverallProgress);
  const finishExport = useExportStore((s) => s.finishExport);
  const failExport = useExportStore((s) => s.failExport);

  const handleBuildKeygroup = useCallback(
    async (config: { programName: string; sampleId: string; rootNote: number; lowNote: number; highNote: number }) => {
      const sample = samples.find((s) => s.id === config.sampleId);
      if (!sample) return;

      const steps: ExportStep[] = [
        { id: 'prepare', label: 'Preparing samples', description: getProcessingMessage('export'), progress: 0, status: 'pending' },
        { id: 'pitch', label: 'Pitch-shifting samples', description: getProcessingMessage('process'), progress: 0, status: 'pending' },
        { id: 'encode', label: 'Encoding WAV files', description: getProcessingMessage('build'), progress: 0, status: 'pending' },
        { id: 'build', label: 'Building XPM structure', description: getProcessingMessage('build'), progress: 0, status: 'pending' },
        { id: 'package', label: 'Packaging files', description: getProcessingMessage('export'), progress: 0, status: 'pending' },
      ];

      startExport(steps);

      try {
        updateStep('prepare', { status: 'active', progress: 50 });
        const audioBuffer = await getDecodedBuffer(sample.id, sample.buffer);
        completeStep('prepare');
        setOverallProgress(10);

        updateStep('pitch', { status: 'active' });
        const { bitDepth } = useExportStore.getState();
        const result = await buildKeygroupProgram({
          programName: config.programName,
          sourceBuffer: audioBuffer,
          rootNote: config.rootNote,
          lowNote: config.lowNote,
          highNote: config.highNote,
          layers: pads[0]?.layers || [],
          bitDepth,
          onProgress: (step, progress) => {
            updateStep('pitch', { detail: step, progress });
            setOverallProgress(10 + (progress / 100) * 60);
          },
        });
        completeStep('pitch');
        setOverallProgress(70);

        updateStep('encode', { status: 'active', progress: 50 });
        completeStep('encode');
        setOverallProgress(80);

        updateStep('build', { status: 'active', progress: 50 });
        completeStep('build');
        setOverallProgress(90);

        updateStep('package', { status: 'active' });
        await downloadXpmWithSamples(
          result.xpmContent,
          config.programName,
          result.samples.map((s) => ({ fileName: s.fileName, data: s.data }))
        );
        completeStep('package');
        setOverallProgress(100);
        finishExport();
      } catch (error) {
        const msg = error instanceof Error ? error.message : 'Unknown error';
        failExport(msg);
      }
    },
    [samples, pads, startExport, updateStep, completeStep, setOverallProgress, finishExport, failExport]
  );

  const handleBuildDrum = useCallback(
    async (config: { programName: string }) => {
      const steps: ExportStep[] = [
        { id: 'prepare', label: 'Preparing samples', description: getProcessingMessage('export'), progress: 0, status: 'pending' },
        { id: 'build', label: 'Building XPM structure', description: getProcessingMessage('build'), progress: 0, status: 'pending' },
        { id: 'package', label: 'Packaging files', description: getProcessingMessage('export'), progress: 0, status: 'pending' },
      ];

      startExport(steps);

      try {
        updateStep('prepare', { status: 'active', progress: 50 });

        // Collect sample files from loaded pads, re-encoding to WAV at selected bit depth
        const { bitDepth } = useExportStore.getState();
        const sampleFiles: { fileName: string; data: ArrayBuffer }[] = [];
        const seenFiles = new Set<string>();
        const missingSamples: string[] = [];

        for (const pad of pads) {
          for (const layer of pad.layers) {
            const normalizedFile = layer.sampleFile.replace(/\.wav$/i, '.WAV');
            if (layer.active && layer.sampleId && !seenFiles.has(normalizedFile)) {
              seenFiles.add(normalizedFile);
              const sample = samples.find((s) => s.id === layer.sampleId);
              if (sample) {
                // Re-encode to WAV at the selected bit depth
                const audioBuffer = await getDecodedBuffer(sample.id, sample.buffer);
                const wavData = encodeWav(audioBuffer, bitDepth);
                // Normalize to .WAV — MPC's Linux FS is case-sensitive
                sampleFiles.push({ fileName: layer.sampleFile.replace(/\.wav$/i, '.WAV'), data: wavData });
              } else {
                // Sample referenced by pad but not found in the loaded samples list.
                // The exported XPM will reference a WAV that doesn't exist in the ZIP.
                missingSamples.push(normalizedFile);
              }
            }
          }
        }

        // Warn the user about missing samples so they know their export may
        // have silent pads on MPC hardware.
        if (missingSamples.length > 0) {
          useToastStore.getState().addToast({
            type: 'warning',
            title: `${missingSamples.length} sample(s) missing from export`,
            message: 'Some pad layers reference samples that are no longer loaded. Those pads will be silent on MPC.',
          });
        }

        completeStep('prepare');
        setOverallProgress(30);

        updateStep('build', { status: 'active', progress: 50 });
        const xpmContent = buildDrumProgram({
          programName: config.programName,
          padAssignments: pads,
        });
        completeStep('build');
        setOverallProgress(60);

        updateStep('package', { status: 'active' });
        await downloadXpmWithSamples(xpmContent, config.programName, sampleFiles);
        completeStep('package');
        setOverallProgress(100);
        finishExport();
      } catch (error) {
        const msg = error instanceof Error ? error.message : 'Unknown error';
        failExport(msg);
      }
    },
    [samples, pads, startExport, updateStep, completeStep, setOverallProgress, finishExport, failExport]
  );

  // Show helpful empty state when no samples are loaded
  const hasSamples = samples.length > 0;
  const hasPadContent = pads.some((p) => p.layers.some((l) => l.active && l.sampleId));

  if (!hasSamples && !hasPadContent) {
    return (
      <Card>
        <div className="flex flex-col items-center justify-center py-10 text-center space-y-3">
          <div className="w-12 h-12 rounded-full bg-surface-alt flex items-center justify-center">
            <svg width="24" height="24" viewBox="0 0 24 24" fill="none" className="text-text-muted">
              <path d="M12 3v18M3 12h18" stroke="currentColor" strokeWidth="2" strokeLinecap="round" />
            </svg>
          </div>
          <div>
            <p className="text-sm font-medium text-text-primary">No samples to export</p>
            <p className="text-xs text-text-muted mt-1 max-w-[240px]">
              Import audio samples and assign them to pads first, then come back here to build your MPC program.
            </p>
          </div>
        </div>
      </Card>
    );
  }

  return (
    <div className="space-y-4">
      {/* Export preset selector */}
      <div className="flex items-center gap-2">
        <Select
          options={presets.map((p) => ({ value: p.id, label: p.name }))}
          value={activePresetId ?? ''}
          onChange={handlePresetChange}
          placeholder="Apply a preset…"
          size="sm"
          label="Export Preset"
        />
      </div>

      <Card>
        <CardHeader>
          <CardTitle>Build XPM Program</CardTitle>
        </CardHeader>

        <Tabs
          tabs={[
            { id: 'Keygroup', label: 'Keygroup' },
            { id: 'Drum', label: 'Drum' },
          ]}
          activeTab={programType}
          onChange={(t) => { const VALID_PROGRAM_TYPES = new Set(['Drum', 'Keygroup', 'Clip', 'Plugin']); if (VALID_PROGRAM_TYPES.has(t)) setProgramType(t as ProgramType); }}
          className="mb-4"
        />

        {/* Bit depth selector */}
        <div className="flex items-center gap-3 mb-4 px-1">
          <span className="text-xs font-medium text-text-secondary">WAV Bit Depth:</span>
          <div className="flex gap-1">
            {([16, 24] as ExportBitDepth[]).map((depth) => (
              <button
                key={depth}
                aria-pressed={bitDepth === depth}
                onClick={() => setBitDepth(depth)}
                className={`px-3 py-1.5 rounded text-xs font-medium transition-all
                  ${bitDepth === depth
                    ? 'bg-accent-teal text-white'
                    : 'bg-surface-alt text-text-secondary hover:text-text-primary'
                  }`}
              >
                {depth}-bit
              </button>
            ))}
          </div>
          <span className="text-[10px] text-text-muted">
            {bitDepth === 24 ? 'Higher quality, larger files' : 'Standard MPC quality'}
          </span>
        </div>

        {programType === 'Keygroup' ? (
          <KeygroupEditor onBuild={handleBuildKeygroup} />
        ) : (
          <DrumEditor onBuild={handleBuildDrum} />
        )}
      </Card>

      {programType === 'Keygroup' && (
        <ProgramEditor programType={programType} />
      )}

      <ExportProgress />
    </div>
  );
}
