#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include <array>
#include <vector>
#include <cmath>
#include <atomic>

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
}
#endif

//==============================================================================
// PlaySurface Constants
namespace PS {
    // Layout
    static constexpr int kDesktopW     = 1060;
    static constexpr int kDesktopH     = 344;
    static constexpr int kHeaderH      = 32;  // WCAG 2.5.5: minimum 24px touch targets after padding
    static constexpr int kZone1W       = 480;
    static constexpr int kZone2W       = 200;
    static constexpr int kZone4W       = 100;
    static constexpr int kZone3H       = 80;
    static constexpr int kMainZoneH    = 240;
    static constexpr int kPadCols      = 4;
    static constexpr int kPadRows      = 4;
    static constexpr int kNumPads      = kPadCols * kPadRows;
    static constexpr int kOrbitDiameter = 180;

    // Animation
    static constexpr float kOrbitFriction   = 0.98f;
    static constexpr float kSnapSpring      = 0.08f;
    static constexpr float kVelDecay        = 0.92f;
    static constexpr float kWarmMemoryDur   = 1.5f;  // seconds
    static constexpr int   kOrbitTrailSize  = 60;
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
    enum class Mode { Pad, Fretless, Drum };

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
    void setBank(Bank b) { currentBank = b; repaint(); }
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

    void setMode(Mode m) { mode = m; repaint(); }
    Mode getMode() const { return mode; }
    void setOctave(int oct) { octaveOffset = juce::jlimit(PS::kMinOctave, PS::kMaxOctave, oct); repaint(); }
    int  getOctave() const { return octaveOffset; }
    void setScale(int idx) { currentScale = juce::jlimit(0, (int)scales.size() - 1, idx); repaint(); }
    void setRootKey(int key) { rootKey = juce::jlimit(0, 11, key); repaint(); }

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
            // P0-3: on fretless release, reset pitch bend to 0 first
            if (mode == Mode::Fretless && midiCollector)
            {
                auto msg = juce::MidiMessage::pitchWheel(midiChannel, 0);
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
                int dist = std::abs(candidate - note);
                if (dist < bestDist) { bestDist = dist; best = candidate; }
            }
        }
        return juce::jlimit(0, 127, best);
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
            // Reset pitch bend to centre before the new note.
            if (midiCollector)
            {
                auto msg = juce::MidiMessage::pitchWheel(midiChannel, 0);
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

                    // Glow shadow: wider fill at low opacity
                    g.setColour(accentColour.withAlpha(vel * 0.25f));
                    g.fillRoundedRectangle(padRect.expanded(3.0f), 5.0f);
                }
                else
                {
                    // Non-hit pad: accent @ 0.07 fill
                    g.setColour(accentColour.withAlpha(0.07f));
                    g.fillRoundedRectangle(padRect, 4.0f);

                    // Accent @ 0.18 border
                    g.setColour(accentColour.withAlpha(0.18f));
                    g.drawRoundedRectangle(padRect, 4.0f, 1.0f);
                }

                // Warm memory ghost circles — radial gradient using accent
                for (const auto& wm : warmMemory)
                {
                    if (wm.pad == pad && wm.age < kWarmMemoryDur)
                    {
                        float alpha = (1.0f - wm.age / kWarmMemoryDur) * 0.15f;
                        float pcx = padRect.getCentreX(), pcy = padRect.getCentreY();
                        float gr = 10.0f;
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
                    label = juce::String(noteNames[note % 12]) + juce::String(note / 12 - 1);
                }

                // Note label: white on hit, accent @ 0.55 otherwise
                g.setColour(isHit ? juce::Colours::white : accentColour.withAlpha(0.55f));
                g.setFont(juce::Font(9.0f));
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
            g.setFont(juce::Font(9.0f).boldened());
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
                g.setFont(juce::Font(8.0f));
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

            // XO Gold ring — 2.5px for Retina clarity
            g.setColour(juce::Colour(0xFFE9C46A));
            g.drawEllipse(cx - ringR, noteY - ringR, ringR * 2, ringR * 2, 2.5f);

            // Center dot
            g.setColour(juce::Colour(0xFFE9C46A).withAlpha(0.7f));
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
                g.setColour(juce::Colour(0xFFE9C46A).withAlpha(lastFretlessVelocity_ * 0.9f));
                g.strokePath(arcPath, juce::PathStrokeType(3.0f));
            }
        }
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NoteInputZone)
};

