#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include <array>
#include <vector>
#include <cmath>
#include <atomic>
#include "KeysMode.h"
#include "HarmonicField.h"
#include "XOuijaPanel.h"

// Forward declaration — PlaySurface stores a pointer to XOlokunProcessor so it
// can forward XOuija CC output events.  We also include the processor header
// here so the inline wireOnCCOutput() lambda can call pushCCOutput() without
// requiring callers to pre-include it.  XOlokunProcessor.h has no UI includes
// so there is no circular dependency.
namespace xolokun { class XOlokunProcessor; }
#include "../../XOlokunProcessor.h"

#include "GestureTrailBuffer.h"

namespace xolokun {

//==============================================================================
// Helper: lighten a colour toward white by [amount] (0=no change, 1=white)
static inline juce::Colour lightenColour(juce::Colour c, float amount)
{
    return c.interpolatedWith(juce::Colours::white, amount);
}

//==============================================================================
// Forward declarations for color/font access from XOlokunEditor.h.
// When PlaySurface.h is included from within XOlokunEditor.h the full
// GalleryColors namespace is already defined; guard the stub so we don't
// emit a second identical inline definition.
#ifndef XOLOKUN_GALLERY_COLORS_DEFINED
namespace GalleryColors {
    inline juce::Colour get(uint32_t hex) { return juce::Colour(hex); }
    constexpr uint32_t xoGold = 0xFFE9C46A;
    inline uint32_t textDark() { return 0xFF1A1A1A; }
}
#endif

//==============================================================================
// PlaySurface Constants
namespace PS {
    // V2 layout dimensions
    static constexpr int kDesktopW     = 700;        // Narrower
    static constexpr int kDesktopH     = 484;        // 420 (main) + 64 (strip)
    static constexpr int kXOuijaW     = 165;        // XOuija panel width
    static constexpr int kMainZoneH   = 420;        // Main content height
    static constexpr int kStripH      = 64;         // Performance strip (matches kZone3H)
    static constexpr int kHeaderH     = 28;         // Mode tab bar height

    // V1 layout constants (kept for backward compatibility with any external references)
    static constexpr int kZone1W       = 480;
    static constexpr int kZone2W       = 200;
    static constexpr int kZone4W       = 100;
    static constexpr int kZone3H       = 64;        // same as kStripH

    // Pad grid constants (unchanged)
    static constexpr int kPadCols      = 4;
    static constexpr int kPadRows      = 4;
    static constexpr int kNumPads      = kPadCols * kPadRows;

    // Animation
    static constexpr float kVelDecay        = 0.92f;
    static constexpr float kWarmMemoryDur   = 1.5f;  // seconds
    static constexpr int   kStripTrailSize  = 45;

    // Colors
    static constexpr uint32_t kAmber       = 0xFFF5C97A;
    static constexpr uint32_t kTerracotta  = 0xFFE07A5F;
    static constexpr uint32_t kTeal        = 0xFF2A9D8F;
    static constexpr uint32_t kFireGreen   = 0xFF4ADE80;
    static constexpr uint32_t kPanicRed    = 0xFFEF4444;
    static constexpr uint32_t kSurfaceBg   = 0xFF1A1A1A;
    static constexpr uint32_t kSurfaceCard = 0xFF2D2D2D;
    static constexpr uint32_t kTextLight   = 0xFFEEEEEE;
    static constexpr uint32_t kTextDim     = 0xFF888888;

    // MIDI
    static constexpr int kBaseNote    = 48; // C3
    static constexpr int kMinOctave   = -3;
    static constexpr int kMaxOctave   = 3;
}

//==============================================================================
// Zone 1: Note Input — 4x4 velocity-sensitive pad grid
//
class NoteInputZone : public juce::Component
{
public:
    enum class Mode { Pad, Fretless, Drum, Keys };

    //----------------------------------------------------------------------
    // P0-1: MIDI pipeline wiring.
    // Set this pointer (owned externally, e.g. by XOlokunProcessor) before
    // any interaction. When set, note-on/off messages are enqueued into the
    // collector for thread-safe delivery to the audio thread instead of
    // firing the raw std::function callbacks.
    // The std::function callbacks remain available as a fallback for tests
    // or callers that have not yet wired the collector.
    juce::MidiMessageCollector* midiCollector = nullptr;

    // MIDI channel for outgoing messages (1-16, default 1)
    int midiChannel = 1;

    std::function<void(int note, float velocity)> onNoteOn;
    std::function<void(int note)> onNoteOff;
    // Aftertouch callback — fired when the pad is held and the mouse moves
    // within it. pressure is 0-1 derived from Y position within the pad cell
    // (top = max pressure, bottom = min pressure, matching MPC convention).
    std::function<void(int note, float pressure)> onAftertouch;

    // Engine accent colour — set by PlaySurface::setAccentColour().
    // Default: XO Gold.
    juce::Colour accentColour { 0xFFE9C46A };
    void setAccentColour(juce::Colour c) { accentColour = c; repaint(); }

    // XOuija-reactive coloring state (Spec Section 8.2)
    // Updated by the XOuija planchette via PlaySurface::setHarmonicField().
    void setHarmonicField(int rootKey, int tension)
    {
        harmonicRootKey_ = rootKey % 12;
        harmonicTension_ = std::clamp(tension, 0, 6);
        repaint();
    }

    //----------------------------------------------------------------------
    // Bank selection (A=0, B=1, C=2, D=3).
    // In Pad mode: note = bankBaseNote(bank) + pad position
    //   Bank A: 36-51 (C2-D#3) — MPC default Bank A
    //   Bank B: 52-67 (E3-G4)
    //   Bank C: 68-83 (G#4-B5)
    //   Bank D: 84-99 (C6-D#7)
    // In Drum mode: bank selects an alternate drum kit offset (16 per bank).
    // kBaseNote (48) is unused in bank-aware mode; kept for legacy scale mode.
    enum class Bank { A = 0, B = 1, C = 2, D = 3 };
    void setBank(Bank b) { currentBank = b; scaleNotesDirty_ = true; repaint(); }
    Bank getBank() const { return currentBank; }

    NoteInputZone()
    {
        // Scale options
        scales = {
            { "Chromatic",    { 0,1,2,3,4,5,6,7,8,9,10,11 } },
            { "Major",        { 0,2,4,5,7,9,11 } },
            { "Minor",        { 0,2,3,5,7,8,10 } },
            { "Dorian",       { 0,2,3,5,7,9,10 } },
            { "Mixolydian",   { 0,2,4,5,7,9,10 } },
            { "Pent Minor",   { 0,3,5,7,10 } },
            { "Pent Major",   { 0,2,4,7,9 } },
            { "Blues",         { 0,3,5,6,7,10 } },
            { "Harm Minor",   { 0,2,3,5,7,8,11 } },
        };
        currentScale = 0; // Chromatic
        rootKey = 0;      // C
        octaveOffset = 0;
    }

    enum class ScaleMode { Off, Filter, Highlight };
    ScaleMode scaleMode = ScaleMode::Off;
    // P2-2: setter so callers don't access scaleMode directly
    void setScaleMode(ScaleMode m) { scaleMode = m; scaleNotesDirty_ = true; repaint(); }

    void setMode(Mode m) { mode = m; repaint(); }
    Mode getMode() const { return mode; }
    void setOctave(int oct) { octaveOffset = juce::jlimit(PS::kMinOctave, PS::kMaxOctave, oct); scaleNotesDirty_ = true; repaint(); }
    int  getOctave() const { return octaveOffset; }
    void setScale(int idx) { currentScale = juce::jlimit(0, (int)scales.size() - 1, idx); scaleNotesDirty_ = true; repaint(); }
    void setRootKey(int key) { rootKey = juce::jlimit(0, 11, key); scaleNotesDirty_ = true; repaint(); }

    void paint(juce::Graphics& g) override
    {
        using namespace PS;
        auto b = getLocalBounds().toFloat();

        // Radial gradient background: accent @ 0.04 center → kSurfaceBg edge
        {
            float cx = b.getCentreX(), cy = b.getCentreY();
            float r  = juce::jmax(b.getWidth(), b.getHeight()) * 0.7f;
            juce::ColourGradient bg(accentColour.withAlpha(0.04f), cx, cy,
                                    juce::Colour(kSurfaceBg), cx + r, cy, true);
            g.setGradientFill(bg);
            g.fillRect(b);
        }

        if (mode == Mode::Pad || mode == Mode::Drum)
            paintPadGrid(g);
        else
            paintFretless(g);
    }

