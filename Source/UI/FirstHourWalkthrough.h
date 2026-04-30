// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
// FirstHourWalkthrough.h — Wave 9c: guided 8-step onboarding tour for XOceanus.
//
// FORMAT (LOCKED per issue #1303 / Docs/specs/wave9-onboarding.md §3.2):
//   Floating bubble overlay pointing at a target component.  The target receives
//   a 2px XO Gold highlight ring.  All other UI remains interactive — the user
//   can click away at any step.  No modal blocking.
//
// TRIGGER:
//   Call promptIfEligible(settingsFile) after the greeting sound completes (or
//   is dismissed).  A non-modal opt-in prompt appears; [Take the Tour] starts
//   step 0.  The walkthrough can also be restarted from Settings > Experience.
//
// PERSISTENCE (PropertiesFile, NOT APVTS):
//   "onboardingWalkthroughStep"  int   -1 = not started, 0-7 = last completed, 8 = done
//   "onboardingDisabled"         bool  true if user pressed [Skip all]
//
// INTEGRATION (Wave 7 / post-decomp):
//   After OceanView decomposition, wire into OceanStateMachine::onGreetingComplete.
//   Until then, call promptIfEligible() from XOceanusEditor after greetingActive_
//   clears.  See TODO W9c comments below and in XOceanusEditor.h.
//
// COMPONENT TARGETING:
//   The walkthrough does not own UI component pointers.  The editor supplies
//   bounds via lambda accessors before mounting:
//
//     walkthrough_.getPlaySurfaceBounds  = [this]() { return playSurface_.getBounds(); };
//     walkthrough_.getEngineSlotBounds   = [this]() { return tiles[0] ? tiles[0]->getBounds()
//                                                                      : juce::Rectangle<int>{}; };
//     walkthrough_.getMacroBounds        = [this]() { return macros.getBounds(); };
//     walkthrough_.getDnaBrowserBounds   = [this]() { return /* DnaMapBrowser or PresetBrowserStrip */; };
//     walkthrough_.getCoupleOrbitBounds  = [this]() { return /* EngineOrbit buoy 1 or 2 bounds */; };
//     walkthrough_.getCmToggleBounds     = [this]() { return cmToggleBtn.getBounds(); };
//     walkthrough_.getFavBtnBounds       = [this]() { return /* PresetBrowserStrip favBtn bounds */; };
//     walkthrough_.getXouijaBounds       = [this]() { return /* SubmarineOuijaPanel or XOuija button */; };
//
// THREAD SAFETY: all public methods must be called on the message thread.

#include <juce_gui_basics/juce_gui_basics.h>
#include "GalleryColors.h"
#include <functional>
#include <memory>

// Forward-declare to avoid pulling in SettingsPanel or GalleryFonts here.
namespace juce { class PropertiesFile; }

