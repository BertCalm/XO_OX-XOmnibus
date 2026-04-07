// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once

//==============================================================================
// NexusDisplay — Center identity hub for the Ocean View radial layout.
//
// Renders the current preset's sonic identity in three stacked layers:
//   1. DnaHexagon (96×96) — 6D Sonic DNA profile at full gallery size.
//   2. Preset name — 18px Space Grotesk Bold, XO Gold with a subtle glow.
//   3. Mood badge  — inline pill, mood colour at 12% alpha fill.
//
// Callbacks:
//   onPresetNameClicked — user clicked the name label (open DNA browser).
//   onDnaClicked        — user clicked the hexagon (cycle axis projection).
//
// Preferred height: ~142px  (96 hex + 8 gap + 22 name + 4 gap + 18 badge).
// Width is unconstrained; components are always horizontally centred.
//==============================================================================

#include <juce_gui_basics/juce_gui_basics.h>
#include "../GalleryColors.h"
#include "../Gallery/DnaHexagon.h"

namespace xoceanus
{

class NexusDisplay : public juce::Component
{
public:
    //==========================================================================
    NexusDisplay()
    {
        // Ghost hexagons must be added BEFORE dnaHex_ so they paint behind it.
        addAndMakeVisible(ghostHex60_);
        ghostHex60_.setInterceptsMouseClicks(false, false);
        ghostHex60_.setAlpha(0.05f);

        addAndMakeVisible(ghostHex30_);
        ghostHex30_.setInterceptsMouseClicks(false, false);
        ghostHex30_.setAlpha(0.10f);

        addAndMakeVisible(dnaHex_);
        dnaHex_.setAccentColor(accentColour_);

        // #1007 FIX 2: NexusDisplay must be keyboard-reachable (WCAG 2.1.1).
        // wantsKeyFocus=true lets Tab navigation reach it and enables the
        // Space/Return/D key handlers below.
        A11y::setup(*this,
                    "Preset Identity",
                    "Current preset DNA, name, and mood. "
                    "Press Space or Return to browse presets. "
                    "Press D to cycle DNA axis projection.",
                    /*wantsKeyFocus=*/true);
    }

    //==========================================================================
    // juce::Component overrides
    //==========================================================================

    void paint(juce::Graphics& g) override
    {
        // ── Preset name ───────────────────────────────────────────────────────
        // #1008 FIX 2: Multi-pass glow effect.
        // 4 offset passes at ±1px with decreasing alpha, then one sharp pass
        // on top.  This produces a real XO-Gold halo rather than a faux drop
        // shadow.  Offsets: (+1,+1), (-1,-1), (+1,-1), (-1,+1) at 20% alpha,
        // followed by the opaque sharp foreground.
        if (presetNameBounds_.getWidth() > 0)
        {
            const juce::Font nameFont = GalleryFonts::display(kPresetFontSize);
            g.setFont(nameFont);

            // Glow passes: 4 offset draws at 20% alpha to simulate a halo.
            const juce::Colour glowColour = accentColour_.withAlpha(0.20f);
            g.setColour(glowColour);
            static constexpr int kGlowOffsets[4][2] = { {1, 1}, {-1, -1}, {1, -1}, {-1, 1} };
            for (const auto& off : kGlowOffsets)
            {
                g.drawText(presetName_,
                           presetNameBounds_.translated(off[0], off[1]),
                           juce::Justification::centred,
                           false);
            }

            // Sharp foreground pass on top.
            g.setColour(accentColour_);
            g.drawText(presetName_,
                       presetNameBounds_,
                       juce::Justification::centred,
                       false);
        }

        // ── Mood badge ────────────────────────────────────────────────────────
        if (moodBadgeBounds_.getWidth() > 0)
        {
            const auto pillRect = moodBadgeBounds_.toFloat();

            // Fill: mood colour at 12% alpha
            g.setColour(moodColour_.withAlpha(kMoodPillAlpha));
            g.fillRoundedRectangle(pillRect, pillRect.getHeight() * 0.5f);

            // Mood name text at full opacity
            g.setFont(GalleryFonts::label(kMoodFontSize));
            g.setColour(moodColour_);
            g.drawText(moodName_,
                       moodBadgeBounds_,
                       juce::Justification::centred,
                       false);
        }

        // ── #909: Live parameter readouts ─────────────────────────────────────
        // Voice count + 4 macro bar gauges, drawn below the mood badge.
        // Rendered at low opacity so they are informational without dominating
        // the identity visual hierarchy.
        if (liveReadoutsBounds_.getWidth() > 0)
        {
            const int w = getWidth();
            const auto rb = liveReadoutsBounds_;

            // Voice count pill — "Vx" where x is the polyphonic voice count.
            g.setFont(GalleryFonts::value(9.0f));
            const juce::Colour readoutCol = accentColour_.withAlpha(0.55f);
            g.setColour(readoutCol);
            juce::String voiceStr = (voiceCount_ > 0)
                                        ? ("V" + juce::String(voiceCount_))
                                        : "V0";
            g.drawText(voiceStr,
                       juce::Rectangle<int>(rb.getX(), rb.getY(), 28, rb.getHeight()),
                       juce::Justification::centredLeft,
                       false);

            // Macro bar gauges: 4 tiny horizontal bars (M1-M4) filling the remainder.
            // Each bar is 8pt tall, 2pt gap between bars, label "M1"..."M4" on left.
            const int barAreaX = rb.getX() + 32;
            const int barAreaW = juce::jmax(0, w - barAreaX * 2); // symmetric margins
            const int barH = 8;   // increased from 4px for legibility
            const int barGapV = 3;
            const int totalBarH = 4 * barH + 3 * barGapV;
            const int barStartY = rb.getCentreY() - totalBarH / 2;

            for (int i = 0; i < 4; ++i)
            {
                const int barY = barStartY + i * (barH + barGapV);
                const float val = juce::jlimit(0.0f, 1.0f, macroValues_[i]);

                // Track background
                g.setColour(accentColour_.withAlpha(0.10f));
                g.fillRoundedRectangle(juce::Rectangle<float>(
                    (float)barAreaX, (float)barY, (float)barAreaW, (float)barH), 2.0f);

                // Fill proportional to value
                if (val > 0.001f)
                {
                    g.setColour(accentColour_.withAlpha(0.45f));
                    g.fillRoundedRectangle(juce::Rectangle<float>(
                        (float)barAreaX, (float)barY,
                        val * (float)barAreaW, (float)barH), 2.0f);
                }
            }
        }

        // #1007 FIX 2: Visible focus ring for keyboard users (WCAG 2.1.1).
        if (hasKeyboardFocus(false))
            A11y::drawFocusRing(g, getLocalBounds().toFloat(), 4.0f);
    }

