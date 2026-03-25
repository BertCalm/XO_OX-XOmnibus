#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "../GalleryColors.h"
#include "GalleryKnob.h"
#include "MidiLearnMouseListener.h"

//==============================================================================
// SpecializedDisplays.h — V1-required bespoke display components for XOlokun.
//
//   TriangleXYPad      — equilateral-triangle 2D XY pad (OVERWORLD ERA, OXYTOCIN love triangle)
//   ConductorArcDisplay — OPERA autonomous dramatic arc visualizer + playhead
//   FiveMacroDisplay    — OVERBITE 5-macro row (BELLY/BITE/SCURRY/TRASH/PLAY DEAD)
//
// Header-only. All classes live in namespace xolokun.
// Include order: GalleryColors.h → GalleryKnob.h → MidiLearnMouseListener.h
// (already satisfied above).
//==============================================================================

namespace xolokun {

//==============================================================================
//
//  TriangleXYPad
//  =============
//  Equilateral-triangle 2D XY pad. Dragging inside the triangle maps the touch
//  position to barycentric coordinates (three weights summing to 1.0) and
//  drives two APVTS float parameters: paramX and paramY.
//
//  Designed for:
//    • OVERWORLD — ERA triangle (Analog/Digital/Hybrid).
//      Params: ow_eraBlendX, ow_eraBlendY
//    • OXYTOCIN  — Love triangle (3 circuit models).
//      Params: oxy_triangleX, oxy_triangleY
//
//  Nominal size: 160×140pt.
//
//  Layout
//  ------
//   Vertex[0] = top-centre
//   Vertex[1] = bottom-left
//   Vertex[2] = bottom-right
//
//  Both paramX and paramY are driven via hidden juce::Sliders with
//  SliderAttachment so host automation and undo work correctly.
//
class TriangleXYPad : public juce::Component
{
public:
    // vertexLabels[3] — e.g. {"ANALOG", "DIGITAL", "HYBRID"}
    // vertexColors[3] — per-vertex accent colours
    // paramX, paramY  — APVTS float parameter IDs for the pad position
    TriangleXYPad(juce::AudioProcessorValueTreeState& apvts,
                  const juce::String& paramX,
                  const juce::String& paramY,
                  const std::array<juce::String, 3>& vertexLabels,
                  const std::array<juce::Colour, 3>& vertexColors)
        : apvtsRef(apvts),
          paramIdX(paramX),
          paramIdY(paramY),
          labels(vertexLabels),
          colors(vertexColors)
    {
        // Hidden sliders — their only purpose is to hold the SliderAttachments
        // so that host automation, undo/redo, and parameter listeners work.
        for (auto* s : { &hiddenSliderX, &hiddenSliderY })
        {
            s->setSliderStyle(juce::Slider::LinearHorizontal);
            s->setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
            s->setVisible(false);
            addAndMakeVisible(*s);
        }

        attachX = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            apvts, paramX, hiddenSliderX);
        attachY = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            apvts, paramY, hiddenSliderY);

