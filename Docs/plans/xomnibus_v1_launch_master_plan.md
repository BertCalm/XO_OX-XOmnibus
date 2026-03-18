# XOmnibus V1 — Launch Coordinator's Master Timeline

**Document type:** Operational master plan
**Owner:** Launch Coordinator (XO_OX Designs)
**Version:** 1.0 — 2026-03-17
**Launch target:** L+0 (exact date TBD — set L once all L-7 gates are green)
**Scope:** XOmnibus plugin (AU/Standalone, macOS) + TIDE TABLES (free XPN) + MACHINE GUN REEF ($15 XPN)

---

## How to Read This Document

- **L** = launch day (the day the GitHub repo goes public and packs go live)
- **L-28** = 28 days before launch, etc.
- **Gates** are hard stops. No phase advances until its gate criteria are met.
- Items marked **[BLOCKER]** must be complete before L. No exceptions.
- Items marked **[IMPORTANT]** should be complete by L; may launch within 48 hours if unavoidable.
- Items marked **[NICE]** add momentum but do not gate launch.
- Responsibility abbreviations: **DEV** = you (development + DSP), **OPS** = you (ops + admin), **CONTENT** = you (writing + social), **MEDIA** = you or a trusted collaborator

---

## Current State (as of 2026-03-17)

### Plugin — green lights

- auval PASS (AU bundle, codes `Xomn`/`XoOx`)
- Build PASS — 0 errors, 7 pre-existing non-blocking warnings
- 34 engines registered; 22 engines fully doctrine-compliant
- 2,550 factory presets, 0 duplicates, 100% Sonic DNA coverage, ~92/100 health score
- All 6 doctrines (D001–D006) resolved fleet-wide
- Prism Sweep 12 rounds complete — release readiness gate PASSED (ref: `Docs/release_readiness_12k.md`)
- 4 concept engines (OSTINATO, OPENSKY, OCEANDEEP, OUIE) approved V1 scope — DSP builds pending

### Plugin — known gaps before public repo

- No `README.md` in repo root
- No `LICENSE` file (GPLv3 intended per `xomnibus_brand_identity_and_launch.md`)
- No `CONTRIBUTING.md`
- No `Community/` folder structure
- Binary release not packaged (no GitHub Release with `.component` bundle)
- 4 concept engines have no source code yet

### Packs — gaps

