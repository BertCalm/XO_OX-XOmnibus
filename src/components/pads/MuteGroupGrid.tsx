'use client';

import React, { useState, useCallback, useMemo } from 'react';
import { usePadStore } from '@/stores/padStore';

const GROUP_COLORS = [
  'bg-gray-500/20',       // 0 = no group
  'bg-red-500/40',        // 1
  'bg-blue-500/40',       // 2
  'bg-green-500/40',      // 3
  'bg-yellow-500/40',     // 4
  'bg-purple-500/40',     // 5
  'bg-pink-500/40',       // 6
  'bg-cyan-500/40',       // 7
  'bg-orange-500/40',     // 8
];

const GROUP_BORDER_COLORS = [
  'border-transparent',   // 0
  'border-red-500',       // 1
  'border-blue-500',      // 2
  'border-green-500',     // 3
  'border-yellow-500',    // 4
  'border-purple-500',    // 5
  'border-pink-500',      // 6
  'border-cyan-500',      // 7
  'border-orange-500',    // 8
];

const GROUP_LABELS = [
  'None', 'Group 1', 'Group 2', 'Group 3', 'Group 4',
  'Group 5', 'Group 6', 'Group 7', 'Group 8',
];

export default function MuteGroupGrid() {
  const pads = usePadStore((s) => s.pads);
  const currentBank = usePadStore((s) => s.currentBank);
  const updatePad = usePadStore((s) => s.updatePad);
  const [activeGroup, setActiveGroup] = useState(1);

  const bankOffset = currentBank * 16;
  const visiblePads = pads.slice(bankOffset, bankOffset + 16);

  // MPC layout: bottom-left to top-right
  const displayOrder = [
    [12, 13, 14, 15],
    [8, 9, 10, 11],
    [4, 5, 6, 7],
    [0, 1, 2, 3],
  ];

  const withHistory = usePadStore((s) => s.withHistory);

  const handlePadClick = useCallback(
    (localIdx: number) => {
      const globalIdx = bankOffset + localIdx;
      const pad = pads[globalIdx];
      if (!pad) return;

      // Toggle: if pad already has this group, remove it (set to 0)
      // Otherwise, assign the active group
      const newGroup = pad.muteGroup === activeGroup ? 0 : activeGroup;
      withHistory('Set mute group', () => updatePad(globalIdx, { muteGroup: newGroup }));
    },
    [bankOffset, pads, activeGroup, updatePad, withHistory]
  );

  // Count pads per group for the legend
  const groupCounts = useMemo(() => {
    const counts = new Map<number, number>();
    for (const pad of visiblePads) {
      if (pad.muteGroup > 0) {
        counts.set(pad.muteGroup, (counts.get(pad.muteGroup) || 0) + 1);
      }
    }
    return counts;
  }, [visiblePads]);

  return (
    <div className="space-y-3">
      {/* Group selector */}
      <div className="space-y-1.5">
        <p className="text-[10px] text-text-muted uppercase tracking-wider">
          Paint Mode — Click pads to assign
        </p>
        <div className="flex flex-wrap gap-1">
          {GROUP_LABELS.slice(1).map((label, i) => {
            const groupNum = i + 1;
            const count = groupCounts.get(groupNum) || 0;
            return (
              <button
                key={groupNum}
                onClick={() => setActiveGroup(groupNum)}
                aria-pressed={activeGroup === groupNum}
                aria-label={`Mute group ${groupNum}${count > 0 ? `, ${count} pad${count !== 1 ? 's' : ''} assigned` : ''}`}
                className={`px-2 py-1 rounded text-[10px] font-medium transition-all
                  border-2 ${
                    activeGroup === groupNum
                      ? `${GROUP_COLORS[groupNum]} ${GROUP_BORDER_COLORS[groupNum]} text-text-primary`
                      : 'border-transparent bg-surface-alt text-text-muted hover:text-text-secondary'
                  }`}
              >
                {groupNum}
                {count > 0 && (
                  <span className="ml-1 opacity-60">({count})</span>
                )}
              </button>
            );
          })}
        </div>
      </div>

      {/* 4x4 Grid */}
      <div className="grid grid-cols-4 gap-1.5">
        {displayOrder.flat().map((localIdx) => {
          const globalIdx = bankOffset + localIdx;
          const pad = visiblePads[localIdx] || pads[globalIdx];
          if (!pad) return null;

          const group = pad.muteGroup;
          const hasContent = pad.layers.some((l) => l.active && l.sampleId);
          const activeLayers = pad.layers.filter((l) => l.active && l.sampleId);
          const sampleName = activeLayers[0]?.sampleName || '';

          return (
            <button
              key={globalIdx}
              onClick={() => handlePadClick(localIdx)}
              aria-label={`Pad ${localIdx + 1}${sampleName ? `, ${sampleName}` : ''}${group > 0 ? `, mute group ${group}` : ', no mute group'}. Click to ${group === activeGroup ? 'remove from' : 'assign to'} group ${activeGroup}`}
              className={`relative aspect-square rounded-lg border-2 transition-all
                flex flex-col items-center justify-center gap-0.5 text-center
                ${group > 0
                  ? `${GROUP_COLORS[group]} ${GROUP_BORDER_COLORS[group]}`
                  : 'bg-surface-alt border-transparent hover:border-border'
                }
                ${hasContent ? 'cursor-pointer' : 'cursor-pointer opacity-50'}
              `}
            >
              {/* Pad number */}
              <span className="text-[10px] font-mono text-text-muted">
                {localIdx + 1}
              </span>

              {/* Sample name */}
              {hasContent && (
                <span className="text-[8px] text-text-secondary truncate max-w-full px-1">
                  {sampleName}
                </span>
              )}

              {/* Group badge */}
              {group > 0 && (
                <span className={`absolute top-0.5 right-0.5 w-3.5 h-3.5 rounded-full
                  flex items-center justify-center text-[8px] font-bold text-white
                  ${group === 1 ? 'bg-red-500' :
                    group === 2 ? 'bg-blue-500' :
                    group === 3 ? 'bg-green-500' :
                    group === 4 ? 'bg-yellow-500' :
                    group === 5 ? 'bg-purple-500' :
                    group === 6 ? 'bg-pink-500' :
                    group === 7 ? 'bg-cyan-500' :
                    'bg-orange-500'
                  }`}
                >
                  {group}
                </span>
              )}
            </button>
          );
        })}
      </div>

      {/* Usage hint */}
      <p className="text-[10px] text-text-muted text-center">
        Pads in the same mute group will choke each other (e.g., open/closed hi-hat)
      </p>
    </div>
  );
}
