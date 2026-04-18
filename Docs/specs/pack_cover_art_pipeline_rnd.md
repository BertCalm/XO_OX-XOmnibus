# Pack Cover Art Pipeline — R&D

**Topic:** Automated and semi-automated cover art generation for XO_OX XPN packs
**Date:** 2026-03-16
**Status:** Research / Pre-implementation

---

## 1. Current State — What `xpn_cover_art.py` Produces

`Tools/xpn_cover_art.py` is a self-contained Python raster pipeline built on Pillow + NumPy. It renders procedural cover art at up to 2000×2000px (PNG) using engine-specific generative styles.

**Pipeline summary:**

1. A dark vignette base canvas is generated from the engine's `bg_base` RGB tuple using a radial brightness gradient.
2. One of eight style functions is applied over the base — each is a NumPy-level float32 operation driven by a seeded RNG. Styles: `transient_spikes`, `pixel_grid`, `angular_cuts`, `wave_morph`, `tape_streaks`, `lissajous`, `freq_bands`, `dense_blocks`, `particle_scatter`.
3. If spectral fingerprints are passed (`xpn_optic_fingerprint` integration), a second modulation pass adjusts centroid brightness, warm tint, contrast from transient density, polarity tint (feliX blue / Oscar pink), and draws an 8-band silhouette bar graph.
4. PIL text overlay adds: left-edge accent color bar, pack title with drop shadow, engine label in a rounded-rectangle pill, preset count in XO Gold, version string, and a faint `XO_OX` watermark.
5. The master render is 2000px; outputs are LANCZOS-downsampled to requested resolutions (500, 1000, 2000). A `pad_colors.json` manifest can be co-written for downstream XPM injection.

**Limitations of the current approach:**

- Raster-only: scaling requires re-render or LANCZOS downsample; no vector source to derive print-ready or animated variants from.
- Font loading cascades through hard-coded macOS system paths, falling back to PIL's built-in bitmap default. The XO_OX brand fonts (Space Grotesk, Inter, JetBrains Mono) are never used.
- Each style function is monolithic; compositing order is fixed (base → style → fingerprint → text). New visual motifs require new style functions, not template overrides.
- No batch driver — each pack requires a direct Python call or a shell loop calling the CLI.

---

## 2. SVG-First Pipeline

SVG is the correct intermediate format for this pipeline for four reasons:

1. **Resolution independence.** One SVG master downsizes to 500px MPC thumbnail or scales to poster print with no quality loss. PNG exports become a compilation step, not the source.
2. **Scriptability.** Python's `xml.etree.ElementTree` or `lxml` can write complete SVG documents without external dependencies. Variable substitution is string interpolation over a template DOM.
3. **Version control and diffability.** SVG is plain text. Two cover variants for the same pack differ by a handful of hex color and text values — readable in `git diff`, reviewable in PR.
4. **Downstream compositing.** SVG layers are individually addressable. An animated variant for the site or a cropped social card can reuse the same source by hiding or repositioning named groups.

**Proposed template schema:**

```
XOPackCoverTemplate
├── layer: background         — radial gradient from bg_base to near-black
├── layer: motif              — engine-style geometry (SVG paths / defs, seeded)
├── layer: fingerprint_bars   — optional 8-band silhouette (rect elements)
├── layer: accent_bar         — left-edge solid rect, fill = accent_color
├── layer: text_pack_name     — Space Grotesk, WARM_WHITE
├── layer: engine_pill        — rounded rect + label, fill = accent_color
├── layer: metadata           — preset count (XO Gold), version (muted)
└── layer: watermark          — XO_OX, 22% opacity, bottom-right
```

Template variables (substituted at generation time):

```json
{
  "pack_name":     "808 Reborn Collection",
  "engine_label":  "ONSET",
  "accent_color":  "#0066FF",
  "bg_base":       "#00081C",
  "motif_style":   "transient_spikes",
  "preset_count":  20,
  "version":       "1.0",
  "rng_seed":      42
}
```

The motif layer is the only part that requires procedural generation. All other layers are pure SVG primitives with substituted values. A renderer calls `cairosvg` or `librsvg` (via subprocess) to convert the final SVG to PNG at each requested resolution, inheriting LANCZOS-equivalent subpixel rendering from the SVG viewport transform.

---

## 3. Engine Accent Color Integration

All 34 registered engines have canonical accent colors defined in `ENGINE_DEFS` (in sync with the CLAUDE.md engine table). Cover art should derive its visual identity directly from these values.

**Single-engine pack (dominant color):**
The accent color drives the motif layer fill, the accent bar, the engine pill background, and the radial vignette brightest point. The `bg_base` (near-black tinted toward the accent hue) provides depth without muddying the accent. This is the current behavior — it works well and should be preserved.

**Multi-engine pack (2–4 engines):**
Options:
- *Gradient:* A linear or angular gradient interpolates between up to 4 accent colors across the background. The accent bar is replaced by a thin multi-color stripe. The dominant engine (highest preset count or listed first) determines motif style.
- *Quadrant grid:* For 4-engine packs, the canvas is divided into a 2×2 grid with each quadrant filled by one engine's accent color and a miniaturized motif. A neutral XO Gold ring frames the center intersection. Suitable for Constellation family and collection packs.

