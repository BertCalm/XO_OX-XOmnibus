#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "../../XOlokunProcessor.h"
#include "../GalleryColors.h"
#include "GalleryKnob.h"
#include "MidiLearnMouseListener.h"

// clang-format off
namespace xolokun {

//==============================================================================
// DrumPadGrid — V1-REQUIRED shared component for percussion engines.
//
// Embeds inside EngineDetailPanel when a percussion engine (ONSET, OFFERING)
// is loaded. Shows an 8-voice (4×2) or 16-voice (4×4) trigger pad grid with
// per-voice parameter knobs (Tune / Decay / Level) that appear below the grid
// when a pad is selected.
//
// Architecture
// ─────────────
//   • ParameterDiscovery: scans APVTS at construction for params matching
//     `{prefix}_voice{N}_*` (or `{prefix}_voice{N}{suffix}`). Groups them
//     by voice number. If no per-voice params are found the param strip is
//     hidden (grid-only mode).
//   • PadCell: lightweight POD struct — bounds + voice index. All hit state
//     is kept here (hitAlpha float decayed by the 10Hz timer).
//   • Param strip: 3 GalleryKnobs (Tune / Decay / Level) created on pad
//     selection and destroyed on deselection. Only the selected voice's
//     attachments are alive at any time (per spec constraint).
//   • MIDI: mouseDown triggers a MidiMessageCollector note-on via
//     processor.getMidiCollector(). Base note = 37 (KAI-P0-03 MPC A01).
//     Velocity derived from Y position within pad (top=soft, bottom=hard).
//   • Hit flash: accent color fill at velocity-scaled alpha; 10Hz timer
//     fades hitAlpha by kFadeStep per tick (200ms = 2 ticks × 100ms).
//
// Depth-zone gradient for pad backgrounds:
//   Row 0 (top)  → kSunlit   (surface — bright, warm)
//   Last row     → kMidnight (deep — dark, saturated)
//
// KAI-P0-03: base MIDI note = 37 (MPC pad A01), not 36.
//
// Gallery code: DrumPadGrid | depends on: GalleryKnob, GalleryColors, A11y
//==============================================================================

class DrumPadGrid : public juce::Component,
                    private juce::Timer
{
public:
    // ── Construction ──────────────────────────────────────────────────────────
    DrumPadGrid(XOlokunProcessor& proc_,
                const juce::String& enginePrefix_,
                juce::Colour accentColour_,
                int numVoices_ = 8)
        : proc(proc_)
        , enginePrefix(enginePrefix_)
        , accentColour(accentColour_)
        , numVoices(juce::jlimit(1, 16, numVoices_))
    {
        discoverVoiceParams();
        initPadCells();
        buildParamStrip();   // creates param row sub-components (hidden until first selection)
        startTimerHz(10);    // hit-flash decay + lazy strip visibility

        A11y::setup(*this,
                    "Drum Pad Grid",
                    juce::String(numVoices) + "-voice pad grid. Click a pad to trigger and select it.");
    }

    ~DrumPadGrid() override
    {
        stopTimer();
        destroyParamStrip();
    }

    // ── Public API ────────────────────────────────────────────────────────────

    // Total pixel height needed for a given available width.
    // Accounts for grid + optional param strip.
    int getRequiredHeight(int availableWidth) const
    {
        int cols = columnsForWidth(availableWidth);
        int rows = (numVoices + cols - 1) / cols;
        int gridH = kPadGap + rows * (kPadSize + kPadGap);

        int stripH = hasVoiceParams ? (kParamStripH + kParamStripPad) : 0;
        return kTopPad + gridH + stripH + kBottomPad;
    }

    // ── juce::Component overrides ─────────────────────────────────────────────

    void paint(juce::Graphics& g) override
    {
        using namespace GalleryColors;

        // ── Background ───────────────────────────────────────────────────────
        g.fillAll(get(slotBg()));

        int cols = columnsForWidth(getWidth());
        int rows = (numVoices + cols - 1) / cols;

        // ── Pad cells ────────────────────────────────────────────────────────
        for (int i = 0; i < numVoices; ++i)
        {
            auto& cell = padCells[i];
            auto  r    = cell.bounds.toFloat();

            int row = i / cols;

            // Depth-zone gradient: top row = Sunlit, bottom row = Midnight
            float t = (rows > 1) ? (float)row / (float)(rows - 1) : 0.0f;
            juce::Colour depthTop    = juce::Colour(kSunlit);
            juce::Colour depthBottom = juce::Colour(kMidnight);
            juce::Colour padBg       = depthTop.interpolatedWith(depthBottom, t);

            // ── Pad background (rounded rect) ─────────────────────────────
            g.setColour(padBg);
            g.fillRoundedRectangle(r, kPadCorner);

            // ── Hit flash — accent color at velocity alpha, fading ────────
            if (cell.hitAlpha > 0.001f)
            {
                g.setColour(accentColour.withAlpha(cell.hitAlpha));
                g.fillRoundedRectangle(r, kPadCorner);
            }

            // ── Selected pad: 2px accent border ──────────────────────────
            if (i == selectedPad)
            {
                g.setColour(accentColour);
                g.drawRoundedRectangle(r.reduced(1.0f), kPadCorner, 2.0f);
            }
            else
            {
                // Subtle border for unselected pads
                g.setColour(get(borderGray()).withAlpha(0.40f));
                g.drawRoundedRectangle(r, kPadCorner, 1.0f);
            }

            // ── Pad number — top-left, JetBrains Mono 8pt ────────────────
            g.setColour(juce::Colours::white.withAlpha(0.55f));
            g.setFont(GalleryFonts::value(8.0f));
            g.drawText(juce::String(i + 1),
                       cell.bounds.getX() + 4,
                       cell.bounds.getY() + 3,
                       20, 12,
                       juce::Justification::centredLeft);

            // ── Voice name — center, Space Grotesk SemiBold 9pt ───────────
            // Use the per-voice label if discovered; otherwise "V{N}"
            juce::String voiceName = (i < (int)voiceLabels.size() && voiceLabels[i].isNotEmpty())
                                        ? voiceLabels[i]
                                        : ("V" + juce::String(i + 1));
            g.setColour(juce::Colours::white.withAlpha(0.85f));
            g.setFont(juce::Font(juce::FontOptions{}.withTypeface(GalleryFonts::spaceGroteskBold()).withHeight(9.0f)));
            g.drawText(voiceName, cell.bounds, juce::Justification::centred);
        }

        // ── Param strip header label ──────────────────────────────────────────
        if (hasVoiceParams && selectedPad >= 0)
        {
            int stripY = getParamStripY();
            g.setColour(accentColour.withAlpha(0.80f));
            g.setFont(juce::Font(juce::FontOptions{}.withTypeface(GalleryFonts::spaceGroteskBold()).withHeight(9.0f)));
            juce::String header = "VOICE " + juce::String(selectedPad + 1);
            g.drawText(header, kPadGap, stripY - kParamHeaderH, getWidth() - kPadGap * 2, kParamHeaderH,
                       juce::Justification::centredLeft);

            // Divider line above param strip
            g.setColour(accentColour.withAlpha(0.20f));
            g.drawHorizontalLine(stripY - kParamHeaderH - 2,
                                 (float)kPadGap, (float)(getWidth() - kPadGap));
        }
    }

    void resized() override
    {
        layoutPadCells();
        layoutParamStrip();
    }

    void mouseDown(const juce::MouseEvent& e) override
    {
        // Right-click falls through to JUCE default (context menus etc.)
        if (e.mods.isRightButtonDown())
            return;

        // Hit-test: find which pad was clicked
        for (int i = 0; i < numVoices; ++i)
        {
            if (padCells[i].bounds.contains(e.getPosition()))
            {
                triggerPad(i, e);
                return;
            }
        }
    }

private:
    // ── Internal constants ────────────────────────────────────────────────────

    static constexpr int     kPadSize        = 56;   // minimum pad side (pt)
    static constexpr int     kPadGap         = 4;    // gap between pads
    static constexpr float   kPadCorner      = 8.0f; // rounded-rect radius
    static constexpr int     kTopPad         = 8;
    static constexpr int     kBottomPad      = 8;
    static constexpr int     kParamStripH    = 80;   // height of knob row
    static constexpr int     kParamStripPad  = 4;
    static constexpr int     kParamHeaderH   = 16;   // "VOICE N" label row
    static constexpr int     kNumStripKnobs  = 3;    // Tune / Decay / Level
    static constexpr float   kFadeStep       = 0.5f; // hit alpha per 100ms tick → 200ms decay
    static constexpr int     kBaseMidiNote   = 37;   // KAI-P0-03: MPC A01

    // Depth-zone pad background colors
    static constexpr uint32_t kSunlit   = 0xFF48CAE4; // bright surface water
    static constexpr uint32_t kMidnight = 0xFF1A0A2E; // deep trench violet

    // ── Per-pad cell ──────────────────────────────────────────────────────────
    struct PadCell
    {
        int               voiceIndex = 0;
        juce::Rectangle<int> bounds;
        float             hitAlpha = 0.0f;  // 0..1 flash strength, decays over time
    };

    // ── Per-voice discovered params ───────────────────────────────────────────
    struct VoiceParams
    {
        juce::String tuneId;   // e.g. "perc_voice1_tune"  — empty if absent
        juce::String decayId;  // e.g. "perc_voice1_decay" — empty if absent
        juce::String levelId;  // e.g. "perc_voice1_level" — empty if absent
    };

    // ── Strip knob bundle (created/destroyed on pad selection) ────────────────
    struct StripKnob
    {
        std::unique_ptr<GalleryKnob>   knob;
        std::unique_ptr<juce::Label>   label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
        juce::Rectangle<int>           bounds;
        bool                           active = false; // true when wired to a param
    };

    // ── Parameter discovery ────────────────────────────────────────────────────
    // Scans APVTS for params matching `{prefix}_voice{N}_*`.
    // Also accepts flattened names: `{prefix}_voice{N}{Suffix}` (no underscore
    // between digit and suffix) so engines that name params "voice1tune" work too.
    //
    // Discovery is called once at construction; results are stored in voiceParamMap
    // and voiceLabels.
    void discoverVoiceParams()
    {
        voiceParamMap.assign((size_t)numVoices, VoiceParams{});
        voiceLabels.assign((size_t)numVoices, {});
        hasVoiceParams = false;

        auto& rawParams = proc.getParameters();
        juce::String pfx = enginePrefix + "_";

        // Build a per-voice discovery map.
        // We look for params whose ID starts with `pfx` and contains "voice"
        // followed by at least one digit, then optionally "_", then a suffix.
        for (auto* p : rawParams)
        {
            if (auto* rp = dynamic_cast<juce::RangedAudioParameter*>(p))
            {
                juce::String pid = rp->getParameterID();
                if (!pid.startsWith(pfx)) continue;

                // Inner part after the prefix underscore, e.g. "voice1_tune"
                juce::String inner = pid.substring(pfx.length()).toLowerCase();

                // Find "voice" keyword
                int voicePos = inner.indexOf("voice");
                if (voicePos < 0) continue;

                // Extract digit(s) after "voice"
                int digitStart = voicePos + 5; // length of "voice"
                int digitEnd   = digitStart;
                while (digitEnd < inner.length() &&
                       juce::CharacterFunctions::isDigit(inner[digitEnd]))
                    ++digitEnd;

                if (digitEnd == digitStart) continue; // no digit found

                int voiceIndex = inner.substring(digitStart, digitEnd).getIntValue() - 1;
                if (voiceIndex < 0 || voiceIndex >= numVoices) continue;

                // Remainder is the suffix (may start with "_")
                juce::String suffix = inner.substring(digitEnd);
                if (suffix.startsWith("_")) suffix = suffix.substring(1);

                VoiceParams& vp = voiceParamMap[(size_t)voiceIndex];

                if (suffix == "tune")  { vp.tuneId  = pid; hasVoiceParams = true; }
                if (suffix == "decay") { vp.decayId = pid; hasVoiceParams = true; }
                if (suffix == "level") { vp.levelId = pid; hasVoiceParams = true; }
                // Additional suffixes are discovered but not shown in the strip.
                // If a voice name param exists (e.g. "voice1_name") it is not a
                // RangedAudioParameter so it won't appear here.
            }
        }
    }

    // ── Pad cell initialization ───────────────────────────────────────────────
    void initPadCells()
    {
        padCells.resize((size_t)numVoices);
        for (int i = 0; i < numVoices; ++i)
            padCells[(size_t)i].voiceIndex = i;
    }

    // ── Grid layout ───────────────────────────────────────────────────────────
    // 4 columns always (spec: 4×2 for 8-voice, 4×4 for 16-voice).
    static constexpr int kGridCols = 4;

    int columnsForWidth(int /*w*/) const
    {
        return kGridCols;
    }

    void layoutPadCells()
    {
        int cols  = kGridCols;
        int totalW = getWidth() - kPadGap * 2; // total pad area width
        int padW  = (totalW - kPadGap * (cols - 1)) / cols;
        int padH  = juce::jmax(kPadSize, padW); // square-ish; min 56pt

        int x0 = kPadGap;
        int y0 = kTopPad;

        for (int i = 0; i < numVoices; ++i)
        {
            int col = i % cols;
            int row = i / cols;
            int x = x0 + col * (padW + kPadGap);
            int y = y0 + kPadGap + row * (padH + kPadGap);
            padCells[(size_t)i].bounds = { x, y, padW, padH };
        }
    }

    // ── Param strip — creation, layout, destruction ───────────────────────────

    // Called once from constructor.  Creates the three StripKnob slots
    // (knob + label widgets without attachments) and hides them initially.
    void buildParamStrip()
    {
        static const char* kLabels[kNumStripKnobs] = { "TUNE", "DECAY", "LEVEL" };

        stripKnobs.resize(kNumStripKnobs);
        for (int i = 0; i < kNumStripKnobs; ++i)
        {
            auto& sk = stripKnobs[(size_t)i];

            sk.knob  = std::make_unique<GalleryKnob>();
            sk.label = std::make_unique<juce::Label>();

            sk.knob->setSliderStyle(juce::Slider::RotaryVerticalDrag);
            sk.knob->setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
            sk.knob->setColour(juce::Slider::rotarySliderFillColourId, accentColour);
            sk.knob->setVisible(false);

            sk.label->setText(kLabels[i], juce::dontSendNotification);
            sk.label->setFont(GalleryFonts::label(8.0f));
            sk.label->setColour(juce::Label::textColourId,
                                GalleryColors::get(GalleryColors::textMid()));
            sk.label->setJustificationType(juce::Justification::centred);
            sk.label->setInterceptsMouseClicks(false, false);
            sk.label->setVisible(false);

            addAndMakeVisible(*sk.knob);
            addAndMakeVisible(*sk.label);
        }
    }

    // Wire attachments for the given voice (called when a pad is selected).
    // Tears down any existing attachment first (per spec: don't hold all at once).
    void activateParamStripForVoice(int voiceIndex)
    {
        if (!hasVoiceParams) return;
        if (voiceIndex < 0 || voiceIndex >= numVoices) return;

        // Destroy old attachments first (attachment must die before slider)
        for (auto& sk : stripKnobs)
        {
            sk.attachment.reset();
            sk.active = false;
        }

        auto& apvts = proc.getAPVTS();
        auto& vp    = voiceParamMap[(size_t)voiceIndex];

        // Helper to wire one strip knob if the param exists
        auto wire = [&](int slotIndex, const juce::String& pid, const juce::String& tooltip)
        {
            if (pid.isEmpty()) return;
            if (!apvts.getParameter(pid)) return;

            auto& sk = stripKnobs[(size_t)slotIndex];
            sk.knob->setTooltip(tooltip);
            A11y::setup(*sk.knob, tooltip,
                        tooltip + " for voice " + juce::String(voiceIndex + 1));
            sk.attachment =
                std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                    apvts, pid, *sk.knob);
            enableKnobReset(*sk.knob, apvts, pid);
            sk.active = true;
        };

        wire(0, vp.tuneId,  "Tune");
        wire(1, vp.decayId, "Decay");
        wire(2, vp.levelId, "Level");

        // Show knobs that have an active param; hide those that don't
        for (auto& sk : stripKnobs)
        {
            sk.knob->setVisible(sk.active);
            sk.label->setVisible(sk.active);
        }

        layoutParamStrip();
        repaint();
    }

