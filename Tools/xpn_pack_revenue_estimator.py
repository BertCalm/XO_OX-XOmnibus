#!/usr/bin/env python3
"""
xpn_pack_revenue_estimator.py — XO_OX Pack Revenue Projection Tool

Models revenue across 3 Patreon tiers and standalone pack sales.
Scenarios: conservative / moderate / optimistic

Usage:
    python Tools/xpn_pack_revenue_estimator.py
    python Tools/xpn_pack_revenue_estimator.py --patrons-current 150 --patrons-deep 40 --pack-price 14.99
    python Tools/xpn_pack_revenue_estimator.py --packs-planned 8 --scenario moderate
"""

import argparse
import math
import json
import sys

# ---------------------------------------------------------------------------
# Constants
# ---------------------------------------------------------------------------

PATREON_FEE_RATE = 0.08        # Patreon takes ~8% + payment processing
PAYMENT_FEE_RATE = 0.029       # Stripe/payment ~2.9% + $0.30 per transaction
PAYMENT_FEE_FIXED = 0.30

TIER_SIGNAL = {"name": "Signal", "price": 0.00, "label": "Signal/$0"}
TIER_CURRENT = {"name": "Current", "price": 5.00, "label": "Current/$5"}
TIER_DEEP = {"name": "Deep Water", "price": 15.00, "label": "Deep Water/$15"}

# Scenario patron counts: (signal_free, current_5, deep_15)
SCENARIOS = {
    "conservative": {"signal": 200,  "current": 50,  "deep": 10},
    "moderate":     {"signal": 800,  "current": 200, "deep": 50},
    "optimistic":   {"signal": 2000, "current": 500, "deep": 150},
}

# Fleet defaults (can be overridden via CLI)
DEFAULT_ENGINES = 34
DEFAULT_PACKS_PLANNED = 5
DEFAULT_PACK_PRICE_STANDARD = 9.99
DEFAULT_PACK_PRICE_PREMIUM = 19.99

# Assumed standalone pack conversion rates by scenario
STANDALONE_CONVERSIONS = {
    "conservative": 20,    # units sold per pack release
    "moderate":     80,
    "optimistic":   250,
}

# Assumed releases per year
RELEASES_PER_YEAR = 4

# Monthly creator costs (baseline break-even target)
MONTHLY_CREATOR_COSTS = {
    "hosting":      10.00,   # Firebase / CDN
    "tools":        20.00,   # software subscriptions
    "patreon_fee":  0.00,    # computed dynamically
    "time_hours":   20,      # hours/month on pack work
    "hourly_rate":  50.00,   # target $50/hr for break-even
}


# ---------------------------------------------------------------------------
# Revenue calculations
# ---------------------------------------------------------------------------

def net_patreon(gross: float) -> float:
    """Return net after Patreon platform fee + payment processing."""
    after_platform = gross * (1 - PATREON_FEE_RATE)
    after_payment = after_platform * (1 - PAYMENT_FEE_RATE) - PAYMENT_FEE_FIXED
    return max(after_payment, 0.0)


def monthly_patreon_revenue(current_patrons: int, deep_patrons: int) -> dict:
    """Gross and net monthly Patreon revenue."""
    gross_current = current_patrons * TIER_CURRENT["price"]
    gross_deep = deep_patrons * TIER_DEEP["price"]
    gross_total = gross_current + gross_deep

    net_current = net_patreon(gross_current) if gross_current > 0 else 0.0
    net_deep = net_patreon(gross_deep) if gross_deep > 0 else 0.0

    return {
        "gross_current": gross_current,
        "gross_deep": gross_deep,
        "gross_total": gross_total,
        "net_current": net_current,
        "net_deep": net_deep,
        "net_total": net_current + net_deep,
    }


