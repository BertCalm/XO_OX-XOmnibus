'use client';

import { useEffect, useRef } from 'react';
import { usePadStore } from '@/stores/padStore';
import { useAudioStore } from '@/stores/audioStore';
import { useProjectStore } from '@/stores/projectStore';
import { useEnvelopeStore } from '@/stores/envelopeStore';
import { useModulationStore } from '@/stores/modulationStore';
import { useExpressionStore } from '@/stores/expressionStore';
import { useToastStore } from '@/stores/toastStore';
import { persistProjectState } from '@/lib/storage/projectManager';
import { saveStoreSnapshot } from '@/lib/storage/db';

/**
 * Auto-saves project state to IndexedDB with debouncing.
 * Watches pad assignments and sample changes.
 *
 * Key safety feature: `consumeDirtySampleIds()` clears the dirty set
 * atomically. If the save then fails, those IDs would be permanently
 * lost — so we re-mark them as dirty on failure.
 */
export function useAutoSave() {
  const timerRef = useRef<NodeJS.Timeout | null>(null);
  /** Prevent spamming the user with repeated quota error toasts */
  const hasShownQuotaToastRef = useRef(false);
  const pads = usePadStore((s) => s.pads);
  const samples = useAudioStore((s) => s.samples);
  const currentProject = useProjectStore((s) => s.currentProject);
  // Subscribe to envelope/modulation/expression changes so they trigger auto-save.
  // Without these, changes to per-pad DSP settings are silently lost on reload.
  const padEnvelopes = useEnvelopeStore((s) => s.padEnvelopes);
  const padModulation = useModulationStore((s) => s.padModulation);
  const padExpressions = useExpressionStore((s) => s.padExpressions);

  useEffect(() => {
    if (!currentProject) return;

    if (timerRef.current) clearTimeout(timerRef.current);
    timerRef.current = setTimeout(async () => {
      // Read LIVE state inside the timeout — the closed-over hook values
      // from 2 seconds ago may be stale if the user made rapid changes.
      const liveProject = useProjectStore.getState().currentProject;
      if (!liveProject) return;

      // Guard: don't save while auto-restore is still hydrating stores.
      // Saving partially-hydrated state would overwrite the full project
      // with incomplete data (e.g., pads loaded but envelopes still default).
      if (useProjectStore.getState().isRestoring) return;

      const livePads = usePadStore.getState().pads;
      const liveSamples = useAudioStore.getState().samples;

      // Consume dirty sample IDs BEFORE persisting — these are samples
      // that were modified in-place by DSP tools (era mastering, normalize,
      // etc.) and need their audio data re-saved to IndexedDB.
      const dirtySampleIds = useAudioStore.getState().consumeDirtySampleIds();

      try {
        await persistProjectState(
          {
            ...liveProject,
            updatedAt: Date.now(),
            padAssignments: livePads,
            samples: liveSamples,
          },
          dirtySampleIds,
        );

        // Persist envelope/modulation/expression state alongside project
        const projectId = liveProject.id;
        await Promise.all([
          saveStoreSnapshot({
            id: `envelopes-${projectId}`,
            data: JSON.stringify(useEnvelopeStore.getState().padEnvelopes),
            projectId,
          }),
          saveStoreSnapshot({
            id: `modulations-${projectId}`,
            data: JSON.stringify(useModulationStore.getState().padModulation),
            projectId,
          }),
          saveStoreSnapshot({
            id: `expressions-${projectId}`,
            data: JSON.stringify(useExpressionStore.getState().padExpressions),
            projectId,
          }),
        ]);

        // Remember last project for auto-restore on next page load
        try { localStorage.setItem('xo_ox_lastProjectId', projectId); } catch { /* noop — localStorage unavailable */ }

        // Reset quota toast flag on successful save
        hasShownQuotaToastRef.current = false;
      } catch (error) {
        console.warn('Auto-save failed:', error);

        // Re-mark dirty sample IDs so the next save attempt picks them up.
        // Without this, consumeDirtySampleIds() already cleared them and
        // they would be permanently lost.
        // Uses reMarkDirty() instead of updateSample({}) to avoid mutating
        // the samples array — which would trigger this effect again and
        // create an infinite retry loop on persistent errors like quota.
        if (dirtySampleIds.length > 0) {
          useAudioStore.getState().reMarkDirty(dirtySampleIds);
        }

        // Show user-facing feedback for save errors
        const isQuotaError =
          error instanceof DOMException && error.name === 'QuotaExceededError';
        if (isQuotaError && !hasShownQuotaToastRef.current) {
          hasShownQuotaToastRef.current = true;
          useToastStore.getState().addToast({
            type: 'error',
            title: 'Storage full — save failed',
            message: 'Free up disk space or remove unused samples to continue saving.',
          });
        } else if (!isQuotaError && !hasShownQuotaToastRef.current) {
          // Non-quota errors (IndexedDB unavailable, corrupt, etc.)
          // Show once to avoid spamming on repeated failures
          hasShownQuotaToastRef.current = true;
          useToastStore.getState().addToast({
            type: 'error',
            title: 'Auto-save failed',
            message: error instanceof Error ? error.message : 'Changes may not persist across page reloads.',
          });
        }
      }
    }, 2000); // 2 second debounce

    return () => {
      if (timerRef.current) clearTimeout(timerRef.current);
    };
  }, [pads, samples, currentProject, padEnvelopes, padModulation, padExpressions]);
}
