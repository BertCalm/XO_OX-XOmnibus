import type { XpmModulationConfig, XpmModulationRoute, XpmModSource, XpmModDest } from './xpmTypes';

// Runtime validation sets — must stay in sync with the union types in xpmTypes.ts
const VALID_MOD_SOURCES = new Set<string>(['Velocity', 'Aftertouch', 'ModWheel', 'LFO1', 'PitchEnv', 'FilterEnv', 'AmpEnv']);
const VALID_MOD_DESTS = new Set<string>(['FilterCutoff', 'FilterResonance', 'Volume', 'Pan', 'Pitch', 'LFO1Rate', 'LFO1Depth']);

// ---------------------------------------------------------------------------
// Types
// ---------------------------------------------------------------------------

export interface ParsedXpmKit {
  name: string;
  type: 'Keygroup' | 'Drum';
  instruments: ParsedInstrument[];
  padNoteMap: { padNumber: number; note: number }[];
  padGroupMap: { padNumber: number; group: number }[];
  /** Raw XML string for reference */
  rawXml: string;
}

export interface ParsedInstrument {
  number: number;
  lowNote: number;
  highNote: number;
  triggerMode: number;
  zonePlay: number;
  filterType: number;
  cutoff: number;
  resonance: number;
  filterEnvAmount: number;
  volumeEnvelope: { attack: number; hold: number; decay: number; sustain: number; release: number };
  filterEnvelope: { attack: number; hold: number; decay: number; sustain: number; release: number };
  pitchEnvelope: { attack: number; hold: number; decay: number; sustain: number; release: number; amount: number };
  velocitySensitivity: number;
  ignoreBaseNote: boolean;
  layers: ParsedLayer[];
  modulation?: XpmModulationConfig;
}

export interface ParsedLayer {
  number: number;
  active: boolean;
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
  sliceLoopCrossFadeLength: number;
  sliceIndex: number;
  direction: number;
  offset: number;
  probability: number;
}

// ---------------------------------------------------------------------------
// Safe XML extraction helpers
// ---------------------------------------------------------------------------

function getText(el: Element, tag: string, defaultVal = ''): string {
  const child = el.getElementsByTagName(tag)[0];
  return child?.textContent?.trim() ?? defaultVal;
}

function getFloat(el: Element, tag: string, defaultVal = 0): number {
  const raw = getText(el, tag);
  if (raw === '') return defaultVal;
  const n = parseFloat(raw);
  return Number.isFinite(n) ? n : defaultVal;
}

function getInt(el: Element, tag: string, defaultVal = 0): number {
  const raw = getText(el, tag);
  if (raw === '') return defaultVal;
  const n = parseInt(raw, 10);
  return Number.isFinite(n) ? n : defaultVal;
}

function getBool(el: Element, tag: string, defaultVal = false): boolean {
  const raw = getText(el, tag).toLowerCase();
  if (raw === 'true') return true;
  if (raw === 'false') return false;
  return defaultVal;
}

// ---------------------------------------------------------------------------
// Layer parsing
// ---------------------------------------------------------------------------

function parseLayer(layerEl: Element): ParsedLayer {
  const numberAttr = layerEl.getAttribute('number');
  const number = numberAttr ? parseInt(numberAttr, 10) : getInt(layerEl, 'Number', 1);

  return {
    number: Number.isFinite(number) ? number : 1,
    active: getBool(layerEl, 'Active', false),
    sampleName: getText(layerEl, 'SampleName'),
    sampleFile: getText(layerEl, 'SampleFile'),
    volume: getFloat(layerEl, 'Volume', 1.0),
    pan: getFloat(layerEl, 'Pan', 0.5),
    pitch: getFloat(layerEl, 'Pitch', 0.0),
    tuneCoarse: getInt(layerEl, 'TuneCoarse', 0),
    tuneFine: getInt(layerEl, 'TuneFine', 0),
    velStart: getInt(layerEl, 'VelStart', 0),
    velEnd: getInt(layerEl, 'VelEnd', 127),
    rootNote: getInt(layerEl, 'RootNote', 60),
    keyTrack: getBool(layerEl, 'KeyTrack', true),
    pitchRandom: getFloat(layerEl, 'PitchRandom', 0.0),
    volumeRandom: getFloat(layerEl, 'VolumeRandom', 0.0),
    panRandom: getFloat(layerEl, 'PanRandom', 0.0),
    sliceStart: getInt(layerEl, 'SliceStart', 0),
    sliceEnd: getInt(layerEl, 'SliceEnd', 0),
    sliceLoop: getInt(layerEl, 'SliceLoop', 0),
    sliceLoopStart: getInt(layerEl, 'SliceLoopStart', 0),
    sliceLoopCrossFadeLength: getInt(layerEl, 'SliceLoopCrossFadeLength', 0),
    sliceIndex: getInt(layerEl, 'SliceIndex', 128),
    direction: getInt(layerEl, 'Direction', 0),
    offset: getInt(layerEl, 'Offset', 0),
    probability: getInt(layerEl, 'Probability', 100),
  };
}

