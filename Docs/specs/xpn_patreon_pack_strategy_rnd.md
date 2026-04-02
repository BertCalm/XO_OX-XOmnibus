# XPN Patreon Pack Strategy — R&D Spec
**Date:** 2026-03-16
**Project:** XO_OX Pack Ecosystem

---

## Purpose

Define how XPN expansion packs are tiered, produced, released, and delivered to Patreon subscribers. This is the operational backbone of the creator economy: free plugin drives discovery, paid packs drive recurring revenue.

Patreon URL: https://www.patreon.com/cw/XO_OX (live as of 2026-03-22)

---

## 1. Tier Architecture

### Signal — $0 / Free
- XOceanus plugin (AU + Standalone, macOS)
- TIDE TABLES free pack (permanent — no gate, no email required)
- Full Field Guide access (all published posts)
- Signal feed (product update posts)

**Role:** Converts discovery into installed users. TIDE TABLES must be exceptional — it is the audition for the paid tier.

### Current — $5/month
- All Signal benefits
- 1 new XPN pack per month (exclusive for 30 days before any wider release)
- Early access to Field Guide posts (48 hours before public)
- "Current" role in any future Discord / community space

**Role:** The core recurring revenue tier. Designed to be an easy yes — one pack per month at $5 is a clear value proposition for any active MPC producer.

### Deep Water — $15/month
- All Current benefits
- Full back-catalog vault (all packs released to date, available immediately on join)
- Preview builds of XOceanus (2–4 weeks before public release)
- Monthly voting rights: choose the theme/engine focus of one upcoming pack per quarter
- Name in plugin credits (annual release)

**Role:** Superfan tier. The back-catalog vault is the primary value driver — Deep Water must launch with at least 3 packs in the vault to justify the price immediately.

---

## 2. Release Cadence

**Drop date:** 1st of each month.

**Advance notice:** Announce the pack title and cover art on the 20th of the preceding month. Current and Deep Water members receive an email preview with 2–3 sound clips before the public announcement.

**Exclusivity window:** 30 days. Current and Deep Water members get the pack on the 1st. The pack goes on sale outside Patreon on the 1st of the following month.

