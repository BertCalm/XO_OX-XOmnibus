/**
 * Smart Sample Categorizer
 *
 * Auto-detects sample types from filename patterns and audio characteristics.
 * Supports hybrid mode: filename regex + spectral FFT fallback.
 * Used for one-touch kit building and intelligent pad assignment.
 */

import {
  analyzeSpectralFingerprint,
  classifyBySpectrum,
} from './spectralFingerprinter';

export type SampleCategory =
  | 'kick'
  | 'snare'
  | 'clap'
  | 'hat-closed'
  | 'hat-open'
  | 'cymbal'
  | 'tom'
  | 'perc'
  | 'bass'
  | 'keys'
  | 'pad'
  | 'lead'
  | 'fx'
  | 'vocal'
  | 'unknown';

export interface CategorizedSample {
  sampleId: string;
  category: SampleCategory;
  confidence: number; // 0-1
  subcategory?: string;
}

// Filename pattern matching rules (ordered by specificity)
const CATEGORY_PATTERNS: { category: SampleCategory; patterns: RegExp[]; subcategory?: string }[] = [
  // Kicks
  { category: 'kick', patterns: [
    /\bkick\b/i, /\bkik\b/i, /\bkck\b/i, /\bbd\b/i, /\bbass[\s_-]?drum\b/i,
    /\b808\b/i, /\bkk\b/i, /\bboom\b/i, /\bthump\b/i,
  ]},
  // Snares
  { category: 'snare', patterns: [
    /\bsnare\b/i, /\bsnr\b/i, /\bsd\b/i, /\brimshot\b/i, /\brim\b/i,
  ]},
  // Claps
  { category: 'clap', patterns: [
    /\bclap\b/i, /\bclp\b/i, /\bhandclap\b/i, /\bsnap\b/i,
  ]},
  // Closed hats
  { category: 'hat-closed', patterns: [
    /\bclosed[\s_-]?hat\b/i, /\bch[\s_-]?hat\b/i, /\bhh[\s_-]?c\b/i,
    /\bclosed[\s_-]?hh\b/i, /\bhihat[\s_-]?c/i, /\bhat[\s_-]?cl/i,
  ]},
  // Open hats
  { category: 'hat-open', patterns: [
    /\bopen[\s_-]?hat\b/i, /\boh[\s_-]?hat\b/i, /\bhh[\s_-]?o\b/i,
    /\bopen[\s_-]?hh\b/i, /\bhihat[\s_-]?o/i, /\bhat[\s_-]?op/i,
  ]},
  // Generic hats (closed by default)
  { category: 'hat-closed', patterns: [
    /\bhi[\s_-]?hat\b/i, /\bhihat\b/i, /\bhat\b/i, /\bhh\b/i,
  ]},
  // Cymbals
  { category: 'cymbal', patterns: [
    /\bcymbal\b/i, /\bcym\b/i, /\bcrash\b/i, /\bride\b/i, /\bsplash\b/i,
  ]},
  // Toms
  { category: 'tom', patterns: [
    /\btom\b/i, /\btm\b/i, /\bconga\b/i, /\bbongo\b/i,
  ]},
  // Percussion
  { category: 'perc', patterns: [
    /\bperc\b/i, /\bshaker\b/i, /\btambourine\b/i, /\btamb\b/i,
    /\bcowbell\b/i, /\bbell\b/i, /\bwood[\s_-]?block\b/i, /\btriangle\b/i,
    /\bguiro\b/i, /\bclave\b/i, /\bmaracas\b/i, /\bcastanet\b/i,
    /\bvibraslap\b/i, /\bagogo\b/i, /\btimbale\b/i, /\bcabasa\b/i,
  ]},
  // Bass
  { category: 'bass', patterns: [
    /\bbass\b/i, /\bsub\b/i, /\b808[\s_-]?bass\b/i, /\breese\b/i,
  ]},
  // Keys
  { category: 'keys', patterns: [
    /\bkeys?\b/i, /\bpiano\b/i, /\borgan\b/i, /\brhodes\b/i,
    /\bwurlitzer\b/i, /\bclav\b/i, /\belectric[\s_-]?piano\b/i, /\bep\b/i,
  ]},
  // Pads
  { category: 'pad', patterns: [
    /\bpad\b/i, /\batmosphere\b/i, /\bambient\b/i, /\btexture\b/i,
    /\bdrone\b/i, /\bstring\b/i, /\bchoir\b/i,
  ]},
  // Leads
  { category: 'lead', patterns: [
    /\blead\b/i, /\bsynth\b/i, /\bpluck\b/i, /\barp\b/i, /\bstab\b/i,
  ]},
  // FX
  { category: 'fx', patterns: [
    /\bfx\b/i, /\beffect\b/i, /\briser\b/i, /\bsweep\b/i, /\bimpact\b/i,
    /\bboom\b/i, /\bnoise\b/i, /\bscratch\b/i, /\bvinyl\b/i, /\btransition\b/i,
    /\bdownlifter\b/i, /\buplifter\b/i,
  ]},
  // Vocals
  { category: 'vocal', patterns: [
    /\bvocal\b/i, /\bvox\b/i, /\bvoice\b/i, /\bchant\b/i, /\bspoken\b/i,
    /\bad[\s_-]?lib\b/i, /\bsing\b/i,
  ]},
];

