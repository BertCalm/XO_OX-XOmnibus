// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
// Future feature — not yet wired into the UI. See GitHub issue #670.
// DragDropModRouter implements Vital-style drag-drop modulation routing.
// To activate: instantiate in XOceanusEditor and add as a transparent overlay.
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include <vector>
#include <algorithm>
// GalleryColors.h lives at Source/UI/GalleryColors.h; use the Source/ root include
// path so this header is includable from any translation unit regardless of depth.
#include "UI/GalleryColors.h"
#include "ModSourceHandle.h"

namespace xoceanus
{

//==============================================================================
// ModRoute — a single modulation assignment: one source → one parameter.
//
// serialisation key: "modRoutes" in the preset JSON / APVTS ValueTree.
//
// depth range: -1.0 to +1.0
//   +1.0 = full upward modulation (source fully opens the destination)
//   -1.0 = full downward (inverted) modulation
//    0.0 = disabled but kept in the table (can be adjusted or removed)
//
// bipolar:
//   true  — source is treated as ±1 (LFOs, envelopes when centered)
//   false — source is treated as 0..1 (velocity, aftertouch, mod wheel)
//
struct ModRoute
{
    int sourceId;             // cast to ModSourceId — stored as int for ValueTree compat
    juce::String destParamId; // APVTS parameter ID of the destination
    float depth;              // -1.0 to +1.0
    bool bipolar;             // true = source range is ±1
    // Wave 5 C5: per-route slot index for sequencer-scoped sources.
    // -1 = not slot-scoped (all non-sequencer sources, backward-compat default).
    // 0–3 = which slotSequencer to read from (SeqStepValue, BeatPhase, ChordToneIdx).
    int slotIndex{-1};
};

//==============================================================================
// ModRoutingModel — the single source of truth for all active mod routes.
//
// Thread safety:
//   All mutations happen on the message thread (UI).  The audio engine reads
//   routes via a const reference to a vector snapshot obtained through
//   getRoutesCopy().  No locks on the audio path are required because the
//   model is mutated only when the user edits routing, not during audio.
//
//   If hot-swappable audio-thread safety is needed in the future, the same
//   double-buffered atomic shared_ptr pattern used by MegaCouplingMatrix can
//   be applied here.
//
// Serialisation:
//   toValueTree() / fromValueTree() — compatible with APVTS::state storage.
//   The key "modRoutes" is reserved in the preset JSON.
//
// Max routes: 32 hard cap.  Attempting to add beyond the cap is a silent no-op
// so UI callers can always call addRoute() without checking capacity first.
//
class ModRoutingModel
{
public:
    static constexpr int MaxRoutes = 32;

    //==========================================================================
    // Queries (message thread safe, const)

    // Returns a snapshot copy — safe to iterate without holding a reference.
    std::vector<ModRoute> getRoutesCopy() const { return routes; }

    // Returns all routes whose destParamId matches the given parameter.
    std::vector<ModRoute> getRoutesForParam(const juce::String& paramId) const
    {
        std::vector<ModRoute> result;
        for (const auto& r : routes)
            if (r.destParamId == paramId)
                result.push_back(r);
        return result;
    }

    int getRouteCount() const noexcept { return static_cast<int>(routes.size()); }
    bool isFull() const noexcept { return static_cast<int>(routes.size()) >= MaxRoutes; }

    //==========================================================================
    // Mutations (message thread only)

    // Add a route.  Returns the index of the new route, or -1 if the model is
    // full.  If a route from the same source to the same dest already exists,
    // the existing route's depth is updated and its index is returned.
    //
    // slotIndex: -1 = not slot-scoped (default, backward-compat).
    //            0–3 = which PerEnginePatternSequencer slot to read (C5: SeqStepValue etc.)
    int addRoute(int sourceId, const juce::String& destParamId, float depth, bool bipolar = false,
                 int slotIndex = -1)
    {
        // Update existing route with the same (source, dest) pair.
        for (int i = 0; i < static_cast<int>(routes.size()); ++i)
        {
            if (routes[static_cast<size_t>(i)].sourceId == sourceId &&
                routes[static_cast<size_t>(i)].destParamId == destParamId)
            {
                routes[static_cast<size_t>(i)].depth = juce::jlimit(-1.0f, 1.0f, depth);
                routes[static_cast<size_t>(i)].bipolar = bipolar;
                routes[static_cast<size_t>(i)].slotIndex = slotIndex;
                notifyListeners();
                return i;
            }
        }

        // Hard cap guard.
        if (isFull())
            return -1;

        ModRoute r;
        r.sourceId = sourceId;
        r.destParamId = destParamId;
        r.depth = juce::jlimit(-1.0f, 1.0f, depth);
        r.bipolar = bipolar;
        r.slotIndex = slotIndex;
        routes.push_back(r);

        notifyListeners();
        return static_cast<int>(routes.size()) - 1;
    }

    // Remove route at index `idx`.  Silently no-ops on out-of-range idx.
    void removeRoute(int idx)
    {
        if (idx < 0 || idx >= static_cast<int>(routes.size()))
            return;
        routes.erase(routes.begin() + idx);
        notifyListeners();
    }

