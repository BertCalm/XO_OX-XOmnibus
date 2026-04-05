import type { Config } from 'tailwindcss';

/**
 * All custom colors reference CSS custom properties (set by ThemeProvider).
 * Variables are space-separated RGB values, e.g. --color-surface: 255 255 255
 * The `<alpha-value>` placeholder enables Tailwind opacity modifiers like bg-surface/50.
 */
function cssVar(name: string) {
  return `rgb(var(${name}) / <alpha-value>)`;
}

const config: Config = {
  content: [
    './src/components/**/*.{js,ts,jsx,tsx,mdx}',
    './src/app/**/*.{js,ts,jsx,tsx,mdx}',
  ],
  theme: {
    extend: {
      colors: {
        surface: {
          DEFAULT: cssVar('--color-surface'),
          alt: cssVar('--color-surface-alt'),
          bg: cssVar('--color-surface-bg'),
        },
        border: {
          DEFAULT: cssVar('--color-border'),
          hover: cssVar('--color-border-hover'),
        },
        text: {
          primary: cssVar('--color-text-primary'),
          secondary: cssVar('--color-text-secondary'),
          muted: cssVar('--color-text-muted'),
        },
        accent: {
          teal: {
            DEFAULT: cssVar('--color-accent-teal'),
            light: cssVar('--color-accent-teal-light'),
            dark: cssVar('--color-accent-teal-dark'),
            50: cssVar('--color-accent-teal-50'),
          },
          plum: {
            DEFAULT: cssVar('--color-accent-plum'),
            light: cssVar('--color-accent-plum-light'),
            dark: cssVar('--color-accent-plum-dark'),
            50: cssVar('--color-accent-plum-50'),
          },
        },
      },
      fontFamily: {
        sans: ['Inter', 'system-ui', '-apple-system', 'sans-serif'],
        mono: ['JetBrains Mono', 'Fira Code', 'monospace'],
      },
      boxShadow: {
        card: 'var(--shadow-card)',
        elevated: 'var(--shadow-elevated)',
        modal: 'var(--shadow-modal)',
        'glass-inset': 'inset 0 1px 1px rgba(255,255,255,0.1), inset 0 -1px 1px rgba(0,0,0,0.4)',
        'glass-raised': '0 8px 32px rgba(0,0,0,0.5), inset 0 1px 1px rgba(255,255,255,0.08)',
        'glass-sunken': 'inset 0 2px 8px rgba(0,0,0,0.6), inset 0 1px 2px rgba(0,0,0,0.4)',
      },
      backgroundImage: {
        'gradient-progress':
          'linear-gradient(135deg, rgb(var(--color-accent-teal)) 0%, rgb(var(--color-accent-plum)) 100%)',
      },
    },
  },
  plugins: [],
};

export default config;
