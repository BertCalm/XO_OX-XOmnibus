'use client';

import { useEffect, useCallback } from 'react';
import { usePadStore } from '@/stores/padStore';
import { usePlaybackStore } from '@/stores/playbackStore';
import { useHistoryStore } from '@/stores/historyStore';
import { useToastStore } from '@/stores/toastStore';

type SidebarSection = 'samples' | 'pads' | 'program' | 'export';

interface UseHotkeysOptions {
  /** Current active sidebar section */
  activeSection: SidebarSection;
  /** Callback to change the sidebar section */
  onSectionChange: (section: SidebarSection) => void;
  /** Callback to toggle the keyboard shortcuts overlay */
  onToggleShortcuts: () => void;
  /** Callback to toggle the command palette (Cmd+K) */
  onToggleCommandPalette?: () => void;
}

const SECTION_MAP: Record<string, SidebarSection> = {
  '1': 'samples',
  '2': 'pads',
  '3': 'program',
  '4': 'export',
};

/**
 * Global keyboard shortcuts for MPC-style workflow.
 *
 * Navigation:
 *   1-4:              Switch sidebar sections
 *   ?:                Show keyboard shortcuts overlay
 *
 * Pad Operations (when in Pads section):
 *   Arrow keys:       Navigate 4x4 pad grid spatially (wraps at edges)
 *   Shift+Up/Down:    Navigate between layers (0-7)
 *
 * Global:
 *   Space:            Trigger active pad (play/preview)
 *   Delete/Backspace: Clear current layer sample
 *   Cmd+Shift+C:     Copy pad
 *   Cmd+Shift+V:     Paste pad
 *   Escape:           Stop all playback
 *   Shift+D:          Duplicate active pad settings to next empty pad
 *   Cmd/Ctrl+Z:       Undo
 *   Cmd/Ctrl+Shift+Z: Redo
 *   Cmd/Ctrl+K:       Toggle command palette
 */
