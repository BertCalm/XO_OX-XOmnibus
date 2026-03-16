# XPN Pack Label & Visual Identity System — R&D Spec
**XO_OX Designs | March 2026**

> *"The cover art is the first sound. Before the producer hears a note, the image has already told them what kind of music this pack wants to make."*

---

## Overview

This document specifies the visual identity system for XO_OX XPN expansion packs — every surface where the pack presents itself: MPC browser thumbnail, cover image, collection packaging, print/merch, community templates, and potential animated formats. The goal is a system where every pack is instantly recognizable as XO_OX while remaining immediately distinguishable from every other XO_OX pack.

---

## 1. Cover Art System — Current State and What Is Missing

### 1.1 What `xpn_cover_art.py` Produces

`Tools/xpn_cover_art.py` is a self-contained Pillow + NumPy pipeline that renders procedural raster cover art at 500×500, 1000×1000, and 2000×2000px. Its pipeline:

1. Dark vignette base canvas derived from each engine's `bg_base` RGB tuple.
2. One of eight NumPy-level style functions over the base: `transient_spikes`, `pixel_grid`, `angular_cuts`, `wave_morph`, `tape_streaks`, `lissajous`, `freq_bands`, `dense_blocks`, `particle_scatter`.
3. Optional fingerprint modulation pass (`xpn_optic_fingerprint` integration) that adjusts centroid brightness, warm tint, contrast from transient density, and feliX/Oscar polarity tint, and draws an 8-band spectral silhouette.
4. PIL text overlay: left-edge accent color bar, pack title with drop shadow, engine label pill (rounded rectangle in accent color), preset count in XO Gold, version string, faint `XO_OX` watermark bottom-right.

### 1.2 What It Is Missing

**Brand fonts.** The tool cascades through hard-coded macOS system font paths (Arial Bold, Helvetica) before falling back to PIL's bitmap default. Space Grotesk, Inter, and JetBrains Mono are never used. The brand typography is invisible in every generated pack.

**Wordmark vs. text.** "XO_OX" rendered in a system fallback font at 22% opacity is not a wordmark. It is a label. The XO_OX brand mark should be a composed glyph or SVG path, not system-font text.

**Mood indicator.** The 7 moods (Foundation, Atmosphere, Entangled, Prism, Flux, Aether, Family) have no visual representation on the cover. A pack's mood category is as important to the producer browsing the MPC library as the engine name.

**Collection identity.** Kitchen Essentials, Travel/Vessels, and Artwork/Color packs all generate identical-structure covers. There is no visual signal that OSTERIA and OSPREY are from the same ShoreSystem family, or that four Artwork packs are a color quad.

**Competitive comparison.** NI Maschine Expansions use bold photographic hero images cropped to a square, with a simple two-line text treatment (pack name in a clean sans, 3D or instrument icon). Output packs (Arcade, Signal) use abstract generative art with confident full-bleed color fields and a single large wordmark. MSXII Sound Design covers are photograph-dominant with atmospheric color grading. All three use crisp, purposeful typography at scale. XO_OX's procedural patterns are distinctive but the typography layer undermines the quality signal.

**What great MPC expansion cover art does:**
- Communicates the sonic character in 200ms at 200px (the MPC browser size).
- Uses one dominant color, maximum two secondary colors — not gradients of many.
- Has one large text element (pack name or brand) and one small text element (category or tagline).
- Feels like a physical artifact: a record sleeve, a gear manufacturer's manual, a scientific specimen card.

---

## 2. XO_OX Visual Identity Applied to Pack Covers

### 2.1 Color System

The canonical XO_OX cover palette has three tiers:

| Role | Token | Value |
|------|-------|-------|
| Shell / neutral | WARM_WHITE | `#F8F6F3` |
| Brand constant | XO_GOLD | `#E9C46A` |
| Engine identity | accent_color | per-engine (34 values in ENGINE_DEFS) |

**Rule:** Every cover uses exactly one engine accent color as the dominant hue, XO Gold as the brand constant (preset count, active highlights), and WARM_WHITE as the typography fill. The `bg_base` near-black is derived by desaturating and darkening the accent hue — it is never pure black.

**Multi-engine packs:** Two engines → angular gradient between the two accent colors, each occupying 50% of the background diagonal. Four engines → 2×2 quadrant grid with XO Gold ring at center intersection. More than four → accent palette as concentric rings; outer ring = XO Gold.

### 2.2 Persistent Visual Elements

Every XO_OX pack cover, regardless of engine or collection, must contain all four of these elements in consistent positions:

**XO_OX wordmark.** Bottom-right corner, minimum 6% canvas width. Not system-font text — a path-rendered mark. At 200px display size it reads as a small brand seal, not a legible string. At 2000px it is fully legible. Opacity: 85% (not 22% as current).