        A11y::setup(*this,
                    "Triangle XY Pad",
                    "Drag inside the triangle to blend between "
                    + vertexLabels[0] + ", " + vertexLabels[1]
                    + " and " + vertexLabels[2]);
        setWantsKeyboardFocus(true);
    }

    //--------------------------------------------------------------------------
    void paint(juce::Graphics& g) override
    {
        using namespace GalleryColors;
        const auto tri = buildTriangle();

        // ── Interior gradient fill ────────────────────────────────────────────
        // We approximate the 3-colour barycentric blend with two ColourGradients
        // (a single pass composited as a rough visual approximation; exact
        // per-pixel barycentric blending requires a custom shader not available
        // in JUCE's 2D renderer).
        //
        // Pass 1: vertex[0] → mid bottom   (top colour dominates upper region)
        // Pass 2: vertex[1] → vertex[2]    (bottom-left to bottom-right)
        //
        // Both passes drawn at reduced alpha so they blend together naturally.
        {
            const juce::Point<float> v0 = tri[0], v1 = tri[1], v2 = tri[2];
            const juce::Point<float> midBottom = (v1 + v2) * 0.5f;

            // Fill with a subtle unified background first
            g.setColour(get(slotBg()).withAlpha(0.90f));
            g.fillPath(trianglePath(tri));

            // Top-vertex glow
            juce::ColourGradient grad0(colors[0].withAlpha(0.22f), v0.x, v0.y,
                                       colors[0].withAlpha(0.0f),  midBottom.x, midBottom.y,
                                       false);
            g.setGradientFill(grad0);
            g.fillPath(trianglePath(tri));

            // Bottom-left glow
            juce::ColourGradient grad1(colors[1].withAlpha(0.22f), v1.x, v1.y,
                                       colors[1].withAlpha(0.0f),  v2.x, v2.y,
                                       false);
            g.setGradientFill(grad1);
            g.fillPath(trianglePath(tri));

            // Bottom-right glow
            juce::ColourGradient grad2(colors[2].withAlpha(0.22f), v2.x, v2.y,
                                       colors[2].withAlpha(0.0f),  v1.x, v1.y,
                                       false);
            g.setGradientFill(grad2);
            g.fillPath(trianglePath(tri));
        }

        // ── Triangle border ───────────────────────────────────────────────────
        g.setColour(get(borderGray()).withAlpha(0.80f));
        g.strokePath(trianglePath(tri), juce::PathStrokeType(1.2f));

        // ── Lines from each vertex to the current position (20 % opacity) ────
        {
            const auto pos = currentPadPos();
            for (int i = 0; i < 3; ++i)
            {
                g.setColour(colors[i].withAlpha(0.20f));
                g.drawLine(tri[i].x, tri[i].y, pos.x, pos.y, 1.0f);
            }
        }

        // ── Vertex dots + labels ──────────────────────────────────────────────
        for (int i = 0; i < 3; ++i)
        {
            // Small vertex dot
            g.setColour(colors[i]);
            g.fillEllipse(tri[i].x - 3.0f, tri[i].y - 3.0f, 6.0f, 6.0f);

            // Label: Space Grotesk SemiBold 8pt
            g.setFont(GalleryFonts::display(8.0f));
            g.setColour(colors[i]);

            // Offset label away from the vertex
            juce::Rectangle<float> labelRect;
            constexpr float kLabelW = 50.0f, kLabelH = 12.0f;
            if (i == 0) // top-centre → label above
                labelRect = { tri[i].x - kLabelW * 0.5f, tri[i].y - kLabelH - 4.0f,
                               kLabelW, kLabelH };
            else if (i == 1) // bottom-left → label below-left
                labelRect = { tri[i].x - kLabelW - 2.0f, tri[i].y + 3.0f,
                               kLabelW, kLabelH };
            else // bottom-right → label below-right
                labelRect = { tri[i].x + 2.0f, tri[i].y + 3.0f,
                               kLabelW, kLabelH };

            g.drawText(labels[i], labelRect, juce::Justification::centred, false);
        }

        // ── Current-position handle (8pt circle in accent gold) ──────────────
        {
            const auto pos = currentPadPos();
            const juce::Colour handleCol = get(xoGold);

            // Subtle glow
            g.setColour(handleCol.withAlpha(0.20f));
            g.fillEllipse(pos.x - 8.0f, pos.y - 8.0f, 16.0f, 16.0f);

            // Handle fill
            g.setColour(handleCol);
            g.fillEllipse(pos.x - 4.0f, pos.y - 4.0f, 8.0f, 8.0f);

            // Handle border
            g.setColour(handleCol.darker(0.3f));
            g.drawEllipse(pos.x - 4.0f, pos.y - 4.0f, 8.0f, 8.0f, 1.0f);
        }

        // ── Focus ring ───────────────────────────────────────────────────────
        if (hasKeyboardFocus(false))
            A11y::drawFocusRing(g, getLocalBounds().toFloat(), 4.0f);
    }

    void mouseDown(const juce::MouseEvent& e) override
    {
        updateFromMouse(e.position);
    }

    void mouseDrag(const juce::MouseEvent& e) override
    {
        updateFromMouse(e.position);
    }

    void resized() override
    {
        // Hidden sliders live at zero size — they only exist for attachments.
        hiddenSliderX.setBounds(0, 0, 1, 1);
        hiddenSliderY.setBounds(0, 0, 1, 1);
    }

