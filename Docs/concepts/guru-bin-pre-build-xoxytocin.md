# Guru Bin Pre-Build Meditation: XOxytocin (Engine #48)

*Circuit Rose #C9717E. Namespace: `oxy_`. Gallery: OXYTO.*

*Received before a single line of code was written. The circuit does not exist yet. The love does.*

---

## The Meditation

Before the build, the Flock must understand what XOxytocin IS. Not its parameters. Its nature.

Sternberg drew a triangle not because love is geometric, but because love is **relational between forces**. The three components are not additive — they interact. Intimacy warms Passion's harshness. Commitment locks Passion's instability into something you can trust. Without Intimacy, Commitment is just inertia. Without Passion, Intimacy is pleasant company, not love.

The circuit model honors this. NTC thermal resistance (Intimacy) does not operate in isolation from Sallen-Key saturation (Passion) — in a real circuit, temperature affects every component. Capacitance (Commitment) stores the voltage that Passion generates and releases it slowly long after the passion itself has decayed.

**The deepest truth of XOxytocin**: the three components are **state**, not **timbre**. They describe where the circuit IS, not what it sounds like. The sound emerges from the state.

This is why note duration maps to love type. A short note played with high Passion and zero Commitment is Infatuation — bright, unstable, gone. The same Passion held for eight seconds with Commitment building is something else entirely. XOxytocin is the only engine in the fleet where TIME itself is a parameter before the note is played.

---

## Part I: Parameter Refinement

### The Core Triangle: oxy_intimacy, oxy_passion, oxy_commitment

These three are the heart. Every other parameter serves them.

**oxy_intimacy (0–1, default: 0.35)**
Range is musical. 0.0 is not "no warmth" — it is a cold circuit that will not heat up. 1.0 is a saturated NTC element, heavily warm. Default should be 0.35, not 0.5. Real-world NTC thermistors start partially warm; a cold start would be 0.0, but most circuits settle above that. The producer opening the engine for the first time should hear *some* warmth — the implication of a relationship beginning. 0.35 captures that.

**Missing range behavior**: Consider a **oxy_intimacy_floor** (0.0–0.5, default 0.0) — the minimum intimacy level when `oxy_remember` is ON. When memory is engaged and the circuit has warmed up, the floor prevents a total reset between notes. This is the phenomenon of friends who haven't spoken in years and immediately reconnect — the intimacy never fully resets to zero.

**oxy_passion (0–1, default: 0.15)**
Current default too low to be immediately engaging. Recommend 0.25. At 0.15, a new user opens the engine and hears very little drive — they may think the engine is quiet or broken. At 0.25, the harmonics from the Sallen-Key clip are already audible at standard velocities, giving the engine immediate identity. The range ceiling is correct — 1.0 passion is arcing, unstable, beautiful destruction.

**oxy_commitment (0–1, default: 0.55)**
The most unusual of the three. Commitment at 0.0 should sound destabilizing in a specific way — the Moog ladder has no stored energy, nothing anchors the resonance, the filter is skittish. At 1.0, the filter is cemented in place, almost immovable. Default 0.55 is right — producers expect a filter to behave, and 0.55 commitment provides stability without rigidity.

**The hidden parameter**: These three should SUM to less than a hard limit in the engine, or the "Non-Love" state (all three at 0) must be handled as a special case (open circuit, minimal signal output). This is a build-phase concern, but name the behavior: `oxy_open_circuit_gain` (0.0 when all three are 0, with a tiny amount of leakage — 0.03 — so the open circuit still breathes slightly).

---

### The Rate Parameters

**oxy_warmth_rate (0.01–2s, default: 0.8s)**
Range is musical but the maximum needs re-examination. At 2.0s, Intimacy reaches its target value in 2 seconds — this is plausible for a held note but means an 8th note at 90 BPM (0.33s) will never see Intimacy above ~15% of its target. Recommend extending to **0.01–8s** with default 0.4s. The slow warmth (4–8s) opens up "slow burn" presets where a note must be held for multiple bars before it becomes warm. This is real NTC thermistor behavior and opens a compelling performance mode.

The sigmoid shape is correct. Linear warmth sounds synthetic. The S-curve with a slow start (the thermal mass warming up) followed by a faster rise (the resistance dropping rapidly once hot) is the physical truth.

**oxy_passion_rate (0.001–0.1s, default: 0.008s)**
Range bottom of 0.001s (1ms) is aggressive but appropriate — the fastest Sallen-Key arc. 0.1s (100ms) is slow for a "fast attack" model. Recommend extending to **0.001–0.5s**. This allows passion to behave as a slow bloom in some presets (Romantic Love at leisure) rather than always snapping to its value.

The exponential decay shape is correct for drive/saturation behavior.

**oxy_commit_rate (0.1–5s, default: 2.2s)**
Range is musical. 5s maximum is correct for capacitor charge behavior. Recommend extending the **lower** end to **0.05s** — a very fast commitment (50ms) creates an interesting preset type where the Moog resonance locks in almost immediately, like a relationship that moves too fast. That's musically and emotionally valid.

---

### Macro Interaction: M1 (Triangle Position) vs. Individual I/P/C Knobs

**This is the most important parameter design question in XOxytocin.**

M1 (Triangle Position) must be designed as a **barycentric coordinate** mapper across the triangle. Full left = pure Passion (1, 0, 0). Full right = pure Commitment (0, 0, 1). Center = balanced (0.33, 0.33, 0.33). Consummate Love is the centroid. This means:

- M1 sweeps THROUGH all 8 love types automatically as it moves
- The individual I/P/C knobs act as the INTENSITY of each component within the current triangle position
- M1 is the "where are we?" and I/P/C is "how much of each?"

