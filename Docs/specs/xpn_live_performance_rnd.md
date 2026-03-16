# XPN Live Performance R&D
**Date**: 2026-03-16
**Status**: R&D Reference — Production Design Rules

---

## 1. Polyphony Management

**MPC hardware polyphony ceilings (sampler voices, not synth voices):**

| Model | Max Sampler Voices |
|-------|--------------------|
| MPC Live III | 128 voices |
| MPC X (Gen 2) | 128 voices |
| MPC One (Gen 2) | 64 voices |
| MPC One (original) | 32 voices |

These ceilings apply to simultaneous audio streams playing from RAM. Each pad that triggers a sample consumes one voice until the sample ends or is stolen. Long sustaining samples with slow release tails are the primary polyphony drain.

**XPN design response:**

- Set `OneShot=True` in every drum keygroup that does not need sustain. OneShot mode kills the voice as soon as the sample finishes playing, freeing the voice immediately rather than waiting for a MIDI note-off.
- For melodic or sustained sounds, set explicit `AmpEnv_Release` values — do not leave them at default long tails. A 300–400ms release is almost always sufficient for live use.
- Design kick, snare, clap, and hat layers as self-contained single hits (no sustain loop). Pad synth layers may sustain but should have a hard voice limit of 2 per pad.

---

## 2. Fast Load Optimization

MPC loads XPN packs by reading sample files from internal storage into RAM. Load time is proportional to total sample data volume, not file count alone.

**Primary factors (ranked by impact):**

1. **Total uncompressed audio data** — a 50-sample pack at 2 seconds each loads faster than a 20-sample pack at 10 seconds each. Duration beats file count.
2. **Sample rate** — 44.1 kHz files load faster than 48 kHz or 96 kHz; MPC converts at load time on non-native rates. Render all XPN samples at 44.1 kHz / 24-bit.
3. **Stereo vs mono** — mono files are half the data of stereo. Use stereo only where stereo width is musically essential (pads, reverb tails). Drums should be mono or mid-only by default.
4. **File count** — each file incurs an I/O seek overhead. Round-robin variants multiply file count; limit to 3–4 cycle samples per voice for live packs (vs 6–8 for studio packs).

**Target: sub-5-second load on MPC Live III**

The MPC Live III has NVMe-class internal storage. At approximately 200 MB/s sustained read, 5 seconds allows ~1 GB of raw audio. In practice, OS and filesystem overhead reduce effective throughput. A safe live-load budget is:

- Max 200 samples per program
- Max 2 seconds per sample (98% of drum hits are well under this)
- Max total program size: 400 MB uncompressed (stereo 44.1/24)
- Warm load (program already in MPC RAM cache from previous set): effectively instant

---

## 3. Hot-Swap Between Packs

MPC does not pre-load programs that are not active. When you switch programs mid-performance there is a load gap — the new program must read from storage before it can play. On MPC Live III this gap is typically 1–3 seconds for a well-optimized pack.

**Strategies to eliminate the gap:**

- **Setlist pre-load**: Load all programs for the set before the performance begins. Use multiple program slots (up to 8 in a project) — assign Pack A to Program 1, Pack B to Program 2, etc. Switch between them with a program change message or manual track switch. There is no load delay because both are already in RAM.
- **RAM budget across a set**: MPC Live III has 2 GB RAM. 8 programs × 200 MB = 1.6 GB — well within budget. MPC One users should cap at 4 programs × 150 MB.
- **`setlist_builder.py` loading order**: Sequence packs from largest to smallest. The first program should be the main performance kit (loads during sound check). Transitional packs load next. Ending/closer packs load last, while earlier packs are already warm.
- **Avoid hot program creation**: Do not build new programs during performance. All programs used in the set must exist before downbeat.

---

## 4. Crash Prevention

MPC crashes during live performance almost always fall into three categories:

**RAM overflow**

