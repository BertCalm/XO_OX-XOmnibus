// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include "MasterFXSequencer.h"
#include <juce_core/juce_core.h>
#include <array>
#include <vector>
#include <cstdint>

namespace xoceanus {

//==============================================================================
// PageRSequencer — Fairlight CMI Page R-inspired visual step sequencer.
//
// The Fairlight CMI (1979) introduced "Page R" — the first visual step
// sequencer for manipulating audio parameters in a grid. Each row was a
// parameter, each column was a time step, and you drew values with a
// light pen. Revolutionary for its time, it let composers SEE rhythm
// as a visual pattern.
//
// PageRSequencer extends the MasterFXSequencer with per-step custom values,
// visual editing capabilities, and multi-parameter lane support. Instead of
// algorithmic patterns only, users can draw arbitrary modulation shapes.
//
// Features:
//   - Up to 4 parameter lanes (simultaneous modulation targets)
//   - Per-step value editing (0-1 range per step per lane)
//   - Algorithmic pattern presets (can be drawn over / edited)
//   - Step muting (individual steps can be silenced)
//   - Pattern length independent per lane
//   - Copy/paste between lanes
//   - Random fill with musical weighting
//   - Morphing between two patterns over time
//
// Integration: This extends MasterFXSequencer — uses the same clock system
// but adds user-drawn step data and multi-lane capability.
//
// Inspired by: Fairlight CMI Page R (1979), Elektron parameter locks,
//              Ableton step sequencer, Make Noise René
//==============================================================================
class PageRSequencer
{
public:
    static constexpr int kMaxSteps = 16;
    static constexpr int kMaxLanes = 4;

    struct Lane
    {
        MasterFXSequencer::Target target = MasterFXSequencer::Target::None;
        std::array<float, kMaxSteps> values {};        // 0-1 per step
        std::array<bool, kMaxSteps> muted {};           // Per-step mute
        int length = 8;                                  // Steps for this lane (1-16)
        float smoothing = 0.0f;                          // Inter-step glide (0=step, 1=smooth)
        bool enabled = false;
    };

    struct Snapshot
    {
        std::array<Lane, kMaxLanes> lanes;
        juce::String name;
    };

    PageRSequencer() = default;

    void prepare (double sampleRate)
    {
        sr = sampleRate;
    }

    //--------------------------------------------------------------------------
    // Lane Configuration

    Lane& getLane (int index)
    {
        return lanes[static_cast<size_t> (std::clamp (index, 0, kMaxLanes - 1))];
    }

    const Lane& getLane (int index) const
    {
        return lanes[static_cast<size_t> (std::clamp (index, 0, kMaxLanes - 1))];
    }

    void setLaneTarget (int laneIdx, MasterFXSequencer::Target target)
    {
        if (laneIdx >= 0 && laneIdx < kMaxLanes)
            lanes[static_cast<size_t> (laneIdx)].target = target;
    }

    void setLaneEnabled (int laneIdx, bool enabled)
    {
        if (laneIdx >= 0 && laneIdx < kMaxLanes)
            lanes[static_cast<size_t> (laneIdx)].enabled = enabled;
    }

    void setLaneLength (int laneIdx, int steps)
    {
        if (laneIdx >= 0 && laneIdx < kMaxLanes)
            lanes[static_cast<size_t> (laneIdx)].length = std::clamp (steps, 1, kMaxSteps);
    }

    void setLaneSmoothing (int laneIdx, float smooth)
    {
        if (laneIdx >= 0 && laneIdx < kMaxLanes)
            lanes[static_cast<size_t> (laneIdx)].smoothing = std::clamp (smooth, 0.0f, 1.0f);
    }

    //--------------------------------------------------------------------------
    // Step Editing

    void setStepValue (int laneIdx, int step, float value)
    {
        if (laneIdx >= 0 && laneIdx < kMaxLanes && step >= 0 && step < kMaxSteps)
            lanes[static_cast<size_t> (laneIdx)].values[static_cast<size_t> (step)] =
                std::clamp (value, 0.0f, 1.0f);
    }

