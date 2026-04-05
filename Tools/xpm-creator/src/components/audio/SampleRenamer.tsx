'use client';

import React, { useState, useCallback } from 'react';
import { useAudioStore } from '@/stores/audioStore';
import { usePadStore } from '@/stores/padStore';
import { midiNoteToName } from '@/types';
import Button from '@/components/ui/Button';

type RenameTemplate = 'pad-position' | 'note-velocity' | 'kit-prefix';

const TEMPLATES: { id: RenameTemplate; label: string; example: string }[] = [
  {
    id: 'pad-position',
    label: 'Pad Position',
    example: 'MyKit_Pad01_Kick',
  },
  {
    id: 'note-velocity',
    label: 'Note + Velocity',
    example: 'MyKit_C3_V1',
  },
  {
    id: 'kit-prefix',
    label: 'Kit Prefix Only',
    example: 'MyKit_Kick',
  },
];

export default function SampleRenamer() {
  const samples = useAudioStore((s) => s.samples);
  const renameSample = useAudioStore((s) => s.renameSample);
  const pads = usePadStore((s) => s.pads);
  const [kitName, setKitName] = useState('MyKit');
  const [template, setTemplate] = useState<RenameTemplate>('pad-position');
  const [showRenamer, setShowRenamer] = useState(false);

  const handleRename = useCallback(() => {
    if (!kitName.trim() || samples.length === 0) return;

    const prefix = kitName.trim().replace(/[^a-zA-Z0-9_-]/g, '_');

    // Build a map of sampleId → pad/layer info for position-based naming
    const samplePadMap = new Map<string, { padIdx: number; layerIdx: number }>();
    for (let p = 0; p < pads.length; p++) {
      for (let l = 0; l < pads[p].layers.length; l++) {
        const layer = pads[p].layers[l];
        if (layer.active && layer.sampleId && !samplePadMap.has(layer.sampleId)) {
          samplePadMap.set(layer.sampleId, { padIdx: p, layerIdx: l });
        }
      }
    }

    for (const sample of samples) {
      let newName: string;
      const padInfo = samplePadMap.get(sample.id);

      switch (template) {
        case 'pad-position': {
          if (padInfo) {
            const padNum = String(padInfo.padIdx + 1).padStart(2, '0');
            const layerNum = padInfo.layerIdx + 1;
            const original = sample.name.replace(/^.*?_/, '').substring(0, 16);
            newName = `${prefix}_Pad${padNum}_L${layerNum}_${original}`;
          } else {
            newName = `${prefix}_${sample.name}`;
          }
          break;
        }

        case 'note-velocity': {
          const noteName = midiNoteToName(sample.rootNote).replace('#', 's');
          if (padInfo) {
            const layer = pads[padInfo.padIdx].layers[padInfo.layerIdx];
            newName = `${prefix}_${noteName}_V${padInfo.layerIdx + 1}`;
          } else {
            newName = `${prefix}_${noteName}`;
          }
          break;
        }

        case 'kit-prefix': {
          // Just add the prefix, keep the original name
          const cleanOriginal = sample.name
            .replace(/^[^_]*_/, '') // Remove any existing prefix
            .substring(0, 24);
          newName = `${prefix}_${cleanOriginal || sample.name.substring(0, 24)}`;
          break;
        }

        default:
          newName = sample.name;
      }

      renameSample(sample.id, newName);
    }

    setShowRenamer(false);
  }, [kitName, template, samples, pads, renameSample]);

  if (samples.length === 0) return null;

  if (!showRenamer) {
    return (
      <button
        onClick={() => setShowRenamer(true)}
        className="w-full flex items-center gap-1.5 px-2 py-1.5 rounded text-[10px] font-medium
          bg-surface-alt text-text-secondary hover:text-text-primary hover:bg-surface
          transition-all border border-transparent hover:border-border"
      >
        <span className="text-xs">🏷</span>
        Batch Rename
      </button>
    );
  }

  return (
    <div className="space-y-2 p-2 bg-surface-alt rounded-lg border border-border">
      <p className="text-[10px] text-text-muted uppercase tracking-wider font-semibold">
        Batch Rename ({samples.length} samples)
      </p>

      {/* Kit name input */}
      <input
        value={kitName}
        onChange={(e) => setKitName(e.target.value)}
        placeholder="Kit name prefix"
        className="input-field text-xs py-1"
      />

      {/* Template selector */}
      <div className="space-y-1">
        {TEMPLATES.map((t) => (
          <button
            key={t.id}
            onClick={() => setTemplate(t.id)}
            className={`w-full text-left px-2 py-1.5 rounded text-[10px] transition-all
              ${template === t.id
                ? 'bg-accent-teal/10 text-accent-teal border border-accent-teal/20'
                : 'bg-surface text-text-secondary hover:text-text-primary border border-transparent'
              }`}
          >
            <span className="font-medium">{t.label}</span>
            <br />
            <span className="text-text-muted">{t.example.replace('MyKit', kitName || 'MyKit')}</span>
          </button>
        ))}
      </div>

      {/* Actions */}
      <div className="flex gap-1 pt-1">
        <Button
          variant="primary"
          size="sm"
          onClick={handleRename}
          disabled={!kitName.trim()}
        >
          Rename All
        </Button>
        <Button
          variant="ghost"
          size="sm"
          onClick={() => setShowRenamer(false)}
        >
          Cancel
        </Button>
      </div>
    </div>
  );
}
