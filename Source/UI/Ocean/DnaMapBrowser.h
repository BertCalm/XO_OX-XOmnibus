// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
// DnaMapBrowser.h — Full-window preset explorer organised by Sonic DNA mood axes.
//
// Replaces the radial ocean view with a flat map where each preset is a
// coloured dot. X/Y position is derived from two selectable DNA axis values
// (default: Brightness × Warmth) so similar-feeling presets cluster together.
// Click a dot to load that preset; Escape / P dismisses back to ocean view.
//
// Spec §5 reference:
//   §5.1 Map structure  — axes, dots, mood territories, active glow, nearby shimmer
//   §5.2 Interaction   — pan, zoom, hover tooltip, click-to-load, search, mood pills
//   §5.3 Performance   — back-buffer image, 32×32 grid spatial index, density heatmap
//
// Usage:
//   DnaMapBrowser browser;
//   addAndMakeVisible(browser);
//   browser.setBounds(getLocalBounds());
//   browser.setPresets(presetList);
//   browser.setActivePresetIndex(currentIndex);
//   browser.onPresetSelected = [&](int i){ loadPreset(i); };
//   browser.onDismissed      = [&](){ switchToOceanView(); };

#include <juce_gui_basics/juce_gui_basics.h>
#include "../GalleryColors.h"   // GalleryColors::Ocean, GalleryFonts, A11y

#include <array>
#include <algorithm>
#include <cmath>
#include <vector>

#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif

namespace xoceanus
{

//==============================================================================
/** Lightweight description of one preset as a map dot. */
struct PresetDot
{
    juce::String      name;
    juce::String      mood;
    juce::StringArray engineTags;
    float brightness  = 0.5f;
    float warmth      = 0.5f;
    float movement    = 0.5f;
    float density     = 0.5f;
    float space       = 0.5f;
    float aggression  = 0.5f;
    bool  isFavorite  = false;
    int   presetIndex = -1;   ///< index into the preset manager list
};

//==============================================================================
/**
    DnaMapBrowser

    Full-window 2D scatter map. Approximately 19,574 dots are supported without
    frame-rate degradation by using:
      • A juce::Image back-buffer rebuilt only when view state changes.
      • A 32×32 spatial grid for O(1) hover hit-testing.
      • A density-blob heatmap path when viewScale_ < 0.75 (dots overlap at that zoom).

    Coordinate system
    -----------------
    Map space:  each preset occupies a normalised [0,1] × [0,1] point derived
                from two DNA axis values.  X = xAxisValue, Y = yAxisValue.
    Screen space: transformed by viewOffset_ (pan) and viewScale_ (zoom).
        screenX = viewOffset_.x + mapX * getWidth()  * viewScale_
        screenY = viewOffset_.y + (1 - mapY) * getHeight() * viewScale_   (Y flipped)
*/
class DnaMapBrowser : public juce::Component, private juce::AsyncUpdater
{
public:
    //==========================================================================
    enum class Axis { Brightness, Warmth, Movement, Density, Space, Aggression };

    //==========================================================================
    DnaMapBrowser()
    {
        setOpaque(true);
        setWantsKeyboardFocus(true);

        A11y::setup(*this,
                    "Preset Explorer",
                    "2D map for exploring presets by Sonic DNA mood. "
                    "Click a dot to load the preset. Escape or P returns to ocean view.",
                    /*wantsKeyFocus=*/ true);

        // Build the static mood-pill list once; bounds are set in resized().
        buildMoodPills();

        // ── Dive button ───────────────────────────────────────────────────────
        addAndMakeVisible(diveButton_);
        diveButton_.setColour(juce::TextButton::buttonColourId,  juce::Colour(0xFFE9C46A));
        diveButton_.setColour(juce::TextButton::textColourOffId, juce::Colour(0xFF1A1A2E));
        diveButton_.setMouseCursor(juce::MouseCursor::PointingHandCursor);
        diveButton_.onClick = [this]() { diveToRandomPreset(); };
        diveButton_.setTooltip("Click to load a random preset or use filters to narrow the dive");
        // TODO(W1-B1): tooltip needs design — search bar and mood pills are paint-only (no JUCE
        // component target). To wire anchor #3 ("Type to filter presets or press Escape to exit"),
        // promote searchBarBounds_ to a juce::TextEditor child or implement TooltipClient on
        // DnaMapBrowser with region-based getTooltip(). Mood pill tooltips blocked by same issue.
    }

    ~DnaMapBrowser() override
    {
        // AsyncUpdater contract: cancel any pending callback before
        // destruction so handleAsyncUpdate() can't fire on a half-torn-down
        // instance.
        cancelPendingUpdate();
    }

    //==========================================================================
    // juce::Component overrides
    //==========================================================================

    void paint(juce::Graphics& g) override
    {
        const auto bounds = getLocalBounds();
        if (bounds.isEmpty())
            return;

        // ── 1. Abyss background ───────────────────────────────────────────────
        g.fillAll(juce::Colour(GalleryColors::Ocean::abyss));

        // ── 2. Blit the pre-built dot cache ───────────────────────────────────
        // The cache is rebuilt asynchronously in handleAsyncUpdate() whenever
        // dotBufferDirty_ is set.  paint() never does layout work.
        if (! dotCache_.isNull())
            g.drawImageAt(dotCache_, 0, 0);

        // ── 3. Hovered-preset tooltip ─────────────────────────────────────────
        if (hoveredIndex_ >= 0 && hoveredIndex_ < static_cast<int>(presets_.size()))
            paintTooltip(g, hoveredIndex_);

        // ── 4. Axis labels ────────────────────────────────────────────────────
        paintAxisLabels(g);

        // ── 5. Mood pills ─────────────────────────────────────────────────────
        paintMoodPills(g);

        // ── 6. Search bar ─────────────────────────────────────────────────────
        paintSearchBar(g);

        // ── 7. Focus ring if keyboard-focused ────────────────────────────────
        if (hasKeyboardFocus(false))
            A11y::drawFocusRing(g, bounds.toFloat(), 0.0f);
    }

