#pragma once
// XOverlap KnotMatrix — mathematical knot-topology FDN routing matrices.
//
// Each knot type encodes its crossing pattern as a 6×6 feedback routing matrix.
// All matrices are doubly stochastic (column sums = 1) for energy preservation.
//
// UNKNOT     — 6×6 identity: pure self-resonance, no inter-voice routing.
// TREFOIL    — Two independent 3-cycles (Z/3 symmetry): 0→1→2→0, 3→4→5→3.
// FIGURE-EIGHT — Symmetric "swap halves" (amphichiral: M[i][j]=M[j][i]):
//               voice i ↔ voice (5-i). Single crossing per pair.
// TORUS T(p,q) — Single 6-cycle with delay length ratios encoding p:q harmonic lock.
//               Delay ratios produce resonant frequencies in q:p ratio.
//
// interpolate() blends identity toward the knot matrix via tangle depth:
//   M_eff = (1 - depth) * I + depth * M_knot
//
// Column sums of interpolated matrix: (1-d)*1 + d*1 = 1. Stable for feedback < 1.

#include <array>
#include <cmath>
#include <algorithm>

namespace xoverlap {

struct KnotMatrix {
    using Matrix = std::array<std::array<float, 6>, 6>;

    //==========================================================================
    /// UNKNOT — identity matrix. Each voice feeds only itself.
    static Matrix unknot() noexcept
    {
        Matrix m{};
        for (int i = 0; i < 6; ++i)
            m[static_cast<size_t>(i)][static_cast<size_t>(i)] = 1.0f;
        return m;
    }

    //==========================================================================
    /// TREFOIL — two independent 3-cycles (Z/3 cyclic symmetry of the trefoil).
    /// Group A: voice 0 → 1 → 2 → 0
    /// Group B: voice 3 → 4 → 5 → 3
    static Matrix trefoil() noexcept
    {
        Matrix m{};
        // Group A: 0→1, 1→2, 2→0  (row=output, col=input)
        m[1][0] = 1.0f;
        m[2][1] = 1.0f;
        m[0][2] = 1.0f;
        // Group B: 3→4, 4→5, 5→3
        m[4][3] = 1.0f;
        m[5][4] = 1.0f;
        m[3][5] = 1.0f;
        return m;
    }

    //==========================================================================
    /// FIGURE-EIGHT — amphichiral: M[i][j] = M[j][i].
    /// Swap-pairs: voice i ↔ voice (5-i).
    ///   0 ↔ 5,  1 ↔ 4,  2 ↔ 3
    /// The single-crossing-per-pair structure encodes the figure-eight's
    /// amphicheiral (self-mirror) topology.
    static Matrix figureEight() noexcept
    {
        Matrix m{};
        // Each voice routes to its mirror across the center
        for (int i = 0; i < 6; ++i)
            m[5 - i][i] = 1.0f;  // voice i feeds voice (5-i)
        return m;
    }

    //==========================================================================
    /// TORUS T(p,q) — single 6-cycle: 0→1→2→3→4→5→0.
    /// The routing matrix is the same for all torus knots; the timbral
    /// distinction is entirely in torusRatios() below, which encodes p:q
    /// harmonic locking via delay length ratios.
    static Matrix torus(int /*p*/, int /*q*/) noexcept
    {
        Matrix m{};
        // Single 6-cycle: voice i feeds voice (i+1) mod 6
        for (int i = 0; i < 6; ++i)
            m[(i + 1) % 6][i] = 1.0f;
        return m;
    }

    //==========================================================================
    /// Interpolate from identity toward the knot matrix.
    ///   M_eff = (1 - depth) * I + depth * M_knot
    /// At depth=0: pure self-resonance (identity). At depth=1: full knot routing.
    static Matrix interpolate(const Matrix& knotMat, float depth) noexcept
    {
        depth = std::clamp(depth, 0.0f, 1.0f);
        float d  = depth;
        float id = 1.0f - depth;

        Matrix result{};
        for (int row = 0; row < 6; ++row)
        {
            for (int col = 0; col < 6; ++col)
            {
                float identVal = (row == col) ? 1.0f : 0.0f;
                result[static_cast<size_t>(row)][static_cast<size_t>(col)] =
                    id * identVal + d * knotMat[static_cast<size_t>(row)][static_cast<size_t>(col)];
            }
        }
        return result;
    }

    //==========================================================================
    /// Delay length ratios for Torus T(p,q) harmonic locking.
    ///
    /// Returns 6 ratios such that resonant frequencies are in ratio q:p
    /// (alternating between the two torus wrap directions):
    ///   ratio[even] = float(p) / float(p+q)   →  frequency ∝ (p+q)/p
    ///   ratio[odd]  = float(q) / float(p+q)   →  frequency ∝ (p+q)/q
    ///   frequency ratio = [(p+q)/p] / [(p+q)/q] = q/p = q:p
    ///
    /// Example: T(3,2) → ratios = {0.6, 0.4, 0.6, 0.4, 0.6, 0.4}
    ///   Frequencies in ratio (p+q)/p : (p+q)/q = 5/3 : 5/2 = 10:15 = 2:3 ✓
    static std::array<float, 6> torusRatios(int p, int q) noexcept
    {
        // Guard against degenerate inputs
        int pp = std::max(2, std::min(p, 7));
        int qq = std::max(2, std::min(q, 7));
        if (pp == qq) qq = pp + 1; // ensure distinct ratios

        float pq     = static_cast<float>(pp + qq);
        float rEven  = static_cast<float>(pp) / pq;  // longer delay → lower frequency
        float rOdd   = static_cast<float>(qq) / pq;  // shorter delay → higher frequency

        return { rEven, rOdd, rEven, rOdd, rEven, rOdd };
    }
};

} // namespace xoverlap
