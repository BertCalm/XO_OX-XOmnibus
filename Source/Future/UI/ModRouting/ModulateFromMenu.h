// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
// Wave 5 A3 — right-click "Modulate from…" context menu (Bitwig-style).
//
// Usage pattern (from inside any knob / parameter component):
//
//   void mouseDown(const juce::MouseEvent& e) override
//   {
//       if (e.mods.isRightButtonDown())
//       {
//           ModulateFromMenu::show(modModel, myParamId, this);
//           return;
//       }
//       // ... normal knob handling
//   }
//
// When a source is picked, a new route is added to the model at depth 0.5
// (or 0.3 for unipolar sources). Existing routes for the same (source, dest)
// pair surface a depth-adjust dialog instead of creating a duplicate.
//
// TODO Wave5-A3 mount: Callers that want the right-click menu need to:
//   1. Hold a reference to a ModRoutingModel (passed from the editor).
//   2. Call ModulateFromMenu::show(model, paramId, this) from their mouseDown
//      when e.mods.isRightButtonDown(). No component subclass is required.
//
// ────────────────────────────────────────────────────────────────────────────
// Extended source list (D9 F4 + G3 spec)
//
// ModSourceId in ModSourceHandle.h now defines all 18 sources (IDs 0–17)
// matching spec D9 F4 + G3. All entries in kAllModSources below have valid
// enum values and are routable via ModRoutingModel. The "extended" sentinel
// logic (id >= Count) no longer applies — Count is now 18.
//
#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "ModSourceHandle.h"
#include "DragDropModRouter.h"
#include "UI/GalleryColors.h"

