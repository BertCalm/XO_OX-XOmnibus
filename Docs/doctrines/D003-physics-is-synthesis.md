# D003 — The Physics IS the Synthesis

**Status:** ONGOING — applies to physically-modeled engines only

## Definition

For any engine that claims physical modeling (strings, membranes, percussion, room acoustics, fluid dynamics), the physics must be rigorously implemented and academically cited. The mathematical model is the artistic statement. Approximations that sound plausible but are physically wrong undermine the integrity of the instrument.

## Requirements

- Physical model implementations must cite the academic source (paper, author, year)
- Approximations must be documented with their limitations
- Parameters must map to physically meaningful quantities (e.g., mallet hardness, string tension, room volume)
- The model must behave in ways that are physically intuitive (stiffer = brighter, heavier = lower pitch)

## Examples of Compliant Implementations

- OWARE: Chaigne 1997 mallet contact-time synthesis — B032 blessed
- OSTINATO: Bessel zeros sourced from physics papers for modal membrane synthesis — B017 blessed
- OBSCURA: Daguerreotype silver — stiffness parameter maps to physical plate stiffness
- ORGANON: Variational Free Energy metabolism — B011 blessed, "publishable as paper"

## Common Failure Patterns

- Claiming "physical modeling" while using only AM/FM synthesis
- Filter cutoff labeled "String Tension" with no physical relationship to actual string tension
- Missing or fabricated citations
- Parameters that don't behave physically (e.g., stiffer = darker — backwards)

## Test Criteria

For physically-modeled engines:
1. Identify the claimed physical system
2. Verify academic citations exist in the engine header or synthesis guide
3. Test parameter extremes — do they behave as the physical system would?
4. Verify the model has audible failure modes consistent with physics (e.g., bow pressure too high = scratchy)

## Fleet Compliance

Applies to: OWARE, OSTINATO, OBSCURA, ORGANON, OCELOT, OSIER, OXALIS, OVERWASH, and any future physically-modeled engine.
Non-physical engines are exempt.
