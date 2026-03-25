# OPAL — Quick Pass (Guru Bin)
## Date: March 24, 2026
## Depth: Quick (The Finger + The Ear)

---

### Engine Profile

- **Full name:** XOpal — Granular Synthesis
- **Architecture:** 12 clouds, 32 grains, 4-second buffer, 6-slot mod matrix
- **Parameters:** 86 frozen (opal_ prefix)
- **Preset count:** 666 (largest non-percussion library in fleet)
- **Accent color:** Lavender `#A78BFA`
- **Prefix:** `opal_`
- **Key macros:** SCATTER, DRIFT, COUPLING, SPACE (proper labels) — but only 22% of presets use them
- **Moods covered:** Entangled (246), Flux (93), Atmosphere (92), Aether (89), Family (57), Foundation (43), Prism (37), Submerged (9)

---

### Macro Coverage Assessment

| Macro Label | Presets | Macros Active | Verdict |
|-------------|---------|---------------|---------|
| SCATTER/DRIFT/COUPLING/SPACE (correct) | 150 (22%) | 12 of 150 (8%) | Critical gap — proper labels, almost no values set |
| CHARACTER/MOVEMENT/COUPLING/SPACE (generic) | 458 (68%) | 364 of 458 (79%) | Labels wrong, values active — mismatch |
| Other | 58 (10%) | Mixed | Variable |

**The central OPAL paradox:** Presets with the *correct* OPAL-specific macro labels (SCATTER/DRIFT/COUPLING/SPACE) have *less* active macro usage (8%) than presets with *generic* labels (79%). This means the correct-label presets were labeled properly but then left unexpressive. A SCATTER macro at 0.0 in every preset is a broken promise to every player who reaches for it expecting the granular cloud to explode.

---

### Key Findings

**1. Position Parameter Clustered Around Default**
93% of OPAL presets have `opal_position ≤ 0.15` (default cluster). Position controls the playback start point in the grain buffer — it's the granular equivalent of sample scrubbing, and it's one of the most sonically distinctive parameters OPAL offers. Sweeping position changes the entire character of the cloud: early positions (0.0) give attack material, late positions (0.5+) give tail and decay. With 93% of presets at position < 0.15, the entire upper half of the timbral space is unexplored.

**2. Shimmer and Frost Barely Used**
Shimmer (`opal_shimmer`) active in only 17% of presets. Frost (`opal_frost`) active in only 12%. These are OPAL's "Character" controls — the granular texture modifiers that shift the sonic identity beyond raw grain parameters. A granular synth without shimmer is a granular synth without atmosphere. For an engine whose name is the mineral famous for its iridescence, these should be among the most-used parameters in the library.

**3. Mod Matrix Near-Empty**
Only 18% of presets engage the 6-slot mod matrix. This is the most damning finding for a modular granular engine: the mod matrix is what makes a granular cloud *respond* — LFO→grain size, envelope→scatter, velocity→density. Without mod matrix connections, OPAL presets are static photographs of grain settings. A granular synthesizer should almost never have an empty mod matrix.

**4. The "Grain X" Naming Problem**
38 presets are named with "Grain " as a prefix: "Grain Cathedral," "Grain Memory," "Grain Storm," etc. These are accurate descriptions of the synthesis type but uninspiring as preset names. The name "Grain Cathedral" tells the player what *method* was used, not what the sound *is*. Contrast with "Drift Cathedral" — the same space, but the name conjures the experience rather than the DSP.

**5. Entangled Coupling Presets Are Skeletal**
The Entangled mood holds 246 OPAL presets — 37% of the library. But these are disproportionately *coupling concept sketches* rather than fully developed presets. "Grain Luminance" (OPTIC coupling), "Grain Chromatophore" (Octopus coupling), "Grain Migration" (ORCA coupling) — the coupling concept is evocative, but the underlying OPAL parameters are almost completely unset: no grain_size, no density, no position, no scatter, no shimmer/frost. These presets exist as *annotations of an idea* rather than playable sounds.

**6. Freeze Mode Almost Unused**
Freeze (`opal_freeze`) active in only 4% of presets (33 of 666). Freeze locks the buffer position and creates infinite granular sustain — a core granular technique pioneered by Envelop's use of position freeze and by Ableton's Granulator. An engine with 32 grain streams and a freeze parameter that's used in only 4% of presets has left its most meditative capability almost entirely unexplored.

**7. The OSC Dual-Source is Untapped**
OPAL has `opal_osc2Shape`, `opal_osc2Mix`, and `opal_osc2Detune` — a secondary oscillator as the grain source. This feature is referenced in almost no presets at non-zero values. The dual-oscillator granular source (one oscillator per cloud, with detune between them) creates a beating, pulsating grain texture that is entirely absent from the library.

---

### 5 Weakest Presets (Priority Refinement Targets)

| Preset | Mood | Score | Primary Issues |
|--------|------|-------|----------------|
| Grain Luminance | Entangled | 8 | No granular params set, no macros, no mod, position default, generic name |
| Grain Chromatophore | Entangled | 8 | Coupling sketch only — filterCutoff set but no grain engine params |
| Grain Knot | Entangled | 8 | Same pattern — granular topological concept without granular parameters |
| Grain Migration | Entangled | 8 | ORCA coupling annotation; grain_size set but density/position/scatter absent |
| Grain Memory Fade II | Entangled | 8 | The "position scrubbing" description is exactly right — but position=0.0 |

