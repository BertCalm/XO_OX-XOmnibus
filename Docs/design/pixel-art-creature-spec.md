# XOceanus Pixel Art Creature Sprite Spec

**Date:** 2026-03-24
**Purpose:** Asset generation guide for 74 engine creature sprites (feliX + Oscar have 3D renders)

---

## Technical Spec

| Property | Value | Reason |
|----------|-------|--------|
| **Canvas** | 32x32px | Scales to 16px (half) and 64px (2x). Reads at 24-48px in UI |
| **Format** | PNG, transparent background | Matches feliX/Oscar render format |
| **Palette** | Max 8 colors per sprite | Engine accent color + 3-4 shades + black + white + 1-2 supporting |
| **Style** | Clean pixel art, 1px outlines, no anti-aliasing | Crisp at all sizes, retro charm |
| **Orientation** | Facing right, centered | Consistent across fleet, flippable for coupling pairs |
| **Poses** | 1 primary (idle) per engine | 88 sprites total scope |
| **Naming** | `creature-{engine_id}.png` | e.g., `creature-obrix.png` |
| **Location** | `Assets/creatures/` | New directory in repo |

---

## 3-Tier Creature System

| Tier | Style | Count | Engines |
|------|-------|-------|---------|
| **1** | 3D renders (multiple poses) | 2 | feliX (OddfeliX), Oscar (OddOscar) |
| **2** | Pixel art sprites (32x32) | 74 | All other engines |

---

## Generation Priority

### Batch 1 — V1 Curated Engines (~28)
Generate first. These ship with V1 "The Deep Opens."

### Batch 2 — Kitchen Collection (24)
Generate before each Patreon milestone unlock.

### Batch 3 — Remaining fleet
Generate for V1.1/V1.2 content drops.

---

## Complete Creature List (76 Engines)

### Hero Pair (3D Renders — DONE)

| # | Engine | Creature | Color | Status |
|---|--------|----------|-------|--------|
| 1 | ODDFELIX | Neon Tetra | `#00A6D6` | DONE (4 poses) |
| 2 | ODDOSCAR | Axolotl | `#E8839B` | DONE (2 poses) |

### Sunlit Zone

| # | Engine | Creature | Pixel Art Description | Color |
|---|--------|----------|----------------------|-------|
| 3 | ONSET | Pistol Shrimp | Tiny shrimp, one oversized claw, electric blue spark from snap | `#0066FF` |
| 4 | OVERWORLD | Pixel Fish | Chunky 8-bit fish, NES-era aesthetic (meta — chiptune engine) | `#39FF14` |
| 5 | OPTIC | Bioluminescent Plankton | Tiny glowing dot-cluster, phosphor green pulses | `#00FF41` |
| 6 | OBLIQUE | Prism Jellyfish | Translucent bell, violet light refracting through body | `#BF40FF` |
| 7 | OHM | Sea Lettuce | Gentle swaying seagrass cluster, sage green | `#87AE73` |
| 8 | ORPHICA | Siphonophore Harp | Delicate chain-of-pearls organism, seafoam glow | `#7FDBCA` |
| 9 | OBBLIGATO | Twin Seahorses | Two tiny seahorses facing each other, coral pink | `#FF8A7A` |
| 10 | OTTONI | Trumpet Fish | Long snout fish, patina green-brass body | `#5B8A72` |
| 11 | OLE | Dancing Shrimp | Flamenco-posed cleaner shrimp, hibiscus pink | `#C9377A` |
| 12 | ORIGAMI | Paper Nautilus | Folded-shell nautilus, vermillion, geometric edges | `#E63946` |
| 13 | OPENSKY | Flying Fish | Fins spread as wings, breaking the surface, sunburst orange | `#FF8C00` |

### Thermocline

