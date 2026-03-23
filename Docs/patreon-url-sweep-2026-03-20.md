# Patreon URL Sweep — 2026-03-20

Sweep of all XO_OX repos and config files for Patreon references.
Placeholder URL in use: `patreon.com/xoox` (not live — account not yet created).

**COMPLETED 2026-03-22.** Patreon account live at https://www.patreon.com/cw/XO_OX. All placeholder URLs replaced across all sections.

---

## Section 1 — LIVE SITE FILES (highest priority — user-facing broken links)

These files contain `https://patreon.com/xoox` as actual href values. Users clicking these links land on a broken or wrong page.

| File | Line | Current Text | Action |
|------|------|-------------|--------|
| `/Users/joshuacramblet/Documents/GitHub/XO_OX-XOmnibus/site/index.html` | 2340 | `<a href="https://patreon.com/xoox" class="patreon-btn" target="_blank" rel="noopener">Support on Patreon</a>` | Replace URL with real Patreon URL |
| `/Users/joshuacramblet/Documents/GitHub/XO_OX-XOmnibus/site/index.html` | 2380 | `<a href="https://patreon.com/xoox" target="_blank" rel="noopener">Patreon</a>` | Replace URL with real Patreon URL |

**Real URL to use:** `TODO — pending Patreon account creation`

---

## Section 2 — DOCS / SPECS (internal planning docs that reference the placeholder)

These mention `patreon.com/xoox` explicitly as a placeholder. Update once the real URL is known, or leave as-is since they document the placeholder status.

| File | Line | Current Text |
|------|------|-------------|
| `/Users/joshuacramblet/Documents/GitHub/XO_OX-XOmnibus/Docs/plans/xomnibus_v1_launch_master_plan.md` | 46 | `Patreon URL is placeholder \`patreon.com/xoox\` across site, README, inserts (BLOCKER #1 in existing checklist)` |
| `/Users/joshuacramblet/Documents/GitHub/XO_OX-XOmnibus/Docs/plans/xomnibus_v1_launch_master_plan.md` | 101 | `- [ ] **[BLOCKER] Patreon URL.** Set up real Patreon account, update the URL everywhere: \`site/index.html\`, any pack inserts, README. Search the entire repo for \`patreon.com/xoox\` and replace all occurrences.` |
| `/Users/joshuacramblet/Documents/GitHub/XO_OX-XOmnibus/Docs/sprint_report_2026-03-18.md` | 150 | `- Patreon URL placeholder (\`patreon.com/xoox\`) — needs real URL before launch` |
| `/Users/joshuacramblet/Documents/GitHub/XO_OX-XOmnibus/Docs/morning_plan_2026-03-20.md` | 94 | `\| **Patreon URL** \| Manual \| Still placeholder \`patreon.com/xoox\` \|` |
| `/Users/joshuacramblet/Documents/GitHub/XO_OX-XOmnibus/Docs/specs/xpn_launch_checklist_v1_rnd.md` | 12 | `- [ ] **1. Patreon URL updated.** Replace placeholder \`patreon.com/xoox\` with the real URL across the site, README files, and any pack inserts.` |
| `/Users/joshuacramblet/Documents/GitHub/XO_OX-XOmnibus/Docs/specs/xpn_patreon_pack_strategy_rnd.md` | 11 | `Patreon URL: \`patreon.com/xoox\` (placeholder — not live)` |
| `/Users/joshuacramblet/Documents/GitHub/XO_OX-XOmnibus/Docs/specs/xpn_patreon_pack_strategy_rnd.md` | 141 | `- **Patreon URL activation:** \`patreon.com/xoox\` must be claimed and configured before any public announcement.` |
| `/Users/joshuacramblet/Documents/GitHub/XO_OX-XOmnibus/Docs/specs/xpn_patreon_content_calendar_rnd.md` | 248 | `- **Patreon URL:** Currently placeholder \`patreon.com/xoox\` — finalize and update all site references before April launch` |
| `/Users/joshuacramblet/Documents/GitHub/XO_OX-XOmnibus/Docs/specs/xpn_patreon_q2_2026_calendar_rnd.md` | 14 | `**Patreon URL:** patreon.com/xoox (placeholder — update before April 1 launch post)` |
| `/Users/joshuacramblet/Documents/GitHub/XO_OX-XOmnibus/Docs/specs/xpn_patreon_q2_2026_calendar_rnd.md` | 211 | `- Confirm and update Patreon URL (patreon.com/xoox is placeholder)` |
| `/Users/joshuacramblet/Documents/GitHub/XO_OX-XOmnibus/Docs/specs/oxport_v2_feature_backlog.md` | 370 | `**Blocking dependencies:** Patreon API access (currently placeholder \`patreon.com/xoox\`);` |

---