def annual_pack_revenue(pack_price: float, units_per_release: int,
                        releases_per_year: int = RELEASES_PER_YEAR) -> dict:
    """Standalone pack sales annual revenue."""
    gross_per_release = pack_price * units_per_release
    gross_annual = gross_per_release * releases_per_year
    # Bandcamp/Gumroad ~15% fee
    net_annual = gross_annual * 0.85
    return {
        "gross_per_release": gross_per_release,
        "gross_annual": gross_annual,
        "net_annual": net_annual,
        "releases_per_year": releases_per_year,
        "units_per_release": units_per_release,
    }


def break_even_packs(monthly_net_patreon: float, pack_price: float) -> dict:
    """How many pack sales/releases needed to cover monthly costs."""
    time_cost = MONTHLY_CREATOR_COSTS["time_hours"] * MONTHLY_CREATOR_COSTS["hourly_rate"]
    fixed_costs = (MONTHLY_CREATOR_COSTS["hosting"] +
                   MONTHLY_CREATOR_COSTS["tools"] + time_cost)
    gap = max(fixed_costs - monthly_net_patreon, 0.0)
    net_per_pack_unit = pack_price * 0.85  # after marketplace fees
    units_needed = math.ceil(gap / net_per_pack_unit) if net_per_pack_unit > 0 else float("inf")
    releases_needed = math.ceil(units_needed / STANDALONE_CONVERSIONS["moderate"])
    return {
        "monthly_fixed_costs": fixed_costs,
        "gap_from_patreon": gap,
        "units_needed_monthly": units_needed,
        "releases_needed_annually": releases_needed * 12,
        "notes": "Based on moderate conversion rate & $50/hr time valuation",
    }


# ---------------------------------------------------------------------------
# ASCII table rendering
# ---------------------------------------------------------------------------

def pad(s: str, width: int, align: str = "left") -> str:
    s = str(s)
    if align == "right":
        return s.rjust(width)
    elif align == "center":
        return s.center(width)
    return s.ljust(width)


def fmt_dollar(amount: float) -> str:
    return f"${amount:,.2f}"


def print_separator(widths: list[int], char: str = "-") -> None:
    print("+" + "+".join(char * (w + 2) for w in widths) + "+")


def print_row(cells: list[str], widths: list[int], aligns: list[str] = None) -> None:
    if aligns is None:
        aligns = ["left"] * len(cells)
    row = "| " + " | ".join(pad(c, w, a) for c, w, a in zip(cells, widths, aligns)) + " |"
    print(row)


