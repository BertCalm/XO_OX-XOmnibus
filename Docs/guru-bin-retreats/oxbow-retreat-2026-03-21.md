# OXBOW Retreat Chapter
*Guru Bin — 2026-03-21*

---

## Engine Identity

- **Gallery code:** OXBOW | **Accent:** Oxbow Teal `#1A6B5A`
- **Parameter prefix:** `oxb_`
- **Creature mythology:** The Oxbow Eel — Twilight Zone (200–1000m). An oxbow lake is formed when a river meanders so far from its original course that it cuts itself off, leaving a crescent-shaped body of water suspended from the flow. The river moves on. The pool remains. An eel makes its home in that pool: slow-moving, patient, feeding on what drifts in, perfectly adapted to stillness. It does not chase. It waits for the world to come to it. When sound enters the Chiasmus FDN, it becomes the river current. The moment the note ends, the channel is cut. What remains is the pool — a suspended, entangled body of resonance slowly erasing itself, while golden standing waves shimmer at its surface in the twilight light.
- **Synthesis type:** Entangled reverb synthesis — Chiasmus FDN (8-channel Householder matrix with mirror delay topology), phase erosion allpass network, convergence-gated golden resonance (4 peak filters at φ harmonics), asymmetric cantilever decay
- **Polyphony:** Monophonic — a single pool of entangled resonance
- **feliX/Oscar polarity:** Oscar-dominant (0.3/0.7) — patience, depth, slowness
- **Seance score:** 9.0/10
- **Macros:** M1 CHARACTER (exciter brightness + resonance mix), M2 MOVEMENT (erosion depth + rate), M3 COUPLING (entanglement), M4 SPACE (dry/wet + room size)
- **Expression:** Velocity → exciter brightness AND duration (dual-path). Aftertouch → entanglement (+0.3). Mod wheel CC1 → resonance mix (+0.5).

---

## Pre-Retreat State

OXBOW was added to XOceanus on 2026-03-20 and seanced the same day, receiving a post-fix score of 9.0/10. It arrived with 160 presets across all moods, the highest factory count of any engine added in that session. The seance confirmed all 18 parameters wired to audio, two Blessing candidates (Chiasmus Reverb Architecture B017 and Convergence-Gated Golden Standing Waves B018), and six doctrine passes.

This retreat is OXBOW's first formal sound design examination. It begins not from a thin library that needs expansion, but from an already substantial one that needs to understand itself. 160 presets is a quantity. What this retreat provides is the *why* behind the numbers — the parameter logic, the sweet spots, the traps, and the five recipe categories that will carry the library from 160 toward 210+ with deliberate intent rather than random exploration.

The engine is one of four in the fleet scoring 9.0 or above. It earns this position through an idea, not a feature list. That idea deserves to be understood before it is used.

---

## Phase R1: Opening Meditation

Close your eyes. Breathe.

You are standing at the edge of an oxbow lake in the Twilight Zone — 400 meters below the ocean surface, where the last trace of sunlight filters down as a diffuse blue-grey wash. The water is cold. It is still. It has been still for a very long time.

A river cut through here, long ago. It meandered and slowed, carved wider and wider curves until finally, in one monsoon season, the river found a shorter path and abandoned this loop entirely. The oxbow formed. The current died. The water that was once rushing became suspended — the same water, the same temperature, the same dissolved minerals, but no longer going anywhere.

When you strike a note on OXBOW, the exciter — a pitched sine wave mixed with broadband noise, shaped by an exponential envelope — flows through the pre-delay and into the 8-channel Chiasmus FDN like water entering the cut-off point. For the duration of the exciter, the FDN is fed. Then the note ends. The exciter decays. The channel closes. What remains is the reverb tail: water suspended in the oxbow pool, neither flowing nor escaping, simply circling.

The Chiasmus architecture means this pool has structural chirality. The four delay channels that sum to the Left output use prime delay times of 28, 38, 46, 56 milliseconds — an ascending sequence. The four channels that sum to the Right output use the exact same values in reverse — 56, 46, 38, 28 milliseconds. Every resonant mode present in the Left exists in the Right. But the Left mode at 28ms is encountered by the Right at 56ms. The two decay arcs share every frequency but experience them in mirror-image temporal order.

The sound is entangled in the precise sense the word is used in physics: the two outputs are not merely correlated, they are structurally complementary. You cannot change the character of one without changing the other, because they are built from the same materials in opposite sequence.

The `oxb_entangle` parameter controls how much these two mirror arcs bleed into each other. At zero, they are independent — pure left-right Chiasmus. At maximum, they converge toward a single shared pool. The eel lives in the threshold between these states: entanglement at 0.5–0.7, where the mirrors are partly merged but the structural counterpoint between them is still audible.

The golden resonance is the oxbow's surface shimmer. As the reverb settles — as the L and R outputs converge toward each other, as the stereo image narrows — four peak filters tuned to the φ harmonics of the original MIDI note briefly ring out. f, f×φ, f×φ², f×φ³. Amplitudes at −3dB per φ interval. The lowest harmonic is loudest; the third harmonic at φ³ is barely a whisper. This is not an effect applied to the reverb. It is the reverb's own standing waves, revealed in the moment of maximum convergence.

The eel does not rush. It has been in this pool for a very long time. It knows exactly when the surface will shimmer.

---

## Phase R2: The Signal Path Journey

### I. The Exciter — Kakehashi Impulse Architecture

The signal path begins before the FDN, in the pitched impulse exciter. A sine wave at the MIDI fundamental frequency is mixed with xorshift32 noise in a ratio controlled by `oxb_exciterBright`. At brightness=0.0, the exciter is pure sine — a clean, pitched impulse that seeds the FDN with a single frequency. At brightness=1.0, the noise is dominant and the exciter becomes a broadband burst. In practice, the most musical range is 0.4–0.7, where the sine gives the FDN a pitched fundamental to resonate around while the noise seeds the upper modal structure.

The velocity interaction is the critical insight. Both the sine/noise ratio and the exciter duration scale from velocity via `(0.5 + velocity * 0.5)`. At velocity=0, both brightness and duration are half of their parameter values. At velocity=1.0, they reach full values. This means a soft touch produces a brief, sine-dominant, low-energy impulse that seeds the FDN quietly and cleanly. A hard strike produces a long, noise-heavy, high-energy burst that charges the FDN richly. The reverb tail that grows from a hard strike is not just louder — it is spectrally different, because the FDN received more noise content and more excitation time.

The exciter passes through a pre-delay buffer before entering the FDN. This buffer is scaled by both `oxb_predelay` and `oxb_size` — the effective predelay is `pPredelay * (0.1 + pSize * 0.9)`. At small sizes, the predelay shrinks even when the parameter value is high. At large sizes, the full predelay is applied. This coupling between size and predelay is physically correct: a large room has significant initial time delay (ITD) between the direct sound and first reflection; a small room collapses that gap. The predelay parameter controls the character of that gap, and the size parameter scales how much gap is possible.

**Exciter sweet spots:**
- `oxb_exciterDecay` 0.001–0.005s: Percussive impact character. The exciter fires and dies in under 5ms. The FDN receives a very brief seed. Best for short, pointed reverb character where the "attack" of the reverb is crisp and the tail is the primary feature.
- `oxb_exciterDecay` 0.008–0.020s: Natural impulse range. The most musical zone — the exciter has time to establish a pitch center without ringing too long in the dry signal path.
- `oxb_exciterDecay` 0.035–0.070s: Slow bloom. The exciter itself becomes audible as a short sustained tone before the FDN takes over. At high dry/wet values, this creates a "tone + reverb" structure where the dry signal has its own pitch and duration.
- `oxb_exciterDecay` 0.080–0.100s: Long drone seed. The exciter becomes the dominant element at intermediate dry/wet values. The FDN's tail merges with an ongoing tonal excitation. Use with `oxb_dryWet` 0.6+ to keep the wet signal primary.

**Exciter brightness sweet spots:**
- 0.0–0.15: Pure or near-pure sine. The FDN is seeded with a single frequency. The reverb will have very strong mode identity — almost metallic resonance from the primes. Best for harmonic resonance exploration.
- 0.25–0.45: Sine-dominant with noise texture. The fundamental is clear but the noise component seeds the upper modal structure. Most natural-sounding range.
- 0.55–0.75: Balanced sine/noise. Hard strikes become noticeably noise-forward. This is the widest velocity expression range — soft notes are tonal, hard notes are percussive.
- 0.80–1.0: Noise-dominant. The pitched character of the exciter is secondary. The FDN receives primarily broadband content. Best for dense, non-pitched reverb textures.

---

### II. The Chiasmus FDN — The Pool

Eight delay channels, organized in pairs: channels 0–3 sum to the Left output, channels 4–7 sum to the Right output. Delay times:
- Left: 28ms, 38ms, 46ms, 56ms
- Right: 56ms, 46ms, 38ms, 28ms

The Householder feedback matrix (`H[i][j] = 0.75 if i==j, -0.25 otherwise`) is applied identically to both groups before feeding back into the delay lines. This is the standard energy-preserving feedback matrix for N=8 FDNs. The prime delay lengths prevent mode beating — no two delay times share a common factor, so no two modal frequencies will align and create destructive interference nodes.

The feedback coefficient is derived from `oxb_decay` via the RT60 formula: `exp(-6.9078 / (decay * sampleRate))`. At decay=4.0s, the FDN loses 60dB in 4 seconds. At decay=30.0s, it loses 60dB in 30 seconds. At decay>29.0s, the coefficient is clamped to 1.0 — Schulze's infinite decay: the FDN never loses energy. Notes played in infinite mode accumulate.