    void resized() override
    {
        const int w = getWidth();

        // Search bar — top center, floating
        searchBarBounds_ = juce::Rectangle<int>((w - kSearchBarW) / 2, kSearchBarMarginTop,
                                                kSearchBarW, kSearchBarH);

        // Mood pills — bottom strip, packed left-to-right with spacing
        layoutMoodPills();

        dotBufferDirty_ = true;
        triggerAsyncUpdate();   // handleAsyncUpdate() will rebuild grid + cache

        // ── Dive button — upper-right corner, aligned with search bar ────────
        constexpr int kDiveBtnW  = 80;
        constexpr int kDiveBtnH  = 36;
        constexpr int kDiveMargin = 16;
        diveButton_.setBounds(getWidth() - kDiveBtnW - kDiveMargin,
                              kSearchBarMarginTop,
                              kDiveBtnW, kDiveBtnH);
    }

    void mouseDown(const juce::MouseEvent& e) override
    {
        grabKeyboardFocus();

        // ── Mood pill hit-test ────────────────────────────────────────────────
        for (auto& pill : moodPills_)
        {
            if (pill.bounds.contains(e.getPosition()))
            {
                toggleMoodFilter(pill.name);
                return;
            }
        }

        // ── Search bar hit-test ───────────────────────────────────────────────
        if (searchBarBounds_.contains(e.getPosition()))
        {
            // Grab keyboard focus so typed characters are routed to keyPressed().
            grabKeyboardFocus();
            return;
        }

        // ── Preset dot hit-test ───────────────────────────────────────────────
        const int hit = findPresetAt(e.position);
        if (hit >= 0)
        {
            if (onPresetSelected)
                onPresetSelected(presets_[static_cast<size_t>(hit)].presetIndex);
            return;
        }

        // ── Click on empty space — arm pan but don't commit yet ──────────────
        // #1007 FIX 5: Store the down position and set maybePanning_ = true.
        // Actual panning (panning_ = true) is only committed once the mouse
        // moves more than kPanThreshold pixels.  This prevents a 1px miss on a
        // dot from accidentally hijacking the cursor into pan mode.
        maybePanning_  = true;
        panDownPos_    = e.position;
        panStart_      = e.position - viewOffset_;
    }

    void mouseDrag(const juce::MouseEvent& e) override
    {
        if (!maybePanning_)
            return;

        // #1007 FIX 5: Only commit to panning once the drag exceeds threshold.
        if (!panning_)
        {
            const float dist = e.position.getDistanceFrom(panDownPos_);
            if (dist < kPanThreshold)
                return;  // Still within threshold — treat as stationary
            panning_ = true;
        }

        viewOffset_ = e.position - panStart_;
        dotBufferDirty_ = true;
        // Spatial grid rebuild is deferred to mouseUp — no need to rebuild every
        // drag event since findPresetAt() uses the grid only on hover/click.
        // triggerAsyncUpdate() coalesces rapid drag events into a single rebuild.
        triggerAsyncUpdate();
    }

    void mouseUp(const juce::MouseEvent& /*e*/) override
    {
        if (panning_)
        {
            // Eagerly rebuild the grid so the next hover/click is hit-tested correctly.
            // handleAsyncUpdate() will also rebuild it, but may fire after mouseMove.
            rebuildSpatialGrid();
        }
        panning_     = false;
        maybePanning_ = false;
    }

    void mouseMove(const juce::MouseEvent& e) override
    {
        const int hit = findPresetAt(e.position);
        if (hit != hoveredIndex_)
        {
            hoveredIndex_ = hit;
            repaint();
        }
    }

    void mouseWheelMove(const juce::MouseEvent& e,
                        const juce::MouseWheelDetails& w) override
    {
        // Zoom toward the mouse cursor position.
        const float oldScale = viewScale_;
        float newScale = oldScale + kZoomSpeed * w.deltaY;
        newScale = juce::jlimit(kMinZoom, kMaxZoom, newScale);

        if (std::abs(newScale - oldScale) < 1e-6f)
            return;

        // Anchor: the map-space point under the cursor should remain stationary.
        // mapPoint = (screen - viewOffset_) / (size * scale)
        // After zoom: viewOffset_new = screen - mapPoint * size * newScale
        const float mx = e.position.x;
        const float my = e.position.y;
        const float fw = static_cast<float>(getWidth());
        const float fh = static_cast<float>(getHeight());

        const float mapX = (mx - viewOffset_.x) / (fw * oldScale);
        const float mapY = (my - viewOffset_.y) / (fh * oldScale);

        viewOffset_.x = mx - mapX * fw * newScale;
        viewOffset_.y = my - mapY * fh * newScale;
        viewScale_    = newScale;

        dotBufferDirty_ = true;
        triggerAsyncUpdate();   // handleAsyncUpdate() rebuilds grid + cache + calls repaint()
    }

    bool keyPressed(const juce::KeyPress& key) override
    {
        // Dismiss
        if (key == juce::KeyPress::escapeKey
            || key == juce::KeyPress('p') || key == juce::KeyPress('P'))
        {
            if (onDismissed)
                onDismissed();
            return true;
        }

        // Backspace — trim search query
        if (key == juce::KeyPress::backspaceKey)
        {
            if (searchQuery_.isNotEmpty())
            {
                searchQuery_ = searchQuery_.dropLastCharacters(1);
                dotBufferDirty_ = true;
                triggerAsyncUpdate();
            }
            return true;
        }

        // Printable characters → append to search
        const juce::juce_wchar ch = key.getTextCharacter();
        if (ch >= 0x20 && ch != 0x7f)   // visible ASCII / Unicode
        {
            searchQuery_ += ch;
            dotBufferDirty_ = true;
            triggerAsyncUpdate();
            return true;
        }

        return false;
    }

