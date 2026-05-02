// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
// SubmarineHudBar.h — Floating submarine HUD navigation bar for the XOceanus Ocean View.
//
// The bar floats at the top of the ocean viewport (positioned by the parent at
// top: 12px, left: 16px, right: 16px). Height is 40 px. The container itself
// is transparent and passes mouse clicks through to the button hit regions;
// setInterceptsMouseClicks(true, true) is used so this component receives events
// but only consumes them when a hit region is matched.
//
// Layout (left → right):
//
//   [ ☰ Engines ]  [ ↶ ]  [ ↷ ]  ─── [ ◀ ] [ Preset Name ] [ ▶ ] [ ♥ ] [ SAVE ] [ A/B ] ───  [ Chain ]  [ Export ]  REACT [dial]  [ ⚙ ]
//
// Preset navigation controls live in the flex-spacer region:
//   ◀ / ▶  — step through the preset list
//   Preset Name — read-only label, centred; click opens preset browser
//   ♥  — favourite toggle
//   SAVE — save current state to preset
//   A/B  — toggle A/B compare (latches a snapshot of current params)
//
// Button styles follow the submarine frosted-glass pill aesthetic:
//   • Default : bg rgba(14,16,22,0.70), border rgba(200,204,216,0.10), text rgba(200,204,216,0.55)
//   • Hover   : bg rgba(200,204,216,0.08), border rgba(200,204,216,0.18), text rgba(200,204,216,0.85)
//   • Active  : bg rgba(60,180,170,0.10),  border rgba(60,180,170,0.35),  text rgba(60,180,170,0.90)
//
// Icon buttons (Undo, Redo, Settings) are 32×32 px squares with custom Path geometry:
//   • Undo    — counterclockwise arc with left-pointing arrowhead
//   • Redo    — clockwise arc with right-pointing arrowhead
//   • Settings — standard gear: 8 evenly-spaced trapezoidal teeth around a centre hole
//
// The Chain button prepends a small chain-link icon (two interlocking ovals).
//
// The REACT dial is a 28×28 px rotary knob (same style as the SAT/DELAY/REVERB dials
// in MasterFXStripCompact): value arc from 135° to 405°, teal fill, indicator line.
// Vertical drag changes the normalised value (0–1). Drag up = increase.
//
// All interactions fire std::function callbacks — parent wires them without any
// upward include dependency on this component.
//
// File is entirely self-contained (header-only inline) following the XOceanus
// convention for UI sub-components.

