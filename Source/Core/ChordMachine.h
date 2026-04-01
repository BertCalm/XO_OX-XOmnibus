#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include <algorithm>
#include <atomic>
#include <climits>
#include <cmath>

namespace xolokun {

//==============================================================================
// Chord Machine — distributes chord voicings across XOlokun's 4 primary engine slots.
//
// One MIDI note in → 4 engine-specific MIDI notes out (primary slots only).
// Each slot voices a different chord tone through its own synthesis engine.
// The Ghost Slot (slot 4) does not participate in chord distribution.
//
// Thread safety:
//   - processBlock() runs on the audio thread — no allocation, no locks.
//   - State setters use std::atomic for cross-thread visibility.
//   - Voice leading uses brute-force permutation (24 iterations for 4 slots).
//
// kChordSlots: the number of primary engine slots (fixed at 4).
// The outputMidi array size is parameterised via kOutputMidiSlots so that the
// caller (XOlokunProcessor) can pass the full MaxSlots array — ChordMachine
// only writes into indices 0..kChordSlots-1.
static constexpr int kChordSlots       = 4;   // number of chord voices / primary slots
static constexpr int kOutputMidiSlots  = 5;   // output array size = XOlokunProcessor::MaxSlots
//

//==============================================================================
enum class PaletteType : int {
    Warm = 0,       // Minor 7th (deep house, emotional)
    Bright,         // Major 7th (disco, soulful)
    Tension,        // Dominant 7th (acid, tech)
    Open,           // Sus4 + octave (atmospheric, progressive)
    Dark,           // Minor 9th (dark house, minimal)
    Sweet,          // Add9 (lo-fi, nu-disco)
    Complex,        // 11th (jazz house, broken beat)
    Raw,            // Power chord (industrial, hard techno)
    NumPalettes
};

enum class VoicingMode : int {
    RootSpread = 0, // Root anchored low, others spread upward
    Drop2,          // 2nd-from-top dropped down an octave (jazz, neo-soul)
    Quartal,        // Stacked perfect 4ths (modern house, techno)
    UpperStructure, // Root low + upper voices an octave up
    Unison,         // All slots play the same note
    NumModes
};

enum class RhythmPattern : int {
    Four = 0,       // X...X...X...X... (four-on-the-floor stab)
    Off,            // ..X...X...X...X. (off-beat classic house)
    Synco,          // X..X..X...X..X.. (syncopated deep house)
    ChordStab,      // X.X...X.X...X.X. (Daft Punk-style rhythmic)
    Gate,           // XXXXXXXXXXXXXXXX  (sustained pad, gate = 1.0)
    Pulse,          // X.......X....... (half-note hits)
    Broken,         // X..X.X....X..X.. (broken beat / UK garage)
    Rest,           // ................ (silent, for breakdowns)
    NumPatterns
};

enum class VelocityCurve : int {
    Equal = 0,      // 100/100/100/100 — flat
    RootHeavy,      // 100/70/60/50 — bass-forward (house default)
    TopBright,      // 60/70/80/100 — upper tones dominate
    VShape,         // 100/60/60/100 — root + top, mid recessed
    NumCurves
};

//==============================================================================
struct StepData {
    int rootNote = -1;      // -1 = use live root / global root
    bool active = true;     // step on/off
    float gate = 0.75f;     // duration as fraction of step length
    float velocity = 0.9f;  // 0-1
};

//==============================================================================
struct ChordAssignment {
    std::array<int, 4> midiNotes {{ -1, -1, -1, -1 }};    // -1 = silent
    std::array<float, 4> velocities {{ 0.f, 0.f, 0.f, 0.f }};
};

//==============================================================================
// ChordDistributor — computes which MIDI note each slot should play.
//
// Stateless: all methods are const and operate on inputs only.
// Audio-thread safe: no allocation, O(1) lookup tables.
//
class ChordDistributor {
public:

    //-- Distribution: palette + voicing + spread → 4 notes -------------------

    ChordAssignment distribute (int rootNote, PaletteType palette,
                                VoicingMode voicing, float spread,
                                float velocity) const
    {
        ChordAssignment result;

        // 1. Get 4 intervals from palette
        const int palIdx = std::max (0, std::min (static_cast<int> (PaletteType::NumPalettes) - 1,
                                                   static_cast<int> (palette)));
        std::array<int, 4> notes;
        for (int i = 0; i < 4; ++i)
            notes[i] = rootNote + kPaletteIntervals[palIdx][i];

        // 2. Apply SPREAD — how many unique chord tones
        applySpread (notes, rootNote, spread);

        // 3. Apply voicing — rearranges note layout
        applyVoicing (notes, rootNote, voicing);

        // 4. Clamp to valid MIDI range and assign
        for (int i = 0; i < 4; ++i)
        {
            notes[i] = std::max (0, std::min (127, notes[i]));
            result.midiNotes[i] = notes[i];
            result.velocities[i] = velocity;
        }

        return result;
    }

    //-- Voice Leading: minimize total pitch movement between chords ----------
    //
    // Brute-force bipartite matching over all 24 permutations of 4 slots.
    // Slot 0 (root) gets a 12-semitone penalty for moving away from
    // its natural position, encouraging root stability.

