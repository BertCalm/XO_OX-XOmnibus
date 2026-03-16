#!/usr/bin/env python3
"""
XPN Setlist Builder — XO_OX Designs
Set-long kit validation and loading order builder for MPC Live III.

Reads a JSON setlist file listing all packs to be used in a live set.
For each XPN (ZIP), opens it, parses all XPM files, counts WAV references,
and estimates RAM usage. Validates against the target device RAM ceiling.
Generates three output documents: setlist_validation.txt, loading_order.txt,
and swap_guide.txt.

Device profiles:
  MPC_Live_III  — 2800 MB practical ceiling
  MPC_X         — 2400 MB practical ceiling
  MPC_One       — 1200 MB practical ceiling

RAM estimation strategy:
  - Parse WAV paths from XPM <File> / <SampleFile> elements.
  - If the WAV is present inside the XPN ZIP, use its actual compressed size
    × decompression ratio estimate (see WAV_INFLATE_RATIO).
  - If the WAV is absent (stub reference), estimate from a 2-second stereo
    WAV at 44100 Hz / 16-bit: 44100 × 2ch × 2bytes × 2s ≈ 352 KB per sample.
  - Program overhead: 2 MB per XPM program (MPC program table + voice state).
  - Total per pack: sum(sample_estimates) + (num_programs × PROGRAM_OVERHEAD_MB).

Setlist JSON format:
    {
        "device": "MPC_Live_III",
        "packs": [
            {
                "name": "ONSET_Foundation",
                "path": "./ONSET_Foundation.xpn",
                "programs": 3
            }
        ]
    }

Usage:
    python xpn_setlist_builder.py \\
        --setlist my_set.json \\
        --device MPC_Live_III \\
        --output ./out/

Output files in ./out/:
    setlist_validation.txt   — fit/fail verdict per pack, total RAM
    loading_order.txt        — recommended load sequence
    swap_guide.txt           — live swap instructions if set exceeds RAM
"""

import argparse
import json
import os
import sys
import zipfile
from pathlib import Path
from xml.etree import ElementTree as ET


# =============================================================================
# DEVICE PROFILES
# =============================================================================

# Practical RAM ceilings per device (in MB).
# "Practical" = hardware RAM minus OS + MPC firmware overhead.
DEVICE_PROFILES = {
    "MPC_Live_III": 2800,
    "MPC_X":        2400,
    "MPC_One":      1200,
}

DEVICE_ALIASES = {
    "live3":        "MPC_Live_III",
    "live_iii":     "MPC_Live_III",
    "mpc_live_iii": "MPC_Live_III",
    "mpc_live3":    "MPC_Live_III",
    "x":            "MPC_X",
    "mpc_x":        "MPC_X",
    "one":          "MPC_One",
    "mpc_one":      "MPC_One",
}


def resolve_device(name: str) -> str:
    """Resolve device name (case-insensitive, with aliases) to canonical form."""
    canon = name.strip()
    if canon in DEVICE_PROFILES:
        return canon
    lower = canon.lower().replace("-", "_").replace(" ", "_")
    if lower in DEVICE_ALIASES:
        return DEVICE_ALIASES[lower]
    if lower in {k.lower() for k in DEVICE_PROFILES}:
        for k in DEVICE_PROFILES:
            if k.lower() == lower:
                return k
    return canon  # return as-is; validation will fail later with a clear message


def device_ram_mb(device: str) -> int:
    """Return practical RAM ceiling in MB for the given device name."""
    return DEVICE_PROFILES.get(device, 0)


# =============================================================================
# RAM ESTIMATION CONSTANTS
# =============================================================================

# WAVs stored inside the XPN ZIP are DEFLATE-compressed. PCM audio compresses
# poorly — typical ratio is 1.05–1.15× (almost no compression for random audio).
# We use 1.08× as a conservative inflate estimate.
WAV_INFLATE_RATIO = 1.08

# Default sample RAM estimate when a WAV is referenced but not embedded.
# Based on: 44100 Hz × 2 channels × 2 bytes/sample × 2 seconds = 352,800 bytes.
DEFAULT_SAMPLE_BYTES = 44100 * 2 * 2 * 2   # ≈ 352 KB

