# Engine Sound Design Philosophy — XPN Pack Design Rules
**R&D Document | XO_OX Designs | 2026-03-16**

Per-engine sonic identity distilled into actionable rules for sound designers building XPN packs. Each entry covers what makes the engine worth sampling, where it sits on the feliX-Oscar axis, three signature moves, the ideal pack format, and one coupling that unlocks a new dimension.

---

## 1. OBESE (`fat_`) — Hot Pink `#FF1493`

**Sonic identity:** OBESE is saturation as an instrument. The MOJO control is an orthogonal analog/digital axis — pushing it analog adds harmonic grit and sag, pushing it digital sharpens transients into something almost brittle. No other engine in the fleet treats distortion character as a first-class parameter.

**feliX-Oscar:** +1.5 (warm and dominant, pushes into the mix rather than sitting back)

**Signature moves:**
- Set `fat_satDrive` high and `fat_mojoControl` fully analog — result is mid-century keyboard character with crosstalk warmth
- Automate `fat_mojoControl` via LFO at 0.03 Hz for slow analog-to-digital drift: the sound "ages" across a phrase
- Velocity-scale `fat_satDrive` so pianissimo notes are clean and fortissimo notes crunch — the instrument earns its name dynamically

**Best XPN pack type:** Keygroup — OBESE rewards chromatic playability across a keyboard; the saturation character holds up note-to-note in a way that makes sustained chords interesting at every velocity layer

**Coupling recommendation:** Route OBESE → OVERDUB (COUPLING: AMP). OVERDUB's tape delay trails become saturated artifacts — ghost notes that sound like tape warble burned into the repeats.

---

## 2. ORACLE (`oracle_`) — Prophecy Indigo `#4B0082`

**Sonic identity:** ORACLE uses GENDY stochastic synthesis: breakpoint functions evolve probabilistically between note triggers, meaning no two sustained notes are identical. The Maqam modal system adds microtonal inflection from non-Western pitch spaces that sit outside conventional tuning.

**feliX-Oscar:** -1.5 (probabilistic and alien; the least predictable voice in the fleet — firmly Oscar territory)

**Signature moves:**
- Set `oracle_breakpoints` to maximum count with slow stochastic rate — the waveform breathes over seconds, not cycles
- Use Maqam tuning on a minor-scale keygroup: sampled single notes across an octave will all have different timbral fingerprints even from the same root note trigger
- Keep filter open and let the stochastic modulation serve as the filter — GENDY shapes its own spectral envelope

**Best XPN pack type:** Sample library — capture long single-note renders across 3 octaves at multiple stochastic seed states; the variation is too valuable to lose to a keygroup's stretching artifacts

**Coupling recommendation:** ORACLE → OBLIQUE (COUPLING: PITCH). OBLIQUE's prismatic bounce maps the stochastic pitch micro-variations into prismatic harmonics — a random oracle becomes a shimmering harp of probabilistic overtones.

---

## 3. OUROBOROS (`ouro_`) — Strange Attractor Red `#FF2D2D`

**Sonic identity:** OUROBOROS is a chaotic feedback system held on a leash. The Leash Mechanism (Blessing B003) means the chaos is always musical — it never collapses into noise or silence, it orbits a strange attractor. Velocity Coupling Outputs (B007) make it a modulation source as well as a voice.

**feliX-Oscar:** -2 (the most Oscar engine in the fleet — unpredictable, self-referential, anti-melodic in its natural state)

**Signature moves:**
- Set `ouro_topology` to maximum and `ouro_leash` to 30% — the system loops chaotically but returns to a tonal center every few cycles; record 8-bar loops and extract the moments of order
- Low `ouro_leash` + high resonance filter: the chaos gets tamed into droning filtered feedback that sounds like a room mode
- Use Velocity Coupling Output routed to a second engine's filter cutoff — OUROBOROS becomes a velocity-reactive chaos modulator

**Best XPN pack type:** Sample library — OUROBOROS is too temporally unpredictable for live keygroup use; capture 30-60 second renders, then slice for stems, risers, and textural loops

**Coupling recommendation:** OUROBOROS → OPAL (COUPLING: MOD). The chaos feeds OPAL's grain scatter parameter — granular clouds that breathe and collapse on attractor cycles rather than regular LFO movement.

---

## 4. ORGANON (`organon_`) — Bioluminescent Cyan `#00CED1`

**Sonic identity:** ORGANON models variational free energy metabolism — the engine's synthesis voice evolves by minimizing prediction error over time, meaning it "learns" the musical context and shifts its timbre accordingly. Seance praised it as publishable DSP research.

**feliX-Oscar:** -0.5 (adaptive and intelligent; slightly Oscar in its self-determination but responsive enough to feel musical)

**Signature moves:**
- Set `organon_metabolicRate` slow (0.02 Hz equivalent) and play a repeating pattern — the engine will subtly shift toward harmonic stability over 8-16 bars, creating natural preset evolution without automation
- High `organon_metabolicRate` in response to velocity: loud playing triggers rapid metabolic response; quiet playing lets it settle — a dynamics-aware timbre engine
- Use alongside sustained pads: ORGANON's adaptation sharpens its spectral center toward the frequency gaps in the mix — biologically fills negative space

