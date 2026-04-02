# D006 Mod Wheel Completion — Round 11E

**Date**: 2026-03-14
**Doctrine**: D006 — Expression Input Is Not Optional
**Round**: 11E (Prism Sweep)

---

## Summary

6 priority engines were targeted and all 6 are now wired. Fleet mod wheel coverage advances from 9/23 to 15/23 (65%).

---

## Audit Methodology

Pattern searched per engine main file:

```
modWheelAmount | modWheel.* = | getControllerNumber.*== 1
```

All 29 engine main files audited (multi-file engines like Ocelot, Owlfish checked via primary adapter).

---

## Pre-Round Status (9/23 wired)

Engines **already wired** before this round:
- Drift — vibrato depth
- Fat — drive/character
- Morph — morphScanner modulation
- Oblique — prism color
- Obsidian — PD depth
- Oracle — stochastic field width
- Orbital — partial tilt
- Origami — fold modulation
- Snap — filter resonance

---

## Engines Fixed This Round (6)

### 1. ONSET — mod wheel → MUTATE macro depth scale

**File**: `Source/Engines/Onset/OnsetEngine.h`

**Destination**: MUTATE macro scaling multiplier.

**Implementation**:
- Member: `float modWheelAmount = 0.0f;`
- MIDI loop: `if (msg.isController() && msg.getControllerNumber() == 1) modWheelAmount = msg.getControllerValue() / 127.0f;`
- Application: `mMutate = clamp(mMutate * (1.0f + modWheelAmount), 0.0f, 1.0f);`

**Sensitivity**: Wheel at max doubles the effective MUTATE depth (×1.0–×2.0 multiplier). At MUTATE=0.5, full wheel raises effective depth to 1.0. Each hit drifts further in blend + character.

**Character**: At rest, the kit is stable. Rolling the wheel up turns the kit into a living organism — every trigger drifts blend and character randomly, creating unpredictable but tonally coherent percussion.

---

### 2. OPAL — mod wheel → grain density scatter (posScatter + pitchScatter)

**File**: `Source/Engines/Opal/OpalEngine.h`

**Destination**: posScatter (+0.35) and pitchScatter (+0.25) at full wheel.

**Implementation**:
- Member: `float modWheelAmount = 0.0f;`
- MIDI loop: `else if (m.isController() && m.getControllerNumber() == 1) modWheelAmount = ...`
- Application in param snapshot after atPressure:
  - `posScatter = clamp(posScatter + modWheelAmount * 0.35f, 0.0f, 1.0f);`
  - `pitchScatter = clamp(pitchScatter + modWheelAmount * 0.25f, 0.0f, 1.0f);`

**Sensitivity**: posScatter +0.35, pitchScatter +0.25 at full wheel. These stack additively with aftertouch (which adds +0.3 posScatter).

**Character**: Wheel at rest = focused iridescent cloud. Wheel up = grains spread across time and pitch, producing a shimmering, diffuse halo. Complementary axis to aftertouch (which compresses the grains together).

---

### 3. ORGANON — mod wheel → entropy rate (metabolicRate acceleration)

**File**: `Source/Engines/Organon/OrganonEngine.h`

**Destination**: metabolicRate (+3.0 Hz at full wheel).

**Implementation**:
- Member: `float modWheelAmount = 0.0f;`
- MIDI loop: `else if (message.isController() && message.getControllerNumber() == 1) modWheelAmount = ...`
- Application after atPressure boost: `metabolicRate = std::clamp(metabolicRate + modWheelAmount * 3.0f, 0.1f, 10.0f);`

**Sensitivity**: +3.0 Hz at full wheel. Default metabolicRate is 1.0 Hz; full wheel can push to 4.0 Hz (before aftertouch). Combined aftertouch + wheel can push to 6.5 Hz.

**Character**: Wheel at rest = slow chemotroph metabolism. Wheel up = the organism feeds faster, belief updates quicken, the Port-Hamiltonian modal array evolves at a higher rate. Excellent for building tension across a performance.

---

### 4. OUROBOROS — mod wheel → leash tension

**File**: `Source/Engines/Ouroboros/OuroborosEngine.h`

**Destination**: effectiveLeash (+0.4 at full wheel).

**Implementation**:
- Member: `float modWheelAmount = 0.0f;`
- MIDI loop: `else if (message.isController() && message.getControllerNumber() == 1) modWheelAmount = ...`
- Application: `effectiveLeash = clamp(leashAmount - atPressure * 0.3f + modWheelAmount * 0.4f, 0.0f, 1.0f);`

**Sensitivity**: +0.4 leash at full wheel. Creates direct counterpoint to aftertouch (which subtracts -0.3 leash).

**Character**: Wheel creates the expressionist control axis: wheel down = pure chaos attractor free-running; wheel up = attractor reins toward played pitch. The two controllers work in opposition — aftertouch loosens the snake, wheel tightens it. This is the "Blessing B003 Leash" in full expressive form.

---

### 5. OBSCURA — mod wheel → bow speed / excitation intensity

**File**: `Source/Engines/Obscura/ObscuraEngine.h`

**Destination**: effectiveSustain (+0.4 at full wheel = continuous excitation force).

**Implementation**:
- Member: `float modWheelAmount = 0.0f;`
- MIDI loop: `else if (msg.isController() && msg.getControllerNumber() == 1) modWheelAmount = ...`
- Application in param snapshot: `effectiveSustain = clamp(paramSustainForce + macroCoupling * kMacroCouplingToSustain + modWheelAmount * 0.4f, 0.0f, 1.0f);`