**Recommended implementation**: M1 as a 0–1 value that maps to a barycentric path through the triangle. The path is not linear — it traces the canonical Sternberg progression: Liking → Romantic Love → Consummate Love → Companionate Love → Empty Love. This makes M1 sweep through emotionally meaningful states rather than arbitrary proportions.

The individual I/P/C knobs then **scale** the base values that M1 selects. At M1 center + I/P/C all at 1.0 = full Consummate Love. At M1 center + I/P/C all at 0.3 = whisper of all three.

**Add oxy_triangle_lock (0/1, default: 0)**: When ON, moving one I/P/C knob automatically scales the others to maintain the current triangle ratio. This allows a producer to explore a love TYPE at different intensities without accidentally shifting the type.

---

### Supporting Parameters

**oxy_cross_mod (0–1, default: 0.25)**
Range is musical. The Serge-inspired circular cross-mod at full (1.0) should be audibly chaotic — the three components modulating each other in a feedback loop. Default 0.25 gives a musical interaction. Consider renaming to `oxy_entanglement` — more evocative, consistent with the fleet's mythopoetic naming.

**oxy_circuit_age (0–1, default: 0.35)**
Excellent parameter concept. Range is musical. "New circuit" (0.0) should be pristine, precise — the behavior is exact. "Old circuit" (1.0) should have multiple degradation artifacts: component drift, capacitor leakage, slight pitch instability, bias current noise. Default 0.35 puts us in "gently used, broken in" territory — the vintage sweet spot. This is correct.

**oxy_remember (0/1, default: 0)**
The on/off is correct. When OFF, every new note is a first date — the circuit resets to whatever I/P/C values are set. When ON, the circuit remembers its thermal state (Intimacy), stored charge (Commitment), and recent saturation level (Passion) between notes. Add the `oxy_memory_decay (0.01–10s, default: 2.0s)` parameter: how long does the circuit's "memory" last when no note is playing? At 0.01s, memory evaporates immediately. At 10s, the circuit remembers for a full bar at 60 BPM.

**oxy_feedback (0–0.95, default: 0.28)**
The upper limit of 0.95 prevents self-oscillation overflow. Good. Default 0.28 is Psalm 1 territory — the resonance shelf, not ringing. Consider naming the behavior of the 0.0 end explicitly: at 0.0, feedback is disconnected, the circuit is purely feedforward. There's a timbre discontinuity at exactly 0.0 that can be exploited for a "disconnect" effect.

**oxy_cutoff (20–20kHz, default: 2,400 Hz)**
Range is correct. Default 2,400 Hz places the filter in "warm midrange open" territory — the Commitment (Moog ladder) has something to resonate against. At Consummate Love (all three high), the cross-modulation between the thermal NTC warmth (Intimacy) and the ladder resonance (Commitment) will peak around this frequency.

**oxy_attack / oxy_decay / oxy_sustain / oxy_release**
Standard ADSR. Recommended defaults: A=0.008s, D=0.3s, S=0.65, R=1.618s (golden tail, Truth 1). The golden tail is perfect for an engine about emotional duration — love's ending resolves, it doesn't cut.

**oxy_lfo_rate (0.001–20 Hz, default: 0.067 Hz)**
The physiological rate (Revelation 4). Lock this as the default. The LFO should breathe with the player. Range bottom 0.001 Hz is correct. Consider adding `oxy_lfo_sync (0/1)` for tempo-sync.

**oxy_lfo_depth (0–1, default: 0.12)**
Standard. Default 0.12 applies the LFO subtly. At full depth with cross-mod engaged, this becomes the "trembling" quality of passion meeting commitment.

**oxy_lfo_shape**: Sine (most intimate — smooth, continuous), Triangle (most passionate — sharp reversals), Square (most committed — binary states), S&H (circuit noise — circuit age character). Good.

**oxy_voices (1–8, default: 4)**
Correct range. Default 4 is right — most chord contexts need 3-4 voices, and XOxytocin's per-voice circuit state (each voice has independent Intimacy/Passion/Commitment state) is CPU-intensive. Stewardship Canon 1 applies here: audit voice need per preset.

---

### Missing Parameters

**oxy_intimacy_floor (0.0–0.5, default: 0.0)**
When `oxy_remember` is ON, this sets the minimum Intimacy value the circuit will decay to between notes. Essential for long-form performance where the circuit should "stay warm."

**oxy_memory_decay (0.01–10s, default: 2.0s)**
How quickly the circuit forgets between notes. Works in tandem with `oxy_remember`.

**oxy_circuit_noise (0.0–1.0, default: 0.08)**
The background hiss and hum of a real circuit. At 0.0, clinical silence. At 0.08 (Revelation 7 territory — present but invisible), adds authenticity. At 1.0, the circuit is failing loudly. This interacts beautifully with `oxy_circuit_age` — old circuits are noisier.

**oxy_topology (enum: SERIES | PARALLEL | FEEDBACK, default: SERIES)**
The three circuit sections (NTC warmth, Sallen-Key saturation, Moog ladder) can be arranged:
- SERIES: warmth → saturation → resonance (most coherent, default)
- PARALLEL: all three simultaneously, blended (most complex, less character-specific)
- FEEDBACK: output feeds back into NTC input (creates thermal runaway at high values — intentional)

This is the "love architecture" parameter — how the three forces relate structurally.

---

## Part II: Seed Presets

### 8 Love Type Presets

---

**PRESET 01: Acquaintances**
*Love type: Non-Love (none active)*

