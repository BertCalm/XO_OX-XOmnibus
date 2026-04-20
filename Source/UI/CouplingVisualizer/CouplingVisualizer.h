// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include <juce_gui_extra/juce_gui_extra.h>
#include "../../Core/MegaCouplingMatrix.h"
#include "../../Core/SynthEngine.h"
#include "../GalleryColors.h"     // GalleryColors namespace (no editor circular dep)
#include "../CouplingColors.h"    // CouplingTypeColors — canonical coupling colour source
#include <array>
#include <atomic>
#include <functional>

namespace xoceanus
{

//==============================================================================
// CouplingVisualizer — full-panel graph view of cross-engine modulation routing.
//
// Architecture: Docs/coupling-ui-architecture-2026-03-21.md
//
// Up to 5 engine slots render as nodes (4 primary at cardinal points + Ghost Slot at bottom-centre).
// Active coupling routes render as animated cubic Bézier arcs coloured by type.
// Arc brightness pulses in real time, driven by per-route RMS atomics written
// by the audio thread (std::memory_order_relaxed reads — no locks).
//
// Interaction:
//   - Drag from an output port to an input port  →  add route (inline popup)
//   - Click arc                                   →  select / highlight
//   - Right-click arc                             →  context menu (edit/delete)
//   - Delete key on selected arc                  →  remove user route
//
// Constructor signature:
//   CouplingVisualizer viz(matrix, slotNameFn, slotColorFn);
//
// Thread safety:
//   - timerCallback() and paint() run exclusively on the message thread.
//   - Audio thread writes: routeRMS[n].store(..., relaxed) — one atomic per route.
//   - Message thread reads: routeRMS[n].load(relaxed) once per timerCallback().
//   - couplingMatrix.getRoutes() performs a single std::atomic_load per tick.
//   - No locks anywhere in this component.
//
// Performance budget: < 1 ms per frame at 30 Hz (see architecture doc §8).
//==============================================================================


//==============================================================================
// CouplingVisualizer
//==============================================================================
class CouplingVisualizer : public juce::Component, private juce::Timer
{
public:
    //--------------------------------------------------------------------------
    // Constants — geometry and visual tunables
    //--------------------------------------------------------------------------

    // Node diamond layout: fraction of min(width,height) used as orbit radius.
    static constexpr float kOrbitFraction = 0.34f;

    // Node circle radius in pixels.
    static constexpr float kNodeRadius = 28.0f;

    // Port ellipse radius (output/input connection dots).
    static constexpr float kPortRadius = 5.0f;
    static constexpr float kPortHoverRadius = 8.0f;

    // Hit-test tolerance for arcs (in pixels).
    static constexpr float kArcHitRadius = 8.0f;

    // Bézier perpendicular offset factor — controls arc curvature.
    static constexpr float kArcCurvature = 0.35f;

    // Stagger offset multipliers for multi-route pairs.
    static constexpr float kArcStagger = 0.20f;

    // Maximum RMS slots — matches MegaCouplingMatrix::MaxRoutes.
    static constexpr int kMaxRMSSlots = 64;

    //--------------------------------------------------------------------------
    // Construction
    //--------------------------------------------------------------------------

    explicit CouplingVisualizer(MegaCouplingMatrix& matrix, std::function<juce::String(int)> slotNameFn,
                                std::function<juce::Colour(int)> slotColorFn)
        : couplingMatrix(matrix), getSlotName(std::move(slotNameFn)), getSlotColor(std::move(slotColorFn))
    {
        setTitle("Coupling Visualizer");
        setDescription("Graph view of cross-engine modulation routes. "
                       "Drag from an output port to an input port to create a route. "
                       "Right-click a route arc for options.");
        setWantsKeyboardFocus(true);

        // Initialise all RMS atomics to silence.
        for (auto& a : routeRMS)
            a.store(0.0f, std::memory_order_relaxed);
    }

    ~CouplingVisualizer() override { stopTimer(); }

    //-- Lifecycle (message thread only) ----------------------------------------

    void start()
    {
        JUCE_ASSERT_MESSAGE_THREAD
        startTimerHz(reducedMotion ? 10 : 30);
    }

    void stop()
    {
        JUCE_ASSERT_MESSAGE_THREAD
        stopTimer();
    }

    void setReducedMotion(bool enabled)
    {
        JUCE_ASSERT_MESSAGE_THREAD
        reducedMotion = enabled;
        if (isTimerRunning())
            start();
    }

    /// Call this from the audio thread (processBlock) to feed per-route RMS.
    /// Index must match the order routes appear in MegaCouplingMatrix::getRoutes().
    /// This is the ONLY method that may be called from the audio thread.
    /// Thread safety: uses std::atomic<float>::store(relaxed) — no locks.
    void setRouteRMS(int routeIndex, float rms) noexcept
    {
        // NOTE: intentionally no JUCE_ASSERT_MESSAGE_THREAD — audio thread call site.
        if (routeIndex >= 0 && routeIndex < kMaxRMSSlots)
            routeRMS[static_cast<size_t>(routeIndex)].store(rms, std::memory_order_relaxed);
    }

    /// Force a route list refresh (call after external route mutations).
    /// Must be called from the message thread.
    void refresh()
    {
        JUCE_ASSERT_MESSAGE_THREAD
        cachedRoutes = couplingMatrix.getRoutes();
        rebuildLegendCache();
        repaint();
    }

