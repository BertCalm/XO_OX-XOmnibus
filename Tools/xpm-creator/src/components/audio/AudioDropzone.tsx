'use client';

import React, { useCallback, useState, useRef } from 'react';
import { getAcceptedAudioFormats } from '@/lib/audio/audioUtils';

interface AudioDropzoneProps {
  onFilesSelected: (files: File[]) => void;
  accepting?: boolean;
}

export default function AudioDropzone({ onFilesSelected, accepting = true }: AudioDropzoneProps) {
  const [isDragging, setIsDragging] = useState(false);
  const inputRef = useRef<HTMLInputElement>(null);

  const handleDrop = useCallback(
    (e: React.DragEvent) => {
      e.preventDefault();
      setIsDragging(false);
      if (!accepting) return;

      const files = Array.from(e.dataTransfer.files);
      if (files.length > 0) {
        onFilesSelected(files);
      }
    },
    [onFilesSelected, accepting]
  );

  const handleDragOver = useCallback((e: React.DragEvent) => {
    e.preventDefault();
    setIsDragging(true);
  }, []);

  const handleDragLeave = useCallback((e: React.DragEvent) => {
    e.preventDefault();
    // Only clear dragging state when leaving the dropzone entirely,
    // not when moving between child elements (prevents flicker)
    const relatedTarget = e.relatedTarget as Node | null;
    if (!relatedTarget || !e.currentTarget.contains(relatedTarget)) {
      setIsDragging(false);
    }
  }, []);

  const handleClick = () => {
    inputRef.current?.click();
  };

  const handleFileInput = (e: React.ChangeEvent<HTMLInputElement>) => {
    const files = Array.from(e.target.files || []);
    if (files.length > 0) {
      onFilesSelected(files);
    }
    if (inputRef.current) inputRef.current.value = '';
  };

  return (
    <div
      role="button"
      tabIndex={accepting ? 0 : -1}
      aria-label="Drop audio files or click to browse"
      onDrop={handleDrop}
      onDragOver={handleDragOver}
      onDragLeave={handleDragLeave}
      onClick={handleClick}
      onKeyDown={(e) => {
        if (e.key === 'Enter' || e.key === ' ') {
          e.preventDefault();
          handleClick();
        }
      }}
      className={`relative flex flex-col items-center justify-center gap-3 p-8
        border-2 border-dashed rounded-xl cursor-pointer
        transition-all duration-200
        ${isDragging
          ? 'border-accent-teal bg-accent-teal-50 scale-[1.02]'
          : 'border-border hover:border-accent-teal/40 hover:bg-surface-alt'
        }
        ${!accepting ? 'opacity-50 pointer-events-none' : ''}`}
    >
      <div className={`w-12 h-12 rounded-full flex items-center justify-center
        ${isDragging ? 'bg-accent-teal/10' : 'bg-surface-alt'}`}>
        <svg width="24" height="24" viewBox="0 0 24 24" fill="none"
          className={isDragging ? 'text-accent-teal' : 'text-text-muted'}>
          <path d="M12 16V4M8 8l4-4 4 4" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round" />
          <path d="M20 16v2a2 2 0 01-2 2H6a2 2 0 01-2-2v-2" stroke="currentColor" strokeWidth="2" strokeLinecap="round" />
        </svg>
      </div>

      <div className="text-center">
        <p className="text-sm font-medium text-text-primary">
          {isDragging ? 'Drop files here' : 'Drop audio files or click to browse'}
        </p>
        <p className="text-xs text-text-muted mt-1">
          WAV, MP3, FLAC, MP4 (audio will be extracted)
        </p>
      </div>

      <input
        ref={inputRef}
        type="file"
        accept={getAcceptedAudioFormats()}
        multiple
        className="hidden"
        aria-label="Select audio files"
        onChange={handleFileInput}
      />
    </div>
  );
}
