import { create } from 'zustand';
import { persist } from 'zustand/middleware';

export type MilestoneId =
  | 'first-sample'
  | 'first-pad'
  | 'first-export'
  | 'ten-samples'
  | 'full-bank'
  | 'ten-projects';

interface MilestoneDefinition {
  id: MilestoneId;
  title: string;
  description: string;
  emoji: string;
}

export const MILESTONES: MilestoneDefinition[] = [
  { id: 'first-sample', title: 'First Dig', description: 'Imported your first sample', emoji: '🎤' },
  { id: 'first-pad', title: 'First Forge', description: 'Assigned a sample to a pad', emoji: '🔨' },
  { id: 'first-export', title: 'First Ship', description: 'Exported your first program', emoji: '🚀' },
  { id: 'ten-samples', title: 'Crate Digger', description: 'Imported 10 samples', emoji: '📦' },
  { id: 'full-bank', title: 'Full Bank', description: 'Filled all 16 pads in a bank', emoji: '🏆' },
  { id: 'ten-projects', title: 'Prolific', description: 'Created 10 projects', emoji: '⭐' },
];

interface MilestoneState {
  achieved: Set<MilestoneId>;
  /** Returns true if the milestone was newly achieved (first time) */
  checkAndAchieve: (id: MilestoneId) => boolean;
  isAchieved: (id: MilestoneId) => boolean;
}

export const useMilestoneStore = create<MilestoneState>()(
  persist(
    (set, get) => ({
      achieved: new Set<MilestoneId>(),

      checkAndAchieve: (id) => {
        if (get().achieved.has(id)) return false;
        set((state) => {
          const next = new Set(state.achieved);
          next.add(id);
          return { achieved: next };
        });
        return true;
      },

      isAchieved: (id) => get().achieved.has(id),
    }),
    {
      name: 'xo-ox-milestones',
      partialize: (state) => ({
        achieved: Array.from(state.achieved),
      }),
      merge: (persisted, current) => {
        const data = persisted as { achieved?: MilestoneId[] } | null;
        return {
          ...current,
          achieved: new Set(data?.achieved ?? []),
        };
      },
    }
  )
);
