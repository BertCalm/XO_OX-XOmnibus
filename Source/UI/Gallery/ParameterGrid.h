#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "../../XOlokunProcessor.h"
#include "../EngineVocabulary.h"
#include "../GalleryColors.h"
#include "GalleryKnob.h"
#include "MidiLearnMouseListener.h"

namespace xolokun
{

//==============================================================================
// ParameterGrid — auto-generates knobs for all params matching a prefix.
// Mounted inside a Viewport for scrolling.
//
// Parameters are sorted into color-coded sections (OSC / FILTER / MOD / FX /
// MACRO / OTHER) based on keywords in the parameter inner name.  Each section
// gets a Space Grotesk Bold 10pt ALL-CAPS header with a color dot, and every
// knob cell receives a subtle 3px left-border bar in the section color.
class ParameterGrid : public juce::Component
{
public:
    // ── Section identity ────────────────────────────────────────────────────
    enum class Section { OSC, FILTER, MOD, FX, MACRO, OTHER };

    // Classify a parameter by its inner name (part after prefix_).
    static Section classifyParam(const juce::String& inner)
    {
        juce::String s = inner.toLowerCase();

        // MACRO checked first — "macroCharacter" must not fall through to OSC/OTHER
        if (s.contains("macro") || s.contains("character") ||
            s.contains("movement") || s.contains("coupling") ||
            s.contains("space"))
            return Section::MACRO;

        // OSC
        if (s.contains("osc") || s.contains("wave") || s.contains("tune") ||
            s.contains("pitch") || s.contains("detune") || s.contains("harm") ||
            s.contains("partial") || s.contains("src"))
            return Section::OSC;

        // FILTER
        if (s.contains("filter") || s.contains("cutoff") || s.contains("reso") ||
            s.contains("freq") || s.contains("filt") || s.contains("flt") ||
            s.contains("svf") || s.contains("q"))
            return Section::FILTER;

        // MOD
        if (s.contains("lfo") || s.contains("mod") || s.contains("env") ||
            s.contains("attack") || s.contains("decay") || s.contains("sustain") ||
            s.contains("release") || s.contains("rate") || s.contains("depth") ||
            s.contains("amount") || s.contains("adsr"))
            return Section::MOD;

        // FX
        if (s.contains("delay") || s.contains("reverb") || s.contains("chorus") ||
            s.contains("drive") || s.contains("sat") || s.contains("wet") ||
            s.contains("dry") || s.contains("mix") || s.contains("fx") ||
            s.contains("distort") || s.contains("compress"))
            return Section::FX;

        return Section::OTHER;
    }

    static juce::Colour sectionColour(Section s)
    {
        switch (s)
        {
            case Section::OSC:    return juce::Colour(0xFF48CAE4); // Sunlit blue
            case Section::FILTER: return juce::Colour(0xFFFF6B6B); // Warm red
            case Section::MOD:    return juce::Colour(0xFF00FF41); // Phosphor green
            case Section::FX:     return juce::Colour(0xFFBF40FF); // Prism violet
            case Section::MACRO:  return juce::Colour(0xFFE9C46A); // XO Gold
            default:              return juce::Colour(0xFF9E9B97); // text-muted
        }
    }

    static const char* sectionName(Section s)
    {
        switch (s)
        {
            case Section::OSC:    return "OSC";
            case Section::FILTER: return "FILTER";
            case Section::MOD:    return "MOD";
            case Section::FX:     return "FX";
            case Section::MACRO:  return "MACRO";
            default:              return "OTHER";
        }
    }

