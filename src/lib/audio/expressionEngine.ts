/**
 * Expression Engine — Real-time AudioNode insertion for expression modes.
 *
 * Inserts additional Web Audio processing nodes into the voice chain
 * AFTER the main signal path (source -> gainEnv -> filter -> panner -> layerGain).
 * The caller disconnects layerGain from ctx.destination, then connects
 * layerGain -> chain.input and chain.output -> ctx.destination.
 *
 * Does NOT modify the existing chain — only inserts a new chain segment.
 */

// ---------------------------------------------------------------------------
// Types
// ---------------------------------------------------------------------------

export type ExpressionMode =
  | 'valve-bloom'
  | 'transient-snap'
  | 'space-fold'
  | 'bit-crush'
  | 'ring-mod'
  | 'tape-saturation'
  | 'sub-harmonics'
  | 'lo-fi'
  | 'stereo-spread'
  | 'none';

export interface ExpressionConfig {
  mode: ExpressionMode;
  /** Intensity 0-1 */
  intensity: number;
  /** Velocity threshold: only trigger above this velocity (0-127) */
  velocityThreshold: number;
}

/** Nodes created by expression processing — caller must disconnect on cleanup */
export interface ExpressionNodeChain {
  /** The first node in the expression chain (connect layerGain to this) */
  input: AudioNode;
  /** The last node in the expression chain (connect this to destination) */
  output: AudioNode;
  /** All nodes to disconnect on cleanup */
  nodes: AudioNode[];
}

// ---------------------------------------------------------------------------
// Default config
// ---------------------------------------------------------------------------

export const DEFAULT_EXPRESSION: ExpressionConfig = {
  mode: 'none',
  intensity: 0.5,
  velocityThreshold: 0,
};

// ---------------------------------------------------------------------------
// Display metadata
// ---------------------------------------------------------------------------

export const EXPRESSION_MODE_INFO: Record<
  ExpressionMode,
  { label: string; icon: string; description: string }
> = {
  'valve-bloom':     { label: 'Valve Bloom',     icon: '\u{1F525}', description: 'Tube warmth & slow swell' },
  'transient-snap':  { label: 'Transient Snap',  icon: '\u26A1',    description: 'Sharp attack spike + presence' },
  'space-fold':      { label: 'Space Fold',      icon: '\u{1F300}', description: 'Velocity-mapped stereo width' },
  'bit-crush':       { label: 'Bit Crush',       icon: '\u{1F47E}', description: 'Lo-fi digital degradation' },
  'ring-mod':        { label: 'Ring Mod',         icon: '\u{1F514}', description: 'Metallic ring modulation' },
  'tape-saturation': { label: 'Tape Sat',         icon: '\u{1F4FC}', description: 'Warm analog tape character' },
  'sub-harmonics':   { label: 'Sub Harmonics',    icon: '\u{1F50A}', description: 'Octave-down sub layer' },
  'lo-fi':           { label: 'Lo-Fi',            icon: '\u{1F4FB}', description: 'Retro bandpass + bit reduction' },
  'stereo-spread':   { label: 'Stereo Spread',    icon: '\u{1F3A7}', description: 'Haas-style stereo widening' },
  'none':            { label: 'Off',              icon: '\u2B55',    description: 'No expression processing' },
};

// ---------------------------------------------------------------------------
// Curve helpers
// ---------------------------------------------------------------------------

/**
 * Build a Float32Array lookup table of `size` samples mapping -1..+1 through
 * the supplied shaping function.
 */
function buildCurve(
  size: number,
  fn: (x: number) => number,
): Float32Array<ArrayBuffer> {
  const curve = new Float32Array(size);
  // Guard: size=1 would cause division by zero in (size - 1)
  const divisor = size > 1 ? size - 1 : 1;
  for (let i = 0; i < size; i++) {
    const x = (2 * i) / divisor - 1; // -1 .. +1
    curve[i] = fn(x);
  }
  return curve as Float32Array<ArrayBuffer>;
}

/**
 * Tanh soft-clipping curve driven by `drive` (higher = more saturation).
 */
function tanhCurve(size: number, drive: number): Float32Array<ArrayBuffer> {
  return buildCurve(size, (x) => Math.tanh(x * drive));
}