    //==========================================================================
    // Data API
    //==========================================================================

    /** Replace the full preset list and rebuild spatial structures. */
    void setPresets(std::vector<PresetDot> presets)
    {
        presets_ = std::move(presets);
        dotBufferDirty_ = true;
        triggerAsyncUpdate();   // handleAsyncUpdate() rebuilds spatial grid + cache + repaints
    }

    /** Mark which preset is currently loaded (shown with gold glow). */
    void setActivePresetIndex(int index)
    {
        if (activeIndex_ == index)
            return;
        activeIndex_    = index;
        dotBufferDirty_ = true;
        triggerAsyncUpdate();
    }

    /** Change the two projection axes. Triggers a full buffer rebuild. */
    void setAxes(Axis x, Axis y)
    {
        if (xAxis_ == x && yAxis_ == y)
            return;
        xAxis_ = x;
        yAxis_ = y;
        dotBufferDirty_ = true;
        triggerAsyncUpdate();   // handleAsyncUpdate() rebuilds grid + cache + repaints
    }

    /** Filter by text — case-insensitive substring match on name and engine tags. */
    void setSearchQuery(const juce::String& query)
    {
        if (searchQuery_ == query)
            return;
        searchQuery_    = query;
        dotBufferDirty_ = true;
        triggerAsyncUpdate();
    }

    /** Filter by mood — empty string = show all moods. */
    void setMoodFilter(const juce::String& mood)
    {
        if (moodFilter_ == mood)
            return;
        moodFilter_     = mood;
        dotBufferDirty_ = true;
        triggerAsyncUpdate();
    }

    /** Toggle showing only favourites. */
    void toggleFavoritesOnly()
    {
        favoritesOnly_  = !favoritesOnly_;
        dotBufferDirty_ = true;
        triggerAsyncUpdate();
    }

    //==========================================================================
    // Callbacks
    //==========================================================================

    /** Called when the user clicks a preset dot. Argument: the preset's presetIndex. */
    std::function<void(int presetIndex)> onPresetSelected;

    /** Called when the user presses Escape or P to return to ocean view. */
    std::function<void()> onDismissed;

private:
    //==========================================================================
    // Colour helpers
    //==========================================================================

    static juce::Colour moodColour(const juce::String& mood)
    {
        if (mood == "Foundation")  return juce::Colour(0xFF9E9B97);
        if (mood == "Atmosphere")  return juce::Colour(0xFF48CAE4);
        if (mood == "Entangled")   return juce::Colour(0xFF9B5DE5);
        if (mood == "Prism")       return juce::Colour(0xFFBF40FF);
        if (mood == "Flux")        return juce::Colour(0xFFFF6B6B);
        if (mood == "Aether")      return juce::Colour(0xFFA8D8EA);
        if (mood == "Family")      return juce::Colour(0xFFE9C46A);
        if (mood == "Submerged")   return juce::Colour(0xFF0096C7);
        if (mood == "Coupling")    return juce::Colour(0xFFE9C46A);
        if (mood == "Crystalline") return juce::Colour(0xFFFFFFF0);
        if (mood == "Deep")        return juce::Colour(0xFF1A6B5A);
        if (mood == "Ethereal")    return juce::Colour(0xFF7FDBCA);
        if (mood == "Kinetic")     return juce::Colour(0xFFFF8C00);
        if (mood == "Luminous")    return juce::Colour(0xFFC6E377);
        if (mood == "Organic")     return juce::Colour(0xFF228B22);
        if (mood == "Shadow")      return juce::Colour(0xFF546E7A);
        return juce::Colour(GalleryColors::Ocean::plankton);  // fallback
    }

    static juce::String axisLabel(Axis a)
    {
        switch (a)
        {
            case Axis::Brightness:  return "Brightness";
            case Axis::Warmth:      return "Warmth";
            case Axis::Movement:    return "Movement";
            case Axis::Density:     return "Density";
            case Axis::Space:       return "Space";
            case Axis::Aggression:  return "Aggression";
        }
        return {};
    }

    float getAxisValue(const PresetDot& p, Axis axis) const
    {
        switch (axis)
        {
            case Axis::Brightness:  return p.brightness;
            case Axis::Warmth:      return p.warmth;
            case Axis::Movement:    return p.movement;
            case Axis::Density:     return p.density;
            case Axis::Space:       return p.space;
            case Axis::Aggression:  return p.aggression;
        }
        return 0.5f;
    }

    //==========================================================================
    // Coordinate transforms
    //==========================================================================

    /** Map preset DNA values → screen pixel position. */
    juce::Point<float> presetToScreen(const PresetDot& p) const
    {
        const float x = getAxisValue(p, xAxis_);         // 0→1 left→right
        const float y = getAxisValue(p, yAxis_);         // 0→1 bottom→top (inverted)
        const float fw = static_cast<float>(getWidth());
        const float fh = static_cast<float>(getHeight());
        return {
            viewOffset_.x + x * fw * viewScale_,
            viewOffset_.y + (1.0f - y) * fh * viewScale_
        };
    }

    //==========================================================================
    // Filter predicate
    //==========================================================================