- Symptom: MPC freezes or reboots when switching programs or triggering pads.
- Cause: Total loaded audio exceeds available RAM. This is more likely with multiple long samples or many round-robin variants active simultaneously.
- Prevention: Stay within the RAM budgets in Section 3. Monitor RAM usage in MPC settings before the set.

**Simultaneous long samples**

- Symptom: Stuttering or dropped voices after dense fills.
- Cause: Many held voices (long releases, no OneShot) hitting the polyphony ceiling simultaneously, causing the engine to steal voices unpredictably.
- Prevention: OneShot on all percussive elements. Hard maximum release times on melodic layers. Test by playing every pad simultaneously and holding — if output degrades, voices are over-allocated.

**Corrupt XPM**

- Symptom: Program fails to load, MPC shows error on import.
- Cause: Missing sample references (files moved after export), malformed XML, or empty required fields.
- Prevention: Run the XPN packager's validation pass before delivery. Every sample referenced in the XPM must be present at the relative path declared. Test import on a clean MPC project before committing to a set.

**Pre-gig stability test protocol:**

1. Import the full setlist project onto a fresh MPC project (not the development one).
2. Load all programs. Verify RAM usage is under 80% of available.
3. Play every pad on every program for 30 seconds of dense triggering.
4. Switch between programs 10 times in rapid succession.
5. If any program fails to load or any voice drops — fix before the gig.

---

## 5. MIDI Note Priority

When MPC's polyphony ceiling is hit, it uses a **steal-oldest** algorithm by default: the voice that has been playing the longest is terminated to free a slot for the new note. This is the most musically neutral choice for drum performance — it removes the tail of the oldest decay rather than cutting a currently-playing hit.

**Practical implications for pad design:**

- Long pad or bass notes will be stolen before short transients. This is generally correct behavior (the transient is perceptually more important than a decaying tail).
- If a specific voice must never be stolen — for example, a long reverb tail on a snare — designate that sample's program to a separate track/program slot with reserved polyphony headroom.
- Kick drums should be OneShot and short enough that their voice is already released before polyphony pressure builds. A kick at 500ms tail or less will self-release before steal can occur at typical 128-BPM tempos.
- Avoid stacking 4+ simultaneous layered voices on a single pad for live packs. Studio packs can use 6-layer velocity stacks; live packs should cap at 3 layers per pad.

---

## 6. Live-Friendly XPN Design Checklist

**8 rules for a "live-safe" pack:**

1. **Sample duration caps by voice type**
   - Kick / snare / clap / hi-hat: max 1.5 seconds
   - Tom / perc / FX hit: max 2.5 seconds
   - Melodic one-shots (keys, stabs): max 4 seconds
   - Sustained pads: allowed, but gate release must be ≤ 500ms in live variants

2. **OneShot=True on all percussive elements**
   Set `OneShot=True` in the XPM keygroup for every non-sustain layer. This is the single highest-impact setting for live stability.

3. **Max 3 velocity layers per pad in live packs**
   Studio packs may use 6 layers. Live packs cap at 3 — top, mid, soft. Reduces total sample count and RAM footprint.

4. **Max 4 round-robin cycle variants per voice**
   More variety is available in studio packs. Live packs prioritize fast load over maximal humanization.

5. **44.1 kHz / 24-bit audio only**
   No 48 kHz, no 96 kHz. MPC performs sample rate conversion at load time — 44.1 kHz avoids conversion overhead entirely.

6. **Stereo for pads only; mono for drums**
   Mono drum samples are half the RAM and half the polyphony bandwidth of stereo. Export all percussion as mono. Export pads and sustaining elements as stereo only when stereo width is sonically essential.

7. **RAM budget per program: 200 MB maximum (MPC Live III / X); 150 MB (MPC One)**
   Measure uncompressed WAV size of all samples in the program. If over budget, shorten tails, reduce round-robins, or convert stereo hits to mono.

8. **Pre-gig validation: load + dense-play test on clean project**
   Every program must pass a 30-second dense-play test (all pads, held) with no voice dropouts or freezes on target hardware before a set. This is the minimum acceptable QA gate for live-deployed packs.
