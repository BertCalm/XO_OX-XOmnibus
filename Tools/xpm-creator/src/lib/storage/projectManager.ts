import { saveProject, getProject, getAllProjects, deleteProject, saveSample, getSamplesByProject, deleteSample, getStoreSnapshots } from './db';
import type { Project, AudioSample, ProgramType } from '@/types';
import { v4 as uuid } from 'uuid';

/** Convert an in-memory AudioSample to the flat DB row format. */
function serializeSample(sample: AudioSample, projectId: string) {
  return {
    id: sample.id,
    name: sample.name,
    fileName: sample.fileName,
    duration: sample.duration,
    sampleRate: sample.sampleRate,
    channels: sample.channels,
    bitDepth: sample.bitDepth,
    buffer: sample.buffer,
    rootNote: sample.rootNote,
    createdAt: sample.createdAt,
    projectId,
    tags: JSON.stringify(sample.tags ?? []),
    isFavorite: sample.isFavorite ? 1 : 0,
    waveformPeaks: JSON.stringify(sample.waveformPeaks ?? []),
  };
}

export async function createNewProject(name: string, type: ProgramType): Promise<Project> {
  const project: Project = {
    id: uuid(),
    name,
    createdAt: Date.now(),
    updatedAt: Date.now(),
    programType: type,
    samples: [],
    padAssignments: [],
    programs: [],
  };

  await saveProject({
    id: project.id,
    name: project.name,
    programType: project.programType,
    createdAt: project.createdAt,
    updatedAt: project.updatedAt,
    padAssignmentsJson: JSON.stringify(project.padAssignments),
    programsJson: JSON.stringify(project.programs),
  });

  return project;
}

export async function loadProject(id: string): Promise<Project | null> {
  const projectData = await getProject(id);
  if (!projectData) return null;

  const dbSamples = await getSamplesByProject(id);
  const snapshots = await getStoreSnapshots(id);

  return {
    id: projectData.id,
    name: projectData.name,
    createdAt: projectData.createdAt,
    updatedAt: projectData.updatedAt,
    programType: projectData.programType,
    samples: dbSamples.map((s) => ({
      id: s.id,
      name: s.name,
      fileName: s.fileName,
      duration: s.duration,
      sampleRate: s.sampleRate,
      channels: s.channels,
      bitDepth: s.bitDepth,
      buffer: s.buffer,
      rootNote: s.rootNote,
      createdAt: s.createdAt,
      // v2 fields — default gracefully for pre-v2 rows
      tags: (() => { try { return JSON.parse(s.tags || '[]'); } catch { return []; } })(),
      isFavorite: Boolean(s.isFavorite),
      waveformPeaks: (() => { try { return JSON.parse(s.waveformPeaks || '[]'); } catch { return []; } })(),
    })),
    padAssignments: (() => { try { return JSON.parse(projectData.padAssignmentsJson || '[]'); } catch { return []; } })(),
    programs: (() => { try { return JSON.parse(projectData.programsJson || '[]'); } catch { return []; } })(),
    storeSnapshots: snapshots,
  };
}

export async function persistProjectState(
  project: Project,
  dirtySampleIds?: string[],
): Promise<void> {
  await saveProject({
    id: project.id,
    name: project.name,
    programType: project.programType,
    createdAt: project.createdAt,
    updatedAt: Date.now(),
    padAssignmentsJson: JSON.stringify(project.padAssignments),
    programsJson: JSON.stringify(project.programs),
  });

  // Persist any samples that were modified since last save (e.g., by DSP
  // tools like era mastering, normalize, etc.). Without this, in-place
  // sample modifications are lost on page reload.
  if (dirtySampleIds && dirtySampleIds.length > 0) {
    const dirtySet = new Set(dirtySampleIds);
    for (const sample of project.samples) {
      if (dirtySet.has(sample.id)) {
        await saveSample(serializeSample(sample, project.id));
      }
    }
  }
}

export async function addSampleToProject(
  projectId: string,
  sample: AudioSample
): Promise<void> {
  await saveSample(serializeSample(sample, projectId));
}

export async function removeSampleFromProject(sampleId: string): Promise<void> {
  await deleteSample(sampleId);
}

// Runtime validation set for ProgramType values from IndexedDB —
// external data may contain stale/unknown values from older app versions.
const VALID_PROGRAM_TYPES = new Set<string>(['Drum', 'Keygroup', 'Clip', 'Plugin']);
const DEFAULT_PROGRAM_TYPE: ProgramType = 'Drum';

export async function listProjects(): Promise<{ id: string; name: string; updatedAt: number; programType: ProgramType }[]> {
  const all = await getAllProjects();
  return all
    .map((p) => ({
      id: p.id,
      name: p.name,
      updatedAt: p.updatedAt,
      programType: VALID_PROGRAM_TYPES.has(p.programType)
        ? (p.programType as ProgramType)
        : DEFAULT_PROGRAM_TYPE,
    }))
    .sort((a, b) => b.updatedAt - a.updatedAt);
}

export { deleteProject };
