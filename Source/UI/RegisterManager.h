// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

namespace xoceanus
{

//==============================================================================
// RegisterManager — D4: 3-Register Auto-Switching with Lock
//
// Three visual modes (Gallery, Performance, Coupling) that switch automatically
// based on context with a manual lock toggle.
//
// Register definitions:
//   Gallery     (idle)    : light cockpit opacity 0.8-1.0, engine browsing mode
//   Performance (playing) : dark cockpit per B041, opacity driven by note activity
//   Coupling    (routing) : gold tint overlay when coupling inspector is visible
//
// Detection priority (highest to lowest):
//   1. Locked       → stay in current register
//   2. Coupling tab → Coupling register
//   3. Note activity → Performance register
//   4. Otherwise    → Gallery register
//
// Transitions use 400ms ease-in-out (smoothstep) with a 300ms debounce to
// prevent flickering when signals change rapidly.
//
// Lock state is per-instance and persists across DAW session save/reload.
//==============================================================================

class RegisterManager
{
public:
    enum class Register
    {
        Gallery,
        Performance,
        Coupling
    };

    //==========================================================================
    // update() is called from timerCallback on every animation tick.
    //   hasNoteActivity : true when note activity level exceeds a threshold
    //   couplingVisible : true when the coupling inspector tab is active
    //   dtMs            : elapsed time since last call, in milliseconds
    void update (bool hasNoteActivity, bool couplingVisible, float dtMs)
    {
        // If locked, never change the target register.
        if (locked_)
        {
            // Still advance the transition so the lock can be applied while
            // a transition was already in progress.
            advanceTransition (dtMs);
            return;
        }

        // Determine what the register *should* be right now.
        Register desired = Register::Gallery;
        if (couplingVisible)
            desired = Register::Coupling;
        else if (hasNoteActivity)
            desired = Register::Performance;

        if (desired != pendingTarget_)
        {
            // New desired state — start (or restart) the debounce timer.
            pendingTarget_  = desired;
            debounceTimer_ = 0.0f;
        }
        else
        {
            // Advance the debounce timer; once it fires, commit the target.
            debounceTimer_ += dtMs;
            if (debounceTimer_ >= kDebounceMs && target_ != desired)
            {
                target_     = desired;
                transition_ = 0.0f; // restart the transition from the beginning
            }
        }

        advanceTransition (dtMs);
    }

    //==========================================================================
    // Smoothstep-interpolated progress: 0.0 = fully in previous register,
    // 1.0 = fully settled in the current target.
    float transitionProgress() const noexcept
    {
        float t = transition_;
        return t * t * (3.0f - 2.0f * t); // smoothstep
    }

    //==========================================================================
    Register current()  const noexcept { return current_; }
    Register target()   const noexcept { return target_; }
    bool     isLocked() const noexcept { return locked_; }

    void toggleLock() noexcept { locked_ = !locked_; }
    void setLocked (bool v) noexcept { locked_ = v; }

    //==========================================================================
    // Persistence — store/restore lock state and the locked register.
    // The caller owns the ValueTree; children are named "RegisterManager".
    void saveState (juce::ValueTree& parentState) const
    {
        auto child = parentState.getOrCreateChildWithName ("RegisterManager", nullptr);
        child.setProperty ("locked",          locked_,                      nullptr);
        child.setProperty ("currentRegister", static_cast<int> (current_), nullptr);
    }

    void restoreState (const juce::ValueTree& parentState)
    {
        auto child = parentState.getChildWithName ("RegisterManager");
        if (!child.isValid())
            return;

        locked_ = static_cast<bool> (static_cast<int> (child ["locked"]));

        int r = static_cast<int> (child ["currentRegister"]);
        if (r >= 0 && r <= 2)
        {
            current_    = static_cast<Register> (r);
            target_     = current_;
            transition_ = 1.0f; // already settled — no animation on restore
        }
    }

    //==========================================================================
    // Persist/restore via XML attributes on an existing XmlElement.
    // Used by the processor's getStateInformation / setStateInformation.
    void saveToXml (juce::XmlElement& xml) const
    {
        xml.setAttribute ("registerLocked",  locked_ ? 1 : 0);
        xml.setAttribute ("registerCurrent", static_cast<int> (current_));
    }

    void restoreFromXml (const juce::XmlElement& xml)
    {
        locked_ = xml.getIntAttribute ("registerLocked", 0) != 0;
        int r   = xml.getIntAttribute ("registerCurrent", 0);
        if (r >= 0 && r <= 2)
        {
            current_    = static_cast<Register> (r);
            target_     = current_;
            transition_ = 1.0f;
        }
    }

    // Convenience overload — called by the editor constructor with pre-fetched
    // values from the processor's persisted state.
    void restoreFromXmlValues (bool locked, int registerIndex)
    {
        locked_ = locked;
        int r   = registerIndex;
        if (r >= 0 && r <= 2)
        {
            current_         = static_cast<Register> (r);
            target_          = current_;
            pendingTarget_   = current_;
            transition_      = 1.0f;
            debounceTimer_   = 0.0f;
        }
    }

private:
    //==========================================================================
    void advanceTransition (float dtMs) noexcept
    {
        if (transition_ < 1.0f)
        {
            transition_ += dtMs / kTransitionMs;
            if (transition_ >= 1.0f)
            {
                transition_ = 1.0f;
                current_    = target_; // commit once fully settled
            }
        }
    }

    //==========================================================================
    Register current_      = Register::Gallery;
    Register target_       = Register::Gallery;
    Register pendingTarget_ = Register::Gallery;
    bool     locked_       = false;
    float    transition_   = 1.0f;   // 1.0 = stable (no animation in progress)
    float    debounceTimer_ = 0.0f;  // ms since pending target was set

    static constexpr float kDebounceMs   = 300.0f;
    static constexpr float kTransitionMs = 400.0f;
};

} // namespace xoceanus