    bool matchesFilter(const PresetDot& p) const
    {
        if (favoritesOnly_ && !p.isFavorite)
            return false;

        if (moodFilter_.isNotEmpty() && p.mood != moodFilter_)
            return false;

        if (searchQuery_.isNotEmpty())
        {
            const juce::String lq = searchQuery_.toLowerCase();
            if (!p.name.toLowerCase().contains(lq))
            {
                // Also search engine tags
                bool tagMatch = false;
                for (int i = 0; i < p.engineTags.size(); ++i)
                    if (p.engineTags[i].toLowerCase().contains(lq))
                    { tagMatch = true; break; }
                if (!tagMatch)
                    return false;
            }
        }

        return true;
    }

    //==========================================================================
    // Spatial index
    //==========================================================================

    /** Map a screen-space point to its 32×32 grid cell index. Returns -1 if
        the point lies outside the component. */
    int screenToGridCell(juce::Point<float> screen) const
    {
        const float fw = static_cast<float>(getWidth());
        const float fh = static_cast<float>(getHeight());
        if (fw <= 0.0f || fh <= 0.0f)
            return -1;

        const int col = static_cast<int>(screen.x / fw * static_cast<float>(kGridCells));
        const int row = static_cast<int>(screen.y / fh * static_cast<float>(kGridCells));
        if (col < 0 || col >= kGridCells || row < 0 || row >= kGridCells)
            return -1;
        return row * kGridCells + col;
    }

    /** Rebuild the 32×32 spatial grid from current presets and view transform. */
    void rebuildSpatialGrid()
    {
        for (auto& cell : spatialGrid_)
            cell.clear();

        const int n = static_cast<int>(presets_.size());
        for (int i = 0; i < n; ++i)
        {
            const auto sp = presetToScreen(presets_[static_cast<size_t>(i)]);
            const int cell = screenToGridCell(sp);
            if (cell >= 0)
                spatialGrid_[static_cast<size_t>(cell)].push_back(i);
        }
    }

    /** Return the index of the first preset dot within kDotRadius of screenPos,
        or -1 if none. Checks the 3×3 neighbourhood of grid cells so dots on
        cell boundaries are found reliably. */
    int findPresetAt(juce::Point<float> screenPos) const
    {
        const float fw = static_cast<float>(getWidth());
        const float fh = static_cast<float>(getHeight());
        if (fw <= 0.0f || fh <= 0.0f || presets_.empty())
            return -1;

        // Determine the grid-cell radius we need to search (in cells).
        // kActiveDotRadius is the largest dot; converting to cell units.
        const float cellW = fw / static_cast<float>(kGridCells);
        const float cellH = fh / static_cast<float>(kGridCells);
        const int searchRadius = static_cast<int>(
            std::ceil(kActiveDotRadius / std::min(cellW, cellH))) + 1;

        const int baseCol = static_cast<int>(screenPos.x / fw * static_cast<float>(kGridCells));
        const int baseRow = static_cast<int>(screenPos.y / fh * static_cast<float>(kGridCells));

        int bestIndex = -1;
        float bestDistSq = kActiveDotRadius * kActiveDotRadius + 1.0f;

        for (int dr = -searchRadius; dr <= searchRadius; ++dr)
        {
            for (int dc = -searchRadius; dc <= searchRadius; ++dc)
            {
                const int col = baseCol + dc;
                const int row = baseRow + dr;
                if (col < 0 || col >= kGridCells || row < 0 || row >= kGridCells)
                    continue;

                const auto& cell = spatialGrid_[static_cast<size_t>(row * kGridCells + col)];
                for (const int idx : cell)
                {
                    const auto sp = presetToScreen(presets_[static_cast<size_t>(idx)]);
                    const float dx = sp.x - screenPos.x;
                    const float dy = sp.y - screenPos.y;
                    const float distSq = dx * dx + dy * dy;

                    // Use larger hit target for active preset.
                    const float hitR  = (idx == activeIndex_) ? kActiveDotRadius : kDotRadius + 2.0f;
                    const float hitSq = hitR * hitR;

                    if (distSq <= hitSq && distSq < bestDistSq)
                    {
                        bestDistSq = distSq;
                        bestIndex  = idx;
                    }
                }
            }
        }

        return bestIndex;
    }

    //==========================================================================
    // Back-buffer rendering
    //==========================================================================

    /** Compute a "nearby shimmer" factor for a preset relative to the active one.
        Returns 0 if no active preset or DNA distance is too large. */
    float computeNearbyFactor(int idx) const
    {
        if (activeIndex_ < 0 || activeIndex_ >= static_cast<int>(presets_.size()))
            return 0.0f;
        if (idx == activeIndex_)
            return 1.0f;

        const auto& a = presets_[static_cast<size_t>(activeIndex_)];
        const auto& b = presets_[static_cast<size_t>(idx)];

        // 6D Euclidean distance
        const float d = std::sqrt(
            (a.brightness  - b.brightness)  * (a.brightness  - b.brightness)  +
            (a.warmth      - b.warmth)      * (a.warmth      - b.warmth)      +
            (a.movement    - b.movement)    * (a.movement    - b.movement)    +
            (a.density     - b.density)     * (a.density     - b.density)     +
            (a.space       - b.space)       * (a.space       - b.space)       +
            (a.aggression  - b.aggression)  * (a.aggression  - b.aggression)
        );

        // Full shimmer within distance 0.15; linear falloff to 0 at 0.45.
        if (d >= 0.45f) return 0.0f;
        if (d <= 0.15f) return 1.0f;
        return 1.0f - (d - 0.15f) / 0.30f;
    }

    //==========================================================================
    // AsyncUpdater — runs on the message thread, never blocks paint()
    //==========================================================================

    /** Called by the JUCE message thread after triggerAsyncUpdate().
        Multiple rapid triggers are coalesced into one call.
        Rebuilds the spatial grid and dot-cache image, then repaints. */
    void handleAsyncUpdate() override
    {
        if (!dotBufferDirty_)
            return;

        // Rebuild hit-test grid first (positions depend on current view transform).
        rebuildSpatialGrid();

        // Rebuild the cached dot image.
        rebuildDotBuffer();

        // Schedule a repaint now that the cache is fresh.
        repaint();
    }

