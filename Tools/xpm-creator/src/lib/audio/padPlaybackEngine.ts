import type { PadLayer, PadAssignment } from '@/types';
import type { PadEnvelopeSettings } from '@/stores/envelopeStore';

// ----- Types -----

export interface PadVoiceHandle {
  source: AudioBufferSourceNode;
  gainEnv: GainNode;
  filter: BiquadFilterNode | null;
  panner: StereoPannerNode;
  layerGain: GainNode;
  startTime: number;
  onEnded: (() => void) | null;
  stop: () => void;
  forceStop: () => void;
}

type SampleLookup = (sampleId: string) => AudioBuffer | null;

// ----- Constants -----

const MIN_FILTER_FREQ = 60;
const MAX_FILTER_FREQ = 20000;

const FILTER_TYPE_MAP: Record<number, BiquadFilterType> = {
  1: 'lowpass',
  2: 'bandpass',
  3: 'highpass',
};

// ----- Envelope Scheduling -----

/**
 * Schedule an AHDSR envelope on a Web Audio param.
 * @param exponential - Use exponential ramps (natural for volume/gain).
 *                      Linear is kept for filter frequency modulation.
 */
function scheduleAHDSR(
  param: AudioParam,
  ctx: AudioContext,
  startTime: number,
  attack: number,
  hold: number,
  decay: number,
  sustain: number,
  peakValue: number = 1,
  baseValue: number = 0,
  exponential: boolean = false
): void {
  const eps = 0.001; // small value to avoid 0 for exponential ramps

  param.setValueAtTime(Math.max(baseValue, eps), startTime);

  // Attack: ramp to peak
  const attackEnd = startTime + Math.max(attack, 0.001);
  if (exponential) {
    param.exponentialRampToValueAtTime(Math.max(peakValue, eps), attackEnd);
  } else {
    param.linearRampToValueAtTime(peakValue, attackEnd);
  }

  // Hold: stay at peak
  const holdEnd = attackEnd + hold;
  if (hold > 0) {
    param.setValueAtTime(Math.max(peakValue, eps), holdEnd);
  }

  // Decay: ramp to sustain level
  const sustainValue = baseValue + sustain * (peakValue - baseValue);
  const decayEnd = holdEnd + Math.max(decay, 0.001);
  if (exponential) {
    param.exponentialRampToValueAtTime(Math.max(sustainValue, eps), decayEnd);
  } else {
    param.linearRampToValueAtTime(Math.max(sustainValue, eps), decayEnd);
  }

  // When sustain is zero (or near-zero), the exponential ramp can only
  // reach eps (0.001 / -60dB) — not true silence. Schedule a hard zero
  // after the ramp to prevent ghost voices from accumulating.
  if (sustainValue <= 0 && exponential) {
    param.setValueAtTime(0, decayEnd + 0.001);
  }

  // Sustain: hold at sustain level (continues until release is triggered)
}

/**
 * Schedule the release phase of an envelope.
 * @param exponential - Use exponential ramp (matches attack/decay curve type).
 */
function scheduleRelease(
  param: AudioParam,
  releaseTime: number,
  release: number,
  targetValue: number = 0,
  exponential: boolean = false
): void {
  // cancelAndHoldAtTime freezes the param at its current interpolated value,
  // avoiding the stale-value click caused by cancelScheduledValues + param.value
  if (typeof param.cancelAndHoldAtTime === 'function') {
    param.cancelAndHoldAtTime(releaseTime);
  } else {
    // Fallback for older browsers that lack cancelAndHoldAtTime
    param.cancelScheduledValues(releaseTime);
    param.setValueAtTime(Math.max(param.value !== 0 ? param.value : 0.001, 0.001), releaseTime);
  }
  const endValue = Math.max(targetValue, 0.001);
  const endTime = releaseTime + Math.max(release, 0.005);
  if (exponential) {
    param.exponentialRampToValueAtTime(endValue, endTime);
  } else {
    param.linearRampToValueAtTime(endValue, endTime);
  }
}

// ----- Utility -----

function mapCutoffToFrequency(cutoff: number): number {
  // Exponential mapping: 0→60Hz, 1→20000Hz
  return MIN_FILTER_FREQ * Math.pow(MAX_FILTER_FREQ / MIN_FILTER_FREQ, cutoff);
}

