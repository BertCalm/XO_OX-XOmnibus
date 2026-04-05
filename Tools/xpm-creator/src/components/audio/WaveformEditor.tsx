'use client';

import React, { useRef, useEffect, useState, useCallback } from 'react';
import { useAudioStore } from '@/stores/audioStore';
import { sliceAudioBuffer, autoDetectTransients, normalizeBuffer } from '@/lib/audio/audioSlicer';
import { getDecodedBuffer, invalidateCache } from '@/lib/audio/audioBufferCache';
import { encodeWavAsync } from '@/lib/audio/wavEncoder';
import { generateWaveformPeaks } from '@/lib/audio/audioUtils';
import { getAudioContext } from '@/lib/audio/audioContext';
import { applyFadeIn, applyFadeOut, reverseBuffer, applyFilter } from '@/lib/audio/chopProcessors';
import { pitchShiftSemitones } from '@/lib/audio/pitchShifter';
import { analyzeSpectralFingerprint } from '@/lib/audio/spectralFingerprinter';
import { classifyBySpectrum } from '@/lib/audio/spectralFingerprinter';
import { sortToBank } from '@/lib/audio/spectralBankSorter';
import type { AudioSample, ChopRegion } from '@/types';
import { usePadStore } from '@/stores/padStore';
import { useHistoryStore } from '@/stores/historyStore';
import { v4 as uuid } from 'uuid';
import Button from '@/components/ui/Button';
import Slider from '@/components/ui/Slider';
import ContextMenu, { type ContextMenuItem } from '@/components/ui/ContextMenu';
import { useToastStore } from '@/stores/toastStore';

interface WaveformEditorProps {
  sample: AudioSample;
}

