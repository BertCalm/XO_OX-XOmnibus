'use client';

import React, { useRef, useEffect } from 'react';
import Slider from '@/components/ui/Slider';
import type { InstrumentEnvelope } from '@/types';

interface EnvelopeEditorProps {
  label: string;
  envelope: InstrumentEnvelope;
  onChange: (envelope: InstrumentEnvelope) => void;
}

export default function EnvelopeEditor({ label, envelope, onChange }: EnvelopeEditorProps) {
  const canvasRef = useRef<HTMLCanvasElement>(null);

  const update = (key: keyof InstrumentEnvelope, value: number) => {
    onChange({ ...envelope, [key]: value });
  };

  // Draw AHDSR curve on canvas via useEffect — callback refs fire during
  // React's commit phase before browser layout, so getBoundingClientRect()
  // returns {width: 0, height: 0} and the curve is invisible on first mount.
  useEffect(() => {
    const canvas = canvasRef.current;
    if (!canvas) return;
    const ctx = canvas.getContext('2d');
    if (!ctx) return;

    const dpr = window.devicePixelRatio || 1;
    const rect = canvas.getBoundingClientRect();
    // Guard: skip drawing if the canvas hasn't been laid out yet
    if (rect.width === 0 || rect.height === 0) return;
    canvas.width = rect.width * dpr;
    canvas.height = rect.height * dpr;
    ctx.scale(dpr, dpr);

    const w = rect.width;
    const h = rect.height;
    const pad = 4;

    ctx.clearRect(0, 0, w, h);

    // Read theme colors from CSS custom properties
    const cs = getComputedStyle(document.documentElement);
    const surfaceBg = cs.getPropertyValue('--color-surface-bg').trim();
    const accentTeal = cs.getPropertyValue('--color-accent-teal').trim();
    const accentPlum = cs.getPropertyValue('--color-accent-plum').trim();
    const tealHex = surfaceBg ? `rgb(${accentTeal})` : '#0D9488';
    const plumHex = surfaceBg ? `rgb(${accentPlum})` : '#7C3AED';
    const bgHex = surfaceBg ? `rgb(${surfaceBg})` : '#FAFAFA';

    // Background
    ctx.fillStyle = bgHex;
    ctx.fillRect(0, 0, w, h);

    // Calculate ADSR points
    const totalTime = Math.max(
      envelope.attack + envelope.hold + envelope.decay + envelope.release + 0.1,
      0.5
    );

    const timeToX = (t: number) => pad + (t / totalTime) * (w - pad * 2);
    const valToY = (v: number) => h - pad - v * (h - pad * 2);

    const attackEnd = envelope.attack;
    const holdEnd = attackEnd + envelope.hold;
    const decayEnd = holdEnd + envelope.decay;
    const sustainDuration = totalTime - decayEnd - envelope.release;
    const sustainEnd = decayEnd + Math.max(sustainDuration, 0.05);
    const releaseEnd = sustainEnd + envelope.release;

    // Draw curve
    const gradient = ctx.createLinearGradient(0, 0, w, 0);
    gradient.addColorStop(0, tealHex);
    gradient.addColorStop(1, plumHex);
    ctx.strokeStyle = gradient;
    ctx.lineWidth = 2;
    ctx.lineJoin = 'round';
    ctx.lineCap = 'round';

    ctx.beginPath();
    ctx.moveTo(timeToX(0), valToY(0));
    ctx.lineTo(timeToX(attackEnd), valToY(1)); // Attack (linear)
    ctx.lineTo(timeToX(holdEnd), valToY(1)); // Hold (flat)

    // Decay: exponential ramp from peak (1.0) down to sustain level
    const sustainLevel = envelope.sustain;
    const attackEndX = timeToX(holdEnd);
    const decayEndX = timeToX(decayEnd);
    const decayPoints = 20;
    for (let i = 1; i <= decayPoints; i++) {
      const t = i / decayPoints;
      const expValue = sustainLevel + (1 - sustainLevel) * Math.exp(-5 * t);
      const px = attackEndX + (decayEndX - attackEndX) * t;
      const py = valToY(expValue);
      ctx.lineTo(px, py);
    }

    ctx.lineTo(timeToX(sustainEnd), valToY(sustainLevel)); // Sustain (flat)

    // Release: exponential ramp from sustain level down to zero
    const releaseStartX = timeToX(sustainEnd);
    const releaseEndX = timeToX(releaseEnd);
    const releasePoints = 20;
    for (let i = 1; i <= releasePoints; i++) {
      const t = i / releasePoints;
      const expValue = sustainLevel * Math.exp(-5 * t);
      const px = releaseStartX + (releaseEndX - releaseStartX) * t;
      const py = valToY(expValue);
      ctx.lineTo(px, py);
    }

    ctx.stroke();

    // Fill under curve
    ctx.globalAlpha = 0.08;
    ctx.fillStyle = tealHex;
    ctx.lineTo(timeToX(releaseEnd), valToY(0));
    ctx.lineTo(timeToX(0), valToY(0));
    ctx.fill();
    ctx.globalAlpha = 1;

    // Draw dots at control points
    const points = [
      { x: timeToX(0), y: valToY(0) },
      { x: timeToX(attackEnd), y: valToY(1) },
      { x: timeToX(holdEnd), y: valToY(1) },
      { x: timeToX(decayEnd), y: valToY(envelope.sustain) },
      { x: timeToX(sustainEnd), y: valToY(envelope.sustain) },
      { x: timeToX(releaseEnd), y: valToY(0) },
    ];

    for (const point of points) {
      ctx.fillStyle = '#FFFFFF';
      ctx.strokeStyle = tealHex;
      ctx.lineWidth = 1.5;
      ctx.beginPath();
      ctx.arc(point.x, point.y, 3, 0, Math.PI * 2);
      ctx.fill();
      ctx.stroke();
    }
  }, [envelope]);

  return (
    <div className="space-y-2">
      <span className="label">{label}</span>

      <canvas
        ref={canvasRef}
        role="img"
        aria-label={`AHDSR envelope: Attack ${envelope.attack}s, Hold ${envelope.hold}s, Decay ${envelope.decay}s, Sustain ${Math.round(envelope.sustain * 100)}%, Release ${envelope.release}s`}
        className="w-full h-16 rounded-lg border border-border"
      />

      <div className="grid grid-cols-2 gap-2">
        <Slider
          label="Attack"
          value={envelope.attack}
          onChange={(v) => update('attack', v)}
          min={0}
          max={2}
          step={0.01}
          unit="s"
        />
        <Slider
          label="Hold"
          value={envelope.hold}
          onChange={(v) => update('hold', v)}
          min={0}
          max={2}
          step={0.01}
          unit="s"
        />
        <Slider
          label="Decay"
          value={envelope.decay}
          onChange={(v) => update('decay', v)}
          min={0}
          max={2}
          step={0.01}
          unit="s"
        />
        <Slider
          label="Sustain"
          value={envelope.sustain}
          onChange={(v) => update('sustain', v)}
          min={0}
          max={1}
          step={0.01}
        />
        <Slider
          label="Release"
          value={envelope.release}
          onChange={(v) => update('release', v)}
          min={0}
          max={5}
          step={0.01}
          unit="s"
        />
      </div>
    </div>
  );
}
