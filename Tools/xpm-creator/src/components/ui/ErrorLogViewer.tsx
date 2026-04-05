'use client';

import React, { useState, useMemo, useCallback } from 'react';
import Modal from '@/components/ui/Modal';
import Button from '@/components/ui/Button';
import { useErrorStore, type ErrorEntry } from '@/stores/errorStore';
import { useToastStore } from '@/stores/toastStore';

/* ------------------------------------------------------------------ */
/* Helpers                                                             */
/* ------------------------------------------------------------------ */

const ALL_SECTIONS = 'All';

function formatRelativeTime(timestamp: number): string {
  const diff = Date.now() - timestamp;
  const seconds = Math.floor(diff / 1000);
  if (seconds < 60) return `${seconds}s ago`;
  const minutes = Math.floor(seconds / 60);
  if (minutes < 60) return `${minutes}m ago`;
  const hours = Math.floor(minutes / 60);
  if (hours < 24) return `${hours}h ago`;
  const days = Math.floor(hours / 24);
  return `${days}d ago`;
}

const sectionColors: Record<string, string> = {
  'The Crate': 'bg-blue-500/15 text-blue-400 border-blue-500/30',
  'The Forge': 'bg-purple-500/15 text-purple-400 border-purple-500/30',
  'Program Editor': 'bg-amber-500/15 text-amber-400 border-amber-500/30',
  'The Breath': 'bg-green-500/15 text-green-400 border-green-500/30',
  'Audio Engine': 'bg-red-500/15 text-red-400 border-red-500/30',
  'Export': 'bg-teal-500/15 text-teal-400 border-teal-500/30',
};

function getSectionBadgeClass(section: string): string {
  return sectionColors[section] || 'bg-gray-500/15 text-gray-400 border-gray-500/30';
}

/* ------------------------------------------------------------------ */
/* Error Row                                                           */
/* ------------------------------------------------------------------ */

function ErrorRow({
  entry,
  isEven,
}: {
  entry: ErrorEntry;
  isEven: boolean;
}) {
  const [expanded, setExpanded] = useState(false);
  const resolveError = useErrorStore((s) => s.resolveError);

  return (
    <div
      className={`
        border-b border-border last:border-b-0
        ${isEven ? 'bg-surface' : 'bg-surface-alt/50'}
        ${entry.resolved ? 'opacity-50' : ''}
      `}
    >
      {/* Main row */}
      <button
        onClick={() => setExpanded((prev) => !prev)}
        className="w-full text-left px-4 py-3 flex items-start gap-3 hover:bg-surface-alt/80 transition-colors"
      >
        {/* Expand chevron */}
        <svg
          width="14"
          height="14"
          viewBox="0 0 14 14"
          fill="none"
          className={`flex-shrink-0 mt-0.5 text-text-muted transition-transform duration-150 ${
            expanded ? 'rotate-90' : ''
          }`}
        >
          <path d="M5 3l4 4-4 4" stroke="currentColor" strokeWidth="1.5" strokeLinecap="round" strokeLinejoin="round" />
        </svg>

        {/* Timestamp */}
        <span className="flex-shrink-0 text-[11px] text-text-muted font-mono w-16 mt-0.5">
          {formatRelativeTime(entry.timestamp)}
        </span>

        {/* Section badge */}
        <span
          className={`flex-shrink-0 text-[10px] font-medium px-2 py-0.5 rounded-full border ${getSectionBadgeClass(
            entry.section
          )}`}
        >
          {entry.section}
        </span>

        {/* Error message */}
        <span className="flex-1 text-xs text-text-primary truncate">
          {entry.error}
        </span>

        {/* Resolved indicator */}
        {entry.resolved && (
          <span className="flex-shrink-0 text-[10px] text-green-500 font-medium">
            Resolved
          </span>
        )}
      </button>

      {/* Expanded details */}
      {expanded && (
        <div className="px-4 pb-3 pl-12 space-y-2">
          {/* Full error message */}
          <div>
            <p className="text-[10px] uppercase tracking-wider text-text-muted font-semibold mb-1">
              Error
            </p>
            <p className="text-xs text-text-primary font-mono bg-surface-alt rounded p-2 break-all">
              {entry.error}
            </p>
          </div>

          {/* Stack trace */}
          {entry.stack && (
            <div>
              <p className="text-[10px] uppercase tracking-wider text-text-muted font-semibold mb-1">
                Stack Trace
              </p>
              <pre className="text-[11px] text-text-secondary font-mono bg-surface-alt rounded p-2 overflow-x-auto max-h-40 scrollbar-thin whitespace-pre-wrap break-all">
                {entry.stack}
              </pre>
            </div>
          )}

          {/* Context */}
          {entry.context && Object.keys(entry.context).length > 0 && (
            <div>
              <p className="text-[10px] uppercase tracking-wider text-text-muted font-semibold mb-1">
                Context
              </p>
              <pre className="text-[11px] text-text-secondary font-mono bg-surface-alt rounded p-2 overflow-x-auto max-h-32 scrollbar-thin">
                {JSON.stringify(entry.context, null, 2)}
              </pre>
            </div>
          )}

          {/* Workaround */}
          {entry.workaround && (
            <div className="flex items-start gap-2 bg-amber-500/10 border border-amber-500/20 rounded-lg p-2">
              <svg width="14" height="14" viewBox="0 0 14 14" fill="none" className="flex-shrink-0 mt-0.5 text-amber-500">
                <path d="M7 1l6 11H1L7 1z" stroke="currentColor" strokeWidth="1.2" strokeLinejoin="round" />
                <path d="M7 5.5v2" stroke="currentColor" strokeWidth="1.2" strokeLinecap="round" />
                <circle cx="7" cy="9.5" r="0.5" fill="currentColor" />
              </svg>
              <p className="text-[11px] text-amber-400 leading-relaxed">
                <span className="font-medium">Suggestion:</span> {entry.workaround}
              </p>
            </div>
          )}

          {/* Actions */}
          {!entry.resolved && (
            <div className="flex justify-end pt-1">
              <Button
                variant="ghost"
                size="sm"
                onClick={(e) => {
                  e.stopPropagation();
                  resolveError(entry.id);
                }}
              >
                Mark Resolved
              </Button>
            </div>
          )}
        </div>
      )}
    </div>
  );
}