    void setStepMuted (int laneIdx, int step, bool muted)
    {
        if (laneIdx >= 0 && laneIdx < kMaxLanes && step >= 0 && step < kMaxSteps)
            lanes[static_cast<size_t> (laneIdx)].muted[static_cast<size_t> (step)] = muted;
    }

    float getStepValue (int laneIdx, int step) const
    {
        if (laneIdx >= 0 && laneIdx < kMaxLanes && step >= 0 && step < kMaxSteps)
            return lanes[static_cast<size_t> (laneIdx)].values[static_cast<size_t> (step)];
        return 0.0f;
    }

    //--------------------------------------------------------------------------
    // Pattern Operations

    /// Fill a lane with an algorithmic pattern
    void fillPattern (int laneIdx, MasterFXSequencer::Pattern pattern)
    {
        if (laneIdx < 0 || laneIdx >= kMaxLanes) return;
        auto& lane = lanes[static_cast<size_t> (laneIdx)];

        for (int s = 0; s < lane.length; ++s)
        {
            float pos = static_cast<float> (s) / static_cast<float> (lane.length);

            switch (pattern)
            {
                case MasterFXSequencer::Pattern::Pulse:
                    lane.values[static_cast<size_t> (s)] = (s % 2 == 0) ? 1.0f : 0.0f;
                    break;
                case MasterFXSequencer::Pattern::RampUp:
                    lane.values[static_cast<size_t> (s)] = pos;
                    break;
                case MasterFXSequencer::Pattern::RampDown:
                    lane.values[static_cast<size_t> (s)] = 1.0f - pos;
                    break;
                case MasterFXSequencer::Pattern::Triangle:
                {
                    float t = pos * 2.0f;
                    lane.values[static_cast<size_t> (s)] = (t <= 1.0f) ? t : (2.0f - t);
                    break;
                }
                default:
                    lane.values[static_cast<size_t> (s)] = 0.5f;
                    break;
            }

            lane.muted[static_cast<size_t> (s)] = false;
        }
    }

    /// Randomize a lane with musical weighting
    void randomizeLane (int laneIdx, float density = 0.5f)
    {
        if (laneIdx < 0 || laneIdx >= kMaxLanes) return;
        auto& lane = lanes[static_cast<size_t> (laneIdx)];

        for (int s = 0; s < lane.length; ++s)
        {
            float r = nextRandom();
            // Musical weighting: downbeats stronger, offbeats quieter
            float beatWeight = (s % 4 == 0) ? 1.0f : (s % 2 == 0) ? 0.7f : 0.4f;
            lane.values[static_cast<size_t> (s)] = r * beatWeight;
            lane.muted[static_cast<size_t> (s)] = (nextRandom() > density);
        }
    }

    /// Copy one lane to another
    void copyLane (int srcLane, int dstLane)
    {
        if (srcLane >= 0 && srcLane < kMaxLanes && dstLane >= 0 && dstLane < kMaxLanes)
        {
            auto target = lanes[static_cast<size_t> (dstLane)].target;
            lanes[static_cast<size_t> (dstLane)] = lanes[static_cast<size_t> (srcLane)];
            lanes[static_cast<size_t> (dstLane)].target = target;
        }
    }

    /// Clear all steps in a lane
    void clearLane (int laneIdx)
    {
        if (laneIdx >= 0 && laneIdx < kMaxLanes)
        {
            auto& lane = lanes[static_cast<size_t> (laneIdx)];
            lane.values.fill (0.0f);
            lane.muted.fill (false);
        }
    }

    //--------------------------------------------------------------------------
    // Playback — called per block to get current modulation values

    struct ModOutput
    {
        MasterFXSequencer::Target target;
        float value;
    };

