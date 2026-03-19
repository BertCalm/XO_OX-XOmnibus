#!/usr/bin/env python3
"""
xpn_pack_registry.py — XO_OX XPN Pack Registry Manager

Manages pack_registry.json at the repo root: a master catalog of all released
(and planned/beta) XO_OX XPN packs.

Commands:
  add     Add a new pack entry to the registry
  update  Update fields on an existing pack by ID
  list    List packs with optional --status / --engine / --tier filters
  stats   Summary counts by status, tier, engine, total preset count
  export  Export full registry as json | markdown | csv

Usage:
  python xpn_pack_registry.py add --pack-name "Iron Machines" --engine ONSET \
      --mood Foundation --tier SIGNAL --version 1.0.0 --status planned

  python xpn_pack_registry.py update --id onset-iron-machines \
      --version 1.1.0 --status released --gumroad-id XXXX

  python xpn_pack_registry.py list --status released --engine ONSET

  python xpn_pack_registry.py stats

  python xpn_pack_registry.py export --format markdown

Registry location: {repo_root}/pack_registry.json
Override with:  --registry /path/to/pack_registry.json
"""

import argparse
import csv
import io
import json
import re
import subprocess
import sys
from datetime import datetime, timezone
from pathlib import Path

REGISTRY_VERSION = "1.0"
VALID_STATUSES = {"planned", "beta", "released", "deprecated"}


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def slugify(text: str) -> str:
    """Convert a string to a lowercase hyphen-separated slug."""
    text = text.lower().strip()
    text = re.sub(r"[^\w\s-]", "", text)
    text = re.sub(r"[\s_]+", "-", text)
    text = re.sub(r"-+", "-", text)
    return text.strip("-")


def make_id(pack_name: str, engine: str) -> str:
    return f"{slugify(engine)}-{slugify(pack_name)}"


def now_iso() -> str:
    return datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")


def find_registry(override) -> Path:
    if override:
        return Path(override)
    # Try git root
    try:
        result = subprocess.run(
            ["git", "rev-parse", "--show-toplevel"],
            capture_output=True, text=True, check=True
        )
        return Path(result.stdout.strip()) / "pack_registry.json"
    except Exception:
        return Path.cwd() / "pack_registry.json"


def load_registry(path: Path) -> dict:
    if path.exists():
        with open(path) as f:
            return json.load(f)
    return {
        "registry_version": REGISTRY_VERSION,
        "last_updated": now_iso(),
        "packs": []
    }


def save_registry(path: Path, data: dict) -> None:
    data["last_updated"] = now_iso()
    with open(path, "w") as f:
        json.dump(data, f, indent=2)
    print(f"Registry saved: {path}")


# ---------------------------------------------------------------------------
# Commands
# ---------------------------------------------------------------------------

def cmd_add(args, registry_path: Path) -> None:
    data = load_registry(registry_path)
    pack_id = make_id(args.pack_name, args.engine)

    # Check for duplicate
    if any(p["id"] == pack_id for p in data["packs"]):
        print(f"ERROR: Pack with id '{pack_id}' already exists. Use 'update' to modify it.")
        sys.exit(1)

    xpn_filename = (
        args.xpn_filename
        or f"{args.engine}_{re.sub(r'[^A-Za-z0-9]', '', args.pack_name)}_v{args.version}.xpn"
    )

    entry = {
        "id": pack_id,
        "pack_name": args.pack_name,
        "engine": args.engine.upper(),
        "mood": args.mood,
        "tier": args.tier.upper(),
        "version": args.version,
        "release_date": args.release_date or None,
        "gumroad_product_id": args.gumroad_id or None,
        "xpn_filename": xpn_filename,
        "preset_count": args.preset_count,
        "price_usd": args.price_usd,
        "tags": [t.strip() for t in args.tags.split(",")] if args.tags else [],
        "status": args.status,
    }

    data["packs"].append(entry)
    save_registry(registry_path, data)
    print(f"Added pack: {pack_id}")


