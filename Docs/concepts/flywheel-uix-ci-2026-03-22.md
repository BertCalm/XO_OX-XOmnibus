# Flywheel / Skills CI Audit — UI/UX Skill Cluster
## Date: 2026-03-22

**Skills Audited:**
1. `/uix-design-studio` (`~/.claude/skills/uix-design-studio/skill.md`) — updated with JUCE-y Lucy
2. `pixel-rabbit` (`~/.claude/skills/uix-design-studio/pixel-rabbit.md`) — newly created
3. `/atelier` (`~/.claude/skills/atelier/SKILL.md`)
4. `/fab-five` (`~/.claude/skills/fab-five/SKILL.md`)
5. `/ios-optimizer` (`~/.claude/skills/ios-optimizer/SKILL.md`)

**Auditor:** Flywheel CI — static read + cross-skill analysis
**Scope:** Quality, consistency, cross-wiring, completeness, and actionability

---

## Preliminary Finding: File Naming Inconsistency

The UIX Design Studio uses `skill.md` (lowercase) while `/atelier`, `/fab-five`, and `/ios-optimizer` all use `SKILL.md` (uppercase). The `pixel-rabbit.md` is a reference file living inside the UIX Studio directory rather than in its own skill directory. This is not a bug but it creates an irregular pattern — Pixel is described as a skill but has no invocation descriptor, only a `type: reference` field. **Recommend standardizing file names across all skills to `SKILL.md` for the primary entry point.**

---

## Skill 1: UIX Design Studio (`/uix-design-studio`)

### Score: 8.5 / 10

### Strengths

**Description trigger coverage is exceptional.** The frontmatter description lists 35+ trigger phrases covering the full range of invocation scenarios: by architect name (`ulf`, `issea`, `xavier`, `lucy`), by task type (`ui review`, `ux audit`, `juce ui`, `lookandfeel`), by feeling (`make it feel premium`, `tactile`, `feel like an instrument`), and by proactive trigger condition. This is the most complete trigger description in the cluster.

**Lucy integration is structurally correct.** JUCE-y Lucy is embedded at every level — she appears in:
- The architect roster with a full character description
- The Studio Session Protocol (Individual Assessments, Synthesis)
- The Priority Actions tagging system (`[L]`, `[UIXL]`)
- The Review Checklist (dedicated "Framework Fitness" section)
- The Platform-Specific Notes (she "owns" the JUCE section)
- The Synthesis section, where her role in feasibility-checking other architects' proposals is explicitly defined

The synthesis note — "A design that survives all four lenses is a design that ships" — cleanly codifies why Lucy is not optional.

**JUCE doctrine depth is production-ready.** Lucy's Platform-Specific Notes section reads like a reference document: `AsyncUpdater` vs `Timer` vs `ComponentAnimator` guidance, `LookAndFeel` as primary styling mechanism, format-specific constraints (AUv3, VST3, AAX), cross-platform rendering differences. This is genuinely useful standing doctrine that will prevent bugs.

**The ethos statement is coherent.** Three philosophies (Nordic, Japanese, Apple/NY) map cleanly to three architects, and the fourth architect (Lucy) extends the ethos into the JUCE implementation layer without tonal dissonance.

### Gaps

**Lucy is not mentioned in the Signature Design Principles section.** The five principles (Tactile Digitalism, Progressive Disclosure, Kinetic Empathy, Ecosystem Symbiosis) read as purely aesthetic — Lucy's implementation lens is absent. Kinetic Empathy especially needs a Lucy annotation: "animations must be timer- or ComponentAnimator-driven; animation never delays input."

**No cross-skill references in the skill body.** The description mentions that the Studio should be invoked proactively, but the skill itself does not tell the Studio when to invoke other skills. Missing: "invoke `/fab-five` for pre-release polish pass", "invoke `/atelier` when the design concern crosses into the web presence", "invoke `/ios-optimizer` when AUv3 constraints conflict with a design proposal". Compare to the Atelier, which has an explicit "When to Invoke Other Skills" table.

**The web platform section is thin.** The iOS section is well-developed (matching the Platform-Specific Notes standard), but the Web (`XO-OX.org`) section is only four bullet points and covers basics that any web developer would know. Given that the Atelier's Forge builds web UI to the same design standards, the web section should include specifics about the XO_OX design system tokens (XO Gold `#E9C46A`, Gallery Model shell `#F8F6F3`, Space Grotesk/Inter/JetBrains Mono) and Tailwind usage patterns.