    ChordAssignment voiceLead (const ChordAssignment& previous,
                               const ChordAssignment& target) const
    {
        // Check if previous had any valid notes
        bool hasPrevious = false;
        for (int i = 0; i < 4; ++i)
            if (previous.midiNotes[i] >= 0) { hasPrevious = true; break; }

        if (! hasPrevious)
            return target;

        std::array<int, 4> bestPerm = {{ 0, 1, 2, 3 }};
        int bestCost = INT_MAX;
        std::array<int, 4> perm = {{ 0, 1, 2, 3 }};

        do {
            int cost = 0;
            for (int i = 0; i < 4; ++i)
            {
                if (previous.midiNotes[i] < 0 || target.midiNotes[perm[i]] < 0)
                    continue;

                int diff = std::abs (previous.midiNotes[i] - target.midiNotes[perm[i]]);
                cost += diff;

                // Root-slot affinity: penalize moving root to non-root position
                if (i == 0 && perm[i] != 0)
                    cost += 12;
            }

            if (cost < bestCost)
            {
                bestCost = cost;
                bestPerm = perm;
            }
        } while (std::next_permutation (perm.begin(), perm.end()));

        // Apply best permutation
        ChordAssignment result;
        for (int i = 0; i < 4; ++i)
        {
            result.midiNotes[i] = target.midiNotes[bestPerm[i]];
            result.velocities[i] = target.velocities[bestPerm[i]];
        }

        return result;
    }

private:
    // Palette interval tables: 4 semitone offsets from root.
    // Each row defines the character of a chord quality.
    static constexpr int kPaletteIntervals[8][4] = {
        { 0,  3,  7, 10 },   // Warm     — minor 7th
        { 0,  4,  7, 11 },   // Bright   — major 7th
        { 0,  4,  7, 10 },   // Tension  — dominant 7th
        { 0,  5,  7, 12 },   // Open     — sus4 + octave
        { 0,  3,  7, 14 },   // Dark     — minor 9th (9th = root + 14)
        { 0,  4,  7, 14 },   // Sweet    — add9
        { 0,  4, 10, 17 },   // Complex  — 11th (M3, m7, 11th)
        { 0,  7, 12, 19 },   // Raw      — root, 5th, octave, 5th+oct
    };

    //-- SPREAD: controls how many unique chord tones are active ---------------
    //
    //  0.00 - 0.20:  UNISON — all 4 slots play root
    //  0.20 - 0.40:  ROOT+5TH — 2 unique notes
    //  0.40 - 0.60:  TRIAD — 3 unique notes (slot 4 doubles 5th)
    //  0.60 - 0.80:  FULL — all 4 palette notes
    //  0.80 - 1.00:  WIDE — 4 notes, upper voices shifted up an octave

    void applySpread (std::array<int, 4>& notes, int root, float spread) const
    {
        if (spread < 0.2f)
        {
            notes[0] = notes[1] = notes[2] = notes[3] = root;
        }
        else if (spread < 0.4f)
        {
            int fifth = root + 7;
            notes[0] = root;
            notes[1] = root;
            notes[2] = fifth;
            notes[3] = fifth;
        }
        else if (spread < 0.6f)
        {
            // Triad: root + interval[1] + interval[2], slot 4 doubles interval[2]
            notes[3] = notes[2];
        }
        else if (spread >= 0.8f)
        {
            // Wide: push upper 3 voices up an octave
            notes[1] += 12;
            notes[2] += 12;
            notes[3] += 12;
        }
        // 0.6–0.8: full chord from palette, notes unchanged
    }

    //-- VOICING MODE: rearranges note layout ----------------------------------

    void applyVoicing (std::array<int, 4>& notes, int root, VoicingMode mode) const
    {
        switch (mode)
        {
            case VoicingMode::RootSpread:
                // Default close position ascending — already in place from palette.
                break;

            case VoicingMode::Drop2:
            {
                // Sort to close position, then drop the 2nd-from-top down an octave.
                std::sort (notes.begin(), notes.end());
                if (notes[2] >= 12)
                    notes[2] -= 12;
                std::sort (notes.begin(), notes.end());
                break;
            }

            case VoicingMode::Quartal:
            {
                // Stacked perfect 4ths from root — ignores palette intervals.
                notes[0] = root;
                notes[1] = root + 5;    // P4
                notes[2] = root + 10;   // P4 + P4
                notes[3] = root + 15;   // P4 + P4 + P4
                break;
            }

            case VoicingMode::UpperStructure:
            {
                // Root stays low, upper voices shifted up until they're
                // at least an octave above root. Bounded to prevent
                // runaway with extreme input values.
                notes[0] = root;
                for (int i = 1; i < 4; ++i)
                {
                    while (notes[i] < root + 12 && notes[i] < 120)
                        notes[i] += 12;
                }
                break;
            }

            case VoicingMode::Unison:
            {
                notes[0] = notes[1] = notes[2] = notes[3] = root;
                break;
            }

            default:
                break;
        }
    }
};

//==============================================================================
// Rhythm pattern tables: which of 16 steps are active.
// Applied when a pattern is selected; user can override individual steps.

namespace patterns {
    static constexpr bool kStepActive[8][16] = {
        { 1,0,0,0, 1,0,0,0, 1,0,0,0, 1,0,0,0 }, // FOUR
        { 0,0,1,0, 0,0,1,0, 0,0,1,0, 0,0,1,0 }, // OFF
        { 1,0,0,1, 0,0,1,0, 0,0,1,0, 0,1,0,0 }, // SYNCO
        { 1,0,1,0, 0,0,1,0, 1,0,0,0, 1,0,1,0 }, // CHORD-STAB
        { 1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1 }, // GATE
        { 1,0,0,0, 0,0,0,0, 1,0,0,0, 0,0,0,0 }, // PULSE
        { 1,0,0,1, 0,1,0,0, 0,0,1,0, 0,1,0,0 }, // BROKEN
        { 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0 }, // REST
    };
} // namespace patterns

//==============================================================================
// Velocity curve tables: per-slot velocity multipliers (0.0 – 1.0).
namespace velcurves {
    static constexpr float kCurves[4][4] = {
        { 1.0f, 1.0f, 1.0f, 1.0f },   // Equal
        { 1.0f, 0.7f, 0.6f, 0.5f },   // RootHeavy
        { 0.6f, 0.7f, 0.8f, 1.0f },   // TopBright
        { 1.0f, 0.6f, 0.6f, 1.0f },   // VShape
    };
} // namespace velcurves

//==============================================================================
// ChordMachine — the main MIDI processing hub.
//
// Two operating modes:
//   1. LIVE (sequencer off): incoming MIDI noteOn/Off triggers chord distribution.
//   2. SEQUENCER (sequencer on): 16-step clock triggers chords. Live MIDI
//      sets the root note that steps follow.
//
// Monophonic by design (one chord at a time). Voice leading smooths transitions.
//
class ChordMachine {
public:
    static constexpr int NumSteps = 16;