    void mouseDown(const juce::MouseEvent& e) override { handleTouch(e, true); }
    void mouseDrag(const juce::MouseEvent& e) override { handleTouch(e, false); }
    void mouseUp(const juce::MouseEvent&) override
    {
        if (lastNote >= 0)
        {
            // P0-3: on fretless release, reset pitch bend to centre (8192 = no bend).
            // Value 0 would be maximum downward bend — do NOT use 0 here.
            if (mode == Mode::Fretless && midiCollector)
            {
                auto msg = juce::MidiMessage::pitchWheel(midiChannel, 8192);
                msg.setTimeStamp(juce::Time::getMillisecondCounterHiRes() * 0.001);
                midiCollector->addMessageToQueue(msg);
            }
            // Zero-out aftertouch before note-off to prevent stuck pressure
            if (mode == Mode::Pad || mode == Mode::Drum)
                fireAftertouch(lastNote, 0.0f);
            fireNoteOff(lastNote);
            lastNote = -1;
            lastPad = -1;
        }
    }

    // Called at 30fps for animation decay
    void tick()
    {
        bool needsRepaint = false;
        for (auto& v : padVelocity)
        {
            if (v > 0.01f) { v *= PS::kVelDecay; needsRepaint = true; }
            else v = 0.0f;
        }
        // Warm memory aging
        for (auto& wm : warmMemory)
        {
            if (wm.age < PS::kWarmMemoryDur)
            {
                wm.age += 1.0f / 30.0f;
                needsRepaint = true;
            }
        }
        if (needsRepaint) repaint();
    }

private:
    // ------------------------------------------------------------------
    // P0-1 helpers: route note events to MidiMessageCollector when wired,
    // otherwise fall back to the legacy std::function callbacks.
    void fireNoteOn(int note, float velocity)
    {
        if (midiCollector)
        {
            auto msg = juce::MidiMessage::noteOn(midiChannel, note,
                                                  static_cast<float>(velocity));
            msg.setTimeStamp(juce::Time::getMillisecondCounterHiRes() * 0.001);
            midiCollector->addMessageToQueue(msg);
        }
        else if (onNoteOn)
        {
            onNoteOn(note, velocity);
        }
    }

    void fireNoteOff(int note)
    {
        if (midiCollector)
        {
            auto msg = juce::MidiMessage::noteOff(midiChannel, note);
            msg.setTimeStamp(juce::Time::getMillisecondCounterHiRes() * 0.001);
            midiCollector->addMessageToQueue(msg);
        }
        else if (onNoteOff)
        {
            onNoteOff(note);
        }
    }

    // Polyphonic key pressure (per-note aftertouch).
    // pressure is 0-1; converted to MIDI 0-127 before dispatch.
    // MPC convention: continued downward pressure after the initial hit.
    void fireAftertouch(int note, float pressure)
    {
        uint8_t midiPressure = (uint8_t)juce::jlimit(0, 127, (int)(pressure * 127.0f));
        if (midiCollector)
        {
            auto msg = juce::MidiMessage::aftertouchChange(midiChannel, note, midiPressure);
            msg.setTimeStamp(juce::Time::getMillisecondCounterHiRes() * 0.001);
            midiCollector->addMessageToQueue(msg);
        }
        else if (onAftertouch)
        {
            onAftertouch(note, pressure);
        }
    }

    struct ScaleDef { const char* name; std::vector<int> intervals; };
    struct WarmMemoryEntry { int pad = -1; float age = 99.0f; };

    Mode mode = Mode::Pad;
    Bank currentBank = Bank::A;
    int  octaveOffset = 0;
    int  currentScale = 0;
    int  rootKey = 0;
    int  lastNote = -1;
    int  lastPad  = -1;   // pad index of the currently sounding note (for aftertouch geometry)
    float lastFretlessVelocity_ = 0.75f;  // updated on each fretless touch; drives ring glow
    std::vector<ScaleDef> scales;
    std::array<float, PS::kNumPads> padVelocity {};
    std::array<WarmMemoryEntry, 8> warmMemory {};
    int warmMemIdx = 0;

    // Fix #35: scale note cache — 16 pre-computed notes for FILTER mode.
    // Marked mutable so const helpers can rebuild on demand.
    mutable std::array<int, PS::kNumPads> cachedScaleNotes_ {};
    mutable bool scaleNotesDirty_ = true;

    // XOuija harmonic field state (Spec Section 8.2)
    int harmonicRootKey_ = 0;  // current key from XOuija (semitone 0-11)
    int harmonicTension_ = 0;  // fifths distance from C for color temperature (0-6)

    int midiNoteForPad(int pad) const
    {
        if (mode == Mode::Drum)
        {
            // MPC standard Bank A drum note mapping (pad 0 = bottom-left = MPC Pad 1).
            // Each bank shifts the entire table by 16 notes, following MPC convention:
            //   Bank A = base table  (notes 36-51 area)
            //   Bank B = base + 16
            //   Bank C = base + 32
            //   Bank D = base + 48
            // Source: Akai MPC Bank A default, spec section 3.8.
            static constexpr int kDrumNotes[PS::kNumPads] = {
                37, 36, 42, 82,  // row 0 (bottom): Kick A, Kick B, Snare A, Snare B
                40, 38, 46, 44,  // row 1: Clap, Hat Closed, Hat Open, Ride
                48, 47, 45, 43,  // row 2: Tom 1, Perc A, Perc B, Crash
                49, 55, 57, 51,  // row 3 (top): Fx 1, Fx 2, Fx 3, Fx 4
            };
            int bankOffset = (int)currentBank * 16;
            return juce::jlimit(0, 127, kDrumNotes[pad] + bankOffset);
        }

        // FILTER mode: pads show consecutive in-scale degrees only
        if (scaleMode == ScaleMode::Filter)
            return scaleNoteForPad(pad);

        // Pad mode: MPC-standard Bank A starts at note 36 (C2).
        // Each bank adds 16: A=36, B=52, C=68, D=84.
        // Layout: left-to-right within a row, bottom-to-top rows (MPC standard).
        // Octave offset still applies on top of the bank base.
        static constexpr int kBankBase[4] = { 36, 52, 68, 84 };
        int row = pad / PS::kPadCols;
        int col = pad % PS::kPadCols;
        int rawNote = kBankBase[(int)currentBank] + (octaveOffset * 12) + (row * PS::kPadCols) + col;
        return quantizeToScale(juce::jlimit(0, 127, rawNote));
    }

    int quantizeToScale(int note) const
    {
        if (currentScale == 0) return note; // Chromatic
        auto& intervals = scales[(size_t)currentScale].intervals;
        int best = note;
        int bestDist = 999;
        for (int octSearch = -1; octSearch <= 1; ++octSearch)
        {
            for (int interval : intervals)
            {
                int candidate = rootKey + interval + ((note / 12) + octSearch) * 12;
                if (candidate < 0) continue; // P1-3: skip negative candidates
                int dist = std::abs(candidate - note);
                if (dist < bestDist) { bestDist = dist; best = candidate; }
            }
        }
        return juce::jlimit(0, 127, best);
    }

    // Returns true if 'note' is in the current scale (relative to rootKey).
    // Chromatic scale (index 0) always returns true.
    bool isNoteInScale(int note) const
    {
        if (currentScale == 0) return true;
        auto& intervals = scales[(size_t)currentScale].intervals;
        int relativeToRoot = ((note % 12) - rootKey + 12) % 12;
        for (auto interval : intervals)
            if (interval == relativeToRoot) return true;
        return false;
    }

    // FILTER mode: compute MIDI note for pad by walking up only in-scale semitones.
    // padIndex 0 = lowest in-scale note at or above the bank+octave base.
    // Fix #35: returns from cache (rebuilt lazily when scale/root/bank/octave changes).
    int scaleNoteForPad(int padIndex) const
    {
        if (scaleNotesDirty_)
            rebuildScaleNoteCache();
        if (padIndex >= 0 && padIndex < PS::kNumPads)
            return cachedScaleNotes_[static_cast<size_t>(padIndex)];
        return computeScaleNoteForPad(padIndex); // fallback for out-of-range index
    }

    // Fix #35: cache rebuild helper — called once on first access after a dirty state change.
    void rebuildScaleNoteCache() const
    {
        for (int i = 0; i < PS::kNumPads; ++i)
            cachedScaleNotes_[static_cast<size_t>(i)] = computeScaleNoteForPad(i);
        scaleNotesDirty_ = false;
    }

