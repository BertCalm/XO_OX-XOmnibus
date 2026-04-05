export interface XpmVersion {
  fileVersion: string;
  application: string;
  appVersion: string;
  platform: string;
}

export interface XpmLayerData {
  number: number;
  active: boolean;
  volume: number;
  pan: number;
  pitch: number;
  tuneCoarse: number;
  tuneFine: number;
  velStart: number;
  velEnd: number;
  sampleName: string;
  sampleFile: string;
  rootNote: number;
  keyTrack: boolean;
  pitchRandom: number;
  volumeRandom: number;
  panRandom: number;
  sliceStart: number;
  sliceEnd: number;
  sliceLoop: number;
  sliceLoopStart: number;
  sliceLoopCrossFadeLength: number;
  sliceIndex: number;
  direction: number;
  offset: number;
  /** Probability of this layer triggering (0-100, default 100). MPC 2.x feature. */
  probability?: number;
}

export interface XpmInstrumentData {
  number: number;
  lowNote: number;
  highNote: number;
  ignoreBaseNote: boolean;
  zonePlay: number;
  triggerMode: number;
  filterType: number;
  cutoff: number;
  resonance: number;
  filterEnvAmt: number;
  filterAttack: number;
  filterDecay: number;
  filterSustain: number;
  filterRelease: number;
  filterHold: number;
  volumeAttack: number;
  volumeHold: number;
  volumeDecay: number;
  volumeSustain: number;
  volumeRelease: number;
  velocitySensitivity: number;
  pitchAttack: number;
  pitchHold: number;
  pitchDecay: number;
  pitchSustain: number;
  pitchRelease: number;
  pitchEnvAmount: number;
  layers: XpmLayerData[];
  modulation?: XpmModulationConfig;
}

export interface XpmProgramData {
  name: string;
  type: 'Keygroup' | 'Drum';
  volume: number;
  pan: number;
  pitch: number;
  tuneCoarse: number;
  tuneFine: number;
  send1: number;
  send2: number;
  send3: number;
  send4: number;
  mute: boolean;
  solo: boolean;
  instruments: XpmInstrumentData[];
  keygroupNumKeygroups: number;
  keygroupPitchBendRange: number;
  keygroupWheelToLfo: number;
  padNoteMap: { padNumber: number; note: number }[];
  padGroupMap: { padNumber: number; group: number }[];
}

export interface XpmFile {
  version: XpmVersion;
  program: XpmProgramData;
}

export interface GeneratedSample {
  fileName: string;
  data: ArrayBuffer;
  midiNote: number;
}

/** Modulation source types supported by MPC 2.x */
export type XpmModSource = 'Velocity' | 'Aftertouch' | 'ModWheel' | 'LFO1' | 'PitchEnv' | 'FilterEnv' | 'AmpEnv';

/** Modulation destination types */
export type XpmModDest = 'FilterCutoff' | 'FilterResonance' | 'Volume' | 'Pan' | 'Pitch' | 'LFO1Rate' | 'LFO1Depth';

export interface XpmModulationRoute {
  source: XpmModSource;
  destination: XpmModDest;
  amount: number; // -1.0 to 1.0
}

export interface XpmModulationConfig {
  routes: XpmModulationRoute[];
  /** LFO settings if any route uses LFO1 */
  lfo1?: {
    rate: number;    // 0.0-1.0 (normalized frequency)
    shape: number;   // 0=sine, 1=triangle, 2=saw, 3=square, 4=random
    sync: boolean;
  };
}
