'use client';

import React, { useState, useRef, useEffect, useCallback, useId } from 'react';
import { createPortal } from 'react-dom';

interface InfoPopoverProps {
  title: string;
  description: string;
  learnMore?: string;
}

interface PopoverCoords {
  top: number;
  left: number;
}

const GAP = 8;

function computePopoverPosition(
  triggerRect: DOMRect,
  popoverRect: DOMRect,
): PopoverCoords {
  const padding = 12;

  // Prefer positioning below the trigger, centered horizontally
  let top = triggerRect.bottom + GAP;
  let left = triggerRect.left + triggerRect.width / 2 - popoverRect.width / 2;

  // If it would overflow below the viewport, position above instead
  if (top + popoverRect.height > window.innerHeight - padding) {
    top = triggerRect.top - popoverRect.height - GAP;
  }

  // Clamp horizontally to viewport
  left = Math.max(padding, Math.min(left, window.innerWidth - popoverRect.width - padding));

  // Clamp vertically to viewport
  top = Math.max(padding, Math.min(top, window.innerHeight - popoverRect.height - padding));

  return { top, left };
}

export default function InfoPopover({ title, description, learnMore }: InfoPopoverProps) {
  const [open, setOpen] = useState(false);
  const [coords, setCoords] = useState<PopoverCoords>({ top: 0, left: 0 });
  const [mounted, setMounted] = useState(false);

  const triggerRef = useRef<HTMLButtonElement>(null);
  const popoverRef = useRef<HTMLDivElement>(null);
  const popoverId = useId();

  useEffect(() => {
    setMounted(true);
    return () => setMounted(false);
  }, []);

  // Recompute position when popover opens
  useEffect(() => {
    if (!open || !triggerRef.current || !popoverRef.current) return;

    const triggerRect = triggerRef.current.getBoundingClientRect();
    const popoverRect = popoverRef.current.getBoundingClientRect();
    setCoords(computePopoverPosition(triggerRect, popoverRect));
  }, [open]);

  // Close on click outside
  useEffect(() => {
    if (!open) return;

    const handleClickOutside = (e: MouseEvent) => {
      const target = e.target as Node;
      if (
        triggerRef.current &&
        !triggerRef.current.contains(target) &&
        popoverRef.current &&
        !popoverRef.current.contains(target)
      ) {
        setOpen(false);
      }
    };

    const handleEscape = (e: KeyboardEvent) => {
      if (e.key === 'Escape') {
        setOpen(false);
        triggerRef.current?.focus();
      }
    };

    // Use a rAF so the triggering click doesn't immediately close
    requestAnimationFrame(() => {
      document.addEventListener('mousedown', handleClickOutside);
      document.addEventListener('keydown', handleEscape);
    });

    return () => {
      document.removeEventListener('mousedown', handleClickOutside);
      document.removeEventListener('keydown', handleEscape);
    };
  }, [open]);

  const toggle = useCallback(() => {
    setOpen((prev) => !prev);
  }, []);

  const popoverContent = open && mounted
    ? createPortal(
        <div
          ref={popoverRef}
          id={popoverId}
          role="dialog"
          aria-label={title}
          className="fixed z-[9998]"
          style={{
            top: coords.top,
            left: coords.left,
            maxWidth: 280,
            animation: 'infoPopoverFadeIn 150ms ease-out forwards',
          }}
        >
          <div
            className="rounded-lg border border-border bg-surface shadow-elevated p-3"
          >
            {/* Title */}
            <h4 className="text-xs font-semibold text-text-primary leading-tight">
              {title}
            </h4>

            {/* Description */}
            <p className="mt-1 text-[11px] text-text-secondary leading-relaxed">
              {description}
            </p>

            {/* Optional learn more link */}
            {learnMore && (
              <a
                href={learnMore}
                target="_blank"
                rel="noopener noreferrer"
                className="inline-flex items-center gap-1 mt-2 text-[11px] font-medium
                  text-accent-teal hover:text-accent-teal-dark transition-colors"
              >
                Learn more
                <svg
                  width="10"
                  height="10"
                  viewBox="0 0 10 10"
                  fill="none"
                  className="mt-px"
                >
                  <path
                    d="M3 7L7 3M7 3H3.5M7 3V6.5"
                    stroke="currentColor"
                    strokeWidth="1.25"
                    strokeLinecap="round"
                    strokeLinejoin="round"
                  />
                </svg>
              </a>
            )}
          </div>

          <style>{`
            @keyframes infoPopoverFadeIn {
              from {
                opacity: 0;
                transform: translateY(-4px);
              }
              to {
                opacity: 1;
                transform: translateY(0);
              }
            }
          `}</style>
        </div>,
        document.body,
      )
    : null;

  return (
    <>
      <button
        ref={triggerRef}
        type="button"
        onClick={toggle}
        aria-expanded={open}
        aria-controls={open ? popoverId : undefined}
        aria-label={`Info: ${title}`}
        className="inline-flex items-center justify-center w-[18px] h-[18px]
          rounded-full border border-border text-text-muted
          hover:text-accent-teal hover:border-accent-teal/40
          focus-visible:outline-none focus-visible:ring-2 focus-visible:ring-accent-teal/30
          transition-colors duration-150 flex-shrink-0"
      >
        <svg width="10" height="10" viewBox="0 0 10 10" fill="none">
          <text
            x="5"
            y="8"
            textAnchor="middle"
            fill="currentColor"
            fontSize="8"
            fontWeight="600"
            fontFamily="inherit"
          >
            ?
          </text>
        </svg>
      </button>
      {popoverContent}
    </>
  );
}
