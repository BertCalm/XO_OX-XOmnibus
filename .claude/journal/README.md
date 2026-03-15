# Skill Journal

This directory contains session logs used by the skill mining framework.

## Files

- `schema.json` — JSON Schema for journal entries
- `entries.jsonl` — Append-only log of session entries (one JSON object per line)

## How It Works

After each substantive session (multi-step work, not trivial edits), a journal entry is appended to `entries.jsonl`. The `/mine-skills` skill reads these entries to identify patterns worth promoting to the framework.

## Cross-Repo Aggregation

The `/meta-mine` skill can read journals from multiple repositories. Set the `SKILL_FRAMEWORK_HOME` environment variable to point to your central framework repo, which aggregates entries from all projects.

```
~/.claude/framework/          # or a dedicated repo
  journal/
    aggregated.jsonl           # merged from all projects
  primitives/                  # portable across all projects
  meta-skills/
    meta-mine.md               # cross-repo mining skill
```
