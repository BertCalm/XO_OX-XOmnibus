# Chapter 2: MPC Workflow Mastery — From Pack to Performance

*Written by Kai, MPC Workflow Specialist, XO_OX Designs*

---

There's a version of MPC ownership where you load a pack, play a few pads, get busy with life, and come back to the same three sounds for the next six months. It's not your fault — MPC is a deep environment and its depth isn't always obvious from the surface. The hardware looks approachable. The workflow rewards patience.

This chapter is about collapsing that patience curve. By the end of it, you'll understand exactly how XO_OX packs are structured, why they respond the way they do to your playing, what those Q-Links are actually connected to, and how to move between production and performance without losing your momentum.

Everything here applies to MPC Live III and MPC software 3.0+. Most of it applies to MPC Live II and X as well. Where hardware generations diverge, I'll flag it.

Let's get into it.

---

## 2.1 Understanding MPC's Program Architecture

*Programs vs. Tracks vs. Sequences — what lives where and why*

Before you can use XO_OX packs fluently, you need a clear model of how MPC organizes its sound world. The terminology is precise, and conflating these concepts is the root cause of most workflow confusion.

**The Program** is the atomic unit of sound in MPC. A program is a self-contained patch: one instrument (or one drum kit), its samples, its envelope settings, its FX chain, and its Q-Link mappings. A Keygroup program is a pitched instrument — samples spread across the keyboard, playable across multiple octaves. A Drum program is a collection of 16+ pads, each pad a discrete percussive hit with its own sample set, velocity layers, and kit voice settings. A Plugin program hosts a virtual instrument directly in the MPC environment. A CV program sends control voltage to external hardware.

XO_OX packs primarily ship in two formats: Drum programs (from the ONSET engine) and Keygroup programs (from melodic engines like OPAL, OVERDUB, ODYSSEY, and the rest of the fleet). Some packs include both — a kit and a corresponding tonal program designed to sit in the same mix.

**The Track** is where a program lives inside a project. Think of a Track as an instance of a program assigned to a channel. One track = one program = one MIDI channel = one set of pads. You can load the same program onto multiple tracks — useful for layering — and each track instance can have its own Q-Link state.

**The Sequence** is the container for your composition. A Sequence holds all the MIDI events recorded across all active tracks, for a specific section of your music. Most producers work in clip-launcher mode: multiple sequences per project, each one a section (intro, verse, chorus, break, outro), switching between them live or in arrangement.

The important thing about sequences in the context of XO_OX packs: **the program is not the sequence**. When you switch sequences, your programs persist. This means you can sequence the same ONSET drum program differently in your intro and your peak — and swap Q-Link states per sequence for different timbral feels without reloading anything.

> **Workflow tip**: Create a "template project" with your three or four most-used XO_OX programs pre-loaded across tracks. Save it as your starting point for every session. Loading time savings alone are worth this habit.

**Pad Banks** operate at the track level, not the sequence level. Bank A through Bank D gives you 64 pads from one program. XO_OX drum packs are designed to use all four banks meaningfully — more on this in section 2.5.

One more concept worth naming: the **Program Group**. In MPC 3.0+, you can group programs for simultaneous Q-Link control — useful when you want one knob to affect both the kick program and the bass program in complementary ways. XO_OX "Coupled" packs (marked in the manifest) are designed with specific Program Group assignments in mind.

---

## 2.2 The Expansion Pack Hierarchy

*Collections → Packs → Programs → Keygroups — how the XO_OX catalog is organized*

XO_OX's release architecture has four levels. Understanding them makes browsing and purchasing decisions much clearer.

**Collections** are the highest level. A Collection is a themed group of packs that share a design philosophy, visual identity, and compositional universe. Current XO_OX collections include Kitchen Essentials (acoustic flavor, FX recipe, boutique synthesis wildcard), Travel / Water / Vessels (geographic era, water-body acoustic character), and Artwork / Color (visual art movement, color science, cultural voice). Collections are released over time — individual packs within a collection can be purchased separately, but owning the full collection unlocks cross-pack coupling recipes.

