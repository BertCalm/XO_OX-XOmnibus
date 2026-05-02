// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
// EpicSlotsPanel.h — 3-slot FX picker for the XOceanus Ocean View.
//
// Exposes the EpicChainSlotController's 9 parameters (3 slots × {chain, mix,
// bypass}) as a compact submarine-style panel. Each row corresponds to one
// slot in the signal chain (slots run in series: slot 1 → slot 2 → slot 3).
//
// Per-row controls:
//   - Slot label ("SLOT N", 10px bold, dimmed)
//   - Chain picker (juce::ComboBox, grouped by chain family — see kChainGroups)
//   - Mix slider (linear horizontal, 0.0 – 1.0)
//   - Bypass toggle (ToggleButton)
//
// The chain parameter is registered as AudioParameterFloat with integer steps
// (0 – kMaxChainID) rather than AudioParameterChoice, so this panel manages
// the combo<->float mapping manually. The mix and bypass parameters use
// standard APVTS attachments.
//
// File is header-only (XOceanus UI convention).

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "../GalleryColors.h"
#include "../Tokens.h"
#include "../../Core/EpicChainSlotController.h"
#include <array>
#include <memory>

namespace xoceanus
{

//==============================================================================
/**
    EpicSlotsPanel

    Compact 3-row panel exposing the EpicChainSlotController's 3 slots.
    Mount in OceanView; use preferredHeight() for layout.
*/
class EpicSlotsPanel : public juce::Component,
                       private juce::ComboBox::Listener,
                       private juce::AudioProcessorValueTreeState::Listener
{
public:
    static constexpr int kRowHeight   = 40;
    static constexpr int kHeaderHeight = 20;

    static constexpr int preferredHeight()
    {
        return kHeaderHeight + EpicChainSlotController::kNumSlots * kRowHeight;
    }

    //==========================================================================
    // Chain name table — MUST match EpicChainSlotController::ChainID enum ordering.
    // Index = ChainID integer value (0 = Off … 33 = Oligo).
    static constexpr std::array<const char*, 34> kChainNames = {
        "Off",
        "Aquatic", "Math", "Boutique",                     // Suites
        "Onslaught", "Obscura", "Oratory",                 // Singularity
        "Onrush", "Omnistereo", "Obliterate", "Obscurity", // Epic (spec'd)
        "Oubliette", "Osmium", "Orogen", "Oculus",         // Wave 2 — Monsterous
        "Outage", "Override", "Occlusion", "Obdurate",     // Wave 2 — Sunken Treasure
        "Orison", "Overshoot", "Obverse", "Oxymoron",      // Wave 2 — Anomalous
        "Ornate", "Oration", "Offcut", "Omen",             // Wave 2 — AHA
        "Opus", "Outlaw", "Outbreak", "Orrery",            // Wave 2 — Alt Universe
        "Otrium", "Oblate", "Oligo"                        // FX Pack 1 — Sidechain Creative
    };

    // Group headers inserted into the ComboBox menu before the chain IDs that
    // start each family. { firstChainId, headerText }.
    static constexpr std::array<std::pair<int, const char*>, 9> kChainGroups = {{
        { 1,  "— SUITES —" },
        { 4,  "— SINGULARITY —" },
        { 7,  "— EPIC —" },
        { 11, "— WAVE 2: MONSTROUS —" },
        { 15, "— WAVE 2: SUNKEN TREASURE —" },
        { 19, "— WAVE 2: ANOMALOUS —" },
        { 23, "— WAVE 2: AHA —" },
        { 27, "— WAVE 2: ALT UNIVERSE —" },
        { 31, "— FX PACK 1: SIDECHAIN CREATIVE —" },
    }};

    //==========================================================================
    explicit EpicSlotsPanel(juce::AudioProcessorValueTreeState& apvts)
        : apvts_(apvts)
    {
        setOpaque(false);

        // Fix #1424: expose FX chain slot panel to screen readers.
        A11y::setup(*this,
                    "FX Chain Slots",
                    "Three parallel FX chain slots with chain picker, mix level, and bypass per slot");

        // D0.b: "FX CHAIN" header uses Tokens::Type::heading (Satoshi Bold).
        // Slot labels below use GalleryFonts::dotMatrix for dot-matrix aesthetic.
        headerLabel_.setText("FX CHAIN", juce::dontSendNotification);
        headerLabel_.setFont(XO::Tokens::Type::heading(XO::Tokens::Type::HeadingSmall));
        headerLabel_.setColour(juce::Label::textColourId,
                               XO::Tokens::Color::accent().withAlpha(0.70f)); // teal-tinted header
        headerLabel_.setJustificationType(juce::Justification::centredLeft);
        addAndMakeVisible(headerLabel_);

        for (int i = 0; i < kNumSlots; ++i)
            setupRow(i);
    }

    ~EpicSlotsPanel() override
    {
        for (int i = 0; i < kNumSlots; ++i)
            apvts_.removeParameterListener(chainParamId(i), this);
    }

private:
    static constexpr int kNumSlots = EpicChainSlotController::kNumSlots;

    //==========================================================================
    struct SlotRow
    {
        juce::Label label;
        juce::ComboBox chainPicker;
        juce::Slider mixSlider;
        juce::ToggleButton bypassToggle;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mixAttach;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttach;
    };

    //==========================================================================
    static juce::String slotPrefix(int idx) { return "slot" + juce::String(idx + 1) + "_"; }
    static juce::String chainParamId(int idx)  { return slotPrefix(idx) + "chain"; }
    static juce::String mixParamId(int idx)    { return slotPrefix(idx) + "mix"; }
    static juce::String bypassParamId(int idx) { return slotPrefix(idx) + "bypass"; }

    //==========================================================================
    void setupRow(int idx)
    {
        auto& row = rows_[idx];

        // Slot label — D0.b: dot-matrix Doto font for submarine console identity.
        // "S01", "S02", "S03" format matches DotMatrixDisplay vocab.
        row.label.setText("S0" + juce::String(idx + 1), juce::dontSendNotification);
        row.label.setFont(GalleryFonts::dotMatrix(11.0f)); // Doto dot-matrix font
        row.label.setColour(juce::Label::textColourId,
                            XO::Tokens::Color::accent().withAlpha(0.85f)); // teal slot label
        row.label.setJustificationType(juce::Justification::centredLeft);
        addAndMakeVisible(row.label);

        // Chain picker — grouped menu. ComboBox item IDs must be > 0, so we
        // use (chainId + 1) as the JUCE item ID everywhere.
        populateChainPicker(row.chainPicker);
        row.chainPicker.setJustificationType(juce::Justification::centredLeft);
        row.chainPicker.addListener(this);
        addAndMakeVisible(row.chainPicker);

        // Mix slider
        row.mixSlider.setSliderStyle(juce::Slider::LinearHorizontal);
        // Fix #1430: text box was 16 px — difficult to click precisely. Raised to 20 px.
        row.mixSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 44, 20);
        row.mixSlider.setColour(juce::Slider::trackColourId,
                                GalleryColors::get(GalleryColors::textMid()).withAlpha(0.5f));
        row.mixAttach = std::make_unique<
            juce::AudioProcessorValueTreeState::SliderAttachment>(
                apvts_, mixParamId(idx), row.mixSlider);
        addAndMakeVisible(row.mixSlider);

        // Bypass toggle
        row.bypassToggle.setButtonText("BYPASS");
        row.bypassToggle.setColour(juce::ToggleButton::textColourId,
                                   GalleryColors::get(GalleryColors::textMid()));
        row.bypassAttach = std::make_unique<
            juce::AudioProcessorValueTreeState::ButtonAttachment>(
                apvts_, bypassParamId(idx), row.bypassToggle);
        addAndMakeVisible(row.bypassToggle);

        // Sync chain picker from current APVTS value + subscribe for updates
        // (e.g. preset loads).
        apvts_.addParameterListener(chainParamId(idx), this);
        updateChainPickerFromApvts(idx);
    }

