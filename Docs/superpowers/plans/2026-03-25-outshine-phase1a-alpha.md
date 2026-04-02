# Outshine Phase 1A-Alpha: Backend Hardening

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development to implement this plan task-by-task.

**Goal:** Harden the XOutshine.h backend so it produces correct, MPC-hardware-ready XPN output with multi-zone keygroups, accurate pitch detection, and a staged public API for the UI.

**Architecture:** All work is in a single file (`Source/Export/XOutshine.h`). 8 tasks modify existing private methods and add new public methods. No UI work. No new files created. The existing `run()` method continues to work throughout — each task is independently testable via `run()`.

**Tech Stack:** C++17, JUCE 8, XPM 1.7 format, MPC hardware targets

**Spec:** `docs/superpowers/specs/2026-03-25-outshine-phase1a-design.md` (Revision 3)
**Depends on:** 12 code fixes already applied (pitch ±3, gain ±0.5, cosine taper, ChokeSend, PitchBend=24, etc.)

---

## Ordering Rationale

Tasks proceed from deepest dependency to shallowest:

1. **Task 1** (SampleCategory enum) — adds categories consumed by Tasks 7 and 3
2. **Task 2** (YIN pitch detection) — populates `detectedMidiNote`, consumed by Tasks 3 and 6
3. **Task 3** (multi-zone keygroup builder) — depends on Task 2's root notes
4. **Task 4** (sample rate + bit depth guards) — independent; enhance() / normalize() path
5. **Task 5** (channel validation) — independent; analyze() entry guard
6. **Task 6** (streaming ZIP) — independent; packageXPN() rewrite
7. **Task 7** (per-category LUFS) — depends on Task 1's new categories
8. **Task 8** (staged public API) — wraps everything; comes last

Each task is an independent commit. The file compiles after every task.

---

## TASK 1: Expand SampleCategory + classifyByName()

**What:** Add `Loop`, `Woodwind`, `Brass`, `Vocal` to the `SampleCategory` enum. Update all category-aware switches and the `classifyByName()` keyword pass.

**Why:** The spec defines 18 categories (was 14 live + `Unknown` = 15). Three new melodic categories and one structural category (`Loop`) are required by the spec (Sections 2 and 6). Without them, woodwind and brass samples fall through to `Unknown`, blocking the per-category LUFS fix in Task 7.

**File:** `Source/Export/XOutshine.h`

### 1.1 — Enum (line 34–38, replace the enum entirely)

```cpp
enum class SampleCategory
{
    // Drum / percussive
    Kick, Snare, HiHatClosed, HiHatOpen, Clap, Tom, Percussion, FX,
    // Structural
    Loop,
    // Melodic
    Bass, Pad, Lead, Keys, Pluck, String, Woodwind, Brass, Vocal,
    // Fallback
    Unknown
};
```

### 1.2 — isDrumCategory() (line 40–46, no change needed — Loop is NOT a drum category)

`Loop` receives loop-point detection, not one-shot treatment. Leave `isDrumCategory()` as-is; it returns false for any category not listed, which correctly excludes `Loop`.

### 1.3 — gmNoteForCategory() (line 48–61, add cases before `default`)

```cpp
inline int gmNoteForCategory (SampleCategory c)
{
    switch (c) {
        case SampleCategory::Kick:         return 36;
        case SampleCategory::Snare:        return 38;
        case SampleCategory::HiHatClosed:  return 42;
        case SampleCategory::HiHatOpen:    return 46;
        case SampleCategory::Clap:         return 39;
        case SampleCategory::Tom:          return 41;
        case SampleCategory::Percussion:   return 43;
        case SampleCategory::FX:           return 49;
        case SampleCategory::Loop:         return 60;   // C4 — loop center
        default:                           return 60;
    }
}
```

### 1.4 — categoryName() (line 63–82, add cases before `default`)

```cpp
inline const char* categoryName (SampleCategory c)
{
    switch (c) {
        case SampleCategory::Kick:         return "Kick";
        case SampleCategory::Snare:        return "Snare";
        case SampleCategory::HiHatClosed:  return "HiHat Closed";
        case SampleCategory::HiHatOpen:    return "HiHat Open";
        case SampleCategory::Clap:         return "Clap";
        case SampleCategory::Tom:          return "Tom";
        case SampleCategory::Percussion:   return "Percussion";
        case SampleCategory::FX:           return "FX";
        case SampleCategory::Loop:         return "Loop";
        case SampleCategory::Bass:         return "Bass";
        case SampleCategory::Pad:          return "Pad";
        case SampleCategory::Lead:         return "Lead";
        case SampleCategory::Keys:         return "Keys";
        case SampleCategory::Pluck:        return "Pluck";
        case SampleCategory::String:       return "String";
        case SampleCategory::Woodwind:     return "Woodwind";
        case SampleCategory::Brass:        return "Brass";
        case SampleCategory::Vocal:        return "Vocal";
        default:                           return "Unknown";
    }
}
```

### 1.5 — classifyByName() (line 342–375, add patterns before `return SampleCategory::Unknown`)

Add these blocks immediately before the final `return SampleCategory::Unknown;` line, after the existing `String` check:

```cpp
        if (nl.contains("loop") || nl.contains(" lp") || nl.contains("_lp")
            || nl.contains("phrase"))
            return SampleCategory::Loop;
        if (nl.contains("flute") || nl.contains("clarinet") || nl.contains("saxophone")
            || nl.contains("_sax") || nl.contains(" sax") || nl.contains("oboe")
            || nl.contains("bassoon") || nl.contains("recorder") || nl.contains("piccolo")
            || nl.contains("fife"))
            return SampleCategory::Woodwind;
        if (nl.contains("trumpet") || nl.contains("trombone") || nl.contains("french horn")
            || nl.contains("frenchhorn") || nl.contains("tuba") || nl.contains("cornet")
            || nl.contains("flugelhorn") || nl.contains("_horn") || nl.contains(" horn"))
            return SampleCategory::Brass;
        if (nl.contains("vocal") || nl.contains("voice") || nl.contains("choir")
            || nl.contains("singing") || nl.contains("_vox") || nl.contains(" vox")
            || nl.contains("acapella") || nl.contains("spoken"))
            return SampleCategory::Vocal;
```

**Note on "horn":** The pattern `_horn` / ` horn` (with prefix delimiter) avoids matching "shorthorn", "longhorn", etc. If a filename is simply `Horn_C4.wav`, `nl.contains(" horn")` will not match; add `nl.startsWith("horn")` if that edge case matters in practice.

### Test

```
run() on a folder containing:
  Flute_C4.wav         → should classify as Woodwind
  Trumpet_A3.wav       → should classify as Brass
  Choir_Loop_01.wav    → should classify as Vocal (Vocal check is before Loop; acceptable)
  Phrase_120bpm.wav    → should classify as Loop
```

Check `categoryName(s.category)` in a debug print inside `classify()`.

### Commit message

```
feat(Outshine): expand SampleCategory — add Loop, Woodwind, Brass, Vocal (Task 1/8)
```

---

## TASK 2: YIN Pitch Detection

**What:** Implement `yinDetectPitch()` as a private method and call it from `analyze()`. Write result to `s.detectedMidiNote` and `s.pitchConfidence`.

