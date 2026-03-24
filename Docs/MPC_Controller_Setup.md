# MPC Controller Setup for XOlokun

Use any MPC as a hands-on controller for XOlokun in your DAW.

---

## Quick Setup (3 Steps)

1. **Connect** your MPC to your computer via USB.
2. **Set MPC to Controller Mode** — on the MPC, go to Menu > MIDI/Sync and set the MPC to send MIDI over USB. On MPC software units (One, Live, Key), enter Controller Mode from the main menu.
3. **Load XOlokun** in your DAW on a track receiving MIDI from the MPC.

That's it. Pads send notes, pitch bend works, and you can start playing immediately.

---

## Recommended MIDI Mapping

### What works out of the box (no setup needed)

| MPC Control | XOlokun Target | Notes |
|---|---|---|
| Pads | MIDI notes | Pads trigger voices — default behavior |
| Pitch bend | Pitch | Standard pitch wheel / strip |
| Mod wheel (Key 61) | Mod wheel | Routed per-engine |

### Q-Link mapping (set up once)

Assign your Q-Links to these CCs in the MPC's MIDI control editor:

| Q-Link | CC | XOlokun Macro | What it does |
|---|---|---|---|
| Q1 | CC 1 | CHARACTER | Sweeps the engine's tonal identity |
| Q2 | CC 2 | MOVEMENT | Controls motion, modulation speed |
| Q3 | CC 3 | COUPLING | Blends between coupled engine pairs |
| Q4 | CC 4 | SPACE | Adjusts reverb, room, atmosphere |

**Q-Links 5-8** (if available) — assign to CC 5-8 for engine-specific parameters. Each engine documents its own secondary params.

### Aftertouch

MPC pads send channel aftertouch. Map this to filter modulation in your DAW's MIDI learn or XOlokun host:

| Source | Suggested Target |
|---|---|
| Aftertouch | Filter cutoff / expression |

---

## MPCe 3D Pad Mapping (Live III / XL)

The MPCe pads sense X, Y, and Z (pressure) independently per pad. Map these axes for expressive control:

| Pad Axis | XOlokun Target | CC |
|---|---|---|
| X (left-right) | CHARACTER macro | CC 1 |
| Y (top-bottom) | MOVEMENT macro | CC 2 |
| Z (pressure) | Expression / filter cutoff | CC 11 or aftertouch |

This turns each pad into a 3D performance surface — slide your finger to morph the sound in real time.

---

## Per-Hardware Notes

### MPC One / MPC One+
- 4 Q-Links (knobs) — map to the 4 macros above
- 16 velocity-sensitive pads
- No pitch/mod wheels — use Q-Links or DAW automation

### MPC Live II / MPC Live III
- 4 Q-Links (knobs)
- 16 velocity-sensitive pads
- Live III adds 3D pad sensing (see MPCe section above)
- Touchscreen for quick CC assignment

### MPC XL
- 16 Q-Links with OLED displays — map Q1-4 to macros, Q5-8 to engine params, Q9-16 to FX or additional engine controls
- 3D pad sensing on all 16 pads
- The extra Q-Links make this the best XOlokun controller in the lineup

### MPC Key 61
- Pitch and mod wheels built in — both work immediately
- 4 Q-Links (knobs) — map to macros
- 61 keys for melodic engines, pads for drums/triggers
- Best choice for playing melodic XOlokun engines

---

## MIDI Learn Quick Reference

### In your DAW
Most DAWs support MIDI learn on plugin parameters:

1. Right-click (or Ctrl-click) the XOlokun knob you want to control
2. Select "MIDI Learn" or "Learn MIDI CC"
3. Move the Q-Link or pad on your MPC
4. Done — the assignment is saved with your project

### Common DAW shortcuts
- **Ableton Live**: Click MIDI button (top right) > click parameter > twist knob
- **Logic Pro**: Cmd+L > move controller
- **FL Studio**: Right-click knob > Link to Controller > twist knob
- **MPC Software (standalone)**: Edit Program > Pad Assign for note mapping; MIDI Learn in plugin window for Q-Links

### Tip
Save your Q-Link template on the MPC so you don't have to reassign CCs every session. Name it something like "XOlokun 4 Macros" and load it whenever you open a session.
