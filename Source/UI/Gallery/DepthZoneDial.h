// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
// DepthZoneDial.h — Circular engine browser dial for the XOceanus header.
//
// A 48×48pt circular dial that lets the user browse engines by dragging
// clockwise/counterclockwise through the water-column depth order:
//   Sunlit (bright, melodic) → Twilight (textured) → Midnight (experimental)
//
// Visual design:
//   Outer ring (48pt): Radial gradient segmented by depth zone:
//     Top arc    — Sunlit  #48CAE4 (cyan)
//     Middle arc — Twilight #0096C7 (blue)
//     Bottom arc — Midnight #7B2FBE (violet)
//   Inner disc (36pt): Engine accent color at 80% opacity.
//   Center text: Engine short name (Space Grotesk Bold 9pt, ≤5 chars).
//   Position dot (4pt): Bright dot on outer ring showing depth-zone position.
//
// Interaction:
//   Drag CW/CCW: Accumulate angle, snap to next/prev engine at 15° intervals.
//   Click:       Open EnginePickerPopup in a CallOutBox.
//   Mouse wheel: Scroll through engines (up = next, down = prev).
//
// Usage:
//   auto dial = std::make_unique<DepthZoneDial>(processor);
//   dial->setSlot(0);
//   dial->onEngineSelected = [this](const juce::String& id) {
//       processor.loadEngine(currentSlot, id.toStdString());
//   };

#include <juce_audio_processors/juce_audio_processors.h>
#include "../../XOceanusProcessor.h"
#include "../../Core/EngineRegistry.h"
#include "../GalleryColors.h"
#include "EnginePickerPopup.h"

