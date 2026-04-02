# ORPHICA Synthesis Guide

**Engine:** ORPHICA | **Accent:** Siren Seafoam `#7FDBCA`
**Parameter prefix:** `orph_` | **Max voices:** 16

---

## What It Is

ORPHICA is a microsound harp — a physical modeling engine where each plucked string passes through a grain processor before it leaves the voice. The underlying sound is a waveguide harp: clear, pitched, decaying. What makes ORPHICA different from every other string engine is the per-voice microsound layer that captures the string's own output into a small circular buffer and reads it back as clouds of overlapping grains. The string and the grain cloud exist in the same voice; they share the same pitch. The result sits somewhere between a plucked instrument and a dispersed texture — precise and crystalline in the transient, diffuse and shimmering in the sustain.

## The DSP Engine

Each ORPHICA voice runs a `PluckExciter` into a Karplus-Strong waveguide loop: exciter → delay line → damping filter → body resonance → sympathetic bank → output. The PluckExciter fires at note-on with a configurable brightness, and `FamilyOrganicDrift` adds continuous slow pitch microvariations. String material (Nylon, Steel, Crystal, Light) adjusts damping and brightness: Nylon is warm and quick-decaying, Steel is brighter and longer, Crystal adds a negative damping offset for glassier sustain, Light (ethereal) is the most open with the shortest natural decay.

The per-voice `OrphicaMicrosound` engine captures the waveguide output sample-by-sample into an 8192-sample (~186ms at 44.1kHz) circular buffer, then reads four overlapping Hann-windowed grains simultaneously. Grain mode determines where each grain reads from: **Stutter** reads from just behind the write head (immediate repeat), **Scatter** randomizes read position by a scatter amount (up to `kBufSize` samples back), **Freeze** locks read positions and stops writing (instant drone), **Reverse** reads backwards from the write head.

The FX chain is split into two paths by a configurable crossover note. Below the crossover, voices route to the LOW path: sub-octave sine, tape saturation, dark delay, and deep plate reverb. Above the crossover, voices route to the HIGH path: shimmer reverb (pitch-shifted feedback), micro delay, spectral smear, and crystal chorus. The stereo spread follows: LOW leans left, HIGH leans right, producing a natural low-left / high-right imaging without any explicit panning logic.

## The Voice Architecture

16 polyphonic voices — the largest voice count in the Constellation Family — because ORPHICA is designed to be layered. Each voice carries its own grain buffer, meaning 16 simultaneous grain clouds are possible. The microsound engine processes each voice individually before the crossover split, so the grain texture follows the pitch. High notes get shimmer-reversed micro delay; low notes get tape warmth and plate reverb. The distinction is not user-configurable — it emerges from where each note falls relative to the crossover note, which defaults to C4 (MIDI 60) with a 6-semitone blend zone.

## The Macro System

### PLUCK (M1)
PLUCK governs the sharpness and presence of the attack transient. It adds directly to the pluck brightness parameter and scales the gain of the exciter signal into the waveguide. At low PLUCK you get a soft, airy initiation — the string seems to appear from nowhere. High PLUCK is a hard, defined transient with immediate harmonic content. PLUCK is the macro to automate when you want ORPHICA to cut through a dense arrangement vs. dissolve into an ambient texture. Because it operates on the exciter gain, it also affects how long the sympathetic strings ring: harder plucks energize the sympathetic bank more.

### FRACTURE (M2)
FRACTURE drives microsound intensity. It adds to the `orph_microMix` parameter and increases grain scatter simultaneously. At zero, the waveguide is clean and the grain layer is silent. As FRACTURE rises, the sustain begins to fragment — grains scatter across the buffer history, creating pitch-smeared echoes and ghost harmonics. Above 0.7, FRACTURE produces an almost percussive grain storm on sustained notes. FRACTURE works best in Scatter or Reverse microsound mode, where the pitch information in the grains is partially destroyed. In Freeze mode, high FRACTURE creates an instant granular drone that ignores new note input until FRACTURE drops.

### SURFACE (M3)
SURFACE shifts the balance between the LOW and HIGH FX paths by biasing the crossover note split. Below 0.5, more voices route to the LOW path (warmer, more reverberant, plate-heavy). Above 0.5, more voices route to the HIGH path (brighter, more shimmer, chorus-widened). SURFACE does not change timbre directly — it redistributes the spatial character. Use it to tilt ORPHICA from a deep resonant instrument to a high, shimmering one within a single preset without touching individual brightness parameters. At 0.5, the crossover lands at the note defined by `orph_crossoverNote`.

### DIVINE (M4)
DIVINE scales the three highest-order effects simultaneously: shimmer reverb mix, deep plate mix, and spectral smear amount. It is the macro for atmosphere. Low DIVINE is transparent — the harp exists in a dry space. High DIVINE dissolves the instrument into a luminous field. At maximum, the spectral smear granularizes the FX output itself, and the shimmer verb pitches up the reverb tails, creating octave ghosts that ring independent of the original note. DIVINE is the macro for the ending of a track — play a chord, push DIVINE, and let it unfold.

## Key Parameters

