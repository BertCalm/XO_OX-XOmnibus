#!/usr/bin/env python3
"""
xpn_pack_thumbnail_spec.py — XO_OX Pack Thumbnail Design Brief Generator

Generates a structured Markdown design brief for XPN pack cover art (512×512px).
Output is a human-readable spec for a designer or image generation tool.

Usage:
    python xpn_pack_thumbnail_spec.py --name "Pack Name" --engines OPAL ONSET \
        --mood Atmosphere --brightness 0.4 --warmth 0.6 [--output brief.md]
"""

import argparse
import sys
from datetime import date

# ---------------------------------------------------------------------------
# Engine accent color registry (canonical — matches XOceanus CLAUDE.md table)
# ---------------------------------------------------------------------------
ENGINE_COLORS = {
    "OBLONG":     "#FF6B6B",
    "OVERBITE":   "#F0EDE8",
    "OVERDUB":    "#7EC8E3",
    "ODYSSEY":    "#C8A8E9",
    "ONSET":      "#E8C547",
    "OPAL":       "#A78BFA",
    "OVERWORLD":  "#00CED1",
    "FAT":        "#FF8C42",
    "OVERLAP":    "#00FFB4",
    "OUTWIT":     "#CC6600",
    "OHM":        "#9DC88D",
    "ORPHICA":    "#D4A5C9",
    "OBBLIGATO":  "#87CEEB",
    "OTTONI":     "#DAA520",
    "OLE":        "#FF4500",
    "OVERTONE":   "#A8D8EA",
    "KNOT":       "#8E4585",
    "ORGANISM":   "#C6E377",
    "OSTINATO":   "#F4A261",
    "OPENSKY":    "#90E0EF",
    "OCEANDEEP":  "#023E8A",
    "OUIE":       "#F72585",
}

XO_GOLD = "#E9C46A"
SHELL_WHITE = "#F8F6F3"

# ---------------------------------------------------------------------------
# DNA dimension descriptors
# ---------------------------------------------------------------------------
def dna_brightness_direction(v: float) -> dict:
    if v >= 0.7:
        return {
            "background": "Light, near-white background. High luminosity. Background tone: near " + SHELL_WHITE + " with slight cool shift.",
            "shapes": "Sharp, crisp geometric shapes. Hard-edged silhouettes. Clean negative space.",
            "lighting": "Bright, flat or high-key lighting. Minimal shadow depth.",
        }
    elif v >= 0.4:
        return {
            "background": "Mid-tone background. Balanced luminosity. Subtle gradient from " + SHELL_WHITE + " toward a muted accent tone.",
            "shapes": "Mixed geometry — some hard edges, some soft curves. Moderate contrast.",
            "lighting": "Neutral lighting with soft directional shadow.",
        }
    else:
        return {
            "background": "Dark or deeply muted background. Low-key treatment. Rich shadow.",
            "shapes": "Recessed shapes with high-contrast highlights. Strong chiaroscuro.",
            "lighting": "Low-key lighting, dramatic shadows, glowing accent color pops.",
        }

def dna_warmth_direction(v: float) -> dict:
    if v >= 0.7:
        return {
            "palette_temp": "Warm earth tones dominate. Ambers, terracottas, warm golds. " + XO_GOLD + " as a primary accent.",
            "forms": "Organic, flowing curves. Rounded forms. Nature-inspired silhouettes.",
        }
    elif v >= 0.4:
        return {
            "palette_temp": "Balanced temperature. Warm neutrals with cool highlights. Gold accent used sparingly.",
            "forms": "Mixed organic and geometric. Subtle curves with structured framing.",
        }
    else:
        return {
            "palette_temp": "Cool, desaturated tones. Blue-greys and silvers. XO Gold only as a single focal accent.",
            "forms": "Precise geometric forms. Grid-based or crystalline structure. Minimal organic softness.",
        }

