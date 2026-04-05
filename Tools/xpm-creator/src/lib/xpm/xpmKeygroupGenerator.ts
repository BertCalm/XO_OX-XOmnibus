import type { XpmProgramData, XpmInstrumentData, GeneratedSample } from './xpmTypes';
import { getDefaultInstrument, getDefaultLayer } from './xpmTemplate';
import { XPM_DEFAULTS, MAX_LAYERS_PER_PAD } from '@/constants/mpcDefaults';
import { generateXpmXml } from './xpmGenerator';
import { pitchShiftSemitones } from '@/lib/audio/pitchShifter';
import { encodeWavAsync } from '@/lib/audio/wavEncoder';
import { midiNoteToName } from '@/types';
import type { PadAssignment } from '@/types';
import { useEnvelopeStore } from '@/stores/envelopeStore';
import { useModulationStore } from '@/stores/modulationStore';

export interface KeygroupConfig {
  programName: string;
  sourceBuffer: AudioBuffer;
  rootNote: number;
  lowNote: number;
  highNote: number;
  layers: PadAssignment['layers'];
  bitDepth?: number;
  /** Pad index to read envelope/modulation settings from (defaults to 0) */
  padIndex?: number;
  onProgress?: (step: string, progress: number) => void;
}

interface KeygroupResult {
  xpmContent: string;
  samples: GeneratedSample[];
}

/**
 * Build a Keygroup XPM program with pre-rendered pitch-shifted samples.
 * Each note in the range gets its own WAV file pitched from the source.
 */
export async function buildKeygroupProgram(
  config: KeygroupConfig
): Promise<KeygroupResult> {
  const { programName, sourceBuffer, rootNote, lowNote, highNote, layers, bitDepth = 16, padIndex = 0, onProgress } = config;

  // Validate note range
  if (lowNote > highNote) {
    throw new Error(`Invalid keygroup range: lowNote (${lowNote}) > highNote (${highNote})`);
  }

  // MPC firmware supports a maximum of 128 keygroups per program
  const MAX_KEYGROUPS = 128;
  const totalNotes = highNote - lowNote + 1;
  if (totalNotes > MAX_KEYGROUPS) {
    throw new Error(
      `Keygroup range of ${totalNotes} notes exceeds MPC's ${MAX_KEYGROUPS}-keygroup limit. ` +
      `Reduce the range (currently ${lowNote}–${highNote}).`
    );
  }

  const generatedSamples: GeneratedSample[] = [];
  const instruments: XpmInstrumentData[] = [];

  // Read envelope and modulation settings once (snapshot) for export
  const envSettings = useEnvelopeStore.getState().getEnvelope(padIndex);
  const modConfig = useModulationStore.getState().getModulation(padIndex);

  // Determine how many notes per keygroup instrument
  // For multi-sampled instruments, we create one instrument per pitched sample
  for (let note = lowNote; note <= highNote; note++) {
    const semitones = note - rootNote;
    const noteProgress = ((note - lowNote) / totalNotes) * 100;

    onProgress?.(`Pitch-shifting to ${midiNoteToName(note)}...`, noteProgress);

    // Pitch shift the source
    const shiftedBuffer = await pitchShiftSemitones(sourceBuffer, semitones);

    // Create the sample file name — truncate to fit MPC's 32-char filename limit
    const baseName = sanitizeFileName(programName);
    const noteName = midiNoteToName(note).replace('#', 's');
    const suffix = `_${noteName}.WAV`;
    // Reserve space for suffix, truncate baseName if needed
    const maxBase = 32 - suffix.length;
    const truncatedBase = baseName.length > maxBase ? baseName.substring(0, maxBase) : baseName;
    const sampleFileName = `${truncatedBase}${suffix}`;
    const sampleName = `${truncatedBase}_${noteName}`;

    // Encode to WAV (async — non-blocking)
    const wavData = await encodeWavAsync(shiftedBuffer, bitDepth);
    generatedSamples.push({
      fileName: sampleFileName,
      data: wavData,
      midiNote: note,
    });

    // Create the instrument (keygroup) for this note
    const instrument = getDefaultInstrument(note - lowNote + 1);
    instrument.lowNote = note;
    instrument.highNote = note;

    // Apply envelope & filter settings (same as drum program export)
    instrument.volumeAttack = envSettings.volumeEnvelope.attack;
    instrument.volumeHold = envSettings.volumeEnvelope.hold;
    instrument.volumeDecay = envSettings.volumeEnvelope.decay;
    instrument.volumeSustain = envSettings.volumeEnvelope.sustain;
    instrument.volumeRelease = envSettings.volumeEnvelope.release;
    instrument.filterType = envSettings.filterType;
    instrument.cutoff = envSettings.filterCutoff;
    instrument.resonance = envSettings.filterResonance;
    // Clamp to MPC's bipolar range [-1, 1] before XML output
    instrument.filterEnvAmt = Math.max(-1, Math.min(1, envSettings.filterEnvAmount));
    instrument.filterAttack = envSettings.filterEnvelope.attack;
    instrument.filterHold = envSettings.filterEnvelope.hold;
    instrument.filterDecay = envSettings.filterEnvelope.decay;
    instrument.filterSustain = envSettings.filterEnvelope.sustain;
    instrument.filterRelease = envSettings.filterEnvelope.release;

    // Apply modulation routing
    if (modConfig.routes.length > 0) {
      instrument.modulation = {
        routes: modConfig.routes.map((r) => ({ ...r })),
        lfo1: modConfig.lfo1 ? { ...modConfig.lfo1 } : undefined,
      };
    }

    // Set up the primary layer
    instrument.layers[0] = {
      ...getDefaultLayer(1),
      active: true,
      sampleName,
      sampleFile: sampleFileName,
      rootNote: note,
      keyTrack: false, // Each sample is pre-pitched, no keytrack needed
    };

    // Copy additional layers if provided (for layered sounds)
    if (layers && layers.length > 1) {
      for (let i = 1; i < layers.length && i < MAX_LAYERS_PER_PAD; i++) {
        const srcLayer = layers[i];
        if (srcLayer.active && srcLayer.sampleId) {
          instrument.layers[i] = {
            ...getDefaultLayer(i + 1),
            active: true,
            sampleName: srcLayer.sampleName,
            sampleFile: srcLayer.sampleFile.replace(/\.wav$/i, '.WAV'),
            rootNote: note,
            volume: srcLayer.volume,
            pan: srcLayer.pan,
            pitch: srcLayer.pitch,
            tuneCoarse: srcLayer.tuneCoarse,
            tuneFine: srcLayer.tuneFine,
            velStart: srcLayer.velStart,
            velEnd: srcLayer.velEnd,
            pitchRandom: srcLayer.pitchRandom,
            volumeRandom: srcLayer.volumeRandom,
            panRandom: srcLayer.panRandom,
            offset: srcLayer.offset,
          };
        }
      }
    }

    instruments.push(instrument);
  }

  onProgress?.('Building XPM structure...', 90);

  // Build the program data
  const programData: XpmProgramData = {
    name: programName,
    type: 'Keygroup',
    volume: XPM_DEFAULTS.volume,
    pan: XPM_DEFAULTS.pan,
    pitch: XPM_DEFAULTS.pitch,
    tuneCoarse: XPM_DEFAULTS.tuneCoarse,
    tuneFine: XPM_DEFAULTS.tuneFine,
    send1: XPM_DEFAULTS.send1,
    send2: XPM_DEFAULTS.send2,
    send3: XPM_DEFAULTS.send3,
    send4: XPM_DEFAULTS.send4,
    mute: false,
    solo: false,
    instruments,
    keygroupNumKeygroups: instruments.length,
    keygroupPitchBendRange: XPM_DEFAULTS.pitchBendRange,
    keygroupWheelToLfo: XPM_DEFAULTS.wheelToLfo,
    padNoteMap: [],
    padGroupMap: [],
  };

  onProgress?.('Generating XML...', 95);
  const xpmContent = generateXpmXml(programData);

  onProgress?.('Complete', 100);

  return {
    xpmContent,
    samples: generatedSamples,
  };
}

