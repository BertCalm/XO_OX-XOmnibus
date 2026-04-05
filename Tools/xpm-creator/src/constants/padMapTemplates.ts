/**
 * Pad Map Templates
 *
 * Pre-configured MIDI note mappings for the 128-pad grid.
 * Each template maps pad indices → MIDI note numbers, following
 * different hardware conventions and musical layouts.
 */

export interface PadMapTemplate {
  id: string;
  name: string;
  description: string;
  /** MIDI note for each pad index (0-127). Length must be 128. */
  noteMap: number[];
}

/**
 * MPC Classic layout — Bank A starts at MIDI 37 (C#2), the traditional
 * MPC60/2000/3000 mapping. Banks B-H continue sequentially.
 */
function buildMpcClassicMap(): number[] {
  // Classic MPC: Pad 1 = 37, Pad 2 = 36, Pad 3 = 42, Pad 4 = 82, ...
  // Bank A follows the iconic 4x4 bottom-left-to-top-right order
  const bankA = [37, 36, 42, 82, 40, 38, 46, 44, 48, 47, 45, 43, 49, 55, 51, 53];
  const map: number[] = [];
  for (let bank = 0; bank < 8; bank++) {
    if (bank === 0) {
      map.push(...bankA);
    } else {
      // Banks B-H: sequential from note 54 onward (skipping Bank A notes)
      for (let pad = 0; pad < 16; pad++) {
        map.push(Math.min(127, 54 + (bank - 1) * 16 + pad));
      }
    }
  }
  return map;
}

/**
 * GM Standard — General MIDI drum map. Pad 1 = Bass Drum (35),
 * laid out to match GM percussion channel 10 conventions.
 */
function buildGmStandardMap(): number[] {
  // GM drum sounds: 35=Acoustic Bass Drum through 81=Open Triangle
  // Bank A maps the core kit (kick, snare, hats, toms, cymbals)
  const bankA = [
    35, // Acoustic Bass Drum
    36, // Bass Drum 1
    38, // Acoustic Snare
    40, // Electric Snare
    37, // Side Stick
    39, // Hand Clap
    42, // Closed Hi-Hat
    44, // Pedal Hi-Hat
    46, // Open Hi-Hat
    41, // Low Floor Tom
    43, // High Floor Tom
    45, // Low Tom
    47, // Low-Mid Tom
    48, // Hi-Mid Tom
    49, // Crash Cymbal 1
    51, // Ride Cymbal 1
  ];
  const map: number[] = [...bankA];
  // Banks B-H: continue from GM note 50 onward, skipping those already in Bank A
  const usedNotes = new Set(bankA);
  let nextNote = 50;
  for (let i = 16; i < 128; i++) {
    while (usedNotes.has(nextNote) && nextNote <= 127) nextNote++;
    map.push(Math.min(127, nextNote));
    usedNotes.add(nextNote);
    nextNote++;
  }
  return map;
}

/**
 * Chromatic — pad 0 = C-1 (MIDI 0), ascending chromatically.
 * Useful for melodic instruments and keygroup programs.
 */
function buildChromaticMap(): number[] {
  return Array.from({ length: 128 }, (_, i) => i);
}

export const PAD_MAP_TEMPLATES: PadMapTemplate[] = [
  {
    id: 'default',
    name: 'Default',
    description: 'Linear 1:1 mapping — pad index equals MIDI note',
    noteMap: buildChromaticMap(),
  },
  {
    id: 'mpc-classic',
    name: 'MPC Classic',
    description: 'Traditional MPC60/2000 layout — Bank A starts at C#2',
    noteMap: buildMpcClassicMap(),
  },
  {
    id: 'gm-standard',
    name: 'GM Standard',
    description: 'General MIDI drum map — core kit on Bank A',
    noteMap: buildGmStandardMap(),
  },
  {
    id: 'chromatic',
    name: 'Chromatic',
    description: 'C-1 to G9 ascending — ideal for keygroup programs',
    noteMap: buildChromaticMap(),
  },
];

export function getPadMapTemplate(id: string): PadMapTemplate | undefined {
  return PAD_MAP_TEMPLATES.find((t) => t.id === id);
}