    int computeScaleNoteForPad(int padIndex) const
    {
        static constexpr int kBankBase[4] = { 36, 52, 68, 84 };
        int baseNote = kBankBase[(int)currentBank] + (octaveOffset * 12);

        if (currentScale == 0)
        {
            // Chromatic — every note is in scale; same as normal layout
            int row = padIndex / PS::kPadCols;
            int col = padIndex % PS::kPadCols;
            return juce::jlimit(0, 127, baseNote + (row * PS::kPadCols) + col);
        }

        auto& intervals = scales[(size_t)currentScale].intervals;
        int degree = 0;
        for (int n = baseNote; n <= 127; ++n)
        {
            int relativeToRoot = ((n % 12) - rootKey + 12) % 12;
            bool inScale = false;
            for (auto interval : intervals)
                if (interval == relativeToRoot) { inScale = true; break; }
            if (inScale)
            {
                if (degree == padIndex)
                    return juce::jlimit(0, 127, n);
                ++degree;
            }
        }
        // Fallback: return base note if we ran out of range
        return juce::jlimit(0, 127, baseNote);
    }

    void handleTouch(const juce::MouseEvent& e, bool isDown)
    {
        if (mode == Mode::Fretless)
        {
            handleFretlessTouch(e, isDown);
            return;
        }

        auto b = getLocalBounds().toFloat();

        // Square grid: match the same centering used in paintPadGrid().
        float padSide  = juce::jmin(b.getWidth() / PS::kPadCols, b.getHeight() / PS::kPadRows);
        float gridW    = padSide * PS::kPadCols;
        float gridH    = padSide * PS::kPadRows;
        float originX  = b.getX() + (b.getWidth()  - gridW) * 0.5f;
        float originY  = b.getY() + (b.getHeight() - gridH) * 0.5f;

        // Map mouse position into grid-local space, ignoring clicks outside the grid.
        float lx = (float)e.x - originX;
        float ly = (float)e.y - originY;
        if (lx < 0.0f || lx >= gridW || ly < 0.0f || ly >= gridH)
        {
            // Outside the grid area — fire note-off if we had one active.
            if (isDown && lastNote >= 0)
            {
                fireAftertouch(lastNote, 0.0f);
                fireNoteOff(lastNote);
                lastNote = -1;
                lastPad  = -1;
            }
            return;
        }

        float padW = padSide;
        float padH = padSide;

        int col = (int)(lx / padSide);
        int row = PS::kPadRows - 1 - (int)(ly / padSide);
        col = juce::jlimit(0, PS::kPadCols - 1, col);
        row = juce::jlimit(0, PS::kPadRows - 1, row);
        int pad = row * PS::kPadCols + col;

        // Velocity: Y position within the specific pad cell (top of pad = hard, bottom = soft).
        float padH2 = gridH / PS::kPadRows;
        int displayRow = PS::kPadRows - 1 - row;
        float padTopY = displayRow * padH2;
        float yInPad = juce::jlimit(0.0f, 1.0f, (ly - padTopY) / padH2);
        float velocity = juce::jlimit(0.05f, 1.0f, 1.0f - yInPad);
        int note = midiNoteForPad(pad);

        // HIGHLIGHT mode: quantize out-of-scale notes to nearest in-scale note
        if (scaleMode == ScaleMode::Highlight && mode != Mode::Drum)
            note = quantizeToScale(note);

        if (isDown || note != lastNote)
        {
            if (lastNote >= 0)
            {
                fireAftertouch(lastNote, 0.0f); // zero pressure before leaving the pad
                fireNoteOff(lastNote);
            }
            padVelocity[(size_t)pad] = velocity;
            warmMemory[(size_t)warmMemIdx] = { pad, 0.0f };
            warmMemIdx = (warmMemIdx + 1) % (int)warmMemory.size();
            lastNote = note;
            lastPad  = pad;
            fireNoteOn(note, velocity);
        }
        else if (!isDown && pad == lastPad && lastNote >= 0)
        {
            // Drag within the same pad: send aftertouch.
            // Aftertouch pressure = Y position within the pad cell.
            // top of cell = maximum pressure (1.0), bottom = minimum (0.0).
            int displayRow = PS::kPadRows - 1 - row; // flip: row 0 is rendered at top
            float padTopY = displayRow * padH;
            float yInPad  = juce::jlimit(0.0f, 1.0f, (ly - padTopY) / padH);
            // Top of pad → pressure 1.0; bottom → 0.0
            float pressure = juce::jlimit(0.0f, 1.0f, 1.0f - yInPad);
            fireAftertouch(lastNote, pressure);
        }
    }

    void handleFretlessTouch(const juce::MouseEvent& e, bool isDown)
    {
        auto b = getLocalBounds();

        // P0-2: X=pitch (left=low, right=high), Y=expression (bottom=dark,top=bright)
        // This is the XOuija spec axis: X drives pitch, Y drives expression.
        float xNorm = juce::jlimit(0.0f, 1.0f, (float)e.x / b.getWidth());
        float yNorm = juce::jlimit(0.0f, 1.0f,
                                   1.0f - (float)e.y / b.getHeight()); // bottom=0, top=1

        // Pitch from X: C1 (MIDI 24) → C7 (MIDI 96), 6-octave range
        float pitchF = 24.0f + xNorm * 72.0f;
        int   note   = juce::jlimit(24, 96, quantizeToScale((int)pitchF));

        // Expression from Y: bottom=softest, top=brightest.  Used as the
        // initial velocity on touch-down AND as an ongoing expression signal.
        float expression = juce::jmap(yNorm, 0.0f, 1.0f, 0.35f, 1.0f);
        expression = juce::jlimit(0.35f, 1.0f, expression);
        lastFretlessVelocity_ = expression;

        if (isDown)
        {
            // Touch began: fire note-on at the initial Y-derived velocity.
            if (lastNote >= 0) fireNoteOff(lastNote);
            lastNote = note;
            // Reset pitch bend to centre (8192 = no bend) before the new note.
            if (midiCollector)
            {
                auto msg = juce::MidiMessage::pitchWheel(midiChannel, 8192);
                msg.setTimeStamp(juce::Time::getMillisecondCounterHiRes() * 0.001);
                midiCollector->addMessageToQueue(msg);
            }
            fireNoteOn(note, expression);
        }
        else
        {
            // P0-3: drag — do NOT fire note-on/off storms.
            // Instead: if the quantised note has crossed a semitone boundary,
            // shift the base note via a new note-on ONLY if the semitone
            // changed, then use pitch bend for sub-semitone continuous pitch.
            // For continuous fretless glide we use pitch bend relative to the
            // base note that was sounded on touch-down (lastNote).
            if (lastNote >= 0)
            {
                // Compute continuous float pitch relative to lastNote
                // and convert to a 14-bit pitch bend value
                // (bend range assumed ±2 semitones; standard MIDI default).
                static constexpr float kBendSemitones = 2.0f;
                float bendSemitones = pitchF - (float)lastNote;
                float bendNorm = juce::jlimit(-1.0f, 1.0f,
                                              bendSemitones / kBendSemitones);
                // MIDI pitch wheel: 0=full down, 8192=centre, 16383=full up
                int pitchWheelValue = 8192 + (int)(bendNorm * 8191.0f);
                pitchWheelValue = juce::jlimit(0, 16383, pitchWheelValue);

                if (midiCollector)
                {
                    auto msg = juce::MidiMessage::pitchWheel(midiChannel, pitchWheelValue);
                    msg.setTimeStamp(juce::Time::getMillisecondCounterHiRes() * 0.001);
                    midiCollector->addMessageToQueue(msg);
                }
            }
        }
    }