**Engine name badge.** Rounded rectangle, fill = accent color, dark text, upper-left zone. The badge contains the engine short name in Space Grotesk Bold, uppercase. This is the element that must survive at 200px — it should be minimum 18% canvas width.

**Pack name.** Space Grotesk Bold, WARM_WHITE, below or adjacent to the engine badge. At 200px this will be partially illegible — that is acceptable. The engine badge is the 200px anchor; the pack name reads at 400px and above.

**Mood indicator.** Bottom-left, a small colored capsule (4% canvas height) containing the mood name in Space Grotesk Medium. Color: the mood's canonical tint (Foundation = muted earth, Atmosphere = sky blue, Entangled = dual-color split, Prism = spectral gradient, Flux = kinetic orange, Aether = near-white, Family = XO Gold). This element does not need to be legible at 200px — it adds color information at the bottom edge that differentiates mood at a glance.

---

## 3. Pack Name Typography

### 3.1 Font: Space Grotesk

Pack display names use **Space Grotesk Bold** exclusively. No other font for pack titles. Rationale: Space Grotesk is the XO_OX display font; it is geometric but warm; it does not look like a default; it reads at small sizes due to consistent stroke weight.

**Size rules (relative to canvas):**
- Pack name: 5.5% canvas height
- Engine badge label: 3.5% canvas height
- Mood indicator: 2% canvas height
- XO_OX wordmark: 2.8% canvas height
- Metadata (preset count, version): 2.2% canvas height in JetBrains Mono

### 3.2 Name Rules

Pack names: 2–3 words, maximum 25 characters including spaces.

**Structural patterns that work:**
- Noun + Adjective: "Ocelot Winter", "Gravity Hollow", "Brass Storm"
- Place + Material: "Porto Copper", "Shore Glass", "Trench Carbon"
- Action + Object: "Split River", "Fold Fire", "Breach Light"

**Forbidden words:** Essential, Classic, Ultimate, Premium, Pro, Best, Top, Vol, Collection (as first word), Pack (never in the name). These are category labels that compress zero character information. Every word must be doing sonic or mythological work.

**Name-to-accent color relationship:** The pack name should not contradict the engine accent color. An ONSET (Electric Blue) pack should not have a name with warm connotations like "Ember Noon." The name extends the color world — it does not override it.

---

## 4. Collection Visual Identities

### 4.1 Kitchen Essentials — Warm Domestic Palette

Kitchen Essentials covers share a warm, slightly desaturated palette that signals the domestic and material. Engine accent colors are still present but shifted warm (+10° hue rotation toward amber) in the motif layer. Background uses a warm near-black with faint textile grain (a noise layer at 8% opacity simulating linen or kraft paper).

**Persistent Kitchen element:** A thin horizontal rule at 25% from top, WARM_WHITE at 30% opacity, width 80% canvas — references a counter edge, a shelf, a horizon.

**Typography treatment:** Pack names in Kitchen Essentials are material and plain. No astronomical or aquatic references. Words like: Copper, Salt, Smoke, Clay, Grain, Ember, Char, Steep.

**Badge variation:** Engine pill uses the collection's warm-shifted accent, with "KITCHEN" in small caps below the engine name in the badge.

### 4.2 Travel / Vessels — Cartographic and Nautical

Travel/Vessels covers reference cartography, navigation instruments, and vessel materials. Background motifs incorporate coordinate grid overlays (faint at 12% opacity) and compass rose fragments. The accent color is used full-strength but the bg_base shifts to a deep sea-ink navy regardless of engine hue.

**Persistent Travel element:** A subtle coordinate pair at bottom-left in JetBrains Mono, 1.8% canvas height, 40% opacity. Not real coordinates — derived from the engine's depth zone in the water column atlas (e.g., OSPREY at photic zone = `N 48° 12' / W 125° 44'`).

**Typography treatment:** Pack names reference vessel types, ports, routes, weather. Words like: Drift, Haul, Shore, Wake, Tide, Passage, Chart, Stern.

**Sub-collection differentiation:** Sail packs use wave-morph motif regardless of engine default. Industrial packs use angular-cuts motif. Leisure packs use particle-scatter. Historical packs use freq-bands.

### 4.3 Artwork / Color — Bold Color Fields

Artwork/Color covers are the most visually assertive in the fleet. Each quad has a designated complementary color pair (defined in `artwork_collection_overview.md`). The cover uses the primary accent color as a full-bleed background (not near-black) and the complementary color for the motif layer and typography.

**Color Quad A** (Oxblood/Onyx/Ochre/Orchid): Deep field backgrounds, earth complementary pairs.
**Color Quad B** (Ottanio/Oma'oma'o/Ostrum/Ōni): Medium-saturation fields, cultural complementary pairs.

