# XO_OX MPC Workflow Guide — Research Spec
*Ships as WORKFLOW_GUIDE.md inside every XPN pack*

---

## WORKFLOW_GUIDE.md

**XO_OX Expansion Pack — MPC Workflow Guide**
*For MPC Live III / MPC X / MPC One / Key 61 running firmware 3.x+*

---

### 1. Loading Your XO_OX Expansion

**Via USB drive**
Copy the `.xpn` file to the root of a FAT32-formatted USB drive. Insert into MPC, open the **Expansion Browser** (MENU → Expansions), and select your drive from the location dropdown. Tap the pack name to install — MPC copies samples to internal storage automatically.

**Via direct download to internal storage**
Transfer the `.xpn` file to `/Expansions/` on MPC internal storage using MPC Software connected over USB. On MPC standalone, browse to the file via **File** → **Import Expansion** and confirm install.

**Finding your pack**
After install, go to MENU → Expansions → browse by pack name. Each XO_OX expansion appears as a single entry. Tap to open.

**What's inside**
Every XO_OX pack ships with up to 4 programs:

| Program Type | Description |
|---|---|
| Drum Kit | 16-pad layout — one voice per pad, velocity-layered |
| Keygroup — Melodic | Pitched instrument across full keyboard range |
| Keygroup — Bass | Low-register character voice, often mono |
| Keygroup — Texture | Atmospheric/pad voice, sustain-focused |

Not every pack ships all four. Check the pack README for the specific contents.

---

### 2. Understanding the Q-Links

XO_OX packs assign all four Q-Links to macros in this fixed order:

| Q-Link | Macro | What It Does |
|---|---|---|
| Q1 | CHARACTER | Core voice character — feliX↔Oscar sweep |
| Q2 | MOVEMENT | Internal motion, animation, modulation rate |
| Q3 | COUPLING | Interaction between layered elements |
| Q4 | SPACE | Reverb/delay send depth |

**Reading Q-Link labels on MPC**
In Program Edit, scroll to the Q-Link section — labels display as 8-character truncations (e.g., `CHARACTR`, `MOVEMENT`). On Key 61, the OLED strip shows the full label when a Q-Link is touched.

**The feliX↔Oscar principle**
Q1 (CHARACTER) is the defining sweep of every XO_OX voice. Turned fully left (0) is feliX — clean, bright, precise. Turned fully right (127) is Oscar — warm, saturated, dark. This is not a tone knob. It changes the fundamental character of the sound. Find your zone, then leave it.

**Live performance automation**
Hold any pad and sweep a Q-Link while recording — MPC writes per-pad CC automation into the sequence. For macro sweeps mid-bar, enter Record mode, arm the Q-Link track, and automate freely. To lock a Q-Link value per pattern, use **Q-Link Learn** to snapshot the position before recording begins.

---

### 3. Velocity Layers

XO_OX drum programs use a 4-layer canonical curve mapped to standard dynamic zones:

| Layer | Velocity Range | Feel |
|---|---|---|
| pp | 1–40 | Ghost hit, brush |
| mp | 41–80 | Controlled hit |
| f | 81–110 | Full hit |
| ff | 111–127 | Accent, slam |

**Dialing pad sensitivity on MPC Live III**
Go to MENU → Preferences → Pad Sensitivity. XO_OX packs are calibrated for **medium** sensitivity (level 4–5). If ghost notes are not triggering, lower the sensitivity floor. If accents compress into the f layer, raise the sensitivity ceiling.

**Hitting multiple layers intentionally**
Enable NOTE REPEAT, set to 1/16 or 1/32, and hold a pad with light pressure. Gradually increase strike force across the repeat cycle — the layer transitions become audible as a swell. This is the fastest way to audition all four layers on a single voice without building a pattern.

---

### 4. Using Entangled / Coupling Presets

Some XO_OX packs include **Entangled** programs — presets that use the COUPLING macro to route one engine's output as a modulation source into another. These require **XOlokun** running as a plugin.

**Standalone MPC (no DAW)**
XOlokun cannot run natively on MPC standalone. Entangled presets in standalone mode will function as standard programs — the COUPLING macro affects internal modulation only, not engine-to-engine routing.

**Plugin mode (MPC connected to DAW)**
1. Open MPC Software in **Plugin** mode inside your DAW (Ableton, Logic, etc.).
2. Load XOlokun on a separate instrument track. Set its MIDI input to the MPC's virtual MIDI output port.
3. On the MPC pad bank, assign the Entangled program pads to send MIDI to that port.
4. Route XOlokun audio output back into the DAW mixer or directly into the MPC via the audio input if using hardware loopback.
5. COUPLING (Q3) now controls the live engine-to-engine interaction in real time.

This setup is most practical for studio sessions. For live use without a DAW, treat Entangled programs as standard programs.

---

### 5. Performance Techniques

**Bank switching mid-performance**
XO_OX packs organize voices across Banks A–D. Bank A is the primary kit. Banks B and C hold alternate round-robin versions or variation voices. Bank D is reserved for texture/FX. Switch banks mid-performance using the BANK buttons — current pads finish their playback before the new bank activates.

**Choke groups for hihat patterns**
Open and closed hats in XO_OX drum programs share Choke Group 1 by default. The closed hat chokes the open hat on hit — standard 909 behavior. To override (let open hat ring through), go to Program Edit → pad → Choke Group → set to None.

**Q-Link automation recording**
Arm a sequence for recording. Before pressing Play+Record, set your Q-Link starting positions. MPC records all Q-Link movements as CC automation per channel. To correct a sweep, use **Step Sequencer** → select the automation lane → draw or erase nodes manually.

**Layering multiple XO_OX programs**
Create a new Track per program. Assign each track to a separate pad bank or MIDI channel. Use the MPC's internal mixer to balance levels. When layering a Drum Kit with a Keygroup Texture program, route them to separate Sub Mixes for independent FX processing.

---

### 6. Exporting to DAW

**Bounce to audio**
In MPC standalone, go to MENU → Export → Bounce Song/Section. Select stems per track for the cleanest separation. File format: 24-bit WAV, 44.1kHz or 48kHz — match your DAW project sample rate.

**Live recording from MPC audio output**
Connect MPC Main Out (L/R or individual outs) to your audio interface. Record directly into DAW as audio. For stems, use MPC's individual outputs — assign each drum voice to a dedicated output in Program Edit → Output. XO_OX drum programs label outputs by voice name.

**MIDI file export and DAW reimport with XOlokun**
In MPC, go to File → Export → MIDI. Select the sequence or pattern. In your DAW, import the MIDI file and route it to XOlokun on an instrument track. The MIDI note layout from XO_OX drum programs maps directly to XOlokun's drum engine pad layout — no remapping needed.

**Stem export workflow summary**
1. MPC → Export → Individual Tracks (WAV stems)
2. Import stems into DAW
3. Import corresponding MIDI from MPC
4. Load XOlokun on a parallel instrument track for live plugin layering over the rendered stems
5. Automate XOlokun Q-Links (CHARACTER/MOVEMENT/COUPLING/SPACE) via DAW automation lanes for final mix movement

---

*XO_OX — feliX & Oscar. Every pack, both polarities.*
