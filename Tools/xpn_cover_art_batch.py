#!/usr/bin/env python3
"""
XPN Cover Art Batch Generator — XO_OX Designs
Generates cover art for multiple MPC expansion packs from a JSON manifest.

Supports three layout modes determined by number of engines:
  - single     (1 engine)  : dominant accent color, standard single-engine cover
  - duo        (2 engines) : diagonal split with both engine accents
  - collection (3+ engines): color grid mosaic with all accent colors

CLI:
    python xpn_cover_art_batch.py --manifest packs.json --output-dir ./covers/ [--size 1400]

Manifest format (packs.json):
    [
      {
        "pack_name":     "808 Reborn Collection",
        "engine":        "ONSET",                        # primary engine (required)
        "engines":       ["ONSET"],                      # full engine list (optional; overrides "engine")
        "accent_color":  "#0066FF",                      # override; derived from ENGINE_DEFS if omitted
        "mood_category": "Foundation",
        "pack_number":   1,                              # optional
        "preset_count":  20,                             # optional, default 0
        "version":       "1.0"                           # optional, default "1.0"
      },
      ...
    ]

Dependencies: Pillow, numpy (same as xpn_cover_art.py)
"""

import argparse
import hashlib
import json
import math
import os
import sys
import time
from concurrent.futures import ProcessPoolExecutor, as_completed
from datetime import date
from pathlib import Path

# ---------------------------------------------------------------------------
# Resolve sibling xpn_cover_art module
# ---------------------------------------------------------------------------

_TOOLS_DIR = Path(__file__).parent
if str(_TOOLS_DIR) not in sys.path:
    sys.path.insert(0, str(_TOOLS_DIR))

try:
    from xpn_cover_art import (
        ENGINE_DEFS,
        STYLE_FUNCS,
        XO_GOLD,
        WARM_WHITE,
        PILLOW_AVAILABLE,
        _make_base,
        _blend_layer,
        _arr_to_image,
        _add_text_overlay,
    )
except ImportError as _e:
    print(f"ERROR: cannot import xpn_cover_art — {_e}")
    sys.exit(1)

try:
    import numpy as np
    from PIL import Image, ImageDraw, ImageFilter, ImageFont
except ImportError:
    print("ERROR: pip install Pillow numpy")
    sys.exit(1)


# ---------------------------------------------------------------------------
# Layout mode detection
# ---------------------------------------------------------------------------

def _detect_layout(engine_list: list) -> str:
    """Return 'single', 'duo', or 'collection' based on engine count."""
    n = len(engine_list)
    if n <= 1:
        return "single"
    if n == 2:
        return "duo"
    return "collection"


# ---------------------------------------------------------------------------
# Hex ↔ RGB helpers
# ---------------------------------------------------------------------------

def _hex_to_rgb(hex_color: str) -> tuple:
    h = hex_color.lstrip("#")
    return tuple(int(h[i:i+2], 16) for i in (0, 2, 4))


def _engine_accent(engine_name: str) -> tuple:
    """Return (R, G, B) accent for an engine name. Falls back to XO Gold."""
    key = engine_name.upper()
    return ENGINE_DEFS.get(key, ENGINE_DEFS["DEFAULT"])["accent"]


def _engine_bg(engine_name: str) -> tuple:
    key = engine_name.upper()
    return ENGINE_DEFS.get(key, ENGINE_DEFS["DEFAULT"])["bg_base"]


# ---------------------------------------------------------------------------
# Multi-engine background builders
# ---------------------------------------------------------------------------

