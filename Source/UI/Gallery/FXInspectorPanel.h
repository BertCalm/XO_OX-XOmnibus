// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "../GalleryColors.h"
#include "GalleryKnob.h"

// Forward-declare to avoid including the full processor header here.
namespace xoceanus { class XOceanusProcessor; }

namespace xoceanus {

//==============================================================================
// FXInspectorPanel — Column C / C3 (FX) tab content.
//
// Displays the master FX chain as a scrollable accordion of collapsible cards.
// Each card has a collapsed row (32pt: bypass toggle, name, wet/dry mini-knob,
// chevron) and an expanded section (~120pt: per-effect param knobs).
//
// Accordion rule: only one slot is expanded at a time to conserve vertical
// space in the 320pt sidebar.
//
// Knob creation is LAZY — expanded param knobs are built on first expand and
// remain alive until the panel is destroyed (or the slot is reset). Attachments
// are destroyed before knobs to satisfy JUCE's destruction-order contract.
//
// Wire into SidebarPanel::setFXInspector() and add as a child of contentArea
// when the FX tab is active.

class FXInspectorPanel : public juce::Component
{
public:
    //==========================================================================
    explicit FXInspectorPanel(juce::AudioProcessorValueTreeState& apvts)
        : myApvts(apvts)
    {
        // Define the 8 FX slots.  The first paramId in expandedParamIds is the
        // primary "mix" param shown in the collapsed strip; the remainder are
        // shown only when expanded.
        static const SlotDef defs[] = {
            { "SATURATION",     "master_satDrive",      { "master_satDrive",    "master_satMode" } },
            { "CORRUPTION",     "master_corrMix",       { "master_corrMix",     "master_corrBits",
                                                          "master_corrSR",      "master_corrFM",
                                                          "master_corrTone" } },
            { "COMB",           "master_combMix",       { "master_combMix",     "master_combFreq",
                                                          "master_combFeedback","master_combDamping" } },
            { "FREQ SHIFT",     "master_fshiftMix",     { "master_fshiftHz",    "master_fshiftMix",
                                                          "master_fshiftMode" } },
            { "REVERB",         "master_reverbMix",     { "master_reverbSize",  "master_reverbMix" } },
            { "COMPRESSOR",     "master_compMix",       { "master_compRatio",   "master_compAttack",
                                                          "master_compRelease",  "master_compMix" } },
            { "DELAY",          "master_delayMix",      { "master_delayTime",   "master_delayFeedback",
                                                          "master_delayMix" } },
            { "MODULATION",     "master_modMix",        { "master_modRate",     "master_modDepth",
                                                          "master_modMix",      "master_modMode" } },
        };

        for (auto& d : defs)
        {
            auto& slot = slots.emplace_back();
            slot.name           = d.name;
            slot.mixParamId     = d.mixParamId;
            for (auto* id : d.expandedIds)
                slot.expandedIds.add(id);

            // Collapsed-strip mix knob — created eagerly.
            slot.mixKnob = std::make_unique<GalleryKnob>();
            slot.mixKnob->setSliderStyle(juce::Slider::RotaryVerticalDrag);
            slot.mixKnob->setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
            slot.mixKnob->setColour(juce::Slider::rotarySliderFillColourId,
                                    GalleryColors::get(GalleryColors::textMid()).withAlpha(0.70f));
            slot.mixKnob->setTooltip(juce::String(d.name) + " wet/dry mix");
            A11y::setup(*slot.mixKnob, juce::String(d.name) + " Mix",
                        "Wet/dry mix for " + juce::String(d.name));
            addAndMakeVisible(*slot.mixKnob);

            // Attachment must outlive the knob; stored in the slot's unique_ptr.
            slot.mixAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                apvts, d.mixParamId, *slot.mixKnob);
            enableKnobReset(*slot.mixKnob, apvts, d.mixParamId);
        }

        // The inner content component holds all slot cards and is scrolled via
        // the Viewport.
        innerContent.setSize(kPanelWidth, computeInnerHeight());
        viewport.setViewedComponent(&innerContent, false);
        viewport.setScrollBarsShown(true, false);
        viewport.setScrollBarThickness(6);
        addAndMakeVisible(viewport);
    }

