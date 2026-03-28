# OBRIX Pocket

**The reef remembers.**

A creature-collecting modular synthesizer game for iPhone. Catch aquatic specimens, wire them together to build a synth voice, and play music through the instrument you've grown.

## Features (75 Sprints)

### Catch
- 4 category-specific mini-games (pattern match, frequency sweep, rhythm tap, echo memory)
- Monster Rancher DNA: songs from your music library birth unique creatures
- Multi-round HP system with escape/retry and rarity-upgrade bonus rounds
- Biome-aware spawning, seasonal events, random encounters

### Build
- 16 core specimens (4 Sources + 4 Processors + 4 Modulators + 4 Effects)
- Category shapes: circles, squares, diamonds, hexagons
- Long-press wiring with validation colors + coupling affinity
- Reef presets, stasis storage, specimen fusion

### Play
- Multi-touch keyboard with velocity, glissando, expression (Y=filter, X=pitch)
- Scale modes (chromatic/major/minor/pentatonic), chord mode, arpeggiator
- Performance recording, loop recording with overdub
- Fullscreen performance mode, MIDI export, live audio recording
- Tilt → filter sweep, shake → stutter effect

### Grow
- Specimen leveling (1→10) with play-style tracking (aggressive/gentle)
- 32 evolution branches at Level 10
- Spectral drift (specimens co-evolve when wired)
- Visual aging (Newborn→Ancient)
- Daily challenges, weekly dive challenges, streaks, milestones

### Dive
- 60-second generative performance with depth zones
- Player interaction (touch steering + tilt modulation)
- Source switching per zone, high scores, weekly challenges

### Share
- .xoreef export/import for desktop integration
- 15-second social clips (audio + card)
- MIDI file export
- Shareable creature cards + reef gallery images

### Connect
- MIDI input from external keyboards
- OSC output for wireless desktop control
- Local notifications (dormancy, daily energy, music catch)
- NFC specimen trading (Phase 2)

## Architecture
- Swift + SwiftUI + SpriteKit (UI)
- JUCE 8 + C++ (audio engine)
- GRDB (persistence)
- Lock-free audio bridge (SPSC queues + atomic params)
- 65+ Swift files, ~17,000 lines

## Build
```
cd ObrixPocket
xcodegen generate
open ObrixPocket.xcodeproj
```

Requires: Xcode 15+, iOS 16+, iPhone target

---

*Built with 75 sprints of focused game mechanics development.*
*Reviewed by Phantom Circuit Interactive across 5 audit rounds.*
*Vision Quest 015: The instrument remembers everything.*
