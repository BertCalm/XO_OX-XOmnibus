'use client';

import React, { useState, useCallback, useRef } from 'react';
import { useThemeStore, THEMES, applyThemeToDOM, type TextureType } from '@/stores/themeStore';
import Modal from './Modal';

const TEXTURE_OPTIONS: { id: TextureType; label: string; icon: string }[] = [
  { id: 'none', label: 'Clean', icon: '✦' },
  { id: 'dust', label: 'Dust', icon: '✧' },
  { id: 'grid', label: 'Grid', icon: '▦' },
  { id: 'halftone', label: 'Halftone', icon: '◍' },
  { id: 'noise', label: 'Noise', icon: '░' },
];

type FilterCategory = 'all' | 'light' | 'dark' | 'vintage' | 'neon';

/** Categorize themes by mood beyond just light/dark */
function getThemeMood(themeId: string): 'neon' | 'vintage' | 'light' | 'dark' {
  if (/neon|trap|tokyo|chrome/.test(themeId)) return 'neon';
  if (/lofi|tape|crate|vinyl|golden|blax|swamp/.test(themeId)) return 'vintage';
  const theme = THEMES.find((t) => t.id === themeId);
  return theme?.category === 'light' ? 'light' : 'dark';
}

const MOOD_LABELS: Record<FilterCategory, string> = {
  all: 'All',
  light: 'Light',
  dark: 'Dark',
  vintage: 'Vintage',
  neon: 'Neon',
};