    // Pad grid — Engine Accent Adaptive style.
    // Pads are always square (1:1 aspect ratio), grid centered within available bounds.
    void paintPadGrid(juce::Graphics& g)
    {
        using namespace PS;
        auto b = getLocalBounds().toFloat();

        // Enforce square pads: compute the largest square cell that fits
        float padSide = juce::jmin(b.getWidth() / kPadCols, b.getHeight() / kPadRows);
        float gridW   = padSide * kPadCols;
        float gridH   = padSide * kPadRows;
        // Center the grid within available bounds
        float originX = b.getX() + (b.getWidth()  - gridW) * 0.5f;
        float originY = b.getY() + (b.getHeight() - gridH) * 0.5f;

        float padW = padSide;
        float padH = padSide;

        for (int row = 0; row < kPadRows; ++row)
        {
            for (int col = 0; col < kPadCols; ++col)
            {
                int pad = row * kPadCols + col;
                int displayRow = kPadRows - 1 - row; // flip for bottom-left origin
                float x = originX + col * padW;
                float y = originY + displayRow * padH;
                auto padRect = juce::Rectangle<float>(x, y, padW, padH).reduced(2.0f);

                float vel = padVelocity[(size_t)pad];
                bool isHit = (vel > 0.01f);

                if (isHit)
                {
                    // P1-4: Draw glow shadow FIRST so subsequent pads paint over any bleed
                    g.setColour(accentColour.withAlpha(vel * 0.25f));
                    g.fillRoundedRectangle(padRect.expanded(3.0f), 5.0f);

                    // Hit pad: radial gradient from accent @ 0.42 center to accent @ 0.07 edge
                    float cx = padRect.getCentreX(), cy = padRect.getCentreY();
                    float r  = padRect.getWidth() * 0.7f;
                    juce::ColourGradient grad(accentColour.withAlpha(0.07f + vel * 0.35f), cx, cy,
                                             accentColour.withAlpha(0.07f), cx + r, cy, true);
                    g.setGradientFill(grad);
                    g.fillRoundedRectangle(padRect, 4.0f);

                    // Hit border: accent @ 0.50
                    g.setColour(accentColour.withAlpha(0.50f));
                    g.drawRoundedRectangle(padRect, 4.0f, 1.5f);
                }
                else
                {
                    // Determine scale visibility for this pad (only in Pad mode, not Drum)
                    int padNote = midiNoteForPad(pad);
                    bool inScale = (mode == Mode::Drum) || isNoteInScale(padNote);
                    bool dimPad  = (scaleMode == ScaleMode::Highlight && mode != Mode::Drum && !inScale);

                    float borderAlpha = dimPad ? 0.08f : 0.18f;

                    // Non-hit pad: accent @ 0.07 fill
                    g.setColour(accentColour.withAlpha(0.07f));
                    g.fillRoundedRectangle(padRect, 4.0f);

                    // Border: dimmed or normal
                    g.setColour(accentColour.withAlpha(borderAlpha));
                    g.drawRoundedRectangle(padRect, 4.0f, 1.0f);

                    // Root key accent: XO Gold 2px bottom border
                    // P2-3: In HIGHLIGHT mode use quantized note for root detection
                    int rootCheckNote = (scaleMode == ScaleMode::Highlight && mode != Mode::Drum)
                                        ? quantizeToScale(padNote) : padNote;
                    bool isRootPad = (scaleMode != ScaleMode::Off && mode != Mode::Drum
                                      && (rootCheckNote % 12) == rootKey);
                    if (isRootPad)
                    {
                        juce::Colour xoGold(0xFFE9C46A);
                        float bx = padRect.getX();
                        float by = padRect.getBottom() - 2.0f;
                        float bw = padRect.getWidth();
                        g.setColour(xoGold);
                        g.fillRect(bx, by, bw, 2.0f);
                    }
                }

                // XOuija-reactive coloring (Spec Section 8.2)
                // Additive overlay drawn after base pad rendering; only in Pad mode.
                if (mode == Mode::Pad)
                {
                    int midiNote = midiNoteForPad(pad);
                    bool inKey  = HarmonicField::isInKey(midiNote, harmonicRootKey_);
                    bool isRoot = HarmonicField::isRoot(midiNote, harmonicRootKey_);

                    auto [tr, tg, tb] = HarmonicField::tensionColor(harmonicTension_);
                    juce::Colour tensionColour = juce::Colour::fromFloatRGBA(tr, tg, tb, 1.0f);

                    if (!inKey)
                    {
                        // Out-of-key: dim to 20% opacity overlay
                        g.setColour(juce::Colours::black.withAlpha(0.80f));
                        g.fillRoundedRectangle(padRect, 4.0f);
                    }
                    else
                    {
                        // In-key: glow with tension color
                        g.setColour(tensionColour.withAlpha(0.15f));
                        g.fillRoundedRectangle(padRect, 4.0f);
                    }

                    if (isRoot)
                    {
                        // Root note: XO Gold bottom border (4px)
                        g.setColour(juce::Colour(0xffE9C46A));
                        auto borderRect = padRect;
                        g.fillRect(borderRect.removeFromBottom(4.0f));
                    }
                }

                // Warm memory ghost circles — radial gradient using accent
                for (const auto& wm : warmMemory)
                {
                    if (wm.pad == pad && wm.age < kWarmMemoryDur)
                    {
                        float alpha = (1.0f - wm.age / kWarmMemoryDur) * 0.15f;
                        float pcx = padRect.getCentreX(), pcy = padRect.getCentreY();
                        float gr = padRect.getWidth() * 0.22f; // P2-6: dynamic radius
                        juce::ColourGradient ghostGrad(accentColour.withAlpha(alpha * 2.0f), pcx, pcy,
                                                       accentColour.withAlpha(0.0f), pcx + gr, pcy, true);
                        g.setGradientFill(ghostGrad);
                        g.fillEllipse(pcx - gr, pcy - gr, gr * 2, gr * 2);
                    }
                }

                // Note label
                int note = midiNoteForPad(pad);
                static const char* noteNames[] = {"C","Db","D","Eb","E","F","Gb","G","Ab","A","Bb","B"};
                juce::String label;
                if (mode == Mode::Drum)
                {
                    // P0-4: drum labels match MPC Bank A standard note table
                    static const char* drumNames[] = {
                        "Kick A","Kick B","Snr A","Snr B",  // row 0 (bottom)
                        "Clap","HH-C","HH-O","Ride",        // row 1
                        "Tom 1","Perc A","Perc B","Crash",   // row 2
                        "Fx 1","Fx 2","Fx 3","Fx 4",        // row 3 (top)
                    };
                    label = drumNames[pad];
                }
                else
                {
                    // P1-1: In HIGHLIGHT mode the fired note is quantized, so show
                    // the quantized note as the label so the display matches what plays.
                    int displayNote = (scaleMode == ScaleMode::Highlight)
                                      ? quantizeToScale(note) : note;
                    label = juce::String(noteNames[displayNote % 12]) + juce::String(displayNote / 12 - 1);
                }

                // Note label: white on hit; dimmed (0.25) for out-of-scale pads in Highlight mode; accent @ 0.55 otherwise
                float labelAlpha = 0.55f;
                if (!isHit && scaleMode == ScaleMode::Highlight && mode != Mode::Drum && !isNoteInScale(note))
                    labelAlpha = 0.25f;
                g.setColour(isHit ? juce::Colours::white : accentColour.withAlpha(labelAlpha));
                g.setFont(juce::Font(juce::FontOptions{}.withHeight(9.0f)));
                g.drawText(label, padRect, juce::Justification::centred);
            }
        }

        // Bank badge — top-right corner, accent-tinted
        {
            static const char* kBankNames[] = { "A", "B", "C", "D" };
            juce::String badge = juce::String("BNK ") + kBankNames[(int)currentBank];
            auto badgeRect = juce::Rectangle<float>(
                originX + gridW - 50.0f, originY, 48.0f, 16.0f);
            g.setColour(accentColour.withAlpha(0.75f));
            g.setFont(juce::Font(juce::FontOptions{}.withHeight(9.0f)).boldened());
            g.drawText(badge, badgeRect, juce::Justification::centredRight);
        }
    }