    ~FXInspectorPanel() override
    {
        // Explicitly destroy each slot (attachment before knobs) in reverse
        // order for deterministic teardown.
        for (auto it = slots.rbegin(); it != slots.rend(); ++it)
            it->teardown();
    }

    //==========================================================================
    void resized() override
    {
        viewport.setBounds(getLocalBounds());
        innerContent.setSize(getWidth(), computeInnerHeight());
        layoutSlots();
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(GalleryColors::get(GalleryColors::shellWhite()));
    }

    //==========================================================================
    // Returns the total content height required to display all slots at their
    // current expanded/collapsed state.  Useful for the parent to size us.
    int getRequiredHeight() const
    {
        return computeInnerHeight();
    }

    //==========================================================================
    // Respond to clicks on slot headers and chevrons.  Because the slots are
    // children of innerContent (not this component directly), this is forwarded
    // by SlotCard::mouseDown below.
    void toggleSlot(int index)
    {
        jassert(juce::isPositiveAndBelow(index, static_cast<int>(slots.size())));

        bool nowExpanded = !slots[static_cast<size_t>(index)].expanded;

        // Accordion: collapse all first, then open the tapped one.
        for (auto& s : slots)
            s.expanded = false;

        if (nowExpanded)
        {
            auto& slot = slots[static_cast<size_t>(index)];
            slot.expanded = true;
            buildExpandedKnobs(slot);  // lazy construction
        }

        // Reflow heights.
        int newH = computeInnerHeight();
        innerContent.setSize(getWidth(), newH);
        layoutSlots();

        // Animate: juce::ComponentAnimator animates bounds.
        // We achieve the accordion motion by immediately assigning the target
        // bounds in layoutSlots() and letting the animator tween each card
        // from its prior position.  The cards are added as direct children of
        // innerContent so their positions are relative to it.
        for (int i = 0; i < static_cast<int>(slotCards.size()); ++i)
        {
            auto target = targetBoundsForSlot(i);
            if (!A11y::prefersReducedMotion())
                animator.animateComponent(slotCards[static_cast<size_t>(i)].get(),
                                          target, 1.0f, 150, false, 0.0, 0.0);
            else
                slotCards[static_cast<size_t>(i)]->setBounds(target);
        }

        repaint();
    }

private:
    //==========================================================================
    // SlotDef — compile-time definition for each FX slot.
    struct SlotDef
    {
        const char*                name;
        const char*                mixParamId;
        std::vector<const char*>   expandedIds;
    };

    //==========================================================================
    // Returns a short uppercase label for a given parameter ID.
    // Maps "master_satDrive" → "DRIVE", etc.
    static juce::String shortLabelForParamId(const juce::String& pid)
    {
        // Strip the "master_" prefix if present.
        juce::String tail = pid.startsWith("master_") ? pid.substring(7) : pid;

        // Explicit short-name table (maps camelCase suffix → display label).
        struct Entry { const char* suffix; const char* label; };
        static const Entry table[] = {
            { "satDrive",       "DRIVE"  },
            { "satMode",        "MODE"   },
            { "corrMix",        "MIX"    },
            { "corrBits",       "BITS"   },
            { "corrSR",         "RATE"   },
            { "corrFM",         "FM"     },
            { "corrTone",       "TONE"   },
            { "combMix",        "MIX"    },
            { "combFreq",       "FREQ"   },
            { "combFeedback",   "FB"     },
            { "combDamping",    "DAMP"   },
            { "fshiftHz",       "HZ"     },
            { "fshiftMix",      "MIX"    },
            { "fshiftMode",     "MODE"   },
            { "reverbSize",     "SIZE"   },
            { "reverbMix",      "MIX"    },
            { "compRatio",      "RATIO"  },
            { "compAttack",     "ATK"    },
            { "compRelease",    "REL"    },
            { "compMix",        "MIX"    },
            { "delayTime",      "TIME"   },
            { "delayFeedback",  "FB"     },
            { "delayMix",       "MIX"    },
            { "modRate",        "RATE"   },
            { "modDepth",       "DEPTH"  },
            { "modMix",         "MIX"    },
            { "modMode",        "MODE"   },
        };
        for (auto& e : table)
            if (tail == e.suffix)
                return e.label;
        // Fallback: uppercase the tail (trim leading digits if any).
        return tail.toUpperCase().substring(0, 6);
    }