    // ── Constructor ─────────────────────────────────────────────────────────
    // midiLearn: optional — when non-null every knob gets a right-click MIDI
    // learn context menu and shows visual feedback (amber ring / green badge).
    ParameterGrid(XOlokunProcessor& proc,
                  const juce::String& engId,
                  const juce::String& enginePrefix,
                  juce::Colour accentColour,
                  MIDILearnManager* midiLearn = nullptr)
    {
        auto& apvts = proc.getAPVTS();
        auto& rawParams = proc.getParameters(); // juce::AudioProcessor::getParameters()
        juce::String pfx = enginePrefix + "_";

        // ── Collect params into per-section buckets ─────────────────────────
        struct ParamEntry { juce::String pid; juce::String shortLabel; Section sec; };
        // 6 buckets — index matches Section enum value (OSC=0 … OTHER=5)
        std::vector<ParamEntry> buckets[6];

        for (auto* p : rawParams)
        {
            if (auto* rp = dynamic_cast<juce::RangedAudioParameter*>(p))
            {
                juce::String pid = rp->getParameterID();
                if (!pid.startsWith(pfx))
                    continue;

                auto inner = pid.substring(pfx.length()); // e.g. "filterCutoff"
                juce::String shortLabel = EngineVocabulary::labelFor(
                    engId, pid, makeShortLabel(inner));

                Section sec = classifyParam(inner);
                buckets[static_cast<int>(sec)].push_back({ pid, shortLabel, sec });
            }
        }

        // ── Flatten buckets → ordered param list; record section runs ───────
        // Display order: OSC, FILTER, MOD, FX, MACRO, OTHER
        constexpr Section kOrder[] = {
            Section::OSC, Section::FILTER, Section::MOD,
            Section::FX,  Section::MACRO,  Section::OTHER
        };

        for (auto orderedSec : kOrder)
        {
            auto& bucket = buckets[static_cast<int>(orderedSec)];
            if (bucket.empty()) continue;

            int startIdx = (int)sliders.size();

            for (auto& entry : bucket)
            {
                auto* rp = dynamic_cast<juce::RangedAudioParameter*>(
                    apvts.getParameter(entry.pid));
                if (!rp) continue;

                auto slider = std::make_unique<GalleryKnob>();
                slider->setSliderStyle(juce::Slider::RotaryVerticalDrag);
                slider->setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
                slider->setColour(juce::Slider::rotarySliderFillColourId, accentColour);
                slider->setTooltip(rp->getName(64));
                A11y::setup(*slider, rp->getName(64),
                            rp->getName(64) + " (" + entry.pid + ")");
                addAndMakeVisible(*slider);

                auto label = std::make_unique<juce::Label>();
                label->setText(entry.shortLabel, juce::dontSendNotification);
                label->setFont(GalleryFonts::label(8.0f));
                label->setColour(juce::Label::textColourId,
                                 GalleryColors::get(GalleryColors::textMid()));
                label->setJustificationType(juce::Justification::centred);
                label->setInterceptsMouseClicks(false, false);
                addAndMakeVisible(*label);

                attachments.push_back(
                    std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                        apvts, entry.pid, *slider));

                enableKnobReset (*slider, apvts, entry.pid);

                // Wire MIDI learn if manager is available
                if (midiLearn)
                {
                    auto* ml = slider->setupMidiLearn(entry.pid, *midiLearn);
                    midiLearnListeners.emplace_back(ml);
                }

                sliders.push_back(std::move(slider));
                labels.push_back(std::move(label));
            }

            sectionRuns.push_back({ orderedSec, startIdx, (int)sliders.size() - startIdx });
        }
    }

    // ── Height calculation — accounts for per-section header rows ───────────
    int getRequiredHeight(int availableWidth) const
    {
        int cols = juce::jmax(1, availableWidth / kCellW);
        int y = kPad;

        for (auto& run : sectionRuns)
        {
            y += kHeaderRowH; // section header strip
            int rows = (run.count + cols - 1) / cols;
            y += rows * kCellH;
        }

        return y + kPad;
    }

    void resized() override
    {
        int cols    = juce::jmax(1, getWidth() / kCellW);
        int offsetX = (getWidth() - cols * kCellW) / 2;
        int y       = kPad;

        knobBounds.resize(sliders.size());
        int flatIdx = 0;

        for (auto& run : sectionRuns)
        {
            y += kHeaderRowH; // skip over section header

            for (int i = 0; i < run.count; ++i, ++flatIdx)
            {
                int col = i % cols;
                int row = i / cols;
                int cx  = offsetX + col * kCellW;
                int cy  = y + row * kCellH;

                sliders[flatIdx]->setBounds(cx + 6, cy + 4, kCellW - 12, kCellW - 12);
                labels[flatIdx]->setBounds(cx, cy + kCellW - 4, kCellW, 14);
                knobBounds[flatIdx] = { cx, cy };
            }

            int rows = (run.count + cols - 1) / cols;
            y += rows * kCellH;
        }
    }

