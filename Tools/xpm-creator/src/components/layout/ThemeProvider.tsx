'use client';

import { useEffect } from 'react';
import { useThemeStore, applyThemeToDOM, THEMES } from '@/stores/themeStore';

/**
 * ThemeProvider — Client component that applies the active theme's
 * CSS custom properties to the document root on mount and on theme change.
 * Renders nothing visible.
 */
export default function ThemeProvider() {
  const activeThemeId = useThemeStore((s) => s.activeThemeId);

  useEffect(() => {
    const theme = THEMES.find((t) => t.id === activeThemeId) || THEMES[0];
    applyThemeToDOM(theme);
  }, [activeThemeId]);

  return null;
}