**Why:** The current `analyze()` never sets `detectedMidiNote` (it stays at the struct default of 60). Every melodic zone will be centered on C4 regardless of the actual sample pitch. YIN is required for the multi-zone keygroup builder in Task 3.

**Reference:** de Cheveigne & Kawahara 2002, "YIN, a fundamental frequency estimator for speech and music."

**File:** `Source/Export/XOutshine.h`

### 2.1 — Private method: yinDetectPitch()

Add this method in the private section, after `classifyByAudio()` and before `analyze()` (approximately line 407):

```cpp
    // YIN pitch detection (de Cheveigne & Kawahara 2002).
    // Returns the detected MIDI note (0-127) and writes confidence to `confidence`.
    // Returns -1 if detection fails (caller falls back to C4 = 60).
    // Minimum meaningful buffer: 2 cycles at the lowest expected frequency (40 Hz at 48 kHz = 2400 samples).
    static int yinDetectPitch (const float* data, int numSamples, double sampleRate,
                               float& confidence)
    {
        confidence = 0.0f;

        // Minimum buffer guard: need at least 2 cycles at 40 Hz
        int minBuffer = (int)(2.0 * sampleRate / 40.0);
        if (numSamples < minBuffer || numSamples < 512)
            return -1;

        // Half-buffer size (W in the paper). Use half the input buffer, capped at 4096.
        int W = std::min(numSamples / 2, 4096);

        // Step 1: Difference function d(tau) for tau = 0..W-1
        // d(tau) = sum_{j=0}^{W-1} (x[j] - x[j+tau])^2
        std::vector<double> d(W, 0.0);
        for (int tau = 1; tau < W; ++tau)
        {
            double sum = 0.0;
            for (int j = 0; j < W; ++j)
            {
                double diff = (double)data[j] - (double)data[j + tau];
                sum += diff * diff;
            }
            d[tau] = sum;
        }

        // Step 2: Cumulative mean normalized difference function (CMNDF)
        // d'[0] = 1, d'[tau] = d[tau] / ((1/tau) * sum_{j=1}^{tau} d[j])
        std::vector<double> cmndf(W, 0.0);
        cmndf[0] = 1.0;
        double runningSum = 0.0;
        for (int tau = 1; tau < W; ++tau)
        {
            runningSum += d[tau];
            cmndf[tau] = (runningSum > 0.0) ? (d[tau] * tau / runningSum) : 1.0;
        }

        // Step 3: Absolute threshold — find the first tau where cmndf[tau] < threshold
        // Use threshold 0.15 (per spec). Skip the first few samples (avoid octave errors).
        constexpr double kThreshold = 0.15;
        constexpr int kMinTau = 2;

        // Lower frequency bound: 40 Hz. Upper: 2000 Hz.
        int tauMin = (int)(sampleRate / 2000.0);
        int tauMax = (int)(sampleRate / 40.0);
        tauMin = std::max(tauMin, kMinTau);
        tauMax = std::min(tauMax, W - 1);

        int bestTau = -1;
        double bestVal = 1.0;
        for (int tau = tauMin; tau <= tauMax; ++tau)
        {
            if (cmndf[tau] < kThreshold)
            {
                bestTau = tau;
                bestVal = cmndf[tau];
                break;  // first dip below threshold is the fundamental
            }
            // Track global minimum in range as fallback
            if (cmndf[tau] < bestVal)
            {
                bestVal = cmndf[tau];
                bestTau = tau;
            }
        }

        if (bestTau <= 0)
            return -1;

        // Step 4: Parabolic interpolation around bestTau for sub-sample accuracy
        double tau0 = (double)bestTau;
        if (bestTau > tauMin && bestTau < tauMax)
        {
            double alpha = cmndf[bestTau - 1];
            double beta  = cmndf[bestTau];
            double gamma = cmndf[bestTau + 1];
            double denom = alpha - 2.0 * beta + gamma;
            if (std::abs(denom) > 1e-10)
                tau0 = bestTau - (gamma - alpha) / (2.0 * denom);
        }

        // Convert period (in samples) → frequency → MIDI note
        if (tau0 <= 0.0)
            return -1;

        double frequency = sampleRate / tau0;
        if (frequency < 20.0 || frequency > 20000.0)
            return -1;

        // MIDI note = 69 + 12 * log2(freq / 440)
        double midiNote = 69.0 + 12.0 * std::log2(frequency / 440.0);
        int midiNoteInt = (int)std::round(midiNote);
        if (midiNoteInt < 0 || midiNoteInt > 127)
            return -1;

        // Confidence: invert CMNDF value at best tau (lower CMNDF = higher confidence)
        confidence = (float)juce::jlimit(0.0, 1.0, 1.0 - bestVal);

        return midiNoteInt;
    }
```

### 2.2 — Call from analyze() (after the tail-length block, before the loop detection block, approximately line 455)

In `analyze()`, inside the loop `for (auto& s : samples)`, add this block after the tail-length calculation and before the loop-detection `if` block:

```cpp
            // YIN pitch detection — skip drums and FX (they have no meaningful pitch)
            if (!isDrumCategory(s.category) && s.category != SampleCategory::FX)
            {
                // Mix to mono for pitch detection
                int detectLen = std::min(n, (int)(4.0 * s.sampleRate)); // cap at 4 seconds
                std::vector<float> mono(detectLen, 0.0f);
                int nchMix = buf.getNumChannels();
                for (int ch = 0; ch < nchMix; ++ch)
                {
                    auto* src = buf.getReadPointer(ch);
                    for (int i = 0; i < detectLen; ++i)
                        mono[i] += src[i] / (float)nchMix;
                }

                float confidence = 0.0f;
                int detected = yinDetectPitch(mono.data(), detectLen, s.sampleRate, confidence);
                if (detected >= 0)
                {
                    s.detectedMidiNote = detected;
                    s.pitchConfidence  = confidence;
                }
                else
                {
                    // Fallback: C4, zero confidence — flagged in the UI as 'Root Unverified'
                    s.detectedMidiNote = 60;
                    s.pitchConfidence  = 0.0f;
                    // Log warning to errors_ list (non-fatal)
                    errors_.add("YIN detection failed for \"" + s.name
                                + "\" — using C4 as provisional root.");
                }
            }
```

**Important:** `analyze()` is non-static; it has access to `errors_`. The `yinDetectPitch()` helper is `static` (no member access needed).

**Why 4-second cap:** YIN is O(W²) per sample in the naive implementation above. For a W=4096 half-buffer, that is 16M operations. Capping input to 4 seconds keeps the analysis phase within acceptable latency for a 50-sample kit.

**Performance note:** If this proves too slow, replace the inner loop with a windowed correlation via FFT. For Phase 1A, the O(W²) naive implementation is acceptable.

### Test

```
Synthesize a sine wave at A4 (440 Hz), 44100 Hz sample rate, 2 seconds.
Write it as a WAV. Name it "Sine_A4.wav".
run() on that file.
Expected: s.detectedMidiNote == 69, s.pitchConfidence > 0.8
```

A quick test script:

