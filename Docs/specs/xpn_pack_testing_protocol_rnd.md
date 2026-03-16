# XPN Pack Hardware Testing Protocol
## Human-Ears, Hardware-on-Floor QA Before Release

**Date**: 2026-03-16
**Scope**: Systematic hardware testing of XO_OX .xpn packs on physical MPC devices prior to v1.0.0 release gate

---

## Overview

Automated tools (oxport.py validation, XML schema checks, velocity curve inspection) catch structural defects. They cannot catch:

- A sample that renders differently at 44.1 kHz vs 48 kHz
- A Q-Link that technically moves a parameter but produces no audible change
- A pad that sounds clipped at velocity 127 because the rendered file peaked at -0.2 dBFS after limiting
- A choke group that works in Standalone but silently breaks in MPC OS 3.5
- A frequency cluster that sounds fine in isolation but causes phase cancellation when folded to mono

This protocol is the human-ears pass. It runs after automated QA passes and before the pack is signed and distributed.

---

## 1. Testing Hardware Setup

### Primary Test Device: MPC Live III (MPCe)

The MPCe is the primary device because:
- It is the current flagship MPC at time of writing (MPC OS 3.5+)
- It has the 3D pad surface (pressure + XY position) — the most demanding test for MPCe-native packs
- Its internal audio interface runs at 44.1 kHz or 48 kHz selectable — test both for packs that contain pitched content
- Standalone mode: no host DAW assumptions, no plugin bridging, no CPU sharing

**MPCe test rig checklist:**
- [ ] MPC OS version noted in test log (e.g., `MPC OS 3.5.1`)
- [ ] Audio output: balanced TRS → studio monitors (preferred) or USB audio to headphone amp
- [ ] Sample rate set to 44100 Hz for first pass (most XO_OX packs render at 44.1 kHz)
- [ ] Internal storage or fast SD card (not USB drive) — slow storage causes load stutters that mask real bugs
- [ ] No active projects loaded when importing — fresh state

### Secondary Test Device: MPC One

Run the full pad test protocol (Section 3) on MPC One after MPCe passes. Reasons:

- MPC One ships to more users than MPCe — it is the practical compatibility floor
- MPC One has no 3D pad surface — confirms the pack degrades gracefully (no broken pads when 3D features are absent)
- Different internal audio hardware — catches sample rate or audio routing edge cases

**MPC One checklist:**
- [ ] Same MPC OS version as MPCe where possible
- [ ] Confirm all programs load and pads produce sound
- [ ] Skip Section 5 (3D pad test) — not applicable

### Headphones vs Monitors

| Scenario | Recommended |
|----------|-------------|
| Initial pad check (do pads make sound) | Either |
| Velocity dynamics test | Studio monitors — low-frequency detail is compressed in headphones |
| Choke group test | Either |
| Mix balance + mono compatibility | Studio monitors into room — reveals low-end buildup |
| Q-Link zipper noise | Headphones — reveals artifacts masked by room acoustics |
| 3D pad morphing | Headphones — continuous timbral movement is clearer |

### Sample Rate Verification

Packs that contain pitched melodic content (keygroup programs) must be verified at 44100 Hz.

**Why 44100 Hz matters:** If a sample was rendered at 44100 Hz but the MPC is set to 48000 Hz, the MPC resamples internally. Most programs use linear interpolation — the pitch will be accurate but the timbre will change (HF roll-off, aliasing on transients). For drum packs the effect is subtle. For harmonic content it can be destructive.

**Test procedure:**
1. In MPC OS: `Menu → Settings → Audio → Sample Rate → 44100`
2. Load the pack program
3. Play a melodic line or chord — confirm it sounds identical to the reference DAW render
4. Repeat at 48000 Hz and note any timbral difference in the test log

---

## 2. Installation Test

### Import Procedure

1. Transfer the `.xpn` ZIP to a USB drive or the MPC's internal storage
2. On MPC OS: `Menu → Expansions → Import Expansion`
3. Select the `.xpn` file and confirm import

