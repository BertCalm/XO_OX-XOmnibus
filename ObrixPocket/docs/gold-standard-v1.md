# OBRIX Pocket V1 — The Gold Standard
## What Should Exist. What Should Be Perfect.
### 2026-04-05

---

## The Five Tests

Every system is measured against one question. If the answer is "no," the system fails.

| Test | Question | Current |
|------|----------|---------|
| **Electroplankton** | Does the first touch make music? | No — 4 silent slides |
| **Moog** | Does every parameter change sound different? | No — genetics inaudible, swing/contour unwired |
| **Pokémon** | Is the core loop discoverable through play? | No — wiring is hidden, Dive is gated |
| **Ship** | Will App Store accept and 100K users survive? | No — 3 background mode violations |
| **TIDEsigns** | Would every Tide complete their flow? | No — VoiceOver broken on both primary surfaces |

---

## I. THE IDEAL FILE STRUCTURE

### What to DELETE (vapor — compile but do nothing)

| File | Reason | Lines Recovered |
|------|--------|----------------|
| `ReefAmbientManager.swift` | Duplicate of ReefHeartbeat, hardcodes C pentatonic | ~93 |
| `CommunityEventManager.swift` | Fakes 100 players, no server, App Store risk | ~400 |
| `SpecimenFusion.swift` | Never called, no UI path | ~200 |
| `MacroControl.swift` (MacroPreset section) | savePreset/loadPreset never called | ~80 |
| `PerformanceRecorder.swift` | export methods never consumed | ~180 |
| `ReefSnapshot.swift` | Overlaps with ReefPreset, neither canonical | ~150 |
| **Total** | | **~1,100 lines** |

### What to MOVE (not audio, currently in Sources/Audio/)

```
Sources/Audio/ → Sources/Game/
├── AchievementSystem.swift    → Game/Progression/
├── BadgeManager.swift         → Game/Progression/
├── MasteryManager.swift       → Game/Progression/
├── LoreSystem.swift           → Game/Narrative/
├── NarrativeArcManager.swift  → Game/Narrative/
├── SeasonalEventManager.swift → Game/Seasons/
├── ReefEnergyManager.swift    → Game/Economy/
├── CollectionTracker.swift    → Game/Progression/
├── StreakManager.swift         → Game/Progression/
├── NotificationManager.swift  → Game/Lifecycle/
├── CookbookManager.swift      → Game/Breeding/
├── DailyChallengeManager.swift → Game/Challenges/
├── WeeklyChallengeManager.swift → Game/Challenges/
├── SpecimenGifting.swift      → Game/Social/
├── TradePostManager.swift     → Game/Social/
├── ReefVisitor.swift          → Game/Social/
├── RandomEncounterManager.swift → Game/Events/
├── LegendarySpecimenManager.swift → Game/Events/
├── DiveHistoryManager.swift   → Game/Dive/
├── ReefStatsTracker.swift     → Game/Analytics/
├── JournalManager.swift       → Game/Narrative/
├── SpecimenJournal.swift      → Game/Narrative/
```

### What to QUARANTINE (Phase 2 — remove from compile target)

```
Phase2/
├── CollaborativeDive.swift     (MultipeerConnectivity — App Store risk if compiled)
├── SpecimenGifting.swift       (requires discovery mechanism)
├── TradePostManager.swift      (executeTrade returns to void)
├── ReefVisitor.swift           (manual file exchange)
├── LegendarySpecimenManager.swift (endgame — no players will reach)
├── MasteryManager.swift        (prestige — depends on endgame)
├── RandomEncounterManager.swift (effects never consumed)
```

### What to ADD (doesn't exist yet)

