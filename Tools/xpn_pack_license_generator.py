#!/usr/bin/env python3
"""
xpn_pack_license_generator.py — Generate LICENSE.txt for XO_OX XPN expansion packs.

Usage:
    python xpn_pack_license_generator.py --pack-dir ./MyPack [--type standard|open|exclusive]
        [--pack-name "My Pack Name"] [--year 2026] [--output LICENSE.txt]
"""

import argparse
import sys
from datetime import date
from pathlib import Path
from typing import Optional


# ---------------------------------------------------------------------------
# License templates
# ---------------------------------------------------------------------------

STANDARD_TEMPLATE = """\
XO_OX STANDARD PACK LICENSE
============================
Pack:      {pack_name}
Version:   {version}
Date:      {date}
Copyright: © {year} XO_OX. All rights reserved.

PERMITTED USES
--------------
  • Use these samples in your own musical productions, beats, soundtracks,
    and compositions — commercially or non-commercially.
  • Pitch, time-stretch, layer, chop, or otherwise transform the samples
    as part of an original work.
  • Distribute and sell finished musical works (songs, albums, sync licenses,
    game audio, film scores) that incorporate these samples.
  • Use within the XOmnibus synthesis engine or any host DAW freely and
    without restriction.
  • Include transformed samples in sample packs you create, provided the
    XO_OX source samples are not extractable in their original or near-original
    form (e.g., deeply processed, layered, rendered through synthesis).

PROHIBITED USES
---------------
  • Redistributing, sharing, uploading, or re-releasing these samples in
    raw or minimally processed form — whether free or for sale.
  • Reselling or sublicensing this pack or its contents as a standalone
    sample library, sound pack, or preset bank.
  • Claiming authorship or ownership of the original samples.
  • Using these samples in any product that competes directly with XO_OX
    expansion packs or the XOmnibus instrument.

ATTRIBUTION
-----------
Attribution is appreciated but not required for finished musical works.
When crediting samples, use the format:
    Samples from {pack_name} by XO_OX

DISCLAIMER
----------
These samples are provided "as is" without warranty of any kind. XO_OX
accepts no liability for how these samples are used in productions.

For licensing inquiries: hello@xo-ox.org
XO_OX — xo-ox.org
"""

OPEN_TEMPLATE = """\
XO_OX OPEN PACK LICENSE
========================
Pack:      {pack_name}
Version:   {version}
Date:      {date}
Copyright: © {year} XO_OX. Some rights reserved.

This pack is released under the XO_OX Open Pack License, which allows
broader sharing while protecting against commercial resale of raw samples.

PERMITTED USES
--------------
  • Use these samples in your own musical productions, beats, soundtracks,
    and compositions — commercially or non-commercially.
  • Pitch, time-stretch, layer, chop, or otherwise transform the samples
    as part of an original work.
  • Distribute and sell finished musical works that incorporate these samples
    without restriction.
  • Redistribute this pack freely in its original or lightly repackaged form,
    provided attribution is given and no charge is made for the samples alone.
  • Share this pack with other producers, post it on forums, include it in
    free producer resource bundles — with attribution.
  • Use within the XOmnibus synthesis engine or any host DAW freely.

PROHIBITED USES
---------------
  • Selling or charging money for this pack or its raw/minimally processed
    samples as a standalone product (sample library, sound kit, preset bank).
  • Redistributing without attribution to XO_OX.
  • Claiming authorship or ownership of the original samples.
  • Including in paid bundles where the value proposition is the samples
    themselves rather than a finished musical work.

ATTRIBUTION (REQUIRED FOR REDISTRIBUTION)
------------------------------------------
Any redistribution of this pack must include the following credit:
    Samples from {pack_name} by XO_OX — xo-ox.org

For finished musical works, attribution is appreciated but not required.
Suggested credit format:
    Samples from {pack_name} by XO_OX

DISCLAIMER
----------
These samples are provided "as is" without warranty of any kind. XO_OX
accepts no liability for how these samples are used in productions.

For licensing inquiries: hello@xo-ox.org
XO_OX — xo-ox.org
"""

