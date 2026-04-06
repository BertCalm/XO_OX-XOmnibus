#!/usr/bin/env python3
"""
XPN Pack Thumbnail Generator — XO_OX Tools
Generates a simple programmatic cover art PNG for a .xpn pack.

Pure Python stdlib only — no Pillow, no external deps.
Uses struct + zlib to write raw PNG bytes.

Usage:
    python xpn_pack_thumbnail_generator.py <pack.xpn> [--output cover.png] [--size 400]

Output: 400x400 PNG with:
  - Solid background (engine accent color, darkened 40%)
  - Pack name in pixel-art text (centered)
  - Engine name in smaller text below
  - XO_OX logo mark (two interlocked circles, top-right corner)
  - DNA bar visualization at bottom (6 bars)
"""

import argparse
import json
import struct
import sys
import zipfile
import zlib
from pathlib import Path


# ---------------------------------------------------------------------------
# Engine accent color lookup (34 registered engines + 4 V1 concept engines)
# ---------------------------------------------------------------------------
ENGINE_COLORS = {
    "ODDFELIX":  (0x00, 0xA6, 0xD6),  # Neon Tetra Blue
    "ODDOSCAR":  (0xE8, 0x83, 0x9B),  # Axolotl Gill Pink
    "OVERDUB":   (0x6B, 0x7B, 0x3A),  # Olive
    "ODYSSEY":   (0x7B, 0x2D, 0x8B),  # Violet
    "OBLONG":    (0xE9, 0xA8, 0x4A),  # Amber
    "OBESE":     (0xFF, 0x14, 0x93),  # Hot Pink
    "ONSET":     (0x00, 0x66, 0xFF),  # Electric Blue
    "OVERWORLD": (0x39, 0xFF, 0x14),  # Neon Green
    "OPAL":      (0xA7, 0x8B, 0xFA),  # Lavender
    "ORGANON":   (0x00, 0xCE, 0xD1),  # Bioluminescent Cyan
    "OUROBOROS": (0xFF, 0x2D, 0x2D),  # Strange Attractor Red
    "OBSIDIAN":  (0xE8, 0xE0, 0xD8),  # Crystal White
    "ORIGAMI":   (0xE6, 0x39, 0x46),  # Vermillion Fold
    "ORACLE":    (0x4B, 0x00, 0x82),  # Prophecy Indigo
    "OBSCURA":   (0x8A, 0x9B, 0xA8),  # Daguerreotype Silver
    "OCEANIC":   (0x00, 0xB4, 0xA0),  # Phosphorescent Teal
    "OCELOT":    (0xC5, 0x83, 0x2B),  # Ocelot Tawny
    "OVERBITE":  (0xF0, 0xED, 0xE8),  # Fang White
    "ORBITAL":   (0xFF, 0x6B, 0x6B),  # Warm Red
    "OPTIC":     (0x00, 0xFF, 0x41),  # Phosphor Green
    "OBLIQUE":   (0xBF, 0x40, 0xFF),  # Prism Violet
    "OSPREY":    (0x1B, 0x4F, 0x8A),  # Azulejo Blue
    "OSTERIA":   (0x72, 0x2F, 0x37),  # Porto Wine
    "OWLFISH":   (0xB8, 0x86, 0x0B),  # Abyssal Gold
    "OHM":       (0x87, 0xAE, 0x73),  # Sage
    "ORPHICA":   (0x7F, 0xDB, 0xCA),  # Siren Seafoam
    "OBBLIGATO": (0xFF, 0x8A, 0x7A),  # Rascal Coral
    "OTTONI":    (0x5B, 0x8A, 0x72),  # Patina
    "OLE":       (0xC9, 0x37, 0x7A),  # Hibiscus
    "OMBRE":     (0x7B, 0x6B, 0x8A),  # Shadow Mauve
    "ORCA":      (0x1B, 0x28, 0x38),  # Deep Ocean
    "OCTOPUS":   (0xE0, 0x40, 0xFB),  # Chromatophore Magenta
    "OVERLAP":   (0x00, 0xFF, 0xB4),  # Lion's Mane Green
    "OUTWIT":    (0xCC, 0x66, 0x00),  # Octopus Amber
    # V1 concept engines
    "OSTINATO":  (0xE8, 0x70, 0x1A),  # Firelight Orange
    "OPENSKY":   (0xFF, 0x8C, 0x00),  # Sunburst
    "OCEANDEEP": (0x2D, 0x0A, 0x4E),  # Trench Violet
    "OUIE":      (0x70, 0x80, 0x90),  # Hammerhead Steel
}

