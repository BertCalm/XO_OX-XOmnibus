# DOC-001: Every Filter Needs Key Tracking

**Ghosts**: Moog, Pearlman, Tomita
**Engine**: OBRIX (2026-03-19)

A filter without key tracking sounds inconsistent across registers — bass notes muddy, high notes brittle. Cutoff should scale with note frequency (typically +100 cents/octave). This is not cosmetic. It is playability.

**Applies to**: Any engine with a static filter cutoff parameter. Key tracking should be a parameter (`engine_filterKeyTrack`, 0–100%) that scales cutoff linearly with MIDI note number.
