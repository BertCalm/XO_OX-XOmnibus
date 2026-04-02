# XOceanus Board Governance Audit — 2026-03-24

**Prepared by:** XO_OX Board of Directors (Overnight Audit Session)
**Scope:** V1 Launch Readiness — 4 Pillars: Brand, Security, Standards, Launch
**Date:** 2026-03-24
**Repo:** `~/Documents/GitHub/XO_OX-XOceanus/`

---

## Summary Table

| ID | Pillar | Finding | Severity | Status |
|----|--------|---------|----------|--------|
| B-001 | Brand | "XOceanus" in `CONTRIBUTING.md` auval commands (old AU code, wrong case) | P1 | Open |
| B-002 | Brand | `site/updates.html` has one XOceanus reference — in correct narrative context | P3 | Informational |
| B-003 | Brand | `Docs/design/xoceanus_design_guidelines.md` — filename and one footer line use old name | P3 | Open |
| B-004 | Brand | `site/index.html` missing Space Grotesk font — loaded only on aquarium and updates pages | P2 | Open |
| B-005 | Brand | AU identifier still `Xomn` / `XoOx` — CMakeLists updated to `Xolk`; docs not yet updated | P1 | Partially Resolved |
| B-006 | Brand | Source code and CMakeLists are fully XOceanus-clean (0 XOceanus references) | — | PASS |
| B-007 | Brand | design-tokens.css fully reflects brand packet: Abyssal `#080B1A`, XO Gold `#E9C46A`, depth system, all 3 typefaces | — | PASS |
| S-001 | Security | No hardcoded API keys, secrets, or tokens found in source code | — | PASS |
| S-002 | Security | `SecureKeyStore.h` uses AES-256 + macOS Keychain — architecture is sound | — | PASS |
| S-003 | Security | Supabase anon key is injected at runtime via `VaultConfig` struct (not hardcoded) | — | PASS |
| S-004 | Security | `.gitignore` missing `*.pem` companion: `server.key`, `*.cert`, `credentials.json`, `secrets/` | P2 | Open |
| S-005 | Security | `SECURITY.md` exists and is complete. Responsible disclosure email set. | — | PASS |
| S-006 | Security | Git history clean audit incomplete — formal `git log -S` secret scan not documented | P1 | Open |
| T-001 | Standards | 18 engine directories missing `.cpp` stub files (header-only is correct per architecture rules) | — | PASS (by design) |
| T-002 | Standards | Engine file naming is inconsistent: `OperaAdapter.cpp` vs `ObrixEngine.cpp` vs `OsmosisEngine.h` | P2 | Open |
| T-003 | Standards | Preset schema is consistent across sampled files (`schema_version`, `name`, `mood`, `dna`, `macroLabels`) | — | PASS |
| T-004 | Standards | `Presets/XOceanus/Oxytocin/` is a stray directory with mood-based subdirectories — not a valid mood dir | P2 | Open |
| T-005 | Standards | `CONTRIBUTING.md`, `SECURITY.md`, `CODE_OF_CONDUCT.md` all exist. GitHub issue templates exist. | — | PASS |
| T-006 | Standards | `CONTRIBUTING.md` cites `auval -v aumu Xomn Xoox` — both wrong code (`Xomn` not `Xolk`) and wrong case (`Xoox` not `XoOx`) | P1 | Open |
| L-001 | Launch | V1 launch plan `v1-launch-plan-2026-03-20.md` hard gates are all still "PENDING" as-written in that doc | P0 | Needs verification |
| L-002 | Launch | No V1.0 git tag exists — only `archive/v1-launch-prep` tag | P0 | Open |
| L-003 | Launch | Download page: `site/index.html` links to GitHub releases (`BertCalm/XO_OX-XOceanus/releases`) — no release exists | P0 | Open |
| L-004 | Launch | Patreon URL (`www.patreon.com/cw/XO_OX`) appears 43 times across site HTML. Per MEMORY.md this IS the live URL — but `Docs/patreon-url-sweep-2026-03-20.md` still documents it as placeholder | P2 | Needs verification |
| L-005 | Launch | No press kit folder exists at `site/press-kit/` — required by v1-launch-plan | P1 | Open |
| L-006 | Launch | No hero preset audio clips on site — 71 required (Phase 0 aquarium), 5 required for download page | P1 | Open |
| L-007 | Launch | `Presets/XOceanus/` has 16 mood directories (including the stray `Oxytocin/` dir) vs spec's 15 canonical moods | P2 | Open |