    // Zone-aware fretless paintFretless:
    //   bottom (low notes) = Midnight zone (violet)
    //   middle             = Twilight zone (deep blue)
    //   top (high notes)   = Sunlit zone (cyan)
    // Matches the web PlaySurface ocean depth gradient.
    void paintFretless(juce::Graphics& g)
    {
        using namespace PS;
        auto b = getLocalBounds().toFloat();

        // ── Ocean depth background gradient (bottom=midnight, top=sunlit) ───
        {
            juce::ColourGradient grad(juce::Colour(0xFF150820), b.getX(), b.getBottom(),  // Midnight base
                                       juce::Colour(0xFF0A1F30), b.getX(), b.getY(), false);
            grad.addColour(0.20, juce::Colour(0xFF7B2FBE).withAlpha(0.35f));   // Midnight zone
            grad.addColour(0.45, juce::Colour(0xFF0D2744));                     // Twilight zone base
            grad.addColour(0.72, juce::Colour(0xFF0096C7).withAlpha(0.5f));    // Twilight zone top
            grad.addColour(0.85, juce::Colour(0xFF0D3550));                     // Sunlit base
            g.setGradientFill(grad);
            g.fillRect(b);
        }

        // Zone boundary lines — 0.30 alpha: more prominent than octave fret lines (0.35) to read as
        // structural zone dividers rather than just another fret line.
        {
            float sunlitY   = b.getBottom() - b.getHeight() * 0.45f;  // 45% up = twilight boundary
            float twilightY = b.getBottom() - b.getHeight() * 0.80f;  // 80% up = sunlit boundary
            g.setColour(juce::Colour(0xFF0096C7).withAlpha(0.30f));
            g.drawHorizontalLine((int)sunlitY,   b.getX(), b.getRight());
            g.setColour(juce::Colour(0xFF48CAE4).withAlpha(0.30f));
            g.drawHorizontalLine((int)twilightY, b.getX(), b.getRight());
        }

        // ── Zone-colored fret lines ──────────────────────────────────────────
        auto& intervals = scales[(size_t)currentScale].intervals;
        int totalSemitones = 72; // C1-C7
        for (int semi = 0; semi <= totalSemitones; ++semi)
        {
            float y = b.getBottom() - (float)semi / totalSemitones * b.getHeight();
            float normY = 1.0f - (float)semi / totalSemitones;  // 0=top/sunlit, 1=bottom/midnight

            // Choose zone color for this position
            juce::Colour zoneColor;
            if (normY < 0.20f)      zoneColor = juce::Colour(0xFF48CAE4);   // Sunlit — cyan
            else if (normY < 0.55f) zoneColor = juce::Colour(0xFF0096C7);   // Twilight — blue
            else                     zoneColor = juce::Colour(0xFF7B2FBE);   // Midnight — violet

            bool isOctave = (semi % 12 == 0);
            bool isScaleNote = (currentScale == 0) ||
                std::find(intervals.begin(), intervals.end(), semi % 12) != intervals.end();

            if (isOctave)
            {
                g.setColour(zoneColor.withAlpha(0.35f));
                g.drawHorizontalLine((int)y, b.getX(), b.getRight());
                int octNum = (24 + semi) / 12 - 1;
                g.setFont(juce::Font(juce::FontOptions{}.withHeight(8.0f)));
                g.setColour(zoneColor.withAlpha(0.55f));
                g.drawText("C" + juce::String(octNum),
                    juce::Rectangle<float>(4, y - 10, 24, 12),
                    juce::Justification::centredLeft);
            }
            else if (isScaleNote)
            {
                g.setColour(zoneColor.withAlpha(0.12f));
                g.drawHorizontalLine((int)y, b.getX(), b.getRight());
            }
        }

        // ── Touch cursor — XO Gold ring + zone-colored glow ─────────────────
        if (lastNote >= 0)
        {
            float noteY = b.getBottom() - (float)(lastNote - 24) / totalSemitones * b.getHeight();
            float normY = 1.0f - (float)(lastNote - 24) / totalSemitones;

            juce::Colour zoneColor;
            if (normY < 0.20f)      zoneColor = juce::Colour(0xFF48CAE4);
            else if (normY < 0.55f) zoneColor = juce::Colour(0xFF0096C7);
            else                     zoneColor = juce::Colour(0xFF7B2FBE);

            // Zone-colored guide line across the strip
            g.setColour(zoneColor.withAlpha(0.22f));
            g.drawHorizontalLine((int)noteY, b.getX(), b.getRight());

            // XO Gold ring cursor (28pt equivalent on desktop)
            const float ringR = 14.0f;
            float cx = b.getCentreX();

            // Glow radius scales with velocity: softest touch = tight glow, hardest = wide halo
            float glowR = ringR + 4.0f + lastFretlessVelocity_ * 6.0f;

            // Zone glow behind ring — radius driven by expression
            g.setColour(zoneColor.withAlpha(0.20f));
            g.fillEllipse(cx - glowR, noteY - glowR, glowR * 2, glowR * 2);

            // P1-2: Use accentColour (not hardcoded XO Gold) for fretless cursor
            g.setColour(accentColour);
            g.drawEllipse(cx - ringR, noteY - ringR, ringR * 2, ringR * 2, 2.5f);

            // Center dot
            g.setColour(accentColour.withAlpha(0.7f));
            g.fillEllipse(cx - 3, noteY - 3, 6, 6);

            // Velocity arc indicator — 60° sweep at top of ring, alpha proportional to velocity
            {
                const float arcSweepDeg = 60.0f;
                const float startDeg    = 270.0f - arcSweepDeg * 0.5f; // centred at top (270° = 12 o'clock)
                juce::Path arcPath;
                arcPath.addArc(cx - ringR, noteY - ringR, ringR * 2, ringR * 2,
                               juce::degreesToRadians(startDeg),
                               juce::degreesToRadians(startDeg + arcSweepDeg),
                               true);
                g.setColour(accentColour.withAlpha(lastFretlessVelocity_ * 0.9f));
                g.strokePath(arcPath, juce::PathStrokeType(3.0f));
            }
        }
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NoteInputZone)
};

//==============================================================================
// Zone 3: Performance Strip — Full-width XY gestural controller
// (V1 OrbitPathZone removed — dead code, not used in V2 layout)
//
class PerformanceStrip : public juce::Component
{
public:
    PerformanceStrip() = default;
    enum class StripMode { DubSpace, FilterSweep, Coupling, DubSiren };

    std::function<void(float x, float y)> onPositionChanged;

    void setStripMode(StripMode m)
    {
        stripMode = m;
        repaint();
    }

    // Engine accent colour — set by PlaySurface::setAccentColour()
    juce::Colour accentColour { 0xFFE9C46A };
    void setAccentColour(juce::Colour c) { accentColour = c; repaint(); }

    void paint(juce::Graphics& g) override
    {
        using namespace PS;
        auto b = getLocalBounds().toFloat();

        // Background: linear gradient accent @ 0.08 top → accent @ 0.03 bottom
        {
            juce::ColourGradient bg(accentColour.withAlpha(0.08f), b.getX(), b.getY(),
                                    accentColour.withAlpha(0.03f), b.getX(), b.getBottom(), false);
            g.setGradientFill(bg);
            g.fillRect(b);
        }

        // Mode-specific tint overlay (preserves per-mode colour identity)
        {
            static constexpr uint32_t kStripModeTints[] = {
                0xFF2D4D5A,  // DubSpace  — dark teal
                0xFF00A6D6,  // FilterSweep — teal
                0xFFE9C46A,  // Coupling — XO Gold
                0xFFFF6B6B,  // DubSiren — warm red
            };
            g.setColour(juce::Colour(kStripModeTints[(int)stripMode]).withAlpha(0.08f));
            g.fillRect(b);
        }

        // Mode tabs inside strip (Spec Section 9.2) — rendered at left edge
        {
            constexpr const char* kModeTabNames[] = { "DubSpace", "FilterSweep", "Coupling", "DubSiren" };
            constexpr float tabW = 72.0f;
            constexpr float tabH = 22.0f;
            float tabY = (b.getHeight() - tabH) / 2.0f + b.getY();

            g.setFont(juce::Font(juce::FontOptions{}.withHeight(8.0f)));
            for (int i = 0; i < 4; ++i)
            {
                auto tabRect = juce::Rectangle<float>(b.getX() + i * tabW + 4.0f, tabY, tabW - 4.0f, tabH);
                bool active = (i == static_cast<int>(stripMode));

                if (active)
                {
                    g.setColour(accentColour.withAlpha(0.25f));
                    g.fillRoundedRectangle(tabRect, 3.0f);
                }

                g.setColour(juce::Colours::white.withAlpha(active ? 1.0f : 0.45f));
                g.drawText(kModeTabNames[i], tabRect, juce::Justification::centred);
            }
        }

        // Gestural area is to the right of the 4 tabs (4 × 72 = 288px offset)
        auto gestureArea = b.withTrimmedLeft(288.0f + 4.0f);

        // Crosshair gridlines (Spec Section 9.3) — opacity 0.10, 1px white
        g.setColour(juce::Colours::white.withAlpha(0.10f));
        float midY = gestureArea.getCentreY();
        float midX = gestureArea.getCentreX();
        g.drawHorizontalLine(static_cast<int>(midY), gestureArea.getX(), gestureArea.getRight());
        g.drawVerticalLine(static_cast<int>(midX), gestureArea.getY(), gestureArea.getBottom());

        // Gesture trail — accent coloured (constrained to gestureArea)
        for (int i = 0; i < kStripTrailSize; ++i)
        {
            int idx = (stripTrailHead - i + kStripTrailSize) % kStripTrailSize;
            auto& pt = stripTrail[idx];
            if (pt.age > kWarmMemoryDur) continue;
            float alpha = (1.0f - pt.age / kWarmMemoryDur) * 0.5f;
            float sx = gestureArea.getX() + pt.x * gestureArea.getWidth();
            float sy = gestureArea.getY() + (1.0f - pt.y) * gestureArea.getHeight();
            g.setColour(accentColour.withAlpha(alpha));
            g.fillEllipse(sx - 2, sy - 2, 4, 4);
        }

        // Cursor position mapped to gestureArea
        float cx = gestureArea.getX() + stripX * gestureArea.getWidth();
        float cy = gestureArea.getY() + (1.0f - stripY) * gestureArea.getHeight();

        // Vertical bar from cursor to strip floor (Spec Section 9.3)
        g.setColour(accentColour.withAlpha(0.30f));
        g.drawVerticalLine(static_cast<int>(cx), cy, gestureArea.getBottom());

        // Floor glow (horizontal ellipse at bottom of vertical bar)
        g.setColour(accentColour.withAlpha(0.30f));
        g.fillEllipse(cx - 8.0f, gestureArea.getBottom() - 4.0f, 16.0f, 4.0f);

        // Glow halo (16px diameter, 60% opacity)
        g.setColour(accentColour.withAlpha(0.60f));
        g.fillEllipse(cx - 8.0f, cy - 8.0f, 16.0f, 16.0f);

        // Core dot (10px diameter, full accent)
        g.setColour(accentColour);
        g.fillEllipse(cx - 5.0f, cy - 5.0f, 10.0f, 10.0f);

        // Axis labels — 8px uppercase, 35% opacity, at the edges of gestureArea (Spec Section 9.4)
        // Mode-specific: X axis = bottom-left of gestureArea, Y axis = top-right of gestureArea
        g.setFont(juce::Font(juce::FontOptions{}.withHeight(8.0f)));
        g.setColour(juce::Colours::white.withAlpha(0.35f));
        static const char* xLabels[] = { "DELAY FB", "CUTOFF",  "SPREAD",  "PITCH" };
        static const char* yLabels[] = { "REVERB",   "RESONANCE", "DEPTH", "SIREN DEPTH" };
        g.drawText(xLabels[(int)stripMode], gestureArea.reduced(4), juce::Justification::bottomLeft);
        g.drawText(yLabels[(int)stripMode], gestureArea.reduced(4), juce::Justification::topRight);
    }

