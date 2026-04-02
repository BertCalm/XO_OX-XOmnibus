# Init Patch Improvements — DB003 Resolution

**Date:** 2026-03-14
**Author:** XO_OX Designs
**Context:** DB003 Debate Resolution

---

## The Debate

**DB003** asks: "Init patch: immediate beauty vs. blank canvas?"

The seance split on doctrine:
- **Vangelis + Kakehashi position:** A synthesizer player must be rewarded immediately. The first sound you hear shapes everything that follows. If the init sounds like a test tone, you have told the player their time doesn't matter.
- **Schulze position:** A blank canvas is honest. The init patch is a diagnostic — does the engine work? The player should discover the beauty themselves.

**Resolution for XOceanus:** Performance instruments need immediate beauty; experimental instruments may benefit from neutral starts.

The practical test used: **Does the init patch make someone say "oh, that's interesting" within the first 3 seconds — on white keys, without prior knowledge of the engine?**

---

## Assessment: All 8 Engines Under Review

| Engine | Existing Init | 3-Second Test | Represents Engine? | White Keys Playable | Verdict |
|--------|--------------|--------------|-------------------|---------------------|---------|
| Overworld | None | FAIL — no preset | FAIL | N/A | NEEDS INIT |
| Morph (OddOscar) | Implicit defaults | PASS — bloom pad is recognizable | YES — morph at 0.5 is between sine+saw | YES | Acceptable |
| Oblique | "Oblique Clean Stab" near-init | PARTIAL — bounce isn't shown | PARTIAL — prism not alive | YES | Acceptable |
| Ocelot | None (Foundation) | FAIL — no init shows ecosystem | FAIL — macros were dead | YES | NEEDS INIT |
| Snap (OddfeliX) | "Snap Knock" near-init | PASS — percussive identity clear | YES — snap attack is present | NO (drum-pitched) | Acceptable |
| Orbital | No dedicated init | PARTIAL — default profile=Sawtooth | PARTIAL — group envelope not shown | YES | Acceptable |
| Obsidian | None | FAIL — default PD depth=0.5 with no envelope movement means the timbre doesn't breathe | FAIL — phase distortion story untold | YES | NEEDS INIT |
| Origami | None | FAIL — default fold is present but doesn't change over the note | FAIL — spectral fold story untold | YES | NEEDS INIT |

---

## The 4 Weakest Engines — Why They Failed

### 1. Overworld (OVERWORLD)

**Root cause:** Zero Foundation presets. The only existing preset ("Thunderforce") is maximum FM aggression at ERA 0.5/0.95 — it skips straight to Flux energy and never shows the chip-blending story.

The seance verdict: *"The ERA triangle arrives at the XOceanus dock with no luggage."* The B009 blessing is the ERA triangle (a 2D timbral crossfade that Buchla/Schulze/Vangelis/Pearlman all praised). No preset demonstrated it in a welcoming way.

**What makes the new init better:**
- ERA X and Y both at 0.5 — the barycentric center, where all three chip architectures contribute equally
- NES triangle wave enabled alongside pulse: two NES voices provide warmth and movement
- FM algorithm 1 with modest feedback (2): a gentle bell quality from Genesis, not aggression
- SNES moderate decay: the BRR string shimmer underneath
- ERA drift rate 0.12Hz with depth 0.08: the ERA position breathes slowly without being obvious
- Echo at 20% mix with a simple FIR rolloff: chip-appropriate delay without washing the character
- Moving the ERA macro immediately reveals the core instrument identity

### 2. Ocelot (OCELOT, score 6.4/10)

**Root cause:** The EcosystemMatrix (4 strata — Floor, Understory, Canopy, Emergent — with 12 cross-feed routes) is Ocelot's defining innovation. All existing presets (Submerged, Abyss, Tropicalia) are Atmosphere/Entangled mood presets tuned for specific scenes. None served as a neutral demonstration that lets the player understand what the engine does.

The seance verdict: *"The EcosystemMatrix is the most novel DSP concept in the fleet and it is completely inaccessible."* The D004 violation (macros all dead) has since been fixed, but the init was still missing.

**What makes the new init better:**
- Kalimba (floor model 3) at moderate tension and damping: immediately identifiable, musical on white keys
- Floor → Canopy cross-feed at 0.35: kalimba hits cause the canopy shimmer to open
- Floor → Emergent cross-feed at 0.5 with creature trigger=floor_amp: each kalimba hit summons a bird trill — visible ecosystem behavior from bar one
- Understory at 50% level but clean (bitDepth=16, tapeAge=0.05): texture is present but not the story yet
- PROWL macro (turn it up) increases floor drive and creature frequency — the ecosystem intensifies
- All 4 macros are now live (post D004 fix): PROWL, FOLIAGE, ECOSYSTEM, CANOPY all produce audible changes

### 3. Obsidian (OBSIDIAN, score 6.6/10)

