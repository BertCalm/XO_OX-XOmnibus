#pragma once
// CouplingInspectorPanel.h — C2 "COUPLE" tab content for SidebarPanel (320pt wide).
//
// Layout (top-to-bottom):
//   1. Mini coupling graph (320 × 120pt) — CouplingVisualizer embedded at reduced height.
//   2. Four collapsible route cards — each wired to cp_r{N}_{active,type,amount,source,target}.
//   3. Action row (32pt) — BAKE | CLR | coupling preset dropdown.
//
// Architecture: header-only (.h), created by SidebarPanel when the COUPLE tab is
// first shown (lazy construction mirrors the PresetBrowser pattern).
//
// Patterns followed: GalleryColors, GalleryFonts, GalleryKnob, A11y, enableKnobReset.

#include <juce_audio_processors/juce_audio_processors.h>
#include "../../XOlokunProcessor.h"
#include "../../Core/MegaCouplingMatrix.h"
#include "../../Core/CouplingPresetManager.h"
#include "../CouplingVisualizer/CouplingVisualizer.h"
#include "../GalleryColors.h"
#include "GalleryKnob.h"

namespace xolokun {

//==============================================================================
// CouplingInspectorPanel
//==============================================================================
class CouplingInspectorPanel : public juce::Component
{
public:
    //==========================================================================
    explicit CouplingInspectorPanel (XOlokunProcessor& proc)
        : processor (proc),
          apvts     (proc.getAPVTS()),
          miniViz   (proc.getCouplingMatrix(),
                     [&proc] (int slot) -> juce::String
                     {
                         auto* eng = proc.getEngine (slot);
                         return eng ? eng->getEngineId() : juce::String{};
                     },
                     [&proc] (int slot) -> juce::Colour
                     {
                         auto* eng = proc.getEngine (slot);
                         return eng ? eng->getAccentColour()
                                    : juce::Colour (0xFF555555);
                     })
    {
        setTitle ("Coupling Inspector");
        setDescription ("Coupling route editor: mini graph, 4 route cards, and BAKE/CLR controls");

        // ── Mini coupling graph ────────────────────────────────────────────────
        addAndMakeVisible (miniViz);
        miniViz.start();

        // ── Route cards (4) ───────────────────────────────────────────────────
        for (int r = 0; r < kNumRoutes; ++r)
        {
            auto& card   = routeCards[r];
            juce::String prefix = "cp_r" + juce::String (r + 1) + "_";
            juce::String routeLabel = "ROUTE " + juce::String (r + 1);

            // Header — click anywhere on it to expand/collapse
            card.headerBtn.setButtonText (routeLabel);
            card.headerBtn.setClickingTogglesState (false);
            card.headerBtn.setColour (juce::TextButton::buttonColourId,
                                      GalleryColors::get (GalleryColors::shellWhite()));
            card.headerBtn.setColour (juce::TextButton::buttonOnColourId,
                                      GalleryColors::get (GalleryColors::shellWhite()));
            card.headerBtn.setColour (juce::TextButton::textColourOffId,
                                      GalleryColors::get (GalleryColors::textDark()));
            card.headerBtn.setColour (juce::TextButton::textColourOnId,
                                      GalleryColors::get (GalleryColors::textDark()));
            A11y::setup (card.headerBtn,
                         routeLabel + " expand/collapse",
                         "Toggle route " + juce::String (r + 1) + " detail view");
            card.headerBtn.onClick = [this, r] { toggleCard (r); };
            addAndMakeVisible (card.headerBtn);

            // Active toggle — right side of header row
            card.activeBtn.setButtonText ("ON");
            card.activeBtn.setClickingTogglesState (true);
            card.activeBtn.setColour (juce::TextButton::buttonColourId,
                                      GalleryColors::get (GalleryColors::slotBg()));
            card.activeBtn.setColour (juce::TextButton::buttonOnColourId,
                                      GalleryColors::get (GalleryColors::xoGold));
            card.activeBtn.setColour (juce::TextButton::textColourOffId,
                                      GalleryColors::get (GalleryColors::textMid()));
            card.activeBtn.setColour (juce::TextButton::textColourOnId,
                                      GalleryColors::get (GalleryColors::textDark()));
            A11y::setup (card.activeBtn,
                         "Route " + juce::String (r + 1) + " enable",
                         "Activate or deactivate coupling route " + juce::String (r + 1));
            card.activeAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
                apvts, prefix + "active", card.activeBtn);
            addAndMakeVisible (card.activeBtn);

            // Type selector — 15 coupling types (14 existing + TriangularCoupling)
            card.typeBox.addItemList ({
                "AmpToFilter", "AmpToPitch", "LFOToPitch", "EnvToMorph",
                "AudioToFM", "AudioToRing", "FilterToFilter", "AmpToChoke",
                "RhythmToBlend", "EnvToDecay", "PitchToPitch", "AudioToWavetable",
                "AudioToBuffer", "KnotTopology", "TriangularCoupling"
            }, 1);
            card.typeBox.setTextWhenNoChoicesAvailable ("No types");
            card.typeBox.setColour (juce::ComboBox::backgroundColourId,
                                    GalleryColors::get (GalleryColors::shellWhite()));
            card.typeBox.setColour (juce::ComboBox::outlineColourId,
                                    GalleryColors::get (GalleryColors::borderGray()));
            card.typeBox.setColour (juce::ComboBox::textColourId,
                                    GalleryColors::get (GalleryColors::textDark()));
            A11y::setup (card.typeBox,
                         "Route " + juce::String (r + 1) + " coupling type",
                         "Select the coupling modulation type for route " + juce::String (r + 1));
            card.typeAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
                apvts, prefix + "type", card.typeBox);
            addChildComponent (card.typeBox);  // hidden until expanded

