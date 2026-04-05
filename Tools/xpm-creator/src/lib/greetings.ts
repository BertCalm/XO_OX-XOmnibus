/**
 * XO_OX Greeting System
 *
 * Generates contextual greetings based on time of day.
 * Used by the Header to give the app personality.
 */

interface Greeting {
  text: string;
  emoji: string;
}

const greetings: Record<string, Greeting[]> = {
  // 5-11am: morning vibes
  morning: [
    { text: 'Rise and grind', emoji: '\u2600\uFE0F' },
    { text: 'Early bird gets the beat', emoji: '\uD83D\uDC26' },
    { text: 'Morning session loading', emoji: '\uD83C\uDF05' },
    { text: 'Fresh ears, fresh beats', emoji: '\u2615' },
    { text: 'Dawn patrol', emoji: '\uD83C\uDF04' },
  ],
  // 12-5pm: afternoon flow
  afternoon: [
    { text: 'In the zone', emoji: '\uD83C\uDFAF' },
    { text: 'Crafting heat', emoji: '\uD83D\uDD25' },
    { text: 'The Forge is hot', emoji: '\u2692\uFE0F' },
    { text: 'Peak hours', emoji: '\u26A1' },
    { text: 'Locked in', emoji: '\uD83D\uDD12' },
  ],
  // 6-11pm: evening/night session
  evening: [
    { text: 'Late night vibes', emoji: '\uD83C\uDF19' },
    { text: 'After hours', emoji: '\uD83C\uDFB9' },
    { text: 'The lab is open', emoji: '\uD83E\uDDEA' },
    { text: 'Night shift', emoji: '\uD83C\uDF03' },
    { text: 'Golden hour session', emoji: '\uD83C\uDF07' },
  ],
  // 12-4am: deep night
  lateNight: [
    { text: 'Burning the midnight oil', emoji: '\uD83D\uDD6F\uFE0F' },
    { text: 'Ghost hours', emoji: '\uD83D\uDC7B' },
    { text: 'The witching hour', emoji: '\uD83C\uDF11' },
    { text: 'Sleep is overrated', emoji: '\uD83E\uDD71' },
    { text: 'Nocturnal mode', emoji: '\uD83E\uDD87' },
  ],
};

function getTimeBucket(hour: number): string {
  if (hour >= 5 && hour < 12) return 'morning';
  if (hour >= 12 && hour < 18) return 'afternoon';
  if (hour >= 18 && hour < 24) return 'evening';
  return 'lateNight'; // 0-4am
}

/**
 * Returns a random contextual greeting based on the current time of day.
 */
export function getGreeting(): Greeting {
  const hour = new Date().getHours();
  const bucket = getTimeBucket(hour);
  const options = greetings[bucket];
  const index = Math.floor(Math.random() * options.length);
  return options[index];
}