**Critical rule for Artwork packs:** The background is NOT near-black. It is the accent color at 70% saturation and 35% brightness. This makes Artwork packs instantly visually distinct from any other XO_OX category — they are vivid, not dark.

**Typography treatment:** Pack names reference art movements, color theory, visual techniques. Words: Ground, Fold, Field, Wash, Layer, Glaze, Void, Mass.

**Persistent Artwork element:** A thin vertical strip at 88% from left edge, complementary accent color at 100% opacity, 2% canvas width — references a painting's edge, a color swatch, a slide mount.

---

## 5. MPC Expansion Browser Display Requirements

### 5.1 Size Contexts

The MPC browser displays cover art at two primary sizes: **200×200px** (grid browse) and **400×400px** (detail/preview). The 500px export serves both contexts (downsampled by MPC). The 1000px export serves MPC XL and retina displays.

### 5.2 200px Survival Rules

At 200px, only three elements need to be legible or visually meaningful:

1. **Engine accent color** — must be present as a large, high-contrast field. The accent bar at the left edge is 1.2% canvas width (= ~2.4px at 200px, which is 1 device pixel and invisible). The engine badge and motif are the accent color carriers. The motif must have at least 30% canvas area filled with accent-saturated pixels.

2. **Engine badge** — must read at minimum as a colored pill shape, even if the text is illegible. Minimum badge width: 20% canvas. Rounded corners radius: 8% of badge height.

3. **Visual contrast** — dark background with light typography, or light field with dark typography (Artwork/Color packs). The pack must not look like a gray smear at 200px.

**What to test:** Downsample a 2000px render to 200px using nearest-neighbor (not LANCZOS) and check that the engine accent color reads, the badge is visible as a shape, and the mood capsule adds a second color at the bottom edge.

### 5.3 400px Full Read

At 400px, the pack name should be legible, the engine badge should be legible, and the mood indicator should be identifiable by color even if text is small.

---

## 6. Print and Merch Extension

### 6.1 Sticker Sheets

XPN cover art scales to sticker format with one constraint: the full-bleed motif must work on both white and transparent sticker stock. The near-black bg_base covers are optimized for dark sticker stock. A "sticker variant" derive changes only the background: set bg_base to transparent (RGBA), keep all foreground elements. The engine pill, pack name, mood capsule, and XO_OX wordmark float on transparent background — functional on any sticker color.

**Sheet layout:** 4×4 grid of 16 stickers, each 50mm square. A collection sheet groups 12 packs from one engine or one collection tier. The XO_OX wordmark in XO Gold anchors the sheet title area.

### 6.2 Zines

The 2000px master cover art is print-ready at 167 DPI for A5 zine page (120×120mm). For a zine context, the cover art becomes a chapter opener or section divider. The typographic elements (pack name, engine badge) need no modification — they are already sized for print. The motif layer benefits from the AI-assist photographic path (deferred) for zine quality.

**Zine system:** One zine per collection. Kitchen Essentials zine = 24-page, one spread per engine (cover art full-bleed on left, sound design notes on right). Cover: collection wordmark on WARM_WHITE.

### 6.3 T-Shirts and Apparel

Cover art as apparel requires isolation of the foreground layer. The motif is too visually complex for screen printing. The apparel variant uses:
- XO_OX wordmark (large, centered)
- Engine pill (pack-specific, below wordmark)
- Accent color as the garment color (not printed on white)

This is a "merch-first" variant derived from the same template, not a modified cover. The cover's bg_base color maps to the garment color, the accent maps to the print color.

---

## 7. Community Pack Cover Art — Template System

### 7.1 Design Problem

Community packs must look XO_OX without requiring Pillow/NumPy expertise or access to the cover art generator. The goal is visual coherence across first-party and community packs in the MPC browser.

### 7.2 Template Approach

A set of **8 locked SVG templates** — one per motif style — that community creators can fill in via variable substitution. Each template has three user-editable zones:

| Zone | Variable | Input type |
|------|----------|-----------|
| Background color | `bg_hex` | hex color picker (restricted to ENGINE_DEFS accents or custom) |
| Accent color | `accent_hex` | hex color picker |
| Pack name | `pack_name` | text input, max 25 chars, Space Grotesk Bold enforced |

All other elements (XO_OX wordmark position and size, badge structure, mood capsule layout, motif pattern) are locked.

The template pack includes an `xo_ox_community_cover_kit.zip` containing:
- 8 SVG templates (one per motif style)
- `SpaceGrotesk-Bold.ttf` + `JetBrainsMono-Regular.ttf` (OFL licensed, safe to redistribute)
- A one-page PDF guide: "Three variables. One template. Your pack."
- A `community_cover_validator.py` script that accepts a PNG and checks: aspect ratio is 1:1, dominant color matches an ENGINE_DEFS accent within ±20 HSL points, XO_OX wordmark region is present (pixel signature check).

