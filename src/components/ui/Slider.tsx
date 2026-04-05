'use client';

import React from 'react';

interface SliderProps {
  value: number;
  onChange: (value: number) => void;
  min?: number;
  max?: number;
  step?: number;
  label?: string;
  showValue?: boolean;
  unit?: string;
  className?: string;
}

export default function Slider({
  value,
  onChange,
  min = 0,
  max = 100,
  step = 1,
  label,
  showValue = true,
  unit = '',
  className = '',
}: SliderProps) {
  const percent = ((value - min) / (max - min)) * 100;

  return (
    <div className={`w-full ${className}`}>
      {(label || showValue) && (
        <div className="flex items-center justify-between mb-1.5">
          {label && <span className="text-xs font-medium text-text-secondary">{label}</span>}
          {showValue && (
            <span className="text-xs font-mono text-text-muted">
              {value.toFixed(step < 1 ? 2 : 0)}{unit}
            </span>
          )}
        </div>
      )}
      <div className="relative">
        <input
          type="range"
          min={min}
          max={max}
          step={step}
          value={value}
          aria-label={label || undefined}
          onChange={(e) => onChange(parseFloat(e.target.value))}
          className="w-full h-2 rounded-full appearance-none cursor-pointer
            bg-surface-alt outline-none
            [&::-webkit-slider-thumb]:appearance-none
            [&::-webkit-slider-thumb]:w-4 [&::-webkit-slider-thumb]:h-4
            [&::-webkit-slider-thumb]:rounded-full
            [&::-webkit-slider-thumb]:bg-accent-teal
            [&::-webkit-slider-thumb]:border-2 [&::-webkit-slider-thumb]:border-white
            [&::-webkit-slider-thumb]:shadow-card
            [&::-webkit-slider-thumb]:cursor-pointer
            [&::-webkit-slider-thumb]:transition-transform
            [&::-webkit-slider-thumb]:hover:scale-110"
          style={{
            background: `linear-gradient(to right, rgb(var(--color-accent-teal)) 0%, rgb(var(--color-accent-plum)) ${percent}%, rgb(var(--color-surface-alt)) ${percent}%)`,
          }}
        />
      </div>
    </div>
  );
}
