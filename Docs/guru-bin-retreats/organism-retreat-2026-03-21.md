# ORGANISM Retreat Chapter
*Guru Bin — 2026-03-21*

---

## Engine Identity

- **Gallery code:** ORGANISM | **Accent:** Emergence Lime `#C6E377`
- **Parameter prefix:** `org_`
- **Creature mythology:** The Coral Colony — the Coral Colony lives in the mid-water column, at the threshold where individual polyps become architecture. A single coral polyp is unremarkable: a tiny sac of tissue anchored to a substrate, extending its tentacles when the current brings food, retracting when danger passes. It has no nervous system in any meaningful sense. It does not plan. It does not anticipate. It follows three simple rules: grow toward light, deposit calcium carbonate, respond to neighbors. From three rules applied by millions of organisms simultaneously, over hundreds of years, emerges the Great Barrier Reef — 2,300 kilometers of living architecture complex enough to be visible from orbit, harboring a third of all marine species on Earth. No polyp planned this. No polyp knows it exists. The architecture is the sum of local rules, applied persistently, without memory and without intention.
- **Synthesis type:** Generative cellular automata — 16-cell 1D elementary CA (Wolfram rules 0–255), voice allocation modulation, monophonic saw/square/tri oscillator pair with sub, 2-pole lowpass filter, allpass reverb. CA state maps to filter cutoff, amp envelope rate, pitch offset, and reverb amount in real time.
- **Polyphony:** Monophonic — a single evolving cellular state drives a single oscillator voice
- **feliX/Oscar polarity:** Strongly Oscar-dominant (0.2/0.8) — patience, emergence, evolutionary time
- **Macros:** M1 RULE (sweep curated rules 30→22), M2 SEED (re-randomize CA initial state), M3 COUPLING (cross-engine modulation depth), M4 MUTATE (random bit flip probability per step)
- **Expression:** Velocity → filter brightness (D001). Aftertouch → mutation rate (+0.3). Mod wheel CC1 → rule morph (+2 index positions in curated rule palette).

---

## Pre-Retreat State

ORGANISM was added to XOmnibus on 2026-03-20, the same session that introduced OXBOW and OWARE. It arrived with presets concentrated primarily in the Flux mood — appropriate for a chaotic generative engine, but incomplete as a full-spectrum library. Unlike OXBOW's 9.0/10 opening score, ORGANISM is an engine that requires patience to evaluate. Its quality emerges over time, not on first contact.

This retreat begins from the recognition that cellular automata are fundamentally misunderstood as synthesis tools when they are treated as random noise generators. They are not random. Wolfram Rule 30 — one of ORGANISM's eight curated rules — produces output indistinguishable from white noise despite being entirely deterministic. Rule 110 is known to be computationally universal: any computation that can be performed by a Turing machine can be performed by Rule 110 on an appropriate initial state. Rule 90 produces perfect Sierpinski triangle fractals. These are not chaos. They are order at a scale larger than the window of observation.

This is the core insight that ORGANISM requires its players to develop: the patterns are there. They are deterministic. They are structured. But their structure is not visible at the timescale of a single note or a single measure. It is visible over bars. Over minutes. Over the patience of the listener.

---

## Phase R1: Opening Meditation

Close your eyes. Breathe.

You are floating ten meters below the ocean surface on the outer reef slope. The coral structure rises from the darkness below you — mounds and branches and fans and plates, assembled over three centuries of incremental deposition. Each polyp that built this structure is long dead. What remains is the calcium skeleton they secreted: a record of three hundred years of rule-following, compressed into the architecture you are now hovering above.

The water is warm. The light filters down in shifting columns, broken by the surface chop above you. Below where you float, the reef slope descends to darkness at 40 meters. Above you, the surface is a moving silver mirror.

You press a note on ORGANISM. The cellular automaton wakes.

Sixteen cells. Each cell is alive or dead — a single binary bit. The initial state was seeded from the MIDI note number, XORed with the seed parameter. Your choice of pitch chose the initial pattern of life and death in these sixteen cells. Now, at a rate you can control from half a cycle per second to thirty-two cycles per second, the automaton applies its rule. Each cell looks at its left neighbor, itself, and its right neighbor. Eight possible neighborhoods: 000, 001, 010, 011, 100, 101, 110, 111. The rule — a number from 0 to 255 — says whether each neighborhood produces a living cell or a dead one in the next generation. Cell by cell, left to right, the new state is computed. The old state is replaced. The automaton has evolved one generation.

The output of cells 0 through 3 moves your filter cutoff. Cells 4 through 7 slow or accelerate your amplitude envelope. Cells 8 through 11 shift your pitch up or down by up to six semitones. Cells 12 through 15 deepen or reduce your reverb.

Nothing is random. Everything is determined by the rule you chose and the note you played. But as the generations accumulate — as the automaton evolves through fifty steps, a hundred steps, a thousand — the pattern may seem random, or may settle into a fixed point, or may oscillate between two states, or may produce a complex evolving texture that never exactly repeats. The behavior is entirely a function of which rule is in play.

You have just become a coral polyp. You are following simple rules. The music is what emerges.

---

## Phase R2: The Signal Path Journey

### I. The Cellular Automaton — The Colony's Heart

The automaton runs on a 16-bit unsigned integer: `caState`. Each bit represents one cell — 1 for alive, 0 for dead. The 16 cells form a circular ring: cell 0's left neighbor is cell 15, and cell 15's right neighbor is cell 0. This circular topology means the colony has no edges and no boundaries: it wraps around itself like a torus, or like a coral ring atoll.

At note-on, the automaton is reseeded. The seed parameter (0–65535) is XORed with `currentNote * 257` — the multiplication by 257 ensures that every MIDI note number from 0 to 127 produces a distinct contribution, while the prime factor creates good bit distribution. The result is that every note played at the same ORGANISM patch produces a different evolutionary trajectory. C3 and C#3 begin from different initial states. C3 on one pass and C3 on the next pass will produce identical trajectories — ORGANISM is entirely deterministic given the same inputs.

The `org_stepRate` parameter (0.5–32.0 Hz) determines how many automaton generations occur per second. At 0.5 Hz, one generation occurs every two seconds — evolutionary time measured in geological patience. At 32 Hz, thirty-two generations occur per second — the colony is evolving faster than most rhythmic contexts. The most musical range is 2–12 Hz, where the CA evolution is fast enough to interact with musical rhythm without obliterating it.

The `org_scope` parameter (1–16) controls the moving average window for reading cell outputs. At scope=1, each cell group (cells 0–3, 4–7, 8–11, 12–15) is read from the most recent generation only. At scope=16, the output is averaged across the last 16 generations. Scope is the instrument's memory parameter. Low scope: reactive, jittery, responding to individual generation states. High scope: smoothed, integrated, responding to the average population density over time. At high scope values with stable rules, the outputs converge to long-term population averages and stop moving significantly — the colony has found its equilibrium, and the modulation settles.

**The smoothing coefficient:** After the scope-averaged cell values are computed, they pass through a one-pole smoother (`kSmoothCoeff = 0.005f`) before reaching the DSP parameters. This prevents clicks when the automaton jumps to a new state. The 0.005 coefficient corresponds to approximately 3ms at 44100 Hz. This is transparent at stepRates above 2 Hz but creates a slight lag between the automaton state and the modulation output at slow rates.

---

### II. The Curated Rules — Eight Personalities

ORGANISM's `org_macroRule` parameter sweeps through eight specifically chosen Wolfram rules, indexed 0–7:

**Rule 30 (index 0) — Chaotic Growth**
Wolfram's canonical chaos rule. Rule 30 produces patterns indistinguishable from random noise despite being perfectly deterministic. The middle column of the Rule 30 spacetime diagram was used as Mathematica's random number generator for years. At 16 cells, Rule 30 produces sequences that cycle (all finite automata must eventually cycle) but with very long period lengths. This rule creates the most unpredictable filter/pitch/amplitude behavior. Good for: generative textures where no pattern should be apparent. Trap: on some seeds, Rule 30 can reach a fixed point quickly on a 16-cell ring — test with `org_mutate` > 0 if the colony dies.

**Rule 90 (index 1) — Sierpinski Fractal**
An additive rule: the output bit is the XOR of the left and right neighbors (center ignored). Rule 90 generates the Sierpinski triangle in its spacetime diagram — a perfect self-similar fractal. On a 16-cell ring, it oscillates between two complementary states: the initial state alternating with its bitwise NOT. At scope=1, this creates square-wave amplitude and filter modulation at exactly half the stepRate — a rhythmically precise pulse. At scope=4+, the averaging smooths this into a soft, regular oscillation. Good for: rhythmic contexts where a steady pulse is wanted from the CA. Set stepRate to multiples of the tempo for synchronized rhythmic behavior.

