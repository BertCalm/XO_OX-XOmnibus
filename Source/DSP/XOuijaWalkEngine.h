// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once

/*
    XOuijaWalkEngine.h
    ==================
    Tempo-synced autonomous planchette walk engine for the XOuija 8×8 cell grid.
    Wave 5, D1 / issue #1287.  Decision source: D8 B4 (FULLY LOCKED 2026-04-25).

    Ownership + threading:
      - XOuijaWalkEngine is owned by XOceanusProcessor (audio-thread side).
      - prepareToPlay() and processBlock() are called exclusively on the audio thread.
      - UI thread interacts via:
          1. setCalmWild() / setConsonantDissonant() / setTendencyCol/Row()
             — std::atomic<float> writes, safe from any thread.
          2. enqueueEdit()
             — writes through a juce::AbstractFifo + fixed array (SPSC, lock-free).
          3. getSnapshot()
             — reads atomic planchette position + heatmap atomics (lock-free).
      - No heap allocation on the audio thread after prepareToPlay().

    Movement model (D8-B4):
      Each walk step the engine:
        1. Reads the BPM and PPQ from the host to compute a sample-accurate step
           boundary (default = one bar / 4 beats).
        2. Drains pending cell edits from the SPSC queue (UI thread → audio thread).
        3. Selects the next cell using:
               a. calm_wild  [0, 1]:  0 = adjacent only (Chebyshev ≤ 1),
                                      1 = can leap to any cell.
               b. tendency   [-1, 1] per axis: biases direction.
               c. consonant_dissonant [0, 1]:  0 = prefer low-tension chords,
                                               1 = prefer dissonant chords.
               d. Pinned cell exclusion.
               e. Heatmap cooling: recently visited cells have reduced weight
                  (exponential decay at ~0.01/sec, preventing local oscillation).
        4. Advances the planchette, updates the atomic heatmap, fires onCellChanged
           on the AUDIO thread for the output router to consume inline.

    Planchette output:
      XOuijaOutputRouter reads currentCell() once per processBlock() advance to
      dispatch chord / rhythm / texture to the D6/D7/D9 systems.

    Backward compatibility (CC 85/86):
      circleX() and influenceY() return the current planchette column/row as a
      normalized [0, 1] value so existing CC automations keep working.

    Spec: Docs/specs/wave5-d1-xouija-multilayer-cells.md section 4.
    JUCE 8, C++17.
*/

#include "XOuijaCell.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>
#include <array>
#include <functional>
#include <random>
#include <cmath>
#include <algorithm>
#include <cstdint>

namespace xoceanus {

//==============================================================================
// ChordTension — static lookup: lower value = more consonant.
// Used by the consonant_dissonant selection weight.
//
// Values are perceptual approximations (0.0 = pure consonance, 1.0 = max tension).
// Bijection with ChordType enum order; Count sentinel not used.
//
namespace detail
{
    // tension_for[] indexed by ChordType ordinal.
    // Tertian tension ordering per common-practice perception:
    //   Major/Sus chords = low (0.0–0.2)
    //   Dominant/minor7  = mid (0.3–0.5)
    //   Diminished/HalfDim = high (0.6–0.8)
    //   Quartal/Modal-world = varied by brightness
    inline constexpr float tension_for[] = {
        // Maj   Min    Dom7   Maj7   Min7   Dim    Aug    Sus2   Sus4
           0.0f, 0.1f,  0.45f, 0.25f, 0.30f, 0.75f, 0.55f, 0.05f, 0.05f,
        // Add9  Min9   Maj9   Dom9   HalfDim
           0.20f, 0.35f, 0.30f, 0.50f, 0.70f,
        // Quartal3  Quartal4  Quintal3
           0.40f,    0.50f,    0.30f,
        // Hijaz  Bhairavi  YoScale  InScale
           0.65f, 0.60f,    0.15f,   0.25f,
        // Unison  Open5
           0.0f,   0.10f,
    };