    // Remove all routes targeting a specific parameter.
    void removeRoutesForParam(const juce::String& paramId)
    {
        auto prevSize = routes.size();
        routes.erase(
            std::remove_if(routes.begin(), routes.end(), [&](const ModRoute& r) { return r.destParamId == paramId; }),
            routes.end());
        if (routes.size() != prevSize)
            notifyListeners();
    }

    // Adjust depth of route at `idx`.  Clamps to [-1, +1].
    void setRouteDepth(int idx, float depth)
    {
        if (idx < 0 || idx >= static_cast<int>(routes.size()))
            return;
        routes[static_cast<size_t>(idx)].depth = juce::jlimit(-1.0f, 1.0f, depth);
        notifyListeners();
    }

    void clearAllRoutes()
    {
        if (!routes.empty())
        {
            routes.clear();
            notifyListeners();
        }
    }

    //==========================================================================
    // ValueTree serialisation

    juce::ValueTree toValueTree() const
    {
        juce::ValueTree vt("modRoutes");
        for (const auto& r : routes)
        {
            juce::ValueTree child("route");
            child.setProperty("sourceId", r.sourceId, nullptr);
            child.setProperty("destParamId", r.destParamId, nullptr);
            child.setProperty("depth", r.depth, nullptr);
            child.setProperty("bipolar", r.bipolar, nullptr);
            // C5: persist slotIndex (-1 for non-slot-scoped routes).
            child.setProperty("slotIndex", r.slotIndex, nullptr);
            vt.addChild(child, -1, nullptr);
        }
        return vt;
    }

    void fromValueTree(const juce::ValueTree& vt)
    {
        if (!vt.isValid() || vt.getType().toString() != "modRoutes")
            return;

        routes.clear();
        for (int i = 0; i < vt.getNumChildren() && static_cast<int>(routes.size()) < MaxRoutes; ++i)
        {
            auto child = vt.getChild(i);
            if (!child.isValid())
                continue;

            // Runtime-validate sourceId against known range before asserting type.
            int srcInt = static_cast<int>(child.getProperty("sourceId", -1));
            if (srcInt < 0 || srcInt >= static_cast<int>(ModSourceId::Count))
                continue;

            juce::String destId = child.getProperty("destParamId", "").toString();
            if (destId.isEmpty())
                continue;

            float depth = static_cast<float>(double(child.getProperty("depth", 0.0)));
            depth = juce::jlimit(-1.0f, 1.0f, depth);

            bool bipolar = static_cast<bool>(int(child.getProperty("bipolar", 0)));

            // C5: restore slotIndex; default -1 for backward-compat with older presets.
            int slotIdx = static_cast<int>(child.getProperty("slotIndex", -1));
            // Validate: must be -1 (N/A) or 0–3.
            if (slotIdx < -1 || slotIdx > 3)
                slotIdx = -1;

            ModRoute r;
            r.sourceId = srcInt;
            r.destParamId = destId;
            r.depth = depth;
            r.bipolar = bipolar;
            r.slotIndex = slotIdx;
            routes.push_back(r);
        }
        notifyListeners();
    }

    //==========================================================================
    // Change listeners — UI components register here to redraw when routes change.

    void addListener(juce::ChangeListener* l) { broadcaster.addChangeListener(l); }
    void removeListener(juce::ChangeListener* l) { broadcaster.removeChangeListener(l); }

private:
    void notifyListeners() { broadcaster.sendChangeMessage(); }

    std::vector<ModRoute> routes;
    juce::ChangeBroadcaster broadcaster;
};

//==============================================================================
// ModKnobOverlay — a 1px transparent component that sits on top of (or is
// painted by) each destination knob to show modulation arcs.
//
// This is NOT a separate heavyweight component.  It is a value type that a
// knob's owner can store.  Call paintModArcs() from the knob's paint() method
// after drawing the normal knob body.
//
// Arc geometry:
//   A short curved arc is drawn around the outside of the knob's rotary
//   track.  The arc starts at the knob's current value angle and extends
//   ± (depth * half-sweep) to show the modulation range.  When depth is
//   negative the arc sweeps in the opposite direction.
//
//   Track arc range follows the standard rotary convention:
//     rotaryStartAngle = -2.42 rad  (~-138°)
//     rotaryEndAngle   = +2.42 rad  (~+138°)
//   These match JUCE default and GalleryLookAndFeel.
//
struct ModKnobOverlay
{
    static constexpr float kRotaryStart = -2.42f; // rad — matches GalleryLookAndFeel
    static constexpr float kRotaryEnd = 2.42f;
    static constexpr float kSweep = kRotaryEnd - kRotaryStart; // total sweep (4.84 rad)

