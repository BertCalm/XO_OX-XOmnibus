# Seance Report: OSTINATO

**Date:** 2026-03-19
**Engine:** XOstinato — "The Fire Circle"
**Gallery Code:** OSTINATO
**Accent Color:** Firelight Orange `#E8701A`
**Parameter Prefix:** `osti_`
**Seance Number:** 30
**Presets at time of seance:** 0 (engine newly integrated 2026-03-18)

---

## Ghost Panel Summary

| Ghost | Score (1-10) | Key Comment |
|-------|-------------|-------------|
| Moog | 8 | "Velocity into filter cutoff at that range — 400 Hz to 16 kHz — that's a voltage-controlled expression I'd be proud of. The velSens knob per seat is exactly right. But I want a second LFO. One breath is not a life." |
| Buchla | 9 | "Eight seats, and the seats listen to each other. The CIRCLE system — neighbors triggering ghost notes from acoustic pressure — this is decentralized, emergent, non-hierarchical. West Coast philosophy in percussion form. The pitch spike as a physical event rather than pitch tracking — beautifully unconventional." |
| Smith | 7 | "The physical modeling is rigorous and the parameter coverage is comprehensive. But I count one LFO — hardcoded. No mod matrix. And exciterMix is registered but dead in the DSP. That's a broken promise. Fix it." |
| Kakehashi | 9 | "You press play and the circle begins. You pick up a stick and play along. You stop — the circle continues. This is what instruments should do. Musical utility at the highest level. The 12 instruments from 12 traditions — this is the Roland philosophy: bring the world to one machine." |
| Ciani | 8 | "The spatial arrangement — eight voices placed across the stereo field in a ring — I feel the circle. The humanization drawn from ethnomusicology, not random noise. Organic. And the CIRCLE macro gradually pulling voices into sympathetic resonance... this could be beautiful in a quadraphonic space." |
| Schulze | 7 | "The breathing LFO runs at 0.06 Hz forever. It cannot be slowed. I cannot make a 200-second drift. The engine is alive — I hear it breathing — but I cannot make it breathe cosmically. The LFO rate must be user-accessible. This is where temporal depth lives and right now it is locked." |
| Vangelis | 8 | "Aftertouch intensifies the fire. That is correct — when I lean into the instrument, it should burn brighter. The velocity-to-filter shaping is immediate and expressive. The GATHER macro pulling the circle from loose to locked is genuinely musical. I could score something with this." |
| Tomita | 8 | "Bessel function zeros for circular membranes, Raman's 1934 tabla harmonic measurements, JASA citations in the instrument table. This is physical modeling done properly. Twelve instruments, twelve traditions, forty-eight articulations — the timbral range from surdo bass to doumbek snap is an orchestra of percussion." |

**Consensus Score: 8.0 / 10**

*The panel is in strong agreement: OSTINATO is architecturally exceptional — perhaps the most musically democratic engine in the fleet. The CIRCLE system draws unanimous praise. The D002 gap (one hardcoded LFO, no user control, no mod matrix) and the D004 exciterMix dead parameter are concrete fixes that would push this to 8.5+.*

---

## Doctrine Compliance

| Doctrine | Status | Ghost Commentary |
|----------|--------|-----------------|
| D001 | **STRONG PASS** | `baseCutoff = 400.0f + brightness * effectiveVel * 16000.0f` — velocity directly drives filter cutoff from 400 Hz to 16 kHz. Also shapes exciter energy (`excitationLevel = velocityScale`), pitch spike intensity, and attack time. The velSens per-seat parameter controls depth. Moog: "That's the range I'd use." |
| D002 | **PARTIAL** | 4 macros fully wired ✓. Aftertouch ✓. Mod wheel ✓. But: ONE LFO, hardcoded at 0.06 Hz with no user control. No mod matrix. Minimum is 2 LFOs. Smith: "One LFO is a rhythm section with one hand." |
| D003 | **STRONG PASS** | Modal membrane Bessel zeros from Kinsler & Frey. Tabla modes from Raman (1934). Taiko from Fletcher & Rossing. Djembe from Brindle et al., JASA 2005. Four body types (Cylindrical/Conical/Box/Open) with appropriate physics per shape. Tomita: "I would have cited these sources in a lecture." |
| D004 | **FAIL** | `osti_seat{N}_exciterMix` is registered, read from APVTS, passed to `triggerSeat()`, then passed to `OstiSubVoice::trigger()` as a parameter — and then **never applied**. The actual exciter mix comes from the instrument data table (`art.exciterMix`), not the user parameter. All 8 seats × 1 dead parameter = **8 dead parameters**. Smith: "Broken promise." |
| D005 | **PASS** | `OstiBreathingLFO` runs at 0.06 Hz (one cycle per ~17 seconds) in every `OstiSubVoice::processSample()`. Modulates filter cutoff ±12% continuously. Rate is below perceptible LFO threshold — the sound feels alive without sounding modulated. Schulze: "It breathes. But I want a slower breath." |
| D006 | **PASS** | Velocity → filter cutoff + exciter amplitude ✓. Aftertouch (channel pressure) → FIRE boost ✓ (`fireBoost = clamp(fireBoost + atPressure * 0.5f, 0.0f, 3.0f)`). Mod wheel CC1 → CIRCLE depth ✓ (`gCircleAmt = clamp(gCircleAmt + modWheelAmount * 0.3f, 0.0f, 1.0f)`). |

