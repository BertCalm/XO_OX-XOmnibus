#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "../../XOlokunProcessor.h"
#include "../../Core/MegaCouplingMatrix.h"
#include "../GalleryColors.h"

namespace xolokun
{

//==============================================================================
// Coupling type → short label, used by both OverviewPanel and CouplingPanel.
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

//==============================================================================
// OverviewPanel — right-side content when no engine is selected.
class OverviewPanel : public juce::Component
{
public:
    explicit OverviewPanel(XOlokunProcessor& proc) : processor(proc) {}

    // Called by the editor when engine state or coupling routes change.
    // Avoids calling getRoutes() (which copies a vector) inside paint().
    void refresh()
    {
        cachedActiveEngines.clear();
        for (int i = 0; i < XOlokunProcessor::MaxSlots; ++i)
        {
            auto* eng = processor.getEngine(i);
            if (eng) cachedActiveEngines.push_back({eng->getEngineId(), eng->getAccentColour()});
        }
        cachedRoutes = processor.getCouplingMatrix().getRoutes();
        repaint();
    }

    void paint(juce::Graphics& g) override
    {
        using namespace GalleryColors;
        g.fillAll(get(slotBg()));

        auto b = getLocalBounds().toFloat();
        float w = b.getWidth();
        float h = b.getHeight();

        // Active engine chain (cached in refresh() — no allocation in paint())
        const auto& active = cachedActiveEngines;

        if (active.empty())
        {
            // ── Empty state: XO Gold logo mark + instruction ──────────────────
            g.setColour(get(xoGold).withAlpha(0.18f));
            float markR = 48.0f;
            float markX = b.getCentreX() - markR;
            float markY = h * 0.30f - markR;
            g.fillEllipse(markX, markY, markR * 2, markR * 2);
            g.setColour(get(xoGold).withAlpha(0.55f));
            g.drawEllipse(markX, markY, markR * 2, markR * 2, 2.0f);
            g.setFont(GalleryFonts::display(22.0f));
            g.drawText("XO", (int)markX, (int)markY, (int)markR * 2, (int)markR * 2,
                       juce::Justification::centred);

            g.setColour(get(textMid()));
            g.setFont(GalleryFonts::body(12.0f));
            float instrY = h * 0.30f + markR + 16.0f;
            g.drawText("Click an engine tile to edit parameters",
                       b.withY(instrY).withHeight(20.0f).toNearestInt(),
                       juce::Justification::centred);

            g.setColour(get(textMid()).withAlpha(0.55f));
            g.setFont(GalleryFonts::body(10.0f));
            g.drawText("No engines loaded — use the tiles on the left",
                       b.withY(h * 0.65f).withHeight(20.0f).toNearestInt(),
                       juce::Justification::centred);
            return;
        }

        // ── Character portrait (first active engine) ──────────────────────────
        const juce::String& engineId = active[0].first;
        const juce::Colour  accent   = active[0].second;

        // Diagonal gradient wash — accent colour left edge, transparent at 70%
        juce::ColourGradient wash(accent.withAlpha(0.12f), 0.0f, 0.0f,
                                  juce::Colours::transparentBlack, w * 0.70f, 0.0f, false);
        g.setGradientFill(wash);
        g.fillAll();

        // 4 px left accent stripe
        g.setColour(accent.withAlpha(0.60f));
        g.fillRect(0.0f, 0.0f, 4.0f, h);

        // Portrait zone: top 38% of the panel
        float portraitH = h * 0.38f;

        // Engine name — large display font, anchored to bottom of portrait zone
        g.setColour(accent);
        g.setFont(GalleryFonts::display(28.0f));
        g.drawFittedText(engineId.toUpperCase(),
                         juce::Rectangle<int>(12, 0, (int)w - 20, (int)(portraitH * 0.72f)),
                         juce::Justification::centredBottom, 1);

        // Engine archetype tagline lookup
        static const std::pair<const char*, const char*> kArchetypes[] = {
            {"Opera",     "Additive-vocal Kuramoto — autonomous dramatic arcs"},
            {"Offering",  "Psychology-as-DSP boom bap — Berlyne curiosity engine"},
            {"Oware",     "Mallet physics + sympathetic resonance — material continuum"},
            {"Oxbow",     "Entangled reverb — chiasmus FDN + phase erosion"},
            {"Overbite",  "Five-macro apex predator — fang white silence"},
            {"Oceandeep", "Hydrostatic compression + bioluminescent exciter"},
            {"Orbweave",  "Topological knot coupling — trefoil/figure-eight matrices"},
            {"Overtone",  "Continued fraction spectral — \xcf\x80, e, \xcf\x86, \xe2\x88\x9a\x32 timbres"},
            {"Organism",  "Cellular automata generative — coral colony growth"},
            {"Ostinato",  "Modal membrane synthesis — world rhythm engine"},
            {"Opensky",   "Euphoric shimmer + Shepard ascension geometry"},
            {"Ouie",      "Duophonic hammerhead — 8-algorithm STRIFE/LOVE axis"},
            {"Obrix",     "Modular brick synthesis — coral reef ecology"},
            {"Oracle",    "GENDY stochastic + Maqam microtonal synthesis"},
            {"Organon",   "Variational free energy — metabolism as modulation"},
            {"Ouroboros", "Strange attractor — chaotic feedback with leash"},
            {"Obsidian",  "Crystal-clear subtractive — precision cutting tool"},
            {"Origami",   "Fold-point waveshaping — Vermillion geometry engine"},
            {"Oceanic",   "Chromatophore modulator — bioluminescent sea texture"},
            {"Ocelot",    "Biome crossfade — adaptive timbral territory"},
            {"Oblique",   "Prismatic bounce — RTJ x Funk x Tame Impala"},
            {"Osprey",    "Shore-system cultural fusion — 5 coastline identities"},
            {"Osteria",   "Porto Wine resonance — warmth-saturated harmonic mesh"},
            {"Orbital",   "Group envelope system — 8-voice dynamic architecture"},
            {"Oblong",    "Resonant string model — warm acoustic character"},
            {"Obese",     "Saturated poly — Mojo analog/digital axis"},
            {"Orphica",   "Plucked string body — velocity-brightened resonance"},
            {"Obbligato", "Breath-driven formant — obligatory melodic voice"},
            {"Ottoni",    "Patinated brass — Patina spectral character"},
            {"Onset",     "XVC cross-voice coupling — multi-circuit percussion"},
            {"Ole",       "Hibiscus drama — flamenco attack articulation engine"},
            {"Ohm",       "Sage meddling — zen macro drift and calm oscillation"},
            {"Optic",     "Zero-audio identity — visual AutoPulse modulator"},
            {"Overworld", "ERA triangle — 2D Buchla/Schulze/Vangelis crossfade"},
            {"Overdub",   "Spring reverb core — Vangelis metallic depth tail"},
            {"Opal",      "Granular clouds — textural time-stretch synthesis"},
            {"Owlfish",   "Mixtur-Trautonium oscillator — abyssal depth tones"},
            {"Odyssey",   "Wavetable drift — evolving spectral morphology"},
            {"OddOscar",  "Axolotl regeneration — morphing algorithm crossfade"},
            {"OddfeliX",  "Neon tetra filter — quick-change timbral snap"},
            {"Osmosis",   "External audio membrane — permeability coupling source"},
            {"Ombre",     "Dual-narrative blend — memory meets perception"},
            {"Orca",      "Apex predator wavetable — echolocation + breach"},
            {"Octopus",   "Decentralized alien intelligence — 8-arm chromatophore"},
            {"XOverlap",  "KnotMatrix FDN — biorthogonal voice entanglement"},
            {"XOutwit",   "Chromatophore ambush — rapid-fire spectral surprise"},
        };
        juce::String tag;
        for (const auto& [id, desc] : kArchetypes)
            if (engineId.containsIgnoreCase(id)) { tag = desc; break; }

        if (tag.isNotEmpty())
        {
            g.setColour(get(textMid()));
            g.setFont(GalleryFonts::body(9.0f));
            g.drawFittedText(tag,
                             juce::Rectangle<int>(12, (int)(portraitH * 0.74f), (int)w - 20, 28),
                             juce::Justification::centredTop, 2);
        }

        // Subtle separator line below portrait zone
        g.setColour(accent.withAlpha(0.18f));
        g.drawHorizontalLine((int)portraitH, 12.0f, w - 12.0f);

        // ── Engine chain pills + coupling routes (below portrait) ─────────────
        // Draw engine chain pills with XO Gold connectors
        float chainY = portraitH + (h - portraitH) * 0.28f;
        float pillW  = 80.0f, pillH = 24.0f;
        float totalW = active.size() * pillW + (active.size() - 1) * 24.0f;
        float startX = (b.getWidth() - totalW) * 0.5f;

        for (int i = 0; i < (int)active.size(); ++i)
        {
            float px = startX + i * (pillW + 24.0f);
            juce::Rectangle<float> pill(px, chainY - pillH * 0.5f, pillW, pillH);

            g.setColour(active[i].second.withAlpha(0.15f));
            g.fillRoundedRectangle(pill, 12.0f);
            g.setColour(active[i].second);
            g.drawRoundedRectangle(pill, 12.0f, 1.5f);
            g.setFont(GalleryFonts::heading(9.5f));
            g.drawFittedText(active[i].first.toUpperCase(),
                             pill.toNearestInt(), juce::Justification::centred, 1);

            if (i < (int)active.size() - 1)
            {
                float lx1 = px + pillW + 3.0f;
                float lx2 = px + pillW + 21.0f;
                g.setColour(get(xoGold).withAlpha(0.5f));
                g.drawLine(lx1, chainY, lx2 - 5.0f, chainY, 1.5f);
                juce::Path head;
                head.addTriangle(lx2, chainY, lx2 - 6.0f, chainY - 4.0f, lx2 - 6.0f, chainY + 4.0f);
                g.fillPath(head);
            }
        }

        // ── Mini arc node diagram — replaces text-row coupling route list ────────
        {
            const auto& routes = cachedRoutes;
            float routeY = chainY + pillH * 0.5f + 12.0f;
            float padding = 16.0f;

            // Count active routes for the summary text below the diagram
            int numActive = (int)std::count_if(routes.begin(), routes.end(),
                                               [](const auto& r){ return r.active && r.amount >= 0.005f; });

            // 4 engine nodes at corners of an 80×60 sub-rect in the panel bottom
            auto nodeArea = juce::Rectangle<float>(padding, routeY, w - 2.0f * padding, 60.0f);
            juce::Point<float> nodePos[4] = {
                nodeArea.getTopLeft().translated(8.0f, 8.0f),
                nodeArea.getTopRight().translated(-8.0f, 8.0f),
                nodeArea.getBottomLeft().translated(8.0f, -8.0f),
                nodeArea.getBottomRight().translated(-8.0f, -8.0f),
            };

            // Draw Bézier arcs for each active coupling route
            for (const auto& route : routes)
            {
                if (!route.active || route.amount < 0.005f) continue;
                if (route.sourceSlot < 0 || route.sourceSlot >= 4) continue;
                if (route.destSlot   < 0 || route.destSlot   >= 4) continue;

                auto from = nodePos[route.sourceSlot];
                auto to   = nodePos[route.destSlot];
                float midX = (from.x + to.x) * 0.5f;
                float midY = (from.y + to.y) * 0.5f - 12.0f; // bow upward

                juce::Path arc;
                arc.startNewSubPath(from);
                arc.quadraticTo(juce::Point<float>(midX, midY), to);

                // Color by coupling type — mirrors CouplingArcOverlay palette
                juce::Colour arcCol;
                switch (route.type)
                {
                    case CouplingType::AudioToFM:
                    case CouplingType::AudioToRing:
                    case CouplingType::AudioToWavetable:
                    case CouplingType::AudioToBuffer:
                        arcCol = juce::Colour(0xFF0096C7); // Twilight Blue — audio-rate
                        break;
                    case CouplingType::KnotTopology:
                        arcCol = juce::Colour(0xFF7B2FBE); // Midnight Violet — bidirectional
                        break;
                    default:
                        arcCol = juce::Colour(0xFFE9C46A); // XO Gold — modulation
                        break;
                }

                g.setColour(arcCol.withAlpha(0.35f + route.amount * 0.45f));
                g.strokePath(arc, juce::PathStrokeType(1.5f));
            }

            // Draw engine node circles (4 corner positions)
            for (int i = 0; i < 4; ++i)
            {
                bool hasEng = (i < (int)cachedActiveEngines.size());
                juce::Colour nodeCol = hasEng ? cachedActiveEngines[static_cast<size_t>(i)].second
                                              : get(emptySlot());
                g.setColour(nodeCol.withAlpha(0.70f));
                g.fillEllipse(nodePos[i].x - 6.0f, nodePos[i].y - 6.0f, 12.0f, 12.0f);
                g.setColour(nodeCol);
                g.drawEllipse(nodePos[i].x - 6.0f, nodePos[i].y - 6.0f, 12.0f, 12.0f, 1.0f);

                // Slot number label
                g.setFont(juce::Font(juce::FontOptions(7.0f)));
                g.setColour(juce::Colours::white.withAlpha(0.70f));
                g.drawText(juce::String(i + 1),
                           (int)(nodePos[i].x - 6.0f), (int)(nodePos[i].y - 6.0f), 12, 12,
                           juce::Justification::centred);
            }

            // Route count summary line below the diagram
            float summaryY = nodeArea.getBottom() + 6.0f;
            g.setColour(get(textMid()).withAlpha(0.70f));
            g.setFont(GalleryFonts::body(9.0f));
            juce::String summaryText = numActive > 0
                ? juce::String(numActive) + " active route" + (numActive > 1 ? "s" : "")
                : "No active coupling routes";
            g.drawText(summaryText,
                       b.withY(summaryY).withHeight(14.0f).toNearestInt(),
                       juce::Justification::centred);
        }
    }

private:
    XOlokunProcessor& processor;
    // Cached state — updated in refresh(), never in paint()
    std::vector<std::pair<juce::String, juce::Colour>> cachedActiveEngines;
    std::vector<MegaCouplingMatrix::CouplingRoute> cachedRoutes;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OverviewPanel)
};

} // namespace xolokun