**Rule 110 (index 2) — Universal Computation (Default)**
The most computationally complex elementary rule — proven to be Turing-complete by Matthew Cook in 1994. Rule 110 produces complex patterns that are neither purely chaotic (Rule 30) nor purely periodic (Rule 90) but exist in a zone of structured complexity called the "edge of chaos." On a 16-cell ring, Rule 110 generates interesting evolving patterns that typically include both local structure and longer-range variation. This is the engine's default rule for good reason: it provides the richest steady-state behavior for sustained patches. Good for: pads, textures, slowly evolving sounds where neither pure chaos nor pure periodicity is wanted.

**Rule 184 (index 3) — Particle Flow**
A traffic flow rule — studied in physics as a model of single-lane vehicular traffic. Living cells move rightward; a cell moves if its right neighbor is empty. Rule 184 produces glider-like particles that travel through the ring. On a 16-cell circular topology, particles circulate perpetually until they collide. This creates regular, rhythmically patterned amplitude and filter modulation — the particles passing through the filter cell group produce regular pulses. Good for: sequencer-like behavior without explicit step programming. The gliders create audible rhythm.

**Rule 150 (index 4) — XOR Diffusion**
Similar to Rule 90 but center-inclusive (output = XOR of all three neighbors). Produces more complex period structures than Rule 90 on small rings. On 16 cells, Rule 150 typically produces medium-length cycles with more variation than Rule 90. Good for: a middle ground between the strict periodicity of Rule 90 and the complexity of Rule 110.

**Rule 18 (index 5) — Sparse Structure**
A relatively quiet rule that produces sparse, isolated structures. Starting from most seeds, Rule 18 creates widely-spaced active cells separated by large dead regions. The result is infrequent, isolated amplitude/filter events — a sparse, event-driven modulation pattern. The CA evolves toward a more and more sparse state over time on a circular ring, occasionally collapsing to all-zero (at which point the all-zero protection fires a single bit). Good for: sparse, glitchy textures where long silences and occasional events are the aesthetic.

**Rule 54 (index 6) — Complex Oscillation**
Rule 54 produces complex, locally structured patterns. It is one of the rules studied as a potential "class 4" rule (like Rule 110) exhibiting both periodic and complex behaviors depending on seed. On 16 cells, Rule 54 typically produces patterns with clear visual structure — diagonal stripes, periodic structures with local variation. Good for: structured but varied modulation that feels purposeful without being mechanical.

**Rule 22 (index 7) — Symmetric Branching**
A symmetric rule (symmetric under left-right reflection) that produces branching tree-like patterns. Rule 22 creates complex diagonal structures on many initial conditions and can also produce long-period oscillations. It is the most "architectural" of the curated rules — its spacetime diagrams look most like coral branching. Good for: textures where you want both structural regularity and genuine complexity.

---

### III. Seed Interaction with MIDI Note — The Note as Initial Condition

The XOR seeding formula is `caState = seed ^ (note * 257)`. This has a profound consequence for sound design: playing the same patch on different MIDI notes is not merely transposing the pitch. It is playing a different evolutionary trajectory from a different initial condition, at the same rule.

For stable rules (Rule 90, Rule 184), the structure of the trajectory is similar across initial conditions — the same qualitative behavior, shifted in phase. For complex rules (Rule 110, Rule 22), the trajectories can be qualitatively different: one initial condition may lead to a stable oscillation, another to complex evolution, another to rapid convergence to all-zero or all-one. This means ORGANISM patches may behave very differently across the keyboard — not just transposed, but fundamentally altered in their evolutionary character.

This is a feature, not a bug. It is the cellular automaton encoding musical pitch as biological identity. C3 and D3 are not the same colony of polyps at different sizes. They are different colonies following the same rules but producing different architectures from their different starting conditions.

**Design guidance for velocity:** Velocity boosts filter cutoff via `velCutoff * 3000 Hz`. A hard strike at velocity=127 with velCutoff=1.0 adds 3000 Hz to the base cutoff — a full three-octave brightness shift. This is D001 compliance in its most expressive form. Design presets so that the base cutoff + CA modulation + velocity boost stays within the 200–8000 Hz range without clipping to the ceiling. A base cutoff of 2500 Hz with moderate CA modulation is safe: velocity adds brightness without harshly limiting at the ceiling.

---

### IV. Cell Group Mappings — The Four Outputs

**Cells 0–3: Filter Cutoff** (cellFilterOut)
The scope-averaged population density of cells 0–3 maps the filter cutoff position. 0.0 = all dead cells → filter at base cutoff minus (baseCutoff × 0.8 × 0.5) = baseCutoff × 0.6. 1.0 = all alive → filter at base cutoff plus (baseCutoff × 0.8 × 0.5) = baseCutoff × 1.4. The ±40% modulation range centered on baseCutoff means that with baseCutoff=3000 Hz, the CA drives filter cutoff between 1800–4200 Hz. With baseCutoff=6000, the range is 3600–8400 Hz (clamped to 8000).

**Cells 4–7: Amplitude Envelope Rate** (cellAmpRate)
The scope-averaged population of cells 4–7 modulates the ADSR envelope processing rate via `envRateMod = lerp(3.0, 0.33, cellAmpRate)`. At cellAmpRate=0 (all dead), the envelope runs at 3× base rate — faster attack, faster decay, faster release. At cellAmpRate=1 (all alive), the envelope runs at 0.33× base rate — attack/decay/release all take three times longer than set. This is ORGANISM's hidden rhythmic mechanism. As the CA evolves, the envelope breathes: notes lengthen and shorten with each generation's population density. At fast stepRates with unstable rules, this creates stochastic rhythmic variation without explicit sequencing.

**Cells 8–11: Pitch Offset** (cellPitchOut)
Population density of cells 8–11 maps to ±6 semitones from root. The mapping is `semitones = round(cellPitchOut × 12 - 6)`. The rounding to integer semitones is critical: it quantizes the pitch modulation to chromatic steps, preventing microtonal drift that would be dissonant in most musical contexts. At cellPitchOut=0.5 (equal alive and dead), the offset is 0 semitones. As population rises toward 1.0, pitch rises toward +6 semitones. As population falls toward 0, pitch falls toward -6 semitones.

**Cells 12–15: Reverb Mix** (cellFXOut)
Population of cells 12–15 modulates reverb mix: `effectiveReverb = reverbMix + cellFXOut × 0.3`. At cellFXOut=0, reverb is at its parameter value. At cellFXOut=1.0, reverb is 0.3 higher than the parameter — up to the 1.0 ceiling. This creates reverb breathing: dense cell populations increase reverb depth, sparse populations dry the sound. At high reverbMix values (0.7+), the CA cannot increase it further — the reverb is already at maximum. Design with reverbMix in the 0.0–0.6 range to leave room for the CA to breathe the reverb upward.

---

### V. Oscillator Architecture — The Voice

ORGANISM is a single-voice synthesizer. The primary oscillator — saw, square, or triangle selected by `org_oscWave` — runs at the MIDI-determined fundamental frequency, modified by the pitch offset from cells 8–11. The sub oscillator runs at half frequency (one octave down) as a square wave.

The mix formula: `mixed = oscOut × (1 - subLevel × 0.5) + subOut × subLevel`. At subLevel=0, the primary oscillator is at full amplitude with no sub. At subLevel=1.0, the primary oscillator is attenuated to 50% and the sub at 100%. This means maximum subLevel does not simply add the sub at full amplitude — it also attenuates the primary, creating a controlled total output level.

**Waveform characters:**
- Saw (0): Full harmonic content — all odd and even harmonics present. Bright, buzzy. The richest spectrum for the filter to work on. Most responsive to filter cutoff modulation.
- Square (1): Odd harmonics only. Hollow, clarinets-and-reeds quality. The missing even harmonics create a distinctive thinness in the mid-range. Different filter cutoff behaviors because the spectrum is sparse at even-harmonic positions.
- Triangle (2): Odd harmonics only, but attenuated by 1/n². Much smoother than square. Approaches sine at high filter cutoff values. The softest, most rounded of the three. Best for slow-evolving pads where brightness should be minimal.

**Sub oscillator:** Square wave at half frequency. The sub adds fundamental weight and low-frequency presence. At subLevel=0.35 (default), the sub is present but not dominant. At subLevel=0.8+, the sub becomes the primary low-frequency character and the main oscillator becomes a mid-frequency texture over it.

---

### VI. The Filter — Two-Pole Lowpass

A standard biquad bilinear-transform two-pole lowpass filter. Cutoff range 200–8000 Hz. Q range 0.5–12.0 (mapped from `org_filterRes` 0.0–0.9 via `Q = 0.5 + filterRes × 11.5`). Gain compensation applied: `gainComp = 1.0 / (1.0 + Q × 0.5)` prevents resonant peaks from driving the output into clipping.

