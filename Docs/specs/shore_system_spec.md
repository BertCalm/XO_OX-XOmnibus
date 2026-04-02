# ShoreSystem — Formal Specification

**Blessed: 2026-03-14 | Engines: OSPREY + OSTERIA | Blessing: B012**
**Source header: `Source/DSP/ShoreSystem/ShoreSystem.h`**

---

## 1. What the ShoreSystem Is

The ShoreSystem is a shared, read-only cultural data model consumed simultaneously by two engines: OSPREY and OSTERIA. It encodes five coastal regions of the world — Atlantic, Nordic, Mediterranean, Pacific, Southern — as concrete synthesis parameters. Neither engine owns the data; both read from the same constexpr tables.

The key word is *shared*. When a user moves the Shore knob, both engines respond to the same cultural context at the same moment. A user does not configure OSPREY for "Mediterranean" and OSTERIA separately for something else. They move one knob and both engines align: the resonators shift toward Bouzouki and Oud spectra, the tavern room becomes a tile-floored open-air rembetika den, the rhythm voice takes on the Darbuka's swing feel, and the ocean's swell period shortens to the choppiness of a landlocked sea.

This is ensemble behavior without explicit coupling. The engines do not send signals to each other. They share a coordinate space.

---

## 2. The Five Coastlines

The shore parameter is a continuous float `[0.0, 4.0]`. Integer values represent pure coastlines; fractional values interpolate between adjacent ones.

| Index | Name | Geography | Musical Identity |
|-------|------|-----------|-----------------|
| 0.0 | Atlantic | Portugal / West Africa / Ireland | Guitarra portuguesa, Kora, Uilleann pipes — oceanic folk with long ring-out |
| 1.0 | Nordic | Scandinavia / Iceland | Hardingfele, Langspil, Kulning — sparse, deep, high brightness, long decay |
| 2.0 | Mediterranean | Greece / Turkey / Egypt | Bouzouki, Oud, Ney — warm inharmonic attack, short bright decay |
| 3.0 | Pacific | Japan / Polynesia / Tibet | Koto, Conch, Singing Bowl — minimal transients, extreme ring, high inharmonicity |
| 4.0 | Southern | Brazil / Madagascar / Indonesia | Cavaquinho, Valiha, Gamelan — warm and inharmonic, rich sustain |

None of these are simulations of those instruments. They are spectral fingerprints — formant center frequencies, bandwidths, and gain ratios derived from those instruments' acoustics, applied to a modal resonator bank. Atlantic sounds vaguely like the North Atlantic coast. Pacific sounds vaguely like a Japanese harbor at dusk. Mediterranean has the chop.

---

## 3. Data Architecture

The ShoreSystem is a single header file with five data layers, all `constexpr` static arrays. There is no runtime state, no allocation, no mutation. Thread safety is guaranteed by construction.

### 3.1 ResonatorProfile — Spectral fingerprint of a folk instrument

```cpp
struct ResonatorProfile {
    const char* name;
    float formantFreqs[4];      // Hz — 4 formant center frequencies
    float formantBandwidths[4]; // Hz — formant 3dB bandwidth
    float formantGains[4];      // linear 0-1 — relative formant energy
    float attackMs;             // typical attack time
    float decayMs;              // typical ring-out time
    float brightness;           // 0-1 high-partial emphasis
    float inharmonicity;        // 0-1 deviation from harmonic series
};
```

Each shore carries three `ResonatorProfile` entries in a `ShoreResonators` struct:
- `[0]` = bass/drone voice (Guitarra, Hardingfele, Bouzouki, Koto, Cavaquinho)
- `[1]` = chordal/harmonic voice (Kora, Langspil, Oud, Conch, Valiha)
- `[2]` = melodic/lead voice (Uilleann pipes, Kulning, Ney, Singing Bowl, Gamelan)

These slot indices correspond directly to OSTERIA's `QuartetRole` enum (Bass=0, Harmony=1, Melody=2, Rhythm=3). The Rhythm voice maps to slot `[0]` (bass) for its formant bank.

### 3.2 CreatureVoice — Marine and coastal fauna vocalizations

```cpp
struct CreatureVoice {
    const char* name;
    float startFreqs[3];  // Hz — formant start positions
    float endFreqs[3];    // Hz — formant end positions
    float bandwidths[3];  // Hz — formant widths
    float sweepMs;        // duration of formant sweep
    float gapMs;          // silence between calls
    float amplitude;      // relative level
};
```

