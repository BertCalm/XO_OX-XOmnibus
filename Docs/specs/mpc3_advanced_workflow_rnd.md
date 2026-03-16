# MPC OS 3.x Advanced Workflow R&D
## XO_OX Pack & Oxport Optimization Reference

*Compiled: 2026-03-16 | Target: MPC OS 3.x (3.0–3.3)*

---

## 1. Q-Link Assignments in XPN

**How it works technically**

Q-Links in MPC 3.x are 16 assignable encoders/faders grouped into 4 banks (Q1–Q16). In Program Edit mode, each Q-Link can be assigned to any modulatable parameter on any pad: volume, pan, tuning, filter cutoff/resonance, envelope ADSR stages, sample start/end, LFO rate/depth, and plugin insert parameters. Assignments live inside the `.xpm` file under `<QLink>` elements — each entry specifies the Q-Link index (1–16), the target pad number or "ALL PADS", the parameter ID string, and the control range (min/max, bipolar flag).

**What Oxport should do**

Export a sensible default Q-Link map on every drum kit XPM:
- Q1–Q8 → per-voice Filter Cutoff (pads 1–8, range 0–127)
- Q9–Q12 → Attack, Decay, Sustain, Release on "ALL PADS" (global feel shaping)
- Q13 → Tune ALL PADS (±24 semitones, bipolar)
- Q14 → Sample Start ALL PADS (0–100%, for glitch/chop)
- Q15–Q16 → Send FX levels (reverb send, delay send)

This layout matches the physical top-row/bottom-row hand position on MPC X/Live hardware. Melodic keygroup XPMs should swap Q1–Q8 to individual-voice tune for performance detuning.

**Known quirks**

Q-Link assignments saved in XPM are silently ignored if the target parameter ID string doesn't match the firmware version's internal name. MPC 3.2 renamed several envelope parameter IDs — test exports on both 3.1 and 3.2 targets. Plugin insert Q-Link assignments require the plugin to be present; if the AIR plugin is unlicensed the assignment loads but controls nothing.

---

## 2. Pad Performance Modes

**How they work**

- **Note Repeat**: Gates incoming note at a subdivided rate while pad is held. Velocity follows pad pressure (aftertouch on MPC X) or locks to the last-struck velocity depending on the "Latch" setting.
- **16 Levels**: Remaps pads 1–16 to 16 fixed velocity steps (typically 8, 16, 24… 127) of the last-struck pad. All 16 trigger the same pad/sample.
- **Full Level**: Forces every pad to 127 regardless of strike force.

**Interaction with XPM velocity layers**

16 Levels exposes the full velocity layer stack — a kit with 4 velocity layers (pp/mp/mf/ff) will step through them evenly across the 16 pads. Design velocity breakpoints at 32/64/96/127 so 16 Levels lands meaningfully on each layer boundary rather than clustering in the middle. Note Repeat interacts with Release time: short releases on hat voices create clean rhythmic gates; long releases on kicks smear at fast subdivisions. Set default release on kick/snare ≤ 500ms for usable Note Repeat behavior.

Full Level is the default live performance mode — every layer-switching decision must sound intentional at 127. The loudest velocity layer is the face of the pack.

---

## 3. Plugin Chain in Standalone

**Available AIR plugins (MPC 3.x standalone)**

Bundled (no license required): Tube Amp, Vintage (EQ/compressor suite), Flavor (lo-fi/bit-crush), Hype (harmonics exciter). Licensed (separate purchase or MPC Software bundle): Solina (string ensemble), Electric (stage piano), Bassline, TubeSynth, Drum Synth.

**Pack design approach**

XPN packs cannot embed plugin chains — the XPM format supports insert FX references by plugin ID string, but standalone MPC will skip missing plugins silently. The safe strategy: use only bundled AIR inserts in factory XPM insert slots. For Tube Amp and Vintage, bake recommended presets into the XPM insert XML so users get the intended color on load. Document suggested licensed plugin chains in the pack's README/liner notes rather than hardcoding them, so the kit loads cleanly on all units.

---

## 4. Ableton Link + Clock Sync

**How it works**

MPC 3.x joins an Ableton Link session over WiFi or USB-C Ethernet. BPM and beat phase sync across peers; MPC becomes a Link peer, not a MIDI clock slave. LFO tempo sync and arp subdivisions in MPC programs follow the internal BPM, which Link keeps aligned.

**Oxport metadata**

Bake a `<Tempo>` element in the XPM/XPN program header reflecting the pack's intended BPM (or a representative mid-range BPM for genre-flexible packs). When MPC loads the program, it does not auto-set the session BPM from this value — it's advisory only. However, tempo-sync LFO rates stored as division values (1/4, 1/8T, etc.) resolve correctly regardless of host BPM, so always store LFO rates as named subdivisions rather than raw Hz values. This makes the pack Link-transparent by design.

---

## 5. MIDI Program Change for Live Set Switching

**How MPC handles it**

In Standalone, MIDI PC messages on the configured receive channel cycle through programs in the current project in program-list order (PC 0 = program slot 1, PC 127 = slot 128). Programs load with a small buffer gap (~100–200ms on MPC X). Pads assigned to "Program" mode inherit the newly loaded program immediately.

**XPN/XPM structure**

Order programs in the XPN bundle in setlist sequence. `setlist_builder.py` should emit a `<ProgramOrder>` manifest matching the intended PC map so the exported project's program list index is deterministic. Avoid program names that sort differently alphabetically vs. intended PC order — use zero-padded numeric prefixes (`01_Intro`, `02_Drop`) to lock sort order.

**Known quirk**

MPC 3.2 introduced a "PC Latch" option that delays program switch to the next bar downbeat — useful for live use but catches users off guard. Pack documentation should mention this setting.

---

## 6. Multi-Track Export from MPC Standalone

**Limitations**

MPC standalone has no individual stem export. The workaround is "Track Mute" bounce: solo one pad/track, export audio, repeat. This is tedious for 16-pad kits. A faster path: use the "Pad Perform" → "Pad Mute Groups" to assign all non-target pads to a dedicated mute group, then record a full loop pass per voice.

**Kit structure for easiest bouncing**

Assign each drum voice to its own MIDI track in the sequence (kick on Track 1, snare on Track 2, etc.) during XPN construction. Oxport's `drum_export.py` should already emit one track per voice — verify `<MIDITrack>` index matches pad number. With one voice per MIDI track, users can solo tracks in the sequencer view and bounce to audio without manual mute gymnastics. Name tracks with the voice label ("KICK", "SNARE_TOP") so the MPC track list is self-documenting.

**Firmware note**

MPC 3.3 added "Export Stems" in the Save menu for sequences — it bounces each MIDI track to a separate audio file automatically. This is the target workflow. Structure XPN sequences with one-voice-per-track so the 3.3 Export Stems path works without any user reorganization.

---

*Reference firmware: MPC OS 3.3.x | Hardware targets: MPC X, MPC Live II, MPC One+*