**Packs** are what you download and install. A pack is an `.xpn` file — which is actually a ZIP archive containing programs, samples, and metadata. One pack typically contains 2–5 programs and all the samples they reference. Pack names follow the format `[ENGINE_NAME]/[Pack Concept]`, for example `ONSET/808 Reborn` or `OPAL/Tide Memory`. Pack version numbers use semantic versioning: `1.0.0` is the initial release, `1.1.0` adds programs, `1.0.1` fixes sample issues.

**Programs** are the individual `.xpm` files inside a pack. A program is what actually loads onto an MPC track. One pack might contain: a full drum kit (Drum program), a tonal pad derived from the same engine preset (Keygroup program), and a bonus FX chain program. When you browse packs on MPC hardware, you're browsing programs — the `.xpn` container is already unpacked into the MPC's expansion library.

**Keygroups** (for Keygroup programs) and **Pads** (for Drum programs) are the finest granularity — individual zones within a program. A Keygroup is a pitched range mapped to a sample or sample stack. A pad is one of the 16 physical pads in a drum program, each with its own velocity layers, mute groups, and kit voice settings.

When XO_OX describes a pack as having "8-velocity-layer Keygroups," we mean each individual note zone in that program has 8 samples stacked by velocity — so a soft press gets a different recording than a hard press, not just a louder version of the same one. This matters for the Velocity-Timbre Contract in section 2.7.

> **Worth knowing**: Inside MPC's expansion manager, packs are listed by Pack name. To see what programs are inside before loading, tap the pack thumbnail and look at the program list on the detail screen. This saves the "I loaded the wrong program" fumble during a session.

---

## 2.3 Loading and Navigating XO_OX Packs on Hardware

*Step-by-step for MPC Live III*

Installing an XO_OX pack and navigating to a program takes about 90 seconds the first time, less than 30 once you know the path. Here is the complete flow on MPC Live III running firmware 3.4+.

**Installing from USB or SD:**

1. Copy the `.xpn` file to your USB drive or SD card.
2. With MPC powered on, insert the drive.
3. Press **BROWSE** to open the browser.
4. Navigate to your drive and locate the `.xpn` file.
5. Long-press the `.xpn` file icon → select **Install Expansion Pack**.
6. MPC will unpack the archive and register it in your expansion library. Time varies with pack size — a standard XO_OX drum pack (200–400 samples) takes 15–45 seconds.
7. When complete, the pack appears in your **Expansions** library.

**Installing from MPC Connect (desktop):**

1. Open MPC Connect, select your connected MPC.
2. In the Expansions panel, click **Install Pack** and navigate to the `.xpn` file.
3. MPC Connect transfers and installs over USB. Faster than SD card for large packs.

**Loading a program from the Expansions library:**

1. Press **BROWSE**, then tap the **Expansions** tab (usually the second tab, recognizable by the XPN icon).
2. Scroll or search for your pack. XO_OX packs are named `XO_OX / [Pack Name]` in the vendor column.
3. Tap the pack to expand its program list.
4. Tap a program to preview it (the MPC will play its preview audio if one is included — XO_OX packs include 10-second previews).
5. To load: drag the program to a track in the track view, or press **Load** while the program is selected.
6. The program is now on your track, pads are lit, Q-Links are assigned.

**Finding the program you want quickly:**

MPC 3.0+ search uses both pack name and program name. Type "ONSET" to find all ONSET engine programs across all installed packs. Type "Foundation" to find all foundation-mood programs. XO_OX programs are tagged with engine name, mood, energy level, and genre — so `OPAL atmosphere` finds all low-energy granular programs.

> **Tip: Pre-load for live sets**: MPC Live III can hold up to 32 programs in memory simultaneously. Before a live set, load all programs you plan to use across tracks in a single project. Use the track mute to silence what you're not using. Switching between programs mid-set via the browser introduces loading latency; switching between pre-loaded tracks is instant.

**Navigating pad banks:**

