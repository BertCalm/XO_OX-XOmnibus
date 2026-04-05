import type { SonicDnaProfile } from './sonicDnaExtractor';
import type { PadEnvelopeSettings } from '@/stores/envelopeStore';
import type { XpmModulationConfig } from './xpmTypes';
import type { InstrumentEnvelope } from '@/types';

// ---------------------------------------------------------------------------
// Types
// ---------------------------------------------------------------------------

export type DnaApplicationMode =
  | 'full'
  | 'envelopes-only'
  | 'filters-only'
  | 'modulation-only';

export interface DnaInjectionConfig {
  /** Which aspects of the DNA to apply. */
  mode: DnaApplicationMode;
  /** Blend factor 0-1: 0 = keep current, 1 = full DNA replacement. */
  blend: number;
  /** Pad indices to apply to. If undefined, applies to all loaded pads. */
  targetPads?: number[];
}

export interface DnaInjectionResult {
  /** Envelope updates keyed by pad index (ready for envelopeStore). */
  envelopeUpdates: Record<number, PadEnvelopeSettings>;
  /** Modulation updates keyed by pad index (ready for modulationStore). */
  modulationUpdates: Record<number, XpmModulationConfig>;
  /** Human-readable summary of changes per pad. */
  changes: { padIndex: number; description: string }[];
}

// ---------------------------------------------------------------------------
// Interpolation helpers
// ---------------------------------------------------------------------------

function lerp(a: number, b: number, t: number): number {
  return a * (1 - t) + b * t;
}

function lerpEnvelope(
  current: InstrumentEnvelope,
  target: InstrumentEnvelope,
  blend: number,
): InstrumentEnvelope {
  return {
    attack: lerp(current.attack, target.attack, blend),
    hold: lerp(current.hold, target.hold, blend),
    decay: lerp(current.decay, target.decay, blend),
    sustain: lerp(current.sustain, target.sustain, blend),
    release: lerp(current.release, target.release, blend),
  };
}

// ---------------------------------------------------------------------------
// Main injection computation
// ---------------------------------------------------------------------------

/**
 * Compute DNA injection results without mutating any stores.
 *
 * Returns envelope and modulation updates that the caller can apply
 * to the respective stores. This allows preview before committing.
 *
 * @param loadedPadIndices - Indices of pads that have samples loaded
 * @param currentEnvelopes - Current envelope settings per pad (from envelopeStore)
 * @param currentModulations - Current modulation configs per pad (from modulationStore)
 * @param dna - The Sonic DNA profile to inject
 * @param config - Injection configuration (mode, blend, target pads)
 */
