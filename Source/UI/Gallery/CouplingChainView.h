#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "../../XOlokunProcessor.h"
#include "../../Core/MegaCouplingMatrix.h"
#include "../GalleryColors.h"
namespace xolokun {

// Local couplingTypeLabel — avoids circular include with OverviewPanel.h
#ifndef XOLOKUN_COUPLING_TYPE_LABEL_DEFINED
#define XOLOKUN_COUPLING_TYPE_LABEL_DEFINED
inline juce::String couplingTypeLabel(CouplingType t)
{
    switch (t) {
        case CouplingType::AmpToFilter:      return "Amp->F";
        case CouplingType::AmpToPitch:       return "Amp->P";
        case CouplingType::LFOToPitch:       return "LFO->P";
        case CouplingType::EnvToMorph:       return "Env->M";
        case CouplingType::AudioToFM:        return "Au->FM";
        case CouplingType::AudioToRing:      return "Ring";
        case CouplingType::FilterToFilter:   return "F->F";
        case CouplingType::AmpToChoke:       return "Choke";
        case CouplingType::RhythmToBlend:    return "R->B";
        case CouplingType::EnvToDecay:       return "Env->D";
        case CouplingType::PitchToPitch:     return "P->P";
        case CouplingType::AudioToWavetable: return "Au->W";
        default:                             return "?";
    }
}
#endif

//==============================================================================
// CouplingChainView — 520×48pt horizontal signal-flow diagram.
//
// Renders the active audio routing path left-to-right:
//   [ENGINE A] → [coupling type arrow] → [ENGINE B] → ... → [FX] → [OUT]
//
// The view makes the "engine-as-insert" concept immediately understandable by
// flattening the coupling graph into a left-to-right chain ordered by slot.
// If routing forms a cycle, all engines in the cycle are shown and the return
// arc is marked with a small curved arrow.
//
// Usage:
//   - Construct with the XOlokunProcessor reference.
//   - Call refresh() from OverviewPanel::refresh() or an editor timer.
//   - Set bounds to 520×48 (or proportionally scaled).
//   - No timer needed — refreshed externally; no allocation in paint().
//
// Layout (fixed slot geometry):
//   Each engine node    : 64×32pt rounded rect, 2px border in accent colour
//   Inter-node gap      : 32pt containing arrow + type label
//   FX node             : 40×32pt, XO Gold border
//   OUT node            : 32×32pt, textMid border, speaker glyph
//
// Color coding (mirrors CouplingArcOverlay palette):
//   Audio routes  (FM / Ring / Wavetable / Buffer)  → Twilight Blue   #0096C7
//   Modulation    (Amp / LFO / Env / Filter / Pitch) → XO Gold        #E9C46A
//   KnotTopology  (bidirectional entanglement)        → Midnight Violet #7B2FBE
//   TriangularCoupling                               → Hibiscus Pink  #C9377A
//
class CouplingChainView : public juce::Component
{
public:
    explicit CouplingChainView(XOlokunProcessor& proc)
        : processor(proc)
    {
        setInterceptsMouseClicks(false, false);
    }