---

## Pillar 1: Brand Consistency

### Finding B-001 — P1: CONTRIBUTING.md contains stale AU identifier

`CONTRIBUTING.md` lines 29 and 81 instruct contributors to run:

```
auval -v aumu Xomn Xoox
```

This is wrong on two counts:

1. **Wrong plugin code**: `CMakeLists.txt` now uses `PLUGIN_CODE Xolk` (changed as part of the XOceanus rename, per `Docs/rebrand-xoceanus-rac-review.md`). The correct auval command is `aumu Xolk XoOx`.
2. **Wrong case**: `Xoox` should be `XoOx`. The four-character codes are case-sensitive. The build verification log (`Docs/build-logs/build_verification_11j.md`) explicitly flags this: "codes documented elsewhere as `XOmn`/`XOox` do not match the binary — use `Xomn`/`XoOx`." The code has now moved to `Xolk` and case must be `XoOx`.

This is a P1 because any contributor following CONTRIBUTING.md will run a failing auval command and may incorrectly conclude the build is broken.

**Fix required:** Update `CONTRIBUTING.md` lines 29 and 81 to `auval -v aumu Xolk XoOx`.

Also update all docs that still cite `aumu Xomn XoOx` — these are now stale post-rename: `Docs/v1-launch-plan-2026-03-20.md` (line 139), `Docs/plans/xoceanus_v1_launch_master_plan.md`, and `Docs/build-logs/` files.

---

### Finding B-002 — P3: updates.html XOceanus reference is intentional

`site/updates.html` line 752:

> "The old name was XOceanus. It meant 'everything bus' — a transportation metaphor. Accurate. Utilitarian. Completely wrong."

This is **correct brand narrative** — the rename story. No action required. Verified as intentional.

---

### Finding B-003 — P3: xoceanus_design_guidelines.md filename and footer

`Docs/design/xoceanus_design_guidelines.md` has one XOceanus occurrence in its own footer (line 836):

> *"This document was compiled from all design-related sources in the XO_OX-XOceanus repo on 2026-03-17."*

The filename itself (`xoceanus_design_guidelines.md`) predates the rename. The document is internal-only (not linked from the site) and has been superseded by `Docs/design/xoceanus-definitive-ui-spec.md`. No public exposure. Recommend renaming the file and updating the footer in a cleanup pass, but this is not ship-blocking.

---

### Finding B-004 — P2: Space Grotesk missing from site/index.html

The brand packet specifies Space Grotesk as the primary display typeface. The `design-tokens.css` file correctly defines `--font-display: 'Space Grotesk'` and uses it throughout. However:

- `site/aquarium.html`: loads Space Grotesk via Google Fonts — PASS
- `site/updates.html`: loads Space Grotesk via Google Fonts — PASS
- `site/index.html`: does **NOT** load Space Grotesk — only loads `Inter` and `JetBrains Mono`

The Google Fonts link on `site/index.html` (line 32):
```html
<link href="https://fonts.googleapis.com/css2?family=Inter:wght@400;500;600&family=JetBrains+Mono:wght@400;500&display=swap" rel="stylesheet">
```

Space Grotesk is absent. Any CSS token referencing `--font-display` on the index page will fall back to `system-ui`. The homepage is the brand's primary first impression — this is a P2.

**Fix:** Add `&family=Space+Grotesk:wght@500;700` to the Google Fonts URL on `site/index.html`.

---

### Finding B-005 — P1: AU identifier stale across documentation

The `CMakeLists.txt` correctly shows the post-rename identifier:
```
PLUGIN_MANUFACTURER_CODE XoOx
PLUGIN_CODE Xolk
```