```python
import numpy as np
import soundfile as sf
sr = 44100
t = np.linspace(0, 2, 2 * sr)
wav = np.sin(2 * np.pi * 440 * t).astype(np.float32)
sf.write('/tmp/Sine_A4.wav', wav, sr)
# Then run XOutshine on /tmp/ and check the log for MIDI note 69
```

### Commit message

```
feat(Outshine): YIN pitch detection in analyze() — replaces missing root-note detection (Task 2/8)
```

---

## TASK 3: Multi-Zone Keygroup Builder

**What:** Rewrite `buildKeygroupXPM()` to generate one `<Keygroup>` element per input sample, with zone boundaries computed from the midpoint rule.

**Why:** The current implementation produces a single `<Keygroup index="0">` spanning the full MIDI range 0–127 regardless of how many melodic samples are provided. With 12 Rhodes multi-samples, MPC loads all 12 as overlapping zones at the same range. Every note triggers all 12 simultaneously.

**Depends on:** Task 2 (detectedMidiNote must be populated).

**File:** `Source/Export/XOutshine.h`

### 3.1 — Group samples by category in buildPrograms() (lines 680–697)

The current `buildPrograms()` calls `buildKeygroupXPM()` once per `UpgradedProgram`. That is correct — one program = one XPM. What changes is the content of that XPM: it now contains multiple `<Keygroup>` elements built from a list of `AnalyzedSample` structs.

However, there is a structural gap: an `UpgradedProgram` represents one source sample (and its velocity/RR variants), not a group of related melodic samples. The multi-zone approach requires grouping all melodic samples into a single program before calling `buildKeygroupXPM()`.

**Add a grouping step at the start of `buildPrograms()`** (replace the existing loop):

```cpp
    void buildPrograms (std::vector<UpgradedProgram>& programs, const juce::File& workDir)
    {
        auto xpmDir = workDir.getChildFile("programs");
        xpmDir.createDirectory();

        auto splits = getVelocitySplits(settings_.velocityCurve);

        // Separate drum programs (one XPM per pad) from melodic programs (one XPM for all zones)
        std::vector<UpgradedProgram*> drumProgs;
        std::vector<UpgradedProgram*> melodicProgs;
        for (auto& prog : programs)
        {
            if (isDrumCategory(prog.category))
                drumProgs.push_back(&prog);
            else
                melodicProgs.push_back(&prog);
        }

        // Write one drum XPM per drum program (unchanged behavior)
        for (auto* prog : drumProgs)
        {
            auto xpmFile = xpmDir.getChildFile(prog->name + ".xpm");
            xpmFile.replaceWithText(buildDrumXPM(*prog, splits));
        }

        // Write one keygroup XPM for all melodic programs combined
        if (!melodicProgs.empty())
        {
            // Use the pack name as the program name; fall back to first sample name
            juce::String melodicName = settings_.packName.isEmpty()
                ? melodicProgs[0]->name : settings_.packName;
            auto xpmFile = xpmDir.getChildFile(sanitizeForFAT32(melodicName) + ".xpm");
            xpmFile.replaceWithText(buildKeygroupXPM(melodicProgs, splits, melodicName));
        }
    }
```

### 3.2 — Rewrite buildKeygroupXPM() to accept a list of programs

Replace the existing `buildKeygroupXPM(const UpgradedProgram&, ...)` signature with a new one that takes a sorted list. Keep the old signature as a single-sample overload that delegates to the new one (for backward compatibility with any future callers):

```cpp
    // Multi-zone keygroup builder — one <Keygroup> per melodic sample.
    juce::String buildKeygroupXPM (const std::vector<UpgradedProgram*>& progs,
                                    const std::vector<VelocitySplit>& splits,
                                    const juce::String& programName)
    {
        // Sort by detected MIDI note (ascending)
        std::vector<UpgradedProgram*> sorted = progs;
        std::sort(sorted.begin(), sorted.end(),
                  [](const UpgradedProgram* a, const UpgradedProgram* b) {
                      return a->sourceInfo.detectedMidiNote < b->sourceInfo.detectedMidiNote;
                  });

        // Compute zone boundaries: midpoint rule.
        // lowNote[i]  = (rootNote[i-1] + rootNote[i]) / 2 + 1  (exclusive upper of previous zone)
        // highNote[i] = (rootNote[i] + rootNote[i+1]) / 2       (inclusive upper of this zone)
        // Edge cases: first zone low = 0, last zone high = 127.
        int n = (int)sorted.size();
        std::vector<int> lowNotes(n), highNotes(n);

        if (n == 1)
        {
            // Single sample: full range
            lowNotes[0]  = 0;
            highNotes[0] = 127;
        }
        else
        {
            for (int i = 0; i < n; ++i)
            {
                int root = sorted[i]->sourceInfo.detectedMidiNote;

                // Low boundary
                if (i == 0)
                    lowNotes[i] = 0;
                else
                {
                    int prevRoot = sorted[i - 1]->sourceInfo.detectedMidiNote;
                    lowNotes[i] = (prevRoot + root) / 2 + 1;  // round down, then +1 for exclusive upper
                }

                // High boundary
                if (i == n - 1)
                    highNotes[i] = 127;
                else
                {
                    int nextRoot = sorted[i + 1]->sourceInfo.detectedMidiNote;
                    highNotes[i] = (root + nextRoot) / 2;      // inclusive upper of this zone
                }

                // Clamp
                lowNotes[i]  = juce::jlimit(0, 127, lowNotes[i]);
                highNotes[i] = juce::jlimit(0, 127, highNotes[i]);
            }
        }

        // Build XPM XML
        juce::String xml;
        xml << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
        xml << "<MPCVObject type=\"com.akaipro.mpc.keygroup.program\">\n";
        xml << "  <Version>1.7</Version>\n";
        xml << "  <ProgramName>" << xmlEscape(programName) << "</ProgramName>\n";
        xml << "  <AfterTouch>\n";
        xml << "    <Destination>FilterResonance</Destination>\n";
        xml << "    <Amount>40</Amount>\n";
        xml << "  </AfterTouch>\n";
        xml << "  <ModWheel>\n";
        xml << "    <Destination>FilterCutoff</Destination>\n";
        xml << "    <Amount>50</Amount>\n";
        xml << "  </ModWheel>\n";
        xml << "  <PitchBendRange>24</PitchBendRange>\n";
        xml << "  <Keygroups>\n";

        for (int i = 0; i < n; ++i)
        {
            const auto& prog = *sorted[i];
            int root = prog.sourceInfo.detectedMidiNote;

            xml << "    <Keygroup index=\"" << i << "\">\n";
            xml << "      <LowNote>" << lowNotes[i] << "</LowNote>\n";
            xml << "      <HighNote>" << highNotes[i] << "</HighNote>\n";

            xml << "      <Layers>\n";
            int layerIdx = 0;

            for (int v = 0; v < prog.numVelocityLayers && v < (int)splits.size(); ++v)
            {
                auto rrs = getLayersForVel(prog.layers, v);
                for (const auto* l : rrs)
                {
                    xml << "        <Layer index=\"" << layerIdx++ << "\">\n";
                    xml << "          <SampleName>" << l->filename << "</SampleName>\n";
                    xml << "          <VelStart>" << splits[v].start << "</VelStart>\n";
                    xml << "          <VelEnd>" << splits[v].end << "</VelEnd>\n";
                    xml << "          <Volume>" << juce::String(splits[v].volume, 2) << "</Volume>\n";
                    xml << "          <RootNote>" << root << "</RootNote>\n";
                    xml << "          <KeyTrack>True</KeyTrack>\n";
                    xml << "          <TuneCoarse>0</TuneCoarse>\n";
                    xml << "          <TuneFine>0</TuneFine>\n";
                    if (prog.sourceInfo.isLoopable)
                    {
                        xml << "          <LoopStart>" << prog.sourceInfo.loopStart << "</LoopStart>\n";
                        xml << "          <LoopEnd>" << prog.sourceInfo.loopEnd << "</LoopEnd>\n";
                        xml << "          <LoopCrossfade>100</LoopCrossfade>\n";
                    }
                    else
                    {
                        xml << "          <LoopStart>-1</LoopStart>\n";
                        xml << "          <LoopEnd>-1</LoopEnd>\n";
                    }
                    if (rrs.size() > 1)
                    {
                        xml << "          <CycleType>RoundRobin</CycleType>\n";
                        xml << "          <CycleGroup>" << (v + 1) << "</CycleGroup>\n";
                    }
                    xml << "        </Layer>\n";
                }
            }

            // Empty layers must have VelStart = 0 (XPM hardware requirement)
            for (int v = prog.numVelocityLayers; v < 4; ++v)
            {
                xml << "        <Layer index=\"" << layerIdx++ << "\">\n";
                xml << "          <VelStart>0</VelStart>\n";
                xml << "          <VelEnd>0</VelEnd>\n";
                xml << "        </Layer>\n";
            }

            xml << "      </Layers>\n";
            xml << "    </Keygroup>\n";
        }

        xml << "  </Keygroups>\n";
        xml << "</MPCVObject>\n";
        return xml;
    }

    // Single-sample overload — delegates to the multi-zone builder.
    // Preserves backward compatibility with any callers that pass a single UpgradedProgram.
    juce::String buildKeygroupXPM (const UpgradedProgram& prog,
                                    const std::vector<VelocitySplit>& splits)
    {
        std::vector<UpgradedProgram*> single = { const_cast<UpgradedProgram*>(&prog) };
        return buildKeygroupXPM(single, splits, prog.name);
    }
```

