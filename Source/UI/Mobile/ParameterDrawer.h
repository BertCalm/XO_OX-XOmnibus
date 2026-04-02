// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "MobileLayoutManager.h"
#include "HapticEngine.h"

namespace xoceanus {

//==============================================================================
// ParameterDrawer — iPhone bottom drawer for engine parameters.
//
// A slide-up panel with 4 snap states: Closed, Peek, Half, Full.
// Each state reveals progressively more parameter controls.
//
// Interaction:
//   - Swipe up from bottom edge: advance to next state
//   - Swipe down on drawer: retreat to previous state
//   - Tap handle bar: toggle Closed ↔ Peek
//   - Tap outside drawer (while open): close
//   - Spring physics for smooth snap transitions
//
// Content:
//   - Peek: macro knobs + preset name + engine badges
//   - Half: 8-12 key parameters grouped by section (Osc, Filter, Amp, FX)
//   - Full: all parameters, scrollable, tabbed by engine
//
class ParameterDrawer : public juce::Component {
public:
    enum class State {
        Closed,     // Only handle bar visible (8pt)
        Peek,       // Macro knobs + preset info (100pt)
        Half,       // Key parameters (50% screen)
        Full        // All parameters, scrollable (90% screen)
    };

    ParameterDrawer()
    {
        handleBar.setInterceptsMouseClicks(true, false);
        addAndMakeVisible(handleBar);

        contentViewport.setScrollBarsShown(true, false);
        addAndMakeVisible(contentViewport);
    }

    //-- Configuration -----------------------------------------------------------

    void setHapticEngine(HapticEngine* engine) { haptics = engine; }

    // Set maximum allowed state (Intuitive mode caps at Half)
    void setMaxState(State max) { maxState = max; }

    // Set whether mode is Intuitive (starts Closed) or Advanced (starts Peek)
    void setDefaultState(State def)
    {
        if (currentState == State::Closed || currentState == defaultState)
        {
            defaultState = def;
            animateToState(def);
        }
        else
        {
            defaultState = def;
        }
    }

    State getCurrentState() const { return currentState; }

    //-- State transitions -------------------------------------------------------

    void setState(State newState)
    {
        if (newState > maxState)
            newState = maxState;
        animateToState(newState);
    }

    void advanceState()
    {
        switch (currentState)
        {
            case State::Closed: setState(State::Peek); break;
            case State::Peek:   setState(State::Half); break;
            case State::Half:   setState(State::Full); break;
            case State::Full:   break;
        }
    }

    void retreatState()
    {
        switch (currentState)
        {
            case State::Closed: break;
            case State::Peek:   setState(State::Closed); break;
            case State::Half:   setState(State::Peek); break;
            case State::Full:   setState(State::Half); break;
        }
    }

    void close() { setState(State::Closed); }

    //-- Component overrides -----------------------------------------------------

    void resized() override
    {
        auto bounds = getLocalBounds();

        // Handle bar at the top of the drawer
        handleBar.setBounds(bounds.removeFromTop(handleBarHeight));

        // Content fills the rest
        contentViewport.setBounds(bounds);
    }

    void paint(juce::Graphics& g) override
    {
        // Drawer background
        g.setColour(juce::Colour(0xFFF8F6F3));  // Gallery shell white
        g.fillRoundedRectangle(getLocalBounds().toFloat(), 12.0f);

        // Handle indicator (pill shape at top center)
        auto handleBounds = getLocalBounds().toFloat();
        float pillW = 36.0f, pillH = 4.0f;
        float pillX = handleBounds.getCentreX() - pillW * 0.5f;
        float pillY = 8.0f;
        g.setColour(juce::Colour(0xFFDDDAD5));  // borderGray
        g.fillRoundedRectangle(pillX, pillY, pillW, pillH, 2.0f);
    }

    //-- Touch handling for drag-to-expand/collapse ------------------------------

    void mouseDown(const juce::MouseEvent& e) override
    {
        dragStartY = e.position.y;
        dragStartDrawerY = static_cast<float>(getY());
        isDragging = true;
    }

    void mouseDrag(const juce::MouseEvent& e) override
    {
        if (!isDragging)
            return;

        float deltaY = e.position.y - dragStartY;
        float newY = dragStartDrawerY + deltaY;

        // Clamp to valid range
        float parentH = static_cast<float>(getParentHeight());
        float minY = parentH * 0.1f;   // Full state: 90% visible
        float maxY = parentH - handleBarHeight;  // Closed state

        newY = juce::jlimit(minY, maxY, newY);
        setTopLeftPosition(getX(), static_cast<int>(newY));
    }

    void mouseUp(const juce::MouseEvent& e) override
    {
        if (!isDragging)
            return;

        isDragging = false;
        float velocity = e.position.y - dragStartY;

        // Determine target state based on current position and swipe velocity
        float parentH = static_cast<float>(getParentHeight());
        float currentRatio = 1.0f - (static_cast<float>(getY()) / parentH);

        // Velocity-based: fast swipe overrides position
        if (velocity < -80.0f)  // Fast swipe up
        {
            advanceState();
            return;
        }
        if (velocity > 80.0f)   // Fast swipe down
        {
            retreatState();
            return;
        }

        // Position-based snap
        if (currentRatio > 0.75f)
            setState(State::Full);
        else if (currentRatio > 0.4f)
            setState(State::Half);
        else if (currentRatio > 0.15f)
            setState(State::Peek);
        else
            setState(State::Closed);
    }

    //-- Content management ------------------------------------------------------

    // Set the component displayed in the drawer content area.
    // Typically a ParameterGrid or a tabbed engine parameter panel.
    void setContent(juce::Component* content)
    {
        contentViewport.setViewedComponent(content, false);
    }

    juce::Viewport& getViewport() { return contentViewport; }

private:
    static constexpr int handleBarHeight = 20;

    State currentState = State::Closed;
    State defaultState = State::Closed;
    State maxState = State::Full;

    juce::Component handleBar;
    juce::Viewport contentViewport;
    HapticEngine* haptics = nullptr;

    float dragStartY = 0.0f;
    float dragStartDrawerY = 0.0f;
    bool isDragging = false;

    float getTargetY(State state) const
    {
        float parentH = static_cast<float>(getParentHeight());
        switch (state)
        {
            case State::Closed: return parentH - handleBarHeight;
            case State::Peek:   return parentH - 100.0f;
            case State::Half:   return parentH * 0.5f;
            case State::Full:   return parentH * 0.1f;
        }
        return parentH - handleBarHeight;
    }

    void animateToState(State newState)
    {
        if (newState == currentState)
            return;

        currentState = newState;
        float targetY = getTargetY(newState);

        // Animate with JUCE Desktop::Animator
        auto& animator = juce::Desktop::getInstance().getAnimator();
        animator.animateComponent(this,
            getBounds().withY(static_cast<int>(targetY)),
            1.0f,  // alpha
            200,   // duration ms
            false, // use proxy
            0.0,   // start speed
            0.0);  // end speed

        if (haptics)
            haptics->fire(HapticEngine::Event::DrawerSnap);
    }
};

} // namespace xoceanus
