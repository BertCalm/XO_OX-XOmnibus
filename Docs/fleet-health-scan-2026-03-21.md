# Fleet Health Scan — 2026-03-21

**Total engines in `Source/Engines/`:** 44
**Total `.xometa` preset files:** 16,085
**Scan date:** 2026-03-21

---

## Methodology

- **Lines:** `wc -l` on the primary `*Engine.h` or `*Adapter.h` file (shallowest match in engine dir)
- **Solo presets:** `.xometa` files where `engines` array contains exactly this engine and no others
- **Total presets:** `.xometa` files where `engines` array contains this engine (solo + coupled)
- **Shared DSP utilities:** `#include` or class usage of any file in `Source/DSP/` detected in the main `.h`
- **Pitch bend:** presence of `isPitchWheel` in main `.h`
- **Aftertouch:** presence of `isChannelPressure` in main `.h`

Seven engines (Bite, Bob, Drift, Dub, Fat, Morph, Snap) have 0 presets — they are fully implemented DSP engines not yet shipped into the preset library.

---

## Fleet Table — sorted by Solo Preset count (lowest first)

| Engine | Main File | Lines | Solo Presets | Total (incl. coupled) | Shared DSP Utilities | Pitch Bend | Aftertouch |
|--------|-----------|------:|-------------:|----------------------:|----------------------|:----------:|:----------:|
| Bite | `BiteEngine.h` | 2,372 | 0 | 0 | VoiceAllocator, StandardADSR, PolyBLEP, CytomicSVF | no | YES |
| Bob | `BobEngine.h` | 1,740 | 0 | 0 | StandardLFO, VoiceAllocator, StandardADSR, PolyBLEP | no | YES |
| Drift | `DriftEngine.h` | 1,736 | 0 | 0 | StandardLFO, VoiceAllocator, StandardADSR, PolyBLEP, CytomicSVF | no | YES |
| Dub | `DubEngine.h` | 1,326 | 0 | 0 | VoiceAllocator, StandardADSR, PolyBLEP, CytomicSVF | no | YES |
| Fat | `FatEngine.h` | 1,556 | 0 | 0 | StandardLFO, VoiceAllocator, PitchBendUtil, StandardADSR, PolyBLEP | YES | YES |
| Morph | `MorphEngine.h` | 1,245 | 0 | 0 | VoiceAllocator, PitchBendUtil, StandardADSR, PolyBLEP, WavetableOscillator | YES | YES |
| Snap | `SnapEngine.h` | 1,092 | 0 | 0 | VoiceAllocator, PitchBendUtil, StandardADSR, PolyBLEP, CytomicSVF | YES | YES |
| Oware | `OwareEngine.h` | 911 | 20 | 23 | StandardLFO, VoiceAllocator, GlideProcessor, PitchBendUtil, FilterEnvelope, CytomicSVF, ParameterSmoother | YES | YES |
| Owlfish | `OwlfishEngine.h` | 245 | 98 | 351 | — | no | YES |
| Osprey | `OspreyEngine.h` | 2,050 | 106 | 456 | StandardLFO, VoiceAllocator, StandardADSR, CytomicSVF | no | YES |
| Oceanic | `OceanicEngine.h` | 1,331 | 110 | 487 | StandardLFO, VoiceAllocator, StandardADSR, PolyBLEP, CytomicSVF, ParameterSmoother | no | YES |
| Origami | `OrigamiEngine.h` | 1,863 | 133 | 658 | StandardLFO, VoiceAllocator, GlideProcessor, CytomicSVF, ParameterSmoother | no | YES |
| Oracle | `OracleEngine.h` | 1,641 | 135 | 684 | StandardLFO, VoiceAllocator, GlideProcessor, StandardADSR, CytomicSVF | no | YES |
| Ocelot | `OcelotEngine.h` | 152 | 141 | 437 | — | no | YES |
| Oxbow | `OxbowEngine.h` | 691 | 150 | 150 | StandardLFO, CytomicSVF | no | YES |
| Osteria | `OsteriaEngine.h` | 1,949 | 164 | 450 | StandardLFO, VoiceAllocator, StandardADSR, CytomicSVF | no | YES |
| Ole | `OleEngine.h` | 374 | 169 | 517 | — | no | YES |
| Ombre | `OmbreEngine.h` | 946 | 170 | 428 | PolyBLEP, CytomicSVF | no | YES |
| Orbital | `OrbitalEngine.h` | 1,628 | 175 | 552 | CytomicSVF | no | YES |
| Ottoni | `OttoniEngine.h` | 462 | 175 | 522 | — | no | YES |
| Octopus | `OctopusEngine.h` | 1,455 | 178 | 390 | StandardLFO, VoiceAllocator, StandardADSR, CytomicSVF, WavetableOscillator | no | **no** |
| Ostinato | `OstinatoEngine.h` | 2,189 | 179 | 198 | StandardLFO, CytomicSVF | no | YES |
| Obbligato | `ObbligatoEngine.h` | 510 | 180 | 548 | — | no | YES |
| Oblique | `ObliqueEngine.h` | 1,700 | 180 | 625 | PitchBendUtil, PolyBLEP, CytomicSVF | YES | YES |
| Ohm | `OhmEngine.h` | 616 | 181 | 577 | — | no | YES |
| Obsidian | `ObsidianEngine.h` | 1,433 | 190 | 518 | StandardLFO, VoiceAllocator, StandardADSR, CytomicSVF | no | YES |
| Ouroboros | `OuroborosEngine.h` | 1,546 | 191 | 626 | — | no | YES |
| Orca | `OrcaEngine.h` | 1,348 | 194 | 435 | StandardLFO, VoiceAllocator, GlideProcessor, StandardADSR, CytomicSVF, WavetableOscillator, ParameterSmoother | no | **no** |
| Orphica | `OrphicaEngine.h` | 635 | 194 | 547 | — | YES | YES |
| Overtone | `OvertoneEngine.h` | 921 | 318 | 338 | — | no | YES |
| OpenSky | `OpenSkyEngine.h` | 1,417 | 359 | 377 | StandardLFO, VoiceAllocator, StandardADSR, PolyBLEP, CytomicSVF | no | YES |
| Organism | `OrganismEngine.h` | 997 | 361 | 382 | PolyBLEP | no | YES |
| Ouie | `OuieEngine.h` | 1,903 | 374 | 393 | StandardLFO, VoiceAllocator, StandardADSR, PolyBLEP, CytomicSVF | no | YES |
| Outwit | `XOutwitAdapter.h` | 716 | 381 | 688 | — | YES | YES |
| Obrix | `ObrixEngine.h` | 1,758 | 384 | 405 | PolyBLEP, CytomicSVF | YES | YES |
| Overlap | `XOverlapAdapter.h` | 775 | 393 | 673 | — | no | YES |
| Opal | `OpalEngine.h` | 2,539 | 225 | 675 | PolyBLEP, CytomicSVF | no | YES |
| Optic | `OpticEngine.h` | 1,054 | 216 | 600 | CytomicSVF | no | YES |
| Overworld | `OverworldEngine.h` | 711 | 216 | 582 | — | no | YES |
| Organon | `OrganonEngine.h` | 1,587 | 218 | 757 | CytomicSVF | no | YES |
| Obbligato | — (dup row removed — see 180 above) | | | | | | |
| OceanDeep | `OceandeepEngine.h` | 943 | 277 | 296 | — | YES | YES |
| Onset | `OnsetEngine.h` | 2,206 | 331 | 641 | CytomicSVF | no | YES |
| Orbweave | `OrbweaveEngine.h` | 1,125 | 337 | 343 | PolyBLEP, CytomicSVF | YES | YES |
| Obscura | `ObscuraEngine.h` | 1,730 | 53 | 495 | StandardLFO, VoiceAllocator, StandardADSR, CytomicSVF, ParameterSmoother | no | YES |

