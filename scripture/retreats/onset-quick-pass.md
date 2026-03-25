# ONSET — Quick Pass (Guru Bin)
## Date: March 24, 2026
## Depth: Quick (The Finger + The Ear)

---

### Engine Profile

- **Full name:** XOnset — The Surface Splash
- **Seance score:** Flagship V1 candidate. B002 (XVC Cross-Voice Coupling), B006 (Dual-Layer Blend)
- **Architecture:** 8 voices × Layer X (Circuit: 808/909 topology) + Layer O (Algorithm: FM/modal/KS/PD)
- **Parameters:** 111 (5 phases complete)
- **Preset count:** 636 (largest percussion library in fleet)
- **Accent color:** Electric Blue `#0066FF`
- **Prefix:** `perc_`
- **Moods covered:** Entangled (193), Foundation (174), Flux (86), Prism (67), Atmosphere (48), Aether (38), Family (25), Submerged (5)

---

### Macro Coverage Assessment

| Macro | Zero% | Active% | Verdict |
|-------|-------|---------|---------|
| COUPLING | 26% zero | 74% active | Good — above fleet average |
| SPACE | 0% zero | 100% active | Excellent |
| CHARACTER | 26% zero | 74% active | Good |
| MOVEMENT | 26% zero | 74% active | Good |

**Overall macro verdict:** ONSET macro engagement is genuinely strong. The 247 presets using generic CHARACTER/MOVEMENT labels (versus the proper MACHINE/PUNCH/SPACE/MUTATE vocabulary) are the primary concern — not macro inactivity, but macro *mislabeling*. A player reaching for "PUNCH" and finding "MOVEMENT" is disoriented. The correct label vocabulary is already established in 303 presets; the remaining 247 need migration.

---

### Key Findings

**1. Label Vocabulary Split: Two Dialects in One Library**
303 presets use the proper ONSET vocabulary (MACHINE/PUNCH/SPACE/MUTATE). 247 presets use the generic fleet vocabulary (CHARACTER/MOVEMENT/COUPLING/SPACE). This creates a confusing browsing experience: players learning ONSET through its custom labels will encounter presets that don't speak the same language. Migration is straightforward: the CHARACTER macro in old-vocabulary presets maps cleanly to MACHINE, and MOVEMENT maps to PUNCH. This is a library normalization task, not a sound design task.

**2. XVC Cross-Voice Coupling Activated in 49% of Presets**
312 of 636 presets have `perc_xvc_global_amount > 0`. This is strong relative to the fleet (B002 is the flagship ONSET blessing — XVC is its signature feature), but it also means 49% of the library operates without the feature that makes ONSET a *neuron network* rather than a drum machine. Every preset should have considered whether XVC serves the concept; 49% apparently didn't.

**3. Diversity Anchor Presets Are Technically Weak**
Several presets identified in the weakest tier ("Sparse Crystal", "Violent Omen Split", "Machine Flux Blaze") have descriptions explicitly framing them as "extreme-DNA diversity anchors" or "zone fillers." These were generated to fill cosine diversity gaps in the preset space — not as sound design artifacts. They have no macro mappings, no XVC, no FX, and minimal parameters. They are the most mechanically produced presets in the library and the most sonically hollow.

**4. Flux Mood Imbalance**
Flux (86 presets) skews heavily toward "electronic machine" territory — Onset Flux Maximum, Machine Flux Blaze, COLD FLUX 7, CRYO FLUX 5, SPARSE FLUX 3, SPARSE FLUX 8. The naming pattern suggests batch generation. Flux as a mood should capture transformation and motion — but these presets freeze in one posture.

**5. Submerged Mood Almost Empty**
5 Submerged presets for a 636-preset percussion library is a significant gap. Underwater percussion, deep resonance, hydrostatic decay — the Submerged mood is where ONSET's Layer O modal resonators would shine most. This is an opportunity for differentiated sound design, not gap-filling.

**6. The B006 Blend Axis is Undertapped in Descriptions**
The Circuit/Algorithm blend (Layer X ↔ Layer O) is B006 — one of ONSET's two blessings. Yet most preset descriptions don't narrate where on the blend axis the preset lives, or what the BLEND-related macro does. The story of each preset should include its position in the X↔O spectrum.

---

### 5 Weakest Presets (Priority Refinement Targets)

