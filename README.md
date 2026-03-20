# XOmnibus

**A free, open-source multi-engine synthesizer platform by XO_OX Designs.**

42 synthesis engines in one plugin. Each engine is a distinct instrument with its own DSP architecture, sonic character, and aquatic creature identity. Engines couple, collide, and mutate into sounds impossible with any single synth.

## Engines

| Engine | Character | Creature |
|--------|-----------|----------|
| OddfeliX | Neon analog character | Neon Tetra |
| OddOscar | Morphing axolotl | Axolotl |
| Overdub | Dub synth + performance FX | Olive Rockfish |
| Odyssey | Drift synthesis, nautilus journey | Nautilus |
| Oblong | Coral reef warmth | Brain Coral |
| Obese | Fat analog saturation | Blobfish |
| Overbite | Bass-forward character | Fangtooth |
| Onset | 8-voice percussion | Pistol Shrimp |
| Overworld | Chip synth (NES/Genesis/SNES) | Pixel Reef |
| Opal | Granular synthesis | Opal Squid |
| Orbital | Group envelope orbits | Moon Jellyfish |
| Organon | Metabolic synthesis | Hydrothermal Worm |
| Ouroboros | Chaotic feedback loops | Ouroboros Eel |
| Obsidian | Crystal resonance | Obsidian Sponge |
| Origami | Wavefold synthesis | Paper Nautilus |
| Oracle | Stochastic + GENDY | Oracle Fish |
| Obscura | Physical string modeling | Coelacanth |
| Oceanic | Boid swarm synthesis | Sardine School |
| Ocelot | Biome ecosystem engine | Ocelot Catfish |
| Optic | Visual modulation (no audio) | Firefly Squid |
| Oblique | Prismatic bounce synthesis | Prism Shrimp |
| Osprey | ShoreSystem cultural engine | Osprey |
| Osteria | ShoreSystem culinary engine | Harbor Seal |
| Owlfish | Mixtur-Trautonium + subharmonic | Owlfish |
| Ohm | Hippy jam resistance | Electric Eel |
| Orphica | Microsound harp | Siphonophore |
| Obbligato | Dual wind synthesis | Pilot Fish Pair |
| Ottoni | Triple brass | Trumpet Fish |
| Ole | Afro-Latin trio | Flamenco Cuttlefish |
| Ombre | Dual narrative (memory/forgetting) | Shadow Octopus |
| Orca | Apex predator wavetable | Orca |
| Octopus | 8-arm decentralized intelligence | Giant Pacific Octopus |
| Overlap | Knot-topology FDN | Lion's Mane Jellyfish |
| Outwit | 8-arm Wolfram CA | Giant Pacific Octopus |
| Ostinato | Rhythmic ostinato looper | Mantis Shrimp |
| OpenSky | Euphoric shimmer (supersaw + shimmer reverb) | Flying Fish |
| OceanDeep | Abyssal sub-bass (anglerfish + pressure) | Anglerfish |
| Ouie | Duophonic hammerhead (2 voices x 8 algorithms) | Hammerhead Shark |
| Obrix | Modular brick synthesis (coral reef) | Coral Reef Colony |
| Orbweave | Topological knot coupling (Kelp Knot) | Kelp Forest Spider |
| Overtone | Continued fraction spectral (Nautilus) | Nautilus |
| Organism | Cellular automata generative (Coral Colony) | Coral Colony |

## Features

- **4-slot engine gallery** — load any combination of engines
- **Cross-engine coupling** — 13 coupling types let engines modulate each other
- **21,000+ factory presets** across 8 mood categories
- **PlaySurface** — unified 4-zone playing interface
- **6D Sonic DNA** — every preset tagged with brightness, warmth, movement, density, space, aggression
- **4 performance macros** per engine
- **MPE support** — per-note expression across all engines

## Build

Requires [JUCE](https://juce.com/) (included as submodule in `Libs/JUCE`).

```bash
# macOS (AU + Standalone)
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build

# Run tests
./build/XOmnibusTests
```

## Formats

- **AU** — macOS (shipping)
- **Standalone** — macOS (shipping)
- **AUv3** — iOS (planned)
- **VST3** — V2

## Presets

Factory presets live in `Presets/XOmnibus/` organized by mood:
Foundation, Atmosphere, Entangled, Prism, Flux, Aether, Family, Submerged.

Preset format is `.xometa` (JSON). See `Docs/xomnibus_master_specification.md` for the schema.

## Documentation

- `Docs/xomnibus_master_specification.md` — the single source of truth
- `Docs/xomnibus_sound_design_guides.md` — per-engine sound design guides
- `Docs/seance_cross_reference.md` — ghost council verdicts for all engines

## License

[MIT](LICENSE) — free to use, modify, and distribute.

## About XO_OX Designs

XO_OX builds instruments where character comes first. Every engine is designed as a standalone instrument, then integrated into XOmnibus where it can couple with others to create sounds impossible alone.

Website: [XO-OX.org](https://xo-ox.org)
