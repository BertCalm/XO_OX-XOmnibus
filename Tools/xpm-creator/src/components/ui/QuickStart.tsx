'use client';

import React from 'react';

interface QuickStartProps {
  onImportSamples: () => void;
  onImportKit: () => void;
  onNewProject: () => void;
}

export default function QuickStart({ onImportSamples, onImportKit, onNewProject }: QuickStartProps) {
  return (
    <div className="flex flex-col items-center justify-center py-12 px-6 space-y-8">
      {/* XO_OX Logo/Brand */}
      <div className="text-center space-y-2">
        <h2 className="text-3xl font-bold tracking-tight font-mono text-text-primary">
          XO_OX
        </h2>
        <p className="text-sm text-text-secondary">
          World-class kits, made easily.
        </p>
      </div>

      {/* Quick Actions */}
      <div className="grid grid-cols-1 sm:grid-cols-3 gap-4 max-w-2xl w-full">
        {/* Start Fresh */}
        <button onClick={onNewProject} className="group p-6 rounded-xl border-2 border-dashed border-border hover:border-accent-teal/50 bg-surface transition-all text-center space-y-3">
          <div className="text-3xl">🎹</div>
          <div>
            <h3 className="text-sm font-semibold text-text-primary">Start Fresh</h3>
            <p className="text-[10px] text-text-muted mt-1">Create a new drum kit or keygroup from scratch</p>
          </div>
        </button>

        {/* Import Samples */}
        <button onClick={onImportSamples} className="group p-6 rounded-xl border-2 border-dashed border-border hover:border-accent-plum/50 bg-surface transition-all text-center space-y-3">
          <div className="text-3xl">📦</div>
          <div>
            <h3 className="text-sm font-semibold text-text-primary">Import Samples</h3>
            <p className="text-[10px] text-text-muted mt-1">Drop WAV files to start building</p>
          </div>
        </button>

        {/* Upcycle Kit */}
        <button onClick={onImportKit} className="group p-6 rounded-xl border-2 border-dashed border-border hover:border-accent-teal/50 bg-surface transition-all text-center space-y-3">
          <div className="text-3xl">♻️</div>
          <div>
            <h3 className="text-sm font-semibold text-text-primary">Upcycle a Kit</h3>
            <p className="text-[10px] text-text-muted mt-1">Import an existing XPM/XPN and enhance it</p>
          </div>
        </button>
      </div>

      {/* Feature Highlights */}
      <div className="max-w-lg text-center space-y-2">
        <p className="text-[10px] text-text-muted uppercase tracking-wider font-semibold">Powered by</p>
        <div className="flex flex-wrap justify-center gap-2">
          {[
            '10 Expression Modes',
            '8-Layer Cycle Engine',
            '20 Boutique Themes',
            'Auto-Choke Intelligence',
            'Spectral Air Injection',
            'Time Traveler Processing',
            'Smart Kit Analysis',
            'Velocity Curve Presets',
          ].map((feature) => (
            <span key={feature} className="px-2 py-0.5 rounded-full bg-surface-alt text-[9px] text-text-secondary font-medium">
              {feature}
            </span>
          ))}
        </div>
      </div>

      {/* Shortcut hint */}
      <p className="text-[9px] text-text-muted">
        Press <kbd className="px-1 py-0.5 rounded bg-surface-alt text-[8px] font-mono">⌘K</kbd> for quick actions
        {' · '}
        <kbd className="px-1 py-0.5 rounded bg-surface-alt text-[8px] font-mono">?</kbd> for shortcuts
      </p>
    </div>
  );
}
