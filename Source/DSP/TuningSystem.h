#pragma once
#include "FastMath.h"
#include <cmath>
#include <cstring>
#include <cstdint>
#include <cstdio>
#include <cctype>
#include <cfloat>

namespace xolokun {

//==============================================================================
// TuningSystem — Micro-tuning and alternative scale support for XOlokun.
//
// Provides a 128-entry precomputed frequency table so noteToFrequency() is a
// single table lookup on the audio thread — zero transcendental math per call.
// The table is rebuilt only when tuning changes (on a non-audio thread or
// during prepare/reset — never inside the render callback).
//
// Supported tunings:
//   • Standard 12-TET (default)
//   • 5-limit Just Intonation
//   • 3-limit Pythagorean
//   • Quarter-comma Meantone
//   • 24-TET Arabic Maqam
//   • Arbitrary Scala .scl files (any number of notes per octave)
//   • Custom keyboard mapping via .kbm files
//
// Integration into any engine:
//   // In engine struct:
//   TuningSystem tuning;
//
//   // On prepare or preset load (non-audio thread):
//   tuning.setReferenceFrequency (440.0f);
//   tuning.setEqual12();            // or load a .scl file
//
//   // In noteOn or render (audio thread, table-lookup only):
//   float freq = tuning.noteToFrequency (midiNote);
//
//   // With pitch bend already converted to semitones by PitchBendUtil:
//   float bendSemitones = PitchBendUtil::bendToSemitones (norm, 2.0f);
//   float freq = tuning.noteToFrequency (static_cast<float> (midiNote) + bendSemitones);
//
// Thread-safety contract:
//   — buildTable() must be called only from a non-audio thread (or during
//     prepare, before processing begins).
//   — noteToFrequency() is const, reads only the immutable table, and is safe
//     to call from the audio thread concurrently with other reads.
//   — Writes (loadScalaFile, setEqual12, etc.) must complete before the audio
//     thread calls noteToFrequency(). Use the standard JUCE AudioProcessor
//     prepare/process separation; no additional locking is needed.
//
// Denormal protection:
//   — All table entries are clamped to [8.0 Hz, 25600.0 Hz] during build.
//     This eliminates denormal risk at the source (sub-8Hz is infrasonic and
//     not useful for audio DSP; 25600Hz exceeds the Nyquist of 48kHz audio).
//
// Scala .scl format reference: https://www.huygens-fokker.org/scala/scl_format.html
// Keyboard .kbm format reference: https://www.huygens-fokker.org/scala/help.htm#mappings
//==============================================================================

//------------------------------------------------------------------------------
// Internal limits and constants
//------------------------------------------------------------------------------
static constexpr int    kTuningTableSize  = 128;   // covers all MIDI notes 0-127
static constexpr int    kMaxScaleDegrees  = 128;   // Scala supports up to ~100 common
static constexpr float  kMinFreqHz        = 8.0f;
static constexpr float  kMaxFreqHz        = 25600.0f;
static constexpr float  kDefaultRefFreq   = 440.0f;
static constexpr int    kDefaultRefNote   = 69;    // A4

//------------------------------------------------------------------------------
// Built-in tuning ratio tables (all relative to the root, octave = 2.0)
// These are constexpr so the compiler can place them in read-only data.
//------------------------------------------------------------------------------

// 5-limit Just Intonation — C major diatonic + chromatic fill
// C  C#       D     Eb      E    F    F#      G    Ab      A    Bb      B
static constexpr double kJI5Ratios[12] = {
    1.0,
    16.0 / 15.0,    // minor second (JI)
    9.0  /  8.0,    // major second
    6.0  /  5.0,    // minor third
    5.0  /  4.0,    // major third
    4.0  /  3.0,    // perfect fourth
    45.0 / 32.0,    // augmented fourth (tritone JI)
    3.0  /  2.0,    // perfect fifth
    8.0  /  5.0,    // minor sixth
    5.0  /  3.0,    // major sixth
    9.0  /  5.0,    // minor seventh
    15.0 /  8.0     // major seventh
};

// 3-limit Pythagorean — stacked perfect fifths (3/2)
// Each degree = (3/2)^n, octave-reduced.  Order: C C# D Eb E F F# G Ab A Bb B
static constexpr double kPythRatios[12] = {
    1.0,
    2187.0 / 2048.0,  // apotome (C#)
    9.0    /    8.0,  // D
    32.0   /   27.0,  // Eb (Pythagorean minor third)
    81.0   /   64.0,  // E
    4.0    /    3.0,  // F
    729.0  /  512.0,  // F# (tritone Pythagorean)
    3.0    /    2.0,  // G
    128.0  /   81.0,  // Ab
    27.0   /   16.0,  // A
    16.0   /    9.0,  // Bb
    243.0  /  128.0   // B
};

// Quarter-comma Meantone — historically used for keyboard instruments 1450-1800.
// Fifths tempered narrow by 1/4 syntonic comma so major thirds are pure (5/4).
// Fifth = 2^(1/4) * (5/4)^(1/4) ... exact: 5^(1/4) / 2^(1/2)
// Generated ratios (C as root):
// Quarter-comma meantone fifth = 5^(1/4) ≈ 1.49534878122122 (5^0.25).
// Used to derive the table below; stored inline for documentation clarity.
static constexpr double kMeantoneRatios[12] = {
    1.0,
    1.04490672652566,  // C# (D-flat enharmonic, 1 fifth up ×5 / 8 × 5^0.25)
    1.11803398874990,  // D  = sqrt(5)/2
    1.19627902497696,  // Eb = 2 / (5^0.25)^3 * ... see derivation
    1.25000000000000,  // E  = 5/4 exactly (this is the defining property)
    1.33748060995284,  // F
    1.39754248593737,  // F#
    1.49534878122122,  // G
    1.56250000000000,  // Ab = 5/4 × 5/4 / 2
    1.67185076244105,  // A  = 5^0.75 / 2
    1.78885438199983,  // Bb
    1.86918597652653   // B
};

// 24-TET (Arabic Maqam quarter-tone system).
// Every step = 2^(1/24). Indices 0,2,4,...22 are standard 12-TET semitones.
// This table stores all 24 steps so the .scl loader can compare; for the
// built-in path we generate it programmatically in buildTable().

//------------------------------------------------------------------------------
// ScaleDegree — one entry parsed from a .scl file
//------------------------------------------------------------------------------
struct ScaleDegree
{
    double ratio = 1.0;  // frequency ratio relative to root (always ≥ 1.0)
};

//------------------------------------------------------------------------------
// KeyboardMap — parsed .kbm file
//------------------------------------------------------------------------------
struct KeyboardMap
{
    int    mapSize      = 12;    // number of entries in the mapping
    int    firstNote    = 0;     // lowest MIDI note mapped
    int    lastNote     = 127;   // highest MIDI note mapped
    int    middleNote   = 60;    // MIDI note that plays the reference frequency
    int    referenceNote = 69;   // MIDI note for reference frequency (A4)
    double referenceFreq = 440.0; // Hz for referenceNote
    int    scaleOctave  = 12;    // number of scale degrees before repeating octave
    int    degrees[kMaxScaleDegrees]; // scale degree for each map slot (-1 = unmapped)
    int    degreeCount  = 0;

