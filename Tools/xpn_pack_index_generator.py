#!/usr/bin/env python3
"""
xpn_pack_index_generator.py — XO_OX Designs
Scans a directory of .xpn pack files and generates a self-contained static
index.html for local browsing of the pack catalog.

Each .xpn is a ZIP archive; this tool reads bundle_manifest.json from the
root of each archive to extract metadata.  Packs missing a bundle_manifest
are still listed with graceful fallbacks.

Usage:
    python xpn_pack_index_generator.py --packs-dir ./dist
    python xpn_pack_index_generator.py --packs-dir ./dist --output ./catalog.html
    python xpn_pack_index_generator.py --packs-dir ./dist --title "My Pack Catalog"

Output: a single self-contained HTML file — no external assets, no server needed.
"""

import argparse
import json
import os
import sys
import zipfile
from pathlib import Path
from typing import Optional

# ---------------------------------------------------------------------------
# Engine accent colors — canonical list (all 38 registered + concept engines)
# ---------------------------------------------------------------------------

ENGINE_COLORS: dict[str, str] = {
    "ONSET":      "#0066FF",
    "OBLONG":     "#E9A84A",
    "OBESE":      "#FF1493",
    "ORACLE":     "#4B0082",
    "OVERWORLD":  "#39FF14",
    "OPAL":       "#A78BFA",
    "OVERDUB":    "#6B7B3A",
    "ODYSSEY":    "#7B2D8B",
    "OBBLIGATO":  "#FF8A7A",
    "OSTERIA":    "#722F37",
    "OHM":        "#87AE73",
    "ORPHICA":    "#7FDBCA",
    "OTTONI":     "#5B8A72",
    "OLE":        "#C9377A",
    "OMBRE":      "#7B6B8A",
    "ORCA":       "#1B2838",
    "OCTOPUS":    "#E040FB",
    "OVERLAP":    "#00FFB4",
    "OUTWIT":     "#CC6600",
    "OBSCURA":    "#8A9BA8",
    "OCEANIC":    "#00B4A0",
    "OBLIQUE":    "#BF40FF",
    "OSPREY":     "#1B4F8A",
    "ORBITAL":    "#FF6B6B",
    "OPTIC":      "#00FF41",
    "ORIGAMI":    "#E63946",
    "OUROBOROS":  "#FF2D2D",
    "OBSIDIAN":   "#E8E0D8",
    "OCELOT":     "#C5832B",
    "OVERBITE":   "#F0EDE8",
    "ORGANON":    "#00CED1",
    "OWLFISH":    "#B8860B",
    "ODDFELIX":   "#00A6D6",
    "ODDOSCAR":   "#E8839B",
    # V1 concept engines
    "OSTINATO":   "#E8701A",
    "OPENSKY":    "#FF8C00",
    "OCEANDEEP":  "#2D0A4E",
    "OUIE":       "#708090",
}

# Moods in canonical order
MOODS = ["Foundation", "Atmosphere", "Entangled", "Prism", "Flux", "Aether", "Family"]

# Sonic DNA axis labels in order
DNA_AXES = ["brightness", "warmth", "movement", "density", "space", "aggression"]

# Fallback color for unknown engines
FALLBACK_COLOR = "#999999"


# ---------------------------------------------------------------------------
# Pack reading
# ---------------------------------------------------------------------------

def _read_bundle_manifest(xpn_path: Path) -> dict:
    """Read bundle_manifest.json from a .xpn ZIP, returning a dict (may be empty)."""
    try:
        with zipfile.ZipFile(xpn_path, "r") as zf:
            names = zf.namelist()
            # bundle_manifest may be at root or one level deep
            candidates = [n for n in names if n.endswith("bundle_manifest.json")]
            if not candidates:
                return {}
            # Prefer shortest path (root-level)
            candidates.sort(key=len)
            data = zf.read(candidates[0])
            return json.loads(data.decode("utf-8"))
    except Exception:
        return {}


