'use client';

import React from 'react';
import Slider from '@/components/ui/Slider';
import { midiNoteToName } from '@/types';

interface PitchSettingsProps {
  rootNote: number;
  lowNote: number;
  highNote: number;
  onRootNoteChange: (note: number) => void;
  onLowNoteChange: (note: number) => void;
  onHighNoteChange: (note: number) => void;
}

export default function PitchSettings({
  rootNote,
  lowNote,
  highNote,
  onRootNoteChange,
  onLowNoteChange,
  onHighNoteChange,
}: PitchSettingsProps) {
  const totalNotes = highNote - lowNote + 1;

  return (
    <div className="space-y-3">
      <span className="label">Pitch Shift Settings</span>

      <div className="bg-surface-alt rounded-lg p-3 space-y-2">
        <div className="flex items-center justify-between text-xs">
          <span className="text-text-secondary">Root Note</span>
          <span className="font-mono text-accent-teal font-medium">
            {midiNoteToName(rootNote)} ({rootNote})
          </span>
        </div>
        <Slider
          value={rootNote}
          onChange={onRootNoteChange}
          min={0}
          max={127}
          step={1}
        />
      </div>

      <div className="bg-surface-alt rounded-lg p-3 space-y-2">
        <div className="flex items-center justify-between text-xs">
          <span className="text-text-secondary">Range</span>
          <span className="font-mono text-text-muted">
            {midiNoteToName(lowNote)} - {midiNoteToName(highNote)}
          </span>
        </div>
        <Slider
          label="Low"
          value={lowNote}
          onChange={onLowNoteChange}
          min={0}
          max={rootNote}
          step={1}
        />
        <Slider
          label="High"
          value={highNote}
          onChange={onHighNoteChange}
          min={rootNote}
          max={127}
          step={1}
        />
      </div>

      <div className="flex items-center justify-between text-xs text-text-muted px-1">
        <span>{totalNotes} notes to generate</span>
        <span>{totalNotes} WAV files</span>
      </div>

      {/* Visual keyboard range display */}
      <div className="bg-surface rounded-lg border border-border p-2">
        <div className="flex gap-px h-6 overflow-hidden rounded">
          {Array.from({ length: 49 }, (_, i) => {
            const note = 36 + i; // Start at C2
            const isBlack = [1, 3, 6, 8, 10].includes(note % 12);
            const inRange = note >= lowNote && note <= highNote;
            const isRoot = note === rootNote;
            return (
              <div
                key={i}
                className={`flex-1 min-w-[3px] rounded-sm transition-colors
                  ${isRoot
                    ? 'bg-accent-teal'
                    : inRange
                      ? isBlack ? 'bg-accent-plum/60' : 'bg-accent-plum/20'
                      : isBlack ? 'bg-text-primary/70' : 'bg-surface-alt'
                  }`}
              />
            );
          })}
        </div>
      </div>
    </div>
  );
}