    void prepare (double sampleRate, int /*maxBlockSize*/)
    {
        sr = sampleRate;
        resetSequencer();
    }

    //-- Main processing (audio thread) ----------------------------------------

    void processBlock (const juce::MidiBuffer& inputMidi,
                       std::array<juce::MidiBuffer, kOutputMidiSlots>& outputMidi,
                       int numSamples)
    {
        for (auto& buf : outputMidi)
            buf.clear();

        if (! enabled.load (std::memory_order_relaxed))
        {
            // Bypass: copy input to all 4 slots
            for (auto& buf : outputMidi)
                buf = inputMidi;
            return;
        }

        // Snapshot APVTS-driven state once per block (block-constant)
        const auto pal = static_cast<PaletteType> (
            palette.load (std::memory_order_relaxed));
        const auto voic = static_cast<VoicingMode> (
            voicing.load (std::memory_order_relaxed));
        const float spr = spread.load (std::memory_order_relaxed);
        const bool seqRunning = sequencerRunning.load (std::memory_order_relaxed);

        if (seqRunning)
            processSequencerMode (inputMidi, outputMidi, numSamples, pal, voic, spr);
        else
            processLiveMode (inputMidi, outputMidi, pal, voic, spr);
    }

    //-- State setters (message thread, read by audio thread via atomics) ------

    void setEnabled (bool on)    { enabled.store (on, std::memory_order_relaxed); }
    bool isEnabled() const       { return enabled.load (std::memory_order_relaxed); }

    void setPalette (PaletteType p) { palette.store (static_cast<int> (p), std::memory_order_relaxed); }
    PaletteType getPalette() const  { return static_cast<PaletteType> (palette.load (std::memory_order_relaxed)); }

    void setVoicing (VoicingMode v) { voicing.store (static_cast<int> (v), std::memory_order_relaxed); }
    VoicingMode getVoicing() const  { return static_cast<VoicingMode> (voicing.load (std::memory_order_relaxed)); }

    void setSpread (float s)    { spread.store (s, std::memory_order_relaxed); }
    float getSpread() const     { return spread.load (std::memory_order_relaxed); }

    //-- Sequencer setters (message thread) ------------------------------------

    void setSequencerRunning (bool on)
    {
        sequencerRunning.store (on, std::memory_order_relaxed);
        if (! on) needsSeqReset.store (true, std::memory_order_relaxed);
    }
    bool isSequencerRunning() const { return sequencerRunning.load (std::memory_order_relaxed); }

    void setBPM (float b)       { bpm.store (std::max (30.0f, std::min (300.0f, b)), std::memory_order_relaxed); }
    float getBPM() const        { return bpm.load (std::memory_order_relaxed); }

    void setSwing (float s)     { swing.store (std::max (0.0f, std::min (1.0f, s)), std::memory_order_relaxed); }
    float getSwing() const      { return swing.load (std::memory_order_relaxed); }

    void setGlobalGate (float g) { globalGate.store (std::max (0.01f, std::min (1.0f, g)), std::memory_order_relaxed); }
    float getGlobalGate() const  { return globalGate.load (std::memory_order_relaxed); }

    void setVelocityCurve (VelocityCurve c) { velocityCurve.store (static_cast<int> (c), std::memory_order_relaxed); }
    VelocityCurve getVelocityCurve() const  { return static_cast<VelocityCurve> (velocityCurve.load (std::memory_order_relaxed)); }

    void setHumanize (float h)      { humanize.store (std::max (0.0f, std::min (1.0f, h)), std::memory_order_relaxed); }
    float getHumanize() const       { return humanize.load (std::memory_order_relaxed); }