# Per-program RAM overhead: program table, voice state, envelope state, etc.
# MPC documentation doesn't publish this; 2 MB is a conservative real-world
# estimate based on community measurements of Live II/III behaviour.
PROGRAM_OVERHEAD_MB = 2.0

# Safety headroom to keep below the ceiling (percent of ceiling).
# Headroom prevents MPC's internal memory manager from thrashing.
HEADROOM_PCT = 0.05   # 5%


# =============================================================================
# XPM PARSING
# =============================================================================

def _extract_wav_refs_from_xpm_xml(xml_text: str) -> list:
    """
    Parse XPM XML text and return a list of all unique WAV file references.
    Checks both <File> and <SampleFile> elements under <Layer> blocks.
    Returns paths as strings exactly as they appear in the XPM.
    """
    refs = set()
    try:
        root = ET.fromstring(xml_text)
    except ET.ParseError:
        return []

    for layer in root.iter("Layer"):
        for tag in ("File", "SampleFile", "SampleName"):
            el = layer.find(tag)
            if el is not None and el.text:
                val = el.text.strip()
                if val and (val.lower().endswith(".wav") or "Samples/" in val):
                    refs.add(val)

    return list(refs)


# =============================================================================
# PACK ANALYSIS
# =============================================================================

class PackAnalysis:
    """Result of analysing a single XPN pack."""

    def __init__(self, name: str, path: str):
        self.name: str = name
        self.path: str = path
        self.error: str = ""               # non-empty if pack could not be opened
        self.num_programs: int = 0
        self.num_wav_refs: int = 0         # total WAV references across all XPMs
        self.num_unique_wavs: int = 0      # deduplicated
        self.embedded_wav_bytes: int = 0   # bytes of WAVs actually inside the ZIP
        self.stub_wav_count: int = 0       # WAV refs with no embedded file
        self.estimated_ram_mb: float = 0.0
        self.program_names: list = []


def analyse_pack(pack_entry: dict, setlist_dir: Path) -> PackAnalysis:
    """
    Open an XPN file (ZIP), parse its XPM programs, and estimate RAM usage.

    pack_entry: dict with keys: name, path, programs (optional hint)
    setlist_dir: directory containing the setlist JSON (for resolving relative paths)
    """
    name = pack_entry.get("name", "unknown")
    raw_path = pack_entry.get("path", "")
    result = PackAnalysis(name, raw_path)

    # Resolve the path relative to the setlist file's directory
    pack_path = Path(raw_path)
    if not pack_path.is_absolute():
        pack_path = setlist_dir / pack_path

    if not pack_path.exists():
        result.error = f"File not found: {pack_path}"
        return result

    if not zipfile.is_zipfile(str(pack_path)):
        result.error = f"Not a valid ZIP/XPN file: {pack_path}"
        return result

    try:
        with zipfile.ZipFile(str(pack_path), "r") as zf:
            namelist = zf.namelist()

            # Find all XPM files in the archive
            xpm_names = [n for n in namelist if n.lower().endswith(".xpm")]
            result.num_programs = len(xpm_names)

            # Build a set of embedded WAV paths (lower-cased for matching)
            embedded_wavs = {}  # lower_name → compressed_size
            for info in zf.infolist():
                lower = info.filename.lower()
                if lower.endswith(".wav"):
                    embedded_wavs[lower] = info.compress_size

            all_wav_refs = set()

            for xpm_name in xpm_names:
                try:
                    xpm_bytes = zf.read(xpm_name)
                    xpm_text = xpm_bytes.decode("utf-8", errors="replace")
                    result.program_names.append(
                        Path(xpm_name).stem
                    )
                    wav_refs = _extract_wav_refs_from_xpm_xml(xpm_text)
                    for ref in wav_refs:
                        all_wav_refs.add(ref)
                except (KeyError, UnicodeDecodeError):
                    continue

            result.num_wav_refs = len(all_wav_refs)
            result.num_unique_wavs = len(all_wav_refs)

            # Estimate RAM for each unique WAV reference
            total_sample_bytes = 0
            stub_count = 0

            for ref in all_wav_refs:
                # Try to find in embedded WAVs (normalise path separators)
                ref_lower = ref.lower().replace("\\", "/")
                ref_basename = Path(ref_lower).name

                # First: try exact match
                matched_size = None
                if ref_lower in embedded_wavs:
                    matched_size = embedded_wavs[ref_lower]
                else:
                    # Try basename match (samples may be in Samples/ subdirs)
                    for emb_path, emb_size in embedded_wavs.items():
                        if Path(emb_path).name == ref_basename:
                            matched_size = emb_size
                            break

                if matched_size is not None:
                    # Inflate compressed size to approximate decompressed WAV size
                    inflated = int(matched_size * WAV_INFLATE_RATIO)
                    total_sample_bytes += inflated
                    result.embedded_wav_bytes += matched_size
                else:
                    # No embedded WAV — use stub estimate
                    total_sample_bytes += DEFAULT_SAMPLE_BYTES
                    stub_count += 1

            result.stub_wav_count = stub_count

            # Total RAM: sample data + per-program overhead
            num_programs = result.num_programs or pack_entry.get("programs", 1)
            program_overhead = num_programs * PROGRAM_OVERHEAD_MB * 1024 * 1024

            total_bytes = total_sample_bytes + program_overhead
            result.estimated_ram_mb = total_bytes / (1024 * 1024)

    except zipfile.BadZipFile as exc:
        result.error = f"Bad ZIP file: {exc}"

    return result