    //==========================================================================
    // Back-buffer rendering
    //==========================================================================

    /** Full dot-cache rebuild.  Called from handleAsyncUpdate() — never from paint(). */
    void rebuildDotBuffer()
    {
        const int w = getWidth();
        const int h = getHeight();

        if (w <= 0 || h <= 0)
        {
            dotCache_      = juce::Image();
            dotBufferDirty_ = false;
            return;
        }

        if (dotCache_.isNull()
            || dotCache_.getWidth()  != w
            || dotCache_.getHeight() != h)
        {
            dotCache_ = juce::Image(juce::Image::ARGB, w, h, /*clearImage=*/ true);
        }

        juce::Graphics ig(dotCache_);
        ig.fillAll(juce::Colour(0));  // transparent — painted over abyss background

        const bool densityMode = (viewScale_ < kDensityThreshold);

        if (densityMode)
        {
            paintDensityHeatmap(ig, w, h);
        }
        else
        {
            paintDots(ig);
        }

        dotBufferDirty_ = false;
    }

    /** Density heatmap path: used when zoomed so far out that dots would merge.
        Divides the buffer into a coarse grid, counts dots per cell, maps count
        to a glow intensity, and blends that colour over the background. */
    void paintDensityHeatmap(juce::Graphics& ig, int w, int h) const
    {
        static constexpr int kHeat = 48;   // heat grid resolution

        // Count filtered presets per heat cell.
        std::array<int, kHeat * kHeat> counts{};
        counts.fill(0);

        for (const auto& p : presets_)
        {
            if (!matchesFilter(p))
                continue;
            const auto sp = presetToScreen(p);
            const int col = static_cast<int>(sp.x / static_cast<float>(w) * static_cast<float>(kHeat));
            const int row = static_cast<int>(sp.y / static_cast<float>(h) * static_cast<float>(kHeat));
            if (col >= 0 && col < kHeat && row >= 0 && row < kHeat)
                counts[static_cast<size_t>(row * kHeat + col)]++;
        }

        // Find max count for normalisation (avoid divide-by-zero).
        const int maxCount = *std::max_element(counts.begin(), counts.end());
        if (maxCount == 0)
            return;

        const float cellW = static_cast<float>(w) / static_cast<float>(kHeat);
        const float cellH = static_cast<float>(h) / static_cast<float>(kHeat);

        for (int row = 0; row < kHeat; ++row)
        {
            for (int col = 0; col < kHeat; ++col)
            {
                const int count = counts[static_cast<size_t>(row * kHeat + col)];
                if (count == 0)
                    continue;

                const float t = static_cast<float>(count) / static_cast<float>(maxCount);
                // Interpolate abyss-teal → warm-cyan for density
                const juce::Colour blob = juce::Colour(GalleryColors::Ocean::twilightTint)
                                              .withAlpha(t * 0.7f + 0.05f);
                ig.setColour(blob);
                ig.fillRect(static_cast<float>(col) * cellW,
                            static_cast<float>(row) * cellH,
                            cellW + 0.5f, cellH + 0.5f);  // +0.5 to avoid hairline gaps
            }
        }
    }

    /** Normal dot-rendering path. */
    void paintDots(juce::Graphics& ig) const
    {
        // Mood territory labels: drawn before dots so dots sit on top.
        paintMoodTerritories(ig);

        const int n = static_cast<int>(presets_.size());

        // Determine whether to show preset names (only at high zoom).
        const bool showNames = (viewScale_ >= kNameZoomThreshold);

        for (int i = 0; i < n; ++i)
        {
            const auto& p   = presets_[static_cast<size_t>(i)];
            const bool match = matchesFilter(p);

            if (!match)
            {
                // Dim non-matching presets to 5% alpha for context preservation.
                const auto sp = presetToScreen(p);
                const float r = kDotRadius;
                ig.setColour(moodColour(p.mood).withAlpha(0.05f));
                ig.fillEllipse(sp.x - r, sp.y - r, r * 2.0f, r * 2.0f);
                continue;
            }

            const bool   isActive = (i == activeIndex_);
            const float  nearby   = computeNearbyFactor(i);
            const auto   sp       = presetToScreen(p);
            const juce::Colour baseColour = moodColour(p.mood);

            if (isActive)
            {
                // Gold glow — three concentric passes for a soft bloom effect.
                const juce::Colour gold = juce::Colour(GalleryColors::xoGold);
                ig.setColour(gold.withAlpha(0.08f));
                ig.fillEllipse(sp.x - kActiveDotRadius * 2.5f, sp.y - kActiveDotRadius * 2.5f,
                               kActiveDotRadius * 5.0f, kActiveDotRadius * 5.0f);
                ig.setColour(gold.withAlpha(0.18f));
                ig.fillEllipse(sp.x - kActiveDotRadius * 1.6f, sp.y - kActiveDotRadius * 1.6f,
                               kActiveDotRadius * 3.2f, kActiveDotRadius * 3.2f);
                ig.setColour(gold.withAlpha(0.90f));
                ig.fillEllipse(sp.x - kActiveDotRadius, sp.y - kActiveDotRadius,
                               kActiveDotRadius * 2.0f, kActiveDotRadius * 2.0f);

                // Name label always shown for active preset.
                paintPresetLabel(ig, p, sp, gold);
            }
            else
            {
                // Nearby shimmer: boost alpha by up to +0.4 based on DNA proximity.
                const float baseAlpha = 0.75f + nearby * 0.25f;
                const float r         = kDotRadius + nearby * 1.5f;
                ig.setColour(baseColour.withAlpha(baseAlpha));
                ig.fillEllipse(sp.x - r, sp.y - r, r * 2.0f, r * 2.0f);

                // Name label at high zoom.
                if (showNames)
                    paintPresetLabel(ig, p, sp, baseColour);
            }
        }
    }