    // Draw all active mod arcs for `paramId`.
    // Call this from inside the knob's paint() function.
    // `bounds` should be the knob's local bounds in float coords.
    static void paintModArcs(juce::Graphics& g, const juce::Rectangle<float>& bounds, const juce::String& paramId,
                             float currentNormalisedValue, const ModRoutingModel& model)
    {
        auto routes = model.getRoutesForParam(paramId);
        if (routes.empty())
            return;

        float diameter = juce::jmin(bounds.getWidth(), bounds.getHeight());
        float radius = diameter * 0.5f;
        float cx = bounds.getCentreX();
        float cy = bounds.getCentreY();

        // Arcs are drawn just outside the fill arc track (r + 3px)
        float arcRadius = radius - 3.0f + 4.5f;

        // Current value angle
        float currentAngle = kRotaryStart + currentNormalisedValue * kSweep;

        for (const auto& r : routes)
        {
            juce::Colour srcColour = modSourceColour(static_cast<ModSourceId>(r.sourceId));

            // Half-sweep = |depth| * quarter of full sweep
            float halfSweep = std::abs(r.depth) * (kSweep * 0.25f);
            halfSweep = juce::jlimit(0.0f, kSweep * 0.5f, halfSweep);

            float arcStart = currentAngle;
            float arcEnd = currentAngle + (r.depth >= 0.0f ? halfSweep : -halfSweep);

            // Ensure start < end for drawing
            if (arcStart > arcEnd)
                std::swap(arcStart, arcEnd);

            // Clamp to rotary sweep range
            arcStart = juce::jlimit(kRotaryStart, kRotaryEnd, arcStart);
            arcEnd = juce::jlimit(kRotaryStart, kRotaryEnd, arcEnd);

            if (arcEnd - arcStart < 0.02f)
                continue; // too small to draw

            // Draw: glow pass (wide, transparent) then core pass
            juce::Path arc;
            arc.addCentredArc(cx, cy, arcRadius, arcRadius, 0.0f, arcStart, arcEnd, true);

            // Glow
            g.setColour(srcColour.withAlpha(0.18f));
            g.strokePath(arc, juce::PathStrokeType(4.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            // Core
            g.setColour(srcColour.withAlpha(0.75f));
            g.strokePath(arc, juce::PathStrokeType(1.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }
    }
};

//==============================================================================
// ModRouteListPanel — an optional collapsible text list of all active routes.
//
// Shows one row per route:  "[source]  →  param.id  depth: +0.65"
// Right-clicking a row offers "Set Depth…" and "Remove Route" menu items.
//
// Wire up with setModel() and call refresh() or listen for model changes.
//
class ModRouteListPanel : public juce::Component, public juce::ChangeListener, private juce::ListBoxModel
{
public:
    ModRouteListPanel()
    {
        listBox.setModel(this);
        listBox.setRowHeight(22);
        listBox.setMultipleSelectionEnabled(false);
        listBox.setColour(juce::ListBox::backgroundColourId, juce::Colour(GalleryColors::elevated()));
        listBox.setColour(juce::ListBox::outlineColourId, GalleryColors::border());
        addAndMakeVisible(listBox);

        // Header label
        headerLabel.setFont(GalleryFonts::label(9.5f));
        headerLabel.setColour(juce::Label::textColourId, juce::Colour(GalleryColors::t2()));
        headerLabel.setText("MOD ROUTES", juce::dontSendNotification);
        headerLabel.setJustificationType(juce::Justification::centredLeft);
        addAndMakeVisible(headerLabel);

        A11y::setup(*this, "Modulation Routes", "List of all active modulation assignments");
    }

    ~ModRouteListPanel() override
    {
        if (model != nullptr)
            model->removeListener(this);
    }

    void setModel(ModRoutingModel* m)
    {
        if (model != nullptr)
            model->removeListener(this);
        model = m;
        if (model != nullptr)
            model->addListener(this);
        refresh();
    }

    void refresh()
    {
        if (model != nullptr)
            cachedRoutes = model->getRoutesCopy();
        else
            cachedRoutes.clear();

        listBox.updateContent();
        repaint();
    }

    void resized() override
    {
        auto b = getLocalBounds();
        headerLabel.setBounds(b.removeFromTop(18).withTrimmedLeft(6));
        listBox.setBounds(b.reduced(2, 0));
    }

    //==========================================================================
    // juce::ChangeListener
    void changeListenerCallback(juce::ChangeBroadcaster*) override { refresh(); }

    //==========================================================================
    // ListBoxModel
    int getNumRows() override { return static_cast<int>(cachedRoutes.size()); }

    void paintListBoxItem(int row, juce::Graphics& g, int width, int height, bool selected) override
    {
        if (row < 0 || row >= static_cast<int>(cachedRoutes.size()))
            return;

        const auto& r = cachedRoutes[static_cast<size_t>(row)];

        // Row background
        if (selected)
            g.fillAll(juce::Colour(GalleryColors::xoGold).withAlpha(0.12f));
        else if (row % 2 == 1)
            g.fillAll(juce::Colour(GalleryColors::elevated()).withAlpha(0.5f));

        // Source color swatch (4px wide strip on left)
        juce::Colour srcColour = modSourceColour(static_cast<ModSourceId>(r.sourceId));
        g.setColour(srcColour);
        g.fillRect(0, 2, 3, height - 4);

        // Source name — append "S1"–"S4" suffix for slot-scoped sources (C5).
        auto srcName = modSourceName(static_cast<ModSourceId>(r.sourceId));
        if (r.slotIndex >= 0 && r.slotIndex <= 3)
            srcName += " S" + juce::String(r.slotIndex + 1);
        g.setFont(GalleryFonts::label(9.5f));
        g.setColour(juce::Colour(GalleryColors::t1()));
        g.drawText(srcName, 8, 0, 70, height, juce::Justification::centredLeft);

        // Arrow
        g.setColour(juce::Colour(GalleryColors::t3()));
        g.drawText(juce::CharPointer_UTF8("\xe2\x86\x92"), 78, 0, 14, height, juce::Justification::centred);

        // Dest param ID (truncated)
        g.setFont(GalleryFonts::value(9.0f));
        g.setColour(juce::Colour(GalleryColors::t2()));
        g.drawText(r.destParamId, 94, 0, width - 140, height, juce::Justification::centredLeft,
                   /* useEllipsis = */ true);

        // Depth value (right-aligned)
        juce::String depthStr = (r.depth >= 0.0f ? "+" : "") + juce::String(r.depth, 2);
        g.setFont(GalleryFonts::value(9.0f));
        g.setColour(srcColour.withAlpha(0.85f));
        g.drawText(depthStr, width - 42, 0, 38, height, juce::Justification::centredRight);
    }

    void listBoxItemClicked(int row, const juce::MouseEvent& e) override
    {
        if (e.mods.isRightButtonDown())
            showContextMenu(row);
    }

    void listBoxItemDoubleClicked(int row, const juce::MouseEvent&) override { showDepthEditor(row); }

private:
    void showContextMenu(int row)
    {
        if (row < 0 || row >= static_cast<int>(cachedRoutes.size()) || model == nullptr)
            return;

        juce::PopupMenu menu;
        menu.addItem(1, "Set Depth...");
        menu.addSeparator();
        menu.addItem(2, "Remove Route");

        menu.showMenuAsync(juce::PopupMenu::Options{},
                           [this, row](int result)
                           {
                               if (result == 1)
                                   showDepthEditor(row);
                               if (result == 2 && model != nullptr)
                                   model->removeRoute(row);
                           });
    }

    void showDepthEditor(int row)
    {
        if (row < 0 || row >= static_cast<int>(cachedRoutes.size()) || model == nullptr)
            return;

        const auto& r = cachedRoutes[static_cast<size_t>(row)];
        // C5: include slot suffix if this is a slot-scoped route.
        juce::String srcLabel = modSourceName(static_cast<ModSourceId>(r.sourceId));
        if (r.slotIndex >= 0 && r.slotIndex <= 3)
            srcLabel += " (Slot " + juce::String(r.slotIndex + 1) + ")";
        auto* alert = new juce::AlertWindow("Set Modulation Depth",
                                            "Enter depth for " + srcLabel + " -> " + r.destParamId,
                                            juce::MessageBoxIconType::NoIcon);
        alert->addTextEditor("depth", juce::String(r.depth, 3), "Depth (-1.0 to +1.0):");
        alert->addButton("OK", 1, juce::KeyPress(juce::KeyPress::returnKey));
        alert->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));

        // Capture `row` by value — safe because the alert lives longer than this lambda.
        alert->enterModalState(true,
                               juce::ModalCallbackFunction::create(
                                   [this, row, alert](int result)
                                   {
                                       if (result == 1 && model != nullptr)
                                       {
                                           auto depthStr = alert->getTextEditorContents("depth");
                                           float newDepth = depthStr.getFloatValue();
                                           model->setRouteDepth(row, newDepth);
                                       }
                                       delete alert;
                                   }),
                               false);
    }

    juce::ListBox listBox;
    juce::Label headerLabel;
    ModRoutingModel* model{nullptr};
    std::vector<ModRoute> cachedRoutes;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModRouteListPanel)
};

//==============================================================================
// DragDropModRouter — transparent overlay for Vital-style drag-drop mod routing.
//
// Architecture overview
// ─────────────────────
// This component is an INVISIBLE, FULL-EDITOR overlay — same size as the
// editor root.  It is added as the topmost child so it receives drag-and-drop
// events before any other component.  It is fully transparent to MOUSE events
// in its idle state (setInterceptsMouseClicks(false, false)) so that normal
// interaction with knobs/buttons beneath passes through unobstructed.
//
// The component activates only while a drag is in progress:
//   1. A ModSourceHandle (child of a source row) calls startDragging().
//   2. JUCE routes the drag to DragDropModRouter because it is registered as
//      a DragAndDropTarget.
//   3. While dragging, the overlay paints a live connector line from the
//      source handle center to the current cursor position.
//   4. On drop, it calls getComponentAt() to find a GalleryKnob under the
//      cursor, extracts its APVTS paramId, and calls model->addRoute().
//
// Source handle strip
// ───────────────────
// A horizontal strip of ModSourceHandle buttons is shown at the top of the
// overlay in a compact pill-shaped bar.  This is the user's starting point
// for creating routes.  The strip can be hidden via setSourceStripVisible().
//
// State during drag
// ─────────────────
// dragActive:      true while a drag is in-flight
// dragSourcePos:   screen coords of the drag source handle center
// dragCursorPos:   current cursor position (updated by itemDragMove)
// dragSourceId:    which ModSourceId is being dragged
//
// Painting (idle state, no drag): nothing (transparent).
// Painting (drag active):
//   • A dashed Bézier line from source handle to cursor
//   • The cursor end has a small glowing crosshair to indicate "droppable"
//   • When over a valid knob target: draw a highlight ring on the target
//
// Usage
// ─────
//   // In XOceanusEditor constructor:
//   modRouter = std::make_unique<DragDropModRouter>(apvts, modModel);
//   addAndMakeVisible(*modRouter);
//   modRouter->toFront(false);
//
//   // In XOceanusEditor::resized():
//   modRouter->setBounds(getLocalBounds());
//
// A3 TODO: when source palette / draggable handles are added, give them a
// DragAndDropContainer ancestor — most likely by replacing this `public juce::Component`
// with `public juce::DragAndDropContainer` (which IS-A Component in this JUCE version),
// so findParentDragContainerFor(handle) resolves. Editor cannot host the container
// itself: AudioProcessorEditor + DragAndDropContainer both reach Component, creating
// a diamond that breaks every addChildComponent/addAndMakeVisible call (Wave 5 A1 CI).
class DragDropModRouter : public juce::Component, public juce::DragAndDropTarget, public juce::ChangeListener
{
public:
    //==========================================================================
    DragDropModRouter(juce::AudioProcessorValueTreeState& apvts, ModRoutingModel& model) : apvts(apvts), model(model)
    {
        setInterceptsMouseClicks(false, false); // idle = pass-through
        setBufferedToImage(false);              // no buffer — transparent most of the time

        // Source strip — always visible at the top of the overlay
        for (int i = 0; i < static_cast<int>(ModSourceId::Count); ++i)
        {
            auto handle = std::make_unique<ModSourceHandle>(static_cast<ModSourceId>(i));
            addAndMakeVisible(*handle);
            sourceHandles.push_back(std::move(handle));
        }

        // Route list panel — hidden by default, shown via setRouteListVisible()
        routeListPanel.setModel(&model);
        routeListPanel.setVisible(false);
        addChildComponent(routeListPanel);

        model.addListener(this);

        A11y::setup(*this, "Modulation Router", "Drag source handles to parameter knobs to create modulation routes");
    }

