#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

namespace xolokun {

// ColumnLayoutManager — computes all zone rectangles for the 3-column + header
// + status bar + PlaySurface layout. Pure data struct, no JUCE Component inheritance.
// Used by XOlokunEditor::resized() to get bounds for all zones.
//
// Default proportions at 1100x700:
//   Header: 64pt
//   Column A (Engine Rack): 260pt
//   Column B (Main Canvas): 520pt
//   Column C (Browser/Inspector): 320pt
//   Status Bar: 28pt
//   PlaySurface: 220pt (when visible)
//   FieldMap: 65pt (within Column B)
//
// At minimum 960x600:
//   Columns scale proportionally: 240/440/280
//   PlaySurface auto-collapses if body height < 320pt
//
// MPC Hardware 800x480:
//   2-column fallback: Column A + Column B only, Column C hidden
//
// Cinematic Mode:
//   Columns A+C collapse, Column B expands to full width

struct ColumnLayoutManager
{
    // ── Configuration ──────────────────────────────────────────
    static constexpr int kHeaderH          = 64;
    static constexpr int kStatusBarH       = 28;
    static constexpr int kPlaySurfaceH     = 220;
    static constexpr int kFieldMapH        = 65;

    // Default column widths at reference width (1100)
    static constexpr int kRefWidth         = 1100;
    static constexpr int kDefaultColA      = 260;
    static constexpr int kDefaultColB      = 520;
    static constexpr int kDefaultColC      = 320;

    // Minimum column widths (below which content is unusable)
    static constexpr int kMinColA          = 180;
    static constexpr int kMinColB          = 400;
    static constexpr int kMinColC          = 48;   // icon strip when collapsed
    static constexpr int kMinBodyH         = 320;  // auto-collapse PlaySurface threshold

    // MPC hardware breakpoint
    static constexpr int kMpcMaxWidth      = 800;

    // ── State ──────────────────────────────────────────────────
    bool playSurfaceVisible  = false;
    bool columnCCollapsed    = false;
    bool cinematicMode       = false;

