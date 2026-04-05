'use client';

import React, { useState, useEffect, useRef, useMemo, useCallback } from 'react';

export interface CommandAction {
  id: string;
  label: string;
  description?: string;
  icon?: string;
  category: string;
  shortcut?: string;
  action: () => void;
}

interface CommandPaletteProps {
  isOpen: boolean;
  onClose: () => void;
  actions: CommandAction[];
}

/**
 * Fuzzy match: checks if all characters in the query appear in the target
 * in order (case-insensitive). Returns a score where lower is better,
 * or -1 if no match.
 */
function fuzzyMatch(query: string, target: string): number {
  const q = query.toLowerCase();
  const t = target.toLowerCase();

  if (q.length === 0) return 0;

  let qi = 0;
  let score = 0;
  let lastMatchIndex = -1;

  for (let ti = 0; ti < t.length && qi < q.length; ti++) {
    if (t[ti] === q[qi]) {
      // Bonus for consecutive matches
      const gap = lastMatchIndex >= 0 ? ti - lastMatchIndex - 1 : ti;
      score += gap;
      lastMatchIndex = ti;
      qi++;
    }
  }

  // All query characters matched
  if (qi === q.length) return score;
  return -1;
}

export default function CommandPalette({ isOpen, onClose, actions }: CommandPaletteProps) {
  const [query, setQuery] = useState('');
  const [selectedIndex, setSelectedIndex] = useState(0);
  const inputRef = useRef<HTMLInputElement>(null);
  const listRef = useRef<HTMLDivElement>(null);
  const overlayRef = useRef<HTMLDivElement>(null);

  // Filter and score actions based on query
  const filteredActions = useMemo(() => {
    if (!query.trim()) return actions;

    const scored: { action: CommandAction; score: number }[] = [];
    for (const action of actions) {
      const labelScore = fuzzyMatch(query, action.label);
      const descScore = action.description
        ? fuzzyMatch(query, action.description)
        : -1;
      const catScore = fuzzyMatch(query, action.category);

      // Take the best (lowest non-negative) score
      const scores = [labelScore, descScore, catScore].filter((s) => s >= 0);
      if (scores.length > 0) {
        // Prioritize label matches
        const bestScore =
          labelScore >= 0
            ? labelScore
            : Math.min(...scores) + 100;
        scored.push({ action, score: bestScore });
      }
    }

    scored.sort((a, b) => a.score - b.score);
    return scored.map((s) => s.action);
  }, [query, actions]);

  // Group filtered actions by category (preserving order, O(n) via Map index)
  const groupedActions = useMemo(() => {
    const groups: { category: string; actions: CommandAction[] }[] = [];
    const indexMap = new Map<string, number>();

    for (const action of filteredActions) {
      let idx = indexMap.get(action.category);
      if (idx === undefined) {
        idx = groups.length;
        indexMap.set(action.category, idx);
        groups.push({ category: action.category, actions: [] });
      }
      groups[idx].actions.push(action);
    }

    return groups;
  }, [filteredActions]);

  // Reset state when opening
  useEffect(() => {
    if (isOpen) {
      setQuery('');
      setSelectedIndex(0);
      // Focus input after animation frame
      requestAnimationFrame(() => {
        inputRef.current?.focus();
      });
    }
  }, [isOpen]);

  // Clamp selected index when results change
  useEffect(() => {
    if (selectedIndex >= filteredActions.length) {
      setSelectedIndex(Math.max(0, filteredActions.length - 1));
    }
  }, [filteredActions.length, selectedIndex]);

  // Scroll selected item into view
  useEffect(() => {
    if (!listRef.current) return;
    const selectedEl = listRef.current.querySelector(
      `[data-command-index="${selectedIndex}"]`
    );
    if (selectedEl) {
      selectedEl.scrollIntoView({ block: 'nearest' });
    }
  }, [selectedIndex]);

  const executeAction = useCallback(
    (action: CommandAction) => {
      onClose();
      // Delay action to allow close animation
      requestAnimationFrame(() => {
        action.action();
      });
    },
    [onClose]
  );

  const handleKeyDown = useCallback(
    (e: React.KeyboardEvent) => {
      switch (e.key) {
        case 'ArrowDown':
          e.preventDefault();
          setSelectedIndex((prev) =>
            prev < filteredActions.length - 1 ? prev + 1 : 0
          );
          break;
        case 'ArrowUp':
          e.preventDefault();
          setSelectedIndex((prev) =>
            prev > 0 ? prev - 1 : filteredActions.length - 1
          );
          break;
        case 'Enter':
          e.preventDefault();
          if (filteredActions[selectedIndex]) {
            executeAction(filteredActions[selectedIndex]);
          }
          break;
        case 'Escape':
          e.preventDefault();
          onClose();
          break;
        case 'Tab':
          // Trap focus inside the palette — keep focus on the input
          e.preventDefault();
          inputRef.current?.focus();
          break;
      }
    },
    [filteredActions, selectedIndex, executeAction, onClose]
  );

  if (!isOpen) return null;

  // Build a flat index counter for keyboard navigation across groups
  let flatIndex = 0;

  return (
    <div
      ref={overlayRef}
      className="fixed inset-0 z-[60] flex items-start justify-center pt-[15vh]"
      onClick={(e) => {
        if (e.target === overlayRef.current) onClose();
      }}
    >
      {/* Backdrop */}
      <div className="absolute inset-0 bg-black/40 backdrop-blur-sm animate-cmdPaletteFadeIn" />

      {/* Palette modal */}
      <div
        role="dialog"
        aria-modal="true"
        aria-label="Command palette"
        className="relative w-full max-w-lg mx-4 glass-panel glass-raised rounded-2xl
          overflow-hidden animate-cmdPaletteScaleIn"
        onKeyDown={handleKeyDown}
      >
        {/* Search input */}
        <div className="flex items-center gap-3 px-4 py-3 border-b border-border">
          {/* Magnifying glass icon */}
          <svg
            width="18"
            height="18"
            viewBox="0 0 20 20"
            fill="none"
            className="text-text-muted shrink-0"
          >
            <circle
              cx="9"
              cy="9"
              r="6"
              stroke="currentColor"
              strokeWidth="1.5"
            />
            <path
              d="M13.5 13.5L17 17"
              stroke="currentColor"
              strokeWidth="1.5"
              strokeLinecap="round"
            />
          </svg>
          <input
            ref={inputRef}
            type="text"
            value={query}
            onChange={(e) => {
              setQuery(e.target.value);
              setSelectedIndex(0);
            }}
            placeholder="Search commands..."
            aria-label="Search commands"
            className="flex-1 bg-transparent text-sm text-text-primary placeholder:text-text-muted
              outline-none border-none"
            spellCheck={false}
            autoComplete="off"
          />
          <kbd className="hidden sm:flex items-center px-1.5 py-0.5 rounded-md bg-surface-alt
            border border-border text-[10px] font-mono text-text-muted">
            ESC
          </kbd>
        </div>

        {/* Results */}
        <div
          ref={listRef}
          role="listbox"
          aria-label="Command results"
          className="max-h-[360px] overflow-y-auto scrollbar-thin py-2"
        >
          {filteredActions.length === 0 ? (
            <div className="px-4 py-8 text-center">
              <p className="text-sm text-text-muted">No commands found</p>
              <p className="text-xs text-text-muted mt-1">
                Try a different search term
              </p>
            </div>
          ) : (
            groupedActions.map((group) => (
              <div key={group.category}>
                {/* Category header */}
                <div className="px-4 pt-2 pb-1">
                  <span className="text-[10px] font-semibold text-text-muted uppercase tracking-wider">
                    {group.category}
                  </span>
                </div>

                {/* Actions in this category */}
                {group.actions.map((action) => {
                  const currentIndex = flatIndex++;
                  const isSelected = currentIndex === selectedIndex;

                  return (
                    <button
                      key={action.id}
                      role="option"
                      aria-selected={isSelected}
                      data-command-index={currentIndex}
                      onClick={() => executeAction(action)}
                      onMouseEnter={() => setSelectedIndex(currentIndex)}
                      className={`w-full flex items-center gap-3 px-4 py-2.5 text-left transition-colors
                        ${
                          isSelected
                            ? 'bg-accent-teal/10 text-text-primary'
                            : 'text-text-secondary hover:bg-surface-alt'
                        }`}
                    >
                      {/* Icon */}
                      {action.icon && (
                        <span className="text-base shrink-0 w-6 text-center">
                          {action.icon}
                        </span>
                      )}

                      {/* Label + Description */}
                      <div className="flex-1 min-w-0">
                        <div className="text-sm font-medium truncate">
                          {action.label}
                        </div>
                        {action.description && (
                          <div className="text-xs text-text-muted truncate mt-0.5">
                            {action.description}
                          </div>
                        )}
                      </div>

                      {/* Shortcut badge */}
                      {action.shortcut && (
                        <kbd
                          className={`shrink-0 px-1.5 py-0.5 rounded-md border text-[10px] font-mono
                            min-w-[22px] text-center
                            ${
                              isSelected
                                ? 'bg-accent-teal/20 border-accent-teal/30 text-accent-teal'
                                : 'bg-surface-alt border-border text-text-muted'
                            }`}
                        >
                          {action.shortcut}
                        </kbd>
                      )}
                    </button>
                  );
                })}
              </div>
            ))
          )}
        </div>

        {/* Footer hint */}
        <div className="flex items-center gap-4 px-4 py-2.5 border-t border-border bg-surface-alt/50">
          <div className="flex items-center gap-1.5 text-[10px] text-text-muted">
            <kbd className="px-1 py-0.5 rounded bg-surface-alt border border-border font-mono">
              &uarr;
            </kbd>
            <kbd className="px-1 py-0.5 rounded bg-surface-alt border border-border font-mono">
              &darr;
            </kbd>
            <span>navigate</span>
          </div>
          <div className="flex items-center gap-1.5 text-[10px] text-text-muted">
            <kbd className="px-1.5 py-0.5 rounded bg-surface-alt border border-border font-mono">
              &crarr;
            </kbd>
            <span>select</span>
          </div>
          <div className="flex items-center gap-1.5 text-[10px] text-text-muted">
            <kbd className="px-1.5 py-0.5 rounded bg-surface-alt border border-border font-mono">
              esc
            </kbd>
            <span>close</span>
          </div>
          <div className="ml-auto text-[10px] text-text-muted">
            {filteredActions.length} command{filteredActions.length !== 1 ? 's' : ''}
          </div>
        </div>
      </div>

    </div>
  );
}
