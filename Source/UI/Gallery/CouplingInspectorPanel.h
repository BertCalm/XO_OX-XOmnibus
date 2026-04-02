// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
// CouplingInspectorPanel.h — C2 "COUPLE" tab content for SidebarPanel (320pt wide).
//
// Layout (top-to-bottom):
//   1. Mini coupling graph (320 × 96pt) — CouplingVisualizer embedded at reduced height.
//   2. Four collapsible route cards — each wired to cp_r{N}_{active,type,amount,source,target}.
//   3. Action row (32pt) — BAKE | CLR | coupling preset dropdown.
//
// Architecture: header-only (.h), created by SidebarPanel when the COUPLE tab is
// first shown (lazy construction mirrors the PresetBrowser pattern).
//
// Patterns followed: GalleryColors, GalleryFonts, GalleryKnob, A11y, enableKnobReset.
// Visual polish pass (2026-03-27): dark theme, route card info density, gold BAKE.

#include <juce_audio_processors/juce_audio_processors.h>
#include "../../XOceanusProcessor.h"
#include "../../Core/MegaCouplingMatrix.h"
#include "../../Core/CouplingPresetManager.h"
#include "../CouplingVisualizer/CouplingVisualizer.h"
#include "../GalleryColors.h"
#include "GalleryKnob.h"

namespace xoceanus {

//==============================================================================
// CouplingInspectorPanel
//==============================================================================
class CouplingInspectorPanel : public juce::Component
{
public:
    //==========================================================================
    explicit CouplingInspectorPanel (XOceanusProcessor& proc)
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

            // Header — transparent button so the card background shows through;
            // text uses T1 for the route number, drawn directly in paint().
            card.headerBtn.setButtonText ("R" + juce::String(r + 1) + "  —  tap to configure");
            card.headerBtn.setClickingTogglesState (false);
            card.headerBtn.setColour (juce::TextButton::buttonColourId,
                                      juce::Colours::transparentBlack);
            card.headerBtn.setColour (juce::TextButton::buttonOnColourId,
                                      juce::Colours::transparentBlack);
            card.headerBtn.setColour (juce::TextButton::textColourOffId,
                                      GalleryColors::get(GalleryColors::t2()));
            card.headerBtn.setColour (juce::TextButton::textColourOnId,
                                      GalleryColors::get(GalleryColors::t1()));
            A11y::setup (card.headerBtn,
                         routeLabel + " expand/collapse",
                         "Toggle route " + juce::String (r + 1) + " detail view");
            card.headerBtn.onClick = [this, r] { toggleCard (r); };
            addAndMakeVisible (card.headerBtn);