---

## Clean Table (no duplicates, sorted solo presets ascending)

| # | Engine | Main File | Lines | Solo Presets | Total | Shared DSP Utilities | Pitch Bend | Aftertouch |
|---|--------|-----------|------:|-------------:|------:|----------------------|:----------:|:----------:|
| 1 | Bite | `BiteEngine.h` | 2,372 | 0 | 0 | VoiceAllocator, StandardADSR, PolyBLEP, CytomicSVF | no | YES |
| 2 | Bob | `BobEngine.h` | 1,740 | 0 | 0 | StandardLFO, VoiceAllocator, StandardADSR, PolyBLEP | no | YES |
| 3 | Drift | `DriftEngine.h` | 1,736 | 0 | 0 | StandardLFO, VoiceAllocator, StandardADSR, PolyBLEP, CytomicSVF | no | YES |
| 4 | Dub | `DubEngine.h` | 1,326 | 0 | 0 | VoiceAllocator, StandardADSR, PolyBLEP, CytomicSVF | no | YES |
| 5 | Fat | `FatEngine.h` | 1,556 | 0 | 0 | StandardLFO, VoiceAllocator, PitchBendUtil, StandardADSR, PolyBLEP | YES | YES |
| 6 | Morph | `MorphEngine.h` | 1,245 | 0 | 0 | VoiceAllocator, PitchBendUtil, StandardADSR, PolyBLEP, WavetableOscillator | YES | YES |
| 7 | Snap | `SnapEngine.h` | 1,092 | 0 | 0 | VoiceAllocator, PitchBendUtil, StandardADSR, PolyBLEP, CytomicSVF | YES | YES |
| 8 | Oware | `OwareEngine.h` | 911 | 20 | 23 | StandardLFO, VoiceAllocator, GlideProcessor, PitchBendUtil, FilterEnvelope, CytomicSVF, ParameterSmoother | YES | YES |
| 9 | Obscura | `ObscuraEngine.h` | 1,730 | 53 | 495 | StandardLFO, VoiceAllocator, StandardADSR, CytomicSVF, ParameterSmoother | no | YES |
| 10 | Owlfish | `OwlfishEngine.h` | 245 | 98 | 351 | — | no | YES |
| 11 | Osprey | `OspreyEngine.h` | 2,050 | 106 | 456 | StandardLFO, VoiceAllocator, StandardADSR, CytomicSVF | no | YES |
| 12 | Oceanic | `OceanicEngine.h` | 1,331 | 110 | 487 | StandardLFO, VoiceAllocator, StandardADSR, PolyBLEP, CytomicSVF, ParameterSmoother | no | YES |
| 13 | Origami | `OrigamiEngine.h` | 1,863 | 133 | 658 | StandardLFO, VoiceAllocator, GlideProcessor, CytomicSVF, ParameterSmoother | no | YES |
| 14 | Oracle | `OracleEngine.h` | 1,641 | 135 | 684 | StandardLFO, VoiceAllocator, GlideProcessor, StandardADSR, CytomicSVF | no | YES |
| 15 | Ocelot | `OcelotEngine.h` | 152 | 141 | 437 | — | no | YES |
| 16 | Oxbow | `OxbowEngine.h` | 691 | 150 | 150 | StandardLFO, CytomicSVF | no | YES |
| 17 | Osteria | `OsteriaEngine.h` | 1,949 | 164 | 450 | StandardLFO, VoiceAllocator, StandardADSR, CytomicSVF | no | YES |
| 18 | Ole | `OleEngine.h` | 374 | 169 | 517 | — | no | YES |
| 19 | Ombre | `OmbreEngine.h` | 946 | 170 | 428 | PolyBLEP, CytomicSVF | no | YES |
| 20 | Orbital | `OrbitalEngine.h` | 1,628 | 175 | 552 | CytomicSVF | no | YES |
| 21 | Ottoni | `OttoniEngine.h` | 462 | 175 | 522 | — | no | YES |
| 22 | Octopus | `OctopusEngine.h` | 1,455 | 178 | 390 | StandardLFO, VoiceAllocator, StandardADSR, CytomicSVF, WavetableOscillator | no | **no** |
| 23 | Ostinato | `OstinatoEngine.h` | 2,189 | 179 | 198 | StandardLFO, CytomicSVF | no | YES |
| 24 | Obbligato | `ObbligatoEngine.h` | 510 | 180 | 548 | — | no | YES |
| 25 | Oblique | `ObliqueEngine.h` | 1,700 | 180 | 625 | PitchBendUtil, PolyBLEP, CytomicSVF | YES | YES |
| 26 | Ohm | `OhmEngine.h` | 616 | 181 | 577 | — | no | YES |
| 27 | Obsidian | `ObsidianEngine.h` | 1,433 | 190 | 518 | StandardLFO, VoiceAllocator, StandardADSR, CytomicSVF | no | YES |
| 28 | Ouroboros | `OuroborosEngine.h` | 1,546 | 191 | 626 | — | no | YES |
| 29 | Orca | `OrcaEngine.h` | 1,348 | 194 | 435 | StandardLFO, VoiceAllocator, GlideProcessor, StandardADSR, CytomicSVF, WavetableOscillator, ParameterSmoother | no | **no** |
| 30 | Orphica | `OrphicaEngine.h` | 635 | 194 | 547 | — | YES | YES |
| 31 | Optic | `OpticEngine.h` | 1,054 | 216 | 600 | CytomicSVF | no | YES |
| 32 | Overworld | `OverworldEngine.h` | 711 | 216 | 582 | — | no | YES |
| 33 | Organon | `OrganonEngine.h` | 1,587 | 218 | 757 | CytomicSVF | no | YES |
| 34 | Opal | `OpalEngine.h` | 2,539 | 225 | 675 | PolyBLEP, CytomicSVF | no | YES |
| 35 | OceanDeep | `OceandeepEngine.h` | 943 | 277 | 296 | — | YES | YES |
| 36 | Overtone | `OvertoneEngine.h` | 921 | 318 | 338 | — | no | YES |
| 37 | Onset | `OnsetEngine.h` | 2,206 | 331 | 641 | CytomicSVF | no | YES |
| 38 | Orbweave | `OrbweaveEngine.h` | 1,125 | 337 | 343 | PolyBLEP, CytomicSVF | YES | YES |
| 39 | OpenSky | `OpenSkyEngine.h` | 1,417 | 359 | 377 | StandardLFO, VoiceAllocator, StandardADSR, PolyBLEP, CytomicSVF | no | YES |
| 40 | Organism | `OrganismEngine.h` | 997 | 361 | 382 | PolyBLEP | no | YES |
| 41 | Ouie | `OuieEngine.h` | 1,903 | 374 | 393 | StandardLFO, VoiceAllocator, StandardADSR, PolyBLEP, CytomicSVF | no | YES |
| 42 | Outwit | `XOutwitAdapter.h` | 716 | 381 | 688 | — | YES | YES |
| 43 | Obrix | `ObrixEngine.h` | 1,758 | 384 | 405 | PolyBLEP, CytomicSVF | YES | YES |
| 44 | Overlap | `XOverlapAdapter.h` | 775 | 393 | 673 | — | no | YES |