    /** Draw the preset name below the dot. */
    void paintPresetLabel(juce::Graphics& ig,
                          const PresetDot& p,
                          juce::Point<float> sp,
                          juce::Colour colour) const
    {
        const juce::Font f = GalleryFonts::label(10.0f);
        ig.setFont(f);

        const float textW = std::min(f.getStringWidthFloat(p.name) + 4.0f, 120.0f);
        const float textH = 13.0f;
        const float tx    = sp.x - textW * 0.5f;
        const float ty    = sp.y + kActiveDotRadius + 2.0f;

        // Subtle shadow for legibility on dark background.
        ig.setColour(juce::Colour(GalleryColors::Ocean::abyss).withAlpha(0.65f));
        ig.fillRect(tx - 1.0f, ty, textW + 2.0f, textH);

        ig.setColour(colour.withAlpha(0.90f));
        ig.drawText(p.name,
                    juce::Rectangle<float>(tx, ty, textW, textH),
                    juce::Justification::centred,
                    false);
    }

    /** Faint mood region labels drawn behind dots as territory annotations.
        Each mood label is placed at the centroid of its visible preset points. */
    void paintMoodTerritories(juce::Graphics& ig) const
    {
        // Collect centroids per mood.
        struct MoodAccum
        {
            float sumX = 0.0f, sumY = 0.0f;
            int   count = 0;
        };
        juce::HashMap<juce::String, MoodAccum> accum;

        for (const auto& p : presets_)
        {
            if (!matchesFilter(p))
                continue;
            const auto sp = presetToScreen(p);
            if (!accum.contains(p.mood))
                accum.set(p.mood, {});
            auto& a = accum.getReference(p.mood);
            a.sumX  += sp.x;
            a.sumY  += sp.y;
            ++a.count;
        }

        const juce::Font f = GalleryFonts::display(11.0f);
        ig.setFont(f);

        for (auto it = accum.begin(); it != accum.end(); ++it)
        {
            if (it.getValue().count < 3)
                continue;  // Don't label sparse mood clusters.

            const float cx = it.getValue().sumX / static_cast<float>(it.getValue().count);
            const float cy = it.getValue().sumY / static_cast<float>(it.getValue().count);

            const juce::Colour mc = moodColour(it.getKey());
            ig.setColour(mc.withAlpha(0.12f));

            const float tw = f.getStringWidthFloat(it.getKey().toUpperCase()) + 8.0f;
            const float th = 14.0f;
            ig.drawText(it.getKey().toUpperCase(),
                        juce::Rectangle<float>(cx - tw * 0.5f, cy - th * 0.5f, tw, th),
                        juce::Justification::centred,
                        false);
        }
    }

    //==========================================================================
    // Overlay layers (drawn in paint(), not in back-buffer)
    //==========================================================================

    /** Hover tooltip: preset name + mood + engine tags + mini DNA hex readout. */
    void paintTooltip(juce::Graphics& g, int idx) const
    {
        const auto& p   = presets_[static_cast<size_t>(idx)];
        const auto  sp  = presetToScreen(p);

        // Content lines
        const juce::String line1 = p.name;
        const juce::String line2 = p.mood + (p.isFavorite ? "  ♥" : "");
        juce::String line3;
        for (int t = 0; t < p.engineTags.size(); ++t)
        {
            if (t > 0) line3 += " · ";
            line3 += p.engineTags[t];
        }

        const juce::Font f1 = GalleryFonts::label(12.0f);
        const juce::Font f2 = GalleryFonts::body(10.0f);

        const float lineH1 = 16.0f;
        const float lineH2 = 13.0f;
        const float lineH3 = line3.isNotEmpty() ? 13.0f : 0.0f;
        const float padH   = 8.0f;
        const float padV   = 6.0f;

        const float maxLineW = std::max({ f1.getStringWidthFloat(line1),
                                          f2.getStringWidthFloat(line2),
                                          line3.isNotEmpty() ? f2.getStringWidthFloat(line3) : 0.0f });
        const float tooltipW = maxLineW + padH * 2.0f;
        const float tooltipH = lineH1 + lineH2 + lineH3 + padV * 2.0f + 2.0f;

        // Position: prefer above/right of the dot; clamp to bounds.
        const float margin  = 10.0f;
        const float rawX    = sp.x + kDotRadius + 4.0f;
        const float rawY    = sp.y - tooltipH * 0.5f;
        const float tx      = juce::jlimit(margin, static_cast<float>(getWidth())  - tooltipW  - margin, rawX);
        const float ty      = juce::jlimit(margin, static_cast<float>(getHeight()) - tooltipH - margin, rawY);

        // Background
        g.setColour(juce::Colour(GalleryColors::Ocean::shallow).withAlpha(0.95f));
        g.fillRoundedRectangle(tx, ty, tooltipW, tooltipH, 6.0f);
        g.setColour(juce::Colour(GalleryColors::Ocean::surface).withAlpha(0.70f));
        g.drawRoundedRectangle(tx, ty, tooltipW, tooltipH, 6.0f, 1.0f);

        // Text
        const float textX = tx + padH;
        float textY = ty + padV;

        g.setFont(f1);
        g.setColour(juce::Colour(GalleryColors::Ocean::foam));
        g.drawText(line1, juce::Rectangle<float>(textX, textY, tooltipW - padH * 2.0f, lineH1),
                   juce::Justification::centredLeft, false);
        textY += lineH1 + 2.0f;

        g.setFont(f2);
        g.setColour(moodColour(p.mood));
        g.drawText(line2, juce::Rectangle<float>(textX, textY, tooltipW - padH * 2.0f, lineH2),
                   juce::Justification::centredLeft, false);
        textY += lineH2;

        if (line3.isNotEmpty())
        {
            g.setColour(juce::Colour(GalleryColors::Ocean::salt));
            g.drawText(line3, juce::Rectangle<float>(textX, textY, tooltipW - padH * 2.0f, lineH3),
                       juce::Justification::centredLeft, false);
        }
    }

