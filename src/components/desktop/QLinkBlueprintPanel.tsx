'use client';

import React, { useState } from 'react';
import Button from '@/components/ui/Button';
import { QLINK_PRESETS, generateQLinkXml } from '@/lib/xpm/qlinkBlueprint';
import type { QLinkPreset } from '@/lib/xpm/qlinkBlueprint';
import { useToastStore } from '@/stores/toastStore';

export default function QLinkBlueprintPanel() {
  const [selectedPresetId, setSelectedPresetId] = useState<string>('performance');
  const [xmlPreview, setXmlPreview] = useState<string | null>(null);

  const selectedPreset: QLinkPreset | undefined = QLINK_PRESETS.find(
    (p) => p.id === selectedPresetId
  );

  const handleSelectPreset = (presetId: string) => {
    setSelectedPresetId(presetId);
    setXmlPreview(null);
  };

  const handleGenerateXml = () => {
    if (!selectedPreset) return;
    const xml = generateQLinkXml(selectedPreset.mappings);
    setXmlPreview(xml);
  };

  const handleCopyXml = async () => {
    if (!xmlPreview) return;
    try {
      await navigator.clipboard.writeText(xmlPreview);
      useToastStore.getState().addToast({ type: 'success', title: 'XML copied to clipboard' });
    } catch (err) {
      console.error('Failed to copy XML to clipboard:', err);
      useToastStore.getState().addToast({ type: 'error', title: 'Failed to copy to clipboard' });
    }
  };

  return (
    <div className="space-y-3 p-3 bg-surface-alt rounded-lg border border-border">
      {/* Header */}
      <div className="flex items-center gap-1.5">
        <span className="text-sm" aria-hidden="true">
          {'\uD83C\uDF9B\uFE0F'}
        </span>
        <p className="text-[10px] text-text-muted uppercase tracking-wider font-semibold">
          Q-Link Blueprint
        </p>
      </div>

      {/* Preset selector buttons */}
      <div className="flex flex-col gap-1.5">
        {QLINK_PRESETS.map((preset) => (
          <button
            key={preset.id}
            onClick={() => handleSelectPreset(preset.id)}
            className={`flex items-start gap-2 p-2 rounded-lg border text-left transition-all
              ${selectedPresetId === preset.id
                ? 'bg-accent-teal/10 border-accent-teal text-accent-teal'
                : 'bg-surface-bg border-border text-text-secondary hover:bg-surface-alt'
              }`}
          >
            <span className="text-sm shrink-0">{preset.icon}</span>
            <div className="min-w-0">
              <p className="text-[10px] font-semibold leading-tight">
                {preset.name}
              </p>
              <p className={`text-[9px] leading-tight mt-0.5 ${
                selectedPresetId === preset.id
                  ? 'text-accent-teal/70'
                  : 'text-text-muted'
              }`}>
                {preset.description}
              </p>
            </div>
          </button>
        ))}
      </div>

      {/* Knob mapping preview */}
      {selectedPreset && (
        <div className="space-y-1.5">
          <p className="text-[10px] text-text-muted uppercase tracking-wider">
            Knob Mappings
          </p>
          <div className="grid grid-cols-2 gap-1">
            {selectedPreset.mappings.map((mapping) => (
              <div
                key={mapping.knobIndex}
                className="px-2 py-1.5 rounded bg-surface-bg border border-border"
              >
                <p className="text-[10px] text-text-primary font-medium leading-tight">
                  Q{mapping.knobIndex + 1}: {mapping.label}
                </p>
                <p className="text-[8px] text-text-muted leading-tight mt-0.5">
                  {mapping.parameter}
                </p>
              </div>
            ))}
          </div>
        </div>
      )}

      {/* Generate XML button */}
      <Button
        variant="primary"
        size="sm"
        className="w-full"
        disabled={!selectedPreset}
        onClick={handleGenerateXml}
      >
        Generate XML
      </Button>

      {/* XML preview */}
      {xmlPreview && (
        <div className="space-y-1.5">
          <div className="flex items-center justify-between">
            <p className="text-[10px] text-text-muted uppercase tracking-wider">
              XML Output
            </p>
            <Button
              variant="ghost"
              size="sm"
              onClick={handleCopyXml}
            >
              Copy XML
            </Button>
          </div>
          <pre className="p-2 rounded bg-surface-bg border border-border text-[9px] text-text-secondary font-mono overflow-x-auto overflow-y-auto max-h-32 whitespace-pre">
            {xmlPreview}
          </pre>
        </div>
      )}

      {/* Footer hint */}
      <p className="text-[8px] text-text-muted">
        Q-Link mappings are embedded into XPM programs for hardware knob control on MPC devices.
      </p>
    </div>
  );
}