    // ── Paint — section headers + left-border bars on each knob cell ────────
    void paint(juce::Graphics& g) override
    {
        int cols      = juce::jmax(1, getWidth() / kCellW);
        int y         = kPad;
        int flatIdx   = 0;
        bool firstSec = true;

        for (auto& run : sectionRuns)
        {
            juce::Colour secCol  = sectionColour(run.sec);
            juce::String secText = sectionName(run.sec);

            // ── Separator line above every section except the first ─────────
            if (!firstSec)
            {
                g.setColour(secCol.withAlpha(0.20f));
                g.drawHorizontalLine(y, 4.0f, (float)(getWidth() - 4));
            }
            firstSec = false;

            // ── Section header background tint ─────────────────────────────
            g.setColour(secCol.withAlpha(0.06f));
            g.fillRect(0, y, getWidth(), kHeaderRowH);

            // ── Color dot (6×6 circle, vertically centered in header row) ──
            const int dotSize = 6;
            const int dotX    = 10;
            const int dotY    = y + (kHeaderRowH - dotSize) / 2;
            g.setColour(secCol);
            g.fillEllipse((float)dotX, (float)dotY, (float)dotSize, (float)dotSize);

            // ── Section name — Space Grotesk Bold 10pt, ALL CAPS ───────────
            g.setColour(secCol.brighter(0.15f));
            g.setFont(juce::Font(GalleryFonts::spaceGroteskBold()).withHeight(10.0f));
            g.drawText(secText,
                       dotX + dotSize + 6, y,
                       getWidth() - dotX - dotSize - 16, kHeaderRowH,
                       juce::Justification::centredLeft);

            y += kHeaderRowH;

            // ── 3px left-border bar on each knob cell ──────────────────────
            for (int i = 0; i < run.count; ++i, ++flatIdx)
            {
                if (flatIdx >= (int)knobBounds.size()) break;
                auto [cx, cy] = knobBounds[flatIdx];
                g.setColour(secCol.withAlpha(0.50f));
                g.fillRect(cx, cy + 2, 3, kCellH - 4);
            }

            int rows = (run.count + cols - 1) / cols;
            y += rows * kCellH;
        }
    }

private:
    // Section run descriptor — one entry per non-empty section, in display order
    struct SectionRun { Section sec; int startIdx; int count; };

    // "filterCutoff" → "CUTOFF", "level" → "LEVEL", "ampAttack" → "ATTACK"
    static juce::String makeShortLabel(const juce::String& inner)
    {
        for (int i = 1; i < inner.length(); ++i)
            if (juce::CharacterFunctions::isUpperCase(inner[i]))
                return inner.substring(i).toUpperCase();
        return inner.toUpperCase();
    }

    static constexpr int kCellW      = 82;
    static constexpr int kCellH      = 90;
    static constexpr int kPad        = 12;
    static constexpr int kHeaderRowH = 22; // height of each section header strip

    // Destruction order matters: listeners → attachments → sliders.
    // Members are destroyed in reverse declaration order.
    std::vector<std::unique_ptr<GalleryKnob>> sliders;
    std::vector<std::unique_ptr<juce::Label>>  labels;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> attachments;
    // MIDI learn mouse listeners — owned here, attached to individual sliders.
    // Must be declared AFTER sliders so listeners are destroyed before sliders.
    std::vector<std::unique_ptr<MidiLearnMouseListener>> midiLearnListeners;

    // Section layout metadata — populated during construction / resized()
    std::vector<SectionRun>         sectionRuns; // ordered section descriptors
    std::vector<std::pair<int,int>>  knobBounds;  // (cx, cy) per slider, filled in resized()

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ParameterGrid)
};

} // namespace xolokun
