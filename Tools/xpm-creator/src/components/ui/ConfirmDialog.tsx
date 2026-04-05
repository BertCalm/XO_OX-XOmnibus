'use client';

import React from 'react';
import Modal from './Modal';
import Button from './Button';

type ConfirmVariant = 'danger' | 'warning';

interface ConfirmDialogProps {
  open: boolean;
  onConfirm: () => void;
  onCancel: () => void;
  title: string;
  message: string;
  confirmLabel?: string;
  cancelLabel?: string;
  variant?: ConfirmVariant;
}

const variantIcon: Record<ConfirmVariant, React.ReactNode> = {
  danger: (
    <div className="w-9 h-9 rounded-full bg-red-500/10 flex items-center justify-center shrink-0">
      <svg width="18" height="18" viewBox="0 0 18 18" fill="none">
        <path
          d="M9 6v3.5M9 12.5h.007M3.545 14.5h10.91c1.126 0 1.83-1.22 1.265-2.195L10.265 3.81c-.566-.975-1.964-.975-2.53 0L3.28 12.305c-.565.975.14 2.195 1.265 2.195z"
          stroke="currentColor"
          strokeWidth="1.5"
          strokeLinecap="round"
          strokeLinejoin="round"
          className="text-red-400"
        />
      </svg>
    </div>
  ),
  warning: (
    <div className="w-9 h-9 rounded-full bg-yellow-500/10 flex items-center justify-center shrink-0">
      <svg width="18" height="18" viewBox="0 0 18 18" fill="none">
        <path
          d="M9 6v3.5M9 12.5h.007M3.545 14.5h10.91c1.126 0 1.83-1.22 1.265-2.195L10.265 3.81c-.566-.975-1.964-.975-2.53 0L3.28 12.305c-.565.975.14 2.195 1.265 2.195z"
          stroke="currentColor"
          strokeWidth="1.5"
          strokeLinecap="round"
          strokeLinejoin="round"
          className="text-yellow-400"
        />
      </svg>
    </div>
  ),
};

export default function ConfirmDialog({
  open,
  onConfirm,
  onCancel,
  title,
  message,
  confirmLabel = 'Confirm',
  cancelLabel = 'Cancel',
  variant = 'danger',
}: ConfirmDialogProps) {
  return (
    <Modal open={open} onClose={onCancel} size="sm">
      <div className="flex gap-3">
        {variantIcon[variant]}
        <div className="flex-1 min-w-0">
          <h3 className="text-sm font-semibold text-text-primary">{title}</h3>
          <p className="text-xs text-text-secondary mt-1 leading-relaxed">{message}</p>
        </div>
      </div>
      <div className="flex items-center justify-end gap-2 mt-5">
        <Button variant="ghost" size="sm" onClick={onCancel}>
          {cancelLabel}
        </Button>
        <Button
          variant={variant === 'danger' ? 'danger' : 'primary'}
          size="sm"
          onClick={onConfirm}
        >
          {confirmLabel}
        </Button>
      </div>
    </Modal>
  );
}
