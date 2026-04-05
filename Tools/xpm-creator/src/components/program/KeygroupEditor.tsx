'use client';

import React, { useState } from 'react';
import Card, { CardHeader, CardTitle } from '@/components/ui/Card';
import Button from '@/components/ui/Button';
import Slider from '@/components/ui/Slider';
import PitchSettings from './PitchSettings';
import { useAudioStore } from '@/stores/audioStore';
import { midiNoteToName } from '@/types';
import { validateName } from '@/lib/sanitize';

interface KeygroupEditorProps {
  onBuild: (config: {
    programName: string;
    sampleId: string;
    rootNote: number;
    lowNote: number;
    highNote: number;
  }) => void;
}

export default function KeygroupEditor({ onBuild }: KeygroupEditorProps) {
  const samples = useAudioStore((s) => s.samples);
  const activeSampleId = useAudioStore((s) => s.activeSampleId);
  const [programName, setProgramName] = useState('');
  const [selectedSampleId, setSelectedSampleId] = useState(activeSampleId || '');
  const [rootNote, setRootNote] = useState(60);
  const [lowNote, setLowNote] = useState(36);
  const [highNote, setHighNote] = useState(84);

  const selectedSample = samples.find((s) => s.id === selectedSampleId);

  const handleBuild = () => {
    if (!selectedSampleId || !programName.trim()) return;
    onBuild({
      programName: validateName(programName),
      sampleId: selectedSampleId,
      rootNote,
      lowNote,
      highNote,
    });
  };

  return (
    <div className="space-y-4">
      <Card>
        <CardHeader>
          <CardTitle>Keygroup Program</CardTitle>
        </CardHeader>

        <div className="space-y-3">
          <div>
            <label className="label">Program Name</label>
            <input
              value={programName}
              onChange={(e) => setProgramName(e.target.value)}
              placeholder="My Instrument"
              maxLength={255}
              className="input-field"
            />
          </div>

          <div>
            <label className="label">Source Sample</label>
            <select
              value={selectedSampleId}
              onChange={(e) => setSelectedSampleId(e.target.value)}
              className="input-field"
            >
              <option value="">Select a sample...</option>
              {samples.map((s) => (
                <option key={s.id} value={s.id}>
                  {s.name}
                </option>
              ))}
            </select>
          </div>

          {selectedSample && (
            <div className="bg-surface-alt rounded-lg p-2 text-xs text-text-secondary">
              <p>Duration: {selectedSample.duration.toFixed(2)}s</p>
              <p>Sample Rate: {selectedSample.sampleRate}Hz</p>
              <p>Channels: {selectedSample.channels}</p>
            </div>
          )}
        </div>
      </Card>

      <Card>
        <PitchSettings
          rootNote={rootNote}
          lowNote={lowNote}
          highNote={highNote}
          onRootNoteChange={setRootNote}
          onLowNoteChange={setLowNote}
          onHighNoteChange={setHighNote}
        />
      </Card>

      <Button
        variant="primary"
        className="w-full"
        disabled={!selectedSampleId || !programName.trim()}
        onClick={handleBuild}
      >
        Build Keygroup ({highNote - lowNote + 1} samples)
      </Button>
    </div>
  );
}