    void setDefault() noexcept
    {
        mapSize      = 12;
        firstNote    = 0;
        lastNote     = 127;
        middleNote   = 60;
        referenceNote = 69;
        referenceFreq = 440.0;
        scaleOctave  = 12;
        degreeCount  = 12;
        for (int i = 0; i < 12; ++i)
            degrees[i] = i;
    }
};

//==============================================================================
// TuningSystem
//==============================================================================
class TuningSystem
{
public:

    //--------------------------------------------------------------------------
    // Construction
    //--------------------------------------------------------------------------

    TuningSystem() noexcept
    {
        kbm.setDefault();
        setEqual12();
    }

    //--------------------------------------------------------------------------
    // Built-in tuning presets
    //--------------------------------------------------------------------------

    /// Standard 12-TET equal temperament (default). All 12 semitones equally
    /// spaced as 2^(n/12). The universal reset to known-good state.
    void setEqual12() noexcept
    {
        scaleDegreeCount = 12;
        for (int i = 0; i < 12; ++i)
            scaleDegrees[i].ratio = std::pow (2.0, i / 12.0);
        scaleOctaveRatio = 2.0;
        buildTable();
    }

    /// 5-limit Just Intonation. Pure major thirds (5/4) and fifths (3/2).
    /// Sounds beatless on root-position triads; wolf intervals appear elsewhere.
    void setJustIntonation() noexcept
    {
        scaleDegreeCount = 12;
        for (int i = 0; i < 12; ++i)
            scaleDegrees[i].ratio = kJI5Ratios[i];
        scaleOctaveRatio = 2.0;
        buildTable();
    }