    static_assert(static_cast<int>(ChordType::Count) ==
                  static_cast<int>(sizeof(tension_for) / sizeof(tension_for[0])),
                  "tension_for size must match ChordType::Count");
}

//==============================================================================
class XOuijaWalkEngine
{
public:
    //==========================================================================
    // Snapshot — lock-free read for the UI thread.
    //
    struct Snapshot
    {
        int   cellIndex  = 0;
        float col        = 0.0f;   // [0, kCols-1] as float
        float row        = 0.0f;   // [0, kRows-1] as float
        float heatmap[XOuijaCellGrid::kSize] {};
    };

    //--------------------------------------------------------------------------
    // Constructors
    //--------------------------------------------------------------------------

    // Explicit default constructor required: JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR
    // declares 'XOuijaWalkEngine(const XOuijaWalkEngine&) = delete', which suppresses
    // the compiler-generated default constructor.  Without it the compiler refuses to
    // default-construct this class (juce::AbstractFifo has no default constructor and
    // the in-class initializer 'editFifo_ { 64 }' is only applied by a defaulted ctor).
    XOuijaWalkEngine() = default;

    //--------------------------------------------------------------------------
    // Audio thread API
    //--------------------------------------------------------------------------

    void prepareToPlay(double sampleRate, double /*blockSize*/)
    {
        sampleRate_ = (sampleRate > 0.0) ? sampleRate : 44100.0;
        // Step size is recomputed each block from live BPM (processBlock()).
        // Guard initial division by setting a safe default (1 bar at 120 BPM).
        recomputeStepSize(120.0, sampleRate_);
        phaseSamples_ = 0.0;
    }

    // processBlock — call once per audio callback on the audio thread.
    // numSamples : block size in samples.
    // bpm        : current host tempo (from PlayHead).  Must be > 0.
    // ppqPosition: host playhead position in quarter notes.
    // isPlaying  : host transport running.
    //
    // When isPlaying is false the planchette is frozen (no steps advance).
    //
    void processBlock(int numSamples, double bpm, double /*ppqPosition*/, bool isPlaying)
    {
        applyPendingEdits();

        if (!isPlaying || bpm <= 0.0)
            return;

        recomputeStepSize(bpm, sampleRate_);
        phaseSamples_ += static_cast<double>(numSamples);

        if (phaseSamples_ >= stepSizeSamples_)
        {
            phaseSamples_ = std::fmod(phaseSamples_, stepSizeSamples_);
            advanceStep();
        }
    }

    //--------------------------------------------------------------------------
    // Grid mutation — enqueue from UI thread (SPSC lock-free).
    //
    // Delivers cell edits to the audio thread via AbstractFifo.
    // Drops silently if the queue is full (64 slots, capacity > any single UI burst).
    //
    void enqueueEdit(int cellIndex, const XOuijaCell& cell)
    {
        if (cellIndex < 0 || cellIndex >= XOuijaCellGrid::kSize) return;
        int start1, size1, start2, size2;
        editFifo_.prepareToWrite(1, start1, size1, start2, size2);
        if (size1 > 0)
        {
            editBuf_[static_cast<size_t>(start1)] = { cellIndex, cell };
            editFifo_.finishedWrite(1);
        }
        // Queue full: drop silently — UI will retry on next interaction.
    }

    //--------------------------------------------------------------------------
    // Mood / tendency atomics — safe from any thread.
    //
    void setCalmWild(float v) noexcept
    {
        calmWild_.store(juce::jlimit(0.0f, 1.0f, v), std::memory_order_relaxed);
    }
    void setConsonantDissonant(float v) noexcept
    {
        consonantDissonant_.store(juce::jlimit(0.0f, 1.0f, v), std::memory_order_relaxed);
    }
    void setTendencyCol(float v) noexcept
    {
        tendencyCol_.store(juce::jlimit(-1.0f, 1.0f, v), std::memory_order_relaxed);
    }
    void setTendencyRow(float v) noexcept
    {
        tendencyRow_.store(juce::jlimit(-1.0f, 1.0f, v), std::memory_order_relaxed);
    }

    //--------------------------------------------------------------------------
    // Walk tempo (in beats/bars per step).
    // Default = 4 beats (1 bar at 4/4).  Selectable: 0.5 / 1 / 2 / 4 / 8 beats.
    // Call from message thread; recomputeStepSize() reads it atomically.
    //
    void setBeatsPerStep(float beats) noexcept
    {
        beatsPerStep_.store(juce::jlimit(0.5f, 8.0f, beats), std::memory_order_relaxed);
    }

