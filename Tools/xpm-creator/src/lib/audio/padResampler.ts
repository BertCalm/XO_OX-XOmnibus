/**
 * Pad Resampler — renders a pad's full audio output (all layers, envelopes, filters)
 * into a new AudioBuffer using OfflineAudioContext.
 * 
 * This is the digital equivalent of "bouncing" or "printing" a pad to a new sample —
 * capturing everything: volume envelope, filter sweep, pitch changes, panning, velocity.
 */

import type { PadAssignment, PadLayer } from '@/types';
import type { PadEnvelopeSettings } from '@/stores/envelopeStore';
import { getAudioContext } from '@/lib/audio/audioContext';

const MIN_FILTER_FREQ = 60;
const MAX_FILTER_FREQ = 20000;

const FILTER_TYPE_MAP: Record<number, BiquadFilterType> = {
  1: 'lowpass',
  2: 'bandpass',
  3: 'highpass',
};

function mapCutoffToFrequency(cutoff: number): number {
  return MIN_FILTER_FREQ * Math.pow(MAX_FILTER_FREQ / MIN_FILTER_FREQ, cutoff);
}

function computePlaybackRate(tuneCoarse: number, tuneFine: number): number {
  return Math.pow(2, (tuneCoarse + tuneFine / 100) / 12);
}

function scheduleAHDSR(
  param: AudioParam,
  startTime: number,
  attack: number,
  hold: number,
  decay: number,
  sustain: number,
  peakValue: number = 1,
  baseValue: number = 0,
  release?: number,
  noteOffTime?: number,
  exponential: boolean = false
): void {
  const eps = 0.001;
  param.setValueAtTime(Math.max(baseValue, eps), startTime);

  // Attack: ramp to peak — exponential for volume (matches human hearing),
  // linear for filter frequency modulation (matches live engine).
  const attackEnd = startTime + Math.max(attack, 0.001);
  if (exponential) {
    param.exponentialRampToValueAtTime(Math.max(peakValue, eps), attackEnd);
  } else {
    param.linearRampToValueAtTime(peakValue, attackEnd);
  }

  // Hold: stay at peak
  const holdEnd = attackEnd + hold;
  if (hold > 0) param.setValueAtTime(Math.max(peakValue, eps), holdEnd);

  // Decay: ramp to sustain
  const sustainValue = baseValue + sustain * (peakValue - baseValue);
  const decayEnd = holdEnd + Math.max(decay, 0.001);
  if (exponential) {
    param.exponentialRampToValueAtTime(Math.max(sustainValue, eps), decayEnd);
  } else {
    param.linearRampToValueAtTime(Math.max(sustainValue, eps), decayEnd);
  }

  // When sustain is zero (or near-zero) and exponential, the ramp can only
  // reach eps (0.001 / -60dB). Schedule a hard zero to prevent ghost voices.
  if (sustainValue <= 0 && exponential) {
    param.setValueAtTime(0, decayEnd + 0.001);
  }

  // Schedule release ramp: sustain level → target at note-off time.
  // Must match live engine's release target (0.001 = -60dB) to avoid
  // audible differences between preview and bounce.
  //
  // When sustainValue <= 0, the hard-zero setValueAtTime above already
  // brought the param to 0. Snapping it back to eps here would cause an
  // audible click, so skip the release phase entirely — the signal is
  // already silent.
  if (release != null && noteOffTime != null && sustainValue > 0) {
    const releaseStart = Math.max(decayEnd, noteOffTime);
    param.setValueAtTime(sustainValue, releaseStart);
    const endValue = Math.max(baseValue, 0.001);
    const endTime = releaseStart + Math.max(release, 0.001);
    if (exponential) {
      param.exponentialRampToValueAtTime(endValue, endTime);
    } else {
      param.linearRampToValueAtTime(endValue, endTime);
    }
  }
}

type SampleLookup = (sampleId: string) => AudioBuffer | null;

/**
 * Resample a single pad layer into an offline rendering.
 */