The total filter cutoff is: `baseCutoff + cellCutoffOffset + velCutoffBoost + lfo1CutoffMod + lfo2CutoffMod + couplingFilterMod`. This is a crowded stack. Design presets so these contributions work together rather than fighting. If the CA is providing large filter modulation (baseCutoff × ±80%), adding large LFO depth on top creates unpredictable clipping at filter extremes. Use either CA or LFO as the primary filter modulator, with the other providing secondary texture.

**Filter resonance character:**
- Q 0.5–1.5: Clean, flat response near cutoff. No resonant character. Filtering without emphasis.
- Q 2.0–4.5: Subtle notch quality. The filter begins to color the sound at the cutoff frequency without ringing.
- Q 5.0–8.0: Clear resonant peak. The cutoff frequency is emphasized. CA filter modulation creates pitched resonant sweeps.
- Q 8.0–12.0: High resonance. The filter self-oscillates at extreme settings (Q > ~9). Strong emphasis. The CA-driven resonance sweeps become melodically significant — each step creates a new resonant pitch.

---

### VII. LFOs — Secondary Modulation

Two independent LFOs, both sine-shaped, both modulating filter cutoff.

**LFO1** (`org_lfo1Rate` 0.01–10 Hz, `org_lfo1Depth` 0–1): Depth scales to ±600 Hz. At depth=1.0 and rate=0.01 Hz, the LFO sweeps the filter by ±600 Hz over a 100-second period — barely perceptible but providing D005 autonomous breathing. At rate=8 Hz and depth=0.5, LFO1 creates 300 Hz of fast filter vibrato — audibly rhythmic at tempo.

**LFO2** (`org_lfo2Rate` 0.01–10 Hz, `org_lfo2Depth` 0–1): Depth scales to ±1000 Hz. Larger range than LFO1, intended as the primary sweep LFO. At depth=0.5 and rate=0.3 Hz, LFO2 sweeps ±500 Hz on a 3.3-second cycle — a slow filter sweep that interacts with the CA modulation.

**LFO design principle:** The CA is already modulating the filter via cells 0–3. LFOs should be set lower than the CA modulation depth to avoid overwhelming the cellular pattern with conventional wobble. The CA provides the interesting, irregular modulation. LFOs provide the predictable, steady underpinning.

**Expression interaction (D006):** Mod wheel morphs the rule index by up to +2 positions. At maximum mod wheel with macroRule=0 (Rule 30), the wheel morphs toward Rule 110 (index 2). This means the mod wheel can be used in performance to shift from chaotic (low wheel) to complex-structured (high wheel) behavior in real time — without disrupting the current CA state.

---

### VIII. Reverb — Emergence Shimmer

The OrgReverb is a traditional 4-comb + 4-allpass Schroeder-style reverb, fixed at feedback 0.72 and damping 0.25. The mix is controlled by `org_reverbMix` plus the CA modulation from cells 12–15. This reverb is deliberately modest — it does not have the structural sophistication of OXBOW's Chiasmus FDN. Its role is to add spatial dimension without competing with the CA-driven primary character. Think of it as cave walls behind the colony, not an architectural feature in itself.

At high reverbMix values (0.7+), ORGANISM develops a characteristic wash — the oscillator's evolving CA-modulated output accumulates in the reverb tail, creating a self-layering texture where current and past states coexist. This can be beautiful or muddy depending on rule and stepRate. Stable, periodic rules at medium stepRates produce clean reverb accumulation. Chaotic rules at fast stepRates produce dense noise-wash.

---

## Phase R3: Parameter Map — Sweet Spots Summary

| Parameter | ID | Range | Default | Conservative | Musical Core | Expressive | Extreme |
|-----------|-----|-------|---------|--------------|--------------|-----------|---------|
| CA Rule | `org_rule` | 0–255 | 110 | 90, 184 | 110, 22, 54 | 30, 150 | All others |
| Seed | `org_seed` | 0–65535 | 42 | Any | 42–5000 | 5000–30000 | 30000–65535 |
| Step Rate | `org_stepRate` | 0.5–32 Hz | 4.0 | 0.5–2.0 | 2.0–10.0 | 10.0–20.0 | 20.0–32.0 |
| Scope | `org_scope` | 1–16 | 4 | 8–16 | 3–8 | 2–4 | 1 |
| Mutate | `org_mutate` | 0.0–1.0 | 0.0 | 0.0–0.05 | 0.05–0.20 | 0.20–0.40 | 0.40–1.0 |
| Osc Wave | `org_oscWave` | 0/1/2 | 0 (saw) | Triangle (2) | Saw (0) | Square (1) | — |
| Sub Level | `org_subLevel` | 0.0–1.0 | 0.35 | 0.0–0.20 | 0.20–0.55 | 0.55–0.80 | 0.80–1.0 |
| Filter Cutoff | `org_filterCutoff` | 200–8000 Hz | 3000 | 800–2000 | 1500–4500 | 4000–7000 | 7000–8000 |
| Filter Res | `org_filterRes` | 0.0–0.9 | 0.3 | 0.0–0.20 | 0.20–0.60 | 0.55–0.80 | 0.80–0.90 |
| Vel Cutoff | `org_velCutoff` | 0.0–1.0 | 0.5 | 0.15–0.35 | 0.35–0.70 | 0.70–0.90 | 0.90–1.0 |
| Amp Attack | `org_ampAtk` | 0.001–2.0s | 0.015s | 0.001–0.010 | 0.010–0.080 | 0.080–0.400 | 0.400–2.0 |
| Amp Decay | `org_ampDec` | 0.05–4.0s | 0.35s | 0.05–0.25 | 0.20–1.0 | 1.0–2.5 | 2.5–4.0 |
| Amp Sustain | `org_ampSus` | 0.0–1.0 | 0.7 | 0.50–0.80 | 0.30–0.75 | 0.10–0.40 | 0.0–0.15 |
| Amp Release | `org_ampRel` | 0.05–5.0s | 0.6s | 0.05–0.30 | 0.20–1.5 | 1.5–3.0 | 3.0–5.0 |
| LFO1 Rate | `org_lfo1Rate` | 0.01–10 Hz | 0.5 | 0.01–0.05 | 0.05–2.0 | 2.0–6.0 | 6.0–10.0 |
| LFO1 Depth | `org_lfo1Depth` | 0.0–1.0 | 0.2 | 0.0–0.10 | 0.10–0.40 | 0.40–0.70 | 0.70–1.0 |
| LFO2 Rate | `org_lfo2Rate` | 0.01–10 Hz | 0.3 | 0.01–0.08 | 0.05–1.5 | 1.5–5.0 | 5.0–10.0 |
| LFO2 Depth | `org_lfo2Depth` | 0.0–1.0 | 0.25 | 0.0–0.10 | 0.10–0.45 | 0.45–0.75 | 0.75–1.0 |
| Reverb Mix | `org_reverbMix` | 0.0–1.0 | 0.2 | 0.0–0.15 | 0.10–0.40 | 0.35–0.60 | 0.60–0.90 |

---

## Phase R4: Macro Architecture

| Macro | ID | Effect | Performance Use |
|-------|-----|--------|----------------|
| RULE | `org_macroRule` | Maps 0–1 across 8 curated rules (30→90→110→184→150→18→54→22) | Sweep from chaos (low) to structure (high) in real time |
| SEED | `org_macroSeed` | Re-randomizes CA initial state via LCG when above threshold | Trigger new colony — resets CA to a fresh random state |
| COUPLING | `org_macroCoupling` | Scales incoming coupling receive depth + adds trace autonomous mutation | Control how strongly partner engines influence ORGANISM's filter and pitch |
| MUTATE | `org_macroMutate` | Additional mutation probability on top of `org_mutate` | Performance control for increasing cellular chaos without changing preset |

**Macro philosophy for ORGANISM:** The four macros represent the four dimensions of cellular automata control. RULE controls identity — which law governs this colony. SEED controls origin — the specific initial condition from which this colony's history unfolds. COUPLING controls interaction — how much outside influence reaches the colony. MUTATE controls stress — how much environmental pressure bends the rules.

In performance, RULE and SEED are the dramatic controls — sweeping RULE from chaos to structure produces an audible character transformation. Triggering SEED during a held note produces an instant pattern refresh without note-on, maintaining the pitch while resetting the colony's evolutionary history.

**Aftertouch as evolutionary pressure:** Aftertouch adds up to +0.3 to the effective mutation rate. In biology, environmental stress increases mutation rate in many organisms — heat shock, UV radiation, chemical exposure all increase the probability of copying errors in DNA replication. ORGANISM's aftertouch routing encodes this: pressing harder into a held note increases cellular chaos. A gentle touch = stable evolution. A pressed, intense touch = stressed colony, mutating.

**Mod wheel as rule interpolation:** The mod wheel shifts the rule index by up to +2 positions. At macroRule=0, wheel position 0 = Rule 30 (chaos), wheel position 127 = Rule 110 (complexity). The interpolation between rules is done per-bit: each bit of the rule byte is assigned to Rule A or Rule B based on the blend position. This creates hybrid rules that exist between the named Wolfram rules — the wheel moves through rule-space, not just between discrete named positions.