    //==========================================================================
    // FXSlot — runtime state for a single FX slot card.
    struct FXSlot
    {
        juce::String     name;
        juce::String     mixParamId;
        juce::StringArray expandedIds;     // all params shown when expanded
        bool             expanded = false;

        // Collapsed strip — always alive.
        std::unique_ptr<GalleryKnob>
            mixKnob;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
            mixAtt;

        // Expanded knobs — created lazily, stored in destruction-safe order.
        // Attachments must be destroyed BEFORE knobs.
        std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>>
            paramAtts;
        std::vector<std::unique_ptr<GalleryKnob>>
            paramKnobs;
        // Param labels shown below each expanded knob.
        std::vector<std::unique_ptr<juce::Label>>
            paramLabels;

        bool expandedKnobsBuilt = false;

        // Safe teardown in correct order: attachments MUST be destroyed
        // before the sliders they reference (JUCE contract).
        // paramAtts → paramKnobs → paramLabels, then mixAtt → mixKnob.
        void teardown()
        {
            paramAtts.clear();   // expanded param attachments first
            paramKnobs.clear();  // then expanded knobs
            paramLabels.clear(); // then labels
            mixAtt.reset();      // mix attachment before mix knob
            mixKnob.reset();     // mix knob last
        }
    };

    //==========================================================================
    // SlotCard — a child component that draws one FX slot card and routes
    // click events back to the panel.
    class SlotCard : public juce::Component
    {
    public:
        SlotCard(FXInspectorPanel& owner, int index)
            : panel(owner), slotIndex(index) {}

        void mouseDown(const juce::MouseEvent& e) override
        {
            // Only the header region (top 32pt) toggles expand/collapse.
            if (e.y <= kHeaderH)
                panel.toggleSlot(slotIndex);
        }

        void paint(juce::Graphics& g) override
        {
            using namespace GalleryColors;
            auto b = getLocalBounds().toFloat();

            // Card background
            g.setColour(get(slotBg()));
            g.fillRoundedRectangle(b, 5.0f);

            // Card border
            g.setColour(get(borderGray()));
            g.drawRoundedRectangle(b.reduced(0.5f), 5.0f, 1.0f);

            const auto& slot = panel.slots[static_cast<size_t>(slotIndex)];

            // Header region highlight
            auto headerR = b.removeFromTop(static_cast<float>(kHeaderH));
            if (slot.expanded)
            {
                g.setColour(get(borderGray()).withAlpha(0.35f));
                g.fillRoundedRectangle(headerR, 5.0f);
            }

            // Bypass LED (7×7 circle, left-aligned)
            // Collapsed: XO Gold at moderate alpha so it's always visible.
            // Expanded:  full XO Gold, fully opaque.
            float ledSize  = 7.0f;
            float ledX     = headerR.getX() + 8.0f;
            float ledY     = headerR.getCentreY() - ledSize * 0.5f;
            g.setColour(slot.expanded
                        ? get(xoGold)
                        : get(xoGold).withAlpha(0.55f));
            g.fillEllipse(ledX, ledY, ledSize, ledSize);
            g.setColour(slot.expanded
                        ? get(xoGold).withAlpha(0.80f)
                        : get(borderGray()).withAlpha(0.60f));
            g.drawEllipse(ledX, ledY, ledSize, ledSize, 1.0f);

            // FX name label — Space Grotesk SemiBold 10pt
            float nameX = ledX + ledSize + 6.0f;
            float nameW = headerR.getRight() - kChevronW - kMixKnobW - nameX - 4.0f;
            g.setColour(get(textDark()));
            g.setFont(GalleryFonts::display(10.0f));
            g.drawText(slot.name,
                       juce::Rectangle<float>(nameX,
                                              headerR.getY(),
                                              nameW,
                                              static_cast<float>(kHeaderH)),
                       juce::Justification::centredLeft, true);

            // Chevron (right edge of header)
            float cx = b.getRight() - static_cast<float>(kChevronW) * 0.5f;
            float cy = headerR.getCentreY();
            float cs = 5.0f;
            g.setColour(get(textMid()).withAlpha(0.60f));
            juce::Path chevron;
            if (slot.expanded)
            {
                // Up chevron
                chevron.startNewSubPath(cx - cs, cy + cs * 0.4f);
                chevron.lineTo(cx,           cy - cs * 0.4f);
                chevron.lineTo(cx + cs,       cy + cs * 0.4f);
            }
            else
            {
                // Down chevron
                chevron.startNewSubPath(cx - cs, cy - cs * 0.4f);
                chevron.lineTo(cx,           cy + cs * 0.4f);
                chevron.lineTo(cx + cs,       cy - cs * 0.4f);
            }
            g.strokePath(chevron,
                         juce::PathStrokeType(1.5f,
                                              juce::PathStrokeType::curved,
                                              juce::PathStrokeType::rounded));

            // Divider between header and expanded area
            if (slot.expanded && getHeight() > kHeaderH)
            {
                g.setColour(get(borderGray()).withAlpha(0.60f));
                g.drawHorizontalLine(kHeaderH,
                                     headerR.getX() + 4.0f,
                                     b.getRight() - 4.0f);
            }
        }