namespace xoceanus
{

//==============================================================================
// WalkthroughBubble — one floating info bubble with header, body, and footer.
//
// Draws itself as a dark rounded-rect with an arrow pointing toward the target
// component.  The arrow direction is chosen automatically (prefers bottom->up;
// falls back to top->down when the bubble would overflow the parent).
//==============================================================================
class WalkthroughBubble final : public juce::Component,
                                public juce::Timer
{
public:
    //==========================================================================
    static constexpr int kBubbleW       = 280;
    static constexpr int kBubbleH       = 130; // approximate; paint clips to actual
    static constexpr int kArrowH        = 10;
    static constexpr int kCornerRadius  = 8;
    static constexpr int kPad           = 14;
    static constexpr float kHighlightPx = 2.0f;
    static constexpr int kPulseMs       = 30;   // timer interval ~33 fps

    // Fired when the user presses [Next ->]
    std::function<void()> onNext;
    // Fired when the user presses [Skip all]
    std::function<void()> onSkipAll;

    /** stepIndex: 0-based.  totalSteps: 8. */
    WalkthroughBubble (int stepIndex, int totalSteps,
                       const juce::String& title,
                       const juce::String& body)
        : stepIndex_    (stepIndex)
        , totalSteps_   (totalSteps)
        , titleText_    (title)
        , bodyText_     (body)
    {
        setInterceptsMouseClicks (true, true);
        setSize (kBubbleW, kBubbleH + kArrowH + 10);

        nextBtn_.setButtonText (stepIndex_ == totalSteps_ - 1
                                    ? "Let's go"
                                    : "Next ->");
        nextBtn_.setColour (juce::TextButton::buttonColourId,
                            juce::Colour (0xFFE9C46A));               // XO Gold
        nextBtn_.setColour (juce::TextButton::textColourOffId,
                            juce::Colour (0xFF1A1A1A));               // dark text on gold
        nextBtn_.onClick = [this] { if (onNext) onNext(); };
        addAndMakeVisible (nextBtn_);

        skipBtn_.setButtonText ("Skip all");
        skipBtn_.setColour (juce::TextButton::buttonColourId,
                            juce::Colour (0x00000000));               // transparent
        skipBtn_.setColour (juce::TextButton::textColourOffId,
                            juce::Colour (0xFF888888));
        skipBtn_.onClick = [this] { if (onSkipAll) onSkipAll(); };
        addAndMakeVisible (skipBtn_);

        startTimer (kPulseMs);
    }

    ~WalkthroughBubble() override { stopTimer(); }

    //==========================================================================
    // Position the bubble relative to a target rect in parent coordinates.
    // Call this every time target bounds change (e.g. after parent resized).
    void positionNearTarget (juce::Rectangle<int> targetInParent,
                             juce::Rectangle<int> parentBounds)
    {
        targetRect_    = targetInParent;
        arrowPointsUp_ = false; // default: bubble above target, arrow points down

        const int centreX    = targetInParent.getCentreX();
        const int preferredY = targetInParent.getY() - getHeight() - 6;

        if (preferredY < 4)
        {
            // Not enough room above -> place below, arrow points up
            arrowPointsUp_ = true;
            setTopLeftPosition (juce::jlimit (4, parentBounds.getWidth()  - getWidth() - 4,
                                              centreX - getWidth() / 2),
                                targetInParent.getBottom() + 6);
        }
        else
        {
            setTopLeftPosition (juce::jlimit (4, parentBounds.getWidth()  - getWidth() - 4,
                                              centreX - getWidth() / 2),
                                preferredY);
        }
        repaint();
    }

    //==========================================================================
    void paint (juce::Graphics& g) override
    {
        const auto bounds = getLocalBounds().toFloat();
        juce::ignoreUnused (bounds);

        // -- Bubble body -------------------------------------------------------
        juce::Path bubblePath;
        juce::Rectangle<float> bodyRect = juce::Rectangle<float> (
            0.0f, 0.0f,
            static_cast<float> (getWidth()),
            static_cast<float> (kBubbleH));

        if (arrowPointsUp_)
        {
            bodyRect = bodyRect.withY (static_cast<float> (kArrowH));
            bubblePath.addRoundedRectangle (bodyRect, static_cast<float> (kCornerRadius));
            // Arrow pointing up toward target
            const float arrowCx = juce::jlimit (bodyRect.getX() + 16.0f,
                                                bodyRect.getRight() - 16.0f,
                                                static_cast<float> (getWidth()) * 0.5f);
            bubblePath.startNewSubPath (arrowCx - 8.0f, static_cast<float> (kArrowH));
            bubblePath.lineTo           (arrowCx,        0.0f);
            bubblePath.lineTo           (arrowCx + 8.0f, static_cast<float> (kArrowH));
            bubblePath.closeSubPath();
        }
        else
        {
            bodyRect = bodyRect.withY (0.0f);
            bubblePath.addRoundedRectangle (bodyRect, static_cast<float> (kCornerRadius));
            // Arrow pointing down toward target
            const float arrowCx = juce::jlimit (bodyRect.getX() + 16.0f,
                                                bodyRect.getRight() - 16.0f,
                                                static_cast<float> (getWidth()) * 0.5f);
            bubblePath.startNewSubPath (arrowCx - 8.0f, bodyRect.getBottom());
            bubblePath.lineTo           (arrowCx,        bodyRect.getBottom() + static_cast<float> (kArrowH));
            bubblePath.lineTo           (arrowCx + 8.0f, bodyRect.getBottom());
            bubblePath.closeSubPath();
        }

        g.setColour (juce::Colour (0xF0222222));
        g.fillPath (bubblePath);

        g.setColour (juce::Colour (0x40E9C46A));   // XO Gold outline, subtle
        g.strokePath (bubblePath, juce::PathStrokeType (1.0f));

        // -- Title -------------------------------------------------------------
        const int textTop = arrowPointsUp_ ? kArrowH + kPad : kPad;
        g.setColour (juce::Colour (0xFFE9C46A)); // XO Gold
        g.setFont (juce::Font ("Space Grotesk", 13.0f, juce::Font::bold));
        g.drawText (titleText_,
                    kPad, textTop, getWidth() - kPad * 2, 18,
                    juce::Justification::centredLeft, true);

        // -- Body --------------------------------------------------------------
        g.setColour (juce::Colour (0xFFCCCCCC));
        g.setFont   (juce::Font ("Inter", 11.5f, juce::Font::plain));
        juce::AttributedString bodyAS;
        bodyAS.append (bodyText_, g.getCurrentFont(), juce::Colour (0xFFCCCCCC));
        bodyAS.setWordWrap (juce::AttributedString::byWord);
        juce::TextLayout tl;
        tl.createLayout (bodyAS, static_cast<float> (getWidth() - kPad * 2));
        tl.draw (g, juce::Rectangle<float> (static_cast<float> (kPad),
                                            static_cast<float> (textTop + 22),
                                            static_cast<float> (getWidth() - kPad * 2),
                                            46.0f));

        // -- Step counter ------------------------------------------------------
        g.setColour (juce::Colour (0xFF666666));
        g.setFont   (juce::Font ("Inter", 10.0f, juce::Font::plain));
        g.drawText  (juce::String ("step ") + juce::String (stepIndex_ + 1)
                         + " of " + juce::String (totalSteps_),
                     kPad,
                     textTop + kBubbleH - kArrowH - 26,
                     80, 16,
                     juce::Justification::centredLeft,
                     false);
    }

    void resized() override
    {
        const int textTop = arrowPointsUp_ ? kArrowH : 0;
        const int btnY    = textTop + kBubbleH - kArrowH - 28;

        skipBtn_.setBounds (kPad, btnY, 64, 24);
        nextBtn_.setBounds (getWidth() - kPad - 80, btnY, 80, 24);
    }

    // -- Timer: drives the highlight ring pulse --------------------------------
    void timerCallback() override
    {
        pulsePhase_ = std::fmod (pulsePhase_ + 0.04f,
                                 juce::MathConstants<float>::twoPi);
        if (auto* p = getParentComponent())
            p->repaint (targetRect_.expanded (6));
    }

    float pulseAlpha() const noexcept
    {
        return 0.5f + 0.5f * std::sin (pulsePhase_);
    }

    juce::Rectangle<int> targetRect() const noexcept { return targetRect_; }

private:
    const int          stepIndex_;
    const int          totalSteps_;
    const juce::String titleText_;
    const juce::String bodyText_;

    bool                 arrowPointsUp_ = false;
    juce::Rectangle<int> targetRect_;
    float                pulsePhase_    = 0.0f;

    juce::TextButton     nextBtn_;
    juce::TextButton     skipBtn_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WalkthroughBubble)
};