/**
 * Staircase quantise curve — maps the continuous -1..+1 range into `levels`
 * discrete steps (creates a bit-crush / lo-fi effect).
 */
function staircaseCurve(size: number, levels: number): Float32Array<ArrayBuffer> {
  return buildCurve(size, (x) => {
    return Math.round(x * levels) / levels;
  });
}

// ---------------------------------------------------------------------------
// Per-mode chain builders
// ---------------------------------------------------------------------------

function buildValveBloom(
  ctx: AudioContext,
  intensity: number,
): ExpressionNodeChain {
  const drive = intensity * 3;

  const shaper = ctx.createWaveShaper();
  shaper.curve = tanhCurve(256, Math.max(drive, 0.5));
  shaper.oversample = '2x';

  const gain = ctx.createGain();
  const now = ctx.currentTime;
  gain.gain.setValueAtTime(0.7, now);
  gain.gain.linearRampToValueAtTime(1.0, now + 0.08 * intensity);

  shaper.connect(gain);

  return { input: shaper, output: gain, nodes: [shaper, gain] };
}

function buildTransientSnap(
  ctx: AudioContext,
  intensity: number,
): ExpressionNodeChain {
  const now = ctx.currentTime;

  const gain = ctx.createGain();
  gain.gain.setValueAtTime(1 + intensity * 2, now);
  gain.gain.exponentialRampToValueAtTime(1.0, now + 0.01);

  const filter = ctx.createBiquadFilter();
  filter.type = 'highpass';
  filter.frequency.setValueAtTime(2000 * intensity, now);
  filter.Q.setValueAtTime(0.7, now);

  gain.connect(filter);

  return { input: gain, output: filter, nodes: [gain, filter] };
}

function buildSpaceFold(
  ctx: AudioContext,
  intensity: number,
  velocity: number,
): ExpressionNodeChain {
  const panner = ctx.createStereoPanner();
  const pan = (velocity / 127 - 0.5) * intensity;
  panner.pan.setValueAtTime(Math.max(-1, Math.min(1, pan)), ctx.currentTime);

  return { input: panner, output: panner, nodes: [panner] };
}

function buildBitCrush(
  ctx: AudioContext,
  intensity: number,
): ExpressionNodeChain {
  const levels = Math.max(4, Math.floor(256 * (1 - intensity * 0.9)));

  const shaper = ctx.createWaveShaper();
  shaper.curve = staircaseCurve(4096, levels);
  shaper.oversample = 'none';

  return { input: shaper, output: shaper, nodes: [shaper] };
}

function buildRingMod(
  ctx: AudioContext,
  intensity: number,
): ExpressionNodeChain {
  const now = ctx.currentTime;
  const freq = 100 + intensity * 900;

  // Dry/wet blend: intensity=0 → clean passthrough, intensity=1 → full ring mod.
  // The modulation gain node multiplies the signal by the oscillator output.
  // We set its base gain to (1 - intensity) so there's always a dry component,
  // then add the osc scaled by intensity to create the wet ring-mod component.
  const modGain = ctx.createGain();
  modGain.gain.setValueAtTime(1 - intensity, now);

  const osc = ctx.createOscillator();
  osc.type = 'sine';
  osc.frequency.setValueAtTime(freq, now);

  // Scale the oscillator output by intensity before modulating the gain param.
  // This ensures the modulation depth is proportional to intensity.
  const oscGain = ctx.createGain();
  oscGain.gain.setValueAtTime(intensity, now);
  osc.connect(oscGain);
  oscGain.connect(modGain.gain);
  osc.start();

  return { input: modGain, output: modGain, nodes: [modGain, oscGain, osc] };
}

function buildTapeSaturation(
  ctx: AudioContext,
  intensity: number,
): ExpressionNodeChain {
  const drive = 1 + intensity * 2;

  const shaper = ctx.createWaveShaper();
  shaper.curve = tanhCurve(256, drive);
  shaper.oversample = '2x';

  const filter = ctx.createBiquadFilter();
  filter.type = 'lowpass';
  filter.frequency.setValueAtTime(
    12000 - intensity * 6000,
    ctx.currentTime,
  );
  filter.Q.setValueAtTime(0.5, ctx.currentTime);

  shaper.connect(filter);

  return { input: shaper, output: filter, nodes: [shaper, filter] };
}