| File | Purpose | Priority |
|------|---------|----------|
| `Sources/Audio/ParameterRegistry.swift` | SINGLE source of truth for ALL param IDs — eliminates the 3-file disagreement | CRITICAL |
| `Sources/Audio/IO/OSCReceiver.swift` | Bidirectional OSC (Chemist Proposal #4) | Week 4+ |
| `Sources/UI/Reef/WiringTutorialOverlay.swift` | Animated drag arrow for first-time wire discovery | Week 3 |
| `Tests/ParameterRegistryTests.swift` | Compile-time assertion: every specimen param ID maps to a real engine param | CRITICAL |
| `Tests/AudioPipelineTests.swift` | Assert: param → mapping → bridge → engine round-trip | HIGH |

### The Ideal Structure (V1 Ship)

```
Sources/
├── App/                          # 4 files
│   ├── ObrixPocketApp.swift      # DI container + lifecycle
│   ├── ContentView.swift         # Tab shell
│   ├── AppConstants.swift        # Magic numbers
│   └── GameCoordinator.swift     # Event bus (MOVED from Audio/)
│
├── Audio/                        # ONLY audio/DSP — ~25 files
│   ├── Engine/
│   │   ├── ObrixSynthEngine.swift
│   │   ├── AudioEngineManager.swift
│   │   ├── ParameterRegistry.swift   ★ NEW
│   │   └── AudioExporter.swift
│   ├── Bridge/
│   │   ├── ObrixBridge.h/.mm
│   │   └── JUCE modules
│   ├── Music/
│   │   ├── HarmonicContext.swift
│   │   ├── DiveComposer.swift
│   │   ├── ArrangementEngine.swift
│   │   ├── ReefHeartbeat.swift       (THE ONLY ambient system)
│   │   └── ReefPerformanceSurface.swift
│   ├── Genetics/
│   │   ├── HarmonicDNA.swift
│   │   ├── VoiceProfile.swift
│   │   ├── MusicalRole.swift
│   │   └── GeneticSystem.swift
│   ├── Expression/
│   │   ├── PlayerExpression.swift
│   │   ├── SoundMemory.swift
│   │   └── SpecimenAudioBridge.swift
│   └── IO/
│       ├── MIDIInputManager.swift
│       ├── MIDIOutputManager.swift
│       └── OSCSender.swift
│
├── Game/                         # Game systems — ~20 files
│   ├── Core/
│   │   ├── ReefStore.swift
│   │   └── ReefEnergyManager.swift   (SINGLE energy system)
│   ├── Catch/
│   │   ├── SpawnManager.swift
│   │   ├── SpecimenCatalog.swift
│   │   ├── SpecimenHashGenerator.swift
│   │   ├── SpectralCapture.swift
│   │   ├── BiomeDetector.swift
│   │   └── WeatherService.swift
│   ├── Breeding/
│   │   ├── BreedingSystem.swift
│   │   ├── CouplingAffinity.swift
│   │   └── EvolutionCatalog.swift
│   ├── Progression/
│   │   ├── SpecimenLeveling.swift
│   │   ├── SpecimenAging.swift
│   │   ├── AchievementSystem.swift   (WIRE the 37 unwired achievements or trim to 8)
│   │   ├── DiveScoring.swift
│   │   └── DiveHistoryManager.swift  (WIRE record() call from DiveTab)
│   ├── Narrative/
│   │   ├── NarrativeArcManager.swift
│   │   └── JournalManager.swift
│   └── Economy/
│       └── FirstLaunchManager.swift  (RENAME energy key)
│
├── UI/                           # ~30 files
│   ├── Design/
│   │   ├── DesignTokens.swift    (SINGLE color source — delete ReefScene duplicate)
│   │   └── ReefTheme.swift       (5 ACTUALLY DISTINCT themes)
│   ├── Onboarding/
│   │   └── OnboardingView.swift  (SOUND-FIRST, learn-by-doing)
│   ├── Reef/
│   │   ├── ReefTab.swift
│   │   ├── ReefScene.swift
│   │   ├── PlayKeyboard.swift    (120pt, scale-aware chords, VoiceOver)
│   │   ├── ReefHeaderView.swift
│   │   ├── SpecimenParamPanel.swift
│   │   └── WiringTutorialOverlay.swift  ★ NEW
│   ├── Catch/
│   │   ├── CatchTab.swift
│   │   ├── MusicCatchFlow.swift
│   │   ├── ChestCeremony.swift
│   │   └── CreatureCard.swift
│   ├── Dive/
│   │   ├── DiveTab.swift
│   │   └── DiveScene.swift
│   ├── Collection/
│   │   ├── CollectionTab.swift
│   │   ├── MicroscopeView.swift
│   │   └── CompareView.swift
│   └── Shared/
│       ├── AccessibilityHelpers.swift
│       ├── AudioSettingsView.swift  (ADD settings gear to tab bar)
│       └── ShareSheet.swift
│
├── Persistence/                  # 4 files (unchanged)
│   ├── DatabaseManager.swift
│   ├── ReefPersistence.swift
│   ├── PersistenceCoordinator.swift
│   └── SpecimenRecord.swift
│
└── Utilities/                    # 5 files
    ├── TickScheduler.swift
    ├── HapticEngine.swift
    ├── MotionController.swift
    ├── MetronomeManager.swift
    └── LoopRecorder.swift
```

**V1 Ship: ~84 files** (down from 136). Every file does one thing. No vapor.

---

## II. WHAT PERFECT LOOKS LIKE — PER SYSTEM

### 1. The First 60 Seconds (Electroplankton Test)

**PERFECT:**
- App opens. Ambient reef sound plays immediately (ReefHeartbeat, not silence).
- Starter Sawfin arrives with a 1.5-second mini ChestCeremony: name, sprite, one auto-played note.
- The sound is warm, filtered, beautiful. Curated preset — not engine defaults.
- Keyboard appears: 120pt tall, labeled note names, pentatonic scale pre-selected.
- Hint: pulsing glow on Sawfin sprite, not 10pt text at 25% opacity.
- First tap on keyboard plays a note. The sprite reacts (pulse animation synced to noteOn).
- After first note: Curtain (filter) auto-spawns with its own 1-second ceremony.
- Animated arrow overlay: "Drag from Sawfin to Curtain."
- First wire: Living Chord plays for 3 seconds. Text: "Your reef has a voice."
- Music Catch button: labeled "SONG", tooltip on first view.
- Dive unlocks at 2 specimens, not 4.
- Total permission asks before first sound: ZERO.

**GAP:** Currently: 4 silent slides → 64pt keyboard → 10pt hint → no ceremony for starter → Dive locked at 4.

### 2. The Sound (Moog Test)

**PERFECT:**
- `ParameterRegistry.swift` is the SINGLE source of truth. Every param ID exists exactly once.
- Every specimen's `defaultParams` maps through `parameterMapping` to real engine params. A unit test validates this at build time.
- `configureFromProfile()` is called in `DiveComposer.configure()`. Genetics shape the Dive's key, scale, and chord progressions.
- VoiceProfile.swing is applied: even-subdivision notes are delayed by `swing * subdivisionDuration * 0.33`.
- VoiceProfile.contour is applied: phrase position biases note selection direction.
- Specimen influence delta is ±0.3 (not ±0.04). A Lv.10 specimen audibly changes the Dive.
- ArrangementEngine.tempoMultiplier is wired: intro at 0.9x, peak at 1.05x.
- ReefHeartbeat is the ONLY ambient system. Melodic memory keys by subtypeID.
- HarmonicContext 7th is gated: `if harmonicComplexity > 0.5 { degrees.append(root + 6) }`.
- Tension notes are context-aware: leading tone near resolution, not unconditional tritone.
- Chord mode derives intervals from `scale.intervals` — minor scale → minor chord.
- Arp velocity uses initial touch force, not hardcoded 0.7.
- Breeding produces audibly different offspring (genetics → audio is wired).

**GAP:** Currently: swing/contour unwired, genetics inaudible, ±0.04 influence, unconditional 7th, hardcoded arp velocity, ReefAmbientManager plays C pentatonic.

### 3. The Game (Pokémon Test)

**PERFECT:**
- Catch: Location permission asked WITH context ("OBRIX uses your location to place creatures near you").
- Catch: Reef Proximity defaults to device timezone capital, not 0°N 0°E.
- Wire: Discoverable through animated overlay, not 10pt text.
- Dive: Unlocks at 2 specimens. Each specimen gets a visible "voice" indicator during dive.
- Dive: Degradation HUD updates live (not frozen).
- Collection: Discoverable compare mode with labeled toggle.
- Energy: ONE system, not two with shared keys. `ReefEnergyManager` only.
- Achievements: 8-12 achievable achievements, not 40 where 37 are unwired.
- Wiring events route through GameCoordinator → NarrativeArc → Lore.
- Journal records automatically (not dependent on unwired callers).
- Every game event produces a toast, haptic, or visual feedback. No silent operations.

**GAP:** Currently: Dive gated at 4, degradation frozen, two energy systems, 37/40 achievements unwired, GameCoordinator events not routed from ReefTab.

### 4. The Architecture (Ship Test)

**PERFECT:**
- App Store: Only background modes actually used (audio only for V1).
- No frameworks linked that aren't used (remove CoreBluetooth, gate MultipeerConnectivity).
- All entitlements match capabilities: WeatherKit yes, push NO (remove remote-notification).
- Memory warning handler sheds SpriteKit textures, not just 10KB param cache.
- Init order: ALL dependencies wired in `init()`, not in `.onAppear`. No nil-reference windows.
- SeasonalEventManager actually wired to GameCoordinator.
- LazyInitManager.runDeferredInit() actually called.
- ReefStore is a model, not a god object. Persistence extracted to ReefPersistence (extension is fine, but clearly bounded).
- Zero singletons needed at app scope — use dependency injection via @EnvironmentObject.
- Sources/Audio/ contains ONLY audio code. Sources/Game/ contains game systems.
- A new developer can find where notes trigger in 30 minutes, not 2 hours.

**GAP:** Currently: 3 rejected background modes, 18 singletons, nil-reference window in onAppear, Audio/ directory lies about its contents.

### 5. The Surface (TIDEsigns Test)

**PERFECT:**
- ReefTheme: 5 visually distinct themes. Ocean = deep blue, Volcanic = amber-rust, Arctic = pale ice-blue, Deep = true black with bioluminescent accents, Coral = warm pink-orange. Luminosity delta ≥ 20%.
- Theme extends to SwiftUI chrome, not just SpriteKit grid.
- All text meets WCAG AA (4.5:1 contrast minimum). No text below 11pt.
- Keyboard: 120pt height, whiteKeyColor #2A2A2E (visible), note labels on white keys.
- PlayKeyboard: full VoiceOver — one `accessibilityElement` per key, `accessibilityLabel: "C4"`.
- ReefScene: VoiceOver overlay taps are functional (onTapGesture wired to noteOn).
- ReduceMotion respected on ALL animations, not just ChestCeremony.
- Dynamic Type: param panel reflows at +3 accessibility size, not overflows.
- Category shapes (circle/square/diamond/hexagon) used alongside color EVERYWHERE, not just ReefScene.
- DesignTokens category colors used from DesignTokens, not duplicated in ReefScene.
- Onboarding: no jargon. "Sources make sound. Processors shape it. Effects add space."

**GAP:** Currently: themes identical, contrast failures, 64pt keyboard, VoiceOver broken on reef + keyboard, ReduceMotion checked in 1 of 30+ animations.

---

## III. THE PARAMETER TRUTH

**This is the single most important technical question in the codebase:**

The issue-triage scan found that 18 files use `obrix_flt1Cutoff` (specimen namespace) but the engine only knows `obrix_proc1Cutoff` (JUCE APVTS namespace). The phantom sniff found that `obrix_flt1Cutoff` IS consistently used and `obrix_proc1Cutoff` is the canonical engine name.

**The answer is: AudioEngineManager.parameterMapping translates between them.**

```
Specimen writes: obrix_flt1Cutoff (value 0.65)
  → AudioEngineManager.collectMappedParams() looks up mapping
  → Mapping: { engineParam: "obrix_proc1Cutoff", scale: { 20 + v * 19980 } }
  → Bridge receives: obrix_proc1Cutoff = 13007 Hz
```

**The test for perfection:**
Every code path that sends a parameter to the engine MUST go through AudioEngineManager.parameterMapping. Any path that calls `ObrixBridge.shared()?.setParameter()` directly with a specimen-namespace key will silently no-op.

**What must be verified:**
1. Does MacroControl.swift go through AudioEngineManager or direct to bridge?
2. Does ReefAcoustics.getObrixParameters() go through AudioEngineManager or direct?
3. Does SpecimenAudioBridge stack modifiers in specimen namespace (correct) or engine namespace?
4. Does DiveComposer apply bridge modifiers through AudioEngineManager?

If ANY of these bypass AudioEngineManager, they are broken. If they all go through it, the 18-file "ghost param ID" finding is a FALSE ALARM.

**Create `ParameterRegistry.swift` regardless** — it should be the compile-time-validated single source of truth.

---

## IV. THE SOUND TRUTH — What Makes It Music

The Sound Executioner found that the generative music system has correct ARCHITECTURE but broken WIRING. Here is what "perfect" sounds like:

### The 60-Second Dive (Perfect Version)

| Time | Section | What Happens |
|------|---------|-------------|
| 0-8s | Intro | One voice enters at specimen's preferred octave. Tempo 0.9x (99 BPM). Scale from genetic HarmonicProfile. Swing from VoiceProfile. |
| 8-20s | Build | Voices stagger in by role priority. Bass first, then melody, then harmony. Each voice uses its contour shape. Tempo rises to 1.0x. |
| 20-44s | Peak | All voices active. Density at maximum. Lv.7+ specimens get their 8-bar signature solo. Player position matters — X selects chord tone, Y modulates filter. Specimen influence at ±0.3 makes each reef sound distinct. |
| 44-52s | Outro | Voices exit in reverse order. Tempo drops to 0.95x. Tension resolves to tonic. |
| 52-60s | Fade | Final voice sustains. Reverb tail. Score appears. |

**What changes from current:**
1. `configureFromProfile()` call → genetics shape scale/key
2. Swing applied → rhythm has groove, not grid
3. Contour applied → melodies arc, not orbit
4. tempoMultiplier wired → sections breathe
5. Influence amplified → specimens matter
6. 7th gated → chords match zone character
7. Signature solos at peak → each creature has a musical moment

### The Reef Ambient (Perfect Version)

- ReefHeartbeat is the sole ambient system
- lastNotes keyed by subtypeID → melodic memory works
- Scale derived from reef's dominant harmonic profile, not hardcoded C pentatonic
- Mood drifts through the full progression, not random 30% chance per 10 phrases
- Population count affects density (already partially implemented)
- Background idle for 3+ minutes → ReefHeartbeat plays a slow evolving piece

---

## V. CONFIDENCE INTERVALS

### Phantom Sniff Results (audit accuracy)

| Finding | Status | Impact |
|---------|--------|--------|
| SpectralCapture FFT race | PHANTOM — nil-first IS the fix | Remove from ship blockers |
| StreakManager rewards discarded | PHANTOM — correctly routed via ReefIndicatorBar | Remove from findings |
| Starter Curtain wrong param key | PHANTOM — `obrix_flt1Cutoff` IS correct | Remove from findings |
| PlayKeyboard bypasses AudioEngineManager | PARTIAL — applyCachedParams IS called, direct bridge noteOn is real | Downgrade from CRITICAL to HIGH |
| tappedZoneIndex race | PARTIAL — logic gap, not thread race | Reclassify as logic bug |

**Corrected Ship Blocker Count: 10** (was 12, minus SpectralCapture race and Curtain param key)

**Corrected Composite Score: 3.8/10** (was 3.2, adjusted for phantom removals)

---

## VI. THE FOUR-WEEK SPRINT (Revised)

### Week 1: Make It Sound Like Music + Fix App Store Blockers
- Wire `configureFromProfile()` in DiveComposer.configure()
- Implement VoiceProfile swing in tick()
- Read VoiceProfile contour in note selection
- Gate 7th on harmonicComplexity
- Fix ReefHeartbeat lastNotes keying
- Delete ReefAmbientManager
- Wire ArrangementEngine.tempoMultiplier
- Amplify specimen influence to ±0.3
- Remove unused background modes (fetch, processing, remote-notification, background location)
- Remove CoreBluetooth framework + usage descriptions
- Quarantine CollaborativeDive.swift from compile target

### Week 2: Make It Feel Like an Instrument
- Keyboard height: 120pt minimum
- Chord mode: derive from scale intervals
- Arp: use initial touch force as velocity
- Key contrast: raise whiteKeyColor
- Fix activeVoiceCount stub
- Route PlayKeyboard through AudioEngineManager
- Fix shared energy UserDefaults key
- Wire SeasonalEventManager in ObrixPocketApp
- Call LazyInitManager.runDeferredInit()

### Week 3: Make the First 60 Seconds Magical
- Sound-first onboarding (starter ceremony + auto-play)
- Label Music Catch button ("SONG")
- Animate second specimen arrival
- Add wiring tutorial overlay
- Drop Dive gate from 4 to 2 specimens
- Curate starter Sawfin preset
- Fix Reef Proximity 0,0 fallback
- Implement PlayKeyboard VoiceOver
- Wire ReefAccessibilityOverlay taps
- Fix all contrast values

### Week 4: Clean the Marble
- Move game systems out of Sources/Audio/
- Create ParameterRegistry.swift (single source of truth)
- Add build-time param validation test
- Quarantine Phase 2 files
- Wire GameCoordinator events from ReefTab
- Trim achievements to 8-12 achievable ones
- Wire DiveHistoryManager.record() from DiveTab
- Differentiate reef themes
- Apply ReduceMotion to all animations

**After Week 4: TIDEsigns re-audit for SHIP verdict. Target: 8.5/10.**
