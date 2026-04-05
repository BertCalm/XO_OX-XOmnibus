import { getAudioContext, createOfflineContext } from './audioContext';
import {
  adjustGain,
  applyFilter,
  applyFadeIn,
  applyLogFadeOut,
  applySafetyFadeOut,
  reverseBuffer,
} from './chopProcessors';
import {
  renderGranularCloud,
  applyResonator,
  applyTransientSnap,
  applyValveBloom,
} from './sculptingProcessors';
import { encodeWav } from './wavEncoder';
import { generateWaveformPeaks } from './audioUtils';
import { v4 as uuid } from 'uuid';
import type { AudioSample } from '@/types';

// ---------------------------------------------------------------------------
// Types
// ---------------------------------------------------------------------------

/**
 * Expression modes determine how the 8 layers differ from each other.
 *
 * - velocity: Classic velocity layers — soft/filtered→loud/bright (use with Velocity play mode)
 * - round-robin: Similar volume, subtle variations for natural repetition (use with Cycle mode)
 * - textured: Progressive analog warmth/saturation from clean to gritty
 * - lo-fi: Progressive degradation — bit-crush, sample-rate reduction
 * - spectral: Filter sweep morphing — each layer emphasizes different frequency bands
 */
export type AutoLayerMode =
  | 'velocity'
  | 'round-robin'
  | 'textured'
  | 'lo-fi'
  | 'spectral'
  | 'detuned'
  | 'octave'
  | 'drift'
  | 'harmonic'
  | 'stereo'
  | 'granular'
  | 'resonator'
  | 'transient'
  | 'valve-bloom'
  | 'freeze';

export interface AutoLayerConfig {
  /** Expression mode determines the character of variation */
  mode: AutoLayerMode;
  /** How much variation between layers (0–1). Default 0.75 */
  variation: number;
  /** Random micro-variations for natural feel (0–1). Default 0.4 */
  humanize: number;
  /** How to distribute 2 source samples across layers */
  dualMode: 'blend' | 'split' | 'interleave';
}

export interface GeneratedLayer {
  sample: AudioSample;
  velStart: number;
  velEnd: number;
  layerIndex: number;
}

interface LayerSpec {
  volume: number;
  filterCutoff: number;
  filterResonance: number;
  filterType: BiquadFilterType;
  pitchCents: number;
  attackMs: number;
  tailRatio: number;
  velStart: number;
  velEnd: number;
  sourceIndex: number;
  /** Soft-clip saturation amount (0 = none, 1 = heavy) */
  saturation: number;
  /** Sample-rate reduction factor (1 = original, 0.5 = half) */
  srReduction: number;
  /** Bit-depth reduction (16 = original, 4 = crushed) */
  bitReduction: number;
  /** Whether to reverse the buffer */
  reverse: boolean;
  /** Sculpting processor to apply before the standard chain */
  sculpting?: {
    type: 'granular' | 'resonator' | 'transient' | 'valve-bloom' | 'freeze';
    params: Record<string, number>;
  };
}

// ---------------------------------------------------------------------------
// Expression Mode Presets
// ---------------------------------------------------------------------------

/**
 * VELOCITY — Classic dynamic expression.
 * Models how a real instrument responds to playing intensity:
 * soft = quieter, duller, shorter | hard = louder, brighter, full-length
 */
function velocitySpecs(variation: number, humanize: number): Omit<LayerSpec, 'sourceIndex'>[] {
  const base = [
    { vol: 0.15, fc: 800,   pitch: -8, atk: 8, tail: 0.40 },
    { vol: 0.25, fc: 1200,  pitch: -5, atk: 6, tail: 0.50 },
    { vol: 0.38, fc: 1800,  pitch: -2, atk: 4, tail: 0.60 },
    { vol: 0.50, fc: 2800,  pitch: 0,  atk: 2, tail: 0.72 },
    { vol: 0.65, fc: 4500,  pitch: 1,  atk: 1, tail: 0.82 },
    { vol: 0.78, fc: 7000,  pitch: 3,  atk: 0, tail: 0.90 },
    { vol: 0.90, fc: 12000, pitch: 5,  atk: 0, tail: 0.96 },
    { vol: 1.00, fc: 20000, pitch: 7,  atk: 0, tail: 1.00 },
  ];

  return base.map((b, i) => {
    const v = variation;
    const hP = humanize > 0 ? (Math.random() - 0.5) * 2 * humanize * 5 : 0;
    const hV = humanize > 0 ? 1 + (Math.random() - 0.5) * 2 * humanize * 0.06 : 1;
    return {
      volume: clamp(lerp(1.0, b.vol, v) * hV, 0.05, 1.0),
      filterCutoff: clamp(lerp(20000, b.fc, v), 200, 20000),
      filterResonance: 0.7,
      filterType: 'lowpass' as BiquadFilterType,
      pitchCents: Math.round(lerp(0, b.pitch, v) + hP),
      attackMs: lerp(0, b.atk, v),
      tailRatio: clamp(lerp(1.0, b.tail, v), 0.2, 1.0),
      velStart: i * 16,
      velEnd: Math.min(i * 16 + 15, 127),
      saturation: 0,
      srReduction: 1,
      bitReduction: 16,
      reverse: false,
    };
  });
}

/**
 * ROUND ROBIN — Subtle variations for natural repetition.
 * All layers have similar volume/brightness but unique micro-character.
 * Use with Cycle play mode to avoid the "machine gun" effect.
 */
function roundRobinSpecs(variation: number, humanize: number): Omit<LayerSpec, 'sourceIndex'>[] {
  return Array.from({ length: 8 }, (_, i) => {
    const v = variation;
    // Each layer gets unique random offsets seeded by index
    const pitchOffset = (Math.sin(i * 2.7 + 0.3) * 6 + (Math.random() - 0.5) * 4) * v;
    const filterOffset = (Math.cos(i * 1.9 + 0.7) * 2000 + (Math.random() - 0.5) * 1000) * v;
    const volOffset = (Math.sin(i * 3.1 + 1.2) * 0.08 + (Math.random() - 0.5) * 0.04) * v;
    const tailOffset = (Math.cos(i * 2.3 + 0.5) * 0.06) * v;
    const atkOffset = Math.abs(Math.sin(i * 1.7)) * 1.5 * v;

    return {
      volume: clamp(0.85 + volOffset, 0.6, 1.0),
      filterCutoff: clamp(14000 + filterOffset, 4000, 20000),
      filterResonance: 0.5 + Math.random() * 0.5 * v,
      filterType: 'lowpass' as BiquadFilterType,
      pitchCents: Math.round(pitchOffset + (humanize > 0 ? (Math.random() - 0.5) * humanize * 3 : 0)),
      attackMs: atkOffset,
      tailRatio: clamp(0.95 + tailOffset, 0.8, 1.0),
      // Equal velocity ranges — all layers are accessible at any velocity
      velStart: 0,
      velEnd: 127,
      saturation: 0,
      srReduction: 1,
      bitReduction: 16,
      reverse: false,
    };
  });
}

