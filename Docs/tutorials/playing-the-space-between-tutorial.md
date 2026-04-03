# Playing the Space Between
## A Performance Guide to XOceanus Coupling

**Version:** 1.0 — March 2026
**Audience:** Producers familiar with synthesis who are new to inter-engine coupling
**Prerequisite:** Basic familiarity with XOceanus's four-slot engine architecture

---

## Introduction: The Space Between Engines

Most synthesizers give you one voice. Layer two voices and you still get two separate sounds playing together — coexisting but not talking to each other. XOceanus is built around a different idea.

Coupling is not routing. Routing means signal from engine A arrives at the input of engine B. Coupling means the *behavior* of engine A reshapes the *parameters* of engine B while both are playing their own material. The drum hit doesn't go into the bass — the drum hit changes *how* the bass exists for a moment. The LFO in one engine doesn't drive the pitch of another — it changes where the other engine looks in its wavetable, which changes its timbre, which changes its relationship to what you're playing.

This is the space between engines. It is where XOceanus sounds unlike anything else.

When you hear a coupling preset correctly, the two engines become a single organism with more behavioral complexity than either could produce alone. The mallet hits pump the reverb tail. The cellular automata engine decides where in a spectral buffer the shimmer engine looks for material. The drum machine's amplitude envelope dictates when the bass breathes. None of this is routing — it is conversation.

This guide walks you through the coupling architecture, the Performance View, five core techniques, and three worked examples. By the end you will understand how to use coupling as a live performance tool, not just a preset feature.

---

## Part 1: The 14 Coupling Types

XOceanus implements fourteen distinct coupling types. Each defines a *relationship* between source engine output and destination engine parameter. Learn what each one does and you will hear the vocabulary of every Entangled preset in the library.

The coupling types split naturally into four groups: amplitude modulation, LFO/envelope modulation, audio-rate modulation, and topological coupling.

### Amplitude Modulation Group

**AmpToFilter** — The source engine's amplitude envelope modulates the destination engine's filter cutoff. Loud transients open the filter; quiet passages close it. This is the coupling type most analogous to sidechain compression, but instead of ducking volume, it sculpts timbre. The kick drum opens the bass filter. The mallet strike brightens the reverb tail.

**AmpToPitch** — The source engine's amplitude envelope bends the destination engine's pitch. A strong attack causes a brief pitch rise; decay causes pitch fall. Useful for expressive, organic pitch behavior that tracks the energy of the source engine rather than a fixed envelope curve.

**AmpToChoke** — The source engine's amplitude above a threshold silences (chokes) the destination engine. This is the electronic drum hi-hat relationship — the open hat and closed hat cannot coexist. Use it for any situation where one engine should silence the other when it is loud.

### LFO and Envelope Modulation Group

**LFOToPitch** — The source engine's internal LFO signal drives pitch modulation in the destination engine. The destination engine's pitch wobbles at the rate and depth of the source engine's LFO, not its own. Two engines can share a single modulation source this way, creating synchronized modulation that no standalone synth can produce because it would require external routing.

**EnvToMorph** — The source engine's amplitude envelope sweeps the destination engine's wavetable or morph parameter. As the source plays and decays, it drags the destination through its morphological space. A pluck causes a morph scan; a pad swell causes a slow morph journey. Particularly expressive with OPAL (granular) as the destination.

**EnvToDecay** — The source engine's envelope controls the destination engine's decay time. A longer attack on the source extends the destination's decay; a short source envelope produces short, snappy decay on the destination. Drums that set their own reverb tail length.

**FilterToFilter** — The source engine's filter output is injected into the destination engine's filter input. This creates serial timbral filtering — two filter stages in conversation, each contributing to the spectral shape of what reaches the output. The combined resonance behavior can be richer and stranger than a single filter chain.

**PitchToPitch** — The source engine's current pitch is transmitted to the destination engine as a pitch offset, creating automatic harmony. Play one note and both engines track with a fixed interval between them. Interval is set by the coupling amount. This is the coupling type that most directly replaces a harmonizer.

**RhythmToBlend** — The source engine's internal rhythm or pattern gates the destination engine's blend parameter. The destination's wet/dry or layer balance pulses in sync with the source engine's rhythmic output. Used in Coupled Bounce and related trap presets to synchronize the drum machine's groove with the bass character.

### Audio-Rate Modulation Group

**AudioToFM** — The source engine's audio output, at full audio rate, modulates the destination engine's FM carrier ratio or index. This is not control-rate decimation — it is audio-rate cross-FM. The source's tonal content becomes the modulator waveshape for the destination's FM synthesis. Use for rich, harmonically complex timbres that would be impossible with a single FM engine.