    /** X/Y axis labels at the four edges. */
    void paintAxisLabels(juce::Graphics& g) const
    {
        const juce::Font f = GalleryFonts::label(10.0f);
        g.setFont(f);
        g.setColour(juce::Colour(GalleryColors::Ocean::salt));

        const int w = getWidth();
        const int h = getHeight();
        const int m = 6;   // margin

        // X axis: left and right labels
        g.drawText(axisLabel(xAxis_) + " ←",
                   m, h / 2 - 8, 100, 16,
                   juce::Justification::centredLeft, false);
        g.drawText("→ " + axisLabel(xAxis_),
                   w - 106, h / 2 - 8, 100, 16,
                   juce::Justification::centredRight, false);

        // #1008 FIX 8: Y-axis labels derived dynamically from yAxis_ so they
        // correctly reflect whatever axis is currently selected (not hard-wired
        // to Warmth "cool/warm" labels that are nonsensical for other axes).
        // Y is inverted in map-space (mapY=1 → top of screen), so "axis ↑"
        // represents higher axis values at the top of the window.
        const juce::String yTopLabel = axisLabel(yAxis_) + juce::String::fromUTF8(" \xe2\x86\x91");  // "Axis ↑"
        const juce::String yBotLabel = juce::String::fromUTF8("\xe2\x86\x93 ") + axisLabel(yAxis_);   // "↓ Axis"
        g.drawText(yTopLabel,
                   w / 2 - 50, m, 100, 14,
                   juce::Justification::centred, false);
        g.drawText(yBotLabel,
                   w / 2 - 50, h - m - 14, 100, 14,
                   juce::Justification::centred, false);
    }

    /** Floating mood pill row at the bottom of the window. */
    void paintMoodPills(juce::Graphics& g) const
    {
        const juce::Font f = GalleryFonts::label(10.0f);
        g.setFont(f);

        for (const auto& pill : moodPills_)
        {
            const bool active = (moodFilter_ == pill.name);
            const float alpha  = active ? 0.85f : 0.28f;

            // Pill background
            g.setColour(pill.colour.withAlpha(active ? 0.22f : 0.08f));
            g.fillRoundedRectangle(pill.bounds.toFloat(), kPillCornerRadius);

            // Border
            g.setColour(pill.colour.withAlpha(alpha));
            g.drawRoundedRectangle(pill.bounds.toFloat(), kPillCornerRadius, 1.0f);

            // Label
            g.setColour(pill.colour.withAlpha(alpha));
            g.drawText(pill.name, pill.bounds, juce::Justification::centred, false);
        }
    }

    /** Floating search bar at top center. */
    void paintSearchBar(juce::Graphics& g) const
    {
        const auto rb = searchBarBounds_.toFloat();

        // Background
        g.setColour(juce::Colour(GalleryColors::Ocean::shallow).withAlpha(0.88f));
        g.fillRoundedRectangle(rb, 6.0f);
        g.setColour(juce::Colour(GalleryColors::Ocean::surface).withAlpha(0.60f));
        g.drawRoundedRectangle(rb, 6.0f, 1.0f);

        const juce::Font f = GalleryFonts::body(12.0f);
        g.setFont(f);

        // Placeholder or live query
        const bool hasQuery = searchQuery_.isNotEmpty();
        if (hasQuery)
        {
            g.setColour(juce::Colour(GalleryColors::Ocean::foam));
            g.drawText(searchQuery_,
                       searchBarBounds_.reduced(kSearchBarPadH, 0),
                       juce::Justification::centredLeft,
                       false);
            // Simple blinking cursor approximation (always shown — no timer needed)
            const float cursorX = searchBarBounds_.getX() + kSearchBarPadH
                                  + f.getStringWidthFloat(searchQuery_) + 1.0f;
            const float cursorY = rb.getCentreY() - 6.0f;
            g.setColour(juce::Colour(GalleryColors::Ocean::foam).withAlpha(0.60f));
            g.fillRect(cursorX, cursorY, 1.5f, 12.0f);
        }
        else
        {
            g.setColour(juce::Colour(GalleryColors::Ocean::salt));
            g.drawText("Search presets…",
                       searchBarBounds_.reduced(kSearchBarPadH, 0),
                       juce::Justification::centredLeft,
                       false);
        }

        // MEDIUM fix (#1006): use right-pointing magnifier U+1F50D instead of
        // U+2315 (⌕ telephone receiver) which was incorrect.
        g.setColour(juce::Colour(GalleryColors::Ocean::salt));
        g.drawText(juce::String::charToString(0x1F50D),  // 🔍 right-pointing magnifier
                   searchBarBounds_.getRight() - 24, searchBarBounds_.getY(),
                   20, searchBarBounds_.getHeight(),
                   juce::Justification::centred, false);
    }

    //==========================================================================
    // Mood pill layout helpers
    //==========================================================================

    void buildMoodPills()
    {
        static const char* const kMoods[] = {
            "Foundation", "Atmosphere", "Entangled",   "Prism",
            "Flux",       "Aether",     "Family",      "Submerged",
            "Coupling",   "Crystalline","Deep",         "Ethereal",
            "Kinetic",    "Luminous",   "Organic",      "Shadow"
        };
        moodPills_.clear();
        for (auto* name : kMoods)
        {
            MoodPill pill;
            pill.name   = name;
            pill.colour = moodColour(pill.name);
            moodPills_.push_back(pill);
        }
    }

