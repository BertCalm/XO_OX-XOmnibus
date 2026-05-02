// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include <array>
#include <vector>
#include <cmath>
#include <atomic>
#include "KeysMode.h"
#include "HarmonicField.h"
#include "TideController.h"

namespace xoceanus
{
class XOceanusProcessor;
}
#include "../../XOceanusProcessor.h"

#include "GestureTrailBuffer.h"
#include "../GalleryColors.h"

namespace xoceanus
{

//==============================================================================
// Helper: lighten a colour toward white by [amount] (0=no change, 1=white)
static inline juce::Colour lightenColour(juce::Colour c, float amount)
{
    return c.interpolatedWith(juce::Colours::white, amount);
}

//==============================================================================
// PlaySurface Constants
namespace PS
{
// V2 layout dimensions
static constexpr int kDesktopW = 700;  // Narrower
static constexpr int kDesktopH = 484;  // 420 (main) + 64 (strip)
static constexpr int kMainZoneH = 420; // Main content height
static constexpr int kStripH = 64;     // Performance strip (matches kZone3H)
static constexpr int kHeaderH = 28;    // Mode tab bar row height (one row)
static constexpr int kHeaderH2 = kHeaderH * 2; // Two-row header total height (D2)

// V1 layout constants (kept for backward compatibility with any external references)
static constexpr int kZone1W = 480;
static constexpr int kZone2W = 200;
static constexpr int kZone4W = 100;
static constexpr int kZone3H = 64; // same as kStripH

// Pad grid constants (unchanged)
static constexpr int kPadCols = 4;
static constexpr int kPadRows = 4;
static constexpr int kNumPads = kPadCols * kPadRows;
static constexpr float kPadGap = 4.0f; // pixel gap between adjacent pads

// Animation
static constexpr float kVelDecay = 0.92f;
static constexpr float kWarmMemoryDur = 1.5f; // seconds
static constexpr int kStripTrailSize = 45;

// Colors — performance accent tones (theme-independent)
static constexpr uint32_t kAmber = 0xFFF5C97A;
static constexpr uint32_t kTerracotta = 0xFFE07A5F;
static constexpr uint32_t kTeal = 0xFF2A9D8F;
static constexpr uint32_t kFireGreen = 0xFF4ADE80;
static constexpr uint32_t kPanicRed = 0xFFEF4444;

// Surface and text tones — route through GalleryColors so light mode works.
// Call these functions instead of using the old constexpr values directly.
static inline juce::Colour surfaceBg()
{
    return GalleryColors::get(GalleryColors::surface());
}
static inline juce::Colour surfaceCard()
{
    return GalleryColors::get(GalleryColors::elevated());
}
static inline juce::Colour textLight()
{
    return GalleryColors::get(GalleryColors::t1());
}
static inline juce::Colour textDim()
{
    return GalleryColors::get(GalleryColors::t2());
}

// MIDI
static constexpr int kBaseNote = 48; // C3
static constexpr int kMinOctave = -3;
static constexpr int kMaxOctave = 3;
} // namespace PS

//==============================================================================
// Zone 1: Note Input — 4x4 velocity-sensitive pad grid
//
class NoteInputZone : public juce::Component
{
public:
    enum class Mode
    {
        Pad,
        Fretless,
        Drum,
        Keys
    };

    //----------------------------------------------------------------------
    // P0-1: MIDI pipeline wiring.
    // Set this pointer (owned externally, e.g. by XOceanusProcessor) before
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
    juce::Colour accentColour{0xFFE9C46A};
    void setAccentColour(juce::Colour c)
    {
        accentColour = c;
        repaint();
    }

    // Harmonic field state — kept for future use (see issue #1174).
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
    enum class Bank
    {
        A = 0,
        B = 1,
        C = 2,
        D = 3
    };
    void setBank(Bank b)
    {
        currentBank = b;
        scaleNotesDirty_ = true;
        repaint();
    }
    Bank getBank() const { return currentBank; }

    NoteInputZone()
    {
        // Scale options
        scales = {
            {"Chromatic", {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11}},
            {"Major", {0, 2, 4, 5, 7, 9, 11}},
            {"Minor", {0, 2, 3, 5, 7, 8, 10}},
            {"Dorian", {0, 2, 3, 5, 7, 9, 10}},
            {"Mixolydian", {0, 2, 4, 5, 7, 9, 10}},
            {"Pent Minor", {0, 3, 5, 7, 10}},
            {"Pent Major", {0, 2, 4, 7, 9}},
            {"Blues", {0, 3, 5, 6, 7, 10}},
            {"Harm Minor", {0, 2, 3, 5, 7, 8, 11}},
        };
        currentScale = 0; // Chromatic
        rootKey = 0;      // C
        octaveOffset = 0;
    }

    enum class ScaleMode
    {
        Off,
        Filter,
        Highlight
    };
    ScaleMode scaleMode = ScaleMode::Off;
    // P2-2: setter so callers don't access scaleMode directly
    void setScaleMode(ScaleMode m)
    {
        scaleMode = m;
        scaleNotesDirty_ = true;
        repaint();
    }

    void setMode(Mode m)
    {
        mode = m;
        repaint();
    }
    Mode getMode() const { return mode; }
    void setOctave(int oct)
    {
        octaveOffset = juce::jlimit(PS::kMinOctave, PS::kMaxOctave, oct);
        scaleNotesDirty_ = true;
        repaint();
    }
    int getOctave() const { return octaveOffset; }
    void setScale(int idx)
    {
        currentScale = juce::jlimit(0, (int)scales.size() - 1, idx);
        scaleNotesDirty_ = true;
        repaint();
    }
    void setRootKey(int key)
    {
        rootKey = juce::jlimit(0, 11, key);
        scaleNotesDirty_ = true;
        repaint();
    }

