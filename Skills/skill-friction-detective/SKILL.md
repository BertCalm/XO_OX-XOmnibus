# Skill: /skill-friction-detective

**Invoke with:** `/skill-friction-detective [scan | audit | repair <skill-name> | link]`
**Status:** LIVE
**Purpose:** Meta-skill that actively detects friction, frustration, silent failures, and inefficiencies across the XOlokun skill library — then repairs, enhances, links, and optimizes the affected skills.

---

## Philosophy

Every skill is a promise. A skill that leaves the user confused, looping, or abandoned has broken that promise silently. This meta-skill exists to **hear the silence** — to recognize when a process is leaking, when a user is frustrated before they say so, and when two skills are orbiting the same problem without acknowledging each other.

The detective doesn't wait for a bug report. It reads the room.

---

## When to Use This Skill

Use this skill when:
- A session produced unexpected back-and-forth, repeated questions, or clarification loops on a skill-driven task
- A skill was invoked but the result required significant correction or re-invocation
- A user expressed frustration, confusion, or said "that's not what I meant" mid-skill
- A task was abandoned midway through a skill process
- Output quality from a skill seemed inconsistent across two similar invocations
- You notice two skills duplicating guidance without cross-referencing each other
- A skill's output was accepted but clearly incomplete (silent failure)
- Proactively after any session where multiple skills were used — look for what didn't flow

**Modes:**
- `/skill-friction-detective scan` — Analyze the current session's conversation for friction signals
- `/skill-friction-detective audit` — Full library-wide structural audit of all skills
- `/skill-friction-detective repair <skill-name>` — Deep-dive fix of a specific skill
- `/skill-friction-detective link` — Find and document cross-skill linking opportunities

---

## Friction Signal Taxonomy

Before diagnosing, learn to recognize friction types. Each has a different root cause and repair.

### Type F1 — Re-invocation Loop
**Signal:** User invokes the same skill twice for the same task, or explicitly restarts a phase.
**Meaning:** Phase output was ambiguous, incomplete, or produced a wrong fork.
**Root cause:** Missing precondition check, ambiguous branching, or a phase that assumes context it doesn't verify.

### Type F2 — Redirect Cascade
**Signal:** User says "wait, do X first" or "actually, use Y instead" — redirecting mid-skill.
**Meaning:** The skill started too fast without gathering necessary context, or the skill selection guide led to the wrong skill.
**Root cause:** Missing intake phase; skill description too broad; selection guide has overlapping triggers.

### Type F3 — Clarification Spiral
**Signal:** Multiple user messages asking "what does X mean?" or "where do I find Y?" mid-skill.
**Meaning:** The skill uses undefined terms, references undocumented files, or assumes knowledge the user doesn't have.
**Root cause:** Missing glossary, broken internal reference, unexplained jargon, or an assumed prerequisite skill that wasn't recommended.

### Type F4 — Silent Failure
**Signal:** Skill "completes" but the output is accepted without validation — and later proves wrong or incomplete.
**Meaning:** The quality gate didn't catch the error, or the quality gate was bypassed/incomplete.
**Root cause:** Weak quality gate checklist, missing validation step, or a phase that produces plausible-looking but incorrect output (e.g., wrong engine prefix, fake DNA values).

### Type F5 — Abandoned Thread
**Signal:** User stops mid-skill and moves to a different task without completing or saying why.
**Meaning:** The skill became too complex, too time-consuming, or produced a blocker the user couldn't resolve.
**Root cause:** Phase has too many steps, a required tool/file is missing, or a prerequisite was not surfaced early enough.

### Type F6 — Orphan Output
**Signal:** Skill produced output (e.g., a preset, a health report) that was never used in a follow-up action.
**Meaning:** The skill doesn't guide the user to the next logical step — it ends without a handoff.
**Root cause:** Missing "What to do next" section; skill is not linked into the broader workflow.

### Type F7 — Skill Mismatch
**Signal:** User invoked Skill A when Skill B was the right tool — realized only after partial execution.
**Meaning:** The skill selection guide has overlapping descriptions, or a skill's "When to Use" is too broad.
**Root cause:** Selection guide ambiguity; two skills serving adjacent purposes without clear differentiation.

### Type F8 — Stale Assumption
**Signal:** Skill references a file, count, or state that no longer exists (e.g., "22 engines" when the fleet is now 43).
**Meaning:** The skill was written for an earlier state of the project and hasn't been updated.
**Root cause:** Missing version-awareness; skill was not updated during fleet expansions, doctrine updates, or file renames.