---

## Sonic Identity

OSTINATO occupies territory no other XOlokun engine touches: **communal world percussion with emergent inter-voice behavior.** The engine's unique voice is the space between drummers — the way eight performers in a circle begin to respond to each other, not because they were told to, but because they hear each other.

**What it can make that nothing else can:**
- An automatic world drum circle that continues playing when you stop
- Eight different traditional percussion instruments from eight traditions simultaneously, each with authentic articulation sets
- Ghost notes that emerge from neighboring voices when CIRCLE is raised — not programmed, but sympathetically triggered
- A surdo at 55 Hz alongside a doumbek snap at 220 Hz alongside tabla na at 260 Hz — the full vertical stack of world percussion in one engine

**Dry patch quality:** Immediately compelling. The default circle (Djembe/Taiko/Conga/Tabla/Cajón/Doumbek/Frame Drum/Surdo) starts playing as soon as the engine is loaded at 120 BPM with a moving, organic feel. The physical modeling gives each instrument genuine character — the Taiko's thick membrane and long delay body sit below the Djembe's bright, resonant slap. This is not a sample player simulating drums: it is a drum circle.

**Character range:** Enormous. Pattern 3 (Sparse) on all seats with GATHER at 0 and SPACE at 1 produces near-ambient texture — individual hits floating in reverb, breathing LFO creating subtle life. Pattern 2 (Fill) on all seats with FIRE at 1 produces overwhelming polyrhythmic density. The Tongue Drum in harmonic mode at high brightness creates pitched metalophone territory the engine can cover.

**Init patch:** The default parameter state (all seats active, basic patterns, GATHER 0.5, FIRE 0.5) launches immediately into a compelling multi-cultural groove. No blank canvas. This engine answers DB003 for percussion: the default IS the invitation.

---

## Preset Review

*No factory presets exist yet — engine integrated 2026-03-18, 0 presets at seance time. Design targets for first presets based on seance findings:*

| Preset Target | Mood | Key Parameters | DNA Target |
|--------------|------|----------------|-----------|
| Circle of Fire | Foundation | All 8 seats active, patterns 0, FIRE 0.7 | High movement, medium density |
| Sparse Ritual | Atmosphere | Patterns 3 (sparse), SPACE 0.8, GATHER 0.2 | Low density, high space, low aggression |
| Ghost Circle | Aether | CIRCLE 0.8, humanize 0.5, FIRE 0.2 | High movement, low density |
| World Engine | Foundation | All 12 traditions represented in 8 seats | High density, balanced |
| Tabla Meditation | Atmosphere | Seats: Tabla×3, Frame Drum×2, Tongue Drum×3 | Medium movement, high warmth |
| Machine Meets Human | Entangled | ONSET→OSTINATO (AmpToChoke), pattern interaction | High movement, high coupling |
| The Heartbeat | Foundation | Surdo+Djembe basic patterns, GATHER 0.9 | High movement, low space |
| Deep Circle | Prism | Taiko+Surdo+Dundun low tuning, SPACE 0.7 | High warmth, low brightness |

---

## Coupling Assessment

**getSampleForCoupling output:**
- Channel 0: Left audio
- Channel 1: Right audio
- Channel 2: Envelope follower (one-pole ~10ms, sends rhythmic pulse information)

The envelope follower (channel 2) is particularly valuable — it sends the rhythmic energy of the circle as a modulation signal. Any engine receiving from OSTINATO's channel 2 via `AmpToFilter` will have its filter cutoff pumped by the drum circle's rhythm. This is an inherently musical coupling.

**Supported input coupling types:**
- `AmpToFilter` → modulates `couplingFilterMod` → adds to all seat brightness values
- `EnvToDecay` → modulates `couplingDecayMod` → adds to all seat decay times
- `RhythmToBlend` → **STUB** (comment only, no implementation)
- `AmpToChoke` → **Creative repurpose**: instead of choking, loud AmpToChoke input triggers ghost notes on a random seat. The ONSET×OSTINATO "Machine Meets Human" route uses this.
- `AudioToFM` → adds to `couplingFilterMod` at 0.3× scale (audio content modulates exciter brightness)

**Natural pairings:**
| Partner | Route | Musical Result |
|---------|-------|----------------|
| ONSET | ONSET→OSTINATO (AmpToChoke) | Machine triggers ghost notes in the circle — electronic percussion summons human percussion |
| OVERDUB | OSTINATO→OVERDUB (envelope follower) | Drum circle drives delay time in the dub system — World Dub |
| OPAL | OSTINATO→OPAL (AmpToFilter) | Circle rhythm pumps granular scatter — Scattered Gathering |
| OPENSKY | OSTINATO→OPENSKY (AmpToFilter) | Drum circle punches the shimmer reverb — Circle Sky |
| OCEANDEEP | OSTINATO→OCEANDEEP (EnvToDecay) | Circle extends the sub decay — The Deep Drum |

