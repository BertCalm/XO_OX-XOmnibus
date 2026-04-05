'use client';

import React, { useCallback, useRef } from 'react';
import { useLayoutStore, MIN_PANEL_WIDTH, MAX_PANEL_WIDTH } from '@/stores/layoutStore';

interface WorkspaceLayoutProps {
  sidebar: React.ReactNode;
  main: React.ReactNode;
  panel?: React.ReactNode;
}

export default function WorkspaceLayout({ sidebar, main, panel }: WorkspaceLayoutProps) {
  const panelWidth = useLayoutStore((s) => s.panelWidth);
  const setPanelWidth = useLayoutStore((s) => s.setPanelWidth);
  const resetLayout = useLayoutStore((s) => s.resetLayout);
  const isDragging = useRef(false);

  const handlePanelResizeStart = useCallback(
    (e: React.MouseEvent) => {
      e.preventDefault();
      isDragging.current = true;
      const startX = e.clientX;
      const startWidth = panelWidth;

      const onMouseMove = (moveEvent: MouseEvent) => {
        if (!isDragging.current) return;
        // Panel is on the right — dragging left increases width
        const delta = startX - moveEvent.clientX;
        setPanelWidth(startWidth + delta);
      };

      const onMouseUp = () => {
        isDragging.current = false;
        document.removeEventListener('mousemove', onMouseMove);
        document.removeEventListener('mouseup', onMouseUp);
        document.body.style.cursor = '';
        document.body.style.userSelect = '';
      };

      document.addEventListener('mousemove', onMouseMove);
      document.addEventListener('mouseup', onMouseUp);
      document.body.style.cursor = 'col-resize';
      document.body.style.userSelect = 'none';
    },
    [panelWidth, setPanelWidth]
  );

  const handleDoubleClick = useCallback(() => {
    resetLayout();
  }, [resetLayout]);

  return (
    <div className="flex h-[calc(100vh-3.5rem-1.75rem)] overflow-hidden">
      {sidebar}
      <main className="flex-1 overflow-auto scrollbar-thin">
        {main}
      </main>
      {panel && (
        <>
          {/* Resize handle */}
          <div
            className="w-1 cursor-col-resize bg-transparent hover:bg-accent-teal/30 active:bg-accent-teal/50
              transition-colors flex-shrink-0"
            onMouseDown={handlePanelResizeStart}
            onDoubleClick={handleDoubleClick}
            role="separator"
            aria-orientation="vertical"
            aria-label="Resize panel"
            title="Drag to resize, double-click to reset"
          />
          <aside
            className="border-l border-border glass-panel overflow-auto scrollbar-thin flex-shrink-0"
            style={{ width: panelWidth }}
          >
            {panel}
          </aside>
        </>
      )}
    </div>
  );
}
