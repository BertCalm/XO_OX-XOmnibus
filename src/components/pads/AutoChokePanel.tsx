'use client';

import React, { useState, useMemo, useCallback } from 'react';
import { usePadStore } from '@/stores/padStore';
import { useHistoryStore } from '@/stores/historyStore';
import { suggestMuteGroups, MUTE_GROUP_NAMES, MUTE_GROUP_COLORS } from '@/lib/audio/autoChoke';
import { useToastStore } from '@/stores/toastStore';
import Button from '@/components/ui/Button';
import ConfirmDialog from '@/components/ui/ConfirmDialog';

export default function AutoChokePanel() {
  const pads = usePadStore((s) => s.pads);
  const currentBank = usePadStore((s) => s.currentBank);

  const [isOpen, setIsOpen] = useState(false);
  const [overrides, setOverrides] = useState<Record<number, number>>({});
  const [showClearConfirm, setShowClearConfirm] = useState(false);

  const suggestions = useMemo(() => suggestMuteGroups(pads), [pads]);

  // Build group summary counts using overrides where present
  const groupSummary = useMemo(() => {
    const counts: Record<number, number> = { 0: 0, 1: 0, 2: 0, 3: 0, 4: 0 };
    for (const s of suggestions) {
      const group = overrides[s.padIndex] ?? s.suggestedGroup;
      counts[group] = (counts[group] || 0) + 1;
    }
    return counts;
  }, [suggestions, overrides]);

  const handleOverride = useCallback((padIndex: number, group: number) => {
    setOverrides((prev) => ({ ...prev, [padIndex]: group }));
  }, []);

  const handleApplyAll = useCallback(() => {
    // Snapshot for undo, then batch all mute group assignments
    // Use updatePad store action instead of direct setState (CLAUDE.md rule)
    const history = useHistoryStore.getState();
    const store = usePadStore.getState();
    history.snapshot(store.pads);
    for (const s of suggestions) {
      const group = overrides[s.padIndex] ?? s.suggestedGroup;
      store.updatePad(s.padIndex, { muteGroup: group });
    }
    history.pushState('Auto-choke mute groups', usePadStore.getState().pads);
    const { addToast } = useToastStore.getState();
    addToast({ type: 'success', title: `Mute groups applied to ${suggestions.length} pads` });
    setIsOpen(false);
    setOverrides({});
  }, [suggestions, overrides]);

  const handleClearAll = useCallback(() => {
    // Snapshot for undo, then batch clear mute groups for current bank
    // Use updatePad store action instead of direct setState (CLAUDE.md rule)
    const history = useHistoryStore.getState();
    const store = usePadStore.getState();
    history.snapshot(store.pads);
    const bankOffset = currentBank * 16;
    for (let i = bankOffset; i < bankOffset + 16 && i < store.pads.length; i++) {
      if (store.pads[i].muteGroup !== 0) {
        store.updatePad(i, { muteGroup: 0 });
      }
    }
    history.pushState('Clear mute groups', usePadStore.getState().pads);
    const { addToast } = useToastStore.getState();
    addToast({ type: 'info', title: 'Mute groups cleared for current bank' });
    setOverrides({});
    setShowClearConfirm(false);
  }, [currentBank]);

  // Closed state — just the trigger button
  if (!isOpen) {
    return (
      <button
        onClick={() => setIsOpen(true)}
        disabled={suggestions.length === 0}
        className="px-2 py-1 rounded text-[10px] font-medium
          bg-surface-alt text-text-secondary hover:text-text-primary
          hover:bg-surface transition-all disabled:opacity-50
          border border-transparent hover:border-border"
        title="Auto-detect sample types and assign mute groups for choking"
      >
        🔗 Smart Link
      </button>
    );
  }

  return (
    <div className="space-y-3 p-3 bg-surface-alt rounded-lg border border-border animate-in fade-in slide-in-from-top-1 duration-150">
      {/* Header */}
      <div className="flex items-center justify-between">
        <p className="text-[10px] text-text-muted uppercase tracking-wider font-semibold">
          Auto-Choke Intelligence
        </p>
        <button
          onClick={() => { setIsOpen(false); setOverrides({}); }}
          className="text-text-muted hover:text-text-primary text-xs"
          aria-label="Close Auto-Choke panel"
        >
          ✕
        </button>
      </div>

      {/* Group summary */}
      <div className="flex flex-wrap gap-1">
        {([1, 2, 3, 4] as const).map((group) => (
          <span
            key={group}
            className={`px-1.5 py-0.5 rounded text-[9px] font-medium
              bg-surface border border-border ${MUTE_GROUP_COLORS[group]}`}
          >
            {MUTE_GROUP_NAMES[group]}: {groupSummary[group] || 0}
          </span>
        ))}
        <span className="px-1.5 py-0.5 rounded text-[9px] font-medium bg-surface border border-border text-text-muted">
          None: {groupSummary[0] || 0}
        </span>
      </div>

      {/* Pad suggestion list */}
      {suggestions.length === 0 ? (
        <p className="text-[10px] text-text-muted text-center py-2">
          No samples loaded on any pads.
        </p>
      ) : (
        <div className="space-y-1 max-h-48 overflow-y-auto pr-1">
          {suggestions.map((s) => {
            const effectiveGroup = overrides[s.padIndex] ?? s.suggestedGroup;
            return (
              <div
                key={s.padIndex}
                className="flex items-center gap-2 px-2 py-1.5 rounded bg-surface text-[10px]"
              >
                {/* Pad number */}
                <span className="font-mono text-text-muted w-5 text-right shrink-0">
                  {s.padNumber + 1}
                </span>

                {/* Sample name + category */}
                <div className="min-w-0 flex-1">
                  <p className="text-text-primary truncate leading-tight">
                    {s.sampleName}
                  </p>
                  <p className="text-text-muted leading-tight">
                    {s.category}
                  </p>
                </div>

                {/* Mute group badge */}
                <span
                  className={`px-1.5 py-0.5 rounded text-[9px] font-semibold shrink-0
                    ${MUTE_GROUP_COLORS[effectiveGroup]}`}
                >
                  {MUTE_GROUP_NAMES[effectiveGroup]}
                </span>

                {/* Manual override dropdown */}
                <select
                  value={effectiveGroup}
                  onChange={(e) => handleOverride(s.padIndex, Number(e.target.value))}
                  aria-label={`Mute group for pad ${s.padIndex + 1}`}
                  className="bg-surface-alt text-text-primary text-[10px] rounded
                    border border-border px-1 py-0.5 cursor-pointer shrink-0
                    focus:outline-none focus:border-accent-teal"
                >
                  {[0, 1, 2, 3, 4].map((g) => (
                    <option key={g} value={g}>
                      {g}: {MUTE_GROUP_NAMES[g]}
                    </option>
                  ))}
                </select>
              </div>
            );
          })}
        </div>
      )}

      {/* Action buttons */}
      <div className="flex gap-1 pt-1">
        <Button
          variant="primary"
          size="sm"
          disabled={suggestions.length === 0}
          onClick={handleApplyAll}
        >
          Apply All
        </Button>
        <Button variant="ghost" size="sm" onClick={() => setShowClearConfirm(true)}>
          Clear All
        </Button>
        <Button
          variant="ghost"
          size="sm"
          onClick={() => { setIsOpen(false); setOverrides({}); }}
        >
          Cancel
        </Button>
      </div>

      {/* Hint */}
      <p className="text-[9px] text-text-muted">
        Pads in the same mute group choke each other. Hi-hats (open/closed) are the most common use case.
      </p>

      {/* Clear All confirmation */}
      <ConfirmDialog
        open={showClearConfirm}
        onConfirm={handleClearAll}
        onCancel={() => setShowClearConfirm(false)}
        title="Clear All Mute Groups?"
        message="This will remove all mute group assignments from pads in the current bank. Pads will no longer choke each other."
        confirmLabel="Clear All"
        variant="warning"
      />
    </div>
  );
}