Each shore carries three creatures in a `ShoreCreatures` struct:
- `[0]` = bird (high, fast — storm petrel, arctic tern, albatross, etc.)
- `[1]` = whale/deep (low, slow — humpback, beluga, Pacific whale, etc.)
- `[2]` = ambient (continuous — harbor foghorn, ice crack, reef crackle, tropical rain)

Creature voices are used by OSPREY's `CreatureFormant` resonator, which models ocean fauna as three interpolating bandpass filters. OSTERIA does not use creature voices directly — it uses the tavern murmur generator instead.

### 3.3 FluidCharacter — Ocean physics per coast

```cpp
struct FluidCharacter {
    float swellPeriodBase;   // seconds — base swell cycle length
    float swellDepth;        // 0-1 pronounced the swell is
    float chopFreqBase;      // Hz — surface chop frequency
    float chopAmount;        // 0-1 chop intensity at high sea state
    float depthBias;         // 0-1 bias toward subsurface energy
    float turbulenceOnset;   // 0-1 sea state threshold for turbulence
};
```

Atlantic: long rolling swells (12s period, 0.8 depth). Nordic: deep heavy slow waves (18s period). Mediterranean: short choppy (5s period, high chop). Pacific: vast gentle (25s period). Southern: warm rolling (8s period, moderate swing).

Used exclusively by OSPREY's `FluidEnergyModel`. OSTERIA does not consume fluid character — it lives on the shore, not in the ocean.

### 3.4 TavernCharacter — Room acoustics for seaside gathering places

```cpp
struct TavernCharacter {
    const char* name;
    float roomSizeMs;         // early reflection delay
    float decayMs;            // RT60
    float absorption;         // 0-1 HF absorption (wood=high, tile=low)
    float warmth;             // 0-1 low-frequency boost
    float murmurBrightness;   // 0-1 conversation texture brightness
    float density;            // 0-1 reflection density
};
```

| Shore | Tavern Name | Character |
|-------|------------|-----------|
| Atlantic | FadoHouse | Stone walls, low ceiling, wood bar, fireplace. High density, high warmth, dark tail. |
| Nordic | Sjohus | Deep timber paneling, heavy insulation. Very long RT60, maximum warmth and absorption. |
| Mediterranean | RembetikaDen | Open-air terrace, tile floor. Short bright tail, high murmur brightness, low density. |
| Pacific | HarborIzakaya | Paper screens, tatami, garden. Very short decay, maximum absorption, sparse density. |
| Southern | MornaBar | Corrugated roof, open sides, tropical air. Short decay, medium warmth and murmur. |

Used exclusively by OSTERIA's `TavernRoom` FDN reverb. OSPREY has a separate harbor verb (allpass chain) that uses a fixed room model, not the tavern character.

### 3.5 ShoreRhythm — Percussive pulse character

```cpp
struct ShoreRhythm {
    const char* name;
    float pulseRate;      // Hz — base pulse frequency
    float swing;          // 0-1 — rhythmic swing amount
    float accentPattern;  // 0=even, 0.33=waltz, 0.5=4/4, 0.67=6/8
};
```

| Shore | Drum | Feel |
|-------|------|------|
| Atlantic | Bodhran | 2.5 Hz, 0.15 swing, 6/8 feel |
| Nordic | Sami Drum | 1.8 Hz, 0.05 swing, waltz/3-feel |
| Mediterranean | Darbuka | 3.5 Hz, 0.3 swing, 4/4 with heavy swing |
| Pacific | Taiko | 1.2 Hz, 0.0 swing, slow and even |
| Southern | Djembe | 3.0 Hz, 0.25 swing, 6/8 with swing |

Used exclusively by OSTERIA's Rhythm quartet channel (`QuartetRole::Rhythm`). OSPREY has no rhythmic component.

---

## 4. Shore Morphing Utilities

The ShoreSystem provides three decomposition and interpolation utilities used by both engines at every block boundary.

**`decomposeShore(float shoreValue) -> ShoreMorphState`**
Takes a continuous `[0.0, 4.0]` value and returns `{shoreA, shoreB, frac}` — the two adjacent integer shore indices and the interpolation fraction between them. Shore values at the endpoints (0.0 or 4.0) clamp gracefully.

**`morphResonator(const ShoreMorphState&, int slot) -> ResonatorProfile`**
Linearly interpolates all `ResonatorProfile` fields between two shores for a given instrument slot. The `name` field is assigned winner-take-all at `frac < 0.5`. All numeric fields (formant frequencies, bandwidths, gains, attack, decay, brightness, inharmonicity) interpolate continuously.