export default function WaveformEditor({ sample }: WaveformEditorProps) {
  const canvasRef = useRef<HTMLCanvasElement>(null);
  const containerRef = useRef<HTMLDivElement>(null);
  const [regions, setRegions] = useState<ChopRegion[]>([]);
  const [threshold, setThreshold] = useState(30);
  const [isSelecting, setIsSelecting] = useState(false);
  const [selectionStart, setSelectionStart] = useState<number | null>(null);
  const [selectionEnd, setSelectionEnd] = useState<number | null>(null);
  const [playingRegionId, setPlayingRegionId] = useState<string | null>(null);
  const sourceRef = useRef<AudioBufferSourceNode | null>(null);
  const mouseDownPos = useRef<{ x: number; y: number } | null>(null);
  const [selectedRegionId, setSelectedRegionId] = useState<string | null>(null);
  const [isProcessing, setIsProcessing] = useState(false);
  const [dragHandle, setDragHandle] = useState<{ regionId: string; edge: 'start' | 'end' } | null>(null);
  const [normalizeOnChop, setNormalizeOnChop] = useState(true);
  const [smartSort, setSmartSort] = useState(false);
  const [contextMenu, setContextMenu] = useState<{ x: number; y: number } | null>(null);
  const addSample = useAudioStore((s) => s.addSample);
  const updateSample = useAudioStore((s) => s.updateSample);
  const addToast = useToastStore((s) => s.addToast);

  const peaks = sample.waveformPeaks || [];
  const selectedRegion = regions.find((r) => r.id === selectedRegionId) || null;

  // Stop playback and clean up
  const stopPlayback = useCallback(() => {
    if (sourceRef.current) {
      try {
        sourceRef.current.stop();
      } catch {
        // Already stopped
      }
      sourceRef.current = null;
    }
    setPlayingRegionId(null);
  }, []);

  // Clean up playback on unmount
  useEffect(() => {
    return () => {
      if (sourceRef.current) {
        try { sourceRef.current.stop(); } catch { /* noop */ }
        sourceRef.current = null;
      }
    };
  }, []);

  // Play a specific chop region
  const playRegion = useCallback(async (region: ChopRegion) => {
    stopPlayback();

    try {
      const ctx = getAudioContext();
      const audioBuffer = await getDecodedBuffer(sample.id, sample.buffer);
      const source = ctx.createBufferSource();
      source.buffer = audioBuffer;
      source.connect(ctx.destination);
      source.onended = () => {
        sourceRef.current = null;
        setPlayingRegionId(null);
      };
      const duration = region.end - region.start;
      source.start(0, region.start, duration);
      sourceRef.current = source;
      setPlayingRegionId(region.id);
    } catch (error) {
      console.error('Region playback failed:', error);
      addToast({ type: 'error', title: 'Region playback failed', message: String(error) });
      setPlayingRegionId(null);
    }
  }, [sample.id, sample.buffer, stopPlayback]);

  // Find which chop region contains a given time
  const getRegionAtTime = useCallback((time: number): ChopRegion | null => {
    for (const region of regions) {
      if (time >= region.start && time <= region.end) {
        return region;
      }
    }
    return null;
  }, [regions]);

  // Check if mouse is near a region boundary (within 5px)
  const getHandleAtX = useCallback(
    (clientX: number): { regionId: string; edge: 'start' | 'end' } | null => {
      const canvas = canvasRef.current;
      if (!canvas || regions.length === 0) return null;
      const rect = canvas.getBoundingClientRect();
      const x = clientX - rect.left;
      const threshold = 5; // px

      for (const region of regions) {
        const startX = (region.start / sample.duration) * rect.width;
        const endX = (region.end / sample.duration) * rect.width;
        if (Math.abs(x - startX) < threshold) return { regionId: region.id, edge: 'start' };
        if (Math.abs(x - endX) < threshold) return { regionId: region.id, edge: 'end' };
      }
      return null;
    },
    [regions, sample.duration]
  );

  // Process a chop region in-place — splice the processed audio back into the full sample
  const processRegion = useCallback(
    async (
      region: ChopRegion,
      processFn: (buffer: AudioBuffer) => AudioBuffer | Promise<AudioBuffer>,
      label: string
    ) => {
      setIsProcessing(true);
      try {
        const ctx = getAudioContext();
        const fullBuffer = await getDecodedBuffer(sample.id, sample.buffer);
        const sr = fullBuffer.sampleRate;
        const startSample = Math.floor(region.start * sr);
        const endSample = Math.min(Math.floor(region.end * sr), fullBuffer.length);

        // Slice the selected region and process it
        const sliced = sliceAudioBuffer(fullBuffer, region.start, region.end);
        const processed = await processFn(sliced);

        // Splice processed region back into the full buffer
        const beforeLen = startSample;
        const afterStart = endSample;
        const afterLen = fullBuffer.length - afterStart;
        const newLength = beforeLen + processed.length + afterLen;

        const result = ctx.createBuffer(fullBuffer.numberOfChannels, newLength, sr);

        for (let ch = 0; ch < fullBuffer.numberOfChannels; ch++) {
          const srcData = fullBuffer.getChannelData(ch);
          const destData = result.getChannelData(ch);
          const processedData = processed.numberOfChannels > ch
            ? processed.getChannelData(ch)
            : processed.getChannelData(0); // fallback to mono

          // Copy audio before the region
          for (let i = 0; i < beforeLen; i++) {
            destData[i] = srcData[i];
          }
          // Copy the processed region
          for (let i = 0; i < processed.length; i++) {
            destData[beforeLen + i] = processedData[i];
          }
          // Copy audio after the region
          for (let i = 0; i < afterLen; i++) {
            destData[beforeLen + processed.length + i] = srcData[afterStart + i];
          }
        }

        // Re-encode and update the sample in-place (async — non-blocking)
        const wavData = await encodeWavAsync(result, 16);
        const newPeaks = generateWaveformPeaks(result);

        updateSample(sample.id, {
          buffer: wavData,
          waveformPeaks: newPeaks,
          duration: result.duration,
        });
        // Invalidate decode cache — the buffer has changed
        invalidateCache(sample.id);

        addToast({ type: 'success', title: `${label} applied to ${region.name}` });
      } catch (error) {
        console.error(`Process ${label} failed:`, error);
        addToast({ type: 'error', title: `${label} failed`, message: String(error) });
      } finally {
        setIsProcessing(false);
      }
    },
    [sample.buffer, sample.id, updateSample, addToast]
  );

  const handleNormalize = useCallback(() => {
    if (!selectedRegion) return;
    processRegion(selectedRegion, (buf) => normalizeBuffer(buf), 'Normalize');
  }, [selectedRegion, processRegion]);

  const handleReverse = useCallback(() => {
    if (!selectedRegion) return;
    processRegion(selectedRegion, (buf) => reverseBuffer(buf), 'Reverse');
  }, [selectedRegion, processRegion]);

  const handleFadeIn = useCallback(() => {
    if (!selectedRegion) return;
    const duration = (selectedRegion.end - selectedRegion.start) * 0.1;
    processRegion(selectedRegion, (buf) => applyFadeIn(buf, Math.max(duration, 0.01)), 'Fade In');
  }, [selectedRegion, processRegion]);

  const handleFadeOut = useCallback(() => {
    if (!selectedRegion) return;
    const duration = (selectedRegion.end - selectedRegion.start) * 0.1;
    processRegion(selectedRegion, (buf) => applyFadeOut(buf, Math.max(duration, 0.01)), 'Fade Out');
  }, [selectedRegion, processRegion]);

  const handlePitchUp = useCallback(() => {
    if (!selectedRegion) return;
    processRegion(selectedRegion, (buf) => pitchShiftSemitones(buf, 1), 'Pitch +1');
  }, [selectedRegion, processRegion]);

  const handlePitchDown = useCallback(() => {
    if (!selectedRegion) return;
    processRegion(selectedRegion, (buf) => pitchShiftSemitones(buf, -1), 'Pitch −1');
  }, [selectedRegion, processRegion]);

  const handleFilterLP = useCallback(() => {
    if (!selectedRegion) return;
    processRegion(selectedRegion, (buf) => applyFilter(buf, 'lowpass', 2000, 1), 'LP Filter');
  }, [selectedRegion, processRegion]);

  const handleFilterHP = useCallback(() => {
    if (!selectedRegion) return;
    processRegion(selectedRegion, (buf) => applyFilter(buf, 'highpass', 300, 1), 'HP Filter');
  }, [selectedRegion, processRegion]);

  // Draw waveform
  useEffect(() => {
    const canvas = canvasRef.current;
    if (!canvas || peaks.length === 0) return;

    const ctx = canvas.getContext('2d');
    if (!ctx) return;

    const dpr = window.devicePixelRatio || 1;
    const rect = canvas.getBoundingClientRect();
    canvas.width = rect.width * dpr;
    canvas.height = rect.height * dpr;
    ctx.scale(dpr, dpr);

    const width = rect.width;
    const height = rect.height;
    const midY = height / 2;

    // Background — read from theme CSS variables so dark themes work correctly
    const cs = getComputedStyle(canvas);
    const computedBg = cs.getPropertyValue('--color-surface-bg').trim();
    const computedBorder = cs.getPropertyValue('--color-border').trim();
    const computedTeal = cs.getPropertyValue('--color-accent-teal').trim();
    const computedPlum = cs.getPropertyValue('--color-accent-plum').trim();
    const tealColor = computedTeal ? `rgb(${computedTeal})` : '#0D9488';
    const plumColor = computedPlum ? `rgb(${computedPlum})` : '#7C3AED';
    ctx.fillStyle = computedBg ? `rgb(${computedBg})` : '#FAFAFA';
    ctx.fillRect(0, 0, width, height);

    // Draw selection region
    if (selectionStart !== null && selectionEnd !== null) {
      const startX = (selectionStart / sample.duration) * width;
      const endX = (selectionEnd / sample.duration) * width;
      ctx.fillStyle = 'rgba(13, 148, 136, 0.1)';
      ctx.fillRect(startX, 0, endX - startX, height);
    }

    // Draw chop regions
    for (const region of regions) {
      const startX = (region.start / sample.duration) * width;
      const endX = (region.end / sample.duration) * width;
      const isPlaying = region.id === playingRegionId;
      const isSelected = region.id === selectedRegionId;

      // Fill
      ctx.fillStyle = isSelected
        ? 'rgba(13, 148, 136, 0.15)'
        : isPlaying
          ? 'rgba(124, 58, 237, 0.2)'
          : 'rgba(124, 58, 237, 0.08)';
      ctx.fillRect(startX, 0, endX - startX, height);

      // Boundaries
      ctx.strokeStyle = isSelected
        ? 'rgba(13, 148, 136, 0.8)'
        : isPlaying
          ? 'rgba(124, 58, 237, 0.7)'
          : 'rgba(124, 58, 237, 0.3)';
      ctx.lineWidth = (isPlaying || isSelected) ? 2 : 1;
      ctx.beginPath();
      ctx.moveTo(startX, 0);
      ctx.lineTo(startX, height);
      ctx.stroke();

      // Right boundary
      if (isPlaying || isSelected) {
        ctx.beginPath();
        ctx.moveTo(endX, 0);
        ctx.lineTo(endX, height);
        ctx.stroke();
      }

      // Trim handles for selected region
      if (isSelected) {
        const handleSize = 6;
        ctx.fillStyle = 'rgba(13, 148, 136, 0.9)';
        // Start handle
        ctx.fillRect(startX - handleSize / 2, 0, handleSize, handleSize);
        ctx.fillRect(startX - handleSize / 2, height - handleSize, handleSize, handleSize);
        // End handle
        ctx.fillRect(endX - handleSize / 2, 0, handleSize, handleSize);
        ctx.fillRect(endX - handleSize / 2, height - handleSize, handleSize, handleSize);
      }
    }

    // Draw waveform — single gradient reused across all bars
    const barWidth = width / peaks.length;
    const waveGradient = ctx.createLinearGradient(0, 0, 0, height);
    waveGradient.addColorStop(0, tealColor);
    waveGradient.addColorStop(1, plumColor);
    ctx.fillStyle = waveGradient;

    for (let i = 0; i < peaks.length; i++) {
      const peak = peaks[i];
      const barHeight = peak * midY * 0.9;
      ctx.fillRect(i * barWidth, midY - barHeight, Math.max(barWidth - 0.5, 1), barHeight * 2);
    }

    // Center line
    ctx.strokeStyle = computedBorder ? `rgb(${computedBorder})` : '#E8E8E8';
    ctx.lineWidth = 0.5;
    ctx.beginPath();
    ctx.moveTo(0, midY);
    ctx.lineTo(width, midY);
    ctx.stroke();
  }, [peaks, regions, selectionStart, selectionEnd, sample.duration, playingRegionId, selectedRegionId]);

  const getTimeFromX = useCallback(
    (clientX: number): number => {
      const canvas = canvasRef.current;
      if (!canvas) return 0;
      const rect = canvas.getBoundingClientRect();
      const x = clientX - rect.left;
      return (x / rect.width) * sample.duration;
    },
    [sample.duration]
  );

  const handleMouseDown = (e: React.MouseEvent) => {
    mouseDownPos.current = { x: e.clientX, y: e.clientY };

    // Check for trim handle drag
    const handle = getHandleAtX(e.clientX);
    if (handle) {
      setDragHandle(handle);
      e.preventDefault();
      return;
    }

    setIsSelecting(true);
    const time = getTimeFromX(e.clientX);
    setSelectionStart(time);
    setSelectionEnd(time);
  };

  const handleMouseMove = (e: React.MouseEvent) => {
    // Handle trim handle dragging
    if (dragHandle) {
      const time = Math.max(0, Math.min(sample.duration, getTimeFromX(e.clientX)));
      setRegions((prev) =>
        prev.map((r) => {
          if (r.id !== dragHandle.regionId) return r;
          if (dragHandle.edge === 'start') {
            return { ...r, start: Math.min(time, r.end - 0.01) };
          } else {
            return { ...r, end: Math.max(time, r.start + 0.01) };
          }
        })
      );
      return;
    }

    // Update cursor based on hover
    if (!isSelecting && regions.length > 0) {
      const handle = getHandleAtX(e.clientX);
      const time = getTimeFromX(e.clientX);
      const hoveredRegion = getRegionAtTime(time);
      const canvas = canvasRef.current;
      if (canvas) {
        if (handle) {
          canvas.style.cursor = 'col-resize';
        } else if (hoveredRegion) {
          canvas.style.cursor = 'pointer';
        } else {
          canvas.style.cursor = 'crosshair';
        }
      }
    }

    if (!isSelecting) return;
    setSelectionEnd(getTimeFromX(e.clientX));
  };

  const handleMouseUp = (e: React.MouseEvent) => {
    // End trim handle drag
    if (dragHandle) {
      setDragHandle(null);
      mouseDownPos.current = null;
      return;
    }

    setIsSelecting(false);

    // Detect click vs drag (< 3px movement = click)
    if (mouseDownPos.current && regions.length > 0) {
      const dx = e.clientX - mouseDownPos.current.x;
      const dy = e.clientY - mouseDownPos.current.y;
      const distance = Math.sqrt(dx * dx + dy * dy);

      if (distance < 3) {
        const time = getTimeFromX(e.clientX);
        const clickedRegion = getRegionAtTime(time);

        if (clickedRegion) {
          // If double-clicking same region: toggle play. Single click: select.
          if (playingRegionId === clickedRegion.id) {
            stopPlayback();
          } else if (selectedRegionId === clickedRegion.id) {
            // Already selected — play it
            playRegion(clickedRegion);
          } else {
            // First click — select the region
            setSelectedRegionId(clickedRegion.id);
          }
          // Clear selection since this was a click, not a drag
          setSelectionStart(null);
          setSelectionEnd(null);
        } else {
          // Clicked outside any region — deselect
          setSelectedRegionId(null);
        }
      }
    }

    mouseDownPos.current = null;
  };

  const handleAutoChop = async () => {
    try {
      const audioBuffer = await getDecodedBuffer(sample.id, sample.buffer);
      const detected = autoDetectTransients(audioBuffer, threshold / 100);
      setRegions(detected);
    } catch (error) {
      console.error('Auto-chop failed:', error);
      addToast({ type: 'error', title: 'Auto-chop failed', message: String(error) });
    }
  };

  const handleExtractSelection = async () => {
    if (selectionStart === null || selectionEnd === null) return;
    const start = Math.min(selectionStart, selectionEnd);
    const end = Math.max(selectionStart, selectionEnd);
    if (end - start < 0.01) return;

    try {
      const audioBuffer = await getDecodedBuffer(sample.id, sample.buffer);
      const sliced = sliceAudioBuffer(audioBuffer, start, end);
      const wavData = await encodeWavAsync(sliced, 16);
      const newPeaks = generateWaveformPeaks(sliced);

      addSample({
        id: uuid(),
        name: `${sample.name}_chop`,
        fileName: `${sample.name}_chop.WAV`,
        duration: sliced.duration,
        sampleRate: sliced.sampleRate,
        channels: sliced.numberOfChannels,
        bitDepth: 16,
        buffer: wavData,
        waveformPeaks: newPeaks,
        rootNote: 60,
        createdAt: Date.now(),
      });

      setSelectionStart(null);
      setSelectionEnd(null);
      addToast({ type: 'success', title: 'Material extracted' });
    } catch (error) {
      console.error('Extract failed:', error);
      addToast({ type: 'error', title: 'Extract failed', message: String(error) });
    }
  };

  const handleExtractAllRegions = async () => {
    if (regions.length === 0) return;

    try {
      const audioBuffer = await getDecodedBuffer(sample.id, sample.buffer);
      let succeeded = 0;
      let failed = 0;

      for (let i = 0; i < regions.length; i++) {
        try {
          const region = regions[i];
          const sliced = sliceAudioBuffer(audioBuffer, region.start, region.end);
          const wavData = await encodeWavAsync(sliced, 16);
          const newPeaks = generateWaveformPeaks(sliced);

          addSample({
            id: uuid(),
            name: `${sample.name}_${i + 1}`,
            fileName: `${sample.name}_${i + 1}.WAV`,
            duration: sliced.duration,
            sampleRate: sliced.sampleRate,
            channels: sliced.numberOfChannels,
            bitDepth: 16,
            buffer: wavData,
            waveformPeaks: newPeaks,
            rootNote: 60,
            createdAt: Date.now(),
          });
          succeeded++;
        } catch (regionError) {
          console.warn(`Extract region ${i + 1} failed:`, regionError);
          failed++;
        }
      }

      if (failed === 0) {
        addToast({ type: 'success', title: `${succeeded} slices harvested` });
      } else {
        addToast({
          type: 'warning',
          title: `Extracted ${succeeded}/${succeeded + failed} regions (${failed} failed)`,
        });
      }
    } catch (error) {
      console.error('Extract all failed:', error);
      addToast({ type: 'error', title: 'Extract all failed', message: String(error) });
    }
  };

  /**
   * Chop → Pads: Extract all chop regions, create samples, and auto-assign
   * each one to a pad. In sequential mode, assigns in order from the current
   * bank offset. In Smart Sort mode, uses spectral analysis to place samples
   * in musically appropriate pad zones (kicks→1-4, snares→5-8, etc.).
   */
  const handleChopToPads = async () => {
    if (regions.length === 0) return;
    setIsProcessing(true);

    try {
      const audioBuffer = await getDecodedBuffer(sample.id, sample.buffer);
      const padStore = usePadStore.getState();
      const bankOffset = padStore.currentBank * 16;
      const maxPads = Math.min(regions.length, 16); // Cap at 16 pads per bank

      // Slice all regions into audio buffers
      const slicedBuffers: AudioBuffer[] = [];
      for (let i = 0; i < maxPads; i++) {
        const region = regions[i];
        const sliced = sliceAudioBuffer(audioBuffer, region.start, region.end);
        slicedBuffers.push(normalizeOnChop ? normalizeBuffer(sliced) : sliced);
      }

      // Determine pad assignment order
      let padMapping: number[]; // padMapping[sliceIdx] = relative pad index (0-15)

      if (smartSort) {
        // Smart Sort: run spectral analysis on each slice, then sort into zones.
        // Use allSettled so one analysis failure doesn't abort the entire batch.
        const results = await Promise.allSettled(
          slicedBuffers.map((buf) => analyzeSpectralFingerprint(buf)),
        );

        // Collect fingerprints — substitute a null placeholder for failures
        const fingerprints = results.map((r) => r.status === 'fulfilled' ? r.value : null);
        const failedCount = fingerprints.filter((f) => f === null).length;

        // Only run sorting on the successfully analyzed slices
        const validFingerprints = fingerprints.filter((f): f is NonNullable<typeof f> => f !== null);

        if (validFingerprints.length > 0) {
          const classifications = validFingerprints.map((fp) => classifyBySpectrum(fp));
          const bankResult = sortToBank(validFingerprints, classifications);

          // Build mapping from bankResult (which only covers valid entries)
          padMapping = new Array(maxPads).fill(-1);
          let validIdx = 0;
          for (let i = 0; i < maxPads; i++) {
            if (fingerprints[i] !== null) {
              const entry = bankResult.entries[validIdx];
              if (entry) {
                padMapping[i] = entry.targetPadIndex;
              }
              validIdx++;
            }
          }
        } else {
          padMapping = new Array(maxPads).fill(-1);
        }

        if (failedCount > 0) {
          addToast({
            type: 'warning',
            title: `Smart Sort: ${failedCount} slice(s) couldn't be analyzed — assigned sequentially`,
          });
        }

        // Fallback: any unmapped slices get the next empty pad
        const usedPads = new Set(padMapping.filter((p) => p >= 0));
        let nextEmpty = 0;
        for (let i = 0; i < maxPads; i++) {
          if (padMapping[i] === -1) {
            while (usedPads.has(nextEmpty) && nextEmpty < 16) nextEmpty++;
            padMapping[i] = nextEmpty;
            usedPads.add(nextEmpty);
            nextEmpty++;
          }
        }
      } else {
        // Sequential: assign in order
        padMapping = Array.from({ length: maxPads }, (_, i) => i);
      }

      // Snapshot for undo before modifying pads
      const history = useHistoryStore.getState();
      history.snapshot(usePadStore.getState().pads);

      // Create samples and assign to pads
      for (let i = 0; i < maxPads; i++) {
        const finalBuffer = slicedBuffers[i];
        const wavData = await encodeWavAsync(finalBuffer, 16);
        const newPeaks = generateWaveformPeaks(finalBuffer);

        const sampleId = uuid();
        const sampleName = `${sample.name}_${i + 1}`;
        const fileName = `${sampleName}.WAV`;

        // Add sample to library
        addSample({
          id: sampleId,
          name: sampleName,
          fileName,
          duration: finalBuffer.duration,
          sampleRate: finalBuffer.sampleRate,
          channels: finalBuffer.numberOfChannels,
          bitDepth: 16,
          buffer: wavData,
          waveformPeaks: newPeaks,
          rootNote: 60,
          createdAt: Date.now(),
        });

        // Assign to pad (layer 0)
        const globalPadIdx = bankOffset + padMapping[i];
        padStore.assignSampleToLayer(
          globalPadIdx,
          0,
          sampleId,
          sampleName,
          fileName,
          60
        );
      }

      history.pushState('Chop to pads', usePadStore.getState().pads);

      // Select the first assigned pad
      padStore.setActivePad(bankOffset);
      addToast({ type: 'success', title: `${maxPads} chops loaded into the forge` });
    } catch (error) {
      console.error('Chop-to-pads failed:', error);
      addToast({ type: 'error', title: 'Chop-to-pads failed', message: String(error) });
    } finally {
      setIsProcessing(false);
    }
  };

  // Right-click context menu on the waveform canvas
  const handleCanvasContextMenu = useCallback(
    (e: React.MouseEvent) => {
      e.preventDefault();
      setContextMenu({ x: e.clientX, y: e.clientY });
    },
    []
  );

  const contextMenuItems: ContextMenuItem[] = [
    {
      id: 'auto-chop',
      label: 'Auto-Chop',
      icon: <span className="text-[10px]">✂️</span>,
      action: handleAutoChop,
    },
    {
      id: 'extract-selection',
      label: 'Extract Selection',
      icon: <span className="text-[10px]">📋</span>,
      disabled: selectionStart === null || selectionEnd === null,
      action: handleExtractSelection,
    },
    {
      id: 'divider-1',
      label: '',
      divider: true,
      action: () => {},
    },
    {
      id: 'clear-regions',
      label: 'Clear All Regions',
      icon: <span className="text-[10px]">🗑️</span>,
      disabled: regions.length === 0,
      danger: true,
      action: () => setRegions([]),
    },
    {
      id: 'deselect',
      label: 'Deselect Region',
      disabled: !selectedRegionId,
      action: () => { setSelectedRegionId(null); setSelectionStart(null); setSelectionEnd(null); },
    },
    {
      id: 'divider-2',
      label: '',
      divider: true,
      action: () => {},
    },
    ...(selectedRegion
      ? [
          {
            id: 'normalize-ctx',
            label: 'Normalize',
            action: handleNormalize,
            disabled: isProcessing,
          },
          {
            id: 'reverse-ctx',
            label: 'Reverse',
            action: handleReverse,
            disabled: isProcessing,
          },
          {
            id: 'fade-in-ctx',
            label: 'Fade In',
            action: handleFadeIn,
            disabled: isProcessing,
          },
          {
            id: 'fade-out-ctx',
            label: 'Fade Out',
            action: handleFadeOut,
            disabled: isProcessing,
          },
        ] as ContextMenuItem[]
      : []),
  ];

  return (
    <div className="space-y-3">
      <div
        ref={containerRef}
        role="application"
        aria-label="Waveform editor. Use mouse to select chop regions."
        tabIndex={0}
        className="relative bg-surface-bg rounded-lg border border-border overflow-hidden"
      >
        <canvas
          ref={canvasRef}
          className="w-full h-32 cursor-crosshair"
          onMouseDown={handleMouseDown}
          onMouseMove={handleMouseMove}
          onMouseUp={handleMouseUp}
          onMouseLeave={() => { setIsSelecting(false); setDragHandle(null); mouseDownPos.current = null; }}
          onContextMenu={handleCanvasContextMenu}
        />
      </div>

      <div className="flex items-center gap-x-2 gap-y-2 flex-wrap">
        <div className="flex items-center gap-2 flex-1 min-w-[200px]">
          <Slider
            value={threshold}
            onChange={setThreshold}
            min={10}
            max={80}
            label="Sensitivity"
            className="flex-1"
          />
        </div>
        <Button variant="secondary" size="sm" onClick={handleAutoChop} disabled={isProcessing}>
          Auto-Chop
        </Button>
        {regions.length > 0 && (
          <>
            <Button variant="primary" size="sm" onClick={handleExtractAllRegions}>
              Extract All ({regions.length})
            </Button>
            <Button
              variant="accent"
              size="sm"
              onClick={handleChopToPads}
              disabled={isProcessing}
              title={`Chop & assign ${Math.min(regions.length, 16)} slices to pads in the current bank`}
            >
              ✂️ Chop → Pads
            </Button>
            <label className="flex items-center gap-1 text-[10px] text-text-secondary cursor-pointer select-none">
              <input
                type="checkbox"
                checked={normalizeOnChop}
                onChange={(e) => setNormalizeOnChop(e.target.checked)}
                className="w-3 h-3 rounded accent-accent-teal"
              />
              Normalize
            </label>
            <label
              className="flex items-center gap-1 text-[10px] text-text-secondary cursor-pointer select-none"
              title="Use spectral analysis to sort chops into musical pad zones (kicks→1-4, snares→5-8, perc→9-12, hats→13-16)"
            >
              <input
                type="checkbox"
                checked={smartSort}
                onChange={(e) => setSmartSort(e.target.checked)}
                className="w-3 h-3 rounded accent-accent-purple"
              />
              Smart Sort
            </label>
          </>
        )}
        {selectionStart !== null && selectionEnd !== null && (
          <Button variant="accent" size="sm" onClick={handleExtractSelection}>
            Extract Selection
          </Button>
        )}
      </div>

      {/* Chop manipulation toolbar — shown when a region is selected */}
      {selectedRegion && (
        <div className="flex items-center gap-1.5 flex-wrap p-2 bg-surface-alt rounded-lg border border-border">
          <span className="text-[10px] font-medium text-text-muted mr-1 uppercase tracking-wider">
            {selectedRegion.name}
          </span>
          <Button variant="ghost" size="sm" onClick={handleNormalize} disabled={isProcessing}>
            Normalize
          </Button>
          <Button variant="ghost" size="sm" onClick={handleReverse} disabled={isProcessing}>
            Reverse
          </Button>
          <Button variant="ghost" size="sm" onClick={handleFadeIn} disabled={isProcessing}>
            Fade In
          </Button>
          <Button variant="ghost" size="sm" onClick={handleFadeOut} disabled={isProcessing}>
            Fade Out
          </Button>
          <div className="w-px h-4 bg-border mx-0.5" />
          <Button variant="ghost" size="sm" onClick={handlePitchDown} disabled={isProcessing}>
            Pitch −
          </Button>
          <Button variant="ghost" size="sm" onClick={handlePitchUp} disabled={isProcessing}>
            Pitch +
          </Button>
          <div className="w-px h-4 bg-border mx-0.5" />
          <Button variant="ghost" size="sm" onClick={handleFilterLP} disabled={isProcessing}>
            LP Filter
          </Button>
          <Button variant="ghost" size="sm" onClick={handleFilterHP} disabled={isProcessing}>
            HP Filter
          </Button>
        </div>
      )}

      {/* Right-click context menu */}
      {contextMenu && (
        <ContextMenu
          items={contextMenuItems}
          x={contextMenu.x}
          y={contextMenu.y}
          onClose={() => setContextMenu(null)}
        />
      )}
    </div>
  );
}