    void paint(juce::Graphics& g) override
    {
        using namespace PS;
        auto b = getLocalBounds().toFloat();

        // Radial gradient background: accent @ 0.04 center → surfaceBg edge
        {
            float cx = b.getCentreX(), cy = b.getCentreY();
            float r = juce::jmax(b.getWidth(), b.getHeight()) * 0.7f;
            juce::ColourGradient bg(accentColour.withAlpha(0.04f), cx, cy, PS::surfaceBg(), cx + r, cy, true);
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
            if (v > 0.01f)
            {
                v *= PS::kVelDecay;
                needsRepaint = true;
            }
            else
                v = 0.0f;
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
        if (needsRepaint)
            repaint();
    }

private:
    // ------------------------------------------------------------------
    // P0-1 helpers: route note events to MidiMessageCollector when wired,
    // otherwise fall back to the legacy std::function callbacks.
    void fireNoteOn(int note, float velocity)
    {
        if (midiCollector)
        {
            auto msg = juce::MidiMessage::noteOn(midiChannel, note, static_cast<float>(velocity));
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

    struct ScaleDef
    {
        const char* name;
        std::vector<int> intervals;
    };
    struct WarmMemoryEntry
    {
        int pad = -1;
        float age = 99.0f;
    };

    Mode mode = Mode::Pad;
    Bank currentBank = Bank::A;
    int octaveOffset = 0;
    int currentScale = 0;
    int rootKey = 0;
    int lastNote = -1;
    int lastPad = -1;                    // pad index of the currently sounding note (for aftertouch geometry)
    float lastFretlessVelocity_ = 0.75f; // updated on each fretless touch; drives ring glow
    std::vector<ScaleDef> scales;
    std::array<float, PS::kNumPads> padVelocity{};
    std::array<WarmMemoryEntry, 8> warmMemory{};
    int warmMemIdx = 0;

    // Fix #35: scale note cache — 16 pre-computed notes for FILTER mode.
    // Marked mutable so const helpers can rebuild on demand.
    mutable std::array<int, PS::kNumPads> cachedScaleNotes_{};
    mutable bool scaleNotesDirty_ = true;

    // Harmonic field state (see issue #1174)
    int harmonicRootKey_ = 0; // harmonic root key (semitone 0-11)
    int harmonicTension_ = 0; // fifths distance from C for color temperature (0-6)

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
                37, 36, 42, 82, // row 0 (bottom): Kick A, Kick B, Snare A, Snare B
                40, 38, 46, 44, // row 1: Clap, Hat Closed, Hat Open, Ride
                48, 47, 45, 43, // row 2: Tom 1, Perc A, Perc B, Crash
                49, 55, 57, 51, // row 3 (top): Fx 1, Fx 2, Fx 3, Fx 4
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
        static constexpr int kBankBase[4] = {36, 52, 68, 84};
        int row = pad / PS::kPadCols;
        int col = pad % PS::kPadCols;
        int rawNote = kBankBase[(int)currentBank] + (octaveOffset * 12) + (row * PS::kPadCols) + col;
        return quantizeToScale(juce::jlimit(0, 127, rawNote));
    }

    int quantizeToScale(int note) const
    {
        if (currentScale == 0)
            return note; // Chromatic
        auto& intervals = scales[(size_t)currentScale].intervals;
        int best = note;
        int bestDist = 999;
        for (int octSearch = -1; octSearch <= 1; ++octSearch)
        {
            for (int interval : intervals)
            {
                int candidate = rootKey + interval + ((note / 12) + octSearch) * 12;
                if (candidate < 0)
                    continue; // P1-3: skip negative candidates
                int dist = std::abs(candidate - note);
                if (dist < bestDist)
                {
                    bestDist = dist;
                    best = candidate;
                }
            }
        }
        return juce::jlimit(0, 127, best);
    }

    // Returns true if 'note' is in the current scale (relative to rootKey).
    // Chromatic scale (index 0) always returns true.
    bool isNoteInScale(int note) const
    {
        if (currentScale == 0)
            return true;
        auto& intervals = scales[(size_t)currentScale].intervals;
        int relativeToRoot = ((note % 12) - rootKey + 12) % 12;
        for (auto interval : intervals)
            if (interval == relativeToRoot)
                return true;
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
        static constexpr int kBankBase[4] = {36, 52, 68, 84};
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
                if (interval == relativeToRoot)
                {
                    inScale = true;
                    break;
                }
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

        // Square grid: match the same geometry used in paintPadGrid().
        // Account for inter-pad gaps when computing the maximum square pad size.
        const float gap = PS::kPadGap;
        float availW = b.getWidth() - (PS::kPadCols - 1) * gap;
        float availH = b.getHeight() - (PS::kPadRows - 1) * gap;
        float padSize = juce::jmin(availW / PS::kPadCols, availH / PS::kPadRows);
        float gridW = padSize * PS::kPadCols + gap * (PS::kPadCols - 1);
        float gridH = padSize * PS::kPadRows + gap * (PS::kPadRows - 1);
        float originX = b.getX() + (b.getWidth() - gridW) * 0.5f;
        float originY = b.getY() + (b.getHeight() - gridH) * 0.5f;
        float stride = padSize + gap; // distance from one pad origin to the next

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
                lastPad = -1;
            }
            return;
        }

        int col = (int)(lx / stride);
        int row = PS::kPadRows - 1 - (int)(ly / stride);
        col = juce::jlimit(0, PS::kPadCols - 1, col);
        row = juce::jlimit(0, PS::kPadRows - 1, row);
        int pad = row * PS::kPadCols + col;

        // Velocity: Y position within the specific pad cell (top of pad = hard, bottom = soft).
        int displayRow = PS::kPadRows - 1 - row;
        float padTopY = displayRow * stride;
        float yInPad = juce::jlimit(0.0f, 1.0f, (ly - padTopY) / padSize);
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
            warmMemory[(size_t)warmMemIdx] = {pad, 0.0f};
            warmMemIdx = (warmMemIdx + 1) % (int)warmMemory.size();
            lastNote = note;
            lastPad = pad;
            fireNoteOn(note, velocity);
        }
        else if (!isDown && pad == lastPad && lastNote >= 0)
        {
            // Drag within the same pad: send aftertouch.
            // Aftertouch pressure = Y position within the pad cell.
            // top of cell = maximum pressure (1.0), bottom = minimum (0.0).
            int displayRow2 = PS::kPadRows - 1 - row; // flip: row 0 is rendered at top
            float padTopY2 = displayRow2 * stride;
            float yInPad2 = juce::jlimit(0.0f, 1.0f, (ly - padTopY2) / padSize);
            // Top of pad → pressure 1.0; bottom → 0.0
            float pressure = juce::jlimit(0.0f, 1.0f, 1.0f - yInPad2);
            fireAftertouch(lastNote, pressure);
        }
    }