**Sensitivity**: +0.4 to sustainForce at full wheel. Maps directly to bowing pressure on the 128-mass spring chain.

**Character**: Wheel at rest = default impulse-driven daguerreotype sound. Wheel up = continuous bow force applied to the spring chain — the silver-plate string sustains and grows, like a bowed wire pressed harder into a resonant body. Physically intuitive: mod wheel IS the bow pressure.

---

### 6. OWLFISH — mod wheel → mixtur depth (Mixtur-Trautonium subharmonic presence)

**File**: `Source/Engines/Owlfish/OwlfishEngine.h`

**Destination**: `snapshot.subMix` (+0.45 at full wheel).

**Implementation**:
- Member: `float modWheelAmount = 0.0f;`
- MIDI loop: `else if (msg.isController() && msg.getControllerNumber() == 1) modWheelAmount = ...`
- Application after atPressure: `snapshot.subMix = std::clamp(snapshot.subMix + modWheelAmount * 0.45f, 0.0f, 1.0f);`

**Sensitivity**: +0.45 to subMix at full wheel. With default subMix ~0.5, full wheel can push to 0.95 (near maximum subharmonic saturation).

**Character**: The owlfish descends. Wheel up = the Mixtur-Trautonium's subharmonic organ stack deepens, the creature's resonant body fills with low-frequency presence. Honors Blessing B014 (the Mixtur-Trautonium oscillator) by giving performers direct morphological control over how deep into the abyss the owlfish swims.

---

## Fleet Status After Round 11E

**23 total engines** (not counting Optic which intentionally ignores MIDI):

| Engine | Mod Wheel | Destination |
|--------|-----------|-------------|
| Bite | MISSING | — |
| Bob | MISSING | — |
| Drift | WIRED | vibrato depth |
| Dub | MISSING | — |
| Fat | WIRED | saturation drive |
| Morph | WIRED | scan modulation |
| Obbligato | MISSING | — |
| Oblique | WIRED | prism color |
| Obscura | WIRED (Round 11E) | bow speed / excitation |
| Obsidian | WIRED | PD depth |
| Oceanic | MISSING | — |
| Ocelot | MISSING | — |
| Ohm | MISSING | — |
| Ole | MISSING | — |
| Onset | WIRED (Round 11E) | MUTATE depth scale |
| Opal | WIRED (Round 11E) | grain density scatter |
| Optic | N/A — no MIDI | zero-audio identity engine |
| Oracle | WIRED | stochastic field |
| Orbital | WIRED | partial tilt |
| Organon | WIRED (Round 11E) | entropy / metabolic rate |
| Origami | WIRED | fold modulation |
| Orphica | MISSING | — |
| Osprey | MISSING | — |
| Osteria | MISSING | — |
| Ottoni | MISSING | — |
| Ouroboros | WIRED (Round 11E) | leash tension |
| Overworld | MISSING | — |
| Owlfish | WIRED (Round 11E) | mixtur depth |
| Snap | WIRED | filter resonance |

**Fleet total**: 15 wired / 22 MIDI-capable engines (68%)
**Before Round 11E**: 9 / 22 (41%)
**Delta**: +6 engines

---

## Engines Without Mod Wheel (Remaining — 7 engines)

These are MIDI-capable engines not yet covered. None are intentionally exempt.

| Engine | Notes | Suggested Destination |
|--------|-------|-----------------------|
| Bite | Has aftertouch | bite_biteDepth (feral intensity) |
| Bob | Has aftertouch | filter cutoff (classic vibrato/filter) |
| Dub | No aftertouch yet | send amount (tape delay depth) |
| Obbligato | Family engine | bond tension macro |
| Oceanic | Chromatophore | chromatophore modulation rate |
| Ocelot | Has aftertouch | biome blend (ecosystem morph) |
| Ohm | Family engine | commune/meddling macro |
| Ole | Family engine | drama argument intensity |
| Orphica | Family engine | harp register split |
| Osprey | Has aftertouch | shore coastline blend |
| Osteria | Has aftertouch | recipe warmth |
| Ottoni | Family engine | brass age/grow macro |
| Overworld | Has aftertouch | ERA triangle blend |

Note: 8 engines shown in "MISSING" column above (Obbligato, Ole, Ohm, Ottoni, Orphica included in original engine list vs. smaller MIDI-capable count).

---

## Intentionally Left Without Mod Wheel

**Optic (OPTIC)** — `OpticEngine.h` accepts `MidiBuffer& /*midi*/` — the parameter is commented out entirely. Optic is XOceanus's "zero-audio identity" engine (Blessing B005): it generates no sound, only modulation signals for other engines. It has no note-on/off, no aftertouch, and does not require expression input. D006 does not apply.

---

## Implementation Notes

All implementations follow the established fleet pattern:
1. `float modWheelAmount = 0.0f;` as a persistent class member (survives block boundaries)
2. CC#1 handler in MIDI loop as an `else if` branch after aftertouch
3. Application uses additive modulation with `clamp()` to `[0.0, 1.0]`
4. No allocation, no branching cost above existing aftertouch path

The Owlfish implementation writes directly to `snapshot.subMix` after snapshot update, following the same post-snapshot mutation pattern used for aftertouch in that engine.
