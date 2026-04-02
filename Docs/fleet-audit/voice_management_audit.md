# Voice Management Audit
**Date:** 2026-03-14
**Prism Sweep Round:** 8
**Engines audited:** Snap (OddfeliX), Orbital, Obsidian, Origami, Oracle, Morph (OddOscar), Oblique, Ocelot, Overworld, Osprey

---

## 1. Audit Table

| Engine | Max Voices | Stealing Policy | Legato | Note-off Handling | Stuck-note Protection |
|--------|-----------|----------------|--------|------------------|-----------------------|
| **Snap** | 8 (param: 1/2/4/8) | LRU (oldest startTime) + 5ms crossfade | None — percussive engine; always retriggered (appropriate) | No release stage; note-off forces envelope to ~0.01 and lets exponential decay finish | allNotesOff → reset(); polyphony-limit guard forces extra voices to fade quickly |
| **Orbital** | 6 (fixed) | LRU (oldest startTime) + 5ms crossfade | None — no voice mode param; always poly, always retriggered | Full ADSR release phase; decay/release min clamped to 50ms | allNotesOff sets all voices to Off + active=false immediately |
| **Obsidian** | 16 (param: Mono/Legato/Poly16) | LRU (oldest startTimestamp) + 5ms crossfade (capped at 50% on poly steal) | Full — voiceMode=0 Mono (retrigger), voiceMode=1 Legato (pitch glide, no retrigger when gate open), voiceMode=2 Poly16 | Full ADSR on amplitude and PD envelopes; isFadingOut guard prevents double-release | allNotesOff → reset(); sustainPedal (CC64) tracked |
| **Origami** | 8 (param: Mono/Legato/Poly4/Poly8) | LRU (oldest startTime) + 5ms crossfade (capped at 50%) | Full — voiceMode=1 Legato detects wasAlreadyActive; glide coefficient via exponential IIR | Full ADSR on amp + fold envelopes | allNotesOff → reset(); STFT buffers cleared on steal |
| **Oracle** | 8 (param: Mono/Legato/Poly4/Poly8) | LRU (oldest startTime) + 5ms crossfade (capped at 50%) | Full — Legato: stochastic waveform evolution continues uninterrupted while pitch glides | Full ADSR on amplitude + stochastic level envelopes | allNotesOff → reset() |
| **Morph** | 16 (param: 1/2/4/8/16) | LRU (oldest startTime) + 5ms crossfade | **Fixed (Round 8)** — new morph_voiceMode param: Poly/Mono/Legato; legato detects wasActive and glides pitch | Full ADSR inline (Attack/Decay/Sustain/Release/Off); stuck-silent Sustain guard; sustain pedal (CC64) tracked | allNotesOff → reset(); sustain pedal release correctly drains held voices |
| **Oblique** | 8 (fixed) | LRU (oldest startTime) + 5ms crossfade | None — no voice mode param; glide exists but applies unconditionally to every new voice, not a legato detect | Full ADSR inline; exponential release; -60dB threshold deactivates voice | allNotesOff → reset() |
| **Ocelot** | 8 (fixed) | **Quietest-amplitude policy; Fixed (Round 8)** — noteOn now sets stealFadeGain before reinitializing voice | None — no voice mode param; always poly, always retriggered | Full ADSR via AmpEnvelope; gate(false) triggers Release; isActive() gates lifecycle | allNotesOff() calls noteOff() on all active voices (graceful release, not hard cut) |
| **Overworld** | 8 (fixed, hardware-motivated SNES limit) | **Fixed (Round 8)** — round-robin steal now sets stealFadeGain=1.0; 5ms ramp applied in Voice::process() | None — no voice mode, no glide wired through adapter | Full ADSR via Envelope class; noteOff → envelope.noteOff() | allNotesOff() in VoicePool; voice deactivates when envelope.isActive() returns false |
| **Osprey** | 8 (fixed) | LRU (oldest startTimestamp) + 5ms crossfade (capped at 50%) | None — glideCoefficient field exists but hardcoded to 1.0 (instant); no voice mode param | Full OspreyADSR with 0.0001f bias preventing stuck-zero envelope | allNotesOff → reset(); bias fix documented at line 106 |

