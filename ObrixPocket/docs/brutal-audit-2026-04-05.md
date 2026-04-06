# OBRIX Pocket — The Brutal Audit
## 2026-04-05 | The Truth That Hurts Enough to Heal

---

## The Three Sentences

1. **45% of your codebase is vapor.** 25 of 56 audited files are orphans, facades, stubs, or broken wiring. ~8,000–12,000 lines of Swift that compile, do nothing, and give the illusion of a finished product.

2. **Your generative music engine is a well-organized random number generator.** The harmonic architecture is correct. The genetic DNA system is never wired to audio. The swing values are never applied. The contour shapes are never read. The specimen influence delta is ±0.04 on a 0-1 scale — inaudible. A Lv.10 bred specimen sounds identical to a Lv.1 freshly caught one.

3. **75% of casual users will delete this app within 10 minutes.** Silent onboarding, jargon slides, a 64pt keyboard in 5% contrast, the best feature (Dive) locked behind a 4-specimen gate, and the "tell a friend" moment (Music Catch) hidden behind an unlabeled button.

---

## I. THE SOUND EXECUTIONER'S VERDICT

### What Claims to Be Music but Produces Noise

| System | Claim | Reality |
|--------|-------|---------|
| DiveComposer melody | "Coherent melodic contour" | Orbits one note. No phrase memory. Chromatic neighbors at complexity>0.5 create chromatic crawl |
| Specimen influence | "Your specimen shapes the Dive" | ±0.04 on 0-1 scale. Inaudible. Warmth shifts note selection by 15% of chord range |
| VoiceProfile swing | "Foamspray: 0.6 swing" | Never applied. All specimens tick on identical grid |
| VoiceProfile contour | "Bellcrab: arch contour" | Never read. All specimens produce identical melodic shapes |
| HarmonicDNA genetics | "Bred offspring has Lydian preference" | configureFromProfile() is never called by DiveComposer. Genetics produce zero audio |
| ReefAmbientManager | "Living reef soundscape" | Random C pentatonic notes, every reef, always. No harmonic context, no specimen influence |
| ArrangementEngine tempo | "Intro slower, peak urgent" | tempoMultiplier computed, never used. BPM fixed at 110 for entire dive |
| ArrangementEngine density | "Section density targets" | densityTarget computed, never read by DiveComposer |
| Zone progressions | "Handcrafted chord voicings" | Dead code. DiveComposer builds chords from HarmonicContext, not from the progressions dict |
| ReefHeartbeat memory | "Melodic memory per specimen" | lastNotes indexed by incrementing counter, never matches. Stateless random selection |
| tappedZoneIndex | "Call-and-response on tap" | Set at 60Hz, cleared same frame. DiveComposer reads at 7Hz. Never sees the value |
| Tension notes | "Harmonic tension near obstacles" | Medium intensity = unconditional tritone. No resolution mechanism. Car alarm in a shopping mall |

### What Actually Works

- HarmonicContext chord construction (tertian harmony from scale degrees) — correct
- DiveComposer zone structure (4 zones with appropriate harmonic character) — good design
- VoiceProfile per-specimen personality data — well-designed, just never consumed
- ReefPerformanceSurface role-based note selection — the only place chords are played as chords
- ArrangementEngine section scheduling and voice entry staggering — correctly sequenced

### The One-Line Fix That Changes Everything

```swift
// In DiveComposer.configure(), add:
harmonicContext.configureFromProfile(harmonicProfileStore.blendedProfile(for: specimenIDs))
```

This single call wires the entire genetic harmonic system — preferred scale, modal character, interval affinity, tonal gravity — into live audio. The infrastructure is already built. It is already tested. It produces correct output. It is simply never called.

---

## II. THE STUB HUNTER'S VERDICT

### Codebase Reality: 55% Real, 45% Vapor