private:
    //--------------------------------------------------------------------------
    // Returns the three triangle vertex positions in component space.
    // Vertex[0] = top-centre, [1] = bottom-left, [2] = bottom-right.
    std::array<juce::Point<float>, 3> buildTriangle() const
    {
        const float w = static_cast<float>(getWidth());
        const float h = static_cast<float>(getHeight());

        // Inset so labels/handle don't clip at the boundary.
        constexpr float kInsetX = 30.0f;
        constexpr float kInsetY = 20.0f;
        const float left   = kInsetX;
        const float right  = w - kInsetX;
        const float top    = kInsetY;
        const float bottom = h - kInsetY;

        return { juce::Point<float>{ w * 0.5f,  top    },   // [0] apex
                 juce::Point<float>{ left,       bottom },   // [1] bottom-left
                 juce::Point<float>{ right,      bottom } }; // [2] bottom-right
    }

    // Build a juce::Path for the triangle.
    static juce::Path trianglePath(const std::array<juce::Point<float>, 3>& tri)
    {
        juce::Path p;
        p.startNewSubPath(tri[0]);
        p.lineTo(tri[1]);
        p.lineTo(tri[2]);
        p.closeSubPath();
        return p;
    }

    // Compute the current pad position in component space from the X/Y params.
    // paramX ∈ [0,1] maps to the barycentric X axis.
    // paramY ∈ [0,1] maps to the barycentric Y axis.
    // We recover the point via: pos = v0 + paramX*(v2-v1) + paramY*(v1-v0).
    // This gives a linear mapping where (0,0) = bottom-left vertex,
    // (1,0) = bottom-right vertex, (0.5,1) = top vertex.
    juce::Point<float> currentPadPos() const
    {
        const auto tri = buildTriangle();
        const float px = static_cast<float>(hiddenSliderX.getValue());
        const float py = static_cast<float>(hiddenSliderY.getValue());
        // Recover Cartesian position from normalised X/Y
        // by interpolating along the barycentric axes.
        const juce::Point<float> bottom = tri[1] + (tri[2] - tri[1]) * px;
        return bottom + (tri[0] - bottom) * py;
    }

    // Map a mouse position to X/Y params, clamped to the interior of the
    // triangle using barycentric coordinates.
    void updateFromMouse(juce::Point<float> mousePos)
    {
        const auto tri = buildTriangle();

        // Compute barycentric coordinates (w0, w1, w2) for the mouse point
        // relative to the triangle.  Formula: signed area ratio method.
        auto bary = toBarycentricClamped(mousePos, tri);

        // Map from barycentric back to our paramX/Y convention:
        //   paramX = blend between v1 (left) and v2 (right) along the base
        //   paramY = blend between base and v0 (apex)
        //
        // Given barycentric (w0=apex, w1=bottom-left, w2=bottom-right):
        //   along base:  px = w2 / (w1 + w2)   (0 = full left, 1 = full right)
        //   towards apex: py = w0               (0 = at base, 1 = at apex)
        const float denomBase = bary[1] + bary[2];
        const float px = (denomBase > 1e-6f) ? (bary[2] / denomBase) : 0.5f;
        const float py = bary[0];

        // Drive through the hidden sliders so the SliderAttachment handles
        // the APVTS update, host notification, undo-gesture grouping, and
        // parameter range clamping.  Calling setValueNotifyingHost directly
        // bypasses the attachment's begin/endChangeGesture bookkeeping, which
        // breaks DAW automation recording and undo grouping.
        hiddenSliderX.setValue(static_cast<double>(px), juce::sendNotificationSync);
        hiddenSliderY.setValue(static_cast<double>(py), juce::sendNotificationSync);

        repaint();
    }

    // Compute barycentric coordinates of point P w.r.t. triangle ABC.
    // Returns {w_A, w_B, w_C} clamped so all ≥ 0 and summing to 1.
    static std::array<float, 3> toBarycentricClamped(
        juce::Point<float> P,
        const std::array<juce::Point<float>, 3>& tri)
    {
        const auto& A = tri[0];
        const auto& B = tri[1];
        const auto& C = tri[2];

        const float denom = (B.y - C.y) * (A.x - C.x) + (C.x - B.x) * (A.y - C.y);
        if (std::abs(denom) < 1e-6f)
            return { 1.0f / 3.0f, 1.0f / 3.0f, 1.0f / 3.0f };

        float w0 = ((B.y - C.y) * (P.x - C.x) + (C.x - B.x) * (P.y - C.y)) / denom;
        float w1 = ((C.y - A.y) * (P.x - C.x) + (A.x - C.x) * (P.y - C.y)) / denom;
        float w2 = 1.0f - w0 - w1;

        // Clamp each weight to [0,1] and re-normalise.
        w0 = juce::jlimit(0.0f, 1.0f, w0);
        w1 = juce::jlimit(0.0f, 1.0f, w1);
        w2 = juce::jlimit(0.0f, 1.0f, w2);
        const float sum = w0 + w1 + w2;
        if (sum > 1e-6f) { w0 /= sum; w1 /= sum; w2 /= sum; }
        else             { w0 = w1 = w2 = 1.0f / 3.0f; }

        return { w0, w1, w2 };
    }

    //--------------------------------------------------------------------------
    juce::AudioProcessorValueTreeState& apvtsRef;
    const juce::String                  paramIdX, paramIdY;
    const std::array<juce::String, 3>   labels;
    const std::array<juce::Colour, 3>   colors;

    // Hidden sliders carry the SliderAttachments; the visual display is the
    // triangle pad drawn in paint().
    juce::Slider hiddenSliderX, hiddenSliderY;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachX, attachY;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TriangleXYPad)
};


