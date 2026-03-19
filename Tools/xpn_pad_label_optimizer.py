#!/usr/bin/env python3
"""
XPN Pad Label Optimizer — XO_OX Designs
Reads all .xpm programs inside a .xpn ZIP and rewrites pad labels so that
the 6-character hardware display on the MPC shows meaningful names instead
of auto-truncated noise.

Problem: MPC hardware shows exactly 6 characters per pad. When a sample is
named "Kick_Heavy_Bass_01.wav", the MPC shows "Kick_H" — useless. This tool
applies smart truncation rules to produce labels like "HvyBas".

Truncation pipeline (applied in order):
  1. Strip file extension
  2. Remove common drum-type prefixes:  Kick_, Snare_, Hat_, CHat_, OHat_,
     HiHat_, Tom_, Clap_, Perc_, Cymbal_, Crash_, Ride_, Shaker_, FX_
  3. Remove common suffixes:  _01.._09, _v1.._v9, _FX, _wet, _dry, _rr,
     _mono, _st, _L, _R, _x (case-insensitive)
  4. Apply vocabulary shortcuts:
       Heavy   → Hvy    Bright  → Brt    Open    → Opn    Closed  → Cls
       Tight   → Tgt    Warm    → Wrm    Hard    → Hrd    Soft    → Sft
       Short   → Shrt   Long    → Lng    Dark    → Drk    Light   → Lgt
       Attack  → Atk    Release → Rel    Punch   → Pnch   Snap    → Snp
       Brush   → Brsh   Rimshot → Rim    Ghost   → Gst    Vintage → Vnt
       Room    → Rm     Hall    → Hl     Reverb  → Rv     Delay   → Dly
  5. CamelCase the first two meaningful tokens and concat → up to 6 chars
  6. If still > 6 chars, take first 6 characters
  7. Deduplicate within program by appending 1-digit suffix (A–Z then 0–9)

The tool modifies the <SampleName> child text inside each <Instrument> block.
MPC reads pad labels from <SampleName> on drum programs; the label is the
first 6 chars as displayed on hardware.

Usage:
    # Preview changes without writing
    python3 xpn_pad_label_optimizer.py --xpn pack.xpn --dry-run --show-diff

    # Write to a new file
    python3 xpn_pad_label_optimizer.py --xpn pack.xpn --output optimized.xpn

    # Overwrite the source file
    python3 xpn_pad_label_optimizer.py --xpn pack.xpn --in-place

    # Show diff and write
    python3 xpn_pad_label_optimizer.py --xpn pack.xpn --output out.xpn --show-diff

stdlib only — no third-party dependencies.
"""

import argparse
import os
import re
import shutil
import sys
import tempfile
import zipfile
from pathlib import Path
from xml.etree import ElementTree as ET


# ---------------------------------------------------------------------------
# Truncation rules
# ---------------------------------------------------------------------------

DRUM_PREFIXES = re.compile(
    r'^(Kick|Snare|Hat|CHat|OHat|HiHat|Tom|Clap|Perc|Cymbal|Crash|Ride|'
    r'Shaker|FX|Bass|Floor|Snap)_+',
    re.IGNORECASE,
)

TRAILING_JUNK = re.compile(
    r'[_\-](0[1-9]|v[1-9]|c[1-9]|rr[1-9]?|fx|wet|dry|mono|st|stereo|[lrLR]|x)$',
    re.IGNORECASE,
)

VOCAB = [
    (re.compile(r'\bHeavy\b',   re.I), 'Hvy'),
    (re.compile(r'\bBright\b',  re.I), 'Brt'),
    (re.compile(r'\bOpen\b',    re.I), 'Opn'),
    (re.compile(r'\bClosed\b',  re.I), 'Cls'),
    (re.compile(r'\bTight\b',   re.I), 'Tgt'),
    (re.compile(r'\bWarm\b',    re.I), 'Wrm'),
    (re.compile(r'\bHard\b',    re.I), 'Hrd'),
    (re.compile(r'\bSoft\b',    re.I), 'Sft'),
    (re.compile(r'\bShort\b',   re.I), 'Shrt'),
    (re.compile(r'\bLong\b',    re.I), 'Lng'),
    (re.compile(r'\bDark\b',    re.I), 'Drk'),
    (re.compile(r'\bLight\b',   re.I), 'Lgt'),
    (re.compile(r'\bAttack\b',  re.I), 'Atk'),
    (re.compile(r'\bRelease\b', re.I), 'Rel'),
    (re.compile(r'\bPunch\b',   re.I), 'Pnch'),
    (re.compile(r'\bSnap\b',    re.I), 'Snp'),
    (re.compile(r'\bBrush\b',   re.I), 'Brsh'),
    (re.compile(r'\bRimshot\b', re.I), 'Rim'),
    (re.compile(r'\bGhost\b',   re.I), 'Gst'),
    (re.compile(r'\bVintage\b', re.I), 'Vnt'),
    (re.compile(r'\bRoom\b',    re.I), 'Rm'),
    (re.compile(r'\bHall\b',    re.I), 'Hl'),
    (re.compile(r'\bReverb\b',  re.I), 'Rv'),
    (re.compile(r'\bDelay\b',   re.I), 'Dly'),
    (re.compile(r'\bLayer\b',   re.I), ''),
    (re.compile(r'\bSample\b',  re.I), ''),
]