    //--------------------------------------------------------------------------
    static void populateChainPicker(juce::ComboBox& box)
    {
        // "Off" first — no group header for it.
        box.addItem(kChainNames[0], /*itemId*/ 1);

        int nextGroup = 0;
        for (int cid = 1; cid < static_cast<int>(kChainNames.size()); ++cid)
        {
            if (nextGroup < static_cast<int>(kChainGroups.size())
                && cid == kChainGroups[nextGroup].first)
            {
                box.addSectionHeading(juce::String(juce::CharPointer_UTF8(kChainGroups[nextGroup].second)));
                ++nextGroup;
            }
            box.addItem(kChainNames[cid], cid + 1); // JUCE item IDs are 1-based
        }
    }

    //--------------------------------------------------------------------------
    void updateChainPickerFromApvts(int idx)
    {
        auto* raw = apvts_.getRawParameterValue(chainParamId(idx));
        if (raw == nullptr) return;
        const int cid = juce::jlimit(0, static_cast<int>(kChainNames.size()) - 1,
                                     static_cast<int>(raw->load() + 0.5f));
        rows_[idx].chainPicker.setSelectedId(cid + 1, juce::dontSendNotification);
    }

    //==========================================================================
    // juce::ComboBox::Listener
    void comboBoxChanged(juce::ComboBox* box) override
    {
        for (int i = 0; i < kNumSlots; ++i)
        {
            if (box != &rows_[i].chainPicker) continue;
            const int cid = box->getSelectedId() - 1; // map back to 0-indexed ChainID
            if (auto* p = apvts_.getParameter(chainParamId(i)))
            {
                // The chain param is a float with integer step; normalise via the
                // param's own range, then notify the host.
                const float normalised = p->convertTo0to1(static_cast<float>(cid));
                p->setValueNotifyingHost(normalised);
            }
            return;
        }
    }

