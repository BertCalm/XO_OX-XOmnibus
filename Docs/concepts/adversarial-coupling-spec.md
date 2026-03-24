# Adversarial Coupling — Design Specification

**Status:** Concept / Pre-implementation
**Author:** Claude Code (design session 2026-03-21)
**Context:** Chef Quad engines (XOto, XOctave, XOleg, XOtis) — and any engine pair
**Scope:** A new coupling paradigm where engines COMPETE rather than blend

---

## 1. Conceptual Foundation

Every existing coupling type in XOlokun (all 14, including KnotTopology) is fundamentally
cooperative. Source feeds destination. Both engines coexist. The relationship enhances each engine's
expression without taking anything away.

Adversarial coupling inverts that premise. Two engines share a finite pool of resources — frequency
space, voices, FX send — and the more active engine colonizes more of the pool. The quieter engine
is pushed into narrower territory. One chef dominates; the other retreats.

This is not ducking. `AmpToChoke` already ducks. Adversarial coupling is zero-sum across three
separate resource axes simultaneously, driven by a single continuous `competition_intensity` scalar
that can be automated, macro-mapped, or MIDI-CC controlled in real time.

---

## 2. Where This Lives in the Architecture

### 2.1 New Enum Value: `CouplingType::Adversarial`

Adversarial coupling is a 15th `CouplingType`, not a modifier to existing types.

Rationale for a new enum value rather than a flag on existing types:

- `CouplingRoute::type` is the dispatch key in `MegaCouplingMatrix::processBlock()`. Adding a flag
  would require reading two fields atomically on the audio thread or complicating the lock-free route
  list structure. A single enum value keeps dispatch O(1) and the route struct unchanged.
- Adversarial processing requires reading from BOTH engines (bidirectional amplitude comparison)
  before applying any modulation to either. The existing dispatch pattern calls `applyCouplingInput`
  on only the `destSlot` engine. Adversarial coupling needs a dedicated dispatch path — the same
  reason `KnotTopology` has its own `processKnotRoute()` method.
- A new enum value keeps `kNumCouplingTypes` accurate and future-proofs `couplingTypeName()`.

`CouplingTypes.h` addition (SDK header):

```cpp
Adversarial     ///< Zero-sum competition: engines fight for spectral space, voices, and FX
                ///< send. competition_intensity (encoded in amount field, 0.0–1.0) controls
                ///< ducking depth, voice reallocation aggressiveness, and FX crossfade steepness.
                ///< At amount=0.0 behavior is identical to no coupling (safe default).
                ///< Unlike all other types, both engines are simultaneously source AND destination.
```

`kNumCouplingTypes` → 15.

### 2.2 `competition_intensity` Encoding

Adversarial coupling reuses the existing `CouplingRoute::amount` field (float 0.0–1.0). No struct
changes are needed.

```
amount = competition_intensity
  0.0  → fully cooperative (zero-sum mechanisms are bypassed entirely; silent by default)
  0.5  → moderate competition (ducking active, voice stealing threshold crossed)
  1.0  → full battle (maximum ducking depth, aggressive voice stealing, hard FX crossfade)
```

This mirrors how `KnotTopology` encodes `linkingNumber` in `amount` — a single float carries all
necessary state within the existing `CouplingRoute` struct.

**Performance overlay compatibility:** Because `competition_intensity` is just `amount`, the
existing `cp_r*_amount` APVTS parameters from the Coupling Performance System (Phase A, committed
2026-03-21) can automate or macro-map adversarial intensity with no new parameters required.

### 2.3 Dispatch Path in `MegaCouplingMatrix::processBlock()`

A new `processAdversarialRoute()` private method, dispatched before the `isAudioRoute` branch:

```
if (route.type == CouplingType::KnotTopology)    → processKnotRoute()     [existing]
if (route.type == CouplingType::Adversarial)      → processAdversarialRoute()  [new]
if (isAudioRoute)                                 → audio path            [existing]
else                                              → control-rate path     [existing]
```

---

## 3. The Three Mechanisms

### 3.1 Mechanism 1: Spectral Ducking (Dynamic Multiband Sidechain)

**What it does:** The louder engine's broadband output pushes the quieter engine into narrower
frequency bands. Frequency space is zero-sum. One chef's tonal richness compresses the other's.

**Signal flow:**

```
[Slot A output] ──RMS detect──┐
                              ├─ compare amplitudes → determine dominant/submissive
[Slot B output] ──RMS detect──┘

dominant engine:    passes through unchanged
submissive engine: per-band gain reduction applied
```