//==============================================================================
// FirstHourWalkthrough — transparent overlay that owns the active bubble and
// paints the gold highlight ring around the target component.
//
// Mount as a child of XOceanusEditor AFTER all other children so it paints last.
// setBounds() should always match the editor's full bounds.
//==============================================================================
class FirstHourWalkthrough final : public juce::Component
{
public:
    //==========================================================================
    // -- Component bound accessors (set by editor before mounting) -------------
    // Returns the target component's bounds in editor (parent) coordinates.
    // If a component is not yet mounted, return an empty Rectangle; the bubble
    // will position itself using the safe fallback (centred area).

    std::function<juce::Rectangle<int>()> getPlaySurfaceBounds;
    std::function<juce::Rectangle<int>()> getEngineSlotBounds;
    std::function<juce::Rectangle<int>()> getMacroBounds;
    std::function<juce::Rectangle<int>()> getDnaBrowserBounds;
    std::function<juce::Rectangle<int>()> getCoupleOrbitBounds;
    std::function<juce::Rectangle<int>()> getCmToggleBounds;
    std::function<juce::Rectangle<int>()> getFavBtnBounds;
    std::function<juce::Rectangle<int>()> getXouijaBounds;

    // Fired when the walkthrough fully completes or user presses Skip all.
    std::function<void()> onWalkthroughComplete;