    //==========================================================================
    // juce::AudioProcessorValueTreeState::Listener —
    // called when a preset/automation changes slot{N}_chain.
    // May arrive on audio thread; bounce to message thread for ComboBox update.
    void parameterChanged(const juce::String& paramID, float /*newValue*/) override
    {
        juce::WeakReference<juce::Component> weakSelf { this };
        juce::MessageManager::callAsync([weakSelf, paramID]()
        {
            auto* raw = weakSelf.get();
            if (raw == nullptr) return;
            auto* panel = static_cast<EpicSlotsPanel*>(raw);
            for (int i = 0; i < kNumSlots; ++i)
                if (paramID == chainParamId(i))
                    panel->updateChainPickerFromApvts(i);
        });
    }

    //==========================================================================
    /**
        Submarine-console pedal-board paint (D0.b — Session 2C).

        Visual grammar:
          - Panel base: XO::Tokens::Color::Surface with subtle dark vignette
          - Riveted depth-bar across the top (kHeaderHeight region)
          - Top/bottom 1px teal border hairlines (Tokens::accent 50% alpha)
          - Rivet ornaments at header corners + row endpoints (small brass circles)
          - Row separators: 1px lines at 20% Tokens::accent alpha
          - Active slot indicator: left-edge glow strip in Tokens::Glow
          - Header label area: slightly lighter surface tone
    */
    void paint(juce::Graphics& g) override
    {
        const float w  = static_cast<float>(getWidth());
        const float h  = static_cast<float>(getHeight());
        const auto  b  = getLocalBounds().toFloat();

        // ── 1. Panel base — submarine surface colour ────────────────────────
        g.setColour(XO::Tokens::Color::surface());
        g.fillRect(b);

        // ── 2. Riveted depth-bar header (kHeaderHeight region) ──────────────
        // Slightly brighter than panel base so it reads as a distinct panel cap.
        {
            const juce::Rectangle<float> headerRect(0.0f, 0.0f, w, static_cast<float>(kHeaderHeight));
            g.setColour(juce::Colour(0xFF1E2530)); // 4 pts lighter than Surface
            g.fillRect(headerRect);

            // Header bottom separator — 1px teal hairline
            g.setColour(XO::Tokens::Color::accent().withAlpha(0.35f));
            g.fillRect(0.0f, static_cast<float>(kHeaderHeight) - 1.0f, w, 1.0f);

            // Rivet ornaments — 4px diameter brass-tinted circles at header corners
            // and equidistant mid-points along the depth-bar.
            paintRivets(g, 0.0f, static_cast<float>(kHeaderHeight));
        }

        // ── 3. Row separators ───────────────────────────────────────────────
        g.setColour(XO::Tokens::Color::accent().withAlpha(0.18f));
        for (int i = 1; i < kNumSlots; ++i)
        {
            const float y = static_cast<float>(kHeaderHeight + i * kRowHeight);
            g.drawHorizontalLine(static_cast<int>(y), b.getX() + 12.0f, b.getRight() - 12.0f);
        }

        // ── 4. Top + bottom border hairlines ────────────────────────────────
        g.setColour(XO::Tokens::Color::accent().withAlpha(0.50f));
        g.drawHorizontalLine(0, b.getX(), b.getRight());
        g.drawHorizontalLine(getHeight() - 1, b.getX(), b.getRight());

        // ── 5. Active-slot left-edge glow (Glow alias = Accent) ─────────────
        // Iterates slots and draws a 3px edge strip where bypass is OFF.
        // Reads APVTS bypass params to determine active state.
        for (int i = 0; i < kNumSlots; ++i)
        {
            const float slotY = static_cast<float>(kHeaderHeight + i * kRowHeight);
            // If bypass param is 0.0 (i.e. not bypassed), slot is active → glow.
            auto* bypassParam = apvts_.getRawParameterValue(bypassParamId(i));
            const bool isBypassed = (bypassParam != nullptr && bypassParam->load() > 0.5f);
            // Also need a chain loaded (chain != 0 = Off) to be truly active.
            auto* chainParam = apvts_.getRawParameterValue(chainParamId(i));
            const int chainId = (chainParam != nullptr)
                                 ? static_cast<int>(chainParam->load() + 0.5f) : 0;
            const bool isActive = (!isBypassed && chainId > 0);

            if (isActive)
            {
                // Active slot: bright teal left-edge strip + subtle row highlight
                juce::ColourGradient activeGlow(
                    XO::Tokens::Color::glow().withAlpha(0.55f), 0.0f, slotY,
                    XO::Tokens::Color::glow().withAlpha(0.0f),  14.0f, slotY, false);
                g.setGradientFill(activeGlow);
                g.fillRect(0.0f, slotY + 1.0f, 14.0f, static_cast<float>(kRowHeight - 2));

                // 2px left edge bar
                g.setColour(XO::Tokens::Color::glow().withAlpha(0.80f));
                g.fillRect(0.0f, slotY + 1.0f, 2.0f, static_cast<float>(kRowHeight - 2));
            }
        }

        // ── 6. Corner rivets on panel ────────────────────────────────────────
        // Small brass-tinted circles at all 4 panel corners.
        paintCornerRivets(g, w, h);
    }