---

## 2. Per-Engine Detail

### Snap (OddfeliX) — GOOD
- **Max voices:** 8, parameter-controlled (1/2/4/8 via bit-shift).
- **Stealing:** LRU with `startTime` counter; 5ms linear crossfade via `fadeOutLevel`.
- **Legato:** None — correct for a percussive engine. Every note-on is a fresh trigger.
- **Note-off:** Forces `envelopeLevel = min(current, 0.01f)` then lets exponential decay finish naturally. No hard cut.
- **Stuck notes:** `reset()` on allNotesOff. Envelope auto-deactivates voice at 0.0. Additional polyphony-limit guard forces voices beyond `maxPolyphony` to fade quickly.

### Orbital — GOOD
- **Max voices:** 6 (fixed — computationally justified: 64 partials × 6 = 384 simultaneous oscillators).
- **Stealing:** LRU with `startTime`; 5ms crossfade via `fadeOutLevel`.
- **Legato:** None — no voice mode parameter. Always polyphonic and retriggered.
- **Note-off:** Proper ADSR; noteOff triggers Release stage. Decay and release both clamped to 50ms minimum to prevent clicks.
- **Stuck notes:** allNotesOff sets all voices to `Off` + `active=false` immediately.

### Obsidian — EXCELLENT
- **Max voices:** 16.
- **Stealing:** LRU; 5ms crossfade capped at 50% on poly steal (fast steal).
- **Legato:** Full — voiceMode=0 Mono (retrigger), voiceMode=1 Legato (pitch glide, no retrigger when gate open), voiceMode=2 Poly16.
- **Note-off:** Full ADSR via custom envelope; triggers release on both amplitude and PD envelopes. `isFadingOut` guard prevents double-release.
- **Stuck notes:** allNotesOff → reset(); sustain pedal (CC64) tracked.

### Origami — EXCELLENT
- **Max voices:** 8 (parameter-controlled: Mono/Legato/Poly4/Poly8).
- **Stealing:** LRU; 5ms crossfade capped at 50%.
- **Legato:** Full — voiceMode=1 Legato detects `wasAlreadyActive` and slides pitch without retrigger; glide coefficient is exponential-approach (per-sample IIR).
- **Note-off:** Full ADSR on amplitude and fold envelopes.
- **Stuck notes:** allNotesOff → reset(); STFT buffers cleared on new poly voice (no spectral bleed).

### Oracle — EXCELLENT
- **Max voices:** 8 (parameter-controlled: Mono/Legato/Poly4/Poly8).
- **Stealing:** LRU; 5ms crossfade capped at 50%.
- **Legato:** Full — voiceMode=1 Legato: stochastic waveform evolution continues uninterrupted while pitch glides, preserving the GENDY engine's continuity signature.
- **Note-off:** Full ADSR on amplitude and stochastic envelopes.
- **Stuck notes:** allNotesOff → reset().

### Morph (OddOscar) — FIXED in Round 8 (was: WEAK — no legato)
**Before:** No voice mode parameter. Always polyphonic, always fully retriggered. Pad engine missing a natural feature.
**After:** `morph_voiceMode` (Poly/Mono/Legato) + `morph_glide` (0–2s) added. Legato mode detects `wasActive` and glides pitch via per-sample IIR without retriggering the Bloom envelope.

### Oblique — ACCEPTABLE (no legato)
- **Max voices:** 8 (fixed).
- **Stealing:** LRU with 5ms crossfade.
- **Legato:** Not implemented. Glide (`oblq_glide`) applies unconditionally — when a note is played, if glideTime > 0 and the previous frequency > 10Hz, it glides from there. This is "always-on portamento" rather than true legato. No `wasActive` check; no single-voice legato mode.
- **Note-off:** Full ADSR inline; exponential release; -60dB threshold deactivates voice.
- **Stuck notes:** allNotesOff → reset().

