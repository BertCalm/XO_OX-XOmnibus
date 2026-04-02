// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include <cmath>
#include <algorithm>

/// OxytocinTriangle — Sternberg barycentric love triangle.
///
/// Maintains I/P/C coordinates (always sum to 1.0 in normalised space).
/// M1 (CHARACTER macro) traces a canonical path through eight love types.
/// Love type detection returns the current state as an integer 0-8.

enum class LoveType : int
{
    NonLove       = 0,
    Liking        = 1,
    Infatuation   = 2,
    EmptyLove     = 3,
    Romantic      = 4,
    Companionate  = 5,
    Fatuous       = 6,
    Consummate    = 7,
    Obsession     = 8
};

struct LoveCoords { float I, P, C; };

class OxytocinTriangle
{
public:
    OxytocinTriangle() { current = { 0.33f, 0.33f, 0.34f }; }

    /// Compute barycentric coordinates for a normalised CHARACTER position [0..1].
    static LoveCoords fromCharacterPosition (float t) noexcept
    {
        t = std::clamp (t, 0.0f, 1.0f);

        // Canonical path keyframes
        struct KF { float t, I, P, C; };
        constexpr KF kf[] = {
            { 0.00f, 0.33f, 0.33f, 0.34f },   // Non-Love (balanced, low energy)
            { 0.15f, 0.10f, 0.80f, 0.10f },   // Infatuation
            { 0.30f, 0.45f, 0.45f, 0.10f },   // Romantic
            { 0.50f, 0.33f, 0.33f, 0.34f },   // Consummate (balanced, full energy)
            { 0.70f, 0.45f, 0.10f, 0.45f },   // Companionate
            { 0.85f, 0.10f, 0.10f, 0.80f },   // Empty Love
            { 1.00f, 0.05f, 0.05f, 0.90f }    // Obsession
        };
        constexpr int N = 7;

        // Find enclosing segment and lerp
        for (int i = 0; i < N - 1; ++i)
        {
            if (t <= kf[i + 1].t)
            {
                float span = kf[i + 1].t - kf[i].t;
                float frac = (span > 0.0f) ? (t - kf[i].t) / span : 0.0f;
                return {
                    kf[i].I + frac * (kf[i + 1].I - kf[i].I),
                    kf[i].P + frac * (kf[i + 1].P - kf[i].P),
                    kf[i].C + frac * (kf[i + 1].C - kf[i].C)
                };
            }
        }
        return { kf[N - 1].I, kf[N - 1].P, kf[N - 1].C };
    }

    /// Detect love type from I/P/C values.
    static LoveType detectType (float I, float P, float C) noexcept
    {
        // Obsession first (extreme C dominance)
        if (C > 0.85f && I < 0.2f && P < 0.2f) return LoveType::Obsession;
        if (I < 0.3f && P < 0.3f && C < 0.3f)  return LoveType::NonLove;
        if (I > 0.3f && P > 0.3f && C > 0.3f)  return LoveType::Consummate;
        if (I > 0.5f && P < 0.3f && C < 0.3f)  return LoveType::Liking;
        if (P > 0.5f && I < 0.3f && C < 0.3f)  return LoveType::Infatuation;
        if (C > 0.5f && I < 0.3f && P < 0.3f)  return LoveType::EmptyLove;
        if (I > 0.4f && P > 0.4f && C < 0.3f)  return LoveType::Romantic;
        if (I > 0.4f && C > 0.4f && P < 0.3f)  return LoveType::Companionate;
        if (P > 0.4f && C > 0.4f && I < 0.3f)  return LoveType::Fatuous;
        return LoveType::NonLove;
    }

    /// Return forced I/P/C for a given lock type (1-8).
    static LoveCoords coordsForLockType (int lockType) noexcept
    {
        switch (lockType)
        {
            case 1: return { 0.70f, 0.15f, 0.15f };   // Liking
            case 2: return { 0.10f, 0.80f, 0.10f };   // Infatuation
            case 3: return { 0.10f, 0.10f, 0.80f };   // Empty Love
            case 4: return { 0.45f, 0.45f, 0.10f };   // Romantic
            case 5: return { 0.45f, 0.10f, 0.45f };   // Companionate
            case 6: return { 0.10f, 0.45f, 0.45f };   // Fatuous
            case 7: return { 0.34f, 0.33f, 0.33f };   // Consummate
            case 8: return { 0.05f, 0.05f, 0.90f };   // Obsession
            default: return { 0.33f, 0.33f, 0.34f };
        }
    }

    // Current coordinates (updated each block by voice/engine)
    LoveCoords current;
};
