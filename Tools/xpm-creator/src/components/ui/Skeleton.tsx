'use client';

import React from 'react';

type SkeletonVariant = 'text' | 'circle' | 'rect';

interface SkeletonProps {
  variant?: SkeletonVariant;
  width?: string | number;
  height?: string | number;
  className?: string;
  /** Number of skeleton lines to render (only for text variant) */
  lines?: number;
}

function singleSkeleton(
  variant: SkeletonVariant,
  width?: string | number,
  height?: string | number,
  className = '',
  key?: number
) {
  const w = typeof width === 'number' ? `${width}px` : width;
  const h = typeof height === 'number' ? `${height}px` : height;

  const baseClass = `skeleton-shimmer bg-surface-alt ${className}`;

  switch (variant) {
    case 'circle':
      return (
        <div
          key={key}
          className={`${baseClass} rounded-full shrink-0`}
          style={{
            width: w || '32px',
            height: h || w || '32px',
          }}
        />
      );
    case 'rect':
      return (
        <div
          key={key}
          className={`${baseClass} rounded-lg`}
          style={{
            width: w || '100%',
            height: h || '40px',
          }}
        />
      );
    case 'text':
    default:
      return (
        <div
          key={key}
          className={`${baseClass} rounded`}
          style={{
            width: w || '100%',
            height: h || '12px',
          }}
        />
      );
  }
}

export default function Skeleton({
  variant = 'text',
  width,
  height,
  className = '',
  lines = 1,
}: SkeletonProps) {
  if (variant === 'text' && lines > 1) {
    return (
      <div className="space-y-2">
        {Array.from({ length: lines }).map((_, i) =>
          singleSkeleton(
            'text',
            // Last line is shorter for a natural look
            i === lines - 1 ? '60%' : width,
            height,
            className,
            i
          )
        )}
      </div>
    );
  }

  return singleSkeleton(variant, width, height, className);
}