def dna_movement_direction(v: float) -> dict:
    if v >= 0.7:
        return "Dynamic diagonal composition. Implied motion — sweeping arcs, velocity streaks, or motion-blur-style gradients along a 30–45° axis. Energy flows from lower-left to upper-right."
    elif v >= 0.4:
        return "Moderate dynamism. Off-center focal point. Slight diagonal tension without full sweep. One dominant directional element."
    else:
        return "Still, balanced composition. Centered or symmetrical layout. No implied motion. Stable horizon-line structure."

def dna_density_direction(v: float) -> dict:
    if v >= 0.7:
        return "Dense, layered composition. Multiple overlapping elements, texture layers, and fine detail. Complex pattern work fills the frame. Depth through z-stacking."
    elif v >= 0.4:
        return "Moderate complexity. 3–5 distinct visual elements. Some layering but clear focal hierarchy. Secondary texture at reduced opacity."
    else:
        return "Sparse, minimal composition. 1–2 dominant elements. Generous negative space. No texture fill — let the background breathe."

def dna_space_direction(v: float) -> dict:
    if v >= 0.7:
        return "Minimal composition with large breathing room. Subject occupies 20–35% of frame. Vast negative space, silences the eye. Air and openness are the message."
    elif v >= 0.4:
        return "Balanced use of space. Subject at ~50% frame weight. Comfortable margin around all elements."
    else:
        return "Packed composition. Subject fills 70–85% of frame. Tight cropping, immersive density."

def dna_aggression_direction(v: float) -> dict:
    if v >= 0.7:
        return "Bold, high-contrast treatment. Hard edges, sharp cuts, no soft transitions. Maximum value contrast. Black and accent with no mid-tones in key areas."
    elif v >= 0.4:
        return "Moderate contrast. Some hard edges balanced with softer gradients. Controlled tension."
    else:
        return "Soft, low-contrast treatment. Smooth gradients, feathered edges. Gentle, restrained energy."

# ---------------------------------------------------------------------------
# Mood → color temperature mapping
# ---------------------------------------------------------------------------
MOOD_PALETTES = {
    "Foundation": {
        "description": "Neutral warm white with gold accents. Grounded, essential, timeless.",
        "primary_bg": SHELL_WHITE,
        "accent_1": XO_GOLD,
        "accent_2": "#D4C5A9",
        "gradient": "Subtle warm-white to light cream radial gradient from center.",
        "typography_color": "#2C2416",
    },
    "Atmosphere": {
        "description": "Blue-grey with misty soft gradients. Expansive, aerial, diffuse.",
        "primary_bg": "#B8C5D0",
        "accent_1": "#7A9BB5",
        "accent_2": "#E8EDF2",
        "gradient": "Vertical gradient from pale sky-grey at top to deeper blue-grey at bottom. Soft mist layer at 30% opacity.",
        "typography_color": "#1A2A38",
    },
    "Entangled": {
        "description": "Two-color split using complementary engine accent colors. Tension and unity.",
        "primary_bg": "Split-diagonal: engine accent A fills upper-left triangle, engine accent B fills lower-right. Thin XO Gold separator line at diagonal edge.",
        "accent_1": "Engine accent color 1 (see palette section)",
        "accent_2": "Engine accent color 2 (see palette section)",
        "gradient": "Each triangle carries a subtle radial highlight at its corner. No soft blend at the seam — hard diagonal cut.",
        "typography_color": SHELL_WHITE,
    },
    "Prism": {
        "description": "Full spectrum gradient, prismatic refraction. Light dispersal and color decomposition.",
        "primary_bg": "Linear gradient 135°: violet → indigo → cyan → green → gold → amber. Saturated.",
        "accent_1": XO_GOLD,
        "accent_2": "#A78BFA",
        "gradient": "Prismatic band overlay at 20% opacity — thin rainbow stripe across mid-frame at slight tilt.",
        "typography_color": SHELL_WHITE,
    },
    "Flux": {
        "description": "Flowing gradients with transitional, in-between colors. Becoming, not being.",
        "primary_bg": "Multi-stop flowing gradient: engine accent → XO Gold → near-white. Smooth S-curve path.",
        "accent_1": XO_GOLD,
        "accent_2": "#F0E8D8",
        "gradient": "Organic, non-linear gradient flow — not a hard diagonal. Resembles ink in water.",
        "typography_color": "#1A1A1A",
    },
    "Aether": {
        "description": "High-key whites and ethereal pale tones. Luminous, barely-there, transcendent.",
        "primary_bg": "#FAFAF8",
        "accent_1": "#E8E0F0",
        "accent_2": "#D4EEF8",
        "gradient": "Near-white radial gradient with the faintest breath of lavender or ice-blue at edges.",
        "typography_color": "#8888A0",
    },
    "Family": {
        "description": "Warm amber and inclusive gathering energy. Hearth, community, belonging.",
        "primary_bg": "#F5E6C8",
        "accent_1": XO_GOLD,
        "accent_2": "#C8956C",
        "gradient": "Warm amber radial gradient from golden center, cooling to soft terracotta at edges.",
        "typography_color": "#3C2010",
    },
}

