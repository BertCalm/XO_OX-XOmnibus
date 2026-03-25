# OBRIX — Quick Pass (Guru Bin)
## Date: March 24, 2026
## Depth: Quick (The Finger + The Ear)

---

### Engine Profile

- **Full name:** XObrix — Ocean Bricks: The Living Reef
- **Seance score:** ~8.5/10 (V1 flagship, B016 AMENDED, Wave 5 complete)
- **Parameters:** 81 (Wave 5 — Reef Residency)
- **Accent color:** Reef Jade `#1E8B7E`
- **Prefix:** `obrix_`
- **Waves:** 5 (Sources, Processors, Modulators, FX, Reef Residency)
- **Presets sampled:** 466 (full library scan)
- **Moods covered:** Foundation (83), Flux (77), Aether (69), Prism (66), Atmosphere (66), Submerged (49), Entangled (33), Family (23)

---

### Macro Coverage Assessment

| Macro | Zero% | Active% | Verdict |
|-------|-------|---------|---------|
| CHARACTER | 62% zero | 38% active | Thin — needs population |
| MOVEMENT | 28% zero | 72% active | Healthy |
| COUPLING | 86% zero | 14% active | Critical gap |
| SPACE | 60% zero | 40% active | Thin |

**Overall macro verdict:** COUPLING macro is nearly unused across the library (86% at zero). For a flagship modular engine where coupling is a first-class interaction mode, this is a significant gap. Presets that engage COUPLING as a live performance dimension are rare and should be prioritized in a full retreat.

---

### Key Findings

**1. Wave 4/5 Adoption is Critically Low**
Only 12% of presets engage any Wave 4 parameters (Harmonic Field, Environmental, Brick Ecology, Stateful Synthesis). This means 409 of 466 presets treat OBRIX as a Wave 1-3 instrument. The JI attractor (`obrix_fieldStrength`), environmental state (`obrix_envTemp/Pressure/Current/Turbidity`), and ecological competition (`obrix_competitionStrength/symbiosisStrength`) — the features that make OBRIX uniquely *alive* — are almost entirely absent from the library.

**2. Drift Bus Underused in Half the Library**
45% of presets have `obrix_driftDepth > 0`. But the Drift Bus (0.001–0.05 Hz ultra-slow LFO with per-voice irrational phase offsets) is the Berlin School ensemble drift that separates OBRIX from a static modular patch. 55% of presets are static in time in a way OBRIX was not designed for.

**3. FX Chain Bypassed in 34% of Presets**
163 presets have all three FX slots at zero mix. While some patches (subs, leads) legitimately need no FX, this proportion in a habitat engine with Delay/Chorus/Reverb suggests many presets were built in isolation rather than as sonic environments.

**4. Tutorial Presets Mixed into Foundation**
Multiple TUTORIAL-prefixed presets ("TUTORIAL OBRIX 01 Single Brick Found", "TUTORIAL OBRIX 02 Drift Bus Demo", "TUTORIAL OBRIX 03 Journey Arc") live in Foundation and Atmosphere moods alongside production-ready presets. These should be tagged `tier: tutorial` and separated from the browsable library or given evocative names that honor their pedagogical purpose.

**5. Foundation Presets Are Intentionally Sparse — But Too Many Are Static**
Foundation mood rightfully features minimal presets for pedagogical demonstration. However, presets like "Single Polyp," "Hollow Reef," "Sine Foundation," and "The Collision" have all four macros at exactly 0.0 with no modulation. Even Foundation presets should offer one macro dimension of expressivity. A player who grabs "Sine Foundation" deserves at least one knob that makes it breathe.

**6. FM Depth and Unison Largely Untapped**
`obrix_fmDepth` (source-to-source FM ±24 semitones) and `obrix_unisonDetune` appear at non-zero values in very few presets. The FM coupling between sources is one of OBRIX's most sonically distinctive capabilities and is barely explored.

---

### 5 Weakest Presets (Priority Refinement Targets)