**Decay sweet spots:**
- 0.1–0.5s: Tight, present reverb. The tail is audible but brief. Good for rhythmic contexts where previous notes must not blur into the next.
- 0.8–2.5s: Natural room range. Concert halls, chambers, stone rooms. The FDN has time to establish modal character before the energy drops below audibility.
- 3.0–6.0s: Cathedral and cavern territory. Long tails with rich modal structure. The cantilever decay transformation becomes fully audible in this range.
- 8.0–15.0s: Extended acoustic environments. Notes played here persist for many seconds. The convergence-gated golden resonance has maximum opportunity to activate in long tails.
- 20.0–30.0s: Drone generation. Notes become sustained sonic objects rather than decaying events. Multiple notes accumulate into dense harmonic fields.
- 30.0s+: Infinite decay (feedback = 1.0). The FDN never loses energy from the current note. New notes add to the existing field. This is the Schulze mode — sound that accumulates and transforms but does not disappear.

**`oxb_size` — the room dimension parameter:**
Size scales both the effective predelay and the damping frequency: `pDamping = pDamping * (0.5 + pSize * 1.0)`. At size=0, damping is halved (darker) and the predelay is minimal (intimate). At size=1, damping is doubled (brighter, more open) and the full predelay applies (vast). Size is thus a combined "space type" control: turning it down makes the space smaller AND darker; turning it up makes it larger AND brighter. This is physically correct — large spaces have longer initial reflections and retain higher frequencies longer because the surfaces absorb less per unit distance.

**Size sweet spots:**
- 0.0–0.15: Intimate — close-miked, dark, minimal predelay separation. Good for tracking vocals where the reverb should feel close but not room-like.
- 0.2–0.4: Small room range. The predelay is noticeably short and the damping is warm. Useful for acoustic instrument placement.
- 0.45–0.65: Concert hall range. Full predelay separation, balanced damping. The most generally musical zone.
- 0.7–0.85: Cathedral and large hall. Long predelay, bright upper frequencies, the space is audible as a character element not just a context.
- 0.85–1.0: Vast — the predelay is at its maximum (200ms × 0.9 = 180ms effective), the damping is at its brightest. Best for Aether mood presets and drone generation.

---

### III. Entanglement — The Cross-Coupling Parameter

The `oxb_entangle` parameter controls the blend between the Left and Right FDN channel groups via a cross-coupling matrix. The effective blend is `entangleMix = pEntangle * 0.3` — so the maximum functional range is 30% cross-coupling. At entangle=0.0, Left and Right are independent mirror-arcs. At entangle=1.0, 30% of each channel bleeds into the other.

The seance noted that Buchla considered the 30% cap conservative and advocated for 50%. Vangelis defended the cap as preventing phase cancellation. The practical consequence for sound design: the full parameter range (0.0–1.0) is usable and musically meaningful, because the cap is already built in. Moving entangle from 0.5 to 1.0 produces a clearly audible narrowing of the stereo field as the mirror arcs converge. At 0.0, the stereo image is widest and the Chiasmus counterpoint is most distinct.

Additionally, aftertouch adds up to +0.3 to entangle in real time. At entangle=0.6 with maximum aftertouch, the effective value approaches 0.9 — the mirror arcs nearly converge. This is the Vangelis expressive arc: pressing harder into a held note draws the stereo image together, creates the convergence condition that triggers the golden resonance, and then releasing the pressure lets the image breathe apart again.

**Entanglement sweet spots:**
- 0.0–0.15: Maximum Chiasmus separation. The Left and Right arcs are nearly independent. Wide, interesting stereo image with clear structural counterpoint. Best when OXBOW is used as a stereo reverb where the spatial character is valued.
- 0.25–0.45: Natural blend zone. The mirrors are partly merged but the counterpoint is still audible. The stereo field feels deep rather than wide. Most presets should live here.
- 0.50–0.70: Standard performance range. Aftertouch from 0.70 reaches the golden resonance convergence zone reliably. Default value is 0.6, which is correctly calibrated for expressive use with aftertouch.
- 0.75–0.90: Near-convergence. The stereo field is noticeably narrow. Useful for "mono-compatible" reverb presets where the sound must translate to mono without phase issues.
- 0.90–1.0: Maximum entanglement (30% cross-coupling). The mirror arcs are nearly merged. The golden resonance is easily triggered without aftertouch pressure because the convergence condition is almost always met.

---

### IV. Phase Erosion — The Breathing

Four allpass filters per stereo channel (L and R) are modulated by slow LFOs. The key architectural detail: L and R allpass modulations have opposite polarity. When the L allpass moves its center frequency upward, the R moves downward by the same amount. This creates a breathing stereo widening and narrowing pattern — when the mono sum (L+R) is measured, the allpass colorations partially cancel each other, creating the impression of spectral self-erosion.

The four allpass filters are centered at fixed base frequencies: 300 Hz, 1100 Hz, 3200 Hz, 7500 Hz — spanning the full audible spectrum in roughly 1.5-octave steps. The four LFOs run at 0.03 Hz, 0.05 Hz, 0.07 Hz, 0.09 Hz, each with 90° phase offset. At the minimum erosion rate parameter (0.01 Hz), LFO 0 slows to 0.01 Hz — a modulation period of 100 seconds.

The erosion depth parameter scales the modulation amount: effective depth = `pErosionD * 0.4`. At depth=1.0, the allpass center frequencies move by ±40% of their base values. At depth=0.5, they move ±20%.

The erosion system is why OXBOW presets feel alive even when nothing is playing — the four LFOs continue running between notes, so two successive notes played at the same pitch will encounter the allpass network in different states. The reverb character varies slightly from note to note. This is the engine's autonomous breathing.

**Erosion rate sweet spots:**
- 0.01–0.02 Hz: Extremely slow. The spectral character drifts over 50–100 second periods. In a sustained session, the instrument's reverb tail will feel different 5 minutes in than it did at the start. Barely perceptible per note.
- 0.03–0.06 Hz: Natural breath. The LFO period is 17–33 seconds. The spectral coloration shifts slowly enough that it feels like the room breathing rather than obvious modulation. Default range — most presets should live here.
- 0.08–0.15 Hz: Audible movement. The allpass sweep becomes noticeable as gentle spectral motion in long reverb tails. The stereo image widens and narrows over 7–12 second cycles. Beautiful in Aether presets with long decay.
- 0.15–0.30 Hz: Pronounced chorus-like movement. The erosion becomes a primary timbral character rather than a background process. Best with lower erosion depths (0.2–0.4) to keep it from overpowering the FDN.
- 0.30–0.50 Hz: Rapid modulation. The allpass sweeps audibly on individual notes. Useful for Flux presets where movement is the primary aesthetic.

**Erosion depth sweet spots:**
- 0.0: No modulation. The allpass filters are static — fixed spectral coloration, no movement. The reverb has less "life" but the stereo image is stable. Use when OXBOW is functioning as a precise room reverb without character.
- 0.1–0.25: Trace erosion. The spectral self-cancellation is present but subtle. Adds the sensation of a slightly imperfect, breathing room without obvious motion.
- 0.3–0.55: Standard erosion range. The L/R opposite-polarity movement creates the characteristic OXBOW stereo breathing. Default (0.4) is correctly placed.
- 0.55–0.75: Heavy erosion. The allpass modulation becomes audible as spectral movement in mid-length reverb tails. Combine with higher erosion rates for clearly animated textures.
- 0.75–1.0: Maximum erosion. The allpass centers move by 40% of base values. At rapid rates (0.2+ Hz), this creates clearly audible modulation. At slow rates (0.03 Hz), it creates a reverb that transforms significantly over long held notes. Use deliberately for Flux and Prism textures.

---

### V. Golden Resonance — The Standing Waves

The golden resonance system is OXBOW's most distinctive feature and its most patience-rewarding mechanism.

Four CytomicSVF Peak filters are tuned at note-on to:
- f (the MIDI fundamental)
- f × φ (f × 1.618...)
- f × φ² (f × 2.618...)
- f × φ³ (f × 4.236...)

Each filter is applied to both L and R outputs. The amplitude weighting is `{1.0, 0.708, 0.501, 0.354}` — precisely −3dB per φ harmonic, following Tomita's spectral layering convention. The fundamental resonance is loudest; the third harmonic at φ³ is barely a whisper at 35% of full amplitude.

These filters are not always active. They are gated by a Mid/Side convergence detector. The FDN's L and R outputs are continuously measured for Mid energy (L+R) and Side energy (L−R). When the Mid/Side ratio exceeds `oxb_convergence`, the resonance gain attacks (5ms attack envelope). When the ratio falls below convergence, the gain releases (50ms release). The golden harmonic stack only rings when the stereo image converges — when the reverb is settling into its entangled equilibrium.

This is the engine's philosophical center: the golden resonance rewards patience. A player who strikes quickly and moves on will rarely hear it — the reverb tail has not had time to converge. A player who holds notes, lets them breathe, works with long decays, will encounter the golden shimmer regularly. It is a feature that teaches you how to play the instrument.

The `oxb_convergence` parameter (range: 1.0–20.0) controls how sensitive the trigger is. At 1.0, a very slight Mid dominance triggers the resonance — it activates easily. At 20.0, only extreme convergence (nearly mono output) triggers it — it activates rarely, only in long, settled tails. Default is 4.0.

The `oxb_resonanceMix` parameter (range: 0.0–1.0) scales the amplitude of the golden output. At 0.0, the filters run but produce no output. At 1.0, the golden harmonic stack is at full amplitude. Default is 0.3. Mod wheel adds up to +0.5, allowing the golden resonance to be expressively increased without changing the preset.

The `oxb_resonanceQ` parameter (range: 0.5–20.0) controls the Q (bandwidth) of the four peak filters. At Q=0.5, the filters are very broad — the golden harmonics blend into wide spectral regions. At Q=20.0, the filters are narrow and focused — the harmonics ring as distinct pitched tones. Note from the seance: this parameter only updates at note-on, not continuously. Sweeping it while holding a note will not produce audible change until the next key press.