However, multiple documentation files still reference the old `Xomn` code as authoritative (not as historical records):
- `Docs/v1-launch-plan-2026-03-20.md` line 139: `auval -v aumu Xomn XoOx`
- `Docs/plans/xoceanus_v1_launch_master_plan.md` lines 29, 288, 338
- `Docs/build-logs/build_verification_12j.md` (could confuse contributors checking historical context)

The `Docs/rebrand-xoceanus-rac-review.md` correctly documents the change from `Xomn` to `Xolk` and the rationale ("Must never change again after V1"). The source of truth is correct; the documentation is stale.

**Verdict on AU identifier change:** The `Xomn` → `Xolk` change is correctly locked pre-V1. Post-V1 this code must never change.

---

### Findings B-006, B-007 — PASS

Source code is fully XOceanus-clean: zero XOceanus references across all `Source/` files. CMakeLists.txt uses `XOceanus` throughout.

The `design-tokens.css` fully reflects the brand packet's visual identity:
- Abyssal `#080B1A` — present as `--xo-abyssal`
- Pelagic `#111B33` — present as `--xo-pelagic`
- XO Gold `#E9C46A` — present as `--xo-gold`
- Cyan Luminescence `#00E5FF` — present as `--bio-cyan`
- Depth gradient system (surface → mesopelagic → bathypelagic → abyssal → hadal) — present
- Space Grotesk, Inter, JetBrains Mono — defined in token system

---

## Pillar 2: Security

### Finding S-001, S-002, S-003 — PASS

Full audit of `Source/` found zero hardcoded API keys, JWT tokens, passwords, or credentials. The AI feature uses a well-architected `SecureKeyStore.h` with:
- AES-256 encryption with a device-derived key
- macOS Keychain integration
- Keys exist in plaintext only during API calls, then memory-zeroed
- Keys are never logged or included in crash reports

The Supabase integration (`SharedRecipeVault.h`, `CommunityInsights.h`) uses a `VaultConfig` struct populated at runtime — no credentials are hardcoded in source.

---

### Finding S-004 — P2: .gitignore has gaps

Current `.gitignore` covers:
- `.env` — present
- `*.pem` — present
- `*.key` — present

Missing common credential file patterns that should be added as a precaution:
- `*.cert` / `*.crt`
- `credentials.json` (common OAuth / service account format)
- `secrets/` directory (common convention)
- `config.local.*` (common local override pattern)
- `supabase/.env.local` (Supabase CLI creates this)
- `*.p12` / `*.pfx` (certificate bundles)

**Fix:** Expand the "Secrets / local config" section of `.gitignore`.

---

### Finding S-006 — P1: Git history secret scan not formally documented

The v1-launch-plan `Hard Gates` table includes:
> "Security audit — git history clean, no secrets exposed | PENDING"

The `Docs/fleet-audit/architect-audit-session-9.md` confirms source code is clean today. However, no document confirms a `git log -S` / `git grep` scan across the **full commit history** (not just the current working tree). If a secret was committed and subsequently removed in a later commit, it still exists in git history and could be extracted.

**Fix required before V1:** Run `git log --all -S "sk-" --source` and similar patterns against known secret formats (Anthropic, OpenAI, Google, Supabase JWT `eyJ`). Document the result. If any matches are found, use `git filter-repo` to scrub history before making the repo public (per launch plan: "GitHub repo goes public" at 9:05 AM launch day).

---

## Pillar 3: Standards

### Finding T-001 — PASS (by design)

18 engine directories have only `.h` header files and no `.cpp` stubs. This is **correct** per the documented architecture rule (CLAUDE.md line 33): "All DSP lives inline in `.h` headers; `.cpp` files are one-line stubs." The 55 engines that have `.cpp` files follow the stub pattern. The 18 Kitchen Collection engines (Oaken, Obelisk, Ochre, Octave, Ogre, Olate, Oleg, Omega, Opaline, Oto, Otis, Oven, Overcast, Overflow, Overwash, Overworn, and others) appear to be header-only, which is architecturally permitted.

---

### Finding T-002 — P2: Engine adapter file naming is inconsistent