        void resized() override
        {
            const auto& slot = panel.slots[static_cast<size_t>(slotIndex)];

            // Mix knob — in collapsed strip
            if (slot.mixKnob != nullptr)
            {
                int ky = (kHeaderH - kMixKnobW) / 2;
                slot.mixKnob->setBounds(getWidth() - kChevronW - kMixKnobW - 2,
                                        ky,
                                        kMixKnobW,
                                        kMixKnobW);
            }

            // Expanded param knobs — horizontal row below header
            if (slot.expanded && !slot.paramKnobs.empty())
            {
                int n       = static_cast<int>(slot.paramKnobs.size());
                int xArea   = getWidth() - 16;
                int cellW   = xArea / n;
                int kSize   = juce::jmin(cellW - 4, kExpandedKnobSize);
                int startX  = 8;
                // Centre the knob+label block vertically within the expanded area.
                // Block height = kSize (knob) + 2px gap + kLabelH (label).
                int blockH  = kSize + 2 + kLabelH;
                int kTop    = kHeaderH + (kExpandedAreaH - blockH) / 2;
                int lblTop  = kTop + kSize + 2;

                for (int i = 0; i < n; ++i)
                {
                    int cellX = startX + i * cellW;
                    int kX    = cellX + (cellW - kSize) / 2;

                    if (slot.paramKnobs[static_cast<size_t>(i)] != nullptr)
                        slot.paramKnobs[static_cast<size_t>(i)]->setBounds(kX, kTop, kSize, kSize);

                    if (i < static_cast<int>(slot.paramLabels.size()) &&
                        slot.paramLabels[static_cast<size_t>(i)] != nullptr)
                    {
                        slot.paramLabels[static_cast<size_t>(i)]->setBounds(
                            cellX, lblTop, cellW, kLabelH);
                    }
                }
            }
        }

        static constexpr int kHeaderH        = 32;
        static constexpr int kMixKnobW       = 24;
        static constexpr int kChevronW       = 22;
        static constexpr int kExpandedAreaH  = 120;
        static constexpr int kExpandedKnobSize = 36;
        static constexpr int kLabelH         = 12;