    void rebuildLegendCache()
    {
        std::map<CouplingType, int> typeCounts;
        for (const auto& route : cachedRoutes)
            if (route.active && route.amount >= 0.001f)
                typeCounts[route.type]++;

        cachedLegendItems_.clear();
        for (const auto& [type, count] : typeCounts)
            cachedLegendItems_.push_back({
                CouplingTypeColors::forType(type),
                CouplingTypeColors::shortLabel(type) + " (" + juce::String(count) + ")"
            });
    }

    //--------------------------------------------------------------------------
    // Component — paint
    //--------------------------------------------------------------------------

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();

        // Shell background
        g.fillAll(juce::Colour(GalleryColors::shellWhite()));

        // Compute node positions for this frame's bounds
        computeNodePositions(bounds);

        // Draw existing coupling arcs (behind nodes)
        paintCouplingArcs(g);

        // Draw rubber-band drag arc (on top of static arcs, behind nodes)
        if (dragState == DragState::DraggingFromSource)
            paintDragArc(g);

        // Draw engine slot nodes (above arcs)
        paintNodes(g);

        // Legend strip at bottom
        paintLegend(g, bounds);

        if (hasKeyboardFocus(false))
            A11y::drawFocusRing(g, getLocalBounds().toFloat(), 4.0f);
    }

    void resized() override
    {
        // Node positions recomputed on every paint() from getLocalBounds().
        // Nothing to lay out — this component has no child components.
    }

    //--------------------------------------------------------------------------
    // Component — mouse interaction
    //--------------------------------------------------------------------------

    void mouseMove(const juce::MouseEvent& e) override
    {
        // Update hover state for nodes and ports.
        hoveredNode = hitTestNode(e.position);
        hoveredPort = hitTestPort(e.position);
        hoveredArc = hitTestArc(e.position);
        repaint();
    }

    void mouseDown(const juce::MouseEvent& e) override
    {
        if (e.mods.isRightButtonDown())
        {
            // Right-click on arc → context menu
            int arcIdx = hitTestArc(e.position);
            if (arcIdx >= 0)
                showArcContextMenu(arcIdx, e.position);
            return;
        }

        // Left-click: start drag from output port, or select arc
        PortHit port = hitTestPort(e.position);
        if (port.valid && port.isOutput)
        {
            // Begin drag
            dragState = DragState::DraggingFromSource;
            dragSourceSlot = port.slot;
            dragSourcePort = port.portIndex;
            dragCurrentPos = e.position;
            repaint();
        }
        else
        {
            // Select arc (if hit)
            selectedArc = hitTestArc(e.position);
            repaint();
        }
    }

    void mouseDrag(const juce::MouseEvent& e) override
    {
        if (dragState == DragState::DraggingFromSource)
        {
            dragCurrentPos = e.position;
            // Highlight valid drop targets
            hoveredPort = hitTestPort(e.position);
            repaint();
        }
    }

    void mouseUp(const juce::MouseEvent& e) override
    {
        if (dragState == DragState::DraggingFromSource)
        {
            PortHit target = hitTestPort(e.position);
            if (target.valid && !target.isOutput && target.slot != dragSourceSlot)
            {
                // Valid drop — show inline route-creation popup
                showAddRoutePopup(dragSourceSlot, target.slot, e.position);
            }

            // Cancel drag regardless
            dragState = DragState::Idle;
            repaint();
        }
    }

    void mouseExit(const juce::MouseEvent&) override
    {
        hoveredNode = -1;
        hoveredPort = {false, false, -1, -1};
        hoveredArc = -1;
        repaint();
    }

    //--------------------------------------------------------------------------
    // Component — keyboard
    //--------------------------------------------------------------------------

    bool keyPressed(const juce::KeyPress& key) override
    {
        // Delete / Backspace — remove selected user route
        if ((key == juce::KeyPress::deleteKey || key == juce::KeyPress::backspaceKey) && selectedArc >= 0 &&
            selectedArc < (int)cachedRoutes.size())
        {
            const auto& route = cachedRoutes[static_cast<size_t>(selectedArc)];
            if (!route.isNormalled)
                couplingMatrix.removeUserRoute(route.sourceSlot, route.destSlot, route.type);
            selectedArc = -1;
            refresh();
            return true;
        }

        // Escape — deselect / cancel drag
        if (key == juce::KeyPress::escapeKey)
        {
            selectedArc = -1;
            dragState = DragState::Idle;
            repaint();
            return true;
        }

        // 1–9 — set selected route amount
        if (selectedArc >= 0 && selectedArc < (int)cachedRoutes.size())
        {
            int digit = key.getKeyCode() - '0';
            if (digit >= 1 && digit <= 9)
            {
                auto& route = cachedRoutes[static_cast<size_t>(selectedArc)];
                float newAmount = static_cast<float>(digit) * 0.1f;
                // Rebuild route with new amount via remove + add
                MegaCouplingMatrix::CouplingRoute updated = route;
                updated.amount = newAmount;
                updated.isNormalled = false;
                if (!route.isNormalled)
                    couplingMatrix.removeUserRoute(route.sourceSlot, route.destSlot, route.type);
                couplingMatrix.addRoute(updated);
                refresh();
                return true;
            }
        }

        return false;
    }

private:
    //--------------------------------------------------------------------------
    // Timer — 30 Hz animation driver
    //--------------------------------------------------------------------------

