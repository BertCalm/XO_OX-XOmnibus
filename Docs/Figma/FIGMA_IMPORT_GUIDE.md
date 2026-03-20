# XOmnibus Figma Import Guide

## SVG Files

| File | Description | Dimensions |
|------|-------------|------------|
| `XOmnibus_Desktop_Light.svg` | macOS/AU/VST3 — light mode (primary) | 880 x 562 |
| `XOmnibus_Desktop_Dark.svg` | macOS/AU/VST3 — dark mode | 880 x 562 |
| `XOmnibus_iPad.svg` | iPad Regular/Full — AUv3 | 1024 x 768 |
| `XOmnibus_iPhone.svg` | iPhone Portrait — AUv3 carousel | 390 x 844 |

## How to Import

1. Open Figma, create a new file
2. **File > Place Image** (or drag SVG onto canvas)
3. Right-click imported SVG > **Flatten** to ungroup into editable layers
4. All groups have descriptive IDs (e.g., `SlotA`, `Macro_CHARACTER`, `Knob_FilterCutoff`)

## Design Tokens

### Colors

#### Light Mode (Primary)
| Token | Hex | Usage |
|-------|-----|-------|
| Shell White | `#F8F6F3` | Background, main shell |
| Text Dark | `#1A1A1A` | Primary text |
| Text Mid | `#777570` | Secondary text, labels |
| Border Gray | `#DDDAD5` | Borders, dividers |
| Slot Bg | `#FCFBF9` | Cards, panels, slots |
| Empty Slot | `#EAE8E4` | Empty/disabled states |
| XO Gold | `#E9C46A` | Macros, coupling, active states |
| XO Gold Text | `#9E7C2E` | Gold text (WCAG AA on white) |

#### Dark Mode
| Token | Hex | Usage |
|-------|-----|-------|
| Shell Dark | `#1A1A1A` | Background |
| Text Light | `#EEEEEE` | Primary text |
| Text Mid | `#C8C8C8` | Secondary text |
| Border | `#4A4A4A` | Borders, dividers |
| Card Bg | `#2D2D2D` | Cards, panels |
| Empty Slot | `#363636` | Empty/disabled states |
| XO Gold | `#E9C46A` | Same gold in both modes |

#### Engine Accent Colors
| Engine | Hex | Name |
|--------|-----|------|
| OddfeliX | `#00A6D6` | Neon Tetra Blue |
| OddOscar | `#E8839B` | Axolotl Gill Pink |
| Overdub | `#6B7B3A` | Olive |
| Odyssey | `#7B2D8B` | Violet |
| Oblong | `#E9A84A` | Amber |
| Obese | `#FF1493` | Hot Pink |
| Onset | `#0066FF` | Electric Blue |
| Overworld | `#39FF14` | Neon Green |
| Opal | `#A78BFA` | Lavender |
| Orbital | `#FF6B6B` | Warm Red |
| Organon | `#00CED1` | Bioluminescent Cyan |
| Ouroboros | `#FF2D2D` | Strange Attractor Red |
| Obsidian | `#E8E0D8` | Crystal White |
| Overbite | `#F0EDE8` | Fang White |
| Origami | `#E63946` | Vermillion Fold |
| Oracle | `#4B0082` | Prophecy Indigo |
| Obscura | `#8A9BA8` | Daguerreotype Silver |
| Oceanic | `#00B4A0` | Phosphorescent Teal |
| Ocelot | `#C5832B` | Ocelot Tawny |
| Optic | `#00FF41` | Phosphor Green |
| Oblique | `#BF40FF` | Prism Violet |
| Osprey | `#1B4F8A` | Azulejo Blue |
| Osteria | `#722F37` | Porto Wine |
| Owlfish | `#B8860B` | Abyssal Gold |
| Ohm | `#87AE73` | Sage |
| Orphica | `#7FDBCA` | Siren Seafoam |
| Obbligato | `#FF8A7A` | Rascal Coral |
| Ottoni | `#5B8A72` | Patina |
| Ole | `#C9377A` | Hibiscus |
| Overlap | `#00FFB4` | Bioluminescent Cyan-Green |
| Outwit | `#CC6600` | Chromatophore Amber |
| Ombre | `#7B6B8A` | Shadow Mauve |
| Orca | `#1B2838` | Deep Ocean |
| Octopus | `#E040FB` | Chromatophore Magenta |
| Ostinato | `#E8701A` | Firelight Orange |
| OpenSky | `#FF8C00` | Sunburst |
| OceanDeep | `#2D0A4E` | Trench Violet |
| Ouie | `#708090` | Hammerhead Steel |
| Obrix | `#1E8B7E` | Reef Jade |
| Orbweave | `#8E4585` | Kelp Knot Purple |
| Overtone | `#A8D8EA` | Spectral Ice |
| Organism | `#C6E377` | Emergence Lime |

### Typography

| Role | Font | Weight | Example Size |
|------|------|--------|-------------|
| Display | Space Grotesk | Bold | 14–18px |
| Heading | Inter | Bold | 10–12px |
| Body | Inter | Regular | 8–10px |
| Label | Inter | Regular | 7–8px |
| Value | JetBrains Mono | Regular | 8–9px |

### Layout Dimensions

#### Desktop (880 x 562)
| Region | Size | Position |
|--------|------|----------|
| Header | 880 x 50 | Top |
| Sidebar | 155 x (H - 50 - 105 - 68) | Left, below header |
| Main Content | (880 - 155) x remaining | Right of sidebar |
| Master FX Strip | 880 x 68 | Above macros |
| Macro Strip | 880 x 105 | Bottom |
| Engine Slot Tile | 147 x 80 | In sidebar, 4 slots |

#### iPad (1024 x 768)
| Region | Size | Notes |
|--------|------|-------|
| Header | 1024 x 48 | Engine pills, preset browser |
| PlaySurface | 1024 x 340 | 4 zones visible |
| Perf Strip | 1024 x 80 | Coupling + FX quick access |
| Bottom Section | 1024 x 300 | Waveform + macro knobs |

#### iPhone (390 x 844)
| Region | Size | Notes |
|--------|------|-------|
| Status Bar | 390 x 54 | iOS chrome |
| Header | 390 x 44 | Compact engine pills |
| PlaySurface | 390 x 380 | 1 zone, carousel swipe |
| Macro Bar | 390 x 56 | 24pt compact knobs |
| Param Drawer | 390 x 276 | Drag-up, peek state |

### Component Patterns

- **Rotary Knob**: Circle track + arc indicator + center dot + label + value
- **Engine Slot Tile**: Rounded rect + accent color bar (left 5px) + name + level meter
- **Coupling Card**: Pill shape, XO Gold tint, mono font route label
- **FX Slot**: Rounded rect card + label + mini progress bar + value
- **Pad Cell**: Rounded rect with engine accent tint, note label
- **Macro Knob**: Same as rotary but XO Gold stroke, bold label

### Accessibility

- Focus ring: `#0066CC` (light) / `#58A6FF` (dark), 2px stroke
- Min touch target: 44x44pt (mobile), 24x24pt (desktop)
- All text meets WCAG AA contrast ratios
