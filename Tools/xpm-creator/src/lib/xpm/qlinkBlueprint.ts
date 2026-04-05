// ---------------------------------------------------------------------------
// Types
// ---------------------------------------------------------------------------

export interface QLinkMapping {
  /** Knob index (0-15, Q-Link knobs on MPC) */
  knobIndex: number;
  /** Parameter being controlled */
  parameter: string;
  /** Human-readable label for the knob */
  label: string;
  /** Minimum value */
  min: number;
  /** Maximum value */
  max: number;
  /** Default/center value */
  defaultValue: number;
}

export interface QLinkPreset {
  id: string;
  name: string;
  icon: string;
  description: string;
  mappings: QLinkMapping[];
}

// ---------------------------------------------------------------------------
// Presets
// ---------------------------------------------------------------------------

export const QLINK_PRESETS: QLinkPreset[] = [
  {
    id: 'performance',
    name: 'Performance',
    icon: '🎹',
    description: 'Live performance controls — filter sweeps, volume, and FX sends',
    mappings: [
      { knobIndex: 0, parameter: 'FILTER_CUTOFF', label: 'Filter', min: 0, max: 127, defaultValue: 127 },
      { knobIndex: 1, parameter: 'FILTER_RESONANCE', label: 'Resonance', min: 0, max: 127, defaultValue: 0 },
      { knobIndex: 2, parameter: 'VOLUME', label: 'Volume', min: 0, max: 127, defaultValue: 100 },
      { knobIndex: 3, parameter: 'PAN', label: 'Pan', min: 0, max: 127, defaultValue: 64 },
      { knobIndex: 4, parameter: 'SEND_1', label: 'Reverb', min: 0, max: 127, defaultValue: 0 },
      { knobIndex: 5, parameter: 'SEND_2', label: 'Delay', min: 0, max: 127, defaultValue: 0 },
      { knobIndex: 6, parameter: 'ATTACK', label: 'Attack', min: 0, max: 127, defaultValue: 0 },
      { knobIndex: 7, parameter: 'DECAY', label: 'Decay', min: 0, max: 127, defaultValue: 64 },
    ],
  },
  {
    id: 'sound-design',
    name: 'Sound Design',
    icon: '🔬',
    description: 'Deep sound sculpting — envelopes, tuning, and filter modulation',
    mappings: [
      { knobIndex: 0, parameter: 'FILTER_CUTOFF', label: 'Cutoff', min: 0, max: 127, defaultValue: 127 },
      { knobIndex: 1, parameter: 'FILTER_RESONANCE', label: 'Reso', min: 0, max: 127, defaultValue: 0 },
      { knobIndex: 2, parameter: 'FILTER_ENV_AMOUNT', label: 'Flt Env', min: 0, max: 127, defaultValue: 64 },
      { knobIndex: 3, parameter: 'TUNE_COARSE', label: 'Tune', min: 0, max: 127, defaultValue: 64 },
      { knobIndex: 4, parameter: 'ATTACK', label: 'Attack', min: 0, max: 127, defaultValue: 0 },
      { knobIndex: 5, parameter: 'HOLD', label: 'Hold', min: 0, max: 127, defaultValue: 0 },
      { knobIndex: 6, parameter: 'DECAY', label: 'Decay', min: 0, max: 127, defaultValue: 64 },
      { knobIndex: 7, parameter: 'RELEASE', label: 'Release', min: 0, max: 127, defaultValue: 0 },
    ],
  },
  {
    id: 'live',
    name: 'Live',
    icon: '🎤',
    description: 'Minimal live setup — volume, filter, and two FX sends',
    mappings: [
      { knobIndex: 0, parameter: 'VOLUME', label: 'Vol', min: 0, max: 127, defaultValue: 100 },
      { knobIndex: 1, parameter: 'FILTER_CUTOFF', label: 'Filter', min: 0, max: 127, defaultValue: 127 },
      { knobIndex: 2, parameter: 'SEND_1', label: 'Send A', min: 0, max: 127, defaultValue: 0 },
      { knobIndex: 3, parameter: 'SEND_2', label: 'Send B', min: 0, max: 127, defaultValue: 0 },
    ],
  },
];

// ---------------------------------------------------------------------------
// XML Generation
// ---------------------------------------------------------------------------

/**
 * Generate the Q-Link XML block for XPM output.
 * This is injected into the XPM drum program to set up Q-Link knob assignments.
 */
export function generateQLinkXml(mappings: QLinkMapping[]): string {
  if (mappings.length === 0) return '';

  const lines: string[] = ['  <QLink>'];

  for (let i = 0; i < mappings.length; i++) {
    const m = mappings[i];
    lines.push(`    <Knob index="${m.knobIndex}">`);
    lines.push(`      <Parameter>${escapeXml(m.parameter)}</Parameter>`);
    lines.push(`      <Label>${escapeXml(m.label)}</Label>`);
    lines.push(`      <Min>${m.min}</Min>`);
    lines.push(`      <Max>${m.max}</Max>`);
    lines.push(`      <Default>${m.defaultValue}</Default>`);
    lines.push(`    </Knob>`);
  }

  lines.push('  </QLink>');
  return lines.join('\n');
}

/**
 * Escape special characters for XML output.
 */
function escapeXml(str: string): string {
  return str
    .replace(/&/g, '&amp;')
    .replace(/</g, '&lt;')
    .replace(/>/g, '&gt;')
    .replace(/"/g, '&quot;')
    .replace(/'/g, '&apos;');
}