//==============================================================================
// Zone 2: Orbit Path — Physics-based circular XY expression
//
class OrbitPathZone : public juce::Component
{
public:
    OrbitPathZone() = default;
    enum class PhysicsMode { Free, Lock, Snap };

    std::function<void(float x, float y)> onPositionChanged;

    void setPhysicsMode(PhysicsMode m) { physMode = m; repaint(); }

    // Engine accent colour — set by PlaySurface::setAccentColour()
    juce::Colour accentColour { 0xFFE9C46A };
    void setAccentColour(juce::Colour c) { accentColour = c; repaint(); }

    void paint(juce::Graphics& g) override
    {
        using namespace PS;

        // ── Tab strip (18px) — accent-adaptive ───────────────────────────
        static constexpr float kTabH = 18.0f;

        const char* tabLabels[3] = { "FREE", "LOCK", "SNAP" };
        float tabW = (float)getWidth() / 3.0f;

        for (int i = 0; i < 3; ++i)
        {
            bool active = (int)physMode == i;
            auto tabR = juce::Rectangle<float>(i * tabW, 0.0f, tabW, kTabH);

            // Active tab: accent @ 0.12 background, accent @ 0.95 text
            // Inactive tab: kSurfaceCard background, accent @ 0.40 text
            g.setColour(active ? accentColour.withAlpha(0.12f) : juce::Colour(kSurfaceCard));
            g.fillRect(tabR);
            g.setColour(accentColour.withAlpha(0.20f));
            g.drawRect(tabR, 0.5f);
            g.setFont(juce::Font(9.0f).boldened());
            g.setColour(active ? accentColour.withAlpha(0.95f) : accentColour.withAlpha(0.40f));
            g.drawText(tabLabels[i], tabR, juce::Justification::centred);
        }

        // ── Orbit area (below tab strip) ─────────────────────────────────
        auto orbitBounds = getLocalBounds().withTrimmedTop((int)kTabH).toFloat();
        float cx = orbitBounds.getCentreX();
        float cy = orbitBounds.getCentreY();
        float diam = juce::jmin(orbitBounds.getWidth(), orbitBounds.getHeight()) * 0.85f;
        float radius = diam * 0.5f;

        // Background
        g.setColour(juce::Colour(kSurfaceBg));
        g.fillRect(orbitBounds);

        // Ring interior: radial gradient accent @ 0.05 center → transparent
        {
            juce::ColourGradient ringFill(accentColour.withAlpha(0.05f), cx, cy,
                                          accentColour.withAlpha(0.0f), cx + radius, cy, true);
            g.setGradientFill(ringFill);
            g.fillEllipse(cx - radius, cy - radius, radius * 2, radius * 2);
        }

        // Ring border: accent @ 0.22
        g.setColour(accentColour.withAlpha(0.22f));
        g.drawEllipse(cx - radius, cy - radius, radius * 2, radius * 2, 1.0f);

        // Center crosshair: accent @ 0.07
        g.setColour(accentColour.withAlpha(0.07f));
        g.drawHorizontalLine((int)cy, cx - radius, cx + radius);
        g.drawVerticalLine((int)cx, cy - radius, cy + radius);

        // Trail — 2 ghost points with accent @ 0.22 and @ 0.12
        // (Draw only the last 2 meaningful trail points from the ring buffer)
        {
            int drawn = 0;
            for (int i = 1; i < kOrbitTrailSize && drawn < 2; ++i)
            {
                int idx = (trailHead - i + kOrbitTrailSize) % kOrbitTrailSize;
                if (trail[idx].x == 0.0f && trail[idx].y == 0.0f) continue;
                float alpha = (drawn == 0) ? 0.22f : 0.12f;
                float size  = (drawn == 0) ? 8.0f  : 6.0f;
                float tx = cx + trail[idx].x * radius;
                float ty = cy + trail[idx].y * radius;
                // Box-shadow approximation: slightly larger translucent circle behind
                g.setColour(accentColour.withAlpha(alpha * 0.4f));
                g.fillEllipse(tx - size, ty - size, size * 2, size * 2);
                g.setColour(accentColour.withAlpha(alpha));
                g.fillEllipse(tx - size * 0.5f, ty - size * 0.5f, size, size);
                ++drawn;
            }
        }

        // Cursor: 13px, radial gradient from lighten(accent,60%) center → accent @ 0.50 edge
        float cursorX = cx + posX * radius;
        float cursorY = cy + posY * radius;
        {
            const float cursorR = 6.5f; // 13px diameter
            juce::Colour cursorCenter = lightenColour(accentColour, 0.60f);
            juce::ColourGradient cursorGrad(cursorCenter, cursorX, cursorY,
                                            accentColour.withAlpha(0.50f),
                                            cursorX + cursorR, cursorY, true);
            g.setGradientFill(cursorGrad);
            g.fillEllipse(cursorX - cursorR, cursorY - cursorR, cursorR * 2, cursorR * 2);
        }

        // ── LOCK mode: accent anchor dot at center ────────────────────────
        if (physMode == PhysicsMode::Lock)
        {
            g.setColour(accentColour.withAlpha(0.9f));
            g.fillEllipse(cx - 4.0f, cy - 4.0f, 8.0f, 8.0f);
            g.setColour(accentColour.withAlpha(0.35f));
            g.drawEllipse(cx - 8.0f, cy - 8.0f, 16.0f, 16.0f, 1.0f);
        }

        // ── SNAP mode: faint dotted arc from cursor toward center ─────────
        if (physMode == PhysicsMode::Snap)
        {
            float dx = cx - cursorX;
            float dy = cy - cursorY;
            float dist = std::sqrt(dx * dx + dy * dy);
            if (dist > 2.0f)
            {
                juce::Path springPath;
                int steps = 12;
                for (int s = 0; s <= steps; ++s)
                {
                    float t = (float)s / (float)steps;
                    float px = cursorX + dx * t;
                    float py = cursorY + dy * t;
                    if (s == 0) springPath.startNewSubPath(px, py);
                    else         springPath.lineTo(px, py);
                }
                juce::PathStrokeType stroke(1.0f);
                float dashLengths[] = { 3.0f, 3.0f };
                stroke.createDashedStroke(springPath, springPath, dashLengths, 2);
                g.setColour(accentColour.withAlpha(0.40f));
                g.strokePath(springPath, stroke);
                // Small spring-return target dot at center
                g.setColour(accentColour.withAlpha(0.60f));
                g.fillEllipse(cx - 3.0f, cy - 3.0f, 6.0f, 6.0f);
            }
        }

        // Axis labels: accent @ 0.25
        g.setColour(accentColour.withAlpha(0.25f));
        g.setFont(juce::Font(9.0f));
        g.drawText("CUTOFF", juce::Rectangle<float>(cx - radius, cy + radius + 2, radius * 2, 12),
                   juce::Justification::centred);
        g.drawText("RES", juce::Rectangle<float>(cx + radius + 2, cy - 6, 30, 12),
                   juce::Justification::centredLeft);
    }