| Preset | Mood | Score | Primary Issues |
|--------|------|-------|----------------|
| The Collision | Foundation | 6 | All macros zero, no FX, no modulation, no drift — Wave4 absent |
| First Reef Wall | Foundation | 6 | All macros zero, no FX, no drift — Wave4 absent |
| Single Polyp | Foundation | 6 | All macros zero, no modulation — described as "smallest living unit" but has no expression |
| Hollow Reef | Foundation | 6 | All macros zero, no modulation, no FX — Wave4 absent |
| Sine Foundation | Foundation | 6 | All macros zero, no modulation, no FX — "pure sub bass" but dead in time |

---

### Recommended Refinements

| Preset | Issue | Suggestion |
|--------|-------|-----------|
| The Collision | All 4 macros at 0.0, no drift | Set MOVEMENT → `obrix_driftDepth` 0–0.15. Set CHARACTER → `obrix_srcMix` (crossfade the collision dynamically). Add Reverb FX1 at 0.15 mix. driftDepth=0.05, driftRate=0.008 minimum. |
| First Reef Wall | Static — cutoff fixed, no expression | Set CHARACTER → `obrix_proc1Cutoff` sweep range. Add mod1 as ADSR env → filter (mod1Type=1, mod1Target=2, mod1Depth=0.5). "Reef wall breathes" concept — cutoff ebbs. |
| Single Polyp | Single sine, nothing moves | Add slow LFO as mod1 → filter (mod1Type=2, rate=0.15 Hz, depth=0.3). Set MOVEMENT → mod rate. Give it a real identity: "A polyp pulsing with bioluminescence" — driftDepth=0.02. |
| Hollow Reef | Square wave, completely static | Two sources: keep src1=Square, add src2=Sine at -12st, srcMix=0.3. Add proc3 LPF at 800 Hz. MOVEMENT → proc3 cutoff sweep. Rename: "Hollow Chamber" |
| Sine Foundation | Pure sine, nothing added | Add fieldStrength=0.3, fieldPrimeLimit=0 (3-limit) for subtle JI gravity. driftDepth=0.04. CHARACTER macro → `obrix_fieldStrength` to move between untuned and harmonically locked. |

---

### Naming Improvements

| Current Name | Suggested Name | Why |
|-------------|---------------|-----|
| TUTORIAL OBRIX 01 Single Brick Found | Coral Seed | "Tutorial" in a browsable preset name breaks immersion; the concept (first building block) becomes the name |
| TUTORIAL OBRIX 02 Drift Bus Demo | Reef Drift | The Drift Bus is poetic enough without needing "demo" in the name |
| TUTORIAL OBRIX 03 Journey Arc | Journey Mode | Concise, retains the Journey Mode concept |
| Brick Supersaw | Coral Array | "Brick" is internal vocabulary; the sound is a living array of corals |
| Laser Grid (Obrix) | Sonar Grid | "Laser" doesn't fit the aquatic mythology; "Sonar" does |

---

### Next Steps

- [ ] **Full retreat recommended? YES** — Wave4 adoption (12%) is critically low for a 5-Wave flagship engine. A full retreat should focus on "The Wave4 Awakening": 20+ presets that make each Wave4 system (Harmonic Field, Environmental, Ecology, Stateful) a character in the library.
- [ ] **Priority: COUPLING macro** — 86% of presets leave COUPLING at zero. For V1 release, the flagship engine should have 30+ presets where COUPLING is a designed performance axis.
- [ ] **Drift Bus pass** — 55% dead in time. A "Drift Bus Activation" pass across the entire library (add driftDepth ≥ 0.04 to any preset that doesn't have a specific reason to be static) would raise the floor.
- [ ] **Tutorial preset quarantine** — Move TUTORIAL-prefixed presets to a separate `tutorial` tier or rename them to evocative names.
- [ ] **FM + Unison exploration** — No dedicated FM-heavy or Unison-heavy presets identified. At minimum 15 presets should foreground these Wave 2 features as their sonic identity.
- [ ] Sweet spots discovered: `fieldStrength=0.5–0.8` with `fieldPrimeLimit=1` (5-limit JI) creates the most distinctive OBRIX-only harmonic gravity. `competitionStrength=0.3–0.5` with `src1Type=Sine, src2Type=Noise` creates living breath. `envPressure=0.7` dramatically alters LFO rate character.