    void mouseDown(const juce::MouseEvent& e) override { updateStrip(e); touching = true; }
    void mouseDrag(const juce::MouseEvent& e) override { if (touching) updateStrip(e); }
    void mouseUp(const juce::MouseEvent&) override
    {
        touching = false;
        // Record release position as spring-back start; reset elapsed to begin 250ms ease-out
        springStartX_  = stripX;
        springStartY_  = stripY;
        springElapsed_ = 0.0f;
    }

    void tick()
    {
        float prevX = stripX, prevY = stripY;

        if (!touching)
        {
            // Spring-back: 250ms quadratic ease-out (Spec Section 9.5)
            // kSpringDuration = 0.250s; timer runs at 30fps → each tick = 1/30s
            constexpr float kSpringDuration = 0.250f;  // seconds
            float targetX = springTargets[(int)stripMode].x;
            float targetY = springTargets[(int)stripMode].y;

            springElapsed_ += 1.0f / 30.0f;
            float t = juce::jlimit(0.0f, 1.0f, springElapsed_ / kSpringDuration);
            // Quadratic ease-out: starts fast, decelerates to target
            float eased = 1.0f - std::pow(1.0f - t, 2.0f);

            stripX = springStartX_ + (targetX - springStartX_) * eased;
            stripY = springStartY_ + (targetY - springStartY_) * eased;
        }

        // Age trail
        bool hasActiveTrail = false;
        for (auto& pt : stripTrail)
        {
            pt.age += 1.0f / 30.0f;
            if (pt.age < PS::kWarmMemoryDur) hasActiveTrail = true;
        }

        if (onPositionChanged)
            onPositionChanged(stripX, stripY);

        // Only repaint when position moved or trails are still fading
        bool moved = std::fabs(stripX - prevX) > 0.0001f || std::fabs(stripY - prevY) > 0.0001f;
        if (moved || touching || hasActiveTrail)
            repaint();
    }

private:
    StripMode stripMode = StripMode::DubSpace;
    float stripX = 0.3f, stripY = 0.2f;
    bool  touching = false;

    // Spring-back animation state (250ms ease-out)
    float springElapsed_ = 999.0f;   // Start in "done" state
    float springStartX_  = 0.3f;
    float springStartY_  = 0.2f;

    struct SpringTarget { float x, y; };
    static constexpr SpringTarget springTargets[] = {
        { 0.3f, 0.2f },  // DubSpace
        { 0.3f, 0.3f },  // FilterSweep (cutoff ~1200Hz log)
        { 0.3f, 0.15f }, // Coupling
        { 0.5f, 0.5f },  // DubSiren (center)
    };

    struct TrailPoint { float x = 0.0f, y = 0.0f; float age = 99.0f; };
    std::array<TrailPoint, PS::kStripTrailSize> stripTrail {};
    int stripTrailHead = 0;

