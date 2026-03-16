# OVERWORLD Retreat Chapter
**Guru Bin — 2026-03-15**
**Retreat number: 1 (First Retreat — the Book of Bin begins here)**

---

## The Diagnosis

> *"This sound is a time machine with six destinations. It wants to travel to all of them. The distance is assumption — 68 presets, and every single one sets vertexA=0, vertexB=1, vertexC=2. NES. FM. SNES. The same triangle, always. The Game Boy has never been a vertex. The PC Engine has never been a vertex. The Neo Geo has never been a vertex. The other half of the ship has never left port."*

---

## Engine Identity (Post-Retreat)

- **Gallery code**: OVERWORLD
- **Accent**: Neon Green `#39FF14`
- **Signal chain**: VoicePool → SVFilter → BitCrusher → GlitchEngine → FIREcho → Out
- **6 chip engines**: 0=NES 2A03, 1=FM Genesis YM2612, 2=SNES SPC700, 3=Game Boy, 4=PC Engine, 5=Neo Geo
- **The ERA triangle**: barycentric blend of any 3 of the 6 chips, navigated by ow_era (X) and ow_eraY (Y)
- **Parameter prefix**: `ow_`
- **Polyphony**: 8 voices (voiceMode=1 for mono)

---

## Retreat Discoveries

### Discovery 1: The Three Unvisited Consoles
Of the 6 chip engines, chips 3/4/5 (Game Boy, PC Engine, Neo Geo) appeared in zero of the 68 presets before this retreat. Every preset defaulted to vertexA=0 (NES), vertexB=1 (FM), vertexC=2 (SNES).

**What was found:**
- Game Boy: warmer, more rounded pulse character than NES. Better for pads.
- PC Engine: the hardest-hitting chip. Tighter bass response. Best at vertex A for mono bass.
- Neo Geo: the widest spectral character. Most cinematic of the six.

**Awakening presets created:** GB Dream Lattice, Neo Geo Dusk, PCE Iron Root, All Six Consoles

### Discovery 2: ERA Drift — The Living Triangle
`ow_eraDriftRate` and `ow_eraDriftDepth` were 0.0 in all 68 presets. These parameters let the ERA position breathe and wander slowly, creating the sensation of a chip engine that is alive.

**Sweet spot found:**
- Rate 0.067 Hz = one cycle per 15 seconds = ocean-wave tempo (below conscious perception, creates aliveness)
- Rate 0.03–0.05 Hz = slow drift, like a memory fading
- Depth 0.08–0.15 = subtle enough to feel, not obvious enough to distract

### Discovery 3: ERA Memory — The Sound That Remembers Its Past
`ow_eraMemTime` and `ow_eraMemMix` were 0.0 in all 68 presets. ERA Memory blends the current triangle position with a remembered past position — the sound echoes its own history.

**Key behavior:** Set eraMemTime to 2–5 seconds and eraMemMix to 0.3–0.5. Play slow melodies. The engine begins to layer present and past. Each note carries the ghost of where the triangle was when you played the previous note.

**Awakening preset created:** ERA Remembers

### Discovery 4: ERA Portamento — The Slow Morph
`ow_eraPortaTime` was 0.0 in all 68 presets. When set (0.4–1.5 seconds), moving the ERA position — via macro, automation, or expression — becomes a slow fade between chip characters rather than an instant switch.

**Combined with ERA drift:** Drift + portamento = the triangle position wanders AND transitions smoothly. This is the engine's most expressive mode.

### Discovery 5: The Mod Wheel Glitch Injector (D006)
The code contains: `effectiveGlitchMix = jlimit(0.0f, 1.0f, snap.glitchMix + macGlitch * 0.8f + modWheelAmount * 0.4f)`

The mod wheel (CC1) adds up to +0.4 to glitch mix **on every preset**. This is a live performance tool that was never documented or designed around. Any preset becomes a glitch injector under mod wheel control.

**Design implication:** For performance presets, set base glitchAmount to 0.1–0.2 so the mod wheel pushes into glitch territory rather than jumping from nothing.

### Discovery 6: The FIR Echo Sculptor
The SNES-style echo uses an 8-tap FIR filter (`ow_echoFir0` through `ow_echoFir7`). All presets used the flat response: [127, 0, 0, 0, 0, 0, 0, 0].