| Classification | Count | % | Lines (est.) |
|----------------|-------|---|-------------|
| REAL (functional, wired) | 31 | 55% | ~20,000 |
| WIRED BUT BROKEN | 10 | 18% | ~4,000 |
| ORPHAN (dead code) | 7 | 13% | ~3,000 |
| FACADE (looks complete, does nothing) | 7 | 13% | ~4,000 |
| STUB | 1 | 2% | ~500 |

### The Ghosts in the Machine

**The community is a lie.** CommunityEventManager simulates 100 fake players with a deterministic LCG seeded from the calendar date. Events always succeed. There is no server, no API, no real players.

**The achievement system is 92.5% unwired.** 40 achievements exist. 3 have confirmed callers from GameCoordinator. The other 37 wait for triggers that never fire.

**The economy is unrouted.** StreakManager calculates XP rewards and discards them. RandomEncounterManager computes multipliers nothing reads. TradePostManager creates specimens that vanish into void.

**The environmental systems are decorative.** ReefWeather computes filter/LFO/tempo modifiers. ReefChemistry computes pairwise bond effects. SpecimenAging computes life-stage shifts. None of these reach the audio engine.

**The social features require manual file exchange.** ReefVisitor, SpecimenGifting, CollaborativeDive (687 lines of MultipeerConnectivity) — all require players to find each other outside the app and share files via AirDrop.

**The endgame doesn't exist.** MasteryManager (prestige) and LegendarySpecimenManager require months of daily play to reach. Trigger hooks are not wired. No player will ever see this content.

### What This Means

You have one game inside a blueprint of three. The core loop — catch, wire, play, dive — is real. Everything layered on top (community, social, achievements, weather, chemistry, aging, mastery, lore) is a design document expressed in Swift. It compiles. It does nothing.

---

## III. THE UX DEMOLISHER'S VERDICT

### The First 10 Minutes: Kill Rate by Minute

| Minute | Event | Kill Risk (cumulative) |
|--------|-------|----------------------|
| 0-1 | Silent onboarding, jargon slides ("synth module", "signal chain") | 15% |
| 1-3 | 64pt keyboard, 5% contrast, no note labels, no explanation of grid | 35% |
| 3-5 | Location permission with no context, Reef Proximity defaults to 0°N 0°E | 60% |
| 5-10 | Dive locked (need 4 specimens), Music Catch button unlabeled, wiring undiscoverable | 75% |

### Competitor Comparison

| Metric | OBRIX | Pokémon GO | Moog D | GarageBand |
|--------|-------|------------|--------|------------|
| Time to first sound | 20-30 sec | N/A (game) | <5 sec | <10 sec |
| First sound curated? | No | N/A | Yes (world-class) | Yes (grand piano) |
| Core loop clear in 10 min | No | Yes | Yes | Yes |
| "Tell a friend" moment | Hidden | AR camera | "I played a Moog" | "I made a song" |
| Permission asks before value | Location (no context) | Location (shown why) | None | None |
| Best feature locked? | Yes (Dive: 4 specimens) | No | No | No |

### What a Professional Would Say

"It's like... a synthesizer game? I think you collect creatures and they make sounds? I didn't really understand what I was supposed to do. The keyboard was really small."

### The Single Word

**Latent.** Everything great about this app is present but inert. The antidote is not more features. It is earlier music.

---

## IV. THE ARCHITECTURE SURGEON'S VERDICT

### App Store: WILL BE REJECTED

Three of five declared background modes will trigger rejection:
1. `remote-notification` — no `aps-environment` entitlement
2. `location` (background) — missing `NSLocationAlwaysAndWhenInUseUsageDescription`
3. `fetch` + `processing` — no BGTaskScheduler registration in code

CoreBluetooth linked with usage descriptions but zero CBCentralManager usage.

### Memory: COSMETIC WARNING HANDLER

- Steady state: ~100-200 MB (SpriteKit scenes dominant)
- iPhone SE 2nd gen (3GB): jetsam kill risk when both scenes + SpectralCapture active
- `handleMemoryWarning()` frees ~10 KB of param cache on a 100-200 MB footprint

