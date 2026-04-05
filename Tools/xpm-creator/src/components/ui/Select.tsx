'use client';

import React, { useId } from 'react';

type SelectSize = 'sm' | 'md';

interface SelectOption {
  value: string;
  label: string;
}

interface SelectProps extends Omit<React.SelectHTMLAttributes<HTMLSelectElement>, 'size'> {
  options: SelectOption[];
  label?: string;
  placeholder?: string;
  size?: SelectSize;
}

const sizeClasses: Record<SelectSize, string> = {
  sm: 'px-2 py-1 text-[10px] pr-6',
  md: 'px-3 py-2 text-xs pr-8',
};

export default function Select({
  options,
  label,
  placeholder,
  size = 'md',
  disabled,
  className = '',
  ...props
}: SelectProps) {
  const generatedId = useId();
  const selectId = props.id || generatedId;

  return (
    <div className="flex flex-col gap-1">
      {label && (
        <label
          htmlFor={selectId}
          className="text-[10px] font-medium text-text-muted uppercase tracking-wider"
        >
          {label}
        </label>
      )}
      <div className="relative">
        <select
          id={selectId}
          disabled={disabled}
          className={`appearance-none w-full rounded-lg
            bg-surface border border-border text-text-primary
            hover:border-text-muted focus:border-accent-teal focus:outline-none
            focus:ring-1 focus:ring-accent-teal/30
            transition-all duration-150 cursor-pointer
            disabled:opacity-50 disabled:cursor-not-allowed
            ${sizeClasses[size]} ${className}`}
          {...props}
        >
          {placeholder && (
            <option value="" disabled>
              {placeholder}
            </option>
          )}
          {options.map((opt) => (
            <option key={opt.value} value={opt.value}>
              {opt.label}
            </option>
          ))}
        </select>
        {/* Custom chevron */}
        <svg
          className={`absolute right-2 top-1/2 -translate-y-1/2 pointer-events-none text-text-muted
            ${size === 'sm' ? 'w-3 h-3' : 'w-3.5 h-3.5'}`}
          viewBox="0 0 16 16"
          fill="none"
        >
          <path
            d="M4 6l4 4 4-4"
            stroke="currentColor"
            strokeWidth="1.5"
            strokeLinecap="round"
            strokeLinejoin="round"
          />
        </svg>
      </div>
    </div>
  );
}
