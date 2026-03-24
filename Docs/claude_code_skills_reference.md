# Claude Code Skills Reference Library

> Curated for **XO_OX Designs / XOlokun** — C++ audio plugin development, DSP, CMake, cross-platform builds (AU/VST3/AUv3), preset management, and multi-engine architecture.

---

## Table of Contents

1. [Cross-Environment Compatibility](#cross-environment-compatibility)
2. [Skill System Quick Reference](#skill-system-quick-reference)
3. [Recommended Skills for XOlokun](#recommended-skills-for-xolokun)
4. [Community Skill Collections](#community-skill-collections)
5. [Skill Registries & Marketplaces](#skill-registries--marketplaces)
6. [Curated Awesome Lists](#curated-awesome-lists)
7. [Building Custom XOlokun Skills](#building-custom-xolokun-skills)
8. [Security Notes](#security-notes)

---

## Cross-Environment Compatibility

Skills work in both **local CLI** and **cloud web sessions** — but only if stored correctly.

### The Rule

| Location | CLI | Desktop | Web (claude.ai/code) |
|----------|-----|---------|----------------------|
| `.claude/skills/` (project, in repo) | Yes | Yes | **Yes** |
| `~/.claude/skills/` (user-level, local) | Yes | Yes | **No** |
| Plugin skills | Yes | Yes | **No** |

**For skills to work everywhere, commit them to the repo under `.claude/skills/`.**

### Single Definition Strategy

One `SKILL.md` works identically across all environments. Key practices:

- **Store in `.claude/skills/`** — committed to version control, available everywhere
- **Use `${CLAUDE_SKILL_DIR}`** for bundled scripts — avoids hardcoded absolute paths
- **Use `$CLAUDE_CODE_REMOTE`** to detect web vs local if behavior must diverge
- **No separate trigger mechanisms needed** — `/slash-command` invocation and auto-invocation work identically in CLI, Desktop, and web

### Environment Detection (in hook/script)

```bash
#!/bin/bash
if [ "$CLAUDE_CODE_REMOTE" = "true" ]; then
  # Cloud web session — no local filesystem access
  echo "Web session"
else
  # Local CLI or Desktop
  echo "Local session"
fi
```

### Hooks Compatibility

| Hook Type | CLI | Desktop | Web |
|-----------|-----|---------|-----|
| Project hooks (`.claude/settings.json` in repo) | Yes | Yes | Yes |
| User hooks (`~/.claude/settings.json` local) | Yes | Yes | No |

---

## Skill System Quick Reference

### SKILL.md Format

```yaml
---
name: my-skill                          # Slash command name (defaults to directory name)
description: What it does and when       # Drives auto-invocation — be specific
disable-model-invocation: true           # Only user can invoke (use for side-effects)
user-invocable: false                    # Only Claude can invoke (background knowledge)
argument-hint: [engine-name]             # Autocomplete hint
allowed-tools: Read, Grep, Glob         # Restrict tool access
context: fork                            # Run in isolated subagent
agent: Explore                           # Subagent type (with context: fork)
---

Instructions in markdown. Use $ARGUMENTS for passed args.
Use !`command` for dynamic context injection.
Reference [supporting-file.md](supporting-file.md) for details.
```

### Directory Structure

```
.claude/skills/
└── my-skill/
    ├── SKILL.md              # Required: main instructions
    ├── reference.md           # Optional: detailed docs
    ├── templates/             # Optional: output templates
    └── scripts/               # Optional: helper scripts
```

### Key Rules

- Keep `SKILL.md` under 500 lines — move detail to supporting files
- Use `disable-model-invocation: true` for anything with side effects (deploys, commits, exports)
- Use `allowed-tools` to restrict access (read-only skills shouldn't have Write)
- Descriptions with natural-language triggers improve auto-invocation accuracy

---

## Recommended Skills for XOlokun

### Tier 1 — High-Impact, Install First

#### 1. obra/superpowers
- **URL:** https://github.com/obra/superpowers
- **What:** Agentic development methodology — TDD (red-green-refactor), systematic 4-phase debugging, structured planning, git worktree management, code review
- **Why for XOlokun:** Enforces discipline on a 29-engine codebase. Tests must fail before implementation. Architecture review triggers after 3 failed fix attempts. Git worktree support for parallel engine work.
- **Companion:** https://github.com/obra/superpowers-marketplace
- **Quality:** Gold standard. Cited in every major curated list. Composable, self-evolving.

#### 2. Jeffallan/claude-skills — C++ Pro
- **URL:** https://github.com/Jeffallan/claude-skills
- **Docs:** https://jeffallan.github.io/claude-skills/skills/language/cpp-pro/
- **What:** Senior C++ engineer persona — C++20/23, concepts, ranges, coroutines, template metaprogramming, SFINAE, type traits, CRTP, smart pointers, custom allocators, move semantics, RAII, SIMD, atomics, lock-free programming, CMake, Conan, sanitizers, clang-tidy, cppcheck, Catch2, GoogleTest
- **Why for XOlokun:** Directly covers the tech stack. DSP code benefits from SIMD, lock-free, and sanitizer expertise. CMake build guidance.
- **66 total skills** across 12 categories (languages, frameworks, infrastructure, security, testing)

#### 3. juce-dev Plugin
- **Source:** https://danielraffel.me/2026/03/06/a-claude-code-plugin-for-building-juce-audio-plugins/
- **What:** JUCE plugin scaffolding, CMake project generation, Xcode config, code signing, Metal GPU UI
- **Why for XOlokun:** AU/VST3/AUv3 plugin development patterns. CMake scaffold for audio plugins. Xcode/code signing workflow.
- **Invoke:** `/juce-dev:create "Plugin Name"`

### Tier 2 — Strong Additions

#### 4. alirezarezvani/claude-skills
- **URL:** https://github.com/alirezarezvani/claude-skills
- **What:** 177 production-ready skills across 9 domains. Senior Architect, QA, DevOps, SecOps, Code Reviewer roles. 254 Python automation scripts (stdlib-only).
- **Stars:** 4,400+ (most starred community library)
- **Install:** `/plugin marketplace add alirezarezvani/claude-skills`
- **Why for XOlokun:** Architect + QA roles for engine design review. Python scripts complement the existing Tools/ directory.

#### 5. Trail of Bits — Security Skills
- **URL:** https://github.com/trailofbits/skills
- **Curated (code-reviewed):** https://github.com/trailofbits/skills-curated
- **What:** Vulnerability detection, audit workflows, constant-time analysis, differential security review with git history
- **Why for XOlokun:** Plugin code runs in DAW host processes — security matters. Curated repo is staff-reviewed for safety.

#### 6. Code Review Skills
- **aidankinzett/claude-git-pr-skill:** https://github.com/aidankinzett/claude-git-pr-skill — PR review with pending reviews, code suggestions, user approval flow
- **awesome-skills/code-review-skill:** https://github.com/awesome-skills/code-review-skill — 4-phase review, severity labeling, per-language security checklists

### Tier 3 — Specialized / Situational

#### 7. levnikolaevich/claude-code-skills
- **URL:** https://github.com/levnikolaevich/claude-code-skills
- **What:** Full delivery workflow — research, discovery, epic planning, task breakdown, implementation, testing, code review, quality gates. Orchestrator-Worker pattern (L1/L2/L3 agents).
- **When useful:** Large multi-engine refactoring sessions, fleet-wide changes (like the Prism Sweep)

#### 8. mhattingpete/claude-skills-marketplace
- **URL:** https://github.com/mhattingpete/claude-skills-marketplace
- **What:** Git automation, test fixing, code review, feature planning, bulk refactoring (auto-switches for 10+ files with 90% token savings)
- **When useful:** Bulk preset fixes, fleet-wide parameter changes

#### 9. Anthropic Official Skills
- **URL:** https://github.com/anthropics/skills (37.5k stars)
- **17 official skills:** algorithmic-art, brand-guidelines, canvas-design, claude-api, doc-coauthoring, docx, frontend-design, internal-comms, mcp-builder, pdf, pptx, **skill-creator**, theme-factory, web-artifacts-builder, webapp-testing, xlsx
- **Key skill:** `skill-creator` — interactively guides you through building new skills (useful for creating custom XOlokun skills)

---

## Community Skill Collections

| Collection | URL | Size | Notes |
|-----------|-----|------|-------|
| **obra/superpowers** | [GitHub](https://github.com/obra/superpowers) | Framework | TDD, debugging, planning, git worktrees |
| **alirezarezvani/claude-skills** | [GitHub](https://github.com/alirezarezvani/claude-skills) | 177 skills | 4,400+ stars, cross-compatible |
| **Jeffallan/claude-skills** | [GitHub](https://github.com/Jeffallan/claude-skills) | 66 skills | C++ Pro, browsable docs site |
| **VoltAgent/awesome-agent-skills** | [GitHub](https://github.com/VoltAgent/awesome-agent-skills) | 500+ skills | Verified from Anthropic, Google, Stripe, Cloudflare |
| **Trail of Bits** | [GitHub](https://github.com/trailofbits/skills) | Security | Staff code-reviewed curated repo |
| **levnikolaevich** | [GitHub](https://github.com/levnikolaevich/claude-code-skills) | Workflow | Orchestrator-Worker delivery pipeline |
| **mhattingpete** | [GitHub](https://github.com/mhattingpete/claude-skills-marketplace) | Dev tools | Git, test fixing, bulk refactoring |
| **Anthropic Official** | [GitHub](https://github.com/anthropics/skills) | 17 skills | 37.5k stars, includes skill-creator |

---

## Skill Registries & Marketplaces

| Registry | URL | Size | Notes |
|----------|-----|------|-------|
| **SkillsMP** | [skillsmp.com](https://skillsmp.com) | 400,000+ | Aggregates from GitHub, min 2 stars filter |
| **SkillHub** | [skillhub.club](https://www.skillhub.club/) | 7,000+ | AI-evaluated, Claude/Codex/Gemini/OpenCode |
| **MCP Market** | [mcpmarket.com/tools/skills](https://mcpmarket.com/tools/skills) | — | Agent Skills directory |
| **mcpservers.org** | [mcpservers.org/agent-skills](https://mcpservers.org/agent-skills) | — | Discover reusable skills |
| **awesome-skills.com** | [awesome-skills.com](https://awesome-skills.com/) | 122+ | Curated — testing, dev tools, docs |
| **ClaudePluginHub** | [claudepluginhub.com](https://www.claudepluginhub.com/) | — | Plugin/skill browser |

---

## Curated Awesome Lists

| List | URL | Focus |
|------|-----|-------|
| **travisvn/awesome-claude-skills** | [GitHub](https://github.com/travisvn/awesome-claude-skills) | Skills + resources |
| **hesreallyhim/awesome-claude-code** | [GitHub](https://github.com/hesreallyhim/awesome-claude-code) | Skills, hooks, commands, agents, plugins |
| **jqueryscript/awesome-claude-code** | [GitHub](https://github.com/jqueryscript/awesome-claude-code) | Tools + integrations with star ratings |
| **ComposioHQ/awesome-claude-skills** | [GitHub](https://github.com/ComposioHQ/awesome-claude-skills) | Skills + tools |
| **BehiSecc/awesome-claude-skills** | [GitHub](https://github.com/BehiSecc/awesome-claude-skills) | Security-focused, includes claude-starter (40 auto-activating skills) |
| **karanb192/awesome-claude-skills** | [GitHub](https://github.com/karanb192/awesome-claude-skills) | 50+ verified skills |

---

## Building Custom XOlokun Skills

No audio DSP skill exists in the community. XOlokun has unique requirements that warrant custom skills. These should live in `.claude/skills/` (project-level) for cross-environment compatibility.

### Recommended Custom Skills to Build

#### `/validate-engine` — Doctrine Compliance Check
Run the 6 Doctrines against any engine. Checks velocity→timbre (D001), modulation (D002), physics rigor (D003), dead parameters (D004), LFO breathing (D005), and expression input (D006).

#### `/new-xo-engine` — Engine Creation Wizard
Already referenced in CLAUDE.md. Walk through ideation, architecture, scaffold, and integration. Encode the rules from `Docs/xolokun_new_engine_process.md`.

#### `/preset-audit` — Preset Health Check
Validate all 2,369 presets: schema compliance, DNA coverage, duplicate detection, macro effectiveness, naming conventions.

#### `/export-xpn` — MPC Export Validation
Enforce the 3 critical XPM rules: `KeyTrack=True`, `RootNote=0`, empty layer `VelStart=0`.

#### `/seance` — Structured Engine Evaluation
Formalized seance protocol for evaluating engine quality against the ghost panel.

#### `/dsp-safety` — Audio Thread Safety Audit
Check for memory allocation, blocking I/O, missing denormal protection, and feedback path safety on the audio thread. Encodes the Architecture Rules from CLAUDE.md.

### Custom Skill Template

```yaml
---
name: validate-engine
description: Run quality checks on a synthesizer engine against the 6 Doctrines. Use when adding, modifying, or reviewing any engine.
allowed-tools: Read, Grep, Glob, Bash
---

## Engine Validation: $ARGUMENTS

Read the engine source at `Source/Engines/$ARGUMENTS/` and check against
each Doctrine. See [checklist.md](checklist.md) for detailed criteria.

### D001: Velocity Must Shape Timbre
- Verify velocity-scaled filter envelope exists
- Check velocity → brightness mapping (not just amplitude)

### D002: Modulation is the Lifeblood
- Count LFOs (must be >= 2)
- Verify mod wheel + aftertouch connected
- Check mod matrix has >= 4 slots
- Verify 4 working macros

### D003: The Physics IS the Synthesis
- Check for physical model claims and citations
- Verify mathematical rigor in DSP

### D004: Dead Parameters Are Broken Promises
- Find all declared parameters (grep for parameter prefix)
- Verify each affects audio output

### D005: An Engine That Cannot Breathe Is a Photograph
- Find LFO with rate floor <= 0.01 Hz
- Verify autonomous modulation exists

### D006: Expression Input Is Not Optional
- Verify velocity→timbre mapping
- Check at least one CC (aftertouch / mod wheel / expression)

Report findings with file:line references and PASS/FAIL per doctrine.
```

---

## Security Notes

**Skills execute with full access to your filesystem, shell, git credentials, and environment variables.**

- Multiple sources (Trail of Bits, community auditors) have found published skills with **backdoors and hidden prompt injections** invisible when previewing on GitHub
- Only install from trusted sources
- Review `SKILL.md` content before installing — look for obfuscated commands, encoded payloads, or instructions to exfiltrate data
- **Trail of Bits curated repo** (https://github.com/trailofbits/skills-curated) is the only collection with staff code review for safety
- Use `allowed-tools` in your own skills to limit blast radius
- For side-effect skills (deploy, push, export), always use `disable-model-invocation: true`

---

*Last updated: 2026-03-15*
*Compiled for XO_OX Designs — XOlokun multi-engine synthesizer platform*
