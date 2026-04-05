'use client';

import { useEffect, useRef } from 'react';
import { useAudioStore } from '@/stores/audioStore';
import { usePadStore } from '@/stores/padStore';
import { useProjectStore } from '@/stores/projectStore';
import { useEnvelopeStore } from '@/stores/envelopeStore';
import { useModulationStore } from '@/stores/modulationStore';
import { useExpressionStore } from '@/stores/expressionStore';
import { loadProject } from '@/lib/storage/projectManager';
import { useToastStore } from '@/stores/toastStore';
import type { PadEnvelopeSettings } from '@/stores/envelopeStore';

/**
 * Auto-restores the last project from IndexedDB on mount.
 * Runs once before useAutoSave kicks in, so the app resumes
 * where the user left off — like a DAW session recall.
 *
 * Hydration order matters:
 * 1. Samples first (pads reference sample IDs)
 * 2. Pads second (envelope/mod/expression reference pad indices)
 * 3. Envelope/modulation/expression stores
 * 4. currentProject LAST (triggers auto-save subscriptions)
 */
export function useAutoRestore() {
  const hasRestored = useRef(false);

  useEffect(() => {
    if (hasRestored.current) return;
    hasRestored.current = true;

    let lastProjectId: string | null = null;
    try {
      lastProjectId = localStorage.getItem('xo_ox_lastProjectId');
    } catch {
      // localStorage unavailable (private browsing, iframe restrictions)
    }
    if (!lastProjectId) return;

    const restore = async () => {
      useProjectStore.getState().setRestoring(true);
      try {
        const project = await loadProject(lastProjectId);
        if (!project) return;

        // 1. Hydrate audio samples
        useAudioStore.getState().setSamples(project.samples ?? []);

        // 2. Hydrate pad assignments
        if (project.padAssignments?.length) {
          usePadStore.getState().setPads(project.padAssignments);
        }

        // 3. Clear ALL stale envelope/modulation/expression data before hydration.
        // Without clearing modulation & expression stores, data from a previous
        // project leaks into the restored project.
        useEnvelopeStore.getState().clearAll();
        useModulationStore.getState().clearAll();
        useExpressionStore.getState().clearAll();
        if (project.storeSnapshots) {
          for (const snapshot of project.storeSnapshots) {
            try {
              const data = JSON.parse(snapshot.data);
              if (snapshot.id.startsWith('envelopes')) {
                // Bulk-set in a single setState call instead of N×3 individual
                // set calls — prevents 384 re-renders for 128 pads.
                useEnvelopeStore.setState({ padEnvelopes: data as Record<number, PadEnvelopeSettings> });
              } else if (snapshot.id.startsWith('modulations')) {
                useModulationStore.getState().bulkSetModulation(data);
              } else if (snapshot.id.startsWith('expressions')) {
                useExpressionStore.getState().bulkSetExpressions(data);
              }
            } catch (e) {
              console.warn('Failed to restore snapshot:', snapshot.id, e);
            }
          }
        }

        // 4. Set current project LAST — this triggers auto-save subscriptions.
        // We pass the full project including samples/padAssignments so the
        // auto-save effect has accurate data on its first run.
        useProjectStore.getState().setCurrentProject({
          id: project.id,
          name: project.name,
          programType: project.programType,
          createdAt: project.createdAt,
          updatedAt: project.updatedAt,
          samples: project.samples ?? [],
          padAssignments: project.padAssignments ?? [],
          programs: project.programs ?? [],
        });
      } catch (error) {
        console.error('Auto-restore failed:', error);
        // Clear stale project ID so we don't retry the same broken restore forever
        try { localStorage.removeItem('xo_ox_lastProjectId'); } catch { /* noop */ }
        useToastStore.getState().addToast({
          type: 'warning',
          title: 'Session restore failed',
          message: 'Your previous session could not be restored. Try loading the project manually.',
        });
      } finally {
        useProjectStore.getState().setRestoring(false);
      }
    };

    restore();
  }, []);
}