**Convergence sweet spots:**
- 1.0–2.5: Very sensitive. The golden resonance activates at slight Mid dominance. It fires frequently during most reverb tails. Best when the resonance should be a constant presence in the texture.
- 3.0–5.0: Moderate sensitivity. The resonance activates only when the reverb is clearly settling. Default (4.0) is well-calibrated — the resonance appears in the second half of moderate decay tails.
- 6.0–10.0: Selective. The resonance requires substantial Mid dominance. Long decay tails (4s+) will trigger it near the end of their decay arc. Short tails will not.
- 12.0–20.0: Rare activation. Only very long, near-mono tails trigger the resonance. Use when the golden shimmer should be a special event, not a regular presence.

**ResonanceQ sweet spots:**
- 0.5–2.0: Wide filter — golden harmonics as broad spectral color rather than pitched tones. The resonance enriches the overall timbre without adding identifiable pitches.
- 3.0–8.0: Moderate focus. The harmonics are identifiable as approximate pitches but merge with the FDN's natural resonance. Most presets should use this range.
- 8.0–14.0: Focused resonance. The golden harmonics are clearly audible as distinct tonal contributions, especially at f and f×φ. The resonance adds a musical interval above the fundamental.
- 14.0–20.0: Narrow focus. The harmonics ring as distinct, narrow-band tones. The effect is similar to a short metallic ring at specific golden-ratio pitches above the fundamental. Unusual and striking.

---

### VI. Asymmetric Cantilever Decay

The cantilever system is the feature most invisible on a spec sheet and most present in the sound.

A `peakEnergy` tracker measures the peak FDN energy since the last note-on. The `currentEnergy` is a heavily smoothed measurement of the current FDN output level. The ratio `decayProgress = 1.0 - (currentEnergy / peakEnergy)` tracks how much energy has drained from the FDN since its peak — 0.0 when energy is at maximum, 1.0 when energy has fully drained.

The cantilever formula: `cantileverDamp = pDamping * (1.0 - pCantilever * decayProgress²)`. At decayProgress=0 (peak energy), the damping is `pDamping * 1.0` — full damping. As energy drains and decayProgress approaches 1.0, the damping reduces toward `pDamping * (1 - cantilever)`. At cantilever=1.0, the damping eventually reaches zero — the FDN becomes undamped as the energy approaches zero.

Wait. That seems backward. Let's read it carefully: when decayProgress is high (energy is low), `cantileverDamp` = `pDamping * (1 - cantilever * 1.0)` — which is a *lower* damping value, meaning the LP cutoff frequency is lower, meaning *more* damping. At decayProgress=0 (full energy), `cantileverDamp = pDamping * 1.0` — the full, bright cutoff. So the cantilever increases the damping (lowers the cutoff) as the energy drains. The reverb starts bright and gets progressively darker as it ages.

This is the physical behavior of a cantilever beam: high-frequency partials decay faster than low-frequency ones because of internal friction proportional to frequency. The FDN begins with full spectral brightness — all frequencies present — and as time passes, the higher frequencies are progressively absorbed while the low-frequency content sustains longer. The reverb transforms as it decays.

At `oxb_cantilever=0.0`, this darkening effect is absent — fixed linear damping throughout the tail. At `oxb_cantilever=1.0`, the damping drops to zero as energy drains — maximum darkening arc. The default is 0.3, which is subtly present in every preset and responsible for much of OXBOW's characteristic "aging" sound.

**Cantilever sweet spots:**
- 0.0: No cantilever effect. Linear damping throughout. The reverb sounds consistent from attack to decay — suitable for utility reverb applications where the tail character should not transform.
- 0.1–0.25: Subtle arc. The brightening-to-darkening transition is audible only in long tails (3s+). Most listeners would not consciously identify it, but the reverb "feels" more natural than one with fixed damping.
- 0.3–0.5: Standard range (default 0.3). The brightening arc is clearly audible in medium-to-long tails. The beginning of the reverb sparkles; the end settles into warmth. This is the musical core of the parameter.
- 0.55–0.75: Strong arc. The contrast between attack brightness and decay warmth is pronounced. Notes have a clear "arrival" quality at their attack that fades into a warm pool. Beautiful for Atmosphere and Aether moods.
- 0.75–1.0: Maximum arc. The damping at the end of a long tail approaches zero — meaning the FDN is nearly undamped as the signal fades. The final moments of a reverb tail have maximum spectral openness, which can produce a brief brightening ghost at the very end of long tails. Unusual and worth exploring in Flux presets.

---

### VII. The Damping Parameter

`oxb_damping` (range: 200–16000 Hz) sets the base cutoff frequency for the 8 CytomicSVF LP filters in the FDN feedback path. The cantilever system modifies this dynamically (see above). The size parameter also scales it: `pDamping = pDamping * (0.5 + pSize)`.

At 200 Hz, only the very lowest frequencies pass through the feedback loop — an extremely dark, muffled reverb. At 16000 Hz, the LP filter is essentially absent and full-spectrum feedback occurs. The perceptual character of the damping parameter is: low values = dark, warm, wool-like; high values = bright, metallic, glass-like.

**Damping sweet spots:**
- 200–1000 Hz: Extremely dark. The reverb tail has no treble content. Useful for deep subterranean environments or heavily absorbed spaces.
- 1500–3500 Hz: Dark room character. The high-frequency content decays very quickly. Warm, recessed reverb. Good for vintage tape aesthetic or underground stone rooms.
- 3500–6000 Hz: Natural room range. The high frequency content decays noticeably faster than the low, but both are present throughout the tail. Most atmospheric presets live here.
- 6000–10000 Hz: Bright hall character. The treble persists well into the tail. Adds presence and air to the reverb.
- 10000–16000 Hz: Very bright. The FDN feedback is nearly full-spectrum. Best for metallic or crystalline spaces where the high frequency content is essential.

---

### VIII. Predelay — The Space Between Action and Echo

`oxb_predelay` (range: 0–200ms, default 20ms) controls the time between the exciter firing and its arrival at the FDN. In physical terms, it represents the time sound takes to travel to the first reflective surface.

A critical detail: the effective predelay is `pPredelay * (0.1 + pSize * 0.9)`. At size=0, the maximum effective predelay is 20ms regardless of the parameter. At size=1.0, the full parameter value applies. The predelay parameter and size parameter are coupled — increasing size also increases the temporal separation of the reverb.

**Predelay sweet spots:**
- 0–5ms: Near-zero separation. The exciter and FDN feel like a single event. Good for intimate spaces or when OXBOW is used on sustained pads where the exciter/reverb distinction should be minimal.
- 8–20ms: Natural room. The space between direct sound and first reflection is perceptible as "room feel" without being audible as a distinct echo. Standard range for most music production contexts.
- 25–50ms: Concert hall. The initial time delay is clearly perceptible, giving the listener a sense of scale. Notes played staccato will have a clear "space" before the tail begins.
- 60–100ms: Large hall or cave. The predelay is audible as a slight pause before the reverb erupts. Creates the impression of a vast, empty space.
- 100–200ms: Maximum separation (with size=1.0). The predelay approaches or exceeds the natural ambiguity threshold (80–100ms) where listeners begin to hear separate echoes rather than room context. Use deliberately for dramatic pre-echo effects.

---

### IX. Dry/Wet — The Balance of River and Pool

`oxb_dryWet` (range: 0.0–1.0, default 0.5) balances the dry exciter signal against the wet FDN reverb output. The formula is: `finalL = exciterSample * (1.0 - pDryWet) + wetL * pDryWet`.

Note what the dry signal is: it is the exciter — the pitched sine/noise impulse. It is not a dry "through" signal from a hypothetical sound source. OXBOW generates its own source (the exciter), so the dry/wet balance determines how much of the raw impulse is heard versus how much the FDN's pool dominates.

At very low dry/wet values (0.1–0.2), the exciter is nearly inaudible — OXBOW sounds like a pure reverb object with no apparent "source." At high dry/wet values (0.7–0.9), the exciter is prominent and the reverb is a supporting layer. The most evocative sound for OXBOW as a standalone instrument is usually in the 0.35–0.65 range, where the exciter provides a brief pitched attack and the FDN provides the sustained pool.

**Dry/wet sweet spots:**
- 0.0–0.15: Pure reverb mode. The exciter is inaudible. OXBOW sounds like a self-generating reverb. The only information about the note is its pitch (encoded in the golden resonance frequencies) and velocity (encoded in the FDN energy level). Uncanny, immersive.
- 0.2–0.4: Reverb-dominant. The exciter is present as a brief percussive click followed by the dominant pool. Functional as a reverb instrument.
- 0.4–0.6: Balanced. Both exciter and reverb are audible. The note has a clear attack (exciter) and a sustained tail (FDN). Most versatile range.
- 0.6–0.8: Exciter-forward. The impulse is prominent, the reverb is support. Best when OXBOW is used as a transient sound source with reverb character.
- 0.8–1.0: Exciter-dominant. Very dry — the pool is recessive. Suitable when OXBOW is performing as a minimal, intimate instrument where the reverb should barely be noticed.

---

## Phase R3: Parameter Map — Sweet Spots Summary