From any track with a Drum or Keygroup program loaded, press **PAD BANK** to cycle through banks A, B, C, D. XO_OX drum packs are organized with:
- **Bank A**: Core kit voices (kick, snare, hats, clap)
- **Bank B**: Tonal/melodic percussion, FX hits, sustain elements
- **Bank C**: Variations and alternate takes on Bank A voices
- **Bank D**: Performance elements (fills, transitions, field recordings)

This layout is consistent across all XO_OX drum packs, so once you learn one, you know them all.

---

## 2.4 Q-Link Mastery — Morphing Timbre in Real Time

*The 4 Q-Link slots and what XO_OX maps to them*

The MPC's Q-Links are four physical knobs (on Live III, eight — but four active per bank) that control real-time parameter adjustments on the loaded program. They are the expressive surface of MPC performance. Most third-party packs use Q-Links as an afterthought — usually just a filter cutoff and a reverb send. XO_OX packs treat Q-Links as a first-class design element, and the assignments are intentional.

**The 4-Q-Link Design Standard for XO_OX Packs:**

Every XO_OX program ships with at least four Q-Link assignments. The philosophy:

- **Q1 — Tone / Character**: The core timbral quality of the sound. For most programs, this controls filter cutoff or harmonic tilt — moving Q1 from left to right moves the sound from closed and dark to open and bright. This is the "color" knob.

- **Q2 — Space / Environment**: The acoustic environment around the sound. Reverb send depth, room size, or pre-delay. Moving Q2 changes where the sound feels like it's happening — an intimate close-mic'd recording vs. a cavernous space. This is the "room" knob.

- **Q3 — Movement / Modulation**: The rate or depth of internal modulation — LFO rate, grain scatter (OPAL), flutter (OVERDUB tape engine), arpeggiator density. This controls how alive and unstable the sound feels. This is the "life" knob.

- **Q4 — Weight / Density**: The physical mass of the sound. For percussion: attack strength, transient sharpness. For melodic instruments: unison detune width, filter resonance, harmonic density. Moving Q4 from left to right adds physical presence. This is the "body" knob.

This mapping is consistent across the fleet. A producer who learns the Q1–4 grammar on one XO_OX pack immediately understands all of them.

**Engine-specific Q-Link naming conventions:**

The MPC OLED display shows an 8-character label for each Q-Link. XO_OX uses a standardized abbreviation system:

| Assignment | Display Label | Engine Examples |
|-----------|--------------|-----------------|
| Filter cutoff | `TONE` | All engines |
| Reverb send | `SPACE` | All engines |
| LFO rate | `MOVE` | OVERDUB, OPAL, ODYSSEY |
| Grain scatter | `SCTTR` | OPAL |
| Tape flutter | `FLTR` | OVERDUB |
| Harmonic tilt | `TILT` | OBLONG, OBESE |
| Grain density | `DNSTY` | OPAL |
| Era blend | `ERA` | OVERWORLD |
| Commune | `COMMUNE` | OHM |
| Pluck bright | `PLUCK` | ORPHICA |
| Breath | `BRTH` | OBBLIGATO |
| Chaos | `CHAOS` | OUROBOROS |
| Metabolic | `META` | ORGANON |

**Using Q-Links in live performance:**

In performance mode, Q-Links control the active track's program. Switching tracks switches Q-Link context — the physical knobs now control the newly active track. MPC 3.0 introduced Q-Link lock, which freezes a Q-Link to its track even when you switch — useful for keeping reverb active on a pad you want ringing while you play a new track.

> **Q-Link performance move**: Set Q1 (TONE) to a slow, deliberate sweep during your build section. XO_OX programs are designed so that the full Q1 travel from 0 to 127 is a meaningful musical statement — not a sudden click but a smooth timbral arc. Automate this in your sequence for hands-free builds.

**The 8-Q-Link banks on MPC Live III:**

MPC Live III expands to 8 physical Q-Links. XO_OX programs use Banks A and B:
- **Bank A (Q1–4)**: Core controls as above — Tone, Space, Movement, Weight
- **Bank B (Q5–8)**: Engine-specific deeper controls — varies by engine, always labeled