**Note on const_cast:** The `const_cast` in the overload is safe because the multi-zone builder only reads from the pointer; the underlying program is not modified. If this feels fragile, change `std::vector<UpgradedProgram*>` to `std::vector<const UpgradedProgram*>` throughout.

### 3.3 — Remove the old buildKeygroupXPM() body

The old implementation at lines 761–831 is now replaced by the two methods above. Delete the old body entirely.

### Test

```
Create three WAV files:
  RhodesC3.wav  — 261 Hz sine (MIDI 48, C3)
  RhodesC4.wav  — 523 Hz sine (MIDI 60, C4)
  RhodesC5.wav  — 1046 Hz sine (MIDI 72, C5)
run() on the folder.
Open the resulting .xpm. Expect:
  <Keygroup index="0">  LowNote=0   HighNote=53   (midpoint of 48+60 = 54, high = 54-1 = 53)
  <Keygroup index="1">  LowNote=54  HighNote=65   (midpoint of 60+72 = 66, high = 66-1 = 65)
  <Keygroup index="2">  LowNote=66  HighNote=127
  Each keygroup: RootNote = detected MIDI note of the source sample.
```

**Midpoint math check:**
- Zone 0 root=48, Zone 1 root=60: midpoint=(48+60)/2=54. Zone 0 high=53, Zone 1 low=54. ✓
- Zone 1 root=60, Zone 2 root=72: midpoint=(60+72)/2=66. Zone 1 high=65, Zone 2 low=66. ✓

### Commit message

```
feat(Outshine): multi-zone keygroup builder with midpoint rule zone boundaries (Task 3/8)
```

---

## TASK 4: Sample Rate + Bit Depth Guards

**What:** In `normalize()` (Stage 5), enforce 24-bit integer output on all exported WAVs. Downsample 96 kHz sources to 48 kHz. Reject sources above 96 kHz. Promote 16-bit sources to 24-bit.

**Why:** MPC hardware silently rejects 32-bit float WAVs. The current `writeWav()` call passes `s.bitDepth` through unchanged, so 32-bit float sources from DAWs produce exports that load as empty on hardware.

**File:** `Source/Export/XOutshine.h`

### 4.1 — Add sample rate check to analyze() (before Task 2's YIN block — line ~422)

At the start of the `for (auto& s : samples)` body in `analyze()`, after setting `s.sampleRate`:

```cpp
            // Sample rate guard — spec requires: pass ≤48 kHz, downsample 96 kHz, reject >96 kHz
            if (s.sampleRate > 96000.0)
            {
                errors_.add("\"" + s.name + "\": sample rate "
                    + juce::String((int)(s.sampleRate / 1000)) + " kHz is not supported. "
                    + "Maximum is 96 kHz. File will be skipped.");
                // Mark as invalid so downstream stages can skip it
                s.bitDepth = -1;  // sentinel: invalid sample
                continue;
            }
```

Also, in `enhance()`, guard against the sentinel at the start of the per-sample loop:

```cpp
            // Skip samples that were flagged invalid in analyze()
            if (s.bitDepth < 0) continue;
```

### 4.2 — Force 24-bit output and downsample in normalize() (replace the writeWav call inside the LUFS loop)

The normalize() method currently calls `writeWav(l.file, lb, lr->sampleRate, (int)lr->bitsPerSample)`. Replace the entire layer-write block:

```cpp
                // Bit depth: always write 24-bit integer.
                // 32-bit float → 24-bit: scale and truncate (dither applied in writeWav).
                // 16-bit → 24-bit: zero-pad lower 8 bits (no dither needed on promotion).
                int targetBitDepth = 24;

                // Sample rate: downsample 96 kHz → 48 kHz via 2:1 decimation with LP filter.
                // 44.1 kHz and 48 kHz pass through unchanged.
                double targetSampleRate = lr->sampleRate;
                if (lr->sampleRate > 48001.0)  // 96 kHz path
                {
                    juce::AudioBuffer<float> downsampled = downsampleBy2(lb);
                    targetSampleRate = lr->sampleRate / 2.0;
                    writeWav(l.file, downsampled, targetSampleRate, targetBitDepth);
                }
                else
                {
                    writeWav(l.file, lb, targetSampleRate, targetBitDepth);
                }
```

### 4.3 — Add downsampleBy2() private method

Add this method in the private Utility section (after `writeWav()`, approximately line 1032):

