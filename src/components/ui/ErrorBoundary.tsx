'use client';

import React, { Component } from 'react';
import { useErrorStore } from '@/stores/errorStore';

interface Props {
  children: React.ReactNode;
  fallback?: React.ReactNode;
  /** Name of the section for error reporting */
  section?: string;
}

interface State {
  hasError: boolean;
  error: Error | null;
}

export default class ErrorBoundary extends Component<Props, State> {
  state: State = { hasError: false, error: null };

  static getDerivedStateFromError(error: Error): State {
    return { hasError: true, error };
  }

  componentDidCatch(error: Error, info: React.ErrorInfo) {
    // Log to centralized error store with toast notification
    useErrorStore.getState().logError(
      this.props.section || 'Unknown section',
      error,
      {
        componentStack: info.componentStack ?? undefined,
        section: this.props.section,
      }
    );
  }

  handleReset = () => {
    this.setState({ hasError: false, error: null });
  };

  render() {
    if (this.state.hasError) {
      if (this.props.fallback) return this.props.fallback;

      return (
        <div className="p-6 text-center space-y-3">
          <div className="text-2xl">&#9888;&#65039;</div>
          <h3 className="text-sm font-semibold text-text-primary">
            Something went wrong{this.props.section ? ` in ${this.props.section}` : ''}
          </h3>
          <p className="text-xs text-text-muted max-w-xs mx-auto">
            {this.state.error?.message || 'An unexpected error occurred.'}
          </p>
          <p className="text-[10px] text-text-muted">
            This error has been logged. Check the Error Log for details and suggested fixes.
          </p>
          <button
            onClick={this.handleReset}
            className="px-3 py-1.5 rounded-lg text-xs font-medium bg-accent-teal text-white hover:bg-accent-teal-dark transition-colors"
          >
            Try Again
          </button>
        </div>
      );
    }
    return this.props.children;
  }
}
