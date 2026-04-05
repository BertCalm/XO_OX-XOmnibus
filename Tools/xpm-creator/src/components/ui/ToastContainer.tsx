'use client';

import React, { useEffect, useCallback, useState, useRef } from 'react';
import { createPortal } from 'react-dom';
import { useToastStore, type Toast, type ToastType } from '@/stores/toastStore';

/* ------------------------------------------------------------------ */
/* Icon components per toast type                                      */
/* ------------------------------------------------------------------ */

const iconsByType: Record<ToastType, React.ReactNode> = {
  success: (
    <svg width="18" height="18" viewBox="0 0 18 18" fill="none" className="flex-shrink-0">
      <circle cx="9" cy="9" r="8" stroke="currentColor" strokeWidth="1.5" className="text-green-500" />
      <path d="M5.5 9.5l2 2 5-5" stroke="currentColor" strokeWidth="1.5" strokeLinecap="round" strokeLinejoin="round" className="text-green-500" />
    </svg>
  ),
  error: (
    <svg width="18" height="18" viewBox="0 0 18 18" fill="none" className="flex-shrink-0">
      <circle cx="9" cy="9" r="8" stroke="currentColor" strokeWidth="1.5" className="text-red-500" />
      <path d="M6.5 6.5l5 5M11.5 6.5l-5 5" stroke="currentColor" strokeWidth="1.5" strokeLinecap="round" className="text-red-500" />
    </svg>
  ),
  warning: (
    <svg width="18" height="18" viewBox="0 0 18 18" fill="none" className="flex-shrink-0">
      <path d="M9 2l7.5 13H1.5L9 2z" stroke="currentColor" strokeWidth="1.5" strokeLinejoin="round" className="text-amber-500" />
      <path d="M9 7.5v3" stroke="currentColor" strokeWidth="1.5" strokeLinecap="round" className="text-amber-500" />
      <circle cx="9" cy="13" r="0.75" fill="currentColor" className="text-amber-500" />
    </svg>
  ),
  info: (
    <svg width="18" height="18" viewBox="0 0 18 18" fill="none" className="flex-shrink-0">
      <circle cx="9" cy="9" r="8" stroke="currentColor" strokeWidth="1.5" className="text-accent-teal" />
      <path d="M9 8v4" stroke="currentColor" strokeWidth="1.5" strokeLinecap="round" className="text-accent-teal" />
      <circle cx="9" cy="5.5" r="0.75" fill="currentColor" className="text-accent-teal" />
    </svg>
  ),
};

const borderColorByType: Record<ToastType, string> = {
  success: 'border-l-green-500',
  error: 'border-l-red-500',
  warning: 'border-l-amber-500',
  info: 'border-l-accent-teal',
};

/* ------------------------------------------------------------------ */
/* Individual Toast                                                    */
/* ------------------------------------------------------------------ */

function ToastItem({ toast }: { toast: Toast }) {
  const removeToast = useToastStore((s) => s.removeToast);
  const [exiting, setExiting] = useState(false);
  const timerRef = useRef<ReturnType<typeof setTimeout> | null>(null);

  const dismiss = useCallback(() => {
    setExiting(true);
    // Wait for exit animation before removing from store
    setTimeout(() => removeToast(toast.id), 200);
  }, [removeToast, toast.id]);

  useEffect(() => {
    if (toast.duration && toast.duration > 0) {
      timerRef.current = setTimeout(dismiss, toast.duration);
    }
    return () => {
      if (timerRef.current) clearTimeout(timerRef.current);
    };
  }, [toast.duration, dismiss]);

  // Pause auto-dismiss on hover
  const handleMouseEnter = () => {
    if (timerRef.current) {
      clearTimeout(timerRef.current);
      timerRef.current = null;
    }
  };

  const handleMouseLeave = () => {
    if (toast.duration && toast.duration > 0) {
      timerRef.current = setTimeout(dismiss, 2000);
    }
  };

  return (
    <div
      onMouseEnter={handleMouseEnter}
      onMouseLeave={handleMouseLeave}
      className={`
        max-w-[380px] w-full bg-surface rounded-lg border border-border border-l-4
        ${borderColorByType[toast.type]}
        shadow-elevated
        flex items-start gap-3 p-3
        transition-all duration-200 ease-out
        ${exiting
          ? 'opacity-0 translate-x-8'
          : 'opacity-100 translate-x-0 animate-toast-enter'
        }
      `}
      role={toast.type === 'error' || toast.type === 'warning' ? 'alert' : undefined}
      aria-live={toast.type === 'error' || toast.type === 'warning' ? 'assertive' : undefined}
    >
      {/* Icon */}
      <div className="mt-0.5">{iconsByType[toast.type]}</div>

      {/* Content */}
      <div className="flex-1 min-w-0">
        <p className="text-sm font-medium text-text-primary leading-tight">{toast.title}</p>
        {toast.message && (
          <p className="text-xs text-text-secondary mt-0.5 leading-relaxed">{toast.message}</p>
        )}
        {toast.action && (
          <button
            onClick={() => {
              toast.action!.onClick();
              dismiss();
            }}
            className="mt-1.5 text-xs font-medium text-accent-teal hover:text-accent-teal-dark transition-colors"
          >
            {toast.action.label}
          </button>
        )}
      </div>

      {/* Dismiss button */}
      {(toast.dismissible ?? true) && (
        <button
          onClick={dismiss}
          className="flex-shrink-0 p-0.5 rounded text-text-muted hover:text-text-primary transition-colors"
          aria-label="Dismiss notification"
        >
          <svg width="14" height="14" viewBox="0 0 14 14" fill="none">
            <path d="M4 4l6 6M10 4l-6 6" stroke="currentColor" strokeWidth="1.5" strokeLinecap="round" />
          </svg>
        </button>
      )}
    </div>
  );
}

/* ------------------------------------------------------------------ */
/* Toast Container (Portal)                                            */
/* ------------------------------------------------------------------ */

export default function ToastContainer() {
  const toasts = useToastStore((s) => s.toasts);
  const [mounted, setMounted] = useState(false);

  useEffect(() => {
    setMounted(true);
  }, []);

  if (!mounted) return null;

  return createPortal(
    <div
      className="fixed bottom-4 right-4 z-[9998] flex flex-col-reverse gap-2 pointer-events-none"
      role="status"
      aria-label="Notifications"
      aria-live="polite"
    >
      {toasts.map((toast) => (
        <div key={toast.id} className="pointer-events-auto">
          <ToastItem toast={toast} />
        </div>
      ))}
    </div>,
    document.body
  );
}
