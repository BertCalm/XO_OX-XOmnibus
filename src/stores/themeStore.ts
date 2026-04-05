'use client';

import { create } from 'zustand';
import { persist } from 'zustand/middleware';

// ---------------------------------------------------------------------------
// Types
// ---------------------------------------------------------------------------

export type TextureType = 'none' | 'dust' | 'grid' | 'halftone' | 'noise';
export type WaveformStyle = 'default' | 'neon' | 'aliased' | 'gradient';

export interface ThemeColors {
  surface: string;
  surfaceAlt: string;
  surfaceBg: string;
  border: string;
  borderHover: string;
  textPrimary: string;
  textSecondary: string;
  textMuted: string;
  accentTeal: string;
  accentTealLight: string;
  accentTealDark: string;
  accentTeal50: string;
  accentPlum: string;
  accentPlumLight: string;
  accentPlumDark: string;
  accentPlum50: string;
}

export interface ThemeDefinition {
  id: string;
  name: string;
  description: string;
  category: 'light' | 'dark';
  texture: TextureType;
  waveformStyle: WaveformStyle;
  colors: ThemeColors;
}

// ---------------------------------------------------------------------------
// Color Utility Helpers
// ---------------------------------------------------------------------------

function hexToRgbArray(hex: string): [number, number, number] {
  const r = parseInt(hex.slice(1, 3), 16);
  const g = parseInt(hex.slice(3, 5), 16);
  const b = parseInt(hex.slice(5, 7), 16);
  return [r, g, b];
}

function rgbArrayToHex(rgb: [number, number, number]): string {
  return (
    '#' +
    rgb
      .map((v) =>
        Math.max(0, Math.min(255, Math.round(v)))
          .toString(16)
          .padStart(2, '0')
      )
      .join('')
  );
}

function blendColors(base: string, mix: string, amount: number): string {
  const [r1, g1, b1] = hexToRgbArray(base);
  const [r2, g2, b2] = hexToRgbArray(mix);
  return rgbArrayToHex([
    Math.round(r1 + (r2 - r1) * amount),
    Math.round(g1 + (g2 - g1) * amount),
    Math.round(b1 + (b2 - b1) * amount),
  ]);
}

function darken(hex: string, amount: number): string {
  const [r, g, b] = hexToRgbArray(hex);
  return rgbArrayToHex([
    Math.round(r * (1 - amount)),
    Math.round(g * (1 - amount)),
    Math.round(b * (1 - amount)),
  ]);
}

function lighten(hex: string, amount: number): string {
  const [r, g, b] = hexToRgbArray(hex);
  return rgbArrayToHex([
    Math.round(r + (255 - r) * amount),
    Math.round(g + (255 - g) * amount),
    Math.round(b + (255 - b) * amount),
  ]);
}

// ---------------------------------------------------------------------------
// Theme Factory — build full 16-color palette from 9 base colors
// ---------------------------------------------------------------------------

interface ThemeBase {
  id: string;
  name: string;
  description: string;
  category: 'light' | 'dark';
  texture: TextureType;
  waveformStyle: WaveformStyle;
  surface: string;
  surfaceAlt: string;
  surfaceBg: string;
  border: string;
  textPrimary: string;
  textSecondary: string;
  textMuted: string;
  accentPrimary: string;
  accentSecondary: string;
}

function buildTheme(base: ThemeBase): ThemeDefinition {
  const isLight = base.category === 'light';
  return {
    id: base.id,
    name: base.name,
    description: base.description,
    category: base.category,
    texture: base.texture,
    waveformStyle: base.waveformStyle,
    colors: {
      surface: base.surface,
      surfaceAlt: base.surfaceAlt,
      surfaceBg: base.surfaceBg,
      border: base.border,
      borderHover: isLight ? darken(base.border, 0.15) : lighten(base.border, 0.15),
      textPrimary: base.textPrimary,
      textSecondary: base.textSecondary,
      textMuted: base.textMuted,
      accentTeal: base.accentPrimary,
      accentTealLight: blendColors(base.surface, base.accentPrimary, isLight ? 0.12 : 0.18),
      accentTealDark: isLight ? darken(base.accentPrimary, 0.14) : lighten(base.accentPrimary, 0.12),
      accentTeal50: blendColors(base.surfaceBg, base.accentPrimary, isLight ? 0.04 : 0.07),
      accentPlum: base.accentSecondary,
      accentPlumLight: blendColors(base.surface, base.accentSecondary, isLight ? 0.12 : 0.18),
      accentPlumDark: isLight ? darken(base.accentSecondary, 0.14) : lighten(base.accentSecondary, 0.12),
      accentPlum50: blendColors(base.surfaceBg, base.accentSecondary, isLight ? 0.04 : 0.07),
    },
  };
}