---

## Key Findings

### Engines with 0 presets (7 — newly built, unshipped)
These engines have substantial DSP implementations but no `.xometa` entries yet. They need preset campaigns.

| Engine | Lines | Note |
|--------|------:|-------|
| Bite | 2,372 | No pitch bend |
| Bob | 1,740 | No pitch bend |
| Drift | 1,736 | No pitch bend |
| Fat | 1,556 | Has PitchBendUtil ✓ |
| Morph | 1,245 | Has PitchBendUtil + WavetableOscillator ✓ |
| Snap | 1,092 | Has PitchBendUtil ✓ |
| Dub | 1,326 | No pitch bend |

### Thinnest shipped engines (solo preset count < 150)
These are the highest-priority candidates for preset expansion campaigns.

| Engine | Solo | Total | Gap to 200 |
|--------|-----:|------:|----------:|
| Oware | 20 | 23 | 180 |
| Obscura | 53 | 495 | 147 |
| Owlfish | 98 | 351 | 102 |
| Osprey | 106 | 456 | 94 |
| Oceanic | 110 | 487 | 90 |
| Origami | 133 | 658 | 67 |
| Oracle | 135 | 684 | 65 |
| Ocelot | 141 | 437 | 59 |
| Oxbow | 150 | 150 | 50 |