**`morphCreature(const ShoreMorphState&, int slot) -> CreatureVoice`**
Same interpolation pattern for creature voice formant frequencies, bandwidths, sweep times, gap times, and amplitudes.

**`morphFluid(const ShoreMorphState&) -> FluidCharacter`**
Linearly interpolates all six fluid character fields.

**`morphTavern(const ShoreMorphState&) -> TavernCharacter`**
Linearly interpolates all numeric tavern fields. The room name is winner-take-all at `frac < 0.5`.

**`morphRhythm(const ShoreMorphState&) -> ShoreRhythm`**
Linearly interpolates pulse rate, swing, and accent pattern.

---

## 5. How OSPREY Uses the ShoreSystem

OSPREY is the ocean. It reads shore position as a single global value (`osprey_shore`, range 0-4) that controls the engine's entire timbral character. At each block boundary, OSPREY calls:

```cpp
ShoreMorphState shoreState = decomposeShore(pShore);

ResonatorProfile morphedResonators[3];
for (int i = 0; i < 3; ++i)
    morphedResonators[i] = morphResonator(shoreState, i);

CreatureVoice morphedCreatures[3];
for (int i = 0; i < 3; ++i)
    morphedCreatures[i] = morphCreature(shoreState, i);

FluidCharacter morphedFluid = morphFluid(shoreState);
```

These three interpolated structures are then applied to OSPREY's per-voice modal resonator banks, creature formant voices, and the global `FluidEnergyModel` respectively. The result is that moving the Shore knob changes the instrument character (what the ocean sounds like as a musical resonator), the creature sounds (what lives in the water), and the physical behavior of the wave generator.

OSPREY uses three instrument slots per voice in a 16-resonator bank: 3 groups x 4 formants = 12 resonators, plus 4 sympathetic resonators that are tuned to harmonics of the fundamental. The instrument slot a voice reads depends on the voice's configuration — OSPREY treats this as a single monolithic timbral profile, not a quartet.

OSPREY does **not** use `TavernCharacter` or `ShoreRhythm`.

---

## 6. How OSTERIA Uses the ShoreSystem

OSTERIA is the shore. It is a jazz quartet (Bass, Harmony, Melody, Rhythm) — four voices that can each occupy different positions on the Shore axis simultaneously. OSTERIA uses the ShoreSystem in three distinct ways:

### 6.1 Per-Channel Resonator Morphing

Each of the four quartet channels (`QuartetChannel`) holds its own `shorePos` float. At each control-rate update (~50Hz), OSTERIA calls `morphResonator(decomposeShore(ch.shorePos), slot)` independently for each channel, where `slot = static_cast<int>(ch.role)` (0=Bass, 1=Harmony, 2=Melody, 0 for Rhythm). The resulting formant frequencies are loaded into each channel's 4-pole Cytomic SVF bandpass bank.

This means that at any moment, the Bass voice could be playing Kora formants (Atlantic), the Harmony voice playing Oud formants (Mediterranean), and the Melody voice playing Singing Bowl formants (Pacific) — all simultaneously.

### 6.2 Timbral Memory (Palimpsest Effect)

Each `QuartetChannel` maintains a 32-position circular buffer of recent shore positions. At each control-rate update, the channel records its current `shorePos`. The `applyMemory()` method averages all 32 entries into a historical position, calls `morphResonator()` on that historical average, and blends the resulting formant frequencies with the current formant frequencies at a user-controlled memory amount.

The effect: if the bass voice has spent time at the Mediterranean shore, traces of the Oud's inharmonic warmth persist in its formant character even after the shore knob has moved to the Pacific. The quartet accumulates a living history of everywhere it has been.

### 6.3 Tavern Room and Rhythm

OSTERIA's `TavernRoom` FDN reverb is configured each block by calling `morphTavern(decomposeShore(pTavernShore))`. This is a *separate* shore parameter (`osteria_tavernShore`) from the four per-channel shore parameters — the room can be at a different coastline than the ensemble. A Pacific-positioned quartet could be playing in a FadoHouse acoustic space.

The Rhythm channel's transient pulse is driven by `ShoreRhythm` data. The `transientPhase` accumulates at `pulseRate` Hz; when it crosses a threshold, a transient burst fires with attack shaped by the rhythm's swing and accent pattern.

OSTERIA does **not** use `FluidCharacter` or `CreatureVoice`. The ocean physics belong to OSPREY alone.

---

## 7. Synthesis Consequences of Each Coastline

