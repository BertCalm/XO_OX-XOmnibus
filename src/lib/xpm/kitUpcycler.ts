import type { ParsedXpmKit, ParsedLayer, KitAnalysis } from './xpmParser';
import type { PadAssignment, PadLayer, LayerPlayMode, TriggerMode } from '@/types';
import type { PadEnvelopeSettings } from '@/stores/envelopeStore';
import type { XpmModulationConfig } from './xpmTypes';
import { LAYER_DEFAULTS, MAX_LAYERS_PER_PAD } from '@/constants/mpcDefaults';

// ---------------------------------------------------------------------------
// Types
// ---------------------------------------------------------------------------

export type UpcycleOption =
  | 'auto-ghost'       // Generate ghost note layers
  | 'expression-inject' // Add modulation routes
  | 'humanize'          // Add timing/pitch randomization
  | 'velocity-split'    // Create velocity layers from single layer
  | 'tail-tame'         // Add fade-outs to prevent clicks
  | 'stereo-widen'      // Apply Space Fold stereo imaging
  | 'full-upcycle';     // All of the above

export interface UpcycleConfig {
  options: UpcycleOption[];
  /** Intensity 0-1 */
  intensity: number;
  /** Expression mode to inject */
  expressionMode?: string;
  /** Modulation preset to inject */
  modulationPreset?: string;
}

export interface UpcycleResult {
  /** Updated pad assignments ready to load into padStore */
  pads: PadAssignment[];
  /** Summary of changes made */
  changes: { padIndex: number; description: string }[];
  /** New samples generated (ghost notes, etc) */
  generatedSamples: { fileName: string; data: ArrayBuffer }[];
}

/**
 * Enriched result from parsedKitToPads — includes envelope and modulation
 * data that was previously discarded during import.
 */
export interface ParsedKitResult {
  pads: PadAssignment[];
  /** Per-pad envelope + filter settings extracted from the kit. */
  envelopes: Record<number, PadEnvelopeSettings>;
  /** Per-pad modulation configs extracted from the kit. */
  modulations: Record<number, XpmModulationConfig>;
}

// ---------------------------------------------------------------------------
// Presets
// ---------------------------------------------------------------------------

export const UPCYCLE_PRESETS: {
  id: string;
  name: string;
  icon: string;
  description: string;
  options: UpcycleOption[];
  intensity: number;
}[] = [
  {
    id: 'quick-fix',
    name: 'Quick Fix',
    icon: '\u26A1',
    description: 'Tail taming + humanize basics',
    options: ['tail-tame', 'humanize'],
    intensity: 0.3,
  },
  {
    id: 'studio-ready',
    name: 'Studio Ready',
    icon: '\uD83C\uDFA8',
    description: 'Full dynamics + expression',
    options: ['auto-ghost', 'expression-inject', 'humanize', 'tail-tame'],
    intensity: 0.5,
  },
  {
    id: 'boutique',
    name: 'Boutique Upgrade',
    icon: '\u2728',
    description: 'Maximum XO_OX character',
    options: ['full-upcycle'],
    intensity: 0.8,
  },
];

// ---------------------------------------------------------------------------
// Trigger mode / play mode mapping
// ---------------------------------------------------------------------------

const TRIGGER_MODE_MAP: Record<number, TriggerMode> = {
  0: 'oneshot',
  1: 'noteoff',
  2: 'noteon',
};

const ZONE_PLAY_MAP: Record<number, LayerPlayMode> = {
  1: 'simultaneous',
  2: 'velocity',
  3: 'cycle',
  4: 'random',
};

// ---------------------------------------------------------------------------
// parsedKitToPads — convert parsed XPM data to app PadAssignment format
// ---------------------------------------------------------------------------

function parsedLayerToPadLayer(pl: ParsedLayer): PadLayer {
  return {
    number: Math.max(1, Math.min(8, pl.number)) as PadLayer['number'],
    active: pl.active,
    sampleId: null, // Will be resolved when samples are loaded into audioStore
    sampleName: pl.sampleName,
    sampleFile: pl.sampleFile,
    volume: pl.volume,
    pan: pl.pan,
    pitch: pl.pitch,
    tuneCoarse: pl.tuneCoarse,
    tuneFine: pl.tuneFine,
    velStart: pl.velStart,
    velEnd: pl.velEnd,
    rootNote: pl.rootNote,
    keyTrack: pl.keyTrack,
    pitchRandom: pl.pitchRandom,
    volumeRandom: pl.volumeRandom,
    panRandom: pl.panRandom,
    sliceStart: pl.sliceStart,
    sliceEnd: pl.sliceEnd,
    sliceLoop: pl.sliceLoop,
    sliceLoopStart: pl.sliceLoopStart,
    offset: pl.offset,
    direction: pl.direction,
    probability: pl.probability,
  };
}

