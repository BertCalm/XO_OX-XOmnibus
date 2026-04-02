# XPN Pack Format Decision: Sample Library vs Keygroup vs Drum Program

**R&D Topic**: When to use which MPC program type for XO_OX XPN packs — and what "Sample Library" actually means in the MPC context.

---

## 1. XPN Program Types — What MPC Actually Supports

MPC XPN packs can contain six program types. Each has a distinct identity in the browser and a distinct use case.

**Drum Program** — 64 pads mapped to MIDI notes 35–98. Each pad holds one or more samples with velocity layers, choke groups, and round-robin pools. This is the canonical home for drums, one-shots, loops, and percussive sound effects. The MPC browser surfaces these under "Drums."

**Keygroup Program** — N instruments, each spanning a pitch range, with up to 4 velocity layers per note zone. Each zone has its own filter, amp envelope, and LFO. Plays chromatically from a keyboard or pads. Surfaces in the browser under "Instruments" or "Keys."

**MIDI Program** — Sends MIDI to an external device or plugin. Not relevant for sample-based XO_OX packs.

**Plugin Program** — Hosts an MPC-native VST/AU. Relevant for the future XOceanus MPC plugin path but not for sample XPN packs.

**CV Program** — Eurorack CV/gate output. Not relevant here.

**Sample Library** — This is the critical one to clarify (see Section 4).

---

## 2. Keygroup Format Deep Dive

A Keygroup program is a multi-zone instrument. Each "keygroup" defines a pitch range, a root note, and up to 4 velocity layers within that range. Each layer points to a WAV file.

Per-zone controls: filter cutoff/resonance, amp envelope (ADSR), pitch envelope, LFO. These run at the program level in simpler setups, but MPC allows per-layer overrides.

**What makes a great keygroup:**
- Root notes set correctly so pitch scaling sounds natural across the keyboard
- Velocity layers that feel like one continuous instrument, not three distinct timbres bolted together
- Release tails that decay naturally without clicks (WAV files with proper fade-outs)
- At minimum 3 velocity layers for any expressive melodic instrument; 2 layers is marginal

**When keygroup is the right choice:** Any pitched instrument meant to be played chromatically. Bass lines, leads, pads, plucks, mallets, bowed strings, winds. If the user will play it on a keyboard or roll notes in the piano roll, it goes Keygroup.

---

## 3. Drum Program Deep Dive

64 pads. Each pad holds samples in velocity groups (soft/medium/hard thresholds set manually) and a sample pool for round-robin cycling. Choke groups mute related pads (e.g., open hi-hat choked by closed hat). Note assignment is fixed per pad.

**When drum program is right:** Anything the user will trigger by hitting a pad, not playing a melody. Drums obviously. One-shot hits, stingers, texture bursts, foley, loops. Even a "bass hit" sample pack is better served as a drum program if the user isn't playing it chromatically.

---

## 4. "Sample Library" in MPC — The Real Answer

"Sample Library" is **not a program type in the MPC XPN spec.** It is a browser category label, not a program class.

When Akai and third-party pack makers use the term "Sample Library," they typically mean one of two things:

1. **A folder-based WAV collection** inside an XPN with no program file at all — just organized raw samples. MPC displays these under "Samples" in the browser. The user loads them manually into whatever program they build.

2. **A collection XPN with multiple programs of mixed types** — drums + keygroups + atmospheric pads all in one pack. The browser shows the pack, and the user drills into sub-folders to find the programs.

There is no `<ProgramType>SampleLibrary</ProgramType>` field in MPC XML. If your XPN tooling generates a "Sample Library" type, it is likely creating a Keygroup program with wide zone ranges, or simply bundling loose WAV files.

**Practical implication for XO_OX:** When team members say "ship this as a Sample Library," they mean "bundle the stems/textures as loose WAVs, let the user place them." That is a valid creative choice but it is not a program — it is raw material.

---

## 5. Hybrid Packs — Multiple Program Types in One XPN

Yes. One XPN can contain multiple programs of any type. The XPN ZIP structure supports subdirectories, and MPC reads all valid program XML files it finds.

A well-structured hybrid pack looks like:

```
MyPack.xpn
  Programs/
    MyPack_Drums.xpn_drumprog      ← Drum Program
    MyPack_Bass.xpn_kgprog         ← Keygroup Program
    MyPack_Pads.xpn_kgprog         ← Keygroup Program
  Samples/
    Drums/
    Bass/
    Pads/
    Stems/                         ← loose WAVs, no program
```

MPC browser shows each program as a separate loadable item within the pack. The Stems folder appears under the pack's sample browser. Users can load one, two, or all programs simultaneously on separate tracks.

Hybrid packs increase perceived value significantly. An OPAL pack that ships one atmospheric keygroup + one texture stems folder is more useful than either alone.

---

## 6. XO_OX Engine → Format Decision Tree

**Percussive engines (ONSET):**
Drum Program. Always. 64-pad layout with velocity layers, round-robin per voice, choke groups for hat pairs and cymbal stacks.

**Granular/textural (OPAL, OBSIDIAN):**
Default to Keygroup if any pitched content (granular clouds with a root note). Use loose WAV stems alongside for raw texture material. A hybrid Keygroup + Stems folder is ideal.

**Melodic/pitched (OVERWORLD, ORACLE, OBBLIGATO, OTTONI):**
Keygroup. Multi-zone, minimum 3 velocity layers, root notes set precisely. These engines render into instruments the user plays, not pads they hit.

**Character/saturation (OBESE):**
Split by use. Bass and lead presets → Keygroup (played chromatically). Texture hits, distorted one-shots → Drum Program. A hybrid pack with both is the right default for OBESE.

**Atmospheric/drone (OHM, OSMOSIS, OVERLAP):**
Loose WAV stems as the primary deliverable, organized by key and mood. Optionally wrap in a wide-zone Keygroup for direct playability. The raw stems are the real asset here — users will loop, chop, and layer them their own way.

---

## Decision Tree (Quick Reference)

```
Is it pitched and played chromatically?
  YES → Keygroup Program
  NO  → Is it a hit/one-shot/loop triggered by a pad?
          YES → Drum Program
          NO  → Is it a texture/stem for user assembly?
                  YES → Loose WAVs (+ optional wide-zone Keygroup)
                  NO  → Reconsider what the deliverable actually is
```

**When in doubt: Keygroup for melodic, Drum Program for percussive, loose WAVs for ambient/atmospheric.**

"Sample Library" is a marketing term, not a program type. Use it in pack names and descriptions all you want — just know the actual MPC program types you are shipping inside.
