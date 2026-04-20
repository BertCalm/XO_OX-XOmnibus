# OBRIX Pocket — Scope & Build Plan
> "The Reef" — a specimen-based collectible synthesizer for iPhone
> Canonical spec as of 2026-03-26

## 1. Product Identity

OBRIX Pocket is a **standalone iPhone app** — not a plugin, not a port.
It is one of three products in the XOceanus platform:

| Platform | Product | Identity | Core Verb |
|----------|---------|----------|-----------|
| iPhone | OBRIX Pocket | The Reef | PLAY + COLLECT |
| iPad | OBRIX Academy | The School | LEARN |
| Desktop | XOceanus | The Ocean | CREATE + PRODUCE |

Each has its own soul. No duplication. The connection between them (Reef Link) is the value.

## 2. Core Concept

OBRIX Pocket is a **collectible roguelike synthesizer**:
- **Collectible**: Wild specimens (synthesis modules) spawn in the world. You catch them.
- **Roguelike**: The Dive — a timed performance challenge with real consequences.
- **Synthesizer**: Your collection of specimens forms a reef that you play as an instrument.

One engine: OBRIX (81 parameters, modular brick synthesis, coral reef metaphor).

## 3. The Specimen System

### 3.1 What Is a Specimen?
A specimen is a synthesis module configuration — one of OBRIX's building blocks (source oscillator, processor, modulator, or effect) with specific parameter values plus spectral DNA from its catch environment.

### 3.2 Specimen Categories
- **Sources (Shells)**: PolyBLEP oscillators, noise generators
- **Processors (Coral)**: Filters, waveshapers, feedback paths
- **Modulators (Currents)**: Envelopes, LFOs, velocity/aftertouch maps
- **Effects (Tide Pools)**: Delay, chorus, reverb

### 3.3 Specimen Rarity
- **Common**: Spawn everywhere, basic parameter ranges
- **Uncommon**: Spawn from specific conditions (weather, time, milestones)
- **Rare**: Spawn from coupling discovery or community events
- **Legendary**: Deep Specimen rewards from surviving Dives past 1000m

Note: Phantom is a **health state**, not a rarity tier. A Legendary that bleaches is still Legendary-rarity, but in Phantom state. The rarity enum is `Common | Uncommon | Rare | Legendary`.

### 3.4 Specimen Spectral DNA
When caught, each specimen captures an ambient spectral snapshot during the catch encounter (~5 seconds):
- Microphone → 64-float spectral profile (frequency distribution only, no raw audio stored)
- Accelerometer → movement pattern → default LFO character
- Time of day → filter warmth (night = darker)
- Weather API → temperature/humidity/pressure → modulation depth defaults
- Compass heading → stereo field bias

The specimen literally sounds like WHERE and WHEN you caught it.

**Offline fallbacks** (when sensors/APIs are unavailable):
| Input | Always Available? | Offline Fallback |
|-------|:-:|----------------|
| Microphone | Requires permission | Default "Studio" spectral profile. Specimen tagged "Studio Caught." |
| Accelerometer | Yes (no permission needed) | Always works — no fallback needed |
| Time of day | Yes (system clock) | Always works — no fallback needed |
| Weather API | Requires network | Last cached weather if within 6 hours. Otherwise "Fair Weather" default. Specimen tagged "Weather Unknown." |
| Compass | Yes (no permission needed) | Always works — no fallback needed |

When the app foregrounds from background (e.g., via catch notification), the audio session transitions from .playback → .playAndRecord before the spectral capture begins. This transition takes ~50ms and is handled automatically before the catch screen appears.

Offline-caught specimens are fully functional. The "Studio Caught" and "Weather Unknown" tags
appear in the provenance card but do not affect gameplay or rarity. The spectral DNA simply
has neutral values for the unavailable inputs.

### 3.4.1 Spectral Fingerprint
Each specimen's 64-float spectral profile is rendered as a unique **spectral fingerprint** — a
radial waveform glyph (think: circular audio waveform) that serves as the specimen's sonic identity.

Fingerprint appearances:
- **During catch**: Behind the creature, pulsing with ambient sound in real time
- **In the reef grid**: Subtle background texture in each slot (low opacity, behind the creature)
- **In Collection detail card**: Large, interactive — tap regions of the fingerprint to hear those
  frequency bands isolated
- **During Abyssal Dive challenges**: Two fingerprints shown side-by-side for visual spectral
  complementarity assessment (the player can SEE which specimens have low overlap)

Two specimens of the same subtype caught in different environments have obviously different
fingerprints. The fingerprint IS the provenance, visualized. Rendered via SpriteKit custom
shader (radial path from 64 amplitude values, ~20 lines of Metal shader code).

### 3.5 Phantoms (Bleached Specimens)
Health enters DORMANCY if the app is not opened for 7+ days. During dormancy, health decays at 1 point/day. Opening the app pauses dormancy for 7 days (the "weekly check-in" window). Active play further extends the window.