    void resized() override
    {
        const auto bounds = getLocalBounds();
        const int w = bounds.getWidth();

        // ── DNA Hexagon + ghost trails: all share the same bounds ────────────
        const int hexX = (w - kDnaSize) / 2;
        ghostHex60_.setBounds(hexX, 0, kDnaSize, kDnaSize);
        ghostHex30_.setBounds(hexX, 0, kDnaSize, kDnaSize);
        dnaHex_.setBounds(hexX, 0, kDnaSize, kDnaSize);

        int y = kDnaSize + kGapHexName;

        // ── Preset name bounds ────────────────────────────────────────────────
        // Measure text width so clicks hit accurately; cap at component width.
        const juce::Font nameFont = GalleryFonts::display(kPresetFontSize);
        const int nameTextW = juce::jmin(
            static_cast<int>(std::ceil(nameFont.getStringWidthFloat(presetName_))) + 2,
            w);
        const int nameH = static_cast<int>(std::ceil(kPresetFontSize)) + 4;
        presetNameBounds_ = juce::Rectangle<int>((w - nameTextW) / 2, y, nameTextW, nameH);
        y += nameH + kGapNameMood;

        // ── Mood badge bounds ─────────────────────────────────────────────────
        // Pill width = text width + horizontal padding on each side.
        const juce::Font moodFont = GalleryFonts::label(kMoodFontSize);
        const int moodTextW = static_cast<int>(std::ceil(moodFont.getStringWidthFloat(moodName_)));
        const int pillW = moodTextW + 2 * kMoodPillPadH;
        const int pillH = static_cast<int>(std::ceil(kMoodFontSize)) + 2 * kMoodPillPadV;
        moodBadgeBounds_ = juce::Rectangle<int>((w - pillW) / 2, y, pillW, pillH);
        y += pillH + kGapMoodReadout;

        // ── #909: Live readouts strip — voice count pill + 4 macro bars ───────
        // Height = 4 bars × 8pt + 3 gaps × 3pt = 41pt total
        constexpr int kReadoutH = 41;
        liveReadoutsBounds_ = juce::Rectangle<int>(0, y, w, kReadoutH);
    }

    void mouseDown(const juce::MouseEvent& e) override
    {
        grabKeyboardFocus();

        // Hit-test hexagon first (it owns the top region).
        if (dnaHex_.getBounds().contains(e.getPosition()))
        {
            if (onDnaClicked)
                onDnaClicked();
            return;
        }

        // Then test the preset name label.
        if (presetNameBounds_.contains(e.getPosition()))
        {
            if (onPresetNameClicked)
                onPresetNameClicked();
        }
    }