// ---------------------------------------------------------------------------
// The 20 Hip-Hop Inspired Themes
// ---------------------------------------------------------------------------

const THEME_BASES: ThemeBase[] = [
  // ---- LIGHT THEMES ----
  {
    id: 'studio-clean',
    name: 'Studio Clean',
    description: 'Professional light interface',
    category: 'light',
    texture: 'none',
    waveformStyle: 'default',
    surface: '#FFFFFF',
    surfaceAlt: '#F5F5F5',
    surfaceBg: '#FAFAFA',
    border: '#E8E8E8',
    textPrimary: '#1A1A2E',
    textSecondary: '#6B7280',
    textMuted: '#9CA3AF',
    accentPrimary: '#0D9488',
    accentSecondary: '#7C3AED',
  },
  {
    id: 'cloud-rap',
    name: 'Cloud Rap',
    description: 'Dreamy pastel waves — A$AP Rocky vibes',
    category: 'light',
    texture: 'dust',
    waveformStyle: 'gradient',
    surface: '#F0EBFA',
    surfaceAlt: '#E6E1F5',
    surfaceBg: '#F5F0FF',
    border: '#D2C8E6',
    textPrimary: '#3C3250',
    textSecondary: '#786E96',
    textMuted: '#A096BE',
    accentPrimary: '#A078F0',
    accentSecondary: '#FA82B4',
  },
  {
    id: 'ice-cold',
    name: 'Ice Cold',
    description: 'Clean blue frost — OutKast clarity',
    category: 'light',
    texture: 'none',
    waveformStyle: 'default',
    surface: '#F0F5FA',
    surfaceAlt: '#E4EBF5',
    surfaceBg: '#EBF0F8',
    border: '#C8D2E1',
    textPrimary: '#1E283C',
    textSecondary: '#505F78',
    textMuted: '#8291AA',
    accentPrimary: '#288CDC',
    accentSecondary: '#64B4F0',
  },

  // ---- DARK THEMES ----
  {
    id: 'midnight-mpc',
    name: 'Midnight MPC',
    description: 'Gold-lit studio darkness — late night sessions',
    category: 'dark',
    texture: 'dust',
    waveformStyle: 'neon',
    surface: '#1E1E28',
    surfaceAlt: '#161620',
    surfaceBg: '#101018',
    border: '#37374A',
    textPrimary: '#F0F0F5',
    textSecondary: '#A0A0B4',
    textMuted: '#6E6E82',
    accentPrimary: '#FFB74D',
    accentSecondary: '#FF7043',
  },
  {
    id: 'boom-bap',
    name: 'Boom Bap',
    description: 'Blueprint schematic — DJ Premier era',
    category: 'dark',
    texture: 'grid',
    waveformStyle: 'aliased',
    surface: '#232A38',
    surfaceAlt: '#1C2230',
    surfaceBg: '#161C28',
    border: '#374258',
    textPrimary: '#DCE1EB',
    textSecondary: '#96A0B4',
    textMuted: '#6A7488',
    accentPrimary: '#64B5F6',
    accentSecondary: '#FFC864',
  },
  {
    id: 'lo-fi-tape',
    name: 'Lo-Fi Tape',
    description: 'Warm cassette haze — J Dilla warmth',
    category: 'dark',
    texture: 'noise',
    waveformStyle: 'aliased',
    surface: '#3E362E',
    surfaceAlt: '#342C24',
    surfaceBg: '#2A241E',
    border: '#504840',
    textPrimary: '#E6DCC8',
    textSecondary: '#B0A08C',
    textMuted: '#827264',
    accentPrimary: '#E6AA50',
    accentSecondary: '#C8825A',
  },
  {
    id: 'neon-trap',
    name: 'Neon Trap',
    description: 'Cyber glow — Metro Boomin future',
    category: 'dark',
    texture: 'grid',
    waveformStyle: 'neon',
    surface: '#12121E',
    surfaceAlt: '#0C0C18',
    surfaceBg: '#080812',
    border: '#282841',
    textPrimary: '#E6E6FA',
    textSecondary: '#9696BE',
    textMuted: '#64648C',
    accentPrimary: '#00FFC8',
    accentSecondary: '#FF32C8',
  },
  {
    id: 'golden-era',
    name: 'Golden Era',
    description: '90s warmth — Tribe Called Quest gold',
    category: 'dark',
    texture: 'dust',
    waveformStyle: 'default',
    surface: '#2D261C',
    surfaceAlt: '#262016',
    surfaceBg: '#201B12',
    border: '#413828',
    textPrimary: '#F0E1C8',
    textSecondary: '#B9AA91',
    textMuted: '#877864',
    accentPrimary: '#DAA520',
    accentSecondary: '#B4643C',
  },
  {
    id: 'crate-digger',
    name: 'Crate Digger',
    description: 'Dusty vinyl earth — Madlib basement',
    category: 'dark',
    texture: 'dust',
    waveformStyle: 'aliased',
    surface: '#373028',
    surfaceAlt: '#2D2620',
    surfaceBg: '#26201A',
    border: '#4B4237',
    textPrimary: '#E6D9C3',
    textSecondary: '#AA9B87',
    textMuted: '#7D6E5F',
    accentPrimary: '#A0C878',
    accentSecondary: '#C8A064',
  },
  {
    id: 'south-side',
    name: 'South Side',
    description: 'Aggressive heat — Houston screw culture',
    category: 'dark',
    texture: 'noise',
    waveformStyle: 'neon',
    surface: '#1C1212',
    surfaceAlt: '#160C0C',
    surfaceBg: '#100808',
    border: '#371E1E',
    textPrimary: '#F0E1E1',
    textSecondary: '#B49696',
    textMuted: '#826464',
    accentPrimary: '#DC2828',
    accentSecondary: '#FF8C28',
  },
  {
    id: '808-minimal',
    name: '808 Minimal',
    description: 'Stripped to the bone — Roland machine aesthetic',
    category: 'dark',
    texture: 'none',
    waveformStyle: 'default',
    surface: '#141414',
    surfaceAlt: '#0E0E0E',
    surfaceBg: '#080808',
    border: '#2D2D2D',
    textPrimary: '#DCDCDC',
    textSecondary: '#8C8C8C',
    textMuted: '#5A5A5A',
    accentPrimary: '#C81E1E',
    accentSecondary: '#A0A0A0',
  },
  {
    id: 'jazz-lounge',
    name: 'Jazz Lounge',
    description: 'Smoky blue satin — Robert Glasper sessions',
    category: 'dark',
    texture: 'dust',
    waveformStyle: 'gradient',
    surface: '#1E2030',
    surfaceAlt: '#181A28',
    surfaceBg: '#121420',
    border: '#323448',
    textPrimary: '#E1DCF0',
    textSecondary: '#A09BB4',
    textMuted: '#6E6987',
    accentPrimary: '#E6B450',
    accentSecondary: '#6496E6',
  },
  {
    id: 'vinyl-sunrise',
    name: 'Vinyl Sunrise',
    description: 'Warm amber glow — Kanye soul samples',
    category: 'dark',
    texture: 'halftone',
    waveformStyle: 'gradient',
    surface: '#322820',
    surfaceAlt: '#2A201C',
    surfaceBg: '#231A16',
    border: '#4A3A32',
    textPrimary: '#F5E6D7',
    textSecondary: '#BEAA96',
    textMuted: '#8C7D6E',
    accentPrimary: '#FF8C32',
    accentSecondary: '#FFC850',
  },
  {
    id: 'purple-reign',
    name: 'Purple Reign',
    description: 'Royal velvet — Chopped Not Slopped',
    category: 'dark',
    texture: 'dust',
    waveformStyle: 'neon',
    surface: '#231932',
    surfaceAlt: '#1C122A',
    surfaceBg: '#160E23',
    border: '#3C2D50',
    textPrimary: '#EBE1FA',
    textSecondary: '#AA9BC3',
    textMuted: '#786A8F',
    accentPrimary: '#B464FF',
    accentSecondary: '#FFB464',
  },
  {
    id: 'chrome-city',
    name: 'Chrome City',
    description: 'Metallic precision — Timbaland futurism',
    category: 'dark',
    texture: 'grid',
    waveformStyle: 'default',
    surface: '#282C32',
    surfaceAlt: '#20242A',
    surfaceBg: '#1A1C22',
    border: '#3C414B',
    textPrimary: '#E6EBF0',
    textSecondary: '#A0AAB9',
    textMuted: '#6E7582',
    accentPrimary: '#AAC8E6',
    accentSecondary: '#C8D2DC',
  },
  {
    id: 'blaxploitation',
    name: 'Blaxploitation',
    description: 'Bold 70s funk — Isaac Hayes cinema',
    category: 'dark',
    texture: 'halftone',
    waveformStyle: 'gradient',
    surface: '#322314',
    surfaceAlt: '#2A1C0E',
    surfaceBg: '#24180A',
    border: '#463423',
    textPrimary: '#FFEBCD',
    textSecondary: '#C8AF8C',
    textMuted: '#967D69',
    accentPrimary: '#FF8C00',
    accentSecondary: '#DCB43C',
  },
  {
    id: 'concrete-jungle',
    name: 'Concrete Jungle',
    description: 'Urban canopy — NYC rooftop cyphers',
    category: 'dark',
    texture: 'grid',
    waveformStyle: 'default',
    surface: '#2A2E2A',
    surfaceAlt: '#222622',
    surfaceBg: '#1C1E1C',
    border: '#3C423C',
    textPrimary: '#E1E6E1',
    textSecondary: '#9BA59B',
    textMuted: '#6E766E',
    accentPrimary: '#64BE78',
    accentSecondary: '#B4BEA0',
  },
  {
    id: 'tokyo-drift',
    name: 'Tokyo Drift',
    description: 'Neon red slash — Nujabes meets Akira',
    category: 'dark',
    texture: 'grid',
    waveformStyle: 'neon',
    surface: '#141018',
    surfaceAlt: '#0E0A12',
    surfaceBg: '#0A060E',
    border: '#2D2637',
    textPrimary: '#F0EBFA',
    textSecondary: '#AAA0BE',
    textMuted: '#736887',
    accentPrimary: '#FF2850',
    accentSecondary: '#50C8FF',
  },
  {
    id: 'swamp-funk',
    name: 'Swamp Funk',
    description: 'Murky bayou green — Outkast Aquemini',
    category: 'dark',
    texture: 'noise',
    waveformStyle: 'aliased',
    surface: '#1C2319',
    surfaceAlt: '#161C12',
    surfaceBg: '#10160E',
    border: '#303A2A',
    textPrimary: '#D7E1D2',
    textSecondary: '#96A58C',
    textMuted: '#697662',
    accentPrimary: '#8CC83C',
    accentSecondary: '#C8B450',
  },
  {
    id: 'studio-midnight',
    name: 'Studio Midnight',
    description: 'Pro dark console — Pharrell in the lab',
    category: 'dark',
    texture: 'none',
    waveformStyle: 'default',
    surface: '#202228',
    surfaceAlt: '#1A1C24',
    surfaceBg: '#14161C',
    border: '#343844',
    textPrimary: '#E6E8F0',
    textSecondary: '#9BA0B4',
    textMuted: '#696E82',
    accentPrimary: '#3CB4AA',
    accentSecondary: '#8C64DC',
  },
];

