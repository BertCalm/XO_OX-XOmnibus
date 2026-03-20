# VIS-001: LFO Audio-Rate Crossover as Synthesis Mode

**Ghost**: Schulze (with Buchla implicit agreement)
**Engine**: OBRIX (2026-03-19)

When LFO rate crosses from modulation territory (< ~20 Hz) into audio territory (20+ Hz), modulation becomes synthesis. The boundary is not a cliff — it is a gradient. An LFO that sweeps from 0.01 Hz to 80+ Hz lets a musician slide from "slow breath" to "FM synthesis" without a mode switch. This is a richer timbral vocabulary than simply having separate LFO and FM sections.

**Target for OBRIX**: Extend `obrix_mod1Rate` and `obrix_mod2Rate` upper bound from 30 Hz → 80 Hz.
**Broader principle**: LFO max rate for any expressive engine should reach audio territory.
