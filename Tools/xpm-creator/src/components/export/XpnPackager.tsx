'use client';

import React, { useState, useCallback } from 'react';
import Card, { CardHeader, CardTitle } from '@/components/ui/Card';
import Button from '@/components/ui/Button';
import CoverArtUploader from './CoverArtUploader';
import ContentArtGenerator from './ContentArtGenerator';
import ExportProgress from './ExportProgress';
import { useExportStore } from '@/stores/exportStore';
import { buildXpnPackage, downloadXpn } from '@/lib/xpn/xpnPackager';
import type { XpnPackageConfig, XpnProgram, XpnSample } from '@/lib/xpn/xpnTypes';
import type { ExportStep } from '@/types';
import type { PreviewPadInfo } from '@/lib/audio/previewRenderer';
import { renderPreview } from '@/lib/audio/previewRenderer';
import { usePadStore } from '@/stores/padStore';
import { useAudioStore } from '@/stores/audioStore';
import { useToastStore } from '@/stores/toastStore';
import { getDecodedBuffer } from '@/lib/audio/audioBufferCache';
import { categorizeSample } from '@/lib/audio/sampleCategorizer';

interface StoredProgram {
  id: string;
  name: string;
  xpmContent: string;
  samples: { fileName: string; data: ArrayBuffer }[];
}