| # | Engine | Creature | Pixel Art Description | Color |
|---|--------|----------|----------------------|-------|
| 14 | OVERDUB | Echo Eel | Moray eel with trailing afterimages, olive green | `#6B7B3A` |
| 15 | ODYSSEY | Sea Turtle | Ancient wanderer, violet shell with drift patterns | `#7B2D8B` |
| 16 | OPAL | Opalescent Nudibranch | Frilly sea slug, lavender with iridescent spots | `#A78BFA` |
| 17 | OCELOT | Ocelot Cat-Fish | Spotted catfish with feline markings, tawny | `#C5832B` |
| 18 | OWLFISH | Owl-Eyed Barreleye | Deep-sea fish with tubular upward-facing eyes, gold | `#B8860B` |
| 19 | OMBRE | Gradient Cuttlefish | Cuttlefish transitioning color head-to-tail, mauve | `#7B6B8A` |
| 20 | OUIE | Hammerhead Shark | Distinctive T-shaped head, steel grey, powerful | `#708090` |
| 21 | OVERWASH | Tide Foam Anemone | Soft waving tentacles, white foam-like tips | `#F0F8FF` |
| 22 | OVERWORN | Worn Barnacle | Weathered barnacle cluster, felt grey, textured | `#808080` |
| 23 | OVERCAST | Cloud Jellyfish | Flat mushroom-cap jelly, slate grey, drifting | `#778899` |

### The Reef

| # | Engine | Creature | Pixel Art Description | Color |
|---|--------|----------|----------------------|-------|
| 24 | OBLONG | Living Coral | Brain coral, rounded amber form, textured ridges | `#E9A84A` |
| 25 | OSPREY | Osprey Ray | Spotted eagle ray, azulejo blue wing-fins | `#1B4F8A` |
| 26 | OSTERIA | Wine Crab | Hermit crab in wine-bottle-cap shell, porto red | `#722F37` |
| 27 | OBRIX | Reef Architect Coral | Branching staghorn coral, jade green, geometric | `#1E8B7E` |
| 28 | ORBITAL | Circular Clownfish | Clownfish swimming in circle, warm red-orange | `#FF6B6B` |
| 29 | OCEANIC | Layered Lionfish | Lionfish with spectral fin layers, teal | `#00B4A0` |
| 30 | OTO | Pipe Organ Clam | Giant clam with pipe-like ridges, ivory | `#F5F0E8` |
| 31 | OCTAVE | Tonewheel Snail | Snail with spinning wheel shell, teak brown | `#8B6914` |
| 32 | OLEG | Theatre Anemone | Red sea anemone with dramatic tentacle spread | `#C0392B` |
| 33 | OTIS | Gospel Goldfish | Plump goldfish, gospel gold, joyful expression | `#D4A017` |
| 34 | OASIS | Yellowfin Tuna | Sleek tuna with silver-gold flank, mid-water, cardamom gold | `#C49B3F` |
| 35 | ONKOLO | Amber Lanternfish | Small fish with amber lantern organ | `#FFBF00` |
| 36 | OUTLOOK | Horizon Dolphin | Dolphin leaping toward horizon, indigo | `#4169E1` |
| 37 | ORCHARD | Blossom Jellyfish | Pink bell with petal-like tentacles | `#FFB7C5` |
| 38 | OVERGROW | Vine Anemone | Anemone overtaken by green growth tendrils | `#228B22` |
| 39 | OSIER | Willow Kelp | Long silvery kelp fronds swaying, willow-like | `#C0C8C8` |
| 40 | OXALIS | Wood Sorrel Nudibranch | Tiny slug with lilac leaf-shaped frills | `#9B59B6` |

### Open Water