**AudioToRing** — The source engine's audio output is ring-modulated against the destination engine's audio output. Mathematically: (source × destination). Both engines continue playing independently but their combined output contains sum and difference frequencies that neither produces alone. Metallic, inharmonic, alien.

**AudioToWavetable** — The source engine's audio output is periodically injected into the destination engine's wavetable source. The source's current waveform shape becomes the destination's oscillator waveform, captured every N blocks. The destination essentially plays waveforms drawn by the source engine.

**AudioToBuffer** — The source engine's audio output streams continuously into the destination engine's ring buffer. Currently wired to OPAL as the destination — OPAL's granular engine pulls from this ring buffer for grain material. Load any engine into slot A, set AudioToBuffer coupling to OPAL in slot B, and OPAL becomes a time telescope pointed at engine A: scattering the other engine's audio across time and pitching it in clouds.

### Topological Coupling

**KnotTopology** — Bidirectional irreducible coupling. Both engines modulate each other simultaneously in the same processing block: A modulates B's filter, then B modulates A's filter, in the same audio frame. Neither engine is the source or the destination — they are entangled. Removing one breaks the patch. The linking number (encoded in the coupling depth parameter) sets entanglement depth from 1 (loose) to 5 (deeply entangled, strong mutual modulation). This is the coupling type unique to the Singularity Collection — the OXBOW and ORBWEAVE engines were designed specifically for it.

---

## Part 2: The Performance View

The Performance View is accessed via the **P** button in the XOceanus header. It replaces the Overview Panel with a real-time coupling control surface built for live use.

### Layout

The Performance View has three sections:

**Left: CouplingStripEditor** — A node-and-arc visualization of the four engine slots and their active coupling routes. Each engine slot displays its accent color. Active routes appear as arcs between nodes. Bidirectional routes (KnotTopology) show as double arcs. This is your visual reference for the current coupling state — glance at it during performance to understand what is connected to what.

**Right: Route Detail Panel** — Four route slots, each with:
- **ON toggle** — activates or deactivates this route
- **Type dropdown** — all 15 coupling types (AudioToFM, KnotTopology, AmpToFilter, etc.)
- **Depth slider** — bipolar, −1.0 to +1.0. Negative values invert the coupling: AmpToFilter with negative depth means loud source *closes* the filter rather than opening it.
- **Source and Target slot selectors** — which of the four engine slots is source, which is destination

**Bottom: Macro Strip** — Four XO Gold macro knobs: CHARACTER, MOVEMENT, COUPLING, SPACE. These are the same four macros present in every preset. In the Performance View, they are always visible and always accessible, regardless of which engines are loaded. This is by design: macros are your hands.

### BAKE and CLR

**CLR** resets all four route slots to inactive. Use it to clear experimental routing without leaving the Performance View.

**BAKE** captures the current coupling performance state — active routes, types, depths, source/target assignments — as a named coupling preset. The BakedCouplingState stores engine names by canonical ID, not slot index, so recalled coupling presets survive engine slot reordering. Click BAKE, name the state, and it appears in the Coupling Presets dropdown for instant recall.

### The Coupling Preset Dropdown

The dropdown at the top of the Route Detail Panel loads saved coupling presets (BakedCouplingState objects). These are separate from the full `.xometa` presets in the Preset Browser — they capture only the performance overlay: routes, depths, and macro values. Load any `.xometa` preset, then load a coupling preset on top of it, and you have combined two independent design layers.

### Audio Thread Safety

The Performance View writes to APVTS parameters (`cp_r1_active`, `cp_r1_type`, `cp_r1_depth`, etc.) on the message thread. The MegaCouplingMatrix reads route changes atomically via a lock-free double-buffer (std::atomic_store / std::atomic_load). Route changes take effect within one audio block — typically under 3ms at 48 kHz / 128-sample buffers. You will not hear pops or glitches when toggling routes in real time, and coupling type changes on audio-rate routes crossfade over 50–100ms to prevent discontinuities.

---

## Part 3: Five Performance Techniques

These five techniques are the core vocabulary of live coupling performance. Each names a gesture and explains how to execute it in the Performance View.

### Technique 1: The Fade

**What it is:** The crossfader between two coupled engines as a primary performance gesture — not volume crossfade, but coupling depth crossfade.

