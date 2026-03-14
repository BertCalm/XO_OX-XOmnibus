#!/usr/bin/env python3
"""
XPN Cover Art Generator — XO_OX Designs
Generates branded procedural cover art for MPC expansion packs.

Outputs:
  - artwork_2000.png  (2000×2000, social/web master)
  - artwork.png       (1000×1000, MPC expansion cover)

Usage:
    from xpn_cover_art import generate_cover
    generate_cover(
        engine="ONSET",
        pack_name="808 Reborn Collection",
        preset_count=20,
        output_dir="/path/to/pack",
        version="1.0"
    )

Dependencies: Pillow, numpy
    pip install Pillow numpy
"""

import math
import os
import random
from pathlib import Path

try:
    import numpy as np
    from PIL import Image, ImageDraw, ImageFilter, ImageFont
    PILLOW_AVAILABLE = True
except ImportError:
    PILLOW_AVAILABLE = False


# =============================================================================
# ENGINE DEFINITIONS
# =============================================================================

ENGINE_DEFS = {
    "ONSET": {
        "accent":   (0,   102, 255),     # Electric Blue
        "bg_base":  (0,    8,  28),      # Near-black blue
        "style":    "transient_spikes",
        "label":    "ONSET",
    },
    "OVERWORLD": {
        "accent":   (57,  255, 20),      # Neon Green
        "bg_base":  (0,   12,   0),      # Near-black green
        "style":    "pixel_grid",
        "label":    "OVERWORLD",
    },
    "SNAP": {
        "accent":   (0,  166, 214),     # Neon Tetra Blue
        "bg_base":  (20,    8,   5),
        "style":    "angular_cuts",
        "label":    "SNAP",
    },
    "MORPH": {
        "accent":   (232, 131, 155),     # Axolotl Gill Pink
        "bg_base":  (0,   12,  12),
        "style":    "wave_morph",
        "label":    "MORPH",
    },
    "DUB": {
        "accent":   (107, 123,  58),     # Olive
        "bg_base":  (8,   10,   2),
        "style":    "tape_streaks",
        "label":    "DUB",
    },
    "DRIFT": {
        "accent":   (123,  45, 139),     # Violet
        "bg_base":  (8,    0,  18),
        "style":    "lissajous",
        "label":    "DRIFT",
    },
    "BOB": {
        "accent":   (233, 168,  74),     # Amber
        "bg_base":  (20,  12,   0),
        "style":    "freq_bands",
        "label":    "BOB",
    },
    "FAT": {
        "accent":   (255,  20, 147),     # Hot Pink
        "bg_base":  (18,   0,  10),
        "style":    "dense_blocks",
        "label":    "FAT",
    },
    "OPAL": {
        "accent":   (167, 139, 250),     # Lavender
        "bg_base":  (8,    4,  20),
        "style":    "particle_scatter",
        "label":    "OPAL",
    },
    # Canonical O-prefix engine names
    "ODDFELIX": {
        "accent":   (0,   166, 214),     # Neon Tetra Blue
        "bg_base":  (0,    6,  18),
        "style":    "angular_cuts",
        "label":    "ODDFELIX",
    },
    "ODDOSCAR": {
        "accent":   (232, 131, 155),     # Axolotl Gill Pink
        "bg_base":  (0,   12,  12),
        "style":    "wave_morph",
        "label":    "ODDOSCAR",
    },
    "OVERDUB": {
        "accent":   (107, 123,  58),     # Olive
        "bg_base":  (8,   10,   2),
        "style":    "tape_streaks",
        "label":    "OVERDUB",
    },
    "ODYSSEY": {
        "accent":   (123,  45, 139),     # Violet
        "bg_base":  (8,    0,  18),
        "style":    "lissajous",
        "label":    "ODYSSEY",
    },
    "OBLONG": {
        "accent":   (233, 168,  74),     # Amber
        "bg_base":  (20,  12,   0),
        "style":    "freq_bands",
        "label":    "OBLONG",
    },
    "OBESE": {
        "accent":   (255,  20, 147),     # Hot Pink
        "bg_base":  (18,   0,  10),
        "style":    "dense_blocks",
        "label":    "OBESE",
    },
    "OVERBITE": {
        "accent":   (240, 237, 232),     # Fang White
        "bg_base":  (10,   8,   6),
        "style":    "angular_cuts",
        "label":    "OVERBITE",
    },
    # Constellation engines
    "OHM": {
        "accent":   (135, 174, 115),     # Sage
        "bg_base":  (6,   12,   4),
        "style":    "wave_morph",
        "label":    "OHM",
    },
    "ORPHICA": {
        "accent":   (127, 219, 202),     # Siren Seafoam
        "bg_base":  (0,   12,  10),
        "style":    "lissajous",
        "label":    "ORPHICA",
    },
    "OBBLIGATO": {
        "accent":   (255, 138, 122),     # Rascal Coral
        "bg_base":  (18,   6,   4),
        "style":    "freq_bands",
        "label":    "OBBLIGATO",
    },
    "OTTONI": {
        "accent":   (91,  138, 114),     # Patina
        "bg_base":  (4,   10,   6),
        "style":    "freq_bands",
        "label":    "OTTONI",
    },
    "OLE": {
        "accent":   (201,  55, 122),     # Hibiscus
        "bg_base":  (14,   2,   8),
        "style":    "angular_cuts",
        "label":    "OLE",
    },
    # Fallback for unknown engines
    "DEFAULT": {
        "accent":   (233, 196, 106),     # XO Gold
        "bg_base":  (12,  10,   8),
        "style":    "freq_bands",
        "label":    "XO_OX",
    },
}