    // #1007 FIX 2: Keyboard handler so the component is WCAG 2.1.1 compliant.
    // Space/Return → open preset browser (same as clicking the name label).
    // D            → cycle DNA axis projection (same as clicking the hexagon).
    bool keyPressed(const juce::KeyPress& key) override
    {
        if (key == juce::KeyPress::spaceKey || key == juce::KeyPress::returnKey)
        {
            if (onPresetNameClicked)
                onPresetNameClicked();
            return true;
        }

        if (key == juce::KeyPress('d') || key == juce::KeyPress('D'))
        {
            if (onDnaClicked)
                onDnaClicked();
            return true;
        }

        return false;
    }

    // #1007 FIX 2: Focus change triggers repaint so the focus ring appears/disappears.
    void focusGained(FocusChangeType) override { repaint(); }
    void focusLost(FocusChangeType)   override { repaint(); }

    //==========================================================================
    // Data setters
    //==========================================================================

    void setPresetName(const juce::String& name)
    {
        if (presetName_ == name)
            return;
        presetName_ = name;
        resized();   // re-measure name bounds
        repaint();
    }

    void setMoodName(const juce::String& mood)
    {
        if (moodName_ == mood)
            return;
        moodName_ = mood;
        resized();   // re-measure pill bounds
        repaint();
    }

    void setMoodColour(juce::Colour colour)
    {
        moodColour_ = colour;
        repaint();
    }

    void setDNA(float brightness, float warmth, float movement,
                float density, float space, float aggression)
    {
        // Cache preset DNA then blend with session drift before forwarding.
        presetDna_[0] = brightness;
        presetDna_[1] = warmth;
        presetDna_[2] = movement;
        presetDna_[3] = density;
        presetDna_[4] = space;
        presetDna_[5] = aggression;
        blendAndApplyDna();
    }

    // Feed a note-on event into the leaky-integrator that accumulates session DNA.
    // pitch:    raw MIDI note number 0-127
    // velocity: normalised 0.0-1.0
    // interval: absolute semitone distance from the previous note (pass 0 for first note)
    void updateSessionDna(float pitch, float velocity, float interval)
    {
        // Normalise pitch: high notes → high brightness value
        const float pitchNorm    = pitch / 127.0f;
        const float intervalNorm = std::min(interval / 24.0f, 1.0f);

        // Leaky integrator: 98% old state + 2% new input per note
        constexpr float kLeak  = 0.98f;
        constexpr float kInput = 0.02f;

        sessionDna_[0] = kLeak * sessionDna_[0] + kInput * pitchNorm;            // brightness
        sessionDna_[1] = kLeak * sessionDna_[1] + kInput * (1.0f - pitchNorm);   // warmth (inverse brightness)
        sessionDna_[2] = kLeak * sessionDna_[2] + kInput * velocity;             // movement
        sessionDna_[3] = kLeak * sessionDna_[3] + kInput * velocity;             // density
        sessionDna_[4] = kLeak * sessionDna_[4] + kInput * intervalNorm;         // space
        sessionDna_[5] = kLeak * sessionDna_[5] + kInput * velocity * 0.8f;      // aggression

        blendAndApplyDna();
    }

    void setAccentColour(juce::Colour accent)
    {
        accentColour_ = accent;
        dnaHex_.setAccentColor(accent);
        repaint();
    }

    // #909: Live parameter feedback — voice count and active macro values.
    // Call from the editor's timer (at ~10 Hz) to keep readouts current.
    // voiceCount: total polyphonic voices across all loaded engines.
    // macroValues[4]: current normalised [0,1] value for macros 1-4.
    void setLiveReadouts(int voiceCount, const std::array<float, 4>& macroValues)
    {
        bool changed = (voiceCount != voiceCount_);
        for (int i = 0; i < 4; ++i)
            if (std::abs(macroValues[i] - macroValues_[i]) > 0.005f)
                changed = true;

        if (!changed)
            return;

        voiceCount_ = voiceCount;
        macroValues_ = macroValues;
        repaint();
    }

    // Feature 6 (Schulze): Sustained-voice DNA accumulation.
    // Call from the editor timer to store the current polyphonic voice count.
    void setSustainedVoiceCount(int count)
    {
        sustainedVoiceCount_ = count;
    }

    // Feature 6 (Schulze): Drift sessionDna_ toward presetDna_ at 0.001/sec
    // while any voices are held. Call from the editor timer (e.g. dtSeconds=0.1
    // at 10 Hz). Has no effect when no voices are sounding.
    void tickSustainedDna(float dtSeconds)
    {
        if (sustainedVoiceCount_ <= 0)
            return;

        for (int i = 0; i < 6; ++i)
            sessionDna_[i] += (presetDna_[i] - sessionDna_[i]) * 0.001f * dtSeconds;

        blendAndApplyDna();
    }

    //==========================================================================
    // Callbacks
    //==========================================================================