# =============================================================================
# SETLIST VALIDATION
# =============================================================================

def generate_setlist_validation(
    analyses: list,
    device: str,
    ram_ceiling_mb: int,
    headroom_mb: float,
) -> str:
    """Generate setlist_validation.txt content."""
    total_ram = sum(a.estimated_ram_mb for a in analyses if not a.error)
    fits = (total_ram <= (ram_ceiling_mb - headroom_mb))

    lines = [
        "XPN Setlist Builder — Validation Report",
        f"XO_OX Designs | {_today()}",
        "",
        f"Device         : {device}",
        f"RAM ceiling    : {ram_ceiling_mb} MB",
        f"Safety headroom: {headroom_mb:.0f} MB ({HEADROOM_PCT*100:.0f}% of ceiling)",
        f"Available RAM  : {ram_ceiling_mb - headroom_mb:.0f} MB",
        "",
        "=" * 60,
        "PACK ANALYSIS",
        "=" * 60,
        "",
    ]

    for a in analyses:
        if a.error:
            lines.append(f"[ERROR] {a.name}")
            lines.append(f"        Path  : {a.path}")
            lines.append(f"        Error : {a.error}")
            lines.append("")
            continue

        status = "OK" if a.estimated_ram_mb <= (ram_ceiling_mb - headroom_mb) else "LARGE"
        lines.append(f"[{status:5s}] {a.name}")
        lines.append(f"        Path          : {a.path}")
        lines.append(f"        Programs      : {a.num_programs}")
        lines.append(f"        Unique WAVs   : {a.num_unique_wavs}")
        lines.append(f"        Stub WAVs     : {a.stub_wav_count}  (estimated @ {DEFAULT_SAMPLE_BYTES // 1024} KB each)")
        lines.append(f"        Estimated RAM : {a.estimated_ram_mb:.1f} MB")
        if a.program_names:
            prog_list = ", ".join(a.program_names[:8])
            if len(a.program_names) > 8:
                prog_list += f", ... (+{len(a.program_names) - 8} more)"
            lines.append(f"        Programs      : {prog_list}")
        lines.append("")

    lines += [
        "=" * 60,
        "SUMMARY",
        "=" * 60,
        "",
        f"Total estimated RAM : {total_ram:.1f} MB",
        f"Available RAM       : {ram_ceiling_mb - headroom_mb:.0f} MB",
        f"Remaining headroom  : {(ram_ceiling_mb - headroom_mb) - total_ram:.1f} MB",
        "",
    ]

    if fits:
        lines.append("VERDICT: SET FITS IN MEMORY — all packs can be loaded simultaneously.")
    else:
        over = total_ram - (ram_ceiling_mb - headroom_mb)
        lines.append(f"VERDICT: SET EXCEEDS RAM BY {over:.1f} MB — see swap_guide.txt.")

    lines += [
        "",
        "Notes:",
        "  - RAM estimates are approximate. Actual MPC usage depends on sample",
        "    format, bit depth, and MPC firmware version.",
        "  - Stub WAV estimates assume 2-second stereo 44.1 kHz / 16-bit samples.",
        "  - Program overhead estimated at 2 MB per XPM program.",
        "  - Run with embedded WAVs in XPN files for more accurate estimates.",
    ]

    return "\n".join(lines) + "\n"