XO_GOLD = (233, 196, 106)
WARM_WHITE = (248, 246, 243)


# =============================================================================
# BACKGROUND GENERATORS
# =============================================================================

def _make_base(size, bg_base):
    """Create gradient base canvas."""
    arr = np.zeros((size, size, 3), dtype=np.float32)
    r, g, b = [x / 255.0 for x in bg_base]
    # Radial vignette from center
    cx, cy = size / 2, size / 2
    Y, X = np.mgrid[0:size, 0:size]
    dist = np.sqrt((X - cx) ** 2 + (Y - cy) ** 2) / (size * 0.75)
    dist = np.clip(dist, 0, 1)
    brightness = (1.0 - dist * 0.6)
    arr[:, :, 0] = r * brightness
    arr[:, :, 1] = g * brightness
    arr[:, :, 2] = b * brightness
    return arr


def _blend_layer(arr, layer, alpha):
    """Alpha-composite a float layer onto arr."""
    return arr * (1 - alpha) + layer * alpha


def style_transient_spikes(arr, size, accent, rng):
    """ONSET: Sharp vertical transient spikes, percussive geometry."""
    r, g, b = [x / 255.0 for x in accent]
    layer = np.zeros((size, size, 3), dtype=np.float32)

    # 24 vertical bars of varying width and brightness
    n_bars = 24
    for i in range(n_bars):
        x_center = rng.randint(20, size - 20)
        width = rng.randint(1, 6) if i % 4 != 0 else rng.randint(8, 20)
        height_frac = rng.uniform(0.3, 1.0)
        brightness = rng.uniform(0.3, 1.0)
        # Bar starts from bottom (transient impulse feel)
        y_start = int(size * (1.0 - height_frac))
        y_end = size
        x0 = max(0, x_center - width // 2)
        x1 = min(size, x_center + width // 2 + 1)
        # Fade the top
        for y in range(y_start, y_end):
            fade = 1.0 - (y - y_start) / max(1, y_end - y_start - 1) * 0.3
            intensity = brightness * fade
            layer[y, x0:x1, 0] += r * intensity
            layer[y, x0:x1, 1] += g * intensity
            layer[y, x0:x1, 2] += b * intensity

    layer = np.clip(layer, 0, 1)
    return _blend_layer(arr, layer, 0.7)


def style_pixel_grid(arr, size, accent, rng):
    """OVERWORLD: 8-bit pixel grid with scanlines."""
    r, g, b = [x / 255.0 for x in accent]
    layer = np.zeros((size, size, 3), dtype=np.float32)

    pixel_size = 20
    # Fill grid with varying brightness pixels
    for py in range(0, size, pixel_size):
        for px in range(0, size, pixel_size):
            if rng.random() > 0.55:
                brightness = rng.uniform(0.1, 0.8)
                layer[py:py+pixel_size-1, px:px+pixel_size-1, 0] = r * brightness
                layer[py:py+pixel_size-1, px:px+pixel_size-1, 1] = g * brightness
                layer[py:py+pixel_size-1, px:px+pixel_size-1, 2] = b * brightness

    # Horizontal scanlines
    for y in range(0, size, 4):
        layer[y, :, :] *= 0.4

    return _blend_layer(arr, np.clip(layer, 0, 1), 0.8)


def style_angular_cuts(arr, size, accent, rng):
    """SNAP: Sharp angular diagonal cuts across the canvas."""
    r, g, b = [x / 255.0 for x in accent]
    layer = np.zeros((size, size, 3), dtype=np.float32)

    Y, X = np.mgrid[0:size, 0:size]
    # Multiple diagonal bands
    angles = [rng.uniform(30, 60), rng.uniform(120, 150), rng.uniform(-20, 20)]
    for angle in angles:
        rad = math.radians(angle)
        proj = X * math.cos(rad) + Y * math.sin(rad)
        period = rng.randint(200, 500)
        brightness = rng.uniform(0.2, 0.6)
        band = (np.sin(proj / period * math.pi * 2) + 1) / 2 * brightness
        layer[:, :, 0] += r * band
        layer[:, :, 1] += g * band
        layer[:, :, 2] += b * band

    return _blend_layer(arr, np.clip(layer, 0, 1), 0.65)


def style_wave_morph(arr, size, accent, rng):
    """MORPH: Smooth sinusoidal wave blends."""
    r, g, b = [x / 255.0 for x in accent]
    layer = np.zeros((size, size, 3), dtype=np.float32)

    Y, X = np.mgrid[0:size, 0:size]
    # Layered sine waves
    for i in range(5):
        freq_x = rng.uniform(0.5, 3.0) / size
        freq_y = rng.uniform(0.5, 3.0) / size
        phase = rng.uniform(0, math.pi * 2)
        amp = rng.uniform(0.15, 0.4)
        wave = (np.sin(X * freq_x * math.pi * 2 + Y * freq_y * math.pi + phase) + 1) / 2
        layer[:, :, 0] += r * wave * amp
        layer[:, :, 1] += g * wave * amp
        layer[:, :, 2] += b * wave * amp

    return _blend_layer(arr, np.clip(layer, 0, 1), 0.7)


def style_tape_streaks(arr, size, accent, rng):
    """DUB: Diagonal tape echo streak trails."""
    r, g, b = [x / 255.0 for x in accent]
    layer = np.zeros((size, size, 3), dtype=np.float32)

    # 12 diagonal streaks with fade
    for _ in range(12):
        x_start = rng.randint(0, size)
        y_start = rng.randint(0, size // 2)
        length = rng.randint(200, size)
        width = rng.randint(1, 4)
        brightness = rng.uniform(0.2, 0.7)
        # Diagonal angle: mostly horizontal with slight downward slope
        dx = rng.uniform(0.7, 1.0)
        dy = rng.uniform(0.1, 0.5)
        for step in range(length):
            x = int(x_start + step * dx) % size
            y = int(y_start + step * dy) % size
            fade = 1.0 - step / length
            for w in range(-width, width + 1):
                yy = np.clip(y + w, 0, size - 1)
                layer[yy, x, 0] += r * brightness * fade
                layer[yy, x, 1] += g * brightness * fade
                layer[yy, x, 2] += b * brightness * fade

    return _blend_layer(arr, np.clip(layer, 0, 1), 0.65)


def style_lissajous(arr, size, accent, rng):
    """DRIFT: Lissajous figures and psychedelic orbital curves."""
    r, g, b = [x / 255.0 for x in accent]
    layer = np.zeros((size, size, 3), dtype=np.float32)

    cx, cy = size / 2, size / 2
    scale = size * 0.42
    # Multiple Lissajous curves — thickened with a 2px radius brush
    ratios = [(1, 2), (2, 3), (3, 4), (1, 3), (3, 5)]
    for (a_freq, b_freq) in ratios:
        phase = rng.uniform(0, math.pi)
        brightness = rng.uniform(0.55, 0.9)
        n_pts = 12000
        for i in range(n_pts):
            t = i / n_pts * math.pi * 2
            x = int(cx + scale * math.sin(a_freq * t + phase))
            y = int(cy + scale * math.sin(b_freq * t))
            fade = (math.sin(t * 3) + 1) / 2 * 0.7 + 0.3
            intensity = brightness * fade
            # 2-pixel thick brush
            for dy in range(-1, 2):
                for dx in range(-1, 2):
                    px, py = x + dx, y + dy
                    if 0 <= px < size and 0 <= py < size:
                        layer[py, px, 0] = min(1.0, layer[py, px, 0] + r * intensity * 0.5)
                        layer[py, px, 1] = min(1.0, layer[py, px, 1] + g * intensity * 0.5)
                        layer[py, px, 2] = min(1.0, layer[py, px, 2] + b * intensity * 0.5)

    return _blend_layer(arr, np.clip(layer, 0, 1), 0.85)


def style_freq_bands(arr, size, accent, rng):
    """BOB: Horizontal frequency spectrum bands, warm analog feel."""
    r, g, b = [x / 255.0 for x in accent]
    layer = np.zeros((size, size, 3), dtype=np.float32)

    Y = np.arange(size).reshape(-1, 1)
    # Generate spectrum-like horizontal bands
    n_bands = 40
    for i in range(n_bands):
        y_center = rng.randint(0, size)
        band_height = rng.randint(5, 40)
        brightness = rng.uniform(0.1, 0.6)
        y0 = max(0, y_center - band_height // 2)
        y1 = min(size, y_center + band_height // 2)
        # Amplitude envelope across x
        x_pts = np.arange(size)
        envelope = rng.uniform(0.3, 1.0, size)
        # Smooth it
        kernel_size = 40
        envelope = np.convolve(envelope, np.ones(kernel_size)/kernel_size, mode='same')
        envelope = envelope / envelope.max() * brightness
        layer[y0:y1, :, 0] += r * envelope
        layer[y0:y1, :, 1] += g * envelope
        layer[y0:y1, :, 2] += b * envelope

    return _blend_layer(arr, np.clip(layer, 0, 1), 0.65)


def style_dense_blocks(arr, size, accent, rng):
    """FAT: Dense spectral blocks, thick horizontal masses."""
    r, g, b = [x / 255.0 for x in accent]
    layer = np.zeros((size, size, 3), dtype=np.float32)

    # Large overlapping rectangular blocks
    for _ in range(30):
        x0 = rng.randint(0, size // 2)
        y0 = rng.randint(0, size)
        w = rng.randint(size // 4, size)
        h = rng.randint(10, 80)
        brightness = rng.uniform(0.1, 0.5)
        x1 = min(size, x0 + w)
        y1 = min(size, y0 + h)
        layer[y0:y1, x0:x1, 0] += r * brightness
        layer[y0:y1, x0:x1, 1] += g * brightness
        layer[y0:y1, x0:x1, 2] += b * brightness

    return _blend_layer(arr, np.clip(layer, 0, 1), 0.7)


def style_particle_scatter(arr, size, accent, rng):
    """OPAL: Granular particle scatter field."""
    r, g, b = [x / 255.0 for x in accent]
    layer = np.zeros((size, size, 3), dtype=np.float32)

    cx, cy = size / 2, size / 2
    # 3000 particles scattered in a cloud
    n_particles = 3000
    for _ in range(n_particles):
        # Gaussian cluster around center with some outliers
        if rng.random() > 0.2:
            x = int(rng.gauss(cx, size * 0.25)) % size
            y = int(rng.gauss(cy, size * 0.25)) % size
        else:
            x = rng.randint(0, size)
            y = rng.randint(0, size)
        radius = rng.randint(1, 4)
        brightness = rng.uniform(0.2, 0.9)
        for dy in range(-radius, radius + 1):
            for dx in range(-radius, radius + 1):
                if dx*dx + dy*dy <= radius*radius:
                    px, py = (x+dx) % size, (y+dy) % size
                    layer[py, px, 0] = min(1.0, layer[py, px, 0] + r * brightness)
                    layer[py, px, 1] = min(1.0, layer[py, px, 1] + g * brightness)
                    layer[py, px, 2] = min(1.0, layer[py, px, 2] + b * brightness)

    return _blend_layer(arr, np.clip(layer, 0, 1), 0.75)


STYLE_FUNCS = {
    "transient_spikes":  style_transient_spikes,
    "pixel_grid":        style_pixel_grid,
    "angular_cuts":      style_angular_cuts,
    "wave_morph":        style_wave_morph,
    "tape_streaks":      style_tape_streaks,
    "lissajous":         style_lissajous,
    "freq_bands":        style_freq_bands,
    "dense_blocks":      style_dense_blocks,
    "particle_scatter":  style_particle_scatter,
}


# =============================================================================
# TEXT OVERLAY
# =============================================================================

def _arr_to_image(arr):
    """Convert float32 numpy array [0,1] to PIL Image."""
    return Image.fromarray((np.clip(arr, 0, 1) * 255).astype(np.uint8))


def _hex_to_rgb(hex_color):
    h = hex_color.lstrip("#")
    return tuple(int(h[i:i+2], 16) for i in (0, 2, 4))


def _add_text_overlay(img, engine_def, pack_name, preset_count, version):
    """Draw pack name, engine label, preset count, and XO_OX watermark."""
    draw = ImageDraw.Draw(img)
    size = img.width
    accent = engine_def["accent"]
    label = engine_def["label"]

    # Try to load system fonts, fall back to PIL default
    def try_font(size_pt):
        font_candidates = [
            "/System/Library/Fonts/Supplemental/Arial Bold.ttf",
            "/System/Library/Fonts/Helvetica.ttc",
            "/Library/Fonts/Arial Bold.ttf",
            "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf",
        ]
        for path in font_candidates:
            if os.path.exists(path):
                try:
                    return ImageFont.truetype(path, size_pt)
                except Exception:
                    pass
        return ImageFont.load_default()

    font_large  = try_font(int(size * 0.055))   # Pack title
    font_medium = try_font(int(size * 0.035))   # Engine label
    font_small  = try_font(int(size * 0.022))   # Metadata
    font_wm     = try_font(int(size * 0.028))   # Watermark

    pad = int(size * 0.04)

    # --- Engine accent bar (left edge) ---
    bar_width = int(size * 0.012)
    draw.rectangle([(pad, pad), (pad + bar_width, size - pad)],
                   fill=accent + (180,) if len(accent) == 3 else accent)

    # --- Pack title (upper left, below bar) ---
    text_x = pad + bar_width + int(size * 0.02)
    text_y = int(size * 0.06)
    # Drop shadow
    draw.text((text_x + 3, text_y + 3), pack_name, font=font_large,
              fill=(0, 0, 0, 140))
    draw.text((text_x, text_y), pack_name, font=font_large, fill=WARM_WHITE)

    # --- Engine label pill ---
    pill_y = text_y + int(size * 0.075)
    pill_pad_x = int(size * 0.015)
    pill_pad_y = int(size * 0.008)
    bbox = draw.textbbox((text_x, pill_y), label, font=font_medium)
    rect = (bbox[0] - pill_pad_x, bbox[1] - pill_pad_y,
            bbox[2] + pill_pad_x, bbox[3] + pill_pad_y)
    draw.rounded_rectangle(rect, radius=int(size * 0.01), fill=accent)
    draw.text((text_x, pill_y), label, font=font_medium, fill=(0, 0, 0))

    # --- Preset count + version (bottom left) ---
    meta_y = size - pad - int(size * 0.06)
    count_text = f"{preset_count} PRESETS"
    draw.text((text_x + 1, meta_y + 1), count_text, font=font_small, fill=(0, 0, 0, 120))
    draw.text((text_x, meta_y), count_text, font=font_small, fill=XO_GOLD)

    ver_text = f"v{version}"
    draw.text((text_x + 1, meta_y + int(size * 0.03) + 1), ver_text, font=font_small,
              fill=(0, 0, 0, 80))
    draw.text((text_x, meta_y + int(size * 0.03)), ver_text, font=font_small,
              fill=(160, 155, 150))

    # --- XO_OX watermark (bottom right, subtle) ---
    wm_text = "XO_OX"
    wm_bbox = draw.textbbox((0, 0), wm_text, font=font_wm)
    wm_w = wm_bbox[2] - wm_bbox[0]
    wm_h = wm_bbox[3] - wm_bbox[1]
    wm_x = size - pad - wm_w
    wm_y = size - pad - wm_h
    draw.text((wm_x, wm_y), wm_text, font=font_wm, fill=(255, 255, 255, 55))

    return img


# =============================================================================
# PUBLIC API
# =============================================================================

def generate_cover(
    engine: str,
    pack_name: str,
    output_dir: str,
    preset_count: int = 0,
    version: str = "1.0",
    seed: int = None,
) -> dict:
    """
    Generate cover art for an XPN expansion pack.

    Args:
        engine:        Engine name (e.g., "ONSET", "FAT"). Case-insensitive.
        pack_name:     Display name for the pack (e.g., "808 Reborn Collection").
        output_dir:    Directory to write artwork files.
        preset_count:  Number of presets in the pack (displayed on cover).
        version:       Version string (e.g., "1.0").
        seed:          RNG seed for reproducible art. None = random.

    Returns:
        dict with keys "cover_1000" and "cover_2000" pointing to output paths.
    """
    if not PILLOW_AVAILABLE:
        raise RuntimeError("Pillow and numpy required: pip install Pillow numpy")

    engine_key = engine.upper()
    eng = ENGINE_DEFS.get(engine_key, ENGINE_DEFS["DEFAULT"])
    rng = random.Random(seed)
    np_rng = np.random.RandomState(seed if seed is not None else rng.randint(0, 2**31))

    SIZE = 2000

    # 1. Build background
    arr = _make_base(SIZE, eng["bg_base"])

    # 2. Apply engine-specific style
    style_func = STYLE_FUNCS.get(eng["style"], style_freq_bands)

    # Pass np_rng wrapped as a compatible random object
    class NpRngAdapter:
        def __init__(self, nprng):
            self._r = nprng
        def randint(self, a, b): return int(self._r.randint(a, b + 1))
        def uniform(self, a, b): return float(self._r.uniform(a, b))
        def random(self): return float(self._r.uniform(0, 1))
        def gauss(self, mu, sigma): return float(self._r.normal(mu, sigma))

    arr = style_func(arr, SIZE, eng["accent"], NpRngAdapter(np_rng))

    # 3. Convert to PIL, apply subtle blur for anti-aliasing
    img = _arr_to_image(arr)
    img = img.filter(ImageFilter.GaussianBlur(radius=0.8))

    # 4. Add text overlay
    img = img.convert("RGBA")
    img = _add_text_overlay(img, eng, pack_name, preset_count, version)
    img = img.convert("RGB")

    # 5. Save outputs
    out_path = Path(output_dir)
    out_path.mkdir(parents=True, exist_ok=True)

    path_2000 = out_path / "artwork_2000.png"
    path_1000 = out_path / "artwork.png"

    img.save(str(path_2000), "PNG", optimize=True)

    # Downscale to 1000×1000 with Lanczos
    img_1000 = img.resize((1000, 1000), Image.LANCZOS)
    img_1000.save(str(path_1000), "PNG", optimize=True)

    print(f"  Cover art: {path_1000.name} + {path_2000.name}")
    return {"cover_1000": str(path_1000), "cover_2000": str(path_2000)}


# =============================================================================
# CLI
# =============================================================================

if __name__ == "__main__":
    import sys
    import argparse

    parser = argparse.ArgumentParser(description="Generate XPN cover art")
    parser.add_argument("--engine",    required=True, help="Engine name (ONSET, FAT, etc.)")
    parser.add_argument("--pack-name", required=True, help="Pack display name")
    parser.add_argument("--output",    required=True, help="Output directory")
    parser.add_argument("--count",     type=int, default=0,  help="Preset count")
    parser.add_argument("--version",   default="1.0",         help="Version string")
    parser.add_argument("--seed",      type=int, default=None, help="RNG seed")
    args = parser.parse_args()

    if not PILLOW_AVAILABLE:
        print("ERROR: pip install Pillow numpy")
        sys.exit(1)

    result = generate_cover(
        engine=args.engine,
        pack_name=args.pack_name,
        output_dir=args.output,
        preset_count=args.count,
        version=args.version,
        seed=args.seed,
    )
    print(f"Done: {result}")