**No mention of the Figma compendium.** The compendium (`Docs/figma-asset-compendium.md`) contains node IDs and adoption decisions for the Game UX Kit and Lean Mantine Library. The UIX Studio should know to consult it before making design decisions that may already have a resolved reference.

**"[UIX]" tag in Priority Actions silently excludes Lucy.** The tag is defined as "Design consensus (Lucy defers on implementation approach)" — but this framing is ambiguous. If Lucy disagrees with an implementation approach, that disagreement should surface, not be silenced by a tag. Recommend renaming to `[UXI]` (the three aesthetic architects) and clarifying that `[UIX]` means all four have reviewed.

### Recommendations

1. Add a Lucy annotation to each Signature Design Principle, noting the implementation constraint or enabler associated with the principle (small addition, high value for JUCE developers who read the principles without reading the full JUCE section).
2. Add a "When to Invoke Other Skills" section at the end of the skill, matching the Atelier's format:
   - `/atelier` — when design decisions cross into the XO-OX.org web presence
   - `/fab-five` — for pre-release visual polish pass on completed UI
   - `/ios-optimizer` — when AUv3 format constraints force design trade-offs
   - `/board` — when brand-visible changes (new color, new typography) need governance sign-off
3. Expand the Web platform section to include the specific XO_OX design tokens and reference the Figma compendium.
4. Add a line to the Context Scan step: "If a Figma compendium entry exists for the relevant components, read `Docs/figma-asset-compendium.md` before proposing alternatives."
5. Rename the `[UIX]` tag to `[UXI]` (three aesthetic architects) so `[UIX]` can unambiguously mean all four.

### Cross-Skill Wiring

Should reference: `/atelier`, `/fab-five`, `/ios-optimizer`, `/board`
Should be referenced by: `/atelier` (already does), `/fab-five` (does not), `/ios-optimizer` (does not)

---

## Skill 2: Pixel the Asset Curator Rabbit (`pixel-rabbit`)

### Score: 7.0 / 10

### Strengths

**The wishlist format is excellent.** The five-field template (Need / Products / Requirements / Search Terms / Alternatives) is clear, actionable, and product-aware. Every item maps to specific XO_OX products, which prevents collecting assets that benefit nobody.

**Pixel's evaluation criteria are precisely calibrated to the project.** The checklist — dark mode first, 4px grid, Space Grotesk/Inter/JetBrains Mono compatibility, proper component states — reflects actual XO_OX design system decisions, not generic UX best practices. This specificity makes Pixel's judgments immediately applicable.

**The wishlist is seeded with genuinely project-relevant items.** The Audio/Music Production UI Kit (CRITICAL) is the single most obvious gap in the compendium right now. The iOS Music/Audio App Template (HIGH) aligns directly with the iOS AUv3 work in progress. The Aquatic/Ocean illustration set (HIGH) reflects the mythology. These are not generic design asset lists — they're generated from actual project needs.

**The ROI framing in Pixel's personality is the right mental model.** "Does this download save more time than it costs to integrate?" focuses the curation on utility, not acquisition.

**The catalogue workflow (ADOPT / REFERENCE / REJECT) is clean.** Three outcomes, no ambiguity.

### Gaps

**No invocation trigger.** Pixel has `type: reference` in the frontmatter, not a proper skill description with trigger phrases. This means Pixel cannot be invoked like a skill — the user must know to ask about Pixel by name. The description field exists but only describes the role, not how to invoke it. Pixel should either have a proper skill description with trigger phrases, or the UIX Studio skill should explicitly state "asset curation questions are handled by Pixel — ask about Pixel" somewhere in its body.

**Pixel has no awareness of the Figma compendium state.** The wishlist items are initialized but Pixel's protocol only instructs her to "check `Docs/figma-asset-compendium.md`" for gaps — there is no step to read what is already there and cross-reference it against the wishlist. As the compendium grows (it already has Game UX Kit and Lean Mantine Library with full node IDs), Pixel should know to read it at the start of every session, not just when cataloguing new downloads.