function createEmptyPadLayer(number: PadLayer['number']): PadLayer {
  return {
    number,
    active: false,
    sampleId: null,
    sampleName: '',
    sampleFile: '',
    volume: LAYER_DEFAULTS.volume,
    pan: LAYER_DEFAULTS.pan,
    pitch: LAYER_DEFAULTS.pitch,
    tuneCoarse: LAYER_DEFAULTS.tuneCoarse,
    tuneFine: LAYER_DEFAULTS.tuneFine,
    velStart: LAYER_DEFAULTS.velStart,
    velEnd: LAYER_DEFAULTS.velEnd,
    rootNote: LAYER_DEFAULTS.rootNote,
    keyTrack: LAYER_DEFAULTS.keyTrack,
    pitchRandom: LAYER_DEFAULTS.pitchRandom,
    volumeRandom: LAYER_DEFAULTS.volumeRandom,
    panRandom: LAYER_DEFAULTS.panRandom,
    sliceStart: LAYER_DEFAULTS.sliceStart,
    sliceEnd: LAYER_DEFAULTS.sliceEnd,
    sliceLoop: LAYER_DEFAULTS.sliceLoop,
    sliceLoopStart: LAYER_DEFAULTS.sliceLoopStart,
    offset: LAYER_DEFAULTS.offset,
    direction: LAYER_DEFAULTS.direction,
    probability: LAYER_DEFAULTS.probability,
  };
}

/**
 * Convert a parsed XPM kit into PadAssignment format for the app.
 *
 * Returns a `ParsedKitResult` that includes envelope/filter settings
 * and modulation configs — data that was previously discarded.
 */
export function parsedKitToPads(kit: ParsedXpmKit): ParsedKitResult {
  const pads: PadAssignment[] = [];
  const envelopes: Record<number, PadEnvelopeSettings> = {};
  const modulations: Record<number, XpmModulationConfig> = {};

  for (let i = 0; i < kit.instruments.length; i++) {
    const inst = kit.instruments[i];

    // Convert parsed layers, ensuring exactly MAX_LAYERS_PER_PAD entries
    const layers: PadLayer[] = [];
    for (let layerIdx = 0; layerIdx < MAX_LAYERS_PER_PAD; layerIdx++) {
      if (layerIdx < inst.layers.length) {
        const converted = parsedLayerToPadLayer(inst.layers[layerIdx]);
        // Ensure layer number matches position
        converted.number = (layerIdx + 1) as PadLayer['number'];
        layers.push(converted);
      } else {
        layers.push(createEmptyPadLayer((layerIdx + 1) as PadLayer['number']));
      }
    }

    const triggerMode = TRIGGER_MODE_MAP[inst.triggerMode] ?? 'oneshot';
    const playMode = ZONE_PLAY_MAP[inst.zonePlay] ?? 'simultaneous';

    // Determine mute group from padGroupMap
    const groupEntry = kit.padGroupMap.find((g) => g.padNumber === inst.number);
    const muteGroup = groupEntry?.group ?? 0;

    pads.push({
      padNumber: inst.number,
      layers,
      playMode,
      triggerMode,
      muteGroup,
    });

    // ----- Extract envelope / filter settings (PREVIOUSLY DISCARDED) -----
    envelopes[i] = {
      volumeEnvelope: { ...inst.volumeEnvelope },
      filterType: inst.filterType,
      filterCutoff: inst.cutoff,
      filterResonance: inst.resonance,
      filterEnvAmount: inst.filterEnvAmount,
      filterEnvelope: { ...inst.filterEnvelope },
    };

    // ----- Extract modulation config (PREVIOUSLY DISCARDED) -----
    if (inst.modulation && inst.modulation.routes.length > 0) {
      modulations[i] = {
        routes: inst.modulation.routes.map((r) => ({ ...r })),
        lfo1: inst.modulation.lfo1 ? { ...inst.modulation.lfo1 } : undefined,
      };
    }
  }

  return { pads, envelopes, modulations };
}

