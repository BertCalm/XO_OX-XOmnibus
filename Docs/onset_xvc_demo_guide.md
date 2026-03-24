# ONSET XVC Demo Guide

**Cross-Voice Coupling — The Rhythm Brain**

*For performers, sound designers, and developers exploring Blessing B002.*

---

## What Is XVC?

Cross-Voice Coupling (XVC) is ONSET's internal modulation system where one voice's amplitude directly modulates a parameter on another voice — in real-time, every audio block. The result is a kit that responds to itself: when the kick lands hard, the kit tightens, brightens, or bends around it. When the snare fires, other voices change character. No external routing table required.

This is fundamentally different from sidechaining in a DAW. XVC happens inside the synthesis engine, block by block, before any FX. The coupling creates emergent timbral relationships that feel organic because they are — they arise from the physics of the voices interacting rather than from explicitly programmed automation.

The seance labeled XVC "3-5 years ahead of industry" (Blessing B002). Vision V001 identifies it as the future of drum synthesis. This document proves why.

---

## The XVC Architecture

ONSET has 8 dedicated voices, permanently mapped:

| Voice | Role | Circuit | Default Algorithm |
|-------|------|---------|------------------|
| V1 | Kick | BridgedT (808 topology) | Modal |
| V2 | Snare | NoiseBurst (808/909) | FM |
| V3 | HH-C | Metallic (808 hat network) | FM |
| V4 | HH-O | Metallic | FM |
| V5 | Clap | NoiseBurst (multi-burst) | Phase Distortion |
| V6 | Tom | BridgedT | Modal |
| V7 | Perc A | BridgedT / Karplus-Strong | K-S |
| V8 | Perc B | Metallic | Modal |

Each voice generates a peak amplitude value during each audio block. XVC reads the previous block's peak values as modulation sources and applies them to target parameters before synthesis begins. This is a one-block latency feedback loop — real-time, not delayed.

---

## The 6 XVC Parameters

| Parameter ID | What It Does | Range | Default |
|-------------|-------------|-------|---------|
| `perc_xvc_global_amount` | Master XVC intensity — scales all coupling signals | 0–1 | 0.5 |
| `perc_xvc_kick_to_snare_filter` | Kick peak → adds to snare tone (filter brightness) | 0–1 | 0.15 |
| `perc_xvc_snare_to_hat_decay` | Snare peak → subtracts from closed-hat decay (tightens) | 0–1 | 0.10 |
| `perc_xvc_kick_to_tom_pitch` | Kick peak → subtracts from tom pitch (up to 6 semitones down) | 0–1 | 0.0 |
| `perc_xvc_snare_to_perc_blend` | Snare peak → adds to Perc A blend (pushes toward algorithm) | 0–1 | 0.0 |
| `perc_xvc_hat_choke` | Enable/disable closed-hat choking open-hat (boolean via threshold ≥0.5) | 0–1 | 1.0 |

### How Routing Works (Under the Hood)

Every process block, ONSET:
1. Reads voice peaks from the previous block: `voicePeaks[0..7]`
2. Scales by `xvcGlobalAmount`: `kickPeakScaled = voicePeaks[0] * xvcGlobalAmount`
3. Applies each route additively to the parameter array before voice synthesis:
   - `vTone[1] += kickPeakScaled * kickToSnareFilterAmount`
   - `vDecay[2] -= snarePeakScaled * snareToHatDecayAmount * 0.5`
   - `vPitch[5] -= kickPeakScaled * kickToTomPitchAmount * 6.0`
   - `vBlend[6] += snarePeakScaled * snareToPercBlendAmount * 0.3`
4. Hat choke: when V3 (HH-C) triggers, V4 (HH-O) receives a choke signal (immediate decay-to-silence)

The key insight: these are **additive** modulations on top of preset values. A preset with `vTone[1] = 0.4` and `kickToSnareFilter = 0.8` will have the snare's tone pushed up toward 1.0 on every strong kick hit — then decay back to 0.4 between hits.

