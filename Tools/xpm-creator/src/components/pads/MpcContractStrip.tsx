'use client';

import React, { useMemo, useState, useRef, useCallback, useEffect, useId } from 'react';
import { createPortal } from 'react-dom';
import { usePadStore } from '@/stores/padStore';
import type { PadAssignment } from '@/types';

// ---------------------------------------------------------------------------
// Rule status types
// ---------------------------------------------------------------------------

export type RuleStatusLevel = 'pass' | 'warn' | 'fail';

export interface RuleStatus {
  status: RuleStatusLevel;
  /** Number of layers that comply with this rule */
  compliant: number;
  /** Total active (non-empty) layers checked */
  total: number;
  issues: string[];
}

// ---------------------------------------------------------------------------
// Rule checkers
// ---------------------------------------------------------------------------

/**
 * KEYTRACK rule.
 *
 * Drum programs: every active layer must have keyTrack=false.
 * Keygroup programs: every active layer must have keyTrack=true.
 *
 * "Active" means the layer has a sampleId assigned.
 */
function checkKeyTrack(
  pads: PadAssignment[],
  programType: 'drum' | 'keygroup',
): RuleStatus {
  const expectedValue = programType === 'keygroup';
  let compliant = 0;
  let total = 0;
  const issues: string[] = [];

  for (const pad of pads) {
    for (const layer of pad.layers) {
      if (!layer.active || !layer.sampleId) continue;
      total++;
      if (layer.keyTrack === expectedValue) {
        compliant++;
      } else {
        issues.push(
          `Pad ${pad.padNumber + 1} L${layer.number}: keyTrack=${String(layer.keyTrack)} (expected ${String(expectedValue)})`,
        );
      }
    }
  }

  if (total === 0) {
    // No active layers yet — nothing to validate, treat as pass
    return { status: 'pass', compliant: 0, total: 0, issues: [] };
  }

  const status: RuleStatusLevel =
    compliant === total ? 'pass' : issues.length > 0 ? 'fail' : 'pass';

  return { status, compliant, total, issues };
}

/**
 * ROOT NOTE rule.
 *
 * All active layers must have rootNote=0 (MPC auto-detect convention).
 * Empty (inactive) layers are exempt.
 */
function checkRootNote(pads: PadAssignment[]): RuleStatus {
  let compliant = 0;
  let total = 0;
  const issues: string[] = [];

  for (const pad of pads) {
    for (const layer of pad.layers) {
      if (!layer.active || !layer.sampleId) continue;
      total++;
      if (layer.rootNote === 0) {
        compliant++;
      } else {
        issues.push(
          `Pad ${pad.padNumber + 1} L${layer.number}: rootNote=${layer.rootNote} (must be 0)`,
        );
      }
    }
  }

  if (total === 0) {
    return { status: 'pass', compliant: 0, total: 0, issues: [] };
  }

  const status: RuleStatusLevel = compliant === total ? 'pass' : 'fail';
  return { status, compliant, total, issues };
}

/**
 * VEL ZONES rule.
 *
 * Two checks:
 * 1. Empty (inactive) layers must have velStart=0 — prevents ghost triggering.
 * 2. Active layers within a pad must not have overlapping velocity zones.
 */
function checkVelZones(pads: PadAssignment[]): RuleStatus {
  let compliant = 0;
  let total = 0;
  const issues: string[] = [];

  for (const pad of pads) {
    // Check 1: empty layers must have velStart=0
    for (const layer of pad.layers) {
      if (!layer.active || !layer.sampleId) {
        total++;
        if (layer.velStart === 0) {
          compliant++;
        } else {
          issues.push(
            `Pad ${pad.padNumber + 1} L${layer.number}: empty layer velStart=${layer.velStart} (must be 0)`,
          );
        }
      }
    }

    // Check 2: active layers must have non-overlapping velocity zones
    const activeLayers = pad.layers.filter((l) => l.active && l.sampleId);
    if (activeLayers.length <= 1) continue;

    // Sort by velStart for overlap detection
    const sorted = [...activeLayers].sort((a, b) => a.velStart - b.velStart);
    for (let i = 0; i < sorted.length - 1; i++) {
      const current = sorted[i];
      const next = sorted[i + 1];
      total++;
      if (current.velEnd < next.velStart) {
        compliant++;
      } else if (current.velEnd >= next.velStart) {
        // Overlapping or adjacent zones
        if (current.velEnd >= next.velStart) {
          issues.push(
            `Pad ${pad.padNumber + 1}: velocity overlap — L${current.number} ends at ${current.velEnd}, L${next.number} starts at ${next.velStart}`,
          );
        } else {
          compliant++;
        }
      }
    }
  }

  if (total === 0) {
    return { status: 'pass', compliant: 0, total: 0, issues: [] };
  }

  let status: RuleStatusLevel;
  if (compliant === total) {
    status = 'pass';
  } else if (issues.some((i) => i.includes('empty layer'))) {
    // Empty-layer ghost-trigger issues are fails (hard export risk)
    status = 'fail';
  } else {
    // Velocity overlap is also a fail — MPC behavior is undefined
    status = 'fail';
  }

  return { status, compliant, total, issues };
}

