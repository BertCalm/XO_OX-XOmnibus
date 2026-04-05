'use client';

import React from 'react';

type ButtonVariant = 'primary' | 'secondary' | 'accent' | 'ghost' | 'danger';
type ButtonSize = 'sm' | 'md' | 'lg';

interface ButtonProps extends React.ButtonHTMLAttributes<HTMLButtonElement> {
  variant?: ButtonVariant;
  size?: ButtonSize;
  icon?: React.ReactNode;
  loading?: boolean;
}

const variantClasses: Record<ButtonVariant, string> = {
  primary: 'bg-accent-teal text-white hover:bg-accent-teal-dark',
  secondary: 'bg-surface border border-border text-text-primary hover:bg-surface-alt',
  accent: 'bg-accent-plum text-white hover:bg-accent-plum-dark',
  ghost: 'bg-transparent text-text-secondary hover:bg-surface-alt hover:text-text-primary',
  danger: 'bg-red-500 text-white hover:bg-red-600',
};

const sizeClasses: Record<ButtonSize, string> = {
  sm: 'px-3 py-1.5 text-xs gap-1.5',
  md: 'px-4 py-2.5 text-sm gap-2',
  lg: 'px-6 py-3 text-base gap-2.5',
};

export default function Button({
  variant = 'primary',
  size = 'md',
  icon,
  loading,
  children,
  disabled,
  className = '',
  ...props
}: ButtonProps) {
  return (
    <button
      className={`inline-flex items-center justify-center font-medium rounded-lg
        transition-all duration-150 ease-out active:scale-[0.98]
        disabled:opacity-50 disabled:pointer-events-none disabled:cursor-not-allowed
        ${variantClasses[variant]} ${sizeClasses[size]} ${className}`}
      disabled={disabled || loading}
      {...props}
    >
      {loading ? (
        <svg className="animate-spin h-4 w-4" viewBox="0 0 24 24">
          <circle className="opacity-25" cx="12" cy="12" r="10" stroke="currentColor" strokeWidth="4" fill="none" />
          <path className="opacity-75" fill="currentColor" d="M4 12a8 8 0 018-8V0C5.373 0 0 5.373 0 12h4z" />
        </svg>
      ) : icon ? (
        <span className="flex-shrink-0">{icon}</span>
      ) : null}
      {children}
    </button>
  );
}