**Note:** `RhythmToBlend` coupling is declared but unimplemented. This could be a powerful route: an external rhythm source modulating which pattern steps fire. Should be implemented.

---

## Blessing Candidates

### B016 Candidate: The Communal Ghost Trigger System
**Engine:** OSTINATO
**Mechanism:** The CIRCLE macro enables acoustic sympathy between the 8 seats. Each block, OSTINATO checks the previous block's peak amplitude for each seat's neighbors (circular topology: left and right adjacent seats). If `neighborPeak × gCircleAmt > 0.4f` and the target seat is currently silent, a quiet ghost note is triggered at `velocity = neighborPeak × gCircleAmt × 0.25`. The result: loud beats in one instrument excite sympathetic quiet responses in adjacent instruments — just as a loud djembe hit in a real circle causes nearby players to instinctively respond.

**Why it's a Blessing:** No other engine in the fleet models inter-voice acoustic sympathy. The 8 sequencers + the CIRCLE system means the engine has two distinct "will" levels: the programmed pattern and the emergent response. The circle develops a conversation the composer didn't write. Buchla: "Decentralized, non-hierarchical — eight agents in a social system." This is genuinely novel.

### B017 Candidate: The Cross-Tradition Pattern Library
**Engine:** OSTINATO
**Mechanism:** 96 patterns (8 per instrument × 12 instruments) embedded directly in the DSP code. Each pattern is a 16-step world rhythm archetype drawn from authentic cultural sources: Djembe patterns include Kuku rhythm approximations and Sangban-style approaches; Tabla patterns model na/tin/tun/ge compositional relationships from Hindustani tradition; Doumbek patterns include doum/tek/ka sequences from Arab maqam percussion; Taiko patterns include center/edge/rim/flam structures from Japanese festival drumming. The modal frequency data cites academic sources (Raman 1934, JASA 2005, Fletcher & Rossing). This level of ethnomusicological research embedded in firmware is exceptional.

**Why it's a Blessing:** Tomita: "I've seen academic papers with fewer citations than this instrument table." The 48 articulations across 12 traditions represent a serious commitment to cultural authenticity that no other drum machine in the fleet (or arguably the market) demonstrates at the DSP level.

---

## Debate Relevance

**DB004 (Expression vs. Evolution):** OSTINATO resolves this debate for the percussion category. It offers expression (velocity → filter, aftertouch → fire, mod wheel → circle) simultaneously with evolution (autonomous pattern system, breathing LFO, CIRCLE emergent sympathy). The performer can be expressive AND the engine evolves independently. These are not in tension here — they reinforce each other.

**DB001 (Mutual Exclusivity vs. Effect Chaining):** The 8 seats are effectively a chaining/layering system at the rhythmic level. Seat 1's djembe chaining into Seat 2's taiko through the CIRCLE system is a new form of effect chaining — not audio effects, but behavioral effects. This is a new axis the debate hadn't considered.

**DB003 (Init Patch: Immediate Beauty vs. Blank Canvas):** OSTINATO answers definitively for Vangelis and Kakehashi: the default circle plays immediately and it sounds like a world drum circle. This is not blank canvas territory. The engine invites.

---

## Recommendations

### P0: Fix D004 — exciterMix is a dead parameter
**Location:** `OstiSubVoice::trigger()`, `Source/Engines/Ostinato/OstinatoEngine.h`
**Problem:** The `exciterMix` parameter (per seat, 8 parameters × 2 sub-voices) is registered, exposed to users, read from APVTS, and passed all the way to `OstiSubVoice::trigger()` — but never applied. The actual exciter mix is pulled from `art.exciterMix` in the instrument data table, ignoring the user value.
**Fix:** In `OstiSubVoice::trigger()`, use `exciterMix` to override the articulation's default noise/pitched ratio in `OstiModalMembrane::trigger()`. Pass the blended value: `float effectiveExcMix = lerp(art.exciterMix, exciterMix, 0.7f)` — user control blended with instrument character.

### P1: D002 — Add user-accessible LFO control
**Problem:** One hardcoded breathing LFO at 0.06 Hz with no user control. D002 requires minimum 2 LFOs with user-accessible rate.
**Fix options:**
- Add global `osti_breathRate` and `osti_breathDepth` parameters to expose the breathing LFO (fast fix)
- Add a second modulation LFO (global or per-seat) targeting decay, pan, or filter with user rate/depth/target (full fix)

### P2: Implement RhythmToBlend coupling
**Location:** `OstinatoEngine::applyCouplingInput()`, case `CouplingType::RhythmToBlend`
**Problem:** Declared coupling type, acknowledged in comments, but stub only.
**Fix:** Use incoming rhythm signal to modulate `densityThreshold` in the pattern sequencer — external rhythm source controls which steps fire.

### Low Priority: Swing range
Current swing range is 0–100 (percentage). The parameter maps to `samplesPerStep * swing * 0.005` — at 100, swing is 50% of a step. This is correct but the mapping feels backwards for users (100 = maximum swing). Consider a 0–0.5 range with a descriptive label.
