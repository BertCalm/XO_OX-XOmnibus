# BLS-001: OBRIX CytomicSVF Filter Choice

**Ghost**: Moog
**Engine**: OBRIX (2026-03-19)

Bob Moog blessed the choice of CytomicSVF (Andrew Simper's state variable filter) as the filter topology for OBRIX. "That's a serious decision. It tells me someone here is not faking warmth, they're engineering it." The unified LP/HP/BP from the same integrator chain means three Processor slots draw from a cooperative resonant structure rather than disconnected biquads. Protect this topology through future refactoring — do not replace with cheaper biquad implementations.
