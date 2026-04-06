# OBRIX Pocket — Combined 5-Lens Audit
## 2026-04-05 | PC + TIDEsigns + QDD + Feature Junkies + Sisters

**Reviewers**: 28 across 5 skill lenses
**Composite Score**: 4.9/10 → Target 8.0+
**Verdict**: FIX LIST (not SHIP, not BLOCK)

---

## Dimensional Scores

| Dimension | Score | Source |
|-----------|-------|--------|
| Audio Bridge / DSP Safety | 8.5/10 | PC + Masters + Ghost Council |
| Musical Correctness | 3.5/10 | Knowledge Squad + Wendy Carlos + Mizuguchi |
| UI/UX Polish | 4.0/10 | Coral + Sheen + UX Drill Sergeant |
| Accessibility | 2.5/10 | Haven + UX Drill |
| Game Loop / Retention | 5.5/10 | Tajiri + Miyamoto + Snob Squad |
| Architecture / Extensibility | 5.0/10 | Lattice + Forge + PC Atelier |
| Vision Alignment | 4.5/10 | Marina + Ghost Council verdicts |
| Feature Depth | 6.0/10 | Feature Junkies |

---

## TIER 0: SHIP BLOCKERS (12)

| # | Finding | File:Line | Complexity |
|---|---------|-----------|------------|
| S1 | Chord mode hardcodes major triads regardless of scale | PlayKeyboard.swift:305-315 | Small |
| S2 | Onboarding is completely silent — audio-first product with mute intro | OnboardingView.swift (entire) | Medium |
| S3 | `activeVoiceCount` returns hardcoded 0 | ObrixBridge.mm:485-489 | Small |
| S4 | `.xoreef` import fails silently — no user feedback | ObrixPocketApp.swift:182-188 | Small |
| S5 | HarmonicContext unconditional 7th on ALL chords | HarmonicContext.swift:203-214 | Small |
| S6 | Touch targets: black keys 16pt (HIG minimum: 44pt) | PlayKeyboard.swift:133 | Medium |
| S7 | SpectralCapture FFT use-after-free on cancel | SpectralCapture.swift:229-245 | Medium |
| S8 | Shared UserDefaults key corrupts both energy systems | FirstLaunchManager.swift:149, ReefEnergyManager.swift:20 | Small |
| S9 | PlayKeyboard noteOn/noteOff bypass AudioEngineManager | ReefTab.swift:211,236 | Medium |
| S10 | VoiceOver: PlayKeyboard has zero accessibility | PlayKeyboard.swift | Medium |
| S11 | VoiceOver: ReefScene overlay taps non-functional | ReefAccessibilityOverlay | Small |
| S12 | Contrast failures: mutedText 3.7:1, onboarding 2.3:1 | DesignTokens.swift:79, OnboardingView.swift:200 | Small |

### Ghost Council Votes on Ship Blockers

| Finding | BLOCK | CONDITIONAL | BLESS |
|---------|-------|-------------|-------|
| S1 Chord mode | 8/8 | 0 | 0 |
| S2 Silent onboarding | 8/8 | 0 | 0 |
| S3 Voice count 0 | 6/8 | 2 | 0 |
| S4 Import silent fail | 6/8 | 2 | 0 |
| S5 Unconditional 7th | 6/8 | 2 | 0 |
| S6 Touch targets | 6/8 | 2 | 0 |
| S7 SpectralCapture race | 0 | 8/8 | 0 |

---

## TIER 1: LAUNCH BLOCKERS (15)

| # | Finding | File:Line |
|---|---------|-----------|
| L1 | Breeding: no hybrid subtypes — offspring is pure parent copy | BreedingSystem.swift:429-431 |
| L2 | Param namespace collision: 3 files disagree on noise-pink src1Type | SpecimenFactory:140, SpecimenHashGenerator:208, AudioEngineManager:188 |
| L3 | Arp timer not invalidated on background — stuck notes | PlayKeyboard.swift:480 |
| L4 | Arp velocity locked at 0.7, bypasses 3D Touch | PlayKeyboard.swift:477 |
| L5 | KeyboardScale has no minor pentatonic | PlayKeyboard.swift:24-28 |
| L6 | Lydian maps to Major — #4 lost | HarmonicDNA.swift:651-652 |
| L7 | pentatonicMinor maps to major pentatonic | HarmonicDNA.swift:644-645 |
| L8 | DiveComposer tick unsynchronized with SpriteKit display link | DiveComposer.swift:311-314 |
| L9 | Starter Curtain uses wrong param key | FirstLaunchManager.swift:116-120 |
| L10 | Keyboard height hardcoded at 64pt | ReefTab.swift:266 |
| L11 | Dive degradation HUD frozen | DiveTab.swift:501-503 |
| L12 | GameCoordinator events not routed from ReefTab | ReefTab.swift:207-230 |
| L13 | ReefTheme 5 themes visually indistinguishable | ReefTheme.swift:14-44 |
| L14 | DatabaseManager silently swallows init failure | DatabaseManager.swift:27-30 |
| L15 | 5/8 biome types never detected | BiomeDetector.swift:175-179 |

---

## TIER 2: POST-LAUNCH (18 items)

### Musical
- DiveComposer voice profile lookup O(n^2) per tick
- Tension note uses tritone at medium intensity
- processBlock clamp +/-4.0 too high (should be 1.5)
- Harmonic minor falls back to natural minor

### UI Polish
- Collection uses errorRed for processor category color
- Ceremony badge same gold for all rarities
- Param panel header crams 6 elements in one row
- Spacing scale missing 10pt and 32pt tokens
- ReefScene non-deterministic speckles

