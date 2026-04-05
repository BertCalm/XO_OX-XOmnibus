import { getAudioContext } from './audioContext';
import { useThemeStore } from '@/stores/themeStore';

/**
 * Theme-aware sound character.
 * Dark themes → deeper tones, vintage → warmer harmonics, neon → brighter.
 */
type SoundCharacter = { freqMultiplier: number; waveform: OscillatorType; detune: number };

function getThemeSoundCharacter(): SoundCharacter {
  try {
    const { activeThemeId: currentTheme } = useThemeStore.getState();
    // Categorize theme by ID patterns
    if (/neon|trap|tokyo|chrome/.test(currentTheme)) {
      return { freqMultiplier: 1.15, waveform: 'sawtooth', detune: 0 }; // Bright, edgy
    }
    if (/lofi|tape|crate|vinyl|golden|blax|swamp/.test(currentTheme)) {
      return { freqMultiplier: 0.85, waveform: 'triangle', detune: 5 }; // Warm, vintage
    }
    if (/midnight|boom|808|studio-midnight|south/.test(currentTheme)) {
      return { freqMultiplier: 0.8, waveform: 'sine', detune: 0 }; // Deep, dark
    }
    // Light/neutral themes
    return { freqMultiplier: 1.0, waveform: 'sine', detune: 0 };
  } catch {
    return { freqMultiplier: 1.0, waveform: 'sine', detune: 0 };
  }
}

/**
 * Play a subtle UI confirmation sound (tiny sine burst).
 * Used when a sample is assigned to a pad.
 * Theme-aware: adjusts frequency and waveform based on current theme.
 */
export function playDropSound(): void {
  try {
    const ctx = getAudioContext();
    const char = getThemeSoundCharacter();
    const osc = ctx.createOscillator();
    const gain = ctx.createGain();
    osc.frequency.value = 880 * char.freqMultiplier;
    osc.type = char.waveform;
    osc.detune.value = char.detune;
    gain.gain.setValueAtTime(0.05, ctx.currentTime);
    gain.gain.exponentialRampToValueAtTime(0.001, ctx.currentTime + 0.08);
    osc.connect(gain);
    gain.connect(ctx.destination);
    osc.start(ctx.currentTime);
    osc.stop(ctx.currentTime + 0.08);
    // Disconnect nodes after playback to prevent GC-pinned AudioNode leaks
    osc.onended = () => { try { gain.disconnect(); } catch { /* noop */ } };
  } catch {
    // Silently ignore — audio context may not be available
  }
}

/**
 * Play a success chime (for export complete, kit built, etc).
 * Quick ascending two-note chime: A5 -> E6.
 * Theme-aware: adjusts character based on current theme.
 */
export function playSuccessChime(): void {
  try {
    const ctx = getAudioContext();
    const char = getThemeSoundCharacter();
    [880, 1318.5].forEach((freq, i) => {
      const osc = ctx.createOscillator();
      const gain = ctx.createGain();
      osc.frequency.value = freq * char.freqMultiplier;
      osc.type = char.waveform;
      osc.detune.value = char.detune;
      const start = ctx.currentTime + i * 0.1;
      gain.gain.setValueAtTime(0.04, start);
      gain.gain.exponentialRampToValueAtTime(0.001, start + 0.15);
      osc.connect(gain);
      gain.connect(ctx.destination);
      osc.start(start);
      osc.stop(start + 0.15);
      osc.onended = () => { try { gain.disconnect(); } catch { /* noop */ } };
    });
  } catch {
    // Silently ignore
  }
}

/**
 * Play a low thud sound (for errors or warnings).
 * Theme-aware: adjusts character based on current theme.
 */
export function playErrorThud(): void {
  try {
    const ctx = getAudioContext();
    const char = getThemeSoundCharacter();
    const osc = ctx.createOscillator();
    const gain = ctx.createGain();
    osc.frequency.value = 110 * char.freqMultiplier;
    osc.type = 'triangle'; // Always triangle for error (warm, noticeable)
    osc.detune.value = char.detune;
    gain.gain.setValueAtTime(0.06, ctx.currentTime);
    gain.gain.exponentialRampToValueAtTime(0.001, ctx.currentTime + 0.2);
    osc.connect(gain);
    gain.connect(ctx.destination);
    osc.start(ctx.currentTime);
    osc.stop(ctx.currentTime + 0.2);
    osc.onended = () => { try { gain.disconnect(); } catch { /* noop */ } };
  } catch {
    // Silently ignore
  }
}
