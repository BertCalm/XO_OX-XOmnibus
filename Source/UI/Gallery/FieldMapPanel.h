#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "../GalleryColors.h"

namespace xolokun
{

//==============================================================================
// FieldMapPanel — sonic cartography of the current session.
//
// Every note played deposits a glowing point in a 2D canvas:
//   X = session time (recent = right, scrolling left continuously)
//   Y = pitch (C1=bottom, C7=top), with depth zone bands as background
//
// Points: radius proportional to velocity, color from engine accent,
// alpha = 1 - (age / 240s). Fades over 4 minutes.
// Background: warm dark wash with Sunlit/Twilight/Midnight zone bands.
// Live cursor: thin vertical line at right edge (current moment).
//
class FieldMapPanel : public juce::Component, private juce::Timer
{
public:
    struct NoteEvent {
        float sessionTimeS;     // seconds since session start
        float normalizedPitch;  // 0.0 (C1) to 1.0 (C7)
        float velocity;         // 0.0–1.0, drives dot radius
        juce::Colour engineColor;
    };

    FieldMapPanel()
    {
        sessionStart = juce::Time::getCurrentTime();
        A11y::setup(*this, "Field Map", "Shows note activity as a visual heat map", false);
        // A11Y05: respect reduced-motion preference — drop to 1Hz refresh when active
        if (A11y::prefersReducedMotion())
            startTimerHz(1);
        else
            startTimerHz(30);
    }

    ~FieldMapPanel() override { stopTimer(); }

    // Called from the message thread (timer drain) when a note fires.
    void addNote(int midiNote, float velocity, juce::Colour engineColor)
    {
        NoteEvent ev;
        ev.sessionTimeS    = static_cast<float>((juce::Time::getCurrentTime() - sessionStart).inSeconds());
        ev.normalizedPitch = juce::jmap(static_cast<float>(juce::jlimit(24, 96, midiNote)), 24.0f, 96.0f, 0.0f, 1.0f);
        ev.velocity        = velocity;
        ev.engineColor     = engineColor;

        // Append to ring buffer (overwrite oldest)
        events[headIdx % kMaxEvents] = ev;
        ++headIdx;
    }

    void timerCallback() override { if (!isVisible()) return; repaint(); }

    void paint(juce::Graphics& g) override
    {
        auto b = getLocalBounds().toFloat();
        float now = static_cast<float>((juce::Time::getCurrentTime() - sessionStart).inSeconds());
        const float kWindowS = 240.0f; // 4-minute display window

        // ── Background — dark wash with zone bands ───────────────────────────
        g.setColour(juce::Colour(0xFF0D0D1A));
        g.fillRect(b);

        // Zone bands (bottom=Midnight, top=Sunlit — matching XOlokun ecology)
        struct ZoneBand { float yNormStart, yNormEnd; juce::uint32 rgba; float alpha; };
        const ZoneBand bands[] = {
            { 0.80f, 1.00f, 0xFF48CAE4, 0.04f },  // Sunlit   — top 20%
            { 0.45f, 0.80f, 0xFF0096C7, 0.03f },  // Twilight — middle
            { 0.00f, 0.45f, 0xFF7B2FBE, 0.04f },  // Midnight — bottom
        };
        for (const auto& band : bands)
        {
            float yTop    = b.getBottom() - band.yNormEnd   * b.getHeight();
            float yBottom = b.getBottom() - band.yNormStart * b.getHeight();
            g.setColour(juce::Colour(band.rgba).withAlpha(band.alpha));
            g.fillRect(b.getX(), yTop, b.getWidth(), yBottom - yTop);
        }

        // ── Zone boundary lines ───────────────────────────────────────────────
        auto drawBand = [&](float yNorm, juce::uint32 col) {
            float y = b.getBottom() - yNorm * b.getHeight();
            g.setColour(juce::Colour(col).withAlpha(0.12f));
            g.drawHorizontalLine(static_cast<int>(y), b.getX(), b.getRight());
        };
        drawBand(0.80f, 0xFF48CAE4);
        drawBand(0.45f, 0xFF0096C7);

        // ── Note points ──────────────────────────────────────────────────────
        int total = static_cast<int>(std::min(headIdx, kMaxEvents));
        for (int i = 0; i < total; ++i)
        {
            const auto& ev = events[i % kMaxEvents];
            float age = now - ev.sessionTimeS;
            if (age > kWindowS || age < 0.0f) continue;

            float alpha = 1.0f - (age / kWindowS);
            alpha = alpha * alpha; // quadratic fade — lingers bright then drops off
            if (alpha < 0.02f) continue;

            // X: map session time within window to pixel X
            float xNorm = (ev.sessionTimeS - (now - kWindowS)) / kWindowS;
            float x     = b.getX() + xNorm * b.getWidth();
            // Y: pitch (0=bottom C1, 1=top C7)
            float y     = b.getBottom() - ev.normalizedPitch * b.getHeight();
            // Radius: 2.5 base + velocity * 4
            float r     = 2.5f + ev.velocity * 4.0f;

            // Glow pass (wider, dimmer)
            g.setColour(ev.engineColor.withAlpha(alpha * 0.15f));
            g.fillEllipse(x - r * 2.0f, y - r * 2.0f, r * 4.0f, r * 4.0f);
            // Core dot
            g.setColour(ev.engineColor.withAlpha(alpha * 0.80f));
            g.fillEllipse(x - r, y - r, r * 2.0f, r * 2.0f);
        }

        // ── Live time cursor ─────────────────────────────────────────────────
        float cursorX = b.getRight() - 1.0f;
        g.setColour(juce::Colour(0xFFE9C46A).withAlpha(0.40f)); // XO Gold
        g.drawVerticalLine(static_cast<int>(cursorX), b.getY(), b.getBottom());

        // ── Label ────────────────────────────────────────────────────────────
        g.setFont(GalleryFonts::body(8.0f));
        g.setColour(juce::Colours::white.withAlpha(0.20f));
        g.drawText("FIELD MAP", b.reduced(6.0f, 4.0f), juce::Justification::topLeft);
    }

private:
    static constexpr size_t kMaxEvents = 4096;
    std::array<NoteEvent, kMaxEvents> events {};
    size_t headIdx = 0;
    juce::Time sessionStart;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FieldMapPanel)
};

} // namespace xolokun
