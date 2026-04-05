'use client';

import React, { useState, useEffect, useRef, useCallback } from 'react';
import Button from '@/components/ui/Button';
import ThemeSwitcher from '@/components/ui/ThemeSwitcher';
import { useAudioStore } from '@/stores/audioStore';
import { usePadStore } from '@/stores/padStore';
import { useExportStore } from '@/stores/exportStore';
import { useThemeStore, THEMES } from '@/stores/themeStore';
import { useKonamiCode } from '@/hooks/useKonamiCode';
import { getGreeting } from '@/lib/greetings';
import { useMilestoneStore } from '@/stores/milestoneStore';

/**
 * XO_OX emotional states — the logo picks the highest-priority matching state.
 * Each state maps to unique eye behavior and optional glow.
 */
export type LogoState =
  | 'celebrating'  // Export success, milestone achieved
  | 'concerned'    // Error occurred
  | 'thinking'     // Processing / exporting
  | 'excited'      // First sample drop of session
  | 'focused'      // Active playback
  | 'listening'    // Samples loaded, idle
  | 'sleeping'     // Idle for 5+ minutes
  | 'idle';        // Default state

/** Eye parameters per logo state */
const LOGO_STATE_EYES: Record<LogoState, { height: number; glow?: string }> = {
  celebrating: { height: 3.5, glow: '0 0 8px rgb(var(--color-accent-teal) / 0.6)' },
  concerned:   { height: 1, glow: '0 0 4px rgba(239, 68, 68, 0.4)' },
  thinking:    { height: 3, glow: '0 0 6px rgb(var(--color-accent-plum) / 0.4)' },
  excited:     { height: 3.5, glow: '0 0 8px rgb(var(--color-accent-teal) / 0.5)' },
  focused:     { height: 2.5 },
  listening:   { height: 2 },
  sleeping:    { height: 0.5 },
  idle:        { height: 2 },
};

interface HeaderProps {
  projectName?: string;
  onNewProject?: () => void;
  onExport?: () => void;
  activeSection?: 'crate' | 'forge' | 'breath';
}

const SECTION_LABELS: Record<string, string> = {
  crate: 'The Crate',
  forge: 'The Forge',
  breath: 'The Breath',
};

/**
 * Sentient XO_OX Logo SVG with animated blinking "eyes" (the underscores).
 * The underscores between the letters act as eyelids that blink.
 * Now driven by an 8-state emotional system.
 */
function SentientLogo({
  isBlinking,
  logoState,
}: {
  isBlinking: boolean;
  logoState: LogoState;
}) {
  const stateEyes = LOGO_STATE_EYES[logoState];
  // Blink overrides state eye height
  const eyeHeight = isBlinking ? 0.3 : stateEyes.height;
  // Eye y position adjusts so they stay vertically centered on the baseline
  const eyeBaseY = 18;
  const eyeY = eyeBaseY - eyeHeight / 2;

  return (
    <svg
      width="80"
      height="24"
      viewBox="0 0 80 24"
      fill="none"
      xmlns="http://www.w3.org/2000/svg"
      aria-label="XO_OX logo"
    >
      <defs>
        <linearGradient id="xo-gradient" x1="0" y1="0" x2="80" y2="24" gradientUnits="userSpaceOnUse">
          <stop offset="0%" stopColor="rgb(var(--color-accent-teal, 13 148 136))" />
          <stop offset="100%" stopColor="rgb(var(--color-accent-plum, 124 58 237))" />
        </linearGradient>
      </defs>

      {/* X */}
      <text
        x="2"
        y="17"
        fontFamily="'SF Mono', 'Fira Code', 'Cascadia Code', monospace"
        fontSize="16"
        fontWeight="700"
        fill="url(#xo-gradient)"
        letterSpacing="0"
      >
        X
      </text>

      {/* O */}
      <text
        x="14"
        y="17"
        fontFamily="'SF Mono', 'Fira Code', 'Cascadia Code', monospace"
        fontSize="16"
        fontWeight="700"
        fill="url(#xo-gradient)"
        letterSpacing="0"
      >
        O
      </text>

      {/* Left eye (underscore) */}
      <rect
        x="28"
        y={eyeY}
        width="8"
        height={eyeHeight}
        rx="0.5"
        fill="url(#xo-gradient)"
        style={{
          transition: isBlinking
            ? 'height 75ms ease-in, y 75ms ease-in'
            : 'height 200ms cubic-bezier(0.4, 0, 0.2, 1), y 200ms cubic-bezier(0.4, 0, 0.2, 1)',
          filter: stateEyes.glow ? `drop-shadow(${stateEyes.glow})` : undefined,
          animation: logoState === 'sleeping' ? 'xo-breathe 4s ease-in-out infinite' : undefined,
          opacity: logoState === 'sleeping' ? 0.5 : 1,
        }}
      />

      {/* O */}
      <text
        x="40"
        y="17"
        fontFamily="'SF Mono', 'Fira Code', 'Cascadia Code', monospace"
        fontSize="16"
        fontWeight="700"
        fill="url(#xo-gradient)"
        letterSpacing="0"
      >
        O
      </text>

      {/* X */}
      <text
        x="52"
        y="17"
        fontFamily="'SF Mono', 'Fira Code', 'Cascadia Code', monospace"
        fontSize="16"
        fontWeight="700"
        fill="url(#xo-gradient)"
        letterSpacing="0"
      >
        X
      </text>

      {/* Right eye (underscore) */}
      <rect
        x="38"
        y={eyeY}
        width="8"
        height={eyeHeight}
        rx="0.5"
        fill="url(#xo-gradient)"
        style={{
          transition: isBlinking
            ? 'height 75ms ease-in, y 75ms ease-in'
            : 'height 200ms cubic-bezier(0.4, 0, 0.2, 1), y 200ms cubic-bezier(0.4, 0, 0.2, 1)',
          filter: stateEyes.glow ? `drop-shadow(${stateEyes.glow})` : undefined,
          animation: logoState === 'sleeping' ? 'xo-breathe 4s ease-in-out infinite' : undefined,
          opacity: logoState === 'sleeping' ? 0.5 : 1,
        }}
      />

      {/* Sleeping breathing animation */}
      <style>{`
        @keyframes xo-breathe {
          0%, 100% { opacity: 0.3; }
          50% { opacity: 0.7; }
        }
      `}</style>
    </svg>
  );
}

