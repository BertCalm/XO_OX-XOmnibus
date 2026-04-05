'use client';

import React, { useState, useCallback, useMemo, useRef } from 'react';
import { extractSonicDna } from '@/lib/xpm/sonicDnaExtractor';
import type { SonicDnaProfile } from '@/lib/xpm/sonicDnaExtractor';
import { computeDnaInjection } from '@/lib/xpm/dnaInjector';
import type { DnaApplicationMode } from '@/lib/xpm/dnaInjector';
import { parseXpmXml } from '@/lib/xpm/xpmParser';
import { extractSingleXpm } from '@/lib/xpn/xpnExtractor';
import { usePadStore } from '@/stores/padStore';
import { useEnvelopeStore } from '@/stores/envelopeStore';
import { useModulationStore } from '@/stores/modulationStore';
import { useToastStore } from '@/stores/toastStore';
import Card, { CardHeader, CardTitle } from '@/components/ui/Card';
import Button from '@/components/ui/Button';
import Slider from '@/components/ui/Slider';

// ---------------------------------------------------------------------------
// Component
// ---------------------------------------------------------------------------

export default function DnaInjectorPanel() {
  const [dna, setDna] = useState<SonicDnaProfile | null>(null);
  const [mode, setMode] = useState<DnaApplicationMode>('full');
  const [blend, setBlend] = useState(75);
  const [isDragOver, setIsDragOver] = useState(false);
  const [error, setError] = useState<string | null>(null);
  const [applied, setApplied] = useState(false);
  const [changeCount, setChangeCount] = useState(0);

  const pads = usePadStore((s) => s.pads);
  const dnaInputRef = useRef<HTMLInputElement>(null);

  // -------------------------------------------------------------------------
  // Load a DNA source file
  // -------------------------------------------------------------------------

  const loadDnaSource = useCallback(async (file: File) => {
    setError(null);
    setApplied(false);

    try {
      const ext = file.name.toLowerCase().split('.').pop();
      const buffer = await file.arrayBuffer();

      let kit;
      if (ext === 'xpm') {
        const result = await extractSingleXpm(buffer);
        kit = result.kit;
      } else if (ext === 'xpn') {
        // For XPN, just use the first program's kit as DNA source
        const { extractXpn } = await import('@/lib/xpn/xpnExtractor');
        const extracted = await extractXpn(buffer);
        if (extracted.programs.length === 0) {
          throw new Error('No programs found in XPN file');
        }
        kit = extracted.programs[0].kit;
      } else {
        throw new Error('Please drop an .xpm or .xpn file');
      }

      const profile = extractSonicDna(kit);
      setDna(profile);
    } catch (err) {
      setError(err instanceof Error ? err.message : 'Failed to extract DNA');
    }
  }, []);

  const handleDrop = useCallback(
    (e: React.DragEvent) => {
      e.preventDefault();
      setIsDragOver(false);
      const file = e.dataTransfer.files[0];
      if (file) loadDnaSource(file);
    },
    [loadDnaSource],
  );

  // -------------------------------------------------------------------------
  // Apply DNA
  // -------------------------------------------------------------------------

  const applyDna = useCallback(() => {
    if (!dna) return;

    // Identify pads with loaded samples
    const loadedIndices = pads
      .map((pad, i) => (pad.layers.some((l) => l.active && (l.sampleId || l.sampleName)) ? i : -1))
      .filter((i) => i >= 0);

    if (loadedIndices.length === 0) {
      setError('No pads have samples loaded. Import or drag samples first.');
      return;
    }

    try {
      // Get current state snapshots
      const envState = useEnvelopeStore.getState();
      const modState = useModulationStore.getState();

      const currentEnvelopes: Record<number, ReturnType<typeof envState.getEnvelope>> = {};
      const currentModulations: Record<number, ReturnType<typeof modState.getModulation>> = {};
      for (const idx of loadedIndices) {
        currentEnvelopes[idx] = envState.getEnvelope(idx);
        currentModulations[idx] = modState.getModulation(idx);
      }

      // Compute injection
      const result = computeDnaInjection(
        loadedIndices,
        currentEnvelopes,
        currentModulations,
        dna,
        { mode, blend: blend / 100 },
      );

      // Apply envelope updates
      for (const [padIdxStr, envUpdate] of Object.entries(result.envelopeUpdates)) {
        const padIdx = parseInt(padIdxStr, 10);
        envState.setVolumeEnvelope(padIdx, envUpdate.volumeEnvelope);
        envState.setFilterEnvelope(padIdx, envUpdate.filterEnvelope);
        envState.setFilterSettings(padIdx, {
          filterType: envUpdate.filterType,
          filterCutoff: envUpdate.filterCutoff,
          filterResonance: envUpdate.filterResonance,
          filterEnvAmount: envUpdate.filterEnvAmount,
        });
      }

      // Apply modulation updates
      for (const [padIdxStr, modUpdate] of Object.entries(result.modulationUpdates)) {
        const padIdx = parseInt(padIdxStr, 10);
        modState.setModulation(padIdx, modUpdate);
      }

      setChangeCount(result.changes.length);
      setApplied(true);
    } catch (err) {
      console.error('DNA injection failed:', err);
      setError(err instanceof Error ? err.message : 'DNA injection failed unexpectedly');
      useToastStore.getState().addToast({
        type: 'error',
        title: 'DNA injection failed',
        message: 'Could not apply the Sonic DNA profile. Try a different source file.',
      });
    }
  }, [dna, pads, mode, blend]);

  // -------------------------------------------------------------------------
  // Mode buttons
  // -------------------------------------------------------------------------

  const modes: { id: DnaApplicationMode; label: string; icon: string }[] = [
    { id: 'full', label: 'Full DNA', icon: '\uD83E\uDDEC' },
    { id: 'envelopes-only', label: 'Envelopes', icon: '\uD83D\uDCC8' },
    { id: 'filters-only', label: 'Filters', icon: '\uD83C\uDF9B\uFE0F' },
    { id: 'modulation-only', label: 'Modulation', icon: '\uD83C\uDFB5' },
  ];

  // -------------------------------------------------------------------------
  // Render
  // -------------------------------------------------------------------------

  return (
    <Card elevated className="w-full">
      <CardHeader>
        <CardTitle>DNA Injector</CardTitle>
        <span className="text-[10px] font-mono text-text-muted uppercase tracking-wider">
          {dna ? 'Ready' : 'Drop Source'}
        </span>
      </CardHeader>

      {error && (
        <div className="mb-3 px-3 py-2 bg-red-500/10 border border-red-500/20 rounded-lg">
          <p className="text-xs text-red-400">{error}</p>
        </div>
      )}

      {!dna ? (
        // Drop zone for DNA source
        <div
          role="button"
          tabIndex={0}
          aria-label="Drop a pro kit — extract Sonic DNA from an .xpm or .xpn file"
          onDragOver={(e) => { e.preventDefault(); setIsDragOver(true); }}
          onDragLeave={() => setIsDragOver(false)}
          onDrop={handleDrop}
          onClick={() => dnaInputRef.current?.click()}
          onKeyDown={(e) => {
            if (e.key === 'Enter' || e.key === ' ') {
              e.preventDefault();
              dnaInputRef.current?.click();
            }
          }}
          className={`flex flex-col items-center justify-center gap-3 p-8
            border-2 border-dashed rounded-xl cursor-pointer transition-all duration-200
            focus:outline-none focus:ring-2 focus:ring-accent-teal focus:ring-offset-2 focus:ring-offset-surface-bg
            ${isDragOver
              ? 'border-accent-teal bg-accent-teal/5 scale-[1.01]'
              : 'border-border hover:border-accent-teal/50 hover:bg-surface-alt/50'
            }`}
        >
          <input
            ref={dnaInputRef}
            type="file"
            accept=".xpm,.xpn"
            onChange={(e) => {
              const file = e.target.files?.[0];
              if (file) loadDnaSource(file);
              if (e.target) e.target.value = '';
            }}
            className="hidden"
          />
          <span className="text-2xl">{'\uD83E\uDDEC'}</span>
          <div className="text-center">
            <p className="text-sm font-semibold text-text-primary">Drop a Pro Kit</p>
            <p className="text-xs text-text-secondary mt-1">
              Extract Sonic DNA from an .xpm or .xpn file
            </p>
          </div>
        </div>
      ) : (
        <div className="space-y-4">
          {/* DNA Profile Summary */}
          <div className="p-3 bg-surface-alt/50 rounded-lg space-y-2">
            <div className="flex items-center justify-between">
              <span className="text-sm font-semibold text-text-primary">{dna.kitName}</span>
              <Button variant="ghost" size="sm" onClick={() => { setDna(null); setApplied(false); }}>
                Clear
              </Button>
            </div>

            <div className="grid grid-cols-2 gap-x-4 gap-y-1 text-xs text-text-secondary">
              <span>Customized: {dna.customizedCount}/{dna.totalInstruments} instruments</span>
              <span>Filter: {['Off', 'LP', 'BP', 'HP'][dna.filterType] ?? 'Off'}</span>
              <span>Cutoff: {(dna.filterCutoff * 100).toFixed(0)}%</span>
              <span>Resonance: {(dna.filterResonance * 100).toFixed(0)}%</span>
              <span>Mod routes: {dna.modulation?.routes.length ?? 0}</span>
              <span>Vel sensitivity: {dna.velocitySensitivity.toFixed(2)}</span>
            </div>

            {/* Envelope preview */}
            <div className="flex gap-4 text-[10px] font-mono text-text-muted">
              <span>Vol: A{fmtMs(dna.volumeEnvelope.attack)} D{fmtMs(dna.volumeEnvelope.decay)} S{(dna.volumeEnvelope.sustain * 100).toFixed(0)}% R{fmtMs(dna.volumeEnvelope.release)}</span>
            </div>
          </div>

          {/* Application Mode */}
          <div>
            <p className="text-xs font-medium text-text-secondary mb-2">Application Mode</p>
            <div className="grid grid-cols-4 gap-1.5">
              {modes.map((m) => (
                <button
                  key={m.id}
                  onClick={() => setMode(m.id)}
                  className={`flex flex-col items-center gap-1 p-2 rounded-lg border transition-all text-center
                    ${mode === m.id
                      ? 'border-accent-teal bg-accent-teal/5 ring-1 ring-accent-teal/20'
                      : 'border-border hover:border-accent-teal/30'
                    }`}
                >
                  <span className="text-base">{m.icon}</span>
                  <span className="text-[10px] font-medium text-text-primary">{m.label}</span>
                </button>
              ))}
            </div>
          </div>

          {/* Blend Slider */}
          <Slider
            value={blend}
            onChange={setBlend}
            min={0}
            max={100}
            step={1}
            label="Blend"
            unit="%"
          />

          {/* Apply Button */}
          {applied ? (
            <div className="flex items-center gap-2 p-3 bg-accent-teal/10 rounded-lg">
              <svg width="16" height="16" viewBox="0 0 16 16" fill="none">
                <path d="M4 8l3 3 5-5" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round" className="text-accent-teal" />
              </svg>
              <span className="text-xs font-medium text-accent-teal">
                DNA applied to {changeCount} pad{changeCount !== 1 ? 's' : ''}
              </span>
            </div>
          ) : (
            <Button variant="primary" size="md" onClick={applyDna} className="w-full">
              Apply DNA
            </Button>
          )}
        </div>
      )}
    </Card>
  );
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

function fmtMs(value: number): string {
  if (value === 0) return '0';
  if (value < 0.01) return '<1ms';
  return `${Math.round(value * 1000)}ms`;
}
