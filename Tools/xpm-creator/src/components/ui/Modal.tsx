'use client';

import React, { useEffect, useRef, useCallback } from 'react';

interface ModalProps {
  open: boolean;
  onClose: () => void;
  title?: string;
  children: React.ReactNode;
  size?: 'sm' | 'md' | 'lg' | 'xl';
}

const sizeClasses = {
  sm: 'max-w-sm',
  md: 'max-w-md',
  lg: 'max-w-lg',
  xl: 'max-w-2xl',
};

const FOCUSABLE_SELECTOR =
  'a[href], button:not([disabled]), textarea:not([disabled]), input:not([disabled]), select:not([disabled]), [tabindex]:not([tabindex="-1"])';

export default function Modal({ open, onClose, title, children, size = 'md' }: ModalProps) {
  const overlayRef = useRef<HTMLDivElement>(null);
  const dialogRef = useRef<HTMLDivElement>(null);
  const previouslyFocusedRef = useRef<HTMLElement | null>(null);
  const titleId = useRef(`modal-title-${Math.random().toString(36).slice(2, 8)}`).current;

  // Keep onClose in a ref so handleKeyDown never goes stale, and the
  // keyboard listener effect doesn't re-run on every parent render.
  const onCloseRef = useRef(onClose);
  onCloseRef.current = onClose;

  // Focus trap — keep Tab/Shift+Tab within modal
  const handleKeyDown = useCallback(
    (e: KeyboardEvent) => {
      if (e.key === 'Escape') {
        onCloseRef.current();
        return;
      }
      if (e.key !== 'Tab') return;

      const dialog = dialogRef.current;
      if (!dialog) return;

      const focusable = Array.from(dialog.querySelectorAll<HTMLElement>(FOCUSABLE_SELECTOR));
      if (focusable.length === 0) return;

      const first = focusable[0];
      const last = focusable[focusable.length - 1];

      if (e.shiftKey) {
        if (document.activeElement === first) {
          e.preventDefault();
          last.focus();
        }
      } else {
        if (document.activeElement === last) {
          e.preventDefault();
          first.focus();
        }
      }
    },
    [], // stable — reads onClose from ref
  );

  // Keyboard listener — stable callback means this only runs on open/close
  useEffect(() => {
    if (open) {
      document.addEventListener('keydown', handleKeyDown);
      document.body.style.overflow = 'hidden';
    }
    return () => {
      document.removeEventListener('keydown', handleKeyDown);
      document.body.style.overflow = '';
    };
  }, [open, handleKeyDown]);

  // Initial focus — runs ONCE when modal opens, doesn't re-run on state changes
  useEffect(() => {
    if (open) {
      previouslyFocusedRef.current = document.activeElement as HTMLElement;

      requestAnimationFrame(() => {
        const dialog = dialogRef.current;
        if (dialog) {
          const firstFocusable = dialog.querySelector<HTMLElement>(FOCUSABLE_SELECTOR);
          firstFocusable?.focus();
        }
      });
    } else {
      // Restore focus when modal closes
      if (previouslyFocusedRef.current) {
        previouslyFocusedRef.current.focus();
        previouslyFocusedRef.current = null;
      }
    }
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [open]);

  if (!open) return null;

  return (
    <div
      ref={overlayRef}
      className="fixed inset-0 z-50 flex items-center justify-center p-4
        bg-black/50 backdrop-blur-sm animate-in fade-in duration-200"
      onClick={(e) => {
        if (e.target === overlayRef.current) onClose();
      }}
    >
      <div
        ref={dialogRef}
        role="dialog"
        aria-modal="true"
        aria-labelledby={title ? titleId : undefined}
        className={`${sizeClasses[size]} w-full glass-panel glass-raised rounded-2xl
          p-6 animate-in zoom-in-95 duration-200`}
      >
        {title && (
          <div className="flex items-center justify-between mb-4">
            <h2 id={titleId} className="text-lg font-semibold text-text-primary">{title}</h2>
            <button
              onClick={onClose}
              aria-label="Close dialog"
              className="p-1 rounded-lg text-text-muted hover:text-text-primary
                hover:bg-surface-alt transition-colors"
            >
              <svg width="20" height="20" viewBox="0 0 20 20" fill="none">
                <path d="M6 6l8 8M14 6l-8 8" stroke="currentColor" strokeWidth="1.5" strokeLinecap="round" />
              </svg>
            </button>
          </div>
        )}
        {children}
      </div>
    </div>
  );
}