DEFAULT_COLOR = (0x44, 0x44, 0x44)  # fallback dark gray

XO_GOLD = (0xE9, 0xC4, 0x6A)


# ---------------------------------------------------------------------------
# 5x7 pixel-art bitmap font — uppercase A-Z, 0-9, space, hyphen, underscore
# Each character: list of 7 rows, each row is a 5-bit integer (MSB = leftmost pixel)
# ---------------------------------------------------------------------------
FONT_5X7 = {
    ' ': [0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b00000],
    '-': [0b00000, 0b00000, 0b00000, 0b11111, 0b00000, 0b00000, 0b00000],
    '_': [0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b11111],
    '.': [0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b01100, 0b01100],
    'A': [0b01110, 0b10001, 0b10001, 0b11111, 0b10001, 0b10001, 0b10001],
    'B': [0b11110, 0b10001, 0b10001, 0b11110, 0b10001, 0b10001, 0b11110],
    'C': [0b01111, 0b10000, 0b10000, 0b10000, 0b10000, 0b10000, 0b01111],
    'D': [0b11110, 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b11110],
    'E': [0b11111, 0b10000, 0b10000, 0b11110, 0b10000, 0b10000, 0b11111],
    'F': [0b11111, 0b10000, 0b10000, 0b11110, 0b10000, 0b10000, 0b10000],
    'G': [0b01111, 0b10000, 0b10000, 0b10111, 0b10001, 0b10001, 0b01111],
    'H': [0b10001, 0b10001, 0b10001, 0b11111, 0b10001, 0b10001, 0b10001],
    'I': [0b11111, 0b00100, 0b00100, 0b00100, 0b00100, 0b00100, 0b11111],
    'J': [0b00111, 0b00001, 0b00001, 0b00001, 0b10001, 0b10001, 0b01110],
    'K': [0b10001, 0b10010, 0b10100, 0b11000, 0b10100, 0b10010, 0b10001],
    'L': [0b10000, 0b10000, 0b10000, 0b10000, 0b10000, 0b10000, 0b11111],
    'M': [0b10001, 0b11011, 0b10101, 0b10001, 0b10001, 0b10001, 0b10001],
    'N': [0b10001, 0b11001, 0b10101, 0b10011, 0b10001, 0b10001, 0b10001],
    'O': [0b01110, 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b01110],
    'P': [0b11110, 0b10001, 0b10001, 0b11110, 0b10000, 0b10000, 0b10000],
    'Q': [0b01110, 0b10001, 0b10001, 0b10001, 0b10101, 0b10010, 0b01101],
    'R': [0b11110, 0b10001, 0b10001, 0b11110, 0b10100, 0b10010, 0b10001],
    'S': [0b01111, 0b10000, 0b10000, 0b01110, 0b00001, 0b00001, 0b11110],
    'T': [0b11111, 0b00100, 0b00100, 0b00100, 0b00100, 0b00100, 0b00100],
    'U': [0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b01110],
    'V': [0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b01010, 0b00100],
    'W': [0b10001, 0b10001, 0b10001, 0b10001, 0b10101, 0b11011, 0b10001],
    'X': [0b10001, 0b10001, 0b01010, 0b00100, 0b01010, 0b10001, 0b10001],
    'Y': [0b10001, 0b10001, 0b01010, 0b00100, 0b00100, 0b00100, 0b00100],
    'Z': [0b11111, 0b00001, 0b00010, 0b00100, 0b01000, 0b10000, 0b11111],
    '0': [0b01110, 0b10011, 0b10101, 0b10101, 0b11001, 0b10001, 0b01110],
    '1': [0b00100, 0b01100, 0b00100, 0b00100, 0b00100, 0b00100, 0b11111],
    '2': [0b01110, 0b10001, 0b00001, 0b00110, 0b01000, 0b10000, 0b11111],
    '3': [0b11111, 0b00010, 0b00100, 0b00110, 0b00001, 0b10001, 0b01110],
    '4': [0b00010, 0b00110, 0b01010, 0b10010, 0b11111, 0b00010, 0b00010],
    '5': [0b11111, 0b10000, 0b11110, 0b00001, 0b00001, 0b10001, 0b01110],
    '6': [0b01110, 0b10000, 0b10000, 0b11110, 0b10001, 0b10001, 0b01110],
    '7': [0b11111, 0b00001, 0b00010, 0b00100, 0b01000, 0b01000, 0b01000],
    '8': [0b01110, 0b10001, 0b10001, 0b01110, 0b10001, 0b10001, 0b01110],
    '9': [0b01110, 0b10001, 0b10001, 0b01111, 0b00001, 0b00001, 0b01110],
}


