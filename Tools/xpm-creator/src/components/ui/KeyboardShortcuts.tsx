'use client';

import React, { useEffect, useCallback } from 'react';

const SHORTCUTS = [
  {
    category: 'Navigation',
    shortcuts: [
      { keys: ['1'], description: 'The Crate (Samples)' },
      { keys: ['2'], description: 'The Forge (Pads)' },
      { keys: ['3'], description: 'Program Editor' },
      { keys: ['4'], description: 'The Breath (Export)' },
    ],
  },
  {
    category: 'Pad Operations',
    shortcuts: [
      { keys: ['\u2190', '\u2192'], description: 'Navigate pads' },
      { keys: ['\u2191\u2193'], description: 'Navigate pad rows' },
      { keys: ['Shift', '\u2191\u2193'], description: 'Navigate layers' },
      { keys: ['Space'], description: 'Play current pad' },
      { keys: ['Esc'], description: 'Stop all playback' },
      { keys: ['Del'], description: 'Clear current layer' },
      { keys: ['\u2318', 'Shift', 'C'], description: 'Copy pad' },
      { keys: ['\u2318', 'Shift', 'V'], description: 'Paste pad' },
      { keys: ['Shift', 'D'], description: 'Duplicate pad to next empty' },
      { keys: ['\u2318', 'K'], description: 'Command palette' },
    ],
  },
  {
    category: 'History',
    shortcuts: [
      { keys: ['\u2318', 'Z'], description: 'Undo' },
      { keys: ['\u2318', 'Shift', 'Z'], description: 'Redo' },
    ],
  },
  {
    category: 'Fun',
    shortcuts: [
      { keys: ['?'], description: 'Show shortcuts' },
      { keys: ['\u2191\u2191\u2193\u2193\u2190\u2192\u2190\u2192BA'], description: '???' },
    ],
  },
];

interface KeyboardShortcutsProps {
  open: boolean;
  onClose: () => void;
}

export default function KeyboardShortcuts({ open, onClose }: KeyboardShortcutsProps) {
  const handleKeyDown = useCallback(
    (e: KeyboardEvent) => {
      if (e.key === 'Escape') {
        onClose();
      }
    },
    [onClose]
  );

  useEffect(() => {
    if (open) {
      window.addEventListener('keydown', handleKeyDown);
      return () => window.removeEventListener('keydown', handleKeyDown);
    }
  }, [open, handleKeyDown]);

  if (!open) return null;

  return (
    <div
      className="fixed inset-0 z-50 flex items-center justify-center"
      onClick={onClose}
    >
      {/* Backdrop */}
      <div className="absolute inset-0 bg-black/60 backdrop-blur-sm" />

      {/* Panel */}
      <div
        className="relative bg-surface-primary border border-border rounded-2xl shadow-2xl max-w-lg w-full mx-4 max-h-[80vh] overflow-y-auto"
        onClick={(e) => e.stopPropagation()}
      >
        <div className="p-6">
          <div className="flex items-center justify-between mb-6">
            <h2 className="text-lg font-bold text-text-primary">Keyboard Shortcuts</h2>
            <button
              onClick={onClose}
              className="text-text-muted hover:text-text-primary transition-colors text-sm"
            >
              ESC
            </button>
          </div>

          <div className="space-y-6">
            {SHORTCUTS.map((group) => (
              <div key={group.category}>
                <h3 className="text-xs font-semibold text-text-secondary uppercase tracking-wider mb-3">
                  {group.category}
                </h3>
                <div className="space-y-2">
                  {group.shortcuts.map((shortcut, i) => (
                    <div
                      key={i}
                      className="flex items-center justify-between py-1.5"
                    >
                      <span className="text-sm text-text-primary">
                        {shortcut.description}
                      </span>
                      <div className="flex items-center gap-1">
                        {shortcut.keys.map((key, j) => (
                          <React.Fragment key={j}>
                            {j > 0 && (
                              <span className="text-text-muted text-xs">+</span>
                            )}
                            <kbd className="px-2 py-0.5 rounded-md bg-surface-secondary border border-border text-xs font-mono text-text-secondary min-w-[24px] text-center">
                              {key}
                            </kbd>
                          </React.Fragment>
                        ))}
                      </div>
                    </div>
                  ))}
                </div>
              </div>
            ))}
          </div>

          <div className="mt-6 pt-4 border-t border-border">
            <p className="text-xs text-text-muted text-center">
              Press <kbd className="px-1.5 py-0.5 rounded bg-surface-secondary border border-border text-[10px] font-mono">?</kbd> anytime to toggle this overlay
            </p>
          </div>
        </div>
      </div>
    </div>
  );
}
