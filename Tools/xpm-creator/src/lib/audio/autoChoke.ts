/**
 * Auto-Choke Intelligence
 *
 * Analyzes sample categories to suggest and auto-assign Mute Groups,
 * preventing overlapping sounds (like open/closed hi-hats, or multiple 808s).
 *
 * Mute Group Rules:
 * 1. Hi-Hat Group (1): hat-closed + hat-open choke each other
 * 2. Cymbal Group (2): All cymbals choke each other (crash, ride, splash)
 * 3. 808/Bass Group (3): All bass/808 sounds choke each other (prevent mud)
 * 4. Clap/Snap Group (4): Claps and snaps choke (prevent layering issues)
 * 5. No group (0): kicks, snares, toms, percs, fx, vocals — play independently
 */

import { categorizeSample } from './sampleCategorizer';
import type { SampleCategory } from './sampleCategorizer';
import type { PadAssignment } from '@/types';

export interface MuteGroupSuggestion {
  padIndex: number;
  padNumber: number;
  sampleName: string;
  category: SampleCategory;
  suggestedGroup: number;
  groupName: string;
}

export const MUTE_GROUP_NAMES: Record<number, string> = {
  0: 'None',
  1: 'Hi-Hats',
  2: 'Cymbals',
  3: 'Bass/808',
  4: 'Clap/Snap',
};

export const MUTE_GROUP_COLORS: Record<number, string> = {
  0: 'text-text-muted',
  1: 'text-amber-400',
  2: 'text-sky-400',
  3: 'text-red-400',
  4: 'text-green-400',
};

/** Map a sample category to its suggested mute group number. */
function categoryToMuteGroup(category: SampleCategory): number {
  switch (category) {
    case 'hat-closed':
    case 'hat-open':
      return 1;
    case 'cymbal':
      return 2;
    case 'bass':
      return 3;
    case 'clap':
      return 4;
    default:
      return 0;
  }
}

/**
 * Analyze pads and suggest mute groups based on sample categories.
 *
 * Iterates all pads, categorizes each pad's first active sample by filename,
 * and returns an array of suggestions mapping each pad to a mute group.
 */
export function suggestMuteGroups(pads: PadAssignment[]): MuteGroupSuggestion[] {
  const suggestions: MuteGroupSuggestion[] = [];

  for (let i = 0; i < pads.length; i++) {
    const pad = pads[i];
    // Find the first active layer with a sample loaded
    const activeLayer = pad.layers.find((l) => l.active && l.sampleId);
    if (!activeLayer) continue;

    // Use the sample filename for categorization (fall back to sample name)
    const fileName = activeLayer.sampleFile || activeLayer.sampleName;
    if (!fileName) continue;

    const { category } = categorizeSample(fileName);
    const suggestedGroup = categoryToMuteGroup(category);

    suggestions.push({
      padIndex: i,
      padNumber: pad.padNumber,
      sampleName: activeLayer.sampleName,
      category,
      suggestedGroup,
      groupName: MUTE_GROUP_NAMES[suggestedGroup] || 'None',
    });
  }

  return suggestions;
}