### 7.3 Attribution Badge

Community covers include a small "Community" badge alongside the engine pill — same pill structure, WARM_WHITE background, dark text. This distinguishes community packs from first-party in the browser without degrading their visual quality.

---

## 8. Animated Cover Art

### 8.1 MPC 3.x Browser Animation Support

MPC 3.x firmware added preliminary support for animated images in the expansion browser — animated GIF and APNG are parsed by the asset loader, though behavior depends on firmware version. This is undocumented in the official SDK. Animated covers appear in the browser grid as looping animations when the pack is highlighted.

### 8.2 Animation Design Constraints

Any animated cover must:
- Loop seamlessly (first and last frames must be visually continuous).
- Work as a static image if animation is not supported (the first frame must be a complete, correct cover).
- Be under 2MB as APNG for the 500px resolution. Animated GIF is acceptable for MPC compatibility but APNG preserves alpha and color depth.
- Loop duration: 4–8 seconds. Faster loops feel nervous; slower loops feel purposeful.

### 8.3 Motion Concepts by Style

**transient_spikes (ONSET):** Bars pulse from bottom up in sequence, a slow left-to-right sweep at 1Hz. Reads as a drum machine sequencer.

**lissajous (DRIFT, ODYSSEY):** The Lissajous curves rotate slowly — phase increment of 0.005 radians per frame. Two curves counter-rotate. Hypnotic at 6 seconds/loop.

**wave_morph (MORPH, OHM, OVERLAP):** The sine wave phase advances at constant speed. Appears as a slow horizontal flow. Almost imperceptible at 200px — works as a presence signal rather than obvious motion.

**particle_scatter (OPAL, ORGANON, OPENSKY):** Particles drift outward from center and fade at edges, replaced by new particles. Reads as a breathing or breathing-in motion.

**pixel_grid (OVERWORLD, OUTWIT):** Individual pixels randomly flip brightness, simulating a live display. Fast variant: 0.1s/frame. Slow variant: 0.4s/frame (preferred — fast looks like noise).

**tape_streaks (DUB, OVERDUB, OBSCURA):** Streaks advance along their diagonal vector and wrap. At 500px this is 3–5 pixels of motion per frame — subtle but unmistakable as tape transport.

**freq_bands (BOB, OTTONI, OSPREY):** Band amplitudes animate up and down as a simulated spectrum analyzer. No actual audio data — deterministic sine-wave envelope modulation of band height.

**angular_cuts (SNAP, OLE, OVERBITE):** Cuts shift phase slowly, one angle per 8 frames. The pattern breathes rather than moves — a slow pulse rather than a continuous animation.

**dense_blocks (FAT, OCEANDEEP):** Blocks fade in and out at staggered intervals. Gives a flickering, stratified depth.

### 8.4 Implementation Path

`generate_cover_animated()` wraps `generate_cover()` with a frame loop. Each frame advances a `phase` variable passed to the style function. Pillow's `save(format="PNG", save_all=True, append_images=frames, loop=0, duration=frame_ms)` generates APNG directly. A 6-second loop at 12fps = 72 frames. Estimated generation time per pack: ~30s on M-series hardware (72× the single-frame cost). Batch generation should be optional and gated behind an `--animated` flag.

---

## Implementation Priority

| Priority | Item | Effort |
|----------|------|--------|
| P1 | Bundle Space Grotesk + JetBrains Mono in `Tools/fonts/`, update `try_font()` | 1 hour |
| P1 | Mood indicator capsule element in text overlay | 2 hours |
| P1 | Increase XO_OX wordmark opacity to 85% | 15 min |
| P2 | Engine badge minimum 20% width enforcement | 1 hour |
| P2 | Collection visual variant flags (`--collection kitchen`, `--collection travel`, `--collection artwork`) | 4 hours |
| P2 | Sticker variant export (`--sticker`, transparent bg) | 2 hours |
| P3 | SVG template kit for community creators | 1 day |
| P3 | `community_cover_validator.py` | 3 hours |
| P4 | Animated APNG export (`--animated`) | 1 day |
| P4 | AI-assist photographic background path (Opus-level, deferred to V2) | TBD |

---

## Open Questions

1. Should the XO_OX wordmark be a path-rendered SVG glyph embedded in Pillow via `aggdraw`, or a font-rendered bitmap? The font route is simpler but the mark is not designed as a typeface glyph.
2. Artwork/Color collection packs use full-saturation backgrounds — do these read correctly on the MPC's LCD at various brightness settings? Needs hardware test before committing to the approach.
3. The "Community" attribution badge distinguishes community packs in the browser. Is this desirable from a community-building perspective, or does it create a two-tier system that discourages submission?
4. Animated APNG support in the MPC browser is undocumented. A firmware test on MPC One and MPC Live II running 3.x is required before building the animation pipeline.
