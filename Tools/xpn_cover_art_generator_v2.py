#!/usr/bin/env python3
"""
XPN Cover Art Generator v2 — XO_OX Tools
Generates Gallery Model cover art PNG for a .xpn pack.

Pure Python stdlib only — no Pillow, no external deps.
Uses struct + zlib to write raw PNG bytes.

Visual design: flag layout — warm white dominant, engine accent as bold left block.
Matches the XO_OX Gallery Model UI aesthetic (light mode, editorial, confident).

Usage:
    python xpn_cover_art_generator_v2.py <pack.xpn> [--output cover.png] [--size 400|800]
                                          [--engine OVERRIDE] [--name OVERRIDE]

Output: 400x400 PNG with:
  - Warm white background (#F8F6F3), accent color fills left 40% (flag layout)
  - Pack name in large pixel-art text, right zone, dark color
  - Engine name in smaller text below pack name, in accent color
  - DNA dots strip at bottom: 6 dots filled proportionally, white on accent background
  - XO_OX wordmark (interlocked X+O letters) bottom-right of white zone, small, dark
"""

import argparse
import json
import struct
import sys
import zipfile
import zlib
from pathlib import Path


# ---------------------------------------------------------------------------
# Engine accent color lookup (34 registered engines + 4 concept engines)
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
    # Concept engines
    "OSTINATO":  (0xE8, 0x70, 0x1A),  # Firelight Orange
    "OPENSKY":   (0xFF, 0x8C, 0x00),  # Sunburst
    "OCEANDEEP": (0x2D, 0x0A, 0x4E),  # Trench Violet
    "OUIE":      (0x70, 0x80, 0x90),  # Hammerhead Steel
    # Wave 4-7 engines (added 2026-03-20 through 2026-03-23)
    "OBRIX":     (0x1E, 0x8B, 0x7E),  # Reef Jade
    "ORBWEAVE":  (0x8E, 0x45, 0x85),  # Kelp Knot Purple
    "OVERTONE":  (0xA8, 0xD8, 0xEA),  # Spectral Ice
    "ORGANISM":  (0xC6, 0xE3, 0x77),  # Emergence Lime
    "OXBOW":     (0x1A, 0x6B, 0x5A),  # Oxbow Teal
    "OWARE":     (0xB5, 0x88, 0x3E),  # Akan Goldweight
    "OPERA":     (0xD4, 0xAF, 0x37),  # Aria Gold
    "OFFERING":  (0xE5, 0xB8, 0x0B),  # Crate Wax Yellow
    "OSMOSIS":   (0xC0, 0xC0, 0xC0),  # Surface Tension Silver
    # Engine #48 (added 2026-03-23)
    "OXYTOCIN":  (0x9B, 0x5D, 0xE5),  # Synapse Violet
}

DEFAULT_COLOR = (0x44, 0x44, 0x44)  # fallback dark gray

BG_WHITE    = (0xF8, 0xF6, 0xF3)   # Gallery Model warm white
TEXT_DARK   = (0x2C, 0x2C, 0x2C)   # dark text on white
DOT_STRIP_H = 30                    # height of DNA dot strip at bottom


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
    """Serialize a 2D list of (R,G,B) tuples to PNG bytes. pixels[y][x] = (r,g,b)."""
    sig = b'\x89PNG\r\n\x1a\n'

    ihdr_data = struct.pack('>IIBBBBB', width, height,
                            8,   # bit depth
                            2,   # color type: RGB
                            0,   # compression
                            0,   # filter
                            0)   # interlace
    ihdr = _png_chunk(b'IHDR', ihdr_data)

    raw_rows = []
    for y in range(height):
        row_bytes = bytearray([0x00])  # filter type None
        for x in range(width):
            r, g, b = pixels[y][x]
            row_bytes += bytes([r, g, b])
        raw_rows.append(bytes(row_bytes))

    compressed = zlib.compress(b''.join(raw_rows), 9)
    idat = _png_chunk(b'IDAT', compressed)
    iend = _png_chunk(b'IEND', b'')
    return sig + ihdr + idat + iend


# ---------------------------------------------------------------------------
# Pixel operations
# ---------------------------------------------------------------------------

def _make_pixels(width: int, height: int, color: tuple) -> list:
    return [[color] * width for _ in range(height)]