    // Call from OverviewPanel::refresh() or editor timer (message thread only).
    // Snapshots all coupling state here so paint() performs zero allocation.
    void refresh()
    {
        nodes.clear();
        links.clear();
        hasCycle = false;

        // ── 1. Collect active engines ordered by slot ────────────────────────
        // MaxSlots is 5 (4 primary + Ghost Slot); guard both processor and spec.
        constexpr int kMaxSlots = XOlokunProcessor::MaxSlots; // 5 as of 2026-03-25

        struct SlotInfo {
            int            slot;
            juce::String   name;
            juce::Colour   accent;
        };
        std::vector<SlotInfo> active;
        active.reserve(static_cast<size_t>(kMaxSlots));

        for (int i = 0; i < kMaxSlots; ++i)
        {
            auto* eng = processor.getEngine(i);
            if (eng)
                active.push_back({ i, eng->getEngineId(), eng->getAccentColour() });
        }

        if (active.empty())
            return; // empty state; paint() will render placeholder text

        // ── 2. Snapshot routes ───────────────────────────────────────────────
        const auto routes = processor.getCouplingMatrix().getRoutes();

        // ── 3. Build chain order: source slots with no incoming active routes
        //        go first. Remaining slots follow by slot number.
        //        For cycle detection we track visited slots.
        // ─────────────────────────────────────────────────────────────────────

        // Map slot → SlotInfo for quick lookup
        std::array<const SlotInfo*, kMaxSlots> bySlot {};
        for (const auto& s : active)
            bySlot[static_cast<size_t>(s.slot)] = &s;

        // Count how many active routes point INTO each slot
        std::array<int, kMaxSlots> inDegree {};
        for (const auto& r : routes)
        {
            if (!r.active || r.amount < 0.001f) continue;
            if (r.sourceSlot < 0 || r.sourceSlot >= kMaxSlots) continue;
            if (r.destSlot   < 0 || r.destSlot   >= kMaxSlots) continue;
            if (!bySlot[static_cast<size_t>(r.sourceSlot)]) continue;
            if (!bySlot[static_cast<size_t>(r.destSlot)])   continue;
            inDegree[static_cast<size_t>(r.destSlot)]++;
        }

        // Topological sort (Kahn's algorithm — detects cycles gracefully)
        std::vector<int> order; // slot indices in chain order
        order.reserve(static_cast<size_t>(active.size()));

        // Start with sources (in-degree 0 among active slots)
        std::vector<int> queue;
        for (const auto& s : active)
            if (inDegree[static_cast<size_t>(s.slot)] == 0)
                queue.push_back(s.slot);

        // If everything has incoming routes (pure cycle), fall back to slot order
        if (queue.empty())
        {
            hasCycle = true;
            for (const auto& s : active)
                queue.push_back(s.slot);
        }

        std::array<bool, kMaxSlots> visited {};
        std::array<int,  kMaxSlots> tempDegree = inDegree;

        while (!queue.empty())
        {
            // Pick the lowest slot number from the queue for determinism
            auto minIt = std::min_element(queue.begin(), queue.end());
            int slot = *minIt;
            queue.erase(minIt);

            if (visited[static_cast<size_t>(slot)]) continue;
            visited[static_cast<size_t>(slot)] = true;
            order.push_back(slot);

            // Reduce in-degree of slots this one routes to
            for (const auto& r : routes)
            {
                if (!r.active || r.amount < 0.001f) continue;
                if (r.sourceSlot != slot) continue;
                if (r.destSlot < 0 || r.destSlot >= kMaxSlots) continue;
                if (!bySlot[static_cast<size_t>(r.destSlot)]) continue;
                if (visited[static_cast<size_t>(r.destSlot)]) continue;

                tempDegree[static_cast<size_t>(r.destSlot)]--;
                if (tempDegree[static_cast<size_t>(r.destSlot)] <= 0)
                    queue.push_back(r.destSlot);
            }
        }

        // Any slots not yet visited (part of a cycle) — append in slot order
        for (const auto& s : active)
        {
            if (!visited[static_cast<size_t>(s.slot)])
            {
                hasCycle = true;
                order.push_back(s.slot);
            }
        }

        // ── 4. Build ChainNode list from the ordered slots ───────────────────
        nodes.reserve(order.size());
        for (int slot : order)
        {
            const auto* info = bySlot[static_cast<size_t>(slot)];
            if (info)
                nodes.push_back({ info->name, info->accent, slot });
        }

        // ── 5. Build ChainLink list (one link per adjacent node pair) ────────
        // For each consecutive pair, find the best (highest-amount) route
        // connecting them in either direction.
        links.reserve(nodes.size());
        for (size_t i = 0; i + 1 < nodes.size(); ++i)
        {
            int slotA = nodes[i].slot;
            int slotB = nodes[i + 1].slot;

            const MegaCouplingMatrix::CouplingRoute* best = nullptr;
            for (const auto& r : routes)
            {
                if (!r.active || r.amount < 0.001f) continue;
                bool connects = (r.sourceSlot == slotA && r.destSlot == slotB)
                             || (r.sourceSlot == slotB && r.destSlot == slotA);
                if (!connects) continue;
                if (!best || r.amount > best->amount)
                    best = &r;
            }

            ChainLink lk;
            if (best)
            {
                lk.typeLabel  = couplingTypeLabel(best->type);
                lk.linkColor  = colorForCouplingType(best->type);
            }
            else
            {
                // No explicit route between these adjacent nodes — show a neutral
                // connector so the visual flow is preserved
                lk.typeLabel  = {};
                lk.linkColor  = juce::Colour(GalleryColors::borderGray());
            }
            links.push_back(lk);
        }
    }