- Patreon URL is placeholder `patreon.com/xoox` across site, README, inserts (BLOCKER #1 in existing checklist)
- Audio demos not recorded (hardware MPC required)
- TIDE TABLES QA score not confirmed against release candidate
- MACHINE GUN REEF not yet finalized
- Cover art status unknown

### Site — gaps

- XO-OX.org is live with 7 pages
- Missing audio demo embeds (hero preset clips)
- Plugin download page not yet published
- Press kit not published

---

## Phase Overview

| Phase | Window | Theme | Gate |
|-------|--------|-------|------|
| **P0** — Pre-conditions | Now → L-21 | Finish what must be true | All BLOCKERS cleared |
| **P1** — Soft launch prep | L-21 → L-7 | Assets, accounts, content | Go/no-go meeting |
| **P2** — Launch window | L-7 → L-1 | Final checks, staging | Green light or delay |
| **P3** — Launch day | L+0 | Execute the runbook | — |
| **P4** — Post-launch week | L+1 → L+7 | Monitor, respond, hotfix | First patch if needed |
| **P5** — Opening nights | L+7 → L+42 | Weekly engine spotlights | Sustainable cadence |

---

## PHASE 0 — Pre-Conditions (Now → L-21)

Everything in this phase is a prerequisite to any public-facing action. Do not announce, tease, or share links until P0 is complete.

### Plugin repo preparation

- [ ] **[BLOCKER] Create `README.md` in repo root.** Must include: what XOmnibus is (2-paragraph plain-language description), platform requirements (macOS, AU host), download link (to Releases page), quick-start (load a preset, hear coupling), link to Getting Started guide on XO-OX.org, link to Discord, link to CONTRIBUTING.md. Max 4 minutes from landing on the page to knowing what this is. Use the elevator pitch from `xomnibus_brand_identity_and_launch.md` §4.3.
- [ ] **[BLOCKER] Add `LICENSE` file — GPLv3.** Obtain the standard GPLv3 text from gnu.org. One file at repo root. No modifications.
- [ ] **[BLOCKER] Create `CONTRIBUTING.md`.** Sections: contribution types (bug reports, presets, docs, code), preset submission workflow (the Guest Exhibition pipeline), code contribution rules (parameter IDs are frozen, no audio thread allocation, DSP in `.h` headers), how to run the build, where to ask questions. Template exists in `xomnibus_brand_identity_and_launch.md` §7.3 — expand it.
- [ ] **[BLOCKER] Create `Community/` directory with `PRESET_SUBMISSION.md`.** Instructions for submitting a `.xometa` preset via GitHub PR, including schema requirements and naming conventions.
- [ ] **[BLOCKER] Create a GitHub Release for V1.0.** Tag `v1.0.0`. Attach the compiled `.component` (AU bundle) as a release asset. Write release notes: what's included (34 engines, 2,550 presets), platform requirements, install instructions (copy to `/Library/Audio/Plug-Ins/Components/`), known limitations (5 Constellation engine stubs produce no audio, 4 concept engines pending).
- [ ] **[IMPORTANT] Suppress or explain stub engines in UI.** The 5 Constellation family engine stubs (Obbligato, Ohm, Ole, Orphica, Ottoni) register but produce no audio. Decide: hide them from the engine selector, or show them with a "Coming in V1.1" badge. Either way, document the decision in the Release Notes. Do not leave silent engine slots with no explanation for new users.
- [ ] **[IMPORTANT] Verify repo is public-ready.** Confirm: no API keys, no personal credentials, no large binaries that should not be in the repo. Run `git log --all --oneline` and check there is no sensitive history. Check `.gitignore` covers build artifacts.
- [ ] **[NICE] Enable GitHub Discussions** on the repo. Set up categories: Presets, Bug Reports, Feature Requests, Show and Tell, Developer Chat. This supplements Discord for async technical conversations.
- [ ] **[NICE] Add repository topics on GitHub.** Use: `synthesizer`, `audio-plugin`, `juce`, `open-source`, `au-plugin`, `sound-design`, `preset`, `macos`. These drive organic discovery.

### Concept engine DSP builds (4 engines)

- [ ] **[BLOCKER for complete V1]** OSTINATO DSP build — communal drum circle engine. Has design spec at `Docs/xomnibus_new_engine_process.md` and concept at `Docs/concepts/`.
- [ ] **[BLOCKER for complete V1]** OPENSKY DSP build — euphoric shimmer, pure feliX.
- [ ] **[BLOCKER for complete V1]** OCEANDEEP DSP build — abyssal bass, pure Oscar.
- [ ] **[BLOCKER for complete V1]** OUIE DSP build — duophonic hammerhead, STRIFE↔LOVE axis.

**Decision point:** If any concept engine DSP build cannot ship before L, either (a) delay L, or (b) ship V1.0 with 30 engines + clear "V1.1 coming" messaging, and add those 4 engines in a V1.1 patch within 30 days. Do not delay the full launch indefinitely. Set this go/no-go no later than L-21.

### Accounts and infrastructure

- [ ] **[BLOCKER] Patreon URL.** Set up real Patreon account, update the URL everywhere: `site/index.html`, any pack inserts, README. Search the entire repo for `patreon.com/xoox` and replace all occurrences. Verify the real URL resolves on mobile and desktop.
- [ ] **[BLOCKER] Gumroad account set up, payment processing verified.** Create account, connect bank/payout, run a test purchase with a personal card. Confirm receipt email arrives and download link works.
- [ ] **[IMPORTANT] Discord server created and configured.** Channel structure per `xomnibus_brand_identity_and_launch.md` §11.4: announcements, general, preset-sharing, sound-design-tips, coupling-experiments, feature-requests, bug-reports, show-your-work, developer-chat. Set up roles: Visitor, Artist, Curator, Architect. Add invite link to README, CONTRIBUTING.md, and XO-OX.org.
- [ ] **[IMPORTANT] Email capture mechanism active.** Gumroad auto-captures on purchase for paid packs. For TIDE TABLES (free), either route through a Gumroad "free product" checkout or add a separate sign-up. Test the full flow end-to-end.

### XPN pack preparation

- [ ] **[BLOCKER] TIDE TABLES passes `full_qa_runner.py` with score ≥ 70.** Run against release candidate `.xpn` file. Log the score.
- [ ] **[BLOCKER] TIDE TABLES: minimum 3 hardware audio demos recorded.** From actual `.xpn` on hardware MPC. At least one must demonstrate velocity expression. Hardware capture only — no DAW renders.
- [ ] **[BLOCKER] TIDE TABLES cover art finalized.** 800×800px PNG. ONSET accent color `#0066FF` themed. Verify at 300×300 thumbnail size.
- [ ] **[BLOCKER] MACHINE GUN REEF passes `full_qa_runner.py` with score ≥ 80.** Higher bar for paid product.
- [ ] **[BLOCKER] MPCE_SETUP.md present and accurate in both packs.** Verify current MPC firmware compatibility notes.
- [ ] **[BLOCKER] Hardware test on MPC — full clean load.** Download from Gumroad link (not dev filesystem), load pack, test all programs, velocity response, program switching.
- [ ] **[IMPORTANT] Minimum firmware version documented.** If `.xpn` behavior differs between firmware 2.14 and 2.16, document the minimum supported version on the download page and in the README.

---

## Phase 0 Gate — Go/No-Go for P1

**All BLOCKER items in P0 must be checked before proceeding.** Do a written gate review:

```
DATE:
PLUGIN BLOCKERS CLEAR?  [ ] Yes  [ ] No — outstanding:
ACCOUNT BLOCKERS CLEAR? [ ] Yes  [ ] No — outstanding:
PACK BLOCKERS CLEAR?    [ ] Yes  [ ] No — outstanding:
CONCEPT ENGINE DECISION: [ ] All 4 shipping  [ ] V1.1 plan locked
GO / NO-GO: ___________
```

If NO-GO: identify the single longest-pole blocker, estimate completion date, reset L accordingly.

---

## PHASE 1 — Soft Launch Prep (L-21 → L-7)

Assets, content, and pre-launch visibility. Nothing is live for general public yet, but trusted contacts may receive preview access.

### Press kit assembly

- [ ] **[IMPORTANT] Compile press kit directory** at `Assets/press-kit/`:
  ```
  press-kit/
  ├── logos/
  │   ├── xomnibus-logo-light.svg
  │   ├── xomnibus-logo-dark.svg
  │   ├── xomnibus-logomark.svg
  │   └── xo-ox-brandmark.svg
  ├── screenshots/
  │   ├── hero-coupling-view.png        (4K, light mode, 2 engines coupled, active coupling viz)
  │   ├── preset-browser-moods.png      (4K, mood browser open, DNA radar visible)
  │   ├── engine-panel-onset.png        (engine detail, Electric Blue accent)
  │   ├── engine-panel-opal.png         (engine detail, Lavender accent)
  │   └── engine-panel-odyssey.png      (engine detail, Violet accent)
  ├── audio-demos/
  │   └── [3+ hardware captures from TIDE TABLES]
  ├── one-sheet.pdf                     (1 page: product summary, 3 key screenshots, stats, links)
  ├── fact-sheet.md                     (plain text version of one-sheet)
  └── brand-guidelines-summary.pdf      (condensed: colors, fonts, do's and don'ts)
  ```
- [ ] **[IMPORTANT] One-sheet written.** Update the fact sheet template in `xomnibus_brand_identity_and_launch.md` §12.2 with current numbers: 34 engines (30 with DSP, or all 34 if concept builds complete), 2,550 presets, platform, price, GitHub URL, Discord URL, contact email. Print-ready PDF.
- [ ] **[NICE] Screenshots taken at retina resolution.** Light mode, two engines actively coupled. Use `OPAL→ONSET` or `ODYSSEY→OBBLIGATO` as hero coupling — visually distinctive accent colors.

### XO-OX.org content additions

- [ ] **[BLOCKER] XOmnibus plugin download page published.** New page or section on XO-OX.org with: hero screenshot, 3-sentence description, platform requirements, GitHub Release download button, install instructions (drag to Components folder), Discord link. Not a product sales page — a welcome mat.
- [ ] **[BLOCKER] Patreon URL corrected on all site pages.** Check `index.html`, `packs.html`, `guide.html`, `aquarium.html`, `manifesto.html`, `updates.html`.
- [ ] **[IMPORTANT] Audio demo embeds added to site.** At minimum: site home page and packs page get embedded audio (SoundCloud, Bandcamp, or direct HTML5 audio elements). Demos must be hardware-captured.
- [ ] **[IMPORTANT] Pack download pages live and tested on site.** TIDE TABLES page with Gumroad embed. MACHINE GUN REEF page with Gumroad embed. Test the full flow: land → click → Gumroad → download or purchase → file arrives.
- [ ] **[NICE] XO-OX.org Aquarium page updated.** Confirm all 34 engines are mapped in the Water Column Atlas with correct accent colors and depth zones.

### Content pipeline — pre-launch teaser sequence

- [ ] **[IMPORTANT] 4-week social teaser calendar written and scheduled.** Do not post yet — write and queue. Suggested cadence:
  - L-21: "Something is coming." — coupling symbol image, no text beyond tagline
  - L-14: "34 engines. One platform. Free." — hero screenshot, no download link yet
  - L-7: "Launching [DATE]. Free. Open source." — 60s trailer if ready, or coupling demo clip
  - L-1: "Tomorrow." — single image, launch date
- [ ] **[IMPORTANT] Field Guide launch article written.** Article on ONSET velocity and expression (per existing checklist item #9). Schedule to publish simultaneously with L+0 announcement, not a week later.
- [ ] **[IMPORTANT] Reddit posts drafted.** Two posts: (1) r/synthesizers / r/musicproduction — non-promotional, "here's what we built and why," lead with the open-source angle; (2) r/makinghiphop — lead with TIDE TABLES free pack and ONSET origin story. Do not post until L+0. Do not post simultaneously — stagger by 2–3 hours.
- [ ] **[NICE] KVR Audio product listing drafted.** Product description, screenshots, links. Ready to submit at L+0.
- [ ] **[NICE] YouTube walkthrough video produced.** "First 5 Minutes" — download to saving a custom preset. Screen capture + voiceover. Target under 6 minutes. No production value required; clarity and authenticity matter more.
- [ ] **[NICE] YouTube pack walkthrough produced.** For TIDE TABLES: load the pack, play 2 programs, one velocity demo. Hardware + screen capture. No minimum length.

### Beta program (optional but strongly recommended)

- [ ] **[NICE] Identify 3–5 trusted beta testers.** Criteria: MPC owners who can test `.xpn` hardware loading; ideally producers who are not in your immediate circle (fresh eyes). One should be on a different macOS version than your dev machine.
- [ ] **[NICE] Beta access window: L-14 to L-7.** Share the GitHub Release candidate and TIDE TABLES `.xpn` as a private link. Ask specifically: (1) does it install without issues? (2) does the hero preset auto-load? (3) do the macros produce audible change? (4) does the coupling demo click or pop? (5) does TIDE TABLES load clean on MPC from a fresh download?
- [ ] **[NICE] Compile beta findings by L-7.** Fix anything that gates launch. Log anything that doesn't.

---

## Phase 1 Gate — L-7 Final Go/No-Go

```
DATE:
PLUGIN DOWNLOAD PAGE LIVE?         [ ] Yes  [ ] No
LICENSE + README + CONTRIBUTING?   [ ] Yes  [ ] No
GITHUB RELEASE BUILT?              [ ] Yes  [ ] No
GUMROAD TEST PURCHASE PASSED?      [ ] Yes  [ ] No
SOCIAL TEASER QUEUE STAGED?        [ ] Yes  [ ] No
PATREON URL LIVE?                  [ ] Yes  [ ] No
AUDIO DEMOS HARDWARE-CAPTURED?     [ ] Yes  [ ] No
DISCORD SERVER READY?              [ ] Yes  [ ] No

GO / NO-GO: ___________
```

If NO-GO: Do not set L+0 until this gate clears. L-7 is the absolute minimum runway.

---

## PHASE 2 — Launch Window (L-7 → L-1)

All pre-launch content is locked. No new features. Bug fixes and asset finalization only.

- [ ] **L-7:** Lock release candidate. No DSP changes. Tag `v1.0.0-rc1` on GitHub (private until L+0).
- [ ] **L-7:** Run `full_qa_runner.py` one final time on both `.xpn` files using the RC candidate. Log scores. If either pack fails its threshold, fix and re-run — do not proceed to L+0.
- [ ] **L-6:** Final hardware test. Fresh download from the Gumroad staging link. Load both packs on MPC. Sign off or flag.
- [ ] **L-5:** Final site check. All links working. Download page loads. Audio embeds play. Patreon URL resolves. Mobile and desktop.
- [ ] **L-4:** Final press kit review. All screenshots at correct resolution. One-sheet PDF renders cleanly. Fact sheet numbers match the release.
- [ ] **L-3:** Schedule all social posts in the queue tool. Verify the L+0 posts are set to the correct time. Do not manually post on launch day if you can avoid it — automate so you can monitor.
- [ ] **L-2:** Write the launch announcement text for all channels (GitHub, Reddit, Discord announcement, KVR). Keep a master doc with all variants. Discord announcement posts to `#announcements` at L+0. Reddit posts fire L+0 + 2 hours.
- [ ] **L-1:** Final internal review. Play through 10 presets across 4 moods in the release build — not the dev build. Confirm coupling is audible on a hero 2-engine preset. Make sure you know how to do a hotfix if needed (see P4).

### Gumroad UTM tracking links

- [ ] Create UTM-tagged Gumroad links for: Instagram, YouTube, Reddit, Discord, the Field Guide article. These are the only way to know which channel drives downloads on day one. Format: `?utm_source=instagram&utm_medium=social&utm_campaign=v1launch`.

---

## PHASE 3 — Launch Day Runbook (L+0)

**Target time: 09:00 local time (early enough for US east coast morning, European afternoon)**

### L+0 — 08:00 (1 hour before)

- [ ] Make GitHub repo public. Verify it loads correctly: README renders, license visible, Releases page shows `v1.0.0` with `.component` download.
- [ ] Publish GitHub Release (`v1.0.0`). Release notes go live.
- [ ] Enable GitHub Discussions if not already enabled.
- [ ] Publish Gumroad product pages (TIDE TABLES and MACHINE GUN REEF). Verify both resolve correctly.
- [ ] Publish XO-OX.org plugin download page.
- [ ] Post Discord `#announcements` message.

### L+0 — 09:00 (launch moment)

- [ ] Social posts go live (scheduled or manual): Instagram, Twitter/X, Threads, YouTube (if video ready).
- [ ] Post Reddit r/synthesizers thread. Framing: "I spent two years building a free open-source synth — it ships today." Lead with what it sounds like, not the feature list. Include the 60s trailer or a coupling demo gif if possible.
- [ ] Post r/musicproduction with a different framing: "Built a free plugin + free drum pack for MPC producers." Lead with TIDE TABLES.
- [ ] Submit KVR Audio product listing.
- [ ] Send press kit to media contacts who expressed interest. Do not cold-email everyone on day one — save cold outreach for L+7 when you have early reception to reference.

### L+0 — 09:00 to 18:00 (monitor window)

- [ ] Monitor GitHub Issues every 2 hours. Flag any install failures immediately.
- [ ] Monitor Discord `#bug-reports`. Respond to every post within 4 hours on launch day.
- [ ] Monitor Reddit threads. Respond to every top-level comment within 2 hours. Neutral tone — do not be defensive about criticism.
- [ ] Monitor Gumroad dashboard. Note download counts for TIDE TABLES and purchase counts for MACHINE GUN REEF.
- [ ] Log any recurring issues. If 3 or more users report the same bug, it is a P1 hotfix candidate.
- [ ] Share a "launch is live" update in your personal network (direct messages to collaborators, community contacts who supported the project). This is separate from public social.

### L+0 — End of day debrief

Write a brief internal note (can be a single paragraph) covering:
- Download count (TIDE TABLES), purchase count (MACHINE GUN REEF)
- GitHub stars at EOD
- Discord joins
- Top 3 pieces of feedback (positive)
- Top 3 bugs or complaints
- Any P1 hotfix needed? Y/N

---

## PHASE 4 — Post-Launch Week (L+1 → L+7)

### Monitoring protocol

- Check GitHub Issues daily. Triage: P0 (crashes/data loss), P1 (broken core feature), P2 (minor friction), P3 (nice-to-have).
- Check Discord daily. Reply to every question in `#bug-reports` and `#sound-design-tips`.
- Update `pack_registry.json` with TIDE TABLES and MACHINE GUN REEF entries (QA score, release date, download stats at L+7).

### Hotfix protocol

If a P0 bug is confirmed (crash, data loss, AU validation failure):
1. Fix in a branch named `hotfix/v1.0.1`.
2. Re-run auval: `auval -v aumu Xomn XoOx`. Must PASS before any release.
3. Build and package new `.component`.
4. Create GitHub Release `v1.0.1` with the fix described clearly in release notes.
5. Post in Discord `#announcements`: "v1.0.1 released — fixes [issue]. If you experienced [symptom], please re-download."
6. Update the download page on XO-OX.org.

If a P1 bug is confirmed (broken coupling for a specific engine pair, preset browser crash):
- Fix within 72 hours. Same hotfix process. Communicate proactively on Discord.

P2 and P3 bugs: collect and ship in a v1.1 patch at L+30.

### Content cadence L+1 to L+7

- [ ] **L+1:** Field Guide article live (ONSET velocity guide). Share link in Discord and on social.
- [ ] **L+2:** "Launch day recap" post. Honest numbers (stars, downloads, feedback highlights). This builds trust and creates a momentum narrative.
- [ ] **L+3:** Instagram Sound Post featuring an ONSET preset from TIDE TABLES.
- [ ] **L+5:** YouTube pack walkthrough goes live if not yet published.
- [ ] **L+7:** First "Opening Night" engine spotlight begins (see P5). Topic: OddfeliX/OddOscar — the original coupling.
- [ ] **L+7:** Outreach to YouTube reviewers. Send press kit + a personal message explaining what makes the coupling system distinctive. Prioritize reviewers who cover free plugins: Venus Theory, Andrew Huang's team, Bedroom Producers Blog. Do not mass-email — 3–5 personalized messages.

---

## PHASE 5 — Opening Nights (L+7 → L+42)

Weekly engine spotlights per the brand strategy. Each is a low-production "exhibition opening" — one social post, one Field Guide entry or short blog post, one community challenge.

| Week | Engine | Theme | Content |
|------|--------|-------|---------|
| L+7 | OddfeliX / OddOscar | The Original Coupling | Field Guide: "How X and O Started It All." Community challenge: make a preset using ONLY the coupling corridor (coupling must be the instrument). |
| L+14 | ODYSSEY | The Journey | Field Guide: "Familiar to Alien — the JOURNEY macro." Sound Post: one Atmosphere mood preset. Challenge: breed an Atmosphere preset with a Foundation preset, share the offspring DNA. |
| L+21 | OBLONG | Warm Curiosity | Field Guide: "Fuzzy Textures and Tactile Warmth." Challenge: design a preset where M1 (CHARACTER) tells a complete story 0→1. |
| L+28 | OVERDUB | The Dub Engine | Field Guide: "Send/Return, Tape Delay, Spring Reverb." Community: post a track that uses OVERDUB as a send FX. |
| L+35 | OBESE | Heavy Character | Challenge: OBESE + any other engine coupled — most surprising pair wins (voted in Discord). |
| L+42 | ONSET | Circuit Percussion | XVC demo. Challenge: build a groove using only ONSET XVC cross-voice coupling. |

After L+42, assess community health metrics and decide whether to continue Opening Nights or shift to a different content format.

---

## Plugin Launch Checklist — Complete Reference

This consolidates all plugin-specific go/no-go items in one scannable list.

### Build and distribution — BLOCKERS

- [ ] `README.md` at repo root (elevator pitch, install instructions, quick start)
- [ ] `LICENSE` (GPLv3)
- [ ] `CONTRIBUTING.md` (preset submission, code rules, how to build)
- [ ] `Community/PRESET_SUBMISSION.md`
- [ ] GitHub Release `v1.0.0` with `.component` attached
- [ ] AU bundle passes `auval -v aumu Xomn XoOx` on release build
- [ ] Repo is public and private credentials are confirmed absent

### User experience — IMPORTANT

- [ ] Hero preset auto-loads on first launch (coupling audible immediately)
- [ ] Stub engine handling: hidden or labeled (no unexplained silent slots)
- [ ] Getting Started guide published on XO-OX.org
- [ ] Plugin download page live on XO-OX.org with install instructions
- [ ] Minimum macOS version documented (README + download page)

### Community infrastructure — IMPORTANT

- [ ] Discord server live with all channels and roles configured
- [ ] Discord invite link in README, CONTRIBUTING.md, and XO-OX.org
- [ ] GitHub Discussions enabled with category structure

### Documentation — IMPORTANT

- [ ] Engine deep-dive pages published for at least 6 engines on XO-OX.org
- [ ] Coupling explainer published (what coupling is, how to activate it)
- [ ] Preset DNA system explained (what the 6 dimensions mean)

---

## XPN Pack Launch Checklist — Additions to Existing

The existing checklist in `xpn_launch_checklist_v1_rnd.md` covers 20 items. The following are additions or clarifications not yet captured there.

- [ ] **[BLOCKER]** UTM-tagged Gumroad links created for Instagram, YouTube, Reddit, Discord, Field Guide before any public announcement.
- [ ] **[BLOCKER]** Both packs tested with a download sourced from the live Gumroad link — not dev filesystem. The file path must be what a customer receives.
- [ ] **[IMPORTANT]** MACHINE GUN REEF cover art finalized. 800×800px PNG. ONSET Electric Blue `#0066FF` themed (same engine as TIDE TABLES — distinguish via composition/typography, not color swap).
- [ ] **[IMPORTANT]** `pack_registry.json` updated with both packs before launch. Fields: tier, price, engine, QA score, release date, Gumroad URL.
- [ ] **[IMPORTANT]** TIDE TABLES and MACHINE GUN REEF entries published on XO-OX.org packs page with DNA charts and audio embeds before L+0.
- [ ] **[NICE]** Drum Broker submission package drafted (MACHINE GUN REEF): description, cover art, audio preview. Submit L+7.

---

## Integrated Launch Timeline — Plugin + Packs

**Question: Same day or staggered?**

**Recommendation: Same day, L+0.** The plugin and packs are designed as a system — TIDE TABLES showcases ONSET, which is inside XOmnibus. Separating them by days weakens both launches. The packs provide immediate proof of concept ("here is what ONSET sounds like on MPC") while the plugin provides the synthesis source ("here is how ONSET works"). They tell a complete story together.

**Stagger packs only if:**
- MACHINE GUN REEF's QA score does not clear 80 by L-7. In that case, launch TIDE TABLES on L+0 and MACHINE GUN REEF on L+14. Do not delay the free pack to wait for the paid one.

---

## Press Kit — Complete Asset Checklist

### Logos (vector, all variants)

- [ ] XOmnibus wordmark — light background version (dark text, SVG)
- [ ] XOmnibus wordmark — dark background version (light text, SVG)
- [ ] XOmnibus coupling symbol logomark only (SVG, scalable to 16px favicon)
- [ ] XO_OX brand mark (SVG)
- [ ] All as PNG at 512px, 1024px, 2048px

### Screenshots (retina PNG, light mode)

- [ ] Hero coupling view: two engines active, coupling strip showing signal, preset browser visible (4K)
- [ ] Preset browser: mood tabs visible, DNA radar chart visible on a selected preset (4K)
- [ ] Macro bar: all 4 macros (CHARACTER, MOVEMENT, COUPLING, SPACE) labeled and active
- [ ] Engine panel detail: ONSET (Electric Blue) — shows the drum voice layout
- [ ] Engine panel detail: OPAL (Lavender) — shows the granular grain cloud
- [ ] Engine panel detail: ODYSSEY (Violet) — shows the JOURNEY arc

### Audio demos

- [ ] 3+ hardware MPC captures from TIDE TABLES (velocity demonstrated in at least 1)
- [ ] 1 coupling demo — OPAL receiving ONSET audio as grain source (AudioToBuffer — Blessing B013 is ideal here)
- [ ] 1 preset breeding demo audio clip (optional but distinctive)

### Written materials

- [ ] One-sheet PDF (product summary, 3 key screenshots, key stats, website + GitHub + Discord links)
- [ ] Fact sheet plain text (same content, no formatting — for journalists who strip PDF)
- [ ] Brand guidelines summary PDF (colors, fonts, logo rules, voice examples)
- [ ] Elevator pitch paragraph (from `xomnibus_brand_identity_and_launch.md` §4.3 — adapt for 34 engines)

---

## Success Metrics — How V1 Launch Succeeded

### Week 1 targets (L+0 to L+7)

| Metric | Target | Signal |
|--------|--------|--------|
| GitHub stars | ≥ 100 | Community curiosity |
| GitHub forks | ≥ 10 | Developer interest |
| TIDE TABLES downloads | ≥ 200 | MPC producer reach |
| MACHINE GUN REEF purchases | ≥ 25 | Willingness to pay for XPN packs |
| Discord joins | ≥ 50 | Community forming |
| No P0 bugs reported | 0 crashes | Build quality |
| auval stays PASS on user machines | 0 AU failures | Distribution integrity |

### 30-day targets (L+0 to L+30)

| Metric | Target | Signal |
|--------|--------|--------|
| GitHub stars | ≥ 300 | Sustained interest |
| TIDE TABLES downloads | ≥ 500 | Word-of-mouth working |
| MACHINE GUN REEF purchases | ≥ 75 | Pack revenue baseline |
| Discord members | ≥ 150 | Active community |
| Community presets submitted | ≥ 5 | Contribution culture started |
| YouTube mentions / reviews | ≥ 2 | External reach |
| KVR thread replies | ≥ 25 | Synth community engagement |
| Email list size | ≥ 100 | Direct audience owned |

### 90-day targets (L+0 to L+90)

| Metric | Target | Signal |
|--------|--------|--------|
| GitHub stars | ≥ 500 | Platform with momentum |
| TIDE TABLES downloads | ≥ 1,000 | Organic growth |
| MACHINE GUN REEF purchases | ≥ 200 | Pack revenue covers tools investment |
| Discord members | ≥ 300 | Healthy community |
| Community presets submitted | ≥ 25 | Contribution pipeline working |
| YouTube reviews | ≥ 5 | Discovery flywheel |
| V1.1 shipped | ✅ | Iteration cadence established |
| Patreon revenue | Any amount > 0 | Patronage model viable |

### What failure looks like (and what to do)

| Signal | What It Means | Response |
|--------|---------------|----------|
| < 50 stars in week 1 | Announcement did not reach the right audience | Repost to subreddits not yet covered; reach out to 2 more YouTubers |
| 0 MACHINE GUN REEF purchases in week 1 | Price too high, or free pack didn't convert | Check Gumroad analytics for abandonment; consider $10 introductory price for L+30 |
| Multiple install failures reported | Build artifact issue | Re-package, re-release `v1.0.1`, announce fix proactively |
| Discord stays empty after L+7 | Community not forming | Post daily prompts for 2 weeks; feature one community member's work per day |
| P0 crash reported by 3+ users | Stable bug in release build | Hotfix within 24 hours; communicate transparently about what happened |

---

## Risk Register

| # | Risk | Likelihood | Impact | Mitigation |
|---|------|-----------|--------|------------|
| R1 | Patreon URL still placeholder at launch | Medium (existing blocker) | Embarrassing, breaks trust | BLOCKER #1. Assign today. |
| R2 | Site down on launch day | Low | Kills momentum | Gumroad is primary distribution. Site outage does not block purchases. |
| R3 | Pack format breaks on a specific MPC firmware | Medium | Poor first experience for MPC owners | Test firmware 2.14 and 2.16. Document minimum supported version prominently. |
| R4 | Audio demos recorded from DAW not hardware | Medium | Undermines product integrity | Checklist gates all demo uploads against hardware-only requirement. |
| R5 | Concept engine DSP not complete by L | High | V1 is 30/34 engines | Decide by L-21: delay or ship 30+V1.1 roadmap. Do not leave silent stubs unexplained. |
| R6 | GitHub repo goes public with credentials | Low | Security and trust failure | Full audit before L-7. Check git history. |
| R7 | AU bundle corrupted in GitHub Release asset | Low | Every download fails | Test download and install from GitHub Release before L+0. |
| R8 | No P0 bugs but silent failure in coupling preset | Medium | Core feature appears broken | Include an ONSET+OPAL coupling preset in the hero preset list; verify it works in the release build. |
| R9 | MACHINE GUN REEF QA score misses 80 | Medium | Cannot ship paid pack | Prepare TIDE TABLES-only L+0 plan; MG REEF at L+14 with dedicated announcement. |
| R10 | Community does not form on Discord | Medium | Launch loses sustainability | Schedule daily prompts for L+1 to L+14 in advance. Feature one member's work every 2 days. |

---

## Appendix A — Checklist Summary (Print Version)

### BLOCKERS — Launch cannot happen without these

Plugin:
- [ ] README.md (elevator pitch + install + quick start)
- [ ] LICENSE (GPLv3)
- [ ] CONTRIBUTING.md
- [ ] Community/PRESET_SUBMISSION.md
- [ ] GitHub Release v1.0.0 with .component attached
- [ ] auval PASS on release build
- [ ] Repo public, no credentials in history
- [ ] XO-OX.org plugin download page live
- [ ] Patreon URL corrected everywhere

Packs:
- [ ] TIDE TABLES QA ≥ 70 on release candidate
- [ ] TIDE TABLES 3+ hardware audio demos
- [ ] TIDE TABLES cover art (800×800 PNG)
- [ ] MACHINE GUN REEF QA ≥ 80 on release candidate (or L+14 fallback plan)
- [ ] MPCE_SETUP.md in both packs
- [ ] Hardware clean-load test (from Gumroad link)
- [ ] Gumroad test purchase with real card confirmed

Accounts:
- [ ] Patreon real URL live
- [ ] Gumroad account + payout configured
- [ ] UTM links created for all channels

### IMPORTANT — Should be done by L, can follow within 48h

- [ ] Discord server live with channels + roles
- [ ] Getting Started guide on XO-OX.org
- [ ] Engine deep-dives for at least 6 engines on XO-OX.org
- [ ] Coupling explainer published
- [ ] Audio demo embeds on site
- [ ] Pack pages live on XO-OX.org (TIDE TABLES + MACHINE GUN REEF)
- [ ] Social teaser calendar executed through L-1
- [ ] Field Guide launch article ready to publish at L+0
- [ ] Reddit posts staged and ready
- [ ] Stub engines handled in UI (hidden or labeled)
- [ ] Minimum firmware version documented

### NICE — Beneficial, not blocking

- [ ] Beta program 3–5 testers (L-14 to L-7)
- [ ] YouTube "First 5 Minutes" walkthrough
- [ ] YouTube pack walkthrough (TIDE TABLES)
- [ ] GitHub Discussions enabled
- [ ] GitHub repository topics added
- [ ] 60s trailer video
- [ ] KVR Audio listing
- [ ] Drum Broker submission draft (MACHINE GUN REEF, submit L+7)
- [ ] Patreon launch post + first exclusive content
- [ ] pack_registry.json updated with both packs before L+0
- [ ] Press kit published as downloadable zip on XO-OX.org

---

## Appendix B — Responsibility Matrix

All responsibility is single-person (XO_OX Designs). The matrix exists to make priority conflicts explicit.

| Domain | L-21 priority | L-7 priority | L+0 priority |
|--------|--------------|-------------|-------------|
| DSP / Build | Concept engine builds | Lock RC, auval re-verify | Hotfix only |
| Open source repo | README, LICENSE, CONTRIBUTING | Final repo audit | Monitor issues |
| Packs | QA runs, hardware test | Final QA, Gumroad staging | Monitor downloads |
| Site / Accounts | Download page, Patreon fix | Final link check | None (stable) |
| Content | Teaser calendar, article draft | Schedule posts | Monitor Reddit/Discord |
| Community | Discord setup | Discord ready check | Respond to every message |

---

*This document supersedes any partial launch planning in the existing specs directory. Update in place as items clear — do not archive until L+90 post-mortem is complete.*