    void handleFretlessTouch(const juce::MouseEvent& e, bool isDown)
    {
        auto b = getLocalBounds();

        // P0-2: X=pitch (left=low, right=high), Y=expression (bottom=dark,top=bright)
        float xNorm = juce::jlimit(0.0f, 1.0f, (float)e.x / b.getWidth());
        float yNorm = juce::jlimit(0.0f, 1.0f,
                                   1.0f - (float)e.y / b.getHeight()); // bottom=0, top=1

        // Pitch from X: C1 (MIDI 24) → C7 (MIDI 96), 6-octave range
        float pitchF = 24.0f + xNorm * 72.0f;
        int note = juce::jlimit(24, 96, quantizeToScale((int)pitchF));

        // Expression from Y: bottom=softest, top=brightest.  Used as the
        // initial velocity on touch-down AND as an ongoing expression signal.
        float expression = juce::jmap(yNorm, 0.0f, 1.0f, 0.35f, 1.0f);
        expression = juce::jlimit(0.35f, 1.0f, expression);
        lastFretlessVelocity_ = expression;

        if (isDown)
        {
            // Touch began: fire note-on at the initial Y-derived velocity.
            if (lastNote >= 0)
                fireNoteOff(lastNote);
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
                float bendNorm = juce::jlimit(-1.0f, 1.0f, bendSemitones / kBendSemitones);
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

        // Enforce perfectly square pads (aspect-ratio: 1).
        // Subtract inter-pad gaps from available space before computing cell size.
        const float gap = kPadGap;
        float availW = b.getWidth() - (kPadCols - 1) * gap;
        float availH = b.getHeight() - (kPadRows - 1) * gap;
        float padSize = juce::jmin(availW / kPadCols, availH / kPadRows);
        float gridW = padSize * kPadCols + gap * (kPadCols - 1);
        float gridH = padSize * kPadRows + gap * (kPadRows - 1);
        // Center the grid within the available component bounds
        float originX = b.getX() + (b.getWidth() - gridW) * 0.5f;
        float originY = b.getY() + (b.getHeight() - gridH) * 0.5f;
        float stride = padSize + gap; // distance from one pad origin to the next

        for (int row = 0; row < kPadRows; ++row)
        {
            for (int col = 0; col < kPadCols; ++col)
            {
                int pad = row * kPadCols + col;
                int displayRow = kPadRows - 1 - row; // flip for bottom-left origin
                float x = originX + col * stride;
                float y = originY + displayRow * stride;
                auto padRect = juce::Rectangle<float>(x, y, padSize, padSize).reduced(1.0f);

                float vel = padVelocity[(size_t)pad];
                bool isHit = (vel > 0.01f);

                if (isHit)
                {
                    // P1-4: Draw glow shadow FIRST so subsequent pads paint over any bleed
                    g.setColour(accentColour.withAlpha(vel * 0.25f));
                    g.fillRoundedRectangle(padRect.expanded(3.0f), 5.0f);

                    // Hit pad: radial gradient from accent @ 0.42 center to accent @ 0.07 edge
                    float cx = padRect.getCentreX(), cy = padRect.getCentreY();
                    float r = padRect.getWidth() * 0.7f;
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
                    bool dimPad = (scaleMode == ScaleMode::Highlight && mode != Mode::Drum && !inScale);

                    float borderAlpha = dimPad ? 0.08f : 0.18f;

                    // Non-hit pad: accent @ 0.07 fill
                    g.setColour(accentColour.withAlpha(0.07f));
                    g.fillRoundedRectangle(padRect, 4.0f);

                    // Border: dimmed or normal
                    g.setColour(accentColour.withAlpha(borderAlpha));
                    g.drawRoundedRectangle(padRect, 4.0f, 1.0f);

                    // Root key accent: XO Gold 2px bottom border
                    // P2-3: In HIGHLIGHT mode use quantized note for root detection
                    int rootCheckNote =
                        (scaleMode == ScaleMode::Highlight && mode != Mode::Drum) ? quantizeToScale(padNote) : padNote;
                    bool isRootPad =
                        (scaleMode != ScaleMode::Off && mode != Mode::Drum && (rootCheckNote % 12) == rootKey);
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

                // Harmonic field coloring — additive overlay, Pad mode only.
                if (mode == Mode::Pad)
                {
                    int midiNote = midiNoteForPad(pad);
                    bool inKey = HarmonicField::isInKey(midiNote, harmonicRootKey_);
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
                        // In-key: glow with tension color — raised 0.15 → 0.22 (UIX Fix 3)
                        g.setColour(tensionColour.withAlpha(0.22f));
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
                static const char* noteNames[] = {"C", "Db", "D", "Eb", "E", "F", "Gb", "G", "Ab", "A", "Bb", "B"};
                juce::String label;
                if (mode == Mode::Drum)
                {
                    // P0-4: drum labels match MPC Bank A standard note table
                    static const char* drumNames[] = {
                        "Kick A", "Kick B", "Snr A",  "Snr B", // row 0 (bottom)
                        "Clap",   "HH-C",   "HH-O",   "Ride",  // row 1
                        "Tom 1",  "Perc A", "Perc B", "Crash", // row 2
                        "Fx 1",   "Fx 2",   "Fx 3",   "Fx 4",  // row 3 (top)
                    };
                    label = drumNames[pad];
                }
                else
                {
                    // P1-1: In HIGHLIGHT mode the fired note is quantized, so show
                    // the quantized note as the label so the display matches what plays.
                    int displayNote = (scaleMode == ScaleMode::Highlight) ? quantizeToScale(note) : note;
                    // Note label cache: 128 slots, built once on first use.
                    static juce::String kNoteLabels[128];
                    static bool kNoteLabelsBuilt = false;
                    if (!kNoteLabelsBuilt)
                    {
                        for (int n = 0; n < 128; ++n)
                            kNoteLabels[n] = juce::String(noteNames[n % 12]) + juce::String(n / 12 - 1);
                        kNoteLabelsBuilt = true;
                    }
                    int safeNote = juce::jlimit(0, 127, displayNote);
                    label = kNoteLabels[safeNote];
                }

                // Note label: white on hit; dimmed (0.25) for out-of-scale pads in Highlight mode; accent @ 0.55 otherwise
                float labelAlpha = 0.55f;
                if (!isHit && scaleMode == ScaleMode::Highlight && mode != Mode::Drum && !isNoteInScale(note))
                    labelAlpha = 0.25f;
                g.setColour(isHit ? juce::Colours::white : accentColour.withAlpha(labelAlpha));
                g.setFont(GalleryFonts::body(10.0f)); // (#885: 9pt→10pt legibility floor)
                g.drawText(label, padRect, juce::Justification::centred);
            }
        }

        // Bank badge — top-right corner, accent-tinted
        {
            static const juce::String kBankBadges[] = {"BNK A", "BNK B", "BNK C", "BNK D"};
            const auto& badge = kBankBadges[juce::jlimit(0, 3, (int)currentBank)];
            auto badgeRect = juce::Rectangle<float>(originX + gridW - 50.0f, originY, 48.0f, 16.0f);
            g.setColour(accentColour.withAlpha(0.75f));
            g.setFont(GalleryFonts::display(10.0f)); // (#885: 9pt→10pt legibility floor)
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
            juce::ColourGradient grad(juce::Colour(0xFF150820), b.getX(), b.getBottom(), // Midnight base
                                      juce::Colour(0xFF0A1F30), b.getX(), b.getY(), false);
            grad.addColour(0.20, juce::Colour(0xFF7B2FBE).withAlpha(0.35f)); // Midnight zone
            grad.addColour(0.45, juce::Colour(0xFF0D2744));                  // Twilight zone base
            grad.addColour(0.72, juce::Colour(0xFF0096C7).withAlpha(0.5f));  // Twilight zone top
            grad.addColour(0.85, juce::Colour(0xFF0D3550));                  // Sunlit base
            g.setGradientFill(grad);
            g.fillRect(b);
        }

        // Zone boundary lines — 0.30 alpha: more prominent than octave fret lines (0.35) to read as
        // structural zone dividers rather than just another fret line.
        {
            float sunlitY = b.getBottom() - b.getHeight() * 0.45f;   // 45% up = twilight boundary
            float twilightY = b.getBottom() - b.getHeight() * 0.80f; // 80% up = sunlit boundary
            g.setColour(juce::Colour(0xFF0096C7).withAlpha(0.30f));
            g.drawHorizontalLine((int)sunlitY, b.getX(), b.getRight());
            g.setColour(juce::Colour(0xFF48CAE4).withAlpha(0.30f));
            g.drawHorizontalLine((int)twilightY, b.getX(), b.getRight());
        }

        // ── Zone-colored fret lines ──────────────────────────────────────────
        auto& intervals = scales[(size_t)currentScale].intervals;
        int totalSemitones = 72; // C1-C7
        for (int semi = 0; semi <= totalSemitones; ++semi)
        {
            float y = b.getBottom() - (float)semi / totalSemitones * b.getHeight();
            float normY = 1.0f - (float)semi / totalSemitones; // 0=top/sunlit, 1=bottom/midnight

            // Choose zone color for this position
            juce::Colour zoneColor;
            if (normY < 0.20f)
                zoneColor = juce::Colour(0xFF48CAE4); // Sunlit — cyan
            else if (normY < 0.55f)
                zoneColor = juce::Colour(0xFF0096C7); // Twilight — blue
            else
                zoneColor = juce::Colour(0xFF7B2FBE); // Midnight — violet

            bool isOctave = (semi % 12 == 0);
            bool isScaleNote =
                (currentScale == 0) || std::find(intervals.begin(), intervals.end(), semi % 12) != intervals.end();

            if (isOctave)
            {
                g.setColour(zoneColor.withAlpha(0.35f));
                g.drawHorizontalLine((int)y, b.getX(), b.getRight());
                int octNum = (24 + semi) / 12 - 1;
                g.setFont(GalleryFonts::body(10.0f)); // (#885: 8pt→10pt legibility floor)
                g.setColour(zoneColor.withAlpha(0.55f));
                g.drawText("C" + juce::String(octNum), juce::Rectangle<float>(4, y - 10, 26, 12),
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
            if (normY < 0.20f)
                zoneColor = juce::Colour(0xFF48CAE4);
            else if (normY < 0.55f)
                zoneColor = juce::Colour(0xFF0096C7);
            else
                zoneColor = juce::Colour(0xFF7B2FBE);

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
                const float startDeg = 270.0f - arcSweepDeg * 0.5f; // centred at top (270° = 12 o'clock)
                juce::Path arcPath;
                arcPath.addArc(cx - ringR, noteY - ringR, ringR * 2, ringR * 2, juce::degreesToRadians(startDeg),
                               juce::degreesToRadians(startDeg + arcSweepDeg), true);
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
    PerformanceStrip()
        // Cache font once — avoids per-paint Font construction (UIX Fix 1C)
        : axisFont_(juce::Font(juce::FontOptions{}.withHeight(8.0f)))
    {
    }
    enum class StripMode
    {
        DubSpace,
        FilterSweep,
        Coupling,
        DubSiren
    };

    std::function<void(float x, float y)> onPositionChanged;

    void setStripMode(StripMode m)
    {
        stripMode = m;
        repaint();
    }

    // Engine accent colour — set by PlaySurface::setAccentColour()
    juce::Colour accentColour{0xFFE9C46A};
    void setAccentColour(juce::Colour c)
    {
        accentColour = c;
        repaint();
    }

    void paint(juce::Graphics& g) override
    {
        using namespace PS;
        auto b = getLocalBounds().toFloat();

        // Background: linear gradient accent @ 0.08 top → accent @ 0.03 bottom
        {
            juce::ColourGradient bg(accentColour.withAlpha(0.08f), b.getX(), b.getY(), accentColour.withAlpha(0.03f),
                                    b.getX(), b.getBottom(), false);
            g.setGradientFill(bg);
            g.fillRect(b);
        }

        // Mode-specific tint overlay (preserves per-mode colour identity)
        {
            static constexpr uint32_t kStripModeTints[] = {
                0xFF2D4D5A, // DubSpace  — dark teal
                0xFF00A6D6, // FilterSweep — teal
                0xFFE9C46A, // Coupling — XO Gold
                0xFFFF6B6B, // DubSiren — warm red
            };
            g.setColour(juce::Colour(kStripModeTints[(int)stripMode]).withAlpha(0.08f));
            g.fillRect(b);
        }

        // Full strip is the gestural area — mode tabs moved to PlaySurface header (UIX ruling)
        auto gestureArea = b;

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
            if (pt.age > kWarmMemoryDur)
                continue;
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
        g.setFont(axisFont_); // cached — avoids per-paint Font construction (UIX Fix 1C)
        g.setColour(juce::Colours::white.withAlpha(0.35f));
        static const char* xLabels[] = {"DELAY FB", "CUTOFF", "SPREAD", "PITCH"};
        static const char* yLabels[] = {"REVERB", "RESONANCE", "DEPTH", "SIREN DEPTH"};
        g.drawText(xLabels[(int)stripMode], gestureArea.reduced(4), juce::Justification::bottomLeft);
        g.drawText(yLabels[(int)stripMode], gestureArea.reduced(4), juce::Justification::topRight);
    }

    void mouseDown(const juce::MouseEvent& e) override
    {
        updateStrip(e);
        touching = true;
    }
    void mouseDrag(const juce::MouseEvent& e) override
    {
        if (touching)
            updateStrip(e);
    }
    void mouseUp(const juce::MouseEvent&) override
    {
        touching = false;
        // Record release position as spring-back start; reset elapsed to begin 250ms ease-out
        springStartX_ = stripX;
        springStartY_ = stripY;
        springElapsed_ = 0.0f;
    }

    void tick()
    {
        float prevX = stripX, prevY = stripY;

        if (!touching)
        {
            // Spring-back: 250ms quadratic ease-out (Spec Section 9.5)
            // kSpringDuration = 0.250s; timer runs at 30fps → each tick = 1/30s
            constexpr float kSpringDuration = 0.250f; // seconds
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
            if (pt.age < PS::kWarmMemoryDur)
                hasActiveTrail = true;
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
    bool touching = false;

    // Cached font — initialized in constructor, avoids per-paint construction (UIX Fix 1C)
    juce::Font axisFont_; // 8px — gesture area axis labels

    // Spring-back animation state (250ms ease-out)
    float springElapsed_ = 999.0f; // Start in "done" state
    float springStartX_ = 0.3f;
    float springStartY_ = 0.2f;

    struct SpringTarget
    {
        float x, y;
    };
    static constexpr SpringTarget springTargets[] = {
        {0.3f, 0.2f},  // DubSpace
        {0.3f, 0.3f},  // FilterSweep (cutoff ~1200Hz log)
        {0.3f, 0.15f}, // Coupling
        {0.5f, 0.5f},  // DubSiren (center)
    };

    struct TrailPoint
    {
        float x = 0.0f, y = 0.0f;
        float age = 99.0f;
    };
    std::array<TrailPoint, PS::kStripTrailSize> stripTrail{};
    int stripTrailHead = 0;

    void updateStrip(const juce::MouseEvent& e)
    {
        // Full strip is the gestural area — mode tabs are in the PlaySurface header
        auto b = getLocalBounds().toFloat();
        auto gestureArea = b;
        float gx = juce::jlimit(gestureArea.getX(), gestureArea.getRight(), static_cast<float>(e.x));
        stripX = juce::jlimit(0.0f, 1.0f, (gx - gestureArea.getX()) / gestureArea.getWidth());
        stripY = juce::jlimit(0.0f, 1.0f, 1.0f - (e.y - b.getY()) / b.getHeight());

        stripTrail[(size_t)stripTrailHead] = {stripX, stripY, 0.0f};
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
        setTitle("PlaySurface");
        setDescription("V2 performance interface: note input / keys mode, performance strip");
        setWantsKeyboardFocus(true);

        noteInput.setTitle("Note Input Zone");
        noteInput.setDescription("Pad grid or fretless strip for note input");
        strip.setTitle("Performance Strip");
        strip.setDescription("Touch strip for filter sweeps, coupling, and dub effects");

        addAndMakeVisible(noteInput);
        addAndMakeVisible(strip);

        // V2: KeysMode
        addAndMakeVisible(keysMode_);
        A11y::setup(keysMode_, "Keys Mode", "Piano-style keyboard input surface");
        keysMode_.setVisible(false); // Initially hidden — shown when mode == Keys

        // Wire KeysMode MIDI collector (same as NoteInputZone's)
        keysMode_.midiCollector = noteInput.midiCollector;

        // ── D4 (Wave 6): Top-level mode tabs — KEYS | PADS | XY ──────────────
        // PAD and DRUM are no longer separate top-level tabs.  They are unified
        // under PADS; a sub-toggle (♪=scale-aware / ▦=drum-kit) selects the
        // note-label style inside the pad grid.
        // XY gives the performance strip the full play area.
        for (int i = 0; i < kNumModeTabs; ++i)
        {
            modeButtons[i].setClickingTogglesState(true);
            modeButtons[i].setRadioGroupId(101);
            addAndMakeVisible(modeButtons[i]);
        }
        static const char* kModeTabLabels[kNumModeTabs] = { "KEYS", "PADS", "XY" };
        for (int i = 0; i < kNumModeTabs; ++i)
            modeButtons[i].setButtonText(kModeTabLabels[i]);
        modeButtons[0].setToggleState(true, juce::dontSendNotification); // default = KEYS
        A11y::setup(modeButtons[0], "Keys Mode",  "Switch to piano-keyboard play surface");
        A11y::setup(modeButtons[1], "Pads Mode",  "Switch to velocity-sensitive pad grid (scale-aware or drum-kit sub-mode)");
        A11y::setup(modeButtons[2], "XY Mode",    "Switch to full-screen XY performance strip");

        // D4 tab callbacks — each tab updates surfaceTab_ and calls resized()
        // D3 (1D-P2B): latchBadge_ no longer shown in header; fire onLatchStateChanged
        //              so the status bar (TransportBar) can update its LATCH indicator.
        modeButtons[0].onClick = [this]()
        {
            surfaceTab_ = SurfaceTab::Keys;
            noteInput.setMode(NoteInputZone::Mode::Keys);
            if (onLatchStateChanged) onLatchStateChanged(true);
            resized();
            keysMode_.grabKeyboardFocus();
        };
        modeButtons[1].onClick = [this]()
        {
            surfaceTab_ = SurfaceTab::Pads;
            // Sub-mode drives NoteInputZone::Mode (Pad or Drum)
            applyPadsSubMode();
            if (onLatchStateChanged) onLatchStateChanged(false);
            resized();
            noteInput.grabKeyboardFocus();
        };
        modeButtons[2].onClick = [this]()
        {
            surfaceTab_ = SurfaceTab::XY;
            if (onLatchStateChanged) onLatchStateChanged(false);
            resized();
        };

        // ── D4: PADS sub-mode toggle ♪ / ▦ ────────────────────────────────────
        // ♪ = scale-aware pad labels (was "PAD" mode)
        // ▦ = drum-kit pad labels (was "DRUM" mode)
        // Visible only when PADS tab is active.
        padsSubModeBtn_.setClickingTogglesState(true);
        padsSubModeBtn_.setButtonText(kPadsSubModeLabels[0]); // default = NOTE
        padsSubModeBtn_.setToggleState(false, juce::dontSendNotification);
        addAndMakeVisible(padsSubModeBtn_);
        A11y::setup(padsSubModeBtn_,
                    "Pads Sub-mode Toggle",
                    "Toggle between scale-aware note pads (musical mode) and drum-kit labelled pads (percussion mode)");
        padsSubModeBtn_.onClick = [this]()
        {
            drumSubMode_ = padsSubModeBtn_.getToggleState();
            padsSubModeBtn_.setButtonText(kPadsSubModeLabels[drumSubMode_ ? 1 : 0]);
            applyPadsSubMode();
        };

        // Octave controls
        octDownBtn.setButtonText("-");
        octUpBtn.setButtonText("+");
        octLabel.setText("OCT 0", juce::dontSendNotification);
        octLabel.setJustificationType(juce::Justification::centred);
        octLabel.setColour(juce::Label::textColourId, PS::textLight());
        addAndMakeVisible(octDownBtn);
        A11y::setup(octDownBtn, "Octave Down", "Shift note range one octave lower");
        addAndMakeVisible(octUpBtn);
        A11y::setup(octUpBtn, "Octave Up", "Shift note range one octave higher");
        addAndMakeVisible(octLabel);
        A11y::setup(octLabel, "Octave Display", "Current octave offset", false);

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
        static const char* bankLabels[] = {"A", "B", "C", "D"};
        for (int i = 0; i < 4; ++i)
        {
            bankButtons[i].setClickingTogglesState(true);
            bankButtons[i].setRadioGroupId(103);
            bankButtons[i].setButtonText(bankLabels[i]);
            addAndMakeVisible(bankButtons[i]);
            A11y::setup(bankButtons[i], juce::String("Bank ") + bankLabels[i],
                        juce::String("Select note bank ") + bankLabels[i] + " (16-note offset)");
        }
        bankButtons[0].setToggleState(true, juce::dontSendNotification);

        for (int i = 0; i < 4; ++i)
        {
            bankButtons[i].onClick = [this, i] { noteInput.setBank(static_cast<NoteInputZone::Bank>(i)); };
        }

        // Scale mode button — cycles Off → Filter → Highlight → Off
        scaleModeBtn.setButtonText("SCL");
        addAndMakeVisible(scaleModeBtn);
        A11y::setup(scaleModeBtn, "Scale Mode", "Cycle scale mode: Off, Filter (hide out-of-scale pads), Highlight");
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

            switch (ni.scaleMode)
            {
            case NoteInputZone::ScaleMode::Off:
                scaleModeBtn.setButtonText("SCL");
                break;
            case NoteInputZone::ScaleMode::Filter:
                scaleModeBtn.setButtonText("FLT");
                break;
            case NoteInputZone::ScaleMode::Highlight:
                scaleModeBtn.setButtonText("HLT");
                break;
            }
        };

        // ── TideController — wave-surface expression controller ───────────────
        // Sits in the left-panel slot; toggled by TIDE button.
        addAndMakeVisible(tideController_);
        tideController_.setVisible(false); // Hidden until TIDE button is pressed
        A11y::setup(tideController_, "Tide Controller",
                    "2D water-surface simulation — touch to create ripples, output maps to the target parameter");

        // TIDE toggle button — placed at far right of header left-side controls
        tideModeBtn_.setButtonText("TIDE");
        tideModeBtn_.setClickingTogglesState(true);
        addAndMakeVisible(tideModeBtn_);
        A11y::setup(tideModeBtn_, "Tide Mode",
                    "Toggle the Tide Controller water-surface expression panel in the left column");
        tideModeBtn_.onClick = [this]
        {
            tideActive_ = tideModeBtn_.getToggleState();
            tideController_.setVisible(tideActive_);
            resized(); // re-layout so bounds are assigned to the now-visible component
            // Route keyboard focus to the tide controller when active
            if (tideActive_)
                tideController_.grabKeyboardFocus();
        };

        // F-004: LATCH indicator badge — shown only in Keys mode.
        // Keys mode deliberately does NOT fire noteOff on mouse-up (latch behaviour).
        // This amber badge makes the held-state semantic visible so users understand
        // why notes sustain after release.  Non-interactive: mouse events pass through.
        latchBadge_.setButtonText("LATCH");
        latchBadge_.setInterceptsMouseClicks(false, false); // read-only indicator
        latchBadge_.setTooltip("Keys mode latches notes — release does not stop them. "
                               "Click another mode to release all held notes.");
        // Amber background (PS::kAmber) + dark text for high contrast on the deep bg.
        latchBadge_.setColour(juce::TextButton::buttonColourId,
                               juce::Colour(PS::kAmber).withAlpha(0.85f));
        latchBadge_.setColour(juce::TextButton::buttonOnColourId,
                               juce::Colour(PS::kAmber));
        latchBadge_.setColour(juce::TextButton::textColourOffId,
                               juce::Colour(0xFF1A1208)); // near-black — legible on amber
        latchBadge_.setColour(juce::TextButton::textColourOnId,
                               juce::Colour(0xFF1A1208));
        addAndMakeVisible(latchBadge_);
        latchBadge_.setVisible(surfaceTab_ == SurfaceTab::Keys); // default = Keys → visible
        A11y::setup(latchBadge_,
                    "Latch Active",
                    "Keys mode is active: notes latch and sustain after you release. "
                    "Switch to another mode to release all held notes.");

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
        A11y::setup(stripModeButtons[0], "Strip Mode: Dub", "Performance strip controls dub space delay effect");
        A11y::setup(stripModeButtons[1], "Strip Mode: Filter", "Performance strip controls filter sweep");
        A11y::setup(stripModeButtons[2], "Strip Mode: Coupling", "Performance strip controls coupling intensity");
        A11y::setup(stripModeButtons[3], "Strip Mode: Siren", "Performance strip controls dub siren pitch");

        for (int i = 0; i < 4; ++i)
        {
            stripModeButtons[i].onClick = [this, i]
            {
                strip.setStripMode(static_cast<PerformanceStrip::StripMode>(i));
            };
        }

        // F11: Explicit button colors — override default JUCE styling
        {
            auto applyBtnColors = [](juce::TextButton& btn)
            {
                btn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF2D2D2D));
                btn.setColour(juce::TextButton::buttonOnColourId, GalleryColors::get(GalleryColors::xoGold));
                btn.setColour(juce::TextButton::textColourOffId, juce::Colour(0xFFAAAAAA));
                btn.setColour(juce::TextButton::textColourOnId, GalleryColors::get(GalleryColors::textDark()));
            };
            for (int i = 0; i < kNumModeTabs; ++i)
                applyBtnColors(modeButtons[i]);
            applyBtnColors(padsSubModeBtn_); // D4: sub-mode toggle
            for (int i = 0; i < 4; ++i)
                applyBtnColors(bankButtons[i]);
            for (int i = 0; i < 4; ++i)
                applyBtnColors(stripModeButtons[i]);
            applyBtnColors(octDownBtn);
            applyBtnColors(octUpBtn);
            applyBtnColors(scaleModeBtn);
            applyBtnColors(tideModeBtn_);
        }

        // Timer is started in visibilityChanged() when the window becomes visible.
    }

    ~PlaySurface() override { stopTimer(); }

    // ── Task 12: Processor wiring ─────────────────────────────────────────────
    void setProcessor(XOceanusProcessor* p)
    {
        processor_ = p;
    }

    // Handle incoming CC — stub for future CC routing (see issue #1174).
    void handleIncomingCC(int cc, int value)
    {
        if (cc == 90)
        {
            // Drift toggle — store for future use.
            driftToggleState_ = (value >= 64);
        }
    }

    // P0-1: Wire the MIDI pipeline.
    // Call this from XOceanusEditor after construction, passing the processor's
    // MidiMessageCollector.  Once set, all note-on/off events from the PlaySurface
    // are delivered to the audio thread via the collector rather than the raw
    // std::function callbacks.
    void setMidiCollector(juce::MidiMessageCollector* collector, int channel = 1)
    {
        noteInput.midiCollector = collector;
        noteInput.midiChannel = channel;
        // V2: also wire KeysMode
        keysMode_.midiCollector = collector;
        keysMode_.midiChannel = channel;
    }

    // Engine Accent Adaptive: propagate accent colour to all four zones.
    // Call this from XOceanusEditor::timerCallback() when the active engine changes.
    void setAccentColour(juce::Colour c)
    {
        if (c == accentColour)
            return; // B1: early-return guard — skip repaints if unchanged
        accentColour = c;
        noteInput.setAccentColour(c);
        strip.setAccentColour(c);
        // V2 components
        keysMode_.setAccentColour(c);
        tideController_.setAccentColor(c);

        // P2-1: also update header button "on" colours so mode/bank/strip buttons
        // reflect the current engine accent when toggled.
        auto updateBtnAccent = [&](juce::TextButton& btn) { btn.setColour(juce::TextButton::buttonOnColourId, c); };
        for (int i = 0; i < kNumModeTabs; ++i)
            updateBtnAccent(modeButtons[i]);
        for (int i = 0; i < 4; ++i)
            updateBtnAccent(bankButtons[i]);
        for (int i = 0; i < 4; ++i)
            updateBtnAccent(stripModeButtons[i]);
        updateBtnAccent(octDownBtn);
        updateBtnAccent(octUpBtn);
        updateBtnAccent(scaleModeBtn);
        updateBtnAccent(tideModeBtn_);
        updateBtnAccent(padsSubModeBtn_);

        repaint();
    }

    // ── D4 (Wave 6): Surface-default auto-switch ──────────────────────────────
    // Call this from XOceanusEditor whenever the engine in a slot changes.
    // isDrumEngine = true  → switch to PADS tab + drum sub-mode (▦) automatically
    // isDrumEngine = false → no-op (respect whatever the user last set)
    // The user can always override by clicking a different tab.
    void setSurfaceDefault(bool isDrumEngine)
    {
        if (!isDrumEngine)
            return;

        // Auto-switch to PADS tab + drum sub-mode
        surfaceTab_   = SurfaceTab::Pads;
        drumSubMode_  = true;
        padsSubModeBtn_.setToggleState(true, juce::dontSendNotification);
        padsSubModeBtn_.setButtonText(kPadsSubModeLabels[1]); // KIT
        applyPadsSubMode();

        // Sync the tab button selection
        for (int i = 0; i < kNumModeTabs; ++i)
            modeButtons[i].setToggleState(i == 1 /* PADS */, juce::dontSendNotification);

        resized();
    }

    // ── D4 (Wave 6): APVTS-driven layout mode setter ─────────────────────────
    // Driven by slot[N]_layout_mode AudioParameterChoice (0=PlaySurface, 1=PadGrid).
    // Call from XOceanusEditor::timerCallback() or a param listener when the
    // APVTS value changes. PadGrid mode = shortcut to PADS tab with drum sub-mode.
    void setLayoutMode(int mode /* 0=PlaySurface, 1=PadGrid */)
    {
        if (mode == 1 /* PadGrid */)
        {
            setSurfaceDefault(true); // Switches to PADS + drum sub-mode
        }
        // mode 0 (PlaySurface) = no auto-switch; respect last user selection
    }

    // D3 (1D-P2B): LATCH state accessor + callback for status-bar display.
    // Returns true when Keys mode is active (latch is always on in Keys).
    // Parent (OceanView/TransportBar) wires onLatchStateChanged to update the
    // status-bar LATCH indicator whenever the tab changes.
    bool isLatchActive() const noexcept { return surfaceTab_ == SurfaceTab::Keys; }
    std::function<void(bool latchOn)> onLatchStateChanged;

    // Public zone accessors for wiring callbacks
    NoteInputZone& getNoteInput() { return noteInput; }
    PerformanceStrip& getStrip() { return strip; }

    // TideController accessors
    TideController& getTideController() { return tideController_; }

    /// Convenience: attach a JUCE parameter to the TideController output.
    /// The controller will call setValueNotifyingHost() on this parameter
    /// whenever the wave-surface mean height changes.
    /// Pass nullptr to detach.
    void setTideTargetParameter(juce::RangedAudioParameter* param)
    {
        tideController_.setTargetParameter(param);
    }

    void resized() override
    {
        auto bounds = getLocalBounds();

        // ── D2 (1D-P2B): Two-row header ──────────────────────────────────────
        // Row 1: mode tabs (KEYS | PADS | XY) + NOTE/KIT toggle right-adjacent
        //        to the PADS tab. No per-mode controls in row 1.
        // Row 2: per-mode controls (OCT ±, bank A–D, SCL, TIDE).
        //        Strip mode buttons (DUB/FILT/CPL/SIREN) right-aligned in row 2.
        // LATCH badge relocated to status bar area per D3 (no longer in header).

        auto headerRow1 = bounds.removeFromTop(PS::kHeaderH);
        auto headerRow2 = bounds.removeFromTop(PS::kHeaderH);

        // ── Row 1: mode tabs ──────────────────────────────────────────────────
        // 3 tabs at ~90px each (wider now that row 1 is uncluttered).
        int modeTabW = 90;
        for (int i = 0; i < kNumModeTabs; ++i)
            modeButtons[i].setBounds(headerRow1.removeFromLeft(modeTabW).reduced(2));

        // NOTE/KIT toggle — immediately after the PADS tab (index 1).
        // Visible only when PADS tab is active.
        headerRow1.removeFromLeft(2);
        padsSubModeBtn_.setBounds(headerRow1.removeFromLeft(42).reduced(2));
        padsSubModeBtn_.setVisible(surfaceTab_ == SurfaceTab::Pads);

        // ── Row 2: per-mode controls ──────────────────────────────────────────
        // Strip mode buttons right-aligned first (take from right before left fills).
        for (int i = 3; i >= 0; --i)
            stripModeButtons[i].setBounds(headerRow2.removeFromRight(36).reduced(2));

        // OCT controls (visible in KEYS and PADS tabs).
        const bool showOctBank = (surfaceTab_ == SurfaceTab::Keys
                                   || surfaceTab_ == SurfaceTab::Pads);
        octDownBtn.setBounds(headerRow2.removeFromLeft(32).reduced(2));
        octDownBtn.setVisible(showOctBank);
        octLabel.setBounds(headerRow2.removeFromLeft(40).reduced(2));
        octLabel.setVisible(showOctBank);
        octUpBtn.setBounds(headerRow2.removeFromLeft(32).reduced(2));
        octUpBtn.setVisible(showOctBank);

        // Bank selector buttons (only in PADS tab).
        headerRow2.removeFromLeft(4);
        for (int i = 0; i < 4; ++i)
        {
            bankButtons[i].setVisible(surfaceTab_ == SurfaceTab::Pads);
            bankButtons[i].setBounds(headerRow2.removeFromLeft(30).reduced(2));
        }

        // Scale mode button (KEYS/PADS tabs).
        headerRow2.removeFromLeft(4);
        scaleModeBtn.setBounds(headerRow2.removeFromLeft(36).reduced(2));
        scaleModeBtn.setVisible(showOctBank);

        // TIDE toggle button.
        headerRow2.removeFromLeft(4);
        tideModeBtn_.setBounds(headerRow2.removeFromLeft(40).reduced(2));

        // LATCH badge: hide completely from header — relocated to TransportBar (D3).
        // The badge itself is kept as a member for tooltip/A11y state, but it is
        // no longer positioned in the header row.  setBounds({}) ensures it is
        // off-screen; setVisible(false) keeps it from intercepting mouse events.
        latchBadge_.setBounds({});
        latchBadge_.setVisible(false);

        // ── D4 tab-dependent layout ───────────────────────────────────────────
        //
        // KEYS tab — full content area split: left = TideController, right = KeysMode
        // PADS tab — full content area split: left = TideController, right = NoteInputZone
        // XY   tab — full content area = PerformanceStrip (no bottom strip footer)
        //
        // The bottom PerformanceStrip is always shown except in XY full-screen tab.

        bool showLeftCol  = (surfaceTab_ == SurfaceTab::Keys || surfaceTab_ == SurfaceTab::Pads);
        bool showKeys     = (surfaceTab_ == SurfaceTab::Keys);
        bool showNoteInput = (surfaceTab_ == SurfaceTab::Pads);
        bool showXY       = (surfaceTab_ == SurfaceTab::XY);

        // 1D-2A C1: strip is always visible — bounds differ by tab.
        // (Was a dead-write pair: `setVisible(false)` then immediate `setVisible(true)`
        // in XY mode. Audit-flagged as misleading; collapsed to a single coherent path.)
        if (showXY)
            strip.setBounds(bounds); // full area in XY
        else
            strip.setBounds(bounds.removeFromBottom(PS::kStripH)); // bottom row in KEYS/PADS
        strip.setVisible(true);

        if (showLeftCol)
        {
            // ── Left column — TideController ─────────────────────────────────
            static constexpr int kLeftColW = 165; // left-panel slot width (px)
            auto leftColumnBounds = bounds.removeFromLeft(kLeftColW);
            // TideController: centred square within the left column (120pt target).
            // We give it the full column width; it clips itself to a circle.
            tideController_.setBounds(leftColumnBounds);
            tideController_.setVisible(tideActive_);
        }
        else
        {
            tideController_.setBounds({});
            tideController_.setVisible(false);
        }

        // ── Note area — remaining space ───────────────────────────────────────
        auto noteArea = bounds;

        noteInput.setVisible(showNoteInput);
        keysMode_.setVisible(showKeys);

        if (showKeys)
            keysMode_.setBounds(noteArea);
        else if (showNoteInput)
            noteInput.setBounds(noteArea);
    }

    void paint(juce::Graphics& g) override { g.fillAll(PS::surfaceBg()); }

    // Re-apply theme-sensitive label colors whenever the global theme changes.
    void lookAndFeelChanged() override
    {
        octLabel.setColour(juce::Label::textColourId, PS::textLight());
        repaint();
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

    bool keyPressed(const juce::KeyPress& key) override
    {
        // Let JUCE's focus chain route key events to the active sub-panel.
        // KeysMode has setWantsKeyboardFocus(true) and its own keyPressed override;
        // grabKeyboardFocus() is called on the relevant panel whenever a sub-mode
        // becomes active (see tide toggle and modeButtons callbacks), so no manual
        // delegation is needed here.
        return false;
    }

    bool keyStateChanged(bool /*isKeyDown*/) override { return false; }

private:
    void timerCallback() override
    {
        noteInput.tick();
        strip.tick();
    }

    juce::Colour accentColour{0xFFE9C46A}; // Default: XO Gold

    // ── Task 12: processor pointer for CC output forwarding ──────────────────
    XOceanusProcessor* processor_ = nullptr;
    bool driftToggleState_ = false; // CC 90 state — used by Task 13

    NoteInputZone noteInput;

    // V2 layout components
    PerformanceStrip strip;
    KeysMode keysMode_;
private:

    // TideController — wave-surface expression controller.
    // Shown in the left panel slot when tideActive_ is true.
    TideController tideController_;
    bool tideActive_ = false;

    // ── D4 (Wave 6): Top-level surface tab state ──────────────────────────────
    enum class SurfaceTab { Keys = 0, Pads = 1, XY = 2 };
    SurfaceTab surfaceTab_ = SurfaceTab::Keys; // default = KEYS tab

    // PADS sub-mode: false = scale-aware (♪), true = drum-kit (▦)
    bool drumSubMode_ = false;

    static constexpr int kNumModeTabs = 3; // KEYS | PADS | XY
    // Sub-mode toggle labels — ♪ = musical/scale-aware, ▦ = drum-kit
    static constexpr const char* kPadsSubModeLabels[2] = { "NOTE", "KIT" };
    //    ♪  = U+266A = \xe2\x99\xaa (UTF-8)
    //    ▦  = U+25A6 = \xe2\x96\xa6 (UTF-8)

    // ── Helper: apply NoteInputZone mode from pads sub-mode state ────────────
    void applyPadsSubMode()
    {
        noteInput.setMode(drumSubMode_ ? NoteInputZone::Mode::Drum : NoteInputZone::Mode::Pad);
    }

    std::array<juce::TextButton, kNumModeTabs> modeButtons;
    juce::TextButton padsSubModeBtn_; // D4: ♪/▦ sub-mode toggle inside PADS tab
    std::array<juce::TextButton, 4> stripModeButtons;
    std::array<juce::TextButton, 4> bankButtons; // A / B / C / D bank selectors
    juce::TextButton octDownBtn, octUpBtn;
    juce::Label octLabel;
    juce::TextButton scaleModeBtn;  // Cycles: SCL (Off) → FLT (Filter) → HLT (Highlight)
    juce::TextButton tideModeBtn_;  // Toggles TideController in the left panel slot

    // F-004: LATCH indicator badge — amber read-only badge shown only in Keys mode.
    // Keys mode silently latches notes (handleKeysUp does not fire noteOff).
    // The badge signals this held-state behaviour so users don't mistake sustained
    // notes for a bug.  It is non-interactive; the tooltip explains the semantics.
    juce::TextButton latchBadge_;

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
// Usage in XOceanusEditor:
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
    // V2 popup dimensions: 700×484 (wider than V1 — TideController + pad grid).
    // Was 520×520 in V1.
    static constexpr int kDefaultW = PS::kDesktopW; // 700
    static constexpr int kDefaultH = PS::kDesktopH; // 484
    static constexpr int kMinW = 500;
    static constexpr int kMinH = 400;
    static constexpr int kMaxW = 1200;
    static constexpr int kMaxH = 900;

    // Optional callback fired when the window is closed/hidden by the user.
    // XOceanusEditor uses this to sync the "PS" toggle button state.
    std::function<void()> onClosed;

    PlaySurfaceWindow()
        : juce::DocumentWindow("PlaySurface", PS::surfaceBg(), juce::DocumentWindow::allButtons,
                               true /* addToDesktop */)
    {
        setUsingNativeTitleBar(true);
        setResizable(true, true);
        setResizeLimits(kMinW, kMinH, kMaxW, kMaxH); // Was 320-1200 square in V1

        // Own the PlaySurface content component.
        // resizeToFitContent=false: we size the window explicitly via centreWithSize().
        auto* content = new PlaySurface();
        setContentOwned(content, true);

        // Position initially centered on the main screen.
        // V2 default: 700×484 (was 520×520 in V1)
        centreWithSize(kDefaultW, kDefaultH);

        // Performance tool window — keep it visible above the DAW.
        setAlwaysOnTop(true);
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
    PlaySurface& getPlaySurface() { return *static_cast<PlaySurface*>(getContentComponent()); }

    //----------------------------------------------------------------------
    // Closing hides the window rather than deleting it so MIDI state,
    // mode selections, and bank selections survive hide/show cycles.
    void closeButtonPressed() override
    {
        setVisible(false);
        if (onClosed)
            onClosed();
    }

    //----------------------------------------------------------------------
    // Escape key closes the popup (hides it).
    bool keyPressed(const juce::KeyPress& key) override
    {
        if (key == juce::KeyPress::escapeKey)
        {
            setVisible(false);
            if (onClosed)
                onClosed();
            return true;
        }
        return juce::DocumentWindow::keyPressed(key);
    }

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PlaySurfaceWindow)
};

} // namespace xoceanus
