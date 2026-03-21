# Playing the Space Between
## A Producer's Guide to XOmnibus Coupling Performance

---

## 1. The Space Between

Every synth gives you a sound. XOmnibus gives you a *relationship*.

When you load two engines and draw a coupling route between them, something happens that neither engine could produce alone. Engine A begins sending a signal — its amplitude, its envelope, its LFO, its raw audio — and Engine B begins responding. The character of Engine B changes. It breathes differently. It filters differently. It decays differently. And what you hear is no longer Engine A or Engine B. It is the space between them.

This is what coupling means in XOmnibus. Not layering. Not mixing. Not routing audio in parallel. *Coupling* — one engine's behavior reshaping another engine's synthesis in real time, at a structural level.

The aquatic mythology helps here. Think of two creatures sharing the same current. The comb jelly does not hand the anglerfish a signal. The comb jelly's neural pulse changes the water around it. The anglerfish, suspended in that same water, responds to the change — its filter opens, its pitch drifts, its character shifts in ways that are entirely its own but inextricable from the comb jelly's presence. That relationship, that shared water, is the coupling.

You cannot record the space between two engines and play it back without both engines present. It is alive only while both are running.

---

## 2. Getting Started — Your First Coupling

### Loading Two Engines

XOmnibus gives you four engine slots in the main view. Each slot is a node in the visual Coupling Matrix. For your first coupling, start with two:

1. Load ONSET (or any engine with a strong rhythmic output) into Slot 1.
2. Load OVERBITE (or any engine with a filter-driven character) into Slot 2.
3. Navigate to the Performance View by pressing the **P** button in the header.

The Coupling Matrix shows two circular nodes, one for each loaded engine, connected by arcs for each active coupling route. Empty slots appear as greyed-out circles. Active routes are drawn as colored arcs between the nodes — the arc color blends the accent colors of the two engines, and the arc thickness reflects the coupling amount.

### Choosing a Coupling Type

In the Route Detail panel on the right side of the Performance View, you will see four route slots. Activate Route 1 by clicking its enable toggle, then:

- **Source:** Select Slot 1 (ONSET)
- **Target:** Select Slot 2 (OVERBITE)
- **Type:** Choose `Amp->Filter`
- **Depth:** Set to 0.30 to start

This creates a single coupling route. ONSET's amplitude — how loud it is at any given moment — will now drive OVERBITE's filter cutoff. When ONSET is loud, OVERBITE's filter opens. When ONSET falls silent, OVERBITE's filter closes.

### The Crossfader

The **Depth** slider for each route runs from -1.0 to +1.0. This is the coupling crossfader — not a mixer between two signals, but a control over how deeply one engine influences the other.

- At 0.0, the coupling is inactive even if the route is enabled.
- At 0.30 (Whisper intensity), the influence is subtle — you hear OVERBITE behaving mostly like itself, with a gentle breath from ONSET.
- At 0.50 (Dialogue intensity), the two engines are equal partners. ONSET's rhythm is legible in OVERBITE's filter movement.
- At 0.80 (Possession intensity), ONSET dominates. OVERBITE's filter is largely rewritten by ONSET's dynamics. OVERBITE no longer sounds like itself in isolation.

Start at 0.30. Play a note on OVERBITE while ONSET runs. Then slowly raise the depth. Notice at what point the coupling becomes audible. Notice at what point it becomes the dominant character of the sound. The crossfader is your primary expressive tool.

### Step-by-Step: Your First Coupled Sound

1. Load ONSET into Slot 1. Set a simple kick and snare pattern.
2. Load OVERBITE into Slot 2. Find a bass preset with an open filter.
3. Open Performance View (P button).
4. Enable Route 1. Source: Slot 1. Target: Slot 2. Type: `Amp->Filter`. Depth: 0.30.
5. Play the pattern. Hear the filter on OVERBITE breathe with the kick.
6. Raise depth to 0.50. The filter opens wider on each kick.
7. Add Route 2. Source: Slot 1. Target: Slot 2. Type: `Env->Decay`. Depth: 0.20.
8. Now each kick extends OVERBITE's decay time as well. The bass holds longer on loud hits.

