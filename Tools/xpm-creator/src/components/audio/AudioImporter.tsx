'use client';

import React, { useCallback } from 'react';
import AudioDropzone from './AudioDropzone';
import { useAudioStore } from '@/stores/audioStore';
import { decodeAudioFile, decodeArrayBuffer, isVideoFile, generateWaveformPeaks } from '@/lib/audio/audioUtils';
import { extractAudioFromVideo } from '@/lib/audio/mp4Extractor';
import type { AudioSample } from '@/types';
import { v4 as uuid } from 'uuid';
import { encodeWavAsync } from '@/lib/audio/wavEncoder';
import { detectRootNote } from '@/lib/audio/pitchDetector';
import { parseFilenameForMapping } from '@/lib/audio/filenameParser';
import { useToastStore } from '@/stores/toastStore';
import { getProcessingMessage } from '@/lib/audio/processingMessages';

export default function AudioImporter() {
  const addSample = useAudioStore((s) => s.addSample);
  const setProcessing = useAudioStore((s) => s.setProcessing);
  const isProcessing = useAudioStore((s) => s.isProcessing);
  const processingMessage = useAudioStore((s) => s.processingMessage);

  const handleFiles = useCallback(
    async (files: File[]) => {
      try {
        for (const file of files) {
          try {
            setProcessing(true, `Processing ${file.name}...`);

            let audioBuffer: AudioBuffer;

            if (isVideoFile(file)) {
              setProcessing(true, `Extracting audio from ${file.name}...`);
              const wavData = await extractAudioFromVideo(file, (p) => {
                setProcessing(true, `Extracting audio... ${Math.round(p)}%`);
              });
              audioBuffer = await decodeArrayBuffer(wavData);
            } else {
              audioBuffer = await decodeAudioFile(file);
            }

            const wavData = await encodeWavAsync(audioBuffer, 16);
            const peaks = generateWaveformPeaks(audioBuffer);
            const baseName = file.name.replace(/\.[^.]+$/, '');

            // Detect root note: first try filename parsing, then pitch detection
            let rootNote = 60;
            const filenameParsed = parseFilenameForMapping(file.name);
            if (filenameParsed.rootNote !== null) {
              rootNote = filenameParsed.rootNote;
            } else {
              // Fall back to pitch detection for tonal samples
              try {
                setProcessing(true, `Detecting pitch for ${file.name}...`);
                const pitchResult = detectRootNote(audioBuffer);
                if (pitchResult) {
                  rootNote = pitchResult.midiNote;
                }
              } catch {
                // Pitch detection failed (e.g., noise/percussion) — keep default
              }
            }

            const sample: AudioSample = {
              id: uuid(),
              name: baseName,
              fileName: `${baseName}.WAV`,
              duration: audioBuffer.duration,
              sampleRate: audioBuffer.sampleRate,
              channels: audioBuffer.numberOfChannels,
              bitDepth: 16,
              buffer: wavData,
              waveformPeaks: peaks,
              rootNote,
              createdAt: Date.now(),
            };

            addSample(sample);
          } catch (error) {
            console.error(`Failed to process ${file.name}:`, error);
            useToastStore.getState().addToast({
              type: 'error',
              title: `Failed to import ${file.name}`,
              message: error instanceof Error ? error.message : String(error),
            });
          }
        }
      } finally {
        setProcessing(false);
      }
    },
    [addSample, setProcessing]
  );

  return (
    <div className="space-y-3">
      <AudioDropzone onFilesSelected={handleFiles} accepting={!isProcessing} />
      <div role="status" aria-live="polite" aria-atomic="true">
        {isProcessing && (
          <div className="flex items-center gap-2 px-3 py-2 bg-accent-teal-50 rounded-lg">
            <svg className="animate-spin h-4 w-4 text-accent-teal flex-shrink-0" aria-hidden="true" viewBox="0 0 24 24">
              <circle className="opacity-25" cx="12" cy="12" r="10" stroke="currentColor" strokeWidth="4" fill="none" />
              <path className="opacity-75" fill="currentColor" d="M4 12a8 8 0 018-8V0C5.373 0 0 5.373 0 12h4z" />
            </svg>
            <div className="min-w-0">
              <span className="text-xs text-accent-teal font-medium block truncate">{processingMessage}</span>
              <span className="text-[10px] text-accent-teal/60 italic">{getProcessingMessage('import')}</span>
            </div>
          </div>
        )}
      </div>
    </div>
  );
}