    // ── Compute ────────────────────────────────────────────────
    // Call this with the editor's total width and height.
    // All getXxx() methods return valid rectangles after compute().
    void compute(int totalWidth, int totalHeight)
    {
        totalW = totalWidth;
        totalH = totalHeight;

        // Header and status bar are fixed height
        headerBounds    = { 0, 0, totalW, kHeaderH };
        statusBarBounds = { 0, totalH - kStatusBarH, totalW, kStatusBarH };

        // Body region: between header and status bar
        int bodyTop    = kHeaderH;
        int bodyBottom = totalH - kStatusBarH;

        // PlaySurface: at bottom of body, above status bar
        if (playSurfaceVisible)
        {
            int availableBody = bodyBottom - bodyTop;
            if (availableBody - kPlaySurfaceH < kMinBodyH)
            {
                // Auto-collapse: not enough room for columns + PlaySurface
                playSurfaceAutoCollapsed = true;
                playSurfaceBounds = { 0, totalH, totalW, kPlaySurfaceH }; // parked fully off-screen (consistent with hidden case)
            }
            else
            {
                playSurfaceAutoCollapsed = false;
                playSurfaceBounds = { 0, bodyBottom - kPlaySurfaceH, totalW, kPlaySurfaceH };
                bodyBottom -= kPlaySurfaceH;
            }
        }
        else
        {
            playSurfaceAutoCollapsed = false;
            playSurfaceBounds = { 0, totalH, totalW, kPlaySurfaceH }; // parked below window
        }

        int bodyH = bodyBottom - bodyTop;

        // Column widths — proportional scaling with minimums
        if (cinematicMode)
        {
            // Cinematic: Column B takes full width
            colAWidth = 0;
            colBWidth = totalW;
            colCWidth = 0;
        }
        else if (totalW <= kMpcMaxWidth)
        {
            // MPC fallback: 2-column, no Column C
            colAWidth = std::max(kMinColA, totalW * kDefaultColA / kRefWidth);
            colCWidth = 0;
            colBWidth = totalW - colAWidth;
        }
        else if (columnCCollapsed)
        {
            // Column C collapsed to icon strip
            colCWidth = kMinColC;
            colAWidth = std::max(kMinColA, (totalW - colCWidth) * kDefaultColA / (kDefaultColA + kDefaultColB));
            colBWidth = totalW - colAWidth - colCWidth;
        }
        else
        {
            // Normal 3-column proportional
            float scale = static_cast<float>(totalW) / kRefWidth;
            colAWidth   = std::max(kMinColA, static_cast<int>(kDefaultColA * scale));
            colCWidth   = std::max(kMinColC, static_cast<int>(kDefaultColC * scale));
            colBWidth   = totalW - colAWidth - colCWidth;

            // Ensure Column B never goes below minimum
            if (colBWidth < kMinColB)
            {
                colBWidth = kMinColB;
                // Steal from C first, then A
                int remaining = totalW - colBWidth - colAWidth;
                if (remaining >= kMinColC)
                {
                    colCWidth = remaining;
                }
                else
                {
                    colCWidth = kMinColC;
                    colAWidth = totalW - colBWidth - colCWidth;
                }
            }
        }

        // Column bounds
        columnABounds = { 0,                    bodyTop, colAWidth, bodyH };
        columnBBounds = { colAWidth,             bodyTop, colBWidth, bodyH };
        columnCBounds = { colAWidth + colBWidth, bodyTop, colCWidth, bodyH };

        // FieldMap: bottom strip of Column B (inside Column B bounds)
        fieldMapBounds = { colAWidth, bodyTop + bodyH - kFieldMapH, colBWidth, kFieldMapH };

        // Column B panel area: Column B minus FieldMap
        columnBPanelBounds = { colAWidth, bodyTop, colBWidth, bodyH - kFieldMapH };
        if (columnBPanelBounds.getHeight() < 0)
            columnBPanelBounds.setHeight(0);
    }

    // ── Accessors ──────────────────────────────────────────────
    juce::Rectangle<int> getHeader()        const { return headerBounds; }
    juce::Rectangle<int> getStatusBar()     const { return statusBarBounds; }
    juce::Rectangle<int> getColumnA()       const { return columnABounds; }
    juce::Rectangle<int> getColumnB()       const { return columnBBounds; }
    juce::Rectangle<int> getColumnBPanel()  const { return columnBPanelBounds; }
    juce::Rectangle<int> getColumnC()       const { return columnCBounds; }
    juce::Rectangle<int> getPlaySurface()   const { return playSurfaceBounds; }
    juce::Rectangle<int> getFieldMap()      const { return fieldMapBounds; }

    bool isPlaySurfaceAutoCollapsed() const { return playSurfaceAutoCollapsed; }
    bool isMpcMode()        const { return !cinematicMode && totalW <= kMpcMaxWidth; }
    bool isCinematic()      const { return cinematicMode; }
    bool isColumnCVisible() const { return colCWidth > kMinColC; }

    int getColumnAWidth() const { return colAWidth; }
    int getColumnBWidth() const { return colBWidth; }
    int getColumnCWidth() const { return colCWidth; }

private:
    int totalW = 0, totalH = 0;
    int colAWidth = 0, colBWidth = 0, colCWidth = 0;
    bool playSurfaceAutoCollapsed = false;

    juce::Rectangle<int> headerBounds;
    juce::Rectangle<int> statusBarBounds;
    juce::Rectangle<int> columnABounds;
    juce::Rectangle<int> columnBBounds;
    juce::Rectangle<int> columnBPanelBounds;
    juce::Rectangle<int> columnCBounds;
    juce::Rectangle<int> playSurfaceBounds;
    juce::Rectangle<int> fieldMapBounds;
};

} // namespace xolokun