### Architecture
- Sources/Audio/ contains 10+ non-audio game systems
- GeneticManager stores genomes in UserDefaults
- 6+ singleton managers block iPad portability
- categoryColors duplicated between ReefScene and DesignTokens
- SeasonalEventManager bypasses PersistenceCoordinator

### Accessibility
- ReduceMotion not checked beyond ChestCeremony
- Dynamic Type overflow in param panel at +3 accessibility size
- Undiscovered specimens no VoiceOver label
- Ceremony skip misleading for VoiceOver users

---

## Ghost Council — New Findings (Not in LVL1)

| Ghost | Finding | Severity |
|-------|---------|----------|
| Tajiri | KeyboardScale has no minor pentatonic — always major | HIGH |
| Mizuguchi | DiveComposer tick unsynchronized with SpriteKit display link | HIGH |
| Miyamoto | Arp timer not invalidated on app background — stuck notes | HIGH |
| Yu Suzuki | blendTraits has no hybrid subtype resolution | HIGH |
| Bob Moog | noise-pink src1Type disagrees across 3 files (5, 3, 5) | HIGH |
| Dave Smith | OSC namespace has no version envelope | MEDIUM |
| Kakehashi | Starter Curtain params diverge from catalog defaults | MEDIUM |
| Wendy Carlos | Arp velocity locked at 0.7, bypasses 3D Touch | HIGH |

---

## TIDEsigns Verdict

| Member | Status | Key Issue |
|--------|--------|-----------|
| Marina | HOLD | Onboarding silent, Dive story invisible, naming 3-way incoherence |
| Drift | HOLD | Chord mode lies, arp stuck notes on background, keyboard 64pt |
| Coral | HOLD | Themes identical, contrast failures, param density |
| Haven | HOLD | VoiceOver non-functional on reef + keyboard |
| Lattice | HOLD | Directory mislabeling, dual energy keys, singletons block iPad |
| Forge | HOLD | Voice count stub, SpectralCapture UAF, keyboard bypasses engine |
| Sheen | HOLD | Energy collision, biomes stubbed, import no rollback |

**Verdict: FIX LIST** — Fix Tier 0 + Tier 1, then re-run Phase 5 (polish only).

---

## Feature Junkies — Chemist's 5 Sculpted Proposals

All 5 voted MUST HAVE by the panel:

1. **Living Chord Memory** — Specimens remember chords from play; breed vocabulary
2. **Mutation Market** — Shareable param tweaks as 12-char codes (replaces NFC)
3. **Specimen Signature Dive** — Per-creature 8-bar solo at peak section
4. **Reef as Multi-Track Stage** — 16-ch MIDI + drone/loop/thru performance modes
5. **Link-Synced Adaptive Dive** — Bar-based timing, Link tempo override

---

## Phantom Circuit Post-Mortem

The bridge (`ObrixBridge.mm`) is production-grade real-time audio. Everything downstream
fails at the seams — three param tables that disagree, a keyboard that ignores its own
scale property, an arp that hardcodes velocity after implementing 3D Touch sensing.

**One More Thing**: The Dive's zone-based harmonic grammar is genuinely musical. Fix the
7th gate, sync the tick to SpriteKit, amplify specimen influence from +/-0.04 to +/-0.4,
and the Dive could be the feature that makes this app go viral. The Specimen Signature
Dive proposal is the TikTok moment. Build that.

**Atelier**: Architecture permits the future in places (DesignTokens, GRDB, GameCoordinator)
and forecloses it in others (singletons, Audio/ mislabeling, god-object ReefStore). The iPad
port will require un-singletonizing 6+ managers. Start now.

---

## Recommended Sprint Plan

### Sprint A (Week 1): Ship Blocker Surgery
- S1: Fix chord mode (derive from scale intervals)
- S3: Implement activeVoiceCount
- S4: Add toast on .xoreef import failure
- S5: Gate 7th on harmonicComplexity
- S8: Rename energy UserDefaults key
- S9: Route keyboard through AudioEngineManager
- S11: Wire ReefAccessibilityOverlay tap handler
- S12: Fix contrast values

### Sprint B (Week 2): Instrument + Accessibility
- S2: Add sound to onboarding
- S6: Increase keyboard height + expand hit zones
- S7: Fix SpectralCapture FFT lifecycle
- S10: Implement PlayKeyboard accessibilityElements
- L3: Invalidate arp on background
- L4: Arp velocity from initial touch force
- L9: Use SpecimenCatalog defaults for starter Curtain

### Sprint C (Week 3): Musical Correctness
- L1: Design hybrid subtype model for breeding
- L2: Single authoritative parameter table
- L5-L7: Add minor pentatonic, Lydian, fix mappings
- L8: Sync DiveComposer tick to display link
- L10: Draggable keyboard height
- L11: Wire dive degradation HUD
- L12: Route GameCoordinator events
- L13: Differentiate reef themes

### Sprint D (Week 4): Quality + Feature Start
- L14-L15: DB init error handling, biome detection
- Tier 2 items (top priority first)
- Begin Chemist Proposal 1 (Living Chord Memory)

**Target after Sprint C**: 8.0/10 composite, TIDEsigns re-audit for SHIP verdict.

---

## Sisters Process Notes

- **Model routing**: Correct. Haiku for file reads, Sonnet for audits, Opus for synthesis.
- **Waste identified**: Ghost Council re-read files already summarized in LVL1. Pre-digest findings with inline code snippets next time.
- **Downgrade opportunity**: None found. Task complexity matched model tiers.
- **Smoothness Index**: SANDED (comprehensive, minimal rework needed)
- **Next run improvement**: Output structured JSON findings for automatic deduplication.