What the ShoreSystem actually *sounds* like is not incidental — each coastline produces a distinct synthesis character:

**Atlantic (0.0):** Full midrange body. Guitarra portuguesa's formants push energy at 320 Hz and 1200 Hz; the Kora adds warmth at 250 Hz with a clear 900 Hz peak. The ocean has long 12-second swells with moderate chop. The tavern is warm, dense, dark. The bodhran drives a 6/8 feel with moderate swing. Playing feel: warm, rhythmically propulsive, oceanic.

**Nordic (1.0):** High brightness and long decay. Hardingfele has significant 5200 Hz energy and long ring-out (1200ms). The Langspil contributes a hollow 200 Hz body with low brightness. Kulning pushes extreme upper formants (7000 Hz, 0.85 brightness). The ocean has 18-second deep slow swells with almost no chop. The tavern is heavily dampened and warm with long RT60. The Sami drum has minimal swing and a waltz feel. Playing feel: sparse, ancient, wide open.

**Mediterranean (2.0):** Inharmonic attack and short bright tail. The Bouzouki's inharmonicity (0.06) and 2ms attack create a percussive plucked character. The Oud's 0.08 inharmonicity is the highest in the system, producing distinctly non-Western partials. The Ney has a breathy high formant at 6500 Hz. The ocean has short 5-second chop. The tavern is tile-bright with minimal absorption. The darbuka drives a 4/4 feel with heavy 0.3 swing. Playing feel: percussive, soulful, bright.