---

## Reading the XVC Matrix in a Preset

Open any `.xometa` file and look for the XVC block:

```json
"perc_xvc_kick_to_snare_filter": 0.75,
"perc_xvc_snare_to_hat_decay": 0.05,
"perc_xvc_kick_to_tom_pitch": 0.0,
"perc_xvc_snare_to_perc_blend": 0.0,
"perc_xvc_hat_choke": 1.0,
"perc_xvc_global_amount": 0.9
```

Read it as: "**Kick→Snare Filter:** high (0.75 × global 0.9 = effective 0.675) — strong kick hits will dramatically brighten the snare. **Snare→Hat Decay:** minimal (0.05) — snare barely affects hat timing. **Hat Choke:** on. **Kick→Tom Pitch:** silent. **Snare→Perc Blend:** silent."

The `global_amount` acts as a master multiplier. At 0.0, XVC is bypassed entirely. At 1.0, each individual route operates at full scale. This gives you a single macro-style control over all coupling depth simultaneously.

---

## The 12 XVC Demo Presets

All presets live in `Presets/XOlokun/Flux/Onset/XVC/` except Demo 12 which is in `Presets/XOlokun/Entangled/`.

### Demo 1 — Kick Pumps Hats
**File:** `Kick_Pumps_Hats.xometa`
**Core route:** `kick_to_snare_filter: 0.75`, `global_amount: 0.9`
**What to listen for:** Play a pattern where the kick hits on beats 1 and 3. Notice how the hats and snare tone brightens on kick beats — not because of volume, but because the kick's amplitude physically opens the snare filter. The opposite of ducking: the kick energizes the rest of the kit.
**Experiment:** Set `global_amount` to 0.0 and hear the flat, uncoupled version. Then raise it back to 0.9 and feel the kit come alive.

### Demo 2 — Snare Blooms Space
**File:** `Snare_Blooms_Space.xometa`
**Core route:** `snare_to_perc_blend: 0.95`, `global_amount: 0.85`
**What to listen for:** The Perc A voice (V7) starts in circuit mode (low blend). Every snare hit pushes it toward the algorithmic FM layer (up to +0.285 blend shift). On strong snare hits, Perc A briefly becomes an FM voice then returns to its analog character. A snare-triggered timbre bloom on the textural layer.
**Experiment:** Set Perc A blend to 0.0 (pure circuit) and hear it pop to FM on every snare. Or set it to 0.9 (mostly algorithm) and hear the snare push it to full FM mode.

### Demo 3 — Hat Chokes Clap
**File:** `Hat_Chokes_Clap.xometa`
**Core routes:** `hat_choke: 1.0`, `snare_to_hat_decay: 0.9`, `global_amount: 0.85`
**What to listen for:** This is the most naturalistic preset. Program a pattern with both closed and open hats. The closed hat immediately cuts the open hat (hardware choke behavior). Additionally, every snare hit tightens the closed hat's decay to near-minimum — after a snare, the hat is physically briefer. This is how a real drummer plays: snare accent = hat closes tighter.
**The key insight:** Disable `hat_choke` (set to 0.0) and you lose the natural cymbal physics. Enable it and the kit breathes like acoustic hardware.

### Demo 4 — Kick Pitches Toms
**File:** `Kick_Pitches_Toms.xometa`
**Core route:** `kick_to_tom_pitch: 1.0`, `global_amount: 0.95`
**What to listen for:** The Tom (V6) has a positive pitch offset (+3 semitones from base). When the kick fires, it subtracts up to 6 semitones from the tom pitch. If V6 pitch = +3 and kick peak = 1.0, the resulting tom pitch = +3 - (1.0 × 0.95 × 6.0) = -2.7 semitones. Full kick hit pulls tom pitch down by nearly 6 semitones. Softer kick = less pitch drop.
**Experiment:** Dial in tom rolls alongside kick. The toms pitch-slide with every kick hit — natural, dynamic, velocity-sensitive pitch coupling without programming a single automation curve.