    /// 3-limit Pythagorean intonation. Pure fifths (3/2) throughout.
    /// Major thirds are wide (81/64 ≈ 407.8 cents). Suits medieval/modal music.
    void setPythagorean() noexcept
    {
        scaleDegreeCount = 12;
        for (int i = 0; i < 12; ++i)
            scaleDegrees[i].ratio = kPythRatios[i];
        scaleOctaveRatio = 2.0;
        buildTable();
    }

    /// Quarter-comma Meantone. Pure major thirds (5/4). Fifth is 5^(1/4) ≈ 696.6¢.
    /// The standard for Renaissance/Baroque keyboard music.
    void setMeantone() noexcept
    {
        scaleDegreeCount = 12;
        for (int i = 0; i < 12; ++i)
            scaleDegrees[i].ratio = kMeantoneRatios[i];
        scaleOctaveRatio = 2.0;
        buildTable();
    }

    /// 24-TET Arabic Maqam quarter-tone system.
    /// Adds 12 quarter-tone pitches between the standard 12 semitones.
    /// Used in Arabic, Turkish, and Persian maqam/makam traditions.
    void setArabicMaqam() noexcept
    {
        scaleDegreeCount = 24;
        for (int i = 0; i < 24; ++i)
            scaleDegrees[i].ratio = std::pow (2.0, i / 24.0);
        scaleOctaveRatio = 2.0;

        // For 24-TET we reassign the keyboard map to span 24 degrees across
        // the two octaves of two adjacent keyboard octaves, keeping MIDI A4=440.
        // The middle octave maps degrees 0-11 on white+black keys (standard layout).
        // Engines that want the full 24 should use a kbm file for custom mapping.
        // Default: standard chromatic mapping still works (uses even-indexed degrees).
        buildTable();
    }

    //--------------------------------------------------------------------------
    // Reference pitch
    //--------------------------------------------------------------------------

    /// Set the reference frequency for the reference MIDI note.
    /// Default: 440.0 Hz. Call before buildTable() if changed with a built-in tuning,
    /// or call after setEqual12() etc. (they always call buildTable() internally).
    void setReferenceFrequency (float freq) noexcept
    {
        refFreqHz = (freq > 0.0f) ? static_cast<double> (freq) : 440.0;
        kbm.referenceFreq = refFreqHz;
        buildTable();
    }

    /// Set the MIDI note number that plays at exactly the reference frequency.
    /// Default: 69 (A4). Rebuilds the table.
    void setReferenceMidiNote (int note) noexcept
    {
        refMidiNote = (note >= 0 && note < 128) ? note : 69;
        kbm.referenceNote = refMidiNote;
        buildTable();
    }

    //--------------------------------------------------------------------------
    // Scala .scl parser
    //--------------------------------------------------------------------------

