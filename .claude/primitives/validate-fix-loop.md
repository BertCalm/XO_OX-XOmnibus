# Primitive: Validate-Fix Loop

Run a validator, parse structured output, auto-fix trivial issues, re-validate until clean.

## Pattern

```
validate(target, rules)
  → if errors:
      fixable = errors.filter(auto_fixable)
      blockers = errors.filter(not auto_fixable)
      apply_fixes(fixable)
      report_blockers(blockers)
      validate(target, rules)  # re-run
  → if clean:
      return success
```

## Inputs

- **target**: What to validate (files, configs, schemas, etc.)
- **validator**: The command or script that checks compliance
- **fix_mode**: Whether to auto-fix (e.g., `--fix` flag) or report only
- **max_iterations**: Prevent infinite loops (default: 3)

## Outputs

- Validation report (clean or remaining blockers)
- List of auto-applied fixes
- Count of iterations needed

## Usage Examples

**XOlokun presets:**
```bash
python3 Tools/validate_presets.py --strict    # validate
python3 Tools/validate_presets.py --fix       # auto-fix + re-validate
```

**Generic (any project):**
```bash
eslint --fix src/                             # JS/TS linting
black --check . && black .                    # Python formatting
terraform validate && terraform fmt           # Infrastructure
```

## Portability Notes

This primitive works anywhere there's a validator with structured output. The shape is always:
1. Run check
2. Parse results into fixable vs. blockers
3. Apply fixes
4. Re-check
5. Report remaining issues
