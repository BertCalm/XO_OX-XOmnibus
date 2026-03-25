#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "../GalleryColors.h"
#include "../../Export/XOutshine.h"

namespace xolokun {

class OutshineZoneMap : public juce::Component,
                        public juce::TooltipClient
{
public:
    std::function<void(int grainIndex)> onZoneClicked;

    OutshineZoneMap()
    {
        setWantsKeyboardFocus(false);
        A11y::setup(*this, "Zone Map", "Keyboard zone visualization showing sample mapping");
        setMouseCursor(juce::MouseCursor::PointingHandCursor);
    }

    void setSamples(const std::vector<AnalyzedSample>& samples)
    {
        zones.clear();
        if (samples.empty()) { repaint(); return; }

        // Sort by detected MIDI note
        struct IndexedNote { int midi; int index; float confidence; };
        std::vector<IndexedNote> sorted;
        for (int i = 0; i < (int)samples.size(); ++i)
            sorted.push_back({ samples[(size_t)i].detectedMidiNote, i, samples[(size_t)i].pitchConfidence });
        std::sort(sorted.begin(), sorted.end(), [](auto& a, auto& b) { return a.midi < b.midi; });

        // Compute zone boundaries using midpoint rule
        for (int i = 0; i < (int)sorted.size(); ++i)
        {
            int low  = (i == 0) ? kMidiLow : (sorted[(size_t)(i-1)].midi + sorted[(size_t)i].midi) / 2;
            int high = (i == (int)sorted.size() - 1) ? kMidiHigh
                       : (sorted[(size_t)i].midi + sorted[(size_t)(i+1)].midi) / 2 - 1;

            zones.push_back({
                sorted[(size_t)i].midi,
                low, high,
                sorted[(size_t)i].index,
                sorted[(size_t)i].confidence < 0.15f,
                midiToNoteName(sorted[(size_t)i].midi)
            });
        }
        repaint();
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(GalleryColors::get(GalleryColors::slotBg()));
        auto bounds = getLocalBounds().toFloat();

        for (int i = 0; i < (int)zones.size(); ++i)
        {
            const auto& z = zones[(size_t)i];
            float x1 = midiToX(z.lowMidi);
            float x2 = midiToX(z.highMidi + 1);
            juce::Rectangle<float> zoneRect(x1, 0, x2 - x1, bounds.getHeight() - 18.0f);

            g.setColour(GalleryColors::get(GalleryColors::xoGold).withAlpha(0.30f));
            g.fillRect(zoneRect);

            g.setColour(GalleryColors::get(GalleryColors::borderGray()));
            g.drawVerticalLine((int)x1, 0.0f, bounds.getHeight() - 18.0f);

            g.setColour(GalleryColors::get(GalleryColors::textDark()));
            g.setFont(GalleryFonts::value(9.0f));
            g.drawText(z.noteName,
                       (int)((x1 + x2) / 2.0f - 16), (int)(bounds.getHeight() - 16), 32, 14,
                       juce::Justification::centred);

            if (z.pitchUnverified)
            {
                g.setColour(juce::Colour(0xFFE9A84A));
                g.setFont(GalleryFonts::body(10.0f));
                g.drawText("!", (int)x1 + 2, 2, 12, 14, juce::Justification::centredLeft);
            }
        }

        // White/black key pattern (5% opacity overlay)
        g.setColour(GalleryColors::get(GalleryColors::textDark()).withAlpha(0.05f));
        for (int midi = kMidiLow; midi <= kMidiHigh; ++midi)
        {
            bool isBlack = (midi % 12 == 1 || midi % 12 == 3 || midi % 12 == 6 ||
                            midi % 12 == 8 || midi % 12 == 10);
            if (isBlack)
            {
                float x = midiToX(midi);
                float w = midiToX(midi + 1) - x;
                g.fillRect(x, 0.0f, w, (bounds.getHeight() - 18.0f) * 0.6f);
            }
        }

        if (hasKeyboardFocus(false))
        {
            g.setColour(GalleryColors::get(GalleryColors::xoGold));
            g.drawRect(getLocalBounds(), 1);
        }
    }

    void mouseDown(const juce::MouseEvent& e) override
    {
        int idx = hitZone((float)e.x);
        if (idx >= 0 && onZoneClicked)
            onZoneClicked(zones[(size_t)idx].grainIndex);
    }

    juce::String getTooltip() override
    {
        auto pos = getMouseXYRelative();
        int idx = hitZone((float)pos.x);
        if (idx < 0) return {};
        const auto& z = zones[(size_t)idx];
        return midiToNoteName(z.lowMidi) + " - " + midiToNoteName(z.highMidi)
               + " | Root: " + z.noteName
               + (z.pitchUnverified ? " (unverified)" : "");
    }

private:
    struct ZoneEntry
    {
        int    rootMidi;
        int    lowMidi;
        int    highMidi;
        int    grainIndex;
        bool   pitchUnverified;
        juce::String noteName;
    };

    float midiToX(int midiNote) const
    {
        return (float)(midiNote - kMidiLow) / (float)(kMidiHigh - kMidiLow + 1) * (float)getWidth();
    }

    int xToMidi(float x) const
    {
        return kMidiLow + (int)(x / (float)getWidth() * (float)(kMidiHigh - kMidiLow + 1));
    }

    int hitZone(float x) const
    {
        int midi = xToMidi(x);
        for (int i = 0; i < (int)zones.size(); ++i)
            if (midi >= zones[(size_t)i].lowMidi && midi <= zones[(size_t)i].highMidi)
                return i;
        return -1;
    }

    static juce::String midiToNoteName(int midi)
    {
        static const char* names[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
        int octave = (midi / 12) - 2;  // C-2 = MIDI 0
        return juce::String(names[midi % 12]) + juce::String(octave);
    }

    std::vector<ZoneEntry> zones;
    int hoveredZone { -1 };

    static constexpr int kMidiLow  = 0;
    static constexpr int kMidiHigh = 108;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OutshineZoneMap)
};

} // namespace xolokun