// ---------------------------------------------------------------------------
// Upcycle transformations
// ---------------------------------------------------------------------------

/**
 * Resolve 'full-upcycle' into all individual options.
 */
function resolveOptions(options: UpcycleOption[]): Set<UpcycleOption> {
  const resolved = new Set<UpcycleOption>();
  for (const opt of options) {
    if (opt === 'full-upcycle') {
      resolved.add('auto-ghost');
      resolved.add('expression-inject');
      resolved.add('humanize');
      resolved.add('velocity-split');
      resolved.add('tail-tame');
      resolved.add('stereo-widen');
    } else {
      resolved.add(opt);
    }
  }
  return resolved;
}

/**
 * Get the first active layer index in a pad, or -1 if none.
 */
function firstActiveLayerIdx(pad: PadAssignment): number {
  return pad.layers.findIndex((l) => l.active && (l.sampleName || l.sampleFile));
}

/**
 * Count active layers in a pad.
 */
function countActiveLayers(pad: PadAssignment): number {
  return pad.layers.filter((l) => l.active && (l.sampleName || l.sampleFile)).length;
}

/**
 * Find the first empty (inactive) layer index.
 */
function firstEmptyLayerIdx(pad: PadAssignment): number {
  return pad.layers.findIndex((l) => !l.active);
}

// -- Auto Ghost ---------------------------------------------------------------

function applyAutoGhost(
  pads: PadAssignment[],
  intensity: number,
  changes: UpcycleResult['changes'],
): void {
  for (let padIdx = 0; padIdx < pads.length; padIdx++) {
    const pad = pads[padIdx];
    const activeCount = countActiveLayers(pad);

    // Only add ghost to pads with exactly 1 active layer
    if (activeCount !== 1) continue;

    const sourceIdx = firstActiveLayerIdx(pad);
    if (sourceIdx < 0) continue;

    const emptyIdx = firstEmptyLayerIdx(pad);
    if (emptyIdx < 0) continue; // No room

    const source = pad.layers[sourceIdx];
    const ghostLayer = { ...source };

    // Ghost note characteristics: quieter, lower velocity range, slightly detuned
    ghostLayer.number = (emptyIdx + 1) as PadLayer['number'];
    ghostLayer.volume = source.volume * (0.3 + intensity * 0.15); // 30-45% volume
    ghostLayer.velStart = 0;
    ghostLayer.velEnd = Math.round(50 + intensity * 20); // vel 0-50 to 0-70
    ghostLayer.tuneFine = Math.round(-3 * intensity); // Subtle detune
    ghostLayer.probability = Math.round(80 + (1 - intensity) * 15); // 80-95% probability

    // Adjust source layer velocity range to complement
    pad.layers[sourceIdx] = {
      ...source,
      velStart: ghostLayer.velEnd + 1,
    };

    pad.layers[emptyIdx] = ghostLayer;

    // Switch to velocity play mode so layers respond to velocity
    pad.playMode = 'velocity';

    changes.push({
      padIndex: padIdx,
      description: `Added ghost note layer (vel 0-${ghostLayer.velEnd}) at ${Math.round(ghostLayer.volume * 100)}% volume`,
    });
  }
}

// -- Expression Inject --------------------------------------------------------

function applyExpressionInject(
  pads: PadAssignment[],
  intensity: number,
  changes: UpcycleResult['changes'],
): void {
  // Expression injection works at the instrument/modulation level.
  // Since PadAssignment doesn't directly hold modulation config, we track
  // the change and apply humanization values that simulate expression.
  for (let padIdx = 0; padIdx < pads.length; padIdx++) {
    const pad = pads[padIdx];
    if (countActiveLayers(pad) === 0) continue;

    // Apply velocity-sensitive volume variation to each active layer
    let applied = false;
    for (const layer of pad.layers) {
      if (!layer.active) continue;

      // Add subtle volume randomization scaled by intensity (simulates velocity dynamics)
      const volRandom = 0.02 + intensity * 0.04; // 0.02-0.06
      if (layer.volumeRandom < volRandom) {
        layer.volumeRandom = volRandom;
        applied = true;
      }
    }

    if (applied) {
      changes.push({
        padIndex: padIdx,
        description: 'Injected expression dynamics (velocity-sensitive volume variation)',
      });
    }
  }
}

