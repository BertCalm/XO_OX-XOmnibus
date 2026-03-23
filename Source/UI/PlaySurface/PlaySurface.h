#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include <array>
#include <vector>
#include <cmath>

namespace xomnibus {

//==============================================================================
// Forward declarations for color/font access from XOmnibusEditor.h
// (included after the main editor header)
namespace GalleryColors {
    inline juce::Colour get(uint32_t hex) { return juce::Colour(hex); }
}

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

    std::function<void(int note, float velocity)> onNoteOn;
    std::function<void(int note)> onNoteOff;

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
        g.fillAll(juce::Colour(kSurfaceBg));

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
            if (onNoteOff) onNoteOff(lastNote);
            lastNote = -1;
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
    struct ScaleDef { const char* name; std::vector<int> intervals; };
    struct WarmMemoryEntry { int pad = -1; float age = 99.0f; };

    Mode mode = Mode::Pad;
    int  octaveOffset = 0;
    int  currentScale = 0;
    int  rootKey = 0;
    int  lastNote = -1;
    float lastFretlessVelocity_ = 0.75f;  // updated on each fretless touch; drives ring glow
    std::vector<ScaleDef> scales;
    std::array<float, PS::kNumPads> padVelocity {};
    std::array<WarmMemoryEntry, 8> warmMemory {};
    int warmMemIdx = 0;

    int midiNoteForPad(int pad) const
    {
        int row = pad / PS::kPadCols;
        int col = pad % PS::kPadCols;
        int rawNote = PS::kBaseNote + (octaveOffset * 12) + (row * 4) + col;
        return quantizeToScale(rawNote);
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

        auto b = getLocalBounds();
        int col = (int)((float)e.x / b.getWidth() * PS::kPadCols);
        int row = PS::kPadRows - 1 - (int)((float)e.y / b.getHeight() * PS::kPadRows);
        col = juce::jlimit(0, PS::kPadCols - 1, col);
        row = juce::jlimit(0, PS::kPadRows - 1, row);
        int pad = row * PS::kPadCols + col;

        float velocity = juce::jlimit(0.3f, 1.0f, 1.0f - (float)e.y / b.getHeight());
        int note = midiNoteForPad(pad);

        if (isDown || note != lastNote)
        {
            if (lastNote >= 0 && onNoteOff) onNoteOff(lastNote);
            padVelocity[(size_t)pad] = velocity;
            warmMemory[(size_t)warmMemIdx] = { pad, 0.0f };
            warmMemIdx = (warmMemIdx + 1) % (int)warmMemory.size();
            lastNote = note;
            if (onNoteOn) onNoteOn(note, velocity);
        }
    }

    void handleFretlessTouch(const juce::MouseEvent& e, bool isDown)
    {
        auto b = getLocalBounds();
        float yNorm = 1.0f - (float)e.y / b.getHeight(); // bottom=0, top=1
        int note = 24 + (int)(yNorm * 72.0f); // C1-C7
        note = juce::jlimit(24, 96, quantizeToScale(note));

        // Y-axis expression mapping:
        //   bottom 20% of strip  → velocity 0.35f (softest touch zone)
        //   top 20% of strip     → velocity 1.00f (brightest touch zone)
        //   middle 60%           → linear interpolation across full strip height
        float velocity = juce::jmap(yNorm, 0.0f, 1.0f, 0.35f, 1.0f);
        velocity = juce::jlimit(0.35f, 1.0f, velocity);

        lastFretlessVelocity_ = velocity;

        if (isDown || note != lastNote)
        {
            if (lastNote >= 0 && onNoteOff) onNoteOff(lastNote);
            lastNote = note;
            if (onNoteOn) onNoteOn(note, velocity);
        }
    }