function computePlaybackRate(tuneCoarse: number, tuneFine: number): number {
  return Math.pow(2, (tuneCoarse + tuneFine / 100) / 12);
}

function applyRandomization(base: number, randomAmount: number, min: number, max: number): number {
  if (randomAmount <= 0) return base;
  const offset = (Math.random() * 2 - 1) * randomAmount;
  return Math.max(min, Math.min(max, base + offset));
}

// ----- Single Voice Playback -----

export function playPadVoice(
  ctx: AudioContext,
  audioBuffer: AudioBuffer,
  layer: PadLayer,
  envSettings: PadEnvelopeSettings,
  velocity: number = 100
): PadVoiceHandle {
  const now = ctx.currentTime;

  // Apply randomization to layer parameters
  const volume = applyRandomization(layer.volume, layer.volumeRandom, 0, 1);
  const pan = applyRandomization(layer.pan, layer.panRandom, 0, 1);
  const pitchRandCents = layer.pitchRandom > 0 ? (Math.random() * 2 - 1) * layer.pitchRandom * 100 : 0;

  // Velocity scaling (0-127 mapped to 0-1 gain multiplier)
  const velGain = velocity / 127;

  // 1. Source node with pitch
  const source = ctx.createBufferSource();
  source.buffer = audioBuffer;
  const rate = computePlaybackRate(layer.tuneCoarse, layer.tuneFine + pitchRandCents);
  source.playbackRate.value = rate;

  // 2. Volume envelope gain node — use exponential curves for natural response
  const gainEnv = ctx.createGain();
  const { volumeEnvelope } = envSettings;
  scheduleAHDSR(
    gainEnv.gain,
    ctx,
    now,
    volumeEnvelope.attack,
    volumeEnvelope.hold,
    volumeEnvelope.decay,
    volumeEnvelope.sustain,
    1,    // peakValue
    0,    // baseValue
    true  // exponential — matches human hearing perception
  );

  // 3. Filter (optional — only if filterType != 0)
  let filterNode: BiquadFilterNode | null = null;
  const filterType = FILTER_TYPE_MAP[envSettings.filterType];

  if (filterType) {
    filterNode = ctx.createBiquadFilter();
    filterNode.type = filterType;

    const baseFreq = mapCutoffToFrequency(envSettings.filterCutoff);
    const resonance = 0.5 + envSettings.filterResonance * 19.5;
    filterNode.Q.value = resonance;

    if (envSettings.filterEnvAmount !== 0) {
      // Bipolar modulation: positive = sweep up, negative = sweep down
      const amount = envSettings.filterEnvAmount;
      let peakFreq: number;
      if (amount > 0) {
        // Positive: sweep from baseFreq up toward MAX_FILTER_FREQ
        const envRange = MAX_FILTER_FREQ - baseFreq;
        peakFreq = baseFreq + envRange * amount;
      } else {
        // Negative: sweep from baseFreq down toward MIN_FILTER_FREQ
        const envRange = baseFreq - MIN_FILTER_FREQ;
        peakFreq = baseFreq + envRange * amount; // amount is negative, so this subtracts
      }
      peakFreq = Math.max(MIN_FILTER_FREQ, Math.min(MAX_FILTER_FREQ, peakFreq));

      const { filterEnvelope } = envSettings;

      filterNode.frequency.setValueAtTime(baseFreq, now);
      scheduleAHDSR(
        filterNode.frequency,
        ctx,
        now,
        filterEnvelope.attack,
        filterEnvelope.hold,
        filterEnvelope.decay,
        filterEnvelope.sustain,
        peakFreq,
        baseFreq
      );
    } else {
      filterNode.frequency.value = baseFreq;
    }
  }

  // 4. Stereo panner (pan 0-1 mapped to -1 to +1)
  const panner = ctx.createStereoPanner();
  panner.pan.value = (pan - 0.5) * 2;

  // 5. Layer gain (volume + velocity)
  const layerGain = ctx.createGain();
  layerGain.gain.value = volume * velGain;

  // Connect chain: source → gainEnv → [filter] → panner → layerGain → destination
  source.connect(gainEnv);
  if (filterNode) {
    gainEnv.connect(filterNode);
    filterNode.connect(panner);
  } else {
    gainEnv.connect(panner);
  }
  panner.connect(layerGain);
  layerGain.connect(ctx.destination);

  // Handle sample offset
  const offsetSeconds = layer.offset > 0 ? layer.offset / audioBuffer.sampleRate : 0;

  // Start playback
  source.start(0, offsetSeconds);

  const handle: PadVoiceHandle = {
    source,
    gainEnv,
    filter: filterNode,
    panner,
    layerGain,
    startTime: now,
    onEnded: null,

    stop: () => {
      try {
        const releaseTime = ctx.currentTime;
        // Volume release uses exponential curve for natural decay
        scheduleRelease(gainEnv.gain, releaseTime, volumeEnvelope.release, 0, true);
        if (filterNode && filterType) {
          // Filter release stays linear (frequency domain)
          scheduleRelease(
            filterNode.frequency,
            releaseTime,
            envSettings.filterEnvelope.release,
            MIN_FILTER_FREQ,
            false
          );
        }
        // Schedule actual stop after release completes
        const stopTime = releaseTime + volumeEnvelope.release + 0.05;
        source.stop(stopTime);
      } catch {
        // Already stopped
      }
    },

    forceStop: () => {
      try {
        source.stop();
      } catch {
        // Already stopped
      }
      try {
        gainEnv.disconnect();
        if (filterNode) filterNode.disconnect();
        panner.disconnect();
        layerGain.disconnect();
      } catch {
        // Already disconnected
      }
    },
  };

  // Cleanup when source ends naturally
  source.onended = () => {
    try {
      gainEnv.disconnect();
      if (filterNode) filterNode.disconnect();
      panner.disconnect();
      layerGain.disconnect();
    } catch {
      // Already disconnected
    }
    handle.onEnded?.();
  };

  return handle;
}