**How to execute:** Load a two-engine preset. In the Performance View, map the COUPLING macro to the depth slider of one or more routes. As you sweep COUPLING, the coupling deepens or weakens, transforming the character of both engines simultaneously. At zero: two engines playing side by side, independent. At maximum: one engine's behavior is deeply written into the other.

**What it sounds like:** In an OWARE × OXBOW preset, sweeping COUPLING from 0 to full with AmpToFilter coupling makes the reverb tail increasingly reactive to mallet transients. At low coupling, the reverb decays naturally. At high coupling, every mallet hit pumps the reverb's filter character with its attack shape.

**Practical note:** The Fade is most effective when you are performing live in a DAW arrangement view. Automate the COUPLING macro over four bars. Use it to mark structural transitions — intro (low coupling, engines independent) to drop (high coupling, engines entangled).

### Technique 2: The Push

**What it is:** Using aftertouch to modulate coupling depth in real time, adding expressive dimensional control to a note that is already sounding.

**How to execute:** In most engines, aftertouch is mapped to filter cutoff or modulation depth by default. When AmpToFilter or LFOToPitch coupling is active, increasing aftertouch on the source engine increases its modulation signal amplitude, which deepens the coupling effect in the destination engine. You are pushing the coupling signal without touching the Performance View at all — it is a consequence of the engine's expression wiring.

**What it sounds like:** Playing an ORGANISM (cellular automata) preset coupled to OVERTONE via LFOToPitch: at rest, OVERTONE plays its spectral synthesis normally. As you push aftertouch on ORGANISM, its LFO deepens, which drives stronger pitch modulation in OVERTONE. The spectral engine begins to wobble and drift in sync with how hard you are pressing.

**Practical note:** This technique requires choosing a source engine with meaningful aftertouch output (D006 doctrine — all 23 MIDI-capable engines have it). OUROBOROS (chaos attractor), OBRIX (modular brick), and ORGANON (metabolic) have particularly rich aftertouch responses that create complex coupling signals when pushed.

### Technique 3: The Freeze

**What it is:** Locking the coupling state at a peak moment — capturing the relationship between engines at its most entangled and sustaining it while you play.

**How to execute:** In the Performance View, bring coupling depth to maximum with one or more active routes. Let both engines reach their expressive peak — filters wide open, FM deeply interactive, whatever the type dictates. Then use the BAKE button to snapshot the state. Load the baked preset any time you want to return to that exact coupling configuration instantly.

Alternatively: in AudioToBuffer coupling with OPAL as destination, OPAL has an internal grain freeze parameter. When OPAL freezes its grain buffer, the coupling is implicitly locked — OPAL continues granulating from the frozen moment of the source engine's audio, even as the source continues playing. This is a sonic freeze, not a parameter freeze: the moment is suspended in grain space.

**What it sounds like:** A freeze during an ORBWEAVE × OPENSKY AudioToFM route: at the coupling peak, OPENSKY's shimmer is most heavily FM-modulated by the knot topology signals from ORBWEAVE. Baking that state and recalling it during the mix means you can always return to the most complex, maximal version of the patch.

**Practical note:** Use BAKE generously. It is non-destructive — it creates a new entry in the Coupling Presets dropdown without modifying any existing preset. Build a performance set of five or six baked states representing different coupling intensities for the same engine pair, and treat them like preset scenes.

### Technique 4: The Drift

**What it is:** Very slow AmpToFilter or EnvToMorph coupling used as passive background modulation — a field of slow change underneath a static-seeming sound.

**How to execute:** Load any two-engine preset. Set a single AmpToFilter route at very low depth (0.08 to 0.15). Set the source engine to produce very slow, breath-like amplitude modulation — either a long envelope or a very slow LFO assigned to output amplitude. The result is filter movement in the destination that happens over 8–30 seconds, beneath the threshold of deliberate perception but above the threshold of absence.

**What it sounds like:** ORGANISM coupled to OVERDUB via AmpToFilter at depth 0.10, with ORGANISM's CA engine producing slow population wave outputs: the dub echo's filter opens and closes at a geological pace — over minutes, not beats. The listener does not hear modulation. They hear a sound that somehow stays alive.

**Practical note:** The Drift is an arrangement tool disguised as a technique. In a long ambient track, a Drift coupling preset can make a static pad feel inhabited for thirty minutes without any additional automation. It is the coupling equivalent of very slow sample-and-hold — always slightly somewhere else, never jarringly different.

### Technique 5: The Conversation

**What it is:** Two engines mutually coupled — A modulates B and B modulates A simultaneously — creating feedback-driven timbral emergence that neither engine designed.

