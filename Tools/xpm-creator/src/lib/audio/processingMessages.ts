/**
 * Processing Messages — personality-driven status text.
 *
 * Instead of generic "Processing..." messages, XO_OX speaks
 * through contextual messages keyed by operation type.
 * Messages rotate on each call for variety.
 */

const PROCESSING_MESSAGES: Record<string, string[]> = {
  export: [
    'Forging your program\u2026',
    'XO_OX is packaging\u2026',
    'Sealing the crate\u2026',
    'Wrapping it up tight\u2026',
    'Almost ready to ship\u2026',
  ],
  import: [
    'Sourcing material\u2026',
    'XO_OX is listening\u2026',
    'Analyzing the goods\u2026',
    'Digging through the crate\u2026',
    'Fresh samples incoming\u2026',
  ],
  chop: [
    'Slicing the frequencies\u2026',
    'Finding the sweet spots\u2026',
    'XO_OX is chopping\u2026',
    'Breaking it down\u2026',
    'Precision cuts in progress\u2026',
  ],
  build: [
    'Constructing the kit\u2026',
    'XO_OX is building\u2026',
    'Assembling the pieces\u2026',
    'Laying the foundation\u2026',
    'Wiring up the pads\u2026',
  ],
  process: [
    'XO_OX is thinking\u2026',
    'Tuning the frequencies\u2026',
    'Almost there, stay with me\u2026',
    'Working the magic\u2026',
    'Processing the vibes\u2026',
  ],
  general: [
    'XO_OX is on it\u2026',
    'One moment\u2026',
    'Hang tight\u2026',
    'Working\u2026',
    'In progress\u2026',
  ],
};

/** Counter per category to cycle through messages sequentially */
const counters: Record<string, number> = {};

/**
 * Get the next processing message for a given category.
 * Cycles through messages in order for consistent variety.
 */
export function getProcessingMessage(category: string = 'general'): string {
  const messages = PROCESSING_MESSAGES[category] ?? PROCESSING_MESSAGES.general;
  const idx = (counters[category] ?? 0) % messages.length;
  counters[category] = idx + 1;
  return messages[idx];
}

/**
 * Get a random processing message for a given category.
 */
export function getRandomProcessingMessage(category: string = 'general'): string {
  const messages = PROCESSING_MESSAGES[category] ?? PROCESSING_MESSAGES.general;
  return messages[Math.floor(Math.random() * messages.length)];
}
