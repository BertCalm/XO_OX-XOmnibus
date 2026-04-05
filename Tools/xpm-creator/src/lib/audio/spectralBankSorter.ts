/**
 * Spectral Bank Sorter — Wave 2: Spectral Intelligence
 *
 * Auto-sorts chop results into MPC pad zones based on spectral fingerprints.
 * Groups samples into 4 spectral zones across a 16-pad bank layout:
 *   - Pads  1-4  (index 0-3):   Kick zone        (dominant subBass + bass)
 *   - Pads  5-8  (index 4-7):   Snare/Clap zone  (mid-high snap)
 *   - Pads  9-12 (index 8-11):  Mid perc zone     (toms, congas, general perc)
 *   - Pads 13-16 (index 12-15): Hi-freq zone      (hats, cymbals)
 */

import type { SpectralFingerprint } from './spectralFingerprinter';
import type { SampleCategory } from './sampleCategorizer';

// ---------------------------------------------------------------------------
// Types
// ---------------------------------------------------------------------------

export interface BankSortEntry {
  originalIndex: number;
  targetPadIndex: number;
  category: SampleCategory;
  confidence: number;
}

export interface BankSortResult {
  entries: BankSortEntry[];
  /** How many samples were successfully categorized (not 'unknown'). */
  categorizedCount: number;
}

// ---------------------------------------------------------------------------
// Zone definitions
// ---------------------------------------------------------------------------

/** Category sets that belong to each pad zone. */
const KICK_ZONE_CATEGORIES: ReadonlySet<SampleCategory> = new Set<SampleCategory>([
  'kick',
  'bass',
]);

const SNARE_ZONE_CATEGORIES: ReadonlySet<SampleCategory> = new Set<SampleCategory>([
  'snare',
  'clap',
]);

const MID_PERC_ZONE_CATEGORIES: ReadonlySet<SampleCategory> = new Set<SampleCategory>([
  'tom',
  'perc',
  'keys',
  'pad',
  'lead',
  'fx',
  'vocal',
]);

const HI_FREQ_ZONE_CATEGORIES: ReadonlySet<SampleCategory> = new Set<SampleCategory>([
  'hat-closed',
  'hat-open',
  'cymbal',
]);

/** Zone start index and size (4 pads per zone, 16 total). */
interface ZoneSpec {
  startIdx: number;
  size: number;
  categories: ReadonlySet<SampleCategory>;
}

const ZONES: ZoneSpec[] = [
  { startIdx: 0,  size: 4, categories: KICK_ZONE_CATEGORIES },
  { startIdx: 4,  size: 4, categories: SNARE_ZONE_CATEGORIES },
  { startIdx: 8,  size: 4, categories: MID_PERC_ZONE_CATEGORIES },
  { startIdx: 12, size: 4, categories: HI_FREQ_ZONE_CATEGORIES },
];

const TOTAL_PADS = 16;

// ---------------------------------------------------------------------------
// Sorting
// ---------------------------------------------------------------------------

interface IndexedClassification {
  originalIndex: number;
  category: SampleCategory;
  confidence: number;
}

/**
 * Sort an array of spectral fingerprints + classification results into a
 * 16-pad MPC bank layout.
 *
 * Algorithm:
 * 1. Group samples by their spectral category into the 4 zones.
 * 2. Within each zone, sort by confidence descending.
 * 3. Assign pad indices sequentially within each zone.
 * 4. Overflow samples go to the next available empty pad.
 * 5. Uncategorized ('unknown') samples fill remaining empty pads.
 */
export function sortToBank(
  fingerprints: SpectralFingerprint[],
  categories: { category: SampleCategory; confidence: number }[],
): BankSortResult {
  const count = Math.min(fingerprints.length, categories.length);

  // Build indexed list
  const items: IndexedClassification[] = [];
  for (let i = 0; i < count; i++) {
    items.push({
      originalIndex: i,
      category: categories[i].category,
      confidence: categories[i].confidence,
    });
  }

  // Separate unknowns from categorised
  const unknowns: IndexedClassification[] = [];
  const zoneGroups: IndexedClassification[][] = [[], [], [], []];

  items.forEach((item) => {
    if (item.category === 'unknown') {
      unknowns.push(item);
      return;
    }

    let placed = false;
    for (let z = 0; z < ZONES.length; z++) {
      if (ZONES[z].categories.has(item.category)) {
        zoneGroups[z].push(item);
        placed = true;
        break;
      }
    }

    // If category doesn't match any zone explicitly, treat as unknown
    if (!placed) {
      unknowns.push(item);
    }
  });

  // Sort each zone group by confidence descending
  for (let z = 0; z < zoneGroups.length; z++) {
    zoneGroups[z].sort((a, b) => b.confidence - a.confidence);
  }

  // Track which pad indices are occupied
  const padAssigned: (BankSortEntry | null)[] = new Array(TOTAL_PADS).fill(null);
  const entries: BankSortEntry[] = [];
  const overflow: IndexedClassification[] = [];

  // Phase 1: assign within zones
  for (let z = 0; z < ZONES.length; z++) {
    const zone = ZONES[z];
    const group = zoneGroups[z];

    for (let i = 0; i < group.length; i++) {
      if (i < zone.size) {
        // Fits within zone
        const padIdx = zone.startIdx + i;
        const entry: BankSortEntry = {
          originalIndex: group[i].originalIndex,
          targetPadIndex: padIdx,
          category: group[i].category,
          confidence: group[i].confidence,
        };
        padAssigned[padIdx] = entry;
        entries.push(entry);
      } else {
        // Overflow
        overflow.push(group[i]);
      }
    }
  }

  // Phase 2: place overflow into empty pads
  // Sort overflow by confidence descending so higher-confidence samples get pads first
  overflow.sort((a, b) => b.confidence - a.confidence);

  overflow.forEach((item) => {
    const emptyIdx = findNextEmptyPad(padAssigned);
    if (emptyIdx !== -1) {
      const entry: BankSortEntry = {
        originalIndex: item.originalIndex,
        targetPadIndex: emptyIdx,
        category: item.category,
        confidence: item.confidence,
      };
      padAssigned[emptyIdx] = entry;
      entries.push(entry);
    }
  });

  // Phase 3: place unknowns into remaining empty pads
  // Sort unknowns by original index to maintain a predictable order
  unknowns.sort((a, b) => a.originalIndex - b.originalIndex);

  unknowns.forEach((item) => {
    const emptyIdx = findNextEmptyPad(padAssigned);
    if (emptyIdx !== -1) {
      const entry: BankSortEntry = {
        originalIndex: item.originalIndex,
        targetPadIndex: emptyIdx,
        category: item.category,
        confidence: item.confidence,
      };
      padAssigned[emptyIdx] = entry;
      entries.push(entry);
    }
  });

  // Sort final entries by targetPadIndex for consistent output
  entries.sort((a, b) => a.targetPadIndex - b.targetPadIndex);

  // Count categorized (non-unknown)
  let categorizedCount = 0;
  entries.forEach((e) => {
    if (e.category !== 'unknown') {
      categorizedCount++;
    }
  });

  return {
    entries,
    categorizedCount,
  };
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

/** Find the first empty pad index, or -1 if all are full. */
function findNextEmptyPad(
  padAssigned: (BankSortEntry | null)[],
): number {
  for (let i = 0; i < padAssigned.length; i++) {
    if (padAssigned[i] === null) return i;
  }
  return -1;
}
