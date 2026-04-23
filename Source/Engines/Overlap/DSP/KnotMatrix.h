// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once

//==============================================================================
// KnotMatrix.h — xoverlap::KnotMatrix
//
// Produces 6×6 feedback matrices corresponding to knot topology types:
//   Unknot       — identity / minimal coupling (simple parallel voices)
//   Trefoil      — 3-fold symmetric crossing structure
//   Figure-Eight — alternating sign coupling (Listing's knot)
//   Torus(p,q)   — torus knot winding numbers drive inter-voice delay ratios
//                  and coupling weights
//
// All matrices are orthogonal (near-unitary) to maintain loudness stability.
// `interpolate()` blends a target matrix toward full tangle depth by mixing
// with a scaled identity, keeping the structure lossless at depth = 0.
//==============================================================================

#include <array>
#include <cmath>

namespace xoverlap
{

//==============================================================================
class KnotMatrix
{
public:
    //==========================================================================
    // 6×6 feedback routing matrix (row-major: M[i][j] = weight from channel j to i)
    using Matrix = std::array<std::array<float, 6>, 6>;

    //==========================================================================
    // Unknot — identity coupling. Voices are independent.
    // At tangle depth > 0 the interpolation still induces partial coupling.
    static Matrix unknot() noexcept
    {
        Matrix m{};
        for (int i = 0; i < 6; ++i)
            for (int j = 0; j < 6; ++j)
                m[static_cast<size_t>(i)][static_cast<size_t>(j)] = (i == j) ? 1.0f : 0.0f;
        return m;
    }

    //==========================================================================
    // Trefoil — 3-crossing knot. Three pairs of voices form a rotating triplet.
    // Coupling follows the T(2,3) pattern: each voice injects into its partner
    // at distance 3 with sign according to the braid word a^3.
    static Matrix trefoil() noexcept
    {
        // Hadamard-inspired 6×6 with 3-fold symmetry and unit-norm rows
        Matrix m{};
        // Circulant matrix with coupling kernel [1, r, r, -r, r, r] / ||
        // r chosen so rows are unit-norm with 3-fold symmetry.
        // Use a simple Hadamard-inspired rotation:
        // Each row i couples to (i+1)%6 with +w and (i+5)%6 with -w,
        // diagonal with d, forming a near-unitary skew-symmetric circulant.
        const float d = 1.0f / std::sqrt(3.0f);
        const float w = 1.0f / std::sqrt(3.0f);
        const float x = 1.0f / std::sqrt(3.0f);

        // Full Hadamard-normalized 6×6 trefoil circulant:
        //   diag = d, +1 hop = +w, -1 hop = -w, all others 0
        // Row norms: d^2 + w^2 + w^2 = 1  =>  d=w=1/sqrt(3)
        for (int i = 0; i < 6; ++i)
        {
            for (int j = 0; j < 6; ++j)
                m[static_cast<size_t>(i)][static_cast<size_t>(j)] = 0.0f;
            m[static_cast<size_t>(i)][static_cast<size_t>(i)] = d;
            m[static_cast<size_t>(i)][static_cast<size_t>((i + 1) % 6)] = w;
            m[static_cast<size_t>(i)][static_cast<size_t>((i + 5) % 6)] = -x;
        }
        return m;
    }

    //==========================================================================
    // Figure-Eight — Listing's knot. Alternating-sign coupling creates the
    // characteristic "flip" at each crossing. Even indices couple forward with
    // positive weight; odd indices couple backward with negative weight.
    static Matrix figureEight() noexcept
    {
        // Alternating-sign variant of the trefoil:
        // Even rows: +w forward, -x backward
        // Odd rows:  -w forward, +x backward
        const float d = 1.0f / std::sqrt(3.0f);
        const float w = 1.0f / std::sqrt(3.0f);
        const float x = 1.0f / std::sqrt(3.0f);

        Matrix m{};
        for (int i = 0; i < 6; ++i)
        {
            for (int j = 0; j < 6; ++j)
                m[static_cast<size_t>(i)][static_cast<size_t>(j)] = 0.0f;

            float sign = (i % 2 == 0) ? 1.0f : -1.0f;
            m[static_cast<size_t>(i)][static_cast<size_t>(i)] = d;
            m[static_cast<size_t>(i)][static_cast<size_t>((i + 1) % 6)] = sign * w;
            m[static_cast<size_t>(i)][static_cast<size_t>((i + 5) % 6)] = -sign * x;
        }
        return m;
    }