    void mouseDown(const juce::MouseEvent& e) override
    {
        // Tab strip click (top 18px)
        if (e.y < 18)
        {
            int tab = (int)(e.x / ((float)getWidth() / 3.0f));
            physMode = (PhysicsMode)juce::jlimit(0, 2, tab);
            repaint();
            return;
        }
        updateFromMouse(e);
        dragging = true;
    }
    void mouseDrag(const juce::MouseEvent& e) override { if (dragging) updateFromMouse(e); }
    void mouseUp(const juce::MouseEvent&) override { dragging = false; }

    // Called at 30fps
    void tick()
    {
        float prevX = posX, prevY = posY;

        if (!dragging)
        {
            switch (physMode)
            {
                case PhysicsMode::Free:
                    posX += velX;
                    posY += velY;
                    velX *= PS::kOrbitFriction;
                    velY *= PS::kOrbitFriction;
                    constrainToCircle();
                    break;
                case PhysicsMode::Snap:
                    posX += (0.0f - posX) * PS::kSnapSpring;
                    posY += (0.0f - posY) * PS::kSnapSpring;
                    break;
                case PhysicsMode::Lock:
                    break; // Stay put
            }
        }

        // Record trail
        trail[(size_t)trailHead] = { posX, posY };
        trailHead = (trailHead + 1) % PS::kOrbitTrailSize;

        if (onPositionChanged)
            onPositionChanged((posX + 1.0f) * 0.5f, (posY + 1.0f) * 0.5f);

        // Only repaint when position actually moved (avoids idle redraws in Lock mode)
        bool moved = std::fabs(posX - prevX) > 0.0001f || std::fabs(posY - prevY) > 0.0001f;
        if (moved || dragging)
            repaint();
    }

private:
    PhysicsMode physMode = PhysicsMode::Free;
    float posX = 0.0f, posY = 0.0f;
    float velX = 0.0f, velY = 0.0f;
    bool  dragging = false;