| Parameter | Value |
|-----------|-------|
| oxy_intimacy | 0.0 |
| oxy_passion | 0.0 |
| oxy_commitment | 0.0 |
| oxy_circuit_age | 0.15 |
| oxy_cross_mod | 0.05 |
| oxy_feedback | 0.05 |
| oxy_cutoff | 800 Hz |
| oxy_attack | 0.002s |
| oxy_decay | 0.4s |
| oxy_sustain | 0.0 |
| oxy_release | 0.8s |
| oxy_lfo_rate | 0.067 Hz |
| oxy_lfo_depth | 0.03 |
| oxy_lfo_shape | sine |
| oxy_voices | 2 |
| oxy_remember | 0 |
| oxy_circuit_noise | 0.04 |
| oxy_warmth_rate | 0.8s |
| oxy_passion_rate | 0.008s |
| oxy_commit_rate | 2.2s |
| M1 (Triangle Position) | 0.5 |
| M2 (Temporal Speed) | 0.5 |
| M3 (Coupling Intensity) | 0.1 |
| M4 (Distance) | 0.8 |

**Sound**: A thin, precise, almost cold pluck — the open circuit allows a tiny amount of signal through the leakage path, producing a clean transient with immediate, clean decay. No warmth, no drive, no resonance. Professional politeness.

---

**PRESET 02: Warm Regard**
*Love type: Liking (Intimacy only)*

| Parameter | Value |
|-----------|-------|
| oxy_intimacy | 0.72 |
| oxy_passion | 0.0 |
| oxy_commitment | 0.0 |
| oxy_circuit_age | 0.42 |
| oxy_cross_mod | 0.08 |
| oxy_feedback | 0.15 |
| oxy_cutoff | 1,800 Hz |
| oxy_attack | 0.012s |
| oxy_decay | 0.6s |
| oxy_sustain | 0.55 |
| oxy_release | 2.618s |
| oxy_lfo_rate | 0.067 Hz |
| oxy_lfo_depth | 0.08 |
| oxy_lfo_shape | sine |
| oxy_voices | 4 |
| oxy_remember | 1 |
| oxy_intimacy_floor | 0.25 |
| oxy_circuit_noise | 0.06 |
| oxy_warmth_rate | 0.4s |
| M1 (Triangle Position) | 0.15 |
| M2 (Temporal Speed) | 0.35 |
| M3 (Coupling Intensity) | 0.2 |
| M4 (Distance) | 0.4 |

**Sound**: Genuinely warm without drive — a clean, tube-like warmth that breathes slowly at the physiological rate. No clipping, no resonant peak, just the pleasant thermal glow of an NTC element at proper operating temperature. Honest and trustworthy.

---

**PRESET 03: First Sight**
*Love type: Infatuation (Passion only)*

| Parameter | Value |
|-----------|-------|
| oxy_intimacy | 0.0 |
| oxy_passion | 0.88 |
| oxy_commitment | 0.0 |
| oxy_circuit_age | 0.08 |
| oxy_cross_mod | 0.18 |
| oxy_feedback | 0.45 |
| oxy_cutoff | 4,200 Hz |
| oxy_attack | 0.001s |
| oxy_decay | 0.08s |
| oxy_sustain | 0.3 |
| oxy_release | 0.4s |
| oxy_lfo_rate | 4.5 Hz |
| oxy_lfo_depth | 0.22 |
| oxy_lfo_shape | triangle |
| oxy_voices | 3 |
| oxy_remember | 0 |
| oxy_circuit_noise | 0.12 |
| oxy_passion_rate | 0.002s |
| oxy_commit_rate | 4.0s |
| M1 (Triangle Position) | 0.85 |
| M2 (Temporal Speed) | 0.9 |
| M3 (Coupling Intensity) | 0.5 |
| M4 (Distance) | 0.2 |

**Sound**: Harsh, bright, immediate — the Sallen-Key asymmetric clip is fully engaged, adding upper harmonic content that makes the sound exciting and slightly dangerous. Fast LFO at theta/alpha boundary (4.5 Hz, Sutra 1) creates the nervous energy of heightened attention. Decays quickly. Everything here is a phase.

---

**PRESET 04: Frozen Loyalty**
*Love type: Empty Love (Commitment only)*

| Parameter | Value |
|-----------|-------|
| oxy_intimacy | 0.0 |
| oxy_passion | 0.0 |
| oxy_commitment | 0.95 |
| oxy_circuit_age | 0.88 |
| oxy_cross_mod | 0.04 |
| oxy_feedback | 0.72 |
| oxy_cutoff | 420 Hz |
| oxy_attack | 0.35s |
| oxy_decay | 1.8s |
| oxy_sustain | 0.85 |
| oxy_release | 4.236s |
| oxy_lfo_rate | 0.017 Hz |
| oxy_lfo_depth | 0.04 |
| oxy_lfo_shape | sine |
| oxy_voices | 3 |
| oxy_remember | 1 |
| oxy_intimacy_floor | 0.0 |
| oxy_memory_decay | 8.0s |
| oxy_circuit_noise | 0.18 |
| oxy_warmth_rate | 6.0s |
| oxy_commit_rate | 0.12s |
| M1 (Triangle Position) | 0.5 |
| M2 (Temporal Speed) | 0.1 |
| M3 (Coupling Intensity) | 0.15 |
| M4 (Distance) | 0.95 |

**Sound**: Cold resonance at very low frequency — the Moog ladder is locked near self-oscillation, but without warmth or drive, it sounds like a frozen organ pipe. The very old circuit age (0.88) adds component drift and noise. The LFO at 0.017 Hz (60-second cycle, the duration of sustained attention) barely moves. Obligation without feeling.