### Demo 5 — Full Mesh XVC
**File:** `Full_Mesh_XVC.xometa`
**Core routes:** All 4 routes at 0.85, `hat_choke: 1.0`, `global_amount: 1.0`
**What to listen for:** Every route active simultaneously. Kick brightens snare (0.85 amount), snare tightens hats (0.85), kick bends toms (0.85), snare pulls perc toward FM (0.85), hat chokes. Each hit creates a ripple through the entire kit. This is the "rhythm brain" firing on all neurons — the full capability of XVC expressed at once.
**Warning:** This can feel overwhelming at first. Start with a simple 4-on-the-floor pattern. Let the kit's self-modulation settle into your ears before adding complexity.

### Demo 6 — Inverse Field
**File:** `Inverse_Field.xometa`
**Core routes:** All individual routes near 0.02, `hat_choke: 0.0`, `global_amount: 1.0`
**What to listen for:** The `global_amount` is maxed but all individual routes are essentially zero — and hat choke is disabled. This is not XVC "off." It is XVC at maximum global intensity routing through near-zero connections. The result: a slightly open, airy kit where open hats sustain freely (no choke), hats are not tightened by snare, toms are not pitched by kick. The kit breathes in the opposite direction from Demo 5.
**The concept:** Sometimes the best use of XVC is choosing *not* to couple. Inverse Field demonstrates that XVC global at 1.0 is not inherently aggressive — it is a potential that you populate with individual routes.

### Demo 7 — Mutate Cascade
**File:** `Mutate_Cascade.xometa`
**Core:** MUTATE macro at 0.75 + all XVC routes at 0.7–0.8
**What to listen for:** MUTATE adds per-block random drift to blend and character on every voice. XVC routes take those constantly-shifting voices and couples their fluctuations to other voices. The kick's character drift propagates into the snare filter, which propagates into hat timing, which propagates into Perc A's blend. Not pure random — a cascading stochastic organism where the random drift is multiplied and correlated through the coupling network.
**The key insight:** MUTATE alone sounds random. MUTATE + XVC sounds alive. The coupling creates dependencies between the random signals, making them move together in complex but related ways.

### Demo 8 — Kick Dominates
**File:** `Kick_Dominates.xometa`
**Core routes:** `kick_to_snare_filter: 1.0`, `kick_to_tom_pitch: 1.0`, `global_amount: 1.0`
**What to listen for:** Massive kick (level 1.0, body 0.95, deep pitch -3). All kick-sourced XVC routes maxed. The kick exerts maximum gravitational force on the kit — brightest snare filter on every kick, maximum tom pitch drop. The snare routes are minimal so only kick sculpts the ensemble.
**Use case:** Hip-hop, trap, any context where the kick is the rhythmic and timbral anchor. The kit literally sounds heavier when the kick hits.

### Demo 9 — Ghost Generator
**File:** `Ghost_Generator.xometa`
**Core route:** `snare_to_perc_blend: 1.0`, `global_amount: 0.95`; V7 level: 0.22, V8 level: 0.20
**What to listen for:** V7 (Perc A) and V8 (Perc B) have very low output levels — they would rarely be heard in isolation. V7 is set to nearly pure circuit (blend 0.08) and uses FM algorithm mode 0. When snare hits, `snare_to_perc_blend` pushes V7 to +0.30 blend (toward algorithm). At higher FM depths, V7's self-resonance can generate subtle ghost hits not programmed in the sequence.
**The magic:** Program only kick, snare, and hats in your sequencer. Let XVC populate the ghost notes on the percussion voices. Every pattern generates unique ghost distribution depending on snare velocity.