    // Pad grid with ecological depth zone coloring.
    // Row 3 (top/high notes) = Sunlit zone (cyan), Row 0 (bottom/low) = Midnight (violet).
    void paintPadGrid(juce::Graphics& g)
    {
        using namespace PS;
        auto b = getLocalBounds().toFloat();
        float padW = b.getWidth() / kPadCols;
        float padH = b.getHeight() / kPadRows;

        // Depth zone colors mapped to rows (displayRow 0=top=sunlit, 3=bottom=midnight)
        const juce::Colour kZoneColors[4] = {
            juce::Colour(0xFF48CAE4),  // row 0 display (top — Sunlit)
            juce::Colour(0xFF0096C7),  // row 1 (Twilight upper)
            juce::Colour(0xFF0070AA),  // row 2 (Twilight lower)
            juce::Colour(0xFF7B2FBE),  // row 3 (Midnight)
        };

        for (int row = 0; row < kPadRows; ++row)
        {
            for (int col = 0; col < kPadCols; ++col)
            {
                int pad = row * kPadCols + col;
                int displayRow = kPadRows - 1 - row; // flip for bottom-left origin
                float x = col * padW;
                float y = displayRow * padH;
                auto padRect = juce::Rectangle<float>(x, y, padW, padH).reduced(2.0f);

                juce::Colour zoneCol = kZoneColors[displayRow];

                // Base color — very dark, with zone tint
                g.setColour(juce::Colour(kSurfaceBg).interpolatedWith(zoneCol, 0.06f));
                g.fillRoundedRectangle(padRect, 4.0f);

                // Zone border — subtle color classification
                g.setColour(zoneCol.withAlpha(0.18f));
                g.drawRoundedRectangle(padRect, 4.0f, 1.0f);

                // Velocity heatmap glow — zone-colored fill only (no competing highlight layer)
                float vel = padVelocity[(size_t)pad];
                if (vel > 0.01f)
                {
                    g.setColour(zoneCol.withAlpha(vel * 0.65f));
                    g.fillRoundedRectangle(padRect, 4.0f);
                }

                // Warm memory ghost circles
                for (const auto& wm : warmMemory)
                {
                    if (wm.pad == pad && wm.age < kWarmMemoryDur)
                    {
                        float alpha = (1.0f - wm.age / kWarmMemoryDur) * 0.4f;
                        g.setColour(juce::Colours::white.withAlpha(alpha));
                        float cx = padRect.getCentreX(), cy = padRect.getCentreY();
                        g.drawEllipse(cx - 8, cy - 8, 16, 16, 1.5f);
                    }
                }

                // Note label
                int note = midiNoteForPad(pad);
                static const char* noteNames[] = {"C","Db","D","Eb","E","F","Gb","G","Ab","A","Bb","B"};
                juce::String label;
                if (mode == Mode::Drum)
                {
                    static const char* drumNames[] = {
                        "Kick","Snare","Kick","Snare",  // row 0
                        "Clap","HH-C","HH-O","Acc",    // row 1
                        "Tom","Perc A","Perc B","Acc",  // row 2
                        "","","","",                     // row 3
                    };
                    label = drumNames[pad];
                }
                else
                {
                    label = juce::String(noteNames[note % 12]) + juce::String(note / 12 - 1);
                }

                // Note label — zone-tinted
                g.setColour(zoneCol.withAlpha(vel > 0.01f ? 0.95f : 0.55f));
                g.setFont(juce::Font(9.0f));
                g.drawText(label, padRect, juce::Justification::centred);
            }
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
    enum class PhysicsMode { Free, Lock, Snap };

    std::function<void(float x, float y)> onPositionChanged;

    void setPhysicsMode(PhysicsMode m) { physMode = m; repaint(); }

    void paint(juce::Graphics& g) override
    {
        using namespace PS;

        // ── Tab strip (18px) ──────────────────────────────────────────────
        static constexpr float kTabH = 18.0f;
        // Chromatophore Amber (OUTWIT accent) for active tab
        static constexpr uint32_t kTabActive    = 0xFFCC6600;
        static constexpr uint32_t kTabActiveTxt = 0xFF1A1A1A;
        static constexpr uint32_t kTabInactive  = 0xFF2D2D2D;  // kSurfaceCard
        static constexpr uint32_t kTabBorder    = 0xFF444444;
        static constexpr uint32_t kTabInactTxt  = 0xFF888888;  // kTextDim

        const char* tabLabels[3] = { "FREE", "LOCK", "SNAP" };
        float tabW = (float)getWidth() / 3.0f;

        for (int i = 0; i < 3; ++i)
        {
            bool active = (int)physMode == i;
            auto tabR = juce::Rectangle<float>(i * tabW, 0.0f, tabW, kTabH);
            g.setColour(juce::Colour(active ? kTabActive : kTabInactive));
            g.fillRect(tabR);
            g.setColour(juce::Colour(kTabBorder));
            g.drawRect(tabR, 0.5f);
            g.setFont(juce::Font(7.0f).boldened());
            g.setColour(juce::Colour(active ? kTabActiveTxt : kTabInactTxt));
            g.drawText(tabLabels[i], tabR, juce::Justification::centred);
        }

        // ── Orbit area (below tab strip) ─────────────────────────────────
        auto orbitBounds = getLocalBounds().withTrimmedTop((int)kTabH).toFloat();
        float cx = orbitBounds.getCentreX();
        float cy = orbitBounds.getCentreY();
        float radius = kOrbitDiameter * 0.5f;

        // Background
        g.setColour(juce::Colour(kSurfaceBg));
        g.fillRect(orbitBounds);

        // Boundary ring
        g.setColour(juce::Colours::white.withAlpha(0.1f));
        g.drawEllipse(cx - radius, cy - radius, radius * 2, radius * 2, 1.0f);

        // Center crosshair
        g.setColour(juce::Colours::white.withAlpha(0.06f));
        g.drawHorizontalLine((int)cy, cx - radius, cx + radius);
        g.drawVerticalLine((int)cx, cy - radius, cy + radius);

        // Trail
        for (int i = 0; i < kOrbitTrailSize; ++i)
        {
            int idx = (trailHead - i + kOrbitTrailSize) % kOrbitTrailSize;
            if (trail[idx].x == 0.0f && trail[idx].y == 0.0f && i > 0) continue;
            float alpha = (1.0f - (float)i / kOrbitTrailSize) * 0.6f;
            float size = 2.0f + 4.0f * (1.0f - (float)i / kOrbitTrailSize);
            float tx = cx + trail[idx].x * radius;
            float ty = cy + trail[idx].y * radius;
            g.setColour(juce::Colour(kAmber).withAlpha(alpha));
            g.fillEllipse(tx - size * 0.5f, ty - size * 0.5f, size, size);
        }

        // Cursor
        float cursorX = cx + posX * radius;
        float cursorY = cy + posY * radius;

        // Glow
        g.setColour(juce::Colour(kAmber).withAlpha(0.15f));
        g.fillEllipse(cursorX - 16, cursorY - 16, 32, 32);
        // Cursor dot
        g.setColour(juce::Colour(kAmber));
        g.fillEllipse(cursorX - 6, cursorY - 6, 12, 12);

        // ── LOCK mode: amber anchor dot at center ─────────────────────────
        if (physMode == PhysicsMode::Lock)
        {
            g.setColour(juce::Colour(kTabActive).withAlpha(0.9f));
            g.fillEllipse(cx - 4.0f, cy - 4.0f, 8.0f, 8.0f);
            g.setColour(juce::Colour(kTabActive).withAlpha(0.35f));
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
                g.setColour(juce::Colour(kTabActive).withAlpha(0.4f));
                g.strokePath(springPath, stroke);
                // Small spring-return target dot at center
                g.setColour(juce::Colour(kTabActive).withAlpha(0.6f));
                g.fillEllipse(cx - 3.0f, cy - 3.0f, 6.0f, 6.0f);
            }
        }

        // Axis labels
        g.setColour(juce::Colour(kTextDim).withAlpha(0.5f));
        g.setFont(juce::Font(7.0f));
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
        float radius = PS::kOrbitDiameter * 0.5f;

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
    enum class StripMode { DubSpace, FilterSweep, Coupling, DubSiren };

    std::function<void(float x, float y)> onPositionChanged;

    void setStripMode(StripMode m)
    {
        stripMode = m;
        repaint();
    }

    void paint(juce::Graphics& g) override
    {
        using namespace PS;
        auto b = getLocalBounds().toFloat();

        // Background gradient per mode
        switch (stripMode)
        {
            case StripMode::DubSpace:
                g.setGradientFill(juce::ColourGradient(
                    juce::Colour(kTerracotta), b.getX(), b.getCentreY(),
                    juce::Colour(kTeal), b.getRight(), b.getCentreY(), false));
                break;
            case StripMode::FilterSweep:
                g.setColour(juce::Colour(kSurfaceCard));
                break;
            case StripMode::Coupling:
            {
                auto mid = b.getCentreX();
                g.setGradientFill(juce::ColourGradient(
                    juce::Colour(kTerracotta), b.getX(), b.getCentreY(),
                    juce::Colour(kTeal), b.getRight(), b.getCentreY(), false));
                break;
            }
            case StripMode::DubSiren:
                g.setColour(juce::Colour(kSurfaceCard));
                break;
        }
        g.setOpacity(0.3f);
        g.fillRect(b);
        g.setOpacity(1.0f);

        // Gesture trail
        for (int i = 0; i < kStripTrailSize; ++i)
        {
            int idx = (stripTrailHead - i + kStripTrailSize) % kStripTrailSize;
            auto& pt = stripTrail[idx];
            if (pt.age > kWarmMemoryDur) continue;
            float alpha = (1.0f - pt.age / kWarmMemoryDur) * 0.5f;
            float sx = b.getX() + pt.x * b.getWidth();
            float sy = b.getY() + (1.0f - pt.y) * b.getHeight();
            g.setColour(juce::Colour(kAmber).withAlpha(alpha));
            g.fillEllipse(sx - 2, sy - 2, 4, 4);
        }

        // Cursor
        float cx = b.getX() + stripX * b.getWidth();
        float cy = b.getY() + (1.0f - stripY) * b.getHeight();
        g.setColour(juce::Colour(kAmber).withAlpha(0.2f));
        g.fillEllipse(cx - 12, cy - 12, 24, 24);
        g.setColour(juce::Colour(kAmber));
        g.fillEllipse(cx - 5, cy - 5, 10, 10);

        // Mode label
        g.setColour(juce::Colour(kTextLight).withAlpha(0.5f));
        g.setFont(juce::Font(8.0f));
        static const char* modeNames[] = { "DUB SPACE", "FILTER SWEEP", "COUPLING", "DUB SIREN" };
        g.drawText(modeNames[(int)stripMode], b.reduced(4), juce::Justification::topLeft);

        // Axis labels
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
    std::function<void()> onFire;
    std::function<void(bool held)> onXoSend;
    std::function<void(bool held)> onEchoCut;
    std::function<void()> onPanic;

    void paint(juce::Graphics& g) override
    {
        using namespace PS;
        auto b = getLocalBounds().toFloat();
        float padH = b.getHeight() / 4.0f;

        struct PadDef { const char* label; uint32_t color; };
        static const PadDef pads[] = {
            { "FIRE",     kFireGreen },
            { "XOSEND",   kAmber },
            { "ECHO CUT", kAmber },
            { "PANIC",    kPanicRed },
        };

        for (int i = 0; i < 4; ++i)
        {
            auto rect = juce::Rectangle<float>(b.getX(), b.getY() + i * padH, b.getWidth(), padH).reduced(3.0f);
            auto col = juce::Colour(pads[i].color);
            bool pressed = padStates[i];

            g.setColour(pressed ? col : col.withAlpha(0.15f));
            g.fillRoundedRectangle(rect, 6.0f);
            g.setColour(col.withAlpha(pressed ? 1.0f : 0.5f));
            g.drawRoundedRectangle(rect, 6.0f, 2.0f);

            g.setColour(pressed ? juce::Colours::black : col);
            g.setFont(juce::Font(10.0f).boldened());
            g.drawText(pads[i].label, rect, juce::Justification::centred);

            // Keyboard hint
            static const char* keys[] = { "Z", "X", "C", "V" };
            g.setColour(col.withAlpha(0.3f));
            g.setFont(juce::Font(7.0f));
            g.drawText(keys[i], rect.reduced(4), juce::Justification::bottomRight);
        }
    }

    void mouseDown(const juce::MouseEvent& e) override
    {
        int pad = padFromY(e.y);
        if (pad < 0 || pad > 3) return;
        padStates[(size_t)pad] = true;
        firePad(pad, true);
        repaint();
    }

    void mouseUp(const juce::MouseEvent& e) override
    {
        int pad = padFromY(e.y);
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

        startTimerHz(30);
    }

    ~PlaySurface() override { stopTimer(); }

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

        // Strip mode buttons at right of header
        for (int i = 3; i >= 0; --i)
            stripModeButtons[i].setBounds(header.removeFromRight(48).reduced(2));

        // Bottom strip
        auto stripArea = area.removeFromBottom(PS::kZone3H);
        strip.setBounds(stripArea);

        // Main zones (left to right)
        auto mainArea = area;
        // Scale zones proportionally
        float totalW = (float)mainArea.getWidth();
        float z1frac = (float)PS::kZone1W / (PS::kZone1W + PS::kZone2W + PS::kZone4W);
        float z2frac = (float)PS::kZone2W / (PS::kZone1W + PS::kZone2W + PS::kZone4W);

        int z1w = (int)(totalW * z1frac);
        int z4w = (int)(totalW * (1.0f - z1frac - z2frac));

        noteInput.setBounds(mainArea.removeFromLeft(z1w));
        perfPads.setBounds(mainArea.removeFromRight(z4w));
        orbitPath.setBounds(mainArea); // center gets the rest
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colour(PS::kSurfaceBg));
    }

    bool keyPressed(const juce::KeyPress& key) override
    {
        return perfPads.handleKey(key, true);
    }

private:
    void timerCallback() override
    {
        noteInput.tick();
        orbitPath.tick();
        strip.tick();
    }

    NoteInputZone      noteInput;
    OrbitPathZone      orbitPath;
    PerformanceStrip   strip;
    PerformancePads    perfPads;

    std::array<juce::TextButton, 3> modeButtons;
    std::array<juce::TextButton, 4> stripModeButtons;
    juce::TextButton octDownBtn, octUpBtn;
    juce::Label      octLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PlaySurface)
};

} // namespace xomnibus