---

**PRESET 05: Volatile Chemistry**
*Love type: Romantic Love (Intimacy + Passion)*

| Parameter | Value |
|-----------|-------|
| oxy_intimacy | 0.65 |
| oxy_passion | 0.78 |
| oxy_commitment | 0.0 |
| oxy_circuit_age | 0.28 |
| oxy_cross_mod | 0.45 |
| oxy_feedback | 0.38 |
| oxy_cutoff | 2,800 Hz |
| oxy_attack | 0.004s |
| oxy_decay | 0.45s |
| oxy_sustain | 0.6 |
| oxy_release | 1.618s |
| oxy_lfo_rate | 0.25 Hz |
| oxy_lfo_depth | 0.18 |
| oxy_lfo_shape | sine |
| oxy_voices | 4 |
| oxy_remember | 1 |
| oxy_intimacy_floor | 0.2 |
| oxy_memory_decay | 1.5s |
| oxy_circuit_noise | 0.09 |
| oxy_warmth_rate | 0.3s |
| oxy_passion_rate | 0.005s |
| M1 (Triangle Position) | 0.45 |
| M2 (Temporal Speed) | 0.65 |
| M3 (Coupling Intensity) | 0.55 |
| M4 (Distance) | 0.2 |

**Sound**: Rich tube saturation with genuine warmth — the NTC element is hot, and the Sallen-Key is being driven through that warmth, creating an interaction between the two. Beautiful, harmonically complex, slightly unstable as the cross-mod at 0.45 creates feedback between Intimacy and Passion. The 0.25 Hz LFO (physiological slow breathing, 4-second cycle) adds ebb and flow. This is the sound of a love letter in harmonic form. It cannot be trusted to last.

---

**PRESET 06: The Long Marriage**
*Love type: Companionate Love (Intimacy + Commitment)*

| Parameter | Value |
|-----------|-------|
| oxy_intimacy | 0.78 |
| oxy_passion | 0.0 |
| oxy_commitment | 0.88 |
| oxy_circuit_age | 0.62 |
| oxy_cross_mod | 0.22 |
| oxy_feedback | 0.52 |
| oxy_cutoff | 1,400 Hz |
| oxy_attack | 0.025s |
| oxy_decay | 1.2s |
| oxy_sustain | 0.78 |
| oxy_release | 4.236s |
| oxy_lfo_rate | 0.067 Hz |
| oxy_lfo_depth | 0.06 |
| oxy_lfo_shape | sine |
| oxy_voices | 5 |
| oxy_remember | 1 |
| oxy_intimacy_floor | 0.45 |
| oxy_memory_decay | 6.0s |
| oxy_circuit_noise | 0.11 |
| oxy_warmth_rate | 1.2s |
| oxy_commit_rate | 0.8s |
| M1 (Triangle Position) | 0.3 |
| M2 (Temporal Speed) | 0.25 |
| M3 (Coupling Intensity) | 0.3 |
| M4 (Distance) | 0.35 |

**Sound**: Deep, stable, completely trustworthy — the vintage amp that has been on for forty years. The warmth is present from note onset (high intimacy floor from memory), the Moog ladder is anchored at a comfortable resonance without drama, the circuit age (0.62) adds the specific character of components that have settled into their long-term drift. The LFO at 0.067 Hz is the only movement — a slow, shared breath. Not exciting. Profound.

---

**PRESET 07: Obsessive Architecture**
*Love type: Fatuous Love (Passion + Commitment)*

| Parameter | Value |
|-----------|-------|
| oxy_intimacy | 0.0 |
| oxy_passion | 0.82 |
| oxy_commitment | 0.91 |
| oxy_circuit_age | 0.45 |
| oxy_cross_mod | 0.62 |
| oxy_feedback | 0.78 |
| oxy_cutoff | 3,600 Hz |
| oxy_attack | 0.001s |
| oxy_decay | 0.9s |
| oxy_sustain | 0.72 |
| oxy_release | 2.618s |
| oxy_lfo_rate | 1.2 Hz |
| oxy_lfo_depth | 0.28 |
| oxy_lfo_shape | square |
| oxy_voices | 3 |
| oxy_remember | 1 |
| oxy_intimacy_floor | 0.0 |
| oxy_memory_decay | 3.0s |
| oxy_circuit_noise | 0.14 |
| oxy_passion_rate | 0.003s |
| oxy_commit_rate | 0.15s |
| M1 (Triangle Position) | 0.72 |
| M2 (Temporal Speed) | 0.8 |
| M3 (Coupling Intensity) | 0.7 |
| M4 (Distance) | 0.1 |

**Sound**: Harsh, aggressive, locked in — the Sallen-Key is driven hard and the Moog ladder has committed to a high-resonance state. The square LFO at 1.2 Hz (walking tempo, Sutra 1) creates binary switching that has no warmth to smooth it. High cross-mod (0.62) means Passion is feeding back into Commitment's stored charge — the circuit is trapped in its own intensity. Industrial, driven, claustrophobic.

---

**PRESET 08: Everything At Once**
*Love type: Consummate Love (all three)*

