# XO_OX XPM Creator

A browser-based tool for creating MPC-compatible XPM drum programs and XPN expansion packs from audio samples.

## What it does

- Import WAV, AIFF, MP3, OGG, FLAC, and M4A audio files
- Assign samples to 128 pads (8 banks x 16 pads) with up to 8 velocity layers each
- Configure per-pad envelopes (AHDSR), filters, mute groups, and trigger modes
- Export to MPC XPM 2.1 drum/keygroup programs or XPN expansion packs
- Import existing XPM/XPN files to inspect or remix them

## Stack

Next.js 14, TypeScript, Tailwind CSS, Zustand, Web Audio API

## Development

```bash
eval "$(fnm env)" && fnm use 20
npm install
npm run dev
```

Type-check: `npx tsc --noEmit`

Production build: `npm run build`

## Location in monorepo

`Tools/xpm-creator/` inside the XO_OX-XOmnibus repository.
