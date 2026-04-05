/**
 * Web MIDI API wrapper for triggering pads from hardware controllers.
 * Maps incoming MIDI notes to pad indices using standard MPC note mapping.
 */

export type MidiNoteHandler = (padIndex: number, velocity: number) => void;
export type MidiNoteOffHandler = (padIndex: number) => void;
export type MidiDeviceChangeHandler = (inputs: MIDIInput[]) => void;

interface MidiInputState {
  access: MIDIAccess | null;
  activeInput: MIDIInput | null;
  noteOnHandler: MidiNoteHandler | null;
  noteOffHandler: MidiNoteOffHandler | null;
  deviceChangeHandler: MidiDeviceChangeHandler | null;
  noteTopadMap: Map<number, number>;
  padToNoteMap: Map<number, number>;
  currentBank: number;
}

// Standard MPC drum pad MIDI note mapping (per bank, 16 pads each)
const BANK_NOTE_MAPS: number[][] = [
  // Bank A
  [37, 36, 42, 82, 40, 38, 46, 44, 49, 55, 57, 56, 69, 54, 70, 81],
  // Bank B
  [37 + 16, 36 + 16, 42 + 16, 82 + 16, 40 + 16, 38 + 16, 46 + 16, 44 + 16,
   49 + 16, 55 + 16, 57 + 16, 56 + 16, 69 + 16, 54 + 16, 70 + 16, 81 + 16].map(n => Math.min(n, 127)),
];

// Simple sequential mapping fallback (note 36-51 = pads 0-15, etc.)
function buildSequentialMap(bankOffset: number): Map<number, number> {
  const map = new Map<number, number>();
  for (let i = 0; i < 16; i++) {
    const note = 36 + bankOffset * 16 + i;
    if (note <= 127) {
      map.set(note, bankOffset * 16 + i);
    }
  }
  return map;
}

const state: MidiInputState = {
  access: null,
  activeInput: null,
  noteOnHandler: null,
  noteOffHandler: null,
  deviceChangeHandler: null,
  noteTopadMap: new Map(),
  padToNoteMap: new Map(),
  currentBank: 0,
};

/** Check if Web MIDI is supported in this browser */
export function isMidiSupported(): boolean {
  return typeof navigator !== 'undefined' && 'requestMIDIAccess' in navigator;
}

/** Build note-to-pad mapping for the current bank */
function buildNoteMap(bank: number): void {
  state.noteTopadMap.clear();
  state.padToNoteMap.clear();

  if (bank < BANK_NOTE_MAPS.length) {
    // Use MPC standard mapping for known banks
    const notes = BANK_NOTE_MAPS[bank];
    for (let i = 0; i < notes.length; i++) {
      const globalPadIdx = bank * 16 + i;
      state.noteTopadMap.set(notes[i], globalPadIdx);
      state.padToNoteMap.set(globalPadIdx, notes[i]);
    }
  } else {
    // Use sequential mapping for higher banks
    const seqMap = buildSequentialMap(bank);
    seqMap.forEach((padIdx, note) => {
      state.noteTopadMap.set(note, padIdx);
      state.padToNoteMap.set(padIdx, note);
    });
  }

  // Only mapped notes trigger pads — unmapped notes are ignored to prevent
  // ghost triggers from chromatic keys or other unrelated MIDI data.
}

/** Handle incoming MIDI messages */
function handleMidiMessage(event: MIDIMessageEvent): void {
  const data = event.data;
  if (!data || data.length < 2) return;

  const status = data[0] & 0xf0;
  const note = data[1];
  const velocity = data.length > 2 ? data[2] : 0;

  if (status === 0x90 && velocity > 0) {
    // Note On
    const padIndex = state.noteTopadMap.get(note);
    if (padIndex !== undefined && state.noteOnHandler) {
      state.noteOnHandler(padIndex, velocity);
    }
  } else if (status === 0x80 || (status === 0x90 && velocity === 0)) {
    // Note Off
    const padIndex = state.noteTopadMap.get(note);
    if (padIndex !== undefined && state.noteOffHandler) {
      state.noteOffHandler(padIndex);
    }
  }
}

/** Error thrown when MIDI permission is denied by the user or browser */
export class MidiPermissionDeniedError extends Error {
  constructor(message: string = 'MIDI access was denied by the browser or user') {
    super(message);
    this.name = 'MidiPermissionDeniedError';
  }
}

/** Request MIDI access and enumerate devices */
export async function initMidi(): Promise<MIDIInput[]> {
  if (!isMidiSupported()) {
    return [];
  }

  try {
    state.access = await navigator.requestMIDIAccess({ sysex: false });

    // Listen for device changes (hot-plug)
    state.access.onstatechange = () => {
      if (state.deviceChangeHandler) {
        state.deviceChangeHandler(getAvailableInputs());
      }
    };

    buildNoteMap(state.currentBank);
    return getAvailableInputs();
  } catch (err) {
    // Distinguish permission denial from other failures so the UI can show
    // "permission denied" instead of misleading "no devices found"
    if (err instanceof DOMException && (err.name === 'SecurityError' || err.name === 'NotAllowedError')) {
      throw new MidiPermissionDeniedError(
        'MIDI access denied. Check your browser permissions to enable MIDI input.',
      );
    }
    // Log and re-throw so the UI can show an appropriate error toast
    // instead of silently showing "no devices found".
    console.warn('MIDI initialization failed:', err);
    throw new Error(
      `MIDI initialization failed: ${err instanceof Error ? err.message : 'unknown error'}`,
    );
  }
}

/** Get all available MIDI input devices */
export function getAvailableInputs(): MIDIInput[] {
  if (!state.access) return [];
  const inputs: MIDIInput[] = [];
  state.access.inputs.forEach((input) => {
    inputs.push(input);
  });
  return inputs;
}

/** Connect to a specific MIDI input device */
export function connectInput(input: MIDIInput): void {
  // Disconnect previous
  if (state.activeInput) {
    state.activeInput.onmidimessage = null;
  }

  state.activeInput = input;
  input.onmidimessage = handleMidiMessage;
}

/** Disconnect the current MIDI input */
export function disconnectInput(): void {
  if (state.activeInput) {
    state.activeInput.onmidimessage = null;
    state.activeInput = null;
  }
}

/** Set the callback for MIDI note-on events */
export function onNoteOn(handler: MidiNoteHandler): void {
  state.noteOnHandler = handler;
}

/** Set the callback for MIDI note-off events */
export function onNoteOff(handler: MidiNoteOffHandler): void {
  state.noteOffHandler = handler;
}

/** Set the callback for device connection/disconnection */
export function onDeviceChange(handler: MidiDeviceChangeHandler): void {
  state.deviceChangeHandler = handler;
}

/** Switch the active bank for note mapping */
export function setMidiBank(bank: number): void {
  state.currentBank = bank;
  buildNoteMap(bank);
}

/** Get the currently connected input device */
export function getActiveInput(): MIDIInput | null {
  return state.activeInput;
}

/** Clean up all MIDI connections */
export function disposeMidi(): void {
  disconnectInput();
  state.noteOnHandler = null;
  state.noteOffHandler = null;
  state.deviceChangeHandler = null;
  state.access = null;
}
