// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "../../XOceanusProcessor.h"
#include "../../Core/MegaCouplingMatrix.h"
#include "../GalleryColors.h"
#include "../CouplingColors.h" // CouplingTypeColors::displayName() — #713
#include "GalleryKnob.h"

namespace xoceanus
{

//==============================================================================
// CouplingPopover — inline control panel for one MegaCouplingMatrix route.
//
// Shown inside a juce::CallOutBox when the user clicks a coupling arc in the
// OverviewPanel. Owns all APVTS attachments; they are destroyed when the
// CallOutBox closes and takes the CouplingPopover with it.
//
// Layout (220 × 160 pt):
//   Row 0  [0–24]   Header: "ROUTE N" label + accent dot + source→target label
//   Row 1  [24–44]  Active toggle  (ButtonAttachment → cp_rN_active)
//   Row 2  [44–72]  Type selector  (ComboBoxAttachment → cp_rN_type)
//   Row 3  [72–120] Amount knob    (SliderAttachment  → cp_rN_amount, bipolar)
//   Row 4 [120–148] Source selector (ComboBoxAttachment → cp_rN_source)
//   Row 5 [148–176] Target selector (ComboBoxAttachment → cp_rN_target)
//   (component height snapped to 176 to fit; CallOutBox sizes itself around us)
//
// Constructor: CouplingPopover(XOceanusProcessor& proc, int routeIndex)
//   routeIndex is 1–4, mapping to parameter IDs cp_r1_* … cp_r4_*
//
class CouplingPopover : public juce::Component
{
public:
    CouplingPopover(XOceanusProcessor& proc, int routeIndex) : processor(proc), route(routeIndex)
    {
        jassert(route >= 1 && route <= 4);
        auto& apvts = proc.getAPVTS();

        const juce::String prefix = "cp_r" + juce::String(route) + "_";

        // ── Active toggle ────────────────────────────────────────────────────
        activeButton.setButtonText("ACTIVE");
        activeButton.setClickingTogglesState(true);
        addAndMakeVisible(activeButton);
        activeAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(apvts, prefix + "active",
                                                                                              activeButton);
        A11y::setup(activeButton, "Route " + juce::String(route) + " Active", "Enable or disable this coupling route");

        // ── Type selector — #713: use human-readable display names from
        //    CouplingTypeColors::displayName() instead of raw C++ enum strings.
        {
            auto types = CouplingTypeColors::allTypes();
            for (int i = 0; i < static_cast<int>(types.size()); ++i)
                typeBox.addItem(CouplingTypeColors::displayName(types[static_cast<size_t>(i)]), i + 1);
        }
        addAndMakeVisible(typeBox);
        typeAttach =
            std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(apvts, prefix + "type", typeBox);
        A11y::setup(typeBox, "Coupling Type", "Select the modulation routing type for this route");

        // ── Amount knob (bipolar, center-detent) ─────────────────────────────
        amountKnob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        amountKnob.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 44, 14);
        amountKnob.setRange(-1.0, 1.0, 0.0);
        amountKnob.setNumDecimalPlacesToDisplay(2);
        addAndMakeVisible(amountKnob);
        amountAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, prefix + "amount",
                                                                                              amountKnob);
        enableKnobReset(amountKnob, apvts, prefix + "amount");
        A11y::setup(amountKnob, "Coupling Amount", "Bipolar coupling depth — negative values invert the modulation");

