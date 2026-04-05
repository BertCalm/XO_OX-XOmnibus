'use client';

import React, { useState, useMemo } from 'react';
import Card, { CardHeader, CardTitle } from '@/components/ui/Card';
import Button from '@/components/ui/Button';
import Tooltip from '@/components/ui/Tooltip';
import PadGrid from '@/components/pads/PadGrid';
import PadLayerEditor from '@/components/pads/PadLayerEditor';
import { usePadStore } from '@/stores/padStore';

interface DrumEditorProps {
  onBuild: (config: { programName: string }) => void;
  /** When true, the Build button is disabled and reasons are shown as a tooltip */
  exportBlocked?: boolean;
  exportBlockedReasons?: string[];
}

export default function DrumEditor({ onBuild, exportBlocked = false, exportBlockedReasons = [] }: DrumEditorProps) {
  const [programName, setProgramName] = useState('');
  const pads = usePadStore((s) => s.pads);

  const loadedPadCount = useMemo(
    () => pads.filter((p) => p.layers.some((l) => l.active && l.sampleId)).length,
    [pads]
  );

  const handleBuild = () => {
    if (!programName.trim()) return;
    onBuild({ programName: programName.trim() });
  };

  return (
    <div className="space-y-4">
      <Card>
        <CardHeader>
          <CardTitle>Drum Program</CardTitle>
        </CardHeader>

        <div className="space-y-3">
          <div>
            <label className="label">Program Name</label>
            <input
              value={programName}
              onChange={(e) => setProgramName(e.target.value)}
              placeholder="My Drum Kit"
              className="input-field"
            />
          </div>

          <div className="text-xs text-text-secondary">
            {loadedPadCount} pad{loadedPadCount !== 1 ? 's' : ''} loaded
          </div>
        </div>
      </Card>

      <div className="grid grid-cols-1 lg:grid-cols-2 gap-4">
        <Card padding="sm">
          <PadGrid />
        </Card>
        <Card padding="none">
          <PadLayerEditor />
        </Card>
      </div>

      <Tooltip
        content={exportBlocked ? 'MPC Contract violations must be resolved before exporting' : ''}
        description={exportBlocked ? exportBlockedReasons.join(' · ') : undefined}
        position="top"
        disabled={!exportBlocked}
      >
        <Button
          variant="primary"
          className="w-full"
          disabled={!programName.trim() || loadedPadCount === 0 || exportBlocked}
          onClick={handleBuild}
        >
          {exportBlocked
            ? `Fix ${exportBlockedReasons.length} MPC issue(s) to export`
            : `Build Drum Program (${loadedPadCount} pads)`}
        </Button>
      </Tooltip>
    </div>
  );
}
