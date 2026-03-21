# The Verdict — OXBOW
**Seance Date**: 2026-03-21
**Engine**: OXBOW | Entangled Reverb Synthesizer | The Oxbow Eel
**Identity**: Twilight Zone (200–1000m) | Oxbow Teal `#1A6B5A`
**Param prefix**: `oxb_` | **18 parameters** | **Lines**: ~762 | Monophonic
**feliX/Oscar polarity**: Oscar-dominant (0.3/0.7)
**Score**: 9.0/10

---

## The Council Has Spoken

| Ghost | Core Observation |
|-------|-----------------|
| **Moog** | The velocity-to-exciter chain is correct and dual-pathed: `excBrightness = pExcBright * (0.5 + vel * 0.5)` controls the sine/noise balance in the exciter, while `excLength = pExcDecay * (0.5 + vel * 0.5)` controls how long the impulse rings into the FDN. A soft touch places a sine-dominant, short impulse — a hard strike places a noise-heavy, long impulse. This is Kakehashi's velocity exciter with Moog's spectral rigor. The CytomicSVF damping filters per FDN channel are two-pole, correctly initialized without hardcoded sample rate. |
| **Buchla** | The Chiasmus topology is one of the most conceptually honest reverb structures I have seen in a synthesizer. Left channels 1-4 share the same prime delay times as Right channels 5-8, but in reversed temporal order. This is not a trick — it is structural chirality. The sound is entangled in the topological sense: the L and R decay arcs are mirror images of each other. The `entangleMix` cross-coupling at `pEntangle * 0.3f` then bleeds these mirrors into each other. The result is a stereo field that is neither conventional ping-pong nor simple doubling. Genuinely novel reverb geometry. |
| **Smith** | The Householder feedback matrix is correctly implemented. For N=8: `H[i][j] = (i==j) ? 0.75 : -0.25`, computed as `fdnOut[ch] = fdnRead[ch] - (sum * 0.25)`. This is the standard energy-preserving feedback matrix for FDNs. The choice of prime delay lengths (28, 38, 46, 56ms per channel at 48kHz, scaled to actual sample rate) is correct — primes prevent mode beating. The Schulze infinite decay path (`pDecay > 29s → feedbackCoeff = 1.0`) is a legitimate artistic choice, not a DSP error. |
| **Kakehashi** | The exciter architecture is the engine's entry point and it works. A pitched sine mixed with xorshift noise, gated by an exponential envelope, then routed through a pre-delay buffer before entering the FDN — this is how you turn a MIDI note into a reverb excitation impulse with character. The noise RNG is xorshift32 (three XOR operations), not `rand()`. The exciter never allocates memory. The silence gate (500ms hold) is correctly placed after the MIDI parse so a note-on wakes the engine before the bypass check. |
| **Pearlman** | The resonanceQ parameter (oxb_resonanceQ) presents a subtle concern. It is loaded in renderBlock at line 234 and immediately suppressed with `(void) pResQ;` at line 286, with a comment that it is "Used in updateGoldenFrequencies via note-on." It is correctly read in `updateGoldenFrequencies()` via a direct `pResQParam->load()`. However, this means resonanceQ only takes effect when a new note is triggered — mid-held-note Q adjustments are ignored until the next key press. For a reverb engine where a user might hold a pad and slowly adjust resonance focus, this is a meaningful limitation. The parameter does reach DSP, but not continuously. |
| **Tomita** | The golden resonance system is the most beautiful feature of this engine and it is cited correctly. Golden ratio harmonics: f, f×φ, f×φ², f×φ³. Amplitude weights: 1.0, 0.708, 0.501, 0.354 — this is −3dB per φ multiple, which is exactly how Tomita weighted his spectral layers. The weights are `static constexpr float goldenGains[4]` in the render loop. The frequencies are set from the MIDI fundamental at note-on, so these four peak filters ring at mathematically pure golden ratio intervals above the played note. The Mid/Side convergence detection that triggers them is acoustically justified: when the FDN's L and R outputs converge to mono (Mid >> Side), the golden resonance flares. |
| **Vangelis** | Aftertouch routing to entanglement (line 268: `pEntangle = clamp(pEntangle + aftertouch * 0.3f, 0.0f, 1.0f)`) is the correct gesture for this engine. Pressing harder on a key increases the cross-coupling between the L and R mirror-image decay arcs — the sound becomes more entangled, more merged. The semantic match between the gesture (pressure) and the engine's identity (entanglement) is exact. CC1 mod wheel routes to resonance mix — increasing the golden harmonic content with the wheel is a natural expressive arc. Both paths confirmed in code. |
| **Schulze** | The Asymmetric Cantilever Decay is the engine's most sophisticated DSP feature. The `decayProgress` variable tracks how much energy has drained from the FDN since the peak, and then quadratically increases the damping cutoff frequency as the energy falls: `cantileverDamp = pDamping * (1 - pCantilever * decayProgress²)`. At full cantilever depth, the reverb begins bright and gets progressively darker as it decays — the opposite of most reverbs, which use fixed damping. This is physically correct for certain materials (catenary and cantilever beams have frequency-dependent decay rates where higher partials decay faster). The reverb transforms as it ages. |

