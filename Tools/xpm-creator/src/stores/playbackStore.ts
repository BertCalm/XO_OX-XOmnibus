import { create } from 'zustand';
import { getAudioContext, ensureAudioContextRunning } from '@/lib/audio/audioContext';
import { getCachedBuffer, getDecodedBuffer } from '@/lib/audio/audioBufferCache';
import { playPad, forceStopVoices, fadeStopVoices } from '@/lib/audio/padPlaybackEngine';
import type { PadVoiceHandle } from '@/lib/audio/padPlaybackEngine';
import { createExpressionChain } from '@/lib/audio/expressionEngine';
import type { ExpressionNodeChain } from '@/lib/audio/expressionEngine';
import { usePadStore } from './padStore';
import { useEnvelopeStore } from './envelopeStore';
import { useExpressionStore } from './expressionStore';
import { useAudioStore } from './audioStore';
import { useToastStore } from './toastStore';

interface PlaybackState {
  activeVoices: Record<number, PadVoiceHandle[]>;
  /** Expression chains per pad — must be cleaned up on ALL stop paths to prevent OscillatorNode CPU leaks */
  activeExpressionChains: Record<number, ExpressionNodeChain[]>;
  /** Record-based playing pads for reference-stable Zustand updates (avoids Set re-render issue) */
  playingPads: Record<number, boolean>;
  cycleCounters: Record<number, number>;
  lastRandomIndices: Record<number, number>;
  /** Generation counter per pad — incremented on each trigger to prevent stale onEnded callbacks from cleaning up new voices */
  _voiceGenerations: Record<number, number>;

  triggerPad: (padIndex: number, velocity?: number) => Promise<void>;
  stopPad: (padIndex: number) => void;
  stopAll: () => void;
}

/** Stop all OscillatorNodes and disconnect all nodes in expression chains */
function cleanupExpressionChains(chains: ExpressionNodeChain[]): void {
  for (const chain of chains) {
    for (const node of chain.nodes) {
      if (node instanceof OscillatorNode) {
        try { node.stop(); } catch { /* already stopped */ }
      }
      try { node.disconnect(); } catch { /* already disconnected */ }
    }
  }
}