**DSP approach — 3-band sidechain compression:**

```
Band 0: lo    (20 Hz –  500 Hz)
Band 1: mid   (500 Hz –  4 kHz)
Band 2: hi    (4 kHz – 20 kHz)

For each band:
  dominantLevel  = RMS of dominant engine's output in this band
  gain_reduction = 1.0 - (dominantLevel * competition_intensity * kDuckingScale)
  gain_reduction = clamp(gain_reduction, (1.0 - competition_intensity), 1.0)

Submissive engine's band output is multiplied by gain_reduction before final summing.
```

`kDuckingScale` is a tuning constant (~2.0 to 3.0 — the dominant engine needs to be loud enough to
cause meaningful ducking without the submissive engine disappearing at moderate playing).

**Frequency floor:** The submissive engine retains at least `(1.0 - competition_intensity)` of its
gain in each band. At `competition_intensity = 1.0`, it can be reduced to 0.0. At
`competition_intensity = 0.5`, it retains at least 50% in every band. This prevents total silence
and preserves the "ghost" of the submissive chef.

**Implementation note:** Per-band RMS detection on the audio thread requires a short (8–16 ms)
leaky integrator. The integrator state lives in the `processAdversarialRoute()` method as member
variables on `MegaCouplingMatrix`, not heap-allocated per block. Time constant should be
configurable but defaults to 10 ms — fast enough to track transients, slow enough not to pump on
every note attack.

**Control rate or audio rate?** Sidechain gain values are control-rate: the RMS envelope changes
slowly enough that updating every 32 samples (matching `kControlRateRatio`) is sonically transparent.
The actual gain application to the output buffer must be audio-rate (sample-by-sample multiplication)
to avoid zipper noise.

---

### 3.2 Mechanism 2: Voice Stealing (Activity-Based Reallocation)

**What it does:** The total polyphony budget of 8 voices is shared between two competing engines.
Normally each engine manages its own voice pool independently. With `competition_intensity > 0.5`,
the more active engine earns additional voice slots by "stealing" from the less active one.

**Signal flow:**

```
[Slot A].getActiveVoiceCount()  →  activityA
[Slot B].getActiveVoiceCount()  →  activityB

if competition_intensity > kVoiceStealThreshold (0.5):
    dominantSlot    = (activityA >= activityB) ? A : B
    submissiveSlot  = other

    voicesForDominant   = kBaseVoices + floor(stolen * competition_intensity)
    voicesForSubmissive = kTotalVoices - voicesForDominant
```

**The `voiceQuota` concept:**

Each engine already implements `getMaxVoices()` (returns its designed polyphony ceiling) and
`getActiveVoiceCount()` (returns live voice count). Adversarial coupling adds a soft quota that
sits between these: a `recommendedMaxVoices` hint that the engine is expected to honor when the
quota drops below its native maximum.

```
kTotalVoices    = 8   (shared pool)
kBaseVoices     = 4   (neither engine drops below this, regardless of intensity)
stolen          = up to kTotalVoices - (2 * kBaseVoices) = 0..4 additional voices

at competition_intensity = 0.5:  stolen = 0  (threshold just crossed, no action yet)
at competition_intensity = 0.75: stolen = 2  (dominant gets 6, submissive gets 2... but floor is 4)

Wait — floor is kBaseVoices = 4. So:
  dominant    = clamp(4 + floor((intensity - 0.5) * 2.0 * 4), 4, 8) = 4..8
  submissive  = 8 - dominant = 0..4 ... clamped to kBaseVoices floor:
  submissive  = max(8 - dominant, kBaseVoices)
  dominant    = 8 - submissive

At intensity = 1.0: dominant = 8, submissive = 4 (floor)
```

**Enforcement:** The quota is a soft hint passed via a new optional SynthEngine method
`setVoiceQuota(int maxVoices)`. Default implementation ignores it (backward compatible). Chef Quad
engines honor it by clamping new voice allocation. The Adversarial route calls `setVoiceQuota` on
both engines once per block (control-rate — no need for sample-accurate voice quota changes).

**`getActiveVoiceCount()` dependency:** This method already exists on `SynthEngine` (line 119 in
`SynthEngine.h`) with a default return of 0. Any engine used in an adversarial pair must override
it. Chef Quad engines will override this from day one. If both engines return 0 (stubs), no voice
stealing occurs — a safe default.

**Activity tie-breaking:** When `activityA == activityB`, no stealing occurs. The tie is broken only
on the next block where one engine has more voices active.

