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
    indexes: { 'by-project': string };
  };
}

const DB_NAME = 'xpm-creator';
const DB_VERSION = 3;

let dbInstance: IDBPDatabase<XpmCreatorDB> | null = null;
/** Cached error from a permanent open failure (e.g. private browsing). */
let dbError: Error | null = null;
/** In-flight promise prevents concurrent callers from opening duplicate connections. */
let dbPromise: Promise<IDBPDatabase<XpmCreatorDB>> | null = null;

export async function getDB(): Promise<IDBPDatabase<XpmCreatorDB>> {
  // #809 — cache permanent failures so we don't retry on every call
  if (dbError) throw dbError;
  if (dbInstance) return dbInstance;
  if (dbPromise) return dbPromise;

  dbPromise = (async () => {
    try {
      const instance = await openDB<XpmCreatorDB>(DB_NAME, DB_VERSION, {
        upgrade(db, oldVersion, _newVersion, transaction) {
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
          // v3: #806 — add projectId index on storeSnapshots to eliminate O(n)
          // table scans. Handles both fresh installs (store created above in v2
          // block) and upgrades from v2 (store already exists — access via the
          // versionchange transaction, which is the only way to modify indexes).
          if (oldVersion < 3) {
            const snapshotStore = db.objectStoreNames.contains('storeSnapshots')
              ? transaction.objectStore('storeSnapshots')
              : db.createObjectStore('storeSnapshots', { keyPath: 'id' });
            if (!snapshotStore.indexNames.contains('by-project')) {
              snapshotStore.createIndex('by-project', 'projectId', { unique: false });
            }
          }
        },
      });
      dbInstance = instance;
      return instance;
    } catch (error) {
      // #809 — cache the error so subsequent calls fail fast without retrying
      dbError = new Error(
        `IndexedDB unavailable: ${error instanceof Error ? error.message : 'unknown error'}. ` +
        'Project data cannot be saved. Check that you are not in private browsing mode.'
      );
      throw dbError;
    } finally {
      // Clear in-flight promise regardless of outcome
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

  // Delete associated store snapshots via index (O(k) not O(n))
  const snapshotStore = tx.objectStore('storeSnapshots');
  const snapshotIndex = snapshotStore.index('by-project');
  let snapshotCursor = await snapshotIndex.openCursor(id);
  while (snapshotCursor) {
    await snapshotCursor.delete();
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
  // #806 — use projectId index (O(k)) instead of full table scan (O(n))
  return db.getAllFromIndex('storeSnapshots', 'by-project', projectId);
}

export async function deleteStoreSnapshots(projectId: string): Promise<void> {
  const db = await getDB();
  // #806 — use projectId index to find only matching records (O(k) not O(n))
  const tx = db.transaction('storeSnapshots', 'readwrite');
  let cursor = await tx.store.index('by-project').openCursor(projectId);
  while (cursor) {
    await cursor.delete();
    cursor = await cursor.continue();
  }
  await tx.done;
}