// ---------------------------------------------------------------------------
// Modulation parsing
// ---------------------------------------------------------------------------

function parseModulation(modEl: Element): XpmModulationConfig {
  const config: XpmModulationConfig = { routes: [] };

  // LFO1
  const lfo1Els = modEl.getElementsByTagName('LFO1');
  if (lfo1Els.length > 0) {
    const lfo1El = lfo1Els[0];
    config.lfo1 = {
      rate: getFloat(lfo1El, 'Rate', 0.3),
      shape: getInt(lfo1El, 'Shape', 0),
      sync: getBool(lfo1El, 'Sync', false),
    };
  }

  // Routes
  const routeEls = modEl.getElementsByTagName('Route');
  for (let i = 0; i < routeEls.length; i++) {
    const routeEl = routeEls[i];
    const sourceRaw = getText(routeEl, 'Source');
    const destRaw = getText(routeEl, 'Destination');
    const amount = getFloat(routeEl, 'Amount', 0);

    // Validate against known union types — skip unknown values from
    // newer MPC firmware or third-party editors rather than crashing
    if (sourceRaw && destRaw && VALID_MOD_SOURCES.has(sourceRaw) && VALID_MOD_DESTS.has(destRaw)) {
      config.routes.push({
        source: sourceRaw as XpmModSource,
        destination: destRaw as XpmModDest,
        amount,
      });
    }
  }

  return config;
}

// ---------------------------------------------------------------------------
// Instrument parsing
// ---------------------------------------------------------------------------

function parseInstrument(instEl: Element): ParsedInstrument {
  const numberAttr = instEl.getAttribute('number');
  const number = numberAttr ? parseInt(numberAttr, 10) : getInt(instEl, 'Number', 0);

  // Parse layers
  const layers: ParsedLayer[] = [];
  const layersContainer = instEl.getElementsByTagName('Layers')[0];
  if (layersContainer) {
    const layerEls = layersContainer.getElementsByTagName('Layer');
    for (let i = 0; i < layerEls.length; i++) {
      layers.push(parseLayer(layerEls[i]));
    }
  }
  // If no Layers container, try direct Layer children (some XPM variations)
  if (layers.length === 0) {
    const layerEls = instEl.getElementsByTagName('Layer');
    for (let i = 0; i < layerEls.length; i++) {
      layers.push(parseLayer(layerEls[i]));
    }
  }

  // Parse modulation
  let modulation: XpmModulationConfig | undefined;
  const modEls = instEl.getElementsByTagName('Modulation');
  if (modEls.length > 0) {
    modulation = parseModulation(modEls[0]);
  }

  return {
    number: Number.isFinite(number) ? number : 0,
    lowNote: getInt(instEl, 'LowNote', 0),
    highNote: getInt(instEl, 'HighNote', 127),
    triggerMode: getInt(instEl, 'TriggerMode', 0),
    zonePlay: getInt(instEl, 'ZonePlay', 1),
    filterType: getInt(instEl, 'FilterType', 0),
    cutoff: getFloat(instEl, 'Cutoff', 1.0),
    resonance: getFloat(instEl, 'Resonance', 0.0),
    filterEnvAmount: getFloat(instEl, 'FilterEnvAmt', 0.0),
    volumeEnvelope: {
      attack: getFloat(instEl, 'VolumeAttack', 0.0),
      hold: getFloat(instEl, 'VolumeHold', 0.0),
      decay: getFloat(instEl, 'VolumeDecay', 0.047244),
      sustain: getFloat(instEl, 'VolumeSustain', 1.0),
      release: getFloat(instEl, 'VolumeRelease', 0.0),
    },
    filterEnvelope: {
      attack: getFloat(instEl, 'FilterAttack', 0.0),
      hold: getFloat(instEl, 'FilterHold', 0.0),
      decay: getFloat(instEl, 'FilterDecay', 0.0),
      sustain: getFloat(instEl, 'FilterSustain', 1.0),
      release: getFloat(instEl, 'FilterRelease', 0.0),
    },
    pitchEnvelope: {
      attack: getFloat(instEl, 'PitchAttack', 0.0),
      hold: getFloat(instEl, 'PitchHold', 0.0),
      decay: getFloat(instEl, 'PitchDecay', 0.0),
      sustain: getFloat(instEl, 'PitchSustain', 1.0),
      release: getFloat(instEl, 'PitchRelease', 0.0),
      amount: getFloat(instEl, 'PitchEnvAmount', 0.5),
    },
    velocitySensitivity: getFloat(instEl, 'VelocitySensitivity', 1.0),
    ignoreBaseNote: getBool(instEl, 'IgnoreBaseNote', false),
    layers,
    modulation,
  };
}