def _set_pixel(pixels: list, x: int, y: int, color: tuple, width: int, height: int):
    if 0 <= x < width and 0 <= y < height:
        pixels[y][x] = color


def _draw_rect(pixels: list, x0: int, y0: int, x1: int, y1: int,
               color: tuple, width: int, height: int):
    """Fill a filled rectangle (x0,y0) inclusive to (x1,y1) exclusive."""
    for y in range(max(0, y0), min(height, y1)):
        for x in range(max(0, x0), min(width, x1)):
            pixels[y][x] = color


def _blend(c1: tuple, c2: tuple, t: float) -> tuple:
    """Linear blend between two RGB tuples. t=0 -> c1, t=1 -> c2."""
    return tuple(int(c1[i] + (c2[i] - c1[i]) * t) for i in range(3))


def _luminance(color: tuple) -> float:
    """Relative luminance (0-1) of an sRGB color."""
    return 0.299 * color[0] / 255 + 0.587 * color[1] / 255 + 0.114 * color[2] / 255


# ---------------------------------------------------------------------------
# 5x7 pixel-art font rendering
# ---------------------------------------------------------------------------

def _char_width(scale: int) -> int:
    return 5 * scale + scale  # 5 pixels + 1-pixel gap


def _text_pixel_width(text: str, scale: int) -> int:
    return len(text) * _char_width(scale) - scale  # remove trailing gap


def _draw_char(pixels: list, ch: str, ox: int, oy: int, color: tuple,
               width: int, height: int, scale: int = 1):
    """Draw a single 5x7 character at (ox, oy)."""
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