You have just built a sidechain-coupled bass that does not duck — it breathes.

**Try this:** Mute ONSET. OVERBITE plays on its own. Its filter is closed, its decay is short. Unmute ONSET. The drum pattern rewrites OVERBITE's character. The coupling is the performance.

---

## 3. The 14 Coupling Types — A Field Guide

XOmnibus implements 14 coupling types, organized here by character. Some are subtle currents; some are full possession.

### Subtle — Control-Rate Modulation

These types exchange slowly-evolving signals (amplitude envelopes, LFO states, pitch values). They are inherently smooth — you can switch between them mid-performance without audio artifacts.

**`Amp->Filter`** — The foundational coupling. Source amplitude opens or closes the target's filter cutoff. A loud source brightens the target; a silent source darkens it. Universally supported across all 44 engines. Use this first when exploring any new engine pair.

**`Amp->Pitch`** — Source amplitude bends the target's pitch. At low amounts, this creates gentle vibrato-like instability tied to the source's dynamics. At high amounts, loud notes from the source push the target sharp. Musical with percussive sources; dangerous with sustained pads.

**`LFO->Pitch`** — Source LFO signal modulates target pitch. The target's tuning begins breathing at the source engine's LFO rate and depth. Two engines with LFOs at slightly different rates create a slow beating that neither engine could generate alone. Works especially well with OVERDUB or OBBLIGATO as sources.

**`Env->Morph`** — Source amplitude envelope sweeps the target's morph or position parameter. For granular engines like OPAL, this moves the grain scan position. For wavetable engines, it shifts the wavetable frame. For OWARE, it moves the material continuum from wood toward metal. Produces some of the most musical coupling results in the fleet.

**`Env->Decay`** — Source envelope extends or compresses the target's decay time. Loud source signals mean longer target decay. Used to create percussion-driven sustain behavior: the kick drum tells the bass how long to ring.

**`Amp->Choke`** — Source amplitude silences the target (voice choking). When the source plays, the target falls quiet. The classic drum machine hi-hat open/closed relationship. Currently supported in ONSET as a target; use to create auto-duck behavior or rhythmic gating tied to another engine's dynamics.

**`Filter->Filter`** — Source filter output feeds into target's filter input. The target's filter processes the source's already-filtered signal. Creates cascaded filtering where both filter characters compound. Best explored with OPTIC as source — OPTIC's analysis channels provide exceptionally rich coupling signal.

**`Pitch->Pitch`** — Source pitch modulates target pitch. When you play a note on the source engine, the target's pitch shifts by an interval. Use for live harmonization — Source playing C, target shifts up a fifth or a fourth depending on the coupling amount. Works best with melodic sources.

**`Rhythm->Blend`** — Source rhythm signal (amplitude fluctuation pattern, not strictly tempo-synced) modulates the target's blend parameter. In OPAL, this varies grain density. In OVERDUB, it modulates delay mix. OUROBOROS feeding this type into ONSET turns chaotic attractor amplitude into drum pattern variation — deterministic chaos as groove.

### Aggressive — Audio-Rate Modulation

These types pass actual audio-rate signals between engines. Switching between them mid-performance requires a brief crossfade (50ms for most, 100ms for `Audio->Buffer` and KnotTopology) to avoid clicks.

**`Audio->FM`** — Source audio signal becomes the FM modulator input for the target engine. The source's full spectral content — not just its amplitude — is injected into the target's frequency modulation path. At low amounts, this adds slight inharmonic color. At high amounts, the source rewrites the target's harmonic structure entirely on a sample-by-sample basis. Most dramatic of all coupling types. ORACLE feeding ORGANON with `Audio->FM` produces genuinely unpredictable metabolic synthesis.