**Pass criteria:**
- [ ] Pack appears in the Expansions browser under the correct name
- [ ] Pack thumbnail/artwork is visible (not a broken image placeholder)
- [ ] All programs are listed under the pack (count must match manifest)
- [ ] No error dialogs during import

### Program Load Test

For each program in the pack:
1. Tap the program name to load it
2. Wait for all samples to load (spinner must disappear)
3. Hit any pad

**Pass criteria:**
- [ ] Program loads without error dialog
- [ ] No "missing sample" warnings in the pad grid
- [ ] At least one pad produces sound immediately

### Fresh MPC Environment Test

Perform one full import on an MPC that has not had the pack installed before — ideally a device that has never had any XO_OX content. This catches:

- Absolute path assumptions in sample references (a common XPM bug)
- Dependencies on other packs being present (XPN packs must be self-contained)
- File naming collisions with factory content

If a fresh device is not available, use the MPC's `Restore Factory Settings` option before testing. Note in the test log that this was a factory-restore test, not a genuinely fresh device.

---

## 3. Pad Test Protocol (Per Program)

Run this protocol on every program in the pack. For packs with more than 12 programs, test all programs in the flagship kit category and a random sample of at least 3 from each other category.

### 3.1 Soft Velocity Test (velocity ≤ 30)

Set MPC pad sensitivity to a fixed low value or use the velocity lock feature. Hit every pad at soft velocity (≤30).

**Pass criteria:**
- [ ] Every pad that is expected to produce sound does produce sound — no silent pads at soft velocity
- [ ] Sound is audible without straining to hear it (soft velocity should not be inaudible)
- [ ] The timbre at soft velocity is distinctly different from hard velocity (D001 — velocity shapes timbre)

**Common failures:** Velocity curve mapped too aggressively so that ≤30 falls below the sample's threshold; filter fully closed at low velocity with no audible tone.

### 3.2 Hard Velocity Test (velocity ≥ 110)

Hit every pad at hard velocity (≥110), ideally at full 127.

**Pass criteria:**
- [ ] No clipping or digital distortion on any pad
- [ ] No clicks or pops on attack transients
- [ ] Hard velocity sounds louder and/or brighter than soft velocity — not just louder
- [ ] No pads that are silent at hard velocity (can happen if velocity curve is inverted)

### 3.3 Hat Choke Group Test

Applies to any program with both open and closed hi-hat variants.

1. Hold the open hat pad — let it ring sustaining
2. While it is ringing, hit the closed hat pad
3. The open hat must cut off immediately

**Pass criteria:**
- [ ] Open hat stops playing when closed hat is triggered
- [ ] Closed hat sounds normally (choke does not silence the triggering pad)
- [ ] Test the reverse: hold closed hat, trigger open hat — open hat should also choke closed hat (symmetric MuteGroup behavior in MPC)

**Common failures:** MuteGroup set to 0 (disabled) by accident in XPM; pads in the same choke group by number but different programs loaded as layers.

### 3.4 Sustain Test (Melodic / Long Samples)

For any program with melodic content or long atmospheric samples:

1. Hit a pad
2. Keep the pad held
3. Verify the sample sustains (does not release immediately)
4. Release the pad — sample should decay naturally or stop based on program design

**Pass criteria:**
- [ ] Held pad sustains at minimum 2 seconds without fading to silence unless that is the intended design
- [ ] Release behavior matches the sonic intent (plucked instruments: release quickly; pads: gentle tail)

---

## 4. Q-Link Test

Load each program. Rotate all 4 Q-Links from minimum to maximum and back.

### Per Q-Link:
- [ ] Audible change at some point in the rotation range — a Q-Link that produces no sound change is broken
- [ ] No zipper noise (stairstepped volume jumps or pitch quantization artifacts)
- [ ] No dropout or silence mid-rotation
- [ ] Change is musically useful — not just a tiny HPF shift that only affects 20 kHz content
- [ ] Extreme positions (fully left, fully right) do not silence the instrument unless that is intentional (e.g., a wet/dry mix at 0%)

**Zipper noise test:** Rotate quickly while holding a sustained pad. Smooth parameter modulation should sound smooth. Stairstepping indicates the CC resolution is too coarse or the parameter response curve is too aggressive.