**How to execute:** In the Performance View, activate two routes:
- Route 1: Engine A as source → Engine B as destination (e.g., AmpToFilter, depth 0.4)
- Route 2: Engine B as source → Engine A as destination (e.g., LFOToPitch, depth 0.3)

The amounts should be moderate — if both are at 1.0, the mutual coupling can create unstable resonances (which can be intentional). Start at 0.3–0.5 on both sides.

Alternatively, use a single KnotTopology route — this implements the Conversation internally with full audio-thread precision and a linking number that scales both directions symmetrically.

**What it sounds like:** OUROBOROS (chaos attractor) and ORGANON (metabolic synthesis) in conversation: OUROBOROS's amplitude modulates ORGANON's filter, while ORGANON's LFO modulates OUROBOROS's pitch. ORGANON responds to chaos; OUROBOROS responds to ORGANON's metabolic state. The result is a coupled system with emergent oscillation — a new kind of dynamics neither engine contains alone.

**Practical note:** The Conversation is most stable when source types are mismatched: AmpToFilter in one direction, LFOToPitch in the other. Matched types (AmpToFilter both ways) approach the KnotTopology behavior and can create resonance cascades. Those are not always wrong — sometimes they are exactly the sound — but be ready to reduce depth or add CLR.

---

## Part 4: Three Worked Examples

### Example 1: Mallet Hits Pump the Reverb
**Engines:** OWARE (slot A) + OXBOW (slot B)
**Coupling type:** AmpToFilter
**Starting preset:** `COUPLING_OWARE_x_OXBOW_Mallet_Pump.xometa`

OWARE is a tuned percussion synthesizer with eight material modes — wood, metal, bell, bowl, and others. OXBOW is an entangled reverb synth built around a Chiasmus Feedback Delay Network. When OWARE's amplitude envelope drives OXBOW's filter cutoff via AmpToFilter coupling, every mallet strike opens the reverb's spectral character at exactly the same moment as the attack, then closes it as the transient decays.

**Setup:**
1. Load OWARE in slot A, OXBOW in slot B.
2. Open the Performance View (P button).
3. Activate Route 1: Source = Slot A (OWARE), Target = Slot B (OXBOW), Type = AmpToFilter, Depth = 0.55.
4. Play any note on a keyboard or pad controller. Each attack pumps the reverb filter open.
5. Adjust OWARE's `owr_material` to 2 (Metal) for a brighter transient — more filter pumping per hit.
6. Adjust OXBOW's `oxb_entangle` to 0.6 to add internal FDN feedback — the pumped reverb tail develops complex harmonic resonance.
7. Map the COUPLING macro to Route 1 depth for live control.

**The sound:** Each mallet hit produces not just a reverb tail — it produces a reverb tail that briefly sounds like the inside of the instrument rather than the space around it. The filter follows the physical transient of the mallet articulation.

**Experiment:** Set coupling depth to a negative value (−0.35). Now loud mallet hits close the reverb filter — the room goes dead on impact and breathes open on the decay. A completely different relationship between percussion and space.

---

### Example 2: The Cellular Automata Pilot
**Engines:** ORGANISM (slot A) + OVERTONE (slot B)
**Coupling type:** LFOToPitch
**Starting preset:** `COUPLING_ORGANISM_x_OVERTONE_CA_Pilot.xometa`

ORGANISM is a cellular automata generative engine — its internal rule sets produce complex LFO-like outputs that evolve over time based on population dynamics rather than waveform oscillation. OVERTONE is a continued-fraction spectral engine that derives timbre from rational approximations to π, e, φ, and √2. When ORGANISM's CA output drives pitch in OVERTONE via LFOToPitch coupling, the spectral engine's pitch follows a deterministic but non-repeating modulation path that no standalone LFO could produce.

**Setup:**
1. Load ORGANISM in slot A, OVERTONE in slot B.
2. Open Performance View. Activate Route 1: Source = Slot A, Target = Slot B, Type = LFOToPitch, Depth = 0.35.
3. Play a held note on OVERTONE. The pitch begins to drift according to ORGANISM's CA output.
4. Change ORGANISM's `org_ruleSet` parameter. Each rule set produces a different modulation character — some produce slow undulating drift, some produce chaotic jumps, some produce long-period repetition.
5. Set ORGANISM's `org_density` to 0.5 for medium CA population activity. Higher density = faster, more active pitch modulation.
6. Increase LFOToPitch depth to 0.6 for wider pitch deviation. Reduce to 0.15 for subtle pitch breathing.

