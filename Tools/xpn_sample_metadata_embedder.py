#!/usr/bin/env python3
"""
xpn_sample_metadata_embedder.py — XO_OX XPN Tool Suite

Embeds metadata into WAV files using the RIFF LIST/INFO chunk standard.
MPC and DAW users see meaningful tags (name, artist, pack, genre, comment, date)
without any modification to the audio data.

Usage:
  python xpn_sample_metadata_embedder.py input.wav --name "Kick 808" --pack "XObese"
  python xpn_sample_metadata_embedder.py --batch ./samples/ --pack "XObese" --artist "XO_OX"
  python xpn_sample_metadata_embedder.py input.wav --meta metadata.json
  python xpn_sample_metadata_embedder.py input.wav --name "Kick" --dry-run
"""

import argparse
import json
import struct
import sys
from datetime import datetime
from pathlib import Path
from typing import Dict, List, Optional, Tuple


# RIFF INFO field IDs (4-byte FourCC codes)
INFO_FIELDS = {
    "name":    b"INAM",  # Sample/clip name
    "artist":  b"IART",  # Artist / creator
    "pack":    b"IPRD",  # Product / pack name
    "genre":   b"IGNR",  # Genre
    "comment": b"ICMT",  # Comment / description
    "date":    b"ICRD",  # Creation date (ISO 8601 recommended)
}


# ---------------------------------------------------------------------------
# WAV parsing
# ---------------------------------------------------------------------------

def _read_chunks(data: bytes) -> Dict[bytes, List[Tuple[int, int]]]:
    """
    Parse a RIFF/WAVE file and return a map of FourCC -> list of (offset, size)
    tuples for every top-level chunk found inside the WAVE container.
    The special key b'RIFF' maps to a single entry with the full file extent.
    Raises ValueError on structural problems.
    """
    if len(data) < 12:
        raise ValueError("File too small to be a valid WAV.")
    riff_id, riff_size, wave_id = struct.unpack_from("<4sI4s", data, 0)
    if riff_id != b"RIFF":
        raise ValueError(f"Not a RIFF file (got {riff_id!r}).")
    if wave_id != b"WAVE":
        raise ValueError(f"RIFF type is not WAVE (got {wave_id!r}).")

    chunks: Dict[bytes, List[Tuple[int, int]]] = {}
    offset = 12
    end = min(8 + riff_size, len(data))

    while offset + 8 <= end:
        chunk_id, chunk_size = struct.unpack_from("<4sI", data, offset)
        chunks.setdefault(chunk_id, []).append((offset, chunk_size))
        # Chunk data is padded to even byte boundary
        offset += 8 + chunk_size + (chunk_size & 1)

    return chunks


def validate_wav(path: Path) -> bytes:
    """Read and validate a WAV file. Returns raw bytes. Raises on error."""
    raw = path.read_bytes()
    _read_chunks(raw)  # raises ValueError on bad structure
    return raw


# ---------------------------------------------------------------------------
# LIST/INFO chunk building
# ---------------------------------------------------------------------------

def _pack_info_string(value: str) -> bytes:
    """Encode a metadata string as null-terminated bytes, padded to even length."""
    encoded = value.encode("utf-8", errors="replace") + b"\x00"
    if len(encoded) & 1:
        encoded += b"\x00"  # pad to even
    return encoded


def build_info_chunk(fields: dict[str, str]) -> bytes:
    """
    Build a complete LIST/INFO chunk from a dict of {field_name: value}.
    field_name keys must be in INFO_FIELDS.
    Returns the raw bytes ready to splice into the RIFF container.
    """
    info_body = b""
    for field_name, value in fields.items():
        if not value:
            continue
        fourcc = INFO_FIELDS.get(field_name)
        if fourcc is None:
            continue
        encoded = _pack_info_string(value)
        # sub-chunk: FourCC(4) + size(4) + data
        info_body += fourcc + struct.pack("<I", len(encoded)) + encoded

    if not info_body:
        return b""

    # LIST chunk: b"LIST" + size(4) + b"INFO" + body
    list_body = b"INFO" + info_body
    chunk = b"LIST" + struct.pack("<I", len(list_body)) + list_body
    # Pad to even (list_body already built from even sub-chunks, so usually fine)
    if len(chunk) & 1:
        chunk += b"\x00"
    return chunk