---

### Recommended Refinements

| Preset | Issue | Suggestion |
|--------|-------|-----------|
| Grain Luminance | No grain params set — only filterCutoff=2800 exists | grain_size=80, density=12, position=0.0, posScatter=0.2, pitchScatter=0.5, shimmer=0.4 (luminance). Mod1: LFO→posScatter (rate 0.08, depth 0.15) for breathing light. SCATTER macro 0.2, DRIFT 0.15. Rename: "Bioluminescent Cloud" |
| Grain Chromatophore | filterCutoff=5114, nothing else | grain_size=45, density=25, position=0.05, posScatter=0.35, panScatter=0.25, shimmer=0.3. Mod1: envelope→density (mod1Src=2, mod1Dst=density, mod1Amt=0.4). SCATTER 0.35, DRIFT 0.2. The chromatophore idea = rapid scatter variation → Mod matrix is the solution |
| Grain Knot | grain_size=0.45, filterCutoff=0.55, nothing else | grain_size=120, density=8, position=0.25 (mid-buffer, knot sits in the middle of the loop), posScatter=0.08, pitchScatter=0.3, shimmer=0.2, frost=0.3. SCATTER 0.08 (the knot is tight), DRIFT 0.35. Rename: "Topological Knot Cloud" |
| Grain Migration | grain_size=0.363, filterCutoff=0.556, nothing else | grain_size=200 (large grains = whale-scale migration), density=6, position=0.0, posScatter=0.15, pitchScatter=2.0, shimmer=0.1, frost=0.0. Mod1: velocity→density (loud strikes = denser cloud). SCATTER 0.15, DRIFT 0.45. Rename: "Whale Migration Cloud" |
| Grain Memory Fade II | filterCutoff=7542, nothing else — description says "position scrubbing backward" | grain_size=300 (long grains = sustained memory), density=10, position=0.55 (late buffer — the memory fades from the past), posScatter=0.05 (tight — memory is specific), pitchScatter=0.8, shimmer=0.0, frost=0.25 (frozen recollection). SCATTER 0.05, DRIFT 0.25 (slow fade). Mod1: LFO→position (rate 0.003 Hz, depth 0.1 — position drifts backward over 5 minutes) |

---

### Naming Improvements

| Current Name | Suggested Name | Why |
|-------------|---------------|-----|
| Grain Luminance | Bioluminescent Cloud | "Luminance" is an adjective describing DSP; "Bioluminescent Cloud" is a living image |
| Grain Chromatophore | Colour Shift Cloud | The chromatophore concept = instant colour change = scatter explosion |
| Grain Knot | Topological Knot Cloud | Retains the ORBWEAVE coupling concept with more visual specificity |
| Grain Migration | Whale Migration Cloud | The ORCA coupling story becomes the name |
| Grain Memory Fade II | Memory Dissolve | "Fade II" is a sequel number; "Dissolve" captures the backward scrub concept |
| Grain Drift | Cloud Drift | "Grain" as a prefix is always too technical — the experience is the drift, not the grain |
| Grain Solitude | Solitary Cloud | Same fix — "Solitude" is the feeling, not "Grain Solitude" |
| Grain Storm | Cloud Tempest | "Storm" is already evocative; "Grain Storm" reduces it to its DSP |
| Radiant Grain Flow | Radiant Flow | Remove the redundant "Grain" |
| DARK COLD KINETIC VAST... (DNA names) | Full rename required | These auto-generated DNA-descriptor names should be replaced with evocative image-based names |

---

### Next Steps

- [ ] **Full retreat recommended? YES — HIGHEST PRIORITY among the three engines.** OPAL has the largest library (666 presets) and the most systemic quality issues. The Entangled coupling sketch problem alone represents 80+ presets that need parameter completion.
- [ ] **Entangled coupling sketch completion** — All Entangled OPAL presets identified as having only filterCutoff or 1-2 params set need full granular parameter builds. This is not a small fix; it's a dedicated session.
- [ ] **SCATTER/DRIFT macro population** — The 150 presets with correct labels but near-zero macro values should all receive SCATTER values that represent the preset's actual scatter philosophy. At minimum: SCATTER maps to `opal_posScatter`, DRIFT maps to `opal_macroDrift`.
- [ ] **Position exploration** — Write 20 presets specifically exploring position values 0.2–0.6. These presets don't exist and they represent an entirely undiscovered timbral region.
- [ ] **Freeze awakening** — Write 10 dedicated Freeze presets: grain_freeze=1.0, position used as the frozen moment. SCATTER macro controls how far grains scatter from the frozen position.
- [ ] **Mod matrix activation pass** — Any preset with an empty mod matrix should receive at least one connection: LFO1→posScatter (rate 0.1–0.5 Hz, depth 0.1–0.3) is the minimum viable OPAL preset.
- [ ] **DNA-descriptor renaming** — The 41 "BRIGHT WARM KINETIC..." named presets need proper names. Write the brief for each, then name them.
- [ ] Sweet spots discovered: `grain_size=80–150ms, density=15–25, posScatter=0.2–0.35` = the OPAL sweet spot for atmospheric granular pads. `freeze=1.0, posScatter=0.0–0.05, pitchScatter=1.0–3.0` = meditative grain drone. `grain_size=20–40ms, density=45–80, shimmer=0.4+` = metallic shimmer texture that cannot be achieved elsewhere in the fleet.