/**
 * Build a simpler keygroup that uses keytrack (single sample, MPC handles pitch).
 * Useful for quick creation where pre-rendering is not needed.
 */
export function buildSimpleKeygroupProgram(
  programName: string,
  sampleName: string,
  sampleFile: string,
  rootNote: number
): string {
  const instrument = getDefaultInstrument(1);
  instrument.lowNote = 0;
  instrument.highNote = 127;

  instrument.layers[0] = {
    ...getDefaultLayer(1),
    active: true,
    sampleName,
    sampleFile,
    rootNote,
    keyTrack: true,
  };

  const programData: XpmProgramData = {
    name: programName,
    type: 'Keygroup',
    volume: XPM_DEFAULTS.volume,
    pan: XPM_DEFAULTS.pan,
    pitch: XPM_DEFAULTS.pitch,
    tuneCoarse: XPM_DEFAULTS.tuneCoarse,
    tuneFine: XPM_DEFAULTS.tuneFine,
    send1: XPM_DEFAULTS.send1,
    send2: XPM_DEFAULTS.send2,
    send3: XPM_DEFAULTS.send3,
    send4: XPM_DEFAULTS.send4,
    mute: false,
    solo: false,
    instruments: [instrument],
    keygroupNumKeygroups: 1,
    keygroupPitchBendRange: XPM_DEFAULTS.pitchBendRange,
    keygroupWheelToLfo: XPM_DEFAULTS.wheelToLfo,
    padNoteMap: [],
    padGroupMap: [],
  };

  return generateXpmXml(programData);
}

function sanitizeFileName(name: string): string {
  return name
    .replace(/[^a-zA-Z0-9_-]/g, '_')
    .replace(/_+/g, '_')
    .replace(/^_|_$/g, '')
    || 'Sample'; // Fallback — empty sanitized name produces _C4.WAV filenames
}
