# XOmnibus

**34 synthesis engines. One mythology. Free forever.**

XOmnibus is a free, open-source multi-engine synthesizer platform by [XO_OX Designs](https://xo-ox.org). It merges character instruments into one unified creative environment where engines couple, collide, and mutate into sounds impossible with any single synth.

Every engine has a creature identity, a depth zone in the water column, and a sonic character shaped by the feliX–Oscar polarity — the bright and the deep, the surface and the abyss.

## What's Inside

- **34 synthesis engines** — each a distinct paradigm (granular, FM, physical modeling, spectral, stochastic, chip, additive, subtractive, and more)
- **11,247 factory presets** across 8 mood categories with 6D Sonic DNA
- **MegaCouplingMatrix** — 12 coupling types for cross-engine modulation
- **4 simultaneous engine slots** with 50ms crossfade hot-swap
- **PlaySurface** — 4-zone unified playing interface (Pad/Fretless/Drum modes)
- **Gallery Model UI** — warm white shell frames each engine's accent color

## Formats

| Platform | Formats |
|----------|---------|
| macOS | AU, Standalone |
| iOS | AUv3, Standalone |
| Windows/Linux | VST3 (planned) |

## Build

Requires: CMake 3.22+, Ninja, C++17 compiler, JUCE 8.0.4+

```bash
# macOS
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

## The Engines

| Engine | Character | Accent Color |
|--------|-----------|-------------|
| OddfeliX | Neon tetra — Karplus-Strong transient design | `#00A6D6` |
| OddOscar | Axolotl — PPG wavetable morphing | `#E8839B` |
| XOverdub | Dub synth + performance FX | `#6B7B3A` |
| XOdyssey | Analog drift modeling | `#7B2D8B` |
| XOblong | Warm analog character | `#E9A84A` |
| XObese | Maximalist saturation | `#FF1493` |
| XOnset | 8-voice drum synthesis (XVC coupling) | `#0066FF` |
| XOverworld | Chip synth — NES/Genesis/SNES | `#39FF14` |
| XOpal | Granular synthesis | `#A78BFA` |
| XOrbital | Additive partials | `#FF6B6B` |
| XOrganon | Variational free energy metabolism | `#00CED1` |
| XOuroboros | Strange attractor chaos | `#FF2D2D` |
| XObsidian | Crystalline physical modeling | `#E8E0D8` |
| XOverbite | Bass-forward character (5 macros) | `#F0EDE8` |
| XOrigami | Spectral folding STFT | `#E63946` |
| XOracle | GENDY stochastic + maqam | `#4B0082` |
| XObscura | Physical string modeling | `#8A9BA8` |
| XOceanic | Boid flocking synthesis | `#00B4A0` |
| XOcelot | Multi-biome emergent | `#C5832B` |
| XOptic | Visual modulation (audio-reactive) | `#00FF41` |
| XOblique | Prismatic bounce delays | `#BF40FF` |
| XOsprey | Azulejo coastal synthesis | `#1B4F8A` |
| XOsteria | Porto wine warmth | `#722F37` |
| XOwlfish | Mixtur-Trautonium oscillator | `#B8860B` |
| XOhm | Hippy Dad FM jam | `#87AE73` |
| XOrphica | Microsound harp | `#7FDBCA` |
| XObbligato | Dual wind instruments | `#FF8A7A` |
| XOttoni | Triple brass modeling | `#5B8A72` |
| XOlé | Afro-Latin trio | `#C9377A` |
| XOmbre | Dual-narrative engine | `#7B6B8A` |
| XOrca | Apex predator wavetable | `#1B2838` |
| XOctopus | Decentralized 8-arm intelligence | `#E040FB` |
| XOverlap | Knot-topology FDN reverb | `#00FFB4` |
| XOutwit | 8-arm Wolfram cellular automata | `#CC6600` |

## Presets

Factory presets live in `Presets/XOmnibus/` organized by mood:

**Foundation** · **Atmosphere** · **Entangled** · **Prism** · **Flux** · **Aether** · **Family** · **Submerged**

Each preset includes 6D Sonic DNA (brightness, warmth, movement, density, space, aggression) and 4 macros (CHARACTER, MOVEMENT, COUPLING, SPACE).

## XPN Expansion Packs

XOmnibus presets can be exported as XPN expansion packs for Akai MPC hardware. The XPN export pipeline lives in `Tools/`.

- **XOmnibus core = free forever.**
- **XPN packs = optional content** (some free, some paid).

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md) for build setup, code style, and PR guidelines.

## Security

See [SECURITY.md](SECURITY.md) for reporting vulnerabilities.

## License

[MIT](LICENSE) — Copyright (c) 2026 Joshua Cramblet / XO_OX Designs

## Links

- [XO-OX.org](https://xo-ox.org) — Home
- [Field Guide](https://xo-ox.org/guide) — Deep dives into each engine
- [Aquarium](https://xo-ox.org/aquarium) — Water column atlas