## Section 3 — TOOLS / CODE (Python scripts with placeholder in checklist strings)

These contain the placeholder inside checklist strings that get written into generated output. Update when real URL is known so generated pack release notes carry the correct link.

| File | Line | Current Text |
|------|------|-------------|
| `/Users/joshuacramblet/Documents/GitHub/XO_OX-XOmnibus/Tools/xpn_tide_tables_pack_builder.py` | 294 | `"[ ] Update patreon.com/xoox bio link to point to TIDE TABLES landing page"` |
| `/Users/joshuacramblet/Documents/GitHub/XO_OX-XOmnibus/Docs/packs/tide_tables/tide_tables_spec.json` | 734 | `"[ ] Update patreon.com/xoox bio link to point to TIDE TABLES landing page"` |

---

## Section 4 — MEMORY / CONFIG FILES (cross-repo)

These carry the reminder that the URL is a placeholder. Update once real URL is set.

| File | Line | Current Text |
|------|------|-------------|
| `/Users/joshuacramblet/Documents/GitHub/xo-ox-claude-config/memory/MEMORY.md` | 118 | `- **PENDING**: Update Patreon URL (currently placeholder \`patreon.com/xoox\`)` |
| `/Users/joshuacramblet/.claude/projects/-Users-joshuacramblet/memory/MEMORY.md` | 120 | `- **PENDING**: Update Patreon URL (currently placeholder \`patreon.com/xoox\`)` |
| `/Users/joshuacramblet/.claude/projects/-Users-joshuacramblet/memory/v1-scope-decision-march-2026.md` | 57 | `- Fix Patreon URL (not patreon.com/xoox)` |

---

## Section 5 — EXCLUDED (not XO_OX-owned, no action needed)

These contain `patreon.com` but reference third-party package authors, not XO_OX:

- `/Users/joshuacramblet/Documents/GitHub/XO_OX-XOmnibus/Libs/JUCE/examples/Plugins/WebViewPluginDemoGUI/package-lock.json` — multiple entries for `patreon.com/feross`, `patreon.com/infusion`, `patreon.com/mdevils` (npm package author funding links, not XO_OX)

---

## Section 6 — GENERIC PATREON MENTIONS (no URL to replace)

These mention "Patreon" as a concept/channel without a specific URL. No action needed here — they'll remain accurate regardless of what the final URL is.

Files with generic mentions (not exhaustive — these are contextual references in strategy docs, ebook chapters, skills):
- `Docs/ebook/chapter_06_pack_economy.md` — "6.6 Patreon Membership" section (narrative, no URL)
- `Docs/ebook/chapter_01_xpn_format.md` — "goes to Patreon or the XO-OX.org download page" (no URL)
- `Docs/specs/field_guide_posts_pipeline.md` — "Patreon companion:" labels (no URL)
- `Docs/specs/site_content_strategy_rnd.md` — "emerging Patreon community layer" (no URL)
- `Docs/specs/xpn_social_proof_community_rnd.md` — "Patreon Community Mechanics" section (no URL)
- `Docs/specs/xpn_collection_first_five_packs_rnd.md` — "Patreon commitment" (no URL)
- `Docs/concepts/artwork_collection_overview.md` — "Patreon-exclusive" zine mention (no URL)
- `xo-ox-claude-config/skills/patreon-content-manager/SKILL.md` — skill definition (no URL)
- `xo-ox-claude-config/skills/consultant/SKILL.md` — mentions Patreon tiers as concept (no URL)
- `xo-ox-claude-config/skills/ringleader/SKILL.md` — skill roster entry (no URL)
- `xo-ox-claude-config/skills/launch-coordinator/SKILL.md` — launch checklist step (no URL)
- `xo-ox-claude-config/skills/community/SKILL.md` — community funnel mention (no URL)
- `xo-ox-claude-config/memory/skill-ecosystem.md` — "Patreon posts written in XOmnibus/patreon/" (path reference, no URL)

---

## Replacement Plan (execute once real URL is confirmed)

**Step 1 — Site (immediate public impact):**
```
# In site/index.html — replace both occurrences:
patreon.com/xoox  →  patreon.com/REAL_HANDLE
```

**Step 2 — Docs / Specs (cleanup pass):**
Run a repo-wide find-and-replace for `patreon.com/xoox` → real URL across all `.md`, `.json`, `.py` files in XO_OX-XOmnibus (excluding `Libs/JUCE/`).

**Step 3 — Memory files:**
Update both MEMORY.md files to remove the PENDING note and replace with the confirmed URL.

**Step 4 — Python tool:**
Update `Tools/xpn_tide_tables_pack_builder.py` line 294 so generated release checklists carry the real URL.

---

*Generated by sweep on 2026-03-20. Do not modify source files until real Patreon URL is confirmed.*