    // ── paint ─────────────────────────────────────────────────────────────────
    // No allocation.  All geometry derived from cached nodes/links.
    void paint(juce::Graphics& g) override
    {
        using namespace GalleryColors;

        const auto bounds = getLocalBounds().toFloat();
        const float W = bounds.getWidth();
        const float H = bounds.getHeight();

        // ── Background ───────────────────────────────────────────────────────
        g.setColour(get(slotBg()).withAlpha(0.0f)); // transparent — sits on OverviewPanel
        g.fillAll(juce::Colours::transparentBlack);

        // ── Empty state ───────────────────────────────────────────────────────
        if (nodes.empty())
        {
            g.setColour(get(textMid()));
            g.setFont(GalleryFonts::body(9.0f));
            g.drawText("No signal chain",
                       bounds.toNearestInt(),
                       juce::Justification::centred);
            return;
        }

        // ── Geometry constants ────────────────────────────────────────────────
        constexpr float kEngineNodeW = 64.0f;
        constexpr float kEngineNodeH = 32.0f;
        constexpr float kFXNodeW     = 40.0f;
        constexpr float kOUTNodeW    = 32.0f;
        constexpr float kNodeH       = 32.0f;
        constexpr float kGapW        = 32.0f;
        constexpr float kCornerR     = 5.0f;
        constexpr float kBorderW     = 2.0f;
        constexpr float kBadgeR      = 8.0f;   // badge circle diameter
        constexpr float kArrowHead   = 5.0f;   // arrow head half-height

        // Total chain width (engine nodes + gap/arrow segments + FX + OUT)
        const int  N          = static_cast<int>(nodes.size());
        const float totalW    = N * kEngineNodeW
                              + (N - 1) * kGapW
                              + kGapW + kFXNodeW   // gap → FX
                              + kGapW + kOUTNodeW; // gap → OUT

        // Scale down if wider than component; clip at 0.5 minimum to stay legible
        const float scale     = juce::jmax(0.5f, juce::jmin(1.0f, W / totalW));
        const float nodeW     = kEngineNodeW * scale;
        const float fxW       = kFXNodeW    * scale;
        const float outW      = kOUTNodeW   * scale;
        const float nodeH     = kNodeH      * scale;
        const float gapW      = kGapW       * scale;
        const float nodeY     = (H - nodeH) * 0.5f;

        // ── Draw nodes + inter-node arrows ────────────────────────────────────
        float curX = (W - (N * nodeW + (N - 1) * gapW + gapW + fxW + gapW + outW)) * 0.5f;

        for (int i = 0; i < N; ++i)
        {
            const auto& node = nodes[static_cast<size_t>(i)];

            // Engine node rounded rect
            juce::Rectangle<float> nr(curX, nodeY, nodeW, nodeH);
            g.setColour(node.accent.withAlpha(0.12f));
            g.fillRoundedRectangle(nr, kCornerR * scale);
            g.setColour(node.accent);
            g.drawRoundedRectangle(nr, kCornerR * scale, kBorderW);

            // Engine name — Space Grotesk SemiBold, ALL CAPS, 9pt (scaled)
            g.setColour(get(textDark()));
            g.setFont(GalleryFonts::display(9.0f * scale));
            g.drawFittedText(node.engineName.toUpperCase(),
                             nr.reduced(kBadgeR * scale, 0.0f).toNearestInt(),
                             juce::Justification::centred, 1);

            // Slot badge — small circle top-left corner, 8pt diameter
            {
                const float bx = curX + 2.0f;
                const float by = nodeY + 2.0f;
                const float bd = kBadgeR * scale;
                g.setColour(node.accent.withAlpha(0.85f));
                g.fillEllipse(bx, by, bd, bd);
                g.setColour(juce::Colours::white.withAlpha(0.90f));
                g.setFont(juce::Font(juce::FontOptions(7.0f * scale)));
                g.drawText(juce::String(node.slot + 1),
                           (int)bx, (int)by, (int)bd, (int)bd,
                           juce::Justification::centred);
            }

            curX += nodeW;

            // ── Arrow + coupling label between this node and the next engine node
            if (i < N - 1)
            {
                const auto& lk = links[static_cast<size_t>(i)];
                drawArrow(g, curX, curX + gapW, nodeY + nodeH * 0.5f,
                          lk.linkColor, lk.typeLabel, scale);
                curX += gapW;
            }
        }

        // ── Arrow → FX node ───────────────────────────────────────────────────
        // Use XO Gold for the FX segment arrow
        drawArrow(g, curX, curX + gapW, nodeY + nodeH * 0.5f,
                  get(xoGold), {}, scale);
        curX += gapW;

        // FX node
        {
            juce::Rectangle<float> fxr(curX, nodeY, fxW, nodeH);
            g.setColour(get(xoGold).withAlpha(0.12f));
            g.fillRoundedRectangle(fxr, kCornerR * scale);
            g.setColour(get(xoGold));
            g.drawRoundedRectangle(fxr, kCornerR * scale, kBorderW);
            g.setColour(get(xoGoldText()));
            g.setFont(GalleryFonts::display(9.0f * scale));
            g.drawText("FX", fxr.toNearestInt(), juce::Justification::centred);
        }
        curX += fxW;

        // ── Arrow → OUT node ──────────────────────────────────────────────────
        drawArrow(g, curX, curX + gapW, nodeY + nodeH * 0.5f,
                  get(textMid()), {}, scale);
        curX += gapW;

        // OUT node — speaker glyph drawn with paths
        {
            juce::Rectangle<float> outr(curX, nodeY, outW, nodeH);
            g.setColour(get(borderGray()));
            g.drawRoundedRectangle(outr, kCornerR * scale, kBorderW);

            // Simple speaker icon: rectangle body + triangle horn
            const float cx  = curX + outW * 0.5f;
            const float cy  = nodeY + nodeH * 0.5f;
            const float spW = outW * 0.28f;
            const float spH = nodeH * 0.32f;
            const float lx  = cx - spW * 0.5f - spH * 0.7f;

            g.setColour(get(textMid()));

            // Speaker body (small rect)
            juce::Rectangle<float> body(lx, cy - spH * 0.5f, spH * 0.7f, spH);
            g.fillRect(body);

            // Horn triangle
            juce::Path horn;
            horn.addTriangle(lx + spH * 0.7f, cy - spH * 0.5f,
                             lx + spH * 0.7f, cy + spH * 0.5f,
                             cx + spW,        cy + spH * 0.85f);
            horn.addTriangle(lx + spH * 0.7f, cy - spH * 0.5f,
                             cx + spW,        cy - spH * 0.85f,
                             cx + spW,        cy + spH * 0.85f);
            g.fillPath(horn);

            // Sound wave arcs (2 concentric)
            const float arcX = cx + spW + 1.0f;
            for (int wave = 1; wave <= 2; ++wave)
            {
                const float r = spH * 0.4f * wave;
                juce::Path arc;
                arc.addArc(arcX - r, cy - r, r * 2.0f, r * 2.0f,
                           -0.55f, 0.55f, true);
                g.setColour(get(textMid()).withAlpha(0.55f));
                g.strokePath(arc, juce::PathStrokeType(1.0f * scale));
            }
        }

        // ── Cycle marker — small curved return arrow below the chain ──────────
        if (hasCycle && N > 1)
        {
            // Bracket from last engine node back to first, drawn below the strip
            const float chainStartX = (W - (N * nodeW + (N - 1) * gapW
                                            + gapW + fxW + gapW + outW)) * 0.5f;
            const float lastEngineEndX = chainStartX + N * nodeW + (N - 1) * gapW;
            const float markerY = nodeY + nodeH + 5.0f * scale;

            juce::Path cycleArc;
            cycleArc.startNewSubPath(chainStartX + nodeW * 0.5f, markerY);
            cycleArc.cubicTo(chainStartX + nodeW * 0.5f, markerY + 8.0f * scale,
                             lastEngineEndX - nodeW * 0.5f, markerY + 8.0f * scale,
                             lastEngineEndX - nodeW * 0.5f, markerY);

            // Draw with Midnight Violet (cycle = knot-like entanglement)
            g.setColour(juce::Colour(0xFF7B2FBE).withAlpha(0.55f));
            g.strokePath(cycleArc, juce::PathStrokeType(1.5f));

            // Small arrowhead at the right end
            juce::Path cycleHead;
            const float hx = lastEngineEndX - nodeW * 0.5f;
            cycleHead.addTriangle(hx, markerY,
                                  hx - 4.0f, markerY + 4.0f,
                                  hx - 4.0f, markerY - 4.0f);
            g.fillPath(cycleHead);

            // "CYCLE" micro-label centred below the arc
            g.setColour(juce::Colour(0xFF7B2FBE).withAlpha(0.70f));
            g.setFont(GalleryFonts::value(7.0f * scale));
            g.drawText("CYCLE",
                       (int)chainStartX, (int)(markerY + 10.0f * scale),
                       (int)(lastEngineEndX - chainStartX), 10,
                       juce::Justification::centred);
        }
    }

private:
    //===========================================================================
    // Cached chain data — populated in refresh(), read in paint()