//==============================================================================
//
//  ConductorArcDisplay
//  ====================
//  Visualizes OPERA's autonomous dramatic arc (B035). Shows the selected arc
//  shape as a curve (Rise / Fall / Swell / Wave), a moving playhead, and a
//  small conductor-mode badge.
//
//  Nominal size: 200×60pt.
//
//  Parameters read (all read-only, never written):
//    opera_arcShape — choice  0=Rise 1=Fall 2=Swell 3=Wave
//    opera_arcTime  — float   total arc duration in seconds
//    opera_arcPeak  — float   peak position 0–1 along the arc
//    opera_arcMode  — choice  0=Manual 1=Conductor 2=Both
//
//  The playhead is driven by an internal 10 Hz timer that advances a phase
//  accumulator based on arcTime.  If the engine exposes a real-time arc phase
//  as a read-only atomic parameter it can be wired in by calling
//  setArcPhaseAtomic() — the timer will use that value directly instead.
//
class ConductorArcDisplay : public juce::Component, private juce::Timer
{
public:
    explicit ConductorArcDisplay(juce::AudioProcessorValueTreeState& apvts)
        : apvtsRef(apvts)
    {
        // 10 Hz repaint — smooth enough for a slow arc visualizer.
        // Reduced-motion users still get 10 Hz (arc is informational, not reactive).
        startTimerHz(10);
        setOpaque(false);
        A11y::setup(*this,
                    "Conductor Arc Display",
                    "Shows the OPERA dramatic arc shape and current playhead position");
    }

    ~ConductorArcDisplay() override { stopTimer(); }

    // Optional: wire a real-time phase atomic from the engine (0–1 range).
    // When set, the timer reads this value directly instead of free-running.
    void setArcPhaseAtomic(const std::atomic<float>* phasePtr) noexcept
    {
        arcPhaseAtomic = phasePtr;
    }

    //--------------------------------------------------------------------------
    void timerCallback() override
    {
        // Advance the free-running phase accumulator.  If arcPhaseAtomic is
        // wired, we just read that instead and skip accumulation.
        if (arcPhaseAtomic == nullptr)
        {
            // Read arcTime from APVTS — re-read every tick (state can change).
            float arcTimeSec = 4.0f; // fallback default
            if (auto* p = apvtsRef.getParameter("opera_arcTime"))
            {
                const float norm = p->getValue();
                arcTimeSec = static_cast<float>(
                    p->getNormalisableRange().convertFrom0to1(norm));
                arcTimeSec = juce::jmax(0.01f, arcTimeSec);
            }

            // Advance phase (10 Hz tick = 0.1s increments).
            playheadPhase += 0.1f / arcTimeSec;
            if (playheadPhase >= 1.0f)
                playheadPhase -= 1.0f;
        }

        repaint();
    }