Bank B assignments for ONSET drum programs follow the MACHINE/PUNCH/SPACE/MUTATE macro system from XOmnibus: Q5 = MACHINE (kit character preset), Q6 = PUNCH (transient sharpness), Q7 = SPACE (reverb depth), Q8 = MUTATE (random variation seed).

---

## 2.5 Pad Banking Strategies

*How XO_OX organizes 64+ sounds across 4 pad banks*

A 16-pad MPC grid with 4 banks is 64 pads — enough to hold a complete compositional world. XO_OX drum packs are designed to fill this space with intentionality, not just quantity.

**The Standard Bank Architecture:**

As introduced in 2.3, XO_OX uses a consistent four-bank structure across all drum packs:

```
BANK A — Foundation Kit
[ Kick 1   ] [ Snare 1  ] [ CH 1     ] [ OH 1     ]
[ Kick 2   ] [ Snare 2  ] [ CH 2     ] [ OH 2     ]
[ Clap 1   ] [ Tom Lo   ] [ Tom Hi   ] [ Perc 1   ]
[ FX 1     ] [ Crash    ] [ Ride     ] [ Perc 2   ]

BANK B — Tonal + Extended
[ Bass Hit ] [ Chord 1  ] [ Pad 1    ] [ Stab 1   ]
[ Sub Hit  ] [ Chord 2  ] [ Pad 2    ] [ Stab 2   ]
[ Atmos 1  ] [ Atmos 2  ] [ FX Riser ] [ FX Fall  ]
[ Vocal 1  ] [ Vocal 2  ] [ Loop 1   ] [ Loop 2   ]

BANK C — Variations + Alternates
[ Kick Alt ] [ Snr Alt  ] [ CH Alt   ] [ OH Alt   ]
[ Kick Sat ] [ Snr Sat  ] [ CH Open  ] [ OH Open  ]
[ Clap Alt ] [ Tom Alt  ] [ Perc Alt ] [ FX Alt   ]
[ Kick Dry ] [ Snr Dry  ] [ Hat Dry  ] [ Fx Dry   ]

BANK D — Performance + Transitions
[ Fill 1   ] [ Fill 2   ] [ Fill 3   ] [ Fill 4   ]
[ Rise 1   ] [ Rise 2   ] [ Drop 1   ] [ Drop 2   ]
[ Intro    ] [ Verse    ] [ Bridge   ] [ Outro    ]
[ FX Prfm1 ] [ FX Prfm2 ] [ FX Prfm3 ] [ FX Prfm4 ]
```

This is a template, not a rigid rule. The specific content varies by pack. What stays consistent: Bank A is always the foundational kit; Bank D is always performance-oriented material.

**Playing across banks in a live set:**

The production instinct is to stay in Bank A and use everything there. The performance opportunity is in crossing banks mid-pattern. A snare from Bank A + a sustain clap from Bank C layered in the same sequence bar creates a layered snare hit that feels live rather than programmed. Bank D's fills and transitions, triggered by hand at the right moment, add the human intervention that sequences alone can't.

> **Workflow**: Record a basic pattern in Bank A. Leave Bank D empty in your recording. In your live take or final performance pass, manually trigger Bank D fills at section transitions — no programming required, pure feel.

**Keygroup programs and pad banking:**

For Keygroup (melodic) programs, pad banking works differently. Bank A is typically C-major across two octaves (8 pads per octave, C1–B1 on Bank A rows 1–2, C2–B2 on rows 3–4). Banks B and C extend the range or provide alternate articulations. Bank D holds special performance materials: chord voicings, transpositions, or pre-configured harmonic clusters.

XO_OX melodic programs include a "play in bank" page in the program notes — a single-page reference showing what each bank contains, printed in the pack's liner notes PDF.

**The 5th-bank pattern for premium packs:**

