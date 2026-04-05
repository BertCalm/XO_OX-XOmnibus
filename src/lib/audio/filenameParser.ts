import { noteNameToMidi } from '@/types';

/**
 * Parsed metadata extracted from a sample filename.
 */
export interface FilenameMetadata {
  /** Detected root MIDI note (0-127), or null if not found */
  rootNote: number | null;
  /** Detected velocity layer index (1-based), or null if not found */
  velocityLayer: number | null;
  /** Detected velocity value (0-127), or null if not found */
  velocityValue: number | null;
  /** The "clean" name portion of the filename (without extension/note/velocity) */
  cleanName: string;
}

/**
 * Note name regex: matches note names like C1, C#3, Db4, Fs2, etc.
 * Supports both sharp (#/s/S) and flat (b) notation.
 * The note must be followed by an octave number (-2 to 8).
 */
const NOTE_PATTERN = /(?:^|[_\-\s.])([A-Ga-g])([#sSbB]?)(-?[0-8])(?:[_\-\s.]|$)/;

/**
 * Velocity layer pattern: matches V1, V2, vel1, vel2, Layer1, L1, etc.
 */
const VEL_LAYER_PATTERN = /(?:^|[_\-\s.])(v|vel|layer|l)(\d{1,2})(?:[_\-\s.]|$)/i;

/**
 * Velocity value pattern: matches vel127, velocity100, etc.
 */
const VEL_VALUE_PATTERN = /(?:^|[_\-\s.])vel(?:ocity)?(\d{1,3})(?:[_\-\s.]|$)/i;

/**
 * MIDI note number pattern: matches explicit midi note references like "note60", "midi60", "n60"
 */
const MIDI_NOTE_PATTERN = /(?:^|[_\-\s.])(note|midi|n)(\d{1,3})(?:[_\-\s.]|$)/i;

/**
 * Parse a sample filename to extract mapping metadata.
 *
 * Handles various naming conventions:
 * - "Bass_C1.wav" → rootNote: 36
 * - "Piano_Cs3_V1.wav" → rootNote: 49, velocityLayer: 1
 * - "Snare_vel80.wav" → velocityValue: 80
 * - "Pad_Db4_Layer2.wav" → rootNote: 61, velocityLayer: 2
 * - "String_note60.wav" → rootNote: 60
 * - "Kick_F#2.wav" → rootNote: 42
 *
 * @param filename - The sample filename (with or without extension)
 * @returns Parsed metadata
 */
export function parseFilenameForMapping(filename: string): FilenameMetadata {
  // Remove file extension
  const nameWithoutExt = filename.replace(/\.(wav|mp3|ogg|flac|aif|aiff|m4a)$/i, '');

  let rootNote: number | null = null;
  let velocityLayer: number | null = null;
  let velocityValue: number | null = null;
  let cleanName = nameWithoutExt;

  // Try to extract MIDI note number first (most explicit)
  const midiMatch = nameWithoutExt.match(MIDI_NOTE_PATTERN);
  if (midiMatch) {
    const midiNum = parseInt(midiMatch[2], 10);
    if (midiNum >= 0 && midiNum <= 127) {
      rootNote = midiNum;
      cleanName = cleanName.replace(MIDI_NOTE_PATTERN, ' ').trim();
    }
  }

  // Try to extract note name (C#3, Db4, Fs2, etc.)
  if (rootNote === null) {
    const noteMatch = nameWithoutExt.match(NOTE_PATTERN);
    if (noteMatch) {
      const letter = noteMatch[1].toUpperCase();
      let accidental = noteMatch[2];
      const octave = noteMatch[3];

      // Normalize accidentals: 's' or 'S' means sharp
      if (accidental === 's' || accidental === 'S') accidental = '#';
      if (accidental === 'B' && letter !== 'B') accidental = 'b'; // uppercase B only if not the note B

      const noteName = `${letter}${accidental}${octave}`;
      const midi = noteNameToMidi(noteName);
      if (midi >= 0 && midi <= 127) {
        rootNote = midi;
        cleanName = cleanName.replace(NOTE_PATTERN, ' ').trim();
      }
    }
  }

  // Try to extract velocity layer number (V1, Layer2, L3)
  const velLayerMatch = nameWithoutExt.match(VEL_LAYER_PATTERN);
  if (velLayerMatch) {
    const layerNum = parseInt(velLayerMatch[2], 10);
    if (layerNum >= 1 && layerNum <= 8) {
      velocityLayer = layerNum;
      cleanName = cleanName.replace(VEL_LAYER_PATTERN, ' ').trim();
    }
  }

  // Try to extract velocity value (vel80, velocity127)
  const velValueMatch = nameWithoutExt.match(VEL_VALUE_PATTERN);
  if (velValueMatch) {
    const velVal = parseInt(velValueMatch[1], 10);
    if (velVal >= 0 && velVal <= 127) {
      velocityValue = velVal;
      // Don't remove from cleanName if it wasn't already captured by velLayerMatch
      if (!velLayerMatch) {
        cleanName = cleanName.replace(VEL_VALUE_PATTERN, ' ').trim();
      }
    }
  }

  // Clean up the name: remove extra separators
  cleanName = cleanName
    .replace(/[_\-]+/g, ' ')
    .replace(/\s+/g, ' ')
    .trim();

  return {
    rootNote,
    velocityLayer,
    velocityValue,
    cleanName: cleanName || nameWithoutExt,
  };
}

/**
 * Auto-map velocity ranges for a set of velocity layers.
 * Given N layers, divides 0-127 into N equal ranges.
 *
 * @param layerCount - Total number of velocity layers
 * @param layerIndex - 0-based index of this layer
 * @returns { velStart, velEnd } for this layer
 */
export function autoMapVelocityRange(
  layerCount: number,
  layerIndex: number
): { velStart: number; velEnd: number } {
  if (layerCount <= 1) return { velStart: 0, velEnd: 127 };

  const rangeSize = Math.floor(128 / layerCount);
  const velStart = layerIndex * rangeSize;
  const velEnd = layerIndex === layerCount - 1 ? 127 : velStart + rangeSize - 1;

  return { velStart, velEnd };
}