/**
 * TEXTURED — Progressive analog warmth/saturation.
 * Layer 1 = clean, pristine | Layer 8 = warm, saturated, "pushed"
 * Creates a velocity response that goes from digital → analog character.
 */
function texturedSpecs(variation: number, humanize: number): Omit<LayerSpec, 'sourceIndex'>[] {
  const base = [
    { vol: 0.40, sat: 0.0,  fc: 20000, pitch: 0,  tail: 0.70 },
    { vol: 0.50, sat: 0.08, fc: 16000, pitch: 0,  tail: 0.76 },
    { vol: 0.60, sat: 0.18, fc: 13000, pitch: 1,  tail: 0.82 },
    { vol: 0.70, sat: 0.30, fc: 10000, pitch: 2,  tail: 0.88 },
    { vol: 0.78, sat: 0.42, fc: 8000,  pitch: 3,  tail: 0.92 },
    { vol: 0.86, sat: 0.55, fc: 6500,  pitch: 4,  tail: 0.95 },
    { vol: 0.93, sat: 0.70, fc: 5500,  pitch: 5,  tail: 0.98 },
    { vol: 1.00, sat: 0.85, fc: 4500,  pitch: 6,  tail: 1.00 },
  ];

  return base.map((b, i) => {
    const v = variation;
    const hP = humanize > 0 ? (Math.random() - 0.5) * humanize * 3 : 0;
    return {
      volume: clamp(lerp(0.85, b.vol, v), 0.2, 1.0),
      filterCutoff: clamp(lerp(20000, b.fc, v), 2000, 20000),
      filterResonance: 0.5,
      filterType: 'lowpass' as BiquadFilterType,
      pitchCents: Math.round(lerp(0, b.pitch, v) + hP),
      attackMs: 0,
      tailRatio: clamp(lerp(1.0, b.tail, v), 0.5, 1.0),
      velStart: i * 16,
      velEnd: Math.min(i * 16 + 15, 127),
      saturation: lerp(0, b.sat, v),
      srReduction: 1,
      bitReduction: 16,
      reverse: false,
    };
  });
}

/**
 * LO-FI — Progressive digital degradation.
 * Layer 1 = pristine | Layer 8 = crushed, aliased, vintage sampler character.
 * Think SP-1200, MPC60, early Akai samplers.
 */
function lofiSpecs(variation: number, humanize: number): Omit<LayerSpec, 'sourceIndex'>[] {
  const base = [
    { vol: 1.00, sr: 1.0,   bits: 16, fc: 20000, pitch: 0 },
    { vol: 0.95, sr: 0.9,   bits: 14, fc: 16000, pitch: -1 },
    { vol: 0.90, sr: 0.75,  bits: 12, fc: 12000, pitch: -2 },
    { vol: 0.85, sr: 0.6,   bits: 12, fc: 9000,  pitch: -3 },
    { vol: 0.80, sr: 0.5,   bits: 10, fc: 7000,  pitch: -4 },
    { vol: 0.78, sr: 0.4,   bits: 10, fc: 5500,  pitch: -5 },
    { vol: 0.75, sr: 0.33,  bits: 8,  fc: 4000,  pitch: -6 },
    { vol: 0.72, sr: 0.25,  bits: 8,  fc: 3000,  pitch: -8 },
  ];

  return base.map((b, i) => {
    const v = variation;
    const hP = humanize > 0 ? (Math.random() - 0.5) * humanize * 4 : 0;
    return {
      volume: clamp(lerp(1.0, b.vol, v), 0.4, 1.0),
      filterCutoff: clamp(lerp(20000, b.fc, v), 1000, 20000),
      filterResonance: 0.5,
      filterType: 'lowpass' as BiquadFilterType,
      pitchCents: Math.round(lerp(0, b.pitch, v) + hP),
      attackMs: 0,
      tailRatio: 1.0,
      velStart: i * 16,
      velEnd: Math.min(i * 16 + 15, 127),
      saturation: 0,
      srReduction: lerp(1, b.sr, v),
      bitReduction: Math.round(lerp(16, b.bits, v)),
      reverse: false,
    };
  });
}

/**
 * SPECTRAL — Filter sweep morphing.
 * Each layer emphasizes a different frequency band using bandpass filters.
 * Creates an evolving, shifting timbre across the velocity range.
 * Like sweeping a resonant filter from bass to treble.
 */
function spectralSpecs(variation: number, humanize: number): Omit<LayerSpec, 'sourceIndex'>[] {
  // Center frequencies for 8 bandpass sweeps (from sub-bass to air)
  const centers = [200, 400, 800, 1600, 3200, 6400, 10000, 16000];

  return centers.map((fc, i) => {
    const v = variation;
    const hP = humanize > 0 ? (Math.random() - 0.5) * humanize * 4 : 0;
    // Higher Q = narrower band = more dramatic effect
    const q = lerp(0.5, 2.5, v);
    // Volume compensation — bandpass reduces overall level
    const volBoost = lerp(1.0, 1.3, v);

    return {
      volume: clamp(volBoost * (0.75 + i * 0.035), 0.5, 1.0),
      filterCutoff: clamp(lerp(10000, fc, v), 100, 20000),
      filterResonance: q,
      filterType: (v > 0.3 ? 'bandpass' : 'lowpass') as BiquadFilterType,
      pitchCents: Math.round(hP),
      attackMs: 0,
      tailRatio: 1.0,
      velStart: i * 16,
      velEnd: Math.min(i * 16 + 15, 127),
      saturation: 0,
      srReduction: 1,
      bitReduction: 16,
      reverse: false,
    };
  });
}

/**
 * DETUNED — Chorus/unison thickness.
 * Layers have progressive pitch detuning, creating synth-like width.
 * Think supersaw unison, Juno chorus, or thick pad stacking.
 * Multi-source: sources detune in opposite directions for extra width.
 */