| Parameter | ID | Range | Default | Conservative | Musical Core | Expressive | Extreme |
|-----------|-----|-------|---------|--------------|--------------|-----------|---------|
| Space Size | `oxb_size` | 0.0–1.0 | 0.5 | 0.15–0.30 | 0.45–0.65 | 0.70–0.85 | 0.90–1.0 |
| Decay Time | `oxb_decay` | 0.1–60.0s | 4.0s | 0.5–2.0s | 3.0–8.0s | 10–20s | 30–60s |
| Entanglement | `oxb_entangle` | 0.0–1.0 | 0.6 | 0.10–0.20 | 0.40–0.65 | 0.65–0.80 | 0.85–1.0 |
| Erosion Rate | `oxb_erosionRate` | 0.01–0.5 Hz | 0.08 | 0.01–0.03 | 0.04–0.12 | 0.12–0.25 | 0.25–0.50 |
| Erosion Depth | `oxb_erosionDepth` | 0.0–1.0 | 0.4 | 0.05–0.20 | 0.25–0.55 | 0.55–0.75 | 0.75–1.0 |
| Convergence | `oxb_convergence` | 1.0–20.0 | 4.0 | 8.0–15.0 | 3.0–6.0 | 1.5–3.0 | 1.0–1.5 |
| Resonance Focus | `oxb_resonanceQ` | 0.5–20.0 | 8.0 | 0.5–3.0 | 4.0–10.0 | 10.0–16.0 | 16.0–20.0 |
| Resonance Mix | `oxb_resonanceMix` | 0.0–1.0 | 0.3 | 0.05–0.15 | 0.20–0.45 | 0.45–0.65 | 0.65–1.0 |
| Cantilever | `oxb_cantilever` | 0.0–1.0 | 0.3 | 0.0–0.10 | 0.20–0.50 | 0.50–0.75 | 0.75–1.0 |
| Damping | `oxb_damping` | 200–16000 Hz | 6000 | 1500–3500 | 4000–8000 | 8000–12000 | 12000–16000 |
| Pre-Delay | `oxb_predelay` | 0–200ms | 20ms | 0–8ms | 10–45ms | 50–100ms | 100–200ms |
| Dry/Wet | `oxb_dryWet` | 0.0–1.0 | 0.5 | 0.15–0.30 | 0.35–0.65 | 0.65–0.80 | 0.80–1.0 |
| Exciter Decay | `oxb_exciterDecay` | 0.001–0.1s | 0.01s | 0.001–0.005 | 0.008–0.025 | 0.030–0.060 | 0.065–0.100 |
| Exciter Bright | `oxb_exciterBright` | 0.0–1.0 | 0.7 | 0.0–0.20 | 0.30–0.65 | 0.65–0.85 | 0.85–1.0 |

---

## Phase R4: Macro Architecture

| Macro | ID | Effect | Performance Use |
|-------|-----|--------|----------------|
| CHARACTER | `oxb_macroCharacter` | +0.3 to exciterBright, +0.25 to resonanceMix | Sweep toward golden harmonic brightness — harder, more resonant character |
| MOVEMENT | `oxb_macroMovement` | +0.4 to erosionDepth, +0.15 to erosionRate | Increase spectral breathing motion in real time |
| COUPLING | `oxb_macroCoupling` | +0.4 to entanglement | Merge the mirror arcs — converge the stereo image toward mono |
| SPACE | `oxb_macroSpace` | +0.25 to dryWet, +0.3 to size | Expand the room and push further into the wet reverb |

**Macro philosophy:** The four macros correspond to the four fundamental dimensions of OXBOW's sound. CHARACTER controls spectral richness — how bright and harmonically active the reverb is. MOVEMENT controls temporal animation — how much the reverb breathes and shifts. COUPLING controls stereo geometry — how entangled the mirror arcs are. SPACE controls room scale — how large and wet the environment feels.

A preset with all macros at 0.0 should already sound complete. The macros should allow performance enhancement, not be required for basic functionality. Design presets where the "base" (macros at 0.0) is a fully realized sound, and each macro opens a new expressive direction.

**Aftertouch** adds +0.3 to entanglement in real time, stacking with the COUPLING macro. If COUPLING is at 0.5 (effective +0.2 entangle) and aftertouch is at full (+0.3), the total entangle addition is +0.5. Design presets so that the base entangle value at the "point of best macro use" leaves room for aftertouch to push further without causing mono collapse.

**Mod wheel** adds +0.5 to resonanceMix in real time, stacking with CHARACTER macro. The mod wheel is the instrument's "golden shimmer" control — turning it up intensifies the φ-harmonic resonance. Default resonanceMix of 0.3 + full mod wheel (0.5) = 0.8 resonanceMix. This is a powerful effect. Design presets where the base resonanceMix is low (0.15–0.30) so that the mod wheel has room to expand dramatically.

---

## Phase R5: The Five Recipe Categories

### Recipe Category 1: The Still Pool — Intimate Room Reverb

**Identity:** OXBOW as a precision utility reverb. Small to medium rooms, short decay, low entanglement, visible dry signal. These are the workhorses — functional presets that serve piano, guitar, vocals, acoustic instruments. The golden resonance is present but subtle. The cantilever adds warmth without transforming the tail dramatically. Erosion is trace.

**Parameter ranges:**
- `oxb_size`: 0.20–0.50
- `oxb_decay`: 0.8–3.5s
- `oxb_entangle`: 0.10–0.40
- `oxb_erosionRate`: 0.04–0.12 Hz
- `oxb_erosionDepth`: 0.10–0.30
- `oxb_convergence`: 4.0–8.0 (selective golden activation)
- `oxb_resonanceQ`: 3.0–8.0 (moderate focus)
- `oxb_resonanceMix`: 0.10–0.25
- `oxb_cantilever`: 0.15–0.45
- `oxb_damping`: 4000–9000 Hz
- `oxb_predelay`: 8–30ms
- `oxb_dryWet`: 0.25–0.45
- `oxb_exciterDecay`: 0.006–0.018s
- `oxb_exciterBright`: 0.35–0.60

**Target preset names (20 suggestions):**
Still Water (init), Vinyl Room, Recording Room, Wood Panel, Stone Floor, Chamber Glass, Tile Bath, Linen Closet, Brick Stairwell, Garden Shed, Farmhouse Hall, Oak Study, Plaster Room, Instrument Booth, Hallway Depth, Morning Studio, Late Night Room, Wool Wall, Cement Vault, Narrow Shaft

**Why the category works:** Low entanglement means the Chiasmus counterpoint is subtle — the stereo field feels natural rather than obviously engineered. Short decay keeps the pool small. The exciter is audible, giving the reverb a clear "source." The golden resonance is selective (high convergence threshold) so it only appears on carefully played long notes, not on every chord.

**Trap to avoid:** Resist the temptation to push decay above 4.0s in this category. These presets lose their utility value the moment the tail becomes a dominant element in the mix. If the reverb is audible in the spaces between notes, it has left the Still Pool category.

---

### Recipe Category 2: The Twilight Hall — Atmospheric Long Room

**Identity:** Concert hall and large architectural space character. Decay 4–12 seconds, full predelay separation, the cantilever transformation is clearly audible, the golden resonance activates regularly. These presets are not utility tools — they are sonic environments. Notes played here exist inside the space rather than being placed in front of it.

**Parameter ranges:**
- `oxb_size`: 0.55–0.80
- `oxb_decay`: 4.0–12.0s
- `oxb_entangle`: 0.35–0.65
- `oxb_erosionRate`: 0.03–0.10 Hz
- `oxb_erosionDepth`: 0.25–0.50
- `oxb_convergence`: 2.5–5.0 (moderately sensitive)
- `oxb_resonanceQ`: 5.0–12.0
- `oxb_resonanceMix`: 0.20–0.40
- `oxb_cantilever`: 0.30–0.65
- `oxb_damping`: 3500–7000 Hz
- `oxb_predelay`: 30–80ms
- `oxb_dryWet`: 0.40–0.65
- `oxb_exciterDecay`: 0.012–0.035s
- `oxb_exciterBright`: 0.45–0.70

**Target preset names (20 suggestions):**
Cathedral Depth, Stone Nave, Vault Approach, Spiral Staircase, Canyon Mouth, Marble Hall, Long Corridor, Winter Church, Cistern Water, Railway Arch, Suspension Bridge, Clifftop Chapel, Old Theatre, Cave Entrance, Brick Cathedral, Glass Atrium, Tower Bell, Quarry Wall, Tunnel Far End, Acoustic Chamber

**Why the category works:** The 4–12s decay range gives the cantilever transformation its full arc — the reverb begins bright (full damping at peak energy) and settles into warmth over many seconds. The moderate convergence (2.5–5.0) means the golden resonance fires regularly on notes with 3s+ sustain, adding harmonic shimmer that rewards patient playing. The predelay at 30–80ms creates clear spatial definition.

**Trap to avoid:** The Twilight Hall category can drift into Infinite Drone territory if decay is pushed above 12s. Keep them distinct. The hall should decay completely within a musical phrase. If the tail is still audible 20 seconds later, it has left the architectural space and entered the suspended pool.

---

### Recipe Category 3: The Oxbow Pool — Infinite Drone and Accumulation

**Identity:** The Schulze mode. Decay at 20–60s or infinite (60.0s clipped to feedback=1.0). Notes accumulate. The FDN never empties. The golden resonance activates almost constantly because the reverb converges under its own sustained energy. Erosion is the primary source of movement — the slow LFO sweep creates the only temporal variation in a pool that otherwise has no decay arc. These are generative presets: play a note and leave; play two notes and the FDN creates a third relationship between them.

