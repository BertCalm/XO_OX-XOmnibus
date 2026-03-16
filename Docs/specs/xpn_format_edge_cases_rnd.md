# XPN/XPM Format Edge Cases — R&D Reference

**Status**: Living document — update as new behaviors are confirmed
**Purpose**: Safety/debugging reference for Oxport development
**Last updated**: 2026-03-16

---

## 1. Filename Restrictions

**Known safe character set**: `A-Z`, `a-z`, `0-9`, hyphen `-`, underscore `_`, period `.`

**Characters that cause problems**:
- Spaces in filenames: MPC may display the name correctly but path resolution can fail on some firmware versions. Avoid in WAV and XPM filenames.
- Parentheses `()`, brackets `[]`, ampersand `&`, percent `%`: Observed to cause mis-display or silent load failures on MPC OS 3.x.
- Unicode / non-ASCII characters: MPC file system is effectively ASCII-only. Accented characters (é, ü, ñ) and emoji will either be stripped, replaced with `?`, or cause the sample to fail silently.
- Forward slash `/` and backslash `\` in names (outside path separators): Corrupts the path field entirely.

**Length limits**: Keep filenames under 64 characters including extension. The MPC UI truncates display at roughly 32 characters but the internal limit appears to be 64. Filenames exceeding this may load but display as blank or truncated in the pad assignment view.

**Recommendation**: Generate all output filenames using only `[A-Za-z0-9_-]` plus `.wav` / `.xpm` / `.xpn` extensions. Strip or transliterate any other characters at export time.

---

## 2. Sample Path Handling

XPM files support both absolute paths (`/data/expansions/MyPack/Samples/kick.wav`) and relative paths (`Samples/kick.wav`).

**Absolute paths break on install**: If a pack is authored on one MPC unit (or standalone mode with a specific SD card mount point) and installed on another, absolute paths pointing to the author's directory will fail. The sample shows as missing — no error is raised, the pad silently loads nothing.

**MPC path resolution order** (observed behavior, not officially documented):
1. Absolute path if it exists verbatim on the current filesystem
2. Path relative to the XPM file's directory
3. Path relative to the pack root (the folder containing the `.xpn`'s extracted contents)

**Recommendation**: Always use paths relative to the XPM file location. This survives installation to any directory. The XO_OX XPN tools should enforce relative paths and never embed absolute paths in generated XPM XML.

---

## 3. Empty Layer Gotchas

**Golden rule**: Empty (unused) `<Layer>` elements must have `VelStart="0"` and `VelEnd="0"`.

**Why this matters**: The MPC's XPM parser scans all layer elements regardless of whether a sample path is assigned. If a layer has a non-zero velocity range but an empty or missing `SampleFile` attribute, the firmware attempts to allocate a voice slot for that range. The result is "ghost triggering" — notes in that velocity band trigger a voice with no audio data, consuming a polyphony slot and occasionally producing a brief click or DC offset pop at note-on. This is especially noticeable in quiet passages.

**What actually happens if violated**: Ghost triggers manifest as near-inaudible clicks or as polyphony stealing. In some firmware versions the ghost voice never releases, eventually consuming all voice slots until the program is reloaded.

---

## 4. Keygroup Zone Overlap

When two keygroup zones share overlapping note ranges, MPC behavior is: **both keygroups trigger simultaneously**.

**Safe overlap**: A 1–2 semitone crossfade overlap for legato transitions is generally safe and intentional in expressive instruments.

**Unsafe overlap**: Full-range overlap (e.g., two keygroups both covering C0–G9) causes both to trigger at full velocity for every note. This doubles polyphony consumption and can produce phase cancellation or unwanted doubling.

**No crash observed**, but no user-visible warning is given either. The overlap is silent and invisible in the MPC UI. Oxport should validate that keygroup note ranges are non-overlapping (or only minimally overlapping where intentional) before writing the XPM.

---

## 5. Sample Rate Mismatches

MPC **does resample on load** when a WAV file's sample rate differs from the current project/engine rate.

**Observed behavior**: A 48kHz WAV loaded into a 44.1kHz project plays back at the correct pitch and duration — MPC performs real-time or load-time SRC. Quality appears to be linear interpolation (acceptable, not high-end).

**Artifacts**: Mild aliasing on high-frequency transients (cymbals, hi-hats) when downsampling from 48kHz → 44.1kHz. Upsampling from 44.1kHz → 48kHz is cleaner.

**Recommendation**: Render all XO_OX pack samples at 44.1kHz / 24-bit. This is the MPC native rate for most users, eliminates SRC artifacts, and reduces file size vs 48kHz. If stems are recorded at 48kHz, convert at export time.

---

## 6. XPN ZIP Structure Requirements

**Compression**: MPC accepts both Deflate and Store modes. No observed difference in load behavior. Use Deflate for smaller file size.

**Nested folder depth**: Keep the structure shallow. Observed failures at depth > 3 levels (e.g., `PackName/Sounds/Category/Sub/file.wav`). Recommended max depth: 2 levels below pack root (`Samples/CategoryName/file.wav`).

**Symlinks**: Do not include symlinks in the ZIP. MPC firmware does not resolve them and the target file will not be found.

**Top-level structure**: The ZIP should extract to a single named folder (e.g., `XO_PACK_NAME/`) containing the XPM files and a `Samples/` subfolder. Flat ZIPs (files at root level) may work in some firmware versions but are unreliable.

---

## 7. RootNote=0 Convention

`RootNote="0"` is the MPC convention meaning **"auto-detect pitch from filename or let MPC decide"**. When set to `0`, MPC attempts to parse a note name from the sample filename (e.g., `Pad_C4.wav` → root C4) or defaults to C3.

**If RootNote is set to a real MIDI value** (e.g., `RootNote="60"` for C4): MPC uses that value for pitch-shifting calculations when the keygroup spans multiple notes. For drum/one-shot kits where no pitch shifting is desired, setting a non-zero RootNote can cause subtle pitch artifacts if the keygroup range is wider than a single note.

**Recommendation**: Use `RootNote="0"` for all drum/percussion layers. Use explicit RootNote values only for melodic/tonal keygroups where accurate pitch shifting is required.

---

## 8. Large Pack Behavior

MPC loads all samples for a program into RAM on program load. There is no streaming from storage during playback.

**When pack exceeds available RAM**: MPC silently drops samples that cannot be loaded — lower-priority slots (higher velocity layers, higher keygroup indices) appear to be dropped first, though the exact priority is undocumented. No error dialog is shown; the pads simply produce no audio.

**Practical RAM budget**: MPC Live II has ~2GB RAM with ~1GB typically available for samples after OS overhead. A pack should stay under 500MB of uncompressed audio to leave headroom for multi-program setups.

**Recommendation**: Enforce a per-pack sample budget in Oxport. Warn at 400MB, hard-cap at 600MB. Compress samples to 16-bit if needed to stay within budget (24-bit is preferred for quality; 16-bit is acceptable for percussion).

---

## 9. Firmware Version Differences

**MPC OS 3.0 → 3.1**: `CycleType` and `CycleGroup` attributes were added to `<Layer>` elements to support round-robin playback. XPMs without these fields load fine on 3.1+ (defaults to no round-robin), but XPMs using these fields will not work correctly on 3.0.

**MPC OS 3.2**: Added `PadNoteMap` and `PadGroupMap` at the program level for per-pad note/group assignment. These fields are ignored by 3.0–3.1 without error.

**MPC OS 3.3**: No confirmed schema changes. Improved SRC quality (anecdotal). Some users report more reliable relative path resolution introduced in this version.

**Safe baseline**: Target MPC OS 3.1 as minimum. Use `CycleType`/`CycleGroup` freely — they are ignored gracefully on older firmware. Document the minimum firmware version in each pack's README.

---

## Open Questions (Unconfirmed)

- Maximum number of keygroups per XPM program (128? 256? observed limit unknown)
- Behavior of duplicate `PadNote` assignments within the same program
- Whether MPC caches decompressed sample data across program reloads or re-reads from ZIP each time
- Exact memory behavior on MPC One (smaller RAM budget than Live II)

*Update this document when new behaviors are confirmed through testing or firmware release notes.*