    /// Update step position and compute modulation values.
    /// Call once per block. Uses the same step from the parent MasterFXSequencer.
    std::array<ModOutput, kMaxLanes> getModValues (int currentStep) const
    {
        std::array<ModOutput, kMaxLanes> output;

        for (int l = 0; l < kMaxLanes; ++l)
        {
            const auto& lane = lanes[static_cast<size_t> (l)];
            output[static_cast<size_t> (l)].target = lane.target;

            if (!lane.enabled || lane.target == MasterFXSequencer::Target::None)
            {
                output[static_cast<size_t> (l)].value = 0.0f;
                continue;
            }

            int step = currentStep % lane.length;

            if (lane.muted[static_cast<size_t> (step)])
            {
                output[static_cast<size_t> (l)].value = 0.0f;
                continue;
            }

            float value = lane.values[static_cast<size_t> (step)];

            // Smoothing: interpolate toward next step
            if (lane.smoothing > 0.01f)
            {
                int nextStep = (step + 1) % lane.length;
                float nextVal = lane.muted[static_cast<size_t> (nextStep)]
                              ? 0.0f
                              : lane.values[static_cast<size_t> (nextStep)];
                value = value + lane.smoothing * 0.3f * (nextVal - value);
            }

            output[static_cast<size_t> (l)].value = value;
        }

        return output;
    }

    //--------------------------------------------------------------------------
    // Snapshot save/restore (for pattern presets)

    Snapshot takeSnapshot (const juce::String& name = "") const
    {
        Snapshot snap;
        snap.lanes = lanes;
        snap.name = name;
        return snap;
    }

    void loadSnapshot (const Snapshot& snap)
    {
        lanes = snap.lanes;
    }

    //--------------------------------------------------------------------------
    // Serialization (for .xorecipe integration)

    juce::var toJSON() const
    {
        juce::Array<juce::var> lanesArr;
        for (int l = 0; l < kMaxLanes; ++l)
        {
            const auto& lane = lanes[static_cast<size_t> (l)];
            auto* laneObj = new juce::DynamicObject();
            laneObj->setProperty ("target", static_cast<int> (lane.target));
            laneObj->setProperty ("length", lane.length);
            laneObj->setProperty ("smoothing", lane.smoothing);
            laneObj->setProperty ("enabled", lane.enabled);

            juce::Array<juce::var> vals, mutes;
            for (int s = 0; s < lane.length; ++s)
            {
                vals.add (lane.values[static_cast<size_t> (s)]);
                mutes.add (lane.muted[static_cast<size_t> (s)]);
            }
            laneObj->setProperty ("values", vals);
            laneObj->setProperty ("muted", mutes);
            lanesArr.add (juce::var (laneObj));
        }
        return lanesArr;
    }

    void fromJSON (const juce::var& json)
    {
        if (auto* arr = json.getArray())
        {
            for (int l = 0; l < std::min (kMaxLanes, arr->size()); ++l)
            {
                if (auto* lObj = (*arr)[l].getDynamicObject())
                {
                    auto& lane = lanes[static_cast<size_t> (l)];
                    lane.target = static_cast<MasterFXSequencer::Target> (
                        static_cast<int> (lObj->getProperty ("target")));
                    lane.length = static_cast<int> (lObj->getProperty ("length"));
                    lane.smoothing = static_cast<float> (lObj->getProperty ("smoothing"));
                    lane.enabled = static_cast<bool> (lObj->getProperty ("enabled"));

                    if (auto* vals = lObj->getProperty ("values").getArray())
                        for (int s = 0; s < std::min (kMaxSteps, vals->size()); ++s)
                            lane.values[static_cast<size_t> (s)] = static_cast<float> ((*vals)[s]);

                    if (auto* mutes = lObj->getProperty ("muted").getArray())
                        for (int s = 0; s < std::min (kMaxSteps, mutes->size()); ++s)
                            lane.muted[static_cast<size_t> (s)] = static_cast<bool> ((*mutes)[s]);
                }
            }
        }
    }

    void reset()
    {
        for (auto& lane : lanes)
        {
            lane.values.fill (0.0f);
            lane.muted.fill (false);
        }
    }

private:
    double sr = 44100.0;
    std::array<Lane, kMaxLanes> lanes;
    uint32_t randState = 0xCAFEBABE;

    float nextRandom()
    {
        randState ^= randState << 13;
        randState ^= randState >> 17;
        randState ^= randState << 5;
        return static_cast<float> (randState & 0x7FFFFF) / static_cast<float> (0x7FFFFF);
    }
};

} // namespace xoceanus