    void setSidechainDuck (float d)  { sidechainDuck.store (std::max (0.0f, std::min (1.0f, d)), std::memory_order_relaxed); }
    float getSidechainDuck() const   { return sidechainDuck.load (std::memory_order_relaxed); }

    // Apply a rhythm pattern preset (sets step active flags)
    void applyPattern (RhythmPattern p)
    {
        int idx = std::max (0, std::min (static_cast<int> (RhythmPattern::NumPatterns) - 1,
                                          static_cast<int> (p)));
        for (int i = 0; i < NumSteps; ++i)
            steps[i].active = patterns::kStepActive[idx][i];
        activePattern.store (static_cast<int> (p), std::memory_order_relaxed);
    }
    RhythmPattern getPattern() const { return static_cast<RhythmPattern> (activePattern.load (std::memory_order_relaxed)); }

    // Per-step editing (message thread)
    void setStepRoot (int step, int root)    { if (step >= 0 && step < NumSteps) steps[step].rootNote = root; }
    void setStepActive (int step, bool on)   { if (step >= 0 && step < NumSteps) steps[step].active = on; }
    void setStepGate (int step, float g)     { if (step >= 0 && step < NumSteps) steps[step].gate = g; }
    void setStepVelocity (int step, float v) { if (step >= 0 && step < NumSteps) steps[step].velocity = v; }
    StepData getStep (int step) const        { return (step >= 0 && step < NumSteps) ? steps[step] : StepData{}; }

    // DAW sync: call from processor before processBlock when host transport is available
    void syncToHost (double ppqPosition, double hostBPM, bool hostPlaying)
    {
        if (hostPlaying && hostBPM > 0.0)
        {
            bpm.store (static_cast<float> (hostBPM), std::memory_order_relaxed);

            // Compute step position from PPQ (4 steps per beat = 16th notes)
            double stepPos = ppqPosition * 4.0;
            int newStep = static_cast<int> (std::fmod (stepPos, 16.0));
            if (newStep < 0) newStep += 16;

            // Only hard-sync if we're significantly off (avoids jitter)
            if (std::abs (newStep - seqCurrentStep) > 1
                || (newStep == 0 && seqCurrentStep == 15))
            {
                seqCurrentStep = newStep;
                double frac = stepPos - std::floor (stepPos);
                seqSamplesUntilStep = static_cast<int> ((1.0 - frac) * computeBaseSamplesPerStep());
            }
        }
    }

    // Read-only access for UI visualization (message thread)
    ChordAssignment getCurrentAssignment() const { return currentAssignment; }
    bool hasChord() const { return hasActiveChord; }
    int getCurrentStep() const { return seqCurrentStep; }
    int getLiveRoot() const { return liveRoot; }

    //-- Label helpers (UI convenience) ----------------------------------------

    static const char* paletteName (PaletteType p)
    {
        static const char* names[] = { "WARM", "BRIGHT", "TENSION", "OPEN",
                                        "DARK", "SWEET", "COMPLEX", "RAW" };
        int i = static_cast<int> (p);
        return (i >= 0 && i < 8) ? names[i] : "?";
    }

    static const char* voicingName (VoicingMode v)
    {
        static const char* names[] = { "ROOT-SPREAD", "DROP-2", "QUARTAL",
                                        "UPPER STRUCT", "UNISON" };
        int i = static_cast<int> (v);
        return (i >= 0 && i < 5) ? names[i] : "?";
    }

    static const char* patternName (RhythmPattern p)
    {
        static const char* names[] = { "FOUR", "OFF-BEAT", "SYNCO", "STAB",
                                        "GATE", "PULSE", "BROKEN", "REST" };
        int i = static_cast<int> (p);
        return (i >= 0 && i < 8) ? names[i] : "?";
    }

    static const char* velocityCurveName (VelocityCurve c)
    {
        static const char* names[] = { "EQUAL", "ROOT HEAVY", "TOP BRIGHT", "V-SHAPE" };
        int i = static_cast<int> (c);
        return (i >= 0 && i < 4) ? names[i] : "?";
    }

    static const char* spreadLabel (float sp)
    {
        if (sp < 0.2f)  return "UNISON";
        if (sp < 0.4f)  return "POWER";
        if (sp < 0.6f)  return "TRIAD";
        if (sp < 0.8f)  return "7TH";
        return "WIDE";
    }

    static juce::String midiNoteToName (int note)
    {
        if (note < 0 || note > 127) return "-";
        static const char* names[] = { "C", "C#", "D", "Eb", "E", "F",
                                        "F#", "G", "Ab", "A", "Bb", "B" };
        return juce::String (names[note % 12]) + juce::String ((note / 12) - 1);
    }

    //-- Eno mode setters -------------------------------------------------------

    void setEnoMode (bool on)    { enoMode.store (on, std::memory_order_relaxed); }
    bool isEnoMode() const       { return enoMode.load (std::memory_order_relaxed); }

    //-- Serialization (message thread) ----------------------------------------