EXCLUSIVE_TEMPLATE = """\
XO_OX EXCLUSIVE LICENSE
========================
Pack:      {pack_name}
Version:   {version}
Date:      {date}
Copyright: © {year} XO_OX. All rights reserved.

This license grants exclusive rights to a single licensee upon verified
one-time purchase. Exclusivity terms are confirmed in the sale agreement.

EXCLUSIVE RIGHTS GRANTED
-------------------------
  • Sole and exclusive right to use these samples commercially in any and
    all productions, releases, sync licenses, advertisements, games, and
    other media — without restriction on volume or revenue.
  • Right to use samples in finished works distributed under any business
    model: streaming, download, sync, broadcast, live performance.
  • Right to transform, process, and incorporate samples into proprietary
    sound design, instruments, or tools for personal/internal use.
  • Full commercial exploitation of any finished work containing these
    samples, including sublicensing finished works to third parties.
  • Use within the XOmnibus synthesis engine or any host DAW freely.

EXCLUSIVITY SCOPE
-----------------
  • Upon exclusive purchase, XO_OX will not sell or license these same
    samples to any other party for the duration agreed in the sale contract.
  • Default exclusivity period: perpetual unless otherwise stated.

PROHIBITED USES
---------------
  • Redistributing, sharing, uploading, or transferring the raw sample files
    to any third party — exclusivity does not include the right to resell
    or sublicense the underlying sample content.
  • Reselling this exclusive license or the raw samples as a sample library.
  • Using the samples in a way that would allow third parties to extract
    or reconstruct the original sample recordings.

ATTRIBUTION
-----------
Attribution is not required. If credited, use:
    Samples from {pack_name} by XO_OX

DISCLAIMER
----------
These samples are provided "as is" without warranty of any kind. XO_OX
accepts no liability for how these samples are used in productions.
Exclusive license terms are binding as confirmed at time of purchase.

For licensing inquiries: hello@xo-ox.org
XO_OX — xo-ox.org
"""

TEMPLATES = {
    "standard": STANDARD_TEMPLATE,
    "open": OPEN_TEMPLATE,
    "exclusive": EXCLUSIVE_TEMPLATE,
}


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def infer_pack_name(pack_dir: Path) -> str:
    """Derive a human-readable pack name from the directory name."""
    name = pack_dir.name
    # Replace underscores/hyphens with spaces and title-case
    return name.replace("_", " ").replace("-", " ").title()


def generate_license(
    pack_name: str,
    license_type: str,
    year: int,
    version: str = "1.0",
    today: Optional[str] = None,
) -> str:
    template = TEMPLATES[license_type]
    return template.format(
        pack_name=pack_name,
        version=version,
        date=today or date.today().isoformat(),
        year=year,
    )


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Generate a LICENSE.txt for an XO_OX XPN expansion pack.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
License types:
  standard   Commercial use allowed; no redistribution; no resale (default)
  open       Commercial use + redistribution with attribution; no resale
  exclusive  One-time exclusive sale; unlimited commercial use

Examples:
  python xpn_pack_license_generator.py --pack-dir ./Aquatic_Drums
  python xpn_pack_license_generator.py --pack-dir ./Aquatic_Drums --type open
  python xpn_pack_license_generator.py --pack-dir ./Aquatic_Drums \\
      --pack-name "Aquatic Drums Vol. 1" --year 2026 --type standard
        """,
    )
    parser.add_argument(
        "--pack-dir",
        required=True,
        type=Path,
        help="Path to the pack directory (LICENSE.txt is written here)",
    )
    parser.add_argument(
        "--type",
        dest="license_type",
        choices=["standard", "open", "exclusive"],
        default="standard",
        help="License variant to generate (default: standard)",
    )
    parser.add_argument(
        "--pack-name",
        default=None,
        help="Human-readable pack name (inferred from directory name if omitted)",
    )
    parser.add_argument(
        "--year",
        type=int,
        default=date.today().year,
        help="Copyright year (default: current year)",
    )
    parser.add_argument(
        "--version",
        default="1.0",
        help="Pack version string (default: 1.0)",
    )
    parser.add_argument(
        "--output",
        default="LICENSE.txt",
        help="Output filename within --pack-dir (default: LICENSE.txt)",
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Print the generated license to stdout without writing a file",
    )
    return parser


def main() -> int:
    parser = build_parser()
    args = parser.parse_args()

    pack_dir: Path = args.pack_dir.resolve()

    if not args.dry_run and not pack_dir.exists():
        print(f"Error: pack directory does not exist: {pack_dir}", file=sys.stderr)
        return 1

    pack_name = args.pack_name or infer_pack_name(pack_dir)

    license_text = generate_license(
        pack_name=pack_name,
        license_type=args.license_type,
        year=args.year,
        version=args.version,
    )

    if args.dry_run:
        print(license_text)
        return 0

    output_path = pack_dir / args.output
    output_path.write_text(license_text, encoding="utf-8")

    print(f"License written: {output_path}")
    print(f"  Pack:    {pack_name}")
    print(f"  Type:    {args.license_type}")
    print(f"  Year:    {args.year}")
    print(f"  Version: {args.version}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
