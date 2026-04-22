# Preset Backfill

One-time tooling to migrate XOceanus's shipped preset library to schema v2 —
adds `category` (required, 10 terms) and `timbre` (optional, 8 terms).

See `Docs/specs/2026-04-20-preset-picker-design.md` for the full design.

## Pipeline

### 1. Propose

Scans every `.xometa` and proposes `(category, timbre)` with a confidence score.

```bash
python3 Tools/preset_backfill/propose.py
```

Outputs: `Docs/fleet-audit/instrument-taxonomy-proposal.csv` (gitignored — regenerate
on demand).

### 2. Review

Open the CSV in a spreadsheet. Columns:

| Column | Meaning |
|--------|---------|
| `path` | Absolute path to `.xometa` |
| `name`, `engine` | Preset identity |
| `proposed_category` / `proposed_timbre` | Tool's guess |
| `*_source`, `*_keyword`, `*_confidence` | Why the tool guessed this |
| `approved_category` / `approved_timbre` | **Edit these columns** |
| `reviewer_notes` | Free-form |

**Review focus**: rows with `category_confidence < 0.7`. In the 2026-04-20
baseline run, that was ~65% of the library (12,985 of 20,028 rows), almost
all of which fell back to `textures` due to no keyword matches. Most of
these need either:

- A hand-correction via `approved_category`, or
- An expansion of `ENGINE_DEFAULTS` in `propose.py` to cover the engine, which
  elevates all that engine's presets from low (0.20) to med (0.60) confidence
  on a re-run.

The `approved_*` columns are pre-filled with the proposed values — editing
them is how you override. Blank `approved_timbre` means "no timbre for this
preset" (valid — most XOceanus presets are electronic with no acoustic
reference).

### 3. Apply (dry-run first)

```bash
python3 Tools/preset_backfill/apply.py --dry-run
```

Reports what would change. Expect ~20,000 `would_change` entries, zero errors.

### 4. Apply (for real)

```bash
python3 Tools/preset_backfill/apply.py
```

Rewrites each `.xometa` with:

- `schema_version: 2`
- `category: <approved value>`
- `timbre: <approved value or absent>`

Auto-detects existing indentation to keep diffs minimal.

### 5. Verify

```bash
python3 Tools/verify_schema_compatibility.py
```

Expected post-backfill:

- `v1_count: 0`
- `v2_count: <total>`
- `missing_category: 0`
- `malformed: 0`
- `invalid_category: 0`
- `invalid_timbre: 0`

## Reruns

Both tools are idempotent — re-running `apply.py` with the same CSV does
nothing (reports `unchanged`). Re-running `propose.py` regenerates the CSV
from scratch, overwriting any manual review work.

**Commit the reviewed CSV before running apply.py** so review work is
recoverable. The default `.gitignore` excludes the generated CSV — remove
that entry (or `git add -f`) if you want to preserve review history.

## Rolling back

If apply.py produces unacceptable results, revert the preset library with:

```bash
git checkout -- "XOceanus Presets/"
```

(assuming the preset directory is where your library lives; adjust to your
actual preset root.) Then regenerate the CSV, correct the review, and apply
again.