    // Serialize full Chord Machine state to a juce::var (for .xometa storage).
    juce::var serializeState() const
    {
        auto* obj = new juce::DynamicObject();

        obj->setProperty ("enabled",        isEnabled());
        obj->setProperty ("palette",        static_cast<int> (getPalette()));
        obj->setProperty ("voicing",        static_cast<int> (getVoicing()));
        obj->setProperty ("spread",         static_cast<double> (getSpread()));
        obj->setProperty ("velocityCurve",  static_cast<int> (getVelocityCurve()));
        obj->setProperty ("humanize",       static_cast<double> (getHumanize()));
        obj->setProperty ("sidechainDuck",  static_cast<double> (getSidechainDuck()));
        obj->setProperty ("enoMode",        isEnoMode());

        // Sequencer state
        auto* seqObj = new juce::DynamicObject();
        seqObj->setProperty ("running",  isSequencerRunning());
        seqObj->setProperty ("bpm",      static_cast<double> (getBPM()));
        seqObj->setProperty ("swing",    static_cast<double> (getSwing()));
        seqObj->setProperty ("gate",     static_cast<double> (getGlobalGate()));
        seqObj->setProperty ("pattern",  static_cast<int> (getPattern()));

        // Per-step data
        juce::var stepsArray;
        for (int i = 0; i < NumSteps; ++i)
        {
            auto* stepObj = new juce::DynamicObject();
            stepObj->setProperty ("root",     steps[i].rootNote);
            stepObj->setProperty ("active",   steps[i].active);
            stepObj->setProperty ("gate",     static_cast<double> (steps[i].gate));
            stepObj->setProperty ("velocity", static_cast<double> (steps[i].velocity));
            stepsArray.append (juce::var (stepObj));
        }
        seqObj->setProperty ("steps", stepsArray);
        obj->setProperty ("sequence", juce::var (seqObj));

        return juce::var (obj);
    }