// ---------------------------------------------------------------------------
// Public hook — used by XpmExporter to block export
// ---------------------------------------------------------------------------

export function useContractStatus(programType: 'drum' | 'keygroup' = 'drum'): {
  keyTrack: RuleStatus;
  rootNote: RuleStatus;
  velZones: RuleStatus;
} {
  const pads = usePadStore((s) => s.pads);

  const keyTrack = useMemo(() => checkKeyTrack(pads, programType), [pads, programType]);
  const rootNote = useMemo(() => checkRootNote(pads), [pads]);
  const velZones = useMemo(() => checkVelZones(pads), [pads]);

  return { keyTrack, rootNote, velZones };
}

export function useExportBlocked(programType: 'drum' | 'keygroup' = 'drum'): {
  blocked: boolean;
  reasons: string[];
} {
  const { keyTrack, rootNote, velZones } = useContractStatus(programType);

  return useMemo(() => {
    const reasons: string[] = [];
    if (keyTrack.status === 'fail') {
      reasons.push(`KeyTrack: ${keyTrack.issues.length} violation(s)`);
    }
    if (rootNote.status === 'fail') {
      reasons.push(`Root Note: ${rootNote.issues.length} layer(s) not set to 0`);
    }
    if (velZones.status === 'fail') {
      reasons.push(`Vel Zones: ${velZones.issues.length} issue(s)`);
    }
    return { blocked: reasons.length > 0, reasons };
  }, [keyTrack, rootNote, velZones]);
}

// ---------------------------------------------------------------------------
// Segment tooltip — inline portal, no dependency on shared Tooltip component
// (keeping MpcContractStrip self-contained)
// ---------------------------------------------------------------------------

interface SegmentTooltipProps {
  content: React.ReactNode;
  children: React.ReactNode;
}

function SegmentTooltip({ content, children }: SegmentTooltipProps) {
  const [visible, setVisible] = useState(false);
  const [coords, setCoords] = useState({ top: 0, left: 0 });
  const [mounted, setMounted] = useState(false);
  const triggerRef = useRef<HTMLDivElement>(null);
  const tooltipRef = useRef<HTMLDivElement>(null);
  const timerRef = useRef<ReturnType<typeof setTimeout> | null>(null);
  const tooltipId = useId();

  useEffect(() => {
    setMounted(true);
    return () => setMounted(false);
  }, []);

  const show = useCallback(() => {
    timerRef.current = setTimeout(() => setVisible(true), 300);
  }, []);

  const hide = useCallback(() => {
    if (timerRef.current) clearTimeout(timerRef.current);
    setVisible(false);
  }, []);

  useEffect(() => {
    if (!visible || !triggerRef.current || !tooltipRef.current) return;
    const tr = triggerRef.current.getBoundingClientRect();
    const tt = tooltipRef.current.getBoundingClientRect();
    let top = tr.top - tt.height - 10;
    let left = tr.left + tr.width / 2 - tt.width / 2;
    left = Math.max(8, Math.min(left, window.innerWidth - tt.width - 8));
    top = Math.max(8, Math.min(top, window.innerHeight - tt.height - 8));
    setCoords({ top, left });
  }, [visible]);

  useEffect(() => () => { if (timerRef.current) clearTimeout(timerRef.current); }, []);

  return (
    <>
      <div
        ref={triggerRef}
        onMouseEnter={show}
        onMouseLeave={hide}
        onFocus={show}
        onBlur={hide}
        aria-describedby={visible ? tooltipId : undefined}
        className="flex-1 min-w-0"
      >
        {children}
      </div>
      {visible && mounted &&
        createPortal(
          <div
            ref={tooltipRef}
            id={tooltipId}
            role="tooltip"
            className="fixed z-[9999] pointer-events-none rounded-lg border border-border
              bg-surface/95 backdrop-blur-sm shadow-elevated px-3 py-2 text-[11px]"
            style={{ top: coords.top, left: coords.left, maxWidth: 320 }}
          >
            {content}
          </div>,
          document.body,
        )
      }
    </>
  );
}

// ---------------------------------------------------------------------------
// Individual segment
// ---------------------------------------------------------------------------

const STATUS_DOT: Record<RuleStatusLevel, string> = {
  pass: 'text-emerald-400',
  warn: 'text-amber-400',
  fail: 'text-red-400',
};

