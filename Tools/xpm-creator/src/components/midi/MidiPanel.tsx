'use client';

import React, { useState, useEffect, useCallback, useRef } from 'react';
import { useToastStore } from '@/stores/toastStore';
import {
  isMidiSupported,
  initMidi,
  getAvailableInputs,
  connectInput,
  disconnectInput,
  onNoteOn,
  onNoteOff,
  onDeviceChange,
  setMidiBank,
  getActiveInput,
  disposeMidi,
} from '@/lib/midi/midiInput';
import { usePlaybackStore } from '@/stores/playbackStore';
import { usePadStore } from '@/stores/padStore';
import Card, { CardHeader, CardTitle } from '@/components/ui/Card';

export default function MidiPanel() {
  const [supported, setSupported] = useState(false);
  const [initialized, setInitialized] = useState(false);
  const [inputs, setInputs] = useState<MIDIInput[]>([]);
  const [activeInputId, setActiveInputId] = useState<string | null>(null);
  const [lastNote, setLastNote] = useState<{ pad: number; vel: number } | null>(null);
  const [activityFlash, setActivityFlash] = useState(false);
  const flashTimeout = useRef<ReturnType<typeof setTimeout> | null>(null);

  const triggerPad = usePlaybackStore((s) => s.triggerPad);
  const stopPad = usePlaybackStore((s) => s.stopPad);
  const currentBank = usePadStore((s) => s.currentBank);

  // Check MIDI support on mount
  useEffect(() => {
    setSupported(isMidiSupported());
  }, []);

  // Sync bank changes to MIDI mapping
  useEffect(() => {
    setMidiBank(currentBank);
  }, [currentBank]);

  // Set up MIDI note handlers
  useEffect(() => {
    onNoteOn((padIndex: number, velocity: number) => {
      triggerPad(padIndex, velocity);
      setLastNote({ pad: padIndex, vel: velocity });

      // Flash activity indicator
      setActivityFlash(true);
      if (flashTimeout.current) clearTimeout(flashTimeout.current);
      flashTimeout.current = setTimeout(() => setActivityFlash(false), 150);
    });

    onNoteOff((padIndex: number) => {
      // For noteoff trigger mode pads, stop playback
      const pad = usePadStore.getState().pads[padIndex];
      if (pad && pad.triggerMode === 'noteoff') {
        stopPad(padIndex);
      }
    });

    return () => {
      if (flashTimeout.current) clearTimeout(flashTimeout.current);
      // Clear handlers to prevent stale closures triggering on unmounted component.
      // disposeMidi() in the separate cleanup effect also clears them, but this
      // guard covers the case where triggerPad/stopPad change before unmount.
      onNoteOn((() => {}) as (padIndex: number, velocity: number) => void);
      onNoteOff((() => {}) as (padIndex: number) => void);
    };
  }, [triggerPad, stopPad]);

  const handleInit = useCallback(async () => {
    try {
      const available = await initMidi();
      setInputs(available);
      setInitialized(true);

      onDeviceChange((newInputs: MIDIInput[]) => {
        setInputs(newInputs);
        // If active device was removed, update state
        const active = getActiveInput();
        if (active && !newInputs.find((i) => i.id === active.id)) {
          setActiveInputId(null);
        }
      });

      // Auto-connect first device if available
      if (available.length > 0) {
        connectInput(available[0]);
        setActiveInputId(available[0].id);
      }
    } catch (err) {
      console.error('MIDI init failed:', err);
      useToastStore.getState().addToast({
        type: 'error',
        title: 'MIDI unavailable',
        message: 'Check that your browser supports Web MIDI and that permission was granted.',
      });
    }
  }, []);

  const handleSelectDevice = useCallback((deviceId: string) => {
    const device = inputs.find((i) => i.id === deviceId);
    if (device) {
      connectInput(device);
      setActiveInputId(deviceId);
    }
  }, [inputs]);

  const handleDisconnect = useCallback(() => {
    disconnectInput();
    setActiveInputId(null);
  }, []);

  // Cleanup on unmount
  useEffect(() => {
    return () => {
      disposeMidi();
    };
  }, []);

  if (!supported) {
    return (
      <Card padding="sm">
        <div className="p-3 text-center space-y-2">
          <div className="text-lg">🎹</div>
          <p className="text-xs text-text-muted">
            MIDI not supported in this browser.
            Use Chrome or Edge for MIDI controller input.
          </p>
        </div>
      </Card>
    );
  }

  return (
    <Card padding="sm">
      <CardHeader>
        <div className="flex items-center gap-2">
          <CardTitle>MIDI Input</CardTitle>
          {activeInputId && (
            <div className={`w-2 h-2 rounded-full transition-colors ${activityFlash ? 'bg-accent-teal' : 'bg-green-500'}`} />
          )}
        </div>
      </CardHeader>

      <div className="p-3 space-y-3">
        {!initialized ? (
          <button
            onClick={handleInit}
            className="w-full px-3 py-2 rounded-lg text-xs font-medium
              bg-accent-teal text-white hover:bg-accent-teal-dark
              transition-colors flex items-center justify-center gap-2"
          >
            <span>🎹</span>
            <span>Enable MIDI Input</span>
          </button>
        ) : (
          <>
            {/* Device selector */}
            {inputs.length === 0 ? (
              <p className="text-xs text-text-muted text-center py-2">
                No MIDI devices detected. Connect a controller and try again.
              </p>
            ) : (
              <div className="space-y-2">
                <label className="text-[10px] text-text-muted uppercase tracking-wider">
                  Device
                </label>
                <select
                  value={activeInputId || ''}
                  onChange={(e) => {
                    if (e.target.value) {
                      handleSelectDevice(e.target.value);
                    } else {
                      handleDisconnect();
                    }
                  }}
                  className="w-full px-2 py-1.5 rounded-lg text-xs
                    bg-surface-alt text-text-primary border border-border
                    focus:border-accent-teal focus:outline-none"
                >
                  <option value="">Disconnected</option>
                  {inputs.map((input) => (
                    <option key={input.id} value={input.id}>
                      {input.name || `MIDI Device ${input.id}`}
                    </option>
                  ))}
                </select>
              </div>
            )}

            {/* Status & last note */}
            {activeInputId && (
              <div className="flex items-center justify-between text-[10px] text-text-muted">
                <span className="flex items-center gap-1">
                  <span className="w-1.5 h-1.5 rounded-full bg-green-500" />
                  Connected
                </span>
                {lastNote && (
                  <span>
                    Pad {lastNote.pad + 1} · vel {lastNote.vel}
                  </span>
                )}
              </div>
            )}

            {/* Refresh button */}
            <button
              onClick={async () => {
                try {
                  const available = getAvailableInputs();
                  setInputs(available);
                } catch (err) {
                  console.error('MIDI refresh failed:', err);
                  useToastStore.getState().addToast({
                    type: 'error',
                    title: 'MIDI refresh failed',
                    message: err instanceof Error ? err.message : 'Could not refresh MIDI devices.',
                  });
                }
              }}
              className="text-[10px] text-text-muted hover:text-text-primary
                transition-colors underline"
            >
              Refresh devices
            </button>
          </>
        )}
      </div>
    </Card>
  );
}