    struct TrailPoint { float x = 0.0f, y = 0.0f; };
    std::array<TrailPoint, PS::kOrbitTrailSize> trail {};
    int trailHead = 0;

    void updateFromMouse(const juce::MouseEvent& e)
    {
        // Orbit area sits below the 18px tab strip
        auto orbitBounds = getLocalBounds().withTrimmedTop(18).toFloat();
        float cx = orbitBounds.getCentreX();
        float cy = orbitBounds.getCentreY();
        float diam = juce::jmin(orbitBounds.getWidth(), orbitBounds.getHeight()) * 0.85f;
        float radius = diam * 0.5f;

        float newX = (e.x - cx) / radius;
        float newY = (e.y - cy) / radius;

        // Velocity from delta
        velX = (newX - posX) * 0.3f;
        velY = (newY - posY) * 0.3f;

        posX = newX;
        posY = newY;
        constrainToCircle();
    }

    void constrainToCircle()
    {
        float dist = std::sqrt(posX * posX + posY * posY);
        if (dist > 1.0f)
        {
            // Reflect off boundary
            float nx = posX / dist, ny = posY / dist;
            float dotVN = velX * nx + velY * ny;
            velX -= 2.0f * dotVN * nx;
            velY -= 2.0f * dotVN * ny;
            posX = nx * 0.99f;
            posY = ny * 0.99f;
        }
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OrbitPathZone)
};