    private:
        FXInspectorPanel& panel;
        int               slotIndex;
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SlotCard)
    };

    //==========================================================================
    // Build expanded knobs for a slot the first time it is opened.
    void buildExpandedKnobs(FXSlot& slot)
    {
        if (slot.expandedKnobsBuilt)
            return;
        slot.expandedKnobsBuilt = true;

        int slotIdx = slotIndexOf(slot);

        for (int i = 0; i < slot.expandedIds.size(); ++i)
        {
            const juce::String& pid = slot.expandedIds[i];

            auto knob = std::make_unique<GalleryKnob>();
            knob->setSliderStyle(juce::Slider::RotaryVerticalDrag);
            knob->setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
            knob->setColour(juce::Slider::rotarySliderFillColourId,
                            GalleryColors::get(GalleryColors::textMid()).withAlpha(0.65f));
            knob->setTooltip(pid);
            A11y::setup(*knob, pid, "FX parameter " + pid);

            // Build the label shown below this knob.
            auto lbl = std::make_unique<juce::Label>();
            lbl->setText(shortLabelForParamId(pid), juce::dontSendNotification);
            lbl->setFont(GalleryFonts::value(11.0f));
            lbl->setColour(juce::Label::textColourId,
                           GalleryColors::get(GalleryColors::t2()));
            lbl->setJustificationType(juce::Justification::centred);
            lbl->setInterceptsMouseClicks(false, false);

            // Add to the SlotCard (not to innerContent directly).
            if (slotIdx >= 0 && slotIdx < static_cast<int>(slotCards.size()))
            {
                slotCards[static_cast<size_t>(slotIdx)]->addAndMakeVisible(*knob);
                slotCards[static_cast<size_t>(slotIdx)]->addAndMakeVisible(*lbl);
            }

            auto att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                myApvts, pid, *knob);
            enableKnobReset(*knob, myApvts, pid);

            // Store atts first so they are destroyed before knobs.
            slot.paramAtts.push_back(std::move(att));
            slot.paramKnobs.push_back(std::move(knob));
            slot.paramLabels.push_back(std::move(lbl));
        }
    }

    //==========================================================================
    // Returns the index of a slot within the slots vector, or -1 if not found.
    int slotIndexOf(const FXSlot& target) const
    {
        for (int i = 0; i < static_cast<int>(slots.size()); ++i)
        {
            if (&slots[static_cast<size_t>(i)] == &target)
                return i;
        }
        return -1;
    }

    //==========================================================================
    // Total height of innerContent given current expand/collapse state.
    int computeInnerHeight() const
    {
        int h = kTopPad;
        for (auto& s : slots)
            h += slotHeight(s) + kSlotGap;
        h += kBottomPad;
        return juce::jmax(h, 1);
    }

    int slotHeight(const FXSlot& s) const noexcept
    {
        return SlotCard::kHeaderH + (s.expanded ? SlotCard::kExpandedAreaH : 0);
    }

    //==========================================================================
    // Compute the absolute bounds for slot[i] within innerContent.
    juce::Rectangle<int> targetBoundsForSlot(int index) const
    {
        int y = kTopPad;
        for (int i = 0; i < static_cast<int>(slots.size()); ++i)
        {
            const auto& s  = slots[static_cast<size_t>(i)];
            int          sh = slotHeight(s);
            if (i == index)
                return { kCardInset, y, innerContent.getWidth() - kCardInset * 2, sh };
            y += sh + kSlotGap;
        }
        return {};
    }

    //==========================================================================
    // Place all slot cards at their current target bounds (no animation).
    void layoutSlots()
    {
        // Ensure we have one SlotCard per FXSlot.
        while (static_cast<int>(slotCards.size()) < static_cast<int>(slots.size()))
        {
            int idx = static_cast<int>(slotCards.size());
            auto card = std::make_unique<SlotCard>(*this, idx);

            // Add mix knob as child of the card.
            auto& slot = slots[static_cast<size_t>(idx)];
            if (slot.mixKnob != nullptr)
                card->addAndMakeVisible(*slot.mixKnob);

            innerContent.addAndMakeVisible(*card);
            slotCards.push_back(std::move(card));
        }

        for (int i = 0; i < static_cast<int>(slots.size()); ++i)
        {
            auto bounds = targetBoundsForSlot(i);
            slotCards[static_cast<size_t>(i)]->setBounds(bounds);
            slotCards[static_cast<size_t>(i)]->resized();
        }
    }

    //==========================================================================
    static constexpr int kPanelWidth  = 320;
    static constexpr int kCardInset   = 6;
    static constexpr int kTopPad      = 8;
    static constexpr int kBottomPad   = 12;
    static constexpr int kSlotGap     = 4;

    //==========================================================================
    juce::AudioProcessorValueTreeState& myApvts;

    // FX slot data — owns knobs and attachments.
    std::vector<FXSlot>                              slots;

    // One SlotCard per slot — owned here, parented to innerContent.
    std::vector<std::unique_ptr<SlotCard>>           slotCards;

    // Inner scrollable surface — NOT owned by viewport (false flag in ctor).
    juce::Component                                  innerContent;

    // Viewport wraps innerContent.
    juce::Viewport                                   viewport;

    // Animator for accordion transitions.
    juce::ComponentAnimator                          animator;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FXInspectorPanel)
};

} // namespace xoceanus