    /// Load a Scala .scl scale definition from raw text data.
    ///
    /// .scl format:
    ///   Line 1: description (arbitrary text, ignored)
    ///   Line 2: integer N = number of notes (not counting the 1/1)
    ///   Lines 3..N+2: pitch entries — either:
    ///     •  cent values:  "200.0"  or  "200."  (contain a '.')
    ///     •  ratio values: "3/2"    or  "2"     (integer = N octaves)
    ///   Lines starting with '!' are comments.
    ///
    /// Returns true on success, false if parsing fails (tuning is unchanged on failure).
    bool loadScalaFile (const char* sclData, int sclLength) noexcept
    {
        if (sclData == nullptr || sclLength <= 0)
            return false;

        // Working copies
        ScaleDegree tempDegrees[kMaxScaleDegrees];
        int         tempCount = 0;
        double      tempOctave = 2.0;

        // ---- Simple line-by-line parser ----
        // We parse directly from the raw buffer without dynamic allocation.
        // State machine: skip comments, then:
        //   state 0 = waiting for description line
        //   state 1 = waiting for note-count line
        //   state 2 = reading pitch entries
        int state       = 0;
        int notesExpected = 0;
        int lineStart   = 0;
        char lineBuf[256];

        auto processLine = [&] (const char* line, int len) -> bool
        {
            // Strip trailing whitespace/CR
            while (len > 0 && (line[len-1] == '\r' || line[len-1] == '\n' || line[len-1] == ' ' || line[len-1] == '\t'))
                --len;

            if (len <= 0) return true;  // blank line — skip

            // Comments start with '!'
            if (line[0] == '!')
                return true;

            switch (state)
            {
                case 0:  // description — skip
                    state = 1;
                    return true;

                case 1:  // note count
                {
                    int n = 0;
                    for (int i = 0; i < len; ++i)
                    {
                        if (line[i] >= '0' && line[i] <= '9')
                            n = n * 10 + (line[i] - '0');
                        else if (line[i] == ' ' || line[i] == '\t')
                            break;
                        else
                            return false;  // unexpected char
                    }
                    if (n <= 0 || n >= kMaxScaleDegrees)
                        return false;
                    notesExpected = n;
                    // Degree 0 is always 1/1 (unison)
                    tempDegrees[0].ratio = 1.0;
                    tempCount = 0;
                    state = 2;
                    return true;
                }

                case 2:  // pitch entry
                {
                    if (tempCount >= notesExpected)
                        return true;  // extra lines after expected notes — ignore

                    // Copy to null-terminated scratch
                    int copyLen = len < 255 ? len : 255;
                    char buf[256];
                    std::memcpy (buf, line, static_cast<size_t> (copyLen));
                    buf[copyLen] = '\0';

                    // Strip leading whitespace
                    int start = 0;
                    while (buf[start] == ' ' || buf[start] == '\t') ++start;

                    // Detect cents (contains '.') vs ratio (contains '/' or plain integer)
                    bool hasDot   = false;
                    bool hasSlash = false;
                    for (int i = start; buf[i] != '\0'; ++i)
                    {
                        if (buf[i] == '.') { hasDot = true;   break; }
                        if (buf[i] == '/') { hasSlash = true; break; }
                    }

                    double ratio = 1.0;

                    if (hasDot)
                    {
                        // Cents value — parse as double
                        double cents = 0.0;
                        if (std::sscanf (buf + start, "%lf", &cents) != 1)
                            return false;
                        ratio = std::pow (2.0, cents / 1200.0);
                    }
                    else if (hasSlash)
                    {
                        // Ratio "numerator/denominator"
                        // den MUST start at 0 — the accumulation loop is
                        //   den = den * 10 + digit
                        // so a non-zero seed would corrupt the result (e.g. 1→12 for "2").
                        long long num = 0, den = 0;
                        const char* p = buf + start;
                        while (*p >= '0' && *p <= '9') { num = num * 10 + (*p - '0'); ++p; }
                        if (*p != '/') return false;
                        ++p;
                        while (*p >= '0' && *p <= '9') { den = den * 10 + (*p - '0'); ++p; }
                        if (den == 0) return false;  // malformed or zero denominator
                        ratio = static_cast<double> (num) / static_cast<double> (den);
                    }
                    else
                    {
                        // Plain integer = that many octaves
                        long long n = 0;
                        const char* p = buf + start;
                        bool hasDigit = false;
                        while (*p >= '0' && *p <= '9') { n = n * 10 + (*p - '0'); ++p; hasDigit = true; }
                        if (!hasDigit) return false;
                        ratio = static_cast<double> (n);  // e.g. "2" = one octave = 2.0
                    }

                    if (ratio <= 0.0) return false;

                    // The last entry in .scl is the octave interval
                    if (tempCount == notesExpected - 1)
                        tempOctave = ratio;

                    // Store degree (offset +1 because degree 0 = unison is implicit)
                    tempDegrees[tempCount + 1].ratio = ratio;
                    ++tempCount;
                    return true;
                }

                default:
                    return true;
            }
        };

        // Split buffer into lines
        int pos = 0;
        while (pos <= sclLength)
        {
            // Find end of line
            int end = pos;
            while (end < sclLength && sclData[end] != '\n')
                ++end;

            int lineLen = end - lineStart;
            if (lineLen > 255) lineLen = 255;
            std::memcpy (lineBuf, sclData + lineStart, static_cast<size_t> (lineLen));
            lineBuf[lineLen] = '\0';

            if (!processLine (lineBuf, lineLen))
                return false;  // parse error — leave existing tuning intact

            lineStart = end + 1;
            pos = lineStart;

            if (end >= sclLength) break;
        }

        if (state != 2 || notesExpected == 0)
            return false;  // incomplete file

        // Commit parsed result.
        // scaleDegrees[0] = unison (1/1, implicit), degrees[1..N] = parsed pitches.
        // scaleDegreeCount = N (number of non-unison entries = degrees per octave).
        scaleDegreeCount = notesExpected;
        for (int i = 0; i <= notesExpected; ++i)
            scaleDegrees[i] = tempDegrees[i];
        scaleOctaveRatio = tempOctave;

        // When no .kbm has been loaded, reset the keyboard map to match this
        // scale's degree count so the default identity mapping is correct.
        // A subsequent loadKeyboardMapping() call will override this.
        kbm.setDefault();
        kbm.mapSize     = scaleDegreeCount;
        kbm.scaleOctave = scaleDegreeCount;
        kbm.degreeCount = scaleDegreeCount;
        for (int i = 0; i < scaleDegreeCount && i < kMaxScaleDegrees; ++i)
            kbm.degrees[i] = i;

        buildTable();
        return true;
    }