# ---------------------------------------------------------------------------
# Metadata diff / display
# ---------------------------------------------------------------------------

def parse_existing_info(data: bytes) -> Dict[str, str]:
    """Extract existing LIST/INFO metadata from WAV bytes. Returns field->value map."""
    result: Dict[str, str] = {}
    try:
        chunks = _read_chunks(data)
    except ValueError:
        return result

    for (offset, size) in chunks.get(b"LIST", []):
        list_data_start = offset + 8
        if list_data_start + 4 > len(data):
            continue
        list_type = data[list_data_start : list_data_start + 4]
        if list_type != b"INFO":
            continue
        sub_offset = list_data_start + 4
        sub_end = list_data_start + size
        while sub_offset + 8 <= sub_end and sub_offset + 8 <= len(data):
            sub_id = data[sub_offset : sub_offset + 4]
            sub_size = struct.unpack_from("<I", data, sub_offset + 4)[0]
            sub_data = data[sub_offset + 8 : sub_offset + 8 + sub_size]
            value = sub_data.rstrip(b"\x00").decode("utf-8", errors="replace")
            for field_name, fourcc in INFO_FIELDS.items():
                if sub_id == fourcc:
                    result[field_name] = value
            sub_offset += 8 + sub_size + (sub_size & 1)
    return result


def describe_changes(existing: Dict[str, str], incoming: Dict[str, str]) -> List[str]:
    """Return human-readable lines describing what would change."""
    lines = []
    all_keys = set(list(existing.keys()) + list(incoming.keys()))
    for key in sorted(all_keys):
        old_val = existing.get(key, "")
        new_val = incoming.get(key, "")
        if not new_val:
            continue
        if old_val == new_val:
            lines.append(f"  {key:10s}  (unchanged) {old_val!r}")
        elif old_val:
            lines.append(f"  {key:10s}  {old_val!r}  →  {new_val!r}")
        else:
            lines.append(f"  {key:10s}  (new) {new_val!r}")
    return lines


# ---------------------------------------------------------------------------
# Core embed logic
# ---------------------------------------------------------------------------

def embed_metadata(
    raw: bytes,
    fields: Dict[str, str],
    *,
    dry_run: bool = False,
    verbose: bool = True,
) -> Optional[bytes]:
    """
    Rebuild the WAV with updated LIST/INFO chunk. Returns new bytes, or None
    on dry-run. Preserves all non-LIST chunks (fmt, data, smpl, etc.) verbatim.
    """
    existing = parse_existing_info(raw)

    # Merge: incoming fields override existing; blank incoming values keep existing
    merged: Dict[str, str] = dict(existing)
    for k, v in fields.items():
        if v:
            merged[k] = v

    if verbose:
        changes = describe_changes(existing, merged)
        if changes:
            print("  Metadata changes:")
            for line in changes:
                print(line)
        else:
            print("  No metadata changes.")

    if dry_run:
        return None

    # Build new LIST/INFO chunk
    new_info_chunk = build_info_chunk(merged)

    # Collect all chunks except LIST (we'll re-add a fresh one)
    chunks = _read_chunks(raw)
    preserved_chunks = b""
    for chunk_id, locations in chunks.items():
        if chunk_id == b"LIST":
            continue  # drop old LIST chunks
        for (offset, size) in locations:
            chunk_bytes = raw[offset : offset + 8 + size + (size & 1)]
            preserved_chunks += chunk_bytes

    # Reassemble: WAVE header + preserved chunks + new INFO chunk
    wave_body = preserved_chunks + new_info_chunk
    riff_size = 4 + len(wave_body)  # 4 for "WAVE" type
    header = b"RIFF" + struct.pack("<I", riff_size) + b"WAVE"
    return header + wave_body


