# Primitive: Bulk Transform

Apply a transformation (rename, reformat, migrate) across all files matching a pattern.

## Pattern

```
find(file_pattern)
  → for each file:
      read(file)
      transform(content, rules)
      write(file, transformed)
  → report(changed_count, skipped_count, error_count)
```

## Inputs

- **file_pattern**: Glob or regex to select target files (e.g., `Presets/**/*.xometa`)
- **transform_rules**: The changes to apply (rename map, schema migration, format conversion)
- **dry_run**: Preview changes without writing (default: true first pass)
- **backup**: Whether to create backups before writing

## Outputs

- Count of files changed, skipped, errored
- Diff summary of what changed
- Backup locations if created

## Usage Examples

**XOmnibus engine rename:**
```python
rules = {"XOddCouple": "OddfeliX", "XOblongBob": "XOblong"}
# Tools/apply_renames.py applies across presets, docs, and tools
```

**Generic:**
```bash
# Rename an API field across all configs
# Migrate schema version in all manifest files
# Convert date formats across data files
```

## Portability Notes

Any project that undergoes renames, schema migrations, or format changes needs this. The key safety features are: dry-run first, backup before write, report on completion.

## Safety Checklist

- [ ] Dry run first — preview all changes
- [ ] Verify file count matches expectations
- [ ] Check for false positives (partial string matches)
- [ ] Back up before applying (or rely on git)
- [ ] Re-validate after transform completes