    // Restore Chord Machine state from a juce::var (loaded from .xometa).
    void restoreState (const juce::var& state)
    {
        if (! state.isObject())
            return;

        auto* obj = state.getDynamicObject();
        if (obj == nullptr)
            return;

        if (obj->hasProperty ("enabled"))
            setEnabled (static_cast<bool> (obj->getProperty ("enabled")));
        if (obj->hasProperty ("palette"))
        {
            // Range-check before cast to avoid UB from malformed preset data (issue #425).
            int idx = static_cast<int> (obj->getProperty ("palette"));
            if (idx >= 0 && idx < static_cast<int> (PaletteType::NumPalettes))
                setPalette (static_cast<PaletteType> (idx));
        }
        if (obj->hasProperty ("voicing"))
        {
            int idx = static_cast<int> (obj->getProperty ("voicing"));
            if (idx >= 0 && idx < static_cast<int> (VoicingMode::NumModes))
                setVoicing (static_cast<VoicingMode> (idx));
        }
        if (obj->hasProperty ("spread"))
            setSpread (static_cast<float> (obj->getProperty ("spread")));
        if (obj->hasProperty ("velocityCurve"))
        {
            int idx = static_cast<int> (obj->getProperty ("velocityCurve"));
            if (idx >= 0 && idx < static_cast<int> (VelocityCurve::NumCurves))
                setVelocityCurve (static_cast<VelocityCurve> (idx));
        }
        if (obj->hasProperty ("humanize"))
            setHumanize (static_cast<float> (obj->getProperty ("humanize")));
        if (obj->hasProperty ("sidechainDuck"))
            setSidechainDuck (static_cast<float> (obj->getProperty ("sidechainDuck")));
        if (obj->hasProperty ("enoMode"))
            setEnoMode (static_cast<bool> (obj->getProperty ("enoMode")));

        // Sequencer state
        if (obj->hasProperty ("sequence"))
        {
            auto seqVar = obj->getProperty ("sequence");
            if (seqVar.isObject())
            {
                auto* seqObj = seqVar.getDynamicObject();
                if (seqObj != nullptr)
                {
                    if (seqObj->hasProperty ("running"))
                        setSequencerRunning (static_cast<bool> (seqObj->getProperty ("running")));
                    if (seqObj->hasProperty ("bpm"))
                        setBPM (static_cast<float> (seqObj->getProperty ("bpm")));
                    if (seqObj->hasProperty ("swing"))
                        setSwing (static_cast<float> (seqObj->getProperty ("swing")));
                    if (seqObj->hasProperty ("gate"))
                        setGlobalGate (static_cast<float> (seqObj->getProperty ("gate")));
                    if (seqObj->hasProperty ("pattern"))
                    {
                        int idx = static_cast<int> (seqObj->getProperty ("pattern"));
                        if (idx >= 0 && idx < static_cast<int> (RhythmPattern::NumPatterns))
                            applyPattern (static_cast<RhythmPattern> (idx));
                    }

                    // Per-step data
                    if (seqObj->hasProperty ("steps"))
                    {
                        auto stepsVar = seqObj->getProperty ("steps");
                        if (stepsVar.isArray())
                        {
                            auto* arr = stepsVar.getArray();
                            int count = std::min (static_cast<int> (arr->size()), NumSteps);
                            for (int i = 0; i < count; ++i)
                            {
                                auto stepVar = (*arr)[i];
                                if (stepVar.isObject())
                                {
                                    auto* stepObj = stepVar.getDynamicObject();
                                    if (stepObj != nullptr)
                                    {
                                        if (stepObj->hasProperty ("root"))
                                            steps[i].rootNote = static_cast<int> (stepObj->getProperty ("root"));
                                        if (stepObj->hasProperty ("active"))
                                            steps[i].active = static_cast<bool> (stepObj->getProperty ("active"));
                                        if (stepObj->hasProperty ("gate"))
                                            steps[i].gate = static_cast<float> (stepObj->getProperty ("gate"));
                                        if (stepObj->hasProperty ("velocity"))
                                            steps[i].velocity = static_cast<float> (stepObj->getProperty ("velocity"));
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

private:

    //==========================================================================
    // LIVE MODE — incoming MIDI directly triggers chord distribution
    //==========================================================================

    void processLiveMode (const juce::MidiBuffer& inputMidi,
                          std::array<juce::MidiBuffer, kOutputMidiSlots>& outputMidi,
                          PaletteType pal, VoicingMode voic, float spr)
    {
        for (const auto metadata : inputMidi)
        {
            const auto msg = metadata.getMessage();
            const int samplePos = metadata.samplePosition;

            if (msg.isNoteOn())
            {
                triggerChord (msg.getNoteNumber(), msg.getFloatVelocity(),
                              pal, voic, spr, samplePos, outputMidi);
            }
            else if (msg.isNoteOff())
            {
                if (msg.getNoteNumber() == activeInputNote)
                    releaseCurrentChord (samplePos, outputMidi);
            }
            else if (msg.isAllNotesOff() || msg.isAllSoundOff())
            {
                for (auto& buf : outputMidi)
                    buf.addEvent (msg, samplePos);
                releaseAllNotes (samplePos, outputMidi);
            }
            else
            {
                for (auto& buf : outputMidi)
                    buf.addEvent (msg, samplePos);
            }
        }
    }

    //==========================================================================
    // SEQUENCER MODE — 16-step clock triggers chords, live MIDI sets root
    //==========================================================================

    void processSequencerMode (const juce::MidiBuffer& inputMidi,
                               std::array<juce::MidiBuffer, kOutputMidiSlots>& outputMidi,
                               int numSamples,
                               PaletteType pal, VoicingMode voic, float spr)
    {
        // Handle reset (from stopping the sequencer)
        if (needsSeqReset.exchange (false, std::memory_order_relaxed))
            resetSequencer();

        // Apply Eno voicing offset (audio-thread-only, survives APVTS sync)
        if (enoVoicingOffset > 0)
        {
            int v = (static_cast<int> (voic) + enoVoicingOffset)
                    % static_cast<int> (VoicingMode::NumModes);
            voic = static_cast<VoicingMode> (v);
        }

        // Process live MIDI: noteOn sets root, CCs/pitchbend pass through
        for (const auto metadata : inputMidi)
        {
            const auto msg = metadata.getMessage();
            const int samplePos = metadata.samplePosition;

            if (msg.isNoteOn())
            {
                liveRoot = msg.getNoteNumber();
                liveVelocity = msg.getFloatVelocity();
            }
            else if (msg.isNoteOff())
            {
                // Ignore — sequencer controls release timing
            }
            else if (msg.isAllNotesOff() || msg.isAllSoundOff())
            {
                for (auto& buf : outputMidi)
                    buf.addEvent (msg, samplePos);
                releaseAllNotes (samplePos, outputMidi);
                resetSequencer();
                return;
            }
            else
            {
                // CCs, pitchbend → all slots
                for (auto& buf : outputMidi)
                    buf.addEvent (msg, samplePos);
            }
        }

        // Snapshot sequencer params (block-constant)
        const float curBPM = bpm.load (std::memory_order_relaxed);
        const float curSwing = swing.load (std::memory_order_relaxed);
        const float curGate = globalGate.load (std::memory_order_relaxed);

        const double baseSPS = sr * 60.0 / static_cast<double> (curBPM) / 4.0;

        // Event loop: jump to next event boundary (at most ~3 events per block)
        int pos = 0;
        while (pos < numSamples)
        {
            // Find the nearest upcoming event
            int samplesToGateOff = hasActiveChord ? seqSamplesUntilGateOff : INT_MAX;
            int samplesToStep = seqSamplesUntilStep;
            int remaining = numSamples - pos;

            // No events this block — just count down
            if (samplesToGateOff >= remaining && samplesToStep >= remaining)
            {
                if (hasActiveChord) seqSamplesUntilGateOff -= remaining;
                seqSamplesUntilStep -= remaining;
                break;
            }

            // Gate off comes first (or simultaneous)
            if (samplesToGateOff <= samplesToStep && samplesToGateOff < remaining)
            {
                pos += samplesToGateOff;
                seqSamplesUntilStep -= samplesToGateOff;
                seqSamplesUntilGateOff = 0;
                releaseCurrentChord (std::min (pos, numSamples - 1), outputMidi);
                continue;
            }

            // Step advance
            if (samplesToStep < remaining)
            {
                pos += samplesToStep;
                if (hasActiveChord)
                    seqSamplesUntilGateOff -= samplesToStep;

                // Advance step
                seqCurrentStep = (seqCurrentStep + 1) % NumSteps;

                // Eno mutations fire at cycle boundary (step 15 → 0)
                if (seqCurrentStep == 0)
                    applyEnoMutations();

                // Compute timing for this step (swing affects odd steps)
                double stepLen = baseSPS;
                if (seqCurrentStep & 1)
                    stepLen *= (1.0 + static_cast<double> (curSwing) * 0.5);
                else if (seqCurrentStep > 0)
                    stepLen *= (1.0 - static_cast<double> (curSwing) * 0.5);

                seqSamplesUntilStep = std::max (1, static_cast<int> (stepLen));

                // Release previous chord if gate is still open
                if (hasActiveChord)
                    releaseCurrentChord (std::min (pos, numSamples - 1), outputMidi);

                // Trigger new chord if step is active
                const auto& step = steps[seqCurrentStep];
                if (step.active)
                {
                    int root = (step.rootNote >= 0) ? step.rootNote : liveRoot;
                    float vel = step.velocity * liveVelocity;

                    triggerChord (root, vel, pal, voic, spr,
                                  std::min (pos, numSamples - 1), outputMidi);

                    // Compute gate-off time
                    float gateAmount = (step.gate > 0.0f) ? step.gate : curGate;
                    seqSamplesUntilGateOff = std::max (1,
                        static_cast<int> (gateAmount * stepLen));
                }

                continue;
            }

            break; // safety
        }
    }

    //==========================================================================
    // Shared chord trigger / release
    //==========================================================================

    void triggerChord (int noteNumber, float velocity,
                       PaletteType pal, VoicingMode voic, float spr,
                       int samplePos,
                       std::array<juce::MidiBuffer, kOutputMidiSlots>& outputMidi)
    {
        if (hasActiveChord)
            releaseCurrentChord (samplePos, outputMidi);

        auto newAssignment = distributor.distribute (noteNumber, pal, voic, spr, velocity);

        if (hadPreviousChord)
            newAssignment = distributor.voiceLead (previousAssignment, newAssignment);

        // Apply velocity curve — per-slot scaling
        const int curveIdx = std::max (0, std::min (
            static_cast<int> (VelocityCurve::NumCurves) - 1,
            velocityCurve.load (std::memory_order_relaxed)));
        for (int i = 0; i < 4; ++i)
            newAssignment.velocities[i] *= velcurves::kCurves[curveIdx][i];

        // Apply humanize — slight velocity variation per slot
        const float hum = humanize.load (std::memory_order_relaxed);
        if (hum > 0.001f)
        {
            for (int i = 0; i < 4; ++i)
            {
                // Simple deterministic pseudo-random based on note + slot + step
                uint32_t seed = static_cast<uint32_t> (noteNumber * 17 + i * 31 + seqCurrentStep * 53);
                seed ^= seed << 13; seed ^= seed >> 17; seed ^= seed << 5;
                float jitter = (static_cast<float> (seed & 0xFFFF) / 32768.0f - 1.0f) * hum * 0.15f;
                newAssignment.velocities[i] = std::max (0.05f,
                    std::min (1.0f, newAssignment.velocities[i] + jitter));
            }
        }

        // Apply sidechain duck — attenuate velocity on downbeats (steps 0, 4, 8, 12)
        const float duck = sidechainDuck.load (std::memory_order_relaxed);
        if (duck > 0.001f && sequencerRunning.load (std::memory_order_relaxed))
        {
            // Steps 0, 4, 8, 12 are the "kick" positions in a 16-step bar
            if ((seqCurrentStep & 3) == 0)
            {
                float duckAmount = 1.0f - duck * 0.7f; // max 70% reduction
                for (int i = 0; i < 4; ++i)
                    newAssignment.velocities[i] *= duckAmount;
            }
        }

        for (int i = 0; i < 4; ++i)
        {
            if (newAssignment.midiNotes[i] >= 0 && newAssignment.midiNotes[i] <= 127)
            {
                outputMidi[i].addEvent (
                    juce::MidiMessage::noteOn (1, newAssignment.midiNotes[i],
                                               newAssignment.velocities[i]),
                    samplePos);
            }
        }

        previousAssignment = currentAssignment;
        currentAssignment = newAssignment;
        activeInputNote = noteNumber;
        hasActiveChord = true;
        hadPreviousChord = true;
    }

    void releaseCurrentChord (int samplePos,
                              std::array<juce::MidiBuffer, kOutputMidiSlots>& outputMidi)
    {
        if (! hasActiveChord)
            return;

        for (int i = 0; i < 4; ++i)
        {
            if (currentAssignment.midiNotes[i] >= 0 && currentAssignment.midiNotes[i] <= 127)
            {
                outputMidi[i].addEvent (
                    juce::MidiMessage::noteOff (1, currentAssignment.midiNotes[i]),
                    samplePos);
            }
        }

        previousAssignment = currentAssignment;
        hasActiveChord = false;
    }

    void releaseAllNotes (int samplePos,
                          std::array<juce::MidiBuffer, kOutputMidiSlots>& outputMidi)
    {
        releaseCurrentChord (samplePos, outputMidi);
        currentAssignment = {};
        previousAssignment = {};
        hasActiveChord = false;
        hadPreviousChord = false;
    }

    void resetSequencer()
    {
        if (hasActiveChord)
        {
            // Can't emit noteOff without outputMidi — engines will clean up
            // via allNotesOff. Reset state only.
            hasActiveChord = false;
        }
        seqCurrentStep = NumSteps - 1; // next advance → step 0
        seqSamplesUntilStep = 1;       // trigger immediately on next block
        seqSamplesUntilGateOff = 0;
        currentAssignment = {};
        previousAssignment = {};
        hadPreviousChord = false;
        enoCycleCount = 0;
        enoVoicingOffset = 0;
    }

    double computeBaseSamplesPerStep() const
    {
        return sr * 60.0 / static_cast<double> (bpm.load (std::memory_order_relaxed)) / 4.0;
    }

    //-- Eno mode: controlled generative mutations on cycle boundary -----------
    //
    // Called when sequencer wraps from step 15 → 0.
    // Each mutation has a probability gate — the result is a chord sequence
    // that slowly evolves, never exactly repeating, but stays coherent.

    uint32_t enoRand()
    {
        // xorshift32 — fast, deterministic, no allocation
        enoRngState ^= enoRngState << 13;
        enoRngState ^= enoRngState >> 17;
        enoRngState ^= enoRngState << 5;
        return enoRngState;
    }

    // Returns 0.0–1.0 from the RNG
    float enoRandFloat() { return static_cast<float> (enoRand() & 0xFFFF) / 65535.0f; }

    void applyEnoMutations()
    {
        if (! enoMode.load (std::memory_order_relaxed))
            return;

        ++enoCycleCount;

        // 1. Root drift: ~5% chance per step, shift by ±1 or ±2 semitones
        for (int i = 0; i < NumSteps; ++i)
        {
            if (enoRandFloat() < 0.05f)
            {
                int shift = (enoRand() & 1) ? 1 : -1;
                if (enoRandFloat() < 0.3f)
                    shift *= 2;  // occasional ±2

                if (steps[i].rootNote >= 0)
                    steps[i].rootNote = std::max (24, std::min (96, steps[i].rootNote + shift));
            }
        }

        // 2. Rest injection: ~8% chance per active step → temporarily mute
        for (int i = 0; i < NumSteps; ++i)
        {
            if (steps[i].active && enoRandFloat() < 0.08f)
                steps[i].active = false;
        }

        // 3. Rest recovery: ~12% chance per inactive step → reactivate
        //    (prevents the sequence from going completely silent over time)
        for (int i = 0; i < NumSteps; ++i)
        {
            if (! steps[i].active && enoRandFloat() < 0.12f)
                steps[i].active = true;
        }

        // 4. Voicing rotation: every 4 cycles, advance voicing offset by 1.
        //    Uses a separate offset rather than writing to the voicing atomic,
        //    because the APVTS sync would overwrite it every block.
        if ((enoCycleCount & 3) == 0)
            enoVoicingOffset = (enoVoicingOffset + 1) % static_cast<int> (VoicingMode::NumModes);

        // 5. Velocity drift: slight per-step velocity wander (±5%)
        for (int i = 0; i < NumSteps; ++i)
        {
            if (enoRandFloat() < 0.15f)
            {
                float drift = (enoRandFloat() - 0.5f) * 0.1f;
                steps[i].velocity = std::max (0.3f, std::min (1.0f, steps[i].velocity + drift));
            }
        }
    }

    //-- Members ---------------------------------------------------------------

    ChordDistributor distributor;
    ChordAssignment currentAssignment;
    ChordAssignment previousAssignment;

    int activeInputNote = -1;
    bool hasActiveChord = false;
    bool hadPreviousChord = false;

    // Live MIDI state
    int liveRoot = 60;          // C4 default
    float liveVelocity = 0.8f;

    // Sequencer step data.
    // Written by: message thread (applyPattern, setStep*, restoreState)
    //             audio thread (applyEnoMutations)
    // Read by:    audio thread (processSequencerMode)
    //             message thread (getStep, serializeState, UI paint)
    // Benign race: StepData fields are independent POD values. A torn read
    // produces at worst one step with a briefly inconsistent velocity or gate,
    // which is inaudible. This is the standard JUCE pattern for step data.
    std::array<StepData, NumSteps> steps;
    int seqCurrentStep = 15;        // starts at 15 so first advance → 0
    int seqSamplesUntilStep = 1;    // trigger immediately
    int seqSamplesUntilGateOff = 0;

    // Atomics (set from message thread, read from audio thread)
    std::atomic<bool>  enabled            { false };
    std::atomic<int>   palette            { 0 };     // PaletteType
    std::atomic<int>   voicing            { 0 };     // VoicingMode
    std::atomic<float> spread             { 0.75f };

    std::atomic<bool>  sequencerRunning   { false };
    std::atomic<bool>  needsSeqReset      { false };
    std::atomic<float> bpm                { 122.0f };
    std::atomic<float> swing              { 0.0f };
    std::atomic<float> globalGate         { 0.75f };
    std::atomic<int>   activePattern      { 0 };     // RhythmPattern
    std::atomic<int>   velocityCurve      { 1 };     // VelocityCurve (default: RootHeavy)
    std::atomic<float> humanize           { 0.0f };
    std::atomic<float> sidechainDuck      { 0.0f };
    std::atomic<bool>  enoMode            { false };

    // Eno mode state (audio thread only)
    int enoCycleCount = 0;             // counts full 16-step cycles
    int enoVoicingOffset = 0;          // added to voicing index (survives APVTS sync)
    uint32_t enoRngState = 42;         // xorshift32 seed

    double sr = 44100.0;
};

} // namespace xolokun