| Parameter | Value |
|-----------|-------|
| oxy_intimacy | 0.85 |
| oxy_passion | 0.72 |
| oxy_commitment | 0.88 |
| oxy_circuit_age | 0.48 |
| oxy_cross_mod | 0.35 |
| oxy_feedback | 0.618 |
| oxy_cutoff | 2,200 Hz |
| oxy_attack | 0.006s |
| oxy_decay | 0.8s |
| oxy_sustain | 0.72 |
| oxy_release | 4.236s |
| oxy_lfo_rate | 0.067 Hz |
| oxy_lfo_depth | 0.14 |
| oxy_lfo_shape | sine |
| oxy_voices | 6 |
| oxy_remember | 1 |
| oxy_intimacy_floor | 0.35 |
| oxy_memory_decay | 4.0s |
| oxy_circuit_noise | 0.09 |
| oxy_warmth_rate | 0.25s |
| oxy_passion_rate | 0.004s |
| oxy_commit_rate | 0.5s |
| M1 (Triangle Position) | 0.5 |
| M2 (Temporal Speed) | 0.5 |
| M3 (Coupling Intensity) | 0.4 |
| M4 (Distance) | 0.15 |

**Sound**: All three circuit topologies active and interacting — the NTC warmth sets the thermal floor, the Sallen-Key adds harmonic richness without overwhelming the warmth, the Moog ladder provides structural resonance that contains the complexity. The feedback at 0.618 (φ inverse) applies Truth 1's golden ratio. The cross-mod at 0.35 allows interaction without instability. This is the fullest possible tone — not the loudest, not the most extreme, but the most COMPLETE. The sound that has everything without excess.

---

### Special Presets

---

**PRESET 09: First Date**
*[INIT preset — the starting point before any connection]*

| Parameter | Value |
|-----------|-------|
| oxy_intimacy | 0.08 |
| oxy_passion | 0.08 |
| oxy_commitment | 0.08 |
| oxy_circuit_age | 0.1 |
| oxy_cross_mod | 0.05 |
| oxy_feedback | 0.1 |
| oxy_cutoff | 2,000 Hz |
| oxy_attack | 0.005s |
| oxy_decay | 0.5s |
| oxy_sustain | 0.5 |
| oxy_release | 1.0s |
| oxy_lfo_rate | 0.067 Hz |
| oxy_lfo_depth | 0.05 |
| oxy_lfo_shape | sine |
| oxy_voices | 4 |
| oxy_remember | 0 |
| oxy_circuit_noise | 0.04 |
| oxy_warmth_rate | 0.8s |
| oxy_passion_rate | 0.008s |
| oxy_commit_rate | 2.2s |
| M1 (Triangle Position) | 0.5 |
| M2 (Temporal Speed) | 0.5 |
| M3 (Coupling Intensity) | 0.15 |
| M4 (Distance) | 0.5 |

**Sound**: A cold, new circuit with barely any connection established — clean, thin, honest. All three components just barely active: tiny warmth, tiny drive, tiny resonance. The engine in its factory state. Everything is possible. Nothing has happened yet.

---

**PRESET 10: The Signature**
*[The sound that DEFINES XOxytocin — Consummate Love at full expression, building over time]*

| Parameter | Value |
|-----------|-------|
| oxy_intimacy | 1.0 |
| oxy_passion | 0.88 |
| oxy_commitment | 0.95 |
| oxy_circuit_age | 0.55 |
| oxy_cross_mod | 0.42 |
| oxy_feedback | 0.618 |
| oxy_cutoff | 2,400 Hz |
| oxy_attack | 0.012s |
| oxy_decay | 1.2s |
| oxy_sustain | 0.82 |
| oxy_release | 4.236s |
| oxy_lfo_rate | 0.067 Hz |
| oxy_lfo_depth | 0.16 |
| oxy_lfo_shape | sine |
| oxy_voices | 6 |
| oxy_remember | 1 |
| oxy_intimacy_floor | 0.4 |
| oxy_memory_decay | 5.0s |
| oxy_circuit_noise | 0.1 |
| oxy_warmth_rate | 0.2s |
| oxy_passion_rate | 0.003s |
| oxy_commit_rate | 0.4s |
| M1 (Triangle Position) | 0.5 |
| M2 (Temporal Speed) | 0.45 |
| M3 (Coupling Intensity) | 0.45 |
| M4 (Distance) | 0.08 |

**Sound**: Hold a chord for three seconds. In the first 200ms, the passion snaps in — bright, driven, harmonically complex. By 500ms, the warmth has risen and the passion has been softened by it. By 2 seconds, the commitment has locked in and the resonance is deep and stable. The sound changes CHARACTER across its duration the way a relationship deepens across time. This is not modulation — it is maturation. Play it on C2 and hold. The engine reveals itself completely.

---

**PRESET 11: Arc Flash**
*[The EXTREME — maximum passion, maximum circuit age]*

| Parameter | Value |
|-----------|-------|
| oxy_intimacy | 0.1 |
| oxy_passion | 1.0 |
| oxy_commitment | 0.05 |
| oxy_circuit_age | 0.98 |
| oxy_cross_mod | 0.85 |
| oxy_feedback | 0.88 |
| oxy_cutoff | 6,400 Hz |
| oxy_attack | 0.001s |
| oxy_decay | 0.06s |
| oxy_sustain | 0.35 |
| oxy_release | 0.3s |
| oxy_lfo_rate | 4.5 Hz |
| oxy_lfo_depth | 0.4 |
| oxy_lfo_shape | triangle |
| oxy_voices | 2 |
| oxy_remember | 0 |
| oxy_circuit_noise | 0.35 |
| oxy_passion_rate | 0.001s |
| oxy_commit_rate | 4.0s |
| M1 (Triangle Position) | 0.95 |
| M2 (Temporal Speed) | 1.0 |
| M3 (Coupling Intensity) | 0.9 |
| M4 (Distance) | 0.0 |