Effective cycle: a specimen goes from full health (100) to Phantom in ~107 days of COMPLETE abandonment (7-day grace + 100-day decay). A player who opens the app once a week never experiences health decay at all.

At 0 health, the specimen **bleaches** — it doesn't die, it TRANSFORMS:
- The pixel creature goes translucent/ghostly
- Synthesis character inverts (bright → dark, rhythmic → sustained, filtered → raw)
- The phantom fills spectral gaps the living specimens leave

Phantom tradeoffs — interesting but gameplay-limited:
- Inverted spectral character (unique sound)
- Coupling-capable with other Phantoms and Commons/Uncommons
- CANNOT enter the Dive (ejected on descent start)
- CANNOT couple with Legendary specimens
- CANNOT be traded (too fragile to transfer)
- CAN be revived (returns with spectral scar — memory of phantom state)
- The tradeoff: a reef with many Phantoms sounds INTERESTING but is DIVE-INELIGIBLE

Revival methods: play it, couple it with a healthy specimen, return to catch biome.
Revived specimens carry a spectral scar — memory of the phantom state.

**Stasis mode**: Store up to 10 specimens in cryo (no decay, no sound). Thaw anytime.

## 4. Spawn System — Fun, Not Barriers

CRITICAL: Location flavors spectral DNA but does NOT gate specimen types.
Multiple overlapping spawn sources ensure everyone gets everything:

| Source | Mechanic | Gets You |
|--------|----------|----------|
| Daily Drift | 1-2 types randomly on map each day, duplicated every ~500m | Common + Uncommon |
| Login Milestones | Day 3, 7, 14, 30, 60, 100 streaks | Guaranteed rarer specimens |
| Performance Rewards | Survive a Dive (3+ min) | Deep Specimens |
| Weather Bonus | Real weather → matching specimens spawn near you | Spectral flavor variety |
| Coupling Discovery | Slot two new specimens together → offspring grows (Phase 4) | Emergent specimens |
| Exploration Bonus | Walk to a new area (500m+ from any catch) → bonus | Rewards movement |
| Time of Day | Nocturnal at night, golden hour at dawn/dusk | Everyone has time |
| Community Events | "Crystal Specimens everywhere this week" (Phase 4) | Global, time-limited |

### 4.1 Accessibility: Reef Proximity
Settings toggle: **Reef Proximity: HOME**
All specimens spawn within 50m of saved home location. Full access, zero travel.
Framed as a reef behavior ("your reef is a homebody"), not a disability accommodation.
Anyone can use it for any reason.

## 5. The Reef — PlaySurface

### 5.1 Visual Layout
4x4 grid (16 slots). Each occupied slot displays:
- Pixel creature (.xogenome) with health-based bioluminescent glow
- Category color ring: blue (Source), red (Processor), green (Modulator), purple (Effect)
- Rarity indicator: border thickness (Common=1px, Uncommon=2px, Rare=3px, Legendary=4px gold)
- Phantom overlay: translucent shimmer effect when bleached
- Coupling lines: animated connections between coupled specimens, pulse rate = coupling depth

**Organic layout**: Slots use pseudo-random offsets (±8px from grid center, deterministic per slot
index via a seeded noise function). Coupling lines render as curved Bézier paths, not straight lines.
The reef subtly breathes — a slow brightness oscillation (0.1Hz) synchronized to the ambient drone.
New specimens grow into position over 2 seconds (SpriteKit ease-out animation). Bleaching transitions
are gradual (3-second color drain). Revival floods color back like dawn (2-second ease-in).

The layout FEELS organic but touch targets remain predictable — offsets are stable once placed,
minimum 85×85pt per slot preserved. The interaction geometry is a grid; the visual is a living reef.

### 5.2 The Reef IS the Playing Surface
The 4x4 grid doubles as the instrument interface. No separate PlaySurface screen.

**Touch interactions:**
- **Tap a slot**: Note-on at that specimen's pitch mapping. Velocity from tap speed (time from touch-down to touch-up, inverted — fast tap = high velocity). On legacy devices with 3D Touch (iPhone 6s–XS Max), tap pressure is used instead for more natural feel.
- **Hold a slot**: Sustained note. Longer holds on OBRIX produce evolving timbre (B040 Note Duration principle).
- **Slide between slots**: Legato transition. The coupling between the two specimens modulates in real time during the slide. Pitch glides if both are Sources.
- **Two-finger pinch on a slot**: Adjusts that specimen's primary macro (CHARACTER). Visual feedback: the creature grows/shrinks with the pinch.
- **Two-finger rotate on a slot**: Adjusts MOVEMENT macro. The creature spins.
- **Three-finger swipe down (anywhere)**: All notes off / panic.