**The sound:** A spectral pad whose pitch slowly finds and loses stability according to a living cellular system. Not vibrato — a pitch that evolves according to rules rather than shapes. Each performance is slightly different because the CA engine's output path never exactly repeats at human-audible timescales.

**Experiment:** Add a second route: OVERTONE as source → ORGANISM as destination, type = AmpToFilter, depth = 0.2. This completes the Conversation: ORGANISM drives OVERTONE's pitch; OVERTONE's amplitude influences ORGANISM's internal filter state. The two engines develop a feedback relationship. The coupling becomes a shared behavioral space.

---

### Example 3: Chaos Feeds Metabolism
**Engines:** OUROBOROS (slot A) + ORGANON (slot B)
**Coupling type:** AudioToFM
**Starting preset:** `COUPLING_OUROBOROS_x_ORGANON_Chaos_Metabolic.xometa`

OUROBOROS generates audio from a strange attractor — its output is deterministically chaotic waveforms shaped by Lorenz, Rossler, and other attractor topologies. ORGANON synthesizes sound through a metabolic model with Variational Free Energy dynamics (Blessing B011). Its coupling input handling is rated score 5 in the coupling audit — it treats incoming audio as metabolic input, ingesting it per-voice rather than as a global signal.

When OUROBOROS's audio drives ORGANON via AudioToFM, the chaos attractor's waveform becomes the FM modulator for every voice ORGANON produces. No two notes ever have exactly the same FM modulation because the attractor is at a different position in its phase space at each attack.

**Setup:**
1. Load OUROBOROS in slot A, ORGANON in slot B.
2. In Performance View: Route 1 = AudioToFM, Source Slot A → Dest Slot B, Depth = 0.55.
3. Set OUROBOROS's `ouro_topology` to 1 (Lorenz attractor). Its audio output becomes chaotic waveforms derived from the Lorenz equations.
4. Set ORGANON's `organon_metabolicRate` to 0.65 for active metabolic dynamics. Lower values produce a slower-responding organism.
5. Play chords. Each voice in ORGANON gets a different slice of the attractor's phase trajectory as its FM modulator — polyphony with inherent timbral variance.
6. Experiment with OUROBOROS's `ouro_leash` parameter (Blessing B003): high leash values constrain the attractor near its center, producing more tonal FM modulation; low leash values allow wide chaos excursions, producing increasingly inharmonic FM results.

**The sound:** Organic, never-repeating harmonic complexity. Chords that breathe and mutate because the FM modulator is a living chaotic system. The longer you hold notes, the further the attractor travels from its initial state, gradually transforming the timbre of held voices.

**Experiment:** Add Route 2: ORGANON amplitude → OUROBOROS FM input via AmpToFilter at depth 0.25. Now the organism's metabolic output modulates the chaos engine's self-modulation. This is a closed loop: chaos feeds metabolism, metabolism modulates chaos. The system is now genuinely co-evolving rather than one-directional.

---

## Part 5: Tips for Live Performance

### CPU Management

Coupling adds processing load because the MegaCouplingMatrix runs additional operations per block for each active route. The system is optimized — control-rate coupling types (AmpToFilter, LFOToPitch, EnvToMorph, etc.) use SRO decimation at 1:32 ratio, sampling the source every 32nd sample and interpolating between control points. This reduces CPU cost for modulation coupling by approximately 97% compared to audio-rate sampling.

Audio-rate coupling types (AudioToFM, AudioToRing, AudioToWavetable, AudioToBuffer, KnotTopology) run at full sample rate and have proportionally higher cost. If CPU is constrained:
- Start with one audio-rate route per session rather than four
- AudioToBuffer (OPAL grain streaming) is the heaviest single route — use it when it is the featured sound, not as background texture
- KnotTopology processes two directions per block; treat it as two routes for CPU planning

For live performance, test your full routing configuration during soundcheck at the target buffer size. A 256-sample buffer in the studio may become a 512-sample buffer live — coupling routes that were borderline at 256 will be comfortable at 512.

### Mono vs. Stereo Coupling Sends

Audio-rate coupling routes (AudioToFM, AudioToRing) perform a per-sample L+R to mono mixdown before sending to the destination engine. This is correct and intentional — FM modulation index is a scalar value, not a stereo pair. The mono mixdown preserves level integrity: a hard-panned source contributes at half the amplitude it would if only one channel were summed.

AudioToBuffer is the sole exception — it preserves true stereo and writes both L and R channels into OPAL's grain ring buffer. If you want stereo grain material, AudioToBuffer is the only coupling type that delivers it.

