/**
 * XPN Spec — TypeScript bridge to the canonical velocity zone definitions.
 *
 * SOURCE OF TRUTH: Tools/xpn-spec.json
 * The JSON file is the canonical record shared between the Python Oxport pipeline
 * (xpn_velocity_standard.py) and this TypeScript UI.  The values below mirror
 * that file exactly.  If zone boundaries or render midpoints change, update
 * Tools/xpn-spec.json first, then mirror the values in the CANONICAL_ZONES
 * table below.
 *
 * All velocity zone logic in xpm-creator MUST derive from this module —
 * never hardcode zone boundaries elsewhere.
 *
 * Direct JSON import is intentionally avoided: the spec file lives outside
 * the Next.js project root (Tools/xpn-spec.json vs Tools/xpm-creator/),
 * which prevents TypeScript from resolving it as a module import.
 */

// ---- Velocity zone types ----

export interface VelocityZone {
  name: string;
  /** Inclusive lower bound (MPC VelStart). */
  low: number;
  /** Inclusive upper bound (MPC VelEnd). */
  high: number;
  /** Suggested MIDI velocity for offline rendering of this layer. */
  renderMidpoint: number;
}

/**
 * Canonical 4-zone table.
 * Mirrors Tools/xpn-spec.json "velocity.zones" exactly.
 * Ghost(1-20) · Light(21-55) · Medium(56-90) · Hard(91-127)
 * Adopted 2026-04-04, Ghost Council QDD Level 2, 8/8 ghosts.
 */
const CANONICAL_ZONES: VelocityZone[] = [
  { name: 'Ghost',  low:  1, high:  20, renderMidpoint:  10 },
  { name: 'Light',  low: 21, high:  55, renderMidpoint:  38 },
  { name: 'Medium', low: 56, high:  90, renderMidpoint:  73 },
  { name: 'Hard',   low: 91, high: 127, renderMidpoint: 109 },
];

/**
 * Return velocity zones for a given number of layers.
 *
 * - 4 layers: Ghost · Light · Medium · Hard  (canonical spec zones)
 * - 3 layers: Soft(1-55) · Medium(56-90) · Hard(91-127)
 *             Ghost and Light collapsed into a single Soft zone.
 * - 2 layers: Soft(1-55) · Hard(56-127)
 *             Two musical halves: quiet and powerful.
 * - 1 layer:  Full(1-127)
 *
 * For counts > 4, zones are divided evenly across 1-127 using the canonical
 * top boundary (127) and bottom boundary (1).
 */
export function getVelocityZones(numLayers: number): VelocityZone[] {
  if (numLayers <= 0) return [];

  if (numLayers === 1) {
    return [{ name: 'Full', low: 1, high: 127, renderMidpoint: 64 }];
  }

  if (numLayers === 2) {
    // Soft spans Ghost + Light zones; Hard spans Medium + Hard zones.
    return [
      { name: 'Soft', low:  1, high:  55, renderMidpoint: 28 },
      { name: 'Hard', low: 56, high: 127, renderMidpoint: 91 },
    ];
  }

  if (numLayers === 3) {
    // Soft collapses Ghost + Light; Medium and Hard stay canonical.
    return [
      { name: 'Soft',   low:  1, high:  55, renderMidpoint:  28 },
      { name: 'Medium', low: 56, high:  90, renderMidpoint:  73 },
      { name: 'Hard',   low: 91, high: 127, renderMidpoint: 109 },
    ];
  }

  if (numLayers === 4) {
    return CANONICAL_ZONES;
  }

  // For counts > 4: divide 1-127 evenly (126 values across numLayers buckets).
  const zones: VelocityZone[] = [];
  const total = 127; // values 1..127
  for (let i = 0; i < numLayers; i++) {
    const low = Math.round(1 + (i * total) / numLayers);
    const high = i === numLayers - 1
      ? 127
      : Math.round(1 + ((i + 1) * total) / numLayers) - 1;
    const renderMidpoint = Math.round((low + high) / 2);
    zones.push({ name: `Layer ${i + 1}`, low, high, renderMidpoint });
  }
  return zones;
}

// ---- Golden rules (MPC XPM format invariants) ----

/**
 * MPC XPM format constants that must never change.
 * Mirrors the critical rules documented in Tools/xpn-spec.json,
 * xpn_velocity_standard.py, and CLAUDE.md.
 */
export const GOLDEN_RULES = {
  /** Drum programs: KeyTrack = False (pads play at fixed pitch). */
  keyTrackDrum: false,
  /** Keygroup programs: KeyTrack = True (MPC transposes across zones). */
  keyTrackKeygroup: true,
  /**
   * Keygroup programs: RootNote = 0 (MPC auto-detect convention).
   * Pre-rendered pitch-shifted samples use rootNote=0 so MPC picks up the
   * embedded tuning rather than applying additional transposition.
   */
  rootNote: 0,
  /**
   * Empty (inactive) layers: VelStart = 0.
   * Prevents MPC from ghost-triggering silent layers when velocity = 0.
   */
  emptyLayerVelStart: 0,
} as const;

// ---- Tier constants (mirrors Tools/xpn-spec.json "tiers") ----

export interface TierConfig {
  maxSlots: number;
  velLayers: number;
  roundRobin: boolean;
}

export const TIERS: Record<string, TierConfig> = {
  SURFACE: { maxSlots: 16,  velLayers: 1, roundRobin: false },
  DEEP:    { maxSlots: 64,  velLayers: 4, roundRobin: false },
  TRENCH:  { maxSlots: 96,  velLayers: 4, roundRobin: true  },
};
