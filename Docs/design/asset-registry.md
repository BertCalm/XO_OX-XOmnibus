# XO_OX Design Asset Registry
## Master Catalogue — All Design Assets Across the Pipeline

**Version**: 1.0 — 2026-03-23
**Scope**: XOmnibus (desktop plugin), Outshine/Originate (companion tools), XO-OX.org (website), audio-xpm-creator (web app), iOS AUv3/Standalone
**Maintained by**: UIX Design Studio (Ulf, Issea, Xavier, Lucy)
**Target lookup time**: Under 10 seconds for any asset

---

## HOW TO USE THIS REGISTRY

1. **Find by category** — use the section headers below (Fonts, Figma Kits, Knobs, etc.)
2. **Find by use case** — see the [Quick-Access Tables](#quick-access-tables) at the bottom
3. **Find by product** — each asset lists "Relevance to XOmnibus" with specific component names
4. **Status codes**:
   - `ADOPTED` — in active use in production files
   - `REVIEW` — acquired, not yet evaluated for fit
   - `REFERENCE` — good for inspiration/patterns, not directly incorporated
   - `REJECTED` — evaluated and ruled out (reason noted)

---

## TABLE OF CONTENTS

1. [Fonts](#1-fonts)
2. [Figma Kits — Audio UI Library (audio-ui.com)](#2-figma-kits--audio-ui-library)
3. [Figma Kits — Community & Third-Party](#3-figma-kits--community--third-party)
4. [Knob / Control Libraries](#4-knob--control-libraries)
5. [Button Libraries](#5-button-libraries)
6. [Slider Libraries](#6-slider-libraries)
7. [VU Meters & Level Indicators](#7-vu-meters--level-indicators)
8. [Audio UI Component Kits (Complete)](#8-audio-ui-component-kits-complete)
9. [Gradient Packs](#9-gradient-packs)
10. [Icon Sets](#10-icon-sets)
11. [UI Component Kits & Design Systems](#11-ui-component-kits--design-systems)
12. [Design System — XO_OX Source of Truth](#12-design-system--xo_ox-source-of-truth)
13. [HTML Prototypes & Mockups](#13-html-prototypes--mockups)
14. [Reference Screenshots](#14-reference-screenshots)
15. [Audio-Specific Specialty Assets](#15-audio-specific-specialty-assets)
16. [Quick-Access Tables](#quick-access-tables)
17. [JUCE Translation Notes (Lucy's Reference)](#juce-translation-notes-lucys-reference)

---

## 1. FONTS

> All fonts below are for display/header/body use. For XOmnibus production, Space Grotesk + Inter + JetBrains Mono are the canonical three. Others are REVIEW/REFERENCE for the iOS app, XO-OX.org headers, or future expansions.

---

### 1.1 ADOPTED — Production Fonts (XOmnibus / Outshine / Site)

| Name | Weights | Format | License | Use | Location |
|------|---------|--------|---------|-----|----------|
| **Space Grotesk** | Light, Regular, Medium, SemiBold, Bold (300–700) | WOFF2/TTF (Google Fonts CDN) | OFL (free) | Window titles (20pt SemiBold), section headers (11pt SemiBold ALL CAPS), tab labels (13pt SemiBold), engine names (13pt Regular) | Google Fonts CDN — embedded via BinaryData in JUCE |
| **Inter** | Light, Regular, Medium, SemiBold (300–600) | WOFF2/TTF (Google Fonts CDN) | OFL (free) | Body copy (12pt Regular), form field labels (11pt Medium), descriptions, tooltips, site body text | Google Fonts CDN — embedded via BinaryData in JUCE |
| **JetBrains Mono** | Light, Regular, Medium (300–500) | WOFF2/TTF (Google Fonts CDN) | OFL (free) | All numeric values (11pt Regular): Hz, dB, MIDI notes, BPM, sample counts, parameter readouts | Google Fonts CDN — embedded via BinaryData in JUCE |

**Note**: These three are referenced in both the JUCE plugin (`Docs/design/xomnibus_ui_master_spec_v2.md`) and the site (`Site/design-tokens.css`, `Docs/mockups/xomnibus-main-ui.html`). Always embed via `BinaryData` — never rely on system installation.

---

### 1.2 ADOPTED — Site Display Fonts (XO-OX.org headers, loaded via WOFF)

| Name | Weights | Format | License | Use | Location |
|------|---------|--------|---------|-----|----------|
| **Nebulica** | Light, Regular, Medium, SemiBold, Bold, ExtraBold (6 weights) | WOFF2 | Commercial (Ui8 purchase) | XO-OX.org large display headings (aquarium section, hero overlays) | `~/Documents/GitHub/XO_OX-XOmnibus/Site/fonts/` + source pack at `~/Downloads/nebulica_NjliZmQ4YWYzNzVhNWYwMDk1NzM2MDE0/OTF/` |
| **Chrys** | Light, Regular, Distorted (3 weights) | WOFF + OTF | Commercial (Ui8 purchase) | XO-OX.org specialty headings; Distorted variant for avant-garde contexts | `~/Documents/GitHub/XO_OX-XOmnibus/Site/fonts/` + source at `~/Downloads/chrys_sans_serif_font_family_NjliZmQ4YWYzNzVhNWYwMDk1NzM2MDE0/OTF/` |

---

### 1.3 ADOPTED — Glyph / Symbol Fonts (XO-OX.org display panels)

| Name | Style | Format | License | Use | Location |
|------|-------|--------|---------|-----|----------|
| **HINO Aloraglyphs** | Symbolic glyphs | OTF | Commercial (Ui8 purchase) | Decorative glyph accents in site panels and engine cards | `~/Documents/GitHub/XO_OX-XOmnibus/Site/fonts/HINOAloraglyphs.otf` |
| **HINO Cryptaglyphs** | Runic/cipher symbols | OTF | Commercial (Ui8 purchase) | Cipher/rune accents in mythology-themed UI elements | `~/Documents/GitHub/XO_OX-XOmnibus/Site/fonts/HINOCryptaglyphs-Regular.otf` |
| **HINO Cyberglyphs** | Cyber/tech symbols | OTF | Commercial (Ui8 purchase) | Cyber/tech accent glyphs for plugin chrome details | `~/Documents/GitHub/XO_OX-XOmnibus/Site/fonts/HINOCyberglyphs.otf` |

---

### 1.4 REVIEW — Candidate Display Fonts

| Name | Weights | Format | License | Style | Relevance | Location |
|------|---------|--------|---------|-------|-----------|----------|
| **Nebulica** (additional OTF) | Thin, ExtraLight, Black (full 9-weight family) | OTF | Commercial (Ui8) | Geometric, futuristic sans | Strong for XOmnibus nameplate and premium headers | `~/Downloads/nebulica_NjliZmQ4YWYzNzVhNWYwMDk1NzM2MDE0/OTF/` |
| **Brate** | Light, Regular, Medium, SemiBold, Bold (5 weights) | OTF | Commercial (Ui8) | Clean geometric sans, wide tracking | Body alternative for large display; good at 48–72px | `~/Downloads/Brate/` |
| **Fonzy** | Thin, Light, Regular, Bold, Outline (5 weights) | OTF | Commercial (Ui8) | Rounded geometric, playful-but-clean | iOS app UI candidate — warm character, legible | `~/Downloads/fonzy_sans_serif_5_font_family_pack_NjliZmQ4YWYzNzVhNWYwMDk1NzM2MDE0/OTF/` |
| **Immani 2** | Full variable family | OTF/TTF | Commercial (Ui8) | Modern variable sans | Variable font experiments; axis testing | `~/Downloads/immani_2_font_family_pack_NjliZmQ4YWYzNzVhNWYwMDk1NzM2MDE0/` |
| **LTS Raela Pro** | Light, Regular, Bold, Black, Italic variants (8+ weights) | TTF | Commercial (Ui8) | Editorial sans-serif | Long-form site editorial content; Field Guide articles | `~/Downloads/lts-raela-pro_NjliZmQ4YWYzNzVhNWYwMDk1NzM2MDE0/` |
| **Berkslund** | Thin, Light, Regular, Bold (4 weights) | OTF | Commercial (Ui8) | Clean angular sans | Minimal UI elements; could replace Inter for subheads | `~/Downloads/berkslund_NjliZmQ4YWYzNzVhNWYwMDk1NzM2MDE0/` |
| **Bulta** | Bold only | OTF | Commercial (Ui8) | Heavy rounded sans | Hero/nameplate at very large sizes only | `~/Downloads/bulta_NjliZmQ4YWYzNzVhNWYwMDk1NzM2MDE0/` |
| **Dakar** | Regular, Italic, Halftone (3 styles) | OTF/TTF | Readme included | Condensed display, alt-glyphs | Nameplate candidate — strong character | `~/Downloads/Dakar Font/` |
| **Neue Reman** | Upright, Italic, Condensed, Expanded, Semi-Condensed, Variable (full family) | Variable TTF | Commercial | Variable-width serif/sans hybrid | Long-form reading; Field Guide articles | `~/Downloads/Neue Reman Sans Serif /` |
| **Sherika** | Multiple widths (Normal, Condensed, Expanded) | Variable | Commercial | Variable geometric sans | Flexible UI alternative | `~/Downloads/Sherika Font Family/` |
| **Polly Rounded** | Thin, Light, Regular, Bold (4 weights) | OTF/TTF | Commercial (Ui8) | Rounded, friendly | iOS AUv3 app body text; approachable for mobile | `~/Downloads/Polly_Rounded/` |
| **Qirvalen** | Variable | OTF/TTF + VF | Commercial (Ui8) | Variable organic | Organic display; specialty headers | `~/Downloads/qirvalen_NjliZmQ4YWYzNzVhNWYwMDk1NzM2MDE0/` |
| **Quora** | Regular | OTF | Commercial (Ui8) | Mono-width serif | Specialty nameplate or logo text | `~/Downloads/quora_NjliZmQ4YWYzNzVhNWYwMDk1NzM2MDE0/` |
| **Rangled** | Variable-width | OTF/TTF | Commercial (Ui8) | Condensed gothic | High-density data panel labels | `~/Downloads/rangled/` |
| **Royal Ocean** | Regular (single weight) | OTF + TTF | Check license | Decorative display | Ocean/aquatic-themed decorative text; aquarium section | `~/Downloads/Fonts/RoyalOcean.otf` |
| **Enfonix** | Standard, Mono, Pro variants | OTF | Commercial (Ui8) | Monospaced industrial | Data readouts; alternative to JetBrains Mono | `~/Downloads/enfonix_NjliZmQ4YWYzNzVhNWYwMDk1NzM2MDE0/` |
| **Bitroad** | Regular, Italic | OTF/TTF/WOFF | Commercial (Ui8) | Condensed display | Album/pack title typography | `~/Downloads/bitroad_NjliZmQ4YWYzNzVhNWYwMDk1NzM2MDE0/` |
| **Trigué Sans** | Variable family | OTF/TTF | Commercial (Ui8) | Humanist variable sans | Body text alternative for site | `~/Downloads/triguesans-sans-serif-font_NjliZmQ4YWYzNzVhNWYwMDk1NzM2MDE0/` |
| **Ezy** | Variable | Variable TTF | Commercial (Ui8) | Variable condensed | Display experiments | `~/Downloads/ezyzip_NjliZmQ4YWYzNzVhNWYwMDk1NzM2MDE0/` |
| **Safari** | Regular | OTF/TTF/WOFF | Commercial (Ui8) | Geometric caps | Single-use display / decorative header | `~/Downloads/safari_NjliZmQ4YWYzNzVhNWYwMDk1NzM2MDE0/` |
| **Figo Shift Editorial** | Multiple weights + variable | Variable | Commercial (Ui8) | Variable editorial serif | Long-form editorial; philosophy/manifesto pages | `~/Downloads/figo-shift-editorial_NjliZmQ4YWYzNzVhNWYwMDk1NzM2MDE0/` |

---

### 1.5 REFERENCE — Specialty / Display Fonts

| Name | Weights | Format | License | Style | Notes | Location |
|------|---------|--------|---------|-------|-------|----------|
| **Aperture 3.2** | Regular (demo + full OTF) | OTF/TTF/WOFF/SVG | Demo available | Stencil futuristic | Strong for sci-fi/tech contexts; XOsmosis or XOnset UI chrome | `~/Downloads/APERTURE-Reg/` |
| **Aqum 2** | Classic, Bold, Curl, Small Caps (4 styles) | OTF | Commercial | Rounded tech | Specialty display; curved letterforms suit aquatic branding | `~/Downloads/Aqum 2 Complete family/` |
| **ATTACK** | ExtraLight, Light, Regular, Bold, ExtraBold, Black (6 weights) | ZIP archives per weight | Commercial | Heavy impact | Maximum-impact headings; live performance contexts | `~/Downloads/attack-font-family_NjliZmQ4YWYzNzVhNWYwMDk1NzM2MDE0/` |
| **Copixel Display** | Display only | OTF | Commercial (Ui8) | Pixel/bitmap display | LED display simulations; chip-synth UIs (XOverworld) | `~/Downloads/copixel-display-_NjliZmQ4YWYzNzVhNWYwMDk1NzM2MDE0/` |
| **Kalipixel** | Regular | OTF/TTF | Commercial | Pixel futuristic | Pixel/retro game contexts; XOverworld UI chrome | `~/Downloads/Kalipixel - Futuristic Typeface v1/` |
| **Grit** | Preview images + OTF/TTF | Commercial (Ui8) | Textured/degraded | Grungy display; analog warmth. Vinyl/tape contexts | `~/Downloads/grit_NjliZmQ4YWYzNzVhNWYwMDk1NzM2MDE0/` |
| **Main File (Rupa)** | Full variable family | Variable | Commercial (Ui8) | Variable humanist | General candidate | `~/Downloads/rupa-package_NjliZmQ4YWYzNzVhNWYwMDk1NzM2MDE0/` |

---

## 2. FIGMA KITS — AUDIO UI LIBRARY

> Source: audio-ui.com ("Samy") — commercial purchase, single-designer operation. All kits: you may use in any product but cannot resell or redistribute as standalone assets.
> All paths under: `~/Downloads/Audio UI/`

### 2.1 Figma Kit 01 — Keyboard Instrument UI
- **Location**: `~/Downloads/Audio UI/FIGMA-KITS/Figma-Kit-01/`
- **Contains**: Big Knob + filmstrip, Small Knob + filmstrip, Button + filmstrip, Full keyboard with Black Key / Left Key / Middle Key / Right Key filmstrips, Background panel
- **Style**: Dark hardware — black keys, metal knobs, dark panel
- **Status**: REVIEW
- **Relevance**: Plugin Keyboard/PlaySurface view. The keyboard filmstrips directly applicable to any piano-register display (XOpal, XOven, XObelisk).
- **JUCE path**: `juce::Image` filmstrips → `CustomKeyboardComponent` painting

### 2.2 Figma Kit 02 — Compact Synth UI
- **Location**: `~/Downloads/Audio UI/FIGMA-KITS/Figma-kit-02/`
- **Contains**: Background, Big Knob, Medium Knob, Small Knob, Buttons, Sliders, Vent/grille texture
- **Style**: Dark metal hardware, brushed finish
- **Status**: REVIEW
- **Relevance**: General plugin control surface — any engine that needs physical hardware feel

### 2.3 Figma Kit 04 — Full Synthesizer UI
- **Location**: `~/Downloads/Audio UI/FIGMA-KITS/Figma-kit-04/`
- **Contains**: Background, Big Button, Big Knob, Middle Knob, Small Knob, Slider, Small Button, Square Button (Gray + Green variants), Vent, VU Meter
- **Style**: Industrial dark panel with colored buttons
- **Status**: REVIEW
- **Relevance**: Most complete single-synth kit in the collection. VU Meter directly usable.

### 2.4 Figma Kit 05 — Dark Minimal Synth
- **Location**: `~/Downloads/Audio UI/FIGMA-KITS/Figma-Kit-05/`
- **Contains**: Background, Buttons, Knobs, Slider, Preset Tab, Light/LED indicators
- **Style**: Ultra-minimal dark, almost monochromatic
- **Status**: REVIEW
- **Relevance**: Minimalist engine surfaces (XOracleDeep, XObsidian aesthetic)

### 2.5 Figma Kit 06 — Classic Hardware Synth
- **Location**: `~/Downloads/Audio UI/FIGMA-KITS/Figma-Kit-06/`
- **Contains**: Big Button, Big Knobs, Light indicators, Middle Knobs, Panels, Port/connector graphics, Slider, Small Button, Small Knobs
- **Style**: Vintage hardware with port/connector details
- **Status**: REVIEW
- **Relevance**: Retro-synth surfaces; XOverworld (chip synth) or XOrbweave

### 2.6 Figma Kit — Mobile Application Audio GUI
- **Location**: `~/Downloads/Audio UI/FIGMA-KITS/Mobile_Application_Audio_GUI/`
- **Contains**: Arrow controls, Buttons, Knobs, Other controls, Sliders, VU Meter, Wheel (mod wheel)
- **Style**: Mobile-optimized dark UI, touch-friendly hit targets
- **Status**: REVIEW
- **Relevance**: iOS AUv3/Standalone app — this is the primary mobile reference. Touch targets, thumb zones, haptic-friendly controls.
- **Note**: Includes mod wheel — directly applicable to iOS XOmnibus expression controls

### 2.7 Solar-Gray Kit 07 — Minimal Light/Gray Theme
- **Location**: `~/Downloads/Audio UI/FIGMA-KITS/Solar-Gray-Kit-07/`
- **Contains**: Buttons, Knobs, ModWheel, Panels, Sliders
- **Style**: Light gray / silver — contrasts with the dark kits
- **Status**: REVIEW
- **Relevance**: Gallery Model light shell exploration; "light mode" plugin surface experiments

---

## 3. FIGMA KITS — COMMUNITY & THIRD-PARTY

> Full details including node IDs are in the companion file: `Docs/figma-asset-compendium.md`

| # | Name | Figma File ID | Relevance | Primary Use | Status |
|---|------|--------------|-----------|-------------|--------|
| 1 | **Game UX Kit** (Dismantle Studio) | `xo643maNsuCFslvd9DaZ7k` | ★★★★★ | Typography system, button state matrix, color methodology, spacing scale, 12-col grid | ADOPTED — design system foundations |
| 2 | **Lean Mantine Library** (Alley Corp Nord) | `3jtGAzg3jCdmcgAIhJblYc` | ★★★★★ | All form inputs (7 states), sliders, dark mode palette, modals, tables, progress bars | ADOPTED — production component reference |
| 3 | **FLOW V.4.0** | `yzqQkVf2alXAsF4jweZ3Rl` | ★★★★☆ | Toast/notification patterns, page templates, audio player wireframes, empty states | ADOPTED — notification + page patterns |
| 4 | **iOS App Wireframes** | `Ch2tom2TeEb73GKDmr7z9j` | ★★★★☆ | Mobile layouts, 4-tab bottom nav, grid card patterns, settings screens | REFERENCE — iOS app design |
| 5 | **Wireframing Starter Kit** | `9DzPp9y4kWcqDy7UeOTYXw` | ★★★☆☆ | HeroIcons (160+), Tabler Icons media controls, audio player controls | REFERENCE — icon source |
| 6 | **Helio Wireframe Kit** | `Su4kqol0vZyExOdzRJCu5h` | ★★★☆☆ | Feather Icons (280+) — CPU, database, sliders, layers icons | REFERENCE — icon fallback set |
| 7 | **UX Research Kit** | `ApLBqJ9GdX4R4tLy0iRx8P` | ★★★☆☆ | Persona templates, journey mapping, competitive analysis frameworks | REFERENCE — product strategy |
| 8 | **BRIX Website Wireframes** | `IzeCoEbEt5QU14lF0rr0iF` | ★★☆☆☆ | Hero sections, feature grids, pricing tables | REFERENCE — XO-OX.org |
| 9 | **Ant UX Wireframes** | `641CwZJyZKKYN9FOdsyTRo` | ★★☆☆☆ | Layout templates (2-col, 3-col, left-col) | REFERENCE — layout structure |

### 3.1 Additional .fig Files in Downloads

| Name | Location | Source | Status | Notes |
|------|----------|--------|--------|-------|
| **AiDEA – Smart SaaS Dashboard** | `~/Downloads/AiDEA – Smart SaaS Dashboard UI Kit.fig` | Ui8 | REVIEW | Dashboard components; relevant to Outshine pipeline view |
| **Aura AI Assistant Mobile App** | `~/Downloads/Aura Ai Assistant Mobile App UI Kit.fig` | Ui8 | REVIEW | Mobile AI assistant patterns; iOS XOmnibus UX reference |
| **Floma – Go Fluent** | `~/Downloads/Floma - Go Fluent Go Floma.fig` | Ui8 | REVIEW | Motion/animation-first design system |
| **Kids Game UI** | `~/Downloads/Kids Game.fig` | Unknown | REFERENCE | Playful UI patterns; may inspire aquarium interactivity |
| **Source Fusion AI 1.1** | `~/Downloads/Source Fusion AI 1.1.fig` | Ui8 | REVIEW | AI interface patterns; Originate export flow |
| **Spin Wheel Telegram Game Vol 3** | `~/Downloads/Spin Wheel  Telegram game Vol 3.fig` | Unknown | REFERENCE | Rotary interaction patterns |
| **Stacks Design System** | `~/Downloads/stacks-design-system.fig` | Ui8 | REVIEW | Full design system; dark mode components |
| **Ultimate Minimal Wireframe Components** | `~/Downloads/Ultimate Minimal Wireframe Components.fig` | Ui8 | REFERENCE | Minimal wireframe library |
| **Ultimate Pack of HERO Gradients** | `~/Downloads/Ultimate pack of HERO gradients.fig` | Ui8 | REVIEW | Gradient swatches in Figma format |
| **Caser UI Kit** | `~/Downloads/caser-ui-kit_NjliZmQ4YWYzNzVhNWYwMDk1NzM2MDE0/Caser UI Kit.fig` | Ui8 | REVIEW | Full UI kit with Sketch/XD variants |
| **UI.win All-in-One Atomic Design System** | `~/Downloads/uiwin-all-in-one-atomic-design-system-ui-kits_NjliZmQ4YWYzNzVhNWYwMDk1NzM2MDE0/` | Ui8 | REVIEW | Atomic design system; token-first architecture |

---

## 4. KNOB / CONTROL LIBRARIES

> Source: audio-ui.com (Samy) — commercial purchase
> Base path: `~/Downloads/Audio UI/KNOBS/`
> Format: All are PNG filmstrips (typically 100 frames at various sizes — exact frame count varies per knob)
> JUCE: Load with `juce::ImageCache::getFromFile()`, calculate frame index as `(int)(normalizedValue * (numFrames - 1))`

### 4.1 Individual Knob Catalogue (93 styles)

Each subdirectory contains: `[name]-Filmstrip.png` or `final-[n].png` + an `assets/` folder with individual frames + OBJ 3D model + shadow variants.

**Notable multi-colorway knobs** (multiple Filmstrip variants per knob):
- **Knob-100**: 5 colorways — Black, Gray, Green, Orange, Red filmstrips
- **T&V Knob-81**: Texture & Variation variant

**Full list by number**:

| Number | Style Notes | Colorways | Location |
|--------|-------------|-----------|----------|
| 01 | Minimal flat dark | 1 | `KNOBS/knob-01/` |
| 02 | Metal top indicator | 1 | `KNOBS/Knob-02/` |
| 03 | Rounded rubber cap | 1 | `KNOBS/knob-03/` |
| 05 | Contoured metal | 1 | `KNOBS/knob-05/` |
| 06 | Deep concave center | 1 | `KNOBS/knob-06/` |
| 07 | Flat topped, ridged | 1 | `KNOBS/knob-07/` |
| 09 | Classic instrument knob | 1 | `KNOBS/knob-09/` |
| 10 | Large format encoder | 1 | `KNOBS/knob-10/` |
| 11 | Flat center, dark rim | 1 | `KNOBS/Knob-11/` |
| 12 | Metal brushed cap | 1 | `KNOBS/Knob-12/` |
| 14 | Ribbed rubber | 1 | `KNOBS/Knob-14/` |
| 15 | Vintage amp style | 1 | `KNOBS/Knob-15/` |
| 16 | Gloss top | 1 | `KNOBS/knob-16/` |
| 17 | Industrial encoder | 1 | `KNOBS/knob-17/` |
| 18 | Low-profile dark | 1 | `KNOBS/knob-18/` |
| 19 | Chrome ring | 1 | `KNOBS/knob-19/` |
| 20 | Classic black | 1 | `KNOBS/knob-20/` |
| 21 | Flat matte | 1 | `KNOBS/Knob-21/` |
| 23 | Screw-top indicator | 1 | `KNOBS/Knob-23/` |
| 24 | Large center dot | 1 | `KNOBS/Knob-24/` |
| 25 | Curved sides | 1 | `KNOBS/Knob-25/` |
| 26 | Wide flat | 1 | `KNOBS/Knob-26/` |
| 27 | Brushed aluminum | 1 | `KNOBS/Knob-27/` |
| 28 | Soft rubber tactile | 1 | `KNOBS/knob-28/` |
| 29 | High gloss | 1 | `KNOBS/knob-29/` |
| 30 | Classic pointer | 1 | `KNOBS/Knob-30/` |
| 31 | Metal dome | 1 | `KNOBS/knob-31/` |
| 32 | Hexagonal | 1 | `KNOBS/knob-32/` |
| 33 | Compact round | 1 | `KNOBS/knob-33/` |
| 34 | Deep knurled | 1 | `KNOBS/knob-34/` |
| 35 | Flat wide cap | 1 | `KNOBS/knob-35/` |
| 41 | Wide-brimmed dark | 1 | `KNOBS/knob-41/` |
| 42 | Rubber dimple | 1 | `KNOBS/knob-42/` |
| 43 | Machined metal | 1 | `KNOBS/knob-43/` |
| 44 | Ridge-top | 1 | `KNOBS/knob-44/` |
| 45 | Narrow encoder | 1 | `KNOBS/knob-45/` |
| 46 | Recessed indicator | 1 | `KNOBS/knob-46/` |
| 47 | Grooved sides | 1 | `KNOBS/knob-47/` |
| 48 | Flat matte wide | 1 | `KNOBS/knob-48/` |
| 49 | Angled cap | 1 | `KNOBS/knob-49/` |
| 50 | Classic vintage | 1 | `KNOBS/Knob-50/` |
| 51 | Tactile bump | 1 | `KNOBS/knob-51/` |
| 52 | High-contrast dark | 1 | `KNOBS/knob-52/` |
| 53 | Pointer line indicator | 1 | `KNOBS/knob-53/` |
| 54 | Low flat profile | 1 | `KNOBS/knob-54/` |
| 55 | Flat with chrome ring | 1 | `KNOBS/knob-55/` |
| 56 | Dark matte dome | 1 | `KNOBS/knob-56/` |
| 57 | Contoured rubber | 1 | `KNOBS/knob-57/` |
| 58 | Large industrial | 1 | `KNOBS/knob-58/` |
| 59 | Narrow profile | 1 | `KNOBS/knob-59/` |
| 60 | Ridged cylinder | 1 | `KNOBS/knob-60/` |
| 61 | Deep set center | 1 | `KNOBS/knob-61/` |
| 62 | Wide machined | 1 | `KNOBS/knob-62/` |
| 63 | Classic round top | 1 | `KNOBS/knob-63/` |
| 64 | Angular facets | 1 | `KNOBS/knob-64/` |
| 65 | Elongated encoder | 1 | `KNOBS/knob-65/` |
| 66 | Flat-top metal | 1 | `KNOBS/knob-66/` |
| 67 | Curved profile | 1 | `KNOBS/knob-67/` |
| 69 | Soft dome | 1 | `KNOBS/knob-69/` |
| 70 | Pinch grip | 1 | `KNOBS/knob-70/` |
| 71 | Dark chrome | 1 | `KNOBS/knob-71/` |
| 72 | Flat with LED ring (implied) | 1 | `KNOBS/knob-72/` |
| 73 | Tall narrow | 1 | `KNOBS/knob-73/` |
| 74 | Retro pointer | 1 | `KNOBS/knob-74/` |
| 75 | Wide ridged | 1 | `KNOBS/Knob-75/` |
| 76 | Low-profile flat | 1 | `KNOBS/knob-76/` |
| 77 | Asymmetric tip | 1 | `KNOBS/knob-77/` |
| 78 | Classic black round | 1 | `KNOBS/knob-78/` |
| 79 | Smooth dome | 1 | `KNOBS/knob-79/` |
| 80 | Wide-cap vintage | 1 | `KNOBS/knob-80/` |
| 81 (T&V) | Texture & Variation | 1 (specialty) | `KNOBS/T&V knob-81/` |
| 82 | Angled wedge | 1 | `KNOBS/knob-82/` |
| 83 | Compact encoder | 1 | `KNOBS/knob-83/` |
| 85 | Chrome dome | 1 | `KNOBS/knob-85/` |
| 86 | Industrial collar | 1 | `KNOBS/knob-86/` |
| 87 | Wide flat metal | 1 | `KNOBS/knob-87/` |
| 88 | High gloss black | 1 | `KNOBS/Knob-88/` |
| 91 | Matte rounded | 1 | `KNOBS/knob-91/` |
| 92 | Narrow chrome | 1 | `KNOBS/knob-92/` |
| 93 | Classic studio | 1 | `KNOBS/Knob-93/` |
| 94 | Flat ribbed | 1 | `KNOBS/knob-94/` |
| 95 | Precision indicator | 1 | `KNOBS/Knob-95/` |
| 96 | Wide vintage | 1 | `KNOBS/Knob-96/` |
| 97 | Compact metal | 1 | `KNOBS/knob-97/` |
| 98 | Chrome with groove | 1 | `KNOBS/knob-98/` |
| 99 | Dark with pointer | 1 | `KNOBS/knob-99/` |
| 100 | Large multi-color | 5 (B/Gr/Gn/Or/R) | `KNOBS/Knob-100/` |
| 101 | Premium studio | 1 | `KNOBS/Knob-101/` |
| 102 | Flat matte premium | 1 | `KNOBS/Knob-102/` |
| 103 | Precision encoder | 1 | `KNOBS/Knob-103/` |
| 104 | Contoured wide | 1 | `KNOBS/Knob-104/` |
| 105 | Dark machined | 1 | `KNOBS/Knob-105/` |
| 106 | Studio standard | 1 | `KNOBS/Knob-106/` |

---

### 4.2 Knob Sets (9 coordinated collections, 5 knobs per set)

Each set contains knob-01 through knob-05 as a coordinated family — use a full set when you want visual cohesion across all controls in an engine panel.

| Set | Style Description | Filmstrip format | Location |
|-----|-------------------|-----------------|----------|
| **Set 01** | Consistent dark hardware family | PNG per knob | `KNOBS-SET/knob-set-01/` |
| **Set 02** | Matched metal family | PNG per knob | `KNOBS-SET/knob-set-02/` |
| **Set 03** | Dark industrial set | PNG per knob | `KNOBS-SET/knob-set-03/` |
| **Set 04** | Packaged set (ZIP included) | PNG per knob | `KNOBS-SET/Knob-set-04/` |
| **Set 05** | Machined aluminum family | PNG per knob | `KNOBS-SET/knob-set-05/` |
| **Set 06** | Premium studio set | PNG per knob | `KNOBS-SET/knob-set-06/` |
| **Set 07** | Compact controls family | PNG per knob | `KNOBS-SET/knob-set-07/` |
| **Set 08** | Filmstrip-optimized set (named filmstrips: Knob-01/02/03-Filmstrip.png) | Proper filmstrips | `KNOBS-SET/knob-set-08/` |
| **Set 09** | Premium multi-size family (named Knob-01/02/03-Filmstrip.png) | Proper filmstrips | `KNOBS-SET/knob-set-09/` |

**Note on Sets 08 and 09**: These are the most JUCE-ready — they ship with properly named `*-Filmstrip.png` files alongside OBJ 3D sources and shadow masks.

---

## 5. BUTTON LIBRARIES

> Source: audio-ui.com — commercial purchase
> Base path: `~/Downloads/Audio UI/BUTTONS/`
> 8 button styles total

| Style | Contents | Format | Location |
|-------|----------|--------|----------|
| **Button-01** | Nickel variant, Silver variant | PNG frames (Nickel/ Silver/ subdirs) | `BUTTONS/Button-01/` |
| **Button-02** | Main state + secondary variant (Button-02.1) | PNG | `BUTTONS/Button-02/` |
| **Button-03** | Single style | PNG | `BUTTONS/Button-03/` |
| **Button-04** | Single style | PNG | `BUTTONS/Button-04/` |
| **Button-05** | Single style | PNG | `BUTTONS/Button-05/` |
| **Button-06** | Single style | PNG | `BUTTONS/Button-06/` |
| **Button-07** | Single style | PNG | `BUTTONS/Button-07/` |
| **Button-08** | Single style | PNG | `BUTTONS/Button-08/` |

**JUCE path**: Use `juce::Image` with index 0 = off, index 1 = on (or more frames for hover/disabled states). Wrap in custom `juce::Button` with `paintButton()` override.

---

## 6. SLIDER LIBRARIES

> Source: audio-ui.com — commercial purchase
> Base path: `~/Downloads/Audio UI/SLIDERS/`
> 3 slider styles (metal + 2 plastic families)

| Style | Type | Contents | Location |
|-------|------|----------|----------|
| **Metal Slider Set 02** | Vertical/horizontal metal | Track + thumb filmstrips | `SLIDERS/metal-slider-set-02/` |
| **Plastic Slider Set 01** | Plastic light style | Track + thumb components | `SLIDERS/Plastic-Slider-set-01/` |
| **Plastic Slider Set 03** | Plastic alternate finish | Track + thumb components | `SLIDERS/Plastic slider-set-03/` |

**JUCE path**: Custom `juce::Slider` with `LookAndFeel::drawLinearSlider()` override. Draw track and thumb from `juce::Image` — use `thumb` image at position `trackStart + (normalizedValue * trackLength)`.

---

## 7. VU METERS & LEVEL INDICATORS

> Source: audio-ui.com — commercial purchase
> Base path: `~/Downloads/Audio UI/VU-METER/`
> 4 VU meter styles

| Style | Contains | Style Notes | Location |
|-------|----------|-------------|----------|
| **VU Meter 01 (final-vm-01)** | Multiple frame variants | Classic analog needle VU | `VU-METER/final-vm-01/` |
| **VU Meter 02** | Frame sequence | Backlit bar-style meter | `VU-METER/vu-meter-02/` |
| **VU Meter 03** | Frame sequence | Modern LED-segment style | `VU-METER/vu-meter-03/` |
| **VU Meter 04** | Frame sequence | Compact bar graph | `VU-METER/VU-Meter-04/` |

**JUCE path**: Drive meter level from `AudioProcessorValueTreeState` via `AsyncUpdater` (never paint from audio thread). Use `juce::Image` frame selection based on dB value mapped 0.0–1.0.

---

## 8. AUDIO UI COMPONENT KITS (COMPLETE)

> Source: audio-ui.com — commercial purchase
> Base path: `~/Downloads/Audio UI/KITS/`
> 12 complete multi-component kits (Kit-01, 03, 04, 06, 07, 14, 16, 17, 20, 25, 28, 30)

Each kit contains a coordinated set of: background panel, knobs (big + small or big + middle + small), buttons, and often sliders — designed to work together as a unified control surface.

| Kit | Components | Style Notes | Location |
|-----|-----------|-------------|----------|
| **Kit-01** | Big Knob, Small Knob, Button, H-Slider, V-Slider, Mod Wheel, VU Meter, Background (zipped) | Dark metal synthesizer | `KITS/Kit-01/` |
| **Kit-03** | Background (3 variants), Big Knob, Middle Knob, Small Knob, Button | Dark industrial panel | `KITS/kit-03/` |
| **Kit-04** | Background, Big Knob (blue + green variants), Small Knob, Button, Panel, Pedal, Lights (blue + green) | Colorful hardware with LED indicators | `KITS/kit-04/` |
| **Kit-06** | Background, Big Knob, Small Knob, Button, Slider | Clean dark minimal | `KITS/kit-06/` |
| **Kit-07** | Background, Big Knob, Light Knob (illuminated variant), Small Knob, Light indicators, VU Meter | Illuminated controls — excellent for active/inactive states | `KITS/kit-07/` |
| **Kit-14** | Background, Glass elements, Big Knob, Small Knob, Button, Slider | Glass/translucent surfaces | `KITS/kit-14/` |
| **Kit-16** | Background, Big Knob, Small Knob, Big Button, Small Button, Slider | Dark studio standard | `KITS/kit-16/` |
| **Kit-17** | Background, Big Knob, Small Knob, LED indicators, Button | LED-centric dark panel | `KITS/kit-17/` |
| **Kit-20** | Background, Big Knob, Button, Glass elements, Slider, Small Knob | Glass + dark metal hybrid | `KITS/Kit-20/` |
| **Kit-25** | Background, Big Knob, Button, Slider, Small Knob | Standard dark hardware | `KITS/kit-25/` |
| **Kit-28** | Background, Big Knob, Middle Knob, Small Knob, Button, Slider, Wheel | Full-featured compact kit | `KITS/kit-28/` |
| **Kit-30** | Background, Big Knob, Small Knob, Slider | Minimal 4-component set | `KITS/Kit-30/` |

**Recommended kits by XOmnibus use case**:
- **PlaySurface macro knobs**: Kit-07 (illuminated) or Kit-01 (metal)
- **Engine preset selector**: Kit-17 (LED) or Kit-14 (glass)
- **iOS AUv3 app**: Kit-04 (colorful, distinct states) or Kit-14 (glass, premium)

---

## 9. GRADIENT PACKS

| Name | Count | Format | Theme | Relevance | Status | Location |
|------|-------|--------|-------|-----------|--------|----------|
| **Hero Gradients v1** | 36 images | PNG + JPG | Abstract hero backgrounds | XO-OX.org hero sections, engine card backgrounds | REVIEW | `~/Downloads/Hero Gradients v1/` |
| **Hero Gradients v2** | 32 images | PNG + JPG | Abstract hero — v2 variants | Continuation of v1; more options for site sections | REVIEW | `~/Downloads/Hero Gradients v2/` |
| **Galactic Gradients** | 1 folder + ~28 images | PNG + JPG | Space/cosmic blue-purple gradients | Deep ocean / abyss engine aesthetics; XOceanDeep, XOabyss backgrounds | REVIEW | `~/Downloads/Galactic Gradients/` |
| **Cubic Glass Gradient** | Small set | PNG | Glassmorphism cubes | Glass-panel UI surface exploration; XOmnibus glass effects | REVIEW | `~/Downloads/Cubic Glass Gradient/` |
| **Holographic Series 13–24** | 12 images | PNG | Holographic foil textures | Premium material backgrounds; special edition UI surfaces | REVIEW | `~/Downloads/templify_holographic_13-24_NjliZmQ4YWYzNzVhNWYwMDk1NzM2MDE0/` |
| **Holographic Series (set 2)** | 12 images | PNG | Holographic variant set | Same series, additional frames | REVIEW | `~/Downloads/templify_holographic_13-24_NjliZmQ4YWYzNzVhNWYwMDk1NzM2MDE0(1)/` |
| **Ultimate Pack of HERO Gradients** | Figma source | `.fig` | Curated gradient library with Figma swatches | Full gradient system in Figma; best for design token extraction | REVIEW | `~/Downloads/Ultimate pack of HERO gradients.fig` |
| **Raw Gradients** | 1 image | PNG | Raw gradient mesh | Background texture experiment | REFERENCE | `~/Downloads/raw-gradients-jotepq9z.png` |

**JUCE translation**: Gradients implemented as `juce::ColourGradient`. Extract hex values from PNG reference images. For Oklch-perceptual interpolation, convert stops to Oklch before blending in `ColourGradient::addColour()`.

---

## 10. ICON SETS

### 10.1 Figma-native icon sets (in community kits — see §3)

| Set | Count | Style | Source | Node ID |
|-----|-------|-------|--------|---------|
| **HeroIcons** | 160+ | Outlined 24×24 | Wireframing Starter Kit | `1418:488` |
| **Tabler Icons** (media player subset) | ~20 player controls | Outlined | Wireframing Starter Kit | same file |
| **Feather Icons** | 280+ | Outlined 24×24 | Helio Wireframe Kit | `54:386` |
| **SF Symbols** | System (iOS/macOS native) | System | Native — no file needed | — |

### 10.2 Standalone Icon Sets

| Name | Count | Style | Format | Relevance | Status | Location |
|------|-------|-------|--------|-----------|--------|----------|
| **Iconizer Pro v1.0.1** | 70+ (35 Duo Line + 35 Solid) + Figma | Duo-tone + Solid | SVG + `.fig` | General UI iconography; Outshine toolbar, Originate export panel | REVIEW | `~/Downloads/Iconizer-Pro-v-1-0-1-1/` |
| **Uicon Animated Icons Vol 4** | ~75 animated | Animated (Lottie/SVG) | SVG/GIF variants | Micro-interactions: loading states, success animations, XPN export completion | REVIEW | `~/Downloads/Uicon Animated Icons Vol4/` |
| **Ocean's Nostalgia — Fishing Trip Icons** | Small set | Illustrated | PNG | Thematic ocean/fishing icons | REFERENCE (aquarium branding) | `~/Downloads/Ocean's Nostalgia - Fishing Trip Icons/` |
| **Ocean's Nostalgia — Icons Pack 1** | Small set | Illustrated | PNG | Ocean/aquatic theme icons | REFERENCE (aquarium branding) | `~/Downloads/Ocean's Nostalgia - Icons Pack 1/` |
| **LED Board** | Full alphanumeric A-Z, 0-9, symbols | Bitmap LED segments | PNG (off + on per character) | 7-segment display readouts; BPM/tempo display; XOverworld chip display | REVIEW | `~/Downloads/LEDboard/` |

**Priority guidance**:
1. **SF Symbols** — always first for native macOS/iOS elements
2. **HeroIcons** (Wireframing Starter Kit) — primary web/cross-platform set
3. **Feather Icons** (Helio) — fallback; broader category coverage
4. **Iconizer Pro** — for UI-specific needs not covered by above
5. **Uicon Animated** — micro-interactions only (not static icons)

---

## 11. UI COMPONENT KITS & DESIGN SYSTEMS

### 11.1 Complete UI Essential Pack v2.4

- **Location**: `~/Downloads/Complete_UI_Essential_Pack_v2.4/`
- **Source**: Commercial purchase
- **Contains**: 13 themed component sets:
  - Flat, Paper, Wood, Stone, Metal, Hologram, Gradient, Glass, Pumpkin, MysticWood, Papernote, Metalworks, Runewood
  - Plus: Cursors, Misc, License
- **Relevance**: The Glass (08), Metal (05), Hologram (06) themes directly align with XOmnibus aesthetic. Stone (04) and Wood (03) relevant for organic engine surfaces.
- **Status**: REVIEW
- **Format**: Sprite sheets per component per theme

### 11.2 UI HUD Kit v1.2.1

- **Location**: `~/Downloads/UI HUD/` and `~/Downloads/UI HUD_1.2.1.zip`
- **Source**: Commercial purchase
- **Contains**: Board, Buttons, Clocks, HUD panels, Icons, Notifications, Scrollbar
- **Relevance**: HUD-style overlays for PlaySurface performance mode; Coupling crossfader overlay
- **Status**: REVIEW
- **Format**: PNG sprite sheets

### 11.3 Futuristic UI Kit (Samolevsky — 200 elements)

- **Location**: `~/Downloads/samolevskycom-futuristic-ui-kit-200-design-elements_NjliZmQ4YWYzNzVhNWYwMDk1NzM2MDE0.zip`
- **Source**: Ui8
- **Contains**: 200 futuristic UI elements (ZIP — not yet unzipped)
- **Relevance**: Sci-fi/futuristic chrome for XOsmosis, XOrbweave, XOptic engine UIs
- **Status**: REVIEW (zip not yet extracted)

### 11.4 Caser UI Kit

- **Location**: `~/Downloads/caser-ui-kit_NjliZmQ4YWYzNzVhNWYwMDk1NzM2MDE0/`
- **Contains**: Full UI kit in `.fig`, `.sketch`, and `.xd` formats + Poppins font
- **Relevance**: Cross-tool design workflow; contains Poppins if needed for web-only contexts
- **Status**: REVIEW

### 11.5 UI.win All-in-One Atomic Design System

- **Location**: `~/Downloads/uiwin-all-in-one-atomic-design-system-ui-kits_NjliZmQ4YWYzNzVhNWYwMDk1NzM2MDE0/`
- **Contains**: Atomic design system `.fig` file
- **Relevance**: Design token architecture reference; component hierarchy patterns
- **Status**: REVIEW

### 11.6 Recording Studio UI Kit (Ezyzip)

- **Location**: `~/Downloads/ezyzip_NjliZmQ4YWYzNzVhNWYwMDk1NzM2MDE0/`
- **Contains**: `Recording Studio UI Pack.fig` + `Studio Record UI/` folder
- **Relevance**: HIGHEST relevance to audio product design — recording studio panels directly applicable to XOmnibus transport controls, Outshine layout
- **Status**: REVIEW (evaluate this first among the Ui8 fig files)

---

## 12. DESIGN SYSTEM — XO_OX SOURCE OF TRUTH

> These files ARE the canonical XO_OX design system. All other assets must reconcile to these.

### 12.1 Core Token Files

| File | Path | Status | Notes |
|------|------|--------|-------|
| **design-tokens.css** | `~/Documents/GitHub/XO_OX-XOmnibus/Site/design-tokens.css` | CANONICAL | 10 sections: Global Brand, Depth Zones, 71 Engine Colors, Anatomy, Glass Formula, PlaySurface, Typography, Type Scale (10 tokens), Spacing, Animation |
| **engine-creature-map.json** | `~/Documents/GitHub/XO_OX-XOmnibus/Site/engine-creature-map.json` | CANONICAL | Engine name → mythology mapping → accent color |

### 12.2 Design Documentation (all in `Docs/design/`)

| File | Path | Purpose |
|------|------|---------|
| **Master UI Spec v2** | `Docs/design/xomnibus_ui_master_spec_v2.md` | Complete component specs, all panels |
| **Design Guidelines** | `Docs/design/xomnibus_design_guidelines.md` | Methodology and constraints |
| **Accessibility Audit** | `Docs/design/accessibility-audit.md` | WCAG compliance notes |
| **Button System Spec** | `Docs/design/button-system-spec.md` | Full button state matrix |
| **Input State Matrix** | `Docs/design/input-state-matrix.md` | 7-state form input system |
| **Toast Notification System** | `Docs/design/toast-notification-system.md` | 5-status toast patterns |
| **Outshine Empty State** | `Docs/design/outshine-empty-state.md` | First-launch UX |

### 12.3 Design Tokens Quick Reference

**Brand Colors**:
- `--xo-bg: #0E0E10` — deepest layer
- `--xo-bg-panel: #141416` — panel background
- `--xo-bg-card: #1A1A1E` — card / tile
- `--xo-bg-raised: #1F1F24` — raised element
- `--xo-gold: #E9C46A` — global accent
- `--xo-shell: #F8F6F3` — Gallery Model light shell
- `--xo-border: rgba(255,255,255,0.07)` — universal border
- `--xo-text-primary: #F0EDE8`
- `--xo-text-muted: rgba(240,237,232,0.45)`
- `--xo-text-dim: rgba(240,237,232,0.20)`

**Depth Zone Colors**:
- `--zone-sunlit: #48CAE4` (0–55% depth)
- `--zone-twilight: #0096C7` (55–80% depth)
- `--zone-midnight: #7B2FBE` (80–100% depth)

**Typography**:
- Space Grotesk SemiBold 20pt — window titles
- Space Grotesk SemiBold 11pt ALL CAPS +0.08em — section headers
- Space Grotesk SemiBold 13pt — tab labels
- Space Grotesk Regular 13pt — engine names / zone labels
- Inter Regular 12pt — body copy, descriptions, tooltips
- Inter Medium 11pt — form field labels, inline metadata
- JetBrains Mono Regular 11pt — all numeric values

---

## 13. HTML PROTOTYPES & MOCKUPS

| File | Path | Description | Status |
|------|------|-------------|--------|
| **XOmnibus Main UI Mockup v3** | `~/Documents/GitHub/XO_OX-XOmnibus/Docs/mockups/xomnibus-main-ui.html` | Interactive HTML prototype, Rounds 51–120. Full engine data model, knob drag, oscilloscope canvas, coupling matrix, ADSR paths, preset navigation, dark/light mode toggle. Uses Space Grotesk + Inter + JetBrains Mono via Google Fonts CDN. | ADOPTED — primary interactive reference |
| **Outshine Prototype** | `~/Documents/GitHub/audio-xpm-creator/Docs/mockups/outshine-prototype.html` | OUTSHINE window wireframe | REVIEW |

---

## 14. REFERENCE SCREENSHOTS

> All stored at `~/` (home directory root). 49 total files.

### 14.1 XO-OX.org Evolution (aquarium, hero, instruments, site pages)

See full catalogue in `Docs/figma-asset-compendium.md` Section C — 49 files covering:
- Aquarium & Aquatic series (9 files)
- Manifesto series (6 files)
- PlaySurface iterations (6 files)
- XO Branding & Hero (5 files)
- Instruments & Content (9 files)
- Site Pages & Guide (10 files)
- Signal Feed & Other (5 files — includes `full-page-9.8.png`, `hero-section-final.png`)

All files follow naming convention: `[section]-[descriptor].png` at `~/[filename].png`

### 14.2 Specialty Reference Images

| File | Theme | Notes |
|------|-------|-------|
| `~/aquarium-water-column.png` | Depth zone visualization | Engine placement by depth zone — reference for spatial navigation |
| `~/playsurface-zones.png` | PlaySurface zone map | Zone boundary definitions |
| `~/manifesto-coupling.png` | Coupling concept | Visual language for coupling relationships |
| `~/instrument-demo-pads-zoom.png` | Pad detail | Pad grid close-up reference |

---

## 15. AUDIO-SPECIFIC SPECIALTY ASSETS

### 15.1 Serum Skins (Reference only)

| Name | Format | Notes | Location |
|------|--------|-------|----------|
| **Dark Blue Serum 2 Skin** | ZIP | Serum 2 skin — reference for professional plugin visual language | `~/Downloads/Audio UI/SERUM/Dark-Blue-Serum2-Skin.zip` |
| **NEOWAVE 1000 Serum 2 Skins** | ZIP | Neowave skin pack — reference for premium synth UI vocabulary | `~/Downloads/Audio UI/SERUM/NEOWAVE-1000-Serum2-Skins.zip` |

**Status**: REFERENCE — do not copy; study for visual language and control conventions.

### 15.2 LED/Display Components

| Name | Contents | Relevance | Location |
|------|----------|-----------|----------|
| **LED Board** | Full character set (A-Z, 0-9, symbols) — each as on/off PNG pair | 7-segment displays, BPM counter, XOverworld chip interface | `~/Downloads/LEDboard/` |
| **Site LED characters** | Pre-rendered LED sprites | Already deployed in XO-OX.org panels | `~/Documents/GitHub/XO_OX-XOmnibus/Site/img/led/` |

### 15.3 Light Set

| Name | Contents | Location |
|------|----------|----------|
| **Light Set 01** | Light-01, Light-02, Light-03 indicator variants + Shadow layers | `~/Downloads/Audio UI/LIGHT/Light-Set-01/` |

**Relevance**: Active/inactive state indicators for engine slots, preset loaded state, coupling connection indicators.

### 15.4 Outshine & Originate Design Spec

- **Path**: `~/Documents/GitHub/audio-xpm-creator/DESIGN_SPECIFICATION_OUTSHINE_ORIGINATE.md`
- **Content**: 1,200+ lines — full wireframes (ASCII art), interaction design, font spec, color system, component library, animation spec, accessibility
- **Status**: ADOPTED — authoritative spec for both companion tools

---

## QUICK-ACCESS TABLES

### By Use Case

| Need | Asset | Location |
|------|-------|----------|
| **Macro knob (large)** | Knob-100 (5 colorways), Knob-93, Knob-50 | `~/Downloads/Audio UI/KNOBS/` |
| **Parameter knob (standard)** | Knob-Set-08 or Knob-Set-09 (proper filmstrips) | `~/Downloads/Audio UI/KNOBS-SET/` |
| **Small parameter knob** | Any single KNOBS/ entry | `~/Downloads/Audio UI/KNOBS/` |
| **Coordinated full-surface knobs** | Knob Sets 01–07 (5-knob families) | `~/Downloads/Audio UI/KNOBS-SET/` |
| **Level/VU meter** | VU Meter 01 (analog) or 03 (LED segment) | `~/Downloads/Audio UI/VU-METER/` |
| **Toggle/button** | Button-01 Nickel/Silver or Kit-07 buttons | `~/Downloads/Audio UI/BUTTONS/` |
| **Fader/slider** | Metal Slider Set 02 | `~/Downloads/Audio UI/SLIDERS/` |
| **Full plugin surface** | Kit-01 (dark metal) or Kit-14 (glass) | `~/Downloads/Audio UI/KITS/` |
| **Mobile iOS controls** | Mobile Application Audio GUI kit | `~/Downloads/Audio UI/FIGMA-KITS/Mobile_Application_Audio_GUI/` |
| **Window title font** | Space Grotesk SemiBold (Google Fonts) | Google Fonts CDN |
| **Numeric readout font** | JetBrains Mono Regular (Google Fonts) | Google Fonts CDN |
| **Display/nameplate font** | Nebulica Bold/ExtraBold | `~/Downloads/nebulica_.../OTF/` |
| **Brand accent color** | `--xo-gold: #E9C46A` | `Site/design-tokens.css` |
| **Engine accent colors** | 71 engine colors table | `Docs/figma-asset-compendium.md` Section B |
| **Button state matrix** | Game UX Kit node `47:211` | Figma: `xo643maNsuCFslvd9DaZ7k` |
| **Form input states (7)** | Lean Mantine Library `3050:2818` | Figma: `3jtGAzg3jCdmcgAIhJblYc` |
| **Toast notifications** | FLOW V.4.0 `7008:42828` | Figma: `yzqQkVf2alXAsF4jweZ3Rl` |
| **Icon set (general)** | HeroIcons (160+ outlined) | Wireframing Starter Kit: `9DzPp9y4kWcqDy7UeOTYXw` |
| **Icon set (audio/media)** | Tabler player icons | Same file, node `1418:488` |
| **Gradient backgrounds** | Hero Gradients v1 or v2 | `~/Downloads/Hero Gradients v1/` |
| **Glass surface gradient** | Cubic Glass Gradient | `~/Downloads/Cubic Glass Gradient/` |
| **Design system tokens** | design-tokens.css | `Site/design-tokens.css` |
| **iOS layout patterns** | iOS App Wireframes | Figma: `Ch2tom2TeEb73GKDmr7z9j` |
| **Recording studio UI** | Recording Studio UI Pack | `~/Downloads/ezyzip_.../Recording Studio UI Pack.fig` |
| **LEDs / 7-segment display** | LED Board character set | `~/Downloads/LEDboard/` |
| **Serum skin reference** | NEOWAVE Serum 2 Skins | `~/Downloads/Audio UI/SERUM/` |
| **Holographic texture** | Holographic Series 13–24 | `~/Downloads/templify_holographic_13-24_.../` |

### By Product Surface

| Product Surface | Primary Assets | Secondary |
|-----------------|---------------|-----------|
| **XOmnibus Engine Panel** | Kit-01 or Kit-07 knobs, design-tokens.css | Knob-Set-08/09 |
| **XOmnibus Coupling View** | PlaySurface screenshots, coupling-ui-architecture doc | Figma FLOW modals |
| **XOmnibus PlaySurface** | Kit-07 illuminated, Button-01 | Light Set-01 |
| **Outshine (plugin)** | design spec OUTSHINE_ORIGINATE.md, Metal Slider Set 02 | Mantine sliders |
| **Originate (export)** | design spec, FLOW toasts | Mantine modals |
| **XO-OX.org** | Nebulica, Chrys, Hero Gradients, aquarium screenshots | FLOW page templates |
| **iOS AUv3 app** | Mobile Audio GUI kit, iOS Wireframes (Figma) | Polly Rounded font |
| **audio-xpm-creator** | Game UX Kit, Mantine Library | Outshine spec |

---

## JUCE TRANSLATION NOTES (LUCY'S REFERENCE)

> This section maps asset types to concrete JUCE implementation patterns. Written from Lucy's perspective: pragmatic, complete, no hand-waving.

---

### A. Filmstrip Knobs → `juce::Image` + Frame Calculation

```cpp
// In your LookAndFeel subclass:
void drawRotarySlider(juce::Graphics& g,
                      int x, int y, int width, int height,
                      float sliderPos,              // 0.0f → 1.0f
                      float rotaryStartAngle,
                      float rotaryEndAngle,
                      juce::Slider& slider) override
{
    // Load filmstrip once (cache it — never load per paint call)
    auto filmstrip = juce::ImageCache::getFromMemory(
        BinaryData::Knob93_Filmstrip_png,
        BinaryData::Knob93_Filmstrip_pngSize);

    const int numFrames = 100;           // Most audio-ui.com knobs are 100 frames
    const int frameHeight = filmstrip.getHeight() / numFrames;
    const int frameIndex = juce::roundToInt(sliderPos * (numFrames - 1));

    // Source clip from filmstrip (vertical strip layout)
    auto sourceClip = juce::Rectangle<int>(
        0, frameIndex * frameHeight,
        filmstrip.getWidth(), frameHeight);

    g.drawImage(filmstrip, x, y, width, height,
                sourceClip.getX(), sourceClip.getY(),
                sourceClip.getWidth(), sourceClip.getHeight());
}
```

**Embed filmstrip in BinaryData**: Add PNG to JUCE `BinaryData` in the Projucer `Files` list. The generated name will be `Knob93_Filmstrip_png` (spaces/hyphens converted to underscores).

**Frame count**: audio-ui.com knobs vary — most individual KNOBS/ entries use ~100 frames. Knob-Set-08/09 explicitly label their files as Filmstrip. Measure actual frame count: `frameHeight = filmstrip.getHeight() / numFrames`. Start with 100; if the knob looks wrong, try 63 or 127.

**Shadow layers**: The `shadow/` and `shadow-100/` subdirectories in each knob folder contain pre-rendered drop shadows. Composite: draw shadow first (`drawImage` with opacity), then knob on top.

---

### B. SVG Icons → `juce::Drawable::createFromSVG()`

```cpp
// For HeroIcons / Feather Icons exported as SVG:
std::unique_ptr<juce::Drawable> icon;

void setupIcon()
{
    juce::XmlDocument xmlDoc(juce::String(BinaryData::icon_play_svg,
                                          BinaryData::icon_play_svgSize));
    auto xmlElement = xmlDoc.getDocumentElement();
    icon = juce::Drawable::createFromSVG(*xmlElement);
}

void paint(juce::Graphics& g) override
{
    if (icon != nullptr)
    {
        icon->setColour(juce::DrawableShape::fillColourId,
                        juce::Colour(0xFFE9C46A));  // XO Gold
        icon->drawWithin(g, getLocalBounds().toFloat(),
                         juce::RectanglePlacement::centred, 1.0f);
    }
}
```

**Tinting SVGs**: The `setColour()` call only works on SVGs that use `currentColor` as their fill. HeroIcons and Feather Icons both support this pattern. If tinting fails, export from Figma with `currentColor` fill explicitly set.

**For Uicon animated icons**: These are Lottie/CSS animations, not JUCE-native. Use only in web contexts (XO-OX.org, audio-xpm-creator). In JUCE, hand-roll equivalent `juce::ComponentAnimator` transitions.

---

### C. Gradient Packs → `juce::ColourGradient` with Oklch Interpolation

```cpp
// Extract stop colors from gradient PNG references using a color picker.
// For perceptual uniformity, convert hex stops to Oklch before blending.

void paintGradientBackground(juce::Graphics& g,
                              juce::Rectangle<float> bounds)
{
    // Example: Deep Ocean gradient (Galactic Gradients reference)
    // Stop 0: #0E0E10 (--xo-bg), Stop 1: #1B2838 (ORCA Deep Ocean), Stop 2: #7B2FBE (--zone-midnight)
    juce::ColourGradient gradient(
        juce::Colour(0xFF0E0E10),   // --xo-bg at top
        bounds.getTopLeft(),
        juce::Colour(0xFF2D0A4E),   // OCEANDEEP Trench Violet at bottom
        bounds.getBottomLeft(),
        false);                     // false = linear (not radial)

    // Add midpoint stop for depth zone color
    gradient.addColour(0.55, juce::Colour(0xFF0096C7));  // --zone-twilight

    g.setGradientFill(gradient);
    g.fillRect(bounds);
}
```

**Oklch note**: JUCE does not natively support Oklch interpolation. For perceptually uniform gradients (visible in Hero Gradients packs), either: (a) add intermediate stops extracted from the reference PNG, or (b) implement a manual Oklch → sRGB → `juce::Colour` conversion and sample 8–16 stops.

**Radial gradients for knob arcs**: Use `juce::ColourGradient(centerColor, cx, cy, edgeColor, cx + radius, cy, true)` (last param `true` = radial).

---

### D. Font Files → `juce::Typeface::createSystemTypefaceFor()` via BinaryData

```cpp
// In PluginProcessor.cpp or a shared FontCache singleton:

juce::Font getSpaceGroteskSemiBold(float size)
{
    static juce::Typeface::Ptr tf = juce::Typeface::createSystemTypefaceFor(
        BinaryData::SpaceGrotesk_SemiBold_ttf,
        BinaryData::SpaceGrotesk_SemiBold_ttfSize);

    return juce::Font(tf).withHeight(size);
}

juce::Font getJetBrainsMono(float size)
{
    static juce::Typeface::Ptr tf = juce::Typeface::createSystemTypefaceFor(
        BinaryData::JetBrainsMono_Regular_ttf,
        BinaryData::JetBrainsMono_Regular_ttfSize);

    return juce::Font(tf).withHeight(size);
}
```

**Critical**: Use `static` for the `Typeface::Ptr` — typeface creation is expensive. Create once, reuse forever.

**Which format to embed**: Prefer TTF over OTF for maximum JUCE compatibility on all platforms (macOS Core Graphics, Windows Direct2D, Linux Cairo). WOFF/WOFF2 are web-only and will not load in JUCE BinaryData.

**For the HINO glyph fonts**: These are specialty display fonts — only embed if you are rendering actual glyph characters via `g.drawText()`. For decorative purposes, consider pre-rendering to `juce::Image` instead.

**Font availability by asset**:
- Space Grotesk TTF: Google Fonts download → add to BinaryData
- Inter TTF: Google Fonts download → add to BinaryData
- JetBrains Mono TTF: Google Fonts download → add to BinaryData
- Nebulica OTF: `~/Downloads/nebulica_.../OTF/Nebulica-Bold.otf` → convert to TTF for JUCE
- Chrys TTF: `~/Downloads/Audio UI/KNOBS/` — wait, check: `~/Documents/GitHub/XO_OX-XOmnibus/Site/fonts/Chrys-Regular.woff` — need source OTF from `~/Downloads/chrys_.../OTF/`

---

### E. Figma Design Tokens → JUCE `juce::Colour` / `juce::Font` Constants

```cpp
// DesignTokens.h — translate design-tokens.css to JUCE constants
// Source of truth: Site/design-tokens.css

namespace XOTokens
{
    // Backgrounds
    static const juce::Colour bg          { 0xFF0E0E10 };
    static const juce::Colour bgPanel     { 0xFF141416 };
    static const juce::Colour bgCard      { 0xFF1A1A1E };
    static const juce::Colour bgRaised    { 0xFF1F1F24 };

    // Text
    static const juce::Colour textPrimary { 0xFFF0EDE8 };
    static const juce::Colour textMuted   { 0x73F0EDE8 };   // 45% opacity = 0x73
    static const juce::Colour textDim     { 0x33F0EDE8 };   // 20% opacity = 0x33

    // Accent
    static const juce::Colour gold        { 0xFFE9C46A };

    // Borders (7% white)
    static const juce::Colour border      { 0x12FFFFFF };   // 0x12 ≈ 7% of 0xFF

    // Depth Zones
    static const juce::Colour zoneSunlit  { 0xFF48CAE4 };
    static const juce::Colour zoneTwilight{ 0xFF0096C7 };
    static const juce::Colour zoneMidnight{ 0xFF7B2FBE };

    // Font sizes (in JUCE points — not CSS px)
    // CSS px → JUCE pt: multiply by 0.75 for typical 96dpi, or use
    // getHeight() in component's resized() for DPI-aware sizing
    static constexpr float fontSizeWindowTitle  = 15.0f;  // 20px CSS ≈ 15pt
    static constexpr float fontSizeSectionHdr   = 8.25f;  // 11px CSS ≈ 8.25pt
    static constexpr float fontSizeTabLabel     = 9.75f;  // 13px CSS ≈ 9.75pt
    static constexpr float fontSizeBody         = 9.0f;   // 12px CSS ≈ 9pt
    static constexpr float fontSizeLabel        = 8.25f;  // 11px CSS ≈ 8.25pt
    static constexpr float fontSizeNumeric      = 8.25f;  // 11px CSS ≈ 8.25pt
}
```

**DPI scaling**: Never hardcode pixel values. Always use `getWidth()`, `getHeight()`, or fractional proportions in `resized()`. For font size, compute from component height: `auto fontSize = (float)getHeight() * 0.18f` rather than hardcoding `15.0f`.

**CSS custom property → JUCE mapping**: CSS `rgba(240, 237, 232, 0.45)` → JUCE hex: alpha = `round(0.45 × 255)` = `0x73`, so `juce::Colour(0x73F0EDE8)`. Pattern: `(alpha_hex)(R_hex)(G_hex)(B_hex)`.

---

### F. VU Meter / Level Indicator — Real-Time Safe Pattern

```cpp
// NEVER call repaint() from the audio thread.
// Pattern: AsyncUpdater bridges audio → UI.

class MeterComponent : public juce::Component,
                       public juce::AsyncUpdater
{
public:
    // Called from audio thread (process block):
    void setLevel(float newLevel)
    {
        currentLevel.store(newLevel, std::memory_order_relaxed);
        triggerAsyncUpdate();  // schedules a message-thread repaint
    }

    // Called on message thread:
    void handleAsyncUpdate() override { repaint(); }

    void paint(juce::Graphics& g) override
    {
        float level = currentLevel.load(std::memory_order_relaxed);
        int frameIndex = juce::roundToInt(level * (numFrames - 1));

        // Draw VU meter frame
        auto filmstrip = juce::ImageCache::getFromMemory(
            BinaryData::VUMeter03_png, BinaryData::VUMeter03_pngSize);
        int frameH = filmstrip.getHeight() / numFrames;
        g.drawImage(filmstrip, 0, 0, getWidth(), getHeight(),
                    0, frameIndex * frameH, filmstrip.getWidth(), frameH);
    }

private:
    std::atomic<float> currentLevel { 0.0f };
    static constexpr int numFrames = 100;
};
```

---

### G. Button Filmstrips — State Machine Pattern

```cpp
class AudioUIButton : public juce::Button
{
public:
    AudioUIButton(const juce::String& name,
                  const juce::Image& filmstrip,
                  int frameOff, int frameOn,
                  int frameHover = -1,        // -1 = use frameOff
                  int frameDisabled = -1)      // -1 = use frameOff dimmed
        : juce::Button(name), strip(filmstrip),
          fOff(frameOff), fOn(frameOn),
          fHover(frameHover >= 0 ? frameHover : frameOff),
          fDisabled(frameDisabled >= 0 ? frameDisabled : frameOff)
    {}

    void paintButton(juce::Graphics& g,
                     bool isMouseOver, bool isButtonDown) override
    {
        int frame = fOff;
        if (!isEnabled())         frame = fDisabled;
        else if (getToggleState()) frame = fOn;
        else if (isMouseOver)      frame = fHover;

        int fh = strip.getHeight() / totalFrames;
        g.drawImage(strip,
                    0, 0, getWidth(), getHeight(),
                    0, frame * fh, strip.getWidth(), fh);

        if (fDisabled == fOff && !isEnabled())
            g.fillAll(juce::Colours::black.withAlpha(0.4f));  // dim disabled state
    }

private:
    juce::Image strip;
    int fOff, fOn, fHover, fDisabled;
    static constexpr int totalFrames = 2;  // Most audio-ui.com buttons have 2 frames
};
```

---

### H. Asset Loading Priority & Memory Budget

| Asset Type | Recommended Loading | Memory Impact | Notes |
|------------|--------------------|-|-------|
| Filmstrip knobs (100 frames, ~200×200px per frame) | `ImageCache::getFromMemory()` once, static cache | ~8MB per filmstrip (RGBA) | Do not load per-component; share via `ImageCache` |
| Button filmstrips (2 frames) | `ImageCache` or static member | ~100KB | Negligible — load freely |
| VU meters (100 frames) | `ImageCache` + `AsyncUpdater` | ~2–5MB | Critical: never `repaint()` from audio thread |
| Fonts (TTF) | `static Typeface::Ptr` | ~200–500KB per font | Create once in plugin constructor |
| Gradient backgrounds | Computed `ColourGradient` (no image) | Negligible | Recompute in `resized()` only, not `paint()` |
| SVG icons | `Drawable::createFromSVG()`, cached as `unique_ptr` | ~10–50KB per icon | Cache in LookAndFeel; don't recreate per paint |

**Lucy's rule of thumb**: If an asset doesn't change while the plugin is open, it should be `static` or otherwise cached. The only things that should move per `paint()` call are parameter-driven values (knob position, meter level, waveform data).

---

*Registry last updated: 2026-03-23*
*Next review: After iOS build phase begins — evaluate Mobile Audio GUI kit and font stack for touch optimization*