**Root cause:** Phase distortion synthesis (CZ-series heritage) is Obsidian's entire identity — the timbre IS the waveform shape, controlled by densityX and tiltY in a 2D space. The parameter defaults (densityX=0.5, depth=0.5, all envelopes static) produce a recognizable but expressionless tone. The PD depth envelope (depthAttack/depthDecay/depthSustain) was never showcased in any existing preset — it makes the timbre travel through the PD space over the note's life, which is the core feature.

The seance verdict: *"The right channel has never heard one of the two filters"* (P0 bug, now fixed). Even with the P0 fix, the phase distortion story remained undemonstrated.

**What makes the new init better:**
- densityX=0.55, tiltY=0.45: slightly asymmetric placement creates harmonic interest from the CZ architecture (not the center, where harmonics partially cancel)
- depth=0.7: enough PD depth that the timbre is clearly non-sinusoidal
- stiffness=0.1: faint Euler-Bernoulli inharmonicity gives a slight bell quality
- cascadeBlend=0.25: stage 2 begins to add cross-modulated complexity
- PD depth envelope: attack=0.003s (instant), decay=0.35s, sustain=0.3, release=0.4 — the phase distortion opens fully at note-on then settles to a calmer timbre during sustain
- lfo1 at 0.25Hz depth=0.08: slow tremolo breath on the PD depth, meeting D005 doctrine
- The timbre journey from note-on to sustain demonstrates what "phase distortion synthesis" means, without requiring explanation

### 4. Origami (ORIGAMI, not formally scored)

**Root cause:** Spectral fold synthesis (FFT-based, inspired by Metasynth/Kyma) is Origami's entire identity. At default settings (foldPoint=0.5, foldDepth=0.5, flat fold envelope) the spectral fold is active but static — you can't tell anything is happening differently from a simple oscillator at first touch.

The seance question: *"The instantaneousFreq variable is a spectral compass that no preset has ever used."* Beyond that detail, no Foundation preset had been written to show what the fold does over time.

**What makes the new init better:**
- foldPoint=0.45: just below mid-spectrum, placing the fold crease in the rich 2–5kHz region where harmonics are most audible
- foldDepth=0.65: deep enough that the fold is clearly creating inharmonic metallic overtones, not subtle
- foldCount=1 (single fold): clean enough to understand what's happening — one fold, one crease
- Fold envelope: attack=0.005s (instant), decay=0.6s, sustain=0.2, release=0.4 — the fold opens fully at note-on then relaxes toward 0.2 during sustain. The note begins metallic and clarifies to a cleaner tone. You hear the fold working within the first second.
- oscMix=0.6: the raw oscillator is present alongside the folded spectrum, grounding the sound tonally
- lfo1 at 0.3Hz depth=0.12: slow spectral shimmer breathing the fold point, meeting D005 doctrine
- FOLD macro (push it up) deepens the fold and adds more folds — the instrument reveals its range immediately

---

## Engines Not Changed and Why

| Engine | Justification |
|--------|--------------|
| Morph (OddOscar) | Default params produce a valid slow-bloom sawtooth pad. The morph at 0.5 already hints at the wavetable scan. Existing Foundation presets cover the range. |
| Oblique | "Oblique Clean Stab" (Foundation) functions as a near-init. The engine's bounce/prism complexity has 3 Foundation presets demonstrating different characters. |
| Snap (OddfeliX) | "Snap Knock" and "Snap Tuned Bell" together cover the engine identity. The transient character is immediately clear. |
| Orbital | Default params (Sawtooth profile, group envelopes staggered, brightness=0.7) produce a usable harmonic tone. The Group Envelope system is the B001 blessing but orbital_macroEvolve=0.5 (default) already shows partial timing differences. |

---

## File Locations

| Engine | Init Patch Path |
|--------|----------------|
| Overworld | `Source/Engines/Overworld/Presets/Foundation/Init.xometa` |
| Ocelot | `Source/Engines/Ocelot/Presets/Foundation/Init.xometa` |
| Obsidian | `Source/Engines/Obsidian/Presets/Foundation/Init.xometa` |
| Origami | `Source/Engines/Origami/Presets/Foundation/Init.xometa` |

All four use `"starterFor"` field to designate them as the Module Starter preset for their engine.

---

## DB003 Status

**Partially resolved.** The fleet's consensus position:

1. **Performance/instrument engines** (Overworld, Ocelot, Obsidian, Origami): immediate beauty wins. These engines have complex architectures where the player cannot intuit the sound from parameter labels alone. An init patch that demonstrates the engine's signature capability in 3 seconds is essential onboarding.

2. **Experimental/modular engines** (Optic, Oracle, Organon): neutral starts may be appropriate. The Schulze position applies where the sound design process IS the instrument. A blank canvas here respects the player's creative intent.

3. **Percussion engines** (OddfeliX/Snap, Onset): identity is better shown through multiple Foundation presets (each demonstrating a mode) than a single init. "Snap Knock" + "Snap Tuned Bell" together tell the Snap story better than any single init could.

The debate remains open for the experimental tier. DB003 is marked **partially resolved**.
