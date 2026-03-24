# XO_OX Figma Asset Compendium
## Curated Community Design Resources

Date: 2026-03-22

### Purpose
Reference compendium of community Figma wireframe kits, component libraries, and design systems curated for use across all XO_OX products: XOmnibus (desktop plugin), Outshine/Originate (companion tools), XO-OX.org (website), audio-xpm-creator (web app), and iOS AUv3/Standalone app.

---

## Kit Index

### 1. Game UX Kit (Dismantle Studio)
**File**: `xo643maNsuCFslvd9DaZ7k`
**Relevance**: ★★★★★ (HIGHEST)
**Applies to**: All products

**What it contains:**
- **Typography System**: 5 levels (Display 100/80/60px, Header 40/36/32px, Title 30/26/22px, Body 26/22/18px, Label 16/14/12px) using Inter font. Each with size, line-height, and weight specs. Includes use-case descriptions.
- **Button System**: Primary/Secondary/Tertiary × 5 states (Default/Hover/Focused/Pressed/Disabled) × 4 sizes (Large/Medium/Small/Extra Small) + icon-only variants. Complete state matrix.
- **Color Palette**: Primary Black #161616, Gray #766D8D, White #F9F8FF, Accent Primary #7357C0, Accent Secondary #C0AAFD, 4 gray tones (#9695AB → #E8E6FE), Positive #3FCF92, Negative #E64848
- **Spacing System**: 4px base unit with 12 multipliers (0.5x through 20x = 2px through 80px). Visual scale reference.
- **Layout Grid**: 12-column grid system for 1920×1080 and 2560×1440 with 60px safe zones.
- **Icons**: Referenced but linked to external icon sets

**XO_OX Incorporation:**
- ADOPT the typography scale structure (5 levels × 3 sizes each = 15 type styles). Map to XO_OX fonts: Space Grotesk for Display/Header, Inter for Title/Body/Label, JetBrains Mono for numeric values.
- ADOPT the button state matrix as our standard. Replace purple accents with XO Gold #E9C46A.
- ADAPT the spacing system — our UIX spec already uses 4px base, this confirms 12 multiplier tokens.
- REFERENCE the grid system for Outshine/Originate window layouts and XO-OX.org responsive design.
- ADOPT the color methodology (functional naming: Primary/Accent/Positive/Negative/Gray scale) and map to XO_OX dark theme.

**Key Node IDs to reference:**
- Typography: `5:3`
- Buttons: `47:211`
- Colors: `11:9` (section)
- Spacing: `10:389` (section)
- Layout Grid: `364:954` (section)
- Components (various): `10:389`, `47:211`, `75:375`, `94:2156`, `203:358`, `215:802-1011`, `226:462`, `256:645`, `263:699`, `296:695`, `345:775`, `364:954`, `366:1604-1868`

---

### 2. Lean Mantine Library (Alley Corp Nord)
**File**: `3jtGAzg3jCdmcgAIhJblYc`
**Relevance**: ★★★★★ (HIGHEST)
**Applies to**: Outshine, Originate, audio-xpm-creator, XO-OX.org

