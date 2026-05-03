// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
// EpicSlotsPanel.h — 3-slot FX picker for the XOceanus Ocean View.
//
// Exposes the EpicChainSlotController's 9 parameters (3 slots × {chain, mix,
// bypass}) as a compact submarine-style panel. Each row corresponds to one
// slot in the signal chain (slots run in series: slot 1 → slot 2 → slot 3).
//
// Per-row controls when chain ≠ Off:
//   [S01 label] [chain dropdown] [knob 1] … [knob N] [MIX knob] [ADV] [BYPASS]
//
// Per-row controls when chain = Off:
//   [S01 label] [chain dropdown ▼ "Off"]  [empty space]  [BYPASS]
//
// Session 1G additions (D1.c, D2.c, D4.b):
//   Original design shipped accordion expand/collapse.
//
// fix/1g-always-visible-knobs (this revision):
//   Replaces accordion model with always-visible per-FX knobs per slot row,
//   matching the MasterFXStripCompact vocabulary:
//     - 1-6 rotary knobs always rendered when chain ≠ Off
//     - MIX rotary knob replaces the horizontal stretch slider
//     - ADV button per slot for overflow parameters (fires onAdvClicked)
//     - kRowHeight grown from 40→56 to accommodate 32px knob + 12px label
//     - No accordion state, no dynamic height changes
//
// The chain parameter is registered as AudioParameterFloat with integer steps
// (0 – kMaxChainID) rather than AudioParameterChoice, so this panel manages
// the combo<->float mapping manually. Mix and bypass parameters use
// standard APVTS attachments.
//
// File is header-only (XOceanus UI convention).

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "../GalleryColors.h"
#include "../Tokens.h"
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
    Always-visible per-FX knobs — no accordion expand/collapse.
    Mount in OceanView; use preferredHeight() for layout.