def cmd_update(args, registry_path: Path) -> None:
    data = load_registry(registry_path)
    pack = next((p for p in data["packs"] if p["id"] == args.id), None)
    if not pack:
        print(f"ERROR: No pack with id '{args.id}' found.")
        sys.exit(1)

    updatable = {
        "version": args.version,
        "status": args.status,
        "gumroad_product_id": args.gumroad_id,
        "release_date": args.release_date,
        "preset_count": args.preset_count,
        "price_usd": args.price_usd,
        "mood": args.mood,
        "tier": args.tier.upper() if args.tier else None,
        "tags": [t.strip() for t in args.tags.split(",")] if args.tags else None,
        "xpn_filename": args.xpn_filename,
    }

    changed = []
    for field, value in updatable.items():
        if value is not None:
            pack[field] = value
            changed.append(field)

    if not changed:
        print("No fields to update. Pass at least one update flag.")
        sys.exit(1)

    save_registry(registry_path, data)
    print(f"Updated pack '{args.id}': {', '.join(changed)}")


def cmd_list(args, registry_path: Path) -> None:
    data = load_registry(registry_path)
    packs = data["packs"]

    if args.status:
        packs = [p for p in packs if p.get("status") == args.status]
    if args.engine:
        packs = [p for p in packs if p.get("engine", "").upper() == args.engine.upper()]
    if args.tier:
        packs = [p for p in packs if p.get("tier", "").upper() == args.tier.upper()]

    if not packs:
        print("No packs match the filter criteria.")
        return

    col_w = [36, 12, 10, 10, 8, 10, 8]
    header = f"{'ID':<36}  {'PACK NAME':<12}  {'ENGINE':<10}  {'STATUS':<10}  {'TIER':<8}  {'VERSION':<10}  {'PRESETS':<8}"
    print(header)
    print("-" * len(header))
    for p in packs:
        print(
            f"{p['id']:<36}  {p['pack_name']:<12}  {p.get('engine',''):<10}  "
            f"{p.get('status',''):<10}  {p.get('tier',''):<8}  "
            f"{p.get('version',''):<10}  {str(p.get('preset_count', '?')):<8}"
        )
    print(f"\n{len(packs)} pack(s) listed.")


def cmd_stats(args, registry_path: Path) -> None:
    data = load_registry(registry_path)
    packs = data["packs"]

    total = len(packs)
    total_presets = sum(p.get("preset_count", 0) or 0 for p in packs)

    by_status: dict = {}
    by_tier: dict = {}
    by_engine: dict = {}

    for p in packs:
        s = p.get("status", "unknown")
        by_status[s] = by_status.get(s, 0) + 1
        t = p.get("tier", "unknown")
        by_tier[t] = by_tier.get(t, 0) + 1
        e = p.get("engine", "unknown")
        by_engine[e] = by_engine.get(e, 0) + 1

    print(f"XO_OX Pack Registry — {registry_path}")
    print(f"Registry version : {data.get('registry_version')}")
    print(f"Last updated     : {data.get('last_updated')}")
    print(f"Total packs      : {total}")
    print(f"Total presets    : {total_presets}")
    print()

    print("By Status:")
    for k, v in sorted(by_status.items()):
        print(f"  {k:<14} {v}")

    print("\nBy Tier:")
    for k, v in sorted(by_tier.items()):
        print(f"  {k:<14} {v}")

    print("\nBy Engine:")
    for k, v in sorted(by_engine.items()):
        print(f"  {k:<14} {v}")


