# audio-xpm-creator

## Environment
- `eval "$(fnm env)" && fnm use 20` — required before any npm/npx commands (fnm, not nvm)
- `npx tsc --noEmit` — type-check without emitting (no test suite yet)
- Next.js 14 app with `'use client'` components, Tailwind CSS, Zustand stores

## Architecture
- **Stores** (`src/stores/`): Zustand with `create<State>((set, get) => ({...}))` pattern
- **Audio** (`src/lib/audio/`): Web Audio API — AudioContext, OfflineAudioContext, BiquadFilterNode
- **Persistence**: IndexedDB via `src/lib/storage/db.ts` + `src/lib/storage/projectManager.ts`; auto-save/restore via hooks
- **Undo/Redo**: `src/stores/historyStore.ts` — snapshot-based state history

## Critical Patterns (bugs found when violated)

### Zustand Store Mutations
- After any `await`, re-read state: `const { pads } = usePadStore.getState()` — closures go stale
- Inside `setTimeout`/debounce callbacks, ALWAYS re-read live state via `getState()` — never use closed-over selector values
- Return new objects from `set()`: `set(state => ({ pads: [...state.pads] }))` — never mutate in-place
- Per-item change tracking in `.map()`: use `let itemChanged = false` inside the map callback, not a shared outer variable
- No-op `set()` returns should be `{}` not `state` — returning full state defeats Zustand's identity optimization
- Never call `usePadStore.setState()` directly from components — use store actions to maintain history integration

### Audio / DSP
- `invalidateCache(sampleId)` after ANY operation that mutates a sample's audio buffer
- `dirtySampleIds` in audioStore must be marked for ALL metadata changes (rename, tags, favorite) not just buffer changes
- Always pass `sampleRate` — never hardcode 44100; users may have 48kHz/96kHz audio interfaces
- Derive `OfflineAudioContext` sampleRate from source buffers or live `AudioContext`, not hardcoded
- Bipolar modulation: use `!== 0` not `> 0` — negative envelope amounts sweep downward
- Use `?? 0` not `|| 0` when indexing Float32Array — `0.0` is a valid sample value
- IIR filter coefficients: use `exp(-2*PI*fc/sr)` (matched-Z), not `w/(w+1)` (Euler approximation)
- Use `cancelAndHoldAtTime` when interrupting envelopes — `cancelScheduledValues` + `param.value` causes clicks
- Offline render envelopes must match live engine (exponential vs linear) or exports will sound different

### Undo/Redo
- Synchronous mutations: `withHistory('description', () => { /* sync mutation */ })`
- Async operations: `history.snapshot(pads)` before await, then `history.pushState('desc', pads)` after
- Deep-clone snapshots before storing — live references can be clobbered by concurrent mutations
- `computeDiffs` must handle added/removed pads (array length changes), not just modified indices
- UI undo/redo handlers must read live state via `getState()`, not closed-over selector values

### Persistence
- `removeSample` must delete from IndexedDB too — otherwise deleted samples accumulate forever
- Kit import must clear envelopeStore/modulationStore before loading — prevents cross-kit contamination
- When importing from XPN expansions, filter samples to only those referenced by the selected program

### Error Handling
- All user-facing async operations must show toast on failure via `useToastStore.getState().addToast()`
- Never pass raw `async` functions to `onClick` — wrap in try-catch handler
- Guard `localStorage` access with try-catch (fails in private browsing / iframe)
- On restore failure: clear stale IDs from localStorage to prevent retry loops

### XML/File Parsing
- Runtime-validate parsed strings before `as` type assertions — external files may have unknown values
- Use validation sets: `const VALID_VALUES = new Set(['A', 'B'])` + `VALID_VALUES.has(raw)`

## QA Agent Categories
When running parallel QA agents, use these 4 specializations for best coverage:
1. **Deep Logic Hunter** — race conditions, data integrity, off-by-one, memory leaks
2. **Silent Failure Hunter** — swallowed errors, missing toasts, fire-and-forget async
3. **Audio DSP Specialist** — Web Audio API lifecycle, sample rate math, envelope correctness
4. **State & Persistence** — Zustand mutations, undo/redo edge cases, IndexedDB consistency

Run iteratively: fix findings -> re-scan -> repeat until agents struggle to find issues.