// ----- Pad Playback (Multi-Layer) -----

export function playPad(
  ctx: AudioContext,
  pad: PadAssignment,
  envSettings: PadEnvelopeSettings,
  sampleLookup: SampleLookup,
  velocity: number = 100,
  cycleCounter?: number,
  lastRandomIndex?: number
): { voices: PadVoiceHandle[]; nextCycleCounter: number; lastRandomIndex: number } {
  const activeLayers = pad.layers.filter((l) => l.active && l.sampleId);
  if (activeLayers.length === 0) return { voices: [], nextCycleCounter: cycleCounter ?? 0, lastRandomIndex: lastRandomIndex ?? -1 };

  let layersToPlay: PadLayer[] = [];
  let crossfadeGains: number[] | undefined;
  let nextCycle = cycleCounter ?? 0;
  let newRandomIndex = lastRandomIndex ?? -1;

  switch (pad.playMode) {
    case 'simultaneous':
      layersToPlay = activeLayers;
      break;

    case 'velocity': {
      // --- Velocity Crossfading ---
      // Instead of hard-switching between layers, crossfade in a 10-velocity
      // overlap zone between adjacent layers. Limit to max 2 concurrent layers.
      const CROSSFADE_ZONE = 10; // velocity units of overlap

      // Sort active layers by velStart ascending for consistent zone checking
      const sortedVelLayers = [...activeLayers].sort((a, b) => a.velStart - b.velStart);

      // Find all layers whose zones (expanded by crossfade) include this velocity
      const candidates: { layer: PadLayer; gain: number }[] = [];

      for (let li = 0; li < sortedVelLayers.length; li++) {
        const layer = sortedVelLayers[li];
        const halfZone = CROSSFADE_ZONE / 2;

        // Expanded range for crossfade consideration
        const expandedStart = Math.max(0, layer.velStart - halfZone);
        const expandedEnd = Math.min(127, layer.velEnd + halfZone);

        if (velocity >= expandedStart && velocity <= expandedEnd) {
          // Calculate crossfade gain based on distance from zone edges
          let gain = 1.0;

          // Fade in at the bottom of the zone
          if (velocity < layer.velStart) {
            const fadeIn = (velocity - expandedStart) / halfZone;
            gain = Math.sin(fadeIn * Math.PI * 0.5); // Equal-power crossfade
          }
          // Fade out at the top of the zone
          else if (velocity > layer.velEnd) {
            const fadeOut = (expandedEnd - velocity) / halfZone;
            gain = Math.sin(fadeOut * Math.PI * 0.5);
          }

          candidates.push({ layer, gain });
        }
      }

      // Limit to max 2 overlapping layers for performance
      if (candidates.length > 2) {
        candidates.sort((a, b) => b.gain - a.gain);
        candidates.length = 2;
      }

      if (candidates.length > 0) {
        // Store crossfade gains for use in voice creation
        crossfadeGains = candidates.map((c) => c.gain);
        layersToPlay = candidates.map((c) => c.layer);
      } else if (activeLayers.length > 0) {
        // Fallback: play the closest layer
        let closest = activeLayers[0];
        let closestDist = Infinity;
        for (const layer of activeLayers) {
          const center = (layer.velStart + layer.velEnd) / 2;
          const dist = Math.abs(velocity - center);
          if (dist < closestDist) {
            closestDist = dist;
            closest = layer;
          }
        }
        layersToPlay = [closest];
        crossfadeGains = [1.0];
      }
      break;
    }

    case 'cycle':
      nextCycle = (cycleCounter ?? 0) % activeLayers.length;
      layersToPlay = [activeLayers[nextCycle]];
      nextCycle = nextCycle + 1;
      break;

    case 'random': {
      // Anti-repeat: avoid same layer twice in a row when there are 2+ options
      let idx = Math.floor(Math.random() * activeLayers.length);
      if (activeLayers.length > 1 && idx === (lastRandomIndex ?? -1)) {
        idx = (idx + 1) % activeLayers.length;
      }
      newRandomIndex = idx;
      layersToPlay = [activeLayers[idx]];
      break;
    }
  }

  const voices: PadVoiceHandle[] = [];

  for (let li = 0; li < layersToPlay.length; li++) {
    const layer = layersToPlay[li];
    if (!layer.sampleId) continue;

    // Probability gate — skip this layer if the dice roll fails
    const prob = layer.probability ?? 100;
    if (prob < 100 && Math.random() * 100 >= prob) continue;

    const buffer = sampleLookup(layer.sampleId);
    if (!buffer) continue;

    const voice = playPadVoice(ctx, buffer, layer, envSettings, velocity);

    // Apply crossfade gain if in velocity crossfade mode
    if (crossfadeGains && crossfadeGains[li] !== undefined && crossfadeGains[li] < 1.0) {
      voice.layerGain.gain.value *= crossfadeGains[li];
    }

    voices.push(voice);
  }

  return { voices, nextCycleCounter: nextCycle, lastRandomIndex: newRandomIndex };
}