Sampling 6 engine directories reveals two distinct naming conventions:

| Engine | Files | Convention |
|--------|-------|-----------|
| Obrix | `ObrixEngine.h`, `ObrixEngine.cpp` | `{Name}Engine.{ext}` |
| Opera | `OperaAdapter.h`, `OperaAdapter.cpp`, `OperaEngine.h`, `OperaBreathEngine.h`, `OperaConductor.h`, ... | `{Name}Adapter.{ext}` + sub-modules |
| Oware | `OwareEngine.h`, `OwareEngine.cpp` | `{Name}Engine.{ext}` |
| Ostinato | `OstinatoEngine.h`, `OstinatoEngine.cpp` | `{Name}Engine.{ext}` |
| Octopus | `OctopusEngine.h`, `OctopusEngine.cpp` | `{Name}Engine.{ext}` |
| Osmosis | `OsmosisEngine.h` (no .cpp) | `{Name}Engine.h` only |
| Outlook | `OutlookEngine.h` (no .cpp) | `{Name}Engine.h` only |

The majority use `{Name}Engine.h` / `{Name}Engine.cpp`. Opera is the outlier using `Adapter` nomenclature (likely because it was added via the JUCE integration pattern). This is an aesthetic inconsistency — it does not affect build correctness — but creates confusion for new contributors looking to add engines.

**Recommendation:** Document in CONTRIBUTING.md that the canonical pattern is `{Name}Engine.h` + `{Name}Engine.cpp` (one-line stub). The Opera adapter structure is a documented exception.

---

### Finding T-003 — PASS

Preset schema is consistent across three sampled files from different moods:
- `Aether/OpenSky_TB_Cirrostratus.xometa`: `schema_version`, `name`, `mood`, `engines`, `author`, `version`, `description`, `tags`, `macroLabels`, `dna` — all present
- `Coupling/Coupling_Chaos_Partials.xometa`: same structure with `couplingIntensity` added for Entangled presets
- `Foundation/Abyssal_Root.xometa`: same structure (author at top vs bottom is a key ordering difference, not a schema violation)

DNA fields (brightness, warmth, movement, density, space, aggression) are uniformly present. MacroLabels present in all. Schema is healthy.

---

### Finding T-004 — P2: Stray `Presets/XOceanus/Oxytocin/` directory

The canonical preset mood directories (per CLAUDE.md and the product spec) are 15: Foundation, Atmosphere, Entangled, Prism, Flux, Aether, Family, Submerged, Coupling, Crystalline, Deep, Ethereal, Kinetic, Luminous, Organic.

`Presets/XOceanus/` contains a 16th directory: `Oxytocin/`. Inspection reveals this is itself a nested structure containing mood-based subdirectories (`Atmosphere/`, `Entangled/`, `Ethereal/`, `Flux/`, `Foundation/`, `Kinetic/`, `Luminous/`, `Submerged/`) with preset `.xometa` files inside them. This is a **non-standard layout** — OXYTOCIN presets that should be in the canonical mood directories are instead organized under a per-engine subdirectory.

This creates two problems:
1. The preset browser will likely not surface these presets unless it explicitly handles this nested path format.
2. It sets a precedent for per-engine directories that conflicts with the mood-based organizational scheme all other presets follow.

**Fix:** Flatten the Oxytocin presets — move `Presets/XOceanus/Oxytocin/Atmosphere/*.xometa` → `Presets/XOceanus/Atmosphere/`, and so on for each mood, then remove the empty `Oxytocin/` directory.

---

### Finding T-005 — PASS

Community infrastructure files verified present and substantive:
- `README.md`: Complete — XOceanus branding, 73-engine table, mythology narrative, mission statement. No stale XOceanus references.
- `CONTRIBUTING.md`: Present with build setup, environment instructions, preset submission guidelines. (Note B-001 above for the auval command error.)
- `SECURITY.md`: Present with responsible disclosure process, scope, and supported versions.
- `CODE_OF_CONDUCT.md`: Contributor Covenant — complete, with enforcement contact `conduct@xo-ox.org`.
- `.github/ISSUE_TEMPLATE/`: `bug_report.md` and `feature_request.md` both present.
- `.github/workflows/`: `build.yml` and `cleanup-branches.yml` present.