**`Audio->Ring`** — Source audio is ring-modulated against the target's audio output. The product contains sum and difference frequencies of both signals — no fundamentals, only sidebands. Best with harmonic sources; chaotic with dense textures. Currently ORBITAL is the primary engine that handles `Audio->Ring` as a target; OPTIC accepts it as an analysis input.

**`Audio->Wavetable`** — Source audio replaces or modulates the target's wavetable source. In OPAL, the source audio becomes the grain buffer — OPAL will scatter fragments of the source rather than its original source material. In ORIGAMI, the source replaces the STFT analysis buffer. OVERWORLD feeding OPAL with `Audio->Wavetable` turns NES chip synthesis into a granular cloud.

**`Audio->Buffer`** — Source audio is written into the target's internal audio buffer. The deepest integration point in the coupling system: source audio becomes the target's synthesis material, not just a modulator. Requires the longest crossfade (100ms) when switching. Supported by ORGANON's per-voice ingestion buffer, which processes this material through its metabolic analysis system.

### Bidirectional — The Knot

**`KnotTopology`** — The only coupling type that flows in both directions simultaneously. Engine A modulates Engine B, and Engine B modulates Engine A, in the same processing pass. The **Depth** slider, when used with KnotTopology, controls the *linking number* — how many parameter pairs are entangled (1 at 0.0 through 5 at 1.0). A KnotTopology route cannot be meaningfully "broken in one direction" — removing it changes both engines' behavior fundamentally. Think of it as a bond, not a signal. OCEANIC and OCTOPUS entangled at linking number 4 breathe as one organism. OVERDUB and ORGANON at linking number 5 sustain each other indefinitely in mutual resonance — neither falls silent first.

---

## 4. The Performance System

### Macros

The four macro knobs — CHARACTER, MOVEMENT, COUPLING, SPACE — are visible at the bottom of the Performance View. The **COUPLING** macro (macro3) has a default target: the depth of active coupling routes. Turning it clockwise deepens the coupling across all active routes simultaneously.

This means your MPCe quad-corner pad — or any hardware controller mapped to the COUPLING macro — becomes a real-time coupling depth fader. Pull it left: the two engines sound like themselves. Push it right: they begin reshaping each other.

Each macro can target any parameter on any engine, including coupling route depths. Assigning the CHARACTER macro to control ONSET's character parameter while simultaneously assigning it to OVERBITE's bass character means one knob moves both engines in the same direction — or in opposite directions if you invert one of the targets.

### MIDI CC Mapping

Coupling route depths have default MIDI CC assignments out of the box:

| MIDI CC | Target |
|---------|--------|
| CC3 | Route 1 depth |
| CC9 | Route 2 depth |
| CC14 | Route 3 depth |
| CC15 | Route 4 depth |
| CC85 | Route 1 coupling type |

Any of these can be overridden using the MIDI Learn button in the Route Detail panel. Click MIDI Learn for any route, move a knob or fader on your controller, and that CC takes over. The coupling is now under your hands.

CC85 targets the coupling type for Route 1. Moving a knob through its range sweeps through coupling types — from AmpToFilter at the low end through KnotTopology at the high end. When switching between control-rate types, the change is instant. When switching into or out of audio-rate types (AudioToFM, AudioToRing, AudioToWavetable, AudioToBuffer, KnotTopology), a 50-100ms crossfade plays automatically to prevent clicks.

### The Crossfader as a Performance Tool

The coupling depth slider is not just a mixer. At moderate amounts it is a timbral control — the target engine sounds like a different instrument depending on where the slider sits. This makes it worth mapping to a physical fader or expression pedal rather than setting and forgetting.

**Live techniques:**

- **Sweep:** Assign Route 1 depth to a fader. Start at 0.0. Slowly sweep to 0.8 over 8 bars. The target engine gradually becomes possessed by the source.

- **Jump:** Assign a mod wheel to Route 1 depth. Press mod wheel at key moments — a dramatic coupling hit on the downbeat, then release.