**What it contains:**
- **Colors**: Full Open Color palette with Primary (10 shades), Neutral (Black/White + Gray 0-9), Dark (0-9), Destructive (10 shades). Both light and dark mode references.
- **Text Input**: Default/Filled/Unstyled variants × 7 states (Placeholder/Active Placeholder/Active Normal/Normal/Disabled/Error/Error+Disabled). Light + dark mode playground.
- **Components** (based on Mantine UI library — https://mantine.dev):
  - Buttons, Inputs, Dropdowns, Select, Multiselect
  - Sliders, Toggles, Checkboxes, Radio buttons
  - Tables, Cards, Modals, Drawers, Popovers
  - Tabs, Accordion, Breadcrumbs, Pagination
  - Badges, Tags, Chips, Tooltips
  - Progress bars, Loaders, Skeletons
  - Notifications, Alerts
  - Navigation (NavLink, AppShell, Sidebar)
  - Data display (Timeline, Stepper, Stat)

**XO_OX Incorporation:**
- ADOPT the input state matrix (7 states × 3 variants × 2 modes) as our standard for all form inputs in Outshine, Originate, and audio-xpm-creator.
- REFERENCE the dark mode color scale (Dark 0-9) when building the XO_OX dark theme. Map Dark.7 (#1A1B1E) ≈ our #1A1A1A, Dark.6 (#25262B) ≈ our #242424.
- ADOPT the slider component pattern for Outshine's expression controls and audio-xpm-creator's envelope editors.
- ADOPT the progress bar and stepper patterns for Outshine's 9-stage pipeline visualization.
- ADOPT the modal/drawer patterns for Originate's export dialog and Outshine's FX routing popover.

**Key Node IDs to reference:**
- Colors: `3050:12615`, `3050:9196`
- Typography: `1811:3685`, `1812:3858`
- Buttons: `2339:1906`, `2654:13656`, `2700:26777`
- Text Inputs: `3050:2818`
- Selects/Dropdowns: `3050:2752`, `3050:2536`
- Sliders: `3050:1781`, `3050:1574`
- Tables: `3049:3770`
- Modals: `3060:1072`, `3073:2150`
- Tabs: `3061:3021`
- Progress/Stepper: `3364:8777`
- Notifications: `3049:4207`, `3049:4152`
- Cards: `3050:3618-3622`
- Navigation: `3050:4308`, `3050:4925-4926`
- Tooltips: `3430:303`, `3429:138`
- Data Display: `3076:9868`, `3215:993`, `3272:996`
- Misc: `3050:6344-7277`, `1702:2673`, `1716:7218-7219`, `1809:3775`, `1810:4862`, `1811:3740`
- Charts/Visualizations: `3366:7844`, `3439:826`, `3447:21933`

---

### 3. FLOW V.4.0 (Flexible Library for Optimised Wireframes)
**File**: `yzqQkVf2alXAsF4jweZ3Rl`
**Relevance**: ★★★★☆ (HIGH)
**Applies to**: XO-OX.org, audio-xpm-creator, Outshine/Originate

**What it contains:**
The most comprehensive kit in the collection — 100+ nodes covering:
- **Toast/Notifications**: Info/Success/Warning/Error/Neutral × Desktop/Mobile. Clean status indicators with colored left borders.
- **Page Templates**: Hero sections, feature grids, pricing tables, testimonials, FAQ, footer variations
- **Navigation**: Top nav bars, sidebars, breadcrumbs, tab bars, mega menus
- **Forms**: Full form library (text, select, checkbox, radio, toggle, file upload, date picker, search)
- **Data Display**: Tables, cards, lists, stat blocks, charts, dashboards
- **Feedback**: Modals, dialogs, drawers, alerts, toasts, progress bars, empty states
- **Media**: Image galleries, video players, audio players, file previews, carousels
- **E-commerce**: Product cards, cart, checkout, pricing
- **Content**: Blog layouts, article pages, documentation, changelogs
- **Settings**: Account pages, profile settings, notification preferences

**XO_OX Incorporation:**
- ADOPT the toast/notification system. The 5-status pattern (Info=blue, Success=green, Warning=amber, Error=red, Neutral=gray) maps perfectly to XO_OX operational states (render progress, export success, classification warnings, format errors, general info).
- REFERENCE page templates for XO-OX.org page layouts (hero, features grid, pricing for Patreon tiers, FAQ, documentation).
- ADOPT the audio player wireframe pattern as starting point for the Playable Aquarium audio controls.
- REFERENCE the dashboard patterns for the Fleet Inspector and Pilgrimage Tracker web views.
- ADOPT empty state patterns for Outshine's first-launch state.
- REFERENCE the changelog pattern for the XO_OX release notes page.

**Key Node IDs (organized by category):**
- Toasts: `7008:42828`, `7008:42947`
- Navigation: `7381:12596`, `7410:20727`, `7008:43170`
- Forms: `7008:43764`, `7410:22262`, `7008:47710`
- Modals/Dialogs: `7822:2452`
- Page Templates: `6337:51537-51914`, `7505:14852`, `7821:2117-2118`
- Cards: `7008:69104`, `7519:9081`
- Data Tables: `5606:72311`
- Audio/Media: `7579:51899`, `7581:75002-75007`
- Content/Blog: `7581:108794-133003` (large range)
- Dashboard: `7646:16098-16099`, `7665:49601-49602`
- Settings: `7693:18455-18456`, `7706:5323-7436`
- Progress: `7708:7452-7453`
- Charts: `7754:47628`
- Hero Sections: `7710:51805`
- Footers: `5885:82513-82674`
- Pricing: `7580:63046`
- FAQ: `5865:78686`
- Empty States: `5560:49614`, `5562:51939`
- Sidebar: `7318:72951-80401` (large range)
- Changelog: `7823:17626`
- Documentation: `6031:78264`
- Profile/Account: `6337:606`, `6340:50118-51232`
- Stats/Metrics: `6338:54408`, `6341:51232`
- Lists: `6373:7298`
- Date Picker: `6446:2876`
- Search: `6718:1824`, `6725:63031`, `6727:1780`, `6729:39341`
- File Upload: `6744:12322-14250`
- Onboarding: `6767:96230`, `6774:96398-100719`
- Error Pages: `6878:4090-62534`
- Landing Pages: `6913:21887-53935`
- Checkout: `6948:6216`
- Player: `7319:82200-82874`
- Tabs: `7327:44820-44903`
- Calendar: `10066:926-927`
- Stepper: `3137:23265-26716`
- Alerts: `7334:105342-107469`, `7335:107173`, `7336:107469`, `7337:107535`
- Misc: `2319:5951`, `6346:520`, `9817:38898-38899`, `10032:2105526-2105940`

---

### 4. iOS App Wireframes
**File**: `Ch2tom2TeEb73GKDmr7z9j`
**Relevance**: ★★★★☆ (HIGH)
**Applies to**: iOS AUv3/Standalone app

**What it contains:**
- 30+ mobile screens covering common iOS patterns at 375×812 (iPhone standard):
  - Login/Signup flows
  - Profile pages with avatar + grid layouts
  - Navigation patterns (back nav, tab bars, action sheets)
  - Form inputs and settings screens
  - Content feeds (cards, lists)
  - Media views (galleries, detail views)
  - Onboarding flows
  - Empty states and error screens
  - Search and filter patterns

**XO_OX Incorporation:**
- REFERENCE the 3×3 grid card layout for the iOS engine selector (each card = one engine).
- ADOPT the 4-tab bottom navigation pattern for the iOS app (Engines | Presets | Coupling | Settings).
- REFERENCE the profile page layout for the "Engine Detail" view (avatar = engine icon, grid = preset cards).
- ADOPT the settings screen patterns for iOS XOmnibus preferences.
- REFERENCE onboarding flow for iOS first-launch tutorial.

**Key Node IDs:**
- Login/Form: `1:2`, `1:43`, `1:73`, `1:96`
- Profile/Grid: `3:26`, `3:128`
- Settings: `3:221`, `3:302`
- Content/Feed: `3:423`, `3:447`, `3:490`, `3:523`
- Navigation: `3:776`, `3:849`
- Search: `9:57`, `8:0`
- Media: `7:82-255`
- Onboarding: `3:936-977`
- Detail Views: `12:74`
- Calendar/Scheduler: `13:427-537`
- Dashboard: `17:0`
- Maps/Spatial: `31:723`
- Charts: `230:1029`

---

### 5. Wireframing Starter Kit
**File**: `9DzPp9y4kWcqDy7UeOTYXw`
**Relevance**: ★★★☆☆ (MEDIUM)
**Applies to**: All products (foundation reference)

**What it contains:**
- **Icons**: 160+ HeroIcons (outlined, 24×24) + Tabler Icons. Includes music-note, film, play, pause, volume controls, microphone — directly relevant to audio products.
- **Text Styles**: H1/H2/H3 headings + Body/Body Heavy/Body Small/Link. Clean hierarchy reference.
- **Form Components**: Text inputs, dropdowns, checkboxes, radio buttons, toggles
- **Buttons**: Basic button styles with text variants
- **Cards**: Content cards, product cards, stat cards
- **Navigation**: Headers, sidebars, breadcrumbs
- **Tables**: Data tables with sorting
- **Media players**: Audio/video player controls
- **Modals**: Dialog patterns
- **Various UI patterns**: Tooltips, badges, avatars, pagination

**XO_OX Incorporation:**
- REFERENCE the audio-related HeroIcons (music-note, volume-up, volume-off, play, pause, fast-forward, rewind) for Outshine/Originate's audio preview controls.
- REFERENCE the media player controls for the Playable Aquarium audio interface.
- ADOPT the player control icons from Tabler Icons (player-play, player-prev, player-next, player-shuffle, player-repeat, player-heart, player-volume) for Outshine's preview keyboard controls.

**Key Node IDs:**
- Icons: `1418:488`
- Text Styles: `1418:964`
- Buttons: `1418:1023`
- Form Components: `1418:1127`, `1418:1159`
- Cards: `1418:1234`, `1451:910`
- Navigation: `1418:1320`
- Tables: `1418:1817`
- Media Player: `1462:992`
- Various: `1418:1576-1791`, `1420:635`, `1422:781-949`, `1428:1135`, `1450:1246-1247`

---

### 6. Helio Wireframe Kit
**File**: `Su4kqol0vZyExOdzRJCu5h`
**Relevance**: ★★★☆☆ (MEDIUM)
**Applies to**: All products (icon reference)

**What it contains:**
- **Feather Icons**: 280+ icons at 24×24px (by Cole Bemis). Full set including arrows, media controls, UI elements, social icons, developer icons.
- **Page Components**: Headers, footers, hero sections, feature grids, testimonials, pricing tables
- **Form Elements**: Full form component library
- **Data Display**: Tables, charts, stat blocks, timelines
- **Navigation**: Various nav patterns, sidebars, tab bars

**XO_OX Incorporation:**
- REFERENCE Feather icons as fallback when SF Symbols don't have the right metaphor. The set includes CPU, database, terminal, sliders, layers, grid, activity — all relevant to synth/DSP tooling.
- REFERENCE hero section patterns for XO-OX.org page layouts.

**Key Node IDs:**
- All Icons: `54:386`
- Hero: `96:11298`
- Navigation: `54:1482`
- Features: `54:2665`, `54:2795`
- Testimonials: `54:2864`
- Pricing: `54:3007`
- CTA: `54:2931`
- Footer: `54:3086`, `54:3143`
- Content: `54:3214`
- Forms: `55:498`, `55:590`, `55:663`
- Data: `60:572`, `60:574`
- Charts: `72:904`
- Pages: `75:2898`, `75:3911`, `95:1405`
- Dashboard: `529:1571`, `529:1638`
- Overview: `54:354`

---

### 7. UX Research Kit
**File**: `ApLBqJ9GdX4R4tLy0iRx8P`
**Relevance**: ★★★☆☆ (MEDIUM)
**Applies to**: Product strategy, user research

**What it contains:**
- User persona templates
- Journey mapping frameworks
- Research planning templates
- Affinity diagram layouts
- Usability test note templates
- Competitive analysis frameworks

**XO_OX Incorporation:**
- REFERENCE persona templates for formalizing the XO_OX target user profiles (beatmaker, film composer, sound designer, jazz musician — the Producer's Guild archetypes).
- REFERENCE journey mapping for the Outshine "drop samples → playable instrument" user flow and the XPN export pipeline user experience.
- REFERENCE competitive analysis template for the MPCe competitive window documentation.

**Key Node IDs:**
- Personas: `501:1091`
- Research: `602:36`
- Journey Map: `630:2`
- Templates: `489:901-912`, `491:899`

---

### 8. BRIX Website Wireframes UI Kit
**File**: `IzeCoEbEt5QU14lF0rr0iF`
**Relevance**: ★★☆☆☆ (LOWER)
**Applies to**: XO-OX.org

**What it contains:**
- Website wireframe patterns: hero sections, feature grids, pricing tables
- Standard web component wireframes

**XO_OX Incorporation:**
- REFERENCE for XO-OX.org responsive layout patterns.
- Less specific than FLOW V.4.0 for our needs.

**Key Node IDs:**
- Hero: `6:820`
- Features: `5:4129`
- Sections: `6:2419`

---

### 9. Ant UX Wireframes
**File**: `641CwZJyZKKYN9FOdsyTRo`
**Relevance**: ★★☆☆☆ (LOWER)
**Applies to**: Layout reference

**What it contains:**
- Page layout templates: 2-column, 3-column, 2-row, 3-row, left column, right column, top navigation, waterfall/masonry

**XO_OX Incorporation:**
- REFERENCE the layout templates when designing new pages for XO-OX.org.
- The left-column layout maps to Outshine's source panel + design canvas layout.

**Key Node IDs:**
- All layouts: `0:546`
- Wireframe components: `2:4`, `2:8`

---

## Cross-Product Component Mapping

This table shows which kit components to reference for each XO_OX product surface:

| Component Need | Primary Kit | Secondary Kit | Notes |
|----------------|-------------|---------------|-------|
| **Buttons** | Game UX Kit (47:211) | Mantine (2339:1906) | Map purple → XO Gold |
| **Text Inputs** | Mantine (3050:2818) | FLOW | 7-state matrix, dark mode |
| **Sliders** | Mantine (3050:1781) | — | Critical for Outshine expression |
| **Dropdowns** | Mantine (3050:2752) | FLOW | Used in FX routing, velocity strategy |
| **Toggles** | Mantine (2654:13656) | — | Formant lock, round-robin on/off |
| **Tables** | Mantine (3049:3770) | FLOW (5606:72311) | Preset lists, parameter tables |
| **Tabs** | Mantine (3061:3021) | — | Outshine Zone/Velocity/FX/Expression |
| **Toast/Notifications** | FLOW (7008:42828) | — | 5-status system for all products |
| **Modals/Dialogs** | Mantine (3060:1072) | FLOW (7822:2452) | Export confirmation, error dialogs |
| **Progress Bars** | Mantine (3364:8777) | FLOW (7708:7452) | Render pipeline progress |
| **Cards** | Mantine (3050:3618-3622) | Game UX | Engine cards, preset cards |
| **Navigation** | iOS Wireframes (3:776) | FLOW | Bottom tabs for iOS |
| **Icons** | Wireframing Starter (1418:488) | Helio (54:386) | HeroIcons primary, Feather fallback |
| **Audio Controls** | Wireframing Starter (1462:992) | — | Play/pause/seek for preview |
| **Typography Scale** | Game UX Kit (5:3) | Wireframing Starter (1418:964) | 5-level hierarchy |
| **Color System** | Game UX Kit (11:9) | Mantine (3050:12615) | Functional naming + shade scales |
| **Spacing System** | Game UX Kit (10:389) | — | 4px base, 12 multipliers |
| **Grid Layout** | Game UX Kit (364:954) | Ant UX (0:546) | 12-column responsive |
| **Empty States** | FLOW (5560:49614) | iOS (3:936) | Outshine first launch, no results |
| **Error States** | FLOW (6878:4090) | iOS Wireframes | Format errors, render failures |
| **Settings/Prefs** | iOS Wireframes (3:221) | FLOW | iOS app preferences |
| **Onboarding** | iOS Wireframes (3:936) | FLOW (6767:96230) | First-launch tutorials |
| **Hero Sections** | FLOW (7710:51805) | Helio (96:11298) | XO-OX.org pages |
| **Pricing** | FLOW (7580:63046) | Helio (54:3007) | Patreon tiers page |
| **Changelog** | FLOW (7823:17626) | — | Release notes page |
| **Dashboard** | FLOW (7646:16098) | Helio (529:1571) | Fleet Inspector web view |
| **User Personas** | UX Research (501:1091) | — | Producer archetypes |
| **Journey Maps** | UX Research (630:2) | — | User flow documentation |

---

## Design Token Alignment

### How community kit tokens map to XO_OX tokens:

| Kit Token | XO_OX Token | Value |
|-----------|-------------|-------|
| Game UX Primary Black | `--xo-bg-base` | #161616 → #1A1A1A |
| Game UX Primary Gray | `--xo-text-secondary` | #766D8D (reference) |
| Game UX Primary White | `--xo-text-primary` | #F9F8FF → #F5F5F5 |
| Game UX Accent Primary | `--xo-accent` | #7357C0 → #E9C46A (XO Gold) |
| Game UX Accent Secondary | `--xo-accent-hover` | #C0AAFD → #F0D080 |
| Game UX Positive | `--xo-status-success` | #3FCF92 → #4CAF50 |
| Game UX Negative | `--xo-status-error` | #E64848 → #F44336 |
| Mantine Dark.7 | `--xo-bg-base` | #1A1B1E ≈ #1A1A1A |
| Mantine Dark.6 | `--xo-bg-panel` | #25262B ≈ #242424 |
| Mantine Dark.5 | `--xo-bg-card` | #2C2E33 ≈ #2E2E2E |

### Typography alignment:

| Kit Style | XO_OX Style | Font | Size |
|-----------|-------------|------|------|
| Game UX Display Large | Window Title | Space Grotesk SemiBold | 20pt (scaled down from 100pt) |
| Game UX Header Large | Section Header | Space Grotesk SemiBold | 14pt (scaled from 40pt) |
| Game UX Title Medium | Panel Title | Space Grotesk Regular | 13pt |
| Game UX Body Medium | Body Text | Inter Regular | 12pt |
| Game UX Body Small | Caption | Inter Regular | 11pt |
| Game UX Label Medium | Form Label | Inter Medium | 11pt |
| Game UX Label Small | Numeric Value | JetBrains Mono Regular | 11pt |

---

## Figma File Quick-Access URLs

For convenience, here are the base URLs for each kit:

1. **Game UX Kit**: `https://www.figma.com/design/xo643maNsuCFslvd9DaZ7k/Game-UX-Kit--Community-`
2. **Lean Mantine Library**: `https://www.figma.com/design/3jtGAzg3jCdmcgAIhJblYc/Lean-Mantine-Library`
3. **FLOW V.4.0**: `https://www.figma.com/design/yzqQkVf2alXAsF4jweZ3Rl/FLOW-V.4.0`
4. **iOS App Wireframes**: `https://www.figma.com/design/Ch2tom2TeEb73GKDmr7z9j/iOS-App-Wireframes--Community-`
5. **Wireframing Starter Kit**: `https://www.figma.com/design/9DzPp9y4kWcqDy7UeOTYXw/Wireframing-Starter-Kit--Community-`
6. **Helio Wireframe Kit**: `https://www.figma.com/design/Su4kqol0vZyExOdzRJCu5h/Helio---Wireframe-Kit--Community-`
7. **UX Research Kit**: `https://www.figma.com/design/ApLBqJ9GdX4R4tLy0iRx8P/UX-research-kit--Community-`
8. **BRIX Website Wireframes**: `https://www.figma.com/design/IzeCoEbEt5QU14lF0rr0iF/Website-Wireframes-UI-Kit`
9. **Ant UX Wireframes**: `https://www.figma.com/design/641CwZJyZKKYN9FOdsyTRo/Ant-UX-Wireframes--Community-`

---

## Recommended Workflow

### When designing a new UI surface:
1. Check this compendium for relevant component references
2. Open the kit's Figma file and navigate to the node ID
3. Screenshot or duplicate the component into the XO_OX design file
4. Remap colors to XO_OX tokens (primarily: accent → XO Gold #E9C46A, bg → #1A1A1A dark theme)
5. Remap typography to XO_OX fonts (Space Grotesk / Inter / JetBrains Mono)
6. Document any new patterns back into this compendium

### Priority order for component adoption:
1. **Lean Mantine Library** — production-ready components with dark mode and proper state coverage
2. **Game UX Kit** — design system foundations (type, color, spacing, grid)
3. **FLOW V.4.0** — comprehensive wireframe patterns for pages and feedback
4. **iOS App Wireframes** — mobile-specific patterns
5. **Others** — reference as needed

---

## XO_OX Internal Design Assets

### A. Design System Source of Truth

#### design-tokens.css
**Path**: `Site/design-tokens.css`
**Status**: CANONICAL — all other specs derive from this file

Contains:
- Global Brand tokens (backgrounds, borders, text, XO Gold `#E9C46A`)
- Ecological Depth Zones (Sunlit `#48CAE4`, Twilight `#0096C7`, Midnight `#7B2FBE`)
- 46 Engine Accent Colors with mythological names
- Glass/Porthole visual formulas
- PlaySurface tokens
- Typography scale
- Animation timing

#### engine-creature-map.json
**Path**: `Site/engine-creature-map.json`
**Status**: CANONICAL — engine-to-mythology mapping for aquarium and UI

---

### B. Engine Signature Color Palette (71 Engines)

#### Original Fleet (48 Engines)

| Engine | Color Name | Hex | Engine | Color Name | Hex |
|--------|-----------|-----|--------|-----------|-----|
| ODDFELIX | Neon Tetra Blue | `#00A6D6` | ODDOSCAR | Axolotl Gill Pink | `#E8839B` |
| OVERDUB | Olive | `#6B7B3A` | ODYSSEY | Violet | `#7B2D8B` |
| OBLONG | Amber | `#E9A84A` | OBESE | Hot Pink | `#FF1493` |
| ONSET | Electric Blue | `#0066FF` | OVERWORLD | Neon Green | `#39FF14` |
| OPAL | Lavender | `#A78BFA` | ORGANON | Bioluminescent Cyan | `#00CED1` |
| OUROBOROS | Strange Attractor Red | `#FF2D2D` | OBSIDIAN | Crystal White | `#E8E0D8` |
| ORIGAMI | Vermillion Fold | `#E63946` | ORACLE | Prophecy Indigo | `#4B0082` |
| OBSCURA | Daguerreotype Silver | `#8A9BA8` | OCEANIC | Phosphorescent Teal | `#00B4A0` |
| OCELOT | Ocelot Tawny | `#C5832B` | OVERBITE | Fang White | `#F0EDE8` |
| ORBITAL | Warm Red | `#FF6B6B` | OPTIC | Phosphor Green | `#00FF41` |
| OBLIQUE | Prism Violet | `#BF40FF` | OSPREY | Azulejo Blue | `#1B4F8A` |
| OSTERIA | Porto Wine | `#722F37` | OWLFISH | Abyssal Gold | `#B8860B` |
| OHM | Sage | `#87AE73` | ORPHICA | Siren Seafoam | `#7FDBCA` |
| OBBLIGATO | Rascal Coral | `#FF8A7A` | OTTONI | Patina | `#5B8A72` |
| OLE | Hibiscus | `#C9377A` | OVERLAP | Bioluminescent Cyan-Green | `#00FFB4` |
| OUTWIT | Chromatophore Amber | `#CC6600` | OMBRE | Shadow Mauve | `#7B6B8A` |
| ORCA | Deep Ocean | `#1B2838` | OCTOPUS | Chromatophore Magenta | `#E040FB` |
| OSTINATO | Firelight Orange | `#E8701A` | OPENSKY | Sunburst | `#FF8C00` |
| OCEANDEEP | Trench Violet | `#2D0A4E` | OUIE | Hammerhead Steel | `#708090` |
| OBRIX | Reef Jade | `#1E8B7E` | ORBWEAVE | Kelp Knot Purple | `#8E4585` |
| OVERTONE | Spectral Ice | `#A8D8EA` | ORGANISM | Emergence Lime | `#C6E377` |
| OXBOW | Oxbow Teal | `#1A6B5A` | OWARE | Akan Goldweight | `#B5883E` |
| OPERA | Aria Gold | `#D4AF37` | OFFERING | Crate Wax Yellow | `#E5B80B` |
| OSMOSIS | Surface Tension Silver | `#C0C0C0` | OXYTOCIN | Synapse Violet | `#9B5DE5` |

#### Kitchen Collection (24 Engines — V2 Paid Expansion)

| Quad | Engine | Color Name | Hex |
|------|--------|-----------|-----|
| Chef (Organs) | XOto | Bamboo Green | `#7BA05B` |
| Chef | XOctave | Bordeaux | `#6B2D3E` |
| Chef | XOleg | Orthodox Gold | `#C5A036` |
| Chef | XOtis | Soul Gold | `#DAA520` |
| Kitchen (Pianos) | XOven | Cast Iron Black | `#2C2C2C` |
| Kitchen | XOchre | Copper Patina | `#B87333` |
| Kitchen | XObelisk | Obsidian Slate | `#4A4A4A` |
| Kitchen | XOpaline | Crystal Blue | `#B8D4E3` |
| Cellar (Bass) | XOgre | Deep Earth Brown | `#4A2C0A` |
| Cellar | XOlate | Burgundy | `#6B1A2A` |
| Cellar | XOaken | Dark Walnut | `#3D2412` |
| Cellar | XOmega | Copper Still | `#B04010` |
| Garden (Strings) | XOrchard | Harvest Gold | `#DAA520` |
| Garden | XOvergrow | Vine Green | `#3A5F0B` |
| Garden | XOsier | Willow Green | `#6B8E23` |
| Garden | XOxalis | Clover Purple | `#6A0DAD` |
| Broth (Pads) | XOverwash | Tea Amber | `#D4A76A` |
| Broth | XOverworn | Reduced Wine | `#4A1A2E` |
| Broth | XOverflow | Steam White | `#E8E8E8` |
| Broth | XOvercast | Ice Blue | `#B0E0E6` |
| Fusion (EP) | XOasis | Cardamom Gold | `#C49B3F` |
| Fusion | XOddfellow | Neon Night | `#FF6B35` |
| Fusion | XOnkolo | Kente Gold | `#FFB300` |
| Fusion | XOpcode | Digital Cyan | `#00CED1` |

#### Brand Constants
| Token | Value | Usage |
|-------|-------|-------|
| XO Gold | `#E9C46A` | Macros, coupling strips, active states, header bars |
| Gallery White | `#F8F6F3` | Light shell background (Gallery Model) |
| Gallery Dark | `#1A1A1A` | Dark inner panels, plugin background |
| Panel Mid | `#242424` | Panel backgrounds |
| Card Surface | `#2E2E2E` | Card/elevated surfaces |

---

### C. Screenshot Library (49 Mockups)

All in `~/` (home directory root). These document the visual evolution of XO-OX.org and the plugin UI.

#### Aquarium & Aquatic (9 files)
| File | Content |
|------|---------|
| `aquarium-hero.png` | Full-page aquarium hero section |
| `aquarium-water-column.png` | Water depth visualization (engine placement by depth zone) |
| `aquatic-aquarium.png` | Interactive aquarium interface |
| `aquatic-chord-machine.png` | Chord machine UI component |
| `aquatic-footer.png` | Site footer design |
| `aquatic-guide.png` | Field Guide section layout |
| `aquatic-hero.png` | Aquatic-themed hero |
| `aquatic-instruments.png` | Instrument selector grid |
| `aquatic-packs.png` | Pack browser interface |
| `aquatic-philosophy.png` | Philosophy/branding section |

#### Manifesto Series (6 files)
| File | Content |
|------|---------|
| `manifesto-hero.png` | Opening spread |
| `manifesto-ai-partnership.png` | AI partnership positioning |
| `manifesto-coupling.png` | Coupling concept visualization |
| `manifesto-enabling.png` | Enabling features showcase |
| `manifesto-rejection.png` | What XO_OX rejects (anti-patterns) |
| `manifesto-closing.png` | Conclusion/CTA |

#### PlaySurface Iteration (6 files)
| File | Content |
|------|---------|
| `playsurface-9.8.png` | Version 9.8 layout |
| `playsurface-final-polish.png` | Final visual refinement |
| `playsurface-full-labels.png` | Fully labeled zones and controls |
| `playsurface-with-glows.png` | Glow/lighting effects version |
| `playsurface-zone-separators.png` | Zone boundary design |
| `playsurface-zones.png` | Zone structure overview |

#### XO Branding & Hero (5 files)
| File | Content |
|------|---------|
| `xo-hero.png` | Main hero section |
| `xo_ox_hero.png` | Full XO_OX branded hero |
| `xo_ox_full.png` | Complete page layout |
| `xo_ox_full_revealed.png` | All layers revealed |
| `xo-engine-modal.png` | Engine selector modal dialog |

#### Instruments & Content (8 files)
| File | Content |
|------|---------|
| `xo-instruments.png` | Instrument grid layout |
| `xo-chordmachine.png` | Chord machine demo |
| `xo-packs-hero.png` | Pack browser hero |
| `xo-packs-grid.png` | Pack grid layout |
| `xo-packs-support.png` | Support/info section |
| `instrument-demo-hero.png` | Demo hero section |
| `instrument-demo-9.8-final.png` | Final demo layout |
| `instrument-demo-playsurface.png` | PlaySurface in demo context |
| `instrument-demo-pads-zoom.png` | Pad detail zoom |

#### Site Pages & Guide (10 files)
| File | Content |
|------|---------|
| `guide-hero.png` | Field Guide landing hero |
| `guide-coming.png` | "Coming soon" placeholder |
| `guide-posts.png` | Blog/guide post grid |
| `site-index-hero.png` | Home page hero |
| `site-index-download-mpc.png` | MPC download CTA |
| `site-aquarium.png` | Aquarium full page |
| `site-aqua-pairs.png` | Engine card pairs |
| `site-aquarium-cards.png` | Card components |
| `site-hero.png` | Generic hero template |
| `site-philosophy.png` | Philosophy values section |
| `site-ecological-alignment.png` | Ecological positioning |

#### Signal Feed & Other (5 files)
| File | Content |
|------|---------|
| `signal-feed.png` | Signal/update feed layout |
| `signal-posts.png` | Post cards in feed |
| `full-page-9.8.png` | Complete page at v9.8 |
| `hero-section-final.png` | Finalized hero iteration |

---

### D. Design Documentation Index

#### Core Design System Docs
| File | Path | Content |
|------|------|---------|
| Master UI Spec v2 | `Docs/design/xomnibus_ui_master_spec_v2.md` | Complete UI component specifications |
| Design Guidelines | `Docs/design/xomnibus_design_guidelines.md` | Design methodology and constraints |
| Technical Design System | `Docs/xomnibus_technical_design_system.md` | Implementation-level design tokens |
| Brand Identity | `Docs/xomnibus_brand_identity_and_launch.md` | Brand positioning and launch strategy |
| Coupling UI Architecture | `Docs/coupling-ui-architecture-2026-03-21.md` | Coupling interface specification |

#### Outshine & Originate
| File | Path | Content |
|------|------|---------|
| Outshine Format Spec | `Docs/xoutshine-forge-spec.md` | 1,530-line format specification |
| Outshine Seance | `Docs/seance-outshine-2026-03-22.md` | Ghost council verdict (8.4/10) |
| UIX Design Spec | `~/Documents/GitHub/audio-xpm-creator/DESIGN_SPECIFICATION_OUTSHINE_ORIGINATE.md` | 1,200+ line wireframes, fonts, colors, components, animations |

#### Tools & Pipeline
| File | Path | Content |
|------|------|---------|
| Cover Art Generator | `Tools/xpn_cover_art.py` | Procedural XPN cover art |
| Cover Art v2 | `Tools/xpn_cover_art_generator_v2.py` | Enhanced cover art pipeline |
| Cover Art Batch | `Tools/xpn_cover_art_batch.py` | Batch cover art generation |
| Cover Art Audit | `Tools/xpn_cover_art_audit.py` | Quality validation for covers |

#### HTML Prototype
| File | Path | Content |
|------|------|---------|
| Main UI Mockup | `Docs/mockups/xomnibus-main-ui.html` | Interactive HTML prototype of plugin UI |

---

### E. Site Assets Inventory

**Path**: `Site/` (within XOmnibus repo)
**Total**: 8,843+ image assets

Key directories:
- `Site/img/led/` — LED character sprites (A-Z, 0-9, symbols) for display panels
- `Site/img/engines/` — Per-engine illustrations and icons
- `Site/css/` — Stylesheets including design-tokens.css
- `Site/js/` — Interactive components (aquarium, coupling visualizer)

---

### F. Typography Specification (Unified)

| Context | Font | Weight | Sizes | Source |
|---------|------|--------|-------|--------|
| Window titles, section headers | Space Grotesk | SemiBold (600) | 20pt / 14pt / 13pt / 11pt | UIX Spec |
| Engine names, labels | Space Grotesk | Regular (400) | 13pt | UIX Spec |
| Body text | Inter | Regular (400) | 12pt | UIX Spec |
| Form labels | Inter | Medium (500) | 11pt | UIX Spec |
| Numeric values, data | JetBrains Mono | Regular (400) | 11pt | UIX Spec |
| XO-OX.org headings | Space Grotesk | Bold (700) | 48/36/24px | Site CSS |
| XO-OX.org body | Inter | Regular (400) | 16/14px | Site CSS |

All fonts are free/OFL from Google Fonts.

---

### G. Icon Sources (Priority Order)

1. **SF Symbols** (macOS/iOS native) — Primary for desktop plugin and iOS app
2. **HeroIcons** (Wireframing Starter Kit, node `1418:488`) — 160+ outlined icons, includes audio-specific (music-note, volume, play/pause)
3. **Tabler Icons** (Wireframing Starter Kit) — Media player controls (player-play, player-prev, player-next, player-shuffle, player-repeat, player-heart, player-volume)
4. **Feather Icons** (Helio Kit, node `54:386`) — 280+ icons, fallback set. Includes CPU, database, terminal, sliders, layers, activity
5. **Custom SVG** — XO_OX-specific icons (engine archetypes, coupling types, zone indicators)

---

## Version History
- 2026-03-22: Initial compendium — 9 Figma community kits (250+ nodes)
- 2026-03-22: Expanded — added 49 screenshots, 71 engine colors, 80+ design docs, 8,843+ site assets, typography spec, icon sources, design token mapping. Full cross-session asset consolidation.