def scan_packs(packs_dir: Path) -> list[dict]:
    """Scan packs_dir for .xpn files, returning sorted list of pack metadata dicts."""
    packs = []
    for xpn_file in sorted(packs_dir.glob("*.xpn")):
        manifest = _read_bundle_manifest(xpn_file)

        # Primary engine: manifest may have "engines" list or singular "engine"
        engines_raw = manifest.get("engines") or []
        if isinstance(engines_raw, str):
            engines_raw = [engines_raw]
        if not engines_raw and manifest.get("engine"):
            engines_raw = [manifest["engine"]]
        engines = [e.upper() for e in engines_raw] if engines_raw else ["UNKNOWN"]

        sonic_dna = manifest.get("sonic_dna") or {}

        tags = manifest.get("tags") or []
        if isinstance(tags, str):
            tags = [t.strip() for t in tags.split(",") if t.strip()]

        # Infer tier from tags first, then mood
        tier = _infer_tier(tags, manifest.get("mood", ""))

        packs.append({
            "filename":     xpn_file.name,
            "pack_name":    manifest.get("pack_name") or xpn_file.stem,
            "engines":      engines,
            "mood":         manifest.get("mood") or "Unknown",
            "version":      manifest.get("version") or "1.0.0",
            "preset_count": manifest.get("program_count") or manifest.get("preset_count") or 0,
            "sonic_dna":    sonic_dna,
            "tags":         tags,
            "tier":         tier,
            "collection":   manifest.get("collection") or "",
        })
    return packs


def _infer_tier(tags: list[str], mood: str) -> str:
    """Derive a tier label from tags or mood for badge display."""
    tags_lower = {t.lower() for t in tags}
    for candidate in ("flagship", "essentials", "core", "deluxe", "starter", "free"):
        if candidate in tags_lower:
            return candidate.capitalize()
    # Fall back to mood category
    if mood in ("Foundation", "Atmosphere"):
        return "Core"
    if mood in ("Entangled", "Prism"):
        return "Expanded"
    if mood in ("Flux", "Aether", "Family"):
        return "Advanced"
    return "Standard"


# ---------------------------------------------------------------------------
# HTML generation helpers
# ---------------------------------------------------------------------------

def _engine_badge_html(engine: str) -> str:
    color = ENGINE_COLORS.get(engine, FALLBACK_COLOR)
    # Choose text color for contrast (light bg → dark text)
    text_color = _contrasting_text(color)
    return (
        f'<span class="engine-badge" '
        f'style="background:{color};color:{text_color}">'
        f'{engine}</span>'
    )


def _contrasting_text(hex_color: str) -> str:
    """Return #1A1A1A or #F8F6F3 for readable contrast against hex_color."""
    hex_color = hex_color.lstrip("#")
    if len(hex_color) != 6:
        return "#1A1A1A"
    r, g, b = int(hex_color[0:2], 16), int(hex_color[2:4], 16), int(hex_color[4:6], 16)
    # Relative luminance (simplified)
    lum = (0.299 * r + 0.587 * g + 0.114 * b) / 255
    return "#1A1A1A" if lum > 0.45 else "#F8F6F3"


def _dna_bars_html(sonic_dna: dict) -> str:
    """Return HTML for a 6-bar mini Sonic DNA chart."""
    bars = []
    for axis in DNA_AXES:
        val = sonic_dna.get(axis, 0.5)
        try:
            val = float(val)
        except (TypeError, ValueError):
            val = 0.5
        val = max(0.0, min(1.0, val))
        pct = int(val * 100)
        # Map axis to a hue for visual differentiation
        hue_map = {
            "brightness": "#E9C46A",
            "warmth":     "#E07A4A",
            "movement":   "#6ABFAD",
            "density":    "#7B8FA8",
            "space":      "#A89CCC",
            "aggression": "#D45F5F",
        }
        bar_color = hue_map.get(axis, "#E9C46A")
        bars.append(
            f'<div class="dna-bar-wrap" title="{axis}: {val:.2f}">'
            f'<div class="dna-bar" style="height:{pct}%;background:{bar_color}"></div>'
            f'<div class="dna-label">{axis[:3]}</div>'
            f'</div>'
        )
    return '<div class="dna-chart">' + "".join(bars) + "</div>"