//==============================================================================
// Zone 3: Performance Strip — Full-width XY gestural controller
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

        // Gesture trail — accent coloured
        for (int i = 0; i < kStripTrailSize; ++i)
        {
            int idx = (stripTrailHead - i + kStripTrailSize) % kStripTrailSize;
            auto& pt = stripTrail[idx];
            if (pt.age > kWarmMemoryDur) continue;
            float alpha = (1.0f - pt.age / kWarmMemoryDur) * 0.5f;
            float sx = b.getX() + pt.x * b.getWidth();
            float sy = b.getY() + (1.0f - pt.y) * b.getHeight();
            g.setColour(accentColour.withAlpha(alpha));
            g.fillEllipse(sx - 2, sy - 2, 4, 4);
        }

        // Bar indicator: 3px wide vertical bar at stripX, linear gradient accent @ 0.70 bottom → transparent top
        {
            float barX = b.getX() + stripX * b.getWidth() - 1.5f;
            juce::ColourGradient barGrad(accentColour.withAlpha(0.70f), barX, b.getBottom(),
                                         accentColour.withAlpha(0.0f),  barX, b.getY(), false);
            g.setGradientFill(barGrad);
            g.fillRect(juce::Rectangle<float>(barX, b.getY(), 3.0f, b.getHeight()));

            // Floor glow: 18px wide radial gradient under the bar
            float floorY = b.getBottom() - 4.0f;
            float glowCx = barX + 1.5f;
            juce::ColourGradient floorGlow(accentColour.withAlpha(0.08f), glowCx, floorY,
                                           accentColour.withAlpha(0.0f), glowCx + 9.0f, floorY, true);
            g.setGradientFill(floorGlow);
            g.fillEllipse(glowCx - 9.0f, floorY - 4.0f, 18.0f, 8.0f);
        }

        // Cursor dot
        float cursorCx = b.getX() + stripX * b.getWidth();
        float cursorCy = b.getY() + (1.0f - stripY) * b.getHeight();
        g.setColour(accentColour.withAlpha(0.20f));
        g.fillEllipse(cursorCx - 12, cursorCy - 12, 24, 24);
        g.setColour(accentColour);
        g.fillEllipse(cursorCx - 5, cursorCy - 5, 10, 10);

        // Mode label: accent @ 0.40
        g.setColour(accentColour.withAlpha(0.40f));
        g.setFont(juce::Font(8.0f));
        static const char* modeNames[] = { "DUB SPACE", "FILTER SWEEP", "COUPLING", "DUB SIREN" };
        g.drawText(modeNames[(int)stripMode], b.reduced(4), juce::Justification::topLeft);

        // Axis labels: accent @ 0.40
        static const char* xLabels[] = { "DELAY FB", "CUTOFF", "X>O PUMP", "ECHO" };
        static const char* yLabels[] = { "REVERB", "RESONANCE", "O>X DRIFT", "PITCH" };
        g.drawText(xLabels[(int)stripMode], b.reduced(4), juce::Justification::bottomLeft);
        g.drawText(yLabels[(int)stripMode], b.reduced(4), juce::Justification::topRight);
    }

    void mouseDown(const juce::MouseEvent& e) override { updateStrip(e); touching = true; }
    void mouseDrag(const juce::MouseEvent& e) override { if (touching) updateStrip(e); }
    void mouseUp(const juce::MouseEvent&) override { touching = false; }

    void tick()
    {
        float prevX = stripX, prevY = stripY;

        if (!touching)
        {
            // Spring-back
            float targetX = springTargets[(int)stripMode].x;
            float targetY = springTargets[(int)stripMode].y;
            stripX += (targetX - stripX) * PS::kSnapSpring;
            stripY += (targetY - stripY) * PS::kSnapSpring;
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
        auto b = getLocalBounds().toFloat();
        stripX = juce::jlimit(0.0f, 1.0f, (e.x - b.getX()) / b.getWidth());
        stripY = juce::jlimit(0.0f, 1.0f, 1.0f - (e.y - b.getY()) / b.getHeight());

        stripTrail[(size_t)stripTrailHead] = { stripX, stripY, 0.0f };
        stripTrailHead = (stripTrailHead + 1) % PS::kStripTrailSize;
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PerformanceStrip)
};

//==============================================================================
// Zone 4: Performance Pads — FIRE, XOSEND, ECHO CUT, PANIC
//
class PerformancePads : public juce::Component
{
public:
    PerformancePads() = default;
    std::function<void()> onFire;
    std::function<void(bool held)> onXoSend;
    std::function<void(bool held)> onEchoCut;
    std::function<void()> onPanic;

    // Engine accent colour — set by PlaySurface::setAccentColour()
    juce::Colour accentColour { 0xFFE9C46A };
    void setAccentColour(juce::Colour c) { accentColour = c; repaint(); }