function detunedSpecs(variation: number, humanize: number): Omit<LayerSpec, 'sourceIndex'>[] {
  // Detune amounts in cents, symmetric around zero
  const base = [
    { cents: 0,   vol: 1.00, fc: 20000 },
    { cents: -3,  vol: 0.95, fc: 18000 },
    { cents: 5,   vol: 0.93, fc: 17000 },
    { cents: -8,  vol: 0.90, fc: 15000 },
    { cents: 10,  vol: 0.88, fc: 14000 },
    { cents: -14, vol: 0.85, fc: 12000 },
    { cents: 18,  vol: 0.82, fc: 11000 },
    { cents: -22, vol: 0.80, fc: 10000 },
  ];

  return base.map((b, i) => {
    const v = variation;
    const hP = humanize > 0 ? (Math.random() - 0.5) * humanize * 3 : 0;
    return {
      volume: clamp(lerp(0.9, b.vol, v), 0.5, 1.0),
      filterCutoff: clamp(lerp(20000, b.fc, v), 5000, 20000),
      filterResonance: 0.5,
      filterType: 'lowpass' as BiquadFilterType,
      pitchCents: Math.round(lerp(0, b.cents, v) + hP),
      attackMs: 0,
      tailRatio: 1.0,
      velStart: i * 16,
      velEnd: Math.min(i * 16 + 15, 127),
      saturation: lerp(0, 0.08, v), // Subtle warmth from detuning
      srReduction: 1,
      bitReduction: 16,
      reverse: false,
    };
  });
}

/**
 * OCTAVE — Octave stacking for organ/choir depth.
 * Layers bring in different octave doublings progressively.
 * Layer 1 = root only. Layer 8 = root + octave up + octave down + 5th.
 * Creates rich, full-spectrum harmonic content.
 */
function octaveSpecs(variation: number, humanize: number): Omit<LayerSpec, 'sourceIndex'>[] {
  // Each layer adds a different harmonic interval in cents
  // 0 = unison, 1200 = octave up, -1200 = octave down, 700 = 5th, 500 = 4th
  const base = [
    { pitch: 0,     vol: 0.45, fc: 8000,  tail: 0.50 },  // Root only, soft
    { pitch: 0,     vol: 0.55, fc: 10000, tail: 0.60 },  // Root, louder
    { pitch: 1200,  vol: 0.50, fc: 14000, tail: 0.70 },  // Octave up
    { pitch: -1200, vol: 0.55, fc: 6000,  tail: 0.75 },  // Octave down
    { pitch: 700,   vol: 0.60, fc: 12000, tail: 0.82 },  // 5th
    { pitch: 0,     vol: 0.75, fc: 16000, tail: 0.90 },  // Root full
    { pitch: 500,   vol: 0.70, fc: 14000, tail: 0.95 },  // 4th
    { pitch: 1200,  vol: 0.85, fc: 18000, tail: 1.00 },  // Octave up, full
  ];

  return base.map((b, i) => {
    const v = variation;
    const hP = humanize > 0 ? (Math.random() - 0.5) * humanize * 8 : 0;
    return {
      volume: clamp(lerp(0.85, b.vol, v), 0.2, 1.0),
      filterCutoff: clamp(lerp(20000, b.fc, v), 2000, 20000),
      filterResonance: 0.5,
      filterType: 'lowpass' as BiquadFilterType,
      pitchCents: Math.round(lerp(0, b.pitch, v) + hP),
      attackMs: lerp(0, i * 0.3, v),
      tailRatio: clamp(lerp(1.0, b.tail, v), 0.3, 1.0),
      velStart: i * 16,
      velEnd: Math.min(i * 16 + 15, 127),
      saturation: 0,
      srReduction: 1,
      bitReduction: 16,
      reverse: false,
    };
  });
}

/**
 * DRIFT — Analog instability / tape flutter.
 * Simulates the character of analog equipment — pitch wander, micro-timing,
 * random gain fluctuations. Each layer has unique "imperfections."
 * Creates an alive, breathing quality.
 */
function driftSpecs(variation: number, humanize: number): Omit<LayerSpec, 'sourceIndex'>[] {
  return Array.from({ length: 8 }, (_, i) => {
    const v = variation;
    // Use golden ratio–based offsets for non-repeating drift patterns
    const phi = 1.618033988749;
    const drift = (i * phi) % 1;

    const pitchDrift = (Math.sin(drift * Math.PI * 4) * 20 + (Math.random() - 0.5) * 15) * v;
    const volDrift = (Math.sin(drift * Math.PI * 2.5) * 0.12 + (Math.random() - 0.5) * 0.06) * v;
    const filterDrift = (Math.cos(drift * Math.PI * 3) * 4000 + (Math.random() - 0.5) * 2000) * v;
    const atkDrift = Math.abs(Math.sin(drift * Math.PI * 1.7)) * 3 * v;
    const tailDrift = (Math.cos(drift * Math.PI * 2.1) * 0.08) * v;
    const satDrift = Math.abs(Math.sin(drift * Math.PI * 3.3)) * 0.2 * v;

    const hP = humanize > 0 ? (Math.random() - 0.5) * humanize * 8 : 0;

    return {
      volume: clamp(0.82 + volDrift, 0.5, 1.0),
      filterCutoff: clamp(12000 + filterDrift, 3000, 20000),
      filterResonance: 0.5 + Math.random() * 0.4 * v,
      filterType: 'lowpass' as BiquadFilterType,
      pitchCents: Math.round(pitchDrift + hP),
      attackMs: atkDrift,
      tailRatio: clamp(0.92 + tailDrift, 0.7, 1.0),
      velStart: 0,
      velEnd: 127,
      saturation: clamp(satDrift, 0, 0.3),
      srReduction: lerp(1, 0.92 + Math.random() * 0.08, v), // Subtle SR wobble
      bitReduction: 16,
      reverse: false,
    };
  });
}

/**
 * HARMONIC — Overtone enrichment through waveshaping.
 * Progressive harmonic content from pure fundamental to rich overtones.
 * Layer 1 = clean sine-like. Layer 8 = harmonically rich, almost organ-like.
 * Differs from Textured (which saturates) — this specifically targets harmonic series.
 */