    struct ChainNode {
        juce::String  engineName;
        juce::Colour  accent;
        int           slot;
    };

    struct ChainLink {
        juce::String  typeLabel;  // e.g. "Au->FM", "Ring" — empty = no route
        juce::Colour  linkColor;
    };

    XOlokunProcessor&        processor;
    std::vector<ChainNode>   nodes;  // ordered engine chain
    std::vector<ChainLink>   links;  // links[i] connects nodes[i] → nodes[i+1]
    bool                     hasCycle = false;

    //===========================================================================
    // Helpers (no allocation, called from paint())

    // Map a CouplingType to its display colour.
    // Matches CouplingArcOverlay palette exactly.
    static juce::Colour colorForCouplingType(CouplingType t) noexcept
    {
        switch (t)
        {
            case CouplingType::AudioToFM:
            case CouplingType::AudioToRing:
            case CouplingType::AudioToWavetable:
            case CouplingType::AudioToBuffer:
                return juce::Colour(0xFF0096C7); // Twilight Blue — audio-rate

            case CouplingType::KnotTopology:
                return juce::Colour(0xFF7B2FBE); // Midnight Violet — entanglement

            case CouplingType::TriangularCoupling:
                return juce::Colour(0xFFC9377A); // Hibiscus Pink — love triangle

            default:
                return juce::Colour(0xFFE9C46A); // XO Gold — modulation
        }
    }