# ---------------------------------------------------------------------------
# PNG helpers
# ---------------------------------------------------------------------------

def _png_chunk(chunk_type: bytes, data: bytes) -> bytes:
    """Encode a single PNG chunk: length + type + data + CRC."""
    length = struct.pack('>I', len(data))
    crc = struct.pack('>I', zlib.crc32(chunk_type + data) & 0xFFFFFFFF)
    return length + chunk_type + data + crc


def _build_png(pixels: list, width: int, height: int) -> bytes:
    """
    Serialize a 2D list of (R,G,B) tuples to PNG bytes.
    pixels[y][x] = (r, g, b)
    """
    # PNG signature
    sig = b'\x89PNG\r\n\x1a\n'

    # IHDR chunk
    ihdr_data = struct.pack('>IIBBBBB', width, height,
                            8,    # bit depth
                            2,    # color type: RGB
                            0,    # compression
                            0,    # filter
                            0)    # interlace
    ihdr = _png_chunk(b'IHDR', ihdr_data)

    # IDAT chunk: filter byte 0x00 per scanline, then zlib compress
    raw_rows = []
    for y in range(height):
        row_bytes = bytearray([0x00])  # filter type None
        for x in range(width):
            r, g, b = pixels[y][x]
            row_bytes += bytes([r, g, b])
        raw_rows.append(bytes(row_bytes))

    raw_data = b''.join(raw_rows)
    compressed = zlib.compress(raw_data, 9)
    idat = _png_chunk(b'IDAT', compressed)

    # IEND chunk
    iend = _png_chunk(b'IEND', b'')

    return sig + ihdr + idat + iend


# ---------------------------------------------------------------------------
# Pixel operations
# ---------------------------------------------------------------------------

def _darken(color: tuple, factor: float = 0.6) -> tuple:
    """Darken an RGB tuple by multiplying each channel by factor."""
    return tuple(max(0, int(c * factor)) for c in color)


def _blend(c1: tuple, c2: tuple, t: float) -> tuple:
    """Linear blend between two RGB tuples. t=0 → c1, t=1 → c2."""
    return tuple(int(c1[i] + (c2[i] - c1[i]) * t) for i in range(3))


def _fill(pixels: list, color: tuple):
    """Fill entire pixel buffer with a solid color."""
    for row in pixels:
        for x in range(len(row)):
            row[x] = color


def _set_pixel(pixels: list, x: int, y: int, color: tuple, width: int, height: int):
    if 0 <= x < width and 0 <= y < height:
        pixels[y][x] = color


def _draw_circle_outline(pixels: list, cx: int, cy: int, r: int,
                         color: tuple, width: int, height: int, thickness: int = 2):
    """Draw a circle outline using midpoint circle algorithm."""
    for t in range(thickness):
        radius = r - t
        if radius <= 0:
            break
        x = radius
        y = 0
        err = 0
        while x >= y:
            for dx, dy in [(x, y), (y, x), (-y, x), (-x, y),
                           (-x, -y), (-y, -x), (y, -x), (x, -y)]:
                _set_pixel(pixels, cx + dx, cy + dy, color, width, height)
            y += 1
            if err <= 0:
                err += 2 * y + 1
            if err > 0:
                x -= 1
                err -= 2 * x + 1