function renderLayerToOffline(
  ctx: OfflineAudioContext,
  audioBuffer: AudioBuffer,
  layer: PadLayer,
  envSettings: PadEnvelopeSettings,
  velocity: number
): void {
  const now = 0; // Offline context starts at 0

  const velGain = velocity / 127;
  const rate = computePlaybackRate(layer.tuneCoarse, layer.tuneFine);
  // Compute note-off time: when the sample finishes playing at the adjusted rate
  const offsetSeconds = layer.offset > 0 ? layer.offset / audioBuffer.sampleRate : 0;
  const samplePlayDuration = (audioBuffer.duration - offsetSeconds) / rate;
  const noteOffTime = Math.max(0, samplePlayDuration);

  // Source
  const source = ctx.createBufferSource();
  source.buffer = audioBuffer;
  source.playbackRate.value = rate;

  // Volume envelope — includes release ramp so the envelope doesn't
  // hold at sustain level indefinitely through the allocated tail.
  const gainEnv = ctx.createGain();
  const { volumeEnvelope } = envSettings;
  scheduleAHDSR(
    gainEnv.gain, now,
    volumeEnvelope.attack, volumeEnvelope.hold,
    volumeEnvelope.decay, volumeEnvelope.sustain,
    1, 0,
    volumeEnvelope.release, noteOffTime,
    true // exponential — matches live engine's human-hearing-perception curve
  );

  // Filter
  let filterNode: BiquadFilterNode | null = null;
  const filterType = FILTER_TYPE_MAP[envSettings.filterType];
  if (filterType) {
    filterNode = ctx.createBiquadFilter();
    filterNode.type = filterType;
    const baseFreq = mapCutoffToFrequency(envSettings.filterCutoff);
    filterNode.Q.value = 0.5 + envSettings.filterResonance * 19.5;

    if (envSettings.filterEnvAmount !== 0) {
      // Bipolar modulation: positive = sweep up, negative = sweep down
      const amount = envSettings.filterEnvAmount;
      let peakFreq: number;
      if (amount > 0) {
        const envRange = MAX_FILTER_FREQ - baseFreq;
        peakFreq = baseFreq + envRange * amount;
      } else {
        const envRange = baseFreq - MIN_FILTER_FREQ;
        peakFreq = baseFreq + envRange * amount; // amount is negative → subtracts
      }
      peakFreq = Math.max(MIN_FILTER_FREQ, Math.min(MAX_FILTER_FREQ, peakFreq));
      const { filterEnvelope } = envSettings;
      filterNode.frequency.setValueAtTime(baseFreq, now);
      scheduleAHDSR(
        filterNode.frequency, now,
        filterEnvelope.attack, filterEnvelope.hold,
        filterEnvelope.decay, filterEnvelope.sustain,
        peakFreq, baseFreq,
        filterEnvelope.release, noteOffTime
      );
    } else {
      filterNode.frequency.value = baseFreq;
    }
  }

  // Panner
  const panner = ctx.createStereoPanner();
  panner.pan.value = (layer.pan - 0.5) * 2;

  // Layer gain
  const layerGain = ctx.createGain();
  layerGain.gain.value = layer.volume * velGain;

  // Connect chain
  source.connect(gainEnv);
  if (filterNode) {
    gainEnv.connect(filterNode);
    filterNode.connect(panner);
  } else {
    gainEnv.connect(panner);
  }
  panner.connect(layerGain);
  layerGain.connect(ctx.destination);

  // Start (offsetSeconds already computed above for noteOffTime)
  source.start(0, offsetSeconds);
}

/**
 * Resample a pad's full audio output to a new AudioBuffer.
 * 
 * @param pad - The pad configuration
 * @param envSettings - Envelope and filter settings for this pad
 * @param sampleLookup - Function to resolve sampleId → AudioBuffer
 * @param velocity - Trigger velocity (0-127)
 * @param durationOverride - Optional max duration in seconds (default: auto from longest layer)
 * @returns A new AudioBuffer containing the rendered pad output
 */
export async function resamplePad(
  pad: PadAssignment,
  envSettings: PadEnvelopeSettings,
  sampleLookup: SampleLookup,
  velocity: number = 127,
  durationOverride?: number
): Promise<AudioBuffer> {
  // Determine which layers to render based on play mode
  const activeLayers = pad.layers.filter((l) => l.active && l.sampleId);
  if (activeLayers.length === 0) {
    throw new Error('No active layers to resample');
  }

  let layersToRender: PadLayer[];
  switch (pad.playMode) {
    case 'simultaneous':
      layersToRender = activeLayers;
      break;
    case 'velocity':
      layersToRender = activeLayers.filter(
        (l) => velocity >= l.velStart && velocity <= l.velEnd
      );
      if (layersToRender.length === 0) layersToRender = [activeLayers[0]];
      break;
    case 'cycle':
    case 'random':
      // For cycle/random, render the first active layer
      layersToRender = [activeLayers[0]];
      break;
    default:
      layersToRender = activeLayers;
  }

  // Calculate duration: longest sample adjusted for pitch + envelope release
  let maxDuration = 0;
  for (const layer of layersToRender) {
    if (!layer.sampleId) continue;
    const buf = sampleLookup(layer.sampleId);
    if (!buf) continue;
    const rate = computePlaybackRate(layer.tuneCoarse, layer.tuneFine);
    const adjustedDuration = buf.duration / rate;
    if (adjustedDuration > maxDuration) maxDuration = adjustedDuration;
  }

  // Add envelope release time + a small tail
  const releasePadding = envSettings.volumeEnvelope.release + 0.1;
  const totalDuration = durationOverride || (maxDuration + releasePadding);

  // Derive sample rate from source material, fall back to live AudioContext rate
  let sampleRate = getAudioContext().sampleRate;
  for (const layer of layersToRender) {
    if (!layer.sampleId) continue;
    const buf = sampleLookup(layer.sampleId);
    if (buf) { sampleRate = buf.sampleRate; break; }
  }
  // Guard: ensure minimum 1 sample to prevent OfflineAudioContext(0) crash
  const totalSamples = Math.max(1, Math.ceil(totalDuration * sampleRate));

  // Create offline context (stereo output)
  const offlineCtx = new OfflineAudioContext(2, totalSamples, sampleRate);

  // Render each layer
  for (const layer of layersToRender) {
    if (!layer.sampleId) continue;
    const buf = sampleLookup(layer.sampleId);
    if (!buf) continue;
    renderLayerToOffline(offlineCtx, buf, layer, envSettings, velocity);
  }

  // Render — wrap in try/catch since OfflineAudioContext.startRendering()
  // can reject on invalid graph states or resource limits
  let renderedBuffer: AudioBuffer;
  try {
    renderedBuffer = await offlineCtx.startRendering();
  } catch (err) {
    throw new Error(
      `Pad resample rendering failed: ${err instanceof Error ? err.message : 'unknown error'}`
    );
  }
  return renderedBuffer;
}
