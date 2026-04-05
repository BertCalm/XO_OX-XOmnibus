'use client';

import React from 'react';
import { useThemeStore } from '@/stores/themeStore';

/**
 * Full-screen texture overlay that responds to the active theme's texture setting.
 * Uses CSS patterns defined in globals.css — renders as a pointer-events-none layer.
 */
export default function TextureOverlay() {
  const texture = useThemeStore((s) => s.getActiveTexture());

  if (texture === 'none') return null;

  return <div className="texture-overlay" data-texture={texture} />;
}
