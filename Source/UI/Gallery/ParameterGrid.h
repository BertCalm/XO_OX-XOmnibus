#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "../../XOlokunProcessor.h"
#include "../EngineVocabulary.h"
#include "../GalleryColors.h"
#include "GalleryKnob.h"
#include "MidiLearnMouseListener.h"
#include "CockpitHost.h"
#include <set>

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
        juce::String pfx = enginePrefix.endsWith("_") ? enginePrefix : (enginePrefix + "_");

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

        // ── Cache the cutoff parameter pointer once ──────────────────────────
        // paint() reads this each frame for the Parameter Terrain tint.
        // Scanning paramSlots in paint() is O(n) per frame — cache it here
        // instead (Fix #8: APVTS scan in paint).
        {
            auto& apvts = proc.getAPVTS();
            for (auto& slot : paramSlots)
            {
                auto inner = slot.pid.fromLastOccurrenceOf("_", false, false).toLowerCase();
                juce::String pidLow = slot.pid.toLowerCase();
                if (inner.contains("cutoff") || inner.contains("filter") ||
                    inner.contains("flt")    || inner.contains("freq")   ||
                    pidLow.contains("cutoff") || pidLow.contains("filtfreq"))
                {
                    cachedCutoffParam_ = dynamic_cast<juce::RangedAudioParameter*>(
                                             apvts.getParameter(slot.pid));
                    break;
                }
            }
        }

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
    // Collapsed sections contribute only kHeaderRowH (no knob rows).
    int getRequiredHeight(int availableWidth) const
    {
        int cols = juce::jmax(1, availableWidth / kCellW);
        int y = kPad;

        for (auto& run : sectionRuns)
        {
            y += kHeaderRowH; // section header strip always visible
            if (collapsedSections.count(run.sec))
                continue; // header only, no knob rows
            int rows = (run.count + cols - 1) / cols;
            y += rows * kCellH;
        }

        return y + kPad;
    }

    // W25: Repaint when the LookAndFeel (theme) changes so colours update.
    void lookAndFeelChanged() override { repaint(); }

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

            if (collapsedSections.count(run.sec))
            {
                // Section is collapsed — place all knobs off-screen so
                // updateVisibleAttachments() will tear them down.
                for (int i = 0; i < run.count; ++i, ++flatIdx)
                    knobBounds[flatIdx] = { -1000, -1000 };
                continue; // header strip only — no knob rows added to y
            }

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
                    const int knobX = cx + (kCellW - kKnobSize) / 2;
                    lk->knob->setBounds(knobX, cy, kKnobSize, kKnobSize);
                    lk->label->setBounds(cx, cy + kKnobSize + 3, kCellW, 12);
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
        // Dark Cockpit B041: apply performance opacity
        float opacity = 1.0f;
        if (auto* host = CockpitHost::find(this))
            opacity = host->getCockpitOpacity();
        if (opacity < 0.05f) return; // B041 performance optimization
        g.setOpacity(opacity);

        // ── Parameter Terrain (Vision Quest 015) ─────────────────────────────
        // Background shifts warm (XO Gold) when filter is open, cool (Teal) when
        // closed.  3.5% opacity — barely perceptible but felt on every repaint.
        // cachedCutoffParam_ is resolved once at construction (Fix #8).
        {
            float cutoffNorm = 0.5f; // default: neutral (no filter param found)
            if (cachedCutoffParam_ != nullptr)
                cutoffNorm = cachedCutoffParam_->getValue(); // already 0-1 normalized

            // Warm = orange/gold tint when filter open (high cutoff)
            // Cool = teal tint when filter closed (low cutoff)
            juce::Colour warm(0xffE9C46A); // XO Gold
            juce::Colour cool(0xff2A9D8F); // Teal
            juce::Colour terrain = warm.interpolatedWith(cool, 1.0f - cutoffNorm);

            g.setColour(terrain.withAlpha(0.035f)); // 3.5% — felt, not seen
            g.fillRect(getLocalBounds());
        }

        int cols      = juce::jmax(1, getWidth() / kCellW);
        int y         = kPad;
        int flatIdx   = 0;
        bool firstSec = true;

        for (auto& run : sectionRuns)
        {
            juce::Colour secCol  = sectionColour(run.sec);
            juce::String secText = sectionName(run.sec);
            bool collapsed       = collapsedSections.count(run.sec) > 0;

            // ── Separator between sections — border() = rgba(255,255,255,0.07) ─
            if (!firstSec)
            {
                g.setColour(GalleryColors::border());
                g.drawHorizontalLine(y, 4.0f, (float)(getWidth() - 4));
            }
            firstSec = false;

            // ── Section header background — slightly brighter when collapsed ───
            g.setColour(collapsed ? juce::Colour(0x10FFFFFF)   // a touch more visible
                                  : juce::Colour(0x08FFFFFF)); // rgba(255,255,255,0.03)
            g.fillRect(0, y, getWidth(), kHeaderRowH);

            // ── Color dot — 7×7px, section accent color ───────────────────────
            const int dotSize = 7;
            const int dotX    = 10;
            const int dotY    = y + (kHeaderRowH - dotSize) / 2;
            g.setColour(secCol);
            g.fillEllipse((float)dotX, (float)dotY, (float)dotSize, (float)dotSize);

            // ── Section title — 9.5px, weight 600, Display, uppercase, T2 text ─
            // P6 fix: cache the section font and arrow strings as static locals
            // to avoid reconstructing them on every paint() call.
            static const juce::Font kSectionFont =
                juce::Font(juce::FontOptions{}.withTypeface(GalleryFonts::spaceGroteskBold()).withHeight(9.5f));
            static const juce::String kArrowCollapsed (juce::CharPointer_UTF8("\xe2\x96\xb8")); // ▸
            static const juce::String kArrowExpanded  (juce::CharPointer_UTF8("\xe2\x96\xbe")); // ▾

            g.setColour(GalleryColors::get(GalleryColors::t2())); // T2 text
            g.setFont(kSectionFont);
            g.drawText(secText,
                       dotX + dotSize + 6, y,
                       getWidth() - dotX - dotSize - 16, kHeaderRowH,
                       juce::Justification::centredLeft);

            // ── Collapse indicator arrow — right-aligned, T3 color, 9px ───────
            {
                g.setColour(GalleryColors::get(GalleryColors::t3()));
                g.setFont(GalleryFonts::value(9.0f));
                g.drawText(collapsed ? kArrowCollapsed : kArrowExpanded,
                           0, y, getWidth() - 10, kHeaderRowH,
                           juce::Justification::centredRight);
            }

            y += kHeaderRowH;

            if (collapsed)
            {
                // Skip knob cells — advance flatIdx only, no y advance
                flatIdx += run.count;
                continue;
            }

            // ── 3px left-border bar on each knob cell ──────────────────────
            for (int i = 0; i < run.count; ++i, ++flatIdx)
            {
                if (flatIdx >= (int)knobBounds.size()) break;
                auto [cx, cy] = knobBounds[flatIdx];
                g.setColour(secCol.withAlpha(0.45f));
                g.fillRect(cx, cy + 2, 3, kCellH - 4);
            }

            int rows = (run.count + cols - 1) / cols;
            y += rows * kCellH;
        }
    }

    // ── Mouse handling — toggle collapse on header click ────────────────────
    void mouseDown(const juce::MouseEvent& e) override
    {
        int y = kPad;
        for (auto& run : sectionRuns)
        {
            if (e.y >= y && e.y < y + kHeaderRowH)
            {
                // Toggle this section
                if (collapsedSections.count(run.sec))
                    collapsedSections.erase(run.sec);
                else
                    collapsedSections.insert(run.sec);

                // Seed the animated height interpolation — start from current
                // displayed height and glide to the new target over ~220ms.
                currentAnimHeight_ = (float)getHeight();
                targetAnimHeight_  = (float)getRequiredHeight(getWidth());
                animating_         = true;
                startTimerHz(30); // fast repaint during animation

                // Kick the layout immediately so knob off-screen positions are
                // updated (collapsed knobs get –1000,–1000) even before animation ends.
                setSize(getWidth(), (int)currentAnimHeight_);
                if (parentViewport)
                    parentViewport->setViewPosition(0, juce::jmax(0, y - 10));

                repaint();
                return;
            }

            y += kHeaderRowH;
            if (!collapsedSections.count(run.sec))
            {
                int cols = juce::jmax(1, getWidth() / kCellW);
                int rows = (run.count + cols - 1) / cols;
                y += rows * kCellH;
            }
        }
    }

    // ── Cursor change — pointing hand over section headers ──────────────────
    void mouseMove(const juce::MouseEvent& e) override
    {
        int y = kPad;
        for (auto& run : sectionRuns)
        {
            if (e.y >= y && e.y < y + kHeaderRowH)
            {
                setMouseCursor(juce::MouseCursor::PointingHandCursor);
                return;
            }
            y += kHeaderRowH;
            if (!collapsedSections.count(run.sec))
            {
                int cols = juce::jmax(1, getWidth() / kCellW);
                y += (run.count + cols - 1) / cols * kCellH;
            }
        }
        setMouseCursor(juce::MouseCursor::NormalCursor);
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
        // L-NEW-03: O(n) APVTS lookup per knob creation. Cache param pointers at init time.
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
        // Prototype: JetBrains Mono 8px, T3 color (#5E5C5A)
        lk->label->setFont(GalleryFonts::value(8.0f));
        lk->label->setColour(juce::Label::textColourId,
                             GalleryColors::get(GalleryColors::t3()));
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
            const int knobX = cx + (kCellW - kKnobSize) / 2;
            lk->knob->setBounds(knobX, cy + 2, kKnobSize, kKnobSize);
            lk->label->setBounds(cx, cy + kKnobSize + 3, kCellW, 12);
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

    // juce::Timer callback — runs at 10 Hz idle, 30 Hz during animation.
    // Handles two duties:
    //   1. Visibility-range checks for lazy knob creation/destruction.
    //   2. Height interpolation for smooth section collapse/expand (~220ms).
    void timerCallback() override
    {
        // ── Section collapse/expand animation ────────────────────────────────
        // Linear interpolation toward targetAnimHeight_.  Step size derived from
        // a fixed 220ms duration at 30 Hz (~7 frames).
        if (animating_)
        {
            constexpr float kDurMs   = 220.0f;
            constexpr float kFps     = 30.0f;
            const float     step     = std::abs(targetAnimHeight_ - currentAnimHeight_)
                                       / (kDurMs / (1000.0f / kFps));
            if (step < 1.0f || std::abs(targetAnimHeight_ - currentAnimHeight_) < 1.0f)
            {
                // Animation complete — snap to target and drop back to idle rate
                currentAnimHeight_ = targetAnimHeight_;
                animating_         = false;
                startTimerHz(10);
            }
            else
            {
                if (currentAnimHeight_ < targetAnimHeight_)
                    currentAnimHeight_ = juce::jmin(currentAnimHeight_ + step, targetAnimHeight_);
                else
                    currentAnimHeight_ = juce::jmax(currentAnimHeight_ - step, targetAnimHeight_);
            }

            setSize(getWidth(), (int)currentAnimHeight_);
            repaint();
        }

        // ── Viewport scroll visibility update ────────────────────────────────
        if (!parentViewport) return;
        int viewY = parentViewport->getViewArea().getY();
        if (viewY != lastViewY_)
        {
            lastViewY_ = viewY;
            updateVisibleAttachments();
        }
    }

    // ── Layout constants ──────────────────────────────────────────────────────
    // Prototype: 32px knobs in ~58px groups, flex-wrap with 10px/14px gap.
    // 60×52 cells fit ~8 cols in Column B's ~490px — matching the dense grid.
    static constexpr int kCellW           = 56;
    static constexpr int kCellH           = 52;
    static constexpr int kKnobSize        = 32;
    static constexpr int kPad             = 8;
    static constexpr int kHeaderRowH      = 28;  // height of each section header strip
    static constexpr int kVisibilityMargin = 100; // px preload margin for smooth scrolling

    // ── Collapse state — which sections are currently collapsed ──────────────
    // All sections start expanded by default (set is empty at construction).
    std::set<Section> collapsedSections;

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

    // ── Cached cutoff parameter pointer (Fix #8: avoid APVTS scan in paint()) ─
    // Resolved once at construction; nullptr if this engine has no filter param.
    juce::RangedAudioParameter* cachedCutoffParam_ = nullptr;

    // ── Dirty-flag for 10 Hz timer (FIX 11) ──────────────────────────────────
    int lastViewY_ = -1; // cached scroll position; -1 forces first update

    // ── Section collapse/expand animation state ───────────────────────────────
    // When a section header is clicked, currentAnimHeight_ glides to
    // targetAnimHeight_ over ~220ms at 30 Hz.
    float currentAnimHeight_ = 0.0f;
    float targetAnimHeight_  = 0.0f;
    bool  animating_         = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ParameterGrid)
};

} // namespace xolokun
