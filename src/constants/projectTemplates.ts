/**
 * Project Templates / Starter Kits
 *
 * Pre-configured starting points for new projects.
 * Each template defines a program type, pad map, and
 * suggested sample category layout for quick setup.
 */

import type { ProgramType } from '@/types';
import type { SampleCategory } from '@/lib/audio/sampleCategorizer';

export interface ProjectTemplate {
  id: string;
  name: string;
  description: string;
  emoji: string;
  programType: ProgramType;
  /** ID from padMapTemplates — applied on creation */
  padMapTemplateId?: string;
  /** Suggested sample categories for auto-assignment hints */
  suggestedCategories?: SampleCategory[];
}

export const PROJECT_TEMPLATES: ProjectTemplate[] = [
  {
    id: 'blank',
    name: 'Blank',
    description: 'Start from scratch with an empty project',
    emoji: '\u2728',
    programType: 'Drum',
  },
  {
    id: 'boom-bap',
    name: 'Boom Bap Kit',
    description: 'Classic hip-hop drum layout — kick, snare, hats, percussion',
    emoji: '\uD83E\uDD4A',
    programType: 'Drum',
    padMapTemplateId: 'mpc-classic',
    suggestedCategories: ['kick', 'snare', 'hat-closed', 'hat-open', 'clap', 'perc', 'cymbal', 'tom'],
  },
  {
    id: 'trap',
    name: 'Trap Kit',
    description: '808-heavy layout — sub bass, hats, snares, FX',
    emoji: '\uD83D\uDD25',
    programType: 'Drum',
    padMapTemplateId: 'mpc-classic',
    suggestedCategories: ['kick', 'snare', 'hat-closed', 'hat-open', 'clap', 'perc', 'fx', 'vocal'],
  },
  {
    id: 'lofi',
    name: 'Lo-Fi Kit',
    description: 'Dusty, warm sounds — perfect for chill beats',
    emoji: '\uD83C\uDFB5',
    programType: 'Drum',
    padMapTemplateId: 'mpc-classic',
    suggestedCategories: ['kick', 'snare', 'hat-closed', 'perc', 'keys', 'pad', 'fx', 'vocal'],
  },
  {
    id: 'chromatic',
    name: 'Chromatic Keys',
    description: 'Melodic keygroup layout — chromatic MIDI mapping',
    emoji: '\uD83C\uDFB9',
    programType: 'Keygroup',
    padMapTemplateId: 'chromatic',
    suggestedCategories: ['keys', 'pad', 'lead', 'bass'],
  },
];

export function getProjectTemplate(id: string): ProjectTemplate | undefined {
  return PROJECT_TEMPLATES.find((t) => t.id === id);
}
