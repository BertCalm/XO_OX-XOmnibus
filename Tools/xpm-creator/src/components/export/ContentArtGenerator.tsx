'use client';

import React, { useRef, useEffect, useState, useCallback } from 'react';
import { useThemeStore, THEMES } from '@/stores/themeStore';
import { useToastStore } from '@/stores/toastStore';
import Button from '@/components/ui/Button';

interface ContentArtGeneratorProps {
  title: string;
  subtitle?: string;
  /** Number of loaded pads (shown as lit pads in the grid) */
  padCount?: number;
  /** Sample categories present in the kit (for visual variety) */
  categories?: string[];
  /** Theme variation: 'default' | 'neon' | 'vintage' */
  variant?: 'default' | 'neon' | 'vintage';
  onGenerated?: (imageData: ArrayBuffer, type: 'png') => void;
}

/**
 * Canvas-based Content.png generator for XPN expansions.
 * Creates branded cover art using the active theme's colors.
 * The art is recognizable in the MPC hardware browser.
 */
export default function ContentArtGenerator({
  title,
  subtitle,
  padCount = 0,
  categories = [],
  variant = 'default',
  onGenerated,
}: ContentArtGeneratorProps) {
  const canvasRef = useRef<HTMLCanvasElement>(null);
  const activeThemeId = useThemeStore((s) => s.activeThemeId);
  const [previewUrl, setPreviewUrl] = useState<string>('');
  const prevUrlRef = useRef<string>('');

  // Clean up blob URLs on unmount and when preview changes
  useEffect(() => {
    return () => {
      if (prevUrlRef.current) URL.revokeObjectURL(prevUrlRef.current);
    };
  }, []);

  const renderArt = useCallback(() => {
    const canvas = canvasRef.current;
    if (!canvas) return;

    const ctx = canvas.getContext('2d');
    if (!ctx) return;

    const size = 480;
    canvas.width = size;
    canvas.height = size;

    const theme = THEMES.find((t) => t.id === activeThemeId) || THEMES[0];
    const colors = theme.colors;

    // Background gradient
    const bgGrad = ctx.createLinearGradient(0, 0, size, size);
    bgGrad.addColorStop(0, colors.surfaceBg);
    bgGrad.addColorStop(0.5, colors.surface);
    bgGrad.addColorStop(1, colors.surfaceAlt);
    ctx.fillStyle = bgGrad;
    ctx.fillRect(0, 0, size, size);

    // Subtle noise texture
    const imageData = ctx.getImageData(0, 0, size, size);
    const data = imageData.data;
    for (let i = 0; i < data.length; i += 4) {
      const noise = (Math.random() - 0.5) * 8;
      data[i] = Math.max(0, Math.min(255, data[i] + noise));
      data[i + 1] = Math.max(0, Math.min(255, data[i + 1] + noise));
      data[i + 2] = Math.max(0, Math.min(255, data[i + 2] + noise));
    }
    ctx.putImageData(imageData, 0, 0);

    // Accent glow orbs
    const drawOrb = (x: number, y: number, r: number, color: string, alpha: number) => {
      const grad = ctx.createRadialGradient(x, y, 0, x, y, r);
      grad.addColorStop(0, color + Math.round(alpha * 255).toString(16).padStart(2, '0'));
      grad.addColorStop(1, color + '00');
      ctx.fillStyle = grad;
      ctx.fillRect(x - r, y - r, r * 2, r * 2);
    };

    drawOrb(size * 0.2, size * 0.3, size * 0.4, colors.accentTeal, 0.15);
    drawOrb(size * 0.8, size * 0.7, size * 0.35, colors.accentPlum, 0.12);
    drawOrb(size * 0.5, size * 0.5, size * 0.25, colors.accentTealDark, 0.08);

    // XO_OX Logo (centered, large)
    ctx.save();
    const logoY = size * 0.32;
    const logoSize = size * 0.15;

    ctx.font = `bold ${logoSize}px "SF Mono", "Monaco", "Inconsolata", "Fira Mono", monospace`;
    ctx.textAlign = 'center';
    ctx.textBaseline = 'middle';

    // Shadow
    ctx.shadowColor = colors.accentTeal + '60';
    ctx.shadowBlur = 20;
    ctx.shadowOffsetX = 0;
    ctx.shadowOffsetY = 4;

    // Gradient text fill
    const textGrad = ctx.createLinearGradient(size * 0.25, logoY, size * 0.75, logoY);
    textGrad.addColorStop(0, colors.accentTeal);
    textGrad.addColorStop(0.5, colors.textPrimary);
    textGrad.addColorStop(1, colors.accentPlum);
    ctx.fillStyle = textGrad;
    ctx.fillText('XO_OX', size / 2, logoY);

    ctx.restore();

    // Theme variant color overrides
    const variantColors = {
      neon: { primary: '#00FF88', secondary: '#FF00FF', glow: true },
      vintage: { primary: '#D4A574', secondary: '#8B6914', glow: false },
      default: { primary: colors.accentTeal, secondary: colors.accentPlum, glow: false },
    };
    const vColors = variantColors[variant];

    // 4x4 Mini pad grid (decorative, below logo)
    // Pads light up based on padCount for dynamic kit representation
    const gridX = size * 0.3;
    const gridY = size * 0.48;
    const gridSize = size * 0.4;
    const padSize = gridSize / 4 - 4;
    const padGap = 4;
    const loadedPads = Math.min(padCount, 16);

    for (let row = 0; row < 4; row++) {
      for (let col = 0; col < 4; col++) {
        const padIdx = row * 4 + col;
        const px = gridX + col * (padSize + padGap);
        const py = gridY + row * (padSize + padGap);

        const isLoaded = padIdx < loadedPads;
        const isAccent = (row + col) % 3 === 0;

        if (isLoaded) {
          // Lit pad — brighter, with optional glow
          const opacity = 0.3 + Math.random() * 0.3;
          if (vColors.glow) {
            ctx.shadowColor = isAccent ? vColors.primary : vColors.secondary;
            ctx.shadowBlur = 6;
          }
          ctx.fillStyle = isAccent
            ? vColors.primary + Math.round(opacity * 255).toString(16).padStart(2, '0')
            : vColors.secondary + Math.round(opacity * 255).toString(16).padStart(2, '0');
        } else {
          // Unloaded pad — dim
          const opacity = 0.06 + Math.random() * 0.06;
          ctx.fillStyle = colors.textMuted + Math.round(opacity * 255).toString(16).padStart(2, '0');
          ctx.shadowBlur = 0;
        }

        // Rounded rect
        const r = 3;
        ctx.beginPath();
        ctx.moveTo(px + r, py);
        ctx.lineTo(px + padSize - r, py);
        ctx.quadraticCurveTo(px + padSize, py, px + padSize, py + r);
        ctx.lineTo(px + padSize, py + padSize - r);
        ctx.quadraticCurveTo(px + padSize, py + padSize, px + padSize - r, py + padSize);
        ctx.lineTo(px + r, py + padSize);
        ctx.quadraticCurveTo(px, py + padSize, px, py + padSize - r);
        ctx.lineTo(px, py + r);
        ctx.quadraticCurveTo(px, py, px + r, py);
        ctx.closePath();
        ctx.fill();
        ctx.shadowBlur = 0;
      }
    }

    // Category count badge (if categories provided)
    if (categories.length > 0) {
      ctx.save();
      const badgeY = size * 0.44;
      ctx.font = `${size * 0.025}px "SF Mono", "Monaco", monospace`;
      ctx.textAlign = 'center';
      ctx.fillStyle = colors.textMuted;
      const uniqueCategories = Array.from(new Set(categories));
      const catText = `${loadedPads} pads · ${uniqueCategories.length} categories`;
      ctx.fillText(catText, size / 2, badgeY);
      ctx.restore();
    }

    // Title text
    if (title) {
      ctx.save();
      const titleY = size * 0.82;
      ctx.font = `bold ${Math.min(size * 0.06, 28)}px -apple-system, BlinkMacSystemFont, "Segoe UI", sans-serif`;
      ctx.textAlign = 'center';
      ctx.textBaseline = 'middle';
      ctx.fillStyle = colors.textPrimary;
      ctx.shadowColor = 'rgba(0,0,0,0.5)';
      ctx.shadowBlur = 8;

      // Truncate if needed
      const displayTitle = title.length > 24 ? title.substring(0, 22) + '...' : title;
      ctx.fillText(displayTitle, size / 2, titleY);
      ctx.restore();
    }

    // Subtitle / creator
    if (subtitle) {
      ctx.save();
      const subY = size * 0.88;
      ctx.font = `${size * 0.035}px -apple-system, BlinkMacSystemFont, "Segoe UI", sans-serif`;
      ctx.textAlign = 'center';
      ctx.textBaseline = 'middle';
      ctx.fillStyle = colors.textSecondary;
      ctx.fillText(subtitle, size / 2, subY);
      ctx.restore();
    }

    // Bottom tag line
    ctx.save();
    ctx.font = `${size * 0.022}px "SF Mono", "Monaco", monospace`;
    ctx.textAlign = 'center';
    ctx.fillStyle = colors.textMuted;
    ctx.fillText('Crafted with XO_OX', size / 2, size * 0.95);
    ctx.restore();

    // Update preview using blob URL (more efficient than data URL)
    canvas.toBlob((blob) => {
      if (blob) {
        // Revoke previous blob URL to prevent memory leak
        if (prevUrlRef.current) URL.revokeObjectURL(prevUrlRef.current);
        const url = URL.createObjectURL(blob);
        prevUrlRef.current = url;
        setPreviewUrl(url);
      }
    }, 'image/png');
  }, [activeThemeId, title, subtitle, padCount, categories, variant]);

  useEffect(() => {
    renderArt();
  }, [renderArt]);

  const handleExport = useCallback(() => {
    const canvas = canvasRef.current;
    if (!canvas) return;

    canvas.toBlob((blob) => {
      if (blob && onGenerated) {
        blob.arrayBuffer().then((buffer) => {
          onGenerated(buffer, 'png');
        }).catch((err) => {
          console.error('Failed to convert cover art to ArrayBuffer:', err);
          useToastStore.getState().addToast({
            type: 'error',
            title: 'Cover art export failed',
            message: 'Could not process the generated cover art.',
          });
        });
      }
    }, 'image/png');
  }, [onGenerated]);

  return (
    <div className="space-y-2">
      <div className="flex items-center justify-between">
        <p className="text-[10px] text-text-muted uppercase tracking-wider font-semibold">
          Cover Art Preview
        </p>
        <Button variant="ghost" size="sm" onClick={handleExport}>
          Use as Cover
        </Button>
      </div>

      <div className="relative group">
        <canvas
          ref={canvasRef}
          className="hidden"
        />
        {previewUrl && (
          <img
            src={previewUrl}
            alt="Cover art preview"
            className="w-full max-w-[240px] aspect-square rounded-lg border border-border mx-auto"
          />
        )}
        <div className="absolute inset-0 flex items-center justify-center opacity-0 group-hover:opacity-100 transition-opacity">
          <Button variant="primary" size="sm" onClick={renderArt}>
            Regenerate
          </Button>
        </div>
      </div>

      <p className="text-[8px] text-text-muted text-center">
        480x480 PNG using {THEMES.find((t) => t.id === activeThemeId)?.name || 'current'} theme colors
      </p>
    </div>
  );
}