// ----- Voice Management -----

export function stopVoices(voices: PadVoiceHandle[]): void {
  for (const voice of voices) {
    voice.stop();
  }
}

export function forceStopVoices(voices: PadVoiceHandle[]): void {
  for (const voice of voices) {
    voice.forceStop();
  }
}

/**
 * Quick fade-out for mute group choking — avoids clicks by ramping
 * gain to zero over a short duration (default 8ms) before stopping.
 */
export function fadeStopVoices(voices: PadVoiceHandle[], fadeDuration: number = 0.008): void {
  for (const voice of voices) {
    try {
      const ctx = voice.gainEnv.context as AudioContext;
      const now = ctx.currentTime;

      // Use cancelAndHoldAtTime to freeze the gain at its current interpolated
      // value — cancelScheduledValues + param.value can read a stale value and
      // cause a click when the gain jumps.
      if (typeof voice.gainEnv.gain.cancelAndHoldAtTime === 'function') {
        voice.gainEnv.gain.cancelAndHoldAtTime(now);
      } else {
        // Fallback for older browsers
        voice.gainEnv.gain.cancelScheduledValues(now);
        voice.gainEnv.gain.setValueAtTime(Math.max(voice.gainEnv.gain.value !== 0 ? voice.gainEnv.gain.value : 0.001, 0.001), now);
      }
      voice.gainEnv.gain.linearRampToValueAtTime(0.0001, now + fadeDuration);

      // Schedule actual stop after fade completes
      voice.source.stop(now + fadeDuration + 0.002);
    } catch {
      // Already stopped — force cleanup
      voice.forceStop();
    }
  }
}