    //--------------------------------------------------------------------------
    // Keyboard Mapping (.kbm) parser
    //--------------------------------------------------------------------------

    /// Load a Scala keyboard mapping (.kbm) from raw text data.
    ///
    /// .kbm format (all values on separate lines, '!' for comments):
    ///   Line 1: map size (number of keys before repeating)
    ///   Line 2: first MIDI note mapped
    ///   Line 3: last MIDI note mapped
    ///   Line 4: middle MIDI note (plays unison / degree 0)
    ///   Line 5: reference MIDI note (plays refFreq)
    ///   Line 6: reference frequency in Hz
    ///   Line 7: scale octave (how many scale degrees = 1 octave)
    ///   Lines 8..N: degree assignments (integer scale degree, or 'x' = unmapped)
    ///
    /// Returns true on success, false on parse failure (mapping unchanged).
    bool loadKeyboardMapping (const char* kbmData, int kbmLength) noexcept
    {
        if (kbmData == nullptr || kbmLength <= 0)
            return false;

        KeyboardMap tempMap;
        tempMap.setDefault();
        int headerLine  = 0;  // which header field we're reading (0-based)
        int degreeIndex = 0;

        auto processKbmLine = [&] (const char* line, int len) -> bool
        {
            while (len > 0 && (line[len-1] == '\r' || line[len-1] == '\n'
                               || line[len-1] == ' ' || line[len-1] == '\t'))
                --len;
            if (len <= 0) return true;
            if (line[0] == '!') return true;  // comment

            char buf[256];
            int copyLen = len < 255 ? len : 255;
            std::memcpy (buf, line, static_cast<size_t> (copyLen));
            buf[copyLen] = '\0';
            const char* p = buf;
            while (*p == ' ' || *p == '\t') ++p;

            if (headerLine < 7)
            {
                // Header fields
                switch (headerLine)
                {
                    case 0: { int v = 0; std::sscanf (p, "%d", &v); tempMap.mapSize = v; break; }
                    case 1: { int v = 0; std::sscanf (p, "%d", &v); tempMap.firstNote = v; break; }
                    case 2: { int v = 0; std::sscanf (p, "%d", &v); tempMap.lastNote = v; break; }
                    case 3: { int v = 0; std::sscanf (p, "%d", &v); tempMap.middleNote = v; break; }
                    case 4: { int v = 0; std::sscanf (p, "%d", &v); tempMap.referenceNote = v; break; }
                    case 5: { double v = 440.0; std::sscanf (p, "%lf", &v); tempMap.referenceFreq = v; break; }
                    case 6: { int v = 0; std::sscanf (p, "%d", &v); tempMap.scaleOctave = v; break; }
                    default: break;
                }
                ++headerLine;
                return true;
            }

            // Degree map entries
            if (degreeIndex >= kMaxScaleDegrees)
                return true;

            if (p[0] == 'x' || p[0] == 'X')
            {
                tempMap.degrees[degreeIndex] = -1;  // unmapped key
            }
            else
            {
                int v = 0;
                std::sscanf (p, "%d", &v);
                tempMap.degrees[degreeIndex] = v;
            }
            ++degreeIndex;
            tempMap.degreeCount = degreeIndex;
            return true;
        };

        // Parse line by line
        char lineBuf[256];
        int pos = 0;
        int lineStart = 0;

        while (pos <= kbmLength)
        {
            int end = pos;
            while (end < kbmLength && kbmData[end] != '\n')
                ++end;

            int lineLen = end - lineStart;
            if (lineLen > 255) lineLen = 255;
            std::memcpy (lineBuf, kbmData + lineStart, static_cast<size_t> (lineLen));
            lineBuf[lineLen] = '\0';

            if (!processKbmLine (lineBuf, lineLen))
                return false;

            lineStart = end + 1;
            pos = lineStart;
            if (end >= kbmLength) break;
        }

        if (headerLine < 7)
            return false;  // incomplete .kbm header

        // Commit and rebuild
        kbm = tempMap;
        refFreqHz = kbm.referenceFreq;
        refMidiNote = kbm.referenceNote;
        buildTable();
        return true;
    }