// Build all themes from base definitions
export const THEMES: ThemeDefinition[] = THEME_BASES.map(buildTheme);

// ---------------------------------------------------------------------------
// CSS Variable Mapping
// ---------------------------------------------------------------------------

const VAR_MAP: Record<keyof ThemeColors, string> = {
  surface: '--color-surface',
  surfaceAlt: '--color-surface-alt',
  surfaceBg: '--color-surface-bg',
  border: '--color-border',
  borderHover: '--color-border-hover',
  textPrimary: '--color-text-primary',
  textSecondary: '--color-text-secondary',
  textMuted: '--color-text-muted',
  accentTeal: '--color-accent-teal',
  accentTealLight: '--color-accent-teal-light',
  accentTealDark: '--color-accent-teal-dark',
  accentTeal50: '--color-accent-teal-50',
  accentPlum: '--color-accent-plum',
  accentPlumLight: '--color-accent-plum-light',
  accentPlumDark: '--color-accent-plum-dark',
  accentPlum50: '--color-accent-plum-50',
};

/** Convert hex "#RRGGBB" → "R G B" for Tailwind CSS variable consumption */
export function hexToRgbString(hex: string): string {
  const [r, g, b] = hexToRgbArray(hex);
  return `${r} ${g} ${b}`;
}

