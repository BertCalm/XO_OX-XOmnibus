import type { XpmProgramData, XpmInstrumentData } from './xpmTypes';
import { getDefaultInstrument, getDefaultLayer } from './xpmTemplate';
import { XPM_DEFAULTS, TRIGGER_MODES, ZONE_PLAY_MODES } from '@/constants/mpcDefaults';
import { generateXpmXml } from './xpmGenerator';
import type { PadAssignment } from '@/types';
import { useEnvelopeStore } from '@/stores/envelopeStore';
import { useModulationStore } from '@/stores/modulationStore';

/**
 * Standard MPC drum pad MIDI note mapping (Bank A, pads 1-16)
 * Pads are numbered bottom-left to top-right:
 * 13 14 15 16
 *  9 10 11 12
 *  5  6  7  8
 *  1  2  3  4
 */
const DRUM_PAD_NOTES_BANK_A = [
  37, 36, 42, 82, // Row 1 (bottom): Pad 1-4
  40, 38, 46, 44, // Row 2: Pad 5-8
  48, 47, 45, 43, // Row 3: Pad 9-12
  49, 55, 51, 53, // Row 4 (top): Pad 13-16
];

export interface DrumKitConfig {
  programName: string;
  padAssignments: PadAssignment[];
}

/**
 * Build a Drum-type XPM program from pad assignments.
 * Each pad becomes an Instrument with up to 8 layers.
 */
export function buildDrumProgram(config: DrumKitConfig): string {
  const { programName, padAssignments } = config;

  const instruments: XpmInstrumentData[] = [];

  // Read all envelope and modulation settings once (snapshot) for export
  const envelopeState = useEnvelopeStore.getState();
  const modulationState = useModulationStore.getState();

  for (let i = 0; i < padAssignments.length; i++) {
    const pad = padAssignments[i];
    const hasAnySample = pad.layers.some((l) => l.active && l.sampleId);
    if (!hasAnySample) continue;

    const midiNote = i < DRUM_PAD_NOTES_BANK_A.length
      ? DRUM_PAD_NOTES_BANK_A[i]
      : Math.min(127, 54 + (i - DRUM_PAD_NOTES_BANK_A.length));

    const instrument = getDefaultInstrument(i + 1);
    instrument.lowNote = midiNote;
    instrument.highNote = midiNote;
    instrument.triggerMode = TRIGGER_MODES[pad.triggerMode];
    instrument.zonePlay = ZONE_PLAY_MODES[pad.playMode];

    // Write envelope & filter settings from the envelope store
    const envSettings = envelopeState.getEnvelope(i);

    // Volume envelope (AHDSR)
    instrument.volumeAttack = envSettings.volumeEnvelope.attack;
    instrument.volumeHold = envSettings.volumeEnvelope.hold;
    instrument.volumeDecay = envSettings.volumeEnvelope.decay;
    instrument.volumeSustain = envSettings.volumeEnvelope.sustain;
    instrument.volumeRelease = envSettings.volumeEnvelope.release;

    // Filter settings
    instrument.filterType = envSettings.filterType;
    instrument.cutoff = envSettings.filterCutoff;
    instrument.resonance = envSettings.filterResonance;
    // Clamp to MPC's bipolar range [-1, 1] before XML output
    instrument.filterEnvAmt = Math.max(-1, Math.min(1, envSettings.filterEnvAmount));

    // Filter envelope (AHDSR)
    instrument.filterAttack = envSettings.filterEnvelope.attack;
    instrument.filterHold = envSettings.filterEnvelope.hold;
    instrument.filterDecay = envSettings.filterEnvelope.decay;
    instrument.filterSustain = envSettings.filterEnvelope.sustain;
    instrument.filterRelease = envSettings.filterEnvelope.release;

    // Map layers
    for (let l = 0; l < pad.layers.length; l++) {
      const srcLayer = pad.layers[l];
      if (!srcLayer.active || !srcLayer.sampleId) {
        instrument.layers[l] = getDefaultLayer(l + 1);
        continue;
      }

      instrument.layers[l] = {
        number: l + 1,
        active: true,
        volume: srcLayer.volume,
        pan: srcLayer.pan,
        pitch: srcLayer.pitch,
        tuneCoarse: srcLayer.tuneCoarse,
        tuneFine: srcLayer.tuneFine,
        velStart: srcLayer.velStart,
        velEnd: srcLayer.velEnd,
        sampleName: srcLayer.sampleName,
        // Normalize to .WAV — MPC runs Linux (case-sensitive FS)
        sampleFile: srcLayer.sampleFile.replace(/\.wav$/i, '.WAV'),
        rootNote: srcLayer.rootNote,
        keyTrack: srcLayer.keyTrack,
        pitchRandom: srcLayer.pitchRandom,
        volumeRandom: srcLayer.volumeRandom,
        panRandom: srcLayer.panRandom,
        sliceStart: srcLayer.sliceStart,
        sliceEnd: srcLayer.sliceEnd,
        sliceLoop: srcLayer.sliceLoop,
        sliceLoopStart: srcLayer.sliceLoopStart,
        sliceLoopCrossFadeLength: 0,
        sliceIndex: 128,
        direction: srcLayer.direction,
        offset: srcLayer.offset,
        probability: srcLayer.probability,
      };
    }

    // Write modulation routing from the modulation store
    const modConfig = modulationState.getModulation(i);
    if (modConfig.routes.length > 0) {
      instrument.modulation = {
        routes: modConfig.routes.map((r) => ({ ...r })),
        lfo1: modConfig.lfo1 ? { ...modConfig.lfo1 } : undefined,
      };
    }

    instruments.push(instrument);
  }

  // Build pad note map — cap at 64 pads (4 banks × 16 pads).
  // MPC firmware expects max 64; some versions reject 128-entry maps as corrupt.
  const DRUM_PAD_COUNT = 64;
  const padNoteMap = Array.from({ length: DRUM_PAD_COUNT }, (_, i) => ({
    padNumber: i + 1,
    note: i < DRUM_PAD_NOTES_BANK_A.length ? DRUM_PAD_NOTES_BANK_A[i] : Math.min(127, 54 + (i - DRUM_PAD_NOTES_BANK_A.length)),
  }));

  // Build pad group map (matches pad count)
  const padGroupMap = Array.from({ length: DRUM_PAD_COUNT }, (_, i) => {
    const pad = padAssignments[i];
    return {
      padNumber: i + 1,
      group: pad ? pad.muteGroup : 0,
    };
  });

  const programData: XpmProgramData = {
    name: programName,
    type: 'Drum',
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
    keygroupNumKeygroups: 0,
    keygroupPitchBendRange: XPM_DEFAULTS.pitchBendRange,
    keygroupWheelToLfo: XPM_DEFAULTS.wheelToLfo,
    padNoteMap,
    padGroupMap,
  };

  return generateXpmXml(programData);
}