    ~DragDropModRouter() override { model.removeListener(this); }

    //==========================================================================
    // Layout

    void setSourceStripVisible(bool shouldBeVisible)
    {
        for (auto& h : sourceHandles)
            h->setVisible(shouldBeVisible);
        resized();
    }

    void setRouteListVisible(bool shouldBeVisible)
    {
        routeListPanel.setVisible(shouldBeVisible);
        resized();
    }

    void resized() override
    {
        auto b = getLocalBounds();

        // ── Route list panel (right side, 200px wide) ─────────────────────
        if (routeListPanel.isVisible())
        {
            routeListPanel.setBounds(b.removeFromRight(200).reduced(4, 8));
        }

        // ── Source handle strip (top edge, centered) ─────────────────────
        // Strip pill: each handle is 16×16 with 6px horizontal gap between them.
        // Total strip width: N * 16 + (N-1) * 6
        const int n = static_cast<int>(sourceHandles.size());
        const int hW = ModSourceHandle::kDiameter;
        const int gap = 6;
        const int stripW = n * hW + (n - 1) * gap;
        const int stripTop = 8;

        int xOffset = (b.getWidth() - stripW) / 2;
        for (int i = 0; i < n; ++i)
        {
            sourceHandles[static_cast<size_t>(i)]->setBounds(xOffset + i * (hW + gap), stripTop, hW, hW);
        }
    }

