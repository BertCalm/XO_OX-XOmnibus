import { createOfflineContext, getAudioContext } from '@/lib/audio/audioContext';
import { encodeWav } from '@/lib/audio/wavEncoder';

// ---------------------------------------------------------------------------
// Types
// ---------------------------------------------------------------------------

export interface PreviewConfig {
  /** BPM for the preview pattern (default 90) */
  bpm?: number;
  /** Duration in seconds (default 5) */
  duration?: number;
  /** Sample rate (default 44100) */
  sampleRate?: number;
  /** Specific pad indices to use. If undefined, auto-selects representative pads. */
  padIndices?: number[];
}

export interface PreviewPadInfo {
  padIndex: number;
  buffer: AudioBuffer;
  /** Category hint for pattern generation: 'kick' | 'snare' | 'hat' | 'other' */
  role: 'kick' | 'snare' | 'hat' | 'other';
}

// ---------------------------------------------------------------------------
// Internal types
// ---------------------------------------------------------------------------

interface PatternEvent {
  /** Time offset in seconds from the start of the pattern */
  time: number;
  /** Reference to the pad that should be triggered */
  pad: PreviewPadInfo;
  /** MIDI-style velocity 0-127 */
  velocity: number;
}

// ---------------------------------------------------------------------------
// Constants
// ---------------------------------------------------------------------------

const DEFAULT_BPM = 90;
const DEFAULT_DURATION = 5;
const DEFAULT_SAMPLE_RATE = 44100;
const STEPS_PER_BAR = 16;
const BARS_PER_PATTERN = 2;

// ---------------------------------------------------------------------------
// Auto-selection
// ---------------------------------------------------------------------------

/**
 * Pick up to 3 representative pads from the full list.
 * Prefers one of each core role (kick, snare, hat), then fills with 'other'.
 */
function autoSelectPads(pads: PreviewPadInfo[]): PreviewPadInfo[] {
  let kick: PreviewPadInfo | undefined;
  let snare: PreviewPadInfo | undefined;
  let hat: PreviewPadInfo | undefined;
  const others: PreviewPadInfo[] = [];

  pads.forEach((p) => {
    if (!kick && p.role === 'kick') {
      kick = p;
    } else if (!snare && p.role === 'snare') {
      snare = p;
    } else if (!hat && p.role === 'hat') {
      hat = p;
    } else {
      others.push(p);
    }
  });

  const selected: PreviewPadInfo[] = [];
  if (kick) selected.push(kick);
  if (snare) selected.push(snare);
  if (hat) selected.push(hat);

  // Fill remaining slots (up to 3 total) from 'other' pads
  let otherIdx = 0;
  while (selected.length < 3 && otherIdx < others.length) {
    selected.push(others[otherIdx]);
    otherIdx++;
  }

  return selected;
}

// ---------------------------------------------------------------------------
// Pattern generation
// ---------------------------------------------------------------------------

/**
 * Build a 2-bar pattern (in 16th-note resolution) repeated once to create
 * 4 bars of material. Returns timed events in seconds.
 */
