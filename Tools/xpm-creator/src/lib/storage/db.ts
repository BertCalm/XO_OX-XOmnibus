import { openDB, type DBSchema, type IDBPDatabase } from 'idb';

interface XpmCreatorDB extends DBSchema {
  samples: {
    key: string;
    value: {
      id: string;
      name: string;
      fileName: string;
      duration: number;
      sampleRate: number;
      channels: number;
      bitDepth: number;
      buffer: ArrayBuffer;
      rootNote: number;
      createdAt: number;
      projectId: string;
      tags: string;           // JSON-stringified string[]
      isFavorite: number;     // 0 or 1 (IndexedDB doesn't support boolean indexes)
      waveformPeaks: string;  // JSON-stringified number[]
    };
    indexes: { 'by-project': string };
  };
  projects: {
    key: string;
    value: {
      id: string;
      name: string;
      programType: 'Keygroup' | 'Drum';
      createdAt: number;
      updatedAt: number;
      padAssignmentsJson: string;
      programsJson: string;
    };
  };
  storeSnapshots: {
    key: string;
    value: {
      id: string;       // e.g. 'envelopes-<projectId>'
      data: string;     // JSON-stringified store state
      projectId: string;
    };
  };
}

const DB_NAME = 'xpm-creator';
const DB_VERSION = 2;

let dbInstance: IDBPDatabase<XpmCreatorDB> | null = null;
/** In-flight promise prevents concurrent callers from opening duplicate connections. */
let dbPromise: Promise<IDBPDatabase<XpmCreatorDB>> | null = null;

export async function getDB(): Promise<IDBPDatabase<XpmCreatorDB>> {
  if (dbInstance) return dbInstance;
  if (dbPromise) return dbPromise;

  dbPromise = (async () => {
    try {
      const instance = await openDB<XpmCreatorDB>(DB_NAME, DB_VERSION, {
        upgrade(db, oldVersion) {
          // v1: samples + projects stores
          if (oldVersion < 1) {
            if (!db.objectStoreNames.contains('samples')) {
              const sampleStore = db.createObjectStore('samples', { keyPath: 'id' });
              sampleStore.createIndex('by-project', 'projectId');
            }
            if (!db.objectStoreNames.contains('projects')) {
              db.createObjectStore('projects', { keyPath: 'id' });
            }
          }
          // v2: storeSnapshots for envelope/modulation/expression persistence.
          // Existing sample rows gain tags/isFavorite/waveformPeaks fields
          // automatically — IndexedDB is schemaless, so old rows just lack
          // the new fields. We handle missing fields with defaults at read time.
          if (oldVersion < 2) {
            if (!db.objectStoreNames.contains('storeSnapshots')) {
              db.createObjectStore('storeSnapshots', { keyPath: 'id' });
            }
          }
        },
      });
      dbInstance = instance;
      return instance;
    } catch (error) {
      // IndexedDB may be unavailable (private browsing, disabled, corrupt)
      throw new Error(
        `IndexedDB unavailable: ${error instanceof Error ? error.message : 'unknown error'}. ` +
        'Project data cannot be saved. Check that you are not in private browsing mode.'
      );
    } finally {
      // Clear in-flight promise so a failed attempt can be retried
      dbPromise = null;
    }
  })();

  return dbPromise;
}

export async function saveSample(sample: XpmCreatorDB['samples']['value']): Promise<void> {
  const db = await getDB();
  await db.put('samples', sample);
}

export async function getSample(id: string): Promise<XpmCreatorDB['samples']['value'] | undefined> {
  const db = await getDB();
  return db.get('samples', id);
}

export async function getSamplesByProject(projectId: string): Promise<XpmCreatorDB['samples']['value'][]> {
  const db = await getDB();
  return db.getAllFromIndex('samples', 'by-project', projectId);
}

export async function deleteSample(id: string): Promise<void> {
  const db = await getDB();
  await db.delete('samples', id);
}

export async function saveProject(project: XpmCreatorDB['projects']['value']): Promise<void> {
  const db = await getDB();
  await db.put('projects', project);
}

export async function getProject(id: string): Promise<XpmCreatorDB['projects']['value'] | undefined> {
  const db = await getDB();
  return db.get('projects', id);
}

export async function getAllProjects(): Promise<XpmCreatorDB['projects']['value'][]> {
  const db = await getDB();
  return db.getAll('projects');
}

export async function deleteProject(id: string): Promise<void> {
  const db = await getDB();

  // Use a single transaction across all three stores for atomicity.
  // If any delete fails, the entire transaction is rolled back — no
  // partial deletion (e.g., samples deleted but project row remains).
  const tx = db.transaction(['samples', 'projects', 'storeSnapshots'], 'readwrite');

  // Delete associated samples
  const sampleStore = tx.objectStore('samples');
  const sampleIndex = sampleStore.index('by-project');
  let cursor = await sampleIndex.openCursor(id);
  while (cursor) {
    await cursor.delete();
    cursor = await cursor.continue();
  }

  // Delete associated store snapshots
  const snapshotStore = tx.objectStore('storeSnapshots');
  let snapshotCursor = await snapshotStore.openCursor();
  while (snapshotCursor) {
    if (snapshotCursor.value.projectId === id) {
      await snapshotCursor.delete();
    }
    snapshotCursor = await snapshotCursor.continue();
  }

  // Delete project
  await tx.objectStore('projects').delete(id);

  await tx.done;
}

// ── Store Snapshots (envelope/modulation/expression persistence) ──

export async function saveStoreSnapshot(
  snapshot: XpmCreatorDB['storeSnapshots']['value'],
): Promise<void> {
  const db = await getDB();
  await db.put('storeSnapshots', snapshot);
}

export async function getStoreSnapshots(
  projectId: string,
): Promise<XpmCreatorDB['storeSnapshots']['value'][]> {
  const db = await getDB();
  const all = await db.getAll('storeSnapshots');
  return all.filter((s) => s.projectId === projectId);
}

export async function deleteStoreSnapshots(projectId: string): Promise<void> {
  const db = await getDB();
  const all = await db.getAll('storeSnapshots');
  const tx = db.transaction('storeSnapshots', 'readwrite');
  for (const s of all) {
    if (s.projectId === projectId) {
      await tx.store.delete(s.id);
    }
  }
  await tx.done;
}
