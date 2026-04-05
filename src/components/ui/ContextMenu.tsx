'use client';

import React, { useEffect, useCallback, useRef, useState } from 'react';

export interface ContextMenuItem {
  id: string;
  label: string;
  icon?: React.ReactNode;
  shortcut?: string;
  danger?: boolean;
  disabled?: boolean;
  divider?: boolean;
  action: () => void;
}

interface ContextMenuProps {
  items: ContextMenuItem[];
  x: number;
  y: number;
  onClose: () => void;
}

export default function ContextMenu({ items, x, y, onClose }: ContextMenuProps) {
  const menuRef = useRef<HTMLDivElement>(null);
  const [position, setPosition] = useState({ x, y });

  // Viewport clamping — adjust position after mount so menu never overflows
  useEffect(() => {
    const el = menuRef.current;
    if (!el) return;
    const rect = el.getBoundingClientRect();
    const viewW = window.innerWidth;
    const viewH = window.innerHeight;
    let clampedX = x;
    let clampedY = y;
    if (x + rect.width > viewW - 8) clampedX = viewW - rect.width - 8;
    if (y + rect.height > viewH - 8) clampedY = viewH - rect.height - 8;
    if (clampedX < 8) clampedX = 8;
    if (clampedY < 8) clampedY = 8;
    setPosition({ x: clampedX, y: clampedY });
  }, [x, y]);

  // Close on outside click
  useEffect(() => {
    const handleClick = (e: MouseEvent) => {
      if (menuRef.current && !menuRef.current.contains(e.target as Node)) {
        onClose();
      }
    };
    document.addEventListener('mousedown', handleClick);
    return () => document.removeEventListener('mousedown', handleClick);
  }, [onClose]);

  // Track focused menu item index for keyboard navigation
  const [focusedIdx, setFocusedIdx] = useState(-1);
  const actionableItems = items.filter((i) => !i.divider && !i.disabled);

  // Auto-focus the menu container on mount
  useEffect(() => {
    requestAnimationFrame(() => menuRef.current?.focus());
  }, []);

  const handleKeyDown = useCallback(
    (e: React.KeyboardEvent) => {
      switch (e.key) {
        case 'Escape':
          e.preventDefault();
          onClose();
          break;
        case 'ArrowDown': {
          e.preventDefault();
          setFocusedIdx((prev) => {
            const next = prev < actionableItems.length - 1 ? prev + 1 : 0;
            return next;
          });
          break;
        }
        case 'ArrowUp': {
          e.preventDefault();
          setFocusedIdx((prev) => {
            const next = prev > 0 ? prev - 1 : actionableItems.length - 1;
            return next;
          });
          break;
        }
        case 'Enter':
        case ' ': {
          e.preventDefault();
          if (focusedIdx >= 0 && actionableItems[focusedIdx]) {
            actionableItems[focusedIdx].action();
            onClose();
          }
          break;
        }
      }
    },
    [onClose, focusedIdx, actionableItems]
  );

  // Map from actionable item index to original items array
  let actionIdx = 0;

  return (
    <div
      ref={menuRef}
      role="menu"
      tabIndex={-1}
      onKeyDown={handleKeyDown}
      aria-label="Context menu"
      className="fixed z-50 min-w-[160px] py-1 glass-panel glass-raised rounded-lg
        animate-cmdPaletteScaleIn focus:outline-none"
      style={{ left: position.x, top: position.y }}
    >
      {items.map((item) => {
        if (item.divider) {
          return <div key={item.id} className="my-1 border-t border-border" role="separator" />;
        }
        const isDisabled = !!item.disabled;
        const currentActionIdx = isDisabled ? -1 : actionIdx++;
        const isFocused = !isDisabled && currentActionIdx === focusedIdx;

        return (
          <button
            key={item.id}
            role="menuitem"
            tabIndex={-1}
            disabled={isDisabled}
            aria-disabled={isDisabled}
            onClick={() => {
              item.action();
              onClose();
            }}
            className={`w-full flex items-center gap-2 px-3 py-1.5 text-xs text-left transition-colors
              ${item.danger
                ? 'text-red-500 hover:bg-red-500/10'
                : 'text-text-primary hover:bg-surface-alt'
              }
              ${isDisabled ? 'opacity-40 pointer-events-none' : ''}
              ${isFocused ? 'bg-accent-teal/10 ring-1 ring-inset ring-accent-teal/30' : ''}
            `}
          >
            {item.icon && <span className="w-4 h-4 flex items-center justify-center flex-shrink-0">{item.icon}</span>}
            <span className="flex-1">{item.label}</span>
            {item.shortcut && (
              <span className="text-[10px] text-text-muted font-mono ml-2">{item.shortcut}</span>
            )}
          </button>
        );
      })}
    </div>
  );
}