---

## Phase 1: Scan — Read the Session for Friction (`/skill-friction-detective scan`)

Work through the session transcript or conversation context systematically.

### Step 1A — Map Skill Invocations

List every skill that was invoked (explicitly via `/skill-name` or implicitly by following a skill's process). For each:

```
Skill: [name]
Entry point: [what prompted it]
Exit: [completed | abandoned | redirected | re-invoked]
Output used: [yes | no | partially]
```

### Step 1B — Tag Friction Events

For each friction event in the session, tag its type:

```
[Timestamp / Message #]: [F1–F8] — [one-line description]
Example: [Msg 14]: F3 — User asked "where is coupling_audit.md?" mid coupling-preset-designer Phase 1
Example: [Msg 22]: F4 — DNA values accepted but movement=0.1 for an Entangled preset (violates movement≥0.40 rule)
Example: [Msg 31]: F5 — User abandoned xpn-export-specialist after Step 2; no explanation
```

### Step 1C — Score Session Health

After tagging:

```
=== SESSION FRICTION SCAN ===
Skills invoked: [N]
Skills completed cleanly: [N]
Friction events: [N]
  F1 (Re-invocation loops): [N]
  F2 (Redirect cascades):   [N]
  F3 (Clarification spirals): [N]
  F4 (Silent failures):     [N]
  F5 (Abandoned threads):   [N]
  F6 (Orphan outputs):      [N]
  F7 (Skill mismatches):    [N]
  F8 (Stale assumptions):   [N]

Session Friction Score: [N total events]
  0–2: Smooth session — minor tune-up opportunities only
  3–5: Friction present — 1–2 skills need targeted repair
  6–9: High friction — systemic issues; run full audit
  10+: Critical — library needs structural review

Top friction source: [skill name + friction type]
```

---

## Phase 2: Audit — Library-Wide Structural Check (`/skill-friction-detective audit`)

A proactive audit that doesn't require a session transcript. Read every skill in `Skills/` and check it against these structural criteria.

### Audit Checklist (run for each skill)

**Completeness**
- [ ] "When to Use" section is specific (3+ concrete situations, not vague)
- [ ] All file references exist at their stated paths
- [ ] All tool references (`python3 Tools/X.py`, etc.) reference real scripts
- [ ] No engine name counts that are now outdated (currently: 43 engines)
- [ ] No mood count errors (currently: 8 moods — Foundation, Atmosphere, Entangled, Prism, Flux, Aether, Family, Submerged)
- [ ] No references to legacy engine names without alias resolution note

**Quality Gates**
- [ ] Skill has a named Quality Gate section with a checklist (not just prose)
- [ ] Quality gate covers at least 5 checkpoints
- [ ] At least one checkpoint is a negative check (something that MUST NOT be present)
- [ ] Quality gate references a validation tool or cross-reference doc where one exists

**Handoff**
- [ ] Skill ends with a "What to do next" or "Related Skills" section
- [ ] Output produced by the skill has a clear destination (file path, doc, report)
- [ ] If skill produces a file, the file path template is explicit

**Linking**
- [ ] Skills that share subject matter (e.g., coupling) cross-reference each other
- [ ] Skill selection guide in `Skills/README.md` accurately describes this skill's trigger
- [ ] Trigger description doesn't overlap with another skill's trigger without differentiation

**Currency**
- [ ] No facts that have been superseded (engine counts, P0 bug status, doctrine resolution status)
- [ ] Doctrine resolution status is current: D001–D006 all RESOLVED fleet-wide
- [ ] Seance scores are not referenced as definitive (fleet is living data)

### Audit Report Format

```
=== SKILL LIBRARY AUDIT ===
Date: [date]
Skills audited: [N]

SKILL-BY-SKILL RESULTS
----------------------
[skill-name]:
  Completeness: [✅ PASS | ⚠️ GAPS | ❌ FAIL]
  Quality gate: [✅ PASS | ⚠️ WEAK | ❌ MISSING]
  Handoff:      [✅ PASS | ⚠️ PARTIAL | ❌ MISSING]
  Linking:      [✅ PASS | ⚠️ ORPHANED | ❌ MISSING]
  Currency:     [✅ PASS | ⚠️ STALE | ❌ OUTDATED]
  Priority:     [HIGH | MEDIUM | LOW]
  Notes: [specific issues]

LIBRARY-WIDE FINDINGS
---------------------
[Systemic patterns across multiple skills]

REPAIR QUEUE (priority order)
------------------------------
1. [skill-name] — [issue summary] — [friction type]
2. ...

LINKING OPPORTUNITIES
---------------------
[skill-A] ↔ [skill-B]: [shared subject / entry point overlap]
```

---

## Phase 3: Repair — Fix a Specific Skill (`/skill-friction-detective repair <skill-name>`)

### Step 3A — Load Existing Skill

Read `Skills/<skill-name>/SKILL.md` in full. Don't assume you know its content.

### Step 3B — Identify All Issues

Apply the audit checklist from Phase 2. Also check:
- Are all phases numbered consistently?
- Are all code examples syntactically valid?
- Are template blocks copy-paste ready (no `[FILL IN]` left exposed without instruction)?
- Does each template block have a comment explaining what to fill in?
- Are edge cases documented? (What happens if the engine has no coupling? If the mood folder is empty? If the tool doesn't exist?)

### Step 3C — Classify Each Issue

For each issue found:

| Issue | Friction Type | Repair Action | Effort |
|-------|--------------|---------------|--------|
| [describe] | [F1–F8] | [what to change] | [Low/Med/High] |

### Step 3D — Apply Repairs (in order)

1. **Stale facts first** — update counts, paths, doctrine status. These break trust immediately when wrong.
2. **Missing handoff** — add "What to do next" if absent. One of the highest-ROI repairs.
3. **Weak quality gate** — strengthen the checklist; add negative checks.
4. **Clarification gaps** — add a glossary box or inline definition for any term flagged in F3 events.
5. **Selection guide** — update `Skills/README.md` if the trigger description needs refinement.
6. **Cross-references** — add bidirectional links between this skill and its neighbors.

### Step 3E — Write the Repair Log

After editing the skill file:

```
=== REPAIR LOG: [skill-name] ===
Date: [date]
Issues found: [N]
Issues fixed: [N]

Changes made:
- [specific edit 1] — fixes [friction type]
- [specific edit 2] — fixes [friction type]
...

Quality gate: [strengthened | unchanged | added from scratch]
Handoff: [added | improved | unchanged]
Cross-references added: [list]
Stale data updated: [list]

Post-repair assessment: [expected friction reduction]
```

---

## Phase 4: Link — Find and Document Cross-Skill Opportunities (`/skill-friction-detective link`)

Skills are not islands. Friction increases when a user has to discover a related skill by accident.

### Step 4A — Build the Skill Graph

For each skill, identify:
1. **What problem does it solve?** (1-sentence)
2. **What problem does it assume was solved before it?** (prerequisite)
3. **What problem should be solved after it?** (natural successor)
4. **What problem does it share with another skill?** (sibling)

### Step 4B — Map the Known Workflow Chains

Current documented chains (verify and extend):

```
NEW ENGINE PATH:
  /new-xo-engine (Docs process)
    → /engine-health-check (post-build verification)
    → /synth-seance (full quality evaluation)
    → /mod-matrix-builder (if D002/D005/D006 fail)
    → /post-engine-completion-checklist

PRESET PATH:
  /preset-architect (any preset)
    → /dna-designer (DNA assignment after parameters)
    → /coupling-preset-designer (if Entangled mood)
    → /coupling-interaction-cookbook (if unsure which engine pair)

RELEASE PATH:
  /engine-health-check
    → /xpn-export-specialist (after health verified)
    → /sro-optimizer (if CPU budget is tight)

QUALITY PATH:
  /synth-seance
    → /engine-health-check (for specific D-violation triage)
    → /producers-guild (market/product angle)
    → /mod-matrix-builder (doctrine repair)
```

### Step 4C — Identify Missing Links

A missing link exists when:
- Skill A's natural output is Skill B's natural input, but A doesn't mention B
- Two skills serve the same surface area from different angles without acknowledging each other
- A skill ends without pointing to any next step (orphan)

Document each missing link:

```
MISSING LINK: [skill-A] → [skill-B]
Trigger: [what output from A should flow into B]
Repair: Add to A's "What to do next": "If [condition], use /skill-B"
```

### Step 4D — Add Cross-References

For each missing link found, edit both skill files to add the reference — bidirectional where the relationship is symmetric.

Format for cross-reference block (add to bottom of each SKILL.md):

```markdown
---

## What to Do Next

| Situation | Next Skill |
|-----------|-----------|
| [condition after this skill completes] | `/next-skill-name` |
| [alternative condition] | `/alternative-skill` |

## Related Skills

| Skill | Relationship |
|-------|-------------|
| `/skill-name` | [prerequisite | successor | sibling — description] |
```

---

## Phase 5: Proactive Detection Posture

Beyond reactive repair, maintain an active friction watch during any multi-skill session.

### Watch Rules (apply during any skill-heavy session)

1. **If you re-explain the same concept twice in one session** → that concept needs a glossary entry in the relevant skill.

2. **If a user asks "which skill should I use for X?"** → the skill selection guide needs a clearer trigger for X.

3. **If a skill phase produces output the user immediately edits** → that phase has a missing constraint or the template has a wrong default.

4. **If you reach a phase and realize a prerequisite wasn't checked** → that phase needs a precondition guard at the top.

5. **If a skill's quality gate would have caught an error that slipped through** → the quality gate is incomplete; add the missing check.

6. **If two skills both claim to handle "Entangled mood presets"** → they need to divide responsibility explicitly.

7. **If a skill references a file that no longer exists** → that's a P0 stale assumption; fix before the session ends.

8. **If the user reaches a skill's final phase and doesn't know what to do with the output** → the handoff section is missing; add it immediately.

---

## Quality Gate

Before closing a friction-detective session:

- [ ] All friction events from the scan are typed and assigned to a specific skill
- [ ] All F8 (stale assumption) events are fixed immediately — stale facts compound
- [ ] Every identified repair has a logged change (no verbal repairs without a file edit)
- [ ] `Skills/README.md` is updated if any skill's description or trigger was changed
- [ ] At least one linking opportunity was found and connected (even if only one direction)
- [ ] Repair log is written for every skill that was modified
- [ ] No repair introduced new jargon without definition
- [ ] Session friction score is recalculated post-repair (mentally — did we reduce it?)

---

## What to Do Next

| Situation | Next Skill |
|-----------|-----------|
| Skill needed a mod matrix fix | `/mod-matrix-builder` |
| Skill involved a new engine failing quality gate | `/engine-health-check` |
| Skill failure revealed a preset was wrong | `/preset-architect` |
| Fleet-wide pattern found — engine quality concern | `/synth-seance` |
| CPU-related friction in SRO skill | `/sro-optimizer` |

## Related Skills

| Skill | Relationship |
|-------|-------------|
| `/engine-health-check` | Sibling — health-check is the domain-specific QA tool; this skill audits the QA process itself |
| `/synth-seance` | Successor — if friction reveals a systemic engine quality issue, escalate to seance |
| `/preset-architect` | Sibling — if friction is in preset creation workflow |
| `/producers-guild` | Sibling — if friction is at the product/market communication layer |
| `/sro-optimizer` | Sibling — if friction is in the performance/efficiency workflow |
| `/post-engine-completion-checklist` | Sibling — both are meta-level quality processes; run this after checklist reveals gaps |

---

## Reference: All Skills in the Library

| Skill | File | Trigger Situation |
|-------|------|-------------------|
| `/coupling-preset-designer` | `Skills/coupling-preset-designer/SKILL.md` | Creating Entangled mood presets |
| `/coupling-interaction-cookbook` | `Skills/coupling-interaction-cookbook/SKILL.md` | Need best engine pairings quickly |
| `/mod-matrix-builder` | `Skills/mod-matrix-builder/SKILL.md` | D002/D005/D006 violations |
| `/preset-architect` | `Skills/preset-architect/SKILL.md` | Any new `.xometa` preset |
| `/engine-health-check` | `Skills/engine-health-check/SKILL.md` | Quick D001–D006 doctrine check |
| `/dna-designer` | `Skills/dna-designer/SKILL.md` | Assigning 6D Sonic DNA |
| `/xpn-export-specialist` | `Skills/xpn-export-specialist/SKILL.md` | XPN/MPC export pipeline |
| `/sro-optimizer` | `Skills/sro-optimizer/SKILL.md` | CPU optimization / constraint-driven DSP |
| `/synth-seance` | `~/.claude/skills/` | Full ghost council engine evaluation |
| `/post-engine-completion-checklist` | `~/.claude/skills/` | 5-point post-engine audit |
| `/producers-guild` | `~/.claude/skills/` | Market/product review |
| `/skill-friction-detective` | `Skills/skill-friction-detective/SKILL.md` | **This skill** — meta-level skill QA |