    //--------------------------------------------------------------------------
    // Output — polled by XOuijaOutputRouter after processBlock().
    //
    [[nodiscard]] const XOuijaCell& currentCell() const noexcept
    {
        return grid_.at(currentIndex_);
    }

    // Returns true if the planchette advanced to a new cell since the last
    // call to clearCellChangedFlag().  XOuijaOutputRouter uses this to avoid
    // dispatching the same cell twice.
    [[nodiscard]] bool hasCellChanged() const noexcept
    {
        return cellChanged_.load(std::memory_order_acquire);
    }
    void clearCellChangedFlag() noexcept
    {
        cellChanged_.store(false, std::memory_order_release);
    }

    // Backward-compat CC 85/86: normalized planchette column [0,1] and row [0,1].
    [[nodiscard]] float circleX() const noexcept
    {
        const int col = currentIndex_ % XOuijaCellGrid::kCols;
        return static_cast<float>(col) /
               static_cast<float>(XOuijaCellGrid::kCols - 1);
    }
    [[nodiscard]] float influenceY() const noexcept
    {
        const int row = currentIndex_ / XOuijaCellGrid::kCols;
        return static_cast<float>(row) /
               static_cast<float>(XOuijaCellGrid::kRows - 1);
    }

    //--------------------------------------------------------------------------
    // Snapshot — lock-free read, safe from any thread including UI thread.
    //
    [[nodiscard]] Snapshot getSnapshot() const noexcept
    {
        Snapshot s;
        s.cellIndex = atomicIndex_.load(std::memory_order_acquire);
        s.col = static_cast<float>(s.cellIndex % XOuijaCellGrid::kCols);
        s.row = static_cast<float>(s.cellIndex / XOuijaCellGrid::kCols);
        for (int i = 0; i < XOuijaCellGrid::kSize; ++i)
            s.heatmap[i] = heatmapAtomics_[static_cast<size_t>(i)]
                               .load(std::memory_order_relaxed);
        return s;
    }

    //--------------------------------------------------------------------------
    // Grid access for ValueTree persistence (message-thread-only; call only
    // from getStateInformation() / setStateInformation()).
    //
    [[nodiscard]] const XOuijaCellGrid& gridForPersistence() const noexcept
    {
        return grid_;
    }

    // Replaces the entire grid atomically via a bulk enqueue.
    // Must only be called on the message thread (setStateInformation).
    // Each cell is enqueued individually through the SPSC queue so the
    // audio thread receives them without lock.
    void loadGridFromValueTree(const juce::ValueTree& root)
    {
        XOuijaCellGrid loaded;
        loaded.fromValueTree(root);
        // Enqueue all 64 cells.  Queue capacity = 64 — exactly fits.
        for (int i = 0; i < XOuijaCellGrid::kSize; ++i)
            enqueueEdit(i, loaded.at(i));
    }

    // Returns a persisted ValueTree snapshot of the grid (message-thread-only).
    // Called from getStateInformation().
    [[nodiscard]] juce::ValueTree saveGridToValueTree() const
    {
        return grid_.toValueTree();
    }

    //--------------------------------------------------------------------------
    // Optional callback: invoked on the AUDIO THREAD each time the planchette
    // advances to a new cell.  XOuijaOutputRouter wires this in processBlock.
    // Must be RT-safe (no heap allocation, no locks).
    //
    std::function<void(int /*cellIndex*/, const XOuijaCell& /*cell*/)> onCellChanged;

private:
    //--------------------------------------------------------------------------
    // Audio-thread state (no atomics needed — audio thread owns exclusively).
    //
    XOuijaCellGrid grid_ {};
    int            currentIndex_    = 0;
    double         phaseSamples_    = 0.0;
    double         stepSizeSamples_ = 88200.0;   // safe default (1 bar, 120 BPM, 44100 Hz)
    double         sampleRate_      = 44100.0;

    // PCG-based RNG — deterministic within a session, no heap allocation.
    uint64_t       rngState_  = 0x853c49e6748fea9bULL;
    uint64_t       rngStream_ = 0xda3e39cb94b95bdbULL;

