# Primitive: Cross-Repo Sync

Synchronize framework components (primitives, skills, context) between a central framework repo and individual project repos.

## Pattern

```
sync(framework_home, project_path, direction)
  → if direction == "pull":
      read manifest from project
      copy matching components from framework_home to project
  → if direction == "push":
      copy project-local components to framework_home
      tag with project origin
  → if direction == "diff":
      compare project versions against framework canonical versions
      report drift
```

## Inputs

- **framework_home**: Path to central framework repo (`$SKILL_FRAMEWORK_HOME`)
- **project_path**: Path to the current project's `.claude/` directory
- **direction**: `pull` (framework → project), `push` (project → framework), or `diff`
- **manifest**: `.claude/framework-manifest.json` listing which shared components this project uses

## Manifest Format

```json
{
  "framework_version": "1.0.0",
  "primitives": ["validate-fix-loop", "scaffold-and-register", "bulk-transform"],
  "skills": [],
  "context": [],
  "project_type": "synth-engine",
  "last_sync": "2026-03-12T17:00:00Z"
}
```

## Outputs

- Files copied/updated
- Drift report (local modifications vs. canonical versions)
- Manifest updated with sync timestamp

## Portability Notes

This is the mechanism that makes the framework actually portable. Without sync, each project's `.claude/` directory drifts independently. With sync, universal primitives stay consistent while project-specific skills remain local.