# ---------------------------------------------------------------------------
# Metadata loading helpers
# ---------------------------------------------------------------------------

def load_meta_json(path: Path) -> Dict[str, str]:
    """Load metadata from a JSON sidecar file. Keys must match INFO_FIELDS names."""
    with open(path, "r", encoding="utf-8") as f:
        data = json.load(f)
    valid_keys = set(INFO_FIELDS.keys())
    result = {}
    for k, v in data.items():
        if k in valid_keys and isinstance(v, str):
            result[k] = v
        else:
            print(f"  Warning: ignoring unknown/invalid JSON key '{k}'")
    return result


def build_fields_from_args(args: argparse.Namespace) -> Dict[str, str]:
    """Construct the metadata dict from CLI args and optional JSON sidecar."""
    fields: Dict[str, str] = {}

    # JSON sidecar takes base values
    if args.meta:
        meta_path = Path(args.meta)
        if not meta_path.exists():
            print(f"Error: --meta file not found: {meta_path}", file=sys.stderr)
            sys.exit(1)
        fields.update(load_meta_json(meta_path))

    # CLI flags override JSON sidecar
    if args.name:
        fields["name"] = args.name
    if args.artist:
        fields["artist"] = args.artist
    if args.pack:
        fields["pack"] = args.pack
    if args.genre:
        fields["genre"] = args.genre
    if args.comment:
        fields["comment"] = args.comment
    if args.date:
        fields["date"] = args.date
    elif "date" not in fields:
        # Auto-stamp today if no date provided at all
        fields["date"] = datetime.today().strftime("%Y-%m-%d")

    return fields


# ---------------------------------------------------------------------------
# Single-file processing
# ---------------------------------------------------------------------------

def process_file(
    input_path: Path,
    output_path: Path,
    fields: Dict[str, str],
    *,
    dry_run: bool,
) -> bool:
    """
    Validate, embed metadata, and write output WAV.
    Returns True on success (or dry-run success), False on error.
    """
    print(f"\n{'[DRY-RUN] ' if dry_run else ''}Processing: {input_path}")

    try:
        raw = validate_wav(input_path)
    except (ValueError, OSError) as e:
        print(f"  ERROR: {e}")
        return False

    result = embed_metadata(raw, fields, dry_run=dry_run, verbose=True)

    if dry_run:
        print("  Dry-run complete — no files written.")
        return True

    if result is None:
        return False  # embed returned None unexpectedly

    try:
        output_path.parent.mkdir(parents=True, exist_ok=True)
        output_path.write_bytes(result)
        print(f"  Written: {output_path}  ({len(result):,} bytes)")
        return True
    except OSError as e:
        print(f"  ERROR writing output: {e}")
        return False


# ---------------------------------------------------------------------------
# Batch mode
# ---------------------------------------------------------------------------

