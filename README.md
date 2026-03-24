# XOmnibus

**A free, open-source multi-engine synthesizer platform by XO_OX Designs.**

71 synthesis engines in one plugin. Each engine is a distinct instrument with its own DSP architecture, sonic character, and aquatic creature identity. Engines couple, collide, and mutate into sounds impossible with any single synth.

## Engines

| Engine | Character | Creature |
|--------|-----------|----------|
| OddfeliX | Neon analog character | Neon Tetra |
| OddOscar | Morphing axolotl synthesis | Axolotl |
| Overdub | Dub synth + spring reverb | Olive Rockfish |
| Odyssey | Drift synthesis, nautilus journey | Nautilus |
| Oblong | Coral reef warmth | Brain Coral |
| Obese | Fat analog saturation + Mojo axis | Blobfish |
| Overbite | Bass-forward character, 5 macros | Fangtooth |
| Onset | 8-voice percussion + XVC coupling | Pistol Shrimp |
| Overworld | Chip synth (NES/Genesis/SNES/Neo Geo) | Pixel Reef |
| Opal | Granular synthesis | Opal Squid |
| Orbital | Group envelope orbits | Moon Jellyfish |
| Organon | Metabolic variational synthesis | Hydrothermal Worm |
| Ouroboros | Chaotic feedback + leash mechanism | Ouroboros Eel |
| Obsidian | Crystal resonance synthesis | Obsidian Sponge |
| Origami | Wavefold synthesis | Paper Nautilus |
| Oracle | GENDY stochastic + maqam | Oracle Fish |
| Obscura | Physical string modeling | Coelacanth |
| Oceanic | Boid swarm synthesis | Sardine School |
| Ocelot | Biome ecosystem engine | Ocelot Catfish |
| Optic | Visual modulation (Zero-Audio Identity) | Firefly Squid |
| Oblique | Prismatic bounce synthesis | Prism Shrimp |
| Osprey | ShoreSystem 5-coastline engine | Osprey |
| Osteria | ShoreSystem culinary engine | Harbor Seal |
| Owlfish | Mixtur-Trautonium + subharmonic | Owlfish |
| Ohm | Resistance synthesis | Electric Eel |
| Orphica | Microsound harp plucking | Siphonophore |
| Obbligato | Dual wind synthesis | Pilot Fish Pair |
| Ottoni | Triple brass synthesis | Trumpet Fish |
| Ole | Afro-Latin trio synthesis | Flamenco Cuttlefish |
| Ombre | Dual narrative synthesis (memory/forgetting + perception) | Shadow Octopus |
| Orca | Apex predator wavetable + echolocation + breach synthesis | Orca |
| Octopus | 8-arm decentralized intelligence + chromatophore modulation | Giant Pacific Octopus |
| Overlap | Knot-topology FDN reverb | Lion's Mane Jellyfish |
| Outwit | 8-arm Wolfram CA synthesis | Mimic Octopus |
| Ostinato | Physically-modeled world instruments (12 cultures) | Seahorse Drummer |
| OpenSky | Euphoric shimmer supersaw | Flying Fish |
| OceanDeep | Pressure synthesis, deep field | Anglerfish |
| Ouïe | Duophonic hammerhead synthesis | Hammerhead Shark |
| Obrix | Modular brick synthesis (coral reef habitat) | The Reef Itself |
| Orbweave | Topological knot coupling synthesis | Kelp Knot Spider |
| Overtone | Continued fraction spectral engine | Nautilus Shell |
| Organism | Cellular automata generative synthesis | Coral Colony |
| Oxbow | Entangled reverb synth (Chiasmus FDN + phase erosion) | Oxbow Lake |
| Oware | Tuned percussion — material continuum + mallet physics + sympathetic resonance | Sunken Akan Board |
| Opera | Additive-vocal Kuramoto synchronicity synthesis + autonomous Conductor arc | Bioluminescent Siren |
| Offering | Psychology-driven boom bap drum synthesis (Berlyne curiosity + 5 city modes) | Mantis Shrimp |

## Features

- **4-slot engine gallery** — load any combination of engines
- **Cross-engine coupling** — 14 coupling types (including KnotTopology) let engines modulate each other
- **~22,000+ factory presets** across 8 mood categories
- **PlaySurface** — unified 4-zone playing interface (Pad, Fretless, Drum modes)
- **6D Sonic DNA** — every preset tagged with brightness, warmth, movement, density, space, aggression
- **4 performance macros** per engine
- **MPE support** — per-note velocity, aftertouch, and expression across all engines
- **Guru Bin Awakening Presets** — curated hero presets for ORBWEAVE, OVERTONE, ORGANISM, OWARE, OPERA, and more

## Build

Requires [JUCE](https://juce.com/) (included as submodule in `Libs/JUCE`).

```bash
# macOS (AU + Standalone)
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

## Formats

- **AU** — macOS (shipping V1)
- **Standalone** — macOS (shipping V1)
- **AUv3** — iOS (planned V1.1)
- **VST3** — V2

## Presets

Factory presets live in `Presets/XOmnibus/` organized by mood:
Foundation, Atmosphere, Entangled, Prism, Flux, Aether, Family, Submerged.

Preset format is `.xometa` (JSON). See `Docs/xomnibus_master_specification.md` for the schema.

## Documentation

- `Docs/xomnibus_master_specification.md` — the single source of truth
- `Docs/xomnibus_sound_design_guides.md` — per-engine sound design guides
- `scripture/the-scripture.md` — Guru Bin's accumulated DSP wisdom (Book of Bin)
- `Docs/seance_cross_reference.md` — ghost council verdicts for all engines

## License

[MIT](LICENSE) — free to use, modify, and distribute.

## About XO_OX Designs

XO_OX builds instruments where character comes first. Every engine is designed as a standalone instrument, then integrated into XOmnibus where it can couple with others to create sounds impossible alone.

Website: [XO-OX.org](https://xo-ox.org)