### Ocelot — FIXED in Round 8 (was: WEAK — no crossfade on steal)
**Before:** Quietest-amplitude stealing called `noteOff()` then `noteOn()` with no crossfade. The stolen voice entered release and the new note fired simultaneously — a hard click when all 8 voices were loud.
**After:** `OcelotVoice::noteOn()` now captures `lastAmplitude` into `stealFadeGain` before reinitializing. The `renderBlock()` loop decrements `stealFadeGain` over 5ms and applies it as a complementary multiplier on `envGain`, fading out the outgoing signal cleanly.

### Overworld — FIXED in Round 8 (was: WORST — round-robin steal, no crossfade)
**Before:** `VoicePool::findFreeOrSteal()` returned a pointer and `noteOn()` overwrote the voice with zero fade-out. Hard click on every steal at full polyphony.
**After:** `findFreeOrSteal()` sets `target->stealFadeGain = 1.0f` before returning. `Voice::process()` decrements `stealFadeGain` at the 5ms rate and multiplies output by `(1.0 - stealFadeGain)`, so the outgoing signal fades out while the new note's attack ramps up.

**Note:** The Overworld fix lands in the external repo `~/Documents/GitHub/XOverworld/src/engine/` because `OverworldEngine.h` in XOceanus is a thin adapter that includes `VoicePool.h` via `target_include_directories`.

### Osprey — ACCEPTABLE (no legato)
- **Max voices:** 8 (fixed).
- **Stealing:** LRU with 5ms crossfade (capped at 50%).
- **Legato:** Not implemented. `glideCoefficient` field exists in `OspreyVoice` and is wired into the render loop, but `initializeVoice()` always sets it to 1.0 (instant). No voice mode parameter.
- **Note-off:** Full OspreyADSR. The 0.0001f bias on exponential curves prevents the envelope from stalling at zero (explicit documentation comment at line 106 of OspreyEngine.h).
- **Stuck notes:** allNotesOff → reset(); bias fix guards against stuck-zero envelope.

---

## 3. The 3 Worst Implementations (Before Fixes)

### Rank 1 (Worst): Overworld
**Problems (before fix):**
1. Round-robin stealing with **zero crossfade** — stolen voices are hard-cut mid-sustain, causing audible clicks on voice-full polyphony.
2. No legato or glide wired through the XOceanus adapter.
3. Steal policy ignores voice amplitude — could steal a loud, ringing voice just because the counter points at it.

**Fixed:** `stealFadeGain` member added to `Voice`; `VoicePool::findFreeOrSteal()` sets it to 1.0 on steal; `Voice::process()` applies 5ms ramp. Files changed:
- `/Users/joshuacramblet/Documents/GitHub/XOverworld/src/engine/Voice.h`
- `/Users/joshuacramblet/Documents/GitHub/XOverworld/src/engine/VoicePool.h`

### Rank 2: Ocelot
**Problems (before fix):**
1. Quietest-amplitude steal called `noteOff()` then `noteOn()` on the same voice in the same call. The voice's ADSR entered Release, but was immediately reinitiated at Attack — a hard discontinuity producing a click.
2. No legato or voice mode selection.

**Fixed:** `OcelotVoice::noteOn()` captures the outgoing `lastAmplitude` into `stealFadeGain` before reinitializing. The `renderBlock()` loop decrements `stealFadeGain` and applies it as `envGain *= (1.0 - stealFadeGain)` over a 5ms window. Files changed:
- `/Users/joshuacramblet/Documents/GitHub/XO_OX-XOceanus/Source/Engines/Ocelot/OcelotVoice.h`

### Rank 3: Morph (OddOscar)
**Problems (before fix):**
1. No legato mode. Morph is the primary pad engine in the fleet; smooth melodic leads with no retrigger are a standard playability expectation.
2. No portamento/glide parameter.