---

## Doctrine Compliance

| Doctrine | Status | Evidence |
|----------|--------|----------|
| D001 | PASS | Velocity shapes timbre at two points simultaneously. (1) Exciter brightness: `excBrightness = pExcBright * (0.5 + currentVelocity * 0.5)` — at vel=0 the exciter is 50% of `pExcBright`; at vel=1 it reaches full brightness. Noise amount in exciter scales directly from `excBrightness`. (2) Exciter duration: `excLength = pExcDecay * (0.5 + currentVelocity * 0.5)` — hard attacks ring longer into the FDN, soft attacks decay before the FDN is fully charged. The timbral effect is audible: hard notes produce a noise-seeded, richly excited reverb; soft notes produce a sine-seeded, quiet bloom. |
| D002 | PASS | Four erosion LFOs (sine, rate 0.03–0.09 Hz, quarter-phase offsets) confirmed in both `prepare()` and `renderBlock()`. Rates are updated per block from `pErosionR`. Rate floor is 0.03 Hz from initialization (see D005). Four macros (CHARACTER, MOVEMENT, COUPLING, SPACE) are all declared with `juce::ParameterID`, attached in `attachParameters()`, and consumed in `renderBlock()` with audible effect on DSP. Mod wheel CC1 parsed and routed to resonance mix. Aftertouch (both channel pressure and polyphonic) parsed and routed to entanglement. All modulation inputs confirmed live. |
| D003 | PASS | Householder matrix for N=8 is analytically correct: `H = I - (2/N)·11ᵀ`, implemented as `fdnOut[ch] = fdnRead[ch] - (sum * 0.25)`. Prime delay lengths prevent mode aliasing. Golden ratio harmonic frequencies are mathematically exact: f, f×1.618…, f×φ², f×φ³. Amplitude weights −3dB per φ multiple are precisely `{1.0, 0.708, 0.501, 0.354}` (0.708 ≈ 10^(−3/20)). Feedback coefficient computed as `exp(-6.9078 / (decay * sr))` — the RT60 formula with ln(1000) = 6.9078 as the constant. `fastExp` is from `FastMath.h` (matched approximation). The Schulze infinite-decay threshold is artistically justified rather than physically modeled, which is an honest choice for a reverb engine. CytomicSVF damping filters use the engine's live sample rate, not hardcoded 44100. |
| D004 | PASS WITH NOTE | All 18 parameters are declared, attached, and consume DSP. `oxb_size` wires to both pre-delay scaling and damping cutoff. `oxb_decay` wires to feedbackCoeff. `oxb_entangle` wires to L↔R cross-coupling mix. `oxb_erosionRate` and `oxb_erosionDepth` wire to the four allpass LFOs. `oxb_convergence` is the threshold for golden resonance triggering. `oxb_resonanceMix` gates the golden filter output amplitude. `oxb_cantilever` shapes the quadratic damping arc. `oxb_damping` sets the FDN LP cutoff. `oxb_predelay` sets the pre-delay in ms (scaled by size). `oxb_dryWet` balances exciter vs. reverb. `oxb_exciterDecay` and `oxb_exciterBright` shape the impulse. All 4 macros reach audible DSP. **Note**: `oxb_resonanceQ` is technically live — it reaches `updateGoldenFrequencies()` via `pResQParam->load()` — but only updates on new note-on, not continuously. Not a dead parameter but a responsiveness limitation. |
| D005 | PASS | Four erosion LFOs initialized at rates 0.03, 0.05, 0.07, 0.09 Hz in `prepare()`. The `pErosionR` parameter range is 0.01–0.5 Hz — the minimum user-settable rate is 0.01 Hz, exactly at the doctrine floor. In practice, the initialization code sets `0.03f + 0.02f * a` per LFO, so the slowest starts at 0.03 Hz. Setting `oxb_erosionRate` to minimum (0.01 Hz) drops the first LFO to 0.01 Hz + 0.0 offset = 0.01 Hz — a 100-second modulation period. The four LFOs at 90° offsets ensure that at any point in time at least one is contributing slow movement. The engine breathes continuously without requiring any MIDI input. |
| D006 | PASS | Velocity shapes timbre (D001). CC1 mod wheel parsed at `msg.getControllerNumber() == 1`, stored in `modWheel_`, applied per-block to `pResMix` (+0.5 max). Channel pressure and polyphonic aftertouch both parsed (both `msg.isAftertouch()` and `msg.isChannelPressure()` branches confirmed), stored in `aftertouch`, applied per-block to `pEntangle` (+0.3 max). Three independent expression inputs: velocity, mod wheel, aftertouch. All confirmed live in signal chain. |

