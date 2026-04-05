import { getAudioContext } from './audioContext';
import { decodeArrayBuffer, generateWaveformPeaks } from './audioUtils';
import { encodeWavAsync } from './wavEncoder';
import { pitchShiftSemitones } from './pitchShifter';
import {
  generateVelocityLayers,
  type AutoLayerMode,
  type AutoLayerConfig,
  type GeneratedLayer,
} from './autoLayerGenerator';
import { parseFilenameForMapping } from './filenameParser';
import { v4 as uuid } from 'uuid';
import type { AudioSample } from '@/types';

// ---------------------------------------------------------------------------
// Types
// ---------------------------------------------------------------------------

export type StackType = 'solo' | 'duet' | 'trio' | 'quartet';

export interface KeygroupSource {
  /** Original sample from audioStore */
  sample: AudioSample;
  /** Detected or user-set root note (MIDI 0-127) */
  rootNote: number;
  /** The decoded AudioBuffer */
  buffer?: AudioBuffer;
}

export interface KeygroupBuilderConfig {
  /** 1-4 source samples */
  sources: KeygroupSource[];
  /** Target root note to pitch-match all sources to */
  targetRoot: number;
  /** Expression mode for layer generation */
  mode: AutoLayerMode;
  /** Variation intensity 0-1 */
  variation: number;
  /** Humanize amount 0-1 */
  humanize: number;
  /** How to distribute multi-sources across layers */
  dualMode: AutoLayerConfig['dualMode'];
  /** Program name for the keygroup */
  programName: string;
}

export interface KeygroupBuildResult {
  /** Generated samples to add to audioStore */
  samples: AudioSample[];
  /** Layer assignments (padIndex -> layerIndex -> sample + velocity) */
  layerAssignments: {
    padIndex: number;
    layers: GeneratedLayer[];
    playMode: 'velocity' | 'cycle' | 'random';
  }[];
}

// ---------------------------------------------------------------------------
// Stack type helpers
// ---------------------------------------------------------------------------

export function getStackType(count: number): StackType {
  if (count <= 1) return 'solo';
  if (count === 2) return 'duet';
  if (count === 3) return 'trio';
  return 'quartet';
}

export const STACK_LABELS: Record<StackType, { label: string; icon: string; description: string }> = {
  solo: { label: 'Solo', icon: '🎤', description: '1 sample — full expression from one source' },
  duet: { label: 'Duet', icon: '👥', description: '2 samples — interplay between two voices' },
  trio: { label: 'Trio', icon: '🎻', description: '3 samples — rich three-part texture' },
  quartet: { label: 'Quartet', icon: '🎼', description: '4 samples — full ensemble stacking' },
};

// ---------------------------------------------------------------------------
// Root note detection
// ---------------------------------------------------------------------------

/**
 * Detect root note from filename, falling back to default C3 (60).
 */
export function detectRootNote(fileName: string, currentRoot: number = 60): number {
  const parsed = parseFilenameForMapping(fileName);
  return parsed.rootNote ?? currentRoot;
}

/**
 * Calculate the median root note as a good target for pitch matching.
 * Uses the median to avoid extreme transpositions.
 */
export function suggestTargetRoot(sources: KeygroupSource[]): number {
  if (sources.length === 0) return 60;
  const notes = sources.map((s) => s.rootNote).sort((a, b) => a - b);
  const mid = Math.floor(notes.length / 2);
  return notes.length % 2 === 0
    ? Math.round((notes[mid - 1] + notes[mid]) / 2)
    : notes[mid];
}

// ---------------------------------------------------------------------------
// Pitch matching
// ---------------------------------------------------------------------------

/**
 * Pitch-shift all source buffers so they match the target root note.
 * Returns new AudioBuffers — originals are not modified.
 */
async function pitchMatchSources(
  sources: { buffer: AudioBuffer; rootNote: number }[],
  targetRoot: number,
  onProgress?: (current: number, total: number) => void
): Promise<AudioBuffer[]> {
  const results: AudioBuffer[] = [];

  for (let i = 0; i < sources.length; i++) {
    const { buffer, rootNote } = sources[i];
    const semitones = targetRoot - rootNote;

    if (semitones === 0) {
      results.push(buffer);
    } else {
      const shifted = await pitchShiftSemitones(buffer, semitones);
      results.push(shifted);
    }

    onProgress?.(i + 1, sources.length);
  }

  return results;
}

/**
 * Match buffer lengths by zero-padding shorter buffers to the longest length.
 * This prevents artifacts when stacking buffers of different durations.
 */