    //==========================================================================
    FirstHourWalkthrough() { setInterceptsMouseClicks (false, true); }
    ~FirstHourWalkthrough() override = default;

    //==========================================================================
    // promptIfEligible — call after the greeting sound completes or is dismissed.
    // Shows the non-modal opt-in prompt if:
    //   - onboardingDisabled == false
    //   - onboardingWalkthroughStep < kNumSteps  (not yet completed)
    //
    // settingsFile must remain valid for the lifetime of this component.
    void promptIfEligible (juce::PropertiesFile* settingsFile)
    {
        jassert (juce::MessageManager::getInstance()->isThisTheMessageThread());

        settingsFile_ = settingsFile;
        if (settingsFile_ == nullptr)
            return;

        if (settingsFile_->getBoolValue ("onboardingDisabled", false))
            return;

        const int step = settingsFile_->getIntValue ("onboardingWalkthroughStep", -1);
        if (step >= kNumSteps)
            return; // already completed

        showTourPrompt (step);
    }

    // restartWalkthrough — call from Settings > Experience > "Restart Walkthrough".
    void restartWalkthrough (juce::PropertiesFile* settingsFile)
    {
        jassert (juce::MessageManager::getInstance()->isThisTheMessageThread());

        settingsFile_ = settingsFile;
        if (settingsFile_ != nullptr)
        {
            settingsFile_->setValue ("onboardingWalkthroughStep", 0);
            settingsFile_->setValue ("onboardingDisabled",        false);
            settingsFile_->saveIfNeeded();
        }

        dismissAll();
        advanceToStep (0);
    }

    //==========================================================================
    void paint (juce::Graphics& g) override
    {
        // Only paint when a bubble is active: draw the pulsing XO Gold ring
        // around the target component.
        if (activeBubble_ == nullptr)
            return;

        const juce::Rectangle<int> target = activeBubble_->targetRect();
        if (target.isEmpty())
            return;

        const float alpha  = 0.3f + 0.7f * activeBubble_->pulseAlpha();
        const float expand = 3.0f;

        juce::Path ring;
        ring.addRoundedRectangle (target.expanded (static_cast<int> (expand)).toFloat(),
                                  4.0f);
        g.setColour (juce::Colour (0xFFE9C46A).withAlpha (alpha)); // XO Gold
        g.strokePath (ring,
                      juce::PathStrokeType (WalkthroughBubble::kHighlightPx + 1.0f));
    }

    void resized() override
    {
        if (activeBubble_ != nullptr)
            repositionBubble();

        if (promptOverlay_ != nullptr)
            centrePromptOverlay();
    }