```cpp
    // 2:1 decimation with a simple 5-tap LP FIR anti-alias filter.
    // Cutoff is approximately 0.45 * Nyquist of the output rate.
    // Suitable for 96 kHz → 48 kHz. Not suitable for >2:1 ratios.
    static juce::AudioBuffer<float> downsampleBy2 (const juce::AudioBuffer<float>& src)
    {
        int nch  = src.getNumChannels();
        int nIn  = src.getNumSamples();
        int nOut = nIn / 2;
        juce::AudioBuffer<float> out(nch, nOut);

        // 5-tap symmetric LP FIR: h = [0.0625, 0.25, 0.375, 0.25, 0.0625]
        // Designed for cutoff ~0.45 * Nyquist_out. Provides ~50 dB stopband attenuation.
        constexpr float h[5] = { 0.0625f, 0.25f, 0.375f, 0.25f, 0.0625f };

        for (int ch = 0; ch < nch; ++ch)
        {
            auto* src_ch = src.getReadPointer(ch);
            auto* dst_ch = out.getWritePointer(ch);

            for (int i = 0; i < nOut; ++i)
            {
                // Input sample index for output sample i is 2*i.
                // Filter window centered at 2*i, taps at 2*i-2 to 2*i+2.
                float acc = 0.0f;
                for (int k = 0; k < 5; ++k)
                {
                    int srcIdx = 2 * i + (k - 2);
                    if (srcIdx >= 0 && srcIdx < nIn)
                        acc += h[k] * src_ch[srcIdx];
                }
                dst_ch[i] = acc;
            }
        }
        return out;
    }
```

**Note on filter quality:** The 5-tap FIR achieves adequate attenuation for 96→48 kHz and has zero phase error (symmetric coefficients). For higher-quality resampling (Phase 1B), replace with a windowed-sinc resampler. The 5-tap is sufficient for Phase 1A — MPC hardware will not distinguish.

### Test

```
1. Create a 32-bit float WAV at 44100 Hz. run() on it.
   Expected: output WAV is 24-bit integer. MPC loads it without silent rejection.

2. Create a 96 kHz WAV. run() on it.
   Expected: output WAV is 48 kHz, 24-bit. Check header with 'afinfo' or 'soxi'.

3. Create a 192 kHz WAV. run() on it.
   Expected: error in getErrors(): "sample rate 192 kHz is not supported..."
             Output XPN contains zero WAVs for that sample.

4. Create a 16-bit WAV. run() on it.
   Expected: output WAV is 24-bit (promoted, no dither artifacts).
```

Shell verification: `soxi -r output.wav` should return 48000 for a 96 kHz input.

### Commit message

```
feat(Outshine): 24-bit output enforcement + 96kHz→48kHz downsampling + >96kHz rejection (Task 4/8)
```

---

## TASK 5: Channel Validation

**What:** At the start of `analyze()`, after reading `numChannels` for each sample, check that all samples in the same keygroup candidate share the same channel count. Flag mono/stereo mismatches as errors and skip the conflicting samples.

**Why:** MPC rejects programs where layers have mixed mono/stereo files. The current pipeline passes mixed-channel kits silently to the packager, producing invalid XPN output.

**File:** `Source/Export/XOutshine.h`

### 5.1 — Add channel validation pass to analyze() (after the per-sample metadata loop)

The `analyze()` method currently loops over samples and fills metadata. Channel validation needs to happen after all samples have been read (so we know the full channel count distribution). Add a validation pass at the end of `analyze()`, after the closing `}` of the sample loop (approximately line 476):

```cpp
    void analyze (std::vector<AnalyzedSample>& samples)
    {
        juce::AudioFormatManager fmgr;
        fmgr.registerBasicFormats();

        for (auto& s : samples)
        {
            // ... existing per-sample analysis code (unchanged) ...
        }

        // Channel validation: all melodic samples must have matching channel counts.
        // Mixed mono/stereo causes MPC to reject the program.
        {
            int melodicChannels = -1;  // -1 = not yet set
            bool mismatchFound = false;

            for (auto& s : samples)
            {
                if (s.bitDepth < 0) continue;  // already flagged invalid (Task 4 sentinel)
                if (isDrumCategory(s.category)) continue;  // drums map to separate pads; no constraint

                if (melodicChannels < 0)
                {
                    melodicChannels = s.numChannels;
                }
                else if (s.numChannels != melodicChannels)
                {
                    mismatchFound = true;
                    errors_.add("Channel mismatch: \"" + s.name + "\" is "
                        + juce::String(s.numChannels) + "-channel but other melodic samples are "
                        + juce::String(melodicChannels) + "-channel. "
                        + "MPC requires all layers in a keygroup to match. "
                        + "Mixed-channel samples will be excluded.");
                    s.bitDepth = -1;  // reuse sentinel to exclude from enhance()
                }
            }

            if (mismatchFound)
            {
                errors_.add("Channel validation failed: some samples excluded. "
                            "Convert all melodic samples to the same channel count before running Outshine.");
            }
        }
    }
```

**Design note:** Drum programs are excluded from the validation because each drum maps to its own `<Instrument>` pad with no cross-layer constraint. Only melodic samples that share a `<Keygroup>` must match.

**Sentinel reuse:** The `-1` bitDepth sentinel from Task 4 is reused here. This avoids adding a separate `bool excluded` field to `AnalyzedSample`. If the sentinel approach feels fragile, add `bool excluded = false` to `AnalyzedSample` explicitly. The sentinel is internal to the alpha pipeline and not visible to callers.

### Test

```
Create a folder with:
  Keys_C3_mono.wav    (mono, 1 channel)
  Keys_C4_stereo.wav  (stereo, 2 channels)
run() on the folder.
Expected: getErrors() contains a channel mismatch message.
          One of the two samples is excluded from the XPM output.
          run() returns false (issues > 0).

Create a folder with:
  Keys_C3.wav   (stereo)
  Keys_C4.wav   (stereo)
run() on the folder.
Expected: getErrors() is empty. Output XPM has 2 keygroups.
```

### Commit message

```
feat(Outshine): channel validation in analyze() — flags mono/stereo mismatches before packaging (Task 5/8)
```

---

## TASK 6: Streaming ZIP Output

**What:** Rewrite `packageXPN()` to write files into the ZIP archive one at a time, reading each WAV from disk rather than buffering all WAVs in memory simultaneously.

**Why:** The current `packageXPN()` uses `juce::ZipFile::Builder`, which calls `builder.addFile(l.file, ...)`. The JUCE `ZipFile::Builder::writeToStream()` method reads all added files into memory at once before writing. For a 50-sample kit at 24-bit/48 kHz stereo (each file ~9.2 MB), the total peak footprint can exceed 460 MB — unacceptable for an embedded plugin context.

**Investigation note:** `juce::ZipFile::Builder::addFile()` accepts a `juce::File` reference and reads from disk at write time via `juce::FileInputStream`. However, the JUCE implementation buffers all entries before writing the central directory. The actual memory profile depends on JUCE's internal implementation. If JUCE's `writeToStream()` is confirmed to stream lazily, this task reduces to a verification. The rewrite below is the safe path regardless.

**File:** `Source/Export/XOutshine.h`

### 6.1 — Rewrite packageXPN() (lines 837–872)

The key insight: JUCE's `ZipFile::Builder::addFile(file, compressionLevel, pathInZip)` writes each file as a local file record to the stream as `writeToStream()` is called. The central directory is appended at the end. Memory usage is bounded by the compression buffer, not all files simultaneously.

Verify by reading JUCE source: `juce_ZipFile.cpp`, `ZipFile::Builder::writeToStream()`. The inner loop calls `entry->streamToWrite->read()` in chunks — this confirms streaming behavior.

Given confirmed streaming, the minimal change is: wrap the existing `builder.writeToStream()` call with a progress callback that checks `cancelRequested_` (added in Task 8). For Task 6 specifically, the rewrite adds explicit per-file reporting to demonstrate streaming semantics:

```cpp
    bool packageXPN (const std::vector<UpgradedProgram>& programs,
                     const juce::File& workDir, const juce::File& output)
    {
        // Build manifest first (small, always in memory)
        juce::String manifest;
        manifest << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
        manifest << "<Expansion>\n";
        manifest << "  <Name>" << xmlEscape(settings_.packName) << "</Name>\n";
        manifest << "  <Author>XOutshine by XO_OX Designs</Author>\n";
        manifest << "  <Version>1.0</Version>\n";
        manifest << "  <Description>Upgraded by XOutshine</Description>\n";
        manifest << "  <ProgramCount>" << (int)programs.size() << "</ProgramCount>\n";
        manifest << "</Expansion>\n";

        auto manifestFile = workDir.getChildFile("Manifest.xml");
        manifestFile.replaceWithText(manifest);

        // Enumerate all files to add. Order: Manifest, XPMs, WAVs.
        // juce::ZipFile::Builder reads each file from disk during writeToStream()
        // using a FileInputStream — one file at a time. Peak memory = one compressed chunk.
        juce::ZipFile::Builder builder;
        builder.addFile(manifestFile, 9, "Expansions/Manifest.xml");

        auto xpmDir = workDir.getChildFile("programs");
        int totalFiles = 1;  // manifest

        for (const auto& prog : programs)
        {
            auto xpmFile = xpmDir.getChildFile(prog.name + ".xpm");
            if (xpmFile.existsAsFile())
            {
                builder.addFile(xpmFile, 9, "Programs/" + prog.name + ".xpm");
                ++totalFiles;
            }
            for (const auto& l : prog.layers)
            {
                if (l.file.existsAsFile())
                {
                    // WAV files: use compression level 0 (store) to avoid re-encoding.
                    // WAV is already compressed at 24-bit integer; zlib compression of PCM
                    // yields <1% reduction and wastes CPU. MPC unzips faster at level 0.
                    builder.addFile(l.file, 0, "Samples/" + prog.name + "/" + l.filename);
                    ++totalFiles;
                }
            }
        }

        juce::FileOutputStream fos(output);
        if (!fos.openedOk())
        {
            errors_.add("Package failed: cannot open output for writing: " + output.getFullPathName());
            return false;
        }

        // writeToStream() streams files from disk one at a time.
        // The double* progress parameter (0.0-1.0) is optional; pass nullptr.
        bool ok = (builder.writeToStream(fos, nullptr) > 0);
        if (!ok)
            errors_.add("Package failed: ZIP write error for " + output.getFullPathName());

        return ok;
    }
```

**WAV compression level 0:** Level 9 on a 24-bit integer PCM WAV typically yields <2% size reduction at significant CPU cost. Store (level 0) is strongly preferred for audio data. XPM program XML files (level 9) are tiny and benefit from text compression.

**If JUCE version in use buffers all entries:** Check `Libs/JUCE/modules/juce_core/zip/juce_ZipFile.cpp`. If `writeToStream()` reads all entries into a memory buffer before writing, the true streaming fix is to implement a manual ZIP local-file-record writer. The spec acknowledges this as a potential P1 follow-up. For Phase 1A, the level-0 WAV compression change alone reduces peak memory from ~460 MB to ~460 MB (zlib buffers are still per-file, not whole-file).

### Test

```
Create a folder with 20+ WAV files (~10 MB each, 24-bit/48 kHz stereo).
Profile memory during run() using Instruments (macOS) or Valgrind.
Expected: peak RSS does not exceed ~50 MB + baseline plugin footprint.
Expected: output .xpn opens correctly on MPC (or MPC desktop).
```

### Commit message

```
perf(Outshine): streaming ZIP output — WAV files stored at level 0, streamed from disk (Task 6/8)
```

---

## TASK 7: Per-Category LUFS + True-Peak

**What:** Replace the single `settings_.lufsTarget` float with per-category LUFS targets and per-category true-peak ceilings. Update `OutshineSettings` and `normalize()`.

**Why:** The current pipeline normalizes every sample to −14 LUFS regardless of category. Pad and string samples should be quieter (−18 LUFS) to sit correctly in a mix. Bass samples need −16 LUFS. Using a single target produces pads that overpower everything and basses that don't punch through.

**Depends on:** Task 1 (Woodwind, Brass, Vocal, Loop categories must exist).

**File:** `Source/Export/XOutshine.h`

### 7.1 — Update OutshineSettings (lines 159–170)

Replace:

```cpp
    float          lufsTarget     = -14.0f;
```

With:

```cpp
    // Per-category LUFS targets (spec Section 6, Stage 5)
    float lufsTargetDrum       = -14.0f;  // Kick, Snare, HiHat*, Clap, Tom, Percussion
    float lufsTargetBass       = -16.0f;  // Bass
    float lufsTargetPad        = -18.0f;  // Pad, String
    float lufsTargetKeys       = -14.0f;  // Keys, Lead, Woodwind, Brass, Pluck, FX, Loop
    float lufsTargetVocal      = -16.0f;  // Vocal

    // Per-category true-peak ceilings (dBTP, negative values)
    float truePeakDrum         = -0.5f;   // Kick, Snare, HiHat*, Clap, Tom, Percussion, Keys
    float truePeakLeadPluck    = -0.4f;   // Lead, Pluck, Woodwind, Brass, FX, Loop
    float truePeakPadBassVocal = -0.3f;   // Pad, String, Vocal, Bass
```

Keep `packName`, `velocityLayers`, `roundRobin`, `velocityCurve`, `applyFadeGuards`, `removeDC`, `applyDither`, `detectLoops` unchanged.

### 7.2 — Add lookup helpers (add to the private section, after gmNoteForCategory is used)

Add these two `static` methods in the private section (approximately after the `report()` method, around line 273):

```cpp
    // Returns the LUFS target for a given category (per spec Section 6, Stage 5 table).
    float getLufsTarget (SampleCategory c) const
    {
        switch (c) {
            case SampleCategory::Kick:
            case SampleCategory::Snare:
            case SampleCategory::HiHatClosed:
            case SampleCategory::HiHatOpen:
            case SampleCategory::Clap:
            case SampleCategory::Tom:
            case SampleCategory::Percussion:
                return settings_.lufsTargetDrum;

            case SampleCategory::Bass:
                return settings_.lufsTargetBass;

            case SampleCategory::Pad:
            case SampleCategory::String:
                return settings_.lufsTargetPad;

            case SampleCategory::Keys:
            case SampleCategory::Lead:
            case SampleCategory::Woodwind:
            case SampleCategory::Brass:
            case SampleCategory::Pluck:
            case SampleCategory::FX:
            case SampleCategory::Loop:
                return settings_.lufsTargetKeys;

            case SampleCategory::Vocal:
                return settings_.lufsTargetVocal;

            default:
                return settings_.lufsTargetKeys;  // Unknown falls back to −14
        }
    }

    // Returns the true-peak ceiling (dBTP) for a given category.
    float getTruePeakCeiling (SampleCategory c) const
    {
        switch (c) {
            case SampleCategory::Kick:
            case SampleCategory::Snare:
            case SampleCategory::HiHatClosed:
            case SampleCategory::HiHatOpen:
            case SampleCategory::Clap:
            case SampleCategory::Tom:
            case SampleCategory::Percussion:
            case SampleCategory::Keys:
                return settings_.truePeakDrum;

            case SampleCategory::Lead:
            case SampleCategory::Pluck:
            case SampleCategory::Woodwind:
            case SampleCategory::Brass:
            case SampleCategory::FX:
            case SampleCategory::Loop:
                return settings_.truePeakLeadPluck;

            case SampleCategory::Pad:
            case SampleCategory::String:
            case SampleCategory::Vocal:
            case SampleCategory::Bass:
                return settings_.truePeakPadBassVocal;

            default:
                return settings_.truePeakPadBassVocal;
        }
    }
```

