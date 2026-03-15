# /log-session — Session Journal Entry

Log the current session's work to the skill journal for pattern mining.

## When to Use

Run at the end of any substantive session — one that involved 3+ steps, used multiple tools, or required loading significant context. Skip for trivial single-file edits.

## Process

1. **Review what was done this session.** Look at the conversation history, tools invoked, files changed, and context loaded.

2. **Identify the task type.** Classify the work:
   - `scaffolding` — created new files/modules from patterns
   - `validation` — ran checks, fixed issues
   - `generation` — produced content programmatically
   - `migration` — renamed, reformatted, or upgraded existing artifacts
   - `export` — built distributable output
   - `integration` — wired new components into existing systems
   - `bug-fix` — diagnosed and fixed a defect
   - `refactor` — restructured without changing behavior
   - `research` — explored codebase or docs to answer a question
   - `process-design` — designed or improved a workflow

3. **Extract the step sequence.** List each distinct action in order. For each step note:
   - The verb (scaffold, validate, generate, transform, register, test, document, export, migrate)
   - The target (what was acted on)
   - The tool used (if any)
   - Whether it could be automated (`repeatable: true/false`)

4. **Flag friction points.** Anything that:
   - Required re-explaining context that should have been pre-loaded
   - Was done manually but felt like it should be scripted
   - Was a repeated sequence you've done before (in this or another project)
   - Required hunting for information across multiple files

5. **Tag patterns.** Add freeform tags that describe the *shape* of the work:
   - `scaffold-then-register` — created something, then wired it into an index
   - `validate-fix-loop` — ran validation, fixed issues, re-validated
   - `chain-export` — ran multiple export steps in sequence
   - `context-heavy-start` — spent significant time loading context before acting
   - `cross-file-rename` — same change applied across many files

6. **Append the entry** to `.claude/journal/entries.jsonl` as a single JSON line following the schema in `.claude/journal/schema.json`.

## Example Entry

```json
{
  "timestamp": "2026-03-12T17:30:00Z",
  "project": "XOmnibus",
  "project_type": "synth-engine",
  "task_type": "generation",
  "summary": "Generated 40 Overworld presets with DNA fingerprints",
  "steps": [
    {"action": "generate", "target": "preset files", "tool": "generate_overworld_presets.py", "repeatable": true},
    {"action": "validate", "target": "preset files", "tool": "validate_presets.py", "repeatable": true},
    {"action": "transform", "target": "preset DNA", "tool": "compute_preset_dna.py", "repeatable": true},
    {"action": "validate", "target": "preset files", "tool": "validate_presets.py --fix", "repeatable": true}
  ],
  "tools_used": ["generate_overworld_presets.py", "validate_presets.py", "compute_preset_dna.py"],
  "context_loaded": ["xometa_schema.json", "xomnibus_preset_spec_for_builder.md"],
  "context_gaps": [],
  "friction_points": ["Had to run validate twice — once before DNA, once after"],
  "pattern_tags": ["validate-fix-loop", "chain-export"]
}
```

## Cross-Repo Note

If `SKILL_FRAMEWORK_HOME` is set, also append a copy of this entry to `$SKILL_FRAMEWORK_HOME/journal/aggregated.jsonl` so the cross-repo meta-miner can see it.