FilterToFilter is also mono-summed at the coupling bus but typically sounds correct because filter output coloration is a spectral characteristic that reads as timbral, not spatial.

### Coupling as Arrangement Tool

The most underutilized application of coupling is static: configure a coupling relationship that changes the character of both engines, then leave it unchanged for an entire section of a track. The coupling is not moving — it is simply present, changing what both engines fundamentally are for that section.

Example: A verse that uses OWARE and OXBOW with coupling amount 0.0 (two independent engines), a pre-chorus that gradually raises COUPLING macro to 0.4, and a chorus that arrives with coupling at 0.8. Each section has the same two engines loaded, but they sound like different instruments because the relationship between them is different. The transition is not a filter sweep — it is a change in how the instruments relate to each other.

Entangled presets in the library are built around specific coupling amounts that define their identity. The factory coupling amount is the preset's starting position — the COUPLING macro lets you modulate around it up and down.

### Normalled Routes and User Overrides

The MegaCouplingMatrix supports a normalled route system (the Normalled Matriarch pattern). Some engine pairs have pre-wired default routes based on their design compatibility. Loading an engine pair may automatically engage a normalled AmpToFilter route, for instance, without any user configuration. Normalled routes appear in the CouplingStripEditor visualization.

When you add a user route with the same source, destination, and type as a normalled route, the user route takes priority and the normalled route is muted. Removing the user route re-engages the normalled one. This means you can override defaults without losing them — the normalled route is still there, waiting beneath your experiment.

### Starting Points

If you are new to coupling, begin with these progressions:

1. Load any preset from the Entangled mood folder. The coupling is already configured. Play it. Listen for where one engine is driving the other.
2. Open the Performance View and find the active route. See which type is being used and at what depth. Read the source and destination labels.
3. Slowly move the depth slider to zero. Listen to both engines go independent. Move it back up. That motion is coupling working.
4. Change the coupling type in the dropdown. Hear a different relationship emerge.

The Entangled preset library contains over fifty factory presets — each is a worked example of a specific coupling relationship. They are the best documentation of what coupling sounds like. Play them before building your own.

---

## Appendix: Quick Reference

### Coupling Type Summary

| Type | Group | Source Signal | Destination Target | Best For |
|------|-------|---------------|-------------------|----------|
| AmpToFilter | Amplitude | Source amplitude envelope | Dest filter cutoff | Transient-driven filter opening |
| AmpToPitch | Amplitude | Source amplitude envelope | Dest pitch | Organic pitch tracking |
| AmpToChoke | Amplitude | Source amplitude (threshold) | Dest output mute | Hi-hat open/closed, gate chaining |
| LFOToPitch | LFO/Env | Source LFO output | Dest pitch | Shared modulation source |
| EnvToMorph | LFO/Env | Source amplitude envelope | Dest morph/wavetable scan | Envelope-driven timbre scan |
| EnvToDecay | LFO/Env | Source envelope | Dest decay time | Self-setting reverb tails |
| FilterToFilter | LFO/Env | Source filter output | Dest filter input | Serial spectral shaping |
| PitchToPitch | LFO/Env | Source pitch CV | Dest pitch offset | Automatic harmony |
| RhythmToBlend | LFO/Env | Source rhythm gates | Dest blend parameter | Groove-synced blend |
| AudioToFM | Audio-rate | Source audio (stereo→mono) | Dest FM modulator input | Cross-engine FM synthesis |
| AudioToRing | Audio-rate | Source audio | Dest × source multiplication | Inharmonic metallic textures |
| AudioToWavetable | Audio-rate | Source audio (periodic snapshot) | Dest wavetable shape | Live waveform injection |
| AudioToBuffer | Audio-rate | Source audio (stereo stream) | OPAL grain ring buffer | Time telescope granular |
| KnotTopology | Topological | Both engines simultaneously | Both engines simultaneously | Deep mutual entanglement |

### Performance View Parameter IDs

The four route slots use APVTS parameters named `cp_r{N}_{field}` where N is 1–4:
- `cp_r1_active` through `cp_r4_active` — route enable toggles
- `cp_r1_type` through `cp_r4_type` — coupling type (integer index into the 14-type list)
- `cp_r1_depth` through `cp_r4_depth` — coupling depth, bipolar −1.0 to +1.0

These are automatable in any DAW that supports APVTS parameter automation.

---

*XO_OX Designs — Playing the space between since the first two engines were loaded.*