- **Rhythmic crossfade:** Assign Route 1 depth to an LFO (via the MOVEMENT macro) and Route 2 depth to an inverted LFO. The two engines trade influence back and forth rhythmically.

- **Type sweep:** Map CC85 to a continuous knob. Sweep the coupling type mid-performance. The crossfade engine handles the transition. You will hear the character of the space between engines morph as you pass through each type.

---

## 5. Advanced Coupling Techniques

### Coupling as an Effects Processor

One underused pattern: run a simple, static engine in Slot 1 purely as a coupling *source*, with its audio contribution at zero in the mix. Its sole purpose is to generate coupling signals.

OPTIC (XOptic) is designed exactly for this. It is a zero-audio engine — it produces no sound of its own, only modulation signals derived from its input. Load OPTIC into Slot 1. Feed another engine's audio into OPTIC via `Audio->FM` as analysis input. OPTIC outputs up to 8 independent modulation channels: composite envelope, pulse, bass, mid, high, spectral centroid, flux, energy, transient. These drive other engines as coupling sources. OPTIC turns any signal into a precision modulation engine.

Similarly, OUROBOROS can run "dry" in the mix while its four output channels (stereo audio plus two attractor velocity derivatives) drive percussion and melodic engines via coupling. The attractor's chaos produces never-repeating modulation patterns that cannot be generated by any ordinary LFO.

### Using Coupling for Sound Design vs. Live Performance

In sound design contexts, coupling amount is a fixed parameter — you find the sweet spot where the two engines sound right together and leave the route at that depth. The Entangled mood presets in XOmnibus are all designed this way: coupling routes pre-set to their Dialogue intensity (0.45–0.55), chosen for a specific musical effect.

In performance contexts, coupling depth becomes a live gesture. Neither end of the range is "wrong." Zero coupling = two independent engines. Full coupling = one engine possessing another. Your performance is the motion between those states.

The four route slots let you build a coupling network with different types at different depths. You might hold Route 1 (Amp->Filter) at a fixed 0.35 for consistent sidechain character while performing Route 2 (Audio->FM) depth live on a fader — the stable breath underneath a volatile texture control.

### Preset Design with Coupling in Mind

When designing a preset intended to use coupling:

- Set up the source engine first, alone. It should sound musical or rhythmically useful by itself.
- Set up the target engine alone, with its parameters set to their "dry" values — the state it will be in when coupling depth is at zero.
- Add coupling routes and set them to Dialogue intensity (0.50). Evaluate whether the interaction is musically interesting.
- Adjust individual engine parameters (not coupling amounts) to optimize the coupled sound. If the coupled sound is too bright, lower the target engine's base filter cutoff, not the coupling depth.
- Use the macro system to expose the most performance-relevant coupling depth to hardware controllers.

A well-designed coupled preset sounds intentional at Whisper intensity, excellent at Dialogue, and extreme but usable at Possession.

### CPU Management with Coupled Engines

Coupling evaluation itself is lightweight — control-rate types (10 of 14) run at 1/32 the audio sample rate. Audio-rate types (AudioToFM, AudioToRing, AudioToWavetable, AudioToBuffer) process at full sample rate but still represent only a percentage of total engine CPU.

The crossfade engine activates only during type switches (50–100ms windows). During crossfade, coupling evaluation runs at 2x cost for that route — negligible in practice.

If CPU is a concern with complex multi-engine setups, prioritize control-rate types for your active performance routes and reserve audio-rate types for fixed, preset-designed routes that do not switch live.

---

## 6. Recipes

### Recipe A: The Breathing Bass — ONSET and OVERBITE

Two engines that define the XO_OX rhythmic-timbral axis.

**Engines:** ONSET (Slot 1, source), OVERBITE (Slot 2, target)

**Setup:**
1. ONSET: Program a kick on beats 1 and 3, snare on 2 and 4. Keep it clean — this pattern will be legible in the bass.
2. OVERBITE: Load a patch with a moderate filter cutoff (~1200 Hz), medium decay, Belly macro at 0.50.
3. Route 1: `Amp->Filter`, depth 0.45.
4. Route 2: `Env->Decay`, depth 0.20.