---

### Finding T-006 — P1: CONTRIBUTING.md auval command is wrong

See B-001 above. Same finding, both brand and standards impact. The auval command in CONTRIBUTING.md uses the old plugin code `Xomn` and wrong case `Xoox`. The correct command is `auval -v aumu Xolk XoOx`.

---

## Pillar 4: Launch Readiness

### Finding L-001 — P0: V1 Launch Plan hard gates need status verification

`Docs/v1-launch-plan-2026-03-20.md` lists 8 hard gates, all marked "PENDING" as written. Some of these have been addressed since the document was written (community infrastructure now exists; ORBWEAVE has presets; engine count is now 73 not 42). However the document has not been updated, and the Board cannot confirm gate status from the document itself.

**Required action:** Conduct a line-by-line status update on each hard gate. The following can be confirmed as RESOLVED based on this audit:
- "Community infrastructure — README, CONTRIBUTING, SECURITY, CODE_OF_CONDUCT, issue templates" — RESOLVED
- "All materials say '46 engines' consistently" — PARTIALLY RESOLVED (all say 73 now, but the launch plan doc itself still says 46)

The following require manual verification by the developer:
- "Security audit — git history clean" — see S-006
- "Patreon URL" — see L-004
- "ORBWEAVE preset gap" — see L-001 note below
- "Minimum 5 hero preset audio clips on XO-OX.org" — see L-006
- "Download page live" — see L-003
- "Preset silence audit" — no evidence of completion

**ORBWEAVE note:** The launch plan (written 2026-03-20) flagged ORBWEAVE as having 0 presets. Per MEMORY.md and seance records, ORBWEAVE has since received presets. Confirm count before launch.

---

### Finding L-002 — P0: No V1.0 release tag exists

The git repository has one tag: `archive/v1-launch-prep`. There is no `v1.0.0` tag.

Per the launch plan, "GitHub repo goes public" and "tag v1.0.0" happen at 9:05 AM launch day. This is a pre-launch action, not a current blocker — but it must be on the launch day checklist.

The download button in `site/index.html` links to:
```
https://github.com/BertCalm/XO_OX-XOceanus/releases
```

This page will show "no releases" until the tag is created. The link must be live before users are sent to it.

---

### Finding L-003 — P0: Download page links to nonexistent releases

The download section in `site/index.html` (line 2585) links to:
```
https://github.com/BertCalm/XO_OX-XOceanus/releases
```

As of this audit there are no public GitHub releases at that URL. The download CTA is the single most critical user action the site exists to drive. A broken download link on launch day is a P0 — any coverage or traffic directed to the site will hit a dead end.

**Pre-launch requirement:**
1. Tag `v1.0.0` in git
2. Create a GitHub release with the compiled AU bundle (ZIP) and release notes
3. Verify the download link resolves to the release before any public promotion

---

### Finding L-004 — P2: Patreon URL status ambiguous

The site uses `https://www.patreon.com/cw/XO_OX` (43 occurrences across site HTML). MEMORY.md states "DONE — https://www.patreon.com/cw/XO_OX" indicating the Patreon account is live. However, `Docs/patreon-url-sweep-2026-03-20.md` still categorizes it as a placeholder, and the launch plan hard gate says "PENDING."

**Action required:** Verify the Patreon page is publicly visible and the tiers (Explorer $3, Diver $8, Bathynaut $20) are active and accessible. If live, mark this gate closed in the launch plan.

---

### Finding L-005 — P1: Press kit does not exist on site

The v1-launch-plan requires a `/press-kit/` folder at XO-OX.org containing:
- `xoceanus-logo-light.svg` + `xoceanus-logo-dark.svg`
- Hero screenshots (4K)
- `one-sheet.pdf`
- `fact-sheet.md`
- Audio demo ZIP (5 clips)

No `press-kit/` directory exists in `site/`. The press outreach scheduled for 2:00 PM launch day ("email to press — Sonic State, Bedroom Producers Blog, Attack Magazine") cannot proceed without this collateral.