def process_batch(
    batch_dir: Path,
    fields: Dict[str, str],
    *,
    dry_run: bool,
    output_dir: Optional[Path],
) -> None:
    """Process all .wav files in batch_dir (recursive)."""
    wav_files = sorted(batch_dir.rglob("*.wav"))
    if not wav_files:
        print(f"No .wav files found in {batch_dir}")
        return

    print(f"Batch mode: {len(wav_files)} WAV file(s) in {batch_dir}")
    success = fail = 0

    for wav_path in wav_files:
        if output_dir:
            rel = wav_path.relative_to(batch_dir)
            out_path = output_dir / rel
        else:
            out_path = wav_path  # in-place

        ok = process_file(wav_path, out_path, fields, dry_run=dry_run)
        if ok:
            success += 1
        else:
            fail += 1

    print(f"\nBatch complete: {success} succeeded, {fail} failed.")


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def build_parser() -> argparse.ArgumentParser:
    p = argparse.ArgumentParser(
        description=(
            "Embed RIFF LIST/INFO metadata into WAV files for XPN bundle packaging.\n"
            "Preserves audio data (fmt + data chunks) exactly."
        ),
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  # Tag a single file in-place:
  python xpn_sample_metadata_embedder.py kick.wav --name "Kick 808" --pack "XObese"

  # Write to a new file:
  python xpn_sample_metadata_embedder.py kick.wav --name "Kick" --output kick_tagged.wav

  # Use a JSON sidecar (CLI flags override JSON values):
  python xpn_sample_metadata_embedder.py kick.wav --meta meta.json --name "Override Name"

  # Batch-tag a directory of samples (in-place):
  python xpn_sample_metadata_embedder.py --batch ./samples/ --pack "XObese" --artist "XO_OX"

  # Batch with output directory:
  python xpn_sample_metadata_embedder.py --batch ./samples/ --pack "XObese" --output ./tagged/

  # Dry-run (preview changes, no writes):
  python xpn_sample_metadata_embedder.py kick.wav --name "Kick" --dry-run

JSON sidecar format (metadata.json):
  {
    "name": "Kick 808",
    "artist": "XO_OX",
    "pack": "XObese",
    "genre": "Electronic",
    "comment": "Deep 808 with sub resonance",
    "date": "2026-03-16"
  }
""",
    )

    # Positional (optional — not required in batch mode)
    p.add_argument(
        "input",
        nargs="?",
        metavar="INPUT.WAV",
        help="Input WAV file (omit when using --batch).",
    )

    # Metadata flags
    meta = p.add_argument_group("Metadata fields")
    meta.add_argument("--name",    metavar="STR", help="Sample name (INAM)")
    meta.add_argument("--artist",  metavar="STR", default="XO_OX", help="Artist/creator (IART) [default: XO_OX]")
    meta.add_argument("--pack",    metavar="STR", help="Pack/product name (IPRD)")
    meta.add_argument("--genre",   metavar="STR", help="Genre (IGNR)")
    meta.add_argument("--comment", metavar="STR", help="Comment (ICMT)")
    meta.add_argument("--date",    metavar="YYYY-MM-DD", help="Creation date (ICRD) [default: today]")
    meta.add_argument("--meta",    metavar="FILE", help="JSON sidecar file with metadata fields")

    # Mode flags
    p.add_argument("--batch",   metavar="DIR",  help="Process all .wav files in DIR (recursive)")
    p.add_argument("--output",  metavar="PATH", help="Output file (single) or directory (batch). Default: in-place.")
    p.add_argument("--dry-run", action="store_true", help="Preview changes without writing any files")

    return p


def main() -> None:
    parser = build_parser()
    args = parser.parse_args()

    # Validate argument combinations
    if not args.batch and not args.input:
        parser.error("Specify an INPUT.WAV file or use --batch <dir>.")
    if args.batch and args.input:
        parser.error("Cannot use both INPUT.WAV and --batch together.")

    fields = build_fields_from_args(args)

    if not any(fields.values()):
        print("Warning: no metadata fields provided — nothing to embed.")
        sys.exit(0)

    # ---- Batch mode ----
    if args.batch:
        batch_dir = Path(args.batch)
        if not batch_dir.is_dir():
            print(f"Error: --batch path is not a directory: {batch_dir}", file=sys.stderr)
            sys.exit(1)
        output_dir = Path(args.output) if args.output else None
        process_batch(batch_dir, fields, dry_run=args.dry_run, output_dir=output_dir)
        return

    # ---- Single-file mode ----
    input_path = Path(args.input)
    if not input_path.exists():
        print(f"Error: input file not found: {input_path}", file=sys.stderr)
        sys.exit(1)

    if args.output:
        out = Path(args.output)
        # If output is a directory, preserve the filename inside it
        if out.is_dir():
            output_path = out / input_path.name
        else:
            output_path = out
    else:
        output_path = input_path  # in-place

    ok = process_file(input_path, output_path, fields, dry_run=args.dry_run)
    sys.exit(0 if ok else 1)


if __name__ == "__main__":
    main()