**Touch target sizing:**
- At iPhone SE (4.7", 375×667pt): 4x4 grid in ~340×340pt area = 85×85pt per slot. Well above Apple HIG 44×44pt minimum.
- At iPhone 15 Pro Max (6.7", 430×932pt): ~400×400pt area = 100×100pt per slot. Generous.
- Grid occupies the center 80% of screen width, with reef name above and Dive button below.

**Audio routing:**
- Each Source specimen generates audio independently (polyphonic — up to 4 Sources, each with their own voice)
- Processor specimens modify adjacent Source output (adjacency = coupling)
- Modulator specimens modulate adjacent specimens' parameters
- Effect specimens process the mixed output (parallel or serial, configurable)
- The reef's audio output is the sum of all coupled chains

### 5.3 Audio Export
- **Record Reef**: Long-press the reef name → "Record" option. Records reef audio output for up to 60 seconds.
- **Format**: M4A (AAC 256kbps) via AVAudioEngine offline render tap.
- **Share**: On stop, share sheet appears (AirDrop, Files, Messages, social media).
- **Silent export**: Works in silent mode — renders to file without speaker output if needed.
- **15-second clip mode**: Quick-share option generates a 15-second ambient clip from the reef's current state, optimized for Instagram Stories / TikTok. Auto-fades in/out.

### 5.4 MIDI Output
- **CoreMIDI virtual port**: OBRIX Pocket creates a virtual MIDI source visible to other apps (AUM, GarageBand, etc.)
- **Note mapping**: Each reef slot emits on a configurable MIDI channel. Tap velocity maps from the touch-speed curve.
- **No MIDI input**: The reef IS the controller. External MIDI input is intentionally excluded to preserve the phone-as-instrument identity.
- **Ableton Link** (Phase 3): Optional clock sync for LFO and ambient drone tempo-locking.

**Reef ambient mode** (app backgrounded):
- All coupled specimens produce a slow, generative drone
- Modulators run at 1/10th speed (ultra-slow LFO evolution)
- Audio session switches to .playback (mic disabled in background)
- User can disable in Settings
- Respects iOS audio interruptions (phone calls, Siri)
- Ducks under other media (podcast, music) at -12dB

## 6. The Dive — Coupling Puzzle Under Pressure

### 6.1 Concept
The Dive is a 5-minute descent where survival depends on DECISIONS, not endurance. At each depth threshold, the player must solve a coupling challenge: connect the right specimens to generate oxygen. Wrong connections produce dissonance and waste oxygen. Right connections produce harmony and sustain the reef.

### 6.2 Mechanics

**Oxygen**: Shared resource, starts at 100%. Depletes at 2%/sec passively. Correct coupling challenges restore 15-25% per solve. Playing actively (touching specimens) slows depletion to 1%/sec. At 0% oxygen, all specimens begin rapid bleaching (5 health/sec).

**Depth zones** (each 60 seconds):
| Zone | Depth | Passive Depletion | Challenge Type |
|------|-------|-------------------|----------------|
| Sunlit | 0-200m | 2%/sec | Tutorial: connect any two specimens |
| Twilight | 200-600m | 2.5%/sec | Connect specimens of specific CATEGORIES (e.g., "Route a Modulator to a Source") |
| Midnight | 600-1200m | 3%/sec | Connect specimens that share a BIOME (e.g., both caught in rain) |
| Abyssal | 1200m+ | 3.5%/sec | Connect specimens that have COMPLEMENTARY spectral DNA (algorithmic: spectral profiles with lowest overlap = best coupling) |

**Coupling challenges**:
- A challenge appears every 15 seconds: two empty coupling slots highlighted on the reef grid
- The player must drag a line between two specimens that satisfy the challenge criteria
- Correct: oxygen restored, coupling activates with a satisfying harmonic lock, haptic confirmation
- Incorrect: oxygen penalty (-5%), dissonant sound burst, haptic error buzz
- Timeout (10 seconds): oxygen penalty (-10%), the challenge dissolves

**Sonic zone processing** (applied to master output during descent):
| Zone | Sonic Character |
|------|----------------|
| Sunlit | Normal reef sound. Warm, familiar. Golden visual tint. |
| Twilight | Low-pass filter creeps in (~8kHz cutoff, descending). Highs fade. Deep blue tint. |
| Midnight | Reverb extends massively (tail 3s → 8s). Pitch bends -50 cents. Coupling lines glow brighter. |
| Abyssal | Stripped to fundamentals. No reverb. No highs. Raw synthesis only. Max coupling depth. The sound at 1200m reveals your reef's hidden character — something only the Dive surfaces. |

The Dive is a sonic journey that reveals your reef's hidden character at each depth. Players
will Dive not just for Deep Specimens but because the Dive SOUNDS DIFFERENT than regular play.

**Skill expression**:
- Deep knowledge of your reef's spectral DNA composition = faster correct coupling
- Strategic reef arrangement = more coupling options available
- Speed under pressure = more challenges solved per zone
- A day-365 player solves Abyssal challenges from memory; a day-1 player struggles in Twilight

### 6.3 Rewards
- Survive 3+ minutes → Common Deep Specimen
- Survive full 5 minutes → Rare Deep Specimen
- Survive with all specimens above 50% health → Legendary Deep Specimen
- Each Dive adds to lifetime totalDiveDepth

### 6.4 Consequences
- Specimens that reach 0 health during the Dive BLEACH immediately (become Phantoms)
- Phantom specimens in the Dive are ejected (cannot participate in remaining challenges)
- If all specimens become Phantoms, the Dive ends immediately — no reward
- Phantoms earned in the Dive carry a "Deep Phantom" tag — they have unique deep-ocean spectral character

### 6.5 Dive Eligibility
- Minimum 4 non-Phantom specimens must be active in the reef to begin a Dive
- The pre-dive screen (Tab 3) shows each specimen's eligibility status:
  - Green checkmark: healthy, Dive-eligible
  - Red X: Phantom, will be ejected on descent
  - Grey dash: empty slot
- "Begin Descent" button is disabled with tooltip "Need 4+ healthy specimens" if fewer than 4 are eligible
- If exactly 4 are eligible, a warning: "No margin — losing one specimen ends the Dive"

### 6.6 Dive Interaction Mode
During the Dive, the reef grid switches from PLAY mode to DIVE mode:

**PLAY mode (normal reef):**
- Tap = note-on
- Hold = sustained note
- Slide = legato transition
- Pinch/rotate = macro control

**DIVE mode (during descent):**
- Tap a slot = select specimen (highlights it, plays a brief audio preview)
- Drag FROM one specimen TO another = draw coupling route (the challenge action)
- Hold = still plays sustained note (playing generates oxygen)
- Slide between slots = still legato (playing is encouraged)
- Pinch/rotate = DISABLED during Dive (prevents accidental macro changes under pressure)

The mode switch is visually communicated:
- DIVE mode: slot borders pulse with depth-zone color (blue → violet → dark blue → black)
- A "COUPLING CHALLENGE" banner appears at the bottom when a challenge is active
- The drag affordance shows a dotted line from finger to cursor during route drawing
- Successful coupling: solid line snaps into place with harmonic audio confirmation
- Failed coupling: line shatters with dissonant audio burst

Exit: When the Dive ends (survived or failed), mode reverts to PLAY automatically.

## 7. Game Economy

### 7.1 Specimen Categories & Counts
| Category | What It Is | Subtypes | Total Unique |
|----------|-----------|----------|:------------:|
| Source | Oscillators (PolyBLEP, noise, wavetable, FM) | 8 | 8 |
| Processor | Filters, waveshapers, feedback paths | 6 | 6 |
| Modulator | Envelopes, LFOs, velocity maps, aftertouch | 6 | 6 |
| Effect | Delay, chorus, reverb, distortion | 4 | 4 |
| **Total unique specimens** | | | **24** |

Each of the 24 subtypes exists in 4 rarity tiers (Common, Uncommon, Rare, Legendary). Total possible collection: 96 unique specimens. But the reef holds 16 — curation is essential.

### 7.2 Spawn Source Exclusivity
Each source gives exclusive CATEGORIES to prevent cannibalization:

| Source | Exclusive Category | Why |
|--------|-------------------|-----|
| Daily Drift | Sources (oscillators) | Daily engagement hook — the building blocks |
| Login Milestones | Modulators | Rewards consistency with expression tools |
| Performance Rewards (Dive) | Effects | Skill-gated rewards — the polish layer |
| Weather/Time Bonus | Processors (filters) | Environmental flavor matches filter character |
| Coupling Discovery (Phase 4) | Hybrid (cross-category) | Emergent — combining produces novelty |
| Exploration Bonus | Any category, Rare+ rarity guaranteed | Rewards movement with quality, not exclusivity |
| Community Events (Phase 4) | Special limited-edition variants | Time-limited social engagement |

### 7.3 Economy Sinks
| Sink | Mechanism |
|------|-----------|
| Reef capacity (16 slots) | Must RELEASE a specimen to catch when full |
| Health decay (dormancy model) | After 7-day grace + 1pt/day decay → Phantom (gameplay-limited) |
| The Dive | Failed dives bleach specimens |
| Release to wild | Released specimens re-enter spawn pool for other players (with provenance) |
| Prestige | Voluntary reef dissolution for meta-progression |

### 7.4 Endgame: Prestige
**Complete Reef**: Fill all 16 slots with specimens from all 4 categories. Visual celebration: the reef emits a synchronized bioluminescent pulse, all creatures dance.

**Prestige Mode** (post-completion):
- Dissolve the reef — all specimens return to the wild with your provenance
- Your Soul Imprint (behavioral vector) is preserved

**Soul Imprint definition**: A persistent behavioral vector (16 floats) accumulated across all
play sessions:
- Catch preferences: category distribution, biome frequency, time-of-day patterns (4 floats)
- Play style: velocity mean/variance, note density, chord-vs-melody ratio (4 floats)
- Dive character: average depth, challenge solve rate, zone time distribution (4 floats)
- Reef personality: coupling depth preference, phantom ratio, category balance (4 floats)

The Soul Imprint subtly tints the reef's ambient drone — two players with identical specimen
configurations but different Soul Imprints hear slightly different drones. The imprint doesn't
affect gameplay directly; it makes the instrument feel YOURS.

Stored in UserDefaults (128 bytes). Travels with .xoreef snapshots to XOceanus desktop.
Accumulation begins Phase 2. Ambient drone tinting begins Phase 3.

- Receive a Prestige token (visible in your profile)
- Start fresh with an empty reef
- Prestige benefit: each Prestige level makes Deep Phantoms more spectrally complex and beautiful
- Prestige counter visible to other players in trades and Reef Visits (in trades immediately; in Reef Visits from Phase 4).

### 7.5 Phantom Tradeoffs (Revised)
Phantoms are INTERESTING but GAMEPLAY-LIMITED:
- Inverted spectral character (unique sound)
- Coupling-capable with other Phantoms and Commons/Uncommons
- CANNOT enter the Dive (ejected on descent start)
- CANNOT couple with Legendary specimens
- CANNOT be traded (too fragile to transfer)
- CAN be revived (returns with spectral scar — memory of phantom state)
- The tradeoff: a reef with many Phantoms sounds INTERESTING but is DIVE-INELIGIBLE

## 8. Social Features

### 8.1 Trading
Bluetooth proximity detection. Physical meetup required. Specimens transfer (not copy) with full provenance metadata ("Caught in Tokyo rain, 2027-03-15").

### 8.2 Reef Visits
View another player's reef as pixel art (no AR required in initial release (Phase 4)). See their creatures, coupling, health auras.

### 8.3 Competitive Dives (Phase 4)
Optional ranked mode with specimen stakes. Both players agree. Loser's deepest-surviving specimen transfers to winner. Tie-break: if both players reach the same depth, no specimen transfers (draw). Disconnect: if either player disconnects, the dive is voided — no transfers, no penalty.

## 9. Reef Link — Transfer to XOceanus Desktop

### 9.1 Snapshot Transfer
`.xoreef` format — frozen specimen configuration + spectral DNA + coupling state.
One-way: Pocket → XOceanus.

- **Transport**: Local network via Network.framework (Bonjour service advertisement + TCP socket).
  Works over USB (iPhone plugged into Mac creates a USB network interface) and WiFi (same network).
  No MFi certification required. No special framing or byte-stuffing. Transfer JSON files of arbitrary size.
  USB is effectively zero-latency; WiFi adds ~5ms. Both are transparent to the app.
- **Protocol**: iPhone advertises a Bonjour service "_xoreef._tcp". XOceanus desktop discovers it.
  Transfer is a simple HTTP POST of the .xoreef JSON file. Desktop confirms receipt with 200 OK.
  The .xoreef file format is JSON (parameter state) + base64-encoded spectral DNA floats.
  Typical file size: 5-15KB per snapshot.

### 9.2 Sealed on Desktop
Snapshot is specimen-locked. Cannot edit specimens, reorder, or open the hood on desktop.
Behaves like any XOceanus engine: audio out, coupling in, 4 macros exposed.

### 9.3 Snapshot Library
- **Snapshot Library**: Unlimited .xoreef snapshots stored on desktop. Up to 5 can be
  ACTIVE simultaneously (loaded into XOceanus engine slots — matches existing 5-slot system).
  Inactive snapshots remain on disk, loadable anytime. No forced deletion.

## 10. Technical Architecture

### 10.1 Stack (REVISED after QDD)

**JUCE Audio + SwiftUI/SpriteKit UI (Hybrid)**

ObrixEngine.h has deep JUCE dependencies (juce::AudioBuffer, juce::MidiBuffer,
juce::AudioProcessorValueTreeState, juce::String, juce::Colour). Pure AVAudioEngine
hosting (original Option B) would require a 2-3 week C-wrapper extraction sprint
with no user-facing benefit.

Revised approach:
- **Audio layer**: JUCE audio modules (juce_audio_basics, juce_audio_processors,
  juce_dsp). NO JUCE GUI modules. These host ObrixEngine.h natively. ~15-20MB binary cost.
- **UI layer**: SwiftUI (menus, settings, collection) + SpriteKit (reef grid,
  creatures, dive visuals). Native iOS feel. No JUCE windows.
- **Bridge**: Objective-C++ (.mm) translation units bridge JUCE audio ↔ Swift UI.
  The 19 existing .mm bridge files (AudioSession_iOS, HapticEngine, SensorManager,
  ReefBridge, MobilePlaySurface, etc.) are directly reusable.
- **Binary**: ~25-35MB total (15-20MB JUCE audio + 5-10MB app + assets). Under 80MB target.

This is faster to Phase 0 by 2-4 weeks vs the pure-native path, reuses existing
infrastructure, and avoids the JUCE GUI overhead that made Option A bloated.

### 10.2 Key Dependencies
- CoreLocation (significant-change monitoring for biome detection)
- OpenWeather API (free tier, weather for spawn + spectral DNA)
- CoreMotion (accelerometer for spectral DNA + Dive input)
- CoreBluetooth (trading)
- CoreHaptics (coupling feedback)
- SpriteKit (pixel creature rendering)
- AVFoundation (microphone spectral capture)

### 10.3 Data Model
```
Specimen {
  id: UUID
  type: Source | Processor | Modulator | Effect
  subtype: String (e.g., "PolyBLEP", "SVF", "LFO")
  rarity: Common | Uncommon | Rare | Legendary
  health: Int (0-100)
  spectralDNA: [Float] (64 values)
  catchLocation: CLLocationCoordinate2D
  catchTimestamp: Date
  catchWeather: WeatherSnapshot
  catchAccelPattern: [Float] (movement signature)
  parameterState: [String: Float] (OBRIX param values)
  isPhantom: Bool
  phantomScar: Bool (was ever a phantom and revived)
  provenance: String (trade history)
  creatureGenome: XOGenome (pixel art data)
}

Reef {
  specimens: [Specimen?] (fixed-size array, 16 slots)
  couplingRoutes: [(Int, Int, Float)] (source, dest, depth)
  name: String
  totalDiveDepth: Int (lifetime achievement)
}
```

### 10.4 File Formats
- `.xoreef` — reef snapshot for desktop transfer (JSON + base64-encoded spectral DNA floats)
- `.xogenome` — pixel creature definition (already exists)
- Realm or SQLite for local specimen/reef persistence on device

## 11. First Launch Script

### 0-5s: Animated Intro
A pixel-art coral reef grows from a dark ocean floor. XO_OX logo fades in. "OBRIX Pocket" title appears. No permission dialogs of any kind.

### 5-15s: First Specimen Arrives
The ocean floor becomes the reef grid (empty 4x4). A single starter specimen — a Common Source specimen (a basic PolyBLEP oscillator) — floats down from above, glowing softly. Text: "Your first specimen has arrived." The word "specimen" is used throughout (not "brick" — per the QDD naming finding). An arrow points at the specimen.

### 15-25s: The Catch
User taps the specimen. Catch animation: the creature hatches from a shell, lands in slot [0,0] of the reef. A warm haptic confirmation pulse fires. The reef glows faintly around that slot.

### 25-35s: First Sound
Text: "Touch your reef to play." User touches the occupied slot. OBRIX makes sound — a warm, simple, immediately musical tone. The creature reacts visually (bounces, glows brighter). No parameters are visible. Touch = sound. Nothing else required.

### 35-60s: Second Specimen + Coupling
A second specimen appears (a Common Processor — filter). Text: "Tap to catch." User catches it. It lands in slot [0,1]. Now two slots glow. Text: "Touch both — hear them together." User touches both slots simultaneously. Filtered sound. The coupling line appears between them automatically. The user has a 2-specimen reef in under 60 seconds.

### Post-60s: Free Play
No more tutorials until the user has played for 2+ minutes. Then, subtly: an in-app notification (not a push notification — no notification permission requested yet) reads "A wild specimen has appeared nearby." This leads to their first real catch — and it is at this moment, when the user taps "Go catch," that location permission is contextually requested for the first time.

### Permissions Flow
All permissions are requested contextually, never upfront:

- **Location**: Requested before the first outdoor catch attempt — when the user first taps "Go catch" in the Catch tab.
- **Microphone**: Requested before the first spectral DNA capture. Prompt: "Allow microphone to hear your environment and bake it into this specimen's sound?"
- **Motion**: Requested at the same moment as microphone — grouped into a single natural moment.
- **Notifications**: Requested after the 3rd catch. "Want to know when rare specimens appear?"
- **Bluetooth**: Requested only when the user initiates their first trade.

---

## 12. Screen Inventory & Navigation Model

### Navigation Model
Bottom tab bar with 4 tabs, plus modal sheets that slide up from the bottom.

---

### Tab 1: REEF (Home)

**Primary view**: 4x4 reef grid. Each occupied slot shows its pixel creature.

- Coupling lines visible between coupled specimens
- Health aura: glow intensity reflects health percentage (0–100)
- Phantom specimens shown as translucent/ghostly in their slots
- Tap a specimen = hear it solo (brief preview)
- Touch and hold anywhere on the grid = PLAY mode (PlaySurface activates — see Section 5.2)
- Long press a specimen = context menu: Solo / Stasis / Release / View Provenance
- Empty slots show a faint "+" affordance
- Top bar: reef name, total Dive depth (lifetime achievement badge)
- Bottom: "Dive" button — prominent, pulsing gently when reef has 4+ specimens

---

### Tab 2: CATCH (Radar)

**Primary view**: Circular radar — not a map. Simpler and more atmospheric.

- Center dot = your position. Specimens appear as colored pips at approximate distance and direction.
- Pip color by category: blue = Source, red = Processor, green = Modulator, purple = Effect
- Pip size by rarity: larger pips are rarer
- Tap a pip when nearby (within 50m) = opens the catch screen
- **Catch screen**: The specimen SINGS — it plays a 5-second generative phrase using its synthesis
  parameters. The ambient spectral capture runs simultaneously, modulating the phrase in real time
  (your environment duets with the specimen). A touch strip appears at the bottom. Play a note
  that harmonizes → HARMONIC LOCK animation, catch succeeds, creature flies to reef. (Harmonic lock fires when the touch strip note is within ±100 cents of a consonant interval — unison, fourth, fifth, or octave — relative to the specimen's current root pitch. All other intervals succeed but produce a softer "partial lock" with less dramatic animation.) Dissonant
  input doesn't fail the catch — the specimen adapts its phrase. Every catch is a unique 10-second
  musical encounter. The catch teaches the specimen's sonic character through play, not description.
  If microphone is denied: specimen still sings but without ambient modulation (Studio Catch).
- Weather indicator in corner showing current conditions (affects spawn behavior)
- "Reef Proximity" toggle accessible via gear icon in this tab
- **Offline**: Radar still shows Daily Drift specimens (cached; no network required)

---

### Tab 3: DIVE

**Pre-dive view**: Reef configuration overview, specimen health check, coupling map preview.
- "Begin Descent" button — disabled if fewer than 4 NON-PHANTOM specimens in the reef.

**During dive**:
- Depth meter on left edge
- Oxygen gauge at top
- Reef grid in center
- Coupling challenge panel at bottom

**Post-dive results screen**: Depth achieved, specimens bleached and saved, Deep Specimen reward if earned. Post-dive results include a 'Send to Desktop' shortcut button (opens Reef Link flow directly from results).

**Dive history**: Scrollable list of past dives — depth, date, specimens lost and gained.

---

### Tab 4: COLLECTION

**Primary view**: Specimen catalog — all caught specimens in a scrollable grid.

- Filter by: category, rarity, health status, biome of origin
- **Specimen detail**: Full provenance card — catch location, date, weather at catch time, spectral DNA visualization, trade history
- **Stasis vault**: 10 cryo slots. Drag specimens in and out.
- **Trading**: Button opens trade modal (Bluetooth pairing flow)
- **Reef Link**: Button opens USB transfer flow (export `.xoreef` to desktop)
- **Settings**: Accessible via gear icon in this tab

---

### Modal Sheets (Slide Up From Bottom)

| Sheet | Contents |
|-------|----------|
| Settings | Audio output, Reef Proximity toggle, notification preferences, account |
| Trade | Bluetooth discovery, specimen selection, confirmation flow |
| Reef Link | USB connection status, snapshot export, snapshot library on desktop |
| Specimen Detail | Full provenance card, spectral DNA waveform, play button |

---

## 13. Permission Orchestration

| Permission | When Requested | Context Shown to User | If Denied |
|------------|---------------|----------------------|-----------|
| None | First launch (0–60s) | Starter specimen + first play. Zero permissions needed. | N/A |
| Location (When In Use) | First time user taps "Go catch" or switches to Catch tab | "OBRIX Pocket uses your location to flavor each specimen's sound with where you caught it. Your location is never stored or shared." | Reef Proximity auto-activates. Daily Drift specimens appear without location. Catch tab shows a persistent banner: "Enable location to discover wild specimens." |
| Microphone + Motion | First real catch (spectral DNA moment) | "Allow microphone to hear your environment? OBRIX captures a sound fingerprint during the catch encounter — no audio is recorded or stored. This gives each specimen a unique sonic character." Motion is grouped: "Allow motion sensors to capture your movement pattern?" | Catches proceed without spectral DNA. A default spectral profile substitutes. Specimen receives a "Studio Caught" tag instead of location-based DNA. |
| Notifications | After 3rd catch | "Rare specimens can appear anytime. Want to be notified when one is nearby?" | Catch loop continues without degradation. User must check the app manually. |
| Bluetooth | First time user taps "Trade" | "OBRIX uses Bluetooth to find nearby players for specimen trading." | Trading unavailable. "Enable Bluetooth in Settings to trade" message shown in Trade sheet. |

### Re-Request Strategy

If a permission is denied:
- Show a non-intrusive banner inside the relevant tab (not a popup) once per session.
- After 3 sessions of denial with the banner shown, stop asking entirely.
- A "Permissions" section in Settings allows the user to manually re-enable any permission at any time.

---

## 14. Phased Build Plan

### Phase 0: Foundation (3-4 weeks)
**Gate: OBRIX makes sound on iPhone**
- [ ] Week 1: Complete spec (first launch script, screen inventory, PlaySurface, economy numbers, permissions)
- [ ] Week 2: Configure JUCE audio modules for iOS (no GUI). Set up Xcode project with SwiftUI + SpriteKit + JUCE .mm bridge.
- [ ] Week 3: Get ObrixEngine.h rendering audio via JUCE on iOS. Basic SpriteKit scene with a single touch target → note output.
- [ ] Week 3-4: Audio session management (interruptions, background/foreground transitions)
- Deliverable: An app with one animated pixel creature that makes OBRIX sounds when you touch it

### Phase 1: Core Loop MVP (5-6 weeks)
**Gate: Catch → Build → Play is fun**
- [ ] Biome detection (CLLocationManager significant-change)
- [ ] Procedural specimen spawning (Perlin noise + loot tables)
- [ ] Specimen catching UX (notification → open → tap → catch)
- [ ] Spectral DNA capture (ambient FFT during 5-sec catch phrase)
- [ ] Reef grid UI (SpriteKit creatures + coupling lines)
- [ ] Reef ambient mode (background audio)
- [ ] Daily Drift + Login Milestone spawn sources
- [ ] Reef Proximity accessibility toggle
- [ ] Musical catch duet (generative phrase + touch strip + harmonic detection)
- [ ] Organic reef layout (pseudo-random offsets + Bézier coupling + breathing animation)
- [ ] Spectral fingerprint visualization (radial waveform shader from 64-float profile)
- [ ] Audio export (60-sec reef recording to M4A + share sheet)
- Deliverable: TestFlight beta — catch specimens, build reef, play it

### Phase 2: The Dive + Polish (3-4 weeks)
**Gate: The Dive is thrilling**
- [ ] Dive mode (water column descent, coupling pressure ramp)
- [ ] Oxygen mechanic (playing keeps specimens alive)
- [ ] Deep Specimen rewards
- [ ] Health decay + bleaching + phantom transformation
- [ ] Stasis mode (cryo storage)
- [ ] Creature animations (idle, healthy glow, bleaching, phantom)
- [ ] Haptic feedback (CoreHaptics tied to coupling)
- [ ] Dive sonic zones (per-zone master FX processing: LP filter, reverb, pitch, stripping)
- [ ] Soul Imprint accumulation (16-float behavioral vector, UserDefaults persistence). Soul Imprint accumulation begins silently (no user-visible effect until Phase 3 ambient drone tinting).
- [ ] MIDI output (CoreMIDI virtual port, per-slot channel mapping)
- [ ] Specimen card sharing (PNG: pixel creature + spectral fingerprint + provenance text → share sheet)
- Deliverable: Feature-complete beta

### Phase 3: Social + Reef Link (3-4 weeks)
**Gate: Trading works, desktop transfer works**
- [ ] Bluetooth trading (CoreBluetooth proximity)
- [ ] Reef Link transfer protocol (Network.framework Bonjour/TCP)
- [ ] .xoreef export/import
- [ ] XOceanus desktop: OBRIX adapter reads .xoreef snapshots
- [ ] Snapshot library (unlimited on disk, 5 active slots) on desktop
- [ ] Weather + Exploration + Time-of-Day spawn sources
- Deliverable: Full product, ready for App Store submission

### Phase 4: Community + Events (post-launch)
- [ ] Community Events server (Firebase)
- [ ] Competitive Dives
- [ ] Reef Visits (view other players' reefs)
- [ ] Coupling Discovery spawn mechanic
- [ ] Tide parameter (Colony View) on desktop
- [ ] Reef Bounce: on-device 16 one-shot render (one per specimen, 2-3 sec each) + AirDrop to MPC as WAV + XPM bundle

## 15. Risk Register

| Risk | Probability | Impact | Mitigation |
|------|:-----------:|:------:|------------|
| iOS audio bridge takes too long | HIGH | Fatal | Phase 0 is ONLY about audio. If it takes >3 weeks, simplify. |
| Catching specimens isn't fun | HIGH | Fatal | Playtest with 5 people at Phase 1. If not fun, redesign before Phase 2. |
| Apple rejects location usage | Medium | High | Submit minimal music app first, add location in update. |
| Battery drain from location | Medium | Medium | Significant-change only. Test on oldest supported device. |
| Scope creep | HIGH | High | Each phase has a gate. Don't start next phase until gate passes. |
| Binary size too large | Medium | Medium | Ship OBRIX only (not 88 engines). Target <80MB. |

## 16. What This Does NOT Include
- No 88 engines on iOS (desktop only)
- No DAW integration / AUv3
- No preset library browser (desktop feature)
- No coupling inspector UI (desktop feature)
- No AR reef visits (Phase 4+)
- No Sonic Atlas (Phase 4+)
- No web backend (until Community Events in Phase 4)

## 17. Success Metrics
- Phase 0 gate: audio works on iPhone within 3 weeks
- Phase 1 gate: 5 playtesters say catching specimens is fun
- Phase 2 gate: 5 playtesters say the Dive is thrilling
- Launch target: 1,000 downloads in first month
- Retention target: 30% day-7 retention (industry avg for games: 15%)
- Revenue: Free app. Patreon cross-promotion. Desktop upsell via Reef Link.