    void timerCallback() override
    {
        if (!isVisible())
            return;

        // Snapshot route list once per tick (single atomic_load)
        cachedRoutes = couplingMatrix.getRoutes();
        rebuildLegendCache();

        // Check for any active routes before doing further work
        bool hasActive = false;
        for (const auto& r : cachedRoutes)
        {
            if (r.active && r.amount >= 0.001f)
            {
                hasActive = true;
                break;
            }
        }

        if (!hasActive)
        {
            // Step down to 1 Hz idle polling when no routes are active
            if (getTimerInterval() != 1000)
                startTimerHz(1);
            return;
        }

        // Wake back to full rate when active routes exist
        int targetHz = reducedMotion ? 10 : 30;
        if (getTimerInterval() > 100)
            startTimerHz(targetHz);

        // Snapshot per-route RMS for arc brightness (relaxed loads)
        for (int i = 0; i < kMaxRMSSlots; ++i)
            cachedRMS[i] = routeRMS[static_cast<size_t>(i)].load(std::memory_order_relaxed);

        // Advance animation phase (wraps at 2π)
        animPhase = std::fmod(animPhase + 0.12f, juce::MathConstants<float>::twoPi);

        repaint();
    }

    //--------------------------------------------------------------------------
    // Geometry — node positions in diamond layout
    //--------------------------------------------------------------------------

    void computeNodePositions(juce::Rectangle<float> bounds)
    {
        float cx = bounds.getCentreX();
        float cy = bounds.getCentreY() - 16.0f; // shift up slightly for legend
        float r = std::min(bounds.getWidth(), bounds.getHeight()) * kOrbitFraction;

        nodePos[0] = {cx, cy - r}; // top
        nodePos[1] = {cx - r, cy}; // left
        nodePos[2] = {cx + r, cy}; // right
        nodePos[3] = {cx, cy + r}; // bottom
    }

    //--------------------------------------------------------------------------
    // Painting — nodes
    //--------------------------------------------------------------------------

    void paintNodes(juce::Graphics& g)
    {
        for (int i = 0; i < MegaCouplingMatrix::MaxSlots; ++i)
        {
            auto centre = nodePos[i];
            auto name = getSlotName(i);
            auto accent = getSlotColor(i);
            bool hasEng = name.isNotEmpty();
            bool hovered = (hoveredNode == i);
            bool isDragTarget = (dragState == DragState::DraggingFromSource && hoveredPort.valid &&
                                 hoveredPort.slot == i && !hoveredPort.isOutput);

            // --- Glow halo (active engine pulsing) ---
            if (hasEng)
            {
                float rmsSum = 0.0f;
                int routeCount = 0;
                for (int ri = 0; ri < (int)cachedRoutes.size() && ri < kMaxRMSSlots; ++ri)
                {
                    const auto& rt = cachedRoutes[static_cast<size_t>(ri)];
                    if (rt.sourceSlot == i || rt.destSlot == i)
                    {
                        rmsSum += cachedRMS[ri];
                        ++routeCount;
                    }
                }
                float avgRMS = routeCount > 0 ? rmsSum / routeCount : 0.0f;
                float glowAlpha = reducedMotion ? 0.08f : 0.06f + avgRMS * 0.20f;
                float glowR = kNodeRadius * 1.6f;
                g.setColour(accent.withAlpha(glowAlpha));
                g.fillEllipse(centre.x - glowR, centre.y - glowR, glowR * 2, glowR * 2);
            }

            // --- Node fill ---
            auto fillColour = hasEng ? accent.withAlpha(0.12f) : juce::Colour(GalleryColors::emptySlot());
            g.setColour(fillColour);
            g.fillEllipse(centre.x - kNodeRadius, centre.y - kNodeRadius, kNodeRadius * 2, kNodeRadius * 2);

            // --- Node ring ---
            float ringWidth = hovered ? 2.5f : (isDragTarget ? 3.0f : 1.8f);
            auto ringColour = isDragTarget ? juce::Colour(GalleryColors::xoGold)
                                           : (hasEng ? accent : juce::Colour(GalleryColors::borderGray()));
            g.setColour(ringColour);
            g.drawEllipse(centre.x - kNodeRadius, centre.y - kNodeRadius, kNodeRadius * 2, kNodeRadius * 2, ringWidth);

            // --- Slot label ---
            g.setFont(GalleryFonts::heading(10.0f)); // (#885: 8/9pt→10pt legibility floor)
            if (hasEng)
            {
                g.setColour(juce::Colour(GalleryColors::textDark()));
                g.drawText(name, (int)(centre.x - kNodeRadius), (int)(centre.y - 7), (int)(kNodeRadius * 2), 14,
                           juce::Justification::centred);
            }
            else
            {
                // Slot labels are fixed ("SLOT 1"…"SLOT 5") — use static cache.
                static const juce::String kSlotLabels[] = {
                    "SLOT 1", "SLOT 2", "SLOT 3", "SLOT 4", "SLOT 5"
                };
                const juce::String& slotLabel = (i >= 0 && i < 5)
                    ? kSlotLabels[static_cast<size_t>(i)]
                    : ("SLOT " + juce::String(i + 1));
                g.setColour(juce::Colour(GalleryColors::textMid()));
                g.drawText(slotLabel, (int)(centre.x - kNodeRadius), (int)(centre.y - 5),
                           (int)(kNodeRadius * 2), 10, juce::Justification::centred);
            }

            // --- Output / input ports ---
            paintPorts(g, i, centre, accent, hasEng);
        }
    }