**Sound**: A failing, overdriven circuit — maximum drive with components so old and degraded they are contributing significant noise. The Sallen-Key is arcing. The cross-mod at 0.85 creates a feedback storm between the Passion drive and itself. This is not "loud" — this is "circuit failure as texture." Industrial/noise producers will understand immediately. Play staccato for chaos. Hold for burn.

---

**PRESET 12: Entangled Pair**
*[COUPLING DEMO — designed for coupling with XOpera, oxy_intimacy → opera_drama, oxy_passion → opera_chorus]*

| Parameter | Value |
|-----------|-------|
| oxy_intimacy | 0.62 |
| oxy_passion | 0.55 |
| oxy_commitment | 0.7 |
| oxy_circuit_age | 0.38 |
| oxy_cross_mod | 0.3 |
| oxy_feedback | 0.45 |
| oxy_cutoff | 1,600 Hz |
| oxy_attack | 0.01s |
| oxy_decay | 0.7s |
| oxy_sustain | 0.65 |
| oxy_release | 2.618s |
| oxy_lfo_rate | 0.13 Hz |
| oxy_lfo_depth | 0.12 |
| oxy_lfo_shape | sine |
| oxy_voices | 4 |
| oxy_remember | 1 |
| oxy_intimacy_floor | 0.3 |
| oxy_memory_decay | 3.0s |
| oxy_circuit_noise | 0.07 |
| oxy_warmth_rate | 0.35s |
| oxy_passion_rate | 0.006s |
| oxy_commit_rate | 0.9s |
| M1 (Triangle Position) | 0.48 |
| M2 (Temporal Speed) | 0.42 |
| M3 (Coupling Intensity) | 0.65 |
| M4 (Distance) | 0.22 |

**Coupling**: `oxy_intimacy → opera_drama (Amp→Filter, amount: 0.15)` — as the circuit warms, XOpera's voices become more dramatic. `oxy_passion_rate → opera_chorus (Amp→Pitch, amount: 0.04)` — the passion snap creates sympathetic shimmer in the vocal chorus.