    //==========================================================================
    // DragAndDropTarget implementation

    bool isInterestedInDragSource(const SourceDetails& details) override
    {
        return DragPayload::isModSourceDrag(details.description);
    }

    void itemDragEnter(const SourceDetails& details) override
    {
        if (!DragPayload::isModSourceDrag(details.description))
            return;

        dragActive = true;
        auto payload = DragPayload::decode(details.description);
        dragSourceId = payload.sourceId;

        // Source handle center in this component's coordinate space
        if (auto* srcComp = details.sourceComponent.get())
            dragSourcePos = getLocalPoint(srcComp, srcComp->getLocalBounds().getCentre().toFloat());

        dragCursorPos = details.localPosition.toFloat();

        // Activate mouse interception so we can draw the connector line
        setInterceptsMouseClicks(true, false);
        repaint();
    }

    void itemDragMove(const SourceDetails& details) override
    {
        dragCursorPos = details.localPosition.toFloat();

        // Highlight the knob under the cursor (if any)
        currentTargetParamId = findParamIdUnderCursor(details.localPosition);
        repaint();
    }

    void itemDragExit(const SourceDetails& /*details*/) override { resetDragState(); }

    void itemDropped(const SourceDetails& details) override
    {
        if (!DragPayload::isModSourceDrag(details.description))
        {
            resetDragState();
            return;
        }

        auto payload = DragPayload::decode(details.description);
        auto targetParamId = findParamIdUnderCursor(details.localPosition);

        if (targetParamId.isNotEmpty())
        {
            // Default: bipolar for LFOs, unipolar for performance sources
            bool bipolar = (payload.sourceId == ModSourceId::LFO1 || payload.sourceId == ModSourceId::LFO2 ||
                            payload.sourceId == ModSourceId::Envelope);

            // C5: slot-scoped sources require a slot picker before committing the route.
            if (isSlotScopedSource(payload.sourceId))
            {
                showSlotPickerForDrop(payload.sourceId, targetParamId, bipolar);
                // resetDragState is called inside showSlotPickerForDrop's async callback.
                resetDragState();
                return;
            }

            model.addRoute(static_cast<int>(payload.sourceId), targetParamId,
                           /* depth = */ 0.5f, bipolar);
        }

        resetDragState();
    }

