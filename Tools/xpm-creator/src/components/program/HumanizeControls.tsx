'use client';

import React, { useState } from 'react';
import Slider from '@/components/ui/Slider';
import Button from '@/components/ui/Button';
import { usePadStore } from '@/stores/padStore';
import { humanizeLayers, hasStackedSamples, DEFAULT_HUMANIZE_SETTINGS } from '@/lib/audio/humanizer';
import type { HumanizeSettings } from '@/lib/audio/humanizer';

export default function HumanizeControls() {
  const pads = usePadStore((s) => s.pads);
  const activePadIndex = usePadStore((s) => s.activePadIndex);
  const updateLayer = usePadStore((s) => s.updateLayer);
  const withHistory = usePadStore((s) => s.withHistory);
  const [settings, setSettings] = useState<HumanizeSettings>(DEFAULT_HUMANIZE_SETTINGS);
  const [showAdvanced, setShowAdvanced] = useState(false);

  const pad = pads[activePadIndex];
  if (!pad) return null;

  const isStacked = hasStackedSamples(pad.layers);
  const activeLayers = pad.layers.filter((l) => l.active && l.sampleId);

  const handleApply = () => {
    withHistory('Apply humanization', () => {
      const humanized = humanizeLayers(pad.layers, settings);
      humanized.forEach((layer, i) => {
        if (layer.active && layer.sampleId) {
          updateLayer(activePadIndex, i, {
            tuneFine: layer.tuneFine,
            volume: layer.volume,
            pan: layer.pan,
            offset: layer.offset,
            pitchRandom: layer.pitchRandom,
          });
        }
      });
    });
  };

  return (
    <div className="space-y-3">
      <div className="flex items-center justify-between">
        <span className="label">Humanize</span>
        {isStacked && (
          <span className="text-[10px] bg-accent-plum-light text-accent-plum px-1.5 py-0.5 rounded-full">
            Stacked detected
          </span>
        )}
      </div>

      {activeLayers.length < 2 ? (
        <p className="text-xs text-text-muted">
          Add 2+ layers to a pad to use humanization
        </p>
      ) : (
        <>
          <Slider
            label="Amount"
            value={settings.amount}
            onChange={(v) => setSettings({ ...settings, amount: v })}
            min={0}
            max={100}
            step={1}
            unit="%"
          />

          <button
            onClick={() => setShowAdvanced(!showAdvanced)}
            className="text-xs text-accent-teal hover:text-accent-teal-dark transition-colors"
          >
            {showAdvanced ? 'Hide' : 'Show'} advanced settings
          </button>

          {showAdvanced && (
            <div className="space-y-2 pt-1">
              <Slider
                label="Tune variation"
                value={settings.tuneFineRange}
                onChange={(v) => setSettings({ ...settings, tuneFineRange: v })}
                min={0}
                max={20}
                step={1}
                unit=" cents"
              />
              <Slider
                label="Volume variation"
                value={settings.volumeRange}
                onChange={(v) => setSettings({ ...settings, volumeRange: v })}
                min={0}
                max={0.15}
                step={0.005}
              />
              <Slider
                label="Pan variation"
                value={settings.panRange}
                onChange={(v) => setSettings({ ...settings, panRange: v })}
                min={0}
                max={0.1}
                step={0.005}
              />
              <Slider
                label="Timing offset"
                value={settings.offsetRangeMs}
                onChange={(v) => setSettings({ ...settings, offsetRangeMs: v })}
                min={0}
                max={30}
                step={1}
                unit=" ms"
              />
              <Slider
                label="Per-hit pitch random"
                value={settings.pitchRandomRange}
                onChange={(v) => setSettings({ ...settings, pitchRandomRange: v })}
                min={0}
                max={0.1}
                step={0.005}
              />
            </div>
          )}

          <Button variant="accent" size="sm" onClick={handleApply} className="w-full">
            Apply Humanize
          </Button>

          {/* Preview of variations */}
          {activeLayers.length > 1 && (
            <div className="bg-surface-alt rounded-lg p-2 space-y-1">
              <p className="text-[10px] text-text-muted uppercase tracking-wider">Preview</p>
              {pad.layers.map((layer, i) => {
                if (!layer.active || !layer.sampleId) return null;
                return (
                  <div key={i} className="flex items-center gap-2 text-[10px]">
                    <span className="text-text-muted w-5">L{i + 1}</span>
                    <span className="text-text-secondary">
                      tune:{layer.tuneFine > 0 ? '+' : ''}{layer.tuneFine}
                    </span>
                    <span className="text-text-secondary">
                      vol:{layer.volume.toFixed(2)}
                    </span>
                    <span className="text-text-secondary">
                      pan:{layer.pan.toFixed(2)}
                    </span>
                    {layer.offset > 0 && (
                      <span className="text-text-secondary">
                        off:{layer.offset}
                      </span>
                    )}
                  </div>
                );
              })}
            </div>
          )}
        </>
      )}
    </div>
  );
}