// ---------------------------------------------------------------------------
// Main parser
// ---------------------------------------------------------------------------

/**
 * Parse an XPM XML string into a structured kit representation.
 * Uses DOMParser for XML parsing — works in browser and Node (with xmldom or similar).
 */
export function parseXpmXml(xmlString: string): ParsedXpmKit {
  const parser = new DOMParser();
  const doc = parser.parseFromString(xmlString, 'application/xml');

  // Check for parse errors
  const parseError = doc.querySelector('parsererror');
  if (parseError) {
    throw new Error(`XPM XML parse error: ${parseError.textContent}`);
  }

  // Find the <Program> element
  const programEl = doc.getElementsByTagName('Program')[0];
  if (!programEl) {
    throw new Error('No <Program> element found in XPM file');
  }

  // Program type
  const typeAttr = programEl.getAttribute('type');
  const type: 'Keygroup' | 'Drum' = typeAttr === 'Drum' ? 'Drum' : 'Keygroup';

  // Program name
  const name = getText(programEl, 'ProgramName', 'Untitled Kit');

  // Parse instruments
  const instruments: ParsedInstrument[] = [];
  const instrumentsContainer = programEl.getElementsByTagName('Instruments')[0];
  if (instrumentsContainer) {
    const instEls = instrumentsContainer.getElementsByTagName('Instrument');
    for (let i = 0; i < instEls.length; i++) {
      instruments.push(parseInstrument(instEls[i]));
    }
  }

  // Parse PadNoteMap
  const padNoteMap: { padNumber: number; note: number }[] = [];
  const padNoteMapEl = programEl.getElementsByTagName('PadNoteMap')[0];
  if (padNoteMapEl) {
    const padNoteEls = padNoteMapEl.getElementsByTagName('PadNote');
    for (let i = 0; i < padNoteEls.length; i++) {
      const el = padNoteEls[i];
      const rawPadNumber = parseInt(el.getAttribute('number') ?? '0', 10);
      const padNumber = Number.isFinite(rawPadNumber) ? rawPadNumber : 0;
      const note = getInt(el, 'Note', padNumber);
      padNoteMap.push({ padNumber, note });
    }
  }

  // Parse PadGroupMap
  const padGroupMap: { padNumber: number; group: number }[] = [];
  const padGroupMapEl = programEl.getElementsByTagName('PadGroupMap')[0];
  if (padGroupMapEl) {
    const padGroupEls = padGroupMapEl.getElementsByTagName('PadGroup');
    for (let i = 0; i < padGroupEls.length; i++) {
      const el = padGroupEls[i];
      const rawPadNumber = parseInt(el.getAttribute('number') ?? '0', 10);
      const padNumber = Number.isFinite(rawPadNumber) ? rawPadNumber : 0;
      const group = getInt(el, 'Group', 0);
      padGroupMap.push({ padNumber, group });
    }
  }

  return {
    name,
    type,
    instruments,
    padNoteMap,
    padGroupMap,
    rawXml: xmlString,
  };
}

// ---------------------------------------------------------------------------
// Kit analysis
// ---------------------------------------------------------------------------

export interface KitAnalysis {
  totalPads: number;
  padsWithSamples: number;
  maxLayersUsed: number;
  hasModulation: boolean;
  hasVelocitySplits: boolean;
  hasEnvelopeCustomization: boolean;
  hasFilterSettings: boolean;
  hasPitchRandomization: boolean;
  hasVolumeRandomization: boolean;
  /** 0-100 "health" score representing how expressive the kit is */
  healthScore: number;
  suggestions: string[];
}