    //--------------------------------------------------------------------------
    // Cross-thread atomics.
    //
    std::atomic<int>   atomicIndex_   { 0 };
    std::atomic<bool>  cellChanged_   { false };

    std::atomic<float> calmWild_             { 0.3f };
    std::atomic<float> consonantDissonant_   { 0.2f };
    std::atomic<float> tendencyCol_          { 0.0f };
    std::atomic<float> tendencyRow_          { 0.0f };
    std::atomic<float> beatsPerStep_         { 4.0f };   // default = 1 bar (4 beats)

    // Heatmap atomics: written on audio thread, read on UI thread via getSnapshot().
    std::array<std::atomic<float>, XOuijaCellGrid::kSize> heatmapAtomics_ {};

    //--------------------------------------------------------------------------
    // SPSC edit queue (UI → audio thread).
    //
    struct CellEdit
    {
        int        index = 0;
        XOuijaCell cell  {};
    };
    juce::AbstractFifo                      editFifo_ { 64 };
    std::array<CellEdit, 64>                editBuf_  {};

    //--------------------------------------------------------------------------
    // Helpers
    //--------------------------------------------------------------------------

    void recomputeStepSize(double bpm, double sr) noexcept
    {
        const float beatsPerStep = beatsPerStep_.load(std::memory_order_relaxed);
        // step duration in seconds = beatsPerStep * (60 / bpm)
        stepSizeSamples_ = static_cast<double>(beatsPerStep) * 60.0 / bpm * sr;
        if (stepSizeSamples_ < 1.0) stepSizeSamples_ = 1.0;
    }

    void applyPendingEdits()
    {
        int start1, size1, start2, size2;
        editFifo_.prepareToRead(editFifo_.getNumReady(), start1, size1, start2, size2);
        const int total = size1 + size2;
        if (total == 0) return;

        auto apply = [this](int base, int count)
        {
            for (int i = 0; i < count; ++i)
            {
                const auto& e = editBuf_[static_cast<size_t>(base + i)];
                if (e.index >= 0 && e.index < XOuijaCellGrid::kSize)
                    grid_.at(e.index) = e.cell;
            }
        };
        apply(start1, size1);
        apply(start2, size2);
        editFifo_.finishedRead(total);
    }

    void advanceHeatmap(int justVisited)
    {
        // Decay constant: fully cold after ~100 seconds.
        // decay = exp(-1 / (100 * beatsPerStep * 60 / bpm * sr / block_per_step))
        // Simplified: attenuate by a fixed factor per step.
        // At 120 BPM, 1-bar steps = 2 seconds/step.  In 50 steps (~100 sec) → 0.
        // Factor per step = exp(-1/50) ≈ 0.9802.
        constexpr float kDecayPerStep = 0.9802f;

        for (int i = 0; i < XOuijaCellGrid::kSize; ++i)
        {
            const float current = heatmapAtomics_[static_cast<size_t>(i)]
                                      .load(std::memory_order_relaxed);
            const float decayed = current * kDecayPerStep;
            heatmapAtomics_[static_cast<size_t>(i)]
                .store(decayed, std::memory_order_relaxed);
        }
        // Set visited cell to 1.0
        heatmapAtomics_[static_cast<size_t>(justVisited)]
            .store(1.0f, std::memory_order_release);
    }

    // PCG-32 minimal random float in [0, 1).
    float randFloat() noexcept
    {
        rngState_ = rngState_ * 6364136223846793005ULL + (rngStream_ | 1ULL);
        const uint32_t xorshifted =
            static_cast<uint32_t>(((rngState_ >> 18u) ^ rngState_) >> 27u);
        const uint32_t rot = static_cast<uint32_t>(rngState_ >> 59u);
        const uint32_t v = (xorshifted >> rot) | (xorshifted << ((-static_cast<int32_t>(rot)) & 31));
        // Convert to [0,1)
        return static_cast<float>(v) * (1.0f / static_cast<float>(0x100000000LL));
    }

    // Returns a random int in [0, n).  Requires n > 0.
    int randInt(int n) noexcept
    {
        return static_cast<int>(randFloat() * static_cast<float>(n));
    }