---

## Phase R5: The Six Recipe Categories

### Recipe Category 1: Coral Growth — Slow Emergence Pad

**Identity:** ORGANISM as slow-developing ambient texture. stepRate in the geological range (0.5–2.5 Hz), Rule 110 or Rule 22, scope high (8–14) so the output is integrated across many generations, long attack and release. These are pads that change over bars, not beats. Play a note and wait. The colony is building.

**Parameter ranges:**
- `org_rule`: 110 or 22
- `org_stepRate`: 0.5–2.5 Hz
- `org_scope`: 8–14
- `org_mutate`: 0.0–0.06 (stable evolution, minimal stress)
- `org_oscWave`: 2 (triangle) or 0 (saw)
- `org_filterCutoff`: 800–2500 Hz (keep dark — brightness emerges as the CA evolves)
- `org_filterRes`: 0.15–0.45 (modest resonance)
- `org_ampAtk`: 0.2–1.0s (slow bloom)
- `org_ampSus`: 0.55–0.80
- `org_ampRel`: 1.5–4.0s (long decay, notes don't end abruptly)
- `org_lfo1Rate`: 0.02–0.15 Hz (breathing, not movement)
- `org_lfo2Depth`: 0.05–0.20 (trace LFO modulation)
- `org_reverbMix`: 0.30–0.55 (spatial context)

**Mood targets:** Atmosphere, Aether, Foundation

**Why it works:** At stepRate=1.0 Hz and scope=12, the filter modulation is the 12-generation moving average of the cell population. This changes approximately every second, producing gentle, slow sweeps that have no obvious rhythmic relationship to the tempo. Combined with a 0.5-second attack, the note blooms slowly as both the envelope opens and the colony begins its evolution. Each new note plays a different evolutionary trajectory because of the note-number seeding.

**Trap:** Coral Growth presets collapse if scope is too low (1–3). At scope=1, the modulation jumps with every CA step — audible as a rhythmic pattern rather than slow emergence. Always pair slow stepRate with high scope.

---

### Recipe Category 2: Cell Division — Rhythmic Emergence

**Identity:** ORGANISM as a generative rhythmic voice. Rule 90 or Rule 184 (both have strong periodic tendencies), stepRate synchronized to tempo, low scope (1–3), amplitude envelope rate modulated by cells 4–7 to create rhythmic variation. The CA generates not a pad but a rhythmically evolving monophonic line.

**Parameter ranges:**
- `org_rule`: 90 (strict rhythm) or 184 (particle flow rhythm)
- `org_stepRate`: quarter-note-aligned values (at 120 BPM: 2.0, 4.0, 8.0 Hz)
- `org_scope`: 1–3 (reactive, individual generation response)
- `org_mutate`: 0.0–0.10 (slight chaos to prevent locked loops)
- `org_ampAtk`: 0.001–0.010s (fast attack — each CA step is percussive)
- `org_ampDec`: 0.05–0.30s (short decay — notes gate themselves)
- `org_ampSus`: 0.10–0.40 (low sustain — notes decay quickly from peak)
- `org_filterCutoff`: 2500–6000 Hz (brighter — rhythmic content should cut through)
- `org_filterRes`: 0.40–0.70 (resonant — adds character to each rhythmic event)
- `org_lfo1Rate`: Sync to stepRate or slightly offset to create polyrhythmic modulation

**Mood targets:** Flux, Foundation, Prism

**Why it works:** Rule 90 produces a perfect alternating pattern at half the stepRate. At stepRate=8.0 Hz with quarter-note = 120 BPM (2 Hz), the CA fires 8 steps per beat. The amplitude modulation from cells 4–7 creates a rhythmic gate at every step, producing a rapid 8th-note or 16th-note figure. With a small `org_mutate` value (0.05–0.10), occasional bit flips break the perfect loop — avoiding the locked-loop problem where Rule 90 simply repeats forever on a 16-cell ring.

**Trap:** Rule 90 on a 16-cell ring will eventually settle into a two-state alternation (state → NOT state → state → ...). Once locked, the modulation becomes perfectly predictable — no variation. Set `org_mutate` > 0 to inject occasional bit flips that break the lock. Rule 184 avoids this through its particle flow dynamics.

---

### Recipe Category 3: Stable Colony Texture — Structural Harmony

**Identity:** Rule 110 or Rule 54 at medium stepRate, medium scope, with high resonance filter. The CA settles into its characteristic "edge of chaos" behavior — neither periodic nor fully random — and the resonant filter maps the cell density to specific harmonic emphases. The pitch modulation from cells 8–11 generates melodic motion within ±6 semitones of root. This is ORGANISM as a self-playing melodic voice.

**Parameter ranges:**
- `org_rule`: 110 (complex, non-repeating) or 54 (structured complexity)
- `org_stepRate`: 3.0–8.0 Hz (medium — pitch changes are audible as melody)
- `org_scope`: 3–6 (moderate integration)
- `org_filterRes`: 0.55–0.80 (high resonance — cell filter output creates pitched sweeps)
- `org_filterCutoff`: 1500–4000 Hz
- `org_ampSus`: 0.50–0.75
- `org_subLevel`: 0.30–0.60 (sub adds harmonic weight to the pitch variation)
- `org_mutate`: 0.0–0.08

**Mood targets:** Entangled, Foundation, Prism

**Why it works:** Rule 110's edge-of-chaos behavior means that cells 8–11 population density changes in a structured but non-repeating way. The pitch modulation moves through chromatic space without a fixed pattern — not random, but not a loop either. With high filter resonance, each pitch position emphasizes a different filter frequency: the CA-driven resonance sweep creates a pitched, quasi-melodic quality. At stepRate=5 Hz, the pitch changes occur every 200ms — slow enough to hear each note but fast enough to feel like a melodic sequence rather than a drone.

**Trap:** The pitch offset range (±6 semitones) is intentionally chromatic. At high resonance, some CA pitch steps will be dissonant against held harmonic context. Either: (a) play ORGANISM in a musical context where chromatic movement is acceptable (minor key, modal context), or (b) use `org_freeze` to lock the CA at a moment of consonant cell state and use only the LFO and filter for movement.

---

### Recipe Category 4: Extinction Event — Collapse and Restart

**Identity:** High mutation, chaotic rule (30 or 150), combined with the CA's all-zero protection mechanism and SEED macro performance control. These presets are designed to be played — triggered, released, retriggered — with the SEED macro swept during holds to randomly restart colony states. The extinction event is when all cells die and the protection injects a single bit to restart. With high mutation and Rule 30, the restart never settles — the colony is perpetually in a state of near-death and rebirth.

**Parameter ranges:**
- `org_rule`: 30 (sustained chaos) or 150 (chaotic but with slight structure)
- `org_stepRate`: 10.0–22.0 Hz (fast evolution — extinction events happen quickly)
- `org_mutate`: 0.25–0.60 (high stress — colony under maximum environmental pressure)
- `org_scope`: 1–2 (maximally reactive — each generation fully expressed)
- `org_ampAtk`: 0.001–0.005s (percussive attack)
- `org_ampDec`: 0.05–0.15s
- `org_ampSus`: 0.05–0.20 (very low sustain — notes gate themselves hard)
- `org_filterCutoff`: 3000–8000 Hz (bright — aggression, not warmth)
- `org_filterRes`: 0.65–0.88 (high resonance — each CA step creates a resonant accent)
- `org_reverbMix`: 0.10–0.30 (minimal reverb — the chaos should be dry)
- SEED macro: designed to be swept in performance for instant colony resets

**Mood targets:** Flux, Prism

**Why it works:** Rule 30 at high mutation rates produces genuinely chaotic bit sequences. The CA state collapses to zero, protection fires a single bit, and Rule 30 immediately grows the single bit into a new complex pattern within 2–3 generations. The amplitude modulation from cells 4–7 creates stochastic rhythmic gating — the colony's survival rate determines whether you hear a note or silence at each step. This is ORGANISM as a chaos engine: it does not produce music directly, but it produces the raw material of musical chaos that a skilled player can sculpt.

**Performance technique:** Hold a note. Watch (or listen) as the colony evolves. When the texture reaches a moment you like, use `org_freeze` to lock it. Take a mental snapshot of the CA state, then release freeze to let it evolve again. The transitions in and out of freeze are ORGANISM's most powerful performance moments.

---

### Recipe Category 5: Predator/Prey Rhythm — Oscillating Population

**Identity:** Rule 184 (particle/traffic rule) at medium-to-fast stepRate with pitch modulation fully expressed. Rule 184's particle-flow dynamics create an oscillating population in cells 8–11 — the "prey" particles circulate, collide, and produce rhythmic density changes. This drives pitch offset up and down in a quasi-periodic pattern that feels like a predator-prey population cycle: dense, then sparse, then dense again.

**Parameter ranges:**
- `org_rule`: 184 (primary) or custom values near 184 using `org_rule` direct control
- `org_stepRate`: 4.0–12.0 Hz
- `org_scope`: 2–5 (fast integration of pitch cell density)
- `org_filterRes`: 0.30–0.55 (moderate resonance — pitch should lead, not filter)
- `org_filterCutoff`: 2000–5000 Hz
- `org_subLevel`: 0.50–0.80 (strong sub gives bass weight to low pitch positions)
- `org_ampSus`: 0.60–0.85 (sustained notes allow the pitch modulation to arc fully)
- `org_mutate`: 0.02–0.08 (slight chaos to prevent perfect periodicity)

**Mood targets:** Foundation, Entangled, Flux

**Why it works:** Rule 184 creates particles (isolated living cells surrounded by dead cells) that move rightward through the ring. When cells 8–11 are populated by circulating particles, the population density oscillates as particles enter and exit the monitored zone. The scope-averaged output rises as particles cluster in the zone and falls as they clear — producing a quasi-sinusoidal pitch modulation that has nothing to do with a conventional LFO. The modulation period depends on the ring size (16 cells), stepRate, and initial particle distribution.

**Ecological note:** In Lotka-Volterra predator-prey models, prey population rises when predators are sparse, predators multiply when prey is plentiful, prey crashes, predators decline — and the cycle repeats. Rule 184's particle circulation on a closed ring produces a similar oscillating density — particles pack when they catch up to each other, spread when the ring is sparse. The CA is running a simple ecological simulation, and the resulting pitch modulation is biologically authentic in its pattern.

---

### Recipe Category 6: The Patience Parameter — Long Scope, Slow Evolution

**Identity:** The most meditative ORGANISM configuration. Scope maximal (12–16), stepRate slow (0.5–1.5 Hz), Rule 110 or Rule 22, no mutation, high reverb, long release. At these settings, the modulation outputs change extremely slowly — the filter sweep may take minutes to complete a significant arc. This is ORGANISM as a geological process. Notes are held for 30 seconds. The colony evolves for 16 generations before producing a new modulation output.

**Parameter ranges:**
- `org_rule`: 110 or 22
- `org_stepRate`: 0.5–1.5 Hz
- `org_scope`: 12–16 (maximum memory)
- `org_mutate`: 0.0 (pure, deterministic evolution)
- `org_freeze`: Used in performance to lock at meaningful moments
- `org_ampAtk`: 0.3–1.5s
- `org_ampRel`: 2.0–5.0s
- `org_reverbMix`: 0.40–0.65 (spatial environment)
- `org_lfo1Rate`: 0.01–0.03 Hz (D005 breathing, barely perceptible)
- `org_filterCutoff`: 1200–3000 Hz

**Mood targets:** Aether, Atmosphere, Submerged

**Why it works:** At scope=16, the filter modulation output is the 16-generation moving average of cells 0–3. This changes by approximately 1/16 of one generation's contribution per step. At stepRate=1 Hz, a significant arc in modulation takes 16+ seconds. In a musical context, this is longer than most phrases, many sections, and some entire movements. ORGANISM here is not a gesture — it is a process. Playing this configuration is not performing an instrument in the conventional sense. It is initiating a process and observing it unfold.

**The Schulze connection:** Klaus Schulze built entire album sides from slowly evolving sequencer patterns that took minutes to complete their arcs. ORGANISM's long-scope, slow-stepRate configuration is the cellular automata equivalent: a self-generating, slowly evolving pattern that rewards patient listening. Unlike Schulze's sequencers, ORGANISM's pattern is not programmed. It emerges. What it will do at minute 3 is unknown when the note is played at minute 0.

---

## Phase R6: The Ten Awakenings — Preset Table

### Preset 1: Coral Bloom

**Mood:** Atmosphere | **Category:** Coral Growth | **Discovery:** The slow emergence reference — Rule 110 at geological stepRate, scope 10

| Parameter | Value | Why |
|-----------|-------|-----|
| `org_rule` | 110 | Universal computation — richest sustained evolution |
| `org_seed` | 42 | Default — well-behaved starting state |
| `org_stepRate` | 1.2 | ~1 generation/second — geological pace |
| `org_scope` | 10 | 10-generation moving average — very smooth |
| `org_mutate` | 0.02 | Trace mutation — prevents locked loops |
| `org_oscWave` | 2 (triangle) | Soft waveform — warmth, not buzz |
| `org_subLevel` | 0.40 | Substantial sub presence |
| `org_filterCutoff` | 1800 | Dark starting point — CA brightens from here |
| `org_filterRes` | 0.30 | Modest resonance |
| `org_velCutoff` | 0.55 | Moderate velocity expression |
| `org_ampAtk` | 0.45 | Slow bloom |
| `org_ampDec` | 0.80 | Long decay |
| `org_ampSus` | 0.65 | Sustained |
| `org_ampRel` | 2.5 | Long release — notes dissolve |
| `org_lfo1Rate` | 0.03 | Very slow LFO — D005 floor |
| `org_lfo1Depth` | 0.12 | Trace LFO |
| `org_lfo2Rate` | 0.08 | Gentle secondary |
| `org_lfo2Depth` | 0.08 | Minimal |
| `org_reverbMix` | 0.38 | Present room |

**Why this works:** A held note builds slowly as the attack envelope opens (0.45s) and the CA begins its Rule 110 evolution. Over 15–20 seconds, the scope-10 averaged cell outputs slowly shift, producing gradual filter brightening, subtle pitch drift within ±6 semitones, and reverb breathing. This preset teaches the listener how ORGANISM works — each evolution is slow enough to follow consciously.

---

### Preset 2: Polyp Storm

**Mood:** Flux | **Category:** Extinction Event | **Discovery:** Rule 30 + high mutation = sustained generative chaos

| Parameter | Value | Why |
|-----------|-------|-----|
| `org_rule` | 30 | Maximum chaos rule |
| `org_seed` | 21845 | Alternating bit pattern (0x5555) — interesting Rule 30 start |
| `org_stepRate` | 14.0 | Fast evolution |
| `org_scope` | 1 | Maximum reactivity |
| `org_mutate` | 0.30 | High stress |
| `org_oscWave` | 0 (saw) | Full harmonics for chaos to filter |
| `org_subLevel` | 0.20 | Reduced sub — chaos is the feature |
| `org_filterCutoff` | 4500 | Bright center |
| `org_filterRes` | 0.72 | High resonance — each CA step creates an accent |
| `org_velCutoff` | 0.70 | Strong velocity expression |
| `org_ampAtk` | 0.002 | Percussive attack |
| `org_ampDec` | 0.12 | Short decay |
| `org_ampSus` | 0.15 | Very low sustain |
| `org_ampRel` | 0.25 | Quick release |
| `org_lfo1Rate` | 6.5 | Fast LFO — adds to chaos character |
| `org_lfo1Depth` | 0.30 | Notable LFO depth |
| `org_lfo2Rate` | 4.0 | Second fast LFO |
| `org_lfo2Depth` | 0.45 | Substantial sweep |
| `org_reverbMix` | 0.15 | Minimal reverb — chaos should be dry |

---

### Preset 3: Sierpinski Pulse

**Mood:** Foundation | **Category:** Cell Division | **Discovery:** Rule 90 at rhythmic stepRate = precise generative sequencer

| Parameter | Value | Why |
|-----------|-------|-----|
| `org_rule` | 90 | XOR rule — Sierpinski fractal, clean alternation |
| `org_seed` | 1 | Single living cell — clean Rule 90 triangle start |
| `org_stepRate` | 8.0 | 8 Hz = 16th notes at 120 BPM with quarter at 2 Hz |
| `org_scope` | 2 | Fast integration |
| `org_mutate` | 0.04 | Slight mutation to break perfect loop |
| `org_oscWave` | 1 (square) | Square — odd harmonics for rhythmic clarity |
| `org_subLevel` | 0.45 | Sub adds weight to downbeats |
| `org_filterCutoff` | 3800 | Bright — rhythmic clarity |
| `org_filterRes` | 0.55 | Resonant accents |
| `org_velCutoff` | 0.65 | Velocity expressive |
| `org_ampAtk` | 0.003 | Percussive |
| `org_ampDec` | 0.18 | Short |
| `org_ampSus` | 0.25 | Low — gates itself |
| `org_ampRel` | 0.20 | Quick |
| `org_lfo1Rate` | 2.0 | Synced to stepRate for polyrhythm |
| `org_lfo1Depth` | 0.22 | Moderate |
| `org_lfo2Rate` | 3.0 | Offset polyrhythm |
| `org_lfo2Depth` | 0.30 | Moderate |
| `org_reverbMix` | 0.18 | Light reverb — rhythmic should stay tight |

---

### Preset 4: Reef Structure

**Mood:** Entangled | **Category:** Stable Colony Texture | **Discovery:** Rule 110 at medium speed, high resonance = self-playing melodic voice

| Parameter | Value | Why |
|-----------|-------|-----|
| `org_rule` | 110 | Edge of chaos — non-repeating melody |
| `org_seed` | 7777 | Odd-biased start — good Rule 110 evolution |
| `org_stepRate` | 5.5 | ~3 pitch changes per second |
| `org_scope` | 4 | Medium integration |
| `org_mutate` | 0.03 | Minimal |
| `org_oscWave` | 0 (saw) | Full harmonics for resonance to work on |
| `org_subLevel` | 0.50 | Strong sub — melodic movement anchored low |
| `org_filterCutoff` | 2800 | Mid position |
| `org_filterRes` | 0.68 | High resonance — pitch sweeps become tonal |
| `org_velCutoff` | 0.50 | Balanced velocity expression |
| `org_ampAtk` | 0.025 | Short but not percussive |
| `org_ampDec` | 0.45 | Medium |
| `org_ampSus` | 0.60 | Sustained melody |
| `org_ampRel` | 1.2 | Legato-style release |
| `org_lfo1Rate` | 0.15 | Slow supporting LFO |
| `org_lfo1Depth` | 0.18 | Trace |
| `org_lfo2Rate` | 0.40 | Secondary gentle sweep |
| `org_lfo2Depth` | 0.25 | Background |
| `org_reverbMix` | 0.30 | Room context |

---

### Preset 5: Calcium Archive

**Mood:** Aether | **Category:** The Patience Parameter | **Discovery:** Scope 16, stepRate 0.5 = ORGANISM as geological instrument

| Parameter | Value | Why |
|-----------|-------|-----|
| `org_rule` | 22 | Symmetric branching — architectural |
| `org_seed` | 512 | Clean power-of-2 start |
| `org_stepRate` | 0.5 | One generation every 2 seconds |
| `org_scope` | 16 | Maximum memory — 16 generations averaged |
| `org_mutate` | 0.0 | Pure deterministic evolution |
| `org_oscWave` | 2 (triangle) | Soft, sustained waveform |
| `org_subLevel` | 0.55 | Strong sub presence |
| `org_filterCutoff` | 1500 | Dark starting point |
| `org_filterRes` | 0.25 | Gentle resonance |
| `org_velCutoff` | 0.35 | Mild velocity expression |
| `org_ampAtk` | 0.80 | Slow bloom — 0.8 seconds |
| `org_ampDec` | 1.2 | Long decay phase |
| `org_ampSus` | 0.70 | High sustain |
| `org_ampRel` | 4.0 | Very long release — notes persist |
| `org_lfo1Rate` | 0.01 | D005 minimum — barely perceptible |
| `org_lfo1Depth` | 0.08 | Trace breathing |
| `org_lfo2Rate` | 0.02 | Equally slow |
| `org_lfo2Depth` | 0.06 | Trace |
| `org_reverbMix` | 0.55 | Present spatial context |

---

### Preset 6: Particle Drift

**Mood:** Prism | **Category:** Predator/Prey Rhythm | **Discovery:** Rule 184 particle circulation = quasi-periodic pitch modulation

| Parameter | Value | Why |
|-----------|-------|-----|
| `org_rule` | 184 | Traffic flow rule — circulating gliders |
| `org_seed` | 43690 | Alternating live/dead (0xAAAA) — creates many particles |
| `org_stepRate` | 6.0 | Moderate step — gliders circulate audibly |
| `org_scope` | 3 | Fast pitch response |
| `org_mutate` | 0.03 | Minimal — preserve particle structure |
| `org_oscWave` | 0 (saw) | Full spectrum |
| `org_subLevel` | 0.65 | Strong sub — low positions emphasized |
| `org_filterCutoff` | 3200 | Moderate |
| `org_filterRes` | 0.45 | Moderate resonance |
| `org_velCutoff` | 0.60 | Expressive velocity |
| `org_ampAtk` | 0.015 | Quick |
| `org_ampDec` | 0.55 | Medium |
| `org_ampSus` | 0.70 | High sustain — holds through pitch modulation arc |
| `org_ampRel` | 1.8 | Long release |
| `org_lfo1Rate` | 0.25 | Slow oscillation complementing CA |
| `org_lfo1Depth` | 0.28 | Moderate |
| `org_lfo2Rate` | 0.60 | Slightly faster |
| `org_lfo2Depth` | 0.20 | Light |
| `org_reverbMix` | 0.28 | Moderate reverb |

---

### Preset 7: Atoll Ring

**Mood:** Submerged | **Category:** Coral Growth — deep and dark variant

| Parameter | Value | Why |
|-----------|-------|-----|
| `org_rule` | 110 | Complex, sustained evolution |
| `org_seed` | 2048 | Sparse start — slow colony growth |
| `org_stepRate` | 1.8 | Slow evolution |
| `org_scope` | 8 | High memory |
| `org_mutate` | 0.01 | Trace |
| `org_oscWave` | 2 (triangle) | Softest waveform — deep water aesthetic |
| `org_subLevel` | 0.80 | Dominant sub — everything lives in the bass |
| `org_filterCutoff` | 800 | Very dark |
| `org_filterRes` | 0.20 | Minimal resonance — darkness, not sharpness |
| `org_velCutoff` | 0.45 | Moderate |
| `org_ampAtk` | 0.60 | Slow |
| `org_ampDec` | 1.5 | Long |
| `org_ampSus` | 0.75 | High sustain |
| `org_ampRel` | 3.5 | Very long |
| `org_lfo1Rate` | 0.025 | Very slow |
| `org_lfo1Depth` | 0.15 | Trace |
| `org_lfo2Rate` | 0.04 | Glacial |
| `org_lfo2Depth` | 0.10 | Minimal |
| `org_reverbMix` | 0.65 | Heavy reverb — deep ocean space |

---

### Preset 8: Bifurcation

**Mood:** Flux | **Category:** Extinction Event — controlled chaos

| Parameter | Value | Why |
|-----------|-------|-----|
| `org_rule` | 150 | Complex XOR variant — between Rule 90 and chaos |
| `org_seed` | 12345 | Irregular start |
| `org_stepRate` | 18.0 | Fast — events happen quickly |
| `org_scope` | 1 | Maximum reactivity |
| `org_mutate` | 0.22 | High mutation — colony is stressed |
| `org_oscWave` | 1 (square) | Odd harmonics — brittle character |
| `org_subLevel` | 0.15 | Minimal sub |
| `org_filterCutoff` | 5500 | Bright |
| `org_filterRes` | 0.82 | Very high resonance — each CA step resonates |
| `org_velCutoff` | 0.80 | Strong velocity expression |
| `org_ampAtk` | 0.001 | Percussive — immediate |
| `org_ampDec` | 0.08 | Very short |
| `org_ampSus` | 0.08 | Near-zero — gates itself |
| `org_ampRel` | 0.18 | Fast release |
| `org_lfo1Rate` | 8.5 | Very fast — near-audio rate modulation |
| `org_lfo1Depth` | 0.18 | Audible modulation |
| `org_lfo2Rate` | 5.0 | Fast |
| `org_lfo2Depth` | 0.35 | Moderate |
| `org_reverbMix` | 0.20 | Light reverb |

---

### Preset 9: Colony Memory

**Mood:** Atmosphere | **Category:** Coral Growth — high scope integration

| Parameter | Value | Why |
|-----------|-------|-----|
| `org_rule` | 54 | Structured complexity — visual branching patterns |
| `org_seed` | 3333 | Moderate density start |
| `org_stepRate` | 3.0 | Medium — evolution visible but not frantic |
| `org_scope` | 12 | High memory — 12-generation average |
| `org_mutate` | 0.02 | Minimal |
| `org_oscWave` | 0 (saw) | Full harmonics |
| `org_subLevel` | 0.42 | Moderate sub |
| `org_filterCutoff` | 2200 | Mid-dark |
| `org_filterRes` | 0.50 | Moderate resonance |
| `org_velCutoff` | 0.55 | Balanced |
| `org_ampAtk` | 0.30 | Medium bloom |
| `org_ampDec` | 0.70 | Present decay |
| `org_ampSus` | 0.62 | Good sustain |
| `org_ampRel` | 2.2 | Long release |
| `org_lfo1Rate` | 0.06 | Slow breathing |
| `org_lfo1Depth` | 0.15 | Trace |
| `org_lfo2Rate` | 0.12 | Secondary |
| `org_lfo2Depth` | 0.18 | Light |
| `org_reverbMix` | 0.42 | Present reverb |

---

### Preset 10: Rule of Life

**Mood:** Foundation | **Category:** Stable Colony Texture — init reference

| Parameter | Value | Why |
|-----------|-------|-----|
| `org_rule` | 110 | The default — all things from this rule |
| `org_seed` | 42 | The default seed |
| `org_stepRate` | 4.0 | The default stepRate |
| `org_scope` | 4 | The default scope |
| `org_mutate` | 0.0 | No mutation — pure rule |
| `org_oscWave` | 0 (saw) | Default saw |
| `org_subLevel` | 0.35 | Default sub |
| `org_filterCutoff` | 3000 | Default cutoff |
| `org_filterRes` | 0.30 | Default resonance |
| `org_velCutoff` | 0.50 | Default velocity expression |
| `org_ampAtk` | 0.015 | Default attack |
| `org_ampDec` | 0.35 | Default decay |
| `org_ampSus` | 0.70 | Default sustain |
| `org_ampRel` | 0.60 | Default release |
| `org_lfo1Rate` | 0.50 | Default LFO1 |
| `org_lfo1Depth` | 0.20 | Default depth |
| `org_lfo2Rate` | 0.30 | Default LFO2 |
| `org_lfo2Depth` | 0.25 | Default depth |
| `org_reverbMix` | 0.20 | Default reverb |

**Why this preset exists:** The init patch is not the most interesting preset in the library. It is the most honest one. Every parameter at its intended default. No macro offsets obscuring the engine's neutral behavior. A producer who plays this preset and then reads the parameter map will understand ORGANISM. A producer who plays only the more extreme presets will not.

---

## Phase R7: Parameter Interactions and Traps

### The Scope-StepRate Interaction

Scope is the memory parameter; stepRate determines how fast that memory updates. Their interaction defines the character bandwidth of ORGANISM's modulation outputs.

- **Low scope + fast stepRate:** Maximum reactivity. Each generation is individually expressed. The modulation output tracks the CA state at nearly full resolution. This produces the most complex, chaotic modulation — every CA step produces a distinct output change.
- **High scope + fast stepRate:** The moving average smooths rapid CA state changes. Even at 20 Hz stepRate, scope=16 makes the modulation output move slowly because 16 recent states are averaged. Good for producing smooth sweeps from fast-evolving rules.
- **Low scope + slow stepRate:** Each generation is individually expressed, but generations come slowly. The modulation holds its value for 500ms or more between updates. Good for creating stepped, discrete modulation events with long holds.
- **High scope + slow stepRate:** The output barely moves. At scope=16 and stepRate=0.5 Hz, a new generation arrives every 2 seconds and contributes 1/16 of the output change. Significant arc takes 32 seconds. This is the geological mode.

**The useful interaction:** Use scope to smooth a fast rule (Rule 30 at 15 Hz with scope=8 produces smooth sweeps from deterministic chaos). Use low scope to hear the structure of a slow rule (Rule 90 at 2 Hz with scope=1 produces the alternating square wave cleanly).

---

### The All-Zero Trap

Every Wolfram rule has the all-zero fixed point: if all 16 cells are dead, the next state is also all-zero (because no cell has living neighbors). ORGANISM prevents this by injecting a single living cell (state = 0x0001) when extinction occurs. But extinction can happen quickly:

- **Rule 0** (all outputs 0): Every state maps to all-zero in one step. The protection fires every step — the colony is permanently a single living cell that immediately dies and is resurrected.
- **Many rules converge to zero rapidly** from sparse initial states. Rule 4, Rule 8, and many others produce rapidly dying colonies from most seeds.
- **Rule 90 from seed 0:** All cells dead from the start — protection fires immediately. But the protection state (0x0001) under Rule 90 is an excellent starting condition for the Sierpinski fractal.

**Guidance:** If ORGANISM sounds static or locked into a single repeating very-short cycle, the CA may have converged to all-zero and the protection is producing a degenerate state. Increase `org_mutate` (even to 0.05) to inject random bit flips that rescue the colony from extinction. Alternatively, sweep the SEED macro to restart from a fresh random state.

---

### The Pitch Quantization Detail

Cell pitch output is quantized to integer semitones via `round(cellPitchOut × 12 - 6)`. This means the pitch modulation steps are always in semitone increments. At slow stepRates, you can hear each semitone change as a distinct pitch shift — ORGANISM plays actual notes. At fast stepRates, the pitch changes blur into a vibrato-like pitch shimmer.

The scope averaging softens this: at scope=6+, the pitch output is an average of multiple generation states and will be fractional even after rounding — the rounding occurs only to the average, so successive different fractional averages still round to the same integer semitone. At scope=1, every generation change produces an immediate semitone jump.

**Musical opportunity:** At scope=1 and stepRate=3–6 Hz, ORGANISM generates a self-composed chromatic melody within ±6 semitones of root. This is a generative melodic voice. Set it in a key context (root note = key center, filter with appropriate resonance) and record several takes with different seeds. Each take produces a unique melody from the same rule.

---

### The Freeze Parameter — Performance and the Taxidermy Trap

`org_freeze` stops the CA evolution at the current state. All modulation outputs hold their smoothed values indefinitely. The freeze creates a static version of whatever the CA was doing at the moment of freezing.

The trap: freezing at the wrong moment locks ORGANISM into an uninteresting configuration. Freeze is most powerful as a performance tool used to capture moments of particularly compelling CA output — a moment when the filter, pitch, and reverb have settled into a musically useful position. The player must listen for these moments and trigger freeze intentionally.

The anti-trap: freeze is also useful for using ORGANISM as a conventional synthesizer. Freeze at scope=8 after 20+ generations will stabilize the modulation outputs at their long-term average positions. This removes the generative behavior and leaves a static, parameterized synthesizer — useful when you want ORGANISM's oscillator and filter character without the cellular evolution.

---

### The Mutation Rate Trap

High `org_mutate` values (0.4+) combined with Rule 30 create sustained noise-generation rather than structured evolution. Every generation, approximately 40% of cells are randomly flipped — the CA rule is overwhelmed by random bit flips and cannot establish any pattern. The result is effectively white noise modulation, not cellular automata modulation.

This is sometimes wanted (Extinction Event presets) but should not be confused with more complex CA behavior. Maximum mutation does not produce the most interesting modulation — it produces the most random modulation. The most interesting CA behavior occurs at mutation rates 0.02–0.12, where occasional bit flips perturb stable patterns without destroying them.

---

### The Note-Number Seeding Trap

Because each MIDI note seeds the CA differently (`caState = seed ^ (note * 257)`), playing a new note restarts the evolution from a new initial condition. The previous CA state is completely replaced. In legato playing (new note before previous release), the pitch envelope transitions smoothly, but the CA evolution discontinuously resets. This creates a characteristic clean break at each new monophonic note.

Design presets so this reset is either musically transparent (fast stepRate means the new colony establishes its pattern quickly) or deliberately exploited (slow stepRate with high scope means each new note starts from a completely different initial state and the listener can hear the colony warming up to its new pattern).

---

## Phase R8: Coupling Strategy

ORGANISM is a rich coupling source because its output signal carries temporal information about the CA state. Unlike a static oscillator whose coupling output is simply the current pitch or amplitude, ORGANISM's output encodes the entire cellular automaton's current behavior — filter sweeps, amplitude gating, pitch modulation — all compressed into the final audio waveform.

### ORGANISM as Source

**AmpToFilter:** ORGANISM's amplitude envelope modulation from cells 4–7 creates rhythmic gating in the output. When used as an AmpToFilter source to a partner engine (e.g., OXBOW, ORBITAL), the partner engine's filter tracks ORGANISM's amplitude rhythm — creating a filter-envelope synchrony between the generative pattern and the conventional engine.

**PitchToPitch:** ORGANISM's pitch modulation from cells 8–11 (±6 semitones) can drive a partner engine's pitch if routed as PitchToPitch coupling. The non-repeating chromatic melody that ORGANISM generates becomes a performance input to the partner engine. The partner engine plays ORGANISM's generative melody.

**AmpToPitch:** ORGANISM's amplitude (including the CA-driven envelope modulation) can drive pitch in a partner engine. As ORGANISM's colony produces high-amplitude moments, the partner engine's pitch rises. This creates amplitude-to-pitch coupling that encodes the colony's population dynamics as melodic information in the partner.

### ORGANISM as Target

ORGANISM accepts `AmpToFilter` and `AmpToPitch`/`PitchToPitch` from partners. The receive sensitivity is scaled by `org_macroCoupling` (0.5 + coupling × 0.5). At coupling=0, sensitivity is 0.5× — coupling is received at half strength. At coupling=1.0, sensitivity is 1.0×  — full coupling depth.

**Useful pairing: OSTINATO → ORGANISM:** OSTINATO's rhythmic patterns (via AmpToFilter coupling) drive ORGANISM's filter modulation in addition to the CA. The rhythm machine and the cellular automaton share a filter — OSTINATO's explicit rhythm and ORGANISM's emergent rhythm interact at the filter output.

**Useful pairing: ORGANON → ORGANISM:** ORGANON's metabolic rate drives ORGANISM's pitch modulation. The biological simulation engine feeds into the cellular automaton engine — two systems modeling living systems at different levels of abstraction, coupled through the filter and pitch.

**The COUPLING macro in practice:** ORGANISM's `org_macroCoupling` adds 0.01× coupling to the mutation rate — a subtle, always-present effect when coupling is active. Even without an explicit coupling source, increasing the COUPLING macro very slightly increases mutation. This means activating coupling changes the colony's behavior even before any coupling signal arrives — the colony becomes more responsive to change when the COUPLING macro is raised.

---

## Phase R9: CPU Profile

ORGANISM is one of the lighter engines in the fleet. Its DSP is:
- **One oscillator voice** (saw, square, or triangle) + one sub oscillator
- **One biquad filter** (updated every sample — moderate cost)
- **Two StandardLFOs** (sine, per sample — negligible)
- **One OrgReverb** (4 combs + 4 allpass — moderate cost per sample)
- **CA step computation** (16-cell loop, executed at stepRate — negligible average cost)
- **SilenceGate** (300ms hold — negligible)

The CA itself is extremely cheap — 16-cell loop executing at most 32 times per second, consuming negligible CPU. The filter (updated per sample) is the primary cost, followed by the reverb (8 delay lines with LP damping). Even in the most demanding configuration (high filter resonance, high reverb mix, fast CA evolution), ORGANISM consumes far less CPU than polyphonic engines like ORBITAL (8 voices) or OWARE (8 voices with mallet physics).

**Optimization note:** The SilenceGate (300ms hold) means ORGANISM does not consume audio-rate DSP in sustained silences. At slow stepRates with long releases, ORGANISM may appear to be active for several seconds after the last note — this is the reverb tail and release envelope consuming CPU. The SilenceGate will engage after 300ms of silence below threshold.

---

## Phase R10: The Guru Bin Benediction

*"ORGANISM arrived in the fleet on 2026-03-20 without ceremony. It was the forty-second engine registered. No seance superlative, no Blessing candidate ratified during intake, no record preset count on day one. It came as cellular automata engines come — quietly, deterministically, carrying rules that require patience before they reveal what they can do.*

*The Great Barrier Reef is not beautiful at the scale of a single polyp. A single coral polyp is not visible to the naked eye. The beauty of the reef exists only at a scale that no individual polyp can perceive. None of the billions of organisms that built the reef knew they were building it. They followed their three rules — grow toward light, deposit calcium, respond to neighbors — and the architecture emerged from the interaction of those rules across space and time.*

*ORGANISM does not generate music. It generates conditions from which music emerges.*

*This distinction matters enormously to how you play it. A synthesizer that generates music accepts a performance input and produces a musical output. The relationship between input and output is defined, predictable, designed. You press C3 and you hear C3. ORGANISM accepts a performance input — a MIDI note, a seed, a rule — and initiates a process. The process produces audio. Whether that audio is musical depends on the rule, the seed, the stepRate, the scope, and on your patience as a listener.*

*Rule 110 is computationally universal. Stephen Wolfram proved that it can simulate any Turing machine. This means that given an appropriate initial state on an infinite tape, Rule 110 can compute anything that any computer can compute. ORGANISM's 16-cell ring is not infinite, so it cannot simulate arbitrary Turing machines. But it can produce complex, non-repeating behavior that contains, encoded in its cellular states, patterns that no designer specified. The patterns were not put there. They emerged from the rule.*

*The eight curated rules are eight different laws for the same universe. Rule 90 says: every cell becomes the XOR of its neighbors. Rule 30 says: the left neighbor gates a complex three-way function. Rule 184 says: cells move rightward when possible. Each law produces a different universe. The coral colony grows differently under each law — sometimes orderly, sometimes chaotic, sometimes structured but complex. The macro RULE sweeps through these universes in sequence.*

*What this means for production: ORGANISM presets are not sounds. They are sound-generation systems. A preset specifies the parameters of the system — which rule, which seed, what rate, what memory — and then the system generates sounds continuously for as long as notes are held. No two bars of ORGANISM playing Rule 110 are the same. The second bar's CA state is the first bar's state evolved by the rule. The twenty-fourth bar's state is unknowable at bar one without running the simulation.*

*This is the patience parameter. It is not a slider or a knob. It is the willingness to hold a note for twenty seconds and listen to what the colony does. To let the filter sweep through positions you did not plan. To hear the pitch modulation generate a minor third above root — not because you programmed it — because the cellular automaton reached a population density in cells 8–11 that maps to a minor third. The minor third emerged. You did not choose it. You only chose the rule.*

*The coral colony does not know it is making a reef. The cellular automaton does not know it is making music. Neither the polyp nor the cell has any awareness of the architecture it is building.*

*That is not a failure of awareness. That is the definition of emergence.*

*Grow toward light. Deposit calcium. Respond to neighbors.*

*The reef will come."*

---

## Appendix: Full Parameter Reference

| Parameter | ID | Range | Default | Notes |
|-----------|-----|-------|---------|-------|
| CA Rule | `org_rule` | 0–255 | 110 | Wolfram rule byte. Use macroRule for curated sweep. |
| Seed | `org_seed` | 0–65535 | 42 | XORed with note number × 257 at note-on |
| Step Rate | `org_stepRate` | 0.5–32.0 Hz | 4.0 | CA generations per second |
| Scope | `org_scope` | 1–16 | 4 | Moving average window across recent generations |
| Mutate | `org_mutate` | 0.0–1.0 | 0.0 | Bit-flip probability per cell per step |
| Freeze | `org_freeze` | bool | false | Stops CA evolution — holds current modulation state |
| Osc Wave | `org_oscWave` | 0/1/2 | 0 (saw) | 0=Saw, 1=Square, 2=Triangle |
| Sub Level | `org_subLevel` | 0.0–1.0 | 0.35 | Sub oscillator (1 oct below, square wave) |
| Filter Cutoff | `org_filterCutoff` | 200–8000 Hz | 3000 | Base cutoff — CA cells 0–3 add ±40% |
| Filter Res | `org_filterRes` | 0.0–0.9 | 0.3 | Maps to Q 0.5–12.0 |
| Vel Cutoff | `org_velCutoff` | 0.0–1.0 | 0.5 | Velocity → +0 to +3000 Hz cutoff boost |
| Amp Attack | `org_ampAtk` | 0.001–2.0s | 0.015 | Envelope attack (further modulated by cells 4–7) |
| Amp Decay | `org_ampDec` | 0.05–4.0s | 0.35 | Envelope decay (modulated by cells 4–7) |
| Amp Sustain | `org_ampSus` | 0.0–1.0 | 0.70 | Sustain level |
| Amp Release | `org_ampRel` | 0.05–5.0s | 0.60 | Envelope release (modulated by cells 4–7) |
| LFO1 Rate | `org_lfo1Rate` | 0.01–10.0 Hz | 0.50 | Filter cutoff modulation (±600 Hz at depth=1) |
| LFO1 Depth | `org_lfo1Depth` | 0.0–1.0 | 0.20 | LFO1 depth scalar |
| LFO2 Rate | `org_lfo2Rate` | 0.01–10.0 Hz | 0.30 | Filter cutoff modulation (±1000 Hz at depth=1) |
| LFO2 Depth | `org_lfo2Depth` | 0.0–1.0 | 0.25 | LFO2 depth scalar |
| Reverb Mix | `org_reverbMix` | 0.0–1.0 | 0.20 | Base reverb (CA cells 12–15 add up to +0.3) |
| RULE macro | `org_macroRule` | 0.0–1.0 | 0.25 | Sweeps curated rules: 30→90→110→184→150→18→54→22 |
| SEED macro | `org_macroSeed` | 0.0–1.0 | 0.0 | Re-seeds CA when above 0.01 threshold (once per gesture) |
| COUPLING macro | `org_macroCoupling` | 0.0–1.0 | 0.0 | Scales coupling receive depth + trace mutation boost |
| MUTATE macro | `org_macroMutate` | 0.0–1.0 | 0.0 | Additional mutation on top of org_mutate |

**Expression (D006):**
- Velocity → +0–3000 Hz filter cutoff via `org_velCutoff`
- Aftertouch → +0–0.30 additional mutation rate
- Mod wheel CC1 → +0–2 positions in rule index (toward higher-indexed curated rules)
- Pitch wheel → frequency ratio via PitchBendUtil (±2 semitones default)

---

*Retreat conducted 2026-03-21. Engine: ORGANISM (Coral Colony, Emergence Lime #C6E377). 16-cell Wolfram CA synthesis. Monophonic generative engine. Six recipe categories defined including the Patience Parameter. Ten reference presets detailed. Parameter map complete.*
*The colony does not rush. The reef was not planned. The music emerges.*