// Default envelope values from MPC spec
const DEFAULT_VOL_ENV = { attack: 0, hold: 0, decay: 0.047244, sustain: 1, release: 0 };
const DEFAULT_FILTER_ENV = { attack: 0, hold: 0, decay: 0.0, sustain: 1, release: 0 };

function envelopeDiffersFromDefault(
  env: { attack: number; hold: number; decay: number; sustain: number; release: number },
  defaults: typeof DEFAULT_VOL_ENV,
): boolean {
  const tolerance = 0.001;
  return (
    Math.abs(env.attack - defaults.attack) > tolerance ||
    Math.abs(env.hold - defaults.hold) > tolerance ||
    Math.abs(env.decay - defaults.decay) > tolerance ||
    Math.abs(env.sustain - defaults.sustain) > tolerance ||
    Math.abs(env.release - defaults.release) > tolerance
  );
}

/**
 * Analyze a parsed kit and generate a summary of what could be improved.
 */
export function analyzeKit(kit: ParsedXpmKit): KitAnalysis {
  let padsWithSamples = 0;
  let maxLayersUsed = 0;
  let hasModulation = false;
  let hasVelocitySplits = false;
  let hasEnvelopeCustomization = false;
  let hasFilterSettings = false;
  let hasPitchRandomization = false;
  let hasVolumeRandomization = false;

  for (const inst of kit.instruments) {
    // Count active layers for this instrument
    const activeLayers = inst.layers.filter(
      (l) => l.active && (l.sampleName !== '' || l.sampleFile !== ''),
    );
    if (activeLayers.length > 0) {
      padsWithSamples++;
    }
    maxLayersUsed = Math.max(maxLayersUsed, activeLayers.length);

    // Check modulation
    if (inst.modulation && inst.modulation.routes.length > 0) {
      hasModulation = true;
    }

    // Check velocity splits
    for (const layer of activeLayers) {
      if (layer.velStart !== 0 || layer.velEnd !== 127) {
        hasVelocitySplits = true;
      }
      if (layer.pitchRandom > 0) {
        hasPitchRandomization = true;
      }
      if (layer.volumeRandom > 0) {
        hasVolumeRandomization = true;
      }
    }

    // Check envelope customization
    if (envelopeDiffersFromDefault(inst.volumeEnvelope, DEFAULT_VOL_ENV)) {
      hasEnvelopeCustomization = true;
    }
    if (envelopeDiffersFromDefault(inst.filterEnvelope, DEFAULT_FILTER_ENV)) {
      hasEnvelopeCustomization = true;
    }

    // Check filter settings
    if (inst.filterType !== 0 || inst.cutoff < 0.999 || inst.resonance > 0.001) {
      hasFilterSettings = true;
    }
  }

  // Build suggestions
  const suggestions: string[] = [];

  if (!hasVelocitySplits && maxLayersUsed <= 1) {
    suggestions.push('No velocity layers \u2014 add ghost notes for dynamics');
  }

  if (!hasModulation) {
    suggestions.push('No modulation \u2014 inject Velocity Dynamics preset');
  }

  if (maxLayersUsed <= 1) {
    suggestions.push('Single layers only \u2014 use Expression Engine for character');
  }

  if (!hasFilterSettings) {
    suggestions.push('No filter settings \u2014 add Velocity\u2192Filter for expressiveness');
  }

  if (!hasEnvelopeCustomization) {
    suggestions.push('Default envelopes \u2014 customize for punch and presence');
  }

  if (!hasPitchRandomization && !hasVolumeRandomization) {
    suggestions.push('No randomization \u2014 humanize for organic feel');
  }

  // Compute a health score (0-100)
  // Each feature contributes a portion
  let healthScore = 0;
  const featureWeight = 100 / 7;
  if (padsWithSamples > 0) healthScore += featureWeight;
  if (maxLayersUsed > 1) healthScore += featureWeight;
  if (hasModulation) healthScore += featureWeight;
  if (hasVelocitySplits) healthScore += featureWeight;
  if (hasEnvelopeCustomization) healthScore += featureWeight;
  if (hasFilterSettings) healthScore += featureWeight;
  if (hasPitchRandomization || hasVolumeRandomization) healthScore += featureWeight;
  healthScore = Math.round(Math.min(100, healthScore));

  return {
    totalPads: kit.instruments.length,
    padsWithSamples,
    maxLayersUsed,
    hasModulation,
    hasVelocitySplits,
    hasEnvelopeCustomization,
    hasFilterSettings,
    hasPitchRandomization,
    hasVolumeRandomization,
    healthScore,
    suggestions,
  };
}