namespace xoceanus
{

//==============================================================================
// ExtModSourceInfo — metadata for a single source entry in the popup menu.
struct ExtModSourceInfo
{
    int         id;          // cast to ModSourceId if id < Count, else extended
    const char* label;       // short display label (e.g. "LFO 1")
    const char* group;       // section header (nullptr = continue current section)
    bool        bipolar;     // true = source generates ±1 range
    uint32_t    colour;      // 0xAARRGGBB accent colour
};

//==============================================================================
// Full source catalogue — matches spec D9 F4 + G3.
// All 18 sources have valid ModSourceId enum values (Count = 18).
// All are routable via ModRoutingModel (which stores int sourceId).
//
// JUCE PopupMenu item IDs start at 1.  We encode id + 1 as the JUCE item ID
// so 0 remains the "nothing selected" sentinel.
//
static const ExtModSourceInfo kAllModSources[] = {
    // ── Oscillator modulators ─────────────────────────────────────────────
    { 0,  "LFO 1",           "LFOs",      true,  0xFF00CED1 },
    { 1,  "LFO 2",           nullptr,     true,  0xFFA8D8EA },
    { 6,  "LFO 3",           nullptr,     true,  0xFF7EC8E3 },

    // ── Envelopes ─────────────────────────────────────────────────────────
    { 2,  "ENV 1 (Amp)",     "Envelopes", true,  0xFFE8701A },
    { 7,  "ENV 2",           nullptr,     true,  0xFFFFAA55 },

    // ── Macros ────────────────────────────────────────────────────────────
    { 8,  "Macro: TONE",     "Macros",    false, 0xFFE9C46A },
    { 9,  "Macro: TIDE",     nullptr,     false, 0xFF7FDBCA },
    { 10, "Macro: COUPLE",   nullptr,     false, 0xFFFF8A65 },
    { 11, "Macro: DEPTH",    nullptr,     false, 0xFF9B89D4 },

    // ── Performance / MIDI ───────────────────────────────────────────────
    { 3,  "Velocity",        "MIDI",      false, 0xFFC6E377 },
    { 4,  "Aftertouch",      nullptr,     false, 0xFFFF8A7A },
    { 5,  "Mod Wheel",       nullptr,     false, 0xFF4169E1 },
    { 12, "MIDI CC",         nullptr,     false, 0xFF9898D0 },

    // ── MPE ───────────────────────────────────────────────────────────────
    { 13, "MPE Pressure",    "MPE",       false, 0xFFFFD54F },
    { 14, "MPE Slide",       nullptr,     false, 0xFFFF7043 },

    // ── Sequencer / musical ───────────────────────────────────────────────
    { 15, "Seq Step Value",  "Musical",   true,  0xFF81D4FA },
    { 16, "Chord Tone Idx",  nullptr,     false, 0xFFF48FB1 },
    { 17, "Beat Phase",      nullptr,     true,  0xFF80CBC4 },
};

static constexpr int kNumModSources = static_cast<int>(sizeof(kAllModSources) / sizeof(kAllModSources[0]));

//==============================================================================
// ModulateFromMenu — static helper.  No instantiation required.
//
class ModulateFromMenu
{
public:
    // Show the "Modulate from…" popup relative to `anchorComponent`.
    //
    // If a route for (source, destParamId) already exists the menu entry
    // shows the current depth and clicking it opens an adjust dialog rather
    // than adding a duplicate.
    //
    static void show(ModRoutingModel& model,
                     const juce::String& destParamId,
                     juce::Component* anchorComponent)
    {
        juce::PopupMenu menu;

        // ── Look-and-feel for the popup ──────────────────────────────────
        // We use an inline custom LnF for this popup only (not per-frame alloc —
        // it lives on the stack for the duration of showMenuAsync).
        struct MenuLnF : public juce::LookAndFeel_V4
        {
            void drawPopupMenuBackground(juce::Graphics& g, int w, int h) override
            {
                g.setColour(juce::Colour(18, 20, 30));
                g.fillRoundedRectangle(0.f, 0.f, (float)w, (float)h, 6.f);
                g.setColour(juce::Colour(200, 204, 216).withAlpha(0.06f));
                g.drawRoundedRectangle(0.5f, 0.5f, (float)w - 1.f, (float)h - 1.f, 6.f, 1.f);
            }

            void drawPopupMenuSectionHeader(juce::Graphics& g,
                                             const juce::Rectangle<int>& area,
                                             const juce::String& sectionName) override
            {
                g.setFont(GalleryFonts::label(8.0f));
                g.setColour(juce::Colour(200, 204, 216).withAlpha(0.28f));
                g.drawText(sectionName, area.reduced(8, 0), juce::Justification::centredLeft, false);
            }

            void drawPopupMenuItem(juce::Graphics& g, const juce::Rectangle<int>& area,
                                    bool isSeparator, bool isActive, bool isHighlighted,
                                    bool /*isTicked*/, bool /*hasSubMenu*/,
                                    const juce::String& text, const juce::String& /*shortcutKey*/,
                                    const juce::Drawable* /*icon*/,
                                    const juce::Colour* customColour) override
            {
                if (isSeparator)
                {
                    g.setColour(juce::Colour(200, 204, 216).withAlpha(0.05f));
                    g.fillRect(area.reduced(4, 0).withHeight(1));
                    return;
                }

                if (isHighlighted && isActive)
                {
                    g.setColour(juce::Colour(0xFF7FDBCA).withAlpha(0.10f));
                    g.fillRoundedRectangle(area.reduced(2, 1).toFloat(), 3.f);
                }

                // Colour swatch strip (3px left edge)
                if (customColour != nullptr)
                {
                    g.setColour(*customColour);
                    g.fillRect(area.getX() + 2, area.getY() + 3, 3, area.getHeight() - 6);
                }

                g.setFont(GalleryFonts::value(9.5f));
                g.setColour(isActive
                    ? juce::Colour(200, 204, 216).withAlpha(isHighlighted ? 0.90f : 0.65f)
                    : juce::Colour(200, 204, 216).withAlpha(0.25f));

                g.drawText(text, area.withTrimmedLeft(10).reduced(2, 0),
                            juce::Justification::centredLeft, true);
            }

            int getPopupMenuItemHeight() override { return 22; }
            int getPopupMenuBorderSize() override  { return 6; }
        };

        // NOTE: The LnF must outlive the menu's async execution.  We use a
        // shared_ptr captured into the lambda so it is released after the
        // callback fires.
        auto lnf = std::make_shared<MenuLnF>();

        // ── Header ───────────────────────────────────────────────────────
        menu.addSectionHeader("Modulate: " + destParamId);
        menu.addSeparator();

        // ── Existing routes section ───────────────────────────────────────
        auto existingRoutes = model.getRoutesForParam(destParamId);
        if (!existingRoutes.empty())
        {
            menu.addSectionHeader("Active routes");
            for (int i = 0; i < static_cast<int>(existingRoutes.size()); ++i)
            {
                const auto& r = existingRoutes[static_cast<size_t>(i)];
                // Find display info for this sourceId
                const char* srcLabel = "Source";
                uint32_t srcColour = GalleryColors::xoGold;
                for (const auto& info : kAllModSources)
                {
                    if (info.id == r.sourceId)
                    {
                        srcLabel = info.label;
                        srcColour = info.colour;
                        break;
                    }
                }

                juce::String depthStr = (r.depth >= 0.f ? "+" : "") + juce::String(r.depth, 2);
                juce::String label = juce::String(srcLabel) + "   " + depthStr;

                // Offset by 1000 to distinguish from "add new" IDs
                juce::PopupMenu::Item item;
                item.itemID = 1000 + i;
                item.text   = label;
                item.colour = juce::Colour(srcColour);
                item.isEnabled = true;
                menu.addItem(item);
            }
            menu.addItem(999, "Remove All Routes");
            menu.addSeparator();
        }

        // ── Add new route section ─────────────────────────────────────────
        menu.addSectionHeader("Add route from");

        const char* currentGroup = nullptr;
        for (const auto& info : kAllModSources)
        {
            // Check if this source already has a route to this param
            bool hasRoute = false;
            for (const auto& r : existingRoutes)
                if (r.sourceId == info.id) { hasRoute = true; break; }

            if (info.group != nullptr && info.group != currentGroup)
            {
                currentGroup = info.group;
                // group separators are handled via header items already
                // use a subtle text-only separator after first group
                if (info.group != kAllModSources[0].group)
                    menu.addSeparator();
                menu.addSectionHeader(juce::String(currentGroup));
            }

            juce::String label = juce::String(info.label);
            if (hasRoute)
                label += "  (edit)";

            // Item ID = info.id + 1 (0-based ID -> 1-based JUCE item)
            juce::PopupMenu::Item item;
            item.itemID   = info.id + 1;
            item.text     = label;
            item.colour   = juce::Colour(info.colour).withAlpha(hasRoute ? 0.85f : 1.0f);
            item.isEnabled = true;
            item.isTicked  = hasRoute;
            menu.addItem(item);
        }

        if (model.isFull())
        {
            menu.addSeparator();
            menu.addItem(-1, "(Route table full — remove a route first)");
        }

        // ── Show async ────────────────────────────────────────────────────
        auto opts = juce::PopupMenu::Options{}
            .withTargetComponent(anchorComponent)
            .withMaximumNumColumns(1);

        menu.showMenuAsync(opts,
            [&model, destParamId, lnf,
             existingRoutes = std::move(existingRoutes)](int result) mutable
            {
                if (result <= 0)
                    return; // dismissed

                // ── Remove-all existing routes ──────────────────────────
                if (result == 999)
                {
                    model.removeRoutesForParam(destParamId);
                    return;
                }

                // ── Adjust existing route (depth editor) ────────────────
                if (result >= 1000)
                {
                    const int subIdx = result - 1000;
                    if (subIdx >= 0 && subIdx < static_cast<int>(existingRoutes.size()))
                    {
                        const auto& r = existingRoutes[static_cast<size_t>(subIdx)];
                        // Find the index in the full model
                        auto allRoutes = model.getRoutesCopy();
                        for (int j = 0; j < static_cast<int>(allRoutes.size()); ++j)
                        {
                            if (allRoutes[static_cast<size_t>(j)].sourceId == r.sourceId &&
                                allRoutes[static_cast<size_t>(j)].destParamId == r.destParamId)
                            {
                                showDepthEditor(model, j);
                                break;
                            }
                        }
                    }
                    return;
                }

                // ── Add new route (result = sourceId + 1) ───────────────
                const int sourceId = result - 1;

                // Check for existing route with this source → bump to depth editor
                {
                    auto allRoutes = model.getRoutesCopy();
                    for (int j = 0; j < static_cast<int>(allRoutes.size()); ++j)
                    {
                        if (allRoutes[static_cast<size_t>(j)].sourceId == sourceId &&
                            allRoutes[static_cast<size_t>(j)].destParamId == destParamId)
                        {
                            showDepthEditor(model, j);
                            return;
                        }
                    }
                }

                if (model.isFull())
                    return;

                // Determine bipolar flag from catalogue
                bool bipolar = false;
                for (const auto& info : kAllModSources)
                    if (info.id == sourceId) { bipolar = info.bipolar; break; }

                const float defaultDepth = bipolar ? 0.5f : 0.35f;
                model.addRoute(sourceId, destParamId, defaultDepth, bipolar);
            });
    }

private:
    // Depth-adjust alert for an existing route at full-model index routeIdx.
    static void showDepthEditor(ModRoutingModel& model, int routeIdx)
    {
        auto routes = model.getRoutesCopy();
        if (routeIdx < 0 || routeIdx >= static_cast<int>(routes.size()))
            return;

        const auto& r = routes[static_cast<size_t>(routeIdx)];

        // Find display label
        const char* srcLabel = "Source";
        for (const auto& info : kAllModSources)
            if (info.id == r.sourceId) { srcLabel = info.label; break; }

        auto* alert = new juce::AlertWindow(
            "Adjust Mod Depth",
            juce::String(srcLabel) + "  →  " + r.destParamId,
            juce::MessageBoxIconType::NoIcon);

        alert->addTextEditor("depth", juce::String(r.depth, 3), "Depth  (−1.0 to +1.0):");
        alert->addButton("OK",     1, juce::KeyPress(juce::KeyPress::returnKey));
        alert->addButton("Remove", 2);
        alert->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));

        alert->enterModalState(
            true,
            juce::ModalCallbackFunction::create(
                [&model, routeIdx, alert](int res)
                {
                    if (res == 1)
                    {
                        float newDepth = alert->getTextEditorContents("depth").getFloatValue();
                        model.setRouteDepth(routeIdx, newDepth);
                    }
                    else if (res == 2)
                    {
                        model.removeRoute(routeIdx);
                    }
                    delete alert;
                }),
            false);
    }

    ModulateFromMenu() = delete; // static-only class
};

} // namespace xoceanus
