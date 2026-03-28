#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "../../Core/MegaCouplingMatrix.h"
#include "../../Core/EngineRegistry.h"
#include "../GalleryColors.h"

namespace xolokun {

//==============================================================================
// CouplingStripEditor — Visual editor for MegaCouplingMatrix routes.
//
// Displays active engines as colored nodes with coupling arcs between them.
// Users can add/remove/adjust coupling routes by clicking connection points
// and dragging to set amount.
//
class CouplingStripEditor : public juce::Component
{
public:
    CouplingStripEditor(MegaCouplingMatrix& matrix,
                        std::function<juce::String(int)> slotNameFn,
                        std::function<juce::Colour(int)> slotColorFn)
        : couplingMatrix(matrix),
          getSlotName(std::move(slotNameFn)),
          getSlotColor(std::move(slotColorFn))
    {
        setTitle ("Coupling Matrix");
        setDescription ("Visual editor for cross-engine modulation routes. "
                        "Shows engine nodes with coupling arcs between them.");
        setWantsKeyboardFocus (true);
    }

    void refresh()
    {
        cachedRoutes = couplingMatrix.loadRoutes();
        repaint();
    }

    void paint(juce::Graphics& g) override
    {
        auto b = getLocalBounds().toFloat();

        // Dark background
        g.fillAll(juce::Colour(GalleryColors::surface()));

        // Header
        g.setColour(juce::Colour(GalleryColors::xoGold));
        g.setFont(GalleryFonts::display(10.0f));
        g.drawText("COUPLING MATRIX", b.removeFromTop(20), juce::Justification::centred);

        // Draw engine nodes
        float nodeRadius = 28.0f;
        auto mainArea = b.reduced(10, 5);
        float spacing = mainArea.getWidth() / (float)(MegaCouplingMatrix::MaxSlots);

        std::array<juce::Point<float>, MegaCouplingMatrix::MaxSlots> nodePositions;
        for (int i = 0; i < MegaCouplingMatrix::MaxSlots; ++i)
        {
            float cx = mainArea.getX() + spacing * (i + 0.5f);
            float cy = mainArea.getCentreY();
            nodePositions[(size_t)i] = { cx, cy };

            auto name = getSlotName(i);
            auto color = getSlotColor(i);
            bool hasEngine = name.isNotEmpty();

            // Node circle
            g.setColour(hasEngine ? color.withAlpha(0.2f) : juce::Colour(GalleryColors::elevated()));
            g.fillEllipse(cx - nodeRadius, cy - nodeRadius, nodeRadius * 2, nodeRadius * 2);
            g.setColour(hasEngine ? color : GalleryColors::border());
            g.drawEllipse(cx - nodeRadius, cy - nodeRadius, nodeRadius * 2, nodeRadius * 2, 2.0f);

            // Slot number
            g.setFont(GalleryFonts::value(8.0f));
            g.drawText(juce::String(i + 1), cx - 4, cy + nodeRadius + 2, 10, 10,
                       juce::Justification::centred);

            // Engine name
            if (hasEngine)
            {
                g.setColour(juce::Colour(GalleryColors::t1()));
                g.setFont(GalleryFonts::label(9.0f));
                g.drawText(name, cx - nodeRadius, cy - 6, nodeRadius * 2, 12,
                           juce::Justification::centred);
            }
            else
            {
                g.setColour(juce::Colour(GalleryColors::t3()));
                g.setFont(GalleryFonts::body(8.0f));
                g.drawText("empty", cx - nodeRadius, cy - 5, nodeRadius * 2, 10,
                           juce::Justification::centred);
            }
        }

        // Draw coupling arcs
        if (cachedRoutes)
        {
            for (const auto& route : *cachedRoutes)
            {
                if (!route.active || route.amount < 0.001f) continue;
                if (route.sourceSlot < 0 || route.sourceSlot >= MegaCouplingMatrix::MaxSlots) continue;
                if (route.destSlot < 0 || route.destSlot >= MegaCouplingMatrix::MaxSlots) continue;

                auto srcPos = nodePositions[(size_t)route.sourceSlot];
                auto dstPos = nodePositions[(size_t)route.destSlot];
                auto srcColor = getSlotColor(route.sourceSlot);
                auto dstColor = getSlotColor(route.destSlot);

                // Offset arcs vertically to avoid overlap
                float yOff = (route.sourceSlot < route.destSlot) ? -20.0f : 20.0f;

                juce::Path arc;
                arc.startNewSubPath(srcPos.x, srcPos.y - nodeRadius);
                arc.cubicTo(srcPos.x, srcPos.y - nodeRadius + yOff * 2,
                           dstPos.x, dstPos.y - nodeRadius + yOff * 2,
                           dstPos.x, dstPos.y - nodeRadius);

                float alpha = 0.3f + route.amount * 0.7f;
                float thickness = 1.0f + route.amount * 2.0f;

                g.setColour(route.isNormalled
                    ? juce::Colour(GalleryColors::xoGold).withAlpha(alpha * 0.5f)
                    : srcColor.interpolatedWith(dstColor, 0.5f).withAlpha(alpha));
                g.strokePath(arc, juce::PathStrokeType(thickness,
                    juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

                // Arrow head at destination
                float arrowSize = 4.0f + route.amount * 4.0f;
                g.fillEllipse(dstPos.x - arrowSize * 0.5f,
                             dstPos.y - nodeRadius - arrowSize * 0.5f,
                             arrowSize, arrowSize);

                // Type label at midpoint (WCAG: min 8pt for readability)
                float midX = (srcPos.x + dstPos.x) * 0.5f;
                float midY = (srcPos.y + dstPos.y) * 0.5f + yOff * 1.5f - nodeRadius;
                g.setFont(GalleryFonts::label(8.0f));
                g.drawText(couplingTypeShortLabel(route.type),
                          (int)(midX - 24), (int)(midY - 6), 48, 12,
                          juce::Justification::centred);
            }
        }

        // Route count
        int activeCount = 0;
        if (cachedRoutes)
            for (const auto& r : *cachedRoutes)
                if (r.active && r.amount >= 0.001f) ++activeCount;

        g.setColour(juce::Colour(GalleryColors::t2()));
        g.setFont(GalleryFonts::body(8.0f));
        g.drawText(juce::String(activeCount) + " active route" + (activeCount != 1 ? "s" : ""),
                   getLocalBounds().removeFromBottom(14).toFloat(),
                   juce::Justification::centred);
    }

    void mouseDown(const juce::MouseEvent& e) override
    {
        // Click on a node pair to toggle routes (future: drag to add new routes)
        // For now, refresh display
        refresh();
    }

private:
    MegaCouplingMatrix& couplingMatrix;
    std::function<juce::String(int)> getSlotName;
    std::function<juce::Colour(int)> getSlotColor;
    std::shared_ptr<std::vector<MegaCouplingMatrix::CouplingRoute>> cachedRoutes;

    static juce::String couplingTypeShortLabel(CouplingType type)
    {
        switch (type)
        {
            case CouplingType::AmpToFilter:     return "Amp>F";
            case CouplingType::AmpToPitch:      return "Amp>P";
            case CouplingType::LFOToPitch:      return "LFO>P";
            case CouplingType::EnvToMorph:      return "Env>M";
            case CouplingType::AudioToFM:       return "FM";
            case CouplingType::AudioToRing:     return "Ring";
            case CouplingType::FilterToFilter:   return "F>F";
            case CouplingType::AmpToChoke:      return "Choke";
            case CouplingType::RhythmToBlend:   return "Rhy>B";
            case CouplingType::EnvToDecay:      return "Env>D";
            case CouplingType::PitchToPitch:    return "P>P";
            case CouplingType::AudioToWavetable: return "WT";
            case CouplingType::AudioToBuffer:    return "A>B";
            default: return "?";
        }
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CouplingStripEditor)
};

} // namespace xolokun