def _draw_text_at(pixels: list, text: str, ox: int, oy: int, color: tuple,
                  img_width: int, img_height: int, scale: int = 1,
                  max_width: int = None):
    """Draw text starting at (ox, oy). Clips to max_width if given."""
    if max_width is not None:
        max_chars = max(1, max_width // _char_width(scale))
        if len(text) > max_chars:
            text = text[:max_chars - 1] + '.'
    for i, ch in enumerate(text):
        _draw_char(pixels, ch, ox + i * _char_width(scale), oy,
                   color, img_width, img_height, scale)


def _draw_text_centered_in_zone(pixels: list, text: str, zone_x0: int, zone_x1: int,
                                cy: int, color: tuple,
                                img_width: int, img_height: int, scale: int = 1):
    """Draw text centered horizontally within a horizontal zone."""
    zone_w = zone_x1 - zone_x0
    max_chars = max(1, (zone_w - 4) // _char_width(scale))
    if len(text) > max_chars:
        text = text[:max_chars - 1] + '.'
    tw = _text_pixel_width(text, scale)
    ox = zone_x0 + (zone_w - tw) // 2
    for i, ch in enumerate(text):
        _draw_char(pixels, ch, ox + i * _char_width(scale), cy,
                   color, img_width, img_height, scale)


# ---------------------------------------------------------------------------
# XO_OX wordmark — interlocked X and O letters, bottom-right of white zone
# ---------------------------------------------------------------------------

def _draw_xo_wordmark(pixels: list, right_x: int, bottom_y: int,
                      color: tuple, img_width: int, img_height: int, scale: int = 1):
    """
    Draw 'XO' in pixel-art font with slight overlap, ending at (right_x, bottom_y).
    """
    text = 'XO'
    tw = _text_pixel_width(text, scale)
    th = 7 * scale
    ox = right_x - tw
    oy = bottom_y - th
    for i, ch in enumerate(text):
        _draw_char(pixels, ch, ox + i * _char_width(scale), oy,
                   color, img_width, img_height, scale)


# ---------------------------------------------------------------------------
# DNA dot strip at bottom
# ---------------------------------------------------------------------------

DNA_KEYS = ['Warmth', 'Space', 'Mechanical', 'Organic', 'Density', 'Movement']
# 6 neutral-ish dot colors that read on accent backgrounds
DOT_FILL_COLOR  = (0xFF, 0xFF, 0xFF)  # filled dot — white
DOT_EMPTY_COLOR = (0x00, 0x00, 0x00)  # empty outline — alpha proxy: black tint


def _draw_dna_dots(pixels: list, dna: dict, accent: tuple,
                   img_width: int, img_height: int, strip_height: int = 30):
    """
    Bottom strip: solid accent background, 6 circular dots.
    Each dot is filled (white) or empty (dark) proportionally to its DNA value.
    For fractional values, the dot fills proportionally (full/empty using threshold 0.5).
    """
    y0 = img_height - strip_height
    y1 = img_height
    _draw_rect(pixels, 0, y0, img_width, y1, accent, img_width, img_height)

    n = 6
    dot_r = max(3, strip_height // 5)
    total_dot_w = n * (dot_r * 2 + 4)
    start_x = (img_width - total_dot_w) // 2 + dot_r + 2

    for i, key in enumerate(DNA_KEYS):
        value = float(dna.get(key, 0.5))
        value = max(0.0, min(1.0, value))
        cx = start_x + i * (dot_r * 2 + 4)
        cy = y0 + strip_height // 2

        # Draw dot as filled circle or outlined circle
        filled = value >= 0.5
        for dy in range(-dot_r, dot_r + 1):
            for dx in range(-dot_r, dot_r + 1):
                dist_sq = dx * dx + dy * dy
                if dist_sq <= dot_r * dot_r:
                    if filled:
                        _set_pixel(pixels, cx + dx, cy + dy,
                                   DOT_FILL_COLOR, img_width, img_height)
                    else:
                        # Empty interior — keep accent color, draw white rim only
                        rim = dist_sq >= (dot_r - 1) * (dot_r - 1)
                        if rim:
                            _set_pixel(pixels, cx + dx, cy + dy,
                                       DOT_FILL_COLOR, img_width, img_height)


# ---------------------------------------------------------------------------
# Main generation
# ---------------------------------------------------------------------------

def generate_cover_art(engine: str, pack_name: str, sonic_dna: dict,
                       size: int = 400) -> bytes:
    """
    Generate a Gallery Model flag-layout cover art PNG and return raw bytes.

    Layout (400x400 base):
      - Left 40%  : solid accent color block
      - Right 60% : warm white, pack name + engine name + XO_OX mark
      - Bottom 30px: DNA dot strip spanning full width, accent background

    Parameters
    ----------
    engine    : Engine short name, e.g. 'ONSET'
    pack_name : Human-readable pack name
    sonic_dna : Dict of 6 DNA dimension values (0.0-1.0)
    size      : Image dimension in pixels (square)
    """
    W = H = size
    scale_factor = size / 400.0

    accent = ENGINE_COLORS.get(engine.upper(), DEFAULT_COLOR)

    # Scale dot strip height
    strip_h = max(20, int(DOT_STRIP_H * scale_factor))

    # --- Canvas: warm white ---
    pixels = _make_pixels(W, H, BG_WHITE)

    # --- Left color block: left 40%, full height (including dot strip area) ---
    flag_w = int(W * 0.40)
    _draw_rect(pixels, 0, 0, flag_w, H, accent, W, H)

    # --- Thin vertical separator line between flag and white zone ---
    sep_x = flag_w
    sep_color = _blend(accent, TEXT_DARK, 0.3)
    _draw_rect(pixels, sep_x, 0, sep_x + max(1, int(2 * scale_factor)), H,
               sep_color, W, H)

    # --- Engine initial(s) large in left block (vertical center, above strip) ---
    # Use first letter of engine name as a bold accent within the block
    lum = _luminance(accent)
    flag_text_color = (0xFF, 0xFF, 0xFF) if lum < 0.55 else TEXT_DARK
    engine_initial = engine[0].upper() if engine else 'X'
    init_scale = max(4, int(10 * scale_factor))
    # Center initial in the flag zone vertically (exclude dot strip)
    usable_h = H - strip_h
    init_th = 7 * init_scale
    init_tw = _text_pixel_width(engine_initial, init_scale)
    init_ox = (flag_w - init_tw) // 2
    init_oy = (usable_h - init_th) // 2
    _draw_char(pixels, engine_initial, init_ox, init_oy,
               flag_text_color, W, H, scale=init_scale)

    # --- Right zone: x from flag_w+gap to W ---
    right_x0 = flag_w + max(2, int(4 * scale_factor))
    right_w   = W - right_x0

    # Pack name — large, centered in right zone
    # Available vertical space (no dot strip): 0 to H-strip_h
    scale_large = max(2, int(size / 140))
    pack_th = 7 * scale_large
    # Position pack name at ~38% from top of usable area
    pack_cy = int(usable_h * 0.35)
    pack_upper = pack_name.upper()
    _draw_text_centered_in_zone(pixels, pack_upper,
                                right_x0, W,
                                pack_cy, TEXT_DARK, W, H, scale=scale_large)

    # Engine name — smaller, in accent color, below pack name
    scale_small = max(1, int(size / 200))
    engine_name_y = pack_cy + pack_th + max(6, int(10 * scale_factor))
    _draw_text_centered_in_zone(pixels, engine.upper(),
                                right_x0, W,
                                engine_name_y, accent, W, H, scale=scale_small)

    # Thin horizontal rule below engine name
    rule_y = engine_name_y + 7 * scale_small + max(4, int(6 * scale_factor))
    rule_color = _blend(accent, BG_WHITE, 0.5)
    rule_x0 = right_x0 + int(right_w * 0.10)
    rule_x1 = W - int(right_w * 0.10)
    _draw_rect(pixels, rule_x0, rule_y, rule_x1, rule_y + 1, rule_color, W, H)

    # XO_OX wordmark — bottom-right of white area (above dot strip)
    xo_scale = max(1, int(size / 200))
    xo_margin = max(8, int(12 * scale_factor))
    xo_bottom = usable_h - xo_margin
    xo_right  = W - xo_margin
    _draw_xo_wordmark(pixels, xo_right, xo_bottom, TEXT_DARK, W, H, scale=xo_scale)

    # --- DNA dot strip at bottom: full width ---
    _draw_dna_dots(pixels, sonic_dna, accent, W, H, strip_height=strip_h)

    return _build_png(pixels, W, H)


# ---------------------------------------------------------------------------
# XPN reading helpers (identical to v1)
# ---------------------------------------------------------------------------

def _read_xpn(xpn_path: str):
    """
    Read a .xpn ZIP archive and extract engine name, pack name, sonic_dna dict.
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
        for candidate in [n for n in names if n.endswith('expansion.json')]:
            try:
                data = json.loads(zf.read(candidate))
                pack_name = data.get('name', data.get('pack_name', pack_name))
                engine    = data.get('engine', data.get('Engine', engine))
                if 'sonic_dna' in data:
                    sonic_dna.update(data['sonic_dna'])
                break
            except Exception as exc:
                print(f"[WARN] Reading expansion.json {candidate} for cover art metadata: {exc}", file=sys.stderr)

        # Try bundle_manifest.json
        for candidate in [n for n in names if n.endswith('bundle_manifest.json')]:
            try:
                data = json.loads(zf.read(candidate))
                pack_name = data.get('name', data.get('pack_name', pack_name))
                engine    = data.get('engine', data.get('Engine', engine))
                if 'sonic_dna' in data:
                    sonic_dna.update(data['sonic_dna'])
                break
            except Exception as exc:
                print(f"[WARN] Reading bundle_manifest.json {candidate} for cover art metadata: {exc}", file=sys.stderr)

        # Fallback: any JSON with engine/name keys
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
                        print(f"[WARN] Reading fallback JSON {name} for cover art metadata: {exc}", file=sys.stderr)

    return engine, pack_name, sonic_dna


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def main():
    parser = argparse.ArgumentParser(
        description=(
            'Generate a Gallery Model flag-layout cover art PNG for a .xpn pack '
            '(stdlib only, no external deps).'
        )
    )
    parser.add_argument('xpn', nargs='?', help='Path to .xpn pack file')
    parser.add_argument('--output', '-o', default=None,
                        help='Output PNG path (default: <pack>.cover.png)')
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

    if args.engine:
        engine = args.engine
    if args.name:
        pack_name = args.name

    print(f"  Engine   : {engine}")
    print(f"  Pack     : {pack_name}")
    print(f"  Sonic DNA: {sonic_dna}")

    output = args.output or str(xpn_path.with_suffix('.cover.png'))

    print(f"Generating {args.size}x{args.size} Gallery Model cover art ...")
    png_bytes = generate_cover_art(engine, pack_name, sonic_dna, size=args.size)

    with open(output, 'wb') as f:
        f.write(png_bytes)

    kb = len(png_bytes) / 1024
    print(f"Saved: {output}  ({kb:.1f} KB)")


if __name__ == '__main__':
    main()