    // Hide all strip knobs and destroy their attachments.
    void deactivateParamStrip()
    {
        for (auto& sk : stripKnobs)
        {
            sk.attachment.reset();
            sk.active = false;
            sk.knob->setVisible(false);
            sk.label->setVisible(false);
        }
    }

    // Called from destroyParamStrip (destructor path) — also resets knob/label.
    void destroyParamStrip()
    {
        for (auto& sk : stripKnobs)
        {
            // 1. Attachment references the slider — must die first
            sk.attachment.reset();
            // 2. Remove from component hierarchy, then destroy
            if (sk.label)
            {
                removeChildComponent(sk.label.get());
                sk.label.reset();
            }
            if (sk.knob)
            {
                removeChildComponent(sk.knob.get());
                sk.knob.reset();
            }
        }
        stripKnobs.clear();
    }

    int getParamStripY() const
    {
        // Strip starts immediately after the grid rows, plus header row
        int cols  = kGridCols;
        int rows  = (numVoices + cols - 1) / cols;
        int totalW = getWidth() - kPadGap * 2;
        int padW  = (totalW - kPadGap * (cols - 1)) / cols;
        int padH  = juce::jmax(kPadSize, padW);

        return kTopPad + kPadGap + rows * (padH + kPadGap) + kParamStripPad + kParamHeaderH;
    }