export function useHotkeys({
  activeSection,
  onSectionChange,
  onToggleShortcuts,
  onToggleCommandPalette,
}: UseHotkeysOptions) {
  const handleKeyDown = useCallback(
    (e: KeyboardEvent) => {
      // Don't intercept when typing in input fields
      const target = e.target as HTMLElement;
      if (
        target.tagName === 'INPUT' ||
        target.tagName === 'TEXTAREA' ||
        target.tagName === 'SELECT' ||
        target.isContentEditable
      ) {
        return;
      }

      const {
        activePadIndex,
        activeLayerIndex,
        setActivePad,
        setActiveLayer,
        pads,
        currentBank,
        updatePad,
        setPads,
        copyPad,
        pastePad,
        removeLayerSample,
      } = usePadStore.getState();

      // ? (Shift+/) : Toggle keyboard shortcuts overlay
      if (e.key === '?' && e.shiftKey) {
        e.preventDefault();
        onToggleShortcuts();
        return;
      }

      // Cmd/Ctrl+K: Toggle command palette
      if ((e.metaKey || e.ctrlKey) && !e.shiftKey && e.code === 'KeyK') {
        e.preventDefault();
        onToggleCommandPalette?.();
        return;
      }

      // Cmd/Ctrl+Shift+Z: Redo
      if ((e.metaKey || e.ctrlKey) && e.shiftKey && e.code === 'KeyZ') {
        e.preventDefault();
        const { redo } = useHistoryStore.getState();
        const restoredPads = redo(pads);
        if (restoredPads) {
          setPads(restoredPads);
          useToastStore.getState().addToast({ type: 'info', title: 'Redo applied' });
        } else {
          useToastStore.getState().addToast({ type: 'info', title: 'Nothing to redo' });
        }
        return;
      }

      // Cmd/Ctrl+Z: Undo
      if ((e.metaKey || e.ctrlKey) && !e.shiftKey && e.code === 'KeyZ') {
        e.preventDefault();
        const { undo } = useHistoryStore.getState();
        const restoredPads = undo(pads);
        if (restoredPads) {
          setPads(restoredPads);
          useToastStore.getState().addToast({ type: 'info', title: 'Undo applied' });
        } else {
          useToastStore.getState().addToast({ type: 'info', title: 'Nothing to undo' });
        }
        return;
      }

      // Cmd+Shift+C: Copy pad
      if (e.metaKey && e.shiftKey && e.code === 'KeyC') {
        e.preventDefault();
        copyPad(activePadIndex);
        useToastStore.getState().addToast({ type: 'info', title: `Pad ${(activePadIndex % 16) + 1} copied` });
        return;
      }

      // Cmd+Shift+V: Paste pad
      if (e.metaKey && e.shiftKey && e.code === 'KeyV') {
        e.preventDefault();
        const { copiedPad, withHistory } = usePadStore.getState();
        if (copiedPad) {
          withHistory('Paste pad', () => pastePad(activePadIndex));
          useToastStore.getState().addToast({ type: 'success', title: `Pasted to Pad ${(activePadIndex % 16) + 1}` });
        }
        return;
      }

      // Space: Trigger active pad
      if (e.code === 'Space') {
        e.preventDefault();
        const { triggerPad } = usePlaybackStore.getState();
        triggerPad(activePadIndex);
        return;
      }

      // Escape: Stop all playback (only if no modal/dialog is open)
      if (e.code === 'Escape') {
        // Don't stop playback if a dialog or command palette is handling Escape
        if (document.querySelector('[role="dialog"]')) return;
        const { stopAll } = usePlaybackStore.getState();
        stopAll();
        return;
      }

      // Delete/Backspace: Clear current layer sample (with toast feedback)
      if (e.code === 'Delete' || e.code === 'Backspace') {
        e.preventDefault();
        const pad = pads[activePadIndex];
        const layer = pad?.layers[activeLayerIndex];
        if (layer?.sampleId) {
          const { withHistory } = usePadStore.getState();
          withHistory('Clear layer', () => removeLayerSample(activePadIndex, activeLayerIndex));
          useToastStore.getState().addToast({
            type: 'info',
            title: `Layer ${activeLayerIndex + 1} cleared — Cmd+Z to undo`,
          });
        }
        return;
      }

      // No modifier keys held — section switching and pad/layer navigation
      if (!e.shiftKey && !e.ctrlKey && !e.metaKey && !e.altKey) {
        // 1-4: Switch sidebar sections
        const sectionKey = SECTION_MAP[e.key];
        if (sectionKey) {
          e.preventDefault();
          onSectionChange(sectionKey);
          return;
        }

        // Arrow keys for spatial 4x4 grid navigation (when in Pads section)
        // MPC layout: pad 0 = bottom-left, pad 15 = top-right
        // Row goes bottom→top (0-3 = bottom row, 12-15 = top row)
        if (activeSection === 'pads') {
          const bankOffset = currentBank * 16;
          const padWithinBank = activePadIndex - bankOffset;
          const row = Math.floor(padWithinBank / 4); // 0 = bottom, 3 = top
          const col = padWithinBank % 4;              // 0 = left, 3 = right

          if (e.code === 'ArrowUp') {
            e.preventDefault();
            // Move up one row in the grid (wraps top→bottom)
            const newRow = row < 3 ? row + 1 : 0;
            setActivePad(bankOffset + newRow * 4 + col);
            return;
          }
          if (e.code === 'ArrowDown') {
            e.preventDefault();
            // Move down one row in the grid (wraps bottom→top)
            const newRow = row > 0 ? row - 1 : 3;
            setActivePad(bankOffset + newRow * 4 + col);
            return;
          }
          if (e.code === 'ArrowLeft') {
            e.preventDefault();
            // Move left one column (wraps right→left)
            const newCol = col > 0 ? col - 1 : 3;
            setActivePad(bankOffset + row * 4 + newCol);
            return;
          }
          if (e.code === 'ArrowRight') {
            e.preventDefault();
            // Move right one column (wraps left→right)
            const newCol = col < 3 ? col + 1 : 0;
            setActivePad(bankOffset + row * 4 + newCol);
            return;
          }
        }
      }

      // Shift+Up/Down: Navigate between layers (0-7, wrapping)
      if (e.shiftKey && !e.ctrlKey && !e.metaKey && e.code === 'ArrowUp') {
        e.preventDefault();
        const newLayer = activeLayerIndex > 0 ? activeLayerIndex - 1 : 7;
        setActiveLayer(newLayer);
        return;
      }
      if (e.shiftKey && !e.ctrlKey && !e.metaKey && e.code === 'ArrowDown') {
        e.preventDefault();
        const newLayer = activeLayerIndex < 7 ? activeLayerIndex + 1 : 0;
        setActiveLayer(newLayer);
        return;
      }

      // Shift+D: Duplicate active pad settings to next empty pad
      if (e.shiftKey && !e.ctrlKey && !e.metaKey && e.code === 'KeyD') {
        e.preventDefault();
        const sourcePad = pads[activePadIndex];
        if (!sourcePad) return;

        // Find next pad without any loaded samples
        const bankOffset = currentBank * 16;
        for (let offset = 1; offset < 16; offset++) {
          const targetIdx =
            bankOffset + ((activePadIndex - bankOffset + offset) % 16);
          const targetPad = pads[targetIdx];
          if (
            targetPad &&
            !targetPad.layers.some((l) => l.active && l.sampleId)
          ) {
            // Copy play mode, mute group, and trigger mode
            const { withHistory } = usePadStore.getState();
            withHistory('Duplicate pad settings', () =>
              updatePad(targetIdx, {
                playMode: sourcePad.playMode,
                muteGroup: sourcePad.muteGroup,
                triggerMode: sourcePad.triggerMode,
              })
            );
            break;
          }
        }
        return;
      }
    },
    [activeSection, onSectionChange, onToggleShortcuts, onToggleCommandPalette]
  );

  useEffect(() => {
    window.addEventListener('keydown', handleKeyDown);
    return () => window.removeEventListener('keydown', handleKeyDown);
  }, [handleKeyDown]);
}