        // ── Source slot selector — #714: populate with engine names, then
        //    refreshSlotNames() updates text without changing the item IDs so
        //    APVTS serialization remains intact (IDs 1–5 are preserved).
        for (int s = 1; s <= 5; ++s)
            sourceBox.addItem("Slot " + juce::String(s), s);
        addAndMakeVisible(sourceBox);
        sourceAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
            apvts, prefix + "source", sourceBox);
        A11y::setup(sourceBox, "Source Slot", "Engine slot that drives this coupling route");

        // ── Target slot selector ─────────────────────────────────────────────
        for (int s = 1; s <= 5; ++s)
            targetBox.addItem("Slot " + juce::String(s), s);
        addAndMakeVisible(targetBox);
        targetAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
            apvts, prefix + "target", targetBox);
        A11y::setup(targetBox, "Target Slot", "Engine slot that receives this coupling route");

        A11y::setup(*this, "Coupling Route " + juce::String(route),
                    "Controls for cross-engine modulation route " + juce::String(route));

        // Populate combo items with actual engine names now that all items exist.
        refreshSlotNames();

        setSize(220, 176);
    }

    void resized() override
    {
        auto b = getLocalBounds();

        // Row 0: header (24pt) — painted only
        b.removeFromTop(24);

        // Row 1: active toggle (20pt)
        activeButton.setBounds(b.removeFromTop(20).reduced(8, 2));

        // Row 2: type selector (28pt)
        typeBox.setBounds(b.removeFromTop(28).reduced(8, 2));

        // Row 3: amount knob centred in a 48pt zone
        auto knobRow = b.removeFromTop(48);
        int kw = 48, kh = 48;
        amountKnob.setBounds(knobRow.getCentreX() - kw / 2, knobRow.getY(), kw, kh);

        // Row 4: source selector (28pt)
        sourceBox.setBounds(b.removeFromTop(28).reduced(8, 2));

        // Row 5: target selector (28pt)
        targetBox.setBounds(b.removeFromTop(28).reduced(8, 2));
    }

    void paint(juce::Graphics& g) override
    {
        using namespace GalleryColors;

        // Shell background
        g.fillAll(get(shellWhite()));

        auto b = getLocalBounds().toFloat();

        // Header row
        auto headerBounds = b.removeFromTop(24.0f);

        // Accent dot — colour derived from coupling type currently selected
        const juce::Colour accentCol = accentColorForCurrentType();
        g.setColour(accentCol);
        float dotR = 5.0f;
        g.fillEllipse(8.0f, headerBounds.getCentreY() - dotR, dotR * 2.0f, dotR * 2.0f);

        // "ROUTE N" label
        g.setFont(GalleryFonts::display(11.0f));
        g.setColour(get(textDark()));
        g.drawText("ROUTE " + juce::String(route), 20, (int)headerBounds.getY(), 80, (int)headerBounds.getHeight(),
                   juce::Justification::centredLeft);

        // Source→Target label — #714: show engine name, not "S1" slot number.
        juce::String srcLabel = slotDisplayName(sourceBox.getSelectedId() - 1);
        juce::String dstLabel = slotDisplayName(targetBox.getSelectedId() - 1);
        g.setFont(GalleryFonts::label(10.0f)); // (#885: 9pt→10pt legibility floor)
        g.setColour(accentCol);
        g.drawText(srcLabel + " \xe2\x86\x92 " + dstLabel, // UTF-8 right arrow →
                   110, (int)headerBounds.getY(), 100, (int)headerBounds.getHeight(),
                   juce::Justification::centredRight);

        // Thin separator under header
        g.setColour(get(borderGray()));
        g.drawHorizontalLine(24, 8.0f, b.getWidth() - 8.0f);

        // Center-detent indicator on the knob arc
        // (painted beneath knob bounds for the bipolar zero line)
        auto knobCx = (float)amountKnob.getBounds().getCentreX();
        auto knobCy = (float)amountKnob.getBounds().getCentreY();
        float knobR = 20.0f;
        // Vertical tick at centre-bottom (270° = 12 o'clock on rotary, then down 90°)
        // JUCE rotary knobs draw 7 o'clock to 5 o'clock; centre is straight down (270°)
        // We draw a tiny tick just outside the knob at the 6 o'clock position.
        const float centreAngle = juce::MathConstants<float>::pi; // straight down in JUCE rotary coords
        float tickX = knobCx + (knobR + 2.0f) * std::sin(centreAngle);
        float tickY = knobCy - (knobR + 2.0f) * std::cos(centreAngle);
        g.setColour(get(textMid()).withAlpha(0.45f));
        g.fillEllipse(tickX - 2.0f, tickY - 2.0f, 4.0f, 4.0f);

        // Amount label above the knob
        g.setFont(GalleryFonts::label(10.0f)); // (#885: 8pt→10pt legibility floor)
        g.setColour(get(textMid()));
        g.drawText("AMOUNT", amountKnob.getX() - 12, amountKnob.getY() - 12, 72, 12, juce::Justification::centred);

        // Source/Target row labels (left-justified, vertically centred in their rows)
        g.setFont(GalleryFonts::label(10.0f)); // (#885: 8pt→10pt legibility floor)
        g.setColour(get(textMid()));
        g.drawText("SOURCE", 8, sourceBox.getY(), 44, sourceBox.getHeight(), juce::Justification::centredLeft);
        g.drawText("TARGET", 8, targetBox.getY(), 44, targetBox.getHeight(), juce::Justification::centredLeft);
    }

