'use client';

import React, { useEffect, useRef, useState } from 'react';
import { useToastStore } from '@/stores/toastStore';

interface CoverArtUploaderProps {
  onImageSelected: (data: ArrayBuffer, type: 'jpg' | 'png') => void;
  currentImage?: ArrayBuffer | null;
}

export default function CoverArtUploader({ onImageSelected, currentImage }: CoverArtUploaderProps) {
  const inputRef = useRef<HTMLInputElement>(null);
  const [previewUrl, setPreviewUrl] = useState<string | null>(null);
  // Track the latest blob URL in a ref so the cleanup always revokes the
  // current URL — not the stale mount-time value captured by the effect closure.
  const previewUrlRef = useRef<string | null>(null);

  // Revoke the object URL on unmount to prevent memory leaks
  useEffect(() => {
    return () => {
      if (previewUrlRef.current) URL.revokeObjectURL(previewUrlRef.current);
    };
  }, []);

  const handleFileSelect = async (e: React.ChangeEvent<HTMLInputElement>) => {
    const file = e.target.files?.[0];
    if (!file) return;

    const isJpg = file.type === 'image/jpeg';
    const isPng = file.type === 'image/png';
    if (!isJpg && !isPng) {
      useToastStore.getState().addToast({
        type: 'warning',
        title: 'Unsupported image format',
        message: `"${file.name}" is not a JPEG or PNG. Please select a .jpg or .png file.`,
      });
      return;
    }

    try {
      const buffer = await file.arrayBuffer();
      const type = isJpg ? 'jpg' as const : 'png' as const;

      // Create preview — revoke previous URL to prevent memory leak
      const blob = new Blob([buffer], { type: file.type });
      const url = URL.createObjectURL(blob);
      if (previewUrlRef.current) URL.revokeObjectURL(previewUrlRef.current);
      previewUrlRef.current = url;
      setPreviewUrl(url);

      onImageSelected(buffer, type);
    } catch (err) {
      console.error('Failed to read cover art file:', err);
      useToastStore.getState().addToast({
        type: 'error',
        title: 'Failed to read image',
        message: `Could not read "${file.name}". Try selecting the file again.`,
      });
    }
  };

  return (
    <div className="space-y-2">
      <span className="label">Cover Art</span>
      <div
        onClick={() => inputRef.current?.click()}
        className="relative w-32 h-32 rounded-xl border-2 border-dashed border-border
          hover:border-accent-plum/40 cursor-pointer transition-all overflow-hidden
          flex items-center justify-center bg-surface-alt"
      >
        {previewUrl ? (
          <img src={previewUrl} alt="Cover" className="w-full h-full object-cover" />
        ) : (
          <div className="text-center">
            <svg width="24" height="24" viewBox="0 0 24 24" fill="none" className="mx-auto text-text-muted">
              <rect x="3" y="3" width="18" height="18" rx="3" stroke="currentColor" strokeWidth="1.5" />
              <circle cx="9" cy="9" r="2" stroke="currentColor" strokeWidth="1.5" />
              <path d="M3 16l5-5 4 4 3-3 6 6" stroke="currentColor" strokeWidth="1.5" strokeLinecap="round" strokeLinejoin="round" />
            </svg>
            <p className="text-[10px] text-text-muted mt-1">1000x1000 rec.</p>
          </div>
        )}
      </div>
      <input
        ref={inputRef}
        type="file"
        accept="image/jpeg,image/png"
        className="hidden"
        onChange={handleFileSelect}
      />
    </div>
  );
}