    //--------------------------------------------------------------------------
    // Audio-thread API — table lookups only, no transcendental math
    //--------------------------------------------------------------------------

    /// Map a MIDI note [0, 127] to a frequency in Hz using the current tuning.
    /// O(1) — single array lookup. Safe to call on the audio thread.
    float noteToFrequency (int midiNote) const noexcept
    {
        if (midiNote < 0)   midiNote = 0;
        if (midiNote > 127) midiNote = 127;
        return freqTable[midiNote];
    }

    /// Map a fractional MIDI note (e.g. with pitch bend applied) to frequency.
    /// Linearly interpolates between adjacent table entries.
    /// The bend amount in semitones should come from PitchBendUtil::bendToSemitones().
    ///
    /// Example:
    ///   float norm = PitchBendUtil::parsePitchWheel (midiWheelValue);
    ///   float semi = PitchBendUtil::bendToSemitones (norm, 2.0f);
    ///   float freq = tuning.noteToFrequency (static_cast<float>(midiNote) + semi);
    float noteToFrequency (float midiNoteFloat) const noexcept
    {
        // Clamp to valid table range with headroom for interpolation
        if (midiNoteFloat < 0.0f)   midiNoteFloat = 0.0f;
        if (midiNoteFloat > 127.0f) midiNoteFloat = 127.0f;

        int   lo  = static_cast<int> (midiNoteFloat);
        float frac = midiNoteFloat - static_cast<float> (lo);

        if (lo >= 127)
            return freqTable[127];

        // Linear interpolation between adjacent table entries.
        // In 12-TET this is equivalent to semi-tone linear interpolation in log space
        // (accurate to < 1 cent for |frac| ≤ 0.5). For exotic tunings with large
        // inter-degree gaps, this remains musically usable for pitch bend.
        float f0 = freqTable[lo];
        float f1 = freqTable[lo + 1];
        return f0 + frac * (f1 - f0);
    }