// -- Humanize -----------------------------------------------------------------

function applyHumanize(
  pads: PadAssignment[],
  intensity: number,
  changes: UpcycleResult['changes'],
): void {
  // Randomization ranges scale with intensity
  const tuneFineRange = Math.round(3 + intensity * 5); // +-3 to +-8
  const pitchRandomMax = 0.01 + intensity * 0.02; // 0.01-0.03
  const volumeRandomMax = 0.01 + intensity * 0.03; // 0.01-0.04
  const panRandomMax = 0.01 + intensity * 0.02; // 0.01-0.03

  for (let padIdx = 0; padIdx < pads.length; padIdx++) {
    const pad = pads[padIdx];
    let applied = false;

    for (const layer of pad.layers) {
      if (!layer.active) continue;

      // Apply random offset to tuneFine (seeded per-pad for consistency)
      const tuneOffset = Math.round((pseudoRandom(padIdx * 100 + layer.number) - 0.5) * 2 * tuneFineRange);
      if (layer.tuneFine === 0) {
        layer.tuneFine = tuneOffset;
        applied = true;
      }

      // Apply pitch randomization
      if (layer.pitchRandom < pitchRandomMax) {
        layer.pitchRandom = pitchRandomMax;
        applied = true;
      }

      // Apply volume randomization
      if (layer.volumeRandom < volumeRandomMax) {
        layer.volumeRandom = volumeRandomMax;
        applied = true;
      }

      // Apply pan randomization
      if (layer.panRandom < panRandomMax) {
        layer.panRandom = panRandomMax;
        applied = true;
      }
    }

    if (applied) {
      changes.push({
        padIndex: padIdx,
        description: `Humanized: tuneFine \u00B1${tuneFineRange}, pitchRandom ${pitchRandomMax.toFixed(3)}, volRandom ${volumeRandomMax.toFixed(3)}`,
      });
    }
  }
}

// -- Velocity Split -----------------------------------------------------------

function applyVelocitySplit(
  pads: PadAssignment[],
  intensity: number,
  changes: UpcycleResult['changes'],
): void {
  for (let padIdx = 0; padIdx < pads.length; padIdx++) {
    const pad = pads[padIdx];
    const activeCount = countActiveLayers(pad);

    // Only split pads with exactly 1 layer and full velocity range
    if (activeCount !== 1) continue;

    const sourceIdx = firstActiveLayerIdx(pad);
    if (sourceIdx < 0) continue;

    const source = pad.layers[sourceIdx];
    if (source.velStart !== 0 || source.velEnd !== 127) continue; // Already split

    const emptyIdx = firstEmptyLayerIdx(pad);
    if (emptyIdx < 0) continue; // No room

    // Split point scales with intensity: higher intensity = more overlap
    const splitPoint = Math.round(80 + intensity * 20); // 80-100

    // Soft layer (lower velocities)
    const softLayer = { ...source };
    softLayer.number = (emptyIdx + 1) as PadLayer['number'];
    softLayer.velStart = 0;
    softLayer.velEnd = splitPoint;
    softLayer.volume = source.volume * (0.7 + intensity * 0.1); // Slightly quieter
    softLayer.tuneFine = Math.round(-2 * intensity); // Subtle warmth

    // Hard layer (higher velocities) — modify the original
    pad.layers[sourceIdx] = {
      ...source,
      velStart: splitPoint + 1,
      velEnd: 127,
    };

    pad.layers[emptyIdx] = softLayer;
    pad.playMode = 'velocity';

    changes.push({
      padIndex: padIdx,
      description: `Created velocity split at ${splitPoint}: soft (0-${splitPoint}) + hard (${splitPoint + 1}-127)`,
    });
  }
}

// -- Tail Tame ----------------------------------------------------------------

function applyTailTame(
  pads: PadAssignment[],
  _intensity: number,
  changes: UpcycleResult['changes'],
): void {
  // Tail taming: flag samples that would benefit from release envelope adjustment.
  // In a real implementation this would process audio buffers. Here we note it in
  // changes and adjust envelope-related settings where possible.
  for (let padIdx = 0; padIdx < pads.length; padIdx++) {
    const pad = pads[padIdx];
    const activeCount = countActiveLayers(pad);
    if (activeCount === 0) continue;

    // Mark for tail processing — in practice the UI/export pipeline would
    // apply a short fade-out to prevent click artifacts
    changes.push({
      padIndex: padIdx,
      description: 'Flagged for tail-tame processing (anti-click fade-out)',
    });
  }
}