def render_revenue_table(results: dict, pack_price: float) -> None:
    scenarios = ["conservative", "moderate", "optimistic"]
    col0 = 30
    col_w = 18

    widths = [col0] + [col_w] * len(scenarios)
    aligns_header = ["left"] + ["center"] * len(scenarios)
    aligns_data   = ["left"] + ["right"]  * len(scenarios)

    print()
    print("=" * (col0 + (col_w + 3) * len(scenarios) + 1))
    print("  XO_OX PACK REVENUE ESTIMATOR — PROJECTION TABLE")
    print(f"  Pack price: {fmt_dollar(pack_price)}  |  Patreon fee: {PATREON_FEE_RATE*100:.0f}%  |  {RELEASES_PER_YEAR} releases/year")
    print("=" * (col0 + (col_w + 3) * len(scenarios) + 1))
    print()

    # ---- Patron counts ----
    print_separator(widths)
    print_row(["PATRON COUNTS"] + [s.upper() for s in scenarios], widths, aligns_header)
    print_separator(widths, "=")

    for tier_key, label in [("signal", "Signal (free)"),
                             ("current", "Current ($5/mo)"),
                             ("deep", "Deep Water ($15/mo)")]:
        vals = [str(SCENARIOS[s][tier_key]) for s in scenarios]
        if tier_key == "deep":
            vals = [results[s]["custom_patrons"]["deep_override"] or str(SCENARIOS[s][tier_key])
                    for s in scenarios]
            vals = [str(results[s]["inputs"]["deep"]) for s in scenarios]
        elif tier_key == "current":
            vals = [str(results[s]["inputs"]["current"]) for s in scenarios]
        else:
            vals = [str(SCENARIOS[s]["signal"]) for s in scenarios]
        print_row([label] + vals, widths, aligns_data)

    print_separator(widths)

    # ---- Monthly Patreon revenue ----
    print_row(["MONTHLY PATREON REVENUE"] + [""] * len(scenarios), widths, aligns_header)
    print_separator(widths, "=")

    rows = [
        ("Gross — Current tier",  lambda s: fmt_dollar(results[s]["patreon"]["gross_current"])),
        ("Gross — Deep Water tier",lambda s: fmt_dollar(results[s]["patreon"]["gross_deep"])),
        ("Gross total",            lambda s: fmt_dollar(results[s]["patreon"]["gross_total"])),
        ("Net (after fees)",       lambda s: fmt_dollar(results[s]["patreon"]["net_total"])),
    ]
    for label, fn in rows:
        print_row([label] + [fn(s) for s in scenarios], widths, aligns_data)

    print_separator(widths)

    # ---- Annual Patreon ----
    print_row(["ANNUAL PATREON (×12)", ""] + [""] * (len(scenarios) - 1), widths, aligns_header)
    print_separator(widths, "=")

    print_row(["Annual gross"] + [fmt_dollar(results[s]["patreon"]["gross_total"] * 12)
                                   for s in scenarios], widths, aligns_data)
    print_row(["Annual net"] + [fmt_dollar(results[s]["patreon"]["net_total"] * 12)
                                 for s in scenarios], widths, aligns_data)

    print_separator(widths)

    # ---- Pack sales ----
    print_row(["STANDALONE PACK SALES"] + [""] * len(scenarios), widths, aligns_header)
    print_separator(widths, "=")

    print_row(["Units/release (assumed)"] +
              [str(STANDALONE_CONVERSIONS[s]) for s in scenarios],
              widths, aligns_data)
    print_row(["Gross/release"] +
              [fmt_dollar(results[s]["packs"]["gross_per_release"]) for s in scenarios],
              widths, aligns_data)
    print_row(["Annual gross (pack sales)"] +
              [fmt_dollar(results[s]["packs"]["gross_annual"]) for s in scenarios],
              widths, aligns_data)
    print_row(["Annual net (pack sales)"] +
              [fmt_dollar(results[s]["packs"]["net_annual"]) for s in scenarios],
              widths, aligns_data)

    print_separator(widths)

    # ---- Combined annual ----
    print_row(["COMBINED ANNUAL TOTAL"] + [""] * len(scenarios), widths, aligns_header)
    print_separator(widths, "=")

    combined_gross = {s: results[s]["patreon"]["gross_total"] * 12 + results[s]["packs"]["gross_annual"]
                      for s in scenarios}
    combined_net   = {s: results[s]["patreon"]["net_total"] * 12 + results[s]["packs"]["net_annual"]
                      for s in scenarios}

    print_row(["Combined annual gross"] + [fmt_dollar(combined_gross[s]) for s in scenarios],
              widths, aligns_data)
    print_row(["Combined annual net"]   + [fmt_dollar(combined_net[s])   for s in scenarios],
              widths, aligns_data)
    print_row(["Monthly avg net"]       +
              [fmt_dollar(combined_net[s] / 12) for s in scenarios],
              widths, aligns_data)

    print_separator(widths)

    # ---- Break-even ----
    print_row(["BREAK-EVEN ANALYSIS"] + [""] * len(scenarios), widths, aligns_header)
    print_separator(widths, "=")

    be_label = f"Monthly costs (${MONTHLY_CREATOR_COSTS['time_hours']}h @ ${MONTHLY_CREATOR_COSTS['hourly_rate']:.0f}/hr)"
    print_row([be_label] + [fmt_dollar(results[scenarios[0]]["breakeven"]["monthly_fixed_costs"])] * len(scenarios),
              widths, aligns_data)

    print_row(["Gap after Patreon net"] +
              [fmt_dollar(results[s]["breakeven"]["gap_from_patreon"]) for s in scenarios],
              widths, aligns_data)
    print_row(["Pack units/mo to close gap"] +
              [str(results[s]["breakeven"]["units_needed_monthly"]) for s in scenarios],
              widths, aligns_data)

    print_separator(widths)
    print()
    print(f"  Notes:")
    print(f"  • Patreon fee: {PATREON_FEE_RATE*100:.0f}% platform + {PAYMENT_FEE_RATE*100:.1f}% payment + ${PAYMENT_FEE_FIXED:.2f} fixed/charge")
    print(f"  • Pack marketplace fee: 15% (Bandcamp/Gumroad typical)")
    print(f"  • Signal tier is free — counted as audience, no revenue")
    print(f"  • Break-even uses moderate conversion ({STANDALONE_CONVERSIONS['moderate']} units/release)")
    print()