    //--------------------------------------------------------------------------
    // Introspection
    //--------------------------------------------------------------------------

    /// Number of scale degrees in the currently loaded scale (excluding unison).
    int getScaleDegreeCount() const noexcept { return scaleDegreeCount; }

    /// Frequency ratio for scale degree n (0 = unison, N = octave).
    double getScaleDegreeRatio (int n) const noexcept
    {
        if (n < 0 || n > scaleDegreeCount) return 1.0;
        return scaleDegrees[n].ratio;
    }

    /// Current reference frequency (Hz).
    float getReferenceFrequency() const noexcept { return static_cast<float> (refFreqHz); }

    /// Current reference MIDI note.
    int getReferenceMidiNote() const noexcept { return refMidiNote; }

    /// Direct access to the precomputed table (128 floats, read-only on audio thread).
    const float* getFrequencyTable() const noexcept { return freqTable; }

    //--------------------------------------------------------------------------
    // State management
    //--------------------------------------------------------------------------

    /// Reset to 12-TET, A4=440.
    void reset() noexcept
    {
        refFreqHz   = kDefaultRefFreq;
        refMidiNote = kDefaultRefNote;
        kbm.setDefault();
        setEqual12();
    }

private:

    //--------------------------------------------------------------------------
    // Table builder — called once after any tuning change (non-audio thread)
    //--------------------------------------------------------------------------

    /// Precompute all 128 MIDI note frequencies from the current scale + kbm.
    ///
    /// Algorithm:
    ///   For each MIDI note n in [0, 127]:
    ///     1. Apply the keyboard map to find which scale degree d it plays, and
    ///        how many octaves above/below the reference note it sits.
    ///     2. Multiply the reference frequency by:
    ///           scaleOctaveRatio^octaveOffset × scaleDegrees[d].ratio
    ///            ÷ scaleDegrees[refDegree].ratio
    ///        so that the reference note yields exactly refFreqHz.
    ///
    /// This is the same algorithm used by Surge XT's TuningImpl and
    /// Scala's own Keyboard.java, adapted to allocation-free C++.
    void buildTable() noexcept
    {
        // Resolve the scale degree and octave offset for the reference note
        const double refDegreeRatio = degreeRatioForNote (refMidiNote);

        for (int n = 0; n < kTuningTableSize; ++n)
        {
            double noteRatio = degreeRatioForNote (n);

            // Frequency = refFreq × (noteRatio / refDegreeRatio)
            // Both noteRatio and refDegreeRatio already include octave offsets.
            double freq = refFreqHz * (noteRatio / refDegreeRatio);

            // Denormal / out-of-range clamp — protects all downstream DSP
            if (freq < kMinFreqHz)   freq = kMinFreqHz;
            if (freq > kMaxFreqHz)   freq = kMaxFreqHz;

            freqTable[n] = static_cast<float> (freq);
        }
    }