**Collection pack (full palette):**
A collection spanning many engines uses the full accent palette as a color mosaic or concentric ring diagram. The XO Gold brand constant anchors the center. Individual engine pills are omitted in favor of a collection title; the engine count is displayed instead of preset count.

A helper function `blend_accent_colors(engine_list) -> gradient_stops` should be added alongside `get_pad_color_hex()`, returning perceptually-sorted stops (sorted by HSL hue) to avoid muddy midpoints.

---

## 4. Typography System

XO_OX brand fonts: **Space Grotesk** (display / pack titles), **Inter** (body / metadata), **JetBrains Mono** (values / version strings, preset count).

The current tool never uses these — it cascades through Arial Bold and Helvetica system paths before falling back to PIL's bitmap font.

**Embedding approach for SVG/PNG without external deps:**

1. **Bundle fonts as base64 data URIs inside the SVG `<defs>`.** Python's `base64` stdlib encodes the `.ttf` or `.woff2` file. The SVG `@font-face` rule references a `data:font/woff2;base64,...` URI. No external paths required at render time; the SVG is self-contained.

2. **Ship the three font files inside `Tools/fonts/`.** The generator loads them once at startup and encodes them. File sizes: Space Grotesk Bold ~80KB, Inter Regular ~300KB, JetBrains Mono Regular ~200KB. All three are OFL-licensed and safe to bundle.

3. **For PNG-only paths (keeping Pillow):** Call `ImageFont.truetype(str(tools_dir / "fonts" / "SpaceGrotesk-Bold.ttf"), size_pt)` with a local path instead of the system font cascade. This is a one-line change to `try_font()`.

The SVG route is preferred: font rendering is delegated to the SVG renderer (cairo/librsvg), which handles hinting and subpixel rendering correctly at all output sizes.

---

## 5. Batch Generation

For 100+ packs, a batch driver is needed. All variable inputs are already machine-readable: engine name, pack name, preset count, and version are present in each pack's `manifest.json` (as generated by the packager tool).

**`xpn_cover_art_batch.py` — proposed CLI:**

```
python xpn_cover_art_batch.py \
  --manifest-glob "Packs/**/*.xpn/manifest.json" \
  --output-root   "Packs/"                       \
  --resolution    all                            \
  --seed-from-name                               \
  --skip-existing                                \
  --workers 4
```

Key flags:

| Flag | Behavior |
|------|----------|
| `--manifest-glob` | Glob pattern locating pack manifests; each supplies pack_name, engine, count, version |
| `--output-root` | Writes `artwork_500.png` etc. adjacent to each manifest |
| `--resolution` | Same tokens as current CLI: `500`, `1000`, `2000`, `all` |
| `--seed-from-name` | Derives RNG seed from `hash(pack_name) % 2**31` for deterministic, unique art per pack |
| `--skip-existing` | Skip packs that already have `artwork_500.png` (idempotent re-runs) |
| `--workers N` | `concurrent.futures.ProcessPoolExecutor` for parallel rendering |
| `--dry-run` | Print what would be generated without writing files |

Template variables available per manifest: `pack_name`, `engine_name`, `accent_color` (derived from `ENGINE_DEFS`), `mood_category`, `pack_number` (from manifest filename or explicit field), `preset_count`, `version`.

A run over 100 packs at `--resolution 500` with 4 workers completes in under 2 minutes on M-series hardware (current single-engine generation at 500px takes ~0.5–1s).

---

## 6. AI-Assist Path — Photographic Backgrounds (Opus-Level Deferred)

**Flagged as Opus-level deferred work.** Do not implement before SVG pipeline and batch driver are stable.

The XO_OX aquatic brand establishes a water column atlas: 9 depth zones from the photic surface to the abyssal trench. Each engine occupies a depth zone. Photographic or AI-generated backgrounds aligned to this mythology would provide a more organic, premium visual layer behind the geometric motifs.

**Proposed pipeline:**

1. For each engine, a text prompt is derived from the engine's depth zone, aquatic creature identity, and accent color. Example for ONSET: `"abyssal zone bioluminescent particles deep ocean electric blue 2000x2000 high contrast dark photography"`.
2. The prompt is sent to a DALL-E 3 or Stable Diffusion API (SDXL recommended for 1:1 aspect ratio and dark-scene quality). The response is a 2000×2000 photographic background image.
3. The procedural motif layer is composited over the photo at reduced opacity (30–50%), preserving the geometric engine identity while gaining photographic depth.
4. Text overlay and accent elements are unchanged.

**Considerations before committing to this path:**

- API cost at 100+ packs: DALL-E 3 at ~$0.08/image = $8–16 for the full fleet at 2 resolutions. Acceptable but non-zero.
- Reproducibility: photographic backgrounds must be stored in the repo (or in a locked asset store) so cover art does not change between builds. The seed-from-name approach works for procedural art but not for API images.
- Brand consistency: photographic results vary. A human review pass over all 34 engine backgrounds before committing to the batch pipeline is required.
- Licensing: DALL-E 3 outputs are commercially usable under OpenAI's terms. Stable Diffusion local models require model-specific license review.

This path is worth pursuing later after the procedural pipeline is shipping and stable.