function buildSubHarmonics(
  ctx: AudioContext,
  intensity: number,
): ExpressionNodeChain {
  // WaveShaperNode with a curve that introduces sub-harmonic content via a
  // sine-based non-linearity: y = x + k * sin(PI * x)
  const shaper = ctx.createWaveShaper();
  const k = intensity * 0.4;
  shaper.curve = buildCurve(4096, (x) => x + k * Math.sin(Math.PI * x));
  shaper.oversample = '2x';

  // Low-shelf boost at 80Hz to emphasise the generated sub content
  const shelf = ctx.createBiquadFilter();
  shelf.type = 'lowshelf';
  shelf.frequency.setValueAtTime(80, ctx.currentTime);
  shelf.gain.setValueAtTime(intensity * 12, ctx.currentTime);

  shaper.connect(shelf);

  return { input: shaper, output: shelf, nodes: [shaper, shelf] };
}

function buildLoFi(
  ctx: AudioContext,
  intensity: number,
): ExpressionNodeChain {
  const now = ctx.currentTime;

  // Stage 1 — bandpass to narrow the spectrum
  const bandpass = ctx.createBiquadFilter();
  bandpass.type = 'bandpass';
  bandpass.frequency.setValueAtTime(1000, now);
  bandpass.Q.setValueAtTime(0.8, now);

  // Stage 2 — staircase bit reduction
  const shaper = ctx.createWaveShaper();
  shaper.curve = staircaseCurve(4096, 32);
  shaper.oversample = 'none';

  // Stage 3 — lowpass to soften aliasing artifacts
  const lowpass = ctx.createBiquadFilter();
  lowpass.type = 'lowpass';
  lowpass.frequency.setValueAtTime(4000 - intensity * 2000, now);
  lowpass.Q.setValueAtTime(0.5, now);

  bandpass.connect(shaper);
  shaper.connect(lowpass);

  return { input: bandpass, output: lowpass, nodes: [bandpass, shaper, lowpass] };
}

function buildStereoSpread(
  ctx: AudioContext,
  intensity: number,
): ExpressionNodeChain {
  const panner = ctx.createStereoPanner();
  panner.pan.setValueAtTime(intensity * 0.3, ctx.currentTime);

  return { input: panner, output: panner, nodes: [panner] };
}

// ---------------------------------------------------------------------------
// Main entry point
// ---------------------------------------------------------------------------

/**
 * Create expression processing nodes for a voice.
 * Returns null if the mode is 'none' or velocity is below threshold.
 *
 * The caller should:
 * 1. Disconnect layerGain from ctx.destination
 * 2. Connect layerGain -> chain.input
 * 3. Connect chain.output -> ctx.destination
 */
export function createExpressionChain(
  ctx: AudioContext,
  velocity: number,
  config: ExpressionConfig,
): ExpressionNodeChain | null {
  if (config.mode === 'none') return null;
  if (velocity < config.velocityThreshold) return null;

  const intensity = Math.max(0, Math.min(1, config.intensity));

  switch (config.mode) {
    case 'valve-bloom':
      return buildValveBloom(ctx, intensity);

    case 'transient-snap':
      // Hard-gate: only fire on truly hard hits (velocity >= 110)
      if (velocity < 110) return null;
      return buildTransientSnap(ctx, intensity);

    case 'space-fold':
      return buildSpaceFold(ctx, intensity, velocity);

    case 'bit-crush':
      return buildBitCrush(ctx, intensity);

    case 'ring-mod':
      return buildRingMod(ctx, intensity);

    case 'tape-saturation':
      return buildTapeSaturation(ctx, intensity);

    case 'sub-harmonics':
      return buildSubHarmonics(ctx, intensity);

    case 'lo-fi':
      return buildLoFi(ctx, intensity);

    case 'stereo-spread':
      return buildStereoSpread(ctx, intensity);

    default: {
      // Exhaustiveness check — if a new mode is added but not handled the
      // compiler will flag this assignment.
      const _exhaustive: never = config.mode;
      return _exhaustive;
    }
  }
}
