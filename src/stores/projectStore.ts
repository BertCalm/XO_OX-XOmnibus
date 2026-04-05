import { create } from 'zustand';
import type { Project, ProgramType } from '@/types';
import type { PadEnvelopeSettings } from '@/stores/envelopeStore';
import {
  createNewProject,
  loadProject as loadProjectFromDB,
  listProjects,
  persistProjectState,
} from '@/lib/storage/projectManager';
import { saveStoreSnapshot } from '@/lib/storage/db';
import { getProjectTemplate } from '@/constants/projectTemplates';
import { getPadMapTemplate } from '@/constants/padMapTemplates';
import { usePadStore } from '@/stores/padStore';
import { useAudioStore } from '@/stores/audioStore';
import { useEnvelopeStore } from '@/stores/envelopeStore';
import { useModulationStore } from '@/stores/modulationStore';
import { useExpressionStore } from '@/stores/expressionStore';
import { useHistoryStore } from '@/stores/historyStore';
import { usePlaybackStore } from '@/stores/playbackStore';
import { useToastStore } from '@/stores/toastStore';
import { MAX_PADS } from '@/constants/mpcDefaults';

interface ProjectListEntry {
  id: string;
  name: string;
  updatedAt: number;
  programType: ProgramType;
}

interface ProjectState {
  currentProject: Project | null;
  recentProjects: ProjectListEntry[];
  isLoading: boolean;
  /** True while useAutoRestore is hydrating stores from IndexedDB on mount */
  isRestoring: boolean;

  /** Create a new project and persist it to IndexedDB. Optionally apply a template. */
  createProject: (name: string, type: ProgramType, templateId?: string) => Promise<Project>;
  /** Update the in-memory project (does NOT auto-persist; useAutoSave handles that) */
  updateProject: (updates: Partial<Project>) => void;
  /** Replace the current project wholesale */
  setCurrentProject: (project: Project | null) => void;
  /** Load the list of projects from IndexedDB into recentProjects */
  loadProjectList: () => Promise<void>;
  /**
   * Load a project by ID from IndexedDB.
   * Returns the loaded project so callers can hydrate other stores.
   */
  loadProject: (id: string) => Promise<Project | null>;
  /** Persist the current project state to IndexedDB immediately */
  saveNow: () => Promise<void>;
  /** Set the restoring flag (used by useAutoRestore) */
  setRestoring: (v: boolean) => void;
}

export const useProjectStore = create<ProjectState>((set, get) => ({
  currentProject: null,
  recentProjects: [],
  isLoading: false,
  isRestoring: false,

  createProject: async (name, type, templateId?) => {
    set({ isLoading: true });
    try {
      // Stop any playing audio before creating a new project
      usePlaybackStore.getState().stopAll();

      // If a template is specified, resolve its programType and pad map
      const template = templateId ? getProjectTemplate(templateId) : undefined;
      const effectiveType = template?.programType ?? type;

      // Use projectManager to create and persist in one step
      const project = await createNewProject(name, effectiveType);

      // Clear all per-project stores to prevent stale data from the
      // previous project leaking into the new one.
      useEnvelopeStore.getState().clearAll();
      useModulationStore.getState().clearAll();
      useExpressionStore.getState().clearAll();
      useHistoryStore.getState().clear();

      // Reset pad store to clean defaults and clear audio samples —
      // prevents stale data from the previous project leaking into the new one.
      usePadStore.getState().initializePads(MAX_PADS);
      useAudioStore.getState().setSamples([]);

      set({ currentProject: project, isLoading: false });

      // Apply pad map template if the project template specifies one
      if (template?.padMapTemplateId) {
        const padMap = getPadMapTemplate(template.padMapTemplateId);
        if (padMap) {
          usePadStore.getState().applyPadMap(padMap.noteMap);
        }
      }
      // Refresh the recent projects list in the background
      get().loadProjectList();
      return project;
    } catch (error) {
      set({ isLoading: false });
      throw error;
    }
  },

  updateProject: (updates) => {
    const current = get().currentProject;
    if (!current) return;
    set({
      currentProject: { ...current, ...updates, updatedAt: Date.now() },
    });
  },

  setCurrentProject: (project) => set({ currentProject: project }),

  loadProjectList: async () => {
    try {
      const projects = await listProjects();
      set({ recentProjects: projects });
    } catch (error) {
      console.error('Failed to load project list:', error);
      useToastStore.getState().addToast({
        type: 'warning',
        title: 'Could not load recent projects',
        message: 'Your saved projects may not appear. Check browser storage settings.',
      });
    }
  },

  loadProject: async (id) => {
    set({ isLoading: true });
    try {
      // Stop any playing audio before switching projects
      usePlaybackStore.getState().stopAll();

      const project = await loadProjectFromDB(id);
      if (project) {
        // Clear all per-project stores — prevents stale data from the
        // previous project leaking into the loaded one.
        useEnvelopeStore.getState().clearAll();
        useModulationStore.getState().clearAll();
        useExpressionStore.getState().clearAll();
        useHistoryStore.getState().clear();

        // Hydrate audio samples from the loaded project
        useAudioStore.getState().setSamples(project.samples ?? []);

        // Hydrate pad assignments (or reset to defaults if none saved)
        if (project.padAssignments?.length) {
          usePadStore.getState().setPads(project.padAssignments);
        } else {
          usePadStore.getState().initializePads(MAX_PADS);
        }

        // Hydrate envelope/modulation/expression from store snapshots —
        // without this, loadProject loses all per-pad DSP settings.
        if (project.storeSnapshots) {
          for (const snapshot of project.storeSnapshots) {
            try {
              const data = JSON.parse(snapshot.data);
              if (snapshot.id.startsWith('envelopes')) {
                useEnvelopeStore.setState({ padEnvelopes: data as Record<number, PadEnvelopeSettings> });
              } else if (snapshot.id.startsWith('modulations')) {
                useModulationStore.getState().bulkSetModulation(data);
              } else if (snapshot.id.startsWith('expressions')) {
                useExpressionStore.getState().bulkSetExpressions(data);
              }
            } catch (e) {
              console.warn('Failed to restore store snapshot:', snapshot.id, e);
            }
          }
        }

        set({ currentProject: project, isLoading: false });
      } else {
        set({ isLoading: false });
      }
      return project;
    } catch (error) {
      set({ isLoading: false });
      console.error('Failed to load project:', error);
      // Re-throw so callers can distinguish "not found" (null) from "DB error"
      throw error;
    }
  },

  saveNow: async () => {
    const current = get().currentProject;
    if (!current) return;

    // Read live state from source-of-truth stores — the project object's
    // padAssignments/samples may be stale snapshots from load time.
    const livePads = usePadStore.getState().pads;
    const liveSamples = useAudioStore.getState().samples;
    const dirtySampleIds = useAudioStore.getState().consumeDirtySampleIds();

    try {
      await persistProjectState(
        {
          ...current,
          updatedAt: Date.now(),
          padAssignments: livePads,
          samples: liveSamples,
        },
        dirtySampleIds,
      );

      // Also persist envelope/modulation/expression snapshots — mirrors
      // the logic in useAutoSave so manual saves don't lose DSP settings.
      const projectId = current.id;
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
    } catch (error) {
      // Re-mark dirty sample IDs on failure so they're retried by auto-save
      if (dirtySampleIds.length > 0) {
        useAudioStore.getState().reMarkDirty(dirtySampleIds);
      }
      console.error('Manual save failed:', error);
      // Re-throw so UI callers can show an error toast
      throw error;
    }
  },

  setRestoring: (v) => set({ isRestoring: v }),
}));