def _pack_card_html(pack: dict) -> str:
    engines_html = " ".join(_engine_badge_html(e) for e in pack["engines"])
    dna_html = _dna_bars_html(pack["sonic_dna"])
    tier_color = "#E9C46A" if pack["tier"] in ("Core", "Flagship") else "#C8BEA8"
    tags_str = " ".join(f'<span class="tag">{t}</span>' for t in pack["tags"][:5])
    # data attributes used by JS filter
    data_engines = ",".join(pack["engines"])
    return f"""
    <div class="pack-card"
         data-engines="{data_engines}"
         data-mood="{pack['mood']}"
         data-tier="{pack['tier']}">
      <div class="card-header">
        <span class="pack-name">{pack['pack_name']}</span>
        <span class="tier-badge" style="background:{tier_color}">{pack['tier']}</span>
      </div>
      <div class="engines-row">{engines_html}</div>
      <div class="meta-row">
        <span class="mood-tag">{pack['mood']}</span>
        <span class="version">v{pack['version']}</span>
        <span class="preset-count">{pack['preset_count']} presets</span>
      </div>
      {dna_html}
      <div class="tags-row">{tags_str}</div>
      <div class="filename">{pack['filename']}</div>
    </div>"""


# ---------------------------------------------------------------------------
# Full HTML assembly
# ---------------------------------------------------------------------------