export default function Header({
  projectName,
  onNewProject,
  onExport,
  activeSection,
}: HeaderProps) {
  const [isBlinking, setIsBlinking] = useState(false);
  const blinkTimeoutRef = useRef<ReturnType<typeof setTimeout> | null>(null);
  const blinkIntervalRef = useRef<ReturnType<typeof setTimeout> | null>(null);

  // Granular selectors — compute derived values inside selectors for minimal re-renders
  const sampleCount = useAudioStore((s) => s.samples.length);
  const isProcessing = useAudioStore((s) => s.isProcessing);
  const hasAssignedPads = usePadStore((s) =>
    s.pads.some((pad) => pad.layers.some((layer) => layer.active && layer.sampleId !== null))
  );
  const isExporting = useExportStore((s) => s.isExporting);
  const setTheme = useThemeStore((s) => s.setTheme);

  // Audio context status: consider active if there are samples loaded
  const audioEngineActive = sampleCount > 0;

  // ---------------------------------------------------------------------------
  // Logo State Machine — derives emotional state from store signals
  // ---------------------------------------------------------------------------
  const [overrideState, setOverrideState] = useState<LogoState | null>(null);
  const overrideTimeoutRef = useRef<ReturnType<typeof setTimeout> | null>(null);
  const [isSleeping, setIsSleeping] = useState(false);
  const sleepTimerRef = useRef<ReturnType<typeof setTimeout> | null>(null);
  const prevSampleCountRef = useRef(sampleCount);

  // Trigger 'excited' on first sample import of session
  const hasSeenSampleRef = useRef(false);
  useEffect(() => {
    if (sampleCount > 0 && prevSampleCountRef.current === 0 && !hasSeenSampleRef.current) {
      hasSeenSampleRef.current = true;
      setOverrideState('excited');
      if (overrideTimeoutRef.current) clearTimeout(overrideTimeoutRef.current);
      overrideTimeoutRef.current = setTimeout(() => setOverrideState(null), 3000);
    }
    prevSampleCountRef.current = sampleCount;
  }, [sampleCount]);

  // Idle timer → sleeping after 5 minutes of no interaction
  const resetSleepTimer = useCallback(() => {
    setIsSleeping(false);
    if (sleepTimerRef.current) clearTimeout(sleepTimerRef.current);
    sleepTimerRef.current = setTimeout(() => {
      setIsSleeping(true);
    }, 5 * 60 * 1000); // 5 minutes
  }, []);

  useEffect(() => {
    resetSleepTimer();
    // Listen for any user activity to reset
    const onActivity = () => resetSleepTimer();
    window.addEventListener('mousedown', onActivity);
    window.addEventListener('keydown', onActivity);
    return () => {
      if (sleepTimerRef.current) clearTimeout(sleepTimerRef.current);
      window.removeEventListener('mousedown', onActivity);
      window.removeEventListener('keydown', onActivity);
    };
  }, [resetSleepTimer]);

  // Cleanup override timeout
  useEffect(() => {
    return () => {
      if (overrideTimeoutRef.current) clearTimeout(overrideTimeoutRef.current);
    };
  }, []);

  /** Trigger a temporary logo state (e.g., 'celebrating' for 3s) */
  const triggerLogoState = useCallback((state: LogoState, durationMs: number = 3000) => {
    setOverrideState(state);
    if (overrideTimeoutRef.current) clearTimeout(overrideTimeoutRef.current);
    overrideTimeoutRef.current = setTimeout(() => setOverrideState(null), durationMs);
  }, []);

  // Celebrate milestones — watch achievement count and trigger celebrating state on increase
  const achievedCount = useMilestoneStore((s) => s.achieved.size);
  const prevAchievedCountRef = useRef(achievedCount);
  useEffect(() => {
    if (achievedCount > prevAchievedCountRef.current) {
      triggerLogoState('celebrating', 3000);
    }
    prevAchievedCountRef.current = achievedCount;
  }, [achievedCount, triggerLogoState]);

  // Derive current logo state from priority order
  const logoState: LogoState = overrideState
    ?? (isProcessing || isExporting ? 'thinking' : null)
    ?? (isSleeping ? 'sleeping' : null)
    ?? (sampleCount > 0 ? 'listening' : null)
    ?? 'idle';

  // ---------------------------------------------------------------------------
  // Greeting system — pick a random greeting on mount (client-only to avoid
  // SSR hydration mismatch since getGreeting() uses Math.random())
  // ---------------------------------------------------------------------------
  const [greeting, setGreeting] = useState<{ text: string; emoji: string }>({ text: '', emoji: '' });
  const [greetingReady, setGreetingReady] = useState(false);
  useEffect(() => {
    setGreeting(getGreeting());
    setGreetingReady(true);
  }, []);

  // ---------------------------------------------------------------------------
  // About Easter egg — 5 rapid clicks on the logo area
  // ---------------------------------------------------------------------------
  const [showAbout, setShowAbout] = useState(false);
  const logoClickTimestamps = useRef<number[]>([]);
  const aboutTimeoutRef = useRef<ReturnType<typeof setTimeout> | null>(null);

  const handleLogoClick = useCallback((e: React.MouseEvent) => {
    e.stopPropagation();

    const now = Date.now();
    // Keep only clicks within the last 2 seconds
    logoClickTimestamps.current = logoClickTimestamps.current.filter(
      (t) => now - t < 2000
    );
    logoClickTimestamps.current.push(now);

    if (logoClickTimestamps.current.length >= 5) {
      logoClickTimestamps.current = [];
      setShowAbout(true);
      // Auto-dismiss after 4 seconds
      if (aboutTimeoutRef.current) clearTimeout(aboutTimeoutRef.current);
      aboutTimeoutRef.current = setTimeout(() => {
        setShowAbout(false);
      }, 4000);
    }
  }, []);

  // Cleanup about timeout
  useEffect(() => {
    return () => {
      if (aboutTimeoutRef.current) clearTimeout(aboutTimeoutRef.current);
    };
  }, []);

  // ---------------------------------------------------------------------------
  // Konami Code Easter egg — rapid theme cycling then random landing
  // ---------------------------------------------------------------------------
  const konamiCycleRef = useRef<ReturnType<typeof setInterval> | null>(null);
  const konamiTimeoutRef = useRef<ReturnType<typeof setTimeout> | null>(null);

  const handleKonamiCode = useCallback(() => {
    if (process.env.NODE_ENV === 'development') {
      // eslint-disable-next-line no-console
      console.log('\uD83C\uDFAE XO_OX says: You found the secret! \uD83C\uDFAE');
    }

    // Cycle through all themes rapidly
    let cycleIndex = 0;
    if (konamiCycleRef.current) clearInterval(konamiCycleRef.current);
    if (konamiTimeoutRef.current) clearTimeout(konamiTimeoutRef.current);

    konamiCycleRef.current = setInterval(() => {
      setTheme(THEMES[cycleIndex % THEMES.length].id);
      cycleIndex++;
    }, 100);

    // After 2 seconds, stop and land on a random theme.
    // Track timeout in a ref so unmount can clear it.
    konamiTimeoutRef.current = setTimeout(() => {
      if (konamiCycleRef.current) clearInterval(konamiCycleRef.current);
      konamiCycleRef.current = null;
      konamiTimeoutRef.current = null;
      const randomTheme = THEMES[Math.floor(Math.random() * THEMES.length)];
      setTheme(randomTheme.id);
    }, 2000);
  }, [setTheme]);

  useKonamiCode(handleKonamiCode);

  // Cleanup konami interval + timeout
  useEffect(() => {
    return () => {
      if (konamiCycleRef.current) clearInterval(konamiCycleRef.current);
      if (konamiTimeoutRef.current) clearTimeout(konamiTimeoutRef.current);
    };
  }, []);

  // ---------------------------------------------------------------------------
  // Blink cycle
  // ---------------------------------------------------------------------------

  /**
   * Perform a single blink: eyes close for 150ms then reopen.
   * Skipped entirely when in dilated (processing) state.
   */
  const doBlink = useCallback(() => {
    if (isProcessing) return;
    setIsBlinking(true);
    blinkTimeoutRef.current = setTimeout(() => {
      setIsBlinking(false);
    }, 150);
  }, [isProcessing]);

  /**
   * Schedule the next random blink at a random interval between 3-8 seconds.
   */
  const scheduleNextBlink = useCallback(() => {
    const delay = 3000 + Math.random() * 5000; // 3-8s
    blinkIntervalRef.current = setTimeout(() => {
      doBlink();
      scheduleNextBlink();
    }, delay);
  }, [doBlink]);

  // Start the random blink cycle
  useEffect(() => {
    scheduleNextBlink();
    return () => {
      if (blinkIntervalRef.current) clearTimeout(blinkIntervalRef.current);
      if (blinkTimeoutRef.current) clearTimeout(blinkTimeoutRef.current);
    };
  }, [scheduleNextBlink]);

  /**
   * Acknowledgment blink on header click.
   */
  const handleHeaderClick = useCallback(() => {
    if (isProcessing) return;
    // Cancel any pending scheduled blink to avoid overlap
    if (blinkIntervalRef.current) clearTimeout(blinkIntervalRef.current);
    if (blinkTimeoutRef.current) clearTimeout(blinkTimeoutRef.current);
    // Quick acknowledgment blink
    setIsBlinking(true);
    blinkTimeoutRef.current = setTimeout(() => {
      setIsBlinking(false);
      // Resume the random blink cycle
      scheduleNextBlink();
    }, 150);
  }, [isProcessing, scheduleNextBlink]);

  const sectionLabel = activeSection ? SECTION_LABELS[activeSection] : null;

  return (
    <header
      className="h-14 glass-panel border-b border-border flex items-center justify-between px-4"
      onClick={handleHeaderClick}
      style={{ cursor: 'default', userSelect: 'none' }}
    >
      {/* Left: Logo + Brand + Status */}
      <div className="flex items-center gap-3">
        {/* Sentient Logo */}
        <div className="flex items-center gap-2">
          <div
            className="flex items-center justify-center rounded-lg"
            style={{
              width: 88,
              height: 32,
              background: 'rgba(var(--color-accent-teal, 13 148 136), 0.08)',
              cursor: 'pointer',
            }}
            role="button"
            tabIndex={0}
            aria-label="XO_OX logo"
            onClick={handleLogoClick}
            onKeyDown={(e) => { if (e.key === 'Enter' || e.key === ' ') { e.preventDefault(); handleLogoClick(e as unknown as React.MouseEvent); } }}
            title="XO_OX"
          >
            <SentientLogo isBlinking={isBlinking} logoState={logoState} />
          </div>
          <h1 className="text-base font-bold text-text-primary tracking-tight">
            XPM Creator
          </h1>
        </div>

        {/* Project name + section indicator */}
        {projectName && (
          <>
            <span className="text-text-muted">/</span>
            <span className="text-sm text-text-secondary">{projectName}</span>
          </>
        )}
        {sectionLabel && (
          <>
            <span className="text-text-muted">/</span>
            <span
              className="text-xs font-medium tracking-wide uppercase"
              style={{
                background: 'linear-gradient(90deg, rgb(var(--color-accent-teal, 13 148 136)), rgb(var(--color-accent-plum, 124 58 237)))',
                WebkitBackgroundClip: 'text',
                WebkitTextFillColor: 'transparent',
                backgroundClip: 'text',
              }}
            >
              {sectionLabel}
            </span>
          </>
        )}

        {/* Greeting (client-only to prevent SSR hydration mismatch) */}
        {greetingReady && greeting.text && (
          <div className="hidden md:flex items-center gap-1.5 ml-2">
            <span className="text-xs text-text-muted">
              {greeting.emoji} {greeting.text}
            </span>
          </div>
        )}

        {/* Status Indicators */}
        <div className="flex items-center gap-3 ml-3">
          {/* Audio Engine Status */}
          <div className="flex items-center gap-1.5">
            <span
              style={{
                width: 6,
                height: 6,
                borderRadius: '50%',
                backgroundColor: audioEngineActive
                  ? 'rgb(34, 197, 94)'
                  : 'rgb(156, 163, 175)',
                display: 'inline-block',
                boxShadow: audioEngineActive
                  ? '0 0 4px rgba(34, 197, 94, 0.5)'
                  : 'none',
                transition: 'background-color 300ms ease, box-shadow 300ms ease',
              }}
            />
            <span className="text-xs text-text-muted hidden sm:inline">Audio</span>
          </div>

          {/* Sample Count */}
          <div className="flex items-center gap-1.5">
            <span
              className="text-xs font-medium"
              style={{
                color: sampleCount > 0
                  ? 'rgb(var(--color-accent-teal, 13 148 136))'
                  : 'rgb(var(--color-text-muted, 156 163 175))',
                transition: 'color 300ms ease',
              }}
            >
              {sampleCount} {sampleCount === 1 ? 'sample' : 'samples'}
            </span>
          </div>

          {/* Export Ready */}
          <div className="flex items-center gap-1.5">
            <span
              style={{
                width: 6,
                height: 6,
                borderRadius: '50%',
                backgroundColor: hasAssignedPads
                  ? 'rgb(var(--color-accent-teal, 13 148 136))'
                  : 'rgb(156, 163, 175)',
                display: 'inline-block',
                boxShadow: hasAssignedPads
                  ? '0 0 4px rgba(var(--color-accent-teal, 13 148 136), 0.5)'
                  : 'none',
                transition: 'background-color 300ms ease, box-shadow 300ms ease',
              }}
            />
            <span className="text-xs text-text-muted hidden sm:inline">
              {hasAssignedPads ? 'Export Ready' : 'Pads Empty'}
            </span>
          </div>
        </div>
      </div>

      {/* Right: Theme + Actions */}
      <div className="flex items-center gap-2" onClick={(e) => e.stopPropagation()}>
        <ThemeSwitcher />
        {onNewProject && (
          <Button variant="secondary" size="sm" onClick={onNewProject}>
            New Project
          </Button>
        )}
        {onExport && (
          <Button variant="primary" size="sm" onClick={onExport}>
            Export
          </Button>
        )}
      </div>

      {/* About Toast Easter Egg */}
      {showAbout && (
        <div
          className="fixed top-16 left-1/2 -translate-x-1/2 z-50 animate-fadeInUp"
          style={{
            background: 'linear-gradient(135deg, rgba(var(--color-surface, 255 255 255), 0.97), rgba(var(--color-surface-alt, 245 245 245), 0.97))',
            border: '1px solid rgba(var(--color-accent-teal, 13 148 136), 0.3)',
            borderRadius: 12,
            padding: '16px 24px',
            boxShadow: '0 8px 32px rgba(0,0,0,0.15), 0 0 0 1px rgba(var(--color-accent-plum, 124 58 237), 0.1)',
            maxWidth: 360,
            backdropFilter: 'blur(8px)',
          }}
        >
          <p
            className="text-sm font-bold mb-1"
            style={{
              background: 'linear-gradient(90deg, rgb(var(--color-accent-teal, 13 148 136)), rgb(var(--color-accent-plum, 124 58 237)))',
              WebkitBackgroundClip: 'text',
              WebkitTextFillColor: 'transparent',
              backgroundClip: 'text',
            }}
          >
            XO_OX (zoe ox) v0.1
          </p>
          <p className="text-xs text-text-secondary leading-relaxed">
            Crafted with love for the MPC community
          </p>
          <p className="text-xs text-text-muted mt-1">
            10 Expression Modes &bull; 20 Themes &bull; 8-Layer Engine
          </p>
          <p className="text-xs text-text-muted mt-0.5 italic">
            Built to make world-class kits, easily.
          </p>
        </div>
      )}
    </header>
  );
}