### Threading: 2 CONFIRMED RACES

1. Post-init wiring race: `audioEngine.reefStoreRef` set in `.onAppear`, JUCE thread starts same block. Window exists where audio thread reads nil.
2. Load-during-mutation: `addSpecimen()` during async `load()` completion — generation counter doesn't protect this path.

### Init Graph: 9 @StateObject + 18 SINGLETONS

- `SeasonalEventManager` reference in GameCoordinator is NEVER SET — nil at runtime. Seasonal system is wired but non-functional.
- `LazyInitManager.runDeferredInit()` is never called from the app entry point.
- Post-init wiring in `.onAppear` creates a nil-reference window that will generate spurious bugs.

### Bus Factor: HIGH

A new developer will be productively confused for 2 days minimum. The Sources/Audio/ directory contains AchievementSystem, LoreSystem, NarrativeArcManager, SeasonalEventManager — none of which are audio. The dual singleton/StateObject graph is undocumented. The wired-post-init pattern is a trap.

---

## V. THE PATH TO DAVID

### Week 0: Stop the Bleeding (before anything else)

1. Remove background modes you don't use (fetch, processing, remote-notification, background location)
2. Remove CoreBluetooth framework and usage descriptions
3. Fix the shared `"obrix_last_energy_date"` UserDefaults key collision
4. Wire `seasonalEventManager` in ObrixPocketApp
5. Call `LazyInitManager.runDeferredInit()` from app entry point

### Week 1: Make It Sound Like Music

6. Wire `harmonicContext.configureFromProfile()` in DiveComposer — genetics become audible
7. Implement VoiceProfile swing in tick() — even-subdivision delay
8. Read VoiceProfile contour in note selection
9. Fix ReefHeartbeat lastNotes to key by subtypeID
10. Delete ReefAmbientManager (use ReefHeartbeat exclusively)
11. Fix tappedZoneIndex race (consume on next tick, not clear every frame)
12. Gate the 7th on harmonicComplexity
13. Fix tensionNote to be context-aware (leading tone, not tritone)
14. Wire ArrangementEngine.tempoMultiplier into tick scheduler
15. Amplify specimen influence from ±0.04 to ±0.3

### Week 2: Make It Feel Like an Instrument

16. Keyboard height: 120pt minimum, draggable resize
17. Chord mode: derive intervals from active scale
18. Arp: pass initial touch force as base velocity
19. Key contrast: raise whiteKeyColor to #2A2A2E
20. Fix activeVoiceCount stub
21. Route PlayKeyboard through AudioEngineManager (not direct bridge calls)
22. Implement PlayKeyboard VoiceOver accessibility

### Week 3: Make the First 60 Seconds Magical

23. Replace 4-slide onboarding with sound-first intro (creature arrives with ceremony + auto-plays)
24. Label the Music Catch button ("SONG")
25. Animate second specimen arrival (don't place silently)
26. Add wiring tutorial overlay (animated drag arrow)
27. Drop Dive gate from 4 to 2 specimens
28. Curate starter Sawfin preset for guaranteed beauty
29. Fix Reef Proximity 0,0 fallback to timezone capital

### Week 4: Delete the Vapor

30. Classify all 25 vapor files: DELETE, DEFER, or WIRE
31. Move non-audio files out of Sources/Audio/ into Sources/Game/
32. Wire the 37 unwired achievements (or remove them)
33. Wire ReefWeather + ReefChemistry + SpecimenAging to audio engine (or delete)
34. Remove CommunityEventManager fake-player simulation (or add real server)

### After Week 4: Build the David

With the marble removed, what remains is:
- A real JUCE audio engine with production-grade bridging
- A genetic harmonic system that shapes how creatures sound
- A Monster Rancher DNA mechanic that is genuinely novel
- A zone-based generative music system with correct voice leading
- A creature collection loop with natural retention hooks

That is the David. Everything else was marble.