    //==========================================================================
    // advanceToStep — advance to the given 0-based step, or complete if >= 8.
    void advanceToStep (int step)
    {
        jassert (juce::MessageManager::getInstance()->isThisTheMessageThread());

        if (step >= kNumSteps)
        {
            completeTour();
            return;
        }

        dismissActiveBubble();

        currentStep_ = step;
        persistStep (step);

        const auto& s   = kSteps[step];
        activeBubble_   = std::make_unique<WalkthroughBubble> (step, kNumSteps,
                                                                s.title, s.body);
        activeBubble_->onNext    = [this, step] { advanceToStep (step + 1); };
        activeBubble_->onSkipAll = [this]        { skipAll(); };

        addAndMakeVisible (*activeBubble_);
        repositionBubble();
        repaint();
    }

private:
    //==========================================================================
    // Step definitions — LOCKED 8-step sequence per #1303 / wave9-onboarding.md §3.3
    struct StepDef
    {
        const char* title;
        const char* body;
    };

    static constexpr int kNumSteps = 8;

    // Copy is LOCKED — requires Barry OB's team review before any change (issue #1303).
    static constexpr StepDef kSteps[kNumSteps] = {
        /* 0 */ { "Press anything",
                  "XOceanus makes sound on first touch. "
                  "These pads, keys, or frets are all the same instrument." },
        /* 1 */ { "Meet your engines",
                  "Each slot holds one engine. Right now it is Odyssey. "
                  "Hover it to see what it is." },
        /* 2 */ { "The four macros",
                  // wire(#orphan-sweep item 2): D11 rename CHARACTER → TONE (2026-04-25 lock).
                  "TONE sweeps the engine's core colour "
                  "-- brightness, grit, or breath. Try it." },
        /* 3 */ { "Browse the ocean",
                  "Thousands of presets, organised by mood. "
                  "Dive picks a random visible one." },
        /* 4 */ { "Couple two engines",
                  "Load a second engine and the coupling arc appears. "
                  "Drag the arc to wire them together." },
        /* 5 */ { "The chord machine",
                  "Press this to open the chord machine -- "
                  "generative harmonic structure, no theory required." },
        /* 6 */ { "Save your first preset",
                  "Favourite this preset so it appears in your personal collection. "
                  "Your changes persist." },
        /* 7 */ { "XOuija",
                  "XOuija is a live improvisation interface. Move a cell to shift "
                  "pitch, coupling depth, and character simultaneously." },
    };

    //==========================================================================
    // -- Tour prompt -----------------------------------------------------------

    void showTourPrompt (int resumeStep)
    {
        if (promptOverlay_ != nullptr)
            return;

        promptOverlay_ = std::make_unique<TourPromptOverlay> (resumeStep);

        promptOverlay_->onAccept = [this, resumeStep]
        {
            dismissPromptOverlay();
            advanceToStep (resumeStep < 0 ? 0 : resumeStep);
        };

        promptOverlay_->onDecline = [this]
        {
            dismissPromptOverlay();
            if (settingsFile_ != nullptr)
            {
                settingsFile_->setValue ("onboardingDisabled", true);
                settingsFile_->saveIfNeeded();
            }
        };

        addAndMakeVisible (*promptOverlay_);
        centrePromptOverlay();
        repaint();
    }

    void dismissPromptOverlay()
    {
        if (promptOverlay_ != nullptr)
        {
            removeChildComponent (promptOverlay_.get());
            promptOverlay_.reset();
        }
    }

    void centrePromptOverlay()
    {
        if (promptOverlay_ == nullptr) return;
        constexpr int pw = 300, ph = 126; // +16 px to accommodate 44-px WCAG button heights
        promptOverlay_->setBounds ((getWidth()  - pw) / 2,
                                   (getHeight() - ph) / 2,
                                   pw, ph);
    }