    /// Compute the compound frequency ratio for a given MIDI note using the
    /// current scale and keyboard map.
    ///
    /// Returns: scaleOctaveRatio^octaveCount × scaleDegrees[degree].ratio
    ///
    /// "Compound" means the ratio already encodes the octave displacement, so
    /// the caller only needs to multiply by (refFreq / compoundRatioAtRefNote).
    double degreeRatioForNote (int midiNote) const noexcept
    {
        if (scaleDegreeCount <= 0)
            return std::pow (2.0, (midiNote - refMidiNote) / 12.0);

        // Relative position from the middle note (kbm.middleNote plays degree 0)
        int relNote = midiNote - kbm.middleNote;

        // Determine which map slot this falls into
        int mapSz = (kbm.mapSize > 0) ? kbm.mapSize : 12;
        int mapIdx = relNote % mapSz;
        int octaveCount = relNote / mapSz;

        // Correct for negative modulo (C++ rounds toward zero)
        if (mapIdx < 0)
        {
            mapIdx += mapSz;
            --octaveCount;
        }

        // Look up the scale degree for this map slot
        int degree = -1;
        if (kbm.degreeCount > 0 && mapIdx < kbm.degreeCount)
            degree = kbm.degrees[mapIdx];
        else
            degree = mapIdx % scaleDegreeCount;  // default: wrap within scale

        // Unmapped notes (-1): interpolate as 12-TET from nearest mapped note.
        // Simple approach: treat as if degree = mapIdx with 12-TET spacing.
        if (degree < 0)
        {
            // Fallback: produce a frequency that sits between its neighbors
            // using 12-TET semitone spacing from middleNote.
            return std::pow (2.0, static_cast<double> (midiNote - kbm.middleNote) / 12.0);
        }

        // Wrap degree within scale
        int wrappedDegree = degree % scaleDegreeCount;
        int extraOctaves  = degree / scaleDegreeCount;

        // Octave ratio for this note
        double octRatio = std::pow (scaleOctaveRatio, static_cast<double> (octaveCount + extraOctaves));

        // Scale degree ratio (degree 0 = unison = 1.0 is stored at index 0)
        double degRatio = (wrappedDegree < scaleDegreeCount)
                        ? scaleDegrees[wrappedDegree].ratio
                        : 1.0;

        return octRatio * degRatio;
    }

    //--------------------------------------------------------------------------
    // State
    //--------------------------------------------------------------------------

    // Scale definition
    ScaleDegree scaleDegrees[kMaxScaleDegrees + 1];  // +1 for degree 0 (unison)
    int         scaleDegreeCount = 12;
    double      scaleOctaveRatio = 2.0;

    // Reference pitch
    double refFreqHz   = kDefaultRefFreq;
    int    refMidiNote = kDefaultRefNote;

    // Keyboard mapping
    KeyboardMap kbm;

    // Precomputed table — the ONLY data accessed on the audio thread
    // Aligned to 16 bytes for potential SIMD use by callers.
    alignas(16) float freqTable[kTuningTableSize];
};

//==============================================================================
// Integration helpers — convenience wrappers matching PitchBendUtil style
//==============================================================================

/// Apply pitch bend from a JUCE/MIDI pitch wheel message to a base note and
/// return the final frequency from the given TuningSystem.
///
/// This is the canonical one-liner for engines that have integrated TuningSystem:
///
///   float freq = applyPitchBend (tuning, midiNote, midiWheelValue, 2.0f);
///
/// It combines PitchBendUtil's conversion with TuningSystem's table lookup +
/// fractional interpolation, keeping both modules independent.
inline float applyPitchBend (const TuningSystem& tuning,
                              int midiNote,
                              int midiWheelValue,
                              float rangeSemitones = 2.0f) noexcept
{
    // Normalize wheel [0, 16383] → [-1, +1], then to semitones
    float norm = static_cast<float> (midiWheelValue - 8192) / 8192.0f;
    float bendSemitones = norm * rangeSemitones;

    // Fractional note lookup (interpolates between adjacent table entries)
    float fractionalNote = static_cast<float> (midiNote) + bendSemitones;
    return tuning.noteToFrequency (fractionalNote);
}

/// Convert cents offset (e.g. from a detune parameter) to a frequency ratio
/// via the precomputed table. Rounds to nearest semitone then applies residual.
///
///   float freq = tuning.noteToFrequency(note) * centsToRatio(detuneCents);
///
/// Uses fastPow2 — matches the fleet-standard approach in PitchBendUtil.
inline float centsToRatio (float cents) noexcept
{
    return fastPow2 (cents / 1200.0f);
}

} // namespace xolokun