# ---------------------------------------------------------------------------
# Fleet stats summary
# ---------------------------------------------------------------------------

def print_fleet_summary(engines: int, packs_planned: int, pack_price: float) -> None:
    print("  FLEET STATS")
    print(f"  Engines in XOceanus : {engines}")
    print(f"  Packs planned       : {packs_planned}")
    print(f"  Pack price (default): {fmt_dollar(pack_price)}")
    print(f"  Premium pack price  : {fmt_dollar(DEFAULT_PACK_PRICE_PREMIUM)}")
    print(f"  Free gateway packs  : yes (audience building)")
    print()


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def build_results(patrons_current: dict, patrons_deep: dict, pack_price: float) -> dict:
    results = {}
    for s in ["conservative", "moderate", "optimistic"]:
        cur = patrons_current.get(s, SCENARIOS[s]["current"])
        dep = patrons_deep.get(s, SCENARIOS[s]["deep"])
        patreon = monthly_patreon_revenue(cur, dep)
        packs = annual_pack_revenue(pack_price, STANDALONE_CONVERSIONS[s])
        be = break_even_packs(patreon["net_total"], pack_price)
        results[s] = {
            "inputs": {"current": cur, "deep": dep},
            "custom_patrons": {"deep_override": None},
            "patreon": patreon,
            "packs": packs,
            "breakeven": be,
        }
    return results


def main() -> int:
    parser = argparse.ArgumentParser(
        description="XO_OX Pack Revenue Estimator — Patreon + standalone pack projections"
    )
    parser.add_argument("--patrons-current", type=int, default=None,
                        help="Override Current ($5) patron count for all scenarios")
    parser.add_argument("--patrons-deep", type=int, default=None,
                        help="Override Deep Water ($15) patron count for all scenarios")
    parser.add_argument("--pack-price", type=float, default=DEFAULT_PACK_PRICE_STANDARD,
                        help=f"Standalone pack price (default: {DEFAULT_PACK_PRICE_STANDARD})")
    parser.add_argument("--packs-planned", type=int, default=DEFAULT_PACKS_PLANNED,
                        help=f"Number of packs planned (default: {DEFAULT_PACKS_PLANNED})")
    parser.add_argument("--engines", type=int, default=DEFAULT_ENGINES,
                        help=f"Engine count in fleet (default: {DEFAULT_ENGINES})")
    parser.add_argument("--scenario", choices=["conservative", "moderate", "optimistic", "all"],
                        default="all", help="Which scenario to display (default: all)")
    parser.add_argument("--json", action="store_true",
                        help="Output raw JSON results instead of ASCII table")

    args = parser.parse_args()

    # Build per-scenario patron overrides
    patrons_current = {}
    patrons_deep = {}
    if args.patrons_current is not None:
        for s in ["conservative", "moderate", "optimistic"]:
            patrons_current[s] = args.patrons_current
    if args.patrons_deep is not None:
        for s in ["conservative", "moderate", "optimistic"]:
            patrons_deep[s] = args.patrons_deep

    results = build_results(patrons_current, patrons_deep, args.pack_price)

    if args.json:
        print(json.dumps(results, indent=2))
        return 0

    print()
    print_fleet_summary(args.engines, args.packs_planned, args.pack_price)
    render_revenue_table(results, args.pack_price)

    return 0


if __name__ == "__main__":
    sys.exit(main())
