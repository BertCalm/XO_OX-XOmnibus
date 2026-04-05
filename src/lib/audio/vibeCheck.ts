import type { PadAssignment, PadLayer } from '@/types';

export interface VibeCheckIntensity {
  /** 0.0-1.0, how much randomization to apply */
  amount: number;
  /** Named preset */
  preset: 'subtle' | 'moderate' | 'wild';
}

export const VIBE_PRESETS: { id: string; name: string; icon: string; description: string; amount: number }[] = [
  { id: 'subtle', name: 'Whisper', icon: '\u{1F30A}', description: 'Barely there \u2014 analog warmth', amount: 0.25 },
  { id: 'moderate', name: 'Character', icon: '\u{1F525}', description: 'Noticeable boutique feel', amount: 0.5 },
  { id: 'wild', name: 'Sentient', icon: '\u{1F9E0}', description: 'Maximum organic chaos', amount: 1.0 },
];

/**
 * Linearly interpolate a random value within [-range, +range], scaled by amount.
 */
function randBipolar(range: number, amount: number): number {
  return (Math.random() * 2 - 1) * range * amount;
}

/**
 * Random value within [min, max], scaled by amount (min is always applied, range scales).
 */
function randRange(min: number, max: number, amount: number): number {
  return min + Math.random() * (max - min) * amount;
}

/**
 * Clamp a number between lo and hi.
 */
function clamp(value: number, lo: number, hi: number): number {
  return Math.min(hi, Math.max(lo, value));
}

/**
 * Generate randomized layer updates for a single pad.
 * Returns an array of partial layer updates indexed by layer position.
 *
 * At amount=1.0 the full ranges are:
 *   tuneFine:      random(-8, +8)
 *   volume:        clamp(current + random(-0.06, +0.06), 0, 1)
 *   pan:           clamp(current + random(-0.05, +0.05), 0, 1)
 *   pitchRandom:   random(0.01, 0.04)
 *   volumeRandom:  random(0.02, 0.06)
 *   panRandom:     random(0.01, 0.04)
 *   offset:        random(0, 600)
 *   probability:   for layers index > 0, random(80, 100)
 *
 * At lower amounts all ranges scale proportionally.
 */
export function generateVibeUpdates(
  pad: PadAssignment,
  amount: number
): { layerIndex: number; updates: Partial<PadLayer> }[] {
  const results: { layerIndex: number; updates: Partial<PadLayer> }[] = [];

  for (let i = 0; i < pad.layers.length; i++) {
    const layer = pad.layers[i];

    // Only process active layers that have a sample assigned
    if (!layer.active || !layer.sampleId) continue;

    const updates: Partial<PadLayer> = {};

    // Detune: random fine tune offset scaled by amount. Full range: -8 to +8 cents.
    updates.tuneFine = Math.round(layer.tuneFine + randBipolar(8, amount));

    // Volume variation: offset current volume. Full range: +/-0.06.
    updates.volume = clamp(layer.volume + randBipolar(0.06, amount), 0, 1);

    // Pan scatter: offset current pan. Full range: +/-0.05.
    updates.pan = clamp(layer.pan + randBipolar(0.05, amount), 0, 1);

    // Pitch random: 0.01 to 0.04 at full amount.
    updates.pitchRandom = randRange(0.01, 0.04, amount);

    // Volume random: 0.02 to 0.06 at full amount.
    updates.volumeRandom = randRange(0.02, 0.06, amount);

    // Pan random: 0.01 to 0.04 at full amount.
    updates.panRandom = randRange(0.01, 0.04, amount);

    // Sample start offset: 0 to 600 samples at full amount.
    updates.offset = Math.round(Math.random() * 600 * amount);

    // Probability: for layers beyond the first active one, set 80-100% at full amount.
    // At lower amounts the floor rises toward 100.
    if (i > 0) {
      const floor = 100 - (100 - 80) * amount; // 100 at amount=0, 80 at amount=1
      updates.probability = Math.round(floor + Math.random() * (100 - floor));
    }

    results.push({ layerIndex: i, updates });
  }

  return results;
}
