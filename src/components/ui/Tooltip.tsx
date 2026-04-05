'use client';

import React, { useState, useRef, useCallback, useEffect, useId } from 'react';
import { createPortal } from 'react-dom';

interface TooltipProps {
  children: React.ReactNode;
  content: string;
  description?: string;
  shortcut?: string;
  position?: 'top' | 'bottom' | 'left' | 'right';
  delay?: number;
  disabled?: boolean;
}

interface TooltipPosition {
  top: number;
  left: number;
}

const ARROW_SIZE = 5;
const GAP = 8;

function computePosition(
  triggerRect: DOMRect,
  tooltipRect: DOMRect,
  position: 'top' | 'bottom' | 'left' | 'right',
): TooltipPosition {
  let top = 0;
  let left = 0;

  switch (position) {
    case 'top':
      top = triggerRect.top - tooltipRect.height - GAP - ARROW_SIZE;
      left = triggerRect.left + triggerRect.width / 2 - tooltipRect.width / 2;
      break;
    case 'bottom':
      top = triggerRect.bottom + GAP + ARROW_SIZE;
      left = triggerRect.left + triggerRect.width / 2 - tooltipRect.width / 2;
      break;
    case 'left':
      top = triggerRect.top + triggerRect.height / 2 - tooltipRect.height / 2;
      left = triggerRect.left - tooltipRect.width - GAP - ARROW_SIZE;
      break;
    case 'right':
      top = triggerRect.top + triggerRect.height / 2 - tooltipRect.height / 2;
      left = triggerRect.right + GAP + ARROW_SIZE;
      break;
  }

  // Clamp to viewport edges with padding
  const padding = 8;
  left = Math.max(padding, Math.min(left, window.innerWidth - tooltipRect.width - padding));
  top = Math.max(padding, Math.min(top, window.innerHeight - tooltipRect.height - padding));

  return { top, left };
}

const arrowStyles: Record<string, React.CSSProperties> = {
  top: {
    bottom: -ARROW_SIZE,
    left: '50%',
    transform: 'translateX(-50%) rotate(45deg)',
  },
  bottom: {
    top: -ARROW_SIZE,
    left: '50%',
    transform: 'translateX(-50%) rotate(45deg)',
  },
  left: {
    right: -ARROW_SIZE,
    top: '50%',
    transform: 'translateY(-50%) rotate(45deg)',
  },
  right: {
    left: -ARROW_SIZE,
    top: '50%',
    transform: 'translateY(-50%) rotate(45deg)',
  },
};

const translateOrigins: Record<string, string> = {
  top: 'translateY(4px)',
  bottom: 'translateY(-4px)',
  left: 'translateX(4px)',
  right: 'translateX(-4px)',
};

export default function Tooltip({
  children,
  content,
  description,
  shortcut,
  position = 'top',
  delay = 400,
  disabled = false,
}: TooltipProps) {
  const [visible, setVisible] = useState(false);
  const [coords, setCoords] = useState<TooltipPosition>({ top: 0, left: 0 });
  const [mounted, setMounted] = useState(false);

  const triggerRef = useRef<HTMLDivElement>(null);
  const tooltipRef = useRef<HTMLDivElement>(null);
  const timeoutRef = useRef<ReturnType<typeof setTimeout> | null>(null);
  const tooltipId = useId();

  // Track if component is mounted (for portal target)
  useEffect(() => {
    setMounted(true);
    return () => setMounted(false);
  }, []);

  const show = useCallback(() => {
    if (disabled) return;
    timeoutRef.current = setTimeout(() => {
      setVisible(true);
    }, delay);
  }, [delay, disabled]);

  const hide = useCallback(() => {
    if (timeoutRef.current) {
      clearTimeout(timeoutRef.current);
      timeoutRef.current = null;
    }
    setVisible(false);
  }, []);

  // Recompute position when tooltip becomes visible
  useEffect(() => {
    if (!visible || !triggerRef.current || !tooltipRef.current) return;

    const triggerRect = triggerRef.current.getBoundingClientRect();
    const tooltipRect = tooltipRef.current.getBoundingClientRect();
    const pos = computePosition(triggerRect, tooltipRect, position);
    setCoords(pos);
  }, [visible, position]);

  // Clean up timeout on unmount
  useEffect(() => {
    return () => {
      if (timeoutRef.current) clearTimeout(timeoutRef.current);
    };
  }, []);

  const tooltipContent = visible && mounted
    ? createPortal(
        <div
          ref={tooltipRef}
          id={tooltipId}
          role="tooltip"
          className="fixed z-[9999] pointer-events-none"
          style={{
            top: coords.top,
            left: coords.left,
            maxWidth: 240,
            animation: 'tooltipFadeIn 150ms ease-out forwards',
          }}
        >
          {/* Tooltip body */}
          <div
            className="relative rounded-lg border border-border bg-surface/95 backdrop-blur-sm
              shadow-elevated px-2.5 py-1.5"
          >
            {/* Arrow */}
            <span
              className="absolute w-2.5 h-2.5 bg-surface/95 border border-border"
              style={{
                ...arrowStyles[position],
                // Hide the two border sides that face away from the tooltip body
                clipPath:
                  position === 'top'
                    ? 'polygon(0% 0%, 100% 0%, 100% 100%)'
                    : position === 'bottom'
                      ? 'polygon(0% 0%, 0% 100%, 100% 0%)'
                      : position === 'left'
                        ? 'polygon(0% 0%, 100% 0%, 0% 100%)'
                        : 'polygon(100% 0%, 0% 100%, 100% 100%)',
              }}
            />

            {/* Title row */}
            <div className="flex items-center gap-2">
              <span className="text-[11px] font-medium text-text-primary leading-tight">
                {content}
              </span>
              {shortcut && (
                <kbd
                  className="inline-flex items-center gap-0.5 rounded bg-surface-alt
                    px-1 py-0.5 font-mono text-[10px] text-text-muted leading-none
                    border border-border"
                >
                  {shortcut}
                </kbd>
              )}
            </div>

            {/* Optional description */}
            {description && (
              <p className="mt-0.5 text-[10px] text-text-secondary leading-relaxed">
                {description}
              </p>
            )}
          </div>

          {/* Inline keyframes scoped to this portal */}
          <style>{`
            @keyframes tooltipFadeIn {
              from {
                opacity: 0;
                transform: ${translateOrigins[position]};
              }
              to {
                opacity: 1;
                transform: translate(0, 0);
              }
            }
          `}</style>
        </div>,
        document.body,
      )
    : null;

  return (
    <>
      <div
        ref={triggerRef}
        onMouseEnter={show}
        onMouseLeave={hide}
        onFocus={show}
        onBlur={hide}
        aria-describedby={visible ? tooltipId : undefined}
        className="inline-flex"
      >
        {children}
      </div>
      {tooltipContent}
    </>
  );
}