**What to listen for:** Each kick opens OVERBITE's filter — the bass gets briefly brighter and longer on every beat 1 and 3. The snare creates a shorter, sharper opening. Between hits, the bass settles into its darker, shorter self.

**Try this:** Slowly raise Route 1 depth from 0.45 to 0.75. At 0.75, the bass almost entirely loses its own filter identity — it speaks only when the drum speaks.

**The space between:** The space between ONSET and OVERBITE is a breathing instrument that neither engine could be alone. ONSET cannot sing. OVERBITE cannot keep time. Together, the drum teaches the bass how to move.

---

### Recipe B: The Granular Journey — ODYSSEY and OPAL

Two engines with complementary relationships to time and scatter.

**Engines:** ODYSSEY (Slot 1, source), OPAL (Slot 2, target)

**Setup:**
1. ODYSSEY: Load a slow-evolving pad preset — long attack, long release, the Familiar→Alien arc at moderate depth. The envelope should be slow enough to last 4–8 seconds.
2. OPAL: Load a granular cloud with grain size at 80ms, moderate scatter, grain scan at 0.5.
3. Route 1: `Env->Morph`, depth 0.50.
4. Route 2: `Amp->Filter`, depth 0.25.

**What to listen for:** As ODYSSEY's pad builds from attack through sustain, its envelope sweeps OPAL's grain scan position. The granular cloud changes character as the pad evolves — grains from different positions in OPAL's source buffer are scattered depending on where ODYSSEY's envelope sits. Hold a chord for 6 seconds and watch the cloud change.

**Try this:** Add Route 3: `LFO->Pitch`, depth 0.20. Now ODYSSEY's LFO adds slow pitch variation to OPAL's grains. The cloud is moving in three dimensions simultaneously — position (EnvToMorph), brightness (AmpToFilter), and pitch (LFOToPitch).

**The space between:** Two different kinds of journey. ODYSSEY moves through psychological distance over time. OPAL scatters each moment into fragments across time. The coupling maps emotional arc onto atomic scatter.

---

### Recipe C: The Metabolic Pair — ORACLE and ORGANON

Two engines that share the deepest intellectual coupling in XOmnibus.

**Engines:** ORACLE (Slot 1, source), ORGANON (Slot 2, target)

**Setup:**
1. ORACLE: A GENDY stochastic preset — the algorithm generates genuine randomness. A slow waveform evolution rate (0.5–1.0 Hz) gives ORGANON time to respond.
2. ORGANON: Low metabolic rate (1.0 Hz), moderate enzyme selectivity (400–700 Hz).
3. Route 1: `Audio->FM`, depth 0.50.

**What to listen for:** ORACLE's stochastic audio enters ORGANON's per-voice ingestion buffer. The organism metabolizes it. Over 2–4 seconds, ORGANON's modal array has been tuned by what it has ingested. Notes played after this point sound different from notes played in the first second — the organism has learned the prophet's voice.

**Try this:** Sustain a long note for 10 seconds. Listen to how ORGANON's character changes during the sustain. It starts as itself, then gradually absorbs the character of ORACLE's stochastic output.

**The space between:** Ancient probability mathematics feeding a living organism. ORACLE generates waveforms through the mathematics of ancient stochastic algorithms. ORGANON converts that randomness into biological adaptation. No two notes sound exactly the same because ORACLE's randomness is ORGANON's diet.

---

### Recipe D: The Deterministic Groove — OUROBOROS and ONSET

Chaos in service of rhythm.

**Engines:** OUROBOROS (Slot 1, source), ONSET (Slot 2, target)

**Setup:**
1. OUROBOROS: An attractor preset — Lorenz or Aizawa. Let it run. It needs no note triggers; the attractor evolves continuously.
2. ONSET: A standard four-on-the-floor pattern.
3. Route 1: `Amp->Filter`, depth 0.45.
4. Route 2: `Rhythm->Blend`, depth 0.35.