function harmonicSpecs(variation: number, humanize: number): Omit<LayerSpec, 'sourceIndex'>[] {
  const base = [
    { sat: 0.0,  fc: 4000,  q: 2.0, vol: 0.40, pitch: 0 },
    { sat: 0.05, fc: 5500,  q: 1.8, vol: 0.50, pitch: 0 },
    { sat: 0.12, fc: 7500,  q: 1.5, vol: 0.60, pitch: 2 },
    { sat: 0.22, fc: 10000, q: 1.2, vol: 0.70, pitch: 3 },
    { sat: 0.35, fc: 13000, q: 1.0, vol: 0.78, pitch: 4 },
    { sat: 0.50, fc: 16000, q: 0.8, vol: 0.86, pitch: 5 },
    { sat: 0.65, fc: 18000, q: 0.7, vol: 0.93, pitch: 6 },
    { sat: 0.80, fc: 20000, q: 0.5, vol: 1.00, pitch: 7 },
  ];

  return base.map((b, i) => {
    const v = variation;
    const hP = humanize > 0 ? (Math.random() - 0.5) * humanize * 3 : 0;
    return {
      volume: clamp(lerp(0.85, b.vol, v), 0.2, 1.0),
      // First use lowpass to shape base tone, harmonic content comes from saturation
      filterCutoff: clamp(lerp(20000, b.fc, v), 2000, 20000),
      filterResonance: lerp(0.5, b.q, v),
      filterType: 'lowpass' as BiquadFilterType,
      pitchCents: Math.round(lerp(0, b.pitch, v) + hP),
      attackMs: 0,
      tailRatio: 1.0,
      velStart: i * 16,
      velEnd: Math.min(i * 16 + 15, 127),
      // Saturation creates harmonics — key difference from Textured is the
      // filter opens WITH the saturation to let harmonics through
      saturation: lerp(0, b.sat, v),
      srReduction: 1,
      bitReduction: 16,
      reverse: false,
    };
  });
}

/**
 * STEREO — Progressive stereo widening.
 * Mono center at soft velocity → wide stereo spread at hard velocity.
 * Achieves width through micro-detuning and complementary filtering
 * between odd/even layers (left-leaning vs right-leaning character).
 */
function stereoSpecs(variation: number, humanize: number): Omit<LayerSpec, 'sourceIndex'>[] {
  // Even layers get slight pitch-up + high filter (right character)
  // Odd layers get slight pitch-down + low filter (left character)
  // Together they create stereo width via timbral difference
  const base = [
    { pitch: 0,   fc: 20000, vol: 0.35, sat: 0 },     // Mono center
    { pitch: 2,   fc: 16000, vol: 0.45, sat: 0 },     // Slight right tilt
    { pitch: -2,  fc: 8000,  vol: 0.50, sat: 0.02 },  // Slight left tilt
    { pitch: 5,   fc: 14000, vol: 0.60, sat: 0.03 },  // More right
    { pitch: -5,  fc: 6000,  vol: 0.65, sat: 0.05 },  // More left
    { pitch: 8,   fc: 12000, vol: 0.75, sat: 0.08 },  // Wide right
    { pitch: -8,  fc: 5000,  vol: 0.85, sat: 0.10 },  // Wide left
    { pitch: 12,  fc: 18000, vol: 1.00, sat: 0.12 },  // Full width
  ];

  return base.map((b, i) => {
    const v = variation;
    const hP = humanize > 0 ? (Math.random() - 0.5) * humanize * 3 : 0;
    return {
      volume: clamp(lerp(0.85, b.vol, v), 0.2, 1.0),
      filterCutoff: clamp(lerp(20000, b.fc, v), 3000, 20000),
      filterResonance: 0.7,
      filterType: 'lowpass' as BiquadFilterType,
      pitchCents: Math.round(lerp(0, b.pitch, v) + hP),
      attackMs: 0,
      tailRatio: 1.0,
      velStart: i * 16,
      velEnd: Math.min(i * 16 + 15, 127),
      saturation: lerp(0, b.sat, v),
      srReduction: 1,
      bitReduction: 16,
      reverse: false,
    };
  });
}

// ---------------------------------------------------------------------------
// Tier 3: Sculpting Modes (create fundamentally new sounds)
// ---------------------------------------------------------------------------

/**
 * GRANULAR — Cloud texture engine.
 * Each layer is a granular cloud with different density, scatter, and character.
 * Layer 1 = sparse, focused. Layer 8 = dense, diffused, ethereal.
 * Inspired by Torso S-4's granular processor.
 */
function granularSpecs(variation: number, humanize: number): Omit<LayerSpec, 'sourceIndex'>[] {
  const base = [
    { density: 20,  scatter: 0.5, grain: 0.08, dur: 1.2, rev: 0.0 },
    { density: 35,  scatter: 1.0, grain: 0.06, dur: 1.5, rev: 0.05 },
    { density: 50,  scatter: 2.0, grain: 0.05, dur: 1.8, rev: 0.10 },
    { density: 70,  scatter: 3.0, grain: 0.04, dur: 2.0, rev: 0.15 },
    { density: 90,  scatter: 4.0, grain: 0.03, dur: 2.5, rev: 0.20 },
    { density: 110, scatter: 5.0, grain: 0.03, dur: 3.0, rev: 0.25 },
    { density: 140, scatter: 7.0, grain: 0.02, dur: 3.5, rev: 0.30 },
    { density: 180, scatter: 10,  grain: 0.02, dur: 4.0, rev: 0.35 },
  ];

  return base.map((b, i) => {
    const v = variation;
    return {
      volume: clamp(0.5 + i * 0.07, 0.3, 1.0),
      filterCutoff: 20000,
      filterResonance: 0.5,
      filterType: 'lowpass' as BiquadFilterType,
      pitchCents: 0,
      attackMs: 0,
      tailRatio: 1.0,
      velStart: i * 16,
      velEnd: Math.min(i * 16 + 15, 127),
      saturation: 0,
      srReduction: 1,
      bitReduction: 16,
      reverse: false,
      sculpting: {
        type: 'granular' as const,
        params: {
          density: lerp(40, b.density, v),
          pitchScatter: lerp(0.5, b.scatter, v),
          grainSize: b.grain,
          durationMultiplier: lerp(1, b.dur, v),
          positionRandom: lerp(0.3, 0.9, v),
          reverseProbability: lerp(0, b.rev, v) + (humanize > 0 ? Math.random() * humanize * 0.1 : 0),
        },
      },
    };
  });
}

/**
 * RESONATOR — Physical modeling resonance.
 * Each layer excites a different resonant frequency, from deep body to
 * bright metallic ring. Creates tonal character from any source.
 * Inspired by Karplus-Strong / Torso S-4 resonant filter.
 */