| Preset | Mood | Score | Primary Issues |
|--------|------|-------|----------------|
| Sparse Crystal | Foundation | 7 | DNA anchor — no macros, no XVC, no FX, template description |
| Cryogenic Flash | Flux | 7 | Cold transient concept with one param set, no macro expression |
| Onset Flux Maximum | Flux | 7 | Zone filler — "ultra-bright" as the entire design brief |
| Machine Flux Blaze | Flux | 7 | Zone filler — same template as Onset Flux Maximum |
| Violent Omen Split | Prism | 6 | DNA anchor — no macros, no XVC, no FX |

---

### Recommended Refinements

| Preset | Issue | Suggestion |
|--------|-------|-----------|
| Sparse Crystal | DNA anchor with no expression | Wire MACHINE → `perc_macro_machine` 0.3, PUNCH → `perc_macro_punch` 0.5. Add `perc_xvc_global_amount = 0.2`. Add reverb mix 0.12. Rename to "Crystal Sparse Kit" — give it an identity beyond DNA filling. |
| Cryogenic Flash | Only `perc_v1_decay` set | Complete the voice: add `perc_char_warmth = 0.1`, `perc_fx_reverb_mix = 0.08` (cold room decay). Set MACHINE → machine 0.2, PUNCH → punch 0.6. XVC: kick→snare filter 0.15 (tightens in the cold). |
| Onset Flux Maximum | Zone filler | Reframe as a real preset: "Plasma Barrage" — extreme brightness as the *concept*. Add `perc_char_grit = 0.6`, `perc_fx_delay_mix = 0.08`, XVC global 0.35. Rename. |
| Machine Flux Blaze | Duplicate of above pattern | Differentiate: Layer O emphasis (high blend value), FM algorithm, `perc_fx_lofi_bits = 6` for digital edge. Rename "Feedback Blaze" — the Layer O FM distortion at high brightness. |
| Violent Omen Split | DNA anchor | Wire macros: MACHINE 0.45, PUNCH 0.65, SPACE 0.3, MUTATE 0.2. Add reverb. Rename "Violent Omen" (drop "Split") — the name already has character; remove the procedural suffix. |

---

### Naming Improvements

| Current Name | Suggested Name | Why |
|-------------|---------------|-----|
| COLD FLUX 7 | Cold Machine State | "COLD FLUX 7" reads as a serial number, not a sound; the concept is a cold machine locked in flux |
| CRYO FLUX 5 | Frozen Transient | Same issue — the sonic concept deserves a name |
| SPARSE FLUX 3 | Sparse Room | "SPARSE FLUX 3" is a label from a template generator |
| SPARSE FLUX 8 | Ghost Room (Eight Strikes) | Retains the sparseness concept with sonic narrative |
| Onset Flux Maximum | Plasma Horizon | "Flux Maximum" sounds like a CPU utilization warning |
| Machine Flux Blaze | Feedback Blaze | Cleaner, retains the fire metaphor with the Layer O algorithm angle |

---

### Next Steps

- [ ] **Full retreat recommended? YES** — Priority: the 247 old-vocabulary presets need macro label migration (CHARACTER→MACHINE, MOVEMENT→PUNCH). This is the single highest-leverage action for ONSET's library coherence.
- [ ] **XVC saturation pass** — Raise XVC global amount to ≥ 0.15 in any preset that doesn't have a specific reason to be voice-isolated. Target: 70%+ of presets actively engaging the B002 feature.
- [ ] **Submerged expansion** — Write 15 Submerged-mood presets: deep cave resonance, underwater transients, hydrostatic decay. Layer O modal resonators at high body, low brightness, long decay.
- [ ] **FLUX mood quality pass** — The 8 "COLD/CRYO/SPARSE FLUX N" presets need renaming and macro population. As a batch, they drag the browsable experience down.
- [ ] **Diversity anchor audit** — Identify all presets with descriptions containing "DNA anchor" or "zone filler" and either promote them to real presets (with sound design) or quarantine them.
- [ ] Sweet spots discovered: `perc_char_warmth = 0.6–0.8` with Layer X (Circuit) creates the 808 heart of ONSET. `perc_xvc_kick_to_snare_filter = 0.3` + `perc_xvc_kick_to_tom_pitch = 0.2` recreates the sidechain compression feel of classic boom bap. `perc_fx_lofi_bits = 8–10` with Layer O (Algorithm) FM creates the MPC-chopped-sample texture.