    // Draw a horizontal arrow from x1 → x2 at vertical centre cy.
    // Optionally draws a JetBrains Mono label below the arrow midpoint.
    // All geometry is scale-adjusted.
    void drawArrow(juce::Graphics& g,
                   float x1, float x2, float cy,
                   juce::Colour colour,
                   const juce::String& label,
                   float scale) const
    {
        constexpr float kArrowHeadH = 5.0f;
        const float ah = kArrowHeadH * scale;

        // Shaft — stop 1px before the head base so there's no gap
        g.setColour(colour.withAlpha(0.75f));
        g.drawLine(x1, cy, x2 - ah, cy, 1.5f);

        // Filled triangle arrowhead
        juce::Path head;
        head.addTriangle(x2,      cy,
                         x2 - ah, cy - ah * 0.65f,
                         x2 - ah, cy + ah * 0.65f);
        g.setColour(colour);
        g.fillPath(head);

        // Type label — JetBrains Mono, 7pt, below the arrow midpoint
        if (label.isNotEmpty())
        {
            const float midX = (x1 + x2) * 0.5f;
            g.setColour(colour.withAlpha(0.85f));
            g.setFont(GalleryFonts::value(7.0f * scale));
            g.drawText(label,
                       (int)(midX - 20.0f), (int)(cy + 3.0f * scale),
                       40, (int)(10.0f * scale),
                       juce::Justification::centred);
        }
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CouplingChainView)
};

} // namespace xolokun
