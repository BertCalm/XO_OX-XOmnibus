# XPN Pack DAW Integration — R&D Notes
**Date**: 2026-03-16 | **Status**: Research draft — speculative sections flagged

---

## 1. MPC Hardware Standalone vs MPC Software Plugin

MPC Software (desktop) is a near-complete feature mirror of standalone MPC hardware. It runs as a VST2/AU plugin inside any DAW or as a standalone application. XPN packs load identically in both environments — the `.xpn` file format is the same, and program/keygroup/drum structures are fully respected.

**Key differences relevant to XPN packs:**

- **Standalone MPC**: Direct hardware control, pads, Q-Links physical. XPN loads natively, no host DAW involved.
- **MPC Software plugin**: Runs inside Ableton/Logic/FL Studio/Pro Tools as a plugin instrument. XPN packs load the same way. The host DAW sees MPC Software as a single stereo (or multi-output) instrument.
- **Multi-output**: MPC Software supports up to 8 stereo output pairs when loaded as a plugin. Each pad or program can be routed to a different output pair — this is the primary bridge for stem work in a DAW host.

**Confirmed**: XPN packs loaded in MPC Software plugin are fully usable inside any AU/VST2-compatible DAW. The limitation is MPC Software's own licensing — producers must own MPC Software (bundled with hardware, or purchasable separately).

---

## 2. Stems Export Workflow

Getting individual pad stems into a DAW from an XPN pack involves two approaches:

**A. MPC Software multi-output routing** (cleanest)
- Assign each pad/program to a separate MPC output pair (Pad 1 → Out 1-2, Pad 2 → Out 3-4, etc.)
- In the DAW, create one audio track per MPC output pair, record-enable all
- Play the pattern — each pad records to its own track simultaneously
- *XPN design implication*: Pads should be grouped logically (kick, snare, hat, perc) so output assignments make musical sense. Avoid heavily layered pads that need internal separation.

**B. MIDI export + re-trigger** (most portable)
- Export MIDI pattern from MPC, import to DAW, route MIDI to MPC Software or any sampler loaded with the same WAV samples
- This requires producers to extract WAVs from the XPN manually (or via a tool)

**XPN design recommendations for easy stemming**: Keep percussion voices on distinct pads (one voice per pad), avoid inter-pad send routing that bakes in mix decisions, use consistent pad-to-output conventions across all XO_OX packs.

---

## 3. MIDI Pack Variant — XPN to Kontakt/EXS24/Battery

There is no official converter tool. The path is technically straightforward but requires a custom script.

An `xpn_to_kontakt.py` tool would need to:
1. Unzip the `.xpn` archive and locate the `.xpm` XML + embedded WAV files
2. Parse `<KeyGroup>` or `<PadNote>` entries: sample path, root note, velocity low/high, loop points
3. Output a Kontakt `.nki` script (NKI is text-based in older versions; newer Kontakt uses KSP scripting or NKI XML) or a Battery 4 `.b4p` preset XML

*Speculative*: NKI format for Kontakt 7 is not fully publicly documented. Battery 4's `.b4p` XML is more accessible. EXS24 (now Quick Sampler in Logic) uses a binary format pre-Logic 10.5, then a documented XML-adjacent format after. A practical near-term target is **Battery 4** (most open format) or **sfz** (fully open, readable by many samplers including Kontakt via SFZ player).

**Recommended first target**: `xpn_to_sfz.py` — SFZ is a plain-text open format, supported by Kontakt, sforzando, ARIA, and others. Far easier to generate than NKI, and covers 80% of the use case.

---

## 4. Ableton Integration — XPN to Drum Rack

Ableton Drum Racks cannot read XPM/XPN directly. The WAV samples inside an XPN are usable, but mapping must be done manually or via tooling.

An `xpn_to_ableton.py` tool approach:
1. Unzip XPN, extract WAVs to a folder
2. Parse XPM XML for pad-to-note mappings (MPC pads A01–D16 map to MIDI notes 37–52 by default)
3. Generate an Ableton `.adg` (Drum Rack preset) file — this is a gzipped XML format

*Speculative*: The `.adg` schema is not officially documented by Ableton but has been reverse-engineered by the community (e.g., the `ablgrab` and `ableton-live-xml` projects on GitHub). Key fields needed: `<DrumBranchPreset>`, `<ReceivingNote>`, `<SimplerDevicePreset>` with sample path references.

Velocity layer data from XPN can map to Ableton's Simpler velocity-to-volume curve, though multi-sample velocity switching (as in XPN keygroups) would require Sampler (not Simpler) inside each Drum Rack cell.

**Feasibility**: Medium. The `.adg` reverse-engineering work exists; a working tool is a 2–3 day effort.

---

## 5. Reaper Integration — XPN to ReaSamplomatic

ReaSamplomatic5000 supports multi-sample mapping with velocity layers. Its preset format is a plain-text `.rpl` (ReaBank/RPL) or can be configured via Reaper's FXChain XML.

*Speculative*: Automating ReaSamplomatic configuration programmatically is possible via Reaper's ReaScript API (Python/Lua) or by generating an FXChain XML file. Velocity layer data from XPN `<VelocityLow>`/`<VelocityHigh>` maps cleanly to ReaSamplomatic's velocity range parameters. A conversion script is feasible and would be simpler than the Ableton path given Reaper's open scripting model.

---

## 6. XO_OX Recommendation — XOmnibus vs XPN Conversion

**The honest answer: XOmnibus is the better path for DAW producers.**

XOmnibus is a JUCE AU/VST3 plugin that loads in every major DAW on macOS and Windows. It carries all 31+ engines, 2,369+ presets, and the full XO_OX sound palette — with no conversion, no file extraction, no format negotiation.

For a DAW producer:
- Load XOmnibus as an instrument track in Ableton/Logic/FL/Reaper/Pro Tools
- Browse presets directly in the plugin UI
- Record MIDI + audio as with any instrument
- Stems via multiple instances or DAW audio routing

XPN packs remain valuable for **MPC-centric producers** who live in the hardware/standalone workflow, and for use cases where the MPC's pattern sequencer, pad performance, or hardware feel is central to the work.

**Marketing framing suggestion**: "XPN packs are for your MPC. XOmnibus is for your DAW. Same sounds, different home." This cleanly segments the two products without implying one is a workaround for the other. A conversion tool (XPN → Ableton/Kontakt/SFZ) could be positioned as a bonus utility for producers who own both.

---

*Speculative sections: NKI format details (Section 3), `.adg` schema specifics (Section 4), ReaSamplomatic XML automation (Section 5). All practical recommendations in Section 6 are based on confirmed plugin capabilities.*