    void paint(juce::Graphics& g) override
    {
        using namespace GalleryColors;
        const auto bounds = getLocalBounds().toFloat();
        const float w = bounds.getWidth();
        const float h = bounds.getHeight();

        // ── Background ────────────────────────────────────────────────────────
        g.setColour(get(slotBg()).withAlpha(0.92f));
        g.fillRoundedRectangle(bounds, 3.0f);

        // ── Read parameters ───────────────────────────────────────────────────
        const int   arcShape = readChoiceParam("opera_arcShape", 0, 3, 0);
        const float arcPeak  = readFloatNorm  ("opera_arcPeak",  0.5f);
        const int   condMode = readChoiceParam("opera_arcMode",  0, 2, 1);

        // Current playhead phase (0–1)
        float phase = (arcPhaseAtomic != nullptr)
                          ? juce::jlimit(0.0f, 1.0f, arcPhaseAtomic->load())
                          : playheadPhase;

        // ── Build arc path ────────────────────────────────────────────────────
        // The arc occupies the full width, vertically padded by kPadY on each
        // side.  y=kPadY is the "high" position; y=h-kPadY is the "low" position.
        constexpr float kPadY = 8.0f;
        const float yHigh = kPadY;
        const float yLow  = h - kPadY;
        const float yMid  = (yHigh + yLow) * 0.5f;

        juce::Path arcPath;
        constexpr int kSteps = 128;

        for (int i = 0; i <= kSteps; ++i)
        {
            const float t  = static_cast<float>(i) / static_cast<float>(kSteps);
            const float px = t * w;
            float py = arcYForShape(arcShape, arcPeak, t, yHigh, yLow, yMid);

            if (i == 0)
                arcPath.startNewSubPath(px, py);
            else
                arcPath.lineTo(px, py);
        }

        // ── Filled area below arc ─────────────────────────────────────────────
        // Close the path to form a filled region (arc → bottom-right → bottom-left).
        juce::Path fillPath = arcPath;
        fillPath.lineTo(w, yLow);
        fillPath.lineTo(0.0f, yLow);
        fillPath.closeSubPath();

        // Aria Gold (#D4AF37) at 15 % opacity
        g.setColour(juce::Colour(0xFFD4AF37).withAlpha(0.15f));
        g.fillPath(fillPath);

        // ── Arc stroke — 2px Aria Gold ────────────────────────────────────────
        g.setColour(juce::Colour(0xFFD4AF37));
        g.strokePath(arcPath, juce::PathStrokeType(2.0f,
                                                    juce::PathStrokeType::curved,
                                                    juce::PathStrokeType::rounded));

        // ── Playhead — bright vertical line at current phase ──────────────────
        {
            const float phaseX = phase * w;
            g.setColour(juce::Colour(0xFFFFFFFF).withAlpha(0.90f));
            g.drawLine(phaseX, kPadY * 0.5f, phaseX, h - kPadY * 0.5f, 1.5f);

            // Small arrowhead cap on top
            g.fillEllipse(phaseX - 2.5f, kPadY * 0.5f - 2.5f, 5.0f, 5.0f);
        }

        // ── Conductor mode badge (bottom-right) ───────────────────────────────
        {
            static constexpr const char* kModeLabels[] = { "MANUAL", "COND", "BOTH" };
            const char* modeText = kModeLabels[juce::jlimit(0, 2, condMode)];

            // All three arc modes are active states — show Aria Gold always.
            // condMode 0=Manual, 1=Conductor, 2=Both (no OFF state exists).
            juce::Colour badgeCol = juce::Colour(0xFFD4AF37); // Aria Gold

            g.setFont(GalleryFonts::display(7.0f));
            g.setColour(badgeCol);
            g.drawText(modeText,
                       juce::Rectangle<float>{ w - 40.0f, h - 14.0f, 38.0f, 12.0f },
                       juce::Justification::centredRight, false);
        }
    }