    void paint(juce::Graphics& g) override
    {
        using namespace PS;
        auto b = getLocalBounds().toFloat();
        float padH = b.getHeight() / 4.0f;

        static const char* labels[] = { "FIRE", "XOSEND", "ECHO CUT", "PANIC" };
        static const char* keys[]   = { "Z", "X", "C", "V" };

        for (int i = 0; i < 4; ++i)
        {
            auto rect = juce::Rectangle<float>(b.getX(), b.getY() + i * padH, b.getWidth(), padH).reduced(3.0f);
            bool pressed = padStates[i];

            if (i == 3)
            {
                // PANIC: always red, independent of engine accent
                juce::Colour panicCol = juce::Colour(kPanicRed);
                g.setColour(pressed ? panicCol : panicCol.withAlpha(0.15f));
                g.fillRoundedRectangle(rect, 6.0f);
                g.setColour(panicCol.withAlpha(pressed ? 1.0f : 0.35f));
                g.drawRoundedRectangle(rect, 6.0f, 2.0f);
                g.setColour(pressed ? juce::Colours::black : panicCol);
                g.setFont(juce::Font(10.0f).boldened());
                g.drawText(labels[i], rect, juce::Justification::centred);
                // Keyboard hint: panic uses red @ 0.45
                g.setColour(panicCol.withAlpha(0.45f));
                g.setFont(juce::Font(9.0f));
                g.drawText(keys[i], rect.reduced(4), juce::Justification::bottomRight);
            }
            else
            {
                // FIRE / X-SEND / ECHO: engine accent @ 0.06 bg, @ 0.15 border, @ 0.55 text
                if (pressed)
                {
                    g.setColour(accentColour.withAlpha(0.25f));
                    g.fillRoundedRectangle(rect, 6.0f);
                    g.setColour(accentColour.withAlpha(0.50f));
                    g.drawRoundedRectangle(rect, 6.0f, 2.0f);
                    // Active text: lighten(accent, 60%)
                    g.setColour(lightenColour(accentColour, 0.60f));
                }
                else
                {
                    g.setColour(accentColour.withAlpha(0.06f));
                    g.fillRoundedRectangle(rect, 6.0f);
                    g.setColour(accentColour.withAlpha(0.15f));
                    g.drawRoundedRectangle(rect, 6.0f, 2.0f);
                    // Normal text: accent @ 0.55
                    g.setColour(accentColour.withAlpha(0.55f));
                }
                g.setFont(juce::Font(10.0f).boldened());
                g.drawText(labels[i], rect, juce::Justification::centred);

                // Keyboard hint: accent @ 0.45
                g.setColour(accentColour.withAlpha(0.45f));
                g.setFont(juce::Font(9.0f));
                g.drawText(keys[i], rect.reduced(4), juce::Justification::bottomRight);
            }
        }
    }

    void mouseDown(const juce::MouseEvent& e) override
    {
        int pad = padFromY(e.y);
        if (pad < 0 || pad > 3) return;
        activePad = pad;
        padStates[(size_t)pad] = true;
        firePad(pad, true);
        repaint();
    }

    void mouseUp(const juce::MouseEvent&) override
    {
        int pad = activePad;
        activePad = -1;
        if (pad < 0 || pad > 3) return;
        padStates[(size_t)pad] = false;
        firePad(pad, false);
        repaint();
    }

    // Keyboard shortcut support (called from parent keyPressed)
    bool handleKey(const juce::KeyPress& key, bool isDown)
    {
        int pad = -1;
        if (key == juce::KeyPress('z') || key == juce::KeyPress('Z')) pad = 0;
        if (key == juce::KeyPress('x') || key == juce::KeyPress('X')) pad = 1;
        if (key == juce::KeyPress('c') || key == juce::KeyPress('C')) pad = 2;
        if (key == juce::KeyPress('v') || key == juce::KeyPress('V')) pad = 3;
        if (pad < 0) return false;
        padStates[(size_t)pad] = isDown;
        firePad(pad, isDown);
        repaint();
        return true;
    }

private:
    std::array<bool, 4> padStates {};
    int activePad = -1;

    int padFromY(int y) const
    {
        float padH = getHeight() / 4.0f;
        return juce::jlimit(0, 3, (int)(y / padH));
    }

