// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>
#include <vector>

namespace xoceanus
{

//==============================================================================
// Layout modes determined by available width, not device type.
// This means an AUv3 in a narrow host panel gets iPhone layout even on iPad.
enum class LayoutMode
{
    iPhonePortrait,  // width < 430pt
    iPhoneLandscape, // width >= 430pt && height < 430pt
    iPadCompact,     // width >= 430pt && width < 600pt (Split View 33%)
    iPadRegular,     // width >= 600pt && width < 1024pt
    iPadFull,        // width >= 1024pt (also Desktop with XO_MOBILE)
    Desktop          // macOS (no XO_MOBILE compile flag)
};

//==============================================================================
// MobileLayoutManager — Adaptive layout coordination.
//
// Determines the current layout mode from component dimensions and notifies
// registered listeners when the mode changes. Provides layout constants
// appropriate to each mode.
//
// This is NOT a layout engine — it is a layout policy coordinator.
// Components query it for their mode-specific constants and arrange
// themselves accordingly.
//
class MobileLayoutManager
{
public:
    //-- Layout constants per mode -----------------------------------------------

    struct LayoutConstants
    {
        float headerHeight;
        float perfPadBarHeight;
        float perfStripHeight;         // Collapsed/peek height
        float perfStripExpandedHeight; // Expanded height
        float macroKnobSize;           // Visual size (touch target is always >= 44pt)
        float padMinSize;              // Minimum pad cell size
        float orbitDiameter;
        float drawerPeekHeight;
        int maxVisibleZones;    // How many PlaySurface zones visible at once
        bool carouselMode;      // True = one zone at a time, swipe between
        bool showOrbitInline;   // True = Orbit visible alongside note input
        bool showStripInline;   // True = strip always visible (not peek/expand)
        bool showBottomSection; // True = iPad-style bottom content area
    };

    static LayoutConstants getConstants(LayoutMode mode)
    {
        switch (mode)
        {
        case LayoutMode::iPhonePortrait:
            return {
                44.0f,  // headerHeight
                56.0f,  // perfPadBarHeight
                44.0f,  // perfStripHeight (peek)
                160.0f, // perfStripExpandedHeight
                24.0f,  // macroKnobSize
                60.0f,  // padMinSize
                200.0f, // orbitDiameter (overlay)
                100.0f, // drawerPeekHeight
                1,      // maxVisibleZones
                true,   // carouselMode
                false,  // showOrbitInline
                false,  // showStripInline
                false   // showBottomSection
            };

        case LayoutMode::iPhoneLandscape:
            return {
                44.0f,  // headerHeight (compact, top-left)
                56.0f,  // perfPadBarHeight (right edge, vertical)
                64.0f,  // perfStripHeight (bottom, always visible)
                64.0f,  // perfStripExpandedHeight (same — no expand in landscape)
                24.0f,  // macroKnobSize (left edge, stacked)
                50.0f,  // padMinSize
                180.0f, // orbitDiameter (overlay)
                0.0f,   // drawerPeekHeight (no drawer in landscape)
                1,      // maxVisibleZones
                false,  // carouselMode (fixed to one mode, 3-finger swipe to switch)
                false,  // showOrbitInline
                true,   // showStripInline
                false   // showBottomSection
            };

        case LayoutMode::iPadCompact:
            return {44.0f, 56.0f, 44.0f, 160.0f, 32.0f, 70.0f, 180.0f, 100.0f, 1, true, false, false, false};

        case LayoutMode::iPadRegular:
            return {
                48.0f,  // headerHeight
                0.0f,   // perfPadBarHeight (integrated into main area)
                80.0f,  // perfStripHeight
                80.0f,  // perfStripExpandedHeight (same — always visible)
                40.0f,  // macroKnobSize
                100.0f, // padMinSize
                200.0f, // orbitDiameter
                0.0f,   // drawerPeekHeight (bottom section instead)
                4,      // maxVisibleZones
                false,  // carouselMode
                true,   // showOrbitInline
                true,   // showStripInline
                true    // showBottomSection
            };

        case LayoutMode::iPadFull:
        case LayoutMode::Desktop:
            return {48.0f, 0.0f, 80.0f, 80.0f, 40.0f, 120.0f, 200.0f, 0.0f, 4, false, true, true, true};
        }

        // Fallback
        return getConstants(LayoutMode::iPadRegular);
    }

    //-- Mode detection ----------------------------------------------------------

    static LayoutMode detectMode(int widthPt, int heightPt)
    {
#if !XO_MOBILE
        return LayoutMode::Desktop;
#else
        if (widthPt < 430)
            return LayoutMode::iPhonePortrait;
        if (heightPt < 430)
            return LayoutMode::iPhoneLandscape;
        if (widthPt < 600)
            return LayoutMode::iPadCompact;
        if (widthPt < 1024)
            return LayoutMode::iPadRegular;
        return LayoutMode::iPadFull;
#endif
    }

    //-- Listener pattern --------------------------------------------------------

    using LayoutChangeCallback = std::function<void(LayoutMode newMode, const LayoutConstants& constants)>;

    void addListener(LayoutChangeCallback cb) { listeners.push_back(std::move(cb)); }

    // Call from the top-level component's resized() method.
    void updateLayout(int widthPt, int heightPt)
    {
        LayoutMode newMode = detectMode(widthPt, heightPt);
        if (newMode != currentMode)
        {
            currentMode = newMode;
            auto constants = getConstants(newMode);
            for (auto& cb : listeners)
                cb(newMode, constants);
        }
    }

    LayoutMode getCurrentMode() const { return currentMode; }
    LayoutConstants getCurrentConstants() const { return getConstants(currentMode); }

    //-- Helpers -----------------------------------------------------------------

    // Check if a component meets minimum touch target size
    static bool meetsMinTouchTarget(float widthPt, float heightPt) { return widthPt >= 44.0f && heightPt >= 44.0f; }

    // Get the safe area insets (notch, home indicator, etc.)
    // On iOS these come from UIView.safeAreaInsets; on desktop they are zero.
    struct SafeAreaInsets
    {
        float top = 0.0f, bottom = 0.0f, left = 0.0f, right = 0.0f;
    };

    void setSafeAreaInsets(SafeAreaInsets insets) { safeArea = insets; }
    SafeAreaInsets getSafeAreaInsets() const { return safeArea; }

    // Returns the usable bounds after subtracting safe area insets
    juce::Rectangle<float> getUsableBounds(float totalWidth, float totalHeight) const
    {
        return {safeArea.left, safeArea.top, totalWidth - safeArea.left - safeArea.right,
                totalHeight - safeArea.top - safeArea.bottom};
    }

private:
    LayoutMode currentMode = LayoutMode::iPhonePortrait;
    SafeAreaInsets safeArea;
    std::vector<LayoutChangeCallback> listeners;
};

} // namespace xoceanus