            // Amount knob (36×36, bipolar)
            card.amountKnob.setSliderStyle (juce::Slider::RotaryVerticalDrag);
            card.amountKnob.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
            card.amountKnob.setColour (juce::Slider::rotarySliderFillColourId,
                                       GalleryColors::get (GalleryColors::xoGold));
            card.amountKnob.setColour (juce::Slider::rotarySliderOutlineColourId,
                                       GalleryColors::get (GalleryColors::borderGray()));
            card.amountKnob.setTooltip ("Route " + juce::String (r + 1) + " amount (bipolar)");
            A11y::setup (card.amountKnob,
                         "Route " + juce::String (r + 1) + " amount",
                         "Bipolar modulation depth for coupling route " + juce::String (r + 1));
            card.amountAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
                apvts, prefix + "amount", card.amountKnob);
            enableKnobReset (card.amountKnob, apvts, prefix + "amount");
            addChildComponent (card.amountKnob);  // hidden until expanded

            // Amount label below knob
            card.amountLabel.setText ("AMT", juce::dontSendNotification);
            card.amountLabel.setFont (GalleryFonts::heading (8.0f));
            card.amountLabel.setColour (juce::Label::textColourId,
                                        GalleryColors::get (GalleryColors::textMid()));
            card.amountLabel.setJustificationType (juce::Justification::centred);
            addChildComponent (card.amountLabel);  // hidden until expanded

