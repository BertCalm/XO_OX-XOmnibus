# P0 Bug Fixes — XO_OX Prism Sweep Round 3

Applied: 2026-03-14

---

## P0-01: Obsidian Right Channel Filter Bypass
**File:** `Source/Engines/Obsidian/ObsidianEngine.h` (~line 796)

**Root cause:** `voice.mainFilter.processSample()` was called only on `outputLeft`. The right channel then passed through an M/S reconstruction that blended filtered-left with unfiltered-right, producing an asymmetric stereo image where the right channel had no filter effect.

**Fix:** Removed the M/S reconstruction. Both `outputLeft` and `outputRight` are now passed directly through `voice.mainFilter.processSample()`.

---

## P0-02: Osteria Warmth Filter Left-Only
**File:** `Source/Engines/Osteria/OsteriaEngine.h` (~line 1227)

**Root cause:** `warmthFilter.processSample()` was called only on `mixL`. `mixR` skipped the warmth (proximity EQ low-shelf boost), producing unbalanced stereo where the right channel lacked the 300 Hz warmth character.

**Fix:** Added `mixR = warmthFilter.processSample(mixR)` immediately after the existing `mixL` call, matching the same pattern used by `smokeFilter` two lines above.

---

## P0-03: Origami STFT Block Size Guard
**File:** `Source/Engines/Origami/OrigamiEngine.h` (~line 474)

**Root cause:** `prepare()` ignored the possibility of `maxBlockSize < kHopSize` (512). If a host reports a block size smaller than the hop interval, the output/coupling cache vectors would be undersized relative to what the STFT pipeline assumes, risking out-of-bounds access.

**Fix:** Added `int safeBlockSize = std::max(maxBlockSize, kHopSize)` before the three `resize()` calls. The output cache and coupling input buffer are now allocated to at least 512 samples regardless of host block size.

---

## P0-04: Obsidian Formant Parameter ID Collision
**File:** `Source/Engines/Obsidian/ObsidianEngine.h` (~line 1073)

**Root cause:** Both `pFormantResonance` and `pFormantIntensity` pointed to `"obsidian_formantIntensity"`. The resonance pointer was therefore an alias for intensity — any code reading `pFormantResonance` was silently reading the intensity value instead.

**Fix:** Changed the `pFormantResonance` assignment to use `"obsidian_formantResonance"`, which is the correct distinct parameter ID.

---

## P0-05: XOverworld getSampleForCoupling Broken
**File:** `Source/Engines/Overworld/OverworldEngine.h` (~lines 45, 270, 281, 415)

**Root cause:** `getSampleForCoupling()` ignored both `channel` and `sampleIndex` arguments, always returning the single `lastSample` scalar (the last sample of the previous block). This meant any engine coupling from Overworld received a stale DC-like value instead of the actual per-sample audio signal.

**Fix (4 changes):**
1. `prepare()` signature updated from `int /*maxBlockSize*/` to `int maxBlockSize`; two `std::vector<float>` caches (`outputCacheLeft`, `outputCacheRight`) allocated to `maxBlockSize` elements.
2. Inside the per-sample render loop, each sample is written into the cache: `outputCacheLeft[s] = sample; outputCacheRight[s] = sample;`.
3. `getSampleForCoupling()` now indexes the correct cache vector by channel and sampleIndex, falling back to `lastSample` only if the index is out of range.
4. `outputCacheLeft` and `outputCacheRight` declared as member variables alongside `lastSample`.