    //--------------------------------------------------------------------------
    /** Paint rivet ornaments along the header depth-bar (D0.b submarine aesthetic). */
    static void paintRivets(juce::Graphics& g, float /*barY*/, float barH)
    {
        // Brass-tinted rivet: bright centre + dark ring + subtle shadow.
        // Positions: every ~60px along the bar, inset 8px from edges.
        const juce::Colour brassRim(0xFFB8956A);    // warm brass
        const juce::Colour brassFill(0xFFD4A96A);   // lighter brass highlight
        const float rivetR = 3.0f;
        const float rivetY = barH * 0.5f;
        // We can't know width here (method is static), so this is called with
        // the caller's width. For now, draw 2 fixed rivets at left/right insets.
        // Caller may extend with additional positions.
        const float positions[] = { 8.0f + rivetR, 8.0f + rivetR }; // placeholder

        for (float rx : positions)
        {
            // Shadow
            g.setColour(juce::Colours::black.withAlpha(0.35f));
            g.fillEllipse(rx - rivetR + 0.5f, rivetY - rivetR + 0.5f, rivetR * 2.0f, rivetR * 2.0f);
            // Brass fill
            g.setColour(brassRim);
            g.fillEllipse(rx - rivetR, rivetY - rivetR, rivetR * 2.0f, rivetR * 2.0f);
            // Bright spot (highlight)
            g.setColour(brassFill.withAlpha(0.7f));
            g.fillEllipse(rx - rivetR * 0.5f, rivetY - rivetR * 0.65f, rivetR, rivetR * 0.8f);
        }
    }