/**
 * Categorize a sample based on its filename.
 */
export function categorizeSample(fileName: string): { category: SampleCategory; confidence: number } {
  const cleanName = fileName
    .replace(/\.(wav|mp3|flac|m4a|ogg|aif|aiff)$/i, '')
    .replace(/[_\-]+/g, ' ');

  for (const rule of CATEGORY_PATTERNS) {
    for (const pattern of rule.patterns) {
      if (pattern.test(cleanName)) {
        return { category: rule.category, confidence: 0.85 };
      }
    }
  }

  return { category: 'unknown', confidence: 0 };
}

/**
 * Categorize multiple samples and return sorted by category.
 */
export function categorizeMultiple(
  samples: { id: string; fileName: string }[]
): CategorizedSample[] {
  return samples.map((s) => {
    const result = categorizeSample(s.fileName);
    return {
      sampleId: s.id,
      category: result.category,
      confidence: result.confidence,
    };
  });
}

// MPC-standard drum pad layout (Bank A)
// Based on General MIDI drum mapping
export const MPC_DRUM_PAD_MAP: { padIndex: number; categories: SampleCategory[]; label: string }[] = [
  // Bottom row (Pads 1-4)
  { padIndex: 0,  categories: ['kick'],       label: 'Kick' },
  { padIndex: 1,  categories: ['kick'],       label: 'Kick 2' },
  { padIndex: 2,  categories: ['snare'],      label: 'Snare' },
  { padIndex: 3,  categories: ['snare', 'clap'], label: 'Snare 2' },
  // Row 2 (Pads 5-8)
  { padIndex: 4,  categories: ['clap'],       label: 'Clap' },
  { padIndex: 5,  categories: ['hat-closed'], label: 'Closed Hat' },
  { padIndex: 6,  categories: ['hat-open'],   label: 'Open Hat' },
  { padIndex: 7,  categories: ['perc'],       label: 'Perc 1' },
  // Row 3 (Pads 9-12)
  { padIndex: 8,  categories: ['tom'],        label: 'Tom Low' },
  { padIndex: 9,  categories: ['tom'],        label: 'Tom Mid' },
  { padIndex: 10, categories: ['tom', 'perc'], label: 'Tom Hi/Perc' },
  { padIndex: 11, categories: ['cymbal'],     label: 'Crash/Ride' },
  // Top row (Pads 13-16)
  { padIndex: 12, categories: ['perc', 'fx'], label: 'Perc 2/FX' },
  { padIndex: 13, categories: ['fx'],         label: 'FX' },
  { padIndex: 14, categories: ['vocal'],      label: 'Vocal/Vox' },
  { padIndex: 15, categories: ['perc', 'fx'], label: 'Perc 3/FX' },
];

/**
 * Auto-assign categorized samples to MPC drum pads.
 * Returns a mapping of padIndex → sampleId.
 */
