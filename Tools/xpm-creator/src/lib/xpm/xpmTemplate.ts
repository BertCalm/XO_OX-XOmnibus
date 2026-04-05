import { XPM_VERSION, XPM_DEFAULTS, LAYER_DEFAULTS, MAX_LAYERS_PER_PAD } from '@/constants/mpcDefaults';
import type { XpmVersion, XpmLayerData, XpmInstrumentData } from './xpmTypes';

export function getDefaultVersion(): XpmVersion {
  return { ...XPM_VERSION };
}

export function getDefaultLayer(number: number): XpmLayerData {
  return {
    number,
    active: false,
    volume: LAYER_DEFAULTS.volume,
    pan: LAYER_DEFAULTS.pan,
    pitch: LAYER_DEFAULTS.pitch,
    tuneCoarse: LAYER_DEFAULTS.tuneCoarse,
    tuneFine: LAYER_DEFAULTS.tuneFine,
    velStart: LAYER_DEFAULTS.velStart,
    velEnd: LAYER_DEFAULTS.velEnd,
    sampleName: '',
    sampleFile: '',
    rootNote: LAYER_DEFAULTS.rootNote,
    keyTrack: LAYER_DEFAULTS.keyTrack,
    pitchRandom: LAYER_DEFAULTS.pitchRandom,
    volumeRandom: LAYER_DEFAULTS.volumeRandom,
    panRandom: LAYER_DEFAULTS.panRandom,
    sliceStart: LAYER_DEFAULTS.sliceStart,
    sliceEnd: LAYER_DEFAULTS.sliceEnd,
    sliceLoop: LAYER_DEFAULTS.sliceLoop,
    sliceLoopStart: LAYER_DEFAULTS.sliceLoopStart,
    sliceLoopCrossFadeLength: LAYER_DEFAULTS.sliceLoopCrossFadeLength,
    sliceIndex: LAYER_DEFAULTS.sliceIndex,
    direction: LAYER_DEFAULTS.direction,
    offset: LAYER_DEFAULTS.offset,
    probability: LAYER_DEFAULTS.probability,
  };
}

export function getDefaultInstrument(number: number): XpmInstrumentData {
  return {
    number,
    lowNote: 0,
    highNote: 127,
    ignoreBaseNote: false,
    zonePlay: XPM_DEFAULTS.zonePlay,
    triggerMode: XPM_DEFAULTS.triggerMode,
    filterType: XPM_DEFAULTS.filterType,
    cutoff: XPM_DEFAULTS.filterCutoff,
    resonance: XPM_DEFAULTS.filterResonance,
    filterEnvAmt: XPM_DEFAULTS.filterEnvAmt,
    filterAttack: 0.000000,
    filterDecay: XPM_DEFAULTS.filterDecay,
    filterSustain: 1.000000,
    filterRelease: 0.000000,
    filterHold: 0.000000,
    volumeAttack: XPM_DEFAULTS.volumeAttack,
    volumeHold: XPM_DEFAULTS.volumeHold,
    volumeDecay: XPM_DEFAULTS.volumeDecay,
    volumeSustain: XPM_DEFAULTS.volumeSustain,
    volumeRelease: XPM_DEFAULTS.volumeRelease,
    velocitySensitivity: XPM_DEFAULTS.velocitySensitivity,
    pitchAttack: 0.000000,
    pitchHold: 0.000000,
    pitchDecay: 0.000000,
    pitchSustain: 1.000000,
    pitchRelease: 0.000000,
    pitchEnvAmount: 0.500000,
    layers: Array.from({ length: MAX_LAYERS_PER_PAD }, (_, i) => getDefaultLayer(i + 1)),
  };
}

/**
 * Format a number to 6 decimal places (MPC standard)
 */
export function fmt(value: number): string {
  // Guard: NaN or Infinity would produce invalid XML values.
  // Default to 0 — matches MPC's behavior for unset parameters.
  if (!Number.isFinite(value)) return (0).toFixed(6);
  return value.toFixed(6);
}

/**
 * Format a boolean to MPC string format
 */
export function fmtBool(value: boolean): string {
  return value ? 'True' : 'False';
}