    void updateStrip(const juce::MouseEvent& e)
    {
        // Gestural area is to the right of the 4 mode tabs (4 × 72 + 4px padding = 292px)
        auto b = getLocalBounds().toFloat();
        auto gestureArea = b.withTrimmedLeft(292.0f);
        float gx = juce::jlimit(gestureArea.getX(), gestureArea.getRight(), static_cast<float>(e.x));
        stripX = juce::jlimit(0.0f, 1.0f, (gx - gestureArea.getX()) / gestureArea.getWidth());
        stripY = juce::jlimit(0.0f, 1.0f, 1.0f - (e.y - b.getY()) / b.getHeight());

        stripTrail[(size_t)stripTrailHead] = { stripX, stripY, 0.0f };
        stripTrailHead = (stripTrailHead + 1) % PS::kStripTrailSize;
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PerformanceStrip)
};

// V1 PerformancePads removed — dead code, not used in V2 layout (Fix #40).

//==============================================================================
// PlaySurface — Unified 4-zone playing interface
//
// Composes the four zones with a header bar for mode/scale/octave controls.
// Designed to live inside a PlaySurfaceWindow (DocumentWindow popup).
// Default popup size: 520×520px (square), so pads are 1:1 ratio.
//
class PlaySurface : public juce::Component, private juce::Timer
{
public:
    PlaySurface()
    {
        setTitle ("PlaySurface");
        setDescription ("V2 performance interface: XOuija panel, note input / keys mode, performance strip");
        setWantsKeyboardFocus (true);

        noteInput.setTitle ("Note Input Zone");
        noteInput.setDescription ("Pad grid or fretless strip for note input");
        strip.setTitle ("Performance Strip");
        strip.setDescription ("Touch strip for filter sweeps, coupling, and dub effects");

        addAndMakeVisible(noteInput);
        addAndMakeVisible(strip);

        // V2: XOuija panel + KeysMode
        addAndMakeVisible(xouijaPanel_);
        addAndMakeVisible(keysMode_);
        keysMode_.setVisible(false); // Initially hidden — shown when mode == Keys

        // Wire XOuija position changes -> pad and keys coloring
        xouijaPanel_.onPositionChanged = [this](float circleX, float /*influenceY*/)
        {
            int key     = HarmonicField::positionToKey(circleX);
            // Tension = distance from C (tonal center). This is intentional:
            // the XOuija circle measures harmonic distance from the global
            // reference, not from any transposed root.
            int tension = HarmonicField::fifthsDistance(key, 0);
            noteInput.setHarmonicField(key, tension);
            keysMode_.setRootKey(key);
        };

        // Wire GOODBYE -> All Notes Off CC #123 on channels 1-16
        xouijaPanel_.onGoodbye = [this]()
        {
            if (auto* mc = noteInput.midiCollector)
            {
                double ts = juce::Time::getMillisecondCounterHiRes() / 1000.0;
                for (int ch = 1; ch <= 16; ++ch)
                {
                    auto msg = juce::MidiMessage::controllerEvent(ch, 123, 0);
                    msg.setTimeStamp(ts);
                    mc->addMessageToQueue(msg);
                }
            }
        };

        // Wire KeysMode MIDI collector (same as NoteInputZone's)
        keysMode_.midiCollector = noteInput.midiCollector;

        // Header controls: mode buttons (V2: PAD | DRUM | KEYS — FRETLESS collapsed into PAD)
        for (int i = 0; i < 3; ++i)
        {
            modeButtons[i].setClickingTogglesState(true);
            modeButtons[i].setRadioGroupId(101);
            addAndMakeVisible(modeButtons[i]);
        }
        modeButtons[0].setButtonText("PAD");
        modeButtons[1].setButtonText("DRUM");
        modeButtons[2].setButtonText("KEYS");
        modeButtons[0].setToggleState(true, juce::dontSendNotification);

        // V2 mode wiring: PAD=0, DRUM=2 (maps to NoteInputZone::Mode::Drum), KEYS=3
        modeButtons[0].onClick = [this]()
        {
            noteInput.setMode(NoteInputZone::Mode::Pad);
            resized(); // swap NoteInputZone <-> KeysMode visibility
        };
        modeButtons[1].onClick = [this]()
        {
            noteInput.setMode(NoteInputZone::Mode::Drum);
            resized();
        };
        modeButtons[2].onClick = [this]()
        {
            noteInput.setMode(NoteInputZone::Mode::Keys);
            resized(); // show KeysMode, hide NoteInputZone
        };

        // Octave controls
        octDownBtn.setButtonText("-");
        octUpBtn.setButtonText("+");
        octLabel.setText("OCT 0", juce::dontSendNotification);
        octLabel.setJustificationType(juce::Justification::centred);
        octLabel.setColour(juce::Label::textColourId, juce::Colour(PS::kTextLight));
        addAndMakeVisible(octDownBtn);
        addAndMakeVisible(octUpBtn);
        addAndMakeVisible(octLabel);

        octDownBtn.onClick = [this]
        {
            noteInput.setOctave(noteInput.getOctave() - 1);
            octLabel.setText("OCT " + juce::String(noteInput.getOctave()), juce::dontSendNotification);
        };
        octUpBtn.onClick = [this]
        {
            noteInput.setOctave(noteInput.getOctave() + 1);
            octLabel.setText("OCT " + juce::String(noteInput.getOctave()), juce::dontSendNotification);
        };

        // Bank selector buttons (A/B/C/D).
        // Selects the 16-note bank offset for Pad and Drum modes.
        static const char* bankLabels[] = { "A", "B", "C", "D" };
        for (int i = 0; i < 4; ++i)
        {
            bankButtons[i].setClickingTogglesState(true);
            bankButtons[i].setRadioGroupId(103);
            bankButtons[i].setButtonText(bankLabels[i]);
            addAndMakeVisible(bankButtons[i]);
        }
        bankButtons[0].setToggleState(true, juce::dontSendNotification);

        for (int i = 0; i < 4; ++i)
        {
            bankButtons[i].onClick = [this, i]
            {
                noteInput.setBank(static_cast<NoteInputZone::Bank>(i));
            };
        }

        // Scale mode button — cycles Off → Filter → Highlight → Off
        scaleModeBtn.setButtonText("SCL");
        addAndMakeVisible(scaleModeBtn);
        scaleModeBtn.onClick = [this]
        {
            // P2-2: use setter rather than direct mutation of scaleMode field
            auto& ni = noteInput;
            NoteInputZone::ScaleMode next;
            if (ni.scaleMode == NoteInputZone::ScaleMode::Off)
                next = NoteInputZone::ScaleMode::Filter;
            else if (ni.scaleMode == NoteInputZone::ScaleMode::Filter)
                next = NoteInputZone::ScaleMode::Highlight;
            else
                next = NoteInputZone::ScaleMode::Off;

            ni.setScaleMode(next);

            switch (ni.scaleMode) {
                case NoteInputZone::ScaleMode::Off:       scaleModeBtn.setButtonText("SCL"); break;
                case NoteInputZone::ScaleMode::Filter:    scaleModeBtn.setButtonText("FLT"); break;
                case NoteInputZone::ScaleMode::Highlight: scaleModeBtn.setButtonText("HLT"); break;
            }
        };

        // Strip mode buttons
        for (int i = 0; i < 4; ++i)
        {
            stripModeButtons[i].setClickingTogglesState(true);
            stripModeButtons[i].setRadioGroupId(102);
            addAndMakeVisible(stripModeButtons[i]);
        }
        stripModeButtons[0].setButtonText("DUB");
        stripModeButtons[1].setButtonText("FILT");
        stripModeButtons[2].setButtonText("CPL");
        stripModeButtons[3].setButtonText("SIREN");
        stripModeButtons[0].setToggleState(true, juce::dontSendNotification);

        for (int i = 0; i < 4; ++i)
        {
            stripModeButtons[i].onClick = [this, i]
            {
                strip.setStripMode(static_cast<PerformanceStrip::StripMode>(i));
            };
        }

        // F11: Explicit button colors — override default JUCE styling
        {
            auto applyBtnColors = [](juce::TextButton& btn) {
                btn.setColour(juce::TextButton::buttonColourId,   juce::Colour(0xFF2D2D2D));
                btn.setColour(juce::TextButton::buttonOnColourId, GalleryColors::get(GalleryColors::xoGold));
                btn.setColour(juce::TextButton::textColourOffId,  juce::Colour(0xFFAAAAAA));
                btn.setColour(juce::TextButton::textColourOnId,   GalleryColors::get(GalleryColors::textDark()));
            };
            for (int i = 0; i < 3; ++i) applyBtnColors(modeButtons[i]);
            for (int i = 0; i < 4; ++i) applyBtnColors(bankButtons[i]);
            for (int i = 0; i < 4; ++i) applyBtnColors(stripModeButtons[i]);
            applyBtnColors(octDownBtn);
            applyBtnColors(octUpBtn);
            applyBtnColors(scaleModeBtn);
        }

        // Wire XOuija CC output — processor_ is null at this point; the lambda
        // will forward once setProcessor() is called from the editor.
        wireOnCCOutput();

        // Timer is started in visibilityChanged() when the window becomes visible.
    }

    ~PlaySurface() override { stopTimer(); }

    // ── Task 12: Processor wiring ─────────────────────────────────────────────
    // Call setProcessor() from XOlokunEditor after construction so that XOuija
    // CC output events (CC 85-90) are forwarded to the audio thread via the
    // processor's lock-free CC queue.  The processor pointer is NOT owned here.
    void setProcessor (XOlokunProcessor* p)
    {
        processor_ = p;
        // Re-wire the onCCOutput callback now that we have the processor.
        wireOnCCOutput();
    }

    // Handle incoming CC for remote planchette control.
    // Call this from XOlokunEditor::handleIncomingMidi() to allow external
    // hardware controllers to drive the XOuija planchette.
    //
    //   CC 86 — influence depth  (0-127 → 0.0-1.0)
    //   CC 89 — home snap        (value==127 → reset planchette to centre)
    //   CC 90 — drift toggle     (future use; state stored but not yet consumed)
    void handleIncomingCC (int cc, int value)
    {
        if (cc == 86)
        {
            xouijaPanel_.setInfluenceDepth (value / 127.0f);
        }
        else if (cc == 89 && value == 127)
        {
            xouijaPanel_.setCirclePosition  (0.5f);
            xouijaPanel_.setInfluenceDepth  (0.5f);
        }
        else if (cc == 90)
        {
            // Drift toggle — store for Task 13 verification.
            driftToggleState_ = (value >= 64);
        }
    }

    // P0-1: Wire the MIDI pipeline.
    // Call this from XOlokunEditor after construction, passing the processor's
    // MidiMessageCollector.  Once set, all note-on/off events from the PlaySurface
    // are delivered to the audio thread via the collector rather than the raw
    // std::function callbacks.
    void setMidiCollector(juce::MidiMessageCollector* collector, int channel = 1)
    {
        noteInput.midiCollector = collector;
        noteInput.midiChannel   = channel;
        // V2: also wire KeysMode
        keysMode_.midiCollector = collector;
        keysMode_.midiChannel   = channel;
    }

    // Engine Accent Adaptive: propagate accent colour to all four zones.
    // Call this from XOlokunEditor::timerCallback() when the active engine changes.
    void setAccentColour(juce::Colour c)
    {
        if (c == accentColour) return; // B1: early-return guard — skip repaints if unchanged
        accentColour = c;
        noteInput.setAccentColour(c);
        strip.setAccentColour(c);
        // V2 components
        xouijaPanel_.setAccentColour(c);
        keysMode_.setAccentColour(c);

        // P2-1: also update header button "on" colours so mode/bank/strip buttons
        // reflect the current engine accent when toggled.
        auto updateBtnAccent = [&](juce::TextButton& btn) {
            btn.setColour(juce::TextButton::buttonOnColourId, c);
        };
        for (int i = 0; i < 3; ++i) updateBtnAccent(modeButtons[i]);
        for (int i = 0; i < 4; ++i) updateBtnAccent(bankButtons[i]);
        for (int i = 0; i < 4; ++i) updateBtnAccent(stripModeButtons[i]);
        updateBtnAccent(octDownBtn);
        updateBtnAccent(octUpBtn);
        updateBtnAccent(scaleModeBtn);

        repaint();
    }