function generatePattern(
  pads: PreviewPadInfo[],
  bpm: number,
  duration: number,
): PatternEvent[] {
  const sixteenthDuration = 60 / bpm / 4;
  const totalSteps = STEPS_PER_BAR * BARS_PER_PATTERN; // 32 steps = 2 bars

  // Build role lookup
  const padsByRole = new Map<string, PreviewPadInfo>();
  pads.forEach((p) => {
    if (!padsByRole.has(p.role)) {
      padsByRole.set(p.role, p);
    }
  });

  const kick = padsByRole.get('kick');
  const snare = padsByRole.get('snare');
  const hat = padsByRole.get('hat');

  // Collect the 2-bar step events (step index + pad + velocity)
  const stepEvents: Array<{ step: number; pad: PreviewPadInfo; velocity: number }> = [];

  const hasFullKit = kick && snare && hat;
  const hasSinglePad = pads.length === 1;

  if (hasFullKit) {
    // Full boom-bap pattern ---------------------------------------------------
    // Kick: positions 0, 2, 8, 10 per bar (relative to bar start)
    [0, 2, 8, 10].forEach((s) => {
      stepEvents.push({ step: s, pad: kick, velocity: 100 });
      stepEvents.push({ step: s + STEPS_PER_BAR, pad: kick, velocity: 100 });
    });

    // Snare: positions 4, 12 per bar
    [4, 12].forEach((s) => {
      stepEvents.push({ step: s, pad: snare, velocity: 110 });
      stepEvents.push({ step: s + STEPS_PER_BAR, pad: snare, velocity: 110 });
    });

    // Hat: every other 16th note with alternating velocity
    for (let bar = 0; bar < BARS_PER_PATTERN; bar++) {
      for (let s = 0; s < STEPS_PER_BAR; s += 2) {
        const vel = s % 4 === 0 ? 80 : 60;
        stepEvents.push({
          step: bar * STEPS_PER_BAR + s,
          pad: hat,
          velocity: vel,
        });
      }
    }
  } else if (hasSinglePad) {
    // Single pad — quarter notes
    const pad = pads[0];
    for (let bar = 0; bar < BARS_PER_PATTERN; bar++) {
      [0, 4, 8, 12].forEach((s) => {
        stepEvents.push({
          step: bar * STEPS_PER_BAR + s,
          pad,
          velocity: 100,
        });
      });
    }
  } else if (kick && !snare && !hat) {
    // Only kicks available — quarter notes
    for (let bar = 0; bar < BARS_PER_PATTERN; bar++) {
      [0, 4, 8, 12].forEach((s) => {
        stepEvents.push({
          step: bar * STEPS_PER_BAR + s,
          pad: kick,
          velocity: 100,
        });
      });
    }
  } else {
    // Partial kit — assign available pads to simplified roles
    const available = [...pads];
    const primary = available[0];
    const secondary = available.length > 1 ? available[1] : undefined;

    // Primary on quarter notes
    for (let bar = 0; bar < BARS_PER_PATTERN; bar++) {
      [0, 4, 8, 12].forEach((s) => {
        stepEvents.push({
          step: bar * STEPS_PER_BAR + s,
          pad: primary,
          velocity: 100,
        });
      });
    }

    // Secondary on the backbeat (beats 2 and 4)
    if (secondary) {
      for (let bar = 0; bar < BARS_PER_PATTERN; bar++) {
        [4, 12].forEach((s) => {
          stepEvents.push({
            step: bar * STEPS_PER_BAR + s,
            pad: secondary,
            velocity: 90,
          });
        });
      }
    }
  }

  // Expand the 2-bar pattern to fill the duration by repeating it -----------
  const patternDuration = totalSteps * sixteenthDuration;
  const events: PatternEvent[] = [];
  let repetition = 0;

  while (true) {
    const offset = repetition * patternDuration;
    if (offset >= duration) break;

    stepEvents.forEach((se) => {
      const time = offset + se.step * sixteenthDuration;
      if (time < duration) {
        events.push({ time, pad: se.pad, velocity: se.velocity });
      }
    });

    repetition++;
  }

  return events;
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

/**
 * Render a preview audio clip of the kit.
 * Plays a simple 4-bar pattern at the specified BPM.
 * Returns encoded WAV data ready for XPN packaging.
 */
export async function renderPreview(
  pads: PreviewPadInfo[],
  config?: PreviewConfig,
): Promise<ArrayBuffer> {
  const bpm = config?.bpm ?? DEFAULT_BPM;
  const duration = config?.duration ?? DEFAULT_DURATION;
  // Derive sample rate from live AudioContext if not specified — avoids
  // hardcoding 44100 which would mismatch 48kHz/96kHz audio interfaces.
  const sampleRate = config?.sampleRate ?? getAudioContext().sampleRate;

  // Resolve which pads to use -----------------------------------------------
  let selectedPads: PreviewPadInfo[];

  if (config?.padIndices && config.padIndices.length > 0) {
    const indexSet = new Set(config.padIndices);
    selectedPads = pads.filter((p) => indexSet.has(p.padIndex));
  } else {
    selectedPads = autoSelectPads(pads);
  }

  if (selectedPads.length === 0) {
    throw new Error('No pads available for preview rendering');
  }

  // Generate the pattern -----------------------------------------------------
  const events = generatePattern(selectedPads, bpm, duration);

  // Offline render -----------------------------------------------------------
  const totalSamples = Math.ceil(duration * sampleRate);
  const offlineCtx = createOfflineContext(2, totalSamples, sampleRate);

  events.forEach((evt) => {
    const source = offlineCtx.createBufferSource();
    source.buffer = evt.pad.buffer;

    const gain = offlineCtx.createGain();
    gain.gain.value = evt.velocity / 127;

    source.connect(gain);
    gain.connect(offlineCtx.destination);
    source.start(evt.time);
  });

  const renderedBuffer = await offlineCtx.startRendering();

  // Encode to 16-bit WAV ----------------------------------------------------
  return encodeWav(renderedBuffer, 16);
}