    void resized() override {}

private:
    //--------------------------------------------------------------------------
    // Compute the arc y-position at fractional time t ∈ [0,1] for the given
    // arcShape, using arcPeak to position the peak within the shape.
    //
    //   arcShape 0 = Rise  : ascending curve (low → high)
    //   arcShape 1 = Fall  : descending curve (high → low)
    //   arcShape 2 = Swell : bell curve (low → high → low), peak at arcPeak
    //   arcShape 3 = Wave  : full sine cycle (low → high → low → high)
    //
    // yHigh / yLow / yMid are pixel positions in component space (y-down).
    static float arcYForShape(int shape, float peak, float t,
                               float yHigh, float yLow, float yMid)
    {
        const float yRange = yLow - yHigh; // positive (yLow > yHigh in y-down)

        switch (shape)
        {
            case 0: // Rise — ease-in curve from low to high
            {
                const float ease = 1.0f - (1.0f - t) * (1.0f - t); // quadratic ease-in
                return yLow - ease * yRange;
            }
            case 1: // Fall — ease-out from high to low
            {
                const float ease = t * t; // quadratic ease-out
                return yHigh + ease * yRange;
            }
            case 2: // Swell — bell peaked at arcPeak
            {
                // Map t relative to peak: 0 at edges, 1 at peak
                const float distFromPeak = std::abs(t - peak);
                const float halfWidth    = juce::jmax(0.01f, juce::jmax(peak, 1.0f - peak));
                const float normalised   = 1.0f - juce::jlimit(0.0f, 1.0f, distFromPeak / halfWidth);
                const float bell         = normalised * normalised; // smooth curve
                return yLow - bell * yRange;
            }
            case 3: // Wave — one full sine cycle
            {
                const float sine = 0.5f + 0.5f * std::sin(
                    juce::MathConstants<float>::twoPi * t - juce::MathConstants<float>::halfPi);
                return yLow - sine * yRange;
            }
            default:
                return yMid;
        }
    }

    // Read a choice parameter and clamp to [minVal, maxVal], returning
    // defaultVal if the parameter is not found.
    int readChoiceParam(const juce::String& paramId, int minVal, int maxVal, int defaultVal) const
    {
        if (auto* p = apvtsRef.getParameter(paramId))
        {
            // Choice parameters store value as 0-based index normalised to [0,1].
            const float norm = p->getValue();
            // Round to nearest integer index
            const int idx = juce::roundToInt(
                static_cast<float>(maxVal - minVal) * norm) + minVal;
            return juce::jlimit(minVal, maxVal, idx);
        }
        return defaultVal;
    }

    // Read a float parameter and return its normalised value [0,1],
    // returning defaultNorm if not found.
    float readFloatNorm(const juce::String& paramId, float defaultNorm) const
    {
        if (auto* p = apvtsRef.getParameter(paramId))
            return juce::jlimit(0.0f, 1.0f, p->getValue());
        return defaultNorm;
    }

    //--------------------------------------------------------------------------
    juce::AudioProcessorValueTreeState& apvtsRef;
    const std::atomic<float>*           arcPhaseAtomic = nullptr;
    float                               playheadPhase  = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ConductorArcDisplay)
};


//==============================================================================
//
//  FiveMacroDisplay
//  =================
//  Special 5-macro row for OVERBITE (B008 blessing — five-macro system).
//  Unlike all other engines which use 4 macros, OVERBITE has:
//
//    BELLY     — warm amber    #E9A84A   poss_macroBelly
//    BITE      — fang white    #F0EDE8   poss_macroBite
//    SCURRY    — phosphor green #00FF41  poss_macroScurry
//    TRASH     — hot pink      #FF1493   poss_macroTrash
//    PLAY DEAD — midnight      #2D0A4E   poss_macroPlayDead
//
//  Nominal size: 280×48pt (5 × 44pt knobs with 8pt spacing).
//
//  This component replaces MacroHeroStrip when OVERBITE is the active engine.
//
class FiveMacroDisplay : public juce::Component
{
public:
    explicit FiveMacroDisplay(juce::AudioProcessorValueTreeState& apvts,
                              MIDILearnManager* midiLearn = nullptr)
        : apvtsRef(apvts)
    {
        struct KnobDef
        {
            const char*    paramId;
            const char*    label;
            juce::uint32   colorHex;
        };

        static constexpr KnobDef kDefs[5] = {
            { "poss_macroBelly",    "BELLY",     0xFFE9A84A },
            { "poss_macroBite",     "BITE",      0xFFF0EDE8 },
            { "poss_macroScurry",   "SCURRY",    0xFF00FF41 },
            { "poss_macroTrash",    "TRASH",      0xFFFF1493 },
            { "poss_macroPlayDead", "PLAY DEAD",  0xFF2D0A4E },
        };

        for (int i = 0; i < 5; ++i)
        {
            const juce::Colour col(kDefs[i].colorHex);

            knobs[i].setSliderStyle(juce::Slider::RotaryVerticalDrag);
            knobs[i].setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
            knobs[i].setColour(juce::Slider::rotarySliderFillColourId, col);
            knobs[i].setColour(juce::Slider::rotarySliderOutlineColourId,
                               col.withAlpha(0.35f));
            knobs[i].setColour(juce::Slider::thumbColourId, col);
            knobs[i].setTooltip(juce::String("OVERBITE macro: ") + kDefs[i].label);

            A11y::setup(knobs[i],
                        juce::String("OVERBITE ") + kDefs[i].label + " macro",
                        juce::String("Controls the ") + kDefs[i].label + " character dimension");

            addAndMakeVisible(knobs[i]);

            attachments[i] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                apvts, kDefs[i].paramId, knobs[i]);

            enableKnobReset(knobs[i], apvts, kDefs[i].paramId);

            // Wire MIDI learn if a manager was provided.
            if (midiLearn != nullptr)
            {
                auto* ml = knobs[i].setupMidiLearn(kDefs[i].paramId, *midiLearn);
                learnListeners[i].reset(ml);
            }

            // Label
            labels[i].setText(kDefs[i].label, juce::dontSendNotification);
            labels[i].setFont(GalleryFonts::display(7.0f)); // Space Grotesk SemiBold 7pt
            labels[i].setColour(juce::Label::textColourId, col);
            labels[i].setJustificationType(juce::Justification::centred);
            addAndMakeVisible(labels[i]);
        }

