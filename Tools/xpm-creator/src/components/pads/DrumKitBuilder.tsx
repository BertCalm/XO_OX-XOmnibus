'use client';

import React, { useState, useMemo, useCallback } from 'react';
import { usePadStore } from '@/stores/padStore';
import { useAudioStore } from '@/stores/audioStore';
import {
  categorizeMultiple,
  CATEGORY_INFO,
  MPC_DRUM_PAD_MAP,
} from '@/lib/audio/sampleCategorizer';
import type { SampleCategory, CategorizedSample } from '@/lib/audio/sampleCategorizer';
import Button from '@/components/ui/Button';
import Modal from '@/components/ui/Modal';
import ConfirmDialog from '@/components/ui/ConfirmDialog';
import { useToastStore } from '@/stores/toastStore';

// ─── Tailwind color-class → inline-safe hex mapping for pad backgrounds ──────
const CATEGORY_BG_COLORS: Record<SampleCategory, string> = {
  'kick':       'rgba(248, 113, 113, 0.15)',
  'snare':      'rgba(251, 146, 60, 0.15)',
  'clap':       'rgba(250, 204, 21, 0.15)',
  'hat-closed': 'rgba(34, 211, 238, 0.15)',
  'hat-open':   'rgba(45, 212, 191, 0.15)',
  'cymbal':     'rgba(96, 165, 250, 0.15)',
  'tom':        'rgba(192, 132, 252, 0.15)',
  'perc':       'rgba(244, 114, 182, 0.15)',
  'bass':       'rgba(129, 140, 248, 0.15)',
  'keys':       'rgba(74, 222, 128, 0.15)',
  'pad':        'rgba(56, 189, 248, 0.15)',
  'lead':       'rgba(167, 139, 250, 0.15)',
  'fx':         'rgba(251, 191, 36, 0.15)',
  'vocal':      'rgba(251, 113, 133, 0.15)',
  'unknown':    'rgba(156, 163, 175, 0.10)',
};

const CATEGORY_BORDER_COLORS: Record<SampleCategory, string> = {
  'kick':       'rgba(248, 113, 113, 0.4)',
  'snare':      'rgba(251, 146, 60, 0.4)',
  'clap':       'rgba(250, 204, 21, 0.4)',
  'hat-closed': 'rgba(34, 211, 238, 0.4)',
  'hat-open':   'rgba(45, 212, 191, 0.4)',
  'cymbal':     'rgba(96, 165, 250, 0.4)',
  'tom':        'rgba(192, 132, 252, 0.4)',
  'perc':       'rgba(244, 114, 182, 0.4)',
  'bass':       'rgba(129, 140, 248, 0.4)',
  'keys':       'rgba(74, 222, 128, 0.4)',
  'pad':        'rgba(56, 189, 248, 0.4)',
  'lead':       'rgba(167, 139, 250, 0.4)',
  'fx':         'rgba(251, 191, 36, 0.4)',
  'vocal':      'rgba(251, 113, 133, 0.4)',
  'unknown':    'rgba(156, 163, 175, 0.25)',
};

// ─── All possible categories for the manual override dropdown ────────────────
const ALL_CATEGORIES: SampleCategory[] = [
  'kick', 'snare', 'clap', 'hat-closed', 'hat-open', 'cymbal',
  'tom', 'perc', 'bass', 'keys', 'pad', 'lead', 'fx', 'vocal', 'unknown',
];

// ─── Confidence display helper ───────────────────────────────────────────────
function confidenceDot(confidence: number) {
  if (confidence >= 0.8) return { color: 'bg-green-400', label: 'high' };
  if (confidence >= 0.5) return { color: 'bg-yellow-400', label: 'medium' };
  return { color: 'bg-red-400', label: 'low' };
}

// ─── Truncate sample name ────────────────────────────────────────────────────
function truncateName(name: string, maxLen = 24): string {
  if (name.length <= maxLen) return name;
  return name.slice(0, maxLen - 1) + '\u2026';
}