def smart_label(stem: str) -> str:
    """Convert a sample filename stem into a ≤6-char meaningful pad label."""
    # 1. Strip drum-type prefix
    s = DRUM_PREFIXES.sub('', stem)

    # 2. Strip trailing junk tokens iteratively
    for _ in range(4):
        prev = s
        s = TRAILING_JUNK.sub('', s)
        if s == prev:
            break

    # 3. Split into tokens on underscores, hyphens, spaces, or CamelCase boundaries
    #    (CamelCase: insert space before an uppercase letter preceded by lowercase/digit)
    s = re.sub(r'([a-z0-9])([A-Z])', r'\1 \2', s)
    tokens = [t for t in re.split(r'[_\-\s]+', s) if t]

    if not tokens:
        return stem[:6]  # nothing survived — fall back to raw stem

    # 4. Apply vocabulary shortcuts per token (avoids \b vs _ boundary issues)
    def _shorten_token(tok: str) -> str:
        for pattern, replacement in VOCAB:
            result = pattern.sub(replacement, tok)
            if result != tok:
                return result
        return tok

    tokens = [_shorten_token(t) for t in tokens]
    tokens = [t for t in tokens if t]  # drop any tokens reduced to empty string

    if not tokens:
        return stem[:6]

    # 5. Build label from leading tokens, capitalising each token's first char
    label = ''
    for tok in tokens:
        if len(label) >= 6:
            break
        label += tok[0].upper() + tok[1:]
    label = label[:6]

    return label if label else stem[:6]


def deduplicate_labels(labels: list) -> list:
    """
    Given a list of raw labels (one per pad), return a list of the same length
    where every label is unique within the list.  Collisions are resolved by
    replacing the last character(s) with A, B, C … Z, 0, 1 … 9.
    """
    seen: dict = {}      # label -> count used
    result = []
    for lbl in labels:
        if lbl not in seen:
            seen[lbl] = 0
            result.append(lbl)
        else:
            seen[lbl] += 1
            idx = seen[lbl]
            suffix_chars = 'ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789'
            suffix = suffix_chars[idx - 1] if idx - 1 < len(suffix_chars) else str(idx)
            # Fit suffix into 6 chars by truncating base
            base = lbl[:6 - len(suffix)]
            unique = base + suffix
            # Track the new unique label too
            seen[unique] = seen.get(unique, 0)
            result.append(unique)
    return result


# ---------------------------------------------------------------------------
# XPM processing
# ---------------------------------------------------------------------------

def _get_first_sample_stem(instrument_elem: ET.Element):
    """Return the stem of the first active SampleFile in an Instrument."""
    for layer in instrument_elem.iter('Layer'):
        active = layer.findtext('Active', 'False')
        if active.strip().lower() != 'true':
            continue
        sf = layer.findtext('SampleFile', '').strip()
        if sf:
            return Path(sf).stem
    return None


def process_xpm(xml_bytes: bytes) -> tuple[bytes, list[tuple[str, str]]]:
    """
    Parse an XPM XML, generate optimised pad labels for every active
    Instrument, patch <SampleName> elements, and return modified XML bytes
    plus a diff list of (old_label, new_label) tuples.
    """
    # Preserve the original declaration line if present
    declaration = ''
    text = xml_bytes.decode('utf-8', errors='replace')
    if text.startswith('<?xml'):
        declaration = text[:text.index('?>') + 2]

    root = ET.fromstring(xml_bytes)

    instruments = list(root.iter('Instrument'))

    # Collect one label candidate per instrument
    raw_labels: list[str] = []
    stems: list[str | None] = []
    for inst in instruments:
        stem = _get_first_sample_stem(inst)
        stems.append(stem)
        raw_labels.append(smart_label(stem) if stem else '')

    deduped = deduplicate_labels(raw_labels)

    diff: list[tuple[str, str]] = []
    for inst, stem, old_raw, new_lbl in zip(instruments, stems, raw_labels, deduped):
        if stem is None:
            continue
        # Current SampleName (first Layer's SampleName used as the pad label)
        for layer in inst.iter('Layer'):
            active = layer.findtext('Active', 'False')
            if active.strip().lower() != 'true':
                continue
            sn_elem = layer.find('SampleName')
            if sn_elem is None:
                continue
            old_lbl = (sn_elem.text or '').strip()
            if old_lbl != new_lbl:
                diff.append((old_lbl, new_lbl))
                sn_elem.text = new_lbl
            break  # only patch the first active layer's SampleName per instrument

    # Serialise back — ET strips the XML declaration, so restore it
    out_bytes = ET.tostring(root, encoding='unicode', xml_declaration=False)
    if declaration:
        out_bytes = declaration + '\n' + out_bytes

    return out_bytes.encode('utf-8'), diff