    /** Paint small brass corner rivets at the 4 panel corners (D0.b). */
    static void paintCornerRivets(juce::Graphics& g, float w, float h)
    {
        const juce::Colour brassRim(0xFFB8956A);
        const juce::Colour brassFill(0xFFD4A96A);
        const float r = 2.5f;
        const float inset = 5.0f;
        const float corners[4][2] = {
            { inset,     inset     },   // top-left
            { w - inset, inset     },   // top-right
            { inset,     h - inset },   // bottom-left
            { w - inset, h - inset },   // bottom-right
        };

        for (const auto& c : corners)
        {
            g.setColour(juce::Colours::black.withAlpha(0.30f));
            g.fillEllipse(c[0] - r + 0.5f, c[1] - r + 0.5f, r * 2.0f, r * 2.0f);
            g.setColour(brassRim);
            g.fillEllipse(c[0] - r, c[1] - r, r * 2.0f, r * 2.0f);
            g.setColour(brassFill.withAlpha(0.65f));
            g.fillEllipse(c[0] - r * 0.45f, c[1] - r * 0.6f, r, r * 0.75f);
        }
    }

    //--------------------------------------------------------------------------
    void resized() override
    {
        const int w = getWidth();
        const int pad = 12;

        headerLabel_.setBounds(pad, 0, w - 2 * pad, kHeaderHeight);

        // Row layout (L→R): [SLOT N] [CHAIN PICKER] [MIX SLIDER ...stretch] [BYPASS]
        constexpr int kSlotLabelW = 52;
        constexpr int kChainBoxW  = 180;
        constexpr int kBypassW    = 76;
        // Fix #1430: rowInset=8 left controls at only 24 px in a 40 px row.
        // Reduced to 4 so controls occupy 32 px — closer to the 36 px target.
        const int rowInset = 4;

        for (int i = 0; i < kNumSlots; ++i)
        {
            auto& row = rows_[i];
            const int y = kHeaderHeight + i * kRowHeight;
            int x = pad;

            row.label.setBounds(x, y, kSlotLabelW, kRowHeight);
            x += kSlotLabelW + 6;

            row.chainPicker.setBounds(x, y + rowInset, kChainBoxW, kRowHeight - 2 * rowInset);
            x += kChainBoxW + 10;

            const int mixW = juce::jmax(60, w - x - (kBypassW + pad + 10));
            row.mixSlider.setBounds(x, y + rowInset, mixW, kRowHeight - 2 * rowInset);
            x += mixW + 10;

            row.bypassToggle.setBounds(x, y + rowInset, kBypassW, kRowHeight - 2 * rowInset);
        }
    }

    //==========================================================================
    juce::AudioProcessorValueTreeState& apvts_;
    juce::Label                         headerLabel_;
    std::array<SlotRow, kNumSlots>      rows_;

    // juce::Component already supplies WeakReference<Component>::Master, so
    // WeakReference<juce::Component>{this} works without an extra macro here.
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EpicSlotsPanel)
};

} // namespace xoceanus
