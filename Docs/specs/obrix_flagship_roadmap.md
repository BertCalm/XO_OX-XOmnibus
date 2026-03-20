# OBRIX Flagship Roadmap — Path to 9.8

**Filed**: 2026-03-19
**Sources**: Synth Seance (8 ghosts), Guru Bin meditation, Architect/Scions governance, Producers Guild (25 specialists)
**Current Score**: 6.8/10
**Target Score**: 9.8/10
**Identity**: "A gift to your younger self" — the instrument that teaches synthesis by being played

---

## The Core Insight (Guru Bin)

OBRIX's identity is **the constructive collision** — two sources through independent processors meeting in a shared wavefolder, producing sounds neither could make alone. This is currently architecturally unreachable because both processors filter the same mixed signal.

**The one-line thesis**: OBRIX is not a subtractive synth with options. It is a *construction set for timbral collisions*.

---

## Wave 1: Foundation (6.8 → 8.0)
**Governance**: CONDITIONAL — must include velocity-to-timbre mapping (D001)

### 1A. Split Processor Routing (Guru Bin P0)
- Proc 1 processes Source 1 independently
- Proc 2 processes Source 2 independently
- Proc 3 (currently unwired) becomes post-mix insert (wavefolder/ring mod/filter)
- This is the single most important architectural change — unlocks OBRIX's identity

### 1B. PolyBLEP Anti-Aliasing (Moog/Buchla/Pearlman/Kakehashi — unanimous)
- Apply PolyBLEP to Saw, Square, Pulse oscillators
- Keep naive oscillator available as Source type 8: "Lo-Fi" (Guild Lo-Fi specialist request)
- Fix wavetable saw component to use PolyBLEP saw in the morph