export const usePlaybackStore = create<PlaybackState>((set, get) => ({
  activeVoices: {},
  activeExpressionChains: {},
  playingPads: {},
  cycleCounters: {},
  lastRandomIndices: {},
  _voiceGenerations: {},

  triggerPad: async (padIndex: number, velocity: number = 127) => {
    const state = get();

    // Increment generation counter — stale onEnded callbacks from previous
    // triggers will see a mismatched generation and become no-ops.
    const generation = (state._voiceGenerations[padIndex] ?? 0) + 1;
    set((s) => ({
      _voiceGenerations: { ...s._voiceGenerations, [padIndex]: generation },
    }));

    // Stop any currently playing voices on this pad + clean up expression chains
    const existing = state.activeVoices[padIndex];
    if (existing && existing.length > 0) {
      forceStopVoices(existing);
    }
    const existingChains = state.activeExpressionChains[padIndex];
    if (existingChains && existingChains.length > 0) {
      cleanupExpressionChains(existingChains);
    }

    // Read pad configuration
    const padState = usePadStore.getState();
    const pad = padState.pads[padIndex];
    if (!pad) return;

    // Check for active layers with samples
    const activeLayers = pad.layers.filter((l) => l.active && l.sampleId);
    if (activeLayers.length === 0) return;

    // Read envelope settings
    const envSettings = useEnvelopeStore.getState().getEnvelope(padIndex);

    let ctx: AudioContext;
    try {
      // Ensure audio context is running (handles browser autoplay policy)
      ctx = await ensureAudioContextRunning();
    } catch (error) {
      console.error('Failed to start audio context:', error);
      useToastStore.getState().addToast({
        type: 'error',
        title: 'Audio playback blocked',
        message: 'Click anywhere on the page first to enable audio (browser autoplay policy).',
      });
      return;
    }

    // Resolve sample buffers — try cache first, then async decode
    const audioStore = useAudioStore.getState();
    const sampleLookup = (sampleId: string): AudioBuffer | null => {
      return getCachedBuffer(sampleId);
    };

    // Pre-decode any uncached buffers
    try {
      for (const layer of activeLayers) {
        if (layer.sampleId && !getCachedBuffer(layer.sampleId)) {
          const sample = audioStore.samples.find((s) => s.id === layer.sampleId);
          if (sample) {
            await getDecodedBuffer(layer.sampleId, sample.buffer);
          }
        }
      }
    } catch (error) {
      console.error('Failed to decode sample buffer for pad', padIndex, error);
      useToastStore.getState().addToast({
        type: 'error',
        title: `Pad ${padIndex + 1}: sample decode failed`,
        message: 'The sample may be corrupt. Try re-importing it.',
      });
      return;
    }

    // Check for mute groups — use short fade to avoid clicks
    if (pad.muteGroup > 0) {
      const allPads = padState.pads;
      const currentVoices = get().activeVoices;
      for (let i = 0; i < allPads.length; i++) {
        if (i !== padIndex && allPads[i].muteGroup === pad.muteGroup && currentVoices[i]) {
          fadeStopVoices(currentVoices[i]);
          // Clean up expression chains for mute-grouped pads
          const mutedChains = get().activeExpressionChains[i];
          if (mutedChains) cleanupExpressionChains(mutedChains);
          set((s) => {
            const newVoices = { ...s.activeVoices };
            delete newVoices[i];
            const newPlaying = { ...s.playingPads };
            delete newPlaying[i];
            const newChains = { ...s.activeExpressionChains };
            delete newChains[i];
            return { activeVoices: newVoices, playingPads: newPlaying, activeExpressionChains: newChains };
          });
        }
      }
    }

    // Re-read cycle state AFTER awaits to prevent stale counter when
    // the same pad is triggered rapidly (double-tap race condition).
    // The original `state` snapshot from line 32 may be stale after the
    // async buffer decoding above.
    const freshState = get();
    const cycleCounter = freshState.cycleCounters[padIndex] ?? 0;
    const lastRandom = freshState.lastRandomIndices[padIndex] ?? -1;
    const { voices, nextCycleCounter, lastRandomIndex } = playPad(
      ctx,
      pad,
      envSettings,
      sampleLookup,
      velocity,
      cycleCounter,
      lastRandom
    );

    if (voices.length === 0) return;

    // Apply expression mode processing to each voice
    const expressionConfig = useExpressionStore.getState().getExpression(padIndex);
    const expressionChains: ExpressionNodeChain[] = [];
    if (expressionConfig.mode !== 'none') {
      for (const voice of voices) {
        const chain = createExpressionChain(ctx, velocity, expressionConfig);
        if (chain) {
          // Re-route: layerGain → expression chain → destination
          // (layerGain was connected to ctx.destination in playPadVoice)
          voice.layerGain.disconnect();
          voice.layerGain.connect(chain.input);
          chain.output.connect(ctx.destination);
          expressionChains.push(chain);
        }
      }
    }

    // Track completion — when all voices finish, mark pad as stopped.
    // The generation check ensures that if the pad was re-triggered (causing
    // a new generation), stale onEnded callbacks from old voices become no-ops
    // instead of cleaning up the new voices.
    let completedCount = 0;
    for (const voice of voices) {
      voice.onEnded = () => {
        completedCount++;
        // Guard: if generation has advanced, a new trigger owns this pad — bail out
        if ((get()._voiceGenerations[padIndex] ?? 0) !== generation) return;
        if (completedCount >= voices.length) {
          // Clean up expression chains from state (forceStop also does this,
          // so this is the natural-end cleanup path)
          const storedChains = get().activeExpressionChains[padIndex];
          if (storedChains) cleanupExpressionChains(storedChains);
          set((s) => {
            const newVoices = { ...s.activeVoices };
            delete newVoices[padIndex];
            const newPlaying = { ...s.playingPads };
            delete newPlaying[padIndex];
            const newChains = { ...s.activeExpressionChains };
            delete newChains[padIndex];
            return { activeVoices: newVoices, playingPads: newPlaying, activeExpressionChains: newChains };
          });
        }
      };
    }

    // Update state — store expression chains alongside voices for cleanup on all stop paths
    set((s) => ({
      activeVoices: { ...s.activeVoices, [padIndex]: voices },
      activeExpressionChains: { ...s.activeExpressionChains, [padIndex]: expressionChains },
      playingPads: { ...s.playingPads, [padIndex]: true },
      cycleCounters: { ...s.cycleCounters, [padIndex]: nextCycleCounter },
      lastRandomIndices: { ...s.lastRandomIndices, [padIndex]: lastRandomIndex },
    }));
  },

  stopPad: (padIndex: number) => {
    const state = get();
    const voices = state.activeVoices[padIndex];
    if (voices) forceStopVoices(voices);
    const chains = state.activeExpressionChains[padIndex];
    if (chains) cleanupExpressionChains(chains);
    set((s) => {
      const newVoices = { ...s.activeVoices };
      delete newVoices[padIndex];
      const newPlaying = { ...s.playingPads };
      delete newPlaying[padIndex];
      const newChains = { ...s.activeExpressionChains };
      delete newChains[padIndex];
      return { activeVoices: newVoices, playingPads: newPlaying, activeExpressionChains: newChains };
    });
  },

  stopAll: () => {
    const state = get();
    for (const voices of Object.values(state.activeVoices)) {
      forceStopVoices(voices);
    }
    for (const chains of Object.values(state.activeExpressionChains)) {
      cleanupExpressionChains(chains);
    }
    set({ activeVoices: {}, activeExpressionChains: {}, playingPads: {} });
  },
}));
