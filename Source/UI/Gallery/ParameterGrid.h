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
//
// Lazy attachment: GalleryKnob / Label / SliderAttachment are only created when
// a slot scrolls within kVisibilityMargin px of the visible area.  Slots that
// scroll back out of range are torn down.  paint() and getRequiredHeight() still
// use the full param list so scrolling geometry is always correct.
class ParameterGrid : public juce::Component,
                      private juce::Timer
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
    // Collects all matching parameter IDs and sorts them into sections, but
    // does NOT create any GalleryKnob / Label / SliderAttachment yet.
    // midiLearn: optional — when non-null every knob gets a right-click MIDI
    // learn context menu and shows visual feedback (amber ring / green badge).
    ParameterGrid(XOlokunProcessor& proc_,
                  const juce::String& engId,
                  const juce::String& enginePrefix,
                  juce::Colour accentColour_,
                  MIDILearnManager* midiLearn_ = nullptr)
        : proc(proc_)
        , accentColour(accentColour_)
        , midiLearn(midiLearn_)
    {
        auto& rawParams = proc.getParameters(); // juce::AudioProcessor::getParameters()
        juce::String pfx = enginePrefix + "_";

        // ── Collect params into per-section buckets ─────────────────────────
        // 6 buckets — index matches Section enum value (OSC=0 … OTHER=5)
        std::vector<ParamSlot> buckets[6];

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

            int startIdx = (int)paramSlots.size();

            for (auto& entry : bucket)
                paramSlots.push_back(entry);

            sectionRuns.push_back({ orderedSec, startIdx,
                                    (int)paramSlots.size() - startIdx });
        }

        // Pre-allocate live knob slots (all null at start)
        liveKnobs.resize(paramSlots.size());

        // Start 10 Hz timer for visibility checks
        startTimerHz(10);
    }

    ~ParameterGrid() override
    {
        stopTimer();
        // Tear down all live knobs in safe destruction order
        for (auto& lk : liveKnobs)
            destroyLiveKnob(lk);
    }

    // ── Viewport linkage ────────────────────────────────────────────────────
    void setParentViewport(juce::Viewport* vp)
    {
        parentViewport = vp;
    }

    // ── Height calculation — accounts for per-section header rows ───────────
    // Always returns the full height as if all knobs exist (viewport needs this).
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

        knobBounds.resize(paramSlots.size());
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

                knobBounds[flatIdx] = { cx, cy };

                // If the live knob already exists, update its bounds immediately
                if (auto& lk = liveKnobs[flatIdx]; lk != nullptr)
                {
                    lk->knob->setBounds(cx + 6, cy + 4, kCellW - 12, kCellW - 12);
                    lk->label->setBounds(cx, cy + kCellW - 4, kCellW, 14);
                }
            }

            int rows = (run.count + cols - 1) / cols;
            y += rows * kCellH;
        }

        // After geometry changes, immediately update which knobs are live
        updateVisibleAttachments();
    }

    // ── Paint — section headers + left-border bars on each knob cell ────────
    // Knob widgets are painted by JUCE automatically; this draws the chrome.
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
    // ── Per-parameter metadata (never destroyed after construction) ──────────
    struct ParamSlot
    {
        juce::String pid;
        juce::String shortLabel;
        Section      sec;
    };

    // ── Live widget bundle — created/destroyed lazily ────────────────────────
    // Destruction order inside destroyLiveKnob():
    //   1. attachment (references the slider — must die first)
    //   2. midiLearnListener (references the knob — must die before knob)
    //   3. label
    //   4. knob
    struct LiveKnob
    {
        std::unique_ptr<GalleryKnob>   knob;
        std::unique_ptr<juce::Label>   label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
        std::unique_ptr<MidiLearnMouseListener> midiLearnListener;
    };

    // Section run descriptor — one entry per non-empty section, in display order
    struct SectionRun { Section sec; int startIdx; int count; };

    // ── Helpers ──────────────────────────────────────────────────────────────

    // "filterCutoff" → "CUTOFF", "level" → "LEVEL", "ampAttack" → "ATTACK"
    static juce::String makeShortLabel(const juce::String& inner)
    {
        for (int i = 1; i < inner.length(); ++i)
            if (juce::CharacterFunctions::isUpperCase(inner[i]))
                return inner.substring(i).toUpperCase();
        return inner.toUpperCase();
    }

    // Safely tear down one live knob bundle.
    // Must be called on the message thread.
    void destroyLiveKnob(std::unique_ptr<LiveKnob>& lk)
    {
        if (!lk) return;
        // 1. Attachment must die before slider (it holds a reference to the slider)
        lk->attachment.reset();
        // 2. De-register the MIDI learn mouse listener from the knob BEFORE deleting
        //    the listener — the knob's internal listener list holds a raw pointer.
        if (lk->midiLearnListener && lk->knob)
            lk->knob->removeMouseListener(lk->midiLearnListener.get());
        lk->midiLearnListener.reset();
        // 3. Label — remove from component hierarchy, then destroy
        if (lk->label)
        {
            removeChildComponent(lk->label.get());
            lk->label.reset();
        }
        // 4. Knob — remove from component hierarchy, then destroy
        if (lk->knob)
        {
            removeChildComponent(lk->knob.get());
            lk->knob.reset();
        }
        lk.reset();
    }

    // Create a live knob for paramSlots[idx] and place it at knobBounds[idx].
    // Must be called on the message thread.
    void createLiveKnob(int idx)
    {
        jassert(idx >= 0 && idx < (int)paramSlots.size());
        jassert(!liveKnobs[idx]);

        auto& slot    = paramSlots[idx];
        auto& apvts   = proc.getAPVTS();
        auto* rp      = dynamic_cast<juce::RangedAudioParameter*>(
                            apvts.getParameter(slot.pid));
        if (!rp) return; // should never happen

        auto lk = std::make_unique<LiveKnob>();

        // ── Knob ────────────────────────────────────────────────────────────
        lk->knob = std::make_unique<GalleryKnob>();
        lk->knob->setSliderStyle(juce::Slider::RotaryVerticalDrag);
        lk->knob->setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        lk->knob->setColour(juce::Slider::rotarySliderFillColourId, accentColour);
        lk->knob->setTooltip(rp->getName(64));
        A11y::setup(*lk->knob, rp->getName(64),
                    rp->getName(64) + " (" + slot.pid + ")");
        addAndMakeVisible(*lk->knob);

        // ── Label ────────────────────────────────────────────────────────────
        lk->label = std::make_unique<juce::Label>();
        lk->label->setText(slot.shortLabel, juce::dontSendNotification);
        lk->label->setFont(GalleryFonts::label(8.0f));
        lk->label->setColour(juce::Label::textColourId,
                             GalleryColors::get(GalleryColors::textMid()));
        lk->label->setJustificationType(juce::Justification::centred);
        lk->label->setInterceptsMouseClicks(false, false);
        addAndMakeVisible(*lk->label);

        // ── SliderAttachment (must come AFTER knob + label exist) ────────────
        lk->attachment =
            std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                apvts, slot.pid, *lk->knob);

        enableKnobReset(*lk->knob, apvts, slot.pid);

        // ── MIDI learn ───────────────────────────────────────────────────────
        if (midiLearn)
        {
            auto* ml = lk->knob->setupMidiLearn(slot.pid, *midiLearn);
            // setupMidiLearn returns a raw pointer; take ownership here.
            // The listener is already registered on the knob — we just own it.
            lk->midiLearnListener.reset(ml);
        }

        // ── Position ─────────────────────────────────────────────────────────
        if (idx < (int)knobBounds.size())
        {
            auto [cx, cy] = knobBounds[idx];
            lk->knob->setBounds(cx + 6, cy + 4, kCellW - 12, kCellW - 12);
            lk->label->setBounds(cx, cy + kCellW - 4, kCellW, 14);
        }

        liveKnobs[idx] = std::move(lk);
    }

    // Called by the 10 Hz timer and from resized().
    // Creates knobs that entered the padded-visible-rect, destroys those that left.
    void updateVisibleAttachments()
    {
        if (knobBounds.empty()) return;

        // Work out the visible rect in our own coordinate space.
        // If no viewport is set we treat the entire component as visible.
        juce::Rectangle<int> visibleInSelf;
        if (parentViewport)
        {
            // getViewArea() is in viewport coords; translate to our coords.
            auto viewArea = parentViewport->getViewArea();
            // Our position relative to the viewport's content area origin:
            auto ourOrigin = getPosition(); // relative to our parent (usually the viewport's content component)
            visibleInSelf = viewArea.translated(-ourOrigin.x, -ourOrigin.y);
        }
        else
        {
            visibleInSelf = getLocalBounds();
        }

        // Expand by margin to pre-create knobs just before they appear
        auto paddedRect = visibleInSelf.expanded(0, kVisibilityMargin);

        for (int i = 0; i < (int)paramSlots.size(); ++i)
        {
            if (i >= (int)knobBounds.size()) break;

            auto [cx, cy] = knobBounds[i];
            juce::Rectangle<int> cellRect(cx, cy, kCellW, kCellH);

            bool shouldBeAlive = paddedRect.intersects(cellRect);

            if (shouldBeAlive && !liveKnobs[i])
                createLiveKnob(i);
            else if (!shouldBeAlive && liveKnobs[i])
                destroyLiveKnob(liveKnobs[i]);
        }
    }

    // juce::Timer callback at 10 Hz
    void timerCallback() override
    {
        updateVisibleAttachments();
    }

    // ── Layout constants ──────────────────────────────────────────────────────
    static constexpr int kCellW           = 82;
    static constexpr int kCellH           = 90;
    static constexpr int kPad             = 12;
    static constexpr int kHeaderRowH      = 22;  // height of each section header strip
    static constexpr int kVisibilityMargin = 100; // px preload margin for smooth scrolling

    // ── Construction-time state captured for lazy creation ───────────────────
    XOlokunProcessor& proc;
    juce::Colour      accentColour;
    MIDILearnManager* midiLearn = nullptr;

    // ── Parameter metadata (stable, never modified after construction) ────────
    std::vector<ParamSlot>  paramSlots;  // flat ordered list of all params
    std::vector<SectionRun> sectionRuns; // ordered section descriptors

    // ── Live widget state (created/destroyed lazily) ──────────────────────────
    // Parallel to paramSlots — index i corresponds to paramSlots[i].
    std::vector<std::unique_ptr<LiveKnob>> liveKnobs;

    // ── Layout geometry (pre-computed in resized()) ───────────────────────────
    std::vector<std::pair<int,int>> knobBounds; // (cx, cy) per slot

    // ── Viewport reference ────────────────────────────────────────────────────
    juce::Viewport* parentViewport = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ParameterGrid)
};

} // namespace xolokun