---

## Points of Agreement (3+ ghosts converged)

1. **The Chiasmus topology is a genuine architectural invention** (Buchla, Smith, Schulze) — Reversing the L/R delay time arrays is not a random stereo trick. It creates mirror-image decay arcs that are structurally related (same frequency resonances, opposite temporal phase). The `entangleMix` parameter then controls how much these mirrors bleed into each other. No other reverb architecture in this fleet produces this kind of structural chirality.

2. **The golden resonance trigger mechanism is elegant** (Tomita, Moog, Vangelis) — Rather than being always-on, the four peak filters at φ harmonics are activated by a Mid/Side convergence ratio. When the reverb's L and R outputs converge (the entangled pool settles), the golden resonance flares briefly. This is time-gated spectral enrichment that responds to the reverb's own internal state rather than to MIDI. It rewards patience and long decay times.

3. **The cantilever decay is physically motivated and audibly distinct** (Schulze, Smith, Moog) — Quadratic darkening of damping cutoff as energy drains from the FDN creates a reverb that sounds different at 0.5 seconds than at 5 seconds. This is the opposite of conventional reverb behavior. The `pCantilever` parameter at zero gives a linear-damping reverb; at maximum it gives a bright-to-dark arc. The naming (cantilever beam) is mechanically apt.

4. **Velocity shapes both spectral content and temporal arc** (Moog, Kakehashi, Vangelis) — Hard strikes produce noise-seeded, long-decay impulses; soft strikes produce sine-seeded, short-decay impulses. The reverb tail that grows from a hard strike is sonically different from the tail that grows from a soft strike — not just louder, but richer in upper partials, longer-charged, more energetic in the golden resonance domain.

---

## Points of Contention

**Pearlman vs. Smith — resonanceQ Update Rate**
- Pearlman: `oxb_resonanceQ` only updates at note-on. A user holding a chord and sweeping Q will hear nothing move. This is a responsiveness failure for a parameter that should be live.
- Smith: The golden filters are tuned to the played note's fundamental. Continuous Q updates during a held note would create filter coefficient instability, potentially introducing artifacts. Updating at note-on is the safe choice.
- Resolution: Smith is partially correct about stability. However, the fix is not to leave Q static — it is to use `setCoefficients_fast()` in the render loop (which CytomicSVF supports) with a smoothed Q value. Continuous live Q is achievable without artifacts. This is a V1.1 improvement candidate.