    //==========================================================================
    // Painting

    void paint(juce::Graphics& g) override
    {
        // Idle state: paint source strip labels and nothing else.
        if (!dragActive)
        {
            paintSourceStripBackground(g);
            return;
        }

        // ── Drag in-progress ──────────────────────────────────────────────
        juce::Colour srcColour = modSourceColour(dragSourceId);

        // Connector line: Bézier from source handle to cursor
        {
            const float dy = dragCursorPos.y - dragSourcePos.y;
            const float controlStrength = std::min(std::abs(dy) * 0.6f + 30.0f, 120.0f);

            juce::Path connector;
            connector.startNewSubPath(dragSourcePos);
            connector.cubicTo(dragSourcePos.x, dragSourcePos.y + controlStrength, dragCursorPos.x,
                              dragCursorPos.y - controlStrength, dragCursorPos.x, dragCursorPos.y);

            // Glow pass
            g.setColour(srcColour.withAlpha(0.15f));
            g.strokePath(connector,
                         juce::PathStrokeType(6.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            // Core line — dashed
            juce::PathStrokeType stroke(2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded);
            float dashLengths[] = {5.0f, 4.0f};
            juce::Path dashedConnector;
            stroke.createDashedStroke(dashedConnector, connector, dashLengths, 2);
            g.setColour(srcColour.withAlpha(0.85f));
            g.fillPath(dashedConnector);
        }

        // Cursor crosshair
        {
            const float cr = 7.0f;
            bool hasTarget = currentTargetParamId.isNotEmpty();
            juce::Colour crossColour =
                hasTarget ? juce::Colour(GalleryColors::xoGold).withAlpha(0.90f) : srcColour.withAlpha(0.70f);

            // Outer glow
            g.setColour(crossColour.withAlpha(0.25f));
            g.fillEllipse(dragCursorPos.x - cr - 3.0f, dragCursorPos.y - cr - 3.0f, (cr + 3.0f) * 2.0f,
                          (cr + 3.0f) * 2.0f);
            // Inner ring
            g.setColour(crossColour);
            g.drawEllipse(dragCursorPos.x - cr, dragCursorPos.y - cr, cr * 2.0f, cr * 2.0f, 1.5f);

            if (hasTarget)
            {
                // Filled dot center — "locked on" indication
                g.setColour(crossColour.withAlpha(0.9f));
                g.fillEllipse(dragCursorPos.x - 2.5f, dragCursorPos.y - 2.5f, 5.0f, 5.0f);
            }
        }

        // Target knob highlight ring (if over a valid param)
        if (currentTargetParamId.isNotEmpty())
        {
            if (auto* targetComp = findComponentForParam(currentTargetParamId))
            {
                auto bounds = getLocalArea(targetComp, targetComp->getLocalBounds()).toFloat();
                float r = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.5f + 4.0f;
                float cx = bounds.getCentreX();
                float cy = bounds.getCentreY();

                // Double-ring highlight: glow + sharp border
                g.setColour(juce::Colour(GalleryColors::xoGold).withAlpha(0.20f));
                g.drawEllipse(cx - r - 2.0f, cy - r - 2.0f, (r + 2.0f) * 2.0f, (r + 2.0f) * 2.0f, 3.5f);

                g.setColour(juce::Colour(GalleryColors::xoGold).withAlpha(0.80f));
                g.drawEllipse(cx - r, cy - r, r * 2.0f, r * 2.0f, 1.8f);

                // Small "+" drop indicator
                g.setFont(GalleryFonts::label(9.0f));
                g.setColour(juce::Colour(GalleryColors::xoGold));
                g.drawText("+", static_cast<int>(cx) + static_cast<int>(r) - 8,
                           static_cast<int>(cy) - static_cast<int>(r) - 12, 14, 14, juce::Justification::centred);
            }
        }
    }

    //==========================================================================
    // Right-click on an existing mod-ring shows depth-adjust/remove menu.
    //
    // This is called by the owning editor when a right-click lands on a knob
    // that has active mod routes.  Decouples the right-click logic from the
    // GalleryKnob subclass (so GalleryKnob stays simple).
    //
    void showModDepthMenuForParam(const juce::String& paramId, juce::Component* relativeTo)
    {
        auto routes = model.getRoutesForParam(paramId);
        if (routes.empty())
            return;

        juce::PopupMenu menu;
        menu.addSectionHeader("Mod Routes: " + paramId);

        for (int i = 0; i < static_cast<int>(routes.size()); ++i)
        {
            const auto& r = routes[static_cast<size_t>(i)];
            // C5: append slot suffix for slot-scoped sources
            auto srcName = modSourceName(static_cast<ModSourceId>(r.sourceId));
            if (r.slotIndex >= 0 && r.slotIndex <= 3)
                srcName += " S" + juce::String(r.slotIndex + 1);
            juce::String label = srcName + "   depth: " + juce::String(r.depth, 2);
            menu.addItem(100 + i, label);
        }
        menu.addSeparator();
        menu.addItem(200, "Remove All for This Parameter");

        menu.showMenuAsync(juce::PopupMenu::Options{}.withTargetComponent(relativeTo),
                           [this, paramId, routes](int result)
                           {
                               if (result >= 100 && result < 100 + static_cast<int>(routes.size()))
                               {
                                   // Find the actual index in the full model (not just per-param subset)
                                   int subIndex = result - 100;
                                   auto allRoutes = model.getRoutesCopy();
                                   const auto& targetRoute = routes[static_cast<size_t>(subIndex)];

                                   for (int j = 0; j < static_cast<int>(allRoutes.size()); ++j)
                                   {
                                       if (allRoutes[static_cast<size_t>(j)].sourceId == targetRoute.sourceId &&
                                           allRoutes[static_cast<size_t>(j)].destParamId == targetRoute.destParamId)
                                       {
                                           showDepthEditorForRoute(j);
                                           break;
                                       }
                                   }
                               }
                               else if (result == 200)
                               {
                                   model.removeRoutesForParam(paramId);
                               }
                           });
    }

    //==========================================================================
    // juce::ChangeListener — model has changed, repaint
    void changeListenerCallback(juce::ChangeBroadcaster*) override { repaint(); }

private:
    //==========================================================================
    // Source strip background — a subtle pill behind the handles
    void paintSourceStripBackground(juce::Graphics& g)
    {
        if (sourceHandles.empty())
            return;

        auto firstBounds = sourceHandles.front()->getBounds();
        auto lastBounds = sourceHandles.back()->getBounds();

        float left = static_cast<float>(firstBounds.getX()) - 10.0f;
        float top = static_cast<float>(firstBounds.getY()) - 5.0f;
        float right = static_cast<float>(lastBounds.getRight()) + 10.0f;
        float bottom = static_cast<float>(firstBounds.getBottom()) + 5.0f;

        juce::Rectangle<float> pill(left, top, right - left, bottom - top);

        // Pill background
        g.setColour(juce::Colour(GalleryColors::elevated()).withAlpha(0.88f));
        g.fillRoundedRectangle(pill, (bottom - top) * 0.5f);

        // Pill border
        g.setColour(GalleryColors::border());
        g.drawRoundedRectangle(pill, (bottom - top) * 0.5f, 1.0f);

        // "MOD" label to the left of handles
        g.setFont(GalleryFonts::label(7.5f));
        g.setColour(juce::Colour(GalleryColors::t3()));
        g.drawText("MOD", static_cast<int>(left - 30), static_cast<int>(top), 28, static_cast<int>(bottom - top),
                   juce::Justification::centredRight);
    }

    //==========================================================================
    // Find the APVTS paramId of the component under a given local position.
    //
    // Strategy: walk up the component hierarchy from the hit-tested component
    // until we find one that has a "paramId" property set (convention used by
    // APVTS attachment helpers) OR is a juce::Slider whose paramId can be
    // retrieved via the APVTS parameter list.
    //
    // This avoids any dependency on the GalleryKnob class from this file.
    //
    juce::String findParamIdUnderCursor(juce::Point<int> localPos) const
    {
        // Convert to screen coords so getComponentAt can search the full hierarchy
        auto screenPos = localPointToGlobal(localPos);

        // Walk from the topmost component under the cursor down to the editor root
        if (auto* desktop = &juce::Desktop::getInstance())
        {
            if (auto* topComp = desktop->findComponentAt(screenPos))
            {
                // Walk up the parent chain looking for a Slider with a known paramId
                auto* comp = topComp;
                while (comp != nullptr)
                {
                    // Check for juce::Slider — then look it up in APVTS
                    if (auto* slider = dynamic_cast<juce::Slider*>(comp))
                    {
                        // Look for a parameter whose attached slider matches this one.
                        // APVTS stores SliderAttachments; we can identify via getParameter().
                        // Fallback: check the component's "paramId" property.
                        juce::var propId = comp->getProperties()["paramId"];
                        if (!propId.isVoid() && propId.toString().isNotEmpty())
                            return propId.toString();

                        // Try to find in APVTS by name prefix match
                        // (Sliders created with APVTS attachments don't automatically
                        //  expose their paramId, so the property convention is preferred.)
                    }
                    comp = comp->getParentComponent();
                }
            }
        }
        return {};
    }

    // Find the component responsible for rendering a given paramId so we can
    // draw the highlight ring at the correct position.
    juce::Component* findComponentForParam(const juce::String& paramId) const
    {
        // Walk the full component tree from the root to find one with this paramId.
        if (auto* root = getParentComponent())
            return findRecursive(root, paramId);
        return nullptr;
    }

    static juce::Component* findRecursive(juce::Component* comp, const juce::String& paramId)
    {
        if (comp == nullptr)
            return nullptr;
        juce::var propId = comp->getProperties()["paramId"];
        if (!propId.isVoid() && propId.toString() == paramId)
            return comp;
        for (int i = 0; i < comp->getNumChildComponents(); ++i)
            if (auto* found = findRecursive(comp->getChildComponent(i), paramId))
                return found;
        return nullptr;
    }

    //==========================================================================
    // Depth editor alert window (inline — avoids adding a public method to the list panel)
    void showDepthEditorForRoute(int routeIndex)
    {
        auto routes = model.getRoutesCopy();
        if (routeIndex < 0 || routeIndex >= static_cast<int>(routes.size()))
            return;

        const auto& r = routes[static_cast<size_t>(routeIndex)];
        // C5: include slot suffix if this is a slot-scoped route.
        juce::String srcLabel = modSourceName(static_cast<ModSourceId>(r.sourceId));
        if (r.slotIndex >= 0 && r.slotIndex <= 3)
            srcLabel += " (Slot " + juce::String(r.slotIndex + 1) + ")";
        auto* alert = new juce::AlertWindow(
            "Set Modulation Depth", srcLabel + " -> " + r.destParamId,
            juce::MessageBoxIconType::NoIcon);

        alert->addTextEditor("depth", juce::String(r.depth, 3), "Depth (-1.0 to +1.0):");
        alert->addButton("OK", 1, juce::KeyPress(juce::KeyPress::returnKey));
        alert->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));

        alert->enterModalState(true,
                               juce::ModalCallbackFunction::create(
                                   [this, routeIndex, alert](int result)
                                   {
                                       if (result == 1)
                                       {
                                           float newDepth = alert->getTextEditorContents("depth").getFloatValue();
                                           model.setRouteDepth(routeIndex, newDepth);
                                       }
                                       delete alert;
                                   }),
                               false);
    }