// ─── Shuffle an array (Fisher-Yates) ─────────────────────────────────────────
function shuffleArray<T>(arr: T[]): T[] {
  const result = [...arr];
  for (let i = result.length - 1; i > 0; i--) {
    const j = Math.floor(Math.random() * (i + 1));
    [result[i], result[j]] = [result[j], result[i]];
  }
  return result;
}

// ─── Main Component ──────────────────────────────────────────────────────────

export default function DrumKitBuilder() {
  const [isOpen, setIsOpen] = useState(false);
  const [showConfirm, setShowConfirm] = useState(false);

  // Options
  const [autoVelocityLayers, setAutoVelocityLayers] = useState(true);
  const [setTriggerModes, setSetTriggerModes] = useState(true);

  // Manual overrides: sampleId -> overridden category
  const [overrides, setOverrides] = useState<Record<string, SampleCategory>>({});

  // Shuffle seed — increment to re-randomize
  const [shuffleSeed, setShuffleSeed] = useState(0);

  // Store hooks
  const samples = useAudioStore((s) => s.samples);
  const assignSampleToLayer = usePadStore((s) => s.assignSampleToLayer);
  const clearPad = usePadStore((s) => s.clearPad);
  const setPadPlayMode = usePadStore((s) => s.setPadPlayMode);
  const setPadTriggerMode = usePadStore((s) => s.setPadTriggerMode);
  const updateLayer = usePadStore((s) => s.updateLayer);
  const withHistory = usePadStore((s) => s.withHistory);

  // ── Categorize all loaded samples ────────────────────────────────────────
  const categorized = useMemo<CategorizedSample[]>(() => {
    if (samples.length === 0) return [];

    const raw = categorizeMultiple(
      samples.map((s) => ({ id: s.id, fileName: s.fileName || s.name }))
    );

    // Apply manual overrides
    return raw.map((c) => {
      if (overrides[c.sampleId]) {
        return { ...c, category: overrides[c.sampleId], confidence: 1.0 };
      }
      return c;
    });
  }, [samples, overrides]);

  // Sort by category for display
  const sortedCategorized = useMemo(() => {
    return [...categorized].sort((a, b) => {
      const catOrder = ALL_CATEGORIES.indexOf(a.category) - ALL_CATEGORIES.indexOf(b.category);
      if (catOrder !== 0) return catOrder;
      return a.sampleId.localeCompare(b.sampleId);
    });
  }, [categorized]);

  // ── Auto-assign with optional shuffle ────────────────────────────────────
  const padAssignments = useMemo(() => {
    if (categorized.length === 0) return new Map<number, string[]>();

    // Group samples by category
    const byCategory = new Map<SampleCategory, CategorizedSample[]>();
    for (const c of categorized) {
      const existing = byCategory.get(c.category) || [];
      existing.push(c);
      byCategory.set(c.category, existing);
    }

    // Shuffle within each category group when shuffleSeed changes
    if (shuffleSeed > 0) {
      Array.from(byCategory.entries()).forEach(([cat, group]) => {
        byCategory.set(cat, shuffleArray(group));
      });
    }

    // Build pad -> sampleId[] map (multiple samples per pad for velocity layers)
    const result = new Map<number, string[]>();

    for (const padSlot of MPC_DRUM_PAD_MAP) {
      const matchingSamples: CategorizedSample[] = [];
      for (const cat of padSlot.categories) {
        const group = byCategory.get(cat) || [];
        matchingSamples.push(...group);
      }

      if (matchingSamples.length > 0) {
        // Deduplicate
        const uniqueIds = Array.from(new Set(matchingSamples.map((s) => s.sampleId)));
        result.set(padSlot.padIndex, uniqueIds);
      }
    }

    // If autoVelocityLayers is off, keep only first sample per pad
    if (!autoVelocityLayers) {
      Array.from(result.entries()).forEach(([padIdx, sampleIds]) => {
        result.set(padIdx, [sampleIds[0]]);
      });
    }

    return result;
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [categorized, autoVelocityLayers, shuffleSeed]);

  // ── Get sample info by ID ────────────────────────────────────────────────
  const getSample = useCallback(
    (id: string) => samples.find((s) => s.id === id),
    [samples]
  );

  // ── Override handler ─────────────────────────────────────────────────────
  const handleOverride = useCallback((sampleId: string, newCategory: SampleCategory) => {
    setOverrides((prev) => ({ ...prev, [sampleId]: newCategory }));
  }, []);

  // ── Shuffle handler ──────────────────────────────────────────────────────
  const handleShuffle = useCallback(() => {
    setShuffleSeed((s) => s + 1);
  }, []);

  // ── Build Kit action ─────────────────────────────────────────────────────
  const handleBuildKit = useCallback(() => {
    withHistory('Build drum kit', () => {
      // 1. Clear all current pad assignments
      for (let i = 0; i < 16; i++) {
        clearPad(i);
      }

      // 2. Assign samples to pads
      const entries = Array.from(padAssignments.entries());
      for (let e = 0; e < entries.length; e++) {
        const padIdx = entries[e][0];
        const sampleIds = entries[e][1];
        if (padIdx >= 16) continue;

        const maxLayers = Math.min(sampleIds.length, 8); // Max 8 layers per pad

        for (let layerIdx = 0; layerIdx < maxLayers; layerIdx++) {
          const sampleId = sampleIds[layerIdx];
          const sample = getSample(sampleId);
          if (!sample) continue;

          assignSampleToLayer(
            padIdx,
            layerIdx,
            sample.id,
            sample.name,
            sample.fileName,
            sample.rootNote
          );

          // 3. If velocity layers, set velocity splits (gap-free, overlap-free)
          if (autoVelocityLayers && maxLayers > 1) {
            const velStart = Math.round(layerIdx * (128 / maxLayers));
            const velEnd = layerIdx === maxLayers - 1 ? 127 : Math.round((layerIdx + 1) * (128 / maxLayers)) - 1;

            updateLayer(padIdx, layerIdx, { velStart, velEnd });
          }
        }

        // Set play mode to velocity if multiple layers stacked
        if (autoVelocityLayers && maxLayers > 1) {
          setPadPlayMode(padIdx, 'velocity');
        }
      }

      // 4. Set trigger modes if enabled
      if (setTriggerModes) {
        for (const padSlot of MPC_DRUM_PAD_MAP) {
          if (padSlot.padIndex >= 16) continue;
          const assignedSamples = padAssignments.get(padSlot.padIndex);
          if (!assignedSamples || assignedSamples.length === 0) continue;

          // Hi-hats -> noteoff, kicks/snares/claps -> oneshot
          if (padSlot.categories.includes('hat-closed') || padSlot.categories.includes('hat-open')) {
            setPadTriggerMode(padSlot.padIndex, 'noteoff');
          } else if (
            padSlot.categories.includes('kick') ||
            padSlot.categories.includes('snare') ||
            padSlot.categories.includes('clap')
          ) {
            setPadTriggerMode(padSlot.padIndex, 'oneshot');
          }
        }
      }
    });

    // Show summary toast
    const assignedCount = padAssignments.size;
    const totalSamples = Array.from(padAssignments.values()).reduce((acc, ids) => acc + ids.length, 0);
    useToastStore.getState().addToast({
      type: 'success',
      title: `Kit built: ${totalSamples} samples → ${assignedCount} pads`,
    });

    setIsOpen(false);
  }, [
    padAssignments, getSample, autoVelocityLayers, setTriggerModes,
    clearPad, assignSampleToLayer, updateLayer, setPadPlayMode, setPadTriggerMode, withHistory,
  ]);

  // ── Stats ────────────────────────────────────────────────────────────────
  const stats = useMemo(() => {
    const assigned = padAssignments.size;
    const totalSamplesAssigned = Array.from(padAssignments.values()).reduce(
      (acc, ids) => acc + ids.length, 0
    );
    // Pre-compute Set for O(1) lookup instead of O(n) .flat().includes() per item
    const assignedIdSet = new Set(Array.from(padAssignments.values()).flat());
    const unassigned = categorized.filter(
      (c) => !assignedIdSet.has(c.sampleId)
    ).length;
    return { assigned, totalSamplesAssigned, unassigned };
  }, [padAssignments, categorized]);

  // ── Pad grid layout (4x4, bottom-to-top like MPC) ───────────────────────
  // MPC pads are traditionally laid out bottom-to-top:
  // Row 4 (top):    13 14 15 16
  // Row 3:           9 10 11 12
  // Row 2:           5  6  7  8
  // Row 1 (bottom):  1  2  3  4
  const padGridRows = useMemo(() => {
    return [
      [12, 13, 14, 15], // Top row (pads 13-16)
      [8,  9, 10, 11],  // Row 3
      [4,  5,  6,  7],  // Row 2
      [0,  1,  2,  3],  // Bottom row (pads 1-4)
    ];
  }, []);

  // ── Trigger button ─────────────────────────────────────────────────────
  if (!isOpen) {
    return (
      <button
        onClick={() => setIsOpen(true)}
        className="px-3 py-2 rounded-lg text-xs font-semibold
          bg-gradient-to-r from-red-500/10 to-orange-500/10
          text-text-secondary hover:text-text-primary
          hover:from-red-500/20 hover:to-orange-500/20
          transition-all border border-transparent hover:border-border"
        title="Auto-detect sample categories and build a drum kit with one click"
      >
        Kit Builder
      </button>
    );
  }

  // ── Modal Content ──────────────────────────────────────────────────────
  return (
    <Modal open={isOpen} onClose={() => setIsOpen(false)} size="xl">
      <div className="space-y-4 max-h-[75vh] overflow-y-auto pr-1">
        {/* ── Header ────────────────────────────────────────────────────── */}
        <div className="flex items-center justify-between">
          <div>
            <h2 className="text-base font-bold text-text-primary">
              One-Touch Drum Kit
            </h2>
            <p className="text-[10px] text-text-muted mt-0.5">
              Auto-detect sample categories and assign to MPC pad layout
            </p>
          </div>
          <div className="flex items-center gap-2 text-[10px] text-text-muted">
            <span>{samples.length} sample{samples.length !== 1 ? 's' : ''} loaded</span>
            {stats.assigned > 0 && (
              <span className="text-accent-teal">
                {stats.assigned} pad{stats.assigned !== 1 ? 's' : ''} assigned
              </span>
            )}
          </div>
        </div>

        {/* ── Empty state ────────────────────────────────────────────── */}
        {samples.length === 0 && (
          <div className="text-center py-8">
            <p className="text-sm text-text-muted">No samples loaded</p>
            <p className="text-[10px] text-text-muted mt-1">
              Import audio samples first, then use the Kit Builder to auto-assign them to pads
            </p>
          </div>
        )}

        {/* ── Sample Detection Panel ─────────────────────────────────── */}
        {samples.length > 0 && (
          <div className="space-y-2">
            <p className="text-[10px] text-text-muted uppercase tracking-wider font-semibold">
              Detected Categories
            </p>
            <div className="space-y-1 max-h-48 overflow-y-auto rounded-lg border border-border p-2 bg-surface">
              {sortedCategorized.map((cat) => {
                const sample = getSample(cat.sampleId);
                if (!sample) return null;
                const info = CATEGORY_INFO[cat.category];
                const conf = confidenceDot(cat.confidence);
                const isOverridden = !!overrides[cat.sampleId];

                return (
                  <div
                    key={cat.sampleId}
                    className="flex items-center gap-2 px-2 py-1.5 rounded-md hover:bg-surface-alt transition-colors group"
                  >
                    {/* Sample name */}
                    <span className="text-[10px] text-text-primary truncate flex-1 min-w-0">
                      {truncateName(sample.name)}
                    </span>

                    {/* Category badge */}
                    <span
                      className={`inline-flex items-center gap-1 px-1.5 py-0.5 rounded text-[9px]
                        font-medium ${info.color} shrink-0`}
                      style={{
                        backgroundColor: CATEGORY_BG_COLORS[cat.category],
                      }}
                    >
                      <span>{info.icon}</span>
                      <span>{info.label}</span>
                    </span>

                    {/* Confidence dot */}
                    <span
                      className={`w-1.5 h-1.5 rounded-full shrink-0 ${conf.color}`}
                      title={`Confidence: ${conf.label} (${Math.round(cat.confidence * 100)}%)`}
                    />

                    {/* Manual override indicator */}
                    {isOverridden && (
                      <span className="text-[8px] text-accent-teal shrink-0" title="Manually overridden">
                        (edited)
                      </span>
                    )}

                    {/* Override dropdown */}
                    <select
                      value={cat.category}
                      onChange={(e) => handleOverride(cat.sampleId, e.target.value as SampleCategory)}
                      aria-label={`Category override for ${cat.sampleId}`}
                      className="text-[9px] bg-transparent border border-transparent hover:border-border
                        rounded px-1 py-0.5 text-text-muted cursor-pointer shrink-0
                        opacity-0 group-hover:opacity-100 group-focus-within:opacity-100 transition-opacity
                        focus:opacity-100 focus:border-border focus:ring-1 focus:ring-accent-teal"
                      title="Override category"
                    >
                      {ALL_CATEGORIES.map((c) => (
                        <option key={c} value={c}>
                          {CATEGORY_INFO[c].icon} {CATEGORY_INFO[c].label}
                        </option>
                      ))}
                    </select>
                  </div>
                );
              })}
            </div>
          </div>
        )}

        {/* ── Pad Layout Preview ─────────────────────────────────────── */}
        {samples.length > 0 && (
          <div className="space-y-2">
            <div className="flex items-center justify-between">
              <p className="text-[10px] text-text-muted uppercase tracking-wider font-semibold">
                Pad Layout Preview
              </p>
              <span className="text-[8px] text-text-muted italic">Bank A · Pads 1–16</span>
            </div>
            <div className="grid grid-cols-4 gap-1.5">
              {padGridRows.flat().map((padIdx) => {
                const padSlot = MPC_DRUM_PAD_MAP.find((p) => p.padIndex === padIdx);
                if (!padSlot) return <div key={padIdx} />;

                const assignedIds = padAssignments.get(padIdx) || [];
                const hasAssignment = assignedIds.length > 0;
                const primaryCategory = padSlot.categories[0];
                const bgColor = hasAssignment
                  ? CATEGORY_BG_COLORS[primaryCategory]
                  : 'rgba(156, 163, 175, 0.05)';
                const borderColor = hasAssignment
                  ? CATEGORY_BORDER_COLORS[primaryCategory]
                  : 'rgba(156, 163, 175, 0.15)';

                // Get first assigned sample name
                const firstSample = assignedIds.length > 0 ? getSample(assignedIds[0]) : null;

                return (
                  <div
                    key={padIdx}
                    className={`relative rounded-lg p-2 min-h-[60px] flex flex-col justify-between
                      transition-all ${hasAssignment ? 'ring-1' : ''}`}
                    style={{
                      backgroundColor: bgColor,
                      borderColor: borderColor,
                      border: `1px solid ${borderColor}`,
                      ...(hasAssignment ? { boxShadow: `0 0 8px ${borderColor}` } : {}),
                    }}
                  >
                    {/* Pad number */}
                    <div className="flex items-start justify-between">
                      <span className="text-[8px] font-bold text-text-muted">
                        {padIdx + 1}
                      </span>
                      {assignedIds.length > 1 && (
                        <span
                          className="text-[8px] font-bold px-1 py-0.5 rounded-full bg-accent-teal/20 text-accent-teal"
                          title={`${assignedIds.length} velocity layers`}
                        >
                          x{assignedIds.length}
                        </span>
                      )}
                    </div>

                    {/* Category label */}
                    <div className="text-center">
                      <span className={`text-[9px] font-semibold ${CATEGORY_INFO[primaryCategory].color}`}>
                        {padSlot.label}
                      </span>
                    </div>

                    {/* Assigned sample name */}
                    <div className="min-h-[12px]">
                      {firstSample ? (
                        <p className="text-[8px] text-text-secondary truncate text-center" title={firstSample.name}>
                          {truncateName(firstSample.name, 16)}
                        </p>
                      ) : (
                        <p className="text-[8px] text-text-muted text-center italic">empty</p>
                      )}
                    </div>
                  </div>
                );
              })}
            </div>
          </div>
        )}

        {/* ── Build Controls ─────────────────────────────────────────── */}
        {samples.length > 0 && (
          <div className="space-y-3 pt-1">
            {/* Option Toggles */}
            <div className="space-y-2">
              <p className="text-[10px] text-text-muted uppercase tracking-wider font-semibold">
                Options
              </p>

              <label className="flex items-center gap-2 cursor-pointer group">
                <input
                  type="checkbox"
                  checked={autoVelocityLayers}
                  onChange={(e) => setAutoVelocityLayers(e.target.checked)}
                  className="w-3.5 h-3.5 rounded border-border text-accent-teal
                    focus:ring-accent-teal focus:ring-offset-0 cursor-pointer"
                />
                <div>
                  <span className="text-[10px] text-text-primary group-hover:text-accent-teal transition-colors">
                    Auto velocity layers
                  </span>
                  <p className="text-[8px] text-text-muted">
                    Stack multiple kicks/snares on the same pad with velocity splits
                  </p>
                </div>
              </label>

              <label className="flex items-center gap-2 cursor-pointer group">
                <input
                  type="checkbox"
                  checked={setTriggerModes}
                  onChange={(e) => setSetTriggerModes(e.target.checked)}
                  className="w-3.5 h-3.5 rounded border-border text-accent-teal
                    focus:ring-accent-teal focus:ring-offset-0 cursor-pointer"
                />
                <div>
                  <span className="text-[10px] text-text-primary group-hover:text-accent-teal transition-colors">
                    Set trigger modes
                  </span>
                  <p className="text-[8px] text-text-muted">
                    Hi-hats to note-off, kicks/snares to one-shot
                  </p>
                </div>
              </label>
            </div>

            {/* Summary */}
            {stats.assigned > 0 && (
              <div className="flex items-center gap-3 text-[9px] p-2 rounded-lg bg-surface border border-border">
                <span className="text-text-muted">
                  <span className="text-accent-teal font-bold">{stats.totalSamplesAssigned}</span> samples
                  {' '}&rarr;{' '}
                  <span className="text-accent-teal font-bold">{stats.assigned}</span> pads
                </span>
                {stats.unassigned > 0 && (
                  <span className="text-text-muted">
                    (<span className="text-yellow-400">{stats.unassigned}</span> unassigned)
                  </span>
                )}
              </div>
            )}

            {/* Action Buttons */}
            <div className="flex items-center gap-2">
              <Button
                variant="primary"
                size="sm"
                onClick={() => setShowConfirm(true)}
                disabled={stats.assigned === 0}
              >
                Build Kit
              </Button>

              <Button
                variant="secondary"
                size="sm"
                onClick={handleShuffle}
                disabled={categorized.length === 0}
              >
                Shuffle
              </Button>

              <div className="flex-1" />

              <Button
                variant="ghost"
                size="sm"
                onClick={() => {
                  setOverrides({});
                  setShuffleSeed(0);
                }}
              >
                Reset
              </Button>

              <Button
                variant="ghost"
                size="sm"
                onClick={() => setIsOpen(false)}
              >
                Cancel
              </Button>
            </div>
          </div>
        )}
      </div>

      {/* Build Kit Confirmation */}
      <ConfirmDialog
        open={showConfirm}
        onConfirm={() => {
          setShowConfirm(false);
          handleBuildKit();
        }}
        onCancel={() => setShowConfirm(false)}
        title="Build Drum Kit?"
        message="This will clear all existing pad assignments in Bank A (pads 1–16) and replace them with the auto-detected kit layout."
        confirmLabel="Build Kit"
        variant="warning"
      />
    </Modal>
  );
}