# =============================================================================
# LOADING ORDER
# =============================================================================

def generate_loading_order(
    analyses: list,
    device: str,
    ram_ceiling_mb: int,
    headroom_mb: float,
) -> str:
    """
    Generate loading_order.txt.
    Strategy: largest packs first — MPC caches warm early, smaller packs
    fill remaining space, reducing total swap events during the set.
    Packs with errors are listed last with a warning.
    """
    good = [a for a in analyses if not a.error]
    errors = [a for a in analyses if a.error]

    # Sort descending by RAM (largest first)
    sorted_packs = sorted(good, key=lambda a: a.estimated_ram_mb, reverse=True)

    lines = [
        "XPN Setlist Builder — Recommended Loading Order",
        f"XO_OX Designs | {_today()}",
        "",
        f"Device : {device}",
        f"Strategy: Largest packs first — fills MPC cache efficiently.",
        "",
        "=" * 60,
        "LOAD SEQUENCE",
        "=" * 60,
        "",
    ]

    cumulative = 0.0
    available = ram_ceiling_mb - headroom_mb

    for i, a in enumerate(sorted_packs, 1):
        cumulative += a.estimated_ram_mb
        over = max(0.0, cumulative - available)
        status = "OK" if over == 0 else f"OVER by {over:.1f} MB"
        lines.append(f"  {i:2d}. {a.name}")
        lines.append(f"       RAM: {a.estimated_ram_mb:.1f} MB  |  "
                     f"Cumulative: {cumulative:.1f} MB / {available:.0f} MB  [{status}]")
        lines.append(f"       Path: {a.path}")
        lines.append("")

    if errors:
        lines += [
            "=" * 60,
            "PACKS WITH ERRORS (excluded from load order)",
            "=" * 60,
            "",
        ]
        for a in errors:
            lines.append(f"  ! {a.name}")
            lines.append(f"    {a.error}")
            lines.append("")

    lines += [
        "MPC Live loading tips:",
        "  1. Load packs via Expansion Manager before the set starts.",
        "  2. Load in the order above — largest programs load first.",
        "  3. Keep swap_guide.txt handy if total RAM exceeds the ceiling.",
    ]

    return "\n".join(lines) + "\n"


# =============================================================================
# SWAP GUIDE
# =============================================================================