**Pacific (3.0):** Maximum ring time and inharmonicity. The Singing Bowl has an 8000ms decay (longest in the system) and 0.15 inharmonicity (second highest). Narrow formant bandwidths (30 Hz for the Bowl's fundamental) create extremely pure resonant peaks. The Pacific whale has the slowest sweepMs (5000ms) and the lowest base frequency (60 Hz). The ocean has the longest swell period (25 seconds). The izakaya has near-maximum absorption. The taiko is slow and even. Playing feel: meditative, immersive, inharmonic overtones.

**Southern (4.0):** Warm density and gamelan character. The Gamelan has 0.2 inharmonicity (highest in the system) with tight bandwidths and moderate brightness. The Valiha brings 880 Hz warmth. The Cavaquinho has a fast 1ms attack and 350ms decay — the most percussive voice in the Southern set. The Southern whale has the tightest formant bandwidths in the creature system (20 Hz fundamental). The mornabar is open and warm. The djembe drives 6/8 with swing. Playing feel: rhythmically alive, inharmonic density, tropical weight.

---

## 8. Generalization Pattern

The ShoreSystem demonstrates an architectural pattern with wide applicability: **Shared Cultural Coordinate Systems** (SCCS). The pattern has four elements:

1. **A shared coordinate axis** — a single continuous float that both engines can read. Neither engine owns the axis.
2. **Layered data tables** — each point on the axis encodes multiple dimensions of synthesis data (spectral profile, room acoustics, rhythmic feel, physical behavior). The layers are not all consumed by all engines.
3. **Selective layer consumption** — each engine reads only the layers relevant to its synthesis technique. OSPREY reads resonators, creatures, and fluid. OSTERIA reads resonators, tavern, and rhythm. There is no wasteful duplication of effort.
4. **Morphing utilities** — shared interpolation functions ensure both engines compute continuous transitions identically. There is no risk of the engines diverging numerically.

### 8.1 Engine Pairs That Could Benefit

**ORACLE + OSTINATO — The Maqam/Rhythm System**

ORACLE is a GENDY/stochastic engine deeply informed by Middle Eastern maqam theory. OSTINATO is a communal drum circle engine. A shared `MaqamSystem` could encode tuning centers, ornament vocabularies, modal weights, and rhythmic cycles (iqa') for traditional musical systems (Arabic maqamat, Persian dastgahs, Turkish makams, Indian ragas). ORACLE would use the tuning and modal data; OSTINATO would use the rhythmic cycle data. A user moving a single "Tradition" knob would align both engines culturally without explicit coupling.

**OVERWORLD + OPAL — The EraSystem**

OVERWORLD already has an ERA crossfade (NES/Genesis/SNES). OPAL is a granular engine. A shared `EraSystem` could encode chip synthesis parameters (clock rates, pulse duty cycles, noise LFSR polynomials) alongside granular parameters (grain size distributions, scatter patterns, spectral density) derived from characteristic sounds of each era. OVERWORLD generates the chip audio; OPAL granulates it. Moving an ERA knob would align both engines to the same period of game audio history.

**OUROBOROS + OCEANDEEP — The PressureSystem**

OUROBOROS is a feedback-network engine with chaotic behavior. OCEANDEEP is an abyssal bass engine (pure Oscar in the water column mythology). A shared `PressureSystem` could encode depth as a physical and musical coordinate: water column pressure affects filter behavior (frequency-dependent absorption increases with depth), biological sound profiles change (sperm whale clicks vs. anglerfish bioluminescence pulses), and musical density shifts (deep music is slow and massive). Both engines would read the same depth coordinate and respond differently — OUROBOROS shifts its feedback topology toward longer chaotic cycles, OCEANDEEP shifts its fundamental register downward and its transient character toward sub-bass mass.

**ORBITAL + OBLIQUE — The SpectralGravitySystem**

ORBITAL is a spectral partials engine using additive synthesis with group envelopes. OBLIQUE is a prismatic frequency-domain engine. A shared `SpectralGravitySystem` could encode "gravitational centers" in the frequency domain — regions of spectral mass and void. ORBITAL would weight its partial amplitudes toward these centers; OBLIQUE would route its frequency-domain prisms to bounce between them. Moving a single "Gravity" knob would create coherent spectral shapes across both engines simultaneously.

### 8.2 Design Rules for New Shared Systems

1. **The shared header must be constexpr throughout.** No runtime state, no initialization order, no allocation. The data must be readable from any thread at any time.

2. **The coordinate axis must be a single float, not a multi-dimensional space.** The Shore axis is `[0.0, 4.0]`. This keeps morphing linear and the user model simple. Complexity lives in the data layers, not in the navigation.

3. **Each engine consumes only the layers it needs.** The system should not require either engine to "accept" data from all layers. Unused layers have zero cost — they are constexpr tables that are simply never referenced.

4. **Morphing utilities are shared, not duplicated.** Both engines call the same `morphResonator()`, `morphTavern()`, etc. functions. If a morphing computation changes, it changes for both engines simultaneously.

5. **The shared coordinate is exposed as an engine parameter, not a coupling signal.** OSPREY has `osprey_shore`. OSTERIA has `osteria_bassShore`, `osteria_harmShore`, `osteria_melShore`, `osteria_rhythmShore`, and `osteria_tavernShore`. These are user-addressable knobs. The synchronization is philosophical, not electronic — the user chooses to align them.

6. **Cultural data requires citations.** The ShoreSystem's formant data traces to real instruments. The Bouzouki's inharmonicity (0.06) and the Oud's (0.08) are grounded in spectral analysis. The rhythm patterns (Bodhran 6/8, Darbuka 4/4 swing) are grounded in ethnomusicological fact. Any future shared cultural system must maintain this rigor.

---

## 9. Implementation Notes

**Header location:** `Source/DSP/ShoreSystem/ShoreSystem.h`

**Include path for engines:**
```cpp
#include "../../DSP/ShoreSystem/ShoreSystem.h"
```
(Used by both `Source/Engines/Osprey/OspreyEngine.h` and `Source/Engines/Osteria/OsteriaEngine.h`)

**Namespace:** `xoceanus`

**Zero runtime cost:** All five data tables (`kShoreResonators`, `kShoreCreatures`, `kFluidCharacter`, `kTavernCharacter`, `kShoreRhythm`) are `static constexpr` arrays. They live in read-only data segment. The morphing functions are `inline` with no virtual dispatch, no heap, no locks.

**FastMath dependency:** The header includes `../FastMath.h` and uses the `lerp()` and `clamp()` functions from that shared DSP utility. These are also inline constexpr-compatible.

**Adding a sixth coast:** Adding a new shore would require: (1) extending the `Shore` enum, (2) adding entries to all five constexpr arrays, (3) updating `decomposeShore()` to clamp to the new maximum. The morphing functions require no changes. Existing engine code requires no changes. The coordinate axis would shift from `[0.0, 4.0]` to `[0.0, 5.0]`.

---

## 10. Seance Record

Blessing B012 was awarded during the Osprey/Osteria joint seance (2026-03-14). Smith called the ShoreSystem "a masterwork of system design." Buchla praised the spectral rigor. Tomita was silent for several moments before saying "this is how instruments should know where they are."

The key phrase from the seance verdict: "instruments that share cultural context at this level." No other synthesis platform has been cited for this pattern. The ShoreSystem is formally protected against: removal of the shared state between Osprey and Osteria, reduction to fewer than 5 coastlines, making the coastline data engine-private rather than shared, and replacement of cultural data with generic parameter presets.