**Fixed:** Added `morph_voiceMode` (Poly/Mono/Legato choice) and `morph_glide` (0–2s float) parameters. Added `currentFrequency`, `targetFrequency`, `glideCoefficient` to `MorphVoice`. Updated `noteOn()` to handle all three modes. Updated render loop to glide via `voice.currentFrequency` instead of computing from `noteNumber` per-sample (poly mode is semantically identical since `currentFrequency == targetFrequency` with `glideCoefficient=1.0`). Files changed:
- `/Users/joshuacramblet/Documents/GitHub/XO_OX-XOceanus/Source/Engines/Morph/MorphEngine.h`

---

## 4. Parameter IDs Added (Morph)

| Parameter ID | Type | Range | Default | Description |
|-------------|------|-------|---------|-------------|
| `morph_voiceMode` | Choice | Poly/Mono/Legato | Poly | Voice allocation mode |
| `morph_glide` | Float | 0–2s | 0.0s | Portamento time (Mono/Legato only) |

These parameter IDs are stable and frozen per XO_OX architecture rules.

---

## 5. Remaining Gaps

### Engines without legato (and where it would add value)

| Engine | Legato Missing | Priority | Notes |
|--------|---------------|----------|-------|
| Oblique | Yes — always-on portamento ≠ legato; needs voiceMode param | High | RTJ/Funk lead engine; legato is natural |
| Osprey | Yes — glideCoefficient field wired but never set below 1.0 | Medium | Small lift: add voiceMode + pass glideCoeff to initializeVoice |
| Ocelot | Yes — textural/ecosystem engine; legato less critical | Low | Less often played melodically |
| Overworld | Yes — not wired through adapter; standalone has groundwork | Low | External repo; requires adapter parameter layout change |

### Orbital — no voice mode parameter
Orbital has no `voiceMode` parameter. As a 64-partial additive engine, mono use is uncommon, but a Legato mode would enable smooth melodic lead lines. Low priority.

### Snap — no legato (intentional)
Snap is explicitly percussive (no sustain stage). No legato is expected or appropriate.

### Overworld adapter glide
The standalone XOverworld does not have a `ow_glide` pitch portamento parameter (it has `ow_eraPortaTime` for ERA position glide, which is different). Adding pitch portamento requires: (1) storing per-voice previous frequency in `Voice`, (2) adding `ow_glide` to the XOceanus parameter layout, (3) passing it through `applyParams`. Medium effort, low urgency.

### Ocelot steal policy — quietest vs. LRU
The quietest-amplitude policy is musically correct for a textural engine (preserves loud voices). With the crossfade fix, the click is resolved. No further change needed on the selection policy itself.

---

## 6. Summary

| Metric | Before Round 8 | After Round 8 | After Round 10F |
|--------|---------------|---------------|----------------|
| Engines with 5ms steal crossfade | 8/10 | 10/10 | 10/10 |
| Engines with allNotesOff MIDI handler | 10/10 | 10/10 (was already present in all) | 10/10 |
| Engines with legato/voice mode | 3/10 (Obsidian, Origami, Oracle) | 4/10 (+ Morph) | 6/10 (+ Oblique + Osprey) |
| Hard-cut steal (no crossfade) | 2 (Overworld, Ocelot) | 0 | 0 |
| Stuck note risk (missing allNotesOff) | 0 | 0 | 0 |
| New parameters added | — | 2 (morph_voiceMode, morph_glide) | 3 (oblq_voiceMode, osprey_voiceMode, osprey_glide) |

---

## 7. Round 10F Followup — Oblique and Osprey Legato (2026-03-14)

Both gaps identified in Section 5 (Remaining Gaps) during Round 9D have been resolved.

### Oblique — RESOLVED

**Root cause:** `noteOn()` always reset `envelopeStage = 0.0f` and `envelopeLevel = 0.0f` unconditionally. There was no `wasActive` check and no voice mode parameter. The `oblq_glide` portamento applied unconditionally to every new note regardless of whether the previous note was still held — "always-on portamento" rather than true legato.