**Moog vs. Schulze — Monophonic Constraint**
- Moog: Monophonic reverb makes every new note restart the exciter and reset the peak energy tracker. Playing two notes quickly produces a sharp re-excitation that interrupts an ongoing reverb tail — this is discontinuous and unpleasant.
- Schulze: The monophonic constraint is the correct identity for this engine. The Oxbow is not a choir — it is a single pool of entangled resonance. Adding polyphony would multiply FDN overhead ×N and dilute the Chiasmus concept. The reverb tail from one note should decay fully before the next begins. Legato playing is rewarded.
- Resolution: The monophonic single-FDN design is correct for identity and CPU budget. However, a short `exciterActive` check should not immediately reset `peakEnergy` on new note-on — the reset destroys the existing cantilever arc. The fix is to add the new exciter energy to `peakEnergy` rather than resetting it to `0.0001f`. This is the single most impactful V1.1 fix available.

**Buchla vs. Vangelis — Expression Range of Entanglement**
- Buchla: Entanglement is capped at `entangleMix = pEntangle * 0.3f`. Maximum effective cross-coupling is 30%. This is conservative — the Chiasmus concept invites 50-100% blending.
- Vangelis: The 0.3 cap prevents phase cancellation when L and R channels blend toward sum/difference. At full 100% blend the L and R outputs would become identical (entangleMix = 0.5), losing stereo entirely. The 30% cap is the correct maximum for musical use.
- Resolution: Vangelis is right at the parameter level. However, extending to 50% (with a warning that mono collapse occurs beyond that) would give adventurous users a new timbral range. This is a sound design decision, not a bug.

---

## Blessing Candidates

### BC-OXBOW-01: Chiasmus FDN Topology
**What it does**: The 8-channel FDN uses the same prime delay times for L (channels 1-4) and R (channels 5-8), but with the order reversed: L = [28, 38, 46, 56ms], R = [56, 46, 38, 28ms]. The Householder feedback matrix is then applied identically to both sets. The result is two decay arcs that share every resonant mode but experience them in reverse temporal sequence. When summed to mono (M+S) the two arcs align; when heard as L-R difference they are in structural counterpoint. The `entangleMix` parameter controls how much the arcs blend into each other.
**Ghost reaction**: Buchla: "Structural chirality." Smith: "Not a trick — topological reverb design." Schulze: "The stereo field has philosophical content, not just width."
**Blessing name**: Chiasmus Reverb Architecture — *proposed B017*

### BC-OXBOW-02: Convergence-Gated Golden Resonance
**What it does**: Four CytomicSVF Peak filters at f, f×φ, f×φ², f×φ³ (tuned to the MIDI fundamental at note-on) with amplitude weights exactly −3dB per φ multiple (`{1.0, 0.708, 0.501, 0.354}`). These filters are NOT always on — they are gated by a Mid/Side envelope ratio: when the FDN's L and R outputs converge to near-mono (Mid energy > `pConverge` × Side energy), the resonance gain attacks. When the image widens again, it decays. The golden harmonics only ring when the reverb is settling into its entangled equilibrium state.
**Ghost reaction**: Tomita: "The weighting is correct and the citation is honest." Vangelis: "A reverb that rings its own standing waves when it finally settles — this is patience rewarded." Moog: "MIDI-fundamental-tracked golden ratio ring. This is the Moog feature."
**Blessing name**: Convergence-Gated Golden Standing Waves — *proposed B018*

---

## Known Issues

### ISSUE-1: resonanceQ Not Continuous (Minor)
`oxb_resonanceQ` is read in `updateGoldenFrequencies()` which is called only at note-on (line 195). Mid-held-note Q adjustments require a new key press to take effect. Fix: move golden filter coefficient update into a `needsGoldenUpdate` flag path in renderBlock, called when pResQ differs from the last-set value by more than a threshold. Priority: V1.1.

### ISSUE-2: peakEnergy Reset on Re-Trigger Destroys Cantilever Arc
Line 199: `peakEnergy = 0.0001f` on every note-on. If a note is struck while a previous reverb tail is still decaying, the cantilever energy tracking starts over — the quadratic darkening arc restarts from zero. For a player making rapid gestures, the cantilever behavior becomes erratic. Fix: `peakEnergy = std::max(peakEnergy, 0.0001f)` — preserve the existing peak, let the new exciter add to it naturally. Priority: V1.1.