**Common failures:** Q-Link assigned to a parameter that was removed or renamed in a later engine build; CC assignment conflicts with another Q-Link.

---

## 5. MPCe 3D Pad Test

Applies to packs explicitly designed for MPCe 3D pad surface. If the pack is not MPCe-native, skip this section and note it in the test log.

### 5.1 Corner-to-Corner Slide

1. Touch a pad at the top-left corner
2. Slowly drag the finger to the bottom-right corner
3. Repeat all 4 corner-to-corner diagonals

**Pass criteria:**
- [ ] Timbre morphs continuously and smoothly across the movement
- [ ] No abrupt jumps or silent zones in the middle of the pad
- [ ] Each corner has a distinct timbral character (the 4 corners should not all sound the same)

### 5.2 Z-Axis Pressure Test

1. Touch a pad lightly — verify sound starts or changes
2. Gradually increase finger pressure without moving XY position
3. Verify the pressure axis produces a distinct modulation (not the same as velocity)

**Pass criteria:**
- [ ] Pressure (Z-axis) produces audible change independent of XY position
- [ ] No sudden jumps at pressure extremes
- [ ] Full pressure range (minimum to maximum) produces a useful modulation range — not a 2% filter shift

### 5.3 All 4 Corner Positions

At rest on a held pad, deliberately position finger at each of the 4 corners in sequence.

**Pass criteria:**
- [ ] All 4 corners produce distinct sounds
- [ ] No corner is silent or broken
- [ ] Transitions between corners are smooth

---

## 6. Mix Balance Test