**Fix applied:**
1. Added `bool wasLegatoActive = false` member to `ObliqueVoice` (struct, line ~638) to track legato state.
2. Added `oblq_voiceMode` parameter (`AudioParameterChoice`: Poly/Mono/Legato, default Poly index 0).
3. In `noteOn()`: if `voiceMode == 2` (Legato), scan voices for one that is `active && !releasing` (gate open). When found: update `noteNumber`, `velocity`, `targetFrequency`; snap `currentFrequency` immediately if `glideTime == 0` (no portamento artefact); set `wasLegatoActive = true`; return without allocating a new voice or retriggering the envelope/bounce. Falls through to normal poly allocation when no gate-open voice exists.
4. All existing Poly presets are unaffected — `oblq_voiceMode` defaults to 0 (Poly).

**Files changed:**
- `/Users/joshuacramblet/Documents/GitHub/XO_OX-XOceanus/Source/Engines/Oblique/ObliqueEngine.h`

**New parameters:**

| Parameter ID | Type | Range | Default | Description |
|-------------|------|-------|---------|-------------|
| `oblq_voiceMode` | Choice | Poly/Mono/Legato | Poly | Voice allocation mode |

---

### Osprey — RESOLVED

**Root cause:** `OspreyVoice` already had a `glideCoefficient` field wired correctly into the per-sample render loop (`currentGlideFrequency += (target - current) * glideCoefficient`), but `initializeVoice()` always hardcoded `glideCoefficient = 1.0f` (instant). There was no `osprey_voiceMode` parameter and no legato detection logic in `handleNoteOn()`.

**Fix applied:**
1. Added `osprey_voiceMode` parameter (Poly/Mono/Legato, default Poly index 0).
2. Added `osprey_glide` parameter (0–2 s, skewed range, default 0 s).
3. Both are loaded in the ParamSnapshot block at the top of `renderBlock()` and passed to `handleNoteOn()`.
4. In `handleNoteOn()`: if `voiceMode == 2` (Legato), scan for a gate-open voice (`active && !fadingOut && envelope not in Release or Idle`). When found: update `noteNumber`, `velocity`, `targetFrequency`; compute `glideCoefficient = 1 - exp(-1/(glideTime * sampleRate))` if `glideTime > 0` (IIR exponential approach), or snap `currentGlideFrequency = target` if glide is zero; return without re-initialising the voice or retriggering the envelope. Falls through to normal poly allocation when no gate-open voice exists.
5. In `initializeVoice()`: signature gains `float glideTimeSec = 0.0f`. When `glideTime > 0` and the voice has a valid previous frequency, `glideCoefficient` is computed from the IIR formula instead of being hardcoded to 1.0. When `glideTime == 0`, `currentGlideFrequency` is snapped to `frequency` and `glideCoefficient = 1.0f` (unchanged behaviour for all existing presets).

**Files changed:**
- `/Users/joshuacramblet/Documents/GitHub/XO_OX-XOceanus/Source/Engines/Osprey/OspreyEngine.h`

**New parameters:**

| Parameter ID | Type | Range | Default | Description |
|-------------|------|-------|---------|-------------|
| `osprey_voiceMode` | Choice | Poly/Mono/Legato | Poly | Voice allocation mode |
| `osprey_glide` | Float | 0–2 s | 0.0 s | Portamento glide time (Legato/Mono) |

---

### Updated Audit Table Rows

| Engine | Legato | Status |
|--------|--------|--------|
| **Oblique** | Full — `oblq_voiceMode` Poly/Mono/Legato; gate-open check (`active && !releasing`); pitch glide respects `oblq_glide`; bounce skipped on legato note | **RESOLVED Round 10F** |
| **Osprey** | Full — `osprey_voiceMode` Poly/Mono/Legato; gate-open check (`active && !fadingOut && envelope != Release/Idle`); `glideCoefficient` computed from `osprey_glide` via IIR formula; envelope not retriggered on legato note | **RESOLVED Round 10F** |

### Remaining Legato Gaps (Low Priority)

| Engine | Gap | Priority |
|--------|-----|----------|
| Ocelot | No voice mode param; always poly; legato low value for textural engine | Low |
| Overworld | Glide not wired through adapter (standalone has groundwork); requires adapter param layout change | Low |
| Orbital | No voice mode; 64-partial additive engine; mono lead use uncommon | Low |