function resonatorSpecs(variation: number, humanize: number): Omit<LayerSpec, 'sourceIndex'>[] {
  // Musical resonant frequencies (loosely follows harmonic series)
  const base = [
    { freq: 110,  fb: 0.70, damp: 2000,  mix: 0.3 },  // Deep body
    { freq: 165,  fb: 0.75, damp: 2500,  mix: 0.35 },  // Low resonance
    { freq: 220,  fb: 0.80, damp: 3000,  mix: 0.40 },  // A3 warm
    { freq: 330,  fb: 0.82, damp: 4000,  mix: 0.45 },  // E4 mid
    { freq: 440,  fb: 0.84, damp: 5000,  mix: 0.50 },  // A4 standard
    { freq: 660,  fb: 0.85, damp: 6000,  mix: 0.55 },  // E5 bright
    { freq: 880,  fb: 0.86, damp: 8000,  mix: 0.60 },  // A5 metallic
    { freq: 1320, fb: 0.88, damp: 10000, mix: 0.65 },  // E6 brilliant
  ];

  return base.map((b, i) => {
    const v = variation;
    const hP = humanize > 0 ? (Math.random() - 0.5) * humanize * 4 : 0;
    return {
      volume: clamp(0.6 + i * 0.05, 0.4, 1.0),
      filterCutoff: 20000,
      filterResonance: 0.5,
      filterType: 'lowpass' as BiquadFilterType,
      pitchCents: Math.round(hP),
      attackMs: 0,
      tailRatio: 1.0,
      velStart: i * 16,
      velEnd: Math.min(i * 16 + 15, 127),
      saturation: 0,
      srReduction: 1,
      bitReduction: 16,
      reverse: false,
      sculpting: {
        type: 'resonator' as const,
        params: {
          frequency: lerp(440, b.freq, v),
          feedback: lerp(0.5, b.fb, v),
          damping: lerp(8000, b.damp, v),
          mix: lerp(0.2, b.mix, v),
        },
      },
    };
  });
}

/**
 * TRANSIENT — Attack shaping.
 * Progressive "snap" intensity — from soft/rounded to sharp/cracking.
 * Layer 1 = gentle bloom. Layer 8 = aggressive transient spike.
 * Great for drums and percussive sounds.
 */
function transientSpecs(variation: number, humanize: number): Omit<LayerSpec, 'sourceIndex'>[] {
  const base = [
    { snap: 0.0,  bloom: 0.6, vol: 0.35, fc: 6000 },
    { snap: 0.0,  bloom: 0.4, vol: 0.45, fc: 8000 },
    { snap: 0.1,  bloom: 0.2, vol: 0.55, fc: 10000 },
    { snap: 0.25, bloom: 0.1, vol: 0.65, fc: 12000 },
    { snap: 0.4,  bloom: 0.0, vol: 0.75, fc: 14000 },
    { snap: 0.55, bloom: 0.0, vol: 0.85, fc: 16000 },
    { snap: 0.7,  bloom: 0.0, vol: 0.92, fc: 18000 },
    { snap: 0.9,  bloom: 0.0, vol: 1.00, fc: 20000 },
  ];

  return base.map((b, i) => {
    const v = variation;
    const hP = humanize > 0 ? (Math.random() - 0.5) * humanize * 3 : 0;
    // Determine sculpting type based on whether this is a "bloom" or "snap" layer
    let sculpting: LayerSpec['sculpting'] = undefined;
    if (b.bloom > 0.1) {
      sculpting = {
        type: 'valve-bloom',
        params: { bloomMs: lerp(20, 80, v), warmth: lerp(0.1, b.bloom, v) },
      };
    } else if (b.snap > 0) {
      sculpting = {
        type: 'transient',
        params: { snapMs: lerp(4, 12, v), intensity: lerp(0.1, b.snap, v) },
      };
    }

    return {
      volume: clamp(lerp(0.8, b.vol, v), 0.2, 1.0),
      filterCutoff: clamp(lerp(20000, b.fc, v), 4000, 20000),
      filterResonance: 0.5,
      filterType: 'lowpass' as BiquadFilterType,
      pitchCents: Math.round(hP),
      attackMs: 0,
      tailRatio: 1.0,
      velStart: i * 16,
      velEnd: Math.min(i * 16 + 15, 127),
      saturation: b.bloom > 0 ? lerp(0, 0.15, v) : 0,
      srReduction: 1,
      bitReduction: 16,
      reverse: false,
      sculpting,
    };
  });
}

/**
 * VALVE BLOOM — Tube amp breathing.
 * Progressive tube saturation with slow attack swell.
 * Layer 1 = quick, clean. Layer 8 = slow bloom with heavy warmth.
 * Makes pads and sustained sounds "breathe" into the mix.
 */
function valveBloomSpecs(variation: number, humanize: number): Omit<LayerSpec, 'sourceIndex'>[] {
  const base = [
    { bloom: 20,  warmth: 0.1, vol: 0.40, fc: 16000 },
    { bloom: 30,  warmth: 0.18, vol: 0.48, fc: 14000 },
    { bloom: 45,  warmth: 0.28, vol: 0.56, fc: 12000 },
    { bloom: 60,  warmth: 0.38, vol: 0.64, fc: 10000 },
    { bloom: 80,  warmth: 0.48, vol: 0.72, fc: 8500 },
    { bloom: 100, warmth: 0.58, vol: 0.80, fc: 7000 },
    { bloom: 130, warmth: 0.70, vol: 0.90, fc: 6000 },
    { bloom: 160, warmth: 0.85, vol: 1.00, fc: 5000 },
  ];

  return base.map((b, i) => {
    const v = variation;
    const hP = humanize > 0 ? (Math.random() - 0.5) * humanize * 3 : 0;
    return {
      volume: clamp(lerp(0.85, b.vol, v), 0.2, 1.0),
      filterCutoff: clamp(lerp(20000, b.fc, v), 3000, 20000),
      filterResonance: 0.5,
      filterType: 'lowpass' as BiquadFilterType,
      pitchCents: Math.round(hP),
      attackMs: 0,
      tailRatio: 1.0,
      velStart: i * 16,
      velEnd: Math.min(i * 16 + 15, 127),
      saturation: 0, // Handled by valve bloom processor
      srReduction: 1,
      bitReduction: 16,
      reverse: false,
      sculpting: {
        type: 'valve-bloom' as const,
        params: {
          bloomMs: lerp(10, b.bloom, v),
          warmth: lerp(0.05, b.warmth, v),
        },
      },
    };
  });
}

/**
 * FREEZE — Spectral time-stretch.
 * Each layer freezes a different moment of the source at different stretch amounts.
 * Layer 1 = near-original. Layer 8 = infinite sustain drone.
 * Creates atmospheric textures from any one-shot.
 */
