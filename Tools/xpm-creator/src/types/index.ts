export type ProgramType = 'Keygroup' | 'Drum';

export type LayerPlayMode = 'simultaneous' | 'velocity' | 'cycle' | 'random';

export type TriggerMode = 'oneshot' | 'noteoff' | 'noteon';

export interface AudioSample {
  id: string;
  name: string;
  fileName: string;
  duration: number;
  sampleRate: number;
  channels: number;
  bitDepth: number;
  buffer: ArrayBuffer;
  waveformPeaks?: number[];
  rootNote: number;
  createdAt: number;
  /** User-assigned tags for filtering and organization */
  tags?: string[];
  /** Whether the user has favorited this sample */
  isFavorite?: boolean;
}

export interface ChopRegion {
  id: string;
  start: number;
  end: number;
  name: string;
  sampleId?: string;
}

export interface PadLayer {
  number: 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8;
  active: boolean;
  sampleId: string | null;
  sampleName: string;
  sampleFile: string;
  volume: number;
  pan: number;
  pitch: number;
  tuneCoarse: number;
  tuneFine: number;
  velStart: number;
  velEnd: number;
  rootNote: number;
  keyTrack: boolean;
  pitchRandom: number;
  volumeRandom: number;
  panRandom: number;
  sliceStart: number;
  sliceEnd: number;
  sliceLoop: number;
  sliceLoopStart: number;
  offset: number;
  direction: number;
  /** Probability of this layer triggering (0-100, default 100). Used for generative kits. */
  probability: number;
}

export interface PadAssignment {
  padNumber: number;
  layers: PadLayer[];
  playMode: LayerPlayMode;
  triggerMode: TriggerMode;
  muteGroup: number;
}

export interface InstrumentEnvelope {
  attack: number;
  hold: number;
  decay: number;
  sustain: number;
  release: number;
}

export interface InstrumentFilter {
  type: number;
  cutoff: number;
  resonance: number;
  envAmount: number;
  envelope: InstrumentEnvelope;
}

export interface Instrument {
  number: number;
  lowNote: number;
  highNote: number;
  zonePlay: number;
  triggerMode: number;
  ignoreBaseNote: boolean;
  filter: InstrumentFilter;
  volumeEnvelope: InstrumentEnvelope;
  pitchEnvelope: InstrumentEnvelope & { amount: number };
  velocitySensitivity: number;
  layers: PadLayer[];
}

export interface XpmProgram {
  id: string;
  name: string;
  type: ProgramType;
  volume: number;
  pan: number;
  pitch: number;
  tuneCoarse: number;
  tuneFine: number;
  instruments: Instrument[];
  padNoteMap: { padNumber: number; note: number }[];
  padGroupMap: { padNumber: number; group: number }[];
  keygroupCount: number;
  pitchBendRange: number;
}

export interface XpnExpansion {
  id: string;
  name: string;
  identifier: string;
  description: string;
  tags: string[];
  coverArt: ArrayBuffer | null;
  coverArtType: 'jpg' | 'png';
  programs: XpmProgram[];
  sampleFiles: Map<string, ArrayBuffer>;
  createdAt: number;
}

export interface ExportStep {
  id: string;
  label: string;
  description: string;
  progress: number;
  status: 'pending' | 'active' | 'complete' | 'error';
  detail?: string;
}

/** Snapshot of a Zustand store persisted alongside a project in IndexedDB. */
export interface StoreSnapshot {
  id: string;       // e.g. 'envelopes-<projectId>'
  data: string;     // JSON-stringified store state
  projectId: string;
}

export interface Project {
  id: string;
  name: string;
  createdAt: number;
  updatedAt: number;
  programType: ProgramType;
  samples: AudioSample[];
  padAssignments: PadAssignment[];
  programs: XpmProgram[];
  /** Envelope/modulation/expression snapshots restored from IndexedDB. */
  storeSnapshots?: StoreSnapshot[];
}

export const NOTE_NAMES = [
  'C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B'
] as const;

export function midiNoteToName(note: number): string {
  // Guard: clamp to valid MIDI range (0-127) to prevent "undefined" names
  const clamped = Math.max(0, Math.min(127, Math.round(note)));
  // MPC convention: MIDI 60 = C4 (middle C) → offset = -1
  const octave = Math.floor(clamped / 12) - 1;
  const name = NOTE_NAMES[clamped % 12];
  return `${name}${octave}`;
}

export function noteNameToMidi(name: string): number {
  const match = name.match(/^([A-G])(#|b)?(-?\d+)$/);
  if (!match) return 60; // Default to middle C for unparseable input
  const [, letter, accidental, octaveStr] = match;
  const baseNotes: Record<string, number> = {
    C: 0, D: 2, E: 4, F: 5, G: 7, A: 9, B: 11,
  };
  let note = baseNotes[letter] ?? 0;
  if (accidental === '#') note += 1;
  if (accidental === 'b') note -= 1;
  // Match midiNoteToName: octave offset = -1 (C4 = MIDI 60)
  const octave = parseInt(octaveStr, 10);
  if (Number.isNaN(octave)) return 60;
  // Clamp result to valid MIDI range
  return Math.max(0, Math.min(127, note + (octave + 1) * 12));
}
