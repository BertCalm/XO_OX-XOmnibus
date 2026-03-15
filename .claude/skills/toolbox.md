# /toolbox — Framework Launcher & Skill Index

Single entry point to the primitive>skill>context framework. Lists what's available, suggests what's relevant, and launches skills.

## Usage

- `/toolbox` — Show everything available with status
- `/toolbox mine` — Jump to `/mine-skills`
- `/toolbox discover` — Jump to `/discover-skills`
- `/toolbox optimize` — Jump to `/optimize-chains`
- `/toolbox log` — Jump to `/log-session`
- `/toolbox meta` — Jump to `/meta-mine`
- `/toolbox status` — Framework health summary
- `/toolbox sync` — Sync with central framework repo

## When Invoked Without Arguments

Display the full toolbox:

```markdown
## Toolbox — {project name} ({project_type})
Framework v{version} | {N} skills | {N} primitives | {N} context slices

### Skills (invoke with /toolbox {shortcut})
| Shortcut | Skill | What it does | Last used |
|----------|-------|-------------|-----------|
| `log` | /log-session | Record this session's work to the journal | {date or "never"} |
| `mine` | /mine-skills | Find patterns in the journal worth promoting | {date or "never"} |
| `meta` | /meta-mine | Cross-repo pattern analysis | {date or "never"} |
| `discover` | /discover-skills | "You might also need..." recommendations | {date or "never"} |
| `optimize` | /optimize-chains | Find chains, improvements, quality gaps | {date or "never"} |

### Primitives (building blocks used by skills)
| Primitive | Pattern |
|-----------|---------|
| validate-fix-loop | Run validator → fix → re-validate |
| scaffold-and-register | Template → create → register → verify |
| bulk-transform | Find files → transform → report |
| chain-pipeline | Step → gate → step → gate → done |
| cross-repo-sync | Sync framework between repos |

### Context Slices (loaded by skills as needed)
| Context | Contains |
|---------|----------|
| skill-mining | Framework overview, available components |
| framework-levels | Maturity stages and advancement signals |

### Journal
- Entries: {count from entries.jsonl}
- Latest: {timestamp of last entry}
- Stage: {Capture | Identify | Extract | Compose | Mature}

### Quick Actions
Based on current state, you should probably:
- {Recommend next action based on journal entry count and last activity}
```

## When Invoked with "status"

Provide a health check:

1. **Read `framework-manifest.json`** for component inventory
2. **Count journal entries** in `entries.jsonl`
3. **Check for stale context** — context files older than 30 days vs. their source files
4. **Check framework home** — is `SKILL_FRAMEWORK_HOME` set? Is it reachable?
5. **Determine maturity stage** based on entry count and project count:
   - < 10 entries → Capture
   - 10-30 entries → Identify
   - 30-50 entries, 2+ projects → Extract
   - 5+ primitives extracted → Compose
   - 70%+ coverage, 3+ projects → Mature

6. **Suggest next action:**
   - If < 3 entries: "Run `/toolbox log` after your next substantive session"
   - If 10+ entries and never mined: "Run `/toolbox mine` to find your first patterns"
   - If skills exist but never chained: "Run `/toolbox optimize` to find chain opportunities"
   - If framework_home is null: "Set `SKILL_FRAMEWORK_HOME` to enable cross-repo features"

## When Invoked with "sync"

Run the cross-repo-sync primitive:

1. Check if `SKILL_FRAMEWORK_HOME` is set
2. If not set, explain how to set it up and offer to create the central framework directory
3. If set, run sync:
   - Pull: copy universal primitives from framework home → this project
   - Push: copy project journal entries → framework home aggregated journal
   - Diff: show any local modifications to shared components
   - Update `last_sync` in manifest

## Dynamic Skill Registration

When new skills are added to `.claude/skills/`, they should be picked up automatically. The toolbox reads the directory, not a hardcoded list. The manifest tracks what's *expected*, but the directory is the source of truth.

To add a new skill to the toolbox:
1. Create `.claude/skills/{name}.md`
2. Add it to `framework-manifest.json` skills array
3. It appears in `/toolbox` automatically

## Proactive Suggestions

When `/toolbox` is invoked, also check:
- Has it been 5+ sessions since the last `/log-session`? → Remind to log
- Has it been 10+ entries since the last `/mine-skills`? → Suggest mining
- Are there approved but unimplemented candidates from the last mining report? → Remind
- Is the journal empty? → Walk through first entry creation