Some XO_OX flagship packs ship a "Bank E" as a separate bonus program — a fifth program with 16 pads of curated "best of the pack" material. This isn't a bank in the MPC hardware sense; it's a separate program designed as a performance-optimized selection from the full four banks. Look for programs with the `_BestOf` suffix in the program list.

---

## 2.6 Performance Mode vs. Production Mode Workflows

*Two modes, two mindsets, one MPC*

MPC's strength is that it is genuinely two different instruments depending on how you approach it. Production mode and performance mode aren't just different screen states — they invite different creative postures.

**Production Mode:**

Production mode is where you build. The primary view is the sequence editor: MIDI events, automation lanes, track arrangement. Your attention is on the arrangement over time — what happens at bar 5, how the chorus differs from the verse, where the sample starts and ends.

XO_OX packs in production mode are tools for sound selection and sound design. You're choosing the right program for each track, adjusting Q-Links to dial in the exact timbre, rendering your decisions to MIDI automation. The 8-Q-Link bank on MPC Live III becomes an automation lane — you can draw in Q-Link movements over time, creating the filter sweep you want in bars 8–12 without touching a knob in the final product.

Best practice for XO_OX packs in production mode:
- Load the program, play it raw before touching Q-Links
- Set Q1 (TONE) first — it's the biggest voice decision
- Lock Q2 (SPACE) early and leave it — reverb character is a once-per-track decision, not a performance variable
- Use Q3 (MOVEMENT) as your automation target — it changes over time naturally in well-produced tracks

**Performance Mode:**

Performance mode is where you play. The primary view is the pad grid, full-screen, maximum pads visible. MIDI recording may or may not be active. Your attention is on what's happening right now — hitting the right pad at the right moment with the right velocity.

XO_OX packs in performance mode are instruments. The Q-Link grammar (Tone / Space / Movement / Weight) is designed to be navigable by feel without looking at labels. After a few sessions with one pack, your hands know that left-of-center on Q1 is the darker sound and right-of-center is the brighter one. This muscle memory is why the Q-Link standard matters — it transfers across the fleet.

Performance mode with XO_OX recommendations:
- Assign the Q-Link you're most likely to perform to a physical position your hand returns to naturally (Q1, leftmost)
- Use Q4 (WEIGHT) for dynamics control in the moment — pulling it back gives you room during verses, pushing it gives impact at drops
- In a live set, switch tracks by pressing the track button directly without stopping playback — XO_OX programs are tuned to have low startup transients so there's no audio artifact when a track activates

> **The two-hand technique**: Left hand on pads, right hand on Q-Links. While your left hand is playing a repetitive pattern, your right hand moves a Q-Link slowly. This continuous manual variation is what makes a live set feel live. Program it in and it sounds robotic. Play it live and it sounds human.

**Hybrid: Live Arrangement Mode:**