    // Chord tension for a cell index (audio-thread read from grid_).
    float cellTension(int idx) const noexcept
    {
        const ChordType ct = grid_.at(idx).chord.type;
        const int ordinal  = static_cast<int>(ct);
        if (ordinal >= 0 && ordinal < static_cast<int>(ChordType::Count))
            return detail::tension_for[ordinal];
        return 0.5f;
    }

    void advanceStep()
    {
        const int nextIndex = selectNextCell();
        currentIndex_ = nextIndex;

        atomicIndex_.store(nextIndex, std::memory_order_release);
        cellChanged_.store(true, std::memory_order_release);

        advanceHeatmap(nextIndex);

        if (onCellChanged)
            onCellChanged(nextIndex, grid_.at(nextIndex));
    }

    int selectNextCell()
    {
        const float cw  = calmWild_.load(std::memory_order_relaxed);
        const float cd  = consonantDissonant_.load(std::memory_order_relaxed);
        const float tCol = tendencyCol_.load(std::memory_order_relaxed);
        const float tRow = tendencyRow_.load(std::memory_order_relaxed);

        const int curCol = currentIndex_ % XOuijaCellGrid::kCols;
        const int curRow = currentIndex_ / XOuijaCellGrid::kCols;

        // Maximum leap radius: calm(0) = 1 (adjacent only), wild(1) = 7 (full board).
        const float maxRadius = 1.0f + cw * 6.0f;

        // Build candidate + weight arrays on the stack — no heap allocation.
        int   candidates[XOuijaCellGrid::kSize];
        float weights   [XOuijaCellGrid::kSize];
        int   numCandidates = 0;
        float totalWeight   = 0.0f;

        for (int idx = 0; idx < XOuijaCellGrid::kSize; ++idx)
        {
            if (grid_.at(idx).pinned) continue;
            if (idx == currentIndex_)  continue;  // don't stay in place

            const int col = idx % XOuijaCellGrid::kCols;
            const int row = idx / XOuijaCellGrid::kCols;

            // Chebyshev distance (max of col/row delta)
            const float dCol = static_cast<float>(std::abs(col - curCol));
            const float dRow = static_cast<float>(std::abs(row - curRow));
            const float dist = std::max(dCol, dRow);

            if (dist > maxRadius) continue;

            // Tendency bias: dot product of (col-curCol, row-curRow) with tendency vector.
            // Positive = cell is in the tendency direction.
            const float tendBias = ((static_cast<float>(col - curCol) * tCol) +
                                    (static_cast<float>(row - curRow) * tRow));
            const float tendWeight = std::max(0.01f, 0.5f + tendBias * 0.5f);

            // Consonant/dissonant bias.
            const float tension = cellTension(idx);
            // cd=0 → prefer low tension (weight = 1 - tension)
            // cd=1 → prefer high tension (weight = tension)
            const float cdWeight = std::max(0.01f,
                (1.0f - cd) * (1.0f - tension) + cd * tension);

            // Heatmap cooldown: recently visited cells are down-weighted.
            const float heat = heatmapAtomics_[static_cast<size_t>(idx)]
                                   .load(std::memory_order_relaxed);
            const float heatWeight = std::max(0.05f, 1.0f - heat * 0.9f);

            const float w = tendWeight * cdWeight * heatWeight;

            candidates[numCandidates] = idx;
            weights   [numCandidates] = w;
            ++numCandidates;
            totalWeight += w;
        }

        if (numCandidates == 0)
        {
            // All candidates excluded (all pinned?): leap to a random unpinned cell.
            for (int idx = 0; idx < XOuijaCellGrid::kSize; ++idx)
            {
                if (!grid_.at(idx).pinned && idx != currentIndex_)
                {
                    candidates[numCandidates++] = idx;
                    totalWeight += 1.0f;
                }
            }
        }
        if (numCandidates == 0)
            return currentIndex_;  // grid is 100% pinned; stay put

        // Weighted random selection.
        float threshold = randFloat() * totalWeight;
        for (int i = 0; i < numCandidates; ++i)
        {
            threshold -= weights[i];
            if (threshold <= 0.0f)
                return candidates[i];
        }
        return candidates[numCandidates - 1];
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(XOuijaWalkEngine)
};

} // namespace xoceanus