**Best XPN pack type:** Hybrid — keygroup for melodic lines combined with a stem sample library capturing metabolic evolution states (early, mid, and settled timbre captures of the same preset)

**Coupling recommendation:** ORGANON → ORACLE (COUPLING: MOD). ORACLE's stochastic breakpoints get metabolic guidance — the chaos organizes over time without becoming predictable.

---

## 5. OPAL (`opal_`) — Lavender `#A78BFA`

**Sonic identity:** OPAL is granular synthesis at its most lyrical. 150 factory presets prove the engine's range — from tight shimmer to cloud-scale textures. `opal_grainSize` and grain scatter are the primary timbral axes; the interaction between them defines whether OPAL sounds like a sampler, a reverb, or an organism.

**feliX-Oscar:** +0.5 (granular is inherently textural/Oscar, but OPAL's pitch tracking and lush preset palette keep it melodically accessible)

**Signature moves:**
- `opal_grainSize` at 20ms, tight scatter, high density — OPAL becomes a lush unison chorus; useful as a doubling layer on keygroups
- `opal_grainSize` at 400ms, wide scatter — transforms any source into clouds; sample a cello note through OPAL at these settings for instant ambient texture library material
- Velocity-map grain density: low velocity = sparse isolated grains; high velocity = dense cloud — gives the engine expressive dynamic range

**Best XPN pack type:** Keygroup — OPAL tracks pitch with enough fidelity for melodic use; a 3-velocity-layer keygroup with different grain density per layer is a complete ambient instrument in one pack

**Coupling recommendation:** OPAL → OBSIDIAN (COUPLING: AMP). Granular clouds feeding OBSIDIAN's crystalline resonators creates frozen reverb tails with defined spectral peaks — controlled infinite shimmer.

---

## 6. OVERWORLD (`ow_`) — Neon Green `#39FF14`

**Sonic identity:** OVERWORLD is a three-way ERA crossfade between NES 2A03, Genesis YM2612, and SNES SPC700 chip synthesis. The ERA Triangle (Blessing B009) is a 2D timbral space — any point within the triangle blends the three chip architectures in proportion, creating a continuous spectrum of chip character from every era of console history.

**feliX-Oscar:** +1 (chip synthesis is inherently punchy and assertive — strongly feliX in its refusal to sit in background)

**Signature moves:**
- Park ERA Triangle at the NES vertex with `ow_era` at max NES: pure 4-bit PWM duty-cycle character; layer two keygroup octaves for instant lo-fi chord stabs
- Blend to YM2612 center: the FM warmth of Genesis brass hits — velocity-scale the FM ratio for velocity-sensitive harmonic brightness (D001 compliance)
- Full SNES SPC700: the soft sampled-synthesis warmth works beautifully for bell-like leads; record single notes across 4 octaves for a complete keygroup that sounds like a game OST instrument

**Best XPN pack type:** Keygroup — chip synthesis sits in the mix with immediate identity and tracks pitch cleanly; multi-ERA velocity layers in a single pack give producers three decades of console history in one instrument

**Coupling recommendation:** OVERWORLD → ORBITAL (COUPLING: PITCH). ORBITAL's group envelope system shapes OVERWORLD's chip tones with Moog-grade articulation curves — chip synthesis with analog feel.

---

## 7. OCEANIC (`ocean_`) — Phosphorescent Teal `#00B4A0`

**Sonic identity:** OCEANIC's Chromatophore Modulator (Blessing B013) maps color-change dynamics — rapid local signal fluctuations producing bioluminescent spectral shimmer. The `ocean_separation` parameter controls the spatial distribution of chromatophore zones, ranging from tight mono shimmer to wide stereo living-color spreads.

**feliX-Oscar:** 0 (OCEANIC sits exactly at center: the chromatophore modulation is reactive like feliX, but the underlying sound is ambient and deep like Oscar)

**Signature moves:**
- High `ocean_separation` with slow chromatophore rate: stereo field pulses with bioluminescence; perfect for wide ambient pad stems
- Fast chromatophore rate + tight separation: the spectral shimmer reads as a tremolo-flanged texture — useful on melodic elements as a modulation source
- Velocity scales chromatophore intensity: light touches produce barely-perceptible shimmer; full velocity triggers color explosions — expressive across the dynamic range

**Best XPN pack type:** Sample library — capture stereo renders at different separation and chromatophore rate settings; loops and one-shots of color-change events make excellent FX library material

**Coupling recommendation:** OCEANIC → OPAL (COUPLING: MOD). Chromatophore activity modulates grain scatter — bioluminescent pulses create synchronized bursts of granular density.

---

## 8. OBSIDIAN (`obsidian_`) — Crystal White `#E8E0D8`

**Sonic identity:** OBSIDIAN is crystalline resonant synthesis — physical modeling of glass and stone structures. The `obsidian_depth` parameter controls resonant density, from a single clear tone to a dense crystalline chord stack. It is one of the quietest engines in the fleet; its dynamic range lives in harmonic color, not amplitude.

**feliX-Oscar:** -1 (crystalline and self-contained; Oscar in its inward quality, but not chaotic — stable and cold)

**Signature moves:**
- Low `obsidian_depth`, high resonance: single-note glass harmonic; record one note per semitone across two octaves for a complete glass instrument keygroup
- High `obsidian_depth` + slow attack: the crystal stack builds like a held organ chord; useful as a harmonic pad layer sampled in root-note long-form stems
- Velocity maps to crystal fracture — harder hits add upper partial complexity; the instrument breaks as you play harder

**Best XPN pack type:** Keygroup — OBSIDIAN's pitch stability and sustain profile make it ideal for chromatic sampling; 2 velocity layers (soft crystal vs. fractured crystal) creates a complete instrument

**Coupling recommendation:** OBSIDIAN ← OUROBOROS (COUPLING: PITCH). OUROBOROS chaos drives crystal fracture — the cold stable resonator becomes a living, breathing crystal damaged by attractor orbits.

---

## 9. OHM (`ohm_`) — Sage `#87AE73`

**Sonic identity:** OHM is the Hippie Dad engine — warm drone synthesis centered on the MEDDLING/COMMUNE axis. COMMUNE pulls voices into cooperative unison; MEDDLING pushes them into harmonic interference. The `ohm_macroMeddling` control is uniquely social: it governs how much each voice interferes with its neighbors.

**feliX-Oscar:** -0.5 (OHM is communal and harmonic — slightly Oscar in its ensemble-first design, but the MEDDLING axis can produce aggressive interference)

**Signature moves:**
- Full COMMUNE + slow LFO on `ohm_macroMeddling`: voices slowly drift into and out of unison — natural chorus without any chorus effect; sounds like a choir breathing
- High MEDDLING with a minor 3rd interval: beating interference creates amplitude modulation from voice interaction alone — no tremolo parameter needed
- Use OHM as a drone root: lock it to the key center, push COMMUNE fully, and record a long sustained stem; use it as a pad foundation for any pack

**Best XPN pack type:** Sample library — OHM's drone and interference behaviors are most useful as stems and textural loops; a root-note stem library in 12 keys gives producers a complete ambient foundation kit

**Coupling recommendation:** OHM → OBBLIGATO (COUPLING: AMP). OHM's communal breath feeds OBBLIGATO's dual wind breath envelope — the result is an ensemble where the harmonic interference becomes breath pressure.

---

## 10. ONSET (`perc_`) — Electric Blue `#0066FF`

**Sonic identity:** ONSET is the drums engine — 8 synthesis voices (Kick, Snare, CHat, OHat, Clap, Tom, Perc, FX) with Dual-Layer Blend Architecture (Blessing B006) and XVC Cross-Voice Coupling (Blessing B002). XVC is 3-5 years ahead of the field: voices modulate each other in real time, so a kick can dynamically tighten the snare on the downbeat.

**feliX-Oscar:** +2 (the most feliX engine in the fleet — transient, assertive, rhythmic, attack-forward)

**Signature moves:**
- XVC route Kick → Snare (COUPLING: AMP): kick ducking sidechains the snare body — gives the kit pump without an external compressor
- Blend Circuit and Algorithm layers at 50/50 on the Snare voice: the analog crackle of the circuit layer bleeds into the digital precision of the algorithm layer — uniquely filthy snare character
- Use `perc_noiseLevel` per-voice to sculpt noise character independently; CHat and Clap benefit from different noise spectra than the Kick — dial each voice as its own instrument

**Best XPN pack type:** Drum kit — ONSET is purpose-built for per-pad sampling; 4-velocity layers with XVC pre-mixed into the renders captures the cross-voice coupling character that a raw single-hit library would miss

**Coupling recommendation:** ONSET → OVERWORLD (COUPLING: PITCH). Kick triggers reset OVERWORLD's ERA triangle position — each downbeat shifts the chip character, making the melodic synth layer rhythmically reactive to the groove.

---

## Summary Reference

| Engine | feliX-Oscar | Best Pack Format | Key Parameter |
|--------|-------------|-----------------|---------------|
| OBESE | +1.5 | Keygroup | `fat_mojoControl` |
| ORACLE | -1.5 | Sample Library | `oracle_breakpoints` |
| OUROBOROS | -2 | Sample Library | `ouro_leash` |
| ORGANON | -0.5 | Hybrid | `organon_metabolicRate` |
| OPAL | +0.5 | Keygroup | `opal_grainSize` |
| OVERWORLD | +1 | Keygroup | `ow_era` |
| OCEANIC | 0 | Sample Library | `ocean_separation` |
| OBSIDIAN | -1 | Keygroup | `obsidian_depth` |
| OHM | -0.5 | Sample Library | `ohm_macroMeddling` |
| ONSET | +2 | Drum Kit | `perc_noiseLevel` + XVC |

**feliX-Oscar axis:** +2 = maximally feliX (assertive, forward, transient) | -2 = maximally Oscar (ambient, chaotic, receding)
