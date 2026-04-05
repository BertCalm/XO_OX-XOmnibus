'use client';

import React, { useState, useCallback, useMemo, useEffect } from 'react';
import Card from '@/components/ui/Card';
import Tabs from '@/components/ui/Tabs';
import Slider from '@/components/ui/Slider';
import PitchSettings from './PitchSettings';
import EnvelopeEditor from './EnvelopeEditor';
import HumanizeControls from './HumanizeControls';
import { useEnvelopeStore } from '@/stores/envelopeStore';
import { usePadStore } from '@/stores/padStore';

const FILTER_TYPES = [
  { value: 0, label: 'Off' },
  { value: 1, label: 'Low Pass' },
  { value: 2, label: 'Band Pass' },
  { value: 3, label: 'High Pass' },
];

interface ProgramEditorProps {
  programType: 'Keygroup' | 'Drum';
}

export default function ProgramEditor({ programType }: ProgramEditorProps) {
  const [activeTab, setActiveTab] = useState(programType === 'Keygroup' ? 'pitch' : 'envelope');
  const [rootNote, setRootNote] = useState(60);
  const [lowNote, setLowNote] = useState(36);
  const [highNote, setHighNote] = useState(84);

  // Reset tab when program type changes — Drum has no pitch tab
  useEffect(() => {
    setActiveTab(programType === 'Keygroup' ? 'pitch' : 'envelope');
  }, [programType]);

  // Granular selectors — avoid subscribing to entire stores
  const activePadIndex = usePadStore((s) => s.activePadIndex);
  // Subscribe directly to envelope data — using the getter function reference
  // would be non-reactive (stable ref that never triggers re-render).
  const envSettings = useEnvelopeStore(
    (s) => s.padEnvelopes[activePadIndex] ?? s.getEnvelope(activePadIndex),
  );
  const setVolumeEnvelope = useEnvelopeStore((s) => s.setVolumeEnvelope);
  const setFilterEnvelope = useEnvelopeStore((s) => s.setFilterEnvelope);
  const setFilterSettings = useEnvelopeStore((s) => s.setFilterSettings);

  // Stable callbacks — avoid inline closures in render
  const handleFilterTypeChange = useCallback(
    (type: number) => {
      setFilterSettings(activePadIndex, { filterType: type });
    },
    [activePadIndex, setFilterSettings]
  );

  const handleVolumeEnvelopeChange = useCallback(
    (env: Parameters<typeof setVolumeEnvelope>[1]) => {
      setVolumeEnvelope(activePadIndex, env);
    },
    [activePadIndex, setVolumeEnvelope]
  );

  const handleFilterEnvelopeChange = useCallback(
    (env: Parameters<typeof setFilterEnvelope>[1]) => {
      setFilterEnvelope(activePadIndex, env);
    },
    [activePadIndex, setFilterEnvelope]
  );

  const handleCutoffChange = useCallback(
    (v: number) => setFilterSettings(activePadIndex, { filterCutoff: v }),
    [activePadIndex, setFilterSettings]
  );

  const handleResonanceChange = useCallback(
    (v: number) => setFilterSettings(activePadIndex, { filterResonance: v }),
    [activePadIndex, setFilterSettings]
  );

  const handleEnvAmountChange = useCallback(
    (v: number) => setFilterSettings(activePadIndex, { filterEnvAmount: v }),
    [activePadIndex, setFilterSettings]
  );

  const tabs = useMemo(() =>
    programType === 'Keygroup'
      ? [
          { id: 'pitch', label: 'Pitch' },
          { id: 'envelope', label: 'Envelope' },
          { id: 'humanize', label: 'Humanize' },
        ]
      : [
          { id: 'envelope', label: 'Envelope' },
          { id: 'humanize', label: 'Humanize' },
        ],
    [programType]
  );

  return (
    <Card padding="none">
      <div className="p-3 border-b border-border">
        <Tabs tabs={tabs} activeTab={activeTab} onChange={setActiveTab} />
      </div>

      <div className="p-4">
        {activeTab === 'pitch' && programType === 'Keygroup' && (
          <PitchSettings
            rootNote={rootNote}
            lowNote={lowNote}
            highNote={highNote}
            onRootNoteChange={setRootNote}
            onLowNoteChange={setLowNote}
            onHighNoteChange={setHighNote}
          />
        )}

        {activeTab === 'envelope' && (
          <div className="space-y-6">
            <EnvelopeEditor
              label="Volume Envelope"
              envelope={envSettings.volumeEnvelope}
              onChange={handleVolumeEnvelopeChange}
            />
            <EnvelopeEditor
              label="Filter Envelope"
              envelope={envSettings.filterEnvelope}
              onChange={handleFilterEnvelopeChange}
            />

            {/* Filter Controls */}
            <div className="space-y-3">
              <h3 className="text-xs font-semibold text-text-secondary uppercase tracking-wider">
                Filter
              </h3>

              {/* Filter type selector */}
              <div className="flex items-center gap-1">
                {FILTER_TYPES.map((ft) => (
                  <button
                    key={ft.value}
                    onClick={() => handleFilterTypeChange(ft.value)}
                    className={`px-3 py-1.5 rounded text-xs font-medium transition-all
                      ${envSettings.filterType === ft.value
                        ? 'bg-accent-plum text-white'
                        : 'bg-surface-alt text-text-secondary hover:text-text-primary'
                      }`}
                  >
                    {ft.label}
                  </button>
                ))}
              </div>

              {/* Filter parameters — only show when filter is active */}
              {envSettings.filterType !== 0 && (
                <div className="space-y-2">
                  <Slider
                    value={envSettings.filterCutoff}
                    onChange={handleCutoffChange}
                    min={0}
                    max={1}
                    step={0.01}
                    label="Cutoff"
                  />
                  <Slider
                    value={envSettings.filterResonance}
                    onChange={handleResonanceChange}
                    min={0}
                    max={1}
                    step={0.01}
                    label="Resonance"
                  />
                  <Slider
                    value={envSettings.filterEnvAmount}
                    onChange={handleEnvAmountChange}
                    min={-1}
                    max={1}
                    step={0.01}
                    label="Envelope Amount"
                  />
                </div>
              )}
            </div>
          </div>
        )}

        {activeTab === 'humanize' && <HumanizeControls />}
      </div>
    </Card>
  );
}