            // Source slot selector (5 slots: Ghost Slot at position 5)
            card.sourceBox.addItemList (
                { "Slot 1", "Slot 2", "Slot 3", "Slot 4", "Slot 5 (Ghost)" }, 1);
            card.sourceBox.setColour (juce::ComboBox::backgroundColourId,
                                      GalleryColors::get (GalleryColors::shellWhite()));
            card.sourceBox.setColour (juce::ComboBox::outlineColourId,
                                      GalleryColors::get (GalleryColors::borderGray()));
            card.sourceBox.setColour (juce::ComboBox::textColourId,
                                      GalleryColors::get (GalleryColors::textDark()));
            A11y::setup (card.sourceBox,
                         "Route " + juce::String (r + 1) + " source slot",
                         "Engine slot that is the coupling source for route " + juce::String (r + 1));
            card.sourceAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
                apvts, prefix + "source", card.sourceBox);
            addChildComponent (card.sourceBox);  // hidden until expanded

            // Target slot selector (5 slots)
            card.targetBox.addItemList (
                { "Slot 1", "Slot 2", "Slot 3", "Slot 4", "Slot 5 (Ghost)" }, 1);
            card.targetBox.setColour (juce::ComboBox::backgroundColourId,
                                      GalleryColors::get (GalleryColors::shellWhite()));
            card.targetBox.setColour (juce::ComboBox::outlineColourId,
                                      GalleryColors::get (GalleryColors::borderGray()));
            card.targetBox.setColour (juce::ComboBox::textColourId,
                                      GalleryColors::get (GalleryColors::textDark()));
            A11y::setup (card.targetBox,
                         "Route " + juce::String (r + 1) + " target slot",
                         "Engine slot that receives the coupling signal for route " + juce::String (r + 1));
            card.targetAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
                apvts, prefix + "target", card.targetBox);
            addChildComponent (card.targetBox);  // hidden until expanded

            // All cards collapsed by default
            card.expanded = false;
        }

        // ── BAKE button ────────────────────────────────────────────────────────
        addAndMakeVisible (bakeBtn);
        bakeBtn.setButtonText ("BAKE");
        bakeBtn.setTooltip ("Save current coupling routes as a reusable coupling preset");
        bakeBtn.setColour (juce::TextButton::buttonColourId,
                           GalleryColors::get (GalleryColors::xoGold).withAlpha (0.15f));
        bakeBtn.setColour (juce::TextButton::textColourOffId,
                           GalleryColors::get (GalleryColors::xoGold));
        A11y::setup (bakeBtn, "Bake Coupling",
                     "Save the current coupling route configuration as a named preset");
        bakeBtn.onClick = [this] { handleBake(); };

        // ── CLR button ─────────────────────────────────────────────────────────
        addAndMakeVisible (clearBtn);
        clearBtn.setButtonText ("CLR");
        clearBtn.setTooltip ("Clear all active coupling routes");
        clearBtn.setColour (juce::TextButton::buttonColourId,
                            GalleryColors::get (GalleryColors::slotBg()));
        clearBtn.setColour (juce::TextButton::textColourOffId,
                            GalleryColors::get (GalleryColors::textMid()));
        A11y::setup (clearBtn, "Clear Coupling",
                     "Deactivate all coupling routes and reset the overlay");
        clearBtn.onClick = [this] {
            processor.getCouplingPresetManager().clearOverlay();
            miniViz.refresh();
            repaint();
        };

        // ── Coupling preset dropdown ────────────────────────────────────────────
        addAndMakeVisible (couplingPresetBox);
        couplingPresetBox.setTextWhenNothingSelected ("Coupling Presets...");
        couplingPresetBox.setTooltip ("Load a saved coupling preset");
        couplingPresetBox.setColour (juce::ComboBox::backgroundColourId,
                                     GalleryColors::get (GalleryColors::shellWhite()));
        couplingPresetBox.setColour (juce::ComboBox::outlineColourId,
                                     GalleryColors::get (GalleryColors::borderGray()));
        couplingPresetBox.setColour (juce::ComboBox::textColourId,
                                     GalleryColors::get (GalleryColors::textDark()));
        A11y::setup (couplingPresetBox, "Coupling Preset Selector",
                     "Choose a saved coupling preset to load");
        couplingPresetBox.onChange = [this] { handlePresetSelected(); };

        refreshPresetList();
    }

    ~CouplingInspectorPanel() override = default;

    //==========================================================================
    // Called by SidebarPanel when the COUPLE tab becomes active.
    void refresh()
    {
        miniViz.refresh();
        refreshPresetList();
        repaint();
    }

    //==========================================================================
    void paint (juce::Graphics& g) override
    {
        using namespace GalleryColors;

        g.fillAll (get (shellWhite()));

        // ── Hairline separator below mini viz ─────────────────────────────────
        g.setColour (get (borderGray()));
        g.drawHorizontalLine (kMiniVizH, 0.0f, static_cast<float> (getWidth()));

        // ── Route card backgrounds ─────────────────────────────────────────────
        int y = kMiniVizH + kCardGap;
        for (int r = 0; r < kNumRoutes; ++r)
        {
            int cardH = routeCards[r].expanded ? kCardExpandedH : kCardCollapsedH;

            juce::Rectangle<float> cardRect (
                static_cast<float> (kCardMargin),
                static_cast<float> (y + kCardGap),
                static_cast<float> (getWidth() - kCardMargin * 2),
                static_cast<float> (cardH - kCardGap));

            g.setColour (get (slotBg()));
            g.fillRoundedRectangle (cardRect, 4.0f);
            g.setColour (get (borderGray()));
            g.drawRoundedRectangle (cardRect, 4.0f, 0.5f);

            // XO Gold left accent bar when active
            bool active = isRouteActive (r);
            if (active)
            {
                g.setColour (get (xoGold));
                g.fillRoundedRectangle (
                    cardRect.getX(),
                    cardRect.getY() + 4.0f,
                    3.0f,
                    cardRect.getHeight() - 8.0f,
                    1.5f);
            }

            // Expand/collapse chevron
            auto chevronBounds = juce::Rectangle<float> (
                cardRect.getRight() - 20.0f,
                cardRect.getY() + (kCardCollapsedH - kCardGap) * 0.5f - 5.0f,
                10.0f, 10.0f);

            g.setColour (get (textMid()));
            float cx = chevronBounds.getCentreX();
            float cy = chevronBounds.getCentreY();
            float hs = 4.0f; // half-span

            if (routeCards[r].expanded)
            {
                // Up chevron ∧
                g.drawLine (cx - hs, cy + 2.5f, cx, cy - 2.5f, 1.5f);
                g.drawLine (cx, cy - 2.5f, cx + hs, cy + 2.5f, 1.5f);
            }
            else
            {
                // Down chevron ∨
                g.drawLine (cx - hs, cy - 2.5f, cx, cy + 2.5f, 1.5f);
                g.drawLine (cx, cy + 2.5f, cx + hs, cy - 2.5f, 1.5f);
            }

            // Source → target annotation when collapsed
            if (!routeCards[r].expanded)
            {
                juce::String srcName = slotName (getSlotIndex (r, "source"));
                juce::String tgtName = slotName (getSlotIndex (r, "target"));
                juce::String routeSummary = srcName + " \xe2\x86\x92 " + tgtName;

                g.setColour (get (textMid()));
                g.setFont (GalleryFonts::body (9.0f));
                g.drawText (routeSummary,
                            juce::Rectangle<int> (
                                (int) cardRect.getX() + 52,
                                (int) cardRect.getY(),
                                (int) cardRect.getWidth() - 80,
                                kCardCollapsedH - kCardGap),
                            juce::Justification::centredLeft);
            }

            // Type badge when expanded
            if (routeCards[r].expanded)
            {
                // "TYPE" label above the type combo
                auto typeLabel = juce::Rectangle<int> (
                    (int) cardRect.getX() + kInnerPad,
                    (int) cardRect.getY() + kHeaderRowH + kRowGap,
                    60, 12);
                g.setColour (get (textMid()));
                g.setFont (GalleryFonts::heading (8.0f));
                g.drawText ("TYPE", typeLabel, juce::Justification::centredLeft);

                // "SRC / TGT" label above source/target row
                auto srcTgtLabel = juce::Rectangle<int> (
                    (int) cardRect.getX() + kInnerPad,
                    (int) cardRect.getY() + kHeaderRowH + kRowGap + kLabelH + kTypeRowH + kRowGap,
                    (int) cardRect.getWidth() - kInnerPad * 2, 12);
                g.drawText ("SRC / TGT", srcTgtLabel, juce::Justification::centredLeft);
            }

            y += cardH;
        }

        // ── Hairline above action row ──────────────────────────────────────────
        g.setColour (get (borderGray()));
        g.drawHorizontalLine (getHeight() - kActionRowH, 0.0f,
                              static_cast<float> (getWidth()));

        // ── Focus rings for keyboard-focused route header buttons ──────────────
        for (int r = 0; r < kNumRoutes; ++r)
        {
            if (routeCards[r].headerBtn.hasKeyboardFocus (false))
                A11y::drawFocusRing (g, routeCards[r].headerBtn.getBounds().toFloat(), 3.0f);
        }
    }

    void resized() override
    {
        const int w = getWidth();

        // ── Mini viz ──────────────────────────────────────────────────────────
        miniViz.setBounds (0, 0, w, kMiniVizH);

        // ── Route cards ───────────────────────────────────────────────────────
        int y = kMiniVizH + kCardGap;
        for (int r = 0; r < kNumRoutes; ++r)
        {
            auto& card = routeCards[r];
            int cardH  = card.expanded ? kCardExpandedH : kCardCollapsedH;

            // Card inner rect (mirrors paint)
            int cardX = kCardMargin;
            int cardY = y + kCardGap;
            int cardW = w - kCardMargin * 2;

            // Header button spans the full header row
            card.headerBtn.setBounds (cardX, cardY, cardW - kActiveBtnW - 4, kHeaderRowH);
            card.activeBtn.setBounds (cardX + cardW - kActiveBtnW, cardY + 3,
                                      kActiveBtnW - 4, kHeaderRowH - 6);

            if (card.expanded)
            {
                // Row 1 (type label + type combo + amount knob)
                int detailY   = cardY + kHeaderRowH + kRowGap + kLabelH;
                int typeW     = cardW - kInnerPad * 2 - kKnobSize - kRowGap;

                card.typeBox.setBounds (cardX + kInnerPad, detailY, typeW, kTypeRowH);
                card.amountKnob.setBounds (cardX + kInnerPad + typeW + kRowGap, detailY,
                                           kKnobSize, kKnobSize);
                card.amountLabel.setBounds (cardX + kInnerPad + typeW + kRowGap,
                                            detailY + kKnobSize + 1,
                                            kKnobSize, 10);

                // Row 2 (source / target)
                int srcTgtY = detailY + kTypeRowH + kRowGap + kLabelH;
                int halfW   = (cardW - kInnerPad * 2 - kArrowW) / 2;
                card.sourceBox.setBounds (cardX + kInnerPad, srcTgtY, halfW, kComboRowH);
                card.targetBox.setBounds (cardX + kInnerPad + halfW + kArrowW,
                                          srcTgtY, halfW, kComboRowH);

                card.typeBox.setVisible (true);
                card.amountKnob.setVisible (true);
                card.amountLabel.setVisible (true);
                card.sourceBox.setVisible (true);
                card.targetBox.setVisible (true);
            }
            else
            {
                card.typeBox.setVisible (false);
                card.amountKnob.setVisible (false);
                card.amountLabel.setVisible (false);
                card.sourceBox.setVisible (false);
                card.targetBox.setVisible (false);
            }

            y += cardH;
        }

        // ── Action row ────────────────────────────────────────────────────────
        auto actionRow = juce::Rectangle<int> (0, getHeight() - kActionRowH, w, kActionRowH);
        actionRow = actionRow.reduced (kCardMargin, 4);

        bakeBtn.setBounds (actionRow.removeFromLeft (50).reduced (0, 2));
        actionRow.removeFromLeft (4);
        clearBtn.setBounds (actionRow.removeFromLeft (40).reduced (0, 2));
        actionRow.removeFromLeft (4);
        couplingPresetBox.setBounds (actionRow.reduced (0, 2));
    }

    //==========================================================================
    void lookAndFeelChanged() override { repaint(); }