def _draw_rect(pixels: list, x0: int, y0: int, x1: int, y1: int,
               color: tuple, width: int, height: int):
    """Fill a rectangle."""
    for y in range(max(0, y0), min(height, y1)):
        for x in range(max(0, x0), min(width, x1)):
            pixels[y][x] = color


def _char_width(scale: int) -> int:
    return 5 * scale + scale  # 5 pixels + 1 gap


def _draw_char(pixels: list, ch: str, ox: int, oy: int, color: tuple,
               width: int, height: int, scale: int = 1):
    """Draw a single 5x7 pixel-art character at (ox, oy) with given scale."""
    rows = FONT_5X7.get(ch.upper(), FONT_5X7.get(' '))
    for row_idx, row_bits in enumerate(rows):
        for col_idx in range(5):
            if row_bits & (1 << (4 - col_idx)):
                for sy in range(scale):
                    for sx in range(scale):
                        _set_pixel(pixels,
                                   ox + col_idx * scale + sx,
                                   oy + row_idx * scale + sy,
                                   color, width, height)


def _text_pixel_width(text: str, scale: int) -> int:
    return len(text) * _char_width(scale) - scale  # remove trailing gap


def _draw_text_centered(pixels: list, text: str, cy: int, color: tuple,
                        img_width: int, img_height: int, scale: int = 1):
    """Draw text centered horizontally at vertical position cy."""
    # Truncate if too wide
    max_chars = (img_width - 20) // _char_width(scale)
    if len(text) > max_chars:
        text = text[:max_chars - 1] + '.'

    tw = _text_pixel_width(text, scale)
    ox = (img_width - tw) // 2
    for i, ch in enumerate(text):
        _draw_char(pixels, ch, ox + i * _char_width(scale), cy,
                   color, img_width, img_height, scale)


# ---------------------------------------------------------------------------
# XO_OX logo mark — two interlocked circles in top-right corner
# ---------------------------------------------------------------------------

def _draw_xo_logo(pixels: list, img_width: int, img_height: int,
                  color: tuple, margin: int = 12, r: int = 14, gap: int = 8):
    """Two interlocked circles (XO brand mark) in top-right corner."""
    # Right circle center
    cx2 = img_width - margin - r
    cy = margin + r
    # Left circle center, offset left by gap
    cx1 = cx2 - gap
    _draw_circle_outline(pixels, cx1, cy, r, color, img_width, img_height, thickness=2)
    _draw_circle_outline(pixels, cx2, cy, r, color, img_width, img_height, thickness=2)


# ---------------------------------------------------------------------------
# DNA bar chart at bottom
# ---------------------------------------------------------------------------

DNA_LABELS = ['WAR', 'SPA', 'MEC', 'ORG', 'DEN', 'MOV']
DNA_COLORS = [
    (0xFF, 0x6B, 0x6B),  # Warmth — warm red
    (0x64, 0xB5, 0xF6),  # Space — sky blue
    (0xA5, 0xD6, 0xA7),  # Mechanical — soft green
    (0xFF, 0xCC, 0x80),  # Organic — amber
    (0xCE, 0x93, 0xD8),  # Density — lilac
    (0x80, 0xDE, 0xEA),  # Movement — teal
]


def _draw_dna_bars(pixels: list, dna: dict, img_width: int, img_height: int,
                   bar_area_height: int = 60, bottom_margin: int = 10):
    """
    Draw 6 DNA dimension bars at the bottom of the image.
    dna: dict with keys Warmth/Space/Mechanical/Organic/Density/Movement (0.0-1.0)
    """
    dna_keys = ['Warmth', 'Space', 'Mechanical', 'Organic', 'Density', 'Movement']

    total_bars = 6
    padding = 8
    available_w = img_width - 2 * padding
    bar_width = available_w // total_bars
    bar_gap = 4

    base_y = img_height - bottom_margin
    max_bar_h = bar_area_height - 14  # leave room for label

    # Background strip (semi-dark)
    _draw_rect(pixels,
               0, img_height - bar_area_height - bottom_margin,
               img_width, img_height,
               (0, 0, 0), img_width, img_height)

    for i, key in enumerate(dna_keys):
        value = float(dna.get(key, 0.5))
        value = max(0.0, min(1.0, value))
        bar_h = max(2, int(value * max_bar_h))

        x0 = padding + i * bar_width + bar_gap // 2
        x1 = x0 + bar_width - bar_gap
        y0 = base_y - bar_h - 12  # 12px for label
        y1 = base_y - 12

        bar_color = DNA_COLORS[i]
        _draw_rect(pixels, x0, y0, x1, y1, bar_color, img_width, img_height)

        # 3-char label below bar (scale=1)
        label = DNA_LABELS[i]
        lx = x0 + (bar_width - bar_gap - _text_pixel_width(label, 1)) // 2
        ly = base_y - 10
        for j, ch in enumerate(label):
            _draw_char(pixels, ch, lx + j * _char_width(1), ly,
                       (180, 180, 180), img_width, img_height, scale=1)


