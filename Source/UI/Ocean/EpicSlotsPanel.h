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

        headerLabel_.setText("FX CHAIN", juce::dontSendNotification);
        headerLabel_.setFont(GalleryFonts::heading(10.0f));
        headerLabel_.setColour(juce::Label::textColourId,
                               GalleryColors::get(GalleryColors::textMid()));
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

        // Slot label
        row.label.setText("SLOT " + juce::String(idx + 1), juce::dontSendNotification);
        row.label.setFont(GalleryFonts::heading(11.0f));
        row.label.setColour(juce::Label::textColourId,
                            GalleryColors::get(GalleryColors::textMid()));
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
        row.mixSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 44, 16);
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
    void paint(juce::Graphics& g) override
    {
        // Subtle panel background + top/bottom hairlines.
        g.fillAll(GalleryColors::get(GalleryColors::slotBg()).withAlpha(0.35f));

        const auto b = getLocalBounds().toFloat();
        g.setColour(GalleryColors::border().withAlpha(0.5f));
        g.drawHorizontalLine(0, b.getX(), b.getRight());
        g.drawHorizontalLine(getHeight() - 1, b.getX(), b.getRight());

        // Row separators
        g.setColour(GalleryColors::border().withAlpha(0.25f));
        for (int i = 1; i < kNumSlots; ++i)
        {
            const int y = kHeaderHeight + i * kRowHeight;
            g.drawHorizontalLine(y, b.getX() + 8.0f, b.getRight() - 8.0f);
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
        const int rowInset = 8;

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