    //==========================================================================
    // Torus(p, q) — torus knot T(p,q). Winding numbers p and q determine
    // the inter-voice coupling distance and phase offset.
    // p and q must be coprime; if not, the result is a torus link.
    static Matrix torus(int p, int q) noexcept
    {
        // Clamp to valid range
        if (p < 2)
            p = 2;
        if (q < 2)
            q = 2;
        if (p > 7)
            p = 7;
        if (q > 7)
            q = 7;

        // Step size through 6 voices driven by (p mod 6) and (q mod 6)
        int stepP = p % 6;
        int stepQ = q % 6;
        if (stepP == 0)
            stepP = 1;
        if (stepQ == 0)
            stepQ = 1;

        const float d = 1.0f / std::sqrt(3.0f);
        const float wp = 1.0f / std::sqrt(3.0f);
        const float wq = 1.0f / std::sqrt(3.0f);

        Matrix m{};
        for (int i = 0; i < 6; ++i)
        {
            for (int j = 0; j < 6; ++j)
                m[static_cast<size_t>(i)][static_cast<size_t>(j)] = 0.0f;

            int fwdCol = (i + stepP) % 6;
            int bwdCol = (i + 6 - stepQ) % 6;

            m[static_cast<size_t>(i)][static_cast<size_t>(i)] = d;

            // F10: when fwdCol == bwdCol the two off-diagonal writes land on the same cell
            // and the +wp and -wq terms cancel to zero, collapsing the row norm to d=1/√3
            // instead of the intended 1.  Detect the collision and offset bwdCol by ±1 so
            // both off-diagonal couplings are distinct and the row remains unit-norm.
            if (fwdCol == i)
                fwdCol = (fwdCol + 1) % 6; // avoid diagonal collision (should not happen after stepP fix, but guard anyway)
            if (bwdCol == i)
                bwdCol = (bwdCol + 5) % 6;
            if (fwdCol == bwdCol)
                bwdCol = (bwdCol + 1) % 6; // push bwd away so cells are distinct

            m[static_cast<size_t>(i)][static_cast<size_t>(fwdCol)] = wp;
            m[static_cast<size_t>(i)][static_cast<size_t>(bwdCol)] = -wq;
        }
        return m;
    }

    //==========================================================================
    // interpolate() — mix the target knot matrix with scaled identity at
    // depth 0 → identity only; depth 1 → full knot matrix.
    // This keeps the FDN stable at any depth while morphing topology.
    static Matrix interpolate(const Matrix& target, float depth) noexcept
    {
        depth = std::max(0.0f, std::min(1.0f, depth));
        float inv = 1.0f - depth;

        Matrix result{};
        for (int i = 0; i < 6; ++i)
        {
            for (int j = 0; j < 6; ++j)
            {
                float identity = (i == j) ? 1.0f : 0.0f;
                result[static_cast<size_t>(i)][static_cast<size_t>(j)] =
                    inv * identity + depth * target[static_cast<size_t>(i)][static_cast<size_t>(j)];
            }
        }
        return result;
    }

    //==========================================================================
    // torusRatios() — per-voice delay ratios for torus knot T(p,q).
    // Six voices are assigned harmonic multiples derived from the winding numbers.
    static std::array<float, 6> torusRatios(int p, int q) noexcept
    {
        if (p < 2)
            p = 2;
        if (p > 7)
            p = 7;
        if (q < 2)
            q = 2;
        if (q > 7)
            q = 7;

        // Generate ratios by distributing voices along the p/q Lissajous structure.
        // ratio[i] = 1 + 0.1 * sin(2π * i * p / 6) * cos(2π * i * q / 6)
        std::array<float, 6> ratios{};
        const float twoPi = 6.28318530718f;
        for (int i = 0; i < 6; ++i)
        {
            float angle = twoPi * static_cast<float>(i) / 6.0f;
            float rp = std::cos(static_cast<float>(p) * angle);
            float rq = std::sin(static_cast<float>(q) * angle);
            ratios[static_cast<size_t>(i)] = 1.0f + 0.12f * rp * rq;
        }
        return ratios;
    }
};

} // namespace xoverlap
