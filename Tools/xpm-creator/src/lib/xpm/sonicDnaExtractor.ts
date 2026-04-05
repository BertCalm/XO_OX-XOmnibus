import type { ParsedXpmKit, ParsedInstrument } from './xpmParser';
import type { XpmModulationConfig } from './xpmTypes';
import type { InstrumentEnvelope } from '@/types';

// ---------------------------------------------------------------------------
// Types
// ---------------------------------------------------------------------------

export interface InstrumentDna {
  index: number;
  volumeEnvelope: InstrumentEnvelope;
  filterEnvelope: InstrumentEnvelope;
  pitchEnvelope: InstrumentEnvelope & { amount: number };
  filterType: number;
  filterCutoff: number;
  filterResonance: number;
  filterEnvAmount: number;
  velocitySensitivity: number;
  modulation?: XpmModulationConfig;
  /** True if this instrument differs from MPC defaults. */
  isCustomized: boolean;
}

export interface SonicDnaProfile {
  kitName: string;
  /** Representative (averaged) volume envelope across all customized instruments. */
  volumeEnvelope: InstrumentEnvelope;
  /** Representative filter envelope. */
  filterEnvelope: InstrumentEnvelope;
  /** Representative pitch envelope. */
  pitchEnvelope: InstrumentEnvelope & { amount: number };
  /** Most common non-off filter type in the kit (0 if none). */
  filterType: number;
  /** Average filter cutoff across instruments with active filters. */
  filterCutoff: number;
  /** Average filter resonance. */
  filterResonance: number;
  /** Average filter envelope amount. */
  filterEnvAmount: number;
  /** Average velocity sensitivity. */
  velocitySensitivity: number;
  /** The most common modulation config found (or undefined if none). */
  modulation?: XpmModulationConfig;
  /** Per-instrument DNA for granular application. */
  instruments: InstrumentDna[];
  /** How many instruments had customized settings. */
  customizedCount: number;
  /** Total instruments in the kit. */
  totalInstruments: number;
}

// ---------------------------------------------------------------------------
// MPC Defaults (for detecting customization)
// ---------------------------------------------------------------------------

const DEFAULT_ENV = { attack: 0, hold: 0, decay: 0.047244, sustain: 1, release: 0 };
const DEFAULT_PITCH_ENV = { ...DEFAULT_ENV, amount: 0.5 };
const TOLERANCE = 0.001;

function envelopeDiffers(
  env: InstrumentEnvelope,
  def: typeof DEFAULT_ENV,
): boolean {
  return (
    Math.abs(env.attack - def.attack) > TOLERANCE ||
    Math.abs(env.hold - def.hold) > TOLERANCE ||
    Math.abs(env.decay - def.decay) > TOLERANCE ||
    Math.abs(env.sustain - def.sustain) > TOLERANCE ||
    Math.abs(env.release - def.release) > TOLERANCE
  );
}

function isInstrumentCustomized(inst: ParsedInstrument): boolean {
  // Check volume envelope
  if (envelopeDiffers(inst.volumeEnvelope, DEFAULT_ENV)) return true;
  // Check filter envelope
  if (envelopeDiffers(inst.filterEnvelope, DEFAULT_ENV)) return true;
  // Check pitch envelope
  if (envelopeDiffers(inst.pitchEnvelope, DEFAULT_ENV)) return true;
  if (Math.abs(inst.pitchEnvelope.amount - 0.5) > TOLERANCE) return true;
  // Check filter settings
  if (inst.filterType !== 0) return true;
  if (inst.cutoff < 0.999) return true;
  if (inst.resonance > 0.001) return true;
  if (Math.abs(inst.filterEnvAmount) > TOLERANCE) return true;
  // Check modulation
  if (inst.modulation && inst.modulation.routes.length > 0) return true;
  // Check velocity sensitivity (default 1.0)
  if (Math.abs(inst.velocitySensitivity - 1.0) > TOLERANCE) return true;

  return false;
}

// ---------------------------------------------------------------------------
// Extraction
// ---------------------------------------------------------------------------

/**
 * Extract the "Sonic DNA" from a single parsed instrument.
 */
export function extractInstrumentDna(inst: ParsedInstrument, index: number): InstrumentDna {
  return {
    index,
    volumeEnvelope: { ...inst.volumeEnvelope },
    filterEnvelope: { ...inst.filterEnvelope },
    pitchEnvelope: { ...inst.pitchEnvelope },
    filterType: inst.filterType,
    filterCutoff: inst.cutoff,
    filterResonance: inst.resonance,
    filterEnvAmount: inst.filterEnvAmount,
    velocitySensitivity: inst.velocitySensitivity,
    modulation: inst.modulation
      ? {
          routes: inst.modulation.routes.map((r) => ({ ...r })),
          lfo1: inst.modulation.lfo1 ? { ...inst.modulation.lfo1 } : undefined,
        }
      : undefined,
    isCustomized: isInstrumentCustomized(inst),
  };
}