def _make_duo_base(size: int, engine_a: str, engine_b: str) -> "np.ndarray":
    """Diagonal split: upper-left tinted to engine_a, lower-right to engine_b."""
    bg_a = _engine_bg(engine_a)
    bg_b = _engine_bg(engine_b)
    acc_a = _engine_accent(engine_a)
    acc_b = _engine_accent(engine_b)

    arr = np.zeros((size, size, 3), dtype=np.float32)

    # Base gradient for each engine quadrant, blended by diagonal mask
    Y, X = np.mgrid[0:size, 0:size]
    # Diagonal mask: 1.0 = fully engine_a side, 0.0 = fully engine_b side
    mask = np.clip((X + Y) / (2.0 * (size - 1)), 0.0, 1.0)  # 0 top-left, 1 bottom-right
    mask_inv = 1.0 - mask

    # Soft diagonal boundary (sigmoid-ish blend around the split line)
    mid = (X + Y) / (2.0 * (size - 1)) - 0.5
    blend_width = 0.08
    soft = np.clip(mid / blend_width * 0.5 + 0.5, 0.0, 1.0)

    # Radial vignette from each engine's "center"
    cx_a, cy_a = size * 0.25, size * 0.25
    cx_b, cy_b = size * 0.75, size * 0.75
    dist_a = np.sqrt((X - cx_a)**2 + (Y - cy_a)**2) / (size * 0.7)
    dist_b = np.sqrt((X - cx_b)**2 + (Y - cy_b)**2) / (size * 0.7)
    bright_a = (1.0 - np.clip(dist_a, 0, 1) * 0.6)
    bright_b = (1.0 - np.clip(dist_b, 0, 1) * 0.6)

    for ch, (va, vb) in enumerate(zip(bg_a, bg_b)):
        layer_a = (va / 255.0) * bright_a
        layer_b = (vb / 255.0) * bright_b
        arr[:, :, ch] = layer_a * (1.0 - soft) + layer_b * soft

    # Subtle accent tint on each side
    for ch, (va, vb) in enumerate(zip(acc_a, acc_b)):
        tint_a = (va / 255.0) * 0.08 * (1.0 - soft)
        tint_b = (vb / 255.0) * 0.08 * soft
        arr[:, :, ch] = np.clip(arr[:, :, ch] + tint_a + tint_b, 0.0, 1.0)

    # Sharp diagonal accent line at the split
    line_mask = np.abs(mid) < (blend_width * 0.15)
    avg_accent = tuple((a + b) // 2 for a, b in zip(acc_a, acc_b))
    for ch, v in enumerate(avg_accent):
        arr[:, :, ch] = np.where(line_mask, np.clip(arr[:, :, ch] + v / 255.0 * 0.5, 0, 1), arr[:, :, ch])

    return arr


def _make_collection_base(size: int, engine_list: list) -> "np.ndarray":
    """Grid mosaic: canvas divided into N color zones, one per engine."""
    arr = np.zeros((size, size, 3), dtype=np.float32)
    n = len(engine_list)
    cols = math.ceil(math.sqrt(n))
    rows = math.ceil(n / cols)
    cell_w = size // cols
    cell_h = size // rows

    for idx, engine in enumerate(engine_list):
        row = idx // cols
        col = idx % cols
        x0 = col * cell_w
        y0 = row * cell_h
        x1 = min(size, x0 + cell_w)
        y1 = min(size, y0 + cell_h)

        accent = _engine_accent(engine)
        bg = _engine_bg(engine)

        # Radial vignette centered in cell
        cx = (x0 + x1) / 2
        cy = (y0 + y1) / 2
        Y_cell, X_cell = np.mgrid[y0:y1, x0:x1]
        dist = np.sqrt((X_cell - cx)**2 + (Y_cell - cy)**2) / (max(cell_w, cell_h) * 0.6)
        bright = (1.0 - np.clip(dist, 0, 1) * 0.55)

        for ch, (vbg, vacc) in enumerate(zip(bg, accent)):
            base_val = (vbg / 255.0) * bright
            tint = (vacc / 255.0) * 0.12
            arr[y0:y1, x0:x1, ch] = np.clip(base_val + tint, 0.0, 1.0)

    # Thin XO Gold grid lines between cells
    gold_f = tuple(v / 255.0 for v in XO_GOLD)
    for c in range(1, cols):
        x = c * cell_w
        arr[:, max(0, x-1):x+2, 0] = gold_f[0] * 0.6
        arr[:, max(0, x-1):x+2, 1] = gold_f[1] * 0.6
        arr[:, max(0, x-1):x+2, 2] = gold_f[2] * 0.6
    for r in range(1, rows):
        y = r * cell_h
        arr[max(0, y-1):y+2, :, 0] = gold_f[0] * 0.6
        arr[max(0, y-1):y+2, :, 1] = gold_f[1] * 0.6
        arr[max(0, y-1):y+2, :, 2] = gold_f[2] * 0.6

    return arr


# ---------------------------------------------------------------------------
# Multi-engine text overlay
# ---------------------------------------------------------------------------

def _add_multi_text_overlay(
    img: "Image.Image",
    engine_list: list,
    pack_name: str,
    mood_category: str,
    pack_number,
    preset_count: int,
    version: str,
    accent_override: tuple = None,
) -> "Image.Image":
    """Text overlay for duo/collection packs — lists all engines as pills."""
    draw = ImageDraw.Draw(img)
    size = img.width

    # Primary accent = first engine (or override)
    primary_accent = accent_override if accent_override else _engine_accent(engine_list[0])

    def try_font(size_pt):
        candidates = [
            "/System/Library/Fonts/Supplemental/Arial Bold.ttf",
            "/System/Library/Fonts/Helvetica.ttc",
            "/Library/Fonts/Arial Bold.ttf",
            "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf",
        ]
        for path in candidates:
            if os.path.exists(path):
                try:
                    return ImageFont.truetype(path, size_pt)
                except Exception as exc:
                    print(f"[WARN] Loading font {path}: {exc}", file=sys.stderr)
        return ImageFont.load_default()

    font_large  = try_font(int(size * 0.055))
    font_medium = try_font(int(size * 0.030))
    font_small  = try_font(int(size * 0.022))
    font_wm     = try_font(int(size * 0.028))

    pad = int(size * 0.04)
    bar_width = int(size * 0.012)

    # Left accent bar (primary engine color)
    draw.rectangle(
        [(pad, pad), (pad + bar_width, size - pad)],
        fill=primary_accent
    )

    text_x = pad + bar_width + int(size * 0.02)

    # Pack title
    text_y = int(size * 0.06)
    draw.text((text_x + 3, text_y + 3), pack_name, font=font_large, fill=(0, 0, 0, 140))
    draw.text((text_x, text_y), pack_name, font=font_large, fill=WARM_WHITE)

    # Engine pills — one per engine, in a row
    pill_y = text_y + int(size * 0.075)
    pill_pad_x = int(size * 0.012)
    pill_pad_y = int(size * 0.007)
    pill_gap = int(size * 0.012)
    cursor_x = text_x
    for eng_name in engine_list:
        acc = _engine_accent(eng_name)
        label = eng_name.upper()
        bbox = draw.textbbox((cursor_x, pill_y), label, font=font_medium)
        rect = (
            bbox[0] - pill_pad_x, bbox[1] - pill_pad_y,
            bbox[2] + pill_pad_x, bbox[3] + pill_pad_y,
        )
        # Wrap to next line if overflow
        if rect[2] > size - pad:
            pill_y += int(size * 0.05)
            cursor_x = text_x
            bbox = draw.textbbox((cursor_x, pill_y), label, font=font_medium)
            rect = (
                bbox[0] - pill_pad_x, bbox[1] - pill_pad_y,
                bbox[2] + pill_pad_x, bbox[3] + pill_pad_y,
            )
        draw.rounded_rectangle(rect, radius=int(size * 0.008), fill=acc)
        draw.text((cursor_x, pill_y), label, font=font_medium, fill=(0, 0, 0))
        cursor_x = rect[2] + pill_gap

    # Mood category badge (upper right)
    if mood_category:
        mood_text = mood_category.upper()
        mood_bbox = draw.textbbox((0, 0), mood_text, font=font_small)
        mood_w = mood_bbox[2] - mood_bbox[0]
        mood_x = size - pad - mood_w - int(size * 0.02)
        mood_y = int(size * 0.06)
        draw.text((mood_x + 1, mood_y + 1), mood_text, font=font_small, fill=(0, 0, 0, 100))
        draw.text((mood_x, mood_y), mood_text, font=font_small, fill=XO_GOLD)

    # Pack number (top right, small)
    if pack_number is not None:
        num_text = f"#{pack_number}"
        nb = draw.textbbox((0, 0), num_text, font=font_small)
        nx = size - pad - (nb[2] - nb[0])
        ny = int(size * 0.06) + int(size * 0.035)
        draw.text((nx, ny), num_text, font=font_small, fill=(180, 175, 170))

    # Preset count + version (bottom left)
    meta_y = size - pad - int(size * 0.06)
    if preset_count:
        count_text = f"{preset_count} PRESETS"
        draw.text((text_x + 1, meta_y + 1), count_text, font=font_small, fill=(0, 0, 0, 120))
        draw.text((text_x, meta_y), count_text, font=font_small, fill=XO_GOLD)

    ver_text = f"v{version}"
    draw.text((text_x, meta_y + int(size * 0.03)), ver_text, font=font_small, fill=(160, 155, 150))

    # XO_OX watermark
    wm_text = "XO_OX"
    wm_bbox = draw.textbbox((0, 0), wm_text, font=font_wm)
    wm_w = wm_bbox[2] - wm_bbox[0]
    wm_h = wm_bbox[3] - wm_bbox[1]
    draw.text((size - pad - wm_w, size - pad - wm_h), wm_text, font=font_wm, fill=(255, 255, 255, 55))

    return img


# ---------------------------------------------------------------------------
# Core single-pack render
# ---------------------------------------------------------------------------

def _seed_from_name(pack_name: str) -> int:
    """Deterministic seed from pack name — unique per pack, stable across runs."""
    digest = hashlib.md5(pack_name.encode("utf-8")).digest()
    return int.from_bytes(digest[:4], "big") % (2**31)


def generate_pack_cover(
    pack_name: str,
    engine_list: list,
    output_dir: str,
    accent_color: str = None,
    mood_category: str = "",
    pack_number=None,
    preset_count: int = 0,
    version: str = "1.0",
    size: int = 1400,
    seed: int = None,
    skip_existing: bool = False,
    dry_run: bool = False,
) -> dict:
    """
    Render cover art for a single pack and save PNG.

    Returns a result dict with keys: pack_name, output_path, layout, status.
    """
    import random as _random

    out_path = Path(output_dir)
    # Sanitize pack name for filename
    safe_name = "".join(c if c.isalnum() or c in " _-" else "_" for c in pack_name)
    safe_name = safe_name.replace(" ", "_").strip("_")
    filename = f"{safe_name}.png"
    dest = out_path / filename

    result = {
        "pack_name": pack_name,
        "output_path": str(dest),
        "layout": _detect_layout(engine_list),
        "engines": engine_list,
        "status": "ok",
    }

    if skip_existing and dest.exists():
        result["status"] = "skipped"
        return result

    if dry_run:
        result["status"] = "dry_run"
        return result

    out_path.mkdir(parents=True, exist_ok=True)

    # Resolve accent override
    accent_rgb = _hex_to_rgb(accent_color) if accent_color else None

    # Seed
    if seed is None:
        seed = _seed_from_name(pack_name)

    import numpy as np
    rng = _random.Random(seed)
    np_rng = np.random.RandomState(seed)

    class _RngAdapter:
        def __init__(self, r): self._r = r
        def randint(self, a, b): return int(self._r.randint(a, b + 1))
        def uniform(self, a, b): return float(self._r.uniform(a, b))
        def random(self): return float(self._r.uniform(0, 1))
        def gauss(self, mu, sigma): return float(self._r.normal(mu, sigma))

    rng_adapter = _RngAdapter(np_rng)
    SIZE = size

    layout = result["layout"]
    primary_engine = engine_list[0] if engine_list else "DEFAULT"
    eng_def = ENGINE_DEFS.get(primary_engine.upper(), ENGINE_DEFS["DEFAULT"])
    primary_accent = accent_rgb if accent_rgb else eng_def["accent"]

    # Build background based on layout
    if layout == "single":
        arr = _make_base(SIZE, eng_def["bg_base"])
    elif layout == "duo":
        arr = _make_duo_base(SIZE, engine_list[0], engine_list[1])
    else:
        arr = _make_collection_base(SIZE, engine_list)

    # Apply primary engine motif style
    style_func = STYLE_FUNCS.get(eng_def["style"], STYLE_FUNCS["freq_bands"])
    arr = style_func(arr, SIZE, primary_accent, rng_adapter)

    # Post-process
    img = _arr_to_image(arr)
    img = img.filter(ImageFilter.GaussianBlur(radius=0.8))
    img = img.convert("RGBA")

    if layout == "single":
        # Reuse the original single-engine text overlay for fidelity
        img = _add_text_overlay(img, eng_def, pack_name, preset_count, version)
    else:
        img = _add_multi_text_overlay(
            img, engine_list, pack_name, mood_category,
            pack_number, preset_count, version, accent_rgb
        )

    img = img.convert("RGB")
    img.save(str(dest), "PNG", optimize=True)
    return result


# ---------------------------------------------------------------------------
# Batch driver
# ---------------------------------------------------------------------------

def _load_manifest(manifest_path: str) -> list:
    with open(manifest_path, "r", encoding="utf-8") as fh:
        data = json.load(fh)
    if not isinstance(data, list):
        raise ValueError("Manifest must be a JSON array of pack entries.")
    return data


def _normalize_entry(entry: dict) -> dict:
    """Normalize a manifest entry to a consistent structure."""
    # Engine list: prefer "engines" array, fall back to singular "engine"
    if "engines" in entry and isinstance(entry["engines"], list):
        engines = [e.upper() for e in entry["engines"] if e]
    elif "engine" in entry:
        engines = [entry["engine"].upper()]
    else:
        engines = ["DEFAULT"]

    return {
        "pack_name":     entry.get("pack_name", "Unnamed Pack"),
        "engines":       engines,
        "accent_color":  entry.get("accent_color", None),
        "mood_category": entry.get("mood_category", ""),
        "pack_number":   entry.get("pack_number", None),
        "preset_count":  int(entry.get("preset_count", 0)),
        "version":       str(entry.get("version", "1.0")),
    }


def _worker(kwargs: dict) -> dict:
    """Top-level function for multiprocessing (must be picklable)."""
    try:
        return generate_pack_cover(**kwargs)
    except Exception as exc:
        return {
            "pack_name": kwargs.get("pack_name", "?"),
            "output_path": "",
            "layout": "?",
            "engines": kwargs.get("engine_list", []),
            "status": f"error: {exc}",
        }


def run_batch(
    manifest_path: str,
    output_dir: str,
    size: int = 1400,
    skip_existing: bool = False,
    dry_run: bool = False,
    workers: int = 1,
) -> dict:
    """
    Process all entries in the manifest and write PNGs to output_dir.

    Returns a batch_report dict.
    """
    entries = _load_manifest(manifest_path)
    print(f"Batch: {len(entries)} packs | size={size}px | workers={workers}"
          + (" [DRY RUN]" if dry_run else ""))

    jobs = []
    for raw in entries:
        norm = _normalize_entry(raw)
        jobs.append({
            "pack_name":     norm["pack_name"],
            "engine_list":   norm["engines"],
            "output_dir":    output_dir,
            "accent_color":  norm["accent_color"],
            "mood_category": norm["mood_category"],
            "pack_number":   norm["pack_number"],
            "preset_count":  norm["preset_count"],
            "version":       norm["version"],
            "size":          size,
            "skip_existing": skip_existing,
            "dry_run":       dry_run,
        })

    t_start = time.time()
    results = []

    if workers > 1 and not dry_run:
        with ProcessPoolExecutor(max_workers=workers) as pool:
            futures = {pool.submit(_worker, j): j for j in jobs}
            for future in as_completed(futures):
                r = future.result()
                results.append(r)
                _print_result(r)
    else:
        for j in jobs:
            r = _worker(j)
            results.append(r)
            _print_result(r)

    elapsed = time.time() - t_start

    counts = {"ok": 0, "skipped": 0, "dry_run": 0, "error": 0}
    for r in results:
        status = r["status"]
        if status.startswith("error"):
            counts["error"] += 1
        elif status in counts:
            counts[status] += 1
        else:
            counts["ok"] += 1

    report = {
        "generated":    date.today().isoformat(),
        "manifest":     str(manifest_path),
        "output_dir":   str(output_dir),
        "size":         size,
        "total":        len(results),
        "counts":       counts,
        "elapsed_s":    round(elapsed, 2),
        "packs":        results,
    }

    report_path = Path(output_dir) / "batch_report.json"
    if not dry_run:
        Path(output_dir).mkdir(parents=True, exist_ok=True)
        with open(report_path, "w", encoding="utf-8") as fh:
            json.dump(report, fh, indent=2)
        print(f"\nReport: {report_path}")

    print(
        f"\nDone: {counts['ok']} generated, {counts['skipped']} skipped, "
        f"{counts['error']} errors — {elapsed:.1f}s"
    )
    return report


def _print_result(r: dict):
    icon = {"ok": "OK", "skipped": "SKIP", "dry_run": "DRY"}.get(r["status"], "ERR")
    engines_str = "+".join(r.get("engines", []))
    print(f"  [{icon}] {r['pack_name']!r:40s} [{r['layout']:>10}] {engines_str}")


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

if __name__ == "__main__":
    if not PILLOW_AVAILABLE:
        print("ERROR: pip install Pillow numpy")
        sys.exit(1)

    parser = argparse.ArgumentParser(
        description="Batch XPN cover art generator — XO_OX Designs",
        formatter_class=argparse.RawTextHelpFormatter,
    )
    parser.add_argument(
        "--manifest", required=True,
        help="Path to JSON manifest (array of pack entries).",
    )
    parser.add_argument(
        "--output-dir", required=True,
        help="Directory to write PNG files and batch_report.json.",
    )
    parser.add_argument(
        "--size", type=int, default=1400,
        help="Output image size in pixels (square). Default: 1400.",
    )
    parser.add_argument(
        "--skip-existing", action="store_true", default=False,
        help="Skip packs whose output PNG already exists.",
    )
    parser.add_argument(
        "--dry-run", action="store_true", default=False,
        help="Print planned operations without writing any files.",
    )
    parser.add_argument(
        "--workers", type=int, default=1,
        help="Number of parallel worker processes. Default: 1.",
    )

    args = parser.parse_args()

    run_batch(
        manifest_path=args.manifest,
        output_dir=args.output_dir,
        size=args.size,
        skip_existing=args.skip_existing,
        dry_run=args.dry_run,
        workers=args.workers,
    )