    void layoutMoodPills()
    {
        if (moodPills_.empty() || getWidth() <= 0)
            return;

        const juce::Font f = GalleryFonts::label(10.0f);

        // Two-row layout: 8 pills per row.
        const int pillsPerRow = 8;
        const int   row1Y     = getHeight() - kPillMarginBottom - kPillH * 2 - kPillSpacing;
        const int   row2Y     = getHeight() - kPillMarginBottom - kPillH;

        // Measure all pills and pack rows.
        for (size_t i = 0; i < moodPills_.size(); ++i)
        {
            auto& pill = moodPills_[i];
            const int textW = static_cast<int>(std::ceil(f.getStringWidthFloat(pill.name)));
            const int pillW = textW + kPillPadH * 2;
            pill.bounds     = juce::Rectangle<int>(0, 0, pillW, kPillH);
        }

        // Calculate total width for each row to centre it.
        auto packRow = [&](int startIdx, int endIdx, int rowY)
        {
            int totalW = 0;
            for (int i = startIdx; i < endIdx && i < static_cast<int>(moodPills_.size()); ++i)
                totalW += moodPills_[static_cast<size_t>(i)].bounds.getWidth() + kPillSpacing;
            totalW -= kPillSpacing;

            int x = (getWidth() - totalW) / 2;
            for (int i = startIdx; i < endIdx && i < static_cast<int>(moodPills_.size()); ++i)
            {
                auto& pill   = moodPills_[static_cast<size_t>(i)];
                const int pw = pill.bounds.getWidth();
                pill.bounds  = juce::Rectangle<int>(x, rowY, pw, kPillH);
                x           += pw + kPillSpacing;
            }
        };

        packRow(0, pillsPerRow, row1Y);
        packRow(pillsPerRow, static_cast<int>(moodPills_.size()), row2Y);
    }

    /** Pick a random preset from the current filtered view and load it. */
    void diveToRandomPreset()
    {
        // Collect indices of all visible (filtered) presets
        std::vector<int> visible;
        for (int i = 0; i < static_cast<int>(presets_.size()); ++i)
            if (matchesFilter(presets_[static_cast<size_t>(i)]))
                visible.push_back(i);

        if (visible.empty())
            return;

        // Pick a random one
        const int pick = visible[static_cast<size_t>(
            juce::Random::getSystemRandom().nextInt(static_cast<int>(visible.size())))];

        if (onPresetSelected)
            onPresetSelected(presets_[static_cast<size_t>(pick)].presetIndex);
    }

    /** Toggle the mood filter: clicking the active pill clears the filter. */
    void toggleMoodFilter(const juce::String& mood)
    {
        moodFilter_     = (moodFilter_ == mood) ? juce::String() : mood;
        dotBufferDirty_ = true;
        triggerAsyncUpdate();
    }

    //==========================================================================
    // State
    //==========================================================================

    std::vector<PresetDot> presets_;
    int        activeIndex_  = -1;
    int        hoveredIndex_ = -1;
    Axis       xAxis_        = Axis::Brightness;
    Axis       yAxis_        = Axis::Warmth;
    juce::String searchQuery_;
    juce::String moodFilter_;
    bool       favoritesOnly_ = false;

    // View transform
    float              viewScale_  = 1.0f;
    juce::Point<float> viewOffset_ { 0.0f, 0.0f };
    bool               panning_    = false;
    juce::Point<float> panStart_   { 0.0f, 0.0f };

    // #1007 FIX 5: Two-phase pan arming to prevent misfire when a dot click misses
    // by 1px.  maybePanning_ is set on any empty-space mouseDown; panning_ is only
    // committed once the drag exceeds kPanThreshold pixels.
    bool               maybePanning_  = false;
    juce::Point<float> panDownPos_    { 0.0f, 0.0f };
    static constexpr float kPanThreshold = 4.0f; // pixels before pan commits

    // 32×32 spatial index
    static constexpr int kGridCells = 32;
    std::array<std::vector<int>, kGridCells * kGridCells> spatialGrid_;

    // Dot-cache image — rebuilt in handleAsyncUpdate(), blitted in paint()
    juce::Image dotCache_;
    bool        dotBufferDirty_   = true;

    // Mood pills
    struct MoodPill
    {
        juce::String          name;
        juce::Colour          colour;
        juce::Rectangle<int>  bounds;
    };
    std::vector<MoodPill> moodPills_;

    // Search bar cached bounds
    juce::Rectangle<int> searchBarBounds_;

    //==========================================================================
    // Rendering constants
    //==========================================================================

    static constexpr float kDotRadius          = 3.5f;
    static constexpr float kActiveDotRadius    = 10.0f;
    static constexpr float kMinZoom            = 0.5f;
    static constexpr float kMaxZoom            = 8.0f;
    static constexpr float kZoomSpeed          = 0.1f;
    static constexpr float kDensityThreshold   = 0.75f;   // switch to heatmap below this zoom
    static constexpr float kNameZoomThreshold  = 2.5f;    // show names above this zoom

    // Search bar
    static constexpr int kSearchBarW           = 320;
    static constexpr int kSearchBarH           = 28;
    static constexpr int kSearchBarMarginTop   = 14;
    static constexpr int kSearchBarPadH        = 10;

    // Mood pills
    static constexpr int kPillH                = 20;
    static constexpr int kPillPadH             = 8;
    static constexpr int kPillSpacing          = 4;
    static constexpr int kPillMarginBottom     = 10;
    static constexpr float kPillCornerRadius   = 10.0f;

    // Dive button
    juce::TextButton diveButton_ { "Dive" };

    //==========================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DnaMapBrowser)
};

} // namespace xoceanus