---

### 3.3 Mechanism 3: Effects Bus Competition (Shared FX Send Crossfade)

**What it does:** A shared reverb/delay send bus is divided between the two competing engines. The
louder engine earns more wet signal. The quieter engine gets proportionally less. Total FX send
sums to 1.0 — the spatial budget is conserved.

**Signal flow:**

```
[Slot A].getSampleForCoupling(ch0, i)  →  outA (last rendered audio)
[Slot B].getSampleForCoupling(ch0, i)  →  outB

rmsA = running RMS of outA
rmsB = running RMS of outB
total = rmsA + rmsB (if total < epsilon, sendA = sendB = 0.5)

rawSendA = rmsA / total   →  range [0.0, 1.0]
rawSendB = rmsB / total   →  range [0.0, 1.0]

// Blend toward equal-split by (1.0 - competition_intensity):
sendA = lerp(0.5, rawSendA, competition_intensity)
sendB = lerp(0.5, rawSendB, competition_intensity)
```

At `competition_intensity = 0.0`: sendA = sendB = 0.5 (equal FX access, no competition).
At `competition_intensity = 1.0`: sends track amplitude ratio exactly.

**Delivery to engines:** `sendA` and `sendB` are passed to each engine via `applyCouplingInput` with
the `CouplingType::Adversarial` tag. Each engine scales its FX return (reverb tail, delay line wet
output) by the received send value. An engine that doesn't handle `Adversarial` in its
`applyCouplingInput` switch falls through to `default: break` — silent failure (FX send stays at
whatever the engine's own parameter sets). This is acceptable: non-Chef engines can participate in
adversarial coupling without implementing this mechanism; only spectral ducking applies to them via
the output buffer multiplication in `processAdversarialRoute()`.

**Smoothing:** The send crossfade must be smoothed to prevent zipper noise. A one-pole smoother
with a 20ms time constant matches the `ParameterSmoother.h` standard from the shared DSP library.
The smoother runs at control rate (every `kControlRateRatio` samples).

---

## 4. Pseudocode — Full `processAdversarialRoute()`

```
processAdversarialRoute(source, dest, route, numSamples):

    intensity = route.amount   // competition_intensity, 0.0–1.0
    if intensity < 0.001:
        return                 // Fully cooperative — zero cost

    // --- STEP 1: Amplitude detection (control-rate, every kControlRateRatio samples) ---

    rmsA = computeRMS(source, numSamples)    // read from source's coupling cache (ch0)
    rmsB = computeRMS(dest,   numSamples)    // read from dest's coupling cache (ch0)

    dominant   = (rmsA >= rmsB) ? SOURCE : DEST
    submissive = (dominant == SOURCE) ? DEST : SOURCE

    // --- STEP 2: Spectral Ducking ---

    // Process 3 bands on the submissive engine's OUTPUT buffer
    // (MegaCouplingMatrix must hold references to both engines' output buffers,
    //  OR the engines expose their output buffer for writing post-render)
    // Pragmatic alternative: apply ducking by calling applyCouplingInput(Adversarial, ...)
    // on submissive engine with the gain reduction embedded in the buffer.

    for band in [LO, MID, HI]:
        dominantBandLevel  = getBandRMS(dominant,   band, numSamples)
        gain_reduction     = 1.0 - (dominantBandLevel * intensity * kDuckingScale)
        gain_reduction     = clamp(gain_reduction, 1.0 - intensity, 1.0)
        smoothedGain[band] = onePoleSmoother(gain_reduction, 10ms)

    // Encode per-band gains into a compact float array passed to submissive engine:
    duckingPayload[0] = smoothedGain[LO]
    duckingPayload[1] = smoothedGain[MID]
    duckingPayload[2] = smoothedGain[HI]
    duckingPayload[3] = intensity

    submissive->applyCouplingInput(CouplingType::Adversarial, intensity,
                                   duckingPayload, 4)

    // --- STEP 3: Voice Stealing ---

    if intensity > kVoiceStealThreshold (0.5):
        actA = source->getActiveVoiceCount()
        actB = dest->getActiveVoiceCount()
        dominantSlot = (actA >= actB) ? source : dest
        submissiveSlot = other

        extraVoices = floor((intensity - 0.5) * 2.0 * (kTotalVoices - 2 * kBaseVoices))
        dominantQuota   = clamp(kBaseVoices + extraVoices, kBaseVoices, kTotalVoices)
        submissiveQuota = max(kTotalVoices - dominantQuota, kBaseVoices)

        dominantSlot->setVoiceQuota(dominantQuota)
        submissiveSlot->setVoiceQuota(submissiveQuota)
    else:
        // Reset to full polyphony for both
        source->setVoiceQuota(kTotalVoices)
        dest->setVoiceQuota(kTotalVoices)

    // --- STEP 4: FX Bus Competition ---

    total = rmsA + rmsB
    if total > kEpsilon:
        rawSendA = rmsA / total
        rawSendB = rmsB / total
    else:
        rawSendA = rawSendB = 0.5

    sendA = lerp(0.5, rawSendA, intensity)
    sendB = lerp(0.5, rawSendB, intensity)

    fxPayload[0] = smoothFXSend(sendA, 20ms)
    fxPayload[1] = smoothFXSend(sendB, 20ms)

    // Both engines receive their FX allocation:
    source->applyCouplingInput(CouplingType::Adversarial, sendA, fxPayload, 2)
    dest->applyCouplingInput(CouplingType::Adversarial, sendB, fxPayload, 2)
```

**Note on the dual `applyCouplingInput` calls:** Because both engines act as source and destination
(like `KnotTopology`), `processAdversarialRoute()` calls `applyCouplingInput` on both — but with
semantically different payloads (ducking for the submissive, FX send for both). An engine's
`applyCouplingInput` implementation reads the payload context from buffer position:
`sourceBuffer[3]` (intensity tag) distinguishes the ducking call from the FX call. This is a
brittle but CPU-zero encoding. An alternative is two separate `CouplingType` subtags, but that
requires enum changes. The payload-position convention is documented per-engine and enforced by the
Chef Quad engines.

---

## 5. Should Adversarial Coupling Be Chef-Exclusive?

**Short answer: No. It should be universal, with Chef Quad engines as the first-class citizens.**

### Arguments for Chef-Exclusive

- The Chef Quad engines are designed from the ground up for competition. They will fully implement
  `setVoiceQuota()`, the FX send branch in `applyCouplingInput`, and the spectral ducking response.
- Non-Chef engines with STUB `applyCouplingInput` would silently ignore the ducking payload —
  partial behavior with no user feedback.
- The concept metaphor (regional chefs competing) is Chef-specific.

### Arguments for Universal

- Spectral ducking (Mechanism 1) works on any engine's output buffer without engine cooperation.
  `processAdversarialRoute()` applies the per-band gain multiplication directly to the output
  coupling cache — it does not require `applyCouplingInput` support from the engine.
- `AmpToChoke` (existing cooperative ducking) is available to all engines. Adversarial ducking
  is a richer version of the same idea. Restricting it to Chef engines creates an arbitrary API
  asymmetry.
- A producer might want ONSET (drums) vs. OBLONG (bass) in competition — a musically valid use case
  entirely outside the Chef Quad.
- `getActiveVoiceCount()` is already on `SynthEngine` with a default return of 0. Voice stealing
  on a non-overriding engine simply has no effect — a safe and silent degradation.

### Recommendation

Mark Adversarial coupling as **universal** in the enum and dispatch path, but document that full
three-mechanism behavior requires:

1. `applyCouplingInput` handling `CouplingType::Adversarial` (for FX bus and ducking response).
2. `getActiveVoiceCount()` override (for voice stealing).
3. `setVoiceQuota()` override (for voice stealing enforcement, a new optional method).

Chef Quad engines implement all three. Existing engines get Mechanism 1 (spectral ducking via output
buffer modification) for free, with Mechanisms 2 and 3 silently inactive. This is the same
graceful degradation pattern used by `AudioToBuffer` (only OPAL handles it; others return early
from the downcast).

Document the tiers in an engine compatibility table:

| Adversarial Tier | Engine Requirements | Mechanisms Active |
|-----------------|--------------------|--------------------|
| Tier 0 — None | STUB applyCouplingInput + getActiveVoiceCount returns 0 | None |
| Tier 1 — Spectral | Any engine (output buffer modified externally) | Mechanism 1 only |
| Tier 2 — Spectral + Voice | getActiveVoiceCount + setVoiceQuota overridden | 1 + 2 |
| Tier 3 — Full Battle | All of the above + Adversarial in applyCouplingInput switch | 1 + 2 + 3 |

Chef Quad engines: Tier 3 from initial build.
Existing engines: Tier 0–1 depending on output cache quality.

---

## 6. Integration with Existing Coupling Infrastructure

### 6.1 Existing Crossfader (`CouplingCrossfader.h`)

The `CouplingCrossfader` (built in Phase A of the Performance System) interpolates smoothly
between coupling types when a performance route changes type mid-session. It handles the
`cp_r*_type` APVTS parameter.

Adversarial coupling integrates cleanly: `CouplingCrossfader` sees it as another enum value
and will crossfade into/out of `CouplingType::Adversarial` the same way it handles any type
switch. The crossfade duration (50ms default, matching engine hot-swap) prevents abrupt transitions
when toggling between a cooperative type and Adversarial.

Edge case: crossfading from `AmpToChoke` (cooperative ducking) to `Adversarial` will create a brief
period where both ducking mechanisms are partially active. This is musically acceptable — the
transition will sound like competition gradually intensifying. No special handling required.

### 6.2 Performance System Macros

The Coupling Performance System (Phase A) extended `MacroSystem.h` to target `cp_r*_amount`
parameters. Since `competition_intensity` IS `amount` for adversarial routes, the existing macro
wiring works with no changes:

```
Macro 3 (COUPLING) → cp_r1_amount → competition_intensity for an adversarial route
```

This means a performer can sweep `competition_intensity` from a knob or expression pedal,
gradually escalating a cooperative performance into full battle and back — a live dramatic arc.

### 6.3 The Standard Crossfader (`amount` field)

The `CouplingRoute::amount` crossfader path in `processBlock()` (`if (route.amount < 0.001f) continue`)
already provides a zero-CPU bypass when `competition_intensity = 0.0`. Adversarial coupling respects
this — the `if intensity < 0.001: return` guard at the top of `processAdversarialRoute()` is
redundant but kept for clarity and safety in case the outer guard is refactored.

### 6.4 Normalled Routes and User Override

Adversarial coupling can be normalled (pre-wired as a default route) for Chef Quad slots, or added
by the user. The `isNormalled` flag in `CouplingRoute` is unchanged. A Chef Quad preset might
normall an `Adversarial` route at `amount = 0.3` (mild competition, engines still coexist) and let
the performer escalate via macro or MIDI CC.

### 6.5 KnotTopology Interaction

`KnotTopology` and `Adversarial` should not be combined on the same engine pair simultaneously.
KnotTopology creates mutual entanglement — each engine's output modulates the other's filter. If
Adversarial is simultaneously active on the same pair, spectral ducking will fight the entanglement,
creating unpredictable amplitude instability.

Recommendation: the UI should warn (or prevent) adding both `KnotTopology` and `Adversarial`
routes between the same source/dest pair. The audio-thread behavior itself won't crash — both routes
are processed independently — but the sonic result is chaotic in an uncontrolled way.

### 6.6 `AudioToBuffer` (OPAL Streaming)

`AudioToBuffer` pushes audio from a source engine into OPAL's grain buffer. Adversarial coupling on
the same source engine would simultaneously duck that engine's output (reducing grain input level)
while the granulator continues reading from the ring buffer. This interaction is musically
interesting — the grain cloud from a submissive engine plays back at full grain volume even as the
engine itself is being spectrally compressed in real time. Not a bug; worth noting as a creative
feature in the coupling cookbook.

---

## 7. Parameter Design Summary

| Parameter | Location | Range | Default | Notes |
|-----------|----------|-------|---------|-------|
| `competition_intensity` | `CouplingRoute::amount` | 0.0–1.0 | 0.0 | Encodes all adversarial behavior depth |
| kDuckingScale | Compile-time constant | 2.0–3.0 | 2.5 | Tuning constant for ducking sensitivity |
| kVoiceStealThreshold | Compile-time constant | — | 0.5 | Intensity at which voice stealing activates |
| kBaseVoices | Compile-time constant | — | 4 | Minimum voices guaranteed to each engine |
| kTotalVoices | Compile-time constant | — | 8 | Shared polyphony pool |
| Ducking smoother TC | Compile-time constant | — | 10ms | One-pole time constant for gain envelope |
| FX send smoother TC | Compile-time constant | — | 20ms | One-pole time constant for send crossfade |
| kEpsilon | Compile-time constant | — | 1e-6f | Silence floor for RMS ratio computation |

No new APVTS parameters are required. Adversarial coupling is fully controlled by the existing
`cp_r*_amount` performance overlay parameters.

No new `CouplingRoute` struct fields are required. The existing `amount` field encodes
`competition_intensity`.

---

## 8. Preset Design Implications

### Mood fit

Adversarial coupling naturally belongs in the **Flux** mood (unstable, evolving, unpredictable) and
**Prism** mood (spectral competition produces refracted, fragmented timbres). It could also appear
in **Foundation** if used subtly (intensity 0.2–0.3) to create natural spectral separation between
two engines without overt competition.

### Chef Quad preset conventions

Chef Quad presets using Adversarial coupling should follow these naming conventions:

- `{ChefA} vs {ChefB}` — head-to-head matchups (e.g., "Oto vs Otis")
- `{Mood} Standoff` — e.g., "Tropic Standoff", "Cathedral Standoff"
- Avoid "battle" in preset names — it implies violence rather than culinary competition

### Macro assignment for adversarial presets

| Macro | Assignment | Notes |
|-------|-----------|-------|
| M1 CHARACTER | Dominant engine character | Controls which chef "speaks" most |
| M2 MOVEMENT | Spectral drift / LFO depth of submissive engine | The retreat has texture |
| M3 COUPLING | `competition_intensity` | The heat dial — sweep from blend to battle |
| M4 SPACE | Shared reverb room size | What both chefs are competing inside |

M3 as the competition dial gives performers an obvious narrative arc: start collaborative, escalate
to full competition, pull back. One knob tells the whole story.

---

## 9. Open Questions for V1 Decision

1. **Output buffer write access:** `processAdversarialRoute()` needs to apply per-band gain
   multiplication to the submissive engine's rendered output buffer. The current API exposes output
   via `getSampleForCoupling()` (read-only, cached). Writing back into the engine's buffer requires
   either a new `applyOutputGain()` interface method, or routing the ducking through
   `applyCouplingInput` (which indirectly modifies future rendered samples, not the current buffer).
   **Decision needed:** Direct output modification (new interface method) vs. predictive modulation
   via `applyCouplingInput` (one-block lag, simpler API).

2. **Three-band implementation cost:** True multiband ducking requires three pairs of BiquadFilters
   (or linkwitz-riley crossovers) running per adversarial route on the audio thread. At 48 kHz,
   three filter states × 2 engines × N adversarial routes adds CPU. For the Chef Quad (typically 2
   engines in competition), this is 12 filter states per block — acceptable. For 4 engines
   simultaneously adversarial (6 routes possible), this is 36 filter states. Consider a simpler
   broadband RMS comparison for V1, with multiband as a V1.1 enhancement.
   **Decision needed:** Broadband ducking V1 → multiband V1.1, or multiband from day one?

3. **`setVoiceQuota()` as interface method:** Adding `setVoiceQuota(int)` to `SynthEngine` with a
   default no-op implementation is backward compatible (all 44 existing engines get the no-op for
   free). But it changes the public interface. If SDK third-party engines are distributed before
   adversarial coupling ships, they may not override it. Document as optional from the start.
   **Decision needed:** Add to `SynthEngine.h` interface now or defer to when Chef Quad is built?

4. **Asymmetric adversarial routes:** Should a CouplingRoute from A→B and a separate one from B→A
   both be permitted as `Adversarial` type? This would create an accidental double-application of
   spectral ducking. The dispatch path should detect mutual adversarial routes on the same pair and
   merge them into a single `processAdversarialRoute()` call.
   **Decision needed:** Enforce single adversarial route per pair (UI-level prevention), or handle
   dual routes safely in the dispatch (detect and merge)?

---

## 10. Summary: Why This Is Different From What Exists

| Feature | `AmpToChoke` (existing) | `Adversarial` (proposed) |
|---------|------------------------|--------------------------|
| Mechanism | Amplitude of A → duck B | Amplitude comparison → zero-sum competition |
| Directionality | One-way (A chokes B) | Bidirectional (louder engine always wins) |
| Spectral | Broadband gain reduction | Three-band dynamic compression |
| Voice allocation | No effect | Redistributes voice pool |
| FX access | No effect | Redistributes shared FX send |
| Who dominates | Always A (static) | Dynamic — changes as performance evolves |
| Cooperative fallback | No (it always chokes) | Yes — at intensity=0, both engines coexist fully |
| Performance modulation | Via separate AmpToChoke amount | Via single `competition_intensity` (M3 macro) |

Adversarial coupling is the first coupling type in XOlokun designed for the scenario where engines
are narrative opponents rather than collaborative voices. It gives producers a tool for tension —
not just texture.

---

*Docs/concepts/adversarial-coupling-spec.md — XOlokun by XO_OX Designs*
*Design only — no implementation. See open questions in Section 9 before writing code.*