/** Inject a theme's colors as CSS custom properties on the document root */
export function applyThemeToDOM(theme: ThemeDefinition): void {
  if (typeof document === 'undefined') return;
  const style = document.documentElement.style;

  // Inject all color variables as space-separated RGB
  for (const [key, cssVar] of Object.entries(VAR_MAP)) {
    const hex = theme.colors[key as keyof ThemeColors];
    style.setProperty(cssVar, hexToRgbString(hex));
  }

  // Inject shadow variables based on light/dark category
  if (theme.category === 'dark') {
    style.setProperty(
      '--shadow-card',
      '0 1px 3px rgba(0,0,0,0.3), 0 1px 2px rgba(0,0,0,0.4)'
    );
    style.setProperty(
      '--shadow-elevated',
      '0 4px 6px -1px rgba(0,0,0,0.35), 0 2px 4px -2px rgba(0,0,0,0.3)'
    );
    style.setProperty(
      '--shadow-modal',
      '0 20px 25px -5px rgba(0,0,0,0.5), 0 8px 10px -6px rgba(0,0,0,0.4)'
    );
  } else {
    style.setProperty(
      '--shadow-card',
      '0 1px 3px rgba(0,0,0,0.04), 0 1px 2px rgba(0,0,0,0.06)'
    );
    style.setProperty(
      '--shadow-elevated',
      '0 4px 6px -1px rgba(0,0,0,0.05), 0 2px 4px -2px rgba(0,0,0,0.05)'
    );
    style.setProperty(
      '--shadow-modal',
      '0 20px 25px -5px rgba(0,0,0,0.1), 0 8px 10px -6px rgba(0,0,0,0.1)'
    );
  }

  // Set data attribute for any CSS selectors that need it
  document.documentElement.setAttribute('data-theme', theme.id);
  document.documentElement.setAttribute('data-theme-mode', theme.category);
}

