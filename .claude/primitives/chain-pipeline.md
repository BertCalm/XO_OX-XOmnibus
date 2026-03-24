# Primitive: Chain Pipeline

Run a multi-step process where each step's output feeds the next, with validation gates between steps.

## Pattern

```
pipeline(steps, input)
  → for each step:
      output = step.run(input)
      if step.gate:
        result = step.gate(output)
        if result.failed:
          report_failure(step, result)
          return  # stop pipeline
      input = output  # feed to next step
  → report_success(final_output)
```

## Inputs

- **steps**: Ordered list of operations, each with:
  - `run`: The action to execute
  - `gate`: Optional validation check before proceeding
  - `on_fail`: What to do on gate failure (stop, skip, retry)
- **input**: Initial data fed to the first step

## Outputs

- Final output from last step
- Pipeline report (which steps ran, which gates passed, timing)

## Usage Examples

**XOlokun export pipeline:**
```
Step 1: Select presets (filter by mood/engine)    → gate: at least 1 preset
Step 2: Bundle into XPN format                     → gate: all files valid
Step 3: Generate cover art                         → gate: image dimensions correct
Step 4: (if Onset) Export drum kit                  → gate: WAV files exist
Step 5: Package final .xpn                         → gate: bundle size reasonable
```

**Generic CI/CD:**
```
Step 1: Lint          → gate: no errors
Step 2: Test          → gate: all pass
Step 3: Build         → gate: no warnings
Step 4: Deploy        → gate: health check
```

## Portability Notes

Any multi-step process with quality gates between steps. The primitive handles: step ordering, gate checking, failure handling, and pipeline reporting. The specific steps and gates are project-specific.