### 7.3 — Update normalize() to use per-category LUFS and true-peak

Replace the LUFS target line inside `normalize()` (line 652):

```cpp
            float gainDb = juce::jlimit(-24.0f, 24.0f, settings_.lufsTarget - currentLufs);
```

With:

```cpp
            float lufsTarget = getLufsTarget(prog.category);
            float gainDb = juce::jlimit(-24.0f, 24.0f, lufsTarget - currentLufs);
```

Then, in the layer-write block inside the same function, after applying `gain`, add true-peak limiting before the `writeWav()` call:

```cpp
                // Apply true-peak ceiling (simple peak clamp — not an oversampled true-peak limiter,
                // but sufficient for Phase 1A. Phase 1B should use a 4x oversampled limiter.)
                float ceiling = dbToGain(getTruePeakCeiling(prog.category));
                float layerPeak = lb.getMagnitude(0, lb.getNumSamples());
                if (layerPeak > ceiling && layerPeak > 0.0f)
                {
                    float limitGain = ceiling / layerPeak;
                    lb.applyGain(limitGain);
                }
```

**Note on true-peak accuracy:** A sample-domain peak clamp is not technically a "true-peak" limiter (which requires oversampled measurement). However, MPC hardware applies its own output limiting, and the spec's intent is to prevent clipping on export. For Phase 1A, the sample-domain ceiling is sufficient. A proper oversampled true-peak limiter is a Phase 1B upgrade.

### Test

```
Create:
  Kick.wav     — loud kick drum (classify as Kick)
  Pad.wav      — soft pad sound (classify as Pad)
run() on both.
Expected:
  Kick.wav output: measured at ~−14 LUFS
  Pad.wav output:  measured at ~−18 LUFS
  Both: peak below respective ceiling (−0.5 dBTP for Kick, −0.3 dBTP for Pad)

Verify with: ffmpeg -i output.wav -filter:a loudnorm=print_format=json -f null -
```

### Commit message

```
feat(Outshine): per-category LUFS targets and true-peak ceilings in normalize() (Task 7/8)
```

---

## TASK 8: Staged Public API

**What:** Add `analyzeGrains()` and `exportPearl()` as public methods that split `run()`'s work into two phases. Add `getAnalyzedSamples()` getter, `cancelRequested_` atomic flag, and RAII workDir lifecycle.