# ---------------------------------------------------------------------------
# Main generation
# ---------------------------------------------------------------------------

def _make_pixels(width: int, height: int, color: tuple) -> list:
    return [[color] * width for _ in range(height)]


def generate_thumbnail(engine: str, pack_name: str, sonic_dna: dict,
                       size: int = 400) -> bytes:
    """
    Generate a thumbnail PNG and return raw bytes.

    Parameters
    ----------
    engine    : Engine short name, e.g. 'ONSET'
    pack_name : Human-readable pack name
    sonic_dna : Dict of 6 DNA dimension values (0.0–1.0)
    size      : Image dimension in pixels (square)
    """
    W = H = size

    # Accent color → darken 40% for background
    accent = ENGINE_COLORS.get(engine.upper(), DEFAULT_COLOR)
    bg = _darken(accent, 0.35)
    # Slightly lighter mid section
    mid = _darken(accent, 0.50)

    pixels = _make_pixels(W, H, bg)

    # Subtle gradient: blend bg→mid top-to-bottom across upper 70%
    grad_end = int(H * 0.72)
    for y in range(grad_end):
        t = y / grad_end
        c = _blend(bg, mid, t * 0.4)
        for x in range(W):
            pixels[y][x] = c

    # Horizontal accent stripe near top
    stripe_color = _blend(accent, (255, 255, 255), 0.15)
    _draw_rect(pixels, 0, 0, W, 4, stripe_color, W, H)

    # XO_OX logo mark — white circles, top-right
    logo_r = max(10, size // 28)
    logo_gap = max(6, size // 50)
    logo_margin = max(10, size // 35)
    _draw_xo_logo(pixels, W, H, (255, 255, 255), margin=logo_margin,
                  r=logo_r, gap=logo_gap)

    # Pack name — large pixel text, centered ~35% from top
    scale_large = max(2, size // 140)
    text_y = int(H * 0.30)
    # Use accent color lightened for text
    text_color = _blend(accent, (255, 255, 255), 0.85)
    _draw_text_centered(pixels, pack_name.upper(), text_y, text_color, W, H,
                        scale=scale_large)

    # Engine name — smaller, below pack name
    scale_small = max(1, size // 200)
    engine_y = text_y + 7 * scale_large + scale_large * 3 + 8
    engine_color = _blend(accent, (255, 255, 255), 0.55)
    _draw_text_centered(pixels, engine.upper(), engine_y, engine_color, W, H,
                        scale=scale_small)

    # Thin separator line
    sep_y = engine_y + 7 * scale_small + 8
    sep_color = _blend(accent, (255, 255, 255), 0.25)
    _draw_rect(pixels, W // 8, sep_y, W - W // 8, sep_y + 1, sep_color, W, H)

    # DNA bars at bottom
    _draw_dna_bars(pixels, sonic_dna, W, H,
                   bar_area_height=max(50, size // 7),
                   bottom_margin=max(6, size // 60))

    return _build_png(pixels, W, H)


# ---------------------------------------------------------------------------
# XPN reading helpers
# ---------------------------------------------------------------------------

def _read_xpn(xpn_path: str):
    """
    Read a .xpn ZIP archive and extract:
      - engine name (from expansion.json or bundle_manifest.json)
      - pack name
      - sonic_dna dict
    Returns (engine, pack_name, sonic_dna).
    """
    engine = "UNKNOWN"
    pack_name = "UNKNOWN PACK"
    sonic_dna = {
        "Warmth": 0.5, "Space": 0.5, "Mechanical": 0.5,
        "Organic": 0.5, "Density": 0.5, "Movement": 0.5,
    }

    with zipfile.ZipFile(xpn_path, 'r') as zf:
        names = zf.namelist()

        # Try expansion.json first
        expansion_candidates = [n for n in names if n.endswith('expansion.json')]
        for candidate in expansion_candidates:
            try:
                data = json.loads(zf.read(candidate))
                pack_name = data.get('name', data.get('pack_name', pack_name))
                engine = data.get('engine', data.get('Engine', engine))
                if 'sonic_dna' in data:
                    sonic_dna.update(data['sonic_dna'])
                break
            except Exception as exc:
                print(f"[WARN] Reading expansion.json {candidate} for thumbnail metadata: {exc}", file=sys.stderr)

        # Try bundle_manifest.json
        manifest_candidates = [n for n in names if n.endswith('bundle_manifest.json')]
        for candidate in manifest_candidates:
            try:
                data = json.loads(zf.read(candidate))
                pack_name = data.get('name', data.get('pack_name', pack_name))
                engine = data.get('engine', data.get('Engine', engine))
                if 'sonic_dna' in data:
                    sonic_dna.update(data['sonic_dna'])
                break
            except Exception as exc:
                print(f"[WARN] Reading bundle_manifest.json {candidate} for thumbnail metadata: {exc}", file=sys.stderr)

        # Fallback: look for any .json with engine/name keys
        if engine == "UNKNOWN":
            for name in names:
                if name.endswith('.json'):
                    try:
                        data = json.loads(zf.read(name))
                        if isinstance(data, dict):
                            if 'engine' in data:
                                engine = data['engine']
                            if 'name' in data and pack_name == "UNKNOWN PACK":
                                pack_name = data['name']
                            if 'sonic_dna' in data:
                                sonic_dna.update(data['sonic_dna'])
                    except Exception as exc:
                        print(f"[WARN] Reading fallback JSON {name} for thumbnail metadata: {exc}", file=sys.stderr)

    return engine, pack_name, sonic_dna


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def main():
    parser = argparse.ArgumentParser(
        description='Generate a programmatic cover art PNG for a .xpn pack (no external deps).'
    )
    parser.add_argument('xpn', nargs='?', help='Path to .xpn pack file')
    parser.add_argument('--output', '-o', default=None,
                        help='Output PNG path (default: <pack>.thumb.png)')
    parser.add_argument('--size', '-s', type=int, default=400,
                        help='Image size in pixels (square, default: 400)')
    parser.add_argument('--engine', '-e', default=None,
                        help='Override engine name')
    parser.add_argument('--name', '-n', default=None,
                        help='Override pack name')
    parser.add_argument('--list-engines', action='store_true',
                        help='List all known engines and their accent colors')
    args = parser.parse_args()

    if args.list_engines:
        print("Known engines and accent colors:")
        for eng, (r, g, b) in sorted(ENGINE_COLORS.items()):
            print(f"  {eng:<12} #{r:02X}{g:02X}{b:02X}")
        return

    if not args.xpn:
        parser.print_help()
        return

    xpn_path = Path(args.xpn)
    if not xpn_path.exists():
        print(f"Error: file not found: {xpn_path}")
        return

    print(f"Reading {xpn_path.name} ...")
    engine, pack_name, sonic_dna = _read_xpn(str(xpn_path))

    # CLI overrides
    if args.engine:
        engine = args.engine
    if args.name:
        pack_name = args.name

    print(f"  Engine   : {engine}")
    print(f"  Pack     : {pack_name}")
    print(f"  Sonic DNA: {sonic_dna}")

    output = args.output or str(xpn_path.with_suffix('.thumb.png'))

    print(f"Generating {args.size}x{args.size} thumbnail ...")
    png_bytes = generate_thumbnail(engine, pack_name, sonic_dna, size=args.size)

    with open(output, 'wb') as f:
        f.write(png_bytes)

    kb = len(png_bytes) / 1024
    print(f"Saved: {output}  ({kb:.1f} KB)")


if __name__ == '__main__':
    main()