---

### Finding L-006 — P1: No hero audio clips on site

The aquarium Phase 0 requires 71 hero audio clips (one per engine) for click-to-play. The launch plan requires a minimum of 5 audio demos embedded on the download page. As of this audit no `.wav` or audio files are present in `site/`. This is a manual recording task (noted in MEMORY.md as ongoing since at least 2026-03-20).

The Aquarium at `site/aquarium.html` is a core brand showcase. Without audio clips it functions as a visual gallery only — the "click a creature to hear its voice" interaction (described in the brand packet as the product's key demonstration) is non-functional.

This is a manual task that cannot be automated. It requires Logic Pro sessions and microphone/DAW recording. Priority: record at minimum the 6 Abyssal Lords (OXYTOCIN, OVERBITE, OWARE, OBSCURA, OUROBOROS, OXBOW) for a minimal viable launch.

---

### Finding L-007 — P2: Stray preset directory creates 16th mood

See T-004. The `Presets/XOceanus/Oxytocin/` directory is the direct symptom. Beyond the organizational issue, if the product lists 15 moods but the filesystem has 16 directories, any script or UI that builds a mood list from the filesystem will display "Oxytocin" as a mood — which it is not.

---

## Recommendations by Priority

### P0 — Do Before Any Public Promotion

1. **L-003** Create the GitHub v1.0.0 release with compiled AU bundle. Verify download link is live.
2. **L-001** Conduct formal hard gate status review against the v1-launch-plan. Sign off each item.
3. **L-002** Tag `v1.0.0` in git, push tag to remote. Do not promote publicly until tag exists.

### P1 — Do Before Launch Day

4. **B-001 / T-006** Fix `CONTRIBUTING.md` auval command to `auval -v aumu Xolk XoOx`.
5. **B-005** Update all documentation references to the old AU code (`Xomn`) to reflect the current `Xolk` identifier.
6. **S-006** Run formal git history secret scan. Document result. Scrub history if any secrets found before making repo public.
7. **L-005** Create `site/press-kit/` with minimum viable press kit collateral.
8. **L-006** Record minimum 6 Abyssal Lord hero audio clips for aquarium and download page.

### P2 — Do Before or Shortly After Launch

9. **B-004** Add Space Grotesk to `site/index.html` Google Fonts link.
10. **S-004** Expand `.gitignore` with missing credential file patterns.
11. **T-002** Document the `{Name}Engine.h` naming standard in CONTRIBUTING.md. Note Opera as a documented exception.
12. **T-004 / L-007** Flatten `Presets/XOceanus/Oxytocin/` nested structure into the canonical 15 mood directories.
13. **L-004** Verify Patreon URL is live and tiers are active. Update launch plan gate status.

### P3 — Nice to Have

14. **B-002** No action required on `updates.html` — narrative context is correct.
15. **B-003** Rename `Docs/design/xoceanus_design_guidelines.md` to `xoceanus_design_guidelines.md` and update its footer in a cleanup pass.

---

## Overall Assessment

The product is architecturally solid and the brand rename is substantially complete. Source code and the CSS design system are fully aligned with the XOceanus brand. Community infrastructure (README, CONTRIBUTING, SECURITY, CODE_OF_CONDUCT, issue templates) is in place — a genuine strength relative to the launch plan's concern.

The primary V1 launch risk is not technical quality — it is readiness infrastructure: no GitHub release exists, the download link is dead, no press kit exists, and no audio clips are recorded. These are all actionable and none require code changes. The competitive window (month 5 of 6-12) makes urgency appropriate.

The AU identifier change from `Xomn` to `Xolk` is correctly implemented in the build system and must never be changed after V1 ships. The documentation inconsistency around auval commands should be resolved before any contributor attempts to validate the build.

**Recommend: Proceed to launch preparation sprint. Clear P0 items first, then P1. Target launch within 2 weeks.**

---

*Board Governance Audit — XO_OX Board of Directors*
*2026-03-24 — Overnight session*
*"The deep opens. Make sure the door works."*