### Missing pitch bend (`isPitchWheel` not found in main `.h`)
36 of 44 engines have no pitch bend processing. Engines with it:
Fat, Morph, Snap, Oblique, Obrix, OceanDeep, Orbweave, Orphica, Outwit, Oware

### Missing aftertouch (`isChannelPressure` not found in main `.h`)
Only 2 shipped engines lack aftertouch: **Octopus** and **Orca** (both use WavetableOscillator — may be intentional).

### Smallest main `.h` files (potential stubs / incomplete engines)
| Engine | Lines | Note |
|--------|------:|-------|
| Ocelot | 152 | Very thin — may be a stub |
| Owlfish | 245 | Thin but has 351 total presets |
| Ole | 374 | Simple engine |
| Ottoni | 462 | Simple engine |
| Obbligato | 510 | Moderate |

### No shared DSP utilities (manual DSP implementations)
14 engines include none of the shared `Source/DSP/` utilities. These carry the highest maintenance surface:
Obbligato, OceanDeep, Ocelot, Ohm, Ole, Orphica, Ottoni, Ouroboros, Outwit (adapter), Overlap (adapter), Overtone, Overworld, Owlfish

---

## Summary Statistics

| Metric | Value |
|--------|-------|
| Total engines | 44 |
| Engines with 0 presets | 7 |
| Engines with < 200 solo presets | 16 |
| Engines missing pitch bend | 34 |
| Engines missing aftertouch | 2 (Octopus, Orca) |
| Engines using StandardLFO | 20 |
| Engines using VoiceAllocator | 20 |
| Engines using CytomicSVF | 30 |
| Engines using PitchBendUtil | 6 |
| Adapter engines (not native) | 2 (Outwit, Overlap) |
| Largest engine (lines) | Opal — 2,539 lines |
| Smallest engine (lines) | Ocelot — 152 lines |
| Total `.xometa` files scanned | 16,085 |