function freezeSpecs(variation: number, humanize: number): Omit<LayerSpec, 'sourceIndex'>[] {
  const base = [
    { point: 0.05, stretch: 5,   slice: 0.08, vol: 0.5 },
    { point: 0.10, stretch: 10,  slice: 0.06, vol: 0.55 },
    { point: 0.15, stretch: 20,  slice: 0.05, vol: 0.60 },
    { point: 0.20, stretch: 30,  slice: 0.05, vol: 0.65 },
    { point: 0.30, stretch: 40,  slice: 0.04, vol: 0.70 },
    { point: 0.40, stretch: 60,  slice: 0.04, vol: 0.75 },
    { point: 0.50, stretch: 80,  slice: 0.03, vol: 0.80 },
    { point: 0.60, stretch: 100, slice: 0.03, vol: 0.85 },
  ];

  return base.map((b, i) => {
    const v = variation;
    const hP = humanize > 0 ? Math.random() * humanize * 0.1 : 0;
    return {
      volume: clamp(lerp(0.7, b.vol, v), 0.3, 1.0),
      filterCutoff: 20000,
      filterResonance: 0.5,
      filterType: 'lowpass' as BiquadFilterType,
      pitchCents: 0,
      attackMs: 0,
      tailRatio: 1.0,
      velStart: i * 16,
      velEnd: Math.min(i * 16 + 15, 127),
      saturation: 0,
      srReduction: 1,
      bitReduction: 16,
      reverse: false,
      sculpting: {
        type: 'freeze' as const,
        params: {
          freezePoint: lerp(0.1, b.point, v) + hP,
          stretchFactor: lerp(3, b.stretch, v),
          sliceDuration: b.slice,
        },
      },
    };
  });
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

function clamp(val: number, min: number, max: number): number {
  return Math.max(min, Math.min(max, val));
}

function lerp(a: number, b: number, t: number): number {
  return a + (b - a) * t;
}

// ---------------------------------------------------------------------------
// Audio Processing
// ---------------------------------------------------------------------------

/**
 * Apply soft-clip saturation to an AudioBuffer.
 * amount 0 = clean, 1 = heavy saturation
 */
function applySaturation(buffer: AudioBuffer, amount: number): AudioBuffer {
  if (amount <= 0) return buffer;
  const ctx = getAudioContext();
  const result = ctx.createBuffer(buffer.numberOfChannels, buffer.length, buffer.sampleRate);
  const drive = 1 + amount * 4; // 1x to 5x overdrive

  for (let ch = 0; ch < buffer.numberOfChannels; ch++) {
    const src = buffer.getChannelData(ch);
    const dest = result.getChannelData(ch);
    for (let i = 0; i < buffer.length; i++) {
      const driven = src[i] * drive;
      // Soft clipping using tanh
      dest[i] = Math.tanh(driven) / Math.tanh(drive); // Normalize to preserve volume
    }
  }

  return result;
}

/**
 * Reduce sample rate by decimation (downsample then hold).
 * factor 1.0 = original, 0.5 = half sample rate character
 */
function reduceSampleRate(buffer: AudioBuffer, factor: number): AudioBuffer {
  if (factor >= 0.99) return buffer;
  const ctx = getAudioContext();
  const result = ctx.createBuffer(buffer.numberOfChannels, buffer.length, buffer.sampleRate);
  const step = Math.max(1, Math.round(1 / factor));

  for (let ch = 0; ch < buffer.numberOfChannels; ch++) {
    const src = buffer.getChannelData(ch);
    const dest = result.getChannelData(ch);
    let held = 0;
    for (let i = 0; i < buffer.length; i++) {
      if (i % step === 0) {
        held = src[i];
      }
      dest[i] = held;
    }
  }

  return result;
}

/**
 * Reduce bit depth by quantization.
 * bits 16 = original, 8 = crushed, 4 = extreme
 */
function reduceBitDepth(buffer: AudioBuffer, bits: number): AudioBuffer {
  if (bits >= 16) return buffer;
  const ctx = getAudioContext();
  const result = ctx.createBuffer(buffer.numberOfChannels, buffer.length, buffer.sampleRate);
  const levels = Math.pow(2, bits);

  for (let ch = 0; ch < buffer.numberOfChannels; ch++) {
    const src = buffer.getChannelData(ch);
    const dest = result.getChannelData(ch);
    for (let i = 0; i < buffer.length; i++) {
      // Clamp to [-1, 1] to prevent out-of-range values from earlier
      // saturation/filter processing from producing invalid WAV output
      dest[i] = Math.max(-1, Math.min(1, Math.round(src[i] * levels) / levels));
    }
  }

  return result;
}

/**
 * Process a single layer through the full chain.
 */
async function processLayer(
  source: AudioBuffer,
  spec: LayerSpec
): Promise<AudioBuffer> {
  const ctx = getAudioContext();
  let buffer = source;

  // 0. Sculpting processors (applied BEFORE the standard chain)
  if (spec.sculpting) {
    const { type, params } = spec.sculpting;
    switch (type) {
      case 'granular':
        buffer = renderGranularCloud(buffer, {
          grainSize: params.grainSize ?? 0.04,
          density: params.density ?? 60,
          pitchScatter: params.pitchScatter ?? 3,
          positionRandom: params.positionRandom ?? 0.7,
          durationMultiplier: params.durationMultiplier ?? 2.5,
          reverseProbability: params.reverseProbability ?? 0.15,
        });
        break;
      case 'resonator':
        buffer = applyResonator(buffer, {
          frequency: params.frequency ?? 220,
          feedback: params.feedback ?? 0.85,
          damping: params.damping ?? 4000,
          mix: params.mix ?? 0.5,
        });
        break;
      case 'transient':
        buffer = applyTransientSnap(buffer, params.snapMs ?? 8, params.intensity ?? 0.6);
        break;
      case 'valve-bloom':
        buffer = applyValveBloom(buffer, params.bloomMs ?? 80, params.warmth ?? 0.5);
        break;
      case 'freeze': {
        // Import dynamically to avoid circular dependency
        const { renderSpectralFreeze } = await import('./sculptingProcessors');
        buffer = renderSpectralFreeze(buffer, {
          freezePoint: params.freezePoint ?? 0.1,
          stretchFactor: params.stretchFactor ?? 40,
          sliceDuration: params.sliceDuration ?? 0.05,
        });
        break;
      }
    }
  }

  // 1. Filter (skip if full-range lowpass)
  const isFullRange = spec.filterType === 'lowpass' && spec.filterCutoff >= 18000;
  if (!isFullRange) {
    buffer = await applyFilter(buffer, spec.filterType, spec.filterCutoff, spec.filterResonance);
  }

  // 2. Pitch micro-shift
  if (spec.pitchCents !== 0) {
    const rate = Math.pow(2, spec.pitchCents / 1200);
    const newLength = Math.ceil(buffer.length / rate);
    const offlineCtx = createOfflineContext(buffer.numberOfChannels, newLength, buffer.sampleRate);
    const src = offlineCtx.createBufferSource();
    src.buffer = buffer;
    src.playbackRate.value = rate;
    src.connect(offlineCtx.destination);
    src.start(0);
    buffer = await offlineCtx.startRendering();
  }

  // 3. Saturation (textured mode)
  if (spec.saturation > 0) {
    buffer = applySaturation(buffer, spec.saturation);
  }

  // 4. Sample-rate reduction (lo-fi mode)
  if (spec.srReduction < 0.99) {
    buffer = reduceSampleRate(buffer, spec.srReduction);
  }

  // 5. Bit-depth reduction (lo-fi mode)
  if (spec.bitReduction < 16) {
    buffer = reduceBitDepth(buffer, spec.bitReduction);
  }

  // 6. Gain/volume
  if (spec.volume < 0.99) {
    buffer = adjustGain(buffer, spec.volume);
  }

  // 7. Attack fade-in
  if (spec.attackMs > 0) {
    buffer = applyFadeIn(buffer, spec.attackMs / 1000);
  }

  // 8. Tail trimming
  if (spec.tailRatio < 0.98) {
    const keepLength = Math.max(
      Math.floor(buffer.length * spec.tailRatio),
      Math.floor(0.05 * buffer.sampleRate)
    );
    if (keepLength < buffer.length) {
      const fadeDuration = (keepLength * 0.15) / buffer.sampleRate;
      const trimmed = ctx.createBuffer(buffer.numberOfChannels, keepLength, buffer.sampleRate);
      for (let ch = 0; ch < buffer.numberOfChannels; ch++) {
        const src = buffer.getChannelData(ch);
        const dest = trimmed.getChannelData(ch);
        for (let i = 0; i < keepLength; i++) {
          dest[i] = src[i];
        }
      }
      buffer = applyLogFadeOut(trimmed, fadeDuration);
    }
  }

  // 9. Reverse (if specified)
  if (spec.reverse) {
    buffer = reverseBuffer(buffer);
  }

  // 10. Safety fade-out
  buffer = applySafetyFadeOut(buffer);

  return buffer;
}

// ---------------------------------------------------------------------------
// Source Distribution
// ---------------------------------------------------------------------------

function assignSourceIndices(
  specs: Omit<LayerSpec, 'sourceIndex'>[],
  numSources: number,
  dualMode: AutoLayerConfig['dualMode']
): LayerSpec[] {
  return specs.map((spec, i) => {
    let sourceIndex = 0;

    if (numSources === 1) {
      sourceIndex = 0;
    } else if (numSources === 2) {
      switch (dualMode) {
        case 'split':
          sourceIndex = i < 4 ? 0 : 1;
          break;
        case 'interleave':
          sourceIndex = i % 2;
          break;
        case 'blend':
        default:
          sourceIndex = i < 3 ? 0 : i > 4 ? 1 : Math.random() > 0.5 ? 0 : 1;
          break;
      }
    } else if (numSources === 3) {
      // Trio: A,B,C,A,B,C,A,B — round-robin across 3 sources
      switch (dualMode) {
        case 'split':
          sourceIndex = i < 3 ? 0 : i < 6 ? 1 : 2;
          break;
        case 'interleave':
          sourceIndex = i % 3;
          break;
        case 'blend':
        default:
          sourceIndex = i % 3;
          break;
      }
    } else if (numSources >= 4) {
      // Quartet: A,B,C,D,A,B,C,D — round-robin across 4 sources
      switch (dualMode) {
        case 'split':
          sourceIndex = i < 2 ? 0 : i < 4 ? 1 : i < 6 ? 2 : 3;
          break;
        case 'interleave':
          sourceIndex = i % numSources;
          break;
        case 'blend':
        default:
          sourceIndex = i % numSources;
          break;
      }
    }

    return { ...spec, sourceIndex };
  });
}

// ---------------------------------------------------------------------------
// Expression Mode Metadata (for UI)
// ---------------------------------------------------------------------------

export const EXPRESSION_MODES: {
  id: AutoLayerMode;
  name: string;
  description: string;
  icon: string;
  playMode: 'velocity' | 'cycle' | 'random';
  labels: string[];
}[] = [
  {
    id: 'velocity',
    name: 'Velocity',
    description: 'Classic dynamics — ghost to smash',
    icon: '🎯',
    playMode: 'velocity',
    labels: ['pp', 'p', 'mp', 'mf', 'f', 'ff', 'fff', 'ffff'],
  },
  {
    id: 'round-robin',
    name: 'Round Robin',
    description: 'Subtle variations for natural repetition',
    icon: '🔄',
    playMode: 'cycle',
    labels: ['v1', 'v2', 'v3', 'v4', 'v5', 'v6', 'v7', 'v8'],
  },
  {
    id: 'textured',
    name: 'Textured',
    description: 'Clean → warm analog saturation',
    icon: '🎛',
    playMode: 'velocity',
    labels: ['clean', 'silk', 'warm', 'glow', 'push', 'drive', 'heat', 'tape'],
  },
  {
    id: 'lo-fi',
    name: 'Lo-Fi',
    description: 'Pristine → crushed SP-1200 grit',
    icon: '📼',
    playMode: 'velocity',
    labels: ['hifi', 'cd', 'dat', 'mpc', 'sp12', 'emu', '8bit', 'crsh'],
  },
  {
    id: 'spectral',
    name: 'Spectral',
    description: 'Filter sweep across frequency bands',
    icon: '🌈',
    playMode: 'velocity',
    labels: ['sub', 'bass', 'low', 'mid', 'high', 'pres', 'bril', 'air'],
  },
  {
    id: 'detuned',
    name: 'Detuned',
    description: 'Chorus/unison thickness — synth-like width',
    icon: '🎹',
    playMode: 'cycle',
    labels: ['uni', '-3¢', '+5¢', '-8¢', '+10¢', '-14¢', '+18¢', '-22¢'],
  },
  {
    id: 'octave',
    name: 'Octave',
    description: 'Organ/choir depth — stacked octaves & 5ths',
    icon: '🎵',
    playMode: 'velocity',
    labels: ['root', 'R+', '8va', '8vb', '5th', 'full', '4th', '8va+'],
  },
  {
    id: 'drift',
    name: 'Drift',
    description: 'Analog flutter — alive, breathing variations',
    icon: '🌊',
    playMode: 'cycle',
    labels: ['d1', 'd2', 'd3', 'd4', 'd5', 'd6', 'd7', 'd8'],
  },
  {
    id: 'harmonic',
    name: 'Harmonic',
    description: 'Overtone enrichment — pure to organ-rich',
    icon: '✨',
    playMode: 'velocity',
    labels: ['pure', 'soft', '2nd', '3rd', '4th', '5th', 'rich', 'full'],
  },
  {
    id: 'stereo',
    name: 'Stereo',
    description: 'Progressive width — mono center to wide field',
    icon: '📡',
    playMode: 'velocity',
    labels: ['mono', 'R+', 'L+', 'R++', 'L++', 'wR', 'wL', 'max'],
  },
  // --- Tier 3: Sculpting Modes ---
  {
    id: 'granular',
    name: 'Granular',
    description: 'Cloud textures — shattered ethereal atmospheres',
    icon: '☁️',
    playMode: 'velocity',
    labels: ['wisp', 'haze', 'mist', 'fog', 'cloud', 'storm', 'nebula', 'void'],
  },
  {
    id: 'resonator',
    name: 'Resonator',
    description: 'Physical modeling — metallic & wooden resonance',
    icon: '🎸',
    playMode: 'velocity',
    labels: ['body', 'low', 'warm', 'mid', 'tone', 'ring', 'metal', 'bell'],
  },
  {
    id: 'transient',
    name: 'Transient',
    description: 'Attack shaping — soft bloom to hard snap',
    icon: '🥊',
    playMode: 'velocity',
    labels: ['bloom', 'soft', 'ease', 'med', 'firm', 'snap', 'crack', 'smack'],
  },
  {
    id: 'valve-bloom',
    name: 'Valve',
    description: 'Tube amp breathing — warm, living pad character',
    icon: '💨',
    playMode: 'velocity',
    labels: ['quick', 'glow', 'warm', 'swell', 'push', 'tube', 'heat', 'bloom'],
  },
  {
    id: 'freeze',
    name: 'Freeze',
    description: 'Spectral freeze — infinite sustain drones',
    icon: '🧊',
    playMode: 'velocity',
    labels: ['near', 'hold', 'slow', 'still', 'lock', 'deep', 'drone', 'inf'],
  },
];

// ---------------------------------------------------------------------------
// Main Entry Point
// ---------------------------------------------------------------------------

/**
 * Generate 8 layers from 1 or 2 source samples using the specified expression mode.
 */
export async function generateVelocityLayers(
  sources: AudioBuffer[],
  baseName: string,
  rootNote: number,
  config: Partial<AutoLayerConfig> = {},
  onProgress?: (layerNum: number) => void
): Promise<GeneratedLayer[]> {
  const fullConfig: AutoLayerConfig = {
    mode: config.mode ?? 'velocity',
    variation: config.variation ?? 0.75,
    humanize: config.humanize ?? 0.4,
    dualMode: config.dualMode ?? 'split',
  };

  // Build specs based on expression mode
  let rawSpecs: Omit<LayerSpec, 'sourceIndex'>[];
  switch (fullConfig.mode) {
    case 'round-robin':
      rawSpecs = roundRobinSpecs(fullConfig.variation, fullConfig.humanize);
      break;
    case 'textured':
      rawSpecs = texturedSpecs(fullConfig.variation, fullConfig.humanize);
      break;
    case 'lo-fi':
      rawSpecs = lofiSpecs(fullConfig.variation, fullConfig.humanize);
      break;
    case 'spectral':
      rawSpecs = spectralSpecs(fullConfig.variation, fullConfig.humanize);
      break;
    case 'detuned':
      rawSpecs = detunedSpecs(fullConfig.variation, fullConfig.humanize);
      break;
    case 'octave':
      rawSpecs = octaveSpecs(fullConfig.variation, fullConfig.humanize);
      break;
    case 'drift':
      rawSpecs = driftSpecs(fullConfig.variation, fullConfig.humanize);
      break;
    case 'harmonic':
      rawSpecs = harmonicSpecs(fullConfig.variation, fullConfig.humanize);
      break;
    case 'stereo':
      rawSpecs = stereoSpecs(fullConfig.variation, fullConfig.humanize);
      break;
    case 'granular':
      rawSpecs = granularSpecs(fullConfig.variation, fullConfig.humanize);
      break;
    case 'resonator':
      rawSpecs = resonatorSpecs(fullConfig.variation, fullConfig.humanize);
      break;
    case 'transient':
      rawSpecs = transientSpecs(fullConfig.variation, fullConfig.humanize);
      break;
    case 'valve-bloom':
      rawSpecs = valveBloomSpecs(fullConfig.variation, fullConfig.humanize);
      break;
    case 'freeze':
      rawSpecs = freezeSpecs(fullConfig.variation, fullConfig.humanize);
      break;
    case 'velocity':
    default:
      rawSpecs = velocitySpecs(fullConfig.variation, fullConfig.humanize);
      break;
  }

  const specs = assignSourceIndices(rawSpecs, sources.length, fullConfig.dualMode);
  const modeInfo = EXPRESSION_MODES.find((m) => m.id === fullConfig.mode) || EXPRESSION_MODES[0];
  const results: GeneratedLayer[] = [];

  for (let i = 0; i < specs.length; i++) {
    const spec = specs[i];
    const source = sources[Math.min(spec.sourceIndex, sources.length - 1)];

    const processed = await processLayer(source, spec);

    const wavBuffer = encodeWav(processed, 16);
    const peaks = generateWaveformPeaks(processed);

    const sample: AudioSample = {
      id: uuid(),
      name: `${baseName}_${modeInfo.labels[i]}`,
      fileName: `${baseName}_${modeInfo.labels[i]}.WAV`,
      duration: processed.duration,
      sampleRate: processed.sampleRate,
      channels: processed.numberOfChannels,
      bitDepth: 16,
      buffer: wavBuffer,
      waveformPeaks: peaks,
      rootNote,
      createdAt: Date.now(),
    };

    results.push({
      sample,
      velStart: spec.velStart,
      velEnd: spec.velEnd,
      layerIndex: i,
    });

    onProgress?.(i + 1);
  }

  return results;
}
