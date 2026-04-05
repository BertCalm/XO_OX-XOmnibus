import { useEffect, useRef } from 'react';
import { useAudioStore } from '@/stores/audioStore';
import { usePadStore } from '@/stores/padStore';
import { useExportStore } from '@/stores/exportStore';
import { useMilestoneStore, MILESTONES, type MilestoneId } from '@/stores/milestoneStore';
import { useToastStore } from '@/stores/toastStore';

/**
 * Subscribes to store changes and checks milestone conditions.
 * Call once at the app root (page.tsx).
 */
export function useMilestoneTracker() {
  const samples = useAudioStore((s) => s.samples);
  const pads = usePadStore((s) => s.pads);
  const isExporting = useExportStore((s) => s.isExporting);
  const overallProgress = useExportStore((s) => s.overallProgress);

  // Track previous export state to detect completion
  const wasExporting = useRef(false);

  useEffect(() => {
    const achieve = (id: MilestoneId) => {
      const isNew = useMilestoneStore.getState().checkAndAchieve(id);
      if (isNew) {
        const def = MILESTONES.find((m) => m.id === id);
        if (def) {
          useToastStore.getState().addToast({
            type: 'success',
            title: `${def.emoji} ${def.title}`,
            message: def.description,
          });
        }
      }
    };

    // First sample
    if (samples.length >= 1) achieve('first-sample');
    // Ten samples
    if (samples.length >= 10) achieve('ten-samples');

    // First pad assignment
    const hasPadContent = pads.some((p) => p.layers.some((l) => l.active && l.sampleId));
    if (hasPadContent) achieve('first-pad');

    // Full bank (any bank with all 16 pads filled)
    for (let bank = 0; bank < 8; bank++) {
      const offset = bank * 16;
      const allFilled = pads.slice(offset, offset + 16).every((p) =>
        p.layers.some((l) => l.active && l.sampleId)
      );
      if (allFilled) {
        achieve('full-bank');
        break;
      }
    }

    // First export (detect transition from exporting to done)
    if (wasExporting.current && !isExporting && overallProgress >= 100) {
      achieve('first-export');
    }
    wasExporting.current = isExporting;
  }, [samples, pads, isExporting, overallProgress]);
}