**External sales channels (post-exclusivity):**
- Primary: Gumroad (direct download, no platform cut beyond Gumroad's fee)
- Secondary: XO-OX.org direct link (long-term, once payment infrastructure is live)
- No Bandcamp — Bandcamp's discovery value is limited for MPC-format packs; Gumroad is cleaner for file delivery

**Pricing outside Patreon:** $8–$12 per pack (individual). Bundle pricing at 3-pack and full-catalog levels once catalog depth warrants it.

---

## 3. Pack Pipeline → Patreon Integration

### XPN Tool Suite Role

The Tools/ pipeline (`~/Documents/GitHub/XO_OX-XOceanus/Tools/`) handles the mechanical production. The handoff to Patreon is a human decision layer on top.

**Automated (runs without manual intervention):**
- `xpn_pack_qa_report.py` — 10-check validation (pad count, velocity layers, file integrity, XPM compliance, etc.). A pack does not ship if any check fails. This is the go/no-go gate.
- `xpn_packager.py` — bundles samples, XPMs, cover art, MPCE_SETUP.md into final ZIP
- `xpn_cover_art.py` — generates cover art from engine accent color + pack name

**Manual (human judgment required):**
- Sound design pass: selecting which presets/samples make the cut
- Naming: pack name, pad labels, walkthrough copy
- Pack walkthrough video (short, beat-in-context, recorded before upload)
- Patreon post copy: title, description, embedded clips, delivery link

### Minimum Viable Pack Definition

A pack ships when it passes all 10 `xpn_pack_qa_report.py` checks **plus** these editorial gates:
1. At least 64 pads with assigned sounds (no empty pads in the primary kit)
2. At least one velocity layer with meaningful timbre shift (not just volume)
3. Cover art generated and reviewed
4. MPCE_SETUP.md bundled (not linked)
5. At least one 30-second beat-in-context clip recorded
6. Pack has been loaded on a physical MPC and verified to play back correctly

---

## 4. Back-Catalog Strategy

**Launch requirement:** 3 packs in the vault before Patreon goes live. Deep Water at $15/month has zero value proposition if the vault is empty on day one.

**Recommended target:** 5 packs before launch. This gives Deep Water members an immediate $40–$60 value at Gumroad prices against a $15 first payment — clear ROI.

### Recommended First 5 Packs (ordered by production priority)

| # | Pack Name | Engine(s) | Rationale |
|---|-----------|-----------|-----------|
| 1 | TIDE TABLES | ODDFELIX + ODDOSCAR | Already exists as free pack — vault anchor, familiar to Signal users |
| 2 | MACHINE GUN REEF | ONSET | Drum engine — most universally useful to MPC producers; referenced in customer journey map |
| 3 | DEEP CURRENT | OPAL + OCEANIC | Granular + physical modeling — atmospheric, distinct from drum pack |
| 4 | SIGNAL FIRE | OVERWORLD + OVERDUB | Chip + dub — lo-fi adjacent, high demand in beat community |
| 5 | ABYSSAL | OBESE + OVERBITE | Bass-forward — covers the low-end use case; OBESE = MOJO macro showcase |

These 5 packs span drums, bass, atmosphere, lo-fi, and granular — a complete production toolkit. A producer who buys into Deep Water on launch day can make a full record.

---

## 5. Analytics + Iteration

### Metrics That Matter

| Metric | Target | Source |
|--------|--------|--------|
| Free → Current conversion rate | ≥5% of active TIDE TABLES downloaders | Patreon dashboard + Gumroad download count |
| Current → Deep Water upgrade rate | ≥15% of Current subscribers | Patreon dashboard |
| Monthly churn (Current) | ≤8% | Patreon dashboard |
| Pack download rate (Current tier) | ≥80% download within 7 days | Patreon file stats |
| Gumroad revenue per pack (post-exclusivity) | Track month-over-month trend | Gumroad analytics |
| Return visit within 7 days of download | Proxy for first-experience success | XO-OX.org analytics |

### XO_OX Tools That Feed Patreon Decisions

- `xpn_pack_qa_report.py` — post-release: track which checks nearly failed. Patterns indicate where the production process needs tightening.
- `xpn_customer_journey_map_rnd.md` — Stage 4 (Investment) and Stage 5 (Habituation) map directly to Current and Deep Water behavior. Churn likely happens at the Stage 4 → 5 transition; if a producer doesn't integrate a pack into their workflow within 2 weeks, they cancel.
- Sonic DNA tagging on packs: track which DNA profiles (high aggression, high space, etc.) correlate with higher download rates. Use this to bias pack selection toward what the audience actually uses.

### Iteration Loop

Monthly cadence, after each pack drop:
1. Check download rate at 7 days. Below 60% = the pack underperformed on first impression — review naming, cover art, and clip quality.
2. Check churn that month. Spike = something in the pack or the announcement missed. Review the walkthrough and description.
3. Check Gumroad sales at 30 days (post-exclusivity). Low sales = the exclusivity window worked (subscribers bought in); high sales = the broader audience wants this sound, which validates the pack direction.
4. Feed voting data (Deep Water) into next quarter's pack pipeline. If 60%+ of Deep Water votes go to one engine, that engine leads the next pack.

---

## Open Questions

- **Patreon URL activation:** DONE 2026-03-22 — https://www.patreon.com/cw/XO_OX is live and configured.
- **File delivery method:** Patreon native file attachments vs. Gumroad private link embedded in post. Gumroad is preferable for file size and download reliability.
- **iOS timing:** XOceanus AUv3 is in scope for V1. If iOS ships before Patreon launch, consider whether XPN packs load cleanly on MPC app — validate before promoting iOS support to subscribers.