### Demo 10 — Polyrhythm Emergence
**File:** `Polyrhythm_Emergence.xometa`
**Core:** MUTATE at 0.45, `snare_to_hat_decay: 0.75`, `kick_to_tom_pitch: 0.7`, `global_amount: 0.88`
**What to listen for:** The combination of hat-decay tightening (after snare) and tom pitch-bending (after kick) creates a perceptual effect of complex rhythm not programmed into the sequence. The hat's apparent timing varies with snare intensity; the tom feels rhythmically offset from the kick because its pitch changes make identical hits sound different. Your brain hears compound rhythm where only a simple pattern exists.
**The concept:** This is XVC's highest-level emergent behavior — apparent polyrhythm from monorhythm. Not a trick, not fake: your auditory system is genuinely parsing the pitch and timbre variations as rhythmic information.

### Demo 11 — Minimal XVC
**File:** `Minimal_XVC.xometa`
**Core:** Only `hat_choke: 1.0`, all other routes at 0.0, `global_amount: 0.2`
**Purpose:** The tutorial entry point. Learn XVC one behavior at a time. Load this preset and toggle `hat_choke` between 1.0 and 0.0. Hear the difference between open hats with and without the choke. This is XVC's simplest expression — physical cymbal behavior. Once you hear it, gradually raise the other XVC routes one by one.
**Recommended progression:** Minimal XVC → Demo 1 (add kick-snare) → Demo 3 (add hat-decay) → Demo 4 (add tom pitch) → Demo 5 (full mesh).

### Demo 12 — XVC Dub Mesh (Entangled)
**File:** `Presets/XOlokun/Entangled/Onset_XVC_Dub_Mesh.xometa`
**Requires:** ONSET + OVERDUB
**Core:** Internal XVC mesh (kick→snare filter 0.8, kick→tom pitch 0.75) + external coupling Onset→Overdub AmpToFilter (0.4)
**What to listen for:** Two layers of coupling simultaneously. Inside ONSET, kick shapes the kit. Externally, the ONSET master output modulates OVERDUB's filter — so the kick not only brightens the snare internally but also pumps the bass synth's filter cutoff. One kick hit creates: (1) internal snare brightening via XVC, (2) internal tom pitch bend via XVC, (3) external OVERDUB filter pump via MegaCoupling. The dub bass breathes with the drums.
**The concept:** XVC (internal) and MegaCouplingMatrix (external) are complementary layers. XVC operates within the drum kit; MegaCoupling operates across engines. This preset demonstrates both simultaneously.

---

## Advanced Patterns (Round 12G)

Six presets exploring coupling patterns not covered in the original 12 demos. All live in `Presets/XOlokun/Flux/Onset/XVC/`.

### Advanced 1 — Tom's Heartbeat
**File:** `Toms_Heartbeat.xometa`
**Core route:** `kick_to_tom_pitch: 0.9`, `global_amount: 0.88`, MUTATE 0.3
**What to listen for:** Tom (V6) sits a perfect fifth above the kick. Every kick hit subtracts up to ~4.9 semitones from the tom pitch (0.9 × 0.88 × 6.0). At full velocity the tom drops nearly five semitones in sympathy — a physical resonance between two drums sharing conceptual mass. MUTATE at 0.3 adds micro-drift to the coupling magnitude, so no two kick-tom interactions are identical. Snare and hats are recessed; the kick-tom dialogue dominates.
**Experiment:** Raise tom pitch offset (V6) to +6 semitones and let the kick pull it to unison on hard hits — the tom effectively "tunes to" the kick under pressure.

### Advanced 2 — Snare Cascade
**File:** `Snare_Cascade.xometa`
**Core routes:** `snare_to_hat_decay: 0.95`, `snare_to_perc_blend: 0.9`, `hat_choke: 1.0`, `global_amount: 0.85`
**What to listen for:** Maximum snare-to-hat decay means a hard snare hit collapses the closed hat to near-zero decay (effectively muting it). Simultaneously, snare-to-perc pushes V7 (Perc A) from circuit mode (blend 0.08) toward FM algorithm. Both effects fire at the same moment — the entire upper register reshapes on every backbeat. The kit's ceiling collapses on 2 and 4.
**The key insight:** Both routes are snare-sourced, so the snare controls the full upper layer. Kick has almost no XVC presence in this preset — unusually, the snare is the dominant modulator.