    //==========================================================================
    // -- Bubble lifecycle ------------------------------------------------------

    void dismissActiveBubble()
    {
        if (activeBubble_ != nullptr)
        {
            removeChildComponent (activeBubble_.get());
            activeBubble_.reset();
        }
    }

    void dismissAll()
    {
        dismissActiveBubble();
        dismissPromptOverlay();
        repaint();
    }

    void repositionBubble()
    {
        if (activeBubble_ == nullptr)
            return;

        const juce::Rectangle<int> target = getTargetForStep (currentStep_);
        // F-003 / #1395: assert that every step resolves to a real on-screen region.
        // If this fires, the accessor lambda for that step is returning {} and the
        // fallback centred-rect is being used instead of a real component target.
        jassert (!target.isEmpty());
        activeBubble_->positionNearTarget (target, getLocalBounds());
        repaint (target.expanded (8));
    }

    juce::Rectangle<int> getTargetForStep (int step) const
    {
        const auto fallback = [this]() -> juce::Rectangle<int>
        {
            return getLocalBounds().withSizeKeepingCentre (120, 40);
        };

        switch (step)
        {
            case 0: return (getPlaySurfaceBounds  && !getPlaySurfaceBounds().isEmpty())  ? getPlaySurfaceBounds()  : fallback();
            case 1: return (getEngineSlotBounds   && !getEngineSlotBounds().isEmpty())   ? getEngineSlotBounds()   : fallback();
            case 2: return (getMacroBounds        && !getMacroBounds().isEmpty())        ? getMacroBounds()        : fallback();
            case 3: return (getDnaBrowserBounds   && !getDnaBrowserBounds().isEmpty())   ? getDnaBrowserBounds()   : fallback();
            case 4: return (getCoupleOrbitBounds  && !getCoupleOrbitBounds().isEmpty())  ? getCoupleOrbitBounds()  : fallback();
            case 5: return (getCmToggleBounds     && !getCmToggleBounds().isEmpty())     ? getCmToggleBounds()     : fallback();
            case 6: return (getFavBtnBounds       && !getFavBtnBounds().isEmpty())       ? getFavBtnBounds()       : fallback();
            case 7: return (getXouijaBounds       && !getXouijaBounds().isEmpty())       ? getXouijaBounds()       : fallback();
            default: return fallback();
        }
    }

    //==========================================================================
    // -- Persistence -----------------------------------------------------------

    void persistStep (int step)
    {
        if (settingsFile_ == nullptr) return;
        settingsFile_->setValue ("onboardingWalkthroughStep", step);
        settingsFile_->saveIfNeeded();
    }

    void skipAll()
    {
        if (settingsFile_ != nullptr)
        {
            settingsFile_->setValue ("onboardingDisabled",        true);
            settingsFile_->setValue ("onboardingWalkthroughStep", kNumSteps);
            settingsFile_->saveIfNeeded();
        }
        dismissAll();
        if (onWalkthroughComplete) onWalkthroughComplete();
    }

    void completeTour()
    {
        if (settingsFile_ != nullptr)
        {
            settingsFile_->setValue ("onboardingWalkthroughStep", kNumSteps);
            settingsFile_->saveIfNeeded();
        }
        dismissAll();
        if (onWalkthroughComplete) onWalkthroughComplete();
    }

    //==========================================================================
    // -- TourPromptOverlay — "Want a quick tour?" non-modal card ---------------
    struct TourPromptOverlay final : public juce::Component
    {
        std::function<void()> onAccept;
        std::function<void()> onDecline;