def build_html(packs: list[dict], title: str) -> str:
    # Collect unique values for filter dropdowns
    all_engines = sorted({e for p in packs for e in p["engines"]})
    all_moods = [m for m in MOODS if any(p["mood"] == m for p in packs)]
    all_tiers = sorted({p["tier"] for p in packs})

    engine_options = "\n".join(
        f'<option value="{e}">{e}</option>' for e in all_engines
    )
    mood_options = "\n".join(
        f'<option value="{m}">{m}</option>' for m in all_moods
    )
    tier_options = "\n".join(
        f'<option value="{t}">{t}</option>' for t in all_tiers
    )

    cards_html = "\n".join(_pack_card_html(p) for p in packs)
    pack_count = len(packs)

    return f"""<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>{title}</title>
<style>
  *, *::before, *::after {{ box-sizing: border-box; margin: 0; padding: 0; }}
  body {{
    font-family: 'Space Grotesk', Inter, system-ui, sans-serif;
    background: #F8F6F3;
    color: #1A1A1A;
    min-height: 100vh;
  }}
  header {{
    background: #1A1A1A;
    color: #F8F6F3;
    padding: 28px 32px 20px;
    border-bottom: 3px solid #E9C46A;
  }}
  header h1 {{ font-size: 1.6rem; letter-spacing: .04em; color: #E9C46A; }}
  header p {{ margin-top: 6px; font-size: .85rem; color: #A89C8C; }}
  .filter-bar {{
    display: flex; gap: 12px; flex-wrap: wrap;
    padding: 16px 32px;
    background: #EFEDE9;
    border-bottom: 1px solid #DDD8D0;
    align-items: center;
  }}
  .filter-bar label {{ font-size: .78rem; font-weight: 600; color: #5A5046; margin-right: 2px; }}
  .filter-bar select, .filter-bar input {{
    padding: 6px 10px; border: 1px solid #C8BEA8;
    border-radius: 6px; background: #F8F6F3;
    font-size: .82rem; color: #1A1A1A;
    outline: none; cursor: pointer;
  }}
  .filter-bar select:focus, .filter-bar input:focus {{
    border-color: #E9C46A; box-shadow: 0 0 0 2px rgba(233,196,106,.25);
  }}
  .filter-bar button {{
    padding: 6px 16px; background: #E9C46A; color: #1A1A1A;
    border: none; border-radius: 6px; font-size: .82rem;
    font-weight: 700; cursor: pointer; letter-spacing: .03em;
  }}
  .filter-bar button:hover {{ background: #D4A93A; }}
  .count-bar {{
    padding: 10px 32px;
    font-size: .8rem; color: #7A7064;
  }}
  .grid {{
    display: grid;
    grid-template-columns: repeat(auto-fill, minmax(260px, 1fr));
    gap: 18px;
    padding: 20px 32px 40px;
  }}
  .pack-card {{
    background: #FFFFFF;
    border: 1px solid #DDD8D0;
    border-radius: 10px;
    padding: 16px;
    display: flex; flex-direction: column; gap: 10px;
    box-shadow: 0 1px 3px rgba(0,0,0,.06);
    transition: box-shadow .15s;
  }}
  .pack-card:hover {{ box-shadow: 0 4px 12px rgba(0,0,0,.1); }}
  .pack-card.hidden {{ display: none; }}
  .card-header {{
    display: flex; justify-content: space-between; align-items: flex-start; gap: 8px;
  }}
  .pack-name {{ font-size: .95rem; font-weight: 700; line-height: 1.3; flex: 1; }}
  .tier-badge {{
    font-size: .68rem; font-weight: 700; padding: 2px 8px;
    border-radius: 20px; color: #1A1A1A; white-space: nowrap; flex-shrink: 0;
  }}
  .engines-row {{ display: flex; flex-wrap: wrap; gap: 5px; }}
  .engine-badge {{
    font-size: .72rem; font-weight: 700; padding: 3px 8px;
    border-radius: 4px; letter-spacing: .04em;
  }}
  .meta-row {{
    display: flex; gap: 8px; align-items: center; flex-wrap: wrap;
    font-size: .78rem; color: #5A5046;
  }}
  .mood-tag {{
    background: #EFEDE9; padding: 2px 8px; border-radius: 20px;
    font-weight: 600; color: #3A3028;
  }}
  .version {{ color: #9A8E80; }}
  .preset-count {{ font-weight: 600; color: #3A5A4A; }}
  /* Sonic DNA mini chart */
  .dna-chart {{
    display: flex; gap: 4px; align-items: flex-end; height: 44px;
    background: #F5F3EF; border-radius: 6px; padding: 6px 8px 0;
  }}
  .dna-bar-wrap {{
    flex: 1; display: flex; flex-direction: column; align-items: center;
    justify-content: flex-end; height: 100%; cursor: default;
  }}
  .dna-bar {{
    width: 100%; border-radius: 2px 2px 0 0;
    min-height: 2px; transition: height .2s;
  }}
  .dna-label {{
    font-size: .56rem; color: #9A8E80; margin-top: 2px;
    text-transform: uppercase; letter-spacing: .03em;
  }}
  .tags-row {{ display: flex; flex-wrap: wrap; gap: 4px; min-height: 0; }}
  .tag {{
    font-size: .68rem; background: #E8E2D8; color: #5A5046;
    padding: 2px 7px; border-radius: 20px;
  }}
  .filename {{ font-size: .65rem; color: #B8AEA0; font-family: monospace; word-break: break-all; }}
  .no-results {{
    grid-column: 1/-1; text-align: center;
    padding: 60px 20px; color: #9A8E80; font-size: .95rem;
  }}
</style>
</head>
<body>
<header>
  <h1>{title}</h1>
  <p>{pack_count} pack{"s" if pack_count != 1 else ""} &mdash; XO_OX Designs</p>
</header>

<div class="filter-bar">
  <label for="flt-engine">Engine</label>
  <select id="flt-engine">
    <option value="">All Engines</option>
    {engine_options}
  </select>

  <label for="flt-mood">Mood</label>
  <select id="flt-mood">
    <option value="">All Moods</option>
    {mood_options}
  </select>

  <label for="flt-tier">Tier</label>
  <select id="flt-tier">
    <option value="">All Tiers</option>
    {tier_options}
  </select>

  <label for="flt-search">Search</label>
  <input id="flt-search" type="search" placeholder="pack name or tag…" style="min-width:160px">

  <button onclick="clearFilters()">Clear</button>
</div>

<div class="count-bar" id="count-bar">Showing {pack_count} of {pack_count} packs</div>

<div class="grid" id="pack-grid">
{cards_html}
  <div class="no-results hidden" id="no-results">No packs match the current filters.</div>
</div>

<script>
(function () {{
  var grid    = document.getElementById('pack-grid');
  var countEl = document.getElementById('count-bar');
  var noRes   = document.getElementById('no-results');
  var total   = {pack_count};

  function getCards() {{
    return Array.from(grid.querySelectorAll('.pack-card'));
  }}

  function applyFilters() {{
    var engine = document.getElementById('flt-engine').value.toUpperCase();
    var mood   = document.getElementById('flt-mood').value;
    var tier   = document.getElementById('flt-tier').value;
    var search = document.getElementById('flt-search').value.trim().toLowerCase();

    var visible = 0;
    getCards().forEach(function(card) {{
      var engines  = card.dataset.engines || '';
      var cardMood = card.dataset.mood    || '';
      var cardTier = card.dataset.tier    || '';
      var name     = (card.querySelector('.pack-name') || {{}}).textContent || '';
      var tagsText = (card.querySelector('.tags-row') || {{}}).textContent || '';

      var ok = true;
      if (engine && engines.toUpperCase().split(',').indexOf(engine) === -1) ok = false;
      if (mood   && cardMood !== mood)  ok = false;
      if (tier   && cardTier !== tier)  ok = false;
      if (search) {{
        var haystack = (name + ' ' + tagsText + ' ' + engines + ' ' + cardMood).toLowerCase();
        if (haystack.indexOf(search) === -1) ok = false;
      }}

      if (ok) {{ card.classList.remove('hidden'); visible++; }}
      else     card.classList.add('hidden');
    }});

    countEl.textContent = 'Showing ' + visible + ' of ' + total + ' pack' + (total === 1 ? '' : 's');
    noRes.classList.toggle('hidden', visible > 0);
  }}

  ['flt-engine','flt-mood','flt-tier','flt-search'].forEach(function(id) {{
    document.getElementById(id).addEventListener('input', applyFilters);
    document.getElementById(id).addEventListener('change', applyFilters);
  }});

  window.clearFilters = function() {{
    document.getElementById('flt-engine').value = '';
    document.getElementById('flt-mood').value   = '';
    document.getElementById('flt-tier').value   = '';
    document.getElementById('flt-search').value = '';
    applyFilters();
  }};
}})();
</script>
</body>
</html>"""