            // Active toggle — right side of header row
            // OFF: dark pill (raised bg, T3 text).  ON: green with white text.
            card.activeBtn.setButtonText ("ON");
            card.activeBtn.setClickingTogglesState (true);
            card.activeBtn.setColour (juce::TextButton::buttonColourId,
                                      GalleryColors::get (GalleryColors::raised()));
            card.activeBtn.setColour (juce::TextButton::buttonOnColourId,
                                      juce::Colour (0xFF2D6A4F));  // muted forest green
            card.activeBtn.setColour (juce::TextButton::textColourOffId,
                                      GalleryColors::get (GalleryColors::t3()));
            card.activeBtn.setColour (juce::TextButton::textColourOnId,
                                      juce::Colour (0xFF52B788));  // bright mint when on
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
                                    GalleryColors::get (GalleryColors::raised()));
            card.typeBox.setColour (juce::ComboBox::outlineColourId,
                                    GalleryColors::border().withAlpha (0.18f));
            card.typeBox.setColour (juce::ComboBox::textColourId,
                                    GalleryColors::get (GalleryColors::t1()));
            card.typeBox.setColour (juce::ComboBox::arrowColourId,
                                    GalleryColors::get (GalleryColors::t3()));
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
                                       GalleryColors::border().withAlpha (0.22f));
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
                                        GalleryColors::get (GalleryColors::t3()));
            card.amountLabel.setJustificationType (juce::Justification::centred);
            addChildComponent (card.amountLabel);  // hidden until expanded

            // Source slot selector (5 slots: Ghost Slot at position 5)
            card.sourceBox.addItemList (
                { "Slot 1", "Slot 2", "Slot 3", "Slot 4", "Slot 5 (Ghost)" }, 1);
            card.sourceBox.setColour (juce::ComboBox::backgroundColourId,
                                      GalleryColors::get (GalleryColors::raised()));
            card.sourceBox.setColour (juce::ComboBox::outlineColourId,
                                      GalleryColors::border().withAlpha (0.18f));
            card.sourceBox.setColour (juce::ComboBox::textColourId,
                                      GalleryColors::get (GalleryColors::t1()));
            card.sourceBox.setColour (juce::ComboBox::arrowColourId,
                                      GalleryColors::get (GalleryColors::t3()));
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
                                      GalleryColors::get (GalleryColors::raised()));
            card.targetBox.setColour (juce::ComboBox::outlineColourId,
                                      GalleryColors::border().withAlpha (0.18f));
            card.targetBox.setColour (juce::ComboBox::textColourId,
                                      GalleryColors::get (GalleryColors::t1()));
            card.targetBox.setColour (juce::ComboBox::arrowColourId,
                                      GalleryColors::get (GalleryColors::t3()));
            A11y::setup (card.targetBox,
                         "Route " + juce::String (r + 1) + " target slot",
                         "Engine slot that receives the coupling signal for route " + juce::String (r + 1));
            card.targetAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
                apvts, prefix + "target", card.targetBox);
            addChildComponent (card.targetBox);  // hidden until expanded

            // All cards collapsed by default
            card.expanded = false;
        }

        // ── BAKE button — XO Gold accent, matches EXPORT button style ──────────
        addAndMakeVisible (bakeBtn);
        bakeBtn.setButtonText ("BAKE");
        bakeBtn.setTooltip ("Save current coupling routes as a reusable coupling preset");
        bakeBtn.setColour (juce::TextButton::buttonColourId,
                           GalleryColors::get (GalleryColors::xoGold).withAlpha (0.18f));
        bakeBtn.setColour (juce::TextButton::buttonOnColourId,
                           GalleryColors::get (GalleryColors::xoGold).withAlpha (0.35f));
        bakeBtn.setColour (juce::TextButton::textColourOffId,
                           GalleryColors::get (GalleryColors::xoGold));
        bakeBtn.setColour (juce::TextButton::textColourOnId,
                           GalleryColors::get (GalleryColors::xoGold));
        A11y::setup (bakeBtn, "Bake Coupling",
                     "Save the current coupling route configuration as a named preset");
        bakeBtn.onClick = [this] { handleBake(); };

        // ── CLR button — subtle T3 style ────────────────────────────────────────
        addAndMakeVisible (clearBtn);
        clearBtn.setButtonText ("CLR");
        clearBtn.setTooltip ("Clear all active coupling routes");
        clearBtn.setColour (juce::TextButton::buttonColourId,
                            GalleryColors::get (GalleryColors::raised()));
        clearBtn.setColour (juce::TextButton::buttonOnColourId,
                            GalleryColors::get (GalleryColors::raised()).brighter (0.1f));
        clearBtn.setColour (juce::TextButton::textColourOffId,
                            GalleryColors::get (GalleryColors::t3()));
        clearBtn.setColour (juce::TextButton::textColourOnId,
                            GalleryColors::get (GalleryColors::t2()));
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
                                     GalleryColors::get (GalleryColors::raised()));
        couplingPresetBox.setColour (juce::ComboBox::outlineColourId,
                                     GalleryColors::border().withAlpha (0.18f));
        couplingPresetBox.setColour (juce::ComboBox::textColourId,
                                     GalleryColors::get (GalleryColors::t2()));
        couplingPresetBox.setColour (juce::ComboBox::arrowColourId,
                                     GalleryColors::get (GalleryColors::t3()));
        A11y::setup (couplingPresetBox, "Coupling Preset Selector",
                     "Choose a saved coupling preset to load");
        couplingPresetBox.onChange = [this] { handlePresetSelected(); };

        refreshPresetList();
    }

    ~CouplingInspectorPanel() override = default;

    //==========================================================================
    // #390: ESC dismisses any active modal dialog owned by this panel.
    // JUCE AlertWindows already bind ESC to their Cancel button, but adding
    // this override ensures ESC is handled even if focus sits on the panel
    // rather than the dialog.
    bool keyPressed (const juce::KeyPress& key) override
    {
        if (key == juce::KeyPress::escapeKey)
        {
            juce::ModalComponentManager::getInstance()->cancelAllModalComponents();
            return true;
        }
        return false;
    }

    //==========================================================================
    // W20: Stop the miniViz timer when this panel is hidden, restart when shown.
    // Prevents the CouplingVisualizer animation timer from running off-screen.
    void visibilityChanged() override
    {
        if (isVisible())
            miniViz.start();
        else
            miniViz.stop();
    }

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

        // Dark theme background — match the elevated surface behind the sidebar
        g.fillAll (get (surface()));

        // ── Section label at top ─────────────────────────────────────────────
        g.setFont (GalleryFonts::display (11.0f));
        g.setColour (get (t1()));
        g.drawText ("COUPLING ROUTES",
                    juce::Rectangle<int> (kCardMargin, 6, 180, 20),
                    juce::Justification::centredLeft);

        // ── Route card backgrounds ─────────────────────────────────────────────
        int y = 28;
        for (int r = 0; r < kNumRoutes; ++r)
        {
            int cardH = routeCards[r].expanded ? kCardExpandedH : kCardCollapsedH;
            bool active = isRouteActive (r);

            juce::Rectangle<float> cardRect (
                static_cast<float> (kCardMargin),
                static_cast<float> (y + kCardGap),
                static_cast<float> (getWidth() - kCardMargin * 2),
                static_cast<float> (cardH - kCardGap));

            // Card fill: elevated bg (#242426), slightly brighter when active
            juce::Colour cardFill = get (elevated());
            if (active)
                cardFill = cardFill.brighter (0.04f);

            g.setColour (cardFill);
            g.fillRoundedRectangle (cardRect, 6.0f);

            // Border: 1px rgba(255,255,255,0.07) — or slightly brighter when active
            juce::Colour cardBorder = active ? border().withAlpha (0.14f) : border();
            g.setColour (cardBorder);
            g.drawRoundedRectangle (cardRect, 6.0f, 1.0f);

            // Active left accent bar (3px, rounded) — uses source engine accent (SHOULD A2-02)
            if (active)
            {
                int srcSlotForBar = getSlotIndex (r, "source");
                auto* srcEngForBar = processor.getEngine (srcSlotForBar);
                juce::Colour barColor = srcEngForBar ? srcEngForBar->getAccentColour()
                                                     : juce::Colour (0xFF52B788); // fallback mint
                g.setColour (barColor.withAlpha (0.9f));
                g.fillRoundedRectangle (
                    cardRect.getX(),
                    cardRect.getY() + 5.0f,
                    3.0f,
                    cardRect.getHeight() - 10.0f,
                    1.5f);
            }

            // ── Route number label (T2 when inactive, T1 when active) ─────────
            {
                juce::String routeNumStr = "R" + juce::String (r + 1);
                g.setFont (GalleryFonts::heading (9.0f));
                g.setColour (active ? get (t1()) : get (t2()));
                g.drawText (routeNumStr,
                            juce::Rectangle<int> (
                                (int) cardRect.getX() + kInnerPad + (active ? 5 : 3),
                                (int) cardRect.getY(),
                                28,
                                kCardCollapsedH - kCardGap),
                            juce::Justification::centredLeft);
            }

            // ── Collapsed summary: "SRC → TGT  ·  TypeShort" or placeholder ─
            if (!routeCards[r].expanded)
            {
                int srcSlot = getSlotIndex (r, "source");
                int tgtSlot = getSlotIndex (r, "target");
                juce::String srcName   = slotName (srcSlot);
                juce::String tgtName   = slotName (tgtSlot);
                juce::String typeShort = getTypeShortLabel (r);

                // Show placeholder when route is not active
                if (!active)
                {
                    g.setFont (GalleryFonts::body (10.0f));
                    g.setColour (get (t2()));
                    juce::String placeholder = "R" + juce::String(r + 1) + "  —  tap to configure";
                    g.drawText (placeholder,
                                juce::Rectangle<float> (
                                    cardRect.getX() + kInnerPad + 30.0f,
                                    cardRect.getY(),
                                    cardRect.getWidth() - kInnerPad - 60.0f,
                                    static_cast<float> (kCardCollapsedH - kCardGap)),
                                juce::Justification::centredLeft, false);
                    y += cardH;
                    continue;
                }

                auto* srcEng = processor.getEngine (srcSlot);
                auto* tgtEng = processor.getEngine (tgtSlot);
                // Source name uses src engine accent; target uses tgt engine accent.
                // Fall back to T2 (active) or T3 (inactive) when slot is empty.
                juce::Colour srcColor = srcEng ? srcEng->getAccentColour()
                                               : get (active ? t2() : t3());
                juce::Colour tgtColor = tgtEng ? tgtEng->getAccentColour()
                                               : get (active ? t2() : t3());

                g.setFont (GalleryFonts::body (9.5f));
                auto font = g.getCurrentFont();

                // Starting x for the summary segment
                float sx = static_cast<float> (cardRect.getX()) + kInnerPad + 30.0f;
                float sy = static_cast<float> (cardRect.getY());
                float sh = static_cast<float> (kCardCollapsedH - kCardGap);

                // Draw source name in src accent color
                float srcW = font.getStringWidthFloat (srcName);
                g.setColour (srcColor.withAlpha (active ? 1.0f : 0.6f));
                g.drawText (srcName,
                            juce::Rectangle<float> (sx, sy, srcW + 4.0f, sh),
                            juce::Justification::centredLeft, false);
                sx += srcW + 4.0f;

                // Arrow in T4 (gold)
                juce::String arrow (juce::CharPointer_UTF8 (" \xe2\x86\x92 "));
                float arrowW = font.getStringWidthFloat (arrow);
                g.setColour (get (t4()));
                g.drawText (arrow,
                            juce::Rectangle<float> (sx, sy, arrowW + 2.0f, sh),
                            juce::Justification::centredLeft, false);
                sx += arrowW + 2.0f;

                // Draw target name in tgt accent color
                float tgtW = font.getStringWidthFloat (tgtName);
                g.setColour (tgtColor.withAlpha (active ? 1.0f : 0.6f));
                g.drawText (tgtName,
                            juce::Rectangle<float> (sx, sy, tgtW + 4.0f, sh),
                            juce::Justification::centredLeft, false);
                sx += tgtW + 4.0f;

                // Type badge (T3, dimmed)
                if (typeShort.isNotEmpty())
                {
                    juce::String badge = juce::String (juce::CharPointer_UTF8 ("  \xc2\xb7 ")) + typeShort;
                    float badgeW = font.getStringWidthFloat (badge);
                    g.setColour (get (t3()).withAlpha (active ? 0.8f : 0.5f));
                    g.drawText (badge,
                                juce::Rectangle<float> (sx, sy, badgeW + 4.0f, sh),
                                juce::Justification::centredLeft, false);
                }
            }

            // ── Expand/collapse chevron ────────────────────────────────────────
            {
                float chevX = cardRect.getRight() - 18.0f;
                float chevY = cardRect.getY() + (kCardCollapsedH - kCardGap) * 0.5f;
                float hs = 3.5f;

                g.setColour (get (t3()));
                if (routeCards[r].expanded)
                {
                    // Up chevron ∧
                    g.drawLine (chevX - hs, chevY + 2.0f, chevX, chevY - 2.0f, 1.2f);
                    g.drawLine (chevX, chevY - 2.0f, chevX + hs, chevY + 2.0f, 1.2f);
                }
                else
                {
                    // Down chevron ∨
                    g.drawLine (chevX - hs, chevY - 2.0f, chevX, chevY + 2.0f, 1.2f);
                    g.drawLine (chevX, chevY + 2.0f, chevX + hs, chevY - 2.0f, 1.2f);
                }
            }

            // ── Expanded section labels (TYPE / SRC / TGT) ────────────────────
            if (routeCards[r].expanded)
            {
                g.setFont (GalleryFonts::heading (8.0f));
                g.setColour (get (t3()));

                // "TYPE" label
                g.drawText ("TYPE",
                            juce::Rectangle<int> (
                                (int) cardRect.getX() + kInnerPad,
                                (int) cardRect.getY() + kHeaderRowH + kRowGap,
                                50, kLabelH),
                            juce::Justification::centredLeft);

                // "SRC / TGT" label
                g.drawText ("SRC \xe2\x86\x92 TGT",
                            juce::Rectangle<int> (
                                (int) cardRect.getX() + kInnerPad,
                                (int) cardRect.getY() + kHeaderRowH + kRowGap + kLabelH + kTypeRowH + kRowGap,
                                (int) cardRect.getWidth() - kInnerPad * 2, kLabelH),
                            juce::Justification::centredLeft);
            }

            y += cardH;
        }

        // ── Routing hint: strongest active route summary ──────────────────────
        {
            int    bestRoute  = -1;
            float  bestAmount = 0.0f;
            for (int r = 0; r < kNumRoutes; ++r)
            {
                if (!isRouteActive (r)) continue;
                juce::String amtId = "cp_r" + juce::String (r + 1) + "_amount";
                float amt = 0.0f;
                if (auto* p = apvts.getRawParameterValue (amtId))
                    amt = std::abs (p->load());
                if (amt > bestAmount)
                {
                    bestAmount = amt;
                    bestRoute  = r;
                }
            }

            if (bestRoute >= 0)
            {
                juce::String srcName   = slotName (getSlotIndex (bestRoute, "source"));
                juce::String tgtName   = slotName (getSlotIndex (bestRoute, "target"));
                juce::String typeShort = getTypeShortLabel (bestRoute);
                juce::String hintText  = srcName
                                         + juce::String (juce::CharPointer_UTF8 (" \xe2\x86\x92 "))
                                         + tgtName
                                         + ": " + typeShort
                                         + " " + juce::String (bestAmount, 2);

                // Gap between last card bottom and action row hairline
                int hintY = y + 2;
                int hintH = getHeight() - kActionRowH - hintY;

                if (hintH > 0)
                {
                    g.setFont (GalleryFonts::value (8.5f));
                    g.setColour (get (t3()));
                    g.drawText (hintText,
                                juce::Rectangle<int> (kCardMargin, hintY,
                                                      getWidth() - kCardMargin * 2, hintH),
                                juce::Justification::centred, false);
                }
            }
        }

        // ── Hairline above action row ──────────────────────────────────────────
        g.setColour (border());
        g.drawHorizontalLine (getHeight() - kActionRowH, 0.0f,
                              static_cast<float> (getWidth()));

        // ── Focus rings for keyboard-focused route header buttons ──────────────
        for (int r = 0; r < kNumRoutes; ++r)
        {
            if (routeCards[r].headerBtn.hasKeyboardFocus (false))
                A11y::drawFocusRing (g, routeCards[r].headerBtn.getBounds().toFloat(), 4.0f);
        }
    }

    void resized() override
    {
        const int w = getWidth();

        // ── Mini viz hidden — duplicate of Column A's MiniCouplingGraph; off-screen
        //    to avoid rendering two copies of the same coupling visualization.
        miniViz.setBounds (0, 0, 0, 0);
        miniViz.setVisible(false);

        // ── Route cards — below "COUPLING ROUTES" header ──────────────────────
        int y = 28;
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
    static constexpr int kMiniVizH       = 96;   // mini graph height (reduced from 120)
    static constexpr int kCardGap        = 2;    // vertical gap between cards (tighter)
    static constexpr int kCardMargin     = 6;    // left/right inset for cards
    static constexpr int kInnerPad       = 8;    // inner padding inside card
    static constexpr int kHeaderRowH     = 24;   // height of header row (label + toggle)
    static constexpr int kActiveBtnW     = 34;   // width of ON/OFF toggle
    static constexpr int kLabelH         = 11;   // micro-label height (TYPE / SRC/TGT)
    static constexpr int kTypeRowH       = 22;   // type combo height
    static constexpr int kComboRowH      = 22;   // source/target combo height
    static constexpr int kKnobSize       = 34;   // amount knob square size
    static constexpr int kArrowW         = 14;   // gap between source and target combos
    static constexpr int kRowGap         = 3;    // gap between rows within card
    static constexpr int kActionRowH     = 34;   // bottom action bar height
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
    // Resolve a 0-based slot index to a display name (full engine ID, uppercased).
    juce::String slotName (int slotIdx) const
    {
        if (slotIdx < 0 || slotIdx >= 5) return "NONE";
        auto* eng = processor.getEngine (slotIdx);
        if (!eng) return "EMPTY";
        auto id = eng->getEngineId();
        return id.isEmpty() ? ("SL" + juce::String (slotIdx + 1)) : id.toUpperCase();
    }

    //==========================================================================
    // Return a short type label string for the route summary badge.
    // Reads the type APVTS param (1-based ComboBox index) and maps to abbreviation.
    juce::String getTypeShortLabel (int r) const
    {
        juce::String paramId = "cp_r" + juce::String (r + 1) + "_type";
        if (auto* param = apvts.getRawParameterValue (paramId))
        {
            // ComboBox attachment maps 1-based index → param value 0..N-1
            int idx = juce::roundToInt (param->load());
            static const char* kLabels[] = {
                "Amp>F", "Amp>P", "LFO>P", "Env>M",
                "FM",    "Ring",  "F>F",   "Choke",
                "Rhy>B","Env>D", "P>P",   "WT",
                "A>Buf", "KNOT", "TRI"
            };
            if (idx >= 0 && idx < 15)
                return kLabels[idx];
        }
        return {};
    }

    //==========================================================================
    // Refresh the coupling preset dropdown from CouplingPresetManager library.
    // When empty, use T3-colored placeholder text.
    void refreshPresetList()
    {
        couplingPresetBox.clear (juce::dontSendNotification);
        auto& cpm   = processor.getCouplingPresetManager();
        auto  names = cpm.getPresetNames();

        if (names.isEmpty())
        {
            couplingPresetBox.setTextWhenNothingSelected ("no presets saved");
            // Dim text color for the empty-state hint
            couplingPresetBox.setColour (juce::ComboBox::textColourId,
                                         GalleryColors::get (GalleryColors::t3()));
        }
        else
        {
            couplingPresetBox.setTextWhenNothingSelected ("Coupling Presets...");
            couplingPresetBox.setColour (juce::ComboBox::textColourId,
                                         GalleryColors::get (GalleryColors::t2()));
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
                            GalleryColors::get (GalleryColors::xoGold).withAlpha (0.18f));
                });
            return;
        }

        // Name input dialog.
        // takeOwnership=true in enterModalState transfers lifetime to JUCE's
        // modal component stack, so the AlertWindow is deleted when the modal
        // session ends — even if this component is destroyed while the dialog
        // is open. The raw ptr inside the lambda is therefore safe for the
        // lifetime of the modal session, and we must NOT call delete dlg.
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
                    // dlg is owned by JUCE (takeOwnership=true below) — do NOT delete here.
                }),
            true /*takeOwnership — JUCE deletes dlg when modal session ends*/);
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
                               GalleryColors::get (GalleryColors::xoGold).withAlpha (0.65f));
            juce::Timer::callAfterDelay (
                350,
                [safeThis = juce::Component::SafePointer<CouplingInspectorPanel> (this)] {
                    if (safeThis)
                        safeThis->bakeBtn.setColour (
                            juce::TextButton::buttonColourId,
                            GalleryColors::get (GalleryColors::xoGold).withAlpha (0.18f));
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

            // W14: Collapse all route cards after a preset load so stale
            // expanded states don't mislead the user about the new routing.
            for (int r = 0; r < kNumRoutes; ++r)
                routeCards[r].expanded = false;

            miniViz.refresh();
            resized();  // re-layout to collapsed card heights
            repaint();
        }
    }

    //==========================================================================
    // Members
    //==========================================================================
    XOceanusProcessor&                    processor;
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

} // namespace xoceanus