    //==========================================================================
    // C5: Returns true for the three slot-scoped ModSourceIds.
    static bool isSlotScopedSource(ModSourceId id) noexcept
    {
        return id == ModSourceId::SeqStepValue ||
               id == ModSourceId::BeatPhase    ||
               id == ModSourceId::ChordToneIdx;
    }

    // C5: Show a popup menu to choose which sequencer slot (1–4) this route reads from.
    // On selection, adds the route with the chosen slotIndex.
    // Called from itemDropped when the dragged source is slot-scoped.
    void showSlotPickerForDrop(ModSourceId sourceId, const juce::String& destParamId, bool bipolar)
    {
        juce::PopupMenu menu;
        menu.addSectionHeader("Which sequencer slot?");
        menu.addItem(1, "Slot 1");
        menu.addItem(2, "Slot 2");
        menu.addItem(3, "Slot 3");
        menu.addItem(4, "Slot 4");

        menu.showMenuAsync(juce::PopupMenu::Options{},
                           [this, sourceId, destParamId, bipolar](int result)
                           {
                               if (result >= 1 && result <= 4)
                               {
                                   const int slotIndex = result - 1; // 0-based
                                   model.addRoute(static_cast<int>(sourceId), destParamId,
                                                  /* depth = */ 0.5f, bipolar, slotIndex);
                               }
                               // result == 0 means cancelled — no route added.
                           });
    }

    //==========================================================================
    void resetDragState()
    {
        dragActive = false;
        dragSourceId = ModSourceId::LFO1;
        currentTargetParamId = {};
        setInterceptsMouseClicks(false, false);
        repaint();
    }

    //==========================================================================
    [[maybe_unused]] juce::AudioProcessorValueTreeState& apvts;
    ModRoutingModel& model;

    // Source handle strip
    std::vector<std::unique_ptr<ModSourceHandle>> sourceHandles;

    // Route list panel (optional)
    ModRouteListPanel routeListPanel;

    // Drag state
    bool dragActive{false};
    ModSourceId dragSourceId{ModSourceId::LFO1};
    juce::Point<float> dragSourcePos{};
    juce::Point<float> dragCursorPos{};
    juce::String currentTargetParamId{};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DragDropModRouter)
};

} // namespace xoceanus