    void firePad(int pad, bool pressed)
    {
        switch (pad)
        {
            case 0: if (pressed && onFire) onFire(); break;
            case 1: if (onXoSend) onXoSend(pressed); break;
            case 2: if (onEchoCut) onEchoCut(pressed); break;
            case 3: if (pressed && onPanic) onPanic(); break;
        }
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PerformancePads)
};

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
        setDescription ("4-zone performance interface: note input, orbit path, performance strip, and drum pads");
        setWantsKeyboardFocus (true);

        noteInput.setTitle ("Note Input Zone");
        noteInput.setDescription ("Pad grid or fretless strip for note input");
        orbitPath.setTitle ("Orbit Path");
        orbitPath.setDescription ("XY controller with orbit physics for continuous modulation");
        strip.setTitle ("Performance Strip");
        strip.setDescription ("Touch strip for filter sweeps, coupling, and dub effects");
        perfPads.setTitle ("Performance Pads");
        perfPads.setDescription ("4 trigger pads with keyboard shortcuts Z, X, C, V");
        perfPads.setWantsKeyboardFocus (true);

        addAndMakeVisible(noteInput);
        addAndMakeVisible(orbitPath);
        addAndMakeVisible(strip);
        addAndMakeVisible(perfPads);

        // Header controls: mode buttons
        for (int i = 0; i < 3; ++i)
        {
            modeButtons[i].setClickingTogglesState(true);
            modeButtons[i].setRadioGroupId(101);
            addAndMakeVisible(modeButtons[i]);
        }
        modeButtons[0].setButtonText("PAD");
        modeButtons[1].setButtonText("FRETLESS");
        modeButtons[2].setButtonText("DRUM");
        modeButtons[0].setToggleState(true, juce::dontSendNotification);

        for (int i = 0; i < 3; ++i)
        {
            modeButtons[i].onClick = [this, i]
            {
                noteInput.setMode(static_cast<NoteInputZone::Mode>(i));
            };
        }

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
                btn.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xFFE9C46A));
                btn.setColour(juce::TextButton::textColourOffId,  juce::Colour(0xFFAAAAAA));
                btn.setColour(juce::TextButton::textColourOnId,   juce::Colour(0xFF1A1A1A));
            };
            for (int i = 0; i < 3; ++i) applyBtnColors(modeButtons[i]);
            for (int i = 0; i < 4; ++i) applyBtnColors(bankButtons[i]);
            for (int i = 0; i < 4; ++i) applyBtnColors(stripModeButtons[i]);
            applyBtnColors(octDownBtn);
            applyBtnColors(octUpBtn);
        }

        // Timer is started in visibilityChanged() when the window becomes visible.
    }

    ~PlaySurface() override { stopTimer(); }

    // P0-1: Wire the MIDI pipeline.
    // Call this from XOlokunEditor after construction, passing the processor's
    // MidiMessageCollector.  Once set, all note-on/off events from the PlaySurface
    // are delivered to the audio thread via the collector rather than the raw
    // std::function callbacks.
    void setMidiCollector(juce::MidiMessageCollector* collector, int channel = 1)
    {
        noteInput.midiCollector = collector;
        noteInput.midiChannel   = channel;
    }

    // Engine Accent Adaptive: propagate accent colour to all four zones.
    // Call this from XOlokunEditor::timerCallback() when the active engine changes.
    void setAccentColour(juce::Colour c)
    {
        accentColour = c;
        noteInput.setAccentColour(c);
        orbitPath.setAccentColour(c);
        strip.setAccentColour(c);
        perfPads.setAccentColour(c);
        repaint();
    }

    // Public zone accessors for wiring callbacks
    NoteInputZone&     getNoteInput()  { return noteInput; }
    OrbitPathZone&     getOrbitPath()  { return orbitPath; }
    PerformanceStrip&  getStrip()     { return strip; }
    PerformancePads&   getPerfPads()  { return perfPads; }

    void resized() override
    {
        auto area = getLocalBounds();

        // Header bar
        auto header = area.removeFromTop(PS::kHeaderH);
        int btnW = 60;
        for (int i = 0; i < 3; ++i)
            modeButtons[i].setBounds(header.removeFromLeft(btnW).reduced(2));
        header.removeFromLeft(10);
        octDownBtn.setBounds(header.removeFromLeft(32).reduced(2));
        octLabel.setBounds(header.removeFromLeft(44).reduced(2));
        octUpBtn.setBounds(header.removeFromLeft(32).reduced(2));

        // Bank selector buttons — 24px each, immediately after octave controls
        header.removeFromLeft(8); // small gap
        for (int i = 0; i < 4; ++i)
            bankButtons[i].setBounds(header.removeFromLeft(24).reduced(2));

        // Strip mode buttons at right of header
        for (int i = 3; i >= 0; --i)
            stripModeButtons[i].setBounds(header.removeFromRight(48).reduced(2));

        // Bottom strip — performance strip (full width below main zones)
        auto stripArea = area.removeFromBottom(PS::kZone3H);
        strip.setBounds(stripArea);

        // Main zones (left to right).
        // Zone 1 (NoteInputZone): the pad grid is always drawn as a 4x4 square grid
        // centered within whatever bounds it receives, so we give it a square region
        // here equal to the available height × height to produce square pads at any size.
        // Zone 4 (PerformancePads): fixed 100px wide strip on the right.
        // Zone 2 (OrbitPath): center gets the remainder.
        auto mainArea = area;
        int mainH = mainArea.getHeight();

        // Give Zone 1 a square region clamped so it never overflows at narrow aspect ratios
        int z1w = std::min(mainH, mainArea.getWidth() - PS::kZone4W - 60);
        z1w = std::max(z1w, 120);  // minimum usable pad size
        // Zone 4 fixed width
        int z4w = std::min(PS::kZone4W, mainArea.getWidth() - z1w - 40);
        z4w = std::max(z4w, 60); // floor so it stays usable

        noteInput.setBounds(mainArea.removeFromLeft(z1w));
        perfPads.setBounds(mainArea.removeFromRight(z4w));
        orbitPath.setBounds(mainArea); // center gets the rest (OrbitPath scales)
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
        // Release all held performance pads when focus leaves the PlaySurface.
        for (int i = 0; i < 4; ++i)
            perfPads.handleKey(juce::KeyPress('z' + i), false);
    }

    bool keyPressed(const juce::KeyPress& key) override
    {
        return perfPads.handleKey(key, true);
    }

    // P0-1 / audit 4.2: handle key release so XOSEND and ECHO CUT receive
    // the isDown=false event and held-button state is cleared.
    bool keyStateChanged(bool /*isKeyDown*/) override
    {
        bool handled = false;
        // Check each of the four keys; dispatch release if no longer held.
        const juce::KeyPress keys[] = {
            juce::KeyPress('z'), juce::KeyPress('x'),
            juce::KeyPress('c'), juce::KeyPress('v'),
        };
        for (int i = 0; i < 4; ++i)
        {
            if (!keys[i].isCurrentlyDown())
                handled |= perfPads.handleKey(keys[i], false);
        }
        return handled;
    }