export function computeDnaInjection(
  loadedPadIndices: number[],
  currentEnvelopes: Record<number, PadEnvelopeSettings>,
  currentModulations: Record<number, XpmModulationConfig>,
  dna: SonicDnaProfile,
  config: DnaInjectionConfig,
): DnaInjectionResult {
  const { mode, blend } = config;
  const targetSet = config.targetPads
    ? new Set(config.targetPads)
    : new Set(loadedPadIndices);

  const envelopeUpdates: Record<number, PadEnvelopeSettings> = {};
  const modulationUpdates: Record<number, XpmModulationConfig> = {};
  const changes: DnaInjectionResult['changes'] = [];

  for (const padIndex of loadedPadIndices) {
    if (!targetSet.has(padIndex)) continue;

    const currentEnv = currentEnvelopes[padIndex] ?? getDefaultEnvelope();
    const currentMod = currentModulations[padIndex] ?? { routes: [] };
    const changeDescriptions: string[] = [];

    // ----- Envelope injection -----
    if (mode === 'full' || mode === 'envelopes-only') {
      const newVolEnv = lerpEnvelope(currentEnv.volumeEnvelope, dna.volumeEnvelope, blend);
      const newFilterEnv = lerpEnvelope(currentEnv.filterEnvelope, dna.filterEnvelope, blend);

      envelopeUpdates[padIndex] = {
        ...currentEnv,
        volumeEnvelope: newVolEnv,
        filterEnvelope: newFilterEnv,
      };

      if (blend > 0) {
        changeDescriptions.push(
          `Volume envelope: A=${fmt(newVolEnv.attack)} H=${fmt(newVolEnv.hold)} D=${fmt(newVolEnv.decay)} S=${fmt(newVolEnv.sustain)} R=${fmt(newVolEnv.release)}`,
        );
      }
    }

    // ----- Filter injection -----
    if (mode === 'full' || mode === 'filters-only') {
      const envUpdate = envelopeUpdates[padIndex] ?? { ...currentEnv };

      // Blend filter parameters
      envUpdate.filterType = blend >= 0.5 ? dna.filterType : currentEnv.filterType;
      envUpdate.filterCutoff = lerp(currentEnv.filterCutoff, dna.filterCutoff, blend);
      envUpdate.filterResonance = lerp(currentEnv.filterResonance, dna.filterResonance, blend);
      envUpdate.filterEnvAmount = lerp(currentEnv.filterEnvAmount, dna.filterEnvAmount, blend);

      envelopeUpdates[padIndex] = envUpdate;

      if (blend > 0 && (dna.filterType !== 0 || dna.filterCutoff < 0.999)) {
        const typeNames = ['Off', 'LP', 'BP', 'HP'];
        changeDescriptions.push(
          `Filter: ${typeNames[envUpdate.filterType] ?? 'Off'} cutoff=${fmt(envUpdate.filterCutoff)} res=${fmt(envUpdate.filterResonance)} env=${fmt(envUpdate.filterEnvAmount)}`,
        );
      }
    }

    // ----- Modulation injection -----
    if (mode === 'full' || mode === 'modulation-only') {
      if (dna.modulation && dna.modulation.routes.length > 0) {
        if (blend >= 0.5) {
          // Above 50% blend: apply DNA modulation, scaling amounts by blend
          const scaledRoutes = dna.modulation.routes.map((route) => ({
            ...route,
            amount: route.amount * blend,
          }));

          modulationUpdates[padIndex] = {
            routes: scaledRoutes,
            lfo1: dna.modulation.lfo1 ? { ...dna.modulation.lfo1 } : undefined,
          };

          changeDescriptions.push(
            `Modulation: ${scaledRoutes.length} route(s) — ${scaledRoutes
              .map((r) => `${r.source}→${r.destination} (${fmt(r.amount)})`)
              .join(', ')}`,
          );
        } else if (currentMod.routes.length === 0 && blend > 0) {
          // Below 50% but current has no modulation: apply with scaled amounts
          const scaledRoutes = dna.modulation.routes.map((route) => ({
            ...route,
            amount: route.amount * blend * 2, // Scale up since blend < 0.5
          }));

          modulationUpdates[padIndex] = {
            routes: scaledRoutes,
            lfo1: dna.modulation.lfo1 ? { ...dna.modulation.lfo1 } : undefined,
          };

          changeDescriptions.push(
            `Modulation (subtle): ${scaledRoutes.length} route(s)`,
          );
        }
      }
    }

    if (changeDescriptions.length > 0) {
      changes.push({
        padIndex,
        description: changeDescriptions.join(' | '),
      });
    }
  }

  return { envelopeUpdates, modulationUpdates, changes };
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

function fmt(n: number): string {
  return n.toFixed(3);
}

function getDefaultEnvelope(): PadEnvelopeSettings {
  return {
    volumeEnvelope: { attack: 0, hold: 0, decay: 0.047244, sustain: 1, release: 0 },
    filterType: 0,
    filterCutoff: 1.0,
    filterResonance: 0.0,
    filterEnvAmount: 0.0,
    filterEnvelope: { attack: 0, hold: 0, decay: 0.047244, sustain: 1, release: 0 },
  };
}