# ---------------------------------------------------------------------------
# ZipFile (XPN) round-trip
# ---------------------------------------------------------------------------

def process_xpn(src_path: Path, dst_path: Path, dry_run: bool, show_diff: bool) -> int:
    """
    Read src_path (.xpn ZIP), process every .xpm inside, write to dst_path.
    Returns total number of pad labels changed.
    """
    total_changes = 0

    with zipfile.ZipFile(src_path, 'r') as zin:
        names = zin.namelist()
        xpm_names = [n for n in names if n.lower().endswith('.xpm')]

        if not xpm_names:
            print('No .xpm files found in the XPN archive.', file=sys.stderr)
            return 0

        if dry_run:
            # Process but don't write output
            for xpm_name in xpm_names:
                data = zin.read(xpm_name)
                _, diff = process_xpm(data)
                total_changes += len(diff)
                if show_diff and diff:
                    print(f'\n  [{xpm_name}]')
                    for old, new in diff:
                        print(f'    {old!r:20s} → {new!r}')
            return total_changes

        # Write to a temp file first, then move atomically
        tmp_fd, tmp_path = tempfile.mkstemp(suffix='.xpn')
        os.close(tmp_fd)
        try:
            with zipfile.ZipFile(src_path, 'r') as zin2, \
                 zipfile.ZipFile(tmp_path, 'w', zipfile.ZIP_DEFLATED) as zout:
                for name in names:
                    data = zin2.read(name)
                    if name.lower().endswith('.xpm'):
                        new_data, diff = process_xpm(data)
                        total_changes += len(diff)
                        if show_diff and diff:
                            print(f'\n  [{name}]')
                            for old, new in diff:
                                print(f'    {old!r:20s} → {new!r}')
                        zout.writestr(name, new_data)
                    else:
                        zout.writestr(name, data)
            shutil.move(tmp_path, dst_path)
        except Exception:
            os.unlink(tmp_path)
            raise

    return total_changes


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def build_parser() -> argparse.ArgumentParser:
    p = argparse.ArgumentParser(
        description='Optimise 6-character MPC pad labels inside an XPN pack.',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    p.add_argument('--xpn', required=True, metavar='FILE',
                   help='Source .xpn file to process')
    p.add_argument('--output', metavar='FILE',
                   help='Write result to this path (mutually exclusive with --in-place)')
    p.add_argument('--in-place', action='store_true',
                   help='Overwrite the source .xpn file')
    p.add_argument('--dry-run', action='store_true',
                   help='Parse and compute labels but do not write any files')
    p.add_argument('--show-diff', action='store_true',
                   help='Print old → new label changes for every pad')
    return p


def main(argv=None):
    parser = build_parser()
    args = parser.parse_args(argv)

    src = Path(args.xpn)
    if not src.exists():
        sys.exit(f'ERROR: file not found: {src}')
    if not zipfile.is_zipfile(src):
        sys.exit(f'ERROR: not a valid ZIP/XPN file: {src}')

    if args.output and args.in_place:
        sys.exit('ERROR: --output and --in-place are mutually exclusive')

    if args.dry_run:
        print(f'DRY RUN — {src}')
        n = process_xpn(src, src, dry_run=True, show_diff=args.show_diff)
        print(f'\n{n} pad label(s) would be changed.')
        return

    if args.in_place:
        dst = src
    elif args.output:
        dst = Path(args.output)
    else:
        sys.exit('ERROR: specify --output <file> or --in-place (or --dry-run to preview)')

    n = process_xpn(src, dst, dry_run=False, show_diff=args.show_diff)
    print(f'{n} pad label(s) updated → {dst}')


if __name__ == '__main__':
    main()