**What to listen for:** OUROBOROS's amplitude — which rises and falls as the attractor orbits — now modulates ONSET's filter and rhythm blend. The drum pattern is no longer a perfect grid. The kick's filter character changes subtly on every beat, driven by where the attractor is in its orbit at the moment of the hit. No two bars sound quite the same, but the attractor is mathematically deterministic — the same initial conditions always produce the same pattern. It *sounds* random but it is not.

**Try this:** Assign Route 1 depth to CC3 on your controller. Sweep it during the performance. At zero: perfect mechanical grid. At 0.75: the attractor has fully invaded the drum machine. The groove lives between those poles.

**The space between:** Deterministic chaos modulating human rhythm. The attractor never actually repeats on human timescales, so the percussion pattern never exactly repeats either. It is always the same song, always played differently.

---

### Recipe E: The Knot — OCEANIC and OCTOPUS

Two engines that breathe as one organism.

**Engines:** OCEANIC (Slot 1), OCTOPUS (Slot 2)

**Setup:**
1. OCEANIC: A bioluminescent pad — phosphorescent teal character, slow swell, medium separation.
2. OCTOPUS: An arm-depth texture — chromatophore spread at 0.5, moderate arm activity.
3. Route 1: `KnotTopology`, depth 0.75 (linking number 4 — deep entanglement).

**What to listen for:** Unlike directional coupling, KnotTopology runs both ways. OCEANIC's amplitude swell drives OCTOPUS's parameter. Simultaneously, OCTOPUS's arm activity feeds back into OCEANIC's separation. Neither engine has priority. They adapt to each other in the same processing pass.

The Knot is recognizable by its mutual breathing quality — when one engine rises, both tend to rise; when one settles, both tend to settle. At linking number 4, four parameter pairs are entangled. It can take a few bars for the system to find its mutual state.

**Try this:** Start with depth at 0.00 (linking number 1, minimal entanglement). Slowly raise depth while holding a sustained chord. Feel the coupling deepen as the linking number rises. At 1.00 (linking number 5), you have created a configuration that cannot be split — remove one engine and the remaining engine sounds wrong in isolation, tuned to a relationship that no longer exists.

**The space between:** With the Knot, there is no space between. The two engines are one thing. The KnotTopology coupling type exists precisely to say that.

---

## 7. The BAKE Button

When you have found a coupling configuration in the Performance View that you want to keep — specific routes, specific types, specific depths — the **BAKE** button saves your current performance overlay into the preset as a permanent coupling pair.

Without BAKE: the coupling overlay is ephemeral. Loading a new preset resets all performance routes to inactive.

With BAKE: your live tweaks become part of the preset. The routes, types, and depths you have crafted in the Performance View are written into the preset's coupling data, indistinguishable from factory-designed routes.

Use BAKE at the end of a sound design session to commit your exploration. Use it mid-performance if you find a configuration worth preserving. Leave it unbaked if you want the freedom to reset to the preset's original coupling on next load.

*Note: The BAKE button is coming in the next update. In the current version, performance overlay routes are ephemeral per session.*

---

## Closing Note

Coupling is not a feature. It is the argument that XOmnibus makes about synthesis.

Most instruments give you one voice. XOmnibus gives you a conversation. The 14 coupling types are 14 different ways that engines can share a current — from a gentle filter modulation that you might miss on first listen, to a full KnotTopology entanglement that makes two engines inseparable.

The space between two engines is where the most unexpected sounds live. It is also where the most useful ones live: the kick drum that teaches the bass how to breathe, the chaos attractor that turns a grid into a groove, the organism that adapts to what it is fed.

Start with `Amp->Filter` at 0.30 and two engines you already know. Find the space between them. Then raise the depth.

---

*XOmnibus — for all*
*XO_OX Designs*