MPC 3.0 introduced a third mode: Live Arrangement, which sits between production and performance. Sequences are visible as clips in a launcher grid. You trigger sequences manually (like Ableton Live's clip launcher) while your programs remain loaded. XO_OX packs are designed to work cleanly in this mode — program transitions are smooth because the programs are persistent across sequences.

---

## 2.7 The Velocity-Timbre Contract

*Why XO_OX packs respond differently to hard vs. soft hits*

This is the most important section in this chapter for understanding why XO_OX packs feel different from most other expansion packs.

**The standard velocity model (most packs):**

Velocity = volume. A soft press = quiet. A hard press = loud. The sample playing is the same sample at both velocities; only the output amplitude changes. This is functional. It is also musically flat — it treats dynamics as a loudness variable rather than an expressive one.

**The XO_OX velocity model:**

Velocity = timbre + volume. A soft press is not just quieter — it's a different recording, with different frequency content, different transient character, different harmonic structure. A hard press isn't just louder — it's a genuinely different state of the instrument.

This is called the Velocity-Timbre Contract, and it is a design doctrine across the XO_OX fleet (Doctrine D001 in the internal spec). Every program in every XO_OX pack is auditioned at minimum velocity (v1) and maximum velocity (v127) during quality review. The rule: if those two extremes don't sound meaningfully different — not just in volume but in character — the velocity design is rejected.

**What this means for your playing:**

Your velocity sensitivity settings on MPC hardware directly affect how much timbral range you have access to. High sensitivity (physical setting in MPC's pad calibration) means small variations in hand pressure produce large velocity range — you can access the full timbral spectrum with nuanced playing. Low sensitivity compresses the velocity range — useful if you want consistent dynamics, but you lose timbral variation.

XO_OX recommends: medium-high sensitivity for melodic programs, medium sensitivity for drum programs. The specific calibration in MPC Live III's pad calibration screen should aim for `vTarget = 90–100` on a firm press.

**The four velocity zones and what they mean:**

| Zone | MIDI Range | Playing Feel | Sound Character |
|------|-----------|-------------|----------------|
| pp | 1–40 | Feather touch, near silence | Soft attack, muted highs, minimal saturation, long tail |
| mp | 41–80 | Normal press, relaxed hand | The designed "canonical" sound — this is the intended character |
| mf | 81–110 | Deliberate, committed | Sharper attack, brighter highs, slight saturation entering |
| ff | 111–127 | Full force | Maximum transient, full frequency content, saturation at its peak |

The mp zone is the reference. Everything else is a departure from it. Designing a beat entirely in the mp zone is composing with the sound's resting identity. Dropping to pp for a verse and pushing to ff for the chorus is composing with its emotional range.

> **The listening test**: Take any XO_OX drum pack. Hold your hand one centimeter above a pad and just brush it (pp velocity). Then hit the same pad hard (ff velocity). The difference you hear is the velocity-timbre range. If it sounds like the same sample quieter and louder, something is wrong. If it sounds like the same instrument in two different emotional states — that's the Contract.

**Engine-specific velocity behavior:**

- **ONSET (percussion)**: Velocity drives the physical impact simulation. A pp kick is the drum brushed; ff is struck at full force with a beater. The filter envelope is velocity-scaled — harder hits open the filter, producing more harmonic content from the drum body.

- **OPAL (granular)**: Velocity controls grain density and scatter. pp = sparse, slow-moving grains. ff = dense cloud of grains with increased scatter and modulation. The same underlying sample becomes a different textural phenomenon at each velocity extreme.

- **OVERDUB (tape delay)**: Velocity drives the input saturation stage. pp = clean signal entering the tape. ff = hot signal, driving tape saturation before the delay even fires. The delay trails respond to this — saturated input produces warmer, denser echoes.

- **OHM (communal drone)**: Velocity controls commune depth — how deeply the drone voices lock to each other. pp = independent, slightly detuned voices. ff = maximum coherence, voices merging. It's not louder, it's more unified.

---

## 2.8 MPC Stems Integration

*Jul 2024 feature — loading XO_OX packs alongside Stems*

MPC firmware 3.3 (released July 2024) added native Stems support — the ability to load Stems-format files directly onto MPC tracks and play individual stem components (drums, bass, melody, other) as separate playable elements. This changed the relationship between sample-based packs and full-track content in a significant way.

**What Stems support means for MPC:**

A Stems file (`.stem.mp4`) contains a full mixed track plus up to four isolated components. On MPC with Stems support, you can load a Stems file onto a track and independently control the volume of each component — mute the drums stem, bring the bass stem up, etc. This turns full-track music into live performance material without requiring you to have the original session files.

**How XO_OX packs work alongside Stems:**

XO_OX packs and Stems files are complementary, not competing. The workflow:

1. Load a Stems file onto Track 1 — a full-track reference with isolatable components
2. Mute the drums stem in the Stems track
3. Load an XO_OX ONSET drum pack onto Track 2
4. Play your live drum hits over the melody, bass, and other stems from the reference track

This gives you the backing musical context from a Stems file with the full expressive range of an XO_OX drum pack replacing the drums. The stems file becomes a minus-one track for your live kit performance.

**Tempo sync and grid alignment:**

MPC's Stems integration includes automatic tempo detection and warp/stretch — the Stems file will lock to your project BPM. XO_OX drum programs are designed at tempo-neutral positions (no tempo-locked samples in the core kit voices, only in the performance Bank D elements that are labeled with BPM). This means your XO_OX kit plays at any BPM without artifacts, while the Stems file stretches to match.

**Q-Link integration across Stems + Pack tracks:**

With a Stems file on Track 1 and a drum pack on Track 2, MPC's 8 Q-Link banks can be split:
- Q-Links 1–4 (Bank A) → Track 2 (drum pack): Tone, Space, Movement, Weight
- Q-Links 5–8 (Bank B) → Track 1 (Stems): drum stem volume, bass stem volume, melody stem volume, other stem volume

This gives you a coherent 8-knob mixing and performance surface across both your live drum kit and the Stems backing track — without touching the touchscreen.

> **Session setup tip**: Save a project template with a Stems placeholder on Track 1 and your favorite XO_OX drum pack on Track 2, Q-Links pre-assigned as above. Any time you start a Stems-based session, load this template and drop in your Stems file. The Q-Link map is already done.

**XO_OX "Stems Ready" packs:**

Starting in 2025, XO_OX packs marked `StemsReady: true` in their manifest include a pre-configured project template file alongside the programs. Load the template, and the track routing, Q-Link assignments, and default FX sends for Stems integration are already set up. Look for the `[S]` badge in the XO_OX pack browser to identify Stems-optimized packs.

**Upcoming: XO_OX Stems files:**

The next phase of XO_OX's MPC integration includes native Stems exports — not just drum packs, but full tracks produced using XOmnibus engines, released as Stems files with isolated XOmnibus engine components. A Stems release where the melody stem is OPAL granular synthesis, the bass stem is OBESE saturation engine, and the atmosphere stem is OHM communal drone — each stem independently playable and Q-Link addressable on MPC hardware. This format is in active development as of this writing.

---

## What You Now Know

By the end of this chapter, you have a working model of MPC's program architecture, understand how XO_OX packs are organized at every level from Collection down to Keygroup, can load and navigate a pack fluently on MPC Live III hardware, and understand the intentional design behind Q-Link assignments, pad banking, and velocity response.

More importantly, you understand the Velocity-Timbre Contract — the philosophy that separates XO_OX packs from the majority of expansion content. Velocity is not a volume dial. It is an expressive dimension. The difference between a producer who ignores this and one who uses it deliberately is the difference between a beat that works and a beat that breathes.

Chapter 3 goes deeper into the coupling dimension — how XOmnibus's cross-engine modulation system can be approximated in MPC's track architecture, and the specific Q-Link recipes that recreate the XOmnibus sound design environment on hardware.

---

> **Quick Reference: Chapter 2 Key Terms**
>
> **Program** — Self-contained instrument patch (samples + envelopes + FX + Q-Links). The atomic sound unit.
>
> **Keygroup Program** — Pitched instrument spread across keyboard zones.
>
> **Drum Program** — Percussive kit of up to 64 pads across 4 banks.
>
> **Track** — One program, one MIDI channel, in a project.
>
> **Sequence** — One section of your composition (intro, chorus, etc.).
>
> **Pad Bank** — One of four 16-pad configurations per track (A/B/C/D = 64 pads total).
>
> **Q-Link** — Physical knob mapped to a program parameter. XO_OX standard: Q1=Tone, Q2=Space, Q3=Movement, Q4=Weight.
>
> **Velocity-Timbre Contract** — XO_OX design doctrine: velocity must shape timbre, not just amplitude. Every layer is a genuinely different sound, not just a volume adjustment.
>
> **Stems** — MPC 3.3+ feature: full-track audio files with isolated instrument stems.
>
> **StemsReady** — XO_OX manifest flag indicating a pack includes a pre-configured Stems integration template.

---

*Kai — MPC workflow specialist, XO_OX Designs*
*"The machine rewards producers who learn it. What I'm trying to do is shorten the time between getting the hardware and knowing the instrument."*