    /** Fired when the user clicks the preset name label. Use to open the DNA browser. */
    std::function<void()> onPresetNameClicked;

    /** Fired when the user clicks the DNA hexagon. Use to cycle axis projections. */
    std::function<void()> onDnaClicked;

private:
    //==========================================================================
    // Child components
    //==========================================================================

    // Feature 5 (Tomita): Ghost trail hexagons — added before dnaHex_ so they
    // render behind it. ghostHex30_ = 30s ago (10% alpha), ghostHex60_ = 60s ago (5% alpha).
    DnaHexagon ghostHex30_;
    DnaHexagon ghostHex60_;

    DnaHexagon dnaHex_;

    //==========================================================================
    // State
    //==========================================================================

    juce::String  presetName_  = "Init";
    juce::String  moodName_    = "Foundation";
    juce::Colour  moodColour_  = juce::Colour(0xFF9E9B97);  // Ocean::salt
    juce::Colour  accentColour_ = juce::Colour(GalleryColors::xoGold);

    // #909: Live parameter readouts — updated via setLiveReadouts()
    int voiceCount_ = 0;
    std::array<float, 4> macroValues_{{0.0f, 0.0f, 0.0f, 0.0f}};

    // Live DNA drift: session accumulator and blend weight.
    // sessionDna_  — leaky integrator over note-on events this session.
    // presetDna_   — last raw preset values passed to setDNA().
    // kSessionBlend — 35% session character, 65% preset identity.
    static constexpr float kSessionBlend = 0.35f;
    std::array<float, 6> sessionDna_ = {0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f};
    std::array<float, 6> presetDna_  = {0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f};

    // Feature 5 (Tomita): DNA drift trail history.
    // dnaHistory_[0] = blended DNA ~30s ago, dnaHistory_[1] = ~60s ago.
    std::array<std::array<float, 6>, 2> dnaHistory_
        {{{{0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f}},
          {{0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f}}}};
    double lastHistorySnapshotMs_ = 0.0;

    // Feature 6 (Schulze): Sustained-voice count for continuous DNA drift.
    int sustainedVoiceCount_ = 0;

    // Cached layout rects — computed in resized().
    juce::Rectangle<int> presetNameBounds_;
    juce::Rectangle<int> moodBadgeBounds_;
    juce::Rectangle<int> liveReadoutsBounds_; // #909: voice count + macro bars

    //==========================================================================
    // Layout constants
    //==========================================================================

    static constexpr int   kDnaSize        = 96;
    static constexpr int   kGapHexName     = 8;
    static constexpr int   kGapNameMood    = 4;
    static constexpr int   kGapMoodReadout = 6;  // #909: gap between mood badge and live readouts
    static constexpr float kPresetFontSize = 18.0f;
    static constexpr float kMoodFontSize   = 11.0f;
    static constexpr float kMoodPillAlpha  = 0.12f;
    static constexpr int   kMoodPillPadH   = 6;   // horizontal padding inside pill
    static constexpr int   kMoodPillPadV   = 2;   // vertical padding inside pill

    //==========================================================================
    // Helpers
    //==========================================================================

    // Blend preset + session DNA at kSessionBlend ratio and push to DnaHexagon.
    // Feature 5 (Tomita): every 30 seconds, rotate the blended value into
    // dnaHistory_ and update the ghost hexagons.
    void blendAndApplyDna()
    {
        std::array<float, 6> blended;
        for (int i = 0; i < 6; ++i)
            blended[i] = presetDna_[i] * (1.0f - kSessionBlend)
                       + sessionDna_[i] * kSessionBlend;

        dnaHex_.setDNA(blended[0], blended[1], blended[2],
                       blended[3], blended[4], blended[5]);
        // DnaHexagon calls repaint() internally.

        // Feature 5 (Tomita): rotate history snapshot every 30 seconds.
        const double nowMs = juce::Time::getMillisecondCounterHiRes();
        if (lastHistorySnapshotMs_ == 0.0)
            lastHistorySnapshotMs_ = nowMs;

        if (nowMs - lastHistorySnapshotMs_ >= 30000.0)
        {
            // Rotate: slot[1] (60s ago) ← slot[0] (30s ago) ← current blended
            dnaHistory_[1] = dnaHistory_[0];
            dnaHistory_[0] = blended;
            lastHistorySnapshotMs_ = nowMs;

            // Push historical DNA values into the ghost hexagons.
            const auto& h30 = dnaHistory_[0];
            ghostHex30_.setDNA(h30[0], h30[1], h30[2], h30[3], h30[4], h30[5]);
            const auto& h60 = dnaHistory_[1];
            ghostHex60_.setDNA(h60[0], h60[1], h60[2], h60[3], h60[4], h60[5]);
        }
    }

    //==========================================================================

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NexusDisplay)
};

} // namespace xoceanus