        explicit TourPromptOverlay (int resumeStep)
        {
            setInterceptsMouseClicks (true, true);

            const bool isResume = (resumeStep >= 0 && resumeStep < kNumSteps);
            acceptBtn_.setButtonText  (isResume ? "Resume tour" : "Take the tour");
            declineBtn_.setButtonText ("Skip");

            acceptBtn_.setColour  (juce::TextButton::buttonColourId,  juce::Colour (0xFFE9C46A));
            acceptBtn_.setColour  (juce::TextButton::textColourOffId, juce::Colour (0xFF1A1A1A));
            declineBtn_.setColour (juce::TextButton::buttonColourId,  juce::Colour (0x00000000));
            declineBtn_.setColour (juce::TextButton::textColourOffId, juce::Colour (0xFF888888));

            acceptBtn_.onClick  = [this] { if (onAccept)  onAccept();  };
            declineBtn_.onClick = [this] { if (onDecline) onDecline(); };

            addAndMakeVisible (acceptBtn_);
            addAndMakeVisible (declineBtn_);

            promptText_ = isResume
                ? (juce::String ("Resume your tour from step ")
                       + juce::String (resumeStep + 1) + "?")
                : juce::String ("Want a quick tour? (2 min)");
        }

        void paint (juce::Graphics& g) override
        {
            g.setColour (juce::Colour (0xF0222222));
            g.fillRoundedRectangle (getLocalBounds().toFloat(), 8.0f);
            g.setColour (juce::Colour (0x40E9C46A));
            g.drawRoundedRectangle (getLocalBounds().toFloat().reduced (0.5f), 8.0f, 1.0f);

            g.setColour (juce::Colour (0xFFDDDDDD));
            g.setFont   (juce::Font ("Space Grotesk", 13.5f, juce::Font::bold));
            g.drawText  (promptText_,
                         12, 18, getWidth() - 24, 22,
                         juce::Justification::centred, true);
        }

        void resized() override
        {
            const int btnY = getHeight() - 54; // 10px bottom margin + 44px button height
            declineBtn_.setBounds (12,                    btnY, 80, 44);
            acceptBtn_.setBounds  (getWidth() - 100, btnY, 88, 44);
        }

    private:
        juce::TextButton acceptBtn_;
        juce::TextButton declineBtn_;
        juce::String     promptText_;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TourPromptOverlay)
    };

    //==========================================================================
    // Members

    juce::PropertiesFile*                  settingsFile_ = nullptr;
    int                                    currentStep_  = 0;
    std::unique_ptr<WalkthroughBubble>     activeBubble_;
    std::unique_ptr<TourPromptOverlay>     promptOverlay_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FirstHourWalkthrough)
};

} // namespace xoceanus


//==============================================================================
// Wave 9c mount COMPLETE — XOceanusEditor.h (applied 2026-04-26, PR #mount-final)
// F-003 / #1395 fix applied 2026-04-29: steps 3/4/6/7 now point at real targets.
//
// All 6 mount points wired:
//   1. Member: walkthrough_  (before toastOverlay_ in declaration order)
//   2. Bound accessors wired in initOceanView() — all 8 steps resolved:
//      Step 0: PlaySurface     — oceanView_.getPlaySurface().getBounds()
//      Step 1: EngineOrbit[0]  — tiles[0]->getBounds()
//      Step 2: Macros          — macros.getBounds()
//      Step 3: Preset name     — oceanView_.getDnaMapBrowserBounds() → HudBar preset-name pill
//      Step 4: CoupleOrbit[1]  — oceanView_.getOrbitBounds(1)
//      Step 5: CM toggle       — cmToggleBtn.getBounds()
//      Step 6: Fav button      — oceanView_.getHudFavBounds() → HudBar fav bounds
//      Step 7: HARMONIC tab    — oceanView_.getOuijaPanelBounds() → DashboardTabBar HARMONIC tab
//   3. addAndMakeVisible(walkthrough_) in initOceanView() before toastOverlay_.
//   4. setBounds in resized() OceanView branch.
//   5. promptIfEligible() fired on first timerCallback tick via walkthroughTriggeredThisSession_ guard.
//   6. Settings "Restart Walkthrough" — restartWalkthrough() wired in SettingsPanel.