private:
    //==========================================================================
    // Layout constants
    //==========================================================================
    static constexpr int kMiniVizH       = 120;  // mini graph height
    static constexpr int kCardGap        = 4;    // vertical gap between cards
    static constexpr int kCardMargin     = 6;    // left/right inset for cards
    static constexpr int kInnerPad       = 8;    // inner padding inside card
    static constexpr int kHeaderRowH     = 24;   // height of header row (label + toggle)
    static constexpr int kActiveBtnW     = 38;   // width of ON/OFF toggle
    static constexpr int kLabelH         = 12;   // micro-label height (TYPE / SRC/TGT)
    static constexpr int kTypeRowH       = 22;   // type combo height
    static constexpr int kComboRowH      = 22;   // source/target combo height
    static constexpr int kKnobSize       = 36;   // amount knob square size
    static constexpr int kArrowW         = 16;   // gap between source and target combos
    static constexpr int kRowGap         = 4;    // gap between rows within card
    static constexpr int kActionRowH     = 32;   // bottom action bar height
    static constexpr int kNumRoutes      = 4;

    // Collapsed card: header row + gaps + outer border allowance
    static constexpr int kCardCollapsedH = kCardGap + kHeaderRowH + kCardGap + 2;

    // Expanded card: header + label + type row + label + src/tgt row + bottom pad
    static constexpr int kCardExpandedH  =
        kCardGap
        + kHeaderRowH + kRowGap
        + kLabelH + kTypeRowH + kRowGap
        + kLabelH + kComboRowH
        + kInnerPad + 2;

    //==========================================================================
    // RouteCard — one collapsible card per coupling route
    //==========================================================================
    struct RouteCard
    {
        bool expanded = false;

        juce::TextButton headerBtn;
        juce::TextButton activeBtn;
        juce::ComboBox   typeBox;
        GalleryKnob      amountKnob;
        juce::Label      amountLabel;
        juce::ComboBox   sourceBox;
        juce::ComboBox   targetBox;

        // APVTS attachments — declared after controls to be destroyed first
        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>   activeAttach;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> typeAttach;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>   amountAttach;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> sourceAttach;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> targetAttach;
    };

    //==========================================================================
    // Toggle expand/collapse for a single card; re-layout and repaint.
    void toggleCard (int r)
    {
        jassert (juce::isPositiveAndBelow (r, kNumRoutes));
        routeCards[r].expanded = !routeCards[r].expanded;

        // Update header button text with indicator
        juce::String routeLabel = "ROUTE " + juce::String (r + 1);
        routeCards[r].headerBtn.setButtonText (routeLabel);

        resized();
        repaint();
    }

    //==========================================================================
    // Read whether a route is currently active (from APVTS).
    bool isRouteActive (int r) const
    {
        juce::String paramId = "cp_r" + juce::String (r + 1) + "_active";
        if (auto* param = apvts.getRawParameterValue (paramId))
            return param->load() > 0.5f;
        return false;
    }

    //==========================================================================
    // Read source or target slot index (0-based) from APVTS.
    int getSlotIndex (int r, const juce::String& suffix) const
    {
        juce::String paramId = "cp_r" + juce::String (r + 1) + "_" + suffix;
        if (auto* param = apvts.getRawParameterValue (paramId))
            return juce::roundToInt (param->load());
        return 0;
    }

    //==========================================================================
    // Resolve a 0-based slot index to a short display name.
    juce::String slotName (int slot) const
    {
        auto* eng = processor.getEngine (slot);
        if (eng)
            return eng->getEngineId().toUpperCase().substring (0, 4);

        if (slot == 4)
            return "GHST";  // Ghost Slot (slot 5)

        return "-";  // dash for empty slot
    }

    //==========================================================================
    // Refresh the coupling preset dropdown from CouplingPresetManager library.
    void refreshPresetList()
    {
        couplingPresetBox.clear (juce::dontSendNotification);
        auto& cpm   = processor.getCouplingPresetManager();
        auto  names = cpm.getPresetNames();

        if (names.isEmpty())
        {
            couplingPresetBox.setTextWhenNothingSelected ("No presets saved");
        }
        else
        {
            couplingPresetBox.setTextWhenNothingSelected ("Coupling Presets...");
            for (int i = 0; i < names.size(); ++i)
                couplingPresetBox.addItem (names[i], i + 1);
        }
    }

    //==========================================================================
    // BAKE — capture current overlay; prompt for name; save to disk.
    void handleBake()
    {
        auto& cpm = processor.getCouplingPresetManager();

        // Nothing to bake — give brief visual feedback on the button
        auto snapshot = cpm.bakeCurrent ("Preview");
        if (!snapshot.hasActiveRoutes())
        {
            bakeBtn.setColour (juce::TextButton::buttonColourId,
                               juce::Colour (0x40FF4444));
            juce::Timer::callAfterDelay (
                400,
                [safeThis = juce::Component::SafePointer<CouplingInspectorPanel> (this)] {
                    if (safeThis)
                        safeThis->bakeBtn.setColour (
                            juce::TextButton::buttonColourId,
                            GalleryColors::get (GalleryColors::xoGold).withAlpha (0.15f));
                });
            return;
        }

        // Name input dialog
        auto* dlg = new juce::AlertWindow (
            "Bake Coupling Preset",
            "Enter a name for this coupling configuration:",
            juce::MessageBoxIconType::NoIcon, this);
        dlg->addTextEditor ("name", "Untitled Coupling", "Preset Name:");
        dlg->addButton ("Save",   1, juce::KeyPress (juce::KeyPress::returnKey));
        dlg->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));

        dlg->enterModalState (
            true,
            juce::ModalCallbackFunction::create (
                [safeThis = juce::Component::SafePointer<CouplingInspectorPanel> (this),
                 dlg] (int result) {
                    if (result == 1 && safeThis)
                    {
                        auto name = dlg->getTextEditorContents ("name").trim();
                        if (name.isEmpty())
                            name = "Untitled Coupling";
                        safeThis->performBake (name);
                    }
                    delete dlg;
                }),
            false);
    }

    void performBake (const juce::String& presetName)
    {
        auto& cpm          = processor.getCouplingPresetManager();
        auto  state        = cpm.bakeCurrent (presetName);
        state.author       = "User";

        auto dir           = CouplingPresetManager::getDefaultDirectory();
        auto sanitized     = presetName.replaceCharacters (" /\\:*?\"<>|",
                                                            "___________");
        auto file          = dir.getChildFile (sanitized + ".xocoupling");

        int suffix = 1;
        while (file.existsAsFile() && suffix < 100)
        {
            file = dir.getChildFile (sanitized + "_" + juce::String (suffix)
                                     + ".xocoupling");
            ++suffix;
        }

        if (cpm.saveToFile (file, state))
        {
            cpm.scanDirectory (CouplingPresetManager::getDefaultDirectory());
            refreshPresetList();

            // Brief XO Gold flash on the BAKE button as confirmation
            bakeBtn.setColour (juce::TextButton::buttonColourId,
                               GalleryColors::get (GalleryColors::xoGold).withAlpha (0.6f));
            juce::Timer::callAfterDelay (
                350,
                [safeThis = juce::Component::SafePointer<CouplingInspectorPanel> (this)] {
                    if (safeThis)
                        safeThis->bakeBtn.setColour (
                            juce::TextButton::buttonColourId,
                            GalleryColors::get (GalleryColors::xoGold).withAlpha (0.15f));
                });
        }
    }

    //==========================================================================
    // Coupling preset recall — load selected preset into APVTS.
    void handlePresetSelected()
    {
        int selectedId = couplingPresetBox.getSelectedId();
        if (selectedId <= 0)
            return;

        auto& cpm   = processor.getCouplingPresetManager();
        const auto* preset = cpm.getPreset (selectedId - 1);
        if (preset)
        {
            cpm.loadBakedCoupling (*preset);
            miniViz.refresh();
            repaint();
        }
    }

    //==========================================================================
    // Members
    //==========================================================================
    XOlokunProcessor&                    processor;
    juce::AudioProcessorValueTreeState&  apvts;

    // Mini coupling graph (top 120pt)
    CouplingVisualizer miniViz;

    // Four collapsible route cards
    std::array<RouteCard, kNumRoutes> routeCards;

    // Action row controls
    juce::TextButton bakeBtn;
    juce::TextButton clearBtn;
    juce::ComboBox   couplingPresetBox;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CouplingInspectorPanel)
};

} // namespace xolokun
