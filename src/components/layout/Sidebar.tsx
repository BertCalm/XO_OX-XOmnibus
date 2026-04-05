'use client';

import React from 'react';

type SidebarSection = 'samples' | 'pads' | 'program' | 'export';

interface SidebarProps {
  activeSection: SidebarSection;
  onSectionChange: (section: SidebarSection) => void;
  sampleCount?: number;
}

const sections: { id: SidebarSection; label: string; metaphor: string; icon: React.ReactNode }[] = [
  {
    id: 'samples',
    label: 'The Crate',
    metaphor: 'Source your material',
    icon: (
      <svg width="20" height="20" viewBox="0 0 20 20" fill="none">
        <path d="M3 4h14M3 8h10M3 12h12M3 16h8" stroke="currentColor" strokeWidth="1.5" strokeLinecap="round" />
      </svg>
    ),
  },
  {
    id: 'pads',
    label: 'The Forge',
    metaphor: 'Shape your pads',
    icon: (
      <svg width="20" height="20" viewBox="0 0 20 20" fill="none">
        <rect x="3" y="3" width="5.5" height="5.5" rx="1.5" stroke="currentColor" strokeWidth="1.5" />
        <rect x="11.5" y="3" width="5.5" height="5.5" rx="1.5" stroke="currentColor" strokeWidth="1.5" />
        <rect x="3" y="11.5" width="5.5" height="5.5" rx="1.5" stroke="currentColor" strokeWidth="1.5" />
        <rect x="11.5" y="11.5" width="5.5" height="5.5" rx="1.5" stroke="currentColor" strokeWidth="1.5" />
      </svg>
    ),
  },
  {
    id: 'program',
    label: 'Program',
    metaphor: 'Configure your program',
    icon: (
      <svg width="20" height="20" viewBox="0 0 20 20" fill="none">
        <path d="M4 6h12M4 10h12M4 14h12" stroke="currentColor" strokeWidth="1.5" strokeLinecap="round" />
        <circle cx="7" cy="6" r="1.5" fill="currentColor" />
        <circle cx="13" cy="10" r="1.5" fill="currentColor" />
        <circle cx="9" cy="14" r="1.5" fill="currentColor" />
      </svg>
    ),
  },
  {
    id: 'export',
    label: 'Ship It',
    metaphor: 'Export your creation',
    icon: (
      <svg width="20" height="20" viewBox="0 0 20 20" fill="none">
        <path d="M10 3v10M6 9l4 4 4-4" stroke="currentColor" strokeWidth="1.5" strokeLinecap="round" strokeLinejoin="round" />
        <path d="M4 14v2a1 1 0 001 1h10a1 1 0 001-1v-2" stroke="currentColor" strokeWidth="1.5" strokeLinecap="round" />
      </svg>
    ),
  },
];

export default function Sidebar({ activeSection, onSectionChange, sampleCount = 0 }: SidebarProps) {
  return (
    <aside className="w-16 lg:w-56 glass-panel border-r border-border flex flex-col py-3">
      <nav className="flex-1 space-y-1 px-2">
        {sections.map((section) => (
          <button
            key={section.id}
            onClick={() => onSectionChange(section.id)}
            className={`w-full flex items-center gap-3 px-3 py-2.5 rounded-lg text-sm font-medium
              transition-all duration-150
              ${activeSection === section.id
                ? 'bg-accent-teal-50 text-accent-teal'
                : 'text-text-secondary hover:text-text-primary hover:bg-surface-alt'
              }`}
          >
            <span className="flex-shrink-0 lg:hidden" title={`${section.label} — ${section.metaphor}`}>
              {section.icon}
            </span>
            <span className="flex-shrink-0 hidden lg:block">{section.icon}</span>
            <span className="hidden lg:block">{section.label}</span>
            {section.id === 'samples' && sampleCount > 0 && (
              <span className="hidden lg:block ml-auto text-xs bg-accent-teal-light text-accent-teal
                px-1.5 py-0.5 rounded-full font-mono">
                {sampleCount}
              </span>
            )}
          </button>
        ))}
      </nav>
    </aside>
  );
}