# ---------------------------------------------------------------------------
# Engine color interaction notes
# ---------------------------------------------------------------------------
def engine_interaction_notes(engine_colors: dict) -> str:
    names = list(engine_colors.keys())
    colors = list(engine_colors.values())
    if len(names) == 1:
        return f"Single engine accent: {names[0]} ({colors[0]}). Use as primary tint layer at 60% opacity over background. Full saturation version as focal highlight."
    elif len(names) == 2:
        return (
            f"Two engine accents: {names[0]} ({colors[0]}) and {names[1]} ({colors[1]}). "
            f"Use {names[0]} as the dominant accent (70% weight) and {names[1]} as the counterpoint (30% weight). "
            f"Allow a narrow blended zone where they meet. Do not fully mix — preserve each identity."
        )
    else:
        parts = [f"{n} ({c})" for n, c in zip(names, colors)]
        dominant = parts[0]
        supporting = ", ".join(parts[1:])
        return (
            f"Multiple engine accents: {dominant} leads as dominant color. "
            f"Supporting accents ({supporting}) appear as secondary highlights, used at reduced opacity (20–40%) "
            f"or as fine detail accents. Avoid equal-weight mixing — maintain clear visual hierarchy."
        )

# ---------------------------------------------------------------------------
# Build the brief
# ---------------------------------------------------------------------------
def generate_brief(args) -> str:
    engines_upper = [e.upper() for e in args.engines]
    found_colors = {e: ENGINE_COLORS[e] for e in engines_upper if e in ENGINE_COLORS}
    unknown = [e for e in engines_upper if e not in ENGINE_COLORS]

    mood_key = args.mood.capitalize() if args.mood.capitalize() in MOOD_PALETTES else "Foundation"
    mood = MOOD_PALETTES[mood_key]

    brightness = max(0.0, min(1.0, args.brightness))
    warmth = max(0.0, min(1.0, args.warmth))
    movement = max(0.0, min(1.0, args.movement))
    density = max(0.0, min(1.0, args.density))
    space = max(0.0, min(1.0, args.space))
    aggression = max(0.0, min(1.0, args.aggression))

    bright_dir = dna_brightness_direction(brightness)
    warmth_dir = dna_warmth_direction(warmth)
    movement_dir = dna_movement_direction(movement)
    density_dir = dna_density_direction(density)
    space_dir = dna_space_direction(space)
    aggression_dir = dna_aggression_direction(aggression)

    engine_color_list = "\n".join(
        f"  - **{name}**: `{color}`" for name, color in found_colors.items()
    )
    if unknown:
        engine_color_list += "\n" + "\n".join(
            f"  - **{name}**: _(unknown engine — assign manually)_" for name in unknown
        )
    if not engine_color_list:
        engine_color_list = "  _(No engines specified)_"

    engine_notes = engine_interaction_notes(found_colors) if found_colors else "No known engine accents — use XO Gold as primary accent."

    brief = f"""# XO_OX Pack Thumbnail Design Brief
**Pack**: {args.name}
**Generated**: {date.today().isoformat()}
**Tool**: xpn_pack_thumbnail_spec.py

---

## 1. Specifications

| Property | Value |
|---|---|
| Dimensions | 512 × 512 px |
| Format | JPEG (quality 92+) |
| Color space | sRGB |
| Typography | Space Grotesk (Bold weight for pack name) |
| Shell White | `{SHELL_WHITE}` |
| XO Gold | `{XO_GOLD}` |

---

## 2. Engines & Accent Colors

{engine_color_list}

**Interaction note**: {engine_notes}

---

## 3. Mood: {mood_key}

> {mood['description']}

| Role | Color |
|---|---|
| Primary Background | `{mood['primary_bg']}` |
| Accent 1 | `{mood['accent_1']}` |
| Accent 2 | `{mood['accent_2']}` |
| Typography | `{mood['typography_color']}` |

**Gradient treatment**: {mood['gradient']}

---

## 4. DNA Centroid → Visual Direction

| Dimension | Value | Direction |
|---|---|---|
| Brightness | {brightness:.2f} | {"High" if brightness >= 0.7 else "Mid" if brightness >= 0.4 else "Low"} |
| Warmth | {warmth:.2f} | {"High" if warmth >= 0.7 else "Mid" if warmth >= 0.4 else "Low"} |
| Movement | {movement:.2f} | {"High" if movement >= 0.7 else "Mid" if movement >= 0.4 else "Low"} |
| Density | {density:.2f} | {"High" if density >= 0.7 else "Mid" if density >= 0.4 else "Low"} |
| Space | {space:.2f} | {"High" if space >= 0.7 else "Mid" if space >= 0.4 else "Low"} |
| Aggression | {aggression:.2f} | {"High" if aggression >= 0.7 else "Mid" if aggression >= 0.4 else "Low"} |

### Background & Lighting
{bright_dir['background']}
{bright_dir['lighting']}

### Palette Temperature
{warmth_dir['palette_temp']}

### Forms & Structure
{warmth_dir['forms']}

### Composition Movement
{movement_dir}

### Density & Complexity
{density_dir}

### Use of Space
{space_dir}

### Contrast & Edge Treatment
{aggression_dir}

---

## 5. Typography

- **Pack name**: "{args.name}"
- **Typeface**: Space Grotesk Bold
- **Size**: 36–48px (scale to fit in one line if possible; wrap to two lines maximum)
- **Color**: `{mood['typography_color']}`
- **Position**: Lower-left quadrant, 24px margin from edges
- **Treatment**: No drop shadow. If contrast is insufficient, add a 4px blur semi-transparent backing rectangle at 30% opacity in `{SHELL_WHITE}`.
- **Engine names** (optional secondary label): Space Grotesk Regular, 14px, 60% opacity, same color, stacked below pack name with 6px gap.

---

## 6. Full Color Palette

| Swatch | Name | Hex |
|---|---|---|
| Shell White | XO_OX base | `{SHELL_WHITE}` |
| XO Gold | XO_OX gold | `{XO_GOLD}` |
| Mood BG | {mood_key} background | `{mood['primary_bg']}` |
| Mood Accent 1 | {mood_key} accent | `{mood['accent_1']}` |
| Mood Accent 2 | {mood_key} secondary | `{mood['accent_2']}` |
{"".join(f"| {name} accent | Engine | `{color}` |{chr(10)}" for name, color in found_colors.items())}
---

## 7. Composition Notes

The composition should feel like a **512×512 album cover** for a boutique electronic instrument pack, not a software product screenshot.

- **Primary focal element**: An abstract or semi-abstract form anchored near the visual center, offset slightly toward upper-right. Suggested forms based on DNA: {"radiant geometric burst" if brightness >= 0.6 else "dense layered texture" if density >= 0.6 else "fluid wave form" if movement >= 0.6 else "balanced abstract glyph"}.
- **Secondary element**: A subtle echo or shadow of the primary form, reduced to 20–30% opacity, displaced 40–80px toward lower-left.
- **XO_OX watermark**: "XO_OX" in Space Grotesk Light, 11px, `{XO_GOLD}` at 50% opacity, upper-right corner, 16px margin.
- **Border treatment**: {"None — let the image bleed to edges." if aggression >= 0.5 else f"Thin 1px border in `{XO_GOLD}` at 30% opacity, inset 8px from all edges."}

---

## 8. Mood Reference

**{mood_key}** — {mood['description']}

Designer reference touchstones:
{"- Dieter Rams product photography. Jan Wenzel book covers. Clean Swiss typography." if mood_key in ("Foundation", "Aether") else ""}
{"- Hiroshi Yoshida woodblock prints. Atmospheric horizon photography. Fog over water." if mood_key == "Atmosphere" else ""}
{"- Josef Albers color interaction studies. Bauhaus complementary contrast posters." if mood_key == "Entangled" else ""}
{"- Prism dispersal photography. Newton's Opticks plate engravings. DSBM album art." if mood_key == "Prism" else ""}
{"- Marbled paper endpapers. Oil-on-water photography. Slow shutter liquid." if mood_key == "Flux" else ""}
{"- Agnes Martin paintings. James Turrell light installations. Minimalist bleached photography." if mood_key == "Aether" else ""}
{"- Saul Bass film poster warmth. WPA national park poster palette. Community mural energy." if mood_key == "Family" else ""}

---

*Brief generated by XO_OX xpn_pack_thumbnail_spec.py — {date.today().isoformat()}*
"""
    return brief.strip()

# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------
def main():
    parser = argparse.ArgumentParser(
        description="Generate a design brief for an XPN pack thumbnail (512×512px cover art)."
    )
    parser.add_argument("--name", required=True, help="Pack name (e.g. 'Deep Water Sessions')")
    parser.add_argument("--engines", nargs="+", required=True, metavar="ENGINE",
                        help="Engine names, e.g. OPAL ONSET OVERLAP")
    parser.add_argument("--mood", default="Foundation",
                        choices=["Foundation", "Atmosphere", "Entangled", "Prism", "Flux", "Aether", "Family"],
                        help="Dominant mood category")
    parser.add_argument("--brightness", type=float, default=0.5,
                        help="DNA brightness centroid 0.0–1.0 (default 0.5)")
    parser.add_argument("--warmth", type=float, default=0.5,
                        help="DNA warmth centroid 0.0–1.0 (default 0.5)")
    parser.add_argument("--movement", type=float, default=0.5,
                        help="DNA movement centroid 0.0–1.0 (default 0.5)")
    parser.add_argument("--density", type=float, default=0.5,
                        help="DNA density centroid 0.0–1.0 (default 0.5)")
    parser.add_argument("--space", type=float, default=0.5,
                        help="DNA space centroid 0.0–1.0 (default 0.5)")
    parser.add_argument("--aggression", type=float, default=0.5,
                        help="DNA aggression centroid 0.0–1.0 (default 0.5)")
    parser.add_argument("--output", metavar="FILE",
                        help="Write brief to this file (default: print to stdout)")

    args = parser.parse_args()
    brief = generate_brief(args)

    if args.output:
        with open(args.output, "w", encoding="utf-8") as f:
            f.write(brief + "\n")
        print(f"Brief written to: {args.output}")
    else:
        print(brief)

if __name__ == "__main__":
    main()