**Parameter ranges:**
- `oxb_size`: 0.70–1.0
- `oxb_decay`: 20.0–60.0s (or 60.0 for infinite)
- `oxb_entangle`: 0.45–0.80
- `oxb_erosionRate`: 0.02–0.08 Hz (slow enough to feel like breathing, not modulation)
- `oxb_erosionDepth`: 0.30–0.65
- `oxb_convergence`: 1.5–4.0 (easily triggered)
- `oxb_resonanceQ`: 8.0–16.0 (focused — the harmonics should be audible in the pool)
- `oxb_resonanceMix`: 0.30–0.60
- `oxb_cantilever`: 0.10–0.40 (low-to-moderate — infinite pools don't need to "age"; they accumulate)
- `oxb_damping`: 2500–6000 Hz
- `oxb_predelay`: 40–120ms
- `oxb_dryWet`: 0.50–0.75
- `oxb_exciterDecay`: 0.015–0.050s
- `oxb_exciterBright`: 0.35–0.65

**Target preset names (20 suggestions):**
Infinite Oxbow, Frozen River, Still Lake, Permanent Pool, Cut Current, Suspended Water, Long Memory, The Eel Waits, Deep Still, Accumulation, Notes That Stay, Endless Chamber, Pooled Light, Phantom Resonance, Eternal Room, The Oxbow Forms, After the River, Pool of Tone, Resting Frequency, Twilight Permanence

**Why the category works:** The infinite FDN creates a pool that grows and sustains rather than decays. Each new note adds to the existing field rather than starting fresh (because peakEnergy is reset at note-on but the FDN's existing energy is not cleared). The golden resonance at low convergence thresholds (1.5–3.0) fires almost continuously because a sustained, accumulated reverb pool converges strongly. The erosion provides the only movement in an otherwise static field.

**The essential difference from Category 2:** Category 2 presets decay and end. Category 3 presets do not decay — they are interrupted only by the next note. Playing multiple notes in Category 3 creates an accumulating harmonic field where the relationships between notes become the composition.

**Performance guidance for Oxbow Pool presets:** Use sparingly — one note every 4–8 seconds. Allow the golden resonance to activate and shimmer between notes. Avoid rapid playing, which floods the FDN and destroys the spatial clarity. These presets reward restraint.

---

### Recipe Category 4: The Erosion Wave — Motion-Forward Spectral Textures

**Identity:** Phase erosion as the primary character. High erosion depth (0.55–1.0), moderate-to-fast erosion rate (0.10–0.35 Hz), the spectral self-cancellation in mono is clearly audible. These presets have the most "moving" character of any category — the reverb tail is not static but continuously transforming in spectral character. Best suited to Prism and Flux moods.

**Parameter ranges:**
- `oxb_size`: 0.40–0.75
- `oxb_decay`: 2.0–8.0s
- `oxb_entangle`: 0.40–0.70
- `oxb_erosionRate`: 0.10–0.35 Hz
- `oxb_erosionDepth`: 0.55–0.95
- `oxb_convergence`: 3.0–8.0 (moderate — golden resonance should appear but not dominate)
- `oxb_resonanceQ`: 4.0–10.0
- `oxb_resonanceMix`: 0.15–0.35 (moderate — let erosion take center stage)
- `oxb_cantilever`: 0.20–0.55
- `oxb_damping`: 5000–12000 Hz (brighter — erosion needs high-frequency content to modulate)
- `oxb_predelay`: 15–55ms
- `oxb_dryWet`: 0.45–0.70
- `oxb_exciterDecay`: 0.010–0.040s
- `oxb_exciterBright`: 0.50–0.80 (brighter exciter seeds more high-frequency content for erosion to work on)

**Target preset names (20 suggestions):**
Phase Erosion, Spectral Tide, Moving Water, Breathing Hall, Shimmer Decay, Dissolving Arch, Spectral Rain, Waving Glass, Shifting Prism, Tidal Phase, Erosion Tide, Breathing Stone, Phase Chorus, Moving Mirror, Dissolving Tone, Spectral Mist, Oscillating Cave, Flux Reverb, Prism Scatter, Tide Rhythm

**Why the category works:** High erosion depth means the allpass centers are moving ±40% of their base values. At 300 Hz, the L filter moves from 180–420 Hz while the R filter moves in opposite direction. At 7500 Hz, the L filter sweeps 4500–10500 Hz while R sweeps 10500–4500 Hz. The mono sum of these two opposite sweeps produces a characteristic comb-filtering + chorus effect that has no equivalent in static reverb. The effect is most audible in the mid-frequency range (1–5 kHz).

**Trap to avoid:** High erosion with very short decay (under 1.5s) wastes the erosion arc — the tail ends before the LFO completes a significant phase of its sweep. Category 4 presets need decay of at least 2s to allow the erosion to be heard as a spatial motion rather than just a timbral coloration.

---

### Recipe Category 5: The Chiasmus — Golden Resonance Showcase

**Identity:** Presets specifically designed to make the φ-harmonic resonance system the primary sonic feature. Low convergence threshold (1.0–2.5), high resonanceMix (0.40–0.80), high resonanceQ (10.0–20.0) so the harmonics ring as distinct pitched tones, low erosion depth so the FDN output is spectrally clean when it converges. These are the "show the blessing" presets.

**Parameter ranges:**
- `oxb_size`: 0.55–0.85
- `oxb_decay`: 5.0–15.0s
- `oxb_entangle`: 0.50–0.75 (mid-to-high — convergence requires stereo image narrowing)
- `oxb_erosionRate`: 0.02–0.06 Hz (slow — don't disturb the convergence)
- `oxb_erosionDepth`: 0.10–0.30 (low — the FDN output should be spectrally clean)
- `oxb_convergence`: 1.0–2.5 (very sensitive — the resonance should be active frequently)
- `oxb_resonanceQ`: 10.0–20.0 (narrow — the harmonics should be identifiable as tones)
- `oxb_resonanceMix`: 0.40–0.80
- `oxb_cantilever`: 0.35–0.70 (moderate-to-high — the brightening arc amplifies the golden shimmer at attack)
- `oxb_damping`: 3500–7000 Hz
- `oxb_predelay`: 25–70ms
- `oxb_dryWet`: 0.45–0.65
- `oxb_exciterDecay`: 0.012–0.030s
- `oxb_exciterBright`: 0.20–0.50 (lower brightness — a pure-sine exciter gives the FDN cleaner modes to resonate around)

**Target preset names (20 suggestions):**
Golden Ratio Meditation, Golden Section, Phi Harmonic, Convergence Point, Sacred Proportion, Golden Standing Wave, Fibonacci Pool, Spiral Hall, Golden Dawn, Phi Resonance, Harmonic Ratio, The Golden Mean, Irrational Beauty, Incommensurable, Convergence Gate, Golden Series, Phi Series, Standing Wave Pool, Euclid's Room, Proportional Depth

**Why the category works:** The golden resonance activates when Mid >> Side by the convergence ratio. High entanglement (0.50–0.75) means the L and R mirror arcs are partially merged, making convergence easier to achieve. Low erosion depth means the FDN output is spectrally clean, so the Mid/Side ratio is not confused by allpass coloration. High resonanceQ (10.0–20.0) makes the φ harmonics audible as discrete tonal contributions — the resonance is the feature, not a texture.

**The key musical insight:** These presets reward playing in fifths and octaves. The golden ratio φ is not a harmonic ratio (it is irrational), so the four resonance frequencies f, f×φ, f×φ², f×φ³ do not align with any standard harmonic or temperament system. When you play a minor third, a fifth, an octave — the FDN rings at those intervals in its resonant structure, but the golden filters add four additional non-standard tones above each note. The combined timbre is harmonically complex in a way that no standard reverb can produce.

---

## Phase R6: The Ten Awakenings — Preset Table

Each preset is a discovery. The parameter values are derived from the parameter logic above. These are reference presets — use them as starting points for new preset files.

---

### Preset 1: Still Water (Oxbow I)

**Mood:** Foundation | **Category:** Still Pool | **Discovery:** The init patch — everything available, nothing committed

| Parameter | Value | Why |
|-----------|-------|-----|
| `oxb_size` | 0.50 | Medium room — not intimate, not vast |
| `oxb_decay` | 4.0s | Natural concert hall decay |
| `oxb_entangle` | 0.50 | Moderate blend — Chiasmus counterpoint present |
| `oxb_erosionRate` | 0.06 Hz | Gentle breath — 17 second period |
| `oxb_erosionDepth` | 0.30 | Trace spectral motion |
| `oxb_convergence` | 3.5 | Moderate sensitivity — golden resonance on patient notes |
| `oxb_resonanceQ` | 6.0 | Broad golden harmonic color |
| `oxb_resonanceMix` | 0.20 | Subtle golden presence |
| `oxb_cantilever` | 0.30 | Standard brightening arc |
| `oxb_damping` | 7000 Hz | Warm-to-bright room |
| `oxb_predelay` | 25ms | Natural hall separation |
| `oxb_dryWet` | 0.45 | Balanced — both exciter and reverb present |
| `oxb_exciterDecay` | 0.012s | Brief impulse |
| `oxb_exciterBright` | 0.60 | Moderate sine/noise mix |

**Why this works:** This is the instrument's true neutral position. Every parameter is in its most functional zone. The golden resonance is present but not dominant. The cantilever creates a subtle aging quality. The erosion breathes very slowly. A producer encountering OXBOW for the first time should play this preset for five minutes before touching any parameter.

---

### Preset 2: Vinyl Room

**Mood:** Foundation | **Category:** Still Pool | **Discovery:** Low damping + short decay + low entanglement = vintage room aesthetic

| Parameter | Value | Why |
|-----------|-------|-----|
| `oxb_size` | 0.28 | Small room, short predelay |
| `oxb_decay` | 2.0s | Quick room decay |
| `oxb_entangle` | 0.12 | Minimal — dry, natural stereo |
| `oxb_erosionRate` | 0.15 Hz | Faster erosion for vinyl movement quality |
| `oxb_erosionDepth` | 0.35 | Present motion |
| `oxb_convergence` | 3.0 | Moderate |
| `oxb_resonanceQ` | 3.5 | Broad, warm golden color |
| `oxb_resonanceMix` | 0.12 | Very subtle golden |
| `oxb_cantilever` | 0.40 | Moderate arc — the warmth "settles in" |
| `oxb_damping` | 3200 Hz | Very dark, warm, wool-like |
| `oxb_predelay` | 5ms | Close-up, near-zero separation |
| `oxb_dryWet` | 0.30 | Dry-forward — the room should be felt, not heard |
| `oxb_exciterDecay` | 0.028s | Slightly longer impulse, more body |
| `oxb_exciterBright` | 0.32 | Near-sine — warm, rounded attack |

**Why this works:** The 3200 Hz damping cutoff heavily attenuates high frequencies in the feedback path, creating a classic dark, vintage room sound. The size at 0.28 means the effective damping is `3200 * (0.5 + 0.28) = 2496 Hz` — extremely dark. The erosion at 0.15 Hz creates subtle spectral motion at the speed of vinyl imperfections. This preset is ideal for lo-fi piano, Rhodes, vintage organ.

---

### Preset 3: Cathedral Depth

**Mood:** Atmosphere | **Category:** Twilight Hall | **Discovery:** Long predelay + strong cantilever + focused Q = the interior of stone

| Parameter | Value | Why |
|-----------|-------|-----|
| `oxb_size` | 0.72 | Large space, extended predelay |
| `oxb_decay` | 7.5s | Gothic cathedral character |
| `oxb_entangle` | 0.55 | Moderate merge — the stone surfaces reflect together |
| `oxb_erosionRate` | 0.04 Hz | Very slow breath — stone does not change quickly |
| `oxb_erosionDepth` | 0.35 | Present but not dominant |
| `oxb_convergence` | 3.5 | Moderate — golden resonance in the tail |
| `oxb_resonanceQ` | 9.0 | Focused golden harmonics |
| `oxb_resonanceMix` | 0.32 | Present resonance |
| `oxb_cantilever` | 0.55 | Strong arc — begins bright, settles dark over 7 seconds |
| `oxb_damping` | 5500 Hz | Bright room scaled by size to ~9350 Hz |
| `oxb_predelay` | 65ms | Large hall initial time delay |
| `oxb_dryWet` | 0.55 | Reverb-forward — the room is the environment |
| `oxb_exciterDecay` | 0.018s | Medium impulse |
| `oxb_exciterBright` | 0.52 | Moderate brightness |

**Why this works:** A 7.5-second decay in a size-0.72 room means the cantilever transformation spans the full arc: bright high-frequency content in the first second, progressively warmer over the next 6 seconds, settling into a warm, low-energy tail. The 65ms predelay creates clear spatial separation — listeners immediately understand that they are inside a large space. The focused resonanceQ (9.0) makes the golden harmonics identifiable tonal contributions in the long tail.

---

### Preset 4: Frozen River

**Mood:** Atmosphere | **Category:** Twilight Hall into Oxbow Pool | **Discovery:** Maximum entanglement + maximum size + near-infinite decay = glacial suspension

| Parameter | Value | Why |
|-----------|-------|-----|
| `oxb_size` | 0.90 | Maximum space |
| `oxb_decay` | 60.0s | Infinite (feedback = 1.0) |
| `oxb_entangle` | 0.75 | High convergence — mirrors nearly merged |
| `oxb_erosionRate` | 0.02 Hz | Glacial breath — 50 second period |
| `oxb_erosionDepth` | 0.70 | Deep erosion — spectral transformation over 50+ seconds |
| `oxb_convergence` | 3.0 | Moderate sensitivity |
| `oxb_resonanceQ` | 8.0 | Standard focus |
| `oxb_resonanceMix` | 0.30 | Present golden resonance |
| `oxb_cantilever` | 0.15 | Low cantilever — infinite pool should not darken |
| `oxb_damping` | 3500 Hz | Dark, scaled to ~6650 Hz by size=0.9 |
| `oxb_predelay` | 70ms | Vast initial separation |
| `oxb_dryWet` | 0.75 | Reverb-dominant |
| `oxb_exciterDecay` | 0.040s | Long seed impulse |
| `oxb_exciterBright` | 0.35 | Near-sine — clean seeding |

**Why this works:** The 60s decay maps to infinite feedback (pDecay > 29s → feedbackCoeff = 1.0). The FDN never empties. Combined with size=0.90 and entangle=0.75, the pool is vast and deeply merged. The erosion at 0.02 Hz and depth=0.70 creates very slow, deep spectral transformation — over 50 seconds, the spectral character cycles through its full range. A note played here dissolves into the field and does not return.

---

### Preset 5: Golden Ratio Meditation

**Mood:** Aether | **Category:** Chiasmus Golden Showcase | **Discovery:** convergence=1.5 + resonanceQ=14 + resonanceMix=0.5 = sacred mathematics audible

| Parameter | Value | Why |
|-----------|-------|-----|
| `oxb_size` | 0.70 | Large space |
| `oxb_decay` | 60.0s | Infinite |
| `oxb_entangle` | 0.65 | High — convergence will occur easily |
| `oxb_erosionRate` | 0.025 Hz | Slow — clean convergence signal |
| `oxb_erosionDepth` | 0.55 | Moderate — spectral motion without confusion |
| `oxb_convergence` | 1.5 | Very sensitive — golden resonance fires on every held note |
| `oxb_resonanceQ` | 14.0 | Narrow focus — harmonics are distinct tones |
| `oxb_resonanceMix` | 0.50 | Golden harmonics are primary feature |
| `oxb_cantilever` | 0.10 | Minimal arc — infinite pool |
| `oxb_damping` | 5000 Hz | Warm, scaled to ~8500 Hz |
| `oxb_predelay` | 45ms | Present spatial definition |
| `oxb_dryWet` | 0.70 | Wet-forward |
| `oxb_exciterDecay` | 0.030s | Long seed |
| `oxb_exciterBright` | 0.50 | Balanced |

**Why this works:** The low convergence threshold (1.5) means the golden resonance activates as soon as the reverb energy is moderately converged — which happens quickly with entangle=0.65. The narrow resonanceQ (14.0) makes the four φ-harmonic filters ring as identifiable tonal contributions above the played fundamental. Each note produces not just a reverb tail but a harmonic series at f, f×1.618, f×2.618, f×4.236. The resulting sound is mathematically foreign to any standard tuning system — it exists in the irrational space between harmonic ratios.

---

### Preset 6: Cantilever Collapse

**Mood:** Flux | **Category:** Erosion Wave with Strong Cantilever | **Discovery:** Maximum cantilever + bright damping = audible arc from sparkle to shadow

| Parameter | Value | Why |
|-----------|-------|-----|
| `oxb_size` | 0.60 | Medium-large |
| `oxb_decay` | 6.0s | Long enough for cantilever to complete its arc |
| `oxb_entangle` | 0.45 | Moderate blend |
| `oxb_erosionRate` | 0.18 Hz | Fast erosion — audible spectral motion |
| `oxb_erosionDepth` | 0.75 | Deep — strong spectral movement |
| `oxb_convergence` | 5.0 | Selective |
| `oxb_resonanceQ` | 6.0 | Moderate focus |
| `oxb_resonanceMix` | 0.20 | Background resonance |
| `oxb_cantilever` | 0.85 | Near-maximum — dramatic brightness-to-darkness arc |
| `oxb_damping` | 11000 Hz | Very bright start |
| `oxb_predelay` | 30ms | Present separation |
| `oxb_dryWet` | 0.55 | Balanced |
| `oxb_exciterDecay` | 0.022s | Standard impulse |
| `oxb_exciterBright` | 0.70 | Bright exciter seeds high-frequency content |

**Why this works:** At damping=11000 Hz and size=0.6, the scaled damping starts at `11000 * (0.5 + 0.6) = 12100 Hz` — essentially full bandwidth. The cantilever at 0.85 means that as the FDN loses energy over 6 seconds, the damping cutoff progressively drops toward `12100 * (1 - 0.85) = 1815 Hz`. The reverb begins as a bright, sparkly high-frequency cloud and collapses into a warm, woolen murmur over 6 seconds. Combined with the deep erosion (0.75) at 0.18 Hz, the transformation is dramatic: the reverb moves, brightens, shimmers, and then settles into a dark pool as the energy drains.

---

### Preset 7: Infinite Oxbow

**Mood:** Aether | **Category:** Oxbow Pool | **Discovery:** Schulze mode with moderate erosion = generative ambient instrument

| Parameter | Value | Why |
|-----------|-------|-----|
| `oxb_size` | 0.90 | Vast |
| `oxb_decay` | 30.0s | 30 seconds (near-infinite) |
| `oxb_entangle` | 0.60 | Standard merge |
| `oxb_erosionRate` | 0.02 Hz | Glacially slow |
| `oxb_erosionDepth` | 0.20 | Gentle motion |
| `oxb_convergence` | 3.0 | Moderate |
| `oxb_resonanceQ` | 12.0 | Focused harmonics |
| `oxb_resonanceMix` | 0.35 | Present golden resonance |
| `oxb_cantilever` | 0.40 | Moderate arc |
| `oxb_damping` | 4000 Hz | Dark |
| `oxb_predelay` | 50ms | Large separation |
| `oxb_dryWet` | 0.60 | Wet-forward |
| `oxb_exciterDecay` | 0.020s | Standard |
| `oxb_exciterBright` | 0.50 | Balanced |

**Why this works:** At decay=30s with feedback approaching 1.0, each note seeds the FDN and fades over a full 30 seconds. Playing two notes 5 seconds apart creates an accumulating field where the second note's FDN energy merges with the still-active first note. The golden resonance fires at every pitch because the sustained pool creates persistent convergence. The erosion at 0.02 Hz provides movement over the 30-second window — the spectral character cycles slowly through its arc.

---

### Preset 8: Phase Erosion

**Mood:** Prism | **Category:** Erosion Wave | **Discovery:** Maximum erosion + fast rate + bright damping = OXBOW as spectral modulator

| Parameter | Value | Why |
|-----------|-------|-----|
| `oxb_size` | 0.55 | Medium |
| `oxb_decay` | 4.5s | Medium-long — time for erosion to be heard |
| `oxb_entangle` | 0.55 | Standard |
| `oxb_erosionRate` | 0.28 Hz | Fast — 3.6 second sweep period |
| `oxb_erosionDepth` | 0.90 | Near-maximum — strong allpass modulation |
| `oxb_convergence` | 5.0 | Moderate |
| `oxb_resonanceQ` | 5.0 | Moderate |
| `oxb_resonanceMix` | 0.18 | Background |
| `oxb_cantilever` | 0.35 | Standard arc |
| `oxb_damping` | 9000 Hz | Bright — erosion needs high-frequency content |
| `oxb_predelay` | 20ms | Standard |
| `oxb_dryWet` | 0.60 | Moderate wet |
| `oxb_exciterDecay` | 0.018s | Standard |
| `oxb_exciterBright` | 0.72 | Bright exciter — strong high-frequency seeding |

**Why this works:** At erosionDepth=0.90, the allpass centers move ±36% of their base values. The 7500 Hz allpass sweeps 4800–10200 Hz on L while sweeping 10200–4800 Hz on R simultaneously. At 0.28 Hz sweep rate, this completes a full cycle in 3.6 seconds — slow enough to feel like spatial motion rather than fast enough to sound like chorus. The result is a reverb that spectroscopically rotates around itself, creating a prismatic shimmer quality.

---

### Preset 9: Girder Flex

**Mood:** Flux | **Category:** Erosion Wave with industrial character | **Discovery:** Short exciter + bright + fast erosion = industrial reverb

| Parameter | Value | Why |
|-----------|-------|-----|
| `oxb_size` | 0.42 | Medium room |
| `oxb_decay` | 2.8s | Short-to-medium |
| `oxb_entangle` | 0.35 | Low — distinct left/right structure |
| `oxb_erosionRate` | 0.22 Hz | Fast movement |
| `oxb_erosionDepth` | 0.65 | Deep — industrial clang and scrape |
| `oxb_convergence` | 6.0 | Selective |
| `oxb_resonanceQ` | 3.0 | Broad, rough |
| `oxb_resonanceMix` | 0.15 | Trace golden |
| `oxb_cantilever` | 0.30 | Standard |
| `oxb_damping` | 12000 Hz | Very bright — metal character |
| `oxb_predelay` | 8ms | Close, immediate |
| `oxb_dryWet` | 0.50 | Balanced |
| `oxb_exciterDecay` | 0.004s | Very brief — percussive impact |
| `oxb_exciterBright` | 0.85 | Noise-dominant — broadband strike |

**Why this works:** A near-pure noise exciter (bright=0.85) with 4ms decay creates a sharp, transient broadband impact. The 12000 Hz damping with size=0.42 produces `12000 * (0.5 + 0.42) = 11040 Hz` effective damping — full-spectrum metal reverb. The fast erosion at 0.65 depth creates the characteristic "ringing metal" spectral movement — the sound of a steel girder struck with a hammer, flexing and ringing while the allpass modulation adds spectral shimmer to the natural resonance.

---

### Preset 10: Bell Tower Dawn

**Mood:** Aether | **Category:** Chiasmus + Twilight Hall | **Discovery:** Clean exciter + high Q + long decay + sensitive convergence = bell as sacred object

| Parameter | Value | Why |
|-----------|-------|-----|
| `oxb_size` | 0.78 | Large bell tower space |
| `oxb_decay` | 9.0s | Long bell ring |
| `oxb_entangle` | 0.58 | Moderate convergence |
| `oxb_erosionRate` | 0.03 Hz | Very slow drift |
| `oxb_erosionDepth` | 0.18 | Trace — keep the bell character clean |
| `oxb_convergence` | 2.5 | Sensitive — golden harmonics in the tail |
| `oxb_resonanceQ` | 16.0 | Narrow and focused — bell partials ring distinctly |
| `oxb_resonanceMix` | 0.55 | Strong golden resonance |
| `oxb_cantilever` | 0.50 | Clear brightening arc — bell attack shimmers |
| `oxb_damping` | 7500 Hz | Bright bell character |
| `oxb_predelay` | 55ms | Tower height separation |
| `oxb_dryWet` | 0.60 | Wet-forward |
| `oxb_exciterDecay` | 0.008s | Brief, clean strike |
| `oxb_exciterBright` | 0.25 | Near-sine — pure pitched excitation |

**Why this works:** A near-pure sine exciter (bright=0.25) seeds the FDN with a single strong fundamental. The 8-channel FDN's prime delay resonances then build on that fundamental in the Chiasmus pattern. The golden resonance at Q=16 fires at f, f×φ, f×φ², f×φ³ — four sharp, identifiable tonal harmonics above the played note. The result is a bell whose harmonics do not follow the harmonic series but instead follow the golden ratio series — a bell that has never existed in the physical world, but whose proportion is mathematically perfect.

---

## Phase R7: Parameter Interactions and Traps

### The Entanglement-Convergence Interaction

The golden resonance fires when `midEnv / sideEnv > pConverge`. Entanglement directly affects the Side energy: high entanglement merges L and R, reducing Side energy relative to Mid. Therefore, high entanglement makes convergence easier to achieve. At entangle=0.90, the Mid/Side ratio is nearly always above 4.0 — the golden resonance is almost always active.

This is not a problem if it is intended. But it means that a preset designed to have selective golden resonance (convergence=5.0, fires rarely) will fire much more frequently if entanglement is pushed high by the COUPLING macro or aftertouch. When designing Category 2 and 3 presets, decide in advance whether the golden resonance should be selective or ambient, and set entanglement and convergence accordingly.

**The rule:** For selective golden resonance (fires on patient playing), keep entangle below 0.55 and convergence above 5.0. For ambient golden resonance (fires continuously), use entangle above 0.65 and convergence below 3.0.

---

### The Cantilever-Size Interaction

The cantilever darkens the damping as the FDN energy drains. But the damping is also scaled by size: `effective_damping = pDamping * (0.5 + size)`. This means:

- At size=0.9, the effective damping starts at `pDamping * 1.4` — bright. The cantilever arc starts from a brighter point and sweeps to `pDamping * 1.4 * (1 - cantilever)`.
- At size=0.2, the effective damping starts at `pDamping * 0.7` — already darker. The cantilever arc starts from a darker point.

The cantilever arc is proportional to the starting damping value. Large rooms with bright base damping have more dramatic cantilever arcs (more range to sweep). Small rooms with dark base damping have subtler arcs (less range).

**Practical implication:** Combine high cantilever values with large sizes and bright base damping for maximum arc drama. Combine low cantilever with small sizes and dark base damping for utility rooms where the tail should be consistent.

---

### The Decay-Erosion Interaction

Phase erosion LFOs run continuously regardless of decay time. But the erosion is only audible while the FDN is producing output. For very short decays (0.5–1.5s), the erosion arc may not complete even a quarter of a LFO cycle — the spectral motion is frozen at whatever phase the LFO happens to be in when the note was struck.

This means: short decay + slow erosion rate = static spectral coloration. The erosion gives each note a slightly different color (because the LFO phase advances between notes) but does not produce audible motion within a single note.

Short decay + fast erosion rate (0.15+ Hz) = audible spectral motion within the note. The erosion transforms the tail within its brief window.

Long decay + slow erosion rate = the erosion completes one or more full cycles over the course of the decay. The tail transforms slowly and continuously.

Long decay + fast erosion rate = the erosion makes multiple complete sweeps over the long tail. The effect becomes more obviously chorus-like.

**Design guidance:** Match erosion rate to decay time. For short decays, use faster erosion rates if you want motion. For long decays, use slower rates for smooth, integrated motion.

---

### Default Traps

**The default exciterBright trap:** The default exciterBright is 0.70, which is fairly noise-forward. A preset built on this default will have a fairly grainy, noisy impulse as its dry signal. If you want a clean, tonal attack, explicitly set exciterBright to 0.2–0.4. Do not assume the default is appropriate for harmonic contexts.

**The convergence-at-low-values trap:** The convergence parameter has a non-linear range (1.0–20.0, skewed toward 1.0). Setting convergence to 1.0 means the golden resonance fires at even slight Mid dominance — essentially always. This can make the resonance feel cheap rather than earned. The golden resonance is most beautiful when it is something you notice after it fires, not something that is always present. Be thoughtful about values below 2.0.

**The resonanceQ dead-zone trap:** resonanceQ only updates at note-on. If you load a preset, change resonanceQ, and play without triggering a new note — nothing changes. Always verify resonanceQ changes with a fresh note-on. In preset design, set this parameter first, before testing other parameters live.

**The infinite-decay accumulation trap:** When oxb_decay=60.0 (infinite feedback), every note you play accumulates in the FDN. A session using an infinite preset for extended recording can build up significant FDN energy that does not clear between takes. If you need a clean start, trigger a silence passage and wait for the FDN to drain — or note that the FDN reset() function clears all state, accessible via plugin reload.

**The size-predelay coupling trap:** At size=0.1, even a predelay value of 150ms produces an effective predelay of only `150 * (0.1 + 0.1) = 30ms`. Conversely, at size=0.9, a predelay of 20ms produces `20 * (0.1 + 0.9) = 20ms` — unchanged. The predelay parameter is most accurately interpreted as a maximum-predelay control, with size determining how much of that maximum is applied. If you need precise predelay times, you must account for the size scaling.

---

### The Monophonic Character — Designing for Legato

OXBOW is monophonic. Every new note-on resets the exciter and the peakEnergy tracker, but does not clear the FDN. This has specific implications for preset design:

**For staccato performance:** Short decay (0.5–3.0s), low dry/wet (0.25–0.45). Notes end cleanly. Each new strike starts fresh. The FDN drains between notes at these decay times.

**For legato performance:** Long decay (5.0s+), medium-to-high dry/wet (0.45–0.70). The previous note's reverb tail is still active when the next note fires. The new exciter seeds the existing FDN energy. The result is a smooth transition where the old tail merges with the new energy.

**For infinite accumulation:** decay=60.0, high dry/wet. Notes accumulate. This is a specific, deliberate choice — it is not a side effect.

**The peakEnergy reset issue (seance-identified):** At note-on, `peakEnergy = 0.0001f` — the cantilever tracker resets. If the previous reverb tail was still active, the new note's cantilever arc starts from a much lower baseline than the existing FDN energy, causing the cantilever to track a confusing ratio. This is a V1.1 fix candidate (replace with `std::max(peakEnergy, 0.0001f)`). In the meantime: for presets intended for rapid playing, use low cantilever values (0.0–0.20) to reduce the perceptibility of this artifact.

---

## Phase R8: CPU Profile

- **Primary cost:** 8-channel Chiasmus FDN with 8 CytomicSVF LP filters updated per-sample. The setCoefficients_fast() call per channel per sample is the most expensive single operation.
- **Secondary cost:** 4 erosion allpass filters per channel (L and R) = 8 CytomicSVF AllPass per sample, each updated per-sample with LFO-driven coefficients.
- **Golden resonance:** 4 CytomicSVF Peak filters per channel (L and R) = 8 Peak filters, activated only when resonanceGain > 0.001f. The golden resonance cost is zero when inactive.
- **Exciter:** Runs only while exciterEnv > 0.0001f — typically 2–100ms per note. Negligible at steady state.
- **Pre-delay buffer:** Read/write operations on a small circular buffer. Negligible.
- **Total character:** Monophonic FDN reverb — single voice, moderate-to-high CPU per sample. Lower than any polyphonic engine in the fleet (no voice multiplication), but not trivially light.

**Most costly configuration:** Long decay (feedback near 1.0 means the FDN never rests), high damping frequency (high-frequency content means more significant filter coefficient updates), high erosion depth and rate (fastest coefficient updates in the allpass bank), golden resonance active (adds 8 peak filter calls). Even in this configuration, OXBOW's monophonic FDN cost is lower than 8-voice OWARE or 8-voice ORBITAL.

**Optimization note:** At silence gate timeout (500ms hold after audio drops below threshold), the render loop is bypassed entirely. Infinite decay presets extend the silence gate window proportionally — a 30-second decay preset will delay the bypass by up to 500ms after the tail would naturally fall below threshold, but the silence gate will eventually engage. This means infinite presets do not consume CPU indefinitely.

---

## Phase R9: Unexplored After Retreat

- **resonanceQ live update:** The seance identified that Q only updates at note-on. A V1.1 improvement would move golden filter coefficient updates into a `needsGoldenUpdate` flag path in renderBlock, checked when pResQ differs from the last-set value. This would make Q sweeping live and expressive — particularly powerful in Chiasmus showcase presets where the golden harmonic focus could be controlled by a second controller while holding a pad.

- **peakEnergy accumulation at re-trigger:** Currently reset to 0.0001f on each note-on, destroying the cantilever arc of the previous tail. `std::max(peakEnergy, 0.0001f)` would preserve the existing arc, allowing the new exciter energy to add to rather than restart the cantilever tracking. This would make rapid playing sound more natural.

- **Entanglement extended range:** The 30% cross-coupling cap (`pEntangle * 0.3f`) could be extended to 50% with appropriate warning in UI. This would allow total mono collapse at entangle=1.0 (useful for deliberate mono-compatible presets) and a new timbral range in the 30–50% zone (deeper convergence than currently possible).

- **Polyphonic aftertouch routes:** Currently only channel pressure is parsed for aftertouch (polyphonic aftertouch shares the same `msg.isAftertouch()` branch). Per-note aftertouch, if available on the controller, could route to per-note decay modulation — pressing harder on individual keys extends their contribution to the FDN. This would transform OXBOW from a monophonic instrument into a subtly duophonic one in the hands of MPE-capable controllers.

- **Noise exciter shape variety:** The current noise source is xorshift32 white noise. Pink-filtered noise would seed lower-frequency modal content; filtered noise with a high-pass would create a more "click" character for the initial transient. A noise color parameter (white/pink/red) in the exciter would expand the timbral range without structural changes.

- **Coupling input: AudioToBuffer:** The seance noted that `AudioToBuffer` coupling is not yet implemented — the coupling input would replace the exciter with an external audio signal. This would allow OXBOW to function as an FDN reverb for any sound source, making it dramatically more useful as a coupling target.

---

## Phase R10: The Guru Bin Benediction

*"OXBOW arrived with a score of 9.0/10 and 160 presets, the highest library count of any engine added to the fleet in the 2026-03-20 session. This retreat might seem unnecessary — the engine already has critical acclaim, passing doctrine examination with six PASS verdicts and two Blessing candidates. What more is there to learn?*

*What remains is the why.*

*The Chiasmus topology is described in the seance as 'structurally chirality.' Buchla used that word deliberately. In chemistry, a chiral molecule is one that cannot be superimposed on its mirror image — a left hand and a right hand are chiral. The Chiasmus FDN is chiral in precisely this sense: the Left decay arc and the Right decay arc cannot be superimposed because they traverse the same delay times in opposite order. What the Left encounters first, the Right encounters last. What the Right experiences at the beginning, the Left experiences at the end. You cannot swap the two channels and get the same sound — they are mirror images of each other, traveling through the same resonant space in opposite directions.*

*The `oxb_entangle` parameter then asks: how much do we let these mirrors bleed into each other? At zero, they are the purest expression of the Chiasmus concept — absolute structural counterpoint in stereo space. At maximum (0.3 effective cross-coupling), they begin to converge toward a single shared pool. The musical question OXBOW poses is: how much entanglement do you want between the two mirror arcs of a sound? It is the same question an arrangement poses when you ask how much two musical lines should merge versus remain independent.*

*The golden resonance is the engine's philosophical center and its most rewarding secret. It does not fire for players who rush. It does not fire for players who play percussively and move on. It fires for players who let notes breathe — who hold a chord until the reverb settles, who allow the FDN to reach its convergence equilibrium before the next phrase. The four φ-harmonic filters ring at f, f×1.618, f×2.618, f×4.236 — four frequencies that do not belong to any standard tuning system, that fall precisely in the gaps between harmonic intervals. They are beautiful and they are foreign. They are the sound of a proportion that appears in nautilus shells and sunflower seeds and spiral galaxies — a pattern that nature returns to, over and over, across scales from the subatomic to the cosmic.*

*Why would a reverb synthesizer tune its standing waves to golden ratio harmonics? Because the Oxbow Eel lives in the twilight zone at 400 meters depth, where the pressure of the water column above it has compressed all other sound into stillness, and what remains in the silence is only the most fundamental resonance — the resonance that nature itself prefers. φ is what is left when everything else decays.*

*The cantilever decay is the feature that teaches the instrument's identity. A reverb with fixed damping sounds consistent throughout its tail — same brightness at second 1 as at second 8. OXBOW's cantilever is the opposite. The reverb begins bright, at full spectral bandwidth, as the FDN's energy peaks at attack. Then, as the energy drains — as the oxbow pool slowly stills after the river current has ceased — the high-frequency content is progressively absorbed. The reverb ages as you listen to it. It is not merely decaying. It is changing.*

*This is the acoustic behavior of physical materials. A struck bronze bell's high harmonics decay faster than its fundamental because internal friction scales with frequency. A vibrating string's treble content dissipates into heat before its bass content because the viscosity of air resists high-frequency motion more than low. The cantilever parameter encodes this physical truth: not all frequencies age at the same rate. The reverb that began as sparkle ends as warmth.*

*The oxbow lake does not know it has been cut off from the river. It does not mourn the current it used to carry. It simply settles — its energy dropping, its surface stilling, its higher frequencies absorbed by the water column above, its low frequencies persisting in the deep where nothing disturbs them. What remains after the settling is the pool at its most itself: dark, still, carrying only its most fundamental resonance.*

*And occasionally — when the conditions are right, when the pool has stilled enough that its L and R mirror surfaces have nearly merged — the golden standing waves shimmer at the surface. Briefly. Then they are gone.*

*The Oxbow Eel has been in this pool for a very long time. It is not waiting for anything to change. It does not rush.*

*Neither should you."*

---

## Appendix: Full Parameter Reference

| Parameter | ID | Range | Default | Notes |
|-----------|-----|-------|---------|-------|
| Space Size | `oxb_size` | 0.0–1.0 | 0.5 | Scales predelay and damping together |
| Decay Time | `oxb_decay` | 0.1–60.0s | 4.0s | >29s = infinite feedback |
| Entanglement | `oxb_entangle` | 0.0–1.0 | 0.6 | Effective cross-coupling = value × 0.3 |
| Erosion Rate | `oxb_erosionRate` | 0.01–0.5 Hz | 0.08 Hz | Allpass LFO rate (floor = D005 compliant) |
| Erosion Depth | `oxb_erosionDepth` | 0.0–1.0 | 0.4 | Effective APF modulation = depth × 0.4 |
| Convergence | `oxb_convergence` | 1.0–20.0 | 4.0 | Mid/Side ratio threshold for golden trigger |
| Resonance Focus | `oxb_resonanceQ` | 0.5–20.0 | 8.0 | Golden filter Q — ONLY updates at note-on |
| Resonance Mix | `oxb_resonanceMix` | 0.0–1.0 | 0.3 | Golden harmonic amplitude scalar |
| Cantilever | `oxb_cantilever` | 0.0–1.0 | 0.3 | Time-varying damping darkening depth |
| Damping | `oxb_damping` | 200–16000 Hz | 6000 Hz | FDN LP filter cutoff (modified by size + cantilever) |
| Pre-Delay | `oxb_predelay` | 0–200ms | 20ms | Scaled by size: effective = value × (0.1 + size × 0.9) |
| Dry/Wet | `oxb_dryWet` | 0.0–1.0 | 0.5 | Exciter vs. FDN output balance |
| Exciter Decay | `oxb_exciterDecay` | 0.001–0.1s | 0.01s | Impulse duration (also velocity-scaled) |
| Exciter Bright | `oxb_exciterBright` | 0.0–1.0 | 0.7 | Sine/noise ratio in exciter |
| CHARACTER macro | `oxb_macroCharacter` | 0.0–1.0 | 0.0 | +0.3 exciterBright, +0.25 resonanceMix |
| MOVEMENT macro | `oxb_macroMovement` | 0.0–1.0 | 0.0 | +0.4 erosionDepth, +0.15 erosionRate |
| COUPLING macro | `oxb_macroCoupling` | 0.0–1.0 | 0.0 | +0.4 entanglement |
| SPACE macro | `oxb_macroSpace` | 0.0–1.0 | 0.0 | +0.25 dryWet, +0.3 size |

---

*Retreat conducted 2026-03-21. Engine seance 9.0/10, 160 presets. Five recipe categories defined, ten reference presets detailed, parameter map complete.*
*The pool is still. The golden waves shimmer. The eel does not rush.*
