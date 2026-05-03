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
// Session 1G additions (D1.c, D2.c, D4.b):
//   - Accordion expand/collapse per slot (only one slot expanded at a time)
//   - When expanded: per-FX primary parameter knobs appear below the row
//   - Bounded growth: max kExpandedRowH additional height per expanded slot
//   - ADV button per slot for overflow parameters (fires onAdvClicked)
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
#include "../../Core/FXParameterManifest.h"
#include <array>
#include <memory>
#include <vector>
#include <functional>

namespace xoceanus
{

//==============================================================================
/**
    EpicSlotsPanel

    Compact 3-row panel exposing the EpicChainSlotController's 3 slots.
    Mount in OceanView; use preferredHeight() or currentHeight() for layout.
    currentHeight() accounts for any accordion expansion (D4.b).
*/
class EpicSlotsPanel : public juce::Component,
                       private juce::ComboBox::Listener,
                       private juce::AudioProcessorValueTreeState::Listener
{
public:
    static constexpr int kRowHeight      = 40;
    static constexpr int kHeaderHeight   = 20;
    static constexpr int kExpandedRowH   = 120; // D4.b: max extra height per expanded slot

    static constexpr int preferredHeight()
    {
        return kHeaderHeight + EpicChainSlotController::kNumSlots * kRowHeight;
    }

    /// Returns the actual current height including any accordion expansion.
    int currentHeight() const noexcept
    {
        int extra = (expandedSlot_ >= 0) ? kExpandedRowH : 0;
        return preferredHeight() + extra;
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

        headerLabel_.setText("FX CHAIN", juce::dontSendNotification);
        headerLabel_.setFont(GalleryFonts::heading(10.0f));
        headerLabel_.setColour(juce::Label::textColourId,
                               GalleryColors::get(GalleryColors::textMid()));
        headerLabel_.setJustificationType(juce::Justification::centredLeft);
        addAndMakeVisible(headerLabel_);

        for (int i = 0; i < kNumSlots; ++i)
        {
            setupRow(i);
            setupAdvButton(i);
        }
    }

    ~EpicSlotsPanel() override
    {
        for (int i = 0; i < kNumSlots; ++i)
            apvts_.removeParameterListener(chainParamId(i), this);
    }

    //==========================================================================
    /// Fired when user clicks ADV button on a slot. slotIdx: 0..2.
    /// P3 follow-up: open full-parameter popup. For now wire externally.
    std::function<void(int slotIdx)> onAdvClicked;

    /// Fired when the panel's total height changes due to accordion expand/collapse.
    /// The parent should call resized() to reflow the layout.
    std::function<void()> onHeightChanged;

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
    // Accordion state (D2.c: only one slot expanded at a time)
    int expandedSlot_ = -1; // -1 = none expanded

    // Per-slot dynamic parameter knobs (rebuilt when slot expands or chain changes)
    std::array<std::vector<std::unique_ptr<juce::Slider>>, kNumSlots>   paramKnobs_;
    std::array<std::vector<std::unique_ptr<juce::Label>>, kNumSlots>    paramKnobLabels_;
    // Note: SliderAttachments are NOT used here — attachments aren't appropriate for
    // knobs that share param IDs across slots (Group A chains). Instead we use
    // direct APVTS parameter reads + begin/set/endChangeGesture pattern.
    // This is consistent with MasterFXStripCompact's approach.

    // Per-slot ADV button (D1.c)
    std::array<juce::TextButton, kNumSlots> advButtons_;

    //==========================================================================
    static juce::String slotPrefix(int idx) { return "slot" + juce::String(idx + 1) + "_"; }
    static juce::String chainParamId(int idx)  { return slotPrefix(idx) + "chain"; }
    static juce::String mixParamId(int idx)    { return slotPrefix(idx) + "mix"; }
    static juce::String bypassParamId(int idx) { return slotPrefix(idx) + "bypass"; }

    /// Returns the current chain ID for a slot (reads live APVTS value).
    int currentChainIdForSlot(int idx) const
    {
        auto* raw = apvts_.getRawParameterValue(chainParamId(idx));
        if (raw == nullptr) return 0;
        return juce::jlimit(0, static_cast<int>(kChainNames.size()) - 1,
                            static_cast<int>(raw->load() + 0.5f));
    }

    //==========================================================================
    void setupRow(int idx)
    {
        auto& row = rows_[idx];

        // Slot label — clicking it toggles the accordion expansion for this slot
        row.label.setText("SLOT " + juce::String(idx + 1), juce::dontSendNotification);
        row.label.setFont(GalleryFonts::heading(11.0f));
        row.label.setColour(juce::Label::textColourId,
                            GalleryColors::get(GalleryColors::textMid()));
        row.label.setJustificationType(juce::Justification::centredLeft);
        // Make label clickable for accordion toggle via addMouseListener
        row.label.setInterceptsMouseClicks(true, false);
        addAndMakeVisible(row.label);
        // Wire click → accordion expand using a MouseListener on the label.
        // We capture idx by value; 'this' lifetime is safe since the label is owned by rows_[].
        row.label.addMouseListener(this, false);

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
    void setupAdvButton(int idx)
    {
        advButtons_[idx].setButtonText("ADV");
        advButtons_[idx].setColour(juce::TextButton::textColourOffId,
                                   GalleryColors::get(GalleryColors::textMid()).withAlpha(0.4f));
        advButtons_[idx].setColour(juce::TextButton::buttonColourId,
                                   juce::Colours::transparentBlack);
        advButtons_[idx].onClick = [this, idx]
        {
            if (onAdvClicked) onAdvClicked(idx);
        };
        // Initially hidden — shown when slot is expanded and chain has advanced params
        advButtons_[idx].setVisible(false);
        addChildComponent(advButtons_[idx]);
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
    // Accordion expand/collapse (D2.c)
    //
    // Toggles which slot is expanded. Only one slot at a time.
    // Expanding a slot builds its parameter knobs; collapsing clears them (D5.a).
    void setSlotExpanded(int slotIdx)
    {
        if (expandedSlot_ == slotIdx)
        {
            // Collapse this slot
            clearParamKnobsForSlot(slotIdx);
            advButtons_[slotIdx].setVisible(false);
            expandedSlot_ = -1;
        }
        else
        {
            // Collapse previously expanded slot if any
            if (expandedSlot_ >= 0)
            {
                clearParamKnobsForSlot(expandedSlot_);
                advButtons_[expandedSlot_].setVisible(false);
            }
            expandedSlot_ = slotIdx;
            // Only build knobs if chain is not Off (D5.a)
            if (currentChainIdForSlot(slotIdx) > 0)
                rebuildParamKnobsForSlot(slotIdx);
        }
        resized();
        if (onHeightChanged) onHeightChanged();
    }

    //--------------------------------------------------------------------------
    // clearParamKnobsForSlot — removes dynamic knobs + labels for one slot
    void clearParamKnobsForSlot(int slotIdx)
    {
        paramKnobs_[slotIdx].clear();
        paramKnobLabels_[slotIdx].clear();
    }

    //--------------------------------------------------------------------------
    // rebuildParamKnobsForSlot — creates dynamic param knobs for the expanded slot.
    //
    // Uses MasterFXStripCompact's rotary pattern:
    //   - Knob is a juce::Slider in RotaryHorizontalVerticalDrag style
    //   - Value is read/written via APVTS parameter directly
    //   - WCAG: minimum knob diameter = 28px (D4.b row budget ÷ 2 rows of 3)
    void rebuildParamKnobsForSlot(int slotIdx)
    {
        clearParamKnobsForSlot(slotIdx);

        const int chainId = currentChainIdForSlot(slotIdx);
        const auto& schema = fx_manifest::schemaForChain(chainId);

        if (schema.primaryCount == 0)
        {
            advButtons_[slotIdx].setVisible(false);
            return;
        }

        for (int pi = 0; pi < schema.primaryCount; ++pi)
        {
            const auto& dp = schema.primary[pi];
            if (dp.fullParamId == nullptr || dp.fullParamId[0] == '\0') continue;

            // Null-safe param guard — skip if param not registered
            auto* juceParam = apvts_.getParameter(juce::String(dp.fullParamId));
            if (juceParam == nullptr)
            {
                DBG("EpicSlotsPanel: param not found: " << juce::String(dp.fullParamId));
                continue;
            }

            // Create knob (rotary style matching MasterFXStripCompact)
            auto knob = std::make_unique<juce::Slider>(juce::Slider::RotaryHorizontalVerticalDrag,
                                                        juce::Slider::NoTextBox);
            knob->setRange(0.0, 1.0);
            knob->setValue(static_cast<double>(juceParam->getValue()));
            knob->setColour(juce::Slider::rotarySliderFillColourId,
                            GalleryColors::get(GalleryColors::textMid()).withAlpha(0.6f));
            knob->setColour(juce::Slider::rotarySliderOutlineColourId,
                            GalleryColors::get(GalleryColors::textMid()).withAlpha(0.2f));

            // Wire mouse gestures to APVTS (no SliderAttachment to avoid
            // parameter ID collision when multiple slots share Group A params).
            // Use raw pointer capture — safe because the lambda is owned by the
            // same unique_ptr it captures, so it's destroyed together with the knob.
            const juce::String paramId(dp.fullParamId);
            juce::Slider* rawKnob = knob.get();
            knob->onValueChange = [this, paramId, rawKnob]
            {
                if (auto* p = apvts_.getParameter(paramId))
                    p->setValueNotifyingHost(static_cast<float>(rawKnob->getValue()));
            };
            knob->onDragStart = [this, paramId]
            {
                if (auto* p = apvts_.getParameter(paramId))
                    p->beginChangeGesture();
            };
            knob->onDragEnd = [this, paramId]
            {
                if (auto* p = apvts_.getParameter(paramId))
                    p->endChangeGesture();
            };

            addAndMakeVisible(*knob);
            paramKnobs_[slotIdx].push_back(std::move(knob));

            // Create label
            auto lbl = std::make_unique<juce::Label>();
            lbl->setText(juce::String(dp.labelShort), juce::dontSendNotification);
            lbl->setFont(GalleryFonts::heading(8.0f));
            lbl->setColour(juce::Label::textColourId,
                           GalleryColors::get(GalleryColors::textMid()).withAlpha(0.5f));
            lbl->setJustificationType(juce::Justification::centred);
            addAndMakeVisible(*lbl);
            paramKnobLabels_[slotIdx].push_back(std::move(lbl));
        }

        // Show ADV button if chain has more params (D1.c)
        advButtons_[slotIdx].setVisible(schema.hasAdvanced);
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

            // 1G: Rebuild param knobs if this slot is currently expanded (chain swapped)
            if (expandedSlot_ == i)
            {
                if (cid > 0)
                    rebuildParamKnobsForSlot(i);
                else
                    clearParamKnobsForSlot(i); // D5.a: Off → no param row
                resized();
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
            {
                if (paramID == chainParamId(i))
                {
                    panel->updateChainPickerFromApvts(i);
                    // Also rebuild param knobs if this slot is expanded
                    if (panel->expandedSlot_ == i)
                    {
                        const int cid = panel->currentChainIdForSlot(i);
                        if (cid > 0)
                            panel->rebuildParamKnobsForSlot(i);
                        else
                            panel->clearParamKnobsForSlot(i);
                        panel->resized();
                    }
                }
            }
        });
    }

    //==========================================================================
    // paint() — Track B owns visual polish here; Track A (1G) makes only the
    // minimal functional change: row-separator Y positions must shift down
    // when a slot before them is expanded (otherwise separators draw over knobs).
    // All color/alpha/chrome values are unchanged from the pre-1G baseline.
    void paint(juce::Graphics& g) override
    {
        // Subtle panel background + top/bottom hairlines.
        g.fillAll(GalleryColors::get(GalleryColors::slotBg()).withAlpha(0.35f));

        const auto b = getLocalBounds().toFloat();
        g.setColour(GalleryColors::border().withAlpha(0.5f));
        g.drawHorizontalLine(0, b.getX(), b.getRight());
        g.drawHorizontalLine(getHeight() - 1, b.getX(), b.getRight());

        // Row separators — Y must account for any expanded slot above (1G minimal change).
        g.setColour(GalleryColors::border().withAlpha(0.25f));
        for (int i = 1; i < kNumSlots; ++i)
        {
            int slotTop = kHeaderHeight + i * kRowHeight;
            // If a slot above this separator is expanded, push the separator down.
            if (expandedSlot_ >= 0 && expandedSlot_ < i)
                slotTop += kExpandedRowH;
            g.drawHorizontalLine(slotTop, b.getX() + 8.0f, b.getRight() - 8.0f);
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

        // Track vertical offset as we move through slots
        int currentY = kHeaderHeight;

        for (int i = 0; i < kNumSlots; ++i)
        {
            auto& row = rows_[i];
            const int y = currentY;
            int x = pad;

            row.label.setBounds(x, y, kSlotLabelW, kRowHeight);
            x += kSlotLabelW + 6;

            row.chainPicker.setBounds(x, y + rowInset, kChainBoxW, kRowHeight - 2 * rowInset);
            x += kChainBoxW + 10;

            const int mixW = juce::jmax(60, w - x - (kBypassW + pad + 10));
            row.mixSlider.setBounds(x, y + rowInset, mixW, kRowHeight - 2 * rowInset);
            x += mixW + 10;

            row.bypassToggle.setBounds(x, y + rowInset, kBypassW, kRowHeight - 2 * rowInset);

            currentY += kRowHeight;

            // If this slot is expanded, lay out the parameter row below it (D4.b)
            if (expandedSlot_ == i)
            {
                layoutParamRow(i, currentY, w, pad);
                currentY += kExpandedRowH;
            }
        }
    }

    //--------------------------------------------------------------------------
    // layoutParamRow — positions the dynamic knobs + labels + ADV button
    // within the kExpandedRowH budget for slot `slotIdx`.
    //
    // Layout: 2 rows of up to 3 knobs each.
    // Knob diameter: (kExpandedRowH - labelH - vGap) / 2, min 28px (WCAG).
    // ADV button: 28×16px, right-aligned at row bottom.
    void layoutParamRow(int slotIdx, int rowY, int totalW, int pad)
    {
        auto& knobs  = paramKnobs_[slotIdx];
        auto& labels = paramKnobLabels_[slotIdx];

        if (knobs.empty()) return;

        // Budget within the kExpandedRowH zone
        constexpr int kLabelH    = 12;
        constexpr int kVGap      = 4;
        constexpr int kAdvH      = 16;
        constexpr int kAdvW      = 28;
        constexpr int kTopPad    = 6;

        // Knob diameter — split available height into 2 rows
        // Available height per row = (kExpandedRowH - kTopPad - kLabelH - kVGap) / 2 - kLabelH
        const int availH      = kExpandedRowH - kTopPad - kAdvH - kVGap;
        const int rowsNeeded  = static_cast<int>(knobs.size()) <= 3 ? 1 : 2;
        const int knobDiam    = juce::jmax(28, (availH / rowsNeeded) - kLabelH - 2);

        const int numKnobs    = static_cast<int>(knobs.size());
        const int knobbedW    = totalW - 2 * pad - (advButtons_[slotIdx].isVisible() ? kAdvW + 4 : 0);
        const int perKnobW    = juce::jmax(knobDiam + 2, knobbedW / juce::jmax(1, juce::jmin(3, numKnobs)));

        int col = 0;
        int kRow = 0;

        for (int ki = 0; ki < numKnobs; ++ki)
        {
            col  = ki % 3;
            kRow = ki / 3;

            const int kx = pad + col * perKnobW + (perKnobW - knobDiam) / 2;
            const int ky = rowY + kTopPad + kRow * (knobDiam + kLabelH + kVGap);

            knobs[ki]->setBounds(kx, ky, knobDiam, knobDiam);

            if (ki < static_cast<int>(labels.size()))
                labels[ki]->setBounds(kx - 4, ky + knobDiam + 1, knobDiam + 8, kLabelH);
        }

        // ADV button — right-aligned at bottom of expanded row (D1.c)
        if (advButtons_[slotIdx].isVisible())
        {
            const int advX = totalW - pad - kAdvW;
            const int advY = rowY + kExpandedRowH - kAdvH - kVGap;
            advButtons_[slotIdx].setBounds(advX, advY, kAdvW, kAdvH);
        }
    }

    //==========================================================================
    // MouseListener bridge — slot label clicks expand the accordion.
    // This fires both for direct clicks on the panel AND for delegated
    // clicks from labels (which call addMouseListener(this, false) in setupRow).
    void mouseUp(const juce::MouseEvent& e) override
    {
        for (int i = 0; i < kNumSlots; ++i)
        {
            if (e.eventComponent == &rows_[i].label)
            {
                setSlotExpanded(i);
                return;
            }
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
