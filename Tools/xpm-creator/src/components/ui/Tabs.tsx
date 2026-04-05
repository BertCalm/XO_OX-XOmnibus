'use client';

import React, { useCallback } from 'react';

interface Tab {
  id: string;
  label: string;
  icon?: React.ReactNode;
}

interface TabsProps {
  tabs: Tab[];
  activeTab: string;
  onChange: (tabId: string) => void;
  className?: string;
}

export default function Tabs({ tabs, activeTab, onChange, className = '' }: TabsProps) {
  const handleKeyDown = useCallback(
    (e: React.KeyboardEvent, currentIndex: number) => {
      let nextIndex: number | null = null;
      if (e.key === 'ArrowRight') {
        nextIndex = (currentIndex + 1) % tabs.length;
      } else if (e.key === 'ArrowLeft') {
        nextIndex = (currentIndex - 1 + tabs.length) % tabs.length;
      } else if (e.key === 'Home') {
        nextIndex = 0;
      } else if (e.key === 'End') {
        nextIndex = tabs.length - 1;
      }
      if (nextIndex !== null) {
        e.preventDefault();
        onChange(tabs[nextIndex].id);
        // Focus the newly activated tab button
        const tabList = e.currentTarget.parentElement;
        const nextButton = tabList?.children[nextIndex] as HTMLElement | undefined;
        nextButton?.focus();
      }
    },
    [tabs, onChange]
  );

  return (
    <div
      role="tablist"
      aria-label="View tabs"
      className={`flex items-center gap-1 p-1 bg-surface-alt rounded-lg ${className}`}
    >
      {tabs.map((tab, index) => (
        <button
          key={tab.id}
          role="tab"
          aria-selected={activeTab === tab.id}
          tabIndex={activeTab === tab.id ? 0 : -1}
          onClick={() => onChange(tab.id)}
          onKeyDown={(e) => handleKeyDown(e, index)}
          className={`flex items-center gap-1.5 px-3 py-1.5 rounded-md text-sm font-medium
            transition-all duration-150
            ${activeTab === tab.id
              ? 'bg-surface text-text-primary shadow-card'
              : 'text-text-secondary hover:text-text-primary'
            }`}
          style={activeTab === tab.id ? {
            borderBottom: '2px solid transparent',
            backgroundImage: 'linear-gradient(rgb(var(--color-surface)), rgb(var(--color-surface))), var(--gradient-brand)',
            backgroundOrigin: 'padding-box, border-box',
            backgroundClip: 'padding-box, border-box',
          } : undefined}
        >
          {tab.icon && <span className="flex-shrink-0">{tab.icon}</span>}
          {tab.label}
        </button>
      ))}
    </div>
  );
}