/** Small color circle preview showing theme's key colors */
export default function ThemeSwitcher() {
  const [open, setOpen] = useState(false);
  const [filter, setFilter] = useState<FilterCategory>('all');
  const { activeThemeId, setTheme, textureOverride, setTexture, getActiveTheme } = useThemeStore();
  const hoverTimeoutRef = useRef<ReturnType<typeof setTimeout> | null>(null);

  const activeTheme = getActiveTheme();
  const filteredThemes =
    filter === 'all'
      ? THEMES
      : THEMES.filter((t) => {
          const mood = getThemeMood(t.id);
          if (filter === 'light' || filter === 'dark') return t.category === filter;
          return mood === filter;
        });

  // Hover preview: temporarily apply theme visuals, revert on mouse leave
  const handleHoverEnter = useCallback(
    (themeId: string) => {
      if (hoverTimeoutRef.current) clearTimeout(hoverTimeoutRef.current);
      hoverTimeoutRef.current = setTimeout(() => {
        const theme = THEMES.find((t) => t.id === themeId);
        if (theme) applyThemeToDOM(theme);
      }, 200); // Small delay to avoid flickering on fast mouse movement
    },
    []
  );

  const handleHoverLeave = useCallback(() => {
    if (hoverTimeoutRef.current) {
      clearTimeout(hoverTimeoutRef.current);
      hoverTimeoutRef.current = null;
    }
    // Revert to the actual active theme
    applyThemeToDOM(activeTheme);
  }, [activeTheme]);

  return (
    <>
      {/* Trigger button */}
      <button
        onClick={() => setOpen(true)}
        className="flex items-center gap-1.5 px-2.5 py-1.5 rounded-lg text-xs font-medium
          bg-surface-alt text-text-secondary hover:text-text-primary hover:bg-surface
          transition-all border border-transparent hover:border-border"
        title="Theme"
      >
        <svg width="14" height="14" viewBox="0 0 16 16" fill="none">
          <circle cx="8" cy="8" r="7" stroke="currentColor" strokeWidth="1.5" />
          <path
            d="M8 1a7 7 0 0 0 0 14V1z"
            fill="currentColor"
            fillOpacity="0.3"
          />
          <circle cx="6" cy="6" r="1.5" fill="currentColor" fillOpacity="0.6" />
          <circle cx="10" cy="5" r="1" fill="currentColor" fillOpacity="0.4" />
          <circle cx="10" cy="10" r="1.2" fill="currentColor" fillOpacity="0.5" />
        </svg>
        <span className="hidden sm:inline">Theme</span>
      </button>

      {/* Theme picker modal */}
      <Modal open={open} onClose={() => setOpen(false)} title="Aesthetic Engine" size="xl">
        <div className="space-y-4">
          {/* Category filter */}
          <div className="flex gap-1 flex-wrap">
            {(['all', 'light', 'dark', 'vintage', 'neon'] as FilterCategory[]).map((cat) => (
              <button
                key={cat}
                onClick={() => setFilter(cat)}
                aria-pressed={filter === cat}
                className={`px-3 py-1 rounded-lg text-xs font-medium transition-all
                  ${filter === cat
                    ? 'bg-accent-teal text-white'
                    : 'bg-surface-alt text-text-secondary hover:text-text-primary'
                  }`}
              >
                {MOOD_LABELS[cat]}
              </button>
            ))}
          </div>

          {/* Theme grid */}
          <div className="grid grid-cols-2 sm:grid-cols-3 gap-2 max-h-[50vh] overflow-y-auto scrollbar-thin pr-1">
            {filteredThemes.map((theme) => {
              const isActive = theme.id === activeThemeId;
              return (
                <button
                  key={theme.id}
                  onClick={() => setTheme(theme.id)}
                  onMouseEnter={() => handleHoverEnter(theme.id)}
                  onMouseLeave={handleHoverLeave}
                  aria-pressed={isActive}
                  aria-label={`${theme.name} theme — ${theme.description}`}
                  className={`text-left p-3 rounded-xl border-2 transition-all
                    ${isActive
                      ? 'border-accent-teal shadow-elevated'
                      : 'border-transparent hover:border-border'
                    }`}
                  style={{
                    backgroundColor: theme.colors.surface,
                  }}
                >
                  <div className="flex items-start justify-between mb-2">
                    <div className="min-w-0">
                      <p
                        className="text-xs font-semibold truncate"
                        style={{ color: theme.colors.textPrimary }}
                      >
                        {theme.name}
                      </p>
                      <p
                        className="text-[10px] truncate mt-0.5"
                        style={{ color: theme.colors.textMuted }}
                      >
                        {theme.description}
                      </p>
                    </div>
                    {isActive && (
                      <span
                        className="shrink-0 w-4 h-4 rounded-full flex items-center justify-center text-[8px] ml-1"
                        style={{
                          backgroundColor: theme.colors.accentTeal,
                          color: '#fff',
                        }}
                      >
                        ✓
                      </span>
                    )}
                  </div>

                  {/* Color preview strip */}
                  <div className="flex gap-1 mt-1">
                    <span
                      className="w-4 h-4 rounded-md border border-black/10"
                      style={{ backgroundColor: theme.colors.surfaceBg }}
                    />
                    <span
                      className="w-4 h-4 rounded-md border border-black/10"
                      style={{ backgroundColor: theme.colors.accentTeal }}
                    />
                    <span
                      className="w-4 h-4 rounded-md border border-black/10"
                      style={{ backgroundColor: theme.colors.accentPlum }}
                    />
                    <span
                      className="w-4 h-4 rounded-md border border-black/10"
                      style={{ backgroundColor: theme.colors.textPrimary }}
                    />
                    <span
                      className="flex-1 h-4 rounded-md border border-black/10"
                      style={{
                        background: `linear-gradient(135deg, ${theme.colors.accentTeal} 0%, ${theme.colors.accentPlum} 100%)`,
                      }}
                    />
                  </div>

                  {/* Category badge */}
                  <div className="mt-2 flex items-center gap-1">
                    <span
                      className="text-[9px] px-1.5 py-0.5 rounded-full font-medium uppercase tracking-wider"
                      style={{
                        backgroundColor:
                          theme.category === 'dark'
                            ? 'rgba(255,255,255,0.1)'
                            : 'rgba(0,0,0,0.06)',
                        color: theme.colors.textSecondary,
                      }}
                    >
                      {theme.category}
                    </span>
                    {theme.texture !== 'none' && (
                      <span
                        className="text-[9px] px-1.5 py-0.5 rounded-full font-medium uppercase tracking-wider"
                        style={{
                          backgroundColor:
                            theme.category === 'dark'
                              ? 'rgba(255,255,255,0.06)'
                              : 'rgba(0,0,0,0.04)',
                          color: theme.colors.textMuted,
                        }}
                      >
                        {theme.texture}
                      </span>
                    )}
                  </div>
                </button>
              );
            })}
          </div>

          {/* Texture override section */}
          <div className="border-t border-border pt-3">
            <p className="text-[10px] text-text-muted uppercase tracking-wider font-semibold mb-2">
              Texture Overlay
            </p>
            <div className="flex gap-1.5">
              {TEXTURE_OPTIONS.map((tex) => {
                const isActiveTexture =
                  textureOverride === null
                    ? activeTheme.texture === tex.id
                    : textureOverride === tex.id;
                const isOverride = textureOverride === tex.id;
                return (
                  <button
                    key={tex.id}
                    onClick={() => {
                      // Toggle: if clicking the current override, clear it (revert to theme default)
                      // If clicking a new one, set the override
                      if (isOverride) {
                        setTexture(null);
                      } else {
                        setTexture(tex.id);
                      }
                    }}
                    aria-pressed={isActiveTexture}
                    className={`flex items-center gap-1 px-2.5 py-1.5 rounded-lg text-[10px] font-medium
                      transition-all border
                      ${isActiveTexture
                        ? 'bg-accent-teal/10 border-accent-teal/30 text-accent-teal'
                        : 'bg-surface-alt border-transparent text-text-muted hover:text-text-secondary'
                      }`}
                  >
                    <span className="text-xs">{tex.icon}</span>
                    {tex.label}
                  </button>
                );
              })}
            </div>
            {textureOverride !== null && (
              <p className="text-[9px] text-text-muted mt-1.5">
                Override active — click again to restore theme default
              </p>
            )}
          </div>
        </div>
      </Modal>
    </>
  );
}
