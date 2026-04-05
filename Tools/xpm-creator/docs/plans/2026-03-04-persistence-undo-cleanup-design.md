# Persistence, Undo/Redo, and Dead Code Cleanup — Design

**Date**: 2026-03-04
**Status**: Approved

## 1. Persistence & Auto-Restore

### Problem

Auto-save writes pads + samples to IndexedDB, but:
- `envelopeStore`, `modulationStore`, `expressionStore` are never persisted
- `tags`, `isFavorite`, `waveformPeaks` are missing from the DB samples schema
- `loadProject()` exists but is never called — state is lost on page reload

### Design

**DB Schema v2** — bump `DB_VERSION` from 1 → 2 with migration:
- Add `tags` (string JSON), `isFavorite` (boolean), `waveformPeaks` (string JSON) to the `samples` object store
- Add new `storeSnapshots` object store: `key = string` (`"envelopes"`, `"modulations"`, `"expressions"`), `value = { id: string, data: string }`

**`useAutoRestore` hook** — runs once on mount (before `useAutoSave`):
- Reads `lastProjectId` from `localStorage`
- Calls `loadProject(lastProjectId)` → project + samples from IndexedDB
- Reads `storeSnapshots` for envelopes/modulations/expressions
- Hydrates all stores: `audioStore.setSamples()`, `padStore.setPads()`, `envelopeStore` bulk, `modulationStore` bulk, `expressionStore` bulk
- Sets `projectStore.setCurrentProject()`
- Shows loading state during restore

**`useAutoSave` expansion**:
- Persist `tags`, `isFavorite`, `waveformPeaks` when saving samples
- Persist envelope/modulation/expression snapshots to `storeSnapshots` table
- Write `lastProjectId` to `localStorage` on every save

**`projectManager.loadProject` update**:
- Map `tags`, `isFavorite`, `waveformPeaks` back onto restored `AudioSample` objects

## 2. Undo/Redo Wiring

### Problem

`historyStore` is fully implemented (diff-based, 50-entry cap) but only `CycleEnginePanel` calls `snapshot()` / `pushState()`. The other ~15 pad-mutating components skip history.

### Design

**`padStore.withHistory(description, fn)` wrapper**:
- Calls `historyStore.snapshot(pads)` → runs `fn()` → calls `historyStore.pushState(description, pads)`
- All pad-mutating components wrap their calls: `padStore.withHistory('Assign sample', () => padStore.assignSampleToLayer(...))`
- `CycleEnginePanel` migrated to use the same wrapper
- No changes needed to `UndoTimeline`, `useHotkeys`, or `historyStore` itself

**Components to wire** (~15):
- `PadGrid.tsx` — drop sample, paste, clear, swap
- `PadLayerEditor.tsx` — layer parameter changes
- `LayerAssignment.tsx` — sample assignment
- `DrumKitBuilder.tsx` — auto-assign all pads
- `VibeCheckButton.tsx` — randomize pads
- `CycleEnginePanel.tsx` — 8-layer build (migrate existing calls)
- `SpaceFoldPanel.tsx` — stereo width
- `SpectralAirPanel.tsx` — spectral injection
- `SubTailPanel.tsx` — sub/tail processing
- `AutoLayerGenerator.tsx` — auto-generate layers
- `VelocityCurveSelector.tsx` — velocity curve changes
- `KeygroupBuilder.tsx` — keygroup pad setup
- `HumanizeControls.tsx` — humanize settings
- `WaveformEditor.tsx` — chop-to-pads
- `KitImporter.tsx` — load kit into pads

## 3. Dead Code Cleanup

### Remove `audioStore.chopRegions`
- State field, 4 actions (`addChopRegion`, `removeChopRegion`, `updateChopRegion`, `clearChopRegions`)
- `ChopRegion` import from audioStore (keep in `types/index.ts` — WaveformEditor uses it locally)

### Clean unused imports (54 ESLint warnings)
- `createOfflineContext` in cycleEngine.ts, ghostNoteGenerator.ts, keygroupBuilder.ts, pitchShifter.ts, sculptingProcessors.ts
- Unused types: `AudioSample`, `PadAssignment` in projectStore.ts
- Unused vars: `hopSize`, `crossfadeSamples`, `width`, `_lowMid`, `sourceDuration`, `DEFAULT_PITCH_ENV`
- Unused components: `ThemeSwatch`, `useState` in PitchSettings.tsx
- Unused format helpers: `fmt`, `MPC_PAD_NOTE_MAP_DEFAULT`, `MPC_PAD_GROUP_MAP_DEFAULT` in xpmKeygroupGenerator.ts
- Unused types: `ParsedInstrument`, `XPM_DEFAULTS` in kitUpcycler.ts, `XpmFile` in xpmGenerator.ts