    // Public zone accessors for wiring callbacks
    NoteInputZone&     getNoteInput()  { return noteInput; }
    PerformanceStrip&  getStrip()     { return strip; }

    void resized() override
    {
        auto bounds = getLocalBounds();

        // ── Header bar (mode tabs + octave + bank + scale) ──────────────────
        auto header = bounds.removeFromTop(PS::kHeaderH);
        // V2 mode tabs: PAD | DRUM | KEYS
        int btnW = 48;
        for (int i = 0; i < 3; ++i)
            modeButtons[i].setBounds(header.removeFromLeft(btnW).reduced(2));
        header.removeFromLeft(4);
        octDownBtn.setBounds(header.removeFromLeft(24).reduced(2));
        octLabel.setBounds(header.removeFromLeft(36).reduced(2));
        octUpBtn.setBounds(header.removeFromLeft(24).reduced(2));

        // Bank selector buttons
        header.removeFromLeft(4);
        for (int i = 0; i < 4; ++i)
            bankButtons[i].setBounds(header.removeFromLeft(20).reduced(2));

        // Scale mode button
        header.removeFromLeft(4);
        scaleModeBtn.setBounds(header.removeFromLeft(32).reduced(2));

        // Strip mode buttons at right of header
        for (int i = 3; i >= 0; --i)
            stripModeButtons[i].setBounds(header.removeFromRight(36).reduced(2));

        // ── Performance Strip — full width, bottom ───────────────────────────
        strip.setBounds(bounds.removeFromBottom(PS::kStripH));

        // ── XOuija Panel — left column ───────────────────────────────────────
        int ouijaW = std::clamp(PS::kXOuijaW,
                                XOuijaPanel::kMinWidth,
                                XOuijaPanel::kMaxWidth);
        xouijaPanel_.setBounds(bounds.removeFromLeft(ouijaW));

        // ── Note area — remaining space (NoteInputZone or KeysMode) ─────────
        auto noteArea = bounds;

        bool showKeys = (noteInput.getMode() == NoteInputZone::Mode::Keys);
        noteInput.setVisible(!showKeys);
        keysMode_.setVisible(showKeys);

        if (showKeys)
            keysMode_.setBounds(noteArea);
        else
            noteInput.setBounds(noteArea);
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colour(PS::kSurfaceBg));
    }

    void visibilityChanged() override
    {
        if (isVisible())
            startTimerHz(30);
        else
            stopTimer();
    }

    void focusLost(FocusChangeType) override
    {
        // Nothing to release (V1 perfPads removed in V2 layout)
    }

    bool keyPressed(const juce::KeyPress& /*key*/) override
    {
        // V1 perfPads removed — XOuija and gesture buttons have their own key handling
        return false;
    }

    bool keyStateChanged(bool /*isKeyDown*/) override
    {
        return false;
    }

private:
    void timerCallback() override
    {
        noteInput.tick();
        strip.tick();
    }

    // Wire xouijaPanel_.onCCOutput → processor_->pushCCOutput().
    // Called once by setProcessor() and once in the constructor (no-op until
    // the processor is set, but the lambda captures processor_ by pointer so
    // the live value is used at call time).
    void wireOnCCOutput()
    {
        xouijaPanel_.onCCOutput = [this](uint8_t cc, uint8_t value)
        {
            if (processor_)
                processor_->pushCCOutput(0, cc, value); // channel 0 = MIDI ch 1
        };
    }

    juce::Colour accentColour { 0xFFE9C46A }; // Default: XO Gold

    // ── Task 12: processor pointer for CC output forwarding ──────────────────
    XOlokunProcessor* processor_       = nullptr;
    bool              driftToggleState_ = false;  // CC 90 state — used by Task 13

    NoteInputZone      noteInput;

    // V2 layout components
    PerformanceStrip   strip;
    XOuijaPanel        xouijaPanel_;
    KeysMode           keysMode_;

    std::array<juce::TextButton, 3> modeButtons;
    std::array<juce::TextButton, 4> stripModeButtons;
    std::array<juce::TextButton, 4> bankButtons;  // A / B / C / D bank selectors
    juce::TextButton octDownBtn, octUpBtn;
    juce::Label      octLabel;
    juce::TextButton scaleModeBtn;  // Cycles: SCL (Off) → FLT (Filter) → HLT (Highlight)

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PlaySurface)
};

//==============================================================================
// PlaySurfaceWindow — floating popup window containing the PlaySurface.
//
// Default size: 520×520px (square, so the 4x4 pad grid renders as square pads).
// Resizable with setResizeLimits(); aspect ratio is NOT locked by default so the
// user can expand horizontally to reveal more OrbitPath space, but the pad grid
// itself always self-centers as squares.
//
// Usage in XOlokunEditor:
//   std::unique_ptr<PlaySurfaceWindow> playSurfaceWindow;
//
//   // Create lazily on first toggle press:
//   if (!playSurfaceWindow)
//   {
//       playSurfaceWindow = std::make_unique<PlaySurfaceWindow>();
//       playSurfaceWindow->getPlaySurface().setMidiCollector(
//           &processor.getMidiCollector(), 1);
//   }
//   playSurfaceWindow->setVisible(true);
//   playSurfaceWindow->toFront(true);
//
class PlaySurfaceWindow : public juce::DocumentWindow
{
public:
    //----------------------------------------------------------------------
    // V2 popup dimensions: 700×484 (narrower, non-square — XOuija panel + pad grid)
    // Was 520×520 in V1.
    static constexpr int kDefaultW  = PS::kDesktopW; // 700
    static constexpr int kDefaultH  = PS::kDesktopH; // 484
    static constexpr int kMinW      = 500;
    static constexpr int kMinH      = 400;
    static constexpr int kMinSize   = 500;  // kept for backward compat
    static constexpr int kMaxW      = 1200;
    static constexpr int kMaxH      = 900;

    // Optional callback fired when the window is closed/hidden by the user.
    // XOlokunEditor uses this to sync the "PS" toggle button state.
    std::function<void()> onClosed;

    PlaySurfaceWindow()
        : juce::DocumentWindow (
              "PlaySurface",
              juce::Colour (PS::kSurfaceBg),
              juce::DocumentWindow::allButtons,
              true /* addToDesktop */)
    {
        setUsingNativeTitleBar (true);
        setResizable (true, true);
        setResizeLimits (kMinW, kMinH, kMaxW, kMaxH); // Was 320-1200 square in V1

        // Own the PlaySurface content component.
        // resizeToFitContent=false: we size the window explicitly via centreWithSize().
        auto* content = new PlaySurface();
        setContentOwned (content, true);

        // Position initially centered on the main screen.
        // V2 default: 700×484 (was 520×520 in V1)
        centreWithSize (kDefaultW, kDefaultH);

        // Performance tool window — keep it visible above the DAW.
        setAlwaysOnTop (true);
    }

    ~PlaySurfaceWindow() override = default;

    //----------------------------------------------------------------------
    // Grab keyboard focus when the window becomes visible so keyboard
    // shortcuts (Z/X/C/V pads, octave keys) work immediately on open.
    void visibilityChanged() override
    {
        if (isVisible())
            getPlaySurface().grabKeyboardFocus();
    }

    //----------------------------------------------------------------------
    // Returns a reference to the embedded PlaySurface for wiring MIDI etc.
    PlaySurface& getPlaySurface()
    {
        return *static_cast<PlaySurface*>(getContentComponent());
    }

    //----------------------------------------------------------------------
    // Closing hides the window rather than deleting it so MIDI state,
    // mode selections, and bank selections survive hide/show cycles.
    void closeButtonPressed() override
    {
        setVisible (false);
        if (onClosed) onClosed();
    }

    //----------------------------------------------------------------------
    // Escape key closes the popup (hides it).
    bool keyPressed (const juce::KeyPress& key) override
    {
        if (key == juce::KeyPress::escapeKey)
        {
            setVisible (false);
            if (onClosed) onClosed();
            return true;
        }
        return juce::DocumentWindow::keyPressed (key);
    }

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PlaySurfaceWindow)
};

} // namespace xolokun