    // Paint the 8 output (top half) and input (bottom half) ports around a node.
    void paintPorts(juce::Graphics& g, int slot, juce::Point<float> centre, juce::Colour accent, bool hasEng)
    {
        if (!hasEng)
            return; // only show ports when an engine is loaded

        for (int p = 0; p < kNumPorts; ++p)
        {
            float angle = portAngle(p); // radians
            bool isOut = portIsOutput(p);
            auto pos = portPosition(centre, angle);

            bool hov = (hoveredPort.valid && hoveredPort.slot == slot && hoveredPort.portIndex == p);
            bool isDT = (dragState == DragState::DraggingFromSource && hoveredPort.valid && hoveredPort.slot == slot &&
                         hoveredPort.portIndex == p && !isOut);

            float r = hov ? kPortHoverRadius : kPortRadius;
            auto c = isDT ? juce::Colour(GalleryColors::xoGold) : (isOut ? accent : accent.withAlpha(0.6f));

            g.setColour(c);
            g.fillEllipse(pos.x - r, pos.y - r, r * 2, r * 2);

            // Small ring to distinguish from node body
            g.setColour(c.brighter(0.3f));
            g.drawEllipse(pos.x - r, pos.y - r, r * 2, r * 2, 1.0f);
        }
    }

    //--------------------------------------------------------------------------
    // Painting — coupling arcs
    //--------------------------------------------------------------------------

    void paintCouplingArcs(juce::Graphics& g)
    {
        // Count routes per slot pair for stagger computation
        struct PairCount
        {
            int count = 0;
            int currentIdx = 0;
        };
        std::array<std::array<PairCount, MegaCouplingMatrix::MaxSlots>, MegaCouplingMatrix::MaxSlots> pairCounts{};

        // First pass: count routes per pair
        for (const auto& route : cachedRoutes)
        {
            if (!route.active || route.amount < 0.001f)
                continue;
            if (!slotInRange(route.sourceSlot) || !slotInRange(route.destSlot))
                continue;
            pairCounts[route.sourceSlot][route.destSlot].count++;
        }

        // Second pass: draw
        int routeIdx = 0;
        for (const auto& route : cachedRoutes)
        {
            bool shouldDraw =
                route.active && route.amount >= 0.001f && slotInRange(route.sourceSlot) && slotInRange(route.destSlot);

            if (shouldDraw)
            {
                auto& pc = pairCounts[route.sourceSlot][route.destSlot];
                float stagger = computeStagger(pc.currentIdx, pc.count);
                pc.currentIdx++;

                float rms = routeIdx < kMaxRMSSlots ? cachedRMS[routeIdx] : 0.0f;
                bool selected = (selectedArc == routeIdx);

                paintSingleArc(g, route, stagger, rms, selected);
            }

            ++routeIdx;
        }
    }