/* ------------------------------------------------------------------ */
/* Error Log Viewer                                                    */
/* ------------------------------------------------------------------ */

interface ErrorLogViewerProps {
  open: boolean;
  onClose: () => void;
}

export default function ErrorLogViewer({ open, onClose }: ErrorLogViewerProps) {
  const errors = useErrorStore((s) => s.errors);
  const clearResolved = useErrorStore((s) => s.clearResolved);
  const exportErrorLog = useErrorStore((s) => s.exportErrorLog);
  const addToast = useToastStore((s) => s.addToast);

  const [sectionFilter, setSectionFilter] = useState<string>(ALL_SECTIONS);

  // Derive unique sections from logged errors
  const sections = useMemo(() => {
    const unique = new Set(errors.map((e) => e.section));
    return [ALL_SECTIONS, ...Array.from(unique).sort()];
  }, [errors]);

  // Filtered & sorted errors (newest first)
  const filteredErrors = useMemo(() => {
    let filtered = errors;
    if (sectionFilter !== ALL_SECTIONS) {
      filtered = errors.filter((e) => e.section === sectionFilter);
    }
    return [...filtered].sort((a, b) => b.timestamp - a.timestamp);
  }, [errors, sectionFilter]);

  const unresolvedCount = useMemo(
    () => errors.filter((e) => !e.resolved).length,
    [errors]
  );

  const handleExport = useCallback(() => {
    const json = exportErrorLog();
    navigator.clipboard.writeText(json).then(
      () => {
        addToast({
          type: 'success',
          title: 'Error log copied',
          message: 'JSON error log has been copied to clipboard',
          duration: 3000,
        });
      },
      () => {
        // Fallback: open in new window
        const blob = new Blob([json], { type: 'application/json' });
        const url = URL.createObjectURL(blob);
        window.open(url, '_blank');
        // Delay revocation — browser needs time to load the blob URL in the new tab
        setTimeout(() => URL.revokeObjectURL(url), 10000);
        addToast({
          type: 'info',
          title: 'Error log exported',
          message: 'Opened in a new tab (clipboard was unavailable)',
          duration: 3000,
        });
      }
    );
  }, [exportErrorLog, addToast]);

  const handleClearResolved = useCallback(() => {
    clearResolved();
    addToast({
      type: 'info',
      title: 'Resolved errors cleared',
      duration: 2000,
    });
  }, [clearResolved, addToast]);

  const resolvedCount = errors.filter((e) => e.resolved).length;

  return (
    <Modal open={open} onClose={onClose} title="Error Log" size="xl">
      <div className="space-y-3">
        {/* Header bar with count badge and actions */}
        <div className="flex items-center justify-between">
          <div className="flex items-center gap-2">
            {/* Count badge */}
            {errors.length > 0 && (
              <span className="inline-flex items-center gap-1 text-xs font-medium px-2 py-0.5 rounded-full bg-red-500/15 text-red-400 border border-red-500/30">
                {unresolvedCount > 0 && (
                  <>
                    <span className="w-1.5 h-1.5 rounded-full bg-red-500 animate-pulse" />
                    {unresolvedCount} unresolved
                  </>
                )}
                {unresolvedCount === 0 && `${errors.length} total`}
              </span>
            )}

            {/* Section filter */}
            {sections.length > 2 && (
              <select
                value={sectionFilter}
                onChange={(e) => setSectionFilter(e.target.value)}
                className="input-field text-xs py-1 px-2 w-auto"
              >
                {sections.map((s) => (
                  <option key={s} value={s}>
                    {s}
                  </option>
                ))}
              </select>
            )}
          </div>

          <div className="flex items-center gap-2">
            {resolvedCount > 0 && (
              <Button variant="ghost" size="sm" onClick={handleClearResolved}>
                Clear Resolved ({resolvedCount})
              </Button>
            )}
            {errors.length > 0 && (
              <Button variant="secondary" size="sm" onClick={handleExport}>
                <svg width="14" height="14" viewBox="0 0 14 14" fill="none" className="mr-1">
                  <path d="M7 2v7M4 6l3 3 3-3M3 11h8" stroke="currentColor" strokeWidth="1.2" strokeLinecap="round" strokeLinejoin="round" />
                </svg>
                Export Log
              </Button>
            )}
          </div>
        </div>

        {/* Error list */}
        {filteredErrors.length === 0 ? (
          <div className="py-12 text-center">
            <div className="text-3xl mb-2">&#9981;</div>
            <p className="text-sm text-text-secondary">
              No errors recorded — smooth sailing!
            </p>
          </div>
        ) : (
          <div className="border border-border rounded-lg overflow-hidden max-h-[60vh] overflow-y-auto scrollbar-thin">
            {filteredErrors.map((entry, i) => (
              <ErrorRow key={entry.id} entry={entry} isEven={i % 2 === 0} />
            ))}
          </div>
        )}
      </div>
    </Modal>
  );
}