### Advanced 3 — Machine Ghost
**File:** `Machine_Ghost.xometa`
**Core routes:** All 4 routes at 0.75–0.95, `hat_choke: 1.0`, `global_amount: 0.92`; all voice levels: 0.15–0.32
**What to listen for:** Direct attacks are barely audible at these low output levels. What you hear is almost entirely XVC: snare_to_perc_blend pushes V7 (Perc A, blend 0.05, FM algo mode 0) hard into FM self-resonance on every snare, generating ghost events not programmed in the sequence. The kick, hats, clap — all present but recessed. Notes appear that exist only because other notes existed. Program kick, snare, and hats; hear kick, snare, hats, and ghosts.
**The concept:** This is Machine Ghost Generator behavior (Demo 9) taken further — all voice levels suppressed simultaneously, leaving only the emergent coupling signal. Raise `perc_level` (master) to bring the direct attacks back while keeping ghosts.

### Advanced 4 — Compression Ring
**File:** `Compression_Ring.xometa`
**Core routes:** `kick_to_snare_filter: 1.0`, `hat_choke: 1.0`, `global_amount: 0.95`; snare tone starts at 0.18
**What to listen for:** Snare tone is preset to 0.18 — dark and compressed-sounding. Kick fires and pushes snare tone toward 0.18 + (1.0 × 0.95 × 1.0) = effectively capped at 1.0 on full kicks. The snare brightens on every kick, returning to dark between kicks. This mimics sidechain compression expressed as timbral change rather than volume change. The "pump" is spectral, not dynamic.
**Use case:** House, techno, and any style where the kick-snare relationship should feel physically connected. The snare literally shares tonal identity with the kick's energy. Hat choke provides hardware cymbal realism on top.

### Advanced 5 — Euclidean Web
**File:** `Euclidean_Web.xometa`
**Core routes:** `snare_to_hat_decay: 0.85`, `hat_choke: 1.0`, `global_amount: 0.82`, MUTATE 0.6
**What to listen for:** This preset isolates the snare→hat-decay path and amplifies it through MUTATE. With MUTATE at 0.6, character and blend on every voice drift stochastically each block. The hat's output amplitude fluctuates with MUTATE, which changes the snare→hat coupling magnitude in turn (because snare peaks are also MUTATE-affected). The result is a MUTATE-mediated indirect feedback: hats influence their own decay variation through the coupling chain. Program an irregular Euclidean hat pattern and the snare timing appears to flex in response.
**The concept:** Reverse causality via MUTATE. The usual flow is snare→hat. Here, MUTATE-driven hat fluctuation creates the appearance of hat→snare influence through the intermediary of MUTATE-scaled coupling signals. Not true bidirectional XVC — but perceptually, the hats drive the pattern.

### Advanced 6 — Velocity Feedback
**File:** `Velocity_Feedback.xometa`
**Core routes:** All 4 routes at 0.45–0.65, `hat_choke: 1.0`, `global_amount: 0.78`, MUTATE 0.2
**What to listen for:** Because XVC signal strength equals voice output amplitude (which scales with velocity), all coupling amounts are effectively velocity-scaled. At pp (soft): coupling near zero — clean, mechanical, isolated voices. At ff (hard): 0.65 × 0.78 × full amplitude = substantial coupling — kick brightens snare, snare tightens hats, kick bends toms, snare pulls perc toward FM. The same pattern becomes organic under pressure. Velocity IS the coupling macro.
**Experiment:** Lock your sequencer to full velocity and notice the heavy coupling. Drop all velocities to minimum and hear the same pattern uncoupled. This is the most direct demonstration of the XVC/velocity relationship described in the Performance Tips section.

---

## Performance Tips