Select the 4 most tonally rich programs from the pack (not the most similar — pick programs that span the pack's sonic range). Load them into a single MPC project across 4 tracks.

### 6.1 Frequency Masking Check

Play all 4 programs simultaneously. Use a simple 4-bar loop.

**Pass criteria:**
- [ ] Each program is audible in the mix without being masked by another
- [ ] No two programs share the same dominant frequency band to the point where one disappears
- [ ] The total mix does not feel congested or muddy in the 200–800 Hz range (common problem with rich pads + kick-heavy drums)

If masking is found: note which programs and what frequency range. This is a sound design note, not necessarily a bug, but must be logged.

### 6.2 DC Offset Check

On MPC OS: `Menu → Sample Edit → Analysis → Waveform` on any sample from the pack. Visually inspect the waveform center line.

**Pass criteria:**
- [ ] Waveform is centered around zero — no visible vertical offset
- [ ] If using a DAW for this check: DC offset meter reads 0.0 dB or not measurable

### 6.3 Mono Compatibility Check

Sum the stereo output to mono (on MPC: use the mono output bus, or fold to mono in a connected DAW).

**Pass criteria:**
- [ ] Each individual program sounds recognizable in mono
- [ ] No program loses more than 6 dB of perceived level when folded to mono
- [ ] No complete frequency nulls (a sound that nearly disappears in mono has phase cancellation in its sample or in its stereo FX chain)

**Common failures:** Wide stereo samples with hard-panned duplicates 180 degrees out of phase; chorus/flanger effects producing stereo phase inversion.

---

## 7. Edge Case Tests

### 7.1 Rapid Successive Triggers

Hit a single pad 16 times as fast as possible (use a finger roll or a drum machine trigger).

**Pass criteria:**
- [ ] No stuck notes (notes that continue playing after all triggers stop)
- [ ] No crashes or audio engine dropouts
- [ ] Polyphony behavior is as designed — either voice stealing kicks in cleanly or polyphony limit is hit without artifacts

### 7.2 Maximum Polyphony Test

Trigger 16 different pads simultaneously (use both hands or set up a 16-pad MIDI trigger grid).

**Pass criteria:**
- [ ] MPC handles 16-voice simultaneous trigger without dropout
- [ ] Voice stealing (if it occurs) is audible but not catastrophic — no sudden silence or distortion
- [ ] Program returns to normal behavior after the polyphony peak (no latent audio engine state corruption)

### 7.3 BPM Sync Test

For any program with rhythmic content (arpeggiated samples, gated sequences, tempo-synced LFOs in XPN keygroup programs):

1. Set MPC BPM to 120
2. Trigger the rhythmic program and let it play 4 bars
3. Change BPM to 90, then to 140 — do this while the program is playing

**Pass criteria:**
- [ ] Rhythmic content locks to each new BPM within 1 bar of the change
- [ ] No hung notes or rhythmic drift after a BPM change
- [ ] Extreme BPM values (60 BPM and 180 BPM) do not cause the rhythmic program to stutter or dropout

If the program has no rhythmic content, mark this test N/A.

---

## 8. Documentation: PACK_TEST_RESULTS.md

Every pack that goes through this protocol must produce a completed `PACK_TEST_RESULTS.md` file. This file is required before the v1.0.0 release gate.

### File Location

```
Tools/test-results/{PACK_NAME}_PACK_TEST_RESULTS.md
```

### Template

```markdown
# {PACK_NAME} — Hardware Test Results

**Date**: YYYY-MM-DD
**Tester**: [name or initials]
**Pack version**: v{X.Y.Z}
**Primary device**: MPC Live III (MPCe) — MPC OS {version}
**Secondary device**: MPC One — MPC OS {version}
**Monitors / headphones**: [model]

---

## 1. Installation Test
- [ ] Pack appears in Expansion browser
- [ ] Artwork loads correctly
- [ ] All {N} programs listed
- [ ] No import errors
- [ ] Fresh-device test: PASS / FAIL / SKIPPED

## 2. Pad Test Results

| Program | Soft Vel | Hard Vel | Choke | Sustain | Notes |
|---------|----------|----------|-------|---------|-------|
| {Program 1} | PASS | PASS | N/A | N/A | |
| {Program 2} | PASS | PASS | PASS | N/A | |
| ... | | | | | |

## 3. Q-Link Test

| Program | Q1 | Q2 | Q3 | Q4 | Notes |
|---------|----|----|----|----|-------|
| {Program 1} | PASS | PASS | PASS | PASS | |
| ... | | | | | |

## 4. 3D Pad Test (MPCe only)
- [ ] Corner-to-corner slide: PASS / FAIL / N/A
- [ ] Z-axis pressure: PASS / FAIL / N/A
- [ ] All 4 corners distinct: PASS / FAIL / N/A

## 5. Mix Balance Test
- [ ] Frequency masking: PASS / ISSUES NOTED
- [ ] DC offset: PASS / FAIL
- [ ] Mono compatibility: PASS / FAIL
- Notes: {observations}

## 6. Edge Case Tests
- [ ] Rapid triggers: PASS / FAIL
- [ ] Max polyphony: PASS / FAIL
- [ ] BPM sync: PASS / FAIL / N/A

## 7. Sample Rate Tests
- 44100 Hz: PASS / FAIL
- 48000 Hz: PASS / FAIL / N/A

---

## Issues Found

| ID | Severity | Description | Status |
|----|----------|-------------|--------|
| 001 | [BLOCK/WARN/NOTE] | {description} | OPEN / FIXED / WONTFIX |

## Overall Verdict

- [ ] READY FOR RELEASE — all blocking issues resolved
- [ ] BLOCKED — see issues table above

**Signed off by**: [name]
**Sign-off date**: YYYY-MM-DD
```

### Severity Definitions

| Severity | Meaning | Release gate |
|----------|---------|-------------|
| BLOCK | Pack cannot ship with this issue — silent pad, crash, clipping, broken choke | Must be fixed before release |
| WARN | Audible problem but not catastrophic — mild masking, borderline zipper noise | Document and decide; prefer fix |
| NOTE | Observation for future improvement — timbral suggestion, not a bug | Log for next version |

---

## Release Gate Summary

A pack is cleared for v1.0.0 release when all of the following are true:

1. `PACK_TEST_RESULTS.md` exists in `Tools/test-results/` and is filled out completely
2. Zero BLOCK-severity issues remain open
3. Both MPCe and MPC One installation tests have PASS status
4. Mono compatibility check is PASS
5. At least one tester has signed off with name and date

Packs with open WARN issues may ship at the judgment of the signing tester, with a note in the issue tracker for the v1.1.0 backlog.
