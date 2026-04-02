# Export Architecture — The Export Pyramid

## Overview

Three tools handle sample export in XOceanus, each at a different level:

```
ORIGINATE  ← UI wizard (desktop app)
    │
OUTSHINE   ← DSP engine (processing brain)
    │
OXPORT     ← batch pipeline (CLI factory)
```

## Originate (User-Facing)

**What**: The desktop UI's sample import wizard.
**Where**: `Source/Export/XOriginate.h` + `Source/UI/Outshine/` (UI components)
**Who uses it**: Producers in the desktop app.
**Flow**: Drag samples → choose profile → preview → export.

## Outshine (DSP Engine)

**What**: The processing engine behind both Originate and Oxport.
**Where**: `Source/Export/XOutshine.h` (C++ DSP engine)
**Who uses it**: Called by Originate (UI) and Oxport (CLI). Never used directly by producers.
**Capabilities**: Sample classification, pitch detection, velocity layering, LUFS normalization, Rebirth FX profiles, loop detection, expression mapping.

## Oxport (Batch Pipeline)

**What**: CLI tool for automated expansion pack creation at scale.
**Where**: `Tools/oxport.py` (Python, 10-stage pipeline)
**Who uses it**: Developers/operators building expansion packs.
**Flow**: render specs → categorize → expand → QA → trim → export → cover art → complement → preview → package.

## When to Use What

| Goal | Tool |
|------|------|
| Import a few samples into a playable instrument | **Originate** (desktop UI) |
| Process samples with specific DSP settings | **Outshine** (via Originate UI or code) |
| Build a full MPC expansion pack (100+ presets) | **Oxport** (CLI: `python3 Tools/oxport.py build`) |
| Batch export multiple engines | **Oxport** (CLI: `python3 Tools/oxport.py batch`) |

## The Producer's Mental Model

> "I **Originate** instruments from my samples.
> The **Outshine** engine makes them sound incredible.
> When I need a full expansion pack, **Oxport** builds it at scale."