const STATUS_BG: Record<RuleStatusLevel, string> = {
  pass: 'border-emerald-400/20 bg-emerald-400/5',
  warn: 'border-amber-400/20 bg-amber-400/5',
  fail: 'border-red-400/20 bg-red-500/10',
};

const STATUS_LABEL: Record<RuleStatusLevel, string> = {
  pass: 'PASS',
  warn: 'WARN',
  fail: 'FAIL',
};

interface SegmentProps {
  label: string;
  rule: string;
  status: RuleStatus;
}

function Segment({ label, rule, status }: SegmentProps) {
  const dotColor = STATUS_DOT[status.status];
  const bg = STATUS_BG[status.status];

  const tooltipContent = (
    <div className="space-y-1.5">
      <div className="flex items-center gap-1.5">
        <span className={`text-[10px] font-bold ${dotColor}`}>{STATUS_LABEL[status.status]}</span>
        <span className="text-text-primary font-medium">{label}</span>
      </div>
      <p className="text-text-secondary text-[10px]">{rule}</p>
      {status.total > 0 && (
        <p className="text-text-muted text-[10px]">
          {status.compliant}/{status.total} checks passed
        </p>
      )}
      {status.issues.length > 0 && (
        <ul className="mt-1 space-y-0.5">
          {status.issues.slice(0, 6).map((issue, i) => (
            <li key={i} className="text-red-400 text-[10px] font-mono leading-snug">
              {issue}
            </li>
          ))}
          {status.issues.length > 6 && (
            <li className="text-text-muted text-[10px]">
              +{status.issues.length - 6} more…
            </li>
          )}
        </ul>
      )}
    </div>
  );

  return (
    <SegmentTooltip content={tooltipContent}>
      <div
        className={`flex items-center gap-1.5 px-2.5 py-1.5 rounded border transition-colors
          cursor-default select-none ${bg}`}
        aria-label={`${label}: ${STATUS_LABEL[status.status]}`}
      >
        {/* Status dot */}
        <span className={`text-[16px] leading-none ${dotColor}`} aria-hidden>
          ●
        </span>
        <div className="min-w-0">
          <div className="text-[9px] font-bold tracking-widest text-text-muted uppercase leading-none">
            {label}
          </div>
          <div className={`text-[10px] font-mono font-medium leading-snug ${dotColor}`}>
            {status.total === 0
              ? 'no layers'
              : `${status.compliant}/${status.total} \u2713`}
          </div>
        </div>
      </div>
    </SegmentTooltip>
  );
}

// ---------------------------------------------------------------------------
// Main strip component
// ---------------------------------------------------------------------------

interface MpcContractStripProps {
  programType?: 'drum' | 'keygroup';
}

export function MpcContractStrip({ programType = 'drum' }: MpcContractStripProps) {
  const { keyTrack, rootNote, velZones } = useContractStatus(programType);

  const overallStatus: RuleStatusLevel =
    keyTrack.status === 'fail' || rootNote.status === 'fail' || velZones.status === 'fail'
      ? 'fail'
      : keyTrack.status === 'warn' || rootNote.status === 'warn' || velZones.status === 'warn'
        ? 'warn'
        : 'pass';

  const headerDot = STATUS_DOT[overallStatus];

  return (
    <div
      className="rounded-lg border border-border bg-zinc-900/60 px-2 py-1.5"
      role="region"
      aria-label="MPC Contract compliance status"
    >
      {/* Header label */}
      <div className="flex items-center gap-1.5 mb-1.5">
        <span className={`text-[10px] leading-none ${headerDot}`} aria-hidden>
          ◆
        </span>
        <span className="text-[9px] font-bold tracking-widest text-text-muted uppercase">
          MPC Contract
        </span>
        {overallStatus === 'fail' && (
          <span className="ml-auto text-[9px] font-medium text-red-400">
            Export blocked
          </span>
        )}
        {overallStatus === 'pass' && (
          <span className="ml-auto text-[9px] font-medium text-emerald-400">
            Ready to export
          </span>
        )}
      </div>

      {/* Three rule segments */}
      <div className="flex gap-1">
        <Segment
          label="KEYTRACK"
          rule={
            programType === 'drum'
              ? 'Drum programs: all active layers must have KeyTrack = False'
              : 'Keygroup programs: all active layers must have KeyTrack = True'
          }
          status={keyTrack}
        />
        <Segment
          label="ROOT NOTE"
          rule="All active layers must have RootNote = 0 (MPC auto-detect)"
          status={rootNote}
        />
        <Segment
          label="VEL ZONES"
          rule="Empty layers: velStart must be 0. Active layers: no overlapping velocity ranges."
          status={velZones}
        />
      </div>
    </div>
  );
}

export default MpcContractStrip;