private:
    juce::Colour accentColorForCurrentType() const
    {
        // Mirror CouplingArcOverlay's three-color palette
        int sel = typeBox.getSelectedId(); // 1-indexed per addItem() calls above
        if (sel >= 5 && sel <= 6)
            return juce::Colour(0xFF0096C7); // Twilight Blue — audio-rate
        if (sel == 12 || sel == 13)
            return juce::Colour(0xFF0096C7); // AudioToWavetable / AudioToBuffer
        if (sel == 14)
            return juce::Colour(0xFF7B2FBE); // Midnight Violet — KnotTopology
        return juce::Colour(0xFFE9C46A);     // XO Gold — modulation
    }

    // #714: Return a short display name for a 0-based slot index.
    // Uses the engine's ID when loaded; falls back to "Slot N" when empty.
    // Slot 4 (Ghost) is always labelled "Ghost".
    juce::String slotDisplayName(int slotIdx) const
    {
        if (slotIdx == 4)
            return "Ghost";
        if (slotIdx < 0 || slotIdx > 3)
            return "Slot " + juce::String(slotIdx + 1);
        auto* eng = processor.getEngine(slotIdx);
        if (eng)
        {
            auto id = eng->getEngineId();
            if (id.isNotEmpty())
                return id.toUpperCase();
        }
        return "Slot " + juce::String(slotIdx + 1);
    }

    // #714: Update source/target combo item text to show engine names.
    // Item IDs (1–5) are preserved so APVTS serialization is unaffected.
    void refreshSlotNames()
    {
        for (int slotId = 1; slotId <= 5; ++slotId)
        {
            juce::String label;
            if (slotId == 5)
            {
                label = "Slot 5 (Ghost)";
            }
            else
            {
                auto* eng = processor.getEngine(slotId - 1);
                if (eng)
                {
                    auto id = eng->getEngineId();
                    label = id.isEmpty() ? ("Slot " + juce::String(slotId))
                                         : (id.toUpperCase() + " (Slot " + juce::String(slotId) + ")");
                }
                else
                {
                    label = "Slot " + juce::String(slotId) + " (empty)";
                }
            }
            sourceBox.changeItemText(slotId, label);
            targetBox.changeItemText(slotId, label);
        }
    }

    XOceanusProcessor& processor; // #714: kept to resolve engine names
    int route; // 1–4

    juce::ToggleButton activeButton;
    GalleryKnob amountKnob;
    juce::ComboBox typeBox;
    juce::ComboBox sourceBox;
    juce::ComboBox targetBox;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> activeAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> amountAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> typeAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> sourceAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> targetAttach;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CouplingPopover)
};

//==============================================================================
// CouplingArcHitTester — transparent overlay for the OverviewPanel that turns
// coupling arcs into clickable targets.
//
// Architecture:
//   1. Call updateArcs() from OverviewPanel::refresh() with the current route
//      list and the 4 node-centre positions (in this component's local coords).
//   2. Layer this component ON TOP of the OverviewPanel in the editor z-order.
//   3. On mouseDown, the hit-test walks the stroked arc paths; if a hit is
//      found it creates a CouplingPopover for that route and launches it in a
//      juce::CallOutBox anchored to the click point.
//
// The component paints nothing — it is purely an input-event router.
// setInterceptsMouseClicks(true, false): catches own clicks, passes child clicks
// through (there are no children).
//
// Hit-testing detail:
//   Each active route stores a juce::Path built from the same Bézier geometry
//   as OverviewPanel's mini arc diagram, then widened to a 6px stroke via
//   juce::PathStrokeType::createStrokedPath. This gives physically accurate
//   hit areas that match the visible arcs without any rectangle-approximation.
//
class CouplingArcHitTester : public juce::Component
{
public:
    explicit CouplingArcHitTester(XOceanusProcessor& proc) : processor(proc)
    {
        setInterceptsMouseClicks(true, false);
        setPaintingIsUnclipped(true);
        A11y::setup(*this, "Coupling Arc Hit Area", "Click on a coupling arc to open its route editor");
    }