**Using MUTATE with XVC:** Start MUTATE at 0.3–0.5 when XVC is active. Full MUTATE (1.0) with full mesh XVC (all routes at 1.0) creates extremely chaotic behavior that can be hard to control in a performance. Use MUTATE as gradual texture introduction: automate it from 0 to 0.5 over 8 bars for a natural kit evolution.

**XVC Global as a Macro:** Automate `perc_xvc_global_amount` as a performance macro. At 0.0, the kit is flat and mechanical (useful for drops). At 1.0, the kit is maximally coupled and organic. Sweeping this from 0 to 1 over a bar creates a dramatic "kit wakes up" effect.

**Velocity and XVC:** XVC peak values are proportional to voice output amplitude. Velocity-soft hits create smaller XVC signals — so hitting softer naturally reduces coupling. This means humanized patterns with velocity variation will have subtle, velocity-proportional coupling. Full-velocity locked patterns will have maximum, consistent coupling. Neither is wrong — they create different feels.

**Layering with Hat Choke:** Hat choke (`perc_xvc_hat_choke`) affects only the physical choke relationship between V3 (HH-C) and V4 (HH-O). It operates independently from the global amount — even at `global_amount = 0.0`, hat choke still works. Hat choke is the one XVC behavior that is always available regardless of global level.

---

## XVC and the Blessing B002 Claim

The seance identified XVC as "3-5 years ahead of industry" because no commercial drum machine implements internal voice-to-voice modulation as a first-class synthesis paradigm. In existing hardware and software:

- **Roland TR-8S:** No internal voice coupling. Fixed topologies.
- **Arturia DrumBrute Impact:** No coupling. Parameter-independent voices.
- **Elektron Rytm:** Parameter locks (per-step value changes) but no continuous amplitude-to-parameter coupling.
- **NI Battery 4:** Sample playback, no synthesis coupling.

The closest analog is the Buchla 200e's modular architecture — but that requires manual patching. XVC bakes 4 meaningful routing types directly into the drum engine's synthesis loop, pre-configured for musical usefulness (kick→snare, snare→hat, kick→tom, snare→perc).

The innovation is the musical intent behind each route: they are not arbitrary — they model real acoustic drum physics (hat choke, kick body affecting snare resonance) and extend them into synthesized territory (pitch coupling, timbre coupling across algorithm layers).

**What remains after the demo:** The current XVC implementation exposes 5 routes (4 modulatable + hat choke). The architecture supports 8×8 = 64 possible routes. Future expansion could expose the full matrix through a visual XVC grid UI — each intersection a knob. Vision V001's roadmap for XVC includes this matrix visualization, bipolar routing (negative coupling amounts), and velocity-threshold gating (only couple when velocity exceeds a threshold).

---

## Preset Schema Quick Reference

Every XVC parameter maps to a `perc_` prefixed key in the `"Onset"` section of a `.xometa` file:

```json
{
  "parameters": {
    "Onset": {
      ...voice parameters (perc_v1_ through perc_v8_)...
      "perc_xvc_kick_to_snare_filter": 0.0–1.0,
      "perc_xvc_snare_to_hat_decay":   0.0–1.0,
      "perc_xvc_kick_to_tom_pitch":    0.0–1.0,
      "perc_xvc_snare_to_perc_blend":  0.0–1.0,
      "perc_xvc_hat_choke":            0.0 or 1.0,
      "perc_xvc_global_amount":        0.0–1.0
    }
  }
}
```

When writing new presets with XVC intent, set `global_amount` first (recommend 0.7–0.95 for perceptible coupling), then dial in individual routes. A route at 0.0 contributes nothing even at global 1.0.

---

*Document version 1.1 — 2026-03-14 (Round 12G: +6 Advanced Pattern presets)*
*Presets in: `Presets/XOlokun/Flux/Onset/XVC/` (17 presets) + `Presets/XOlokun/Entangled/Onset_XVC_Dub_Mesh.xometa` (1 preset)*