| Parameter | Range | Function |
|-----------|-------|----------|
| `orph_stringMaterial` | 0–3 (choice) | Nylon, Steel, Crystal, Light — affects damping and brightness |
| `orph_pluckBrightness` | 0–1 | Base pluck transient brightness |
| `orph_pluckPosition` | 0–1 | 0 = near bridge (bright), 1 = near nut (dark) |
| `orph_stringCount` | 1–6 | Number of sympathetic strings per voice |
| `orph_bodySize` | 0–1 | Body resonance frequency and Q — 0 is small/bright, 1 is large/dark |
| `orph_sympatheticAmt` | 0–1 | Sympathetic string amplitude |
| `orph_damping` | 0.8–0.999 | Feedback loop damping — higher sustains longer |
| `orph_microMode` | 0–3 (choice) | Stutter, Scatter, Freeze, Reverse |
| `orph_microRate` | 0.5–50 Hz | Grain trigger rate |
| `orph_microSize` | 5–200ms | Grain window length |
| `orph_microDensity` | 1–20 | Grain overlap — higher = more simultaneous grains |
| `orph_microScatter` | 0–1 | Position randomization in Scatter mode |
| `orph_microMix` | 0–1 | Grain cloud dry/wet |
| `orph_crossoverNote` | 36–84 (MIDI) | Frequency where LOW and HIGH FX paths split |
| `orph_subAmount` | 0–1 | Sub-octave sine added to LOW path |
| `orph_shimmerMix` | 0–1 | Shimmer reverb amount in HIGH path |
| `orph_spectralSmear` | 0–1 | Granular dissolve on HIGH path output |

## Sound Design Recipes

**The Harp** — Material: Nylon. PLUCK 0.5, FRACTURE 0, SURFACE 0.5, DIVINE 0.25. String count 5, sympathetic 0.4, damping 0.994. Body size 0.4. Micro mix 0. Plate reverb 0.3. A transparent, pitched harp with natural decay. The preset `Family_Sister_Harmony` lives in this territory.

**Crystal Delay** — Material: Crystal. PLUCK 0.7, FRACTURE 0, SURFACE 0.7, DIVINE 0.6. Shimmer mix 0.5, micro delay 8ms, crystal chorus rate 2.0. Crystal material's lower damping produces long, glassy sustain. The HIGH path shimmer verb creates an octave-up ghost that rings through the attack and sustain independently.

**Frozen Lake** — Microsound mode: Freeze. PLUCK 0.3, FRACTURE 0.8, SURFACE 0.3, DIVINE 0.7. Micro mix 0.9, micro size 120ms, micro density 12. Play a chord, let the freeze capture it, then play new notes — the frozen chord drone underlies everything new. A one-voice ambient system.

**Shattered Glass** — Material: Light. Microsound mode: Scatter. PLUCK 0.9, FRACTURE 0.9, SURFACE 0.8, DIVINE 0.4. Micro scatter 0.8, micro rate 25 Hz, micro size 20ms. High pluck with maximum scatter creates a struck-and-shattered transient: the initial note is clear, then the grain cloud disperses it into pitch-smeared debris.

**Sub Harp** — Material: Steel. PLUCK 0.6, FRACTURE 0.1, SURFACE 0.2, DIVINE 0.3. Sub amount 0.5. Crossover note 72. Push all voices into the LOW path (low crossover note), add sub-octave. The steel string sustains while the sub adds weight below it. Works as a bass instrument in a sparse arrangement.

## Family Coupling

ORPHICA accepts `LFOToPitch` (external pitch wobble in semitones — try from OHM's organic drift output), `AmpToFilter` (increases damping, shortening sustain — useful for OBBLIGATO's breath to gate ORPHICA's decay), and `EnvToMorph` (scales pluck exciter intensity, letting OTTONI's brass attacks drive ORPHICA's string energy). ORPHICA's output is dense and harmonically complex — ideal for coupling into OPAL as an audio source or into OVERWORLD's ERA crossfade. The dual LOW/HIGH path output is already stereo-spread, so coupling ORPHICA's amplitude to another engine's filter cutoff produces a spatially animated modulation rather than a flat one.

## Tips & Tricks

- Pluck position controls where along the string the virtual pick strikes. Near the bridge (0.0) yields the bright, nasal tone typical of guitar harmonics. Near the nut (1.0) produces a dark, thick tone with more fundamental than overtone content. This is independent of string material — combine Crystal material with a near-nut pluck for an unusual dark crystal texture.
- Freeze mode is an ambient instrument by itself. Set FRACTURE high, DIVINE high, play a chord in the low register, and freeze it. The grain engine will loop and scatter the frozen moment indefinitely. Play a counter-melody with ORPHICA in the high register — you now have a drone pad and a lead from the same engine.
- The spectral smear (`orph_spectralSmear`) in the HIGH path is a granular dissolve applied to the FX bus, not the dry signal. At low values it adds a gentle motion to the shimmer verb output. At high values it turns the verb tail into a granular texture of its own — especially effective on long high notes.
- With 16 voices, ORPHICA can sustain a full chord and still have capacity for new notes. Do not use it as a one-at-a-time lead. It is a chord engine. Play arpeggios and let the notes accumulate — the grain clouds from earlier voices layer under newer ones.
- DIVINE at max plus Freeze mode plus spectral smear 0.8 creates a sound that has stopped being a harp and become a cloud. The instrument identity dissolves completely. This is a feature, not a bug.
