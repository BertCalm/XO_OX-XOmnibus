// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include <atomic>

namespace xoceanus
{

//==============================================================================
// MoodModulationBus — STUB
//
// Phase 0 scaffolds this header to lock the file path and namespace. Concrete
// API is deferred until Pack 8 (Mastering) becomes the first consumer; the
// mastering bus will read the full 16-mood blend vector to shape compression,
// EQ, and limiter behaviour per preset mood.
//
// Decision D3 (locked 2026-04-27): mood-aware mastering defaults OFF; users
// opt in via a single MASTER_MOOD_AWARE toggle. Mood-Aware preset bank ships
// with the toggle ON inside those presets only.
//
// Mood enum mirrors CLAUDE.md's 16 mood categories. Order is significant —
// preset metadata stores the index.
//
class MoodModulationBus
{
public:
    enum class Mood : int
    {
        Foundation = 0,
        Atmosphere,
        Entangled,
        Prism,
        Flux,
        Aether,
        Family,
        Submerged,
        Coupling,
        Crystalline,
        Deep,
        Ethereal,
        Kinetic,
        Luminous,
        Organic,
        Shadow,
        NumMoods = 16
    };

    MoodModulationBus() noexcept = default;

    //-- Stub API --------------------------------------------------------------

    // Set the current preset's mood (one-hot blend).
    // Pack 8 will extend this to support soft transitions between moods.
    void setMood(Mood m) noexcept
    {
        currentMood_.store(static_cast<int>(m), std::memory_order_release);
    }

    Mood getMood() const noexcept
    {
        return static_cast<Mood>(currentMood_.load(std::memory_order_acquire));
    }

    // Returns 1.0 for the active mood, 0.0 for all others.
    // Pack 8 will replace this with a soft-blend implementation.
    float getMoodWeight(Mood m) const noexcept
    {
        return (getMood() == m) ? 1.0f : 0.0f;
    }

private:
    std::atomic<int> currentMood_ { 0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MoodModulationBus)
};

} // namespace xoceanus