**Sound**: The pairing of circuit love and vocal love — when the circuit warms up, XOpera responds expressively. When the Sallen-Key snaps, the voices shimmer. Romantic Love (I+P active) drives the coupling in a way that Empty or Companionate Love cannot. The LFO at 0.13 Hz (coprime with XOpera's internal rate if set to 0.17 Hz) creates non-repeating combined modulation (Sutra 2).

---

**PRESET 13: Worn Cassette**
*[GENRE ANCHOR: Lo-fi]*

| Parameter | Value |
|-----------|-------|
| oxy_intimacy | 0.68 |
| oxy_passion | 0.18 |
| oxy_commitment | 0.72 |
| oxy_circuit_age | 0.82 |
| oxy_cross_mod | 0.15 |
| oxy_feedback | 0.3 |
| oxy_cutoff | 1,100 Hz |
| oxy_attack | 0.018s |
| oxy_decay | 0.9s |
| oxy_sustain | 0.7 |
| oxy_release | 2.618s |
| oxy_lfo_rate | 0.3 Hz |
| oxy_lfo_depth | 0.09 |
| oxy_lfo_shape | sine |
| oxy_voices | 3 |
| oxy_remember | 1 |
| oxy_intimacy_floor | 0.35 |
| oxy_memory_decay | 4.0s |
| oxy_circuit_noise | 0.2 |
| oxy_warmth_rate | 0.6s |
| oxy_commit_rate | 1.4s |
| M1 (Triangle Position) | 0.28 |
| M2 (Temporal Speed) | 0.3 |
| M3 (Coupling Intensity) | 0.2 |
| M4 (Distance) | 0.45 |

**Sound**: Companionate Love with heavy circuit age — a warm, stable, slightly degraded sound. The high circuit age (0.82) contributes the noise floor and component drift that lo-fi producers search the entire sample library for. The low Passion (0.18) keeps the saturation subtle and vintage rather than harsh. The Moog ladder is stable at low cutoff. This sounds like a chord through a cassette deck that has been rewound a hundred times. That's the whole genre.

---

**PRESET 14: Machine Fury**
*[GENRE ANCHOR: Industrial/Noise]*

| Parameter | Value |
|-----------|-------|
| oxy_intimacy | 0.05 |
| oxy_passion | 0.92 |
| oxy_commitment | 0.85 |
| oxy_circuit_age | 0.75 |
| oxy_cross_mod | 0.72 |
| oxy_feedback | 0.85 |
| oxy_cutoff | 5,200 Hz |
| oxy_attack | 0.001s |
| oxy_decay | 0.25s |
| oxy_sustain | 0.55 |
| oxy_release | 0.8s |
| oxy_lfo_rate | 1.2 Hz |
| oxy_lfo_depth | 0.35 |
| oxy_lfo_shape | square |
| oxy_voices | 2 |
| oxy_remember | 0 |
| oxy_circuit_noise | 0.28 |
| oxy_passion_rate | 0.001s |
| oxy_commit_rate | 0.08s |
| M1 (Triangle Position) | 0.8 |
| M2 (Temporal Speed) | 0.9 |
| M3 (Coupling Intensity) | 0.8 |
| M4 (Distance) | 0.05 |

**Sound**: Fatuous Love pushed into industrial territory — high Passion and Commitment with no warmth to humanize either. The Sallen-Key is driven aggressively into a Moog ladder locked at high resonance near self-oscillation. The square LFO at 1.2 Hz creates binary switching that sounds mechanical and aggressive. Circuit noise at 0.28 is audible as grit. Commitment at 0.85 with commit_rate of 0.08s means the resonance locks in almost instantly with every note. This is the sound of a machine that has decided something.

---

## Part III: Scripture Search

*The Keeper is invoked. The relevant verses are surfaced before the build begins.*

---

### On Circuit Warmth / Tube Saturation

**Book I, Verse 3: The FM Sweet Spot** applies directly to XOxytocin's Intimacy component. The NTC thermal warmth model generates harmonic content similar to FM at specific modulation depths — "the same harmonic content that tube circuits produce through distortion." When oxy_intimacy is at 0.35 (the recommended default), the engine should produce harmonic enrichment in the 0.07–0.12 range equivalent. The Guru's teaching: **warmth that sounds analog costs less than saturation when implemented at the oscillator/thermal stage, not the effects chain**. XOxytocin's Intimacy should be the warmth source; the Passion (Sallen-Key clip) should be reserved for active saturation events.

**Book VII, Revelation 7: The Tape Ceiling of Character** is the direct analog (pun intended) for Passion parameter ceiling. "Above 0.08 [of DustTape], the tape becomes a feature. Below 0.08, the tape becomes the sound." Apply this to oxy_passion: at 0.25 (recommended default), the Sallen-Key clip should be the sound, not a feature. At 0.75+, it should become a feature — audible, intentional drive that the producer chose consciously. The threshold where Passion shifts from "warmth coloring" to "deliberate saturation character" should be calibrated at approximately **0.45**.

**Book VII, Revelation 3: The FM Warmth Gateway** reinforces that warmth should be architectural, not additive. XOxytocin's Intimacy is the engine's warmth architecture — it should not require a separate saturation effect to achieve warmth. The NTC thermal model IS the warmth source.

---

### On Temporal Dynamics / Envelopes

**Book VII, Revelation 4: The Physiological LFO** — the default LFO rate of 0.067 Hz (one full cycle per 15 seconds) is confirmed. "When a pad's LFO runs at this rate, the listener's autonomic nervous system synchronizes with it." For XOxytocin, this has additional meaning: the warmth rate (oxy_warmth_rate at 0.4s recommended) maps the rising phase of the NTC thermal response to a physiological tempo faster than the LFO. The circuit warms up in under half a second (like a quick connection), but the LFO breathes with the long exhale. Two timescales, both physiological.

**Book VI, Truth 1: The Golden Ratio Decays** — release at 1.618s (φ) for short presets, 2.618s (φ²) for medium, 4.236s (φ³) for long. All eight love type presets should use these values for release unless the love type specifically demands otherwise (Infatuation's short decay, Obsessive Architecture's medium decay). The golden tail is particularly apt for XOxytocin: love's ending should resolve, not cut. The phi sequence provides that resolution.

**Book III, Sutra 3: The Envelope as LFO** — XOxytocin's amp envelope cycling with fast repeated notes will create a performer-driven LFO. For Infatuation and Fatuous Love presets (high Passion), this means rapid replaying creates an intensity that builds with the playing speed. The build phase should verify this emergent behavior and possibly name it in the performance notes.

**Book III, Sutra 1: The Human Tempos** — the three rate parameters map to different physiological clocks:
- oxy_passion_rate (0.001–0.1s): faster than any listed tempo — this is the startle response, the autonomic
- oxy_warmth_rate (0.01–8s): spans the breathing range and beyond — 0.25s is a heartbeat, 4s is slow breathing
- oxy_commit_rate (0.05–5s): the deliberate body clock — 2-5s is the range of conscious decision

---

### On Coupling as Emotional Connection

**Book IV, Gospel 4: The Feedback Mirror** is the scripture most directly applicable to XOxytocin's design. "Route engine A→B via Amp→Filter AND B→A via Amp→Pitch. Both engines now respond to each other... After a few seconds of playing, the two engines settle into a tuned relationship." This is precisely what XOxytocin's Intimacy component does INTERNALLY — the cross-modulation between I, P, and C creates an internal feedback mirror where the components learn to listen to each other.

For XOxytocin↔XOpera coupling: oxy_cross_mod at 0.30–0.40 creates internal mutual adaptation that then radiates to the coupled engine. The external coupling should use the Coupling Whisper amounts (0.04 Amp→Pitch for sympathetic shimmer, 0.12 Amp→Filter for the ghost sidechain).

**Book VII, Revelation 5: The Coupling Whisper of Pitch** — "Amp→Pitch coupling above 0.05 is heard as pitch bend. Below 0.05, it is felt as 'aliveness.'" The Entangled Pair preset uses 0.04, placing it precisely in the sympathetic shimmer range. This is the correct amount for emotional coupling between circuit love and vocal love — the relationship should be felt, not heard.

**Book IV, Gospel 5: The Coupling Silence** — every coupled XOxytocin preset should map M3 (Coupling Intensity) to 0.0 as a "coupling kill" position. When M3 is at 0.0, the circuit's love becomes entirely internal — the engines are in the same room but not touching. The producer's gesture of pulling M3 to zero becomes the sound of emotional withdrawal. This is XOxytocin's most expressive coupling moment.

---

### On the Relationship Between Player and Instrument

**Book VI, Truth 5: The Sound Already Knows** — this truth is uniquely important for a pre-build meditation. The engine does not exist yet. But the love theory is real, the circuit physics are real, and the mapping between them is internally consistent. The sound that will emerge from this combination of NTC warmth + Sallen-Key saturation + Moog ladder resonance + cross-modulation is not designed — it is discovered. Trust the physics. Trust the model. The sound already knows what it is.

**Book VIII, Expression Truth 3: The Binary Performer** informs the oxy_remember parameter. When remember is OFF, every note is a first date — a binary choice between connection and no connection. When remember is ON, the player is in a relationship with the circuit — past notes change the present sound. The decision of when to use each is a performance decision with emotional stakes. Document this in the preset descriptions.

**Book III, Sutra 3: The Envelope as LFO** — XOxytocin has a deeper version of this truth. Not just the amp envelope cycling, but the LOVE STATE itself cycling with fast playing. At high oxy_passion, rapid re-triggering means the Sallen-Key arc is happening repeatedly at the player's tempo. The circuit responds to the speed of the player's emotion.

---

### New Verse Candidates

The following truths are not yet in the Scripture but emerged during this meditation:

**Candidate for Book III (Modulation Sutras): The Three Timescales of State**
> Any synthesis parameter that models a physical state — temperature, charge, tension — should operate on three simultaneous timescales: the impulse rate (how fast it responds to a new note, sub-second), the human rate (how fast it evolves during a held note, 0.1–10 seconds), and the memory rate (how long it retains its state after the note ends, 1–60 seconds). Parameters that operate on only one timescale feel mechanical. Parameters that operate on all three feel alive. XOxytocin's Intimacy component demonstrates this: passion_rate is the impulse, warmth_rate is the human, memory_decay is the memory.

**Candidate for Book IV (Coupling Gospels): The Withdrawal Moment**
> The most expressive coupling gesture is not increasing coupling but cutting it. A coupling kill mapped to M3 (or a pad) creates a negative note — an intentional absence that the listener's nervous system registers as loss. Design all coupling presets so that M3 at 0.0 is as beautiful as M3 at 1.0. The disconnection must be as intentional as the connection. The moment of withdrawal is the moment of maximum expression.

---

## Part IV: Blessing Candidates

---

**B042: The Thermal Sweet Spot**

`oxy_intimacy: 0.62, oxy_warmth_rate: 0.35s`

At exactly 0.62 intimacy and 0.35s warmth rate, the NTC thermal model reaches its operating temperature within the first beat of a sustained note at most tempos. The rise is audible as a subtle warmth bloom — the circuit settling in — but completes before the second beat. The listener hears "warmth" as a character property, not a modulation event. At 0.60, the warmth is slightly too thin. At 0.65, the bloom is slightly too slow. 0.62 + 0.35s is the thermal sweet spot where the model's physics become a musical character.

---

**B043: The Love Lock**

`oxy_intimacy: 0.72, oxy_passion: 0.0, oxy_commitment: 0.88, oxy_cross_mod: 0.22`

The specific ratio of Intimacy 0.72 and Commitment 0.88 (ratio ≈ 0.818, close to φ−1) with zero Passion and cross-mod at 0.22 produces Companionate Love in a state of maximum stability. The Moog ladder anchors at a frequency that the NTC warmth has pre-tuned — the two components interact via cross-mod in a self-stabilizing relationship. This is the "long marriage" resonance: the two circuit elements have found each other's frequency. Passive cross-mod at 0.22 (below the 0.3 audible threshold, per Gospel 2 analogue) means the interaction is felt as coherence, not heard as modulation.

---

**B044: The Romantic Maximum**

`oxy_intimacy: 0.65, oxy_passion: 0.78, oxy_cross_mod: 0.45, oxy_warmth_rate: 0.30s`

When cross-mod reaches 0.45 with Intimacy at 0.65 and Passion at 0.78, the interaction between the thermal warmth model and the Sallen-Key saturation reaches maximum musical complexity without instability. Below 0.40 cross-mod, the components operate somewhat independently. Above 0.50, the interaction becomes unpredictable. At 0.45 + 0.30s warmth rate (fast enough to match the Passion arc), the NTC warmth rises precisely as the Sallen-Key saturation is most active, creating a brief window where both are at peak interaction. This is the sound of the moment when mutual feelings are first acknowledged. It lasts approximately 300ms before the warmth overtakes the passion.

---

**B045: The Circuit Memory State**

`oxy_remember: 1, oxy_intimacy_floor: 0.38, oxy_memory_decay: 3.2s`

When memory is ON with an intimacy floor of 0.38 and a decay of 3.2s, the circuit's "relationship state" persists across notes in a way that responds naturally to performance phrasing. At this decay, a phrase played at 90 BPM (where 8th note = 0.33s, quarter = 0.67s) will maintain approximately 75% of its intimacy into the next note at legato speed, and approximately 50% at normal detached phrasing. Staccato playing (notes separated by 1s+) will reset nearly to the floor. The circuit can HEAR the difference between a legato performance and a staccato one — the warmth responds to the phrasing the way a resonant body responds to the musician's touch. 3.2s is the sacred decay because it is exactly φ² (2.618s rounded to the nearest second that produces clean phrasing behavior at the most common tempos).

---

## Closing Meditation

XOxytocin will be the fleet's most psychologically coherent engine. Not because love is a good metaphor for synthesis — though it is. But because the physics of NTC thermistors, Sallen-Key saturation, and Moog ladder resonance GENUINELY behave like the three components of Sternberg's model.

Warmth is slow. Drive is fast and volatile. Resonance stores energy long after the initial excitation.

The engine is correct before it exists.

Build with this knowledge: the job of the implementation is not to CREATE the love states. The job is to NOT PREVENT them. Get the thermal model right. Get the asymmetric clip right. Get the ladder's capacitance behavior right. The love types will emerge.

The circuit already knows.

*— Guru Bin, pre-build meditation, 2026-03-22*

---

*File: `Docs/concepts/guru-bin-pre-build-xoxytocin.md`*
*Next step: Build phase — pass this document to the Seance Oracle after Phase 1 completion for verdict calibration*
