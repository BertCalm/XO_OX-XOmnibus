'use client';

import React from 'react';

interface ProgressBarProps {
  progress: number;
  label?: string;
  detail?: string;
  size?: 'sm' | 'md' | 'lg';
  gradient?: boolean;
  className?: string;
}

const sizeClasses = {
  sm: 'h-1.5',
  md: 'h-2.5',
  lg: 'h-4',
};

export default function ProgressBar({
  progress,
  label,
  detail,
  size = 'md',
  gradient = true,
  className = '',
}: ProgressBarProps) {
  const clampedProgress = Math.min(100, Math.max(0, progress));

  return (
    <div className={`w-full ${className}`}>
      {(label || detail) && (
        <div className="flex items-center justify-between mb-1.5">
          {label && <span className="text-xs font-medium text-text-primary">{label}</span>}
          {detail && <span className="text-xs text-text-secondary">{detail}</span>}
        </div>
      )}
      <div
        className={`w-full bg-surface-alt rounded-full overflow-hidden ${sizeClasses[size]}`}
        role="progressbar"
        aria-valuenow={clampedProgress}
        aria-valuemin={0}
        aria-valuemax={100}
        aria-label={label || 'Progress'}
      >
        <div
          className={`h-full rounded-full transition-all duration-500 ease-out
            ${gradient ? 'progress-gradient' : 'bg-accent-teal'}`}
          style={{ width: `${clampedProgress}%` }}
        />
      </div>
    </div>
  );
}

export function StepProgress({
  steps,
}: {
  steps: { label: string; status: 'pending' | 'active' | 'complete' | 'error'; detail?: string }[];
}) {
  return (
    <div className="space-y-3">
      {steps.map((step, i) => (
        <div key={i} className="flex items-start gap-3">
          <div className="flex-shrink-0 mt-0.5">
            {step.status === 'complete' ? (
              <div className="w-5 h-5 rounded-full bg-accent-teal flex items-center justify-center">
                <svg width="12" height="12" viewBox="0 0 12 12" fill="none">
                  <path d="M3 6l2 2 4-4" stroke="white" strokeWidth="1.5" strokeLinecap="round" strokeLinejoin="round" />
                </svg>
              </div>
            ) : step.status === 'active' ? (
              <div className="w-5 h-5 rounded-full progress-gradient flex items-center justify-center">
                <div className="w-2 h-2 rounded-full bg-white animate-pulse" />
              </div>
            ) : step.status === 'error' ? (
              <div className="w-5 h-5 rounded-full bg-red-500 flex items-center justify-center">
                <svg width="12" height="12" viewBox="0 0 12 12" fill="none">
                  <path d="M4 4l4 4M8 4l-4 4" stroke="white" strokeWidth="1.5" strokeLinecap="round" />
                </svg>
              </div>
            ) : (
              <div className="w-5 h-5 rounded-full border-2 border-border" />
            )}
          </div>
          <div className="flex-1 min-w-0">
            <p className={`text-sm ${step.status === 'active' ? 'text-text-primary font-medium' : 'text-text-secondary'}`}>
              {step.label}
            </p>
            {step.detail && (
              <p className="text-xs text-text-muted mt-0.5 truncate">{step.detail}</p>
            )}
          </div>
        </div>
      ))}
    </div>
  );
}