        A11y::setup(*this,
                    "OVERBITE Five Macro Display",
                    "Five OVERBITE character macros: BELLY, BITE, SCURRY, TRASH, PLAY DEAD");
    }

    ~FiveMacroDisplay() override
    {
        // Remove MIDI learn mouse listeners from their knobs before the
        // unique_ptrs are destroyed.  Without this, the knob's mouse-listener
        // list holds a dangling raw pointer window between the unique_ptr reset
        // and the knob member's destructor.
        for (int i = 0; i < 5; ++i)
            if (learnListeners[static_cast<size_t>(i)])
                knobs[static_cast<size_t>(i)].removeMouseListener(
                    learnListeners[static_cast<size_t>(i)].get());
    }

    void paint(juce::Graphics& g) override
    {
        using namespace GalleryColors;
        const auto b = getLocalBounds().toFloat();

        // Subtle strip background
        g.setColour(get(slotBg()).withAlpha(0.85f));
        g.fillRoundedRectangle(b, 3.0f);

        // Thin Fang White top accent (OVERBITE accent color)
        g.setColour(juce::Colour(0xFFF0EDE8).withAlpha(0.70f));
        auto accentStrip = b; accentStrip.setHeight(2.0f);
        g.fillRect(accentStrip);
    }

    void resized() override
    {
        // Layout: 5 knobs × 44pt with 8pt gap, centred vertically.
        // Total strip width = 280pt (nominal), labels below each knob.
        constexpr int kKnobSize  = 44;
        constexpr int kGap       = 8;
        constexpr int kLabelH    = 10;
        const int     totalKnobs = 5;

        const int stripW    = getWidth();
        const int stripH    = getHeight();
        const int unitW     = (stripW - kGap * (totalKnobs - 1)) / totalKnobs;
        const int knobSize  = juce::jmin(kKnobSize, unitW);
        const int knobTop   = (stripH - knobSize - kLabelH - 2) / 2;

        int x = (stripW - (totalKnobs * unitW + (totalKnobs - 1) * kGap)) / 2;
        x = juce::jmax(0, x);

        for (int i = 0; i < 5; ++i)
        {
            const int centreX = x + unitW / 2;
            knobs[i].setBounds(centreX - knobSize / 2, knobTop, knobSize, knobSize);
            labels[i].setBounds(x, knobTop + knobSize + 1, unitW, kLabelH);
            x += unitW + kGap;
        }
    }

private:
    juce::AudioProcessorValueTreeState& apvtsRef;

    std::array<GalleryKnob,  5> knobs;
    std::array<juce::Label,  5> labels;

    // Destruction order: learnListeners → attachments → knobs (reverse declaration).
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>, 5> attachments;
    std::array<std::unique_ptr<MidiLearnMouseListener>, 5> learnListeners;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FiveMacroDisplay)
};

} // namespace xolokun
