#pragma once
// ABCompare.h — A/B Preset Comparison widget for the XOlokun header.
//
// Two small toggle buttons (A and B) that let the user instantly switch between
// two stored preset states — a pro workflow feature for comparing sound design
// choices without touching the preset browser.
//
// Layout (56 × 24pt total):
//   [ A ][ B ]     4pt gap between buttons, each button 24 × 24pt
//
// State machine:
//   Inactive           — not yet entered; both buttons dim, dashed border.
//   A active           — A button lit XO Gold; B lit silver if stored, else dashed.
//   B active           — B button lit silver; A lit XO Gold.
//
// Interaction:
//   First click on A or B → enters A/B mode, captures current state into A,
//                            stores nothing in B yet.
//   Load any preset while A/B mode is active → always captured into B.
//   Click B            → restores stateB (if stored), shows B.
//   Click A            → restores stateA, shows A.
//   Click currently-active button → deactivates A/B mode entirely.
//
// Integration:
//   Call onPresetLoaded() from the editor whenever a preset is loaded so that
//   the widget can capture the new state into slot B (the comparison slot).
//   Do NOT call this from the constructor.
//
// Architecture constraints:
//   • Header-only (.h) — no .cpp
//   • Depends on GalleryColors.h (GalleryColors, GalleryFonts, A11y namespaces)
//   • Depends on XOlokunProcessor.h (getStateInformation / setStateInformation)
//   • State capture/restore is synchronous on the message thread
//   • No memory allocation during capture beyond MemoryBlock reallocation

#include <juce_audio_processors/juce_audio_processors.h>
#include "../../XOlokunProcessor.h"
#include "../GalleryColors.h"

namespace xolokun {

//==============================================================================
class ABCompare : public juce::Component
{
public:
    //==========================================================================
    explicit ABCompare(XOlokunProcessor& proc)
        : processor(proc)
    {
        A11y::setup(*this,
                    "A/B Compare",
                    "Compare two preset states. Click A or B to enter compare mode.",
                    /*wantsKeyFocus=*/true);
    }

    ~ABCompare() override = default;

    //==========================================================================
    // Call from the editor whenever a new preset has been fully loaded.
    // Always captures the fresh processor state into B (the "comparison" slot).
    // A is the fixed "reference" snapshot taken when entering A/B mode.
    // No-op if A/B mode is not active.
    void onPresetLoaded()
    {
        if (!abActive)
            return;

        captureState(stateB);

        repaint();
    }

    bool isActive() const noexcept { return abActive; }

    //==========================================================================
    void paint(juce::Graphics& g) override
    {
        using namespace GalleryColors;

        const float cornerR = 4.0f;

        // ── A button ─────────────────────────────────────────────────────────
        paintButton(g, btnABounds, "A",
                    /*isActive=*/abActive && showingA,
                    /*isStored=*/stateA.getSize() > 0,
                    /*isA=*/true,
                    cornerR);

        // ── B button ─────────────────────────────────────────────────────────
        paintButton(g, btnBBounds, "B",
                    /*isActive=*/abActive && !showingA,
                    /*isStored=*/stateB.getSize() > 0,
                    /*isA=*/false,
                    cornerR);

        // ── Focus ring (WCAG 2.4.7) ──────────────────────────────────────────
        if (hasKeyboardFocus(true))
            A11y::drawFocusRing(g, getLocalBounds().toFloat(), cornerR);
    }

    void resized() override
    {
        // Total: 56 × 24pt.  A: 0-23, gap: 24-27, B: 28-55.
        // If the component is sized differently, scale proportionally.
        auto b = getLocalBounds();
        const int gap    = 4;
        const int btnW   = (b.getWidth() - gap) / 2;
        const int btnH   = b.getHeight();

        btnABounds = b.removeFromLeft(btnW).toFloat();
        b.removeFromLeft(gap);
        btnBBounds = b.toFloat();

        // Use the actual integer height for the B bounds
        btnBBounds = { btnABounds.getRight() + (float)gap,
                       0.0f,
                       (float)btnW,
                       (float)btnH };
    }

    void mouseDown(const juce::MouseEvent& e) override
    {
        const bool clickedA = btnABounds.contains(e.position);
        const bool clickedB = btnBBounds.contains(e.position);

        if (!clickedA && !clickedB)
            return;

        handleButtonAction(clickedA, clickedB);
        repaint();
    }

    // Space / Enter key toggles between A and B when focused.
    bool keyPressed(const juce::KeyPress& key) override
    {
        if (key == juce::KeyPress::spaceKey || key == juce::KeyPress::returnKey)
        {
            // Drive the same logic as mouseDown without constructing a MouseEvent.
            // Choose the target: if showing A, act as if the user clicked B, and vice versa.
            handleButtonAction(/*clickedA=*/!showingA, /*clickedB=*/showingA);
            repaint();
            return true;
        }
        return false;
    }

private:
    //==========================================================================
    // Core interaction logic — shared by mouseDown() and keyPressed().
    void handleButtonAction(bool clickedA, bool clickedB)
    {
        if (!abActive)
        {
            // ── Enter A/B mode ────────────────────────────────────────────────
            // Capture the current processor state into A (the "before" slot).
            abActive = true;
            captureState(stateA);
            stateB.reset();   // B is empty until the user loads a different preset
            showingA = true;
        }
        else if (clickedA)
        {
            if (showingA)
            {
                // Clicking the already-active slot → deactivate A/B mode.
                deactivate();
            }
            else
            {
                // Switch to A.
                showingA = true;
                restoreState(stateA);
            }
        }
        else if (clickedB)
        {
            if (!showingA)
            {
                // Clicking the already-active slot → deactivate A/B mode.
                deactivate();
            }
            else
            {
                // Switch to B — only if something has been stored there.
                if (stateB.getSize() > 0)
                {
                    showingA = false;
                    restoreState(stateB);
                }
                // If B is empty, do nothing — dashed border signals "not yet stored".
            }
        }
    }

