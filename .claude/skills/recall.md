# /recall — Session Memory & Continuity

Search and surface relevant findings from past sessions. Answers "what did we try last time?" and "what was the context for this decision?"

## Usage

- `/recall {topic}` — Search past session logs for relevant context
- `/recall last` — Summarize the most recent session
- `/recall decisions` — List key decisions made across sessions
- `/recall --engine {engine}` — All session history related to a specific engine
- `/recall blockers` — Find unresolved blockers and open questions from past sessions

## How It Works

### Session logs as memory

`/log-session` writes structured session logs to `.claude/sessions/`. These logs contain:
- What was worked on (tasks)
- What was discovered (findings)
- What was decided (decisions)
- What was left unresolved (blockers, open questions)
- What was tried and didn't work (failed approaches)

`/recall` searches these logs to bring relevant context into the current session.

### Search strategy

For a given query, search in order:

1. **Session logs** in `.claude/sessions/` — grep for topic keywords
2. **Git log** — commit messages mentioning the topic
3. **TODO/FIXME comments** — in-code notes about the topic
4. **Docs** — specification and design docs for background context

### Context assembly

Collect findings and present them chronologically:

```markdown
## Recall: {topic}

### Session History
| Date | Session | What Happened | Outcome |
|------|---------|---------------|---------|
| {date} | {session-id} | {summary} | {resolved/deferred/abandoned} |

### Key Decisions
| Decision | Date | Rationale | Still Valid? |
|----------|------|-----------|-------------|
| {what was decided} | {when} | {why} | {yes/maybe/revisit} |

### Unresolved Items
- {blocker or open question from a past session}
- {another}

### Failed Approaches
| Approach | Date | Why It Failed | Lesson |
|----------|------|---------------|--------|
| {what was tried} | {when} | {why it didn't work} | {what we learned} |

### Related Commits
| Hash | Date | Message |
|------|------|---------|
| {short hash} | {date} | {commit message} |

### Relevant Context
{Any spec or doc references that provide background}
```

## Process

### 1. Search session logs

```bash
# Search session log files for the topic
grep -rl "{topic}" .claude/sessions/ 2>/dev/null
```

Parse matching session logs for structured data (tasks, decisions, blockers).

### 2. Search git history

```bash
git log --oneline --all --grep="{topic}" --since="6 months ago"
```

### 3. Search codebase

Look for TODO/FIXME/HACK comments related to the topic:
```
# grep for in-code notes
rg "TODO|FIXME|HACK" --glob "*.{h,cpp}" | grep -i "{topic}"
```

### 4. Check docs

Search `Docs/` for the topic — often the spec or design docs contain historical context about why things are the way they are.

### 5. Synthesize

Combine all sources into the recall report. Prioritize:
- **Decisions** — most actionable (tells you what was already decided)
- **Failed approaches** — prevents repeating mistakes
- **Unresolved items** — tells you what still needs attention
- **Context** — background for understanding the current state

## Special Modes

### `/recall decisions`
Filter to only decisions across all sessions. Useful for understanding the decision log without noise.

### `/recall blockers`
Filter to only unresolved blockers and open questions. Useful for "what do we still need to figure out?"

### `/recall last`
Just the most recent session summary. Quick context restoration at the start of a new session.

## Primitives Used
- None — this is a read-only search/synthesis skill

## Relationship to Other Skills
- Depends on: `/log-session` (produces the session logs that `/recall` searches)
- Feeds into: `/research` (historical context enriches research), `/recommend` (past decisions constrain recommendations)
- Natural workflow: `/recall {topic}` → `/research {topic}` → work

## Notes
- `/recall` is only as good as `/log-session` output — log sessions consistently
- Session logs should capture *why* decisions were made, not just *what* was done
- Failed approaches are some of the most valuable recall data — they prevent wasted effort
- If no session logs exist yet, `/recall` falls back to git history and code comments
- This skill never modifies files — it only reads and synthesizes
- Consider running `/recall {topic}` at the start of any session that continues previous work