*/
class EpicSlotsPanel : public juce::Component,
                       private juce::ComboBox::Listener,
                       private juce::AudioProcessorValueTreeState::Listener
{
public:
    // kRowHeight grown from 40→56 to fit 32px knob + 12px label + 12px padding.
    static constexpr int kRowHeight      = 56;
    static constexpr int kHeaderHeight   = 20;

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
        {
            setupRow(i);
            setupAdvButton(i);
        }

        // Initial knob build for any slot already set to a non-Off chain.
        // (Handles preset restores before the first comboBoxChanged fires.)
        for (int i = 0; i < kNumSlots; ++i)
        {
            if (currentChainIdForSlot(i) > 0)
                rebuildParamKnobsForSlot(i);
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

private:
    static constexpr int kNumSlots = EpicChainSlotController::kNumSlots;

    // Knob geometry constants — match MasterFXStripCompact knob diameter.
    static constexpr int kKnobDiam  = 32; // matches MasterFXStripCompact's knobSz
    static constexpr int kKnobLabelH = 12;
    static constexpr int kKnobTopPad = 6;  // top padding within the row

    //==========================================================================
    struct SlotRow
    {
        juce::Label     label;
        juce::ComboBox  chainPicker;
        juce::ToggleButton bypassToggle;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttach;
        // MIX rotary knob (replaces the horizontal stretch slider from the accordion model)
        std::unique_ptr<juce::Slider> mixKnob;
        std::unique_ptr<juce::Label>  mixLabel;
        // Note: no SliderAttachment for mixKnob — direct APVTS gesture pattern
        // consistent with MasterFXStripCompact and paramKnobs_ below.
    };

    //==========================================================================
    // Per-slot dynamic parameter knobs (rebuilt when chain changes)
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

        // Slot label — D0.b (Track B 2C): dot-matrix Doto font for submarine console identity.
        // "S01", "S02", "S03" format matches DotMatrixDisplay vocab.
        row.label.setText("S0" + juce::String(idx + 1), juce::dontSendNotification);
        row.label.setFont(GalleryFonts::dotMatrix(11.0f)); // Doto dot-matrix font
        row.label.setColour(juce::Label::textColourId,
                            XO::Tokens::Color::accent().withAlpha(0.85f)); // teal slot label
        row.label.setJustificationType(juce::Justification::centredLeft);
        row.label.setInterceptsMouseClicks(false, false); // labels are display-only now
        addAndMakeVisible(row.label);

        // Chain picker — grouped menu. ComboBox item IDs must be > 0, so we
        // use (chainId + 1) as the JUCE item ID everywhere.
        populateChainPicker(row.chainPicker);
        row.chainPicker.setJustificationType(juce::Justification::centredLeft);
        row.chainPicker.addListener(this);
        // #21: tooltip for slot chain picker
        row.chainPicker.setTooltip("S0" + juce::String(idx + 1) + " FX chain \xe2\x80\x94 choose the signal processing chain for this slot");
        addAndMakeVisible(row.chainPicker);

        // MIX rotary knob — replaces the horizontal stretch slider.
        // Same interaction pattern as MasterFXStripCompact: direct APVTS gestures.
        row.mixKnob = std::make_unique<juce::Slider>(juce::Slider::RotaryHorizontalVerticalDrag,
                                                      juce::Slider::NoTextBox);
        row.mixKnob->setRange(0.0, 1.0);
        // Read initial value from APVTS
        if (auto* p = apvts_.getParameter(mixParamId(idx)))
            row.mixKnob->setValue(static_cast<double>(p->getValue()), juce::dontSendNotification);
        row.mixKnob->setColour(juce::Slider::rotarySliderFillColourId,
                               XO::Tokens::Color::accent().withAlpha(0.60f));
        row.mixKnob->setColour(juce::Slider::rotarySliderOutlineColourId,
                               GalleryColors::get(GalleryColors::textMid()).withAlpha(0.20f));
        row.mixKnob->setTooltip("S0" + juce::String(idx + 1) + " mix \xe2\x80\x94 wet/dry blend (0 = dry, 1 = fully wet). Drag up/down, double-click to reset.");

        // Wire gestures to APVTS (no SliderAttachment — consistent with paramKnobs_ pattern).
        const juce::String mixId = mixParamId(idx);
        juce::Slider* rawMix = row.mixKnob.get();
        row.mixKnob->onValueChange = [this, mixId, rawMix]
        {
            if (auto* p = apvts_.getParameter(mixId))
                p->setValueNotifyingHost(static_cast<float>(rawMix->getValue()));
        };
        row.mixKnob->onDragStart = [this, mixId]
        {
            if (auto* p = apvts_.getParameter(mixId))
                p->beginChangeGesture();
        };
        row.mixKnob->onDragEnd = [this, mixId]
        {
            if (auto* p = apvts_.getParameter(mixId))
                p->endChangeGesture();
        };
        addAndMakeVisible(*row.mixKnob);

        // MIX knob label
        row.mixLabel = std::make_unique<juce::Label>();
        row.mixLabel->setText("MIX", juce::dontSendNotification);
        row.mixLabel->setFont(GalleryFonts::heading(8.0f));
        row.mixLabel->setColour(juce::Label::textColourId,
                                GalleryColors::get(GalleryColors::textMid()).withAlpha(0.5f));
        row.mixLabel->setJustificationType(juce::Justification::centred);
        addAndMakeVisible(*row.mixLabel);

        // Bypass toggle
        row.bypassToggle.setButtonText("BYPASS");
        row.bypassToggle.setColour(juce::ToggleButton::textColourId,
                                   GalleryColors::get(GalleryColors::textMid()));
        // #21: tooltip for bypass toggle
        row.bypassToggle.setTooltip("S0" + juce::String(idx + 1) + " bypass \xe2\x80\x94 disable this FX slot without losing the chain selection");
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
        // ADV button is visible when chain has advanced params; initially hidden
        // until chain is loaded (updated by rebuildParamKnobsForSlot).
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
    // clearParamKnobsForSlot — removes dynamic knobs + labels for one slot
    void clearParamKnobsForSlot(int slotIdx)
    {
        paramKnobs_[slotIdx].clear();
        paramKnobLabels_[slotIdx].clear();
    }

    //--------------------------------------------------------------------------
    // rebuildParamKnobsForSlot — creates dynamic param knobs for the given slot.
    //
    // Uses MasterFXStripCompact's rotary pattern:
    //   - Knob is a juce::Slider in RotaryHorizontalVerticalDrag style
    //   - Value is read/written via APVTS parameter directly
    //   - WCAG: minimum knob diameter = kKnobDiam (32px) ≥ 24px threshold
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

            // Always rebuild param knobs when chain changes.
            // If chain is Off, clear knobs; otherwise rebuild.
            if (cid > 0)
                rebuildParamKnobsForSlot(i);
            else
            {
                clearParamKnobsForSlot(i); // D5.a: Off → no param knobs
                advButtons_[i].setVisible(false);
            }
            resized();
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
                    // Always rebuild param knobs on chain change (always-visible model).
                    const int cid = panel->currentChainIdForSlot(i);
                    if (cid > 0)
                        panel->rebuildParamKnobsForSlot(i);
                    else
                    {
                        panel->clearParamKnobsForSlot(i);
                        panel->advButtons_[i].setVisible(false);
                    }
                    panel->resized();
                }
            }
        });
    }

    //==========================================================================
    /**
        Submarine-console pedal-board paint (D0.b — Session 2C).
        Track B owns visual chrome; Track A (1G) functional corrections preserved.

        Visual grammar:
          - Panel base: XO::Tokens::Color::Surface with subtle dark vignette
          - Riveted depth-bar across the top (kHeaderHeight region)
          - Top/bottom 1px teal border hairlines (Tokens::accent 50% alpha)
          - Rivet ornaments at header corners + row endpoints (small brass circles)
          - Row separators: fixed Y positions (always-visible model — no accordion offset)
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

        // ── 3. Row separators — fixed Y positions (no accordion offset) ─────
        g.setColour(XO::Tokens::Color::accent().withAlpha(0.18f));
        for (int i = 1; i < kNumSlots; ++i)
        {
            const int slotTop = kHeaderHeight + i * kRowHeight;
            g.drawHorizontalLine(slotTop, b.getX() + 12.0f, b.getRight() - 12.0f);
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
        const int w   = getWidth();
        const int pad = 12;

        headerLabel_.setBounds(pad, 0, w - 2 * pad, kHeaderHeight);

        // Row layout when chain ≠ Off (L→R):
        //   [SLOT N] [CHAIN PICKER] [knob1…N] [MIX knob] [ADV] [BYPASS]
        // Row layout when chain = Off:
        //   [SLOT N] [CHAIN PICKER "Off"]  [empty]  [BYPASS]
        //
        // Constants:
        constexpr int kSlotLabelW  = 38;
        constexpr int kChainBoxW   = 130;
        constexpr int kAdvW        = 32;
        constexpr int kAdvH        = 18;
        constexpr int kBypassW     = 68;
        constexpr int kMixKnobW    = kKnobDiam + 8; // knob + small horizontal margin
        const int rowInset = (kRowHeight - kKnobDiam - kKnobLabelH) / 2; // center vertically

        for (int i = 0; i < kNumSlots; ++i)
        {
            auto& row = rows_[i];
            const int y = kHeaderHeight + i * kRowHeight;
            int x = pad;

            row.label.setBounds(x, y, kSlotLabelW, kRowHeight);
            x += kSlotLabelW + 4;

            const int chainPickerH = kRowHeight - 2 * rowInset;
            row.chainPicker.setBounds(x, y + rowInset, kChainBoxW, chainPickerH);
            x += kChainBoxW + 6;

            const int chainId = currentChainIdForSlot(i);
            auto& knobs  = paramKnobs_[i];
            auto& labels = paramKnobLabels_[i];

            if (chainId > 0 && !knobs.empty())
            {
                // Layout: param knobs, then MIX knob, then ADV (if visible), then BYPASS
                // Compute available width for param knobs
                const bool hasAdv = advButtons_[i].isVisible();
                const int rightReserved = kMixKnobW + (hasAdv ? kAdvW + 4 : 0) + kBypassW + pad;
                const int availForParamKnobs = w - x - rightReserved;
                const int numKnobs = static_cast<int>(knobs.size());
                const int perKnobW = juce::jmax(kKnobDiam + 4,
                                                availForParamKnobs / juce::jmax(1, numKnobs));

                for (int ki = 0; ki < numKnobs; ++ki)
                {
                    // Center each knob vertically in the row, label below
                    const int kx = x + ki * perKnobW + (perKnobW - kKnobDiam) / 2;
                    const int ky = y + rowInset;
                    knobs[ki]->setBounds(kx, ky, kKnobDiam, kKnobDiam);
                    if (ki < static_cast<int>(labels.size()))
                        labels[ki]->setBounds(kx - 4, ky + kKnobDiam + 1, kKnobDiam + 8, kKnobLabelH);
                }
                x += numKnobs * perKnobW;

                // MIX rotary knob
                const int mx = x + (kMixKnobW - kKnobDiam) / 2;
                row.mixKnob->setBounds(mx, y + rowInset, kKnobDiam, kKnobDiam);
                row.mixLabel->setBounds(mx - 4, y + rowInset + kKnobDiam + 1,
                                        kKnobDiam + 8, kKnobLabelH);
                row.mixKnob->setVisible(true);
                row.mixLabel->setVisible(true);
                x += kMixKnobW;

                // ADV button — vertically centred in row
                if (hasAdv)
                {
                    const int advY = y + (kRowHeight - kAdvH) / 2;
                    advButtons_[i].setBounds(x, advY, kAdvW, kAdvH);
                    x += kAdvW + 4;
                }
            }
            else
            {
                // Off slot: hide param knobs, MIX knob, and ADV button.
                clearParamKnobsForSlot(i); // ensure visual cleanup
                row.mixKnob->setVisible(false);
                row.mixLabel->setVisible(false);
                advButtons_[i].setVisible(false);
                // Leave x at current position — BYPASS fills right side.
                x = w - pad - kBypassW;
            }

            // BYPASS toggle — right-aligned in row
            const int bypassY = y + (kRowHeight - chainPickerH) / 2;
            const int bypassX = w - pad - kBypassW;
            row.bypassToggle.setBounds(bypassX, bypassY, kBypassW, chainPickerH);
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