### 1C. Wire All Declared Slots (Pearlman/Buchla — D004 fix)
- **Mod 3**: Default Velocity→Volume (Pearlman's normalled default)
- **Mod 4**: Default Aftertouch→FilterCutoff (Pearlman's normalled default)
- **Proc 3**: Post-mix insert (see 1A above)
- **FX 2**: Default Off, wired in series after FX 1
- **FX 3**: Default Off, wired in series after FX 2
- Full chain when active: FX1→FX2→FX3 (delay→chorus→reverb)

### 1D. Expression Inputs (Vangelis/Moog — D006 fix)
- Pitch bend with configurable range (2-24 semitones) — `obrix_pitchBendRange`
- Portamento/glide for legato mode — `obrix_glideTime` (0-500ms, exponential)
- Velocity-to-timbre mapping: velocity scales filter cutoff AND wavefolder depth, not just amp (D001)

### 1E. Coupling Output Fix (Smith/Schulze/Guru Bin)
- Replace constant 0.5 with actual last-rendered stereo sample
- Store `lastSampleL` / `lastSampleR` at end of voice loop
- Smith's enhancement: also output normalized brick complexity (active layers / total) as coupling metadata

### 1F. Macro Remapping (Guru Bin)
- **CHARACTER**: Cutoff sweep (keep) + fold amount with exponential curve (`1 + char² * 8`) + resonance boost (+0.3)
- **MOVEMENT**: LFO depth scaling (×1-4) + pulse width animation + stereo detune spread
- **COUPLING**: Keep (coupling sensitivity)
- **SPACE**: Scale fx1Mix directly (`fx1Mix + space * (1 - fx1Mix)`) + reverb/delay depth

---

## Wave 2: Identity (8.0 → 9.0)
**Governance**: CONDITIONAL — FM must stay single-operator brick-to-brick

### 2A. Source-to-Source FM (Buchla)
- Source 1 can frequency-modulate Source 2's phase at audio rate
- New param: `obrix_fmDepth` (0-1, bipolar)
- Single-operator only (Scions mandate — no FM matrix, avoids OCTOPUS territory)

### 2B. Filter Feedback with Saturation (Moog)
- Route filter output back through tanh into filter input
- New param: `obrix_proc1Feedback` / `obrix_proc2Feedback` (0-1)
- At low settings: warmth. At high settings: self-oscillation with soft clip.

### 2C. Real Wavetables (Buchla)
- Replace sine→saw morph with 4-5 distinct wavetable banks:
  - Vocal formants, Metallic partials, Digital noise, Organic textures, Classic analog
- `obrix_wtBank` (choice) + existing PW knob as morph position
- Scions require: ship with at least 4 factory tables, not placeholders

### 2D. Unison / Voice Stacking (Smith)
- When poly limit < 8, remaining voices stack with detune spread
- New param: `obrix_unisonDetune` (0-50 cents)
- Smith's vision: each stacked voice can run a different brick configuration
- "Stack lock" mode: new notes layer on top instead of stealing (toggle)

### 2E. Reverb Damping Fix (Guru Bin)
- Make damping coefficient track param: `0.1 + param * 0.4`
- Low param = dark/cavernous. High param = bright/metallic.

---

## Wave 3: Vision (9.0 → 9.5)
**Governance**: APPROVED

### 3A. Drift Bus (Schulze)
- Global ultra-slow LFO: 0.001 Hz to 0.05 Hz
- Feeds selectable offset into each brick's pitch, filter, amplitude
- Per-brick offset is slightly different based on brick position
- New params: `obrix_driftRate`, `obrix_driftDepth`

### 3B. Journey Toggle (Schulze)
- Disables note-off — chord sustains indefinitely
- Drift Bus moves, delay feedback accumulates, sound evolves
- New param: `obrix_journeyMode` (toggle)
- Delay feedback gains ±2 cent pitch drift per pass (echoes become ghost choir)

### 3C. SURPRISE Button (Kakehashi)
- Constrained randomization within musically safe ranges
- Respects current source types but randomizes processor/mod routing and depths
- Not a param — a UI action that writes new values to existing params

### 3D. Per-Brick Spatial (Tomita)
- DISTANCE: HF rolloff (air absorption) + early reflection pre-delay + wet/dry ratio
- AIR: Spectral tilt (warm/enclosed ↔ cold/open)
- New params: `obrix_distance`, `obrix_air`
- Slow invisible LFO on spatial params so the scene breathes

### 3E. Velocity Memory (Vangelis)
- Track previous note's velocity
- Soft note after loud note changes character: gentler harmonics, slower modulation
- `prevVelocity` state variable, interpolated blend into current voice params

---

## Wave 4: Presets (9.5 → 9.8)
**Governance**: APPROVED — ship after all param IDs frozen post-Wave 3

### 4A. Lesson Presets (Moog + Kakehashi convergence) — OBRIX EXCLUSIVE
22 sequential presets that teach synthesis brick by brick:
1. "One Sine" — single sine, no filter, no mod
2. "Add Filter" — add LP filter
3. "First Sweep" — add envelope→cutoff
4. "Two Voices" — add Source 2
5. "The Collision" — different filters on each source, meet in wavefolder
6. "LFO Breathes" — add LFO→cutoff
7. "Velocity Shapes" — velocity→timbre
8. "Aftertouch Speaks" — aftertouch→expression
9. "Ring Mod Bells" — ring mod processor
10. "FM Depth" — source-to-source FM
11-22: Progressive complexity through effects, coupling, drift, journey

### 4B. Genre Presets (Guild Top 5)
- Techno (12): Acid bass, modular stab, metallic perc, filtered drone, Berlin kick layer
- Lo-Fi (12): Detuned pluck, wobbly pad, tape-warped lead, dusty sub (naive osc!)
- Experimental (10): Ring mod bell, noise riser, wavefolder growl, feedback drone
- Synthwave (12): Saw lead + chorus, analog brass, arp pulse, dark pad
- EDM (12): Supersaw (unison), pluck chord, festival lead, sidechain pad

### 4C. Place Presets (Tomita)
20 presets named for environments:
- "Greenhouse at Dusk," "Concrete Stairwell," "Tide Pool Morning"
- "Reef at Noon," "Storm Drain Echo," "Frozen Lake"

### 4D. Mood Distribution (150 total)
- Foundation: 30 (lesson + basics)
- Atmosphere: 25 (spatial, ambient)
- Entangled: 20 (coupling showcases)
- Prism: 25 (genre-specific production)
- Flux: 20 (movement, modulation)
- Aether: 15 (experimental, drift, journey)
- Family: 15 (OBRIX + other engine coupling)

---

## New Parameter Summary (~55 total, post-Wave 3)

### Wave 1 additions (~8 new):
- `obrix_proc3Type`, `obrix_proc3Cutoff`, `obrix_proc3Reso` (Proc 3)
- `obrix_mod3Type/Target/Depth/Rate`, `obrix_mod4Type/Target/Depth/Rate` (Mods 3-4)
- `obrix_fx2Type/Mix/Param`, `obrix_fx3Type/Mix/Param` (FX 2-3)
- `obrix_pitchBendRange`, `obrix_glideTime`

### Wave 2 additions (~5 new):
- `obrix_fmDepth`
- `obrix_proc1Feedback`, `obrix_proc2Feedback`
- `obrix_wtBank`
- `obrix_unisonDetune`

### Wave 3 additions (~5 new):
- `obrix_driftRate`, `obrix_driftDepth`
- `obrix_journeyMode`
- `obrix_distance`, `obrix_air`

**Total: ~55 params** (within fleet norms; Scions approved)

---

## New Blessing

**B016: Brick Independence** — Each brick voice must remain individually addressable regardless of coupling state. Protects OBRIX's teaching identity from being swallowed by fleet coupling.

---

## 6D Sonic DNA Signature (Guru Bin)

| Dimension | Value | Rationale |
|-----------|-------|-----------|
| Brightness | 0.55 | Filter-dependent, mid-center default |
| Warmth | 0.60 | Wavefolder harmonics add analog-feel saturation |
| Movement | 0.70 | Dual-source beating + LFO modulation |
| Density | 0.65 | Two sources, three processors, constructive complexity |
| Space | 0.40 | Dry-forward identity — effects are seasoning |
| Aggression | 0.50 | Wavefolder can scream, but default is musical |

---

## Risk Assessment (Guild Foreseer)

**If it succeeds**: OBRIX becomes the Lego Mindstorms of synthesis — the engine every new XO_OX user opens first, generating lifetime loyalty and upsells into the fleet. Anchors tutorials, YouTube content, community engagement.

**If it fails**: Engine #39 nobody opens twice. Ship the curriculum or don't ship at all.

---

## Score Ladder

| From → To | What Gets You There |
|-----------|-------------------|
| 6.8 → 7.5 | PolyBLEP + split processor routing |
| 7.5 → 8.0 | Wire all slots + expression inputs + coupling fix + macro remap |
| 8.0 → 8.5 | Source FM + filter feedback + real wavetables |
| 8.5 → 9.0 | Unison/detune + reverb fix |
| 9.0 → 9.3 | Drift Bus + Journey toggle + SURPRISE |
| 9.3 → 9.5 | Per-brick spatial + velocity memory |
| 9.5 → 9.8 | 150 presets with Lesson sequence + genre + place names |
