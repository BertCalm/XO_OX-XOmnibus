'use client';

import React from 'react';

interface ShortcutBadgeProps {
  /** Array of key labels to render, e.g. ['⌘', 'K'] */
  keys: string[];
  className?: string;
}

/**
 * ShortcutBadge — renders keyboard shortcut keys in a consistent
 * visual style. Used in CommandPalette items and Tooltip shortcuts.
 */
export default function ShortcutBadge({ keys, className = '' }: ShortcutBadgeProps) {
  if (keys.length === 0) return null;

  return (
    <span className={`inline-flex items-center gap-0.5 ${className}`}>
      {keys.map((key, i) => (
        <kbd
          key={i}
          className="inline-flex items-center justify-center min-w-[18px] h-[18px]
            px-1 rounded bg-surface-alt border border-border
            text-[10px] font-mono text-text-muted leading-none"
        >
          {key}
        </kbd>
      ))}
    </span>
  );
}