export function autoAssignToPads(
  categorized: CategorizedSample[]
): Map<number, string> {
  const result = new Map<number, string>();
  const usedSamples = new Set<string>();

  // First pass: assign by exact category match
  for (const padSlot of MPC_DRUM_PAD_MAP) {
    for (const sample of categorized) {
      if (usedSamples.has(sample.sampleId)) continue;
      if (padSlot.categories.includes(sample.category)) {
        result.set(padSlot.padIndex, sample.sampleId);
        usedSamples.add(sample.sampleId);
        break;
      }
    }
  }

  // Second pass: assign remaining samples to empty pads
  const remainingSamples = categorized.filter((s) => !usedSamples.has(s.sampleId));
  const emptyPads = MPC_DRUM_PAD_MAP
    .map((p) => p.padIndex)
    .filter((idx) => !result.has(idx));

  for (let i = 0; i < Math.min(remainingSamples.length, emptyPads.length); i++) {
    result.set(emptyPads[i], remainingSamples[i].sampleId);
  }

  return result;
}

// ---------------------------------------------------------------------------
// Hybrid categorization (filename + spectral fallback)
// ---------------------------------------------------------------------------

/** Confidence threshold — if filename regex scores at or above this, skip spectral. */
const FILENAME_CONFIDENCE_THRESHOLD = 0.85;

/**
 * Hybrid categorization: try filename patterns first. If the filename
 * match is ambiguous or unknown, fall back to spectral FFT analysis.
 *
 * This avoids the cost of FFT for samples with clear naming conventions
 * while still catching samples with non-standard or generic names.
 */
export async function categorizeWithSpectral(
  fileName: string,
  audioBuffer: AudioBuffer,
): Promise<{ category: SampleCategory; confidence: number; source: 'filename' | 'spectral' }> {
  // Stage 1: filename regex
  const fileResult = categorizeSample(fileName);

  if (
    fileResult.category !== 'unknown' &&
    fileResult.confidence >= FILENAME_CONFIDENCE_THRESHOLD
  ) {
    return { ...fileResult, source: 'filename' };
  }

  // Stage 2: spectral analysis
  const fingerprint = await analyzeSpectralFingerprint(audioBuffer);
  const spectralResult = classifyBySpectrum(fingerprint);

  // If spectral result is better confidence, use it; otherwise prefer filename
  if (
    spectralResult.category !== 'unknown' &&
    spectralResult.confidence > fileResult.confidence
  ) {
    return { ...spectralResult, source: 'spectral' };
  }

  // If filename had a match (even low confidence), keep it over spectral unknown
  if (fileResult.category !== 'unknown') {
    return { ...fileResult, source: 'filename' };
  }

  // Both failed — return spectral result (may still be 'unknown')
  return { ...spectralResult, source: 'spectral' };
}

/**
 * Category display metadata
 */
export const CATEGORY_INFO: Record<SampleCategory, { label: string; icon: string; color: string }> = {
  'kick':       { label: 'Kick',       icon: '🥁', color: 'text-red-400' },
  'snare':      { label: 'Snare',      icon: '🪘', color: 'text-orange-400' },
  'clap':       { label: 'Clap',       icon: '👏', color: 'text-yellow-400' },
  'hat-closed': { label: 'Closed Hat', icon: '🔔', color: 'text-cyan-400' },
  'hat-open':   { label: 'Open Hat',   icon: '🔔', color: 'text-teal-400' },
  'cymbal':     { label: 'Cymbal',     icon: '💿', color: 'text-blue-400' },
  'tom':        { label: 'Tom',        icon: '🥁', color: 'text-purple-400' },
  'perc':       { label: 'Perc',       icon: '🎵', color: 'text-pink-400' },
  'bass':       { label: 'Bass',       icon: '🎸', color: 'text-indigo-400' },
  'keys':       { label: 'Keys',       icon: '🎹', color: 'text-green-400' },
  'pad':        { label: 'Pad',        icon: '🌊', color: 'text-sky-400' },
  'lead':       { label: 'Lead',       icon: '✨', color: 'text-violet-400' },
  'fx':         { label: 'FX',         icon: '💥', color: 'text-amber-400' },
  'vocal':      { label: 'Vocal',      icon: '🎤', color: 'text-rose-400' },
  'unknown':    { label: 'Unknown',    icon: '❓', color: 'text-gray-400' },
};