| # | Engine | Creature | Pixel Art Description | Color |
|---|--------|----------|----------------------|-------|
| 41 | OBESE | The Whale | Chubby whale, hot pink belly, playful expression | `#FF1493` |
| 42 | OSTINATO | Drum Octopus | Octopus tapping 8 different surfaces, firelight orange | `#E8701A` |
| 43 | OVERLAP | Kelp Forest | Tangled kelp fronds, cyan-green, dense | `#00FFB4` |
| 44 | OVERTONE | Nautilus | Perfect spiral shell, spectral ice blue | `#A8D8EA` |
| 45 | OXBOW | Oxbow Eel | Electric eel in curved river shape, teal | `#1A6B5A` |
| 46 | OVERFLOW | Deep Current Manta | Manta ray riding deep current, dark blue | `#1A3A5C` |
| 47 | ODDFELLOW | Copper Pufferfish | Pufferfish with metallic copper sheen | `#B87333` |
| 48 | OPCODE | Digital Seahorse | Pixelated seahorse (meta — code-themed), turquoise | `#00CED1` |
| 49 | OLATE | Fretless Eel | Smooth, long eel, walnut brown, no markings | `#5C3317` |
| 50 | OAKEN | Upright Bass Seahorse | Tall seahorse, oak-colored, dignified posture | `#9C6B30` |
| 51 | OMEGA | Synth Bass Shark | Sleek shark, dark blue, electronic pulse lines | `#003366` |

### The Deep

| # | Engine | Creature | Pixel Art Description | Color |
|---|--------|----------|----------------------|-------|
| 52 | ORGANON | Metabolic Jellyfish | Pulsing deep-sea jelly, bioluminescent cyan | `#00CED1` |
| 53 | OBSIDIAN | Volcanic Glass Shrimp | Translucent shrimp near black smoker, crystal white | `#E8E0D8` |
| 54 | OVERBITE | Anglerfish | Classic deep-sea angler with bioluminescent lure, fang white | `#F0EDE8` |
| 55 | OUTWIT | Mimic Octopus | Shapeshifting cephalopod, chromatophore amber | `#CC6600` |
| 56 | OPERA | Singing Whale | Humpback with visible song arcs emanating, aria gold | `#D4AF37` |
| 57 | OFFERING | Crate Crab | Hermit crab carrying a vintage crate, wax yellow | `#E5B80B` |
| 58 | OXYTOCIN | Siphonophore Chain | Praya dubia — colonial organism, synapse violet | `#9B5DE5` |
| 59 | OVEN | Grand Piano Crab | Black crab with lid-shaped shell, ebony | `#1C1C1C` |
| 60 | OCHRE | Prepared Hermit | Hermit crab with objects stuck in shell, ochre | `#CC7722` |
| 61 | OBELISK | Ivory Pillar Coral | Tall column coral, pure white, grand | `#FFFFF0` |
| 62 | OPALINE | Rust Starfish | Star with textured, aged surface, prepared rust | `#B7410E` |
| 63 | OSMOSIS | Membrane Jellyfish | Semi-transparent jelly, silver, permeable body | `#C0C0C0` |

### The Abyss

| # | Engine | Creature | Pixel Art Description | Color |
|---|--------|----------|----------------------|-------|
| 64 | OUROBOROS | Hydrothermal Worm | Tube worm eating its own tail, red spiral | `#FF2D2D` |
| 65 | OBSCURA | Giant Squid | Massive eye, reaching tentacles, daguerreotype silver | `#8A9BA8` |
| 66 | ORCA | Orca | Apex predator silhouette, deep ocean dark | `#1B2838` |
| 67 | OCTOPUS | Deep Octopus | 8 arms splayed, each a different color pulse, magenta | `#E040FB` |
| 68 | ORBWEAVE | Kelp Knot Spider Crab | Crab tangled in kelp knots, purple | `#8E4585` |
| 69 | ORGANISM | Coral Colony | Fractal coral growth pattern, emergence lime | `#C6E377` |
| 70 | OCEANDEEP | Abyssal Fish | Pitch-black fish with single bioluminescent spot, trench violet | `#2D0A4E` |
| 71 | OGRE | Abyssal Ogre Fish | Massive jaw, sub-bass black, menacing | `#0D0D0D` |