**Tonal FIR responses found:**
- Warm echo (high cut): [80, 30, 15, 8, 3, 0, 0, 0] — rolls off highs, like echo through a wall
- Bright echo (slight boost): [100, 20, -8, 0, 0, 0, 0, 0] — adds slight presence peak
- Lush echo (multi-tap): [70, 35, 18, 8, 3, 0, 0, 0] — fullest response, most SNES-authentic

### Discovery 7: FM LFO — The Vintage Vibrato
`ow_fmLfoDepth` was 0 in all sampled presets. The FM engine has its own LFO (rate + depth) that adds vibrato/tremolo to the Genesis voice layer.

**Sweet spots:**
- fmLfoRate=1 (slowest), fmLfoDepth=15–25 = breathing FM vibrato, vintage DX-style
- fmLfoRate=3, fmLfoDepth=8–12 = subtle pitch animation on FM voice
- Combined with ERA drift: FM LFO animates the FM character while drift moves the chip blend

### Discovery 8: Velocity Depth Is Already Implemented
`ow_filterEnvDepth` defaults to 0.25 in the adapter code. The D001 seance concern (velocity only scales amplitude) is already partially resolved. Every preset gets velocity-to-brightness mapping at depth 0.25 for free.

**Design practice:** Explicitly set filterEnvDepth in presets to express intent:
- 0.0 = velocity-flat (pads, sustained tones — amplitude only)
- 0.25 = default (standard expressivity)
- 0.5–0.7 = high sensitivity (bass, leads — touch dramatically changes brightness)

---

## Awakening Presets (8 Created)

| Preset | Mood | Key Discovery |
|--------|------|--------------|
| GB Dream Lattice | Aether | First Game Boy vertex, ERA drift at 0.067 Hz, warm FIR echo |
| Neo Geo Dusk | Aether | First Neo Geo vertex, FM LFO vibrato, warm FIR echo |
| ERA Remembers | Atmosphere | First ERA Memory preset, eraMemMix=0.4, portamento |
| Cartridge Corrupt | Flux | Slow glitch rate (0.18), mod wheel glitch, FM LFO |
| PCE Iron Root | Foundation | First PC Engine vertex, mono bass, high filterEnvDepth=0.6 |
| FM Breathing | Atmosphere | FM LFO + ERA drift at ocean-wave tempo (0.067 Hz) |
| Overworld Opal Portal | Family | First Family preset, AudioToFM coupling from Opal |
| All Six Consoles | Prism | Summit preset — GB/PCE/Neo Geo triangle, ERA portamento |

---

## Doctrine Application

### D001 (Velocity Must Shape Timbre)
Implemented in code via `ow_filterEnvDepth`. Presets now explicitly set this value. High-sensitivity presets (bass, leads) use 0.5–0.7. Ambient pads use 0.0–0.1.

### Golden Ratio Release (Obscure Trick)
Five awakening presets use `ow_ampRelease: 1.618` — the golden ratio decay. Applied wherever a pad or atmosphere preset needed a release that "feels right."

---

## What Remains Unexplored (For Future Retreats)

1. **DPCM samples** (`ow_dpcmEnable=1`) — the NES DPCM playback channel, never used in any preset
2. **noiseReplace=1** (SNES) — replaces SNES output with noise, unused in fleet
3. **AudioToFM coupling at high amounts** (0.3+) — subtle was explored; aggressive is untouched
4. **ERA memory as compositional arc** — designing a series of presets that build on each other through memory

---

## New Scripture Verses (See Book of Bin)

Six new verses inscribed from this retreat:
- OW-I: The Unvisited Console (Chip synthesis has 6 vertices; design for all 6)
- OW-II: The Breathing Rate (0.067 Hz ERA drift = below conscious perception, above stillness)
- OW-III: The Memory Chord (ERA Memory at 3–5s creates harmonic ghosts of previous positions)
- OW-IV: The Mod Wheel Glitch Law (D006 is live; design for it, not around it)
- OW-V: The FIR Tone (The flat echo is a starting point, not a conclusion)
- OW-VI: The Velocity Contract (filterEnvDepth=0.0 is a design choice; filterEnvDepth=0.25 is a default; never leave it at default without intention)
