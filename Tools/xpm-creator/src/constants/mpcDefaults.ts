export const XPM_VERSION = {
  fileVersion: '2.1',
  application: 'MPC-V',
  appVersion: '2.1.0.23',
  platform: 'Windows',
} as const;

export const XPM_DEFAULTS = {
  volume: 0.707946,
  pan: 0.500000,
  pitch: 0.000000,
  tuneCoarse: 0,
  tuneFine: 0,
  filterType: 0,
  filterCutoff: 1.000000,
  filterResonance: 0.000000,
  filterEnvAmt: 0.000000,
  volumeAttack: 0.000000,
  volumeHold: 0.000000,
  volumeDecay: 0.047244,
  volumeSustain: 1.000000,
  volumeRelease: 0.000000,
  velocitySensitivity: 1.000000,
  triggerMode: 0, // oneshot — MPC default for drum programs
  filterDecay: 0.000000, // independent from volumeDecay
  zonePlay: 1,
  pitchBendRange: 0.500000,
  wheelToLfo: 1.000000,
  send1: 0.000000,
  send2: 0.000000,
  send3: 0.000000,
  send4: 0.000000,
} as const;

export const LAYER_DEFAULTS = {
  active: true,
  volume: 1.000000,
  pan: 0.500000,
  pitch: 0.000000,
  tuneCoarse: 0,
  tuneFine: 0,
  velStart: 0,
  velEnd: 127,
  rootNote: 60,
  keyTrack: true,
  pitchRandom: 0.000000,
  volumeRandom: 0.000000,
  panRandom: 0.000000,
  sliceStart: 0,
  sliceEnd: 0,
  sliceLoop: 0,
  sliceLoopStart: 0,
  sliceLoopCrossFadeLength: 0,
  sliceIndex: 128,
  direction: 0,
  offset: 0,
  probability: 100,
} as const;

export const HUMANIZE_DEFAULTS = {
  tuneFineRange: 5,
  volumeRange: 0.04,
  panRange: 0.03,
  offsetRangeMs: 10,
  pitchRandomRange: 0.02,
} as const;

export const MAX_LAYERS_PER_PAD = 8;
export const MAX_KEYGROUPS = 128;
export const MAX_PADS = 128;
export const MIDI_NOTE_RANGE = { min: 0, max: 127 };
/** MPC hardware / XML compatibility value — not a runtime default. Use AudioContext.sampleRate at runtime. */
export const MPC_LEGACY_SAMPLE_RATE = 44100;
export const DEFAULT_BIT_DEPTH = 16;

export const TRIGGER_MODES = {
  oneshot: 0,
  noteoff: 1,
  noteon: 2,
} as const;

export const ZONE_PLAY_MODES = {
  simultaneous: 1,
  velocity: 2,
  cycle: 3,
  random: 4,
} as const;

export const MPC_PAD_NOTE_MAP_DEFAULT: number[] = Array.from(
  { length: 128 },
  (_, i) => i
);

export const MPC_PAD_GROUP_MAP_DEFAULT: number[] = Array.from(
  { length: 128 },
  () => 0
);