#include <juce_gui_basics/juce_gui_basics.h>
#include "../GalleryColors.h"
#include "HudIcons.h"
#include <functional>
#include <cmath>
#include <vector>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace xoceanus
{

//==============================================================================
/**
    SubmarineHudBar

    Floating frosted-glass navigation bar pinned to the top of the Ocean View.
    See file header for full documentation.
*/
class SubmarineHudBar : public juce::Component,
                        public juce::SettableTooltipClient
{
public:
    //==========================================================================
    static constexpr int kBarHeight    = 40;
    static constexpr int kBtnHeight    = 28;
    static constexpr int kIconBtnSize  = 32;
    static constexpr int kDialSize     = 28;   // REACT rotary dial diameter

    //==========================================================================
    // Callbacks — parent wires these in the editor constructor.

    std::function<void()>      onEnginesClicked;
    std::function<void()>      onUndo;
    std::function<void()>      onRedo;
    std::function<void()>      onPresetPrev;     // step to previous preset
    std::function<void()>      onPresetNext;     // step to next preset
    std::function<void()>      onPresetNameClicked; // open preset browser
    std::function<void(bool)>  onFavChanged;     // toggled; bool = new fav state
    std::function<void()>      onSave;           // save preset
    std::function<void(bool)>  onABCompareChanged; // toggled; bool = new A/B state
    std::function<void()>      onChainToggled;   // toggles chain mode; check chainModeActive_ to read new state
    std::function<void()>      onExportClicked;
    std::function<void()>      onSettingsClicked;
    std::function<void(float)> onReactChanged;   // 0–1 normalised — ocean visual reactivity

    //==========================================================================
    SubmarineHudBar()
    {
        setOpaque(false);
        // Container passes through events to hit regions via manual hit-testing.
        // Children are painted manually — no real child components.
        setInterceptsMouseClicks(true, true);

        // Fix #1424: expose primary transport strip to screen readers.
        A11y::setup(*this,
                    "Transport and Preset Bar",
                    "Main transport controls: engines, undo, redo, preset navigation, save, export, settings");
    }

    //==========================================================================
    // State pushers — call from editor on the message thread.

    void setChainModeActive(bool active)
    {
        if (chainModeActive_ != active)
        {
            chainModeActive_ = active;
            repaint();
        }
    }

    /** Returns true when the Chain mode toggle is currently active. */
    bool isChainModeActive() const noexcept { return chainModeActive_; }

    /** wire(#orphan-sweep item 2): expose the fav button hit-rect in local coords.
        Used by FirstHourWalkthrough step 6 to point the bubble at the ♥ button.
        Returns empty rect before the first paint pass (bounds not yet computed). */
    juce::Rectangle<int> getFavBounds() const noexcept { return favBounds_.toNearestInt(); }

    /** F-003 / #1395: expose preset name label hit-rect in local coords.
        Used by FirstHourWalkthrough step 3 to point the bubble at the preset-name
        pill (which opens the DnaMapBrowser when clicked).
        Returns empty rect before the first layout pass (bounds not yet computed). */
    juce::Rectangle<int> getPresetNameBounds() const noexcept { return presetNameBounds_.toNearestInt(); }

    void setPresetName(const juce::String& name)
    {
        if (presetName_ != name) { presetName_ = name; repaint(); }
    }

    void setFavourite(bool isFav)
    {
        if (isFav_ != isFav) { isFav_ = isFav; repaint(); }
    }

    void setABCompareActive(bool active)
    {
        if (abCompareActive_ != active) { abCompareActive_ = active; repaint(); }
    }

    bool isABCompareActive() const noexcept { return abCompareActive_; }

    void setReactLevel(float level01)
    {
        const float clamped = juce::jlimit(0.0f, 1.0f, level01);
        if (reactLevel_ != clamped)
        {
            reactLevel_ = clamped;
            repaint();
        }
    }

    //==========================================================================
    // SettableTooltipClient override — hit-test position against control regions
    // and return a context-appropriate tooltip string.

    juce::String getTooltip() override
    {
        // regions_ is populated by buildLayout() which is called on every paint()
        // and resized(). If the component has been sized but not yet painted, call
        // buildLayout() ourselves so tooltip queries before first paint still work.
        if (regions_.empty() && getWidth() > 0)
            buildLayout();

        // Use the mouse position in local coordinates to find the hovered region.
        const auto mousePos = getMouseXYRelative();
        const float mx = static_cast<float>(mousePos.x);
        const float my = static_cast<float>(mousePos.y);

        for (const auto& reg : regions_)
        {
            if (reg.bounds.expanded(2.0f).contains(mx, my))
            {
                switch (reg.id)
                {
                    case kRegEngines:    return "Browse and add engines (E)";
                    case kRegUndo:       return "Undo last change (\xe2\x8c\x98Z)";
                    case kRegRedo:       return "Redo (\xe2\x8c\x98\xe2\x87\xa7Z)";
                    case kRegPresetPrev: return "Previous preset";
                    case kRegPresetName: return "Open preset browser";
                    case kRegPresetNext: return "Next preset";
                    case kRegFav:        return isFav_ ? "Remove from favourites"
                                                       : "Add to favourites";
                    case kRegSave:       return "Save preset (\xe2\x8c\x98S)";
                    case kRegABCompare:  return "Compare preset A vs B";
                    case kRegChain:      return "Open coupling / FX chain editor";
                    case kRegExport:     return "Export preset to file (.xometa)";
                    case kRegDial:       return "Reactivity \xe2\x80\x94 how strongly the visualizer responds to audio";
                    case kRegSettings:   return "Settings";
                    default:             break;
                }
            }
        }

        return {};
    }

private:
    //==========================================================================
    // Region IDs — used for hit-testing and hover tracking.

    enum RegionId
    {
        kRegEngines      = 0,
        kRegUndo         = 1,
        kRegRedo         = 2,
        kRegPresetPrev   = 3,   // ◀ previous preset
        kRegPresetName   = 4,   // preset name label (click → browser)
        kRegPresetNext   = 5,   // ▶ next preset
        kRegFav          = 6,   // ♥ favourite toggle
        kRegSave         = 7,   // SAVE
        kRegABCompare    = 8,   // A/B compare toggle
        kRegChain        = 9,
        kRegExport       = 10,
        kRegDial         = 11,  // REACT rotary dial
        kRegSettings     = 12,
    };

    struct HudRegion
    {
        juce::Rectangle<float> bounds;
        int                    id;
    };

    //==========================================================================
    // Layout helpers

    /** Rebuild all HudRegion rectangles from the current component width. */
    void buildLayout()
    {
        regions_.clear();

        const float w   = static_cast<float>(getWidth());
        const float h   = static_cast<float>(getHeight() > 0 ? getHeight() : kBarHeight);
        const float gap = 8.0f;

        const float btnY    = (h - static_cast<float>(kBtnHeight))   * 0.5f;
        const float iconY   = (h - static_cast<float>(kIconBtnSize))  * 0.5f;

        float x = 0.0f; // left cursor

        // --- Engines button (text pill, wider) ---
        {
            const float btnW = 74.0f;
            juce::Rectangle<float> r(x, btnY, btnW, static_cast<float>(kBtnHeight));
            regions_.push_back({ r, kRegEngines });
            enginesBounds_ = r;
            x += btnW + gap;
        }

        // --- Undo icon button (32×32) ---
        {
            juce::Rectangle<float> r(x, iconY, static_cast<float>(kIconBtnSize),
                                              static_cast<float>(kIconBtnSize));
            regions_.push_back({ r, kRegUndo });
            undoBounds_ = r;
            x += static_cast<float>(kIconBtnSize) + gap;
        }

        // --- Redo icon button (32×32) ---
        {
            juce::Rectangle<float> r(x, iconY, static_cast<float>(kIconBtnSize),
                                              static_cast<float>(kIconBtnSize));
            regions_.push_back({ r, kRegRedo });
            redoBounds_ = r;
            x += static_cast<float>(kIconBtnSize) + gap;
        }

        // ---- Right side: work right-to-left ----
        float rx = w;

        // --- Settings icon button (32×32) ---
        {
            juce::Rectangle<float> r(rx - static_cast<float>(kIconBtnSize), iconY,
                                     static_cast<float>(kIconBtnSize),
                                     static_cast<float>(kIconBtnSize));
            regions_.push_back({ r, kRegSettings });
            settingsBounds_ = r;
            rx -= static_cast<float>(kIconBtnSize) + gap;
        }

        // --- REACT dial (28×28 rotary knob) ---
        {
            const float d = static_cast<float>(kDialSize);
            juce::Rectangle<float> dialRect(rx - d, (h - d) * 0.5f, d, d);
            regions_.push_back({ dialRect.expanded(4.0f), kRegDial });
            reactDialBounds_ = dialRect;
            rx -= d + gap;
        }

        // --- "REACT" label (9 px, uppercase) ---
        {
            const float labelW = 36.0f;
            reactLabelBounds_ = juce::Rectangle<float>(
                rx - labelW,
                (h - 9.0f) * 0.5f,
                labelW, 9.0f);
            rx -= labelW + gap;
        }

        // --- Export button (text pill) ---
        {
            const float btnW = 64.0f;
            juce::Rectangle<float> r(rx - btnW, btnY, btnW,
                                     static_cast<float>(kBtnHeight));
            regions_.push_back({ r, kRegExport });
            exportBounds_ = r;
            rx -= btnW + gap;
        }

        // --- Chain button (text pill with chain-link icon) ---
        {
            const float btnW = 70.0f;
            juce::Rectangle<float> r(rx - btnW, btnY, btnW,
                                     static_cast<float>(kBtnHeight));
            regions_.push_back({ r, kRegChain });
            chainBounds_ = r;
            rx -= btnW + gap;
        }

        // ---- Centre: preset navigation (between redo and chain) ----
        // Lay out from the right edge of the left group: x is the left cursor after Redo.
        // Lay out: A/B | SAVE | ♥ | ▶ | [ PresetName ] | ◀
        // We place them working left from rx (right of Chain).

        // A/B Compare pill (right of SAVE)
        {
            const float btnW = 38.0f;
            juce::Rectangle<float> r(rx - btnW, btnY, btnW, static_cast<float>(kBtnHeight));
            regions_.push_back({ r, kRegABCompare });
            abCompareBounds_ = r;
            rx -= btnW + gap;
        }

        // SAVE pill
        {
            const float btnW = 48.0f;
            juce::Rectangle<float> r(rx - btnW, btnY, btnW, static_cast<float>(kBtnHeight));
            regions_.push_back({ r, kRegSave });
            saveBounds_ = r;
            rx -= btnW + gap;
        }

        // Favourite ♥ icon button (24×28 visual; 44×44 hit target for WCAG compliance)
        {
            const float btnW = 24.0f;
            juce::Rectangle<float> r(rx - btnW, btnY, btnW, static_cast<float>(kBtnHeight));
            favBounds_ = r;
            // Expand hit rect to ≥44×44; centre on visual glyph.
            const float hitExpandH = std::max(0.0f, (44.0f - r.getWidth())  / 2.0f);
            const float hitExpandV = std::max(0.0f, (44.0f - r.getHeight()) / 2.0f);
            regions_.push_back({ r.expanded(hitExpandH, hitExpandV), kRegFav });
            rx -= btnW + gap;
        }

        // ▶ next preset (20×28 visual; 44×44 hit target)
        {
            const float btnW = 20.0f;
            juce::Rectangle<float> r(rx - btnW, btnY, btnW, static_cast<float>(kBtnHeight));
            presetNextBounds_ = r;
            const float hitExpandH = std::max(0.0f, (44.0f - r.getWidth())  / 2.0f);
            const float hitExpandV = std::max(0.0f, (44.0f - r.getHeight()) / 2.0f);
            regions_.push_back({ r.expanded(hitExpandH, hitExpandV), kRegPresetNext });
            rx -= btnW + 2.0f;
        }

        // Preset name (fills remaining centre space)
        {
            const float nameW = std::max(60.0f, rx - x - 24.0f); // leave 24px for prev arrow
            const float nameX = x + 24.0f; // after ◀ arrow
            presetNameBounds_ = juce::Rectangle<float>(nameX, btnY, nameW, static_cast<float>(kBtnHeight));
            regions_.push_back({ presetNameBounds_, kRegPresetName });
        }

        // ◀ prev preset (20×28 visual; 44×44 hit target)
        {
            const float btnW = 20.0f;
            juce::Rectangle<float> r(x, btnY, btnW, static_cast<float>(kBtnHeight));
            presetPrevBounds_ = r;
            const float hitExpandH = std::max(0.0f, (44.0f - r.getWidth())  / 2.0f);
            const float hitExpandV = std::max(0.0f, (44.0f - r.getHeight()) / 2.0f);
            regions_.push_back({ r.expanded(hitExpandH, hitExpandV), kRegPresetPrev });
        }
    }

    //==========================================================================
    // paint()

    void paint(juce::Graphics& g) override
    {
        buildLayout();

        // --- Engines button ---
        paintTextButton(g, enginesBounds_, "ENGINES", kRegEngines,
                        false, /*withMenuIcon=*/true);

        // --- Undo icon button ---
        paintIconButton(g, undoBounds_, kRegUndo, false);
        paintUndoIcon(g, undoBounds_, /*isRedo=*/false);

        // --- Redo icon button ---
        paintIconButton(g, redoBounds_, kRegRedo, false);
        paintUndoIcon(g, redoBounds_, /*isRedo=*/true);

        // --- Preset navigation (centre) ---
        paintPresetNav(g);

        // --- Chain button (active state if chainModeActive_) ---
        paintChainButton(g);

        // --- Export button (text pill with export glyph icon) ---
        paintExportButton(g);

        // --- REACT label ---
        {
            static const juce::Font reactLabelFont(juce::FontOptions{}
                          .withName(juce::Font::getDefaultSansSerifFontName())
                          .withHeight(9.0f));
            g.setFont(reactLabelFont);
            g.setColour(juce::Colour(200, 204, 216).withAlpha(0.25f));
            g.drawText("REACT", reactLabelBounds_.toNearestInt(),
                       juce::Justification::centredRight, false);
        }

        // --- REACT dial ---
        paintReactDial(g);

        // --- Settings icon button ---
        paintIconButton(g, settingsBounds_, kRegSettings, false);
        paintGearIcon(g, settingsBounds_);
    }

    void resized() override
    {
        buildLayout();
    }

    //--------------------------------------------------------------------------
    // Button paint helpers

    /** Resolve the frosted-glass colours for a given region. */
    struct BtnColors
    {
        juce::Colour bg;
        juce::Colour border;
        juce::Colour text;
    };

    BtnColors resolveColors(int regionId, bool isActive) const
    {
        const bool isHov = (hoveredRegion_ == regionId);

        if (isActive)
        {
            return {
                juce::Colour(60, 180, 170).withAlpha(0.10f),
                juce::Colour(60, 180, 170).withAlpha(0.35f),
                juce::Colour(60, 180, 170).withAlpha(0.90f)
            };
        }

        if (isHov)
        {
            return {
                juce::Colour(200, 204, 216).withAlpha(0.08f),
                juce::Colour(200, 204, 216).withAlpha(0.18f),
                juce::Colour(200, 204, 216).withAlpha(0.85f)
            };
        }

        return {
            juce::Colour(14, 16, 22).withAlpha(0.70f),
            juce::Colour(200, 204, 216).withAlpha(0.10f),
            juce::Colour(200, 204, 216).withAlpha(0.55f)
        };
    }

    void paintTextButton(juce::Graphics& g,
                         const juce::Rectangle<float>& bounds,
                         const char* label,
                         int regionId,
                         bool isActive,
                         bool withMenuIcon)
    {
        const auto c = resolveColors(regionId, isActive);

        g.setColour(c.bg);
        g.fillRoundedRectangle(bounds, 8.0f);
        g.setColour(c.border);
        g.drawRoundedRectangle(bounds, 8.0f, 1.0f);

        static const juce::Font pillFont(juce::FontOptions{}
            .withName(juce::Font::getDefaultSansSerifFontName())
            .withStyle("Bold")
            .withHeight(11.0f));

        g.setFont(pillFont);
        g.setColour(c.text);

        if (withMenuIcon)
        {
            // Draw a small "≡" (three horizontal lines) on the left, then the label.
            const float cx  = bounds.getX() + 14.0f;
            const float cy  = bounds.getCentreY();
            const float lw  = 8.0f;
            const float gap = 2.5f;
            g.setColour(c.text);
            for (int i = -1; i <= 1; ++i)
            {
                g.fillRect(juce::Rectangle<float>(cx - lw * 0.5f,
                                                  cy + static_cast<float>(i) * gap - 0.5f,
                                                  lw, 1.0f));
            }
            // Label to the right of the icon.
            juce::Rectangle<float> textArea(bounds.getX() + 24.0f, bounds.getY(),
                                            bounds.getWidth() - 28.0f, bounds.getHeight());
            g.setColour(c.text);
            g.drawText(label, textArea.toNearestInt(), juce::Justification::centredLeft, false);
        }
        else
        {
            g.drawText(label, bounds.toNearestInt(), juce::Justification::centred, false);
        }
    }

    void paintIconButton(juce::Graphics& g,
                         const juce::Rectangle<float>& bounds,
                         int regionId,
                         bool isActive)
    {
        const auto c = resolveColors(regionId, isActive);

        g.setColour(c.bg);
        g.fillRoundedRectangle(bounds, 8.0f);
        g.setColour(c.border);
        g.drawRoundedRectangle(bounds, 8.0f, 1.0f);
    }

    //--------------------------------------------------------------------------
    // Preset navigation — ◀ [name] ▶ ♥ SAVE A/B (#1104)

    void paintPresetNav(juce::Graphics& g)
    {
        static const juce::Font navFont(juce::FontOptions{}
            .withName(juce::Font::getDefaultSansSerifFontName())
            .withStyle("Bold")
            .withHeight(11.0f));
        // I1a: UTF-8 em-dash — avoids Latin-1 mojibake when char* literal is used directly.
        static const juce::String kEmDashGlyph(juce::CharPointer_UTF8("\xe2\x80\x94"));

        // ◀ prev arrow
        {
            const auto c = resolveColors(kRegPresetPrev, false);
            g.setColour(c.bg);
            g.fillRoundedRectangle(presetPrevBounds_, 5.0f);
            g.setColour(c.text);
            g.setFont(navFont);
            g.drawText(juce::CharPointer_UTF8("\xe2\x97\x80"), presetPrevBounds_.toNearestInt(),
                       juce::Justification::centred, false);
        }

        // Preset name label (frosted pill)
        {
            const auto c = resolveColors(kRegPresetName, false);
            g.setColour(c.bg);
            g.fillRoundedRectangle(presetNameBounds_, 5.0f);
            g.setColour(juce::Colour(200, 204, 216).withAlpha(0.10f));
            g.drawRoundedRectangle(presetNameBounds_, 5.0f, 1.0f);
            g.setFont(navFont);
            g.setColour(juce::Colour(200, 204, 216).withAlpha(
                hoveredRegion_ == kRegPresetName ? 0.85f : 0.65f));
            g.drawText(presetName_.isEmpty() ? kEmDashGlyph : presetName_,
                       presetNameBounds_.toNearestInt(),
                       juce::Justification::centred, true);
        }

        // ▶ next arrow
        {
            const auto c = resolveColors(kRegPresetNext, false);
            g.setColour(c.bg);
            g.fillRoundedRectangle(presetNextBounds_, 5.0f);
            g.setColour(c.text);
            g.setFont(navFont);
            g.drawText(juce::CharPointer_UTF8("\xe2\x96\xb6"), presetNextBounds_.toNearestInt(),
                       juce::Justification::centred, false);
        }

        // ♥ favourite — teal when active, dim otherwise
        {
            const bool isHov = (hoveredRegion_ == kRegFav);
            const juce::Colour heartCol = isFav_
                ? juce::Colour(60, 180, 170).withAlpha(0.90f)
                : juce::Colour(200, 204, 216).withAlpha(isHov ? 0.75f : 0.40f);
            g.setColour(juce::Colour(14, 16, 22).withAlpha(0.70f));
            g.fillRoundedRectangle(favBounds_, 5.0f);
            g.setFont(navFont);
            g.setColour(heartCol);
            g.drawText(juce::CharPointer_UTF8("\xe2\x99\xa5"), favBounds_.toNearestInt(),
                       juce::Justification::centred, false);
        }

        // SAVE pill
        paintTextButton(g, saveBounds_, "SAVE", kRegSave, false, false);

        // A/B compare pill
        paintTextButton(g, abCompareBounds_, "A/B", kRegABCompare, abCompareActive_, false);
    }

    //--------------------------------------------------------------------------
    // Chain button — text pill with interlocking-oval icon

    void paintChainButton(juce::Graphics& g)
    {
        const auto c = resolveColors(kRegChain, chainModeActive_);

        g.setColour(c.bg);
        g.fillRoundedRectangle(chainBounds_, 8.0f);
        g.setColour(c.border);
        g.drawRoundedRectangle(chainBounds_, 8.0f, 1.0f);

        // Chain-link icon: two interlocking ovals drawn as a Path.
        // Left oval slightly to the left, right oval slightly to the right.
        const float iconCX = chainBounds_.getX() + 14.0f;
        const float iconCY = chainBounds_.getCentreY();
        const float ow     = 7.0f;  // oval full width
        const float oh     = 5.0f;  // oval full height
        const float offset = 2.5f;  // horizontal centre offset from icon centre

        g.setColour(c.text);

        // Left link
        {
            juce::Path oval;
            oval.addEllipse(iconCX - offset - ow * 0.5f,
                            iconCY - oh * 0.5f, ow, oh);
            g.strokePath(oval, juce::PathStrokeType(1.3f));
        }
        // Right link (overlaps left by ~offset)
        {
            juce::Path oval;
            oval.addEllipse(iconCX + offset - ow * 0.5f,
                            iconCY - oh * 0.5f, ow, oh);
            g.strokePath(oval, juce::PathStrokeType(1.3f));
        }

        // Label text
        static const juce::Font pillFont(juce::FontOptions{}
            .withName(juce::Font::getDefaultSansSerifFontName())
            .withStyle("Bold")
            .withHeight(11.0f));

        g.setFont(pillFont);
        g.setColour(c.text);

        juce::Rectangle<float> textArea(chainBounds_.getX() + 24.0f, chainBounds_.getY(),
                                        chainBounds_.getWidth() - 28.0f, chainBounds_.getHeight());
        g.drawText("CHAIN", textArea.toNearestInt(), juce::Justification::centredLeft, false);
    }

    //--------------------------------------------------------------------------
    // Undo / Redo icon — via HudIcons factory (standard juce::Path glyphs)

    void paintUndoIcon(juce::Graphics& g,
                       const juce::Rectangle<float>& bounds,
                       bool isRedo)
    {
        const bool isHov = (hoveredRegion_ == (isRedo ? kRegRedo : kRegUndo));
        const juce::Colour col = isHov
            ? juce::Colour(200, 204, 216).withAlpha(0.85f)
            : juce::Colour(200, 204, 216).withAlpha(0.55f);

        // Inset bounds slightly so the icon sits inside the 32×32 hit region.
        const auto iconBounds = bounds.reduced(7.0f, 7.0f);
        const auto icon = isRedo ? HudIcons::makeRedoIcon() : HudIcons::makeUndoIcon();

        HudIcons::drawIconInBounds(g, icon, iconBounds, col, 1.5f);
    }

    //--------------------------------------------------------------------------
    // Gear icon — via HudIcons factory (standard 8-tooth gear with centre hole)

    void paintGearIcon(juce::Graphics& g,
                       const juce::Rectangle<float>& bounds)
    {
        const bool isHov = (hoveredRegion_ == kRegSettings);
        const juce::Colour col = isHov
            ? juce::Colour(200, 204, 216).withAlpha(0.85f)
            : juce::Colour(200, 204, 216).withAlpha(0.55f);

        // Inset so the gear sits neatly inside the 32×32 hit region.
        const auto iconBounds = bounds.reduced(6.0f, 6.0f);

        // Fill gear body.
        HudIcons::drawIconInBounds(g, HudIcons::makeGearIcon(), iconBounds, col,
                                   /*strokeW=*/0.0f); // filled

        // Punch centre hole with button background colour (tracks hover/active state).
        const auto holeBg = resolveColors(kRegSettings, false).bg;
        HudIcons::drawIconInBounds(g, HudIcons::makeGearHole(), iconBounds,
                                   holeBg,
                                   /*strokeW=*/0.0f);
    }

    //--------------------------------------------------------------------------
    // Export button — text pill with export glyph icon (down-arrow into tray)

    void paintExportButton(juce::Graphics& g)
    {
        const auto c = resolveColors(kRegExport, false);

        g.setColour(c.bg);
        g.fillRoundedRectangle(exportBounds_, 8.0f);
        g.setColour(c.border);
        g.drawRoundedRectangle(exportBounds_, 8.0f, 1.0f);

        // Export glyph on the left of the pill.
        const float iconSz = 10.0f;
        const float iconX  = exportBounds_.getX() + 7.0f;
        const float iconY  = exportBounds_.getCentreY() - iconSz * 0.5f;
        HudIcons::drawIconInBounds(g, HudIcons::makeExportIcon(),
                                   juce::Rectangle<float>(iconX, iconY, iconSz, iconSz),
                                   c.text, 1.3f);

        // Label text to the right of the icon.
        static const juce::Font pillFont(juce::FontOptions{}
            .withName(juce::Font::getDefaultSansSerifFontName())
            .withStyle("Bold")
            .withHeight(11.0f));
        g.setFont(pillFont);
        g.setColour(c.text);
        juce::Rectangle<float> textArea(exportBounds_.getX() + 22.0f, exportBounds_.getY(),
                                        exportBounds_.getWidth() - 26.0f, exportBounds_.getHeight());
        g.drawText("EXPORT", textArea.toNearestInt(), juce::Justification::centredLeft, false);
    }

    //--------------------------------------------------------------------------
    // REACT rotary dial — same style as SAT/DELAY/REVERB dials in MasterFXStripCompact

    void paintReactDial(juce::Graphics& g)
    {
        using juce::MathConstants;
        using juce::Colour;
        using juce::Path;
        using juce::PathStrokeType;

        const bool isHov = (hoveredRegion_ == kRegDial);

        const float cx = reactDialBounds_.getCentreX();
        const float cy = reactDialBounds_.getCentreY();
        const float r  = (reactDialBounds_.getWidth() * 0.5f) - 2.0f;

        const float startAngle = 0.75f * MathConstants<float>::pi;  // 135°
        const float endAngle   = 2.25f * MathConstants<float>::pi;  // 405°
        const float totalSweep = endAngle - startAngle;             // 270°
        const float valueAngle = startAngle + reactLevel_ * totalSweep;

        // Background arc (full sweep).
        {
            Path bgArc;
            bgArc.addCentredArc(cx, cy, r, r, 0.0f, startAngle, endAngle, true);
            g.setColour(Colour(200, 204, 216).withAlpha(0.08f));
            g.strokePath(bgArc, PathStrokeType(3.0f,
                PathStrokeType::curved, PathStrokeType::rounded));
        }

        // Value arc (partial sweep).
        if (reactLevel_ > 0.01f)
        {
            Path valArc;
            valArc.addCentredArc(cx, cy, r, r, 0.0f, startAngle, valueAngle, true);
            g.setColour(Colour(127, 219, 202).withAlpha(0.60f));
            g.strokePath(valArc, PathStrokeType(3.0f,
                PathStrokeType::curved, PathStrokeType::rounded));
        }

        // Centre dot.
        const float dotR = r * 0.35f;
        g.setColour(Colour(200, 204, 216).withAlpha(0.06f));
        g.fillEllipse(cx - dotR, cy - dotR, dotR * 2.0f, dotR * 2.0f);
        g.setColour(Colour(200, 204, 216).withAlpha(0.10f));
        g.drawEllipse(cx - dotR, cy - dotR, dotR * 2.0f, dotR * 2.0f, 1.0f);

        // Indicator line — brighter on hover.
        const float indStart = r * 0.25f;
        const float indEnd   = r * 0.75f;
        const float indAlpha = isHov ? 1.0f : 0.80f;
        const float cosA     = std::cos(valueAngle);
        const float sinA     = std::sin(valueAngle);
        g.setColour(Colour(127, 219, 202).withAlpha(indAlpha));
        g.drawLine(cx + indStart * cosA, cy + indStart * sinA,
                   cx + indEnd   * cosA, cy + indEnd   * sinA,
                   1.5f);
    }

    //==========================================================================
    // Mouse handling

    int hitRegion(const juce::MouseEvent& e) const
    {
        const float mx = static_cast<float>(e.x);
        const float my = static_cast<float>(e.y);

        for (const auto& reg : regions_)
        {
            if (reg.bounds.expanded(2.0f).contains(mx, my))
                return reg.id;
        }
        return -1;
    }

    void mouseDown(const juce::MouseEvent& e) override
    {
        const int id = hitRegion(e);

        switch (id)
        {
            case kRegEngines:
                if (onEnginesClicked)
                    onEnginesClicked();
                break;

            case kRegUndo:
                if (onUndo)
                    onUndo();
                break;

            case kRegRedo:
                if (onRedo)
                    onRedo();
                break;

            case kRegPresetPrev:
                if (onPresetPrev) onPresetPrev();
                break;

            case kRegPresetName:
                if (onPresetNameClicked) onPresetNameClicked();
                break;

            case kRegPresetNext:
                if (onPresetNext) onPresetNext();
                break;

            case kRegFav:
                isFav_ = !isFav_;
                repaint();
                if (onFavChanged) onFavChanged(isFav_);
                break;

            case kRegSave:
                if (onSave) onSave();
                break;

            case kRegABCompare:
                abCompareActive_ = !abCompareActive_;
                repaint();
                if (onABCompareChanged) onABCompareChanged(abCompareActive_);
                break;

            case kRegChain:
                chainModeActive_ = !chainModeActive_;
                repaint();
                if (onChainToggled)
                    onChainToggled();
                break;

            case kRegExport:
                if (onExportClicked)
                    onExportClicked();
                break;

            case kRegDial:
                dialDragging_   = true;
                dialDragStartY_ = e.y;
                dialDragStartV_ = reactLevel_;
                break;

            case kRegSettings:
                if (onSettingsClicked)
                    onSettingsClicked();
                break;

            default:
                break;
        }
    }

    void mouseDrag(const juce::MouseEvent& e) override
    {
        if (!dialDragging_)
            return;

        // Map vertical delta to 0–1. Drag up = increase. 100 px = full sweep.
        const float dy    = static_cast<float>(dialDragStartY_ - e.y);
        const float range = 100.0f;
        const float newV  = juce::jlimit(0.0f, 1.0f,
                                         dialDragStartV_ + dy / range);
        reactLevel_ = newV;
        repaint();

        if (onReactChanged)
            onReactChanged(reactLevel_);
    }

    void mouseUp(const juce::MouseEvent& /*e*/) override
    {
        dialDragging_ = false;
        repaint();
    }

    void mouseMove(const juce::MouseEvent& e) override
    {
        const int id = hitRegion(e);

        if (id != hoveredRegion_)
        {
            hoveredRegion_ = id;
            repaint();
        }
    }

    void mouseExit(const juce::MouseEvent& /*e*/) override
    {
        if (hoveredRegion_ != -1)
        {
            hoveredRegion_ = -1;
            repaint();
        }
    }

    //==========================================================================
    // State

    bool  chainModeActive_  = false;
    float reactLevel_       = 0.80f; // default 80% reactivity

    // Preset navigation state (#1104)
    juce::String presetName_;
    bool         isFav_           = false;
    bool         abCompareActive_ = false;

    // Dial drag state (REACT rotary)
    bool  dialDragging_   = false;
    int   dialDragStartY_ = 0;
    float dialDragStartV_ = 0.0f;

    // Hover tracking
    int hoveredRegion_ = -1;

    // Layout rects — rebuilt each buildLayout() call.
    juce::Rectangle<float> enginesBounds_;
    juce::Rectangle<float> undoBounds_;
    juce::Rectangle<float> redoBounds_;
    juce::Rectangle<float> presetPrevBounds_;
    juce::Rectangle<float> presetNameBounds_;
    juce::Rectangle<float> presetNextBounds_;
    juce::Rectangle<float> favBounds_;
    juce::Rectangle<float> saveBounds_;
    juce::Rectangle<float> abCompareBounds_;
    juce::Rectangle<float> chainBounds_;
    juce::Rectangle<float> exportBounds_;
    juce::Rectangle<float> reactLabelBounds_;
    juce::Rectangle<float> reactDialBounds_;
    juce::Rectangle<float> settingsBounds_;

    // Hit-test regions
    std::vector<HudRegion> regions_;

    //==========================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SubmarineHudBar)
};

} // namespace xoceanus