    // Called from OverviewPanel::refresh() after its own arc geometry is rebuilt.
    // nodeCenters must be in THIS component's local coordinate space.
    // (Caller must convert from OverviewPanel's local coords if the two
    //  components do not share the same origin.)
    void updateArcs(const std::vector<MegaCouplingMatrix::CouplingRoute>& routes,
                    const std::array<juce::Point<float>, MegaCouplingMatrix::MaxSlots>& nodeCenters)
    {
        clickableArcs.clear();

        for (int i = 0; i < (int)routes.size(); ++i)
        {
            const auto& r = routes[static_cast<size_t>(i)];
            if (!r.active || r.amount < 0.005f)
                continue;
            if (r.sourceSlot < 0 || r.sourceSlot >= MegaCouplingMatrix::MaxSlots)
                continue;
            if (r.destSlot < 0 || r.destSlot >= MegaCouplingMatrix::MaxSlots)
                continue;

            auto from = nodeCenters[static_cast<size_t>(r.sourceSlot)];
            auto to = nodeCenters[static_cast<size_t>(r.destSlot)];

            // Replicate OverviewPanel's quadratic Bézier geometry (bow upward)
            float midX = (from.x + to.x) * 0.5f;
            float midY = (from.y + to.y) * 0.5f - 12.0f;

            juce::Path thinArc;
            thinArc.startNewSubPath(from);
            thinArc.quadraticTo(juce::Point<float>(midX, midY), to);

            // Widen to a 6px stroked region for comfortable hit-testing
            juce::Path stroked;
            juce::PathStrokeType(6.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded)
                .createStrokedPath(stroked, thinArc);

            // APVTS route index: routes vector is 0-indexed but param IDs are 1-indexed.
            // We use the position in the routes vector + 1 if the routes map directly
            // to cp_r1..cp_r4. Prefer using the route's vector position clamped to [1,4].
            int routeParamIdx = juce::jlimit(1, 4, i + 1);

            clickableArcs.push_back({std::move(stroked), routeParamIdx});
        }
    }

    void paint(juce::Graphics&) override
    {
        // Intentionally empty — this component is invisible.
        // All visual rendering is handled by OverviewPanel beneath this overlay.
    }

    void mouseDown(const juce::MouseEvent& e) override
    {
        const auto clickPt = e.getPosition().toFloat();

        for (const auto& arc : clickableArcs)
        {
            if (arc.strokePath.contains(clickPt))
            {
                showPopoverForRoute(arc.routeIndex, e.getScreenPosition());
                return;
            }
        }
        // Click missed all arcs — pass through (no action).
    }

    void mouseMove(const juce::MouseEvent& e) override
    {
        // Provide cursor feedback: hand cursor over arcs, default elsewhere.
        const auto movePt = e.getPosition().toFloat();
        bool overArc = false;
        for (const auto& arc : clickableArcs)
        {
            if (arc.strokePath.contains(movePt))
            {
                overArc = true;
                break;
            }
        }
        setMouseCursor(overArc ? juce::MouseCursor::PointingHandCursor : juce::MouseCursor::NormalCursor);
    }

private:
    struct ClickableArc
    {
        juce::Path strokePath; // Widened Bézier — used for contains() hit-test
        int routeIndex;        // 1–4 → maps to cp_r1_* … cp_r4_*
    };

    void showPopoverForRoute(int routeIdx, juce::Point<int> screenAnchor)
    {
        // Heap-allocate; CallOutBox takes ownership and deletes on close.
        auto* popover = new CouplingPopover(processor, routeIdx);

        // Convert screen anchor to local component coords for the CallOutBox
        // source rectangle (a 1×1 rect at the click point is sufficient).
        auto localPt = getLocalPoint(nullptr, screenAnchor);
        juce::Rectangle<int> anchorRect(localPt.x, localPt.y, 1, 1);

        juce::CallOutBox::launchAsynchronously(std::unique_ptr<juce::Component>(popover), anchorRect, this);
    }

    XOceanusProcessor& processor;
    std::vector<ClickableArc> clickableArcs;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CouplingArcHitTester)
};

} // namespace xoceanus