def cmd_export(args, registry_path: Path) -> None:
    data = load_registry(registry_path)
    packs = data["packs"]
    fmt = args.format.lower()

    if fmt == "json":
        print(json.dumps(data, indent=2))

    elif fmt == "csv":
        fields = [
            "id", "pack_name", "engine", "mood", "tier", "version",
            "release_date", "gumroad_product_id", "xpn_filename",
            "preset_count", "price_usd", "tags", "status"
        ]
        buf = io.StringIO()
        writer = csv.DictWriter(buf, fieldnames=fields, extrasaction="ignore")
        writer.writeheader()
        for p in packs:
            row = dict(p)
            if isinstance(row.get("tags"), list):
                row["tags"] = ",".join(row["tags"])
            writer.writerow(row)
        print(buf.getvalue(), end="")

    elif fmt == "markdown":
        print(f"# XO_OX Pack Registry\n")
        print(f"**Registry version**: {data.get('registry_version')}  ")
        print(f"**Last updated**: {data.get('last_updated')}  ")
        print(f"**Total packs**: {len(packs)}\n")
        print("| ID | Pack Name | Engine | Status | Tier | Version | Presets | Price |")
        print("|----|-----------|--------|--------|------|---------|---------|-------|")
        for p in packs:
            price = f"${p['price_usd']:.2f}" if p.get("price_usd") is not None else "—"
            presets = str(p.get("preset_count") or "—")
            print(
                f"| {p['id']} | {p['pack_name']} | {p.get('engine','')} | "
                f"{p.get('status','')} | {p.get('tier','')} | {p.get('version','')} | "
                f"{presets} | {price} |"
            )
    else:
        print(f"ERROR: Unknown format '{fmt}'. Use json, markdown, or csv.")
        sys.exit(1)


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="XO_OX XPN Pack Registry Manager",
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument("--registry", help="Path to pack_registry.json (default: repo root)")

    sub = parser.add_subparsers(dest="command")

    # --- add ---
    p_add = sub.add_parser("add", help="Add a new pack to the registry")
    p_add.add_argument("--pack-name", required=True)
    p_add.add_argument("--engine", required=True)
    p_add.add_argument("--mood", required=True)
    p_add.add_argument("--tier", required=True)
    p_add.add_argument("--version", default="1.0.0")
    p_add.add_argument("--status", default="planned", choices=sorted(VALID_STATUSES))
    p_add.add_argument("--release-date")
    p_add.add_argument("--gumroad-id")
    p_add.add_argument("--xpn-filename")
    p_add.add_argument("--preset-count", type=int, default=16)
    p_add.add_argument("--price-usd", type=float, default=9.00)
    p_add.add_argument("--tags", help="Comma-separated tags")

    # --- update ---
    p_upd = sub.add_parser("update", help="Update fields on an existing pack")
    p_upd.add_argument("--id", required=True, help="Pack ID (e.g. onset-iron-machines)")
    p_upd.add_argument("--version")
    p_upd.add_argument("--status", choices=sorted(VALID_STATUSES))
    p_upd.add_argument("--gumroad-id")
    p_upd.add_argument("--release-date")
    p_upd.add_argument("--preset-count", type=int)
    p_upd.add_argument("--price-usd", type=float)
    p_upd.add_argument("--mood")
    p_upd.add_argument("--tier")
    p_upd.add_argument("--tags", help="Comma-separated tags (replaces existing)")
    p_upd.add_argument("--xpn-filename")

    # --- list ---
    p_lst = sub.add_parser("list", help="List packs with optional filters")
    p_lst.add_argument("--status", choices=sorted(VALID_STATUSES))
    p_lst.add_argument("--engine")
    p_lst.add_argument("--tier")

    # --- stats ---
    sub.add_parser("stats", help="Show registry summary statistics")

    # --- export ---
    p_exp = sub.add_parser("export", help="Export registry as json, markdown, or csv")
    p_exp.add_argument("--format", default="json", choices=["json", "markdown", "csv"])

    return parser


def main() -> None:
    parser = build_parser()
    args = parser.parse_args()

    if not args.command:
        parser.print_help()
        sys.exit(0)

    registry_path = find_registry(args.registry)

    dispatch = {
        "add": cmd_add,
        "update": cmd_update,
        "list": cmd_list,
        "stats": cmd_stats,
        "export": cmd_export,
    }
    dispatch[args.command](args, registry_path)


if __name__ == "__main__":
    main()