    void layoutParamStrip()
    {
        if (!hasVoiceParams) return;
        if (stripKnobs.empty()) return;

        int stripY = getParamStripY();
        int activeCount = 0;
        for (auto& sk : stripKnobs)
            if (sk.active) ++activeCount;

        if (activeCount == 0) return;

        const int kKnobSize = 48;
        const int kLabelH   = 14;
        int totalKnobW = activeCount * kKnobSize + (activeCount - 1) * kPadGap;
        int startX = (getWidth() - totalKnobW) / 2;
        int knobY  = stripY;
        int labelY = knobY + kKnobSize + 2;

        int xCursor = startX;
        for (auto& sk : stripKnobs)
        {
            if (!sk.active) continue;
            sk.knob->setBounds(xCursor, knobY, kKnobSize, kKnobSize);
            sk.label->setBounds(xCursor - 4, labelY, kKnobSize + 8, kLabelH);
            xCursor += kKnobSize + kPadGap;
        }
    }

    // ── Pad trigger ───────────────────────────────────────────────────────────
    void triggerPad(int voiceIndex, const juce::MouseEvent& e)
    {
        // Velocity from Y position: top of pad = soft (0.2), bottom = hard (1.0)
        float relY = 0.0f;
        if (padCells[(size_t)voiceIndex].bounds.getHeight() > 0)
        {
            int localY = e.getPosition().y - padCells[(size_t)voiceIndex].bounds.getY();
            relY = (float)localY / (float)padCells[(size_t)voiceIndex].bounds.getHeight();
            relY = juce::jlimit(0.0f, 1.0f, relY);
        }
        float velocity = 0.20f + relY * 0.80f; // 0.20 … 1.0

        // Hit flash
        padCells[(size_t)voiceIndex].hitAlpha = velocity;

        // Selection + param strip
        if (selectedPad != voiceIndex)
        {
            selectedPad = voiceIndex;
            if (hasVoiceParams)
                activateParamStripForVoice(voiceIndex);
        }

        // Send MIDI note-on via processor's MIDI collector.
        // Channel 10 = standard GM percussion channel.
        // addMessageToQueue handles timestamp internally — no setTimeStamp needed.
        int midiNote = juce::jlimit(0, 127, kBaseMidiNote + voiceIndex); // 37+ (KAI-P0-03)
        proc.getMidiCollector().addMessageToQueue(
            juce::MidiMessage::noteOn(10, midiNote, velocity));

        repaint();
    }