/**
 * Average an array of envelope values.
 */
function averageEnvelope(envelopes: InstrumentEnvelope[]): InstrumentEnvelope {
  if (envelopes.length === 0) return { ...DEFAULT_ENV };
  const sum = envelopes.reduce(
    (acc, env) => ({
      attack: acc.attack + env.attack,
      hold: acc.hold + env.hold,
      decay: acc.decay + env.decay,
      sustain: acc.sustain + env.sustain,
      release: acc.release + env.release,
    }),
    { attack: 0, hold: 0, decay: 0, sustain: 0, release: 0 },
  );
  const n = envelopes.length;
  return {
    attack: sum.attack / n,
    hold: sum.hold / n,
    decay: sum.decay / n,
    sustain: sum.sustain / n,
    release: sum.release / n,
  };
}

/**
 * Find the most common non-zero filter type in the kit.
 */
function mostCommonFilterType(instruments: ParsedInstrument[]): number {
  const counts = new Map<number, number>();
  for (const inst of instruments) {
    if (inst.filterType !== 0) {
      counts.set(inst.filterType, (counts.get(inst.filterType) ?? 0) + 1);
    }
  }
  if (counts.size === 0) return 0;

  let maxType = 0;
  let maxCount = 0;
  counts.forEach((count, type) => {
    if (count > maxCount) {
      maxType = type;
      maxCount = count;
    }
  });
  return maxType;
}

/**
 * Find the most common modulation config (by route signature).
 */
function mostCommonModulation(
  instruments: ParsedInstrument[],
): XpmModulationConfig | undefined {
  const modInstruments = instruments.filter(
    (inst) => inst.modulation && inst.modulation.routes.length > 0,
  );
  if (modInstruments.length === 0) return undefined;

  // Use the modulation from the instrument with the most routes
  // (representing the most "complete" modulation setup)
  let best = modInstruments[0].modulation!;
  for (const inst of modInstruments) {
    if (inst.modulation!.routes.length > best.routes.length) {
      best = inst.modulation!;
    }
  }

  return {
    routes: best.routes.map((r) => ({ ...r })),
    lfo1: best.lfo1 ? { ...best.lfo1 } : undefined,
  };
}

/**
 * Extract the full "Sonic DNA" profile from a parsed professional kit.
 * Walks all instruments, identifies customized ones, and computes
 * a representative profile for injection into raw kits.
 */
export function extractSonicDna(kit: ParsedXpmKit): SonicDnaProfile {
  const allDna = kit.instruments.map((inst, i) => extractInstrumentDna(inst, i));
  const customized = allDna.filter((d) => d.isCustomized);

  // Use customized instruments for averaging; fall back to all if none are customized
  const source = customized.length > 0 ? customized : allDna;
  const sourceInsts = source.map((d) => kit.instruments[d.index]);

  // Average envelopes
  const volumeEnvelope = averageEnvelope(sourceInsts.map((inst) => inst.volumeEnvelope));
  const filterEnvelope = averageEnvelope(sourceInsts.map((inst) => inst.filterEnvelope));
  const pitchEnvBase = averageEnvelope(sourceInsts.map((inst) => inst.pitchEnvelope));
  const pitchEnvAmount =
    sourceInsts.reduce((sum, inst) => sum + inst.pitchEnvelope.amount, 0) / sourceInsts.length;

  // Average filter settings (only from instruments with active filters)
  const filteredInsts = sourceInsts.filter((inst) => inst.filterType !== 0);
  const filterCutoff =
    filteredInsts.length > 0
      ? filteredInsts.reduce((sum, inst) => sum + inst.cutoff, 0) / filteredInsts.length
      : 1.0;
  const filterResonance =
    filteredInsts.length > 0
      ? filteredInsts.reduce((sum, inst) => sum + inst.resonance, 0) / filteredInsts.length
      : 0.0;
  const filterEnvAmount =
    filteredInsts.length > 0
      ? filteredInsts.reduce((sum, inst) => sum + inst.filterEnvAmount, 0) / filteredInsts.length
      : 0.0;

  // Average velocity sensitivity
  const velocitySensitivity =
    sourceInsts.reduce((sum, inst) => sum + inst.velocitySensitivity, 0) / sourceInsts.length;

  return {
    kitName: kit.name,
    volumeEnvelope,
    filterEnvelope,
    pitchEnvelope: { ...pitchEnvBase, amount: pitchEnvAmount },
    filterType: mostCommonFilterType(sourceInsts),
    filterCutoff,
    filterResonance,
    filterEnvAmount,
    velocitySensitivity,
    modulation: mostCommonModulation(sourceInsts),
    instruments: allDna,
    customizedCount: customized.length,
    totalInstruments: kit.instruments.length,
  };
}