namespace xoceanus {

//==============================================================================
class DepthZoneDial : public juce::Component,
                      private juce::Timer
{
public:
    //==========================================================================
    // Fires when the user selects an engine via the dial. Caller must call
    // processor.loadEngine(slot, id.toStdString()) to commit the change.
    std::function<void(const juce::String& engineId)> onEngineSelected;

    //==========================================================================
    explicit DepthZoneDial(XOceanusProcessor& proc)
        : processor(proc)
    {
        A11y::setup(*this,
                    "Depth Zone Dial",
                    "Drag or scroll to browse engines in water-column order. "
                    "Click to open the full engine picker.");

        buildEngineOrder();
        setSize(kDialSize, kDialSize);

        // Poll at 4 Hz to detect external slot changes (e.g. preset load).
        startTimerHz(4);
    }

    ~DepthZoneDial() override { stopTimer(); }

    //==========================================================================
    // Set the slot index this dial controls (0–4, where 4 is the Ghost Slot). W17: clamp now includes index 4.
    void setSlot(int slot)
    {
        currentSlot = juce::jlimit(0, 4, slot); // W17: extended to include Ghost Slot (index 4)
        refresh();
    }

    // Sync display state from the processor (call after any external engine change).
    void refresh()
    {
        auto* eng = processor.getEngine(currentSlot);

        juce::String newId      = eng ? juce::String(eng->getEngineId()) : juce::String{};
        juce::Colour newAccent  = eng ? eng->getAccentColour()
                                      : GalleryColors::get(GalleryColors::emptySlot());

        if (newId == currentEngineId && newAccent == currentAccent)
            return; // nothing changed — skip repaint

        currentEngineId = newId;
        currentAccent   = newAccent;
        // P7 fix: mark ring cache dirty so paint() will rebuild it once.
        ringCacheDirty = true;

        // Re-locate within engineOrder so the position dot is correct.
        currentIndex = -1;
        if (currentEngineId.isNotEmpty())
        {
            for (int i = 0; i < (int)engineOrder.size(); ++i)
            {
                if (engineOrder[static_cast<size_t>(i)].equalsIgnoreCase(currentEngineId))
                {
                    currentIndex = i;
                    break;
                }
            }
        }

        repaint();
    }

    //==========================================================================
    // juce::Component overrides
    //==========================================================================
    void paint(juce::Graphics& g) override
    {
        const float cx    = getWidth()  * 0.5f;
        const float cy    = getHeight() * 0.5f;
        const float outer = kDialSize   * 0.5f;           // 24pt
        const float inner = kInnerSize  * 0.5f;           // 18pt
        const float ring  = outer - inner;                 // 6pt ring width

        using namespace GalleryColors;

        // ── Outer ring — depth-zone radial gradient ──────────────────────────
        // P7 fix: cache the ring as a juce::Image — the gradient + border never
        // change between engine selections. Rebuild only when ringCacheDirty is
        // set (in refresh()). In steady-state, paint() just blits the image.
        if (ringCacheDirty || !ringCache.isValid())
        {
            // Rebuild the ring into an off-screen image at device pixel resolution.
            ringCache = juce::Image(juce::Image::ARGB,
                                    getWidth() > 0 ? getWidth() : kDialSize,
                                    getHeight() > 0 ? getHeight() : kDialSize,
                                    true /* clearImage */);
            juce::Graphics rg(ringCache);

            const float rcx = ringCache.getWidth()  * 0.5f;
            const float rcy = ringCache.getHeight() * 0.5f;

            // Outer ring clip path
            juce::Path ringPath;
            ringPath.addEllipse(rcx - outer, rcy - outer, outer * 2.0f, outer * 2.0f);
            ringPath.addEllipse(rcx - inner, rcy - inner, inner * 2.0f, inner * 2.0f);
            ringPath.setUsingNonZeroWinding(false); // subtract inner hole

            {
                juce::Graphics::ScopedSaveState ss(rg);
                rg.reduceClipRegion(ringPath);

                // Multi-stop gradient: Sunlit (top) → Twilight (mid) → Midnight (bottom)
                juce::ColourGradient grad(
                    kSunlitColor,   rcx, rcy - outer,
                    kMidnightColor, rcx, rcy + outer,
                    false);
                grad.addColour(0.5, kTwilightColor);
                rg.setGradientFill(grad);
                rg.fillRect(juce::Rectangle<float>(0.0f, 0.0f,
                                                   (float)ringCache.getWidth(),
                                                   (float)ringCache.getHeight()));
            }

            // Subtle border on the outer ring edge
            rg.setColour(juce::Colours::black.withAlpha(0.18f));
            rg.drawEllipse(rcx - outer, rcy - outer, outer * 2.0f, outer * 2.0f, 1.0f);

            ringCacheDirty = false;
        }

        // Blit cached ring image
        g.drawImageAt(ringCache, 0, 0);

        // ── Inner disc — engine accent color ─────────────────────────────────
        const bool hasEngine = currentEngineId.isNotEmpty();

        juce::Colour discFill = hasEngine
            ? currentAccent.withAlpha(0.82f)
            : get(slotBg()).withAlpha(0.92f);

        // Soft inner shadow / depth ring between outer ring and disc
        {
            juce::ColourGradient shadow(
                juce::Colours::black.withAlpha(0.22f), cx, cy - inner,
                juce::Colours::transparentBlack,        cx, cy,
                true); // radial
            juce::Graphics::ScopedSaveState ss(g);
            g.reduceClipRegion(juce::Rectangle<float>(
                cx - inner - 2.0f, cy - inner - 2.0f,
                (inner + 2.0f) * 2.0f, (inner + 2.0f) * 2.0f).toNearestInt());
            g.setGradientFill(shadow);
            g.fillEllipse(cx - inner - 2.0f, cy - inner - 2.0f,
                          (inner + 2.0f) * 2.0f, (inner + 2.0f) * 2.0f);
        }

        g.setColour(discFill);
        g.fillEllipse(cx - inner, cy - inner, inner * 2.0f, inner * 2.0f);

        // Specular highlight at top of inner disc (glass-dome effect)
        {
            juce::ColourGradient spec(
                juce::Colours::white.withAlpha(0.22f), cx, cy - inner + 2.0f,
                juce::Colours::transparentWhite,        cx, cy,
                false);
            juce::Graphics::ScopedSaveState ss(g);
            juce::Path innerClip;
            innerClip.addEllipse(cx - inner, cy - inner, inner * 2.0f, inner * 2.0f);
            g.reduceClipRegion(innerClip);
            g.setGradientFill(spec);
            g.fillRect(cx - inner, cy - inner, inner * 2.0f, inner);
        }

        // Border around inner disc
        g.setColour(juce::Colours::black.withAlpha(0.12f));
        g.drawEllipse(cx - inner, cy - inner, inner * 2.0f, inner * 2.0f, 1.0f);

        // ── Center label — engine short name ─────────────────────────────────
        const juce::String label = hasEngine
            ? currentEngineId.substring(0, 5).toUpperCase()
            : juce::String(L"\u2014"); // em dash

        // Ensure legibility: use white on dark accents, dark on light accents.
        juce::Colour labelColor = [&]() -> juce::Colour
        {
            if (!hasEngine)
                return get(textMid()).withAlpha(0.55f);
            // Luminance-adaptive: threshold ~0.35
            const float lum = discFill.getPerceivedBrightness();
            return lum > 0.50f
                ? juce::Colours::black.withAlpha(0.75f)
                : juce::Colours::white.withAlpha(0.92f);
        }();

        g.setFont(GalleryFonts::display(9.0f));
        g.setColour(labelColor);
        g.drawText(label,
                   (int)(cx - inner), (int)(cy - inner),
                   (int)(inner * 2.0f), (int)(inner * 2.0f),
                   juce::Justification::centred, true);

        // ── Position dot on outer ring ────────────────────────────────────────
        // The dot travels around the ring to show where in the water column
        // the current engine sits.  It always stays in the middle of the ring.
        if (currentIndex >= 0 && !engineOrder.empty())
        {
            const float fraction = static_cast<float>(currentIndex)
                                   / static_cast<float>(engineOrder.size());
            // fraction 0.0 = Sunlit top, travels CW to 1.0 = back to top.
            const float dotAngle = -juce::MathConstants<float>::halfPi
                                   + fraction * juce::MathConstants<float>::twoPi;
            const float dotR     = inner + ring * 0.5f; // midpoint of ring
            const float dotCx    = cx + dotR * std::cos(dotAngle);
            const float dotCy    = cy + dotR * std::sin(dotAngle);

            // Bright dot — colour is derived from the zone at this position.
            juce::Colour dotColor = zoneColorForFraction(fraction).brighter(0.6f);
            g.setColour(juce::Colours::black.withAlpha(0.28f));
            g.fillEllipse(dotCx - kDotRadius - 0.5f, dotCy - kDotRadius - 0.5f,
                          (kDotRadius + 0.5f) * 2.0f, (kDotRadius + 0.5f) * 2.0f);

            g.setColour(dotColor);
            g.fillEllipse(dotCx - kDotRadius, dotCy - kDotRadius,
                          kDotRadius * 2.0f, kDotRadius * 2.0f);

            // Fine white inner specular
            g.setColour(juce::Colours::white.withAlpha(0.55f));
            g.fillEllipse(dotCx - kDotRadius * 0.45f,
                          dotCy - kDotRadius * 0.60f,
                          kDotRadius * 0.90f,
                          kDotRadius * 0.70f);
        }

        // ── Hover state: brighten ring slightly ───────────────────────────────
        if (isMouseOver() && !isDragging)
        {
            g.setColour(juce::Colours::white.withAlpha(0.06f));
            g.fillEllipse(cx - outer, cy - outer, outer * 2.0f, outer * 2.0f);
        }

        // ── Keyboard focus ring ───────────────────────────────────────────────
        if (hasKeyboardFocus(false))
            A11y::drawCircularFocusRing(g, cx, cy, outer + 2.0f);
    }

    void resized() override
    {
        // Fixed 48×48pt — nothing to lay out.
        // P7 fix: invalidate ring cache if component is ever resized.
        ringCacheDirty = true;
    }

    //==========================================================================
    // Mouse events
    //==========================================================================
    void mouseDown(const juce::MouseEvent& e) override
    {
        // Left-click with no drag → open EnginePickerPopup.
        // We set a flag and confirm on mouseUp only if drag didn't occur.
        dragStartAngle     = angleFromEvent(e);
        accumulatedDelta   = 0.0f;
        isDragging         = false;
        repaint();
    }

    void mouseDrag(const juce::MouseEvent& e) override
    {
        const float angle = angleFromEvent(e);
        float delta = angle - dragStartAngle;

        // Wrap delta into [-π, π] to handle the ±π discontinuity.
        while (delta >  juce::MathConstants<float>::pi) delta -= juce::MathConstants<float>::twoPi;
        while (delta < -juce::MathConstants<float>::pi) delta += juce::MathConstants<float>::twoPi;

        accumulatedDelta += delta;
        dragStartAngle    = angle;

        // Accumulate drag until threshold, then advance one step.
        const float kStep = juce::degreesToRadians(kSnapDegrees);
        while (accumulatedDelta >= kStep)
        {
            accumulatedDelta -= kStep;
            advanceEngine(+1);
            isDragging = true;
        }
        while (accumulatedDelta <= -kStep)
        {
            accumulatedDelta += kStep;
            advanceEngine(-1);
            isDragging = true;
        }

        repaint();
    }

    void mouseUp(const juce::MouseEvent& e) override
    {
        if (!isDragging && e.mouseWasClicked())
            openEnginePickerPopup();

        isDragging = false;
        repaint();
    }

    void mouseWheelMove(const juce::MouseEvent&,
                        const juce::MouseWheelDetails& w) override
    {
        // Wheel up = previous (shallower/sunlit), wheel down = next (deeper/midnight).
        // We use a small accumulator to avoid over-firing on trackpad scroll.
        wheelAccumulator += w.deltaY;

        while (wheelAccumulator >= kWheelThreshold)
        {
            wheelAccumulator -= kWheelThreshold;
            advanceEngine(-1); // up = shallower
        }
        while (wheelAccumulator <= -kWheelThreshold)
        {
            wheelAccumulator += kWheelThreshold;
            advanceEngine(+1); // down = deeper
        }
    }

    bool keyPressed(const juce::KeyPress& key) override
    {
        if (key == juce::KeyPress::rightKey || key == juce::KeyPress::downKey)
        {
            advanceEngine(+1);
            return true;
        }
        if (key == juce::KeyPress::leftKey || key == juce::KeyPress::upKey)
        {
            advanceEngine(-1);
            return true;
        }
        if (key == juce::KeyPress::returnKey || key == juce::KeyPress::spaceKey)
        {
            openEnginePickerPopup();
            return true;
        }
        return false;
    }

private:
    //==========================================================================
    // juce::Timer — poll for external slot changes at 4 Hz
    void timerCallback() override { refresh(); }

    //==========================================================================
    // Advance the engine browser by delta (+1 = deeper/next, -1 = shallower/prev).
    void advanceEngine(int delta)
    {
        if (engineOrder.empty())
            return;

        const int n = static_cast<int>(engineOrder.size());

        // If no current engine, start from index 0.
        int nextIndex = (currentIndex < 0) ? 0
                                           : currentIndex + delta;

        // Wrap around the full engine order list.
        nextIndex = ((nextIndex % n) + n) % n;

        const juce::String newId = engineOrder[static_cast<size_t>(nextIndex)];
        if (newId == currentEngineId)
            return;

        currentEngineId = newId;
        currentIndex    = nextIndex;
        currentAccent   = GalleryColors::accentForEngine(currentEngineId);

        if (onEngineSelected)
            onEngineSelected(currentEngineId);

        repaint();
    }

public:
    //==========================================================================
    // Open the full engine picker in a CallOutBox.
    void openEnginePickerPopup()
    {
        auto* picker = new EnginePickerPopup();

        // Pre-select the current engine in the popup list.
        // (EnginePickerPopup focuses search on open; user can type to filter.)

        picker->onEngineSelected = [this](const juce::String& id)
        {
            if (id == currentEngineId)
                return;
            currentEngineId = id;
            currentAccent   = GalleryColors::accentForEngine(id);

            // Re-locate in order list.
            currentIndex = -1;
            for (int i = 0; i < (int)engineOrder.size(); ++i)
            {
                if (engineOrder[static_cast<size_t>(i)].equalsIgnoreCase(id))
                {
                    currentIndex = i;
                    break;
                }
            }

            if (onEngineSelected)
                onEngineSelected(id);

            repaint();
        };

        juce::Rectangle<int> target = getScreenBounds();
        juce::CallOutBox::launchAsynchronously(
            std::unique_ptr<juce::Component>(picker),
            target,
            nullptr);
    }

private:
    //==========================================================================
    // Build the water-column engine order:
    //   1. All Sunlit  engines (depthZone == 0) — alphabetical
    //   2. All Twilight engines (depthZone == 1) — alphabetical
    //   3. All Midnight engines (depthZone == 2) — alphabetical
    //
    // Uses the same metadata table as EnginePickerPopup. Engines not in the
    // table default to Twilight (zone 1), consistent with EnginePickerPopup.
    void buildEngineOrder()
    {
        engineOrder.clear();

        // Collect IDs from live registry.
        juce::StringArray allIds;
        for (const auto& stdId : EngineRegistry::instance().getRegisteredIds())
            allIds.add(juce::String(stdId.c_str()));

        // Sort each zone bucket alphabetically.
        juce::StringArray zones[3];
        for (const auto& id : allIds)
        {
            int z = depthZoneOf(id);
            zones[juce::jlimit(0, 2, z)].add(id);
        }
        for (auto& bucket : zones)
            bucket.sort(true); // case-insensitive alphabetical

        // Flatten: Sunlit → Twilight → Midnight.
        for (const auto& bucket : zones)
            for (const auto& id : bucket)
                engineOrder.push_back(id);
    }

    //==========================================================================
    // Depth-zone lookup — mirrors EnginePickerPopup::engineMetadataTable().
    static int depthZoneOf(const juce::String& engineId)
    {
        // This table mirrors the zone assignments in EnginePickerPopup exactly.
        // Zone 0 = Sunlit | Zone 1 = Twilight | Zone 2 = Midnight
        static const std::pair<const char*, int> kZoneTable[] = {
            { "Oto",        0 }, { "Octave",    0 }, { "Oleg",      0 }, { "Otis",      0 },
            { "Obelisk",    0 }, { "Orchard",   0 }, { "Osier",     0 },
            { "Overwash",   0 }, { "Overworld", 0 },
            { "Oasis",      0 }, { "OddfeliX",  0 }, { "OddOscar",  0 },
            { "Ohm",        0 }, { "Optic",     0 }, { "Opensky",   0 },
            // Twilight (1)
            { "Oven",       1 }, { "Ochre",     1 }, { "Opaline",   1 },
            { "Olate",      1 }, { "Oaken",     1 },
            { "Overgrow",   1 }, { "Oxalis",    1 },
            { "Overworn",   1 }, { "Overcast",  1 },
            { "Oddfellow",  1 }, { "Onkolo",    1 }, { "Opcode",    1 },
            { "Onset",      1 }, { "Offering",  1 }, { "Oware",     1 }, { "Ostinato",  1 },
            { "Opera",      1 }, { "Obbligato", 1 },
            { "Oblong",     1 }, { "Obese",     1 },
            { "Organon",    1 }, { "Ottoni",    1 }, { "Ole",       1 },
            { "Orphica",    1 }, { "Osprey",    1 }, { "Osteria",   1 },
            { "Opal",       1 }, { "Orbital",   1 }, { "Origami",   1 },
            { "Obscura",    1 }, { "Oblique",   1 }, { "Organism",  1 },
            { "Overtone",   1 }, { "Outlook",   1 },
            { "Oceanic",    1 }, { "Ocelot",    1 },
            { "Ombre",      1 }, { "Odyssey",   1 }, { "Overdub",   1 },
            { "Osmosis",    1 }, { "Outwit",    1 },
            // Midnight (2)
            { "Ogre",       2 }, { "Omega",     2 },
            { "Overflow",   2 },
            { "Obrix",      2 }, { "Oxytocin",  2 }, { "Overbite",  2 },
            { "Ouroboros",  2 }, { "Oracle",    2 },
            { "Obsidian",   2 }, { "Orbweave",  2 }, { "Oxbow",     2 },
            { "Orca",       2 }, { "Octopus",   2 },
            { "Owlfish",    2 }, { "Overlap",   2 },
            { "Oceandeep",  2 }, { "Ouie",      2 },
            { nullptr,      0 },  // sentinel
        };

        const juce::String lower = engineId.toLowerCase();
        for (int i = 0; kZoneTable[i].first != nullptr; ++i)
        {
            if (lower == juce::String(kZoneTable[i].first).toLowerCase())
                return kZoneTable[i].second;
        }
        return 1; // default Twilight (matches EnginePickerPopup)
    }

    //==========================================================================
    // Derive the zone color for a fraction [0, 1] around the ring.
    // 0.0 = Sunlit top, 0.5 = Midnight bottom, 1.0 = back to top.
    static juce::Colour zoneColorForFraction(float fraction)
    {
        // Clamp and map: 0→Sunlit, 0.33→Twilight, 0.66→Midnight, 1→Sunlit
        const float f = juce::jlimit(0.0f, 1.0f, fraction);
        if (f < 0.333f)
            return kSunlitColor.interpolatedWith(kTwilightColor, f / 0.333f);
        if (f < 0.667f)
            return kTwilightColor.interpolatedWith(kMidnightColor, (f - 0.333f) / 0.333f);
        return kMidnightColor.interpolatedWith(kSunlitColor, (f - 0.667f) / 0.333f);
    }

    //==========================================================================
    // Compute the angle from the component centre to the mouse event position.
    float angleFromEvent(const juce::MouseEvent& e) const
    {
        const float dx = e.position.x - getWidth()  * 0.5f;
        const float dy = e.position.y - getHeight() * 0.5f;
        return std::atan2(dy, dx);
    }

    //==========================================================================
    // Constants
    static constexpr int   kDialSize     = 48;
    static constexpr int   kInnerSize    = 36;
    static constexpr float kDotRadius    = 2.0f;
    static constexpr float kSnapDegrees  = 15.0f;
    static constexpr float kWheelThreshold = 0.15f;

    // Depth-zone palette (matches EnginePickerPopup zone colours exactly).
    // inline static const (C++17) avoids ODR issues in header-only files while
    // sidestepping the constexpr question for non-trivially-constructible types.
    inline static const juce::Colour kSunlitColor   { 0xFF48CAE4u };
    inline static const juce::Colour kTwilightColor { 0xFF0096C7u };
    inline static const juce::Colour kMidnightColor { 0xFF7B2FBEu };

    //==========================================================================
    // State
    XOceanusProcessor& processor;

    int          currentSlot      = 0;
    juce::String currentEngineId;
    juce::Colour currentAccent    { GalleryColors::get(GalleryColors::emptySlot()) };

    // Sorted engine list for CW/CCW browsing
    std::vector<juce::String> engineOrder;
    int currentIndex = -1;

    // Drag state
    float dragStartAngle   = 0.0f;
    float accumulatedDelta = 0.0f;
    float wheelAccumulator = 0.0f;
    bool  isDragging       = false;

    // P7 fix: cached ring image — rebuilt only when engine changes (ringCacheDirty=true)
    juce::Image ringCache;
    bool        ringCacheDirty = true;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DepthZoneDial)
};

} // namespace xoceanus