    //==========================================================================
    // State capture / restore — synchronous, message thread only.

    void captureState(juce::MemoryBlock& dest)
    {
        dest.reset();
        processor.getStateInformation(dest);
    }

    void restoreState(const juce::MemoryBlock& src)
    {
        if (src.getSize() > 0)
            processor.setStateInformation(src.getData(), (int)src.getSize());
    }

    void deactivate()
    {
        abActive = false;
        showingA = true;
        stateA.reset();
        stateB.reset();
    }

    //==========================================================================
    // Draw a single A or B button tile.
    //
    //   isActive  — currently selected slot   → filled with accent colour
    //   isStored  — has captured state         → solid border
    //   !isStored — nothing captured yet       → dashed border, no fill
    //   isA       — true = A (XO Gold), false = B (silver)
    void paintButton(juce::Graphics& g,
                     juce::Rectangle<float> bounds,
                     const juce::String& label,
                     bool isActive,
                     bool isStored,
                     bool isA,
                     float cornerR)
    {
        using namespace GalleryColors;

        // ── Fill ─────────────────────────────────────────────────────────────
        if (isActive && isStored)
        {
            // Fully active: solid accent fill.
            juce::Colour fill = isA ? juce::Colour(xoGold)
                                    : juce::Colour(0xFFC0C0C0); // silver
            g.setColour(fill);
            g.fillRoundedRectangle(bounds, cornerR);
        }
        else if (!isActive && isStored)
        {
            // Stored but not currently showing: subtle tinted background.
            juce::Colour base = isA ? juce::Colour(xoGold)
                                    : juce::Colour(0xFFC0C0C0);
            g.setColour(base.withAlpha(0.18f));
            g.fillRoundedRectangle(bounds, cornerR);
        }
        else
        {
            // Not stored yet (or mode off): very faint fill from borderGray.
            g.setColour(get(borderGray()).withAlpha(0.22f));
            g.fillRoundedRectangle(bounds, cornerR);
        }

        // ── Border ───────────────────────────────────────────────────────────
        if (!isStored)
        {
            // Dashed border — signals "slot empty".
            drawDashedBorder(g, bounds, cornerR,
                             get(borderGray()).withAlpha(0.55f));
        }
        else
        {
            juce::Colour borderCol = isActive
                ? (isA ? juce::Colour(xoGold).darker(0.18f)
                        : juce::Colour(0xFF909090))
                : get(borderGray()).withAlpha(0.60f);

            g.setColour(borderCol);
            g.drawRoundedRectangle(bounds.reduced(0.5f), cornerR, 1.0f);
        }

        // ── Label ────────────────────────────────────────────────────────────
        juce::Colour textCol;
        if (isActive && isStored)
        {
            textCol = juce::Colours::white;
        }
        else if (isStored)
        {
            textCol = isA ? juce::Colour(xoGold).darker(0.25f)
                          : juce::Colour(0xFF808080);
        }
        else
        {
            textCol = get(textMid()).withAlpha(0.45f);
        }

        g.setFont(GalleryFonts::value(10.0f));
        g.setColour(textCol);
        g.drawText(label, bounds, juce::Justification::centred, false);
    }

    // Approximate a dashed rounded-rect border using short line segments.
    // JUCE does not natively support dashed corners on rounded rectangles,
    // so we stroke the straight edges only and draw the corners solid.
    static void drawDashedBorder(juce::Graphics& g,
                                 juce::Rectangle<float> b,
                                 float /*cornerR*/,
                                 juce::Colour colour)
    {
        g.setColour(colour);

        // Use a Path with a dashed stroke applied via PathStrokeType.
        juce::Path p;
        p.addRoundedRectangle(b.reduced(0.5f), 5.0f);

        float dashes[] = { 3.0f, 2.5f };
        juce::PathStrokeType stroke(1.0f,
                                    juce::PathStrokeType::mitered,
                                    juce::PathStrokeType::butt);
        juce::Path dashed;
        stroke.createDashedStroke(dashed, p, dashes, 2);
        g.fillPath(dashed);
    }

    //==========================================================================
    XOlokunProcessor& processor;

    bool abActive  = false;  // A/B mode is enabled
    bool showingA  = true;   // currently displaying A (true) or B (false)

    juce::MemoryBlock stateA;   // captured processor state for slot A
    juce::MemoryBlock stateB;   // captured processor state for slot B

    juce::Rectangle<float> btnABounds;
    juce::Rectangle<float> btnBBounds;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ABCompare)
};

} // namespace xolokun
