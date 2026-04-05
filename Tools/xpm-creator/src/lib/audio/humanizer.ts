import { HUMANIZE_DEFAULTS } from '@/constants/mpcDefaults';
import { getAudioContext } from '@/lib/audio/audioContext';
import type { PadLayer } from '@/types';

export interface HumanizeSettings {
  amount: number; // 0-100 percentage
  tuneFineRange: number;
  volumeRange: number;
  panRange: number;
  offsetRangeMs: number;
  pitchRandomRange: number;
}

export const DEFAULT_HUMANIZE_SETTINGS: HumanizeSettings = {
  amount: 50,
  ...HUMANIZE_DEFAULTS,
};

function randomInRange(min: number, max: number): number {
  return min + Math.random() * (max - min);
}

/**
 * Apply humanization to layer parameters when the same sample is stacked.
 * Each layer gets slightly different tuning, volume, pan, and timing offset
 * to create a more natural, organic sound.
 */
export function humanizeLayers(
  layers: PadLayer[],
  settings: HumanizeSettings,
  sampleRate?: number
): PadLayer[] {
  // Derive sample rate from AudioContext if not provided — never hardcode 44100.
  const effectiveSampleRate = sampleRate ?? getAudioContext().sampleRate;
  const scale = settings.amount / 100;

  return layers.map((layer, index) => {
    if (!layer.active || !layer.sampleId) return layer;

    // First layer stays untouched as the anchor
    if (index === 0) return layer;

    const tuneFineOffset = Math.round(
      randomInRange(-settings.tuneFineRange, settings.tuneFineRange) * scale
    );
    const volumeOffset =
      randomInRange(-settings.volumeRange, settings.volumeRange) * scale;
    const panOffset =
      randomInRange(-settings.panRange, settings.panRange) * scale;
    const offsetMs =
      randomInRange(-settings.offsetRangeMs, settings.offsetRangeMs) * scale;
    const pitchRandom = settings.pitchRandomRange * scale;

    // Convert offset from ms to samples at the given sample rate
    const offsetSamples = Math.round((offsetMs / 1000) * effectiveSampleRate);

    return {
      ...layer,
      tuneFine: clampTuneFine(layer.tuneFine + tuneFineOffset),
      volume: clamp(layer.volume + volumeOffset, 0, 1),
      pan: clamp(layer.pan + panOffset, 0, 1),
      offset: Math.max(0, layer.offset + offsetSamples),
      pitchRandom: clamp(pitchRandom, 0, 1),
    };
  });
}

/**
 * Check if a set of layers contains duplicate samples (for auto-humanize detection)
 */
export function hasStackedSamples(layers: PadLayer[]): boolean {
  const sampleIds = layers
    .filter((l) => l.active && l.sampleId)
    .map((l) => l.sampleId);
  return new Set(sampleIds).size < sampleIds.length;
}

/**
 * Get which sample IDs are stacked (appear more than once)
 */
export function getStackedSampleIds(layers: PadLayer[]): string[] {
  const counts = new Map<string, number>();
  layers
    .filter((l) => l.active && l.sampleId)
    .forEach((l) => {
      counts.set(l.sampleId!, (counts.get(l.sampleId!) || 0) + 1);
    });
  return Array.from(counts.entries())
    .filter(([, count]) => count > 1)
    .map(([id]) => id);
}

function clamp(value: number, min: number, max: number): number {
  return Math.min(max, Math.max(min, value));
}

function clampTuneFine(value: number): number {
  // TuneFine in MPC ranges from -99 to +99
  return Math.min(99, Math.max(-99, value));
}