    void paintSingleArc(juce::Graphics& g, const MegaCouplingMatrix::CouplingRoute& route, float stagger, float rms,
                        bool selected)
    {
        auto srcCentre = nodePos[route.sourceSlot];
        auto dstCentre = nodePos[route.destSlot];

        // Compute source output port and dest input port positions.
        // For simplicity in the scaffold, use node-edge midpoints offset toward
        // the target. Full port matching is a V2 refinement.
        float toDestAngle = std::atan2(dstCentre.y - srcCentre.y, dstCentre.x - srcCentre.x);
        float fromSrcAngle = toDestAngle + juce::MathConstants<float>::pi;

        juce::Point<float> srcPt{srcCentre.x + std::cos(toDestAngle) * kNodeRadius,
                                 srcCentre.y + std::sin(toDestAngle) * kNodeRadius};
        juce::Point<float> dstPt{dstCentre.x + std::cos(fromSrcAngle) * kNodeRadius,
                                 dstCentre.y + std::sin(fromSrcAngle) * kNodeRadius};

        // Perpendicular offset for curvature
        juce::Point<float> chord = dstPt - srcPt;
        float chordLen = std::sqrt(chord.x * chord.x + chord.y * chord.y);
        if (chordLen < 1.0f)
            return;

        juce::Point<float> perp{-chord.y / chordLen, chord.x / chordLen};
        float curveOffset = chordLen * (kArcCurvature + stagger);

        juce::Point<float> p1{srcPt.x + chord.x * 0.33f + perp.x * curveOffset,
                              srcPt.y + chord.y * 0.33f + perp.y * curveOffset};
        juce::Point<float> p2{srcPt.x + chord.x * 0.67f + perp.x * curveOffset,
                              srcPt.y + chord.y * 0.67f + perp.y * curveOffset};

        juce::Path arc;
        arc.startNewSubPath(srcPt.x, srcPt.y);
        arc.cubicTo(p1.x, p1.y, p2.x, p2.y, dstPt.x, dstPt.y);

        // For KnotTopology — draw reverse arc too
        juce::Path reverseArc;
        bool isKnot = (route.type == CouplingType::KnotTopology);
        if (isKnot)
        {
            reverseArc.startNewSubPath(dstPt.x, dstPt.y);
            reverseArc.cubicTo(p2.x, p2.y, p1.x, p1.y, srcPt.x, srcPt.y);
        }

        // Visual parameters
        auto typeColour = CouplingTypeColors::forType(route.type);

        float pulse = reducedMotion ? 0.6f + route.amount * 0.4f : 0.6f + rms * 0.4f + std::sin(animPhase) * 0.05f;
        pulse = juce::jlimit(0.0f, 1.0f, pulse);

        float baseAlpha = (0.25f + route.amount * 0.55f) * pulse;
        float strokeW = 1.5f + route.amount * 2.5f;

        if (selected)
        {
            baseAlpha = juce::jlimit(0.0f, 1.0f, baseAlpha * 1.4f);
            strokeW *= 1.3f;
        }

        // Glow pass (3× width, 12% opacity)
        g.setColour(typeColour.withAlpha(baseAlpha * 0.12f));
        g.strokePath(arc,
                     juce::PathStrokeType(strokeW * 3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // Line pass
        if (route.isNormalled)
        {
            // Dashed stroke for normalled routes
            float dashLengths[2] = {4.0f, 4.0f};
            juce::PathStrokeType stroke(strokeW * 0.7f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded);
            juce::Path dashed;
            stroke.createDashedStroke(dashed, arc, dashLengths, 2);
            g.setColour(typeColour.withAlpha(baseAlpha * 0.5f));
            g.fillPath(dashed);
        }
        else
        {
            g.setColour(typeColour.withAlpha(baseAlpha));
            g.strokePath(arc,
                         juce::PathStrokeType(strokeW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }

        // KnotTopology reverse arc (phase-offset pulse)
        if (isKnot && !route.isNormalled)
        {
            float revPulse = 0.6f + rms * 0.4f + std::sin(animPhase + juce::MathConstants<float>::pi) * 0.05f;
            revPulse = juce::jlimit(0.0f, 1.0f, revPulse);
            g.setColour(typeColour.withAlpha(baseAlpha * revPulse * 0.6f));
            g.strokePath(reverseArc, juce::PathStrokeType(strokeW * 0.6f, juce::PathStrokeType::curved,
                                                          juce::PathStrokeType::rounded));
        }

        // Arrowhead at destination
        paintArrowhead(g, p2, dstPt, typeColour.withAlpha(baseAlpha), route.amount);

        // Type swatch + label at arc midpoint
        {
            float midX = srcPt.x + chord.x * 0.5f + perp.x * curveOffset * 0.9f;
            float midY = srcPt.y + chord.y * 0.5f + perp.y * curveOffset * 0.9f;

            // Colour swatch (6×6 filled circle)
            g.setColour(typeColour.withAlpha(0.9f));
            g.fillEllipse(midX - 4, midY - 4, 8, 8);

            // Short label
            g.setColour(juce::Colour(GalleryColors::textDark()));
            g.setFont(GalleryFonts::heading(10.0f)); // (#885: 8pt→10pt legibility floor)
            g.drawText(CouplingTypeColors::shortLabel(route.type), (int)(midX + 6), (int)(midY - 5), 38, 10,
                       juce::Justification::left);
        }
    }

    void paintArrowhead(juce::Graphics& g, juce::Point<float> controlPt, juce::Point<float> tip, juce::Colour colour,
                        float amount)
    {
        float angle = std::atan2(tip.y - controlPt.y, tip.x - controlPt.x);
        float sz = 6.0f + amount * 4.0f;
        float spread = 0.45f; // half-angle in radians

        juce::Point<float> left{tip.x - sz * std::cos(angle - spread), tip.y - sz * std::sin(angle - spread)};
        juce::Point<float> right{tip.x - sz * std::cos(angle + spread), tip.y - sz * std::sin(angle + spread)};

        juce::Path arrow;
        arrow.startNewSubPath(tip.x, tip.y);
        arrow.lineTo(left.x, left.y);
        arrow.lineTo(right.x, right.y);
        arrow.closeSubPath();

        g.setColour(colour);
        g.fillPath(arrow);
    }

    //--------------------------------------------------------------------------
    // Painting — drag rubber-band arc
    //--------------------------------------------------------------------------

    void paintDragArc(juce::Graphics& g)
    {
        auto srcCentre = nodePos[dragSourceSlot];
        float toMouseAngle = std::atan2(dragCurrentPos.y - srcCentre.y, dragCurrentPos.x - srcCentre.x);

        juce::Point<float> srcPt{srcCentre.x + std::cos(toMouseAngle) * kNodeRadius,
                                 srcCentre.y + std::sin(toMouseAngle) * kNodeRadius};

        auto colour = CouplingTypeColors::forType(lastUsedType);

        if (reducedMotion)
        {
            // Straight line in reduced motion mode
            g.setColour(colour.withAlpha(0.5f));
            g.drawLine(srcPt.x, srcPt.y, dragCurrentPos.x, dragCurrentPos.y, 2.0f);
        }
        else
        {
            juce::Path arc;
            arc.startNewSubPath(srcPt.x, srcPt.y);

            juce::Point<float> chord = dragCurrentPos - srcPt;
            float chordLen = std::sqrt(chord.x * chord.x + chord.y * chord.y);
            if (chordLen < 2.0f)
                return;

            juce::Point<float> perp{-chord.y / chordLen, chord.x / chordLen};
            float curveOffset = chordLen * 0.30f;

            arc.cubicTo(srcPt.x + chord.x * 0.33f + perp.x * curveOffset,
                        srcPt.y + chord.y * 0.33f + perp.y * curveOffset,
                        srcPt.x + chord.x * 0.67f + perp.x * curveOffset,
                        srcPt.y + chord.y * 0.67f + perp.y * curveOffset, dragCurrentPos.x, dragCurrentPos.y);

            // Glow
            g.setColour(colour.withAlpha(0.10f));
            g.strokePath(arc, juce::PathStrokeType(8.0f, juce::PathStrokeType::curved));

            // Line
            g.setColour(colour.withAlpha(0.65f));
            g.strokePath(arc, juce::PathStrokeType(2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }

        // Invalid target indicator (red X) — shown on invalid port hover
        if (hoveredPort.valid && (hoveredPort.isOutput || hoveredPort.slot == dragSourceSlot))
        {
            g.setColour(juce::Colours::red.withAlpha(0.8f));
            float x = dragCurrentPos.x;
            float y = dragCurrentPos.y;
            g.drawLine(x - 6, y - 6, x + 6, y + 6, 2.0f);
            g.drawLine(x + 6, y - 6, x - 6, y + 6, 2.0f);
        }
    }

    //--------------------------------------------------------------------------
    // Painting — legend strip
    //--------------------------------------------------------------------------

    void paintLegend(juce::Graphics& g, juce::Rectangle<float> bounds)
    {
        auto stripBounds = bounds.removeFromBottom(22.0f);
        g.setColour(juce::Colour(GalleryColors::borderGray()));
        g.drawHorizontalLine((int)stripBounds.getY(), stripBounds.getX(), stripBounds.getRight());

        float x = stripBounds.getX() + 8.0f;
        float cy = stripBounds.getCentreY();

        // Use pre-built legend items (rebuilt in refresh() — no String alloc in paint).
        for (const auto& item : cachedLegendItems_)
        {
            g.setColour(item.colour);
            g.fillEllipse(x, cy - 4.0f, 8.0f, 8.0f);

            g.setColour(juce::Colour(GalleryColors::textMid()));
            g.setFont(GalleryFonts::body(10.0f)); // (#885: 8pt→10pt legibility floor)
            g.drawText(item.label, (int)(x + 11), (int)(cy - 5), 60, 10, juce::Justification::left);

            x += 72.0f;
            if (x > stripBounds.getRight() - 70.0f)
                break; // wrap guard
        }
    }

    //--------------------------------------------------------------------------
    // Hit testing
    //--------------------------------------------------------------------------

    int hitTestNode(juce::Point<float> pos) const
    {
        for (int i = 0; i < MegaCouplingMatrix::MaxSlots; ++i)
        {
            auto d = pos - nodePos[i];
            if (std::sqrt(d.x * d.x + d.y * d.y) <= kNodeRadius)
                return i;
        }
        return -1;
    }

    struct PortHit
    {
        bool valid = false;
        bool isOutput = false;
        int slot = -1;
        int portIndex = -1;
    };

    PortHit hitTestPort(juce::Point<float> pos) const
    {
        for (int slot = 0; slot < MegaCouplingMatrix::MaxSlots; ++slot)
        {
            if (getSlotName(slot).isEmpty())
                continue;

            for (int p = 0; p < kNumPorts; ++p)
            {
                auto portPos = portPosition(nodePos[slot], portAngle(p));
                auto d = pos - portPos;
                float dist = std::sqrt(d.x * d.x + d.y * d.y);
                if (dist <= kPortHoverRadius * 1.5f)
                    return {true, portIsOutput(p), slot, p};
            }
        }
        return {};
    }

    // Returns index into cachedRoutes, or -1.
    int hitTestArc(juce::Point<float> pos) const
    {
        // Sample each arc at 64 t-values and check distance.
        int routeIdx = 0;
        for (const auto& route : cachedRoutes)
        {
            if (route.active && route.amount >= 0.001f && slotInRange(route.sourceSlot) && slotInRange(route.destSlot))
            {
                if (isNearArc(pos, route))
                    return routeIdx;
            }
            ++routeIdx;
        }
        return -1;
    }

    // Evaluate arc proximity (64-point sampling).
    bool isNearArc(juce::Point<float> pos, const MegaCouplingMatrix::CouplingRoute& route) const
    {
        auto srcC = nodePos[route.sourceSlot];
        auto dstC = nodePos[route.destSlot];
        float toDA = std::atan2(dstC.y - srcC.y, dstC.x - srcC.x);
        float fromA = toDA + juce::MathConstants<float>::pi;

        juce::Point<float> p0{srcC.x + std::cos(toDA) * kNodeRadius, srcC.y + std::sin(toDA) * kNodeRadius};
        juce::Point<float> p3{dstC.x + std::cos(fromA) * kNodeRadius, dstC.y + std::sin(fromA) * kNodeRadius};

        juce::Point<float> chord = p3 - p0;
        float chordLen = std::sqrt(chord.x * chord.x + chord.y * chord.y);
        if (chordLen < 1.0f)
            return false;

        juce::Point<float> perp{-chord.y / chordLen, chord.x / chordLen};
        float curveOffset = chordLen * kArcCurvature;

        juce::Point<float> p1{p0.x + chord.x * 0.33f + perp.x * curveOffset,
                              p0.y + chord.y * 0.33f + perp.y * curveOffset};
        juce::Point<float> p2{p0.x + chord.x * 0.67f + perp.x * curveOffset,
                              p0.y + chord.y * 0.67f + perp.y * curveOffset};

        constexpr int kSamples = 64;
        for (int i = 0; i <= kSamples; ++i)
        {
            float t = static_cast<float>(i) / kSamples;
            float mt = 1.0f - t;

            // Cubic Bézier evaluation
            float bx = mt * mt * mt * p0.x + 3 * mt * mt * t * p1.x + 3 * mt * t * t * p2.x + t * t * t * p3.x;
            float by = mt * mt * mt * p0.y + 3 * mt * mt * t * p1.y + 3 * mt * t * t * p2.y + t * t * t * p3.y;

            float dx = pos.x - bx;
            float dy = pos.y - by;
            if (std::sqrt(dx * dx + dy * dy) <= kArcHitRadius)
                return true;
        }
        return false;
    }

    //--------------------------------------------------------------------------
    // Port geometry helpers
    //--------------------------------------------------------------------------

    // 8 ports: indices 0–3 are outputs (top half), 4–7 are inputs (bottom half)
    static constexpr int kNumPorts = 8;

    static float portAngle(int portIndex)
    {
        // Outputs: 270° to 90° (top arc) in 4 steps → angles -π/2 to π/2
        // Inputs:  90° to 270° (bottom arc) in 4 steps → angles π/2 to 3π/2
        constexpr float pi = juce::MathConstants<float>::pi;
        if (portIndex < 4)
        {
            // Output ports: -π/2, -π/6, π/6, π/2
            float steps[4] = {-pi * 0.5f, -pi * 0.167f, pi * 0.167f, pi * 0.5f};
            return steps[portIndex];
        }
        else
        {
            // Input ports: π/2, 5π/6, 7π/6, 3π/2
            float steps[4] = {pi * 0.5f, pi * 0.833f, pi * 1.167f, pi * 1.5f};
            return steps[portIndex - 4];
        }
    }

    static bool portIsOutput(int portIndex) { return portIndex < 4; }

    static juce::Point<float> portPosition(juce::Point<float> nodeCenter, float angle)
    {
        float portOrbit = kNodeRadius + 8.0f;
        return {nodeCenter.x + std::cos(angle) * portOrbit, nodeCenter.y + std::sin(angle) * portOrbit};
    }

    //--------------------------------------------------------------------------
    // Arc stagger helpers
    //--------------------------------------------------------------------------

    // Stagger offsets for multi-route slot pairs so arcs don't overlap.
    // currentIdx is the 0-based index of this route within the pair, count is total.
    static float computeStagger(int currentIdx, int count)
    {
        if (count <= 1)
            return 0.0f;
        // Centre the stagger around 0: offset = (currentIdx - (count-1)/2) * kArcStagger
        float centre = (count - 1) * 0.5f;
        return (static_cast<float>(currentIdx) - centre) * kArcStagger;
    }

    //--------------------------------------------------------------------------
    // Interaction helpers
    //--------------------------------------------------------------------------

    static bool slotInRange(int slot) { return slot >= 0 && slot < MegaCouplingMatrix::MaxSlots; }

    //--------------------------------------------------------------------------
    // Menus and popups (scaffold — implementation bodies omitted in V1)
    //--------------------------------------------------------------------------

    void showArcContextMenu(int arcIdx, juce::Point<float> /*pos*/)
    {
        if (arcIdx < 0 || arcIdx >= (int)cachedRoutes.size())
            return;
        const auto& route = cachedRoutes[static_cast<size_t>(arcIdx)];

        juce::PopupMenu menu;
        menu.addSectionHeader(CouplingTypeColors::displayName(route.type) + ": Slot " +
                              juce::String(route.sourceSlot + 1) + " -> Slot " + juce::String(route.destSlot + 1) +
                              "  (amt: " + juce::String(route.amount, 2) + ")");
        menu.addItem(1, "Set Amount...");

        juce::PopupMenu typeMenu;
        int itemId = 100;
        for (auto t : CouplingTypeColors::allTypes())
        {
            typeMenu.addItem(itemId++, CouplingTypeColors::displayName(t), true, t == route.type);
        }
        menu.addSubMenu("Change Type", typeMenu);
        menu.addSeparator();
        menu.addItem(2, "Delete Route", !route.isNormalled);

        // Capture route fields needed by the async callbacks.
        const int routeIdx = arcIdx;
        const int srcSlot = route.sourceSlot;
        const int dstSlot = route.destSlot;
        const CouplingType routeType = route.type;

        menu.showMenuAsync(
            juce::PopupMenu::Options().withTargetComponent(this),
            [this, routeIdx, srcSlot, dstSlot, routeType](int result)
            {
                if (result == 1)
                {
                    // Set Amount — show an AlertWindow with a text editor.
                    // takeOwnership=true: JUCE deletes dialog when modal session ends.
                    // Do NOT call delete dialog inside the callback.
                    auto* dialog = new juce::AlertWindow(
                        "Set Coupling Amount", "Enter a value between 0.0 and 1.0:", juce::MessageBoxIconType::NoIcon,
                        this);
                    dialog->addTextEditor("amount",
                                          juce::String((routeIdx >= 0 && routeIdx < (int)cachedRoutes.size())
                                                           ? cachedRoutes[static_cast<size_t>(routeIdx)].amount
                                                           : 0.5f,
                                                       2));
                    dialog->addButton("OK", 1);
                    dialog->addButton("Cancel", 0);

                    dialog->enterModalState(
                        true,
                        juce::ModalCallbackFunction::create(
                            [safeThis = juce::Component::SafePointer<CouplingVisualizer>(this), dialog, srcSlot,
                             dstSlot, routeType](int r)
                            {
                                if (r == 1 && safeThis)
                                {
                                    float newAmt = juce::jlimit(
                                        0.0f, 1.0f, dialog->getTextEditorContents("amount").getFloatValue());

                                    // Replace the route with the updated amount.
                                    auto routes = safeThis->couplingMatrix.getRoutes();
                                    for (auto& ro : routes)
                                    {
                                        if (ro.sourceSlot == srcSlot && ro.destSlot == dstSlot &&
                                            ro.type == routeType && !ro.isNormalled)
                                        {
                                            safeThis->couplingMatrix.removeUserRoute(srcSlot, dstSlot, routeType);
                                            ro.amount = newAmt;
                                            safeThis->couplingMatrix.addRoute(ro);
                                            break;
                                        }
                                    }
                                    safeThis->refresh();
                                }
                                // dialog is owned by JUCE (takeOwnership=true) — do NOT delete here.
                            }),
                        true /*takeOwnership — JUCE deletes dialog when modal session ends*/);
                }
                else if (result >= 100)
                {
                    // Change Type — result 100+ maps to allTypes() index.
                    auto types = CouplingTypeColors::allTypes();
                    int idx = result - 100;
                    if (idx >= 0 && idx < (int)types.size())
                    {
                        CouplingType newType = types[static_cast<size_t>(idx)];
                        if (newType != routeType)
                        {
                            auto routes = couplingMatrix.getRoutes();
                            for (auto ro : routes)
                            {
                                if (ro.sourceSlot == srcSlot && ro.destSlot == dstSlot && ro.type == routeType &&
                                    !ro.isNormalled)
                                {
                                    couplingMatrix.removeUserRoute(srcSlot, dstSlot, routeType);
                                    ro.type = newType;
                                    couplingMatrix.addRoute(ro);
                                    lastUsedType = newType;
                                    break;
                                }
                            }
                            refresh();
                        }
                    }
                }
                else if (result == 2)
                {
                    // Delete Route.
                    couplingMatrix.removeUserRoute(srcSlot, dstSlot, routeType);
                    refresh();
                }
            });
    }

    void showAddRoutePopup(int srcSlot, int dstSlot, juce::Point<float> /*pos*/)
    {
        // Show a type-selector popup so the user picks the coupling type
        // and amount before the route is added.
        juce::PopupMenu typeMenu;
        int itemId = 1;
        for (auto t : CouplingTypeColors::allTypes())
            typeMenu.addItem(itemId++, CouplingTypeColors::displayName(t));

        typeMenu.showMenuAsync(
            juce::PopupMenu::Options().withTargetComponent(this),
            [this, srcSlot, dstSlot](int result)
            {
                if (result <= 0)
                    return; // user dismissed

                auto types = CouplingTypeColors::allTypes();
                int idx = result - 1;
                if (idx < 0 || idx >= (int)types.size())
                    return;

                CouplingType chosenType = types[static_cast<size_t>(idx)];

                // Ask for amount via an AlertWindow.
                // takeOwnership=true: JUCE deletes dialog when modal session ends.
                // Do NOT call delete dialog inside the callback.
                auto* dialog = new juce::AlertWindow(
                    "Set Coupling Amount", "Enter a value between 0.0 and 1.0:", juce::MessageBoxIconType::NoIcon,
                    this);
                dialog->addTextEditor("amount", "0.50");
                dialog->addButton("Add", 1);
                dialog->addButton("Cancel", 0);

                dialog->enterModalState(
                    true,
                    juce::ModalCallbackFunction::create(
                        [safeThis = juce::Component::SafePointer<CouplingVisualizer>(this), dialog, srcSlot, dstSlot,
                         chosenType](int r)
                        {
                            if (r == 1 && safeThis)
                            {
                                float amt =
                                    juce::jlimit(0.0f, 1.0f, dialog->getTextEditorContents("amount").getFloatValue());

                                MegaCouplingMatrix::CouplingRoute newRoute;
                                newRoute.sourceSlot = srcSlot;
                                newRoute.destSlot = dstSlot;
                                newRoute.type = chosenType;
                                newRoute.amount = amt;
                                newRoute.isNormalled = false;
                                newRoute.active = true;

                                safeThis->couplingMatrix.addRoute(newRoute);
                                safeThis->lastUsedType = chosenType;
                                safeThis->refresh();
                            }
                            // dialog is owned by JUCE (takeOwnership=true) — do NOT delete here.
                        }),
                    true /*takeOwnership — JUCE deletes dialog when modal session ends*/);
            });
    }

    //--------------------------------------------------------------------------
    // State
    //--------------------------------------------------------------------------

    MegaCouplingMatrix& couplingMatrix;
    std::function<juce::String(int)> getSlotName;
    std::function<juce::Colour(int)> getSlotColor;

    // Route snapshot — refreshed once per timerCallback()
    std::vector<MegaCouplingMatrix::CouplingRoute> cachedRoutes;

    // Cached legend items — rebuilt in refresh() alongside cachedRoutes.
    // Each entry holds the dot colour and the pre-built label string.
    struct LegendItem { juce::Colour colour; juce::String label; };
    std::vector<LegendItem> cachedLegendItems_;

    // Per-route RMS — written by audio thread, read by message thread
    std::array<std::atomic<float>, kMaxRMSSlots> routeRMS;
    float cachedRMS[kMaxRMSSlots] = {}; // snapshot read each timerCallback

    // Node positions recomputed each paint()
    std::array<juce::Point<float>, MegaCouplingMatrix::MaxSlots> nodePos{};

    // Animation
    float animPhase = 0.0f;
    bool reducedMotion = false;

    // Hover / selection state
    int hoveredNode = -1;
    PortHit hoveredPort{};
    int hoveredArc = -1;
    int selectedArc = -1;

    // Drag state machine
    enum class DragState
    {
        Idle,
        DraggingFromSource
    };
    DragState dragState = DragState::Idle;
    int dragSourceSlot = -1;
    int dragSourcePort = -1;
    juce::Point<float> dragCurrentPos{};

    // Last coupling type used (for drag rubber-band colour and new-route default)
    CouplingType lastUsedType = CouplingType::AmpToFilter;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CouplingVisualizer)
};

} // namespace xoceanus