def generate_swap_guide(
    analyses: list,
    device: str,
    ram_ceiling_mb: int,
    headroom_mb: float,
    original_order: list,
) -> str:
    """
    Generate swap_guide.txt.
    If the full set exceeds RAM, suggest which packs to swap in/out
    at which point during the set.

    Strategy:
      - Divide the set into time slots (equal thirds: early, middle, late).
      - Greedily assign packs to time slots respecting the RAM ceiling.
      - When a new pack can't fit, evict the pack that was last used earliest.
    """
    good = [a for a in analyses if not a.error]
    available = ram_ceiling_mb - headroom_mb
    total_ram = sum(a.estimated_ram_mb for a in good)

    lines = [
        "XPN Setlist Builder — Live Swap Guide",
        f"XO_OX Designs | {_today()}",
        "",
        f"Device          : {device}",
        f"Available RAM   : {available:.0f} MB",
        f"Total set RAM   : {total_ram:.1f} MB",
        "",
    ]

    if total_ram <= available:
        lines += [
            "SET FITS IN MEMORY.",
            "No swaps required — all packs can be loaded simultaneously.",
            "",
            "Load order is listed in loading_order.txt.",
        ]
        return "\n".join(lines) + "\n"

    over = total_ram - available
    lines += [
        f"SET EXCEEDS RAM BY {over:.1f} MB.",
        "Swap strategy: load next pack, unload least-recently-needed pack.",
        "",
        "=" * 60,
        "SWAP PLAN",
        "=" * 60,
        "",
    ]

    # Simple greedy swap: maintain a "loaded" set constrained by available RAM.
    # Process packs in the original setlist order (preserving artistic intent).
    # When we can't fit a new pack, evict the pack earliest in the loaded order.

    loaded: list = []          # list of PackAnalysis, in order loaded
    loaded_ram: float = 0.0
    events: list = []          # list of (pack_name, action, cumulative_ram, note)

    for pack in original_order:
        if pack.error:
            events.append((pack.name, "SKIP", loaded_ram, f"Error: {pack.error}"))
            continue

        # Try to load without eviction
        if loaded_ram + pack.estimated_ram_mb <= available:
            loaded.append(pack)
            loaded_ram += pack.estimated_ram_mb
            events.append((pack.name, "LOAD", loaded_ram,
                           f"Fits. RAM now {loaded_ram:.1f}/{available:.0f} MB."))
        else:
            # Must evict. Evict the pack loaded earliest (FIFO = least likely to be needed soon)
            evicted = loaded.pop(0)
            loaded_ram -= evicted.estimated_ram_mb
            events.append((evicted.name, "UNLOAD", loaded_ram,
                           f"Evicted to make room for {pack.name}."))

            # Now load the new pack
            loaded.append(pack)
            loaded_ram += pack.estimated_ram_mb
            events.append((pack.name, "LOAD", loaded_ram,
                           f"RAM now {loaded_ram:.1f}/{available:.0f} MB."))

    # Format events as numbered steps
    step = 1
    for pack_name, action, cum_ram, note in events:
        symbol = "+" if action == "LOAD" else ("-" if action == "UNLOAD" else "!")
        lines.append(f"  Step {step:2d}. [{symbol}] {action:6s}  {pack_name}")
        lines.append(f"           {note}")
        lines.append(f"           RAM in use after step: {cum_ram:.1f} MB")
        lines.append("")
        step += 1

    lines += [
        "=" * 60,
        "LIVE SWAP WORKFLOW",
        "=" * 60,
        "",
        "Before the set:",
        "  1. Load all packs that appear in the first 3 set slots.",
        "  2. Pre-cue swaps at natural set breaks (verse→chorus, song end).",
        "",
        "During the set:",
        "  - Follow the step list above.",
        "  - Unload BEFORE loading the next pack to avoid RAM overflow.",
        "  - MPC Live III: use Browse → Expansions → Unload Pack.",
        "",
        "Preventing audible gaps:",
        "  - Swap during a breakdown, outro, or DJ transition.",
        "  - Do NOT swap while the program you're evicting is actively playing.",
        "",
        "Alternative — reduce RAM usage:",
        "  - Use xpn_drum_export.py to rebuild packs at lower sample count.",
        "  - Reduce programs per pack (--programs flag) to trim overhead.",
    ]

    return "\n".join(lines) + "\n"


# =============================================================================
# UTILITIES
# =============================================================================

def _today() -> str:
    import datetime
    return str(datetime.date.today())


def _load_setlist(path: Path) -> dict:
    """Load and minimally validate the setlist JSON file."""
    try:
        with open(path, encoding="utf-8") as f:
            data = json.load(f)
    except FileNotFoundError:
        print(f"[ERROR] Setlist file not found: {path}", file=sys.stderr)
        sys.exit(1)
    except json.JSONDecodeError as exc:
        print(f"[ERROR] Invalid JSON in setlist: {exc}", file=sys.stderr)
        sys.exit(1)

    if "packs" not in data:
        print("[ERROR] Setlist JSON must have a 'packs' array.", file=sys.stderr)
        sys.exit(1)

    if not isinstance(data["packs"], list):
        print("[ERROR] 'packs' must be a JSON array.", file=sys.stderr)
        sys.exit(1)

    return data


# =============================================================================
# MAIN
# =============================================================================