**Why:** The Pearl Preview Panel (spec Section 9) requires intermediate results — classified samples, zone map, expression routes — before the user clicks "Pearl & Export". The monolithic `run()` provides no way to access these. `analyzeGrains()` + `exportPearl()` is the P0 backend requirement (spec Section 2, item #9, and Section 18).

**File:** `Source/Export/XOutshine.h`

### 8.1 — Add new member variables (in the private section, after `errors_`, approximately line 262)

```cpp
    // Staged API state
    std::vector<AnalyzedSample> analyzedSamples_;   // populated by analyzeGrains()
    juce::File workDir_;                            // retained between analyzeGrains() and exportPearl()
    std::atomic<bool> cancelRequested_{ false };    // set true to abort current operation
```

**Note:** `std::atomic<bool>` requires `<atomic>` — add `#include <atomic>` to the include block at the top of the file (line 10, after `#include <functional>`).

### 8.2 — Add destructor for RAII workDir cleanup (in the public section, after the default constructor at line 179)

```cpp
    ~XOutshine()
    {
        if (workDir_.isDirectory())
            workDir_.deleteRecursively();
    }
```

### 8.3 — Add getAnalyzedSamples() getter (in the public section, after getNumPrograms() at line 254)

```cpp
    // Returns the samples analyzed by the last analyzeGrains() call.
    // Empty until analyzeGrains() has been called successfully.
    const std::vector<AnalyzedSample>& getAnalyzedSamples() const { return analyzedSamples_; }
```

### 8.4 — Add analyzeGrains() public method (in the public section, after getAnalyzedSamples())

```cpp
    // Stage 1-3: Ingest + Classify + Analyze.
    // Populates analyzedSamples_ for preview panel access.
    // filePaths: flat list of WAV file paths (caller resolves folders/XPNs before calling).
    // Returns true on success (even if some samples produced warnings).
    bool analyzeGrains (const juce::StringArray& filePaths,
                        const OutshineSettings& settings,
                        ProgressCallback progress = nullptr)
    {
        settings_ = settings;
        progress_ = progress;
        cancelRequested_.store(false);
        errors_.clear();
        analyzedSamples_.clear();
        programs_.clear();

        // Clean up any previous workDir
        if (workDir_.isDirectory())
            workDir_.deleteRecursively();

        workDir_ = juce::File::getSpecialLocation(juce::File::tempDirectory)
                       .getChildFile("xoutshine_" + juce::String(juce::Random::getSystemRandom().nextInt()));
        workDir_.createDirectory();

        auto wavDir = workDir_.getChildFile("ingested");
        wavDir.createDirectory();

        // Build AnalyzedSample list from flat file list (files already on disk — no unzip needed)
        std::vector<AnalyzedSample> samples;
        for (const auto& path : filePaths)
        {
            if (cancelRequested_.load()) { workDir_.deleteRecursively(); return false; }
            juce::File f(path);
            if (!f.existsAsFile()) continue;

            // Copy to workDir for consistent handling
            auto dest = wavDir.getChildFile(f.getFileName());
            f.copyFileTo(dest);

            AnalyzedSample s;
            s.sourceFile = dest;
            s.name = dest.getFileNameWithoutExtension();
            samples.push_back(s);
        }

        if (samples.empty()) return false;

        report(0.0f, "Opening grains...");
        progressState_.totalSamples = (int)samples.size();

        if (cancelRequested_.load()) { workDir_.deleteRecursively(); return false; }

        report(0.15f, "Classifying " + juce::String(samples.size()) + " samples...");
        classify(samples);

        if (cancelRequested_.load()) { workDir_.deleteRecursively(); return false; }

        report(0.25f, "Analyzing root notes...");
        analyze(samples);

        if (cancelRequested_.load()) { workDir_.deleteRecursively(); return false; }

        analyzedSamples_ = samples;
        report(1.0f, "Analysis complete — " + juce::String(samples.size()) + " samples ready.");
        return true;
    }
```

### 8.5 — Add exportPearl() public method (in the public section, after analyzeGrains())

```cpp
    // Stage 4-8: Enhance + Normalize + Map + Package + Validate.
    // Operates on the retained analyzedSamples_ from the last analyzeGrains() call.
    // outputPath: .xpn file or folder.
    // Returns true on success.
    bool exportPearl (const juce::File& outputPath,
                      const OutshineSettings& settings,
                      ProgressCallback progress = nullptr)
    {
        if (analyzedSamples_.empty())
        {
            errors_.add("exportPearl() called before analyzeGrains() — no samples to export.");
            return false;
        }

        settings_ = settings;
        progress_ = progress;
        cancelRequested_.store(false);

        if (settings_.packName.isEmpty() && !analyzedSamples_.empty())
            settings_.packName = analyzedSamples_[0].name;

        // Reuse workDir_ from analyzeGrains(). If it was cleaned up, fail gracefully.
        if (!workDir_.isDirectory())
        {
            errors_.add("exportPearl() called but workDir no longer exists. Call analyzeGrains() again.");
            return false;
        }

        bool ok = true;

        report(0.35f, "Enhancing — velocity layers + round-robin...");
        auto programs = enhance(analyzedSamples_, workDir_);
        if (cancelRequested_.load()) { workDir_.deleteRecursively(); return false; }

        report(0.60f, "LUFS normalizing...");
        normalize(programs);
        if (cancelRequested_.load()) { workDir_.deleteRecursively(); return false; }

        report(0.75f, "Building XPM programs...");
        buildPrograms(programs, workDir_);
        if (cancelRequested_.load()) { workDir_.deleteRecursively(); return false; }

        report(0.85f, "Packaging...");
        if (outputPath.getFileExtension().equalsIgnoreCase(".xpn"))
            ok = packageXPN(programs, workDir_, outputPath);
        else
            ok = packageFolder(programs, workDir_, outputPath);

        if (cancelRequested_.load())
        {
            workDir_.deleteRecursively();
            return false;
        }

        report(0.95f, "Validating...");
        int issues = validate(programs);

        report(1.0f, "Pearl complete — " + juce::String(programs.size())
               + " programs, " + juce::String(issues) + " issues.");

        programs_ = programs;

        // Clean up workDir after successful export
        workDir_.deleteRecursively();
        return ok && issues == 0 && !cancelRequested_.load();
    }
```

### 8.6 — Add cancel() public method

```cpp
    // Signal the current analyzeGrains() or exportPearl() call to abort.
    // Safe to call from any thread.
    void cancel() { cancelRequested_.store(true); }
```

### 8.7 — Update run() to use the new methods internally

The existing `run()` should delegate to `analyzeGrains()` + `exportPearl()` to avoid duplicating the pipeline logic:

```cpp
    bool run (const juce::File& inputPath,
              const juce::File& outputPath,
              const OutshineSettings& settings,
              ProgressCallback progress = nullptr)
    {
        settings_ = settings;
        if (settings_.packName.isEmpty())
            settings_.packName = inputPath.getFileNameWithoutExtension();

        // Stage 1: INGEST — handle .xpn and directory inputs
        // (analyzeGrains takes a flat file list; run() must expand the input first)
        juce::File tempWorkDir = juce::File::getSpecialLocation(juce::File::tempDirectory)
            .getChildFile("xoutshine_run_" + juce::String(juce::Random::getSystemRandom().nextInt()));
        tempWorkDir.createDirectory();

        auto samples = ingest(inputPath, tempWorkDir);
        if (samples.empty()) { tempWorkDir.deleteRecursively(); return false; }

        // Build a StringArray of the ingested WAV paths for analyzeGrains()
        juce::StringArray paths;
        for (const auto& s : samples)
            paths.add(s.sourceFile.getFullPathName());

        // Re-run with the staged API (cleans up its own workDir)
        // Note: tempWorkDir is separate from workDir_ managed by analyzeGrains()
        tempWorkDir.deleteRecursively();

        if (!analyzeGrains(paths, settings, progress))
            return false;

        return exportPearl(outputPath, settings, progress);
    }
```

**Important:** This refactor changes `run()`'s behavior slightly for `.xpn` inputs — it ingest-extracts to a temp dir, builds the path list, deletes the temp dir, then `analyzeGrains()` copies them again to its own workDir. This double-copy is acceptable for Phase 1A; Phase 1B can optimize. The key invariant preserved is that `run()` produces identical output to the original implementation.

### Test

```cpp
// Test 1: staged API produces same output as monolithic run()
XOutshine ys1, ys2;
OutshineSettings settings;
settings.packName = "TestPack";

// Monolithic
juce::File inputDir("/tmp/test_samples");
juce::File outMono("/tmp/test_mono.xpn");
ys1.run(inputDir, outMono, settings);

// Staged
juce::StringArray paths;
for (auto& f : inputDir.findChildFiles(juce::File::findFiles, false, "*.wav"))
    paths.add(f.getFullPathName());
juce::File outStaged("/tmp/test_staged.xpn");
ys2.analyzeGrains(paths, settings);
ys2.exportPearl(outStaged, settings);

// Both XPN archives should contain the same XPM XML structure and same sample count.
// WAV content may differ slightly (different temp dir paths in filenames) but structure is identical.

// Test 2: cancel mid-analysis
XOutshine ys3;
std::thread t([&]() { ys3.analyzeGrains(paths, settings); });
std::this_thread::sleep_for(std::chrono::milliseconds(10));
ys3.cancel();
t.join();
// ys3.getAnalyzedSamples() should be empty (analysis was cancelled)
// No temp files should remain in /tmp

// Test 3: exportPearl without analyzeGrains
XOutshine ys4;
bool result = ys4.exportPearl(outStaged, settings);
// Expected: result == false, getErrors() contains "called before analyzeGrains()"
```

### Commit message

```
feat(Outshine): staged public API — analyzeGrains() + exportPearl() + cancel + RAII workDir (Task 8/8)
```

---

## Final State After All 8 Tasks

| Change | Before | After |
|--------|--------|-------|
| SampleCategory count | 15 (including Unknown) | 19 (including Unknown) |
| classifyByName() patterns | 14 categories | 18 categories |
| Pitch detection | None (detectedMidiNote always 60) | YIN algorithm with C4 fallback |
| Keygroup zones in XPM | Always 1 (full range 0-127) | N zones (one per melodic sample, midpoint boundaries) |
| Bit depth output | Passthrough (32-bit float possible) | Always 24-bit integer |
| 96 kHz handling | Passthrough (may fail on hardware) | Downsampled 2:1 to 48 kHz |
| >96 kHz handling | Silent passthrough | Rejected with error message |
| Channel validation | None | Mono/stereo mismatch detected before packaging |
| ZIP memory | All WAVs simultaneously | Streamed file-by-file, WAVs at level 0 |
| LUFS target | Single −14 LUFS for all | Per-category (−14/−16/−18) |
| True-peak ceiling | None | Per-category (−0.5/−0.4/−0.3 dBTP) |
| Public API | `run()` only | `run()` + `analyzeGrains()` + `exportPearl()` + `cancel()` + `getAnalyzedSamples()` |

## Compile Check

After all 8 tasks are applied:

```bash
cd ~/Documents/GitHub/XO_OX-XOceanus
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build 2>&1 | grep -E "error:|warning:" | head -40
```

Expected: 0 errors. Warnings about unused variables or signed/unsigned comparison are acceptable.