export default function XpnPackager() {
  const [title, setTitle] = useState('');
  const [identifier, setIdentifier] = useState('');
  const [description, setDescription] = useState('');
  const [creator, setCreator] = useState('');
  const [tags, setTags] = useState('');
  const [coverArt, setCoverArt] = useState<{ data: ArrayBuffer; type: 'jpg' | 'png' } | null>(null);
  const [programs, setPrograms] = useState<StoredProgram[]>([]);
  const [groupNames, setGroupNames] = useState<Map<number, { group: string; sub: string }>>(new Map());
  const [previewAudio, setPreviewAudio] = useState<{ data: ArrayBuffer; type: 'wav' } | null>(null);
  const [isGeneratingPreview, setIsGeneratingPreview] = useState(false);

  const exportStore = useExportStore();

  const handleAddProgram = (program: Omit<StoredProgram, 'id'>) => {
    setPrograms((prev) => [...prev, { ...program, id: crypto.randomUUID() }]);
  };

  const handleRemoveProgram = (index: number) => {
    setPrograms((prev) => prev.filter((_, i) => i !== index));
  };

  const handleUpdateGroupName = (index: number, group: string, sub: string) => {
    setGroupNames((prev) => {
      const next = new Map(prev);
      next.set(index, { group, sub });
      return next;
    });
  };

  const handleGeneratePreview = useCallback(async () => {
    setIsGeneratingPreview(true);
    try {
      const { pads: storePads } = usePadStore.getState();
      const { samples: storeSamples } = useAudioStore.getState();

      // Build a lookup from sampleId → AudioSample
      const sampleMap = new Map(storeSamples.map((s) => [s.id, s]));

      // Walk pads, collect first active layer's sample and detect its role
      const previewPads: PreviewPadInfo[] = [];

      for (let i = 0; i < storePads.length; i++) {
        const pad = storePads[i];
        // Find first active layer with a sample
        const activeLayer = pad.layers.find((l) => l.active && l.sampleId);
        if (!activeLayer || !activeLayer.sampleId) continue;

        const sample = sampleMap.get(activeLayer.sampleId);
        if (!sample) continue;

        // Decode the WAV buffer to AudioBuffer for the renderer
        const audioBuffer = await getDecodedBuffer(sample.id, sample.buffer);

        // Determine role from filename categorization
        const cat = categorizeSample(sample.fileName || sample.name);
        let role: PreviewPadInfo['role'] = 'other';
        if (cat.category === 'kick') role = 'kick';
        else if (cat.category === 'snare' || cat.category === 'clap') role = 'snare';
        else if (cat.category === 'hat-closed' || cat.category === 'hat-open' || cat.category === 'cymbal') role = 'hat';

        previewPads.push({ padIndex: i, buffer: audioBuffer, role });
      }

      if (previewPads.length === 0) {
        useToastStore.getState().addToast({
          type: 'warning',
          title: 'No samples available',
          message: 'Assign samples to pads first to generate a preview',
        });
        return;
      }

      const wavData = await renderPreview(previewPads, { bpm: 90, duration: 5 });
      setPreviewAudio({ data: wavData, type: 'wav' });
      useToastStore.getState().addToast({ type: 'success', title: 'Preview track rendered' });
    } catch (error) {
      console.error('Preview generation failed:', error);
      useToastStore.getState().addToast({
        type: 'error',
        title: 'Preview generation failed',
        message: error instanceof Error ? error.message : String(error),
      });
    } finally {
      setIsGeneratingPreview(false);
    }
  }, []);

  const MAX_NAME_LENGTH = 32;

  const handleBuildXpn = useCallback(async () => {
    if (!title.trim() || programs.length === 0) return;

    const steps: ExportStep[] = [
      { id: 'prepare', label: 'Preparing programs', description: '', progress: 0, status: 'pending' },
      { id: 'samples', label: 'Collecting samples', description: '', progress: 0, status: 'pending' },
      { id: 'metadata', label: 'Generating metadata', description: '', progress: 0, status: 'pending' },
      { id: 'package', label: 'Building XPN package', description: '', progress: 0, status: 'pending' },
    ];

    exportStore.startExport(steps);

    try {
      exportStore.updateStep('prepare', { status: 'active' });
      const xpnPrograms: XpnProgram[] = programs.map((p, i) => {
        const names = groupNames.get(i);
        return {
          name: p.name.substring(0, MAX_NAME_LENGTH),
          xpmContent: p.xpmContent,
          groupName: (names?.group || 'Default').substring(0, MAX_NAME_LENGTH),
          subName: (names?.sub || p.name).substring(0, MAX_NAME_LENGTH),
        };
      });
      exportStore.completeStep('prepare');
      exportStore.setOverallProgress(20);

      exportStore.updateStep('samples', { status: 'active' });
      const allSamples: XpnSample[] = [];
      const seenFileNames = new Set<string>();
      for (const prog of programs) {
        for (const sample of prog.samples) {
          if (!seenFileNames.has(sample.fileName)) {
            seenFileNames.add(sample.fileName);
            allSamples.push(sample);
          }
        }
      }
      exportStore.completeStep('samples');
      exportStore.setOverallProgress(40);

      exportStore.updateStep('metadata', { status: 'active' });
      const tagList = tags.split(',').map((t) => t.trim()).filter(Boolean);
      const id = identifier.trim() || `com.xpmcreator.${title.replace(/\s+/g, '').toLowerCase()}`;

      const config: XpnPackageConfig = {
        metadata: {
          title: title.trim().substring(0, MAX_NAME_LENGTH),
          identifier: id,
          description: description.trim(),
          creator: creator.trim() || 'XPM Creator',
          tags: tagList,
          version: '1.0',
        },
        programs: xpnPrograms,
        samples: allSamples,
        coverArt: coverArt || undefined,
        previewAudio: previewAudio || undefined,
      };
      exportStore.completeStep('metadata');
      exportStore.setOverallProgress(60);

      exportStore.updateStep('package', { status: 'active' });
      const blob = await buildXpnPackage(config, (step, progress) => {
        exportStore.updateStep('package', { detail: step, progress });
        exportStore.setOverallProgress(60 + (progress / 100) * 40);
      });

      downloadXpn(blob, title.trim().substring(0, MAX_NAME_LENGTH));
      exportStore.completeStep('package');
      exportStore.setOverallProgress(100);
      exportStore.finishExport();
    } catch (error) {
      const msg = error instanceof Error ? error.message : 'Unknown error';
      exportStore.failExport(msg);
      useToastStore.getState().addToast({
        type: 'error',
        title: 'Export failed',
        message: msg,
      });
    }
  }, [title, identifier, description, creator, tags, coverArt, previewAudio, programs, groupNames, exportStore]);

  return (
    <div className="space-y-4">
      <Card>
        <CardHeader>
          <CardTitle>Build XPN Expansion</CardTitle>
        </CardHeader>

        <div className="space-y-3">
          <div className="grid grid-cols-1 md:grid-cols-2 gap-3">
            <div>
              <label className="label">Expansion Title</label>
              <input
                value={title}
                onChange={(e) => setTitle(e.target.value)}
                placeholder="My Expansion Pack"
                className="input-field"
              />
            </div>
            <div>
              <label className="label">Identifier</label>
              <input
                value={identifier}
                onChange={(e) => setIdentifier(e.target.value)}
                placeholder="com.brand.expansionname"
                className="input-field"
              />
            </div>
          </div>

          <div>
            <label className="label">Description</label>
            <textarea
              value={description}
              onChange={(e) => setDescription(e.target.value)}
              placeholder="A collection of custom sounds..."
              className="input-field min-h-[60px] resize-y"
              rows={2}
            />
          </div>

          <div className="grid grid-cols-1 md:grid-cols-2 gap-3">
            <div>
              <label className="label">Creator</label>
              <input
                value={creator}
                onChange={(e) => setCreator(e.target.value)}
                placeholder="Your name"
                className="input-field"
              />
            </div>
            <div>
              <label className="label">Tags (comma-separated)</label>
              <input
                value={tags}
                onChange={(e) => setTags(e.target.value)}
                placeholder="drums, keys, bass"
                className="input-field"
              />
            </div>
          </div>

          <div className="flex gap-4 items-start">
            <CoverArtUploader
              onImageSelected={(data, type) => setCoverArt({ data, type })}
              currentImage={coverArt?.data}
            />
            <div className="flex-1">
              <ContentArtGenerator
                title={title || 'My Expansion'}
                subtitle={creator || 'XO_OX'}
                onGenerated={(data, type) => setCoverArt({ data, type })}
              />
            </div>
          </div>
        </div>
      </Card>

      {/* Programs list */}
      <Card>
        <CardHeader>
          <CardTitle>Programs ({programs.length})</CardTitle>
        </CardHeader>

        {programs.length === 0 ? (
          <div className="text-center py-4">
            <p className="text-xs text-text-muted">
              Build XPM programs first, then add them here
            </p>
          </div>
        ) : (
          <div className="space-y-2">
            {programs.map((prog, i) => {
              const names = groupNames.get(i) || { group: '', sub: '' };
              return (
                <div key={prog.id} className="flex items-center gap-2 p-2 bg-surface-alt rounded-lg">
                  <div className="flex-1 min-w-0 space-y-1">
                    <p className="text-xs font-medium text-text-primary truncate">{prog.name}</p>
                    <div className="flex gap-2">
                      <input
                        value={names.group}
                        onChange={(e) => handleUpdateGroupName(i, e.target.value, names.sub)}
                        placeholder="Group"
                        className="input-field text-[10px] py-0.5 flex-1"
                      />
                      <input
                        value={names.sub}
                        onChange={(e) => handleUpdateGroupName(i, names.group, e.target.value)}
                        placeholder="Sub-name"
                        className="input-field text-[10px] py-0.5 flex-1"
                      />
                    </div>
                  </div>
                  <button
                    onClick={() => handleRemoveProgram(i)}
                    className="p-1 text-text-muted hover:text-red-500 transition-colors"
                  >
                    <svg width="14" height="14" viewBox="0 0 14 14" fill="none">
                      <path d="M4 4l6 6M10 4l-6 6" stroke="currentColor" strokeWidth="1.5" strokeLinecap="round" />
                    </svg>
                  </button>
                </div>
              );
            })}
          </div>
        )}
      </Card>

      {/* Preview audio section */}
      <div className="flex items-center gap-2">
        {previewAudio ? (
          <p className="text-[10px] text-accent-teal flex-1">
            ✓ Preview audio ready (5s WAV)
          </p>
        ) : (
          <p className="text-[10px] text-text-muted flex-1">
            Optional: Generate a preview clip for the expansion
          </p>
        )}
        <Button
          variant="secondary"
          size="sm"
          disabled={isGeneratingPreview}
          loading={isGeneratingPreview}
          onClick={handleGeneratePreview}
        >
          {isGeneratingPreview ? 'Generating...' : previewAudio ? 'Regenerate Preview' : 'Generate Preview'}
        </Button>
      </div>

      <Button
        variant="accent"
        className="w-full"
        disabled={!title.trim() || programs.length === 0}
        onClick={handleBuildXpn}
      >
        Build XPN ({programs.length} program{programs.length !== 1 ? 's' : ''})
      </Button>

      <ExportProgress />
    </div>
  );
}