function matchBufferLengths(buffers: AudioBuffer[]): AudioBuffer[] {
  if (buffers.length <= 1) return buffers;

  const maxLength = Math.max(...buffers.map((b) => b.length));
  // Normalize channel counts — mixing mono + stereo would produce mismatched
  // outputs. Duplicate mono channel data into the missing stereo channel.
  const maxChannels = Math.max(...buffers.map((b) => b.numberOfChannels));
  const sampleRate = buffers[0].sampleRate;
  const ctx = getAudioContext();

  return buffers.map((buf) => {
    if (buf.length === maxLength && buf.numberOfChannels === maxChannels) return buf;

    const padded = ctx.createBuffer(maxChannels, maxLength, sampleRate);
    for (let ch = 0; ch < maxChannels; ch++) {
      // If source has fewer channels, duplicate the last available channel
      const srcCh = Math.min(ch, buf.numberOfChannels - 1);
      const src = buf.getChannelData(srcCh);
      const dest = padded.getChannelData(ch);
      for (let i = 0; i < src.length; i++) {
        dest[i] = src[i];
      }
      // Remaining samples are 0 (silence) by default
    }
    return padded;
  });
}

// ---------------------------------------------------------------------------
// Main Builder
// ---------------------------------------------------------------------------

/**
 * Build a complete keygroup from 1-4 source samples.
 *
 * Pipeline:
 * 1. Decode all source buffers
 * 2. Pitch-match all sources to the target root note
 * 3. Match buffer lengths (zero-pad shorter samples)
 * 4. Generate 8 expression layers using the selected mode
 * 5. Return generated samples + pad assignments
 *
 * The resulting keygroup uses keyTrack=true so the MPC handles
 * transposition across the keyboard from the single root zone.
 */
export async function buildKeygroup(
  config: KeygroupBuilderConfig,
  onProgress?: (step: string, progress: number) => void
): Promise<KeygroupBuildResult> {
  const { sources, targetRoot, mode, variation, humanize, dualMode, programName } = config;

  if (sources.length === 0) {
    throw new Error('At least one source sample is required');
  }

  // ---- Step 1: Decode buffers ----
  onProgress?.('Decoding samples...', 5);
  const decodedSources: { buffer: AudioBuffer; rootNote: number }[] = [];

  for (const src of sources) {
    if (src.buffer) {
      decodedSources.push({ buffer: src.buffer, rootNote: src.rootNote });
    } else {
      const decoded = await decodeArrayBuffer(src.sample.buffer);
      decodedSources.push({ buffer: decoded, rootNote: src.rootNote });
    }
  }

  // ---- Step 2: Pitch-match to target root ----
  onProgress?.('Pitch matching...', 15);
  const matchedBuffers = await pitchMatchSources(
    decodedSources,
    targetRoot,
    (current, total) => {
      onProgress?.(`Pitch matching ${current}/${total}...`, 15 + (current / total) * 20);
    }
  );

  // ---- Step 3: Match buffer lengths ----
  onProgress?.('Aligning sample lengths...', 40);
  const alignedBuffers = matchBufferLengths(matchedBuffers);

  // ---- Step 4: Generate 8 expression layers ----
  onProgress?.('Generating expression layers...', 45);
  const baseName = programName
    .replace(/[^a-zA-Z0-9_-]/g, '_')
    .replace(/_+/g, '_')
    .replace(/^_|_$/g, '')
    .substring(0, 16);

  const generatedLayers = await generateVelocityLayers(
    alignedBuffers,
    baseName,
    targetRoot,
    { mode, variation, humanize, dualMode },
    (layerNum) => {
      onProgress?.(
        `Processing layer ${layerNum}/8...`,
        45 + (layerNum / 8) * 50
      );
    }
  );

  // ---- Step 5: Build result ----
  onProgress?.('Finalizing keygroup...', 98);

  // Collect all generated samples
  const allSamples = generatedLayers.map((l) => l.sample);

  // Determine play mode from expression mode metadata
  const { EXPRESSION_MODES } = await import('./autoLayerGenerator');
  const modeInfo = EXPRESSION_MODES.find((m) => m.id === mode);
  const playMode = (modeInfo?.playMode ?? 'velocity') as 'velocity' | 'cycle' | 'random';

  // Create a single pad assignment (pad 0) with all 8 layers
  const layerAssignments = [
    {
      padIndex: 0, // Caller should offset this to the active pad
      layers: generatedLayers,
      playMode,
    },
  ];

  onProgress?.('Complete!', 100);

  return {
    samples: allSamples,
    layerAssignments,
  };
}