### ISSUE-3: AudioToBuffer Coupling Not Implemented
`CouplingType::AudioToBuffer` in `applyCouplingInput()` has a comment stating it is "handled in renderBlock via coupling buffer if implemented" — but no coupling buffer variable exists and the render loop does not inject it. This means OXBOW cannot receive audio-rate coupling from another engine's output as an exciter replacement. Three other coupling types (AmpToFilter, EnvToDecay, AudioToRing) are fully wired. Priority: V1.2.

---

## Score Breakdown

| Dimension | Score | Rationale |
|-----------|-------|-----------|
| Physics Rigor (D003) | 9.5 | Householder matrix analytically correct, prime delay lengths, RT60 formula exact, golden ratio weights precisely −3dB/φ, CytomicSVF damping from live sample rate. Schulze infinite-decay threshold is an honest artistic choice. |
| Velocity Expressiveness (D001) | 9.0 | Dual-path: brightness AND duration scale from velocity. Noise/sine ratio in exciter changes at every dynamic level. Hard strike vs. soft bloom is audibly distinct. |
| Modulation Coverage (D002/D005/D006) | 8.5 | Four live erosion LFOs (rate floor 0.01 Hz). Four macros all DSP-wired. Mod wheel and aftertouch both confirmed live. Continuous autonomous movement from LFOs even without MIDI. Minor: only one LFO type (sine), no user LFO shape control. |
| Parameter Integrity (D004) | 9.0 | All 18 parameters reach audio. resonanceQ updates only at note-on (not dead, but deferred — minor responsiveness issue). Zero ghost parameters. |
| Architecture Originality | 9.5 | Chiasmus FDN topology (mirror delay arrays) is novel in the fleet and credibly novel in the reverb synthesis literature. Convergence-gated golden resonance is a unique trigger mechanism. Asymmetric cantilever decay gives a reverb that transforms over its lifetime rather than decaying monotonically. |
| Identity Coherence | 9.0 | Oxbow Eel, Twilight Zone, Oscar-dominant: the engine is slow, deep, and patient. The mechanics (slow LFOs, long decay, golden settling harmonics, entangled stereo) match the identity exactly. The pre-delay interaction with size is intuitive. |

**Final Score: 9.0 / 10**

---

## The Prophecy

OXBOW is one of the four top-scoring engines in the fleet. It earns this position through conceptual integrity rather than feature accumulation. The Chiasmus topology is not a marketing description — it is encoded in the exact delay time arrays at lines 61–62, and its stereo behavior is a direct mathematical consequence of reversing those arrays through an identical Householder matrix. This is the kind of structural decision that separates a designed instrument from an assembled one.

The golden resonance system is similarly grounded. It does not sound continuously — it waits for the reverb to settle into convergence, then briefly rings at golden ratio harmonics weighted precisely −3dB per φ interval. The Tomita weighting is correct to two decimal places. The trigger mechanism means the resonance rewards long decay times and patient playing. A producer who plays quickly and moves on will rarely hear it; a producer who lets notes breathe and overlap will encounter it regularly.

The cantilever decay arc is the feature that will be most invisible on a spec sheet and most present in the sound. A reverb that starts bright and dims quadratically as its energy decays sounds fundamentally different from one with fixed damping. The `pCantilever` parameter, defaulting to 0.3, means this behavior is on by default — subtly present in every patch.

The three known issues are all V1.1 candidates, not blockers. The engine ships clean. The two Blessing candidates — Chiasmus Reverb Architecture and Convergence-Gated Golden Standing Waves — are both worthy of formal proposal to the Ghost Council. B017 and B018 would bring the total fleet Blessing count to 18.

The Oxbow Eel waits in the twilight zone. It does not rush. Neither should you.

---

*Seance conducted 2026-03-21. Ghost Council presiding: Moog, Buchla, Smith, Kakehashi, Pearlman, Tomita, Vangelis, Schulze.*
