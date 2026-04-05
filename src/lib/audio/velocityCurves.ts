// ---------------------------------------------------------------------------
// Velocity Curve Presets
// ---------------------------------------------------------------------------
// Provides mathematical velocity response curves that map MIDI input velocity
// (0-127) to layer parameter adjustments. Used by VelocityCurveSelector to
// quickly set up multi-layer velocity splits on pads.
// ---------------------------------------------------------------------------

export type VelocityCurveType =
  | 'linear'       // Standard linear response
  | 'exponential'  // Quiet until you hit hard
  | 'logarithmic'  // Sensitive at low velocities
  | 's-curve'      // Smooth transition with plateau
  | 'compressed'   // Reduced dynamic range
  | 'expanded'     // Maximum dynamic range
  | 'stepped'      // Discrete velocity zones
  | 'inverted'     // Harder = quieter
  | 'fixed-high'   // Always max regardless of velocity
  | 'fixed-mid'    // Always medium
  | 'gentle'       // Subtle dynamics, never too quiet
  | 'punchy';      // Quick rise, stays loud

export interface VelocityCurve {
  id: VelocityCurveType;
  name: string;
  icon: string;
  description: string;
  /** Generate velocity-scaled volume for a given input velocity (0-127). Returns 0.0-1.0 */
  fn: (velocity: number) => number;
}

// ---------------------------------------------------------------------------
// Curve implementations
// ---------------------------------------------------------------------------

function clamp01(value: number): number {
  return Math.max(0, Math.min(1, value));
}

export const VELOCITY_CURVES: VelocityCurve[] = [
  {
    id: 'linear',
    name: 'Linear',
    icon: '📈',
    description: 'Standard linear response — output proportional to input',
    fn: (v) => clamp01(v / 127),
  },
  {
    id: 'exponential',
    name: 'Exponential',
    icon: '🔺',
    description: 'Quiet until you hit hard — emphasises strong hits',
    fn: (v) => clamp01(Math.pow(v / 127, 3)),
  },
  {
    id: 'logarithmic',
    name: 'Logarithmic',
    icon: '🔊',
    description: 'Very sensitive at low velocities — quick response',
    fn: (v) => clamp01(Math.log(1 + (v * 9) / 127) / Math.log(10)),
  },
  {
    id: 's-curve',
    name: 'S-Curve',
    icon: '〰️',
    description: 'Smooth sigmoid transition with a natural plateau',
    fn: (v) => clamp01(1 / (1 + Math.exp(-0.08 * (v - 64)))),
  },
  {
    id: 'compressed',
    name: 'Compressed',
    icon: '🗜️',
    description: 'Reduced dynamic range — always audible, less contrast',
    fn: (v) => clamp01(0.4 + 0.6 * (v / 127)),
  },
  {
    id: 'expanded',
    name: 'Expanded',
    icon: '↔️',
    description: 'Maximum dynamic range — quadratic falloff for softer hits',
    fn: (v) => clamp01(Math.pow(v / 127, 2)),
  },
  {
    id: 'stepped',
    name: 'Stepped',
    icon: '🪜',
    description: 'Four discrete velocity zones — clean quantised levels',
    // +1 offset prevents zone 0 (velocities 0-31) from producing silence.
    // Without it, floor(0/32)/3 = 0 — the entire bottom quarter is muted.
    fn: (v) => clamp01((Math.floor(v / 32) + 1) / 4),
  },
  {
    id: 'inverted',
    name: 'Inverted',
    icon: '🔄',
    description: 'Harder hits produce quieter output — creative effect',
    fn: (v) => clamp01(1 - v / 127),
  },
  {
    id: 'fixed-high',
    name: 'Fixed High',
    icon: '🔴',
    description: 'Always maximum volume regardless of velocity',
    fn: () => 1.0,
  },
  {
    id: 'fixed-mid',
    name: 'Fixed Mid',
    icon: '🟡',
    description: 'Always medium volume regardless of velocity',
    fn: () => 0.6,
  },
  {
    id: 'gentle',
    name: 'Gentle',
    icon: '🌊',
    description: 'Subtle dynamics — never drops below 50% volume',
    fn: (v) => clamp01(0.5 + 0.5 * (v / 127)),
  },
  {
    id: 'punchy',
    name: 'Punchy',
    icon: '👊',
    description: 'Quick rise that clips at half velocity — stays loud',
    fn: (v) => clamp01(Math.min(1, (v / 127) * 2)),
  },
];

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

/**
 * Look up a VelocityCurve by its type id.
 */
export function getCurveById(id: VelocityCurveType): VelocityCurve {
  return VELOCITY_CURVES.find((c) => c.id === id) ?? VELOCITY_CURVES[0];
}

/**
 * Apply a velocity curve to set up multi-layer velocity splits on a pad.
 * Given N active layers, distributes velocity ranges according to the curve.
 *
 * Each layer receives:
 *  - A non-overlapping `velStart` / `velEnd` range spanning 0-127
 *  - A `volume` value derived by evaluating the curve function at the
 *    midpoint of that layer's velocity range.
 */
export function applyVelocityCurveToLayers(
  layerCount: number,
  curveType: VelocityCurveType,
): { velStart: number; velEnd: number; volume: number }[] {
  if (layerCount <= 0) return [];

  const curve = getCurveById(curveType);
  const zoneSize = 128 / layerCount; // 128 values: 0-127

  return Array.from({ length: layerCount }, (_, i) => {
    const velStart = Math.round(i * zoneSize);
    const velEnd = Math.round((i + 1) * zoneSize) - 1;
    const midpoint = Math.round((velStart + velEnd) / 2);
    const volume = curve.fn(midpoint);

    return {
      velStart: Math.max(0, velStart),
      velEnd: Math.min(127, velEnd),
      volume: Math.round(volume * 1000) / 1000, // 3 decimal places
    };
  });
}