def main():
    parser = argparse.ArgumentParser(
        description="XPN Setlist Builder — validate and plan loading order for MPC Live sets.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    parser.add_argument("--setlist", required=True, metavar="SETLIST.json",
                        help="Path to the setlist JSON file.")
    parser.add_argument("--device", default="", metavar="DEVICE",
                        help=(
                            "Target MPC device. Choices: "
                            + ", ".join(DEVICE_PROFILES.keys())
                            + ". Overrides 'device' field in setlist JSON."
                        ))
    parser.add_argument("--output", default="./out", metavar="DIR",
                        help="Output directory for report files (default: ./out).")
    parser.add_argument("--dry-run", action="store_true",
                        help="Analyse packs and print summary without writing files.")
    args = parser.parse_args()

    setlist_path = Path(args.setlist)
    output_dir = Path(args.output)
    setlist_dir = setlist_path.parent.resolve()

    # Load setlist
    setlist_data = _load_setlist(setlist_path)

    # Resolve device
    device_raw = args.device or setlist_data.get("device", "MPC_Live_III")
    device = resolve_device(device_raw)
    if device not in DEVICE_PROFILES:
        known = ", ".join(DEVICE_PROFILES.keys())
        print(f"[ERROR] Unknown device '{device}'. Known devices: {known}", file=sys.stderr)
        sys.exit(1)

    ram_ceiling_mb = device_ram_mb(device)
    headroom_mb = ram_ceiling_mb * HEADROOM_PCT

    packs = setlist_data["packs"]
    if not packs:
        print("[ERROR] Setlist has no packs.", file=sys.stderr)
        sys.exit(1)

    print(f"XPN Setlist Builder — XO_OX Designs")
    print(f"  Setlist : {setlist_path}")
    print(f"  Device  : {device}  ({ram_ceiling_mb} MB ceiling)")
    print(f"  Packs   : {len(packs)}")
    print(f"  Output  : {output_dir}")
    print()

    # Analyse all packs
    analyses_in_order = []
    for pack_entry in packs:
        name = pack_entry.get("name", pack_entry.get("path", "unknown"))
        print(f"  Analysing: {name} ...")
        analysis = analyse_pack(pack_entry, setlist_dir)
        analyses_in_order.append(analysis)
        if analysis.error:
            print(f"    [ERROR] {analysis.error}")
        else:
            print(f"    Programs: {analysis.num_programs}  "
                  f"WAVs: {analysis.num_unique_wavs}  "
                  f"RAM: {analysis.estimated_ram_mb:.1f} MB")

    print()
    total_ram = sum(a.estimated_ram_mb for a in analyses_in_order if not a.error)
    available = ram_ceiling_mb - headroom_mb
    fits = (total_ram <= available)
    print(f"Total estimated RAM : {total_ram:.1f} MB / {available:.0f} MB available")
    print(f"Verdict: {'FITS' if fits else 'EXCEEDS RAM — swap guide generated'}")
    print()

    if args.dry_run:
        print("[DRY RUN] Would write:")
        print(f"  {output_dir}/setlist_validation.txt")
        print(f"  {output_dir}/loading_order.txt")
        print(f"  {output_dir}/swap_guide.txt")
        return

    # Write output files
    output_dir.mkdir(parents=True, exist_ok=True)

    validation_txt = generate_setlist_validation(
        analyses_in_order, device, ram_ceiling_mb, headroom_mb
    )
    validation_path = output_dir / "setlist_validation.txt"
    validation_path.write_text(validation_txt, encoding="utf-8")
    print(f"  Written: {validation_path}")

    loading_txt = generate_loading_order(
        analyses_in_order, device, ram_ceiling_mb, headroom_mb
    )
    loading_path = output_dir / "loading_order.txt"
    loading_path.write_text(loading_txt, encoding="utf-8")
    print(f"  Written: {loading_path}")

    swap_txt = generate_swap_guide(
        analyses_in_order, device, ram_ceiling_mb, headroom_mb,
        original_order=analyses_in_order,
    )
    swap_path = output_dir / "swap_guide.txt"
    swap_path.write_text(swap_txt, encoding="utf-8")
    print(f"  Written: {swap_path}")

    print()
    print(f"Done. Reports written to {output_dir}/")


if __name__ == "__main__":
    main()