    // ── Timer callback — hit flash decay + no-op pad-off ─────────────────────
    void timerCallback() override
    {
        bool needsRepaint = false;
        for (auto& cell : padCells)
        {
            if (cell.hitAlpha > 0.0f)
            {
                cell.hitAlpha = juce::jmax(0.0f, cell.hitAlpha - kFadeStep);
                needsRepaint = true;
            }
        }
        if (needsRepaint) repaint();
    }

    // ── Members ───────────────────────────────────────────────────────────────

    XOlokunProcessor& proc;
    juce::String      enginePrefix;
    juce::Colour      accentColour;
    int               numVoices;

    // Discovered param IDs per voice (size == numVoices)
    std::vector<VoiceParams> voiceParamMap;
    std::vector<juce::String> voiceLabels;    // display name per voice (currently empty; reserved)
    bool hasVoiceParams = false;

    // Pad cells (size == numVoices)
    std::vector<PadCell> padCells;

    // Selection state
    int selectedPad = -1; // -1 = none selected

    // Param strip knobs (always kNumStripKnobs = 3 entries; active flag controls visibility)
    // Destruction order: attachments → labels → knobs (see destroyParamStrip).
    std::vector<StripKnob> stripKnobs;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DrumPadGrid)
};

} // namespace xolokun
// clang-format on