**The wishlist is missing assets relevant to the iOS AUv3 work.** MEMORY.md confirms the iOS app is in design phase with a critical path of 8 steps to GarageBand test. The `[HIGH]` iOS Music/Audio App Template item is correct but should be elevated to `[CRITICAL]` given that iOS is the active development front. Additionally, there is no wishlist item for SwiftUI component libraries, which would be needed for the actual iOS app build.

**No protocol for when the user hits the 30-download daily limit.** Pixel tracks the running tally ("12/30 today") but there is no protocol for prioritizing across CRITICAL items when multiple are unmet and downloads are scarce. A simple priority sort (CRITICAL audio kit before CRITICAL dark mode library, because audio kit fills a more specific gap) would make Pixel more useful under constraint.

**The Protocol for New Downloads does not include a step to update the wishlist for discovered gaps.** Step 7 says "update the wishlist (mark fulfilled items, add newly discovered gaps)" but this is listed after the ADOPT/REFERENCE/REJECT decision, which means newly discovered gaps during a download session may be missed if the user stops before step 7.

**No connection to the UIX Studio session.** When the UIX Studio runs a design review and identifies a gap ("we need a better knob component reference"), there is no protocol for Pixel to receive that gap and add it to the wishlist. The connection is one-way (UIX Studio describes Pixel as part of the team, but Pixel's protocol does not mention the Studio).

### Recommendations

1. Add proper frontmatter description with trigger phrases: `"pixel", "asset curator", "what to download", "download recommendations", "figma wishlist", "ui8 download"` — or add a paragraph to the UIX Studio skill that says "For design asset curation and Ui8 download decisions, consult Pixel."
2. Add a step 0 to the catalogue workflow: "Read `Docs/figma-asset-compendium.md` to understand the current baseline before evaluating the new download."
3. Elevate the iOS Music/Audio App Template to `[CRITICAL]` with a note: "iOS AUv3 is the active design phase as of 2026-03-22."
4. Add a `[CRITICAL]` SwiftUI component library item to the wishlist.
5. Add a step at the top of Generate Download Recommendations: "Read MEMORY.md Next Session Queue — PRIORITY 1 and 2 items should drive the download list, not the general wishlist order."
6. Add a "Gap Discovery" channel: when the UIX Studio or Atelier identifies a design asset gap during a session, they should note it as `[UIX GAP]` or `[ATELIER GAP]` and Pixel should check for these annotations when generating the next recommendation list.

### Cross-Skill Wiring

Should reference: `/uix-design-studio` (receives gaps from), `/atelier` (receives web-specific gaps from), `Docs/figma-asset-compendium.md` (always read first)
Should be referenced by: `/uix-design-studio` (currently describes Pixel as right-hand of HT Ammell in Atelier, but not in UIX Studio itself)

---

## Skill 3: The Atelier (`/atelier`)

### Score: 7.5 / 10

### Strengths

**Cross-functional integration table is the best in the cluster.** The table listing what the Atelier takes from and gives back to each skill is a model that other skills should copy. It makes the Atelier's role as an integrator explicit and concrete.

**Mode architecture is clear and well-differentiated.** Five modes (Feature Request, Self-Generated Roadmap, Blog Post, Product Update Feed, E-Book Phase) cover the full range of site work without overlap.

**The self-generation protocol is excellent.** The instruction to include "at least one 'surprise'" prevents the Atelier from becoming a mechanical checklist executor. This is a hallmark of a well-designed creative skill.

**The Before You Begin — Gather State section is thorough.** Reading MEMORY.md, `xo-ox-domain.md`, and `aquatic-mythology.md` before starting work ensures the Atelier is never operating with stale context. This is the correct pattern.

**The Output Standards section is properly enforced.** Six checks (brand alignment, performance, accessibility, mobile-first, content quality, technical correctness) create a real quality gate.

### Gaps

**No mention of the UIX Design Studio.** The Atelier has HT + Pixel handling design decisions, but there is no protocol for when Pixel (asset curation) or the UIX Studio's four architects should be consulted. For a major redesign — say, the aquarium.html visual overhaul — the Atelier's Pixel handles layout but has no mechanism to escalate to the UIX Studio for a proper four-architect review. This is a meaningful gap: the Atelier is great at building but the UIX Studio is great at reviewing.

**The site architecture reference is likely stale.** The Current site structure lists four files (`index.html`, `packs.html`, `guide.html`, individual guide posts) but from MEMORY.md, the site has evolved significantly (VQ 002 aquarium phase, 7 pages confirmed). The Atelier should not maintain a site structure list inline — it should read the actual files each session per the Gather State step. The inline list creates a false sense of stability.

**The design system section contradicts CLAUDE.md.** CLAUDE.md and the XOceanus design spec are consistent: "Light mode is the primary presentation. Dark mode is a toggle." But the Atelier's design system says "Light mode primary, dark mode toggle" — which matches. However, Pixel's wishlist says "XO_OX is dark-theme-first" for asset evaluation criteria. This is a contradiction across the cluster. One of these is wrong. CLAUDE.md is the authority; the site is light-mode-first, and Pixel's wishlist criterion should be updated to reflect this.

**The Atelier's Pixel is a different character from the UIX skill's Pixel Rabbit.** The Atelier describes Pixel as "HT's right hand on interface decisions" for the web. `pixel-rabbit.md` describes Pixel as "the design asset curator for the UIX Design Studio and the Atelier." These two Pixel descriptions are compatible but subtly different in role. The Atelier's Pixel is a web UX designer; the pixel-rabbit is an asset curator. This needs clarification — either Pixel is two-hats in one (web designer in the Atelier context, asset curator in the UIX/acquisition context), or the names should be differentiated.

**No reference to the Figma asset compendium.** When Forge is building new interactive features and needs component references, the Figma compendium (`Docs/figma-asset-compendium.md`) should be the first stop. The Atelier's workflow has no step that mentions it.

**The `When to Invoke Other Skills` section is good but missing UIX Studio.** The section lists `/fab-five` for visual elevation but not `/uix-design-studio` for design review. These are different: the Fab Five elevates what already exists, the UIX Studio reviews what is planned or in-progress. Both are needed at different stages.

### Recommendations

1. Add `/uix-design-studio` to the `When to Invoke Other Skills` section: "invoke `/uix-design-studio` when designing new page layouts, interaction patterns, or major visual changes — before Forge builds anything."
2. Add a note to the Cross-Functional Integration table row for the Atelier itself: "The Atelier feeds design decisions to the UIX Studio for review; the UIX Studio feeds feasibility constraints back to the Atelier."
3. Remove or mark the Site Architecture Reference section as "verify each session" — do not maintain a stale inline list.
4. Resolve the dark/light mode contradiction: update Pixel's wishlist in `pixel-rabbit.md` line 27 — change "XO_OX is dark-theme-first" to "XO_OX is light-mode-first (Gallery Model), dark mode toggle exists — prefer assets with both modes."
5. Add a reference to `Docs/figma-asset-compendium.md` in the "Before You Begin — Gather State" section.
6. Clarify Pixel's dual-hat role: add a parenthetical to the Pixel team member description — "(in asset acquisition contexts, Pixel also operates as the Asset Curator Rabbit — see `uix-design-studio/pixel-rabbit.md`)."

### Cross-Skill Wiring

Should reference: `/uix-design-studio` (missing — critical gap), `/fab-five` (already references), `Docs/figma-asset-compendium.md` (missing)
Should be referenced by: `/uix-design-studio` (missing), `/fab-five` (not present but less critical)

---

## Skill 4: The Fab Five (`/fab-five`)

### Score: 7.0 / 10

### Strengths

**The Queer Eye framing is memorable and immediately actionable.** Each specialist's question ("Does this look like it was designed, or like it just happened?") is crisp enough to apply instantly to any work. This is effective skill design.

**The three-tool relationship (Sweep / Board / Fab Five) is well-articulated.** The analogy in the "How It Relates" section (Roomba / Government / Stylist) is genuinely clarifying and prevents role confusion.

**Intensity levels are a valuable UX feature.** `touch-up` / `makeover` / `gala` let the user calibrate the effort level, which is important for a skill that can easily expand to fill all available time.

**The Sound Designer's sonic DNA coverage concept is production-relevant.** Asking whether presets cover all six dimensions of the XO_OX Sonic DNA directly aligns with the fleet's quality standards.

**The Storyteller's XO_OX mythology specificity is good.** Mentioning feliX the neon tetra and the water column atlas by name keeps the Storyteller grounded in the actual brand rather than generic brand-voice advice.

### Gaps

**No mention of the UIX Design Studio.** The Stylist (F1) evaluates "UI aesthetics" but has no protocol for escalating to the UIX Studio when UI work needs a deeper four-architect review. The Fab Five is a style pass; the UIX Studio is a design review. They should interoperate. When F1 identifies a major visual issue, the correct escalation is "/uix-design-studio for a full design review" — but this is not stated.

**The Fab Five overlaps with the UIX Studio in UI aesthetics in a potentially confusing way.** The Stylist evaluates color palettes, typography, spacing, and visual hierarchy — all of which are primary UIX Studio territory. The distinction should be clearer: the Fab Five does a holistic style pass across *all surfaces* (code, docs, presets, UI) while the UIX Studio does a *deep* UI/UX review focused specifically on interaction design. Without this distinction, a user asking "make my UI better" might invoke either skill and get different outputs.

**No cross-skill invocation guidance.** The Fab Five operates as a self-contained unit but does not suggest when to invoke other skills. Examples: the Storyteller (F5) should suggest `/synth-seance` when mythology deepening requires historical grounding; the Sound Designer (F4) should suggest `/preset-forge` or `/guru-bin` when the preset library needs new entries; the Architect (F3) should suggest `/sweep` when dependency cleanup is needed.

**The Sound Designer's scope doesn't distinguish JUCE plugin UI from web UI.** F1 (Stylist) evaluates websites, preset names, docs, and UI under one umbrella. For a JUCE plugin UI, the implementation constraints that Lucy enforces are load-bearing; the Fab Five has no mechanism to invoke Lucy for implementation feasibility. A beautiful UI suggestion from F1 could be impractical in JUCE without Lucy's review.

**No reference to the Atelier.** When F5 (Storyteller) or F1 (Stylist) finds that the website doesn't reflect the product identity, the natural escalation is `/atelier` — but this is not stated.

**The Walk-Through step (Phase 1) reads "Browse the codebase"** but gives no specific files to read for the XO_OX context. For new users of the skill, this is vague. A brief note pointing to `Docs/xoceanus_master_specification.md` and `Docs/figma-asset-compendium.md` as starting points would help.

### Recommendations

1. Add a "When the Fab Five Calls in Specialists" section:
   - F1 (Style) → `/uix-design-studio` for deep UI/UX review
   - F3 (Architect) → `/sweep` for dependency cleanup
   - F4 (Sound) → `/guru-bin` for preset retreats and library expansion
   - F5 (Story) → `/synth-seance` for mythology and historical grounding
   - F5 (Story) → `/atelier` when the website needs narrative alignment
2. Add a one-paragraph clarification of Fab Five vs UIX Studio relationship in the "How It Relates" section: "The Fab Five does a broad, holistic style pass across all surfaces. The UIX Studio does a deep design review of UI specifically, including JUCE implementation feasibility. Use the Fab Five first, then call in the UIX Studio if F1's findings reveal deeper UI architecture concerns."
3. Add to Phase 1 Walk-Through: specific files to read for XO_OX context (CLAUDE.md, `Docs/figma-asset-compendium.md`, recent Seance scores, preset naming guidelines).
4. Add a note in F4 (Sound Designer): "For generating new presets from the Guru Bin's perspective, invoke `/guru-bin` — the Fab Five identifies *what* is missing, the Guru Bin creates *what* is needed."

### Cross-Skill Wiring

Should reference: `/uix-design-studio` (missing — important), `/atelier` (missing), `/sweep` (already references conceptually), `/synth-seance` (missing), `/guru-bin` (missing)
Should be referenced by: `/uix-design-studio` (already — Atelier references it), `/atelier` (already)

---

## Skill 5: iOS Optimizer (`/ios-optimizer`)

### Score: 6.5 / 10

### Strengths

**The AUv3 vs AU2 comparison table is the most useful single artifact in the cluster.** It immediately frames the design problem space. Any engineer moving from macOS to iOS reads this table and knows where the traps are.

**The iOS Audit Checklist is actionable and comprehensive.** Six categories (Memory, State Management, CPU, File Access, Audiobus/AUM, Background Audio, App Store) cover the full lifecycle of an iOS AUv3 release. Each item is a binary gate rather than vague guidance.

**The Common iOS Failure Patterns table is incident-driven.** The Symptom / Fix pattern for each failure mode is exactly what an engineer needs at 2am when something crashes in GarageBand.

**The CPU target table provides a measurable iOS standard.** Giving iOS-specific percentage targets (not just "less than macOS") with reasoning (ARM slower, battery management) is more useful than generic "optimize for mobile."

**NEON SIMD and thermal throttling guidance is iOS-specific and non-obvious.** These are genuinely platform-specific insights that do not appear in the macOS documentation.

### Gaps

**No mention of the UIX Design Studio for UI decisions.** The iOS Optimizer is entirely focused on DSP, memory, and compatibility — it has no design layer at all. But iOS AUv3 UI has real design constraints that differ from macOS (touch targets, safe areas, SwiftUI vs UIKit choice, host-provided container sizing). When the Optimizer evaluates an iOS port, it should flag UI issues and escalate to the UIX Studio's Xavier (who owns iOS/macOS HIG and native integration) rather than leaving UI entirely unaddressed.

**No reference to the `ios-design-spec.md`.** MEMORY.md confirms `ios-design-spec.md` exists with specific notes about SwiftUI vs UIKit, AUv3 build targets, and an 8-step critical path to GarageBand test. The iOS Optimizer should read this file in its audit protocol.

**The skill has no "Before You Begin" state-gathering step.** Unlike the Atelier or the UIX Studio, the iOS Optimizer dives directly into checklists without reading current project state. Given that the iOS app status changes (MEMORY.md: "No code written yet. Critical path: 8 steps to GarageBand test"), the Optimizer should start by reading `ios-design-spec.md` and MEMORY.md's iOS section.

**No mention of OperaSVF's known P0 issue.** MEMORY.md explicitly notes "P0: OperaSVF std::tan fix needed. No code written yet." The iOS Optimizer would surface this as a CPU issue on iOS (per-sample `std::tan()` is doubly expensive on ARM) but currently has no mechanism to check known pending issues from MEMORY.md or the Seance findings.

**The argument structure is limited.** The five arguments (`audit`, `memory`, `cpu`, `state`, `compat`, `appstore`, `optimize`) are good but there is no `design` argument for UI/UX review, and no `all` shorthand for a full pre-release sweep.

**No escalation protocol.** When the iOS Optimizer finds a CPU issue, it has no protocol for invoking the DSP Profiler (`/dsp-profiler`) for deeper analysis, or the SRO Optimizer (`/sro-optimizer`) for mitigation. These exist as skills but the iOS Optimizer does not reference them.

**App Store compliance checklist is thin.** Five items is a minimum. Missing: privacy manifest requirement (required since iOS 17), reason-for-API-usage declarations, notarization requirements for embedded binaries, and TestFlight distribution steps.

### Recommendations

1. Add a "Before You Begin" section: read MEMORY.md iOS section and `ios-design-spec.md` if it exists.
2. Add a `design` argument that invokes Xavier from the UIX Studio for iOS HIG review: touch targets, safe areas, SwiftUI/UIKit patterns, host container sizing.
3. Add cross-skill escalation protocols:
   - CPU issues → invoke `/dsp-profiler` for profiling, `/sro-optimizer` for mitigation
   - Memory issues → consult `/build-sentinel` for allocation analysis
   - UI issues → invoke `/uix-design-studio` (Xavier's lens specifically)
4. Expand App Store compliance checklist with: privacy manifest, reason-for-API-usage declarations, TestFlight steps, notarization for embedded binaries.
5. Add a "Known Fleet Issues on iOS" step that reads from MEMORY.md — specifically the OperaSVF P0 (`std::tan()` per-sample) and any other known issues flagged in recent sessions.
6. Add an `all` argument as an alias for full audit mode.

### Cross-Skill Wiring

Should reference: `/uix-design-studio` (missing — meaningful gap), `/dsp-profiler` (missing), `/sro-optimizer` (missing), `/build-sentinel` (missing), `ios-design-spec.md` (missing)
Should be referenced by: `/uix-design-studio` (partially — Xavier is listed as iOS/macOS authority but the iOS Optimizer is not mentioned as a dependency)

---

## Cross-Skill Wiring Summary

The following table maps wiring that should exist but currently does not:

| Skill A | Should invoke / reference | Skill B | Current Status |
|---------|--------------------------|---------|---------------|
| `/uix-design-studio` | "design decisions touching web" → | `/atelier` | Missing |
| `/uix-design-studio` | "pre-release polish pass" → | `/fab-five` | Missing |
| `/uix-design-studio` | "AUv3 format constraints" → | `/ios-optimizer` | Missing |
| `/uix-design-studio` | "brand-visible changes" → | `/board` | Missing |
| `/uix-design-studio` | "design asset questions" → | `pixel-rabbit` | Missing |
| `/atelier` | "new page layouts, interaction review" → | `/uix-design-studio` | **Critical gap** |
| `/atelier` | "component reference" → | `Docs/figma-asset-compendium.md` | Missing |
| `/fab-five` | "deep UI/UX review beyond style pass" → | `/uix-design-studio` | Missing |
| `/fab-five` | "website narrative alignment" → | `/atelier` | Missing |
| `/fab-five` | "preset library expansion" → | `/guru-bin` | Missing |
| `/fab-five` | "dependency cleanup" → | `/sweep` | Implied, not stated |
| `/ios-optimizer` | "iOS UI design review" → | `/uix-design-studio` (Xavier) | **Critical gap** |
| `/ios-optimizer` | "CPU issues" → | `/dsp-profiler`, `/sro-optimizer` | Missing |
| `pixel-rabbit` | "reads current state" → | `Docs/figma-asset-compendium.md` | Missing (step 0) |
| `pixel-rabbit` | "receives design gaps" → | `/uix-design-studio` | Missing |

---

## Specific Issue: Light vs. Dark Mode Contradiction

**File:** `~/.claude/skills/uix-design-studio/pixel-rabbit.md`, line 27
**Current:** "XO_OX is dark-theme-first"
**Correct:** CLAUDE.md and the XOceanus design spec define the Gallery Model as light-mode default with dark mode toggle. The site is light-mode-first. Pixel's evaluation criterion for downloaded assets should read: "Does it include a light mode variant? (XO_OX is light-mode-first — Gallery Model warm white shell. Dark mode toggle exists but is secondary.)"

This contradiction would cause Pixel to reject or deprioritize assets that the UIX Studio, Atelier, and Board would actually want (light-mode components for the JUCE plugin and web presence).

---

## Cluster Health Summary

| Skill | Score | Most Critical Gap | Priority Fix |
|-------|-------|-------------------|-------------|
| `/uix-design-studio` | 8.5/10 | No cross-skill invocation guidance | Add "When to Invoke Other Skills" section |
| `pixel-rabbit` | 7.0/10 | No proper invocation trigger; dark/light mode contradiction | Fix dark/light contradiction immediately (data integrity); add trigger phrases |
| `/atelier` | 7.5/10 | No UIX Design Studio reference | Add `/uix-design-studio` to "When to Invoke Other Skills" |
| `/fab-five` | 7.0/10 | No UIX Studio escalation for F1 findings | Add "When the Fab Five Calls in Specialists" section |
| `/ios-optimizer` | 6.5/10 | No UIX Studio reference for design; no `ios-design-spec.md` read | Add design escalation + before-you-begin state gathering |

**Cluster average: 7.3 / 10**

The skill cluster is functional and shows genuine craft — the UIX Studio's Lucy integration is a model for how to add a new architect cleanly, and the Atelier's cross-functional integration table is best-in-class. The primary systemic gap is **the absence of cross-skill invocation wiring from the spokes toward the UIX Studio hub**. The UIX Studio is the design authority, but no other skill in the cluster explicitly routes to it. Fixing this wiring would raise the cluster average by approximately 0.5-0.75 points without any content rewrites.

**Recommended execution order:**
1. Fix the dark/light mode contradiction in `pixel-rabbit.md` (data integrity, 2 minutes)
2. Add "When to Invoke Other Skills" to `/uix-design-studio` (highest-value change for the hub skill)
3. Add `/uix-design-studio` to `/atelier`'s "When to Invoke" section (closes the most critical gap)
4. Add UIX Studio escalation to `/fab-five` F1 section
5. Add "Before You Begin" and `/uix-design-studio` reference to `/ios-optimizer`
6. Add invocation trigger to `pixel-rabbit.md`
7. Standardize file naming: all primary skill files should use `SKILL.md` (uppercase)

---

*Flywheel CI complete — 2026-03-22*