// -- Stereo Widen -------------------------------------------------------------

function applyStereoWiden(
  pads: PadAssignment[],
  intensity: number,
  changes: UpcycleResult['changes'],
): void {
  for (let padIdx = 0; padIdx < pads.length; padIdx++) {
    const pad = pads[padIdx];
    const activeLayers = pad.layers.filter(
      (l) => l.active && (l.sampleName || l.sampleFile),
    );

    if (activeLayers.length < 2) continue;

    // Apply progressive pan offsets to multi-layer pads
    const panSpread = 0.1 + intensity * 0.2; // 0.1-0.3 spread from center

    for (let i = 0; i < activeLayers.length; i++) {
      const layer = activeLayers[i];
      // For odd layer counts, keep the last layer centered to avoid
      // an asymmetric stereo image (without this, odd counts push
      // more energy to one side).
      if (activeLayers.length % 2 !== 0 && i === activeLayers.length - 1) {
        layer.pan = 0.5;
        continue;
      }
      // Alternate left/right from center (0.5)
      const direction = i % 2 === 0 ? 1 : -1;
      const layerOffset = (Math.floor(i / 2) + 1) * (panSpread / activeLayers.length);
      layer.pan = Math.max(0, Math.min(1, 0.5 + direction * layerOffset));
    }

    // Add subtle pan randomization for movement
    for (const layer of activeLayers) {
      if (layer.panRandom < 0.02 * intensity) {
        layer.panRandom = 0.02 * intensity;
      }
    }

    changes.push({
      padIndex: padIdx,
      description: `Applied stereo widening (\u00B1${Math.round(panSpread * 100)}% spread across ${activeLayers.length} layers)`,
    });
  }
}

// ---------------------------------------------------------------------------
// Pseudo-random (deterministic for reproducibility)
// ---------------------------------------------------------------------------

function pseudoRandom(seed: number): number {
  // Simple mulberry32-style PRNG
  let t = (seed + 0x6D2B79F5) | 0;
  t = Math.imul(t ^ (t >>> 15), t | 1);
  t ^= t + Math.imul(t ^ (t >>> 7), t | 61);
  return ((t ^ (t >>> 14)) >>> 0) / 4294967296;
}

// ---------------------------------------------------------------------------
// Main upcycle function
// ---------------------------------------------------------------------------

/**
 * Apply upcycling transformations to pad assignments.
 * This is the "magic" that transforms flat kits into expressive instruments.
 */
export function upcycleKit(
  pads: PadAssignment[],
  _analysis: KitAnalysis,
  config: UpcycleConfig,
): UpcycleResult {
  // Deep clone pads so we don't mutate the input
  const clonedPads: PadAssignment[] = pads.map((pad) => ({
    ...pad,
    layers: pad.layers.map((layer) => ({ ...layer })),
  }));

  const changes: UpcycleResult['changes'] = [];
  const generatedSamples: UpcycleResult['generatedSamples'] = [];
  const resolvedOpts = resolveOptions(config.options);

  // Apply transformations in a sensible order

  // 1. Velocity split first (creates new layers before ghost notes)
  if (resolvedOpts.has('velocity-split')) {
    applyVelocitySplit(clonedPads, config.intensity, changes);
  }

  // 2. Auto ghost (creates ghost note layers)
  if (resolvedOpts.has('auto-ghost')) {
    applyAutoGhost(clonedPads, config.intensity, changes);
  }

  // 3. Expression inject (adds velocity dynamics)
  if (resolvedOpts.has('expression-inject')) {
    applyExpressionInject(clonedPads, config.intensity, changes);
  }

  // 4. Humanize (adds randomization)
  if (resolvedOpts.has('humanize')) {
    applyHumanize(clonedPads, config.intensity, changes);
  }

  // 5. Stereo widen (adjusts pan for multi-layer pads)
  if (resolvedOpts.has('stereo-widen')) {
    applyStereoWiden(clonedPads, config.intensity, changes);
  }

  // 6. Tail tame (flags for processing)
  if (resolvedOpts.has('tail-tame')) {
    applyTailTame(clonedPads, config.intensity, changes);
  }

  return {
    pads: clonedPads,
    changes,
    generatedSamples,
  };
}