// ---------------------------------------------------------------------------
// Zustand Store
// ---------------------------------------------------------------------------

interface ThemeState {
  activeThemeId: string;
  textureOverride: TextureType | null;

  setTheme: (themeId: string) => void;
  setTexture: (texture: TextureType | null) => void;
  getActiveTheme: () => ThemeDefinition;
  getActiveTexture: () => TextureType;
}

export const useThemeStore = create<ThemeState>()(
  persist(
    (set, get) => ({
      activeThemeId: 'studio-clean',
      textureOverride: null,

      setTheme: (themeId: string) => {
        const theme = THEMES.find((t) => t.id === themeId);
        if (!theme) return;
        set({ activeThemeId: themeId });
        applyThemeToDOM(theme);
      },

      setTexture: (texture: TextureType | null) => {
        set({ textureOverride: texture });
      },

      getActiveTheme: () => {
        const { activeThemeId } = get();
        return THEMES.find((t) => t.id === activeThemeId) || THEMES[0];
      },

      getActiveTexture: () => {
        const { textureOverride } = get();
        if (textureOverride !== null) return textureOverride;
        const theme = get().getActiveTheme();
        return theme.texture;
      },
    }),
    {
      name: 'xpm-theme',
      partialize: (state) => ({
        activeThemeId: state.activeThemeId,
        textureOverride: state.textureOverride,
      }),
    }
  )
);
