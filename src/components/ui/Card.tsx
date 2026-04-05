'use client';

import React from 'react';

interface CardProps {
  children: React.ReactNode;
  elevated?: boolean;
  className?: string;
  padding?: 'none' | 'sm' | 'md' | 'lg';
}

const paddingClasses = {
  none: '',
  sm: 'p-3',
  md: 'p-4',
  lg: 'p-6',
};

export default function Card({
  children,
  elevated = false,
  className = '',
  padding = 'md',
}: CardProps) {
  return (
    <div
      className={`bg-surface rounded-xl border border-border
        ${elevated ? 'shadow-elevated' : 'shadow-card'}
        ${paddingClasses[padding]} ${className}`}
    >
      {children}
    </div>
  );
}

export function CardHeader({
  children,
  className = '',
}: {
  children: React.ReactNode;
  className?: string;
}) {
  return (
    <div className={`flex items-center justify-between pb-3 mb-3 border-b border-border ${className}`}>
      {children}
    </div>
  );
}

export function CardTitle({
  children,
  className = '',
}: {
  children: React.ReactNode;
  className?: string;
}) {
  return (
    <h3 className={`text-sm font-semibold text-text-primary ${className}`}>{children}</h3>
  );
}