# ---------------------------------------------------------------------------
# CLI entry point
# ---------------------------------------------------------------------------

def main(argv: Optional[list] = None) -> int:
    parser = argparse.ArgumentParser(
        description="Generate a static HTML pack catalog for a directory of .xpn files."
    )
    parser.add_argument(
        "--packs-dir", required=True,
        help="Directory containing .xpn pack files to scan.",
    )
    parser.add_argument(
        "--output", default=None,
        help="Output HTML file path (default: <packs-dir>/index.html).",
    )
    parser.add_argument(
        "--title", default="XO_OX Pack Catalog",
        help='Page title (default: "XO_OX Pack Catalog").',
    )
    args = parser.parse_args(argv)

    packs_dir = Path(args.packs_dir).expanduser().resolve()
    if not packs_dir.is_dir():
        print(f"ERROR: --packs-dir '{packs_dir}' does not exist or is not a directory.", file=sys.stderr)
        return 1

    output_path = Path(args.output).expanduser().resolve() if args.output else packs_dir / "index.html"

    print(f"Scanning: {packs_dir}")
    packs = scan_packs(packs_dir)
    print(f"Found {len(packs)} .xpn packs")

    html = build_html(packs, args.title)
    output_path.write_text(html, encoding="utf-8")
    print(f"Written:  {output_path}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