private:
    void timerCallback() override
    {
        noteInput.tick();
        orbitPath.tick();
        strip.tick();
    }

    juce::Colour accentColour { 0xFFE9C46A }; // Default: XO Gold

    NoteInputZone      noteInput;
    OrbitPathZone      orbitPath;
    PerformanceStrip   strip;
    PerformancePads    perfPads;

    std::array<juce::TextButton, 3> modeButtons;
    std::array<juce::TextButton, 4> stripModeButtons;
    std::array<juce::TextButton, 4> bankButtons;  // A / B / C / D bank selectors
    juce::TextButton octDownBtn, octUpBtn;
    juce::Label      octLabel;

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
    // Default popup dimensions: 520×520 so each pad cell is 4×4 of 130px squares
    // (header 32px + strip 80px = 112px overhead; pads get ~408px => ~102px/pad).
    static constexpr int kDefaultW  = 520;
    static constexpr int kDefaultH  = 520;
    static constexpr int kMinSize   = 320;
    static constexpr int kMaxW      = 1200;
    static constexpr int kMaxH      = 1200;

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
        setResizeLimits (kMinSize, kMinSize, kMaxW, kMaxH);

        // Own the PlaySurface content component.
        // resizeToFitContent=false: we size the window explicitly via centreWithSize().
        auto* content = new PlaySurface();
        setContentOwned (content, true);

        // Position initially centered on the main screen.
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
